[CmdletBinding()]
param(
    [string]$ManifestPath = '',
    [string]$BundleRoot = '',
    [string]$RuntimeAddonRoot = '',
    [string]$ServerDiagnosticExecutable = '',
    [string]$EvidenceRoot = '',
    [string[]]$WatchedRoots = @(),
    [string[]]$SpillRoots = @(),
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 300,
    [ValidateRange(100, 5000)][int]$PollMilliseconds = 250,
    [ValidateRange(1, 65535)][int]$LoopbackPort = 2001,
    [string]$WorldResource = 'Worlds/HST_Dev/HST_Dev.ent',
    [string]$MissionHeader = 'Missions/HST_Dev.conf',
    [switch]$SelfTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:EvidenceKind = 'partisan_release_surface_runtime_audit_v2'
$script:ProbeEvidenceKind = 'partisan_release_surface_runtime_probe_v1'
$script:RunContractId = 'partisan.release-surface-audit.run.v2'
$script:HarnessPrefix = 'PartisanReleaseSurfaceAudit_'
$script:HarnessId = 'PartisanReleaseSurfaceAudit'
$script:HarnessOwnerLeaf = '.partisan-release-surface-audit-owner.json'
$script:Modes = @('retail', 'diagnostic')
$script:RequiredLogLeaves = @('console.log', 'script.log', 'error.log')
$script:OptionalLogLeaves = @('crash.log')
$script:AllowedLogLeaves = @(
    $script:RequiredLogLeaves + $script:OptionalLogLeaves)
$script:HardDiagnosticPolicy = 'script-engine-and-process-fatal-v1'

function Resolve-ReleaseSurfacePath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][ValidateSet('Leaf', 'Container')]
        [string]$Kind
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        throw "A required $Kind path is empty."
    }
    $full = [IO.Path]::GetFullPath($Path)
    $pathType = if ($Kind -ceq 'Leaf') { 'Leaf' } else { 'Container' }
    if (-not (Test-Path -LiteralPath $full -PathType $pathType)) {
        throw "A required $Kind path does not exist."
    }
    return $full
}

function Get-ReleaseSurfaceFileSignature {
    param([Parameter(Mandatory = $true)][string]$Path)

    $file = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    if ($file.PSIsContainer) {
        throw 'A release-surface signature target is a directory.'
    }
    return [pscustomobject][ordered]@{
        length = [long]$file.Length
        sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).
            Hash.ToLowerInvariant()
    }
}

function Test-ReleaseSurfaceFileSignatureExact {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual
    )

    return $null -ne $Expected -and $null -ne $Actual -and
        [long]$Expected.length -eq [long]$Actual.length -and
        [string]$Expected.sha256 -ceq [string]$Actual.sha256
}

function New-ReleaseSurfaceToolBinding {
    param(
        [Parameter(Mandatory = $true)][string]$Role,
        [Parameter(Mandatory = $true)][string]$PortablePath,
        [Parameter(Mandatory = $true)][string]$Path
    )

    if ($Role -cnotmatch '^[A-Za-z][A-Za-z0-9]*$' -or
        [string]::IsNullOrWhiteSpace($PortablePath) -or
        $PortablePath.Contains('\') -or $PortablePath.Contains(':') -or
        $PortablePath.StartsWith('/') -or
        $PortablePath.Split('/') -contains '..') {
        throw 'A release-surface tool binding is not portable.'
    }
    $signature = Get-ReleaseSurfaceFileSignature -Path $Path
    return [pscustomobject][ordered]@{
        role = $Role
        path = $PortablePath
        length = [long]$signature.length
        sha256 = [string]$signature.sha256
    }
}

function Get-ReleaseSurfaceTextSha256 {
    param([Parameter(Mandatory = $true)][AllowEmptyString()][string]$Text)

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = (New-Object Text.UTF8Encoding($false)).GetBytes($Text)
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $algorithm.Dispose()
    }
}

function Write-ReleaseSurfaceText {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)][string]$Text,
        [switch]$CreateOnly
    )

    $full = [IO.Path]::GetFullPath($Path)
    if ($CreateOnly -and (Test-Path -LiteralPath $full)) {
        throw 'A create-only release-surface path already exists.'
    }
    $parent = Split-Path -Parent $full
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        New-Item -ItemType Directory -Path $parent -Force -ErrorAction Stop |
            Out-Null
    }
    $encoding = New-Object Text.UTF8Encoding($false)
    if ($CreateOnly) {
        $temporary = Join-Path $parent (
            '.' + (Split-Path -Leaf $full) + '.tmp.' +
            [Guid]::NewGuid().ToString('N'))
        try {
            $bytes = $encoding.GetBytes($Text)
            $stream = New-Object IO.FileStream(
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
            [IO.File]::Move($temporary, $full)
        }
        finally {
            if (Test-Path -LiteralPath $temporary) {
                Remove-Item -LiteralPath $temporary -Force `
                    -ErrorAction SilentlyContinue
            }
        }
    }
    else {
        [IO.File]::WriteAllText($full, $Text, $encoding)
    }
    return Get-ReleaseSurfaceFileSignature -Path $full
}

function Write-ReleaseSurfaceJson {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value,
        [switch]$Portable,
        [switch]$CreateOnly
    )

    $json = ($Value | ConvertTo-Json -Depth 64) + "`n"
    if ($Portable -and $json -match
        '(?i)(?:[A-Z]:[\\/]|\\\\|file://|/(?:Users|home|mnt)/)') {
        throw 'A portable release-surface publication contains a local path.'
    }
    return Write-ReleaseSurfaceText `
        -Path $Path `
        -Text $json `
        -CreateOnly:$CreateOnly
}

function ConvertTo-ReleaseSurfaceRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Path
    )

    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $full = [IO.Path]::GetFullPath($Path)
    $prefix = $rootPath + [IO.Path]::DirectorySeparatorChar
    if (-not $full.StartsWith(
            $prefix,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw 'A retained release-surface path escaped its run root.'
    }
    $relative = $full.Substring($prefix.Length).Replace('\', '/')
    if ([string]::IsNullOrWhiteSpace($relative) -or
        $relative.Contains(':') -or
        $relative.Split('/') -contains '..') {
        throw 'A retained release-surface path is not portable.'
    }
    return $relative
}

function Test-ReleaseSurfacePathOverlap {
    param(
        [Parameter(Mandatory = $true)][string]$First,
        [Parameter(Mandatory = $true)][string]$Second
    )

    $left = [IO.Path]::GetFullPath($First).TrimEnd('\', '/')
    $right = [IO.Path]::GetFullPath($Second).TrimEnd('\', '/')
    if ($left.Equals($right, [StringComparison]::OrdinalIgnoreCase)) {
        return $true
    }
    $separator = [IO.Path]::DirectorySeparatorChar
    return $left.StartsWith(
            $right + $separator,
            [StringComparison]::OrdinalIgnoreCase) -or
        $right.StartsWith(
            $left + $separator,
            [StringComparison]::OrdinalIgnoreCase)
}

function Get-ReleaseSurfaceTreeBinding {
    param([Parameter(Mandatory = $true)][string]$Root)

    $rootPath = Resolve-ReleaseSurfacePath -Path $Root -Kind Container
    $prefix = $rootPath.TrimEnd('\', '/') +
        [IO.Path]::DirectorySeparatorChar
    $rows = New-Object Collections.Generic.List[object]
    foreach ($file in @(Get-ChildItem -LiteralPath $rootPath -Recurse -File `
            -Force -ErrorAction Stop | Sort-Object FullName)) {
        $full = [IO.Path]::GetFullPath($file.FullName)
        if (-not $full.StartsWith(
                $prefix,
                [StringComparison]::OrdinalIgnoreCase)) {
            throw 'A release-surface harness file escaped its root.'
        }
        $relative = $full.Substring($prefix.Length).Replace('\', '/')
        $signature = Get-ReleaseSurfaceFileSignature -Path $full
        [void]$rows.Add([pscustomobject][ordered]@{
            path = $relative
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        })
    }
    if ($rows.Count -eq 0) {
        throw 'A release-surface harness tree is empty.'
    }
    $lines = @($rows.ToArray() | Sort-Object path | ForEach-Object {
        "{0}`t{1}`t{2}" -f $_.sha256, [long]$_.length, $_.path
    })
    return [pscustomobject][ordered]@{
        files = [object[]]$rows.ToArray()
        aggregateSha256 = Get-ReleaseSurfaceTextSha256 `
            -Text (($lines -join "`n") + "`n")
    }
}

function Assert-ReleaseSurfaceTreeBinding {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $actual = Get-ReleaseSurfaceTreeBinding -Root $Root
    if ([string]$actual.aggregateSha256 -cne
            [string]$Expected.aggregateSha256 -or
        @($actual.files).Count -ne @($Expected.files).Count) {
        throw "$Label tree identity changed."
    }
    $expectedRows = @($Expected.files | Sort-Object path | ForEach-Object {
        '{0}|{1}|{2}' -f $_.path, [long]$_.length, $_.sha256
    })
    $actualRows = @($actual.files | Sort-Object path | ForEach-Object {
        '{0}|{1}|{2}' -f $_.path, [long]$_.length, $_.sha256
    })
    if (@(Compare-Object -ReferenceObject $expectedRows `
            -DifferenceObject $actualRows -CaseSensitive).Count -ne 0) {
        throw "$Label tree inventory changed."
    }
    return $actual
}

function Assert-ReleaseSurfaceCandidateIdentity {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual
    )

    foreach ($property in @(
            'CandidateId',
            'CandidateVersion',
            'RuntimeUseDisposition',
            'GitHead',
            'EmbeddedBuildSha',
            'EmbeddedBuildUtc',
            'EmbeddedBuildLabel',
            'CampaignSchema',
            'RuntimeSettingsSchema',
            'AddonId',
            'AddonGuid',
            'PackageHashAlgorithm',
            'PackageSha256',
            'ManifestSha256',
            'ReadySha256',
            'WorkbenchCrc')) {
        if ([string]$Expected.$property -cne [string]$Actual.$property) {
            throw "The sealed candidate identity drifted at $property."
        }
    }
}

function Assert-ReleaseSurfaceExecutableIdentity {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($property in @(
            'fileName', 'fileVersion', 'productVersion', 'length', 'sha256')) {
        if ([string]$Expected.$property -cne [string]$Actual.$property) {
            throw "$Label executable identity drifted at $property."
        }
    }
}

