[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RunEnvelopePath,
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'

$policyId = 'partisan-campaign-debug-full-profile-v2'
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

function Invoke-BoundRunnerSemanticValidation {
    param(
        [Parameter(Mandatory = $true)][string]$RunnerPath,
        [Parameter(Mandatory = $true)][string]$JsonPath,
        [Parameter(Mandatory = $true)][string]$SummaryPath,
        [Parameter(Mandatory = $true)][string]$StateDiffPath,
        [Parameter(Mandatory = $true)][string]$GuardRoot,
        [Parameter(Mandatory = $true)][string]$ExpectedSha,
        [Parameter(Mandatory = $true)][string]$ExpectedUtc,
        [Parameter(Mandatory = $true)][string]$ExpectedLabel
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
        'Test-CampaignDebugArtifacts', 'Get-CampaignDebugHardDiagnosticCensus',
        'Get-GuardErrorCensus')
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
        ExpectedProfile = 'full_certification'
    }
    $diagnosticParameters = @{
        GuardRoot = $GuardRoot
        Profile = 'full_certification'
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
        if ($textValue -match '(?i)file:(?:/+|\\+)') {
            throw "$Label contains a local or rooted absolute path."
        }
        $pathScanValue = [regex]::Replace(
            $textValue,
            '(?i)\bhttps?://(?:localhost|[a-z0-9](?:[a-z0-9.-]*[a-z0-9])?|\[[0-9a-f:.]+\])(?::[0-9]{1,5})?(?:[/?#][^\s"''<>]*)?',
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
    [IO.File]::WriteAllText($Path, $text, (New-Object Text.UTF8Encoding($false)))
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

$runText = Get-Content -Raw -LiteralPath $runItem.FullName
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
if ($fileRows.Count -ne 10) {
    throw 'The full-profile run envelope must retain the canonical ten-file raw set.'
}

$producerPath = $PSCommandPath
$consumerPath = Join-Path $PSScriptRoot 'update-release-docs.ps1'
$runnerPath = Join-Path $PSScriptRoot 'run-guarded-campaign-debug.ps1'
$candidateModulePath = Join-Path $PSScriptRoot 'Partisan.ReleaseCandidate.psm1'
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
    $actualSha = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    if ([long]$item.Length -ne $length -or $actualSha -cne $sha) {
        throw "The retained raw file $path differs from its run-envelope inventory row."
    }
    $rowMap[$path] = [pscustomobject]@{ Item = $item; Length = $length; Sha256 = $sha }
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
        throw 'The retained full-profile raw set is missing or duplicates a canonical file role.'
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
    $retainedManifest = Get-Content -Raw -LiteralPath $manifestItem.FullName | ConvertFrom-Json
    $retainedReady = Get-Content -Raw -LiteralPath $readyItem.FullName | ConvertFrom-Json
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
    (Require-Text (Get-RequiredProperty $launch 'profile' 'run.launch') `
        'run.launch.profile') -cne 'full_certification' -or
    (Require-Text (Get-RequiredProperty $launch 'proofScope' 'run.launch') `
        'run.launch.proofScope') -cne 'full_certification' -or
    -not (Require-Boolean (Get-RequiredProperty $launch 'stagedPackage' 'run.launch') `
        'run.launch.stagedPackage') -or
    -not (Require-Boolean (Get-RequiredProperty $settings 'guardedRuntimeCopy' 'run.settings') `
        'run.settings.guardedRuntimeCopy')) {
    throw 'The retained run is not an active staged full-certification capture.'
}

$rawArtifactItem = $rowMap[$artifactRows[0]].Item
try {
    $raw = Get-Content -Raw -LiteralPath $rawArtifactItem.FullName | ConvertFrom-Json
}
catch {
    throw "The retained full-profile JSON is invalid: $($_.Exception.Message)"
}
$runId = Require-Text (Get-RequiredProperty $raw 'm_sRunId' 'raw full profile') `
    'raw full profile.m_sRunId'
if ($runId -cnotmatch '^seed\d+_t\d+_p\d+_u\d+$' -or
    $artifactRows[0] -cne "raw/campaign-debug/HST_CampaignDebug_$runId.json" -or
    $diffRows[0] -cne "raw/campaign-debug/HST_CampaignDebug_${runId}_state_diff.txt" -or
    $textSummaryRows[0] -cne "raw/campaign-debug/HST_CampaignDebug_${runId}_summary.txt" -or
    (Require-Text (Get-RequiredProperty $raw 'm_sProfile' 'raw full profile') `
        'raw full profile.m_sProfile') -cne 'full_certification' -or
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
    throw 'The retained raw full-profile artifact identity is inconsistent.'
}

