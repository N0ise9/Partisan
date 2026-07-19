[CmdletBinding(DefaultParameterSetName = 'Run')]
param(
    [Parameter(Mandatory = $true, ParameterSetName = 'Run')]
    [ValidateNotNullOrEmpty()]
    [string]$CandidateManifest,

    [Parameter(Mandatory = $true, ParameterSetName = 'Run')]
    [ValidateNotNullOrEmpty()]
    [string]$CampaignDebugRun,

    [Parameter(ParameterSetName = 'Run')]
    [string[]]$ExternalEvidence = @(),

    [Parameter(Mandatory = $true, ParameterSetName = 'Run')]
    [ValidateNotNullOrEmpty()]
    [string]$OutputPath,

    [Parameter(Mandatory = $true, ParameterSetName = 'SelfTest')]
    [switch]$SelfTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:CandidateManifestSchema = 1
$script:CandidateReadySchema = 1
$script:PackagedRunSchema = 1
$script:ExternalEvidenceSchema = 1
$script:CompositeCertificationSchema = 1
$script:PackageHashAlgorithm = 'sha256-manifest-v1'
$script:ExternalEvidenceKind = 'partisan-external-certification-proof'
$script:CompositeCertificationKind = 'partisan-composite-certification'
$script:PackagedRunEvidenceKind = 'packaged-campaign-debug'
$script:CampaignDebugProfile = 'full_certification'
$script:CampaignDebugProofScope = 'full_certification'
$script:ManualGapAssertionOrder = @(
    'persistence.real_restart',
    'phase25.real_restart',
    'phase25.second_client',
    'phase25.two_hour_soak'
)
$script:ScenarioOrder = @(
    'broad_restart',
    'second_client_jip_reconnect',
    'two_hour_soak'
)
$script:ScenarioContracts = [ordered]@{
    broad_restart = [pscustomobject][ordered]@{
        ContractVersion = 1
        AssertionIds = @(
            'persistence.real_restart',
            'phase25.real_restart'
        )
        MinimumDurationSeconds = 60
        MinimumClientCount = 1
    }
    second_client_jip_reconnect = [pscustomobject][ordered]@{
        ContractVersion = 1
        AssertionIds = @('phase25.second_client')
        MinimumDurationSeconds = 60
        MinimumClientCount = 2
    }
    two_hour_soak = [pscustomobject][ordered]@{
        ContractVersion = 1
        AssertionIds = @('phase25.two_hour_soak')
        MinimumDurationSeconds = 7200
        MinimumClientCount = 2
    }
}

function Get-PartisanCompositeSha256 {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (Get-FileHash `
        -LiteralPath $Path `
        -Algorithm SHA256 `
        -ErrorAction Stop).Hash.ToLowerInvariant()
}

function Get-PartisanCompositeTextSha256 {
    param(
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)]
        [string]$Text
    )

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [Text.Encoding]::UTF8.GetBytes($Text)
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $algorithm.Dispose()
    }
}

function Write-PartisanCompositeUtf8File {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [AllowEmptyString()][Parameter(Mandatory = $true)][string]$Text
    )

    $encoding = New-Object Text.UTF8Encoding($false)
    [IO.File]::WriteAllText($Path, $Text, $encoding)
}

function Resolve-PartisanCompositeExistingFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $resolved = [IO.Path]::GetFullPath($Path)
    if (-not (Test-Path -LiteralPath $resolved -PathType Leaf)) {
        throw "$Label does not exist."
    }
    return $resolved
}

function Assert-PartisanCompositeNoReparseAncestry {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $cursor = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    while ($true) {
        $item = Get-Item -LiteralPath $cursor -Force -ErrorAction Stop
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "$Label must not use a reparse point."
        }
        if ($cursor.Equals($rootFull, [StringComparison]::OrdinalIgnoreCase)) {
            break
        }
        $parent = Split-Path -Parent $cursor
        if ([string]::IsNullOrWhiteSpace($parent) -or
            $parent.Equals($cursor, [StringComparison]::OrdinalIgnoreCase)) {
            throw "$Label is outside its evidence root."
        }
        $cursor = $parent
    }
}

function Assert-PartisanCompositeProperties {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string[]]$Names,
        [Parameter(Mandatory = $true)][string]$Label,
        [switch]$Exact
    )

    if ($null -eq $Value) {
        throw "$Label is missing."
    }
    $actualNames = @($Value.PSObject.Properties.Name)
    foreach ($name in $Names) {
        if ($actualNames -cnotcontains $name) {
            throw "$Label is missing required property $name."
        }
    }
    if ($Exact) {
        $difference = @(Compare-Object `
            -ReferenceObject @($Names | Sort-Object) `
            -DifferenceObject @($actualNames | Sort-Object) `
            -CaseSensitive)
        if ($difference.Count -ne 0) {
            throw "$Label contains an unexpected property."
        }
    }
}

function Assert-PartisanCompositeBoolean {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($Value -isnot [bool]) {
        throw "$Label must be a JSON boolean."
    }
    return [bool]$Value
}

function Get-PartisanCompositeInt64 {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string]$Label,
        [long]$Minimum = [long]::MinValue
    )

    if ($Value -isnot [sbyte] -and
        $Value -isnot [byte] -and
        $Value -isnot [int16] -and
        $Value -isnot [uint16] -and
        $Value -isnot [int32] -and
        $Value -isnot [uint32] -and
        $Value -isnot [int64] -and
        $Value -isnot [uint64]) {
        throw "$Label must be a JSON integer."
    }
    $result = [long]$Value
    if ($result -lt $Minimum) {
        throw "$Label is below its minimum value."
    }
    return $result
}

