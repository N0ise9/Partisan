[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$ManifestPath,
    [Parameter(Mandatory = $true)][string]$BundleRoot,
    [Parameter(Mandatory = $true)][string]$RuntimeAddonRoot,
    [Parameter(Mandatory = $true)][string]$ServerDiagnosticExecutable,
    [Parameter(Mandatory = $true)][string]$ClientDiagnosticExecutable,
    [Parameter(Mandatory = $true)][string]$ClientExecutable,
    [Parameter(Mandatory = $true)][string]$EvidenceRoot,
    [Parameter(Mandatory = $true)][string[]]$WatchedRoots,
    [Parameter(Mandatory = $true)][string[]]$SpillRoots,
    [ValidateRange(30, 3600)][int]$StageTimeoutSeconds = 600,
    [ValidateRange(100, 5000)][int]$PollMilliseconds = 500,
    [ValidateRange(1, 60)][int]$ResultGraceSeconds = 5,
    [ValidateRange(1, 65535)][int]$LoopbackPort = 2001,
    [ValidateRange(5, 300)][int]$StandardReadinessSeconds = 20,
    [switch]$LibraryOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:GateContractId = 'partisan.gate1-runtime-retention.v1'
$script:GateEvidenceKind = 'packaged-gate1-runtime-retention'
$script:GateStages = @(
    'autosave_checkpoint',
    'manual_checkpoint',
    'shutdown_checkpoint',
    'native_shutdown_verify',
    'profile_fallback_verify')
$script:StageDirectories = @(
    '00-autosave_checkpoint',
    '01-manual_checkpoint',
    '02-shutdown_checkpoint',
    '03-native_shutdown_verify',
    '04-profile_fallback_verify')

function Get-GateFileSignature {
    param([Parameter(Mandatory = $true)][string]$Path)
    $item = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    if ($item.PSIsContainer) { throw 'A retained signature target is a directory.' }
    return [pscustomobject][ordered]@{
        length = [long]$item.Length
        sha256 = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).
            Hash.ToLowerInvariant()
    }
}

function Get-GateSha256Text {
    param([Parameter(Mandatory = $true)][AllowEmptyString()][string]$Text)
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = (New-Object Text.UTF8Encoding($false)).GetBytes($Text)
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally { $algorithm.Dispose() }
}

