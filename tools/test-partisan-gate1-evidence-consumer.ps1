[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Import-Module (Join-Path $PSScriptRoot 'Partisan.Gate1EvidenceConsumer.psm1') `
    -Force -ErrorAction Stop

$script:Utf8NoBom = New-Object Text.UTF8Encoding($false)
$script:Passed = 0
$script:Rejected = 0
$script:SurfaceToolPaths = [ordered]@{
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
$script:RetentionToolPaths = [ordered]@{
    'gate1-runner' = 'tools/run-guarded-gate1-runtime-retention.ps1'
    'release-candidate-module' = 'tools/Partisan.ReleaseCandidate.psm1'
    'guarded-runtime-module' = 'tools/Partisan.GuardedRuntime.psm1'
    'ordinary-persistence-library' =
        'tools/run-ordinary-campaign-persistence-proof.ps1'
    'gate1-index-producer' = 'tools/New-PartisanGate1RuntimeRetentionIndex.ps1'
    'gate1-evidence-consumer' = 'tools/Partisan.Gate1EvidenceConsumer.psm1'
    'release-docs-consumer' = 'tools/update-release-docs.ps1'
}
$script:Stages = @(
    'autosave_checkpoint', 'manual_checkpoint', 'shutdown_checkpoint',
    'native_shutdown_verify', 'profile_fallback_verify')

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
    return Get-TestSha256Bytes $script:Utf8NoBom.GetBytes($Text)
}

function Write-TestText {
    param([string]$Path, [AllowEmptyString()][string]$Text)
    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    [IO.File]::WriteAllText($Path, $Text, $script:Utf8NoBom)
}

function Write-TestBytes {
    param([string]$Path, [byte[]]$Bytes)
    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    [IO.File]::WriteAllBytes($Path, $Bytes)
}

function Write-TestJson {
    param([string]$Path, $Value)
    $text = ($Value | ConvertTo-Json -Depth 100) + "`n"
    Write-TestText $Path $text
    return Get-TestSignature $Path
}

function Read-TestJson {
    param([string]$Path)
    return [IO.File]::ReadAllText($Path) | ConvertFrom-Json
}

function Get-TestSignature {
    param([string]$Path)
    $bytes = [IO.File]::ReadAllBytes($Path)
    return [pscustomobject][ordered]@{
        length = [long]$bytes.LongLength
        sha256 = Get-TestSha256Bytes $bytes
    }
}

function Get-TestRowsDigest {
    param([object[]]$Rows)
    $lines = @($Rows | Sort-Object path | ForEach-Object {
        "{0}`t{1}`t{2}" -f $_.sha256, [long]$_.length, $_.path
    })
    return Get-TestSha256Text (($lines -join "`n") + "`n")
}

function Get-TestPackageDigest {
    param([object[]]$Rows)
    $lines = @($Rows | Sort-Object indexPath | ForEach-Object {
        "{0}`t{1}`t{2}" -f $_.sha256, [long]$_.length, $_.indexPath
    })
    return Get-TestSha256Text (($lines -join "`n") + "`n")
}