function Assert-ReleaseSurfaceArguments {
    param(
        [Parameter(Mandatory = $true)][ValidateSet('retail', 'diagnostic')]
        [string]$Mode,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $definePositions = @()
    $symbolPositions = @()
    for ($index = 0; $index -lt $Arguments.Count; $index++) {
        $argument = [string]$Arguments[$index]
        if ($argument -match '(?i)scrDefine') { $definePositions += $index }
        if ($argument -match '(?i)^ENABLE_DIAG$') { $symbolPositions += $index }
    }
    if ($Mode -ceq 'retail') {
        if ($definePositions.Count -ne 0 -or $symbolPositions.Count -ne 0) {
            throw 'Retail release-surface arguments must not define script symbols.'
        }
    }
    elseif ($definePositions.Count -ne 1 -or
        $symbolPositions.Count -ne 1 -or
        [string]$Arguments[$definePositions[0]] -cne '-scrDefine' -or
        $definePositions[0] + 1 -ge $Arguments.Count -or
        $symbolPositions[0] -ne $definePositions[0] + 1 -or
        [string]$Arguments[$symbolPositions[0]] -cne 'ENABLE_DIAG') {
        throw ('Diagnostic release-surface arguments must contain exactly one ' +
            "'-scrDefine', 'ENABLE_DIAG' pair.")
    }
    foreach ($flag in @(
            '-addonsDir',
            '-gproj',
            '-server',
            '-MissionHeader',
            '-addons',
            '-profile',
            '-logsDir',
            '-addonTempDir',
            '-hstReleaseCandidateId',
            '-hstReleasePackageSha256',
            '-hstReleaseManifestSha256',
            '-releaseSurfaceRunNonce',
            '-releaseSurfaceExpectedMode')) {
        $positions = @()
        for ($index = 0; $index -lt $Arguments.Count; $index++) {
            if ([string]$Arguments[$index] -ceq $flag) {
                $positions += $index
            }
        }
        if ($positions.Count -ne 1 -or
            $positions[0] + 1 -ge $Arguments.Count) {
            throw "Release-surface runtime option is missing or duplicated: $flag."
        }
    }
    foreach ($forbidden in @(
            '-world',
            '-config',
            '-backendLocalStorage',
            '-backendDisableStorage',
            '-noBackend',
            '-rpl-validation-rdb-disable',
            '-rpl-validation-scr-disable',
            '-rpl-validation-version-disable',
            '-rpl-validation-devbin-disable',
            '-rpl-validation-addons-disable')) {
        if ($Arguments -icontains $forbidden) {
            throw "Release-surface runtime arguments contain forbidden option $forbidden."
        }
    }
}

function ConvertTo-ReleaseSurfacePortableArguments {
    param(
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)]$Replacements
    )

    $rows = New-Object Collections.Generic.List[string]
    $replacementRows = @($Replacements | Sort-Object {
        -1 * ([string]$_.path).Length
    })
    foreach ($raw in $Arguments) {
        $value = [string]$raw
        foreach ($replacement in $replacementRows) {
            $path = [string]$replacement.path
            if (-not [string]::IsNullOrWhiteSpace($path)) {
                $value = $value.Replace(
                    $path,
                    [string]$replacement.token)
            }
        }
        if ($value -match
            '(?i)(?:[A-Z]:[\\/]|\\\\|file://|/(?:Users|home|mnt)/)') {
            throw 'A portable argument retained an unredacted local path.'
        }
        [void]$rows.Add($value.Replace('\', '/'))
    }
    return [string[]]$rows.ToArray()
}

function Get-ReleaseSurfaceContract {
    param([Parameter(Mandatory = $true)][string]$Path)

    $contract = Get-Content -LiteralPath $Path -Raw -ErrorAction Stop |
        ConvertFrom-Json
    foreach ($property in @(
            'schemaVersion',
            'evidenceKind',
            'diagnosticSymbol',
            'expectedForbiddenTypeCount',
            'expectedForbiddenCommandActionIdCount',
            'expectedForbiddenMemberSurfaceCount',
            'expectedForbiddenLiteralSurfaceCount',
            'expectedProductionObservabilityMemberSurfaceCount',
            'commandSurfaceFiles',
            'forbiddenTypeNames',
            'forbiddenCommandActionIds',
            'productionPositiveControlTypeNames',
            'productionPositiveControlCommandActionIds',
            'forbiddenMemberSurfaces',
            'forbiddenLiteralSurfaces',
            'productionObservabilityMemberSurfaces')) {
        if ($null -eq $contract.PSObject.Properties[$property]) {
            throw "Release-surface contract is missing $property."
        }
    }
    if ([int]$contract.schemaVersion -ne 1 -or
        [string]$contract.evidenceKind -cne
            'partisan_release_surface_contract_v1' -or
        [string]$contract.diagnosticSymbol -cne 'ENABLE_DIAG' -or
        @($contract.forbiddenTypeNames).Count -ne
            [int]$contract.expectedForbiddenTypeCount -or
        @($contract.forbiddenCommandActionIds).Count -ne
            [int]$contract.expectedForbiddenCommandActionIdCount -or
        @($contract.forbiddenMemberSurfaces).Count -ne
            [int]$contract.expectedForbiddenMemberSurfaceCount -or
        @($contract.forbiddenLiteralSurfaces).Count -ne
            [int]$contract.expectedForbiddenLiteralSurfaceCount -or
        @($contract.productionObservabilityMemberSurfaces).Count -ne
            [int]$contract.expectedProductionObservabilityMemberSurfaceCount) {
        throw 'Release-surface contract identity or counts are invalid.'
    }
    $commandSurfaceFiles = @($contract.commandSurfaceFiles)
    if ($commandSurfaceFiles.Count -eq 0 -or
        @($commandSurfaceFiles | Sort-Object -Unique).Count -ne
            $commandSurfaceFiles.Count) {
        throw 'Release-surface contract command-surface files are invalid.'
    }
    foreach ($file in $commandSurfaceFiles) {
        if ([string]$file -cnotmatch
            '^Scripts/Game/HST/(?:Components|Services)/[A-Za-z0-9_]+\.c$') {
            throw 'Release-surface contract contains an unsafe command-surface file.'
        }
    }
    foreach ($name in @(
            @($contract.forbiddenTypeNames) +
            @($contract.productionPositiveControlTypeNames))) {
        if ([string]$name -cnotmatch '^[A-Za-z_][A-Za-z0-9_]*$') {
            throw 'Release-surface contract contains an unsafe type name.'
        }
    }
    foreach ($id in @(
            @($contract.forbiddenCommandActionIds) +
            @($contract.productionPositiveControlCommandActionIds))) {
        if ([string]$id -cnotmatch '^[a-z][a-z0-9_]*$') {
            throw 'Release-surface contract contains an unsafe command action ID.'
        }
    }
    return $contract
}

function New-ReleaseSurfaceInsertBlock {
    param(
        [Parameter(Mandatory = $true)][object[]]$Values,
        [Parameter(Mandatory = $true)][string]$Variable
    )

    return (@($Values | ForEach-Object {
        $escaped = ([string]$_).
            Replace('\', '\\').
            Replace('"', '\"').
            Replace("`r", '\r').
            Replace("`n", '\n')
        "`t`t$Variable.Insert(`"$escaped`");"
    }) -join "`n")
}

function New-ReleaseSurfaceMemberProbePlan {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)]$Contract,
        [string]$SourceCommit = ''
    )

    $root = [IO.Path]::GetFullPath($RepositoryRoot).TrimEnd('\', '/')
    $rootPrefix = $root + [IO.Path]::DirectorySeparatorChar
    if (-not [string]::IsNullOrWhiteSpace($SourceCommit) -and
        $SourceCommit -cnotmatch '^[0-9a-f]{40}$') {
        throw 'The runtime member-probe source commit is invalid.'
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
                throw 'A runtime member-probe surface is unsafe.'
            }
            [void]$requests.Add([pscustomobject][ordered]@{
                category = [string]$set.category
                surface = $surface
                path = [string]$Matches.path
                memberName = [string]$Matches.member
            })
        }
    }
    if ($requests.Count -ne
            @($Contract.forbiddenMemberSurfaces).Count +
            @($Contract.productionObservabilityMemberSurfaces).Count -or
        @($requests.surface | Sort-Object -Unique -CaseSensitive).Count -ne
            $requests.Count) {
        throw 'The runtime member-probe surface set is duplicated or incomplete.'
    }

    $resolved = New-Object Collections.Generic.List[object]
    $memberMethodPattern = [regex]::new(
        '^\s*(?!return\b)' +
        '(?:(?:protected|private|static|override|event|proto|sealed|ref|autoptr)\s+)*' +
        '(?:[A-Za-z_][A-Za-z0-9_<>\[\],.]*\s+)+' +
        '(?<member>[A-Za-z_][A-Za-z0-9_]*)\s*\(',
        [Text.RegularExpressions.RegexOptions]::CultureInvariant)
    $memberFieldPattern = [regex]::new(
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

    foreach ($pathGroup in @($requests.ToArray() | Group-Object path)) {
        $portablePath = [string]$pathGroup.Name
        $fullPath = [IO.Path]::GetFullPath(
            (Join-Path $root $portablePath.Replace('/', '\')))
        if (-not $fullPath.StartsWith(
                $rootPrefix,
                [StringComparison]::OrdinalIgnoreCase) -or
            -not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
            throw 'A runtime member-probe source path escaped the repository.'
        }
        $pending = @($pathGroup.Group | Where-Object {
            [string]$_.memberName -cne 'forceDebug'
        })
        $sourceLines = @()
        if ([string]::IsNullOrWhiteSpace($SourceCommit)) {
            $sourceLines = @([IO.File]::ReadAllLines($fullPath))
        }
        else {
            $blobSpec = $SourceCommit + ':' + $portablePath
            $sourceLines = @(& git -C $root show --no-textconv $blobSpec 2>$null)
            if ($LASTEXITCODE -ne 0 -or $sourceLines.Count -eq 0) {
                throw "Runtime member-probe source is absent at candidate commit: $portablePath."
            }
        }
        $currentType = ''
        $lineNumber = 0
        foreach ($line in $sourceLines) {
            $lineNumber++
            $classMatch = $classPattern.Match($line)
            if ($classMatch.Success) {
                $currentType = [string]$classMatch.Groups['type'].Value
            }
            if ([string]::IsNullOrWhiteSpace($currentType)) {
                continue
            }
            $declaration = $memberMethodPattern.Match($line)
            $probeKind = 'method'
            if (-not $declaration.Success) {
                $declaration = $memberFieldPattern.Match($line)
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
            $declaredMember = [string]$declaration.Groups['member'].Value
            foreach ($request in @($pending | Where-Object {
                [string]$_.memberName -ceq $declaredMember
            })) {
                [void]$resolved.Add([pscustomobject][ordered]@{
                    category = [string]$request.category
                    surface = [string]$request.surface
                    declaringType = $currentType
                    memberName = $declaredMember
                    probeKind = $probeKind
                    declarationLine = $lineNumber
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
                declarationLine = $signatureLine
                signatureMethodName =
                    [string]$signatureMatch.Groups['methodName'].Value
            })
        }
    }

    $finalRows = New-Object Collections.Generic.List[object]
    $probeIndex = 0
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
            throw "Runtime member probe $($request.surface) resolved to $($matches.Count) declarations."
        }
        $match = $matches[0]
        $probeClass = 'PartisanReleaseSurfaceMemberProbe' +
            $probeIndex.ToString('D3')
        $probeIndex++
        $probeSource = ''
        switch ([string]$match.probeKind) {
            'method' {
                $probeSource = "class $probeClass : $($match.declaringType) { " +
                    "void Bind() { GetGame().GetCallqueue().Remove($($match.memberName)); } }"
            }
            'constant' {
                $probeSource = "class $probeClass : $($match.declaringType) { " +
                    "void Bind() { if ($($match.memberName) == $($match.memberName)) return; } }"
            }
            'field' {
            }
            'methodSignature' {
                if ($null -eq $match.PSObject.Properties[
                        'signatureMethodName']) {
                    throw 'A guarded signature probe lacks its candidate method binding.'
                }
                $probeSource = "class $probeClass : $($match.declaringType) { " +
                    "void Bind() { $($match.signatureMethodName)(`"`", `"`", true); } }"
            }
            default {
                throw 'A runtime member probe has an unsupported kind.'
            }
        }
        [void]$finalRows.Add([pscustomobject][ordered]@{
            category = [string]$match.category
            surface = [string]$match.surface
            declaringType = [string]$match.declaringType
            memberName = [string]$match.memberName
            probeKind = [string]$match.probeKind
            probeSource = $probeSource
        })
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
        throw 'The runtime member-probe plan is incomplete.'
    }
    return [pscustomobject][ordered]@{
        forbidden = [object[]]$forbidden
        production = [object[]]$production
    }
}

function New-ReleaseSurfaceHarness {
    param(
        [Parameter(Mandatory = $true)][string]$TemplateRoot,
        [Parameter(Mandatory = $true)][string]$DestinationRoot,
        [Parameter(Mandatory = $true)]$Contract,
        [Parameter(Mandatory = $true)]$MemberProbePlan,
        [Parameter(Mandatory = $true)][string]$HarnessGuid,
        [Parameter(Mandatory = $true)][string]$CandidateGuid,
        [Parameter(Mandatory = $true)][string]$RunNonce
    )

    if ($HarnessGuid -cnotmatch '^[0-9A-F]{16}$' -or
        $CandidateGuid -cnotmatch '^[0-9A-F]{16}$' -or
        $HarnessGuid -ceq $CandidateGuid -or
        $RunNonce -cnotmatch '^[0-9a-f]{32}$' -or
        (Test-Path -LiteralPath $DestinationRoot)) {
        throw 'Release-surface harness identity or destination is invalid.'
    }
    $projectTemplate = Resolve-ReleaseSurfacePath `
        -Path (Join-Path $TemplateRoot 'addon.gproj.template') `
        -Kind Leaf
    $sourceTemplate = Resolve-ReleaseSurfacePath `
        -Path (Join-Path $TemplateRoot `
            'Scripts\Game\PartisanReleaseSurfaceAudit.c.template') `
        -Kind Leaf
    $projectText = [IO.File]::ReadAllText($projectTemplate).
        Replace('__HARNESS_ID__', $script:HarnessId).
        Replace('__HARNESS_GUID__', $HarnessGuid).
        Replace('__CANDIDATE_GUID__', $CandidateGuid)
    $sourceText = [IO.File]::ReadAllText($sourceTemplate).
        Replace(
            '__FORBIDDEN_TYPE_INSERTS__',
            (New-ReleaseSurfaceInsertBlock `
                -Values @($Contract.forbiddenTypeNames) `
                -Variable 'forbiddenTypes')).
        Replace(
            '__PRODUCTION_TYPE_INSERTS__',
             (New-ReleaseSurfaceInsertBlock `
                 -Values @($Contract.productionPositiveControlTypeNames) `
                 -Variable 'productionTypes')).
        Replace(
            '__FORBIDDEN_MEMBER_SURFACE_INSERTS__',
            (New-ReleaseSurfaceInsertBlock `
                -Values @($MemberProbePlan.forbidden.surface) `
                -Variable 'forbiddenMemberSurfaces')).
        Replace(
            '__FORBIDDEN_MEMBER_DECLARING_TYPE_INSERTS__',
            (New-ReleaseSurfaceInsertBlock `
                -Values @($MemberProbePlan.forbidden.declaringType) `
                -Variable 'forbiddenMemberDeclaringTypes')).
        Replace(
            '__FORBIDDEN_MEMBER_NAME_INSERTS__',
            (New-ReleaseSurfaceInsertBlock `
                -Values @($MemberProbePlan.forbidden.memberName) `
                -Variable 'forbiddenMemberNames')).
        Replace(
            '__FORBIDDEN_MEMBER_PROBE_KIND_INSERTS__',
            (New-ReleaseSurfaceInsertBlock `
                -Values @($MemberProbePlan.forbidden.probeKind) `
                -Variable 'forbiddenMemberProbeKinds')).
        Replace(
            '__FORBIDDEN_MEMBER_PROBE_SOURCE_INSERTS__',
            (New-ReleaseSurfaceInsertBlock `
                -Values @($MemberProbePlan.forbidden.probeSource) `
                -Variable 'forbiddenMemberProbeSources')).
        Replace(
            '__PRODUCTION_MEMBER_SURFACE_INSERTS__',
            (New-ReleaseSurfaceInsertBlock `
                -Values @($MemberProbePlan.production.surface) `
                -Variable 'productionMemberSurfaces')).
        Replace(
            '__PRODUCTION_MEMBER_DECLARING_TYPE_INSERTS__',
            (New-ReleaseSurfaceInsertBlock `
                -Values @($MemberProbePlan.production.declaringType) `
                -Variable 'productionMemberDeclaringTypes')).
        Replace(
            '__PRODUCTION_MEMBER_NAME_INSERTS__',
            (New-ReleaseSurfaceInsertBlock `
                -Values @($MemberProbePlan.production.memberName) `
                -Variable 'productionMemberNames')).
        Replace(
            '__PRODUCTION_MEMBER_PROBE_KIND_INSERTS__',
            (New-ReleaseSurfaceInsertBlock `
                -Values @($MemberProbePlan.production.probeKind) `
                -Variable 'productionMemberProbeKinds')).
        Replace(
            '__PRODUCTION_MEMBER_PROBE_SOURCE_INSERTS__',
            (New-ReleaseSurfaceInsertBlock `
                -Values @($MemberProbePlan.production.probeSource) `
                -Variable 'productionMemberProbeSources')).
        Replace(
            '__FORBIDDEN_COMMAND_INSERTS__',
            (New-ReleaseSurfaceInsertBlock `
                -Values @($Contract.forbiddenCommandActionIds) `
                -Variable 'forbiddenCommands')).
        Replace(
            '__PRODUCTION_COMMAND_INSERTS__',
            (New-ReleaseSurfaceInsertBlock `
                -Values @($Contract.productionPositiveControlCommandActionIds) `
                -Variable 'productionCommands'))
    if ($projectText -match '__[A-Z0-9_]+__' -or
        $sourceText -match '__[A-Z0-9_]+__') {
        throw 'Release-surface harness rendering left an unresolved placeholder.'
    }
    New-Item -ItemType Directory -Path $DestinationRoot -ErrorAction Stop |
        Out-Null
    [void](Write-ReleaseSurfaceText `
        -Path (Join-Path $DestinationRoot 'addon.gproj') `
        -Text $projectText `
        -CreateOnly)
    [void](Write-ReleaseSurfaceText `
        -Path (Join-Path $DestinationRoot `
            'Scripts\Game\PartisanReleaseSurfaceAudit.c') `
        -Text $sourceText `
        -CreateOnly)
    [void](Write-ReleaseSurfaceJson `
        -Path (Join-Path $DestinationRoot $script:HarnessOwnerLeaf) `
        -Value ([ordered]@{
            schemaVersion = 1
            evidenceKind = 'partisan_release_surface_harness_owner_v1'
            runNonce = $RunNonce
            harnessId = $script:HarnessId
            harnessGuid = $HarnessGuid
            candidateGuid = $CandidateGuid
        }) `
        -Portable `
        -CreateOnly)
    return Get-ReleaseSurfaceTreeBinding -Root $DestinationRoot
}

function Copy-ReleaseSurfaceTree {
    param(
        [Parameter(Mandatory = $true)][string]$SourceRoot,
        [Parameter(Mandatory = $true)][string]$DestinationRoot
    )

    if (Test-Path -LiteralPath $DestinationRoot) {
        throw 'Release-surface harness install destination is not fresh.'
    }
    New-Item -ItemType Directory -Path $DestinationRoot -ErrorAction Stop |
        Out-Null
    $sourcePath = [IO.Path]::GetFullPath($SourceRoot)
    foreach ($directory in @(Get-ChildItem -LiteralPath $sourcePath -Recurse `
            -Directory -Force | Sort-Object FullName)) {
        $relative = ConvertTo-ReleaseSurfaceRelativePath `
            -Root $sourcePath `
            -Path $directory.FullName
        New-Item -ItemType Directory `
            -Path (Join-Path $DestinationRoot $relative) `
            -Force -ErrorAction Stop | Out-Null
    }
    foreach ($file in @(Get-ChildItem -LiteralPath $sourcePath -Recurse `
            -File -Force | Sort-Object FullName)) {
        $relative = ConvertTo-ReleaseSurfaceRelativePath `
            -Root $sourcePath `
            -Path $file.FullName
        Copy-Item -LiteralPath $file.FullName `
            -Destination (Join-Path $DestinationRoot $relative) `
            -ErrorAction Stop
    }
}

function Remove-ReleaseSurfaceHarnessExact {
    param(
        [Parameter(Mandatory = $true)][string]$RuntimeAddonRoot,
        [Parameter(Mandatory = $true)][string]$HarnessRoot,
        [Parameter(Mandatory = $true)]$ExpectedBinding,
        [Parameter(Mandatory = $true)][string]$RunNonce,
        [Parameter(Mandatory = $true)][string]$HarnessGuid
    )

    $runtimePath = [IO.Path]::GetFullPath($RuntimeAddonRoot).TrimEnd('\', '/')
    $harnessPath = [IO.Path]::GetFullPath($HarnessRoot)
    if (-not $harnessPath.StartsWith(
            $runtimePath + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase) -or
        (Split-Path -Leaf $harnessPath) -cnotlike
            ($script:HarnessPrefix + '*')) {
        throw 'Release-surface harness cleanup target escaped its exact root.'
    }
    [void](Assert-ReleaseSurfaceTreeBinding `
        -Expected $ExpectedBinding `
        -Root $harnessPath `
        -Label 'Installed release-surface harness')
    $ownerPath = Join-Path $harnessPath $script:HarnessOwnerLeaf
    $owner = Get-Content -LiteralPath $ownerPath -Raw | ConvertFrom-Json
    if ([int]$owner.schemaVersion -ne 1 -or
        [string]$owner.evidenceKind -cne
            'partisan_release_surface_harness_owner_v1' -or
        [string]$owner.runNonce -cne $RunNonce -or
        [string]$owner.harnessGuid -cne $HarnessGuid) {
        throw 'Release-surface harness cleanup ownership is invalid.'
    }
    Remove-Item -LiteralPath $harnessPath -Recurse -Force -ErrorAction Stop
    if (Test-Path -LiteralPath $harnessPath) {
        throw 'Release-surface harness cleanup left its exact target behind.'
    }
}

function Assert-ReleaseSurfaceMemberProbeRows {
    param(
        [Parameter(Mandatory = $true)][object[]]$Rows,
        [Parameter(Mandatory = $true)][object[]]$ExpectedRows,
        [Parameter(Mandatory = $true)][bool]$ExpectedPresence,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($Rows.Count -ne $ExpectedRows.Count) {
        throw "$Label row count differs from the contract."
    }
    for ($index = 0; $index -lt $ExpectedRows.Count; $index++) {
        $row = $Rows[$index]
        $expected = $ExpectedRows[$index]
        $expectedProperties = @(
            'surface', 'declaringType', 'memberName', 'probeKind',
            'probeSupported', 'present')
        $actualProperties = @($row.PSObject.Properties | ForEach-Object {
            [string]$_.Name
        })
        if (@(Compare-Object -ReferenceObject $expectedProperties `
                -DifferenceObject $actualProperties -CaseSensitive).Count -ne 0 -or
            $row.surface -isnot [string] -or
            $row.declaringType -isnot [string] -or
            $row.memberName -isnot [string] -or
            $row.probeKind -isnot [string] -or
            $row.probeSupported -isnot [bool] -or
            $row.present -isnot [bool]) {
            throw "$Label row has an unexpected schema or type."
        }
        foreach ($property in @(
                'surface', 'declaringType', 'memberName', 'probeKind')) {
            if ([string]$row.$property -cne [string]$expected.$property) {
                throw "$Label row identity differs at $property."
            }
        }
        if (-not [bool]$row.probeSupported -or
            [bool]$row.present -ne $ExpectedPresence) {
            throw "$Label row has an unsupported or unexpected result."
        }
    }
}