function Write-GateJson {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value,
        [switch]$Atomic,
        [switch]$CreateOnly
    )
    $full = [IO.Path]::GetFullPath($Path)
    if ($CreateOnly -and (Test-Path -LiteralPath $full)) {
        throw 'A create-only retained JSON path already exists.'
    }
    $parent = Split-Path -Parent $full
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        New-Item -ItemType Directory -Path $parent -Force -ErrorAction Stop |
            Out-Null
    }
    $json = ($Value | ConvertTo-Json -Depth 64) + "`n"
    $bytes = (New-Object Text.UTF8Encoding($false)).GetBytes($json)
    if ($Atomic) {
        $temporary = $full + '.tmp.' + [Guid]::NewGuid().ToString('N')
        try {
            $stream = [IO.FileStream]::new(
                $temporary,
                [IO.FileMode]::CreateNew,
                [IO.FileAccess]::Write,
                [IO.FileShare]::None)
            try {
                $stream.Write($bytes, 0, $bytes.Length)
                $stream.Flush($true)
            }
            finally {
                $stream.Dispose()
            }
            if ($CreateOnly) {
                [IO.File]::Move($temporary, $full)
            }
            else {
                [IO.File]::Replace($temporary, $full, $null)
            }
        }
        finally {
            if (Test-Path -LiteralPath $temporary) {
                Remove-Item -LiteralPath $temporary -Force `
                    -ErrorAction SilentlyContinue
            }
        }
    }
    else { [IO.File]::WriteAllBytes($full, $bytes) }
    return Get-GateFileSignature $full
}

function Copy-GateStableFile {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )
    $sourcePath = [IO.Path]::GetFullPath($Source)
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        throw "Retained source is missing: $(Split-Path -Leaf $sourcePath)."
    }
    Assert-PartisanNoReparseAncestry -Path $sourcePath
    $before = Get-GateFileSignature $sourcePath
    $destinationPath = [IO.Path]::GetFullPath($Destination)
    if (Test-Path -LiteralPath $destinationPath) {
        throw 'A stable retained copy destination is not fresh.'
    }
    $parent = Split-Path -Parent $destinationPath
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    Copy-Item -LiteralPath $sourcePath -Destination $destinationPath `
        -ErrorAction Stop
    $after = Get-GateFileSignature $sourcePath
    $copy = Get-GateFileSignature $destinationPath
    if ($before.length -ne $after.length -or
        $before.sha256 -cne $after.sha256 -or
        $before.length -ne $copy.length -or
        $before.sha256 -cne $copy.sha256) {
        throw 'A retained file changed while it was copied.'
    }
    return $copy
}

function ConvertTo-GatePortablePath {
    param([string]$Root, [string]$Path)
    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $full = [IO.Path]::GetFullPath($Path)
    $prefix = $rootPath + [IO.Path]::DirectorySeparatorChar
    if (-not $full.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'A retained path escaped the run root.'
    }
    $relative = $full.Substring($prefix.Length).Replace('\', '/')
    if ([string]::IsNullOrWhiteSpace($relative) -or
        $relative.Contains(':') -or $relative.Split('/') -contains '..') {
        throw 'A retained path is not portable.'
    }
    return $relative
}

function Test-GatePathOverlap {
    param([string]$First, [string]$Second)

    $firstPath = [IO.Path]::GetFullPath($First).TrimEnd('\', '/')
    $secondPath = [IO.Path]::GetFullPath($Second).TrimEnd('\', '/')
    if ($firstPath.Equals($secondPath, [StringComparison]::OrdinalIgnoreCase)) {
        return $true
    }
    return $firstPath.StartsWith(
            $secondPath + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase) -or
        $secondPath.StartsWith(
            $firstPath + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)
}

function Assert-GateEvidenceRootSafe {
    param(
        [string]$EvidenceRoot,
        [string[]]$ProtectedRoots,
        [string[]]$WatchedRoots,
        [string[]]$SpillRoots
    )

    if ([string]::IsNullOrWhiteSpace($EvidenceRoot)) {
        throw 'Gate 1 requires an explicit external evidence root.'
    }
    $evidencePath = [IO.Path]::GetFullPath($EvidenceRoot).TrimEnd('\', '/')
    $volumeRoot = [IO.Path]::GetPathRoot($evidencePath).TrimEnd('\', '/')
    if ($evidencePath.Equals(
            $volumeRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'The Gate 1 evidence root must not be a volume root.'
    }
    if ($evidencePath.Contains(',')) {
        throw 'The Gate 1 evidence root must not contain a comma.'
    }
    if ((Test-Path -LiteralPath $evidencePath) -and
        -not (Test-Path -LiteralPath $evidencePath -PathType Container)) {
        throw 'The Gate 1 evidence root is not a directory.'
    }
    $protected = @($ProtectedRoots | Where-Object {
            -not [string]::IsNullOrWhiteSpace([string]$_)
        })
    $watched = @($WatchedRoots | Where-Object {
            -not [string]::IsNullOrWhiteSpace([string]$_)
        })
    $spill = @($SpillRoots | Where-Object {
            -not [string]::IsNullOrWhiteSpace([string]$_)
        })
    if ($watched.Count -ne $WatchedRoots.Count -or $watched.Count -eq 0 -or
        $spill.Count -ne $SpillRoots.Count -or $spill.Count -eq 0 -or
        $protected.Count -ne $ProtectedRoots.Count -or $protected.Count -eq 0) {
        throw 'Gate 1 evidence boundaries must be explicit and nonempty.'
    }
    foreach ($boundary in @($protected + $watched + $spill)) {
        if (Test-GatePathOverlap -First $evidencePath -Second $boundary) {
            throw 'The Gate 1 evidence root overlaps a protected or monitored root.'
        }
    }
    return $evidencePath
}

function Remove-GateSessionRoot {
    param([Parameter(Mandatory = $true)][string]$SessionRoot)

    Assert-PartisanNoReparseTree -Root $SessionRoot
    Remove-Item -LiteralPath $SessionRoot -Recurse -Force -ErrorAction Stop
}

function Get-GateCanonicalRowsDigest {
    param([Parameter(Mandatory = $true)][AllowEmptyCollection()][object[]]$Rows)
    $lines = @($Rows | Sort-Object path | ForEach-Object {
        "{0}`t{1}`t{2}" -f $_.sha256, [long]$_.length, [string]$_.path
    })
    return Get-GateSha256Text (($lines -join "`n") + "`n")
}

function Get-GateArgumentVectorDigest {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)
    return Get-GateSha256Text ((
        [string[]]$Arguments | ConvertTo-Json -Compress) + "`n")
}

function Assert-GateCandidatePeer {
    param($ServerCandidate, $ClientCandidate)
    foreach ($property in @(
        'CandidateId', 'CandidateVersion', 'RuntimeUseDisposition', 'GitHead',
        'EmbeddedBuildSha', 'EmbeddedBuildUtc', 'EmbeddedBuildLabel',
        'CampaignSchema', 'RuntimeSettingsSchema', 'AddonId', 'AddonGuid',
        'PackageHashAlgorithm', 'PackageSha256', 'ManifestSha256',
        'ReadySha256', 'WorkbenchCrc')) {
        if ([string]$ServerCandidate.$property -cne
            [string]$ClientCandidate.$property) {
            throw "Server/client candidate field differs: $property."
        }
    }
    if ([string]$ServerCandidate.RuntimeUseDisposition -cne
        'active-runtime-candidate') {
        throw 'Gate 1 requires the exact active runtime candidate.'
    }
}

function Assert-GateExecutableExact {
    param($Expected, $Actual, [string]$Label)
    foreach ($property in @(
        'fileName', 'fileVersion', 'productVersion', 'length', 'sha256')) {
        if ([string]$Expected.$property -cne [string]$Actual.$property) {
            throw "$Label differs from sealed executable identity: $property."
        }
    }
}

function New-GateSnapshot {
    param(
        [Parameter(Mandatory = $true)][string]$ProfileRoot,
        [Parameter(Mandatory = $true)][string]$DestinationRoot,
        [Parameter(Mandatory = $true)][string]$Stage,
        [Parameter(Mandatory = $true)][ValidateSet('input', 'output')]
        [string]$Direction
    )
    if (Test-Path -LiteralPath $DestinationRoot) {
        throw "$Stage/$Direction snapshot destination is not fresh."
    }
    New-Item -ItemType Directory -Path (Join-Path $DestinationRoot 'files') `
        -Force | Out-Null
    $sources = New-Object Collections.Generic.List[object]
    $nativeRoot = Join-Path $ProfileRoot '.save\game'
    if (Test-Path -LiteralPath $nativeRoot -PathType Container) {
        Assert-PartisanNoReparseTree -Root $nativeRoot
        foreach ($file in @(Get-ChildItem -LiteralPath $nativeRoot -Recurse `
                -File -Force | Sort-Object FullName)) {
            $profilePath = [IO.Path]::GetFullPath($ProfileRoot).TrimEnd('\', '/')
            $filePath = [IO.Path]::GetFullPath($file.FullName)
            $profilePrefix = $profilePath + [IO.Path]::DirectorySeparatorChar
            if (-not $filePath.StartsWith(
                    $profilePrefix, [StringComparison]::OrdinalIgnoreCase)) {
                throw 'A native save snapshot file escaped its profile root.'
            }
            $relative = $filePath.Substring($profilePrefix.Length).
                Replace('\', '/')
            [void]$sources.Add([pscustomobject]@{
                Kind = 'native'
                Source = $file.FullName
                Relative = 'files/native/' + $relative
            })
        }
    }
    foreach ($leaf in @(
        'profile\Partisan\HST_CampaignSaveData.json',
        'profile\Partisan\HST_CampaignSaveData.recovery.json')) {
        $path = Join-Path $ProfileRoot $leaf
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            [void]$sources.Add([pscustomobject]@{
                Kind = 'journal'
                Source = $path
                Relative = 'files/journal/' + $leaf.Replace('\', '/')
            })
        }
    }
    $rows = New-Object Collections.Generic.List[object]
    foreach ($source in @($sources.ToArray() | Sort-Object Relative)) {
        $destination = Join-Path $DestinationRoot `
            ([string]$source.Relative).Replace('/', '\')
        $signature = Copy-GateStableFile $source.Source $destination
        [void]$rows.Add([pscustomobject][ordered]@{
            kind = [string]$source.Kind
            path = [string]$source.Relative
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        })
    }
    $manifest = [ordered]@{
        schemaVersion = 1
        contractId = $script:GateContractId
        stage = $Stage
        direction = $Direction
        files = [object[]]$rows.ToArray()
        aggregateSha256 = Get-GateCanonicalRowsDigest $rows.ToArray()
    }
    $manifestPath = Join-Path $DestinationRoot 'snapshot.json'
    $signature = Write-GateJson -Path $manifestPath -Value $manifest -CreateOnly
    return [pscustomobject]@{
        Path = $manifestPath
        Signature = $signature
        AggregateSha256 = [string]$manifest.aggregateSha256
        Files = [object[]]$rows.ToArray()
    }
}

function New-GateEngineGuardValue {
    param(
        [string]$SessionNonce,
        [string]$StageNonce,
        [string]$RunId,
        [string]$PayloadNonce,
        [string]$Stage,
        $BuildIdentity,
        [string]$WorldResource
    )
    $ordinal = [Array]::IndexOf($script:GateStages, $Stage)
    return [ordered]@{
        m_sMagic = $script:GuardMagic
        m_iVersion = $script:AuthorityVersion
        m_sSessionNonce = $SessionNonce
        m_sStageNonce = $StageNonce
        m_sRunId = $RunId
        m_sPayloadNonce = $PayloadNonce
        m_sRequestedStage = $Stage
        m_iStageOrdinal = $ordinal
        m_sExpectedSource = $script:ExpectedSources[$Stage]
        m_iExpectedSentinelGeneration =
            $script:ExpectedSentinelGenerations[$Stage]
        m_sExpectedSaveType = $script:ExpectedSaveTypes[$Stage]
        m_sExpectedSaveName = $script:ExpectedSaveNames[$Stage]
        m_sBuildSha = $BuildIdentity.BuildSha
        m_sBuildUtc = $BuildIdentity.BuildUtc
        m_sBuildLabel = $BuildIdentity.BuildLabel
        m_iCampaignSchemaVersion = $BuildIdentity.CampaignSchemaVersion
        m_iSettingsSchemaVersion = $BuildIdentity.SettingsSchemaVersion
        m_sWorld = $WorldResource
        m_bAllowCanonicalCampaignOverwrite = $ordinal -le 2
        m_bMixedNativeProofRequired = $true
    }
}

function Start-GateGuardedRole {
    param(
        [ValidateSet('server', 'client')][string]$Role,
        $Context,
        [string]$Executable,
        $Provenance,
        [string[]]$Arguments,
        [string]$WorkingDirectory,
        [string]$TempDirectory,
        $CandidateConsumption
    )
    $oldTemp = [Environment]::GetEnvironmentVariable(
        'TEMP', [EnvironmentVariableTarget]::Process)
    $oldTmp = [Environment]::GetEnvironmentVariable(
        'TMP', [EnvironmentVariableTarget]::Process)
    try {
        [Environment]::SetEnvironmentVariable(
            'TEMP', $TempDirectory, [EnvironmentVariableTarget]::Process)
        [Environment]::SetEnvironmentVariable(
            'TMP', $TempDirectory, [EnvironmentVariableTarget]::Process)
        if ($Role -ceq 'server') {
            return Start-PartisanGuardedServer `
                -Context $Context `
                -Executable $Executable `
                -ExecutableProvenance $Provenance `
                -Arguments $Arguments `
                -WorkingDirectory $WorkingDirectory `
                -CandidateConsumption $CandidateConsumption
        }
        return Start-PartisanGuardedClient `
            -Context $Context `
            -Executable $Executable `
            -ExecutableProvenance $Provenance `
            -Arguments $Arguments `
            -WorkingDirectory $WorkingDirectory `
            -CandidateConsumption $CandidateConsumption
    }
    finally {
        [Environment]::SetEnvironmentVariable(
            'TEMP', $oldTemp, [EnvironmentVariableTarget]::Process)
        [Environment]::SetEnvironmentVariable(
            'TMP', $oldTmp, [EnvironmentVariableTarget]::Process)
    }
}

function Copy-GateLogSet {
    param(
        [string]$SourceRoot,
        [string]$DestinationRoot,
        [string]$Stage,
        [string]$Role
    )
    New-Item -ItemType Directory -Path $DestinationRoot -Force | Out-Null
    $requiredLeaves = @('console.log', 'script.log', 'error.log')
    $allowedLeaves = @($requiredLeaves + @('crash.log'))
    $actualLogs = @(Get-ChildItem -LiteralPath $SourceRoot -Recurse -File -Force |
        Where-Object { $_.Extension -ieq '.log' })
    $unknownLogs = @($actualLogs | Where-Object {
        $_.Name -cnotin $allowedLeaves
    })
    if ($unknownLogs.Count -ne 0) {
        throw "$Stage/$Role produced unknown or case-variant log leaves."
    }
    foreach ($leaf in $requiredLeaves) {
        $matches = @($actualLogs | Where-Object { $_.Name -ceq $leaf })
        if ($matches.Count -ne 1) {
            throw "$Stage/$Role did not produce one exact $leaf."
        }
        $null = Copy-GateStableFile $matches[0].FullName `
            (Join-Path $DestinationRoot $leaf)
    }
    $crashMatches = @($actualLogs | Where-Object { $_.Name -ceq 'crash.log' })
    if ($crashMatches.Count -gt 1) {
        throw "$Stage/$Role produced more than one exact crash.log."
    }
    if ($crashMatches.Count -eq 1) {
        $null = Copy-GateStableFile $crashMatches[0].FullName `
            (Join-Path $DestinationRoot 'crash.log')
    }
}

function Copy-GateStageArtifact {
    param(
        [string]$Source,
        [string]$StageEvidenceRoot,
        [string]$Leaf,
        [switch]$Optional
    )
    if (-not (Test-Path -LiteralPath $Source -PathType Leaf)) {
        if ($Optional) { return '' }
        throw "Required ordinary artifact is missing: $Leaf."
    }
    $destination = Join-Path $StageEvidenceRoot $Leaf
    $null = Copy-GateStableFile $Source $destination
    return $destination
}

function Invoke-GateRuntimeStage {
    param(
        [int]$Ordinal,
        [string]$Stage,
        [string]$RunRoot,
        [string]$GuardBase,
        [string]$PrimaryProfileRoot,
        [string]$FallbackProfileRoot,
        [string]$ClientProfileRoot,
        [string]$SessionRoot,
        [string]$SessionNonce,
        [string]$PayloadNonce,
        [string]$RunId,
        $Candidate,
        [string]$ServerExecutable,
        $ServerProvenance,
        [string]$ClientExecutablePath,
        $ClientProvenance,
        $BuildIdentity,
        [string]$WorldResource,
        [string]$LoadSavePointId,
        [string]$ExpectedSourceFingerprint,
        [string]$ExpectedSentinelFingerprint,
        [string]$ExpectedLatestSavePointId,
        [string[]]$WatchedRoots,
        [string[]]$SpillRoots,
        [int]$LoopbackPort
    )
    $profileRoot = if ($Stage -ceq 'profile_fallback_verify') {
        $FallbackProfileRoot
    }
    else { $PrimaryProfileRoot }
    $debugRoot = Join-Path $profileRoot 'profile\Partisan\debug'
    $stageEvidenceRoot = Join-Path (Join-Path $RunRoot 'raw\stages') `
        $script:StageDirectories[$Ordinal]
    $working = Join-Path $SessionRoot ('working\' + $Stage)
    $temporary = Join-Path $SessionRoot ('temp\' + $Stage)
    $addonTemp = Join-Path $SessionRoot ('addon-temp\' + $Stage)
    $serverLogs = Join-Path $SessionRoot ('logs\' + $Stage + '\server')
    $clientWorking = Join-Path $SessionRoot 'working\shutdown-client'
    $clientTemp = Join-Path $SessionRoot 'temp\shutdown-client'
    $clientAddonTemp = Join-Path $SessionRoot 'addon-temp\shutdown-client'
    $clientLogs = Join-Path $SessionRoot 'logs\shutdown-client'
    foreach ($directory in @(
            $debugRoot, $stageEvidenceRoot, $working, $temporary,
            $addonTemp, $serverLogs)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
        Assert-PartisanNoReparseAncestry -Path $directory
    }
    if ($Stage -ceq 'shutdown_checkpoint') {
        foreach ($directory in @(
                $clientWorking, $clientTemp, $clientAddonTemp, $clientLogs)) {
            New-Item -ItemType Directory -Path $directory -Force | Out-Null
        }
    }

    $stageNonce = [Guid]::NewGuid().ToString('N')
    $resultPath = Join-Path $debugRoot (
        'HST_OrdinaryCampaignPersistenceProof_{0}.{1}.json' -f $RunId, $Stage)
    $guardPath = Join-Path $debugRoot (
        'HST_OrdinaryCampaignPersistenceProof_{0}.guard.json' -f $RunId)
    $carrierPath = Join-Path $debugRoot (
        'HST_OrdinaryCampaignPersistenceProof_{0}.carrier.json' -f $RunId)
    $readyPath = Join-Path $debugRoot (
        'HST_OrdinaryCampaignPersistenceProof_{0}.{1}.mixed_native_ready.json' -f
            $RunId, $Stage)
    $endPath = Join-Path $debugRoot (
        'HST_OrdinaryCampaignPersistenceProof_{0}.{1}.end_bridge.json' -f
            $RunId, $Stage)
    foreach ($path in @($resultPath, $guardPath, $readyPath, $endPath)) {
        if (Test-Path -LiteralPath $path) {
            throw "$Stage ordinary artifact path is not fresh."
        }
    }
    $guard = New-GateEngineGuardValue `
        -SessionNonce $SessionNonce `
        -StageNonce $stageNonce `
        -RunId $RunId `
        -PayloadNonce $PayloadNonce `
        -Stage $Stage `
        -BuildIdentity $BuildIdentity `
        -WorldResource $WorldResource
    Write-JsonUtf8NoBom -Path $guardPath -Value $guard
    $null = Assert-EngineGuard `
        -Guard (Read-JsonArtifact $guardPath) `
        -SessionNonce $SessionNonce `
        -StageNonce $stageNonce `
        -RunId $RunId `
        -PayloadNonce $PayloadNonce `
        -Stage $Stage `
        -ExpectedBuild $BuildIdentity `
        -ExpectedWorld $WorldResource
    $guardInputEvidence = Join-Path $stageEvidenceRoot 'guard-input.json'
    $null = Copy-GateStableFile $guardPath $guardInputEvidence
    $inputSnapshot = New-GateSnapshot `
        -ProfileRoot $profileRoot `
        -DestinationRoot (Join-Path $stageEvidenceRoot 'save-input') `
        -Stage $Stage `
        -Direction input

    $canonicalPath = Join-Path $profileRoot `
        'profile\Partisan\HST_CampaignSaveData.json'
    $recoveryPath = Join-Path $profileRoot `
        'profile\Partisan\HST_CampaignSaveData.recovery.json'
    $journalBefore = $null
    $readOnly = $Ordinal -ge 3
    if ($readOnly) {
        $journalBefore = Get-CampaignJournalFileState $canonicalPath $recoveryPath
        if ([int]$journalBefore.FileCount -ne 2) {
            throw "$Stage requires both fallback journal generations."
        }
    }

    $context = $null
    $teardown = $null
    $receiptTest = $null
    $serverArguments = $null
    $clientArguments = [string[]]@()
    $serverExit = $null
    try {
        $context = New-PartisanGuardedRuntimeContext `
            -GuardBase $GuardBase `
            -Purpose ('gate1_runtime_retention_' + $Ordinal) `
            -WatchedRoots $WatchedRoots `
            -SpillRoots $SpillRoots `
            -LoopbackPorts @($LoopbackPort)
        $candidateStage = New-PartisanCandidateStage `
            -Context $context `
            -Candidate $Candidate
        $serverArguments = [string[]]@(Get-StageArgumentVector `
            -RuntimeAddonPath $Candidate.RuntimeAddonRootPath `
            -PackedAddonsParent $candidateStage.StageRootPath `
            -ProfileRoot $profileRoot `
            -WorldResource $WorldResource `
            -SessionNonce $SessionNonce `
            -StageNonce $stageNonce `
            -RunId $RunId `
            -Stage $Stage `
            -LoadSavePointId $LoadSavePointId)
        $serverArguments += @(
            '-logsDir', $serverLogs,
            '-addonTempDir', $addonTemp,
            '-hstReleaseCandidateId', [string]$Candidate.CandidateId,
            '-hstReleasePackageSha256', [string]$Candidate.PackageSha256,
            '-hstReleaseManifestSha256', [string]$Candidate.ManifestSha256)
        $serverLaunch = Start-GateGuardedRole `
            -Role server `
            -Context $context `
            -Executable $ServerExecutable `
            -Provenance $ServerProvenance `
            -Arguments $serverArguments `
            -WorkingDirectory $working `
            -TempDirectory $temporary `
            -CandidateConsumption $candidateStage
        if ($Stage -ceq 'shutdown_checkpoint') {
            $ready = Wait-StableJsonArtifact `
                -Path $readyPath `
                -DeadlineUtc ([DateTime]::UtcNow.AddSeconds($StageTimeoutSeconds))
            $null = Assert-MixedNativeReadyReceipt `
                -Receipt $ready `
                -SessionNonce $SessionNonce `
                -StageNonce $stageNonce `
                -RunId $RunId `
                -PayloadNonce $PayloadNonce `
                -ExpectedBuild $BuildIdentity `
                -ExpectedWorld $WorldResource
            $clientArguments = [string[]]@(Get-LoopbackClientArgumentVector `
                -RuntimeAddonPath $Candidate.RuntimeAddonRootPath `
                -PackedAddonsParent $candidateStage.StageRootPath `
                -ProfileRoot $ClientProfileRoot `
                -AddonTempDirectory $clientAddonTemp `
                -ClientLogDirectory $clientLogs)
            $clientArguments += @(
                '-hstReleaseCandidateId', [string]$Candidate.CandidateId,
                '-hstReleasePackageSha256', [string]$Candidate.PackageSha256,
                '-hstReleaseManifestSha256', [string]$Candidate.ManifestSha256)
            $null = Start-GateGuardedRole `
                -Role client `
                -Context $context `
                -Executable $ClientExecutablePath `
                -Provenance $ClientProvenance `
                -Arguments $clientArguments `
                -WorkingDirectory $clientWorking `
                -TempDirectory $clientTemp `
                -CandidateConsumption $candidateStage
        }
        $wait = Wait-PartisanGuardedProcess `
            -Context $context `
            -Launch $serverLaunch `
            -TimeoutSeconds $StageTimeoutSeconds `
            -PollMilliseconds $PollMilliseconds `
            -RequireZeroExit
        $serverExit = [int]$wait.ExitCode
        $teardown = Invoke-PartisanGuardedTeardown -Context $context
        $receiptTest = Test-PartisanGuardedRuntimeReceipt `
            -Path $teardown.CleanReceiptPath `
            -ExpectedSignature $teardown.CleanReceiptSignature
        if (-not [bool]$receiptTest.Complete) {
            throw "$Stage guarded runtime receipt did not complete."
        }
    }
    catch {
        $failure = $_
        if ($context -and -not $teardown) {
            try { $null = Invoke-PartisanGuardedTeardown -Context $context }
            catch { }
        }
        throw $failure
    }

    $result = Wait-StableJsonArtifact `
        -Path $resultPath `
        -DeadlineUtc ([DateTime]::UtcNow.AddSeconds($ResultGraceSeconds))
    $result = Assert-StageResult `
        -Result $result `
        -SessionNonce $SessionNonce `
        -StageNonce $stageNonce `
        -RunId $RunId `
        -PayloadNonce $PayloadNonce `
        -Stage $Stage `
        -ExpectedBuild $BuildIdentity `
        -ExpectedWorld $WorldResource `
        -ExpectedPriorSavePointId $ExpectedLatestSavePointId `
        -ExpectedSourceFingerprint $ExpectedSourceFingerprint `
        -ExpectedSentinelFingerprint $ExpectedSentinelFingerprint
    if (Test-Path -LiteralPath $guardPath) {
        throw "$Stage did not consume its one-use ordinary guard."
    }
    $journalAfter = Get-CampaignJournalFileState $canonicalPath $recoveryPath
    if ($readOnly -and -not (Test-CampaignJournalFileStateExact `
            $journalBefore $journalAfter)) {
        throw "$Stage changed its read-only fallback journal."
    }
    $endReceipt = $null
    if ($Stage -ceq 'shutdown_checkpoint') {
        $endReceipt = Wait-StableJsonArtifact `
            -Path $endPath `
            -DeadlineUtc ([DateTime]::UtcNow.AddSeconds($ResultGraceSeconds))
        $endReceipt = Assert-EndBridgeReceipt `
            -Receipt $endReceipt `
            -SessionNonce $SessionNonce `
            -StageNonce $stageNonce `
            -RunId $RunId `
            -PayloadNonce $PayloadNonce `
            -ExpectedBuild $BuildIdentity `
            -ExpectedWorld $WorldResource `
            -ExpectedShutdownSavePointId ([string]$result.m_sCreatedSavePointId)
    }
    $latestSavePointId = if ($Ordinal -le 2) {
        [string]$result.m_sCreatedSavePointId
    }
    elseif (-not [string]::IsNullOrWhiteSpace($ExpectedLatestSavePointId)) {
        $ExpectedLatestSavePointId
    }
    else { $LoadSavePointId }
    $carrier = Wait-StableJsonArtifact `
        -Path $carrierPath `
        -DeadlineUtc ([DateTime]::UtcNow.AddSeconds($ResultGraceSeconds))
    $carrier = Assert-Carrier `
        -Carrier $carrier `
        -SessionNonce $SessionNonce `
        -RunId $RunId `
        -PayloadNonce $PayloadNonce `
        -Stage $Stage `
        -ExpectedBuild $BuildIdentity `
        -ExpectedWorld $WorldResource `
        -Result $result `
        -ExpectedLatestSavePointId $latestSavePointId
    $outputSnapshot = New-GateSnapshot `
        -ProfileRoot $profileRoot `
        -DestinationRoot (Join-Path $stageEvidenceRoot 'save-output') `
        -Stage $Stage `
        -Direction output

    $ownerSource = Join-Path $debugRoot (
        'HST_OrdinaryCampaignPersistenceProof_{0}.owner.json' -f $RunId)
    $ownerEvidence = Copy-GateStageArtifact $ownerSource $stageEvidenceRoot `
        'owner.json'
    $resultEvidence = Copy-GateStageArtifact $resultPath $stageEvidenceRoot `
        'result.json'
    $carrierEvidence = Copy-GateStageArtifact $carrierPath $stageEvidenceRoot `
        'carrier.json'
    $readyEvidence = Copy-GateStageArtifact $readyPath $stageEvidenceRoot `
        'mixed-native-ready.json' -Optional
    $endEvidence = Copy-GateStageArtifact $endPath $stageEvidenceRoot `
        'end-bridge.json' -Optional
    Copy-GateLogSet $serverLogs `
        (Join-Path $stageEvidenceRoot 'diagnostic-server-logs') `
        $Stage 'diagnostic-server'
    if ($Stage -ceq 'shutdown_checkpoint') {
        Copy-GateLogSet $clientLogs `
            (Join-Path $stageEvidenceRoot 'diagnostic-client-logs') `
            $Stage 'diagnostic-client'
    }

    $receiptPortable = ConvertTo-GatePortablePath $RunRoot `
        $teardown.CleanReceiptPath
    $journalPortable = ConvertTo-GatePortablePath $RunRoot $receiptTest.JournalPath
    $completionPortable = ConvertTo-GatePortablePath $RunRoot `
        $receiptTest.CompletionAttestationPath
    return [pscustomobject]@{
        Runtime = [pscustomobject][ordered]@{
            ordinal = $Ordinal
            stage = $Stage
            contextId = [string]$receiptTest.ContextId
            candidateBindingSha256 = [string]$receiptTest.CandidateBindingSha256
            serverExitCode = [int]$serverExit
            clientLaunched = $Stage -ceq 'shutdown_checkpoint'
            serverArgumentsSha256 = Get-GateArgumentVectorDigest $serverArguments
            clientArgumentsSha256 = if ($clientArguments.Count -gt 0) {
                Get-GateArgumentVectorDigest $clientArguments
            }
            else { '' }
            receiptPath = $receiptPortable
            receiptSignature = $teardown.CleanReceiptSignature
            journalPath = $journalPortable
            journalSignature = $receiptTest.JournalSignature
            completionPath = $completionPortable
            completionSignature = $receiptTest.CompletionAttestationSignature
        }
        Persistence = [pscustomobject][ordered]@{
            ordinal = $Ordinal
            stage = $Stage
            stageNonce = $stageNonce
            source = [string]$result.m_sSource
            saveType = [string]$result.m_sExpectedSaveType
            saveName = [string]$script:ExpectedSaveNames[$Stage]
            loadSavePointId = $LoadSavePointId
            createdSavePointId = [string]$result.m_sCreatedSavePointId
            expectedPriorSavePointId = $ExpectedLatestSavePointId
            expectedSourceFingerprint = $ExpectedSourceFingerprint
            expectedSentinelFingerprint = $ExpectedSentinelFingerprint
            latestSavePointId = $latestSavePointId
            sourceFingerprint = [string]$result.m_sSourceFingerprint
            finalFingerprint =
                [string]$result.m_sProfileFallbackReadBackFingerprint
            journalFileCount = [int]$journalAfter.FileCount
            journalGeneration = [int]$result.m_iProfileJournalGeneration
            journalSlot = [string]$result.m_sProfileJournalSlot
            readOnly = [bool]$readOnly
            ownerPath = ConvertTo-GatePortablePath $RunRoot $ownerEvidence
            guardInputPath = ConvertTo-GatePortablePath $RunRoot `
                $guardInputEvidence
            resultPath = ConvertTo-GatePortablePath $RunRoot $resultEvidence
            carrierPath = ConvertTo-GatePortablePath $RunRoot $carrierEvidence
            mixedNativeReadyPath = if ($readyEvidence) {
                ConvertTo-GatePortablePath $RunRoot $readyEvidence
            }
            else { '' }
            endBridgePath = if ($endEvidence) {
                ConvertTo-GatePortablePath $RunRoot $endEvidence
            }
            else { '' }
            inputSnapshotPath = ConvertTo-GatePortablePath $RunRoot `
                $inputSnapshot.Path
            inputSnapshotSha256 = [string]$inputSnapshot.AggregateSha256
            outputSnapshotPath = ConvertTo-GatePortablePath $RunRoot `
                $outputSnapshot.Path
            outputSnapshotSha256 = [string]$outputSnapshot.AggregateSha256
        }
        Result = $result
        Carrier = $carrier
    }
}

function Copy-GateSnapshotIntoProfile {
    param(
        [Parameter(Mandatory = $true)][string]$SnapshotManifestPath,
        [Parameter(Mandatory = $true)][string]$ProfileRoot
    )
    if (Test-Path -LiteralPath $ProfileRoot) {
        throw 'A standard-runtime profile destination is not fresh.'
    }
    New-Item -ItemType Directory -Path $ProfileRoot -Force | Out-Null
    $manifest = Read-JsonArtifact $SnapshotManifestPath
    foreach ($row in @($manifest.files)) {
        $relative = [string]$row.path
        $prefix = if ([string]$row.kind -ceq 'native') {
            'files/native/'
        }
        elseif ([string]$row.kind -ceq 'journal') { 'files/journal/' }
        else { throw 'A diagnostic lineage snapshot contains an unknown file kind.' }
        if (-not $relative.StartsWith($prefix, [StringComparison]::Ordinal)) {
            throw 'A diagnostic lineage snapshot row has an invalid kind prefix.'
        }
        $profileRelative = $relative.Substring($prefix.Length)
        $source = Join-Path (Split-Path -Parent $SnapshotManifestPath) `
            $relative.Replace('/', '\')
        $destination = Join-Path $ProfileRoot $profileRelative.Replace('/', '\')
        $null = Copy-GateStableFile $source $destination
    }
}

function Wait-GateStandardLogReadiness {
    param(
        [Parameter(Mandatory = $true)]$Launch,
        [Parameter(Mandatory = $true)][string]$LogRoot,
        [Parameter(Mandatory = $true)][string]$Stage,
        [Parameter(Mandatory = $true)][datetime]$DeadlineUtc
    )
    $stable = 0
    $lastSignature = ''
    while ([DateTime]::UtcNow -lt $DeadlineUtc) {
        $status = Get-PartisanProcessIdentityStatus -Identity $Launch.RootIdentity
        if ([string]$status.Status -cne 'alive') {
            throw "$Stage standard runtime exited before log readiness."
        }
        $consoleRows = @(Get-ChildItem -LiteralPath $LogRoot -Recurse -File `
            -Force -ErrorAction SilentlyContinue | Where-Object {
                $_.Name -ceq 'console.log'
            })
        if ($consoleRows.Count -eq 1 -and $consoleRows[0].Length -gt 0) {
            $text = [IO.File]::ReadAllText($consoleRows[0].FullName)
            if ($text -match '(?m)Game successfully created\.' -and
                $text -match '(?m)CLI Params:') {
                $signature = Get-GateFileSignature $consoleRows[0].FullName
                $key = [string]$signature.length + ':' + [string]$signature.sha256
                if ($key -ceq $lastSignature) { $stable++ }
                else { $lastSignature = $key; $stable = 1 }
                if ($stable -ge 2) {
                    return [pscustomobject][ordered]@{
                        observed = $true
                        evidence = 'process-alive-and-console-game-created'
                        consoleSignature = $signature
                    }
                }
            }
        }
        Start-Sleep -Milliseconds $PollMilliseconds
    }
    throw "$Stage standard runtime did not reach bounded log readiness."
}

function Invoke-GateStandardRetentionContext {
    param(
        [int]$Ordinal,
        [string]$Stage,
        [string]$RunRoot,
        [string]$GuardBase,
        [string]$SessionRoot,
        [string]$SourceSnapshotPath,
        [string]$LoadSavePointId,
        [bool]$LaunchClient,
        $Candidate,
        [string]$ServerExecutable,
        $ServerProvenance,
        [string]$ClientExecutablePath,
        $ClientProvenance,
        [string]$SettingsEvidencePath,
        [string[]]$WatchedRoots,
        [string[]]$SpillRoots,
        [int]$LoopbackPort,
        [int]$ReadinessSeconds
    )
    $evidenceRoot = Join-Path (Join-Path $RunRoot 'raw\standard-runtime') `
        $script:StageDirectories[$Ordinal]
    $profileRoot = Join-Path $SessionRoot ('standard-profile-' + $Ordinal)
    $working = Join-Path $SessionRoot ('standard-working-' + $Ordinal)
    $temporary = Join-Path $SessionRoot ('standard-temp-' + $Ordinal)
    $addonTemp = Join-Path $SessionRoot ('standard-addon-temp-' + $Ordinal)
    $serverLogs = Join-Path $SessionRoot ('standard-logs-' + $Ordinal + '\server')
    $clientProfile = Join-Path $SessionRoot 'standard-client-profile'
    $clientWorking = Join-Path $SessionRoot 'standard-client-working'
    $clientTemp = Join-Path $SessionRoot 'standard-client-temp'
    $clientAddonTemp = Join-Path $SessionRoot 'standard-client-addon-temp'
    $clientLogs = Join-Path $SessionRoot 'standard-client-logs'
    Copy-GateSnapshotIntoProfile $SourceSnapshotPath $profileRoot
    $settingsDestination = Join-Path $profileRoot 'profile\Partisan\HST_Settings.json'
    $null = Copy-GateStableFile $SettingsEvidencePath $settingsDestination
    foreach ($directory in @(
            $evidenceRoot, $working, $temporary, $addonTemp, $serverLogs)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
    if ($LaunchClient) {
        foreach ($directory in @(
                $clientProfile, $clientWorking, $clientTemp,
                $clientAddonTemp, $clientLogs)) {
            New-Item -ItemType Directory -Path $directory -Force | Out-Null
        }
    }
    $inputSnapshot = New-GateSnapshot `
        -ProfileRoot $profileRoot `
        -DestinationRoot (Join-Path $evidenceRoot 'save-input') `
        -Stage $Stage `
        -Direction input
    $context = $null
    $teardown = $null
    try {
        $context = New-PartisanGuardedRuntimeContext `
            -GuardBase $GuardBase `
            -Purpose ('gate1_standard_retention_' + $Ordinal) `
            -WatchedRoots $WatchedRoots `
            -SpillRoots $SpillRoots `
            -LoopbackPorts @($LoopbackPort)
        $candidateStage = New-PartisanCandidateStage $context $Candidate
        $serverArguments = [string[]]@(
            '-gproj', $candidateStage.PackedProjectPath,
            '-server', 'Worlds/HST_Everon/HST_Everon.ent',
            '-MissionHeader', $script:MissionHeader,
            '-addonsDir', $candidateStage.AddonSearchPath,
            '-addons', $script:ProjectId,
            '-profile', $profileRoot,
            '-logsDir', $serverLogs,
            '-addonTempDir', $addonTemp,
            '-logLevel', 'normal',
            '-logTime', 'datetime',
            '-noThrow',
            '-maxFPS', '30')
        if (-not [string]::IsNullOrWhiteSpace($LoadSavePointId)) {
            Assert-NativeSavePointId $LoadSavePointId "$Stage standard load UUID"
            $serverArguments += @('-loadSessionSave', $LoadSavePointId)
        }
        $serverArguments += @(
            '-hstReleaseCandidateId', [string]$Candidate.CandidateId,
            '-hstReleasePackageSha256', [string]$Candidate.PackageSha256,
            '-hstReleaseManifestSha256', [string]$Candidate.ManifestSha256)
        if ($serverArguments -icontains '-hstOrdinaryCampaignPersistenceProof' -or
            $serverArguments -icontains '-autoshutdown' -or
            $serverArguments -icontains '-keepSessionSave' -or
            @($serverArguments | Where-Object {
                [string]$_ -match '^-' -and
                ([string]$_ -imatch
                    '(?:autotest|selftest|test|proof|debug|diagnostic)' -or
                [string]$_ -imatch '^-scrDefine(?:$|=)')
            }).Count -ne 0) {
            throw "$Stage standard context acquired diagnostic or retention authority."
        }
        $serverLaunch = Start-GateGuardedRole server $context $ServerExecutable `
            $ServerProvenance $serverArguments $working $temporary $candidateStage
        $readiness = Wait-GateStandardLogReadiness `
            -Launch $serverLaunch `
            -LogRoot $serverLogs `
            -Stage $Stage `
            -DeadlineUtc ([DateTime]::UtcNow.AddSeconds($ReadinessSeconds))
        $clientArguments = [string[]]@()
        if ($LaunchClient) {
            $clientArguments = [string[]]@(Get-LoopbackClientArgumentVector `
                -RuntimeAddonPath $Candidate.RuntimeAddonRootPath `
                -PackedAddonsParent $candidateStage.StageRootPath `
                -ProfileRoot $clientProfile `
                -AddonTempDirectory $clientAddonTemp `
                -ClientLogDirectory $clientLogs)
            $clientArguments += @(
                '-hstReleaseCandidateId', [string]$Candidate.CandidateId,
                '-hstReleasePackageSha256', [string]$Candidate.PackageSha256,
                '-hstReleaseManifestSha256', [string]$Candidate.ManifestSha256)
            if ($clientArguments -icontains '-autoshutdown' -or
                $clientArguments -icontains '-keepSessionSave' -or
                @($clientArguments | Where-Object {
                    [string]$_ -match '^-' -and
                    ([string]$_ -imatch
                        '(?:autotest|selftest|test|proof|debug|diagnostic)' -or
                    [string]$_ -imatch '^-scrDefine(?:$|=)')
                }).Count -ne 0) {
                throw "$Stage standard client acquired diagnostic authority."
            }
            $clientLaunch = Start-GateGuardedRole client $context `
                $ClientExecutablePath $ClientProvenance $clientArguments `
                $clientWorking $clientTemp $candidateStage
            $null = Wait-GateStandardLogReadiness `
                -Launch $clientLaunch `
                -LogRoot $clientLogs `
                -Stage ($Stage + '/client') `
                -DeadlineUtc ([DateTime]::UtcNow.AddSeconds($ReadinessSeconds))
        }
        $teardown = Invoke-PartisanGuardedTeardown $context
        $tested = Test-PartisanGuardedRuntimeReceipt `
            $teardown.CleanReceiptPath $teardown.CleanReceiptSignature
        if (-not $tested.Complete) {
            throw "$Stage standard guarded receipt is incomplete."
        }
    }
    catch {
        $failure = $_
        if ($context -and -not $teardown) {
            try { $null = Invoke-PartisanGuardedTeardown $context } catch { }
        }
        throw $failure
    }
    $outputSnapshot = New-GateSnapshot `
        -ProfileRoot $profileRoot `
        -DestinationRoot (Join-Path $evidenceRoot 'save-output') `
        -Stage $Stage `
        -Direction output
    if ([string]$inputSnapshot.AggregateSha256 -cne
        [string]$outputSnapshot.AggregateSha256) {
        throw "$Stage standard retention context mutated its supplied save bytes."
    }
    Copy-GateLogSet $serverLogs (Join-Path $evidenceRoot 'server-logs') `
        $Stage 'server'
    if ($LaunchClient) {
        Copy-GateLogSet $clientLogs (Join-Path $evidenceRoot 'client-logs') `
            $Stage 'client'
    }
    return [pscustomobject][ordered]@{
        ordinal = $Ordinal
        stage = $Stage
        contextId = [string]$tested.ContextId
        candidateBindingSha256 = [string]$tested.CandidateBindingSha256
        serverStopDisposition = 'guarded-external-stop-after-log-readiness'
        readinessEvidence = [string]$readiness.evidence
        clientLaunched = $LaunchClient
        serverArgumentsSha256 = Get-GateArgumentVectorDigest $serverArguments
        clientArgumentsSha256 = if ($clientArguments.Count) {
            Get-GateArgumentVectorDigest $clientArguments
        }
        else { '' }
        receiptPath = ConvertTo-GatePortablePath $RunRoot `
            $teardown.CleanReceiptPath
        receiptSignature = $teardown.CleanReceiptSignature
        journalPath = ConvertTo-GatePortablePath $RunRoot $tested.JournalPath
        journalSignature = $tested.JournalSignature
        completionPath = ConvertTo-GatePortablePath $RunRoot `
            $tested.CompletionAttestationPath
        completionSignature = $tested.CompletionAttestationSignature
        inputSnapshotPath = ConvertTo-GatePortablePath $RunRoot $inputSnapshot.Path
        inputSnapshotSha256 = [string]$inputSnapshot.AggregateSha256
        outputSnapshotPath = ConvertTo-GatePortablePath $RunRoot $outputSnapshot.Path
        outputSnapshotSha256 = [string]$outputSnapshot.AggregateSha256
    }
}

function Get-GatePayloadRole {
    param([string]$Path)
    if ($Path -ceq 'run.owner.json') { return 'run_owner' }
    if ($Path -ceq 'identity/candidate.json') { return 'candidate_manifest' }
    if ($Path -ceq 'identity/candidate.ready.json') { return 'candidate_ready' }
    if ($Path -like 'identity/package/Partisan/*') { return 'candidate_package' }
    if ($Path -ceq 'configuration/HST_Settings.json') { return 'runtime_settings' }
    if ($Path -ceq 'configuration/launch-contract.json') {
        return 'launch_contract'
    }
    if ($Path -like 'raw/guarded-runtime/*.receipt.json') {
        return 'guarded_runtime_receipt'
    }
    if ($Path -like 'raw/guarded-runtime/*.journal.json') {
        return 'guarded_runtime_journal'
    }
    if ($Path -like 'raw/guarded-runtime/*.completion.json') {
        return 'guarded_runtime_completion'
    }
    if ($Path -like 'raw/diagnostic-guarded-runtime/*.receipt.json') {
        return 'diagnostic_guarded_runtime_receipt'
    }
    if ($Path -like 'raw/diagnostic-guarded-runtime/*.journal.json') {
        return 'diagnostic_guarded_runtime_journal'
    }
    if ($Path -like 'raw/diagnostic-guarded-runtime/*.completion.json') {
        return 'diagnostic_guarded_runtime_completion'
    }
    if ($Path -like '*/save-input/snapshot.json' -or
        $Path -like '*/save-output/snapshot.json') {
        return 'save_snapshot_manifest'
    }
    if ($Path -like '*/save-input/files/native/*') { return 'native_save_input' }
    if ($Path -like '*/save-output/files/native/*') { return 'native_save_output' }
    if ($Path -like '*/save-input/files/journal/*') {
        return 'profile_journal_input'
    }
    if ($Path -like '*/save-output/files/journal/*') {
        return 'profile_journal_output'
    }
    if ($Path -like '*/server-logs/console.log') { return 'server_console_log' }
    if ($Path -like '*/server-logs/script.log') { return 'server_script_log' }
    if ($Path -like '*/server-logs/error.log') { return 'server_error_log' }
    if ($Path -like '*/server-logs/crash.log') { return 'server_crash_log' }
    if ($Path -like '*/client-logs/console.log') { return 'client_console_log' }
    if ($Path -like '*/client-logs/script.log') { return 'client_script_log' }
    if ($Path -like '*/client-logs/error.log') { return 'client_error_log' }
    if ($Path -like '*/client-logs/crash.log') { return 'client_crash_log' }
    if ($Path -like '*/diagnostic-server-logs/console.log') {
        return 'diagnostic_server_console_log'
    }
    if ($Path -like '*/diagnostic-server-logs/script.log') {
        return 'diagnostic_server_script_log'
    }
    if ($Path -like '*/diagnostic-server-logs/error.log') {
        return 'diagnostic_server_error_log'
    }
    if ($Path -like '*/diagnostic-server-logs/crash.log') {
        return 'diagnostic_server_crash_log'
    }
    if ($Path -like '*/diagnostic-client-logs/console.log') {
        return 'diagnostic_client_console_log'
    }
    if ($Path -like '*/diagnostic-client-logs/script.log') {
        return 'diagnostic_client_script_log'
    }
    if ($Path -like '*/diagnostic-client-logs/error.log') {
        return 'diagnostic_client_error_log'
    }
    if ($Path -like '*/diagnostic-client-logs/crash.log') {
        return 'diagnostic_client_crash_log'
    }
    switch -CaseSensitive ([IO.Path]::GetFileName($Path)) {
        'owner.json' { return 'ordinary_owner' }
        'guard-input.json' { return 'ordinary_guard' }
        'result.json' { return 'ordinary_result' }
        'carrier.json' { return 'ordinary_carrier' }
        'mixed-native-ready.json' { return 'ordinary_mixed_native_ready' }
        'end-bridge.json' { return 'ordinary_end_bridge' }
    }
    throw "No retained file role exists for $Path."
}

function Get-GatePayloadStage {
    param([string]$Path)
    for ($index = 0; $index -lt $script:StageDirectories.Count; $index++) {
        if ($Path -like ('raw/stages/' + $script:StageDirectories[$index] + '/*')) {
            return $script:GateStages[$index]
        }
        if ($Path -like ('raw/standard-runtime/' +
                $script:StageDirectories[$index] + '/*')) {
            return $script:GateStages[$index]
        }
    }
    return 'run'
}

if ($LibraryOnly) { return }

$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$releaseModulePath = Join-Path $PSScriptRoot 'Partisan.ReleaseCandidate.psm1'
$guardedModulePath = Join-Path $PSScriptRoot 'Partisan.GuardedRuntime.psm1'
$ordinaryPath = Join-Path $PSScriptRoot `
    'run-ordinary-campaign-persistence-proof.ps1'
$indexPath = Join-Path $PSScriptRoot `
    'New-PartisanGate1RuntimeRetentionIndex.ps1'
Import-Module $releaseModulePath -Force -ErrorAction Stop
Import-Module $guardedModulePath -Force -ErrorAction Stop
. $ordinaryPath `
    -Executable $ServerDiagnosticExecutable `
    -RuntimeAddonRoot $RuntimeAddonRoot `
    -WorkbenchExecutable $ServerDiagnosticExecutable `
    -LibraryOnly

$gitHead = (& git -C $repositoryRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $gitHead -cnotmatch '^[0-9a-f]{40}$') {
    throw 'Gate 1 could not resolve the checkout Git HEAD.'
}
$dirty = @(& git -C $repositoryRoot status --porcelain --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $dirty.Count -ne 0) {
    throw 'Gate 1 requires a clean checkout so its harness is immutable.'
}

$serverCandidate = Assert-PartisanReleaseCandidate `
    -ManifestPath $ManifestPath `
    -BundleRoot $BundleRoot `
    -RuntimeAddonRoot $RuntimeAddonRoot `
    -Executable $ServerDiagnosticExecutable `
    -RuntimeRole server `
    -ConsumerIntent runtime
$clientCandidate = Assert-PartisanReleaseCandidate `
    -ManifestPath $ManifestPath `
    -BundleRoot $BundleRoot `
    -RuntimeAddonRoot $RuntimeAddonRoot `
    -Executable $ClientDiagnosticExecutable `
    -RuntimeRole client `
    -ConsumerIntent runtime
Assert-GateCandidatePeer $serverCandidate $clientCandidate

$serverExecutable = [IO.Path]::GetFullPath((Join-Path `
    (Split-Path -Parent $serverCandidate.ExecutablePath) `
    'ArmaReforgerServer.exe'))
$clientExecutablePath = [IO.Path]::GetFullPath($ClientExecutable)
if ((Split-Path -Leaf $serverExecutable) -cne 'ArmaReforgerServer.exe' -or
    (Split-Path -Leaf $clientExecutablePath) -cne 'ArmaReforgerSteam.exe' -or
    -not (Test-Path -LiteralPath $serverExecutable -PathType Leaf) -or
    -not (Test-Path -LiteralPath $clientExecutablePath -PathType Leaf)) {
    throw 'Gate 1 requires the standard server and standard Steam client.'
}
$serverProvenance = Get-PartisanExecutableProvenance $serverExecutable
$clientProvenance = Get-PartisanExecutableProvenance $clientExecutablePath
Assert-GateExecutableExact $serverCandidate.RecordedRuntimeExecutable `
    $serverProvenance 'Standard server'
Assert-GateExecutableExact $clientCandidate.RecordedRuntimeExecutable `
    $clientProvenance 'Standard client'
if ([string]$serverProvenance.fileVersion -cne
        [string]$clientProvenance.fileVersion -or
    [string]$serverProvenance.productVersion -cne
        [string]$clientProvenance.productVersion) {
    throw 'Gate 1 standard server and client are not exact build peers.'
}

$protectedRoots = @(
    $repositoryRoot,
    $serverCandidate.BundleRootPath,
    $serverCandidate.RuntimeAddonRootPath,
    (Split-Path -Parent $serverCandidate.ExecutablePath),
    (Split-Path -Parent $clientCandidate.ExecutablePath),
    (Split-Path -Parent $serverExecutable),
    (Split-Path -Parent $clientExecutablePath))
$evidenceBase = Assert-GateEvidenceRootSafe `
    -EvidenceRoot $EvidenceRoot `
    -ProtectedRoots $protectedRoots `
    -WatchedRoots $WatchedRoots `
    -SpillRoots $SpillRoots
Assert-PartisanNoReparseAncestry -Path $evidenceBase
if (-not (Test-Path -LiteralPath $evidenceBase -PathType Container)) {
    New-Item -ItemType Directory -Path $evidenceBase -Force | Out-Null
}
Assert-PartisanNoReparseAncestry -Path $evidenceBase
$nonce = [Guid]::NewGuid().ToString('N')
$payloadNonce = [Guid]::NewGuid().ToString('N')
$startedUtc = [DateTime]::UtcNow
$stamp = $startedUtc.ToString(
    'yyyyMMddTHHmmssZ', [Globalization.CultureInfo]::InvariantCulture)
$runId = 'gate1_{0}_{1}' -f $stamp, $nonce.Substring(0, 20)
$runRoot = Join-Path (Join-Path (Join-Path $evidenceBase `
    $serverCandidate.CandidateId) 'gate1-runtime-retention') `
    ($stamp + '-' + $nonce.Substring(0, 12))
if (Test-Path -LiteralPath $runRoot) {
    throw 'The final Gate 1 run directory is not fresh.'
}
New-Item -ItemType Directory -Path $runRoot -ErrorAction Stop | Out-Null
$diagnosticGuardBase = Join-Path $runRoot 'raw\diagnostic-guarded-runtime'
$standardGuardBase = Join-Path $runRoot 'raw\guarded-runtime'
$sessionRoot = Join-Path $runRoot 'session'
$primaryProfileRoot = Join-Path $sessionRoot 'primary-profile'
$fallbackProfileRoot = Join-Path $sessionRoot 'fallback-profile'
$clientProfileRoot = Join-Path $sessionRoot 'client-profile'
foreach ($directory in @(
        $diagnosticGuardBase, $standardGuardBase,
        $primaryProfileRoot, $fallbackProfileRoot,
        $clientProfileRoot, (Join-Path $runRoot 'identity\package\Partisan'),
        (Join-Path $runRoot 'configuration'),
        (Join-Path $runRoot 'raw\stages'))) {
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
}
$wrapper = Get-Process -Id $PID -ErrorAction Stop
$owner = [ordered]@{
    schemaVersion = 1
    magic = 'partisan_gate1_runtime_retention_owner_v1'
    runId = $runId
    nonce = $nonce
    ownerPid = $PID
    ownerStartUtc = $wrapper.StartTime.ToUniversalTime().ToString('o')
    purpose = 'packaged-gate1-runtime-retention'
}
$null = Write-GateJson (Join-Path $runRoot 'run.owner.json') $owner -CreateOnly
$sessionOwnerPath = Join-Path $sessionRoot '.owner.json'
$null = Write-GateJson $sessionOwnerPath $owner -CreateOnly

$candidateManifestEvidence = Join-Path $runRoot 'identity\candidate.json'
$candidateReadyEvidence = Join-Path $runRoot 'identity\candidate.ready.json'
$null = Copy-GateStableFile $serverCandidate.TrackedManifestPath `
    $candidateManifestEvidence
$null = Copy-GateStableFile $serverCandidate.TrackedReadyPath `
    $candidateReadyEvidence
$packageEvidenceRows = New-Object Collections.Generic.List[object]
foreach ($row in @($serverCandidate.PackageFiles | Sort-Object indexPath)) {
    $leaf = Split-Path -Leaf ([string]$row.indexPath)
    $destination = Join-Path $runRoot ('identity\package\Partisan\' + $leaf)
    $signature = Copy-GateStableFile `
        (Join-Path $serverCandidate.PackedAddonPath $leaf) $destination
    [void]$packageEvidenceRows.Add([pscustomobject][ordered]@{
        indexPath = [string]$row.indexPath
        path = ConvertTo-GatePortablePath $runRoot $destination
        length = [long]$signature.length
        sha256 = [string]$signature.sha256
    })
}

$buildIdentity = [pscustomobject][ordered]@{
    BuildSha = [string]$serverCandidate.EmbeddedBuildSha
    BuildUtc = [string]$serverCandidate.EmbeddedBuildUtc
    BuildLabel = [string]$serverCandidate.EmbeddedBuildLabel
    CampaignSchemaVersion = [int]$serverCandidate.CampaignSchema
    SettingsSchemaVersion = [int]$serverCandidate.RuntimeSettingsSchema
}
$worldResource = 'Worlds/HST_Everon/HST_Everon.ent'
$primaryPartisan = Join-Path $primaryProfileRoot 'profile\Partisan'
$primaryDebug = Join-Path $primaryPartisan 'debug'
$fallbackPartisan = Join-Path $fallbackProfileRoot 'profile\Partisan'
$fallbackDebug = Join-Path $fallbackPartisan 'debug'
New-Item -ItemType Directory -Path $primaryDebug -Force | Out-Null
$settings = [ordered]@{
    schemaVersion = [int]$buildIdentity.SettingsSchemaVersion
    persistence = [ordered]@{
        autosaveIntervalSeconds = 60
        majorChangeDebounceSeconds = 120
    }
}
$primarySettings = Join-Path $primaryPartisan 'HST_Settings.json'
Write-JsonUtf8NoBom $primarySettings $settings
$settingsEvidence = Join-Path $runRoot 'configuration\HST_Settings.json'
$settingsSignature = Copy-GateStableFile $primarySettings $settingsEvidence
$primaryOwnerPath = Join-Path $primaryDebug (
    'HST_OrdinaryCampaignPersistenceProof_{0}.owner.json' -f $runId)
$primaryOwner = New-EngineOwnerValue `
    -SessionNonce $nonce `
    -RunId $runId `
    -PayloadNonce $payloadNonce `
    -BuildIdentity $buildIdentity `
    -World $worldResource
Write-JsonUtf8NoBom $primaryOwnerPath $primaryOwner
$null = Assert-EngineOwner (Read-JsonArtifact $primaryOwnerPath) `
    $nonce $runId $payloadNonce $buildIdentity $worldResource

$diagnosticRuntimeRows = New-Object Collections.Generic.List[object]
$standardRuntimeRows = New-Object Collections.Generic.List[object]
$persistenceRows = New-Object Collections.Generic.List[object]
$autosave = Invoke-GateRuntimeStage 0 $script:GateStages[0] $runRoot `
    $diagnosticGuardBase $primaryProfileRoot $fallbackProfileRoot $clientProfileRoot `
    $sessionRoot $nonce $payloadNonce $runId $serverCandidate `
    $serverCandidate.ExecutablePath $serverCandidate.DiagnosticExecutable `
    $clientCandidate.ExecutablePath $clientCandidate.DiagnosticExecutable `
    $buildIdentity $worldResource '' '' '' '' $WatchedRoots $SpillRoots `
    $LoopbackPort
[void]$diagnosticRuntimeRows.Add($autosave.Runtime)
[void]$persistenceRows.Add($autosave.Persistence)
$u0 = [string]$autosave.Result.m_sCreatedSavePointId
$f1 = [string]$autosave.Result.m_sProfileFallbackReadBackFingerprint

$manual = Invoke-GateRuntimeStage 1 $script:GateStages[1] $runRoot `
    $diagnosticGuardBase $primaryProfileRoot $fallbackProfileRoot $clientProfileRoot `
    $sessionRoot $nonce $payloadNonce $runId $serverCandidate `
    $serverCandidate.ExecutablePath $serverCandidate.DiagnosticExecutable `
    $clientCandidate.ExecutablePath $clientCandidate.DiagnosticExecutable `
    $buildIdentity $worldResource $u0 $f1 '' $u0 $WatchedRoots $SpillRoots `
    $LoopbackPort
[void]$diagnosticRuntimeRows.Add($manual.Runtime)
[void]$persistenceRows.Add($manual.Persistence)
$u1 = [string]$manual.Result.m_sCreatedSavePointId
$f2 = [string]$manual.Result.m_sProfileFallbackReadBackFingerprint

$shutdown = Invoke-GateRuntimeStage 2 $script:GateStages[2] $runRoot `
    $diagnosticGuardBase $primaryProfileRoot $fallbackProfileRoot $clientProfileRoot `
    $sessionRoot $nonce $payloadNonce $runId $serverCandidate `
    $serverCandidate.ExecutablePath $serverCandidate.DiagnosticExecutable `
    $clientCandidate.ExecutablePath $clientCandidate.DiagnosticExecutable `
    $buildIdentity $worldResource $u1 $f2 '' $u1 $WatchedRoots $SpillRoots `
    $LoopbackPort
[void]$diagnosticRuntimeRows.Add($shutdown.Runtime)
[void]$persistenceRows.Add($shutdown.Persistence)
$u2 = [string]$shutdown.Result.m_sCreatedSavePointId
$f3 = [string]$shutdown.Result.m_sProfileFallbackReadBackFingerprint
$s3 = [string]$shutdown.Result.m_sSentinelFingerprint

$native = Invoke-GateRuntimeStage 3 $script:GateStages[3] $runRoot `
    $diagnosticGuardBase $primaryProfileRoot $fallbackProfileRoot $clientProfileRoot `
    $sessionRoot $nonce $payloadNonce $runId $serverCandidate `
    $serverCandidate.ExecutablePath $serverCandidate.DiagnosticExecutable `
    $clientCandidate.ExecutablePath $clientCandidate.DiagnosticExecutable `
    $buildIdentity $worldResource $u2 $f3 $s3 $u2 $WatchedRoots $SpillRoots `
    $LoopbackPort
[void]$diagnosticRuntimeRows.Add($native.Runtime)
[void]$persistenceRows.Add($native.Persistence)

New-Item -ItemType Directory -Path $fallbackDebug -Force | Out-Null
$fallbackOwnerPath = Join-Path $fallbackDebug (
    'HST_OrdinaryCampaignPersistenceProof_{0}.owner.json' -f $runId)
$fallbackCarrierPath = Join-Path $fallbackDebug (
    'HST_OrdinaryCampaignPersistenceProof_{0}.carrier.json' -f $runId)
$primaryCarrierPath = Join-Path $primaryDebug (
    'HST_OrdinaryCampaignPersistenceProof_{0}.carrier.json' -f $runId)
$fallbackOwner = New-EngineOwnerValue $nonce $runId $payloadNonce `
    $buildIdentity $worldResource
Write-JsonUtf8NoBom $fallbackOwnerPath $fallbackOwner
$primaryCanonical = Join-Path $primaryPartisan 'HST_CampaignSaveData.json'
$primaryRecovery = Join-Path $primaryPartisan 'HST_CampaignSaveData.recovery.json'
$fallbackCanonical = Join-Path $fallbackPartisan 'HST_CampaignSaveData.json'
$fallbackRecovery = Join-Path $fallbackPartisan `
    'HST_CampaignSaveData.recovery.json'
Copy-Item -LiteralPath $primaryCanonical -Destination $fallbackCanonical
Copy-Item -LiteralPath $primaryRecovery -Destination $fallbackRecovery
Copy-Item -LiteralPath $primaryCarrierPath -Destination $fallbackCarrierPath
Assert-FallbackProfilePrelaunchFiles $fallbackProfileRoot @(
    $fallbackCanonical, $fallbackRecovery, $fallbackOwnerPath,
    $fallbackCarrierPath)

$fallback = Invoke-GateRuntimeStage 4 $script:GateStages[4] $runRoot `
    $diagnosticGuardBase $primaryProfileRoot $fallbackProfileRoot $clientProfileRoot `
    $sessionRoot $nonce $payloadNonce $runId $serverCandidate `
    $serverCandidate.ExecutablePath $serverCandidate.DiagnosticExecutable `
    $clientCandidate.ExecutablePath $clientCandidate.DiagnosticExecutable `
    $buildIdentity $worldResource '' $f3 $s3 $u2 $WatchedRoots $SpillRoots `
    $LoopbackPort
[void]$diagnosticRuntimeRows.Add($fallback.Runtime)
[void]$persistenceRows.Add($fallback.Persistence)

$standardLoadIds = @($u0, $u1, $u2, $u2, '')
for ($standardOrdinal = 0; $standardOrdinal -lt 5; $standardOrdinal++) {
    $sourcePortable = [string]$persistenceRows[$standardOrdinal].
        outputSnapshotPath
    $sourceSnapshot = Join-Path $runRoot $sourcePortable.Replace('/', '\')
    $standard = Invoke-GateStandardRetentionContext `
        -Ordinal $standardOrdinal `
        -Stage $script:GateStages[$standardOrdinal] `
        -RunRoot $runRoot `
        -GuardBase $standardGuardBase `
        -SessionRoot $sessionRoot `
        -SourceSnapshotPath $sourceSnapshot `
        -LoadSavePointId ([string]$standardLoadIds[$standardOrdinal]) `
        -LaunchClient ($standardOrdinal -eq 2) `
        -Candidate $serverCandidate `
        -ServerExecutable $serverExecutable `
        -ServerProvenance $serverProvenance `
        -ClientExecutablePath $clientExecutablePath `
        -ClientProvenance $clientProvenance `
        -SettingsEvidencePath $settingsEvidence `
        -WatchedRoots $WatchedRoots `
        -SpillRoots $SpillRoots `
        -LoopbackPort $LoopbackPort `
        -ReadinessSeconds $StandardReadinessSeconds
    [void]$standardRuntimeRows.Add($standard)
}

$launchContract = [ordered]@{
    schemaVersion = 1
    contractId = $script:GateContractId
    runId = $runId
    sessionNonce = $nonce
    payloadNonce = $payloadNonce
    buildIdentity = $buildIdentity
    worldResource = $worldResource
    missionHeader = $script:MissionHeader
    projectId = $script:ProjectId
    stages = [object[]]@($persistenceRows.ToArray() | ForEach-Object {
        [ordered]@{
            ordinal = [int]$_.ordinal
            stage = [string]$_.stage
            stageNonce = [string]$_.stageNonce
            loadSavePointId = [string]$_.loadSavePointId
            latestSavePointId = [string]$_.latestSavePointId
        }
    })
}
$launchContractPath = Join-Path $runRoot 'configuration\launch-contract.json'
$launchContractSignature = Write-GateJson $launchContractPath $launchContract `
    -CreateOnly

$sessionOwner = Read-JsonArtifact $sessionOwnerPath
if ([string]$sessionOwner.magic -cne
        'partisan_gate1_runtime_retention_owner_v1' -or
    [string]$sessionOwner.runId -cne $runId -or
    [string]$sessionOwner.nonce -cne $nonce -or
    -not (Test-PartisanContainedPath $runRoot $sessionRoot)) {
    throw 'Disposable session ownership could not be re-established.'
}
if (@(Get-Process | Where-Object {
        $_.ProcessName -in @(
            'ArmaReforger', 'ArmaReforgerSteam', 'ArmaReforgerServer',
            'ArmaReforgerSteamDiag', 'ArmaReforgerServerDiag',
            'ArmaReforgerWorkbenchSteamDiag')
    }).Count -ne 0) {
    throw 'Disposable session removal requires an engine-free boundary.'
}
Remove-GateSessionRoot -SessionRoot $sessionRoot

$toolRows = @(
    [ordered]@{ role = 'gate1-runner'; path = 'tools/run-guarded-gate1-runtime-retention.ps1' },
    [ordered]@{ role = 'release-candidate-module'; path = 'tools/Partisan.ReleaseCandidate.psm1' },
    [ordered]@{ role = 'guarded-runtime-module'; path = 'tools/Partisan.GuardedRuntime.psm1' },
    [ordered]@{ role = 'ordinary-persistence-library'; path = 'tools/run-ordinary-campaign-persistence-proof.ps1' },
    [ordered]@{ role = 'gate1-index-producer'; path = 'tools/New-PartisanGate1RuntimeRetentionIndex.ps1' },
    [ordered]@{ role = 'gate1-evidence-consumer'; path = 'tools/Partisan.Gate1EvidenceConsumer.psm1' },
    [ordered]@{ role = 'release-docs-consumer'; path = 'tools/update-release-docs.ps1' }
)
$toolEvidence = @($toolRows | ForEach-Object {
    $path = Join-Path $repositoryRoot ([string]$_.path).Replace('/', '\')
    $signature = Get-GateFileSignature $path
    [pscustomobject][ordered]@{
        role = [string]$_.role
        path = [string]$_.path
        length = [long]$signature.length
        sha256 = [string]$signature.sha256
    }
})

$payloadRows = New-Object Collections.Generic.List[object]
foreach ($file in @(Get-ChildItem -LiteralPath $runRoot -Recurse -File -Force |
        Sort-Object FullName)) {
    $portable = ConvertTo-GatePortablePath $runRoot $file.FullName
    if ($portable -in @('run.json', 'release-index.json', 'run.ready.json')) {
        continue
    }
    $signature = Get-GateFileSignature $file.FullName
    [void]$payloadRows.Add([pscustomobject][ordered]@{
        role = Get-GatePayloadRole $portable
        stage = Get-GatePayloadStage $portable
        path = $portable
        length = [long]$signature.length
        sha256 = [string]$signature.sha256
    })
}
$completedUtc = [DateTime]::UtcNow
$candidatePublic = Get-PartisanPublicCandidateIdentity $serverCandidate
$run = [ordered]@{
    schemaVersion = 1
    evidenceKind = $script:GateEvidenceKind
    contractId = $script:GateContractId
    runId = $runId
    startedUtc = $startedUtc.ToString('o')
    completedUtc = $completedUtc.ToString('o')
    candidate = [ordered]@{
        candidateId = [string]$candidatePublic.candidateId
        candidateVersion = [string]$candidatePublic.candidateVersion
        runtimeUseDisposition = [string]$candidatePublic.runtimeUseDisposition
        gitHead = [string]$candidatePublic.gitHead
        embeddedBuildSha = [string]$candidatePublic.embeddedBuildSha
        embeddedBuildUtc = [string]$candidatePublic.embeddedBuildUtc
        embeddedBuildLabel = [string]$candidatePublic.embeddedBuildLabel
        campaignSchema = [int]$candidatePublic.campaignSchema
        runtimeSettingsSchema = [int]$candidatePublic.runtimeSettingsSchema
        addonId = [string]$candidatePublic.addonId
        addonGuid = [string]$candidatePublic.addonGuid
        packageHashAlgorithm = [string]$candidatePublic.packageHashAlgorithm
        packageSha256 = [string]$candidatePublic.packageSha256
        manifestSha256 = [string]$candidatePublic.manifestSha256
        readySha256 = [string]$candidatePublic.readySha256
        workbenchCrc = [string]$candidatePublic.workbenchCrc
        manifestPath = 'identity/candidate.json'
        readyPath = 'identity/candidate.ready.json'
        packageRoot = 'identity/package/Partisan'
        packageFiles = [object[]]$packageEvidenceRows.ToArray()
        executables = [ordered]@{
            serverDiagnostic = $serverCandidate.DiagnosticExecutable
            clientDiagnostic = $clientCandidate.DiagnosticExecutable
            server = $serverProvenance
            client = $clientProvenance
        }
    }
    harness = [ordered]@{
        gitHead = $gitHead
        clean = $true
        tools = [object[]]$toolEvidence
    }
    scenario = [ordered]@{
        executionMode = 'two-phase-engine'
        worldResource = $worldResource
        missionHeader = $script:MissionHeader
        stages = [string[]]$script:GateStages
        claimScope = 'raw-retention-only'
        certificationClaim = 'none'
    }
    configuration = [ordered]@{
        settingsPath = 'configuration/HST_Settings.json'
        settingsSha256 = [string]$settingsSignature.sha256
        launchContractPath = 'configuration/launch-contract.json'
        launchContractSha256 = [string]$launchContractSignature.sha256
        autosaveIntervalSeconds = 60
        majorChangeDebounceSeconds = 120
    }
    lineage = [ordered]@{
        executionClass = 'diagnostic-only-save-lineage'
        retailClaim = 'none'
        contexts = [object[]]$diagnosticRuntimeRows.ToArray()
    }
    runtime = [ordered]@{
        executionClass = 'standard-load-start-log-retention'
        mutationAuthority = 'none'
        byteStabilityClaim = 'observation-only'
        contexts = [object[]]$standardRuntimeRows.ToArray()
    }
    persistence = [ordered]@{ stages = [object[]]$persistenceRows.ToArray() }
    outcome = [ordered]@{
        success = $true
        diagnosticServerLaunchCount = 5
        diagnosticClientLaunchCount = 1
        standardServerLaunchCount = 5
        standardClientLaunchCount = 1
        configsRetained = $true
        logsRetained = $true
        saveBytesRetained = $true
        guardedReceiptsComplete = $true
        standardSaveBytesStable = $true
        disposition = 'passed-noncertifying-retention'
    }
    files = [object[]]@($payloadRows.ToArray() | Sort-Object path)
}
$runPath = Join-Path $runRoot 'run.json'
$runSignature = Write-GateJson $runPath $run -CreateOnly
$indexResult = & $indexPath -RunEnvelopePath $runPath
$releaseIndexPath = Join-Path $runRoot 'release-index.json'
if (-not (Test-Path -LiteralPath $releaseIndexPath -PathType Leaf)) {
    throw 'The Gate 1 index producer did not publish its fixed output.'
}
$indexSignature = Get-GateFileSignature $releaseIndexPath
$ready = [ordered]@{
    schemaVersion = 1
    contractId = 'partisan.gate1-runtime-retention.ready.v1'
    evidenceKind = $script:GateEvidenceKind
    runId = $runId
    candidateId = [string]$serverCandidate.CandidateId
    run = [ordered]@{
        path = 'run.json'
        length = [long]$runSignature.length
        sha256 = [string]$runSignature.sha256
    }
    index = [ordered]@{
        path = 'release-index.json'
        length = [long]$indexSignature.length
        sha256 = [string]$indexSignature.sha256
    }
    disposition = 'passed-noncertifying-retention'
    publishedUtc = [DateTime]::UtcNow.ToString('o')
}
$readyPath = Join-Path $runRoot 'run.ready.json'
$readySignature = Write-GateJson $readyPath $ready -Atomic -CreateOnly
Write-Output ([pscustomobject][ordered]@{
    Success = $true
    RunId = $runId
    CandidateId = [string]$serverCandidate.CandidateId
    RunRoot = $runRoot
    RunSignature = $runSignature
    IndexSignature = $indexSignature
    ReadySignature = $readySignature
    Disposition = 'passed-noncertifying-retention'
    IndexResult = $indexResult
})
