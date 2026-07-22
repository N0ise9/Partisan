[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$producerPath = Join-Path $PSScriptRoot `
    'New-PartisanReleaseSurfaceAuditIndex.ps1'
$modulePath = Join-Path $PSScriptRoot 'Partisan.GuardedRuntime.psm1'
$contractSourcePath = Join-Path (Split-Path -Parent $PSScriptRoot) `
    'docs\data\release_surface_contract.json'
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
Import-Module $modulePath -Force -ErrorAction Stop

function Assert-TestCondition {
    param([bool]$Condition, [string]$Label)
    if (-not $Condition) {
        throw "Release-surface index self-test failed: $Label."
    }
}

function Assert-TestRejected {
    param([string]$Label, [scriptblock]$Action, [string]$Pattern)

    $rejected = $false
    try { & $Action | Out-Null }
    catch {
        if (-not [string]::IsNullOrWhiteSpace($Pattern) -and
            $_.Exception.Message -notmatch $Pattern) {
            throw "Release-surface rejection '$Label' had an unexpected error: $($_.Exception.Message)"
        }
        $rejected = $true
    }
    if (-not $rejected) {
        throw "Release-surface self-test expected rejection: $Label."
    }
}

function Get-TestSha256Bytes {
    param([byte[]]$Bytes)

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally { $algorithm.Dispose() }
}

function Get-TestSha256Text {
    param([AllowEmptyString()][string]$Text)

    return Get-TestSha256Bytes `
        ((New-Object Text.UTF8Encoding($false)).GetBytes($Text))
}

function Get-TestSignature {
    param([string]$Path)

    $item = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    return [pscustomobject][ordered]@{
        length = [long]$item.Length
        sha256 = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).
            Hash.ToLowerInvariant()
    }
}

function Write-TestJson {
    param([string]$Path, $Value)

    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    [IO.File]::WriteAllText(
        $Path,
        (($Value | ConvertTo-Json -Depth 64) + "`n"),
        (New-Object Text.UTF8Encoding($false)))
    return Get-TestSignature $Path
}

function Write-TestText {
    param([string]$Path, [AllowEmptyString()][string]$Text)

    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    [IO.File]::WriteAllText(
        $Path, $Text, (New-Object Text.UTF8Encoding($false)))
    return Get-TestSignature $Path
}

function Read-TestJson {
    param([string]$Path)
    return [IO.File]::ReadAllText($Path) | ConvertFrom-Json
}

function Get-TestFreeUdpPort {
    $socket = New-Object Net.Sockets.Socket(
        [Net.Sockets.AddressFamily]::InterNetwork,
        [Net.Sockets.SocketType]::Dgram,
        [Net.Sockets.ProtocolType]::Udp)
    try {
        $socket.ExclusiveAddressUse = $true
        $socket.Bind((New-Object Net.IPEndPoint([Net.IPAddress]::Loopback, 0)))
        return [int]$socket.LocalEndPoint.Port
    }
    finally { $socket.Dispose() }
}

