[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$RunEnvelopePath,

    [string]$OutputPath = '',

    [switch]$SyntheticValidationOnly,

    [switch]$VerifyPublishedIndex,

    [switch]$LibraryOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:ContractId = 'partisan.gate1-runtime-retention.v1'
$script:IndexContractId = 'partisan.gate1-runtime-retention.index.v1'
$script:EvidenceKind = 'packaged-gate1-runtime-retention'
$script:WorldResource = 'Worlds/HST_Everon/HST_Everon.ent'
$script:MissionHeader = 'Missions/HST_Everon.conf'
$script:ProjectId = '698532771130111D'
$script:Stages = @(
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
$script:DiagnosticInputJournalCounts = @(0, 1, 2, 2, 2)
$script:OutputJournalCounts = @(1, 2, 2, 2, 2)
$script:HarnessToolPaths = [ordered]@{
    'gate1-runner' = 'tools/run-guarded-gate1-runtime-retention.ps1'
    'release-candidate-module' = 'tools/Partisan.ReleaseCandidate.psm1'
    'guarded-runtime-module' = 'tools/Partisan.GuardedRuntime.psm1'
    'ordinary-persistence-library' =
        'tools/run-ordinary-campaign-persistence-proof.ps1'
    'gate1-index-producer' = 'tools/New-PartisanGate1RuntimeRetentionIndex.ps1'
    'gate1-evidence-consumer' = 'tools/Partisan.Gate1EvidenceConsumer.psm1'
    'release-docs-consumer' = 'tools/update-release-docs.ps1'
}

function Get-Sha256Bytes {
    param([Parameter(Mandatory = $true)][byte[]]$Bytes)

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally { $algorithm.Dispose() }
}

function Get-Sha256Text {
    param([Parameter(Mandatory = $true)][AllowEmptyString()][string]$Text)

    return Get-Sha256Bytes `
        -Bytes ((New-Object Text.UTF8Encoding($false)).GetBytes($Text))
}

function Get-StrictTextArtifact {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $leaf = [IO.Path]::GetFullPath($Path)
    if (-not (Test-Path -LiteralPath $leaf -PathType Leaf)) {
        throw "$Label is missing."
    }
    $bytes = [IO.File]::ReadAllBytes($leaf)
    if ($bytes.Length -lt 1) { throw "$Label is empty." }
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
        Sha256 = Get-Sha256Bytes -Bytes $bytes
    }
}

function Get-FileSignature {
    param([Parameter(Mandatory = $true)][string]$Path)

    $leaf = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    if ($leaf.PSIsContainer) { throw 'A file signature target is a directory.' }
    return [pscustomobject][ordered]@{
        length = [long]$leaf.Length
        sha256 = (Get-FileHash -LiteralPath $leaf.FullName -Algorithm SHA256).Hash.
            ToLowerInvariant()
    }
}

function Test-FileSignatureExact {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual
    )

    return [long]$Expected.length -eq [long]$Actual.length -and
        [string]$Expected.sha256 -ceq [string]$Actual.sha256
}

function Assert-ExactProperties {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string[]]$Names,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($null -eq $Value) { throw "$Label is null." }
    $actual = @($Value.PSObject.Properties | ForEach-Object { $_.Name })
    if ($actual.Count -ne $Names.Count) {
        throw "$Label does not have its exact property count."
    }
    foreach ($name in $Names) {
        if ($actual -cnotcontains $name) {
            throw "$Label is missing property $name."
        }
    }
}

function Assert-JsonStringValue {
    param($Value, [Parameter(Mandatory = $true)][string]$Label)

    if (-not ($Value -is [string])) {
        throw "$Label is not an exact JSON string."
    }
}

function Assert-JsonStringProperties {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string[]]$Names,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($name in $Names) {
        Assert-JsonStringValue $Value.$name "$Label.$name"
    }
}

function Assert-JsonBooleanValue {
    param(
        $Value,
        [Parameter(Mandatory = $true)][string]$Label,
        [bool]$Expected
    )

    if (-not ($Value -is [bool]) -or [bool]$Value -ne $Expected) {
        throw "$Label is not the exact JSON boolean $Expected."
    }
}

function Assert-JsonIntegerValue {
    param($Value, [Parameter(Mandatory = $true)][string]$Label)

    if (-not ($Value -is [int]) -and -not ($Value -is [long])) {
        throw "$Label is not an exact integral JSON number."
    }
}

function Assert-JsonIntegerProperties {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string[]]$Names,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($name in $Names) {
        Assert-JsonIntegerValue $Value.$name "$Label.$name"
    }
}

function Assert-JsonStringArray {
    param(
        [AllowEmptyCollection()][object[]]$Values,
        [Parameter(Mandatory = $true)][string]$Label
    )

    for ($index = 0; $index -lt $Values.Count; $index++) {
        Assert-JsonStringValue $Values[$index] "$Label[$index]"
    }
}

function Assert-JsonSignatureTypes {
    param($Value, [Parameter(Mandatory = $true)][string]$Label)

    Assert-ExactProperties $Value @('length', 'sha256') $Label
    Assert-JsonIntegerValue $Value.length "$Label.length"
    Assert-JsonStringValue $Value.sha256 "$Label.sha256"
}

function Skip-StrictJsonWhitespace {
    param([string]$Text, $Index)
    while ($Index.Value -lt $Text.Length -and
        $Text[$Index.Value] -in @(' ', "`t", "`r", "`n")) {
        $Index.Value++
    }
}

function Read-StrictJsonString {
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

function Read-StrictJsonValue {
    param([string]$Text, $Index, [string]$Label)
    Skip-StrictJsonWhitespace $Text $Index
    if ($Index.Value -ge $Text.Length) {
        throw "$Label contains an incomplete JSON value."
    }
    $character = $Text[$Index.Value]
    if ($character -eq '{') {
        $Index.Value++
        $keys = New-Object Collections.Generic.HashSet[string](
            [StringComparer]::Ordinal)
        Skip-StrictJsonWhitespace $Text $Index
        if ($Index.Value -lt $Text.Length -and $Text[$Index.Value] -eq '}') {
            $Index.Value++
            return
        }
        while ($true) {
            Skip-StrictJsonWhitespace $Text $Index
            $key = Read-StrictJsonString $Text $Index $Label
            if (-not $keys.Add($key)) {
                throw "$Label duplicates JSON object property $key."
            }
            Skip-StrictJsonWhitespace $Text $Index
            if ($Index.Value -ge $Text.Length -or $Text[$Index.Value] -ne ':') {
                throw "$Label contains a JSON property without a colon."
            }
            $Index.Value++
            Read-StrictJsonValue $Text $Index $Label
            Skip-StrictJsonWhitespace $Text $Index
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
        Skip-StrictJsonWhitespace $Text $Index
        if ($Index.Value -lt $Text.Length -and $Text[$Index.Value] -eq ']') {
            $Index.Value++
            return
        }
        while ($true) {
            Read-StrictJsonValue $Text $Index $Label
            Skip-StrictJsonWhitespace $Text $Index
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
        $null = Read-StrictJsonString $Text $Index $Label
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

function Assert-NoDuplicateJsonObjectKeys {
    param([string]$Text, [string]$Label)
    $index = 0
    Read-StrictJsonValue $Text ([ref]$index) $Label
    Skip-StrictJsonWhitespace $Text ([ref]$index)
    if ($index -ne $Text.Length) {
        throw "$Label contains trailing JSON content."
    }
}

function Read-StrictJsonArtifact {
    param([string]$Path, [string]$Label)
    $artifact = Get-StrictTextArtifact -Path $Path -Label $Label
    Assert-NoDuplicateJsonObjectKeys -Text $artifact.Text -Label $Label
    try { $value = $artifact.Text | ConvertFrom-Json }
    catch { throw "$Label is not valid JSON." }
    return [pscustomobject][ordered]@{ Artifact = $artifact; Value = $value }
}

function Assert-PortableRelativePath {
    param([string]$Path, [string]$Label)
    if ([string]::IsNullOrWhiteSpace($Path) -or $Path.Contains('\') -or
        $Path.Contains(':') -or $Path.StartsWith('/') -or
        $Path -match '[\r\n\t]' -or $Path.Split('/') -contains '..' -or
        $Path.Split('/') -contains '.' -or $Path.EndsWith('/')) {
        throw "$Label is not a canonical portable relative path."
    }
}

function Resolve-PortableFile {
    param([string]$Root, [string]$Path, [string]$Label)
    Assert-PortableRelativePath $Path $Label
    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $candidate = [IO.Path]::GetFullPath((Join-Path $rootPath $Path))
    $prefix = $rootPath + [IO.Path]::DirectorySeparatorChar
    if (-not $candidate.StartsWith(
            $prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label escaped the retained run root."
    }
    return $candidate
}

function ConvertTo-PortableRelativePath {
    param([string]$Root, [string]$Path, [string]$Label)
    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $fullPath = [IO.Path]::GetFullPath($Path)
    $prefix = $rootPath + [IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith(
            $prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label escaped its retained root."
    }
    return $fullPath.Substring($prefix.Length).Replace('\', '/')
}

function Get-CanonicalRowsDigest {
    param([Parameter(Mandatory = $true)][AllowEmptyCollection()][object[]]$Rows)
    $lines = @($Rows | Sort-Object path | ForEach-Object {
        "{0}`t{1}`t{2}" -f $_.sha256, [long]$_.length, [string]$_.path
    })
    return Get-Sha256Text -Text (($lines -join "`n") + "`n")
}

function Get-ArgumentVectorDigest {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)
    return Get-Sha256Text -Text (
        (([string[]]$Arguments | ConvertTo-Json -Compress) + "`n"))
}

function Get-OptionPositions {
    param([string[]]$Arguments, [string]$Option)
    $rows = New-Object Collections.Generic.List[int]
    for ($index = 0; $index -lt $Arguments.Count; $index++) {
        if ([string]$Arguments[$index] -ceq $Option) { [void]$rows.Add($index) }
    }
    return [int[]]$rows.ToArray()
}

function Get-RequiredOptionValue {
    param([string[]]$Arguments, [string]$Option, [string]$Label)
    $positions = New-Object Collections.Generic.List[int]
    for ($index = 0; $index -lt $Arguments.Count; $index++) {
        if ([string]$Arguments[$index] -ieq $Option) {
            [void]$positions.Add($index)
        }
    }
    if ($positions.Count -ne 1 -or $positions[0] + 1 -ge $Arguments.Count -or
        [string]$Arguments[$positions[0]] -cne $Option -or
        [string]$Arguments[$positions[0] + 1] -like '-*') {
        throw "$Label does not contain one exact $Option value pair."
    }
    return [string]$Arguments[$positions[0] + 1]
}

function Assert-ExactLaunchOptionVector {
    param(
        [string]$Stage,
        [string]$Role,
        [string[]]$Arguments,
        [ValidateSet('diagnostic-lineage', 'standard-retention')]
        [string]$Phase
    )

    if ($Role -ceq 'client') {
        $expectedOptions = [string[]]@(
            '-gproj', '-addonsDir', '-addons', '-addonTempDir', '-client',
            '-profile', '-logsDir', '-logLevel', '-logTime', '-window',
            '-noFocus', '-forceUpdate', '-noSplash', '-noSound', '-noThrow',
            '-maxFPS', '-hstReleaseCandidateId', '-hstReleasePackageSha256',
            '-hstReleaseManifestSha256')
    }
    elseif ($Phase -ceq 'diagnostic-lineage') {
        $expected = New-Object Collections.Generic.List[string]
        foreach ($option in @(
                '-gproj', '-server', '-MissionHeader', '-addonsDir', '-addons',
                '-profile', '-logLevel', '-logTime', '-noThrow', '-maxFPS')) {
            [void]$expected.Add($option)
        }
        if ($Stage -cne 'profile_fallback_verify') {
            [void]$expected.Add('-loadSessionSave')
        }
        if ($Stage -ceq 'shutdown_checkpoint') {
            [void]$expected.Add('-autoshutdown')
        }
        foreach ($option in @(
                '-hstOrdinaryCampaignPersistenceProof',
                '-hstOrdinaryCampaignPersistenceStage',
                '-hstOrdinaryCampaignPersistenceRunId',
                '-hstOrdinaryCampaignPersistenceSessionNonce',
                '-hstOrdinaryCampaignPersistenceStageNonce',
                '-logsDir', '-addonTempDir', '-hstReleaseCandidateId',
                '-hstReleasePackageSha256', '-hstReleaseManifestSha256')) {
            [void]$expected.Add($option)
        }
        $expectedOptions = [string[]]$expected.ToArray()
    }
    else {
        $expected = New-Object Collections.Generic.List[string]
        foreach ($option in @(
                '-gproj', '-server', '-MissionHeader', '-addonsDir', '-addons',
                '-profile', '-logsDir', '-addonTempDir', '-logLevel', '-logTime',
                '-noThrow', '-maxFPS')) {
            [void]$expected.Add($option)
        }
        if ($Stage -cne 'profile_fallback_verify') {
            [void]$expected.Add('-loadSessionSave')
        }
        foreach ($option in @(
                '-hstReleaseCandidateId', '-hstReleasePackageSha256',
                '-hstReleaseManifestSha256')) {
            [void]$expected.Add($option)
        }
        $expectedOptions = [string[]]$expected.ToArray()
    }

    $bareOptions = @(
        '-client', '-window', '-noFocus', '-forceUpdate', '-noSplash',
        '-noSound', '-noThrow', '-autoshutdown')
    $cursor = 0
    foreach ($option in $expectedOptions) {
        if ($cursor -ge $Arguments.Count -or
            [string]$Arguments[$cursor] -cne $option) {
            throw "$Stage/$Role does not have the exact allowed option vector."
        }
        $cursor++
        $takesValue = $option -notin $bareOptions
        if ($option -ceq '-loadSessionSave' -and
            $Phase -ceq 'diagnostic-lineage' -and
            $Stage -ceq 'autosave_checkpoint') {
            $takesValue = $false
        }
        if ($takesValue) {
            if ($cursor -ge $Arguments.Count -or
                [string]$Arguments[$cursor] -like '-*') {
                throw "$Stage/$Role does not have the exact allowed option vector."
            }
            $cursor++
        }
    }
    if ($cursor -ne $Arguments.Count) {
        throw "$Stage/$Role does not have the exact allowed option vector."
    }
}

function Assert-EngineLaunchTopology {
    param(
        [string]$Stage,
        [string]$Role,
        [string[]]$Arguments,
        [string]$CandidateId,
        [string]$PackageSha256,
        [string]$ManifestSha256,
        [string]$AddonGuid,
        [string]$WorldResource,
        [string]$MissionHeader,
        [AllowEmptyString()][string]$RunId = '',
        [AllowEmptyString()][string]$SessionNonce = '',
        [AllowEmptyString()][string]$StageNonce = '',
        [ValidateSet('diagnostic-lineage', 'standard-retention')]
        [string]$Phase
    )
    foreach ($forbidden in @(
        '-keepSessionSave', '-config', '-backendLocalStorage',
        '-backendDisableStorage', '-noBackend', '-rpl-timeout-disable')) {
        if ($Arguments -icontains $forbidden) {
            throw "$Stage/$Role contains forbidden option $forbidden."
        }
    }
    if (@($Arguments | Where-Object {
            [string]$_ -ilike '-rpl-validation-*-disable'
        }).Count -ne 0) {
        throw "$Stage/$Role disables replication validation."
    }
    if ((Get-RequiredOptionValue $Arguments '-hstReleaseCandidateId' `
            "$Stage/$Role") -cne $CandidateId -or
        (Get-RequiredOptionValue $Arguments '-hstReleasePackageSha256' `
            "$Stage/$Role") -cne $PackageSha256 -or
        (Get-RequiredOptionValue $Arguments '-hstReleaseManifestSha256' `
            "$Stage/$Role") -cne $ManifestSha256 -or
        (Get-RequiredOptionValue $Arguments '-addons' `
            "$Stage/$Role") -cne $AddonGuid) {
        throw "$Stage/$Role is not bound to the retained candidate."
    }
    $null = Get-RequiredOptionValue $Arguments '-gproj' "$Stage/$Role"
    $null = Get-RequiredOptionValue $Arguments '-addonsDir' "$Stage/$Role"
    $null = Get-RequiredOptionValue $Arguments '-profile' "$Stage/$Role"
    $null = Get-RequiredOptionValue $Arguments '-logsDir' "$Stage/$Role"
    $null = Get-RequiredOptionValue $Arguments '-addonTempDir' "$Stage/$Role"

    if ($Phase -ceq 'standard-retention') {
        $authorityOptions = @($Arguments | Where-Object {
                [string]$_ -match '^-' -and
                ([string]$_ -imatch
                    '(?:autotest|selftest|test|proof|debug|diagnostic)' -or
                [string]$_ -imatch '^-scrDefine(?:$|=)')
            })
        if ($authorityOptions.Count -ne 0) {
            throw "$Stage/$Role standard context contains test or diagnostic authority."
        }
    }

    Assert-ExactLaunchOptionVector `
        -Stage $Stage `
        -Role $Role `
        -Arguments $Arguments `
        -Phase $Phase
    if ((Get-RequiredOptionValue $Arguments '-logLevel' "$Stage/$Role") -cne
            'normal' -or
        (Get-RequiredOptionValue $Arguments '-logTime' "$Stage/$Role") -cne
            'datetime' -or
        (Get-RequiredOptionValue $Arguments '-maxFPS' "$Stage/$Role") -cne
            '30') {
        throw "$Stage/$Role fixed runtime option values are not exact."
    }

    if ($Role -ceq 'server') {
        if ((Get-RequiredOptionValue $Arguments '-server' "$Stage/server") -cne
                $WorldResource -or
            (Get-RequiredOptionValue $Arguments '-MissionHeader' `
                "$Stage/server") -cne $MissionHeader) {
            throw "$Stage standard server world or mission header is not exact."
        }
        if ($Phase -ceq 'standard-retention') {
            foreach ($diagnosticFlag in @(
                    '-hstOrdinaryCampaignPersistenceProof',
                    '-hstOrdinaryCampaignPersistenceStage',
                    '-hstOrdinaryCampaignPersistenceRunId',
                    '-hstOrdinaryCampaignPersistenceSessionNonce',
                    '-hstOrdinaryCampaignPersistenceStageNonce')) {
                if ($Arguments -icontains $diagnosticFlag) {
                    throw "$Stage standard server contains diagnostic proof authority."
                }
            }
            if (@($Arguments | Where-Object {
                    [string]$_ -imatch '^-hst.*(?:proof|debug|autotest|selftest)'
                }).Count -ne 0) {
                throw "$Stage standard server contains an HST diagnostic option."
            }
            if ($Arguments -icontains '-autoshutdown') {
                throw "$Stage standard retention context contains mutation/end authority."
            }
            $standardLoad = @(Get-OptionPositions $Arguments '-loadSessionSave')
            if ($Stage -ceq 'profile_fallback_verify') {
                if ($standardLoad.Count -ne 0) {
                    throw 'Standard fallback retention has native load authority.'
                }
            }
            elseif ($standardLoad.Count -ne 1 -or
                $standardLoad[0] + 1 -ge $Arguments.Count -or
                [string]$Arguments[$standardLoad[0] + 1] -notmatch
                    '^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$') {
                throw "$Stage standard retention does not load one supplied UUID."
            }
            return
        }
        if ($Arguments -cnotcontains '-hstOrdinaryCampaignPersistenceProof') {
            throw "$Stage diagnostic lineage server lacks proof authority."
        }
        if ([string]::IsNullOrWhiteSpace($RunId) -or
            [string]::IsNullOrWhiteSpace($SessionNonce) -or
            [string]::IsNullOrWhiteSpace($StageNonce) -or
            (Get-RequiredOptionValue $Arguments `
                '-hstOrdinaryCampaignPersistenceProof' "$Stage/server") -cne
                'true' -or
            (Get-RequiredOptionValue $Arguments `
                '-hstOrdinaryCampaignPersistenceStage' "$Stage/server") -cne
                $Stage -or
            (Get-RequiredOptionValue $Arguments `
                '-hstOrdinaryCampaignPersistenceRunId' "$Stage/server") -cne
                $RunId -or
            (Get-RequiredOptionValue $Arguments `
                '-hstOrdinaryCampaignPersistenceSessionNonce' `
                "$Stage/server") -cne $SessionNonce -or
            (Get-RequiredOptionValue $Arguments `
                '-hstOrdinaryCampaignPersistenceStageNonce' `
                "$Stage/server") -cne $StageNonce) {
            throw "$Stage diagnostic authority values are not contract-bound."
        }
        $load = @(Get-OptionPositions $Arguments '-loadSessionSave')
        if ($Stage -ceq 'autosave_checkpoint') {
            if ($load.Count -ne 1 -or $load[0] + 1 -ge $Arguments.Count -or
                [string]$Arguments[$load[0] + 1] -notlike '-*') {
                throw 'Autosave does not use one bare native-session bootstrap flag.'
            }
        }
        elseif ($Stage -in @(
                'manual_checkpoint', 'shutdown_checkpoint',
                'native_shutdown_verify')) {
            if ($load.Count -ne 1 -or $load[0] + 1 -ge $Arguments.Count -or
                [string]$Arguments[$load[0] + 1] -notmatch
                    '^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$') {
                throw "$Stage does not load one exact native save UUID."
            }
        }
        elseif ($load.Count -ne 0) {
            throw 'Fallback verification has native load authority.'
        }
        $shutdownCount = @(Get-OptionPositions $Arguments '-autoshutdown').Count
        if (($Stage -ceq 'shutdown_checkpoint' -and $shutdownCount -ne 1) -or
            ($Stage -cne 'shutdown_checkpoint' -and $shutdownCount -ne 0)) {
            throw "$Stage has incorrect controlled-shutdown authority."
        }
    }
    else {
        $clientPositions = @(Get-OptionPositions $Arguments '-client')
        if ($Stage -cne 'shutdown_checkpoint' -or
            $clientPositions.Count -ne 1 -or
            $Arguments -icontains '-server' -or
            $Arguments -icontains '-MissionHeader' -or
            $Arguments -icontains '-loadSessionSave' -or
            $Arguments -icontains '-autoshutdown') {
            throw 'The one loopback client does not have its exact shutdown topology.'
        }
        if ($Phase -ceq 'standard-retention' -and
            $Arguments -icontains '-hstOrdinaryCampaignPersistenceProof') {
            throw 'The standard retention client contains diagnostic proof authority.'
        }
        if ($Phase -ceq 'standard-retention' -and
            @($Arguments | Where-Object {
                [string]$_ -imatch '^-hst.*(?:proof|debug|autotest|selftest)'
            }).Count -ne 0) {
            throw 'The standard retention client contains an HST diagnostic option.'
        }
    }
}

function Assert-CandidateManifestBinding {
    param(
        $Candidate,
        $Manifest,
        $Ready,
        [string]$ManifestSha256
    )

    Assert-JsonIntegerValue $Manifest.manifestSchemaVersion `
        'Copied candidate manifest.manifestSchemaVersion'
    Assert-JsonIntegerValue $Ready.schemaVersion `
        'Copied candidate ready marker.schemaVersion'
    Assert-JsonStringProperties $Candidate @(
        'candidateId', 'candidateVersion', 'gitHead', 'embeddedBuildSha',
        'embeddedBuildUtc', 'embeddedBuildLabel', 'addonId', 'addonGuid',
        'packageHashAlgorithm', 'packageSha256', 'manifestSha256') `
        'Gate 1 candidate'
    Assert-JsonIntegerProperties $Candidate @(
        'campaignSchema', 'runtimeSettingsSchema') 'Gate 1 candidate'
    Assert-JsonStringProperties $Manifest.candidate @('id', 'version', 'state') `
        'Copied candidate manifest candidate'
    Assert-JsonStringProperties $Manifest.source @('gitHead') `
        'Copied candidate manifest source'
    Assert-JsonStringProperties $Manifest.source.embeddedImplementation @(
        'sha', 'utc', 'label') 'Copied candidate embedded implementation'
    Assert-JsonIntegerProperties $Manifest.source @(
        'campaignSchema', 'runtimeSettingsSchema') `
        'Copied candidate manifest source'
    Assert-JsonStringProperties $Manifest.addon @('id', 'guid') `
        'Copied candidate manifest addon'
    Assert-JsonStringProperties $Manifest.workbench @('crc') `
        'Copied candidate manifest workbench'
    Assert-JsonStringProperties $Manifest.package @('hashAlgorithm', 'sha256') `
        'Copied candidate manifest package'
    Assert-JsonStringProperties $Ready @(
        'candidateId', 'gitHead', 'packageSha256', 'manifestSha256') `
        'Copied candidate ready marker'

    if ([int]$Manifest.manifestSchemaVersion -ne 1 -or
        [int]$Ready.schemaVersion -ne 1 -or
        [string]$Manifest.candidate.id -cne [string]$Candidate.candidateId -or
        [string]$Manifest.candidate.version -cne
            [string]$Candidate.candidateVersion -or
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
        [string]$Ready.candidateId -cne [string]$Candidate.candidateId -or
        [string]$Ready.gitHead -cne [string]$Candidate.gitHead -or
        [string]$Ready.packageSha256 -cne [string]$Candidate.packageSha256 -or
        [string]$Ready.manifestSha256 -cne $ManifestSha256) {
        throw 'Copied candidate manifest or ready identity is not cross-bound.'
    }

    $identityBindings = @(
        [pscustomobject]@{
            Label = 'standard server'
            Manifest = $Manifest.toolchain.server
            Candidate = $Candidate.executables.server
        },
        [pscustomobject]@{
            Label = 'standard client'
            Manifest = $Manifest.toolchain.client
            Candidate = $Candidate.executables.client
        },
        [pscustomobject]@{
            Label = 'diagnostic server'
            Manifest = $Manifest.toolchain.serverDiagnostic
            Candidate = $Candidate.executables.serverDiagnostic
        },
        [pscustomobject]@{
            Label = 'diagnostic client'
            Manifest = $Manifest.toolchain.clientDiagnostic
            Candidate = $Candidate.executables.clientDiagnostic
        })
    foreach ($binding in $identityBindings) {
        Assert-JsonStringProperties $binding.Manifest @(
            'fileName', 'fileVersion', 'productVersion', 'sha256') `
            "Copied candidate $($binding.Label) manifest identity"
        Assert-JsonIntegerValue $binding.Manifest.length `
            "Copied candidate $($binding.Label) manifest identity.length"
        Assert-JsonStringProperties $binding.Candidate @(
            'fileName', 'fileVersion', 'productVersion', 'sha256') `
            "Copied candidate $($binding.Label) retained identity"
        Assert-JsonIntegerValue $binding.Candidate.length `
            "Copied candidate $($binding.Label) retained identity.length"
        foreach ($name in @(
                'fileName', 'fileVersion', 'productVersion', 'length', 'sha256')) {
            if ([string]$binding.Manifest.$name -cne
                [string]$binding.Candidate.$name) {
                throw "Copied candidate $($binding.Label) identity is not exact."
            }
        }
    }
}

function Assert-LaunchContractRunBinding {
    param($Contract, $Run)

    Assert-ExactProperties $Contract.buildIdentity @(
        'BuildSha', 'BuildUtc', 'BuildLabel', 'CampaignSchemaVersion',
        'SettingsSchemaVersion') 'Launch contract build identity'
    Assert-JsonStringProperties $Contract.buildIdentity @(
        'BuildSha', 'BuildUtc', 'BuildLabel') 'Launch contract build identity'
    Assert-JsonIntegerProperties $Contract.buildIdentity @(
        'CampaignSchemaVersion', 'SettingsSchemaVersion') `
        'Launch contract build identity'
    if ([string]$Contract.buildIdentity.BuildSha -cne
            [string]$Run.candidate.embeddedBuildSha -or
        [string]$Contract.buildIdentity.BuildUtc -cne
            [string]$Run.candidate.embeddedBuildUtc -or
        [string]$Contract.buildIdentity.BuildLabel -cne
            [string]$Run.candidate.embeddedBuildLabel -or
        [int]$Contract.buildIdentity.CampaignSchemaVersion -ne
            [int]$Run.candidate.campaignSchema -or
        [int]$Contract.buildIdentity.SettingsSchemaVersion -ne
            [int]$Run.candidate.runtimeSettingsSchema -or
        [string]$Contract.worldResource -cne
            [string]$Run.scenario.worldResource -or
        [string]$Contract.missionHeader -cne
            [string]$Run.scenario.missionHeader -or
        [string]$Contract.projectId -cne [string]$Run.candidate.addonGuid -or
        @($Contract.stages).Count -ne @($Run.scenario.stages).Count) {
        throw 'The launch contract is not cross-bound to candidate and scenario.'
    }
    for ($index = 0; $index -lt @($Contract.stages).Count; $index++) {
        $row = $Contract.stages[$index]
        Assert-ExactProperties $row @(
            'ordinal', 'stage', 'stageNonce', 'loadSavePointId',
            'latestSavePointId') 'Launch contract stage row'
        Assert-JsonIntegerValue $row.ordinal 'Launch contract stage row.ordinal'
        Assert-JsonStringProperties $row @(
            'stage', 'stageNonce', 'loadSavePointId', 'latestSavePointId') `
            'Launch contract stage row'
        if ([int]$row.ordinal -ne $index -or
            [string]$row.stage -cne [string]$Run.scenario.stages[$index] -or
            [string]$row.stageNonce -cnotmatch '^[0-9a-f]{32}$') {
            throw 'The launch contract stage order or identity is invalid.'
        }
    }
}

function Assert-LaunchContractPersistenceBinding {
    param($Contract, [object[]]$Persistence)

    $stages = @($Contract.stages)
    if ($stages.Count -ne $Persistence.Count) {
        throw 'The launch contract and persistence stage counts differ.'
    }
    for ($index = 0; $index -lt $stages.Count; $index++) {
        foreach ($name in @(
                'ordinal', 'stage', 'stageNonce', 'loadSavePointId',
                'latestSavePointId')) {
            if ([string]$stages[$index].$name -cne
                [string]$Persistence[$index].$name) {
                throw 'The launch contract stage is not persistence-bound.'
            }
        }
    }
}

function Assert-HarnessToolRoleSet {
    param(
        [AllowEmptyCollection()][object[]]$Tools,
        [bool]$RequireComplete
    )

    $expectedToolPaths = New-Object `
        'Collections.Generic.Dictionary[string,string]' `
        ([StringComparer]::Ordinal)
    foreach ($expectedRole in $script:HarnessToolPaths.Keys) {
        $expectedToolPaths.Add(
            [string]$expectedRole,
            [string]$script:HarnessToolPaths[$expectedRole])
    }
    $seen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($tool in $Tools) {
        Assert-ExactProperties $tool @('role', 'path', 'length', 'sha256') `
            'Gate 1 harness tool row'
        Assert-JsonStringProperties $tool @('role', 'path', 'sha256') `
            'Gate 1 harness tool row'
        Assert-JsonIntegerValue $tool.length 'Gate 1 harness tool row.length'
        $role = [string]$tool.role
        Assert-PortableRelativePath ([string]$tool.path) 'Harness tool path'
        if (-not $expectedToolPaths.ContainsKey($role) -or
            -not $seen.Add($role) -or
            [string]$tool.path -cne [string]$expectedToolPaths[$role] -or
            [long]$tool.length -lt 1 -or
            [string]$tool.sha256 -cnotmatch '^[0-9a-f]{64}$') {
            throw 'Gate 1 harness tool roles and paths are not exact and unique.'
        }
    }
    if ($RequireComplete) {
        if ($seen.Count -ne $script:HarnessToolPaths.Count) {
            throw 'The production Gate 1 harness tool set is incomplete.'
        }
        foreach ($expectedRole in $script:HarnessToolPaths.Keys) {
            if (-not $seen.Contains([string]$expectedRole)) {
                throw 'The production Gate 1 harness tool set is incomplete.'
            }
        }
    }
}

function Get-GitBlobSignature {
    param(
        [string]$RepositoryRoot,
        [string]$Commit,
        [string]$Path
    )

    Assert-PortableRelativePath $Path 'Git harness blob path'
    $blobOutput = @(& git -C $RepositoryRoot rev-parse `
        ($Commit + ':' + $Path) 2>$null)
    if ($LASTEXITCODE -ne 0) { throw 'A harness Git blob is missing.' }
    $blobId = ($blobOutput -join '').Trim()
    if ($blobId -cnotmatch '^[0-9a-f]{40,64}$') {
        throw 'A harness Git blob identity is invalid.'
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
            sha256 = Get-Sha256Bytes $bytes
        }
    }
    finally {
        $memory.Dispose()
        $process.Dispose()
    }
}

function Assert-HarnessGitBinding {
    param(
        [string]$RepositoryRoot,
        [string]$CandidateHead,
        [string]$HarnessHead,
        [object[]]$Tools,
        [switch]$AllowUnrelatedDirty
    )

    foreach ($head in @($CandidateHead, $HarnessHead)) {
        if ($head -cnotmatch '^[0-9a-f]{40}$') {
            throw 'A harness Git commit identity is invalid.'
        }
        & git -C $RepositoryRoot cat-file -e ($head + '^{commit}') 2>$null
        if ($LASTEXITCODE -ne 0) { throw 'A harness Git commit is unknown.' }
    }
    & git -C $RepositoryRoot merge-base --is-ancestor `
        $CandidateHead $HarnessHead 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw 'The harness commit is not a verified candidate descendant.'
    }
    $currentHead = (@(& git -C $RepositoryRoot rev-parse HEAD 2>$null) -join '').Trim()
    if ($LASTEXITCODE -ne 0 -or $currentHead -cnotmatch '^[0-9a-f]{40}$') {
        throw 'The current checkout commit identity is invalid.'
    }
    & git -C $RepositoryRoot merge-base --is-ancestor `
        $HarnessHead $currentHead 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw 'The current checkout is not a verified harness descendant.'
    }
    if (-not $AllowUnrelatedDirty) {
        if ($currentHead -cne $HarnessHead) {
            throw 'The current checkout does not match the retained harness commit.'
        }
        $dirty = @(& git -C $RepositoryRoot status --porcelain --untracked-files=all)
        if ($LASTEXITCODE -ne 0 -or $dirty.Count -ne 0) {
            throw 'The retained harness checkout is not clean.'
        }
    }
    foreach ($tool in $Tools) {
        $worktreePath = Resolve-PortableFile $RepositoryRoot ([string]$tool.path) `
            'Harness worktree tool path'
        if (-not (Test-Path -LiteralPath $worktreePath -PathType Leaf)) {
            throw 'A retained harness worktree tool is missing.'
        }
        $worktree = Get-FileSignature $worktreePath
        $blob = Get-GitBlobSignature $RepositoryRoot $HarnessHead `
            ([string]$tool.path)
        if (-not (Test-FileSignatureExact $tool $worktree) -or
            -not (Test-FileSignatureExact $tool $blob)) {
            throw 'A retained harness tool differs from its worktree or Git blob.'
        }
    }
}

function Assert-ContextReceiptSidecarBinding {
    param(
        [string]$RunRoot,
        [string]$Stage,
        $Context,
        $Tested
    )

    foreach ($binding in @(
            [pscustomobject]@{
                Label = 'receipt'
                ContextPath = $Context.receiptPath
                ContextSignature = $Context.receiptSignature
                TestedPath = $Tested.ReceiptPath
                TestedSignature = $Tested.ReceiptSignature
                RequireNonempty = $true
            },
            [pscustomobject]@{
                Label = 'journal'
                ContextPath = $Context.journalPath
                ContextSignature = $Context.journalSignature
                TestedPath = $Tested.JournalPath
                TestedSignature = $Tested.JournalSignature
                RequireNonempty = $true
            },
            [pscustomobject]@{
                Label = 'completion'
                ContextPath = $Context.completionPath
                ContextSignature = $Context.completionSignature
                TestedPath = $Tested.CompletionAttestationPath
                TestedSignature = $Tested.CompletionAttestationSignature
                RequireNonempty = $true
            })) {
        Assert-JsonStringValue $binding.ContextPath `
            "$Stage retained $($binding.Label) path"
        Assert-JsonSignatureTypes $binding.ContextSignature `
            "$Stage retained $($binding.Label) signature"
        $portable = ConvertTo-PortableRelativePath `
            -Root $RunRoot `
            -Path ([string]$binding.TestedPath) `
            -Label "$Stage validated $($binding.Label) path"
        if ([string]$binding.ContextPath -cne $portable -or
            -not (Test-FileSignatureExact `
                $binding.ContextSignature $binding.TestedSignature) -or
            ($binding.RequireNonempty -and
                [long]$binding.TestedSignature.length -lt 1)) {
            throw "$Stage retained receipt sidecars are not exactly cross-bound."
        }
    }
}

function Assert-GuardedReceiptScalarTypes {
    param($Receipt, [Parameter(Mandatory = $true)][string]$Label)

    Assert-JsonIntegerValue $Receipt.version "$Label.version"
    Assert-JsonBooleanValue $Receipt.guardRemovalAuthorized `
        "$Label.guardRemovalAuthorized" $true
    Assert-JsonStringProperties $Receipt @(
        'magic', 'nonce', 'guardBindingSha256', 'guardDirectory',
        'journalPath', 'completionAttestationPath', 'completionTokenSha256',
        'candidateBindingSha256', 'guardInventorySha256',
        'boundaryBaselineSha256', 'executableBindingsSha256',
        'processLedgerSha256', 'portSemantics', 'candidateConsumptionBridge',
        'completionSemantics', 'recordedUtc') $Label
    Assert-JsonSignatureTypes $Receipt.journalSignature `
        "$Label.journalSignature"
    Assert-JsonSignatureTypes $Receipt.completionAttestationSignature `
        "$Label.completionAttestationSignature"
    Assert-JsonStringArray ([object[]]@($Receipt.teardownStopOrder)) `
        "$Label.teardownStopOrder"
}

function Assert-GuardedLaunchScalarTypes {
    param($Launch, [Parameter(Mandatory = $true)][string]$Label,
        [bool]$ExpectedEngine)

    Assert-JsonStringProperties $Launch @('role', 'path', 'workingDirectory') `
        $Label
    Assert-JsonIntegerProperties $Launch.identity @(
        'ProcessId', 'ParentProcessId') "$Label.identity"
    Assert-JsonStringProperties $Launch.identity @(
        'StartUtc', 'ExecutablePath', 'CommandLine') "$Label.identity"
    Assert-JsonStringArray ([object[]]@($Launch.identity.Arguments)) `
        "$Label.identity.Arguments"
    Assert-JsonBooleanValue $Launch.isEngine "$Label.isEngine" $ExpectedEngine
    Assert-JsonStringArray ([object[]]@($Launch.arguments)) "$Label.arguments"
    Assert-JsonStringProperties $Launch.provenance @(
        'fileName', 'fileVersion', 'productVersion', 'sha256') `
        "$Label.provenance"
    Assert-JsonIntegerValue $Launch.provenance.length `
        "$Label.provenance.length"
    Assert-JsonStringProperties $Launch.candidateConsumptionEvidence @(
        'mode', 'candidateBindingSha256') "$Label.candidateConsumptionEvidence"
    foreach ($pair in @(
            $Launch.candidateConsumptionEvidence.exactArgumentPairs)) {
        Assert-JsonStringProperties $pair @('flag', 'value') `
            "$Label exact argument pair"
        Assert-JsonIntegerValue $pair.index "$Label exact argument pair.index"
    }
}

function Assert-SnapshotManifest {
    param(
        [string]$RunRoot,
        [string]$ManifestPath,
        [string]$Stage,
        [string]$Direction,
        [string]$ExpectedDigest,
        [ValidateSet(0, 1, 2)][int]$ExpectedJournalCount
    )
    $path = Resolve-PortableFile $RunRoot $ManifestPath 'Snapshot manifest path'
    $parsed = Read-StrictJsonArtifact $path 'Save snapshot manifest'
    $value = $parsed.Value
    Assert-ExactProperties $value @(
        'schemaVersion', 'contractId', 'stage', 'direction', 'files',
        'aggregateSha256') 'Save snapshot manifest'
    Assert-JsonIntegerValue $value.schemaVersion 'Save snapshot manifest.schemaVersion'
    Assert-JsonStringProperties $value @(
        'contractId', 'stage', 'direction', 'aggregateSha256') `
        'Save snapshot manifest'
    if ([int]$value.schemaVersion -ne 1 -or
        [string]$value.contractId -cne $script:ContractId -or
        [string]$value.stage -cne $Stage -or
        [string]$value.direction -cne $Direction -or
        [string]$value.aggregateSha256 -cne $ExpectedDigest) {
        throw "$Stage/$Direction snapshot identity is invalid."
    }
    $snapshotRoot = Split-Path -Parent $path
    $seen = New-Object Collections.Generic.HashSet[string]([StringComparer]::Ordinal)
    $rows = @($value.files)
    foreach ($row in $rows) {
        Assert-ExactProperties $row @('kind', 'path', 'length', 'sha256') `
            'Save snapshot file row'
        Assert-JsonStringProperties $row @('kind', 'path', 'sha256') `
            'Save snapshot file row'
        Assert-JsonIntegerValue $row.length 'Save snapshot file row.length'
        if ([string]$row.kind -notin @('native', 'journal') -or
            [long]$row.length -lt 0 -or
            ([string]$row.kind -ceq 'journal' -and
                [long]$row.length -lt 1) -or
            [string]$row.sha256 -cnotmatch '^[0-9a-f]{64}$' -or
            -not $seen.Add([string]$row.path)) {
            throw "$Stage/$Direction contains an invalid snapshot row."
        }
        if (([string]$row.kind -ceq 'native' -and
                -not ([string]$row.path).StartsWith(
                    'files/native/.save/game/',
                    [StringComparison]::Ordinal)) -or
            ([string]$row.kind -ceq 'journal' -and
                [string]$row.path -notin @(
                    'files/journal/profile/Partisan/HST_CampaignSaveData.json',
                    'files/journal/profile/Partisan/HST_CampaignSaveData.recovery.json'))) {
            throw "$Stage/$Direction snapshot kind and path namespace differ."
        }
        $file = Resolve-PortableFile $snapshotRoot ([string]$row.path) `
            'Snapshot file path'
        if (-not (Test-Path -LiteralPath $file -PathType Leaf) -or
            -not (Test-FileSignatureExact $row (Get-FileSignature $file))) {
            throw "$Stage/$Direction snapshot bytes differ from its manifest."
        }
    }
    if ((Get-CanonicalRowsDigest $rows) -cne [string]$value.aggregateSha256) {
        throw "$Stage/$Direction snapshot aggregate digest is invalid."
    }
    $journalPaths = @($rows | Where-Object {
            [string]$_.kind -ceq 'journal'
        } | ForEach-Object { [string]$_.path } | Sort-Object)
    $expectedJournalPaths = @(if ($ExpectedJournalCount -eq 0) {
        [string[]]@()
    }
    elseif ($ExpectedJournalCount -eq 1) {
        [string[]]@(
            'files/journal/profile/Partisan/HST_CampaignSaveData.json')
    }
    else {
        [string[]]@(
            'files/journal/profile/Partisan/HST_CampaignSaveData.json',
            'files/journal/profile/Partisan/HST_CampaignSaveData.recovery.json')
    })
    $journalSetMatches = $expectedJournalPaths.Count -eq $journalPaths.Count
    if ($journalSetMatches) {
        for ($index = 0; $index -lt $expectedJournalPaths.Count; $index++) {
            if ([string]$expectedJournalPaths[$index] -cne
                [string]$journalPaths[$index]) {
                $journalSetMatches = $false
                break
            }
        }
    }
    if (-not $journalSetMatches) {
        throw "$Stage/$Direction snapshot journal set is not exact."
    }
    $actual = @(Get-ChildItem -LiteralPath (Join-Path $snapshotRoot 'files') `
        -Recurse -File -Force -ErrorAction SilentlyContinue | ForEach-Object {
            ConvertTo-PortableRelativePath $snapshotRoot $_.FullName `
                'Snapshot census file'
        } | Sort-Object)
    $expected = @($rows | ForEach-Object { [string]$_.path } | Sort-Object)
    if (@(Compare-Object $expected $actual -CaseSensitive).Count -ne 0) {
        throw "$Stage/$Direction snapshot has unindexed or missing files."
    }
    Add-Member -InputObject $value -NotePropertyName '__snapshotRoot' `
        -NotePropertyValue $snapshotRoot -Force
    return $value
}

function Get-SnapshotKindRows {
    param($Snapshot, [string]$Kind)
    return @($Snapshot.files | Where-Object { [string]$_.kind -ceq $Kind } |
        Sort-Object path)
}

function Get-NativeSnapshotMetadata {
    param($Snapshot, [string]$Label)
    $rows = New-Object Collections.Generic.List[object]
    foreach ($row in @(Get-SnapshotKindRows $Snapshot 'native' | Where-Object {
            ([string]$_.path).EndsWith(
                '/meta-info.json', [StringComparison]::Ordinal)
        })) {
        $path = Resolve-PortableFile ([string]$Snapshot.__snapshotRoot) `
            ([string]$row.path) "$Label native metadata path"
        $meta = (Read-StrictJsonArtifact $path "$Label native metadata").Value
        foreach ($name in @(
                'm_Id', 'm_eType', 'm_sSavePointDisplayName',
                'm_rMissionResource')) {
            if ($meta.PSObject.Properties.Name -cnotcontains $name) {
                throw "$Label native metadata is missing $name."
            }
        }
        Assert-JsonStringProperties $meta @(
            'm_Id', 'm_sSavePointDisplayName', 'm_rMissionResource') `
            "$Label native metadata"
        Assert-JsonIntegerValue $meta.m_eType "$Label native metadata.m_eType"
        if ([string]$meta.m_Id -cnotmatch
            '^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$' -or
            [string]$meta.m_rMissionResource -cne '6985327711302100') {
            throw "$Label native save metadata identity is invalid."
        }
        [void]$rows.Add([pscustomobject][ordered]@{
            id = [string]$meta.m_Id
            type = [int]$meta.m_eType
            name = [string]$meta.m_sSavePointDisplayName
            mission = [string]$meta.m_rMissionResource
            metaPath = [string]$row.path
        })
    }
    return [object[]]$rows.ToArray()
}

function Assert-NativeSnapshotSaveSet {
    param(
        $Snapshot,
        [string]$Label,
        [AllowEmptyCollection()][object[]]$Expected
    )

    $actual = @(Get-NativeSnapshotMetadata $Snapshot $Label)
    if ($actual.Count -ne $Expected.Count) {
        throw "$Label does not contain the exact native save metadata set."
    }
    $seen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $nativeRows = @(Get-SnapshotKindRows $Snapshot 'native')
    foreach ($expectedRow in $Expected) {
        $matches = @($actual | Where-Object {
                [string]$_.id -ceq [string]$expectedRow.id
            })
        if ($matches.Count -ne 1 -or
            -not $seen.Add([string]$expectedRow.id)) {
            throw "$Label native save IDs are not exact and unique."
        }
        $match = $matches[0]
        if ([int]$match.type -ne [int]$expectedRow.type -or
            [string]$match.name -cne [string]$expectedRow.name -or
            [string]$match.mission -cne '6985327711302100') {
            throw "$Label native save metadata values are not exact."
        }
        $suffix = '/meta-info.json'
        $metaPath = [string]$match.metaPath
        $saveRoot = $metaPath.Substring(0, $metaPath.Length - $suffix.Length)
        $worldPrefix = $saveRoot + '/WorldState/'
        $worldRows = @($nativeRows | Where-Object {
                ([string]$_.path).StartsWith(
                    $worldPrefix, [StringComparison]::Ordinal) -and
                [long]$_.length -gt 0
            })
        if ($worldRows.Count -lt 1) {
            throw "$Label native save metadata has no retained WorldState bytes."
        }
    }
}

function Test-RowSetExact {
    param(
        [AllowEmptyCollection()][object[]]$Expected,
        [AllowEmptyCollection()][object[]]$Actual)
    if ($Expected.Count -ne $Actual.Count) { return $false }
    for ($index = 0; $index -lt $Expected.Count; $index++) {
        foreach ($name in @('kind', 'path', 'length', 'sha256')) {
            if ([string]$Expected[$index].$name -cne
                [string]$Actual[$index].$name) { return $false }
        }
    }
    return $true
}

function Get-ExpectedGateFileProjection {
    param([string]$Path)

    Assert-PortableRelativePath $Path 'Gate 1 census path'
    $runRoles = [ordered]@{
        'run.owner.json' = 'run_owner'
        'identity/candidate.json' = 'candidate_manifest'
        'identity/candidate.ready.json' = 'candidate_ready'
        'configuration/HST_Settings.json' = 'runtime_settings'
        'configuration/launch-contract.json' = 'launch_contract'
    }
    if ($runRoles.Contains($Path)) {
        return [pscustomobject][ordered]@{
            role = [string]$runRoles[$Path]
            stage = 'run'
        }
    }
    if ($Path -cmatch '^identity/package/Partisan/[^/]+$') {
        return [pscustomobject][ordered]@{
            role = 'candidate_package'
            stage = 'run'
        }
    }
    if ($Path -cmatch ('^raw/(diagnostic-)?guarded-runtime/' +
            '\.PartisanGuardedRuntime_[0-9a-f]{32}' +
            '\.(receipt|journal|completion)\.json$')) {
        $diagnostic = -not [string]::IsNullOrEmpty([string]$Matches[1])
        $kind = [string]$Matches[2]
        return [pscustomobject][ordered]@{
            role = if ($diagnostic) {
                'diagnostic_guarded_runtime_' + $kind
            }
            else { 'guarded_runtime_' + $kind }
            stage = 'run'
        }
    }

    for ($index = 0; $index -lt $script:Stages.Count; $index++) {
        $stage = $script:Stages[$index]
        $directory = $script:StageDirectories[$index]
        foreach ($phase in @('diagnostic', 'standard')) {
            $prefix = if ($phase -ceq 'diagnostic') {
                'raw/stages/' + $directory + '/'
            }
            else { 'raw/standard-runtime/' + $directory + '/' }
            if (-not $Path.StartsWith($prefix, [StringComparison]::Ordinal)) {
                continue
            }
            $suffix = $Path.Substring($prefix.Length)
            $role = ''
            if ($suffix -cmatch '^save-(input|output)/snapshot\.json$') {
                $role = 'save_snapshot_manifest'
            }
            elseif ($suffix -cmatch
                '^save-(input|output)/files/native/\.save/game/.+$') {
                $role = 'native_save_' + [string]$Matches[1]
            }
            elseif ($suffix -cmatch
                ('^save-(input|output)/files/journal/profile/Partisan/' +
                    'HST_CampaignSaveData(?:\.recovery)?\.json$')) {
                $role = 'profile_journal_' + [string]$Matches[1]
            }
            elseif ($phase -ceq 'diagnostic' -and
                $suffix -in @(
                    'owner.json', 'guard-input.json', 'result.json',
                    'carrier.json', 'mixed-native-ready.json',
                    'end-bridge.json')) {
                $role = ([ordered]@{
                    'owner.json' = 'ordinary_owner'
                    'guard-input.json' = 'ordinary_guard'
                    'result.json' = 'ordinary_result'
                    'carrier.json' = 'ordinary_carrier'
                    'mixed-native-ready.json' = 'ordinary_mixed_native_ready'
                    'end-bridge.json' = 'ordinary_end_bridge'
                })[$suffix]
                if ($suffix -in @('mixed-native-ready.json', 'end-bridge.json') -and
                    $stage -cne 'shutdown_checkpoint') {
                    throw 'A shutdown-only retained artifact is in the wrong stage.'
                }
            }
            elseif ($phase -ceq 'diagnostic' -and
                $suffix -cmatch
                    '^diagnostic-(server|client)-logs/(console|script|error|crash)\.log$') {
                if ([string]$Matches[1] -ceq 'client' -and
                    $stage -cne 'shutdown_checkpoint') {
                    throw 'A diagnostic client log is in the wrong stage.'
                }
                $role = 'diagnostic_' + [string]$Matches[1] + '_' +
                    [string]$Matches[2] + '_log'
            }
            elseif ($phase -ceq 'standard' -and
                $suffix -cmatch
                    '^(server|client)-logs/(console|script|error|crash)\.log$') {
                if ([string]$Matches[1] -ceq 'client' -and
                    $stage -cne 'shutdown_checkpoint') {
                    throw 'A standard client log is in the wrong stage.'
                }
                $role = [string]$Matches[1] + '_' +
                    [string]$Matches[2] + '_log'
            }
            if ([string]::IsNullOrWhiteSpace($role)) {
                throw 'A retained Gate 1 path has no exact role projection.'
            }
            return [pscustomobject][ordered]@{ role = $role; stage = $stage }
        }
    }
    throw 'A retained Gate 1 path has no exact role projection.'
}

function Write-JsonAtomicCreateOnly {
    param([string]$Path, $Value)
    $json = ($Value | ConvertTo-Json -Depth 64) + "`n"
    $bytes = (New-Object Text.UTF8Encoding($false)).GetBytes($json)
    if (Test-Path -LiteralPath $Path) {
        throw 'The release index output already exists.'
    }
    $temporary = $Path + '.tmp.' + [Guid]::NewGuid().ToString('N')
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
        [IO.File]::Move($temporary, $Path)
    }
    finally {
        if (Test-Path -LiteralPath $temporary) {
            Remove-Item -LiteralPath $temporary -Force -ErrorAction SilentlyContinue
        }
    }
    return Get-FileSignature $Path
}

function Get-CanonicalJsonBytes {
    param([Parameter(Mandatory = $true)]$Value)

    $json = ($Value | ConvertTo-Json -Depth 64) + "`n"
    return (New-Object Text.UTF8Encoding($false)).GetBytes($json)
}

function Test-ByteArrayExact {
    param(
        [Parameter(Mandatory = $true)][byte[]]$Expected,
        [Parameter(Mandatory = $true)][byte[]]$Actual
    )

    if ($Expected.Length -ne $Actual.Length) { return $false }
    for ($index = 0; $index -lt $Expected.Length; $index++) {
        if ($Expected[$index] -ne $Actual[$index]) { return $false }
    }
    return $true
}

function Assert-PublishedGate1RetentionSeal {
    param(
        [Parameter(Mandatory = $true)][string]$RunRoot,
        [Parameter(Mandatory = $true)][string]$IndexPath,
        [Parameter(Mandatory = $true)][string]$ReadyPath,
        [Parameter(Mandatory = $true)]$CanonicalIndex,
        [Parameter(Mandatory = $true)]$RunArtifact
    )

    $indexArtifact = Get-StrictTextArtifact $IndexPath `
        'Published Gate 1 release index'
    Assert-NoDuplicateJsonObjectKeys $indexArtifact.Text `
        'Published Gate 1 release index'
    if ($indexArtifact.Text -match '(?i)(?:[a-z]:[\\/]|\\\\|file://)') {
        throw 'The published Gate 1 release index contains a machine-local path.'
    }
    $expectedIndexBytes = Get-CanonicalJsonBytes $CanonicalIndex
    if (-not (Test-ByteArrayExact $expectedIndexBytes $indexArtifact.Bytes)) {
        throw 'The published Gate 1 release index is not the exact recomputed canonical index.'
    }

    $readyParsed = Read-StrictJsonArtifact $ReadyPath `
        'Published Gate 1 terminal ready seal'
    if ($readyParsed.Artifact.Text -match
            '(?i)(?:[a-z]:[\\/]|\\\\|file://)') {
        throw 'The published Gate 1 terminal ready seal contains a machine-local path.'
    }
    $ready = $readyParsed.Value
    Assert-ExactProperties $ready @(
        'schemaVersion', 'contractId', 'evidenceKind', 'runId', 'candidateId',
        'run', 'index', 'disposition', 'publishedUtc') `
        'Published Gate 1 terminal ready seal'
    Assert-JsonIntegerValue $ready.schemaVersion `
        'Published Gate 1 terminal ready seal.schemaVersion'
    Assert-JsonStringProperties $ready @(
        'contractId', 'evidenceKind', 'runId', 'candidateId', 'disposition',
        'publishedUtc') 'Published Gate 1 terminal ready seal'
    Assert-ExactProperties $ready.run @('path', 'length', 'sha256') `
        'Published Gate 1 terminal ready seal.run'
    Assert-ExactProperties $ready.index @('path', 'length', 'sha256') `
        'Published Gate 1 terminal ready seal.index'
    Assert-JsonStringProperties $ready.run @('path', 'sha256') `
        'Published Gate 1 terminal ready seal.run'
    Assert-JsonStringProperties $ready.index @('path', 'sha256') `
        'Published Gate 1 terminal ready seal.index'
    Assert-JsonIntegerValue $ready.run.length `
        'Published Gate 1 terminal ready seal.run.length'
    Assert-JsonIntegerValue $ready.index.length `
        'Published Gate 1 terminal ready seal.index.length'
    $published = [DateTimeOffset]::MinValue
    if (-not [DateTimeOffset]::TryParseExact(
            [string]$ready.publishedUtc,
            'o',
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::RoundtripKind,
            [ref]$published)) {
        throw 'Published Gate 1 terminal ready time is not an exact round-trip timestamp.'
    }
    if ([int]$ready.schemaVersion -ne 1 -or
        [string]$ready.contractId -cne
            'partisan.gate1-runtime-retention.ready.v1' -or
        [string]$ready.evidenceKind -cne $script:EvidenceKind -or
        [string]$ready.runId -cne [string]$CanonicalIndex.runId -or
        [string]$ready.candidateId -cne [string]$CanonicalIndex.candidateId -or
        [string]$ready.run.path -cne 'run.json' -or
        -not (Test-FileSignatureExact $ready.run $RunArtifact) -or
        [string]$ready.index.path -cne 'release-index.json' -or
        -not (Test-FileSignatureExact $ready.index $indexArtifact) -or
        [string]$ready.disposition -cne
            'passed-noncertifying-retention') {
        throw 'The published Gate 1 terminal ready seal is not exact.'
    }
    return [pscustomobject][ordered]@{
        IndexArtifact = $indexArtifact
        ReadyArtifact = $readyParsed.Artifact
    }
}

if ($VerifyPublishedIndex -and $LibraryOnly) {
    throw 'Published-index verification cannot be combined with library-only mode.'
}
if ($LibraryOnly) { return }

$runArtifact = Get-StrictTextArtifact `
    -Path $RunEnvelopePath `
    -Label 'Retained Gate 1 run envelope'
Assert-NoDuplicateJsonObjectKeys $runArtifact.Text 'Retained Gate 1 run envelope'
if ($runArtifact.Text -match '(?i)(?:[a-z]:[\\/]|\\\\|file://)') {
    throw 'The retained run envelope contains a machine-local path.'
}
try { $run = $runArtifact.Text | ConvertFrom-Json }
catch { throw 'The retained Gate 1 run envelope is not valid JSON.' }
$runRoot = Split-Path -Parent $runArtifact.Path
$terminalReadyPath = Join-Path $runRoot 'run.ready.json'
if (-not $VerifyPublishedIndex -and
    (Test-Path -LiteralPath $terminalReadyPath)) {
    throw 'A terminal ready seal already exists for this Gate 1 run.'
}
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $runRoot 'release-index.json'
}
$outputFull = [IO.Path]::GetFullPath($OutputPath)
if (-not $outputFull.Equals(
        [IO.Path]::GetFullPath((Join-Path $runRoot 'release-index.json')),
        [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The Gate 1 release index must be the fixed run-root sibling.'
}
if ($VerifyPublishedIndex) {
    if (-not (Test-Path -LiteralPath $outputFull -PathType Leaf) -or
        -not (Test-Path -LiteralPath $terminalReadyPath -PathType Leaf)) {
        throw 'Published-index verification requires the fixed index and terminal ready seal.'
    }
}

$guardedModulePath = Join-Path $PSScriptRoot 'Partisan.GuardedRuntime.psm1'
Import-Module -Name $guardedModulePath -Force -ErrorAction Stop
Assert-PartisanNoReparseTree -Root $runRoot

Assert-ExactProperties $run @(
    'schemaVersion', 'evidenceKind', 'contractId', 'runId', 'startedUtc',
    'completedUtc', 'candidate', 'harness', 'scenario', 'configuration',
    'lineage', 'runtime', 'persistence', 'outcome', 'files') `
    'Gate 1 run envelope'
Assert-JsonIntegerValue $run.schemaVersion 'Gate 1 run envelope.schemaVersion'
Assert-JsonStringProperties $run @(
    'evidenceKind', 'contractId', 'runId', 'startedUtc', 'completedUtc') `
    'Gate 1 run envelope'
if ([int]$run.schemaVersion -ne 1 -or
    [string]$run.evidenceKind -cne $script:EvidenceKind -or
    [string]$run.contractId -cne $script:ContractId -or
    [string]$run.runId -cnotmatch '^gate1_[0-9]{8}T[0-9]{6}Z_[0-9a-f]{20}$' -or
    [string]$run.startedUtc -cnotmatch '^\d{4}-\d{2}-\d{2}T' -or
    [string]$run.completedUtc -cnotmatch '^\d{4}-\d{2}-\d{2}T') {
    throw 'The Gate 1 run envelope identity is invalid.'
}

Assert-ExactProperties $run.candidate @(
    'candidateId', 'candidateVersion', 'runtimeUseDisposition', 'gitHead',
    'embeddedBuildSha', 'embeddedBuildUtc', 'embeddedBuildLabel',
    'campaignSchema', 'runtimeSettingsSchema', 'addonId', 'addonGuid',
    'packageHashAlgorithm', 'packageSha256', 'manifestSha256', 'readySha256',
    'workbenchCrc', 'manifestPath', 'readyPath', 'packageRoot', 'packageFiles',
    'executables') 'Gate 1 candidate'
Assert-JsonStringProperties $run.candidate @(
    'candidateId', 'candidateVersion', 'runtimeUseDisposition', 'gitHead',
    'embeddedBuildSha', 'embeddedBuildUtc', 'embeddedBuildLabel', 'addonId',
    'addonGuid', 'packageHashAlgorithm', 'packageSha256', 'manifestSha256',
    'readySha256', 'workbenchCrc', 'manifestPath', 'readyPath', 'packageRoot') `
    'Gate 1 candidate'
Assert-JsonIntegerProperties $run.candidate @(
    'campaignSchema', 'runtimeSettingsSchema') 'Gate 1 candidate'
if ([string]$run.candidate.candidateId -cnotmatch
        '^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$' -or
    [string]$run.candidate.candidateVersion -cnotmatch
        '^[0-9A-Za-z][0-9A-Za-z._-]{0,95}$' -or
    [string]$run.candidate.runtimeUseDisposition -cne
        'active-runtime-candidate' -or
    [string]$run.candidate.gitHead -cnotmatch '^[0-9a-f]{40}$' -or
    [string]$run.candidate.embeddedBuildSha -cnotmatch '^[0-9a-f]{40}$' -or
    [string]::IsNullOrWhiteSpace([string]$run.candidate.embeddedBuildUtc) -or
    [string]::IsNullOrWhiteSpace([string]$run.candidate.embeddedBuildLabel) -or
    [int]$run.candidate.campaignSchema -lt 1 -or
    [int]$run.candidate.runtimeSettingsSchema -lt 1 -or
    [string]$run.candidate.addonId -cnotmatch '^[A-Za-z_][A-Za-z0-9_]*$' -or
    [string]$run.candidate.addonGuid -cnotmatch '^[0-9A-F]{16}$' -or
    [string]$run.candidate.packageHashAlgorithm -cne 'sha256-manifest-v1' -or
    [string]$run.candidate.packageSha256 -cnotmatch '^[0-9a-f]{64}$' -or
    [string]$run.candidate.manifestSha256 -cnotmatch '^[0-9a-f]{64}$' -or
    [string]$run.candidate.readySha256 -cnotmatch '^[0-9a-f]{64}$' -or
    [string]$run.candidate.workbenchCrc -cnotmatch '^[0-9a-f]{8}$') {
    throw 'The retained active candidate identity is invalid.'
}
Assert-ExactProperties $run.candidate.executables @(
    'serverDiagnostic', 'clientDiagnostic', 'server', 'client') `
    'Candidate executable identities'
foreach ($property in @('serverDiagnostic', 'clientDiagnostic', 'server', 'client')) {
    $identity = $run.candidate.executables.$property
    Assert-ExactProperties $identity @(
        'fileName', 'fileVersion', 'productVersion', 'length', 'sha256') `
        "Candidate executable $property"
    Assert-JsonStringProperties $identity @(
        'fileName', 'fileVersion', 'productVersion', 'sha256') `
        "Candidate executable $property"
    Assert-JsonIntegerValue $identity.length `
        "Candidate executable $property.length"
    if ([long]$identity.length -lt 1 -or
        [string]$identity.sha256 -cnotmatch '^[0-9a-f]{64}$') {
        throw "Candidate executable $property identity is invalid."
    }
}
if ([string]$run.candidate.executables.server.fileName -cne
        'ArmaReforgerServer.exe' -or
    [string]$run.candidate.executables.client.fileName -cne
        'ArmaReforgerSteam.exe' -or
    [string]$run.candidate.executables.serverDiagnostic.fileName -cne
        'ArmaReforgerServerDiag.exe' -or
    [string]$run.candidate.executables.clientDiagnostic.fileName -cne
        'ArmaReforgerSteamDiag.exe' -or
    [string]$run.candidate.executables.server.fileVersion -cne
        [string]$run.candidate.executables.client.fileVersion -or
    [string]$run.candidate.executables.server.productVersion -cne
        [string]$run.candidate.executables.client.productVersion -or
    [string]$run.candidate.executables.serverDiagnostic.fileVersion -cne
        [string]$run.candidate.executables.server.fileVersion -or
    [string]$run.candidate.executables.serverDiagnostic.productVersion -cne
        [string]$run.candidate.executables.server.productVersion -or
    [string]$run.candidate.executables.clientDiagnostic.fileVersion -cne
        [string]$run.candidate.executables.client.fileVersion -or
    [string]$run.candidate.executables.clientDiagnostic.productVersion -cne
        [string]$run.candidate.executables.client.productVersion) {
    throw 'The standard server/client runtime pair is not exact or peer-versioned.'
}

$manifestPath = Resolve-PortableFile $runRoot `
    ([string]$run.candidate.manifestPath) 'Candidate manifest path'
$readyPath = Resolve-PortableFile $runRoot `
    ([string]$run.candidate.readyPath) 'Candidate ready path'
$manifestParsed = Read-StrictJsonArtifact $manifestPath 'Copied candidate manifest'
$readyParsed = Read-StrictJsonArtifact $readyPath 'Copied candidate ready marker'
if ($manifestParsed.Artifact.Sha256 -cne [string]$run.candidate.manifestSha256 -or
    $readyParsed.Artifact.Sha256 -cne [string]$run.candidate.readySha256) {
    throw 'Copied candidate or ready bytes are not exact.'
}
Assert-CandidateManifestBinding `
    -Candidate $run.candidate `
    -Manifest $manifestParsed.Value `
    -Ready $readyParsed.Value `
    -ManifestSha256 ([string]$run.candidate.manifestSha256)

$packageRows = @($run.candidate.packageFiles)
$packageRoot = Resolve-PortableFile $runRoot `
    ([string]$run.candidate.packageRoot + '/.root') 'Candidate package root marker'
$packageRoot = Split-Path -Parent $packageRoot
foreach ($row in $packageRows) {
    Assert-ExactProperties $row @('indexPath', 'path', 'length', 'sha256') `
        'Candidate package file row'
    Assert-JsonStringProperties $row @('indexPath', 'path', 'sha256') `
        'Candidate package file row'
    Assert-JsonIntegerValue $row.length 'Candidate package file row.length'
    $path = Resolve-PortableFile $runRoot ([string]$row.path) `
        'Candidate package file'
    if ([string]$row.indexPath -cnotmatch '^Partisan/[^/]+$' -or
        -not (Test-FileSignatureExact $row (Get-FileSignature $path))) {
        throw 'A copied candidate package file differs from its sealed row.'
    }
}
$packageDigestRows = @($packageRows | ForEach-Object {
    [pscustomobject]@{
        path = [string]$_.indexPath
        length = [long]$_.length
        sha256 = [string]$_.sha256
    }
})
if ((Get-CanonicalRowsDigest $packageDigestRows) -cne
        [string]$run.candidate.packageSha256) {
    throw 'The copied candidate package canonical digest is invalid.'
}

$executionMode = [string]$run.scenario.executionMode
if ($executionMode -ceq 'synthetic-self-test') {
    if (-not $SyntheticValidationOnly) {
        throw 'Synthetic fixtures are validation-only and cannot be published.'
    }
    if (-not $VerifyPublishedIndex -and (Test-Path -LiteralPath $outputFull)) {
        throw 'Synthetic fixture validation requires no release index output.'
    }
}
elseif ($SyntheticValidationOnly) {
    throw 'Synthetic validation-only mode requires a synthetic fixture.'
}

Assert-ExactProperties $run.harness @('gitHead', 'clean', 'tools') `
    'Gate 1 harness'
Assert-JsonStringValue $run.harness.gitHead 'Gate 1 harness.gitHead'
Assert-JsonBooleanValue $run.harness.clean 'Gate 1 harness.clean' $true
if ([string]$run.harness.gitHead -cnotmatch '^[0-9a-f]{40}$' -or
    -not [bool]$run.harness.clean) {
    throw 'The Gate 1 harness checkout identity is invalid.'
}
$harnessTools = @($run.harness.tools)
Assert-HarnessToolRoleSet `
    -Tools $harnessTools `
    -RequireComplete ([string]$run.scenario.executionMode -ne
        'synthetic-self-test')
if ([string]$run.scenario.executionMode -ne 'synthetic-self-test') {
    Assert-HarnessGitBinding `
        -RepositoryRoot ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))) `
        -CandidateHead ([string]$run.candidate.gitHead) `
        -HarnessHead ([string]$run.harness.gitHead) `
        -Tools $harnessTools `
        -AllowUnrelatedDirty:$VerifyPublishedIndex
}

Assert-ExactProperties $run.scenario @(
    'executionMode', 'worldResource', 'missionHeader', 'stages',
    'claimScope', 'certificationClaim') 'Gate 1 scenario'
Assert-JsonStringProperties $run.scenario @(
    'executionMode', 'worldResource', 'missionHeader', 'claimScope',
    'certificationClaim') 'Gate 1 scenario'
Assert-JsonStringArray ([object[]]@($run.scenario.stages)) `
    'Gate 1 scenario.stages'
if ([string]$run.scenario.executionMode -notin @(
        'two-phase-engine', 'synthetic-self-test') -or
    [string]$run.scenario.worldResource -cne $script:WorldResource -or
    [string]$run.scenario.missionHeader -cne $script:MissionHeader -or
    [string]$run.scenario.claimScope -cne 'raw-retention-only' -or
    [string]$run.scenario.certificationClaim -cne 'none' -or
    @($run.scenario.stages).Count -ne 5) {
    throw 'The Gate 1 non-certifying scenario contract is invalid.'
}
for ($index = 0; $index -lt $script:Stages.Count; $index++) {
    if ([string]$run.scenario.stages[$index] -cne $script:Stages[$index]) {
        throw 'The Gate 1 stage order is invalid.'
    }
}

Assert-ExactProperties $run.configuration @(
    'settingsPath', 'settingsSha256', 'launchContractPath',
    'launchContractSha256', 'autosaveIntervalSeconds',
    'majorChangeDebounceSeconds') 'Gate 1 configuration'
Assert-JsonStringProperties $run.configuration @(
    'settingsPath', 'settingsSha256', 'launchContractPath',
    'launchContractSha256') 'Gate 1 configuration'
Assert-JsonIntegerProperties $run.configuration @(
    'autosaveIntervalSeconds', 'majorChangeDebounceSeconds') `
    'Gate 1 configuration'
if ([int]$run.configuration.autosaveIntervalSeconds -ne 60 -or
    [int]$run.configuration.majorChangeDebounceSeconds -ne 120) {
    throw 'The Gate 1 persistence scheduler settings are not exact.'
}
$settingsPath = Resolve-PortableFile $runRoot `
    ([string]$run.configuration.settingsPath) 'Runtime settings path'
$settings = Read-StrictJsonArtifact $settingsPath 'Runtime settings'
Assert-JsonIntegerValue $settings.Value.schemaVersion `
    'Runtime settings.schemaVersion'
Assert-JsonIntegerProperties $settings.Value.persistence @(
    'autosaveIntervalSeconds', 'majorChangeDebounceSeconds') `
    'Runtime settings.persistence'
if ($settings.Artifact.Sha256 -cne [string]$run.configuration.settingsSha256 -or
    [int]$settings.Value.schemaVersion -ne
        [int]$run.candidate.runtimeSettingsSchema -or
    [int]$settings.Value.persistence.autosaveIntervalSeconds -ne 60 -or
    [int]$settings.Value.persistence.majorChangeDebounceSeconds -ne 120) {
    throw 'The retained runtime settings are not exact.'
}
$contractPath = Resolve-PortableFile $runRoot `
    ([string]$run.configuration.launchContractPath) 'Launch contract path'
$contractParsed = Read-StrictJsonArtifact $contractPath 'Launch contract'
$contract = $contractParsed.Value
if ($contractParsed.Artifact.Sha256 -cne
        [string]$run.configuration.launchContractSha256) {
    throw 'The launch contract signature is invalid.'
}
Assert-ExactProperties $contract @(
    'schemaVersion', 'contractId', 'runId', 'sessionNonce', 'payloadNonce',
    'buildIdentity', 'worldResource', 'missionHeader', 'projectId', 'stages') `
    'Launch contract'
Assert-JsonIntegerValue $contract.schemaVersion 'Launch contract.schemaVersion'
Assert-JsonStringProperties $contract @(
    'contractId', 'runId', 'sessionNonce', 'payloadNonce', 'worldResource',
    'missionHeader', 'projectId') 'Launch contract'
if ([int]$contract.schemaVersion -ne 1 -or
    [string]$contract.contractId -cne $script:ContractId -or
    [string]$contract.runId -cne [string]$run.runId -or
    [string]$contract.sessionNonce -cnotmatch '^[0-9a-f]{32}$' -or
    [string]$contract.payloadNonce -cnotmatch '^[0-9a-f]{32}$' -or
    @($contract.stages).Count -ne 5) {
    throw 'The portable launch contract identity is invalid.'
}
Assert-LaunchContractRunBinding -Contract $contract -Run $run

Assert-ExactProperties $run.lineage @(
    'executionClass', 'retailClaim', 'contexts') 'Gate 1 diagnostic lineage'
Assert-JsonStringProperties $run.lineage @('executionClass', 'retailClaim') `
    'Gate 1 diagnostic lineage'
if ([string]$run.lineage.executionClass -cne
        'diagnostic-only-save-lineage' -or
    [string]$run.lineage.retailClaim -cne 'none') {
    throw 'The diagnostic save lineage is not explicitly non-retail.'
}
$contexts = @($run.lineage.contexts)
if ($contexts.Count -ne 5) {
    throw 'Gate 1 requires exactly five diagnostic lineage contexts.'
}
$candidateBinding = $null
$diagnosticClientCount = 0
$diagnosticReceiptPaths = New-Object Collections.Generic.HashSet[string](
    [StringComparer]::Ordinal)
for ($index = 0; $index -lt $contexts.Count; $index++) {
    $context = $contexts[$index]
    $stage = $script:Stages[$index]
    Assert-ExactProperties $context @(
        'ordinal', 'stage', 'contextId', 'candidateBindingSha256',
        'serverExitCode', 'clientLaunched', 'serverArgumentsSha256',
        'clientArgumentsSha256', 'receiptPath', 'receiptSignature',
        'journalPath', 'journalSignature', 'completionPath',
        'completionSignature') 'Guarded runtime context row'
    Assert-JsonIntegerProperties $context @('ordinal', 'serverExitCode') `
        'Guarded runtime context row'
    Assert-JsonBooleanValue $context.clientLaunched `
        'Guarded runtime context row.clientLaunched' `
        ($stage -ceq 'shutdown_checkpoint')
    Assert-JsonStringProperties $context @(
        'stage', 'contextId', 'candidateBindingSha256',
        'serverArgumentsSha256', 'clientArgumentsSha256', 'receiptPath',
        'journalPath', 'completionPath') 'Guarded runtime context row'
    Assert-JsonSignatureTypes $context.receiptSignature `
        'Guarded runtime context row.receiptSignature'
    Assert-JsonSignatureTypes $context.journalSignature `
        'Guarded runtime context row.journalSignature'
    Assert-JsonSignatureTypes $context.completionSignature `
        'Guarded runtime context row.completionSignature'
    if ([int]$context.ordinal -ne $index -or
        [string]$context.stage -cne $stage -or
        [string]$context.contextId -cnotmatch '^[0-9a-f]{32}$' -or
        [string]$context.candidateBindingSha256 -cnotmatch '^[0-9a-f]{64}$' -or
        [int]$context.serverExitCode -ne 0 -or
        [bool]$context.clientLaunched -ne ($stage -ceq 'shutdown_checkpoint')) {
        throw "$stage diagnostic lineage context projection is invalid."
    }
    if ($null -eq $candidateBinding) {
        $candidateBinding = [string]$context.candidateBindingSha256
    }
    elseif ($candidateBinding -cne [string]$context.candidateBindingSha256) {
        throw 'The diagnostic lineage contexts do not share one candidate binding.'
    }
    $receiptPath = Resolve-PortableFile $runRoot `
        ([string]$context.receiptPath) 'Diagnostic guarded receipt path'
    $tested = Test-PartisanGuardedRuntimeReceipt `
        -Path $receiptPath `
        -ExpectedSignature $context.receiptSignature
    if (-not [bool]$tested.Complete -or
        [string]$tested.ContextId -cne [string]$context.contextId -or
        [string]$tested.CandidateBindingSha256 -cne $candidateBinding -or
        $tested.FinalRemovalFaultPresent) {
        throw "$stage diagnostic guarded receipt is incomplete."
    }
    Assert-ContextReceiptSidecarBinding `
        -RunRoot $runRoot `
        -Stage $stage `
        -Context $context `
        -Tested $tested
    $receipt = (Read-StrictJsonArtifact $receiptPath `
        "$stage diagnostic guarded receipt").Value
    Assert-GuardedReceiptScalarTypes $receipt `
        "$stage diagnostic guarded receipt"
    $launches = @($receipt.launchBindings)
    $servers = @($launches | Where-Object { [string]$_.role -ceq 'server' })
    $clients = @($launches | Where-Object { [string]$_.role -like 'client-*' })
    if ($servers.Count -ne 1 -or
        $clients.Count -ne [int][bool]$context.clientLaunched -or
        @($launches | Where-Object {
            [string]$_.role -cne 'server' -and [string]$_.role -notlike 'client-*'
        }).Count -ne 0) {
        throw "$stage diagnostic receipt has invalid server/client topology."
    }
    if ([string]$servers[0].provenance.fileName -cne
        'ArmaReforgerServerDiag.exe' -and
        [string]$run.scenario.executionMode -cne 'synthetic-self-test') {
        throw "$stage diagnostic lineage was not created by the diagnostic server."
    }
    if ($clients.Count -eq 1 -and
        [string]$clients[0].provenance.fileName -cne
            'ArmaReforgerSteamDiag.exe' -and
        [string]$run.scenario.executionMode -cne 'synthetic-self-test') {
        throw "$stage diagnostic lineage did not use the diagnostic client."
    }
    if ([string]$run.scenario.executionMode -ne 'synthetic-self-test') {
        foreach ($name in @(
                'fileName', 'fileVersion', 'productVersion', 'length', 'sha256')) {
            if ([string]$servers[0].provenance.$name -cne
                [string]$run.candidate.executables.serverDiagnostic.$name) {
                throw "$stage diagnostic server provenance is not candidate-sealed."
            }
            if ($clients.Count -eq 1 -and
                [string]$clients[0].provenance.$name -cne
                    [string]$run.candidate.executables.clientDiagnostic.$name) {
                throw "$stage diagnostic client provenance is not candidate-sealed."
            }
        }
    }
    foreach ($launch in $launches) {
        Assert-GuardedLaunchScalarTypes `
            -Launch $launch `
            -Label "$stage diagnostic launch" `
            -ExpectedEngine ([string]$run.scenario.executionMode -cne
                'synthetic-self-test')
        if (-not [bool]$launch.isEngine -and
            [string]$run.scenario.executionMode -cne 'synthetic-self-test') {
            throw "$stage production evidence contains a non-engine launch."
        }
        $role = if ([string]$launch.role -ceq 'server') { 'server' } else { 'client' }
        Assert-EngineLaunchTopology `
            -Stage $stage `
            -Role $role `
            -Arguments ([string[]]$launch.arguments) `
            -CandidateId ([string]$run.candidate.candidateId) `
            -PackageSha256 ([string]$run.candidate.packageSha256) `
            -ManifestSha256 ([string]$run.candidate.manifestSha256) `
            -AddonGuid ([string]$run.candidate.addonGuid) `
            -WorldResource ([string]$run.scenario.worldResource) `
            -MissionHeader ([string]$run.scenario.missionHeader) `
            -RunId ([string]$run.runId) `
            -SessionNonce ([string]$contract.sessionNonce) `
            -StageNonce ([string]$contract.stages[$index].stageNonce) `
            -Phase diagnostic-lineage
    }
    $serverArgumentsDigest = Get-ArgumentVectorDigest `
        ([string[]]$servers[0].arguments)
    $clientArgumentsDigest = if ($clients.Count -eq 1) {
        Get-ArgumentVectorDigest ([string[]]$clients[0].arguments)
    }
    else { '' }
    if ($serverArgumentsDigest -cne [string]$context.serverArgumentsSha256 -or
        $clientArgumentsDigest -cne [string]$context.clientArgumentsSha256) {
        throw "$stage portable argument-vector digests are invalid."
    }
    $diagnosticClientCount += $clients.Count
    [void]$diagnosticReceiptPaths.Add([string]$context.receiptPath)
}
if ($diagnosticClientCount -ne 1) {
    throw 'The diagnostic lineage requires exactly one loopback client.'
}

