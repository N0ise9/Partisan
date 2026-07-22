[CmdletBinding(DefaultParameterSetName = 'Run')]
param(
    [Parameter(ParameterSetName = 'Run')]
    [string]$RepositoryRoot = '',

    [Parameter(ParameterSetName = 'Run')]
    [string]$ReleaseStatusPath = 'docs/data/release_status.json',

    [Parameter(ParameterSetName = 'Run')]
    [switch]$LibraryOnly,

    [Parameter(Mandatory = $true, ParameterSetName = 'SelfTest')]
    [switch]$SelfTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:SourceGate1EvidenceOrder = @(
    'foundation',
    'workbenchAllTargets',
    'focusedFiveSuite',
    'forceAuthorityCanary',
    'fullCampaignDebug'
)
$script:SourceGate1EvidenceKinds = [ordered]@{
    foundation = 'source-gate1-foundation'
    workbenchAllTargets = 'source-gate1-workbench-all-targets'
    focusedFiveSuite = 'source-gate1-focused-five-suite'
    forceAuthorityCanary = 'source-gate1-campaign-debug'
    fullCampaignDebug = 'source-gate1-campaign-debug'
}
$script:SourceGate1RunnerPaths = [ordered]@{
    foundation = 'tools/run-source-static-gates.ps1'
    workbenchAllTargets = 'tools/run-source-static-gates.ps1'
    focusedFiveSuite = 'tools/run-source-focused-autotest.ps1'
    forceAuthorityCanary = 'tools/run-source-campaign-debug.ps1'
    fullCampaignDebug = 'tools/run-source-campaign-debug.ps1'
}
$script:SourceGate1WorkbenchTargets = @(
    'PC', 'XBOX_ONE', 'XBOX_SERIES', 'PS4', 'PS5'
)
$script:SourceGate1FocusedSuites = [ordered]@{
    HST_EnemyCounterattackAutotestSuite = 14
    HST_EnemyGarrisonRebuildAutotestSuite = 13
    HST_EnemyPlanningCommitmentAutotestSuite = 17
    HST_EnemyQRFAutotestSuite = 6
    HST_CampaignProfileJournalAuthorityAutotestSuite = 41
}
$script:SourceGate1PublishScope = @(
    'Assets', 'Configs', 'Missions', 'Prefabs', 'Scripts', 'UI', 'Worlds',
    'addon.gproj', 'thumbnail.png'
)
$script:SourceGate1Utf8 = New-Object Text.UTF8Encoding($false, $true)

function Get-SourceGate1Sha256Bytes {
    param([Parameter(Mandatory = $true)][byte[]]$Bytes)

    $hasher = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString(
            $hasher.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally { $hasher.Dispose() }
}

function Get-SourceGate1Sha256Text {
    param([Parameter(Mandatory = $true)][AllowEmptyString()][string]$Text)

    return Get-SourceGate1Sha256Bytes -Bytes $script:SourceGate1Utf8.GetBytes($Text)
}

function Get-SourceGate1Property {
    param($Value, [Parameter(Mandatory = $true)][string]$Name)

    if ($null -eq $Value) { return $null }
    $properties = @($Value.PSObject.Properties | Where-Object {
        [string]$_.Name -ceq $Name
    })
    if ($properties.Count -eq 0) { return $null }
    if ($properties.Count -ne 1) {
        throw "JSON property '$Name' is not unique with exact case."
    }
    return $properties[0].Value
}

function Assert-SourceGate1ExactProperties {
    param(
        $Value,
        [Parameter(Mandatory = $true)][string[]]$Names,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($null -eq $Value) { throw "$Label is missing." }
    $actual = @($Value.PSObject.Properties | ForEach-Object { [string]$_.Name })
    if (@(Compare-Object -ReferenceObject $Names -DifferenceObject $actual `
            -CaseSensitive).Count -ne 0) {
        throw "$Label does not have the exact property set."
    }
}

function Assert-SourceGate1Boolean {
    param($Value, [bool]$Expected, [string]$Label)

    if ($Value -isnot [bool] -or [bool]$Value -ne $Expected) {
        throw "$Label must be the exact JSON boolean $Expected."
    }
}

function Test-SourceGate1Integer {
    param($Value)

    return $Value -is [byte] -or $Value -is [sbyte] -or
        $Value -is [int16] -or $Value -is [uint16] -or
        $Value -is [int32] -or $Value -is [uint32] -or
        $Value -is [int64] -or $Value -is [uint64]
}

function Assert-SourceGate1Integer {
    param($Value, [long]$Expected, [string]$Label)

    if (-not (Test-SourceGate1Integer $Value) -or [long]$Value -ne $Expected) {
        throw "$Label must be the exact integer $Expected."
    }
}

function Require-SourceGate1Integer {
    param($Value, [long]$Minimum, [string]$Label)

    if (-not (Test-SourceGate1Integer $Value) -or [long]$Value -lt $Minimum) {
        throw "$Label must be a JSON integer greater than or equal to $Minimum."
    }
    return [long]$Value
}

function Require-SourceGate1String {
    param($Value, [string]$Label, [switch]$AllowEmpty)

    if ($Value -isnot [string] -or
        (-not $AllowEmpty -and [string]::IsNullOrWhiteSpace([string]$Value))) {
        throw "$Label must be a JSON string."
    }
    return [string]$Value
}

function Require-SourceGate1Sha256 {
    param($Value, [string]$Label)

    $text = Require-SourceGate1String $Value $Label
    if ($text -cnotmatch '^[0-9a-f]{64}$') {
        throw "$Label must be a lowercase SHA-256 value."
    }
    return $text
}

function Require-SourceGate1GitSha {
    param($Value, [string]$Label)

    $text = Require-SourceGate1String $Value $Label
    if ($text -cnotmatch '^[0-9a-f]{40}$') {
        throw "$Label must be a lowercase full Git SHA."
    }
    return $text
}

function Require-SourceGate1Utc {
    param($Value, [string]$Label)

    $text = Require-SourceGate1String $Value $Label
    if ($text -cnotmatch
        '^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d{1,7})?Z$') {
        throw "$Label must be an ISO-8601 UTC timestamp ending in Z."
    }
    try {
        return [DateTimeOffset]::Parse(
            $text,
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::AssumeUniversal -bor
                [Globalization.DateTimeStyles]::AdjustToUniversal)
    }
    catch { throw "$Label is not a valid UTC timestamp." }
}

function Assert-SourceGate1Array {
    param($Value, [string]$Label)

    if ($Value -isnot [Array]) { throw "$Label must be a JSON array." }
}

function Assert-SourceGate1EmptyArray {
    param($Value, [string]$Label)

    Assert-SourceGate1Array $Value $Label
    if (@($Value).Count -ne 0) { throw "$Label must be empty." }
}

function Assert-SourceGate1PortablePath {
    param([string]$Path, [string]$Label)

    if ([string]::IsNullOrWhiteSpace($Path) -or $Path.Contains('\') -or
        $Path.Contains(':') -or $Path.StartsWith('/') -or $Path.Contains('//') -or
        $Path -cmatch '(^|/)\.\.?(/|$)') {
        throw "$Label is not a normalized portable relative path."
    }
}

function Assert-SourceGate1NoLocalPathText {
    param([string]$Text, [string]$Label)

    if ($Text -match
        '(?i)(?:[a-z]:[\\/]|\\\\|file:(?:/+|\\+)|/(?:Users|home|mnt|tmp|var|etc|opt|root|Volumes)(?:[\\/]|[?#]|$))') {
        throw "$Label contains a machine-local path."
    }
}

function Skip-SourceGate1JsonWhitespace {
    param([string]$Text, $Position)

    while ($Position.Value -lt $Text.Length -and
        [int]$Text[$Position.Value] -in @(0x20, 0x09, 0x0a, 0x0d)) {
        $Position.Value++
    }
}

function Read-SourceGate1JsonString {
    param([string]$Text, $Position, [string]$Label)

    if ($Position.Value -ge $Text.Length -or
        [int]$Text[$Position.Value] -ne 0x22) {
        throw "$Label contains invalid JSON: expected a string."
    }
    $Position.Value++
    $builder = New-Object Text.StringBuilder
    while ($Position.Value -lt $Text.Length) {
        $character = $Text[$Position.Value]
        $Position.Value++
        $code = [int]$character
        if ($code -eq 0x22) { return $builder.ToString() }
        if ($code -lt 0x20) {
            throw "$Label contains an unescaped JSON control character."
        }
        if ($code -ne 0x5c) {
            [void]$builder.Append($character)
            continue
        }
        if ($Position.Value -ge $Text.Length) {
            throw "$Label contains an incomplete JSON escape."
        }
        $escape = [int]$Text[$Position.Value]
        $Position.Value++
        switch ($escape) {
            0x22 { [void]$builder.Append([char]0x22) }
            0x5c { [void]$builder.Append([char]0x5c) }
            0x2f { [void]$builder.Append([char]0x2f) }
            0x62 { [void]$builder.Append([char]0x08) }
            0x66 { [void]$builder.Append([char]0x0c) }
            0x6e { [void]$builder.Append([char]0x0a) }
            0x72 { [void]$builder.Append([char]0x0d) }
            0x74 { [void]$builder.Append([char]0x09) }
            0x75 {
                if ($Position.Value + 4 -gt $Text.Length) {
                    throw "$Label contains an incomplete Unicode escape."
                }
                $hex = $Text.Substring($Position.Value, 4)
                if ($hex -cnotmatch '^[0-9a-fA-F]{4}$') {
                    throw "$Label contains an invalid Unicode escape."
                }
                [void]$builder.Append([char][Convert]::ToUInt16($hex, 16))
                $Position.Value += 4
            }
            default { throw "$Label contains an unsupported JSON escape." }
        }
    }
    throw "$Label contains an unterminated JSON string."
}

function Read-SourceGate1JsonNumber {
    param([string]$Text, $Position, [string]$Label)

    if ($Position.Value -lt $Text.Length -and
        [int]$Text[$Position.Value] -eq 0x2d) { $Position.Value++ }
    if ($Position.Value -ge $Text.Length) {
        throw "$Label contains an incomplete JSON number."
    }
    $first = [int]$Text[$Position.Value]
    if ($first -eq 0x30) {
        $Position.Value++
        if ($Position.Value -lt $Text.Length -and
            [int]$Text[$Position.Value] -ge 0x30 -and
            [int]$Text[$Position.Value] -le 0x39) {
            throw "$Label contains a JSON number with a leading zero."
        }
    }
    elseif ($first -ge 0x31 -and $first -le 0x39) {
        do { $Position.Value++ }
        while ($Position.Value -lt $Text.Length -and
            [int]$Text[$Position.Value] -ge 0x30 -and
            [int]$Text[$Position.Value] -le 0x39)
    }
    else { throw "$Label contains an invalid JSON number." }
    if ($Position.Value -lt $Text.Length -and
        [int]$Text[$Position.Value] -eq 0x2e) {
        $Position.Value++
        $start = $Position.Value
        while ($Position.Value -lt $Text.Length -and
            [int]$Text[$Position.Value] -ge 0x30 -and
            [int]$Text[$Position.Value] -le 0x39) { $Position.Value++ }
        if ($Position.Value -eq $start) {
            throw "$Label contains an incomplete JSON fraction."
        }
    }
    if ($Position.Value -lt $Text.Length -and
        [int]$Text[$Position.Value] -in @(0x45, 0x65)) {
        $Position.Value++
        if ($Position.Value -lt $Text.Length -and
            [int]$Text[$Position.Value] -in @(0x2b, 0x2d)) {
            $Position.Value++
        }
        $start = $Position.Value
        while ($Position.Value -lt $Text.Length -and
            [int]$Text[$Position.Value] -ge 0x30 -and
            [int]$Text[$Position.Value] -le 0x39) { $Position.Value++ }
        if ($Position.Value -eq $start) {
            throw "$Label contains an incomplete JSON exponent."
        }
    }
}

function Read-SourceGate1JsonValue {
    param([string]$Text, $Position, [string]$Label)

    Skip-SourceGate1JsonWhitespace $Text $Position
    if ($Position.Value -ge $Text.Length) {
        throw "$Label contains incomplete JSON."
    }
    $code = [int]$Text[$Position.Value]
    if ($code -eq 0x7b) {
        Read-SourceGate1JsonObject $Text $Position $Label
        return
    }
    if ($code -eq 0x5b) {
        Read-SourceGate1JsonArray $Text $Position $Label
        return
    }
    if ($code -eq 0x22) {
        [void](Read-SourceGate1JsonString $Text $Position $Label)
        return
    }
    foreach ($literal in @('true', 'false', 'null')) {
        if ($Position.Value + $literal.Length -le $Text.Length -and
            $Text.Substring($Position.Value, $literal.Length) -ceq $literal) {
            $Position.Value += $literal.Length
            return
        }
    }
    Read-SourceGate1JsonNumber $Text $Position $Label
}

function Read-SourceGate1JsonObject {
    param([string]$Text, $Position, [string]$Label)

    $Position.Value++
    $names = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    Skip-SourceGate1JsonWhitespace $Text $Position
    if ($Position.Value -lt $Text.Length -and
        [int]$Text[$Position.Value] -eq 0x7d) {
        $Position.Value++
        return
    }
    while ($true) {
        $name = Read-SourceGate1JsonString $Text $Position $Label
        if (-not $names.Add($name)) {
            throw "$Label contains duplicate JSON property '$name'."
        }
        Skip-SourceGate1JsonWhitespace $Text $Position
        if ($Position.Value -ge $Text.Length -or
            [int]$Text[$Position.Value] -ne 0x3a) {
            throw "$Label contains invalid JSON property syntax."
        }
        $Position.Value++
        Read-SourceGate1JsonValue $Text $Position $Label
        Skip-SourceGate1JsonWhitespace $Text $Position
        if ($Position.Value -ge $Text.Length) {
            throw "$Label contains an unterminated JSON object."
        }
        $delimiter = [int]$Text[$Position.Value]
        $Position.Value++
        if ($delimiter -eq 0x7d) { return }
        if ($delimiter -ne 0x2c) {
            throw "$Label contains invalid JSON object punctuation."
        }
        Skip-SourceGate1JsonWhitespace $Text $Position
    }
}

function Read-SourceGate1JsonArray {
    param([string]$Text, $Position, [string]$Label)

    $Position.Value++
    Skip-SourceGate1JsonWhitespace $Text $Position
    if ($Position.Value -lt $Text.Length -and
        [int]$Text[$Position.Value] -eq 0x5d) {
        $Position.Value++
        return
    }
    while ($true) {
        Read-SourceGate1JsonValue $Text $Position $Label
        Skip-SourceGate1JsonWhitespace $Text $Position
        if ($Position.Value -ge $Text.Length) {
            throw "$Label contains an unterminated JSON array."
        }
        $delimiter = [int]$Text[$Position.Value]
        $Position.Value++
        if ($delimiter -eq 0x5d) { return }
        if ($delimiter -ne 0x2c) {
            throw "$Label contains invalid JSON array punctuation."
        }
        Skip-SourceGate1JsonWhitespace $Text $Position
    }
}

function Assert-SourceGate1UniqueJson {
    param([string]$Text, [string]$Label)

    $position = 0
    Read-SourceGate1JsonValue $Text ([ref]$position) $Label
    Skip-SourceGate1JsonWhitespace $Text ([ref]$position)
    if ($position -ne $Text.Length) {
        throw "$Label contains trailing JSON content."
    }
}

function Assert-SourceGate1NoReparseAncestry {
    param([string]$Path, [string]$Label)

    $cursor = [IO.Path]::GetFullPath($Path)
    while (-not [string]::IsNullOrWhiteSpace($cursor)) {
        if (Test-Path -LiteralPath $cursor) {
            $item = Get-Item -LiteralPath $cursor -Force
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "$Label contains a reparse point."
            }
        }
        $parent = Split-Path -Parent $cursor
        if ([string]::IsNullOrWhiteSpace($parent) -or $parent -ceq $cursor) {
            break
        }
        $cursor = $parent
    }
}

function Resolve-SourceGate1ContainedFile {
    param([string]$Root, [string]$PortablePath, [string]$Label)

    Assert-SourceGate1PortablePath $PortablePath $Label
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $resolved = [IO.Path]::GetFullPath(
        (Join-Path $rootFull $PortablePath.Replace('/', '\')))
    $prefix = $rootFull + [IO.Path]::DirectorySeparatorChar
    if (-not $resolved.StartsWith(
            $prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label escaped the repository."
    }
    Assert-SourceGate1NoReparseAncestry $resolved $Label
    if (-not (Test-Path -LiteralPath $resolved -PathType Leaf)) {
        throw "$Label is missing."
    }
    return $resolved
}

function Read-SourceGate1JsonFile {
    param([string]$Path, [string]$Label)

    Assert-SourceGate1NoReparseAncestry $Path $Label
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xef -and
        $bytes[1] -eq 0xbb -and $bytes[2] -eq 0xbf) {
        throw "$Label must be BOM-less UTF-8."
    }
    try { $text = $script:SourceGate1Utf8.GetString($bytes) }
    catch { throw "$Label is not strict BOM-less UTF-8." }
    Assert-SourceGate1UniqueJson $text $Label
    Assert-SourceGate1NoLocalPathText $text $Label
    try { $value = $text | ConvertFrom-Json }
    catch { throw "$Label is not valid JSON." }
    return [pscustomobject][ordered]@{
        Value = $value
        Text = $text
        Bytes = $bytes
        Length = [long]$bytes.LongLength
        Sha256 = Get-SourceGate1Sha256Bytes -Bytes $bytes
    }
}

function Invoke-SourceGate1Git {
    param(
        [string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [switch]$AllowFailure
    )

    $output = @(& git -C $RepositoryRoot @Arguments 2>&1)
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0 -and -not $AllowFailure) {
        throw ('Git failed: git ' + ($Arguments -join ' ') + ' | ' +
            (($output | ForEach-Object { [string]$_ }) -join ' '))
    }
    return [pscustomobject][ordered]@{
        ExitCode = $exitCode
        Lines = @($output | ForEach-Object { [string]$_ })
    }
}

function ConvertTo-SourceGate1NativeArgument {
    param([AllowEmptyString()][string]$Value)

    if ($Value.Length -gt 0 -and $Value -notmatch '[\s"]') { return $Value }
    return '"' + $Value.Replace('\', '\').Replace('"', '\"') + '"'
}

function Get-SourceGate1GitBlobBytes {
    param(
        [string]$RepositoryRoot,
        [string]$Revision,
        [string]$PortablePath
    )

    Assert-SourceGate1PortablePath $PortablePath 'runnerPath'
    $git = (Get-Command git -ErrorAction Stop).Source
    $start = New-Object Diagnostics.ProcessStartInfo
    $start.FileName = $git
    $start.WorkingDirectory = [IO.Path]::GetFullPath($RepositoryRoot)
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $start.Arguments = 'cat-file blob ' +
        (ConvertTo-SourceGate1NativeArgument ($Revision + ':' + $PortablePath))
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $start
    $memory = New-Object IO.MemoryStream
    try {
        if (-not $process.Start()) { throw 'Git blob reader did not start.' }
        $process.StandardOutput.BaseStream.CopyTo($memory)
        $errorText = $process.StandardError.ReadToEnd()
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) {
            throw "Git could not read the committed runner blob: $errorText"
        }
        return $memory.ToArray()
    }
    finally {
        $memory.Dispose()
        $process.Dispose()
    }
}

function Test-SourceGate1GitAncestor {
    param([string]$RepositoryRoot, [string]$Ancestor, [string]$Descendant)

    $result = Invoke-SourceGate1Git `
        -RepositoryRoot $RepositoryRoot `
        -Arguments @('merge-base', '--is-ancestor', $Ancestor, $Descendant) `
        -AllowFailure
    return $result.ExitCode -eq 0
}

function Get-SourceGate1PublishInputBinding {
    param([string]$RepositoryRoot, [string]$Revision)

    $arguments = @('ls-tree', '-r', '--full-tree', $Revision, '--') +
        $script:SourceGate1PublishScope
    $gitResult = Invoke-SourceGate1Git `
        -RepositoryRoot $RepositoryRoot `
        -Arguments $arguments
    $rowsByPath = New-Object `
        'Collections.Generic.Dictionary[string,object]' `
        ([StringComparer]::Ordinal)
    $caseInsensitive = New-Object `
        'Collections.Generic.HashSet[string]' `
        ([StringComparer]::OrdinalIgnoreCase)
    foreach ($line in $gitResult.Lines) {
        $match = [Regex]::Match(
            [string]$line,
            '^(?<mode>[0-9]{6})\s+(?<type>blob)\s+(?<oid>[0-9a-f]{40})\t(?<path>.+)$')
        if (-not $match.Success) {
            throw 'The source publish-input tree contains a malformed row.'
        }
        $path = $match.Groups['path'].Value.Replace('\', '/')
        if ($rowsByPath.ContainsKey($path) -or -not $caseInsensitive.Add($path)) {
            throw 'The source publish-input tree contains a duplicate path.'
        }
        $rowsByPath.Add(
            $path,
            [pscustomobject][ordered]@{
                Path = $path
                Oid = $match.Groups['oid'].Value
                Canonical = $match.Groups['mode'].Value + ' ' +
                    $match.Groups['type'].Value + ' ' +
                    $match.Groups['oid'].Value + "`t" + $path
            })
    }
    [string[]]$paths = @($rowsByPath.Keys)
    [Array]::Sort($paths, [StringComparer]::Ordinal)
    if ($paths.Count -eq 0 -or $paths -cnotcontains 'addon.gproj') {
        throw 'The source publish-input tree is empty or missing addon.gproj.'
    }
    $rows = @($paths | ForEach-Object { $rowsByPath[$_] })
    $canonical = (@($rows | ForEach-Object { $_.Canonical }) -join "`n") + "`n"
    return [pscustomobject][ordered]@{
        Policy = 'git-ls-tree-sha256-v1'
        RowCount = $paths.Count
        Sha256 = Get-SourceGate1Sha256Text -Text $canonical
        Rows = $rows
    }
}

function Assert-SourceGate1PublishWorktree {
    param([string]$RepositoryRoot, $PublishBinding)

    $pathspecs = $script:SourceGate1PublishScope
    $index = Invoke-SourceGate1Git `
        -RepositoryRoot $RepositoryRoot `
        -Arguments (@('ls-files', '-v', '--') + $pathspecs)
    if ($index.Lines.Count -ne $PublishBinding.RowCount -or
        @($index.Lines | Where-Object { $_ -cnotmatch '^H .+' }).Count -ne 0) {
        throw ('The publish worktree contains an assume-unchanged, skip-worktree, ' +
            'or index-shape mismatch.')
    }
    foreach ($arguments in @(
            (@('ls-files', '--others', '--exclude-standard', '--') + $pathspecs),
            (@('ls-files', '--others', '--ignored', '--exclude-standard', '--') +
                $pathspecs))) {
        $extras = Invoke-SourceGate1Git `
            -RepositoryRoot $RepositoryRoot `
            -Arguments $arguments
        if ($extras.Lines.Count -ne 0) {
            throw 'The publish worktree contains an untracked or ignored extra.'
        }
    }
    $paths = [string[]]@($PublishBinding.Rows | ForEach-Object { $_.Path })
    foreach ($path in $paths) {
        $resolved = Resolve-SourceGate1ContainedFile `
            -Root $RepositoryRoot `
            -PortablePath $path `
            -Label 'publish worktree input'
        Assert-SourceGate1NoReparseAncestry $resolved 'publish worktree input'
    }
    $hashOutput = @($paths | & git -C $RepositoryRoot hash-object --stdin-paths 2>&1)
    if ($LASTEXITCODE -ne 0 -or $hashOutput.Count -ne $paths.Count) {
        throw 'The publish worktree could not be hashed as one exact file set.'
    }
    for ($indexNumber = 0; $indexNumber -lt $paths.Count; $indexNumber++) {
        if ([string]$hashOutput[$indexNumber] -cne
            [string]$PublishBinding.Rows[$indexNumber].Oid) {
            throw 'A publish worktree input differs from the frozen source Git blob.'
        }
    }

    $extension = 'p' + 'ak'
    $archivePathspecs = @(
        ":(icase,glob)**/*.$extension",
        ":(icase,glob)*.$extension")
    foreach ($arguments in @(
            (@('ls-files', '--cached', '--') + $archivePathspecs),
            (@('ls-files', '--others', '--exclude-standard', '--') +
                $archivePathspecs),
            (@('ls-files', '--others', '--ignored', '--exclude-standard', '--') +
                $archivePathspecs))) {
        $archiveRows = Invoke-SourceGate1Git `
            -RepositoryRoot $RepositoryRoot `
            -Arguments $arguments
        if ($archiveRows.Lines.Count -ne 0) {
            throw 'The source checkout contains a generated archive.'
        }
    }
}

function Assert-SourceGate1Checkpoint {
    param($Checkpoint, [string]$RepositoryRoot)

    Assert-SourceGate1ExactProperties $Checkpoint @(
        'sourceGitHead', 'frozenUtc', 'dirty', 'publishInputPolicy',
        'publishInputTreeSha256', 'publishInputRowCount', 'addonGuid',
        'campaignSchema', 'runtimeSettingsSchema',
        'embeddedImplementationSha', 'embeddedBuildUtc', 'embeddedBuildLabel'
    ) 'gate1Source.checkpoint'
    $sourceHead = Require-SourceGate1GitSha `
        $Checkpoint.sourceGitHead 'checkpoint.sourceGitHead'
    [void](Require-SourceGate1Utc $Checkpoint.frozenUtc 'checkpoint.frozenUtc')
    Assert-SourceGate1Boolean $Checkpoint.dirty $false 'checkpoint.dirty'
    if ([string]$Checkpoint.publishInputPolicy -cne 'git-ls-tree-sha256-v1') {
        throw 'checkpoint.publishInputPolicy is not supported.'
    }
    $treeSha = Require-SourceGate1Sha256 `
        $Checkpoint.publishInputTreeSha256 'checkpoint.publishInputTreeSha256'
    $rowCount = Require-SourceGate1Integer `
        $Checkpoint.publishInputRowCount 1 'checkpoint.publishInputRowCount'
    if ($Checkpoint.addonGuid -isnot [string] -or
        [string]$Checkpoint.addonGuid -cnotmatch '^[0-9A-F]{16}$') {
        throw 'checkpoint.addonGuid is not exact.'
    }
    [void](Require-SourceGate1Integer `
        $Checkpoint.campaignSchema 1 'checkpoint.campaignSchema')
    [void](Require-SourceGate1Integer `
        $Checkpoint.runtimeSettingsSchema 1 'checkpoint.runtimeSettingsSchema')
    [void](Require-SourceGate1GitSha `
        $Checkpoint.embeddedImplementationSha 'checkpoint.embeddedImplementationSha')
    [void](Require-SourceGate1Utc `
        $Checkpoint.embeddedBuildUtc 'checkpoint.embeddedBuildUtc')
    [void](Require-SourceGate1String `
        $Checkpoint.embeddedBuildLabel 'checkpoint.embeddedBuildLabel')

    $resolved = Invoke-SourceGate1Git `
        -RepositoryRoot $RepositoryRoot `
        -Arguments @('rev-parse', "$sourceHead^{commit}")
    if ($resolved.Lines.Count -ne 1 -or $resolved.Lines[0].Trim() -cne $sourceHead) {
        throw 'The source checkpoint does not resolve to its exact commit.'
    }
    $currentHead = (Invoke-SourceGate1Git `
        -RepositoryRoot $RepositoryRoot `
        -Arguments @('rev-parse', 'HEAD')).Lines[0].Trim()
    if (-not (Test-SourceGate1GitAncestor `
            -RepositoryRoot $RepositoryRoot `
            -Ancestor $sourceHead `
            -Descendant $currentHead)) {
        throw 'The source checkpoint is not an ancestor of the current checkout.'
    }
    $actual = Get-SourceGate1PublishInputBinding `
        -RepositoryRoot $RepositoryRoot `
        -Revision $sourceHead
    if ($actual.Policy -cne [string]$Checkpoint.publishInputPolicy -or
        $actual.RowCount -ne $rowCount -or $actual.Sha256 -cne $treeSha) {
        throw 'The source checkpoint publish-input binding does not match Git.'
    }
    Assert-SourceGate1PublishWorktree `
        -RepositoryRoot $RepositoryRoot `
        -PublishBinding $actual
    return [pscustomobject][ordered]@{
        SourceGitHead = $sourceHead
        PublishInputPolicy = $actual.Policy
        PublishInputTreeSha256 = $actual.Sha256
        PublishInputRowCount = $actual.RowCount
        AddonGuid = [string]$Checkpoint.addonGuid
        CampaignSchema = [int]$Checkpoint.campaignSchema
        RuntimeSettingsSchema = [int]$Checkpoint.runtimeSettingsSchema
        EmbeddedImplementationSha = [string]$Checkpoint.embeddedImplementationSha
        EmbeddedBuildUtc = [string]$Checkpoint.embeddedBuildUtc
        EmbeddedBuildLabel = [string]$Checkpoint.embeddedBuildLabel
    }
}

function Assert-SourceGate1SourceBinding {
    param($Source, $Checkpoint, [string]$EvidenceKey, [string]$Label)

    $properties = @(
        'sourceGitHead', 'publishInputPolicy', 'publishInputTreeSha256',
        'publishInputRowCount', 'addonGuid', 'campaignSchema',
        'runtimeSettingsSchema', 'embeddedImplementationSha',
        'embeddedBuildUtc', 'embeddedBuildLabel')
    $campaign = $EvidenceKey -in @(
        'forceAuthorityCanary', 'fullCampaignDebug')
    if ($campaign) {
        $properties += @(
            'executedPublishInputTreeSha256',
            'executedPublishInputRowCount',
            'sourceResourceDatabase')
    }
    Assert-SourceGate1ExactProperties $Source $properties $Label
    $comparisons = [ordered]@{
        sourceGitHead = $Checkpoint.SourceGitHead
        publishInputPolicy = $Checkpoint.PublishInputPolicy
        publishInputTreeSha256 = $Checkpoint.PublishInputTreeSha256
        publishInputRowCount = $Checkpoint.PublishInputRowCount
        addonGuid = $Checkpoint.AddonGuid
        campaignSchema = $Checkpoint.CampaignSchema
        runtimeSettingsSchema = $Checkpoint.RuntimeSettingsSchema
        embeddedImplementationSha = $Checkpoint.EmbeddedImplementationSha
        embeddedBuildUtc = $Checkpoint.EmbeddedBuildUtc
        embeddedBuildLabel = $Checkpoint.EmbeddedBuildLabel
    }
    foreach ($name in $comparisons.Keys) {
        if ([string]$Source.$name -cne [string]$comparisons[$name]) {
            throw "$Label.$name differs from the frozen source checkpoint."
        }
    }
    if ($campaign) {
        if ([string]$Source.executedPublishInputTreeSha256 -cne
                $Checkpoint.PublishInputTreeSha256 -or
            [long]$Source.executedPublishInputRowCount -ne
                $Checkpoint.PublishInputRowCount) {
            throw "$Label executed publish-input binding differs from the checkpoint."
        }
        [void](Assert-SourceGate1ResourceDatabaseIdentity `
            $Source.sourceResourceDatabase "$Label.sourceResourceDatabase")
    }
}

function Assert-SourceGate1HarnessBinding {
    param(
        $Harness,
        [string]$EvidenceKey,
        $Checkpoint,
        [string]$RepositoryRoot
    )

    $expectedPath = [string]$script:SourceGate1RunnerPaths[$EvidenceKey]
    $focused = $EvidenceKey -ceq 'focusedFiveSuite'
    $properties = @(
        'gitHead', 'dirty', 'runnerPath', 'runnerHashPolicy', 'runnerSha256')
    if ($focused) { $properties += 'runnerGitBlobOid' }
    Assert-SourceGate1ExactProperties $Harness $properties "$EvidenceKey.harness"
    $harnessHead = Require-SourceGate1GitSha `
        $Harness.gitHead "$EvidenceKey.harness.gitHead"
    Assert-SourceGate1Boolean `
        $Harness.dirty $false "$EvidenceKey.harness.dirty"
    if ([string]$Harness.runnerPath -cne $expectedPath) {
        throw "$EvidenceKey uses the wrong committed runner."
    }
    Assert-SourceGate1PortablePath `
        ([string]$Harness.runnerPath) "$EvidenceKey.harness.runnerPath"
    $policy = [string]$Harness.runnerHashPolicy
    $expectedPolicy = if ($focused) {
        'normalized-utf8-lf-sha256-v1'
    }
    else { 'sha256-git-blob-bytes-v1' }
    if ($policy -cne $expectedPolicy) {
        throw "$EvidenceKey uses an unsupported runner hash policy."
    }
    $runnerSha = Require-SourceGate1Sha256 `
        $Harness.runnerSha256 "$EvidenceKey.harness.runnerSha256"
    $currentHead = (Invoke-SourceGate1Git `
        -RepositoryRoot $RepositoryRoot `
        -Arguments @('rev-parse', 'HEAD')).Lines[0].Trim()
    if (-not (Test-SourceGate1GitAncestor `
            -RepositoryRoot $RepositoryRoot `
            -Ancestor $Checkpoint.SourceGitHead `
            -Descendant $harnessHead) -or
        -not (Test-SourceGate1GitAncestor `
            -RepositoryRoot $RepositoryRoot `
            -Ancestor $harnessHead `
            -Descendant $currentHead)) {
        throw "$EvidenceKey harness ancestry is invalid."
    }
    $blobOid = (Invoke-SourceGate1Git `
        -RepositoryRoot $RepositoryRoot `
        -Arguments @('rev-parse', ($harnessHead + ':' + $expectedPath))).Lines[0].Trim()
    if ($blobOid -cnotmatch '^[0-9a-f]{40}$') {
        throw "$EvidenceKey runner is not a committed Git blob."
    }
    if ($focused -and [string]$Harness.runnerGitBlobOid -cne $blobOid) {
        throw "$EvidenceKey runner Git blob OID is inconsistent."
    }
    $blobBytes = Get-SourceGate1GitBlobBytes `
        -RepositoryRoot $RepositoryRoot `
        -Revision $harnessHead `
        -PortablePath $expectedPath
    $actualSha = if ($focused) {
        try { $text = $script:SourceGate1Utf8.GetString($blobBytes) }
        catch { throw 'The focused runner Git blob is not strict UTF-8.' }
        Get-SourceGate1Sha256Text `
            -Text $text.Replace("`r`n", "`n").Replace("`r", "`n")
    }
    else { Get-SourceGate1Sha256Bytes -Bytes $blobBytes }
    if ($actualSha -cne $runnerSha) {
        throw "$EvidenceKey committed runner SHA-256 is inconsistent."
    }
    return [pscustomobject][ordered]@{
        GitHead = $harnessHead
        RunnerPath = $expectedPath
        RunnerHashPolicy = $policy
        RunnerSha256 = $runnerSha
    }
}

function Assert-SourceGate1Integrity {
    param($Integrity, [string]$Label)

    Assert-SourceGate1ExactProperties $Integrity @(
        'hashAlgorithm', 'artifactCount', 'artifactSetSha256', 'files') $Label
    if ([string]$Integrity.hashAlgorithm -cne 'sha256-file-set-v1') {
        throw "$Label uses an unsupported hash algorithm."
    }
    $count = Require-SourceGate1Integer `
        $Integrity.artifactCount 1 "$Label.artifactCount"
    $setSha = Require-SourceGate1Sha256 `
        $Integrity.artifactSetSha256 "$Label.artifactSetSha256"
    Assert-SourceGate1Array $Integrity.files "$Label.files"
    $files = @($Integrity.files)
    if ($files.Count -ne $count) {
        throw "$Label artifact count does not match its file rows."
    }
    $seen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $seenIgnoreCase = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::OrdinalIgnoreCase)
    $previous = $null
    $canonical = New-Object Collections.Generic.List[string]
    foreach ($file in $files) {
        Assert-SourceGate1ExactProperties $file @('path', 'length', 'sha256') `
            "$Label.files[]"
        $path = Require-SourceGate1String $file.path "$Label.files[].path"
        Assert-SourceGate1PortablePath $path "$Label.files[].path"
        if (-not $seen.Add($path) -or -not $seenIgnoreCase.Add($path)) {
            throw "$Label contains a duplicate artifact path."
        }
        if ($null -ne $previous -and
            [StringComparer]::Ordinal.Compare([string]$previous, $path) -ge 0) {
            throw "$Label artifact paths are not in canonical ordinal order."
        }
        $length = Require-SourceGate1Integer `
            $file.length 0 "$Label.files[].length"
        $sha = Require-SourceGate1Sha256 `
            $file.sha256 "$Label.files[].sha256"
        [void]$canonical.Add($path + "`t" + $length + "`t" + $sha)
        $previous = $path
    }
    $canonicalText = ($canonical.ToArray() -join "`n") + "`n"
    if ((Get-SourceGate1Sha256Text -Text $canonicalText) -cne $setSha) {
        throw "$Label artifact-set SHA-256 is not canonical."
    }
    return [pscustomobject][ordered]@{
        ArtifactCount = [int]$count
        ArtifactSetSha256 = $setSha
    }
}

function Assert-SourceGate1ResourceDatabaseIdentity {
    param($Identity, [string]$Label)

    if ($null -eq $Identity) { throw "$Label is missing." }
    $length = Require-SourceGate1Integer $Identity.length 1 "$Label.length"
    $sha = Require-SourceGate1Sha256 $Identity.sha256 "$Label.sha256"
    return [pscustomobject][ordered]@{
        Length = $length
        Sha256 = $sha
    }
}

function Assert-SourceGate1ResourceDatabaseMatch {
    param($Expected, $Actual, [string]$Label)

    if ($null -eq $Expected) { return $Actual }
    if ($Expected.Length -ne $Actual.Length -or
        $Expected.Sha256 -cne $Actual.Sha256) {
        throw ("$Label does not match the prior Gate 1 resource database " +
            "($($Expected.Length)/$($Expected.Sha256) versus " +
            "$($Actual.Length)/$($Actual.Sha256)).")
    }
    return $Expected
}

function Assert-SourceGate1FoundationResult {
    param($Summary, [string]$ReferenceStatus)

    $result = $Summary.result
    if ($ReferenceStatus -ceq 'failed') {
        if ([string]$result.status -cne 'failed' -or
            $result.success -isnot [bool] -or [bool]$result.success) {
            throw 'The failed Foundation reference is inconsistent.'
        }
        return $null
    }
    Assert-SourceGate1ExactProperties $result @(
        'status', 'success', 'exitCode', 'completionMarker', 'referenceCount',
        'trackedPakCount', 'workspacePakCount',
        'verifiedTrackedPublishInputCount',
        'extraUntrackedOrIgnoredPublishInputCount',
        'sourceAndHarnessStable', 'failureCodes') 'foundation.result'
    if ([string]$result.status -cne 'passed') {
        throw 'Foundation result status is not passed.'
    }
    Assert-SourceGate1Boolean $result.success $true 'foundation.result.success'
    Assert-SourceGate1Integer $result.exitCode 0 'foundation.result.exitCode'
    Assert-SourceGate1Boolean `
        $result.completionMarker $true 'foundation.result.completionMarker'
    [void](Require-SourceGate1Integer `
        $result.referenceCount 1 'foundation.result.referenceCount')
    Assert-SourceGate1Integer `
        $result.trackedPakCount 0 'foundation.result.trackedPakCount'
    Assert-SourceGate1Integer `
        $result.workspacePakCount 0 'foundation.result.workspacePakCount'
    Assert-SourceGate1Integer `
        $result.verifiedTrackedPublishInputCount `
        ([long]$Summary.source.publishInputRowCount) `
        'foundation.result.verifiedTrackedPublishInputCount'
    Assert-SourceGate1Integer `
        $result.extraUntrackedOrIgnoredPublishInputCount 0 `
        'foundation.result.extraUntrackedOrIgnoredPublishInputCount'
    Assert-SourceGate1Boolean `
        $result.sourceAndHarnessStable $true `
        'foundation.result.sourceAndHarnessStable'
    Assert-SourceGate1EmptyArray `
        $result.failureCodes 'foundation.result.failureCodes'
    return $null
}

function Assert-SourceGate1WorkbenchResult {
    param($Summary, [string]$ReferenceStatus)

    $result = $Summary.result
    if ($ReferenceStatus -ceq 'failed') {
        if ([string]$result.status -cne 'failed' -or
            $result.success -isnot [bool] -or [bool]$result.success) {
            throw 'The failed Workbench reference is inconsistent.'
        }
        return $null
    }
    Assert-SourceGate1ExactProperties $result @(
        'status', 'success', 'targetCount', 'passedTargets',
        'exactRequiredTargets', 'commonCrc', 'positiveFileAndClassCounts',
        'hardErrorCount', 'cleanupAndSpillZeroTargets', 'pakCensusZeroTargets',
        'sourceResourceDatabaseStableTargets',
        'sourceResourceDatabaseFinalStable', 'sourceAndHarnessStable',
        'failureCodes') 'workbench.result'
    if ([string]$result.status -cne 'passed') {
        throw 'Workbench result status is not passed.'
    }
    Assert-SourceGate1Boolean $result.success $true 'workbench.result.success'
    foreach ($name in @(
            'targetCount', 'passedTargets', 'cleanupAndSpillZeroTargets',
            'pakCensusZeroTargets', 'sourceResourceDatabaseStableTargets')) {
        Assert-SourceGate1Integer $result.$name 5 "workbench.result.$name"
    }
    foreach ($name in @(
            'exactRequiredTargets', 'positiveFileAndClassCounts',
            'sourceResourceDatabaseFinalStable', 'sourceAndHarnessStable')) {
        Assert-SourceGate1Boolean $result.$name $true "workbench.result.$name"
    }
    if ($result.commonCrc -isnot [string] -or
        [string]$result.commonCrc -cnotmatch '^[0-9a-f]{8}$') {
        throw 'workbench.result.commonCrc is invalid.'
    }
    Assert-SourceGate1Integer `
        $result.hardErrorCount 0 'workbench.result.hardErrorCount'
    Assert-SourceGate1EmptyArray `
        $result.failureCodes 'workbench.result.failureCodes'
    Assert-SourceGate1Array $Summary.capture.targets 'workbench.capture.targets'
    $targets = @($Summary.capture.targets)
    if ($targets.Count -ne 5) { throw 'Workbench target count is not exact.' }
    $resourceIdentity = Assert-SourceGate1ResourceDatabaseIdentity `
        $Summary.toolchain.sourceResourceDatabase `
        'workbench.toolchain.sourceResourceDatabase'
    for ($index = 0; $index -lt 5; $index++) {
        $row = $targets[$index]
        if ([string]$row.target -cne $script:SourceGate1WorkbenchTargets[$index]) {
            throw 'Workbench targets are not in the exact required order.'
        }
        foreach ($name in @('success', 'cleanupAndSpillZero', 'pakCensusZero',
                'sourceResourceDatabaseStable')) {
            Assert-SourceGate1Boolean $row.$name $true "workbench target $name"
        }
        Assert-SourceGate1Integer $row.exitCode 0 'workbench target exitCode'
        [void](Require-SourceGate1Integer $row.files 1 'workbench target files')
        [void](Require-SourceGate1Integer $row.classes 1 'workbench target classes')
        Assert-SourceGate1Integer `
            $row.hardErrorCount 0 'workbench target hardErrorCount'
        if ([string]$row.crc -cne [string]$result.commonCrc) {
            throw 'Workbench target CRC differs from the common CRC.'
        }
        $targetResource = Assert-SourceGate1ResourceDatabaseIdentity `
            $row.sourceResourceDatabase 'workbench target sourceResourceDatabase'
        [void](Assert-SourceGate1ResourceDatabaseMatch `
            $resourceIdentity $targetResource 'Workbench target')
    }
    return $resourceIdentity
}

function Assert-SourceGate1FocusedResult {
    param($Summary, [string]$ReferenceStatus)

    $result = $Summary.result
    if ($ReferenceStatus -ceq 'failed') {
        if ([string]$result.status -cne 'failed' -or
            $result.success -isnot [bool] -or [bool]$result.success) {
            throw 'The failed focused-suite reference is inconsistent.'
        }
        return $null
    }
    Assert-SourceGate1ExactProperties $result @(
        'status', 'success', 'noncertifying', 'expectedSuiteCount',
        'completedSuiteCount', 'passedSuiteCount', 'expectedTestCount',
        'tests', 'failures', 'errors', 'skipped', 'sourceBindingsStable',
        'sourceIsHarnessAncestor', 'publishInputsMatchHarness',
        'finalEngineProcessCount', 'suiteDefinitions', 'suites', 'error'
    ) 'focused.result'
    if ([string]$result.status -cne 'passed-noncertifying') {
        throw 'Focused-suite result status is not passed-noncertifying.'
    }
    foreach ($name in @(
            'success', 'noncertifying', 'sourceBindingsStable',
            'sourceIsHarnessAncestor', 'publishInputsMatchHarness')) {
        Assert-SourceGate1Boolean $result.$name $true "focused.result.$name"
    }
    foreach ($name in @(
            'expectedSuiteCount', 'completedSuiteCount', 'passedSuiteCount')) {
        Assert-SourceGate1Integer $result.$name 5 "focused.result.$name"
    }
    foreach ($name in @('expectedTestCount', 'tests')) {
        Assert-SourceGate1Integer $result.$name 91 "focused.result.$name"
    }
    foreach ($name in @('failures', 'errors', 'skipped',
            'finalEngineProcessCount')) {
        Assert-SourceGate1Integer $result.$name 0 "focused.result.$name"
    }
    if ($null -ne $result.error -and [string]$result.error -cne '') {
        throw 'focused.result.error is not empty.'
    }
    Assert-SourceGate1Array `
        $result.suiteDefinitions 'focused.result.suiteDefinitions'
    Assert-SourceGate1Array $result.suites 'focused.result.suites'
    $definitions = @($result.suiteDefinitions)
    $suites = @($result.suites)
    if ($definitions.Count -ne 5 -or $suites.Count -ne 5) {
        throw 'Focused-suite manifest does not contain exactly five suites.'
    }
    $suiteNames = @($script:SourceGate1FocusedSuites.Keys)
    for ($index = 0; $index -lt 5; $index++) {
        $expectedName = [string]$suiteNames[$index]
        $expectedCount = [int]$script:SourceGate1FocusedSuites[$expectedName]
        $definition = $definitions[$index]
        $suite = $suites[$index]
        if ([string]$definition.suite -cne $expectedName -or
            [string]$suite.suite -cne $expectedName) {
            throw 'Focused suites are not in the exact required order.'
        }
        Assert-SourceGate1Integer `
            $definition.expectedCaseCount $expectedCount `
            'focused suite definition expectedCaseCount'
        Assert-SourceGate1Integer `
            $suite.expectedCaseCount $expectedCount `
            'focused suite expectedCaseCount'
        if ([string]$suite.status -cne 'passed') {
            throw 'A focused suite is not passed.'
        }
        Assert-SourceGate1Boolean $suite.success $true 'focused suite success'
        Assert-SourceGate1Boolean `
            $suite.sourceBindingsStable $true 'focused suite sourceBindingsStable'
        Assert-SourceGate1Integer `
            $suite.failedListBytes 0 'focused suite failedListBytes'
        Assert-SourceGate1Boolean $suite.junit.valid $true 'focused suite junit.valid'
        Assert-SourceGate1Integer `
            $suite.junit.tests $expectedCount 'focused suite junit.tests'
        foreach ($name in @('failures', 'errors', 'skipped')) {
            Assert-SourceGate1Integer $suite.junit.$name 0 "focused suite junit.$name"
        }
        Assert-SourceGate1Boolean `
            $suite.sourceMount.valid $true 'focused suite sourceMount.valid'
        Assert-SourceGate1Integer `
            $suite.sourceMount.sourcePackedRecordCount 0 `
            'focused suite sourcePackedRecordCount'
        Assert-SourceGate1Integer `
            $suite.sourceMount.sourcePackageRecordCount 0 `
            'focused suite sourcePackageRecordCount'
        Assert-SourceGate1Boolean `
            $suite.sourceMount.resourceDatabaseLoadExact $true `
            'focused suite resourceDatabaseLoadExact'
        Assert-SourceGate1ExactProperties $suite.profileMount @(
            'policy', 'profileArgument', 'resolvedProfilePath',
            'proofAuthorizationRequired', 'proofAuthorizationEstablished',
            'proofSentinelStableThroughExit', 'proofSentinelPath',
            'proofSentinelValueSha256'
        ) 'focused suite profileMount'
        if ([string]$suite.profileMount.policy -cne
                'profile-argument-parent-mount-v1' -or
            [string]$suite.profileMount.profileArgument -cne
                '<suite-scratch>' -or
            [string]$suite.profileMount.resolvedProfilePath -cne
                '<suite-scratch>/profile') {
            throw 'A focused suite has an invalid portable profile mount.'
        }
        $profileProofSuite = $expectedName -ceq
            'HST_CampaignProfileJournalAuthorityAutotestSuite'
        Assert-SourceGate1Boolean `
            $suite.profileMount.proofAuthorizationRequired `
            $profileProofSuite `
            'focused suite proofAuthorizationRequired'
        Assert-SourceGate1Boolean `
            $suite.profileMount.proofAuthorizationEstablished `
            $profileProofSuite `
            'focused suite proofAuthorizationEstablished'
        Assert-SourceGate1Boolean `
            $suite.profileMount.proofSentinelStableThroughExit $true `
            'focused suite proofSentinelStableThroughExit'
        $expectedSentinelPath = if ($profileProofSuite) {
            '<suite-scratch>/profile/.partisan-focused-owner'
        }
        else { '' }
        $expectedSentinelSha = if ($profileProofSuite) {
            Get-SourceGate1Sha256Text 'owned'
        }
        else { '' }
        if ([string]$suite.profileMount.proofSentinelPath -cne
                $expectedSentinelPath -or
            [string]$suite.profileMount.proofSentinelValueSha256 -cne
                $expectedSentinelSha) {
            throw 'A focused suite has an invalid profile proof authorization.'
        }
    }
    return Assert-SourceGate1ResourceDatabaseIdentity `
        $Summary.toolchain.sourceResourceDatabase `
        'focused.toolchain.sourceResourceDatabase'
}

function Assert-SourceGate1ExactStringSet {
    param([string[]]$Expected, $Actual, [string]$Label)

    Assert-SourceGate1Array $Actual $Label
    $rows = @($Actual | ForEach-Object { [string]$_ })
    if ($rows.Count -ne $Expected.Count -or
        @($rows | Group-Object -CaseSensitive | Where-Object Count -ne 1).Count -ne 0) {
        throw "$Label is not the exact required set."
    }
    foreach ($expectedValue in $Expected) {
        if ($rows -cnotcontains $expectedValue) {
            throw "$Label is not the exact required set."
        }
    }
}

function Assert-SourceGate1CampaignCommon {
    param($Summary, [string]$EvidenceKey)

    Assert-SourceGate1Boolean `
        $Summary.result.sourceStable $true "$EvidenceKey.result.sourceStable"
    Assert-SourceGate1Boolean `
        $Summary.result.sourceResourceDatabaseStable $true `
        "$EvidenceKey.result.sourceResourceDatabaseStable"
    Assert-SourceGate1Boolean `
        $Summary.result.settings.stable $true "$EvidenceKey.result.settings.stable"
    Assert-SourceGate1Integer `
        $Summary.result.settings.schemaVersion `
        ([long]$Summary.source.runtimeSettingsSchema) `
        "$EvidenceKey.result.settings.schemaVersion"
    foreach ($name in @(
            'argumentTokensVerified', 'armed', 'started', 'artifactSetExact',
            'artifactBytesStableAfterShutdown')) {
        Assert-SourceGate1Boolean `
            $Summary.result.launch.$name $true "$EvidenceKey.result.launch.$name"
    }
    $mount = $Summary.result.mountAttestation
    Assert-SourceGate1Boolean $mount.valid $true "$EvidenceKey mount valid"
    Assert-SourceGate1Boolean `
        $mount.everyProjectRecordExact $true "$EvidenceKey mount project records"
    Assert-SourceGate1Integer `
        $mount.generatedArchiveMatchCount 0 "$EvidenceKey generated archive count"
    Assert-SourceGate1Integer `
        $mount.sourceResourceDatabaseRecordCount 1 `
        "$EvidenceKey source resource database record count"
    Assert-SourceGate1Boolean `
        $mount.everyResourceDatabaseRecordExact $true `
        "$EvidenceKey source resource database record shape"
    $process = $Summary.result.processCensus
    Assert-SourceGate1EmptyArray $process.before "$EvidenceKey process before"
    Assert-SourceGate1Integer `
        $process.ownedProcessesRemaining 0 "$EvidenceKey owned process residue"
    Assert-SourceGate1EmptyArray `
        $process.unclaimedEngineProcessesObserved `
        "$EvidenceKey unclaimed process observations"
    Assert-SourceGate1EmptyArray $process.after "$EvidenceKey process after"
    Assert-SourceGate1EmptyArray `
        $process.cleanupErrors "$EvidenceKey process cleanup errors"
    Assert-SourceGate1EmptyArray `
        $Summary.result.failureReasons "$EvidenceKey failureReasons"
    $artifact = $Summary.result.artifactValidation
    Assert-SourceGate1Boolean $artifact.Valid $true "$EvidenceKey artifact.Valid"
    Assert-SourceGate1Boolean `
        $artifact.SourceArtifactContract $true `
        "$EvidenceKey artifact.SourceArtifactContract"
    Assert-SourceGate1Boolean `
        $artifact.StateDiffManifestExact $true `
        "$EvidenceKey artifact.StateDiffManifestExact"
    Assert-SourceGate1Integer `
        $artifact.StateDiffRows 18 "$EvidenceKey artifact.StateDiffRows"
    Assert-SourceGate1Integer `
        $artifact.NonzeroStateDiffRows 0 `
        "$EvidenceKey artifact.NonzeroStateDiffRows"
    Assert-SourceGate1Boolean `
        $artifact.FinalOrphanCleanupPass $true `
        "$EvidenceKey artifact.FinalOrphanCleanupPass"
    Assert-SourceGate1Integer `
        $artifact.FinalOrphanActiveGroups 0 `
        "$EvidenceKey artifact.FinalOrphanActiveGroups"
    $census = $Summary.result.hardDiagnosticCensus
    Assert-SourceGate1Boolean $census.Valid $true "$EvidenceKey census.Valid"
    Assert-SourceGate1Integer `
        $census.UnapprovedHardDiagnosticCount 0 `
        "$EvidenceKey census.UnapprovedHardDiagnosticCount"
    $resourceIdentity = Assert-SourceGate1ResourceDatabaseIdentity `
        $mount.sourceResourceDatabase "$EvidenceKey mount sourceResourceDatabase"
    $sourceResource = Assert-SourceGate1ResourceDatabaseIdentity `
        $Summary.source.sourceResourceDatabase `
        "$EvidenceKey source.sourceResourceDatabase"
    $toolBefore = Assert-SourceGate1ResourceDatabaseIdentity `
        $Summary.toolchain.sourceResourceDatabase.before `
        "$EvidenceKey toolchain sourceResourceDatabase.before"
    $toolAfter = Assert-SourceGate1ResourceDatabaseIdentity `
        $Summary.toolchain.sourceResourceDatabase.after `
        "$EvidenceKey toolchain sourceResourceDatabase.after"
    Assert-SourceGate1Boolean `
        $Summary.toolchain.sourceResourceDatabase.stable $true `
        "$EvidenceKey toolchain sourceResourceDatabase.stable"
    foreach ($identity in @($sourceResource, $toolBefore, $toolAfter)) {
        [void](Assert-SourceGate1ResourceDatabaseMatch `
            $resourceIdentity $identity "$EvidenceKey resource database")
    }
    return $resourceIdentity
}

function Assert-SourceGate1CampaignResult {
    param($Summary, [string]$EvidenceKey, [string]$ReferenceStatus)

    $result = $Summary.result
    if ($ReferenceStatus -ceq 'failed') {
        if ([string]$result.status -cne 'failed') {
            throw "$EvidenceKey failed reference is inconsistent."
        }
        return $null
    }
    $expectedProfile = if ($EvidenceKey -ceq 'forceAuthorityCanary') {
        'force_authority'
    }
    else { 'full_certification' }
    if ([string]$Summary.capture.profile -cne $expectedProfile) {
        throw "$EvidenceKey has the wrong Campaign Debug profile."
    }
    $resourceIdentity = Assert-SourceGate1CampaignCommon $Summary $EvidenceKey
    $acceptance = $result.acceptance
    Assert-SourceGate1Boolean `
        $acceptance.accepted $true "$EvidenceKey acceptance.accepted"
    Assert-SourceGate1Boolean `
        $acceptance.diagnosticAxisPassed $true `
        "$EvidenceKey acceptance.diagnosticAxisPassed"
    Assert-SourceGate1Boolean `
        $acceptance.captureAxesPassed $true `
        "$EvidenceKey acceptance.captureAxesPassed"
    Assert-SourceGate1EmptyArray $acceptance.redAxes "$EvidenceKey acceptance.redAxes"
    if ($EvidenceKey -ceq 'forceAuthorityCanary') {
        if ([string]$result.status -cne 'passed-noncertifying' -or
            [string]$acceptance.disposition -cne 'accepted-corrected-canary') {
            throw 'The force-authority canary disposition is not exact.'
        }
        Assert-SourceGate1Boolean `
            $acceptance.correctedCanaryAccepted $true `
            'canary acceptance.correctedCanaryAccepted'
        foreach ($name in @('proofCommonPassed', 'acceptedFull', 'acceptedInternal')) {
            Assert-SourceGate1Boolean $acceptance.$name $false "canary acceptance.$name"
        }
        $counts = $acceptance.rawCaseCounts
        foreach ($binding in @(
                @('caseCount', 11), @('pass', 9), @('warn', 2),
                @('fail', 0), @('blocked', 0), @('skipped', 0))) {
            Assert-SourceGate1Integer `
                $counts.($binding[0]) $binding[1] "canary counts $($binding[0])"
        }
        Assert-SourceGate1Integer `
            $acceptance.rawAssertionCount 91 'canary rawAssertionCount'
        $cert = $acceptance.certificationCounts
        Assert-SourceGate1Integer $cert.required 87 'canary certification required'
        Assert-SourceGate1Integer $cert.proven 87 'canary certification proven'
        foreach ($name in @('fail', 'blocked', 'warn')) {
            Assert-SourceGate1Integer $cert.$name 0 "canary certification $name"
        }
        Assert-SourceGate1Boolean $cert.passed $false 'canary certification passed'
        foreach ($name in @(
                'SourceCanaryContract', 'SourceCanaryCaseSetExact',
                'SourceCanaryAssertionManifestExact',
                'SourceCanaryWarningContractExact',
                'SourceCanaryNoBlockedAssertions',
                'SourceCanaryOrphanContractExact')) {
            Assert-SourceGate1Boolean `
                $result.artifactValidation.$name $true "canary artifact $name"
        }
        $focused = @($result.artifactValidation.FocusedAssertions)
        if ($focused.Count -ne 35 -or @($focused | Where-Object {
                    $_.Pass -isnot [bool] -or -not [bool]$_.Pass -or
                    [string]$_.Status -cne 'PASS'
                }).Count -ne 0) {
            throw 'The canary does not contain 35/35 focused assertions.'
        }
        Assert-SourceGate1Integer `
            $result.hardDiagnosticCensus.HardDiagnosticCount 2 `
            'canary hard diagnostic count'
        Assert-SourceGate1Integer `
            $result.hardDiagnosticCensus.ApprovedStockDiagnosticCount 2 `
            'canary approved stock diagnostic count'
        Assert-SourceGate1Integer `
            $result.hardDiagnosticCensus.ApprovedIntentionalDiagnosticCount 0 `
            'canary approved intentional diagnostic count'
    }
    else {
        Assert-SourceGate1Boolean `
            $acceptance.correctedCanaryAccepted $false `
            'full acceptance.correctedCanaryAccepted'
        Assert-SourceGate1Boolean `
            $acceptance.proofCommonPassed $true `
            'full acceptance.proofCommonPassed'
        $acceptedFull = $acceptance.acceptedFull -is [bool] -and
            [bool]$acceptance.acceptedFull
        $acceptedInternal = $acceptance.acceptedInternal -is [bool] -and
            [bool]$acceptance.acceptedInternal
        if ($acceptedFull -eq $acceptedInternal) {
            throw 'Full Campaign Debug must be accepted as exactly full or internal.'
        }
        $expectedStatus = if ($acceptedFull) { 'passed' }
            else { 'passed-noncertifying' }
        $expectedDisposition = if ($acceptedFull) { 'accepted-full' }
            else { 'accepted-internal' }
        if ([string]$result.status -cne $expectedStatus -or
            [string]$acceptance.disposition -cne $expectedDisposition) {
            throw 'Full Campaign Debug status and disposition are inconsistent.'
        }
        $cert = $acceptance.certificationCounts
        Assert-SourceGate1Boolean $cert.passed $true 'full certification passed'
        foreach ($name in @('fail', 'blocked', 'warn')) {
            Assert-SourceGate1Integer $cert.$name 0 "full certification $name"
        }
        if ([long]$cert.required -le 0 -or
            [long]$cert.proven -ne [long]$cert.required) {
            throw 'Full Campaign Debug certification counts are not exact.'
        }
        Assert-SourceGate1Integer `
            $acceptance.rawCaseCounts.fail 0 'full raw fail count'
        Assert-SourceGate1Integer `
            $acceptance.rawCaseCounts.blocked 0 'full raw blocked count'
        Assert-SourceGate1EmptyArray `
            $acceptance.unsupportedWarningIds 'full unsupported warnings'
        Assert-SourceGate1EmptyArray `
            $acceptance.unsupportedSkippedIds 'full unsupported skips'
        if ($acceptedFull) {
            Assert-SourceGate1Integer `
                $acceptance.rawCaseCounts.warn 0 'accepted-full raw warn count'
            Assert-SourceGate1EmptyArray `
                $acceptance.externalAdvisoryIds 'accepted-full external advisories'
            Assert-SourceGate1EmptyArray `
                $acceptance.skippedAssertionIds 'accepted-full skipped assertions'
        }
        else {
            Assert-SourceGate1ExactStringSet @(
                'isolation.world_scope', 'persistence.real_restart',
                'phase25.real_restart', 'phase25.second_client',
                'phase25.two_hour_soak') `
                $acceptance.externalAdvisoryIds `
                'accepted-internal external advisories'
            $allowedSkips = @(
                'phase24.escalation.support_physicalization',
                'phase24.escalation.group_physicalization')
            Assert-SourceGate1Array `
                $acceptance.skippedAssertionIds 'accepted-internal skipped assertions'
            $skips = @($acceptance.skippedAssertionIds | ForEach-Object { [string]$_ })
            if (@($skips | Group-Object -CaseSensitive |
                    Where-Object Count -ne 1).Count -ne 0 -or
                @($skips | Where-Object { $allowedSkips -cnotcontains $_ }).Count -ne 0) {
                throw 'accepted-internal skipped assertions are not allowed.'
            }
        }
        Assert-SourceGate1Integer `
            $result.hardDiagnosticCensus.HardDiagnosticCount 15 `
            'full hard diagnostic count'
        Assert-SourceGate1Integer `
            $result.hardDiagnosticCensus.ApprovedStockDiagnosticCount 2 `
            'full approved stock diagnostic count'
        Assert-SourceGate1Integer `
            $result.hardDiagnosticCensus.ApprovedIntentionalDiagnosticCount 13 `
            'full approved intentional diagnostic count'
    }
    return $resourceIdentity
}

function Assert-SourceGate1SummaryChronology {
    param(
        $Summary,
        [DateTimeOffset]$FrozenUtc,
        [DateTimeOffset]$StatusAsOfUtc,
        [string]$Label
    )

    $runId = Require-SourceGate1String $Summary.capture.runId "$Label.capture.runId"
    if ($runId -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]{2,159}$') {
        throw "$Label capture run ID is not portable."
    }
    $started = Require-SourceGate1Utc `
        $Summary.capture.startedUtc "$Label.capture.startedUtc"
    $completed = Require-SourceGate1Utc `
        $Summary.capture.completedUtc "$Label.capture.completedUtc"
    if ($started -lt $FrozenUtc -or $completed -lt $started -or
        $completed -gt $StatusAsOfUtc) {
        throw "$Label chronology is inconsistent with the source status."
    }
    return [pscustomobject][ordered]@{
        RunId = $runId
        StartedUtc = $started.ToString('o')
        CompletedUtc = $completed.ToString('o')
    }
}

function Read-SourceGate1TrackedSummary {
    param(
        [string]$RepositoryRoot,
        [string]$EvidenceKey,
        $Reference,
        $Checkpoint,
        [DateTimeOffset]$FrozenUtc,
        [DateTimeOffset]$StatusAsOfUtc
    )

    $path = Require-SourceGate1String `
        $Reference.summaryPath "$EvidenceKey.summaryPath"
    Assert-SourceGate1PortablePath $path "$EvidenceKey.summaryPath"
    $prefix = 'docs/evidence/source-gate1/' +
        $Checkpoint.SourceGitHead + '/'
    if (-not $path.StartsWith($prefix, [StringComparison]::Ordinal) -or
        -not $path.EndsWith('.json', [StringComparison]::Ordinal)) {
        throw "$EvidenceKey summary path is outside its exact source checkpoint root."
    }
    $tracked = Invoke-SourceGate1Git `
        -RepositoryRoot $RepositoryRoot `
        -Arguments @('ls-files', '--error-unmatch', '--', $path)
    if ($tracked.Lines.Count -ne 1 -or
        $tracked.Lines[0].Replace('\', '/') -cne $path) {
        throw "$EvidenceKey summary is not tracked with exact path case."
    }
    $resolved = Resolve-SourceGate1ContainedFile `
        -Root $RepositoryRoot `
        -PortablePath $path `
        -Label "$EvidenceKey summary"
    $parsed = Read-SourceGate1JsonFile $resolved "$EvidenceKey summary"
    $expectedSha = Require-SourceGate1Sha256 `
        $Reference.summarySha256 "$EvidenceKey.summarySha256"
    if ($parsed.Sha256 -cne $expectedSha) {
        throw "$EvidenceKey summary SHA-256 does not match its tracked bytes."
    }
    $summary = $parsed.Value
    Assert-SourceGate1ExactProperties $summary @(
        'schemaVersion', 'evidenceKind', 'source', 'harness', 'toolchain',
        'capture', 'result', 'integrity') "$EvidenceKey summary"
    Assert-SourceGate1Integer `
        $summary.schemaVersion 1 "$EvidenceKey summary.schemaVersion"
    if ([string]$summary.evidenceKind -cne
        [string]$script:SourceGate1EvidenceKinds[$EvidenceKey]) {
        throw "$EvidenceKey summary evidence kind is not exact."
    }
    Assert-SourceGate1SourceBinding `
        -Source $summary.source `
        -Checkpoint $Checkpoint `
        -EvidenceKey $EvidenceKey `
        -Label "$EvidenceKey.source"
    $harness = Assert-SourceGate1HarnessBinding `
        -Harness $summary.harness `
        -EvidenceKey $EvidenceKey `
        -Checkpoint $Checkpoint `
        -RepositoryRoot $RepositoryRoot
    $chronology = Assert-SourceGate1SummaryChronology `
        -Summary $summary `
        -FrozenUtc $FrozenUtc `
        -StatusAsOfUtc $StatusAsOfUtc `
        -Label $EvidenceKey
    $integrity = Assert-SourceGate1Integrity `
        $summary.integrity "$EvidenceKey.integrity"
    $referenceStatus = [string]$Reference.status
    if ([string]$summary.result.status -cne $referenceStatus) {
        throw "$EvidenceKey reference status differs from the summary result."
    }
    $resourceDatabase = $null
    switch ($EvidenceKey) {
        'foundation' {
            Assert-SourceGate1FoundationResult $summary $referenceStatus |
                Out-Null
        }
        'workbenchAllTargets' {
            $validatorOutput = @(Assert-SourceGate1WorkbenchResult `
                $summary $referenceStatus)
            if ($validatorOutput.Count -gt 0) {
                $resourceDatabase = $validatorOutput[-1]
            }
        }
        'focusedFiveSuite' {
            $validatorOutput = @(Assert-SourceGate1FocusedResult `
                $summary $referenceStatus)
            if ($validatorOutput.Count -gt 0) {
                $resourceDatabase = $validatorOutput[-1]
            }
        }
        'forceAuthorityCanary' {
            $validatorOutput = @(Assert-SourceGate1CampaignResult `
                $summary $EvidenceKey $referenceStatus)
            if ($validatorOutput.Count -gt 0) {
                $resourceDatabase = $validatorOutput[-1]
            }
        }
        'fullCampaignDebug' {
            $validatorOutput = @(Assert-SourceGate1CampaignResult `
                $summary $EvidenceKey $referenceStatus)
            if ($validatorOutput.Count -gt 0) {
                $resourceDatabase = $validatorOutput[-1]
            }
        }
        default { throw 'Unknown source Gate 1 evidence key.' }
    }
    return [pscustomobject][ordered]@{
        Key = $EvidenceKey
        Status = $referenceStatus
        SummaryPath = $path
        SummarySha256 = $expectedSha
        EvidenceKind = [string]$summary.evidenceKind
        ResultStatus = [string]$summary.result.status
        RunId = $chronology.RunId
        StartedUtc = $chronology.StartedUtc
        CompletedUtc = $chronology.CompletedUtc
        Harness = $harness
        Integrity = $integrity
        ResourceDatabase = $resourceDatabase
        PortableSummaryValidated = $true
        RawArtifactsReopened = $false
        Summary = $summary
    }
}

function Get-SourceGate1ExpectedPassStatuses {
    param([string]$EvidenceKey)

    switch ($EvidenceKey) {
        'foundation' { return @('passed') }
        'workbenchAllTargets' { return @('passed') }
        'focusedFiveSuite' { return @('passed-noncertifying') }
        'forceAuthorityCanary' { return @('passed-noncertifying') }
        'fullCampaignDebug' { return @('passed', 'passed-noncertifying') }
        default { throw 'Unknown source Gate 1 evidence key.' }
    }
}

function Assert-PartisanSourceGate1Evidence {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [string]$ReleaseStatusPath = 'docs/data/release_status.json'
    )

    $root = [IO.Path]::GetFullPath($RepositoryRoot)
    Assert-SourceGate1NoReparseAncestry $root 'repository root'
    $statusPath = if ([IO.Path]::IsPathRooted($ReleaseStatusPath)) {
        $resolvedStatus = [IO.Path]::GetFullPath($ReleaseStatusPath)
        $rootPrefix = $root.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
        if (-not $resolvedStatus.StartsWith(
                $rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'The release status path must be inside the repository.'
        }
        $resolvedStatus
    }
    else {
        Resolve-SourceGate1ContainedFile `
            -Root $root `
            -PortablePath $ReleaseStatusPath.Replace('\', '/') `
            -Label 'release status'
    }
    $statusParsed = Read-SourceGate1JsonFile $statusPath 'release status'
    $status = $statusParsed.Value
    Assert-SourceGate1Integer $status.schemaVersion 4 'release status.schemaVersion'
    $statusAsOf = Require-SourceGate1Utc `
        $status.statusAsOfUtc 'release status.statusAsOfUtc'
    $gate = Get-SourceGate1Property $status 'gate1Source'
    Assert-SourceGate1ExactProperties $gate @(
        'status', 'checkpoint', 'evidence') 'gate1Source'
    $checkpoint = Assert-SourceGate1Checkpoint `
        -Checkpoint $gate.checkpoint `
        -RepositoryRoot $root
    $frozenUtc = Require-SourceGate1Utc `
        $gate.checkpoint.frozenUtc 'gate1Source.checkpoint.frozenUtc'
    if ($frozenUtc -gt $statusAsOf) {
        throw 'The Gate 1 source checkpoint is newer than statusAsOfUtc.'
    }
    Assert-SourceGate1ExactProperties `
        $gate.evidence $script:SourceGate1EvidenceOrder 'gate1Source.evidence'

    $rows = New-Object Collections.Generic.List[object]
    $terminalSeen = $false
    $terminalStatus = ''
    $resourceDatabase = $null
    $completedCount = 0
    $nextKey = ''
    $previousCompletedUtc = $null
    foreach ($key in $script:SourceGate1EvidenceOrder) {
        $reference = Get-SourceGate1Property $gate.evidence $key
        Assert-SourceGate1ExactProperties $reference @(
            'status', 'summaryPath', 'summarySha256') "gate1Source.evidence.$key"
        $referenceStatus = Require-SourceGate1String `
            $reference.status "gate1Source.evidence.$key.status"
        $passStatuses = @(Get-SourceGate1ExpectedPassStatuses $key)
        $isPass = $passStatuses -ccontains $referenceStatus
        $isPending = $referenceStatus -ceq 'pending'
        $isFailed = $referenceStatus -ceq 'failed'
        if (-not $isPass -and -not $isPending -and -not $isFailed) {
            throw "Gate 1 evidence '$key' has an unsupported status."
        }
        if ($terminalSeen -and -not $isPending) {
            throw 'Gate 1 evidence states do not follow the monotone required order.'
        }
        if ($isPending) {
            if ($null -ne $reference.summaryPath -or
                $null -ne $reference.summarySha256) {
                throw "$key pending evidence must not name a summary."
            }
            if ([string]::IsNullOrEmpty($nextKey)) { $nextKey = $key }
            [void]$rows.Add([pscustomobject][ordered]@{
                Key = $key
                Status = $referenceStatus
                SummaryPath = $null
                SummarySha256 = $null
                EvidenceKind = [string]$script:SourceGate1EvidenceKinds[$key]
                ResultStatus = $null
                RunId = $null
                CompletedUtc = $null
                PortableSummaryValidated = $false
                RawArtifactsReopened = $false
            })
            $terminalSeen = $true
            continue
        }
        $row = Read-SourceGate1TrackedSummary `
            -RepositoryRoot $root `
            -EvidenceKey $key `
            -Reference $reference `
            -Checkpoint $checkpoint `
            -FrozenUtc $frozenUtc `
            -StatusAsOfUtc $statusAsOf
        [void]$rows.Add($row)
        $rowStartedUtc = [DateTimeOffset]$row.StartedUtc
        $rowCompletedUtc = [DateTimeOffset]$row.CompletedUtc
        if ($null -ne $previousCompletedUtc -and
            $rowStartedUtc -lt $previousCompletedUtc) {
            throw 'Gate 1 evidence runs overlap or are out of required order.'
        }
        $previousCompletedUtc = $rowCompletedUtc
        if ($isFailed) {
            if ([string]::IsNullOrEmpty($nextKey)) { $nextKey = $key }
            $terminalSeen = $true
            $terminalStatus = 'failed'
            continue
        }
        $completedCount++
        if ($key -ceq 'workbenchAllTargets' -and
            $null -ne $row.ResourceDatabase) {
            $resourceDatabase = $row.ResourceDatabase
        }
        elseif ($null -ne $row.ResourceDatabase) {
            $resourceDatabase = Assert-SourceGate1ResourceDatabaseMatch `
                $resourceDatabase $row.ResourceDatabase $key
        }
    }
    $derivedStatus = if ($terminalStatus -ceq 'failed') { 'failed' }
        elseif ($completedCount -eq $script:SourceGate1EvidenceOrder.Count) {
            'passed'
        }
        else { 'in-progress' }
    if ($gate.status -isnot [string] -or
        [string]$gate.status -cne $derivedStatus) {
        throw 'gate1Source.status does not match the ordered evidence state machine.'
    }
    if ($completedCount -ge 2 -and $null -eq $resourceDatabase) {
        throw 'Completed runtime evidence lacks a shared resource database identity.'
    }
    return [pscustomobject][ordered]@{
        ValidationKind = 'partisan_source_gate1_evidence_v1'
        GateStatus = $derivedStatus
        SourceGitHead = $checkpoint.SourceGitHead
        PublishInputTreeSha256 = $checkpoint.PublishInputTreeSha256
        PublishInputRowCount = $checkpoint.PublishInputRowCount
        StatusAsOfUtc = $statusAsOf.ToString('o')
        CompletedEvidenceCount = $completedCount
        RequiredEvidenceCount = $script:SourceGate1EvidenceOrder.Count
        NextEvidenceKey = if ([string]::IsNullOrEmpty($nextKey)) { $null }
            else { $nextKey }
        FoundationPassed = $completedCount -ge 1
        WorkbenchAllTargetsPassed = $completedCount -ge 2
        FocusedFiveSuitePassed = $completedCount -ge 3
        ForceAuthorityCanaryPassed = $completedCount -ge 4
        FullCampaignDebugPassed = $completedCount -ge 5
        AllRequiredEvidencePassed = $completedCount -eq 5
        ResourceDatabase = $resourceDatabase
        Evidence = $rows.ToArray()
        RawArtifactsReopened = $false
    }
}

function Write-SourceGate1SelfTestText {
    param([string]$Path, [AllowEmptyString()][string]$Text)

    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    [IO.File]::WriteAllText(
        $Path,
        $Text.Replace("`r`n", "`n").Replace("`r", "`n"),
        $script:SourceGate1Utf8)
}

function Write-SourceGate1SelfTestJson {
    param([string]$Path, $Value)

    $text = ($Value | ConvertTo-Json -Depth 100).Replace("`r`n", "`n") + "`n"
    Write-SourceGate1SelfTestText $Path $text
    return [pscustomobject][ordered]@{
        Text = $text
        Sha256 = Get-SourceGate1Sha256Text $text
    }
}

function New-SourceGate1SelfTestIntegrity {
    $fileSha = Get-SourceGate1Sha256Text 'raw'
    $canonical = "raw.log`t3`t$fileSha`n"
    return [pscustomobject][ordered]@{
        hashAlgorithm = 'sha256-file-set-v1'
        artifactCount = 1
        artifactSetSha256 = Get-SourceGate1Sha256Text $canonical
        files = @([pscustomobject][ordered]@{
            path = 'raw.log'
            length = 3
            sha256 = $fileSha
        })
    }
}

function New-SourceGate1SelfTestHarness {
    param(
        [string]$RepositoryRoot,
        [string]$HarnessHead,
        [string]$EvidenceKey
    )

    $path = [string]$script:SourceGate1RunnerPaths[$EvidenceKey]
    $bytes = Get-SourceGate1GitBlobBytes $RepositoryRoot $HarnessHead $path
    $focused = $EvidenceKey -ceq 'focusedFiveSuite'
    $sha = if ($focused) {
        $text = $script:SourceGate1Utf8.GetString($bytes)
        Get-SourceGate1Sha256Text $text.Replace("`r`n", "`n").Replace("`r", "`n")
    }
    else { Get-SourceGate1Sha256Bytes $bytes }
    $value = [ordered]@{
        gitHead = $HarnessHead
        dirty = $false
        runnerPath = $path
        runnerHashPolicy = if ($focused) {
            'normalized-utf8-lf-sha256-v1'
        } else { 'sha256-git-blob-bytes-v1' }
        runnerSha256 = $sha
    }
    if ($focused) {
        $value.runnerGitBlobOid = (Invoke-SourceGate1Git `
            -RepositoryRoot $RepositoryRoot `
            -Arguments @('rev-parse', ($HarnessHead + ':' + $path))).Lines[0].Trim()
    }
    return [pscustomobject]$value
}

function New-SourceGate1SelfTestSource {
    param($Checkpoint, [string]$EvidenceKey, $ResourceDatabase)

    $value = [ordered]@{
        sourceGitHead = $Checkpoint.sourceGitHead
        publishInputPolicy = $Checkpoint.publishInputPolicy
        publishInputTreeSha256 = $Checkpoint.publishInputTreeSha256
        publishInputRowCount = $Checkpoint.publishInputRowCount
        addonGuid = $Checkpoint.addonGuid
        campaignSchema = $Checkpoint.campaignSchema
        runtimeSettingsSchema = $Checkpoint.runtimeSettingsSchema
        embeddedImplementationSha = $Checkpoint.embeddedImplementationSha
        embeddedBuildUtc = $Checkpoint.embeddedBuildUtc
        embeddedBuildLabel = $Checkpoint.embeddedBuildLabel
    }
    if ($EvidenceKey -in @('forceAuthorityCanary', 'fullCampaignDebug')) {
        $value.executedPublishInputTreeSha256 = $Checkpoint.publishInputTreeSha256
        $value.executedPublishInputRowCount = $Checkpoint.publishInputRowCount
        $value.sourceResourceDatabase = $ResourceDatabase
    }
    return [pscustomobject]$value
}

function New-SourceGate1SelfTestCampaignResult {
    param([string]$EvidenceKey, $ResourceDatabase)

    $canary = $EvidenceKey -ceq 'forceAuthorityCanary'
    $focusedAssertions = @()
    if ($canary) {
        $focusedAssertions = @(0..34 | ForEach-Object {
            [pscustomobject][ordered]@{ Pass = $true; Status = 'PASS' }
        })
    }
    $artifact = [pscustomobject][ordered]@{
        Valid = $true
        StateDiffManifestExact = $true
        StateDiffRows = 18
        NonzeroStateDiffRows = 0
        FinalOrphanCleanupPass = $true
        FinalOrphanActiveGroups = 0
        CorrectedCanaryContract = $false
        CorrectedCanaryCaseSetExact = $false
        CorrectedCanaryAssertionManifestExact = $false
        CorrectedCanaryWarningContractExact = $false
        CorrectedCanaryBlockedContractExact = $false
        CorrectedCanaryOrphanContractExact = $false
        SourceArtifactContract = $true
        SourceCanaryContract = $canary
        SourceCanaryCaseSetExact = $canary
        SourceCanaryAssertionManifestExact = $canary
        SourceCanaryWarningContractExact = $canary
        SourceCanaryNoBlockedAssertions = $canary
        SourceCanaryOrphanContractExact = $canary
        FocusedAssertions = $focusedAssertions
    }
    $acceptance = if ($canary) {
        [pscustomobject][ordered]@{
            accepted = $true
            disposition = 'accepted-corrected-canary'
            correctedCanaryAccepted = $true
            proofCommonPassed = $false
            acceptedFull = $false
            acceptedInternal = $false
            diagnosticAxisPassed = $true
            captureAxesPassed = $true
            rawCaseCounts = [pscustomobject][ordered]@{
                caseCount = 11; pass = 9; warn = 2; fail = 0
                blocked = 0; skipped = 0
            }
            rawAssertionCount = 91
            certificationCounts = [pscustomobject][ordered]@{
                required = 87; proven = 87; fail = 0; blocked = 0
                warn = 0; passed = $false
            }
            externalAdvisoryIds = @()
            unsupportedWarningIds = @()
            skippedAssertionIds = @()
            unsupportedSkippedIds = @()
            redAxes = @()
        }
    }
    else {
        [pscustomobject][ordered]@{
            accepted = $true
            disposition = 'accepted-full'
            correctedCanaryAccepted = $false
            proofCommonPassed = $true
            acceptedFull = $true
            acceptedInternal = $false
            diagnosticAxisPassed = $true
            captureAxesPassed = $true
            rawCaseCounts = [pscustomobject][ordered]@{
                caseCount = 600; pass = 600; warn = 0; fail = 0
                blocked = 0; skipped = 0
            }
            rawAssertionCount = 6000
            certificationCounts = [pscustomobject][ordered]@{
                required = 5000; proven = 5000; fail = 0; blocked = 0
                warn = 0; passed = $true
            }
            externalAdvisoryIds = @()
            unsupportedWarningIds = @()
            skippedAssertionIds = @()
            unsupportedSkippedIds = @()
            redAxes = @()
        }
    }
    return [pscustomobject][ordered]@{
        status = if ($canary) { 'passed-noncertifying' } else { 'passed' }
        runtimeSeconds = 10
        sourceStable = $true
        sourceResourceDatabaseStable = $true
        settings = [pscustomobject][ordered]@{ schemaVersion = 24; stable = $true }
        launch = [pscustomobject][ordered]@{
            argumentTokensVerified = $true
            armed = $true
            started = $true
            artifactSetExact = $true
            artifactBytesStableAfterShutdown = $true
        }
        artifactValidation = $artifact
        hardDiagnosticCensus = [pscustomobject][ordered]@{
            Valid = $true
            HardDiagnosticCount = if ($canary) { 2 } else { 15 }
            ApprovedStockDiagnosticCount = 2
            ApprovedIntentionalDiagnosticCount = if ($canary) { 0 } else { 13 }
            UnapprovedHardDiagnosticCount = 0
        }
        acceptance = $acceptance
        mountAttestation = [pscustomobject][ordered]@{
            valid = $true
            everyProjectRecordExact = $true
            generatedArchiveMatchCount = 0
            sourceResourceDatabaseRecordCount = 1
            everyResourceDatabaseRecordExact = $true
            sourceResourceDatabase = $ResourceDatabase
        }
        processCensus = [pscustomobject][ordered]@{
            before = @()
            ownedProcessesRemaining = 0
            unclaimedEngineProcessesObserved = @()
            after = @()
            cleanupErrors = @()
        }
        failureReasons = @()
    }
}

function New-SourceGate1SelfTestSummary {
    param(
        [string]$RepositoryRoot,
        [string]$HarnessHead,
        [string]$EvidenceKey,
        $Checkpoint,
        $ResourceDatabase
    )

    $toolchain = [pscustomobject][ordered]@{}
    $ordinal = [Array]::IndexOf(
        [object[]]$script:SourceGate1EvidenceOrder, [object]$EvidenceKey)
    $startedUtc = [DateTimeOffset]'2026-01-01T00:01:00Z'
    $startedUtc = $startedUtc.AddMinutes($ordinal * 2)
    $capture = [ordered]@{
        runId = 'selftest-' + $EvidenceKey
        startedUtc = $startedUtc.ToString(
            'yyyy-MM-ddTHH:mm:ssZ', [Globalization.CultureInfo]::InvariantCulture)
        completedUtc = $startedUtc.AddMinutes(1).ToString(
            'yyyy-MM-ddTHH:mm:ssZ', [Globalization.CultureInfo]::InvariantCulture)
    }
    $result = $null
    switch ($EvidenceKey) {
        'foundation' {
            $result = [pscustomobject][ordered]@{
                status = 'passed'; success = $true; exitCode = 0
                completionMarker = $true; referenceCount = 1
                trackedPakCount = 0; workspacePakCount = 0
                verifiedTrackedPublishInputCount = $Checkpoint.publishInputRowCount
                extraUntrackedOrIgnoredPublishInputCount = 0
                sourceAndHarnessStable = $true; failureCodes = @()
            }
        }
        'workbenchAllTargets' {
            $targets = @($script:SourceGate1WorkbenchTargets | ForEach-Object {
                [pscustomobject][ordered]@{
                    target = $_; success = $true; exitCode = 0
                    files = 1; classes = 1; crc = '1234abcd'
                    hardErrorCount = 0; cleanupAndSpillZero = $true
                    pakCensusZero = $true
                    sourceResourceDatabase = $ResourceDatabase
                    sourceResourceDatabaseStable = $true
                }
            })
            $toolchain = [pscustomobject][ordered]@{
                sourceResourceDatabase = $ResourceDatabase
            }
            $capture.targets = $targets
            $result = [pscustomobject][ordered]@{
                status = 'passed'; success = $true; targetCount = 5
                passedTargets = 5; exactRequiredTargets = $true
                commonCrc = '1234abcd'; positiveFileAndClassCounts = $true
                hardErrorCount = 0; cleanupAndSpillZeroTargets = 5
                pakCensusZeroTargets = 5
                sourceResourceDatabaseStableTargets = 5
                sourceResourceDatabaseFinalStable = $true
                sourceAndHarnessStable = $true; failureCodes = @()
            }
        }
        'focusedFiveSuite' {
            $definitions = New-Object Collections.Generic.List[object]
            $suites = New-Object Collections.Generic.List[object]
            foreach ($suiteName in $script:SourceGate1FocusedSuites.Keys) {
                $count = [int]$script:SourceGate1FocusedSuites[$suiteName]
                [void]$definitions.Add([pscustomobject][ordered]@{
                    suite = $suiteName; expectedCaseCount = $count
                })
                [void]$suites.Add([pscustomobject][ordered]@{
                    suite = $suiteName; status = 'passed'; success = $true
                    expectedCaseCount = $count; sourceBindingsStable = $true
                    failedListBytes = 0
                    junit = [pscustomobject][ordered]@{
                        valid = $true; tests = $count; failures = 0
                        errors = 0; skipped = 0
                    }
                    sourceMount = [pscustomobject][ordered]@{
                        valid = $true; sourcePackedRecordCount = 0
                        sourcePackageRecordCount = 0
                        resourceDatabaseLoadExact = $true
                    }
                    profileMount = [pscustomobject][ordered]@{
                        policy = 'profile-argument-parent-mount-v1'
                        profileArgument = '<suite-scratch>'
                        resolvedProfilePath = '<suite-scratch>/profile'
                        proofAuthorizationRequired = $suiteName -ceq
                            'HST_CampaignProfileJournalAuthorityAutotestSuite'
                        proofAuthorizationEstablished = $suiteName -ceq
                            'HST_CampaignProfileJournalAuthorityAutotestSuite'
                        proofSentinelStableThroughExit = $true
                        proofSentinelPath = if ($suiteName -ceq
                            'HST_CampaignProfileJournalAuthorityAutotestSuite') {
                            '<suite-scratch>/profile/.partisan-focused-owner'
                        } else { '' }
                        proofSentinelValueSha256 = if ($suiteName -ceq
                            'HST_CampaignProfileJournalAuthorityAutotestSuite') {
                            Get-SourceGate1Sha256Text 'owned'
                        } else { '' }
                    }
                })
            }
            $toolchain = [pscustomobject][ordered]@{
                sourceResourceDatabase = $ResourceDatabase
            }
            $result = [pscustomobject][ordered]@{
                status = 'passed-noncertifying'; success = $true
                noncertifying = $true; expectedSuiteCount = 5
                completedSuiteCount = 5; passedSuiteCount = 5
                expectedTestCount = 91; tests = 91; failures = 0
                errors = 0; skipped = 0; sourceBindingsStable = $true
                sourceIsHarnessAncestor = $true
                publishInputsMatchHarness = $true; finalEngineProcessCount = 0
                suiteDefinitions = $definitions.ToArray()
                suites = $suites.ToArray(); error = ''
            }
        }
        default {
            $capture.profile = if ($EvidenceKey -ceq 'forceAuthorityCanary') {
                'force_authority'
            } else { 'full_certification' }
            $toolchain = [pscustomobject][ordered]@{
                sourceResourceDatabase = [pscustomobject][ordered]@{
                    before = $ResourceDatabase
                    after = $ResourceDatabase
                    stable = $true
                }
            }
            $result = New-SourceGate1SelfTestCampaignResult `
                $EvidenceKey $ResourceDatabase
        }
    }
    return [pscustomobject][ordered]@{
        schemaVersion = 1
        evidenceKind = [string]$script:SourceGate1EvidenceKinds[$EvidenceKey]
        source = New-SourceGate1SelfTestSource `
            $Checkpoint $EvidenceKey $ResourceDatabase
        harness = New-SourceGate1SelfTestHarness `
            $RepositoryRoot $HarnessHead $EvidenceKey
        toolchain = $toolchain
        capture = [pscustomobject]$capture
        result = $result
        integrity = New-SourceGate1SelfTestIntegrity
    }
}

function Initialize-SourceGate1SelfTestFixture {
    param([string]$Root)

    New-Item -ItemType Directory -Path $Root -Force | Out-Null
    [void](Invoke-SourceGate1Git $Root @('init', '-q'))
    [void](Invoke-SourceGate1Git $Root @('config', 'core.autocrlf', 'false'))
    [void](Invoke-SourceGate1Git $Root @('config', 'user.name',
        'Partisan Source Gate1 Selftest'))
    [void](Invoke-SourceGate1Git $Root @('config', 'user.email',
        'source-gate1-selftest.invalid'))
    Write-SourceGate1SelfTestText (Join-Path $Root 'addon.gproj') "selftest`n"
    [void](Invoke-SourceGate1Git $Root @('add', '--', 'addon.gproj'))
    [void](Invoke-SourceGate1Git $Root @('commit', '-q', '-m', 'source'))
    $sourceHead = (Invoke-SourceGate1Git `
        $Root @('rev-parse', 'HEAD')).Lines[0].Trim()
    foreach ($path in @($script:SourceGate1RunnerPaths.Values |
            Sort-Object -Unique -CaseSensitive)) {
        Write-SourceGate1SelfTestText `
            (Join-Path $Root ([string]$path).Replace('/', '\')) `
            ("# " + [string]$path + "`n")
    }
    [void](Invoke-SourceGate1Git $Root @('add', '--', 'tools'))
    [void](Invoke-SourceGate1Git $Root @('commit', '-q', '-m', 'harness'))
    $harnessHead = (Invoke-SourceGate1Git `
        $Root @('rev-parse', 'HEAD')).Lines[0].Trim()
    $publish = Get-SourceGate1PublishInputBinding $Root $sourceHead
    $checkpoint = [pscustomobject][ordered]@{
        sourceGitHead = $sourceHead
        frozenUtc = '2026-01-01T00:00:00Z'
        dirty = $false
        publishInputPolicy = $publish.Policy
        publishInputTreeSha256 = $publish.Sha256
        publishInputRowCount = $publish.RowCount
        addonGuid = '698532771130111D'
        campaignSchema = 71
        runtimeSettingsSchema = 24
        embeddedImplementationSha = $sourceHead
        embeddedBuildUtc = '2025-12-31T23:59:00Z'
        embeddedBuildLabel = 'source-gate1-selftest'
    }
    $resourceDatabase = [pscustomobject][ordered]@{
        length = 4
        sha256 = Get-SourceGate1Sha256Text 'rdb!'
    }
    $evidenceRoot = 'docs/evidence/source-gate1/' + $sourceHead
    $references = [ordered]@{}
    $summaryPaths = [ordered]@{}
    $summaryTexts = [ordered]@{}
    foreach ($key in $script:SourceGate1EvidenceOrder) {
        $summary = New-SourceGate1SelfTestSummary `
            $Root $harnessHead $key $checkpoint $resourceDatabase
        $path = $evidenceRoot + '/' + $key + '.json'
        $written = Write-SourceGate1SelfTestJson `
            (Join-Path $Root $path.Replace('/', '\')) $summary
        $references[$key] = [pscustomobject][ordered]@{
            status = [string]$summary.result.status
            summaryPath = $path
            summarySha256 = $written.Sha256
        }
        $summaryPaths[$key] = $path
        $summaryTexts[$key] = $written.Text
    }
    $status = [pscustomobject][ordered]@{
        schemaVersion = 4
        statusAsOfUtc = '2026-01-01T01:00:00Z'
        gate1Source = [pscustomobject][ordered]@{
            status = 'passed'
            checkpoint = $checkpoint
            evidence = [pscustomobject]$references
        }
    }
    $statusPath = Join-Path $Root 'docs\data\release_status.json'
    $statusWritten = Write-SourceGate1SelfTestJson $statusPath $status
    [void](Invoke-SourceGate1Git $Root @('add', '--', 'docs'))
    return [pscustomobject][ordered]@{
        Root = $Root
        SourceHead = $sourceHead
        HarnessHead = $harnessHead
        Checkpoint = $checkpoint
        ResourceDatabase = $resourceDatabase
        SummaryPaths = $summaryPaths
        SummaryTexts = $summaryTexts
        StatusPath = $statusPath
        StatusText = $statusWritten.Text
    }
}

function Reset-SourceGate1SelfTestFixture {
    param($Fixture)

    foreach ($key in $script:SourceGate1EvidenceOrder) {
        Write-SourceGate1SelfTestText `
            (Join-Path $Fixture.Root `
                ([string]$Fixture.SummaryPaths[$key]).Replace('/', '\')) `
            ([string]$Fixture.SummaryTexts[$key])
    }
    Write-SourceGate1SelfTestText $Fixture.StatusPath $Fixture.StatusText
    Write-SourceGate1SelfTestText `
        (Join-Path $Fixture.Root 'addon.gproj') "selftest`n"
    $generatedArchive = Join-Path $Fixture.Root `
        ('selftest-generated.' + ('p' + 'ak'))
    if (Test-Path -LiteralPath $generatedArchive -PathType Leaf) {
        Remove-Item -LiteralPath $generatedArchive -Force
    }
}

function Assert-SourceGate1SelfTest {
    param([bool]$Condition, [string]$Message)

    if (-not $Condition) { throw "Source Gate 1 consumer self-test failed: $Message" }
}

function Assert-SourceGate1SelfTestThrows {
    param([scriptblock]$Action, [string]$Message)

    $thrown = $false
    try { & $Action | Out-Null }
    catch { $thrown = $true }
    Assert-SourceGate1SelfTest $thrown $Message
}

function Invoke-SourceGate1EvidenceSelfTest {
    $tempRoot = Join-Path ([IO.Path]::GetTempPath()) `
        ('PartisanSourceGate1Consumer_' + [Guid]::NewGuid().ToString('N'))
    $passed = 0
    $rejected = 0
    try {
        $fixture = Initialize-SourceGate1SelfTestFixture $tempRoot
        $valid = Assert-PartisanSourceGate1Evidence `
            -RepositoryRoot $fixture.Root
        Assert-SourceGate1SelfTest `
            ($valid.GateStatus -ceq 'passed' -and
                $valid.CompletedEvidenceCount -eq 5 -and
                $valid.ResourceDatabase.Length -eq 4) `
            'the complete five-stage fixture was not admitted'
        $passed++

        $pendingStatus = $fixture.StatusText | ConvertFrom-Json
        $pendingStatus.gate1Source.status = 'in-progress'
        foreach ($key in $script:SourceGate1EvidenceOrder) {
            $reference = $pendingStatus.gate1Source.evidence.$key
            $reference.status = 'pending'
            $reference.summaryPath = $null
            $reference.summarySha256 = $null
        }
        [void](Write-SourceGate1SelfTestJson `
            $fixture.StatusPath $pendingStatus)
        $pending = Assert-PartisanSourceGate1Evidence `
            -RepositoryRoot $fixture.Root
        Assert-SourceGate1SelfTest `
            ($pending.GateStatus -ceq 'in-progress' -and
                $pending.CompletedEvidenceCount -eq 0 -and
                $pending.NextEvidenceKey -ceq 'foundation') `
            'the all-pending fixture was not admitted'
        $passed++
        Reset-SourceGate1SelfTestFixture $fixture

        $state = $fixture.StatusText | ConvertFrom-Json
        $state.gate1Source.status = 'in-progress'
        $state.gate1Source.evidence.foundation.status = 'pending'
        $state.gate1Source.evidence.foundation.summaryPath = $null
        $state.gate1Source.evidence.foundation.summarySha256 = $null
        [void](Write-SourceGate1SelfTestJson $fixture.StatusPath $state)
        Assert-SourceGate1SelfTestThrows {
            Assert-PartisanSourceGate1Evidence -RepositoryRoot $fixture.Root
        } 'out-of-order evidence was accepted'
        $rejected++
        Reset-SourceGate1SelfTestFixture $fixture

        $foundationPath = Join-Path $fixture.Root `
            ([string]$fixture.SummaryPaths.foundation).Replace('/', '\')
        $duplicate = [Regex]::Replace(
            [string]$fixture.SummaryTexts.foundation,
            '"schemaVersion"\s*:\s*1\s*,',
            '"schemaVersion":1,"schemaVersion":1,',
            1)
        Write-SourceGate1SelfTestText $foundationPath $duplicate
        $status = $fixture.StatusText | ConvertFrom-Json
        $status.gate1Source.evidence.foundation.summarySha256 =
            Get-SourceGate1Sha256Text $duplicate
        [void](Write-SourceGate1SelfTestJson $fixture.StatusPath $status)
        Assert-SourceGate1SelfTestThrows {
            Assert-PartisanSourceGate1Evidence -RepositoryRoot $fixture.Root
        } 'duplicate JSON keys were accepted'
        $rejected++
        Reset-SourceGate1SelfTestFixture $fixture

        $focusedPath = Join-Path $fixture.Root `
            ([string]$fixture.SummaryPaths.focusedFiveSuite).Replace('/', '\')
        $focused = $fixture.SummaryTexts.focusedFiveSuite | ConvertFrom-Json
        $focused.harness.runnerSha256 = '0' * 64
        $focusedWritten = Write-SourceGate1SelfTestJson $focusedPath $focused
        $status = $fixture.StatusText | ConvertFrom-Json
        $status.gate1Source.evidence.focusedFiveSuite.summarySha256 =
            $focusedWritten.Sha256
        [void](Write-SourceGate1SelfTestJson $fixture.StatusPath $status)
        Assert-SourceGate1SelfTestThrows {
            Assert-PartisanSourceGate1Evidence -RepositoryRoot $fixture.Root
        } 'a forged committed-runner SHA was accepted'
        $rejected++
        Reset-SourceGate1SelfTestFixture $fixture

        $focused = $fixture.SummaryTexts.focusedFiveSuite | ConvertFrom-Json
        $focused.result.suites[4].profileMount.proofAuthorizationEstablished =
            $false
        $focusedWritten = Write-SourceGate1SelfTestJson $focusedPath $focused
        $status = $fixture.StatusText | ConvertFrom-Json
        $status.gate1Source.evidence.focusedFiveSuite.summarySha256 =
            $focusedWritten.Sha256
        [void](Write-SourceGate1SelfTestJson $fixture.StatusPath $status)
        Assert-SourceGate1SelfTestThrows {
            Assert-PartisanSourceGate1Evidence -RepositoryRoot $fixture.Root
        } 'an unarmed destructive profile proof was accepted'
        $rejected++
        Reset-SourceGate1SelfTestFixture $fixture

        $focused = $fixture.SummaryTexts.focusedFiveSuite | ConvertFrom-Json
        $focused.integrity.artifactSetSha256 = '0' * 64
        $focusedWritten = Write-SourceGate1SelfTestJson $focusedPath $focused
        $status = $fixture.StatusText | ConvertFrom-Json
        $status.gate1Source.evidence.focusedFiveSuite.summarySha256 =
            $focusedWritten.Sha256
        [void](Write-SourceGate1SelfTestJson $fixture.StatusPath $status)
        Assert-SourceGate1SelfTestThrows {
            Assert-PartisanSourceGate1Evidence -RepositoryRoot $fixture.Root
        } 'a noncanonical artifact-set hash was accepted'
        $rejected++
        Reset-SourceGate1SelfTestFixture $fixture

        $fullPath = Join-Path $fixture.Root `
            ([string]$fixture.SummaryPaths.fullCampaignDebug).Replace('/', '\')
        $full = $fixture.SummaryTexts.fullCampaignDebug | ConvertFrom-Json
        $otherResource = [pscustomobject]@{ length = 5; sha256 = '1' * 64 }
        $full.source.sourceResourceDatabase = $otherResource
        $full.toolchain.sourceResourceDatabase.before = $otherResource
        $full.toolchain.sourceResourceDatabase.after = $otherResource
        $full.result.mountAttestation.sourceResourceDatabase = $otherResource
        $fullWritten = Write-SourceGate1SelfTestJson $fullPath $full
        $status = $fixture.StatusText | ConvertFrom-Json
        $status.gate1Source.evidence.fullCampaignDebug.summarySha256 =
            $fullWritten.Sha256
        [void](Write-SourceGate1SelfTestJson $fixture.StatusPath $status)
        Assert-SourceGate1SelfTestThrows {
            Assert-PartisanSourceGate1Evidence -RepositoryRoot $fixture.Root
        } 'cross-stage resource database drift was accepted'
        $rejected++
        Reset-SourceGate1SelfTestFixture $fixture

        $focused = $fixture.SummaryTexts.focusedFiveSuite | ConvertFrom-Json
        $focused.capture.startedUtc = '2026-01-01T00:01:30Z'
        $focused.capture.completedUtc = '2026-01-01T00:02:30Z'
        $focusedWritten = Write-SourceGate1SelfTestJson $focusedPath $focused
        $status = $fixture.StatusText | ConvertFrom-Json
        $status.gate1Source.evidence.focusedFiveSuite.summarySha256 =
            $focusedWritten.Sha256
        [void](Write-SourceGate1SelfTestJson $fixture.StatusPath $status)
        Assert-SourceGate1SelfTestThrows {
            Assert-PartisanSourceGate1Evidence -RepositoryRoot $fixture.Root
        } 'overlapping evidence chronology was accepted'
        $rejected++
        Reset-SourceGate1SelfTestFixture $fixture

        Write-SourceGate1SelfTestText `
            (Join-Path $fixture.Root 'addon.gproj') "tampered`n"
        Assert-SourceGate1SelfTestThrows {
            Assert-PartisanSourceGate1Evidence -RepositoryRoot $fixture.Root
        } 'publish worktree byte drift was accepted'
        $rejected++
        Reset-SourceGate1SelfTestFixture $fixture

        $archivePath = Join-Path $fixture.Root `
            ('selftest-generated.' + ('p' + 'ak'))
        Write-SourceGate1SelfTestText $archivePath 'generated'
        Assert-SourceGate1SelfTestThrows {
            Assert-PartisanSourceGate1Evidence -RepositoryRoot $fixture.Root
        } 'a generated archive was accepted in the checkout'
        $rejected++
        Reset-SourceGate1SelfTestFixture $fixture

        $invalidBytes = [byte[]]@(0xff, 0xfe, 0xfd)
        [IO.File]::WriteAllBytes($focusedPath, $invalidBytes)
        $status = $fixture.StatusText | ConvertFrom-Json
        $status.gate1Source.evidence.focusedFiveSuite.summarySha256 =
            Get-SourceGate1Sha256Bytes $invalidBytes
        [void](Write-SourceGate1SelfTestJson $fixture.StatusPath $status)
        Assert-SourceGate1SelfTestThrows {
            Assert-PartisanSourceGate1Evidence -RepositoryRoot $fixture.Root
        } 'non-UTF8 evidence was accepted'
        $rejected++

        Write-Output ('SELFTEST ' + ([pscustomobject][ordered]@{
            success = $true
            admittedFixtures = $passed
            rejectedTamperFixtures = $rejected
            duplicateJsonChecks = 1
            stateMachineChecks = 2
            committedRunnerChecks = 1
            integrityChecks = 1
            resourceDatabaseChecks = 1
            chronologyChecks = 1
            publishWorktreeChecks = 2
            utf8Checks = 1
        } | ConvertTo-Json -Compress))
    }
    finally {
        $expectedPrefix = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).
            TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
        $resolved = [IO.Path]::GetFullPath($tempRoot)
        if ($resolved.StartsWith(
                $expectedPrefix, [StringComparison]::OrdinalIgnoreCase) -and
            (Split-Path -Leaf $resolved) -match
                '^PartisanSourceGate1Consumer_[0-9a-f]{32}$' -and
            (Test-Path -LiteralPath $resolved -PathType Container)) {
            Remove-Item -LiteralPath $resolved -Recurse -Force
        }
    }
}

if ($SelfTest) {
    Invoke-SourceGate1EvidenceSelfTest
    return
}

if (-not $LibraryOnly) {
    if ([string]::IsNullOrWhiteSpace($RepositoryRoot)) {
        $RepositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
    }
    Assert-PartisanSourceGate1Evidence `
        -RepositoryRoot $RepositoryRoot `
        -ReleaseStatusPath $ReleaseStatusPath
}
