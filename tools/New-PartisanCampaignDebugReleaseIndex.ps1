[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RunEnvelopePath,
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'

$fullProfilePolicyId = 'partisan-campaign-debug-full-profile-v2'
$correctedCanaryPolicyId = 'partisan-campaign-debug-corrected-canary-v2'
$correctedCanaryFocusedAssertionIds = @(
    'combat_presence.aggregate',
    'combat_presence.empty_vehicle',
    'combat_presence.authoritative_samples',
    'combat_presence.rejected_rows',
    'combat_presence.heat_lifecycle',
    'combat_presence.schema62_migration',
    'combat_presence.schema63_restore',
    'combat_presence.malformed_fail_cold',
    'combat_presence.deterministic_diagnostics',
    'ownership_transition.aggregate',
    'ownership_transition.military',
    'ownership_transition.political',
    'ownership_transition.recapture',
    'ownership_transition.replay',
    'ownership_transition.restore',
    'ownership_transition.restore_queue_order',
    'ownership_transition.persistence_deadline',
    'ownership_transition.projection_revision',
    'ownership_transition.location_identity',
    'ownership_transition.linked_support',
    'ownership_transition.causes',
    'ownership_transition.security_fail_closed',
    'ownership_transition.migration_retention',
    'town_influence.aggregate',
    'town_influence.scaling',
    'town_influence.hysteresis',
    'town_influence.idempotency',
    'town_influence.projection',
    'town_influence.population',
    'town_influence.rejection',
    'town_influence.ownership_authority',
    'town_influence.external_completion',
    'town_influence.pre64_invader',
    'town_influence.migration',
    'town_influence.current_restore')
$externalRequiredAdvisoryContracts = [ordered]@{
    'isolation.world_scope' = [ordered]@{
        caseId = 'cleanup.state_isolation_restore'
        category = 'cleanup'
        feature = 'campaign_debug'
        stage = 'state_restore'
        expected = 'runtime certification remains scoped to the disposable development session'
        actual = 'world runtime, player inventory, health, and service caches require session restart before another certifying run'
        reason = 'restart the disposable development session before another certification run'
    }
    'persistence.real_restart' = [ordered]@{
        caseId = 'persistence.seeded_roundtrip.phase12'
        category = 'persistence'
        feature = 'persistence_smoke'
        stage = 'early_phase'
        expected = 'external process restart / reconnect remains an explicit later-gate scenario'
        actual = 'non-certifying external advisory | restart/fault gate'
        reason = 'run the immutable package through the external restart matrix before claiming restart certification'
    }
    'phase25.real_restart' = [ordered]@{
        caseId = 'phase25.manual_external_gaps'
        category = 'soak'
        feature = 'external_harness'
        stage = 'final'
        expected = 'real restart-after-primitive remains an explicit later-gate external scenario'
        actual = 'non-certifying external advisory | restart/fault gate'
        reason = 'run the immutable package through the external restart matrix before claiming restart certification'
    }
    'phase25.second_client' = [ordered]@{
        caseId = 'phase25.manual_external_gaps'
        category = 'soak'
        feature = 'external_harness'
        stage = 'final'
        expected = 'second-client join/reconnect remains an explicit later-gate external scenario'
        actual = 'non-certifying external advisory | multiplayer/JIP gate'
        reason = 'run the immutable package with the required clients before claiming multiplayer certification'
    }
    'phase25.two_hour_soak' = [ordered]@{
        caseId = 'phase25.manual_external_gaps'
        category = 'soak'
        feature = 'external_harness'
        stage = 'final'
        expected = 'two-hour endurance remains an explicit later-gate external scenario'
        actual = 'non-certifying external advisory | soak gate'
        reason = 'run the immutable package for the required duration before claiming soak certification'
    }
}
$externalRequiredAdvisoryIds = @($externalRequiredAdvisoryContracts.Keys)
$approvedNoncertifyingSkipIds = @(
    'phase24.escalation.support_physicalization',
    'phase24.escalation.group_physicalization')

function Get-ReleaseIndexStateDiffLabels {
    return @(
        'elapsed',
        'money',
        'HR',
        'training',
        'war level',
        'active missions',
        'objectives',
        'runtime vehicles',
        'mission assets',
        'active groups',
        'support requests',
        'enemy orders',
        'markers',
        'garage vehicles',
        'arsenal items',
        'civilian zones',
        'strategic events',
        'undercover records')
}

function Get-ReleaseIndexStateDiffValidation {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$RunId
    )

    # This is intentionally independent of the guarded runner implementation.
    # The publisher must rederive the complete retained text contract itself.
    $expectedLabels = @(Get-ReleaseIndexStateDiffLabels)
    $normalizedText = $Text.Replace("`r`n", "`n").Replace("`r", "`n")
    if ($normalizedText.EndsWith("`n", [StringComparison]::Ordinal)) {
        $normalizedText = $normalizedText.Substring(0, $normalizedText.Length - 1)
    }
    $lines = @($normalizedText -split "`n")
    $rows = if ($lines.Count -gt 2) {
        @($lines[2..($lines.Count - 1)])
    }
    else {
        @()
    }
    $headerExact = $lines.Count -ge 2 -and
        $lines[0] -ceq 'Partisan campaign debug state diff' -and
        $lines[1] -ceq "run $RunId"
    $labelsExact = $rows.Count -eq $expectedLabels.Count
    $grammarExact = $labelsExact
    $arithmeticExact = $labelsExact
    $zeroDeltaExact = $labelsExact
    $nonzero = 0
    for ($index = 0; $index -lt $rows.Count; $index++) {
        $row = [string]$rows[$index]
        $censusMatch = [regex]::Match(
            $row,
            ' \| delta (?<delta>-?\d+)$',
            [Text.RegularExpressions.RegexOptions]::CultureInvariant)
        if ($censusMatch.Success) {
            $censusDelta = [int64]0
            if ([int64]::TryParse(
                    $censusMatch.Groups['delta'].Value,
                    [Globalization.NumberStyles]::AllowLeadingSign,
                    [Globalization.CultureInfo]::InvariantCulture,
                    [ref]$censusDelta) -and $censusDelta -ne 0) {
                $nonzero++
            }
        }
        if ($index -ge $expectedLabels.Count) {
            $labelsExact = $false
            $grammarExact = $false
            $arithmeticExact = $false
            continue
        }
        $prefix = $expectedLabels[$index] + ' '
        if (-not $row.StartsWith($prefix, [StringComparison]::Ordinal)) {
            $labelsExact = $false
            $grammarExact = $false
            $arithmeticExact = $false
            continue
        }
        $valueMatch = [regex]::Match(
            $row.Substring($prefix.Length),
            '^(?<before>-?\d+) -> (?<after>-?\d+) \| delta (?<delta>-?\d+)$',
            [Text.RegularExpressions.RegexOptions]::CultureInvariant)
        if (-not $valueMatch.Success) {
            $grammarExact = $false
            $arithmeticExact = $false
            continue
        }
        $before = [int64]0
        $after = [int64]0
        $delta = [int64]0
        $integerGrammarExact =
            [int64]::TryParse(
                $valueMatch.Groups['before'].Value,
                [Globalization.NumberStyles]::AllowLeadingSign,
                [Globalization.CultureInfo]::InvariantCulture,
                [ref]$before) -and
            [int64]::TryParse(
                $valueMatch.Groups['after'].Value,
                [Globalization.NumberStyles]::AllowLeadingSign,
                [Globalization.CultureInfo]::InvariantCulture,
                [ref]$after) -and
            [int64]::TryParse(
                $valueMatch.Groups['delta'].Value,
                [Globalization.NumberStyles]::AllowLeadingSign,
                [Globalization.CultureInfo]::InvariantCulture,
                [ref]$delta)
        if (-not $integerGrammarExact) {
            $grammarExact = $false
            $arithmeticExact = $false
            continue
        }
        if (([decimal]$after - [decimal]$before) -ne [decimal]$delta) {
            $arithmeticExact = $false
        }
        if ($delta -ne 0) {
            $zeroDeltaExact = $false
        }
    }
    return [pscustomobject][ordered]@{
        HeaderExact = $headerExact
        LabelsExact = $labelsExact
        GrammarExact = $grammarExact
        ArithmeticExact = $arithmeticExact
        ZeroDeltaExact = $zeroDeltaExact
        ContractExact = $headerExact -and $rows.Count -eq $expectedLabels.Count -and
            $labelsExact -and $grammarExact -and $arithmeticExact -and
            $zeroDeltaExact
        RowCount = $rows.Count
        NonzeroRowCount = $nonzero
    }
}

function Get-RequiredProperty {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($null -eq $Object) {
        throw "$Label is missing."
    }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        throw "$Label.$Name is missing."
    }
    return $property.Value
}

function Require-Text {
    param($Value, [string]$Label)

    if ($null -eq $Value -or [string]::IsNullOrWhiteSpace([string]$Value)) {
        throw "$Label must be a non-empty string."
    }
    return [string]$Value
}

function Require-Sha256 {
    param($Value, [string]$Label)

    $text = Require-Text $Value $Label
    if ($text -cnotmatch '^[0-9a-f]{64}$') {
        throw "$Label must be a lowercase SHA-256 value."
    }
    return $text
}

