Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:SurfaceEvidenceKind =
    'partisan_release_surface_runtime_audit_index_v1'
$script:SurfaceRunEvidenceKind =
    'partisan_release_surface_runtime_audit_v1'
$script:SurfaceDisposition =
    'passed-noncertifying-release-surface-audit'
$script:SurfaceForbiddenMemberCount = 67
$script:SurfaceProductionMemberCount = 91
$script:RetentionEvidenceKind = 'packaged-gate1-runtime-retention'
$script:RetentionDisposition = 'passed-noncertifying-retention'
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
$script:RetentionStages = @(
    'autosave_checkpoint',
    'manual_checkpoint',
    'shutdown_checkpoint',
    'native_shutdown_verify',
    'profile_fallback_verify')

function Get-Gate1ConsumerProperty {
    param($Value, [string]$Name)

    if ($null -eq $Value) { return $null }
    $properties = @($Value.PSObject.Properties | Where-Object {
        [string]$_.Name -ceq $Name
    })
    if ($properties.Count -eq 0) { return $null }
    if ($properties.Count -ne 1) {
        throw "JSON property $Name is not unique with exact case."
    }
    return $properties[0].Value
}

function Assert-Gate1ConsumerExactProperties {
    param($Value, [string[]]$Names, [string]$Label)

    if ($null -eq $Value) { throw "$Label is missing." }
    $actual = @($Value.PSObject.Properties | ForEach-Object { $_.Name })
    if (@(Compare-Object -ReferenceObject @($Names) `
            -DifferenceObject $actual -CaseSensitive).Count -ne 0) {
        throw "$Label does not have the exact property set."
    }
}

function Assert-Gate1ConsumerStringArrayExact {
    param([object[]]$Expected, [object[]]$Actual, [string]$Label)

    if (@($Expected).Count -ne @($Actual).Count) {
        throw "$Label does not have the exact ordered values."
    }
    for ($index = 0; $index -lt @($Expected).Count; $index++) {
        if ([string]$Expected[$index] -cne [string]$Actual[$index]) {
            throw "$Label does not have the exact ordered values."
        }
    }
}

function Require-Gate1ConsumerText {
    param($Value, [string]$Label)

    if ($Value -isnot [string] -or
        [string]::IsNullOrWhiteSpace([string]$Value)) {
        throw "$Label must be a non-empty string."
    }
    return [string]$Value
}

function Test-Gate1ConsumerInteger {
    param($Value, [long]$Minimum = [long]::MinValue)

    if ($Value -isnot [int] -and $Value -isnot [long]) {
        return $false
    }
    return [long]$Value -ge $Minimum
}

function Assert-Gate1ConsumerInteger {
    param($Value, [long]$Expected, [string]$Label)

    if (-not (Test-Gate1ConsumerInteger $Value) -or
        [long]$Value -ne $Expected) {
        throw "$Label must be the exact integer $Expected."
    }
}

function Assert-Gate1ConsumerArray {
    param($Value, [string]$Label)

    if ($Value -isnot [Array]) {
        throw "$Label must be a JSON array."
    }
}

function Require-Gate1ConsumerSha256 {
    param($Value, [string]$Label)

    $text = Require-Gate1ConsumerText $Value $Label
    if ($text -cnotmatch '^[0-9a-f]{64}$') {
        throw "$Label must be a lowercase SHA-256 value."
    }
    return $text
}

function Require-Gate1ConsumerUtc {
    param($Value, [string]$Label)

    $text = Require-Gate1ConsumerText $Value $Label
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

function Get-Gate1ConsumerSha256Bytes {
    param([byte[]]$Bytes)

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally { $algorithm.Dispose() }
}

function Get-Gate1ConsumerSha256Text {
    param([AllowEmptyString()][string]$Text)

    return Get-Gate1ConsumerSha256Bytes `
        ((New-Object Text.UTF8Encoding($false)).GetBytes($Text))
}

function Assert-Gate1ConsumerPortablePath {
    param([string]$Path, [string]$Label)

    if ([string]::IsNullOrWhiteSpace($Path) -or
        $Path.Contains('\') -or $Path.Contains(':') -or
        $Path.StartsWith('/', [StringComparison]::Ordinal) -or
        $Path.Contains('//') -or $Path -cmatch '(^|/)\.\.?(/|$)') {
        throw "$Label is not a normalized portable relative path."
    }
}

function Assert-Gate1ConsumerNoLocalPathText {
    param([string]$Text, [string]$Label)

    if ($Text -match
            '(?i)(?:[a-z]:[\\/]|\\\\|file:(?:/+|\\+)|/(?:Users|home|mnt|tmp|var|etc|opt|root|Volumes)(?:[\\/]|[?#]|$))') {
        throw "$Label contains a machine-local path."
    }
}

function Skip-Gate1ConsumerJsonWhitespace {
    param([string]$Text, $Position)

    while ($Position.Value -lt $Text.Length) {
        $code = [int]$Text[$Position.Value]
        if ($code -notin @(0x20, 0x09, 0x0a, 0x0d)) { break }
        $Position.Value++
    }
}

function Read-Gate1ConsumerJsonString {
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

function Read-Gate1ConsumerJsonNumber {
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

function Read-Gate1ConsumerJsonValue {
    param([string]$Text, $Position, [string]$Label)

    Skip-Gate1ConsumerJsonWhitespace $Text $Position
    if ($Position.Value -ge $Text.Length) {
        throw "$Label contains incomplete JSON."
    }
    $code = [int]$Text[$Position.Value]
    if ($code -eq 0x7b) {
        Read-Gate1ConsumerJsonObject $Text $Position $Label
        return
    }
    if ($code -eq 0x5b) {
        Read-Gate1ConsumerJsonArray $Text $Position $Label
        return
    }
    if ($code -eq 0x22) {
        [void](Read-Gate1ConsumerJsonString $Text $Position $Label)
        return
    }
    foreach ($literal in @('true', 'false', 'null')) {
        if ($Position.Value + $literal.Length -le $Text.Length -and
            $Text.Substring($Position.Value, $literal.Length) -ceq $literal) {
            $Position.Value += $literal.Length
            return
        }
    }
    Read-Gate1ConsumerJsonNumber $Text $Position $Label
}

function Read-Gate1ConsumerJsonObject {
    param([string]$Text, $Position, [string]$Label)

    $Position.Value++
    $names = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    Skip-Gate1ConsumerJsonWhitespace $Text $Position
    if ($Position.Value -lt $Text.Length -and
        [int]$Text[$Position.Value] -eq 0x7d) {
        $Position.Value++
        return
    }
    while ($true) {
        $name = Read-Gate1ConsumerJsonString $Text $Position $Label
        if (-not $names.Add($name)) {
            throw "$Label contains duplicate JSON property '$name'."
        }
        Skip-Gate1ConsumerJsonWhitespace $Text $Position
        if ($Position.Value -ge $Text.Length -or
            [int]$Text[$Position.Value] -ne 0x3a) {
            throw "$Label contains invalid JSON property syntax."
        }
        $Position.Value++
        Read-Gate1ConsumerJsonValue $Text $Position $Label
        Skip-Gate1ConsumerJsonWhitespace $Text $Position
        if ($Position.Value -ge $Text.Length) {
            throw "$Label contains an unterminated JSON object."
        }
        $delimiter = [int]$Text[$Position.Value]
        $Position.Value++
        if ($delimiter -eq 0x7d) { return }
        if ($delimiter -ne 0x2c) {
            throw "$Label contains invalid JSON object punctuation."
        }
        Skip-Gate1ConsumerJsonWhitespace $Text $Position
    }
}

function Read-Gate1ConsumerJsonArray {
    param([string]$Text, $Position, [string]$Label)

    $Position.Value++
    Skip-Gate1ConsumerJsonWhitespace $Text $Position
    if ($Position.Value -lt $Text.Length -and
        [int]$Text[$Position.Value] -eq 0x5d) {
        $Position.Value++
        return
    }
    while ($true) {
        Read-Gate1ConsumerJsonValue $Text $Position $Label
        Skip-Gate1ConsumerJsonWhitespace $Text $Position
        if ($Position.Value -ge $Text.Length) {
            throw "$Label contains an unterminated JSON array."
        }
        $delimiter = [int]$Text[$Position.Value]
        $Position.Value++
        if ($delimiter -eq 0x5d) { return }
        if ($delimiter -ne 0x2c) {
            throw "$Label contains invalid JSON array punctuation."
        }
        Skip-Gate1ConsumerJsonWhitespace $Text $Position
    }
}

function Assert-Gate1ConsumerUniqueJson {
    param([string]$Text, [string]$Label)

    $position = 0
    Read-Gate1ConsumerJsonValue $Text ([ref]$position) $Label
    Skip-Gate1ConsumerJsonWhitespace $Text ([ref]$position)
    if ($position -ne $Text.Length) {
        throw "$Label contains trailing JSON content."
    }
}

function Assert-Gate1ConsumerNoReparseAncestry {
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

function Resolve-Gate1ConsumerContainedFile {
    param(
        [string]$Root,
        [string]$PortablePath,
        [string]$Label)

    Assert-Gate1ConsumerPortablePath $PortablePath $Label
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $path = [IO.Path]::GetFullPath(
        (Join-Path $rootFull $PortablePath.Replace('/', '\')))
    $prefix = $rootFull + [IO.Path]::DirectorySeparatorChar
    if (-not $path.StartsWith(
            $prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label escaped its root."
    }
    Assert-Gate1ConsumerNoReparseAncestry $path $Label
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "$Label is missing."
    }
    return $path
}

function Read-Gate1ConsumerFileSnapshot {
    param([string]$Path, [string]$Label)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Label is missing."
    }
    Assert-Gate1ConsumerNoReparseAncestry $Path $Label
    $full = [IO.Path]::GetFullPath($Path)
    $before = Get-Item -LiteralPath $full -Force
    $bytes = [IO.File]::ReadAllBytes($full)
    $sha256 = Get-Gate1ConsumerSha256Bytes $bytes
    $after = Get-Item -LiteralPath $full -Force
    $finalSha = (Get-FileHash -LiteralPath $full -Algorithm SHA256).
        Hash.ToLowerInvariant()
    if ([long]$before.Length -ne [long]$after.Length -or
        [long]$after.Length -ne [long]$bytes.LongLength -or
        [long]$before.LastWriteTimeUtc.Ticks -ne
            [long]$after.LastWriteTimeUtc.Ticks -or
        $finalSha -cne $sha256) {
        throw "$Label changed while it was read."
    }
    return [pscustomobject][ordered]@{
        Path = $full
        Bytes = $bytes
        Length = [long]$bytes.LongLength
        Sha256 = $sha256
        LastWriteTimeUtcTicks = [long]$after.LastWriteTimeUtc.Ticks
    }
}

function Read-Gate1ConsumerJson {
    param(
        [string]$Path,
        [string]$Label,
        [switch]$Portable)

    $snapshot = Read-Gate1ConsumerFileSnapshot $Path $Label
    $bytes = [byte[]]$snapshot.Bytes
    if (($bytes.Length -ge 3 -and
            $bytes[0] -eq 0xef -and $bytes[1] -eq 0xbb -and
            $bytes[2] -eq 0xbf) -or
        ($bytes.Length -ge 2 -and
            (($bytes[0] -eq 0xff -and $bytes[1] -eq 0xfe) -or
             ($bytes[0] -eq 0xfe -and $bytes[1] -eq 0xff)))) {
        throw "$Label must be BOM-less UTF-8."
    }
    try {
        $text = (New-Object Text.UTF8Encoding($false, $true)).GetString($bytes)
    }
    catch { throw "$Label is not strict BOM-less UTF-8." }
    if ($text.IndexOf([char]0) -ge 0) {
        throw "$Label contains a NUL character."
    }
    Assert-Gate1ConsumerUniqueJson $text $Label
    if ($Portable) {
        Assert-Gate1ConsumerNoLocalPathText $text $Label
    }
    try { $value = $text | ConvertFrom-Json }
    catch { throw "$Label is not valid JSON." }
    return [pscustomobject][ordered]@{
        Artifact = $snapshot
        Text = $text
        Value = $value
    }
}

function Get-Gate1ConsumerTreeSnapshot {
    param([string]$Root, [string]$Label)

    if (-not (Test-Path -LiteralPath $Root -PathType Container)) {
        throw "$Label is missing."
    }
    Assert-Gate1ConsumerNoReparseAncestry $Root $Label
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $prefix = $rootFull + [IO.Path]::DirectorySeparatorChar
    $files = New-Object Collections.Generic.List[object]
    $directories = New-Object 'Collections.Generic.Stack[string]'
    $directories.Push($rootFull)
    while ($directories.Count -gt 0) {
        $directory = $directories.Pop()
        foreach ($item in @(Get-ChildItem -LiteralPath $directory -Force)) {
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "$Label contains a reparse point."
            }
            $full = [IO.Path]::GetFullPath($item.FullName)
            if (-not $full.StartsWith(
                    $prefix, [StringComparison]::OrdinalIgnoreCase)) {
                throw "$Label contains an escaped entry."
            }
            if ($item.PSIsContainer) {
                $directories.Push($full)
                continue
            }
            $relative = $full.Substring($prefix.Length).Replace('\', '/')
            Assert-Gate1ConsumerPortablePath $relative "$Label file path"
            $signature = Read-Gate1ConsumerFileSnapshot $full "$Label/$relative"
            [void]$files.Add([pscustomobject][ordered]@{
                path = $relative
                length = [long]$signature.Length
                sha256 = [string]$signature.Sha256
                lastWriteTimeUtcTicks = [long]$signature.LastWriteTimeUtcTicks
            })
        }
    }
    return [object[]]@($files.ToArray() | Sort-Object path)
}

function Assert-Gate1ConsumerTreeUnchanged {
    param([string]$Root, [object[]]$Expected, [string]$Label)

    $actual = @(Get-Gate1ConsumerTreeSnapshot $Root $Label)
    if (@($Expected).Count -ne $actual.Count) {
        throw "$Label changed during validation."
    }
    for ($index = 0; $index -lt $actual.Count; $index++) {
        foreach ($property in @(
                'path', 'length', 'sha256', 'lastWriteTimeUtcTicks')) {
            if ([string]$Expected[$index].$property -cne
                [string]$actual[$index].$property) {
                throw "$Label changed during validation."
            }
        }
    }
}

function Test-Gate1ConsumerSignature {
    param($Expected, $Actual)

    return $Expected.sha256 -is [string] -and
        $Actual.sha256 -is [string] -and
        (Test-Gate1ConsumerInteger $Expected.length 0) -and
        (Test-Gate1ConsumerInteger $Actual.length 0) -and
        [long]$Expected.length -eq [long]$Actual.length -and
        [string]$Expected.sha256 -ceq [string]$Actual.sha256
}

function Assert-Gate1ConsumerSignature {
    param($Value, [string]$Label)

    $names = @('length', 'sha256')
    if ($null -ne $Value -and
        $null -ne $Value.PSObject.Properties['path']) {
        $names = @('path', 'length', 'sha256')
    }
    Assert-Gate1ConsumerExactProperties $Value $names $Label
    if (-not (Test-Gate1ConsumerInteger $Value.length 0) -or
        [string]$Value.sha256 -cnotmatch '^[0-9a-f]{64}$') {
        throw "$Label is invalid."
    }
}

function Get-Gate1ConsumerRowsDigest {
    param([object[]]$Rows)

    $lines = @($Rows | Sort-Object path | ForEach-Object {
        "{0}`t{1}`t{2}" -f [string]$_.sha256,
            [long]$_.length, [string]$_.path
    })
    return Get-Gate1ConsumerSha256Text (($lines -join "`n") + "`n")
}

function Get-Gate1ConsumerPackageDigest {
    param([object[]]$Rows)

    $lines = @($Rows | Sort-Object indexPath | ForEach-Object {
        "{0}`t{1}`t{2}" -f [string]$_.sha256,
            [long]$_.length, [string]$_.indexPath
    })
    return Get-Gate1ConsumerSha256Text (($lines -join "`n") + "`n")
}

function Get-Gate1ConsumerGitBlobSignature {
    param([string]$RepositoryRoot, [string]$Commit, [string]$Path)

    Assert-Gate1ConsumerPortablePath $Path 'Git tool path'
    $blob = (@(& git -C $RepositoryRoot rev-parse `
        ($Commit + ':' + $Path) 2>$null) -join '').Trim()
    if ($LASTEXITCODE -ne 0 -or $blob -cnotmatch '^[0-9a-f]{40,64}$') {
        throw "Git tool blob $Path is missing."
    }
    $git = Get-Command git -CommandType Application -ErrorAction Stop |
        Select-Object -First 1
    $startInfo = New-Object Diagnostics.ProcessStartInfo
    $startInfo.FileName = $git.Source
    $startInfo.Arguments = 'cat-file blob ' + $blob
    $startInfo.WorkingDirectory = $RepositoryRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $process = New-Object Diagnostics.Process
    $memory = New-Object IO.MemoryStream
    try {
        $process.StartInfo = $startInfo
        if (-not $process.Start()) { throw 'Git tool blob capture did not start.' }
        $process.StandardOutput.BaseStream.CopyTo($memory)
        $errorText = $process.StandardError.ReadToEnd()
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) {
            throw ('Git tool blob capture failed: ' + $errorText.Trim())
        }
        $bytes = $memory.ToArray()
        return [pscustomobject][ordered]@{
            length = [long]$bytes.Length
            sha256 = Get-Gate1ConsumerSha256Bytes $bytes
        }
    }
    finally {
        $memory.Dispose()
        $process.Dispose()
    }
}

function Assert-Gate1ConsumerGitAndTools {
    param(
        [string]$RepositoryRoot,
        [string]$CandidateHead,
        [string]$HarnessHead,
        [object[]]$Tools,
        [Collections.IDictionary]$ExpectedTools,
        [string]$Label)

    foreach ($head in @($CandidateHead, $HarnessHead)) {
        if ($head -cnotmatch '^[0-9a-f]{40}$') {
            throw "$Label contains an invalid Git commit."
        }
        & git -C $RepositoryRoot cat-file -e ($head + '^{commit}') 2>$null
        if ($LASTEXITCODE -ne 0) { throw "$Label contains an unknown Git commit." }
    }
    & git -C $RepositoryRoot merge-base --is-ancestor `
        $CandidateHead $HarnessHead 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw "$Label harness commit is not a candidate descendant."
    }
    $checkoutHead = (@(& git -C $RepositoryRoot rev-parse HEAD 2>$null) -join '').Trim()
    if ($LASTEXITCODE -ne 0 -or $checkoutHead -cnotmatch '^[0-9a-f]{40}$') {
        throw "$Label cannot resolve checkout HEAD."
    }
    & git -C $RepositoryRoot merge-base --is-ancestor `
        $HarnessHead $checkoutHead 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw "$Label harness commit is not an ancestor of checkout HEAD."
    }

    if (@($Tools).Count -ne $ExpectedTools.Count) {
        throw "$Label tool set is incomplete."
    }
    $expectedToolPaths = New-Object `
        'Collections.Generic.Dictionary[string,string]' `
        ([StringComparer]::Ordinal)
    foreach ($expectedRole in $ExpectedTools.Keys) {
        $expectedToolPaths.Add(
            [string]$expectedRole,
            [string]$ExpectedTools[$expectedRole])
    }
    $seen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($tool in @($Tools)) {
        Assert-Gate1ConsumerExactProperties $tool `
            @('role', 'path', 'length', 'sha256') "$Label tool row"
        $role = [string]$tool.role
        if (-not $expectedToolPaths.ContainsKey($role) -or
            -not $seen.Add($role) -or
            [string]$tool.path -cne [string]$expectedToolPaths[$role] -or
            -not (Test-Gate1ConsumerInteger $tool.length 1) -or
            [string]$tool.sha256 -cnotmatch '^[0-9a-f]{64}$') {
            throw "$Label tool role, path, or signature is invalid."
        }
        $worktreePath = Resolve-Gate1ConsumerContainedFile `
            $RepositoryRoot ([string]$tool.path) "$Label worktree tool"
        $worktree = Read-Gate1ConsumerFileSnapshot `
            $worktreePath "$Label worktree tool"
        $blob = Get-Gate1ConsumerGitBlobSignature `
            $RepositoryRoot $HarnessHead ([string]$tool.path)
        if (-not (Test-Gate1ConsumerSignature $tool $worktree) -or
            -not (Test-Gate1ConsumerSignature $tool $blob)) {
            throw "$Label tool differs from its worktree or harness Git blob."
        }
    }
    foreach ($expectedRole in $ExpectedTools.Keys) {
        if (-not $seen.Contains([string]$expectedRole)) {
            throw "$Label tool set is incomplete."
        }
    }
}

function Invoke-Gate1ConsumerPublisherVerification {
    param(
        [string]$RepositoryRoot,
        [string]$PublisherPortablePath,
        [string]$RunEnvelopePath,
        [string]$ValidationKind,
        [string]$RunId,
        [string]$CandidateId,
        [string]$CandidateBindingSha256,
        [long]$FileCount,
        $IndexArtifact,
        $ReadyArtifact,
        [string]$Disposition,
        [string]$Label)

    Assert-Gate1ConsumerPortablePath `
        $PublisherPortablePath "$Label publisher path"
    $publisherPath = Resolve-Gate1ConsumerContainedFile `
        $RepositoryRoot $PublisherPortablePath "$Label publisher"
    $watchRoot = Split-Path -Parent ([IO.Path]::GetFullPath($RunEnvelopePath))
    $watcher = New-Object IO.FileSystemWatcher($watchRoot)
    $watcher.IncludeSubdirectories = $true
    $watcher.NotifyFilter = [IO.NotifyFilters]::FileName -bor
        [IO.NotifyFilters]::DirectoryName -bor
        [IO.NotifyFilters]::LastWrite -bor
        [IO.NotifyFilters]::Size -bor
        [IO.NotifyFilters]::Attributes
    $eventPrefix = 'PartisanGate1PublisherVerify' +
        [Guid]::NewGuid().ToString('N')
    $subscriptions = New-Object Collections.Generic.List[string]
    $publisherWriteEvents = @()
    try {
        foreach ($eventName in @('Changed', 'Created', 'Deleted', 'Renamed')) {
            $sourceIdentifier = $eventPrefix + $eventName
            Register-ObjectEvent `
                -InputObject $watcher `
                -EventName $eventName `
                -SourceIdentifier $sourceIdentifier
            [void]$subscriptions.Add($sourceIdentifier)
        }
        $watcher.EnableRaisingEvents = $true
        $publisherOutput = @(& $publisherPath `
            -RunEnvelopePath $RunEnvelopePath `
            -VerifyPublishedIndex)
        [Threading.Thread]::Sleep(100)
        $publisherWriteEvents = @(Get-Event | Where-Object {
            $_.SourceIdentifier.StartsWith(
                $eventPrefix, [StringComparison]::Ordinal)
        })
    }
    finally {
        $watcher.EnableRaisingEvents = $false
        foreach ($sourceIdentifier in $subscriptions) {
            Unregister-Event -SourceIdentifier $sourceIdentifier `
                -ErrorAction SilentlyContinue
        }
        foreach ($eventRecord in @(Get-Event | Where-Object {
                $_.SourceIdentifier.StartsWith(
                    $eventPrefix, [StringComparison]::Ordinal)
            })) {
            Remove-Event -EventIdentifier $eventRecord.EventIdentifier `
                -ErrorAction SilentlyContinue
        }
        $watcher.Dispose()
    }
    if ($publisherWriteEvents.Count -ne 0) {
        throw "$Label publisher verification wrote within the raw run."
    }
    if ($publisherOutput.Count -ne 1 -or
        $publisherOutput[0] -isnot [PSCustomObject]) {
        throw "$Label publisher verification did not return exactly one object."
    }

    $result = $publisherOutput[0]
    Assert-Gate1ConsumerExactProperties $result @(
        'ValidationKind', 'PublishedIndexValid', 'ReadySealValid',
        'ReadOnlyVerification', 'SyntheticFixture', 'RunId', 'CandidateId',
        'CandidateBindingSha256', 'FileCount', 'IndexSignature',
        'ReadySignature', 'Disposition') "$Label publisher verification"
    Assert-Gate1ConsumerInteger $result.FileCount $FileCount `
        "$Label publisher verification.FileCount"
    Assert-Gate1ConsumerSignature $result.IndexSignature `
        "$Label publisher verification index signature"
    Assert-Gate1ConsumerSignature $result.ReadySignature `
        "$Label publisher verification ready signature"
    foreach ($textProperty in @(
            'ValidationKind', 'RunId', 'CandidateId',
            'CandidateBindingSha256', 'Disposition')) {
        [void](Require-Gate1ConsumerText `
            $result.$textProperty `
            "$Label publisher verification.$textProperty")
    }
    if (
        [string]$result.ValidationKind -cne $ValidationKind -or
        $result.PublishedIndexValid -isnot [bool] -or
        -not [bool]$result.PublishedIndexValid -or
        $result.ReadySealValid -isnot [bool] -or
        -not [bool]$result.ReadySealValid -or
        $result.ReadOnlyVerification -isnot [bool] -or
        -not [bool]$result.ReadOnlyVerification -or
        $result.SyntheticFixture -isnot [bool] -or
        [bool]$result.SyntheticFixture -or
        [string]$result.RunId -cne $RunId -or
        [string]$result.CandidateId -cne $CandidateId -or
        [string]$result.CandidateBindingSha256 -cne
            $CandidateBindingSha256 -or
        -not (Test-Gate1ConsumerSignature `
            $result.IndexSignature $IndexArtifact) -or
        -not (Test-Gate1ConsumerSignature `
            $result.ReadySignature $ReadyArtifact) -or
        [string]$result.Disposition -cne $Disposition) {
        throw "$Label publisher verification result is not exact."
    }
}

function Assert-Gate1ConsumerCandidateCore {
    param($Candidate, $Expected, [string]$Label, [switch]$Retention)

    $names = @(
        'candidateId', 'candidateVersion', 'runtimeUseDisposition', 'gitHead',
        'embeddedBuildSha', 'embeddedBuildUtc', 'embeddedBuildLabel',
        'campaignSchema', 'runtimeSettingsSchema', 'addonId', 'addonGuid',
        'packageHashAlgorithm', 'packageSha256', 'manifestSha256', 'readySha256',
        'workbenchCrc')
    if ($Retention) {
        $names += @('manifestPath', 'readyPath', 'packageRoot', 'packageFiles',
            'executables')
    }
    else {
        $names += @('runtimeRole', 'diagnosticExecutable',
            'recordedDiagnosticExecutable', 'recordedRuntimeExecutable')
    }
    Assert-Gate1ConsumerExactProperties $Candidate $names $Label
    if (-not (Test-Gate1ConsumerInteger $Candidate.campaignSchema 1) -or
        -not (Test-Gate1ConsumerInteger $Candidate.runtimeSettingsSchema 1)) {
        throw "$Label schema values must be exact positive JSON integers."
    }
    $manifest = $Expected.Manifest
    $checks = [ordered]@{
        candidateId = $Expected.CandidateId
        candidateVersion = $Expected.PackageVersion
        runtimeUseDisposition = 'active-runtime-candidate'
        gitHead = $Expected.CandidateSourceHead
        embeddedBuildSha = $Expected.EmbeddedSha
        embeddedBuildUtc = $Expected.EmbeddedUtc
        embeddedBuildLabel = $Expected.EmbeddedLabel
        campaignSchema = [int]$Expected.CampaignSchema
        runtimeSettingsSchema = [int]$Expected.RuntimeSettingsSchema
        addonId = [string]$manifest.addon.id
        addonGuid = [string]$manifest.addon.guid
        packageHashAlgorithm = $Expected.PackageHashAlgorithm
        packageSha256 = $Expected.PackageSha256
        manifestSha256 = $Expected.ManifestSha256
        readySha256 = $Expected.ReadySha256
        workbenchCrc = $Expected.WorkbenchCrc
    }
    foreach ($name in $checks.Keys) {
        if ([string]$Candidate.$name -cne [string]$checks[$name]) {
            throw "$Label differs from the active candidate at $name."
        }
    }
    if (-not $Retention -and [string]$Candidate.runtimeRole -cne 'server') {
        throw "$Label is not the exact server candidate tuple."
    }
}

function Assert-Gate1ConsumerProvenance {
    param($Value, [string]$Label)

    Assert-Gate1ConsumerExactProperties $Value `
        @('fileName', 'fileVersion', 'productVersion', 'length', 'sha256') $Label
    if ($Value.fileName -isnot [string] -or
        [string]::IsNullOrWhiteSpace([string]$Value.fileName) -or
        $Value.fileVersion -isnot [string] -or
        [string]::IsNullOrWhiteSpace([string]$Value.fileVersion) -or
        $Value.productVersion -isnot [string] -or
        [string]::IsNullOrWhiteSpace([string]$Value.productVersion) -or
        -not (Test-Gate1ConsumerInteger $Value.length 1) -or
        $Value.sha256 -isnot [string] -or
        [string]$Value.sha256 -cnotmatch '^[0-9a-f]{64}$') {
        throw "$Label is invalid."
    }
}

function Assert-Gate1ConsumerPackageRows {
    param([object[]]$Rows, [string]$ExpectedDigest, [string]$Label)

    if (@($Rows).Count -ne 4) { throw "$Label must contain four files." }
    $expectedPaths = @(
        'Partisan/addon.gproj', 'Partisan/data.pak',
        'Partisan/resourceDatabase.rdb', 'Partisan/thumbnail.png')
    $seen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($row in @($Rows)) {
        Assert-Gate1ConsumerExactProperties $row `
            @('indexPath', 'length', 'sha256') "$Label row"
        if ($row.indexPath -isnot [string] -or
            [string]$row.indexPath -cnotin $expectedPaths -or
            -not $seen.Add([string]$row.indexPath) -or
            -not (Test-Gate1ConsumerInteger $row.length 1) -or
            $row.sha256 -isnot [string] -or
            [string]$row.sha256 -cnotmatch '^[0-9a-f]{64}$') {
            throw "$Label contains an invalid package row."
        }
    }
    if ((Get-Gate1ConsumerPackageDigest $Rows) -cne $ExpectedDigest) {
        throw "$Label canonical digest is invalid."
    }
}

function Assert-Gate1ConsumerManifestAndReady {
    param(
        [string]$RunRoot,
        [string]$ManifestPath,
        [string]$ReadyPath,
        $Candidate,
        $Expected,
        [string]$Label)

    $manifestFull = Resolve-Gate1ConsumerContainedFile `
        $RunRoot $ManifestPath "$Label candidate manifest"
    $readyFull = Resolve-Gate1ConsumerContainedFile `
        $RunRoot $ReadyPath "$Label candidate ready seal"
    $manifestParsed = Read-Gate1ConsumerJson `
        $manifestFull "$Label candidate manifest" -Portable
    $readyParsed = Read-Gate1ConsumerJson `
        $readyFull "$Label candidate ready seal" -Portable
    if ([string]$manifestParsed.Artifact.Sha256 -cne
            [string]$Expected.ManifestSha256 -or
        [string]$readyParsed.Artifact.Sha256 -cne [string]$Expected.ReadySha256) {
        throw "$Label candidate manifest or ready bytes are not exact."
    }
    $manifest = $manifestParsed.Value
    $ready = $readyParsed.Value
    if ([string]$manifest.candidate.id -cne [string]$Candidate.candidateId -or
        [string]$manifest.candidate.version -cne
            [string]$Candidate.candidateVersion -or
        [string]$manifest.source.gitHead -cne [string]$Candidate.gitHead -or
        [string]$manifest.package.sha256 -cne
            [string]$Candidate.packageSha256 -or
        [string]$ready.candidateId -cne [string]$Candidate.candidateId -or
        [string]$ready.gitHead -cne [string]$Candidate.gitHead -or
        [string]$ready.packageSha256 -cne
            [string]$Candidate.packageSha256 -or
        [string]$ready.manifestSha256 -cne
            [string]$Candidate.manifestSha256) {
        throw "$Label copied candidate identity is not exact."
    }
}

function Assert-Gate1ConsumerFileRows {
    param(
        [object[]]$Rows,
        [object[]]$Tree,
        [string[]]$ControlPaths,
        [string]$ExpectedDigest,
        [string]$Label,
        [switch]$WithRole)

    $seen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($row in @($Rows)) {
        $names = @('path', 'length', 'sha256')
        if ($WithRole) { $names = @('role', 'stage') + $names }
        Assert-Gate1ConsumerExactProperties $row $names "$Label file row"
        if ($row.path -isnot [string]) {
            throw "$Label contains a non-string file path."
        }
        Assert-Gate1ConsumerPortablePath ([string]$row.path) "$Label file path"
        if (-not $seen.Add([string]$row.path) -or
            -not (Test-Gate1ConsumerInteger $row.length 0) -or
            $row.sha256 -isnot [string] -or
            [string]$row.sha256 -cnotmatch '^[0-9a-f]{64}$') {
            throw "$Label contains an invalid or duplicate file row."
        }
        $actual = @($Tree | Where-Object {
            [string]$_.path -ceq [string]$row.path
        })
        if ($actual.Count -ne 1 -or
            -not (Test-Gate1ConsumerSignature $row $actual[0])) {
            throw "$Label file differs from its census."
        }
    }
    if ((Get-Gate1ConsumerRowsDigest $Rows) -cne $ExpectedDigest) {
        throw "$Label aggregate digest is invalid."
    }
    $expected = @($Rows | ForEach-Object { [string]$_.path }) + $ControlPaths
    $actualPaths = @($Tree | ForEach-Object { [string]$_.path })
    if (@(Compare-Object -ReferenceObject @($expected | Sort-Object) `
            -DifferenceObject @($actualPaths | Sort-Object) `
            -CaseSensitive).Count -ne 0) {
        throw "$Label is not a complete and exact file census."
    }
}

function Get-Gate1ConsumerTreeRow {
    param([object[]]$Tree, [string]$Path, [string]$Label)

    $rows = @($Tree | Where-Object { [string]$_.path -ceq $Path })
    if ($rows.Count -ne 1) { throw "$Label is missing from the run tree." }
    return $rows[0]
}

function Test-Gate1ConsumerJsonEqual {
    param($Expected, $Actual)

    return ($Expected | ConvertTo-Json -Compress -Depth 100) -ceq
        ($Actual | ConvertTo-Json -Compress -Depth 100)
}

function Assert-Gate1ConsumerAllJsonStrict {
    param([string]$RunRoot, [object[]]$Tree, [string]$Label)

    foreach ($row in @($Tree | Where-Object {
            [string]$_.path -cmatch '\.json$'
        })) {
        $path = Resolve-Gate1ConsumerContainedFile `
            $RunRoot ([string]$row.path) "$Label JSON path"
        $null = Read-Gate1ConsumerJson $path "$Label/$($row.path)"
    }
}

function Resolve-Gate1ConsumerTrackedIndex {
    param(
        [string]$RepositoryRoot,
        [string]$SummaryPath,
        [string]$ExpectedPrefix,
        [string]$Label)

    Assert-Gate1ConsumerPortablePath $SummaryPath "$Label.summaryPath"
    if (-not $SummaryPath.StartsWith(
            $ExpectedPrefix, [StringComparison]::Ordinal) -or
        -not $SummaryPath.EndsWith('.json', [StringComparison]::Ordinal)) {
        throw "$Label.summaryPath is outside its tracked evidence directory."
    }
    return Resolve-Gate1ConsumerContainedFile `
        $RepositoryRoot $SummaryPath "$Label tracked release index"
}

function Resolve-Gate1ConsumerRunRoot {
    param(
        [string]$EvidenceBundleRoot,
        [string]$RelativePath,
        [string]$Label)

    if ([string]::IsNullOrWhiteSpace($EvidenceBundleRoot)) {
        throw "$Label requires an explicit EvidenceBundleRoot."
    }
    $evidenceRoot = [IO.Path]::GetFullPath($EvidenceBundleRoot).TrimEnd('\', '/')
    if (-not (Test-Path -LiteralPath $evidenceRoot -PathType Container)) {
        throw "$Label EvidenceBundleRoot is missing."
    }
    Assert-Gate1ConsumerNoReparseAncestry $evidenceRoot `
        "$Label EvidenceBundleRoot"
    Assert-Gate1ConsumerPortablePath $RelativePath "$Label bundle path"
    $runRoot = [IO.Path]::GetFullPath(
        (Join-Path $evidenceRoot $RelativePath.Replace('/', '\')))
    $prefix = $evidenceRoot + [IO.Path]::DirectorySeparatorChar
    if (-not $runRoot.StartsWith(
            $prefix, [StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-Path -LiteralPath $runRoot -PathType Container)) {
        throw "$Label raw run is missing or outside EvidenceBundleRoot."
    }
    Assert-Gate1ConsumerNoReparseAncestry $runRoot "$Label raw run"
    return $runRoot
}

function Assert-Gate1ConsumerStatusWindow {
    param(
        $Record,
        $CandidateIdentity,
        [DateTimeOffset]$StatusAsOfUtc,
        [string]$ExpectedDisposition,
        [string]$Label)

    if ([string]$Record.status -cne 'passed-noncertifying' -or
        [string]$Record.disposition -cne $ExpectedDisposition) {
        throw "$Label does not retain the exact non-certifying disposition."
    }
    foreach ($name in @(
            'summarySha256', 'runReadySha256', 'packageSha256',
            'candidateBindingSha256')) {
        $null = Require-Gate1ConsumerSha256 $Record.$name "$Label.$name"
    }
    if ([string]$Record.candidateId -cne
            [string]$CandidateIdentity.CandidateId -or
        [string]$Record.candidateSourceHead -cne
            [string]$CandidateIdentity.CandidateSourceHead -or
        [string]$Record.packageSha256 -cne
            [string]$CandidateIdentity.PackageSha256 -or
        [string]$Record.harnessGitHead -cnotmatch '^[0-9a-f]{40}$' -or
        [string]$Record.runId -cnotmatch '^[A-Za-z0-9][A-Za-z0-9._-]+$' -or
        [string]::IsNullOrWhiteSpace([string]$Record.runLeafId)) {
        throw "$Label is not bound to the exact active candidate or run."
    }
    $started = Require-Gate1ConsumerUtc $Record.startedUtc "$Label.startedUtc"
    $completed = Require-Gate1ConsumerUtc `
        $Record.completedUtc "$Label.completedUtc"
    if ($completed -lt $started -or $completed -gt $StatusAsOfUtc -or
        $started -lt [DateTimeOffset]$CandidateIdentity.CreatedUtc) {
        throw "$Label timestamps do not fit the active candidate/status window."
    }
    $summary = Require-Gate1ConsumerText $Record.summary "$Label.summary"
    Assert-Gate1ConsumerNoLocalPathText $summary "$Label.summary"
    return [pscustomobject][ordered]@{
        StartedUtc = $started
        CompletedUtc = $completed
    }
}

function Assert-PartisanReleaseSurfaceEvidence {
    param(
        $Record,
        $CandidateIdentity,
        [DateTimeOffset]$StatusAsOfUtc,
        [string]$RepositoryRoot,
        [string]$EvidenceBundleRoot,
        [string]$Label)

    Assert-Gate1ConsumerExactProperties $Record @(
        'status', 'summaryPath', 'summarySha256', 'runReadySha256',
        'candidateId', 'candidateSourceHead', 'packageSha256',
        'candidateBindingSha256', 'harnessGitHead', 'runId', 'runLeafId',
        'startedUtc', 'completedUtc', 'disposition',
        'certificationPromotion', 'summary') $Label
    $window = Assert-Gate1ConsumerStatusWindow `
        $Record $CandidateIdentity $StatusAsOfUtc `
        $script:SurfaceDisposition $Label
    if ([string]$Record.certificationPromotion -cne 'none' -or
        [string]$Record.runId -cnotmatch
            '^release_surface_[0-9]{8}T[0-9]{6}Z_[0-9a-f]{20}$' -or
        [string]$Record.runLeafId -cnotmatch
            '^[0-9]{8}T[0-9]{6}Z-[0-9a-f]{32}$') {
        throw "$Label identity or no-promotion contract is invalid."
    }

    $summaryPath = Require-Gate1ConsumerText `
        $Record.summaryPath "$Label.summaryPath"
    $trackedPath = Resolve-Gate1ConsumerTrackedIndex `
        $RepositoryRoot $summaryPath 'docs/evidence/release-surface-audit/' $Label
    $tracked = Read-Gate1ConsumerJson `
        $trackedPath "$Label tracked release index" -Portable
    if ([string]$tracked.Artifact.Sha256 -cne
        [string]$Record.summarySha256) {
        throw "$Label tracked release-index SHA-256 differs from status."
    }
    $index = $tracked.Value
    Assert-Gate1ConsumerExactProperties $index @(
        'schemaVersion', 'contractId', 'evidenceKind', 'runId', 'runLeafId',
        'runNonce', 'source', 'candidate', 'candidateBindingSha256', 'run',
        'evidenceIndex', 'contract', 'harness', 'capture', 'modes', 'cleanup',
        'validation', 'files', 'filesAggregateSha256', 'limitations',
        'disposition', 'certificationPromotion', 'passed') `
        "$Label tracked release index"
    Assert-Gate1ConsumerInteger $index.schemaVersion 1 `
        "$Label tracked release index.schemaVersion"
    if (
        [string]$index.contractId -cne
            'partisan.release-surface-audit.index.v1' -or
        [string]$index.evidenceKind -cne $script:SurfaceEvidenceKind -or
        [string]$index.runId -cne [string]$Record.runId -or
        [string]$index.runLeafId -cne [string]$Record.runLeafId -or
        [string]$index.runNonce -cnotmatch '^[0-9a-f]{32}$' -or
        [string]$index.candidateBindingSha256 -cne
            [string]$Record.candidateBindingSha256 -or
        [string]$index.disposition -cne $script:SurfaceDisposition -or
        [string]$index.certificationPromotion -cne 'none' -or
        $index.passed -isnot [bool] -or -not [bool]$index.passed) {
        throw "$Label tracked release-index identity is invalid."
    }
    Assert-Gate1ConsumerExactProperties $index.source @(
        'bundleRelativePath', 'candidateGitHead', 'harnessGitHead',
        'checkoutClean', 'candidateAncestor', 'executionMode') `
        "$Label release-index source"
    $expectedBundle = [string]$Record.candidateId +
        '/release-surface-audit/' + [string]$Record.runLeafId
    if ([string]$index.source.bundleRelativePath -cne $expectedBundle -or
        [string]$index.source.candidateGitHead -cne
            [string]$Record.candidateSourceHead -or
        [string]$index.source.harnessGitHead -cne
            [string]$Record.harnessGitHead -or
        $index.source.checkoutClean -isnot [bool] -or
        -not [bool]$index.source.checkoutClean -or
        $index.source.candidateAncestor -isnot [bool] -or
        -not [bool]$index.source.candidateAncestor -or
        [string]$index.source.executionMode -cne
            'paired-native-engine-audit') {
        throw "$Label release-index source binding is invalid."
    }
    Assert-Gate1ConsumerCandidateCore `
        $index.candidate $CandidateIdentity "$Label release-index candidate"

    $runRoot = Resolve-Gate1ConsumerRunRoot `
        $EvidenceBundleRoot $expectedBundle $Label
    $tree = @(Get-Gate1ConsumerTreeSnapshot $runRoot "$Label raw run")
    try {
        $externalIndexPath = Resolve-Gate1ConsumerContainedFile `
            $runRoot 'release-index.json' "$Label external release index"
        $externalIndex = Read-Gate1ConsumerJson `
            $externalIndexPath "$Label external release index" -Portable
        if ([string]$externalIndex.Artifact.Sha256 -cne
                [string]$tracked.Artifact.Sha256 -or
            [long]$externalIndex.Artifact.Length -ne
                [long]$tracked.Artifact.Length -or
            -not (Test-Gate1ConsumerJsonEqual $index $externalIndex.Value)) {
            throw "$Label tracked and external release indexes are not exact."
        }

        $runPath = Resolve-Gate1ConsumerContainedFile `
            $runRoot 'run.json' "$Label run envelope"
        $runParsed = Read-Gate1ConsumerJson `
            $runPath "$Label run envelope" -Portable
        Assert-Gate1ConsumerSignature $index.run "$Label run signature"
        if ([string]$index.run.path -cne 'run.json' -or
            -not (Test-Gate1ConsumerSignature `
                $index.run $runParsed.Artifact)) {
            throw "$Label run envelope differs from its release-index seal."
        }
        $run = $runParsed.Value
        Assert-Gate1ConsumerExactProperties $run @(
            'schemaVersion', 'evidenceKind', 'contractId', 'runId',
            'runLeafId', 'runNonce', 'startedUtc', 'completedUtc',
            'disposition', 'certificationPromotion', 'source', 'candidate',
            'candidateBindingSha256', 'pairedSamePackage',
            'candidatePackageSha256', 'harnessGuid', 'harnessSha256',
            'modes', 'cleanup', 'evidenceIndex', 'limitations', 'passed') `
            "$Label run envelope"
        Assert-Gate1ConsumerInteger $run.schemaVersion 1 `
            "$Label run envelope.schemaVersion"
        if (
            [string]$run.evidenceKind -cne $script:SurfaceRunEvidenceKind -or
            [string]$run.contractId -cne
                'partisan.release-surface-audit.run.v1' -or
            [string]$run.runId -cne [string]$Record.runId -or
            [string]$run.runLeafId -cne [string]$Record.runLeafId -or
            [string]$run.runNonce -cne [string]$index.runNonce -or
            [string]$run.startedUtc -cne [string]$Record.startedUtc -or
            [string]$run.completedUtc -cne [string]$Record.completedUtc -or
            [string]$run.disposition -cne $script:SurfaceDisposition -or
            [string]$run.certificationPromotion -cne 'none' -or
            [string]$run.candidateBindingSha256 -cne
                [string]$Record.candidateBindingSha256 -or
            [string]$run.candidatePackageSha256 -cne
                [string]$CandidateIdentity.PackageSha256 -or
            $run.pairedSamePackage -isnot [bool] -or
            -not [bool]$run.pairedSamePackage -or
            $run.passed -isnot [bool] -or -not [bool]$run.passed) {
            throw "$Label run envelope identity or outcome is invalid."
        }
        Assert-Gate1ConsumerExactProperties $run.source @(
            'harnessGitHead', 'checkoutClean', 'candidateAncestor',
            'executionMode') "$Label run source"
        if ([string]$run.source.harnessGitHead -cne
                [string]$Record.harnessGitHead -or
            $run.source.checkoutClean -isnot [bool] -or
            -not [bool]$run.source.checkoutClean -or
            $run.source.candidateAncestor -isnot [bool] -or
            -not [bool]$run.source.candidateAncestor -or
            [string]$run.source.executionMode -cne
                'paired-native-engine-audit') {
            throw "$Label run source is not a clean paired audit."
        }
        Assert-Gate1ConsumerCandidateCore `
            $run.candidate $CandidateIdentity "$Label run candidate"
        if (-not (Test-Gate1ConsumerJsonEqual `
                $index.candidate $run.candidate)) {
            throw "$Label run and release-index candidate tuples differ."
        }

        Assert-Gate1ConsumerExactProperties $index.capture @(
            'startedUtc', 'completedUtc', 'modeCount', 'pairedOrder') `
            "$Label release-index capture"
        Assert-Gate1ConsumerInteger $index.capture.modeCount 2 `
            "$Label release-index capture.modeCount"
        Assert-Gate1ConsumerArray $index.capture.pairedOrder `
            "$Label paired mode order"
        if ([string]$index.capture.startedUtc -cne
                [string]$Record.startedUtc -or
            [string]$index.capture.completedUtc -cne
                [string]$Record.completedUtc) {
            throw "$Label release-index capture timestamps or count differ."
        }
        Assert-Gate1ConsumerStringArrayExact `
            @('retail', 'diagnostic') @($index.capture.pairedOrder) `
            "$Label paired mode order"

        Assert-Gate1ConsumerExactProperties $run.cleanup @(
            'schemaVersion', 'evidenceKind', 'harnessRemoved',
            'harnessResidueCount', 'exactOwnerVerifiedBeforeRemoval', 'passed') `
            "$Label cleanup"
        Assert-Gate1ConsumerInteger $run.cleanup.schemaVersion 1 `
            "$Label cleanup.schemaVersion"
        Assert-Gate1ConsumerInteger $run.cleanup.harnessResidueCount 0 `
            "$Label cleanup.harnessResidueCount"
        if (
            [string]$run.cleanup.evidenceKind -cne
                $script:SurfaceRunEvidenceKind -or
            $run.cleanup.harnessRemoved -isnot [bool] -or
            -not [bool]$run.cleanup.harnessRemoved -or
            $run.cleanup.exactOwnerVerifiedBeforeRemoval -isnot [bool] -or
            -not [bool]$run.cleanup.exactOwnerVerifiedBeforeRemoval -or
            $run.cleanup.passed -isnot [bool] -or
            -not [bool]$run.cleanup.passed -or
            -not (Test-Gate1ConsumerJsonEqual $run.cleanup $index.cleanup)) {
            throw "$Label cleanup is not exact and residue-free."
        }

        Assert-Gate1ConsumerExactProperties $run.evidenceIndex @(
            'path', 'signature', 'aggregateSha256', 'fileCount') `
            "$Label run evidence-index binding"
        Assert-Gate1ConsumerSignature $run.evidenceIndex.signature `
            "$Label evidence-index signature"
        Assert-Gate1ConsumerExactProperties $index.evidenceIndex @(
            'path', 'length', 'sha256', 'fileCount',
            'filesAggregateSha256') "$Label release-index evidence binding"
        $evidenceIndexPath = Resolve-Gate1ConsumerContainedFile `
            $runRoot 'evidence-index.json' "$Label evidence index"
        $evidenceIndexParsed = Read-Gate1ConsumerJson `
            $evidenceIndexPath "$Label evidence index" -Portable
        if ([string]$run.evidenceIndex.path -cne 'evidence-index.json' -or
            [string]$index.evidenceIndex.path -cne 'evidence-index.json' -or
            -not (Test-Gate1ConsumerSignature `
                $run.evidenceIndex.signature $evidenceIndexParsed.Artifact) -or
            -not (Test-Gate1ConsumerSignature `
                $index.evidenceIndex $evidenceIndexParsed.Artifact)) {
            throw "$Label evidence-index signatures are not exact."
        }
        $evidenceIndex = $evidenceIndexParsed.Value
        Assert-Gate1ConsumerExactProperties $evidenceIndex @(
            'schemaVersion', 'evidenceKind', 'files', 'aggregateSha256') `
            "$Label evidence index"
        Assert-Gate1ConsumerInteger $evidenceIndex.schemaVersion 1 `
            "$Label evidence index.schemaVersion"
        Assert-Gate1ConsumerInteger $run.evidenceIndex.fileCount `
            @($index.files).Count "$Label run evidence-index fileCount"
        Assert-Gate1ConsumerInteger $index.evidenceIndex.fileCount `
            @($index.files).Count "$Label release-index evidence fileCount"
        if (
            [string]$evidenceIndex.evidenceKind -cne
                $script:SurfaceRunEvidenceKind -or
            [string]$evidenceIndex.aggregateSha256 -cne
                [string]$index.filesAggregateSha256 -or
            [string]$run.evidenceIndex.aggregateSha256 -cne
                [string]$index.filesAggregateSha256 -or
            [string]$index.evidenceIndex.filesAggregateSha256 -cne
                [string]$index.filesAggregateSha256 -or
            -not (Test-Gate1ConsumerJsonEqual `
                @($evidenceIndex.files) @($index.files))) {
            throw "$Label evidence-index projection differs from release index."
        }
        Assert-Gate1ConsumerFileRows @($index.files) $tree @(
            'evidence-index.json', 'run.json', 'release-index.json',
            'run.ready.json') ([string]$index.filesAggregateSha256) `
            "$Label raw run"

        $bindingsPath = Resolve-Gate1ConsumerContainedFile `
            $runRoot 'identity/bindings.json' "$Label identity bindings"
        $bindings = (Read-Gate1ConsumerJson `
            $bindingsPath "$Label identity bindings" -Portable).Value
        Assert-Gate1ConsumerExactProperties $bindings @(
            'schemaVersion', 'evidenceKind', 'source', 'candidate', 'package',
            'executables', 'harness', 'tools', 'host') `
            "$Label identity bindings"
        Assert-Gate1ConsumerInteger $bindings.schemaVersion 1 `
            "$Label identity bindings.schemaVersion"
        if (
            [string]$bindings.evidenceKind -cne $script:SurfaceRunEvidenceKind -or
            -not (Test-Gate1ConsumerJsonEqual `
                $bindings.candidate $index.candidate)) {
            throw "$Label identity bindings are not candidate-bound."
        }
        Assert-Gate1ConsumerExactProperties $bindings.source @(
            'harnessGitHead', 'checkoutClean', 'candidateAncestor',
            'executionMode') "$Label binding source"
        if ([string]$bindings.source.harnessGitHead -cne
                [string]$Record.harnessGitHead -or
            $bindings.source.checkoutClean -isnot [bool] -or
            -not [bool]$bindings.source.checkoutClean -or
            $bindings.source.candidateAncestor -isnot [bool] -or
            -not [bool]$bindings.source.candidateAncestor -or
            [string]$bindings.source.executionMode -cne
                'paired-native-engine-audit') {
            throw "$Label identity source is not clean and ancestry-bound."
        }
        Assert-Gate1ConsumerExactProperties $bindings.package @(
            'hashAlgorithm', 'sha256', 'files') "$Label package binding"
        if ([string]$bindings.package.hashAlgorithm -cne
                'sha256-manifest-v1' -or
            [string]$bindings.package.sha256 -cne
                [string]$CandidateIdentity.PackageSha256) {
            throw "$Label package binding is invalid."
        }
        Assert-Gate1ConsumerPackageRows @($bindings.package.files) `
            ([string]$CandidateIdentity.PackageSha256) "$Label package binding"

        Assert-Gate1ConsumerExactProperties $index.harness @(
            'id', 'guid', 'aggregateSha256', 'files', 'tools') `
            "$Label release-index harness"
        if ([string]$index.harness.id -cne 'PartisanReleaseSurfaceAudit' -or
            [string]$index.harness.guid -cnotmatch '^[0-9A-F]{16}$' -or
            [string]$index.harness.aggregateSha256 -cnotmatch
                '^[0-9a-f]{64}$' -or
            -not (Test-Gate1ConsumerJsonEqual `
                @($index.harness.tools) @($bindings.tools))) {
            throw "$Label harness identity or tool projection is invalid."
        }
        Assert-Gate1ConsumerGitAndTools `
            $RepositoryRoot ([string]$CandidateIdentity.CandidateSourceHead) `
            ([string]$Record.harnessGitHead) @($index.harness.tools) `
            $script:SurfaceToolPaths "$Label surface publisher"

        Assert-Gate1ConsumerManifestAndReady `
            $runRoot 'identity/candidate.json' 'identity/candidate.ready.json' `
            $index.candidate $CandidateIdentity $Label

        $runModes = @($run.modes)
        $indexModes = @($index.modes)
        if ($runModes.Count -ne 2 -or $indexModes.Count -ne 2) {
            throw "$Label must contain the exact retail/diagnostic pair."
        }
        $contextIds = New-Object 'Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
        for ($ordinal = 0; $ordinal -lt 2; $ordinal++) {
            $mode = @('retail', 'diagnostic')[$ordinal]
            Assert-Gate1ConsumerExactProperties $runModes[$ordinal] @(
                'mode', 'path', 'signature') "$Label $mode mode binding"
            Assert-Gate1ConsumerSignature $runModes[$ordinal].signature `
                "$Label $mode mode signature"
            $expectedModePath = 'modes/' + $mode + '.json'
            if ([string]$runModes[$ordinal].mode -cne $mode -or
                [string]$runModes[$ordinal].path -cne $expectedModePath) {
                throw "$Label $mode mode path or order is invalid."
            }
            $modePath = Resolve-Gate1ConsumerContainedFile `
                $runRoot $expectedModePath "$Label $mode envelope"
            $modeParsed = Read-Gate1ConsumerJson `
                $modePath "$Label $mode envelope" -Portable
            if (-not (Test-Gate1ConsumerSignature `
                    $runModes[$ordinal].signature $modeParsed.Artifact)) {
                throw "$Label $mode envelope signature is invalid."
            }
            $modeValue = $modeParsed.Value
            Assert-Gate1ConsumerExactProperties $modeValue @(
                'schemaVersion', 'evidenceKind', 'mode', 'disposition',
                'candidateId', 'packageSha256', 'manifestSha256', 'readySha256',
                'executable', 'harnessGuid', 'harnessSha256', 'process',
                'arguments', 'streams', 'probe', 'classification', 'passed') `
                "$Label $mode envelope"
            Assert-Gate1ConsumerInteger $modeValue.schemaVersion 1 `
                "$Label $mode envelope.schemaVersion"
            if (
                [string]$modeValue.evidenceKind -cne
                    $script:SurfaceRunEvidenceKind -or
                [string]$modeValue.mode -cne $mode -or
                [string]$modeValue.disposition -cne $script:SurfaceDisposition -or
                [string]$modeValue.candidateId -cne
                    [string]$CandidateIdentity.CandidateId -or
                [string]$modeValue.packageSha256 -cne
                    [string]$CandidateIdentity.PackageSha256 -or
                [string]$modeValue.manifestSha256 -cne
                    [string]$CandidateIdentity.ManifestSha256 -or
                [string]$modeValue.readySha256 -cne
                    [string]$CandidateIdentity.ReadySha256 -or
                [string]$modeValue.harnessGuid -cne
                    [string]$index.harness.guid -or
                [string]$modeValue.harnessSha256 -cne
                    [string]$index.harness.aggregateSha256 -or
                [string]$modeValue.process.candidateBindingSha256 -cne
                    [string]$Record.candidateBindingSha256 -or
                $modeValue.passed -isnot [bool] -or
                -not [bool]$modeValue.passed) {
                throw "$Label $mode envelope is not exact-package bound."
            }
            if (-not $contextIds.Add([string]$modeValue.process.contextId)) {
                throw "$Label paired modes reused one guarded context."
            }
            Assert-Gate1ConsumerExactProperties $indexModes[$ordinal] @(
                'mode', 'path', 'signature', 'executable', 'contextId',
                'candidateBindingSha256', 'forbiddenTypeCount',
                'productionTypeCount', 'forbiddenMemberCount',
                'productionMemberCount', 'forbiddenCommandCount',
                'productionCommandCount', 'hardDiagnosticCount',
                'candidateMountLineCount', 'candidatePackedMountLineCount',
                'harnessMountLineCount', 'uniqueResultMarkerCount',
                'crashArtifactCount', 'passed') "$Label $mode index projection"
            Assert-Gate1ConsumerInteger `
                $indexModes[$ordinal].forbiddenMemberCount `
                $script:SurfaceForbiddenMemberCount `
                "$Label $mode forbiddenMemberCount"
            Assert-Gate1ConsumerInteger `
                $indexModes[$ordinal].productionMemberCount `
                $script:SurfaceProductionMemberCount `
                "$Label $mode productionMemberCount"
            Assert-Gate1ConsumerInteger `
                $indexModes[$ordinal].hardDiagnosticCount 0 `
                "$Label $mode hardDiagnosticCount"
            Assert-Gate1ConsumerInteger `
                $indexModes[$ordinal].crashArtifactCount 0 `
                "$Label $mode crashArtifactCount"
            if ([string]$indexModes[$ordinal].mode -cne $mode -or
                [string]$indexModes[$ordinal].path -cne $expectedModePath -or
                -not (Test-Gate1ConsumerSignature `
                    $indexModes[$ordinal].signature $modeParsed.Artifact) -or
                [string]$indexModes[$ordinal].contextId -cne
                    [string]$modeValue.process.contextId -or
                [string]$indexModes[$ordinal].candidateBindingSha256 -cne
                    [string]$Record.candidateBindingSha256 -or
                $indexModes[$ordinal].passed -isnot [bool] -or
                -not [bool]$indexModes[$ordinal].passed) {
                throw "$Label $mode release-index projection is invalid."
            }
        }

        Assert-Gate1ConsumerExactProperties $index.validation @(
            'candidateTupleExact', 'packageCanonicalDigestExact',
            'executableProvenanceExact', 'harnessSourceAndToolsExact',
            'guardedReceiptsComplete', 'candidateBindingShared',
            'pairedModeOrderExact', 'contractSetsExact',
            'positiveControlsPresent', 'hardDiagnosticsAbsent',
            'crashArtifactsAbsent', 'harnessResidueAbsent',
            'portablePathsExact', 'duplicateJsonKeysAbsent',
            'fullFileCensusExact') "$Label release-index validation"
        foreach ($property in $index.validation.PSObject.Properties) {
            if ($property.Value -isnot [bool] -or -not [bool]$property.Value) {
                throw "$Label release-index validation is not all true."
            }
        }

        $readyPath = Resolve-Gate1ConsumerContainedFile `
            $runRoot 'run.ready.json' "$Label terminal ready seal"
        $readyParsed = Read-Gate1ConsumerJson `
            $readyPath "$Label terminal ready seal" -Portable
        if ([string]$readyParsed.Artifact.Sha256 -cne
            [string]$Record.runReadySha256) {
            throw "$Label terminal ready SHA-256 differs from status."
        }
        $ready = $readyParsed.Value
        Assert-Gate1ConsumerExactProperties $ready @(
            'schemaVersion', 'evidenceKind', 'disposition',
            'certificationPromotion', 'runId', 'runLeafId', 'source',
            'candidateId', 'packageSha256', 'manifestSha256', 'readySha256',
            'candidate', 'candidateBindingSha256', 'run', 'evidenceIndex',
            'releaseIndex', 'cleanupPassed', 'sealedLast') `
            "$Label terminal ready seal"
        Assert-Gate1ConsumerInteger $ready.schemaVersion 1 `
            "$Label terminal ready seal.schemaVersion"
        if (
            [string]$ready.evidenceKind -cne $script:SurfaceRunEvidenceKind -or
            [string]$ready.disposition -cne $script:SurfaceDisposition -or
            [string]$ready.certificationPromotion -cne 'none' -or
            [string]$ready.runId -cne [string]$Record.runId -or
            [string]$ready.runLeafId -cne [string]$Record.runLeafId -or
            [string]$ready.candidateId -cne
                [string]$CandidateIdentity.CandidateId -or
            [string]$ready.packageSha256 -cne
                [string]$CandidateIdentity.PackageSha256 -or
            [string]$ready.manifestSha256 -cne
                [string]$CandidateIdentity.ManifestSha256 -or
            [string]$ready.readySha256 -cne
                [string]$CandidateIdentity.ReadySha256 -or
            [string]$ready.candidateBindingSha256 -cne
                [string]$Record.candidateBindingSha256 -or
            $ready.cleanupPassed -isnot [bool] -or
            -not [bool]$ready.cleanupPassed -or
            $ready.sealedLast -isnot [bool] -or
            -not [bool]$ready.sealedLast -or
            -not (Test-Gate1ConsumerJsonEqual `
                $ready.candidate $index.candidate)) {
            throw "$Label terminal ready seal is not exact and non-certifying."
        }
        Assert-Gate1ConsumerExactProperties $ready.source @(
            'candidateGitHead', 'harnessGitHead') "$Label ready source"
        if ([string]$ready.source.candidateGitHead -cne
                [string]$CandidateIdentity.CandidateSourceHead -or
            [string]$ready.source.harnessGitHead -cne
                [string]$Record.harnessGitHead) {
            throw "$Label ready source binding is invalid."
        }
        foreach ($binding in @(
                [pscustomobject]@{
                    Value = $ready.run
                    Path = 'run.json'
                    Artifact = $runParsed.Artifact
                },
                [pscustomobject]@{
                    Value = $ready.evidenceIndex
                    Path = 'evidence-index.json'
                    Artifact = $evidenceIndexParsed.Artifact
                },
                [pscustomobject]@{
                    Value = $ready.releaseIndex
                    Path = 'release-index.json'
                    Artifact = $externalIndex.Artifact
                })) {
            Assert-Gate1ConsumerExactProperties $binding.Value @(
                'path', 'signature') "$Label ready artifact binding"
            Assert-Gate1ConsumerSignature $binding.Value.signature `
                "$Label ready artifact signature"
            if ([string]$binding.Value.path -cne $binding.Path -or
                -not (Test-Gate1ConsumerSignature `
                    $binding.Value.signature $binding.Artifact)) {
                throw "$Label ready artifact binding is invalid."
            }
        }

        Assert-Gate1ConsumerAllJsonStrict $runRoot $tree $Label
        Invoke-Gate1ConsumerPublisherVerification `
            -RepositoryRoot $RepositoryRoot `
            -PublisherPortablePath `
                $script:SurfaceToolPaths.releaseIndexProducer `
            -RunEnvelopePath $runPath `
            -ValidationKind `
                'partisan_release_surface_published_index_verification_v1' `
            -RunId ([string]$Record.runId) `
            -CandidateId ([string]$Record.candidateId) `
            -CandidateBindingSha256 `
                ([string]$Record.candidateBindingSha256) `
            -FileCount @($index.files).Count `
            -IndexArtifact $externalIndex.Artifact `
            -ReadyArtifact $readyParsed.Artifact `
            -Disposition $script:SurfaceDisposition `
            -Label $Label
        return [pscustomobject][ordered]@{
            Kind = 'release-surface-audit'
            SummaryPath = $summaryPath
            SummarySha256 = [string]$tracked.Artifact.Sha256
            RunReadySha256 = [string]$readyParsed.Artifact.Sha256
            CandidateId = [string]$Record.candidateId
            PackageSha256 = [string]$Record.packageSha256
            CandidateBindingSha256 = [string]$Record.candidateBindingSha256
            HarnessGitHead = [string]$Record.harnessGitHead
            RunId = [string]$Record.runId
            RunLeafId = [string]$Record.runLeafId
            StartedUtc = $window.StartedUtc
            CompletedUtc = $window.CompletedUtc
            Disposition = $script:SurfaceDisposition
            CertificationPromotion = 'none'
            FileCount = @($index.files).Count
        }
    }
    finally {
        Assert-Gate1ConsumerTreeUnchanged $runRoot $tree "$Label raw run"
    }
}

function Assert-PartisanGate1RuntimeRetentionEvidence {
    param(
        $Record,
        $CandidateIdentity,
        [DateTimeOffset]$StatusAsOfUtc,
        [string]$RepositoryRoot,
        [string]$EvidenceBundleRoot,
        [string]$Label)

    Assert-Gate1ConsumerExactProperties $Record @(
        'status', 'summaryPath', 'summarySha256', 'runReadySha256',
        'candidateId', 'candidateSourceHead', 'packageSha256',
        'candidateBindingSha256', 'harnessGitHead', 'runId', 'runLeafId',
        'startedUtc', 'completedUtc', 'disposition', 'certificationClaim',
        'standardSaveRestorationCertified', 'summary') $Label
    $window = Assert-Gate1ConsumerStatusWindow `
        $Record $CandidateIdentity $StatusAsOfUtc `
        $script:RetentionDisposition $Label
    if ([string]$Record.certificationClaim -cne 'none' -or
        $Record.standardSaveRestorationCertified -isnot [bool] -or
        [bool]$Record.standardSaveRestorationCertified -or
        [string]$Record.runId -cnotmatch
            '^gate1_[0-9]{8}T[0-9]{6}Z_[0-9a-f]{20}$' -or
        [string]$Record.runLeafId -cnotmatch
            '^[0-9]{8}T[0-9]{6}Z-[0-9a-f]{12}$') {
        throw "$Label identity or non-certifying retention contract is invalid."
    }
    $runParts = ([string]$Record.runId).Split('_')
    $leafParts = ([string]$Record.runLeafId).Split('-')
    if ($runParts.Count -ne 3 -or $leafParts.Count -ne 2 -or
        $runParts[1] -cne $leafParts[0] -or
        -not $runParts[2].StartsWith(
            $leafParts[1], [StringComparison]::Ordinal)) {
        throw "$Label run ID is not bound to its external run-root leaf."
    }

    $summaryPath = Require-Gate1ConsumerText `
        $Record.summaryPath "$Label.summaryPath"
    $trackedPath = Resolve-Gate1ConsumerTrackedIndex `
        $RepositoryRoot $summaryPath `
        'docs/evidence/gate1-runtime-retention/' $Label
    $tracked = Read-Gate1ConsumerJson `
        $trackedPath "$Label tracked release index" -Portable
    if ([string]$tracked.Artifact.Sha256 -cne
        [string]$Record.summarySha256) {
        throw "$Label tracked release-index SHA-256 differs from status."
    }
    $index = $tracked.Value
    Assert-Gate1ConsumerExactProperties $index @(
        'schemaVersion', 'contractId', 'evidenceKind', 'runId', 'candidateId',
        'gitHead', 'packageSha256', 'candidateBindingSha256', 'run',
        'topology', 'validation', 'files', 'filesAggregateSha256',
        'disposition') "$Label tracked release index"
    Assert-Gate1ConsumerInteger $index.schemaVersion 1 `
        "$Label tracked release index.schemaVersion"
    if (
        [string]$index.contractId -cne
            'partisan.gate1-runtime-retention.index.v1' -or
        [string]$index.evidenceKind -cne $script:RetentionEvidenceKind -or
        [string]$index.runId -cne [string]$Record.runId -or
        [string]$index.candidateId -cne [string]$Record.candidateId -or
        [string]$index.gitHead -cne [string]$Record.candidateSourceHead -or
        [string]$index.packageSha256 -cne [string]$Record.packageSha256 -or
        [string]$index.candidateBindingSha256 -cne
            [string]$Record.candidateBindingSha256 -or
        [string]$index.disposition -cne $script:RetentionDisposition) {
        throw "$Label tracked release-index identity is invalid."
    }

    $bundlePath = [string]$Record.candidateId +
        '/gate1-runtime-retention/' + [string]$Record.runLeafId
    $runRoot = Resolve-Gate1ConsumerRunRoot `
        $EvidenceBundleRoot $bundlePath $Label
    $tree = @(Get-Gate1ConsumerTreeSnapshot $runRoot "$Label raw run")
    try {
        $externalIndexPath = Resolve-Gate1ConsumerContainedFile `
            $runRoot 'release-index.json' "$Label external release index"
        $externalIndex = Read-Gate1ConsumerJson `
            $externalIndexPath "$Label external release index" -Portable
        if ([string]$externalIndex.Artifact.Sha256 -cne
                [string]$tracked.Artifact.Sha256 -or
            [long]$externalIndex.Artifact.Length -ne
                [long]$tracked.Artifact.Length -or
            -not (Test-Gate1ConsumerJsonEqual $index $externalIndex.Value)) {
            throw "$Label tracked and external release indexes are not exact."
        }

        Assert-Gate1ConsumerSignature $index.run "$Label run signature"
        $runPath = Resolve-Gate1ConsumerContainedFile `
            $runRoot 'run.json' "$Label run envelope"
        $runParsed = Read-Gate1ConsumerJson `
            $runPath "$Label run envelope" -Portable
        if ([string]$index.run.path -cne 'run.json' -or
            -not (Test-Gate1ConsumerSignature $index.run $runParsed.Artifact)) {
            throw "$Label run envelope differs from its release-index seal."
        }
        $run = $runParsed.Value
        Assert-Gate1ConsumerExactProperties $run @(
            'schemaVersion', 'evidenceKind', 'contractId', 'runId',
            'startedUtc', 'completedUtc', 'candidate', 'harness', 'scenario',
            'configuration', 'lineage', 'runtime', 'persistence', 'outcome',
            'files') "$Label run envelope"
        Assert-Gate1ConsumerInteger $run.schemaVersion 1 `
            "$Label run envelope.schemaVersion"
        if (
            [string]$run.evidenceKind -cne $script:RetentionEvidenceKind -or
            [string]$run.contractId -cne
                'partisan.gate1-runtime-retention.v1' -or
            [string]$run.runId -cne [string]$Record.runId -or
            [string]$run.startedUtc -cne [string]$Record.startedUtc -or
            [string]$run.completedUtc -cne [string]$Record.completedUtc) {
            throw "$Label run envelope identity or timestamps are invalid."
        }
        Assert-Gate1ConsumerCandidateCore `
            $run.candidate $CandidateIdentity "$Label run candidate" -Retention
        if ([string]$run.candidate.candidateId -cne
                [string]$index.candidateId -or
            [string]$run.candidate.gitHead -cne [string]$index.gitHead -or
            [string]$run.candidate.packageSha256 -cne
                [string]$index.packageSha256) {
            throw "$Label run and release-index candidate tuples differ."
        }

        Assert-Gate1ConsumerExactProperties $run.candidate.executables @(
            'serverDiagnostic', 'clientDiagnostic', 'server', 'client') `
            "$Label candidate executable pair"
        foreach ($name in @(
                'serverDiagnostic', 'clientDiagnostic', 'server', 'client')) {
            Assert-Gate1ConsumerProvenance $run.candidate.executables.$name `
                "$Label candidate executable $name"
        }
        if ([string]$run.candidate.executables.server.fileName -cne
                'ArmaReforgerServer.exe' -or
            [string]$run.candidate.executables.client.fileName -cne
                'ArmaReforgerSteam.exe' -or
            [string]$run.candidate.executables.serverDiagnostic.fileName -cne
                'ArmaReforgerServerDiag.exe' -or
            [string]$run.candidate.executables.clientDiagnostic.fileName -cne
                'ArmaReforgerSteamDiag.exe') {
            throw "$Label standard/diagnostic executable pair is invalid."
        }

        $packageRows = @($run.candidate.packageFiles)
        if ($packageRows.Count -ne 4) {
            throw "$Label copied package inventory must contain four files."
        }
        $packageProjections = New-Object Collections.Generic.List[object]
        foreach ($row in $packageRows) {
            Assert-Gate1ConsumerExactProperties $row @(
                'indexPath', 'path', 'length', 'sha256') `
                "$Label copied package row"
            $path = Resolve-Gate1ConsumerContainedFile `
                $runRoot ([string]$row.path) "$Label copied package file"
            $actual = Read-Gate1ConsumerFileSnapshot `
                $path "$Label copied package file"
            if (-not (Test-Gate1ConsumerSignature $row $actual)) {
                throw "$Label copied package file differs from its row."
            }
            [void]$packageProjections.Add([pscustomobject][ordered]@{
                indexPath = [string]$row.indexPath
                length = [long]$row.length
                sha256 = [string]$row.sha256
            })
        }
        Assert-Gate1ConsumerPackageRows $packageProjections.ToArray() `
            ([string]$CandidateIdentity.PackageSha256) "$Label copied package"
        Assert-Gate1ConsumerManifestAndReady `
            $runRoot ([string]$run.candidate.manifestPath) `
            ([string]$run.candidate.readyPath) $run.candidate `
            $CandidateIdentity $Label

        Assert-Gate1ConsumerExactProperties $run.harness @(
            'gitHead', 'clean', 'tools') "$Label harness"
        if ([string]$run.harness.gitHead -cne
                [string]$Record.harnessGitHead -or
            $run.harness.clean -isnot [bool] -or
            -not [bool]$run.harness.clean) {
            throw "$Label harness is not clean or status-bound."
        }
        Assert-Gate1ConsumerGitAndTools `
            $RepositoryRoot ([string]$CandidateIdentity.CandidateSourceHead) `
            ([string]$Record.harnessGitHead) @($run.harness.tools) `
            $script:RetentionToolPaths "$Label retention publisher"

        Assert-Gate1ConsumerExactProperties $run.scenario @(
            'executionMode', 'worldResource', 'missionHeader', 'stages',
            'claimScope', 'certificationClaim') "$Label scenario"
        if ([string]$run.scenario.executionMode -cne 'two-phase-engine' -or
            [string]$run.scenario.worldResource -cne
                'Worlds/HST_Everon/HST_Everon.ent' -or
            [string]$run.scenario.missionHeader -cne
                'Missions/HST_Everon.conf' -or
            [string]$run.scenario.claimScope -cne 'raw-retention-only' -or
            [string]$run.scenario.certificationClaim -cne 'none') {
            throw "$Label scenario claims more than raw retention."
        }
        Assert-Gate1ConsumerStringArrayExact `
            $script:RetentionStages @($run.scenario.stages) `
            "$Label scenario stages"

        Assert-Gate1ConsumerExactProperties $run.lineage @(
            'executionClass', 'retailClaim', 'contexts') "$Label lineage"
        Assert-Gate1ConsumerExactProperties $run.runtime @(
            'executionClass', 'mutationAuthority', 'byteStabilityClaim',
            'contexts') "$Label standard runtime"
        if ([string]$run.lineage.executionClass -cne
                'diagnostic-only-save-lineage' -or
            [string]$run.lineage.retailClaim -cne 'none' -or
            [string]$run.runtime.executionClass -cne
                'standard-load-start-log-retention' -or
            [string]$run.runtime.mutationAuthority -cne 'none' -or
            [string]$run.runtime.byteStabilityClaim -cne 'observation-only' -or
            @($run.lineage.contexts).Count -ne 5 -or
            @($run.runtime.contexts).Count -ne 5) {
            throw "$Label two-phase authority boundary is invalid."
        }
        foreach ($contexts in @(
                @($run.lineage.contexts), @($run.runtime.contexts))) {
            for ($ordinal = 0; $ordinal -lt 5; $ordinal++) {
                $context = $contexts[$ordinal]
                Assert-Gate1ConsumerInteger `
                    (Get-Gate1ConsumerProperty $context 'ordinal') $ordinal `
                    "$Label context ordinal"
                if ([string](Get-Gate1ConsumerProperty $context 'stage') -cne
                        [string]$script:RetentionStages[$ordinal] -or
                    [string](Get-Gate1ConsumerProperty `
                        $context 'candidateBindingSha256') -cne
                        [string]$Record.candidateBindingSha256) {
                    throw "$Label context is not stage- and candidate-bound."
                }
            }
        }

        Assert-Gate1ConsumerExactProperties $run.outcome @(
            'success', 'diagnosticServerLaunchCount',
            'diagnosticClientLaunchCount', 'standardServerLaunchCount',
            'standardClientLaunchCount', 'configsRetained', 'logsRetained',
            'saveBytesRetained', 'guardedReceiptsComplete',
            'standardSaveBytesStable', 'disposition') "$Label outcome"
        foreach ($countBinding in @(
                [pscustomobject]@{
                    Value = $run.outcome.diagnosticServerLaunchCount
                    Expected = 5; Name = 'diagnosticServerLaunchCount'
                },
                [pscustomobject]@{
                    Value = $run.outcome.diagnosticClientLaunchCount
                    Expected = 1; Name = 'diagnosticClientLaunchCount'
                },
                [pscustomobject]@{
                    Value = $run.outcome.standardServerLaunchCount
                    Expected = 5; Name = 'standardServerLaunchCount'
                },
                [pscustomobject]@{
                    Value = $run.outcome.standardClientLaunchCount
                    Expected = 1; Name = 'standardClientLaunchCount'
                })) {
            Assert-Gate1ConsumerInteger $countBinding.Value `
                $countBinding.Expected "$Label outcome.$($countBinding.Name)"
        }
        if ($run.outcome.success -isnot [bool] -or
            -not [bool]$run.outcome.success -or
            $run.outcome.configsRetained -isnot [bool] -or
            -not [bool]$run.outcome.configsRetained -or
            $run.outcome.logsRetained -isnot [bool] -or
            -not [bool]$run.outcome.logsRetained -or
            $run.outcome.saveBytesRetained -isnot [bool] -or
            -not [bool]$run.outcome.saveBytesRetained -or
            $run.outcome.guardedReceiptsComplete -isnot [bool] -or
            -not [bool]$run.outcome.guardedReceiptsComplete -or
            $run.outcome.standardSaveBytesStable -isnot [bool] -or
            -not [bool]$run.outcome.standardSaveBytesStable -or
            [string]$run.outcome.disposition -cne
                $script:RetentionDisposition) {
            throw "$Label outcome does not retain the exact non-certifying proof."
        }

        Assert-Gate1ConsumerExactProperties $index.topology @(
            'stageCount', 'diagnosticContextCount',
            'diagnosticServerLaunchCount', 'diagnosticClientLaunchCount',
            'standardContextCount', 'standardServerLaunchCount',
            'standardClientLaunchCount', 'standardStages') `
            "$Label release-index topology"
        foreach ($countBinding in @(
                [pscustomobject]@{ Value = $index.topology.stageCount
                    Expected = 5; Name = 'stageCount' },
                [pscustomobject]@{ Value = $index.topology.diagnosticContextCount
                    Expected = 5; Name = 'diagnosticContextCount' },
                [pscustomobject]@{ Value = $index.topology.diagnosticServerLaunchCount
                    Expected = 5; Name = 'diagnosticServerLaunchCount' },
                [pscustomobject]@{ Value = $index.topology.diagnosticClientLaunchCount
                    Expected = 1; Name = 'diagnosticClientLaunchCount' },
                [pscustomobject]@{ Value = $index.topology.standardContextCount
                    Expected = 5; Name = 'standardContextCount' },
                [pscustomobject]@{ Value = $index.topology.standardServerLaunchCount
                    Expected = 5; Name = 'standardServerLaunchCount' },
                [pscustomobject]@{ Value = $index.topology.standardClientLaunchCount
                    Expected = 1; Name = 'standardClientLaunchCount' })) {
            Assert-Gate1ConsumerInteger $countBinding.Value `
                $countBinding.Expected `
                "$Label topology.$($countBinding.Name)"
        }
        Assert-Gate1ConsumerArray $index.topology.standardStages `
            "$Label release-index standard stages"
        if (@($index.topology.standardStages).Count -ne 5) {
            throw "$Label release-index topology is invalid."
        }
        for ($ordinal = 0; $ordinal -lt 5; $ordinal++) {
            $row = $index.topology.standardStages[$ordinal]
            Assert-Gate1ConsumerExactProperties $row @(
                'ordinal', 'stage', 'contextId', 'candidateBindingSha256',
                'serverStopDisposition', 'clientLaunched', 'receiptPath',
                'receiptSha256') "$Label standard-stage projection"
            Assert-Gate1ConsumerInteger $row.ordinal $ordinal `
                "$Label standard-stage projection.ordinal"
            if ([string]$row.stage -cne $script:RetentionStages[$ordinal] -or
                [string]$row.candidateBindingSha256 -cne
                    [string]$Record.candidateBindingSha256 -or
                [string]$row.receiptSha256 -cnotmatch '^[0-9a-f]{64}$') {
                throw "$Label standard-stage projection is invalid."
            }
        }

        Assert-Gate1ConsumerExactProperties $index.validation @(
            'candidateAndReadyExact', 'packageCanonicalDigestExact',
            'guardedReceiptsComplete', 'diagnosticAndStandardPhasesDisjoint',
            'argumentTopologyExact', 'snapshotsExact', 'readOnlyStagesExact',
            'standardSaveByteStabilityObserved',
            'standardSaveRestorationCertified', 'fullFileCensusExact',
            'executionMode') "$Label release-index validation"
        foreach ($name in @(
                'candidateAndReadyExact', 'packageCanonicalDigestExact',
                'guardedReceiptsComplete',
                'diagnosticAndStandardPhasesDisjoint', 'argumentTopologyExact',
                'snapshotsExact', 'readOnlyStagesExact',
                'standardSaveByteStabilityObserved', 'fullFileCensusExact')) {
            if ($index.validation.$name -isnot [bool] -or
                -not [bool]$index.validation.$name) {
                throw "$Label release-index validation.$name is not true."
            }
        }
        if ($index.validation.standardSaveRestorationCertified -isnot [bool] -or
            [bool]$index.validation.standardSaveRestorationCertified -or
            [string]$index.validation.executionMode -cne 'two-phase-engine') {
            throw "$Label may not promote byte stability to save restoration."
        }

        if (-not (Test-Gate1ConsumerJsonEqual `
                @($run.files) @($index.files))) {
            throw "$Label run and release-index file rows differ."
        }
        Assert-Gate1ConsumerFileRows @($index.files) $tree @(
            'run.json', 'release-index.json', 'run.ready.json') `
            ([string]$index.filesAggregateSha256) "$Label raw run" -WithRole

        $readyPath = Resolve-Gate1ConsumerContainedFile `
            $runRoot 'run.ready.json' "$Label terminal ready seal"
        $readyParsed = Read-Gate1ConsumerJson `
            $readyPath "$Label terminal ready seal" -Portable
        if ([string]$readyParsed.Artifact.Sha256 -cne
            [string]$Record.runReadySha256) {
            throw "$Label terminal ready SHA-256 differs from status."
        }
        $ready = $readyParsed.Value
        Assert-Gate1ConsumerExactProperties $ready @(
            'schemaVersion', 'contractId', 'evidenceKind', 'runId',
            'candidateId', 'run', 'index', 'disposition', 'publishedUtc') `
            "$Label terminal ready seal"
        Assert-Gate1ConsumerSignature $ready.run "$Label ready run signature"
        Assert-Gate1ConsumerSignature $ready.index `
            "$Label ready release-index signature"
        $published = Require-Gate1ConsumerUtc `
            $ready.publishedUtc "$Label ready.publishedUtc"
        Assert-Gate1ConsumerInteger $ready.schemaVersion 1 `
            "$Label terminal ready seal.schemaVersion"
        if (
            [string]$ready.contractId -cne
                'partisan.gate1-runtime-retention.ready.v1' -or
            [string]$ready.evidenceKind -cne $script:RetentionEvidenceKind -or
            [string]$ready.runId -cne [string]$Record.runId -or
            [string]$ready.candidateId -cne [string]$Record.candidateId -or
            [string]$ready.run.path -cne 'run.json' -or
            [string]$ready.index.path -cne 'release-index.json' -or
            -not (Test-Gate1ConsumerSignature `
                $ready.run $runParsed.Artifact) -or
            -not (Test-Gate1ConsumerSignature `
                $ready.index $externalIndex.Artifact) -or
            [string]$ready.disposition -cne $script:RetentionDisposition -or
            $published -lt $window.CompletedUtc -or
            $published -gt $StatusAsOfUtc) {
            throw "$Label terminal ready seal is invalid."
        }

        Assert-Gate1ConsumerAllJsonStrict $runRoot $tree $Label
        Invoke-Gate1ConsumerPublisherVerification `
            -RepositoryRoot $RepositoryRoot `
            -PublisherPortablePath `
                $script:RetentionToolPaths['gate1-index-producer'] `
            -RunEnvelopePath $runPath `
            -ValidationKind `
                'partisan_gate1_runtime_retention_published_index_verification_v1' `
            -RunId ([string]$Record.runId) `
            -CandidateId ([string]$Record.candidateId) `
            -CandidateBindingSha256 `
                ([string]$Record.candidateBindingSha256) `
            -FileCount @($index.files).Count `
            -IndexArtifact $externalIndex.Artifact `
            -ReadyArtifact $readyParsed.Artifact `
            -Disposition $script:RetentionDisposition `
            -Label $Label
        return [pscustomobject][ordered]@{
            Kind = 'gate1-runtime-retention'
            SummaryPath = $summaryPath
            SummarySha256 = [string]$tracked.Artifact.Sha256
            RunReadySha256 = [string]$readyParsed.Artifact.Sha256
            CandidateId = [string]$Record.candidateId
            PackageSha256 = [string]$Record.packageSha256
            CandidateBindingSha256 = [string]$Record.candidateBindingSha256
            HarnessGitHead = [string]$Record.harnessGitHead
            RunId = [string]$Record.runId
            RunLeafId = [string]$Record.runLeafId
            StartedUtc = $window.StartedUtc
            CompletedUtc = $window.CompletedUtc
            Disposition = $script:RetentionDisposition
            CertificationClaim = 'none'
            StandardSaveRestorationCertified = $false
            FileCount = @($index.files).Count
        }
    }
    finally {
        Assert-Gate1ConsumerTreeUnchanged $runRoot $tree "$Label raw run"
    }
}

function Assert-PartisanGate1EvidencePair {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Evidence,
        [Parameter(Mandatory = $true)]$CandidateIdentity,
        [Parameter(Mandatory = $true)][DateTimeOffset]$StatusAsOfUtc,
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [AllowEmptyString()][string]$EvidenceBundleRoot = '',
        [switch]$FullCampaignDebugAccepted)

    if ($null -eq $Evidence -or $Evidence -isnot [PSCustomObject]) {
        throw 'Active Gate 1 evidence container must be a JSON object.'
    }
    $surfaceProperties = @($Evidence.PSObject.Properties | Where-Object {
        [string]$_.Name -ceq 'releaseSurfaceAudit'
    })
    $retentionProperties = @($Evidence.PSObject.Properties | Where-Object {
        [string]$_.Name -ceq 'gate1RuntimeRetention'
    })
    $caseVariants = @($Evidence.PSObject.Properties | Where-Object {
        (([string]$_.Name -ieq 'releaseSurfaceAudit') -and
            ([string]$_.Name -cne 'releaseSurfaceAudit')) -or
        (([string]$_.Name -ieq 'gate1RuntimeRetention') -and
            ([string]$_.Name -cne 'gate1RuntimeRetention'))
    })
    if ($caseVariants.Count -ne 0 -or
        $surfaceProperties.Count -gt 1 -or $retentionProperties.Count -gt 1) {
        throw 'Active Gate 1 evidence property names must use exact case once.'
    }
    $surfacePresent = $surfaceProperties.Count -eq 1
    $retentionPresent = $retentionProperties.Count -eq 1
    if ($surfacePresent -xor $retentionPresent) {
        throw ('Active Gate 1 evidence must contain both releaseSurfaceAudit ' +
            'and gate1RuntimeRetention or neither.')
    }
    if (-not $surfacePresent) {
        if ($FullCampaignDebugAccepted) {
            throw ('Accepted active Full Campaign Debug requires both Gate 1 ' +
                'release-surface and runtime-retention evidence records.')
        }
        return [pscustomobject][ordered]@{
            Present = $false
            ReleaseSurfaceAudit = $null
            Gate1RuntimeRetention = $null
        }
    }
    $surface = $surfaceProperties[0].Value
    $retention = $retentionProperties[0].Value
    if ($null -eq $surface -or $null -eq $retention) {
        throw 'Active Gate 1 evidence records must be non-null JSON objects.'
    }
    if ([string]::IsNullOrWhiteSpace($EvidenceBundleRoot)) {
        throw 'Active Gate 1 evidence requires an explicit EvidenceBundleRoot.'
    }
    $repository = [IO.Path]::GetFullPath($RepositoryRoot)
    if (-not (Test-Path -LiteralPath $repository -PathType Container)) {
        throw 'Gate 1 evidence consumer repository root is missing.'
    }
    Assert-Gate1ConsumerNoReparseAncestry $repository `
        'Gate 1 evidence consumer repository root'
    $surfaceValidation = Assert-PartisanReleaseSurfaceEvidence `
        $surface $CandidateIdentity $StatusAsOfUtc $repository `
        $EvidenceBundleRoot 'release_status.evidence.releaseSurfaceAudit'
    $retentionValidation = Assert-PartisanGate1RuntimeRetentionEvidence `
        $retention $CandidateIdentity $StatusAsOfUtc $repository `
        $EvidenceBundleRoot 'release_status.evidence.gate1RuntimeRetention'
    if ([string]$surfaceValidation.SummaryPath -ceq
            [string]$retentionValidation.SummaryPath -or
        [string]$surfaceValidation.SummarySha256 -ceq
            [string]$retentionValidation.SummarySha256 -or
        [string]$surfaceValidation.RunReadySha256 -ceq
            [string]$retentionValidation.RunReadySha256 -or
        [string]$surfaceValidation.RunId -ceq
            [string]$retentionValidation.RunId -or
        [string]$surfaceValidation.RunLeafId -ceq
            [string]$retentionValidation.RunLeafId) {
        throw 'The two active Gate 1 evidence records reuse an evidence identity.'
    }
    return [pscustomobject][ordered]@{
        Present = $true
        ReleaseSurfaceAudit = $surfaceValidation
        Gate1RuntimeRetention = $retentionValidation
    }
}

Export-ModuleMember -Function Assert-PartisanGate1EvidencePair