function Assert-PartisanCompositeSha256 {
    param(
        [Parameter(Mandatory = $true)][string]$Value,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($Value -cnotmatch '^[0-9a-f]{64}$') {
        throw "$Label must be a lowercase SHA-256 digest."
    }
    return $Value
}

function Assert-PartisanCompositeSafeId {
    param(
        [Parameter(Mandatory = $true)][string]$Value,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($Value -cnotmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$') {
        throw "$Label is not a portable identifier."
    }
    return $Value
}

function Assert-PartisanCompositePortablePath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or
        [IO.Path]::IsPathRooted($Path) -or
        $Path.Contains('\') -or
        $Path.Contains(':') -or
        $Path.StartsWith('/') -or
        $Path.EndsWith('/') -or
        $Path.Contains('//') -or
        $Path -match '[\x00-\x1f]') {
        throw "$Label must be a portable relative path."
    }
    foreach ($segment in @($Path.Split('/'))) {
        if ([string]::IsNullOrWhiteSpace($segment) -or
            $segment -ceq '.' -or
            $segment -ceq '..') {
            throw "$Label contains an invalid path segment."
        }
    }
    return $Path
}

function Resolve-PartisanCompositeRecordedFile {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$PortablePath,
        [Parameter(Mandatory = $true)][string]$Label
    )

    [void](Assert-PartisanCompositePortablePath `
        -Path $PortablePath `
        -Label $Label)
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $candidate = [IO.Path]::GetFullPath((Join-Path `
        $rootFull `
        $PortablePath.Replace('/', [IO.Path]::DirectorySeparatorChar)))
    if (-not $candidate.StartsWith(
            $rootFull + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        throw "$Label is missing or outside its evidence root."
    }
    Assert-PartisanCompositeNoReparseAncestry `
        -Root $rootFull `
        -Path $candidate `
        -Label $Label
    return $candidate
}

function Read-PartisanCompositeJson {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label,
        [switch]$RejectLocalPaths
    )

    $resolved = Resolve-PartisanCompositeExistingFile `
        -Path $Path `
        -Label $Label
    $file = Get-Item -LiteralPath $resolved -Force -ErrorAction Stop
    if ($file.Length -le 0) {
        throw "$Label is empty."
    }
    $text = [IO.File]::ReadAllText($resolved)
    if ($RejectLocalPaths -and
        $text -match '(?i)(?:[A-Z]:[\\/]|\\\\|file://|/(?:Users|home|mnt)/)') {
        throw "$Label contains a local absolute path."
    }
    try {
        $value = $text | ConvertFrom-Json -ErrorAction Stop
    }
    catch {
        throw "$Label is not valid JSON."
    }
    return [pscustomobject][ordered]@{
        Path = $resolved
        FileName = Split-Path -Leaf $resolved
        Length = [long]$file.Length
        Sha256 = Get-PartisanCompositeSha256 -Path $resolved
        Text = $text
        Value = $value
    }
}

function ConvertFrom-PartisanCompositeUtcTimestamp {
    param(
        [Parameter(Mandatory = $true)][string]$Value,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $parsed = [DateTimeOffset]::MinValue
    $valid = [DateTimeOffset]::TryParse(
        $Value,
        [Globalization.CultureInfo]::InvariantCulture,
        [Globalization.DateTimeStyles]::AssumeUniversal,
        [ref]$parsed)
    if (-not $valid -or $parsed.Offset -ne [TimeSpan]::Zero) {
        throw "$Label must be an explicit UTC timestamp."
    }
    return $parsed.ToUniversalTime()
}

function Assert-PartisanCompositeStringSequence {
    param(
        [Parameter(Mandatory = $true)]$Actual,
        [Parameter(Mandatory = $true)][string[]]$Expected,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $rows = @($Actual)
    if ($rows.Count -ne $Expected.Count) {
        throw "$Label does not have the exact required cardinality."
    }
    for ($index = 0; $index -lt $Expected.Count; $index++) {
        if ($rows[$index] -isnot [string] -or
            [string]$rows[$index] -cne $Expected[$index]) {
            throw "$Label does not match the exact required sequence."
        }
    }
}

function Assert-PartisanCompositeExecutableIdentityShape {
    param(
        [Parameter(Mandatory = $true)]$Identity,
        [Parameter(Mandatory = $true)][string]$Label
    )

    Assert-PartisanCompositeProperties `
        -Value $Identity `
        -Names @('fileName', 'fileVersion', 'productVersion', 'length', 'sha256') `
        -Label $Label
    if ([string]::IsNullOrWhiteSpace([string]$Identity.fileName) -or
        [string]$Identity.fileName -cne (Split-Path -Leaf ([string]$Identity.fileName)) -or
        [string]::IsNullOrWhiteSpace([string]$Identity.fileVersion) -or
        [string]::IsNullOrWhiteSpace([string]$Identity.productVersion)) {
        throw "$Label contains an invalid executable name or version."
    }
    [void](Get-PartisanCompositeInt64 `
        -Value $Identity.length `
        -Label "$Label length" `
        -Minimum 1)
    [void](Assert-PartisanCompositeSha256 `
        -Value ([string]$Identity.sha256) `
        -Label "$Label sha256")
}

function Assert-PartisanCompositeExecutableIdentityMatch {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual,
        [Parameter(Mandatory = $true)][string]$Label
    )

    Assert-PartisanCompositeExecutableIdentityShape `
        -Identity $Expected `
        -Label "$Label expected identity"
    Assert-PartisanCompositeExecutableIdentityShape `
        -Identity $Actual `
        -Label "$Label actual identity"
    if ([string]$Actual.fileName -cne [string]$Expected.fileName -or
        [string]$Actual.fileVersion -cne [string]$Expected.fileVersion -or
        [string]$Actual.productVersion -cne [string]$Expected.productVersion -or
        [long]$Actual.length -ne [long]$Expected.length -or
        [string]$Actual.sha256 -cne [string]$Expected.sha256) {
        throw "$Label does not match the sealed candidate."
    }
}

function Assert-PartisanCompositeRecordedFiles {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)]$Rows,
        [Parameter(Mandatory = $true)][string]$Label,
        [switch]$RequireRole,
        [switch]$ManifestOnly
    )

    $result = New-Object Collections.Generic.List[object]
    $paths = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::OrdinalIgnoreCase)
    foreach ($row in @($Rows)) {
        $required = @('path', 'length', 'sha256')
        if ($RequireRole) {
            $required += 'role'
        }
        Assert-PartisanCompositeProperties `
            -Value $row `
            -Names $required `
            -Label "$Label row" `
            -Exact:$RequireRole
        $portablePath = Assert-PartisanCompositePortablePath `
            -Path ([string]$row.path) `
            -Label "$Label path"
        if (-not $paths.Add($portablePath)) {
            throw "$Label contains a duplicate file path."
        }
        $length = Get-PartisanCompositeInt64 `
            -Value $row.length `
            -Label "$Label length" `
            -Minimum 0
        $sha256 = Assert-PartisanCompositeSha256 `
            -Value ([string]$row.sha256) `
            -Label "$Label sha256"
        $resolved = ''
        if (-not $ManifestOnly) {
            $resolved = Resolve-PartisanCompositeRecordedFile `
                -Root $Root `
                -PortablePath $portablePath `
                -Label "$Label file"
            $file = Get-Item -LiteralPath $resolved -Force -ErrorAction Stop
            if ([long]$file.Length -ne $length -or
                (Get-PartisanCompositeSha256 -Path $resolved) -cne $sha256) {
                throw "$Label file differs from its recorded hash or length."
            }
        }
        $role = ''
        if ($RequireRole) {
            $role = [string]$row.role
            if ($role -cnotmatch '^[a-z][a-z0-9_]{0,63}$') {
                throw "$Label role is invalid."
            }
        }
        [void]$result.Add([pscustomobject][ordered]@{
            Path = $portablePath
            Role = $role
            Length = $length
            Sha256 = $sha256
            ResolvedPath = $resolved
        })
    }
    if ($result.Count -eq 0) {
        throw "$Label must contain hashed raw evidence."
    }
    return @($result.ToArray())
}

function Get-PartisanCompositeCandidateSeal {
    param([Parameter(Mandatory = $true)][string]$ManifestPath)

    $manifestInput = Read-PartisanCompositeJson `
        -Path $ManifestPath `
        -Label 'Sealed candidate manifest' `
        -RejectLocalPaths
    if ($manifestInput.FileName -cne 'candidate.json') {
        throw 'The sealed candidate manifest must be named candidate.json.'
    }
    $bundleRoot = Split-Path -Parent $manifestInput.Path
    $readyInput = Read-PartisanCompositeJson `
        -Path (Join-Path $bundleRoot 'candidate.ready.json') `
        -Label 'Sealed candidate ready record' `
        -RejectLocalPaths
    $manifest = $manifestInput.Value
    $ready = $readyInput.Value

    Assert-PartisanCompositeProperties `
        -Value $manifest `
        -Names @(
            'manifestSchemaVersion',
            'createdUtc',
            'candidate',
            'source',
            'addon',
            'toolchain',
            'workbench',
            'package',
            'evidence') `
        -Label 'Sealed candidate manifest'
    Assert-PartisanCompositeProperties `
        -Value $manifest.candidate `
        -Names @('id', 'version', 'state') `
        -Label 'Sealed candidate identity'
    Assert-PartisanCompositeProperties `
        -Value $manifest.source `
        -Names @(
            'gitHead',
            'dirty',
            'embeddedImplementation',
            'campaignSchema',
            'runtimeSettingsSchema') `
        -Label 'Sealed candidate source'
    Assert-PartisanCompositeProperties `
        -Value $manifest.source.embeddedImplementation `
        -Names @('sha', 'utc', 'label') `
        -Label 'Sealed embedded implementation'
    Assert-PartisanCompositeProperties `
        -Value $manifest.addon `
        -Names @('id', 'guid') `
        -Label 'Sealed candidate add-on'
    Assert-PartisanCompositeProperties `
        -Value $manifest.toolchain `
        -Names @('workbench', 'server', 'serverDiagnostic', 'client', 'clientDiagnostic') `
        -Label 'Sealed candidate toolchain'
    Assert-PartisanCompositeProperties `
        -Value $manifest.workbench `
        -Names @('crc') `
        -Label 'Sealed candidate Workbench record'
    Assert-PartisanCompositeProperties `
        -Value $manifest.package `
        -Names @('root', 'hashAlgorithm', 'sha256', 'canonicalIndexPath', 'files') `
        -Label 'Sealed candidate package'
    Assert-PartisanCompositeProperties `
        -Value $manifest.evidence `
        -Names @('root', 'files') `
        -Label 'Sealed candidate evidence'
    Assert-PartisanCompositeProperties `
        -Value $ready `
        -Names @('schemaVersion', 'candidateId', 'gitHead', 'packageSha256', 'manifestSha256') `
        -Label 'Sealed candidate ready record' `
        -Exact

    $candidateId = Assert-PartisanCompositeSafeId `
        -Value ([string]$manifest.candidate.id) `
        -Label 'Sealed candidate id'
    $manifestSchema = Get-PartisanCompositeInt64 `
        -Value $manifest.manifestSchemaVersion `
        -Label 'Sealed candidate manifest schema' `
        -Minimum 1
    $readySchema = Get-PartisanCompositeInt64 `
        -Value $ready.schemaVersion `
        -Label 'Sealed candidate ready schema' `
        -Minimum 1
    $campaignSchema = Get-PartisanCompositeInt64 `
        -Value $manifest.source.campaignSchema `
        -Label 'Sealed candidate campaign schema' `
        -Minimum 1
    $runtimeSettingsSchema = Get-PartisanCompositeInt64 `
        -Value $manifest.source.runtimeSettingsSchema `
        -Label 'Sealed candidate runtime-settings schema' `
        -Minimum 1
    if ($manifestSchema -ne $script:CandidateManifestSchema -or
        $readySchema -ne $script:CandidateReadySchema -or
        [string]$manifest.candidate.version -cnotmatch '^[0-9A-Za-z][0-9A-Za-z._-]{0,95}$' -or
        [string]$manifest.candidate.state -cne 'retained-uncertified' -or
        (Split-Path -Leaf $bundleRoot) -cne $candidateId -or
        [string]$manifest.source.gitHead -cnotmatch '^[0-9a-f]{40}$' -or
        [string]$manifest.source.embeddedImplementation.sha -cnotmatch '^[0-9a-f]{40}$' -or
        [string]::IsNullOrWhiteSpace([string]$manifest.source.embeddedImplementation.label) -or
        [string]$manifest.addon.id -cnotmatch '^[A-Za-z_][A-Za-z0-9_]*$' -or
        [string]$manifest.addon.guid -cnotmatch '^[0-9A-F]{16}$' -or
        [string]$manifest.workbench.crc -cnotmatch '^[0-9a-f]{8}$' -or
        [string]$manifest.package.root -cne 'package/Partisan' -or
        [string]$manifest.package.hashAlgorithm -cne $script:PackageHashAlgorithm -or
        [string]$manifest.package.canonicalIndexPath -cne 'evidence/pack/files.sha256' -or
        [string]$manifest.evidence.root -cne 'evidence') {
        throw 'The sealed candidate identity or schema contract is invalid.'
    }
    [void](Assert-PartisanCompositeBoolean `
        -Value $manifest.source.dirty `
        -Label 'Sealed candidate source dirty flag')
    if ([bool]$manifest.source.dirty) {
        throw 'A dirty source tree cannot be composite-certified.'
    }
    [void](ConvertFrom-PartisanCompositeUtcTimestamp `
        -Value ([string]$manifest.createdUtc) `
        -Label 'Sealed candidate creation time')
    [void](ConvertFrom-PartisanCompositeUtcTimestamp `
        -Value ([string]$manifest.source.embeddedImplementation.utc) `
        -Label 'Embedded implementation time')

    foreach ($identityName in @(
            'workbench',
            'server',
            'serverDiagnostic',
            'client',
            'clientDiagnostic')) {
        Assert-PartisanCompositeExecutableIdentityShape `
            -Identity $manifest.toolchain.$identityName `
            -Label "Sealed $identityName executable"
    }

    $packageRows = @($manifest.package.files)
    $requiredPackagePaths = @(
        'package/Partisan/addon.gproj',
        'package/Partisan/data.pak',
        'package/Partisan/resourceDatabase.rdb',
        'package/Partisan/thumbnail.png'
    )
    if ($packageRows.Count -ne $requiredPackagePaths.Count) {
        throw 'The sealed candidate package does not contain the exact release file set.'
    }
    $packageIndexRows = New-Object Collections.Generic.List[string]
    $seenPackagePaths = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($row in @($packageRows | Sort-Object indexPath)) {
        Assert-PartisanCompositeProperties `
            -Value $row `
            -Names @('path', 'indexPath', 'length', 'sha256') `
            -Label 'Sealed package file' `
            -Exact
        $path = Assert-PartisanCompositePortablePath `
            -Path ([string]$row.path) `
            -Label 'Sealed package path'
        $indexPath = Assert-PartisanCompositePortablePath `
            -Path ([string]$row.indexPath) `
            -Label 'Sealed package index path'
        if ($path -cne ('package/' + $indexPath) -or
            $requiredPackagePaths -cnotcontains $path -or
            -not $seenPackagePaths.Add($path)) {
            throw 'The sealed package file mapping is not exact.'
        }
        $length = Get-PartisanCompositeInt64 `
            -Value $row.length `
            -Label 'Sealed package file length' `
            -Minimum 1
        $sha256 = Assert-PartisanCompositeSha256 `
            -Value ([string]$row.sha256) `
            -Label 'Sealed package file sha256'
        [void]$packageIndexRows.Add(("{0}`t{1}`t{2}" -f
            $sha256,
            $length,
            $indexPath))
    }
    $packageDigest = Get-PartisanCompositeTextSha256 `
        -Text (($packageIndexRows.ToArray() -join "`n") + "`n")
    [void](Assert-PartisanCompositeSha256 `
        -Value ([string]$manifest.package.sha256) `
        -Label 'Sealed package digest')
    if ($packageDigest -cne [string]$manifest.package.sha256) {
        throw 'The sealed package digest differs from the canonical file index.'
    }

    $evidenceRows = @(Assert-PartisanCompositeRecordedFiles `
        -Root $bundleRoot `
        -Rows $manifest.evidence.files `
        -Label 'Sealed candidate evidence inventory' `
        -ManifestOnly)
    $canonicalIndex = @($evidenceRows | Where-Object {
        $_.Path -ceq [string]$manifest.package.canonicalIndexPath
    })
    if ($canonicalIndex.Count -ne 1 -or
        $canonicalIndex[0].Sha256 -cne $packageDigest) {
        throw 'The sealed package canonical index is missing or hash-mismatched.'
    }

    if ([string]$ready.candidateId -cne $candidateId -or
        [string]$ready.gitHead -cne [string]$manifest.source.gitHead -or
        [string]$ready.packageSha256 -cne $packageDigest -or
        [string]$ready.manifestSha256 -cne $manifestInput.Sha256) {
        throw 'The candidate ready record does not seal the supplied manifest.'
    }

    $identity = [pscustomobject][ordered]@{
        candidateId = $candidateId
        candidateVersion = [string]$manifest.candidate.version
        gitHead = [string]$manifest.source.gitHead
        embeddedBuildSha = [string]$manifest.source.embeddedImplementation.sha
        embeddedBuildUtc = [string]$manifest.source.embeddedImplementation.utc
        embeddedBuildLabel = [string]$manifest.source.embeddedImplementation.label
        campaignSchema = [int]$campaignSchema
        runtimeSettingsSchema = [int]$runtimeSettingsSchema
        addonId = [string]$manifest.addon.id
        addonGuid = [string]$manifest.addon.guid
        packageHashAlgorithm = [string]$manifest.package.hashAlgorithm
        packageSha256 = $packageDigest
        manifestSha256 = $manifestInput.Sha256
        readySha256 = $readyInput.Sha256
        workbenchCrc = [string]$manifest.workbench.crc
    }
    return [pscustomobject][ordered]@{
        BundleRoot = $bundleRoot
        ManifestInput = $manifestInput
        ReadyInput = $readyInput
        Manifest = $manifest
        Ready = $ready
        Identity = $identity
        Toolchain = $manifest.toolchain
        PackageFileCount = $packageRows.Count
        CandidateEvidenceFileCount = $evidenceRows.Count
    }
}

function Assert-PartisanCompositeCandidateBinding {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual,
        [Parameter(Mandatory = $true)][string]$Label,
        [switch]$Exact
    )

    $names = @(
        'candidateId',
        'candidateVersion',
        'gitHead',
        'embeddedBuildSha',
        'embeddedBuildUtc',
        'embeddedBuildLabel',
        'campaignSchema',
        'runtimeSettingsSchema',
        'addonId',
        'addonGuid',
        'packageHashAlgorithm',
        'packageSha256',
        'manifestSha256',
        'readySha256',
        'workbenchCrc'
    )
    Assert-PartisanCompositeProperties `
        -Value $Actual `
        -Names $names `
        -Label $Label `
        -Exact:$Exact
    [void](Assert-PartisanCompositeSafeId `
        -Value ([string]$Actual.candidateId) `
        -Label "$Label candidate id")
    if ([string]$Actual.candidateVersion -cnotmatch
            '^[0-9A-Za-z][0-9A-Za-z._-]{0,95}$' -or
        [string]$Actual.gitHead -cnotmatch '^[0-9a-f]{40}$' -or
        [string]$Actual.embeddedBuildSha -cnotmatch '^[0-9a-f]{40}$' -or
        [string]$Actual.addonId -cnotmatch '^[A-Za-z_][A-Za-z0-9_]*$' -or
        [string]$Actual.addonGuid -cnotmatch '^[0-9A-F]{16}$' -or
        [string]$Actual.packageHashAlgorithm -cne $script:PackageHashAlgorithm -or
        [string]$Actual.workbenchCrc -cnotmatch '^[0-9a-f]{8}$') {
        throw "$Label contains an invalid candidate identity shape."
    }
    [void](ConvertFrom-PartisanCompositeUtcTimestamp `
        -Value ([string]$Actual.embeddedBuildUtc) `
        -Label "$Label embedded build time")
    [void](Get-PartisanCompositeInt64 `
        -Value $Actual.campaignSchema `
        -Label "$Label campaign schema" `
        -Minimum 1)
    [void](Get-PartisanCompositeInt64 `
        -Value $Actual.runtimeSettingsSchema `
        -Label "$Label runtime-settings schema" `
        -Minimum 1)
    foreach ($hashName in @('packageSha256', 'manifestSha256', 'readySha256')) {
        [void](Assert-PartisanCompositeSha256 `
            -Value ([string]$Actual.$hashName) `
            -Label "$Label $hashName")
    }
    foreach ($name in $names) {
        if ([string]$Actual.$name -cne [string]$Expected.$name) {
            throw "$Label property $name does not match the sealed candidate."
        }
    }
}

function Get-PartisanCompositeRunCleanupProblems {
    param([Parameter(Mandatory = $true)]$Cleanup)

    $zeroNames = @(
        'guardRemaining',
        'ownedProcessesRemaining',
        'newEngineProcessesRemaining',
        'unclaimedEngineProcessesObserved',
        'newDefaultEntriesRemaining',
        'modifiedDefaultFiles',
        'deletedDefaultEntries',
        'missingDefaultRoots',
        'externalSpillEntriesRemaining',
        'modifiedSpillFiles',
        'deletedSpillEntries',
        'missingSpillRoots',
        'cleanupPhaseErrorCount'
    )
    Assert-PartisanCompositeProperties `
        -Value $Cleanup `
        -Names ($zeroNames + @('cleanupPhaseErrors', 'monitoringRootsAreDetectionOnly')) `
        -Label 'Packaged run cleanup'
    $problems = New-Object Collections.Generic.List[string]
    foreach ($name in $zeroNames) {
        $count = Get-PartisanCompositeInt64 `
            -Value $Cleanup.$name `
            -Label "Packaged run cleanup $name" `
            -Minimum 0
        if ($count -ne 0) {
            [void]$problems.Add("packaged_cleanup_$name")
        }
    }
    if (@($Cleanup.cleanupPhaseErrors).Count -ne 0) {
        [void]$problems.Add('packaged_cleanup_errors')
    }
    if (-not (Assert-PartisanCompositeBoolean `
            -Value $Cleanup.monitoringRootsAreDetectionOnly `
            -Label 'Packaged run monitoring-root flag')) {
        [void]$problems.Add('packaged_cleanup_monitoring_scope')
    }
    return @($problems.ToArray())
}

function Get-PartisanCompositeManualGapAssertions {
    param([Parameter(Mandatory = $true)]$CampaignDebugArtifact)

    Assert-PartisanCompositeProperties `
        -Value $CampaignDebugArtifact `
        -Names @(
            'm_sRunId',
            'm_sProfile',
            'm_sBuildSha',
            'm_sBuildUtc',
            'm_sBuildLabel',
            'm_bCertificationPassed',
            'm_aCases') `
        -Label 'Campaign Debug artifact'
    $found = @{}
    foreach ($caseResult in @($CampaignDebugArtifact.m_aCases)) {
        Assert-PartisanCompositeProperties `
            -Value $caseResult `
            -Names @('m_sCaseId', 'm_aAssertions') `
            -Label 'Campaign Debug case'
        foreach ($assertion in @($caseResult.m_aAssertions)) {
            Assert-PartisanCompositeProperties `
                -Value $assertion `
                -Names @(
                    'm_sAssertionId',
                    'm_sExpected',
                    'm_sActual',
                    'm_sStatus',
                    'm_sFailureReason',
                    'm_sProofLevel',
                    'm_sObservedPath',
                    'm_sRequiredPath',
                    'm_bCountsTowardCertification') `
                -Label 'Campaign Debug assertion'
            $assertionId = [string]$assertion.m_sAssertionId
            if ($script:ManualGapAssertionOrder -cnotcontains $assertionId) {
                continue
            }
            if ($found.ContainsKey($assertionId)) {
                throw "Campaign Debug contains duplicate manual-gap assertion $assertionId."
            }
            if ([string]$assertion.m_sStatus -cne 'BLOCKED' -or
                [string]$assertion.m_sProofLevel -cne 'EXTERNAL_PROCESS' -or
                [string]$assertion.m_sObservedPath -cne 'manual_external_gap' -or
                (Assert-PartisanCompositeBoolean `
                    -Value $assertion.m_bCountsTowardCertification `
                    -Label "$assertionId certification-count flag")) {
                throw "Campaign Debug manual-gap assertion $assertionId was relabeled or made in-process."
            }
            if ([string]::IsNullOrWhiteSpace([string]$assertion.m_sExpected) -or
                [string]::IsNullOrWhiteSpace([string]$assertion.m_sActual) -or
                [string]::IsNullOrWhiteSpace([string]$assertion.m_sFailureReason) -or
                [string]::IsNullOrWhiteSpace([string]$assertion.m_sRequiredPath)) {
                throw "Campaign Debug manual-gap assertion $assertionId is incomplete."
            }
            if ($assertionId.StartsWith('phase25.') -and
                [string]$caseResult.m_sCaseId -cne 'phase25.manual_external_gaps') {
                throw "Campaign Debug assertion $assertionId is outside the manual external-gap case."
            }
            $found[$assertionId] = [pscustomobject][ordered]@{
                CaseId = [string]$caseResult.m_sCaseId
                Assertion = $assertion
            }
        }
    }
    foreach ($assertionId in $script:ManualGapAssertionOrder) {
        if (-not $found.ContainsKey($assertionId)) {
            throw "Campaign Debug is missing manual-gap assertion $assertionId."
        }
    }
    return $found
}

function Get-PartisanCompositeRequiredRestartFeatures {
    param([Parameter(Mandatory = $true)]$CampaignDebugArtifact)

    $features = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($caseResult in @($CampaignDebugArtifact.m_aCases)) {
        Assert-PartisanCompositeProperties `
            -Value $caseResult `
            -Names @('m_sCaseId', 'm_sCategory', 'm_sFeature', 'm_sStage') `
            -Label 'Campaign Debug case restart scope'
        $stage = [string]$caseResult.m_sStage
        $category = [string]$caseResult.m_sCategory
        $feature = [string]$caseResult.m_sFeature
        $isPrimitiveProbe = $stage -ceq 'primitive_probe'
        $isMissionPhysicalProbe =
            $stage -ceq 'physical_probe' -and
            $category.StartsWith('mission', [StringComparison]::Ordinal) -and
            $feature -cne 'ai_combat_contact'
        if (-not $isPrimitiveProbe -and -not $isMissionPhysicalProbe) {
            continue
        }
        [void](Assert-PartisanCompositeSafeId `
            -Value $feature `
            -Label 'Campaign Debug restart feature')
        [void]$features.Add($feature)
    }
    $result = @($features | Sort-Object -CaseSensitive)
    if ($result.Count -eq 0) {
        throw 'The Full Campaign Debug artifact does not derive any restart features.'
    }
    return $result
}

function Get-PartisanCompositePackagedRun {
    param(
        [Parameter(Mandatory = $true)]$CandidateSeal,
        [Parameter(Mandatory = $true)][string]$RunPath
    )

    $runInput = Read-PartisanCompositeJson `
        -Path $RunPath `
        -Label 'Packaged Campaign Debug run envelope' `
        -RejectLocalPaths
    if ($runInput.FileName -cne 'run.json') {
        throw 'The packaged Campaign Debug envelope must be named run.json.'
    }
    $runRoot = Split-Path -Parent $runInput.Path
    $run = $runInput.Value
    Assert-PartisanCompositeProperties `
        -Value $run `
        -Names @(
            'schemaVersion',
            'evidenceKind',
            'startedUtc',
            'completedUtc',
            'candidate',
            'harness',
            'launch',
            'outcome',
            'cleanup',
            'files') `
        -Label 'Packaged Campaign Debug run envelope'
    $runSchema = Get-PartisanCompositeInt64 `
        -Value $run.schemaVersion `
        -Label 'Packaged Campaign Debug envelope schema' `
        -Minimum 1
    if ($runSchema -ne $script:PackagedRunSchema -or
        [string]$run.evidenceKind -cne $script:PackagedRunEvidenceKind) {
        throw 'The packaged Campaign Debug envelope schema or evidence kind is invalid.'
    }
    $startedUtc = ConvertFrom-PartisanCompositeUtcTimestamp `
        -Value ([string]$run.startedUtc) `
        -Label 'Packaged Campaign Debug start time'
    $completedUtc = ConvertFrom-PartisanCompositeUtcTimestamp `
        -Value ([string]$run.completedUtc) `
        -Label 'Packaged Campaign Debug completion time'
    if ($completedUtc -lt $startedUtc) {
        throw 'The packaged Campaign Debug completion precedes its start.'
    }

    Assert-PartisanCompositeCandidateBinding `
        -Expected $CandidateSeal.Identity `
        -Actual $run.candidate `
        -Label 'Packaged Campaign Debug candidate binding'
    Assert-PartisanCompositeProperties `
        -Value $run.candidate `
        -Names @(
            'runtimeUseDisposition',
            'runtimeRole',
            'diagnosticExecutable',
            'recordedDiagnosticExecutable',
            'recordedRuntimeExecutable') `
        -Label 'Packaged Campaign Debug runtime binding'
    $runtimeRole = [string]$run.candidate.runtimeRole
    if ($runtimeRole -cnotin @('client', 'server')) {
        throw 'The packaged Campaign Debug runtime role is invalid.'
    }
    if ($runtimeRole -ceq 'client') {
        $expectedRuntime = $CandidateSeal.Toolchain.client
        $expectedDiagnostic = $CandidateSeal.Toolchain.clientDiagnostic
    }
    else {
        $expectedRuntime = $CandidateSeal.Toolchain.server
        $expectedDiagnostic = $CandidateSeal.Toolchain.serverDiagnostic
    }
    Assert-PartisanCompositeExecutableIdentityMatch `
        -Expected $expectedRuntime `
        -Actual $run.candidate.recordedRuntimeExecutable `
        -Label 'Packaged Campaign Debug runtime executable'
    Assert-PartisanCompositeExecutableIdentityMatch `
        -Expected $expectedDiagnostic `
        -Actual $run.candidate.recordedDiagnosticExecutable `
        -Label 'Packaged Campaign Debug recorded diagnostic executable'
    Assert-PartisanCompositeExecutableIdentityMatch `
        -Expected $expectedDiagnostic `
        -Actual $run.candidate.diagnosticExecutable `
        -Label 'Packaged Campaign Debug diagnostic executable'

    Assert-PartisanCompositeProperties `
        -Value $run.harness `
        -Names @('gitHead', 'dirty', 'campaignRunnerSha256', 'candidateModuleSha256') `
        -Label 'Packaged Campaign Debug harness'
    if ([string]$run.harness.gitHead -cnotmatch '^[0-9a-f]{40}$') {
        throw 'The packaged Campaign Debug harness git identity is invalid.'
    }
    [void](Assert-PartisanCompositeBoolean `
        -Value $run.harness.dirty `
        -Label 'Packaged Campaign Debug harness dirty flag')
    [void](Assert-PartisanCompositeSha256 `
        -Value ([string]$run.harness.campaignRunnerSha256) `
        -Label 'Packaged Campaign Debug runner sha256')
    [void](Assert-PartisanCompositeSha256 `
        -Value ([string]$run.harness.candidateModuleSha256) `
        -Label 'Packaged candidate module sha256')

    Assert-PartisanCompositeProperties `
        -Value $run.launch `
        -Names @(
            'profile',
            'proofScope',
            'worldResource',
            'stagedPackage',
            'addonGuid',
            'packageSha256',
            'diagnosticExecutable',
            'recordedRuntimeExecutable') `
        -Label 'Packaged Campaign Debug launch'
    if ([string]$run.launch.profile -cne $script:CampaignDebugProfile -or
        [string]$run.launch.proofScope -cne $script:CampaignDebugProofScope -or
        [string]::IsNullOrWhiteSpace([string]$run.launch.worldResource) -or
        -not (Assert-PartisanCompositeBoolean `
            -Value $run.launch.stagedPackage `
            -Label 'Packaged Campaign Debug staged-package flag') -or
        [string]$run.launch.addonGuid -cne $CandidateSeal.Identity.addonGuid -or
        [string]$run.launch.packageSha256 -cne $CandidateSeal.Identity.packageSha256) {
        throw 'The packaged Campaign Debug launch is not an exact staged Full run.'
    }
    Assert-PartisanCompositeExecutableIdentityMatch `
        -Expected $expectedDiagnostic `
        -Actual $run.launch.diagnosticExecutable `
        -Label 'Packaged Campaign Debug launch diagnostic executable'
    Assert-PartisanCompositeExecutableIdentityMatch `
        -Expected $expectedRuntime `
        -Actual $run.launch.recordedRuntimeExecutable `
        -Label 'Packaged Campaign Debug launch runtime executable'

    $verifiedFiles = @(Assert-PartisanCompositeRecordedFiles `
        -Root $runRoot `
        -Rows $run.files `
        -Label 'Packaged Campaign Debug evidence')
    $manifestCopies = @($verifiedFiles | Where-Object {
        $_.Path -ceq 'identity/candidate.json'
    })
    $readyCopies = @($verifiedFiles | Where-Object {
        $_.Path -ceq 'identity/candidate.ready.json'
    })
    if ($manifestCopies.Count -ne 1 -or
        $manifestCopies[0].Sha256 -cne $CandidateSeal.Identity.manifestSha256 -or
        $readyCopies.Count -ne 1 -or
        $readyCopies[0].Sha256 -cne $CandidateSeal.Identity.readySha256) {
        throw 'The packaged Campaign Debug identity copies do not match the sealed candidate.'
    }

    $campaignJsonRows = @($verifiedFiles | Where-Object {
        $_.Path -cmatch '^raw/campaign-debug/(HST_CampaignDebug_[A-Za-z0-9._-]+)\.json$'
    })
    if ($campaignJsonRows.Count -ne 1) {
        throw 'The packaged run must contain exactly one hashed Campaign Debug JSON artifact.'
    }
    [void]($campaignJsonRows[0].Path -cmatch
        '^raw/campaign-debug/(HST_CampaignDebug_[A-Za-z0-9._-]+)\.json$')
    $artifactBase = $Matches[1]
    $summaryRelative = 'raw/campaign-debug/' + $artifactBase + '_summary.txt'
    $stateDiffRelative = 'raw/campaign-debug/' + $artifactBase + '_state_diff.txt'
    $summaryRows = @($verifiedFiles | Where-Object {
        $_.Path -ceq $summaryRelative
    })
    $stateDiffRows = @($verifiedFiles | Where-Object {
        $_.Path -ceq $stateDiffRelative
    })
    $allCampaignRows = @($verifiedFiles | Where-Object {
        $_.Path.StartsWith(
            'raw/campaign-debug/',
            [StringComparison]::Ordinal)
    })
    if ($summaryRows.Count -ne 1 -or
        $stateDiffRows.Count -ne 1 -or
        $allCampaignRows.Count -ne 3) {
        throw 'The packaged run does not contain the exact three-file Campaign Debug artifact set.'
    }

    $artifactInput = Read-PartisanCompositeJson `
        -Path $campaignJsonRows[0].ResolvedPath `
        -Label 'Raw Campaign Debug JSON artifact'
    $artifact = $artifactInput.Value
    $manualGaps = Get-PartisanCompositeManualGapAssertions `
        -CampaignDebugArtifact $artifact
    $requiredRestartFeatures = @(
        Get-PartisanCompositeRequiredRestartFeatures `
            -CampaignDebugArtifact $artifact)
    if ([string]$artifact.m_sProfile -cne $script:CampaignDebugProfile -or
        [string]$artifact.m_sBuildSha -cne $CandidateSeal.Identity.embeddedBuildSha -or
        [string]$artifact.m_sBuildUtc -cne $CandidateSeal.Identity.embeddedBuildUtc -or
        [string]$artifact.m_sBuildLabel -cne $CandidateSeal.Identity.embeddedBuildLabel) {
        throw 'The raw Campaign Debug artifact does not match the Full profile or sealed build identity.'
    }
    [void](Assert-PartisanCompositeBoolean `
        -Value $artifact.m_bCertificationPassed `
        -Label 'Raw Campaign Debug certification flag')

    Assert-PartisanCompositeProperties `
        -Value $run.outcome `
        -Names @(
            'success',
            'armed',
            'started',
            'completed',
            'candidateBoundaryVerified',
            'mountAttestation',
            'artifactsStable',
            'evidenceCaptured',
            'validation',
            'errorCensus') `
        -Label 'Packaged Campaign Debug outcome'
    Assert-PartisanCompositeProperties `
        -Value $run.outcome.mountAttestation `
        -Names @('Valid', 'GuidExact', 'Packed') `
        -Label 'Packaged Campaign Debug mount attestation'
    Assert-PartisanCompositeProperties `
        -Value $run.outcome.validation `
        -Names @(
            'Valid',
            'RunId',
            'Profile',
            'ProofScope',
            'FullCertification',
            'BuildProvenanceMatches',
            'CertificationPassed') `
        -Label 'Packaged Campaign Debug validation'
    Assert-PartisanCompositeProperties `
        -Value $run.outcome.errorCensus `
        -Names @('Valid', 'HardDiagnosticFree', 'UnapprovedHardDiagnosticCount') `
        -Label 'Packaged Campaign Debug error census'
    if ([string]$run.outcome.validation.RunId -cne [string]$artifact.m_sRunId -or
        [string]$run.outcome.validation.Profile -cne $script:CampaignDebugProfile -or
        [string]$run.outcome.validation.ProofScope -cne $script:CampaignDebugProofScope) {
        throw 'The packaged validation identity does not match the raw Campaign Debug artifact.'
    }

    $acceptanceProblems = New-Object Collections.Generic.List[string]
    foreach ($flag in @(
            'success',
            'armed',
            'started',
            'completed',
            'candidateBoundaryVerified',
            'artifactsStable',
            'evidenceCaptured')) {
        if (-not (Assert-PartisanCompositeBoolean `
                -Value $run.outcome.$flag `
                -Label "Packaged Campaign Debug outcome $flag")) {
            [void]$acceptanceProblems.Add("packaged_outcome_$flag")
        }
    }
    foreach ($flag in @('Valid', 'GuidExact', 'Packed')) {
        if (-not (Assert-PartisanCompositeBoolean `
                -Value $run.outcome.mountAttestation.$flag `
                -Label "Packaged mount attestation $flag")) {
            [void]$acceptanceProblems.Add(
                'packaged_mount_' + $flag.ToLowerInvariant())
        }
    }
    foreach ($flag in @(
            'Valid',
            'FullCertification',
            'BuildProvenanceMatches',
            'CertificationPassed')) {
        if (-not (Assert-PartisanCompositeBoolean `
                -Value $run.outcome.validation.$flag `
                -Label "Packaged validation $flag")) {
            [void]$acceptanceProblems.Add(
                'packaged_validation_' + $flag.ToLowerInvariant())
        }
    }
    if (-not [bool]$artifact.m_bCertificationPassed) {
        [void]$acceptanceProblems.Add('campaign_debug_in_process_certification_failed')
    }
    foreach ($flag in @('Valid', 'HardDiagnosticFree')) {
        if (-not (Assert-PartisanCompositeBoolean `
                -Value $run.outcome.errorCensus.$flag `
                -Label "Packaged error census $flag")) {
            [void]$acceptanceProblems.Add(
                'packaged_error_census_' + $flag.ToLowerInvariant())
        }
    }
    $unapprovedDiagnostics = Get-PartisanCompositeInt64 `
        -Value $run.outcome.errorCensus.UnapprovedHardDiagnosticCount `
        -Label 'Packaged unapproved hard-diagnostic count' `
        -Minimum 0
    if ($unapprovedDiagnostics -ne 0) {
        [void]$acceptanceProblems.Add('packaged_unapproved_hard_diagnostics')
    }
    if (Assert-PartisanCompositeBoolean `
            -Value $run.harness.dirty `
            -Label 'Packaged Campaign Debug harness dirty flag') {
        [void]$acceptanceProblems.Add('packaged_harness_dirty')
    }
    foreach ($problem in @(Get-PartisanCompositeRunCleanupProblems `
            -Cleanup $run.cleanup)) {
        [void]$acceptanceProblems.Add($problem)
    }

    $rawArtifacts = @($allCampaignRows | Sort-Object Path | ForEach-Object {
        [pscustomobject][ordered]@{
            path = $_.Path
            length = $_.Length
            sha256 = $_.Sha256
        }
    })
    return [pscustomobject][ordered]@{
        RunInput = $runInput
        Run = $run
        StartedUtc = $startedUtc
        CompletedUtc = $completedUtc
        Artifact = $artifact
        ArtifactInput = $artifactInput
        ManualGaps = $manualGaps
        RequiredRestartFeatures = $requiredRestartFeatures
        AcceptanceProblems = @($acceptanceProblems.ToArray())
        Accepted = $acceptanceProblems.Count -eq 0
        RawArtifacts = $rawArtifacts
        VerifiedFileCount = $verifiedFiles.Count
    }
}

function Assert-PartisanCompositeExternalTeardown {
    param([Parameter(Mandatory = $true)]$Teardown)

    $names = @(
        'clean',
        'guardEntriesRemaining',
        'ownedProcessesRemaining',
        'profileEntriesRemaining',
        'errors'
    )
    Assert-PartisanCompositeProperties `
        -Value $Teardown `
        -Names $names `
        -Label 'External evidence teardown' `
        -Exact
    if (-not (Assert-PartisanCompositeBoolean `
            -Value $Teardown.clean `
            -Label 'External evidence clean-teardown flag')) {
        throw 'External evidence teardown is not clean.'
    }
    foreach ($name in @(
            'guardEntriesRemaining',
            'ownedProcessesRemaining',
            'profileEntriesRemaining')) {
        if ((Get-PartisanCompositeInt64 `
                -Value $Teardown.$name `
                -Label "External evidence teardown $name" `
                -Minimum 0) -ne 0) {
            throw 'External evidence teardown left owned state behind.'
        }
    }
    if (@($Teardown.errors).Count -ne 0) {
        throw 'External evidence teardown contains cleanup errors.'
    }
}

function Assert-PartisanCompositeExternalRoles {
    param(
        [Parameter(Mandatory = $true)]$Rows,
        [Parameter(Mandatory = $true)][string[]]$RequiredRoles,
        [Parameter(Mandatory = $true)][string]$ScenarioId
    )

    foreach ($role in $RequiredRoles) {
        $matches = @($Rows | Where-Object { $_.Role -ceq $role })
        if ($matches.Count -eq 0) {
            throw "External scenario $ScenarioId is missing raw evidence role $role."
        }
        if ($role -cin @('harness', 'scenario_receipt') -and
            $matches.Count -ne 1) {
            throw "External scenario $ScenarioId must contain exactly one $role file."
        }
    }
}

function Get-PartisanCompositeExternalEvidence {
    param(
        [Parameter(Mandatory = $true)]$CandidateSeal,
        [Parameter(Mandatory = $true)]$PackagedRun,
        [Parameter(Mandatory = $true)][string]$EvidencePath
    )

    $input = Read-PartisanCompositeJson `
        -Path $EvidencePath `
        -Label 'External certification evidence' `
        -RejectLocalPaths
    $root = Split-Path -Parent $input.Path
    $evidence = $input.Value
    $topNames = @(
        'schemaVersion',
        'evidenceKind',
        'evidenceId',
        'startedUtc',
        'completedUtc',
        'candidate',
        'runtime',
        'harness',
        'scenario',
        'execution',
        'teardown',
        'files'
    )
    Assert-PartisanCompositeProperties `
        -Value $evidence `
        -Names $topNames `
        -Label 'External certification evidence' `
        -Exact
    $externalSchema = Get-PartisanCompositeInt64 `
        -Value $evidence.schemaVersion `
        -Label 'External certification evidence schema' `
        -Minimum 1
    if ($externalSchema -ne $script:ExternalEvidenceSchema -or
        [string]$evidence.evidenceKind -cne $script:ExternalEvidenceKind) {
        throw 'External certification evidence schema or kind is invalid.'
    }
    $evidenceId = Assert-PartisanCompositeSafeId `
        -Value ([string]$evidence.evidenceId) `
        -Label 'External evidence id'
    $startedUtc = ConvertFrom-PartisanCompositeUtcTimestamp `
        -Value ([string]$evidence.startedUtc) `
        -Label 'External evidence start time'
    $completedUtc = ConvertFrom-PartisanCompositeUtcTimestamp `
        -Value ([string]$evidence.completedUtc) `
        -Label 'External evidence completion time'
    if ($completedUtc -lt $startedUtc -or
        $startedUtc -lt $PackagedRun.CompletedUtc -or
        $completedUtc -gt [DateTimeOffset]::UtcNow.AddMinutes(5)) {
        throw 'External evidence timestamps are out of order or not post-Full evidence.'
    }

    Assert-PartisanCompositeCandidateBinding `
        -Expected $CandidateSeal.Identity `
        -Actual $evidence.candidate `
        -Label 'External evidence candidate binding' `
        -Exact
    Assert-PartisanCompositeProperties `
        -Value $evidence.runtime `
        -Names @('serverExecutable', 'clientExecutable') `
        -Label 'External evidence runtime binding' `
        -Exact
    Assert-PartisanCompositeExecutableIdentityMatch `
        -Expected $CandidateSeal.Toolchain.server `
        -Actual $evidence.runtime.serverExecutable `
        -Label 'External evidence server executable'
    Assert-PartisanCompositeExecutableIdentityMatch `
        -Expected $CandidateSeal.Toolchain.client `
        -Actual $evidence.runtime.clientExecutable `
        -Label 'External evidence client executable'

    Assert-PartisanCompositeProperties `
        -Value $evidence.scenario `
        -Names @('id', 'contractVersion', 'assertionIds') `
        -Label 'External evidence scenario' `
        -Exact
    $scenarioId = [string]$evidence.scenario.id
    if ($script:ScenarioOrder -cnotcontains $scenarioId) {
        throw 'External evidence scenario is not an explicit certification scenario.'
    }
    $contract = $script:ScenarioContracts[$scenarioId]
    $contractVersion = Get-PartisanCompositeInt64 `
        -Value $evidence.scenario.contractVersion `
        -Label "External scenario $scenarioId contract version" `
        -Minimum 1
    if ($contractVersion -ne $contract.ContractVersion) {
        throw "External scenario $scenarioId uses the wrong contract version."
    }
    Assert-PartisanCompositeStringSequence `
        -Actual $evidence.scenario.assertionIds `
        -Expected $contract.AssertionIds `
        -Label "External scenario $scenarioId assertion mapping"

    Assert-PartisanCompositeProperties `
        -Value $evidence.harness `
        -Names @('gitHead', 'dirty', 'path', 'sha256') `
        -Label 'External evidence harness' `
        -Exact
    if ([string]$evidence.harness.gitHead -cnotmatch '^[0-9a-f]{40}$' -or
        (Assert-PartisanCompositeBoolean `
            -Value $evidence.harness.dirty `
            -Label 'External evidence harness dirty flag')) {
        throw 'External evidence requires a clean, commit-bound harness.'
    }
    $harnessPath = Assert-PartisanCompositePortablePath `
        -Path ([string]$evidence.harness.path) `
        -Label 'External evidence harness path'
    $harnessSha = Assert-PartisanCompositeSha256 `
        -Value ([string]$evidence.harness.sha256) `
        -Label 'External evidence harness sha256'

    $executionNames = @(
        'executed',
        'success',
        'worldResource',
        'durationSeconds',
        'clientCount',
        'restartReceipts',
        'jipJoinCount',
        'reconnectCount',
        'checks'
    )
    Assert-PartisanCompositeProperties `
        -Value $evidence.execution `
        -Names $executionNames `
        -Label 'External evidence execution' `
        -Exact
    if (-not (Assert-PartisanCompositeBoolean `
            -Value $evidence.execution.executed `
            -Label 'External evidence executed flag') -or
        -not (Assert-PartisanCompositeBoolean `
            -Value $evidence.execution.success `
            -Label 'External evidence success flag') -or
        [string]::IsNullOrWhiteSpace([string]$evidence.execution.worldResource)) {
        throw "External scenario $scenarioId was not executed successfully."
    }
    $durationSeconds = Get-PartisanCompositeInt64 `
        -Value $evidence.execution.durationSeconds `
        -Label "External scenario $scenarioId duration" `
        -Minimum 0
    $clientCount = Get-PartisanCompositeInt64 `
        -Value $evidence.execution.clientCount `
        -Label "External scenario $scenarioId client count" `
        -Minimum 0
    $jipJoinCount = Get-PartisanCompositeInt64 `
        -Value $evidence.execution.jipJoinCount `
        -Label "External scenario $scenarioId JIP count" `
        -Minimum 0
    $reconnectCount = Get-PartisanCompositeInt64 `
        -Value $evidence.execution.reconnectCount `
        -Label "External scenario $scenarioId reconnect count" `
        -Minimum 0
    $elapsedSeconds = [long][Math]::Floor(
        ($completedUtc - $startedUtc).TotalSeconds)
    if ($durationSeconds -lt $contract.MinimumDurationSeconds -or
        $elapsedSeconds -lt $contract.MinimumDurationSeconds -or
        $durationSeconds -gt ($elapsedSeconds + 1) -or
        $clientCount -lt $contract.MinimumClientCount) {
        throw "External scenario $scenarioId does not meet its duration or client threshold."
    }

    $restartReceipts = @($evidence.execution.restartReceipts)
    if ($scenarioId -ceq 'broad_restart') {
        $requiredRestartFeatures = @($PackagedRun.RequiredRestartFeatures)
        if ($restartReceipts.Count -lt $requiredRestartFeatures.Count -or
            $reconnectCount -lt $restartReceipts.Count) {
            throw 'Broad restart evidence does not contain enough exact restart/reconnect receipts for the Full feature set.'
        }
        $coveredFeatures = New-Object 'Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
        foreach ($receipt in $restartReceipts) {
            Assert-PartisanCompositeProperties `
                -Value $receipt `
                -Names @(
                    'primitiveFeature',
                    'beforeProcessId',
                    'afterProcessId',
                    'stateVerified',
                    'clientReconnected',
                    'samePackageVerified') `
                -Label 'Broad restart receipt' `
                -Exact
            $primitiveFeature = Assert-PartisanCompositeSafeId `
                -Value ([string]$receipt.primitiveFeature) `
                -Label 'Broad restart primitive feature'
            $beforeProcessId = Get-PartisanCompositeInt64 `
                -Value $receipt.beforeProcessId `
                -Label 'Broad restart old process id' `
                -Minimum 1
            $afterProcessId = Get-PartisanCompositeInt64 `
                -Value $receipt.afterProcessId `
                -Label 'Broad restart new process id' `
                -Minimum 1
            if ($requiredRestartFeatures -cnotcontains $primitiveFeature -or
                $beforeProcessId -eq $afterProcessId -or
                -not (Assert-PartisanCompositeBoolean `
                    -Value $receipt.stateVerified `
                    -Label 'Broad restart state-verification flag') -or
                -not (Assert-PartisanCompositeBoolean `
                    -Value $receipt.clientReconnected `
                    -Label 'Broad restart client-reconnect flag') -or
                -not (Assert-PartisanCompositeBoolean `
                    -Value $receipt.samePackageVerified `
                    -Label 'Broad restart same-package flag')) {
                throw 'A broad restart receipt is out of scope, same-process, or unverified.'
            }
            [void]$coveredFeatures.Add($primitiveFeature)
        }
        foreach ($requiredFeature in $requiredRestartFeatures) {
            if (-not $coveredFeatures.Contains($requiredFeature)) {
                throw "Broad restart evidence is missing Full feature $requiredFeature."
            }
        }
        $checkNames = @('stateConverged', 'noDuplicateState', 'samePackageReloaded')
        Assert-PartisanCompositeProperties `
            -Value $evidence.execution.checks `
            -Names $checkNames `
            -Label 'Broad restart checks' `
            -Exact
        foreach ($name in $checkNames) {
            if (-not (Assert-PartisanCompositeBoolean `
                    -Value $evidence.execution.checks.$name `
                    -Label "Broad restart check $name")) {
                throw 'Broad restart evidence contains an unproven required check.'
            }
        }
        $requiredRoles = @(
            'harness',
            'scenario_receipt',
            'server_log',
            'client_log'
        )
    }
    elseif ($scenarioId -ceq 'second_client_jip_reconnect') {
        if ($restartReceipts.Count -ne 0 -or
            $jipJoinCount -lt 1 -or
            $reconnectCount -lt 1) {
            throw 'Second-client evidence requires explicit JIP and reconnect, not restart receipts.'
        }
        $checkNames = @(
            'primaryClientConverged',
            'secondClientConverged',
            'jipConverged',
            'reconnectConverged'
        )
        Assert-PartisanCompositeProperties `
            -Value $evidence.execution.checks `
            -Names $checkNames `
            -Label 'Second-client checks' `
            -Exact
        foreach ($name in $checkNames) {
            if (-not (Assert-PartisanCompositeBoolean `
                    -Value $evidence.execution.checks.$name `
                    -Label "Second-client check $name")) {
                throw 'Second-client evidence contains an unproven required check.'
            }
        }
        $requiredRoles = @(
            'harness',
            'scenario_receipt',
            'server_log',
            'primary_client_log',
            'second_client_log'
        )
    }
    else {
        if ($restartReceipts.Count -ne 0) {
            throw 'Two-hour soak evidence must not widen itself into restart proof.'
        }
        $checkNames = @(
            'durationMet',
            'campaignTickErrorFree',
            'stateDriftFree',
            'processStable'
        )
        Assert-PartisanCompositeProperties `
            -Value $evidence.execution.checks `
            -Names $checkNames `
            -Label 'Two-hour soak checks' `
            -Exact
        foreach ($name in $checkNames) {
            if (-not (Assert-PartisanCompositeBoolean `
                    -Value $evidence.execution.checks.$name `
                    -Label "Two-hour soak check $name")) {
                throw 'Two-hour soak evidence contains an unproven required check.'
            }
        }
        $requiredRoles = @(
            'harness',
            'scenario_receipt',
            'server_log',
            'client_log',
            'telemetry'
        )
    }

    Assert-PartisanCompositeExternalTeardown -Teardown $evidence.teardown
    $verifiedFiles = @(Assert-PartisanCompositeRecordedFiles `
        -Root $root `
        -Rows $evidence.files `
        -Label "External scenario $scenarioId evidence" `
        -RequireRole)
    Assert-PartisanCompositeExternalRoles `
        -Rows $verifiedFiles `
        -RequiredRoles $requiredRoles `
        -ScenarioId $scenarioId
    $harnessRows = @($verifiedFiles | Where-Object {
        $_.Role -ceq 'harness'
    })
    if ($harnessRows.Count -ne 1 -or
        $harnessRows[0].Path -cne $harnessPath -or
        $harnessRows[0].Sha256 -cne $harnessSha) {
        throw 'External evidence harness identity is not backed by its hashed raw file.'
    }

    return [pscustomobject][ordered]@{
        EvidenceId = $evidenceId
        EvidenceKind = [string]$evidence.evidenceKind
        EnvelopeFileName = $input.FileName
        EnvelopeSha256 = $input.Sha256
        ScenarioId = $scenarioId
        ContractVersion = [int]$contractVersion
        AssertionIds = @($contract.AssertionIds)
        StartedUtc = $startedUtc
        CompletedUtc = $completedUtc
        DurationSeconds = $durationSeconds
        ClientCount = $clientCount
        JipJoinCount = $jipJoinCount
        ReconnectCount = $reconnectCount
        RestartReceiptCount = $restartReceipts.Count
        RestartFeatureCount = if ($scenarioId -ceq 'broad_restart') {
            @($PackagedRun.RequiredRestartFeatures).Count
        }
        else {
            0
        }
        WorldResource = [string]$evidence.execution.worldResource
        HarnessGitHead = [string]$evidence.harness.gitHead
        HarnessSha256 = $harnessSha
        RawEvidenceFileCount = $verifiedFiles.Count
    }
}

function Write-PartisanCompositeCertification {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string]$Path
    )

    $resolved = [IO.Path]::GetFullPath($Path)
    if ([IO.Path]::GetExtension($resolved) -cne '.json') {
        throw 'Composite certification output must use a .json extension.'
    }
    if (Test-Path -LiteralPath $resolved) {
        throw 'Composite certification output already exists.'
    }
    $parent = Split-Path -Parent $resolved
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        throw 'Composite certification output directory does not exist.'
    }
    $json = $Value | ConvertTo-Json -Depth 100
    if ($json -match '(?i)(?:[A-Z]:[\\/]|\\\\|file://|/(?:Users|home|mnt)/)') {
        throw 'Composite certification output would disclose a local absolute path.'
    }
    $partial = Join-Path $parent (
        '.' + (Split-Path -Leaf $resolved) + '.partial.' +
        [Guid]::NewGuid().ToString('N'))
    try {
        Write-PartisanCompositeUtf8File -Path $partial -Text ($json + "`n")
        Move-Item `
            -LiteralPath $partial `
            -Destination $resolved `
            -ErrorAction Stop
    }
    finally {
        if (Test-Path -LiteralPath $partial -PathType Leaf) {
            Remove-Item -LiteralPath $partial -Force -ErrorAction SilentlyContinue
        }
    }
}

function Invoke-PartisanCompositeCertification {
    param(
        [Parameter(Mandatory = $true)][string]$CandidateManifestPath,
        [Parameter(Mandatory = $true)][string]$PackagedRunPath,
        [string[]]$ExternalEvidencePaths = @(),
        [Parameter(Mandatory = $true)][string]$CertificationOutputPath
    )

    $candidateSeal = Get-PartisanCompositeCandidateSeal `
        -ManifestPath $CandidateManifestPath
    $packagedRun = Get-PartisanCompositePackagedRun `
        -CandidateSeal $candidateSeal `
        -RunPath $PackagedRunPath

    $externalByScenario = @{}
    $evidenceIds = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($path in @($ExternalEvidencePaths | Where-Object {
            -not [string]::IsNullOrWhiteSpace([string]$_)
        })) {
        $validated = Get-PartisanCompositeExternalEvidence `
            -CandidateSeal $candidateSeal `
            -PackagedRun $packagedRun `
            -EvidencePath ([string]$path)
        if ($externalByScenario.ContainsKey($validated.ScenarioId)) {
            throw "External scenario $($validated.ScenarioId) was supplied more than once."
        }
        if (-not $evidenceIds.Add($validated.EvidenceId)) {
            throw "External evidence id $($validated.EvidenceId) was supplied more than once."
        }
        $externalByScenario[$validated.ScenarioId] = $validated
    }

    $overlays = New-Object Collections.Generic.List[object]
    $satisfiedGapCount = 0
    foreach ($assertionId in $script:ManualGapAssertionOrder) {
        $scenarioId = ''
        foreach ($candidateScenario in $script:ScenarioOrder) {
            if ($script:ScenarioContracts[$candidateScenario].AssertionIds -ccontains
                $assertionId) {
                $scenarioId = $candidateScenario
                break
            }
        }
        if ([string]::IsNullOrWhiteSpace($scenarioId)) {
            throw "No explicit external scenario maps manual gap $assertionId."
        }
        $gap = $packagedRun.ManualGaps[$assertionId]
        $externalDisposition = 'unreconciled'
        $externalProof = $null
        if ($externalByScenario.ContainsKey($scenarioId)) {
            $evidence = $externalByScenario[$scenarioId]
            $externalDisposition = 'externally_satisfied'
            $satisfiedGapCount++
            $externalProof = [pscustomobject][ordered]@{
                evidenceId = $evidence.EvidenceId
                evidenceKind = $evidence.EvidenceKind
                scenarioId = $evidence.ScenarioId
                contractVersion = $evidence.ContractVersion
                envelopeFileName = $evidence.EnvelopeFileName
                envelopeSha256 = $evidence.EnvelopeSha256
            }
        }
        [void]$overlays.Add([pscustomobject][ordered]@{
            assertionId = $assertionId
            originalCaseId = $gap.CaseId
            originalAssertion = $gap.Assertion
            externalScenarioId = $scenarioId
            externalDisposition = $externalDisposition
            externalProof = $externalProof
        })
    }

    $externalSummaries = New-Object Collections.Generic.List[object]
    foreach ($scenarioId in $script:ScenarioOrder) {
        if (-not $externalByScenario.ContainsKey($scenarioId)) {
            continue
        }
        $evidence = $externalByScenario[$scenarioId]
        [void]$externalSummaries.Add([pscustomobject][ordered]@{
            evidenceId = $evidence.EvidenceId
            evidenceKind = $evidence.EvidenceKind
            scenarioId = $evidence.ScenarioId
            contractVersion = $evidence.ContractVersion
            assertionIds = @($evidence.AssertionIds)
            envelopeFileName = $evidence.EnvelopeFileName
            envelopeSha256 = $evidence.EnvelopeSha256
            startedUtc = $evidence.StartedUtc.ToString('o')
            completedUtc = $evidence.CompletedUtc.ToString('o')
            durationSeconds = $evidence.DurationSeconds
            clientCount = $evidence.ClientCount
            jipJoinCount = $evidence.JipJoinCount
            reconnectCount = $evidence.ReconnectCount
            restartReceiptCount = $evidence.RestartReceiptCount
            restartFeatureCount = $evidence.RestartFeatureCount
            worldResource = $evidence.WorldResource
            harnessGitHead = $evidence.HarnessGitHead
            harnessSha256 = $evidence.HarnessSha256
            rawEvidenceFileCount = $evidence.RawEvidenceFileCount
        })
    }

    $decisionReasons = New-Object Collections.Generic.List[string]
    foreach ($problem in @($packagedRun.AcceptanceProblems)) {
        [void]$decisionReasons.Add($problem)
    }
    foreach ($scenarioId in $script:ScenarioOrder) {
        if (-not $externalByScenario.ContainsKey($scenarioId)) {
            [void]$decisionReasons.Add("external_scenario_missing:$scenarioId")
        }
    }
    $go = $packagedRun.Accepted -and
        $externalByScenario.Count -eq $script:ScenarioOrder.Count -and
        $satisfiedGapCount -eq $script:ManualGapAssertionOrder.Count
    $generatedUtc = [DateTimeOffset]::UtcNow
    $certificationId = $candidateSeal.Identity.candidateId + '-composite-' +
        $generatedUtc.ToString('yyyyMMddTHHmmssZ')
    $result = [pscustomobject][ordered]@{
        schemaVersion = $script:CompositeCertificationSchema
        certificationKind = $script:CompositeCertificationKind
        certificationId = $certificationId
        generatedUtc = $generatedUtc.ToString('o')
        candidate = $candidateSeal.Identity
        sourceEvidence = [pscustomobject][ordered]@{
            candidateSeal = [pscustomobject][ordered]@{
                manifestFileName = $candidateSeal.ManifestInput.FileName
                manifestSha256 = $candidateSeal.ManifestInput.Sha256
                readyFileName = $candidateSeal.ReadyInput.FileName
                readySha256 = $candidateSeal.ReadyInput.Sha256
                packageSha256 = $candidateSeal.Identity.packageSha256
                packageFileCount = $candidateSeal.PackageFileCount
                candidateEvidenceFileCount =
                    $candidateSeal.CandidateEvidenceFileCount
            }
            packagedCampaignDebug = [pscustomobject][ordered]@{
                envelopeFileName = $packagedRun.RunInput.FileName
                envelopeSha256 = $packagedRun.RunInput.Sha256
                evidenceKind = [string]$packagedRun.Run.evidenceKind
                startedUtc = $packagedRun.StartedUtc.ToString('o')
                completedUtc = $packagedRun.CompletedUtc.ToString('o')
                runId = [string]$packagedRun.Artifact.m_sRunId
                verifiedFileCount = $packagedRun.VerifiedFileCount
                rawCampaignDebugArtifacts = @($packagedRun.RawArtifacts)
            }
            externalEvidence = @($externalSummaries.ToArray())
        }
        campaignDebug = [pscustomobject][ordered]@{
            runId = [string]$packagedRun.Artifact.m_sRunId
            profile = [string]$packagedRun.Artifact.m_sProfile
            originalInProcessCertificationPassed =
                [bool]$packagedRun.Artifact.m_bCertificationPassed
            packagedEvidenceAccepted = [bool]$packagedRun.Accepted
            packagedEvidenceProblems = @($packagedRun.AcceptanceProblems)
            originalManualGapCount = $script:ManualGapAssertionOrder.Count
            requiredRestartFeatures = @($packagedRun.RequiredRestartFeatures)
        }
        overlays = @($overlays.ToArray())
        overall = [pscustomobject][ordered]@{
            decision = if ($go) { 'GO' } else { 'NO-GO' }
            inProcessAccepted = [bool]$packagedRun.Accepted
            requiredScenarioCount = $script:ScenarioOrder.Count
            satisfiedScenarioCount = $externalByScenario.Count
            requiredGapCount = $script:ManualGapAssertionOrder.Count
            externallySatisfiedGapCount = $satisfiedGapCount
            unreconciledGapCount =
                $script:ManualGapAssertionOrder.Count - $satisfiedGapCount
            reasons = @($decisionReasons.ToArray())
        }
    }
    Write-PartisanCompositeCertification `
        -Value $result `
        -Path $CertificationOutputPath
    return $result
}

function New-PartisanCompositeSelfTestExecutableIdentity {
    param(
        [Parameter(Mandatory = $true)][string]$FileName,
        [Parameter(Mandatory = $true)][string]$Token
    )

    return [pscustomobject][ordered]@{
        fileName = $FileName
        fileVersion = '1.0.0.1'
        productVersion = '1.0.0.1'
        length = 100
        sha256 = Get-PartisanCompositeTextSha256 -Text $Token
    }
}

function New-PartisanCompositeSelfTestCandidate {
    param([Parameter(Mandatory = $true)][string]$Root)

    $candidateId = 'partisan-composite-selftest'
    $bundleRoot = Join-Path $Root $candidateId
    $packageRoot = Join-Path $bundleRoot 'package\Partisan'
    $evidenceRoot = Join-Path $bundleRoot 'evidence\pack'
    foreach ($directory in @($bundleRoot, $packageRoot, $evidenceRoot)) {
        [void](New-Item -ItemType Directory -Path $directory -Force)
    }
    $packageContents = [ordered]@{
        'addon.gproj' = 'selftest project'
        'data.pak' = 'selftest package data'
        'resourceDatabase.rdb' = 'selftest resource database'
        'thumbnail.png' = 'selftest thumbnail'
    }
    $packageRows = New-Object Collections.Generic.List[object]
    $indexRows = New-Object Collections.Generic.List[string]
    foreach ($name in @($packageContents.Keys | Sort-Object)) {
        $path = Join-Path $packageRoot $name
        Write-PartisanCompositeUtf8File `
            -Path $path `
            -Text ([string]$packageContents[$name])
        $file = Get-Item -LiteralPath $path -Force
        $sha = Get-PartisanCompositeSha256 -Path $path
        $indexPath = 'Partisan/' + $name
        [void]$packageRows.Add([pscustomobject][ordered]@{
            path = 'package/' + $indexPath
            indexPath = $indexPath
            length = [long]$file.Length
            sha256 = $sha
        })
        [void]$indexRows.Add(("{0}`t{1}`t{2}" -f
            $sha,
            [long]$file.Length,
            $indexPath))
    }
    $indexText = ($indexRows.ToArray() -join "`n") + "`n"
    $indexPath = Join-Path $evidenceRoot 'files.sha256'
    Write-PartisanCompositeUtf8File -Path $indexPath -Text $indexText
    $packageSha = Get-PartisanCompositeSha256 -Path $indexPath
    $indexFile = Get-Item -LiteralPath $indexPath -Force
    $createdUtc = [DateTimeOffset]::UtcNow.AddHours(-6)
    $toolchain = [pscustomobject][ordered]@{
        workbench = New-PartisanCompositeSelfTestExecutableIdentity `
            -FileName 'WorkbenchDiag.exe' `
            -Token 'workbench'
        server = New-PartisanCompositeSelfTestExecutableIdentity `
            -FileName 'Server.exe' `
            -Token 'server'
        serverDiagnostic = New-PartisanCompositeSelfTestExecutableIdentity `
            -FileName 'ServerDiag.exe' `
            -Token 'server-diagnostic'
        client = New-PartisanCompositeSelfTestExecutableIdentity `
            -FileName 'Client.exe' `
            -Token 'client'
        clientDiagnostic = New-PartisanCompositeSelfTestExecutableIdentity `
            -FileName 'ClientDiag.exe' `
            -Token 'client-diagnostic'
    }
    $manifest = [pscustomobject][ordered]@{
        manifestSchemaVersion = 1
        createdUtc = $createdUtc.ToString('o')
        candidate = [pscustomobject][ordered]@{
            id = $candidateId
            version = '0.0.0-selftest'
            state = 'retained-uncertified'
        }
        source = [pscustomobject][ordered]@{
            gitHead = '1111111111111111111111111111111111111111'
            dirty = $false
            embeddedImplementation = [pscustomobject][ordered]@{
                sha = '2222222222222222222222222222222222222222'
                utc = $createdUtc.AddMinutes(-1).ToString('o')
                label = 'composite-selftest'
            }
            campaignSchema = 71
            runtimeSettingsSchema = 24
        }
        addon = [pscustomobject][ordered]@{
            id = 'histasi'
            guid = '0123456789ABCDEF'
        }
        toolchain = $toolchain
        workbench = [pscustomobject][ordered]@{
            crc = 'abcdef12'
        }
        package = [pscustomobject][ordered]@{
            root = 'package/Partisan'
            hashAlgorithm = 'sha256-manifest-v1'
            sha256 = $packageSha
            canonicalIndexPath = 'evidence/pack/files.sha256'
            files = @($packageRows.ToArray())
        }
        evidence = [pscustomobject][ordered]@{
            root = 'evidence'
            files = @([pscustomobject][ordered]@{
                path = 'evidence/pack/files.sha256'
                length = [long]$indexFile.Length
                sha256 = $packageSha
            })
        }
    }
    $manifestPath = Join-Path $bundleRoot 'candidate.json'
    Write-PartisanCompositeUtf8File `
        -Path $manifestPath `
        -Text (($manifest | ConvertTo-Json -Depth 30) + "`n")
    $manifestSha = Get-PartisanCompositeSha256 -Path $manifestPath
    $ready = [pscustomobject][ordered]@{
        schemaVersion = 1
        candidateId = $candidateId
        gitHead = [string]$manifest.source.gitHead
        packageSha256 = $packageSha
        manifestSha256 = $manifestSha
    }
    Write-PartisanCompositeUtf8File `
        -Path (Join-Path $bundleRoot 'candidate.ready.json') `
        -Text (($ready | ConvertTo-Json -Depth 10) + "`n")
    return [pscustomobject][ordered]@{
        ManifestPath = $manifestPath
        Toolchain = $toolchain
    }
}

function New-PartisanCompositeSelfTestAssertion {
    param(
        [Parameter(Mandatory = $true)][string]$AssertionId,
        [Parameter(Mandatory = $true)][string]$Expected,
        [Parameter(Mandatory = $true)][string]$Actual
    )

    return [pscustomobject][ordered]@{
        m_sAssertionId = $AssertionId
        m_sExpected = $Expected
        m_sActual = $Actual
        m_sStatus = 'BLOCKED'
        m_sFailureReason = 'external harness remains required'
        m_sProofLevel = 'EXTERNAL_PROCESS'
        m_sObservedPath = 'manual_external_gap'
        m_sRequiredPath = 'external process harness'
        m_bCountsTowardCertification = $false
    }
}

function New-PartisanCompositeSelfTestPackagedRun {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)]$CandidateSeal
    )

    $runRoot = Join-Path $Root 'packaged-full'
    $campaignRoot = Join-Path $runRoot 'raw\campaign-debug'
    $identityRoot = Join-Path $runRoot 'identity'
    foreach ($directory in @($runRoot, $campaignRoot, $identityRoot)) {
        [void](New-Item -ItemType Directory -Path $directory -Force)
    }
    Copy-Item `
        -LiteralPath $CandidateSeal.ManifestInput.Path `
        -Destination (Join-Path $identityRoot 'candidate.json')
    Copy-Item `
        -LiteralPath $CandidateSeal.ReadyInput.Path `
        -Destination (Join-Path $identityRoot 'candidate.ready.json')

    $persistenceAssertion = New-PartisanCompositeSelfTestAssertion `
        -AssertionId 'persistence.real_restart' `
        -Expected 'restart is explicitly not executed here' `
        -Actual 'not executed'
    $phaseRestartAssertion = New-PartisanCompositeSelfTestAssertion `
        -AssertionId 'phase25.real_restart' `
        -Expected 'restart-after-primitive is explicitly not executed here' `
        -Actual 'manual external gap'
    $secondClientAssertion = New-PartisanCompositeSelfTestAssertion `
        -AssertionId 'phase25.second_client' `
        -Expected 'second-client JIP and reconnect are explicitly not executed here' `
        -Actual 'manual external gap'
    $soakAssertion = New-PartisanCompositeSelfTestAssertion `
        -AssertionId 'phase25.two_hour_soak' `
        -Expected 'two-hour soak is explicitly not executed here' `
        -Actual 'manual external gap'
    $artifact = [pscustomobject][ordered]@{
        m_sRunId = 'composite_selftest_full'
        m_sProfile = 'full_certification'
        m_sBuildSha = $CandidateSeal.Identity.embeddedBuildSha
        m_sBuildUtc = $CandidateSeal.Identity.embeddedBuildUtc
        m_sBuildLabel = $CandidateSeal.Identity.embeddedBuildLabel
        m_bCertificationPassed = $true
        m_aCases = @(
            [pscustomobject][ordered]@{
                m_sCaseId = 'persistence.seeded_roundtrip.phase12'
                m_sCategory = 'persistence'
                m_sFeature = 'persistence_smoke'
                m_sStage = 'early_phase'
                m_aAssertions = @($persistenceAssertion)
            },
            [pscustomobject][ordered]@{
                m_sCaseId = 'phase25.manual_external_gaps'
                m_sCategory = 'soak'
                m_sFeature = 'external_harness'
                m_sStage = 'final'
                m_aAssertions = @(
                    $phaseRestartAssertion,
                    $secondClientAssertion,
                    $soakAssertion
                )
            },
            [pscustomobject][ordered]@{
                m_sCaseId = 'selftest.primitive.clear_area'
                m_sCategory = 'mission_runtime'
                m_sFeature = 'clear_area'
                m_sStage = 'primitive_probe'
                m_aAssertions = @()
            },
            [pscustomobject][ordered]@{
                m_sCaseId = 'selftest.primitive.destroy_target'
                m_sCategory = 'mission_runtime'
                m_sFeature = 'destroy_target'
                m_sStage = 'primitive_probe'
                m_aAssertions = @()
            },
            [pscustomobject][ordered]@{
                m_sCaseId = 'selftest.physical.convoy'
                m_sCategory = 'mission_runtime'
                m_sFeature = 'convoy_intercept'
                m_sStage = 'physical_probe'
                m_aAssertions = @()
            },
            [pscustomobject][ordered]@{
                m_sCaseId = 'selftest.physical.generic_contact'
                m_sCategory = 'mission_runtime'
                m_sFeature = 'ai_combat_contact'
                m_sStage = 'physical_probe'
                m_aAssertions = @()
            }
        )
    }
    $baseName = 'HST_CampaignDebug_composite_selftest_full'
    $artifactPath = Join-Path $campaignRoot ($baseName + '.json')
    $summaryPath = Join-Path $campaignRoot ($baseName + '_summary.txt')
    $stateDiffPath = Join-Path $campaignRoot ($baseName + '_state_diff.txt')
    Write-PartisanCompositeUtf8File `
        -Path $artifactPath `
        -Text (($artifact | ConvertTo-Json -Depth 30) + "`n")
    Write-PartisanCompositeUtf8File `
        -Path $summaryPath `
        -Text "selftest summary`n"
    Write-PartisanCompositeUtf8File `
        -Path $stateDiffPath `
        -Text "selftest state diff`n"

    $fileRows = New-Object Collections.Generic.List[object]
    foreach ($row in @(
            [pscustomobject]@{
                Path = Join-Path $identityRoot 'candidate.json'
                Portable = 'identity/candidate.json'
            },
            [pscustomobject]@{
                Path = Join-Path $identityRoot 'candidate.ready.json'
                Portable = 'identity/candidate.ready.json'
            },
            [pscustomobject]@{
                Path = $artifactPath
                Portable = 'raw/campaign-debug/' + $baseName + '.json'
            },
            [pscustomobject]@{
                Path = $summaryPath
                Portable = 'raw/campaign-debug/' + $baseName + '_summary.txt'
            },
            [pscustomobject]@{
                Path = $stateDiffPath
                Portable = 'raw/campaign-debug/' + $baseName + '_state_diff.txt'
            })) {
        $file = Get-Item -LiteralPath $row.Path -Force
        [void]$fileRows.Add([pscustomobject][ordered]@{
            path = $row.Portable
            length = [long]$file.Length
            sha256 = Get-PartisanCompositeSha256 -Path $row.Path
        })
    }
    $startedUtc = [DateTimeOffset]::UtcNow.AddHours(-5)
    $completedUtc = $startedUtc.AddHours(1)
    $runtimeIdentity = $CandidateSeal.Toolchain.client
    $diagnosticIdentity = $CandidateSeal.Toolchain.clientDiagnostic
    $runCandidate = [pscustomobject][ordered]@{}
    foreach ($property in $CandidateSeal.Identity.PSObject.Properties) {
        Add-Member `
            -InputObject $runCandidate `
            -MemberType NoteProperty `
            -Name $property.Name `
            -Value $property.Value
    }
    Add-Member -InputObject $runCandidate -MemberType NoteProperty `
        -Name runtimeUseDisposition -Value 'active-runtime-candidate'
    Add-Member -InputObject $runCandidate -MemberType NoteProperty `
        -Name runtimeRole -Value 'client'
    Add-Member -InputObject $runCandidate -MemberType NoteProperty `
        -Name diagnosticExecutable -Value $diagnosticIdentity
    Add-Member -InputObject $runCandidate -MemberType NoteProperty `
        -Name recordedDiagnosticExecutable -Value $diagnosticIdentity
    Add-Member -InputObject $runCandidate -MemberType NoteProperty `
        -Name recordedRuntimeExecutable -Value $runtimeIdentity
    $run = [pscustomobject][ordered]@{
        schemaVersion = 1
        evidenceKind = 'packaged-campaign-debug'
        startedUtc = $startedUtc.ToString('o')
        completedUtc = $completedUtc.ToString('o')
        candidate = $runCandidate
        harness = [pscustomobject][ordered]@{
            gitHead = '3333333333333333333333333333333333333333'
            dirty = $false
            campaignRunnerSha256 = Get-PartisanCompositeTextSha256 `
                -Text 'campaign-runner'
            candidateModuleSha256 = Get-PartisanCompositeTextSha256 `
                -Text 'candidate-module'
        }
        launch = [pscustomobject][ordered]@{
            profile = 'full_certification'
            proofScope = 'full_certification'
            worldResource = 'Worlds/HST_Dev/HST_Dev.ent'
            stagedPackage = $true
            addonGuid = $CandidateSeal.Identity.addonGuid
            packageSha256 = $CandidateSeal.Identity.packageSha256
            diagnosticExecutable = $diagnosticIdentity
            recordedRuntimeExecutable = $runtimeIdentity
        }
        outcome = [pscustomobject][ordered]@{
            success = $true
            armed = $true
            started = $true
            completed = $true
            candidateBoundaryVerified = $true
            mountAttestation = [pscustomobject][ordered]@{
                Valid = $true
                GuidExact = $true
                Packed = $true
            }
            artifactsStable = $true
            evidenceCaptured = $true
            validation = [pscustomobject][ordered]@{
                Valid = $true
                RunId = [string]$artifact.m_sRunId
                Profile = 'full_certification'
                ProofScope = 'full_certification'
                FullCertification = $true
                BuildProvenanceMatches = $true
                CertificationPassed = $true
            }
            errorCensus = [pscustomobject][ordered]@{
                Valid = $true
                HardDiagnosticFree = $true
                UnapprovedHardDiagnosticCount = 0
            }
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
        files = @($fileRows.ToArray())
    }
    $runPath = Join-Path $runRoot 'run.json'
    Write-PartisanCompositeUtf8File `
        -Path $runPath `
        -Text (($run | ConvertTo-Json -Depth 40) + "`n")
    return [pscustomobject][ordered]@{
        RunPath = $runPath
        RunRoot = $runRoot
        ArtifactPath = $artifactPath
        StartedUtc = $startedUtc
        CompletedUtc = $completedUtc
        RequiredRestartFeatures = @(
            'clear_area',
            'convoy_intercept',
            'destroy_target'
        )
    }
}

function New-PartisanCompositeSelfTestExternalEvidence {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)]$CandidateSeal,
        [Parameter(Mandatory = $true)][string]$ScenarioId,
        [Parameter(Mandatory = $true)][DateTimeOffset]$StartedUtc,
        [Parameter(Mandatory = $true)][long]$DurationSeconds,
        [string[]]$BroadRestartFeatures = @()
    )

    $scenarioRoot = Join-Path $Root $ScenarioId
    [void](New-Item -ItemType Directory -Path $scenarioRoot -Force)
    $roleFiles = [ordered]@{
        harness = 'harness.ps1'
        scenario_receipt = 'scenario-receipt.json'
        server_log = 'server.log'
    }
    if ($ScenarioId -ceq 'second_client_jip_reconnect') {
        $roleFiles.primary_client_log = 'primary-client.log'
        $roleFiles.second_client_log = 'second-client.log'
    }
    else {
        $roleFiles.client_log = 'client.log'
    }
    if ($ScenarioId -ceq 'two_hour_soak') {
        $roleFiles.telemetry = 'telemetry.json'
    }
    $fileRows = New-Object Collections.Generic.List[object]
    foreach ($role in $roleFiles.Keys) {
        $name = [string]$roleFiles[$role]
        $path = Join-Path $scenarioRoot $name
        Write-PartisanCompositeUtf8File `
            -Path $path `
            -Text ("$ScenarioId $role selftest`n")
        $file = Get-Item -LiteralPath $path -Force
        [void]$fileRows.Add([pscustomobject][ordered]@{
            path = $name
            role = [string]$role
            length = [long]$file.Length
            sha256 = Get-PartisanCompositeSha256 -Path $path
        })
    }
    $harnessRow = @($fileRows.ToArray() | Where-Object {
        $_.role -ceq 'harness'
    })[0]
    if ($ScenarioId -ceq 'broad_restart') {
        if (@($BroadRestartFeatures).Count -eq 0) {
            throw 'The self-test broad restart fixture needs derived features.'
        }
        $clientCount = 1
        $jipCount = 0
        $receiptRows = New-Object Collections.Generic.List[object]
        for ($index = 0; $index -lt $BroadRestartFeatures.Count; $index++) {
            [void]$receiptRows.Add([pscustomobject][ordered]@{
                primitiveFeature = [string]$BroadRestartFeatures[$index]
                beforeProcessId = 101 + $index
                afterProcessId = 102 + $index
                stateVerified = $true
                clientReconnected = $true
                samePackageVerified = $true
            })
        }
        $restartReceipts = @($receiptRows.ToArray())
        $reconnectCount = $restartReceipts.Count
        $checks = [pscustomobject][ordered]@{
            stateConverged = $true
            noDuplicateState = $true
            samePackageReloaded = $true
        }
    }
    elseif ($ScenarioId -ceq 'second_client_jip_reconnect') {
        $clientCount = 2
        $jipCount = 1
        $reconnectCount = 1
        $restartReceipts = @()
        $checks = [pscustomobject][ordered]@{
            primaryClientConverged = $true
            secondClientConverged = $true
            jipConverged = $true
            reconnectConverged = $true
        }
    }
    else {
        $clientCount = 2
        $jipCount = 0
        $reconnectCount = 0
        $restartReceipts = @()
        $checks = [pscustomobject][ordered]@{
            durationMet = $true
            campaignTickErrorFree = $true
            stateDriftFree = $true
            processStable = $true
        }
    }
    $candidateBinding = [pscustomobject][ordered]@{}
    foreach ($property in $CandidateSeal.Identity.PSObject.Properties) {
        Add-Member `
            -InputObject $candidateBinding `
            -MemberType NoteProperty `
            -Name $property.Name `
            -Value $property.Value
    }
    $contract = $script:ScenarioContracts[$ScenarioId]
    $evidence = [pscustomobject][ordered]@{
        schemaVersion = 1
        evidenceKind = 'partisan-external-certification-proof'
        evidenceId = 'selftest-' + $ScenarioId
        startedUtc = $StartedUtc.ToString('o')
        completedUtc = $StartedUtc.AddSeconds($DurationSeconds).ToString('o')
        candidate = $candidateBinding
        runtime = [pscustomobject][ordered]@{
            serverExecutable = $CandidateSeal.Toolchain.server
            clientExecutable = $CandidateSeal.Toolchain.client
        }
        harness = [pscustomobject][ordered]@{
            gitHead = '4444444444444444444444444444444444444444'
            dirty = $false
            path = [string]$harnessRow.path
            sha256 = [string]$harnessRow.sha256
        }
        scenario = [pscustomobject][ordered]@{
            id = $ScenarioId
            contractVersion = [int]$contract.ContractVersion
            assertionIds = @($contract.AssertionIds)
        }
        execution = [pscustomobject][ordered]@{
            executed = $true
            success = $true
            worldResource = 'Worlds/Partisan/Certification.ent'
            durationSeconds = $DurationSeconds
            clientCount = $clientCount
            restartReceipts = $restartReceipts
            jipJoinCount = $jipCount
            reconnectCount = $reconnectCount
            checks = $checks
        }
        teardown = [pscustomobject][ordered]@{
            clean = $true
            guardEntriesRemaining = 0
            ownedProcessesRemaining = 0
            profileEntriesRemaining = 0
            errors = @()
        }
        files = @($fileRows.ToArray())
    }
    $evidencePath = Join-Path $scenarioRoot 'evidence.json'
    Write-PartisanCompositeUtf8File `
        -Path $evidencePath `
        -Text (($evidence | ConvertTo-Json -Depth 40) + "`n")
    return $evidencePath
}