$cases = @(Get-RequiredProperty $raw 'm_aCases' 'raw full profile')
$caseStatusCounts = [ordered]@{ PASS = 0; WARN = 0; FAIL = 0; BLOCKED = 0; SKIPPED = 0 }
$certificationStatusCounts = [ordered]@{ PASS = 0; WARN = 0; FAIL = 0; BLOCKED = 0 }
$blockedRecords = New-Object Collections.Generic.List[object]
$warningRecords = New-Object Collections.Generic.List[object]
$failedAssertionIds = New-Object Collections.Generic.List[string]
$warningAssertionIds = New-Object Collections.Generic.List[string]
$skippedAssertionIds = New-Object Collections.Generic.List[string]
$seenCaseIds = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($case in $cases) {
    $caseId = Require-Text (Get-RequiredProperty $case 'm_sCaseId' 'raw case') 'raw case.m_sCaseId'
    if (-not $seenCaseIds.Add($caseId)) {
        throw "The raw full profile duplicates case ID $caseId."
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
        throw "The raw full-profile $($binding[1]) case count is inconsistent."
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
        throw "The raw full-profile $($binding[1]) certification count is inconsistent."
    }
}
$rawCertificationPassed = Require-Boolean `
    (Get-RequiredProperty $raw 'm_bCertificationPassed' 'raw full profile') `
    'raw full profile.m_bCertificationPassed'
$derivedCertificationPassed = $rawCertificationCounts.required -eq $rawCertificationCounts.proven -and
    $rawCertificationCounts.fail -eq 0 -and $rawCertificationCounts.blocked -eq 0 -and
    $rawCertificationCounts.warn -eq 0
if ($rawCertificationPassed -ne $derivedCertificationPassed) {
    throw 'The raw full-profile certification Boolean differs from its assertion census.'
}

$semanticValidation = Invoke-BoundRunnerSemanticValidation `
    -RunnerPath $runnerPath `
    -JsonPath $rawArtifactItem.FullName `
    -SummaryPath $rowMap[$textSummaryRows[0]].Item.FullName `
    -StateDiffPath $rowMap[$diffRows[0]].Item.FullName `
    -GuardRoot (Join-Path $runRoot 'raw') `
    -ExpectedSha $embeddedBuildSha `
    -ExpectedUtc $embeddedBuildUtc `
    -ExpectedLabel $embeddedBuildLabel
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
        'run.outcome.validation.Profile') -cne 'full_certification' -or
    (Require-Text (Get-RequiredProperty $validation 'ProofScope' 'run.outcome.validation') `
        'run.outcome.validation.ProofScope') -cne 'full_certification' -or
    -not (Require-Boolean (Get-RequiredProperty $validation 'FullCertification' 'run.outcome.validation') `
        'run.outcome.validation.FullCertification')) {
    throw 'The guarded-runner validation does not identify the retained full profile.'
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

$diffText = Get-Content -Raw -LiteralPath $rowMap[$diffRows[0]].Item.FullName
$deltaMatches = @([regex]::Matches($diffText, '(?m)^.+\|\s*delta\s+(?<delta>-?\d+)\s*$'))
$nonzeroDeltas = @($deltaMatches | Where-Object { [int64]$_.Groups['delta'].Value -ne 0 })
$stateDiffRowCount = Require-Integer (Get-RequiredProperty $validation 'StateDiffRows' 'run.outcome.validation') `
    'run.outcome.validation.StateDiffRows'
$nonzeroStateDiffRowCount = Require-Integer `
    (Get-RequiredProperty $validation 'NonzeroStateDiffRows' 'run.outcome.validation') `
    'run.outcome.validation.NonzeroStateDiffRows'
if ($deltaMatches.Count -ne $stateDiffRowCount -or
    $nonzeroDeltas.Count -ne $nonzeroStateDiffRowCount) {
    throw 'The retained state diff differs from the guarded-runner validation census.'
}
foreach ($binding in @(
        @('BuildSha', $embeddedBuildSha), @('BuildUtc', $embeddedBuildUtc),
        @('BuildLabel', $embeddedBuildLabel), @('Trigger', 'cli_autostart'))) {
    if ((Require-Text (Get-RequiredProperty $validation $binding[0] `
                'run.outcome.validation') "run.outcome.validation.$($binding[0])") -cne
        [string]$binding[1]) {
        throw "The recorded artifact-validation $($binding[0]) differs from retained raw evidence."
    }
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
$validation = $derivedValidation

