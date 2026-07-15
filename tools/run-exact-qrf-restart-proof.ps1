[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[Alias("WorkbenchExecutablePath")]
	[string] $ExecutablePath,

	[string] $ProjectPath = "",

	[string] $ProfileOutputRoot = "",

	[ValidateSet("before_refund", "after_refund", "after_receipt")]
	[string[]] $Cut = @("before_refund", "after_refund", "after_receipt"),

	[ValidateRange(10, 3600)]
	[int] $StageTimeoutSeconds = 300,

	[ValidateRange(100, 5000)]
	[int] $PollMilliseconds = 500,

	[ValidateRange(1, 60)]
	[int] $ResultGraceSeconds = 5,

	[string[]] $AdditionalArguments = @(),

	[string] $ProjectSwitch = "-gproj",

	[string] $WorldSwitch = "-world",

	[string] $ProfileSwitch = "-profile",

	[switch] $Cleanup
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$script:ResultMagic = "partisan_exact_qrf_restart_result_v1"
$script:CarrierMagic = "partisan_exact_qrf_restart_carrier_v1"
$script:GuardMagic = "partisan_exact_qrf_restart_guard_v1"
$script:LauncherOwnerMagic = "partisan_exact_qrf_restart_launcher_owner_v1"
$script:LauncherSessionMagic = "partisan_exact_qrf_restart_launcher_session_v1"

function Resolve-AbsolutePath
{
	param(
		[Parameter(Mandatory = $true)]
		[string] $Path,

		[string] $BasePath = ""
	)

	if ([System.IO.Path]::IsPathRooted($Path))
	{
		return [System.IO.Path]::GetFullPath($Path)
	}

	if ([string]::IsNullOrWhiteSpace($BasePath))
	{
		$BasePath = (Get-Location).Path
	}

	return [System.IO.Path]::GetFullPath((Join-Path $BasePath $Path))
}

function Write-JsonUtf8NoBom
{
	param(
		[Parameter(Mandatory = $true)]
		[string] $Path,

		[Parameter(Mandatory = $true)]
		[object] $Value
	)

	$json = $Value | ConvertTo-Json -Depth 12
	$encoding = New-Object System.Text.UTF8Encoding($false)
	[System.IO.File]::WriteAllText($Path, $json, $encoding)
}

function Read-JsonFile
{
	param(
		[Parameter(Mandatory = $true)]
		[string] $Path
	)

	return (Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json)
}

function Read-CheckoutBuildIdentity
{
	param(
		[Parameter(Mandatory = $true)]
		[string] $RepositoryRoot
	)

	$buildInfoPath = Join-Path $RepositoryRoot "Scripts\Game\HST\HST_BuildInfo.c"
	$campaignStatePath = Join-Path $RepositoryRoot "Scripts\Game\HST\State\HST_CampaignState.c"
	if (!(Test-Path -LiteralPath $buildInfoPath -PathType Leaf) -or
		!(Test-Path -LiteralPath $campaignStatePath -PathType Leaf))
	{
		throw "checkout build identity sources are missing"
	}

	$buildInfoText = Get-Content -Raw -LiteralPath $buildInfoPath
	$campaignStateText = Get-Content -Raw -LiteralPath $campaignStatePath
	$shaMatch = [regex]::Match($buildInfoText, 'static\s+const\s+string\s+BUILD_SHA\s*=\s*"([^"]+)"')
	$utcMatch = [regex]::Match($buildInfoText, 'static\s+const\s+string\s+BUILD_UTC\s*=\s*"([^"]+)"')
	$labelMatch = [regex]::Match($buildInfoText, 'static\s+const\s+string\s+BUILD_LABEL\s*=\s*"([^"]+)"')
	$schemaMatch = [regex]::Match($campaignStateText, 'static\s+const\s+int\s+SCHEMA_VERSION\s*=\s*([0-9]+)')
	if (!$shaMatch.Success -or !$utcMatch.Success -or !$labelMatch.Success -or
		!$schemaMatch.Success)
	{
		throw "checkout build identity constants could not be parsed"
	}

	return [pscustomobject]@{
		BuildSha = $shaMatch.Groups[1].Value
		BuildUtc = $utcMatch.Groups[1].Value
		BuildLabel = $labelMatch.Groups[1].Value
		CampaignSchemaVersion = [int]$schemaMatch.Groups[1].Value
	}
}

function Assert-JsonProperty
{
	param(
		[Parameter(Mandatory = $true)]
		[object] $Value,

		[Parameter(Mandatory = $true)]
		[string] $PropertyName,

		[Parameter(Mandatory = $true)]
		[string] $ArtifactLabel
	)

	if ($null -eq $Value -or $Value.PSObject.Properties.Name -notcontains $PropertyName)
	{
		throw "$ArtifactLabel is missing JSON property $PropertyName"
	}
}

function Assert-StageResult
{
	param(
		[Parameter(Mandatory = $true)]
		[object] $Result,

		[Parameter(Mandatory = $true)]
		[string] $RunId,

		[Parameter(Mandatory = $true)]
		[string] $CutName,

		[Parameter(Mandatory = $true)]
		[string] $Stage
	)

	$label = "$CutName/$Stage result"
	foreach ($propertyName in @(
		"m_sMagic",
		"m_sRunId",
		"m_sStage",
		"m_bSuccess",
		"m_sBuildSha",
		"m_sBuildUtc",
		"m_sBuildLabel",
		"m_iCampaignSchemaVersion",
		"m_sCutName",
		"m_iCut",
		"m_bRestored",
		"m_bStartupReconcileChanged",
		"m_bSourceExact",
		"m_bSameStateNoOp",
		"m_bPersistedReadBackExact",
		"m_sFingerprint",
		"m_sEvidence"
	))
	{
		Assert-JsonProperty -Value $Result -PropertyName $propertyName -ArtifactLabel $label
	}

	if ([string]$Result.m_sMagic -ne $script:ResultMagic)
	{
		throw "$label has an unexpected magic value"
	}
	if ([string]$Result.m_sRunId -cne $RunId)
	{
		throw "$label belongs to a different run"
	}
	if ([string]$Result.m_sStage -cne $Stage)
	{
		throw "$label reports a different stage"
	}
	if ([string]$Result.m_sCutName -cne $CutName)
	{
		throw "$label reports a different cut"
	}
	if ($Result.m_bSuccess -isnot [bool])
	{
		throw "$label success value is not a JSON boolean"
	}
	foreach ($booleanProperty in @(
		"m_bRestored",
		"m_bStartupReconcileChanged",
		"m_bSourceExact",
		"m_bSameStateNoOp",
		"m_bPersistedReadBackExact"
	))
	{
		if ($Result.$booleanProperty -isnot [bool])
		{
			throw "$label $booleanProperty is not a JSON boolean"
		}
	}
	if ([string]::IsNullOrWhiteSpace([string]$Result.m_sBuildSha) -or
		[string]::IsNullOrWhiteSpace([string]$Result.m_sBuildUtc) -or
		[string]::IsNullOrWhiteSpace([string]$Result.m_sBuildLabel))
	{
		throw "$label is missing exact build identity"
	}
	if ([int]$Result.m_iCampaignSchemaVersion -le 0)
	{
		throw "$label has an invalid campaign schema"
	}
	if ($Result.m_bSuccess -and
		[string]::IsNullOrWhiteSpace([string]$Result.m_sFingerprint))
	{
		throw "$label is missing its state fingerprint"
	}
	if ([string]::IsNullOrWhiteSpace([string]$Result.m_sEvidence))
	{
		throw "$label is missing evidence"
	}
	if ($Stage -eq "prepare" -and
		($Result.m_bRestored -or
			$Result.m_bStartupReconcileChanged -or
			!$Result.m_bSourceExact -or
			$Result.m_bSameStateNoOp -or
			!$Result.m_bPersistedReadBackExact))
	{
		throw "$label violates the fresh PREPARED-source invariant"
	}
	if ($Stage -eq "recover" -and
		(!$Result.m_bRestored -or
			!$Result.m_bStartupReconcileChanged -or
			!$Result.m_bSourceExact -or
			!$Result.m_bSameStateNoOp -or
			!$Result.m_bPersistedReadBackExact))
	{
		throw "$label violates the first-start recovery invariant"
	}
	if ($Stage -eq "replay" -and
		(!$Result.m_bRestored -or
			$Result.m_bStartupReconcileChanged -or
			!$Result.m_bSourceExact -or
			!$Result.m_bSameStateNoOp -or
			!$Result.m_bPersistedReadBackExact))
	{
		throw "$label violates the second-start no-op invariant"
	}

	return $Result
}

function Assert-PreparedCarrier
{
	param(
		[Parameter(Mandatory = $true)]
		[object] $Carrier,

		[Parameter(Mandatory = $true)]
		[string] $RunId,

		[Parameter(Mandatory = $true)]
		[string] $CutName,

		[Parameter(Mandatory = $true)]
		[object] $PrepareResult
	)

	$label = "$CutName carrier"
	foreach ($propertyName in @(
		"m_sMagic",
		"m_sRunId",
		"m_sBuildSha",
		"m_iCampaignSchemaVersion",
		"m_sCutName",
		"m_iCut",
		"m_sPreparedFingerprint"
	))
	{
		Assert-JsonProperty -Value $Carrier -PropertyName $propertyName -ArtifactLabel $label
	}

	if ([string]$Carrier.m_sMagic -ne $script:CarrierMagic -or
		[string]$Carrier.m_sRunId -cne $RunId -or
		[string]$Carrier.m_sCutName -cne $CutName)
	{
		throw "$label identity does not match its launcher guard"
	}
	if ([string]$Carrier.m_sBuildSha -cne [string]$PrepareResult.m_sBuildSha -or
		[int]$Carrier.m_iCampaignSchemaVersion -ne [int]$PrepareResult.m_iCampaignSchemaVersion)
	{
		throw "$label build or schema does not match the prepare result"
	}
	if ([string]::IsNullOrWhiteSpace([string]$Carrier.m_sPreparedFingerprint))
	{
		throw "$label is missing its prepared fingerprint"
	}

	return $Carrier
}

function Assert-LaunchArgumentIsolation
{
	param(
		[AllowEmptyCollection()]
		[Parameter(Mandatory = $true)]
		[string[]] $ExtraArguments,

		[Parameter(Mandatory = $true)]
		[string[]] $ConfiguredSwitches
	)

	foreach ($configuredSwitch in $ConfiguredSwitches)
	{
		if ([string]::IsNullOrWhiteSpace($configuredSwitch) -or
			$configuredSwitch -notmatch '^-[A-Za-z][A-Za-z0-9_-]*$')
		{
			throw "launch switch must be one standalone command-line token: $configuredSwitch"
		}
	}
	if (($ConfiguredSwitches | Select-Object -Unique).Count -ne $ConfiguredSwitches.Count)
	{
		throw "project, world, and profile switches must be distinct"
	}

	$reservedPrefixes = @(
		"-addonsdir",
		"-project",
		"-gproj",
		"-world",
		"-profile",
		"-hstexactqrfrestartstage",
		"-hstexactqrfrestartrunid",
		"-hstexactqrfrestartcut",
		"-hstcampaigndebugprofile"
	)
	foreach ($configuredSwitch in $ConfiguredSwitches)
	{
		$reservedPrefixes += $configuredSwitch.ToLowerInvariant()
	}

	foreach ($extraArgument in $ExtraArguments)
	{
		$normalized = ([string]$extraArgument).Trim().ToLowerInvariant()
		foreach ($reservedPrefix in $reservedPrefixes)
		{
			if ($normalized -eq $reservedPrefix -or
				$normalized.StartsWith($reservedPrefix + "="))
			{
				throw "AdditionalArguments cannot override proof isolation switch $reservedPrefix"
			}
		}
	}
}

function ConvertTo-WindowsCommandLineArgument
{
	param(
		[AllowEmptyString()]
		[Parameter(Mandatory = $true)]
		[string] $Argument
	)

	if ($Argument.Length -gt 0 -and $Argument -notmatch '[\s"]')
	{
		return $Argument
	}

	$builder = New-Object System.Text.StringBuilder
	[void]$builder.Append('"')
	$backslashCount = 0
	foreach ($character in $Argument.ToCharArray())
	{
		if ($character -eq [char]'\')
		{
			$backslashCount++
			continue
		}

		if ($character -eq [char]'"')
		{
			if ($backslashCount -gt 0)
			{
				[void]$builder.Append([char]'\', $backslashCount * 2)
			}
			[void]$builder.Append([char]'\')
			[void]$builder.Append([char]'"')
			$backslashCount = 0
			continue
		}

		if ($backslashCount -gt 0)
		{
			[void]$builder.Append([char]'\', $backslashCount)
			$backslashCount = 0
		}
		[void]$builder.Append($character)
	}

	if ($backslashCount -gt 0)
	{
		[void]$builder.Append([char]'\', $backslashCount * 2)
	}
	[void]$builder.Append('"')
	return $builder.ToString()
}

function Convert-CimCreationDateToUtc
{
	param(
		[Parameter(Mandatory = $true)]
		[object] $CreationDate
	)

	if ($CreationDate -is [datetime])
	{
		return ([datetime]$CreationDate).ToUniversalTime()
	}

	return [System.Management.ManagementDateTimeConverter]::ToDateTime(
		[string]$CreationDate).ToUniversalTime()
}

function Add-OwnedProcess
{
	param(
		[Parameter(Mandatory = $true)]
		[hashtable] $OwnedProcesses,

		[Parameter(Mandatory = $true)]
		[int] $ProcessId,

		[Parameter(Mandatory = $true)]
		[int] $ParentProcessId,

		[Parameter(Mandatory = $true)]
		[datetime] $StartedUtc
	)

	$key = [string]$ProcessId
	if (!$OwnedProcesses.ContainsKey($key))
	{
		$OwnedProcesses[$key] = [pscustomobject]@{
			ProcessId = $ProcessId
			ParentProcessId = $ParentProcessId
			StartedUtc = $StartedUtc.ToUniversalTime()
		}
	}
}

function Update-OwnedProcessTree
{
	param(
		[Parameter(Mandatory = $true)]
		[hashtable] $OwnedProcesses,

		[Parameter(Mandatory = $true)]
		[int] $RootProcessId
	)

	$pendingParents = New-Object System.Collections.ArrayList
	[void]$pendingParents.Add($RootProcessId)
	$visitedParents = @{}
	while ($pendingParents.Count -gt 0)
	{
		$parentId = [int]$pendingParents[0]
		$pendingParents.RemoveAt(0)
		$parentKey = [string]$parentId
		if ($visitedParents.ContainsKey($parentKey))
		{
			continue
		}
		$visitedParents[$parentKey] = $true

		$children = @(Get-CimInstance Win32_Process -Filter "ParentProcessId = $parentId")
		foreach ($child in $children)
		{
			$childId = [int]$child.ProcessId
			$startedUtc = Convert-CimCreationDateToUtc -CreationDate $child.CreationDate
			Add-OwnedProcess -OwnedProcesses $OwnedProcesses -ProcessId $childId -ParentProcessId $parentId -StartedUtc $startedUtc
			[void]$pendingParents.Add($childId)
		}
	}
}

function Get-MatchingOwnedProcess
{
	param(
		[Parameter(Mandatory = $true)]
		[object] $Record
	)

	$process = Get-Process -Id ([int]$Record.ProcessId) -ErrorAction SilentlyContinue
	if ($null -eq $process)
	{
		return $null
	}

	try
	{
		$startedUtc = $process.StartTime.ToUniversalTime()
	}
	catch
	{
		return $null
	}

	$deltaSeconds = [Math]::Abs(($startedUtc - ([datetime]$Record.StartedUtc)).TotalSeconds)
	if ($deltaSeconds -gt 2.0)
	{
		return $null
	}

	return $process
}

function Get-LiveOwnedProcessRecords
{
	param(
		[Parameter(Mandatory = $true)]
		[hashtable] $OwnedProcesses
	)

	$live = @()
	foreach ($record in $OwnedProcesses.Values)
	{
		if ($null -ne (Get-MatchingOwnedProcess -Record $record))
		{
			$live += $record
		}
	}
	return $live
}

function Get-OwnedProcessDepth
{
	param(
		[Parameter(Mandatory = $true)]
		[object] $Record,

		[Parameter(Mandatory = $true)]
		[hashtable] $OwnedProcesses
	)

	$depth = 0
	$current = $Record
	while ($null -ne $current -and $OwnedProcesses.ContainsKey([string]$current.ParentProcessId))
	{
		$depth++
		$current = $OwnedProcesses[[string]$current.ParentProcessId]
	}
	return $depth
}

function Stop-OwnedProcessTree
{
	param(
		[Parameter(Mandatory = $true)]
		[hashtable] $OwnedProcesses,

		[Parameter(Mandatory = $true)]
		[int] $RootProcessId
	)

	$deadline = [datetime]::UtcNow.AddSeconds(10)
	$emptySince = $null
	while ([datetime]::UtcNow -lt $deadline)
	{
		Update-OwnedProcessTree -OwnedProcesses $OwnedProcesses -RootProcessId $RootProcessId
		$live = @(Get-LiveOwnedProcessRecords -OwnedProcesses $OwnedProcesses)
		if ($live.Count -eq 0)
		{
			if ($null -eq $emptySince)
			{
				$emptySince = [datetime]::UtcNow
			}
			if (([datetime]::UtcNow - ([datetime]$emptySince)).TotalSeconds -ge 1.5)
			{
				return
			}
			Start-Sleep -Milliseconds 250
			continue
		}

		$emptySince = $null
		$ordered = @($live | Sort-Object -Property @{
			Expression = { Get-OwnedProcessDepth -Record $_ -OwnedProcesses $OwnedProcesses }
			Descending = $true
		})
		foreach ($record in $ordered)
		{
			$process = Get-MatchingOwnedProcess -Record $record
			if ($null -ne $process)
			{
				Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
			}
		}
		Start-Sleep -Milliseconds 250
	}

	$remaining = @(Get-LiveOwnedProcessRecords -OwnedProcesses $OwnedProcesses)
	if ($remaining.Count -gt 0)
	{
		throw "launched process tree did not close: $($remaining.ProcessId -join ', ')"
	}
}

function Start-HiddenProcess
{
	param(
		[Parameter(Mandatory = $true)]
		[string] $ExecutablePath,

		[Parameter(Mandatory = $true)]
		[string[]] $Arguments,

		[Parameter(Mandatory = $true)]
		[string] $WorkingDirectory
	)

	$quotedArguments = @()
	foreach ($argument in $Arguments)
	{
		$quotedArguments += ConvertTo-WindowsCommandLineArgument -Argument ([string]$argument)
	}

	$startInfo = New-Object System.Diagnostics.ProcessStartInfo
	$startInfo.FileName = $ExecutablePath
	$startInfo.Arguments = $quotedArguments -join " "
	$startInfo.WorkingDirectory = $WorkingDirectory
	$startInfo.UseShellExecute = $false
	$startInfo.CreateNoWindow = $true
	$startInfo.WindowStyle = [System.Diagnostics.ProcessWindowStyle]::Hidden
	$process = [System.Diagnostics.Process]::Start($startInfo)
	if ($null -eq $process)
	{
		throw "failed to launch the requested game diagnostic executable"
	}
	return $process
}

function Invoke-RestartStage
{
	param(
		[Parameter(Mandatory = $true)]
		[string] $ExecutablePath,

		[Parameter(Mandatory = $true)]
		[string] $AddonsDirectory,

		[Parameter(Mandatory = $true)]
		[string] $ResolvedProjectPath,

		[Parameter(Mandatory = $true)]
		[string] $WorldResourcePath,

		[Parameter(Mandatory = $true)]
		[string] $ProfileRoot,

		[Parameter(Mandatory = $true)]
		[string] $DebugRoot,

		[Parameter(Mandatory = $true)]
		[string] $RunId,

		[Parameter(Mandatory = $true)]
		[string] $CutName,

		[Parameter(Mandatory = $true)]
		[string] $Stage,

		[Parameter(Mandatory = $true)]
		[string] $WorkingDirectory
	)

	$resultPath = Join-Path $DebugRoot "HST_ExactQRFRestart_$RunId.$Stage.json"
	if (Test-Path -LiteralPath $resultPath)
	{
		throw "$CutName/$Stage result path was not fresh"
	}

	$arguments = @(
		"-addonsDir",
		$AddonsDirectory,
		$ProjectSwitch,
		$ResolvedProjectPath,
		$WorldSwitch,
		$WorldResourcePath,
		$ProfileSwitch,
		$ProfileRoot,
		"-window",
		"-noFocus",
		"-forceUpdate",
		"-rpl-timeout-disable",
		"-noThrow",
		"-hstExactQRFRestartStage",
		$Stage,
		"-hstExactQRFRestartRunId",
		$RunId,
		"-hstExactQRFRestartCut",
		$CutName
	)
	$arguments += $AdditionalArguments

	$ownedProcesses = @{}
	$process = $null
	try
	{
		$process = Start-HiddenProcess -ExecutablePath $ExecutablePath -Arguments $arguments -WorkingDirectory $WorkingDirectory
		$startedUtc = $process.StartTime.ToUniversalTime()
		Add-OwnedProcess -OwnedProcesses $ownedProcesses -ProcessId $process.Id -ParentProcessId 0 -StartedUtc $startedUtc
		Update-OwnedProcessTree -OwnedProcesses $ownedProcesses -RootProcessId $process.Id
		Write-Host ("START {0,-14} {1,-7} pid {2}" -f $CutName, $Stage, $process.Id)

		$deadline = [datetime]::UtcNow.AddSeconds($StageTimeoutSeconds)
		$exitObservedUtc = $null
		$result = $null
		$lastJsonReadError = ""
		$rootExitCode = $null
		while ([datetime]::UtcNow -lt $deadline)
		{
			Update-OwnedProcessTree -OwnedProcesses $ownedProcesses -RootProcessId $process.Id
			if ($null -eq $result -and (Test-Path -LiteralPath $resultPath -PathType Leaf))
			{
				$candidate = $null
				try
				{
					$candidate = Read-JsonFile -Path $resultPath
					$lastJsonReadError = ""
				}
				catch
				{
					$lastJsonReadError = $_.Exception.Message
				}

				if ($null -ne $candidate)
				{
					$result = Assert-StageResult -Result $candidate -RunId $RunId -CutName $CutName -Stage $Stage
					if (!$result.m_bSuccess)
					{
						throw "$CutName/$Stage reported failure: $($result.m_sEvidence)"
					}
				}
			}

			if ($process.HasExited)
			{
				if ($null -eq $exitObservedUtc)
				{
					$exitObservedUtc = [datetime]::UtcNow
					$rootExitCode = $process.ExitCode
				}
			}

			$live = @(Get-LiveOwnedProcessRecords -OwnedProcesses $ownedProcesses)
			if ($null -ne $result -and $live.Count -eq 0)
			{
				if ($null -eq $rootExitCode)
				{
					$process.Refresh()
					if (!$process.HasExited)
					{
						throw "$CutName/$Stage left its launched root process alive"
					}
					$rootExitCode = $process.ExitCode
				}
				if ([int]$rootExitCode -ne 0)
				{
					throw "$CutName/$Stage process exited with code $rootExitCode"
				}
				return [pscustomobject]@{
					Result = $result
					ExitCode = [int]$rootExitCode
					ResultPath = $resultPath
				}
			}

			if ($null -ne $exitObservedUtc -and $null -eq $result -and
				([datetime]::UtcNow - ([datetime]$exitObservedUtc)).TotalSeconds -ge $ResultGraceSeconds)
			{
				$detail = "matching result JSON was not written"
				if (![string]::IsNullOrWhiteSpace($lastJsonReadError))
				{
					$detail = "result JSON remained unreadable: $lastJsonReadError"
				}
				throw "$CutName/$Stage exited before success: $detail"
			}

			Start-Sleep -Milliseconds $PollMilliseconds
		}

		throw "$CutName/$Stage timed out after $StageTimeoutSeconds seconds"
	}
	catch
	{
		$stageFailure = $_.Exception.Message
		if ($null -ne $process)
		{
			try
			{
				Stop-OwnedProcessTree -OwnedProcesses $ownedProcesses -RootProcessId $process.Id
			}
			catch
			{
				throw "$stageFailure | cleanup failure: $($_.Exception.Message)"
			}
		}
		throw $stageFailure
	}
	finally
	{
		if ($null -ne $process)
		{
			$process.Dispose()
		}
	}
}

function Test-StrictDescendantPath
{
	param(
		[Parameter(Mandatory = $true)]
		[string] $Candidate,

		[Parameter(Mandatory = $true)]
		[string] $Parent
	)

	$fullCandidate = [System.IO.Path]::GetFullPath($Candidate).TrimEnd('\', '/')
	$fullParent = [System.IO.Path]::GetFullPath($Parent).TrimEnd('\', '/')
	$prefix = $fullParent + [System.IO.Path]::DirectorySeparatorChar
	return $fullCandidate.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)
}

function Remove-GuardedSessionRoot
{
	param(
		[Parameter(Mandatory = $true)]
		[string] $SessionRoot,

		[Parameter(Mandatory = $true)]
		[string] $OutputRoot,

		[Parameter(Mandatory = $true)]
		[string] $SessionNonce,

		[Parameter(Mandatory = $true)]
		[object[]] $Profiles,

		[Parameter(Mandatory = $true)]
		[bool] $RemoveEmptyOutputRoot
	)

	if (!(Test-StrictDescendantPath -Candidate $SessionRoot -Parent $OutputRoot))
	{
		throw "cleanup refused a session outside the configured output root"
	}
	$sessionGuardPath = Join-Path $SessionRoot ".partisan-exact-qrf-restart-session.json"
	if (!(Test-Path -LiteralPath $sessionGuardPath -PathType Leaf))
	{
		throw "cleanup refused a session without its nonce guard"
	}
	$sessionGuard = Read-JsonFile -Path $sessionGuardPath
	foreach ($propertyName in @("m_sMagic", "m_sNonce", "m_sSessionRoot"))
	{
		Assert-JsonProperty -Value $sessionGuard -PropertyName $propertyName -ArtifactLabel "launcher session guard"
	}
	if ([string]$sessionGuard.m_sMagic -ne $script:LauncherSessionMagic -or
		[string]$sessionGuard.m_sNonce -cne $SessionNonce -or
		[System.IO.Path]::GetFullPath([string]$sessionGuard.m_sSessionRoot) -cne [System.IO.Path]::GetFullPath($SessionRoot))
	{
		throw "cleanup refused a session whose nonce guard does not match"
	}

	foreach ($profile in $Profiles)
	{
		$profileRoot = [string]$profile.ProfileRoot
		if (!(Test-StrictDescendantPath -Candidate $profileRoot -Parent $SessionRoot))
		{
			throw "cleanup refused a profile outside the nonce-owned session"
		}
		$ownerPath = Join-Path $profileRoot ".partisan-exact-qrf-restart-owner.json"
		$engineGuardPath = Join-Path ([string]$profile.DebugRoot) "HST_ExactQRFRestart_$($profile.RunId).guard.json"
		if (!(Test-Path -LiteralPath $ownerPath -PathType Leaf) -or
			!(Test-Path -LiteralPath $engineGuardPath -PathType Leaf))
		{
			throw "cleanup refused $($profile.CutName) because an ownership guard is missing"
		}
		$owner = Read-JsonFile -Path $ownerPath
		$engineGuard = Read-JsonFile -Path $engineGuardPath
		if ([string]$owner.m_sMagic -ne $script:LauncherOwnerMagic -or
			[string]$owner.m_sNonce -cne $SessionNonce -or
			[string]$owner.m_sRunId -cne [string]$profile.RunId -or
			[string]$engineGuard.m_sMagic -ne $script:GuardMagic -or
			[string]$engineGuard.m_sRunId -cne [string]$profile.RunId)
		{
			throw "cleanup refused $($profile.CutName) because its ownership guard does not match"
		}
	}

	Remove-Item -LiteralPath $SessionRoot -Recurse -Force
	if ($RemoveEmptyOutputRoot -and
		(Test-Path -LiteralPath $OutputRoot -PathType Container) -and
		@(Get-ChildItem -LiteralPath $OutputRoot -Force).Count -eq 0)
	{
		Remove-Item -LiteralPath $OutputRoot -Force
	}
}

$matrix = @()
$profiles = @()
$sessionRoot = ""
$resolvedOutputRoot = ""
$outputRootCreated = $false
try
{
	$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
	$checkoutBuildIdentity = Read-CheckoutBuildIdentity -RepositoryRoot $repoRoot
	$resolvedExecutablePath = Resolve-AbsolutePath -Path $ExecutablePath
	if (!(Test-Path -LiteralPath $resolvedExecutablePath -PathType Leaf))
	{
		throw "game diagnostic executable does not exist: $resolvedExecutablePath"
	}
	if ([System.IO.Path]::GetFileName($resolvedExecutablePath) -ine "ArmaReforgerSteamDiag.exe")
	{
		throw "exact restart proof requires ArmaReforgerSteamDiag.exe"
	}
	$addonsDirectory = Join-Path ([System.IO.Path]::GetDirectoryName($resolvedExecutablePath)) "addons"
	if (!(Test-Path -LiteralPath $addonsDirectory -PathType Container))
	{
		throw "the executable-relative addons directory does not exist: $addonsDirectory"
	}

	if ([string]::IsNullOrWhiteSpace($ProjectPath))
	{
		$addonProject = Join-Path $repoRoot "addon.gproj"
		$genericProject = Join-Path $repoRoot "project.gproj"
		if (Test-Path -LiteralPath $addonProject -PathType Leaf)
		{
			$ProjectPath = $addonProject
		}
		elseif (Test-Path -LiteralPath $genericProject -PathType Leaf)
		{
			$ProjectPath = $genericProject
		}
		else
		{
			throw "repository project file was not found"
		}
	}
	$resolvedProjectPath = Resolve-AbsolutePath -Path $ProjectPath -BasePath $repoRoot
	if (!(Test-Path -LiteralPath $resolvedProjectPath -PathType Leaf))
	{
		throw "project file does not exist: $resolvedProjectPath"
	}
	$checkoutProjectPath = [System.IO.Path]::GetFullPath((Join-Path $repoRoot "addon.gproj"))
	if ($resolvedProjectPath -cne $checkoutProjectPath)
	{
		throw "exact restart proof requires this checkout's addon.gproj"
	}

	$worldResourcePath = "Worlds/HST_Dev/HST_Dev.ent"
	$worldFilePath = Join-Path $repoRoot ($worldResourcePath -replace '/', [System.IO.Path]::DirectorySeparatorChar)
	if (!(Test-Path -LiteralPath $worldFilePath -PathType Leaf))
	{
		throw "exact HST_Dev world was not found in the repository"
	}
	Assert-LaunchArgumentIsolation -ExtraArguments $AdditionalArguments -ConfiguredSwitches @(
		$ProjectSwitch,
		$WorldSwitch,
		$ProfileSwitch
	)

	if ([string]::IsNullOrWhiteSpace($ProfileOutputRoot))
	{
		$ProfileOutputRoot = Join-Path ([System.IO.Path]::GetTempPath()) "PartisanExactQRFRestartProof"
	}
	$resolvedOutputRoot = Resolve-AbsolutePath -Path $ProfileOutputRoot -BasePath $repoRoot
	$outputRootCreated = !(Test-Path -LiteralPath $resolvedOutputRoot)
	[void](New-Item -ItemType Directory -Path $resolvedOutputRoot -Force)

	$normalizedCuts = @()
	foreach ($cutName in $Cut)
	{
		$normalized = $cutName.Trim().ToLowerInvariant()
		if ($normalizedCuts -notcontains $normalized)
		{
			$normalizedCuts += $normalized
		}
	}
	if ($normalizedCuts.Count -eq 0)
	{
		throw "at least one restart cut is required"
	}

	$sessionNonce = [guid]::NewGuid().ToString("N")
	$sessionRoot = Join-Path $resolvedOutputRoot "exact-qrf-restart-$sessionNonce"
	if (Test-Path -LiteralPath $sessionRoot)
	{
		throw "nonce-owned output session already exists"
	}
	[void](New-Item -ItemType Directory -Path $sessionRoot)
	$sessionGuard = [ordered]@{
		m_sMagic = $script:LauncherSessionMagic
		m_sNonce = $sessionNonce
		m_sSessionRoot = [System.IO.Path]::GetFullPath($sessionRoot)
	}
	Write-JsonUtf8NoBom -Path (Join-Path $sessionRoot ".partisan-exact-qrf-restart-session.json") -Value $sessionGuard

	$proofBuildIdentity = ""
	foreach ($cutName in $normalizedCuts)
	{
		$runId = "qrf_{0}_{1}_{2}" -f ([datetime]::UtcNow.ToString("yyyyMMddHHmmss")), $sessionNonce.Substring(0, 12), $cutName
		$profileRoot = Join-Path $sessionRoot $cutName
		# The engine maps $profile beneath the launch profile root's profile
		# directory; proof artifacts must be armed at that exact mounted path.
		$debugRoot = Join-Path $profileRoot "profile\Partisan\debug"
		[void](New-Item -ItemType Directory -Path $debugRoot -Force)

		$owner = [ordered]@{
			m_sMagic = $script:LauncherOwnerMagic
			m_sNonce = $sessionNonce
			m_sRunId = $runId
			m_sRequestedCut = $cutName
			m_sProfileRoot = [System.IO.Path]::GetFullPath($profileRoot)
		}
		Write-JsonUtf8NoBom -Path (Join-Path $profileRoot ".partisan-exact-qrf-restart-owner.json") -Value $owner

		$guard = [ordered]@{
			m_sMagic = $script:GuardMagic
			m_sRunId = $runId
			m_sRequestedCut = $cutName
			m_bAllowCanonicalCampaignOverwrite = $true
		}
		$guardPath = Join-Path $debugRoot "HST_ExactQRFRestart_$runId.guard.json"
		Write-JsonUtf8NoBom -Path $guardPath -Value $guard

		$profileRecord = [pscustomobject]@{
			CutName = $cutName
			RunId = $runId
			ProfileRoot = $profileRoot
			DebugRoot = $debugRoot
		}
		$profiles += $profileRecord

		$runBuildSha = ""
		foreach ($stage in @("prepare", "recover", "replay"))
		{
			$stageOutcome = Invoke-RestartStage -ExecutablePath $resolvedExecutablePath -AddonsDirectory $addonsDirectory -ResolvedProjectPath $resolvedProjectPath -WorldResourcePath $worldResourcePath -ProfileRoot $profileRoot -DebugRoot $debugRoot -RunId $runId -CutName $cutName -Stage $stage -WorkingDirectory $repoRoot
			$result = $stageOutcome.Result
			if ([string]$result.m_sBuildSha -cne [string]$checkoutBuildIdentity.BuildSha -or
				[string]$result.m_sBuildUtc -cne [string]$checkoutBuildIdentity.BuildUtc -or
				[string]$result.m_sBuildLabel -cne [string]$checkoutBuildIdentity.BuildLabel -or
				[int]$result.m_iCampaignSchemaVersion -ne [int]$checkoutBuildIdentity.CampaignSchemaVersion)
			{
				throw "$cutName/$stage runtime build identity does not match this checkout"
			}
			$stageBuildIdentity = "{0}|{1}|{2}|{3}" -f $result.m_sBuildSha, $result.m_sBuildUtc, $result.m_sBuildLabel, $result.m_iCampaignSchemaVersion
			if ([string]::IsNullOrWhiteSpace($proofBuildIdentity))
			{
				$proofBuildIdentity = $stageBuildIdentity
			}
			elseif ($proofBuildIdentity -cne $stageBuildIdentity)
			{
				throw "$cutName/$stage changed build identity during the proof matrix"
			}
			if ([string]::IsNullOrWhiteSpace($runBuildSha))
			{
				$runBuildSha = [string]$result.m_sBuildSha
			}
			elseif ($runBuildSha -cne [string]$result.m_sBuildSha)
			{
				throw "$cutName changed build identity between process stages"
			}

			$fingerprint = [string]$result.m_sFingerprint
			if ($fingerprint.Length -gt 18)
			{
				$fingerprint = $fingerprint.Substring(0, 18)
			}
			$matrix += [pscustomobject]@{
				Cut = $cutName
				Stage = $stage
				Success = [bool]$result.m_bSuccess
				Exit = [int]$stageOutcome.ExitCode
				Build = ([string]$result.m_sBuildSha).Substring(0, [Math]::Min(12, ([string]$result.m_sBuildSha).Length))
				Schema = [int]$result.m_iCampaignSchemaVersion
				Fingerprint = $fingerprint
			}

			if ($stage -eq "prepare")
			{
				$carrierPath = Join-Path $debugRoot "HST_ExactQRFRestart_$runId.carrier.json"
				if (!(Test-Path -LiteralPath $carrierPath -PathType Leaf))
				{
					throw "$cutName prepare succeeded without its durable carrier"
				}
				$carrier = Read-JsonFile -Path $carrierPath
				[void](Assert-PreparedCarrier -Carrier $carrier -RunId $runId -CutName $cutName -PrepareResult $result)
			}
		}
	}

	Write-Host ""
	$matrix | Format-Table -AutoSize
	Write-Host "Artifacts: $sessionRoot"
	if ($Cleanup)
	{
		Remove-GuardedSessionRoot -SessionRoot $sessionRoot -OutputRoot $resolvedOutputRoot -SessionNonce $sessionNonce -Profiles $profiles -RemoveEmptyOutputRoot $outputRootCreated
		Write-Host "Guarded cleanup complete."
	}
	exit 0
}
catch
{
	$failureMessage = $_.Exception.Message
	if ($matrix.Count -gt 0)
	{
		Write-Host ""
		$matrix | Format-Table -AutoSize
	}
	if ($Cleanup -and
		![string]::IsNullOrWhiteSpace($sessionRoot) -and
		(Test-Path -LiteralPath $sessionRoot))
	{
		try
		{
			Remove-GuardedSessionRoot -SessionRoot $sessionRoot -OutputRoot $resolvedOutputRoot -SessionNonce $sessionNonce -Profiles $profiles -RemoveEmptyOutputRoot $outputRootCreated
			Write-Host "Guarded cleanup complete after failure."
		}
		catch
		{
			$failureMessage = "$failureMessage | guarded cleanup failed: $($_.Exception.Message)"
		}
	}
	elseif (![string]::IsNullOrWhiteSpace($sessionRoot) -and (Test-Path -LiteralPath $sessionRoot))
	{
		Write-Host "Artifacts retained: $sessionRoot"
	}
	Write-Error $failureMessage
	exit 1
}
