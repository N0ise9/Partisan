[CmdletBinding()]
param(
    [switch]$ProducerChild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Invoke-CorrectedCanaryProducerChild {
    param([Parameter(Mandatory = $true)][string]$EncodedPayload)

    $payloadEnvironmentName =
        'PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_CHILD_PAYLOAD'
    $resultPath = ''
    $resultTemporaryPath = ''
    $resultPathValidated = $false
    $childExitCode = 2
    $environmentNames = New-Object Collections.Generic.List[string]
    $result = [ordered]@{
        SchemaVersion = 1
        Mode = ''
        Token = ''
        InvocationCompleted = $false
        Succeeded = $false
        Error = ''
    }
    try {
        [Environment]::SetEnvironmentVariable(
            $payloadEnvironmentName,
            $null,
            [EnvironmentVariableTarget]::Process)
        $payloadJson = [Text.Encoding]::UTF8.GetString(
            [Convert]::FromBase64String($EncodedPayload))
        $payloadObject = $payloadJson | ConvertFrom-Json
        $expectedPayloadProperties = @(
            'SchemaVersion', 'Mode', 'Token', 'SelfTestPath',
            'ProducerPath', 'RunPath', 'ResultPath')
        $actualPayloadProperties = @($payloadObject.PSObject.Properties.Name)
        if ($actualPayloadProperties.Count -ne $expectedPayloadProperties.Count) {
            throw 'The corrected-canary producer child payload schema is invalid.'
        }
        foreach ($propertyName in $expectedPayloadProperties) {
            if ($actualPayloadProperties -cnotcontains $propertyName) {
                throw 'The corrected-canary producer child payload schema is invalid.'
            }
        }

        $mode = [string]$payloadObject.Mode
        $token = [string]$payloadObject.Token
        $selfTestChildPath = [IO.Path]::GetFullPath(
            [string]$payloadObject.SelfTestPath)
        $toolsDirectoryPath = [IO.Path]::GetDirectoryName($selfTestChildPath)
        $producerChildPath = [IO.Path]::GetFullPath(
            [string]$payloadObject.ProducerPath)
        $runChildPath = [IO.Path]::GetFullPath(
            [string]$payloadObject.RunPath)
        $resultPath = [IO.Path]::GetFullPath(
            [string]$payloadObject.ResultPath)
        $resultDirectoryPath = [IO.Path]::GetDirectoryName($resultPath)
        $resultTemporaryPath = Join-Path $resultDirectoryPath 'result.json.tmp'
        $result.Mode = $mode
        $result.Token = $token

        $tempPrefix = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd(
            '\', '/') + [IO.Path]::DirectorySeparatorChar
        $runRelative = if ($runChildPath.StartsWith(
                $tempPrefix,
                [StringComparison]::OrdinalIgnoreCase)) {
            $runChildPath.Substring($tempPrefix.Length)
        }
        else {
            ''
        }
        $resultRelative = if ($resultPath.StartsWith(
                $tempPrefix,
                [StringComparison]::OrdinalIgnoreCase)) {
            $resultPath.Substring($tempPrefix.Length)
        }
        else {
            ''
        }
        $runSegments = @($runRelative -split '[\\/]' | Where-Object { $_ })
        $resultSegments = @($resultRelative -split '[\\/]' | Where-Object { $_ })
        $expectedResultDirectory = "producer-child-$mode-$token"
        $expectedFixtureDirectory = switch -CaseSensitive ($mode) {
            'late-drift' { 'late-drift' }
            'publication-window' { 'publication-window' }
            'concurrent-reuse' { 'concurrent-byte-identical-reuse' }
            default { '' }
        }
        $tempParentPath = if ($runSegments.Count -gt 0) {
            [IO.Path]::GetFullPath((Join-Path $tempPrefix $runSegments[0]))
        }
        else {
            ''
        }
        $fixtureDirectoryPath = [IO.Path]::GetDirectoryName(
            [IO.Path]::GetDirectoryName($runChildPath))
        $bundleDirectoryPath = [IO.Path]::GetDirectoryName($runChildPath)
        if ($payloadObject.SchemaVersion -isnot [int] -or
            [int]$payloadObject.SchemaVersion -ne 1 -or
            $payloadObject.Mode -isnot [string] -or
            $payloadObject.Token -isnot [string] -or
            $mode -cnotin @('late-drift', 'publication-window', 'concurrent-reuse') -or
            $token -cnotmatch '^[0-9a-f]{32}$' -or
            $runSegments.Count -ne 4 -or
            $resultSegments.Count -ne 3 -or
            $runSegments[0] -cne $resultSegments[0] -or
            $runSegments[0] -cnotmatch
                '^PartisanCorrectedCanaryV2-[0-9a-f]{32}$' -or
            $runSegments[1] -cne $expectedFixtureDirectory -or
            $runSegments[2] -cnotmatch
                '^\d{8}T\d{6}Z-[0-9a-f]{32}$' -or
            $runSegments[3] -cne 'run.json' -or
            $resultSegments[1] -cne $expectedResultDirectory -or
            $resultSegments[2] -cne 'result.json' -or
            -not $fixtureDirectoryPath.Equals(
                [IO.Path]::GetFullPath((Join-Path `
                        $tempParentPath `
                        $expectedFixtureDirectory)),
                [StringComparison]::OrdinalIgnoreCase) -or
            -not $bundleDirectoryPath.Equals(
                [IO.Path]::GetFullPath((Join-Path `
                        $fixtureDirectoryPath `
                        $runSegments[2])),
                [StringComparison]::OrdinalIgnoreCase) -or
            -not $resultDirectoryPath.Equals(
                [IO.Path]::GetFullPath((Join-Path `
                        $tempParentPath `
                        $expectedResultDirectory)),
                [StringComparison]::OrdinalIgnoreCase) -or
            -not $selfTestChildPath.Equals(
                [IO.Path]::GetFullPath($PSCommandPath),
                [StringComparison]::OrdinalIgnoreCase) -or
            -not [IO.Path]::GetDirectoryName($producerChildPath).Equals(
                [IO.Path]::GetFullPath($PSScriptRoot),
                [StringComparison]::OrdinalIgnoreCase) -or
            (Split-Path -Leaf $producerChildPath) -cne
                'New-PartisanCampaignDebugReleaseIndex.ps1' -or
            (Split-Path -Leaf $runChildPath) -cne 'run.json' -or
            -not (Test-Path -LiteralPath $producerChildPath -PathType Leaf) -or
            -not (Test-Path -LiteralPath $runChildPath -PathType Leaf) -or
            -not (Test-Path -LiteralPath $resultDirectoryPath -PathType Container) -or
            (Test-Path -LiteralPath $resultPath) -or
            (Test-Path -LiteralPath $resultTemporaryPath)) {
            throw 'The corrected-canary producer child payload escaped its bounded fixture.'
        }
        foreach ($pathToInspect in @(
                $selfTestChildPath,
                $producerChildPath,
                $toolsDirectoryPath,
                $tempParentPath,
                $fixtureDirectoryPath,
                $bundleDirectoryPath,
                $runChildPath,
                $resultDirectoryPath)) {
            $pathItem = Get-Item -LiteralPath $pathToInspect -Force -ErrorAction Stop
            if (($pathItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
                (($pathToInspect -cne $selfTestChildPath -and
                        $pathToInspect -cne $producerChildPath -and
                        $pathToInspect -cne $runChildPath) -and
                    -not $pathItem.PSIsContainer)) {
                throw 'The corrected-canary producer child payload traversed a reparse point.'
            }
        }
        $resultPathValidated = $true

        $environmentNamesForMode = switch -CaseSensitive ($mode) {
            'late-drift' {
                @('PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_LATE_DRIFT_TOKEN')
            }
            'publication-window' {
                @('PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_PUBLICATION_TOKEN')
            }
            'concurrent-reuse' {
                @(
                    'PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_CONCURRENT_TOKEN',
                    'PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_PUBLICATION_TOKEN')
            }
        }
        foreach ($environmentName in @($environmentNamesForMode)) {
            [Environment]::SetEnvironmentVariable(
                $environmentName,
                $token,
                [EnvironmentVariableTarget]::Process)
            [void]$environmentNames.Add($environmentName)
        }

        try {
            [void](& $producerChildPath -RunEnvelopePath $runChildPath)
            $result.Succeeded = $true
        }
        catch {
            $result.Error = $_.Exception.Message
        }
        finally {
            $result.InvocationCompleted = $true
        }
        $childExitCode = 0
    }
    catch {
        $result.Error = $_.Exception.Message
    }
    finally {
        foreach ($environmentName in $environmentNames) {
            [Environment]::SetEnvironmentVariable(
                $environmentName,
                $null,
                [EnvironmentVariableTarget]::Process)
        }
        if ($resultPathValidated) {
            $resultStream = $null
            $resultWriter = $null
            try {
                $resultStream = New-Object IO.FileStream(
                    $resultTemporaryPath,
                    [IO.FileMode]::CreateNew,
                    [IO.FileAccess]::Write,
                    [IO.FileShare]::None)
                $resultWriter = New-Object IO.StreamWriter(
                    $resultStream,
                    (New-Object Text.UTF8Encoding($false)))
                $resultWriter.Write(($result | ConvertTo-Json -Compress) + "`n")
                $resultWriter.Flush()
                $resultStream.Flush($true)
                $resultWriter.Dispose()
                $resultWriter = $null
                $resultStream.Dispose()
                $resultStream = $null
                [IO.File]::Move($resultTemporaryPath, $resultPath)
            }
            catch {
                $childExitCode = 3
                [Console]::Error.WriteLine(
                    'The corrected-canary producer child could not publish its result receipt: {0}',
                    $_.Exception.Message)
            }
            finally {
                if ($resultWriter) {
                    $resultWriter.Dispose()
                }
                if ($resultStream) {
                    $resultStream.Dispose()
                }
            }
        }
        elseif ([string]::IsNullOrEmpty($result.Error)) {
            [Console]::Error.WriteLine(
                'The corrected-canary producer child could not validate its result path.')
        }
    }
    return $childExitCode
}

if ($ProducerChild) {
    $childPayloadEnvironmentName =
        'PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_CHILD_PAYLOAD'
    $encodedChildPayload = [string][Environment]::GetEnvironmentVariable(
        $childPayloadEnvironmentName,
        [EnvironmentVariableTarget]::Process)
    if ([string]::IsNullOrEmpty($encodedChildPayload)) {
        throw 'The corrected-canary producer child payload is required.'
    }
    exit (Invoke-CorrectedCanaryProducerChild `
        -EncodedPayload $encodedChildPayload)
}

$producerPath = Join-Path $PSScriptRoot 'New-PartisanCampaignDebugReleaseIndex.ps1'
$runnerPath = Join-Path $PSScriptRoot 'run-guarded-campaign-debug.ps1'
$candidateModulePath = Join-Path $PSScriptRoot 'Partisan.ReleaseCandidate.psm1'
$consumerPath = Join-Path $PSScriptRoot 'update-release-docs.ps1'
$checkoutRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$producerSha = (Get-FileHash -LiteralPath $producerPath -Algorithm SHA256).Hash.ToLowerInvariant()
$runnerSha = (Get-FileHash -LiteralPath $runnerPath -Algorithm SHA256).Hash.ToLowerInvariant()
$candidateModuleSha = (Get-FileHash `
    -LiteralPath $candidateModulePath -Algorithm SHA256).Hash.ToLowerInvariant()
$consumerSha = (Get-FileHash -LiteralPath $consumerPath -Algorithm SHA256).Hash.ToLowerInvariant()

function Get-ImmutableGitBlobSha256 {
    param(
        [Parameter(Mandatory = $true)][string]$Revision,
        [Parameter(Mandatory = $true)][string]$FilePath
    )

    $checkout = $checkoutRoot.TrimEnd('\', '/')
    $prefix = $checkout + [IO.Path]::DirectorySeparatorChar
    $fullPath = [IO.Path]::GetFullPath($FilePath)
    if (-not $fullPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'A corrected-canary harness fixture tool escaped the checkout.'
    }
    $relativePath = $fullPath.Substring($prefix.Length).Replace('\', '/')
    $startInfo = New-Object Diagnostics.ProcessStartInfo
    $startInfo.FileName = 'git'
    $escapedRoot = $checkout.Replace('"', '\"')
    $escapedSpec = ("{0}:{1}" -f $Revision, $relativePath).Replace('"', '\"')
    $startInfo.Arguments = "-C `"$escapedRoot`" cat-file blob `"$escapedSpec`""
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $startInfo
    try {
        if (-not $process.Start()) {
            throw 'The corrected-canary harness fixture blob reader could not start.'
        }
        $memory = New-Object IO.MemoryStream
        try {
            $process.StandardOutput.BaseStream.CopyTo($memory)
            $errorText = $process.StandardError.ReadToEnd()
            $process.WaitForExit()
            if ($process.ExitCode -ne 0) {
                throw "The corrected-canary fixture blob $relativePath could not be read: $errorText"
            }
            $sha = [Security.Cryptography.SHA256]::Create()
            try {
                return ([BitConverter]::ToString(
                    $sha.ComputeHash($memory.ToArray()))).Replace('-', '').ToLowerInvariant()
            }
            finally {
                $sha.Dispose()
            }
        }
        finally {
            $memory.Dispose()
        }
    }
    finally {
        $process.Dispose()
    }
}

function Get-CorrectedCanaryTextSha256 {
    param([Parameter(Mandatory = $true)][string]$Text)

    $encoding = [Text.UTF8Encoding]::new($false)
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString(
                $sha.ComputeHash($encoding.GetBytes($Text)))).
            Replace('-', '').ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
}

$candidateId = 'partisan-rc-0123456789ab-20260719T120000Z'
$candidateHead = '0' * 40
$candidateVersion = '0.1.0-rc.corrected-canary-selftest'
$embeddedBuildSha = '1' * 40
$embeddedBuildUtc = '2026-07-19T11:50:00Z'
$embeddedBuildLabel = 'schema71-settings24-corrected-canary-v2-selftest'
$campaignSchema = 71
$runtimeSettingsSchema = 24
$addonId = 'histasi'
$addonGuid = '698532771130111D'
$packageHashAlgorithm = 'sha256-manifest-v1'
$packageFiles = @(
    [pscustomobject][ordered]@{
        path = 'package/Partisan/addon.gproj'
        indexPath = 'Partisan/addon.gproj'
        length = 499
        sha256 = '3' * 64
    },
    [pscustomobject][ordered]@{
        path = 'package/Partisan/data.pak'
        indexPath = 'Partisan/data.pak'
        length = 4096
        sha256 = '4' * 64
    },
    [pscustomobject][ordered]@{
        path = 'package/Partisan/resourceDatabase.rdb'
        indexPath = 'Partisan/resourceDatabase.rdb'
        length = 2048
        sha256 = '5' * 64
    },
    [pscustomobject][ordered]@{
        path = 'package/Partisan/thumbnail.png'
        indexPath = 'Partisan/thumbnail.png'
        length = 1024
        sha256 = '6' * 64
    })
$packageCanonicalRows = @($packageFiles | Sort-Object indexPath | ForEach-Object {
        "{0}`t{1}`t{2}" -f $_.sha256, ([long]$_.length), $_.indexPath
    })
$packageSha = Get-CorrectedCanaryTextSha256 `
    (($packageCanonicalRows -join "`n") + "`n")
$workbenchCrc = '0123abcd'
$headRows = @(& git -C $checkoutRoot rev-parse HEAD 2>$null)
$headExitCode = $LASTEXITCODE
$harnessHead = ($headRows -join '').Trim()
if ($headExitCode -ne 0 -or $harnessHead -cnotmatch '^[0-9a-f]{40}$') {
    throw 'The corrected-canary self-test could not resolve the harness Git HEAD.'
}
$runnerGitBlobSha = Get-ImmutableGitBlobSha256 $harnessHead $runnerPath
$candidateModuleGitBlobSha = Get-ImmutableGitBlobSha256 $harnessHead $candidateModulePath
$producerGitBlobSha = Get-ImmutableGitBlobSha256 $harnessHead $producerPath
$consumerGitBlobSha = Get-ImmutableGitBlobSha256 $harnessHead $consumerPath
$runId = 'seed1985_t0_p1_u1784500000'

$clientDiagnosticIdentity = [pscustomobject][ordered]@{
    fileName = 'runtime-client-diagnostic.exe'
    fileVersion = '1.0.0.0'
    productVersion = '1.0.0.0'
    length = 4096
    sha256 = '4' * 64
}
$clientRuntimeIdentity = [pscustomobject][ordered]@{
    fileName = 'runtime-client.exe'
    fileVersion = '1.0.0.0'
    productVersion = '1.0.0.0'
    length = 8192
    sha256 = '5' * 64
}

function Write-Utf8Text {
    param([string]$Path, [string]$Text)

    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    [IO.File]::WriteAllText($Path, $Text, (New-Object Text.UTF8Encoding($false)))
}

function Write-Json {
    param([string]$Path, $Value)

    Write-Utf8Text `
        -Path $Path `
        -Text (($Value | ConvertTo-Json -Depth 32).Replace("`r`n", "`n") + "`n")
}

function Get-CorrectedCanaryProcessStreamText {
    param(
        [Parameter(Mandatory = $true)]$Task,
        [Parameter(Mandatory = $true)][string]$Label
    )

    try {
        if (-not $Task.Wait(5000)) {
            return "$Label stream did not close within five seconds."
        }
        $text = [string]$Task.Result
        if ($text.Length -gt 4096) {
            return $text.Substring(0, 4096) + '...<truncated>'
        }
        return $text
    }
    catch {
        return "$Label stream read failed: $($_.Exception.Message)"
    }
}

function Start-CorrectedCanaryProducerProcess {
    param(
        [Parameter(Mandatory = $true)][string]$ProducerPath,
        [Parameter(Mandatory = $true)][string]$RunPath,
        [Parameter(Mandatory = $true)][string]$TempParent,
        [Parameter(Mandatory = $true)][string]$Token,
        [Parameter(Mandatory = $true)]
        [ValidateSet('late-drift', 'publication-window', 'concurrent-reuse')]
        [string]$Mode
    )

    if ($Token -cnotmatch '^[0-9a-f]{32}$') {
        throw 'The corrected-canary producer child token is invalid.'
    }
    $tempParentPath = [IO.Path]::GetFullPath($TempParent).TrimEnd('\', '/')
    $tempPrefix = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd(
        '\', '/') + [IO.Path]::DirectorySeparatorChar
    $tempParentItem = Get-Item `
        -LiteralPath $tempParentPath `
        -Force `
        -ErrorAction Stop
    if (-not $tempParentPath.StartsWith(
            $tempPrefix,
            [StringComparison]::OrdinalIgnoreCase) -or
        (Split-Path -Leaf $tempParentPath) -cnotmatch
            '^PartisanCorrectedCanaryV2-[0-9a-f]{32}$' -or
        -not $tempParentItem.PSIsContainer -or
        ($tempParentItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'The corrected-canary producer child root is outside its bounded temporary parent.'
    }

    $childLeaf = "producer-child-$Mode-$Token"
    $childRoot = [IO.Path]::GetFullPath((Join-Path $tempParentPath $childLeaf))
    if (-not [IO.Path]::GetDirectoryName($childRoot).Equals(
            $tempParentPath,
            [StringComparison]::OrdinalIgnoreCase) -or
        (Test-Path -LiteralPath $childRoot)) {
        throw 'The corrected-canary producer child root already exists or is not directly contained.'
    }
    New-Item -ItemType Directory -Path $childRoot -ErrorAction Stop | Out-Null
    $childRootItem = Get-Item -LiteralPath $childRoot -Force -ErrorAction Stop
    if (-not $childRootItem.PSIsContainer -or
        ($childRootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'The corrected-canary producer child root is not a regular directory.'
    }

    $receiptPath = Join-Path $childRoot 'result.json'
    $selfTestPath = [IO.Path]::GetFullPath($PSCommandPath)
    $selfTestItem = Get-Item -LiteralPath $selfTestPath -Force -ErrorAction Stop
    if ($selfTestItem.PSIsContainer -or
        ($selfTestItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
        (Split-Path -Leaf $selfTestPath) -cne
            'test-partisan-campaign-debug-corrected-canary-release-index.ps1') {
        throw 'The corrected-canary producer child could not bind its self-test script.'
    }
    $payload = [ordered]@{
        SchemaVersion = 1
        Mode = $Mode
        Token = $Token
        SelfTestPath = $selfTestPath
        ProducerPath = [IO.Path]::GetFullPath($ProducerPath)
        RunPath = [IO.Path]::GetFullPath($RunPath)
        ResultPath = [IO.Path]::GetFullPath($receiptPath)
    }
    $payloadJson = $payload | ConvertTo-Json -Compress
    $encodedPayload = [Convert]::ToBase64String(
        [Text.Encoding]::UTF8.GetBytes($payloadJson))
    $payloadEnvironmentName =
        'PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_CHILD_PAYLOAD'
    $powerShellPath = [IO.Path]::GetFullPath((Join-Path $PSHOME 'powershell.exe'))
    $powerShellItem = Get-Item `
        -LiteralPath $powerShellPath `
        -Force `
        -ErrorAction Stop
    if ($powerShellItem.PSIsContainer -or
        ($powerShellItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'The corrected-canary producer child could not resolve Windows PowerShell.'
    }

    $startInfo = New-Object Diagnostics.ProcessStartInfo
    $startInfo.FileName = $powerShellPath
    $escapedSelfTestPath = $selfTestPath
    $startInfo.Arguments =
        "-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File `"$escapedSelfTestPath`" -ProducerChild"
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($environmentName in @(
            $payloadEnvironmentName,
            'PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_LATE_DRIFT_TOKEN',
            'PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_PUBLICATION_TOKEN',
            'PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_CONCURRENT_TOKEN')) {
        $startInfo.EnvironmentVariables.Remove($environmentName)
    }
    $startInfo.EnvironmentVariables[$payloadEnvironmentName] = $encodedPayload

    $process = New-Object Diagnostics.Process
    $process.StartInfo = $startInfo
    $processStarted = $false
    try {
        if (-not $process.Start()) {
            throw 'The corrected-canary producer child process could not start.'
        }
        $processStarted = $true
        $standardOutputTask = $process.StandardOutput.ReadToEndAsync()
        $standardErrorTask = $process.StandardError.ReadToEndAsync()
        return [pscustomobject]@{
            Mode = $Mode
            Token = $Token
            TempParent = $tempParentPath
            Root = $childRoot
            ReceiptPath = $receiptPath
            Process = $process
            StandardOutputTask = $standardOutputTask
            StandardErrorTask = $standardErrorTask
            Outcome = $null
        }
    }
    catch {
        $setupError = $_.Exception.Message
        $cleanupFailure = ''
        if ($processStarted) {
            try {
                if (-not $process.HasExited) {
                    $process.Kill()
                }
                if (-not $process.WaitForExit(5000)) {
                    $cleanupFailure =
                        'the started child survived its five-second cleanup bound'
                }
            }
            catch {
                $cleanupFailure = $_.Exception.Message
            }
        }
        $process.Dispose()
        if (Test-Path -LiteralPath $childRoot -PathType Container) {
            try {
                $failedRootItem = Get-Item `
                    -LiteralPath $childRoot `
                    -Force `
                    -ErrorAction Stop
                $failedRootChildren = @(
                    Get-ChildItem -LiteralPath $childRoot -Force)
                if (($failedRootItem.Attributes -band
                        [IO.FileAttributes]::ReparsePoint) -eq 0 -and
                    @($failedRootChildren | Where-Object {
                            $_.PSIsContainer -or
                            ($_.Attributes -band
                                [IO.FileAttributes]::ReparsePoint) -ne 0
                        }).Count -eq 0) {
                    Remove-Item `
                        -LiteralPath $childRoot `
                        -Recurse `
                        -Force `
                        -ErrorAction Stop
                }
            }
            catch {
                # Preserve the transport setup failure. Unsafe or contended
                # entries remain inside the already-bounded outer fixture.
            }
        }
        if (-not [string]::IsNullOrWhiteSpace($cleanupFailure)) {
            throw ("Corrected-canary producer child setup failed: {0}; " +
                "child cleanup also failed: {1}" -f `
                    $setupError, $cleanupFailure)
        }
        throw
    }
}

function Complete-CorrectedCanaryProducerProcess {
    param(
        [Parameter(Mandatory = $true)]$Child,
        [Parameter(Mandatory = $true)][string]$Label,
        [int]$TimeoutSeconds = 60
    )

    if ($null -ne $Child.Outcome) {
        return $Child.Outcome
    }
    if (-not $Child.Process.WaitForExit($TimeoutSeconds * 1000)) {
        throw "$Label did not exit within $TimeoutSeconds seconds."
    }
    $Child.Process.WaitForExit()
    $standardOutput = Get-CorrectedCanaryProcessStreamText `
        -Task $Child.StandardOutputTask `
        -Label "$Label standard output"
    $standardError = Get-CorrectedCanaryProcessStreamText `
        -Task $Child.StandardErrorTask `
        -Label "$Label standard error"
    $exitCode = [int]$Child.Process.ExitCode

    $expectedRoot = [IO.Path]::GetFullPath((Join-Path `
            $Child.TempParent `
            "producer-child-$($Child.Mode)-$($Child.Token)"))
    $expectedReceiptPath = [IO.Path]::GetFullPath((Join-Path `
            $expectedRoot `
            'result.json'))
    if (-not [IO.Path]::GetFullPath([string]$Child.Root).Equals(
            $expectedRoot,
            [StringComparison]::OrdinalIgnoreCase) -or
        -not [IO.Path]::GetFullPath([string]$Child.ReceiptPath).Equals(
            $expectedReceiptPath,
            [StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-Path -LiteralPath $expectedReceiptPath -PathType Leaf)) {
        throw ("$Label did not publish its exact confined result receipt. " +
            "ExitCode=$exitCode; StandardError=$standardError; " +
            "StandardOutput=$standardOutput")
    }
    foreach ($pathToInspect in @($expectedRoot, $expectedReceiptPath)) {
        $pathItem = Get-Item -LiteralPath $pathToInspect -Force -ErrorAction Stop
        if (($pathItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "$Label result receipt traversed a reparse point."
        }
    }

    $receiptText = Get-Content -Raw -LiteralPath $expectedReceiptPath
    $receipt = $receiptText | ConvertFrom-Json
    $expectedReceiptProperties = @(
        'SchemaVersion', 'Mode', 'Token',
        'InvocationCompleted', 'Succeeded', 'Error')
    $actualReceiptProperties = @($receipt.PSObject.Properties.Name)
    if ($actualReceiptProperties.Count -ne $expectedReceiptProperties.Count) {
        throw "$Label result receipt schema is not exact."
    }
    foreach ($propertyName in $expectedReceiptProperties) {
        if ($actualReceiptProperties -cnotcontains $propertyName) {
            throw "$Label result receipt schema is not exact."
        }
    }
    if ($receipt.SchemaVersion -isnot [int] -or
        [int]$receipt.SchemaVersion -ne 1 -or
        $receipt.Mode -isnot [string] -or
        [string]$receipt.Mode -cne [string]$Child.Mode -or
        $receipt.Token -isnot [string] -or
        [string]$receipt.Token -cne [string]$Child.Token -or
        $receipt.InvocationCompleted -isnot [bool] -or
        $receipt.Succeeded -isnot [bool] -or
        $receipt.Error -isnot [string] -or
        [bool]$receipt.Succeeded -ne
            [string]::IsNullOrEmpty([string]$receipt.Error)) {
        throw "$Label result receipt values are invalid."
    }
    if ($exitCode -ne 0 -or
        -not [bool]$receipt.InvocationCompleted -or
        -not [string]::IsNullOrWhiteSpace($standardOutput) -or
        -not [string]::IsNullOrWhiteSpace($standardError)) {
        throw ("$Label child transport failed. ExitCode=$exitCode; " +
            "ReceiptError=$([string]$receipt.Error); " +
            "StandardError=$standardError; StandardOutput=$standardOutput")
    }
    $outcome = [pscustomobject]@{
        ExitCode = $exitCode
        Receipt = $receipt
        StandardOutput = $standardOutput
        StandardError = $standardError
    }
    $Child.Outcome = $outcome
    return $outcome
}

function Wait-CorrectedCanaryProducerProcessBarrier {
    param(
        [Parameter(Mandatory = $true)]$Barrier,
        [Parameter(Mandatory = $true)]$Child,
        [Parameter(Mandatory = $true)][string]$Label,
        [int]$TimeoutSeconds = 300
    )

    $stopwatch = [Diagnostics.Stopwatch]::StartNew()
    while ($stopwatch.Elapsed.TotalSeconds -lt $TimeoutSeconds) {
        if ($Barrier.WaitOne(250)) {
            return
        }
        if ($Child.Process.HasExited) {
            if ($Barrier.WaitOne(0)) {
                return
            }
            try {
                $outcome = Complete-CorrectedCanaryProducerProcess `
                    -Child $Child `
                    -Label $Label `
                    -TimeoutSeconds 5
            }
            catch {
                throw "$Label ended before signaling: $($_.Exception.Message)"
            }
            throw ("$Label ended before signaling. Succeeded={0}; Error={1}" -f `
                    [bool]$outcome.Receipt.Succeeded,
                    [string]$outcome.Receipt.Error)
        }
    }
    if ($Barrier.WaitOne(0)) {
        return
    }
    if ($Child.Process.HasExited) {
        if ($Barrier.WaitOne(0)) {
            return
        }
        try {
            $outcome = Complete-CorrectedCanaryProducerProcess `
                -Child $Child `
                -Label $Label `
                -TimeoutSeconds 5
        }
        catch {
            throw "$Label ended at its timeout boundary: $($_.Exception.Message)"
        }
        throw ("$Label ended at its timeout boundary. Succeeded={0}; Error={1}" -f `
                [bool]$outcome.Receipt.Succeeded,
                [string]$outcome.Receipt.Error)
    }
    throw "$Label did not signal within $TimeoutSeconds seconds."
}

function Stop-CorrectedCanaryProducerProcess {
    param(
        [Parameter(Mandatory = $true)]$Child,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $process = $Child.Process
    try {
        if (-not $process.HasExited -and -not $process.WaitForExit(2000)) {
            try {
                $taskKillPath = Join-Path $env:SystemRoot 'System32\taskkill.exe'
                if (Test-Path -LiteralPath $taskKillPath -PathType Leaf) {
                    $taskKillInfo = New-Object Diagnostics.ProcessStartInfo
                    $taskKillInfo.FileName = $taskKillPath
                    $taskKillInfo.Arguments = "/PID $([int]$process.Id) /T /F"
                    $taskKillInfo.UseShellExecute = $false
                    $taskKillInfo.CreateNoWindow = $true
                    $taskKillInfo.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
                    $taskKillInfo.RedirectStandardOutput = $true
                    $taskKillInfo.RedirectStandardError = $true
                    $taskKillProcess = New-Object Diagnostics.Process
                    $taskKillProcess.StartInfo = $taskKillInfo
                    try {
                        if ($taskKillProcess.Start()) {
                            $taskKillOutput =
                                $taskKillProcess.StandardOutput.ReadToEndAsync()
                            $taskKillError =
                                $taskKillProcess.StandardError.ReadToEndAsync()
                            [void]$taskKillProcess.WaitForExit(5000)
                            [void]$taskKillOutput.Wait(5000)
                            [void]$taskKillError.Wait(5000)
                        }
                    }
                    finally {
                        $taskKillProcess.Dispose()
                    }
                }
            }
            catch {
                # The exact-process fallback below remains mandatory even when
                # tree termination is unavailable.
            }
            if (-not $process.HasExited) {
                try {
                    $process.Kill()
                }
                catch {
                    # The bounded WaitForExit below reports a surviving child.
                }
            }
        }
        if (-not $process.WaitForExit(5000)) {
            throw "$Label child process could not be stopped."
        }
        $process.WaitForExit()
        [void](Get-CorrectedCanaryProcessStreamText `
            -Task $Child.StandardOutputTask `
            -Label "$Label cleanup standard output")
        [void](Get-CorrectedCanaryProcessStreamText `
            -Task $Child.StandardErrorTask `
            -Label "$Label cleanup standard error")
    }
    finally {
        $process.Dispose()
    }

    $expectedRoot = [IO.Path]::GetFullPath((Join-Path `
            $Child.TempParent `
            "producer-child-$($Child.Mode)-$($Child.Token)"))
    if (-not [IO.Path]::GetFullPath([string]$Child.Root).Equals(
            $expectedRoot,
            [StringComparison]::OrdinalIgnoreCase) -or
        -not [IO.Path]::GetDirectoryName($expectedRoot).Equals(
            [IO.Path]::GetFullPath([string]$Child.TempParent).TrimEnd('\', '/'),
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label child cleanup root is not directly contained."
    }
    if (Test-Path -LiteralPath $expectedRoot -PathType Container) {
        $rootItem = Get-Item -LiteralPath $expectedRoot -Force -ErrorAction Stop
        $rootChildren = @(Get-ChildItem -LiteralPath $expectedRoot -Force)
        if (($rootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
            @($rootChildren | Where-Object {
                    $_.PSIsContainer -or
                    ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0
                }).Count -ne 0) {
            throw "$Label child cleanup root contains an unsafe entry."
        }
        Remove-Item -LiteralPath $expectedRoot -Recurse -Force -ErrorAction Stop
        if (Test-Path -LiteralPath $expectedRoot) {
            throw "$Label child cleanup root could not be removed."
        }
    }
}

function New-Assertion {
    param(
        [string]$Id,
        [string]$Status = 'PASS',
        [bool]$CountsTowardCertification = $false,
        [string]$ProofLevel = 'CONTROLLED_RUNTIME',
        [string]$ObservedPath = 'synthetic_runtime_probe',
        [string]$RequiredPath = 'typed runtime proof',
        [string]$Expected = '',
        [string]$Actual = '',
        [string]$FailureReason = ''
    )

    return [pscustomobject][ordered]@{
        m_sAssertionId = $Id
        m_sExpected = if ([string]::IsNullOrEmpty($Expected)) { "expected $Id" } else { $Expected }
        m_sActual = if ([string]::IsNullOrEmpty($Actual)) { "actual $Id" } else { $Actual }
        m_sStatus = $Status
        m_sFailureReason = if ([string]::IsNullOrEmpty($FailureReason)) {
            "synthetic $Status disposition for $Id"
        }
        else {
            $FailureReason
        }
        m_sProofLevel = $ProofLevel
        m_sObservedPath = $ObservedPath
        m_sRequiredPath = $RequiredPath
        m_bCountsTowardCertification = $CountsTowardCertification
        m_vExpectedPosition = @(0, 0, 0)
        m_vActualPosition = @(0, 0, 0)
        m_fDistanceMeters = 0
        m_sEntityId = ''
        m_sMissionInstanceId = ''
        m_sZoneId = ''
        m_sOrderId = ''
    }
}

function New-Case {
    param(
        [string]$Id,
        [string]$Status = 'PASS',
        [object[]]$Assertions = @(),
        [string]$Category = 'foundation',
        [string]$Feature = 'corrected_canary',
        [string]$Stage = 'runtime',
        [object[]]$Metrics = @()
    )

    return [pscustomobject][ordered]@{
        m_sCaseId = $Id
        m_sCategory = $Category
        m_sFeature = $Feature
        m_sStage = $Stage
        m_sStatus = $Status
        m_sReason = if ($Status -ceq 'PASS') {
            'assertions passed'
        }
        else {
            'synthetic corrected-canary disposition'
        }
        m_iStartSecond = 0
        m_iEndSecond = 1
        m_aAssertions = $Assertions
        m_aMetrics = $Metrics
        m_aEvidence = @('synthetic portable corrected-canary fixture')
    }
}

function New-Metric {
    param(
        [string]$Id,
        [string]$Value,
        [string]$Unit = '',
        [string]$Feature = '',
        [string]$Stage = '')

    return [pscustomobject][ordered]@{
        m_sMetricId = $Id
        m_sName = $Id
        m_sValue = $Value
        m_sUnit = $Unit
        m_sFeature = $Feature
        m_sStage = $Stage
    }
}

function Get-RunnerCorrectedCanaryContracts {
    param([Parameter(Mandatory = $true)][string]$RunId)

    $tokens = $null
    $parseErrors = $null
    $ast = [Management.Automation.Language.Parser]::ParseFile(
        $runnerPath,
        [ref]$tokens,
        [ref]$parseErrors)
    if (@($parseErrors).Count -ne 0) {
        throw 'The guarded runner could not be parsed for corrected-canary fixture contracts.'
    }
    $source = New-Object Collections.Generic.List[string]
    foreach ($name in @(
            'Get-FocusedForceAuthorityAssertionIds',
            'Get-CorrectedCanaryCaseManifest',
            'Get-CorrectedCanaryAssertionManifest',
            'Get-CampaignDebugStateDiffLabels')) {
        $matches = @($ast.FindAll({
                    param($node)
                    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
                        $node.Name -ceq $name
                }, $true))
        if ($matches.Count -ne 1) {
            throw "The guarded runner does not expose exactly one $name fixture contract."
        }
        [void]$source.Add($matches[0].Extent.Text)
    }
    $contractScript = [scriptblock]::Create(@"
param(`$FixtureRunId)
$($source.ToArray() -join "`n`n")
[pscustomobject]@{
    Cases = @(Get-CorrectedCanaryCaseManifest -RunId `$FixtureRunId)
    Assertions = @(Get-CorrectedCanaryAssertionManifest -RunId `$FixtureRunId)
    StateDiffLabels = @(Get-CampaignDebugStateDiffLabels)
}
"@)
    return & $contractScript $RunId
}

function ConvertTo-RecordedValidationSummary {
    param([Parameter(Mandatory = $true)]$Validation)

    return [pscustomobject][ordered]@{
        Valid = [bool]$Validation.Valid
        Problems = @($Validation.Problems)
        RunId = [string]$Validation.RunId
        Profile = [string]$Validation.Profile
        ProofScope = [string]$Validation.ProofScope
        FullCertification = [bool]$Validation.FullCertification
        BuildProvenanceMatches =
            [string]$Validation.BuildSha -ceq $embeddedBuildSha -and
            [string]$Validation.BuildUtc -ceq $embeddedBuildUtc -and
            [string]$Validation.BuildLabel -ceq $embeddedBuildLabel
        StartedAtSecond = [int]$Validation.StartedAtSecond
        EndedAtSecond = [int]$Validation.EndedAtSecond
        CaseCount = [int]$Validation.CaseCount
        Pass = [int]$Validation.Pass
        Warn = [int]$Validation.Warn
        Fail = [int]$Validation.Fail
        Blocked = [int]$Validation.Blocked
        Skipped = [int]$Validation.Skipped
        CertificationRequired = [int]$Validation.CertificationRequired
        CertificationProven = [int]$Validation.CertificationProven
        CertificationFail = [int]$Validation.CertificationFail
        CertificationBlocked = [int]$Validation.CertificationBlocked
        CertificationWarn = [int]$Validation.CertificationWarn
        CertificationPassed = [bool]$Validation.CertificationPassed
        CorrectedCanaryContract = [bool]$Validation.CorrectedCanaryContract
        Trigger = [string]$Validation.Trigger
        ArtifactCount = [int]$Validation.ArtifactCount
        StateDiffRows = [int]$Validation.StateDiffRows
        NonzeroStateDiffRows = [int]$Validation.NonzeroStateDiffRows
        StateDiffManifestExact = [bool]$Validation.StateDiffManifestExact
        Phase17 = @($Validation.Phase17)
        Phase17Metrics = $Validation.Phase17Metrics
        Phase24 = @($Validation.Phase24)
        Phase24Metrics = $Validation.Phase24Metrics
        StagedCleanup = @($Validation.StagedCleanup)
        FocusedCaseId = [string]$Validation.FocusedCaseId
        FocusedCaseStatus = [string]$Validation.FocusedCaseStatus
        FocusedAssertions = @($Validation.FocusedAssertions)
        CorrectedCanaryAssertionManifestExact =
            [bool]$Validation.CorrectedCanaryAssertionManifestExact
        CorrectedCanaryCaseSetExact = [bool]$Validation.CorrectedCanaryCaseSetExact
        CorrectedCanaryWarningContractExact =
            [bool]$Validation.CorrectedCanaryWarningContractExact
        CorrectedCanaryBlockedContractExact =
            [bool]$Validation.CorrectedCanaryBlockedContractExact
        CorrectedCanaryOrphanContractExact =
            [bool]$Validation.CorrectedCanaryOrphanContractExact
        IntentionalMissionConvoyAdmissionDiagnosticsProven = $false
        IntentionalMissionConvoySettlementDiagnosticProven = $false
        IntentionalMissionConvoyCorruptionDiagnosticsProven = $false
        IntentionalMissionConvoyWatchdogDiagnosticProven = $false
        FinalOrphanCleanupPass = [bool]$Validation.FinalOrphanCleanupPass
        FinalOrphanActiveGroups = [string]$Validation.FinalOrphanActiveGroups
    }
}

function Set-RawCounts {
    param($Raw)

    $cases = @($Raw.m_aCases)
    $Raw.m_iPassCount = @($cases | Where-Object m_sStatus -CEQ 'PASS').Count
    $Raw.m_iWarnCount = @($cases | Where-Object m_sStatus -CEQ 'WARN').Count
    $Raw.m_iFailCount = @($cases | Where-Object m_sStatus -CEQ 'FAIL').Count
    $Raw.m_iBlockedCount = @($cases | Where-Object m_sStatus -CEQ 'BLOCKED').Count
    $Raw.m_iSkippedCount = @($cases | Where-Object m_sStatus -CEQ 'SKIPPED').Count
    $certificationAssertions = @($cases | ForEach-Object { @($_.m_aAssertions) } |
        Where-Object { [bool]$_.m_bCountsTowardCertification })
    $Raw.m_iCertificationRequiredCount = $certificationAssertions.Count
    $Raw.m_iCertificationProvenCount = @($certificationAssertions |
        Where-Object m_sStatus -CEQ 'PASS').Count
    $Raw.m_iCertificationFailCount = @($certificationAssertions |
        Where-Object m_sStatus -CEQ 'FAIL').Count
    $Raw.m_iCertificationBlockedCount = @($certificationAssertions |
        Where-Object m_sStatus -CEQ 'BLOCKED').Count
    $Raw.m_iCertificationWarnCount = @($certificationAssertions |
        Where-Object m_sStatus -CEQ 'WARN').Count
    $Raw.m_bCertificationPassed = $false
}

function Get-FocusedAssertionIds {
    return @(
        'combat_presence.aggregate',
        'combat_presence.empty_vehicle',
        'combat_presence.authoritative_samples',
        'combat_presence.rejected_rows',
        'combat_presence.heat_lifecycle',
        'combat_presence.schema62_migration',
        'combat_presence.schema63_restore',
        'combat_presence.malformed_fail_cold',
        'combat_presence.deterministic_diagnostics',
        'ownership_transition.aggregate',
        'ownership_transition.military',
        'ownership_transition.political',
        'ownership_transition.recapture',
        'ownership_transition.replay',
        'ownership_transition.restore',
        'ownership_transition.restore_queue_order',
        'ownership_transition.persistence_deadline',
        'ownership_transition.projection_revision',
        'ownership_transition.location_identity',
        'ownership_transition.linked_support',
        'ownership_transition.causes',
        'ownership_transition.security_fail_closed',
        'ownership_transition.migration_retention',
        'town_influence.aggregate',
        'town_influence.scaling',
        'town_influence.hysteresis',
        'town_influence.idempotency',
        'town_influence.projection',
        'town_influence.population',
        'town_influence.rejection',
        'town_influence.ownership_authority',
        'town_influence.external_completion',
        'town_influence.pre64_invader',
        'town_influence.migration',
        'town_influence.current_restore')
}

function Get-AcceptedDiagnosticLogText {
    return ((@(
        '17:00:00.000 SCRIPT (E): Virtual Machine Exception',
        'Reason: Wrong parameter value',
        "Class: 'SCR_EditableEntityCore'",
        "Function: 'GetPlayerIdentityId'",
        'Stack trace:',
        'Scripts/Game/Utilities/SCR_PlayerIdentityUtils.c:29 Function GetPlayerIdentityId',
        'Scripts/Game/Editor/Core/SCR_EditableEntityCore.c:1189 Function OnPlayerIdentityAvailable',
        'Scripts/Game/GameMode/SCR_BaseGameMode.c:842 Function OnPlayerAuditSuccess',
        '17:00:00.001 SCRIPT (E): Virtual Machine Exception',
        'Reason: Wrong parameter value',
        "Class: 'SCR_ReconnectComponent'",
        "Function: 'GetPlayerIdentityId'",
        'Stack trace:',
        'Scripts/Game/Utilities/SCR_PlayerIdentityUtils.c:29 Function GetPlayerIdentityId',
        'Scripts/Game/GameMode/Components/SCR_ReconnectComponent.c:135 Function HandlePlayerReconnect',
        'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:534 Function ResolveReconnection',
        'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:329 Function OnPlayerDataLoaded_S',
        'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:298 Function RequestPlayerData_S',
        'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:273 Function ExcuteInitialLoadOrSpawn_S',
        'Scripts/Game/Respawn/Logic/SCR_AutoSpawnLogic.c:18 Function OnPlayerAuditSuccess_S',
        'Scripts/Game/GameMode/Respawn/SCR_RespawnSystemComponent.c:258 Function OnPlayerAuditSuccess_S',
        'Scripts/Game/GameMode/SCR_BaseGameMode.c:845 Function OnPlayerAuditSuccess',
        '17:00:01.000 SCRIPT : Partisan campaign debug CLI | armed focused force_authority run (not full certification)',
        '17:00:01.100 SCRIPT : Partisan campaign debug CLI | started force_authority on attempt 1',
        '17:00:03.000 SCRIPT : Partisan campaign debug | PASS | early_mechanics.force_authority | assertions passed'
    ) -join "`n") + "`n")
}

function Invoke-RunnerSemanticValidation {
    param(
        [string]$JsonPath,
        [string]$SummaryPath,
        [string]$StateDiffPath,
        [string]$GuardRoot
    )

    $tokens = $null
    $parseErrors = $null
    $ast = [Management.Automation.Language.Parser]::ParseFile(
        $runnerPath,
        [ref]$tokens,
        [ref]$parseErrors)
    if (@($parseErrors).Count -ne 0) {
        throw 'The guarded runner could not be parsed by the corrected-canary self-test.'
    }
    $requiredFunctions = @(
        'Read-SharedFileText', 'Find-Case', 'Find-Assertion', 'Get-MetricValue',
        'Test-RunMetric', 'Test-ExactPassingAssertion', 'Get-ExactAssertionStatus',
        'Get-ExactAssertionActual', 'Get-FocusedForceAuthorityAssertionIds',
        'Get-CorrectedCanaryCaseManifest', 'Get-CorrectedCanaryAssertionManifest',
        'Get-CampaignDebugStateDiffLabels', 'Get-CampaignDebugStateDiffValidation',
        'Test-CampaignDebugArtifacts', 'Get-CampaignDebugHardDiagnosticCensus',
        'Get-AuxiliaryDiagnosticProjection', 'Get-GuardErrorCensus')
    $functionSource = New-Object Collections.Generic.List[string]
    foreach ($functionName in $requiredFunctions) {
        $matches = @($ast.FindAll({
                    param($node)
                    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
                        $node.Name -ceq $functionName
                }, $true))
        if ($matches.Count -ne 1) {
            throw "The guarded runner does not expose exactly one $functionName function."
        }
        [void]$functionSource.Add($matches[0].Extent.Text)
    }
    $semanticScript = [scriptblock]::Create(@"
param(`$ArtifactParameters, `$DiagnosticParameters)
$($functionSource.ToArray() -join "`n`n")
`$artifactValidation = Test-CampaignDebugArtifacts @ArtifactParameters
`$errorCensus = Get-GuardErrorCensus @DiagnosticParameters
[pscustomobject]@{
    ArtifactValidation = `$artifactValidation
    ErrorCensus = `$errorCensus
}
"@)
    return & $semanticScript @{
        JsonPath = $JsonPath
        SummaryPath = $SummaryPath
        StateDiffPath = $StateDiffPath
        ExpectedSha = $embeddedBuildSha
        ExpectedUtc = $embeddedBuildUtc
        ExpectedLabel = $embeddedBuildLabel
        ExpectedProfile = 'force_authority'
        RequireCorrectedCanaryContract = $true
    } @{
        GuardRoot = $GuardRoot
        Profile = 'force_authority'
        IntentionalMissionConvoyAdmissionDiagnosticsProven = $false
        IntentionalMissionConvoySettlementDiagnosticProven = $false
        IntentionalMissionConvoyCorruptionDiagnosticsProven = $false
        IntentionalMissionConvoyWatchdogDiagnosticProven = $false
    }
}

function New-CorrectedCanaryFixture {
    param(
        [string]$FixtureRoot,
        [ValidateSet(
            'green', 'proof-red', 'warning-red', 'blocked-parent-red',
            'unexpected-blocker', 'certifying-blocker', 'diagnostic-red',
            'hidden-skip', 'focused-certification-swap',
            'balanced-certification-swap', 'nonfocused-id-substitution',
            'diff-missing', 'diff-duplicate', 'diff-renamed', 'diff-order',
            'diff-arithmetic', 'diff-nonnumeric', 'diff-nonzero',
            'diff-extra-line',
            'orphan-id-red', 'orphan-metric-id-red', 'orphan-metric-case-red',
            'orphan-metric-name-red', 'orphan-metadata-red', 'orphan-actual-red',
            'orphan-certification-red', 'error-log-red', 'crash-log-red')]
        [string]$Mode = 'green'
    )

    $leaf = '20260719T120000Z-' + [Guid]::NewGuid().ToString('N')
    $bundle = Join-Path $FixtureRoot $leaf
    New-Item -ItemType Directory -Path $bundle -Force | Out-Null

    $contracts = Get-RunnerCorrectedCanaryContracts -RunId $runId
    $cases = New-Object Collections.Generic.List[object]
    foreach ($caseContract in @($contracts.Cases)) {
        $assertions = New-Object Collections.Generic.List[object]
        foreach ($assertionContract in @($contracts.Assertions | Where-Object {
                    [string]$_.CaseId -ceq [string]$caseContract.Id
                })) {
            $parameters = @{
                Id = [string]$assertionContract.Id
                Status = [string]$assertionContract.Status
                CountsTowardCertification =
                    [bool]$assertionContract.CountsTowardCertification
            }
            if ([string]$assertionContract.Id -ceq 'cleanup.player_marker.live') {
                $parameters.Expected =
                    'enabled player marker service has desired/tracked/live marker after cleanup'
                $parameters.Actual =
                    'enabled 1 | desired 0 | tracked 0 | live 0 | entry 1'
                $parameters.FailureReason =
                    'player marker did not reconcile after campaign debug completion'
                $parameters.ProofLevel = 'STATE_ONLY'
                $parameters.ObservedPath = 'diagnostic_only'
                $parameters.RequiredPath = 'no debug-owned state or world leak'
            }
            elseif ([string]$assertionContract.Id -ceq 'isolation.world_scope') {
                $parameters.Expected =
                    'runtime certification remains scoped to the disposable development session'
                $parameters.Actual =
                    'world runtime, player inventory, health, and service caches require session restart before another certifying run'
                $parameters.FailureReason =
                    'restart the disposable development session before another certification run'
                $parameters.ProofLevel = 'EXTERNAL_PROCESS'
                $parameters.ObservedPath = 'manual_external_gap'
                $parameters.RequiredPath =
                    'external process restart, reconnect, or long-soak harness'
            }
            elseif ([string]$assertionContract.Id -ceq
                'cleanup.orphan_active_groups') {
                $parameters.Expected =
                    'no active groups without zone/mission/support/order/QRF backing'
                $parameters.Actual = '0 | total 0 | debug 0 | smoke 0 | other 0'
                $parameters.FailureReason =
                    'orphan active groups remain after debug run'
                $parameters.ProofLevel = 'STATE_ONLY'
                $parameters.ObservedPath = 'cleanup_probe'
                $parameters.RequiredPath = 'no debug-owned state or world leak'
            }
            [void]$assertions.Add((New-Assertion @parameters))
        }
        $metrics = @()
        if ([string]$caseContract.Id -ceq 'cleanup.run_leak_snapshot') {
            $metrics = @((New-Metric `
                -Id 'cleanup.orphan_active_groups' `
                -Value '0' `
                -Unit 'count' `
                -Feature 'campaign_debug' `
                -Stage 'final'))
        }
        [void]$cases.Add((New-Case `
            -Id ([string]$caseContract.Id) `
            -Status ([string]$caseContract.Status) `
            -Assertions $assertions.ToArray() `
            -Category ([string]$caseContract.Category) `
            -Feature ([string]$caseContract.Feature) `
            -Stage ([string]$caseContract.Stage) `
            -Metrics $metrics))
    }

    $focusedCase = @($cases | Where-Object {
        $_.m_sCaseId -ceq 'early_mechanics.force_authority'
    })[0]
    if ($Mode -ceq 'proof-red') {
        foreach ($assertionId in @(
                'ownership_transition.military',
                'ownership_transition.political')) {
            (@($focusedCase.m_aAssertions | Where-Object {
                $_.m_sAssertionId -ceq $assertionId
            })[0]).m_sStatus = 'FAIL'
        }
        $focusedCase.m_sStatus = 'FAIL'
    }
    if ($Mode -ceq 'focused-certification-swap') {
        $focusedCase.m_aAssertions[0].m_bCountsTowardCertification = $false
        (@($focusedCase.m_aAssertions | Where-Object {
            $_.m_sAssertionId -ceq 'town_influence.external_completion'
        })[0]).m_bCountsTowardCertification = $true
    }
    $markerCase = @($cases | Where-Object {
        $_.m_sCaseId -ceq 'cleanup.player_marker_completion'
    })[0]
    $markerWarning = @($markerCase.m_aAssertions | Where-Object {
        $_.m_sAssertionId -ceq 'cleanup.player_marker.live'
    })[0]
    if ($Mode -ceq 'warning-red') {
        $markerWarning.m_sAssertionId = 'cleanup.player_marker.unexpected'
    }
    if ($Mode -ceq 'hidden-skip') {
        $markerCase.m_aAssertions += New-Assertion `
            -Id 'cleanup.player_marker.hidden_skip' `
            -Status 'SKIPPED'
    }
    $worldScopeCase = @($cases | Where-Object {
        $_.m_sCaseId -ceq 'cleanup.state_isolation_restore'
    })[0]
    $worldScopeBlock = @($worldScopeCase.m_aAssertions | Where-Object {
        $_.m_sAssertionId -ceq 'isolation.world_scope'
    })[0]
    if ($Mode -ceq 'unexpected-blocker') {
        $worldScopeBlock.m_sAssertionId = 'isolation.unexpected_blocker'
    }
    if ($Mode -ceq 'certifying-blocker') {
        $worldScopeBlock.m_bCountsTowardCertification = $true
    }
    if ($Mode -ceq 'blocked-parent-red') {
        $worldScopeTarget = @($cases | Where-Object {
            $_.m_sCaseId -ceq
                'post_case_cleanup.early_mechanics_force_authority'
        })[0]
        $worldScopeCase.m_aAssertions = @(
            $worldScopeCase.m_aAssertions | Where-Object {
                $_.m_sAssertionId -cne 'isolation.world_scope'
            })
        $worldScopeCase.m_sStatus = 'PASS'
        $worldScopeTarget.m_aAssertions = @(
            $worldScopeTarget.m_aAssertions + @($worldScopeBlock))
        $worldScopeTarget.m_sStatus = 'BLOCKED'
    }
    if ($Mode -ceq 'balanced-certification-swap') {
        $cases[0].m_aAssertions[0].m_bCountsTowardCertification = $false
        $smokeAssertion = @($cases | ForEach-Object { @($_.m_aAssertions) } |
            Where-Object m_sAssertionId -CEQ 'cleanup.smoke_prefixed_records')[0]
        $smokeAssertion.m_bCountsTowardCertification = $true
    }
    if ($Mode -ceq 'nonfocused-id-substitution') {
        $cases[0].m_aAssertions[0].m_sAssertionId =
            'isolation.development_world.substituted'
    }
    $orphanCase = @($cases | Where-Object {
        $_.m_sCaseId -ceq 'cleanup.run_leak_snapshot'
    })[0]
    $orphanAssertion = @($orphanCase.m_aAssertions | Where-Object {
        $_.m_sAssertionId -ceq 'cleanup.orphan_active_groups'
    })[0]
    if ($Mode -ceq 'orphan-id-red') {
        $orphanAssertion.m_sAssertionId = 'cleanup.orphan_Active_groups'
    }
    if ($Mode -ceq 'orphan-certification-red') {
        $orphanAssertion.m_bCountsTowardCertification = $false
    }
    if ($Mode -ceq 'orphan-metric-id-red') {
        $orphanCase.m_aMetrics[0].m_sMetricId = 'cleanup.orphan_active_group'
    }
    if ($Mode -ceq 'orphan-metric-case-red') {
        $orphanCase.m_aMetrics[0].m_sMetricId = 'cleanup.Orphan_active_groups'
    }
    if ($Mode -ceq 'orphan-metric-name-red') {
        $orphanCase.m_aMetrics[0].m_sName = 'cleanup.Orphan_active_groups'
    }
    if ($Mode -ceq 'orphan-metadata-red') {
        $orphanAssertion.m_sObservedPath = 'Cleanup_probe'
    }
    if ($Mode -ceq 'orphan-actual-red') {
        $orphanAssertion.m_sActual =
            '0 | total 1 | debug 0 | smoke 0 | other 1'
    }

    $artifactName = "HST_CampaignDebug_$runId.json"
    $summaryName = "HST_CampaignDebug_${runId}_summary.txt"
    $diffName = "HST_CampaignDebug_${runId}_state_diff.txt"
    $raw = [pscustomobject][ordered]@{
        m_sRunId = $runId
        m_sProfile = 'force_authority'
        m_sCampaignSeed = '1985'
        m_sPlayerIdentityId = 'synthetic-admin'
        m_sWorldName = 'HST_Dev'
        m_sBuildSha = $embeddedBuildSha
        m_sBuildUtc = $embeddedBuildUtc
        m_sBuildLabel = $embeddedBuildLabel
        m_sMarkerPrefix = "hst_debug_$runId"
        m_sMissionPrefix = "hst_debug_${runId}_mission_"
        m_sEntityTag = 'HST_CAMPAIGN_DEBUG'
        m_iStartedAtSecond = 0
        m_iEndedAtSecond = 1
        m_iPassCount = 0
        m_iWarnCount = 0
        m_iFailCount = 0
        m_iBlockedCount = 0
        m_iSkippedCount = 0
        m_iCertificationRequiredCount = 0
        m_iCertificationProvenCount = 0
        m_iCertificationFailCount = 0
        m_iCertificationBlockedCount = 0
        m_iCertificationWarnCount = 0
        m_bCertificationPassed = $false
        m_aCases = $cases.ToArray()
        m_aMetrics = @(
            [pscustomobject]@{ m_sMetricId = 'run.build.sha'; m_sValue = $embeddedBuildSha; m_sUnit = 'sha'; m_sFeature = 'campaign_debug'; m_sStage = 'run' },
            [pscustomobject]@{ m_sMetricId = 'run.build.utc'; m_sValue = $embeddedBuildUtc; m_sUnit = 'utc'; m_sFeature = 'campaign_debug'; m_sStage = 'run' },
            [pscustomobject]@{ m_sMetricId = 'run.build.label'; m_sValue = $embeddedBuildLabel; m_sUnit = 'label'; m_sFeature = 'campaign_debug'; m_sStage = 'run' },
            [pscustomobject]@{ m_sMetricId = 'run.trigger'; m_sValue = 'cli_autostart'; m_sUnit = 'source'; m_sFeature = 'campaign_debug'; m_sStage = 'run' })
        m_aArtifacts = @($artifactName, $summaryName, $diffName)
    }
    Set-RawCounts $raw

    $artifactRelative = "raw/campaign-debug/$artifactName"
    $summaryRelative = "raw/campaign-debug/$summaryName"
    $diffRelative = "raw/campaign-debug/$diffName"
    Write-Json (Join-Path $bundle $artifactRelative) $raw
    Write-Utf8Text `
        -Path (Join-Path $bundle $summaryRelative) `
        -Text ("Partisan campaign debug complete`nrun $runId`n" +
            "profile force_authority`nbuild source $embeddedBuildSha | " +
            "UTC $embeddedBuildUtc | label $embeddedBuildLabel`n")
    $diffLines = New-Object Collections.Generic.List[string]
    [void]$diffLines.Add('Partisan campaign debug state diff')
    [void]$diffLines.Add("run $runId")
    $diffLabels = @($contracts.StateDiffLabels)
    if ($Mode -ceq 'diff-missing') {
        $diffLabels = @($diffLabels | Select-Object -First 17)
    }
    elseif ($Mode -ceq 'diff-duplicate') {
        $diffLabels[17] = $diffLabels[16]
    }
    elseif ($Mode -ceq 'diff-renamed') {
        $diffLabels[8] = 'mission asset'
    }
    elseif ($Mode -ceq 'diff-order') {
        $swap = $diffLabels[3]
        $diffLabels[3] = $diffLabels[4]
        $diffLabels[4] = $swap
    }
    foreach ($label in $diffLabels) {
        [void]$diffLines.Add("$label 0 -> 0 | delta 0")
    }
    if ($Mode -ceq 'diff-arithmetic') {
        $diffLines[2] = 'elapsed 0 -> 1 | delta 0'
    }
    elseif ($Mode -ceq 'diff-nonnumeric') {
        $diffLines[2] = 'elapsed before -> 0 | delta 0'
    }
    elseif ($Mode -ceq 'diff-nonzero') {
        $diffLines[2] = 'elapsed 0 -> 1 | delta 1'
    }
    elseif ($Mode -ceq 'diff-extra-line') {
        [void]$diffLines.Add('unexpected 0 -> 0 | delta 0')
    }
    Write-Utf8Text `
        -Path (Join-Path $bundle $diffRelative) `
        -Text (($diffLines.ToArray() -join "`n") + "`n")

    Write-Utf8Text `
        -Path (Join-Path $bundle 'config/HST_Settings.json') `
        -Text "{`"schemaVersion`":$runtimeSettingsSchema}`n"
    $manifest = [pscustomobject][ordered]@{
        manifestSchemaVersion = 1
        createdUtc = '2026-07-19T11:55:00Z'
        candidate = [pscustomobject][ordered]@{
            id = $candidateId
            version = $candidateVersion
            state = 'retained-uncertified'
        }
        source = [pscustomobject][ordered]@{
            gitHead = $candidateHead
            embeddedImplementation = [pscustomobject][ordered]@{
                sha = $embeddedBuildSha
                utc = $embeddedBuildUtc
                label = $embeddedBuildLabel
            }
            campaignSchema = $campaignSchema
            runtimeSettingsSchema = $runtimeSettingsSchema
        }
        addon = [pscustomobject][ordered]@{
            id = $addonId
            guid = $addonGuid
            version = $candidateVersion
        }
        toolchain = [pscustomobject][ordered]@{
            client = $clientRuntimeIdentity
            clientDiagnostic = $clientDiagnosticIdentity
        }
        workbench = [pscustomobject][ordered]@{ crc = $workbenchCrc }
        package = [pscustomobject][ordered]@{
            root = 'package/Partisan'
            hashAlgorithm = $packageHashAlgorithm
            sha256 = $packageSha
            canonicalIndexPath = 'evidence/pack/files.sha256'
            files = $packageFiles
        }
    }
    $manifestPath = Join-Path $bundle 'identity/candidate.json'
    Write-Json $manifestPath $manifest
    $manifestSha = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $ready = [pscustomobject][ordered]@{
        schemaVersion = 1
        candidateId = $candidateId
        gitHead = $candidateHead
        packageSha256 = $packageSha
        manifestSha256 = $manifestSha
    }
    $readyPath = Join-Path $bundle 'identity/candidate.ready.json'
    Write-Json $readyPath $ready
    $readySha = (Get-FileHash -LiteralPath $readyPath -Algorithm SHA256).Hash.ToLowerInvariant()

    $acceptedDiagnosticText = Get-AcceptedDiagnosticLogText
    $diagnosticText = $acceptedDiagnosticText
    if ($Mode -ceq 'diagnostic-red') {
        $diagnosticText += '17:00:04.000 SCRIPT (E): Partisan synthetic unapproved diagnostic' + "`n"
    }
    foreach ($name in @('script.log', 'console.log')) {
        Write-Utf8Text `
            -Path (Join-Path $bundle "raw/logs/logs_synthetic/$name") `
            -Text $diagnosticText
    }
    $errorLogText = $acceptedDiagnosticText
    $crashLogText = $acceptedDiagnosticText
    if ($Mode -ceq 'error-log-red') {
        $errorLogText +=
            '17:00:04.000 SCRIPT (E): unique auxiliary error-log event' + "`n"
    }
    if ($Mode -ceq 'crash-log-red') {
        $crashLogText += 'fatal error: unique auxiliary crash-log event' + "`n"
    }
    Write-Utf8Text `
        -Path (Join-Path $bundle 'raw/logs/logs_synthetic/error.log') `
        -Text $errorLogText
    Write-Utf8Text `
        -Path (Join-Path $bundle 'raw/logs/logs_synthetic/crash.log') `
        -Text $crashLogText

    $settingsSha = (Get-FileHash `
        -LiteralPath (Join-Path $bundle 'config/HST_Settings.json') `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    $semantic = Invoke-RunnerSemanticValidation `
        -JsonPath (Join-Path $bundle $artifactRelative) `
        -SummaryPath (Join-Path $bundle $summaryRelative) `
        -StateDiffPath (Join-Path $bundle $diffRelative) `
        -GuardRoot (Join-Path $bundle 'raw')
    $validation = ConvertTo-RecordedValidationSummary `
        -Validation $semantic.ArtifactValidation
    $errorCensus = $semantic.ErrorCensus
    $outcomeError = ''
    if (-not $validation.Valid) {
        $outcomeError = 'Campaign Debug artifacts completed but failed the exact validation contract.'
    }
    elseif (-not $errorCensus.Valid) {
        $outcomeError = 'Campaign Debug runtime completed with unapproved hard diagnostics.'
    }

    $fileRows = @()
    foreach ($file in @(Get-ChildItem -LiteralPath $bundle -Recurse -File -Force |
            Sort-Object FullName)) {
        $fileRows += [pscustomobject][ordered]@{
            path = $file.FullName.Substring($bundle.Length + 1).Replace('\', '/')
            length = [long]$file.Length
            sha256 = (Get-FileHash `
                -LiteralPath $file.FullName `
                -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }
    $run = [pscustomobject][ordered]@{
        schemaVersion = 2
        evidenceKind = 'packaged-campaign-debug'
        startedUtc = '2026-07-19T12:00:00Z'
        completedUtc = '2026-07-19T12:00:10Z'
        candidate = [pscustomobject][ordered]@{
            candidateId = $candidateId
            candidateVersion = $candidateVersion
            runtimeUseDisposition = 'active-runtime-candidate'
            gitHead = $candidateHead
            embeddedBuildSha = $embeddedBuildSha
            embeddedBuildUtc = $embeddedBuildUtc
            embeddedBuildLabel = $embeddedBuildLabel
            campaignSchema = $campaignSchema
            runtimeSettingsSchema = $runtimeSettingsSchema
            addonId = $addonId
            addonGuid = $addonGuid
            packageHashAlgorithm = $packageHashAlgorithm
            packageSha256 = $packageSha
            manifestSha256 = $manifestSha
            readySha256 = $readySha
            workbenchCrc = $workbenchCrc
            runtimeRole = 'client'
            diagnosticExecutable = $clientDiagnosticIdentity
            recordedDiagnosticExecutable = $clientDiagnosticIdentity
            recordedRuntimeExecutable = $clientRuntimeIdentity
        }
        harness = [pscustomobject][ordered]@{
            gitHead = $harnessHead
            dirty = $false
            campaignRunnerSha256 = $runnerSha
            campaignRunnerGitBlobSha256 = $runnerGitBlobSha
            candidateModuleSha256 = $candidateModuleSha
            candidateModuleGitBlobSha256 = $candidateModuleGitBlobSha
            releaseIndexProducerSha256 = $producerSha
            releaseIndexProducerGitBlobSha256 = $producerGitBlobSha
            releaseDocsConsumerSha256 = $consumerSha
            releaseDocsConsumerGitBlobSha256 = $consumerGitBlobSha
        }
        launch = [pscustomobject][ordered]@{
            profile = 'force_authority'
            proofScope = 'focused_force_authority'
            worldResource = 'Worlds/HST_Dev/HST_Dev.ent'
            stagedPackage = $true
            addonSearchRootCount = 2
            addonGuid = $addonGuid
            packageSha256 = $packageSha
            diagnosticExecutable = $clientDiagnosticIdentity
            recordedRuntimeExecutable = $clientRuntimeIdentity
        }
        outcome = [pscustomobject][ordered]@{
            success = [string]::IsNullOrWhiteSpace($outcomeError)
            armed = $true
            started = $true
            completed = $true
            candidateBoundaryVerified = $true
            mountAttestation = [pscustomobject][ordered]@{
                Valid = $true
                RecordCount = 1
                ExactPathCount = 1
                PackedCount = 1
                InvalidModeCount = 0
                GuidExact = $true
                Packed = $true
            }
            artifactsStable = $true
            evidenceCaptured = $true
            hardDiagnosticClassifierChecks = 38
            runtimeSeconds = 10
            error = $outcomeError
            validation = $validation
            errorCensus = $errorCensus
        }
        settings = [pscustomobject][ordered]@{
            schemaVersion = $runtimeSettingsSchema
            sha256 = $settingsSha
            guardedRuntimeCopy = $true
        }
        cleanup = [pscustomobject][ordered]@{
            guardRemaining = 0
            ownedProcessesRemaining = 0
            newEngineProcessesRemaining = 0
            unclaimedEngineProcessesObserved = 0
            newDefaultEntriesRemaining = 0
            modifiedDefaultFiles = 0
            deletedDefaultEntries = 0
            missingDefaultRoots = 0
            externalSpillEntriesRemaining = 0
            modifiedSpillFiles = 0
            deletedSpillEntries = 0
            missingSpillRoots = 0
            cleanupPhaseErrorCount = 0
            cleanupPhaseErrors = @()
            monitoringRootsAreDetectionOnly = $true
        }
        files = $fileRows
    }
    $runPath = Join-Path $bundle 'run.json'
    Write-Json $runPath $run
    return [pscustomobject]@{
        Bundle = $bundle
        RunPath = $runPath
        IndexPath = Join-Path $bundle 'release-index.json'
        RawPath = Join-Path $bundle $artifactRelative
    }
}

function Invoke-Producer {
    param($Fixture)

    $rows = @(& $producerPath -RunEnvelopePath $Fixture.RunPath)
    if ($rows.Count -ne 1) {
        throw 'The corrected-canary producer returned an unexpected result count.'
    }
    return $rows[0]
}

function Assert-ProducerRejected {
    param(
        [string]$Label,
        [scriptblock]$Action,
        [string]$ExpectedMessage
    )

    try {
        & $Action | Out-Null
    }
    catch {
        if (-not [string]::IsNullOrWhiteSpace($ExpectedMessage) -and
            [string]$_.Exception.Message -cne $ExpectedMessage) {
            throw "Corrected-canary fail-closed self-test rejected $Label for the wrong reason: $($_.Exception.Message)"
        }
        return
    }
    throw "Corrected-canary fail-closed self-test did not reject $Label."
}

$tempParent = [IO.Path]::GetFullPath((Join-Path `
    ([IO.Path]::GetTempPath()) `
    ('PartisanCorrectedCanaryV2-' + [Guid]::NewGuid().ToString('N'))))
$expectedTempPrefix = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') +
    [IO.Path]::DirectorySeparatorChar
if (-not $tempParent.StartsWith(
        $expectedTempPrefix,
        [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Corrected-canary self-test temporary-root containment failed.'
}
New-Item -ItemType Directory -Path $tempParent -Force | Out-Null

try {
    $green = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'green')
    $greenReceipt = Invoke-Producer $green
    $greenIndex = Get-Content -Raw -LiteralPath $green.IndexPath | ConvertFrom-Json
    if ([int]$greenIndex.schemaVersion -ne 2 -or
        [string]$greenIndex.evidenceKind -cne
            'packaged-campaign-debug-corrected-canary' -or
        [string]$greenIndex.policyId -cne
            'partisan-campaign-debug-corrected-canary-v2' -or
        [string]$greenIndex.result.status -cne 'passed-noncertifying' -or
        [string]$greenIndex.result.acceptanceDisposition -cne
            'accepted-noncertifying' -or
        [string]$greenIndex.result.releaseDisposition -cne 'proceed-full-profile' -or
        [bool]$greenIndex.result.certificationPassed -or
        [int]$greenIndex.proof.caseCount -ne 11 -or
        [int]$greenIndex.proof.pass -ne 9 -or
        [int]$greenIndex.proof.warn -ne 1 -or
        [int]$greenIndex.proof.fail -ne 0 -or
        [int]$greenIndex.proof.blocked -ne 1 -or
        [int]$greenIndex.proof.skipped -ne 0 -or
        [string]$greenIndex.proof.focusedCaseId -cne
            'early_mechanics.force_authority' -or
        [string]$greenIndex.proof.focusedCaseStatus -cne 'PASS' -or
        [int]$greenIndex.proof.focusedAssertionCount -ne 35 -or
        [int]$greenIndex.proof.focusedAssertionsPassed -ne 35 -or
        [int]$greenIndex.proof.certificationRequired -ne 87 -or
        [int]$greenIndex.proof.certificationProven -ne 87 -or
        [int]$greenIndex.proof.certificationFail -ne 0 -or
        [int]$greenIndex.proof.certificationBlocked -ne 0 -or
        [int]$greenIndex.proof.certificationWarn -ne 0 -or
        [int]$greenIndex.proof.assertionCount -ne 91 -or
        [int]$greenIndex.proof.stateDiffRows -ne 18 -or
        [int]$greenIndex.proof.nonzeroStateDiffRows -ne 0 -or
        -not [bool]$greenIndex.proof.finalOrphanCleanupPass -or
        [int]$greenIndex.proof.finalOrphanActiveGroups -ne 0 -or
        -not [bool]$greenIndex.proof.focusedAssertionSetExact -or
        -not [bool]$greenIndex.proof.focusedAssertionsCertificationExact -or
        -not [bool]$greenIndex.proof.correctedCanaryCaseSetExact -or
        -not [bool]$greenIndex.proof.correctedCanaryAssertionManifestExact -or
        -not [bool]$greenIndex.proof.correctedCanaryWarningContractExact -or
        -not [bool]$greenIndex.proof.correctedCanaryBlockedContractExact -or
        -not [bool]$greenIndex.proof.correctedCanaryStateDiffManifestExact -or
        -not [bool]$greenIndex.proof.correctedCanaryOrphanContractExact -or
        -not [bool]$greenIndex.proof.correctedCanaryAssertionSkipFree -or
        -not [bool]$greenIndex.proof.correctedCanaryProofAxisPassed -or
        [int]$greenIndex.diagnostics.hardDiagnosticCount -ne 2 -or
        [int]$greenIndex.diagnostics.approvedStockDiagnosticCount -ne 2 -or
        [int]$greenIndex.diagnostics.approvedIntentionalDiagnosticCount -ne 0 -or
        [int]$greenIndex.diagnostics.unapprovedHardDiagnosticCount -ne 0 -or
        [int]$greenIndex.diagnostics.canonicalErrorLogCount -ne 1 -or
        [int]$greenIndex.diagnostics.canonicalCrashLogCount -ne 1 -or
        -not [bool]$greenIndex.diagnostics.auxiliaryDiagnosticsValid -or
        -not [bool]$greenIndex.diagnostics.errorLogProjectionExact -or
        -not [bool]$greenIndex.diagnostics.crashLogProjectionExact -or
        [int]$greenIndex.diagnostics.auxiliaryUnapprovedEventCount -ne 0 -or
        [int]$greenIndex.diagnostics.classifierSelfTestCount -ne 38 -or
        [int]$greenIndex.integrity.envelopeFileCount -ne 10 -or
        -not [bool]$greenIndex.integrity.envelopeFilesRehashed -or
        [string]$greenReceipt.Status -cne 'passed-noncertifying') {
        throw 'The portable corrected-canary green contract self-test failed.'
    }
    $warningIds = @($greenIndex.proof.warningAssertionIds)
    $warningRows = @($greenIndex.proof.warningAssertions)
    $blockedRows = @($greenIndex.proof.blockedAssertions)
    $blockedIds = @($blockedRows | ForEach-Object { $_.id })
    if ($warningIds.Count -ne 1 -or
        [string]$warningIds[0] -cne 'cleanup.player_marker.live' -or
        $warningRows.Count -ne 1 -or
        [string]$warningRows[0].caseId -cne 'cleanup.player_marker_completion' -or
        $blockedIds.Count -ne 1 -or
        [string]$blockedIds[0] -cne 'isolation.world_scope' -or
        $blockedRows.Count -ne 1 -or
        [string]$blockedRows[0].caseId -cne
            'cleanup.state_isolation_restore' -or
        [string]$blockedRows[0].category -cne 'cleanup' -or
        [string]$blockedRows[0].feature -cne 'campaign_debug' -or
        [string]$blockedRows[0].stage -cne 'state_restore' -or
        [string]$blockedRows[0].proofLevel -cne 'EXTERNAL_PROCESS' -or
        [string]$blockedRows[0].observedPath -cne 'manual_external_gap' -or
        [string]$blockedRows[0].requiredPath -cne
            'external process restart, reconnect, or long-soak harness' -or
        [bool]$blockedRows[0].countsTowardCertification) {
        throw 'The portable corrected-canary advisory identity self-test failed.'
    }
    $greenJsonText = Get-Content -Raw -LiteralPath $green.IndexPath
    if ($greenJsonText -match '(?i)[a-z]:[\\/]' -or
        $greenJsonText -match '(?i)file:(?:/+|\\+)') {
        throw 'The portable corrected-canary index leaked a local absolute path.'
    }
    $syntheticDrivePath = ([char]67) + ':/synthetic-root/item'
    $urlWrappedPathCases = @(
        [pscustomobject]@{
            Name = 'URL-wrapped drive path'
            Value = 'https://example.invalid/evidence/' + $syntheticDrivePath
        },
        [pscustomobject]@{
            Name = 'URL-wrapped UNC path'
            Value = 'https://example.invalid//synthetic-host/share/item'
        },
        [pscustomobject]@{
            Name = 'URL-wrapped file URI'
            Value = 'https://example.invalid/redirect?target=file:///' +
                $syntheticDrivePath
        })
    foreach ($pathCase in $urlWrappedPathCases) {
        $pathFixture = New-CorrectedCanaryFixture `
            -FixtureRoot (Join-Path $tempParent (
                'path-' + $pathCase.Name.Replace(' ', '-').ToLowerInvariant()))
        $pathRun = Get-Content -Raw -LiteralPath $pathFixture.RunPath |
            ConvertFrom-Json
        $pathRun.outcome.error = [string]$pathCase.Value
        Write-Json $pathFixture.RunPath $pathRun
        Assert-ProducerRejected $pathCase.Name {
            [void](Invoke-Producer $pathFixture)
        }
    }

    $packageInventoryTamper = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'package-inventory-tamper')
    $packageManifestPath = Join-Path `
        $packageInventoryTamper.Bundle 'identity/candidate.json'
    $packageReadyPath = Join-Path `
        $packageInventoryTamper.Bundle 'identity/candidate.ready.json'
    $packageManifest = Get-Content -Raw -LiteralPath $packageManifestPath |
        ConvertFrom-Json
    $packageManifest.package.files[0].length =
        [long]$packageManifest.package.files[0].length + 1
    Write-Json $packageManifestPath $packageManifest
    $packageManifestSha = (Get-FileHash `
        -LiteralPath $packageManifestPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    $packageReady = Get-Content -Raw -LiteralPath $packageReadyPath |
        ConvertFrom-Json
    $packageReady.manifestSha256 = $packageManifestSha
    Write-Json $packageReadyPath $packageReady
    $packageReadySha = (Get-FileHash `
        -LiteralPath $packageReadyPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    $packageTamperRun = Get-Content -Raw `
        -LiteralPath $packageInventoryTamper.RunPath | ConvertFrom-Json
    $packageTamperRun.candidate.manifestSha256 = $packageManifestSha
    $packageTamperRun.candidate.readySha256 = $packageReadySha
    foreach ($sealedRow in @($packageTamperRun.files)) {
        $sealedPath = Join-Path `
            $packageInventoryTamper.Bundle `
            ([string]$sealedRow.path).Replace('/', '\')
        if ([string]$sealedRow.path -cin @(
                'identity/candidate.json', 'identity/candidate.ready.json')) {
            $sealedItem = Get-Item -LiteralPath $sealedPath
            $sealedRow.length = [long]$sealedItem.Length
            $sealedRow.sha256 = (Get-FileHash `
                -LiteralPath $sealedPath `
                -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }
    Write-Json $packageInventoryTamper.RunPath $packageTamperRun
    Assert-ProducerRejected 'canonical package inventory digest tamper' {
        [void](Invoke-Producer $packageInventoryTamper)
    }
    if (Test-Path -LiteralPath $packageInventoryTamper.IndexPath) {
        throw 'The canonical package inventory tamper published durable evidence.'
    }

    $greenIndexSha = (Get-FileHash -LiteralPath $green.IndexPath -Algorithm SHA256).Hash
    $greenRepeatReceipt = Invoke-Producer $green
    $greenRepeatSha = (Get-FileHash -LiteralPath $green.IndexPath -Algorithm SHA256).Hash
    if ($greenRepeatSha -cne $greenIndexSha -or
        [string]$greenRepeatReceipt.ReleaseIndexSha256 -cne
            $greenIndexSha.ToLowerInvariant()) {
        throw 'The portable corrected-canary byte-identical reuse self-test failed.'
    }

    $immutableConflict = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'immutable-conflict')
    [void](Invoke-Producer $immutableConflict)
    [IO.File]::AppendAllText(
        $immutableConflict.IndexPath,
        " `n",
        (New-Object Text.UTF8Encoding($false)))
    $immutableConflictSha = (Get-FileHash `
        -LiteralPath $immutableConflict.IndexPath -Algorithm SHA256).Hash
    Assert-ProducerRejected 'immutable release-index replacement' {
        [void](Invoke-Producer $immutableConflict)
    }
    $immutableConflictShaAfter = (Get-FileHash `
        -LiteralPath $immutableConflict.IndexPath -Algorithm SHA256).Hash
    if ($immutableConflictShaAfter -cne $immutableConflictSha) {
        throw 'The portable corrected-canary immutable-conflict bytes changed after rejection.'
    }

    $lateDrift = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'late-drift')
    $lateDriftRun = Get-Content -Raw -LiteralPath $lateDrift.RunPath |
        ConvertFrom-Json
    $lateDriftRelative = [string](@($lateDriftRun.files | Where-Object {
        [string]$_.path -cmatch '/script\.log$'
    })[0].path)
    $lateDriftPath = Join-Path `
        $lateDrift.Bundle `
        $lateDriftRelative.Replace('/', '\')
    $lateDriftToken = [Guid]::NewGuid().ToString('N')
    $lateReady = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexReady-' + $lateDriftToken))
    $lateMutated = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexMutated-' + $lateDriftToken))
    $lateChild = $null
    try {
        $lateChild = Start-CorrectedCanaryProducerProcess `
            -ProducerPath $producerPath `
            -RunPath $lateDrift.RunPath `
            -TempParent $tempParent `
            -Token $lateDriftToken `
            -Mode 'late-drift'
        Wait-CorrectedCanaryProducerProcessBarrier `
            -Barrier $lateReady `
            -Child $lateChild `
            -Label 'The corrected-canary late-drift producer publication barrier'
        [IO.File]::AppendAllText(
            $lateDriftPath,
            "late publication mutation`n",
            (New-Object Text.UTF8Encoding($false)))
        [void]$lateMutated.Set()
        $lateOutcome = Complete-CorrectedCanaryProducerProcess `
            -Child $lateChild `
            -Label 'The corrected-canary late-drift producer'
        if ([bool]$lateOutcome.Receipt.Succeeded -or
            [string]$lateOutcome.Receipt.Error -cnotmatch
                'changed immediately before release-index publication' -or
            (Test-Path -LiteralPath $lateDrift.IndexPath)) {
            throw 'The corrected-canary late-drift publication self-test failed closed.'
        }
    }
    finally {
        [void]$lateMutated.Set()
        try {
            if ($lateChild) {
                Stop-CorrectedCanaryProducerProcess `
                    -Child $lateChild `
                    -Label 'The corrected-canary late-drift producer'
            }
        }
        finally {
            $lateReady.Dispose()
            $lateMutated.Dispose()
        }
    }

    $publicationWindow = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'publication-window')
    $publicationRun = Get-Content -Raw -LiteralPath $publicationWindow.RunPath |
        ConvertFrom-Json
    $publicationRelative = [string](@($publicationRun.files | Where-Object {
        [string]$_.path -cmatch '/script\.log$'
    })[0].path)
    $publicationInputPath = Join-Path `
        $publicationWindow.Bundle `
        $publicationRelative.Replace('/', '\')
    $publicationInputSha = (Get-FileHash `
        -LiteralPath $publicationInputPath `
        -Algorithm SHA256).Hash
    $publicationToken = [Guid]::NewGuid().ToString('N')
    $publicationReady = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexPublicationReady-' +
            $publicationToken))
    $publicationAttempted = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexPublicationAttempted-' +
            $publicationToken))
    $publicationChild = $null
    $publicationMutationRejected = $false
    try {
        $publicationChild = Start-CorrectedCanaryProducerProcess `
            -ProducerPath $producerPath `
            -RunPath $publicationWindow.RunPath `
            -TempParent $tempParent `
            -Token $publicationToken `
            -Mode 'publication-window'
        Wait-CorrectedCanaryProducerProcessBarrier `
            -Barrier $publicationReady `
            -Child $publicationChild `
            -Label 'The corrected-canary producer publication window'
        if (-not (Test-Path -LiteralPath $publicationWindow.IndexPath -PathType Leaf)) {
            throw 'The corrected-canary publication seam opened before immutable output existed.'
        }
        try {
            [IO.File]::AppendAllText(
                $publicationInputPath,
                "forbidden publication-window mutation`n",
                (New-Object Text.UTF8Encoding($false)))
        }
        catch [IO.IOException] {
            $publicationMutationRejected = $true
        }
        catch [UnauthorizedAccessException] {
            $publicationMutationRejected = $true
        }
        [void]$publicationAttempted.Set()
        $publicationOutcome = Complete-CorrectedCanaryProducerProcess `
            -Child $publicationChild `
            -Label 'The corrected-canary producer publication window'
        $publicationInputShaAfter = (Get-FileHash `
            -LiteralPath $publicationInputPath `
            -Algorithm SHA256).Hash
        if (-not $publicationMutationRejected -or
            -not [bool]$publicationOutcome.Receipt.Succeeded -or
            -not [string]::IsNullOrEmpty(
                [string]$publicationOutcome.Receipt.Error) -or
            $publicationInputShaAfter -cne $publicationInputSha -or
            -not (Test-Path -LiteralPath $publicationWindow.IndexPath -PathType Leaf)) {
            throw 'The corrected-canary publication-window input-lock self-test failed.'
        }
    }
    finally {
        [void]$publicationAttempted.Set()
        try {
            if ($publicationChild) {
                Stop-CorrectedCanaryProducerProcess `
                    -Child $publicationChild `
                    -Label 'The corrected-canary producer publication window'
            }
        }
        finally {
            $publicationReady.Dispose()
            $publicationAttempted.Dispose()
        }
    }

    $concurrentReuse = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'concurrent-byte-identical-reuse')
    $concurrentToken = [Guid]::NewGuid().ToString('N')
    $concurrentReady = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexConcurrentReady-' +
            $concurrentToken))
    $concurrentPublished = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexConcurrentPublished-' +
            $concurrentToken))
    $reusePublicationReady = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexPublicationReady-' +
            $concurrentToken))
    $reusePublicationAttempted = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexPublicationAttempted-' +
            $concurrentToken))
    $concurrentChild = $null
    $concurrentExtraPath = Join-Path `
        $concurrentReuse.Bundle `
        'raw/concurrent-publication-extra.txt'
    try {
        $concurrentChild = Start-CorrectedCanaryProducerProcess `
            -ProducerPath $producerPath `
            -RunPath $concurrentReuse.RunPath `
            -TempParent $tempParent `
            -Token $concurrentToken `
            -Mode 'concurrent-reuse'
        Wait-CorrectedCanaryProducerProcessBarrier `
            -Barrier $concurrentReady `
            -Child $concurrentChild `
            -Label 'The corrected-canary producer concurrent publication barrier'
        $concurrentTemporaryRows = @(Get-ChildItem `
            -LiteralPath $concurrentReuse.Bundle `
            -File `
            -Force | Where-Object {
                $_.Name -cmatch '^\.release-index\.json\.[0-9a-f]{32}\.tmp$'
            })
        if ($concurrentTemporaryRows.Count -ne 1 -or
            (Test-Path -LiteralPath $concurrentReuse.IndexPath)) {
            throw 'The concurrent publication fixture did not expose one unpublished exact-byte temporary index.'
        }
        [IO.File]::Copy(
            $concurrentTemporaryRows[0].FullName,
            $concurrentReuse.IndexPath,
            $false)
        $concurrentWinnerSha = (Get-FileHash `
            -LiteralPath $concurrentReuse.IndexPath `
            -Algorithm SHA256).Hash
        [void]$concurrentPublished.Set()
        Wait-CorrectedCanaryProducerProcessBarrier `
            -Barrier $reusePublicationReady `
            -Child $concurrentChild `
            -Label 'The corrected-canary concurrent reuser publication window'
        Write-Utf8Text `
            -Path $concurrentExtraPath `
            -Text "synthetic concurrent publication drift`n"
        [void]$reusePublicationAttempted.Set()
        $concurrentOutcome = Complete-CorrectedCanaryProducerProcess `
            -Child $concurrentChild `
            -Label 'The corrected-canary concurrent reuser'
        $concurrentWinnerShaAfter = (Get-FileHash `
            -LiteralPath $concurrentReuse.IndexPath `
            -Algorithm SHA256).Hash
        if ([bool]$concurrentOutcome.Receipt.Succeeded -or
            [string]$concurrentOutcome.Receipt.Error -cnotmatch
                'Run-envelope inventory and retained raw file set after publication' -or
            $concurrentWinnerShaAfter -cne $concurrentWinnerSha -or
            -not (Test-Path -LiteralPath $concurrentReuse.IndexPath -PathType Leaf)) {
            throw 'The concurrent byte-identical publication rollback self-test deleted or changed the winning index.'
        }
    }
    finally {
        [void]$concurrentPublished.Set()
        [void]$reusePublicationAttempted.Set()
        try {
            if ($concurrentChild) {
                Stop-CorrectedCanaryProducerProcess `
                    -Child $concurrentChild `
                    -Label 'The corrected-canary concurrent reuser'
            }
        }
        finally {
            try {
                if (Test-Path -LiteralPath $concurrentExtraPath -PathType Leaf) {
                    Remove-Item -LiteralPath $concurrentExtraPath -Force
                }
            }
            finally {
                $concurrentReady.Dispose()
                $concurrentPublished.Dispose()
                $reusePublicationReady.Dispose()
                $reusePublicationAttempted.Dispose()
            }
        }
    }

    $proofRed = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'proof-red') `
        -Mode 'proof-red'
    [void](Invoke-Producer $proofRed)
    $proofRedIndex = Get-Content -Raw -LiteralPath $proofRed.IndexPath | ConvertFrom-Json
    if ([string]$proofRedIndex.result.status -cne 'failed-corrected-canary' -or
        [string]$proofRedIndex.result.acceptanceDisposition -cne
            'rejected-corrected-canary' -or
        [string]$proofRedIndex.result.releaseDisposition -cne
            'replacement-required') {
        throw 'The portable corrected-canary proof-red disposition self-test failed.'
    }

    $warningRed = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'warning-red') `
        -Mode 'warning-red'
    [void](Invoke-Producer $warningRed)
    $warningRedIndex = Get-Content -Raw -LiteralPath $warningRed.IndexPath | ConvertFrom-Json
    if ([string]$warningRedIndex.result.status -cne 'failed-corrected-canary' -or
        [bool]$warningRedIndex.proof.correctedCanaryWarningContractExact) {
        throw 'The portable corrected-canary warning-red disposition self-test failed.'
    }

    $blockedParentRed = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'blocked-parent-red') `
        -Mode 'blocked-parent-red'
    [void](Invoke-Producer $blockedParentRed)
    $blockedParentRedIndex = Get-Content -Raw `
        -LiteralPath $blockedParentRed.IndexPath | ConvertFrom-Json
    if ([string]$blockedParentRedIndex.result.status -cne
            'failed-corrected-canary' -or
        [bool]$blockedParentRedIndex.proof.correctedCanaryBlockedContractExact -or
        [int]$blockedParentRedIndex.proof.warn -ne 1 -or
        [int]$blockedParentRedIndex.proof.blocked -ne 1) {
        throw 'The portable corrected-canary blocked-parent disposition self-test failed.'
    }

    $hiddenSkip = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'hidden-skip') `
        -Mode 'hidden-skip'
    [void](Invoke-Producer $hiddenSkip)
    $hiddenSkipIndex = Get-Content -Raw -LiteralPath $hiddenSkip.IndexPath |
        ConvertFrom-Json
    if ([string]$hiddenSkipIndex.result.status -cne 'failed-corrected-canary' -or
        [bool]$hiddenSkipIndex.proof.correctedCanaryAssertionSkipFree -or
        @($hiddenSkipIndex.proof.skippedAssertionIds) -cnotcontains
            'cleanup.player_marker.hidden_skip') {
        throw 'The portable corrected-canary hidden-skip rejection self-test failed.'
    }

    $focusedCertificationSwap = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'focused-certification-swap') `
        -Mode 'focused-certification-swap'
    [void](Invoke-Producer $focusedCertificationSwap)
    $focusedCertificationSwapIndex = Get-Content -Raw `
        -LiteralPath $focusedCertificationSwap.IndexPath | ConvertFrom-Json
    if ([string]$focusedCertificationSwapIndex.result.status -cne
            'failed-corrected-canary' -or
        [bool]$focusedCertificationSwapIndex.proof.focusedAssertionsCertificationExact -or
        [int]$focusedCertificationSwapIndex.proof.focusedAssertionsPassed -ne 35 -or
        [int]$focusedCertificationSwapIndex.proof.certificationRequired -ne 87 -or
        [int]$focusedCertificationSwapIndex.proof.certificationProven -ne 87) {
        throw 'The portable corrected-canary focused certification-swap self-test failed.'
    }

    $diagnosticRed = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'diagnostic-red') `
        -Mode 'diagnostic-red'
    [void](Invoke-Producer $diagnosticRed)
    $diagnosticRedIndex = Get-Content -Raw -LiteralPath $diagnosticRed.IndexPath |
        ConvertFrom-Json
    if ([string]$diagnosticRedIndex.result.status -cne 'failed-corrected-canary' -or
        [string]$diagnosticRedIndex.result.acceptanceDisposition -cne
            'rejected-corrected-canary' -or
        [string]$diagnosticRedIndex.result.releaseDisposition -cne
            'replacement-required' -or
        [int]$diagnosticRedIndex.diagnostics.unapprovedHardDiagnosticCount -ne 1) {
        throw 'The portable corrected-canary diagnostic-red disposition self-test failed.'
    }

    $tableDrivenRedContracts = @(
        [pscustomobject]@{ Mode = 'unexpected-blocker'; Section = 'proof'; Field = 'correctedCanaryBlockedContractExact' },
        [pscustomobject]@{ Mode = 'certifying-blocker'; Section = 'proof'; Field = 'correctedCanaryBlockedContractExact' },
        [pscustomobject]@{ Mode = 'balanced-certification-swap'; Section = 'proof'; Field = 'correctedCanaryAssertionManifestExact' },
        [pscustomobject]@{ Mode = 'nonfocused-id-substitution'; Section = 'proof'; Field = 'correctedCanaryAssertionManifestExact' },
        [pscustomobject]@{ Mode = 'diff-missing'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'diff-duplicate'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'diff-renamed'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'diff-order'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'diff-arithmetic'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'diff-nonnumeric'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'diff-nonzero'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'diff-extra-line'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'orphan-id-red'; Section = 'proof'; Field = 'correctedCanaryOrphanContractExact' },
        [pscustomobject]@{ Mode = 'orphan-metric-id-red'; Section = 'proof'; Field = 'correctedCanaryOrphanContractExact' },
        [pscustomobject]@{ Mode = 'orphan-metric-case-red'; Section = 'proof'; Field = 'correctedCanaryOrphanContractExact' },
        [pscustomobject]@{ Mode = 'orphan-metric-name-red'; Section = 'proof'; Field = 'correctedCanaryOrphanContractExact' },
        [pscustomobject]@{ Mode = 'orphan-metadata-red'; Section = 'proof'; Field = 'correctedCanaryOrphanContractExact' },
        [pscustomobject]@{ Mode = 'orphan-actual-red'; Section = 'proof'; Field = 'correctedCanaryOrphanContractExact' },
        [pscustomobject]@{ Mode = 'orphan-certification-red'; Section = 'proof'; Field = 'correctedCanaryOrphanContractExact' },
        [pscustomobject]@{ Mode = 'error-log-red'; Section = 'diagnostics'; Field = 'auxiliaryDiagnosticsValid' },
        [pscustomobject]@{ Mode = 'crash-log-red'; Section = 'diagnostics'; Field = 'auxiliaryDiagnosticsValid' })
    foreach ($contract in $tableDrivenRedContracts) {
        $fixture = New-CorrectedCanaryFixture `
            -FixtureRoot (Join-Path $tempParent ("table-" + $contract.Mode)) `
            -Mode $contract.Mode
        [void](Invoke-Producer $fixture)
        $redIndex = Get-Content -Raw -LiteralPath $fixture.IndexPath |
            ConvertFrom-Json
        if ([string]$redIndex.result.status -cne 'failed-corrected-canary' -or
            [bool]$redIndex.($contract.Section).($contract.Field)) {
            throw "The portable corrected-canary $($contract.Mode) table contract self-test failed."
        }
        $missingOrphanCount = $contract.Mode -cin @(
            'orphan-metric-id-red',
            'orphan-metric-case-red')
        $expectedOrphanCount = if ($missingOrphanCount) { -1 } else { 0 }
        if ([int]$redIndex.proof.finalOrphanActiveGroups -ne
                $expectedOrphanCount -or
            ($missingOrphanCount -and
                [bool]$redIndex.proof.finalOrphanCleanupPass)) {
            throw "The portable corrected-canary $($contract.Mode) missing-orphan-count sentinel is invalid."
        }
    }

    $harnessTamper = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'harness-tamper')
    $harnessTamperRun = Get-Content -Raw -LiteralPath $harnessTamper.RunPath |
        ConvertFrom-Json
    $harnessTamperRun.harness.campaignRunnerSha256 = 'f' * 64
    Write-Json $harnessTamper.RunPath $harnessTamperRun
    Assert-ProducerRejected 'harness tool hash tamper' {
        [void](Invoke-Producer $harnessTamper)
    }

    $headTamper = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'head-tamper')
    $headTamperRun = Get-Content -Raw -LiteralPath $headTamper.RunPath |
        ConvertFrom-Json
    $headTamperRun.harness.gitHead = 'b' * 40
    Write-Json $headTamper.RunPath $headTamperRun
    Assert-ProducerRejected 'harness Git HEAD tamper' {
        [void](Invoke-Producer $headTamper)
    }

    foreach ($blobField in @(
            'campaignRunnerGitBlobSha256',
            'candidateModuleGitBlobSha256',
            'releaseIndexProducerGitBlobSha256',
            'releaseDocsConsumerGitBlobSha256')) {
        $blobTamper = New-CorrectedCanaryFixture `
            -FixtureRoot (Join-Path $tempParent "blob-tamper-$blobField")
        $blobTamperRun = Get-Content -Raw -LiteralPath $blobTamper.RunPath |
            ConvertFrom-Json
        $blobTamperRun.harness.($blobField) = 'f' * 64
        Write-Json $blobTamper.RunPath $blobTamperRun
        Assert-ProducerRejected "harness $blobField tamper" {
            [void](Invoke-Producer $blobTamper)
        }
    }

    $candidateTamper = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'candidate-tamper')
    $candidateTamperRun = Get-Content -Raw -LiteralPath $candidateTamper.RunPath |
        ConvertFrom-Json
    $candidateTamperRun.launch.packageSha256 = 'f' * 64
    Write-Json $candidateTamper.RunPath $candidateTamperRun
    Assert-ProducerRejected 'candidate package identity tamper' {
        [void](Invoke-Producer $candidateTamper)
    }

    $inventoryTamper = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'inventory-tamper')
    [IO.File]::AppendAllText(
        $inventoryTamper.RawPath,
        " `n",
        (New-Object Text.UTF8Encoding($false)))
    Assert-ProducerRejected 'raw artifact hash tamper' {
        [void](Invoke-Producer $inventoryTamper)
    }

    $duplicateRunKey = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'duplicate-run-key')
    $duplicateRunText = Get-Content -Raw -LiteralPath $duplicateRunKey.RunPath
    $duplicateRunMatch = [regex]::Match(
        $duplicateRunText,
        '(?m)^(?<row>\s*"schemaVersion"\s*:\s*2\s*,\r?\n)')
    if (-not $duplicateRunMatch.Success) {
        throw 'The duplicate run-envelope key fixture could not find schemaVersion.'
    }
    $duplicateRunText = $duplicateRunText.Insert(
        $duplicateRunMatch.Index + $duplicateRunMatch.Length,
        $duplicateRunMatch.Groups['row'].Value)
    Write-Utf8Text -Path $duplicateRunKey.RunPath -Text $duplicateRunText
    Assert-ProducerRejected 'duplicate run-envelope JSON key' {
        [void](Invoke-Producer $duplicateRunKey)
    }

    $duplicateRawKey = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'duplicate-raw-key')
    $duplicateRawText = Get-Content -Raw -LiteralPath $duplicateRawKey.RawPath
    $duplicateRawMatch = [regex]::Match(
        $duplicateRawText,
        '(?m)^(?<row>\s*"m_sWorldName"\s*:\s*"HST_Dev"\s*,\r?\n)')
    if (-not $duplicateRawMatch.Success) {
        throw 'The duplicate raw-artifact key fixture could not find m_sWorldName.'
    }
    $duplicateRawText = $duplicateRawText.Insert(
        $duplicateRawMatch.Index + $duplicateRawMatch.Length,
        $duplicateRawMatch.Groups['row'].Value)
    Write-Utf8Text -Path $duplicateRawKey.RawPath -Text $duplicateRawText
    $duplicateRawRun = Get-Content -Raw -LiteralPath $duplicateRawKey.RunPath |
        ConvertFrom-Json
    $duplicateRawRelativePath = $duplicateRawKey.RawPath.Substring(
        $duplicateRawKey.Bundle.Length + 1).Replace('\', '/')
    $duplicateRawInventoryRows = @($duplicateRawRun.files | Where-Object {
        [string]$_.path -ceq $duplicateRawRelativePath
    })
    if ($duplicateRawInventoryRows.Count -ne 1) {
        throw 'The duplicate raw-artifact key fixture lacks one inventory row.'
    }
    $duplicateRawItem = Get-Item -LiteralPath $duplicateRawKey.RawPath
    $duplicateRawInventoryRows[0].length = [long]$duplicateRawItem.Length
    $duplicateRawInventoryRows[0].sha256 = (Get-FileHash `
        -LiteralPath $duplicateRawKey.RawPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    Write-Json $duplicateRawKey.RunPath $duplicateRawRun
    Assert-ProducerRejected 'duplicate raw-artifact JSON key' {
        [void](Invoke-Producer $duplicateRawKey)
    }

    $dirtyHarness = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'dirty-harness')
    $dirtyHarnessRun = Get-Content -Raw -LiteralPath $dirtyHarness.RunPath |
        ConvertFrom-Json
    $dirtyHarnessRun.harness.dirty = $true
    Write-Json $dirtyHarness.RunPath $dirtyHarnessRun
    Assert-ProducerRejected 'dirty harness identity' {
        [void](Invoke-Producer $dirtyHarness)
    }

    $orphanTamper = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'recorded-orphan-tamper')
    $orphanTamperRun = Get-Content -Raw -LiteralPath $orphanTamper.RunPath |
        ConvertFrom-Json
    $orphanTamperRun.outcome.validation.FinalOrphanActiveGroups = [long]1
    Write-Json $orphanTamper.RunPath $orphanTamperRun
    Assert-ProducerRejected `
        -Label 'recorded final-orphan tamper' `
        -ExpectedMessage 'The recorded final-orphan proof differs from retained raw evidence.' `
        -Action { [void](Invoke-Producer $orphanTamper) }

    $timestampTamper = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'timestamp-runtime-tamper')
    $timestampTamperRun = Get-Content -Raw -LiteralPath $timestampTamper.RunPath |
        ConvertFrom-Json
    $timestampTamperRun.outcome.runtimeSeconds = 11
    Write-Json $timestampTamper.RunPath $timestampTamperRun
    Assert-ProducerRejected 'corrected-canary timestamp/runtime tamper' {
        [void](Invoke-Producer $timestampTamper)
    }

    [pscustomobject][ordered]@{
        status = 'passed'
        schemaVersion = 2
        evidenceKind = 'packaged-campaign-debug-corrected-canary'
        policyId = 'partisan-campaign-debug-corrected-canary-v2'
        greenStatus = [string]$greenIndex.result.status
        redStatus = [string]$proofRedIndex.result.status
        caseCensus = '11/9/1/0/1/0'
        focused = '35/35'
        certification = '87/87/0/0/0/false'
        stateDiff = '18/0'
        orphan = 'true/0'
        diagnostics = '2/2/0/0'
        negativeCanaryDispositionChecks = 6
        idempotentPublicationChecks = 1
        immutableConflictChecks = 1
        lateDriftPublicationChecks = 1
        publicationWindowLockChecks = 1
        concurrentPublicationRollbackChecks = 1
        localPathNegativeChecks = $urlWrappedPathCases.Count
        packageInventoryNegativeChecks = 1
        stateDiffNegativeChecks = 8
        failClosedChecks = $tableDrivenRedContracts.Count
    }
}
finally {
    $resolvedTempParent = [IO.Path]::GetFullPath($tempParent)
    if ($resolvedTempParent.StartsWith(
            $expectedTempPrefix,
            [StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path -Leaf $resolvedTempParent) -like 'PartisanCorrectedCanaryV2-*' -and
        (Test-Path -LiteralPath $resolvedTempParent -PathType Container)) {
        Remove-Item -LiteralPath $resolvedTempParent -Recurse -Force
    }
}