function Assert-ReleaseSurfaceProbeResult {
    param(
        [Parameter(Mandatory = $true)]$Result,
        [Parameter(Mandatory = $true)][ValidateSet('retail', 'diagnostic')]
        [string]$Mode,
        [Parameter(Mandatory = $true)]$Contract,
        [Parameter(Mandatory = $true)]$MemberProbePlan,
        [Parameter(Mandatory = $true)][string]$RunNonce,
        [Parameter(Mandatory = $true)]$Candidate
    )

    $requiredProperties = @(
        'schemaVersion',
        'evidenceKind',
        'runNonce',
        'mode',
        'expectedMode',
        'candidateId',
        'packageSha256',
        'manifestSha256',
        'cliIdentityPresent',
        'modeSentinelExact',
        'retailSentinelPresent',
         'diagnosticSentinelPresent',
         'forbiddenTypeExpectedPresent',
        'runtimeCompilerAlwaysPublicCompiled',
        'runtimeCompilerAlwaysProtectedCompiled',
        'runtimeCompilerImpossibleMemberRejected',
        'runtimeCompilerDiagnosticPublicCompiled',
        'runtimeCompilerDiagnosticProtectedCompiled',
        'runtimeMetadataAlwaysFieldPresent',
        'runtimeMetadataDiagnosticFieldPresent',
        'runtimeCompilerAvailable',
        'runtimeMetadataAvailable',
         'forbiddenTypes',
         'productionTypes',
        'forbiddenMemberExpectedPresent',
        'forbiddenMembers',
        'productionMembers',
        'forbiddenCommandExpectedPresent',
        'forbiddenCommands',
        'productionCommands',
        'mismatchCount',
        'passed')
    $actualProperties = @($Result.PSObject.Properties |
        ForEach-Object { [string]$_.Name })
    if (@(Compare-Object -ReferenceObject $requiredProperties `
            -DifferenceObject $actualProperties -CaseSensitive).Count -ne 0) {
        throw "$Mode probe result has an unexpected schema."
    }
    $expectForbidden = $Mode -ceq 'diagnostic'
    if ([int]$Result.schemaVersion -ne 1 -or
        [string]$Result.evidenceKind -cne $script:ProbeEvidenceKind -or
        [string]$Result.runNonce -cne $RunNonce -or
        [string]$Result.mode -cne $Mode -or
        [string]$Result.expectedMode -cne $Mode -or
        [string]$Result.candidateId -cne [string]$Candidate.CandidateId -or
        [string]$Result.packageSha256 -cne
            [string]$Candidate.PackageSha256 -or
        [string]$Result.manifestSha256 -cne
            [string]$Candidate.ManifestSha256 -or
        -not [bool]$Result.cliIdentityPresent -or
        -not [bool]$Result.modeSentinelExact -or
        [bool]$Result.retailSentinelPresent -ne ($Mode -ceq 'retail') -or
        [bool]$Result.diagnosticSentinelPresent -ne
            ($Mode -ceq 'diagnostic') -or
         [bool]$Result.forbiddenTypeExpectedPresent -ne $expectForbidden -or
        $Result.runtimeCompilerAlwaysPublicCompiled -isnot [bool] -or
        $Result.runtimeCompilerAlwaysProtectedCompiled -isnot [bool] -or
        $Result.runtimeCompilerImpossibleMemberRejected -isnot [bool] -or
        $Result.runtimeCompilerDiagnosticPublicCompiled -isnot [bool] -or
        $Result.runtimeCompilerDiagnosticProtectedCompiled -isnot [bool] -or
        $Result.runtimeMetadataAlwaysFieldPresent -isnot [bool] -or
        $Result.runtimeMetadataDiagnosticFieldPresent -isnot [bool] -or
        $Result.runtimeCompilerAvailable -isnot [bool] -or
        $Result.runtimeMetadataAvailable -isnot [bool] -or
        -not [bool]$Result.runtimeCompilerAlwaysPublicCompiled -or
        -not [bool]$Result.runtimeCompilerAlwaysProtectedCompiled -or
        -not [bool]$Result.runtimeCompilerImpossibleMemberRejected -or
        [bool]$Result.runtimeCompilerDiagnosticPublicCompiled -ne
            $expectForbidden -or
        [bool]$Result.runtimeCompilerDiagnosticProtectedCompiled -ne
            $expectForbidden -or
        -not [bool]$Result.runtimeMetadataAlwaysFieldPresent -or
        [bool]$Result.runtimeMetadataDiagnosticFieldPresent -ne
            $expectForbidden -or
        -not [bool]$Result.runtimeCompilerAvailable -or
        -not [bool]$Result.runtimeMetadataAvailable -or
        [bool]$Result.forbiddenMemberExpectedPresent -ne $expectForbidden -or
         [bool]$Result.forbiddenCommandExpectedPresent -ne $expectForbidden -or
        [int]$Result.mismatchCount -ne 0 -or
        -not [bool]$Result.passed) {
        throw "$Mode probe result failed its fixed identity or disposition."
    }

    Assert-ReleaseSurfaceMemberProbeRows `
        -Rows @($Result.forbiddenMembers) `
        -ExpectedRows @($MemberProbePlan.forbidden) `
        -ExpectedPresence $expectForbidden `
        -Label "$Mode forbidden member"
    Assert-ReleaseSurfaceMemberProbeRows `
        -Rows @($Result.productionMembers) `
        -ExpectedRows @($MemberProbePlan.production) `
        -ExpectedPresence $true `
        -Label "$Mode production member"

    foreach ($set in @(
            [pscustomobject]@{
                Label = 'forbidden type'
                Rows = @($Result.forbiddenTypes)
                 Key = 'name'
                 ExpectedNames = @($Contract.forbiddenTypeNames)
                 ExpectedPresence = $expectForbidden
                 PresenceProperties = @('present')
             },
            [pscustomobject]@{
                Label = 'production type'
                Rows = @($Result.productionTypes)
                 Key = 'name'
                 ExpectedNames = @($Contract.productionPositiveControlTypeNames)
                 ExpectedPresence = $true
                 PresenceProperties = @('present')
             },
            [pscustomobject]@{
                Label = 'forbidden command'
                Rows = @($Result.forbiddenCommands)
                 Key = 'id'
                 ExpectedNames = @($Contract.forbiddenCommandActionIds)
                 ExpectedPresence = $expectForbidden
                 PresenceProperties = @(
                     'generatedPresent',
                     'routingPresent')
             },
            [pscustomobject]@{
                Label = 'production command'
                Rows = @($Result.productionCommands)
                 Key = 'id'
                 ExpectedNames = @($Contract.productionPositiveControlCommandActionIds)
                 ExpectedPresence = $true
                 PresenceProperties = @(
                     'generatedPresent',
                     'routingPresent')
             })) {
        $actualNames = @($set.Rows | ForEach-Object {
            [string]$_.$($set.Key)
        })
        if ($actualNames.Count -ne @($set.ExpectedNames).Count -or
            @($actualNames | Sort-Object -Unique).Count -ne
                $actualNames.Count -or
            @(Compare-Object `
                -ReferenceObject @($set.ExpectedNames | Sort-Object) `
                -DifferenceObject @($actualNames | Sort-Object) `
                -CaseSensitive).Count -ne 0) {
            throw "$Mode $($set.Label) result names differ from the contract."
         }
         foreach ($row in $set.Rows) {
             $expectedRowProperties = @(
                 [string]$set.Key) + @($set.PresenceProperties)
             $actualRowProperties = @($row.PSObject.Properties |
                 ForEach-Object { [string]$_.Name })
             if (@(Compare-Object `
                     -ReferenceObject $expectedRowProperties `
                     -DifferenceObject $actualRowProperties `
                     -CaseSensitive).Count -ne 0) {
                 throw "$Mode $($set.Label) row has an unexpected schema."
             }
             foreach ($presenceProperty in @($set.PresenceProperties)) {
                 if ([bool]$row.$presenceProperty -ne
                     [bool]$set.ExpectedPresence) {
                     throw "$Mode $($set.Label) result has an unexpected $presenceProperty value."
                 }
             }
         }
    }
    return [pscustomobject][ordered]@{
        mode = $Mode
        forbiddenTypeCount = @($Result.forbiddenTypes).Count
        productionTypeCount = @($Result.productionTypes).Count
        forbiddenCommandCount = @($Result.forbiddenCommands).Count
        productionCommandCount = @($Result.productionCommands).Count
        forbiddenMemberCount = @($Result.forbiddenMembers).Count
        productionMemberCount = @($Result.productionMembers).Count
        runtimeCompilerAvailable = [bool]$Result.runtimeCompilerAvailable
        runtimeMetadataAvailable = [bool]$Result.runtimeMetadataAvailable
        passed = $true
    }
}

function Get-ReleaseSurfaceCandidateMountAttestation {
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

function Get-ReleaseSurfaceLogClassification {
    param(
        [Parameter(Mandatory = $true)][string]$LogRoot,
        [Parameter(Mandatory = $true)][string]$Mode,
        [Parameter(Mandatory = $true)][string]$CandidateGuid,
        [Parameter(Mandatory = $true)][string]$CandidateProjectPath,
        [Parameter(Mandatory = $true)][string]$HarnessGuid
    )

    $logRows = New-Object Collections.Generic.List[object]
    $texts = New-Object Collections.Generic.List[string]
    $textByLeaf = @{}
    $actualLogFiles = @(Get-ChildItem -LiteralPath $LogRoot -Recurse -File `
        -Force -ErrorAction Stop)
    $unknownLogFiles = @($actualLogFiles | Where-Object {
        $_.Extension -ine '.log' -or
        $_.Name -cnotin $script:AllowedLogLeaves
    })
    if ($unknownLogFiles.Count -ne 0) {
        throw "$Mode runtime produced unknown log leaves."
    }
    foreach ($leaf in $script:RequiredLogLeaves) {
        $matches = @($actualLogFiles | Where-Object { $_.Name -ceq $leaf })
        if ($matches.Count -ne 1) {
            throw "$Mode runtime produced $($matches.Count) required $leaf files."
        }
        $signature = Get-ReleaseSurfaceFileSignature -Path $matches[0].FullName
        [void]$logRows.Add([pscustomobject][ordered]@{
            leaf = $leaf
            path = $matches[0].FullName
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        })
        $text = [IO.File]::ReadAllText($matches[0].FullName)
        [void]$texts.Add($text)
        $textByLeaf[$leaf] = $text
    }
    foreach ($leaf in $script:OptionalLogLeaves) {
        $matches = @($actualLogFiles | Where-Object { $_.Name -ceq $leaf })
        if ($matches.Count -gt 1) {
            throw "$Mode runtime produced $($matches.Count) optional $leaf files."
        }
        if ($matches.Count -eq 1) {
            $signature = Get-ReleaseSurfaceFileSignature -Path $matches[0].FullName
            [void]$logRows.Add([pscustomobject][ordered]@{
                leaf = $leaf
                path = $matches[0].FullName
                length = [long]$signature.length
                sha256 = [string]$signature.sha256
            })
            $text = [IO.File]::ReadAllText($matches[0].FullName)
            [void]$texts.Add($text)
            $textByLeaf[$leaf] = $text
        }
    }
    $allLines = @($texts.ToArray() -split "`r?`n")
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
    foreach ($leaf in @($textByLeaf.Keys)) {
        $leafLines = @(([string]$textByLeaf[$leaf]) -split "`r?`n")
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
    $hardLines = @($hardRows | ForEach-Object { [string]$_.line })
    $hardEventCount = 0
    foreach ($eventGroup in @($hardRows |
            Group-Object -Property line -CaseSensitive)) {
        $leafMaximum = 0
        foreach ($leafGroup in @($eventGroup.Group | Group-Object -Property leaf)) {
            $leafMaximum = [Math]::Max($leafMaximum, $leafGroup.Count)
        }
        $hardEventCount += $leafMaximum
    }
    $stockHeaderRows = @($hardRows | Where-Object {
        -not [string]::IsNullOrEmpty([string]$_.stockTimestamp)
    })
    $stockRows = @($hardRows | Where-Object { [bool]$_.stockCandidate })
    $stockEventGroups = @($stockRows | Group-Object -Property stockTimestamp)
    $candidateMountAttestation =
        Get-ReleaseSurfaceCandidateMountAttestation `
            -ConsoleText ([string]$textByLeaf['console.log']) `
            -CandidateGuid $CandidateGuid `
            -CandidateProjectPath $CandidateProjectPath
    $harnessLines = @($allLines | Where-Object {
        ([string]$_).IndexOf(
            $HarnessGuid,
            [StringComparison]::OrdinalIgnoreCase) -ge 0
    })
    $resultPattern = '^.*Partisan release surface audit \| RESULT \| mode=' +
        [regex]::Escape($Mode) + ' \| passed=(?:1|true) \| mismatches=0\s*$'
    $resultMarkers = @($allLines | Where-Object {
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
        $stockRows.Count -eq 6 -and
        $stockEventGroups.Count -eq 2) {
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
    $crashArtifacts = @(Get-ChildItem -LiteralPath (Split-Path -Parent $LogRoot) `
        -Recurse -File -Force -ErrorAction Stop | Where-Object {
            $_.Extension -in @('.dmp', '.mdmp') -or
            $_.Name -match '(?i)^minidump'
        })
    $crashLogContentValid = -not $textByLeaf.ContainsKey('crash.log') -or
        [string]::IsNullOrWhiteSpace([string]$textByLeaf['crash.log'])
    $valid = $hardAccountingExact -and
        $clusterShapeExact -and
        $clusterLifecycleExact -and
        $unapprovedHardRawCount -eq 0 -and
        $unapprovedHardEventCount -eq 0 -and
        [bool]$candidateMountAttestation.valid -and
        $harnessLines.Count -gt 0 -and
        $resultMarkers.Count -eq 1 -and
        $crashLogContentValid -and
        $crashArtifacts.Count -eq 0
    return [pscustomobject][ordered]@{
        valid = [bool]$valid
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
        candidateMountAttestation = $candidateMountAttestation
        harnessMountLineCount = $harnessLines.Count
        uniqueResultMarkerCount = $resultMarkers.Count
        resultMarkerOccurrenceCount = $resultRows.Count
        crashLogContentValid = [bool]$crashLogContentValid
        crashArtifactCount = $crashArtifacts.Count
        logs = [object[]]$logRows.ToArray()
    }
}

function Start-ReleaseSurfaceGuardedServer {
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter(Mandatory = $true)]$Provenance,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][string]$TempDirectory,
        [Parameter(Mandatory = $true)]$CandidateStage
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
        return Start-PartisanGuardedServer `
            -Context $Context `
            -Executable $Executable `
            -ExecutableProvenance $Provenance `
            -Arguments $Arguments `
            -WorkingDirectory $WorkingDirectory `
            -CandidateConsumption $CandidateStage
    }
    finally {
        [Environment]::SetEnvironmentVariable(
            'TEMP', $oldTemp, [EnvironmentVariableTarget]::Process)
        [Environment]::SetEnvironmentVariable(
            'TMP', $oldTmp, [EnvironmentVariableTarget]::Process)
    }
}