$externalAdvisoryRecords = @($warningRecords | Where-Object {
    $externalRequiredAdvisoryIds -ccontains $_.id
})
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
        'CanonicalConsoleLogCount')) {
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
        'CanonicalConsoleLogCount')) {
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
$diagnosticAxisPassed = $diagnosticBooleans.Valid -and
    $diagnosticBooleans.ChannelArithmeticValid -and
    $diagnosticBooleans.CategoryArithmeticValid -and
    $channelArithmeticDerived -and $categoryArithmeticDerived -and
    $unapprovedKindArithmeticDerived -and $hardDiagnosticFreeDerived -and
    $diagnosticBooleans.LifecycleMarkersValid -and
    $diagnosticBooleans.IdentityBaselinePairValid -and
    $diagnosticBooleans.IntentionalFixtureStructureExact -and
    $diagnosticBooleans.IntentionalFixtureSetValid -and
    $diagnosticBooleans.CanonicalLogPairSameDirectory -and
    $diagnosticIntegers.CrashMarkers -eq 0 -and
    $diagnosticIntegers.PartisanSeverityLineCount -eq 0 -and
    $diagnosticIntegers.MalformedHardDiagnosticCount -eq 0 -and
    $diagnosticIntegers.CanonicalScriptLogCount -eq 1 -and
    $diagnosticIntegers.CanonicalConsoleLogCount -eq 1 -and
    $diagnosticIntegers.UnapprovedHardDiagnosticCount -eq 0 -and
    $classifierChecks -eq 36 -and
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
    throw 'A portable full-profile release index requires zero cleanup and spill residue.'
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
    throw 'A portable full-profile release index requires a complete exact-package wrapper capture.'
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
$proofCommonPassed = $rawCaseCounts.fail -eq 0 -and $rawCaseCounts.blocked -eq 0 -and
    $failedAssertionIds.Count -eq 0 -and $blockedRecords.Count -eq 0 -and
    $unsupportedWarningIds.Count -eq 0 -and
    $unsupportedSkippedIds.Count -eq 0 -and
    $artifactValidationValid -and
    $rawCertificationPassed -and $rawCertificationCounts.fail -eq 0 -and
    $rawCertificationCounts.blocked -eq 0 -and $rawCertificationCounts.warn -eq 0 -and
    $nonzeroStateDiffRowCount -eq 0 -and $finalOrphanCleanupPass -and
    $finalOrphanActiveGroups -eq 0
$acceptedFull = $proofCommonPassed -and $blockedRecords.Count -eq 0 -and
    $warningAssertionIds.Count -eq 0 -and $rawCaseCounts.warn -eq 0 -and
    $guardedRunSucceeded -and
    [string]::IsNullOrWhiteSpace($outcomeError) -and $diagnosticAxisPassed
$acceptedInternal = $false
if (-not $acceptedFull -and $proofCommonPassed -and $externalAdvisoryLinkageValid -and
    $guardedRunSucceeded -and [string]::IsNullOrWhiteSpace($outcomeError) -and
    $diagnosticAxisPassed) {
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

$status = 'failed-full-profile'
$acceptanceDisposition = 'rejected-full-profile'
$releaseDisposition = 'remain-no-go'
$findingStatus = 'release-blocking-red-full-profile'
$findingDefect = 'One or more full-profile acceptance axes failed.'
$findingNextStep = 'Repair every retained proof or diagnostic rejection and seal a new immutable candidate.'
if ($acceptedFull) {
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
$runLeafId = Split-Path -Leaf $runRoot
if ($runLeafId -cnotmatch '^\d{8}T\d{6}Z-[0-9a-f]{32}$') {
    throw 'The retained Campaign Debug run directory does not have a canonical run-leaf ID.'
}
$runEnvelopeSha = (Get-FileHash -LiteralPath $runItem.FullName -Algorithm SHA256).Hash.ToLowerInvariant()

$index = [ordered]@{
    schemaVersion = 2
    evidenceKind = 'packaged-campaign-debug-full-profile'
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
        profile = 'full_certification'
        proofScope = 'full_certification'
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
        stateDiffRows = $stateDiffRowCount
        nonzeroStateDiffRows = $nonzeroStateDiffRowCount
        failedAssertionIds = $failedAssertionIds.ToArray()
        warningAssertionIds = $warningAssertionIds.ToArray()
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
        canonicalLogPairSameDirectory = $diagnosticBooleans.CanonicalLogPairSameDirectory
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

Assert-NoLocalAbsolutePathValue $index 'The portable release index'
Write-PortableJson -Path $outputFullPath -Value $index
$indexSha = (Get-FileHash -LiteralPath $outputFullPath -Algorithm SHA256).Hash.ToLowerInvariant()
[pscustomobject][ordered]@{
    Status = $status
    AcceptanceDisposition = $acceptanceDisposition
    RunEnvelopeSha256 = $runEnvelopeSha
    ReleaseIndexPath = 'release-index.json'
    ReleaseIndexSha256 = $indexSha
    ExternalRequiredAdvisoryIds = @($externalAdvisoryIds)
}