Assert-ExactProperties $run.runtime @(
    'executionClass', 'mutationAuthority', 'byteStabilityClaim', 'contexts') `
    'Gate 1 standard runtime'
Assert-JsonStringProperties $run.runtime @(
    'executionClass', 'mutationAuthority', 'byteStabilityClaim') `
    'Gate 1 standard runtime'
if ([string]$run.runtime.executionClass -cne
        'standard-load-start-log-retention' -or
    [string]$run.runtime.mutationAuthority -cne 'none' -or
    [string]$run.runtime.byteStabilityClaim -cne 'observation-only') {
    throw 'The standard runtime retention claim boundary is invalid.'
}
$standardContexts = @($run.runtime.contexts)
if ($standardContexts.Count -ne 5) {
    throw 'Gate 1 requires exactly five standard runtime contexts.'
}
$clientCount = 0
$runtimeRows = New-Object Collections.Generic.List[object]
$standardLoads = @{}
$standardSnapshots = @{}
for ($index = 0; $index -lt $standardContexts.Count; $index++) {
    $context = $standardContexts[$index]
    Assert-ExactProperties $context @(
        'ordinal', 'stage', 'contextId', 'candidateBindingSha256',
        'serverStopDisposition', 'readinessEvidence', 'clientLaunched',
        'serverArgumentsSha256', 'clientArgumentsSha256', 'receiptPath',
        'receiptSignature', 'journalPath', 'journalSignature',
        'completionPath', 'completionSignature', 'inputSnapshotPath',
        'inputSnapshotSha256', 'outputSnapshotPath',
        'outputSnapshotSha256') 'Standard runtime context row'
    Assert-JsonIntegerValue $context.ordinal 'Standard runtime context row.ordinal'
    Assert-JsonBooleanValue $context.clientLaunched `
        'Standard runtime context row.clientLaunched' ($index -eq 2)
    Assert-JsonStringProperties $context @(
        'stage', 'contextId', 'candidateBindingSha256',
        'serverStopDisposition', 'readinessEvidence', 'serverArgumentsSha256',
        'clientArgumentsSha256', 'receiptPath', 'journalPath',
        'completionPath', 'inputSnapshotPath', 'inputSnapshotSha256',
        'outputSnapshotPath', 'outputSnapshotSha256') `
        'Standard runtime context row'
    Assert-JsonSignatureTypes $context.receiptSignature `
        'Standard runtime context row.receiptSignature'
    Assert-JsonSignatureTypes $context.journalSignature `
        'Standard runtime context row.journalSignature'
    Assert-JsonSignatureTypes $context.completionSignature `
        'Standard runtime context row.completionSignature'
    $stage = $script:Stages[$index]
    if ([int]$context.ordinal -ne $index -or
        [string]$context.stage -cne $stage -or
        [string]$context.contextId -cnotmatch '^[0-9a-f]{32}$' -or
        [string]$context.candidateBindingSha256 -cne $candidateBinding -or
        [string]$context.serverStopDisposition -cne
            'guarded-external-stop-after-log-readiness' -or
        [string]$context.readinessEvidence -cne
            'process-alive-and-console-game-created' -or
        [bool]$context.clientLaunched -ne ($index -eq 2) -or
        [string]$context.inputSnapshotSha256 -cne
            [string]$context.outputSnapshotSha256) {
        throw "$stage standard runtime context projection is invalid."
    }
    if ($diagnosticReceiptPaths.Contains([string]$context.receiptPath)) {
        throw 'A standard runtime context reuses diagnostic lineage evidence.'
    }
    $receiptPath = Resolve-PortableFile $runRoot `
        ([string]$context.receiptPath) 'Standard guarded receipt path'
    $tested = Test-PartisanGuardedRuntimeReceipt `
        -Path $receiptPath `
        -ExpectedSignature $context.receiptSignature
    if (-not $tested.Complete -or $tested.FinalRemovalFaultPresent -or
        [string]$tested.ContextId -cne [string]$context.contextId -or
        [string]$tested.CandidateBindingSha256 -cne $candidateBinding) {
        throw "$stage standard guarded receipt is incomplete."
    }
    Assert-ContextReceiptSidecarBinding `
        -RunRoot $runRoot `
        -Stage $stage `
        -Context $context `
        -Tested $tested
    $receipt = (Read-StrictJsonArtifact $receiptPath `
        "$stage standard guarded receipt").Value
    Assert-GuardedReceiptScalarTypes $receipt `
        "$stage standard guarded receipt"
    $launches = @($receipt.launchBindings)
    $servers = @($launches | Where-Object { [string]$_.role -ceq 'server' })
    $clients = @($launches | Where-Object { [string]$_.role -like 'client-*' })
    if ($servers.Count -ne 1 -or
        $clients.Count -ne [int][bool]$context.clientLaunched) {
        throw "$stage standard receipt has invalid server/client topology."
    }
    if ([string]$servers[0].provenance.fileName -cne 'ArmaReforgerServer.exe' -and
        [string]$run.scenario.executionMode -cne 'synthetic-self-test') {
        throw "$stage standard context did not use the standard server."
    }
    if ([string]$run.scenario.executionMode -ne 'synthetic-self-test') {
        foreach ($name in @(
                'fileName', 'fileVersion', 'productVersion', 'length', 'sha256')) {
            if ([string]$servers[0].provenance.$name -cne
                [string]$run.candidate.executables.server.$name) {
                throw "$stage standard server provenance is not candidate-sealed."
            }
            if ($clients.Count -eq 1 -and
                [string]$clients[0].provenance.$name -cne
                    [string]$run.candidate.executables.client.$name) {
                throw "$stage standard client provenance is not candidate-sealed."
            }
        }
    }
    foreach ($launch in $launches) {
        Assert-GuardedLaunchScalarTypes `
            -Launch $launch `
            -Label "$stage standard launch" `
            -ExpectedEngine ([string]$run.scenario.executionMode -cne
                'synthetic-self-test')
        if (-not [bool]$launch.isEngine -and
            [string]$run.scenario.executionMode -cne 'synthetic-self-test') {
            throw "$stage production standard evidence contains a non-engine launch."
        }
        $role = if ([string]$launch.role -ceq 'server') { 'server' } else { 'client' }
        Assert-EngineLaunchTopology `
            -Stage $stage `
            -Role $role `
            -Arguments ([string[]]$launch.arguments) `
            -CandidateId ([string]$run.candidate.candidateId) `
            -PackageSha256 ([string]$run.candidate.packageSha256) `
            -ManifestSha256 ([string]$run.candidate.manifestSha256) `
            -AddonGuid ([string]$run.candidate.addonGuid) `
            -WorldResource ([string]$run.scenario.worldResource) `
            -MissionHeader ([string]$run.scenario.missionHeader) `
            -Phase standard-retention
    }
    $serverArguments = [string[]]$servers[0].arguments
    $loadPositions = @(Get-OptionPositions $serverArguments '-loadSessionSave')
    $standardLoads[$stage] = if ($loadPositions.Count -eq 1) {
        [string]$serverArguments[$loadPositions[0] + 1]
    }
    else { '' }
    $clientDigest = if ($clients.Count -eq 1) {
        Get-ArgumentVectorDigest ([string[]]$clients[0].arguments)
    }
    else { '' }
    if ((Get-ArgumentVectorDigest $serverArguments) -cne
            [string]$context.serverArgumentsSha256 -or
        $clientDigest -cne [string]$context.clientArgumentsSha256) {
        throw "$stage standard argument-vector digests are invalid."
    }
    $standardInput = Assert-SnapshotManifest `
        -RunRoot $runRoot `
        -ManifestPath ([string]$context.inputSnapshotPath) `
        -Stage $stage `
        -Direction input `
        -ExpectedDigest ([string]$context.inputSnapshotSha256) `
        -ExpectedJournalCount $script:OutputJournalCounts[$index]
    $standardOutput = Assert-SnapshotManifest `
        -RunRoot $runRoot `
        -ManifestPath ([string]$context.outputSnapshotPath) `
        -Stage $stage `
        -Direction output `
        -ExpectedDigest ([string]$context.outputSnapshotSha256) `
        -ExpectedJournalCount $script:OutputJournalCounts[$index]
    if (-not (Test-RowSetExact @($standardInput.files) @($standardOutput.files))) {
        throw "$stage standard context rewrote supplied save bytes."
    }
    $standardSnapshots[$stage + '/input'] = $standardInput
    $standardSnapshots[$stage + '/output'] = $standardOutput
    $clientCount += $clients.Count
    [void]$runtimeRows.Add([pscustomobject][ordered]@{
        ordinal = $index
        stage = $stage
        contextId = [string]$context.contextId
        candidateBindingSha256 = $candidateBinding
        serverStopDisposition = [string]$context.serverStopDisposition
        clientLaunched = [bool]$context.clientLaunched
        receiptPath = [string]$context.receiptPath
        receiptSha256 = [string]$context.receiptSignature.sha256
    })
}
if ($clientCount -ne 1) {
    throw 'Gate 1 requires exactly one standard-runtime client.'
}

Assert-ExactProperties $run.persistence @('stages') 'Gate 1 persistence'
$persistence = @($run.persistence.stages)
if ($persistence.Count -ne 5) { throw 'Gate 1 requires five persistence rows.' }
$snapshots = @{}
for ($index = 0; $index -lt $persistence.Count; $index++) {
    $row = $persistence[$index]
    Assert-ExactProperties $row @(
        'ordinal', 'stage', 'stageNonce', 'source', 'saveType', 'saveName',
        'loadSavePointId', 'createdSavePointId', 'expectedPriorSavePointId',
        'expectedSourceFingerprint', 'expectedSentinelFingerprint',
        'latestSavePointId', 'sourceFingerprint', 'finalFingerprint',
        'journalFileCount', 'journalGeneration', 'journalSlot', 'readOnly',
        'ownerPath', 'guardInputPath', 'resultPath', 'carrierPath',
        'mixedNativeReadyPath', 'endBridgePath', 'inputSnapshotPath',
        'inputSnapshotSha256', 'outputSnapshotPath', 'outputSnapshotSha256') `
        'Persistence stage row'
    Assert-JsonIntegerProperties $row @(
        'ordinal', 'journalFileCount', 'journalGeneration') `
        'Persistence stage row'
    Assert-JsonBooleanValue $row.readOnly 'Persistence stage row.readOnly' `
        ($index -ge 3)
    Assert-JsonStringProperties $row @(
        'stage', 'stageNonce', 'source', 'saveType', 'saveName',
        'loadSavePointId', 'createdSavePointId', 'expectedPriorSavePointId',
        'expectedSourceFingerprint', 'expectedSentinelFingerprint',
        'latestSavePointId', 'sourceFingerprint', 'finalFingerprint',
        'journalSlot', 'ownerPath', 'guardInputPath', 'resultPath',
        'carrierPath', 'mixedNativeReadyPath', 'endBridgePath',
        'inputSnapshotPath', 'inputSnapshotSha256', 'outputSnapshotPath',
        'outputSnapshotSha256') 'Persistence stage row'
    $stage = $script:Stages[$index]
    if ([int]$row.ordinal -ne $index -or [string]$row.stage -cne $stage -or
        [string]$row.stageNonce -cnotmatch '^[0-9a-f]{32}$' -or
        [int]$row.journalFileCount -ne $script:OutputJournalCounts[$index] -or
        [bool]$row.readOnly -ne ($index -ge 3)) {
        throw "$stage persistence row identity is invalid."
    }
    foreach ($portable in @(
        $row.ownerPath, $row.guardInputPath, $row.resultPath,
        $row.carrierPath, $row.inputSnapshotPath, $row.outputSnapshotPath)) {
        $null = Resolve-PortableFile $runRoot ([string]$portable) `
            "$stage retained artifact"
    }
    $snapshots[$stage + '/input'] = Assert-SnapshotManifest `
        -RunRoot $runRoot `
        -ManifestPath ([string]$row.inputSnapshotPath) `
        -Stage $stage `
        -Direction 'input' `
        -ExpectedDigest ([string]$row.inputSnapshotSha256) `
        -ExpectedJournalCount $script:DiagnosticInputJournalCounts[$index]
    $snapshots[$stage + '/output'] = Assert-SnapshotManifest `
        -RunRoot $runRoot `
        -ManifestPath ([string]$row.outputSnapshotPath) `
        -Stage $stage `
        -Direction 'output' `
        -ExpectedDigest ([string]$row.outputSnapshotSha256) `
        -ExpectedJournalCount $script:OutputJournalCounts[$index]
}

$expectedSaveTypes = @('auto', 'manual', 'shutdown', 'none', 'none')
$expectedSaveNames = @(
    'Partisan autosave',
    'Partisan manual checkpoint',
    'Partisan controlled shutdown',
    '', '')
$createdIds = New-Object Collections.Generic.List[string]
for ($index = 0; $index -lt $persistence.Count; $index++) {
    $row = $persistence[$index]
    if ([string]$row.saveType -cne $expectedSaveTypes[$index] -or
        [string]$row.saveName -cne $expectedSaveNames[$index]) {
        throw "$($script:Stages[$index]) save type or name is not exact."
    }
    if ($index -le 2) {
        $id = [string]$row.createdSavePointId
        if ($id -cnotmatch
            '^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$' -or
            $createdIds.Contains($id)) {
            throw "$($script:Stages[$index]) created save ID is invalid or reused."
        }
        [void]$createdIds.Add($id)
    }
    elseif (-not [string]::IsNullOrEmpty([string]$row.createdSavePointId)) {
        throw "$($script:Stages[$index]) unexpectedly reports a created save ID."
    }
}
$expectedNativeMetadata = @(
    [pscustomobject][ordered]@{
        id = [string]$persistence[0].createdSavePointId
        type = 1
        name = 'Partisan autosave'
    },
    [pscustomobject][ordered]@{
        id = [string]$persistence[1].createdSavePointId
        type = 0
        name = 'Partisan manual checkpoint'
    },
    [pscustomobject][ordered]@{
        id = [string]$persistence[2].createdSavePointId
        type = 3
        name = 'Partisan controlled shutdown'
    })
$expectedNativeByStage = @{}
$expectedNativeByStage['autosave_checkpoint'] =
    [object[]]@($expectedNativeMetadata[0])
$expectedNativeByStage['manual_checkpoint'] =
    [object[]]@($expectedNativeMetadata[0], $expectedNativeMetadata[1])
$expectedNativeByStage['shutdown_checkpoint'] =
    [object[]]@($expectedNativeMetadata)
$expectedNativeByStage['native_shutdown_verify'] =
    [object[]]@($expectedNativeMetadata)
$expectedNativeByStage['profile_fallback_verify'] = [object[]]@()
foreach ($stage in $script:Stages) {
    Assert-NativeSnapshotSaveSet `
        -Snapshot $snapshots[$stage + '/output'] `
        -Label "$stage output" `
        -Expected ([object[]]$expectedNativeByStage[$stage])
}

for ($index = 1; $index -le 3; $index++) {
    $previous = $snapshots[$script:Stages[$index - 1] + '/output']
    $current = $snapshots[$script:Stages[$index] + '/input']
    if (-not (Test-RowSetExact @($previous.files) @($current.files))) {
        throw "$($script:Stages[$index]) input is not the preceding stage output."
    }
}
foreach ($stage in @('native_shutdown_verify', 'profile_fallback_verify')) {
    if (-not (Test-RowSetExact `
            @($snapshots[$stage + '/input'].files) `
            @($snapshots[$stage + '/output'].files))) {
        throw "$stage changed its read-only save snapshot."
    }
}
$fallbackNative = @(Get-SnapshotKindRows `
    $snapshots['profile_fallback_verify/input'] 'native')
if ($fallbackNative.Count -ne 0) {
    throw 'Fallback verification input contains native save bytes.'
}
$shutdownJournals = @(Get-SnapshotKindRows `
    $snapshots['native_shutdown_verify/output'] 'journal')
$fallbackJournals = @(Get-SnapshotKindRows `
    $snapshots['profile_fallback_verify/input'] 'journal')
if (-not (Test-RowSetExact $shutdownJournals $fallbackJournals)) {
    throw 'Fallback verification did not receive the exact shutdown journal.'
}

for ($index = 0; $index -lt $script:Stages.Count; $index++) {
    $stage = $script:Stages[$index]
    if (-not (Test-RowSetExact `
            @($snapshots[$stage + '/output'].files) `
            @($standardSnapshots[$stage + '/input'].files))) {
        throw "$stage standard context did not receive exact diagnostic lineage bytes."
    }
}
$expectedStandardLoads = @(
    [string]$persistence[0].createdSavePointId,
    [string]$persistence[1].createdSavePointId,
    [string]$persistence[2].createdSavePointId,
    [string]$persistence[2].createdSavePointId,
    '')
for ($index = 0; $index -lt $script:Stages.Count; $index++) {
    if ([string]$standardLoads[$script:Stages[$index]] -cne
        [string]$expectedStandardLoads[$index]) {
        throw "$($script:Stages[$index]) standard load UUID is not lineage-bound."
    }
}
Assert-LaunchContractPersistenceBinding `
    -Contract $contract `
    -Persistence ([object[]]$persistence)

if ([string]$run.scenario.executionMode -ceq 'two-phase-engine') {
    $ordinaryPath = Join-Path $PSScriptRoot `
        'run-ordinary-campaign-persistence-proof.ps1'
    . $ordinaryPath `
        -Executable $env:ComSpec `
        -RuntimeAddonRoot $runRoot `
        -WorkbenchExecutable $env:ComSpec `
        -LibraryOnly
    foreach ($row in $persistence) {
        $result = (Read-StrictJsonArtifact `
            (Resolve-PortableFile $runRoot ([string]$row.resultPath) 'Stage result') `
            'Ordinary stage result').Value
        $validated = Assert-StageResult `
            -Result $result `
            -SessionNonce ([string]$contract.sessionNonce) `
            -StageNonce ([string]$row.stageNonce) `
            -RunId ([string]$run.runId) `
            -PayloadNonce ([string]$contract.payloadNonce) `
            -Stage ([string]$row.stage) `
            -ExpectedBuild $contract.buildIdentity `
            -ExpectedWorld ([string]$contract.worldResource) `
            -ExpectedPriorSavePointId ([string]$row.expectedPriorSavePointId) `
            -ExpectedSourceFingerprint ([string]$row.expectedSourceFingerprint) `
            -ExpectedSentinelFingerprint ([string]$row.expectedSentinelFingerprint)
        $carrier = (Read-StrictJsonArtifact `
            (Resolve-PortableFile $runRoot ([string]$row.carrierPath) 'Stage carrier') `
            'Ordinary stage carrier').Value
        $null = Assert-Carrier `
            -Carrier $carrier `
            -SessionNonce ([string]$contract.sessionNonce) `
            -RunId ([string]$run.runId) `
            -PayloadNonce ([string]$contract.payloadNonce) `
            -Stage ([string]$row.stage) `
            -ExpectedBuild $contract.buildIdentity `
            -ExpectedWorld ([string]$contract.worldResource) `
            -Result $validated `
            -ExpectedLatestSavePointId ([string]$row.latestSavePointId)
    }
}

Assert-ExactProperties $run.outcome @(
    'success', 'diagnosticServerLaunchCount', 'diagnosticClientLaunchCount',
    'standardServerLaunchCount', 'standardClientLaunchCount',
    'configsRetained', 'logsRetained', 'saveBytesRetained',
    'guardedReceiptsComplete', 'standardSaveBytesStable', 'disposition') `
    'Gate 1 outcome'
foreach ($name in @(
        'success', 'configsRetained', 'logsRetained', 'saveBytesRetained',
        'guardedReceiptsComplete', 'standardSaveBytesStable')) {
    Assert-JsonBooleanValue $run.outcome.$name "Gate 1 outcome.$name" $true
}
Assert-JsonIntegerProperties $run.outcome @(
    'diagnosticServerLaunchCount', 'diagnosticClientLaunchCount',
    'standardServerLaunchCount', 'standardClientLaunchCount') 'Gate 1 outcome'
Assert-JsonStringValue $run.outcome.disposition 'Gate 1 outcome.disposition'
if (-not [bool]$run.outcome.success -or
    [int]$run.outcome.diagnosticServerLaunchCount -ne 5 -or
    [int]$run.outcome.diagnosticClientLaunchCount -ne 1 -or
    [int]$run.outcome.standardServerLaunchCount -ne 5 -or
    [int]$run.outcome.standardClientLaunchCount -ne 1 -or
    -not [bool]$run.outcome.configsRetained -or
    -not [bool]$run.outcome.logsRetained -or
    -not [bool]$run.outcome.saveBytesRetained -or
    -not [bool]$run.outcome.guardedReceiptsComplete -or
    -not [bool]$run.outcome.standardSaveBytesStable -or
    [string]$run.outcome.disposition -cne
        'passed-noncertifying-retention') {
    throw 'The Gate 1 terminal outcome is invalid.'
}

$seenFiles = New-Object Collections.Generic.HashSet[string]([StringComparer]::Ordinal)
$fileRows = @($run.files)
foreach ($row in $fileRows) {
    Assert-ExactProperties $row @('role', 'stage', 'path', 'length', 'sha256') `
        'Gate 1 file row'
    Assert-JsonStringProperties $row @('role', 'stage', 'path', 'sha256') `
        'Gate 1 file row'
    Assert-JsonIntegerValue $row.length 'Gate 1 file row.length'
    $projection = Get-ExpectedGateFileProjection ([string]$row.path)
    if ([string]$row.role -cne [string]$projection.role -or
        [string]$row.stage -cne [string]$projection.stage -or
        [long]$row.length -lt 0 -or
        ([string]$projection.role -like '*journal*' -and
            [long]$row.length -lt 1) -or
        [string]$row.sha256 -cnotmatch '^[0-9a-f]{64}$' -or
        -not $seenFiles.Add([string]$row.path)) {
        throw 'A Gate 1 file role or stage is not derived, valid, and unique.'
    }
    $path = Resolve-PortableFile $runRoot ([string]$row.path) 'Gate 1 file row path'
    if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or
        -not (Test-FileSignatureExact $row (Get-FileSignature $path))) {
        throw 'A retained Gate 1 file differs from its run census.'
    }
}
$controlFiles = @('run.json', 'release-index.json', 'run.ready.json')
$actualFiles = @(Get-ChildItem -LiteralPath $runRoot -Recurse -File -Force |
    ForEach-Object {
        ConvertTo-PortableRelativePath $runRoot $_.FullName `
            'Run census file'
    } | Where-Object { $_ -notin $controlFiles } | Sort-Object)
$expectedFiles = @($fileRows | ForEach-Object { [string]$_.path } | Sort-Object)
if (@(Compare-Object $expectedFiles $actualFiles -CaseSensitive).Count -ne 0) {
    throw 'The run file census is not complete and exact.'
}
foreach ($role in @('server_console_log', 'server_script_log',
        'server_error_log', 'server_crash_log')) {
    if (@($fileRows | Where-Object { [string]$_.role -ceq $role }).Count -ne 5) {
        throw "The retained evidence does not contain five $role files."
    }
}
foreach ($role in @('client_console_log', 'client_script_log',
        'client_error_log', 'client_crash_log')) {
    if (@($fileRows | Where-Object { [string]$_.role -ceq $role }).Count -ne 1) {
        throw "The retained evidence does not contain one $role file."
    }
}
foreach ($role in @('diagnostic_server_console_log',
        'diagnostic_server_script_log', 'diagnostic_server_error_log',
        'diagnostic_server_crash_log')) {
    if (@($fileRows | Where-Object { [string]$_.role -ceq $role }).Count -ne 5) {
        throw "The retained evidence does not contain five $role files."
    }
}
foreach ($role in @('diagnostic_client_console_log',
        'diagnostic_client_script_log', 'diagnostic_client_error_log',
        'diagnostic_client_crash_log')) {
    if (@($fileRows | Where-Object { [string]$_.role -ceq $role }).Count -ne 1) {
        throw "The retained evidence does not contain one $role file."
    }
}
if (@($fileRows | Where-Object {
        [string]$_.role -ceq 'guarded_runtime_receipt'
    }).Count -ne 5 -or
    @($fileRows | Where-Object {
        [string]$_.role -ceq 'diagnostic_guarded_runtime_receipt'
    }).Count -ne 5 -or
    @($fileRows | Where-Object {
        [string]$_.role -ceq 'save_snapshot_manifest'
    }).Count -ne 20) {
    throw 'The retained receipt or save-snapshot topology is incomplete.'
}

$indexRows = @($fileRows | Sort-Object path | ForEach-Object {
    [pscustomobject][ordered]@{
        role = [string]$_.role
        stage = [string]$_.stage
        path = [string]$_.path
        length = [long]$_.length
        sha256 = [string]$_.sha256
    }
})
$index = [ordered]@{
    schemaVersion = 1
    contractId = $script:IndexContractId
    evidenceKind = $script:EvidenceKind
    runId = [string]$run.runId
    candidateId = [string]$run.candidate.candidateId
    gitHead = [string]$run.candidate.gitHead
    packageSha256 = [string]$run.candidate.packageSha256
    candidateBindingSha256 = [string]$candidateBinding
    run = [ordered]@{
        path = 'run.json'
        length = [long]$runArtifact.Length
        sha256 = [string]$runArtifact.Sha256
    }
    topology = [ordered]@{
        stageCount = 5
        diagnosticContextCount = 5
        diagnosticServerLaunchCount = 5
        diagnosticClientLaunchCount = 1
        standardContextCount = 5
        standardServerLaunchCount = 5
        standardClientLaunchCount = 1
        standardStages = [object[]]$runtimeRows.ToArray()
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
        executionMode = [string]$run.scenario.executionMode
    }
    files = [object[]]$indexRows
    filesAggregateSha256 = Get-CanonicalRowsDigest $indexRows
    disposition = 'passed-noncertifying-retention'
}
if ($VerifyPublishedIndex) {
    $seal = Assert-PublishedGate1RetentionSeal `
        -RunRoot $runRoot `
        -IndexPath $outputFull `
        -ReadyPath $terminalReadyPath `
        -CanonicalIndex $index `
        -RunArtifact $runArtifact
    Write-Output ([pscustomobject][ordered]@{
        ValidationKind =
            'partisan_gate1_runtime_retention_published_index_verification_v1'
        PublishedIndexValid = $true
        ReadySealValid = $true
        ReadOnlyVerification = $true
        SyntheticFixture = ($executionMode -ceq 'synthetic-self-test')
        RunId = [string]$run.runId
        CandidateId = [string]$run.candidate.candidateId
        CandidateBindingSha256 = [string]$candidateBinding
        FileCount = [int]$indexRows.Count
        IndexSignature = [pscustomobject][ordered]@{
            length = [long]$seal.IndexArtifact.Length
            sha256 = [string]$seal.IndexArtifact.Sha256
        }
        ReadySignature = [pscustomobject][ordered]@{
            length = [long]$seal.ReadyArtifact.Length
            sha256 = [string]$seal.ReadyArtifact.Sha256
        }
        Disposition = 'passed-noncertifying-retention'
    })
    return
}
if ($executionMode -ceq 'synthetic-self-test') {
    if (Test-Path -LiteralPath $outputFull) {
        throw 'Synthetic fixture validation created a forbidden release index.'
    }
    Write-Output ([pscustomobject][ordered]@{
        Path = ''
        Signature = $null
        RunId = [string]$run.runId
        CandidateId = [string]$run.candidate.candidateId
        FileCount = $indexRows.Count
        Disposition = 'validated-synthetic-fixture-only'
    })
    return
}
if (Test-Path -LiteralPath $terminalReadyPath) {
    throw 'A terminal ready seal appeared before Gate 1 index publication.'
}
$signature = Write-JsonAtomicCreateOnly -Path $outputFull -Value $index
Write-Output ([pscustomobject][ordered]@{
    Path = $outputFull
    Signature = $signature
    RunId = [string]$run.runId
    CandidateId = [string]$run.candidate.candidateId
    FileCount = $indexRows.Count
    Disposition = 'passed-noncertifying-retention'
})