function Invoke-ReleaseSurfaceMode {
    param(
        [Parameter(Mandatory = $true)][ValidateSet('retail', 'diagnostic')]
        [string]$Mode,
        [Parameter(Mandatory = $true)][string]$RunRoot,
        [Parameter(Mandatory = $true)][string]$RunNonce,
        [Parameter(Mandatory = $true)]$Candidate,
        [Parameter(Mandatory = $true)][string]$ManifestPath,
        [Parameter(Mandatory = $true)][string]$BundleRoot,
        [Parameter(Mandatory = $true)][string]$RuntimeAddonRoot,
        [Parameter(Mandatory = $true)][string]$DiagnosticExecutable,
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter(Mandatory = $true)]$ExecutableProvenance,
        [Parameter(Mandatory = $true)]$Contract,
        [Parameter(Mandatory = $true)]$MemberProbePlan,
        [Parameter(Mandatory = $true)][string]$HarnessGuid,
        [Parameter(Mandatory = $true)][string]$HarnessRoot,
        [Parameter(Mandatory = $true)]$HarnessBinding,
        [Parameter(Mandatory = $true)][string[]]$WatchedRoots,
        [Parameter(Mandatory = $true)][string[]]$SpillRoots,
        [Parameter(Mandatory = $true)][int]$LoopbackPort,
        [Parameter(Mandatory = $true)][int]$TimeoutSeconds,
        [Parameter(Mandatory = $true)][int]$PollMilliseconds,
        [Parameter(Mandatory = $true)][string]$WorldResource,
        [Parameter(Mandatory = $true)][string]$MissionHeader
    )

    $modeRoot = Join-Path (Join-Path $RunRoot 'raw') $Mode
    $guardBase = Join-Path $modeRoot 'guarded-runtime'
    $working = Join-Path $modeRoot 'working'
    $temporary = Join-Path $modeRoot 'process-temp'
    $profile = Join-Path $modeRoot 'profile'
    $logs = Join-Path $modeRoot 'logs'
    $addonTemp = Join-Path $modeRoot 'addon-temp'
    foreach ($directory in @(
            $modeRoot,
            $guardBase,
            $working,
            $temporary,
            $profile,
            $logs,
            $addonTemp)) {
        New-Item -ItemType Directory -Path $directory -Force -ErrorAction Stop |
            Out-Null
        Assert-PartisanNoReparseAncestry -Path $directory
    }
    $stdoutPath = Join-Path $modeRoot 'stdout.raw.txt'
    $stderrPath = Join-Path $modeRoot 'stderr.raw.txt'
    [IO.File]::WriteAllBytes($stdoutPath, [byte[]]@())
    [IO.File]::WriteAllBytes($stderrPath, [byte[]]@())
    [void](Write-ReleaseSurfaceJson `
        -Path (Join-Path $modeRoot 'stream-capture.json') `
        -Value ([ordered]@{
            schemaVersion = 1
            mode = $Mode
            captureMode = 'guarded-native-no-inherited-handles'
            stdoutPath = 'stdout.raw.txt'
            stdoutBytes = 0
            stderrPath = 'stderr.raw.txt'
            stderrBytes = 0
            engineLogsAreAuthoritative = $true
        }) `
        -Portable `
        -CreateOnly)

    $context = $null
    $candidateStage = $null
    $teardown = $null
    $receipt = $null
    $launch = $null
    $arguments = $null
    try {
        $context = New-PartisanGuardedRuntimeContext `
            -GuardBase $guardBase `
            -Purpose ('release_surface_' + $Mode) `
            -WatchedRoots $WatchedRoots `
            -SpillRoots $SpillRoots `
            -LoopbackPorts @($LoopbackPort)
        $candidateStage = New-PartisanCandidateStage `
            -Context $context `
            -Candidate $Candidate
        $addons = [string]$Candidate.AddonGuid + ',' + $HarnessGuid
        $arguments = [string[]]@(
            '-gproj', [string]$candidateStage.PackedProjectPath,
            '-server', $WorldResource,
            '-MissionHeader', $MissionHeader,
            '-addonsDir', [string]$candidateStage.AddonSearchPath,
            '-addons', $addons,
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
            '-hstReleaseCandidateId', [string]$Candidate.CandidateId,
            '-hstReleasePackageSha256', [string]$Candidate.PackageSha256,
            '-hstReleaseManifestSha256', [string]$Candidate.ManifestSha256)
        Assert-ReleaseSurfaceArguments -Mode $Mode -Arguments $arguments
        $rawArgumentSignature = Write-ReleaseSurfaceJson `
            -Path (Join-Path $modeRoot 'arguments.raw.json') `
            -Value ([ordered]@{
                schemaVersion = 1
                mode = $Mode
                executable = $Executable
                arguments = $arguments
            }) `
            -CreateOnly
        $portableArguments = ConvertTo-ReleaseSurfacePortableArguments `
            -Arguments $arguments `
            -Replacements @(
                [pscustomobject]@{
                    path = [string]$candidateStage.PackedProjectPath
                    token = '<candidate-stage>/Partisan/addon.gproj'
                },
                [pscustomobject]@{
                    path = [string]$candidateStage.StageRootPath
                    token = '<candidate-stage>'
                },
                [pscustomobject]@{
                    path = $RuntimeAddonRoot
                    token = '<runtime-addons>'
                },
                [pscustomobject]@{
                    path = $RunRoot
                    token = '<run>'
                })
        [void](Write-ReleaseSurfaceJson `
            -Path (Join-Path $modeRoot 'arguments.portable.json') `
            -Value ([ordered]@{
                schemaVersion = 1
                mode = $Mode
                executable = '<runtime>/' + (Split-Path -Leaf $Executable)
                arguments = $portableArguments
                rawArgumentsSha256 = [string]$rawArgumentSignature.sha256
            }) `
            -Portable `
            -CreateOnly)

        [void](Assert-PartisanCandidateStage `
            -Context $context `
            -Stage $candidateStage)
        [void](Assert-ReleaseSurfaceTreeBinding `
            -Expected $HarnessBinding `
            -Root $HarnessRoot `
            -Label "$Mode installed harness prelaunch")
        $launch = Start-ReleaseSurfaceGuardedServer `
            -Context $context `
            -Executable $Executable `
            -Provenance $ExecutableProvenance `
            -Arguments $arguments `
            -WorkingDirectory $working `
            -TempDirectory $temporary `
            -CandidateStage $candidateStage
        $wait = Wait-PartisanGuardedProcess `
            -Context $context `
            -Launch $launch `
            -TimeoutSeconds $TimeoutSeconds `
            -PollMilliseconds $PollMilliseconds `
            -RequireZeroExit
        if ([int]$wait.ExitCode -ne 0) {
            throw "$Mode runtime returned a nonzero exit code."
        }
        [void](Assert-PartisanRuntimeOwnershipAudit -Context $context)
        [void](Assert-PartisanCandidateStage `
            -Context $context `
            -Stage $candidateStage)
        [void](Assert-ReleaseSurfaceTreeBinding `
            -Expected $HarnessBinding `
            -Root $HarnessRoot `
            -Label "$Mode installed harness postprocess")
        $teardown = Invoke-PartisanGuardedTeardown -Context $context
        $receipt = Test-PartisanGuardedRuntimeReceipt `
            -Path $teardown.CleanReceiptPath `
            -ExpectedSignature $teardown.CleanReceiptSignature
        if (-not [bool]$receipt.Complete -or
            [string]$receipt.Status -cne 'complete') {
            throw "$Mode guarded runtime receipt is incomplete."
        }
    }
    catch {
        $failure = $_
        if ($context -and -not $teardown) {
            try {
                $null = Invoke-PartisanGuardedTeardown -Context $context
            }
            catch {
            }
        }
        throw $failure
    }

    $postCandidate = Assert-PartisanReleaseCandidate `
        -ManifestPath $ManifestPath `
        -BundleRoot $BundleRoot `
        -RuntimeAddonRoot $RuntimeAddonRoot `
        -Executable $DiagnosticExecutable `
        -RuntimeRole server `
        -ConsumerIntent runtime
    Assert-ReleaseSurfaceCandidateIdentity `
        -Expected $Candidate `
        -Actual $postCandidate
    $postExecutable = Get-PartisanExecutableProvenance -Path $Executable
    Assert-ReleaseSurfaceExecutableIdentity `
        -Expected $ExecutableProvenance `
        -Actual $postExecutable `
        -Label $Mode
    [void](Assert-ReleaseSurfaceTreeBinding `
        -Expected $HarnessBinding `
        -Root $HarnessRoot `
        -Label "$Mode installed harness final")

    $resultFiles = @(Get-ChildItem -LiteralPath $profile -Recurse -File `
        -Filter 'probe.result.json' -Force -ErrorAction Stop)
    if ($resultFiles.Count -ne 1) {
        throw "$Mode runtime produced $($resultFiles.Count) probe results."
    }
    $result = [IO.File]::ReadAllText($resultFiles[0].FullName) |
        ConvertFrom-Json
    $probeSummary = Assert-ReleaseSurfaceProbeResult `
        -Result $result `
        -Mode $Mode `
        -Contract $Contract `
        -MemberProbePlan $MemberProbePlan `
        -RunNonce $RunNonce `
        -Candidate $Candidate
    $classification = Get-ReleaseSurfaceLogClassification `
        -LogRoot $logs `
        -Mode $Mode `
        -CandidateGuid ([string]$Candidate.AddonGuid) `
        -CandidateProjectPath ([string]$candidateStage.PackedProjectPath) `
        -HarnessGuid $HarnessGuid
    if (-not [bool]$classification.valid) {
        throw "$Mode runtime logs failed closed classification."
    }
    $portableLogRows = @($classification.logs | ForEach-Object {
        [pscustomobject][ordered]@{
            leaf = [string]$_.leaf
            path = ConvertTo-ReleaseSurfaceRelativePath `
                -Root $RunRoot `
                -Path ([string]$_.path)
            length = [long]$_.length
            sha256 = [string]$_.sha256
        }
    })
    $modeValue = [ordered]@{
        schemaVersion = 2
        evidenceKind = $script:EvidenceKind
        mode = $Mode
        disposition = 'passed-noncertifying-release-surface-audit'
        candidateId = [string]$Candidate.CandidateId
        packageSha256 = [string]$Candidate.PackageSha256
        manifestSha256 = [string]$Candidate.ManifestSha256
        readySha256 = [string]$Candidate.ReadySha256
        executable = $postExecutable
        harnessGuid = $HarnessGuid
        harnessSha256 = [string]$HarnessBinding.aggregateSha256
        process = [ordered]@{
            exitCode = 0
            contextId = [string]$receipt.ContextId
            candidateBindingSha256 = [string]$receipt.CandidateBindingSha256
            receiptPath = ConvertTo-ReleaseSurfaceRelativePath `
                -Root $RunRoot `
                -Path ([string]$teardown.CleanReceiptPath)
            receiptSignature = $teardown.CleanReceiptSignature
            journalPath = ConvertTo-ReleaseSurfaceRelativePath `
                -Root $RunRoot `
                -Path ([string]$receipt.JournalPath)
            journalSignature = $receipt.JournalSignature
            completionPath = ConvertTo-ReleaseSurfaceRelativePath `
                -Root $RunRoot `
                -Path ([string]$receipt.CompletionAttestationPath)
            completionSignature = $receipt.CompletionAttestationSignature
        }
        arguments = [ordered]@{
            raw = ConvertTo-ReleaseSurfaceRelativePath `
                -Root $RunRoot `
                -Path (Join-Path $modeRoot 'arguments.raw.json')
            portable = ConvertTo-ReleaseSurfaceRelativePath `
                -Root $RunRoot `
                -Path (Join-Path $modeRoot 'arguments.portable.json')
        }
        streams = [ordered]@{
            stdout = ConvertTo-ReleaseSurfaceRelativePath `
                -Root $RunRoot `
                -Path $stdoutPath
            stderr = ConvertTo-ReleaseSurfaceRelativePath `
                -Root $RunRoot `
                -Path $stderrPath
            capture = ConvertTo-ReleaseSurfaceRelativePath `
                -Root $RunRoot `
                -Path (Join-Path $modeRoot 'stream-capture.json')
        }
        probe = [ordered]@{
            path = ConvertTo-ReleaseSurfaceRelativePath `
                -Root $RunRoot `
                -Path $resultFiles[0].FullName
            signature = Get-ReleaseSurfaceFileSignature `
                -Path $resultFiles[0].FullName
            summary = $probeSummary
        }
        classification = [ordered]@{
            valid = [bool]$classification.valid
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
            candidateMountAttestation =
                $classification.candidateMountAttestation
            harnessMountLineCount = [int]$classification.harnessMountLineCount
            uniqueResultMarkerCount =
                [int]$classification.uniqueResultMarkerCount
            resultMarkerOccurrenceCount =
                [int]$classification.resultMarkerOccurrenceCount
            crashLogContentValid = [bool]$classification.crashLogContentValid
            crashArtifactCount = [int]$classification.crashArtifactCount
            logs = [object[]]$portableLogRows
        }
        passed = $true
    }
    $modePath = Join-Path (Join-Path $RunRoot 'modes') ($Mode + '.json')
    $modeSignature = Write-ReleaseSurfaceJson `
        -Path $modePath `
        -Value $modeValue `
        -Portable `
        -CreateOnly
    return [pscustomobject][ordered]@{
        mode = $Mode
        path = ConvertTo-ReleaseSurfaceRelativePath `
            -Root $RunRoot `
            -Path $modePath
        signature = $modeSignature
        executable = $postExecutable
        probe = $probeSummary
        receipt = $receipt
    }
}

function Get-ReleaseSurfaceEvidenceIndex {
    param([Parameter(Mandatory = $true)][string]$RunRoot)

    $root = [IO.Path]::GetFullPath($RunRoot).TrimEnd('\', '/')
    $prefix = $root + [IO.Path]::DirectorySeparatorChar
    $excluded = @(
        'evidence-index.json',
        'release-index.json',
        'run.json',
        'run.ready.json',
        'run.failure.json')
    $rows = New-Object Collections.Generic.List[object]
    foreach ($file in @(Get-ChildItem -LiteralPath $root -Recurse -File `
            -Force -ErrorAction Stop | Sort-Object FullName)) {
        $full = [IO.Path]::GetFullPath($file.FullName)
        $relative = $full.Substring($prefix.Length).Replace('\', '/')
        if ($excluded -ccontains $relative) {
            continue
        }
        $signature = Get-ReleaseSurfaceFileSignature -Path $full
        [void]$rows.Add([pscustomobject][ordered]@{
            path = $relative
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        })
    }
    $lines = @($rows.ToArray() | Sort-Object path | ForEach-Object {
        "{0}`t{1}`t{2}" -f $_.sha256, [long]$_.length, $_.path
    })
    return [ordered]@{
        schemaVersion = 1
        evidenceKind = $script:EvidenceKind
        files = [object[]]$rows.ToArray()
        aggregateSha256 = Get-ReleaseSurfaceTextSha256 `
            -Text (($lines -join "`n") + "`n")
    }
}

function New-ReleaseSurfaceSyntheticProbeResult {
    param(
        [ValidateSet('retail', 'diagnostic')][string]$Mode,
        $Contract,
        $MemberProbePlan,
        [string]$RunNonce,
        $Candidate
    )

    $present = $Mode -ceq 'diagnostic'
    return [pscustomobject][ordered]@{
        schemaVersion = 1
        evidenceKind = $script:ProbeEvidenceKind
        runNonce = $RunNonce
        mode = $Mode
        expectedMode = $Mode
        candidateId = [string]$Candidate.CandidateId
        packageSha256 = [string]$Candidate.PackageSha256
        manifestSha256 = [string]$Candidate.ManifestSha256
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
            [pscustomobject][ordered]@{ name = [string]$_; present = $present }
        })
        productionTypes = @(
            $Contract.productionPositiveControlTypeNames | ForEach-Object {
                [pscustomobject][ordered]@{
                    name = [string]$_
                    present = $true
                }
            })
        forbiddenMemberExpectedPresent = $present
        forbiddenMembers = @($MemberProbePlan.forbidden | ForEach-Object {
            [pscustomobject][ordered]@{
                surface = [string]$_.surface
                declaringType = [string]$_.declaringType
                memberName = [string]$_.memberName
                probeKind = [string]$_.probeKind
                probeSupported = $true
                present = $present
            }
        })
        productionMembers = @($MemberProbePlan.production | ForEach-Object {
            [pscustomobject][ordered]@{
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
                 [pscustomobject][ordered]@{
                     id = [string]$_
                     generatedPresent = $present
                     routingPresent = $present
                 }
            })
        productionCommands = @(
            $Contract.productionPositiveControlCommandActionIds |
                ForEach-Object {
                     [pscustomobject][ordered]@{
                         id = [string]$_
                         generatedPresent = $true
                         routingPresent = $true
                     }
                })
        mismatchCount = 0
        passed = $true
    }
}

function Invoke-ReleaseSurfaceSelfTest {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$TemplateRoot,
        [Parameter(Mandatory = $true)]$Contract,
        [Parameter(Mandatory = $true)]$MemberProbePlan
    )

    $selfRoot = Join-Path ([IO.Path]::GetTempPath()) (
        'PartisanReleaseSurfaceRunnerSelfTest_' +
        [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $selfRoot -ErrorAction Stop | Out-Null
    try {
        $nonce = [Guid]::NewGuid().ToString('N')
        $candidate = [pscustomobject]@{
            CandidateId = 'self-test-candidate'
            PackageSha256 = '1' * 64
            ManifestSha256 = '2' * 64
        }
        $binding = New-ReleaseSurfaceHarness `
            -TemplateRoot $TemplateRoot `
            -DestinationRoot (Join-Path $selfRoot 'harness') `
            -Contract $Contract `
            -MemberProbePlan $MemberProbePlan `
            -HarnessGuid '0123456789ABCDEF' `
            -CandidateGuid 'FEDCBA9876543210' `
            -RunNonce $nonce
        if (@($binding.files).Count -ne 3) {
            throw 'Harness render self-test produced an unexpected inventory.'
        }
        $selfRuntime = Join-Path $selfRoot 'runtime-addons'
        New-Item -ItemType Directory -Path $selfRuntime -ErrorAction Stop |
            Out-Null
        $selfInstalled = Join-Path $selfRuntime ($script:HarnessPrefix + $nonce)
        Copy-ReleaseSurfaceTree `
            -SourceRoot (Join-Path $selfRoot 'harness') `
            -DestinationRoot $selfInstalled
        [void](Assert-ReleaseSurfaceTreeBinding `
            -Expected $binding `
            -Root $selfInstalled `
            -Label 'Self-test installed harness')
        Remove-ReleaseSurfaceHarnessExact `
            -RuntimeAddonRoot $selfRuntime `
            -HarnessRoot $selfInstalled `
            -ExpectedBinding $binding `
            -RunNonce $nonce `
            -HarnessGuid '0123456789ABCDEF'
        if (Test-Path -LiteralPath $selfInstalled) {
            throw 'Harness cleanup self-test left its target behind.'
        }
        $source = [IO.File]::ReadAllText((Join-Path $selfRoot `
            'harness\Scripts\Game\PartisanReleaseSurfaceAudit.c'))
        if (@([regex]::Matches($source, 'forbiddenTypes\.Insert\(')).Count -ne
                @($Contract.forbiddenTypeNames).Count -or
            @([regex]::Matches($source, 'forbiddenCommands\.Insert\(')).Count -ne
                @($Contract.forbiddenCommandActionIds).Count -or
            @([regex]::Matches(
                $source,
                'forbiddenMemberSurfaces\.Insert\(')).Count -ne
                @($Contract.forbiddenMemberSurfaces).Count -or
            @([regex]::Matches(
                $source,
                'productionMemberSurfaces\.Insert\(')).Count -ne
                @($Contract.productionObservabilityMemberSurfaces).Count -or
             $source.IndexOf('.ToType()', [StringComparison]::Ordinal) -lt 0 -or
             $source.IndexOf('BuildVisibleMenuPayload(',
                 [StringComparison]::Ordinal) -lt 0 -or
             $source.IndexOf('IsVisibleCommandSurfaceAvailable(',
                 [StringComparison]::Ordinal) -lt 0) {
            throw 'Harness render self-test did not bind every runtime probe.'
        }

        $mutatedSourceRoot = Join-Path $selfRoot 'guarded-signature-mutation'
        $memberSourcePaths = @(
            @($Contract.forbiddenMemberSurfaces) +
            @($Contract.productionObservabilityMemberSurfaces) |
                ForEach-Object { ([string]$_ -split '::', 2)[0] } |
                Sort-Object -Unique -CaseSensitive)
        foreach ($portableSourcePath in $memberSourcePaths) {
            $sourcePath = Join-Path $RepositoryRoot `
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
        $copiedPlan = New-ReleaseSurfaceMemberProbePlan `
            -RepositoryRoot $mutatedSourceRoot `
            -Contract $Contract
        if (@($copiedPlan.forbidden).Count -ne
                @($MemberProbePlan.forbidden).Count -or
            @($copiedPlan.production).Count -ne
                @($MemberProbePlan.production).Count) {
            throw 'Guarded-signature mutation fixture is incomplete.'
        }
        & git -C $mutatedSourceRoot init -q
        & git -C $mutatedSourceRoot config core.autocrlf false
        & git -C $mutatedSourceRoot config user.name `
            'Partisan Release Surface Self-Test'
        & git -C $mutatedSourceRoot config user.email `
            'release-surface-self-test.invalid'
        & git -C $mutatedSourceRoot add -- .
        & git -C $mutatedSourceRoot commit -q -m `
            'valid guarded member signature'
        $validSourceCommit = (@(& git -C $mutatedSourceRoot rev-parse HEAD) `
            -join '').Trim()
        if ($LASTEXITCODE -ne 0 -or
            $validSourceCommit -cnotmatch '^[0-9a-f]{40}$') {
            throw 'Guarded-signature fixture commit could not be resolved.'
        }
        $committedPlan = New-ReleaseSurfaceMemberProbePlan `
            -RepositoryRoot $mutatedSourceRoot `
            -Contract $Contract `
            -SourceCommit $validSourceCommit
        if (@($committedPlan.forbidden).Count -ne
                @($MemberProbePlan.forbidden).Count -or
            @($committedPlan.production).Count -ne
                @($MemberProbePlan.production).Count) {
            throw 'Candidate-commit guarded-signature fixture is incomplete.'
        }
        $guardedSourcePath = Join-Path $mutatedSourceRoot `
            'Scripts\Game\HST\Components\HST_CampaignCoordinatorComponent.c'
        $guardedSourceText = [IO.File]::ReadAllText($guardedSourcePath)
        $guardedNeedle = ', bool forceDebug = false'
        if (@([regex]::Matches(
                    $guardedSourceText,
                    [regex]::Escape($guardedNeedle))).Count -ne 1) {
            throw 'Guarded-signature mutation fixture is not exact.'
        }
        [IO.File]::WriteAllText(
            $guardedSourcePath,
            $guardedSourceText.Replace(
                $guardedNeedle,
                ', bool renamedDebug = false'),
            (New-Object Text.UTF8Encoding($false)))
        & git -C $mutatedSourceRoot add -- `
            'Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c'
        & git -C $mutatedSourceRoot commit -q -m `
            'rename guarded member parameter'
        $renamedSourceCommit = (@(& git -C $mutatedSourceRoot rev-parse HEAD) `
            -join '').Trim()
        if ($LASTEXITCODE -ne 0 -or
            $renamedSourceCommit -cnotmatch '^[0-9a-f]{40}$' -or
            $renamedSourceCommit -ceq $validSourceCommit) {
            throw 'Renamed guarded-signature fixture commit could not be resolved.'
        }
        $rejected = $false
        try {
            [void](New-ReleaseSurfaceMemberProbePlan `
                -RepositoryRoot $mutatedSourceRoot `
                -Contract $Contract `
                -SourceCommit $renamedSourceCommit)
        }
        catch {
            if ($_.Exception.Message -cnotmatch
                'guarded member signature') {
                throw
            }
            $rejected = $true
        }
        if (-not $rejected) {
            throw 'A renamed guarded parameter was accepted by source binding.'
        }
        foreach ($mode in $script:Modes) {
            $result = New-ReleaseSurfaceSyntheticProbeResult `
                -Mode $mode `
                -Contract $Contract `
                -MemberProbePlan $MemberProbePlan `
                -RunNonce $nonce `
                -Candidate $candidate
            [void](Assert-ReleaseSurfaceProbeResult `
                -Result $result `
                -Mode $mode `
                -Contract $Contract `
                -MemberProbePlan $MemberProbePlan `
                -RunNonce $nonce `
                -Candidate $candidate)
        }
        $bad = New-ReleaseSurfaceSyntheticProbeResult `
            -Mode diagnostic `
            -Contract $Contract `
            -MemberProbePlan $MemberProbePlan `
            -RunNonce $nonce `
            -Candidate $candidate
        $bad.forbiddenTypes[0].present = $false
        $rejected = $false
        try {
            [void](Assert-ReleaseSurfaceProbeResult `
                -Result $bad `
                -Mode diagnostic `
                -Contract $Contract `
                -MemberProbePlan $MemberProbePlan `
                -RunNonce $nonce `
                -Candidate $candidate)
        }
        catch {
            $rejected = $true
        }
         if (-not $rejected) {
             throw 'Probe mismatch self-test was not rejected.'
         }
         foreach ($property in @('generatedPresent', 'routingPresent')) {
             $bad = New-ReleaseSurfaceSyntheticProbeResult `
                  -Mode diagnostic `
                  -Contract $Contract `
                  -MemberProbePlan $MemberProbePlan `
                 -RunNonce $nonce `
                 -Candidate $candidate
             $bad.forbiddenCommands[0].$property = $false
             $rejected = $false
             try {
                 [void](Assert-ReleaseSurfaceProbeResult `
                     -Result $bad `
                      -Mode diagnostic `
                      -Contract $Contract `
                      -MemberProbePlan $MemberProbePlan `
                     -RunNonce $nonce `
                     -Candidate $candidate)
             }
             catch {
                 $rejected = $true
             }
             if (-not $rejected) {
                 throw "Command $property mismatch self-test was not rejected."
              }
          }
        foreach ($fault in @(
                'forbidden-member-presence',
                'forbidden-member-support',
                'production-member-presence',
                'compiler-control')) {
            $bad = New-ReleaseSurfaceSyntheticProbeResult `
                -Mode diagnostic `
                -Contract $Contract `
                -MemberProbePlan $MemberProbePlan `
                -RunNonce $nonce `
                -Candidate $candidate
            switch ($fault) {
                'forbidden-member-presence' {
                    $bad.forbiddenMembers[0].present = $false
                }
                'forbidden-member-support' {
                    $bad.forbiddenMembers[0].probeSupported = $false
                }
                'production-member-presence' {
                    $bad.productionMembers[0].present = $false
                }
                'compiler-control' {
                    $bad.runtimeCompilerAlwaysPublicCompiled = $false
                    $bad.runtimeCompilerAvailable = $false
                }
            }
            $rejected = $false
            try {
                [void](Assert-ReleaseSurfaceProbeResult `
                    -Result $bad `
                    -Mode diagnostic `
                    -Contract $Contract `
                    -MemberProbePlan $MemberProbePlan `
                    -RunNonce $nonce `
                    -Candidate $candidate)
            }
            catch {
                $rejected = $true
            }
            if (-not $rejected) {
                throw "Runtime member-probe $fault self-test was not rejected."
            }
        }
        $retailArguments = [string[]]@(
            '-addonsDir', 'runtime,stage',
            '-gproj', 'stage/addon.gproj',
            '-server', 'Worlds/HST_Dev/HST_Dev.ent',
            '-MissionHeader', 'Missions/HST_Dev.conf',
            '-addons', 'FEDCBA9876543210,0123456789ABCDEF',
            '-profile', 'profile',
            '-logsDir', 'logs',
            '-addonTempDir', 'addon-temp',
            '-hstReleaseCandidateId', 'candidate',
            '-hstReleasePackageSha256', ('1' * 64),
            '-hstReleaseManifestSha256', ('2' * 64),
            '-releaseSurfaceRunNonce', $nonce,
            '-releaseSurfaceExpectedMode', 'retail')
        Assert-ReleaseSurfaceArguments `
            -Mode retail `
            -Arguments $retailArguments
        $diagnosticBaseArguments = [string[]]$retailArguments.Clone()
        $expectedModeIndex = [Array]::IndexOf(
            $diagnosticBaseArguments,
            '-releaseSurfaceExpectedMode')
        $diagnosticBaseArguments[$expectedModeIndex + 1] = 'diagnostic'
        $diagnosticArguments = [string[]]@(
            $diagnosticBaseArguments + @('-scrDefine', 'ENABLE_DIAG'))
        Assert-ReleaseSurfaceArguments `
            -Mode diagnostic `
            -Arguments $diagnosticArguments

        $argumentFaults = @(
            [pscustomobject]@{
                label = 'retail symbol injection'
                mode = 'retail'
                arguments = [string[]]@(
                    $retailArguments + @('-scrDefine', 'ENABLE_DIAG'))
            },
            [pscustomobject]@{
                label = 'missing diagnostic symbol'
                mode = 'diagnostic'
                arguments = $diagnosticBaseArguments
            },
            [pscustomobject]@{
                label = 'wrong diagnostic symbol'
                mode = 'diagnostic'
                arguments = [string[]]@(
                    $diagnosticBaseArguments + @('-scrDefine', 'OTHER_DIAG'))
            },
            [pscustomobject]@{
                label = 'duplicate diagnostic symbol'
                mode = 'diagnostic'
                arguments = [string[]]@(
                    $diagnosticArguments + @('-scrDefine', 'ENABLE_DIAG'))
            },
            [pscustomobject]@{
                label = 'case-variant diagnostic option'
                mode = 'diagnostic'
                arguments = [string[]]@(
                    $diagnosticBaseArguments + @('-SCRDEFINE', 'ENABLE_DIAG'))
            },
            [pscustomobject]@{
                label = 'case-variant diagnostic symbol'
                mode = 'diagnostic'
                arguments = [string[]]@(
                    $diagnosticBaseArguments + @('-scrDefine', 'enable_diag'))
            },
            [pscustomobject]@{
                label = 'joined diagnostic symbol option'
                mode = 'diagnostic'
                arguments = [string[]]@(
                    $diagnosticBaseArguments + @('-scrDefine=ENABLE_DIAG'))
            })
        foreach ($fault in $argumentFaults) {
            $rejected = $false
            try {
                Assert-ReleaseSurfaceArguments `
                    -Mode ([string]$fault.mode) `
                    -Arguments ([string[]]$fault.arguments)
            }
            catch {
                $rejected = $true
            }
            if (-not $rejected) {
                throw "Script-symbol argument self-test was not rejected: $($fault.label)."
            }
        }
        $portable = ConvertTo-ReleaseSurfacePortableArguments `
            -Arguments @('-profile', (Join-Path $selfRoot 'profile')) `
            -Replacements @([pscustomobject]@{
                path = $selfRoot
                token = '<run>'
            })
        if ($portable[1] -cne '<run>/profile') {
            throw 'Portable argument redaction self-test failed.'
        }
        $logRoot = Join-Path $selfRoot 'synthetic-logs'
        New-Item -ItemType Directory -Path $logRoot -ErrorAction Stop |
            Out-Null
        $resultLine =
            '2026-07-20 17:00:00.100 SCRIPT : Partisan release surface audit | ' +
            'RESULT | mode=retail | passed=true | mismatches=0'
        $replicationFinishingLine =
            '2026-07-20 17:00:00.200 RPL : Replication finishing...'
        $replicationFinishedLine =
            '2026-07-20 17:00:00.300 RPL : Replication finished.'
        $gameDestroyedLine =
            '2026-07-20 17:00:00.600 ENGINE : Game destroyed.'
        $selfCandidateProjectPath = Join-Path $selfRoot `
            'candidate-stage\Partisan\addon.gproj'
        $selfCandidateLogPath = [IO.Path]::GetFullPath(
            $selfCandidateProjectPath).Replace('\', '/')
        $candidateMountLines = @(
            ("2026-07-20 17:00:00.010 ENGINE : gproj: '" +
                $selfCandidateLogPath +
                "' guid: 'FEDCBA9876543210' (packed)"),
            ("2026-07-20 17:00:00.020 ENGINE : gproj: '" +
                $selfCandidateLogPath +
                "' guid: 'FEDCBA9876543210'"))
        $foreignMountLines = @(
            ("2026-07-20 17:00:00.001 ENGINE : gproj: " +
                "'runtime-addons/core/core.gproj' " +
                "guid: '5614BBCCBB55ED1C' (packed)"),
            ("2026-07-20 17:00:00.002 ENGINE : gproj: " +
                "'runtime-addons/data/ArmaReforger.gproj' " +
                "guid: '58D0FB3206B6F859' (packed)"),
            ("2026-07-20 17:00:00.030 ENGINE : gproj: " +
                "'runtime-addons/core/core.gproj' " +
                "guid: '5614BBCCBB55ED1C'"),
            ("2026-07-20 17:00:00.040 ENGINE : gproj: " +
                "'runtime-addons/data/ArmaReforger.gproj' " +
                "guid: '58D0FB3206B6F859'"))
        $mountLines = @(
            $foreignMountLines[0]
            $foreignMountLines[1]
            $candidateMountLines[0]
            $candidateMountLines[1]
            $foreignMountLines[2]
            $foreignMountLines[3])
        $cleanConsoleText = @(
            $mountLines
            'ADDON GUID 0123456789ABCDEF',
            $resultLine,
            $replicationFinishingLine,
            $replicationFinishedLine,
            $gameDestroyedLine
        ) -join "`n"
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'console.log') `
            -Text ($cleanConsoleText + "`n") `
            -CreateOnly)
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'script.log') `
            -Text ($resultLine + "`n") `
            -CreateOnly)
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'error.log') `
            -Text '' `
            -CreateOnly)
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot `
            -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if (-not [bool]$classification.valid -or
            [string]$classification.hardDiagnosticPolicy -cne
                $script:HardDiagnosticPolicy -or
            -not [bool]$classification.hardDiagnosticFree -or
            [int]$classification.hardDiagnosticRawLineCount -ne 0 -or
            [int]$classification.hardDiagnosticEventCount -ne 0 -or
            [bool]$classification.approvedStockDiagnosticClusterPresent -or
            -not [bool]$classification.approvedStockDiagnosticClusterExact -or
            -not [bool]$classification.approvedStockDiagnosticLifecycleExact -or
            -not [bool]$classification.crashLogContentValid -or
            [int]$classification.resultMarkerOccurrenceCount -ne 2 -or
            -not [bool]$classification.candidateMountAttestation.valid -or
            [int]$classification.candidateMountAttestation.recordCount -ne 2 -or
            [int]$classification.candidateMountAttestation.exactPathCount -ne 2 -or
            [int]$classification.candidateMountAttestation.packedCount -ne 1 -or
            [int]$classification.candidateMountAttestation.invalidModeCount -ne 0 -or
            -not [bool]$classification.candidateMountAttestation.guidExact -or
            -not [bool]$classification.candidateMountAttestation.packed) {
            throw 'Valid log-classification self-test was rejected.'
        }
        if (@($classification.logs).Count -ne 3 -or
            @($classification.logs | Where-Object {
                [string]$_.leaf -ceq 'crash.log'
            }).Count -ne 0) {
            throw 'Missing optional crash log self-test was not retained exactly.'
        }
        $cliOnlyConsoleText = @(
            $foreignMountLines[0]
            $foreignMountLines[1]
            ("2026-07-20 17:00:00.010 ENGINE : CLI Params: -gproj " +
                $selfCandidateProjectPath +
                ' -addons FEDCBA9876543210 (packed)'),
            $foreignMountLines[2]
            $foreignMountLines[3]
            'ADDON GUID 0123456789ABCDEF',
            $resultLine,
            $replicationFinishingLine,
            $replicationFinishedLine,
            $gameDestroyedLine
        ) -join "`n"
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'console.log') `
            -Text ($cliOnlyConsoleText + "`n"))
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid -or
            [int]$classification.candidateMountAttestation.recordCount -ne 0 -or
            [bool]$classification.candidateMountAttestation.valid) {
            throw 'CLI-only candidate mount self-test was accepted.'
        }
        $foreignProjectPath = [IO.Path]::GetFullPath((Join-Path $selfRoot `
            'foreign-stage\Partisan\addon.gproj')).Replace('\', '/')
        $foreignConsoleText = $cleanConsoleText.Replace(
            $selfCandidateLogPath,
            $foreignProjectPath)
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'console.log') `
            -Text ($foreignConsoleText + "`n"))
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid -or
            [int]$classification.candidateMountAttestation.recordCount -ne 2 -or
            [int]$classification.candidateMountAttestation.exactPathCount -ne 0 -or
            [bool]$classification.candidateMountAttestation.valid) {
            throw 'Foreign-path candidate mount self-test was accepted.'
        }
        $caseDriftConsoleText = $cleanConsoleText.Replace(
            "guid: 'FEDCBA9876543210'",
            "guid: 'fedcba9876543210'")
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'console.log') `
            -Text ($caseDriftConsoleText + "`n"))
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid -or
            [int]$classification.candidateMountAttestation.recordCount -ne 2 -or
            [int]$classification.candidateMountAttestation.exactPathCount -ne 2 -or
            [bool]$classification.candidateMountAttestation.guidExact -or
            [bool]$classification.candidateMountAttestation.valid) {
            throw 'Case-drifted candidate GUID mount self-test was accepted.'
        }
        $duplicatePackedLine = [string]$candidateMountLines[1] + ' (packed)'
        $extraCandidateLine =
            "2026-07-20 17:00:00.025 ENGINE : gproj: '" +
            $selfCandidateLogPath + "' guid: 'FEDCBA9876543210'"
        $invalidModeLine = [string]$candidateMountLines[1] + ' (loose)'
        $unpackedFirstLine = ([string]$candidateMountLines[0]).Replace(
            ' (packed)', '')
        $candidateShapeCases = @(
            [pscustomobject][ordered]@{
                label = 'Packed-only candidate mount'
                text = $cleanConsoleText.Replace(
                    [string]$candidateMountLines[1], '')
                recordCount = 1
                exactPathCount = 1
                packedCount = 1
                invalidModeCount = 0
                packed = $true
            },
            [pscustomobject][ordered]@{
                label = 'Duplicate-packed candidate mount'
                text = $cleanConsoleText.Replace(
                    [string]$candidateMountLines[1], $duplicatePackedLine)
                recordCount = 2
                exactPathCount = 2
                packedCount = 2
                invalidModeCount = 0
                packed = $false
            },
            [pscustomobject][ordered]@{
                label = 'Extra candidate mount row'
                text = $cleanConsoleText.Replace(
                    [string]$candidateMountLines[1],
                    ([string]$candidateMountLines[1] + "`n" +
                        $extraCandidateLine))
                recordCount = 3
                exactPathCount = 3
                packedCount = 1
                invalidModeCount = 0
                packed = $true
            },
            [pscustomobject][ordered]@{
                label = 'Invalid-mode candidate mount'
                text = $cleanConsoleText.Replace(
                    [string]$candidateMountLines[1], $invalidModeLine)
                recordCount = 2
                exactPathCount = 2
                packedCount = 1
                invalidModeCount = 1
                packed = $false
            },
            [pscustomobject][ordered]@{
                label = 'No-packed candidate mount'
                text = $cleanConsoleText.Replace(
                    [string]$candidateMountLines[0], $unpackedFirstLine)
                recordCount = 2
                exactPathCount = 2
                packedCount = 0
                invalidModeCount = 0
                packed = $false
            })
        foreach ($candidateShapeCase in $candidateShapeCases) {
            [void](Write-ReleaseSurfaceText `
                -Path (Join-Path $logRoot 'console.log') `
                -Text ([string]$candidateShapeCase.text + "`n"))
            $classification = Get-ReleaseSurfaceLogClassification `
                -LogRoot $logRoot -Mode retail `
                -CandidateGuid 'FEDCBA9876543210' `
                -CandidateProjectPath $selfCandidateProjectPath `
                -HarnessGuid '0123456789ABCDEF'
            $mount = $classification.candidateMountAttestation
            if ([bool]$classification.valid -or [bool]$mount.valid -or
                [int]$mount.recordCount -ne
                    [int]$candidateShapeCase.recordCount -or
                [int]$mount.exactPathCount -ne
                    [int]$candidateShapeCase.exactPathCount -or
                [int]$mount.packedCount -ne
                    [int]$candidateShapeCase.packedCount -or
                [int]$mount.invalidModeCount -ne
                    [int]$candidateShapeCase.invalidModeCount -or
                -not [bool]$mount.guidExact -or
                [bool]$mount.packed -ne [bool]$candidateShapeCase.packed) {
                throw ([string]$candidateShapeCase.label +
                    ' self-test was accepted or misclassified.')
            }
        }
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'console.log') `
            -Text ($cleanConsoleText + "`n"))
        $oneMillisecondResultLine = $resultLine.Replace(
            '17:00:00.100 ', '17:00:00.101 ')
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'script.log') `
            -Text ($oneMillisecondResultLine + "`n"))
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if (-not [bool]$classification.valid -or
            [int]$classification.uniqueResultMarkerCount -ne 1 -or
            [int]$classification.resultMarkerOccurrenceCount -ne 2) {
            throw 'One-millisecond result timestamp drift self-test was rejected.'
        }
        $nonprecedingResultLine = $resultLine.Replace(
            '17:00:00.100 ', '17:00:00.200 ')
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'script.log') `
            -Text ($nonprecedingResultLine + "`n"))
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid -or
            [int]$classification.uniqueResultMarkerCount -ne 1 -or
            [int]$classification.resultMarkerOccurrenceCount -ne 2) {
            throw 'Result timestamp at replication finishing self-test was accepted.'
        }
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'script.log') `
            -Text ($resultLine + "`n"))
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'error.log') `
            -Text "2026-07-20 17:00:00.150 WORLD (E): synthetic retained channel error`n")
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if (-not [bool]$classification.valid -or
            -not [bool]$classification.hardDiagnosticFree -or
            [int]$classification.hardDiagnosticRawLineCount -ne 0) {
            throw 'Explicit non-hard engine-channel policy self-test was rejected.'
        }
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'error.log') `
            -Text '')
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'crash.log') `
            -Text '' `
            -CreateOnly)
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot `
            -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if (-not [bool]$classification.valid -or
            -not [bool]$classification.crashLogContentValid -or
            @($classification.logs).Count -ne 4 -or
            @($classification.logs | Where-Object {
                [string]$_.leaf -ceq 'crash.log'
            }).Count -ne 1) {
            throw 'Present optional crash log self-test was not retained exactly.'
        }
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'crash.log') `
            -Text "benign-looking crash channel text`n")
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot `
            -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid -or
            [bool]$classification.crashLogContentValid -or
            -not [bool]$classification.hardDiagnosticFree) {
            throw 'Non-empty benign-looking crash log self-test was accepted.'
        }
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'crash.log') `
            -Text "SCRIPT (E): synthetic crash-log diagnostic`n")
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot `
            -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid -or
            [int]$classification.hardDiagnosticRawLineCount -ne 1 -or
            [int]$classification.hardDiagnosticEventCount -ne 1 -or
            [int]$classification.unapprovedHardDiagnosticRawLineCount -ne 1) {
            throw 'Optional crash-log diagnostic self-test was accepted.'
        }
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'crash.log') `
            -Text '')
        $duplicateRoot = Join-Path $logRoot 'duplicate'
        New-Item -ItemType Directory -Path $duplicateRoot -ErrorAction Stop |
            Out-Null
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $duplicateRoot 'crash.log') `
            -Text '' `
            -CreateOnly)
        $rejected = $false
        try {
            $null = Get-ReleaseSurfaceLogClassification `
                -LogRoot $logRoot `
                -Mode retail `
                -CandidateGuid 'FEDCBA9876543210' `
                -CandidateProjectPath $selfCandidateProjectPath `
                -HarnessGuid '0123456789ABCDEF'
        }
        catch { $rejected = $true }
        if (-not $rejected) {
            throw 'Duplicate optional crash log self-test was not rejected.'
        }
        Remove-Item -LiteralPath $duplicateRoot -Recurse -Force `
            -ErrorAction Stop
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'unknown.log') `
            -Text '' `
            -CreateOnly)
        $rejected = $false
        try {
            $null = Get-ReleaseSurfaceLogClassification `
                -LogRoot $logRoot `
                -Mode retail `
                -CandidateGuid 'FEDCBA9876543210' `
                -CandidateProjectPath $selfCandidateProjectPath `
                -HarnessGuid '0123456789ABCDEF'
        }
        catch { $rejected = $true }
        if (-not $rejected) {
            throw 'Unknown log leaf self-test was not rejected.'
        }
        Remove-Item -LiteralPath (Join-Path $logRoot 'unknown.log') -Force `
            -ErrorAction Stop
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'unexpected.txt') `
            -Text '' `
            -CreateOnly)
        $rejected = $false
        try {
            $null = Get-ReleaseSurfaceLogClassification `
                -LogRoot $logRoot `
                -Mode retail `
                -CandidateGuid 'FEDCBA9876543210' `
                -CandidateProjectPath $selfCandidateProjectPath `
                -HarnessGuid '0123456789ABCDEF'
        }
        catch { $rejected = $true }
        if (-not $rejected) {
            throw 'Unexpected non-log leaf self-test was not rejected.'
        }
        Remove-Item -LiteralPath (Join-Path $logRoot 'unexpected.txt') -Force `
            -ErrorAction Stop
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'console.log') `
            -Text ($cleanConsoleText.Replace(
                    '17:00:00.200 RPL', '17:00:00.350 RPL') + "`n"))
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid -or
            [bool]$classification.approvedStockDiagnosticLifecycleExact) {
            throw 'Reversed lifecycle timestamp self-test was accepted.'
        }
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'console.log') `
            -Text ($cleanConsoleText.Replace(
                    '2026-07-20 17:00:00.200 RPL',
                    '2026-99-20 17:00:00.200 RPL') + "`n"))
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid -or
            [bool]$classification.approvedStockDiagnosticLifecycleExact) {
            throw 'Malformed lifecycle timestamp self-test was accepted.'
        }
        $stockLineA =
            "2026-07-20 17:00:00.400 SCRIPT (E): " +
            "'SCR_BaseResupplySupportStationComponent' needs a entity catalog manager!"
        $stockLineB =
            "2026-07-20 17:00:00.500 SCRIPT (E): " +
            "'SCR_BaseResupplySupportStationComponent' needs a entity catalog manager!"
        $stockConsoleText = @(
            $mountLines
            'ADDON GUID 0123456789ABCDEF',
            $resultLine,
            $replicationFinishingLine,
            $replicationFinishedLine,
            $stockLineA,
            $stockLineB,
            $gameDestroyedLine
        ) -join "`n"
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'console.log') `
            -Text ($stockConsoleText + "`n"))
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'script.log') `
            -Text ((@($resultLine, $stockLineA, $stockLineB) -join "`n") +
                "`n"))
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'error.log') `
            -Text ((@($stockLineA, $stockLineB) -join "`n") + "`n"))
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot `
            -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if (-not [bool]$classification.valid -or
            [bool]$classification.hardDiagnosticFree -or
            [int]$classification.hardDiagnosticRawLineCount -ne 6 -or
            [int]$classification.hardDiagnosticEventCount -ne 2 -or
            -not [bool]$classification.approvedStockDiagnosticClusterPresent -or
            -not [bool]$classification.approvedStockDiagnosticClusterExact -or
            -not [bool]$classification.approvedStockDiagnosticLifecycleExact -or
            [int]$classification.approvedStockDiagnosticRawLineCount -ne 6 -or
            [int]$classification.approvedStockDiagnosticEventCount -ne 2 -or
            [int]$classification.unapprovedHardDiagnosticRawLineCount -ne 0 -or
            [int]$classification.unapprovedHardDiagnosticEventCount -ne 0 -or
            -not [bool]$classification.hardDiagnosticAccountingExact) {
            throw 'Exact stock teardown cluster self-test was rejected.'
        }

        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'error.log') `
            -Text ($stockLineA + "`n"))
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid -or
            [int]$classification.unapprovedHardDiagnosticRawLineCount -ne 5) {
            throw 'Partial stock teardown mirror self-test was accepted.'
        }

        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'console.log') `
            -Text ((@(
                    $mountLines
                    'ADDON GUID 0123456789ABCDEF',
                    $resultLine,
                    $replicationFinishingLine,
                    $replicationFinishedLine,
                    $stockLineA,
                    $gameDestroyedLine) -join "`n") + "`n"))
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'script.log') `
            -Text ((@($resultLine, $stockLineA) -join "`n") + "`n"))
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'error.log') `
            -Text ($stockLineA + "`n"))
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid -or
            [int]$classification.hardDiagnosticEventCount -ne 1) {
            throw 'One-event stock teardown self-test was accepted.'
        }

        $stockLineC = $stockLineB.Replace('.500 ', '.550 ')
        foreach ($leaf in @('console.log', 'script.log', 'error.log')) {
            $leafText = [IO.File]::ReadAllText((Join-Path $logRoot $leaf))
            if ($leaf -ceq 'console.log') {
                $leafText = $stockConsoleText.Replace(
                    $gameDestroyedLine, $stockLineC + "`n" + $gameDestroyedLine)
            }
            elseif ($leaf -ceq 'script.log') {
                $leafText = (@(
                    $resultLine, $stockLineA, $stockLineB, $stockLineC) -join "`n") +
                    "`n"
            }
            else {
                $leafText = (@($stockLineA, $stockLineB, $stockLineC) -join "`n") +
                    "`n"
            }
            [void](Write-ReleaseSurfaceText `
                -Path (Join-Path $logRoot $leaf) -Text $leafText)
        }
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid -or
            [int]$classification.hardDiagnosticEventCount -ne 3) {
            throw 'Third stock teardown event self-test was accepted.'
        }

        $earlyStockA = $stockLineA.Replace('.400 ', '.250 ')
        $earlyStockB = $stockLineB.Replace('.500 ', '.260 ')
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'console.log') `
            -Text ($stockConsoleText.Replace($stockLineA, $earlyStockA).
                Replace($stockLineB, $earlyStockB) + "`n"))
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'script.log') `
            -Text ((@($resultLine, $earlyStockA, $earlyStockB) -join "`n") +
                "`n"))
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'error.log') `
            -Text ((@($earlyStockA, $earlyStockB) -join "`n") + "`n"))
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid -or
            [bool]$classification.approvedStockDiagnosticLifecycleExact) {
            throw 'Pre-replication stock teardown self-test was accepted.'
        }

        $variantLine = $stockLineA.Replace('needs a entity', 'needs an entity')
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'console.log') `
            -Text ($stockConsoleText.Replace($stockLineA, $variantLine) + "`n"))
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'script.log') `
            -Text ((@($resultLine, $variantLine, $stockLineB) -join "`n") +
                "`n"))
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'error.log') `
            -Text ((@($variantLine, $stockLineB) -join "`n") + "`n"))
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid -or
            [int]$classification.unapprovedHardDiagnosticEventCount -ne 2) {
            throw 'Stock teardown message-variant self-test was accepted.'
        }

        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'console.log') `
            -Text ($stockConsoleText.Replace(
                    $stockLineA, $stockLineA + "`n  synthetic diagnostic body") +
                "`n"))
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'script.log') `
            -Text ((@($resultLine, $stockLineA, $stockLineB) -join "`n") +
                "`n"))
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'error.log') `
            -Text ((@($stockLineA, $stockLineB) -join "`n") + "`n"))
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid) {
            throw 'Non-empty stock diagnostic body self-test was accepted.'
        }

        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'console.log') `
            -Text ($cleanConsoleText + "`nSCRIPT (E): synthetic failure`n"))
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'script.log') `
            -Text ($resultLine + "`n"))
        [void](Write-ReleaseSurfaceText `
            -Path (Join-Path $logRoot 'error.log') `
            -Text '')
        $classification = Get-ReleaseSurfaceLogClassification `
            -LogRoot $logRoot `
            -Mode retail `
            -CandidateGuid 'FEDCBA9876543210' `
            -CandidateProjectPath $selfCandidateProjectPath `
            -HarnessGuid '0123456789ABCDEF'
        if ([bool]$classification.valid) {
            throw 'Hard-diagnostic log-classification self-test was accepted.'
        }
        Write-Output ('SELFTEST ' + ([pscustomobject][ordered]@{
            passed = $true
            forbiddenTypeCount = @($Contract.forbiddenTypeNames).Count
            forbiddenCommandCount = @($Contract.forbiddenCommandActionIds).Count
            forbiddenMemberCount = @($MemberProbePlan.forbidden).Count
            productionMemberCount = @($MemberProbePlan.production).Count
            harnessFileCount = @($binding.files).Count
            checks = 56
        } | ConvertTo-Json -Compress))
    }
    finally {
        if (Test-Path -LiteralPath $selfRoot) {
            Remove-Item -LiteralPath $selfRoot -Recurse -Force `
                -ErrorAction SilentlyContinue
        }
    }
}

