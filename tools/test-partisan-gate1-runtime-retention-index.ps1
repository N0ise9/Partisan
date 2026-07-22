[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-TestCondition {
    param([bool]$Condition, [string]$Label)
    if (-not $Condition) { throw "Gate 1 self-test failed: $Label." }
}

function Assert-TestRejected {
    param([string]$Label, [scriptblock]$Action, [string]$Pattern)
    $rejected = $false
    try { & $Action }
    catch {
        if (-not [string]::IsNullOrWhiteSpace($Pattern) -and
            $_.Exception.Message -notmatch $Pattern) {
            throw "Gate 1 rejection '$Label' had an unexpected error: $($_.Exception.Message)"
        }
        $rejected = $true
    }
    if (-not $rejected) { throw "Gate 1 self-test expected rejection: $Label." }
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
    $text = ($Value | ConvertTo-Json -Depth 64) + "`n"
    [IO.File]::WriteAllText(
        $Path, $text, (New-Object Text.UTF8Encoding($false)))
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
        throw 'A synthetic retained path escaped its root.'
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

function Get-TestArgumentDigest {
    param([string[]]$Arguments)
    return Get-TestSha256Text (
        (([string[]]$Arguments | ConvertTo-Json -Compress) + "`n"))
}

function New-TestCanonicalRetentionIndex {
    param($Run, [Parameter(Mandatory = $true)]$RunSignature)

    $candidateBinding = [string]$Run.lineage.contexts[0].candidateBindingSha256
    $runtimeRows = @($Run.runtime.contexts | ForEach-Object {
        [pscustomobject][ordered]@{
            ordinal = [int]$_.ordinal
            stage = [string]$_.stage
            contextId = [string]$_.contextId
            candidateBindingSha256 = [string]$_.candidateBindingSha256
            serverStopDisposition = [string]$_.serverStopDisposition
            clientLaunched = [bool]$_.clientLaunched
            receiptPath = [string]$_.receiptPath
            receiptSha256 = [string]$_.receiptSignature.sha256
        }
    })
    $fileRows = @($Run.files | Sort-Object path | ForEach-Object {
        [pscustomobject][ordered]@{
            role = [string]$_.role
            stage = [string]$_.stage
            path = [string]$_.path
            length = [long]$_.length
            sha256 = [string]$_.sha256
        }
    })
    return [ordered]@{
        schemaVersion = 1
        contractId = 'partisan.gate1-runtime-retention.index.v1'
        evidenceKind = 'packaged-gate1-runtime-retention'
        runId = [string]$Run.runId
        candidateId = [string]$Run.candidate.candidateId
        gitHead = [string]$Run.candidate.gitHead
        packageSha256 = [string]$Run.candidate.packageSha256
        candidateBindingSha256 = $candidateBinding
        run = [ordered]@{
            path = 'run.json'
            length = [long]$RunSignature.length
            sha256 = [string]$RunSignature.sha256
        }
        topology = [ordered]@{
            stageCount = 5
            diagnosticContextCount = 5
            diagnosticServerLaunchCount = 5
            diagnosticClientLaunchCount = 1
            standardContextCount = 5
            standardServerLaunchCount = 5
            standardClientLaunchCount = 1
            standardStages = [object[]]$runtimeRows
        }
        validation = [ordered]@{
            candidateAndReadyExact = $true
            packageCanonicalDigestExact = $true
            guardedReceiptsComplete = $true
            diagnosticAndStandardPhasesDisjoint = $true
            argumentTopologyExact = $true
            snapshotsExact = $true
            readOnlyStagesExact = $true
            standardSaveByteStabilityObserved = $true
            standardSaveRestorationCertified = $false
            fullFileCensusExact = $true
            executionMode = [string]$Run.scenario.executionMode
        }
        files = [object[]]$fileRows
        filesAggregateSha256 = Get-TestRowsDigest $fileRows
        disposition = 'passed-noncertifying-retention'
    }
}

function Write-TestRetentionReadySeal {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Run,
        [Parameter(Mandatory = $true)]$RunSignature,
        [Parameter(Mandatory = $true)]$IndexSignature
    )

    return Write-TestJson $Path ([ordered]@{
        schemaVersion = 1
        contractId = 'partisan.gate1-runtime-retention.ready.v1'
        evidenceKind = 'packaged-gate1-runtime-retention'
        runId = [string]$Run.runId
        candidateId = [string]$Run.candidate.candidateId
        run = [ordered]@{
            path = 'run.json'
            length = [long]$RunSignature.length
            sha256 = [string]$RunSignature.sha256
        }
        index = [ordered]@{
            path = 'release-index.json'
            length = [long]$IndexSignature.length
            sha256 = [string]$IndexSignature.sha256
        }
        disposition = 'passed-noncertifying-retention'
        publishedUtc = '2026-07-20T00:02:00.0000000Z'
    })
}

function Get-TestTreeState {
    param([Parameter(Mandatory = $true)][string]$Root)

    return (@(Get-ChildItem -LiteralPath $Root -Recurse -File -Force |
        Sort-Object FullName | ForEach-Object {
            $signature = Get-TestSignature $_.FullName
            [ordered]@{
                path = ConvertTo-TestPortablePath $Root $_.FullName
                length = [long]$signature.length
                sha256 = [string]$signature.sha256
                lastWriteUtcTicks = [long]$_.LastWriteTimeUtc.Ticks
            }
        }) | ConvertTo-Json -Depth 8 -Compress)
}

function Copy-TestState {
    param([hashtable]$State)
    $copy = @{}
    foreach ($key in $State.Keys) {
        $copy[$key] = [pscustomobject]@{
            Kind = [string]$State[$key].Kind
            Text = [string]$State[$key].Text
        }
    }
    return $copy
}

function New-TestSnapshot {
    param(
        [string]$Root,
        [string]$Stage,
        [string]$Direction,
        [hashtable]$State
    )
    New-Item -ItemType Directory -Path (Join-Path $Root 'files') -Force |
        Out-Null
    $rows = New-Object Collections.Generic.List[object]
    foreach ($relative in @($State.Keys | Sort-Object)) {
        $path = Join-Path $Root $relative.Replace('/', '\')
        $signature = Write-TestText $path ([string]$State[$relative].Text)
        [void]$rows.Add([pscustomobject][ordered]@{
            kind = [string]$State[$relative].Kind
            path = $relative
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        })
    }
    $manifest = [ordered]@{
        schemaVersion = 1
        contractId = 'partisan.gate1-runtime-retention.v1'
        stage = $Stage
        direction = $Direction
        files = [object[]]$rows.ToArray()
        aggregateSha256 = Get-TestRowsDigest $rows.ToArray()
    }
    $path = Join-Path $Root 'snapshot.json'
    $null = Write-TestJson $path $manifest
    return [pscustomobject]@{
        Path = $path
        Digest = [string]$manifest.aggregateSha256
        State = Copy-TestState $State
    }
}

function New-TestNativeSaveRows {
    param([string[]]$Ids)
    $state = @{}
    for ($index = 0; $index -lt $Ids.Count; $index++) {
        $id = $Ids[$index]
        $base = 'files/native/.save/game/6985327711302100/' +
            ('playthrough{0:d3}/savepoint{0:d3}' -f ($index + 1))
        $types = @(1, 0, 3)
        $names = @(
            'Partisan autosave',
            'Partisan manual checkpoint',
            'Partisan controlled shutdown')
        $state[$base + '/meta-info.json'] = [pscustomobject]@{
            Kind = 'native'
            Text = (([ordered]@{
                m_Id = $id
                m_eType = $types[$index]
                m_sSavePointDisplayName = $names[$index]
                m_rMissionResource = '6985327711302100'
            } | ConvertTo-Json -Compress) + "`n")
        }
        $state[$base + '/WorldState/world.bin'] = [pscustomobject]@{
            Kind = 'native'
            Text = 'synthetic-world-' + $id + "`n"
        }
    }
    return $state
}

$modulePath = Join-Path $PSScriptRoot 'Partisan.GuardedRuntime.psm1'
$producerPath = Join-Path $PSScriptRoot `
    'New-PartisanGate1RuntimeRetentionIndex.ps1'
$runnerPath = Join-Path $PSScriptRoot `
    'run-guarded-gate1-runtime-retention.ps1'
. $producerPath -RunEnvelopePath 'library-only' -LibraryOnly
. $runnerPath `
    -ManifestPath 'library-only' `
    -BundleRoot 'library-only' `
    -RuntimeAddonRoot 'library-only' `
    -ServerDiagnosticExecutable 'library-only' `
    -ClientDiagnosticExecutable 'library-only' `
    -ClientExecutable 'library-only' `
    -EvidenceRoot 'library-only' `
    -WatchedRoots @('library-only') `
    -SpillRoots @('library-only') `
    -LibraryOnly
Import-Module $modulePath -Force -ErrorAction Stop

$tempBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$tempRoot = Join-Path $tempBase (
    'PartisanGate1IndexSelfTest_' + [Guid]::NewGuid().ToString('N'))
$testFailure = $null
$cleanupFailure = $null
$checks = New-Object Collections.Generic.List[string]
New-Item -ItemType Directory -Path $tempRoot -ErrorAction Stop | Out-Null

try {
    $libraryBindingResult = @(& $runnerPath `
        -ManifestPath 'binding-self-test' `
        -BundleRoot 'binding-self-test' `
        -RuntimeAddonRoot 'runtime-binding-sentinel' `
        -ServerDiagnosticExecutable 'server-binding-sentinel' `
        -ClientDiagnosticExecutable 'client-diag-binding-sentinel' `
        -ClientExecutable 'client-binding-sentinel' `
        -EvidenceRoot 'evidence-binding-sentinel' `
        -WatchedRoots @('watched-binding-a', 'watched-binding-b') `
        -SpillRoots @('spill-binding-a') `
        -StageTimeoutSeconds 731 `
        -PollMilliseconds 777 `
        -ResultGraceSeconds 9 `
        -LibraryBindingSelfTest)
    Assert-TestCondition (
        $libraryBindingResult.Count -eq 1 -and
        [bool]$libraryBindingResult[0].success -and
        [string]$libraryBindingResult[0].clientExecutable -ceq
            'client-binding-sentinel' -and
        @($libraryBindingResult[0].watchedRoots).Count -eq 2 -and
        [string]$libraryBindingResult[0].watchedRoots[0] -ceq
            'watched-binding-a' -and
        [string]$libraryBindingResult[0].watchedRoots[1] -ceq
            'watched-binding-b' -and
        @($libraryBindingResult[0].spillRoots).Count -eq 1 -and
        [string]$libraryBindingResult[0].spillRoots[0] -ceq
            'spill-binding-a' -and
        [int]$libraryBindingResult[0].stageTimeoutSeconds -eq 731 -and
        [int]$libraryBindingResult[0].pollMilliseconds -eq 777 -and
        [int]$libraryBindingResult[0].resultGraceSeconds -eq 9) `
        'runner preserves every shared argument across the ordinary-library import'
    [void]$checks.Add('runner-ordinary-library-bindings-preserved')

    $failureRunRoot = Join-Path $tempRoot 'failure-envelope'
    $failureSessionRoot = Join-Path $failureRunRoot 'session'
    $failureGuardBase = Join-Path $failureRunRoot `
        'raw\diagnostic-guarded-runtime'
    $failureGuardRoot = Join-Path $failureGuardBase `
        'PartisanGuardedRuntime_synthetic'
    New-Item -ItemType Directory -Path `
        $failureSessionRoot, $failureGuardRoot -Force | Out-Null
    $failureWrapper = Get-Process -Id $PID -ErrorAction Stop
    $failureRunId = 'synthetic_failure'
    $null = Write-TestJson (Join-Path $failureRunRoot 'run.owner.json') `
        ([ordered]@{
            schemaVersion = 1
            magic = 'partisan_gate1_runtime_retention_owner_v1'
            runId = $failureRunId
            nonce = 'synthetic'
            ownerPid = $PID
            ownerStartUtc = $failureWrapper.StartTime.ToUniversalTime().
                ToString('o')
            purpose = 'self-test'
        })
    $failureSessionPath = Join-Path $failureSessionRoot 'retained.txt'
    $failureGuardPath = Join-Path $failureGuardRoot 'retained.txt'
    $failureSessionBefore = Write-TestText $failureSessionPath `
        "retained session bytes`n"
    $failureGuardBefore = Write-TestText $failureGuardPath `
        "retained guard bytes`n"
    $null = Write-TestJson (Join-Path $failureGuardBase `
            '.PartisanGuardedRuntime_synthetic.journal.json') ([ordered]@{
            mode = 'permanent-no-go'
        })
    $syntheticFailure = $null
    try {
        throw ('[PGR_WAIT_IDENTITY_UNKNOWN] Injected local-path failure: ' +
            $tempRoot)
    }
    catch {
        $syntheticFailure = $_
    }
    $failureStartedUtc = [DateTime]::UtcNow.AddSeconds(-1)
    $failureResult = Write-GateFailureEnvelope `
        -RunRoot $failureRunRoot `
        -RunId $failureRunId `
        -StartedUtc $failureStartedUtc `
        -GitHead ('a' * 40) `
        -CandidateId 'partisan-rc-synthetic' `
        -PackageSha256 ('b' * 64) `
        -ManifestSha256 ('c' * 64) `
        -ActivePhase 'diagnostic:profile_fallback_verify' `
        -Failure $syntheticFailure `
        -DiagnosticRows @(
            [pscustomobject]@{ stage = 'autosave_checkpoint' },
            [pscustomobject]@{ stage = 'manual_checkpoint' }) `
        -StandardRows @() `
        -LoopbackPort (Get-TestFreeUdpPort)
    $failureCleanupPath = Join-Path $failureRunRoot 'cleanup.json'
    $failureEnvelopePath = Join-Path $failureRunRoot 'run.failure.json'
    $failureCleanup = Get-Content -LiteralPath $failureCleanupPath -Raw |
        ConvertFrom-Json
    $failureEnvelope = Get-Content -LiteralPath $failureEnvelopePath -Raw |
        ConvertFrom-Json
    Assert-TestCondition (
        [string]$failureCleanup.magic -ceq
            'partisan_gate1_runtime_retention_cleanup_v1' -and
        -not [bool]$failureCleanup.passed -and
        [bool]$failureCleanup.readOnlyAudit -and
        [bool]$failureCleanup.sessionRetained -and
        [int]$failureCleanup.sessionFileCount -eq 1 -and
        [int]$failureCleanup.guardDirectoryCount -eq 1 -and
        [int]$failureCleanup.guardJournalCount -eq 1 -and
        [int]$failureCleanup.permanentNoGoJournalCount -eq 1 -and
        [int]$failureCleanup.invalidGuardJournalCount -eq 0 -and
        [int]$failureCleanup.guardedReceiptCount -eq 0 -and
        [int]$failureCleanup.guardedCompletionCount -eq 0 -and
        -not [bool]$failureCleanup.runEnvelopePresent -and
        -not [bool]$failureCleanup.releaseIndexPresent -and
        -not [bool]$failureCleanup.readySealPresent -and
        [string]$failureEnvelope.magic -ceq
            'partisan_gate1_runtime_retention_failure_v1' -and
        [string]$failureEnvelope.runId -ceq $failureRunId -and
        [string]$failureEnvelope.failure.identifier -ceq
            'PGR_WAIT_IDENTITY_UNKNOWN' -and
        [string]$failureEnvelope.failure.phase -ceq
            'diagnostic:profile_fallback_verify' -and
        [int]$failureEnvelope.progress.diagnosticCompletedCount -eq 2 -and
        @($failureEnvelope.progress.diagnosticCompletedStages).Count -eq 2 -and
        [string]$failureEnvelope.progress.diagnosticCompletedStages[0] -ceq
            'autosave_checkpoint' -and
        [string]$failureEnvelope.progress.diagnosticCompletedStages[1] -ceq
            'manual_checkpoint' -and
        [int]$failureEnvelope.progress.standardCompletedCount -eq 0 -and
        (Test-FileSignatureExact $failureResult.CleanupSignature `
            (Get-TestSignature $failureCleanupPath)) -and
        (Test-FileSignatureExact $failureResult.FailureSignature `
            (Get-TestSignature $failureEnvelopePath)) -and
        (Test-FileSignatureExact $failureSessionBefore `
            (Get-TestSignature $failureSessionPath)) -and
        (Test-FileSignatureExact $failureGuardBefore `
            (Get-TestSignature $failureGuardPath)) -and
        -not (Test-Path -LiteralPath (Join-Path $failureRunRoot 'run.json')) -and
        -not (Test-Path -LiteralPath (
            Join-Path $failureRunRoot 'release-index.json')) -and
        -not (Test-Path -LiteralPath (
            Join-Path $failureRunRoot 'run.ready.json'))) `
        'runner failure envelope is terminal, read-only, and exact'
    $failureSerialized = (Get-Content -LiteralPath $failureCleanupPath -Raw) +
        (Get-Content -LiteralPath $failureEnvelopePath -Raw)
    Assert-TestCondition (
        -not $failureSerialized.Contains($tempRoot) -and
        $failureSerialized -cnotmatch '[A-Za-z]:\\') `
        'runner failure envelope contains no local path'
    Assert-TestRejected 'runner failure envelope create-only replay' {
        Write-GateFailureEnvelope `
            -RunRoot $failureRunRoot `
            -RunId $failureRunId `
            -StartedUtc $failureStartedUtc `
            -GitHead ('a' * 40) `
            -CandidateId 'partisan-rc-synthetic' `
            -PackageSha256 ('b' * 64) `
            -ManifestSha256 ('c' * 64) `
            -ActivePhase 'diagnostic:profile_fallback_verify' `
            -Failure $syntheticFailure `
            -DiagnosticRows @() `
            -StandardRows @() `
            -LoopbackPort (Get-TestFreeUdpPort) | Out-Null
    } 'create-only retained JSON path already exists'
    [void]$checks.Add('runner-terminal-failure-envelope')

    $readyConflictRoot = Join-Path $tempRoot 'failure-ready-conflict'
    New-Item -ItemType Directory -Path $readyConflictRoot -Force | Out-Null
    $readyConflictRunId = 'synthetic_ready_conflict'
    $null = Write-TestJson (Join-Path $readyConflictRoot 'run.owner.json') `
        ([ordered]@{
            schemaVersion = 1
            magic = 'partisan_gate1_runtime_retention_owner_v1'
            runId = $readyConflictRunId
            nonce = 'synthetic'
            ownerPid = $PID
            ownerStartUtc = $failureWrapper.StartTime.ToUniversalTime().
                ToString('o')
            purpose = 'self-test'
        })
    $null = Write-TestJson (Join-Path $readyConflictRoot 'run.ready.json') `
        ([ordered]@{ ready = $true })
    Assert-TestRejected 'runner ready/failure terminal conflict' {
        Write-GateFailureEnvelope `
            -RunRoot $readyConflictRoot `
            -RunId $readyConflictRunId `
            -StartedUtc $failureStartedUtc `
            -GitHead ('a' * 40) `
            -CandidateId 'partisan-rc-synthetic' `
            -PackageSha256 ('b' * 64) `
            -ManifestSha256 ('c' * 64) `
            -ActivePhase 'publication' `
            -Failure $syntheticFailure `
            -DiagnosticRows @() `
            -StandardRows @() `
            -LoopbackPort (Get-TestFreeUdpPort) | Out-Null
    } 'failure sealing refuses to coexist with a ready seal'
    Assert-TestCondition (
        -not (Test-Path -LiteralPath (
            Join-Path $readyConflictRoot 'cleanup.json')) -and
        -not (Test-Path -LiteralPath (
            Join-Path $readyConflictRoot 'run.failure.json'))) `
        'ready seal prevents every failure-envelope side effect'
    [void]$checks.Add('runner-ready-failure-mutual-exclusion')

    $runnerTokens = $null
    $runnerParseErrors = $null
    $runnerAst = [Management.Automation.Language.Parser]::ParseFile(
        $runnerPath,
        [ref]$runnerTokens,
        [ref]$runnerParseErrors)
    Assert-TestCondition ($runnerParseErrors.Count -eq 0) `
        'runner parses for terminal failure-boundary inspection'
    $runnerStatements = @($runnerAst.EndBlock.Statements)
    $terminalTry = @($runnerStatements | Where-Object {
        $_ -is [Management.Automation.Language.TryStatementAst] -and
        $_.Extent.Text.Contains('Write-GateFailureEnvelope')
    })
    Assert-TestCondition ($terminalTry.Count -eq 1) `
        'runner has one top-level terminal failure boundary'
    $terminalTryIndex = [Array]::IndexOf($runnerStatements, $terminalTry[0])
    Assert-TestCondition (
        $terminalTryIndex -gt 0 -and
        $terminalTryIndex -lt ($runnerStatements.Count - 1) -and
        $runnerStatements[$terminalTryIndex - 1].Extent.Text.Contains(
            'run.owner.json') -and
        -not $terminalTry[0].Body.Extent.Text.Contains('Write-Output') -and
        $terminalTry[0].Body.Extent.Text.Contains(
            "`$activePhase = 'session-owner'") -and
        $terminalTry[0].Body.Extent.Text.Contains(
            "Join-Path `$sessionRoot '.owner.json'") -and
        $terminalTry[0].CatchClauses.Count -eq 1 -and
        $terminalTry[0].CatchClauses[0].Body.Extent.Text.Contains(
            'Write-GateFailureEnvelope') -and
        $runnerStatements[$terminalTryIndex + 1].Extent.Text -ceq
            'Write-Output $successResult') `
        'terminal failure boundary begins immediately after exact run ownership'
    [void]$checks.Add('runner-terminal-failure-boundary-placement')

    $logSetSource = Join-Path $tempRoot 'log-set-source'
    foreach ($leaf in @('console.log', 'script.log', 'error.log')) {
        $null = Write-TestText (Join-Path $logSetSource $leaf) `
            ("synthetic required $leaf`n")
    }
    $logSetWithoutCrash = Join-Path $tempRoot 'log-set-without-crash'
    Copy-GateLogSet `
        -SourceRoot $logSetSource `
        -DestinationRoot $logSetWithoutCrash `
        -Stage autosave_checkpoint `
        -Role server
    $withoutCrashLeaves = @(Get-ChildItem -LiteralPath $logSetWithoutCrash `
        -File -Force | Sort-Object Name | ForEach-Object { [string]$_.Name })
    Assert-TestCondition (
        $withoutCrashLeaves.Count -eq 3 -and
        @($withoutCrashLeaves | Where-Object {
                $_ -notin @('console.log', 'script.log', 'error.log')
            }).Count -eq 0) `
        'runner accepts exactly three required logs when crash.log is absent'
    [void]$checks.Add('runner-absent-crash-log-accepted')

    $sourceCrashPath = Join-Path $logSetSource 'crash.log'
    $sourceCrashSignature = Write-TestText $sourceCrashPath `
        "synthetic optional crash log`n"
    $logSetWithCrash = Join-Path $tempRoot 'log-set-with-crash'
    Copy-GateLogSet `
        -SourceRoot $logSetSource `
        -DestinationRoot $logSetWithCrash `
        -Stage autosave_checkpoint `
        -Role server
    $copiedCrashPath = Join-Path $logSetWithCrash 'crash.log'
    Assert-TestCondition (
        (Test-Path -LiteralPath $copiedCrashPath -PathType Leaf) -and
        (Test-FileSignatureExact $sourceCrashSignature `
            (Get-TestSignature $copiedCrashPath))) `
        'runner retains an exact optional crash.log when present'
    [void]$checks.Add('runner-present-crash-log-retained')

    $duplicateCrashPath = Join-Path $logSetSource 'nested\crash.log'
    $null = Write-TestText $duplicateCrashPath "synthetic duplicate crash log`n"
    Assert-TestRejected 'runner duplicate optional crash log' {
        Copy-GateLogSet `
            -SourceRoot $logSetSource `
            -DestinationRoot (Join-Path $tempRoot 'duplicate-crash-output') `
            -Stage autosave_checkpoint `
            -Role server
    } 'more than one exact crash.log'
    Remove-Item -LiteralPath $duplicateCrashPath -Force
    [void]$checks.Add('runner-duplicate-crash-log-rejected')

    $duplicateConsolePath = Join-Path $logSetSource 'nested\console.log'
    $null = Write-TestText $duplicateConsolePath `
        "synthetic duplicate console log`n"
    Assert-TestRejected 'runner duplicate required console log' {
        Copy-GateLogSet `
            -SourceRoot $logSetSource `
            -DestinationRoot (Join-Path $tempRoot 'duplicate-console-output') `
            -Stage autosave_checkpoint `
            -Role server
    } 'did not produce one exact console.log'
    Remove-Item -LiteralPath $duplicateConsolePath -Force
    [void]$checks.Add('runner-duplicate-required-log-rejected')

    $unknownSourceLog = Join-Path $logSetSource 'unknown.log'
    $null = Write-TestText $unknownSourceLog "synthetic unknown log`n"
    Assert-TestRejected 'runner unknown source log' {
        Copy-GateLogSet `
            -SourceRoot $logSetSource `
            -DestinationRoot (Join-Path $tempRoot 'unknown-log-output') `
            -Stage autosave_checkpoint `
            -Role server
    } 'unknown or case-variant log leaves'
    Remove-Item -LiteralPath $unknownSourceLog -Force
    [void]$checks.Add('runner-unknown-source-log-rejected')

    $runRoot = Join-Path $tempRoot 'retained-run'
    $diagnosticGuardBase = Join-Path $runRoot `
        'raw\diagnostic-guarded-runtime'
    $standardGuardBase = Join-Path $runRoot 'raw\guarded-runtime'
    $watched = Join-Path $tempRoot 'watched'
    $spill = Join-Path $tempRoot 'spill'
    $candidateSource = Join-Path $tempRoot 'candidate-source'
    $runtimeAddon = Join-Path $tempRoot 'runtime-addon'
    $working = Join-Path $tempRoot 'working'
    foreach ($directory in @(
            $runRoot, $diagnosticGuardBase, $standardGuardBase,
            $watched, $spill, $candidateSource,
            $runtimeAddon, $working,
            (Join-Path $runRoot 'identity\package\Partisan'),
            (Join-Path $runRoot 'configuration'),
            (Join-Path $runRoot 'raw\stages'))) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
    $safeEvidenceRoot = Join-Path $tempRoot 'safe-evidence'
    $resolvedSafeEvidence = Assert-GateEvidenceRootSafe `
        -EvidenceRoot $safeEvidenceRoot `
        -ProtectedRoots @($candidateSource, $runtimeAddon) `
        -WatchedRoots @($watched) `
        -SpillRoots @($spill)
    Assert-TestCondition (
        [string]$resolvedSafeEvidence -ceq
            [IO.Path]::GetFullPath($safeEvidenceRoot)) `
        'runner accepts a disjoint external evidence root'
    Assert-TestRejected 'volume-root evidence' {
        Assert-GateEvidenceRootSafe `
            -EvidenceRoot ([IO.Path]::GetPathRoot($tempRoot)) `
            -ProtectedRoots @($candidateSource) `
            -WatchedRoots @($watched) `
            -SpillRoots @($spill) | Out-Null
    } 'must not be a volume root'
    [void]$checks.Add('evidence-volume-root-rejected')
    Assert-TestRejected 'protected-root evidence overlap' {
        Assert-GateEvidenceRootSafe `
            -EvidenceRoot (Join-Path $candidateSource 'evidence') `
            -ProtectedRoots @($candidateSource, $runtimeAddon) `
            -WatchedRoots @($watched) `
            -SpillRoots @($spill) | Out-Null
    } 'overlaps a protected or monitored root'
    [void]$checks.Add('evidence-overlap-rejected')

    $reparseSession = Join-Path $tempRoot 'reparse-session'
    $reparseTarget = Join-Path $tempRoot 'reparse-target'
    New-Item -ItemType Directory -Path $reparseSession, $reparseTarget -Force |
        Out-Null
    $reparseMarker = Join-Path $reparseTarget 'preserve.txt'
    $null = Write-TestText $reparseMarker "preserve`n"
    $reparseLink = Join-Path $reparseSession 'late-junction'
    $null = New-Item -ItemType Junction -Path $reparseLink `
        -Target $reparseTarget -ErrorAction Stop
    Assert-TestRejected 'late session reparse insertion' {
        Remove-GateSessionRoot -SessionRoot $reparseSession
    } 'reparse point'
    Assert-TestCondition (
        (Test-Path -LiteralPath $reparseSession -PathType Container) -and
        (Test-Path -LiteralPath $reparseMarker -PathType Leaf)) `
        'late reparse insertion blocks recursive session removal'
    [void]$checks.Add('session-reparse-removal-rejected')
    [IO.Directory]::Delete($reparseLink)
    $ownerPath = Join-Path $runRoot 'run.owner.json'
    $null = Write-TestJson $ownerPath ([ordered]@{
        schemaVersion = 1
        magic = 'partisan_gate1_runtime_retention_owner_v1'
        runId = 'synthetic-owner'
    })

    $sleeperPath = Join-Path $tempRoot 'Gate1SyntheticSleeper.exe'
    Add-Type -TypeDefinition @'
using System;
using System.Threading;
public static class Gate1SyntheticSleeper {
    public static int Main(string[] args) {
        Thread.Sleep(TimeSpan.FromSeconds(30));
        return 0;
    }
}
'@ -Language CSharp -OutputAssembly $sleeperPath -OutputType ConsoleApplication
    $sleeperProvenance = Get-PartisanExecutableProvenance $sleeperPath

    $packageRows = New-Object Collections.Generic.List[object]
    foreach ($entry in @(
            [pscustomobject]@{ Name = 'addon.gproj'; Text = '{"ProjectName":"Partisan"}' },
            [pscustomobject]@{ Name = 'data.pak'; Text = 'synthetic-package-bytes' })) {
        $path = Join-Path $candidateSource $entry.Name
        $signature = Write-TestText $path ([string]$entry.Text)
        [void]$packageRows.Add([pscustomobject][ordered]@{
            indexPath = 'Partisan/' + $entry.Name
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        })
        Copy-Item -LiteralPath $path -Destination (
            Join-Path $runRoot ('identity\package\Partisan\' + $entry.Name))
    }
    $packageDigestRows = @($packageRows.ToArray() | ForEach-Object {
        [pscustomobject]@{
            path = [string]$_.indexPath
            length = [long]$_.length
            sha256 = [string]$_.sha256
        }
    })
    $packageSha = Get-TestRowsDigest $packageDigestRows
    $candidateId = 'partisan-gate1-synthetic-self-test'
    $identity = {
        param([string]$Leaf, [string]$Hash)
        [pscustomobject][ordered]@{
            fileName = $Leaf
            fileVersion = '1.0.0.0'
            productVersion = '1.0.0.0'
            length = 1234
            sha256 = $Hash
        }
    }
    $serverDiagnosticIdentity =
        & $identity 'ArmaReforgerServerDiag.exe' ('1' * 64)
    $clientDiagnosticIdentity =
        & $identity 'ArmaReforgerSteamDiag.exe' ('2' * 64)
    $serverIdentity = & $identity 'ArmaReforgerServer.exe' ('3' * 64)
    $clientIdentity = & $identity 'ArmaReforgerSteam.exe' ('4' * 64)
    $candidate = [pscustomobject][ordered]@{
        CandidateId = $candidateId
        PackageSha256 = $packageSha
        PackageFiles = [object[]]$packageRows.ToArray()
        PackedAddonPath = $candidateSource
        RuntimeAddonRootPath = $runtimeAddon
    }
    $gitHead = '1' * 40
    $manifestPath = Join-Path $runRoot 'identity\candidate.json'
    $manifestValue = [pscustomobject][ordered]@{
        manifestSchemaVersion = 1
        candidate = [ordered]@{
            id = $candidateId
            version = '0.0.0-self-test'
            state = 'retained-uncertified'
        }
        source = [ordered]@{
            gitHead = $gitHead
            embeddedImplementation = [ordered]@{
                sha = '2' * 40
                utc = '2026-07-20T00:00:00Z'
                label = 'synthetic-self-test'
            }
            campaignSchema = 71
            runtimeSettingsSchema = 24
        }
        addon = [ordered]@{
            id = 'histasi'
            guid = '698532771130111D'
        }
        toolchain = [ordered]@{
            server = $serverIdentity
            client = $clientIdentity
            serverDiagnostic = $serverDiagnosticIdentity
            clientDiagnostic = $clientDiagnosticIdentity
        }
        workbench = [ordered]@{ crc = '00000000' }
        package = [ordered]@{
            hashAlgorithm = 'sha256-manifest-v1'
            sha256 = $packageSha
        }
    }
    $null = Write-TestJson $manifestPath $manifestValue
    $manifestSignature = Get-TestSignature $manifestPath
    $readyPath = Join-Path $runRoot 'identity\candidate.ready.json'
    $readyValue = [pscustomobject][ordered]@{
        schemaVersion = 1
        candidateId = $candidateId
        gitHead = $gitHead
        packageSha256 = $packageSha
        manifestSha256 = [string]$manifestSignature.sha256
    }
    $null = Write-TestJson $readyPath $readyValue
    $readySignature = Get-TestSignature $readyPath

    $settingsPath = Join-Path $runRoot 'configuration\HST_Settings.json'
    $settingsSignature = Write-TestJson $settingsPath ([ordered]@{
        schemaVersion = 24
        persistence = [ordered]@{
            autosaveIntervalSeconds = 60
            majorChangeDebounceSeconds = 120
        }
    })
    $runNonce = [Guid]::NewGuid().ToString('N')
    $payloadNonce = [Guid]::NewGuid().ToString('N')
    $runId = 'gate1_20260720T000000Z_' + $runNonce.Substring(0, 20)
    $stages = @(
        'autosave_checkpoint',
        'manual_checkpoint',
        'shutdown_checkpoint',
        'native_shutdown_verify',
        'profile_fallback_verify')
    $stageDirectories = @(
        '00-autosave_checkpoint',
        '01-manual_checkpoint',
        '02-shutdown_checkpoint',
        '03-native_shutdown_verify',
        '04-profile_fallback_verify')
    $saveIds = @(
        '11111111-1111-1111-1111-111111111111',
        '22222222-2222-2222-2222-222222222222',
        '33333333-3333-3333-3333-333333333333')
    $validStandardServerArguments = [string[]]@(
        '-gproj', 'packed/addon.gproj',
        '-server', 'Worlds/HST_Everon/HST_Everon.ent',
        '-MissionHeader', 'Missions/HST_Everon.conf',
        '-addonsDir', 'runtime,packed',
        '-addons', '698532771130111D',
        '-profile', 'profile',
        '-logsDir', 'logs',
        '-addonTempDir', 'addon-temp',
        '-logLevel', 'normal',
        '-logTime', 'datetime',
        '-noThrow',
        '-maxFPS', '30',
        '-loadSessionSave', $saveIds[0],
        '-hstReleaseCandidateId', $candidateId,
        '-hstReleasePackageSha256', $packageSha,
        '-hstReleaseManifestSha256', [string]$manifestSignature.sha256)
    $validStandardClientArguments = [string[]]@(
        '-gproj', 'packed/addon.gproj',
        '-addonsDir', 'runtime,packed',
        '-addons', '698532771130111D',
        '-addonTempDir', 'addon-temp',
        '-client',
        '-profile', 'profile',
        '-logsDir', 'logs',
        '-logLevel', 'normal',
        '-logTime', 'datetime',
        '-window',
        '-noFocus',
        '-forceUpdate',
        '-noSplash',
        '-noSound',
        '-noThrow',
        '-maxFPS', '30',
        '-hstReleaseCandidateId', $candidateId,
        '-hstReleasePackageSha256', $packageSha,
        '-hstReleaseManifestSha256', [string]$manifestSignature.sha256)
    $diagnosticStageNonce = 'a' * 32
    $validDiagnosticServerArguments = [string[]]@(
        '-gproj', 'packed/addon.gproj',
        '-server', 'Worlds/HST_Everon/HST_Everon.ent',
        '-MissionHeader', 'Missions/HST_Everon.conf',
        '-addonsDir', 'runtime,packed',
        '-addons', '698532771130111D',
        '-profile', 'profile',
        '-logLevel', 'normal',
        '-logTime', 'datetime',
        '-noThrow',
        '-maxFPS', '30',
        '-loadSessionSave',
        '-hstOrdinaryCampaignPersistenceProof', 'true',
        '-hstOrdinaryCampaignPersistenceStage', 'autosave_checkpoint',
        '-hstOrdinaryCampaignPersistenceRunId', $runId,
        '-hstOrdinaryCampaignPersistenceSessionNonce', $runNonce,
        '-hstOrdinaryCampaignPersistenceStageNonce', $diagnosticStageNonce,
        '-logsDir', 'logs',
        '-addonTempDir', 'addon-temp',
        '-hstReleaseCandidateId', $candidateId,
        '-hstReleasePackageSha256', $packageSha,
        '-hstReleaseManifestSha256', [string]$manifestSignature.sha256,
        '-scrDefine', 'ENABLE_DIAG')
    $validDiagnosticClientArguments = [string[]]@(
        $validStandardClientArguments + @('-scrDefine', 'ENABLE_DIAG'))
    foreach ($binding in @(
            [pscustomobject]@{
                Role = 'server'
                Stage = 'autosave_checkpoint'
                Arguments = $validDiagnosticServerArguments
            },
            [pscustomobject]@{
                Role = 'client'
                Stage = 'shutdown_checkpoint'
                Arguments = $validDiagnosticClientArguments
            })) {
        Assert-GateScriptSymbolTopology `
            -Arguments ([string[]]$binding.Arguments) `
            -Phase diagnostic-lineage `
            -Role ([string]$binding.Role)
        Assert-RetentionScriptSymbolTopology `
            -Arguments ([string[]]$binding.Arguments) `
            -Phase diagnostic-lineage `
            -Role ([string]$binding.Role) `
            -Stage ([string]$binding.Stage)
    }
    Assert-EngineLaunchTopology `
        -Stage autosave_checkpoint `
        -Role server `
        -Arguments $validDiagnosticServerArguments `
        -CandidateId $candidateId `
        -PackageSha256 $packageSha `
        -ManifestSha256 ([string]$manifestSignature.sha256) `
        -AddonGuid '698532771130111D' `
        -WorldResource 'Worlds/HST_Everon/HST_Everon.ent' `
        -MissionHeader 'Missions/HST_Everon.conf' `
        -RunId $runId `
        -SessionNonce $runNonce `
        -StageNonce $diagnosticStageNonce `
        -Phase diagnostic-lineage
    Assert-EngineLaunchTopology `
        -Stage shutdown_checkpoint `
        -Role client `
        -Arguments $validDiagnosticClientArguments `
        -CandidateId $candidateId `
        -PackageSha256 $packageSha `
        -ManifestSha256 ([string]$manifestSignature.sha256) `
        -AddonGuid '698532771130111D' `
        -WorldResource 'Worlds/HST_Everon/HST_Everon.ent' `
        -MissionHeader 'Missions/HST_Everon.conf' `
        -RunId $runId `
        -SessionNonce $runNonce `
        -StageNonce $diagnosticStageNonce `
        -Phase diagnostic-lineage
    [void]$checks.Add('diagnostic-script-symbol-pairs-accepted')

    $diagnosticMutations = @(
        [pscustomobject]@{
            Label = 'missing'
            Arguments = [string[]]$validDiagnosticServerArguments[
                0..($validDiagnosticServerArguments.Count - 3)]
        },
        [pscustomobject]@{
            Label = 'wrong-symbol'
            Arguments = [string[]]@(
                $validDiagnosticServerArguments[0..(
                    $validDiagnosticServerArguments.Count - 2)] + @('WRONG_SYMBOL'))
        },
        [pscustomobject]@{
            Label = 'duplicate'
            Arguments = [string[]]@(
                $validDiagnosticServerArguments + @('-scrDefine', 'ENABLE_DIAG'))
        },
        [pscustomobject]@{
            Label = 'option-case'
            Arguments = [string[]]@(
                $validDiagnosticServerArguments[0..(
                    $validDiagnosticServerArguments.Count - 3)] +
                @('-SCRDEFINE', 'ENABLE_DIAG'))
        },
        [pscustomobject]@{
            Label = 'symbol-case'
            Arguments = [string[]]@(
                $validDiagnosticServerArguments[0..(
                    $validDiagnosticServerArguments.Count - 2)] + @('enable_diag'))
        },
        [pscustomobject]@{
            Label = 'inline'
            Arguments = [string[]]@(
                $validDiagnosticServerArguments[0..(
                    $validDiagnosticServerArguments.Count - 3)] +
                @('-scrDefine=ENABLE_DIAG'))
        })
    foreach ($mutation in $diagnosticMutations) {
        Assert-TestRejected "runner diagnostic symbol $($mutation.Label)" {
            Assert-GateScriptSymbolTopology `
                -Arguments ([string[]]$mutation.Arguments) `
                -Phase diagnostic-lineage `
                -Role server
        } 'script-symbol'
        Assert-TestRejected "index diagnostic symbol $($mutation.Label)" {
            Assert-RetentionScriptSymbolTopology `
                -Arguments ([string[]]$mutation.Arguments) `
                -Phase diagnostic-lineage `
                -Role server `
                -Stage autosave_checkpoint
        } 'script-symbol'
        [void]$checks.Add(
            'diagnostic-script-symbol-' + [string]$mutation.Label + '-rejected')
    }

    foreach ($phaseBinding in @(
            [pscustomobject]@{
                Role = 'server'
                Stage = 'autosave_checkpoint'
                Arguments = $validStandardServerArguments
            },
            [pscustomobject]@{
                Role = 'client'
                Stage = 'shutdown_checkpoint'
                Arguments = $validStandardClientArguments
            })) {
        foreach ($injection in @(
                [string[]]@('-scrDefine', 'ENABLE_DIAG'),
                [string[]]@('-scrDefine', 'OTHER_SYMBOL'),
                [string[]]@('-SCRDEFINE', 'ENABLE_DIAG'),
                [string[]]@('-scrDefine=ENABLE_DIAG'),
                [string[]]@('ENABLE_DIAG'),
                [string[]]@('enable_diag'))) {
            $injected = [string[]]@(
                [string[]]$phaseBinding.Arguments + [string[]]$injection)
            Assert-TestRejected `
                "runner standard $($phaseBinding.Role) script-symbol injection" {
                Assert-GateScriptSymbolTopology `
                    -Arguments $injected `
                    -Phase standard-retention `
                    -Role ([string]$phaseBinding.Role)
            } 'script-symbol'
            Assert-TestRejected `
                "index standard $($phaseBinding.Role) script-symbol injection" {
                Assert-RetentionScriptSymbolTopology `
                    -Arguments $injected `
                    -Phase standard-retention `
                    -Role ([string]$phaseBinding.Role) `
                    -Stage ([string]$phaseBinding.Stage)
            } 'script-symbol'
        }
        [void]$checks.Add(
            'standard-' + [string]$phaseBinding.Role +
            '-all-script-symbols-rejected')
    }
    $badAddon = [string[]]$validStandardServerArguments.Clone()
    $badAddon[[Array]::IndexOf($badAddon, '-addons') + 1] =
        '0000000000000000'
    Assert-TestRejected 'standard server candidate GUID' {
        Assert-EngineLaunchTopology `
            -Stage autosave_checkpoint -Role server -Arguments $badAddon `
            -CandidateId $candidateId -PackageSha256 $packageSha `
            -ManifestSha256 ([string]$manifestSignature.sha256) `
            -AddonGuid '698532771130111D' `
            -WorldResource 'Worlds/HST_Everon/HST_Everon.ent' `
            -MissionHeader 'Missions/HST_Everon.conf' `
            -Phase standard-retention
    } 'not bound to the retained candidate'
    [void]$checks.Add('server-addon-guid-rejected')
    $badManifest = [string[]]$validStandardServerArguments.Clone()
    $badManifest[[Array]::IndexOf(
            $badManifest, '-hstReleaseManifestSha256') + 1] = '0' * 64
    Assert-TestRejected 'standard server manifest binding' {
        Assert-EngineLaunchTopology `
            -Stage autosave_checkpoint -Role server -Arguments $badManifest `
            -CandidateId $candidateId -PackageSha256 $packageSha `
            -ManifestSha256 ([string]$manifestSignature.sha256) `
            -AddonGuid '698532771130111D' `
            -WorldResource 'Worlds/HST_Everon/HST_Everon.ent' `
            -MissionHeader 'Missions/HST_Everon.conf' `
            -Phase standard-retention
    } 'not bound to the retained candidate'
    [void]$checks.Add('manifest-launch-binding-rejected')
    $badWorld = [string[]]$validStandardServerArguments.Clone()
    $badWorld[[Array]::IndexOf($badWorld, '-server') + 1] =
        'Worlds/Incorrect.ent'
    Assert-TestRejected 'standard server world binding' {
        Assert-EngineLaunchTopology `
            -Stage autosave_checkpoint -Role server -Arguments $badWorld `
            -CandidateId $candidateId -PackageSha256 $packageSha `
            -ManifestSha256 ([string]$manifestSignature.sha256) `
            -AddonGuid '698532771130111D' `
            -WorldResource 'Worlds/HST_Everon/HST_Everon.ent' `
            -MissionHeader 'Missions/HST_Everon.conf' `
            -Phase standard-retention
    } 'world or mission header is not exact'
    [void]$checks.Add('server-world-binding-rejected')
    $badMissionHeader = [string[]]$validStandardServerArguments.Clone()
    $badMissionHeader[[Array]::IndexOf(
            $badMissionHeader, '-MissionHeader') + 1] =
        'Missions/Incorrect.conf'
    Assert-TestRejected 'standard server mission header binding' {
        Assert-EngineLaunchTopology `
            -Stage autosave_checkpoint -Role server `
            -Arguments $badMissionHeader `
            -CandidateId $candidateId -PackageSha256 $packageSha `
            -ManifestSha256 ([string]$manifestSignature.sha256) `
            -AddonGuid '698532771130111D' `
            -WorldResource 'Worlds/HST_Everon/HST_Everon.ent' `
            -MissionHeader 'Missions/HST_Everon.conf' `
            -Phase standard-retention
    } 'world or mission header is not exact'
    [void]$checks.Add('server-mission-header-binding-rejected')
    foreach ($authority in @(
            '-scrDefine', '-scrDefine=ENABLE_DIAG', '-autotest')) {
        Assert-TestRejected "standard authority $authority" {
            Assert-EngineLaunchTopology `
                -Stage autosave_checkpoint -Role server `
                -Arguments ([string[]]@($validStandardServerArguments +
                        @($authority, 'ENABLE_DIAG'))) `
                -CandidateId $candidateId -PackageSha256 $packageSha `
                -ManifestSha256 ([string]$manifestSignature.sha256) `
                -AddonGuid '698532771130111D' `
                -WorldResource 'Worlds/HST_Everon/HST_Everon.ent' `
                -MissionHeader 'Missions/HST_Everon.conf' `
                -Phase standard-retention
        } 'test or diagnostic authority'
    }
    [void]$checks.Add('standard-generic-authority-rejected')
    Assert-TestRejected 'standard unrelated launch option' {
        Assert-EngineLaunchTopology `
            -Stage autosave_checkpoint -Role server `
            -Arguments ([string[]]@($validStandardServerArguments +
                    @('-unrelatedOption', 'value'))) `
            -CandidateId $candidateId -PackageSha256 $packageSha `
            -ManifestSha256 ([string]$manifestSignature.sha256) `
            -AddonGuid '698532771130111D' `
            -WorldResource 'Worlds/HST_Everon/HST_Everon.ent' `
            -MissionHeader 'Missions/HST_Everon.conf' `
            -Phase standard-retention
    } 'exact allowed option vector'
    [void]$checks.Add('standard-unrelated-option-rejected')
    $badClientArguments = [string[]]@(
        '-gproj', 'packed/addon.gproj',
        '-addonsDir', 'runtime,packed',
        '-addons', '0000000000000000',
        '-client',
        '-profile', 'profile',
        '-logsDir', 'logs',
        '-addonTempDir', 'addon-temp',
        '-hstReleaseCandidateId', $candidateId,
        '-hstReleasePackageSha256', $packageSha,
        '-hstReleaseManifestSha256', [string]$manifestSignature.sha256)
    Assert-TestRejected 'standard client candidate GUID' {
        Assert-EngineLaunchTopology `
            -Stage shutdown_checkpoint -Role client `
            -Arguments $badClientArguments `
            -CandidateId $candidateId -PackageSha256 $packageSha `
            -ManifestSha256 ([string]$manifestSignature.sha256) `
            -AddonGuid '698532771130111D' `
            -WorldResource 'Worlds/HST_Everon/HST_Everon.ent' `
            -MissionHeader 'Missions/HST_Everon.conf' `
            -Phase standard-retention
    } 'not bound to the retained candidate'
    [void]$checks.Add('client-addon-guid-rejected')

    $wrongNamespaceState = @{
        'files/native/saves/wrong/meta-info.json' = [pscustomobject]@{
            Kind = 'native'
            Text = "wrong-native-namespace`n"
        }
        'files/journal/profile/Partisan/HST_CampaignSaveData.json' =
            [pscustomobject]@{ Kind = 'journal'; Text = "canonical`n" }
    }
    $wrongNamespaceSnapshot = New-TestSnapshot `
        (Join-Path $tempRoot 'wrong-native-namespace') `
        'autosave_checkpoint' 'output' $wrongNamespaceState
    Assert-TestRejected 'snapshot native namespace' {
        Assert-SnapshotManifest `
            -RunRoot $tempRoot `
            -ManifestPath (ConvertTo-TestPortablePath `
                $tempRoot $wrongNamespaceSnapshot.Path) `
            -Stage 'autosave_checkpoint' `
            -Direction 'output' `
            -ExpectedDigest ([string]$wrongNamespaceSnapshot.Digest) `
            -ExpectedJournalCount 1 | Out-Null
    } 'kind and path namespace differ'
    [void]$checks.Add('snapshot-native-namespace-rejected')

    $missingJournalSnapshot = New-TestSnapshot `
        (Join-Path $tempRoot 'missing-journal-snapshot') `
        'autosave_checkpoint' 'output' `
        (New-TestNativeSaveRows @($saveIds[0]))
    Assert-TestRejected 'snapshot missing journal' {
        Assert-SnapshotManifest `
            -RunRoot $tempRoot `
            -ManifestPath (ConvertTo-TestPortablePath `
                $tempRoot $missingJournalSnapshot.Path) `
            -Stage 'autosave_checkpoint' `
            -Direction 'output' `
            -ExpectedDigest ([string]$missingJournalSnapshot.Digest) `
            -ExpectedJournalCount 1 | Out-Null
    } 'journal set is not exact'
    [void]$checks.Add('snapshot-missing-journal-rejected')

    $emptyJournalState = New-TestNativeSaveRows @($saveIds[0])
    $emptyJournalState[
        'files/journal/profile/Partisan/HST_CampaignSaveData.json'] =
        [pscustomobject]@{ Kind = 'journal'; Text = '' }
    $emptyJournalSnapshot = New-TestSnapshot `
        (Join-Path $tempRoot 'empty-journal-snapshot') `
        'autosave_checkpoint' 'output' $emptyJournalState
    Assert-TestRejected 'snapshot zero-byte journal' {
        Assert-SnapshotManifest `
            -RunRoot $tempRoot `
            -ManifestPath (ConvertTo-TestPortablePath `
                $tempRoot $emptyJournalSnapshot.Path) `
            -Stage 'autosave_checkpoint' `
            -Direction 'output' `
            -ExpectedDigest ([string]$emptyJournalSnapshot.Digest) `
            -ExpectedJournalCount 1 | Out-Null
    } 'invalid snapshot row'
    [void]$checks.Add('snapshot-zero-byte-journal-rejected')

    $duplicateHarnessTools = [object[]]@(
        [pscustomobject][ordered]@{
            role = 'gate1-runner'
            path = 'tools/run-guarded-gate1-runtime-retention.ps1'
            length = 1
            sha256 = '0' * 64
        },
        [pscustomobject][ordered]@{
            role = 'gate1-runner'
            path = 'tools/run-guarded-gate1-runtime-retention.ps1'
            length = 1
            sha256 = '0' * 64
        })
    Assert-TestRejected 'duplicate harness role' {
        Assert-HarnessToolRoleSet `
            -Tools $duplicateHarnessTools `
            -RequireComplete $false
    } 'not exact and unique'
    [void]$checks.Add('duplicate-harness-role-rejected')

    $harnessRepository = Join-Path $tempRoot 'harness-git'
    New-Item -ItemType Directory -Path $harnessRepository -Force | Out-Null
    & git -C $harnessRepository init -q
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic harness Git init failed.' }
    & git -C $harnessRepository config user.name 'Gate 1 Self Test'
    & git -C $harnessRepository config user.email 'gate1-self-test@example.invalid'
    & git -C $harnessRepository config core.autocrlf false
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic harness Git config failed.' }
    foreach ($entry in $script:HarnessToolPaths.GetEnumerator()) {
        $toolPath = Join-Path $harnessRepository `
            ([string]$entry.Value).Replace('/', '\')
        $null = Write-TestText $toolPath `
            ("candidate " + [string]$entry.Key + "`n")
    }
    & git -C $harnessRepository add -- tools
    & git -C $harnessRepository commit -q -m 'candidate harness tools'
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic candidate commit failed.' }
    $candidateHarnessHead = (@(& git -C $harnessRepository rev-parse HEAD) `
        -join '').Trim()
    $fixtureRunnerPath = Join-Path $harnessRepository `
        'tools\run-guarded-gate1-runtime-retention.ps1'
    $null = Write-TestText $fixtureRunnerPath "harness descendant runner`n"
    & git -C $harnessRepository add -- tools
    & git -C $harnessRepository commit -q -m 'descendant harness update'
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic harness commit failed.' }
    $descendantHarnessHead = (@(& git -C $harnessRepository rev-parse HEAD) `
        -join '').Trim()
    $fixtureHarnessTools = New-Object Collections.Generic.List[object]
    foreach ($entry in $script:HarnessToolPaths.GetEnumerator()) {
        $toolPath = Join-Path $harnessRepository `
            ([string]$entry.Value).Replace('/', '\')
        $signature = Get-TestSignature $toolPath
        [void]$fixtureHarnessTools.Add([pscustomobject][ordered]@{
            role = [string]$entry.Key
            path = [string]$entry.Value
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        })
    }
    $fixtureHarnessRows = [object[]]$fixtureHarnessTools.ToArray()
    Assert-HarnessToolRoleSet -Tools $fixtureHarnessRows -RequireComplete $true
    $caseVariantHarnessRows = @($fixtureHarnessRows | ForEach-Object {
        [pscustomobject][ordered]@{
            role = [string]$_.role
            path = [string]$_.path
            length = [long]$_.length
            sha256 = [string]$_.sha256
        }
    })
    $caseVariantHarnessRows[0].role = 'GATE1-RUNNER'
    Assert-TestRejected 'case-variant role replacing gate1-runner' {
        Assert-HarnessToolRoleSet `
            -Tools $caseVariantHarnessRows `
            -RequireComplete $true
    } 'not exact and unique'
    [void]$checks.Add('case-variant-harness-role-rejected')
    Assert-HarnessGitBinding `
        -RepositoryRoot $harnessRepository `
        -CandidateHead $candidateHarnessHead `
        -HarnessHead $descendantHarnessHead `
        -Tools $fixtureHarnessRows
    [void]$checks.Add('harness-descendant-binding')
    Assert-TestRejected 'non-descendant harness commit' {
        Assert-HarnessGitBinding `
            -RepositoryRoot $harnessRepository `
            -CandidateHead $descendantHarnessHead `
            -HarnessHead $candidateHarnessHead `
            -Tools $fixtureHarnessRows
    } 'not a verified candidate descendant'
    [void]$checks.Add('harness-non-descendant-rejected')
    $driftedHarnessRows = @($fixtureHarnessRows | ForEach-Object {
        [pscustomobject][ordered]@{
            role = [string]$_.role
            path = [string]$_.path
            length = [long]$_.length
            sha256 = [string]$_.sha256
        }
    })
    $driftedHarnessRows[0].sha256 = 'f' * 64
    Assert-TestRejected 'harness recorded blob drift' {
        Assert-HarnessGitBinding `
            -RepositoryRoot $harnessRepository `
            -CandidateHead $candidateHarnessHead `
            -HarnessHead $descendantHarnessHead `
            -Tools $driftedHarnessRows
    } 'differs from its worktree or Git blob'
    [void]$checks.Add('harness-blob-drift-rejected')

    $unrelatedTrackedPath = Join-Path $harnessRepository 'docs\unrelated.txt'
    $null = Write-TestText $unrelatedTrackedPath "unrelated descendant`n"
    & git -C $harnessRepository add -- docs/unrelated.txt
    & git -C $harnessRepository commit -q -m 'unrelated descendant'
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic unrelated commit failed.' }
    Assert-TestRejected 'publication checkout newer than harness' {
        Assert-HarnessGitBinding `
            -RepositoryRoot $harnessRepository `
            -CandidateHead $candidateHarnessHead `
            -HarnessHead $descendantHarnessHead `
            -Tools $fixtureHarnessRows
    } 'does not match the retained harness commit'
    $unrelatedDirtyPath = Join-Path $harnessRepository 'unrelated-dirty.txt'
    $null = Write-TestText $unrelatedDirtyPath "unrelated dirty bytes`n"
    Assert-HarnessGitBinding `
        -RepositoryRoot $harnessRepository `
        -CandidateHead $candidateHarnessHead `
        -HarnessHead $descendantHarnessHead `
        -Tools $fixtureHarnessRows `
        -AllowUnrelatedDirty
    [void]$checks.Add('verification-allows-unrelated-descendant-and-dirty-state')
    $fixtureRunnerBytes = [IO.File]::ReadAllBytes($fixtureRunnerPath)
    [IO.File]::AppendAllText($fixtureRunnerPath, 'bound drift')
    Assert-TestRejected 'verification bound tool dirty' {
        Assert-HarnessGitBinding `
            -RepositoryRoot $harnessRepository `
            -CandidateHead $candidateHarnessHead `
            -HarnessHead $descendantHarnessHead `
            -Tools $fixtureHarnessRows `
            -AllowUnrelatedDirty
    } 'differs from its worktree or Git blob'
    [IO.File]::WriteAllBytes($fixtureRunnerPath, $fixtureRunnerBytes)
    [void]$checks.Add('verification-bound-tool-drift-rejected')

    $diagnosticRows = New-Object Collections.Generic.List[object]
    $standardRows = New-Object Collections.Generic.List[object]
    $persistenceRows = New-Object Collections.Generic.List[object]
    $lineageStates = New-Object Collections.Generic.List[object]
    $state = @{}
    $stageNonces = New-Object Collections.Generic.List[string]

    for ($index = 0; $index -lt $stages.Count; $index++) {
        $stage = $stages[$index]
        $stageRoot = Join-Path (Join-Path $runRoot 'raw\stages') `
            $stageDirectories[$index]
        New-Item -ItemType Directory -Path $stageRoot -Force | Out-Null
        $inputState = if ($index -eq 4) {
            $journalOnly = @{}
            foreach ($key in $state.Keys) {
                if ([string]$state[$key].Kind -ceq 'journal') {
                    $journalOnly[$key] = $state[$key]
                }
            }
            Copy-TestState $journalOnly
        }
        else { Copy-TestState $state }
        $input = New-TestSnapshot (Join-Path $stageRoot 'save-input') `
            $stage 'input' $inputState
        if ($index -le 2) {
            $state = New-TestNativeSaveRows $saveIds[0..$index]
            if ($index -eq 0) {
                $state['files/journal/profile/Partisan/HST_CampaignSaveData.json'] =
                    [pscustomobject]@{ Kind = 'journal'; Text = "generation-one`n" }
            }
            else {
                $state['files/journal/profile/Partisan/HST_CampaignSaveData.json'] =
                    [pscustomobject]@{
                        Kind = 'journal'; Text = "generation-$($index + 1)-canonical`n"
                    }
                $state['files/journal/profile/Partisan/HST_CampaignSaveData.recovery.json'] =
                    [pscustomobject]@{
                        Kind = 'journal'; Text = "generation-$index-recovery`n"
                    }
            }
        }
        elseif ($index -eq 4) { $state = Copy-TestState $inputState }
        $output = New-TestSnapshot (Join-Path $stageRoot 'save-output') `
            $stage 'output' (Copy-TestState $state)

        foreach ($leaf in @('owner.json', 'guard-input.json', 'result.json',
                'carrier.json')) {
            $null = Write-TestJson (Join-Path $stageRoot $leaf) ([ordered]@{
                schemaVersion = 1
                stage = $stage
                artifact = $leaf
            })
        }
        if ($stage -ceq 'shutdown_checkpoint') {
            foreach ($leaf in @('mixed-native-ready.json', 'end-bridge.json')) {
                $null = Write-TestJson (Join-Path $stageRoot $leaf) ([ordered]@{
                    schemaVersion = 1
                    stage = $stage
                    artifact = $leaf
                })
            }
        }
        foreach ($role in @('diagnostic-server')) {
            $logRoot = Join-Path $stageRoot ($role + '-logs')
            foreach ($leaf in @('console.log', 'script.log', 'error.log')) {
                $null = Write-TestText (Join-Path $logRoot $leaf) `
                    ("synthetic $stage $role $leaf`n")
            }
        }
        if ($stage -ceq 'shutdown_checkpoint') {
            $logRoot = Join-Path $stageRoot 'diagnostic-client-logs'
            foreach ($leaf in @('console.log', 'script.log', 'error.log')) {
                $null = Write-TestText (Join-Path $logRoot $leaf) `
                    ("synthetic $stage client $leaf`n")
            }
        }

        $context = New-PartisanGuardedRuntimeContext `
            -GuardBase $diagnosticGuardBase `
            -Purpose ('self_test_gate1_' + $index) `
            -WatchedRoots @($watched) `
            -SpillRoots @($spill) `
            -LoopbackPorts @((Get-TestFreeUdpPort))
        $candidateStage = New-PartisanCandidateStage $context $candidate
        $profile = Join-Path $tempRoot ('profile-' + $index)
        $logs = Join-Path $tempRoot ('logs-' + $index)
        $addonTemp = Join-Path $tempRoot ('addon-temp-' + $index)
        foreach ($directory in @($profile, $logs, $addonTemp)) {
            New-Item -ItemType Directory -Path $directory -Force | Out-Null
        }
        $stageNonce = [Guid]::NewGuid().ToString('N')
        [void]$stageNonces.Add($stageNonce)
        $arguments = [string[]]@(
            '-gproj', $candidateStage.PackedProjectPath,
            '-server', 'Worlds/HST_Everon/HST_Everon.ent',
            '-MissionHeader', 'Missions/HST_Everon.conf',
            '-addonsDir', $candidateStage.AddonSearchPath,
            '-addons', '698532771130111D',
            '-profile', $profile,
            '-logLevel', 'normal',
            '-logTime', 'datetime',
            '-noThrow',
            '-maxFPS', '30')
        if ($index -eq 0) {
            $arguments += '-loadSessionSave'
        }
        elseif ($index -le 3) {
            $arguments += @('-loadSessionSave', $saveIds[$index - 1])
        }
        if ($index -eq 2) { $arguments += '-autoshutdown' }
        $arguments += @(
            '-hstOrdinaryCampaignPersistenceProof', 'true',
            '-hstOrdinaryCampaignPersistenceStage', $stage,
            '-hstOrdinaryCampaignPersistenceRunId', $runId,
            '-hstOrdinaryCampaignPersistenceSessionNonce', $runNonce,
            '-hstOrdinaryCampaignPersistenceStageNonce', $stageNonce,
            '-logsDir', $logs,
            '-addonTempDir', $addonTemp,
            '-hstReleaseCandidateId', $candidateId,
            '-hstReleasePackageSha256', $packageSha,
            '-hstReleaseManifestSha256',
                [string]$manifestSignature.sha256,
            '-scrDefine', 'ENABLE_DIAG')
        $serverLaunch = Start-PartisanGuardedServer `
            -Context $context `
            -Executable $sleeperPath `
            -ExecutableProvenance $sleeperProvenance `
            -Arguments $arguments `
            -WorkingDirectory $working `
            -CandidateConsumption $candidateStage `
            -NonEngineSelfTestOnly
        $clientArguments = [string[]]@()
        if ($index -eq 2) {
            $clientArguments = [string[]]@(
                '-gproj', $candidateStage.PackedProjectPath,
                '-addonsDir', $candidateStage.AddonSearchPath,
                '-addons', '698532771130111D',
                '-addonTempDir', $addonTemp,
                '-client',
                '-profile', $profile,
                '-logsDir', $logs,
                '-logLevel', 'normal',
                '-logTime', 'datetime',
                '-window',
                '-noFocus',
                '-forceUpdate',
                '-noSplash',
                '-noSound',
                '-noThrow',
                '-maxFPS', '30',
                '-hstReleaseCandidateId', $candidateId,
                '-hstReleasePackageSha256', $packageSha,
                '-hstReleaseManifestSha256',
                    [string]$manifestSignature.sha256,
                '-scrDefine', 'ENABLE_DIAG')
            $null = Start-PartisanGuardedClient `
                -Context $context `
                -Executable $sleeperPath `
                -ExecutableProvenance $sleeperProvenance `
                -Arguments $clientArguments `
                -WorkingDirectory $working `
                -CandidateConsumption $candidateStage `
                -NonEngineSelfTestOnly
        }
        $teardown = Invoke-PartisanGuardedTeardown $context
        $tested = Test-PartisanGuardedRuntimeReceipt `
            $teardown.CleanReceiptPath $teardown.CleanReceiptSignature
        Assert-TestCondition $tested.Complete "$stage genuine guarded receipt"
        [void]$diagnosticRows.Add([pscustomobject][ordered]@{
            ordinal = $index
            stage = $stage
            contextId = [string]$tested.ContextId
            candidateBindingSha256 = [string]$tested.CandidateBindingSha256
            serverExitCode = 0
            clientLaunched = $index -eq 2
            serverArgumentsSha256 = Get-TestArgumentDigest $arguments
            clientArgumentsSha256 = if ($clientArguments.Count -gt 0) {
                Get-TestArgumentDigest $clientArguments
            }
            else { '' }
            receiptPath = ConvertTo-TestPortablePath $runRoot `
                $teardown.CleanReceiptPath
            receiptSignature = $teardown.CleanReceiptSignature
            journalPath = ConvertTo-TestPortablePath $runRoot $tested.JournalPath
            journalSignature = $tested.JournalSignature
            completionPath = ConvertTo-TestPortablePath $runRoot `
                $tested.CompletionAttestationPath
            completionSignature = $tested.CompletionAttestationSignature
        })
        [void]$lineageStates.Add((Copy-TestState $state))
        $created = if ($index -le 2) { $saveIds[$index] } else { '' }
        $prior = if ($index -eq 0) { '' } else { $saveIds[[Math]::Min($index - 1, 2)] }
        [void]$persistenceRows.Add([pscustomobject][ordered]@{
            ordinal = $index
            stage = $stage
            stageNonce = $stageNonce
            source = @('new_campaign', 'native', 'native', 'native',
                'profile_fallback')[$index]
            saveType = @('auto', 'manual', 'shutdown', 'none', 'none')[$index]
            saveName = @('Partisan autosave', 'Partisan manual checkpoint',
                'Partisan controlled shutdown', '', '')[$index]
            loadSavePointId = if ($index -in @(1, 2, 3)) { $prior } else { '' }
            createdSavePointId = $created
            expectedPriorSavePointId = $prior
            expectedSourceFingerprint = if ($index -eq 0) { '' } else { 'f' * 64 }
            expectedSentinelFingerprint = if ($index -ge 3) { 'e' * 64 } else { '' }
            latestSavePointId = if ($index -le 2) { $created } else { $saveIds[2] }
            sourceFingerprint = 'a' * 64
            finalFingerprint = 'b' * 64
            journalFileCount = if ($index -eq 0) { 1 } else { 2 }
            journalGeneration = [Math]::Min($index + 1, 3)
            journalSlot = if ($index -eq 0) { 'canonical' } else { 'recovery' }
            readOnly = $index -ge 3
            ownerPath = ConvertTo-TestPortablePath $runRoot `
                (Join-Path $stageRoot 'owner.json')
            guardInputPath = ConvertTo-TestPortablePath $runRoot `
                (Join-Path $stageRoot 'guard-input.json')
            resultPath = ConvertTo-TestPortablePath $runRoot `
                (Join-Path $stageRoot 'result.json')
            carrierPath = ConvertTo-TestPortablePath $runRoot `
                (Join-Path $stageRoot 'carrier.json')
            mixedNativeReadyPath = if ($index -eq 2) {
                ConvertTo-TestPortablePath $runRoot `
                    (Join-Path $stageRoot 'mixed-native-ready.json')
            }
            else { '' }
            endBridgePath = if ($index -eq 2) {
                ConvertTo-TestPortablePath $runRoot `
                    (Join-Path $stageRoot 'end-bridge.json')
            }
            else { '' }
            inputSnapshotPath = ConvertTo-TestPortablePath $runRoot $input.Path
            inputSnapshotSha256 = [string]$input.Digest
            outputSnapshotPath = ConvertTo-TestPortablePath $runRoot $output.Path
            outputSnapshotSha256 = [string]$output.Digest
        })
    }
    [void]$checks.Add('five-diagnostic-lineage-contexts')

    for ($index = 0; $index -lt $stages.Count; $index++) {
        $stage = $stages[$index]
        $stageRoot = Join-Path (Join-Path $runRoot 'raw\standard-runtime') `
            $stageDirectories[$index]
        New-Item -ItemType Directory -Path $stageRoot -Force | Out-Null
        $standardInput = New-TestSnapshot `
            (Join-Path $stageRoot 'save-input') $stage 'input' `
            (Copy-TestState $lineageStates[$index])
        $standardOutput = New-TestSnapshot `
            (Join-Path $stageRoot 'save-output') $stage 'output' `
            (Copy-TestState $lineageStates[$index])
        $serverLogRoot = Join-Path $stageRoot 'server-logs'
        foreach ($leaf in @('console.log', 'script.log', 'error.log')) {
            $null = Write-TestText (Join-Path $serverLogRoot $leaf) `
                ("synthetic standard $stage server $leaf`n")
        }
        if ($index -eq 2) {
            $clientLogRoot = Join-Path $stageRoot 'client-logs'
            foreach ($leaf in @('console.log', 'script.log', 'error.log')) {
                $null = Write-TestText (Join-Path $clientLogRoot $leaf) `
                    ("synthetic standard $stage client $leaf`n")
            }
        }

        $context = New-PartisanGuardedRuntimeContext `
            -GuardBase $standardGuardBase `
            -Purpose ('self_test_gate1_standard_' + $index) `
            -WatchedRoots @($watched) `
            -SpillRoots @($spill) `
            -LoopbackPorts @((Get-TestFreeUdpPort))
        $candidateStage = New-PartisanCandidateStage $context $candidate
        $profile = Join-Path $tempRoot ('standard-profile-' + $index)
        $logs = Join-Path $tempRoot ('standard-logs-' + $index)
        $addonTemp = Join-Path $tempRoot ('standard-addon-temp-' + $index)
        foreach ($directory in @($profile, $logs, $addonTemp)) {
            New-Item -ItemType Directory -Path $directory -Force | Out-Null
        }
        $arguments = [string[]]@(
            '-gproj', $candidateStage.PackedProjectPath,
            '-server', 'Worlds/HST_Everon/HST_Everon.ent',
            '-MissionHeader', 'Missions/HST_Everon.conf',
            '-addonsDir', $candidateStage.AddonSearchPath,
            '-addons', '698532771130111D',
            '-profile', $profile,
            '-logsDir', $logs,
            '-addonTempDir', $addonTemp,
            '-logLevel', 'normal',
            '-logTime', 'datetime',
            '-noThrow',
            '-maxFPS', '30')
        if ($index -le 3) {
            $loadIndex = [Math]::Min($index, 2)
            $arguments += @('-loadSessionSave', $saveIds[$loadIndex])
        }
        $arguments += @(
            '-hstReleaseCandidateId', $candidateId,
            '-hstReleasePackageSha256', $packageSha,
            '-hstReleaseManifestSha256',
                [string]$manifestSignature.sha256)
        $serverLaunch = Start-PartisanGuardedServer `
            -Context $context `
            -Executable $sleeperPath `
            -ExecutableProvenance $sleeperProvenance `
            -Arguments $arguments `
            -WorkingDirectory $working `
            -CandidateConsumption $candidateStage `
            -NonEngineSelfTestOnly
        $clientArguments = [string[]]@()
        if ($index -eq 2) {
            $clientArguments = [string[]]@(
                '-gproj', $candidateStage.PackedProjectPath,
                '-addonsDir', $candidateStage.AddonSearchPath,
                '-addons', '698532771130111D',
                '-addonTempDir', $addonTemp,
                '-client',
                '-profile', $profile,
                '-logsDir', $logs,
                '-logLevel', 'normal',
                '-logTime', 'datetime',
                '-window',
                '-noFocus',
                '-forceUpdate',
                '-noSplash',
                '-noSound',
                '-noThrow',
                '-maxFPS', '30',
                '-hstReleaseCandidateId', $candidateId,
                '-hstReleasePackageSha256', $packageSha,
                '-hstReleaseManifestSha256',
                    [string]$manifestSignature.sha256)
            $null = Start-PartisanGuardedClient `
                -Context $context `
                -Executable $sleeperPath `
                -ExecutableProvenance $sleeperProvenance `
                -Arguments $clientArguments `
                -WorkingDirectory $working `
                -CandidateConsumption $candidateStage `
                -NonEngineSelfTestOnly
        }
        $teardown = Invoke-PartisanGuardedTeardown $context
        $tested = Test-PartisanGuardedRuntimeReceipt `
            $teardown.CleanReceiptPath $teardown.CleanReceiptSignature
        Assert-TestCondition $tested.Complete "$stage standard guarded receipt"
        [void]$standardRows.Add([pscustomobject][ordered]@{
            ordinal = $index
            stage = $stage
            contextId = [string]$tested.ContextId
            candidateBindingSha256 = [string]$tested.CandidateBindingSha256
            serverStopDisposition =
                'guarded-external-stop-after-log-readiness'
            readinessEvidence = 'process-alive-and-console-game-created'
            clientLaunched = $index -eq 2
            serverArgumentsSha256 = Get-TestArgumentDigest $arguments
            clientArgumentsSha256 = if ($clientArguments.Count) {
                Get-TestArgumentDigest $clientArguments
            }
            else { '' }
            receiptPath = ConvertTo-TestPortablePath $runRoot `
                $teardown.CleanReceiptPath
            receiptSignature = $teardown.CleanReceiptSignature
            journalPath = ConvertTo-TestPortablePath $runRoot $tested.JournalPath
            journalSignature = $tested.JournalSignature
            completionPath = ConvertTo-TestPortablePath $runRoot `
                $tested.CompletionAttestationPath
            completionSignature = $tested.CompletionAttestationSignature
            inputSnapshotPath = ConvertTo-TestPortablePath $runRoot `
                $standardInput.Path
            inputSnapshotSha256 = [string]$standardInput.Digest
            outputSnapshotPath = ConvertTo-TestPortablePath $runRoot `
                $standardOutput.Path
            outputSnapshotSha256 = [string]$standardOutput.Digest
        })
    }
    [void]$checks.Add('five-disjoint-standard-contexts')

    $launchContractPath = Join-Path $runRoot 'configuration\launch-contract.json'
    $launchContractValue = [pscustomobject][ordered]@{
        schemaVersion = 1
        contractId = 'partisan.gate1-runtime-retention.v1'
        runId = $runId
        sessionNonce = $runNonce
        payloadNonce = $payloadNonce
        buildIdentity = [ordered]@{
            BuildSha = '2' * 40
            BuildUtc = '2026-07-20T00:00:00Z'
            BuildLabel = 'synthetic-self-test'
            CampaignSchemaVersion = 71
            SettingsSchemaVersion = 24
        }
        worldResource = 'Worlds/HST_Everon/HST_Everon.ent'
        missionHeader = 'Missions/HST_Everon.conf'
        projectId = '698532771130111D'
        scriptSymbolTopology = [ordered]@{
            diagnosticOption = '-scrDefine'
            diagnosticSymbol = 'ENABLE_DIAG'
            diagnosticServerLaunchCount = 5
            diagnosticClientLaunchCount = 1
            standardPolicy = 'reject-all-script-symbols'
            standardServerLaunchCount = 5
            standardClientLaunchCount = 1
        }
        stages = [object[]]@($persistenceRows.ToArray() | ForEach-Object {
            [ordered]@{
                ordinal = $_.ordinal
                stage = $_.stage
                stageNonce = $_.stageNonce
                loadSavePointId = $_.loadSavePointId
                latestSavePointId = $_.latestSavePointId
            }
        })
    }
    $launchSignature = Write-TestJson $launchContractPath $launchContractValue

    $packageEvidenceRows = @($packageRows.ToArray() | ForEach-Object {
        $leaf = Split-Path -Leaf ([string]$_.indexPath)
        [pscustomobject][ordered]@{
            indexPath = [string]$_.indexPath
            path = 'identity/package/Partisan/' + $leaf
            length = [long]$_.length
            sha256 = [string]$_.sha256
        }
    })
    $fileRows = New-Object Collections.Generic.List[object]
    foreach ($file in @(Get-ChildItem -LiteralPath $runRoot -Recurse -File -Force |
            Sort-Object FullName)) {
        $portable = ConvertTo-TestPortablePath $runRoot $file.FullName
        $role = if ($portable -ceq 'run.owner.json') { 'run_owner' }
            elseif ($portable -ceq 'identity/candidate.json') { 'candidate_manifest' }
            elseif ($portable -ceq 'identity/candidate.ready.json') { 'candidate_ready' }
            elseif ($portable -like 'identity/package/*') { 'candidate_package' }
            elseif ($portable -ceq 'configuration/HST_Settings.json') { 'runtime_settings' }
            elseif ($portable -ceq 'configuration/launch-contract.json') { 'launch_contract' }
            elseif ($portable -like 'raw/guarded-runtime/*.receipt.json') { 'guarded_runtime_receipt' }
            elseif ($portable -like 'raw/guarded-runtime/*.journal.json') { 'guarded_runtime_journal' }
            elseif ($portable -like 'raw/guarded-runtime/*.completion.json') { 'guarded_runtime_completion' }
            elseif ($portable -like 'raw/diagnostic-guarded-runtime/*.receipt.json') { 'diagnostic_guarded_runtime_receipt' }
            elseif ($portable -like 'raw/diagnostic-guarded-runtime/*.journal.json') { 'diagnostic_guarded_runtime_journal' }
            elseif ($portable -like 'raw/diagnostic-guarded-runtime/*.completion.json') { 'diagnostic_guarded_runtime_completion' }
            elseif ($portable -like '*/save-*/snapshot.json') { 'save_snapshot_manifest' }
            elseif ($portable -like '*/save-input/files/native/*') { 'native_save_input' }
            elseif ($portable -like '*/save-output/files/native/*') { 'native_save_output' }
            elseif ($portable -like '*/save-input/files/journal/*') { 'profile_journal_input' }
            elseif ($portable -like '*/save-output/files/journal/*') { 'profile_journal_output' }
            elseif ($portable -like '*/server-logs/console.log') { 'server_console_log' }
            elseif ($portable -like '*/server-logs/script.log') { 'server_script_log' }
            elseif ($portable -like '*/server-logs/error.log') { 'server_error_log' }
            elseif ($portable -like '*/server-logs/crash.log') { 'server_crash_log' }
            elseif ($portable -like '*/client-logs/console.log') { 'client_console_log' }
            elseif ($portable -like '*/client-logs/script.log') { 'client_script_log' }
            elseif ($portable -like '*/client-logs/error.log') { 'client_error_log' }
            elseif ($portable -like '*/client-logs/crash.log') { 'client_crash_log' }
            elseif ($portable -like '*/diagnostic-server-logs/console.log') { 'diagnostic_server_console_log' }
            elseif ($portable -like '*/diagnostic-server-logs/script.log') { 'diagnostic_server_script_log' }
            elseif ($portable -like '*/diagnostic-server-logs/error.log') { 'diagnostic_server_error_log' }
            elseif ($portable -like '*/diagnostic-server-logs/crash.log') { 'diagnostic_server_crash_log' }
            elseif ($portable -like '*/diagnostic-client-logs/console.log') { 'diagnostic_client_console_log' }
            elseif ($portable -like '*/diagnostic-client-logs/script.log') { 'diagnostic_client_script_log' }
            elseif ($portable -like '*/diagnostic-client-logs/error.log') { 'diagnostic_client_error_log' }
            elseif ($portable -like '*/diagnostic-client-logs/crash.log') { 'diagnostic_client_crash_log' }
            else {
                switch -CaseSensitive ($file.Name) {
                    'owner.json' { 'ordinary_owner'; break }
                    'guard-input.json' { 'ordinary_guard'; break }
                    'result.json' { 'ordinary_result'; break }
                    'carrier.json' { 'ordinary_carrier'; break }
                    'mixed-native-ready.json' { 'ordinary_mixed_native_ready'; break }
                    'end-bridge.json' { 'ordinary_end_bridge'; break }
                    default { throw "Synthetic file role is unknown: $portable" }
                }
            }
        $stageName = 'run'
        for ($index = 0; $index -lt $stageDirectories.Count; $index++) {
            if ($portable -like ('raw/stages/' + $stageDirectories[$index] + '/*')) {
                $stageName = $stages[$index]
            }
            if ($portable -like ('raw/standard-runtime/' +
                    $stageDirectories[$index] + '/*')) {
                $stageName = $stages[$index]
            }
        }
        $signature = Get-TestSignature $file.FullName
        [void]$fileRows.Add([pscustomobject][ordered]@{
            role = [string]$role
            stage = $stageName
            path = $portable
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        })
    }
    $run = [ordered]@{
        schemaVersion = 1
        evidenceKind = 'packaged-gate1-runtime-retention'
        contractId = 'partisan.gate1-runtime-retention.v1'
        runId = $runId
        startedUtc = '2026-07-20T00:00:00.0000000Z'
        completedUtc = '2026-07-20T00:01:00.0000000Z'
        candidate = [ordered]@{
            candidateId = $candidateId
            candidateVersion = '0.0.0-self-test'
            runtimeUseDisposition = 'active-runtime-candidate'
            gitHead = $gitHead
            embeddedBuildSha = '2' * 40
            embeddedBuildUtc = '2026-07-20T00:00:00Z'
            embeddedBuildLabel = 'synthetic-self-test'
            campaignSchema = 71
            runtimeSettingsSchema = 24
            addonId = 'histasi'
            addonGuid = '698532771130111D'
            packageHashAlgorithm = 'sha256-manifest-v1'
            packageSha256 = $packageSha
            manifestSha256 = [string]$manifestSignature.sha256
            readySha256 = [string]$readySignature.sha256
            workbenchCrc = '00000000'
            manifestPath = 'identity/candidate.json'
            readyPath = 'identity/candidate.ready.json'
            packageRoot = 'identity/package/Partisan'
            packageFiles = [object[]]$packageEvidenceRows
            executables = [ordered]@{
                serverDiagnostic = $serverDiagnosticIdentity
                clientDiagnostic = $clientDiagnosticIdentity
                server = $serverIdentity
                client = $clientIdentity
            }
        }
        harness = [ordered]@{
            gitHead = $gitHead
            clean = $true
            tools = @()
        }
        scenario = [ordered]@{
            executionMode = 'synthetic-self-test'
            worldResource = 'Worlds/HST_Everon/HST_Everon.ent'
            missionHeader = 'Missions/HST_Everon.conf'
            stages = [string[]]$stages
            claimScope = 'raw-retention-only'
            certificationClaim = 'none'
        }
        configuration = [ordered]@{
            settingsPath = 'configuration/HST_Settings.json'
            settingsSha256 = [string]$settingsSignature.sha256
            launchContractPath = 'configuration/launch-contract.json'
            launchContractSha256 = [string]$launchSignature.sha256
            autosaveIntervalSeconds = 60
            majorChangeDebounceSeconds = 120
        }
        lineage = [ordered]@{
            executionClass = 'diagnostic-only-save-lineage'
            retailClaim = 'none'
            contexts = [object[]]$diagnosticRows.ToArray()
        }
        runtime = [ordered]@{
            executionClass = 'standard-load-start-log-retention'
            mutationAuthority = 'none'
            byteStabilityClaim = 'observation-only'
            contexts = [object[]]$standardRows.ToArray()
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
        files = [object[]]@($fileRows.ToArray() | Sort-Object path)
    }

    $badCandidateBinding = $run.candidate |
        ConvertTo-Json -Depth 32 | ConvertFrom-Json
    $badCandidateBinding.candidateVersion = '0.0.0-drifted'
    Assert-TestRejected 'candidate manifest cross-binding' {
        Assert-CandidateManifestBinding `
            -Candidate $badCandidateBinding `
            -Manifest $manifestValue `
            -Ready $readyValue `
            -ManifestSha256 ([string]$manifestSignature.sha256)
    } 'not cross-bound'
    [void]$checks.Add('candidate-manifest-cross-binding-rejected')

    $badLaunchRunBinding = $launchContractValue |
        ConvertTo-Json -Depth 32 | ConvertFrom-Json
    $badLaunchRunBinding.buildIdentity.BuildLabel = 'drifted-build-label'
    Assert-TestRejected 'launch contract candidate cross-binding' {
        Assert-LaunchContractRunBinding `
            -Contract $badLaunchRunBinding `
            -Run ([pscustomobject]$run)
    } 'not cross-bound to candidate and scenario'
    [void]$checks.Add('launch-contract-run-binding-rejected')

    $badLaunchSymbolBinding = $launchContractValue |
        ConvertTo-Json -Depth 32 | ConvertFrom-Json
    $badLaunchSymbolBinding.scriptSymbolTopology.diagnosticSymbol = 'enable_diag'
    Assert-TestRejected 'launch contract diagnostic script symbol' {
        Assert-LaunchContractRunBinding `
            -Contract $badLaunchSymbolBinding `
            -Run ([pscustomobject]$run)
    } 'script-symbol topology is invalid'
    [void]$checks.Add('launch-contract-script-symbol-binding-rejected')

    $badLaunchPersistenceBinding = $launchContractValue |
        ConvertTo-Json -Depth 32 | ConvertFrom-Json
    $badLaunchPersistenceBinding.stages[2].latestSavePointId = $saveIds[0]
    Assert-TestRejected 'launch contract persistence cross-binding' {
        Assert-LaunchContractPersistenceBinding `
            -Contract $badLaunchPersistenceBinding `
            -Persistence ([object[]]$persistenceRows.ToArray())
    } 'not persistence-bound'
    [void]$checks.Add('launch-contract-persistence-binding-rejected')

    $runPath = Join-Path $runRoot 'run.json'
    $null = Write-TestJson $runPath $run
    Assert-TestCondition (-not (Test-Path (Join-Path $runRoot 'run.ready.json'))) `
        'ready marker absent before synthetic validation'
    $releaseIndexPath = Join-Path $runRoot 'release-index.json'
    Assert-TestRejected 'normal synthetic publication' {
        & $producerPath -RunEnvelopePath $runPath | Out-Null
    } 'validation-only and cannot be published'
    Assert-TestCondition (-not (Test-Path -LiteralPath $releaseIndexPath)) `
        'normal publisher did not write a synthetic release index'
    [void]$checks.Add('normal-synthetic-publication-rejected')

    $accepted = & $producerPath `
        -RunEnvelopePath $runPath `
        -SyntheticValidationOnly
    Assert-TestCondition (
        -not (Test-Path -LiteralPath $releaseIndexPath) -and
        [string]$accepted.Disposition -ceq
            'validated-synthetic-fixture-only') `
        'valid synthetic retained fixture validated without publication'
    [void]$checks.Add('strict-valid-fixture')
    Assert-TestCondition (
        @($run.files | Where-Object {
                [string]$_.role -like '*_crash_log'
            }).Count -eq 0) `
        'valid synthetic fixture contains no crash.log rows'
    [void]$checks.Add('producer-absent-crash-logs-accepted')

    $runBytes = [IO.File]::ReadAllBytes($runPath)
    $optionalCrashPath = Join-Path $runRoot `
        'raw\standard-runtime\00-autosave_checkpoint\server-logs\crash.log'
    $optionalCrashSignature = Write-TestText $optionalCrashPath `
        "synthetic optional retained crash log`n"
    $optionalCrashPortable = ConvertTo-TestPortablePath $runRoot $optionalCrashPath
    $optionalCrashRun = $run | ConvertTo-Json -Depth 64 | ConvertFrom-Json
    $optionalCrashRun.files = [object[]]@(
        @($optionalCrashRun.files) +
        @([pscustomobject][ordered]@{
            role = 'server_crash_log'
            stage = 'autosave_checkpoint'
            path = $optionalCrashPortable
            length = [long]$optionalCrashSignature.length
            sha256 = [string]$optionalCrashSignature.sha256
        }))
    $null = Write-TestJson $runPath $optionalCrashRun
    $acceptedOptionalCrash = & $producerPath `
        -RunEnvelopePath $runPath `
        -SyntheticValidationOnly
    Assert-TestCondition (
        [string]$acceptedOptionalCrash.Disposition -ceq
            'validated-synthetic-fixture-only') `
        'producer accepts and hashes one optional crash.log context'
    [void]$checks.Add('producer-present-crash-log-accepted')

    $duplicateCrashRun = $optionalCrashRun | ConvertTo-Json -Depth 64 |
        ConvertFrom-Json
    $duplicateCrashRow = @($duplicateCrashRun.files | Where-Object {
            [string]$_.path -ceq $optionalCrashPortable
        })
    Assert-TestCondition ($duplicateCrashRow.Count -eq 1) `
        'optional crash row is exact before duplicate test'
    $duplicateCrashRun.files = [object[]]@(
        @($duplicateCrashRun.files) + @($duplicateCrashRow[0]))
    $null = Write-TestJson $runPath $duplicateCrashRun
    Assert-TestRejected 'duplicate optional crash census row' {
        & $producerPath -RunEnvelopePath $runPath `
            -SyntheticValidationOnly | Out-Null
    } 'role or stage is not derived'
    [void]$checks.Add('producer-duplicate-crash-row-rejected')

    $unknownLogPath = Join-Path $runRoot `
        'raw\standard-runtime\00-autosave_checkpoint\server-logs\unknown.log'
    $null = Write-TestText $unknownLogPath "synthetic unknown retained log`n"
    $null = Write-TestJson $runPath $optionalCrashRun
    Assert-TestRejected 'unknown retained log file' {
        & $producerPath -RunEnvelopePath $runPath `
            -SyntheticValidationOnly | Out-Null
    } 'file census is not complete and exact'
    Remove-Item -LiteralPath $unknownLogPath -Force
    [void]$checks.Add('producer-unknown-log-file-rejected')

    Remove-Item -LiteralPath $optionalCrashPath -Force
    [IO.File]::WriteAllBytes($runPath, $runBytes)

    $requiredLogPath = Join-Path $runRoot `
        'raw\standard-runtime\00-autosave_checkpoint\server-logs\console.log'
    $requiredLogBytes = [IO.File]::ReadAllBytes($requiredLogPath)
    Remove-Item -LiteralPath $requiredLogPath -Force
    $missingRequiredRun = $run | ConvertTo-Json -Depth 64 | ConvertFrom-Json
    $missingRequiredRun.files = [object[]]@(
        $missingRequiredRun.files | Where-Object {
            [string]$_.path -cne
                'raw/standard-runtime/00-autosave_checkpoint/' +
                'server-logs/console.log'
        })
    $null = Write-TestJson $runPath $missingRequiredRun
    Assert-TestRejected 'missing required console log' {
        & $producerPath -RunEnvelopePath $runPath `
            -SyntheticValidationOnly | Out-Null
    } 'does not contain five server_console_log files'
    [IO.File]::WriteAllBytes($requiredLogPath, $requiredLogBytes)
    [IO.File]::WriteAllBytes($runPath, $runBytes)
    [void]$checks.Add('producer-missing-required-log-rejected')

    $stringFalse = $run | ConvertTo-Json -Depth 64 | ConvertFrom-Json
    $stringFalse.outcome.success = 'false'
    $null = Write-TestJson $runPath $stringFalse
    Assert-TestRejected 'string false outcome' {
        & $producerPath -RunEnvelopePath $runPath `
            -SyntheticValidationOnly | Out-Null
    } 'exact JSON boolean'
    [IO.File]::WriteAllBytes($runPath, $runBytes)
    [void]$checks.Add('string-false-boolean-rejected')

    $numericString = $run | ConvertTo-Json -Depth 64 | ConvertFrom-Json
    $numericString.outcome.diagnosticServerLaunchCount = '5'
    $null = Write-TestJson $runPath $numericString
    Assert-TestRejected 'numeric string launch count' {
        & $producerPath -RunEnvelopePath $runPath `
            -SyntheticValidationOnly | Out-Null
    } 'exact integral JSON number'
    [IO.File]::WriteAllBytes($runPath, $runBytes)
    [void]$checks.Add('numeric-string-count-rejected')

    $fractionalCount = $run | ConvertTo-Json -Depth 64 | ConvertFrom-Json
    $fractionalCount.persistence.stages[0].journalGeneration = 1.5
    $null = Write-TestJson $runPath $fractionalCount
    Assert-TestRejected 'fractional persistence count' {
        & $producerPath -RunEnvelopePath $runPath `
            -SyntheticValidationOnly | Out-Null
    } 'exact integral JSON number'
    [IO.File]::WriteAllBytes($runPath, $runBytes)
    [void]$checks.Add('fractional-count-rejected')

    $nonStringIdentity = $run | ConvertTo-Json -Depth 64 | ConvertFrom-Json
    $nonStringIdentity.candidate.candidateVersion = 7
    $null = Write-TestJson $runPath $nonStringIdentity
    Assert-TestRejected 'non-string candidate identity' {
        & $producerPath -RunEnvelopePath $runPath `
            -SyntheticValidationOnly | Out-Null
    } 'exact JSON string'
    [IO.File]::WriteAllBytes($runPath, $runBytes)
    [void]$checks.Add('non-string-identity-rejected')

    $logPath = Join-Path $runRoot `
        'raw\standard-runtime\00-autosave_checkpoint\server-logs\console.log'
    $logBytes = [IO.File]::ReadAllBytes($logPath)
    [IO.File]::AppendAllText($logPath, 'tamper')
    Assert-TestRejected 'payload tamper' {
        & $producerPath -RunEnvelopePath $runPath `
            -SyntheticValidationOnly | Out-Null
    } 'differs from its run census'
    [IO.File]::WriteAllBytes($logPath, $logBytes)
    [void]$checks.Add('payload-tamper-rejected')

    $receiptPath = Join-Path $runRoot `
        ([string]$standardRows[0].receiptPath).Replace('/', '\')
    $receiptBytes = [IO.File]::ReadAllBytes($receiptPath)
    [IO.File]::AppendAllText($receiptPath, ' ')
    Assert-TestRejected 'guarded receipt tamper' {
        & $producerPath -RunEnvelopePath $runPath `
            -SyntheticValidationOnly | Out-Null
    } 'signature|differs'
    [IO.File]::WriteAllBytes($receiptPath, $receiptBytes)
    [void]$checks.Add('receipt-tamper-rejected')

    $receiptValue = Get-Content -Raw -LiteralPath $receiptPath | ConvertFrom-Json
    $receiptValue.guardRemovalAuthorized = 'false'
    $stringBooleanReceiptSignature = Write-TestJson $receiptPath $receiptValue
    $stringBooleanReceiptRun = $run | ConvertTo-Json -Depth 64 |
        ConvertFrom-Json
    $stringBooleanReceiptContext = $stringBooleanReceiptRun.runtime.contexts[0]
    $stringBooleanReceiptContext.receiptSignature.length =
        [long]$stringBooleanReceiptSignature.length
    $stringBooleanReceiptContext.receiptSignature.sha256 =
        [string]$stringBooleanReceiptSignature.sha256
    $stringBooleanReceiptRow = @(
        $stringBooleanReceiptRun.files | Where-Object {
            [string]$_.path -ceq
                [string]$stringBooleanReceiptContext.receiptPath
        })
    Assert-TestCondition ($stringBooleanReceiptRow.Count -eq 1) `
        'string-boolean receipt census target is exact'
    $stringBooleanReceiptRow[0].length =
        [long]$stringBooleanReceiptSignature.length
    $stringBooleanReceiptRow[0].sha256 =
        [string]$stringBooleanReceiptSignature.sha256
    $null = Write-TestJson $runPath $stringBooleanReceiptRun
    Assert-TestRejected 'nested string false receipt boolean' {
        & $producerPath -RunEnvelopePath $runPath `
            -SyntheticValidationOnly | Out-Null
    } 'exact JSON boolean'
    [IO.File]::WriteAllBytes($receiptPath, $receiptBytes)
    [IO.File]::WriteAllBytes($runPath, $runBytes)
    [void]$checks.Add('nested-string-false-boolean-rejected')

    $runText = (New-Object Text.UTF8Encoding($false, $true)).GetString($runBytes)
    $duplicate = $runText -replace '"schemaVersion"\s*:\s*1,',
        '"schemaVersion":1,"schemaVersion":1,'
    [IO.File]::WriteAllText(
        $runPath, $duplicate, (New-Object Text.UTF8Encoding($false)))
    Assert-TestRejected 'duplicate JSON property' {
        & $producerPath -RunEnvelopePath $runPath `
            -SyntheticValidationOnly | Out-Null
    } 'duplicates JSON object property'
    [IO.File]::WriteAllBytes($runPath, $runBytes)
    [void]$checks.Add('duplicate-json-rejected')

    $phaseReuse = $run | ConvertTo-Json -Depth 64 | ConvertFrom-Json
    $phaseReuse.runtime.contexts[0].receiptPath =
        [string]$phaseReuse.lineage.contexts[0].receiptPath
    $null = Write-TestJson $runPath $phaseReuse
    Assert-TestRejected 'diagnostic/standard receipt reuse' {
        & $producerPath -RunEnvelopePath $runPath `
            -SyntheticValidationOnly | Out-Null
    } 'reuses diagnostic lineage evidence'
    [IO.File]::WriteAllBytes($runPath, $runBytes)
    [void]$checks.Add('phase-boundary-reuse-rejected')

    $sidecarReuse = $run | ConvertTo-Json -Depth 64 | ConvertFrom-Json
    $sidecarReuse.runtime.contexts[0].journalPath =
        [string]$sidecarReuse.runtime.contexts[1].journalPath
    $sidecarReuse.runtime.contexts[0].journalSignature =
        $sidecarReuse.runtime.contexts[1].journalSignature
    $null = Write-TestJson $runPath $sidecarReuse
    Assert-TestRejected 'receipt sidecar reuse' {
        & $producerPath -RunEnvelopePath $runPath `
            -SyntheticValidationOnly | Out-Null
    } 'receipt sidecars are not exactly cross-bound'
    [IO.File]::WriteAllBytes($runPath, $runBytes)
    [void]$checks.Add('receipt-sidecar-reuse-rejected')

    $relabelledFile = $run | ConvertTo-Json -Depth 64 | ConvertFrom-Json
    $relabelTarget = @($relabelledFile.files | Where-Object {
            [string]$_.path -ceq
                'raw/standard-runtime/00-autosave_checkpoint/' +
                'server-logs/console.log'
        })
    Assert-TestCondition ($relabelTarget.Count -eq 1) `
        'synthetic relabel target is exact'
    $relabelTarget[0].role = 'client_console_log'
    $null = Write-TestJson $runPath $relabelledFile
    Assert-TestRejected 'run census role relabel' {
        & $producerPath -RunEnvelopePath $runPath `
            -SyntheticValidationOnly | Out-Null
    } 'role or stage is not derived'
    [IO.File]::WriteAllBytes($runPath, $runBytes)
    [void]$checks.Add('census-role-relabel-rejected')

    $terminalReadyPath = Join-Path $runRoot 'run.ready.json'
    $runSignature = Get-TestSignature $runPath
    $canonicalIndex = New-TestCanonicalRetentionIndex $run $runSignature
    $indexSignature = Write-TestJson $releaseIndexPath $canonicalIndex
    $terminalReadySignature = Write-TestRetentionReadySeal `
        -Path $terminalReadyPath `
        -Run $run `
        -RunSignature $runSignature `
        -IndexSignature $indexSignature
    $treeStateBefore = Get-TestTreeState $runRoot
    $verified = & $producerPath `
        -RunEnvelopePath $runPath `
        -SyntheticValidationOnly `
        -VerifyPublishedIndex
    $treeStateAfter = Get-TestTreeState $runRoot
    Assert-ExactProperties $verified @(
        'ValidationKind', 'PublishedIndexValid', 'ReadySealValid',
        'ReadOnlyVerification', 'SyntheticFixture', 'RunId', 'CandidateId',
        'CandidateBindingSha256', 'FileCount', 'IndexSignature',
        'ReadySignature', 'Disposition') 'Published verification result'
    $verifiedIndexSignatureNames = @(
        $verified.IndexSignature.PSObject.Properties |
            ForEach-Object { [string]$_.Name })
    $verifiedReadySignatureNames = @(
        $verified.ReadySignature.PSObject.Properties |
            ForEach-Object { [string]$_.Name })
    Assert-TestCondition (
        $verified.IndexSignature -is [PSCustomObject] -and
        $verifiedIndexSignatureNames.Count -eq 2 -and
        $verifiedIndexSignatureNames -ccontains 'length' -and
        $verifiedIndexSignatureNames -ccontains 'sha256' -and
        $verified.IndexSignature.length -is [long] -and
        $verified.IndexSignature.sha256 -is [string] -and
        $verified.ReadySignature -is [PSCustomObject] -and
        $verifiedReadySignatureNames.Count -eq 2 -and
        $verifiedReadySignatureNames -ccontains 'length' -and
        $verifiedReadySignatureNames -ccontains 'sha256' -and
        $verified.ReadySignature.length -is [long] -and
        $verified.ReadySignature.sha256 -is [string]) `
        'published verification signatures have exact scalar contracts'
    Assert-TestCondition (
        [string]$verified.ValidationKind -ceq
            'partisan_gate1_runtime_retention_published_index_verification_v1' -and
        $verified.PublishedIndexValid -is [bool] -and
        [bool]$verified.PublishedIndexValid -and
        $verified.ReadySealValid -is [bool] -and
        [bool]$verified.ReadySealValid -and
        $verified.ReadOnlyVerification -is [bool] -and
        [bool]$verified.ReadOnlyVerification -and
        $verified.SyntheticFixture -is [bool] -and
        [bool]$verified.SyntheticFixture -and
        [string]$verified.CandidateBindingSha256 -ceq
            [string]$run.lineage.contexts[0].candidateBindingSha256 -and
        (Test-FileSignatureExact $verified.IndexSignature $indexSignature) -and
        (Test-FileSignatureExact $verified.ReadySignature `
            $terminalReadySignature) -and
        [string]$verified.Disposition -ceq
            'passed-noncertifying-retention' -and
        [string]$treeStateAfter -ceq [string]$treeStateBefore) `
        'published-index verification is exact and performs zero writes'
    [void]$checks.Add('published-index-read-only-verification')
    Assert-TestRejected 'synthetic production verification' {
        & $producerPath `
            -RunEnvelopePath $runPath `
            -VerifyPublishedIndex | Out-Null
    } 'validation-only and cannot be published'
    [void]$checks.Add('synthetic-production-verification-rejected')

    $indexBytes = [IO.File]::ReadAllBytes($releaseIndexPath)
    $indexTamper = $canonicalIndex | ConvertTo-Json -Depth 64 | ConvertFrom-Json
    $indexTamper.disposition = 'tampered-outer-index'
    $null = Write-TestJson $releaseIndexPath $indexTamper
    Assert-TestRejected 'published canonical index tamper' {
        & $producerPath `
            -RunEnvelopePath $runPath `
            -SyntheticValidationOnly `
            -VerifyPublishedIndex | Out-Null
    } 'exact recomputed canonical index'
    [IO.File]::WriteAllBytes($releaseIndexPath, $indexBytes)
    [void]$checks.Add('published-canonical-index-tamper-rejected')

    $receiptBytes = [IO.File]::ReadAllBytes($receiptPath)
    $receiptValue = Get-Content -Raw -LiteralPath $receiptPath | ConvertFrom-Json
    $receiptValue.nonce = 'f' * 32
    $tamperedReceiptSignature = Write-TestJson $receiptPath $receiptValue
    $resealedRun = $run | ConvertTo-Json -Depth 64 | ConvertFrom-Json
    $resealedContext = $resealedRun.runtime.contexts[0]
    $resealedContext.receiptSignature.length =
        [long]$tamperedReceiptSignature.length
    $resealedContext.receiptSignature.sha256 =
        [string]$tamperedReceiptSignature.sha256
    $resealedReceiptRow = @($resealedRun.files | Where-Object {
            [string]$_.path -ceq [string]$resealedContext.receiptPath
        })
    Assert-TestCondition ($resealedReceiptRow.Count -eq 1) `
        'resealed receipt census target is exact'
    $resealedReceiptRow[0].length = [long]$tamperedReceiptSignature.length
    $resealedReceiptRow[0].sha256 = [string]$tamperedReceiptSignature.sha256
    $resealedRunSignature = Write-TestJson $runPath $resealedRun
    $resealedIndex = New-TestCanonicalRetentionIndex `
        $resealedRun $resealedRunSignature
    $resealedIndexSignature = Write-TestJson $releaseIndexPath $resealedIndex
    $null = Write-TestRetentionReadySeal `
        -Path $terminalReadyPath `
        -Run $resealedRun `
        -RunSignature $resealedRunSignature `
        -IndexSignature $resealedIndexSignature
    Assert-TestRejected 'resealed raw receipt semantic tamper' {
        & $producerPath `
            -RunEnvelopePath $runPath `
            -SyntheticValidationOnly `
            -VerifyPublishedIndex | Out-Null
    } 'receipt|completion|context|signature'
    [IO.File]::WriteAllBytes($receiptPath, $receiptBytes)
    [IO.File]::WriteAllBytes($runPath, $runBytes)
    $indexSignature = Write-TestJson $releaseIndexPath $canonicalIndex
    $null = Write-TestRetentionReadySeal `
        -Path $terminalReadyPath `
        -Run $run `
        -RunSignature $runSignature `
        -IndexSignature $indexSignature
    $verifiedAgain = & $producerPath `
        -RunEnvelopePath $runPath `
        -SyntheticValidationOnly `
        -VerifyPublishedIndex
    Assert-TestCondition (
        [string]$verifiedAgain.ValidationKind -ceq
            'partisan_gate1_runtime_retention_published_index_verification_v1' -and
        [bool]$verifiedAgain.SyntheticFixture -and
        [string]$verifiedAgain.Disposition -ceq
            'passed-noncertifying-retention') `
        'resealed tamper restoration verifies exactly'
    [void]$checks.Add('resealed-raw-semantic-tamper-rejected')

    Remove-Item -LiteralPath $releaseIndexPath, $terminalReadyPath -Force

    $syntheticStateBefore = Get-TestTreeState $runRoot
    $acceptedAgain = & $producerPath `
        -RunEnvelopePath $runPath `
        -SyntheticValidationOnly
    $syntheticStateAfter = Get-TestTreeState $runRoot
    Assert-TestCondition (
        [string]$acceptedAgain.Disposition -ceq
            'validated-synthetic-fixture-only' -and
        -not (Test-Path -LiteralPath $releaseIndexPath) -and
        -not (Test-Path -LiteralPath $terminalReadyPath) -and
        [string]$syntheticStateAfter -ceq [string]$syntheticStateBefore) `
        'identical synthetic fixture revalidation is publication-free'
    [void]$checks.Add('synthetic-index-never-published')
}
catch { $testFailure = $_ }
finally {
    try {
        $rootPath = [IO.Path]::GetFullPath($tempRoot)
        $prefix = $tempBase.TrimEnd('\', '/') +
            [IO.Path]::DirectorySeparatorChar
        if (-not $rootPath.StartsWith(
                $prefix, [StringComparison]::OrdinalIgnoreCase) -or
            (Split-Path -Leaf $rootPath) -cnotmatch
                '^PartisanGate1IndexSelfTest_[0-9a-f]{32}$') {
            throw 'Self-test cleanup target was not exact.'
        }
        if (Test-Path -LiteralPath $rootPath) {
            Remove-Item -LiteralPath $rootPath -Recurse -Force -ErrorAction Stop
        }
    }
    catch { $cleanupFailure = $_ }
}
if ($cleanupFailure) { throw $cleanupFailure }
if ($testFailure) { throw $testFailure }
Write-Output ([pscustomobject][ordered]@{
    Success = $true
    CheckCount = $checks.Count
    Checks = [string[]]$checks.ToArray()
    EngineProcessesStarted = 0
    SyntheticGuardedContexts = 10
    DiagnosticServerLaunches = 5
    DiagnosticClientLaunches = 1
    StandardServerLaunches = 5
    StandardClientLaunches = 1
})