function Get-CampaignDebugCanonicalPackageDigest {
    param([Parameter(Mandatory = $true)]$Package)

    $expectedPackageProperties = @(
        'root', 'hashAlgorithm', 'sha256', 'canonicalIndexPath', 'files')
    Assert-EqualSet `
        $expectedPackageProperties `
        @($Package.PSObject.Properties.Name) `
        'Retained manifest package properties'
    if ((Require-Text (Get-RequiredProperty $Package 'root' `
                'retained manifest.package') 'retained manifest.package.root') -cne
            'package/Partisan' -or
        (Require-Text (Get-RequiredProperty $Package 'hashAlgorithm' `
                'retained manifest.package') `
            'retained manifest.package.hashAlgorithm') -cne
            'sha256-manifest-v1' -or
        (Require-Text (Get-RequiredProperty $Package 'canonicalIndexPath' `
                'retained manifest.package') `
            'retained manifest.package.canonicalIndexPath') -cne
            'evidence/pack/files.sha256') {
        throw 'The retained manifest package metadata is not canonical.'
    }

    $expectedTuples = New-Object `
        'Collections.Generic.Dictionary[string,string]' `
        ([StringComparer]::Ordinal)
    $expectedTuples.Add(
        'package/Partisan/addon.gproj', 'Partisan/addon.gproj')
    $expectedTuples.Add(
        'package/Partisan/data.pak', 'Partisan/data.pak')
    $expectedTuples.Add(
        'package/Partisan/resourceDatabase.rdb',
        'Partisan/resourceDatabase.rdb')
    $expectedTuples.Add(
        'package/Partisan/thumbnail.png', 'Partisan/thumbnail.png')
    $rows = @((Get-RequiredProperty $Package 'files' 'retained manifest.package'))
    if ($rows.Count -ne $expectedTuples.Count) {
        throw 'The retained manifest must describe exactly four package files.'
    }
    $seenPaths = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $seenIndexPaths = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $canonicalRows = New-Object Collections.Generic.List[string]
    foreach ($row in @($rows | Sort-Object `
            @{ Expression = { [string]$_.indexPath } } -CaseSensitive)) {
        Assert-EqualSet `
            @('path', 'indexPath', 'length', 'sha256') `
            @($row.PSObject.Properties.Name) `
            'Retained manifest package-file properties'
        $path = Require-Text (Get-RequiredProperty $row 'path' `
                'retained manifest package file') `
            'retained manifest package file.path'
        $indexPath = Require-Text (Get-RequiredProperty $row 'indexPath' `
                'retained manifest package file') `
            'retained manifest package file.indexPath'
        $length = Require-Integer (Get-RequiredProperty $row 'length' `
                'retained manifest package file') `
            'retained manifest package file.length'
        $sha = Require-Sha256 (Get-RequiredProperty $row 'sha256' `
                'retained manifest package file') `
            'retained manifest package file.sha256'
        if ($length -le 0 -or
            -not $expectedTuples.ContainsKey($path) -or
            [string]$expectedTuples[$path] -cne $indexPath -or
            -not $seenPaths.Add($path) -or
            -not $seenIndexPaths.Add($indexPath)) {
            throw 'The retained manifest package-file inventory is not exact and unique.'
        }
        [void]$canonicalRows.Add(("{0}`t{1}`t{2}" -f
                $sha, $length, $indexPath))
    }
    if ($seenPaths.Count -ne $expectedTuples.Count -or
        $seenIndexPaths.Count -ne $expectedTuples.Count) {
        throw 'The retained manifest package-file inventory is incomplete.'
    }

    $canonicalText = ($canonicalRows.ToArray() -join "`n") + "`n"
    $encoding = [Text.UTF8Encoding]::new($false)
    $sha256 = [Security.Cryptography.SHA256]::Create()
    try {
        $digest = ([BitConverter]::ToString(
                $sha256.ComputeHash($encoding.GetBytes($canonicalText)))).
            Replace('-', '').ToLowerInvariant()
    }
    finally {
        $sha256.Dispose()
    }
    $declaredDigest = Require-Sha256 `
        (Get-RequiredProperty $Package 'sha256' 'retained manifest.package') `
        'retained manifest.package.sha256'
    if ($digest -cne $declaredDigest) {
        throw 'The retained manifest package digest does not match its canonical inventory.'
    }
    return $digest
}

function Require-Boolean {
    param($Value, [string]$Label)

    if ($Value -isnot [bool]) {
        throw "$Label must be Boolean."
    }
    return [bool]$Value
}

function Require-Integer {
    param($Value, [string]$Label)

    if ($Value -isnot [byte] -and $Value -isnot [sbyte] -and
        $Value -isnot [int16] -and $Value -isnot [uint16] -and
        $Value -isnot [int32] -and $Value -isnot [uint32] -and
        $Value -isnot [int64] -and $Value -isnot [uint64]) {
        throw "$Label must be an integer."
    }
    return [long]$Value
}

function Require-IntegerScalar {
    param($Value, [string]$Label)

    if ($Value -is [string]) {
        $parsed = [long]0
        if ([string]$Value -cnotmatch '^-?\d+$' -or
            -not [long]::TryParse([string]$Value, [ref]$parsed)) {
            throw "$Label must be an integer scalar."
        }
        return $parsed
    }
    return Require-Integer $Value $Label
}

function Require-NormalizedUtcTimestamp {
    param($Value, [string]$Label)

    $text = Require-Text $Value $Label
    if ($text -cnotmatch '^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d{1,7})?Z$') {
        throw "$Label must be a normalized UTC timestamp."
    }
    $parsed = [DateTimeOffset]::MinValue
    $styles = [Globalization.DateTimeStyles]::AssumeUniversal -bor
        [Globalization.DateTimeStyles]::AdjustToUniversal
    if (-not [DateTimeOffset]::TryParse(
            $text,
            [Globalization.CultureInfo]::InvariantCulture,
            $styles,
            [ref]$parsed) -or
        $parsed.Offset -ne [TimeSpan]::Zero) {
        throw "$Label must be a valid UTC timestamp."
    }
    return $parsed
}

function Assert-ExecutableIdentityEqual {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $expectedFileName = Require-Text `
        (Get-RequiredProperty $Expected 'fileName' "$Label expected executable") `
        "$Label expected fileName"
    $actualFileName = Require-Text `
        (Get-RequiredProperty $Actual 'fileName' "$Label actual executable") `
        "$Label actual fileName"
    $expectedFileVersion = Require-Text `
        (Get-RequiredProperty $Expected 'fileVersion' "$Label expected executable") `
        "$Label expected fileVersion"
    $actualFileVersion = Require-Text `
        (Get-RequiredProperty $Actual 'fileVersion' "$Label actual executable") `
        "$Label actual fileVersion"
    $expectedProductVersion = Require-Text `
        (Get-RequiredProperty $Expected 'productVersion' "$Label expected executable") `
        "$Label expected productVersion"
    $actualProductVersion = Require-Text `
        (Get-RequiredProperty $Actual 'productVersion' "$Label actual executable") `
        "$Label actual productVersion"
    $expectedLength = Require-Integer `
        (Get-RequiredProperty $Expected 'length' "$Label expected executable") `
        "$Label expected length"
    $actualLength = Require-Integer `
        (Get-RequiredProperty $Actual 'length' "$Label actual executable") `
        "$Label actual length"
    $expectedSha = Require-Sha256 `
        (Get-RequiredProperty $Expected 'sha256' "$Label expected executable") `
        "$Label expected sha256"
    $actualSha = Require-Sha256 `
        (Get-RequiredProperty $Actual 'sha256' "$Label actual executable") `
        "$Label actual sha256"
    if ($expectedLength -le 0 -or $actualLength -le 0 -or
        $expectedFileName -cne $actualFileName -or
        $expectedFileVersion -cne $actualFileVersion -or
        $expectedProductVersion -cne $actualProductVersion -or
        $expectedLength -ne $actualLength -or
        $expectedSha -cne $actualSha) {
        throw "$Label differs from the retained candidate manifest."
    }
}

function Get-RepositoryHead {
    param([Parameter(Mandatory = $true)][string]$CheckoutRoot)

    $headRows = @(& git -C $CheckoutRoot rev-parse HEAD 2>$null)
    $exitCode = $LASTEXITCODE
    $head = ($headRows -join '').Trim()
    if ($exitCode -ne 0 -or $head -cnotmatch '^[0-9a-f]{40}$') {
        throw 'The immutable Campaign Debug harness Git HEAD could not be resolved.'
    }
    return $head
}

function Get-RepositoryStatusRows {
    param([Parameter(Mandatory = $true)][string]$CheckoutRoot)

    $rows = @(& git -C $CheckoutRoot status --porcelain=v1 --untracked-files=all 2>$null)
    if ($LASTEXITCODE -ne 0) {
        throw 'The immutable Campaign Debug harness Git status could not be resolved.'
    }
    return @($rows | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) })
}

function Invoke-CampaignDebugReleaseIndexSelfTestLateDriftBarrier {
    param([Parameter(Mandatory = $true)][string]$RunRootPath)

    $token = [string]$env:PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_LATE_DRIFT_TOKEN
    if ([string]::IsNullOrEmpty($token)) {
        return
    }

    $tempPrefix = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd(
        '\', '/') + [IO.Path]::DirectorySeparatorChar
    $runRootFull = [IO.Path]::GetFullPath($RunRootPath)
    $relative = if ($runRootFull.StartsWith(
            $tempPrefix,
            [StringComparison]::OrdinalIgnoreCase)) {
        $runRootFull.Substring($tempPrefix.Length)
    }
    else {
        ''
    }
    $firstSegment = @($relative -split '[\\/]' | Where-Object { $_ } |
        Select-Object -First 1)
    if ($token -cnotmatch '^[0-9a-f]{32}$' -or
        $firstSegment.Count -ne 1 -or
        $firstSegment[0] -cnotmatch
            '^PartisanCorrectedCanaryV2-[0-9a-f]{32}$') {
        throw 'The bounded Campaign Debug late-drift self-test seam was denied.'
    }

    $ready = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexReady-' + $token))
    $mutated = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexMutated-' + $token))
    try {
        [void]$ready.Set()
        if (-not $mutated.WaitOne(15000)) {
            throw 'The bounded Campaign Debug late-drift self-test seam timed out.'
        }
    }
    finally {
        $ready.Dispose()
        $mutated.Dispose()
    }
}

function Invoke-CampaignDebugReleaseIndexSelfTestPublicationWindowBarrier {
    param([Parameter(Mandatory = $true)][string]$RunRootPath)

    $token = [string]$env:PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_PUBLICATION_TOKEN
    if ([string]::IsNullOrEmpty($token)) {
        return
    }
    $tempPrefix = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd(
        '\', '/') + [IO.Path]::DirectorySeparatorChar
    $runRootFull = [IO.Path]::GetFullPath($RunRootPath)
    $relative = if ($runRootFull.StartsWith(
            $tempPrefix,
            [StringComparison]::OrdinalIgnoreCase)) {
        $runRootFull.Substring($tempPrefix.Length)
    }
    else {
        ''
    }
    $firstSegment = @($relative -split '[\\/]' | Where-Object { $_ } |
        Select-Object -First 1)
    if ($token -cnotmatch '^[0-9a-f]{32}$' -or
        $firstSegment.Count -ne 1 -or
        $firstSegment[0] -cnotmatch
            '^PartisanCorrectedCanaryV2-[0-9a-f]{32}$') {
        throw 'The bounded Campaign Debug publication-window self-test seam was denied.'
    }

    $ready = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexPublicationReady-' + $token))
    $attempted = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexPublicationAttempted-' + $token))
    try {
        [void]$ready.Set()
        if (-not $attempted.WaitOne(15000)) {
            throw 'The bounded Campaign Debug publication-window self-test seam timed out.'
        }
    }
    finally {
        $ready.Dispose()
        $attempted.Dispose()
    }
}

function Invoke-CampaignDebugReleaseIndexSelfTestConcurrentPublicationBarrier {
    param([Parameter(Mandatory = $true)][string]$OutputPath)

    $token = [string]$env:PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_CONCURRENT_TOKEN
    if ([string]::IsNullOrEmpty($token)) {
        return
    }
    $tempPrefix = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd(
        '\', '/') + [IO.Path]::DirectorySeparatorChar
    $outputFullPath = [IO.Path]::GetFullPath($OutputPath)
    $relative = if ($outputFullPath.StartsWith(
            $tempPrefix,
            [StringComparison]::OrdinalIgnoreCase)) {
        $outputFullPath.Substring($tempPrefix.Length)
    }
    else {
        ''
    }
    $segments = @($relative -split '[\\/]' | Where-Object { $_ })
    if ($token -cnotmatch '^[0-9a-f]{32}$' -or
        $segments.Count -lt 2 -or
        $segments[0] -cnotmatch '^PartisanCorrectedCanaryV2-[0-9a-f]{32}$' -or
        $segments[-1] -cne 'release-index.json') {
        throw 'The bounded Campaign Debug concurrent-publication self-test seam was denied.'
    }

    $ready = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexConcurrentReady-' + $token))
    $published = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexConcurrentPublished-' + $token))
    try {
        [void]$ready.Set()
        if (-not $published.WaitOne(15000)) {
            throw 'The bounded Campaign Debug concurrent-publication self-test seam timed out.'
        }
    }
    finally {
        $ready.Dispose()
        $published.Dispose()
    }
}

function Open-CampaignDebugPublicationReadLocks {
    param([Parameter(Mandatory = $true)][string[]]$Paths)

    $streams = New-Object Collections.Generic.List[IO.FileStream]
    $seen = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    try {
        foreach ($path in $Paths) {
            $fullPath = [IO.Path]::GetFullPath($path)
            if (-not $seen.Add($fullPath)) {
                continue
            }
            $item = Get-Item -LiteralPath $fullPath -Force -ErrorAction Stop
            if ($item.PSIsContainer -or
                ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw 'A Campaign Debug publication input is not a regular non-reparse file.'
            }
            [void]$streams.Add([IO.FileStream]::new(
                    $fullPath,
                    [IO.FileMode]::Open,
                    [IO.FileAccess]::Read,
                    [IO.FileShare]::Read))
        }
        return $streams.ToArray()
    }
    catch {
        for ($index = $streams.Count - 1; $index -ge 0; $index--) {
            $streams[$index].Dispose()
        }
        throw
    }
}

function Close-CampaignDebugPublicationReadLocks {
    param([object[]]$Streams)

    for ($index = @($Streams).Count - 1; $index -ge 0; $index--) {
        if ($Streams[$index]) {
            $Streams[$index].Dispose()
        }
    }
}

function Get-StableFileSnapshot {
    param([Parameter(Mandatory = $true)][string]$Path)

    $item = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    if ($item.PSIsContainer -or
        ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'A retained Campaign Debug snapshot source is not a regular non-reparse file.'
    }
    $bytes = [IO.File]::ReadAllBytes($item.FullName)
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $hash = ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace(
            '-', '').ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
    $encoding = [Text.UTF8Encoding]::new($false, $true)
    try {
        $text = $encoding.GetString($bytes)
    }
    catch {
        throw 'A retained Campaign Debug snapshot source is not valid UTF-8.'
    }
    return [pscustomobject]@{
        Item = $item
        Bytes = $bytes
        Length = [long]$bytes.Length
        Sha256 = $hash
        Text = $text
    }
}

function Test-ExactByteArray {
    param(
        [Parameter(Mandatory = $true)][byte[]]$Expected,
        [Parameter(Mandatory = $true)][byte[]]$Actual
    )

    if ($Expected.Length -ne $Actual.Length) {
        return $false
    }
    for ($index = 0; $index -lt $Expected.Length; $index++) {
        if ($Expected[$index] -ne $Actual[$index]) {
            return $false
        }
    }
    return $true
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
            $literal = $Text.Substring($start, $Index.Value - $start)
            try {
                return [string]($literal | ConvertFrom-Json)
            }
            catch {
                throw "$Label contains an invalid JSON string escape."
            }
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
                    $Text.Substring($Index.Value + 1, 4) -cnotmatch '^[0-9a-fA-F]{4}$') {
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
        $keys = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
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
                throw "$Label contains a JSON object property without a colon."
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
    $numberPattern = [regex]::new(
        '-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?')
    $numberMatch = $numberPattern.Match($Text, [int]$Index.Value)
    if ($numberMatch.Success -and $numberMatch.Index -eq $Index.Value) {
        $Index.Value += $numberMatch.Length
        return
    }
    throw "$Label contains an invalid JSON value."
}

function Assert-NoDuplicateJsonObjectKeys {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $index = 0
    Read-StrictJsonValue $Text ([ref]$index) $Label
    Skip-StrictJsonWhitespace $Text ([ref]$index)
    if ($index -ne $Text.Length) {
        throw "$Label contains trailing JSON content."
    }
}

function Get-ImmutableGitBlobSha256 {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$Revision,
        [Parameter(Mandatory = $true)][string]$FilePath
    )

    if ($Revision -cnotmatch '^[0-9a-f]{40}$') {
        throw 'The immutable Campaign Debug harness revision is not canonical.'
    }
    $checkout = [IO.Path]::GetFullPath($CheckoutRoot).TrimEnd('\', '/')
    $prefix = $checkout + [IO.Path]::DirectorySeparatorChar
    $fullPath = [IO.Path]::GetFullPath($FilePath)
    if (-not $fullPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'An immutable Campaign Debug harness tool escaped the checkout.'
    }
    $relativePath = $fullPath.Substring($prefix.Length).Replace('\', '/')
    $startInfo = New-Object Diagnostics.ProcessStartInfo
    $startInfo.FileName = 'git'
    $escapedRoot = $checkout.Replace('"', '\"')
    $escapedSpec = ("{0}:{1}" -f $Revision, $relativePath).Replace('"', '\"')
    $startInfo.Arguments = "-C `"$escapedRoot`" cat-file blob `"$escapedSpec`""
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $startInfo
    try {
        if (-not $process.Start()) {
            throw 'The immutable Campaign Debug harness blob reader could not start.'
        }
        $memory = New-Object IO.MemoryStream
        try {
            $process.StandardOutput.BaseStream.CopyTo($memory)
            $errorText = $process.StandardError.ReadToEnd()
            $process.WaitForExit()
            if ($process.ExitCode -ne 0) {
                throw "The immutable Campaign Debug harness blob $relativePath could not be read: $errorText"
            }
            $sha = [Security.Cryptography.SHA256]::Create()
            try {
                return ([BitConverter]::ToString(
                    $sha.ComputeHash($memory.ToArray()))).Replace('-', '').ToLowerInvariant()
            }
            finally {
                $sha.Dispose()
            }
        }
        finally {
            $memory.Dispose()
        }
    }
    finally {
        $process.Dispose()
    }
}

function Invoke-BoundRunnerSemanticValidation {
    param(
        [Parameter(Mandatory = $true)][string]$RunnerPath,
        [Parameter(Mandatory = $true)][string]$JsonPath,
        [Parameter(Mandatory = $true)][string]$SummaryPath,
        [Parameter(Mandatory = $true)][string]$StateDiffPath,
        [Parameter(Mandatory = $true)][string]$GuardRoot,
        [Parameter(Mandatory = $true)][string]$ExpectedSha,
        [Parameter(Mandatory = $true)][string]$ExpectedUtc,
        [Parameter(Mandatory = $true)][string]$ExpectedLabel,
        [Parameter(Mandatory = $true)]
        [ValidateSet('full_certification', 'force_authority')]
        [string]$ExpectedProfile
    )

    $tokens = $null
    $parseErrors = $null
    $runnerAst = [Management.Automation.Language.Parser]::ParseFile(
        $RunnerPath,
        [ref]$tokens,
        [ref]$parseErrors)
    if (@($parseErrors).Count -ne 0) {
        throw 'The bound guarded runner cannot be parsed for semantic validation.'
    }
    $requiredFunctions = @(
        'Read-SharedFileText', 'Find-Case', 'Find-Assertion', 'Get-MetricValue',
        'Test-RunMetric', 'Test-ExactPassingAssertion', 'Get-ExactAssertionStatus',
        'Get-ExactAssertionActual', 'Get-FocusedForceAuthorityAssertionIds',
        'Get-CorrectedCanaryCaseManifest', 'Get-CorrectedCanaryAssertionManifest',
        'Get-CampaignDebugStateDiffLabels', 'Get-CampaignDebugStateDiffValidation',
        'Test-CampaignDebugArtifacts', 'Get-CampaignDebugHardDiagnosticCensus',
        'Get-AuxiliaryDiagnosticProjection', 'Get-GuardErrorCensus')
    $functionSource = New-Object Collections.Generic.List[string]
    foreach ($functionName in $requiredFunctions) {
        $matches = @($runnerAst.FindAll({
                    param($node)
                    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
                        $node.Name -ceq $functionName
                }, $true))
        if ($matches.Count -ne 1) {
            throw "The bound guarded runner does not expose exactly one $functionName function."
        }
        [void]$functionSource.Add($matches[0].Extent.Text)
    }
    $semanticScript = [scriptblock]::Create(@"
param(`$ArtifactParameters, `$DiagnosticParameters)
$($functionSource.ToArray() -join "`n`n")
`$artifactValidation = Test-CampaignDebugArtifacts @ArtifactParameters
`$diagnosticParameters.IntentionalMissionConvoyAdmissionDiagnosticsProven =
    [bool]`$artifactValidation.IntentionalMissionConvoyAdmissionDiagnosticsProven
`$diagnosticParameters.IntentionalMissionConvoySettlementDiagnosticProven =
    [bool]`$artifactValidation.IntentionalMissionConvoySettlementDiagnosticProven
`$diagnosticParameters.IntentionalMissionConvoyCorruptionDiagnosticsProven =
    [bool]`$artifactValidation.IntentionalMissionConvoyCorruptionDiagnosticsProven
`$diagnosticParameters.IntentionalMissionConvoyWatchdogDiagnosticProven =
    [bool]`$artifactValidation.IntentionalMissionConvoyWatchdogDiagnosticProven
`$errorCensus = Get-GuardErrorCensus @DiagnosticParameters
[pscustomobject]@{
    ArtifactValidation = `$artifactValidation
    ErrorCensus = `$errorCensus
}
"@)
    $artifactParameters = @{
        JsonPath = $JsonPath
        SummaryPath = $SummaryPath
        StateDiffPath = $StateDiffPath
        ExpectedSha = $ExpectedSha
        ExpectedUtc = $ExpectedUtc
        ExpectedLabel = $ExpectedLabel
        ExpectedProfile = $ExpectedProfile
    }
    if ($ExpectedProfile -ceq 'force_authority') {
        $artifactParameters.RequireCorrectedCanaryContract = $true
    }
    $diagnosticParameters = @{
        GuardRoot = $GuardRoot
        Profile = $ExpectedProfile
        IntentionalMissionConvoyAdmissionDiagnosticsProven = $false
        IntentionalMissionConvoySettlementDiagnosticProven = $false
        IntentionalMissionConvoyCorruptionDiagnosticsProven = $false
        IntentionalMissionConvoyWatchdogDiagnosticProven = $false
    }
    return & $semanticScript $artifactParameters $diagnosticParameters
}

function Require-NormalizedRelativePath {
    param($Value, [string]$Label)

    $path = Require-Text $Value $Label
    if ($path -match '^[\\/]' -or $path.Contains(':') -or
        $path.Contains('\\') -or $path.Contains('//') -or
        $path -match '(^|/)\.\.?(/|$)') {
        throw "$Label must be a normalized relative path."
    }
    return $path
}

function Assert-NoLocalAbsolutePathValue {
    param($Value, [string]$Label)

    if ($null -eq $Value) {
        return
    }
    if ($Value -is [string]) {
        $textValue = [string]$Value
        # Inspect path-bearing tokens before removing otherwise portable HTTP
        # URLs. A URL wrapper must not hide a drive, UNC, or file URI.
        if ($textValue -match '(?i)file:(?:/+|\\+)' -or
            $textValue -match '(?i)(?<![a-z0-9_])[a-z]:[\\/]' -or
            $textValue -match '\\\\') {
            throw "$Label contains a local or rooted absolute path."
        }
        $httpUrlPattern =
            '(?i)\bhttps?://(?:localhost|[a-z0-9](?:[a-z0-9.-]*[a-z0-9])?|\[[0-9a-f:.]+\])(?::[0-9]{1,5})?(?<tail>(?:[/?#][^\s"''<>]*)?)'
        foreach ($urlMatch in [regex]::Matches($textValue, $httpUrlPattern)) {
            $urlTail = [string]$urlMatch.Groups['tail'].Value
            if ($urlTail -match '(^|[?&#=])//(?=[^/\s])') {
                throw "$Label contains a local or rooted absolute path."
            }
        }
        $pathScanValue = [regex]::Replace(
            $textValue,
            $httpUrlPattern,
            '')
        if ($pathScanValue -match '(?i)(?<![a-z0-9_])[a-z]:[\\/]' -or
            $pathScanValue -match '\\\\' -or
            $pathScanValue -match '//' -or
            $pathScanValue -match '(?i)(?<![a-z0-9._-])[\\/](?=[^\s\\/])') {
            throw "$Label contains a local or rooted absolute path."
        }
        return
    }
    if ($Value -is [Collections.IDictionary]) {
        foreach ($entry in $Value.GetEnumerator()) {
            Assert-NoLocalAbsolutePathValue ([string]$entry.Key) "$Label property name"
            Assert-NoLocalAbsolutePathValue $entry.Value $Label
        }
        return
    }
    if ($Value -is [Collections.IEnumerable]) {
        foreach ($itemValue in $Value) {
            Assert-NoLocalAbsolutePathValue $itemValue $Label
        }
        return
    }
    if ($Value -is [PSCustomObject]) {
        foreach ($property in $Value.PSObject.Properties) {
            Assert-NoLocalAbsolutePathValue $property.Name "$Label property name"
            Assert-NoLocalAbsolutePathValue $property.Value $Label
        }
    }
}

function Resolve-ContainedFile {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd(
        [IO.Path]::DirectorySeparatorChar,
        [IO.Path]::AltDirectorySeparatorChar)
    $prefix = $rootPath + [IO.Path]::DirectorySeparatorChar
    $fullPath = [IO.Path]::GetFullPath((Join-Path $rootPath $RelativePath))
    if (-not $fullPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        throw "$Label is missing or escapes the evidence root."
    }
    $item = Get-Item -LiteralPath $fullPath -Force
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "$Label must not be a reparse point."
    }
    return $item
}

function Assert-EqualSet {
    param([string[]]$Expected, [string[]]$Actual, [string]$Label)

    $expectedSorted = @($Expected | Sort-Object -CaseSensitive)
    $actualSorted = @($Actual | Sort-Object -CaseSensitive)
    if ($expectedSorted.Count -ne $actualSorted.Count) {
        throw "$Label count differs."
    }
    for ($index = 0; $index -lt $expectedSorted.Count; $index++) {
        if ($expectedSorted[$index] -cne $actualSorted[$index]) {
            throw "$Label differs."
        }
    }
}

function Write-PortableJson {
    param([string]$Path, $Value)

    $text = (($Value | ConvertTo-Json -Depth 32).Replace("`r`n", "`n") + "`n")
    $encoding = New-Object Text.UTF8Encoding($false)
    $expectedBytes = $encoding.GetBytes($text)

    $assertExistingBytes = {
        param([string]$ExistingPath)

        $existingItem = Get-Item -LiteralPath $ExistingPath -Force -ErrorAction Stop
        if ($existingItem.PSIsContainer -or
            ($existingItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'The existing portable release index is not a regular non-reparse file.'
        }
        $existingBytes = [IO.File]::ReadAllBytes($existingItem.FullName)
        if ($existingBytes.Length -ne $expectedBytes.Length) {
            throw 'The existing immutable portable release index differs from the derived bytes.'
        }
        for ($byteIndex = 0; $byteIndex -lt $expectedBytes.Length; $byteIndex++) {
            if ($existingBytes[$byteIndex] -ne $expectedBytes[$byteIndex]) {
                throw 'The existing immutable portable release index differs from the derived bytes.'
            }
        }
    }

    if (Test-Path -LiteralPath $Path) {
        & $assertExistingBytes $Path
        return $false
    }

    $outputDirectory = Split-Path -Parent $Path
    $temporaryPath = Join-Path $outputDirectory `
        ('.' + [IO.Path]::GetFileName($Path) + '.' + [Guid]::NewGuid().ToString('N') + '.tmp')
    $createdByThisInvocation = $false
    try {
        $stream = New-Object IO.FileStream(
            $temporaryPath,
            [IO.FileMode]::CreateNew,
            [IO.FileAccess]::Write,
            [IO.FileShare]::None)
        try {
            $stream.Write($expectedBytes, 0, $expectedBytes.Length)
            $stream.Flush($true)
        }
        finally {
            $stream.Dispose()
        }

        Invoke-CampaignDebugReleaseIndexSelfTestConcurrentPublicationBarrier `
            -OutputPath $Path
        try {
            [IO.File]::Move($temporaryPath, $Path)
            $createdByThisInvocation = $true
        }
        catch [IO.IOException] {
            if (-not (Test-Path -LiteralPath $Path)) {
                throw
            }
            & $assertExistingBytes $Path
        }
    }
    finally {
        if (Test-Path -LiteralPath $temporaryPath -PathType Leaf) {
            Remove-Item -LiteralPath $temporaryPath -Force
        }
    }
    return $createdByThisInvocation
}

function Invoke-SnapshotSemanticValidation {
    param(
        [Parameter(Mandatory = $true)][hashtable]$SnapshotMap,
        [Parameter(Mandatory = $true)][string]$ArtifactPath,
        [Parameter(Mandatory = $true)][string]$SummaryPath,
        [Parameter(Mandatory = $true)][string]$StateDiffPath,
        [Parameter(Mandatory = $true)]$RunnerSnapshot,
        [Parameter(Mandatory = $true)][string]$ExpectedSha,
        [Parameter(Mandatory = $true)][string]$ExpectedUtc,
        [Parameter(Mandatory = $true)][string]$ExpectedLabel,
        [Parameter(Mandatory = $true)][string]$ExpectedProfile
    )

    $tempBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/')
    $snapshotRoot = Join-Path $tempBase (
        'PartisanCampaignDebugSemanticSnapshot_' + [Guid]::NewGuid().ToString('N'))
    $snapshotRoot = [IO.Path]::GetFullPath($snapshotRoot)
    $tempPrefix = $tempBase + [IO.Path]::DirectorySeparatorChar
    if (-not $snapshotRoot.StartsWith(
            $tempPrefix,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw 'The Campaign Debug semantic snapshot escaped the temporary root.'
    }
    try {
        [void][IO.Directory]::CreateDirectory($snapshotRoot)
        $privateRunnerPath = Join-Path $snapshotRoot '.semantic-runner.ps1'
        $runnerStream = [IO.FileStream]::new(
            $privateRunnerPath,
            [IO.FileMode]::CreateNew,
            [IO.FileAccess]::Write,
            [IO.FileShare]::None)
        try {
            $runnerBytes = [byte[]]$RunnerSnapshot.Bytes
            $runnerStream.Write($runnerBytes, 0, $runnerBytes.Length)
            $runnerStream.Flush($true)
        }
        finally {
            $runnerStream.Dispose()
        }
        foreach ($path in $SnapshotMap.Keys) {
            $target = [IO.Path]::GetFullPath((Join-Path $snapshotRoot $path))
            $snapshotPrefix = $snapshotRoot.TrimEnd('\', '/') +
                [IO.Path]::DirectorySeparatorChar
            if (-not $target.StartsWith(
                    $snapshotPrefix,
                    [StringComparison]::OrdinalIgnoreCase)) {
                throw 'A Campaign Debug semantic snapshot path escaped its private root.'
            }
            [void][IO.Directory]::CreateDirectory((Split-Path -Parent $target))
            $stream = [IO.FileStream]::new(
                $target,
                [IO.FileMode]::CreateNew,
                [IO.FileAccess]::Write,
                [IO.FileShare]::None)
            try {
                $bytes = [byte[]]$SnapshotMap[$path].Bytes
                $stream.Write($bytes, 0, $bytes.Length)
                $stream.Flush($true)
            }
            finally {
                $stream.Dispose()
            }
        }
        return Invoke-BoundRunnerSemanticValidation `
            -RunnerPath $privateRunnerPath `
            -JsonPath (Join-Path $snapshotRoot $ArtifactPath) `
            -SummaryPath (Join-Path $snapshotRoot $SummaryPath) `
            -StateDiffPath (Join-Path $snapshotRoot $StateDiffPath) `
            -GuardRoot (Join-Path $snapshotRoot 'raw') `
            -ExpectedSha $ExpectedSha `
            -ExpectedUtc $ExpectedUtc `
            -ExpectedLabel $ExpectedLabel `
            -ExpectedProfile $ExpectedProfile
    }
    finally {
        if (Test-Path -LiteralPath $snapshotRoot -PathType Container) {
            Remove-Item -LiteralPath $snapshotRoot -Recurse -Force
        }
    }
}

$runItem = Get-Item -LiteralPath $RunEnvelopePath -Force -ErrorAction Stop
if ($runItem.Name -cne 'run.json' -or
    ($runItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'The release-index source must be a non-reparse run.json file.'
}
$runRoot = $runItem.Directory.FullName
$runRootItem = Get-Item -LiteralPath $runRoot -Force
if (($runRootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'The retained Campaign Debug run directory must not be a reparse point.'
}
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $runRoot 'release-index.json'
}
$outputFullPath = [IO.Path]::GetFullPath($OutputPath)
$expectedOutputPath = [IO.Path]::GetFullPath((Join-Path $runRoot 'release-index.json'))
if (-not $outputFullPath.Equals($expectedOutputPath, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The portable release index must be written beside run.json as release-index.json.'
}

$runSnapshot = Get-StableFileSnapshot -Path $runItem.FullName
$runText = $runSnapshot.Text
Assert-NoDuplicateJsonObjectKeys -Text $runText -Label 'The retained run envelope'
try {
    $run = $runText | ConvertFrom-Json
}
catch {
    throw "The retained run envelope is not valid JSON: $($_.Exception.Message)"
}
if ((Require-Integer (Get-RequiredProperty $run 'schemaVersion' 'run') 'run.schemaVersion') -ne 2 -or
    (Require-Text (Get-RequiredProperty $run 'evidenceKind' 'run') 'run.evidenceKind') -cne
        'packaged-campaign-debug') {
    throw 'The retained run envelope does not use the portable release-index contract.'
}
Assert-NoLocalAbsolutePathValue $run 'The retained run envelope'

$candidate = Get-RequiredProperty $run 'candidate' 'run'
$harness = Get-RequiredProperty $run 'harness' 'run'
$launch = Get-RequiredProperty $run 'launch' 'run'
$outcome = Get-RequiredProperty $run 'outcome' 'run'
$settings = Get-RequiredProperty $run 'settings' 'run'
$cleanup = Get-RequiredProperty $run 'cleanup' 'run'
$fileRows = @(Get-RequiredProperty $run 'files' 'run')
$profile = Require-Text (Get-RequiredProperty $launch 'profile' 'run.launch') `
    'run.launch.profile'
$proofScope = Require-Text (Get-RequiredProperty $launch 'proofScope' 'run.launch') `
    'run.launch.proofScope'
$isCorrectedCanary = $profile -ceq 'force_authority'
if ($profile -ceq 'full_certification') {
    if ($proofScope -cne 'full_certification') {
        throw 'The retained full-profile launch proof scope is inconsistent.'
    }
    $policyId = $fullProfilePolicyId
    $indexEvidenceKind = 'packaged-campaign-debug-full-profile'
}
elseif ($isCorrectedCanary) {
    if ($proofScope -cne 'focused_force_authority') {
        throw 'The retained corrected-canary launch proof scope is inconsistent.'
    }
    $policyId = $correctedCanaryPolicyId
    $indexEvidenceKind = 'packaged-campaign-debug-corrected-canary'
}
else {
    throw "The retained Campaign Debug profile $profile is unsupported by the portable release index."
}
if ($fileRows.Count -ne 10) {
    throw 'The Campaign Debug run envelope must retain the canonical ten-file raw set.'
}

$producerPath = $PSCommandPath
$consumerPath = Join-Path $PSScriptRoot 'update-release-docs.ps1'
$runnerPath = Join-Path $PSScriptRoot 'run-guarded-campaign-debug.ps1'
$candidateModulePath = Join-Path $PSScriptRoot 'Partisan.ReleaseCandidate.psm1'
$checkoutRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$producerSha = (Get-FileHash -LiteralPath $producerPath -Algorithm SHA256).Hash.ToLowerInvariant()
$consumerSha = (Get-FileHash -LiteralPath $consumerPath -Algorithm SHA256).Hash.ToLowerInvariant()
$runnerSha = (Get-FileHash -LiteralPath $runnerPath -Algorithm SHA256).Hash.ToLowerInvariant()
$candidateModuleSha = (Get-FileHash `
    -LiteralPath $candidateModulePath -Algorithm SHA256).Hash.ToLowerInvariant()
$harnessHead = Require-Text (Get-RequiredProperty $harness 'gitHead' 'run.harness') `
    'run.harness.gitHead'
$harnessDirty = Require-Boolean (Get-RequiredProperty $harness 'dirty' 'run.harness') `
    'run.harness.dirty'
if ($harnessHead -cnotmatch '^[0-9a-f]{40}$' -or $harnessDirty) {
    throw 'The retained run must bind a clean immutable harness revision.'
}
$harnessHashes = [ordered]@{}
foreach ($hashField in @(
        'campaignRunnerSha256', 'campaignRunnerGitBlobSha256',
        'candidateModuleSha256', 'candidateModuleGitBlobSha256',
        'releaseIndexProducerSha256', 'releaseIndexProducerGitBlobSha256',
        'releaseDocsConsumerSha256', 'releaseDocsConsumerGitBlobSha256')) {
    $harnessHashes[$hashField] = Require-Sha256 `
        (Get-RequiredProperty $harness $hashField 'run.harness') `
        "run.harness.$hashField"
}
if ($harnessHashes.campaignRunnerSha256 -cne $runnerSha -or
    $harnessHashes.candidateModuleSha256 -cne $candidateModuleSha -or
    $harnessHashes.releaseIndexProducerSha256 -cne $producerSha -or
    $harnessHashes.releaseDocsConsumerSha256 -cne $consumerSha) {
    throw 'The retained run envelope does not bind the current guarded-runner tool set.'
}
if ($isCorrectedCanary) {
    $currentHarnessHead = Get-RepositoryHead -CheckoutRoot $checkoutRoot
    if ($harnessHead -cne $currentHarnessHead) {
        throw 'The retained run envelope harness Git HEAD differs from the current immutable revision.'
    }
    if (@(Get-RepositoryStatusRows -CheckoutRoot $checkoutRoot).Count -ne 0) {
        throw 'The corrected-canary release index requires a clean harness checkout.'
    }
    foreach ($binding in @(
            @('campaignRunnerGitBlobSha256', $runnerPath, $runnerSha),
            @('candidateModuleGitBlobSha256', $candidateModulePath, $candidateModuleSha),
            @('releaseIndexProducerGitBlobSha256', $producerPath, $producerSha),
            @('releaseDocsConsumerGitBlobSha256', $consumerPath, $consumerSha))) {
        $immutableBlobSha = Get-ImmutableGitBlobSha256 `
            -CheckoutRoot $checkoutRoot `
            -Revision $harnessHead `
            -FilePath $binding[1]
        if ($harnessHashes[$binding[0]] -cne $immutableBlobSha -or
            [string]$binding[2] -cne $immutableBlobSha) {
            throw "The retained run envelope $($binding[0]) differs from immutable Git content."
        }
    }
}

$rowPaths = New-Object Collections.Generic.List[string]
$rowMap = @{}
foreach ($row in $fileRows) {
    $path = Require-NormalizedRelativePath (Get-RequiredProperty $row 'path' 'run.files row') `
        'run.files.path'
    if ($rowMap.ContainsKey($path)) {
        throw "The run file inventory repeats $path."
    }
    $length = Require-Integer (Get-RequiredProperty $row 'length' "run.files[$path]") `
        "run.files[$path].length"
    $sha = Require-Sha256 (Get-RequiredProperty $row 'sha256' "run.files[$path]") `
        "run.files[$path].sha256"
    if ($length -lt 0) {
        throw "run.files[$path].length must be nonnegative."
    }
    $item = Resolve-ContainedFile $runRoot $path "run.files[$path]"
    $snapshot = Get-StableFileSnapshot -Path $item.FullName
    if ($snapshot.Length -ne $length -or $snapshot.Sha256 -cne $sha) {
        throw "The retained raw file $path differs from its run-envelope inventory row."
    }
    $rowMap[$path] = [pscustomobject]@{
        Item = $item
        Length = $length
        Sha256 = $sha
        Snapshot = $snapshot
    }
    [void]$rowPaths.Add($path)
}

$actualEntries = @(Get-ChildItem -LiteralPath $runRoot -Recurse -Force)
if (@($actualEntries | Where-Object {
            ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0
        }).Count -ne 0) {
    throw 'The retained Campaign Debug run bundle must not contain reparse points.'
}
$actualPaths = @($actualEntries | Where-Object { -not $_.PSIsContainer } | Where-Object {
    $_.FullName -cne $runItem.FullName -and $_.FullName -cne $outputFullPath
} | ForEach-Object {
    $_.FullName.Substring($runRoot.TrimEnd('\', '/').Length + 1).Replace('\', '/')
})
Assert-EqualSet $rowPaths.ToArray() $actualPaths 'Run-envelope inventory and retained raw file set'

$settingsRows = @($rowPaths | Where-Object { $_ -ceq 'config/HST_Settings.json' })
$manifestRows = @($rowPaths | Where-Object { $_ -ceq 'identity/candidate.json' })
$readyRows = @($rowPaths | Where-Object { $_ -ceq 'identity/candidate.ready.json' })
$artifactRows = @($rowPaths | Where-Object { $_ -cmatch '^raw/campaign-debug/HST_CampaignDebug_[a-zA-Z0-9_]+\.json$' })
$diffRows = @($rowPaths | Where-Object { $_ -cmatch '^raw/campaign-debug/HST_CampaignDebug_[a-zA-Z0-9_]+_state_diff\.txt$' })
$textSummaryRows = @($rowPaths | Where-Object { $_ -cmatch '^raw/campaign-debug/HST_CampaignDebug_[a-zA-Z0-9_]+_summary\.txt$' })
$consoleRows = @($rowPaths | Where-Object { $_ -cmatch '^raw/logs/[^/]+/console\.log$' })
$scriptRows = @($rowPaths | Where-Object { $_ -cmatch '^raw/logs/[^/]+/script\.log$' })
$errorRows = @($rowPaths | Where-Object { $_ -cmatch '^raw/logs/[^/]+/error\.log$' })
$crashRows = @($rowPaths | Where-Object { $_ -cmatch '^raw/logs/[^/]+/crash\.log$' })
foreach ($canonicalRows in @(
        $settingsRows, $manifestRows, $readyRows, $artifactRows, $diffRows,
        $textSummaryRows, $consoleRows, $scriptRows, $errorRows, $crashRows)) {
    if (@($canonicalRows).Count -ne 1) {
        throw 'The retained Campaign Debug raw set is missing or duplicates a canonical file role.'
    }
}

$candidateId = Require-Text (Get-RequiredProperty $candidate 'candidateId' 'run.candidate') `
    'run.candidate.candidateId'
if ($candidateId -cnotmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$') {
    throw 'run.candidate.candidateId is not a portable candidate identifier.'
}
$manifestItem = $rowMap[$manifestRows[0]].Item
$readyItem = $rowMap[$readyRows[0]].Item
try {
    Assert-NoDuplicateJsonObjectKeys `
        -Text $rowMap[$manifestRows[0]].Snapshot.Text `
        -Label 'The retained candidate manifest'
    Assert-NoDuplicateJsonObjectKeys `
        -Text $rowMap[$readyRows[0]].Snapshot.Text `
        -Label 'The retained candidate ready marker'
    $retainedManifest = $rowMap[$manifestRows[0]].Snapshot.Text | ConvertFrom-Json
    $retainedReady = $rowMap[$readyRows[0]].Snapshot.Text | ConvertFrom-Json
}
catch {
    throw "The retained candidate manifest or ready seal is invalid JSON: $($_.Exception.Message)"
}
Assert-NoLocalAbsolutePathValue $retainedManifest 'The retained candidate manifest'
Assert-NoLocalAbsolutePathValue $retainedReady 'The retained candidate ready seal'
$manifestCandidate = Get-RequiredProperty $retainedManifest 'candidate' 'retained manifest'
$manifestSource = Get-RequiredProperty $retainedManifest 'source' 'retained manifest'
$manifestEmbedded = Get-RequiredProperty $manifestSource 'embeddedImplementation' `
    'retained manifest.source'
$manifestAddon = Get-RequiredProperty $retainedManifest 'addon' 'retained manifest'
$manifestToolchain = Get-RequiredProperty $retainedManifest 'toolchain' 'retained manifest'
$manifestWorkbench = Get-RequiredProperty $retainedManifest 'workbench' 'retained manifest'
$manifestPackage = Get-RequiredProperty $retainedManifest 'package' 'retained manifest'
$derivedPackageSha = Get-CampaignDebugCanonicalPackageDigest $manifestPackage
$candidateVersion = Require-Text `
    (Get-RequiredProperty $candidate 'candidateVersion' 'run.candidate') `
    'run.candidate.candidateVersion'
$embeddedBuildSha = Require-Text `
    (Get-RequiredProperty $candidate 'embeddedBuildSha' 'run.candidate') `
    'run.candidate.embeddedBuildSha'
$embeddedBuildUtc = Require-Text `
    (Get-RequiredProperty $candidate 'embeddedBuildUtc' 'run.candidate') `
    'run.candidate.embeddedBuildUtc'
$embeddedBuildLabel = Require-Text `
    (Get-RequiredProperty $candidate 'embeddedBuildLabel' 'run.candidate') `
    'run.candidate.embeddedBuildLabel'
$campaignSchema = Require-Integer `
    (Get-RequiredProperty $candidate 'campaignSchema' 'run.candidate') `
    'run.candidate.campaignSchema'
$runtimeSettingsSchema = Require-Integer `
    (Get-RequiredProperty $candidate 'runtimeSettingsSchema' 'run.candidate') `
    'run.candidate.runtimeSettingsSchema'
$addonId = Require-Text (Get-RequiredProperty $candidate 'addonId' 'run.candidate') `
    'run.candidate.addonId'
$addonGuid = Require-Text (Get-RequiredProperty $candidate 'addonGuid' 'run.candidate') `
    'run.candidate.addonGuid'
$packageHashAlgorithm = Require-Text `
    (Get-RequiredProperty $candidate 'packageHashAlgorithm' 'run.candidate') `
    'run.candidate.packageHashAlgorithm'
$candidateSourceHead = Require-Text (Get-RequiredProperty $candidate 'gitHead' 'run.candidate') `
    'run.candidate.gitHead'
if ($candidateSourceHead -cnotmatch '^[0-9a-f]{40}$') {
    throw 'run.candidate.gitHead must be a lowercase full Git SHA.'
}
$packageSha = Require-Sha256 (Get-RequiredProperty $candidate 'packageSha256' 'run.candidate') `
    'run.candidate.packageSha256'
$manifestSha = Require-Sha256 (Get-RequiredProperty $candidate 'manifestSha256' 'run.candidate') `
    'run.candidate.manifestSha256'
$readySha = Require-Sha256 (Get-RequiredProperty $candidate 'readySha256' 'run.candidate') `
    'run.candidate.readySha256'
$settingsSha = Require-Sha256 (Get-RequiredProperty $settings 'sha256' 'run.settings') `
    'run.settings.sha256'
$workbenchCrc = Require-Text `
    (Get-RequiredProperty $candidate 'workbenchCrc' 'run.candidate') `
    'run.candidate.workbenchCrc'
$runtimeRole = Require-Text `
    (Get-RequiredProperty $candidate 'runtimeRole' 'run.candidate') `
    'run.candidate.runtimeRole'
$manifestClient = Get-RequiredProperty $manifestToolchain 'client' `
    'retained manifest.toolchain'
$manifestClientDiagnostic = Get-RequiredProperty $manifestToolchain 'clientDiagnostic' `
    'retained manifest.toolchain'
if ((Require-Integer (Get-RequiredProperty $retainedManifest 'manifestSchemaVersion' `
            'retained manifest') 'retained manifest.manifestSchemaVersion') -ne 1 -or
    (Require-Text (Get-RequiredProperty $manifestCandidate 'id' 'retained manifest.candidate') `
        'retained manifest.candidate.id') -cne $candidateId -or
    (Require-Text (Get-RequiredProperty $manifestCandidate 'version' 'retained manifest.candidate') `
        'retained manifest.candidate.version') -cne $candidateVersion -or
    (Require-Text (Get-RequiredProperty $manifestAddon 'version' 'retained manifest.addon') `
        'retained manifest.addon.version') -cne $candidateVersion -or
    (Require-Text (Get-RequiredProperty $manifestSource 'gitHead' 'retained manifest.source') `
        'retained manifest.source.gitHead') -cne $candidateSourceHead -or
    (Require-Text (Get-RequiredProperty $manifestEmbedded 'sha' `
            'retained manifest.source.embeddedImplementation') `
        'retained manifest.source.embeddedImplementation.sha') -cne $embeddedBuildSha -or
    (Require-Text (Get-RequiredProperty $manifestEmbedded 'utc' `
            'retained manifest.source.embeddedImplementation') `
        'retained manifest.source.embeddedImplementation.utc') -cne $embeddedBuildUtc -or
    (Require-Text (Get-RequiredProperty $manifestEmbedded 'label' `
            'retained manifest.source.embeddedImplementation') `
        'retained manifest.source.embeddedImplementation.label') -cne $embeddedBuildLabel -or
    (Require-Integer (Get-RequiredProperty $manifestSource 'campaignSchema' `
            'retained manifest.source') 'retained manifest.source.campaignSchema') -ne
        $campaignSchema -or
    (Require-Integer (Get-RequiredProperty $manifestSource 'runtimeSettingsSchema' `
            'retained manifest.source') 'retained manifest.source.runtimeSettingsSchema') -ne
        $runtimeSettingsSchema -or
    (Require-Text (Get-RequiredProperty $manifestAddon 'id' 'retained manifest.addon') `
        'retained manifest.addon.id') -cne $addonId -or
    (Require-Text (Get-RequiredProperty $manifestAddon 'guid' 'retained manifest.addon') `
        'retained manifest.addon.guid') -cne $addonGuid -or
    (Require-Text (Get-RequiredProperty $manifestPackage 'hashAlgorithm' `
            'retained manifest.package') 'retained manifest.package.hashAlgorithm') -cne
        $packageHashAlgorithm -or
    $packageHashAlgorithm -cne 'sha256-manifest-v1' -or
    (Require-Sha256 (Get-RequiredProperty $manifestPackage 'sha256' `
            'retained manifest.package') 'retained manifest.package.sha256') -cne $packageSha -or
    $derivedPackageSha -cne $packageSha -or
    (Require-Text (Get-RequiredProperty $manifestWorkbench 'crc' `
            'retained manifest.workbench') 'retained manifest.workbench.crc') -cne $workbenchCrc -or
    $campaignSchema -le 0 -or $runtimeSettingsSchema -le 0 -or
    $addonGuid -cnotmatch '^[0-9A-F]{16}$' -or
    $workbenchCrc -cnotmatch '^[0-9a-f]{8}$' -or
    $runtimeRole -cne 'client') {
    throw 'The run candidate differs from its retained manifest identity.'
}
if ((Require-Integer (Get-RequiredProperty $retainedReady 'schemaVersion' `
            'retained ready seal') 'retained ready seal.schemaVersion') -ne 1 -or
    (Require-Text (Get-RequiredProperty $retainedReady 'candidateId' 'retained ready seal') `
        'retained ready seal.candidateId') -cne $candidateId -or
    (Require-Text (Get-RequiredProperty $retainedReady 'gitHead' 'retained ready seal') `
        'retained ready seal.gitHead') -cne $candidateSourceHead -or
    (Require-Sha256 (Get-RequiredProperty $retainedReady 'packageSha256' `
            'retained ready seal') 'retained ready seal.packageSha256') -cne $packageSha -or
    (Require-Sha256 (Get-RequiredProperty $retainedReady 'manifestSha256' `
            'retained ready seal') 'retained ready seal.manifestSha256') -cne $manifestSha) {
    throw 'The retained candidate ready seal differs from the manifest and run identity.'
}
Assert-ExecutableIdentityEqual $manifestClientDiagnostic `
    (Get-RequiredProperty $candidate 'diagnosticExecutable' 'run.candidate') `
    'run candidate diagnostic executable'
Assert-ExecutableIdentityEqual $manifestClientDiagnostic `
    (Get-RequiredProperty $candidate 'recordedDiagnosticExecutable' 'run.candidate') `
    'run candidate recorded diagnostic executable'
Assert-ExecutableIdentityEqual $manifestClient `
    (Get-RequiredProperty $candidate 'recordedRuntimeExecutable' 'run.candidate') `
    'run candidate recorded runtime executable'
Assert-ExecutableIdentityEqual $manifestClientDiagnostic `
    (Get-RequiredProperty $launch 'diagnosticExecutable' 'run.launch') `
    'launch diagnostic executable'
Assert-ExecutableIdentityEqual $manifestClient `
    (Get-RequiredProperty $launch 'recordedRuntimeExecutable' 'run.launch') `
    'launch recorded runtime executable'
if ($rowMap[$manifestRows[0]].Sha256 -cne $manifestSha -or
    $rowMap[$readyRows[0]].Sha256 -cne $readySha -or
    $rowMap[$settingsRows[0]].Sha256 -cne $settingsSha -or
    (Require-Sha256 (Get-RequiredProperty $launch 'packageSha256' 'run.launch') `
        'run.launch.packageSha256') -cne $packageSha -or
    (Require-Text (Get-RequiredProperty $launch 'addonGuid' 'run.launch') `
        'run.launch.addonGuid') -cne
        $addonGuid -or
    (Require-Integer (Get-RequiredProperty $settings 'schemaVersion' 'run.settings') `
        'run.settings.schemaVersion') -ne $runtimeSettingsSchema) {
    throw 'The retained identity/config rows differ from the run candidate or launch binding.'
}
if ((Require-Text (Get-RequiredProperty $candidate 'runtimeUseDisposition' 'run.candidate') `
        'run.candidate.runtimeUseDisposition') -cne 'active-runtime-candidate' -or
    -not (Require-Boolean (Get-RequiredProperty $launch 'stagedPackage' 'run.launch') `
        'run.launch.stagedPackage') -or
    -not (Require-Boolean (Get-RequiredProperty $settings 'guardedRuntimeCopy' 'run.settings') `
        'run.settings.guardedRuntimeCopy')) {
    throw 'The retained run is not an active staged Campaign Debug capture.'
}
if ($isCorrectedCanary -and
    ((Require-Text (Get-RequiredProperty $launch 'worldResource' 'run.launch') `
            'run.launch.worldResource') -cne 'Worlds/HST_Dev/HST_Dev.ent' -or
        (Require-Integer (Get-RequiredProperty $launch 'addonSearchRootCount' `
                'run.launch') 'run.launch.addonSearchRootCount') -ne 2)) {
    throw 'The corrected-canary launch does not bind the canonical packed HST_Dev runtime.'
}

$rawArtifactItem = $rowMap[$artifactRows[0]].Item
try {
    Assert-NoDuplicateJsonObjectKeys `
        -Text $rowMap[$artifactRows[0]].Snapshot.Text `
        -Label 'The retained raw Campaign Debug artifact'
    $raw = $rowMap[$artifactRows[0]].Snapshot.Text | ConvertFrom-Json
}
catch {
    throw "The retained Campaign Debug JSON is invalid: $($_.Exception.Message)"
}
$runId = Require-Text (Get-RequiredProperty $raw 'm_sRunId' 'raw full profile') `
    'raw full profile.m_sRunId'
if ($runId -cnotmatch '^seed\d+_t\d+_p\d+_u\d+$' -or
    $artifactRows[0] -cne "raw/campaign-debug/HST_CampaignDebug_$runId.json" -or
    $diffRows[0] -cne "raw/campaign-debug/HST_CampaignDebug_${runId}_state_diff.txt" -or
    $textSummaryRows[0] -cne "raw/campaign-debug/HST_CampaignDebug_${runId}_summary.txt" -or
    (Require-Text (Get-RequiredProperty $raw 'm_sProfile' 'raw Campaign Debug profile') `
        'raw Campaign Debug profile.m_sProfile') -cne $profile -or
    (Require-Text (Get-RequiredProperty $raw 'm_sBuildSha' 'raw full profile') `
        'raw full profile.m_sBuildSha') -cne
        (Require-Text (Get-RequiredProperty $candidate 'embeddedBuildSha' 'run.candidate') `
            'run.candidate.embeddedBuildSha') -or
    (Require-Text (Get-RequiredProperty $raw 'm_sBuildUtc' 'raw full profile') `
        'raw full profile.m_sBuildUtc') -cne
        (Require-Text (Get-RequiredProperty $candidate 'embeddedBuildUtc' 'run.candidate') `
            'run.candidate.embeddedBuildUtc') -or
    (Require-Text (Get-RequiredProperty $raw 'm_sBuildLabel' 'raw full profile') `
        'raw full profile.m_sBuildLabel') -cne
        (Require-Text (Get-RequiredProperty $candidate 'embeddedBuildLabel' 'run.candidate') `
            'run.candidate.embeddedBuildLabel')) {
    throw 'The retained raw Campaign Debug artifact identity is inconsistent.'
}

$cases = @(Get-RequiredProperty $raw 'm_aCases' 'raw full profile')
$caseStatusCounts = [ordered]@{ PASS = 0; WARN = 0; FAIL = 0; BLOCKED = 0; SKIPPED = 0 }
$certificationStatusCounts = [ordered]@{ PASS = 0; WARN = 0; FAIL = 0; BLOCKED = 0 }
$blockedRecords = New-Object Collections.Generic.List[object]
$warningRecords = New-Object Collections.Generic.List[object]
$failedAssertionIds = New-Object Collections.Generic.List[string]
$warningAssertionIds = New-Object Collections.Generic.List[string]
$skippedAssertionIds = New-Object Collections.Generic.List[string]
$rawAssertionCount = 0
$seenCaseIds = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($case in $cases) {
    $caseId = Require-Text (Get-RequiredProperty $case 'm_sCaseId' 'raw case') 'raw case.m_sCaseId'
    if (-not $seenCaseIds.Add($caseId)) {
        throw "The raw Campaign Debug profile duplicates case ID $caseId."
    }
    $caseStatus = Require-Text (Get-RequiredProperty $case 'm_sStatus' "raw case $caseId") `
        "raw case $caseId.m_sStatus"
    if ($caseStatus -cnotin @('PASS', 'WARN', 'FAIL', 'BLOCKED', 'SKIPPED')) {
        throw "Raw case $caseId has unsupported status $caseStatus."
    }
    $caseStatusCounts[$caseStatus]++
    $derivedCaseStatus = 'PASS'
    $derivedCaseSeverity = 0
    $caseHasBlockedAssertion = $false
    $caseHasSkippedAssertion = $false
    $seenAssertionIds = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($assertion in @(Get-RequiredProperty $case 'm_aAssertions' "raw case $caseId")) {
        $rawAssertionCount++
        $assertionId = Require-Text (Get-RequiredProperty $assertion 'm_sAssertionId' "raw case $caseId assertion") `
            "raw case $caseId assertion ID"
        if (-not $seenAssertionIds.Add($assertionId)) {
            throw "Raw case $caseId duplicates assertion ID $assertionId."
        }
        $assertionStatus = Require-Text (Get-RequiredProperty $assertion 'm_sStatus' "raw assertion $assertionId") `
            "raw assertion $assertionId status"
        if ($assertionStatus -cnotin @('PASS', 'WARN', 'FAIL', 'BLOCKED', 'SKIPPED')) {
            throw "Raw assertion $assertionId has unsupported status $assertionStatus."
        }
        $assertionSeverity = switch ($assertionStatus) {
            'SKIPPED' { 1 }
            'WARN' { 2 }
            'BLOCKED' { 3 }
            'FAIL' { 4 }
            default { 0 }
        }
        if ($assertionSeverity -gt $derivedCaseSeverity) {
            $derivedCaseSeverity = $assertionSeverity
            $derivedCaseStatus = $assertionStatus
        }
        $countsTowardCertification = Require-Boolean `
            (Get-RequiredProperty $assertion 'm_bCountsTowardCertification' "raw assertion $assertionId") `
            "raw assertion $assertionId certification flag"
        if ($countsTowardCertification) {
            if (-not $certificationStatusCounts.Contains($assertionStatus)) {
                throw "Certification-counting assertion $assertionId has unsupported status $assertionStatus."
            }
            $certificationStatusCounts[$assertionStatus]++
        }
        if ($assertionStatus -ceq 'FAIL') {
            [void]$failedAssertionIds.Add($assertionId)
        }
        elseif ($assertionStatus -ceq 'BLOCKED') {
            $caseHasBlockedAssertion = $true
            [void]$blockedRecords.Add([pscustomobject][ordered]@{
                id = $assertionId
                caseId = $caseId
                category = [string](Get-RequiredProperty $case 'm_sCategory' "raw case $caseId")
                feature = [string](Get-RequiredProperty $case 'm_sFeature' "raw case $caseId")
                stage = [string](Get-RequiredProperty $case 'm_sStage' "raw case $caseId")
                expected = [string](Get-RequiredProperty $assertion 'm_sExpected' "raw assertion $assertionId")
                actual = [string](Get-RequiredProperty $assertion 'm_sActual' "raw assertion $assertionId")
                reason = [string](Get-RequiredProperty $assertion 'm_sFailureReason' "raw assertion $assertionId")
                proofLevel = [string](Get-RequiredProperty $assertion 'm_sProofLevel' "raw assertion $assertionId")
                observedPath = [string](Get-RequiredProperty $assertion 'm_sObservedPath' "raw assertion $assertionId")
                requiredPath = [string](Get-RequiredProperty $assertion 'm_sRequiredPath' "raw assertion $assertionId")
                countsTowardCertification = $countsTowardCertification
            })
        }
        elseif ($assertionStatus -ceq 'WARN') {
            [void]$warningAssertionIds.Add($assertionId)
            [void]$warningRecords.Add([pscustomobject][ordered]@{
                id = $assertionId
                caseId = $caseId
                category = [string](Get-RequiredProperty $case 'm_sCategory' "raw case $caseId")
                feature = [string](Get-RequiredProperty $case 'm_sFeature' "raw case $caseId")
                stage = [string](Get-RequiredProperty $case 'm_sStage' "raw case $caseId")
                expected = [string](Get-RequiredProperty $assertion 'm_sExpected' "raw assertion $assertionId")
                actual = [string](Get-RequiredProperty $assertion 'm_sActual' "raw assertion $assertionId")
                reason = [string](Get-RequiredProperty $assertion 'm_sFailureReason' "raw assertion $assertionId")
                proofLevel = [string](Get-RequiredProperty $assertion 'm_sProofLevel' "raw assertion $assertionId")
                observedPath = [string](Get-RequiredProperty $assertion 'm_sObservedPath' "raw assertion $assertionId")
                requiredPath = [string](Get-RequiredProperty $assertion 'm_sRequiredPath' "raw assertion $assertionId")
                countsTowardCertification = $countsTowardCertification
            })
        }
        elseif ($assertionStatus -ceq 'SKIPPED') {
            $caseHasSkippedAssertion = $true
            [void]$skippedAssertionIds.Add($assertionId)
        }
    }
    if ($caseStatus -cne $derivedCaseStatus) {
        throw "Raw case $caseId status $caseStatus differs from its assertion-derived $derivedCaseStatus disposition."
    }
    if ($caseStatus -ceq 'BLOCKED' -and -not $caseHasBlockedAssertion) {
        throw "Blocked case $caseId has no linked BLOCKED assertion."
    }
    if ($caseStatus -ceq 'SKIPPED' -and -not $caseHasSkippedAssertion) {
        throw "Skipped case $caseId has no linked SKIPPED assertion."
    }
}

$rawCaseCount = $cases.Count
$rawCaseCounts = [ordered]@{
    caseCount = $rawCaseCount
    pass = $caseStatusCounts.PASS
    warn = $caseStatusCounts.WARN
    fail = $caseStatusCounts.FAIL
    blocked = $caseStatusCounts.BLOCKED
    skipped = $caseStatusCounts.SKIPPED
}
$rawCertificationCounts = [ordered]@{
    required = $certificationStatusCounts.PASS + $certificationStatusCounts.WARN +
        $certificationStatusCounts.FAIL + $certificationStatusCounts.BLOCKED
    proven = $certificationStatusCounts.PASS
    fail = $certificationStatusCounts.FAIL
    blocked = $certificationStatusCounts.BLOCKED
    warn = $certificationStatusCounts.WARN
}
foreach ($binding in @(
        @('m_iPassCount', 'pass'), @('m_iWarnCount', 'warn'),
        @('m_iFailCount', 'fail'), @('m_iBlockedCount', 'blocked'),
        @('m_iSkippedCount', 'skipped'))) {
    if ((Require-Integer (Get-RequiredProperty $raw $binding[0] 'raw full profile') `
            "raw full profile.$($binding[0])") -ne $rawCaseCounts[$binding[1]]) {
        throw "The raw Campaign Debug $($binding[1]) case count is inconsistent."
    }
}
foreach ($binding in @(
        @('m_iCertificationRequiredCount', 'required'),
        @('m_iCertificationProvenCount', 'proven'),
        @('m_iCertificationFailCount', 'fail'),
        @('m_iCertificationBlockedCount', 'blocked'),
        @('m_iCertificationWarnCount', 'warn'))) {
    if ((Require-Integer (Get-RequiredProperty $raw $binding[0] 'raw full profile') `
            "raw full profile.$($binding[0])") -ne $rawCertificationCounts[$binding[1]]) {
        throw "The raw Campaign Debug $($binding[1]) certification count is inconsistent."
    }
}
$rawCertificationPassed = Require-Boolean `
    (Get-RequiredProperty $raw 'm_bCertificationPassed' 'raw full profile') `
    'raw full profile.m_bCertificationPassed'
$derivedCertificationPassed = if ($isCorrectedCanary) {
    $false
}
else {
    $rawCertificationCounts.required -eq $rawCertificationCounts.proven -and
        $rawCertificationCounts.fail -eq 0 -and
        $rawCertificationCounts.blocked -eq 0 -and
        $rawCertificationCounts.warn -eq 0
}
if ($rawCertificationPassed -ne $derivedCertificationPassed) {
    throw 'The raw Campaign Debug certification Boolean differs from its profile contract.'
}

$semanticSnapshotMap = @{}
foreach ($snapshotPath in $rowPaths) {
    $semanticSnapshotMap[$snapshotPath] = $rowMap[$snapshotPath].Snapshot
}
$runnerToolSnapshot = Get-StableFileSnapshot -Path $runnerPath
if ($runnerToolSnapshot.Sha256 -cne $runnerSha) {
    throw 'The guarded runner changed before snapshot-bound semantic validation.'
}
$semanticValidation = Invoke-SnapshotSemanticValidation `
    -SnapshotMap $semanticSnapshotMap `
    -RunnerSnapshot $runnerToolSnapshot `
    -ArtifactPath $artifactRows[0] `
    -SummaryPath $textSummaryRows[0] `
    -StateDiffPath $diffRows[0] `
    -ExpectedSha $embeddedBuildSha `
    -ExpectedUtc $embeddedBuildUtc `
    -ExpectedLabel $embeddedBuildLabel `
    -ExpectedProfile $profile
$initialSemanticJson = $semanticValidation | ConvertTo-Json -Depth 64 -Compress
$derivedValidation = $semanticValidation.ArtifactValidation
$derivedErrorCensus = $semanticValidation.ErrorCensus
$validation = Get-RequiredProperty $outcome 'validation' 'run.outcome'
$recordedValidationValid = Require-Boolean `
    (Get-RequiredProperty $validation 'Valid' 'run.outcome.validation') `
    'run.outcome.validation.Valid'
$artifactValidationValid = Require-Boolean $derivedValidation.Valid `
    'derived runner artifact validation.Valid'
if ($recordedValidationValid -ne $artifactValidationValid) {
    throw 'The recorded artifact-validation disposition differs from portable semantic re-derivation.'
}
Assert-EqualSet `
    @(Get-RequiredProperty $validation 'Problems' 'run.outcome.validation') `
    @($derivedValidation.Problems) `
    'Recorded and re-derived artifact-validation problems'
if ((Require-Text (Get-RequiredProperty $validation 'RunId' 'run.outcome.validation') `
        'run.outcome.validation.RunId') -cne $runId -or
    (Require-Text (Get-RequiredProperty $validation 'Profile' 'run.outcome.validation') `
        'run.outcome.validation.Profile') -cne $profile -or
    (Require-Text (Get-RequiredProperty $validation 'ProofScope' 'run.outcome.validation') `
        'run.outcome.validation.ProofScope') -cne $proofScope -or
    (Require-Boolean (Get-RequiredProperty $validation 'FullCertification' `
            'run.outcome.validation') 'run.outcome.validation.FullCertification') -ne
        (-not $isCorrectedCanary)) {
    throw 'The guarded-runner validation does not identify the retained Campaign Debug profile.'
}
if ($isCorrectedCanary -and
    -not (Require-Boolean (Get-RequiredProperty $validation `
                'CorrectedCanaryContract' 'run.outcome.validation') `
            'run.outcome.validation.CorrectedCanaryContract')) {
    throw 'The guarded-runner validation did not bind the corrected-canary contract.'
}
foreach ($binding in @(
        @('CaseCount', 'caseCount'), @('Pass', 'pass'), @('Warn', 'warn'),
        @('Fail', 'fail'), @('Blocked', 'blocked'), @('Skipped', 'skipped'))) {
    if ((Require-Integer (Get-RequiredProperty $validation $binding[0] 'run.outcome.validation') `
            "run.outcome.validation.$($binding[0])") -ne $rawCaseCounts[$binding[1]]) {
        throw "The guarded-runner validation $($binding[0]) differs from raw JSON."
    }
}
foreach ($binding in @(
        @('CertificationRequired', 'required'), @('CertificationProven', 'proven'),
        @('CertificationFail', 'fail'), @('CertificationBlocked', 'blocked'),
        @('CertificationWarn', 'warn'))) {
    if ((Require-Integer (Get-RequiredProperty $validation $binding[0] 'run.outcome.validation') `
            "run.outcome.validation.$($binding[0])") -ne $rawCertificationCounts[$binding[1]]) {
        throw "The guarded-runner validation $($binding[0]) differs from raw JSON."
    }
}
if ((Require-Boolean (Get-RequiredProperty $validation 'CertificationPassed' 'run.outcome.validation') `
        'run.outcome.validation.CertificationPassed') -ne $rawCertificationPassed) {
    throw 'The guarded-runner certification Boolean differs from raw JSON.'
}

$diffText = $rowMap[$diffRows[0]].Snapshot.Text
$releaseIndexStateDiffValidation = Get-ReleaseIndexStateDiffValidation `
    -Text $diffText `
    -RunId $runId
$stateDiffRowCount = Require-Integer (Get-RequiredProperty $validation 'StateDiffRows' 'run.outcome.validation') `
    'run.outcome.validation.StateDiffRows'
$nonzeroStateDiffRowCount = Require-Integer `
    (Get-RequiredProperty $validation 'NonzeroStateDiffRows' 'run.outcome.validation') `
    'run.outcome.validation.NonzeroStateDiffRows'
$recordedStateDiffManifestExact = Require-Boolean `
    (Get-RequiredProperty $validation 'StateDiffManifestExact' 'run.outcome.validation') `
    'run.outcome.validation.StateDiffManifestExact'
$derivedStateDiffManifestExact = Require-Boolean `
    (Get-RequiredProperty $derivedValidation 'StateDiffManifestExact' `
        'derived runner artifact validation') `
    'derived runner artifact validation.StateDiffManifestExact'
if ($releaseIndexStateDiffValidation.RowCount -ne $stateDiffRowCount -or
    $releaseIndexStateDiffValidation.NonzeroRowCount -ne
        $nonzeroStateDiffRowCount -or
    $releaseIndexStateDiffValidation.ContractExact -ne
        $recordedStateDiffManifestExact -or
    $releaseIndexStateDiffValidation.ContractExact -ne
        $derivedStateDiffManifestExact) {
    throw 'The retained state diff differs from the independent publisher grammar, arithmetic, or guarded-runner validation.'
}
if (-not (Require-Boolean (Get-RequiredProperty $validation `
            'BuildProvenanceMatches' 'run.outcome.validation') `
        'run.outcome.validation.BuildProvenanceMatches') -or
    [string]$derivedValidation.BuildSha -cne $embeddedBuildSha -or
    [string]$derivedValidation.BuildUtc -cne $embeddedBuildUtc -or
    [string]$derivedValidation.BuildLabel -cne $embeddedBuildLabel) {
    throw 'The recorded artifact-validation build provenance differs from retained raw evidence.'
}
if ((Require-Text (Get-RequiredProperty $validation 'Trigger' `
            'run.outcome.validation') 'run.outcome.validation.Trigger') -cne
        'cli_autostart' -or
    [string]$derivedValidation.Trigger -cne 'cli_autostart') {
    throw 'The recorded artifact-validation trigger differs from retained raw evidence.'
}
foreach ($binding in @(
        @('StartedAtSecond', 'm_iStartedAtSecond'),
        @('EndedAtSecond', 'm_iEndedAtSecond'))) {
    if ((Require-Integer (Get-RequiredProperty $validation $binding[0] `
                'run.outcome.validation') "run.outcome.validation.$($binding[0])") -ne
        (Require-Integer (Get-RequiredProperty $raw $binding[1] 'raw full profile') `
            "raw full profile.$($binding[1])")) {
        throw "The recorded artifact-validation $($binding[0]) differs from retained raw evidence."
    }
}
if ((Require-Integer (Get-RequiredProperty $validation 'ArtifactCount' `
            'run.outcome.validation') 'run.outcome.validation.ArtifactCount') -ne 3) {
    throw 'The recorded artifact-validation artifact count is not canonical.'
}
$recordedFinalOrphanCleanupPass = Require-Boolean `
    (Get-RequiredProperty $validation 'FinalOrphanCleanupPass' `
        'run.outcome.validation') `
    'run.outcome.validation.FinalOrphanCleanupPass'
$derivedFinalOrphanCleanupPass = Require-Boolean `
    (Get-RequiredProperty $derivedValidation 'FinalOrphanCleanupPass' `
        'derived runner artifact validation') `
    'derived runner artifact validation.FinalOrphanCleanupPass'
$recordedFinalOrphanActiveGroups = Require-IntegerScalar `
    (Get-RequiredProperty $validation 'FinalOrphanActiveGroups' `
        'run.outcome.validation') `
    'run.outcome.validation.FinalOrphanActiveGroups'
$derivedFinalOrphanActiveGroups = Require-IntegerScalar `
    (Get-RequiredProperty $derivedValidation 'FinalOrphanActiveGroups' `
        'derived runner artifact validation') `
    'derived runner artifact validation.FinalOrphanActiveGroups'
if ($recordedFinalOrphanCleanupPass -ne $derivedFinalOrphanCleanupPass -or
    $recordedFinalOrphanActiveGroups -ne $derivedFinalOrphanActiveGroups) {
    throw 'The recorded final-orphan proof differs from retained raw evidence.'
}
$recordedValidation = $validation
$validation = $derivedValidation

$focusedCaseId = $null
$focusedCaseStatus = $null
$focusedAssertionCount = 0
$focusedAssertionsPassed = 0
$focusedAssertionSetExact = $false
$focusedAssertionsCertificationExact = $false
$correctedCanaryCaseSetExact = $false
$correctedCanaryAssertionManifestExact = $false
$correctedCanaryWarningContractExact = $false
$correctedCanaryBlockedContractExact = $false
$correctedCanaryStateDiffManifestExact = $false
$correctedCanaryOrphanContractExact = $false
$correctedCanaryAssertionSkipFree = $false
$correctedCanaryProofAxisPassed = $false
if ($isCorrectedCanary) {
    $focusedCases = @($cases | Where-Object {
        [string]$_.m_sCaseId -ceq 'early_mechanics.force_authority'
    })
    if ($focusedCases.Count -eq 1) {
        $focusedCaseId = [string]$focusedCases[0].m_sCaseId
        $focusedCaseStatus = [string]$focusedCases[0].m_sStatus
        $focusedAssertions = @($focusedCases[0].m_aAssertions)
        $focusedAssertionCount = $focusedAssertions.Count
        $focusedAssertionsPassed = @($focusedAssertions | Where-Object {
            [string]$_.m_sStatus -ceq 'PASS'
        }).Count
        $focusedAssertionIds = @($focusedAssertions | ForEach-Object {
            [string]$_.m_sAssertionId
        })
        try {
            Assert-EqualSet $correctedCanaryFocusedAssertionIds $focusedAssertionIds `
                'Corrected-canary focused assertion set'
            $focusedAssertionSetExact = $true
        }
        catch {
            $focusedAssertionSetExact = $false
        }
        $focusedNoncertifyingAssertions = @($focusedAssertions | Where-Object {
            -not (Require-Boolean `
                (Get-RequiredProperty $_ 'm_bCountsTowardCertification' `
                    'corrected-canary focused assertion') `
                'corrected-canary focused assertion certification flag')
        })
        $focusedAssertionsCertificationExact =
            $focusedNoncertifyingAssertions.Count -eq 1 -and
            [string]$focusedNoncertifyingAssertions[0].m_sAssertionId -ceq
                'town_influence.external_completion'
    }

    $correctedCanaryAssertionSkipFree = $skippedAssertionIds.Count -eq 0

    $expectedCaseIds = @(
        'preflight.state_isolation',
        'post_case_cleanup.preflight_state_isolation',
        'cleanup.prefixed_state.start_preflight.hst_debug_',
        'early_mechanics.force_authority',
        'post_case_cleanup.early_mechanics_force_authority',
        'cleanup.enemy_orders.run_completion',
        "cleanup.prefixed_state.run_completion.hst_debug_$runId",
        'cleanup.prefixed_state.run_completion_persistence_smoke_cleanup.hst_smoke',
        'cleanup.player_marker_completion',
        'cleanup.run_leak_snapshot',
        'cleanup.state_isolation_restore')
    try {
        Assert-EqualSet $expectedCaseIds @($cases | ForEach-Object {
            [string]$_.m_sCaseId
        }) 'Corrected-canary case set'
        $correctedCanaryCaseSetExact = Require-Boolean `
            (Get-RequiredProperty $validation 'CorrectedCanaryCaseSetExact' `
                'derived runner artifact validation') `
            'derived runner artifact validation.CorrectedCanaryCaseSetExact'
    }
    catch {
        $correctedCanaryCaseSetExact = $false
    }

    foreach ($field in @(
            'CorrectedCanaryCaseSetExact',
            'CorrectedCanaryAssertionManifestExact',
            'CorrectedCanaryWarningContractExact',
            'CorrectedCanaryBlockedContractExact',
            'CorrectedCanaryOrphanContractExact',
            'StateDiffManifestExact')) {
        $recordedExact = Require-Boolean `
            (Get-RequiredProperty $recordedValidation $field `
                'run.outcome.validation') `
            "run.outcome.validation.$field"
        $derivedExact = Require-Boolean `
            (Get-RequiredProperty $validation $field `
                'derived runner artifact validation') `
            "derived runner artifact validation.$field"
        if ($recordedExact -ne $derivedExact) {
            throw "The recorded corrected-canary $field differs from retained raw evidence."
        }
    }
    $correctedCanaryAssertionManifestExact =
        [bool]$validation.CorrectedCanaryAssertionManifestExact
    $correctedCanaryStateDiffManifestExact =
        [bool]$validation.StateDiffManifestExact
    $correctedCanaryOrphanContractExact =
        [bool]$validation.CorrectedCanaryOrphanContractExact

    $playerMarkerWarnings = @($warningRecords | Where-Object {
        $_.id -ceq 'cleanup.player_marker.live' -and
        $_.caseId -ceq 'cleanup.player_marker_completion' -and
        -not $_.countsTowardCertification
    })
    $correctedCanaryWarningContractExact =
        [bool]$validation.CorrectedCanaryWarningContractExact -and
        $warningRecords.Count -eq 1 -and
        $playerMarkerWarnings.Count -eq 1

    $worldScopeContract = $externalRequiredAdvisoryContracts[
        'isolation.world_scope']
    $worldScopeBlocks = @($blockedRecords | Where-Object {
        $_.id -ceq 'isolation.world_scope' -and
        $_.caseId -ceq $worldScopeContract.caseId -and
        $_.category -ceq $worldScopeContract.category -and
        $_.feature -ceq $worldScopeContract.feature -and
        $_.stage -ceq $worldScopeContract.stage -and
        $_.expected -ceq $worldScopeContract.expected -and
        $_.actual -ceq $worldScopeContract.actual -and
        $_.reason -ceq $worldScopeContract.reason -and
        $_.proofLevel -ceq 'EXTERNAL_PROCESS' -and
        $_.observedPath -ceq 'manual_external_gap' -and
        $_.requiredPath -ceq
            'external process restart, reconnect, or long-soak harness' -and
        -not $_.countsTowardCertification
    })
    $worldScopeBlockedCases = @($cases | Where-Object {
        [string]$_.m_sCaseId -ceq 'cleanup.state_isolation_restore' -and
        [string]$_.m_sStatus -ceq 'BLOCKED'
    })
    $correctedCanaryBlockedContractExact =
        [bool]$validation.CorrectedCanaryBlockedContractExact -and
        $blockedRecords.Count -eq 1 -and
        $worldScopeBlocks.Count -eq 1 -and
        $worldScopeBlockedCases.Count -eq 1

    $recordedFocusedRows = @(
        @(Get-RequiredProperty $recordedValidation 'FocusedAssertions' `
            'run.outcome.validation') | ForEach-Object {
            '{0}|{1}|{2}|{3}' -f
                (Require-Text (Get-RequiredProperty $_ 'Id' `
                        'run.outcome.validation.FocusedAssertions row') `
                    'run.outcome.validation.FocusedAssertions.Id'),
                (Require-Boolean (Get-RequiredProperty $_ 'Pass' `
                        'run.outcome.validation.FocusedAssertions row') `
                    'run.outcome.validation.FocusedAssertions.Pass'),
                [string](Get-RequiredProperty $_ 'Status' `
                    'run.outcome.validation.FocusedAssertions row'),
                [string](Get-RequiredProperty $_ 'Actual' `
                    'run.outcome.validation.FocusedAssertions row')
        })
    $derivedFocusedRows = @($validation.FocusedAssertions | ForEach-Object {
        '{0}|{1}|{2}|{3}' -f $_.Id, [bool]$_.Pass, [string]$_.Status, [string]$_.Actual
    })
    Assert-EqualSet $derivedFocusedRows $recordedFocusedRows `
        'Recorded and re-derived corrected-canary focused assertions'
    if ((Require-Text (Get-RequiredProperty $recordedValidation 'FocusedCaseId' `
                'run.outcome.validation') 'run.outcome.validation.FocusedCaseId') -cne
            $focusedCaseId -or
        (Require-Text (Get-RequiredProperty $recordedValidation 'FocusedCaseStatus' `
                'run.outcome.validation') 'run.outcome.validation.FocusedCaseStatus') -cne
            $focusedCaseStatus) {
        throw 'The recorded corrected-canary focused case differs from retained raw evidence.'
    }

    $correctedCanaryProofAxisPassed =
        $artifactValidationValid -and $correctedCanaryCaseSetExact -and
        $rawCaseCounts.caseCount -eq 11 -and $rawCaseCounts.pass -eq 9 -and
        $rawCaseCounts.warn -eq 1 -and $rawCaseCounts.fail -eq 0 -and
        $rawCaseCounts.blocked -eq 1 -and $rawCaseCounts.skipped -eq 0 -and
        $rawAssertionCount -eq 91 -and
        $correctedCanaryAssertionManifestExact -and
        $correctedCanaryWarningContractExact -and
        $correctedCanaryBlockedContractExact -and
        $correctedCanaryStateDiffManifestExact -and
        $correctedCanaryOrphanContractExact -and
        $focusedAssertionSetExact -and $focusedCaseStatus -ceq 'PASS' -and
        $focusedAssertionCount -eq 35 -and
        $focusedAssertionsPassed -eq 35 -and
        $focusedAssertionsCertificationExact -and
        $correctedCanaryAssertionSkipFree -and
        $rawCertificationCounts.required -eq 87 -and
        $rawCertificationCounts.proven -eq 87 -and
        $rawCertificationCounts.fail -eq 0 -and
        $rawCertificationCounts.blocked -eq 0 -and
        $rawCertificationCounts.warn -eq 0 -and
        -not $rawCertificationPassed -and
        $nonzeroStateDiffRowCount -eq 0 -and $stateDiffRowCount -eq 18
}

$externalAdvisoryRecords = @($warningRecords | Where-Object {
    $externalRequiredAdvisoryIds -ccontains $_.id
})
if ($isCorrectedCanary) {
    $externalAdvisoryRecords = @($blockedRecords | Where-Object {
        $externalRequiredAdvisoryIds -ccontains $_.id
    })
}
$externalAdvisoryIds = @($externalAdvisoryRecords | ForEach-Object { $_.id })
$duplicateExternalIds = @($externalAdvisoryIds | Group-Object -CaseSensitive |
    Where-Object Count -ne 1)
if ($duplicateExternalIds.Count -ne 0) {
    throw 'The raw full profile duplicates an external-required advisory assertion.'
}
$externalAdvisoryLinkageValid = $true
foreach ($record in $externalAdvisoryRecords) {
    $contract = $externalRequiredAdvisoryContracts[$record.id]
    if ($record.caseId -cne $contract.caseId -or
        $record.category -cne $contract.category -or
        $record.feature -cne $contract.feature -or
        $record.stage -cne $contract.stage -or
        $record.expected -cne $contract.expected -or
        $record.actual -cne $contract.actual -or
        $record.reason -cne $contract.reason -or
        $record.proofLevel -cne 'EXTERNAL_PROCESS' -or
        $record.observedPath -cne 'manual_external_gap' -or
        $record.requiredPath -cne 'external process restart, reconnect, or long-soak harness' -or
        $record.countsTowardCertification) {
        $externalAdvisoryLinkageValid = $false
    }
}
$unsupportedWarningIds = @($warningAssertionIds | Where-Object {
    $externalRequiredAdvisoryIds -cnotcontains $_
})
if ($isCorrectedCanary) {
    $unsupportedWarningIds = @($warningAssertionIds | Where-Object {
        $_ -cne 'cleanup.player_marker.live'
    })
}

$unsupportedSkippedIds = @($skippedAssertionIds | Where-Object {
    $_ -cnotin $approvedNoncertifyingSkipIds
})
if (@($skippedAssertionIds | Group-Object -CaseSensitive | Where-Object Count -ne 1).Count -ne 0) {
    $unsupportedSkippedIds += '<duplicate-skip-id>'
}

$recordedErrorCensus = Get-RequiredProperty $outcome 'errorCensus' 'run.outcome'
foreach ($field in @(
        'HardDiagnosticCount', 'ScriptErrors', 'EngineErrors', 'PartisanErrors',
        'CrashMarkers', 'PartisanSeverityLineCount', 'ApprovedStockDiagnosticCount',
        'ApprovedIntentionalDiagnosticCount', 'MalformedHardDiagnosticCount',
        'UnapprovedHardDiagnosticCount', 'CanonicalScriptLogCount',
        'CanonicalConsoleLogCount', 'CanonicalErrorLogCount',
        'CanonicalCrashLogCount', 'AuxiliaryUnapprovedEventCount')) {
    if ((Require-Integer (Get-RequiredProperty $recordedErrorCensus $field `
                'run.outcome.errorCensus') "run.outcome.errorCensus.$field") -ne
        (Require-Integer (Get-RequiredProperty $derivedErrorCensus $field `
                'derived error census') "derived error census.$field")) {
        throw "The recorded diagnostic census $field differs from retained log re-derivation."
    }
}
foreach ($field in @(
        'Valid', 'HardDiagnosticFree', 'ChannelArithmeticValid',
        'CategoryArithmeticValid', 'LifecycleMarkersValid',
        'IdentityBaselinePairValid', 'IntentionalFixtureStructureExact',
        'IntentionalFixtureSetValid', 'CanonicalLogPairSameDirectory',
        'AuxiliaryLogPairSameDirectory', 'AuxiliaryDiagnosticsValid',
        'ErrorLogProjectionExact', 'CrashLogProjectionExact',
        'IntentionalMissionConvoyAdmissionDiagnosticsProven',
        'IntentionalMissionConvoySettlementDiagnosticProven',
        'IntentionalMissionConvoyCorruptionDiagnosticsProven',
        'IntentionalMissionConvoyWatchdogDiagnosticProven')) {
    if ((Require-Boolean (Get-RequiredProperty $recordedErrorCensus $field `
                'run.outcome.errorCensus') "run.outcome.errorCensus.$field") -ne
        (Require-Boolean (Get-RequiredProperty $derivedErrorCensus $field `
                'derived error census') "derived error census.$field")) {
        throw "The recorded diagnostic census $field differs from retained log re-derivation."
    }
}
$recordedUnapprovedKindRows = @(
    @(Get-RequiredProperty $recordedErrorCensus 'UnapprovedHardDiagnosticKinds' `
        'run.outcome.errorCensus') | ForEach-Object {
        '{0}={1}' -f
            (Require-Text (Get-RequiredProperty $_ 'kind' 'recorded diagnostic kind') `
                'recorded diagnostic kind.kind'),
            (Require-Integer (Get-RequiredProperty $_ 'count' 'recorded diagnostic kind') `
                'recorded diagnostic kind.count')
    })
$derivedUnapprovedKindRows = @(
    @(Get-RequiredProperty $derivedErrorCensus 'UnapprovedHardDiagnosticKinds' `
        'derived error census') | ForEach-Object {
        '{0}={1}' -f
            (Require-Text (Get-RequiredProperty $_ 'kind' 'derived diagnostic kind') `
                'derived diagnostic kind.kind'),
            (Require-Integer (Get-RequiredProperty $_ 'count' 'derived diagnostic kind') `
                'derived diagnostic kind.count')
    })
Assert-EqualSet $recordedUnapprovedKindRows $derivedUnapprovedKindRows `
    'Recorded and re-derived unapproved diagnostic kinds'
$errorCensus = $derivedErrorCensus
$diagnosticIntegers = [ordered]@{}
foreach ($field in @(
        'HardDiagnosticCount', 'ScriptErrors', 'EngineErrors', 'PartisanErrors',
        'CrashMarkers', 'PartisanSeverityLineCount', 'ApprovedStockDiagnosticCount',
        'ApprovedIntentionalDiagnosticCount', 'MalformedHardDiagnosticCount',
        'UnapprovedHardDiagnosticCount', 'CanonicalScriptLogCount',
        'CanonicalConsoleLogCount', 'CanonicalErrorLogCount',
        'CanonicalCrashLogCount', 'AuxiliaryUnapprovedEventCount')) {
    $diagnosticIntegers[$field] = Require-Integer `
        (Get-RequiredProperty $errorCensus $field 'run.outcome.errorCensus') `
        "run.outcome.errorCensus.$field"
    if ($diagnosticIntegers[$field] -lt 0) {
        throw "run.outcome.errorCensus.$field must be nonnegative."
    }
}
$diagnosticBooleans = [ordered]@{}
foreach ($field in @(
        'Valid', 'HardDiagnosticFree', 'ChannelArithmeticValid',
        'CategoryArithmeticValid', 'LifecycleMarkersValid',
        'IdentityBaselinePairValid', 'IntentionalFixtureStructureExact',
        'IntentionalFixtureSetValid', 'CanonicalLogPairSameDirectory',
        'AuxiliaryLogPairSameDirectory', 'AuxiliaryDiagnosticsValid',
        'ErrorLogProjectionExact', 'CrashLogProjectionExact',
        'IntentionalMissionConvoyAdmissionDiagnosticsProven',
        'IntentionalMissionConvoySettlementDiagnosticProven',
        'IntentionalMissionConvoyCorruptionDiagnosticsProven',
        'IntentionalMissionConvoyWatchdogDiagnosticProven')) {
    $diagnosticBooleans[$field] = Require-Boolean `
        (Get-RequiredProperty $errorCensus $field 'run.outcome.errorCensus') `
        "run.outcome.errorCensus.$field"
}
$validationDiagnosticBooleans = [ordered]@{}
foreach ($field in @(
        'IntentionalMissionConvoyAdmissionDiagnosticsProven',
        'IntentionalMissionConvoySettlementDiagnosticProven',
        'IntentionalMissionConvoyCorruptionDiagnosticsProven',
        'IntentionalMissionConvoyWatchdogDiagnosticProven')) {
    $validationDiagnosticBooleans[$field] = Require-Boolean `
        (Get-RequiredProperty $validation $field 'run.outcome.validation') `
        "run.outcome.validation.$field"
}
$unapprovedKinds = @(Get-RequiredProperty $errorCensus 'UnapprovedHardDiagnosticKinds' 'run.outcome.errorCensus')
$unapprovedKindsTotal = 0
foreach ($kind in $unapprovedKinds) {
    $kindName = Require-Text (Get-RequiredProperty $kind 'kind' 'unapproved diagnostic kind') `
        'unapproved diagnostic kind.kind'
    $kindCount = Require-Integer (Get-RequiredProperty $kind 'count' "unapproved diagnostic kind $kindName") `
        "unapproved diagnostic kind $kindName.count"
    if ($kindCount -le 0) {
        throw "Unapproved diagnostic kind $kindName must have a positive count."
    }
    $unapprovedKindsTotal += $kindCount
}
$channelArithmeticDerived = $diagnosticIntegers.HardDiagnosticCount -eq
    ($diagnosticIntegers.ScriptErrors + $diagnosticIntegers.EngineErrors)
$categoryArithmeticDerived = $diagnosticIntegers.HardDiagnosticCount -eq
    ($diagnosticIntegers.ApprovedStockDiagnosticCount +
        $diagnosticIntegers.ApprovedIntentionalDiagnosticCount +
        $diagnosticIntegers.UnapprovedHardDiagnosticCount)
$unapprovedKindArithmeticDerived = $unapprovedKindsTotal -eq
    $diagnosticIntegers.UnapprovedHardDiagnosticCount
$hardDiagnosticFreeDerived = $diagnosticBooleans.HardDiagnosticFree -eq
    ($diagnosticIntegers.HardDiagnosticCount -eq 0)

$classifierChecks = Require-Integer `
    (Get-RequiredProperty $outcome 'hardDiagnosticClassifierChecks' 'run.outcome') `
    'run.outcome.hardDiagnosticClassifierChecks'
$diagnosticAxisCommonPassed = $diagnosticBooleans.Valid -and
    $diagnosticBooleans.ChannelArithmeticValid -and
    $diagnosticBooleans.CategoryArithmeticValid -and
    $channelArithmeticDerived -and $categoryArithmeticDerived -and
    $unapprovedKindArithmeticDerived -and $hardDiagnosticFreeDerived -and
    $diagnosticBooleans.LifecycleMarkersValid -and
    $diagnosticBooleans.IdentityBaselinePairValid -and
    $diagnosticBooleans.IntentionalFixtureStructureExact -and
    $diagnosticBooleans.IntentionalFixtureSetValid -and
    $diagnosticBooleans.CanonicalLogPairSameDirectory -and
    $diagnosticBooleans.AuxiliaryLogPairSameDirectory -and
    $diagnosticBooleans.AuxiliaryDiagnosticsValid -and
    $diagnosticBooleans.ErrorLogProjectionExact -and
    $diagnosticBooleans.CrashLogProjectionExact -and
    $diagnosticIntegers.CrashMarkers -eq 0 -and
    $diagnosticIntegers.PartisanSeverityLineCount -eq 0 -and
    $diagnosticIntegers.MalformedHardDiagnosticCount -eq 0 -and
    $diagnosticIntegers.CanonicalScriptLogCount -eq 1 -and
    $diagnosticIntegers.CanonicalConsoleLogCount -eq 1 -and
    $diagnosticIntegers.CanonicalErrorLogCount -eq 1 -and
    $diagnosticIntegers.CanonicalCrashLogCount -eq 1 -and
    $diagnosticIntegers.AuxiliaryUnapprovedEventCount -eq 0 -and
    $diagnosticIntegers.UnapprovedHardDiagnosticCount -eq 0 -and
    $classifierChecks -eq 38
$diagnosticAxisPassed = if ($isCorrectedCanary) {
    $diagnosticAxisCommonPassed -and
        $diagnosticIntegers.HardDiagnosticCount -eq 2 -and
        $diagnosticIntegers.ScriptErrors -eq 2 -and
        $diagnosticIntegers.EngineErrors -eq 0 -and
        $diagnosticIntegers.PartisanErrors -eq 0 -and
        $diagnosticIntegers.ApprovedStockDiagnosticCount -eq 2 -and
        $diagnosticIntegers.ApprovedIntentionalDiagnosticCount -eq 0 -and
        -not $diagnosticBooleans.IntentionalMissionConvoyAdmissionDiagnosticsProven -and
        -not $diagnosticBooleans.IntentionalMissionConvoySettlementDiagnosticProven -and
        -not $diagnosticBooleans.IntentionalMissionConvoyCorruptionDiagnosticsProven -and
        -not $diagnosticBooleans.IntentionalMissionConvoyWatchdogDiagnosticProven -and
        -not $validationDiagnosticBooleans.IntentionalMissionConvoyAdmissionDiagnosticsProven -and
        -not $validationDiagnosticBooleans.IntentionalMissionConvoySettlementDiagnosticProven -and
        -not $validationDiagnosticBooleans.IntentionalMissionConvoyCorruptionDiagnosticsProven -and
        -not $validationDiagnosticBooleans.IntentionalMissionConvoyWatchdogDiagnosticProven
}
else {
    $diagnosticAxisCommonPassed -and
        $diagnosticIntegers.HardDiagnosticCount -eq 15 -and
        $diagnosticIntegers.ScriptErrors -eq 15 -and
        $diagnosticIntegers.EngineErrors -eq 0 -and
        $diagnosticIntegers.PartisanErrors -eq 13 -and
        $diagnosticIntegers.ApprovedStockDiagnosticCount -eq 2 -and
        $diagnosticIntegers.ApprovedIntentionalDiagnosticCount -eq 13 -and
        $diagnosticBooleans.IntentionalMissionConvoyAdmissionDiagnosticsProven -and
        $diagnosticBooleans.IntentionalMissionConvoySettlementDiagnosticProven -and
        $diagnosticBooleans.IntentionalMissionConvoyCorruptionDiagnosticsProven -and
        $diagnosticBooleans.IntentionalMissionConvoyWatchdogDiagnosticProven -and
        $validationDiagnosticBooleans.IntentionalMissionConvoyAdmissionDiagnosticsProven -and
        $validationDiagnosticBooleans.IntentionalMissionConvoySettlementDiagnosticProven -and
        $validationDiagnosticBooleans.IntentionalMissionConvoyCorruptionDiagnosticsProven -and
        $validationDiagnosticBooleans.IntentionalMissionConvoyWatchdogDiagnosticProven
}

$cleanupErrors = @(Get-RequiredProperty $cleanup 'cleanupPhaseErrors' 'run.cleanup')
$cleanupPassed = $cleanupErrors.Count -eq 0 -and
    (Require-Boolean (Get-RequiredProperty $cleanup 'monitoringRootsAreDetectionOnly' 'run.cleanup') `
        'run.cleanup.monitoringRootsAreDetectionOnly')
foreach ($field in @(
        'guardRemaining', 'ownedProcessesRemaining', 'newEngineProcessesRemaining',
        'unclaimedEngineProcessesObserved', 'newDefaultEntriesRemaining',
        'modifiedDefaultFiles', 'deletedDefaultEntries', 'missingDefaultRoots',
        'externalSpillEntriesRemaining', 'modifiedSpillFiles',
        'deletedSpillEntries', 'missingSpillRoots', 'cleanupPhaseErrorCount')) {
    if ((Require-Integer (Get-RequiredProperty $cleanup $field 'run.cleanup') `
            "run.cleanup.$field") -ne 0) {
        $cleanupPassed = $false
    }
}
if (-not $cleanupPassed) {
    throw 'A portable Campaign Debug release index requires zero cleanup and spill residue.'
}

$mountAttestation = Get-RequiredProperty $outcome 'mountAttestation' 'run.outcome'
$wrapperCaptureSuccess =
    (Require-Boolean (Get-RequiredProperty $outcome 'armed' 'run.outcome') 'run.outcome.armed') -and
    (Require-Boolean (Get-RequiredProperty $outcome 'started' 'run.outcome') 'run.outcome.started') -and
    (Require-Boolean (Get-RequiredProperty $outcome 'completed' 'run.outcome') 'run.outcome.completed') -and
    (Require-Boolean (Get-RequiredProperty $outcome 'candidateBoundaryVerified' 'run.outcome') `
        'run.outcome.candidateBoundaryVerified') -and
    (Require-Boolean (Get-RequiredProperty $outcome 'artifactsStable' 'run.outcome') `
        'run.outcome.artifactsStable') -and
    (Require-Boolean (Get-RequiredProperty $outcome 'evidenceCaptured' 'run.outcome') `
        'run.outcome.evidenceCaptured') -and
    (Require-Boolean (Get-RequiredProperty $mountAttestation 'Valid' 'run.outcome.mountAttestation') `
        'run.outcome.mountAttestation.Valid') -and
    (Require-Boolean (Get-RequiredProperty $mountAttestation 'Packed' 'run.outcome.mountAttestation') `
        'run.outcome.mountAttestation.Packed')
if (-not $wrapperCaptureSuccess) {
    throw 'A portable Campaign Debug release index requires a complete exact-package wrapper capture.'
}

$guardedRunSucceeded = Require-Boolean (Get-RequiredProperty $outcome 'success' 'run.outcome') `
    'run.outcome.success'
$outcomeError = [string](Get-RequiredProperty $outcome 'error' 'run.outcome')
$finalOrphanCleanupPass = Require-Boolean `
    (Get-RequiredProperty $validation 'FinalOrphanCleanupPass' 'run.outcome.validation') `
    'run.outcome.validation.FinalOrphanCleanupPass'
$finalOrphanActiveGroups = Require-IntegerScalar `
    (Get-RequiredProperty $validation 'FinalOrphanActiveGroups' 'run.outcome.validation') `
    'run.outcome.validation.FinalOrphanActiveGroups'
$acceptedCorrectedCanary = $false
$proofCommonPassed = $false
$acceptedFull = $false
$acceptedInternal = $false
if ($isCorrectedCanary) {
    $correctedCanaryProofAxisPassed = $correctedCanaryProofAxisPassed -and
        $finalOrphanCleanupPass -and $finalOrphanActiveGroups -eq 0
    $acceptedCorrectedCanary = $correctedCanaryProofAxisPassed -and
        $diagnosticAxisPassed -and $guardedRunSucceeded -and
        [string]::IsNullOrWhiteSpace($outcomeError)
}
else {
    $proofCommonPassed = $rawCaseCounts.fail -eq 0 -and
        $rawCaseCounts.blocked -eq 0 -and
        $failedAssertionIds.Count -eq 0 -and $blockedRecords.Count -eq 0 -and
        $unsupportedWarningIds.Count -eq 0 -and
        $unsupportedSkippedIds.Count -eq 0 -and
        $artifactValidationValid -and
        $rawCertificationPassed -and $rawCertificationCounts.fail -eq 0 -and
        $rawCertificationCounts.blocked -eq 0 -and
        $rawCertificationCounts.warn -eq 0 -and
        $nonzeroStateDiffRowCount -eq 0 -and $finalOrphanCleanupPass -and
        $finalOrphanActiveGroups -eq 0
    $acceptedFull = $proofCommonPassed -and $blockedRecords.Count -eq 0 -and
        $warningAssertionIds.Count -eq 0 -and $rawCaseCounts.warn -eq 0 -and
        $guardedRunSucceeded -and
        [string]::IsNullOrWhiteSpace($outcomeError) -and $diagnosticAxisPassed
    if (-not $acceptedFull -and $proofCommonPassed -and
        $externalAdvisoryLinkageValid -and $guardedRunSucceeded -and
        [string]::IsNullOrWhiteSpace($outcomeError) -and $diagnosticAxisPassed) {
        try {
            Assert-EqualSet $externalRequiredAdvisoryIds $externalAdvisoryIds `
                'External-required advisory assertion set'
            if ($rawCaseCounts.warn -gt 0) {
                $acceptedInternal = $true
            }
        }
        catch {
            $acceptedInternal = $false
        }
    }
}

$status = 'failed-full-profile'
$acceptanceDisposition = 'rejected-full-profile'
$releaseDisposition = 'remain-no-go'
$findingStatus = 'release-blocking-red-full-profile'
$findingDefect = 'One or more full-profile acceptance axes failed.'
$findingNextStep = 'Repair every retained proof or diagnostic rejection and seal a new immutable candidate.'
if ($isCorrectedCanary) {
    $status = 'failed-corrected-canary'
    $acceptanceDisposition = 'rejected-corrected-canary'
    $releaseDisposition = 'replacement-required'
    $findingStatus = 'rejected-corrected-canary'
    $findingDefect = 'One or more corrected-canary acceptance axes failed.'
    $findingNextStep = 'Repair the retained canary defect, seal a replacement candidate, and rerun the corrected canary.'
    if ($acceptedCorrectedCanary) {
        $status = 'passed-noncertifying'
        $acceptanceDisposition = 'accepted-noncertifying'
        $releaseDisposition = 'proceed-full-profile'
        $findingStatus = 'accepted-noncertifying'
        $findingDefect = 'none'
        $findingNextStep = 'Run the full Campaign Debug profile against the unchanged candidate.'
    }
}
elseif ($acceptedFull) {
    $status = 'passed-full-certification'
    $acceptanceDisposition = 'accepted-full-profile'
    $releaseDisposition = 'advance-external-gates'
    $findingStatus = 'accepted-full-profile'
    $findingDefect = 'none'
    $findingNextStep = 'Advance the unchanged package to the external release gates.'
}
elseif ($acceptedInternal) {
    $status = 'passed-internal-profile-external-required'
    $acceptanceDisposition = 'accepted-internal-profile'
    $releaseDisposition = 'advance-external-required'
    $findingStatus = 'accepted-internal-profile-external-required'
    $findingDefect = 'none'
    $findingNextStep = 'Run the exact retained external-required advisory set against the unchanged package.'
}

$startedUtc = Require-Text (Get-RequiredProperty $run 'startedUtc' 'run') 'run.startedUtc'
$completedUtc = Require-Text (Get-RequiredProperty $run 'completedUtc' 'run') 'run.completedUtc'
$runtimeSeconds = Require-Integer (Get-RequiredProperty $outcome 'runtimeSeconds' 'run.outcome') `
    'run.outcome.runtimeSeconds'
if ($isCorrectedCanary) {
    $startedUtcValue = Require-NormalizedUtcTimestamp $startedUtc 'run.startedUtc'
    $completedUtcValue = Require-NormalizedUtcTimestamp $completedUtc 'run.completedUtc'
    $wallClockSeconds = ($completedUtcValue - $startedUtcValue).TotalSeconds
    if ($completedUtcValue -le $startedUtcValue -or
        $runtimeSeconds -le 0 -or
        $runtimeSeconds -gt [Math]::Ceiling($wallClockSeconds)) {
        throw 'The corrected-canary capture timestamps/runtime are inconsistent.'
    }
}
$runLeafId = Split-Path -Leaf $runRoot
if ($runLeafId -cnotmatch '^\d{8}T\d{6}Z-[0-9a-f]{32}$') {
    throw 'The retained Campaign Debug run directory does not have a canonical run-leaf ID.'
}
$runEnvelopeSha = $runSnapshot.Sha256

$index = [ordered]@{
    schemaVersion = 2
    evidenceKind = $indexEvidenceKind
    policyId = $policyId
    source = [ordered]@{
        bundleRelativePath = "$candidateId/campaign-debug/$runLeafId"
        runEnvelopePath = 'run.json'
        runEnvelopeSha256 = $runEnvelopeSha
        rawArtifactPath = $artifactRows[0]
        rawArtifactSha256 = $rowMap[$artifactRows[0]].Sha256
        stateDiffPath = $diffRows[0]
        stateDiffSha256 = $rowMap[$diffRows[0]].Sha256
        textSummaryPath = $textSummaryRows[0]
        textSummarySha256 = $rowMap[$textSummaryRows[0]].Sha256
        fileCount = $fileRows.Count
        files = $fileRows
        filesRehashed = $true
    }
    candidate = [ordered]@{
        candidateId = $candidateId
        candidateVersion = $candidateVersion
        candidateSourceHead = $candidateSourceHead
        embeddedBuildSha = $embeddedBuildSha
        embeddedBuildUtc = $embeddedBuildUtc
        embeddedBuildLabel = $embeddedBuildLabel
        campaignSchema = $campaignSchema
        runtimeSettingsSchema = $runtimeSettingsSchema
        addonId = $addonId
        addonGuid = $addonGuid
        packageHashAlgorithm = $packageHashAlgorithm
        packageSha256 = $packageSha
        manifestSha256 = $manifestSha
        readySha256 = $readySha
        workbenchCrc = $workbenchCrc
        runtimeUseDispositionAtCapture = [string](Get-RequiredProperty $candidate 'runtimeUseDisposition' 'run.candidate')
        runtimeRole = $runtimeRole
        diagnosticExecutable = Get-RequiredProperty $candidate 'diagnosticExecutable' 'run.candidate'
        recordedDiagnosticExecutable = Get-RequiredProperty $candidate `
            'recordedDiagnosticExecutable' 'run.candidate'
        recordedRuntimeExecutable = Get-RequiredProperty $candidate `
            'recordedRuntimeExecutable' 'run.candidate'
    }
    harness = [ordered]@{
        gitHead = $harnessHead
        dirty = $harnessDirty
        campaignRunnerSha256 = $harnessHashes.campaignRunnerSha256
        campaignRunnerGitBlobSha256 = $harnessHashes.campaignRunnerGitBlobSha256
        candidateModuleSha256 = $harnessHashes.candidateModuleSha256
        candidateModuleGitBlobSha256 = $harnessHashes.candidateModuleGitBlobSha256
        releaseIndexProducerSha256 = $producerSha
        releaseIndexProducerGitBlobSha256 = $harnessHashes.releaseIndexProducerGitBlobSha256
        releaseDocsConsumerSha256 = $consumerSha
        releaseDocsConsumerGitBlobSha256 = $harnessHashes.releaseDocsConsumerGitBlobSha256
    }
    settings = [ordered]@{
        schemaVersion = [int](Get-RequiredProperty $settings 'schemaVersion' 'run.settings')
        sha256 = $settingsSha
        guardedRuntimeCopy = $true
    }
    capture = [ordered]@{
        runLeafId = $runLeafId
        runId = $runId
        profile = $profile
        proofScope = $proofScope
        startedUtc = $startedUtc
        completedUtc = $completedUtc
        runtimeSeconds = $runtimeSeconds
    }
    result = [ordered]@{
        status = $status
        acceptanceDisposition = $acceptanceDisposition
        releaseDisposition = $releaseDisposition
        wrapperCaptureSuccess = $wrapperCaptureSuccess
        guardedRunSucceeded = $guardedRunSucceeded
        runtimeOutcomeSuccess = $guardedRunSucceeded
        armed = $true
        started = $true
        completed = $true
        candidateBoundaryVerified = $true
        mountPacked = $true
        artifactsStable = $true
        evidenceCaptured = $true
        artifactSchemaValidationValid = $artifactValidationValid
        certificationPassed = $rawCertificationPassed
        error = $outcomeError
    }
    proof = [ordered]@{
        startedAtSecond = [int](Get-RequiredProperty $raw 'm_iStartedAtSecond' 'raw full profile')
        endedAtSecond = [int](Get-RequiredProperty $raw 'm_iEndedAtSecond' 'raw full profile')
        caseCount = $rawCaseCounts.caseCount
        pass = $rawCaseCounts.pass
        warn = $rawCaseCounts.warn
        fail = $rawCaseCounts.fail
        blocked = $rawCaseCounts.blocked
        skipped = $rawCaseCounts.skipped
        certificationRequired = $rawCertificationCounts.required
        certificationProven = $rawCertificationCounts.proven
        certificationFail = $rawCertificationCounts.fail
        certificationBlocked = $rawCertificationCounts.blocked
        certificationWarn = $rawCertificationCounts.warn
        assertionCount = $rawAssertionCount
        stateDiffRows = $stateDiffRowCount
        nonzeroStateDiffRows = $nonzeroStateDiffRowCount
        failedAssertionIds = $failedAssertionIds.ToArray()
        warningAssertionIds = $warningAssertionIds.ToArray()
        warningAssertions = $warningRecords.ToArray()
        unsupportedWarningAssertionIds = @($unsupportedWarningIds)
        skippedAssertionIds = $skippedAssertionIds.ToArray()
        approvedNoncertifyingSkipIds = @($skippedAssertionIds | Where-Object {
            $_ -cin $approvedNoncertifyingSkipIds
        })
        unsupportedSkippedAssertionIds = @($unsupportedSkippedIds)
        externalRequiredAdvisoryIds = @($externalAdvisoryIds)
        externalRequiredAdvisories = @($externalAdvisoryRecords)
        blockedAssertions = $blockedRecords.ToArray()
        intentionalMissionConvoyAdmissionDiagnosticsProven =
            $validationDiagnosticBooleans.IntentionalMissionConvoyAdmissionDiagnosticsProven
        intentionalMissionConvoySettlementDiagnosticProven =
            $validationDiagnosticBooleans.IntentionalMissionConvoySettlementDiagnosticProven
        intentionalMissionConvoyCorruptionDiagnosticsProven =
            $validationDiagnosticBooleans.IntentionalMissionConvoyCorruptionDiagnosticsProven
        intentionalMissionConvoyWatchdogDiagnosticProven =
            $validationDiagnosticBooleans.IntentionalMissionConvoyWatchdogDiagnosticProven
        finalOrphanCleanupPass = $finalOrphanCleanupPass
        finalOrphanActiveGroups = $finalOrphanActiveGroups
    }
    diagnostics = [ordered]@{
        valid = $diagnosticBooleans.Valid
        classificationValid = $diagnosticBooleans.Valid
        hardDiagnosticFree = $diagnosticBooleans.HardDiagnosticFree
        hardDiagnosticCount = $diagnosticIntegers.HardDiagnosticCount
        scriptErrors = $diagnosticIntegers.ScriptErrors
        engineErrors = $diagnosticIntegers.EngineErrors
        partisanErrors = $diagnosticIntegers.PartisanErrors
        crashMarkers = $diagnosticIntegers.CrashMarkers
        partisanSeverityLineCount = $diagnosticIntegers.PartisanSeverityLineCount
        approvedStockDiagnosticCount = $diagnosticIntegers.ApprovedStockDiagnosticCount
        approvedIntentionalDiagnosticCount = $diagnosticIntegers.ApprovedIntentionalDiagnosticCount
        unapprovedHardDiagnosticCount = $diagnosticIntegers.UnapprovedHardDiagnosticCount
        unapprovedHardDiagnosticKinds = $unapprovedKinds
        classifierSelfTestCount = $classifierChecks
        malformedHardDiagnosticCount = $diagnosticIntegers.MalformedHardDiagnosticCount
        channelArithmeticValid = $diagnosticBooleans.ChannelArithmeticValid
        categoryArithmeticValid = $diagnosticBooleans.CategoryArithmeticValid
        lifecycleMarkersValid = $diagnosticBooleans.LifecycleMarkersValid
        identityBaselinePairValid = $diagnosticBooleans.IdentityBaselinePairValid
        intentionalFixtureStructureExact = $diagnosticBooleans.IntentionalFixtureStructureExact
        intentionalFixtureSetValid = $diagnosticBooleans.IntentionalFixtureSetValid
        canonicalScriptLogCount = $diagnosticIntegers.CanonicalScriptLogCount
        canonicalConsoleLogCount = $diagnosticIntegers.CanonicalConsoleLogCount
        canonicalErrorLogCount = $diagnosticIntegers.CanonicalErrorLogCount
        canonicalCrashLogCount = $diagnosticIntegers.CanonicalCrashLogCount
        canonicalLogPairSameDirectory = $diagnosticBooleans.CanonicalLogPairSameDirectory
        auxiliaryLogPairSameDirectory = $diagnosticBooleans.AuxiliaryLogPairSameDirectory
        auxiliaryDiagnosticsValid = $diagnosticBooleans.AuxiliaryDiagnosticsValid
        errorLogProjectionExact = $diagnosticBooleans.ErrorLogProjectionExact
        crashLogProjectionExact = $diagnosticBooleans.CrashLogProjectionExact
        auxiliaryUnapprovedEventCount =
            $diagnosticIntegers.AuxiliaryUnapprovedEventCount
    }
    cleanup = $cleanup
    integrity = [ordered]@{
        envelopeSha256 = $runEnvelopeSha
        runSummarySha256 = $runEnvelopeSha
        rawArtifactSha256 = $rowMap[$artifactRows[0]].Sha256
        envelopeFileCount = $fileRows.Count
        envelopeFilesRehashed = $true
        releaseIndexProducerSha256 = $producerSha
        releaseDocsConsumerSha256 = $consumerSha
    }
    finding = [ordered]@{
        status = $findingStatus
        defect = $findingDefect
        nextStep = $findingNextStep
    }
}

if ($isCorrectedCanary) {
    $index.proof.Add('focusedCaseId', $focusedCaseId)
    $index.proof.Add('focusedCaseStatus', $focusedCaseStatus)
    $index.proof.Add('focusedAssertionCount', $focusedAssertionCount)
    $index.proof.Add('focusedAssertionsPassed', $focusedAssertionsPassed)
    $index.proof.Add('focusedAssertionSetExact', $focusedAssertionSetExact)
    $index.proof.Add(
        'focusedAssertionsCertificationExact',
        $focusedAssertionsCertificationExact)
    $index.proof.Add('correctedCanaryCaseSetExact', $correctedCanaryCaseSetExact)
    $index.proof.Add(
        'correctedCanaryAssertionManifestExact',
        $correctedCanaryAssertionManifestExact)
    $index.proof.Add(
        'correctedCanaryWarningContractExact',
        $correctedCanaryWarningContractExact)
    $index.proof.Add(
        'correctedCanaryBlockedContractExact',
        $correctedCanaryBlockedContractExact)
    $index.proof.Add(
        'correctedCanaryStateDiffManifestExact',
        $correctedCanaryStateDiffManifestExact)
    $index.proof.Add(
        'correctedCanaryOrphanContractExact',
        $correctedCanaryOrphanContractExact)
    $index.proof.Add(
        'correctedCanaryAssertionSkipFree',
        $correctedCanaryAssertionSkipFree)
    $index.proof.Add(
        'correctedCanaryProofAxisPassed',
        $correctedCanaryProofAxisPassed)
}

Assert-NoLocalAbsolutePathValue $index 'The portable release index'

# Publication is the trust boundary. Re-read every bound byte, reparse the
# authoritative JSON documents, rerun the semantic validator/census, and
# recheck the clean immutable tool bindings immediately before create-new or
# byte-identical publication.
$finalRunSnapshot = Get-StableFileSnapshot -Path $runItem.FullName
if ($finalRunSnapshot.Sha256 -cne $runSnapshot.Sha256 -or
    -not (Test-ExactByteArray -Expected $runSnapshot.Bytes -Actual $finalRunSnapshot.Bytes)) {
    throw 'The retained run envelope changed before release-index publication.'
}
$finalEntries = @(Get-ChildItem -LiteralPath $runRoot -Recurse -Force)
if (@($finalEntries | Where-Object {
            ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0
        }).Count -ne 0) {
    throw 'The retained Campaign Debug run bundle gained a reparse point before publication.'
}
$finalPaths = @($finalEntries | Where-Object { -not $_.PSIsContainer } | Where-Object {
    $_.FullName -cne $runItem.FullName -and $_.FullName -cne $outputFullPath
} | ForEach-Object {
    $_.FullName.Substring($runRoot.TrimEnd('\', '/').Length + 1).Replace('\', '/')
})
Assert-EqualSet $rowPaths.ToArray() $finalPaths `
    'Final run-envelope inventory and retained raw file set'
$finalSnapshotMap = @{}
foreach ($path in $rowPaths) {
    $finalSnapshot = Get-StableFileSnapshot -Path $rowMap[$path].Item.FullName
    if ($finalSnapshot.Sha256 -cne $rowMap[$path].Sha256 -or
        -not (Test-ExactByteArray `
            -Expected $rowMap[$path].Snapshot.Bytes `
            -Actual $finalSnapshot.Bytes)) {
        throw "The retained raw file $path changed before release-index publication."
    }
    $finalSnapshotMap[$path] = $finalSnapshot
}
try {
    Assert-NoDuplicateJsonObjectKeys `
        -Text $finalRunSnapshot.Text `
        -Label 'The final retained run envelope'
    Assert-NoDuplicateJsonObjectKeys `
        -Text $finalSnapshotMap[$manifestRows[0]].Text `
        -Label 'The final retained candidate manifest'
    Assert-NoDuplicateJsonObjectKeys `
        -Text $finalSnapshotMap[$readyRows[0]].Text `
        -Label 'The final retained candidate ready marker'
    Assert-NoDuplicateJsonObjectKeys `
        -Text $finalSnapshotMap[$artifactRows[0]].Text `
        -Label 'The final retained raw Campaign Debug artifact'
    $null = $finalRunSnapshot.Text | ConvertFrom-Json
    $null = $finalSnapshotMap[$manifestRows[0]].Text |
        ConvertFrom-Json
    $null = $finalSnapshotMap[$readyRows[0]].Text |
        ConvertFrom-Json
    $null = $finalSnapshotMap[$artifactRows[0]].Text |
        ConvertFrom-Json
}
catch {
    throw "A bound Campaign Debug JSON document failed final reparse: $($_.Exception.Message)"
}
$finalSemanticValidation = Invoke-SnapshotSemanticValidation `
    -SnapshotMap $finalSnapshotMap `
    -RunnerSnapshot $runnerToolSnapshot `
    -ArtifactPath $artifactRows[0] `
    -SummaryPath $textSummaryRows[0] `
    -StateDiffPath $diffRows[0] `
    -ExpectedSha $embeddedBuildSha `
    -ExpectedUtc $embeddedBuildUtc `
    -ExpectedLabel $embeddedBuildLabel `
    -ExpectedProfile $profile
$finalSemanticJson = $finalSemanticValidation | ConvertTo-Json -Depth 64 -Compress
if ($finalSemanticJson -cne $initialSemanticJson) {
    throw 'The Campaign Debug semantic validation or diagnostic census changed before publication.'
}
if ($isCorrectedCanary) {
    if ((Get-RepositoryHead -CheckoutRoot $checkoutRoot) -cne $harnessHead -or
        @(Get-RepositoryStatusRows -CheckoutRoot $checkoutRoot).Count -ne 0) {
        throw 'The corrected-canary harness checkout changed before publication.'
    }
    foreach ($binding in @(
            @('campaignRunnerGitBlobSha256', $runnerPath),
            @('candidateModuleGitBlobSha256', $candidateModulePath),
            @('releaseIndexProducerGitBlobSha256', $producerPath),
            @('releaseDocsConsumerGitBlobSha256', $consumerPath))) {
        $finalWorktreeSha = (Get-FileHash `
            -LiteralPath $binding[1] `
            -Algorithm SHA256).Hash.ToLowerInvariant()
        $finalBlobSha = Get-ImmutableGitBlobSha256 `
            -CheckoutRoot $checkoutRoot `
            -Revision $harnessHead `
            -FilePath $binding[1]
        if ($finalWorktreeSha -cne $finalBlobSha -or
            $harnessHashes[$binding[0]] -cne $finalBlobSha) {
            throw "The corrected-canary harness tool $($binding[0]) changed before publication."
        }
    }
}
Invoke-CampaignDebugReleaseIndexSelfTestLateDriftBarrier `
    -RunRootPath $runRoot
$publicationInputPaths = New-Object Collections.Generic.List[string]
[void]$publicationInputPaths.Add($runItem.FullName)
foreach ($path in $rowPaths) {
    [void]$publicationInputPaths.Add($rowMap[$path].Item.FullName)
}
foreach ($toolPath in @(
        $runnerPath, $candidateModulePath, $producerPath, $consumerPath)) {
    [void]$publicationInputPaths.Add($toolPath)
}
$assertPublicationInputsUnchanged = {
    param([Parameter(Mandatory = $true)][string]$Phase)

    $observedRunSnapshot = Get-StableFileSnapshot -Path $runItem.FullName
    if ($observedRunSnapshot.Sha256 -cne $runSnapshot.Sha256 -or
        -not (Test-ExactByteArray `
            -Expected $runSnapshot.Bytes `
            -Actual $observedRunSnapshot.Bytes)) {
        throw "The retained run envelope changed $Phase release-index publication."
    }
    $observedEntries = @(Get-ChildItem -LiteralPath $runRoot -Recurse -Force)
    if (@($observedEntries | Where-Object {
                ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0
            }).Count -ne 0) {
        throw "The retained Campaign Debug run bundle gained a reparse point $Phase publication."
    }
    $observedPaths = @($observedEntries |
        Where-Object { -not $_.PSIsContainer } |
        Where-Object {
            $_.FullName -cne $runItem.FullName -and
            $_.FullName -cne $outputFullPath
        } | ForEach-Object {
            $_.FullName.Substring($runRoot.TrimEnd('\', '/').Length + 1).
                Replace('\', '/')
        })
    Assert-EqualSet $rowPaths.ToArray() $observedPaths `
        "Run-envelope inventory and retained raw file set $Phase publication"
    foreach ($path in $rowPaths) {
        $observedSnapshot = Get-StableFileSnapshot `
            -Path $rowMap[$path].Item.FullName
        if ($observedSnapshot.Sha256 -cne $rowMap[$path].Sha256 -or
            -not (Test-ExactByteArray `
                -Expected $rowMap[$path].Snapshot.Bytes `
                -Actual $observedSnapshot.Bytes)) {
            throw "The retained raw file $path changed $Phase release-index publication."
        }
    }
    foreach ($binding in @(
            @($runnerPath, $runnerSha),
            @($candidateModulePath, $candidateModuleSha),
            @($producerPath, $producerSha),
            @($consumerPath, $consumerSha))) {
        $observedToolSha = (Get-FileHash `
            -LiteralPath $binding[0] `
            -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($observedToolSha -cne [string]$binding[1]) {
            throw "A bound Campaign Debug harness tool changed $Phase publication."
        }
    }
    if ($isCorrectedCanary) {
        if ((Get-RepositoryHead -CheckoutRoot $checkoutRoot) -cne $harnessHead -or
            @(Get-RepositoryStatusRows -CheckoutRoot $checkoutRoot).Count -ne 0) {
            throw "The corrected-canary harness checkout changed $Phase publication."
        }
        foreach ($binding in @(
                @('campaignRunnerGitBlobSha256', $runnerPath),
                @('candidateModuleGitBlobSha256', $candidateModulePath),
                @('releaseIndexProducerGitBlobSha256', $producerPath),
                @('releaseDocsConsumerGitBlobSha256', $consumerPath))) {
            $observedBlobSha = Get-ImmutableGitBlobSha256 `
                -CheckoutRoot $checkoutRoot `
                -Revision $harnessHead `
                -FilePath $binding[1]
            if ($observedBlobSha -cne $harnessHashes[$binding[0]]) {
                throw "A corrected-canary immutable harness blob changed $Phase publication."
            }
        }
    }
}

$publicationLocks = @()
$outputPublicationLock = $null
$publishedNewIndex = $false
$indexSha = ''
try {
    $publicationLocks = @(Open-CampaignDebugPublicationReadLocks `
        -Paths $publicationInputPaths.ToArray())
    & $assertPublicationInputsUnchanged 'immediately before'
    $publishedNewIndex = [bool](Write-PortableJson `
        -Path $outputFullPath `
        -Value $index)
    $outputItem = Get-Item -LiteralPath $outputFullPath -Force -ErrorAction Stop
    if ($outputItem.PSIsContainer -or
        ($outputItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'The published Campaign Debug release index is not a regular non-reparse file.'
    }
    $outputPublicationLock = [IO.FileStream]::new(
        $outputFullPath,
        [IO.FileMode]::Open,
        [IO.FileAccess]::Read,
        [IO.FileShare]::Read)
    Invoke-CampaignDebugReleaseIndexSelfTestPublicationWindowBarrier `
        -RunRootPath $runRoot
    & $assertPublicationInputsUnchanged 'after'
    $indexSha = (Get-FileHash `
        -LiteralPath $outputFullPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
}
catch {
    $publicationError = $_
    if ($outputPublicationLock) {
        $outputPublicationLock.Dispose()
        $outputPublicationLock = $null
    }
    if ($publishedNewIndex -and
        (Test-Path -LiteralPath $outputFullPath -PathType Leaf)) {
        try {
            Remove-Item -LiteralPath $outputFullPath -Force -ErrorAction Stop
        }
        catch {
            throw 'Campaign Debug release-index publication failed and its new output could not be rolled back safely.'
        }
    }
    throw $publicationError
}
finally {
    if ($outputPublicationLock) {
        $outputPublicationLock.Dispose()
    }
    Close-CampaignDebugPublicationReadLocks -Streams $publicationLocks
}
[pscustomobject][ordered]@{
    Status = $status
    AcceptanceDisposition = $acceptanceDisposition
    RunEnvelopeSha256 = $runEnvelopeSha
    ReleaseIndexPath = 'release-index.json'
    ReleaseIndexSha256 = $indexSha
    ExternalRequiredAdvisoryIds = @($externalAdvisoryIds)
}