$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$contractPath = Join-Path $repositoryRoot `
    'docs\data\release_surface_contract.json'
$templateRoot = Join-Path $PSScriptRoot `
    'release-surface-audit-harness-template'
$contract = Get-ReleaseSurfaceContract -Path $contractPath
$memberProbePlan = New-ReleaseSurfaceMemberProbePlan `
    -RepositoryRoot $repositoryRoot `
    -Contract $contract

if ($SelfTest) {
    Invoke-ReleaseSurfaceSelfTest `
        -RepositoryRoot $repositoryRoot `
        -TemplateRoot $templateRoot `
        -Contract $contract `
        -MemberProbePlan $memberProbePlan
    return
}

foreach ($entry in @(
        [pscustomobject]@{ Name = 'ManifestPath'; Value = $ManifestPath },
        [pscustomobject]@{ Name = 'BundleRoot'; Value = $BundleRoot },
        [pscustomobject]@{ Name = 'RuntimeAddonRoot'; Value = $RuntimeAddonRoot },
        [pscustomobject]@{
            Name = 'ServerDiagnosticExecutable'
            Value = $ServerDiagnosticExecutable
        },
        [pscustomobject]@{ Name = 'EvidenceRoot'; Value = $EvidenceRoot })) {
    if ([string]::IsNullOrWhiteSpace([string]$entry.Value)) {
        throw "Real release-surface audit requires $($entry.Name)."
    }
}
if ($WatchedRoots.Count -eq 0 -or $SpillRoots.Count -eq 0) {
    throw 'Real release-surface audit requires explicit watched and spill roots.'
}
if ($WorldResource -cne 'Worlds/HST_Dev/HST_Dev.ent' -or
    $MissionHeader -cne 'Missions/HST_Dev.conf') {
    throw 'Release-surface audit requires the exact disposable HST_Dev scenario.'
}

