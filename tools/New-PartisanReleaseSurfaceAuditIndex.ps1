[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$RunEnvelopePath,

    [string]$OutputPath = '',

    [switch]$SyntheticFixtureValidationOnly,

    [switch]$VerifyPublishedIndex
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:RunContractId = 'partisan.release-surface-audit.run.v2'
$script:IndexContractId = 'partisan.release-surface-audit.index.v2'
$script:EvidenceKind = 'partisan_release_surface_runtime_audit_v2'
$script:IndexEvidenceKind = 'partisan_release_surface_runtime_audit_index_v2'
$script:ProbeEvidenceKind = 'partisan_release_surface_runtime_probe_v1'
$script:ContractEvidenceKind = 'partisan_release_surface_contract_v1'
$script:Modes = @('retail', 'diagnostic')
$script:RequiredLogLeaves = @('console.log', 'script.log', 'error.log')
$script:OptionalLogLeaves = @('crash.log')
$script:AllowedLogLeaves = @(
    $script:RequiredLogLeaves + $script:OptionalLogLeaves)
$script:HardDiagnosticPolicy = 'script-engine-and-process-fatal-v1'
$script:RequiredPackageFiles = @(
    'Partisan/addon.gproj',
    'Partisan/data.pak',
    'Partisan/resourceDatabase.rdb',
    'Partisan/thumbnail.png')
$script:RequiredHarnessFiles = @(
    '.partisan-release-surface-audit-owner.json',
    'Scripts/Game/PartisanReleaseSurfaceAudit.c',
    'addon.gproj')
$script:HarnessToolPaths = [ordered]@{
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
$script:Limitations = @(
    'This audit uses inert compiler and metadata probes for contracted member-surface presence; separately, it deliberately invokes production menu generation and read-only per-command availability inspection, but it does not execute command actions or mutate campaign gameplay state.',
    'Forbidden literal surfaces are proven by the candidate-bound source guard analysis, not by an unreliable package-byte string scan.',
    'It is not gameplay, multiplayer, persistence, restart, soak, or performance certification.',
    'The guarded launcher deliberately gives the child no inherited standard streams; authoritative engine output is retained in the three required logs and in crash.log when the engine emits it.',
    'The machine-bound hard-diagnostic policy is script-engine-and-process-fatal-v1: SCRIPT or ENGINE error severity, access violations, unhandled exceptions, fatal/application-crash signals, and audit ERROR markers. Other retained engine-channel severities are outside this narrow release-surface predicate. A successful crash.log must be absent or empty.',
    'A mode may contain either no hard diagnostic or one exact stock Eden shutdown cluster: two underlying support-station catalog-manager events mirrored once across console.log, script.log, and error.log after replication finishes and before game destruction. Every partial, extra, variant, misplaced, crash-channel, or unrelated hard event fails closed.')

function Get-ReleaseSurfaceIndexSha256Bytes {
    param([Parameter(Mandatory = $true)][byte[]]$Bytes)

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally { $algorithm.Dispose() }
}

function Get-ReleaseSurfaceIndexSha256Text {
    param([Parameter(Mandatory = $true)][AllowEmptyString()][string]$Text)

    return Get-ReleaseSurfaceIndexSha256Bytes `
        -Bytes ((New-Object Text.UTF8Encoding($false)).GetBytes($Text))
}

function Get-ReleaseSurfaceIndexTextArtifact {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label,
        [switch]$AllowEmpty
    )

    $leaf = [IO.Path]::GetFullPath($Path)
    if (-not (Test-Path -LiteralPath $leaf -PathType Leaf)) {
        throw "$Label is missing."
    }
    $bytes = [IO.File]::ReadAllBytes($leaf)
    if (-not $AllowEmpty -and $bytes.Length -lt 1) {
        throw "$Label is empty."
    }
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xef -and
        $bytes[1] -eq 0xbb -and $bytes[2] -eq 0xbf) {
        throw "$Label contains a UTF-8 BOM."
    }
    try {
        $encoding = New-Object Text.UTF8Encoding($false, $true)
        $text = $encoding.GetString($bytes)
    }
    catch { throw "$Label is not strict UTF-8." }
    return [pscustomobject][ordered]@{
        Path = $leaf
        Bytes = $bytes
        Text = $text
        Length = [long]$bytes.Length
        Sha256 = Get-ReleaseSurfaceIndexSha256Bytes -Bytes $bytes
    }
}

function Get-ReleaseSurfaceIndexFileSignature {
    param([Parameter(Mandatory = $true)][string]$Path)

    $leaf = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    if ($leaf.PSIsContainer) { throw 'A file signature target is a directory.' }
    return [pscustomobject][ordered]@{
        length = [long]$leaf.Length
        sha256 = (Get-FileHash -LiteralPath $leaf.FullName -Algorithm SHA256).
            Hash.ToLowerInvariant()
    }
}

function Test-ReleaseSurfaceIndexSignatureExact {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual
    )

    return $null -ne $Expected -and $null -ne $Actual -and
        [long]$Expected.length -eq [long]$Actual.length -and
        [string]$Expected.sha256 -ceq [string]$Actual.sha256
}

function Assert-ReleaseSurfaceIndexExactProperties {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string[]]$Names,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($null -eq $Value) { throw "$Label is null." }
    $actual = @($Value.PSObject.Properties | ForEach-Object { [string]$_.Name })
    if ($actual.Count -ne $Names.Count) {
        throw "$Label does not have its exact property count."
    }
    foreach ($name in $Names) {
        if ($actual -cnotcontains $name) { throw "$Label is missing property $name." }
    }
}

function Assert-ReleaseSurfaceIndexBooleanProperties {
    param($Value, [string[]]$Names, [string]$Label)

    foreach ($name in $Names) {
        if ($Value.$name -isnot [bool]) {
            throw "$Label property $name must be a JSON boolean."
        }
    }
}

function Assert-ReleaseSurfaceIndexIntegerProperties {
    param($Value, [string[]]$Names, [string]$Label)

    foreach ($name in $Names) {
        if ($Value.$name -isnot [int] -and $Value.$name -isnot [long]) {
            throw "$Label property $name must be a JSON integer."
        }
    }
}

function Assert-ReleaseSurfaceIndexStringProperties {
    param($Value, [string[]]$Names, [string]$Label)

    foreach ($name in $Names) {
        if ($Value.$name -isnot [string]) {
            throw "$Label property $name must be a JSON string."
        }
    }
}

function Assert-ReleaseSurfaceIndexStringArray {
    param($Value, [string]$Label)

    if ($null -eq $Value) { throw "$Label must be a JSON array of strings." }
    foreach ($item in @($Value)) {
        if ($item -isnot [string]) {
            throw "$Label must contain only JSON strings."
        }
    }
}

function Skip-ReleaseSurfaceIndexJsonWhitespace {
    param([string]$Text, $Index)
    while ($Index.Value -lt $Text.Length -and
        $Text[$Index.Value] -in @(' ', "`t", "`r", "`n")) {
        $Index.Value++
    }
}

function Read-ReleaseSurfaceIndexJsonString {
    param([string]$Text, $Index, [string]$Label)

    if ($Index.Value -ge $Text.Length -or $Text[$Index.Value] -ne '"') {
        throw "$Label contains an invalid JSON string."
    }
    $start = $Index.Value
    $Index.Value++
    while ($Index.Value -lt $Text.Length) {
        $character = $Text[$Index.Value]
        if ($character -eq '"') {
            $Index.Value++
            try {
                return [string]($Text.Substring(
                    $start, $Index.Value - $start) | ConvertFrom-Json)
            }
            catch { throw "$Label contains an invalid JSON string escape." }
        }
        if ([int][char]$character -lt 0x20) {
            throw "$Label contains an invalid JSON control character."
        }
        if ($character -eq '\') {
            $Index.Value++
            if ($Index.Value -ge $Text.Length) {
                throw "$Label contains an incomplete JSON escape."
            }
            $escape = $Text[$Index.Value]
            if ($escape -eq 'u') {
                if ($Index.Value + 4 -ge $Text.Length -or
                    $Text.Substring($Index.Value + 1, 4) -cnotmatch
                        '^[0-9a-fA-F]{4}$') {
                    throw "$Label contains an invalid JSON Unicode escape."
                }
                $Index.Value += 5
                continue
            }
            if ($escape -notin @('"', '\', '/', 'b', 'f', 'n', 'r', 't')) {
                throw "$Label contains an unsupported JSON escape."
            }
        }
        $Index.Value++
    }
    throw "$Label contains an unterminated JSON string."
}

function Read-ReleaseSurfaceIndexJsonValue {
    param([string]$Text, $Index, [string]$Label)

    Skip-ReleaseSurfaceIndexJsonWhitespace $Text $Index
    if ($Index.Value -ge $Text.Length) {
        throw "$Label contains an incomplete JSON value."
    }
    $character = $Text[$Index.Value]
    if ($character -eq '{') {
        $Index.Value++
        $keys = New-Object Collections.Generic.HashSet[string](
            [StringComparer]::Ordinal)
        Skip-ReleaseSurfaceIndexJsonWhitespace $Text $Index
        if ($Index.Value -lt $Text.Length -and $Text[$Index.Value] -eq '}') {
            $Index.Value++
            return
        }
        while ($true) {
            Skip-ReleaseSurfaceIndexJsonWhitespace $Text $Index
            $key = Read-ReleaseSurfaceIndexJsonString $Text $Index $Label
            if (-not $keys.Add($key)) {
                throw "$Label duplicates JSON object property $key."
            }
            Skip-ReleaseSurfaceIndexJsonWhitespace $Text $Index
            if ($Index.Value -ge $Text.Length -or $Text[$Index.Value] -ne ':') {
                throw "$Label contains a JSON property without a colon."
            }
            $Index.Value++
            Read-ReleaseSurfaceIndexJsonValue $Text $Index $Label
            Skip-ReleaseSurfaceIndexJsonWhitespace $Text $Index
            if ($Index.Value -lt $Text.Length -and $Text[$Index.Value] -eq '}') {
                $Index.Value++
                return
            }
            if ($Index.Value -ge $Text.Length -or $Text[$Index.Value] -ne ',') {
                throw "$Label contains an unterminated JSON object."
            }
            $Index.Value++
        }
    }
    if ($character -eq '[') {
        $Index.Value++
        Skip-ReleaseSurfaceIndexJsonWhitespace $Text $Index
        if ($Index.Value -lt $Text.Length -and $Text[$Index.Value] -eq ']') {
            $Index.Value++
            return
        }
        while ($true) {
            Read-ReleaseSurfaceIndexJsonValue $Text $Index $Label
            Skip-ReleaseSurfaceIndexJsonWhitespace $Text $Index
            if ($Index.Value -lt $Text.Length -and $Text[$Index.Value] -eq ']') {
                $Index.Value++
                return
            }
            if ($Index.Value -ge $Text.Length -or $Text[$Index.Value] -ne ',') {
                throw "$Label contains an unterminated JSON array."
            }
            $Index.Value++
        }
    }
    if ($character -eq '"') {
        $null = Read-ReleaseSurfaceIndexJsonString $Text $Index $Label
        return
    }
    foreach ($literal in @('true', 'false', 'null')) {
        if ($Index.Value + $literal.Length -le $Text.Length -and
            $Text.Substring($Index.Value, $literal.Length) -ceq $literal) {
            $Index.Value += $literal.Length
            return
        }
    }
    $match = [regex]::Match(
        $Text.Substring($Index.Value),
        '^-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?')
    if ($match.Success) {
        $Index.Value += $match.Length
        return
    }
    throw "$Label contains an invalid JSON value."
}

function Assert-ReleaseSurfaceIndexNoDuplicateJsonKeys {
    param([string]$Text, [string]$Label)

    $index = 0
    Read-ReleaseSurfaceIndexJsonValue $Text ([ref]$index) $Label
    Skip-ReleaseSurfaceIndexJsonWhitespace $Text ([ref]$index)
    if ($index -ne $Text.Length) { throw "$Label contains trailing JSON content." }
}

function Assert-ReleaseSurfaceIndexNoLocalPathText {
    param([string]$Text, [string]$Label)

    if ($Text -match '(?i)(?:[a-z]:[\\/]|\\\\|file://|/(?:Users|home|mnt)/)') {
        throw "$Label contains a machine-local path."
    }
}

function Read-ReleaseSurfaceIndexJsonArtifact {
    param(
        [string]$Path,
        [string]$Label,
        [switch]$Portable
    )

    $artifact = Get-ReleaseSurfaceIndexTextArtifact -Path $Path -Label $Label
    Assert-ReleaseSurfaceIndexNoDuplicateJsonKeys $artifact.Text $Label
    if ($Portable) {
        Assert-ReleaseSurfaceIndexNoLocalPathText $artifact.Text $Label
    }
    try { $value = $artifact.Text | ConvertFrom-Json }
    catch { throw "$Label is not valid JSON." }
    return [pscustomobject][ordered]@{ Artifact = $artifact; Value = $value }
}

function Assert-ReleaseSurfaceIndexPortableRelativePath {
    param([string]$Path, [string]$Label)

    if ([string]::IsNullOrWhiteSpace($Path) -or $Path.Contains('\') -or
        $Path.Contains(':') -or $Path.StartsWith('/') -or
        $Path -match '[\r\n\t]' -or $Path.Split('/') -contains '..' -or
        $Path.Split('/') -contains '.' -or $Path.EndsWith('/')) {
        throw "$Label is not a canonical portable relative path."
    }
}

function Resolve-ReleaseSurfaceIndexPortableFile {
    param([string]$Root, [string]$Path, [string]$Label)

    Assert-ReleaseSurfaceIndexPortableRelativePath $Path $Label
    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $candidate = [IO.Path]::GetFullPath((Join-Path $rootPath $Path))
    $prefix = $rootPath + [IO.Path]::DirectorySeparatorChar
    if (-not $candidate.StartsWith(
            $prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label escaped the release-surface run root."
    }
    return $candidate
}

function ConvertTo-ReleaseSurfaceIndexPortableRelativePath {
    param([string]$Root, [string]$Path, [string]$Label)

    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $fullPath = [IO.Path]::GetFullPath($Path)
    $prefix = $rootPath + [IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith(
            $prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label escaped its release-surface root."
    }
    $relative = $fullPath.Substring($prefix.Length).Replace('\', '/')
    Assert-ReleaseSurfaceIndexPortableRelativePath $relative $Label
    return $relative
}

function Get-ReleaseSurfaceIndexCanonicalRowsDigest {
    param([Parameter(Mandatory = $true)][AllowEmptyCollection()][object[]]$Rows)

    $lines = @($Rows | Sort-Object path | ForEach-Object {
        "{0}`t{1}`t{2}" -f $_.sha256, [long]$_.length, [string]$_.path
    })
    return Get-ReleaseSurfaceIndexSha256Text -Text (($lines -join "`n") + "`n")
}

function Test-ReleaseSurfaceIndexStringArrayExact {
    param(
        [AllowEmptyCollection()][object[]]$Expected,
        [AllowEmptyCollection()][object[]]$Actual
    )

    if ($Expected.Count -ne $Actual.Count) { return $false }
    for ($index = 0; $index -lt $Expected.Count; $index++) {
        if ([string]$Expected[$index] -cne [string]$Actual[$index]) {
            return $false
        }
    }
    return $true
}

function Get-ReleaseSurfaceIndexOptionValue {
    param([string[]]$Arguments, [string]$Option, [string]$Label)

    $positions = @()
    for ($index = 0; $index -lt $Arguments.Count; $index++) {
        if ([string]$Arguments[$index] -ceq $Option) { $positions += $index }
    }
    if ($positions.Count -ne 1 -or $positions[0] + 1 -ge $Arguments.Count -or
        [string]$Arguments[$positions[0] + 1] -like '-*') {
        throw "$Label does not contain one exact $Option value pair."
    }
    return [string]$Arguments[$positions[0] + 1]
}

function Write-ReleaseSurfaceIndexAtomicCreateOnly {
    param([string]$Path, $Value)

    $bytes = Get-ReleaseSurfaceIndexCanonicalJsonBytes $Value
    if (Test-Path -LiteralPath $Path) {
        $existing = [IO.File]::ReadAllBytes($Path)
        if ($existing.Length -ne $bytes.Length -or
            (Get-ReleaseSurfaceIndexSha256Bytes $existing) -cne
                (Get-ReleaseSurfaceIndexSha256Bytes $bytes)) {
            throw 'The release-surface audit index already exists with different bytes.'
        }
        return Get-ReleaseSurfaceIndexFileSignature $Path
    }
    $temporary = $Path + '.tmp.' + [Guid]::NewGuid().ToString('N')
    try {
        [IO.File]::WriteAllBytes($temporary, $bytes)
        [IO.File]::Move($temporary, $Path)
    }
    finally {
        if (Test-Path -LiteralPath $temporary) {
            Remove-Item -LiteralPath $temporary -Force -ErrorAction SilentlyContinue
        }
    }
    return Get-ReleaseSurfaceIndexFileSignature $Path
}

function Assert-ReleaseSurfaceIndexSignature {
    param($Value, [string]$Label, [switch]$AllowEmpty)

    Assert-ReleaseSurfaceIndexExactProperties $Value @('length', 'sha256') $Label
    Assert-ReleaseSurfaceIndexIntegerProperties $Value @('length') $Label
    Assert-ReleaseSurfaceIndexStringProperties $Value @('sha256') $Label
    if ([long]$Value.length -lt [int][bool](-not $AllowEmpty) -or
        [string]$Value.sha256 -cnotmatch '^[0-9a-f]{64}$') {
        throw "$Label is invalid."
    }
}

function Assert-ReleaseSurfaceIndexProvenance {
    param($Value, [string]$ExpectedFileName, [string]$Label)

    Assert-ReleaseSurfaceIndexExactProperties $Value @(
        'fileName', 'fileVersion', 'productVersion', 'length', 'sha256') $Label
    Assert-ReleaseSurfaceIndexStringProperties $Value @(
        'fileName', 'fileVersion', 'productVersion', 'sha256') $Label
    Assert-ReleaseSurfaceIndexIntegerProperties $Value @('length') $Label
    if ([string]$Value.fileName -cne $ExpectedFileName -or
        [string]::IsNullOrWhiteSpace([string]$Value.fileVersion) -or
        [string]::IsNullOrWhiteSpace([string]$Value.productVersion) -or
        [long]$Value.length -lt 1 -or
        [string]$Value.sha256 -cnotmatch '^[0-9a-f]{64}$') {
        throw "$Label is invalid."
    }
}

function Assert-ReleaseSurfaceIndexValuesEqual {
    param($Expected, $Actual, [string[]]$Properties, [string]$Label)

    foreach ($property in $Properties) {
        if ([string]$Expected.$property -cne [string]$Actual.$property) {
            throw "$Label differs at $property."
        }
    }
}

function Assert-ReleaseSurfaceIndexCandidate {
    param($Candidate, [string]$Label)

    Assert-ReleaseSurfaceIndexExactProperties $Candidate @(
        'candidateId', 'candidateVersion', 'runtimeUseDisposition', 'gitHead',
        'embeddedBuildSha', 'embeddedBuildUtc', 'embeddedBuildLabel',
        'campaignSchema', 'runtimeSettingsSchema', 'addonId', 'addonGuid',
        'packageHashAlgorithm', 'packageSha256', 'manifestSha256', 'readySha256',
        'workbenchCrc', 'runtimeRole', 'diagnosticExecutable',
        'recordedDiagnosticExecutable', 'recordedRuntimeExecutable') $Label
    Assert-ReleaseSurfaceIndexStringProperties $Candidate @(
        'candidateId', 'candidateVersion', 'runtimeUseDisposition', 'gitHead',
        'embeddedBuildSha', 'embeddedBuildUtc', 'embeddedBuildLabel', 'addonId',
        'addonGuid', 'packageHashAlgorithm', 'packageSha256', 'manifestSha256',
        'readySha256', 'workbenchCrc', 'runtimeRole') $Label
    Assert-ReleaseSurfaceIndexIntegerProperties $Candidate @(
        'campaignSchema', 'runtimeSettingsSchema') $Label
    if ([string]$Candidate.candidateId -cnotmatch
            '^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$' -or
        [string]$Candidate.candidateVersion -cnotmatch
            '^[0-9A-Za-z][0-9A-Za-z._-]{0,95}$' -or
        [string]$Candidate.runtimeUseDisposition -cne
            'active-runtime-candidate' -or
        [string]$Candidate.gitHead -cnotmatch '^[0-9a-f]{40}$' -or
        [string]$Candidate.embeddedBuildSha -cnotmatch '^[0-9a-f]{40}$' -or
        [string]::IsNullOrWhiteSpace([string]$Candidate.embeddedBuildUtc) -or
        [string]::IsNullOrWhiteSpace([string]$Candidate.embeddedBuildLabel) -or
        [int]$Candidate.campaignSchema -lt 1 -or
        [int]$Candidate.runtimeSettingsSchema -lt 1 -or
        [string]$Candidate.addonId -cnotmatch '^[A-Za-z_][A-Za-z0-9_]*$' -or
        [string]$Candidate.addonGuid -cnotmatch '^[0-9A-F]{16}$' -or
        [string]$Candidate.packageHashAlgorithm -cne 'sha256-manifest-v1' -or
        [string]$Candidate.packageSha256 -cnotmatch '^[0-9a-f]{64}$' -or
        [string]$Candidate.manifestSha256 -cnotmatch '^[0-9a-f]{64}$' -or
        [string]$Candidate.readySha256 -cnotmatch '^[0-9a-f]{64}$' -or
        [string]$Candidate.workbenchCrc -cnotmatch '^[0-9a-f]{8}$' -or
        [string]$Candidate.runtimeRole -cne 'server') {
        throw "$Label has an invalid active candidate tuple."
    }
    Assert-ReleaseSurfaceIndexProvenance `
        $Candidate.diagnosticExecutable 'ArmaReforgerServerDiag.exe' `
        "$Label diagnostic executable"
    Assert-ReleaseSurfaceIndexProvenance `
        $Candidate.recordedDiagnosticExecutable 'ArmaReforgerServerDiag.exe' `
        "$Label recorded diagnostic executable"
    Assert-ReleaseSurfaceIndexProvenance `
        $Candidate.recordedRuntimeExecutable 'ArmaReforgerServer.exe' `
        "$Label recorded retail executable"
    Assert-ReleaseSurfaceIndexValuesEqual `
        $Candidate.diagnosticExecutable $Candidate.recordedDiagnosticExecutable `
        @('fileName', 'fileVersion', 'productVersion', 'length', 'sha256') `
        "$Label diagnostic executable"
}

function Assert-ReleaseSurfaceIndexCandidatesEqual {
    param($Expected, $Actual, [string]$Label)

    $properties = @(
        'candidateId', 'candidateVersion', 'runtimeUseDisposition', 'gitHead',
        'embeddedBuildSha', 'embeddedBuildUtc', 'embeddedBuildLabel',
        'campaignSchema', 'runtimeSettingsSchema', 'addonId', 'addonGuid',
        'packageHashAlgorithm', 'packageSha256', 'manifestSha256', 'readySha256',
        'workbenchCrc', 'runtimeRole')
    Assert-ReleaseSurfaceIndexValuesEqual $Expected $Actual $properties $Label
    foreach ($property in @(
            'diagnosticExecutable',
            'recordedDiagnosticExecutable',
            'recordedRuntimeExecutable')) {
        Assert-ReleaseSurfaceIndexValuesEqual `
            $Expected.$property $Actual.$property `
            @('fileName', 'fileVersion', 'productVersion', 'length', 'sha256') `
            "$Label $property"
    }
}

function Assert-ReleaseSurfaceIndexContract {
    param($Contract)

    Assert-ReleaseSurfaceIndexExactProperties $Contract @(
        'schemaVersion', 'evidenceKind', 'diagnosticSymbol', 'paritySource',
        'carrierFileNamePattern', 'forbiddenTypeNamePattern',
        'expectedCarrierFileCount', 'expectedForbiddenTypeCount',
        'expectedForbiddenCommandActionIdCount',
        'expectedForbiddenMemberSurfaceCount',
        'expectedForbiddenLiteralSurfaceCount',
        'expectedProductionObservabilityMemberSurfaceCount', 'carrierFiles',
        'mixedDiagnosticFiles', 'commandSurfaceFiles', 'forbiddenTypeNames',
        'forbiddenCommandActionIds', 'productionPositiveControlTypeNames',
        'productionPositiveControlCommandActionIds',
        'forbiddenMemberSurfaces', 'forbiddenLiteralSurfaces',
        'productionObservabilityMemberSurfaces',
        'productionObservabilityPolicy') 'Release-surface contract'
    Assert-ReleaseSurfaceIndexStringProperties $Contract @(
        'evidenceKind', 'diagnosticSymbol', 'paritySource',
        'carrierFileNamePattern', 'forbiddenTypeNamePattern') `
        'Release-surface contract'
    Assert-ReleaseSurfaceIndexIntegerProperties $Contract @(
        'schemaVersion', 'expectedCarrierFileCount', 'expectedForbiddenTypeCount',
        'expectedForbiddenCommandActionIdCount',
        'expectedForbiddenMemberSurfaceCount',
        'expectedForbiddenLiteralSurfaceCount',
        'expectedProductionObservabilityMemberSurfaceCount') `
        'Release-surface contract'
    if ([int]$Contract.schemaVersion -ne 1 -or
        [string]$Contract.evidenceKind -cne $script:ContractEvidenceKind -or
        [string]$Contract.diagnosticSymbol -cne 'ENABLE_DIAG' -or
        [int]$Contract.expectedCarrierFileCount -ne @($Contract.carrierFiles).Count -or
        [int]$Contract.expectedForbiddenTypeCount -ne
            @($Contract.forbiddenTypeNames).Count -or
        [int]$Contract.expectedForbiddenCommandActionIdCount -ne
            @($Contract.forbiddenCommandActionIds).Count -or
        [int]$Contract.expectedForbiddenMemberSurfaceCount -ne
            @($Contract.forbiddenMemberSurfaces).Count -or
        [int]$Contract.expectedForbiddenLiteralSurfaceCount -ne
            @($Contract.forbiddenLiteralSurfaces).Count -or
        [int]$Contract.expectedProductionObservabilityMemberSurfaceCount -ne
            @($Contract.productionObservabilityMemberSurfaces).Count) {
        throw 'Release-surface contract identity or declared counts are invalid.'
    }
    foreach ($set in @(
            [pscustomobject]@{
                Label = 'carrier path'
                Rows = @($Contract.carrierFiles)
                Pattern = '^Scripts/Game/HST/[A-Za-z0-9_./-]+\.c$'
            },
            [pscustomobject]@{
                Label = 'mixed diagnostic path'
                Rows = @($Contract.mixedDiagnosticFiles)
                Pattern = '^Scripts/Game/HST/[A-Za-z0-9_./-]+\.c$'
            },
            [pscustomobject]@{
                Label = 'command surface path'
                Rows = @($Contract.commandSurfaceFiles)
                Pattern = '^Scripts/Game/HST/[A-Za-z0-9_./-]+\.c$'
            },
            [pscustomobject]@{
                Label = 'forbidden type'
                Rows = @($Contract.forbiddenTypeNames)
                Pattern = '^[A-Za-z_][A-Za-z0-9_]*$'
            },
            [pscustomobject]@{
                Label = 'production type'
                Rows = @($Contract.productionPositiveControlTypeNames)
                Pattern = '^[A-Za-z_][A-Za-z0-9_]*$'
            },
            [pscustomobject]@{
                Label = 'forbidden command'
                Rows = @($Contract.forbiddenCommandActionIds)
                Pattern = '^[a-z][a-z0-9_]*$'
            },
            [pscustomobject]@{
                Label = 'production command'
                Rows = @($Contract.productionPositiveControlCommandActionIds)
                Pattern = '^[a-z][a-z0-9_]*$'
            },
            [pscustomobject]@{
                Label = 'forbidden member surface'
                Rows = @($Contract.forbiddenMemberSurfaces)
                Pattern = '^Scripts/Game/HST/[A-Za-z0-9_./-]+\.c::[^\r\n]+$'
            },
            [pscustomobject]@{
                Label = 'forbidden literal surface'
                Rows = @($Contract.forbiddenLiteralSurfaces)
                Pattern = '^Scripts/Game/HST/[A-Za-z0-9_./-]+\.c::[^\r\n]+$'
            },
            [pscustomobject]@{
                Label = 'production observability member surface'
                Rows = @($Contract.productionObservabilityMemberSurfaces)
                Pattern = '^Scripts/Game/HST/[A-Za-z0-9_./-]+\.c::[^\r\n]+$'
            })) {
        Assert-ReleaseSurfaceIndexStringArray $set.Rows `
            ("Release-surface contract $($set.Label) set")
        if ($set.Rows.Count -eq 0 -or
            @($set.Rows | Sort-Object -Unique -CaseSensitive).Count -ne
                $set.Rows.Count) {
            throw "Release-surface contract $($set.Label) set is empty or duplicated."
        }
        foreach ($row in $set.Rows) {
            if ([string]$row -cnotmatch [string]$set.Pattern) {
                throw "Release-surface contract contains an invalid $($set.Label)."
            }
        }
    }
}

function Get-ReleaseSurfaceIndexMemberProbePlan {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)]$Contract,
        [string]$SourceCommit = ''
    )

    if (-not [string]::IsNullOrWhiteSpace($SourceCommit) -and
        $SourceCommit -cnotmatch '^[0-9a-f]{40}$') {
        throw 'Release-surface member probes have an invalid source commit.'
    }
    $requests = New-Object Collections.Generic.List[object]
    foreach ($set in @(
            [pscustomobject]@{
                category = 'forbidden'
                surfaces = @($Contract.forbiddenMemberSurfaces)
            },
            [pscustomobject]@{
                category = 'production'
                surfaces = @($Contract.productionObservabilityMemberSurfaces)
            })) {
        foreach ($rawSurface in $set.surfaces) {
            $surface = [string]$rawSurface
            if ($surface -cnotmatch
                '^(?<path>Scripts/Game/HST/[A-Za-z0-9_./-]+\.c)::' +
                '(?<member>[A-Za-z_][A-Za-z0-9_]*)$') {
                throw 'Release-surface member-probe contract entry is unsafe.'
            }
            [void]$requests.Add([pscustomobject][ordered]@{
                category = [string]$set.category
                surface = $surface
                path = [string]$Matches.path
                memberName = [string]$Matches.member
            })
        }
    }
    if (@($requests.surface | Sort-Object -Unique -CaseSensitive).Count -ne
        $requests.Count) {
        throw 'Release-surface member-probe contract entries are duplicated.'
    }

    $methodPattern = [regex]::new(
        '^\s*(?!return\b)' +
        '(?:(?:protected|private|static|override|event|proto|sealed|ref|autoptr)\s+)*' +
        '(?:[A-Za-z_][A-Za-z0-9_<>\[\],.]*\s+)+' +
        '(?<member>[A-Za-z_][A-Za-z0-9_]*)\s*\(',
        [Text.RegularExpressions.RegexOptions]::CultureInvariant)
    $fieldPattern = [regex]::new(
        '^\s*(?!return\b)' +
        '(?:(?:protected|private|static|const|ref|autoptr)\s+)*' +
        '(?:[A-Za-z_][A-Za-z0-9_<>\[\],.]*\s+)+' +
        '(?<member>[A-Za-z_][A-Za-z0-9_]*)\s*(?:[;=])',
        [Text.RegularExpressions.RegexOptions]::CultureInvariant)
    $classPattern = [regex]::new(
        '^\s*(?:(?:modded|sealed)\s+)*class\s+' +
        '(?<type>[A-Za-z_][A-Za-z0-9_]*)',
        [Text.RegularExpressions.RegexOptions]::CultureInvariant)
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
    foreach ($pathGroup in @($requests.ToArray() | Group-Object path)) {
        $portablePath = [string]$pathGroup.Name
        $sourceLines = @()
        if ([string]::IsNullOrWhiteSpace($SourceCommit)) {
            $sourcePath = [IO.Path]::GetFullPath((Join-Path `
                $RepositoryRoot $portablePath.Replace('/', '\')))
            $rootPrefix = [IO.Path]::GetFullPath($RepositoryRoot).
                TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
            if (-not $sourcePath.StartsWith(
                    $rootPrefix,
                    [StringComparison]::OrdinalIgnoreCase) -or
                -not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
                throw 'Release-surface synthetic source path escaped the repository.'
            }
            $sourceLines = @([IO.File]::ReadAllLines($sourcePath))
        }
        else {
            $blobSpec = $SourceCommit + ':' + $portablePath
            $sourceLines = @(& git -C $RepositoryRoot show --no-textconv `
                $blobSpec 2>$null)
            if ($LASTEXITCODE -ne 0 -or $sourceLines.Count -eq 0) {
                throw "Release-surface candidate source is absent: $portablePath."
            }
        }
        $pending = @($pathGroup.Group | Where-Object {
            [string]$_.memberName -cne 'forceDebug'
        })
        $currentType = ''
        foreach ($line in $sourceLines) {
            $classMatch = $classPattern.Match($line)
            if ($classMatch.Success) {
                $currentType = [string]$classMatch.Groups['type'].Value
            }
            if ([string]::IsNullOrWhiteSpace($currentType)) {
                continue
            }
            $declaration = $methodPattern.Match($line)
            $probeKind = 'method'
            if (-not $declaration.Success) {
                $declaration = $fieldPattern.Match($line)
                if ($declaration.Success) {
                    if ($line -cmatch '(^|\s)const\s') {
                        $probeKind = 'constant'
                    }
                    else {
                        $probeKind = 'field'
                    }
                }
            }
            if (-not $declaration.Success) {
                continue
            }
            $memberName = [string]$declaration.Groups['member'].Value
            foreach ($request in @($pending | Where-Object {
                [string]$_.memberName -ceq $memberName
            })) {
                [void]$resolved.Add([pscustomobject][ordered]@{
                    category = [string]$request.category
                    surface = [string]$request.surface
                    declaringType = $currentType
                    memberName = $memberName
                    probeKind = $probeKind
                })
            }
        }

        $guardedRequests = @($pathGroup.Group | Where-Object {
            [string]$_.memberName -ceq 'forceDebug'
        })
        if ($guardedRequests.Count -gt 0) {
            $signatureSurface =
                'Scripts/Game/HST/Components/' +
                'HST_CampaignCoordinatorComponent.c::forceDebug'
            if ($guardedRequests.Count -ne 1 -or
                [string]$guardedRequests[0].category -cne 'forbidden' -or
                [string]$guardedRequests[0].surface -cne $signatureSurface -or
                $portablePath -cne
                    'Scripts/Game/HST/Components/' +
                    'HST_CampaignCoordinatorComponent.c') {
                throw 'The guarded member-signature request is invalid.'
            }
            $sourceText = $sourceLines -join "`n"
            $signatureMatches = @($guardedSignaturePattern.Matches($sourceText))
            if ($signatureMatches.Count -ne 1) {
                throw 'The guarded member signature is absent or ambiguous in candidate source.'
            }
            $signatureMatch = $signatureMatches[0]
            $signatureLine = @($sourceText.Substring(
                0,
                $signatureMatch.Index).Split("`n")).Count
            $signatureOwner = ''
            for ($sourceIndex = 0;
                $sourceIndex -lt $signatureLine;
                $sourceIndex++) {
                $ownerMatch = $classPattern.Match($sourceLines[$sourceIndex])
                if ($ownerMatch.Success) {
                    $signatureOwner =
                        [string]$ownerMatch.Groups['type'].Value
                }
            }
            if ($signatureOwner -cne 'HST_CampaignCoordinatorComponent' -or
                [string]$guardedRequests[0].memberName -cne
                    [string]$signatureMatch.Groups['parameterName'].Value) {
                throw 'The guarded member signature owner or parameter is invalid.'
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

    $finalRows = New-Object Collections.Generic.List[object]
    foreach ($request in $requests.ToArray()) {
        $matches = @($resolved.ToArray() | Where-Object {
            [string]$_.category -ceq [string]$request.category -and
            [string]$_.surface -ceq [string]$request.surface
        })
        if ($matches.Count -ne 1) {
            $ambiguousDebugOwner =
                'Scripts/Game/HST/UI/HST_ActionDialogController.c::m_sDebugOwner'
            if ([string]$request.surface -ceq $ambiguousDebugOwner -and
                $matches.Count -eq 2 -and
                [string]$matches[0].declaringType -ceq 'HST_ActionDialogData' -and
                [string]$matches[1].declaringType -ceq
                    'HST_ActionChoiceDialogData') {
                $matches = @($matches[0])
            }
        }
        if ($matches.Count -ne 1) {
            throw "Release-surface member probe $($request.surface) resolved ambiguously."
        }
        [void]$finalRows.Add($matches[0])
    }
    $forbidden = @($finalRows.ToArray() | Where-Object {
        [string]$_.category -ceq 'forbidden'
    })
    $production = @($finalRows.ToArray() | Where-Object {
        [string]$_.category -ceq 'production'
    })
    if ($forbidden.Count -ne @($Contract.forbiddenMemberSurfaces).Count -or
        $production.Count -ne
            @($Contract.productionObservabilityMemberSurfaces).Count) {
        throw 'Release-surface member-probe plan is incomplete.'
    }
    return [pscustomobject][ordered]@{
        forbidden = [object[]]$forbidden
        production = [object[]]$production
    }
}

function Assert-ReleaseSurfaceIndexPackageAndManifest {
    param(
        $Candidate,
        $Bindings,
        $Manifest,
        $Ready,
        $ManifestArtifact,
        $ReadyArtifact
    )

    Assert-ReleaseSurfaceIndexIntegerProperties $Manifest @(
        'manifestSchemaVersion') 'Copied candidate manifest'
    Assert-ReleaseSurfaceIndexStringProperties $Manifest.candidate @(
        'id', 'version', 'state') 'Copied candidate manifest candidate'
    Assert-ReleaseSurfaceIndexStringProperties $Manifest.source @(
        'gitHead') 'Copied candidate manifest source'
    Assert-ReleaseSurfaceIndexStringProperties $Manifest.source.embeddedImplementation @(
        'sha', 'utc', 'label') 'Copied candidate embedded implementation'
    Assert-ReleaseSurfaceIndexIntegerProperties $Manifest.source @(
        'campaignSchema', 'runtimeSettingsSchema') 'Copied candidate manifest source'
    Assert-ReleaseSurfaceIndexStringProperties $Manifest.addon @(
        'id', 'guid') 'Copied candidate manifest addon'
    Assert-ReleaseSurfaceIndexStringProperties $Manifest.workbench @(
        'crc') 'Copied candidate Workbench identity'
    Assert-ReleaseSurfaceIndexStringProperties $Manifest.package @(
        'hashAlgorithm', 'sha256') 'Copied candidate package'
    Assert-ReleaseSurfaceIndexIntegerProperties $Ready @(
        'schemaVersion') 'Copied candidate ready seal'
    Assert-ReleaseSurfaceIndexStringProperties $Ready @(
        'candidateId', 'gitHead', 'packageSha256', 'manifestSha256') `
        'Copied candidate ready seal'
    if ($ManifestArtifact.Sha256 -cne [string]$Candidate.manifestSha256 -or
        $ReadyArtifact.Sha256 -cne [string]$Candidate.readySha256 -or
        [int]$Manifest.manifestSchemaVersion -ne 1 -or
        [string]$Manifest.candidate.id -cne [string]$Candidate.candidateId -or
        [string]$Manifest.candidate.version -cne [string]$Candidate.candidateVersion -or
        [string]$Manifest.candidate.state -cne 'retained-uncertified' -or
        [string]$Manifest.source.gitHead -cne [string]$Candidate.gitHead -or
        [string]$Manifest.source.embeddedImplementation.sha -cne
            [string]$Candidate.embeddedBuildSha -or
        [string]$Manifest.source.embeddedImplementation.utc -cne
            [string]$Candidate.embeddedBuildUtc -or
        [string]$Manifest.source.embeddedImplementation.label -cne
            [string]$Candidate.embeddedBuildLabel -or
        [int]$Manifest.source.campaignSchema -ne [int]$Candidate.campaignSchema -or
        [int]$Manifest.source.runtimeSettingsSchema -ne
            [int]$Candidate.runtimeSettingsSchema -or
        [string]$Manifest.addon.id -cne [string]$Candidate.addonId -or
        [string]$Manifest.addon.guid -cne [string]$Candidate.addonGuid -or
        [string]$Manifest.workbench.crc -cne [string]$Candidate.workbenchCrc -or
        [string]$Manifest.package.hashAlgorithm -cne
            [string]$Candidate.packageHashAlgorithm -or
        [string]$Manifest.package.sha256 -cne [string]$Candidate.packageSha256 -or
        [int]$Ready.schemaVersion -ne 1 -or
        [string]$Ready.candidateId -cne [string]$Candidate.candidateId -or
        [string]$Ready.gitHead -cne [string]$Candidate.gitHead -or
        [string]$Ready.packageSha256 -cne [string]$Candidate.packageSha256 -or
        [string]$Ready.manifestSha256 -cne [string]$Candidate.manifestSha256) {
        throw 'Copied candidate manifest or ready seal is not bound to the run tuple.'
    }

    Assert-ReleaseSurfaceIndexExactProperties $Bindings.package @(
        'hashAlgorithm', 'sha256', 'files') 'Candidate package binding'
    Assert-ReleaseSurfaceIndexStringProperties $Bindings.package @(
        'hashAlgorithm', 'sha256') 'Candidate package binding'
    if ([string]$Bindings.package.hashAlgorithm -cne
            [string]$Candidate.packageHashAlgorithm -or
        [string]$Bindings.package.sha256 -cne [string]$Candidate.packageSha256) {
        throw 'Candidate package binding differs from the run tuple.'
    }
    $bindingRows = @($Bindings.package.files)
    $manifestRows = @($Manifest.package.files)
    if ($bindingRows.Count -ne $script:RequiredPackageFiles.Count -or
        $manifestRows.Count -ne $bindingRows.Count) {
        throw 'Candidate package does not contain the exact four-file inventory.'
    }
    $seen = New-Object Collections.Generic.HashSet[string]([StringComparer]::Ordinal)
    $canonical = New-Object Collections.Generic.List[object]
    foreach ($row in $bindingRows) {
        Assert-ReleaseSurfaceIndexExactProperties $row @(
            'path', 'indexPath', 'length', 'sha256') 'Candidate package file row'
        Assert-ReleaseSurfaceIndexStringProperties $row @(
            'path', 'indexPath', 'sha256') 'Candidate package file row'
        Assert-ReleaseSurfaceIndexIntegerProperties $row @(
            'length') 'Candidate package file row'
        Assert-ReleaseSurfaceIndexPortableRelativePath ([string]$row.path) `
            'Candidate package path'
        Assert-ReleaseSurfaceIndexPortableRelativePath ([string]$row.indexPath) `
            'Candidate package index path'
        if (-not $seen.Add([string]$row.indexPath) -or
            [string]$row.indexPath -cnotin $script:RequiredPackageFiles -or
            [string]$row.path -cne ('package/' + [string]$row.indexPath) -or
            [long]$row.length -lt 1 -or
            [string]$row.sha256 -cnotmatch '^[0-9a-f]{64}$') {
            throw 'Candidate package file row is invalid or duplicated.'
        }
        [void]$canonical.Add([pscustomobject][ordered]@{
            path = [string]$row.indexPath
            length = [long]$row.length
            sha256 = [string]$row.sha256
        })
    }
    if (@(Compare-Object `
            -ReferenceObject @($script:RequiredPackageFiles | Sort-Object) `
            -DifferenceObject @($seen | Sort-Object) `
            -CaseSensitive).Count -ne 0 -or
        (Get-ReleaseSurfaceIndexCanonicalRowsDigest $canonical.ToArray()) -cne
            [string]$Candidate.packageSha256) {
        throw 'Candidate canonical package digest is invalid.'
    }
    for ($index = 0; $index -lt $bindingRows.Count; $index++) {
        Assert-ReleaseSurfaceIndexExactProperties $manifestRows[$index] @(
            'path', 'indexPath', 'length', 'sha256') `
            'Copied manifest package file row'
        Assert-ReleaseSurfaceIndexStringProperties $manifestRows[$index] @(
            'path', 'indexPath', 'sha256') 'Copied manifest package file row'
        Assert-ReleaseSurfaceIndexIntegerProperties $manifestRows[$index] @(
            'length') 'Copied manifest package file row'
        foreach ($property in @('path', 'indexPath', 'length', 'sha256')) {
            if ([string]$bindingRows[$index].$property -cne
                [string]$manifestRows[$index].$property) {
                throw 'Copied manifest package rows differ from the binding.'
            }
        }
    }
}

function Get-ReleaseSurfaceIndexGitBlobSignature {
    param([string]$RepositoryRoot, [string]$Commit, [string]$Path)

    Assert-ReleaseSurfaceIndexPortableRelativePath $Path 'Git tool blob path'
    $blobOutput = @(& git -C $RepositoryRoot rev-parse `
        ($Commit + ':' + $Path) 2>$null)
    if ($LASTEXITCODE -ne 0) { throw 'A release-surface tool Git blob is missing.' }
    $blobId = ($blobOutput -join '').Trim()
    if ($blobId -cnotmatch '^[0-9a-f]{40,64}$') {
        throw 'A release-surface tool Git blob identity is invalid.'
    }
    $gitCommand = Get-Command git -CommandType Application -ErrorAction Stop |
        Select-Object -First 1
    $startInfo = New-Object Diagnostics.ProcessStartInfo
    $startInfo.FileName = $gitCommand.Source
    $startInfo.Arguments = 'cat-file blob ' + $blobId
    $startInfo.WorkingDirectory = $RepositoryRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $process = New-Object Diagnostics.Process
    $memory = New-Object IO.MemoryStream
    try {
        $process.StartInfo = $startInfo
        if (-not $process.Start()) { throw 'Git blob capture did not start.' }
        $process.StandardOutput.BaseStream.CopyTo($memory)
        $errorText = $process.StandardError.ReadToEnd()
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) {
            throw ('Git blob capture failed: ' + $errorText.Trim())
        }
        $bytes = $memory.ToArray()
        return [pscustomobject][ordered]@{
            length = [long]$bytes.Length
            sha256 = Get-ReleaseSurfaceIndexSha256Bytes $bytes
        }
    }
    finally {
        $memory.Dispose()
        $process.Dispose()
    }
}

function Assert-ReleaseSurfaceIndexHarnessAndTools {
    param(
        [string]$RunRoot,
        [string]$RepositoryRoot,
        $Run,
        $Bindings,
        [bool]$Synthetic,
        [bool]$PublishedVerification
    )

    Assert-ReleaseSurfaceIndexExactProperties $Bindings.source @(
        'harnessGitHead', 'checkoutClean', 'candidateAncestor', 'executionMode') `
        'Release-surface source binding'
    Assert-ReleaseSurfaceIndexStringProperties $Bindings.source @(
        'harnessGitHead', 'executionMode') 'Release-surface source binding'
    Assert-ReleaseSurfaceIndexBooleanProperties $Bindings.source @(
        'checkoutClean', 'candidateAncestor') 'Release-surface source binding'
    $expectedMode = if ($Synthetic) { 'synthetic-self-test' }
        else { 'paired-native-engine-audit' }
    if ([string]$Bindings.source.harnessGitHead -cne
            [string]$Run.source.harnessGitHead -or
        -not [bool]$Bindings.source.checkoutClean -or
        -not [bool]$Bindings.source.candidateAncestor -or
        [string]$Bindings.source.executionMode -cne $expectedMode) {
        throw 'Release-surface source binding is invalid.'
    }

    Assert-ReleaseSurfaceIndexExactProperties $Bindings.harness @(
        'id', 'guid', 'candidateDependencyGuid', 'constructionMode',
        'aggregateSha256', 'files') 'Release-surface harness binding'
    Assert-ReleaseSurfaceIndexStringProperties $Bindings.harness @(
        'id', 'guid', 'candidateDependencyGuid', 'constructionMode',
        'aggregateSha256') 'Release-surface harness binding'
    if ([string]$Bindings.harness.id -cne 'PartisanReleaseSurfaceAudit' -or
        [string]$Bindings.harness.guid -cne [string]$Run.harnessGuid -or
        [string]$Bindings.harness.guid -cnotmatch '^[0-9A-F]{16}$' -or
        [string]$Bindings.harness.candidateDependencyGuid -cne
            [string]$Run.candidate.addonGuid -or
        [string]$Bindings.harness.constructionMode -cne
            'external-disposable-runtime-loaded-audit-addon' -or
        [string]$Bindings.harness.aggregateSha256 -cne
            [string]$Run.harnessSha256) {
        throw 'Release-surface harness identity is invalid.'
    }
    $harnessRows = @($Bindings.harness.files)
    if ($harnessRows.Count -ne $script:RequiredHarnessFiles.Count) {
        throw 'Release-surface harness inventory is incomplete.'
    }
    $actualHarnessRows = New-Object Collections.Generic.List[object]
    foreach ($row in $harnessRows) {
        Assert-ReleaseSurfaceIndexExactProperties $row @(
            'path', 'length', 'sha256') 'Release-surface harness file row'
        Assert-ReleaseSurfaceIndexStringProperties $row @(
            'path', 'sha256') 'Release-surface harness file row'
        Assert-ReleaseSurfaceIndexIntegerProperties $row @(
            'length') 'Release-surface harness file row'
        if ([string]$row.path -cnotin $script:RequiredHarnessFiles -or
            [long]$row.length -lt 1 -or
            [string]$row.sha256 -cnotmatch '^[0-9a-f]{64}$') {
            throw 'Release-surface harness file row is invalid.'
        }
        $path = Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
            ('harness/source/' + [string]$row.path) 'Harness evidence file'
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw 'Release-surface harness evidence file is missing.'
        }
        $signature = Get-ReleaseSurfaceIndexFileSignature $path
        if (-not (Test-ReleaseSurfaceIndexSignatureExact $row $signature)) {
            throw 'Release-surface harness evidence file changed.'
        }
        [void]$actualHarnessRows.Add([pscustomobject][ordered]@{
            path = [string]$row.path
            length = [long]$row.length
            sha256 = [string]$row.sha256
        })
    }
    if (@(Compare-Object `
            -ReferenceObject @($script:RequiredHarnessFiles | Sort-Object) `
            -DifferenceObject @($harnessRows.path | Sort-Object) `
            -CaseSensitive).Count -ne 0 -or
        (Get-ReleaseSurfaceIndexCanonicalRowsDigest $actualHarnessRows.ToArray()) `
            -cne [string]$Bindings.harness.aggregateSha256) {
        throw 'Release-surface harness inventory or aggregate digest is invalid.'
    }
    $owner = (Read-ReleaseSurfaceIndexJsonArtifact `
        (Join-Path $RunRoot 'harness\source\.partisan-release-surface-audit-owner.json') `
        'Release-surface harness owner' -Portable).Value
    Assert-ReleaseSurfaceIndexExactProperties $owner @(
        'schemaVersion', 'evidenceKind', 'runNonce', 'harnessId', 'harnessGuid',
        'candidateGuid') 'Release-surface harness owner'
    Assert-ReleaseSurfaceIndexIntegerProperties $owner @(
        'schemaVersion') 'Release-surface harness owner'
    Assert-ReleaseSurfaceIndexStringProperties $owner @(
        'evidenceKind', 'runNonce', 'harnessId', 'harnessGuid', 'candidateGuid') `
        'Release-surface harness owner'
    if ([int]$owner.schemaVersion -ne 1 -or
        [string]$owner.evidenceKind -cne
            'partisan_release_surface_harness_owner_v1' -or
        [string]$owner.runNonce -cne [string]$Run.runNonce -or
        [string]$owner.harnessId -cne 'PartisanReleaseSurfaceAudit' -or
        [string]$owner.harnessGuid -cne [string]$Run.harnessGuid -or
        [string]$owner.candidateGuid -cne [string]$Run.candidate.addonGuid) {
        throw 'Release-surface harness owner is not run-bound.'
    }

    $tools = @($Bindings.tools)
    if ($tools.Count -ne $script:HarnessToolPaths.Count) {
        throw 'Release-surface harness tool set is incomplete.'
    }
    $expectedToolPaths = New-Object `
        'Collections.Generic.Dictionary[string,string]' `
        ([StringComparer]::Ordinal)
    foreach ($expectedRole in $script:HarnessToolPaths.Keys) {
        $expectedToolPaths.Add(
            [string]$expectedRole,
            [string]$script:HarnessToolPaths[$expectedRole])
    }
    $seenRoles = New-Object Collections.Generic.HashSet[string]([StringComparer]::Ordinal)
    foreach ($tool in $tools) {
        Assert-ReleaseSurfaceIndexExactProperties $tool @(
            'role', 'path', 'length', 'sha256') 'Release-surface harness tool row'
        Assert-ReleaseSurfaceIndexStringProperties $tool @(
            'role', 'path', 'sha256') 'Release-surface harness tool row'
        Assert-ReleaseSurfaceIndexIntegerProperties $tool @(
            'length') 'Release-surface harness tool row'
        $role = [string]$tool.role
        if (-not $expectedToolPaths.ContainsKey($role) -or
            -not $seenRoles.Add($role) -or
            [string]$tool.path -cne [string]$expectedToolPaths[$role] -or
            [long]$tool.length -lt 1 -or
            [string]$tool.sha256 -cnotmatch '^[0-9a-f]{64}$') {
            throw 'Release-surface harness tool roles are invalid or duplicated.'
        }
        $worktreePath = Resolve-ReleaseSurfaceIndexPortableFile `
            $RepositoryRoot ([string]$tool.path) 'Harness tool path'
        if (-not (Test-Path -LiteralPath $worktreePath -PathType Leaf) -or
            -not (Test-ReleaseSurfaceIndexSignatureExact `
                $tool (Get-ReleaseSurfaceIndexFileSignature $worktreePath))) {
            throw 'A release-surface tool differs from its worktree binding.'
        }
    }
    foreach ($expectedRole in $script:HarnessToolPaths.Keys) {
        if (-not $seenRoles.Contains([string]$expectedRole)) {
            throw 'Release-surface harness tool set is incomplete.'
        }
    }

    if (-not $Synthetic) {
        foreach ($head in @(
                [string]$Run.candidate.gitHead,
                [string]$Run.source.harnessGitHead)) {
            if ($head -cnotmatch '^[0-9a-f]{40}$') {
                throw 'A release-surface Git commit identity is invalid.'
            }
            & git -C $RepositoryRoot cat-file -e ($head + '^{commit}') 2>$null
            if ($LASTEXITCODE -ne 0) {
                throw 'A release-surface Git commit is unknown.'
            }
        }
        & git -C $RepositoryRoot merge-base --is-ancestor `
            ([string]$Run.candidate.gitHead) `
            ([string]$Run.source.harnessGitHead) 2>$null
        if ($LASTEXITCODE -ne 0) {
            throw 'Harness source HEAD is not a verified candidate descendant.'
        }
        $currentHead = (@(& git -C $RepositoryRoot rev-parse HEAD 2>$null) -join '').Trim()
        if ($LASTEXITCODE -ne 0 -or
            $currentHead -cnotmatch '^[0-9a-f]{40}$') {
            throw 'Current checkout Git HEAD is invalid.'
        }
        if ($PublishedVerification) {
            & git -C $RepositoryRoot merge-base --is-ancestor `
                ([string]$Run.source.harnessGitHead) $currentHead 2>$null
            if ($LASTEXITCODE -ne 0) {
                throw 'Current checkout is not a verified harness descendant.'
            }
        }
        else {
            if ($currentHead -cne [string]$Run.source.harnessGitHead) {
                throw 'Current checkout differs from the release-surface harness HEAD.'
            }
            $dirty = @(& git -C $RepositoryRoot status --porcelain --untracked-files=all)
            if ($LASTEXITCODE -ne 0 -or $dirty.Count -ne 0) {
                throw 'Release-surface harness checkout is not clean.'
            }
        }
        foreach ($tool in $tools) {
            $blob = Get-ReleaseSurfaceIndexGitBlobSignature `
                $RepositoryRoot ([string]$Run.source.harnessGitHead) `
                ([string]$tool.path)
            if (-not (Test-ReleaseSurfaceIndexSignatureExact $tool $blob)) {
                throw 'A release-surface tool differs from its Git blob.'
            }
        }
    }
}

function Assert-ReleaseSurfaceIndexProbeRows {
    param(
        [object[]]$Rows,
        [string]$Key,
        [string[]]$ExpectedNames,
        [string[]]$PresenceProperties,
        [bool]$ExpectedPresence,
        [string]$Label
    )

    $names = @($Rows | ForEach-Object { [string]$_.$Key })
    if ($names.Count -ne $ExpectedNames.Count -or
        @($names | Sort-Object -Unique -CaseSensitive).Count -ne $names.Count -or
        @(Compare-Object `
            -ReferenceObject @($ExpectedNames | Sort-Object) `
            -DifferenceObject @($names | Sort-Object) `
            -CaseSensitive).Count -ne 0) {
        throw "$Label names do not match the release-surface contract."
    }
    foreach ($row in $Rows) {
        Assert-ReleaseSurfaceIndexExactProperties $row `
            (@($Key) + $PresenceProperties) "$Label row"
        Assert-ReleaseSurfaceIndexStringProperties $row @($Key) "$Label row"
        Assert-ReleaseSurfaceIndexBooleanProperties $row $PresenceProperties `
            "$Label row"
        foreach ($property in $PresenceProperties) {
            if ([bool]$row.$property -ne $ExpectedPresence) {
                throw "$Label has an invalid $property value."
            }
        }
    }
}

function Assert-ReleaseSurfaceIndexMemberProbeRows {
    param(
        [object[]]$Rows,
        [object[]]$ExpectedRows,
        [bool]$ExpectedPresence,
        [string]$Label
    )

    if ($Rows.Count -ne $ExpectedRows.Count) {
        throw "$Label count differs from the candidate-bound probe plan."
    }
    for ($index = 0; $index -lt $ExpectedRows.Count; $index++) {
        $row = $Rows[$index]
        $expected = $ExpectedRows[$index]
        Assert-ReleaseSurfaceIndexExactProperties $row @(
            'surface', 'declaringType', 'memberName', 'probeKind',
            'probeSupported', 'present') "$Label row"
        Assert-ReleaseSurfaceIndexStringProperties $row @(
            'surface', 'declaringType', 'memberName', 'probeKind') "$Label row"
        Assert-ReleaseSurfaceIndexBooleanProperties $row @(
            'probeSupported', 'present') "$Label row"
        foreach ($property in @(
                'surface', 'declaringType', 'memberName', 'probeKind')) {
            if ([string]$row.$property -cne [string]$expected.$property) {
                throw "$Label identity differs at $property."
            }
        }
        if (-not [bool]$row.probeSupported -or
            [bool]$row.present -ne $ExpectedPresence) {
            throw "$Label has an unsupported or unexpected runtime result."
        }
    }
}

function Assert-ReleaseSurfaceIndexProbe {
    param($Probe, [string]$Mode, $Run, $Contract, $MemberProbePlan)

    Assert-ReleaseSurfaceIndexExactProperties $Probe @(
        'schemaVersion', 'evidenceKind', 'runNonce', 'mode', 'expectedMode',
        'candidateId', 'packageSha256', 'manifestSha256', 'cliIdentityPresent',
        'modeSentinelExact', 'retailSentinelPresent',
        'diagnosticSentinelPresent', 'forbiddenTypeExpectedPresent',
        'runtimeCompilerAlwaysPublicCompiled',
        'runtimeCompilerAlwaysProtectedCompiled',
        'runtimeCompilerImpossibleMemberRejected',
        'runtimeCompilerDiagnosticPublicCompiled',
        'runtimeCompilerDiagnosticProtectedCompiled',
        'runtimeMetadataAlwaysFieldPresent',
        'runtimeMetadataDiagnosticFieldPresent',
        'runtimeCompilerAvailable', 'runtimeMetadataAvailable',
        'forbiddenTypes', 'productionTypes',
        'forbiddenMemberExpectedPresent', 'forbiddenMembers',
        'productionMembers',
        'forbiddenCommandExpectedPresent', 'forbiddenCommands',
        'productionCommands', 'mismatchCount', 'passed') "$Mode probe"
    Assert-ReleaseSurfaceIndexIntegerProperties $Probe @(
        'schemaVersion', 'mismatchCount') "$Mode probe"
    Assert-ReleaseSurfaceIndexStringProperties $Probe @(
        'evidenceKind', 'runNonce', 'mode', 'expectedMode', 'candidateId',
        'packageSha256', 'manifestSha256') "$Mode probe"
    Assert-ReleaseSurfaceIndexBooleanProperties $Probe @(
        'cliIdentityPresent', 'modeSentinelExact', 'retailSentinelPresent',
        'diagnosticSentinelPresent', 'forbiddenTypeExpectedPresent',
        'runtimeCompilerAlwaysPublicCompiled',
        'runtimeCompilerAlwaysProtectedCompiled',
        'runtimeCompilerImpossibleMemberRejected',
        'runtimeCompilerDiagnosticPublicCompiled',
        'runtimeCompilerDiagnosticProtectedCompiled',
        'runtimeMetadataAlwaysFieldPresent',
        'runtimeMetadataDiagnosticFieldPresent',
        'runtimeCompilerAvailable', 'runtimeMetadataAvailable',
        'forbiddenMemberExpectedPresent',
        'forbiddenCommandExpectedPresent', 'passed') "$Mode probe"
    $expectedForbidden = $Mode -ceq 'diagnostic'
    if ([int]$Probe.schemaVersion -ne 1 -or
        [string]$Probe.evidenceKind -cne $script:ProbeEvidenceKind -or
        [string]$Probe.runNonce -cne [string]$Run.runNonce -or
        [string]$Probe.mode -cne $Mode -or
        [string]$Probe.expectedMode -cne $Mode -or
        [string]$Probe.candidateId -cne [string]$Run.candidate.candidateId -or
        [string]$Probe.packageSha256 -cne
            [string]$Run.candidate.packageSha256 -or
        [string]$Probe.manifestSha256 -cne
            [string]$Run.candidate.manifestSha256 -or
        -not [bool]$Probe.cliIdentityPresent -or
        -not [bool]$Probe.modeSentinelExact -or
        [bool]$Probe.retailSentinelPresent -ne ($Mode -ceq 'retail') -or
        [bool]$Probe.diagnosticSentinelPresent -ne ($Mode -ceq 'diagnostic') -or
        [bool]$Probe.forbiddenTypeExpectedPresent -ne $expectedForbidden -or
        -not [bool]$Probe.runtimeCompilerAlwaysPublicCompiled -or
        -not [bool]$Probe.runtimeCompilerAlwaysProtectedCompiled -or
        -not [bool]$Probe.runtimeCompilerImpossibleMemberRejected -or
        [bool]$Probe.runtimeCompilerDiagnosticPublicCompiled -ne
            $expectedForbidden -or
        [bool]$Probe.runtimeCompilerDiagnosticProtectedCompiled -ne
            $expectedForbidden -or
        -not [bool]$Probe.runtimeMetadataAlwaysFieldPresent -or
        [bool]$Probe.runtimeMetadataDiagnosticFieldPresent -ne
            $expectedForbidden -or
        -not [bool]$Probe.runtimeCompilerAvailable -or
        -not [bool]$Probe.runtimeMetadataAvailable -or
        [bool]$Probe.forbiddenMemberExpectedPresent -ne $expectedForbidden -or
        [bool]$Probe.forbiddenCommandExpectedPresent -ne $expectedForbidden -or
        [int]$Probe.mismatchCount -ne 0 -or -not [bool]$Probe.passed) {
        throw "$Mode probe identity or fixed outcome is invalid."
    }
    Assert-ReleaseSurfaceIndexProbeRows @($Probe.forbiddenTypes) 'name' `
        ([string[]]$Contract.forbiddenTypeNames) @('present') `
        $expectedForbidden "$Mode forbidden type"
    Assert-ReleaseSurfaceIndexProbeRows @($Probe.productionTypes) 'name' `
        ([string[]]$Contract.productionPositiveControlTypeNames) @('present') `
        $true "$Mode production type"
    Assert-ReleaseSurfaceIndexMemberProbeRows `
        @($Probe.forbiddenMembers) @($MemberProbePlan.forbidden) `
        $expectedForbidden "$Mode forbidden member"
    Assert-ReleaseSurfaceIndexMemberProbeRows `
        @($Probe.productionMembers) @($MemberProbePlan.production) `
        $true "$Mode production member"
    Assert-ReleaseSurfaceIndexProbeRows @($Probe.forbiddenCommands) 'id' `
        ([string[]]$Contract.forbiddenCommandActionIds) `
        @('generatedPresent', 'routingPresent') $expectedForbidden `
        "$Mode forbidden command"
    Assert-ReleaseSurfaceIndexProbeRows @($Probe.productionCommands) 'id' `
        ([string[]]$Contract.productionPositiveControlCommandActionIds) `
        @('generatedPresent', 'routingPresent') $true `
        "$Mode production command"
}

function Assert-ReleaseSurfaceIndexArguments {
    param(
        [string]$Mode,
        [string]$RunRoot,
        $Run,
        $ModeValue,
        $RawArgumentsParsed,
        $PortableArgumentsParsed,
        [bool]$Synthetic
    )

    Assert-ReleaseSurfaceIndexExactProperties $RawArgumentsParsed.Value @(
        'schemaVersion', 'mode', 'executable', 'arguments') `
        "$Mode raw arguments"
    Assert-ReleaseSurfaceIndexExactProperties $PortableArgumentsParsed.Value @(
        'schemaVersion', 'mode', 'executable', 'arguments', 'rawArgumentsSha256') `
        "$Mode portable arguments"
    Assert-ReleaseSurfaceIndexIntegerProperties $RawArgumentsParsed.Value @(
        'schemaVersion') "$Mode raw arguments"
    Assert-ReleaseSurfaceIndexStringProperties $RawArgumentsParsed.Value @(
        'mode', 'executable') "$Mode raw arguments"
    Assert-ReleaseSurfaceIndexStringArray $RawArgumentsParsed.Value.arguments `
        "$Mode raw argument array"
    Assert-ReleaseSurfaceIndexIntegerProperties $PortableArgumentsParsed.Value @(
        'schemaVersion') "$Mode portable arguments"
    Assert-ReleaseSurfaceIndexStringProperties $PortableArgumentsParsed.Value @(
        'mode', 'executable', 'rawArgumentsSha256') "$Mode portable arguments"
    Assert-ReleaseSurfaceIndexStringArray $PortableArgumentsParsed.Value.arguments `
        "$Mode portable argument array"
    $expectedExecutable = if ($Synthetic) {
        [string]$ModeValue.executable.fileName
    }
    elseif ($Mode -ceq 'retail') {
        'ArmaReforgerServer.exe'
    }
    else { 'ArmaReforgerServerDiag.exe' }
    if ([int]$RawArgumentsParsed.Value.schemaVersion -ne 1 -or
        [string]$RawArgumentsParsed.Value.mode -cne $Mode -or
        [IO.Path]::GetFileName([string]$RawArgumentsParsed.Value.executable) -cne
            $expectedExecutable -or
        [int]$PortableArgumentsParsed.Value.schemaVersion -ne 1 -or
        [string]$PortableArgumentsParsed.Value.mode -cne $Mode -or
        [string]$PortableArgumentsParsed.Value.executable -cne
            ('<runtime>/' + $expectedExecutable) -or
        [string]$PortableArgumentsParsed.Value.rawArgumentsSha256 -cne
            [string]$RawArgumentsParsed.Artifact.Sha256) {
        throw "$Mode raw or portable argument identity is invalid."
    }
    $expected = [string[]]@(
        '-gproj', '<candidate-stage>/Partisan/addon.gproj',
        '-server', 'Worlds/HST_Dev/HST_Dev.ent',
        '-MissionHeader', 'Missions/HST_Dev.conf',
        '-addonsDir', '<runtime-addons>,<candidate-stage>',
        '-addons', ([string]$Run.candidate.addonGuid + ',' +
            [string]$Run.harnessGuid),
        '-profile', ('<run>/raw/' + $Mode + '/profile'),
        '-logsDir', ('<run>/raw/' + $Mode + '/logs'),
        '-addonTempDir', ('<run>/raw/' + $Mode + '/addon-temp'),
        '-logLevel', 'normal',
        '-logTime', 'datetime',
        '-noThrow',
        '-maxFPS', '30')
    if ($Mode -ceq 'diagnostic') {
        $expected += @('-scrDefine', 'ENABLE_DIAG')
    }
    $expected += @(
        '-releaseSurfaceRunNonce', [string]$Run.runNonce,
        '-releaseSurfaceExpectedMode', $Mode,
        '-hstReleaseCandidateId', [string]$Run.candidate.candidateId,
        '-hstReleasePackageSha256', [string]$Run.candidate.packageSha256,
        '-hstReleaseManifestSha256', [string]$Run.candidate.manifestSha256)
    $portable = [string[]]$PortableArgumentsParsed.Value.arguments
    if (-not (Test-ReleaseSurfaceIndexStringArrayExact $expected $portable)) {
        throw "$Mode portable arguments differ from the fixed launch contract."
    }
    $raw = [string[]]$RawArgumentsParsed.Value.arguments
    if ($raw.Count -ne $portable.Count) {
        throw "$Mode raw arguments have invalid cardinality."
    }
    $rawDefinePositions = @()
    $rawSymbolPositions = @()
    for ($index = 0; $index -lt $raw.Count; $index++) {
        if ([string]$raw[$index] -match '(?i)scrDefine') {
            $rawDefinePositions += $index
        }
        if ([string]$raw[$index] -match '(?i)^ENABLE_DIAG$') {
            $rawSymbolPositions += $index
        }
    }
    if ($Mode -ceq 'retail' -and
        ($rawDefinePositions.Count -ne 0 -or
            $rawSymbolPositions.Count -ne 0)) {
        throw 'Retail raw arguments contain script-symbol authority.'
    }
    if ($Mode -ceq 'diagnostic' -and
        ($rawDefinePositions.Count -ne 1 -or
            $rawSymbolPositions.Count -ne 1 -or
            [string]$raw[$rawDefinePositions[0]] -cne '-scrDefine' -or
            $rawDefinePositions[0] + 1 -ge $raw.Count -or
            $rawSymbolPositions[0] -ne $rawDefinePositions[0] + 1 -or
            [string]$raw[$rawSymbolPositions[0]] -cne 'ENABLE_DIAG')) {
        throw ('Diagnostic raw arguments must contain exactly one ' +
            "'-scrDefine', 'ENABLE_DIAG' pair.")
    }
    foreach ($forbidden in @(
            '-world', '-config', '-backendLocalStorage', '-backendDisableStorage',
            '-noBackend', '-rpl-validation-rdb-disable',
            '-rpl-validation-scr-disable', '-rpl-validation-version-disable',
            '-rpl-validation-devbin-disable', '-rpl-validation-addons-disable')) {
        if ($raw -icontains $forbidden) {
            throw "$Mode raw arguments contain forbidden option $forbidden."
        }
    }
    foreach ($option in @(
            '-gproj', '-server', '-MissionHeader', '-addonsDir', '-addons',
            '-profile', '-logsDir', '-addonTempDir', '-releaseSurfaceRunNonce',
            '-releaseSurfaceExpectedMode', '-hstReleaseCandidateId',
            '-hstReleasePackageSha256', '-hstReleaseManifestSha256')) {
        $null = Get-ReleaseSurfaceIndexOptionValue $raw $option "$Mode raw arguments"
    }
    foreach ($index in 0..($raw.Count - 1)) {
        if ($index -in @(1, 7, 11, 13, 15)) { continue }
        if ([string]$raw[$index] -cne [string]$portable[$index]) {
            throw "$Mode raw and portable arguments differ outside redacted paths."
        }
    }
    if ([IO.Path]::GetFileName(
            (Get-ReleaseSurfaceIndexOptionValue $raw '-gproj' "$Mode raw")) `
            -cne 'addon.gproj' -or
        (Get-ReleaseSurfaceIndexOptionValue $raw '-addonsDir' "$Mode raw").
            IndexOf(',', [StringComparison]::Ordinal) -lt 1) {
        throw "$Mode raw candidate-stage arguments are invalid."
    }
    return [pscustomobject][ordered]@{
        Raw = $raw
        Portable = $portable
        ExecutablePath = [string]$RawArgumentsParsed.Value.executable
        ExecutableLeaf = $expectedExecutable
    }
}

function Assert-ReleaseSurfaceIndexReceipt {
    param(
        [string]$Mode,
        [string]$RunRoot,
        $Run,
        $ModeValue,
        $ArgumentBinding,
        [bool]$Synthetic
    )

    $process = $ModeValue.process
    $receiptPath = Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
        ([string]$process.receiptPath) "$Mode receipt path"
    Assert-ReleaseSurfaceIndexSignature $process.receiptSignature `
        "$Mode receipt signature"
    $tested = Test-PartisanGuardedRuntimeReceipt `
        -Path $receiptPath `
        -ExpectedSignature $process.receiptSignature
    if (-not [bool]$tested.Complete -or
        [string]$tested.Status -cne 'complete' -or
        [string]$tested.Certification -cne 'clean' -or
        -not [bool]$tested.GuardDirectoryAbsent -or
        [bool]$tested.FinalRemovalFaultPresent -or
        -not [bool]$tested.CompletionAttestationExact -or
        [string]$tested.CompletionAttestationStatus -cne
            'guard-removed-by-exact-owner' -or
        [string]$tested.ContextId -cne [string]$process.contextId -or
        [string]$tested.CandidateBindingSha256 -cne
            [string]$process.candidateBindingSha256) {
        throw "$Mode guarded runtime receipt is incomplete or unbound."
    }

    $receiptParsed = Read-ReleaseSurfaceIndexJsonArtifact `
        $receiptPath "$Mode guarded runtime receipt"
    $receipt = $receiptParsed.Value
    Assert-ReleaseSurfaceIndexExactProperties $receipt @(
        'version', 'magic', 'nonce', 'guardBindingSha256', 'guardDirectory',
        'journalPath', 'journalSignature', 'completionAttestationPath',
        'completionAttestationSignature', 'completionTokenSha256',
        'candidateBindingSha256', 'guardInventorySha256',
        'boundaryBaselineSha256', 'executableBindingsSha256',
        'processLedgerSha256', 'launchBindings', 'processLedger',
        'guardedToolHashes', 'portSemantics', 'candidateConsumptionBridge',
        'teardownStopOrder', 'guardRemovalAuthorized', 'completionSemantics',
        'recordedUtc') "$Mode guarded runtime receipt"
    Assert-ReleaseSurfaceIndexIntegerProperties $receipt @(
        'version') "$Mode guarded runtime receipt"
    Assert-ReleaseSurfaceIndexStringProperties $receipt @(
        'magic', 'nonce', 'guardBindingSha256', 'guardDirectory', 'journalPath',
        'completionAttestationPath', 'completionTokenSha256',
        'candidateBindingSha256', 'guardInventorySha256',
        'boundaryBaselineSha256', 'executableBindingsSha256',
        'processLedgerSha256', 'portSemantics', 'candidateConsumptionBridge',
        'completionSemantics', 'recordedUtc') "$Mode guarded runtime receipt"
    Assert-ReleaseSurfaceIndexBooleanProperties $receipt @(
        'guardRemovalAuthorized') "$Mode guarded runtime receipt"
    if ([int]$receipt.version -ne 2 -or
        [string]$receipt.magic -cne 'partisan_guarded_runtime_clean_receipt_v2' -or
        [string]$receipt.nonce -cne [string]$process.contextId -or
        [string]$receipt.candidateBindingSha256 -cne
            [string]$process.candidateBindingSha256 -or
        -not [bool]$receipt.guardRemovalAuthorized -or
        [string]$receipt.portSemantics -cne
            'ipv4-loopback-bind-availability-only-no-pid-attribution' -or
        [string]$receipt.candidateConsumptionBridge -cne
            'exact-stage-and-candidate-argv-enforced-for-engine-launches' -or
        [string]$receipt.completionSemantics -cne
            'complete-only-with-exact-post-removal-attestation') {
        throw "$Mode guarded runtime receipt schema is invalid."
    }
    $launches = @($receipt.launchBindings)
    if ($launches.Count -ne 1) {
        throw "$Mode guarded receipt does not contain exactly one launch."
    }
    $launch = $launches[0]
    Assert-ReleaseSurfaceIndexExactProperties $launch @(
        'role', 'identity', 'path', 'provenance', 'arguments',
        'workingDirectory', 'isEngine', 'candidateConsumptionEvidence') `
        "$Mode guarded launch"
    Assert-ReleaseSurfaceIndexStringProperties $launch @(
        'role', 'path', 'workingDirectory') "$Mode guarded launch"
    Assert-ReleaseSurfaceIndexBooleanProperties $launch @(
        'isEngine') "$Mode guarded launch"
    Assert-ReleaseSurfaceIndexStringArray $launch.arguments `
        "$Mode guarded launch arguments"
    if ([string]$launch.role -cne 'server' -or
        [bool]$launch.isEngine -ne (-not $Synthetic) -or
        [string]$launch.path -cne [string]$ArgumentBinding.ExecutablePath -or
        -not (Test-ReleaseSurfaceIndexStringArrayExact `
            ([string[]]$ArgumentBinding.Raw) ([string[]]$launch.arguments))) {
        throw "$Mode guarded launch topology or raw arguments are invalid."
    }
    Assert-ReleaseSurfaceIndexValuesEqual `
        $ModeValue.executable $launch.provenance `
        @('fileName', 'fileVersion', 'productVersion', 'length', 'sha256') `
        "$Mode guarded executable provenance"
    Assert-ReleaseSurfaceIndexExactProperties $launch.candidateConsumptionEvidence @(
        'mode', 'candidateBindingSha256', 'exactArgumentPairs') `
        "$Mode guarded candidate consumption"
    Assert-ReleaseSurfaceIndexStringProperties `
        $launch.candidateConsumptionEvidence @('mode', 'candidateBindingSha256') `
        "$Mode guarded candidate consumption"
    $expectedConsumption = if ($Synthetic) { 'non-engine-self-test-only' }
        else { 'exact-stage-and-candidate-argv-enforced' }
    $expectedPairs = if ($Synthetic) { 0 } else { 4 }
    $exactPairs = @($launch.candidateConsumptionEvidence.exactArgumentPairs)
    if ([string]$launch.candidateConsumptionEvidence.mode -cne
            $expectedConsumption -or
        [string]$launch.candidateConsumptionEvidence.candidateBindingSha256 -cne
            [string]$process.candidateBindingSha256 -or
        $exactPairs.Count -ne $expectedPairs) {
        throw "$Mode candidate-consumption evidence is invalid."
    }
    $seenPairFlags = New-Object Collections.Generic.HashSet[string](
        [StringComparer]::Ordinal)
    foreach ($pair in $exactPairs) {
        Assert-ReleaseSurfaceIndexExactProperties $pair @(
            'flag', 'value', 'index') "$Mode exact candidate argument pair"
        Assert-ReleaseSurfaceIndexStringProperties $pair @(
            'flag', 'value') "$Mode exact candidate argument pair"
        Assert-ReleaseSurfaceIndexIntegerProperties $pair @(
            'index') "$Mode exact candidate argument pair"
        $pairIndex = [int]$pair.index
        if (-not $seenPairFlags.Add([string]$pair.flag) -or
            $pairIndex -lt 0 -or $pairIndex + 1 -ge $ArgumentBinding.Raw.Count -or
            [string]$ArgumentBinding.Raw[$pairIndex] -cne [string]$pair.flag -or
            [string]$ArgumentBinding.Raw[$pairIndex + 1] -cne [string]$pair.value) {
            throw "$Mode exact candidate argument pair is invalid or duplicated."
        }
    }
    $expectedPairFlags = if ($Synthetic) { @() } else {
        @('-addonsDir', '-gproj', '-hstReleaseCandidateId',
            '-hstReleasePackageSha256')
    }
    if (@(Compare-Object `
            -ReferenceObject @($expectedPairFlags | Sort-Object) `
            -DifferenceObject @($seenPairFlags | Sort-Object) `
            -CaseSensitive).Count -ne 0) {
        throw "$Mode exact candidate argument-pair flag set is invalid."
    }

    foreach ($artifactBinding in @(
            [pscustomobject]@{
                Label = "$Mode journal"
                Path = [string]$process.journalPath
                Signature = $process.journalSignature
                TestedPath = [string]$tested.JournalPath
                TestedSignature = $tested.JournalSignature
            },
            [pscustomobject]@{
                Label = "$Mode completion attestation"
                Path = [string]$process.completionPath
                Signature = $process.completionSignature
                TestedPath = [string]$tested.CompletionAttestationPath
                TestedSignature = $tested.CompletionAttestationSignature
            })) {
        Assert-ReleaseSurfaceIndexSignature $artifactBinding.Signature `
            ($artifactBinding.Label + ' signature')
        $path = Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
            $artifactBinding.Path ($artifactBinding.Label + ' path')
        if (-not ([IO.Path]::GetFullPath($path)).Equals(
                [IO.Path]::GetFullPath($artifactBinding.TestedPath),
                [StringComparison]::OrdinalIgnoreCase) -or
            -not (Test-ReleaseSurfaceIndexSignatureExact `
                $artifactBinding.Signature `
                (Get-ReleaseSurfaceIndexFileSignature $path)) -or
            -not (Test-ReleaseSurfaceIndexSignatureExact `
                $artifactBinding.Signature $artifactBinding.TestedSignature)) {
            throw "$($artifactBinding.Label) path or signature is invalid."
        }
        $null = Read-ReleaseSurfaceIndexJsonArtifact $path $artifactBinding.Label
    }
    return [string]$process.candidateBindingSha256
}

function Assert-ReleaseSurfaceIndexStreams {
    param([string]$Mode, [string]$RunRoot, $ModeValue)

    Assert-ReleaseSurfaceIndexExactProperties $ModeValue.streams @(
        'stdout', 'stderr', 'capture') "$Mode stream binding"
    Assert-ReleaseSurfaceIndexStringProperties $ModeValue.streams @(
        'stdout', 'stderr', 'capture') "$Mode stream binding"
    $stdoutPath = Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
        ([string]$ModeValue.streams.stdout) "$Mode stdout path"
    $stderrPath = Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
        ([string]$ModeValue.streams.stderr) "$Mode stderr path"
    $capturePath = Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
        ([string]$ModeValue.streams.capture) "$Mode capture path"
    if ((Get-Item -LiteralPath $stdoutPath -Force).Length -ne 0 -or
        (Get-Item -LiteralPath $stderrPath -Force).Length -ne 0) {
        throw "$Mode inherited stream artifacts are not empty."
    }
    $capture = (Read-ReleaseSurfaceIndexJsonArtifact `
        $capturePath "$Mode stream capture" -Portable).Value
    Assert-ReleaseSurfaceIndexExactProperties $capture @(
        'schemaVersion', 'mode', 'captureMode', 'stdoutPath', 'stdoutBytes',
        'stderrPath', 'stderrBytes', 'engineLogsAreAuthoritative') `
        "$Mode stream capture"
    Assert-ReleaseSurfaceIndexIntegerProperties $capture @(
        'schemaVersion', 'stdoutBytes', 'stderrBytes') "$Mode stream capture"
    Assert-ReleaseSurfaceIndexStringProperties $capture @(
        'mode', 'captureMode', 'stdoutPath', 'stderrPath') "$Mode stream capture"
    Assert-ReleaseSurfaceIndexBooleanProperties $capture @(
        'engineLogsAreAuthoritative') "$Mode stream capture"
    if ([int]$capture.schemaVersion -ne 1 -or
        [string]$capture.mode -cne $Mode -or
        [string]$capture.captureMode -cne
            'guarded-native-no-inherited-handles' -or
        [string]$capture.stdoutPath -cne 'stdout.raw.txt' -or
        [long]$capture.stdoutBytes -ne 0 -or
        [string]$capture.stderrPath -cne 'stderr.raw.txt' -or
        [long]$capture.stderrBytes -ne 0 -or
        -not [bool]$capture.engineLogsAreAuthoritative) {
        throw "$Mode stream-capture contract is invalid."
    }
}

function Get-ReleaseSurfaceIndexHardDiagnosticCensus {
    param(
        [Parameter(Mandatory = $true)][hashtable]$TextByLeaf,
        [Parameter(Mandatory = $true)][string]$Mode)

    $hardPattern = '(?i)(?:\b(?:SCRIPT|ENGINE)\s*\(E\)|' +
        '\bACCESS_VIOLATION\b|\bunhandled exception\b|\bfatal error\b|' +
        '\bapplication crash\b|Partisan release surface audit\s*\|\s*ERROR\s*\|)'
    $timestampToken = '(?<timestamp>\d{4}-\d{2}-\d{2} ' +
        '\d{2}:\d{2}:\d{2}\.\d{3})'
    $timestampedLinePattern = '^\d{4}-\d{2}-\d{2} ' +
        '\d{2}:\d{2}:\d{2}\.\d{3}\s+'
    $stockPattern = '^' + $timestampToken +
        "\s+SCRIPT\s+\(E\): 'SCR_BaseResupplySupportStationComponent' " +
        'needs a entity catalog manager!\s*$'
    $resultEventPattern = '^' + $timestampToken +
        '\s+SCRIPT\s+:\s+Partisan release surface audit \| RESULT \| mode=' +
        [regex]::Escape($Mode) +
        ' \| passed=(?:1|true) \| mismatches=0\s*$'
    $replicationFinishingPattern = '^' + $timestampToken +
        '\s+RPL\s+:\s+Replication finishing\.\.\.\s*$'
    $replicationFinishedPattern = '^' + $timestampToken +
        '\s+RPL\s+:\s+Replication finished\.\s*$'
    $gameDestroyedPattern = '^' + $timestampToken +
        '\s+ENGINE\s+:\s+Game destroyed\.\s*$'
    $hardRows = New-Object Collections.Generic.List[object]
    $resultRows = New-Object Collections.Generic.List[object]
    $replicationFinishingRows = New-Object Collections.Generic.List[object]
    $replicationFinishedRows = New-Object Collections.Generic.List[object]
    $gameDestroyedRows = New-Object Collections.Generic.List[object]
    $allLines = New-Object Collections.Generic.List[string]
    foreach ($leaf in @($TextByLeaf.Keys)) {
        $leafLines = @(([string]$TextByLeaf[$leaf]) -split "`r?`n")
        foreach ($leafLine in $leafLines) { [void]$allLines.Add($leafLine) }
        for ($lineIndex = 0; $lineIndex -lt $leafLines.Count; $lineIndex++) {
            $line = [string]$leafLines[$lineIndex]
            $stockMatch = [regex]::Match($line, $stockPattern)
            $emptyDiagnosticBody = $true
            if ($stockMatch.Success) {
                for ($bodyIndex = $lineIndex + 1;
                    $bodyIndex -lt $leafLines.Count;
                    $bodyIndex++) {
                    $bodyLine = [string]$leafLines[$bodyIndex]
                    if ($bodyLine -match $timestampedLinePattern) { break }
                    if (-not [string]::IsNullOrWhiteSpace($bodyLine)) {
                        $emptyDiagnosticBody = $false
                        break
                    }
                }
            }
            if ($line -match $hardPattern) {
                [void]$hardRows.Add([pscustomobject][ordered]@{
                    leaf = $leaf
                    index = $lineIndex
                    line = $line
                    stockCandidate = [bool]($stockMatch.Success -and
                        $emptyDiagnosticBody)
                    stockTimestamp = if ($stockMatch.Success) {
                        [string]$stockMatch.Groups['timestamp'].Value
                    }
                    else { '' }
                })
            }
            $resultMatch = [regex]::Match($line, $resultEventPattern)
            if ($resultMatch.Success) {
                [void]$resultRows.Add([pscustomobject][ordered]@{
                    leaf = $leaf
                    index = $lineIndex
                    timestamp = [string]$resultMatch.Groups['timestamp'].Value
                })
            }
            if ($leaf -ceq 'console.log') {
                foreach ($lifecycleBinding in @(
                        [pscustomobject]@{
                            Pattern = $replicationFinishingPattern
                            Rows = $replicationFinishingRows
                        },
                        [pscustomobject]@{
                            Pattern = $replicationFinishedPattern
                            Rows = $replicationFinishedRows
                        },
                        [pscustomobject]@{
                            Pattern = $gameDestroyedPattern
                            Rows = $gameDestroyedRows
                        })) {
                    $lifecycleMatch = [regex]::Match(
                        $line, [string]$lifecycleBinding.Pattern)
                    if ($lifecycleMatch.Success) {
                        [void]$lifecycleBinding.Rows.Add(
                            [pscustomobject][ordered]@{
                                index = $lineIndex
                                timestamp = [string]$lifecycleMatch.
                                    Groups['timestamp'].Value
                            })
                    }
                }
            }
        }
    }
    $hardEventCount = 0
    foreach ($eventGroup in @($hardRows |
            Group-Object -Property line -CaseSensitive)) {
        $leafMaximum = 0
        foreach ($leafGroup in @($eventGroup.Group |
                Group-Object -Property leaf -CaseSensitive)) {
            $leafMaximum = [Math]::Max($leafMaximum, $leafGroup.Count)
        }
        $hardEventCount += $leafMaximum
    }
    $stockHeaderRows = @($hardRows | Where-Object {
        -not [string]::IsNullOrEmpty([string]$_.stockTimestamp)
    })
    $stockRows = @($hardRows | Where-Object { [bool]$_.stockCandidate })
    $stockEventGroups = @($stockRows |
        Group-Object -Property stockTimestamp -CaseSensitive)
    $resultPattern = '^.*Partisan release surface audit \| RESULT \| mode=' +
        [regex]::Escape($Mode) +
        ' \| passed=(?:1|true) \| mismatches=0\s*$'
    $resultMarkers = @($allLines.ToArray() | Where-Object {
        [string]$_ -cmatch $resultPattern
    } | ForEach-Object {
        [regex]::Replace(
            ([string]$_).Trim(), $timestampedLinePattern, '')
    } | Sort-Object -Unique)
    $resultChannelShapeExact = $resultRows.Count -eq 2 -and
        @($resultRows | Where-Object { $_.leaf -ceq 'console.log' }).Count -eq 1 -and
        @($resultRows | Where-Object { $_.leaf -ceq 'script.log' }).Count -eq 1
    $resultTime = [datetime]::MinValue
    $scriptResultTime = [datetime]::MinValue
    $resultTimeValid = $false
    $scriptResultTimeValid = $false
    $consoleResult = $null
    if ($resultChannelShapeExact) {
        $consoleResult = @($resultRows | Where-Object {
            $_.leaf -ceq 'console.log'
        })[0]
        $scriptResult = @($resultRows | Where-Object {
            $_.leaf -ceq 'script.log'
        })[0]
        $resultTimeValid = [datetime]::TryParseExact(
            [string]$consoleResult.timestamp, 'yyyy-MM-dd HH:mm:ss.fff',
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::None, [ref]$resultTime)
        $scriptResultTimeValid = [datetime]::TryParseExact(
            [string]$scriptResult.timestamp, 'yyyy-MM-dd HH:mm:ss.fff',
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::None, [ref]$scriptResultTime)
        if ($resultTimeValid -and $scriptResultTimeValid -and
            $scriptResultTime -gt $resultTime) {
            $resultTime = $scriptResultTime
        }
    }
    $resultChannelsExact = $resultChannelShapeExact -and
        $resultTimeValid -and $scriptResultTimeValid
    $lifecycleExact = $resultChannelsExact -and
        $replicationFinishingRows.Count -eq 1 -and
        $replicationFinishedRows.Count -eq 1 -and
        $gameDestroyedRows.Count -eq 1
    $replicationFinishingTime = [datetime]::MinValue
    $replicationFinishedTime = [datetime]::MinValue
    $gameDestroyedTime = [datetime]::MinValue
    if ($lifecycleExact) {
        $replicationFinishingTimeValid = [datetime]::TryParseExact(
            [string]$replicationFinishingRows[0].timestamp,
            'yyyy-MM-dd HH:mm:ss.fff',
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::None,
            [ref]$replicationFinishingTime)
        $replicationFinishedTimeValid = [datetime]::TryParseExact(
            [string]$replicationFinishedRows[0].timestamp,
            'yyyy-MM-dd HH:mm:ss.fff',
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::None,
            [ref]$replicationFinishedTime)
        $gameDestroyedTimeValid = [datetime]::TryParseExact(
            [string]$gameDestroyedRows[0].timestamp,
            'yyyy-MM-dd HH:mm:ss.fff',
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::None, [ref]$gameDestroyedTime)
        $lifecycleExact = $resultTimeValid -and
            $replicationFinishingTimeValid -and
            $replicationFinishedTimeValid -and $gameDestroyedTimeValid -and
            $consoleResult.index -lt
                $replicationFinishingRows[0].index -and
            $replicationFinishingRows[0].index -lt
                $replicationFinishedRows[0].index -and
            $replicationFinishedRows[0].index -lt
                $gameDestroyedRows[0].index -and
            $resultTime -lt $replicationFinishingTime -and
            $replicationFinishingTime -lt $replicationFinishedTime -and
            $replicationFinishedTime -lt $gameDestroyedTime
    }
    $clusterPresent = $stockHeaderRows.Count -gt 0
    $clusterShapeExact = -not $clusterPresent
    if ($clusterPresent -and $stockHeaderRows.Count -eq 6 -and
        $stockRows.Count -eq 6 -and $stockEventGroups.Count -eq 2) {
        $clusterShapeExact = $true
        foreach ($eventGroup in $stockEventGroups) {
            $eventLeaves = @($eventGroup.Group | ForEach-Object {
                [string]$_.leaf
            } | Sort-Object)
            if ($eventGroup.Count -ne 3 -or
                @(Compare-Object -ReferenceObject @(
                        'console.log', 'error.log', 'script.log') `
                    -DifferenceObject $eventLeaves -CaseSensitive).Count -ne 0) {
                $clusterShapeExact = $false
                break
            }
        }
    }
    $clusterLifecycleExact = $lifecycleExact -and $clusterShapeExact
    if ($clusterLifecycleExact -and $clusterPresent) {
        foreach ($eventGroup in $stockEventGroups) {
            $eventTime = [datetime]::ParseExact(
                [string]$eventGroup.Name, 'yyyy-MM-dd HH:mm:ss.fff',
                [Globalization.CultureInfo]::InvariantCulture)
            $consoleEvent = @($eventGroup.Group | Where-Object {
                $_.leaf -ceq 'console.log'
            })[0]
            if ($eventTime -le $resultTime -or
                $eventTime -le $replicationFinishedTime -or
                $eventTime -ge $gameDestroyedTime -or
                $consoleEvent.index -le $replicationFinishedRows[0].index -or
                $consoleEvent.index -ge $gameDestroyedRows[0].index) {
                $clusterLifecycleExact = $false
                break
            }
        }
    }
    $approvedStockRawCount = if ($clusterShapeExact -and
        $clusterLifecycleExact) { $stockRows.Count } else { 0 }
    $approvedStockEventCount = if ($clusterShapeExact -and
        $clusterLifecycleExact) { $stockEventGroups.Count } else { 0 }
    $unapprovedHardRawCount = $hardRows.Count - $approvedStockRawCount
    $unapprovedHardEventCount = $hardEventCount - $approvedStockEventCount
    $hardAccountingExact = $hardRows.Count -eq
            ($approvedStockRawCount + $unapprovedHardRawCount) -and
        $hardEventCount -eq
            ($approvedStockEventCount + $unapprovedHardEventCount)
    return [pscustomobject][ordered]@{
        allLines = [string[]]$allLines.ToArray()
        hardDiagnosticPolicy = $script:HardDiagnosticPolicy
        hardDiagnosticFree = $hardRows.Count -eq 0
        hardDiagnosticRawLineCount = $hardRows.Count
        hardDiagnosticEventCount = $hardEventCount
        approvedStockDiagnosticClusterPresent = $clusterPresent
        approvedStockDiagnosticClusterExact = $clusterShapeExact
        approvedStockDiagnosticLifecycleExact = $clusterLifecycleExact
        approvedStockDiagnosticRawLineCount = $approvedStockRawCount
        approvedStockDiagnosticEventCount = $approvedStockEventCount
        unapprovedHardDiagnosticRawLineCount = $unapprovedHardRawCount
        unapprovedHardDiagnosticEventCount = $unapprovedHardEventCount
        hardDiagnosticAccountingExact = $hardAccountingExact
        uniqueResultMarkerCount = $resultMarkers.Count
        resultMarkerOccurrenceCount = $resultRows.Count
        crashLogContentValid = -not $TextByLeaf.ContainsKey('crash.log') -or
            [string]::IsNullOrWhiteSpace([string]$TextByLeaf['crash.log'])
    }
}

function Get-ReleaseSurfaceIndexCandidateMountAttestation {
    param(
        [Parameter(Mandatory = $true)][string]$ConsoleText,
        [Parameter(Mandatory = $true)][string]$CandidateGuid,
        [Parameter(Mandatory = $true)][string]$CandidateProjectPath
    )

    $expectedProject = [IO.Path]::GetFullPath($CandidateProjectPath).
        Replace('\', '/')
    $pattern = '(?m)^\s*\d{4}-\d{2}-\d{2} ' +
        '\d{2}:\d{2}:\d{2}\.\d{3}\s+ENGINE\s+:\s+' +
        "gproj:\s+'(?<path>[^']+)'\s+guid:\s+'" +
        "(?<guid>[0-9A-Fa-f]{16})'\s*(?<mode>\([^)]+\))?\s*$"
    $recordCount = 0
    $exactPathCount = 0
    $packedCount = 0
    $invalidModeCount = 0
    $guidCaseDriftCount = 0
    foreach ($match in @([regex]::Matches($ConsoleText, $pattern))) {
        $recordedGuid = [string]$match.Groups['guid'].Value
        if (-not $recordedGuid.Equals(
                $CandidateGuid,
                [StringComparison]::OrdinalIgnoreCase)) {
            continue
        }
        $recordCount++
        if ($recordedGuid -cne $CandidateGuid) {
            $guidCaseDriftCount++
        }
        $recordedProject = [string]$match.Groups['path'].Value
        $recordedProject = $recordedProject.Replace('\', '/')
        if ($recordedProject.Equals(
                $expectedProject,
                [StringComparison]::OrdinalIgnoreCase)) {
            $exactPathCount++
        }
        $mode = [string]$match.Groups['mode'].Value
        if ($mode -ceq '(packed)') {
            $packedCount++
        }
        elseif (-not [string]::IsNullOrEmpty($mode)) {
            $invalidModeCount++
        }
    }
    $guidExact = $recordCount -gt 0 -and $guidCaseDriftCount -eq 0
    $packed = $packedCount -eq 1 -and $invalidModeCount -eq 0
    $valid = $recordCount -eq 2 -and
        $exactPathCount -eq 2 -and
        $packedCount -eq 1 -and
        $invalidModeCount -eq 0 -and
        $guidExact -and $packed
    return [pscustomobject][ordered]@{
        valid = [bool]$valid
        recordCount = $recordCount
        exactPathCount = $exactPathCount
        packedCount = $packedCount
        invalidModeCount = $invalidModeCount
        guidExact = [bool]$guidExact
        packed = [bool]$packed
    }
}

function Assert-ReleaseSurfaceIndexLogClassification {
    param(
        [string]$Mode,
        [string]$RunRoot,
        $Run,
        $ModeValue,
        $ArgumentBinding)

    $classification = $ModeValue.classification
    Assert-ReleaseSurfaceIndexExactProperties $classification @(
        'valid', 'hardDiagnosticPolicy', 'hardDiagnosticFree',
        'hardDiagnosticRawLineCount',
        'hardDiagnosticEventCount', 'approvedStockDiagnosticClusterPresent',
        'approvedStockDiagnosticClusterExact',
        'approvedStockDiagnosticLifecycleExact',
        'approvedStockDiagnosticRawLineCount',
        'approvedStockDiagnosticEventCount',
        'unapprovedHardDiagnosticRawLineCount',
        'unapprovedHardDiagnosticEventCount', 'hardDiagnosticAccountingExact',
        'candidateMountAttestation',
        'harnessMountLineCount', 'uniqueResultMarkerCount',
        'resultMarkerOccurrenceCount', 'crashLogContentValid',
        'crashArtifactCount', 'logs') `
        "$Mode log classification"
    Assert-ReleaseSurfaceIndexStringProperties $classification @(
        'hardDiagnosticPolicy') "$Mode log classification"
    Assert-ReleaseSurfaceIndexBooleanProperties $classification @(
        'valid', 'hardDiagnosticFree', 'approvedStockDiagnosticClusterPresent',
        'approvedStockDiagnosticClusterExact',
        'approvedStockDiagnosticLifecycleExact',
        'hardDiagnosticAccountingExact', 'crashLogContentValid') `
        "$Mode log classification"
    Assert-ReleaseSurfaceIndexIntegerProperties $classification @(
        'hardDiagnosticRawLineCount', 'hardDiagnosticEventCount',
        'approvedStockDiagnosticRawLineCount',
        'approvedStockDiagnosticEventCount',
        'unapprovedHardDiagnosticRawLineCount',
        'unapprovedHardDiagnosticEventCount', 'harnessMountLineCount',
        'uniqueResultMarkerCount', 'resultMarkerOccurrenceCount',
        'crashArtifactCount') `
        "$Mode log classification"
    $recordedMount = $classification.candidateMountAttestation
    Assert-ReleaseSurfaceIndexExactProperties $recordedMount @(
        'valid', 'recordCount', 'exactPathCount', 'packedCount',
        'invalidModeCount', 'guidExact', 'packed') `
        "$Mode candidate mount attestation"
    Assert-ReleaseSurfaceIndexBooleanProperties $recordedMount @(
        'valid', 'guidExact', 'packed') "$Mode candidate mount attestation"
    Assert-ReleaseSurfaceIndexIntegerProperties $recordedMount @(
        'recordCount', 'exactPathCount', 'packedCount', 'invalidModeCount') `
        "$Mode candidate mount attestation"
    $rows = @($classification.logs)
    if ($rows.Count -lt $script:RequiredLogLeaves.Count -or
        $rows.Count -gt $script:AllowedLogLeaves.Count) {
        throw "$Mode retained log count is invalid."
    }
    $texts = New-Object Collections.Generic.List[string]
    $textByLeaf = @{}
    $seen = New-Object Collections.Generic.HashSet[string]([StringComparer]::Ordinal)
    foreach ($row in $rows) {
        Assert-ReleaseSurfaceIndexExactProperties $row @(
            'leaf', 'path', 'length', 'sha256') "$Mode log row"
        Assert-ReleaseSurfaceIndexStringProperties $row @(
            'leaf', 'path', 'sha256') "$Mode log row"
        Assert-ReleaseSurfaceIndexIntegerProperties $row @(
            'length') "$Mode log row"
        if ([string]$row.leaf -cnotin $script:AllowedLogLeaves -or
            -not $seen.Add([string]$row.leaf) -or
            [string]$row.path -cnotmatch
                ('^raw/' + [regex]::Escape($Mode) + '/logs/(?:.+/)?' +
                    [regex]::Escape([string]$row.leaf) + '$')) {
            throw "$Mode log row is invalid or duplicated."
        }
        $path = Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
            ([string]$row.path) "$Mode log path"
        if (-not (Test-ReleaseSurfaceIndexSignatureExact `
                $row (Get-ReleaseSurfaceIndexFileSignature $path))) {
            throw "$Mode retained log changed."
        }
        $text = [IO.File]::ReadAllText($path)
        [void]$texts.Add($text)
        $textByLeaf[[string]$row.leaf] = $text
    }
    $missingRequiredLeaves = @($script:RequiredLogLeaves | Where-Object {
        -not $seen.Contains([string]$_)
    })
    if ($missingRequiredLeaves.Count -ne 0) {
        throw "$Mode required log leaf set is incomplete."
    }
    $modeRoot = Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
        ('raw/' + $Mode + '/stdout.raw.txt') "$Mode raw root anchor"
    $modeRoot = Split-Path -Parent $modeRoot
    $crashArtifacts = @(Get-ChildItem -LiteralPath $modeRoot -Recurse -File `
        -Force -ErrorAction Stop | Where-Object {
            $_.Extension -in @('.dmp', '.mdmp') -or
            $_.Name -match '(?i)^minidump'
        })
    if ($crashArtifacts.Count -ne 0) {
        throw "$Mode logs fail the independently recomputed classification."
    }
    $logTreeRoot = Split-Path -Parent (
        Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
            ('raw/' + $Mode + '/logs/session-anchor') "$Mode log-root anchor")
    $actualLogPaths = @(Get-ChildItem -LiteralPath $logTreeRoot -Recurse -File `
        -Force -ErrorAction Stop | ForEach-Object {
            ConvertTo-ReleaseSurfaceIndexPortableRelativePath `
                $RunRoot $_.FullName "$Mode log-tree path"
        } | Sort-Object)
    $boundLogPaths = @($rows | ForEach-Object { [string]$_.path } | Sort-Object)
    if (@(Compare-Object $boundLogPaths $actualLogPaths -CaseSensitive).Count -ne 0) {
        throw "$Mode log tree contains unbound, duplicated, or unknown log leaves."
    }
    $diagnosticCensus = Get-ReleaseSurfaceIndexHardDiagnosticCensus `
        -TextByLeaf $textByLeaf `
        -Mode $Mode
    $allLines = [string[]]$diagnosticCensus.allLines
    $candidateProjectPath = Get-ReleaseSurfaceIndexOptionValue `
        ([string[]]$ArgumentBinding.Raw) '-gproj' "$Mode raw arguments"
    $candidateMountAttestation =
        Get-ReleaseSurfaceIndexCandidateMountAttestation `
            -ConsoleText ([string]$textByLeaf['console.log']) `
            -CandidateGuid ([string]$Run.candidate.addonGuid) `
            -CandidateProjectPath $candidateProjectPath
    $harnessLines = @($allLines | Where-Object {
        ([string]$_).IndexOf(
            [string]$Run.harnessGuid,
            [StringComparison]::OrdinalIgnoreCase) -ge 0
    })
    if (-not [bool]$classification.valid -or
        [string]$classification.hardDiagnosticPolicy -cne
            [string]$diagnosticCensus.hardDiagnosticPolicy -or
        [bool]$classification.hardDiagnosticFree -ne
            [bool]$diagnosticCensus.hardDiagnosticFree -or
        [int]$classification.hardDiagnosticRawLineCount -ne
            [int]$diagnosticCensus.hardDiagnosticRawLineCount -or
        [int]$classification.hardDiagnosticEventCount -ne
            [int]$diagnosticCensus.hardDiagnosticEventCount -or
        [bool]$classification.approvedStockDiagnosticClusterPresent -ne
            [bool]$diagnosticCensus.approvedStockDiagnosticClusterPresent -or
        [bool]$classification.approvedStockDiagnosticClusterExact -ne
            [bool]$diagnosticCensus.approvedStockDiagnosticClusterExact -or
        [bool]$classification.approvedStockDiagnosticLifecycleExact -ne
            [bool]$diagnosticCensus.approvedStockDiagnosticLifecycleExact -or
        [int]$classification.approvedStockDiagnosticRawLineCount -ne
            [int]$diagnosticCensus.approvedStockDiagnosticRawLineCount -or
        [int]$classification.approvedStockDiagnosticEventCount -ne
            [int]$diagnosticCensus.approvedStockDiagnosticEventCount -or
        [int]$classification.unapprovedHardDiagnosticRawLineCount -ne
            [int]$diagnosticCensus.unapprovedHardDiagnosticRawLineCount -or
        [int]$classification.unapprovedHardDiagnosticEventCount -ne
            [int]$diagnosticCensus.unapprovedHardDiagnosticEventCount -or
        [bool]$classification.hardDiagnosticAccountingExact -ne
            [bool]$diagnosticCensus.hardDiagnosticAccountingExact -or
        [bool]$recordedMount.valid -ne
            [bool]$candidateMountAttestation.valid -or
        [int]$recordedMount.recordCount -ne
            [int]$candidateMountAttestation.recordCount -or
        [int]$recordedMount.exactPathCount -ne
            [int]$candidateMountAttestation.exactPathCount -or
        [int]$recordedMount.packedCount -ne
            [int]$candidateMountAttestation.packedCount -or
        [int]$recordedMount.invalidModeCount -ne
            [int]$candidateMountAttestation.invalidModeCount -or
        [bool]$recordedMount.guidExact -ne
            [bool]$candidateMountAttestation.guidExact -or
        [bool]$recordedMount.packed -ne
            [bool]$candidateMountAttestation.packed -or
        [int]$classification.harnessMountLineCount -ne $harnessLines.Count -or
        [int]$classification.uniqueResultMarkerCount -ne
            [int]$diagnosticCensus.uniqueResultMarkerCount -or
        [int]$classification.resultMarkerOccurrenceCount -ne
            [int]$diagnosticCensus.resultMarkerOccurrenceCount -or
        [bool]$classification.crashLogContentValid -ne
            [bool]$diagnosticCensus.crashLogContentValid -or
        [int]$classification.crashArtifactCount -ne $crashArtifacts.Count -or
        -not [bool]$diagnosticCensus.approvedStockDiagnosticClusterExact -or
        -not [bool]$diagnosticCensus.approvedStockDiagnosticLifecycleExact -or
        -not [bool]$diagnosticCensus.hardDiagnosticAccountingExact -or
        [int]$diagnosticCensus.unapprovedHardDiagnosticRawLineCount -ne 0 -or
        [int]$diagnosticCensus.unapprovedHardDiagnosticEventCount -ne 0 -or
        -not [bool]$candidateMountAttestation.valid -or
        [int]$candidateMountAttestation.recordCount -ne 2 -or
        [int]$candidateMountAttestation.exactPathCount -ne 2 -or
        [int]$candidateMountAttestation.packedCount -ne 1 -or
        [int]$candidateMountAttestation.invalidModeCount -ne 0 -or
        -not [bool]$candidateMountAttestation.guidExact -or
        -not [bool]$candidateMountAttestation.packed -or
        $harnessLines.Count -lt 1 -or
        [int]$diagnosticCensus.uniqueResultMarkerCount -ne 1 -or
        [int]$diagnosticCensus.resultMarkerOccurrenceCount -ne 2 -or
        -not [bool]$diagnosticCensus.crashLogContentValid -or
        $crashArtifacts.Count -ne 0) {
        throw "$Mode logs fail the independently recomputed classification."
    }
    return [pscustomobject][ordered]@{
        hardDiagnosticPolicy =
            [string]$diagnosticCensus.hardDiagnosticPolicy
        hardDiagnosticFree = [bool]$diagnosticCensus.hardDiagnosticFree
        hardDiagnosticRawLineCount =
            [int]$diagnosticCensus.hardDiagnosticRawLineCount
        hardDiagnosticEventCount =
            [int]$diagnosticCensus.hardDiagnosticEventCount
        approvedStockDiagnosticClusterPresent =
            [bool]$diagnosticCensus.approvedStockDiagnosticClusterPresent
        approvedStockDiagnosticClusterExact =
            [bool]$diagnosticCensus.approvedStockDiagnosticClusterExact
        approvedStockDiagnosticLifecycleExact =
            [bool]$diagnosticCensus.approvedStockDiagnosticLifecycleExact
        approvedStockDiagnosticRawLineCount =
            [int]$diagnosticCensus.approvedStockDiagnosticRawLineCount
        approvedStockDiagnosticEventCount =
            [int]$diagnosticCensus.approvedStockDiagnosticEventCount
        unapprovedHardDiagnosticRawLineCount =
            [int]$diagnosticCensus.unapprovedHardDiagnosticRawLineCount
        unapprovedHardDiagnosticEventCount =
            [int]$diagnosticCensus.unapprovedHardDiagnosticEventCount
        hardDiagnosticAccountingExact =
            [bool]$diagnosticCensus.hardDiagnosticAccountingExact
        candidateMountAttestation = $candidateMountAttestation
        harnessMountLineCount = $harnessLines.Count
        uniqueResultMarkerCount =
            [int]$diagnosticCensus.uniqueResultMarkerCount
        resultMarkerOccurrenceCount =
            [int]$diagnosticCensus.resultMarkerOccurrenceCount
        crashLogContentValid =
            [bool]$diagnosticCensus.crashLogContentValid
        crashArtifactCount = $crashArtifacts.Count
    }
}

function Assert-ReleaseSurfaceIndexMode {
    param(
        [string]$Mode,
        [string]$RunRoot,
        $Run,
        $Bindings,
        $Contract,
        $MemberProbePlan,
        $ModeBinding,
        [bool]$Synthetic
    )

    Assert-ReleaseSurfaceIndexStringProperties $ModeBinding @(
        'mode', 'path') "$Mode run-mode binding"
    if ([string]$ModeBinding.mode -cne $Mode -or
        [string]$ModeBinding.path -cne ('modes/' + $Mode + '.json')) {
        throw "$Mode run-mode binding is not fixed or ordered."
    }
    Assert-ReleaseSurfaceIndexSignature $ModeBinding.signature `
        "$Mode envelope signature"
    $modePath = Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
        ([string]$ModeBinding.path) "$Mode envelope path"
    if (-not (Test-ReleaseSurfaceIndexSignatureExact `
            $ModeBinding.signature (Get-ReleaseSurfaceIndexFileSignature $modePath))) {
        throw "$Mode envelope differs from its run binding."
    }
    $modeParsed = Read-ReleaseSurfaceIndexJsonArtifact `
        $modePath "$Mode envelope" -Portable
    $value = $modeParsed.Value
    Assert-ReleaseSurfaceIndexExactProperties $value @(
        'schemaVersion', 'evidenceKind', 'mode', 'disposition', 'candidateId',
        'packageSha256', 'manifestSha256', 'readySha256', 'executable',
        'harnessGuid', 'harnessSha256', 'process', 'arguments', 'streams',
        'probe', 'classification', 'passed') "$Mode envelope"
    Assert-ReleaseSurfaceIndexIntegerProperties $value @(
        'schemaVersion') "$Mode envelope"
    Assert-ReleaseSurfaceIndexStringProperties $value @(
        'evidenceKind', 'mode', 'disposition', 'candidateId', 'packageSha256',
        'manifestSha256', 'readySha256', 'harnessGuid', 'harnessSha256') `
        "$Mode envelope"
    Assert-ReleaseSurfaceIndexBooleanProperties $value @('passed') "$Mode envelope"
    $expectedExecutable = if ($Synthetic) {
        [string]$value.executable.fileName
    }
    elseif ($Mode -ceq 'retail') {
        'ArmaReforgerServer.exe'
    }
    else { 'ArmaReforgerServerDiag.exe' }
    if ([int]$value.schemaVersion -ne 2 -or
        [string]$value.evidenceKind -cne $script:EvidenceKind -or
        [string]$value.mode -cne $Mode -or
        [string]$value.disposition -cne
            'passed-noncertifying-release-surface-audit' -or
        [string]$value.candidateId -cne [string]$Run.candidate.candidateId -or
        [string]$value.packageSha256 -cne
            [string]$Run.candidate.packageSha256 -or
        [string]$value.manifestSha256 -cne
            [string]$Run.candidate.manifestSha256 -or
        [string]$value.readySha256 -cne [string]$Run.candidate.readySha256 -or
        [string]$value.harnessGuid -cne [string]$Run.harnessGuid -or
        [string]$value.harnessSha256 -cne [string]$Run.harnessSha256 -or
        -not [bool]$value.passed) {
        throw "$Mode envelope is not bound to the successful paired run."
    }
    Assert-ReleaseSurfaceIndexProvenance $value.executable $expectedExecutable `
        "$Mode executable"
    $bindingExecutable = if ($Mode -ceq 'retail') {
        $Bindings.executables.retail
    }
    else { $Bindings.executables.diagnostic }
    if (-not $Synthetic) {
        Assert-ReleaseSurfaceIndexValuesEqual `
            $bindingExecutable $value.executable `
            @('fileName', 'fileVersion', 'productVersion', 'length', 'sha256') `
            "$Mode executable binding"
    }

    Assert-ReleaseSurfaceIndexExactProperties $value.process @(
        'exitCode', 'contextId', 'candidateBindingSha256', 'receiptPath',
        'receiptSignature', 'journalPath', 'journalSignature', 'completionPath',
        'completionSignature') "$Mode process binding"
    Assert-ReleaseSurfaceIndexIntegerProperties $value.process @(
        'exitCode') "$Mode process binding"
    Assert-ReleaseSurfaceIndexStringProperties $value.process @(
        'contextId', 'candidateBindingSha256', 'receiptPath', 'journalPath',
        'completionPath') "$Mode process binding"
    if ([int]$value.process.exitCode -ne 0 -or
        [string]$value.process.contextId -cnotmatch '^[0-9a-f]{32}$' -or
        [string]$value.process.candidateBindingSha256 -cnotmatch
            '^[0-9a-f]{64}$') {
        throw "$Mode process binding is invalid."
    }
    foreach ($pathProperty in @('receiptPath', 'journalPath', 'completionPath')) {
        if ([string]$value.process.$pathProperty -cnotmatch
            ('^raw/' + [regex]::Escape($Mode) +
                '/guarded-runtime/\.PartisanGuardedRuntime_[0-9a-f]{32}\.' +
                '(?:receipt|journal|completion)\.json$')) {
            throw "$Mode $pathProperty is outside its fixed guarded evidence root."
        }
    }

    Assert-ReleaseSurfaceIndexExactProperties $value.arguments @(
        'raw', 'portable') "$Mode argument binding"
    Assert-ReleaseSurfaceIndexStringProperties $value.arguments @(
        'raw', 'portable') "$Mode argument binding"
    if ([string]$value.arguments.raw -cne
            ('raw/' + $Mode + '/arguments.raw.json') -or
        [string]$value.arguments.portable -cne
            ('raw/' + $Mode + '/arguments.portable.json')) {
        throw "$Mode argument paths are not fixed."
    }
    $rawArguments = Read-ReleaseSurfaceIndexJsonArtifact `
        (Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
            ([string]$value.arguments.raw) "$Mode raw argument path") `
        "$Mode raw arguments"
    $portableArguments = Read-ReleaseSurfaceIndexJsonArtifact `
        (Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
            ([string]$value.arguments.portable) "$Mode portable argument path") `
        "$Mode portable arguments" -Portable
    $argumentBinding = Assert-ReleaseSurfaceIndexArguments `
        $Mode $RunRoot $Run $value $rawArguments $portableArguments $Synthetic
    $candidateBinding = Assert-ReleaseSurfaceIndexReceipt `
        $Mode $RunRoot $Run $value $argumentBinding $Synthetic
    Assert-ReleaseSurfaceIndexStreams $Mode $RunRoot $value

    Assert-ReleaseSurfaceIndexExactProperties $value.probe @(
        'path', 'signature', 'summary') "$Mode probe binding"
    Assert-ReleaseSurfaceIndexStringProperties $value.probe @(
        'path') "$Mode probe binding"
    if ([string]$value.probe.path -cnotmatch
        ('^raw/' + [regex]::Escape($Mode) + '/profile/.+/probe\.result\.json$')) {
        throw "$Mode probe path is outside its fixed profile root."
    }
    Assert-ReleaseSurfaceIndexSignature $value.probe.signature `
        "$Mode probe signature"
    $probePath = Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
        ([string]$value.probe.path) "$Mode probe path"
    if (-not (Test-ReleaseSurfaceIndexSignatureExact `
            $value.probe.signature (Get-ReleaseSurfaceIndexFileSignature $probePath))) {
        throw "$Mode probe bytes differ from their binding."
    }
    $probe = (Read-ReleaseSurfaceIndexJsonArtifact `
        $probePath "$Mode probe" -Portable).Value
    Assert-ReleaseSurfaceIndexProbe `
        $probe $Mode $Run $Contract $MemberProbePlan
    Assert-ReleaseSurfaceIndexExactProperties $value.probe.summary @(
        'mode', 'forbiddenTypeCount', 'productionTypeCount',
        'forbiddenCommandCount', 'productionCommandCount',
        'forbiddenMemberCount', 'productionMemberCount',
        'runtimeCompilerAvailable', 'runtimeMetadataAvailable', 'passed') `
        "$Mode probe summary"
    Assert-ReleaseSurfaceIndexStringProperties $value.probe.summary @(
        'mode') "$Mode probe summary"
    Assert-ReleaseSurfaceIndexIntegerProperties $value.probe.summary @(
        'forbiddenTypeCount', 'productionTypeCount', 'forbiddenCommandCount',
        'productionCommandCount', 'forbiddenMemberCount',
        'productionMemberCount') "$Mode probe summary"
    Assert-ReleaseSurfaceIndexBooleanProperties $value.probe.summary @(
        'runtimeCompilerAvailable', 'runtimeMetadataAvailable', 'passed') `
        "$Mode probe summary"
    if ([string]$value.probe.summary.mode -cne $Mode -or
        [int]$value.probe.summary.forbiddenTypeCount -ne
            @($Contract.forbiddenTypeNames).Count -or
        [int]$value.probe.summary.productionTypeCount -ne
            @($Contract.productionPositiveControlTypeNames).Count -or
        [int]$value.probe.summary.forbiddenCommandCount -ne
            @($Contract.forbiddenCommandActionIds).Count -or
        [int]$value.probe.summary.productionCommandCount -ne
            @($Contract.productionPositiveControlCommandActionIds).Count -or
        [int]$value.probe.summary.forbiddenMemberCount -ne
            @($MemberProbePlan.forbidden).Count -or
        [int]$value.probe.summary.productionMemberCount -ne
            @($MemberProbePlan.production).Count -or
        -not [bool]$value.probe.summary.runtimeCompilerAvailable -or
        -not [bool]$value.probe.summary.runtimeMetadataAvailable -or
        -not [bool]$value.probe.summary.passed) {
        throw "$Mode probe summary is not an exact projection."
    }
    $classification = Assert-ReleaseSurfaceIndexLogClassification `
        $Mode $RunRoot $Run $value $argumentBinding
    return [pscustomobject][ordered]@{
        mode = $Mode
        path = [string]$ModeBinding.path
        signature = $ModeBinding.signature
        executable = $value.executable
        contextId = [string]$value.process.contextId
        candidateBindingSha256 = $candidateBinding
        forbiddenTypeCount = @($Contract.forbiddenTypeNames).Count
        productionTypeCount = @($Contract.productionPositiveControlTypeNames).Count
        forbiddenCommandCount = @($Contract.forbiddenCommandActionIds).Count
        productionCommandCount =
            @($Contract.productionPositiveControlCommandActionIds).Count
        forbiddenMemberCount = @($MemberProbePlan.forbidden).Count
        productionMemberCount = @($MemberProbePlan.production).Count
        hardDiagnosticPolicy = [string]$classification.hardDiagnosticPolicy
        hardDiagnosticFree = [bool]$classification.hardDiagnosticFree
        hardDiagnosticRawLineCount =
            [int]$classification.hardDiagnosticRawLineCount
        hardDiagnosticEventCount =
            [int]$classification.hardDiagnosticEventCount
        approvedStockDiagnosticClusterPresent =
            [bool]$classification.approvedStockDiagnosticClusterPresent
        approvedStockDiagnosticClusterExact =
            [bool]$classification.approvedStockDiagnosticClusterExact
        approvedStockDiagnosticLifecycleExact =
            [bool]$classification.approvedStockDiagnosticLifecycleExact
        approvedStockDiagnosticRawLineCount =
            [int]$classification.approvedStockDiagnosticRawLineCount
        approvedStockDiagnosticEventCount =
            [int]$classification.approvedStockDiagnosticEventCount
        unapprovedHardDiagnosticRawLineCount =
            [int]$classification.unapprovedHardDiagnosticRawLineCount
        unapprovedHardDiagnosticEventCount =
            [int]$classification.unapprovedHardDiagnosticEventCount
        hardDiagnosticAccountingExact =
            [bool]$classification.hardDiagnosticAccountingExact
        candidateMountAttestation = $classification.candidateMountAttestation
        harnessMountLineCount = [int]$classification.harnessMountLineCount
        uniqueResultMarkerCount = [int]$classification.uniqueResultMarkerCount
        resultMarkerOccurrenceCount =
            [int]$classification.resultMarkerOccurrenceCount
        crashLogContentValid = [bool]$classification.crashLogContentValid
        crashArtifactCount = [int]$classification.crashArtifactCount
        passed = $true
    }
}

function Assert-ReleaseSurfaceIndexEvidenceCensus {
    param([string]$RunRoot, $Run)

    Assert-ReleaseSurfaceIndexExactProperties $Run.evidenceIndex @(
        'path', 'signature', 'aggregateSha256', 'fileCount') `
        'Release-surface evidence-index binding'
    Assert-ReleaseSurfaceIndexStringProperties $Run.evidenceIndex @(
        'path', 'aggregateSha256') 'Release-surface evidence-index binding'
    Assert-ReleaseSurfaceIndexIntegerProperties $Run.evidenceIndex @(
        'fileCount') 'Release-surface evidence-index binding'
    if ([string]$Run.evidenceIndex.path -cne 'evidence-index.json') {
        throw 'Release-surface evidence index is not the fixed sibling.'
    }
    Assert-ReleaseSurfaceIndexSignature $Run.evidenceIndex.signature `
        'Release-surface evidence-index signature'
    $path = Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
        ([string]$Run.evidenceIndex.path) 'Release-surface evidence-index path'
    if (-not (Test-ReleaseSurfaceIndexSignatureExact `
            $Run.evidenceIndex.signature (Get-ReleaseSurfaceIndexFileSignature $path))) {
        throw 'Release-surface evidence index differs from its run binding.'
    }
    $parsed = Read-ReleaseSurfaceIndexJsonArtifact `
        $path 'Release-surface evidence index' -Portable
    $index = $parsed.Value
    Assert-ReleaseSurfaceIndexExactProperties $index @(
        'schemaVersion', 'evidenceKind', 'files', 'aggregateSha256') `
        'Release-surface evidence index'
    Assert-ReleaseSurfaceIndexIntegerProperties $index @(
        'schemaVersion') 'Release-surface evidence index'
    Assert-ReleaseSurfaceIndexStringProperties $index @(
        'evidenceKind', 'aggregateSha256') 'Release-surface evidence index'
    if ([int]$index.schemaVersion -ne 1 -or
        [string]$index.evidenceKind -cne $script:EvidenceKind) {
        throw 'Release-surface evidence index identity is invalid.'
    }
    $rows = @($index.files)
    if ($rows.Count -ne [int]$Run.evidenceIndex.fileCount -or
        [string]$index.aggregateSha256 -cne
            [string]$Run.evidenceIndex.aggregateSha256) {
        throw 'Release-surface evidence index summary is invalid.'
    }
    $rowPaths = @($rows | ForEach-Object { [string]$_.path })
    $sortedRowPaths = @($rowPaths | Sort-Object)
    if (-not (Test-ReleaseSurfaceIndexStringArrayExact `
            $sortedRowPaths $rowPaths)) {
        throw 'Release-surface evidence file rows are not canonically ordered.'
    }
    $seen = New-Object Collections.Generic.HashSet[string]([StringComparer]::Ordinal)
    foreach ($row in $rows) {
        Assert-ReleaseSurfaceIndexExactProperties $row @(
            'path', 'length', 'sha256') 'Release-surface evidence file row'
        Assert-ReleaseSurfaceIndexStringProperties $row @(
            'path', 'sha256') 'Release-surface evidence file row'
        Assert-ReleaseSurfaceIndexIntegerProperties $row @(
            'length') 'Release-surface evidence file row'
        if (-not $seen.Add([string]$row.path) -or
            [long]$row.length -lt 0 -or
            [string]$row.sha256 -cnotmatch '^[0-9a-f]{64}$') {
            throw 'Release-surface evidence file row is invalid or duplicated.'
        }
        $file = Resolve-ReleaseSurfaceIndexPortableFile $RunRoot `
            ([string]$row.path) 'Release-surface evidence file path'
        if (-not (Test-Path -LiteralPath $file -PathType Leaf) -or
            -not (Test-ReleaseSurfaceIndexSignatureExact `
                $row (Get-ReleaseSurfaceIndexFileSignature $file))) {
            throw 'A release-surface evidence file differs from the census.'
        }
    }
    if ((Get-ReleaseSurfaceIndexCanonicalRowsDigest $rows) -cne
        [string]$index.aggregateSha256) {
        throw 'Release-surface evidence aggregate digest is invalid.'
    }
    $controlFiles = @(
        'evidence-index.json', 'run.json', 'release-index.json',
        'run.ready.json', 'run.failure.json')
    $actual = @(Get-ChildItem -LiteralPath $RunRoot -Recurse -File -Force |
        ForEach-Object {
            ConvertTo-ReleaseSurfaceIndexPortableRelativePath `
                $RunRoot $_.FullName 'Release-surface census path'
        } | Where-Object { $_ -notin $controlFiles } | Sort-Object)
    $expected = @($rows | ForEach-Object { [string]$_.path } | Sort-Object)
    if (@(Compare-Object $expected $actual -CaseSensitive).Count -ne 0) {
        throw 'Release-surface run file census is not complete and exact.'
    }
    return [pscustomobject][ordered]@{
        path = 'evidence-index.json'
        length = [long]$parsed.Artifact.Length
        sha256 = [string]$parsed.Artifact.Sha256
        fileCount = $rows.Count
        files = [object[]]$rows
        filesAggregateSha256 = [string]$index.aggregateSha256
    }
}

function Get-ReleaseSurfaceIndexCanonicalJsonBytes {
    param($Value)

    $json = (($Value | ConvertTo-Json -Depth 64).Replace("`r`n", "`n") +
        "`n")
    Assert-ReleaseSurfaceIndexNoLocalPathText $json 'Release-surface audit index'
    return (New-Object Text.UTF8Encoding($false)).GetBytes($json)
}

function Assert-ReleaseSurfacePublishedIndexAndReady {
    param(
        [string]$RunRoot,
        $CanonicalIndex,
        $Run,
        $RunArtifact,
        $EvidenceCensus
    )

    $indexPath = Join-Path $RunRoot 'release-index.json'
    $published = Read-ReleaseSurfaceIndexJsonArtifact `
        $indexPath 'Published release-surface audit index' -Portable
    $canonicalBytes = Get-ReleaseSurfaceIndexCanonicalJsonBytes $CanonicalIndex
    if ($published.Artifact.Length -ne $canonicalBytes.Length -or
        $published.Artifact.Sha256 -cne
            (Get-ReleaseSurfaceIndexSha256Bytes $canonicalBytes)) {
        throw 'Published release-surface audit index is not the exact canonical index.'
    }

    $readyPath = Join-Path $RunRoot 'run.ready.json'
    $readyParsed = Read-ReleaseSurfaceIndexJsonArtifact `
        $readyPath 'Published release-surface ready seal' -Portable
    $ready = $readyParsed.Value
    Assert-ReleaseSurfaceIndexExactProperties $ready @(
        'schemaVersion', 'evidenceKind', 'disposition',
        'certificationPromotion', 'runId', 'runLeafId', 'source', 'candidateId',
        'packageSha256', 'manifestSha256', 'readySha256', 'candidate',
        'candidateBindingSha256', 'run', 'evidenceIndex', 'releaseIndex',
        'cleanupPassed', 'sealedLast') 'Published release-surface ready seal'
    Assert-ReleaseSurfaceIndexIntegerProperties $ready @(
        'schemaVersion') 'Published release-surface ready seal'
    Assert-ReleaseSurfaceIndexStringProperties $ready @(
        'evidenceKind', 'disposition', 'certificationPromotion', 'runId',
        'runLeafId', 'candidateId', 'packageSha256', 'manifestSha256',
        'readySha256', 'candidateBindingSha256') `
        'Published release-surface ready seal'
    Assert-ReleaseSurfaceIndexBooleanProperties $ready @(
        'cleanupPassed', 'sealedLast') 'Published release-surface ready seal'
    Assert-ReleaseSurfaceIndexExactProperties $ready.source @(
        'candidateGitHead', 'harnessGitHead') 'Published ready source binding'
    Assert-ReleaseSurfaceIndexStringProperties $ready.source @(
        'candidateGitHead', 'harnessGitHead') 'Published ready source binding'

    if ([int]$ready.schemaVersion -ne 2 -or
        [string]$ready.evidenceKind -cne $script:EvidenceKind -or
        [string]$ready.disposition -cne
            'passed-noncertifying-release-surface-audit' -or
        [string]$ready.certificationPromotion -cne 'none' -or
        [string]$ready.runId -cne [string]$Run.runId -or
        [string]$ready.runLeafId -cne [string]$Run.runLeafId -or
        [string]$ready.source.candidateGitHead -cne [string]$Run.candidate.gitHead -or
        [string]$ready.source.harnessGitHead -cne
            [string]$Run.source.harnessGitHead -or
        [string]$ready.candidateId -cne [string]$Run.candidate.candidateId -or
        [string]$ready.packageSha256 -cne [string]$Run.candidate.packageSha256 -or
        [string]$ready.manifestSha256 -cne [string]$Run.candidate.manifestSha256 -or
        [string]$ready.readySha256 -cne [string]$Run.candidate.readySha256 -or
        [string]$ready.candidateBindingSha256 -cne
            [string]$Run.candidateBindingSha256 -or
        -not [bool]$ready.cleanupPassed -or -not [bool]$ready.sealedLast) {
        throw 'Published release-surface ready seal is not bound to the successful run.'
    }
    Assert-ReleaseSurfaceIndexCandidate $ready.candidate `
        'Published ready candidate'
    Assert-ReleaseSurfaceIndexCandidatesEqual $Run.candidate $ready.candidate `
        'Published ready candidate'

    $bindings = @(
        [pscustomobject]@{
            Label = 'Published ready run'
            Value = $ready.run
            ExpectedPath = 'run.json'
            ActualSignature = [pscustomobject]@{
                length = [long]$RunArtifact.Length
                sha256 = [string]$RunArtifact.Sha256
            }
        },
        [pscustomobject]@{
            Label = 'Published ready evidence index'
            Value = $ready.evidenceIndex
            ExpectedPath = 'evidence-index.json'
            ActualSignature = [pscustomobject]@{
                length = [long]$EvidenceCensus.length
                sha256 = [string]$EvidenceCensus.sha256
            }
        },
        [pscustomobject]@{
            Label = 'Published ready release index'
            Value = $ready.releaseIndex
            ExpectedPath = 'release-index.json'
            ActualSignature = [pscustomobject]@{
                length = [long]$published.Artifact.Length
                sha256 = [string]$published.Artifact.Sha256
            }
        })
    foreach ($binding in $bindings) {
        Assert-ReleaseSurfaceIndexExactProperties $binding.Value @(
            'path', 'signature') $binding.Label
        Assert-ReleaseSurfaceIndexStringProperties $binding.Value @(
            'path') $binding.Label
        Assert-ReleaseSurfaceIndexSignature $binding.Value.signature `
            ($binding.Label + ' signature')
        if ([string]$binding.Value.path -cne [string]$binding.ExpectedPath -or
            -not (Test-ReleaseSurfaceIndexSignatureExact `
                $binding.Value.signature $binding.ActualSignature)) {
            throw "$($binding.Label) path or signature is invalid."
        }
    }
    return [pscustomobject][ordered]@{
        IndexArtifact = $published.Artifact
        ReadyArtifact = $readyParsed.Artifact
    }
}

$runParsed = Read-ReleaseSurfaceIndexJsonArtifact `
    $RunEnvelopePath 'Release-surface run envelope' -Portable
$run = $runParsed.Value
$runRoot = Split-Path -Parent $runParsed.Artifact.Path
$fixedOutput = [IO.Path]::GetFullPath((Join-Path $runRoot 'release-index.json'))
$synthetic = [bool]$SyntheticFixtureValidationOnly
$verifyPublished = [bool]$VerifyPublishedIndex
$outputFull = $null
if ($verifyPublished) {
    if ($PSBoundParameters.ContainsKey('OutputPath')) {
        throw 'Published release-surface verification cannot accept an output path.'
    }
    $outputFull = $fixedOutput
    if (-not (Test-Path -LiteralPath $fixedOutput -PathType Leaf) -or
        -not (Test-Path -LiteralPath (Join-Path $runRoot 'run.ready.json') `
            -PathType Leaf)) {
        throw 'Published release-surface verification requires the index and ready seal.'
    }
    if (Test-Path -LiteralPath (Join-Path $runRoot 'run.failure.json')) {
        throw 'Published release-surface verification rejects a failure seal.'
    }
}
elseif ($synthetic) {
    if ($PSBoundParameters.ContainsKey('OutputPath')) {
        throw 'Synthetic fixture validation cannot accept an output path.'
    }
    if (Test-Path -LiteralPath $fixedOutput) {
        throw 'Synthetic fixture validation requires an unpublished fixture.'
    }
}
else {
    if ([string]::IsNullOrWhiteSpace($OutputPath)) {
        $OutputPath = $fixedOutput
    }
    $outputFull = [IO.Path]::GetFullPath($OutputPath)
    if (-not $outputFull.Equals(
            $fixedOutput, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'The release-surface audit index must be the fixed run-root sibling.'
    }
}
if (-not $verifyPublished) {
    foreach ($terminalLeaf in @('run.ready.json', 'run.failure.json')) {
        if (Test-Path -LiteralPath (Join-Path $runRoot $terminalLeaf)) {
            throw 'Release-surface index publication must precede every terminal seal.'
        }
    }
}

$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$guardedModulePath = Join-Path $PSScriptRoot 'Partisan.GuardedRuntime.psm1'
Import-Module -Name $guardedModulePath -Force -ErrorAction Stop
Assert-PartisanNoReparseTree -Root $runRoot

Assert-ReleaseSurfaceIndexExactProperties $run @(
    'schemaVersion', 'evidenceKind', 'contractId', 'runId', 'runLeafId',
    'runNonce', 'startedUtc', 'completedUtc', 'disposition',
    'certificationPromotion', 'source', 'candidate',
    'candidateBindingSha256', 'pairedSamePackage', 'candidatePackageSha256',
    'harnessGuid', 'harnessSha256', 'modes', 'cleanup', 'evidenceIndex',
    'limitations', 'passed') 'Release-surface run envelope'
Assert-ReleaseSurfaceIndexExactProperties $run.source @(
    'harnessGitHead', 'checkoutClean', 'candidateAncestor', 'executionMode') `
    'Release-surface run source'
Assert-ReleaseSurfaceIndexIntegerProperties $run @(
    'schemaVersion') 'Release-surface run envelope'
Assert-ReleaseSurfaceIndexStringProperties $run @(
    'evidenceKind', 'contractId', 'runId', 'runLeafId', 'runNonce', 'startedUtc',
    'completedUtc', 'disposition', 'certificationPromotion',
    'candidateBindingSha256', 'candidatePackageSha256', 'harnessGuid',
    'harnessSha256') 'Release-surface run envelope'
Assert-ReleaseSurfaceIndexBooleanProperties $run @(
    'pairedSamePackage', 'passed') 'Release-surface run envelope'
Assert-ReleaseSurfaceIndexStringProperties $run.source @(
    'harnessGitHead', 'executionMode') 'Release-surface run source'
Assert-ReleaseSurfaceIndexBooleanProperties $run.source @(
    'checkoutClean', 'candidateAncestor') 'Release-surface run source'
Assert-ReleaseSurfaceIndexStringArray $run.limitations `
    'Release-surface run limitations'
$executionMode = [string]$run.source.executionMode
if ($synthetic) {
    if ($executionMode -cne 'synthetic-self-test') {
        throw 'Synthetic fixture validation requires a synthetic fixture run.'
    }
}
else {
    if ($executionMode -ceq 'synthetic-self-test') {
        throw 'Synthetic release-surface fixtures cannot be published or production-verified.'
    }
    if ($executionMode -cne 'paired-native-engine-audit') {
        throw 'Release-surface publication requires a paired native-engine audit.'
    }
}
if ([int]$run.schemaVersion -ne 2 -or
    [string]$run.evidenceKind -cne $script:EvidenceKind -or
    [string]$run.contractId -cne $script:RunContractId -or
    [string]$run.runId -cnotmatch
        '^release_surface_[0-9]{8}T[0-9]{6}Z_[0-9a-f]{20}$' -or
    [string]$run.runLeafId -cnotmatch
        '^[0-9]{8}T[0-9]{6}Z-[0-9a-f]{32}$' -or
    [string]$run.runNonce -cnotmatch '^[0-9a-f]{32}$' -or
    -not [string]$run.runLeafId.EndsWith(
        '-' + [string]$run.runNonce, [StringComparison]::Ordinal) -or
    [string]$run.startedUtc -cnotmatch '^\d{4}-\d{2}-\d{2}T' -or
    [string]$run.completedUtc -cnotmatch '^\d{4}-\d{2}-\d{2}T' -or
    [datetime]$run.completedUtc -lt [datetime]$run.startedUtc -or
    [string]$run.disposition -cne
        'passed-noncertifying-release-surface-audit' -or
    [string]$run.certificationPromotion -cne 'none' -or
    [string]$run.source.harnessGitHead -cnotmatch '^[0-9a-f]{40}$' -or
    -not [bool]$run.source.checkoutClean -or
    -not [bool]$run.source.candidateAncestor -or
    -not [bool]$run.pairedSamePackage -or
    -not [bool]$run.passed) {
    throw 'Release-surface run envelope identity or outcome is invalid.'
}
if ([string]$run.runId -cne (
        'release_surface_' + [string]$run.runLeafId.Substring(0, 16) + '_' +
        [string]$run.runNonce.Substring(0, 20))) {
    throw 'Release-surface run ID is not nonce- and leaf-bound.'
}
if ((Split-Path -Leaf $runRoot) -cne [string]$run.runLeafId -or
    (Split-Path -Leaf (Split-Path -Parent (Split-Path -Parent $runRoot))) -cne
        [string]$run.candidate.candidateId) {
    throw 'Release-surface run directory does not match its portable identity.'
}
Assert-ReleaseSurfaceIndexCandidate $run.candidate 'Release-surface candidate'
if ([string]$run.candidatePackageSha256 -cne
        [string]$run.candidate.packageSha256 -or
    [string]$run.candidateBindingSha256 -cnotmatch '^[0-9a-f]{64}$' -or
    [string]$run.harnessGuid -cnotmatch '^[0-9A-F]{16}$' -or
    [string]$run.harnessGuid -ceq [string]$run.candidate.addonGuid -or
    [string]$run.harnessSha256 -cnotmatch '^[0-9a-f]{64}$' -or
    -not (Test-ReleaseSurfaceIndexStringArrayExact `
        ([object[]]$script:Limitations) ([object[]]$run.limitations))) {
    throw 'Release-surface run package, harness, or limitation binding is invalid.'
}

$bindingsParsed = Read-ReleaseSurfaceIndexJsonArtifact `
    (Join-Path $runRoot 'identity\bindings.json') `
    'Release-surface identity bindings' -Portable
$bindings = $bindingsParsed.Value
Assert-ReleaseSurfaceIndexExactProperties $bindings @(
    'schemaVersion', 'evidenceKind', 'source', 'candidate', 'package',
    'executables', 'harness', 'tools', 'host') `
    'Release-surface identity bindings'
Assert-ReleaseSurfaceIndexIntegerProperties $bindings @(
    'schemaVersion') 'Release-surface identity bindings'
Assert-ReleaseSurfaceIndexStringProperties $bindings @(
    'evidenceKind') 'Release-surface identity bindings'
if ([int]$bindings.schemaVersion -ne 1 -or
    [string]$bindings.evidenceKind -cne $script:EvidenceKind) {
    throw 'Release-surface identity binding header is invalid.'
}
Assert-ReleaseSurfaceIndexExactProperties $bindings.host @(
    'powershellVersion', 'powershellEdition', 'operatingSystem') `
    'Release-surface host binding'
Assert-ReleaseSurfaceIndexStringProperties $bindings.host @(
    'powershellVersion', 'powershellEdition', 'operatingSystem') `
    'Release-surface host binding'
if ([string]::IsNullOrWhiteSpace([string]$bindings.host.powershellVersion) -or
    [string]::IsNullOrWhiteSpace([string]$bindings.host.powershellEdition) -or
    [string]::IsNullOrWhiteSpace([string]$bindings.host.operatingSystem)) {
    throw 'Release-surface host binding is incomplete.'
}
Assert-ReleaseSurfaceIndexCandidate $bindings.candidate `
    'Bound release-surface candidate'
Assert-ReleaseSurfaceIndexCandidatesEqual `
    $run.candidate $bindings.candidate 'Bound release-surface candidate'
Assert-ReleaseSurfaceIndexExactProperties $bindings.executables @(
    'retail', 'diagnostic') 'Release-surface executable bindings'
Assert-ReleaseSurfaceIndexProvenance `
    $bindings.executables.retail 'ArmaReforgerServer.exe' `
    'Retail executable binding'
Assert-ReleaseSurfaceIndexProvenance `
    $bindings.executables.diagnostic 'ArmaReforgerServerDiag.exe' `
    'Diagnostic executable binding'
Assert-ReleaseSurfaceIndexValuesEqual `
    $run.candidate.recordedRuntimeExecutable $bindings.executables.retail `
    @('fileName', 'fileVersion', 'productVersion', 'length', 'sha256') `
    'Retail executable binding'
Assert-ReleaseSurfaceIndexValuesEqual `
    $run.candidate.recordedDiagnosticExecutable $bindings.executables.diagnostic `
    @('fileName', 'fileVersion', 'productVersion', 'length', 'sha256') `
    'Diagnostic executable binding'
if ([string]$bindings.executables.retail.productVersion -cne
        [string]$bindings.executables.diagnostic.productVersion -or
    [string]$bindings.executables.retail.fileVersion -cne
        [string]$bindings.executables.diagnostic.fileVersion) {
    throw 'Retail and diagnostic executable version provenance is not paired.'
}

$manifestParsed = Read-ReleaseSurfaceIndexJsonArtifact `
    (Join-Path $runRoot 'identity\candidate.json') `
    'Copied candidate manifest' -Portable
$readyParsed = Read-ReleaseSurfaceIndexJsonArtifact `
    (Join-Path $runRoot 'identity\candidate.ready.json') `
    'Copied candidate ready seal' -Portable
Assert-ReleaseSurfaceIndexPackageAndManifest `
    $run.candidate $bindings $manifestParsed.Value $readyParsed.Value `
    $manifestParsed.Artifact $readyParsed.Artifact
foreach ($executableBinding in @(
        [pscustomobject]@{
            Label = 'retail'
            Manifest = $manifestParsed.Value.toolchain.server
            Binding = $bindings.executables.retail
        },
        [pscustomobject]@{
            Label = 'diagnostic'
            Manifest = $manifestParsed.Value.toolchain.serverDiagnostic
            Binding = $bindings.executables.diagnostic
        })) {
    Assert-ReleaseSurfaceIndexProvenance `
        $executableBinding.Manifest ([string]$executableBinding.Binding.fileName) `
        "$($executableBinding.Label) manifest executable"
    Assert-ReleaseSurfaceIndexValuesEqual `
        $executableBinding.Manifest $executableBinding.Binding `
        @('fileName', 'fileVersion', 'productVersion', 'length', 'sha256') `
        "$($executableBinding.Label) manifest executable"
}

$contractParsed = Read-ReleaseSurfaceIndexJsonArtifact `
    (Join-Path $runRoot 'identity\release_surface_contract.json') `
    'Copied release-surface contract' -Portable
$contract = $contractParsed.Value
Assert-ReleaseSurfaceIndexContract $contract
$memberProbeSourceCommit = [string]$run.candidate.gitHead
if ($synthetic) {
    $memberProbeSourceCommit = ''
}
$memberProbePlan = Get-ReleaseSurfaceIndexMemberProbePlan `
    -RepositoryRoot $repositoryRoot `
    -Contract $contract `
    -SourceCommit $memberProbeSourceCommit
$contractTool = @($bindings.tools | Where-Object {
    [string]$_.role -ceq 'contract'
})
if ($contractTool.Count -ne 1 -or
    -not (Test-ReleaseSurfaceIndexSignatureExact `
        $contractTool[0] (Get-ReleaseSurfaceIndexFileSignature `
            (Join-Path $runRoot 'identity\release_surface_contract.json')))) {
    throw 'Copied release-surface contract differs from its tool binding.'
}
Assert-ReleaseSurfaceIndexHarnessAndTools `
    $runRoot $repositoryRoot $run $bindings $synthetic $verifyPublished

$owner = (Read-ReleaseSurfaceIndexJsonArtifact `
    (Join-Path $runRoot 'run.owner.json') 'Release-surface run owner' -Portable).Value
Assert-ReleaseSurfaceIndexExactProperties $owner @(
    'schemaVersion', 'evidenceKind', 'runId', 'runNonce', 'candidateId',
    'packageSha256', 'harnessGuid', 'harnessSha256', 'disposition') `
    'Release-surface run owner'
Assert-ReleaseSurfaceIndexIntegerProperties $owner @(
    'schemaVersion') 'Release-surface run owner'
Assert-ReleaseSurfaceIndexStringProperties $owner @(
    'evidenceKind', 'runId', 'runNonce', 'candidateId', 'packageSha256',
    'harnessGuid', 'harnessSha256', 'disposition') 'Release-surface run owner'
if ([int]$owner.schemaVersion -ne 1 -or
    [string]$owner.evidenceKind -cne $script:EvidenceKind -or
    [string]$owner.runId -cne [string]$run.runId -or
    [string]$owner.runNonce -cne [string]$run.runNonce -or
    [string]$owner.candidateId -cne [string]$run.candidate.candidateId -or
    [string]$owner.packageSha256 -cne [string]$run.candidate.packageSha256 -or
    [string]$owner.harnessGuid -cne [string]$run.harnessGuid -or
    [string]$owner.harnessSha256 -cne [string]$run.harnessSha256 -or
    [string]$owner.disposition -cne
        'in-progress-noncertifying-release-surface-audit') {
    throw 'Release-surface run owner is not bound to the run.'
}

Assert-ReleaseSurfaceIndexExactProperties $run.cleanup @(
    'schemaVersion', 'evidenceKind', 'harnessRemoved', 'harnessResidueCount',
    'exactOwnerVerifiedBeforeRemoval', 'passed') 'Release-surface cleanup'
Assert-ReleaseSurfaceIndexIntegerProperties $run.cleanup @(
    'schemaVersion', 'harnessResidueCount') 'Release-surface cleanup'
Assert-ReleaseSurfaceIndexStringProperties $run.cleanup @(
    'evidenceKind') 'Release-surface cleanup'
Assert-ReleaseSurfaceIndexBooleanProperties $run.cleanup @(
    'harnessRemoved', 'exactOwnerVerifiedBeforeRemoval', 'passed') `
    'Release-surface cleanup'
if ([int]$run.cleanup.schemaVersion -ne 1 -or
    [string]$run.cleanup.evidenceKind -cne $script:EvidenceKind -or
    -not [bool]$run.cleanup.harnessRemoved -or
    [int]$run.cleanup.harnessResidueCount -ne 0 -or
    -not [bool]$run.cleanup.exactOwnerVerifiedBeforeRemoval -or
    -not [bool]$run.cleanup.passed) {
    throw 'Release-surface cleanup is not exact and residue-free.'
}

$modeBindings = @($run.modes)
if ($modeBindings.Count -ne 2) {
    throw 'Release-surface audit requires exactly two mode envelopes.'
}
$validatedModes = New-Object Collections.Generic.List[object]
for ($index = 0; $index -lt $script:Modes.Count; $index++) {
    $mode = $script:Modes[$index]
    Assert-ReleaseSurfaceIndexExactProperties $modeBindings[$index] @(
        'mode', 'path', 'signature') "$mode run-mode binding"
    [void]$validatedModes.Add((Assert-ReleaseSurfaceIndexMode `
        $mode $runRoot $run $bindings $contract $memberProbePlan `
        $modeBindings[$index] $synthetic))
}
$candidateBindings = @($validatedModes.ToArray() | ForEach-Object {
    [string]$_.candidateBindingSha256
} | Sort-Object -Unique)
if ($candidateBindings.Count -ne 1 -or
    [string]$candidateBindings[0] -cne [string]$run.candidateBindingSha256) {
    throw 'Retail and diagnostic modes do not share the run candidate binding.'
}
$contextIds = @($validatedModes.ToArray() | ForEach-Object {
    [string]$_.contextId
} | Sort-Object -Unique)
if ($contextIds.Count -ne 2) {
    throw 'Retail and diagnostic modes do not have distinct guarded contexts.'
}
$globalCrashArtifacts = @(Get-ChildItem -LiteralPath $runRoot -Recurse -File `
    -Force -ErrorAction Stop | Where-Object {
        $_.Extension -in @('.dmp', '.mdmp') -or
        $_.Name -match '(?i)^minidump'
    })
if ($globalCrashArtifacts.Count -ne 0) {
    throw 'Release-surface run contains a crash artifact.'
}

$census = Assert-ReleaseSurfaceIndexEvidenceCensus $runRoot $run
$index = [ordered]@{
    schemaVersion = 2
    contractId = $script:IndexContractId
    evidenceKind = $script:IndexEvidenceKind
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
        length = [long]$runParsed.Artifact.Length
        sha256 = [string]$runParsed.Artifact.Sha256
    }
    evidenceIndex = [ordered]@{
        path = [string]$census.path
        length = [long]$census.length
        sha256 = [string]$census.sha256
        fileCount = [int]$census.fileCount
        filesAggregateSha256 = [string]$census.filesAggregateSha256
    }
    contract = [ordered]@{
        path = 'identity/release_surface_contract.json'
        length = [long]$contractParsed.Artifact.Length
        sha256 = [string]$contractParsed.Artifact.Sha256
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
        modeCount = $validatedModes.Count
        pairedOrder = [string[]]$script:Modes
    }
    modes = [object[]]$validatedModes.ToArray()
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
    files = [object[]]$census.files
    filesAggregateSha256 = [string]$census.filesAggregateSha256
    limitations = [string[]]$script:Limitations
    disposition = 'passed-noncertifying-release-surface-audit'
    certificationPromotion = 'none'
    passed = $true
}
if ($verifyPublished) {
    $verified = Assert-ReleaseSurfacePublishedIndexAndReady `
        $runRoot $index $run $runParsed.Artifact $census
    Write-Output ([pscustomobject][ordered]@{
        ValidationKind =
            'partisan_release_surface_published_index_verification_v1'
        PublishedIndexValid = $true
        ReadySealValid = $true
        ReadOnlyVerification = $true
        SyntheticFixture = $synthetic
        RunId = [string]$run.runId
        CandidateId = [string]$run.candidate.candidateId
        CandidateBindingSha256 = [string]$run.candidateBindingSha256
        FileCount = [int]$census.fileCount
        IndexSignature = [pscustomobject][ordered]@{
            length = [long]$verified.IndexArtifact.Length
            sha256 = [string]$verified.IndexArtifact.Sha256
        }
        ReadySignature = [pscustomobject][ordered]@{
            length = [long]$verified.ReadyArtifact.Length
            sha256 = [string]$verified.ReadyArtifact.Sha256
        }
        Disposition = 'passed-noncertifying-release-surface-audit'
    })
    return
}
if ($synthetic) {
    if ((Test-Path -LiteralPath $fixedOutput) -or
        (Test-Path -LiteralPath (Join-Path $runRoot 'run.ready.json'))) {
        throw 'Synthetic fixture validation created a forbidden release artifact.'
    }
    Write-Output ([pscustomobject][ordered]@{
        ValidationKind =
            'partisan_release_surface_synthetic_fixture_validation_v1'
        FixtureValid = $true
        ArtifactPublished = $false
        RunId = [string]$run.runId
        CandidateId = [string]$run.candidate.candidateId
        CandidateBindingSha256 = [string]$run.candidateBindingSha256
        EvidenceFileCount = [int]$census.fileCount
    })
    return
}
foreach ($terminalLeaf in @('run.ready.json', 'run.failure.json')) {
    if (Test-Path -LiteralPath (Join-Path $runRoot $terminalLeaf)) {
        throw 'Release-surface index publication must precede every terminal seal.'
    }
}
$signature = Write-ReleaseSurfaceIndexAtomicCreateOnly `
    -Path $outputFull `
    -Value $index
Write-Output ([pscustomobject][ordered]@{
    Path = $outputFull
    Signature = $signature
    RunId = [string]$run.runId
    CandidateId = [string]$run.candidate.candidateId
    CandidateBindingSha256 = [string]$run.candidateBindingSha256
    FileCount = [int]$census.fileCount
    Disposition = 'passed-noncertifying-release-surface-audit'
})