function Assert-PartisanCompositeSelfTest {
    param(
        [Parameter(Mandatory = $true)][bool]$Condition,
        [Parameter(Mandatory = $true)][string]$Message
    )

    if (-not $Condition) {
        throw "Composite certification self-test failed: $Message"
    }
}

function Assert-PartisanCompositeSelfTestThrows {
    param(
        [Parameter(Mandatory = $true)][scriptblock]$Action,
        [Parameter(Mandatory = $true)][string]$Message
    )

    $thrown = $false
    try {
        & $Action | Out-Null
    }
    catch {
        $thrown = $true
    }
    Assert-PartisanCompositeSelfTest -Condition $thrown -Message $Message
}

function Invoke-PartisanCompositeSelfTest {
    $tempParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/')
    $tempRoot = Join-Path $tempParent (
        'partisan-composite-certification-selftest-' +
        [Guid]::NewGuid().ToString('N'))
    $caseCount = 0
    try {
        [void](New-Item -ItemType Directory -Path $tempRoot -Force)
        $candidateFixture = New-PartisanCompositeSelfTestCandidate -Root $tempRoot
        $candidateSeal = Get-PartisanCompositeCandidateSeal `
            -ManifestPath $candidateFixture.ManifestPath
        $runFixture = New-PartisanCompositeSelfTestPackagedRun `
            -Root $tempRoot `
            -CandidateSeal $candidateSeal
        $externalRoot = Join-Path $tempRoot 'external'
        [void](New-Item -ItemType Directory -Path $externalRoot -Force)
        $broadPath = New-PartisanCompositeSelfTestExternalEvidence `
            -Root $externalRoot `
            -CandidateSeal $candidateSeal `
            -ScenarioId 'broad_restart' `
            -StartedUtc $runFixture.CompletedUtc.AddMinutes(1) `
            -DurationSeconds 120 `
            -BroadRestartFeatures $runFixture.RequiredRestartFeatures
        $secondPath = New-PartisanCompositeSelfTestExternalEvidence `
            -Root $externalRoot `
            -CandidateSeal $candidateSeal `
            -ScenarioId 'second_client_jip_reconnect' `
            -StartedUtc $runFixture.CompletedUtc.AddMinutes(10) `
            -DurationSeconds 120
        $soakPath = New-PartisanCompositeSelfTestExternalEvidence `
            -Root $externalRoot `
            -CandidateSeal $candidateSeal `
            -ScenarioId 'two_hour_soak' `
            -StartedUtc $runFixture.CompletedUtc.AddMinutes(20) `
            -DurationSeconds 7200

        $missingOutput = Join-Path $tempRoot 'missing-evidence-certification.json'
        $missing = Invoke-PartisanCompositeCertification `
            -CandidateManifestPath $candidateFixture.ManifestPath `
            -PackagedRunPath $runFixture.RunPath `
            -CertificationOutputPath $missingOutput
        Assert-PartisanCompositeSelfTest `
            -Condition ($missing.overall.decision -ceq 'NO-GO' -and
                [int]$missing.overall.unreconciledGapCount -eq 4 -and
                @($missing.overlays | Where-Object {
                    $_.externalDisposition -ceq 'unreconciled'
                }).Count -eq 4) `
            -Message 'missing evidence did not fail closed as NO-GO'
        $serializedMissing = [IO.File]::ReadAllText($missingOutput) |
            ConvertFrom-Json
        $expectedOriginals = @{
            'persistence.real_restart' = [pscustomobject]@{
                Expected = 'restart is explicitly not executed here'
                Actual = 'not executed'
            }
            'phase25.real_restart' = [pscustomobject]@{
                Expected = 'restart-after-primitive is explicitly not executed here'
                Actual = 'manual external gap'
            }
            'phase25.second_client' = [pscustomobject]@{
                Expected = 'second-client JIP and reconnect are explicitly not executed here'
                Actual = 'manual external gap'
            }
            'phase25.two_hour_soak' = [pscustomobject]@{
                Expected = 'two-hour soak is explicitly not executed here'
                Actual = 'manual external gap'
            }
        }
        foreach ($overlay in @($serializedMissing.overlays)) {
            $expectedOriginal = $expectedOriginals[[string]$overlay.assertionId]
            Assert-PartisanCompositeSelfTest `
                -Condition ($null -ne $expectedOriginal -and
                    [string]$overlay.originalAssertion.m_sStatus -ceq 'BLOCKED' -and
                    [string]$overlay.originalAssertion.m_sExpected -ceq
                        [string]$expectedOriginal.Expected -and
                    [string]$overlay.originalAssertion.m_sActual -ceq
                        [string]$expectedOriginal.Actual) `
                -Message 'an original manual-gap assertion was not preserved'
        }
        $caseCount++

        $goOutput = Join-Path $tempRoot 'go-certification.json'
        $go = Invoke-PartisanCompositeCertification `
            -CandidateManifestPath $candidateFixture.ManifestPath `
            -PackagedRunPath $runFixture.RunPath `
            -ExternalEvidencePaths @($broadPath, $secondPath, $soakPath) `
            -CertificationOutputPath $goOutput
        Assert-PartisanCompositeSelfTest `
            -Condition ($go.overall.decision -ceq 'GO' -and
                [int]$go.overall.satisfiedScenarioCount -eq 3 -and
                [int]$go.overall.externallySatisfiedGapCount -eq 4 -and
                @($go.overlays | Where-Object {
                    $_.externalDisposition -ceq 'externally_satisfied'
                }).Count -eq 4) `
            -Message 'complete exact evidence did not produce GO'
        $broadOverlays = @($go.overlays | Where-Object {
            $_.externalScenarioId -ceq 'broad_restart'
        })
        Assert-PartisanCompositeSelfTest `
            -Condition ($broadOverlays.Count -eq 2 -and
                @($go.overlays | Where-Object {
                    $_.externalScenarioId -ceq 'second_client_jip_reconnect'
                }).Count -eq 1 -and
                @($go.overlays | Where-Object {
                    $_.externalScenarioId -ceq 'two_hour_soak'
                }).Count -eq 1) `
            -Message 'explicit scenario mapping widened or narrowed evidence'
        $caseCount++

        $badCandidatePath = Join-Path (Split-Path -Parent $broadPath) `
            'bad-candidate.json'
        $badCandidate = [IO.File]::ReadAllText($broadPath) | ConvertFrom-Json
        $badCandidate.candidate.packageSha256 = 'f' * 64
        Write-PartisanCompositeUtf8File `
            -Path $badCandidatePath `
            -Text (($badCandidate | ConvertTo-Json -Depth 40) + "`n")
        Assert-PartisanCompositeSelfTestThrows `
            -Action {
                Invoke-PartisanCompositeCertification `
                    -CandidateManifestPath $candidateFixture.ManifestPath `
                    -PackagedRunPath $runFixture.RunPath `
                    -ExternalEvidencePaths @(
                        $badCandidatePath,
                        $secondPath,
                        $soakPath) `
                    -CertificationOutputPath (
                        Join-Path $tempRoot 'bad-candidate-output.json')
            } `
            -Message 'mismatched candidate evidence was accepted'
        $caseCount++

        $widenedPath = Join-Path (Split-Path -Parent $broadPath) `
            'widened.json'
        $widened = [IO.File]::ReadAllText($broadPath) | ConvertFrom-Json
        $widened.scenario.assertionIds = @(
            'persistence.real_restart',
            'phase25.real_restart',
            'phase25.two_hour_soak'
        )
        Write-PartisanCompositeUtf8File `
            -Path $widenedPath `
            -Text (($widened | ConvertTo-Json -Depth 40) + "`n")
        Assert-PartisanCompositeSelfTestThrows `
            -Action {
                Invoke-PartisanCompositeCertification `
                    -CandidateManifestPath $candidateFixture.ManifestPath `
                    -PackagedRunPath $runFixture.RunPath `
                    -ExternalEvidencePaths @(
                        $widenedPath,
                        $secondPath,
                        $soakPath) `
                    -CertificationOutputPath (
                        Join-Path $tempRoot 'widened-output.json')
            } `
            -Message 'evidence widening was accepted'
        $caseCount++

        $duplicateFeaturePath = Join-Path (Split-Path -Parent $broadPath) `
            'duplicate-feature.json'
        $duplicateFeature = [IO.File]::ReadAllText($broadPath) |
            ConvertFrom-Json
        $firstFeature = [string]$duplicateFeature.execution.restartReceipts[0].primitiveFeature
        foreach ($receipt in @($duplicateFeature.execution.restartReceipts)) {
            $receipt.primitiveFeature = $firstFeature
        }
        Write-PartisanCompositeUtf8File `
            -Path $duplicateFeaturePath `
            -Text (($duplicateFeature | ConvertTo-Json -Depth 40) + "`n")
        Assert-PartisanCompositeSelfTestThrows `
            -Action {
                Invoke-PartisanCompositeCertification `
                    -CandidateManifestPath $candidateFixture.ManifestPath `
                    -PackagedRunPath $runFixture.RunPath `
                    -ExternalEvidencePaths @(
                        $duplicateFeaturePath,
                        $secondPath,
                        $soakPath) `
                    -CertificationOutputPath (
                        Join-Path $tempRoot 'duplicate-feature-output.json')
            } `
            -Message 'duplicate restart features reduced the required coverage set'
        $caseCount++

        $shortSoakPath = Join-Path (Split-Path -Parent $soakPath) `
            'short-soak.json'
        $shortSoak = [IO.File]::ReadAllText($soakPath) | ConvertFrom-Json
        $shortSoak.execution.durationSeconds = 7199
        Write-PartisanCompositeUtf8File `
            -Path $shortSoakPath `
            -Text (($shortSoak | ConvertTo-Json -Depth 40) + "`n")
        Assert-PartisanCompositeSelfTestThrows `
            -Action {
                Invoke-PartisanCompositeCertification `
                    -CandidateManifestPath $candidateFixture.ManifestPath `
                    -PackagedRunPath $runFixture.RunPath `
                    -ExternalEvidencePaths @(
                        $broadPath,
                        $secondPath,
                        $shortSoakPath) `
                    -CertificationOutputPath (
                        Join-Path $tempRoot 'short-soak-output.json')
            } `
            -Message 'sub-threshold soak evidence was accepted'
        $caseCount++

        $dirtyTeardownPath = Join-Path (Split-Path -Parent $broadPath) `
            'dirty-teardown.json'
        $dirtyTeardown = [IO.File]::ReadAllText($broadPath) | ConvertFrom-Json
        $dirtyTeardown.teardown.clean = $false
        Write-PartisanCompositeUtf8File `
            -Path $dirtyTeardownPath `
            -Text (($dirtyTeardown | ConvertTo-Json -Depth 40) + "`n")
        Assert-PartisanCompositeSelfTestThrows `
            -Action {
                Invoke-PartisanCompositeCertification `
                    -CandidateManifestPath $candidateFixture.ManifestPath `
                    -PackagedRunPath $runFixture.RunPath `
                    -ExternalEvidencePaths @(
                        $dirtyTeardownPath,
                        $secondPath,
                        $soakPath) `
                    -CertificationOutputPath (
                        Join-Path $tempRoot 'dirty-teardown-output.json')
            } `
            -Message 'dirty external teardown was accepted'
        $caseCount++

        $artifactText = [IO.File]::ReadAllText($runFixture.ArtifactPath)
        try {
            Write-PartisanCompositeUtf8File `
                -Path $runFixture.ArtifactPath `
                -Text ($artifactText + "tampered`n")
            Assert-PartisanCompositeSelfTestThrows `
                -Action {
                    Invoke-PartisanCompositeCertification `
                        -CandidateManifestPath $candidateFixture.ManifestPath `
                        -PackagedRunPath $runFixture.RunPath `
                        -CertificationOutputPath (
                            Join-Path $tempRoot 'tampered-artifact-output.json')
                } `
                -Message 'tampered raw Campaign Debug artifact was accepted'
        }
        finally {
            Write-PartisanCompositeUtf8File `
                -Path $runFixture.ArtifactPath `
                -Text $artifactText
        }
        $caseCount++

        Write-Output ('SELFTEST ' + ([pscustomobject][ordered]@{
            passed = $true
            cases = $caseCount
            scenarios = $script:ScenarioOrder.Count
            manualGaps = $script:ManualGapAssertionOrder.Count
        } | ConvertTo-Json -Compress))
    }
    finally {
        $resolvedTemp = [IO.Path]::GetFullPath($tempRoot).TrimEnd('\', '/')
        if ($resolvedTemp.StartsWith(
                $tempParent + [IO.Path]::DirectorySeparatorChar,
                [StringComparison]::OrdinalIgnoreCase) -and
            (Split-Path -Leaf $resolvedTemp).StartsWith(
                'partisan-composite-certification-selftest-',
                [StringComparison]::Ordinal) -and
            (Test-Path -LiteralPath $resolvedTemp -PathType Container)) {
            Remove-Item `
                -LiteralPath $resolvedTemp `
                -Recurse `
                -Force `
                -ErrorAction SilentlyContinue
        }
    }
}

if ($SelfTest) {
    Invoke-PartisanCompositeSelfTest
    return
}

$certification = Invoke-PartisanCompositeCertification `
    -CandidateManifestPath $CandidateManifest `
    -PackagedRunPath $CampaignDebugRun `
    -ExternalEvidencePaths $ExternalEvidence `
    -CertificationOutputPath $OutputPath
Write-Output ('CERTIFICATION ' + ([pscustomobject][ordered]@{
    certificationId = $certification.certificationId
    decision = $certification.overall.decision
    inProcessAccepted = $certification.overall.inProcessAccepted
    satisfiedScenarios = $certification.overall.satisfiedScenarioCount
    requiredScenarios = $certification.overall.requiredScenarioCount
    externallySatisfiedGaps =
        $certification.overall.externallySatisfiedGapCount
    requiredGaps = $certification.overall.requiredGapCount
} | ConvertTo-Json -Compress))