$candidateModulePath = Join-Path $PSScriptRoot 'Partisan.ReleaseCandidate.psm1'
$guardModulePath = Join-Path $PSScriptRoot 'Partisan.GuardedRuntime.psm1'
$releaseIndexProducerPath = Join-Path $PSScriptRoot `
    'New-PartisanReleaseSurfaceAuditIndex.ps1'
$gate1EvidenceConsumerPath = Join-Path $PSScriptRoot `
    'Partisan.Gate1EvidenceConsumer.psm1'
$releaseDocsConsumerPath = Join-Path $PSScriptRoot 'update-release-docs.ps1'
Import-Module -Name $candidateModulePath -Force -ErrorAction Stop
Import-Module -Name $guardModulePath -Force -ErrorAction Stop

foreach ($toolPath in @(
        $PSCommandPath,
        $candidateModulePath,
        $guardModulePath,
        $releaseIndexProducerPath,
        $gate1EvidenceConsumerPath,
        $releaseDocsConsumerPath,
        $contractPath,
        (Join-Path $templateRoot 'addon.gproj.template'),
        (Join-Path $templateRoot `
            'Scripts\Game\PartisanReleaseSurfaceAudit.c.template'))) {
    [void](Resolve-ReleaseSurfacePath -Path $toolPath -Kind Leaf)
}
$harnessGitHead = (@(& git -C $repositoryRoot rev-parse HEAD 2>$null) -join '').Trim()
if ($LASTEXITCODE -ne 0 -or $harnessGitHead -cnotmatch '^[0-9a-f]{40}$') {
    throw 'Release-surface audit could not resolve the harness Git HEAD.'
}
$dirtyCheckout = @(& git -C $repositoryRoot status --porcelain --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $dirtyCheckout.Count -ne 0) {
    throw 'Release-surface audit requires a clean tracked and untracked checkout.'
}
$null = Assert-PartisanGitWorktreeFilesMatchCommit `
    -RepositoryRoot $repositoryRoot `
    -Commit $harnessGitHead `
    -PortablePaths @(
        'tools/run-guarded-release-surface-audit.ps1',
        'tools/New-PartisanReleaseSurfaceAuditIndex.ps1',
        'tools/Partisan.ReleaseCandidate.psm1',
        'tools/Partisan.GuardedRuntime.psm1',
        'tools/Partisan.Gate1EvidenceConsumer.psm1',
        'tools/update-release-docs.ps1',
        'docs/data/release_surface_contract.json',
        'tools/release-surface-audit-harness-template/addon.gproj.template',
        'tools/release-surface-audit-harness-template/Scripts/Game/PartisanReleaseSurfaceAudit.c.template')

$manifestFull = Resolve-ReleaseSurfacePath -Path $ManifestPath -Kind Leaf
$bundleFull = Resolve-ReleaseSurfacePath -Path $BundleRoot -Kind Container
$runtimeAddonFull = Resolve-ReleaseSurfacePath `
    -Path $RuntimeAddonRoot `
    -Kind Container
$diagnosticFull = Resolve-ReleaseSurfacePath `
    -Path $ServerDiagnosticExecutable `
    -Kind Leaf
if ((Split-Path -Leaf $diagnosticFull) -cne 'ArmaReforgerServerDiag.exe') {
    throw 'ServerDiagnosticExecutable must be ArmaReforgerServerDiag.exe.'
}
$standardFull = Resolve-ReleaseSurfacePath `
    -Path (Join-Path (Split-Path -Parent $diagnosticFull) `
        'ArmaReforgerServer.exe') `
    -Kind Leaf
$evidenceFull = [IO.Path]::GetFullPath($EvidenceRoot)
$evidenceTrimmed = $evidenceFull.TrimEnd('\', '/')
$evidenceVolumeRoot = ([IO.Path]::GetPathRoot($evidenceFull)).TrimEnd('\', '/')
if ($evidenceTrimmed.Equals(
        $evidenceVolumeRoot,
        [StringComparison]::OrdinalIgnoreCase)) {
    throw 'EvidenceRoot must not be a volume root.'
}
foreach ($protectedRoot in @(
        $bundleFull,
        $runtimeAddonFull,
        (Split-Path -Parent $diagnosticFull))) {
    if (Test-ReleaseSurfacePathOverlap `
            -First $evidenceFull `
            -Second $protectedRoot) {
        throw 'EvidenceRoot must not overlap a sealed or runtime installation root.'
    }
}
if (-not (Test-Path -LiteralPath $evidenceFull -PathType Container)) {
    New-Item -ItemType Directory -Path $evidenceFull -Force `
        -ErrorAction Stop | Out-Null
}
Assert-PartisanNoReparseAncestry -Path $evidenceFull

$resolvedWatchedRoots = @($WatchedRoots | ForEach-Object {
    Resolve-ReleaseSurfacePath -Path $_ -Kind Container
})
$resolvedSpillRoots = @($SpillRoots | ForEach-Object {
    Resolve-ReleaseSurfacePath -Path $_ -Kind Container
})
foreach ($boundary in @($resolvedWatchedRoots + $resolvedSpillRoots)) {
    if (Test-ReleaseSurfacePathOverlap `
            -First $evidenceFull `
            -Second $boundary) {
        throw 'EvidenceRoot must not overlap a watched or spill root.'
    }
}

$candidate = Assert-PartisanReleaseCandidate `
    -ManifestPath $manifestFull `
    -BundleRoot $bundleFull `
    -RuntimeAddonRoot $runtimeAddonFull `
    -Executable $diagnosticFull `
    -RuntimeRole server `
    -ConsumerIntent runtime
if ([string]$candidate.RuntimeUseDisposition -cne
    'active-runtime-candidate') {
    throw 'Release-surface audit requires the exact active runtime candidate.'
}
& git -C $repositoryRoot merge-base --is-ancestor `
    ([string]$candidate.GitHead) $harnessGitHead 2>$null
if ($LASTEXITCODE -ne 0) {
    throw 'Release-surface harness HEAD is not a candidate descendant.'
}
$memberProbePlan = New-ReleaseSurfaceMemberProbePlan `
    -RepositoryRoot $repositoryRoot `
    -Contract $contract `
    -SourceCommit ([string]$candidate.GitHead)
$diagnosticProvenance = Get-PartisanExecutableProvenance -Path $diagnosticFull
$standardProvenance = Get-PartisanExecutableProvenance -Path $standardFull
Assert-ReleaseSurfaceExecutableIdentity `
    -Expected $candidate.RecordedDiagnosticExecutable `
    -Actual $diagnosticProvenance `
    -Label diagnostic
Assert-ReleaseSurfaceExecutableIdentity `
    -Expected $candidate.RecordedRuntimeExecutable `
    -Actual $standardProvenance `
    -Label retail

$preexistingHarnesses = @(Get-ChildItem -LiteralPath $runtimeAddonFull `
    -Directory -Force -ErrorAction Stop | Where-Object {
        $_.Name -clike ($script:HarnessPrefix + '*')
    })
if ($preexistingHarnesses.Count -ne 0) {
    throw 'A disposable release-surface harness residue already exists.'
}

$mutex = New-Object Threading.Mutex(
    $false,
    'Local\PartisanReleaseSurfaceAuditGuard')
$mutexAcquired = $false
try {
    try {
        $mutexAcquired = $mutex.WaitOne(0)
    }
    catch [Threading.AbandonedMutexException] {
        $mutexAcquired = $true
    }
    if (-not $mutexAcquired) {
        throw 'Another release-surface audit owns the runtime harness slot.'
    }

    $startedUtc = [DateTime]::UtcNow.ToString('o')
    $runStamp = [DateTime]::UtcNow.ToString(
        'yyyyMMddTHHmmssZ',
        [Globalization.CultureInfo]::InvariantCulture)
    $runNonce = [Guid]::NewGuid().ToString('N')
    $runId = 'release_surface_' + $runStamp + '_' +
        $runNonce.Substring(0, 20)
    $harnessGuid = $runNonce.Substring(0, 16).ToUpperInvariant()
    if ($harnessGuid -ceq [string]$candidate.AddonGuid) {
        $harnessGuid = $runNonce.Substring(16, 16).ToUpperInvariant()
    }
    $runLeaf = $runStamp + '-' + $runNonce
    $runRoot = Join-Path (Join-Path (Join-Path $evidenceFull `
        ([string]$candidate.CandidateId)) 'release-surface-audit') $runLeaf
    if (Test-Path -LiteralPath $runRoot) {
        throw 'Release-surface evidence run root is not fresh.'
    }
    New-Item -ItemType Directory -Path $runRoot -ErrorAction Stop | Out-Null
    foreach ($directory in @(
            (Join-Path $runRoot 'identity'),
            (Join-Path $runRoot 'harness'),
            (Join-Path $runRoot 'modes'),
            (Join-Path $runRoot 'raw'))) {
        New-Item -ItemType Directory -Path $directory -Force `
            -ErrorAction Stop | Out-Null
    }
    Assert-PartisanNoReparseAncestry -Path $runRoot

    $harnessEvidenceRoot = Join-Path $runRoot 'harness\source'
    $harnessBinding = New-ReleaseSurfaceHarness `
        -TemplateRoot $templateRoot `
        -DestinationRoot $harnessEvidenceRoot `
        -Contract $contract `
        -MemberProbePlan $memberProbePlan `
        -HarnessGuid $harnessGuid `
        -CandidateGuid ([string]$candidate.AddonGuid) `
        -RunNonce $runNonce
    $installedHarnessRoot = Join-Path $runtimeAddonFull `
        ($script:HarnessPrefix + $runNonce)
    $installedHarnessBinding = $null
    $harnessInstalled = $false
    $runError = $null
    $cleanupError = $null
    $modeResults = @()
    try {
        Copy-ReleaseSurfaceTree `
            -SourceRoot $harnessEvidenceRoot `
            -DestinationRoot $installedHarnessRoot
        $harnessInstalled = $true
        $installedHarnessBinding = Assert-ReleaseSurfaceTreeBinding `
            -Expected $harnessBinding `
            -Root $installedHarnessRoot `
            -Label 'Installed release-surface harness'

        [void](Write-ReleaseSurfaceJson `
            -Path (Join-Path $runRoot 'run.owner.json') `
            -Value ([ordered]@{
                 schemaVersion = 1
                 evidenceKind = $script:EvidenceKind
                 runId = $runId
                 runNonce = $runNonce
                candidateId = [string]$candidate.CandidateId
                packageSha256 = [string]$candidate.PackageSha256
                harnessGuid = $harnessGuid
                harnessSha256 = [string]$harnessBinding.aggregateSha256
                disposition = 'in-progress-noncertifying-release-surface-audit'
            }) `
            -Portable `
            -CreateOnly)
        Copy-Item -LiteralPath $manifestFull `
            -Destination (Join-Path $runRoot 'identity\candidate.json') `
            -ErrorAction Stop
        Copy-Item -LiteralPath $candidate.TrackedReadyPath `
            -Destination (Join-Path $runRoot 'identity\candidate.ready.json') `
            -ErrorAction Stop
        Copy-Item -LiteralPath $contractPath `
            -Destination (Join-Path $runRoot `
                'identity\release_surface_contract.json') `
            -ErrorAction Stop
        [void](Write-ReleaseSurfaceJson `
            -Path (Join-Path $runRoot 'identity\bindings.json') `
             -Value ([ordered]@{
                 schemaVersion = 1
                 evidenceKind = $script:EvidenceKind
                 source = [ordered]@{
                     harnessGitHead = $harnessGitHead
                     checkoutClean = $true
                     candidateAncestor = $true
                     executionMode = 'paired-native-engine-audit'
                 }
                 candidate = Get-PartisanPublicCandidateIdentity `
                    -Candidate $candidate
                package = [ordered]@{
                    hashAlgorithm = [string]$candidate.PackageHashAlgorithm
                    sha256 = [string]$candidate.PackageSha256
                    files = [object[]]$candidate.PackageFiles
                }
                executables = [ordered]@{
                    retail = $standardProvenance
                    diagnostic = $diagnosticProvenance
                }
                harness = [ordered]@{
                    id = $script:HarnessId
                    guid = $harnessGuid
                    candidateDependencyGuid = [string]$candidate.AddonGuid
                    constructionMode =
                        'external-disposable-runtime-loaded-audit-addon'
                    aggregateSha256 =
                        [string]$harnessBinding.aggregateSha256
                    files = [object[]]$harnessBinding.files
                }
                tools = [object[]]@(
                    New-ReleaseSurfaceToolBinding `
                        -Role runner `
                        -PortablePath `
                            'tools/run-guarded-release-surface-audit.ps1' `
                        -Path $PSCommandPath
                    New-ReleaseSurfaceToolBinding `
                        -Role releaseIndexProducer `
                        -PortablePath `
                            'tools/New-PartisanReleaseSurfaceAuditIndex.ps1' `
                        -Path $releaseIndexProducerPath
                    New-ReleaseSurfaceToolBinding `
                        -Role releaseCandidateModule `
                        -PortablePath 'tools/Partisan.ReleaseCandidate.psm1' `
                        -Path $candidateModulePath
                    New-ReleaseSurfaceToolBinding `
                        -Role guardedRuntimeModule `
                        -PortablePath 'tools/Partisan.GuardedRuntime.psm1' `
                        -Path $guardModulePath
                    New-ReleaseSurfaceToolBinding `
                        -Role gate1EvidenceConsumer `
                        -PortablePath `
                            'tools/Partisan.Gate1EvidenceConsumer.psm1' `
                        -Path $gate1EvidenceConsumerPath
                    New-ReleaseSurfaceToolBinding `
                        -Role releaseDocsConsumer `
                        -PortablePath 'tools/update-release-docs.ps1' `
                        -Path $releaseDocsConsumerPath
                    New-ReleaseSurfaceToolBinding `
                        -Role contract `
                        -PortablePath `
                            'docs/data/release_surface_contract.json' `
                        -Path $contractPath
                    New-ReleaseSurfaceToolBinding `
                        -Role harnessProjectTemplate `
                        -PortablePath `
                            'tools/release-surface-audit-harness-template/addon.gproj.template' `
                        -Path (Join-Path $templateRoot 'addon.gproj.template')
                    New-ReleaseSurfaceToolBinding `
                        -Role harnessSourceTemplate `
                        -PortablePath `
                            'tools/release-surface-audit-harness-template/Scripts/Game/PartisanReleaseSurfaceAudit.c.template' `
                        -Path (Join-Path $templateRoot `
                            'Scripts\Game\PartisanReleaseSurfaceAudit.c.template'))
                host = [ordered]@{
                    powershellVersion = $PSVersionTable.PSVersion.ToString()
                    powershellEdition = [string]$PSVersionTable.PSEdition
                    operatingSystem = [Environment]::OSVersion.VersionString
                }
            }) `
            -Portable `
            -CreateOnly)

        $modeResults = @(
            Invoke-ReleaseSurfaceMode `
                -Mode retail `
                -RunRoot $runRoot `
                -RunNonce $runNonce `
                -Candidate $candidate `
                -ManifestPath $manifestFull `
                -BundleRoot $bundleFull `
                -RuntimeAddonRoot $runtimeAddonFull `
                -DiagnosticExecutable $diagnosticFull `
                -Executable $standardFull `
                -ExecutableProvenance $standardProvenance `
                -Contract $contract `
                -MemberProbePlan $memberProbePlan `
                -HarnessGuid $harnessGuid `
                -HarnessRoot $installedHarnessRoot `
                -HarnessBinding $installedHarnessBinding `
                -WatchedRoots $resolvedWatchedRoots `
                -SpillRoots $resolvedSpillRoots `
                -LoopbackPort $LoopbackPort `
                -TimeoutSeconds $TimeoutSeconds `
                -PollMilliseconds $PollMilliseconds `
                -WorldResource $WorldResource `
                -MissionHeader $MissionHeader
            Invoke-ReleaseSurfaceMode `
                -Mode diagnostic `
                -RunRoot $runRoot `
                -RunNonce $runNonce `
                -Candidate $candidate `
                -ManifestPath $manifestFull `
                -BundleRoot $bundleFull `
                -RuntimeAddonRoot $runtimeAddonFull `
                -DiagnosticExecutable $diagnosticFull `
                -Executable $diagnosticFull `
                -ExecutableProvenance $diagnosticProvenance `
                -Contract $contract `
                -MemberProbePlan $memberProbePlan `
                -HarnessGuid $harnessGuid `
                -HarnessRoot $installedHarnessRoot `
                -HarnessBinding $installedHarnessBinding `
                -WatchedRoots $resolvedWatchedRoots `
                -SpillRoots $resolvedSpillRoots `
                -LoopbackPort $LoopbackPort `
                -TimeoutSeconds $TimeoutSeconds `
                -PollMilliseconds $PollMilliseconds `
                -WorldResource $WorldResource `
                -MissionHeader $MissionHeader)
        if (@($modeResults).Count -ne 2 -or
            [string]$modeResults[0].mode -cne 'retail' -or
            [string]$modeResults[0].probe.mode -cne 'retail' -or
            [string]$modeResults[1].mode -cne 'diagnostic' -or
            [string]$modeResults[1].probe.mode -cne 'diagnostic') {
            throw 'Release-surface audit did not produce the exact paired mode result.'
        }
    }
    catch {
        $runError = $_
    }
    finally {
        if ($harnessInstalled) {
            try {
                Remove-ReleaseSurfaceHarnessExact `
                    -RuntimeAddonRoot $runtimeAddonFull `
                    -HarnessRoot $installedHarnessRoot `
                    -ExpectedBinding $installedHarnessBinding `
                    -RunNonce $runNonce `
                    -HarnessGuid $harnessGuid
                $harnessInstalled = $false
            }
            catch {
                $cleanupError = $_
            }
        }
    }

    $residue = @(Get-ChildItem -LiteralPath $runtimeAddonFull -Directory `
        -Force -ErrorAction Stop | Where-Object {
            $_.Name -clike ($script:HarnessPrefix + '*')
        })
    $cleanupValue = [ordered]@{
        schemaVersion = 1
        evidenceKind = $script:EvidenceKind
        harnessRemoved = -not $harnessInstalled
        harnessResidueCount = $residue.Count
        exactOwnerVerifiedBeforeRemoval = $null -eq $cleanupError
        passed = -not $harnessInstalled -and
            $residue.Count -eq 0 -and
            $null -eq $cleanupError
    }
    [void](Write-ReleaseSurfaceJson `
        -Path (Join-Path $runRoot 'cleanup.json') `
        -Value $cleanupValue `
        -Portable `
        -CreateOnly)

    if ($runError -or $cleanupError -or -not $cleanupValue.passed) {
        $message = if ($runError) {
            [string]$runError.Exception.Message
        }
        elseif ($cleanupError) {
            [string]$cleanupError.Exception.Message
        }
        else {
            'Release-surface harness residue audit failed.'
        }
        foreach ($path in @(
                $repositoryRoot,
                $runRoot,
                $runtimeAddonFull,
                $bundleFull,
                (Split-Path -Parent $diagnosticFull))) {
            $message = $message.Replace($path, '<redacted>')
        }
        [void](Write-ReleaseSurfaceJson `
            -Path (Join-Path $runRoot 'run.failure.json') `
            -Value ([ordered]@{
                schemaVersion = 1
                evidenceKind = $script:EvidenceKind
                disposition = 'failed-noncertifying-release-surface-audit'
                candidateId = [string]$candidate.CandidateId
                packageSha256 = [string]$candidate.PackageSha256
                completedModeCount = @($modeResults).Count
                cleanup = $cleanupValue
                message = $message
            }) `
            -Portable `
            -CreateOnly)
        if ($runError) {
            throw $runError
        }
        if ($cleanupError) {
            throw $cleanupError
        }
        throw 'Release-surface harness residue audit failed.'
    }

    $finalCandidate = Assert-PartisanReleaseCandidate `
        -ManifestPath $manifestFull `
        -BundleRoot $bundleFull `
        -RuntimeAddonRoot $runtimeAddonFull `
        -Executable $diagnosticFull `
        -RuntimeRole server `
        -ConsumerIntent runtime
    Assert-ReleaseSurfaceCandidateIdentity `
        -Expected $candidate `
        -Actual $finalCandidate
    Assert-ReleaseSurfaceExecutableIdentity `
        -Expected $standardProvenance `
        -Actual (Get-PartisanExecutableProvenance -Path $standardFull) `
        -Label retail
    Assert-ReleaseSurfaceExecutableIdentity `
        -Expected $diagnosticProvenance `
        -Actual (Get-PartisanExecutableProvenance -Path $diagnosticFull) `
        -Label diagnostic
    $candidateBindingRows = @($modeResults | ForEach-Object {
        [string]$_.receipt.CandidateBindingSha256
    } | Sort-Object -Unique)
    if (@($modeResults).Count -ne 2 -or $candidateBindingRows.Count -ne 1 -or
        [string]$candidateBindingRows[0] -cnotmatch '^[0-9a-f]{64}$') {
        throw 'Paired release-surface modes do not share one candidate binding.'
    }
    $candidateBindingSha256 = [string]$candidateBindingRows[0]
    $completedUtc = [DateTime]::UtcNow.ToString('o')
    $indexValue = Get-ReleaseSurfaceEvidenceIndex -RunRoot $runRoot
    $indexSignature = Write-ReleaseSurfaceJson `
        -Path (Join-Path $runRoot 'evidence-index.json') `
        -Value $indexValue `
        -Portable `
        -CreateOnly
    $runValue = [ordered]@{
        schemaVersion = 2
        evidenceKind = $script:EvidenceKind
        contractId = $script:RunContractId
        runId = $runId
        runLeafId = $runLeaf
        runNonce = $runNonce
        startedUtc = $startedUtc
        completedUtc = $completedUtc
        disposition = 'passed-noncertifying-release-surface-audit'
        certificationPromotion = 'none'
        source = [ordered]@{
            harnessGitHead = $harnessGitHead
            checkoutClean = $true
            candidateAncestor = $true
            executionMode = 'paired-native-engine-audit'
        }
        candidate = Get-PartisanPublicCandidateIdentity -Candidate $candidate
        candidateBindingSha256 = $candidateBindingSha256
        pairedSamePackage = @($modeResults).Count -eq 2 -and
            [string]$modeResults[0].probe.mode -ceq 'retail' -and
            [string]$modeResults[1].probe.mode -ceq 'diagnostic'
        candidatePackageSha256 = [string]$candidate.PackageSha256
        harnessGuid = $harnessGuid
        harnessSha256 = [string]$harnessBinding.aggregateSha256
        modes = @($modeResults | ForEach-Object {
            [ordered]@{
                mode = [string]$_.mode
                path = [string]$_.path
                signature = $_.signature
            }
        })
        cleanup = $cleanupValue
        evidenceIndex = [ordered]@{
            path = 'evidence-index.json'
            signature = $indexSignature
            aggregateSha256 = [string]$indexValue.aggregateSha256
            fileCount = @($indexValue.files).Count
        }
        limitations = @(
            'This audit uses inert compiler and metadata probes for contracted member-surface presence; separately, it deliberately invokes production menu generation and read-only per-command availability inspection, but it does not execute command actions or mutate campaign gameplay state.',
            'Forbidden literal surfaces are proven by the candidate-bound source guard analysis, not by an unreliable package-byte string scan.',
            'It is not gameplay, multiplayer, persistence, restart, soak, or performance certification.',
            'The guarded launcher deliberately gives the child no inherited standard streams; authoritative engine output is retained in the three required logs and in crash.log when the engine emits it.',
            'The machine-bound hard-diagnostic policy is script-engine-and-process-fatal-v1: SCRIPT or ENGINE error severity, access violations, unhandled exceptions, fatal/application-crash signals, and audit ERROR markers. Other retained engine-channel severities are outside this narrow release-surface predicate. A successful crash.log must be absent or empty.',
            'A mode may contain either no hard diagnostic or one exact stock Eden shutdown cluster: two underlying support-station catalog-manager events mirrored once across console.log, script.log, and error.log after replication finishes and before game destruction. Every partial, extra, variant, misplaced, crash-channel, or unrelated hard event fails closed.'
        )
        passed = $true
    }
    $runSignature = Write-ReleaseSurfaceJson `
        -Path (Join-Path $runRoot 'run.json') `
        -Value $runValue `
        -Portable `
        -CreateOnly
    $releaseIndexPath = Join-Path $runRoot 'release-index.json'
    $published = @(& $releaseIndexProducerPath `
        -RunEnvelopePath (Join-Path $runRoot 'run.json') `
        -OutputPath $releaseIndexPath)
    if ($published.Count -ne 1 -or
        -not (Test-Path -LiteralPath $releaseIndexPath -PathType Leaf)) {
        throw 'Release-surface index publisher did not produce one exact index.'
    }
    $releaseIndexSignature = Get-ReleaseSurfaceFileSignature `
        -Path $releaseIndexPath
    if (-not (Test-ReleaseSurfaceFileSignatureExact `
            -Expected $published[0].Signature `
            -Actual $releaseIndexSignature)) {
        throw 'Published release-surface index signature is not exact.'
    }
    $readyValue = [ordered]@{
        schemaVersion = 2
        evidenceKind = $script:EvidenceKind
        disposition = 'passed-noncertifying-release-surface-audit'
        certificationPromotion = 'none'
        runId = $runId
        runLeafId = $runLeaf
        source = [ordered]@{
            candidateGitHead = [string]$candidate.GitHead
            harnessGitHead = $harnessGitHead
        }
        candidateId = [string]$candidate.CandidateId
        packageSha256 = [string]$candidate.PackageSha256
        manifestSha256 = [string]$candidate.ManifestSha256
        readySha256 = [string]$candidate.ReadySha256
        candidate = Get-PartisanPublicCandidateIdentity -Candidate $candidate
        candidateBindingSha256 = $candidateBindingSha256
        run = [ordered]@{
            path = 'run.json'
            signature = $runSignature
        }
        evidenceIndex = [ordered]@{
            path = 'evidence-index.json'
            signature = $indexSignature
        }
        releaseIndex = [ordered]@{
            path = 'release-index.json'
            signature = $releaseIndexSignature
        }
        cleanupPassed = [bool]$cleanupValue.passed
        sealedLast = $true
    }
    [void](Write-ReleaseSurfaceJson `
        -Path (Join-Path $runRoot 'run.ready.json') `
        -Value $readyValue `
        -Portable `
        -CreateOnly)
    Write-Output ('RESULT ' + ([pscustomobject][ordered]@{
        passed = $true
        disposition = $runValue.disposition
        candidateId = [string]$candidate.CandidateId
        packageSha256 = [string]$candidate.PackageSha256
        modeCount = @($modeResults).Count
        evidenceFileCount = @($indexValue.files).Count
        releaseIndexSha256 = [string]$releaseIndexSignature.sha256
        runRoot = $runRoot
    } | ConvertTo-Json -Compress))
}
finally {
    if ($mutexAcquired) {
        try {
            $mutex.ReleaseMutex()
        }
        catch {
        }
    }
    if ($mutex) {
        $mutex.Dispose()
    }
}