function ConvertTo-TestPortablePath {
    param([string]$Root, [string]$Path)
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $full = [IO.Path]::GetFullPath($Path)
    return $full.Substring(
        $rootFull.Length + 1).Replace('\', '/')
}

function Get-TestPayloadRows {
    param([string]$RunRoot, [switch]$WithRole)
    $controls = @('evidence-index.json', 'run.json', 'release-index.json',
        'run.ready.json')
    return [object[]]@(Get-ChildItem -LiteralPath $RunRoot -Recurse -File -Force |
        ForEach-Object {
            $portable = ConvertTo-TestPortablePath $RunRoot $_.FullName
            if ($portable -in $controls) { return }
            $signature = Get-TestSignature $_.FullName
            if ($WithRole) {
                [pscustomobject][ordered]@{
                    role = 'synthetic_fixture'
                    stage = 'run'
                    path = $portable
                    length = [long]$signature.length
                    sha256 = [string]$signature.sha256
                }
            }
            else {
                [pscustomobject][ordered]@{
                    path = $portable
                    length = [long]$signature.length
                    sha256 = [string]$signature.sha256
                }
            }
        } | Where-Object { $null -ne $_ } | Sort-Object path)
}

function New-TestProvenance {
    param([string]$FileName, [string]$Seed)
    return [pscustomobject][ordered]@{
        fileName = $FileName
        fileVersion = '1.2.3.4'
        productVersion = '1.2.3.4'
        length = 100 + $Seed.Length
        sha256 = Get-TestSha256Text ($Seed + "`n")
    }
}

function New-TestToolRows {
    param([string]$RepositoryRoot, [Collections.IDictionary]$Paths)
    return [object[]]@($Paths.Keys | ForEach-Object {
        $signature = Get-TestSignature (Join-Path $RepositoryRoot `
            ([string]$Paths[$_]).Replace('/', '\'))
        [pscustomobject][ordered]@{
            role = [string]$_
            path = [string]$Paths[$_]
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        }
    })
}

function Get-TestPublisherVerifierStub {
    return @'
param(
    [Parameter(Mandatory = $true)]
    [string]$RunEnvelopePath,

    [switch]$VerifyPublishedIndex
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-StubSignature {
    param([string]$Path)

    $bytes = [IO.File]::ReadAllBytes($Path)
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $sha256 = ([BitConverter]::ToString(
            $algorithm.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally { $algorithm.Dispose() }
    return [pscustomobject][ordered]@{
        length = [long]$bytes.LongLength
        sha256 = $sha256
    }
}

if (-not $VerifyPublishedIndex) {
    throw 'The consumer fixture publisher only supports read-only verification.'
}
$runRoot = Split-Path -Parent ([IO.Path]::GetFullPath($RunEnvelopePath))
$run = Get-Content -LiteralPath $RunEnvelopePath -Raw | ConvertFrom-Json
$indexPath = Join-Path $runRoot 'release-index.json'
$readyPath = Join-Path $runRoot 'run.ready.json'
$index = Get-Content -LiteralPath $indexPath -Raw | ConvertFrom-Json
$publisherName = [IO.Path]::GetFileName($MyInvocation.MyCommand.Path)
$surface = $publisherName -ceq 'New-PartisanReleaseSurfaceAuditIndex.ps1'
$validationKind = if ($surface) {
    'partisan_release_surface_published_index_verification_v1'
}
else {
    'partisan_gate1_runtime_retention_published_index_verification_v1'
}
$disposition = if ($surface) {
    'passed-noncertifying-release-surface-audit'
}
else { 'passed-noncertifying-retention' }
$binding = if ($surface) {
    [string]$run.candidateBindingSha256
}
else { [string]$index.candidateBindingSha256 }
$candidateId = if ($surface) {
    [string]$index.candidate.candidateId
}
else { [string]$index.candidateId }
$result = [pscustomobject][ordered]@{
    ValidationKind = $validationKind
    PublishedIndexValid = $true
    ReadySealValid = $true
    ReadOnlyVerification = $true
    SyntheticFixture = $false
    RunId = [string]$run.runId
    CandidateId = $candidateId
    CandidateBindingSha256 = $binding
    FileCount = [int](@($index.files).Count)
    IndexSignature = Get-StubSignature $indexPath
    ReadySignature = Get-StubSignature $readyPath
    Disposition = $disposition
}
$fault = [Environment]::GetEnvironmentVariable(
    'PARTISAN_GATE1_CONSUMER_STUB_FAULT', 'Process')
if ($fault -ceq 'surface-bool' -and $surface) {
    $result.ReadOnlyVerification = 'true'
}
elseif ($fault -ceq 'retention-signature' -and -not $surface) {
    $result.ReadySignature.sha256 = ('0' * 64)
}
elseif ($fault -ceq 'surface-extra-output' -and $surface) {
    Write-Output 'unexpected publisher output'
}
elseif ($fault -ceq 'surface-string-builder' -and $surface) {
    $result.ValidationKind = New-Object Text.StringBuilder `
        ([string]$result.ValidationKind)
}
elseif ($fault -ceq 'surface-same-byte-rewrite' -and $surface) {
    [IO.File]::WriteAllBytes(
        $RunEnvelopePath,
        [IO.File]::ReadAllBytes($RunEnvelopePath))
}
elseif ($fault -ceq 'surface-transient-file' -and $surface) {
    $transientPath = Join-Path $runRoot 'publisher-transient.tmp'
    [IO.File]::WriteAllText($transientPath, 'transient')
    [IO.File]::Delete($transientPath)
}
Write-Output $result
'@
}

function Initialize-TestRepository {
    param([string]$RepositoryRoot)
    New-Item -ItemType Directory -Path $RepositoryRoot | Out-Null
    & git -C $RepositoryRoot init -q
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic Git repository init failed.' }
    & git -C $RepositoryRoot config core.autocrlf false
    & git -C $RepositoryRoot config user.name 'Partisan Consumer Selftest'
    & git -C $RepositoryRoot config user.email 'consumer-selftest.invalid'
    Write-TestText (Join-Path $RepositoryRoot 'candidate-boundary.txt') `
        "candidate`n"
    & git -C $RepositoryRoot add -- candidate-boundary.txt
    & git -C $RepositoryRoot commit -q -m 'candidate'
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic candidate commit failed.' }
    $candidateHead = (& git -C $RepositoryRoot rev-parse HEAD).Trim()
    $allPaths = @($script:SurfaceToolPaths.Values) +
        @($script:RetentionToolPaths.Values) | Sort-Object -Unique
    foreach ($path in $allPaths) {
        $content = "fixture " + $path + "`n"
        if ([string]$path -cmatch '\.json$') { $content = "{}`n" }
        Write-TestText (Join-Path $RepositoryRoot `
            ([string]$path).Replace('/', '\')) $content
    }
    $publisherStub = Get-TestPublisherVerifierStub
    foreach ($publisherPath in @(
            $script:SurfaceToolPaths.releaseIndexProducer,
            $script:RetentionToolPaths['gate1-index-producer'])) {
        Write-TestText (Join-Path $RepositoryRoot `
            ([string]$publisherPath).Replace('/', '\')) $publisherStub
    }
    & git -C $RepositoryRoot add -- .
    & git -C $RepositoryRoot commit -q -m 'harness tools'
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic harness commit failed.' }
    $harnessHead = (& git -C $RepositoryRoot rev-parse HEAD).Trim()
    return [pscustomobject][ordered]@{
        CandidateHead = $candidateHead
        HarnessHead = $harnessHead
    }
}

function New-TestCandidate {
    param([string]$CandidateHead)
    $packageContents = [ordered]@{
        'Partisan/addon.gproj' = "project`n"
        'Partisan/data.pak' = "package`n"
        'Partisan/resourceDatabase.rdb' = "database`n"
        'Partisan/thumbnail.png' = "thumbnail`n"
    }
    $packageRows = [object[]]@($packageContents.Keys | ForEach-Object {
        $bytes = $script:Utf8NoBom.GetBytes([string]$packageContents[$_])
        [pscustomobject][ordered]@{
            path = 'package/' + [string]$_
            indexPath = [string]$_
            length = [long]$bytes.LongLength
            sha256 = Get-TestSha256Bytes $bytes
        }
    })
    $packageSha = Get-TestPackageDigest $packageRows
    $server = New-TestProvenance 'ArmaReforgerServer.exe' 'server'
    $client = New-TestProvenance 'ArmaReforgerSteam.exe' 'client'
    $serverDiagnostic = New-TestProvenance `
        'ArmaReforgerServerDiag.exe' 'server-diagnostic'
    $clientDiagnostic = New-TestProvenance `
        'ArmaReforgerSteamDiag.exe' 'client-diagnostic'
    foreach ($value in @($client, $serverDiagnostic, $clientDiagnostic)) {
        $value.fileVersion = $server.fileVersion
        $value.productVersion = $server.productVersion
    }
    $manifest = [pscustomobject][ordered]@{
        manifestSchemaVersion = 1
        createdUtc = '2026-07-20T12:00:00Z'
        candidate = [ordered]@{
            id = 'partisan-rc-consumer-selftest'
            version = 'consumer-selftest'
            state = 'retained-uncertified'
        }
        source = [ordered]@{
            gitHead = $CandidateHead
            embeddedImplementation = [ordered]@{
                sha = $CandidateHead
                utc = '2026-07-20T11:59:00Z'
                label = 'consumer-selftest'
            }
            campaignSchema = 71
            runtimeSettingsSchema = 24
        }
        addon = [ordered]@{ id = 'Partisan'; guid = '698532771130111D' }
        workbench = [ordered]@{ crc = '1234abcd' }
        package = [ordered]@{
            hashAlgorithm = 'sha256-manifest-v1'
            sha256 = $packageSha
            files = $packageRows
        }
        toolchain = [ordered]@{
            server = $server
            client = $client
            serverDiagnostic = $serverDiagnostic
            clientDiagnostic = $clientDiagnostic
        }
    }
    $manifestText = ($manifest | ConvertTo-Json -Depth 100) + "`n"
    $manifestSha = Get-TestSha256Text $manifestText
    $ready = [pscustomobject][ordered]@{
        schemaVersion = 1
        candidateId = $manifest.candidate.id
        gitHead = $CandidateHead
        packageSha256 = $packageSha
        manifestSha256 = $manifestSha
    }
    $readyText = ($ready | ConvertTo-Json -Depth 100) + "`n"
    return [pscustomobject][ordered]@{
        Manifest = $manifest
        ManifestText = $manifestText
        ManifestSha256 = $manifestSha
        Ready = $ready
        ReadyText = $readyText
        ReadySha256 = Get-TestSha256Text $readyText
        PackageContents = $packageContents
        PackageRows = $packageRows
        Server = $server
        Client = $client
        ServerDiagnostic = $serverDiagnostic
        ClientDiagnostic = $clientDiagnostic
    }
}

function Get-TestCandidateIdentity {
    param($Candidate)
    return [pscustomobject][ordered]@{
        CandidateId = [string]$Candidate.Manifest.candidate.id
        CandidateSourceHead = [string]$Candidate.Manifest.source.gitHead
        ManifestPath = 'docs/evidence/candidates/synthetic/candidate.json'
        ManifestSha256 = [string]$Candidate.ManifestSha256
        ReadySha256 = [string]$Candidate.ReadySha256
        PackageHashAlgorithm = 'sha256-manifest-v1'
        PackageSha256 = [string]$Candidate.Manifest.package.sha256
        PackageVersion = [string]$Candidate.Manifest.candidate.version
        WorkbenchCrc = [string]$Candidate.Manifest.workbench.crc
        CreatedUtc = [DateTimeOffset]'2026-07-20T12:00:00Z'
        CampaignSchema = 71
        RuntimeSettingsSchema = 24
        EmbeddedSha = [string]$Candidate.Manifest.source.embeddedImplementation.sha
        EmbeddedUtc = [string]$Candidate.Manifest.source.embeddedImplementation.utc
        EmbeddedLabel =
            [string]$Candidate.Manifest.source.embeddedImplementation.label
        Manifest = $Candidate.Manifest
    }
}

function New-TestSurfaceCandidate {
    param($Candidate)
    return [pscustomobject][ordered]@{
        candidateId = [string]$Candidate.Manifest.candidate.id
        candidateVersion = [string]$Candidate.Manifest.candidate.version
        runtimeUseDisposition = 'active-runtime-candidate'
        gitHead = [string]$Candidate.Manifest.source.gitHead
        embeddedBuildSha =
            [string]$Candidate.Manifest.source.embeddedImplementation.sha
        embeddedBuildUtc =
            [string]$Candidate.Manifest.source.embeddedImplementation.utc
        embeddedBuildLabel =
            [string]$Candidate.Manifest.source.embeddedImplementation.label
        campaignSchema = 71
        runtimeSettingsSchema = 24
        addonId = 'Partisan'
        addonGuid = '698532771130111D'
        packageHashAlgorithm = 'sha256-manifest-v1'
        packageSha256 = [string]$Candidate.Manifest.package.sha256
        manifestSha256 = [string]$Candidate.ManifestSha256
        readySha256 = [string]$Candidate.ReadySha256
        workbenchCrc = '1234abcd'
        runtimeRole = 'server'
        diagnosticExecutable = $Candidate.ServerDiagnostic
        recordedDiagnosticExecutable = $Candidate.ServerDiagnostic
        recordedRuntimeExecutable = $Candidate.Server
    }
}

function New-TestSurfaceBundle {
    param(
        [string]$RepositoryRoot,
        [string]$EvidenceRoot,
        $Candidate,
        [string]$HarnessHead,
        [string]$PackagePathPrefix = 'package/')

    $candidateValue = New-TestSurfaceCandidate $Candidate
    $candidateBinding = ('b' * 64)
    $nonce = '11111111111111111111111111111111'
    $runLeaf = '20260720T120100Z-' + $nonce
    $runId = 'release_surface_20260720T120100Z_' + $nonce.Substring(0, 20)
    $started = '2026-07-20T12:01:00Z'
    $completed = '2026-07-20T12:02:00Z'
    $runRoot = Join-Path (Join-Path (Join-Path $EvidenceRoot `
        ([string]$candidateValue.candidateId)) 'release-surface-audit') $runLeaf
    New-Item -ItemType Directory -Path $runRoot -Force | Out-Null

    Write-TestText (Join-Path $runRoot 'identity\candidate.json') `
        $Candidate.ManifestText
    Write-TestText (Join-Path $runRoot 'identity\candidate.ready.json') `
        $Candidate.ReadyText
    Copy-Item -LiteralPath (Join-Path $RepositoryRoot `
        'docs\data\release_surface_contract.json') `
        -Destination (Join-Path $runRoot `
            'identity\release_surface_contract.json')

    $owner = [ordered]@{
        schemaVersion = 1
        evidenceKind = 'partisan_release_surface_runtime_audit_v2'
        runId = $runId
        runNonce = $nonce
        candidateId = [string]$candidateValue.candidateId
        packageSha256 = [string]$candidateValue.packageSha256
        harnessGuid = '1234567890ABCDEF'
        harnessSha256 = ('c' * 64)
        disposition = 'in-progress-noncertifying-release-surface-audit'
    }
    $null = Write-TestJson (Join-Path $runRoot 'run.owner.json') $owner
    $cleanup = [ordered]@{
        schemaVersion = 1
        evidenceKind = 'partisan_release_surface_runtime_audit_v2'
        harnessRemoved = $true
        harnessResidueCount = 0
        exactOwnerVerifiedBeforeRemoval = $true
        passed = $true
    }
    $null = Write-TestJson (Join-Path $runRoot 'cleanup.json') $cleanup

    $harnessOwner = [ordered]@{
        schemaVersion = 1
        evidenceKind = 'partisan_release_surface_harness_owner_v1'
        runNonce = $nonce
        harnessId = 'PartisanReleaseSurfaceAudit'
        harnessGuid = '1234567890ABCDEF'
        candidateGuid = '698532771130111D'
    }
    $null = Write-TestJson (Join-Path $runRoot `
        'harness\source\.partisan-release-surface-audit-owner.json') `
        $harnessOwner
    Write-TestText (Join-Path $runRoot `
        'harness\source\Scripts\Game\PartisanReleaseSurfaceAudit.c') `
        "class PartisanReleaseSurfaceAuditSynthetic {}`n"
    Write-TestText (Join-Path $runRoot 'harness\source\addon.gproj') `
        "synthetic harness`n"
    $harnessFiles = [object[]]@(
        '.partisan-release-surface-audit-owner.json',
        'Scripts/Game/PartisanReleaseSurfaceAudit.c',
        'addon.gproj' | ForEach-Object {
            $path = Join-Path (Join-Path $runRoot 'harness\source') `
                ([string]$_).Replace('/', '\')
            $signature = Get-TestSignature $path
            [pscustomobject][ordered]@{
                path = [string]$_
                length = [long]$signature.length
                sha256 = [string]$signature.sha256
            }
        })
    $harnessAggregate = Get-TestRowsDigest $harnessFiles
    $tools = New-TestToolRows $RepositoryRoot $script:SurfaceToolPaths
    $surfacePackageRows = [object[]]@($Candidate.PackageRows | ForEach-Object {
        [pscustomobject][ordered]@{
            path = $PackagePathPrefix + [string]$_.indexPath
            indexPath = [string]$_.indexPath
            length = [long]$_.length
            sha256 = [string]$_.sha256
        }
    })

    $bindings = [ordered]@{
        schemaVersion = 1
        evidenceKind = 'partisan_release_surface_runtime_audit_v2'
        source = [ordered]@{
            harnessGitHead = $HarnessHead
            checkoutClean = $true
            candidateAncestor = $true
            executionMode = 'paired-native-engine-audit'
        }
        candidate = $candidateValue
        package = [ordered]@{
            hashAlgorithm = 'sha256-manifest-v1'
            sha256 = [string]$candidateValue.packageSha256
            files = $surfacePackageRows
        }
        executables = [ordered]@{
            retail = $Candidate.Server
            diagnostic = $Candidate.ServerDiagnostic
        }
        harness = [ordered]@{
            id = 'PartisanReleaseSurfaceAudit'
            guid = '1234567890ABCDEF'
            candidateDependencyGuid = '698532771130111D'
            constructionMode =
                'external-disposable-runtime-loaded-audit-addon'
            aggregateSha256 = $harnessAggregate
            files = $harnessFiles
        }
        tools = $tools
        host = [ordered]@{
            powershellVersion = '5.1'
            powershellEdition = 'Desktop'
            operatingSystem = 'synthetic'
        }
    }
    $null = Write-TestJson (Join-Path $runRoot 'identity\bindings.json') `
        $bindings

    $modeBindings = New-Object Collections.Generic.List[object]
    $modeIndexRows = New-Object Collections.Generic.List[object]
    for ($ordinal = 0; $ordinal -lt 2; $ordinal++) {
        $mode = @('retail', 'diagnostic')[$ordinal]
        $executable = if ($mode -ceq 'retail') {
            $Candidate.Server
        }
        else { $Candidate.ServerDiagnostic }
        $contextId = if ($mode -ceq 'retail') {
            '22222222222222222222222222222222'
        }
        else { '33333333333333333333333333333333' }
        $stockClusterPresent = $mode -ceq 'retail'
        $modeValue = [ordered]@{
            schemaVersion = 2
            evidenceKind = 'partisan_release_surface_runtime_audit_v2'
            mode = $mode
            disposition = 'passed-noncertifying-release-surface-audit'
            candidateId = [string]$candidateValue.candidateId
            packageSha256 = [string]$candidateValue.packageSha256
            manifestSha256 = [string]$candidateValue.manifestSha256
            readySha256 = [string]$candidateValue.readySha256
            executable = $executable
            harnessGuid = '1234567890ABCDEF'
            harnessSha256 = $harnessAggregate
            process = [ordered]@{
                exitCode = 0
                contextId = $contextId
                candidateBindingSha256 = $candidateBinding
                receiptPath = 'raw/' + $mode + '/guarded-runtime/receipt.json'
                receiptSignature = [ordered]@{
                    length = 1
                    sha256 = ('d' * 64)
                }
                journalPath = 'raw/' + $mode + '/guarded-runtime/journal.json'
                journalSignature = [ordered]@{
                    length = 1
                    sha256 = ('e' * 64)
                }
                completionPath =
                    'raw/' + $mode + '/guarded-runtime/completion.json'
                completionSignature = [ordered]@{
                    length = 1
                    sha256 = ('f' * 64)
                }
            }
            arguments = [ordered]@{
                raw = 'raw/' + $mode + '/arguments.raw.json'
                portable = 'raw/' + $mode + '/arguments.portable.json'
            }
            streams = [ordered]@{ authoritative = $true }
            probe = [ordered]@{
                path = 'raw/' + $mode + '/profile/probe.result.json'
                signature = [ordered]@{
                    length = 1
                    sha256 = ('1' * 64)
                }
                summary = [ordered]@{ mode = $mode; passed = $true }
            }
            classification = [ordered]@{
                valid = $true
                hardDiagnosticPolicy = 'script-engine-and-process-fatal-v1'
                hardDiagnosticFree = -not $stockClusterPresent
                hardDiagnosticRawLineCount =
                    if ($stockClusterPresent) { 6 } else { 0 }
                hardDiagnosticEventCount =
                    if ($stockClusterPresent) { 2 } else { 0 }
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
                candidateMountAttestation = [ordered]@{
                    valid = $true
                    recordCount = 2
                    exactPathCount = 2
                    packedCount = 1
                    invalidModeCount = 0
                    guidExact = $true
                    packed = $true
                }
                harnessMountLineCount = 1
                uniqueResultMarkerCount = 1
                resultMarkerOccurrenceCount = 2
                crashLogContentValid = $true
                crashArtifactCount = 0
                logs = [object[]]$(
                    if ($mode -ceq 'retail') {
                        @('console.log', 'script.log', 'error.log')
                    }
                    else {
                        @('console.log', 'script.log', 'error.log', 'crash.log')
                    })
            }
            passed = $true
        }
        $modePath = Join-Path $runRoot ('modes\' + $mode + '.json')
        $signature = Write-TestJson $modePath $modeValue
        $portableModePath = 'modes/' + $mode + '.json'
        [void]$modeBindings.Add([pscustomobject][ordered]@{
            mode = $mode
            path = $portableModePath
            signature = $signature
        })
        [void]$modeIndexRows.Add([pscustomobject][ordered]@{
            mode = $mode
            path = $portableModePath
            signature = $signature
            executable = $executable
            contextId = $contextId
            candidateBindingSha256 = $candidateBinding
            forbiddenTypeCount = 3
            productionTypeCount = 2
            forbiddenMemberCount = 67
            productionMemberCount = 91
            forbiddenCommandCount = 3
            productionCommandCount = 2
            hardDiagnosticPolicy = 'script-engine-and-process-fatal-v1'
            hardDiagnosticFree = -not $stockClusterPresent
            hardDiagnosticRawLineCount =
                if ($stockClusterPresent) { 6 } else { 0 }
            hardDiagnosticEventCount =
                if ($stockClusterPresent) { 2 } else { 0 }
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
            candidateMountAttestation = [ordered]@{
                valid = $true
                recordCount = 2
                exactPathCount = 2
                packedCount = 1
                invalidModeCount = 0
                guidExact = $true
                packed = $true
            }
            harnessMountLineCount = 1
            uniqueResultMarkerCount = 1
            resultMarkerOccurrenceCount = 2
            crashLogContentValid = $true
            crashArtifactCount = 0
            passed = $true
        })
    }
    Write-TestText (Join-Path $runRoot 'raw\retail\console.log') `
        "retail result`n"
    Write-TestText (Join-Path $runRoot 'raw\diagnostic\console.log') `
        "diagnostic result`n"

    $payloadRows = Get-TestPayloadRows $runRoot
    $aggregate = Get-TestRowsDigest $payloadRows
    $evidenceIndex = [ordered]@{
        schemaVersion = 1
        evidenceKind = 'partisan_release_surface_runtime_audit_v2'
        files = $payloadRows
        aggregateSha256 = $aggregate
    }
    $evidenceIndexSignature = Write-TestJson `
        (Join-Path $runRoot 'evidence-index.json') $evidenceIndex
    $limitations = @(
        'This audit uses inert compiler and metadata probes for contracted member-surface presence; separately, it deliberately invokes production menu generation and read-only per-command availability inspection, but it does not execute command actions or mutate campaign gameplay state.',
        'Forbidden literal surfaces are proven by the candidate-bound source guard analysis, not by an unreliable package-byte string scan.',
        'It is not gameplay, multiplayer, persistence, restart, soak, or performance certification.',
        'The guarded launcher deliberately gives the child no inherited standard streams; authoritative engine output is retained in the three required logs and in crash.log when the engine emits it.',
        'The machine-bound hard-diagnostic policy is script-engine-and-process-fatal-v1: SCRIPT or ENGINE error severity, access violations, unhandled exceptions, fatal/application-crash signals, and audit ERROR markers. Other retained engine-channel severities are outside this narrow release-surface predicate. A successful crash.log must be absent or empty.',
        'A mode may contain either no hard diagnostic or one exact stock Eden shutdown cluster: two underlying support-station catalog-manager events mirrored once across console.log, script.log, and error.log after replication finishes and before game destruction. Every partial, extra, variant, misplaced, crash-channel, or unrelated hard event fails closed.')
    $run = [ordered]@{
        schemaVersion = 2
        evidenceKind = 'partisan_release_surface_runtime_audit_v2'
        contractId = 'partisan.release-surface-audit.run.v2'
        runId = $runId
        runLeafId = $runLeaf
        runNonce = $nonce
        startedUtc = $started
        completedUtc = $completed
        disposition = 'passed-noncertifying-release-surface-audit'
        certificationPromotion = 'none'
        source = [ordered]@{
            harnessGitHead = $HarnessHead
            checkoutClean = $true
            candidateAncestor = $true
            executionMode = 'paired-native-engine-audit'
        }
        candidate = $candidateValue
        candidateBindingSha256 = $candidateBinding
        pairedSamePackage = $true
        candidatePackageSha256 = [string]$candidateValue.packageSha256
        harnessGuid = '1234567890ABCDEF'
        harnessSha256 = $harnessAggregate
        modes = $modeBindings.ToArray()
        cleanup = $cleanup
        evidenceIndex = [ordered]@{
            path = 'evidence-index.json'
            signature = $evidenceIndexSignature
            aggregateSha256 = $aggregate
            fileCount = $payloadRows.Count
        }
        limitations = $limitations
        passed = $true
    }
    $runSignature = Write-TestJson (Join-Path $runRoot 'run.json') $run
    $contractSignature = Get-TestSignature (Join-Path $runRoot `
        'identity\release_surface_contract.json')
    $index = [ordered]@{
        schemaVersion = 2
        contractId = 'partisan.release-surface-audit.index.v2'
        evidenceKind = 'partisan_release_surface_runtime_audit_index_v2'
        runId = $runId
        runLeafId = $runLeaf
        runNonce = $nonce
        source = [ordered]@{
            bundleRelativePath = [string]$candidateValue.candidateId +
                '/release-surface-audit/' + $runLeaf
            candidateGitHead = [string]$candidateValue.gitHead
            harnessGitHead = $HarnessHead
            checkoutClean = $true
            candidateAncestor = $true
            executionMode = 'paired-native-engine-audit'
        }
        candidate = $candidateValue
        candidateBindingSha256 = $candidateBinding
        run = [ordered]@{
            path = 'run.json'
            length = [long]$runSignature.length
            sha256 = [string]$runSignature.sha256
        }
        evidenceIndex = [ordered]@{
            path = 'evidence-index.json'
            length = [long]$evidenceIndexSignature.length
            sha256 = [string]$evidenceIndexSignature.sha256
            fileCount = $payloadRows.Count
            filesAggregateSha256 = $aggregate
        }
        contract = [ordered]@{
            path = 'identity/release_surface_contract.json'
            length = [long]$contractSignature.length
            sha256 = [string]$contractSignature.sha256
            forbiddenTypeCount = 3
            forbiddenCommandCount = 3
            productionTypeCount = 2
            productionCommandCount = 2
            forbiddenMemberSurfaceCount = 67
            forbiddenLiteralSurfaceCount = 2
            productionObservabilityMemberSurfaceCount = 91
        }
        harness = [ordered]@{
            id = 'PartisanReleaseSurfaceAudit'
            guid = '1234567890ABCDEF'
            aggregateSha256 = $harnessAggregate
            files = $harnessFiles
            tools = $tools
        }
        capture = [ordered]@{
            startedUtc = $started
            completedUtc = $completed
            modeCount = 2
            pairedOrder = @('retail', 'diagnostic')
        }
        modes = $modeIndexRows.ToArray()
        cleanup = $cleanup
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
        files = $payloadRows
        filesAggregateSha256 = $aggregate
        limitations = $limitations
        disposition = 'passed-noncertifying-release-surface-audit'
        certificationPromotion = 'none'
        passed = $true
    }
    $externalIndexSignature = Write-TestJson `
        (Join-Path $runRoot 'release-index.json') $index
    $summaryPath = 'docs/evidence/release-surface-audit/synthetic-surface.json'
    $trackedPath = Join-Path $RepositoryRoot $summaryPath.Replace('/', '\')
    $trackedParent = Split-Path -Parent $trackedPath
    New-Item -ItemType Directory -Path $trackedParent -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $runRoot 'release-index.json') `
        -Destination $trackedPath -Force
    $ready = [ordered]@{
        schemaVersion = 2
        evidenceKind = 'partisan_release_surface_runtime_audit_v2'
        disposition = 'passed-noncertifying-release-surface-audit'
        certificationPromotion = 'none'
        runId = $runId
        runLeafId = $runLeaf
        source = [ordered]@{
            candidateGitHead = [string]$candidateValue.gitHead
            harnessGitHead = $HarnessHead
        }
        candidateId = [string]$candidateValue.candidateId
        packageSha256 = [string]$candidateValue.packageSha256
        manifestSha256 = [string]$candidateValue.manifestSha256
        readySha256 = [string]$candidateValue.readySha256
        candidate = $candidateValue
        candidateBindingSha256 = $candidateBinding
        run = [ordered]@{
            path = 'run.json'
            signature = $runSignature
        }
        evidenceIndex = [ordered]@{
            path = 'evidence-index.json'
            signature = $evidenceIndexSignature
        }
        releaseIndex = [ordered]@{
            path = 'release-index.json'
            signature = $externalIndexSignature
        }
        cleanupPassed = $true
        sealedLast = $true
    }
    $readySignature = Write-TestJson (Join-Path $runRoot 'run.ready.json') $ready
    return [pscustomobject][ordered]@{
        Record = [pscustomobject][ordered]@{
            status = 'passed-noncertifying'
            summaryPath = $summaryPath
            summarySha256 = [string]$externalIndexSignature.sha256
            runReadySha256 = [string]$readySignature.sha256
            candidateId = [string]$candidateValue.candidateId
            candidateSourceHead = [string]$candidateValue.gitHead
            packageSha256 = [string]$candidateValue.packageSha256
            candidateBindingSha256 = $candidateBinding
            harnessGitHead = $HarnessHead
            runId = $runId
            runLeafId = $runLeaf
            startedUtc = $started
            completedUtc = $completed
            disposition = 'passed-noncertifying-release-surface-audit'
            certificationPromotion = 'none'
            summary = 'Synthetic paired runtime surface evidence remains non-certifying.'
        }
        RunRoot = $runRoot
        TrackedPath = $trackedPath
    }
}

function New-TestRetentionBundle {
    param(
        [string]$RepositoryRoot,
        [string]$EvidenceRoot,
        $Candidate,
        [string]$HarnessHead)

    $candidateId = [string]$Candidate.Manifest.candidate.id
    $candidateBinding = ('4' * 64)
    $runLeaf = '20260720T120300Z-555555555555'
    $runId = 'gate1_20260720T120300Z_55555555555566666666'
    $started = '2026-07-20T12:03:00Z'
    $completed = '2026-07-20T12:04:00Z'
    $runRoot = Join-Path (Join-Path (Join-Path $EvidenceRoot $candidateId) `
        'gate1-runtime-retention') $runLeaf
    New-Item -ItemType Directory -Path $runRoot -Force | Out-Null
    Write-TestText (Join-Path $runRoot 'identity\candidate.json') `
        $Candidate.ManifestText
    Write-TestText (Join-Path $runRoot 'identity\candidate.ready.json') `
        $Candidate.ReadyText

    $packageRows = New-Object Collections.Generic.List[object]
    foreach ($indexRow in @($Candidate.PackageRows)) {
        $content = [string]$Candidate.PackageContents[$indexRow.indexPath]
        $portable = 'identity/package/' + [string]$indexRow.indexPath
        $path = Join-Path $runRoot $portable.Replace('/', '\')
        Write-TestText $path $content
        [void]$packageRows.Add([pscustomobject][ordered]@{
            indexPath = [string]$indexRow.indexPath
            path = $portable
            length = [long]$indexRow.length
            sha256 = [string]$indexRow.sha256
        })
    }
    $owner = [ordered]@{
        schemaVersion = 1
        evidenceKind = 'packaged-gate1-runtime-retention'
        runId = $runId
        candidateId = $candidateId
        purpose = 'packaged-gate1-runtime-retention'
    }
    $null = Write-TestJson (Join-Path $runRoot 'run.owner.json') $owner
    $null = Write-TestJson (Join-Path $runRoot `
        'configuration\HST_Settings.json') ([ordered]@{
            schemaVersion = 24
            persistence = [ordered]@{
                autosaveIntervalSeconds = 60
                majorChangeDebounceSeconds = 120
            }
        })

    $lineageContexts = New-Object Collections.Generic.List[object]
    $runtimeContexts = New-Object Collections.Generic.List[object]
    $standardStages = New-Object Collections.Generic.List[object]
    for ($ordinal = 0; $ordinal -lt 5; $ordinal++) {
        $stage = $script:Stages[$ordinal]
        [void]$lineageContexts.Add([pscustomobject][ordered]@{
            ordinal = $ordinal
            stage = $stage
            candidateBindingSha256 = $candidateBinding
            contextId = ('6' * 31) + [string]$ordinal
        })
        [void]$runtimeContexts.Add([pscustomobject][ordered]@{
            ordinal = $ordinal
            stage = $stage
            candidateBindingSha256 = $candidateBinding
            contextId = ('7' * 31) + [string]$ordinal
        })
        [void]$standardStages.Add([pscustomobject][ordered]@{
            ordinal = $ordinal
            stage = $stage
            contextId = ('7' * 31) + [string]$ordinal
            candidateBindingSha256 = $candidateBinding
            serverStopDisposition = 'clean'
            clientLaunched = $ordinal -eq 2
            receiptPath = 'raw/standard-runtime/' + $ordinal + '/receipt.json'
            receiptSha256 = Get-TestSha256Text ("receipt-$ordinal`n")
        })
    }

    $tools = New-TestToolRows $RepositoryRoot $script:RetentionToolPaths
    $candidateValue = [ordered]@{
        candidateId = $candidateId
        candidateVersion = [string]$Candidate.Manifest.candidate.version
        runtimeUseDisposition = 'active-runtime-candidate'
        gitHead = [string]$Candidate.Manifest.source.gitHead
        embeddedBuildSha =
            [string]$Candidate.Manifest.source.embeddedImplementation.sha
        embeddedBuildUtc =
            [string]$Candidate.Manifest.source.embeddedImplementation.utc
        embeddedBuildLabel =
            [string]$Candidate.Manifest.source.embeddedImplementation.label
        campaignSchema = 71
        runtimeSettingsSchema = 24
        addonId = 'Partisan'
        addonGuid = '698532771130111D'
        packageHashAlgorithm = 'sha256-manifest-v1'
        packageSha256 = [string]$Candidate.Manifest.package.sha256
        manifestSha256 = [string]$Candidate.ManifestSha256
        readySha256 = [string]$Candidate.ReadySha256
        workbenchCrc = '1234abcd'
        manifestPath = 'identity/candidate.json'
        readyPath = 'identity/candidate.ready.json'
        packageRoot = 'identity/package/Partisan'
        packageFiles = $packageRows.ToArray()
        executables = [ordered]@{
            serverDiagnostic = $Candidate.ServerDiagnostic
            clientDiagnostic = $Candidate.ClientDiagnostic
            server = $Candidate.Server
            client = $Candidate.Client
        }
    }
    $outcome = [ordered]@{
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
    $run = [ordered]@{
        schemaVersion = 1
        evidenceKind = 'packaged-gate1-runtime-retention'
        contractId = 'partisan.gate1-runtime-retention.v1'
        runId = $runId
        startedUtc = $started
        completedUtc = $completed
        candidate = $candidateValue
        harness = [ordered]@{
            gitHead = $HarnessHead
            clean = $true
            tools = $tools
        }
        scenario = [ordered]@{
            executionMode = 'two-phase-engine'
            worldResource = 'Worlds/HST_Everon/HST_Everon.ent'
            missionHeader = 'Missions/HST_Everon.conf'
            stages = $script:Stages
            claimScope = 'raw-retention-only'
            certificationClaim = 'none'
        }
        configuration = [ordered]@{
            settingsPath = 'configuration/HST_Settings.json'
            settingsSha256 = (Get-TestSignature (Join-Path $runRoot `
                'configuration\HST_Settings.json')).sha256
            launchContractPath = 'configuration/launch-contract.json'
            launchContractSha256 = ('8' * 64)
            autosaveIntervalSeconds = 60
            majorChangeDebounceSeconds = 120
        }
        lineage = [ordered]@{
            executionClass = 'diagnostic-only-save-lineage'
            retailClaim = 'none'
            contexts = $lineageContexts.ToArray()
        }
        runtime = [ordered]@{
            executionClass = 'standard-load-start-log-retention'
            mutationAuthority = 'none'
            byteStabilityClaim = 'observation-only'
            contexts = $runtimeContexts.ToArray()
        }
        persistence = [ordered]@{ stages = @() }
        outcome = $outcome
        files = @()
    }
    $null = Write-TestJson (Join-Path $runRoot `
        'configuration\launch-contract.json') ([ordered]@{
            schemaVersion = 1
            contractId = 'partisan.gate1-runtime-retention.v1'
            runId = $runId
        })
    $launchSignature = Get-TestSignature (Join-Path $runRoot `
        'configuration\launch-contract.json')
    $run.configuration.launchContractSha256 = $launchSignature.sha256
    $payloadRows = Get-TestPayloadRows $runRoot -WithRole
    $run.files = $payloadRows
    $runSignature = Write-TestJson (Join-Path $runRoot 'run.json') $run
    $aggregate = Get-TestRowsDigest $payloadRows
    $index = [ordered]@{
        schemaVersion = 1
        contractId = 'partisan.gate1-runtime-retention.index.v1'
        evidenceKind = 'packaged-gate1-runtime-retention'
        runId = $runId
        candidateId = $candidateId
        gitHead = [string]$Candidate.Manifest.source.gitHead
        packageSha256 = [string]$Candidate.Manifest.package.sha256
        candidateBindingSha256 = $candidateBinding
        run = [ordered]@{
            path = 'run.json'
            length = [long]$runSignature.length
            sha256 = [string]$runSignature.sha256
        }
        topology = [ordered]@{
            stageCount = 5
            diagnosticContextCount = 5
            diagnosticServerLaunchCount = 5
            diagnosticClientLaunchCount = 1
            standardContextCount = 5
            standardServerLaunchCount = 5
            standardClientLaunchCount = 1
            standardStages = $standardStages.ToArray()
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
            executionMode = 'two-phase-engine'
        }
        files = $payloadRows
        filesAggregateSha256 = $aggregate
        disposition = 'passed-noncertifying-retention'
    }
    $externalIndexSignature = Write-TestJson `
        (Join-Path $runRoot 'release-index.json') $index
    $summaryPath =
        'docs/evidence/gate1-runtime-retention/synthetic-retention.json'
    $trackedPath = Join-Path $RepositoryRoot $summaryPath.Replace('/', '\')
    $trackedParent = Split-Path -Parent $trackedPath
    New-Item -ItemType Directory -Path $trackedParent -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $runRoot 'release-index.json') `
        -Destination $trackedPath -Force
    $ready = [ordered]@{
        schemaVersion = 1
        contractId = 'partisan.gate1-runtime-retention.ready.v1'
        evidenceKind = 'packaged-gate1-runtime-retention'
        runId = $runId
        candidateId = $candidateId
        run = [ordered]@{
            path = 'run.json'
            length = [long]$runSignature.length
            sha256 = [string]$runSignature.sha256
        }
        index = [ordered]@{
            path = 'release-index.json'
            length = [long]$externalIndexSignature.length
            sha256 = [string]$externalIndexSignature.sha256
        }
        disposition = 'passed-noncertifying-retention'
        publishedUtc = '2026-07-20T12:04:30Z'
    }
    $readySignature = Write-TestJson (Join-Path $runRoot 'run.ready.json') $ready
    return [pscustomobject][ordered]@{
        Record = [pscustomobject][ordered]@{
            status = 'passed-noncertifying'
            summaryPath = $summaryPath
            summarySha256 = [string]$externalIndexSignature.sha256
            runReadySha256 = [string]$readySignature.sha256
            candidateId = $candidateId
            candidateSourceHead = [string]$Candidate.Manifest.source.gitHead
            packageSha256 = [string]$Candidate.Manifest.package.sha256
            candidateBindingSha256 = $candidateBinding
            harnessGitHead = $HarnessHead
            runId = $runId
            runLeafId = $runLeaf
            startedUtc = $started
            completedUtc = $completed
            disposition = 'passed-noncertifying-retention'
            certificationClaim = 'none'
            standardSaveRestorationCertified = $false
            summary = 'Synthetic two-phase byte-retention evidence remains non-certifying.'
        }
        RunRoot = $runRoot
        TrackedPath = $trackedPath
    }
}

function New-TestFixture {
    param(
        [string]$Root,
        [string]$SurfacePackagePathPrefix = 'package/')
    $repository = Join-Path $Root 'repo'
    $external = Join-Path $Root 'external'
    New-Item -ItemType Directory -Path $external -Force | Out-Null
    $commits = Initialize-TestRepository $repository
    $candidate = New-TestCandidate $commits.CandidateHead
    $identity = Get-TestCandidateIdentity $candidate
    $surface = New-TestSurfaceBundle `
        $repository $external $candidate $commits.HarnessHead `
        -PackagePathPrefix $SurfacePackagePathPrefix
    $retention = New-TestRetentionBundle `
        $repository $external $candidate $commits.HarnessHead
    return [pscustomobject][ordered]@{
        Root = $Root
        RepositoryRoot = $repository
        EvidenceRoot = $external
        Candidate = $candidate
        CandidateIdentity = $identity
        StatusAsOfUtc = [DateTimeOffset]'2026-07-20T12:10:00Z'
        Evidence = [pscustomobject][ordered]@{
            releaseSurfaceAudit = $surface.Record
            gate1RuntimeRetention = $retention.Record
        }
        Surface = $surface
        Retention = $retention
    }
}

function Copy-TestValue {
    param($Value)
    return (($Value | ConvertTo-Json -Depth 100) | ConvertFrom-Json)
}

function Invoke-TestConsumer {
    param($Fixture, $Evidence, [switch]$FullAccepted,
        [AllowEmptyString()][string]$EvidenceRoot = $Fixture.EvidenceRoot)
    return Assert-PartisanGate1EvidencePair `
        -Evidence $Evidence `
        -CandidateIdentity $Fixture.CandidateIdentity `
        -StatusAsOfUtc $Fixture.StatusAsOfUtc `
        -RepositoryRoot $Fixture.RepositoryRoot `
        -EvidenceBundleRoot $EvidenceRoot `
        -FullCampaignDebugAccepted:$FullAccepted
}

function Invoke-TestConsumerGitAndToolsValidation {
    param(
        [string]$RepositoryRoot,
        [string]$CandidateHead,
        [string]$HarnessHead,
        [object[]]$Tools,
        [Collections.IDictionary]$ExpectedTools,
        [string]$Label)

    $consumerModule = Get-Module -Name Partisan.Gate1EvidenceConsumer
    if ($null -eq $consumerModule) {
        throw 'Gate 1 evidence consumer module is not loaded.'
    }
    & $consumerModule {
        param(
            [string]$RepositoryRoot,
            [string]$CandidateHead,
            [string]$HarnessHead,
            [object[]]$Tools,
            [Collections.IDictionary]$ExpectedTools,
            [string]$Label)
        Assert-Gate1ConsumerGitAndTools `
            $RepositoryRoot $CandidateHead $HarnessHead $Tools `
            $ExpectedTools $Label
    } $RepositoryRoot $CandidateHead $HarnessHead $Tools $ExpectedTools $Label
}

function Assert-TestPassed {
    param([string]$Label, [scriptblock]$Action)
    try {
        $value = & $Action
    }
    catch { throw "$Label unexpectedly failed: $($_.Exception.Message)" }
    $script:Passed++
    return $value
}

function Assert-TestRejected {
    param([string]$Label, [scriptblock]$Action)
    $threw = $false
    try { $null = & $Action }
    catch { $threw = $true }
    if (-not $threw) { throw "$Label was unexpectedly accepted." }
    $script:Rejected++
}

function Assert-TestSurfaceIndexMutationRejected {
    param(
        $Fixture,
        [string]$Label,
        [scriptblock]$Mutation,
        [string]$ExpectedMessage)

    $externalPath = Join-Path $Fixture.Surface.RunRoot 'release-index.json'
    $trackedPath = $Fixture.Surface.TrackedPath
    $readyPath = Join-Path $Fixture.Surface.RunRoot 'run.ready.json'
    $externalBytes = [IO.File]::ReadAllBytes($externalPath)
    $trackedBytes = [IO.File]::ReadAllBytes($trackedPath)
    $readyBytes = [IO.File]::ReadAllBytes($readyPath)
    try {
        $index = Read-TestJson $externalPath
        & $Mutation $index
        $indexSignature = Write-TestJson $externalPath $index
        Write-TestBytes $trackedPath ([IO.File]::ReadAllBytes($externalPath))

        $ready = Read-TestJson $readyPath
        $ready.releaseIndex.signature = $indexSignature
        $readySignature = Write-TestJson $readyPath $ready

        $case = Copy-TestValue $Fixture.Evidence
        $case.releaseSurfaceAudit.summarySha256 =
            [string]$indexSignature.sha256
        $case.releaseSurfaceAudit.runReadySha256 =
            [string]$readySignature.sha256

        $message = $null
        try { $null = Invoke-TestConsumer $Fixture $case }
        catch { $message = [string]$_.Exception.Message }
        if ($null -eq $message) {
            throw "$Label was unexpectedly accepted."
        }
        if ($message -cnotlike ('*' + $ExpectedMessage + '*')) {
            throw ("{0} was rejected for the wrong reason: {1}" -f
                $Label, $message)
        }
        $script:Rejected++
    }
    finally {
        Write-TestBytes $externalPath $externalBytes
        Write-TestBytes $trackedPath $trackedBytes
        Write-TestBytes $readyPath $readyBytes
    }
}

function Remove-TestFixtureRoot {
    param([string]$Path)
    $full = [IO.Path]::GetFullPath($Path)
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/')
    if ((Split-Path -Parent $full) -cne $temp -or
        (Split-Path -Leaf $full) -cnotmatch
            '^\.partisan-gate1-consumer-[0-9a-f]{32}$') {
        throw 'Consumer self-test cleanup target escaped its exact contract.'
    }
    if (-not (Test-Path -LiteralPath $full)) { return }
    foreach ($item in @(Get-ChildItem -LiteralPath $full -Recurse -Force)) {
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'Consumer self-test cleanup encountered an unexpected reparse point.'
        }
        $item.Attributes = [IO.FileAttributes]::Normal
    }
    [IO.Directory]::Delete($full, $true)
}

$fixtureRoot = Join-Path ([IO.Path]::GetTempPath()) `
    ('.partisan-gate1-consumer-' + [Guid]::NewGuid().ToString('N'))
$fixture = $null
$publisherFaultName = 'PARTISAN_GATE1_CONSUMER_STUB_FAULT'
$publisherFaultPrior = [Environment]::GetEnvironmentVariable(
    $publisherFaultName, 'Process')
[Environment]::SetEnvironmentVariable($publisherFaultName, $null, 'Process')
try {
    $fixture = New-TestFixture $fixtureRoot
    $valid = Assert-TestPassed 'valid structurally representative pair' {
        Invoke-TestConsumer $fixture $fixture.Evidence -FullAccepted
    }
    if (-not $valid.Present -or
        [string]$valid.ReleaseSurfaceAudit.CertificationPromotion -cne 'none' -or
        [string]$valid.ReleaseSurfaceAudit.HardDiagnosticPolicy -cne
            'script-engine-and-process-fatal-v1' -or
        [long]$valid.ReleaseSurfaceAudit.HardDiagnosticFreeModeCount -ne 1 -or
        [long]$valid.ReleaseSurfaceAudit.
            ApprovedStockDiagnosticClusterModeCount -ne 1 -or
        [long]$valid.ReleaseSurfaceAudit.HardDiagnosticRawLineCount -ne 6 -or
        [long]$valid.ReleaseSurfaceAudit.HardDiagnosticEventCount -ne 2 -or
        [long]$valid.ReleaseSurfaceAudit.
            ApprovedStockDiagnosticRawLineCount -ne 6 -or
        [long]$valid.ReleaseSurfaceAudit.
            ApprovedStockDiagnosticEventCount -ne 2 -or
        [long]$valid.ReleaseSurfaceAudit.
            UnapprovedHardDiagnosticRawLineCount -ne 0 -or
        [long]$valid.ReleaseSurfaceAudit.
            UnapprovedHardDiagnosticEventCount -ne 0 -or
        @($valid.ReleaseSurfaceAudit.ModeDiagnosticCensus).Count -ne 2 -or
        [string]$valid.Gate1RuntimeRetention.CertificationClaim -cne 'none' -or
        [bool]$valid.Gate1RuntimeRetention.StandardSaveRestorationCertified) {
        throw 'Valid pair projection promoted certification.'
    }
    $badPackagePathFixture = New-TestFixture `
        (Join-Path $fixtureRoot 'bad-surface-package-path') `
        -SurfacePackagePathPrefix 'staging/'
    Assert-TestRejected 'surface package-row path drift' {
        Invoke-TestConsumer $badPackagePathFixture `
            $badPackagePathFixture.Evidence
    }
    $absent = Assert-TestPassed 'optional absent pair' {
        Invoke-TestConsumer $fixture ([pscustomobject][ordered]@{})
    }
    if ($absent.Present) { throw 'Absent pair projected as present.' }

    foreach ($publisherFault in @(
            [pscustomobject]@{
                Name = 'surface-bool'
                Label = 'surface publisher string boolean result'
            },
            [pscustomobject]@{
                Name = 'retention-signature'
                Label = 'retention publisher ready-signature drift'
            },
            [pscustomobject]@{
                Name = 'surface-extra-output'
                Label = 'surface publisher extra output'
            },
            [pscustomobject]@{
                Name = 'surface-string-builder'
                Label = 'surface publisher non-string identity'
            },
            [pscustomobject]@{
                Name = 'surface-same-byte-rewrite'
                Label = 'surface publisher same-byte rewrite'
            },
            [pscustomobject]@{
                Name = 'surface-transient-file'
                Label = 'surface publisher transient file write'
            })) {
        try {
            [Environment]::SetEnvironmentVariable(
                $publisherFaultName, [string]$publisherFault.Name, 'Process')
            Assert-TestRejected ([string]$publisherFault.Label) {
                Invoke-TestConsumer $fixture $fixture.Evidence
            }
        }
        finally {
            [Environment]::SetEnvironmentVariable(
                $publisherFaultName, $null, 'Process')
        }
    }

    $nonObjectEvidenceValues = New-Object Collections.Generic.List[object]
    [void]$nonObjectEvidenceValues.Add('absent')
    [void]$nonObjectEvidenceValues.Add(7)
    [void]$nonObjectEvidenceValues.Add($true)
    [void]$nonObjectEvidenceValues.Add([object[]]@('absent'))
    foreach ($nonObjectEvidence in $nonObjectEvidenceValues) {
        Assert-TestRejected 'non-object evidence container' {
            Invoke-TestConsumer $fixture $nonObjectEvidence
        }
    }

    Assert-TestRejected 'accepted Full Campaign Debug without pair' {
        Invoke-TestConsumer $fixture ([pscustomobject][ordered]@{}) -FullAccepted
    }
    Assert-TestRejected 'surface-only half pair' {
        Invoke-TestConsumer $fixture ([pscustomobject][ordered]@{
            releaseSurfaceAudit = $fixture.Evidence.releaseSurfaceAudit
        })
    }
    Assert-TestRejected 'retention-only half pair' {
        Invoke-TestConsumer $fixture ([pscustomobject][ordered]@{
            gate1RuntimeRetention = $fixture.Evidence.gate1RuntimeRetention
        })
    }
    Assert-TestRejected 'case-variant pair property names' {
        Invoke-TestConsumer $fixture ([pscustomobject][ordered]@{
            ReleaseSurfaceAudit = $fixture.Evidence.releaseSurfaceAudit
            gate1RuntimeRetention = $fixture.Evidence.gate1RuntimeRetention
        })
    }
    Assert-TestRejected 'explicitly null pair records' {
        Invoke-TestConsumer $fixture ([pscustomobject][ordered]@{
            releaseSurfaceAudit = $null
            gate1RuntimeRetention = $null
        })
    }
    Assert-TestRejected 'missing explicit evidence root' {
        Invoke-TestConsumer $fixture $fixture.Evidence -EvidenceRoot ''
    }

    $caseVariantTools = New-TestToolRows `
        $fixture.RepositoryRoot $script:SurfaceToolPaths
    Assert-TestPassed 'consumer exact ordered-map tool roles' {
        Invoke-TestConsumerGitAndToolsValidation `
            -RepositoryRoot $fixture.RepositoryRoot `
            -CandidateHead $fixture.CandidateIdentity.CandidateSourceHead `
            -HarnessHead $fixture.Surface.Record.harnessGitHead `
            -Tools $caseVariantTools `
            -ExpectedTools $script:SurfaceToolPaths `
            -Label 'exact surface publisher'
    }
    $caseVariantTools[0].role = 'RUNNER'
    Assert-TestRejected 'consumer case-variant role replacing runner' {
        Invoke-TestConsumerGitAndToolsValidation `
            -RepositoryRoot $fixture.RepositoryRoot `
            -CandidateHead $fixture.CandidateIdentity.CandidateSourceHead `
            -HarnessHead $fixture.Surface.Record.harnessGitHead `
            -Tools $caseVariantTools `
            -ExpectedTools $script:SurfaceToolPaths `
            -Label 'case-variant surface publisher'
    }

    $case = Copy-TestValue $fixture.Evidence
    $case.releaseSurfaceAudit | Add-Member -NotePropertyName harmless `
        -NotePropertyValue $true
    Assert-TestRejected 'extra status property' {
        Invoke-TestConsumer $fixture $case
    }
    $case = Copy-TestValue $fixture.Evidence
    $case.releaseSurfaceAudit.certificationPromotion = 'gate1'
    Assert-TestRejected 'surface certification promotion' {
        Invoke-TestConsumer $fixture $case
    }
    $case = Copy-TestValue $fixture.Evidence
    $case.gate1RuntimeRetention.certificationClaim = 'restoration'
    Assert-TestRejected 'retention certification claim' {
        Invoke-TestConsumer $fixture $case
    }
    $case = Copy-TestValue $fixture.Evidence
    $case.gate1RuntimeRetention.standardSaveRestorationCertified = $true
    Assert-TestRejected 'retention restoration promotion' {
        Invoke-TestConsumer $fixture $case
    }
    $case = Copy-TestValue $fixture.Evidence
    $case.releaseSurfaceAudit.disposition = 'passed'
    Assert-TestRejected 'surface disposition drift' {
        Invoke-TestConsumer $fixture $case
    }
    $case = Copy-TestValue $fixture.Evidence
    $case.gate1RuntimeRetention.candidateId = 'different-candidate'
    Assert-TestRejected 'candidate tuple drift' {
        Invoke-TestConsumer $fixture $case
    }
    $case = Copy-TestValue $fixture.Evidence
    $case.releaseSurfaceAudit.candidateBindingSha256 = ('9' * 64)
    Assert-TestRejected 'candidate binding drift' {
        Invoke-TestConsumer $fixture $case
    }
    $case = Copy-TestValue $fixture.Evidence
    $case.gate1RuntimeRetention.runLeafId =
        '20260720T120300Z-aaaaaaaaaaaa'
    Assert-TestRejected 'retention run-leaf binding drift' {
        Invoke-TestConsumer $fixture $case
    }
    $case = Copy-TestValue $fixture.Evidence
    $case.releaseSurfaceAudit.completedUtc = '2026-07-20T12:11:00Z'
    Assert-TestRejected 'future status timestamp' {
        Invoke-TestConsumer $fixture $case
    }
    $case = Copy-TestValue $fixture.Evidence
    $case.releaseSurfaceAudit.summaryPath = '../release-index.json'
    Assert-TestRejected 'tracked index path escape' {
        Invoke-TestConsumer $fixture $case
    }
    $case = Copy-TestValue $fixture.Evidence
    $localPathProbe = 'C' + [char]58 + '\machine\evidence'
    $case.releaseSurfaceAudit.summary = "captured from $localPathProbe"
    Assert-TestRejected 'status local-path leakage' {
        Invoke-TestConsumer $fixture $case
    }
    $case = Copy-TestValue $fixture.Evidence
    $case.gate1RuntimeRetention.summarySha256 = ('0' * 64)
    Assert-TestRejected 'tracked index hash mismatch' {
        Invoke-TestConsumer $fixture $case
    }
    $case = Copy-TestValue $fixture.Evidence
    $case.releaseSurfaceAudit.runReadySha256 = ('0' * 64)
    Assert-TestRejected 'ready seal hash mismatch' {
        Invoke-TestConsumer $fixture $case
    }

    $trackedBytes = [IO.File]::ReadAllBytes($fixture.Surface.TrackedPath)
    try {
        $text = $script:Utf8NoBom.GetString($trackedBytes)
        $duplicate = $text -replace '"schemaVersion"\s*:\s*2,',
            '"schemaVersion": 2, "schemaVersion": 2,'
        Write-TestText $fixture.Surface.TrackedPath $duplicate
        $case = Copy-TestValue $fixture.Evidence
        $case.releaseSurfaceAudit.summarySha256 =
            (Get-TestSignature $fixture.Surface.TrackedPath).sha256
        Assert-TestRejected 'duplicate tracked-index property' {
            Invoke-TestConsumer $fixture $case
        }
    }
    finally { Write-TestBytes $fixture.Surface.TrackedPath $trackedBytes }

    $trackedBytes = [IO.File]::ReadAllBytes($fixture.Retention.TrackedPath)
    try {
        Write-TestBytes $fixture.Retention.TrackedPath `
            ([byte[]](0xff, 0xfe, 0x7b, 0x7d))
        $case = Copy-TestValue $fixture.Evidence
        $case.gate1RuntimeRetention.summarySha256 =
            (Get-TestSignature $fixture.Retention.TrackedPath).sha256
        Assert-TestRejected 'non-UTF8 tracked index' {
            Invoke-TestConsumer $fixture $case
        }
    }
    finally { Write-TestBytes $fixture.Retention.TrackedPath $trackedBytes }

    $externalSurfaceIndex = Join-Path $fixture.Surface.RunRoot `
        'release-index.json'
    $externalBytes = [IO.File]::ReadAllBytes($externalSurfaceIndex)
    try {
        Write-TestBytes $externalSurfaceIndex `
            ($externalBytes + $script:Utf8NoBom.GetBytes(" `n"))
        Assert-TestRejected 'external/tracked index byte drift' {
            Invoke-TestConsumer $fixture $fixture.Evidence
        }
    }
    finally { Write-TestBytes $externalSurfaceIndex $externalBytes }

    Assert-TestSurfaceIndexMutationRejected `
        -Fixture $fixture `
        -Label 'retail forbidden-member count drift' `
        -Mutation {
            param($Index)
            $Index.modes[0].forbiddenMemberCount = 66
        } `
        -ExpectedMessage `
            'retail forbiddenMemberCount must be the exact integer 67.'
    Assert-TestSurfaceIndexMutationRejected `
        -Fixture $fixture `
        -Label 'diagnostic production-member count drift' `
        -Mutation {
            param($Index)
            $Index.modes[1].productionMemberCount = 90
        } `
        -ExpectedMessage `
            'diagnostic productionMemberCount must be the exact integer 91.'
    Assert-TestSurfaceIndexMutationRejected `
        -Fixture $fixture `
        -Label 'string forbidden-member count' `
        -Mutation {
            param($Index)
            $Index.modes[0].forbiddenMemberCount = '67'
        } `
        -ExpectedMessage `
            'retail forbiddenMemberCount must be the exact integer 67.'
    Assert-TestSurfaceIndexMutationRejected `
        -Fixture $fixture `
        -Label 'CLI-only candidate mount projection' `
        -Mutation {
            param($Index)
            $Index.modes[0].candidateMountAttestation.recordCount = 0
        } `
        -ExpectedMessage `
            ('retail index projection candidate mount attestation.' +
                'recordCount must be the exact integer 2.')
    Assert-TestSurfaceIndexMutationRejected `
        -Fixture $fixture `
        -Label 'foreign-path candidate mount projection' `
        -Mutation {
            param($Index)
            $Index.modes[0].candidateMountAttestation.exactPathCount = 0
        } `
        -ExpectedMessage `
            ('retail index projection candidate mount attestation.' +
                'exactPathCount must be the exact integer 2.')
    Assert-TestSurfaceIndexMutationRejected `
        -Fixture $fixture `
        -Label 'case-drifted candidate GUID mount projection' `
        -Mutation {
            param($Index)
            $Index.modes[0].candidateMountAttestation.guidExact = $false
        } `
        -ExpectedMessage `
            ('retail index projection candidate mount attestation is not ' +
                'an exact canonical packed candidate mount.')
    Assert-TestSurfaceIndexMutationRejected `
        -Fixture $fixture `
        -Label 'retail hard-diagnostic policy drift' `
        -Mutation {
            param($Index)
            $Index.modes[0].hardDiagnosticPolicy =
                'script-engine-and-process-fatal-v2'
        } `
        -ExpectedMessage `
            'retail index projection hard-diagnostic policy is not exact.'
    Assert-TestSurfaceIndexMutationRejected `
        -Fixture $fixture `
        -Label 'retail non-empty crash-log projection' `
        -Mutation {
            param($Index)
            $Index.modes[0].crashLogContentValid = $false
        } `
        -ExpectedMessage `
            'retail index projection contains a non-empty crash log.'
    Assert-TestSurfaceIndexMutationRejected `
        -Fixture $fixture `
        -Label 'retail hard-diagnostic accounting drift' `
        -Mutation {
            param($Index)
            $Index.modes[0].hardDiagnosticRawLineCount = 5
        } `
        -ExpectedMessage `
            'retail index projection hard-diagnostic accounting is not exact.'
    Assert-TestSurfaceIndexMutationRejected `
        -Fixture $fixture `
        -Label 'retail unapproved hard event' `
        -Mutation {
            param($Index)
            $Index.modes[0].hardDiagnosticEventCount = 3
            $Index.modes[0].unapprovedHardDiagnosticEventCount = 1
        } `
        -ExpectedMessage `
            'retail index projection contains an unapproved hard diagnostic.'
    Assert-TestSurfaceIndexMutationRejected `
        -Fixture $fixture `
        -Label 'retail stock-cluster projection drift' `
        -Mutation {
            param($Index)
            $Index.modes[0].approvedStockDiagnosticClusterPresent = $false
        } `
        -ExpectedMessage `
            'must be either hard-diagnostic free or the exact six-raw-line/two-event approved stock cluster.'

    $runPath = Join-Path $fixture.Retention.RunRoot 'run.json'
    $runBytes = [IO.File]::ReadAllBytes($runPath)
    try {
        Write-TestBytes $runPath ($runBytes + $script:Utf8NoBom.GetBytes(" `n"))
        Assert-TestRejected 'run envelope signature drift' {
            Invoke-TestConsumer $fixture $fixture.Evidence
        }
    }
    finally { Write-TestBytes $runPath $runBytes }

    $readyPath = Join-Path $fixture.Surface.RunRoot 'run.ready.json'
    $readyBytes = [IO.File]::ReadAllBytes($readyPath)
    try {
        $ready = Read-TestJson $readyPath
        $ready.sealedLast = $false
        $null = Write-TestJson $readyPath $ready
        $case = Copy-TestValue $fixture.Evidence
        $case.releaseSurfaceAudit.runReadySha256 =
            (Get-TestSignature $readyPath).sha256
        Assert-TestRejected 'ready seal semantic drift' {
            Invoke-TestConsumer $fixture $case
        }
    }
    finally { Write-TestBytes $readyPath $readyBytes }

    foreach ($typedReadyMutation in @(
            [pscustomobject]@{
                Label = 'ready sealedLast string false'
                Apply = { param($Ready) $Ready.sealedLast = 'false' }
            },
            [pscustomobject]@{
                Label = 'ready schema numeric string'
                Apply = { param($Ready) $Ready.schemaVersion = '2' }
            },
            [pscustomobject]@{
                Label = 'ready schema fractional number'
                Apply = { param($Ready) $Ready.schemaVersion = 2.9 }
            })) {
        try {
            $ready = Read-TestJson $readyPath
            & $typedReadyMutation.Apply $ready
            $null = Write-TestJson $readyPath $ready
            $case = Copy-TestValue $fixture.Evidence
            $case.releaseSurfaceAudit.runReadySha256 =
                (Get-TestSignature $readyPath).sha256
            Assert-TestRejected ([string]$typedReadyMutation.Label) {
                Invoke-TestConsumer $fixture $case
            }
        }
        finally { Write-TestBytes $readyPath $readyBytes }
    }

    $extraPath = Join-Path $fixture.Retention.RunRoot 'uncensed.txt'
    try {
        Write-TestText $extraPath "uncensed`n"
        Assert-TestRejected 'uncensed raw file' {
            Invoke-TestConsumer $fixture $fixture.Evidence
        }
    }
    finally {
        if (Test-Path -LiteralPath $extraPath) {
            [IO.File]::Delete($extraPath)
        }
    }

    $toolPath = Join-Path $fixture.RepositoryRoot `
        'tools\run-guarded-release-surface-audit.ps1'
    $toolBytes = [IO.File]::ReadAllBytes($toolPath)
    try {
        Write-TestBytes $toolPath `
            ($toolBytes + $script:Utf8NoBom.GetBytes("drift`n"))
        Assert-TestRejected 'publisher worktree tool drift' {
            Invoke-TestConsumer $fixture $fixture.Evidence
        }
    }
    finally { Write-TestBytes $toolPath $toolBytes }

    $junctionTarget = Join-Path $fixtureRoot 'junction-target'
    $junctionPath = Join-Path $fixture.Surface.RunRoot 'uncensed-junction'
    New-Item -ItemType Directory -Path $junctionTarget | Out-Null
    Write-TestText (Join-Path $junctionTarget 'outside.txt') "outside`n"
    try {
        $null = New-Item -ItemType Junction -Path $junctionPath `
            -Target $junctionTarget
        Assert-TestRejected 'raw-run reparse point' {
            Invoke-TestConsumer $fixture $fixture.Evidence
        }
    }
    finally {
        if (Test-Path -LiteralPath $junctionPath) {
            [IO.Directory]::Delete($junctionPath)
        }
    }

    Write-Host (("Gate 1 evidence-pair consumer self-test passed: {0} valid/" +
        "optional cases and {1} adversarial cases.") -f
        $script:Passed, $script:Rejected)
}
finally {
    [Environment]::SetEnvironmentVariable(
        $publisherFaultName, $publisherFaultPrior, 'Process')
    Remove-TestFixtureRoot $fixtureRoot
}