function ConvertTo-TestPortablePath {
    param([string]$Root, [string]$Path)

    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $fullPath = [IO.Path]::GetFullPath($Path)
    $prefix = $rootPath + [IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith(
            $prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'A synthetic release-surface path escaped its run root.'
    }
    return $fullPath.Substring($prefix.Length).Replace('\', '/')
}

function Get-TestRowsDigest {
    param([object[]]$Rows)

    $lines = @($Rows | Sort-Object path | ForEach-Object {
        "{0}`t{1}`t{2}" -f $_.sha256, [long]$_.length, [string]$_.path
    })
    return Get-TestSha256Text (($lines -join "`n") + "`n")
}

function New-TestTreeBinding {
    param([string]$Root)

    $rows = @(Get-ChildItem -LiteralPath $Root -Recurse -File -Force |
        Sort-Object FullName | ForEach-Object {
            $signature = Get-TestSignature $_.FullName
            [pscustomobject][ordered]@{
                path = ConvertTo-TestPortablePath $Root $_.FullName
                length = [long]$signature.length
                sha256 = [string]$signature.sha256
            }
        })
    return [pscustomobject][ordered]@{
        files = [object[]]$rows
        aggregateSha256 = Get-TestRowsDigest $rows
    }
}

function New-TestEvidenceIndex {
    param([string]$RunRoot)

    $excluded = @(
        'evidence-index.json', 'run.json', 'release-index.json',
        'run.ready.json', 'run.failure.json')
    $rows = @(Get-ChildItem -LiteralPath $RunRoot -Recurse -File -Force |
        Sort-Object FullName | ForEach-Object {
            $relative = ConvertTo-TestPortablePath $RunRoot $_.FullName
            if ($relative -notin $excluded) {
                $signature = Get-TestSignature $_.FullName
                [pscustomobject][ordered]@{
                    path = $relative
                    length = [long]$signature.length
                    sha256 = [string]$signature.sha256
                }
            }
        })
    return [ordered]@{
        schemaVersion = 1
        evidenceKind = 'partisan_release_surface_runtime_audit_v2'
        files = [object[]]$rows
        aggregateSha256 = Get-TestRowsDigest $rows
    }
}

function Get-TestToolRows {
    $root = Split-Path -Parent $PSScriptRoot
    $paths = [ordered]@{
        runner = 'tools/run-guarded-release-surface-audit.ps1'
        releaseIndexProducer = 'tools/New-PartisanReleaseSurfaceAuditIndex.ps1'
        releaseCandidateModule = 'tools/Partisan.ReleaseCandidate.psm1'
        guardedRuntimeModule = 'tools/Partisan.GuardedRuntime.psm1'
        gate1EvidenceConsumer = 'tools/Partisan.Gate1EvidenceConsumer.psm1'
        releaseDocsConsumer = 'tools/update-release-docs.ps1'
        contract = 'docs/data/release_surface_contract.json'
        harnessProjectTemplate =
            'tools/release-surface-audit-harness-template/addon.gproj.template'
        harnessSourceTemplate =
            'tools/release-surface-audit-harness-template/Scripts/Game/PartisanReleaseSurfaceAudit.c.template'
    }
    return [object[]]@($paths.GetEnumerator() | ForEach-Object {
        $signature = Get-TestSignature (Join-Path $root ([string]$_.Value))
        [pscustomobject][ordered]@{
            role = [string]$_.Key
            path = [string]$_.Value
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        }
    })
}

function Invoke-TestFixtureValidation {
    param([string]$RunPath)
    return @(& $producerPath `
        -RunEnvelopePath $RunPath `
        -SyntheticFixtureValidationOnly)
}

function Invoke-TestPublisher {
    param([string]$RunPath)
    return @(& $producerPath -RunEnvelopePath $RunPath)
}

function Invoke-TestPublishedVerification {
    param([string]$RunPath)
    return @(& $producerPath `
        -RunEnvelopePath $RunPath `
        -SyntheticFixtureValidationOnly `
        -VerifyPublishedIndex)
}

function New-TestExpectedPublishedIndex {
    param([string]$RunRoot)

    $runPath = Join-Path $RunRoot 'run.json'
    $run = Read-TestJson $runPath
    $bindings = Read-TestJson (Join-Path $RunRoot 'identity\bindings.json')
    $contractPath = Join-Path $RunRoot 'identity\release_surface_contract.json'
    $contract = Read-TestJson $contractPath
    $evidencePath = Join-Path $RunRoot 'evidence-index.json'
    $evidence = Read-TestJson $evidencePath
    $evidenceSignature = Get-TestSignature $evidencePath
    $runSignature = Get-TestSignature $runPath
    $modes = New-Object Collections.Generic.List[object]
    foreach ($mode in @('retail', 'diagnostic')) {
        $binding = @($run.modes | Where-Object {
            [string]$_.mode -ceq $mode
        })[0]
        $modeValue = Read-TestJson (Join-Path $RunRoot `
            ([string]$binding.path).Replace('/', '\'))
        [void]$modes.Add([pscustomobject][ordered]@{
            mode = $mode
            path = [string]$binding.path
            signature = $binding.signature
            executable = $modeValue.executable
            contextId = [string]$modeValue.process.contextId
            candidateBindingSha256 =
                [string]$modeValue.process.candidateBindingSha256
            forbiddenTypeCount = @($contract.forbiddenTypeNames).Count
            productionTypeCount =
                @($contract.productionPositiveControlTypeNames).Count
            forbiddenCommandCount = @($contract.forbiddenCommandActionIds).Count
            productionCommandCount =
                @($contract.productionPositiveControlCommandActionIds).Count
            forbiddenMemberCount = @($contract.forbiddenMemberSurfaces).Count
            productionMemberCount =
                @($contract.productionObservabilityMemberSurfaces).Count
            hardDiagnosticPolicy =
                [string]$modeValue.classification.hardDiagnosticPolicy
            hardDiagnosticFree = [bool]$modeValue.classification.hardDiagnosticFree
            hardDiagnosticRawLineCount =
                [int]$modeValue.classification.hardDiagnosticRawLineCount
            hardDiagnosticEventCount =
                [int]$modeValue.classification.hardDiagnosticEventCount
            approvedStockDiagnosticClusterPresent =
                [bool]$modeValue.classification.
                    approvedStockDiagnosticClusterPresent
            approvedStockDiagnosticClusterExact =
                [bool]$modeValue.classification.
                    approvedStockDiagnosticClusterExact
            approvedStockDiagnosticLifecycleExact =
                [bool]$modeValue.classification.
                    approvedStockDiagnosticLifecycleExact
            approvedStockDiagnosticRawLineCount =
                [int]$modeValue.classification.
                    approvedStockDiagnosticRawLineCount
            approvedStockDiagnosticEventCount =
                [int]$modeValue.classification.
                    approvedStockDiagnosticEventCount
            unapprovedHardDiagnosticRawLineCount =
                [int]$modeValue.classification.
                    unapprovedHardDiagnosticRawLineCount
            unapprovedHardDiagnosticEventCount =
                [int]$modeValue.classification.
                    unapprovedHardDiagnosticEventCount
            hardDiagnosticAccountingExact =
                [bool]$modeValue.classification.hardDiagnosticAccountingExact
            candidateMountLineCount =
                [int]$modeValue.classification.candidateMountLineCount
            candidatePackedMountLineCount =
                [int]$modeValue.classification.candidatePackedMountLineCount
            harnessMountLineCount =
                [int]$modeValue.classification.harnessMountLineCount
            uniqueResultMarkerCount =
                [int]$modeValue.classification.uniqueResultMarkerCount
            resultMarkerOccurrenceCount =
                [int]$modeValue.classification.resultMarkerOccurrenceCount
            crashLogContentValid =
                [bool]$modeValue.classification.crashLogContentValid
            crashArtifactCount = [int]$modeValue.classification.crashArtifactCount
            passed = $true
        })
    }
    return [ordered]@{
        schemaVersion = 2
        contractId = 'partisan.release-surface-audit.index.v2'
        evidenceKind = 'partisan_release_surface_runtime_audit_index_v2'
        runId = [string]$run.runId
        runLeafId = [string]$run.runLeafId
        runNonce = [string]$run.runNonce
        source = [ordered]@{
            bundleRelativePath = [string]$run.candidate.candidateId +
                '/release-surface-audit/' + [string]$run.runLeafId
            candidateGitHead = [string]$run.candidate.gitHead
            harnessGitHead = [string]$run.source.harnessGitHead
            checkoutClean = $true
            candidateAncestor = $true
            executionMode = [string]$run.source.executionMode
        }
        candidate = $run.candidate
        candidateBindingSha256 = [string]$run.candidateBindingSha256
        run = [ordered]@{
            path = 'run.json'
            length = [long]$runSignature.length
            sha256 = [string]$runSignature.sha256
        }
        evidenceIndex = [ordered]@{
            path = 'evidence-index.json'
            length = [long]$evidenceSignature.length
            sha256 = [string]$evidenceSignature.sha256
            fileCount = @($evidence.files).Count
            filesAggregateSha256 = [string]$evidence.aggregateSha256
        }
        contract = [ordered]@{
            path = 'identity/release_surface_contract.json'
            length = [long](Get-TestSignature $contractPath).length
            sha256 = [string](Get-TestSignature $contractPath).sha256
            forbiddenTypeCount = @($contract.forbiddenTypeNames).Count
            forbiddenCommandCount = @($contract.forbiddenCommandActionIds).Count
            productionTypeCount =
                @($contract.productionPositiveControlTypeNames).Count
            productionCommandCount =
                @($contract.productionPositiveControlCommandActionIds).Count
            forbiddenMemberSurfaceCount = @($contract.forbiddenMemberSurfaces).Count
            forbiddenLiteralSurfaceCount = @($contract.forbiddenLiteralSurfaces).Count
            productionObservabilityMemberSurfaceCount =
                @($contract.productionObservabilityMemberSurfaces).Count
        }
        harness = [ordered]@{
            id = [string]$bindings.harness.id
            guid = [string]$bindings.harness.guid
            aggregateSha256 = [string]$bindings.harness.aggregateSha256
            files = [object[]]$bindings.harness.files
            tools = [object[]]$bindings.tools
        }
        capture = [ordered]@{
            startedUtc = [string]$run.startedUtc
            completedUtc = [string]$run.completedUtc
            modeCount = $modes.Count
            pairedOrder = [string[]]@('retail', 'diagnostic')
        }
        modes = [object[]]$modes.ToArray()
        cleanup = $run.cleanup
        validation = [ordered]@{
            candidateTupleExact = $true
            packageCanonicalDigestExact = $true
            executableProvenanceExact = $true
            harnessSourceAndToolsExact = $true
            guardedReceiptsComplete = $true
            candidateBindingShared = $true
            pairedModeOrderExact = $true
            contractSetsExact = $true
            positiveControlsPresent = $true
            hardDiagnosticAccountingExact = $true
            approvedStockDiagnosticClustersExact = $true
            unapprovedHardDiagnosticsAbsent = $true
            crashArtifactsAbsent = $true
            harnessResidueAbsent = $true
            portablePathsExact = $true
            duplicateJsonKeysAbsent = $true
            fullFileCensusExact = $true
        }
        files = [object[]]$evidence.files
        filesAggregateSha256 = [string]$evidence.aggregateSha256
        limitations = [string[]]$run.limitations
        disposition = 'passed-noncertifying-release-surface-audit'
        certificationPromotion = 'none'
        passed = $true
    }
}

function Write-TestPublishedSeals {
    param([string]$RunRoot)

    $runPath = Join-Path $RunRoot 'run.json'
    $run = Read-TestJson $runPath
    $indexPath = Join-Path $RunRoot 'release-index.json'
    $indexSignature = Write-TestJson $indexPath `
        (New-TestExpectedPublishedIndex $RunRoot)
    $ready = [ordered]@{
        schemaVersion = 2
        evidenceKind = 'partisan_release_surface_runtime_audit_v2'
        disposition = 'passed-noncertifying-release-surface-audit'
        certificationPromotion = 'none'
        runId = [string]$run.runId
        runLeafId = [string]$run.runLeafId
        source = [ordered]@{
            candidateGitHead = [string]$run.candidate.gitHead
            harnessGitHead = [string]$run.source.harnessGitHead
        }
        candidateId = [string]$run.candidate.candidateId
        packageSha256 = [string]$run.candidate.packageSha256
        manifestSha256 = [string]$run.candidate.manifestSha256
        readySha256 = [string]$run.candidate.readySha256
        candidate = $run.candidate
        candidateBindingSha256 = [string]$run.candidateBindingSha256
        run = [ordered]@{
            path = 'run.json'
            signature = Get-TestSignature $runPath
        }
        evidenceIndex = [ordered]@{
            path = 'evidence-index.json'
            signature = Get-TestSignature `
                (Join-Path $RunRoot 'evidence-index.json')
        }
        releaseIndex = [ordered]@{
            path = 'release-index.json'
            signature = $indexSignature
        }
        cleanupPassed = $true
        sealedLast = $true
    }
    return Write-TestJson (Join-Path $RunRoot 'run.ready.json') $ready
}

function Get-TestTreeStateDigest {
    param([string]$RunRoot)

    $lines = @(Get-ChildItem -LiteralPath $RunRoot -Recurse -File -Force |
        Sort-Object FullName | ForEach-Object {
            $signature = Get-TestSignature $_.FullName
            '{0}`t{1}`t{2}`t{3}' -f `
                (ConvertTo-TestPortablePath $RunRoot $_.FullName),
                [long]$signature.length,
                [string]$signature.sha256,
                [long]$_.LastWriteTimeUtc.Ticks
        })
    return Get-TestSha256Text (($lines -join "`n") + "`n")
}

function Sync-TestBundleSeals {
    param([string]$RunRoot)

    $runPath = Join-Path $RunRoot 'run.json'
    $run = Read-TestJson $runPath
    for ($index = 0; $index -lt 2; $index++) {
        $mode = @('retail', 'diagnostic')[$index]
        $modePath = Join-Path $RunRoot ('modes\' + $mode + '.json')
        $modeValue = Read-TestJson $modePath
        $probePath = Join-Path $RunRoot `
            ([string]$modeValue.probe.path).Replace('/', '\')
        $modeValue.probe.signature = Get-TestSignature $probePath
        foreach ($row in @($modeValue.classification.logs)) {
            $logPath = Join-Path $RunRoot ([string]$row.path).Replace('/', '\')
            $signature = Get-TestSignature $logPath
            $row.length = [long]$signature.length
            $row.sha256 = [string]$signature.sha256
        }
        $signature = Write-TestJson $modePath $modeValue
        $binding = @($run.modes | Where-Object {
            [string]$_.mode -ceq $mode
        })
        if ($binding.Count -eq 1) { $binding[0].signature = $signature }
    }
    $indexPath = Join-Path $RunRoot 'evidence-index.json'
    $indexValue = New-TestEvidenceIndex $RunRoot
    $indexSignature = Write-TestJson $indexPath $indexValue
    $run.evidenceIndex.signature = $indexSignature
    $run.evidenceIndex.aggregateSha256 = [string]$indexValue.aggregateSha256
    $run.evidenceIndex.fileCount = @($indexValue.files).Count
    $null = Write-TestJson $runPath $run
}

function Sync-TestModeArgumentMutation {
    param(
        [string]$RunRoot,
        [ValidateSet('retail', 'diagnostic')][string]$Mode,
        $RawArguments,
        $PortableArguments
    )

    $rawPath = Join-Path $RunRoot ('raw\' + $Mode + '\arguments.raw.json')
    $portablePath = Join-Path $RunRoot `
        ('raw\' + $Mode + '\arguments.portable.json')
    $rawSignature = Write-TestJson $rawPath $RawArguments
    $PortableArguments.rawArgumentsSha256 = [string]$rawSignature.sha256
    $null = Write-TestJson $portablePath $PortableArguments
    Sync-TestBundleSeals $RunRoot
}

function Get-TestExactArgumentPosition {
    param([object[]]$Arguments, [string]$Value)

    $positions = @()
    for ($index = 0; $index -lt $Arguments.Count; $index++) {
        if ([string]$Arguments[$index] -ceq $Value) { $positions += $index }
    }
    if ($positions.Count -ne 1) {
        throw "Synthetic arguments do not contain one exact $Value token."
    }
    return [int]$positions[0]
}

function Get-TestBaseline {
    param([string]$RunRoot)

    $result = @{}
    foreach ($file in @(Get-ChildItem -LiteralPath $RunRoot -Recurse -File -Force)) {
        $relative = ConvertTo-TestPortablePath $RunRoot $file.FullName
        $result[$relative] = [IO.File]::ReadAllBytes($file.FullName)
    }
    return $result
}

function Restore-TestBaseline {
    param([string]$RunRoot, [hashtable]$Baseline)

    foreach ($file in @(Get-ChildItem -LiteralPath $RunRoot -Recurse -File -Force)) {
        Remove-Item -LiteralPath $file.FullName -Force -ErrorAction Stop
    }
    foreach ($relative in $Baseline.Keys) {
        $path = Join-Path $RunRoot ([string]$relative).Replace('/', '\')
        $parent = Split-Path -Parent $path
        if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
            New-Item -ItemType Directory -Path $parent -Force | Out-Null
        }
        [IO.File]::WriteAllBytes($path, [byte[]]$Baseline[$relative])
    }
}

function Remove-TestRootExact {
    param([string]$Path)

    $base = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/')
    $full = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    if (-not $full.StartsWith(
            $base + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase) -or
        (Split-Path -Leaf $full) -cnotmatch
            '^PartisanReleaseSurfaceIndexSelfTest_[0-9a-f]{32}$') {
        throw 'Release-surface index self-test cleanup escaped its exact root.'
    }
    foreach ($item in @(Get-ChildItem -LiteralPath $full -Recurse -Force)) {
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'Release-surface index self-test cleanup found a reparse point.'
        }
    }
    Remove-Item -LiteralPath $full -Recurse -Force -ErrorAction Stop
    if (Test-Path -LiteralPath $full) {
        throw 'Release-surface index self-test cleanup did not converge.'
    }
}

function Get-TestMemberProbePlan {
    param(
        $Contract,
        [string]$GitHead,
        [string]$SourceRoot = $repositoryRoot)

    $requests = @(
        @($Contract.forbiddenMemberSurfaces | ForEach-Object {
            [pscustomobject]@{ category = 'forbidden'; surface = [string]$_ }
        }) +
        @($Contract.productionObservabilityMemberSurfaces | ForEach-Object {
            [pscustomobject]@{ category = 'production'; surface = [string]$_ }
        }))
    $requests = @($requests | ForEach-Object {
        if ([string]$_.surface -cnotmatch
            '^(?<path>Scripts/Game/HST/[A-Za-z0-9_./-]+\.c)::' +
            '(?<member>[A-Za-z_][A-Za-z0-9_]*)$') {
            throw 'Synthetic member-probe surface is unsafe.'
        }
        [pscustomobject][ordered]@{
            category = [string]$_.category
            surface = [string]$_.surface
            path = [string]$Matches.path
            memberName = [string]$Matches.member
        }
    })
    $methodPattern = [regex]::new(
        '^\s*(?!return\b)' +
        '(?:(?:protected|private|static|override|event|proto|sealed|ref|autoptr)\s+)*' +
        '(?:[A-Za-z_][A-Za-z0-9_<>\[\],.]*\s+)+' +
        '(?<member>[A-Za-z_][A-Za-z0-9_]*)\s*\(')
    $fieldPattern = [regex]::new(
        '^\s*(?!return\b)' +
        '(?:(?:protected|private|static|const|ref|autoptr)\s+)*' +
        '(?:[A-Za-z_][A-Za-z0-9_<>\[\],.]*\s+)+' +
        '(?<member>[A-Za-z_][A-Za-z0-9_]*)\s*(?:[;=])')
    $classPattern = [regex]::new(
        '^\s*(?:(?:modded|sealed)\s+)*class\s+' +
        '(?<type>[A-Za-z_][A-Za-z0-9_]*)')
    $guardedSignaturePattern = [regex]::new(
        '^[ \t]*(?<access>protected)[ \t]+' +
        '(?<returnType>bool)[ \t]+' +
        '(?<methodName>StartMission_S)[ \t]*\([ \t]*' +
        '(?<firstParameterType>string)[ \t]+' +
        '(?<firstParameterName>missionId)[ \t]*,[ \t]*' +
        '(?<secondParameterType>string)[ \t]+' +
        '(?<secondParameterName>targetZoneId)[ \t]*\n' +
        '[ \t]*#ifdef[ \t]+(?<diagnosticSymbol>ENABLE_DIAG)[ \t]*\n' +
        '[ \t]*,[ \t]*(?<parameterType>bool)[ \t]+' +
        '(?<parameterName>forceDebug)[ \t]*=[ \t]*' +
        '(?<defaultValue>false)[ \t]*\n' +
        '[ \t]*#endif[ \t]*\n[ \t]*\)',
        [Text.RegularExpressions.RegexOptions]::CultureInvariant -bor
            [Text.RegularExpressions.RegexOptions]::Multiline)
    $resolved = New-Object Collections.Generic.List[object]
    foreach ($group in @($requests | Group-Object path)) {
        $sourcePath = Join-Path $SourceRoot `
            ([string]$group.Name).Replace('/', '\')
        $lines = @([IO.File]::ReadAllLines($sourcePath))
        if ($lines.Count -eq 0) {
            throw 'Synthetic member-probe candidate source is absent.'
        }
        $currentType = ''
        foreach ($line in $lines) {
            $classMatch = $classPattern.Match($line)
            if ($classMatch.Success) {
                $currentType = [string]$classMatch.Groups['type'].Value
            }
            $match = $methodPattern.Match($line)
            $kind = 'method'
            if (-not $match.Success) {
                $match = $fieldPattern.Match($line)
                if ($match.Success) {
                    $kind = if ($line -cmatch '(^|\s)const\s') {
                        'constant'
                    }
                    else { 'field' }
                }
            }
            if (-not $match.Success) {
                continue
            }
            $member = [string]$match.Groups['member'].Value
            foreach ($request in @($group.Group | Where-Object {
                [string]$_.memberName -cne 'forceDebug' -and
                [string]$_.memberName -ceq $member
            })) {
                [void]$resolved.Add([pscustomobject][ordered]@{
                    category = [string]$request.category
                    surface = [string]$request.surface
                    declaringType = $currentType
                    memberName = $member
                    probeKind = $kind
                })
            }
        }

        $guardedRequests = @($group.Group | Where-Object {
            [string]$_.memberName -ceq 'forceDebug'
        })
        if ($guardedRequests.Count -gt 0) {
            $signatureSurface =
                'Scripts/Game/HST/Components/' +
                'HST_CampaignCoordinatorComponent.c::forceDebug'
            if ($guardedRequests.Count -ne 1 -or
                [string]$guardedRequests[0].category -cne 'forbidden' -or
                [string]$guardedRequests[0].surface -cne $signatureSurface -or
                [string]$group.Name -cne
                    'Scripts/Game/HST/Components/' +
                    'HST_CampaignCoordinatorComponent.c') {
                throw 'Synthetic guarded member-signature request is invalid.'
            }
            $sourceText = $lines -join "`n"
            $signatureMatches = @($guardedSignaturePattern.Matches($sourceText))
            if ($signatureMatches.Count -ne 1) {
                throw 'Synthetic guarded member signature is absent or ambiguous.'
            }
            $signatureMatch = $signatureMatches[0]
            $signatureLine = @($sourceText.Substring(
                0,
                $signatureMatch.Index).Split("`n")).Count
            $signatureOwner = ''
            for ($sourceIndex = 0;
                $sourceIndex -lt $signatureLine;
                $sourceIndex++) {
                $ownerMatch = $classPattern.Match($lines[$sourceIndex])
                if ($ownerMatch.Success) {
                    $signatureOwner =
                        [string]$ownerMatch.Groups['type'].Value
                }
            }
            if ($signatureOwner -cne 'HST_CampaignCoordinatorComponent' -or
                [string]$guardedRequests[0].memberName -cne
                    [string]$signatureMatch.Groups['parameterName'].Value) {
                throw 'Synthetic guarded member signature binding is invalid.'
            }
            [void]$resolved.Add([pscustomobject][ordered]@{
                category = [string]$guardedRequests[0].category
                surface = [string]$guardedRequests[0].surface
                declaringType = $signatureOwner
                memberName =
                    [string]$signatureMatch.Groups['parameterName'].Value
                probeKind = 'methodSignature'
            })
        }
    }
    $rows = New-Object Collections.Generic.List[object]
    foreach ($request in $requests) {
        $matches = @($resolved.ToArray() | Where-Object {
            [string]$_.category -ceq [string]$request.category -and
            [string]$_.surface -ceq [string]$request.surface
        })
        if ($matches.Count -ne 1 -and
            [string]$request.surface -ceq
                'Scripts/Game/HST/UI/HST_ActionDialogController.c::m_sDebugOwner' -and
            $matches.Count -eq 2) {
            $matches = @($matches[0])
        }
        if ($matches.Count -ne 1) {
            throw "Synthetic member-probe source resolution is ambiguous for $($request.surface): $($matches.Count)."
        }
        [void]$rows.Add($matches[0])
    }
    return [pscustomobject][ordered]@{
        forbidden = [object[]]@($rows.ToArray() | Where-Object {
            [string]$_.category -ceq 'forbidden'
        })
        production = [object[]]@($rows.ToArray() | Where-Object {
            [string]$_.category -ceq 'production'
        })
    }
}

function New-TestModeEvidence {
    param(
        [ValidateSet('retail', 'diagnostic')][string]$Mode,
        [string]$RunRoot,
        [string]$RunNonce,
        $CandidateStageIdentity,
        $CandidatePublicIdentity,
        [string]$RuntimeAddon,
        [string]$Watched,
        [string]$Spill,
        [string]$Executable,
        $ExecutableProvenance,
        $Contract,
        $MemberProbePlan,
        [string]$HarnessGuid,
        [string]$HarnessSha256
    )

    $modeRoot = Join-Path $RunRoot ('raw\' + $Mode)
    $guardBase = Join-Path $modeRoot 'guarded-runtime'
    $working = Join-Path $modeRoot 'working'
    $profile = Join-Path $modeRoot 'profile'
    $logs = Join-Path $modeRoot 'logs'
    $addonTemp = Join-Path $modeRoot 'addon-temp'
    foreach ($directory in @(
            $modeRoot, $guardBase, $working, $profile, $logs, $addonTemp)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }

    $context = New-PartisanGuardedRuntimeContext `
        -GuardBase $guardBase `
        -Purpose ('self_test_release_surface_' + $Mode) `
        -WatchedRoots @($Watched) `
        -SpillRoots @($Spill) `
        -LoopbackPorts @((Get-TestFreeUdpPort))
    $stage = New-PartisanCandidateStage $context $CandidateStageIdentity
    $arguments = [string[]]@(
        '-gproj', [string]$stage.PackedProjectPath,
        '-server', 'Worlds/HST_Dev/HST_Dev.ent',
        '-MissionHeader', 'Missions/HST_Dev.conf',
        '-addonsDir', [string]$stage.AddonSearchPath,
        '-addons', ([string]$CandidatePublicIdentity.addonGuid + ',' + $HarnessGuid),
        '-profile', $profile,
        '-logsDir', $logs,
        '-addonTempDir', $addonTemp,
        '-logLevel', 'normal',
        '-logTime', 'datetime',
        '-noThrow',
        '-maxFPS', '30')
    if ($Mode -ceq 'diagnostic') {
        $arguments += @('-scrDefine', 'ENABLE_DIAG')
    }
    $arguments += @(
        '-releaseSurfaceRunNonce', $RunNonce,
        '-releaseSurfaceExpectedMode', $Mode,
        '-hstReleaseCandidateId', [string]$CandidatePublicIdentity.candidateId,
        '-hstReleasePackageSha256',
            [string]$CandidatePublicIdentity.packageSha256,
        '-hstReleaseManifestSha256',
            [string]$CandidatePublicIdentity.manifestSha256)
    $rawSignature = Write-TestJson `
        (Join-Path $modeRoot 'arguments.raw.json') ([ordered]@{
            schemaVersion = 1
            mode = $Mode
            executable = $Executable
            arguments = $arguments
        })
    $portableArguments = [string[]]@(
        '-gproj', '<candidate-stage>/Partisan/addon.gproj',
        '-server', 'Worlds/HST_Dev/HST_Dev.ent',
        '-MissionHeader', 'Missions/HST_Dev.conf',
        '-addonsDir', '<runtime-addons>,<candidate-stage>',
        '-addons', ([string]$CandidatePublicIdentity.addonGuid + ',' + $HarnessGuid),
        '-profile', ('<run>/raw/' + $Mode + '/profile'),
        '-logsDir', ('<run>/raw/' + $Mode + '/logs'),
        '-addonTempDir', ('<run>/raw/' + $Mode + '/addon-temp'),
        '-logLevel', 'normal',
        '-logTime', 'datetime',
        '-noThrow',
        '-maxFPS', '30')
    if ($Mode -ceq 'diagnostic') {
        $portableArguments += @('-scrDefine', 'ENABLE_DIAG')
    }
    $portableArguments += @(
        '-releaseSurfaceRunNonce', $RunNonce,
        '-releaseSurfaceExpectedMode', $Mode,
        '-hstReleaseCandidateId', [string]$CandidatePublicIdentity.candidateId,
        '-hstReleasePackageSha256',
            [string]$CandidatePublicIdentity.packageSha256,
        '-hstReleaseManifestSha256',
            [string]$CandidatePublicIdentity.manifestSha256)
    $null = Write-TestJson `
        (Join-Path $modeRoot 'arguments.portable.json') ([ordered]@{
            schemaVersion = 1
            mode = $Mode
            executable = '<runtime>/' + (Split-Path -Leaf $Executable)
            arguments = $portableArguments
            rawArgumentsSha256 = [string]$rawSignature.sha256
        })
    $null = Write-TestText (Join-Path $modeRoot 'stdout.raw.txt') ''
    $null = Write-TestText (Join-Path $modeRoot 'stderr.raw.txt') ''
    $null = Write-TestJson `
        (Join-Path $modeRoot 'stream-capture.json') ([ordered]@{
            schemaVersion = 1
            mode = $Mode
            captureMode = 'guarded-native-no-inherited-handles'
            stdoutPath = 'stdout.raw.txt'
            stdoutBytes = 0
            stderrPath = 'stderr.raw.txt'
            stderrBytes = 0
            engineLogsAreAuthoritative = $true
        })

    $null = Start-PartisanGuardedServer `
        -Context $context `
        -Executable $Executable `
        -ExecutableProvenance $ExecutableProvenance `
        -Arguments $arguments `
        -WorkingDirectory $working `
        -CandidateConsumption $stage `
        -NonEngineSelfTestOnly
    $teardown = Invoke-PartisanGuardedTeardown $context
    $tested = Test-PartisanGuardedRuntimeReceipt `
        $teardown.CleanReceiptPath $teardown.CleanReceiptSignature
    Assert-TestCondition $tested.Complete "$Mode genuine guarded receipt"

    $present = $Mode -ceq 'diagnostic'
    $probe = [ordered]@{
        schemaVersion = 1
        evidenceKind = 'partisan_release_surface_runtime_probe_v1'
        runNonce = $RunNonce
        mode = $Mode
        expectedMode = $Mode
        candidateId = [string]$CandidatePublicIdentity.candidateId
        packageSha256 = [string]$CandidatePublicIdentity.packageSha256
        manifestSha256 = [string]$CandidatePublicIdentity.manifestSha256
        cliIdentityPresent = $true
        modeSentinelExact = $true
        retailSentinelPresent = $Mode -ceq 'retail'
        diagnosticSentinelPresent = $Mode -ceq 'diagnostic'
        forbiddenTypeExpectedPresent = $present
        runtimeCompilerAlwaysPublicCompiled = $true
        runtimeCompilerAlwaysProtectedCompiled = $true
        runtimeCompilerImpossibleMemberRejected = $true
        runtimeCompilerDiagnosticPublicCompiled = $present
        runtimeCompilerDiagnosticProtectedCompiled = $present
        runtimeMetadataAlwaysFieldPresent = $true
        runtimeMetadataDiagnosticFieldPresent = $present
        runtimeCompilerAvailable = $true
        runtimeMetadataAvailable = $true
        forbiddenTypes = @($Contract.forbiddenTypeNames | ForEach-Object {
            [ordered]@{ name = [string]$_; present = $present }
        })
        productionTypes = @(
            $Contract.productionPositiveControlTypeNames | ForEach-Object {
                [ordered]@{ name = [string]$_; present = $true }
            })
        forbiddenMemberExpectedPresent = $present
        forbiddenMembers = @($MemberProbePlan.forbidden | ForEach-Object {
            [ordered]@{
                surface = [string]$_.surface
                declaringType = [string]$_.declaringType
                memberName = [string]$_.memberName
                probeKind = [string]$_.probeKind
                probeSupported = $true
                present = $present
            }
        })
        productionMembers = @($MemberProbePlan.production | ForEach-Object {
            [ordered]@{
                surface = [string]$_.surface
                declaringType = [string]$_.declaringType
                memberName = [string]$_.memberName
                probeKind = [string]$_.probeKind
                probeSupported = $true
                present = $true
            }
        })
        forbiddenCommandExpectedPresent = $present
        forbiddenCommands = @(
            $Contract.forbiddenCommandActionIds | ForEach-Object {
                [ordered]@{
                    id = [string]$_
                    generatedPresent = $present
                    routingPresent = $present
                }
            })
        productionCommands = @(
            $Contract.productionPositiveControlCommandActionIds |
                ForEach-Object {
                    [ordered]@{
                        id = [string]$_
                        generatedPresent = $true
                        routingPresent = $true
                    }
                })
        mismatchCount = 0
        passed = $true
    }
    $probePath = Join-Path $profile 'Partisan\probe.result.json'
    $probeSignature = Write-TestJson $probePath $probe

    $logRoot = Join-Path $logs 'session'
    $logRows = New-Object Collections.Generic.List[object]
    $logLeaves = @('console.log', 'script.log', 'error.log')
    if ($Mode -ceq 'diagnostic') { $logLeaves += 'crash.log' }
    $resultLine =
        '2026-07-20 17:00:00.100 SCRIPT : ' +
        'Partisan release surface audit | RESULT | mode=' + $Mode +
        ' | passed=true | mismatches=0'
    $replicationFinishingLine =
        '2026-07-20 17:00:00.200 RPL : Replication finishing...'
    $replicationFinishedLine =
        '2026-07-20 17:00:00.300 RPL : Replication finished.'
    $gameDestroyedLine =
        '2026-07-20 17:00:00.600 ENGINE : Game destroyed.'
    $stockLineA =
        "2026-07-20 17:00:00.400 SCRIPT (E): " +
        "'SCR_BaseResupplySupportStationComponent' needs a entity catalog manager!"
    $stockLineB =
        "2026-07-20 17:00:00.500 SCRIPT (E): " +
        "'SCR_BaseResupplySupportStationComponent' needs a entity catalog manager!"
    $stockClusterPresent = $Mode -ceq 'retail'
    foreach ($leaf in $logLeaves) {
        $text = if ($leaf -ceq 'console.log') {
            $consoleLines = @(
                ('AUDIT addon ' + [string]$CandidatePublicIdentity.addonGuid +
                    ' (packed)')
                ('AUDIT harness ' + $HarnessGuid)
                $resultLine
                $replicationFinishingLine
                $replicationFinishedLine)
            if ($stockClusterPresent) {
                $consoleLines += @($stockLineA, $stockLineB)
            }
            $consoleLines += $gameDestroyedLine
            ($consoleLines -join "`n") + "`n"
        }
        elseif ($leaf -ceq 'script.log') {
            $scriptLines = @($resultLine)
            if ($stockClusterPresent) {
                $scriptLines += @($stockLineA, $stockLineB)
            }
            ($scriptLines -join "`n") + "`n"
        }
        elseif ($leaf -ceq 'error.log' -and $stockClusterPresent) {
            (@($stockLineA, $stockLineB) -join "`n") + "`n"
        }
        else { '' }
        $path = Join-Path $logRoot $leaf
        $signature = Write-TestText $path $text
        [void]$logRows.Add([pscustomobject][ordered]@{
            leaf = $leaf
            path = ConvertTo-TestPortablePath $RunRoot $path
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        })
    }

    $modeValue = [ordered]@{
        schemaVersion = 2
        evidenceKind = 'partisan_release_surface_runtime_audit_v2'
        mode = $Mode
        disposition = 'passed-noncertifying-release-surface-audit'
        candidateId = [string]$CandidatePublicIdentity.candidateId
        packageSha256 = [string]$CandidatePublicIdentity.packageSha256
        manifestSha256 = [string]$CandidatePublicIdentity.manifestSha256
        readySha256 = [string]$CandidatePublicIdentity.readySha256
        executable = $ExecutableProvenance
        harnessGuid = $HarnessGuid
        harnessSha256 = $HarnessSha256
        process = [ordered]@{
            exitCode = 0
            contextId = [string]$tested.ContextId
            candidateBindingSha256 = [string]$tested.CandidateBindingSha256
            receiptPath = ConvertTo-TestPortablePath `
                $RunRoot $teardown.CleanReceiptPath
            receiptSignature = $teardown.CleanReceiptSignature
            journalPath = ConvertTo-TestPortablePath $RunRoot $tested.JournalPath
            journalSignature = $tested.JournalSignature
            completionPath = ConvertTo-TestPortablePath `
                $RunRoot $tested.CompletionAttestationPath
            completionSignature = $tested.CompletionAttestationSignature
        }
        arguments = [ordered]@{
            raw = 'raw/' + $Mode + '/arguments.raw.json'
            portable = 'raw/' + $Mode + '/arguments.portable.json'
        }
        streams = [ordered]@{
            stdout = 'raw/' + $Mode + '/stdout.raw.txt'
            stderr = 'raw/' + $Mode + '/stderr.raw.txt'
            capture = 'raw/' + $Mode + '/stream-capture.json'
        }
        probe = [ordered]@{
            path = ConvertTo-TestPortablePath $RunRoot $probePath
            signature = $probeSignature
            summary = [ordered]@{
                mode = $Mode
                forbiddenTypeCount = @($Contract.forbiddenTypeNames).Count
                productionTypeCount =
                    @($Contract.productionPositiveControlTypeNames).Count
                forbiddenCommandCount =
                    @($Contract.forbiddenCommandActionIds).Count
                productionCommandCount =
                    @($Contract.productionPositiveControlCommandActionIds).Count
                forbiddenMemberCount = @($MemberProbePlan.forbidden).Count
                productionMemberCount = @($MemberProbePlan.production).Count
                runtimeCompilerAvailable = $true
                runtimeMetadataAvailable = $true
                passed = $true
            }
        }
        classification = [ordered]@{
            valid = $true
            hardDiagnosticPolicy = 'script-engine-and-process-fatal-v1'
            hardDiagnosticFree = -not $stockClusterPresent
            hardDiagnosticRawLineCount = if ($stockClusterPresent) { 6 } else { 0 }
            hardDiagnosticEventCount = if ($stockClusterPresent) { 2 } else { 0 }
            approvedStockDiagnosticClusterPresent = $stockClusterPresent
            approvedStockDiagnosticClusterExact = $true
            approvedStockDiagnosticLifecycleExact = $true
            approvedStockDiagnosticRawLineCount =
                if ($stockClusterPresent) { 6 } else { 0 }
            approvedStockDiagnosticEventCount =
                if ($stockClusterPresent) { 2 } else { 0 }
            unapprovedHardDiagnosticRawLineCount = 0
            unapprovedHardDiagnosticEventCount = 0
            hardDiagnosticAccountingExact = $true
            candidateMountLineCount = 1
            candidatePackedMountLineCount = 1
            harnessMountLineCount = 1
            uniqueResultMarkerCount = 1
            resultMarkerOccurrenceCount = 2
            crashLogContentValid = $true
            crashArtifactCount = 0
            logs = [object[]]$logRows.ToArray()
        }
        passed = $true
    }
    $modePath = Join-Path $RunRoot ('modes\' + $Mode + '.json')
    $modeSignature = Write-TestJson $modePath $modeValue
    return [pscustomobject][ordered]@{
        mode = $Mode
        path = 'modes/' + $Mode + '.json'
        signature = $modeSignature
        candidateBindingSha256 = [string]$tested.CandidateBindingSha256
    }
}

$tempRoot = Join-Path ([IO.Path]::GetTempPath()) (
    'PartisanReleaseSurfaceIndexSelfTest_' + [Guid]::NewGuid().ToString('N'))
$testFailure = $null
$cleanupFailure = $null
$checks = New-Object Collections.Generic.List[string]
New-Item -ItemType Directory -Path $tempRoot -ErrorAction Stop | Out-Null

try {
    $candidateId = 'partisan-release-surface-synthetic'
    $runNonce = [Guid]::NewGuid().ToString('N')
    $runLeaf = '20260720T000000Z-' + $runNonce
    $runId = 'release_surface_20260720T000000Z_' +
        $runNonce.Substring(0, 20)
    $runRoot = Join-Path (Join-Path (Join-Path (Join-Path $tempRoot `
        'evidence') $candidateId) 'release-surface-audit') $runLeaf
    $candidateSource = Join-Path $tempRoot 'candidate-source'
    $runtimeAddon = Join-Path $tempRoot 'runtime-addons'
    $watched = Join-Path $tempRoot 'watched'
    $spill = Join-Path $tempRoot 'spill'
    $runtimeBin = Join-Path $tempRoot 'runtime-bin'
    foreach ($directory in @(
            $runRoot,
            (Join-Path $runRoot 'identity'),
            (Join-Path $runRoot 'harness\source\Scripts\Game'),
            (Join-Path $runRoot 'modes'),
            (Join-Path $runRoot 'raw'),
            $candidateSource, $runtimeAddon, $watched, $spill, $runtimeBin)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }

    $compiledPath = Join-Path $runtimeBin 'SyntheticReleaseSurfaceHost.exe'
    Add-Type -TypeDefinition @'
using System;
using System.Reflection;
using System.Threading;
[assembly: AssemblyVersion("1.0.0.0")]
[assembly: AssemblyFileVersion("1.0.0.0")]
public static class SyntheticReleaseSurfaceHost {
    public static int Main(string[] args) {
        Thread.Sleep(TimeSpan.FromSeconds(30));
        return 0;
    }
}
'@ -Language CSharp -OutputAssembly $compiledPath -OutputType ConsoleApplication
    $retailExecutable = Join-Path $runtimeBin 'ArmaReforgerServer.exe'
    $diagnosticExecutable = Join-Path $runtimeBin 'ArmaReforgerServerDiag.exe'
    Copy-Item -LiteralPath $compiledPath -Destination $retailExecutable
    Copy-Item -LiteralPath $compiledPath -Destination $diagnosticExecutable
    $sleeperProvenance = Get-PartisanExecutableProvenance $compiledPath
    $retailProvenance = Get-PartisanExecutableProvenance $retailExecutable
    $diagnosticProvenance = Get-PartisanExecutableProvenance $diagnosticExecutable

    $gitPathBeforeResolutionTest = $env:Path
    try {
        $gitApplicationRootA = Join-Path $tempRoot 'git-application-a'
        $gitApplicationRootB = Join-Path $tempRoot 'git-application-b'
        New-Item -ItemType Directory -Path @(
            $gitApplicationRootA, $gitApplicationRootB) -Force | Out-Null
        $gitApplicationPathA = Join-Path $gitApplicationRootA 'git.exe'
        $gitApplicationPathB = Join-Path $gitApplicationRootB 'git.exe'
        Copy-Item -LiteralPath $compiledPath -Destination $gitApplicationPathA
        Copy-Item -LiteralPath $compiledPath -Destination $gitApplicationPathB
        $env:Path = $gitApplicationRootA + [IO.Path]::PathSeparator +
            $gitApplicationRootB + [IO.Path]::PathSeparator +
            $gitPathBeforeResolutionTest

        $tokens = $null
        $parseErrors = $null
        $producerAst = [Management.Automation.Language.Parser]::ParseFile(
            $producerPath, [ref]$tokens, [ref]$parseErrors)
        Assert-TestCondition (@($parseErrors).Count -eq 0) `
            'producer parses for Git-resolution regression'
        $gitSignatureFunctions = @($producerAst.FindAll({
            param($node)
            $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
                $node.Name -ceq 'Get-ReleaseSurfaceIndexGitBlobSignature'
        }, $true))
        Assert-TestCondition ($gitSignatureFunctions.Count -eq 1) `
            'Git blob signature function is unique'
        $gitAssignments = @($gitSignatureFunctions[0].Body.FindAll({
            param($node)
            $node -is [Management.Automation.Language.AssignmentStatementAst] -and
                $node.Left.Extent.Text -ceq '$gitCommand'
        }, $true))
        Assert-TestCondition ($gitAssignments.Count -eq 1) `
            'Git application assignment is unique'
        $gitResolution = [scriptblock]::Create(
            $gitAssignments[0].Right.Extent.Text)
        $availableGitApplications = @(
            Get-Command git -CommandType Application -ErrorAction Stop)
        $resolvedGitApplications = @(& $gitResolution)
        Assert-TestCondition (
            $availableGitApplications.Count -ge 2 -and
            [string]$availableGitApplications[0].Source -ceq
                $gitApplicationPathA -and
            [string]$availableGitApplications[1].Source -ceq
                $gitApplicationPathB -and
            $resolvedGitApplications.Count -eq 1 -and
            [string]$resolvedGitApplications[0].Source -ceq
                $gitApplicationPathA) `
            'multiple Git application matches resolve to one executable'
        [void]$checks.Add('multiple-git-application-resolution-scalar')
    }
    finally { $env:Path = $gitPathBeforeResolutionTest }

    $gitBoundLfPaths = @(
        @((Get-TestToolRows) | ForEach-Object { [string]$_.path }) +
        @(
            'tools/run-guarded-gate1-runtime-retention.ps1',
            'tools/run-ordinary-campaign-persistence-proof.ps1',
            'tools/New-PartisanGate1RuntimeRetentionIndex.ps1') |
            Sort-Object -Unique -CaseSensitive)
    foreach ($portablePath in $gitBoundLfPaths) {
        $attributes = @(& git -C $repositoryRoot check-attr `
            text eol -- $portablePath)
        $attributeExit = $LASTEXITCODE
        Assert-TestCondition (
            $attributeExit -eq 0 -and
            $attributes.Count -eq 2 -and
            $attributes -ccontains ($portablePath + ': text: set') -and
            $attributes -ccontains ($portablePath + ': eol: lf')) `
            ('Git-bound LF attributes are exact for ' + $portablePath)
        $boundText = [IO.File]::ReadAllText(
            (Join-Path $repositoryRoot $portablePath.Replace('/', '\')))
        Assert-TestCondition (
            $boundText.IndexOf("`r", [StringComparison]::Ordinal) -lt 0) `
            ('Git-bound worktree bytes contain no CR for ' + $portablePath)
    }
    [void]$checks.Add('git-bound-tool-lf-worktree-contract')

    $stageRows = New-Object Collections.Generic.List[object]
    $manifestRows = New-Object Collections.Generic.List[object]
    foreach ($entry in @(
            [pscustomobject]@{
                Name = 'addon.gproj'
                Text = '{"ProjectName":"Partisan"}'
            },
            [pscustomobject]@{ Name = 'data.pak'; Text = 'synthetic-data-pak' },
            [pscustomobject]@{
                Name = 'resourceDatabase.rdb'
                Text = 'synthetic-resource-database'
            },
            [pscustomobject]@{ Name = 'thumbnail.png'; Text = 'synthetic-thumbnail' })) {
        $path = Join-Path $candidateSource $entry.Name
        $signature = Write-TestText $path ([string]$entry.Text)
        $indexPath = 'Partisan/' + [string]$entry.Name
        [void]$stageRows.Add([pscustomobject][ordered]@{
            indexPath = $indexPath
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        })
        [void]$manifestRows.Add([pscustomobject][ordered]@{
            path = 'package/' + $indexPath
            indexPath = $indexPath
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        })
    }
    $packageDigestRows = @($stageRows.ToArray() | ForEach-Object {
        [pscustomobject][ordered]@{
            path = [string]$_.indexPath
            length = [long]$_.length
            sha256 = [string]$_.sha256
        }
    })
    $packageSha256 = Get-TestRowsDigest $packageDigestRows
    $gitHead = (@(& git -C $repositoryRoot rev-parse HEAD) -join '').Trim()
    if ($LASTEXITCODE -ne 0 -or $gitHead -cnotmatch '^[0-9a-f]{40}$') {
        throw 'Release-surface index self-test could not resolve its source commit.'
    }
    $embeddedBuildSha = '2' * 40
    $addonGuid = '698532771130111D'
    $harnessGuid = 'A1234567890BCDEF'

    $manifestValue = [ordered]@{
        manifestSchemaVersion = 1
        candidate = [ordered]@{
            id = $candidateId
            version = '0.0.0-self-test'
            state = 'retained-uncertified'
        }
        source = [ordered]@{
            gitHead = $gitHead
            embeddedImplementation = [ordered]@{
                sha = $embeddedBuildSha
                utc = '2026-07-20T00:00:00Z'
                label = 'synthetic-self-test'
            }
            campaignSchema = 71
            runtimeSettingsSchema = 24
        }
        addon = [ordered]@{ id = 'Partisan'; guid = $addonGuid }
        toolchain = [ordered]@{
            server = $retailProvenance
            client = $retailProvenance
            serverDiagnostic = $diagnosticProvenance
            clientDiagnostic = $diagnosticProvenance
        }
        workbench = [ordered]@{ crc = '00000000' }
        package = [ordered]@{
            root = 'package/Partisan'
            hashAlgorithm = 'sha256-manifest-v1'
            sha256 = $packageSha256
            canonicalIndexPath = 'evidence/pack/files.sha256'
            files = [object[]]$manifestRows.ToArray()
        }
        evidence = [ordered]@{ root = 'evidence'; files = @() }
    }
    $manifestPath = Join-Path $runRoot 'identity\candidate.json'
    $manifestSignature = Write-TestJson $manifestPath $manifestValue
    $readyValue = [ordered]@{
        schemaVersion = 1
        candidateId = $candidateId
        gitHead = $gitHead
        packageSha256 = $packageSha256
        manifestSha256 = [string]$manifestSignature.sha256
    }
    $readyPath = Join-Path $runRoot 'identity\candidate.ready.json'
    $readySignature = Write-TestJson $readyPath $readyValue
    $candidatePublic = [pscustomobject][ordered]@{
        candidateId = $candidateId
        candidateVersion = '0.0.0-self-test'
        runtimeUseDisposition = 'active-runtime-candidate'
        gitHead = $gitHead
        embeddedBuildSha = $embeddedBuildSha
        embeddedBuildUtc = '2026-07-20T00:00:00Z'
        embeddedBuildLabel = 'synthetic-self-test'
        campaignSchema = 71
        runtimeSettingsSchema = 24
        addonId = 'Partisan'
        addonGuid = $addonGuid
        packageHashAlgorithm = 'sha256-manifest-v1'
        packageSha256 = $packageSha256
        manifestSha256 = [string]$manifestSignature.sha256
        readySha256 = [string]$readySignature.sha256
        workbenchCrc = '00000000'
        runtimeRole = 'server'
        diagnosticExecutable = $diagnosticProvenance
        recordedDiagnosticExecutable = $diagnosticProvenance
        recordedRuntimeExecutable = $retailProvenance
    }
    $candidateStage = [pscustomobject][ordered]@{
        CandidateId = $candidateId
        PackageSha256 = $packageSha256
        PackageFiles = [object[]]$stageRows.ToArray()
        PackedAddonPath = $candidateSource
        RuntimeAddonRootPath = $runtimeAddon
    }

    Copy-Item -LiteralPath $contractSourcePath `
        -Destination (Join-Path $runRoot `
            'identity\release_surface_contract.json')
    $contract = Read-TestJson $contractSourcePath
    $mutatedSourceRoot = Join-Path $tempRoot 'guarded-signature-mutation'
    $memberSourcePaths = @(
        @($contract.forbiddenMemberSurfaces) +
        @($contract.productionObservabilityMemberSurfaces) |
            ForEach-Object { ([string]$_ -split '::', 2)[0] } |
            Sort-Object -Unique -CaseSensitive)
    foreach ($portableSourcePath in $memberSourcePaths) {
        $sourcePath = Join-Path $repositoryRoot `
            $portableSourcePath.Replace('/', '\')
        $destinationPath = Join-Path $mutatedSourceRoot `
            $portableSourcePath.Replace('/', '\')
        $destinationParent = Split-Path -Parent $destinationPath
        if (-not (Test-Path -LiteralPath $destinationParent `
                -PathType Container)) {
            New-Item -ItemType Directory -Path $destinationParent -Force |
                Out-Null
        }
        Copy-Item -LiteralPath $sourcePath -Destination $destinationPath
    }
    $copiedProbePlan = Get-TestMemberProbePlan `
        -Contract $contract `
        -GitHead $gitHead `
        -SourceRoot $mutatedSourceRoot
    Assert-TestCondition (
        @($copiedProbePlan.forbidden).Count -eq
            [int]$contract.expectedForbiddenMemberSurfaceCount -and
        @($copiedProbePlan.production).Count -eq
            [int]$contract.expectedProductionObservabilityMemberSurfaceCount) `
        'copied candidate member source resolves exactly'
    $guardedSourcePath = Join-Path $mutatedSourceRoot `
        'Scripts\Game\HST\Components\HST_CampaignCoordinatorComponent.c'
    $guardedSourceText = [IO.File]::ReadAllText($guardedSourcePath)
    $guardedNeedle = ', bool forceDebug = false'
    Assert-TestCondition (
        @([regex]::Matches(
                $guardedSourceText,
                [regex]::Escape($guardedNeedle))).Count -eq 1) `
        'guarded parameter mutation fixture is exact'
    $null = Write-TestText `
        $guardedSourcePath `
        $guardedSourceText.Replace(
            $guardedNeedle,
            ', bool renamedDebug = false')
    Assert-TestRejected 'renamed guarded parameter' {
        Get-TestMemberProbePlan `
            -Contract $contract `
            -GitHead $gitHead `
            -SourceRoot $mutatedSourceRoot
    } 'guarded member signature'
    [void]$checks.Add('guarded-parameter-rename-rejected')
    $memberProbePlan = Get-TestMemberProbePlan `
        -Contract $contract `
        -GitHead $gitHead
    $null = Write-TestText `
        (Join-Path $runRoot 'harness\source\addon.gproj') `
        "synthetic release-surface project`n"
    $null = Write-TestText `
        (Join-Path $runRoot `
            'harness\source\Scripts\Game\PartisanReleaseSurfaceAudit.c') `
        "class PartisanReleaseSurfaceAuditSynthetic {}`n"
    $null = Write-TestJson `
        (Join-Path $runRoot `
            'harness\source\.partisan-release-surface-audit-owner.json') `
        ([ordered]@{
            schemaVersion = 1
            evidenceKind = 'partisan_release_surface_harness_owner_v1'
            runNonce = $runNonce
            harnessId = 'PartisanReleaseSurfaceAudit'
            harnessGuid = $harnessGuid
            candidateGuid = $addonGuid
        })
    $harnessBinding = New-TestTreeBinding `
        (Join-Path $runRoot 'harness\source')
    $toolRows = Get-TestToolRows
    $bindingsValue = [ordered]@{
        schemaVersion = 1
        evidenceKind = 'partisan_release_surface_runtime_audit_v2'
        source = [ordered]@{
            harnessGitHead = $gitHead
            checkoutClean = $true
            candidateAncestor = $true
            executionMode = 'synthetic-self-test'
        }
        candidate = $candidatePublic
        package = [ordered]@{
            hashAlgorithm = 'sha256-manifest-v1'
            sha256 = $packageSha256
            files = [object[]]$manifestRows.ToArray()
        }
        executables = [ordered]@{
            retail = $retailProvenance
            diagnostic = $diagnosticProvenance
        }
        harness = [ordered]@{
            id = 'PartisanReleaseSurfaceAudit'
            guid = $harnessGuid
            candidateDependencyGuid = $addonGuid
            constructionMode = 'external-disposable-runtime-loaded-audit-addon'
            aggregateSha256 = [string]$harnessBinding.aggregateSha256
            files = [object[]]$harnessBinding.files
        }
        tools = $toolRows
        host = [ordered]@{
            powershellVersion = $PSVersionTable.PSVersion.ToString()
            powershellEdition = [string]$PSVersionTable.PSEdition
            operatingSystem = [Environment]::OSVersion.VersionString
        }
    }
    $null = Write-TestJson `
        (Join-Path $runRoot 'identity\bindings.json') $bindingsValue
    $null = Write-TestJson (Join-Path $runRoot 'run.owner.json') ([ordered]@{
        schemaVersion = 1
        evidenceKind = 'partisan_release_surface_runtime_audit_v2'
        runId = $runId
        runNonce = $runNonce
        candidateId = $candidateId
        packageSha256 = $packageSha256
        harnessGuid = $harnessGuid
        harnessSha256 = [string]$harnessBinding.aggregateSha256
        disposition = 'in-progress-noncertifying-release-surface-audit'
    })

    $retail = New-TestModeEvidence `
        -Mode retail -RunRoot $runRoot -RunNonce $runNonce `
        -CandidateStageIdentity $candidateStage `
        -CandidatePublicIdentity $candidatePublic `
        -RuntimeAddon $runtimeAddon -Watched $watched -Spill $spill `
        -Executable $compiledPath `
        -ExecutableProvenance $sleeperProvenance `
        -Contract $contract -MemberProbePlan $memberProbePlan `
        -HarnessGuid $harnessGuid `
        -HarnessSha256 ([string]$harnessBinding.aggregateSha256)
    $diagnostic = New-TestModeEvidence `
        -Mode diagnostic -RunRoot $runRoot -RunNonce $runNonce `
        -CandidateStageIdentity $candidateStage `
        -CandidatePublicIdentity $candidatePublic `
        -RuntimeAddon $runtimeAddon -Watched $watched -Spill $spill `
        -Executable $compiledPath `
        -ExecutableProvenance $sleeperProvenance `
        -Contract $contract -MemberProbePlan $memberProbePlan `
        -HarnessGuid $harnessGuid `
        -HarnessSha256 ([string]$harnessBinding.aggregateSha256)
    Assert-TestCondition (
        [string]$retail.candidateBindingSha256 -ceq
            [string]$diagnostic.candidateBindingSha256) `
        'paired synthetic contexts share one candidate binding'
    $retailModeFixture = Read-TestJson (Join-Path $runRoot 'modes\retail.json')
    $diagnosticModeFixture = Read-TestJson `
        (Join-Path $runRoot 'modes\diagnostic.json')
    Assert-TestCondition (
        @($retailModeFixture.classification.logs).Count -eq 3 -and
        [string]$retailModeFixture.classification.hardDiagnosticPolicy -ceq
            'script-engine-and-process-fatal-v1' -and
        [bool]$retailModeFixture.classification.crashLogContentValid -and
        -not [bool]$retailModeFixture.classification.hardDiagnosticFree -and
        [int]$retailModeFixture.classification.hardDiagnosticRawLineCount -eq 6 -and
        [int]$retailModeFixture.classification.hardDiagnosticEventCount -eq 2 -and
        [bool]$retailModeFixture.classification.
            approvedStockDiagnosticClusterPresent -and
        [int]$retailModeFixture.classification.
            approvedStockDiagnosticRawLineCount -eq 6 -and
        [int]$retailModeFixture.classification.
            approvedStockDiagnosticEventCount -eq 2 -and
        @($retailModeFixture.classification.logs | Where-Object {
            [string]$_.leaf -ceq 'crash.log'
        }).Count -eq 0 -and
        @($diagnosticModeFixture.classification.logs).Count -eq 4 -and
        [string]$diagnosticModeFixture.classification.hardDiagnosticPolicy -ceq
            'script-engine-and-process-fatal-v1' -and
        [bool]$diagnosticModeFixture.classification.crashLogContentValid -and
        [bool]$diagnosticModeFixture.classification.hardDiagnosticFree -and
        [int]$diagnosticModeFixture.classification.hardDiagnosticRawLineCount -eq 0 -and
        [int]$diagnosticModeFixture.classification.hardDiagnosticEventCount -eq 0 -and
        -not [bool]$diagnosticModeFixture.classification.
            approvedStockDiagnosticClusterPresent -and
        @($diagnosticModeFixture.classification.logs | Where-Object {
            [string]$_.leaf -ceq 'crash.log'
        }).Count -eq 1) `
        'synthetic modes cover exact stock cluster, clean mode, and optional crash log'

    $cleanup = [ordered]@{
        schemaVersion = 1
        evidenceKind = 'partisan_release_surface_runtime_audit_v2'
        harnessRemoved = $true
        harnessResidueCount = 0
        exactOwnerVerifiedBeforeRemoval = $true
        passed = $true
    }
    $indexValue = New-TestEvidenceIndex $runRoot
    $indexPath = Join-Path $runRoot 'evidence-index.json'
    $indexSignature = Write-TestJson $indexPath $indexValue
    $limitations = @(
        'This audit uses inert compiler and metadata probes for contracted member-surface presence; separately, it deliberately invokes production menu generation and read-only per-command availability inspection, but it does not execute command actions or mutate campaign gameplay state.',
        'Forbidden literal surfaces are proven by the candidate-bound source guard analysis, not by an unreliable package-byte string scan.',
        'It is not gameplay, multiplayer, persistence, restart, soak, or performance certification.',
        'The guarded launcher deliberately gives the child no inherited standard streams; authoritative engine output is retained in the three required logs and in crash.log when the engine emits it.',
        'The machine-bound hard-diagnostic policy is script-engine-and-process-fatal-v1: SCRIPT or ENGINE error severity, access violations, unhandled exceptions, fatal/application-crash signals, and audit ERROR markers. Other retained engine-channel severities are outside this narrow release-surface predicate. A successful crash.log must be absent or empty.',
        'A mode may contain either no hard diagnostic or one exact stock Eden shutdown cluster: two underlying support-station catalog-manager events mirrored once across console.log, script.log, and error.log after replication finishes and before game destruction. Every partial, extra, variant, misplaced, crash-channel, or unrelated hard event fails closed.')
    $runValue = [ordered]@{
        schemaVersion = 2
        evidenceKind = 'partisan_release_surface_runtime_audit_v2'
        contractId = 'partisan.release-surface-audit.run.v2'
        runId = $runId
        runLeafId = $runLeaf
        runNonce = $runNonce
        startedUtc = '2026-07-20T00:00:00Z'
        completedUtc = '2026-07-20T00:01:00Z'
        disposition = 'passed-noncertifying-release-surface-audit'
        certificationPromotion = 'none'
        source = [ordered]@{
            harnessGitHead = $gitHead
            checkoutClean = $true
            candidateAncestor = $true
            executionMode = 'synthetic-self-test'
        }
        candidate = $candidatePublic
        candidateBindingSha256 = [string]$retail.candidateBindingSha256
        pairedSamePackage = $true
        candidatePackageSha256 = $packageSha256
        harnessGuid = $harnessGuid
        harnessSha256 = [string]$harnessBinding.aggregateSha256
        modes = [object[]]@(
            [ordered]@{
                mode = 'retail'
                path = 'modes/retail.json'
                signature = $retail.signature
            },
            [ordered]@{
                mode = 'diagnostic'
                path = 'modes/diagnostic.json'
                signature = $diagnostic.signature
            })
        cleanup = $cleanup
        evidenceIndex = [ordered]@{
            path = 'evidence-index.json'
            signature = $indexSignature
            aggregateSha256 = [string]$indexValue.aggregateSha256
            fileCount = @($indexValue.files).Count
        }
        limitations = $limitations
        passed = $true
    }
    $runPath = Join-Path $runRoot 'run.json'
    $null = Write-TestJson $runPath $runValue
    $baseline = Get-TestBaseline $runRoot
    $retailResultLine =
        '2026-07-20 17:00:00.100 SCRIPT : ' +
        'Partisan release surface audit | RESULT | mode=retail' +
        ' | passed=true | mismatches=0'
    $retailReplicationFinishedLine =
        '2026-07-20 17:00:00.300 RPL : Replication finished.'
    $stockLineA =
        "2026-07-20 17:00:00.400 SCRIPT (E): " +
        "'SCR_BaseResupplySupportStationComponent' needs a entity catalog manager!"
    $stockLineB =
        "2026-07-20 17:00:00.500 SCRIPT (E): " +
        "'SCR_BaseResupplySupportStationComponent' needs a entity catalog manager!"
    $gameDestroyedLine =
        '2026-07-20 17:00:00.600 ENGINE : Game destroyed.'

    $validated = @(Invoke-TestFixtureValidation $runPath)
    Assert-TestCondition (
        $validated.Count -eq 1 -and
        [bool]$validated[0].FixtureValid -and
        -not [bool]$validated[0].ArtifactPublished -and
        -not (Test-Path -LiteralPath (Join-Path $runRoot 'release-index.json')) -and
        -not (Test-Path -LiteralPath (Join-Path $runRoot 'run.ready.json'))) `
        'valid synthetic fixture is validated without release publication'
    $validatedAgain = @(Invoke-TestFixtureValidation $runPath)
    Assert-TestCondition (
        $validatedAgain.Count -eq 1 -and
        [bool]$validatedAgain[0].FixtureValid -and
        -not [bool]$validatedAgain[0].ArtifactPublished -and
        -not (Test-Path -LiteralPath (Join-Path $runRoot 'release-index.json'))) `
        'repeated synthetic fixture validation remains non-publishing'
    [void]$checks.Add('valid-synthetic-fixture-no-publication')

    Assert-TestRejected 'normal invocation with synthetic fixture' {
        Invoke-TestPublisher $runPath
    } 'Synthetic release-surface fixtures cannot be published'
    Assert-TestCondition (
        -not (Test-Path -LiteralPath (Join-Path $runRoot 'release-index.json')) -and
        -not (Test-Path -LiteralPath (Join-Path $runRoot 'run.ready.json'))) `
        'normal synthetic rejection leaves no release artifacts'
    [void]$checks.Add('normal-synthetic-publication-rejected')

    Restore-TestBaseline $runRoot $baseline
    $stringBooleanRun = Read-TestJson $runPath
    $stringBooleanRun.passed = 'false'
    $null = Write-TestJson $runPath $stringBooleanRun
    Assert-TestRejected 'string false run outcome' {
        Invoke-TestFixtureValidation $runPath
    } 'must be a JSON boolean'
    [void]$checks.Add('string-false-boolean-rejected')

    Restore-TestBaseline $runRoot $baseline
    $numericStringRun = Read-TestJson $runPath
    $numericStringRun.schemaVersion = '2'
    $null = Write-TestJson $runPath $numericStringRun
    Assert-TestRejected 'numeric string schema' {
        Invoke-TestFixtureValidation $runPath
    } 'must be a JSON integer'
    [void]$checks.Add('numeric-string-schema-rejected')

    Restore-TestBaseline $runRoot $baseline
    $fractionalRun = Read-TestJson $runPath
    $fractionalRun.evidenceIndex.fileCount = 1.5
    $null = Write-TestJson $runPath $fractionalRun
    Assert-TestRejected 'fractional evidence count' {
        Invoke-TestFixtureValidation $runPath
    } 'must be a JSON integer'
    [void]$checks.Add('fractional-count-rejected')

    Restore-TestBaseline $runRoot $baseline
    $null = Write-TestPublishedSeals $runRoot
    $publishedBaseline = Get-TestBaseline $runRoot
    $treeBeforeVerification = Get-TestTreeStateDigest $runRoot
    $verified = @(Invoke-TestPublishedVerification $runPath)
    $treeAfterVerification = Get-TestTreeStateDigest $runRoot
    Assert-TestCondition (
        $verified.Count -eq 1 -and
        [bool]$verified[0].PublishedIndexValid -and
        [bool]$verified[0].ReadySealValid -and
        [bool]$verified[0].ReadOnlyVerification -and
        [bool]$verified[0].SyntheticFixture -and
        $treeBeforeVerification -ceq $treeAfterVerification) `
        'published verification is exact and performs zero writes'
    [void]$checks.Add('published-index-ready-read-only-verification')
    Assert-TestRejected 'synthetic production verification' {
        & $producerPath -RunEnvelopePath $runPath -VerifyPublishedIndex
    } 'cannot be published or production-verified'
    [void]$checks.Add('synthetic-production-verification-rejected')

    $retailProbePath = Join-Path $runRoot `
        'raw\retail\profile\Partisan\probe.result.json'
    $retailProbe = Read-TestJson $retailProbePath
    $retailProbe.forbiddenTypes[0].present = $true
    $null = Write-TestJson $retailProbePath $retailProbe
    Sync-TestBundleSeals $runRoot
    $null = Write-TestPublishedSeals $runRoot
    Assert-TestRejected 'resealed raw semantic tamper' {
        Invoke-TestPublishedVerification $runPath
    } 'forbidden type'
    [void]$checks.Add('resealed-raw-semantic-tamper-rejected')

    Restore-TestBaseline $runRoot $publishedBaseline
    $tamperedIndexPath = Join-Path $runRoot 'release-index.json'
    $tamperedIndex = Read-TestJson $tamperedIndexPath
    $tamperedIndex.passed = $false
    $tamperedIndexSignature = Write-TestJson $tamperedIndexPath $tamperedIndex
    $tamperedReadyPath = Join-Path $runRoot 'run.ready.json'
    $tamperedReady = Read-TestJson $tamperedReadyPath
    $tamperedReady.releaseIndex.signature = $tamperedIndexSignature
    $null = Write-TestJson $tamperedReadyPath $tamperedReady
    Assert-TestRejected 'resealed outer index tamper' {
        Invoke-TestPublishedVerification $runPath
    } 'exact canonical index'
    [void]$checks.Add('resealed-outer-index-tamper-rejected')

    Restore-TestBaseline $runRoot $publishedBaseline
    $stringReady = Read-TestJson (Join-Path $runRoot 'run.ready.json')
    $stringReady.sealedLast = 'false'
    $null = Write-TestJson (Join-Path $runRoot 'run.ready.json') $stringReady
    Assert-TestRejected 'string false terminal ready seal' {
        Invoke-TestPublishedVerification $runPath
    } 'must be a JSON boolean'
    [void]$checks.Add('string-false-ready-seal-rejected')

    Restore-TestBaseline $runRoot $baseline
    $retailProbe = Read-TestJson $retailProbePath
    $retailProbe.forbiddenTypes[0].present = $true
    $null = Write-TestJson $retailProbePath $retailProbe
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'retail forbidden type presence' {
        Invoke-TestFixtureValidation $runPath
    } 'forbidden type'
    [void]$checks.Add('retail-forbidden-type-rejected')

    Restore-TestBaseline $runRoot $baseline
    $retailProbe = Read-TestJson $retailProbePath
    $retailProbe.forbiddenMembers[0].present = $true
    $null = Write-TestJson $retailProbePath $retailProbe
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'retail forbidden member presence' {
        Invoke-TestFixtureValidation $runPath
    } 'forbidden member'
    [void]$checks.Add('retail-forbidden-member-rejected')

    Restore-TestBaseline $runRoot $baseline
    $retailProbe = Read-TestJson $retailProbePath
    $retailProbe.forbiddenMembers[0].probeSupported = $false
    $null = Write-TestJson $retailProbePath $retailProbe
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'unsupported retail member probe' {
        Invoke-TestFixtureValidation $runPath
    } 'forbidden member'
    [void]$checks.Add('unsupported-member-probe-rejected')

    Restore-TestBaseline $runRoot $baseline
    $retailProbe = Read-TestJson $retailProbePath
    $retailProbe.forbiddenMembers[0].declaringType = 'FabricatedOwner'
    $null = Write-TestJson $retailProbePath $retailProbe
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'fabricated member owner' {
        Invoke-TestFixtureValidation $runPath
    } 'forbidden member identity'
    [void]$checks.Add('fabricated-member-owner-rejected')

    Restore-TestBaseline $runRoot $baseline
    $retailProbe = Read-TestJson $retailProbePath
    $retailProbe.runtimeCompilerAlwaysPublicCompiled = $false
    $retailProbe.runtimeCompilerAvailable = $false
    $null = Write-TestJson $retailProbePath $retailProbe
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'failed compiler controls' {
        Invoke-TestFixtureValidation $runPath
    } 'fixed outcome'
    [void]$checks.Add('failed-compiler-controls-rejected')

    Restore-TestBaseline $runRoot $baseline
    $diagnosticProbePath = Join-Path $runRoot `
        'raw\diagnostic\profile\Partisan\probe.result.json'
    $diagnosticProbe = Read-TestJson $diagnosticProbePath
    $diagnosticProbe.productionMembers[0].present = $false
    $null = Write-TestJson $diagnosticProbePath $diagnosticProbe
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'missing production member' {
        Invoke-TestFixtureValidation $runPath
    } 'production member'
    [void]$checks.Add('missing-production-member-rejected')

    Restore-TestBaseline $runRoot $baseline
    $diagnosticProbePath = Join-Path $runRoot `
        'raw\diagnostic\profile\Partisan\probe.result.json'
    $diagnosticProbe = Read-TestJson $diagnosticProbePath
    $diagnosticProbe.forbiddenCommands[0].routingPresent = $false
    $null = Write-TestJson $diagnosticProbePath $diagnosticProbe
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'diagnostic routing absence' {
        Invoke-TestFixtureValidation $runPath
    } 'routingPresent'
    [void]$checks.Add('diagnostic-routing-rejected')

    Restore-TestBaseline $runRoot $baseline
    Add-Content -LiteralPath `
        (Join-Path $runRoot 'raw\retail\logs\session\script.log') `
        -Value 'SCRIPT (E): synthetic hard diagnostic'
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'hard diagnostic' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('hard-diagnostic-rejected')

    Restore-TestBaseline $runRoot $baseline
    Add-Content -LiteralPath `
        (Join-Path $runRoot 'raw\diagnostic\logs\session\crash.log') `
        -Value 'SCRIPT (E): synthetic crash-log diagnostic'
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'optional crash-log hard diagnostic' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('optional-crash-hard-diagnostic-rejected')

    Restore-TestBaseline $runRoot $baseline
    Add-Content -LiteralPath `
        (Join-Path $runRoot 'raw\diagnostic\logs\session\crash.log') `
        -Value 'benign-looking crash channel content'
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'non-empty benign-looking crash log' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('nonempty-benign-crash-log-rejected')

    Restore-TestBaseline $runRoot $baseline
    $unexpectedLogTreeFile = Join-Path $runRoot `
        'raw\retail\logs\session\unexpected.txt'
    $null = Write-TestText $unexpectedLogTreeFile ''
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'unexpected non-log file in log tree' {
        Invoke-TestFixtureValidation $runPath
    } 'unbound, duplicated, or unknown log leaves'
    [void]$checks.Add('unexpected-log-tree-file-rejected')

    Restore-TestBaseline $runRoot $baseline
    $diagnosticErrorPath = Join-Path $runRoot `
        'raw\diagnostic\logs\session\error.log'
    $null = Write-TestText $diagnosticErrorPath `
        "2026-07-20 17:00:00.150 WORLD (E): synthetic retained channel error`n"
    Sync-TestBundleSeals $runRoot
    $narrowPolicyValidation = @(Invoke-TestFixtureValidation $runPath)
    Assert-TestCondition (
        $narrowPolicyValidation.Count -eq 1 -and
        [bool]$narrowPolicyValidation[0].FixtureValid) `
        'explicit non-hard engine-channel policy remains narrow and retained'
    [void]$checks.Add('explicit-nonhard-channel-policy-accepted')

    Restore-TestBaseline $runRoot $baseline
    $retailScriptPath = Join-Path $runRoot `
        'raw\retail\logs\session\script.log'
    $retailScriptText = [IO.File]::ReadAllText($retailScriptPath)
    $oneMillisecondResultLine = $retailResultLine.Replace(
        '17:00:00.100 ', '17:00:00.101 ')
    $null = Write-TestText $retailScriptPath `
        $retailScriptText.Replace(
            $retailResultLine, $oneMillisecondResultLine)
    Sync-TestBundleSeals $runRoot
    $oneMillisecondValidation = @(Invoke-TestFixtureValidation $runPath)
    Assert-TestCondition (
        $oneMillisecondValidation.Count -eq 1 -and
        [bool]$oneMillisecondValidation[0].FixtureValid) `
        'one-millisecond result timestamp drift remains one mirrored marker'
    [void]$checks.Add('one-millisecond-result-timestamp-drift-accepted')

    Restore-TestBaseline $runRoot $baseline
    $retailScriptText = [IO.File]::ReadAllText($retailScriptPath)
    $nonprecedingResultLine = $retailResultLine.Replace(
        '17:00:00.100 ', '17:00:00.200 ')
    $null = Write-TestText $retailScriptPath `
        $retailScriptText.Replace(
            $retailResultLine, $nonprecedingResultLine)
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'result timestamp at replication finishing' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('nonpreceding-result-timestamp-rejected')

    Restore-TestBaseline $runRoot $baseline
    $diagnosticConsolePath = Join-Path $runRoot `
        'raw\diagnostic\logs\session\console.log'
    $diagnosticConsoleText = [IO.File]::ReadAllText($diagnosticConsolePath).
        Replace('17:00:00.200 RPL', '17:00:00.350 RPL')
    $null = Write-TestText $diagnosticConsolePath $diagnosticConsoleText
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'reversed clean lifecycle timestamps' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('reversed-clean-lifecycle-rejected')

    Restore-TestBaseline $runRoot $baseline
    $diagnosticConsoleText = [IO.File]::ReadAllText($diagnosticConsolePath).
        Replace(
            '2026-07-20 17:00:00.200 RPL',
            '2026-99-20 17:00:00.200 RPL')
    $null = Write-TestText $diagnosticConsolePath $diagnosticConsoleText
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'malformed clean lifecycle timestamp' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('malformed-clean-lifecycle-rejected')

    Restore-TestBaseline $runRoot $baseline
    $retailConsolePath = Join-Path $runRoot `
        'raw\retail\logs\session\console.log'
    $retailConsoleText = [IO.File]::ReadAllText($retailConsolePath).
        Replace($stockLineA + "`n", '').
        Replace($stockLineB + "`n", '').
        Replace(
            $retailReplicationFinishedLine + "`n",
            $stockLineA + "`n" + $stockLineB + "`n" +
                $retailReplicationFinishedLine + "`n")
    $null = Write-TestText $retailConsolePath $retailConsoleText
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'stock cluster before replication finished' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('pre-replication-stock-cluster-rejected')

    Restore-TestBaseline $runRoot $baseline
    foreach ($leaf in @('console.log', 'script.log', 'error.log')) {
        $path = Join-Path $runRoot ('raw\retail\logs\session\' + $leaf)
        $text = [IO.File]::ReadAllText($path).Replace($stockLineB + "`n", '')
        $null = Write-TestText $path $text
    }
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'one-event stock teardown cluster' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('one-event-stock-cluster-rejected')

    Restore-TestBaseline $runRoot $baseline
    $stockLineC = $stockLineB.Replace('.500 ', '.550 ')
    foreach ($leaf in @('console.log', 'script.log', 'error.log')) {
        $path = Join-Path $runRoot ('raw\retail\logs\session\' + $leaf)
        $text = [IO.File]::ReadAllText($path)
        if ($leaf -ceq 'console.log') {
            $text = $text.Replace(
                $gameDestroyedLine,
                $stockLineC + "`n" + $gameDestroyedLine)
        }
        else { $text += $stockLineC + "`n" }
        $null = Write-TestText $path $text
    }
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'third stock teardown event' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('third-stock-event-rejected')

    Restore-TestBaseline $runRoot $baseline
    $errorLogPath = Join-Path $runRoot 'raw\retail\logs\session\error.log'
    $errorText = [IO.File]::ReadAllText($errorLogPath).
        Replace($stockLineA + "`n", '')
    $null = Write-TestText $errorLogPath $errorText
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'missing stock teardown mirror' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('missing-stock-mirror-rejected')

    Restore-TestBaseline $runRoot $baseline
    $scriptLogPath = Join-Path $runRoot 'raw\retail\logs\session\script.log'
    $scriptText = [IO.File]::ReadAllText($scriptLogPath).Replace(
        $stockLineA + "`n", $stockLineA + "`n" + $stockLineA + "`n")
    $null = Write-TestText $scriptLogPath $scriptText
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'duplicate stock event in one log leaf' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('same-leaf-stock-duplicate-rejected')

    Restore-TestBaseline $runRoot $baseline
    $scriptText = [IO.File]::ReadAllText($scriptLogPath).Replace(
        $stockLineA, $stockLineA.Replace('.400 ', '.410 '))
    $null = Write-TestText $scriptLogPath $scriptText
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'cross-leaf stock timestamp drift' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('stock-timestamp-drift-rejected')

    Restore-TestBaseline $runRoot $baseline
    $consoleLogPath = Join-Path $runRoot 'raw\retail\logs\session\console.log'
    $consoleText = [IO.File]::ReadAllText($consoleLogPath).
        Replace($stockLineA + "`n", '').
        Replace($stockLineB + "`n", '').
        Replace(
            $retailResultLine + "`n",
            $stockLineA + "`n" + $stockLineB + "`n" +
                $retailResultLine + "`n")
    $null = Write-TestText $consoleLogPath $consoleText
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'stock teardown before result' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('pre-result-stock-cluster-rejected')

    Restore-TestBaseline $runRoot $baseline
    $consoleText = [IO.File]::ReadAllText($consoleLogPath).
        Replace($stockLineA + "`n", '').
        Replace($stockLineB + "`n", '').
        Replace(
            $gameDestroyedLine + "`n",
            $gameDestroyedLine + "`n" + $stockLineA + "`n" +
                $stockLineB + "`n")
    $null = Write-TestText $consoleLogPath $consoleText
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'stock teardown after game destruction' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('post-destroy-stock-cluster-rejected')

    Restore-TestBaseline $runRoot $baseline
    foreach ($leaf in @('console.log', 'script.log', 'error.log')) {
        $path = Join-Path $runRoot ('raw\retail\logs\session\' + $leaf)
        $text = [IO.File]::ReadAllText($path).
            Replace('needs a entity catalog manager!',
                'needs an entity catalog manager!')
        $null = Write-TestText $path $text
    }
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'stock teardown message variant' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('stock-message-variant-rejected')

    Restore-TestBaseline $runRoot $baseline
    $consoleText = [IO.File]::ReadAllText($consoleLogPath).Replace(
        $stockLineA + "`n",
        $stockLineA + "`n  synthetic diagnostic body`n")
    $null = Write-TestText $consoleLogPath $consoleText
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'stock teardown diagnostic body' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('stock-diagnostic-body-rejected')

    Restore-TestBaseline $runRoot $baseline
    Add-Content -LiteralPath `
        (Join-Path $runRoot 'raw\diagnostic\logs\session\crash.log') `
        -Value $stockLineA
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'stock teardown in crash log' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('stock-crash-channel-rejected')

    Restore-TestBaseline $runRoot $baseline
    $retailModePath = Join-Path $runRoot 'modes\retail.json'
    $retailMode = Read-TestJson $retailModePath
    $retailMode.classification.hardDiagnosticEventCount = 3
    $null = Write-TestJson $retailModePath $retailMode
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'forged hard-diagnostic census' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('forged-hard-diagnostic-census-rejected')

    Restore-TestBaseline $runRoot $baseline
    $retailModePath = Join-Path $runRoot 'modes\retail.json'
    $retailMode = Read-TestJson $retailModePath
    $requiredLogRow = @($retailMode.classification.logs | Where-Object {
        [string]$_.leaf -ceq 'script.log'
    })
    Remove-Item -LiteralPath `
        (Join-Path $runRoot ([string]$requiredLogRow[0].path).Replace('/', '\')) `
        -Force -ErrorAction Stop
    $retailMode.classification.logs = @(
        $retailMode.classification.logs | Where-Object {
            [string]$_.leaf -cne 'script.log'
        })
    $null = Write-TestJson $retailModePath $retailMode
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'missing required log' {
        Invoke-TestFixtureValidation $runPath
    } '(?:retained log count|required log)'
    [void]$checks.Add('missing-required-log-rejected')

    Restore-TestBaseline $runRoot $baseline
    $duplicateCrashPath = Join-Path $runRoot `
        'raw\diagnostic\logs\duplicate\crash.log'
    $null = Write-TestText $duplicateCrashPath ''
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'duplicate optional crash log' {
        Invoke-TestFixtureValidation $runPath
    } 'unbound, duplicated, or unknown log leaves'
    [void]$checks.Add('duplicate-optional-crash-log-rejected')

    Restore-TestBaseline $runRoot $baseline
    $unknownLogPath = Join-Path $runRoot `
        'raw\retail\logs\session\unexpected.log'
    $null = Write-TestText $unknownLogPath ''
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'unknown log leaf' {
        Invoke-TestFixtureValidation $runPath
    } 'unbound, duplicated, or unknown log leaves'
    [void]$checks.Add('unknown-log-leaf-rejected')

    Restore-TestBaseline $runRoot $baseline
    $null = Write-TestText `
        (Join-Path $runRoot 'raw\retail\unlisted.bin') 'unlisted evidence'
    Assert-TestRejected 'unlisted census file' {
        Invoke-TestFixtureValidation $runPath
    } 'census'
    [void]$checks.Add('unlisted-file-rejected')

    Restore-TestBaseline $runRoot $baseline
    $swappedRun = Read-TestJson $runPath
    $swappedModes = @($swappedRun.modes)
    [array]::Reverse($swappedModes)
    $swappedRun.modes = $swappedModes
    $null = Write-TestJson $runPath $swappedRun
    Assert-TestRejected 'mode order swap' {
        Invoke-TestFixtureValidation $runPath
    } 'ordered'
    [void]$checks.Add('mode-order-rejected')

    Restore-TestBaseline $runRoot $baseline
    $runText = [IO.File]::ReadAllText($runPath)
    $duplicateRunText = $runText -replace '^\{',
        "{`n  `"schemaVersion`": 2,"
    [IO.File]::WriteAllText(
        $runPath, $duplicateRunText, (New-Object Text.UTF8Encoding($false)))
    Assert-TestRejected 'duplicate JSON property' {
        Invoke-TestFixtureValidation $runPath
    } 'duplicates JSON object property'
    [void]$checks.Add('duplicate-json-key-rejected')

    Restore-TestBaseline $runRoot $baseline
    $localRun = Read-TestJson $runPath
    $localRun.limitations[0] =
        [IO.Path]::GetPathRoot($tempRoot) + 'synthetic-local-path'
    $null = Write-TestJson $runPath $localRun
    Assert-TestRejected 'local path in portable run' {
        Invoke-TestFixtureValidation $runPath
    } 'machine-local path'
    [void]$checks.Add('portable-local-path-rejected')

    Restore-TestBaseline $runRoot $baseline
    $null = Write-TestJson (Join-Path $runRoot 'run.ready.json') `
        ([ordered]@{ sealedLast = $true })
    Assert-TestRejected 'ready before index' {
        Invoke-TestFixtureValidation $runPath
    } 'precede every terminal seal'
    [void]$checks.Add('ready-prepublication-rejected')

    Restore-TestBaseline $runRoot $baseline
    $cleanupRun = Read-TestJson $runPath
    $cleanupRun.cleanup.harnessResidueCount = 1
    $cleanupRun.cleanup.passed = $false
    $null = Write-TestJson $runPath $cleanupRun
    Assert-TestRejected 'cleanup residue' {
        Invoke-TestFixtureValidation $runPath
    } 'residue-free'
    [void]$checks.Add('cleanup-residue-rejected')

    Restore-TestBaseline $runRoot $baseline
    $tupleRun = Read-TestJson $runPath
    $tupleRun.candidatePackageSha256 = '0' * 64
    $null = Write-TestJson $runPath $tupleRun
    Assert-TestRejected 'candidate tuple mismatch' {
        Invoke-TestFixtureValidation $runPath
    } 'package, harness, or limitation'
    [void]$checks.Add('candidate-tuple-rejected')

    Restore-TestBaseline $runRoot $baseline
    $bindingRun = Read-TestJson $runPath
    $bindingRun.candidateBindingSha256 = '0' * 64
    $null = Write-TestJson $runPath $bindingRun
    Assert-TestRejected 'candidate binding mismatch' {
        Invoke-TestFixtureValidation $runPath
    } 'share the run candidate binding'
    [void]$checks.Add('candidate-binding-rejected')

    Restore-TestBaseline $runRoot $baseline
    $portableArgumentsPath = Join-Path $runRoot `
        'raw\retail\arguments.portable.json'
    $portableArguments = Read-TestJson $portableArgumentsPath
    $portableArguments.arguments[11] =
        [IO.Path]::GetPathRoot($tempRoot) + 'synthetic-profile'
    $null = Write-TestJson $portableArgumentsPath $portableArguments
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'portable argument local path' {
        Invoke-TestFixtureValidation $runPath
    } 'machine-local path'
    [void]$checks.Add('portable-argument-path-rejected')

    Restore-TestBaseline $runRoot $baseline
    $retailRawPath = Join-Path $runRoot 'raw\retail\arguments.raw.json'
    $retailPortablePath = Join-Path $runRoot `
        'raw\retail\arguments.portable.json'
    $retailRaw = Read-TestJson $retailRawPath
    $retailPortable = Read-TestJson $retailPortablePath
    $retailRaw.arguments = [object[]]@(
        $retailRaw.arguments + @('-scrDefine', 'ENABLE_DIAG'))
    $retailPortable.arguments = [object[]]@(
        $retailPortable.arguments + @('-scrDefine', 'ENABLE_DIAG'))
    Sync-TestModeArgumentMutation $runRoot retail $retailRaw $retailPortable
    Assert-TestRejected 'retail script-symbol injection' {
        Invoke-TestFixtureValidation $runPath
    } 'fixed launch contract|script-symbol authority'
    [void]$checks.Add('retail-script-symbol-injection-rejected')

    Restore-TestBaseline $runRoot $baseline
    $diagnosticRawPath = Join-Path $runRoot `
        'raw\diagnostic\arguments.raw.json'
    $diagnosticPortablePath = Join-Path $runRoot `
        'raw\diagnostic\arguments.portable.json'
    $diagnosticRaw = Read-TestJson $diagnosticRawPath
    $diagnosticPortable = Read-TestJson $diagnosticPortablePath
    $diagnosticRaw.arguments = [object[]]@(
        $diagnosticRaw.arguments | Where-Object {
            [string]$_ -cne '-scrDefine' -and
            [string]$_ -cne 'ENABLE_DIAG'
        })
    $diagnosticPortable.arguments = [object[]]@(
        $diagnosticPortable.arguments | Where-Object {
            [string]$_ -cne '-scrDefine' -and
            [string]$_ -cne 'ENABLE_DIAG'
        })
    Sync-TestModeArgumentMutation `
        $runRoot diagnostic $diagnosticRaw $diagnosticPortable
    Assert-TestRejected 'missing diagnostic script symbol' {
        Invoke-TestFixtureValidation $runPath
    } 'fixed launch contract'
    [void]$checks.Add('missing-diagnostic-script-symbol-rejected')

    Restore-TestBaseline $runRoot $baseline
    $diagnosticRaw = Read-TestJson $diagnosticRawPath
    $diagnosticPortable = Read-TestJson $diagnosticPortablePath
    $rawDefineIndex = Get-TestExactArgumentPosition `
        $diagnosticRaw.arguments '-scrDefine'
    $portableDefineIndex = Get-TestExactArgumentPosition `
        $diagnosticPortable.arguments '-scrDefine'
    $diagnosticRaw.arguments[$rawDefineIndex + 1] = 'OTHER_DIAG'
    $diagnosticPortable.arguments[$portableDefineIndex + 1] = 'OTHER_DIAG'
    Sync-TestModeArgumentMutation `
        $runRoot diagnostic $diagnosticRaw $diagnosticPortable
    Assert-TestRejected 'wrong diagnostic script symbol' {
        Invoke-TestFixtureValidation $runPath
    } 'fixed launch contract'
    [void]$checks.Add('wrong-diagnostic-script-symbol-rejected')

    Restore-TestBaseline $runRoot $baseline
    $diagnosticRaw = Read-TestJson $diagnosticRawPath
    $diagnosticPortable = Read-TestJson $diagnosticPortablePath
    $diagnosticRaw.arguments = [object[]]@(
        $diagnosticRaw.arguments + @('-scrDefine', 'ENABLE_DIAG'))
    $diagnosticPortable.arguments = [object[]]@(
        $diagnosticPortable.arguments + @('-scrDefine', 'ENABLE_DIAG'))
    Sync-TestModeArgumentMutation `
        $runRoot diagnostic $diagnosticRaw $diagnosticPortable
    Assert-TestRejected 'duplicate diagnostic script symbol' {
        Invoke-TestFixtureValidation $runPath
    } 'fixed launch contract'
    [void]$checks.Add('duplicate-diagnostic-script-symbol-rejected')

    Restore-TestBaseline $runRoot $baseline
    $diagnosticRaw = Read-TestJson $diagnosticRawPath
    $diagnosticPortable = Read-TestJson $diagnosticPortablePath
    $rawDefineIndex = Get-TestExactArgumentPosition `
        $diagnosticRaw.arguments '-scrDefine'
    $diagnosticRaw.arguments[$rawDefineIndex] = '-SCRDEFINE'
    Sync-TestModeArgumentMutation `
        $runRoot diagnostic $diagnosticRaw $diagnosticPortable
    Assert-TestRejected 'case-variant diagnostic script option' {
        Invoke-TestFixtureValidation $runPath
    } 'Diagnostic raw arguments'
    [void]$checks.Add('case-variant-diagnostic-option-rejected')

    Restore-TestBaseline $runRoot $baseline
    $diagnosticRaw = Read-TestJson $diagnosticRawPath
    $diagnosticPortable = Read-TestJson $diagnosticPortablePath
    $rawDefineIndex = Get-TestExactArgumentPosition `
        $diagnosticRaw.arguments '-scrDefine'
    $diagnosticRaw.arguments[$rawDefineIndex + 1] = 'enable_diag'
    Sync-TestModeArgumentMutation `
        $runRoot diagnostic $diagnosticRaw $diagnosticPortable
    Assert-TestRejected 'case-variant diagnostic script symbol' {
        Invoke-TestFixtureValidation $runPath
    } 'Diagnostic raw arguments'
    [void]$checks.Add('case-variant-diagnostic-symbol-rejected')

    Restore-TestBaseline $runRoot $baseline
    $diagnosticRaw = Read-TestJson $diagnosticRawPath
    $diagnosticPortable = Read-TestJson $diagnosticPortablePath
    $diagnosticRaw.arguments = [object[]]@(
        @($diagnosticRaw.arguments | Where-Object {
            [string]$_ -cne '-scrDefine' -and
            [string]$_ -cne 'ENABLE_DIAG'
        }) + '-scrDefine=ENABLE_DIAG')
    $diagnosticPortable.arguments = [object[]]@(
        @($diagnosticPortable.arguments | Where-Object {
            [string]$_ -cne '-scrDefine' -and
            [string]$_ -cne 'ENABLE_DIAG'
        }) + '-scrDefine=ENABLE_DIAG')
    Sync-TestModeArgumentMutation `
        $runRoot diagnostic $diagnosticRaw $diagnosticPortable
    Assert-TestRejected 'joined diagnostic script option' {
        Invoke-TestFixtureValidation $runPath
    } 'fixed launch contract'
    [void]$checks.Add('joined-diagnostic-script-option-rejected')

    Restore-TestBaseline $runRoot $baseline
    $crashPath = Join-Path $runRoot `
        'raw\diagnostic\logs\session\minidump-synthetic.bin'
    $null = Write-TestText $crashPath 'synthetic crash artifact'
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'crash artifact' {
        Invoke-TestFixtureValidation $runPath
    } 'classification'
    [void]$checks.Add('crash-artifact-rejected')

    Restore-TestBaseline $runRoot $baseline
    $bindingsPath = Join-Path $runRoot 'identity\bindings.json'
    $badBindings = Read-TestJson $bindingsPath
    $badBindings.tools[1].role = 'runner'
    $null = Write-TestJson $bindingsPath $badBindings
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'duplicate tool role' {
        Invoke-TestFixtureValidation $runPath
    } 'roles are invalid or duplicated'
    [void]$checks.Add('duplicate-tool-role-rejected')

    Restore-TestBaseline $runRoot $baseline
    $caseVariantBindings = Read-TestJson $bindingsPath
    $caseVariantBindings.tools[0].role = 'RUNNER'
    $null = Write-TestJson $bindingsPath $caseVariantBindings
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'case-variant tool role replacing runner' {
        Invoke-TestFixtureValidation $runPath
    } 'roles are invalid or duplicated'
    [void]$checks.Add('case-variant-tool-role-rejected')

    Restore-TestBaseline $runRoot $baseline
    $badToolHashBindings = Read-TestJson $bindingsPath
    $badToolHashBindings.tools[0].sha256 = '0' * 64
    $null = Write-TestJson $bindingsPath $badToolHashBindings
    Sync-TestBundleSeals $runRoot
    Assert-TestRejected 'tool worktree hash drift' {
        Invoke-TestFixtureValidation $runPath
    } 'worktree binding'
    [void]$checks.Add('tool-worktree-hash-rejected')

    Restore-TestBaseline $runRoot $baseline
    $evidenceIndexPath = Join-Path $runRoot 'evidence-index.json'
    $duplicateIndex = Read-TestJson $evidenceIndexPath
    $duplicateIndex.files = @($duplicateIndex.files) + $duplicateIndex.files[0]
    $duplicateIndex.aggregateSha256 = Get-TestRowsDigest `
        ([object[]]$duplicateIndex.files)
    $duplicateIndexSignature = Write-TestJson $evidenceIndexPath $duplicateIndex
    $duplicateRun = Read-TestJson $runPath
    $duplicateRun.evidenceIndex.signature = $duplicateIndexSignature
    $duplicateRun.evidenceIndex.aggregateSha256 =
        [string]$duplicateIndex.aggregateSha256
    $duplicateRun.evidenceIndex.fileCount = @($duplicateIndex.files).Count
    $null = Write-TestJson $runPath $duplicateRun
    Assert-TestRejected 'duplicate census path' {
        Invoke-TestFixtureValidation $runPath
    } '(?:canonically ordered|duplicated)'
    [void]$checks.Add('duplicate-census-path-rejected')

    Write-Output ('SELFTEST ' + ([pscustomobject][ordered]@{
        passed = $true
        checks = $checks.Count
        guardedReceiptCount = 2
        negativeCheckCount = $checks.Count - 1
    } | ConvertTo-Json -Compress))
}
catch { $testFailure = $_ }
finally {
    try {
        if (Test-Path -LiteralPath $tempRoot) {
            Remove-TestRootExact $tempRoot
        }
    }
    catch { $cleanupFailure = $_ }
}
if ($testFailure) { throw $testFailure }
if ($cleanupFailure) { throw $cleanupFailure }
