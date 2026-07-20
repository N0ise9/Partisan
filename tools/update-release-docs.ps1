[CmdletBinding()]
param(
	[switch] $Check,
	[switch] $SelfTest,
	[switch] $LibraryOnly,
	[string] $EvidenceBundleRoot = $env:PARTISAN_RELEASE_EVIDENCE_ROOT
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$statusDataPath = Join-Path $root "docs/data/release_status.json"
$parityDataPath = Join-Path $root "docs/data/antistasi_ce311_parity.json"
$currentStatusPath = Join-Path $root "docs/CURRENT_STATUS.md"
$parityMatrixPath = Join-Path $root "docs/ANTISTASI_CE311_PARITY_MATRIX.md"

function Read-JsonFile {
	param([string] $Path)

	if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
		throw "Required release-spec data file is missing: $Path"
	}

	try {
		return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
	}
	catch {
		throw "Release-spec data is not valid JSON: $Path`n$($_.Exception.Message)"
	}
}

function Require-Text {
	param(
		[object] $Value,
		[string] $Label
	)

	if ($null -eq $Value -or [string]::IsNullOrWhiteSpace([string] $Value)) {
		throw "$Label must be a non-empty string."
	}

	return [string] $Value
}

function Require-Count {
	param(
		[object[]] $Values,
		[int] $Minimum,
		[string] $Label
	)

	if (@($Values).Count -lt $Minimum) {
		throw "$Label must contain at least $Minimum item(s)."
	}
}

function Require-Sha256 {
	param(
		[object] $Value,
		[string] $Label
	)

	$text = Require-Text $Value $Label
	if ($text -cnotmatch '^[0-9a-fA-F]{64}$') {
		throw "$Label must be a 64-character SHA-256 value."
	}

	return $text
}

function Resolve-CampaignRunnerPolicy {
	param(
		[object] $RunnerSha256,
		[string] $Label
	)

	$runnerSha = (Require-Sha256 $RunnerSha256 $Label).ToLowerInvariant()
	# Immutable evidence captured before the exact 13-fault boundary repair keeps
	# its original classifier count. A new candidate must use the current guarded
	# runner and its expanded admission/corruption/watchdog boundary matrix.
	if ($runnerSha -ceq
		'56de386cbdb5677f4315c99e690eefa7158cc6e00e043ded221cec04bde74479') {
		return [PSCustomObject] @{
			ClassifierSelfTestCount = 33
			AllowsGenericFullProfileOutcome = $false
			IntentionalAdmissionProven = $null
			IntentionalSettlementProven = $null
			IntentionalCorruptionProven = $null
			IntentionalWatchdogProven = $null
			ApprovedStockDiagnosticCount = $null
			ApprovedIntentionalDiagnosticCount = $null
			HardDiagnosticCount = $null
			ScriptErrors = $null
			EngineErrors = $null
			PartisanErrors = $null
			ExternalRequiredBlockerIds = @()
		}
	}

	throw "$Label is not a legacy immutable runner. New full-profile evidence must use the portable release-index contract and immutable harness-blob validation."
}

function Resolve-CampaignClassifierSelfTestCount {
	param(
		[object] $RunnerSha256,
		[string] $Label
	)

	$policy = Resolve-CampaignRunnerPolicy $RunnerSha256 $Label
	return [int] $policy.ClassifierSelfTestCount
}

function Require-UtcTimestamp {
	param(
		[object] $Value,
		[string] $Label
	)

	$text = Require-Text $Value $Label
	if ($text -cnotmatch '^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d{1,7})?Z$') {
		throw "$Label must be an ISO-8601 UTC timestamp ending in Z."
	}

	try {
		return [DateTimeOffset]::Parse(
			$text,
			[Globalization.CultureInfo]::InvariantCulture,
			[Globalization.DateTimeStyles]::AssumeUniversal -bor
				[Globalization.DateTimeStyles]::AdjustToUniversal)
	}
	catch {
		throw "$Label is not a valid UTC timestamp."
	}
}

function Require-RepoRelativePath {
	param(
		[object] $Value,
		[string] $Label
	)

	$text = Require-Text $Value $Label
	if ($text -match '^[\\/]' -or
		$text.Contains(":") -or
		$text.Contains("\\") -or
		$text.Contains("//") -or
		$text -match '(^|/)\.\.?(/|$)') {
		throw "$Label must be a normalized repo-relative path with no drive, leading slash, backslash, or dot segment."
	}

	return $text
}

function Assert-NoLocalAbsolutePathValue {
	param(
		[object] $Value,
		[string] $Label
	)

	if ($null -eq $Value) {
		return
	}
	if ($Value -is [string]) {
		$textValue = [string] $Value
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
			Assert-NoLocalAbsolutePathValue ([string] $entry.Key) "$Label property name"
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

function Assert-UniqueStrings {
	param(
		[string[]] $Values,
		[string] $Label
	)

	$duplicates = @($Values | Group-Object | Where-Object { $_.Count -gt 1 })
	if ($duplicates.Count -gt 0) {
		throw "$Label contains duplicate value(s): $(($duplicates | ForEach-Object { $_.Name }) -join ', ')"
	}
}

function Assert-EqualSet {
	param(
		[string[]] $Expected,
		[string[]] $Actual,
		[string] $Label
	)

	$diff = @(Compare-Object `
		@($Expected | Sort-Object -CaseSensitive) `
		@($Actual | Sort-Object -CaseSensitive) `
		-CaseSensitive)
	if ($diff.Count -gt 0) {
		throw "$Label differs from the source inventory:`n$($diff | Out-String)"
	}
}

function Get-RequiredMatch {
	param(
		[string] $Text,
		[string] $Pattern,
		[string] $Group,
		[string] $Label
	)

	$match = [regex]::Match($Text, $Pattern)
	if (-not $match.Success) {
		throw "Could not derive $Label from source."
	}

	return $match.Groups[$Group].Value
}

function Escape-MarkdownCell {
	param([object] $Value)

	if ($null -eq $Value) {
		return ""
	}

	$text = [string] $Value
	$text = $text.Replace("`r", " ").Replace("`n", " ")
	$text = $text.Replace("|", "\|")
	return $text.Trim()
}

function Add-Line {
	param(
		[System.Text.StringBuilder] $Builder,
		[AllowEmptyString()]
		[string] $Line = ""
	)

	[void] $Builder.Append($Line)
	[void] $Builder.Append("`n")
}

function Normalize-GeneratedText {
	param([string] $Text)

	if ($null -eq $Text) {
		return ""
	}

	return $Text.Replace("`r`n", "`n").Replace("`r", "`n")
}

function Publish-GeneratedFile {
	param(
		[string] $Path,
		[string] $Content,
		[bool] $CheckOnly
	)

	$normalized = Normalize-GeneratedText $Content
	if (-not $normalized.EndsWith("`n")) {
		$normalized += "`n"
	}

	if ($CheckOnly) {
		if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
			throw "Generated release document is missing: $Path"
		}

		$existing = Normalize-GeneratedText (Get-Content -Raw -LiteralPath $Path)
		if ($existing -cne $normalized) {
			throw "Generated release document is stale: $Path. Run tools/update-release-docs.ps1."
		}

		return
	}

	$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
	[System.IO.File]::WriteAllText($Path, $normalized, $utf8NoBom)
}

function Get-MissionField {
	param(
		[string] $Body,
		[string] $Name,
		[string] $Default = ""
	)

	$match = [regex]::Match($Body, [regex]::Escape($Name) + '\s+"(?<value>[^"]*)"')
	if ($match.Success) {
		return $match.Groups["value"].Value
	}

	return $Default
}

function Get-MissionIntField {
	param(
		[string] $Body,
		[string] $Name,
		[int] $Default = 0
	)

	$match = [regex]::Match($Body, [regex]::Escape($Name) + '\s+(?<value>-?\d+)')
	if ($match.Success) {
		return [int] $match.Groups["value"].Value
	}

	return $Default
}

function Get-ObjectPropertyValue {
	param(
		[object] $Object,
		[string] $Name
	)

	if ($null -eq $Object) {
		return $null
	}

	$property = $Object.PSObject.Properties[$Name]
	if ($null -eq $property) {
		return $null
	}

	return $property.Value
}

function Get-RetainedCandidateIdentity {
	param(
		[object] $Candidate,
		[string] $Label
	)

	if ($null -eq $Candidate) {
		throw "$Label is missing."
	}

	$candidateIdValue = Require-Text `
		(Get-ObjectPropertyValue $Candidate "candidateId") "$Label.candidateId"
	if ($candidateIdValue -cnotmatch '^[A-Za-z0-9][A-Za-z0-9._-]*$') {
		throw "$Label.candidateId contains unsupported characters."
	}
	$candidateSourceValue = Require-Text `
		(Get-ObjectPropertyValue $Candidate "candidateSourceHead") "$Label.candidateSourceHead"
	if ($candidateSourceValue -cnotmatch '^[0-9a-f]{40}$') {
		throw "$Label.candidateSourceHead must be a lowercase full Git SHA."
	}
	$manifestPathValue = Require-RepoRelativePath `
		(Get-ObjectPropertyValue $Candidate "manifestPath") "$Label.manifestPath"
	$manifestShaValue = (Require-Sha256 `
		(Get-ObjectPropertyValue $Candidate "manifestSha256") "$Label.manifestSha256").ToLowerInvariant()
	$readyShaValue = (Require-Sha256 `
		(Get-ObjectPropertyValue $Candidate "readySha256") "$Label.readySha256").ToLowerInvariant()
	$hashAlgorithmValue = Require-Text `
		(Get-ObjectPropertyValue $Candidate "packageHashAlgorithm") "$Label.packageHashAlgorithm"
	if ($hashAlgorithmValue -cne "sha256-manifest-v1") {
		throw "$Label.packageHashAlgorithm must be sha256-manifest-v1."
	}
	$packageShaValue = (Require-Sha256 `
		(Get-ObjectPropertyValue $Candidate "packageSha256") "$Label.packageSha256").ToLowerInvariant()
	$packageVersionValue = Require-Text `
		(Get-ObjectPropertyValue $Candidate "packageVersion") "$Label.packageVersion"
	$workbenchCrcValue = Require-Text `
		(Get-ObjectPropertyValue $Candidate "workbenchCrc") "$Label.workbenchCrc"
	if ($workbenchCrcValue -cnotmatch '^[0-9a-f]{8}$') {
		throw "$Label.workbenchCrc must be a lowercase 8-character CRC32 value."
	}

	$repositoryPrefixValue = [IO.Path]::GetFullPath($root).TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
	$manifestFullPathValue = [IO.Path]::GetFullPath((Join-Path $root $manifestPathValue))
	if (-not $manifestFullPathValue.StartsWith(
		$repositoryPrefixValue,
		[StringComparison]::OrdinalIgnoreCase) -or
		-not (Test-Path -LiteralPath $manifestFullPathValue -PathType Leaf)) {
		throw "$Label manifest is missing or outside the repository."
	}
	$manifestItemValue = Get-Item -LiteralPath $manifestFullPathValue -Force
	if (($manifestItemValue.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
		throw "$Label manifest must not be a reparse point."
	}
	$actualManifestShaValue = (Get-FileHash `
		-LiteralPath $manifestFullPathValue -Algorithm SHA256).Hash.ToLowerInvariant()
	if ($actualManifestShaValue -cne $manifestShaValue) {
		throw "$Label manifest SHA-256 does not match its retained identity."
	}
	$manifestTextValue = Get-Content -Raw -LiteralPath $manifestFullPathValue
	if ($manifestTextValue -match '(?i)[A-Z]:[\\/]') {
		throw "$Label manifest contains a local absolute path."
	}
	$manifestValue = $manifestTextValue | ConvertFrom-Json
	$manifestCandidateValue = Get-ObjectPropertyValue $manifestValue "candidate"
	$manifestSourceValue = Get-ObjectPropertyValue $manifestValue "source"
	$manifestWorkbenchValue = Get-ObjectPropertyValue $manifestValue "workbench"
	$manifestPackageValue = Get-ObjectPropertyValue $manifestValue "package"
	$createdUtcValue = Require-UtcTimestamp `
		(Get-ObjectPropertyValue $manifestValue "createdUtc") "$Label manifest.createdUtc"
	$campaignSchemaValue = Require-IntegerProperty `
		$manifestSourceValue "campaignSchema" "$Label manifest.source.campaignSchema"
	$runtimeSettingsSchemaValue = Require-IntegerProperty `
		$manifestSourceValue "runtimeSettingsSchema" `
		"$Label manifest.source.runtimeSettingsSchema"
	if ([int] (Get-ObjectPropertyValue $manifestValue "manifestSchemaVersion") -ne 1 -or
		$null -eq $manifestCandidateValue -or
		$null -eq $manifestSourceValue -or
		$null -eq $manifestWorkbenchValue -or
		$null -eq $manifestPackageValue -or
		[string] (Get-ObjectPropertyValue $manifestCandidateValue "id") -cne $candidateIdValue -or
		[string] (Get-ObjectPropertyValue $manifestCandidateValue "version") -cne $packageVersionValue -or
		[string] (Get-ObjectPropertyValue $manifestSourceValue "gitHead") -cne $candidateSourceValue -or
		[string] (Get-ObjectPropertyValue $manifestWorkbenchValue "crc") -cne $workbenchCrcValue -or
		[string] (Get-ObjectPropertyValue $manifestPackageValue "hashAlgorithm") -cne $hashAlgorithmValue -or
		[string] (Get-ObjectPropertyValue $manifestPackageValue "sha256") -cne $packageShaValue) {
		throw "$Label differs from its retained manifest."
	}

	$readyFullPathValue = Join-Path (Split-Path -Parent $manifestFullPathValue) "candidate.ready.json"
	if (-not (Test-Path -LiteralPath $readyFullPathValue -PathType Leaf)) {
		throw "$Label is missing its retained ready seal."
	}
	$readyItemValue = Get-Item -LiteralPath $readyFullPathValue -Force
	if (($readyItemValue.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
		throw "$Label ready seal must not be a reparse point."
	}
	$actualReadyShaValue = (Get-FileHash `
		-LiteralPath $readyFullPathValue -Algorithm SHA256).Hash.ToLowerInvariant()
	if ($actualReadyShaValue -cne $readyShaValue) {
		throw "$Label ready-seal SHA-256 does not match its retained identity."
	}
	$readyValue = Get-Content -Raw -LiteralPath $readyFullPathValue | ConvertFrom-Json
	if ([int] (Get-ObjectPropertyValue $readyValue "schemaVersion") -ne 1 -or
		[string] (Get-ObjectPropertyValue $readyValue "candidateId") -cne $candidateIdValue -or
		[string] (Get-ObjectPropertyValue $readyValue "gitHead") -cne $candidateSourceValue -or
		[string] (Get-ObjectPropertyValue $readyValue "packageSha256") -cne $packageShaValue -or
		[string] (Get-ObjectPropertyValue $readyValue "manifestSha256") -cne $manifestShaValue) {
		throw "$Label ready seal differs from its retained identity."
	}

	return [PSCustomObject] @{
		CandidateId = $candidateIdValue
		CandidateSourceHead = $candidateSourceValue
		ManifestPath = $manifestPathValue
		ManifestSha256 = $manifestShaValue
		ReadySha256 = $readyShaValue
		PackageHashAlgorithm = $hashAlgorithmValue
		PackageSha256 = $packageShaValue
		PackageVersion = $packageVersionValue
		WorkbenchCrc = $workbenchCrcValue
		CreatedUtc = $createdUtcValue
		CampaignSchema = $campaignSchemaValue
		RuntimeSettingsSchema = $runtimeSettingsSchemaValue
		Manifest = $manifestValue
	}
}

function Require-IntegerProperty {
	param(
		[object] $Object,
		[string] $Name,
		[string] $Label
	)

	$value = Get-ObjectPropertyValue $Object $Name
	if ($null -eq $value) {
		throw "$Label must be explicitly present as an integer."
	}
	$typeCode = [Type]::GetTypeCode($value.GetType())
	if ($typeCode -notin @(
		[TypeCode]::SByte,
		[TypeCode]::Byte,
		[TypeCode]::Int16,
		[TypeCode]::UInt16,
		[TypeCode]::Int32,
		[TypeCode]::UInt32,
		[TypeCode]::Int64,
		[TypeCode]::UInt64)) {
		throw "$Label must be an integer, not $($value.GetType().Name)."
	}

	return [long] $value
}

function Require-IntegerScalar {
	param(
		[object] $Value,
		[string] $Label
	)

	if ($Value -is [string]) {
		$parsed = [long] 0
		if ([string] $Value -cnotmatch '^-?\d+$' -or
			-not [long]::TryParse([string] $Value, [ref] $parsed)) {
			throw "$Label must be an integer scalar."
		}
		return $parsed
	}
	$typeCode = if ($null -eq $Value) {
		[TypeCode]::Empty
	}
	else {
		[Type]::GetTypeCode($Value.GetType())
	}
	if ($typeCode -notin @(
			[TypeCode]::SByte, [TypeCode]::Byte, [TypeCode]::Int16,
			[TypeCode]::UInt16, [TypeCode]::Int32, [TypeCode]::UInt32,
			[TypeCode]::Int64, [TypeCode]::UInt64)) {
		throw "$Label must be an integer scalar."
	}
	return [long] $Value
}

function Assert-ExecutableIdentityEqual {
	param(
		[object] $Expected,
		[object] $Actual,
		[string] $Label
	)

	$expectedFileName = Require-Text `
		(Get-ObjectPropertyValue $Expected "fileName") "$Label expected fileName"
	$actualFileName = Require-Text `
		(Get-ObjectPropertyValue $Actual "fileName") "$Label actual fileName"
	$expectedFileVersion = Require-Text `
		(Get-ObjectPropertyValue $Expected "fileVersion") "$Label expected fileVersion"
	$actualFileVersion = Require-Text `
		(Get-ObjectPropertyValue $Actual "fileVersion") "$Label actual fileVersion"
	$expectedProductVersion = Require-Text `
		(Get-ObjectPropertyValue $Expected "productVersion") "$Label expected productVersion"
	$actualProductVersion = Require-Text `
		(Get-ObjectPropertyValue $Actual "productVersion") "$Label actual productVersion"
	$expectedLength = Require-IntegerProperty $Expected "length" "$Label expected length"
	$actualLength = Require-IntegerProperty $Actual "length" "$Label actual length"
	$expectedSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Expected "sha256") "$Label expected sha256").ToLowerInvariant()
	$actualSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Actual "sha256") "$Label actual sha256").ToLowerInvariant()
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
		[string] $RunnerSourceText,
		[string] $JsonPath,
		[string] $SummaryPath,
		[string] $StateDiffPath,
		[string] $GuardRoot,
		[string] $ExpectedSha,
		[string] $ExpectedUtc,
		[string] $ExpectedLabel
	)

	$tokens = $null
	$parseErrors = $null
	$runnerAst = [Management.Automation.Language.Parser]::ParseInput(
		$RunnerSourceText,
		[ref] $tokens,
		[ref] $parseErrors)
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
		[void] $functionSource.Add($matches[0].Extent.Text)
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

function Assert-IntegerProperties {
	param(
		[object] $Object,
		[string[]] $Names,
		[string] $Label
	)

	foreach ($name in $Names) {
		Require-IntegerProperty $Object $name "$Label.$name" | Out-Null
	}
}

function Assert-PackagedFocusedEvidence {
	param(
		[object] $Evidence,
		[object] $CandidateIdentity,
		[string] $ExpectedStatus,
		[string] $Label
	)

	if ($null -eq $Evidence) {
		throw "$Label is missing."
	}
	if ([string] (Get-ObjectPropertyValue $Evidence "status") -cne $ExpectedStatus) {
		throw "$Label.status must be $ExpectedStatus."
	}

	$summaryPath = Require-RepoRelativePath `
		(Get-ObjectPropertyValue $Evidence "summaryPath") `
		"$Label.summaryPath"
	$summarySha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "summarySha256") `
		"$Label.summarySha256").ToLowerInvariant()
	$harnessHead = Require-Text `
		(Get-ObjectPropertyValue $Evidence "harnessGitHead") `
		"$Label.harnessGitHead"
	if ($harnessHead -cnotmatch '^[0-9a-f]{40}$') {
		throw "$Label.harnessGitHead must be a lowercase full Git SHA."
	}

	$repositoryPrefix = [IO.Path]::GetFullPath($root).TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
	$summaryFullPath = [IO.Path]::GetFullPath((Join-Path $root $summaryPath))
	if (-not $summaryFullPath.StartsWith(
			$repositoryPrefix,
			[StringComparison]::OrdinalIgnoreCase) -or
		-not (Test-Path -LiteralPath $summaryFullPath -PathType Leaf)) {
		throw "$Label summary is missing or outside the repository."
	}
	$summaryItem = Get-Item -LiteralPath $summaryFullPath -Force
	if (($summaryItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
		throw "$Label summary must not be a reparse point."
	}
	$actualSummarySha = (Get-FileHash `
		-LiteralPath $summaryFullPath `
		-Algorithm SHA256).Hash.ToLowerInvariant()
	if ($actualSummarySha -cne $summarySha) {
		throw "$Label summary SHA-256 does not match release status."
	}

	$summaryText = Get-Content -Raw -LiteralPath $summaryFullPath
	if ($summaryText -match '(?i)[A-Z]:[\\/]') {
		throw "$Label summary contains a local absolute path."
	}
	$summary = $summaryText | ConvertFrom-Json
	$summaryCandidate = Get-ObjectPropertyValue $summary "candidate"
	$summaryHarness = Get-ObjectPropertyValue $summary "harness"
	$acceptedWindow = Get-ObjectPropertyValue $summary "acceptedWindow"
	$summaryResult = Get-ObjectPropertyValue $summary "result"
	$summaryCases = @(Get-ObjectPropertyValue $summary "cases")
	$summaryPreliminary = Get-ObjectPropertyValue $summary "preliminaryRuns"
	$summarySchemaVersion = Require-IntegerProperty `
		$summary "schemaVersion" "$Label summary.schemaVersion"
	if ($summarySchemaVersion -ne 1 -or
		[string] (Get-ObjectPropertyValue $summary "evidenceKind") -cne
			"packaged-focused-autotest-set" -or
		$null -eq $summaryCandidate -or
		$null -eq $summaryHarness -or
		$null -eq $acceptedWindow -or
		$null -eq $summaryResult -or
		$summaryCases.Count -ne 5) {
		throw "$Label summary is structurally incomplete."
	}

	if ([string] (Get-ObjectPropertyValue $Evidence "candidateId") -cne
			[string] $CandidateIdentity.CandidateId -or
		[string] (Get-ObjectPropertyValue $Evidence "candidateSourceHead") -cne
			[string] $CandidateIdentity.CandidateSourceHead -or
		[string] (Get-ObjectPropertyValue $Evidence "packageSha256") -cne
			[string] $CandidateIdentity.PackageSha256 -or
		[string] (Get-ObjectPropertyValue $Evidence "manifestSha256") -cne
			[string] $CandidateIdentity.ManifestSha256 -or
		[string] (Get-ObjectPropertyValue $Evidence "readySha256") -cne
			[string] $CandidateIdentity.ReadySha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "candidateId") -cne
			[string] $CandidateIdentity.CandidateId -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "candidateSourceHead") -cne
			[string] $CandidateIdentity.CandidateSourceHead -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "packageSha256") -cne
			[string] $CandidateIdentity.PackageSha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "manifestSha256") -cne
			[string] $CandidateIdentity.ManifestSha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "readySha256") -cne
			[string] $CandidateIdentity.ReadySha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "workbenchCrc") -cne
			[string] $CandidateIdentity.WorkbenchCrc) {
		throw "$Label differs from its retained candidate identity."
	}

	$runnerSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "focusedRunnerSha256") `
		"$Label.focusedRunnerSha256").ToLowerInvariant()
	$candidateModuleSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "candidateModuleSha256") `
		"$Label.candidateModuleSha256").ToLowerInvariant()
	if ([string] (Get-ObjectPropertyValue $summaryHarness "gitHead") -cne $harnessHead -or
		(Get-ObjectPropertyValue $summaryHarness "dirty") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryHarness "dirty") -or
		[string] (Get-ObjectPropertyValue $summaryHarness "focusedRunnerSha256") -cne $runnerSha -or
		[string] (Get-ObjectPropertyValue $summaryHarness "candidateModuleSha256") -cne
			$candidateModuleSha) {
		throw "$Label does not bind one clean exact harness."
	}

	$expectedFocusedOrder = @(
		"HST_TEST_EnemyCounterattackAuthority",
		"HST_TEST_EnemyGarrisonRebuildAuthority",
		"HST_TEST_EnemyPlanningCommitmentAuthority",
		"HST_TEST_EnemyQRFAuthority",
		"HST_TEST_CampaignProfileJournalAuthority"
	)
	$expectedFocusedSuites = @{
		"HST_TEST_EnemyCounterattackAuthority" = "HST_EnemyCounterattackAutotestSuite"
		"HST_TEST_EnemyGarrisonRebuildAuthority" = "HST_EnemyGarrisonRebuildAutotestSuite"
		"HST_TEST_EnemyPlanningCommitmentAuthority" = "HST_EnemyPlanningCommitmentAutotestSuite"
		"HST_TEST_EnemyQRFAuthority" = "HST_EnemyQRFAutotestSuite"
		"HST_TEST_CampaignProfileJournalAuthority" = "HST_CampaignProfileJournalAuthorityAutotestSuite"
	}
	$focusedCaseNames = @($summaryCases | ForEach-Object {
		[string] (Get-ObjectPropertyValue $_ "testCase")
	})
	Assert-UniqueStrings $focusedCaseNames "$Label testcase IDs"
	Assert-EqualSet $expectedFocusedOrder $focusedCaseNames "$Label testcase IDs"
	for ($caseIndex = 0; $caseIndex -lt $expectedFocusedOrder.Count; $caseIndex++) {
		if ($focusedCaseNames[$caseIndex] -cne $expectedFocusedOrder[$caseIndex]) {
			throw "$Label testcases must retain canonical gate order."
		}
	}

	Assert-IntegerProperties $Evidence @(
		"caseCount", "passedCases", "junitTests", "junitFailures", "junitErrors",
		"junitSkipped", "hardDiagnosticClassifierChecksPerRun", "hardDiagnosticCount",
		"approvedStockFilterDiagnosticCount", "approvedIntentionalFaultDiagnosticCount",
		"unapprovedHardDiagnosticCount", "envelopeFileCount"
	) $Label
	Assert-IntegerProperties $summaryResult @(
		"caseCount", "passedCases", "junitTests", "junitFailures", "junitErrors",
		"junitSkipped", "envelopeFileCount", "hardDiagnosticClassifierChecksPerRun",
		"hardDiagnosticCount", "approvedStockFilterDiagnosticCount",
		"approvedIntentionalFaultDiagnosticCount", "unapprovedHardDiagnosticCount"
	) "$Label summary.result"
	$classifierChecksPerRun = [int] (Get-ObjectPropertyValue `
		$Evidence "hardDiagnosticClassifierChecksPerRun")
	if ($classifierChecksPerRun -le 0) {
		throw "$Label must record a positive classifier self-check count."
	}

	$acceptedStartedText = Require-Text `
		(Get-ObjectPropertyValue $acceptedWindow "startedUtc") `
		"$Label summary.acceptedWindow.startedUtc"
	$acceptedCompletedText = Require-Text `
		(Get-ObjectPropertyValue $acceptedWindow "completedUtc") `
		"$Label summary.acceptedWindow.completedUtc"
	$acceptedStarted = Require-UtcTimestamp $acceptedStartedText `
		"$Label summary.acceptedWindow.startedUtc"
	$acceptedCompleted = Require-UtcTimestamp $acceptedCompletedText `
		"$Label summary.acceptedWindow.completedUtc"
	if ($acceptedStarted -ge $acceptedCompleted) {
		throw "$Label accepted window must have positive duration."
	}

	$runIds = @()
	$envelopeHashes = @()
	$junitTests = 0
	$junitFailures = 0
	$junitErrors = 0
	$junitSkipped = 0
	$hardDiagnostics = 0
	$stockDiagnostics = 0
	$intentionalDiagnostics = 0
	$unapprovedDiagnostics = 0
	$envelopeFileCount = 0
	$previousCompleted = $null
	for ($caseIndex = 0; $caseIndex -lt $summaryCases.Count; $caseIndex++) {
		$focusedCase = $summaryCases[$caseIndex]
		$focusedCaseName = Require-Text `
			(Get-ObjectPropertyValue $focusedCase "testCase") `
			"$Label testcase ID"
		$runId = Require-Text `
			(Get-ObjectPropertyValue $focusedCase "runId") `
			"$Label run ID"
		$envelopeSha = (Require-Sha256 `
			(Get-ObjectPropertyValue $focusedCase "envelopeSha256") `
			"$Label envelope SHA-256").ToLowerInvariant()
		Assert-IntegerProperties $focusedCase @(
			"fileCount", "junitTests", "junitFailures", "junitErrors", "junitSkipped",
			"hardDiagnosticClassifierChecks", "hardDiagnosticCount",
			"approvedStockFilterDiagnosticCount", "approvedIntentionalFaultDiagnosticCount",
			"unapprovedHardDiagnosticCount"
		) "$Label testcase $focusedCaseName"
		$caseHardDiagnostics = [int] (Get-ObjectPropertyValue $focusedCase "hardDiagnosticCount")
		$caseStockDiagnostics = [int] (Get-ObjectPropertyValue $focusedCase "approvedStockFilterDiagnosticCount")
		$caseIntentionalDiagnostics = [int] (Get-ObjectPropertyValue $focusedCase "approvedIntentionalFaultDiagnosticCount")
		$caseUnapprovedDiagnostics = [int] (Get-ObjectPropertyValue $focusedCase "unapprovedHardDiagnosticCount")
		$caseHardDiagnosticFree = Get-ObjectPropertyValue $focusedCase "hardDiagnosticFree"
		if ([string] (Get-ObjectPropertyValue $focusedCase "suiteClass") -cne
				[string] $expectedFocusedSuites[$focusedCaseName] -or
			$runId -cnotmatch '^\d{8}T\d{6}Z-[0-9a-f]{32}$' -or
			(Get-ObjectPropertyValue $focusedCase "success") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $focusedCase "success") -or
			(Get-ObjectPropertyValue $focusedCase "candidateBoundaryVerified") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $focusedCase "candidateBoundaryVerified") -or
			(Get-ObjectPropertyValue $focusedCase "mountPacked") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $focusedCase "mountPacked") -or
			[int] (Get-ObjectPropertyValue $focusedCase "fileCount") -le 0 -or
			[int] (Get-ObjectPropertyValue $focusedCase "junitTests") -ne 1 -or
			[int] (Get-ObjectPropertyValue $focusedCase "junitFailures") -ne 0 -or
			[int] (Get-ObjectPropertyValue $focusedCase "junitErrors") -ne 0 -or
			[int] (Get-ObjectPropertyValue $focusedCase "junitSkipped") -ne 0 -or
			[int] (Get-ObjectPropertyValue $focusedCase "hardDiagnosticClassifierChecks") -ne
				$classifierChecksPerRun -or
			(Get-ObjectPropertyValue $focusedCase "hardDiagnosticClassificationValid") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $focusedCase "hardDiagnosticClassificationValid") -or
			$caseHardDiagnosticFree -isnot [bool] -or
			[bool] $caseHardDiagnosticFree -ne ($caseHardDiagnostics -eq 0) -or
			$caseHardDiagnostics -lt 0 -or
			$caseStockDiagnostics -lt 0 -or
			$caseIntentionalDiagnostics -lt 0 -or
			$caseUnapprovedDiagnostics -ne 0 -or
			$caseHardDiagnostics -ne
				($caseStockDiagnostics + $caseIntentionalDiagnostics + $caseUnapprovedDiagnostics) -or
			(Get-ObjectPropertyValue $focusedCase "cleanupAndSpillZero") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $focusedCase "cleanupAndSpillZero") -or
			(Get-ObjectPropertyValue $focusedCase "envelopeFilesRehashed") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $focusedCase "envelopeFilesRehashed")) {
			throw "$Label testcase $focusedCaseName does not satisfy the accepted result contract."
		}

		$caseStartedText = Require-Text `
			(Get-ObjectPropertyValue $focusedCase "startedUtc") "$Label testcase started UTC"
		$caseCompletedText = Require-Text `
			(Get-ObjectPropertyValue $focusedCase "completedUtc") "$Label testcase completed UTC"
		$caseStarted = Require-UtcTimestamp $caseStartedText "$Label testcase started UTC"
		$caseCompleted = Require-UtcTimestamp $caseCompletedText "$Label testcase completed UTC"
		if ($caseStarted -ge $caseCompleted -or
			($null -ne $previousCompleted -and $caseStarted -lt $previousCompleted)) {
			throw "$Label testcase windows must be positive, ordered, and non-overlapping."
		}
		if ($caseIndex -eq 0 -and $caseStartedText -cne $acceptedStartedText) {
			throw "$Label accepted window must start with its first testcase."
		}
		if ($caseIndex -eq $summaryCases.Count - 1 -and
			$caseCompletedText -cne $acceptedCompletedText) {
			throw "$Label accepted window must end with its final testcase."
		}
		$previousCompleted = $caseCompleted
		$runIds += $runId
		$envelopeHashes += $envelopeSha
		$junitTests += [int] (Get-ObjectPropertyValue $focusedCase "junitTests")
		$junitFailures += [int] (Get-ObjectPropertyValue $focusedCase "junitFailures")
		$junitErrors += [int] (Get-ObjectPropertyValue $focusedCase "junitErrors")
		$junitSkipped += [int] (Get-ObjectPropertyValue $focusedCase "junitSkipped")
		$hardDiagnostics += $caseHardDiagnostics
		$stockDiagnostics += $caseStockDiagnostics
		$intentionalDiagnostics += $caseIntentionalDiagnostics
		$unapprovedDiagnostics += $caseUnapprovedDiagnostics
		$envelopeFileCount += [int] (Get-ObjectPropertyValue $focusedCase "fileCount")
	}
	Assert-UniqueStrings $runIds "$Label run IDs"
	Assert-UniqueStrings $envelopeHashes "$Label envelope hashes"

	if ([string] (Get-ObjectPropertyValue $summaryResult "status") -cne "passed-noncertifying" -or
		[int] (Get-ObjectPropertyValue $summaryResult "caseCount") -ne 5 -or
		[int] (Get-ObjectPropertyValue $summaryResult "passedCases") -ne 5 -or
		[int] (Get-ObjectPropertyValue $summaryResult "junitTests") -ne $junitTests -or
		[int] (Get-ObjectPropertyValue $summaryResult "junitFailures") -ne $junitFailures -or
		[int] (Get-ObjectPropertyValue $summaryResult "junitErrors") -ne $junitErrors -or
		[int] (Get-ObjectPropertyValue $summaryResult "junitSkipped") -ne $junitSkipped -or
		[int] (Get-ObjectPropertyValue $summaryResult "hardDiagnosticClassifierChecksPerRun") -ne
			$classifierChecksPerRun -or
		(Get-ObjectPropertyValue $summaryResult "hardDiagnosticClassificationValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryResult "hardDiagnosticClassificationValid") -or
		(Get-ObjectPropertyValue $summaryResult "hardDiagnosticFree") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryResult "hardDiagnosticFree") -ne
			($hardDiagnostics -eq 0) -or
		[int] (Get-ObjectPropertyValue $summaryResult "hardDiagnosticCount") -ne $hardDiagnostics -or
		[int] (Get-ObjectPropertyValue $summaryResult "approvedStockFilterDiagnosticCount") -ne
			$stockDiagnostics -or
		[int] (Get-ObjectPropertyValue $summaryResult "approvedIntentionalFaultDiagnosticCount") -ne
			$intentionalDiagnostics -or
		[int] (Get-ObjectPropertyValue $summaryResult "unapprovedHardDiagnosticCount") -ne
			$unapprovedDiagnostics -or
		[int] (Get-ObjectPropertyValue $summaryResult "envelopeFileCount") -ne $envelopeFileCount -or
		(Get-ObjectPropertyValue $summaryResult "candidateBoundaryVerified") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryResult "candidateBoundaryVerified") -or
		(Get-ObjectPropertyValue $summaryResult "allMountsPacked") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryResult "allMountsPacked") -or
		(Get-ObjectPropertyValue $summaryResult "allCleanupAndSpillZero") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryResult "allCleanupAndSpillZero") -or
		(Get-ObjectPropertyValue $summaryResult "allEnvelopeFilesRehashed") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryResult "allEnvelopeFilesRehashed")) {
		throw "$Label aggregate does not equal its five accepted runs."
	}
	Require-Text (Get-ObjectPropertyValue $summaryResult "scope") "$Label summary.result.scope" | Out-Null

	$preliminaryStatus = $null
	$preliminaryCaseCount = 0
	if ($null -ne $summaryPreliminary) {
		$preliminaryCaseCount = Require-IntegerProperty `
			$summaryPreliminary "caseCount" "$Label summary.preliminaryRuns.caseCount"
		$preliminaryStatus = Require-Text `
			(Get-ObjectPropertyValue $summaryPreliminary "status") `
			"$Label summary.preliminaryRuns.status"
		Require-Text `
			(Get-ObjectPropertyValue $summaryPreliminary "note") `
			"$Label summary.preliminaryRuns.note" | Out-Null
		if (($preliminaryStatus -ceq "none" -and $preliminaryCaseCount -ne 0) -or
			($preliminaryStatus -ceq "superseded-for-acceptance" -and $preliminaryCaseCount -le 0) -or
			$preliminaryStatus -cnotin @("none", "superseded-for-acceptance")) {
			throw "$Label preliminary-run status and count are inconsistent."
		}
	}

	if ([int] (Get-ObjectPropertyValue $Evidence "caseCount") -ne 5 -or
		[int] (Get-ObjectPropertyValue $Evidence "passedCases") -ne 5 -or
		[int] (Get-ObjectPropertyValue $Evidence "junitTests") -ne $junitTests -or
		[int] (Get-ObjectPropertyValue $Evidence "junitFailures") -ne $junitFailures -or
		[int] (Get-ObjectPropertyValue $Evidence "junitErrors") -ne $junitErrors -or
		[int] (Get-ObjectPropertyValue $Evidence "junitSkipped") -ne $junitSkipped -or
		[int] (Get-ObjectPropertyValue $Evidence "hardDiagnosticClassifierChecksPerRun") -ne
			$classifierChecksPerRun -or
		(Get-ObjectPropertyValue $Evidence "hardDiagnosticFree") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $Evidence "hardDiagnosticFree") -ne
			($hardDiagnostics -eq 0) -or
		[int] (Get-ObjectPropertyValue $Evidence "hardDiagnosticCount") -ne $hardDiagnostics -or
		[int] (Get-ObjectPropertyValue $Evidence "approvedStockFilterDiagnosticCount") -ne
			$stockDiagnostics -or
		[int] (Get-ObjectPropertyValue $Evidence "approvedIntentionalFaultDiagnosticCount") -ne
			$intentionalDiagnostics -or
		[int] (Get-ObjectPropertyValue $Evidence "unapprovedHardDiagnosticCount") -ne
			$unapprovedDiagnostics -or
		[int] (Get-ObjectPropertyValue $Evidence "envelopeFileCount") -ne $envelopeFileCount -or
		(Get-ObjectPropertyValue $Evidence "cleanupAndSpillZero") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $Evidence "cleanupAndSpillZero") -or
		$junitTests -ne 5 -or $junitFailures -ne 0 -or $junitErrors -ne 0 -or
		$junitSkipped -ne 0 -or $unapprovedDiagnostics -ne 0 -or $envelopeFileCount -le 0) {
		throw "$Label release status does not equal its tracked summary."
	}
	Require-Text (Get-ObjectPropertyValue $Evidence "summary") "$Label.summary" | Out-Null

	return [PSCustomObject] @{
		SummaryPath = $summaryPath
		SummarySha256 = $summarySha
		HarnessGitHead = $harnessHead
		AcceptedStartedUtc = $acceptedStarted
		AcceptedCompletedUtc = $acceptedCompleted
		PreliminaryStatus = $preliminaryStatus
		PreliminaryCaseCount = $preliminaryCaseCount
		CaseCount = 5
		PassedCases = 5
		JunitTests = $junitTests
		JunitFailures = $junitFailures
		JunitErrors = $junitErrors
		JunitSkipped = $junitSkipped
		HardDiagnosticCount = $hardDiagnostics
		ApprovedStockDiagnosticCount = $stockDiagnostics
		ApprovedIntentionalDiagnosticCount = $intentionalDiagnostics
		UnapprovedHardDiagnosticCount = $unapprovedDiagnostics
		EnvelopeFileCount = $envelopeFileCount
		CaseRunIds = @($runIds)
		CaseEnvelopeSha256s = @($envelopeHashes)
	}
}

function Assert-CorrectedCanaryEvidence {
	param(
		[object] $Evidence,
		[object] $CandidateIdentity,
		[string] $ExpectedStatus,
		[ValidateSet("accepted", "rejected", "rejected-proof")]
		[string] $ExpectedOutcome,
		[string] $Label,
		[DateTimeOffset] $StatusAsOfUtc,
		[int] $SourceSettingsSchema
	)

	if ($null -eq $Evidence) {
		throw "$Label is missing."
	}
	if ([string] (Get-ObjectPropertyValue $Evidence "status") -cne $ExpectedStatus) {
		throw "$Label.status must be $ExpectedStatus."
	}

	$summaryPath = Require-RepoRelativePath `
		(Get-ObjectPropertyValue $Evidence "summaryPath") "$Label.summaryPath"
	$summarySha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "summarySha256") "$Label.summarySha256").ToLowerInvariant()
	$harnessHead = Require-Text `
		(Get-ObjectPropertyValue $Evidence "harnessGitHead") "$Label.harnessGitHead"
	if ($harnessHead -cnotmatch '^[0-9a-f]{40}$') {
		throw "$Label.harnessGitHead must be a lowercase full Git SHA."
	}

	$repositoryPrefix = [IO.Path]::GetFullPath($root).TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
	$summaryFullPath = [IO.Path]::GetFullPath((Join-Path $root $summaryPath))
	if (-not $summaryFullPath.StartsWith(
		$repositoryPrefix,
		[StringComparison]::OrdinalIgnoreCase) -or
		-not (Test-Path -LiteralPath $summaryFullPath -PathType Leaf)) {
		throw "$Label summary is missing or outside the repository."
	}
	$summaryItem = Get-Item -LiteralPath $summaryFullPath -Force
	if (($summaryItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
		throw "$Label summary must not be a reparse point."
	}
	$actualSummarySha = (Get-FileHash `
		-LiteralPath $summaryFullPath -Algorithm SHA256).Hash.ToLowerInvariant()
	if ($actualSummarySha -cne $summarySha) {
		throw "$Label summary SHA-256 does not match release status."
	}

	$summaryText = Get-Content -Raw -LiteralPath $summaryFullPath
	if ($summaryText -match '(?i)[A-Z]:[\\/]') {
		throw "$Label summary contains a local absolute path."
	}
	$summary = $summaryText | ConvertFrom-Json
	$summaryCandidate = Get-ObjectPropertyValue $summary "candidate"
	$summaryHarness = Get-ObjectPropertyValue $summary "harness"
	$summarySettings = Get-ObjectPropertyValue $summary "settings"
	$summaryCapture = Get-ObjectPropertyValue $summary "capture"
	$summaryResult = Get-ObjectPropertyValue $summary "result"
	$summaryProof = Get-ObjectPropertyValue $summary "proof"
	$summaryDiagnostics = Get-ObjectPropertyValue $summary "diagnostics"
	$summaryCleanup = Get-ObjectPropertyValue $summary "cleanup"
	$summaryIntegrity = Get-ObjectPropertyValue $summary "integrity"
	$summaryFinding = Get-ObjectPropertyValue $summary "finding"
	$summarySchemaVersion = Require-IntegerProperty `
		$summary "schemaVersion" "$Label summary.schemaVersion"
	if ($summarySchemaVersion -ne 1 -or
		[string] (Get-ObjectPropertyValue $summary "evidenceKind") -cne
			"packaged-campaign-debug-corrected-canary" -or
		$null -eq $summaryCandidate -or $null -eq $summaryHarness -or
		$null -eq $summarySettings -or $null -eq $summaryCapture -or
		$null -eq $summaryResult -or $null -eq $summaryProof -or
		$null -eq $summaryDiagnostics -or $null -eq $summaryCleanup -or
		$null -eq $summaryIntegrity -or $null -eq $summaryFinding) {
		throw "$Label summary is structurally incomplete."
	}

	Assert-IntegerProperties $summarySettings @("schemaVersion") "$Label summary.settings"
	Assert-IntegerProperties $summaryCapture @("runtimeSeconds") "$Label summary.capture"
	Assert-IntegerProperties $summaryProof @(
		"caseCount", "pass", "warn", "fail", "blocked", "skipped",
		"focusedAssertionCount", "focusedAssertionsPassed", "certificationRequired",
		"certificationProven", "certificationFail", "certificationBlocked",
		"stateDiffRows", "nonzeroStateDiffRows", "finalOrphanActiveGroups") `
		"$Label summary.proof"
	Assert-IntegerProperties $summaryDiagnostics @(
		"hardDiagnosticCount", "scriptErrors", "engineErrors", "partisanErrors",
		"crashMarkers", "partisanSeverityLineCount", "approvedStockDiagnosticCount",
		"approvedIntentionalDiagnosticCount", "unapprovedHardDiagnosticCount",
		"classifierSelfTestCount", "malformedHardDiagnosticCount",
		"canonicalScriptLogCount", "canonicalConsoleLogCount") "$Label summary.diagnostics"
	Assert-IntegerProperties $summaryCleanup @(
		"guardRemaining", "ownedProcessesRemaining", "newEngineProcessesRemaining",
		"unclaimedEngineProcessesObserved", "newDefaultEntriesRemaining",
		"modifiedDefaultFiles", "deletedDefaultEntries", "missingDefaultRoots",
		"externalSpillEntriesRemaining", "modifiedSpillFiles", "deletedSpillEntries",
		"missingSpillRoots", "cleanupPhaseErrorCount") "$Label summary.cleanup"
	Assert-IntegerProperties $summaryIntegrity @("envelopeFileCount") "$Label summary.integrity"
	Assert-IntegerProperties $Evidence @(
		"runtimeSeconds", "caseCount", "pass", "warn", "fail", "blocked", "skipped",
		"focusedAssertionCount", "focusedAssertionsPassed", "certificationRequired",
		"certificationProven", "stateDiffRows", "nonzeroStateDiffRows",
		"hardDiagnosticCount", "scriptErrors", "engineErrors", "partisanErrors",
		"approvedStockDiagnosticCount", "approvedIntentionalDiagnosticCount",
		"unapprovedHardDiagnosticCount", "classifierSelfTestCount",
		"canonicalScriptLogCount", "canonicalConsoleLogCount", "envelopeFileCount") $Label

	if ([string] (Get-ObjectPropertyValue $Evidence "candidateId") -cne
			[string] $CandidateIdentity.CandidateId -or
		[string] (Get-ObjectPropertyValue $Evidence "candidateSourceHead") -cne
			[string] $CandidateIdentity.CandidateSourceHead -or
		[string] (Get-ObjectPropertyValue $Evidence "packageSha256") -cne
			[string] $CandidateIdentity.PackageSha256 -or
		[string] (Get-ObjectPropertyValue $Evidence "manifestSha256") -cne
			[string] $CandidateIdentity.ManifestSha256 -or
		[string] (Get-ObjectPropertyValue $Evidence "readySha256") -cne
			[string] $CandidateIdentity.ReadySha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "candidateId") -cne
			[string] $CandidateIdentity.CandidateId -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "candidateSourceHead") -cne
			[string] $CandidateIdentity.CandidateSourceHead -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "packageSha256") -cne
			[string] $CandidateIdentity.PackageSha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "manifestSha256") -cne
			[string] $CandidateIdentity.ManifestSha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "readySha256") -cne
			[string] $CandidateIdentity.ReadySha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "workbenchCrc") -cne
			[string] $CandidateIdentity.WorkbenchCrc -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "runtimeUseDispositionAtCapture") -cne
			"active-runtime-candidate") {
		throw "$Label differs from its retained candidate identity."
	}

	$runnerSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "campaignRunnerSha256") `
		"$Label.campaignRunnerSha256").ToLowerInvariant()
	$expectedClassifierSelfTestCount = Resolve-CampaignClassifierSelfTestCount `
		$runnerSha "$Label.campaignRunnerSha256"
	$candidateModuleSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "candidateModuleSha256") `
		"$Label.candidateModuleSha256").ToLowerInvariant()
	$settingsSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "settingsSha256") `
		"$Label.settingsSha256").ToLowerInvariant()
	if ([string] (Get-ObjectPropertyValue $summaryHarness "gitHead") -cne $harnessHead -or
		(Get-ObjectPropertyValue $summaryHarness "dirty") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryHarness "dirty") -or
		[string] (Get-ObjectPropertyValue $summaryHarness "campaignRunnerSha256") -cne $runnerSha -or
		[string] (Get-ObjectPropertyValue $summaryHarness "candidateModuleSha256") -cne
			$candidateModuleSha -or
		[int] (Get-ObjectPropertyValue $summarySettings "schemaVersion") -ne $SourceSettingsSchema -or
		[string] (Get-ObjectPropertyValue $summarySettings "sha256") -cne $settingsSha -or
		(Get-ObjectPropertyValue $summarySettings "guardedRuntimeCopy") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summarySettings "guardedRuntimeCopy")) {
		throw "$Label harness or settings identity is inconsistent."
	}

	$runLeafId = Require-Text `
		(Get-ObjectPropertyValue $summaryCapture "runLeafId") "$Label run-leaf ID"
	$runId = Require-Text `
		(Get-ObjectPropertyValue $summaryCapture "runId") "$Label run ID"
	$startedUtc = Require-UtcTimestamp `
		(Get-ObjectPropertyValue $summaryCapture "startedUtc") "$Label start time"
	$completedUtc = Require-UtcTimestamp `
		(Get-ObjectPropertyValue $summaryCapture "completedUtc") "$Label completion time"
	$captureRuntimeSeconds = [int] (Get-ObjectPropertyValue $summaryCapture "runtimeSeconds")
	if ($completedUtc -lt $startedUtc -or $StatusAsOfUtc -lt $completedUtc -or
		[string] (Get-ObjectPropertyValue $Evidence "startedUtc") -cne
			[string] (Get-ObjectPropertyValue $summaryCapture "startedUtc") -or
		[string] (Get-ObjectPropertyValue $Evidence "completedUtc") -cne
			[string] (Get-ObjectPropertyValue $summaryCapture "completedUtc") -or
		[string] (Get-ObjectPropertyValue $Evidence "runLeafId") -cne
			$runLeafId -or
		$runLeafId -cnotmatch
			'^\d{8}T\d{6}Z-[0-9a-f]{32}$' -or
		[string] (Get-ObjectPropertyValue $Evidence "runId") -cne
			$runId -or
		$runId -cnotmatch
			'^seed\d+_t\d+_p\d+_u\d+$' -or
		[string] (Get-ObjectPropertyValue $summaryCapture "profile") -cne "force_authority" -or
		[string] (Get-ObjectPropertyValue $summaryCapture "proofScope") -cne
			"focused_force_authority" -or
		$captureRuntimeSeconds -le 0 -or
		[int] (Get-ObjectPropertyValue $Evidence "runtimeSeconds") -ne $captureRuntimeSeconds) {
		throw "$Label capture identity, timestamps, or runtime are inconsistent."
	}

	$expectedSummaryStatus = "passed-noncertifying"
	$expectedSuccess = $true
	$expectedError = ""
	$expectedAcceptanceDisposition = "accepted-noncertifying"
	$expectedReleaseDisposition = "proceed-full-profile"
	$expectedDiagnosticsValid = $true
	$expectedHardCount = 2
	$expectedScriptErrors = 2
	$expectedUnapprovedCount = 0
	$expectedUnapprovedKind = ""
	$expectedFindingStatus = "accepted-noncertifying"
	$expectedFindingDefect = "none"
	$expectedArtifactValidationValid = $true
	$expectedPass = 9
	$expectedFail = 0
	$expectedFocusedCaseStatus = "PASS"
	$expectedFocusedAssertionsPassed = 35
	$expectedCertificationProven = 87
	$expectedCertificationFail = 0
	if ($ExpectedOutcome -ceq "rejected") {
		$expectedSummaryStatus = "failed-unapproved-hard-diagnostic"
		$expectedSuccess = $false
		$expectedError = "Campaign Debug runtime completed with unapproved hard diagnostics."
		$expectedAcceptanceDisposition = "rejected-fail-closed"
		$expectedReleaseDisposition = "replacement-required"
		$expectedDiagnosticsValid = $false
		$expectedHardCount = 3
		$expectedScriptErrors = 3
		$expectedUnapprovedCount = 1
		$expectedUnapprovedKind = "virtual-machine-exception"
		$expectedFindingStatus = "source-fix-required"
		$expectedFindingDefect = "MapLocator runtime lifecycle"
	}
	elseif ($ExpectedOutcome -ceq "rejected-proof") {
		$expectedSummaryStatus = "failed-proof-validation"
		$expectedSuccess = $false
		$expectedError = "Campaign Debug artifacts completed but failed the exact validation contract."
		$expectedAcceptanceDisposition = "rejected-focused-proof"
		$expectedReleaseDisposition = "replacement-required"
		$expectedFindingStatus = "source-fix-required"
		$expectedFindingDefect = "Ownership transition proof fixture"
		$expectedArtifactValidationValid = $false
		$expectedPass = 8
		$expectedFail = 1
		$expectedFocusedCaseStatus = "FAIL"
		$expectedFocusedAssertionsPassed = 33
		$expectedCertificationProven = 85
		$expectedCertificationFail = 2
	}
	if ([string] (Get-ObjectPropertyValue $summaryResult "status") -cne $expectedSummaryStatus -or
		(Get-ObjectPropertyValue $Evidence "outcomeSuccess") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $Evidence "outcomeSuccess") -ne $expectedSuccess -or
		(Get-ObjectPropertyValue $summaryResult "success") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryResult "success") -ne $expectedSuccess -or
		[string] (Get-ObjectPropertyValue $Evidence "error") -cne $expectedError -or
		[string] (Get-ObjectPropertyValue $summaryResult "error") -cne $expectedError -or
		[string] (Get-ObjectPropertyValue $Evidence "acceptanceDisposition") -cne
			$expectedAcceptanceDisposition -or
		[string] (Get-ObjectPropertyValue $summaryResult "acceptanceDisposition") -cne
			$expectedAcceptanceDisposition -or
		[string] (Get-ObjectPropertyValue $summaryResult "releaseDisposition") -cne
			$expectedReleaseDisposition) {
		throw "$Label outcome or acceptance disposition is inconsistent."
	}
	foreach ($requiredTrueResultField in @(
		"armed", "started", "completed", "candidateBoundaryVerified", "mountPacked",
		"artifactsStable", "evidenceCaptured")) {
		if ((Get-ObjectPropertyValue $summaryResult $requiredTrueResultField) -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $summaryResult $requiredTrueResultField)) {
			throw "$Label result.$requiredTrueResultField must be true."
		}
	}
	if ((Get-ObjectPropertyValue $summaryResult "artifactSchemaValidationValid") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryResult "artifactSchemaValidationValid") -ne
			$expectedArtifactValidationValid) {
		throw "$Label result.artifactSchemaValidationValid has the wrong acceptance value."
	}
	if ((Get-ObjectPropertyValue $summaryResult "certificationPassed") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryResult "certificationPassed")) {
		throw "$Label scoped canary must remain non-certifying."
	}

	$caseCount = [int] (Get-ObjectPropertyValue $summaryProof "caseCount")
	$pass = [int] (Get-ObjectPropertyValue $summaryProof "pass")
	$warn = [int] (Get-ObjectPropertyValue $summaryProof "warn")
	$fail = [int] (Get-ObjectPropertyValue $summaryProof "fail")
	$blocked = [int] (Get-ObjectPropertyValue $summaryProof "blocked")
	$skipped = [int] (Get-ObjectPropertyValue $summaryProof "skipped")
	if ($caseCount -ne 11 -or $pass -ne $expectedPass -or $warn -ne 1 -or
		$fail -ne $expectedFail -or
		$blocked -ne 1 -or $skipped -ne 0 -or
		$caseCount -ne ($pass + $warn + $fail + $blocked + $skipped) -or
		[string] (Get-ObjectPropertyValue $summaryProof "focusedCaseId") -cne
			"early_mechanics.force_authority" -or
		[string] (Get-ObjectPropertyValue $summaryProof "focusedCaseStatus") -cne
			$expectedFocusedCaseStatus -or
		[int] (Get-ObjectPropertyValue $summaryProof "focusedAssertionCount") -ne 35 -or
		[int] (Get-ObjectPropertyValue $summaryProof "focusedAssertionsPassed") -ne
			$expectedFocusedAssertionsPassed -or
		[int] (Get-ObjectPropertyValue $summaryProof "certificationRequired") -ne 87 -or
		[int] (Get-ObjectPropertyValue $summaryProof "certificationProven") -ne
			$expectedCertificationProven -or
		[int] (Get-ObjectPropertyValue $summaryProof "certificationFail") -ne
			$expectedCertificationFail -or
		[int] (Get-ObjectPropertyValue $summaryProof "certificationBlocked") -ne 0 -or
		[int] (Get-ObjectPropertyValue $summaryProof "stateDiffRows") -ne 18 -or
		[int] (Get-ObjectPropertyValue $summaryProof "nonzeroStateDiffRows") -ne 0 -or
		(Get-ObjectPropertyValue $summaryProof "finalOrphanCleanupPass") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryProof "finalOrphanCleanupPass") -or
		[int] (Get-ObjectPropertyValue $summaryProof "finalOrphanActiveGroups") -ne 0) {
		throw "$Label proof totals are inconsistent."
	}
	foreach ($proofField in @(
		"caseCount", "pass", "warn", "fail", "blocked", "skipped",
		"focusedAssertionCount", "focusedAssertionsPassed", "certificationRequired",
		"certificationProven", "stateDiffRows", "nonzeroStateDiffRows")) {
		if ([int] (Get-ObjectPropertyValue $Evidence $proofField) -ne
			[int] (Get-ObjectPropertyValue $summaryProof $proofField)) {
			throw "$Label.$proofField differs from its summary."
		}
	}
	if ((Get-ObjectPropertyValue $Evidence "finalOrphanCleanupPass") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $Evidence "finalOrphanCleanupPass")) {
		throw "$Label final orphan cleanup proof is not retained."
	}
	if ($ExpectedOutcome -ceq "rejected-proof") {
		$expectedValidationProblems = @(
			"focused-force-authority-case-status",
			"focused-ownership_transition.aggregate",
			"focused-ownership_transition.causes"
		)
		$expectedFailedAssertionIds = @(
			"ownership_transition.aggregate",
			"ownership_transition.causes"
		)
		$actualValidationProblems = @(Get-ObjectPropertyValue $summaryProof "validationProblems")
		$actualFailedAssertionIds = @(Get-ObjectPropertyValue $summaryProof "failedAssertionIds")
		if ($actualValidationProblems.Count -ne $expectedValidationProblems.Count -or
			$actualFailedAssertionIds.Count -ne $expectedFailedAssertionIds.Count) {
			throw "$Label rejected proof must retain the exact validation-problem and failed-assertion inventories."
		}
		for ($problemIndex = 0; $problemIndex -lt $expectedValidationProblems.Count; $problemIndex++) {
			if ([string] $actualValidationProblems[$problemIndex] -cne
				$expectedValidationProblems[$problemIndex]) {
				throw "$Label summary.proof.validationProblems differs at index $problemIndex."
			}
		}
		for ($assertionIndex = 0; $assertionIndex -lt $expectedFailedAssertionIds.Count; $assertionIndex++) {
			if ([string] $actualFailedAssertionIds[$assertionIndex] -cne
				$expectedFailedAssertionIds[$assertionIndex]) {
				throw "$Label summary.proof.failedAssertionIds differs at index $assertionIndex."
			}
		}
		$expectedOwnershipCauseEvidence = "military/political/mission/admin/debug/migration 1/1/0/1/1/1 | serialized intent queued 0 restore 1 political exact-once 0 repeat/restart 1/1"
		if ([string] (Get-ObjectPropertyValue $summaryProof "ownershipCauseEvidence") -cne
			$expectedOwnershipCauseEvidence) {
			throw "$Label summary.proof.ownershipCauseEvidence differs from the rejected runtime proof."
		}
	}

	$hardCount = [int] (Get-ObjectPropertyValue $summaryDiagnostics "hardDiagnosticCount")
	$scriptErrors = [int] (Get-ObjectPropertyValue $summaryDiagnostics "scriptErrors")
	$engineErrors = [int] (Get-ObjectPropertyValue $summaryDiagnostics "engineErrors")
	$partisanErrors = [int] (Get-ObjectPropertyValue $summaryDiagnostics "partisanErrors")
	$stockCount = [int] (Get-ObjectPropertyValue $summaryDiagnostics "approvedStockDiagnosticCount")
	$intentionalCount = [int] (Get-ObjectPropertyValue $summaryDiagnostics "approvedIntentionalDiagnosticCount")
	$unapprovedCount = [int] (Get-ObjectPropertyValue $summaryDiagnostics "unapprovedHardDiagnosticCount")
	$unapprovedKinds = @(Get-ObjectPropertyValue $summaryDiagnostics "unapprovedHardDiagnosticKinds")
	if ($ExpectedOutcome -ceq "rejected" -and $unapprovedKinds.Count -eq 1) {
		Require-IntegerProperty $unapprovedKinds[0] "count" `
			"$Label summary.diagnostics.unapprovedHardDiagnosticKinds[0].count" | Out-Null
	}
	if ((Get-ObjectPropertyValue $summaryDiagnostics "valid") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryDiagnostics "valid") -ne $expectedDiagnosticsValid -or
		(Get-ObjectPropertyValue $summaryDiagnostics "classificationValid") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryDiagnostics "classificationValid") -ne
			$expectedDiagnosticsValid -or
		(Get-ObjectPropertyValue $summaryDiagnostics "hardDiagnosticFree") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryDiagnostics "hardDiagnosticFree") -or
		$hardCount -ne $expectedHardCount -or $scriptErrors -ne $expectedScriptErrors -or
		$engineErrors -ne 0 -or $partisanErrors -ne 0 -or
		$hardCount -ne ($scriptErrors + $engineErrors) -or
		$stockCount -ne 2 -or $intentionalCount -ne 0 -or
		$unapprovedCount -ne $expectedUnapprovedCount -or
		$hardCount -ne ($stockCount + $intentionalCount + $unapprovedCount) -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "classifierSelfTestCount") -ne
			$expectedClassifierSelfTestCount -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "crashMarkers") -ne 0 -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "partisanSeverityLineCount") -ne 0 -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "malformedHardDiagnosticCount") -ne 0) {
		throw "$Label diagnostic census is inconsistent."
	}
	if ((($ExpectedOutcome -ceq "accepted" -or $ExpectedOutcome -ceq "rejected-proof") -and
			$unapprovedKinds.Count -ne 0) -or
		($ExpectedOutcome -ceq "rejected" -and
			($unapprovedKinds.Count -ne 1 -or
			[string] (Get-ObjectPropertyValue $unapprovedKinds[0] "kind") -cne
				$expectedUnapprovedKind -or
			[int] (Get-ObjectPropertyValue $unapprovedKinds[0] "count") -ne 1))) {
		throw "$Label unapproved diagnostic-kind census is inconsistent."
	}
	foreach ($requiredTrueDiagnosticField in @(
		"channelArithmeticValid", "categoryArithmeticValid", "lifecycleMarkersValid",
		"identityBaselinePairValid", "intentionalFixtureStructureExact",
		"intentionalFixtureSetValid", "canonicalLogPairSameDirectory")) {
		if ((Get-ObjectPropertyValue $summaryDiagnostics $requiredTrueDiagnosticField) -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $summaryDiagnostics $requiredTrueDiagnosticField)) {
			throw "$Label diagnostics.$requiredTrueDiagnosticField must be true."
		}
	}
	if ([int] (Get-ObjectPropertyValue $summaryDiagnostics "canonicalScriptLogCount") -ne 1 -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "canonicalConsoleLogCount") -ne 1 -or
		(Get-ObjectPropertyValue $Evidence "diagnosticClassificationValid") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $Evidence "diagnosticClassificationValid") -ne
			$expectedDiagnosticsValid -or
		(Get-ObjectPropertyValue $Evidence "hardDiagnosticFree") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $Evidence "hardDiagnosticFree") -or
		[string] (Get-ObjectPropertyValue $Evidence "unapprovedHardDiagnosticKind") -cne
			$expectedUnapprovedKind) {
		throw "$Label diagnostic classification disposition is inconsistent."
	}
	foreach ($diagnosticField in @(
		"hardDiagnosticCount", "scriptErrors", "engineErrors", "partisanErrors",
		"approvedStockDiagnosticCount", "approvedIntentionalDiagnosticCount",
		"unapprovedHardDiagnosticCount", "classifierSelfTestCount",
		"canonicalScriptLogCount", "canonicalConsoleLogCount")) {
		if ([int] (Get-ObjectPropertyValue $Evidence $diagnosticField) -ne
			[int] (Get-ObjectPropertyValue $summaryDiagnostics $diagnosticField)) {
			throw "$Label.$diagnosticField differs from its summary."
		}
	}
	if ((Get-ObjectPropertyValue $Evidence "canonicalLogPairSameDirectory") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $Evidence "canonicalLogPairSameDirectory")) {
		throw "$Label canonical log pair is not retained."
	}

	foreach ($cleanupField in @(
		"guardRemaining", "ownedProcessesRemaining", "newEngineProcessesRemaining",
		"unclaimedEngineProcessesObserved", "newDefaultEntriesRemaining",
		"modifiedDefaultFiles", "deletedDefaultEntries", "missingDefaultRoots",
		"externalSpillEntriesRemaining", "modifiedSpillFiles", "deletedSpillEntries",
		"missingSpillRoots", "cleanupPhaseErrorCount")) {
		if ([int] (Get-ObjectPropertyValue $summaryCleanup $cleanupField) -ne 0) {
			throw "$Label cleanup field $cleanupField must be zero."
		}
	}
	if ((Get-ObjectPropertyValue $summaryCleanup "monitoringRootsAreDetectionOnly") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryCleanup "monitoringRootsAreDetectionOnly") -or
		(Get-ObjectPropertyValue $Evidence "cleanupAndSpillZero") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $Evidence "cleanupAndSpillZero")) {
		throw "$Label cleanup and spill boundary is not clean."
	}

	$envelopeSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "envelopeSha256") "$Label.envelopeSha256").ToLowerInvariant()
	$runSummarySha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "runSummarySha256") "$Label.runSummarySha256").ToLowerInvariant()
	if ($envelopeSha -cne $runSummarySha -or
		[string] (Get-ObjectPropertyValue $summaryIntegrity "envelopeSha256") -cne $envelopeSha -or
		[string] (Get-ObjectPropertyValue $summaryIntegrity "runSummarySha256") -cne
			$runSummarySha -or
		[int] (Get-ObjectPropertyValue $summaryIntegrity "envelopeFileCount") -ne 10 -or
		[int] (Get-ObjectPropertyValue $Evidence "envelopeFileCount") -ne 10 -or
		(Get-ObjectPropertyValue $summaryIntegrity "envelopeFilesRehashed") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryIntegrity "envelopeFilesRehashed") -or
		(Get-ObjectPropertyValue $Evidence "envelopeFilesRehashed") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $Evidence "envelopeFilesRehashed") -or
		[string] (Get-ObjectPropertyValue $summaryFinding "status") -cne
			$expectedFindingStatus -or
		[string] (Get-ObjectPropertyValue $summaryFinding "defect") -cne
			$expectedFindingDefect -or
		[string]::IsNullOrWhiteSpace([string] (Get-ObjectPropertyValue $summaryFinding "nextStep"))) {
		throw "$Label integrity or next-step disposition is inconsistent."
	}

	return [PSCustomObject] @{
		SummaryPath = $summaryPath
		SummarySha256 = $summarySha
		HarnessGitHead = $harnessHead
		StartedUtc = $startedUtc
		CompletedUtc = $completedUtc
		RunId = $runId
		RunLeafId = $runLeafId
		EnvelopeSha256 = $envelopeSha
		RunSummarySha256 = $runSummarySha
	}
}

function Assert-HistoricalRejectedFullCampaignDebugEvidence {
	param(
		[object] $Evidence,
		[object] $CandidateIdentity,
		[string] $Label,
		[DateTimeOffset] $StatusAsOfUtc,
		[int] $SourceSettingsSchema
	)

	if ($null -eq $Evidence) {
		throw "$Label is missing."
	}
	$expectedStatus = "failed-certification-and-unapproved-diagnostics"
	if ([string] (Get-ObjectPropertyValue $Evidence "status") -cne $expectedStatus) {
		throw "$Label.status must be $expectedStatus."
	}
	$expected = switch ([string] $CandidateIdentity.CandidateId) {
		"partisan-rc-0e632ec4f63e-20260719T004133Z" {
			[PSCustomObject] @{
				SummaryPath = "docs/evidence/campaign-debug/partisan-rc-0e632ec4f63e-20260719T004133Z-full-20260719T014151Z.json"
				SummarySha256 = "ed225ba2acb6932437af55219ff0b6ba69f4a2111880acd11c2555c875819ca7"
				HarnessGitHead = "27052811bb192835fc09ab3cb052b36cabad5df4"
				CampaignRunnerSha256 = "56de386cbdb5677f4315c99e690eefa7158cc6e00e043ded221cec04bde74479"
				CandidateModuleSha256 = "2c827fc6cc965813d95c77a4eb063b5adc79025ce3fdc8bca8d3ea2788b57dc2"
				SettingsSha256 = "fb744c7e85b93797da55e03fb8c2c1f0864e1ae43c2c49dfddd94ba91d24981c"
				RunLeafId = "20260719T014151Z-470870c9cc7e4493afb9a6ceb6ff2bce"
				RunId = "seed1985_t0_p1_u1784425330"
				StartedUtc = "2026-07-19T01:41:51.6669572Z"
				CompletedUtc = "2026-07-19T01:54:57.0935077Z"
				RuntimeSeconds = 783
				CaseCount = 687
				Pass = 584
				Warn = 49
				Fail = 46
				Blocked = 7
				Skipped = 1
				RequiredAssertions = 5687
				ProvenAssertions = 5561
				FailedAssertions = 112
				BlockedAssertions = 14
				Phase24ChecksPassed = 1
				IntentionalAdmissionProven = $true
				IntentionalSettlementProven = $false
				IntentionalCorruptionProven = $true
				IntentionalWatchdogProven = $true
				HardDiagnosticCount = 25
				ScriptErrors = 25
				EngineErrors = 0
				PartisanErrors = 19
				ApprovedStockDiagnosticCount = 2
				ApprovedIntentionalDiagnosticCount = 13
				UnapprovedHardDiagnosticCount = 10
				UnapprovedHardDiagnosticKinds = [ordered] @{
					"partisan-script-error" = 6
					"runtime-script-error" = 2
					"virtual-machine-exception" = 2
				}
				IntentionalFixtureStructureExact = $true
				IntentionalFixtureSetValid = $false
				EnvelopeSha256 = "f61bd05fcc5c95c5d0ddbbeb46a9220771d116b86bad1ad4f26340f4853ec825"
				RawArtifactSha256 = "8ec713c0f4a5208d848c113ca563eaf132476c9f5177d689f28e2020e14c865b"
			}
			break
		}
		"partisan-rc-ee0e8add2a29-20260719T063815Z" {
			[PSCustomObject] @{
				SummaryPath = "docs/evidence/campaign-debug/partisan-rc-ee0e8add2a29-20260719T063815Z-full-20260719T072739Z.json"
				SummarySha256 = "e83bc1e752ac4c1abc5cb57ce097459642e17637f6747e4edc8e7d57569c1884"
				HarnessGitHead = "a5ccf36aee17a4f88d7f1c2f232ce9fc14652018"
				CampaignRunnerSha256 = "56de386cbdb5677f4315c99e690eefa7158cc6e00e043ded221cec04bde74479"
				CandidateModuleSha256 = "22d2f7367d122ca9307140ef6400e5b1646bdf6b12922257811b6185fe345ba7"
				SettingsSha256 = "fb744c7e85b93797da55e03fb8c2c1f0864e1ae43c2c49dfddd94ba91d24981c"
				RunLeafId = "20260719T072739Z-97fc069d58cd427c848c83f99f39e5f9"
				RunId = "seed1985_t0_p1_u1784446076"
				StartedUtc = "2026-07-19T07:27:39.1454367Z"
				CompletedUtc = "2026-07-19T07:40:09.5714410Z"
				RuntimeSeconds = 749
				CaseCount = 685
				Pass = 598
				Warn = 47
				Fail = 26
				Blocked = 13
				Skipped = 1
				RequiredAssertions = 5695
				ProvenAssertions = 5630
				FailedAssertions = 50
				BlockedAssertions = 15
				Phase24ChecksPassed = 2
				IntentionalAdmissionProven = $true
				IntentionalSettlementProven = $true
				IntentionalCorruptionProven = $true
				IntentionalWatchdogProven = $true
				HardDiagnosticCount = 26
				ScriptErrors = 26
				EngineErrors = 0
				PartisanErrors = 22
				ApprovedStockDiagnosticCount = 2
				ApprovedIntentionalDiagnosticCount = 0
				UnapprovedHardDiagnosticCount = 24
				UnapprovedHardDiagnosticKinds = [ordered] @{
					"partisan-script-error" = 22
					"runtime-script-error" = 2
				}
				IntentionalFixtureStructureExact = $false
				IntentionalFixtureSetValid = $false
				EnvelopeSha256 = "fce4928444f15531f254ad4d7e119cf8bfe1d06e6fcb564518d2e052544d4278"
				RawArtifactSha256 = "6b37441665aa3aec1b5b2aa0fa43e70798bf9ba97e7839c34198b423c96144e3"
			}
			break
		}
		default {
			throw "$Label candidate has no sealed full-profile expectation."
		}
	}

	$summaryPath = Require-RepoRelativePath `
		(Get-ObjectPropertyValue $Evidence "summaryPath") "$Label.summaryPath"
	$summarySha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "summarySha256") "$Label.summarySha256").ToLowerInvariant()
	$harnessHead = Require-Text `
		(Get-ObjectPropertyValue $Evidence "harnessGitHead") "$Label.harnessGitHead"
	if ($harnessHead -cnotmatch '^[0-9a-f]{40}$') {
		throw "$Label.harnessGitHead must be a lowercase full Git SHA."
	}
	if ($summaryPath -cne [string] $expected.SummaryPath -or
		$summarySha -cne [string] $expected.SummarySha256 -or
		$harnessHead -cne [string] $expected.HarnessGitHead) {
		throw "$Label summary or harness identity differs from its sealed candidate-specific expectation."
	}

	$repositoryPrefix = [IO.Path]::GetFullPath($root).TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
	$summaryFullPath = [IO.Path]::GetFullPath((Join-Path $root $summaryPath))
	if (-not $summaryFullPath.StartsWith(
		$repositoryPrefix,
		[StringComparison]::OrdinalIgnoreCase) -or
		-not (Test-Path -LiteralPath $summaryFullPath -PathType Leaf)) {
		throw "$Label summary is missing or outside the repository."
	}
	$summaryItem = Get-Item -LiteralPath $summaryFullPath -Force
	if (($summaryItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
		throw "$Label summary must not be a reparse point."
	}
	$actualSummarySha = (Get-FileHash `
		-LiteralPath $summaryFullPath -Algorithm SHA256).Hash.ToLowerInvariant()
	if ($actualSummarySha -cne $summarySha) {
		throw "$Label summary SHA-256 does not match release status."
	}
	$summaryText = Get-Content -Raw -LiteralPath $summaryFullPath
	if ($summaryText -match '(?i)[A-Z]:[\\/]') {
		throw "$Label summary contains a local absolute path."
	}
	$summary = $summaryText | ConvertFrom-Json
	$summaryCandidate = Get-ObjectPropertyValue $summary "candidate"
	$summaryHarness = Get-ObjectPropertyValue $summary "harness"
	$summarySettings = Get-ObjectPropertyValue $summary "settings"
	$summaryCapture = Get-ObjectPropertyValue $summary "capture"
	$summaryResult = Get-ObjectPropertyValue $summary "result"
	$summaryProof = Get-ObjectPropertyValue $summary "proof"
	$summaryDiagnostics = Get-ObjectPropertyValue $summary "diagnostics"
	$summaryCleanup = Get-ObjectPropertyValue $summary "cleanup"
	$summaryIntegrity = Get-ObjectPropertyValue $summary "integrity"
	$summaryFinding = Get-ObjectPropertyValue $summary "finding"
	if ((Require-IntegerProperty $summary "schemaVersion" "$Label summary.schemaVersion") -ne 1 -or
		[string] (Get-ObjectPropertyValue $summary "evidenceKind") -cne
			"packaged-campaign-debug-full-profile" -or
		$null -eq $summaryCandidate -or $null -eq $summaryHarness -or
		$null -eq $summarySettings -or $null -eq $summaryCapture -or
		$null -eq $summaryResult -or $null -eq $summaryProof -or
		$null -eq $summaryDiagnostics -or $null -eq $summaryCleanup -or
		$null -eq $summaryIntegrity -or $null -eq $summaryFinding) {
		throw "$Label summary is structurally incomplete."
	}

	Assert-IntegerProperties $summarySettings @("schemaVersion") "$Label summary.settings"
	Assert-IntegerProperties $summaryCapture @("runtimeSeconds") "$Label summary.capture"
	Assert-IntegerProperties $summaryProof @(
		"startedAtSecond", "endedAtSecond", "caseCount", "pass", "warn", "fail",
		"blocked", "skipped", "certificationRequired", "certificationProven",
		"certificationFail", "certificationBlocked", "certificationWarn",
		"stateDiffRows", "nonzeroStateDiffRows", "phase17ProjectionCheckCount",
		"phase17ProjectionChecksPassed", "phase24CheckCount", "phase24ChecksPassed",
		"stagedCleanupCheckCount", "stagedCleanupChecksPassed",
		"finalOrphanActiveGroups") "$Label summary.proof"
	Assert-IntegerProperties $summaryDiagnostics @(
		"hardDiagnosticCount", "scriptErrors", "engineErrors", "partisanErrors",
		"crashMarkers", "partisanSeverityLineCount", "approvedStockDiagnosticCount",
		"approvedIntentionalDiagnosticCount", "unapprovedHardDiagnosticCount",
		"classifierSelfTestCount", "malformedHardDiagnosticCount",
		"canonicalScriptLogCount", "canonicalConsoleLogCount") "$Label summary.diagnostics"
	Assert-IntegerProperties $summaryCleanup @(
		"guardRemaining", "ownedProcessesRemaining", "newEngineProcessesRemaining",
		"unclaimedEngineProcessesObserved", "newDefaultEntriesRemaining",
		"modifiedDefaultFiles", "deletedDefaultEntries", "missingDefaultRoots",
		"externalSpillEntriesRemaining", "modifiedSpillFiles", "deletedSpillEntries",
		"missingSpillRoots", "cleanupPhaseErrorCount") "$Label summary.cleanup"
	Assert-IntegerProperties $summaryIntegrity @("envelopeFileCount") "$Label summary.integrity"
	Assert-IntegerProperties $Evidence @(
		"runtimeSeconds", "caseCount", "pass", "warn", "fail", "blocked", "skipped",
		"requiredAssertions", "provenAssertions", "failedAssertions",
		"blockedAssertions", "hardDiagnosticCount", "scriptErrors", "engineErrors",
		"partisanErrors", "approvedStockDiagnosticCount",
		"approvedIntentionalDiagnosticCount", "unapprovedHardDiagnosticCount",
		"classifierSelfTestCount", "canonicalScriptLogCount",
		"canonicalConsoleLogCount", "stateDiffRows", "nonzeroStateDiffRows",
		"envelopeFileCount") $Label

	if ([string] (Get-ObjectPropertyValue $Evidence "candidateId") -cne
			[string] $CandidateIdentity.CandidateId -or
		[string] (Get-ObjectPropertyValue $Evidence "candidateSourceHead") -cne
			[string] $CandidateIdentity.CandidateSourceHead -or
		[string] (Get-ObjectPropertyValue $Evidence "packageSha256") -cne
			[string] $CandidateIdentity.PackageSha256 -or
		[string] (Get-ObjectPropertyValue $Evidence "manifestSha256") -cne
			[string] $CandidateIdentity.ManifestSha256 -or
		[string] (Get-ObjectPropertyValue $Evidence "readySha256") -cne
			[string] $CandidateIdentity.ReadySha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "candidateId") -cne
			[string] $CandidateIdentity.CandidateId -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "candidateSourceHead") -cne
			[string] $CandidateIdentity.CandidateSourceHead -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "packageSha256") -cne
			[string] $CandidateIdentity.PackageSha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "manifestSha256") -cne
			[string] $CandidateIdentity.ManifestSha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "readySha256") -cne
			[string] $CandidateIdentity.ReadySha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "workbenchCrc") -cne
			[string] $CandidateIdentity.WorkbenchCrc -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "runtimeUseDispositionAtCapture") -cne
			"active-runtime-candidate") {
		throw "$Label differs from the active retained candidate identity."
	}

	$runnerSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "campaignRunnerSha256") `
		"$Label.campaignRunnerSha256").ToLowerInvariant()
	$expectedClassifierSelfTestCount = Resolve-CampaignClassifierSelfTestCount `
		$runnerSha "$Label.campaignRunnerSha256"
	$candidateModuleSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "candidateModuleSha256") `
		"$Label.candidateModuleSha256").ToLowerInvariant()
	$settingsSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "settingsSha256") `
		"$Label.settingsSha256").ToLowerInvariant()
	if ([string] (Get-ObjectPropertyValue $summaryHarness "gitHead") -cne $harnessHead -or
		(Get-ObjectPropertyValue $summaryHarness "dirty") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryHarness "dirty") -or
		$runnerSha -cne [string] $expected.CampaignRunnerSha256 -or
		$candidateModuleSha -cne [string] $expected.CandidateModuleSha256 -or
		$settingsSha -cne [string] $expected.SettingsSha256 -or
		[string] (Get-ObjectPropertyValue $summaryHarness "campaignRunnerSha256") -cne
			$runnerSha -or
		[string] (Get-ObjectPropertyValue $summaryHarness "candidateModuleSha256") -cne
			$candidateModuleSha -or
		[int] (Get-ObjectPropertyValue $summarySettings "schemaVersion") -ne
			$SourceSettingsSchema -or
		[string] (Get-ObjectPropertyValue $summarySettings "sha256") -cne $settingsSha -or
		(Get-ObjectPropertyValue $summarySettings "guardedRuntimeCopy") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summarySettings "guardedRuntimeCopy")) {
		throw "$Label harness or settings identity is inconsistent."
	}

	$runLeafId = Require-Text `
		(Get-ObjectPropertyValue $summaryCapture "runLeafId") "$Label run-leaf ID"
	$runId = Require-Text `
		(Get-ObjectPropertyValue $summaryCapture "runId") "$Label run ID"
	$startedUtc = Require-UtcTimestamp `
		(Get-ObjectPropertyValue $summaryCapture "startedUtc") "$Label start time"
	$completedUtc = Require-UtcTimestamp `
		(Get-ObjectPropertyValue $summaryCapture "completedUtc") "$Label completion time"
	$captureRuntimeSeconds = [int] (Get-ObjectPropertyValue $summaryCapture "runtimeSeconds")
	if ($completedUtc -lt $startedUtc -or $StatusAsOfUtc -lt $completedUtc -or
		[string] (Get-ObjectPropertyValue $summaryCapture "startedUtc") -cne
			[string] $expected.StartedUtc -or
		[string] (Get-ObjectPropertyValue $summaryCapture "completedUtc") -cne
			[string] $expected.CompletedUtc -or
		[string] (Get-ObjectPropertyValue $Evidence "startedUtc") -cne
			[string] (Get-ObjectPropertyValue $summaryCapture "startedUtc") -or
		[string] (Get-ObjectPropertyValue $Evidence "completedUtc") -cne
			[string] (Get-ObjectPropertyValue $summaryCapture "completedUtc") -or
		[string] (Get-ObjectPropertyValue $Evidence "runLeafId") -cne
			$runLeafId -or
		$runLeafId -cne [string] $expected.RunLeafId -or
		$runLeafId -cnotmatch
			'^\d{8}T\d{6}Z-[0-9a-f]{32}$' -or
		[string] (Get-ObjectPropertyValue $Evidence "runId") -cne
			$runId -or
		$runId -cne [string] $expected.RunId -or
		$runId -cnotmatch
			'^seed\d+_t\d+_p\d+_u\d+$' -or
		[string] (Get-ObjectPropertyValue $summaryCapture "profile") -cne "full_certification" -or
		[string] (Get-ObjectPropertyValue $summaryCapture "proofScope") -cne
			"full_certification" -or
		$captureRuntimeSeconds -ne [int] $expected.RuntimeSeconds -or
		[int] (Get-ObjectPropertyValue $Evidence "runtimeSeconds") -ne $captureRuntimeSeconds) {
		throw "$Label capture identity, timestamps, or runtime are inconsistent."
	}

	$expectedError = "Campaign Debug runtime completed with unapproved hard diagnostics."
	if ([string] (Get-ObjectPropertyValue $summaryResult "status") -cne $expectedStatus -or
		(Get-ObjectPropertyValue $summaryResult "wrapperCaptureSuccess") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryResult "wrapperCaptureSuccess") -or
		(Get-ObjectPropertyValue $Evidence "wrapperCaptureSuccess") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $Evidence "wrapperCaptureSuccess") -or
		(Get-ObjectPropertyValue $summaryResult "runtimeOutcomeSuccess") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryResult "runtimeOutcomeSuccess") -or
		(Get-ObjectPropertyValue $Evidence "runtimeOutcomeSuccess") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $Evidence "runtimeOutcomeSuccess") -or
		[string] (Get-ObjectPropertyValue $summaryResult "acceptanceDisposition") -cne
			"rejected-red-full-profile" -or
		[string] (Get-ObjectPropertyValue $Evidence "acceptanceDisposition") -cne
			"rejected-red-full-profile" -or
		[string] (Get-ObjectPropertyValue $summaryResult "releaseDisposition") -cne
			"remain-no-go" -or
		[string] (Get-ObjectPropertyValue $summaryResult "error") -cne $expectedError -or
		[string] (Get-ObjectPropertyValue $Evidence "error") -cne $expectedError) {
		throw "$Label must separate successful wrapper capture from failed runtime acceptance."
	}
	foreach ($requiredTrueResultField in @(
		"armed", "started", "completed", "candidateBoundaryVerified", "mountPacked",
		"artifactsStable", "evidenceCaptured", "artifactSchemaValidationValid")) {
		if ((Get-ObjectPropertyValue $summaryResult $requiredTrueResultField) -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $summaryResult $requiredTrueResultField)) {
			throw "$Label result.$requiredTrueResultField must be true."
		}
	}
	foreach ($retainedResultField in @(
		"candidateBoundaryVerified", "mountPacked", "artifactsStable",
		"artifactSchemaValidationValid")) {
		if ((Get-ObjectPropertyValue $Evidence $retainedResultField) -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $Evidence $retainedResultField)) {
			throw "$Label.$retainedResultField must be true."
		}
	}
	if ((Get-ObjectPropertyValue $summaryResult "certificationPassed") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryResult "certificationPassed") -or
		(Get-ObjectPropertyValue $Evidence "certificationPassed") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $Evidence "certificationPassed")) {
		throw "$Label must retain failed full certification."
	}

	$caseCount = [int] (Get-ObjectPropertyValue $summaryProof "caseCount")
	$pass = [int] (Get-ObjectPropertyValue $summaryProof "pass")
	$warn = [int] (Get-ObjectPropertyValue $summaryProof "warn")
	$fail = [int] (Get-ObjectPropertyValue $summaryProof "fail")
	$blocked = [int] (Get-ObjectPropertyValue $summaryProof "blocked")
	$skipped = [int] (Get-ObjectPropertyValue $summaryProof "skipped")
	$requiredAssertions = [int] (Get-ObjectPropertyValue $summaryProof "certificationRequired")
	$provenAssertions = [int] (Get-ObjectPropertyValue $summaryProof "certificationProven")
	$failedAssertions = [int] (Get-ObjectPropertyValue $summaryProof "certificationFail")
	$blockedAssertions = [int] (Get-ObjectPropertyValue $summaryProof "certificationBlocked")
	if ((Get-ObjectPropertyValue $summaryProof "fullCertification") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryProof "fullCertification") -or
		[int] (Get-ObjectPropertyValue $summaryProof "startedAtSecond") -ne 0 -or
		[int] (Get-ObjectPropertyValue $summaryProof "endedAtSecond") -ne 7201 -or
		$caseCount -ne [int] $expected.CaseCount -or
		$pass -ne [int] $expected.Pass -or
		$warn -ne [int] $expected.Warn -or
		$fail -ne [int] $expected.Fail -or
		$blocked -ne [int] $expected.Blocked -or
		$skipped -ne [int] $expected.Skipped -or
		$caseCount -ne ($pass + $warn + $fail + $blocked + $skipped) -or
		$requiredAssertions -ne [int] $expected.RequiredAssertions -or
		$provenAssertions -ne [int] $expected.ProvenAssertions -or
		$failedAssertions -ne [int] $expected.FailedAssertions -or
		$blockedAssertions -ne [int] $expected.BlockedAssertions -or
		$requiredAssertions -ne
			($provenAssertions + $failedAssertions + $blockedAssertions) -or
		[int] (Get-ObjectPropertyValue $summaryProof "certificationWarn") -ne 0 -or
		[int] (Get-ObjectPropertyValue $summaryProof "stateDiffRows") -ne 18 -or
		[int] (Get-ObjectPropertyValue $summaryProof "nonzeroStateDiffRows") -ne 0 -or
		[int] (Get-ObjectPropertyValue $summaryProof "phase17ProjectionCheckCount") -ne 11 -or
		[int] (Get-ObjectPropertyValue $summaryProof "phase17ProjectionChecksPassed") -ne 11 -or
		[int] (Get-ObjectPropertyValue $summaryProof "phase24CheckCount") -ne 2 -or
		[int] (Get-ObjectPropertyValue $summaryProof "phase24ChecksPassed") -ne
			[int] $expected.Phase24ChecksPassed -or
		[int] (Get-ObjectPropertyValue $summaryProof "stagedCleanupCheckCount") -ne 6 -or
		[int] (Get-ObjectPropertyValue $summaryProof "stagedCleanupChecksPassed") -ne 6 -or
		(Get-ObjectPropertyValue $summaryProof "finalOrphanCleanupPass") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryProof "finalOrphanCleanupPass") -or
		[int] (Get-ObjectPropertyValue $summaryProof "finalOrphanActiveGroups") -ne 0 -or
		(Get-ObjectPropertyValue $summaryProof "intentionalMissionConvoyAdmissionDiagnosticsProven") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryProof "intentionalMissionConvoyAdmissionDiagnosticsProven") -ne
			[bool] $expected.IntentionalAdmissionProven -or
		(Get-ObjectPropertyValue $summaryProof "intentionalMissionConvoySettlementDiagnosticProven") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryProof "intentionalMissionConvoySettlementDiagnosticProven") -ne
			[bool] $expected.IntentionalSettlementProven -or
		(Get-ObjectPropertyValue $summaryProof "intentionalMissionConvoyCorruptionDiagnosticsProven") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryProof "intentionalMissionConvoyCorruptionDiagnosticsProven") -ne
			[bool] $expected.IntentionalCorruptionProven -or
		(Get-ObjectPropertyValue $summaryProof "intentionalMissionConvoyWatchdogDiagnosticProven") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryProof "intentionalMissionConvoyWatchdogDiagnosticProven") -ne
			[bool] $expected.IntentionalWatchdogProven) {
		throw "$Label proof totals or exact fixture bindings are inconsistent."
	}
	$proofFieldMap = [ordered] @{
		caseCount = "caseCount"
		pass = "pass"
		warn = "warn"
		fail = "fail"
		blocked = "blocked"
		skipped = "skipped"
		requiredAssertions = "certificationRequired"
		provenAssertions = "certificationProven"
		failedAssertions = "certificationFail"
		blockedAssertions = "certificationBlocked"
		stateDiffRows = "stateDiffRows"
		nonzeroStateDiffRows = "nonzeroStateDiffRows"
	}
	foreach ($statusField in $proofFieldMap.Keys) {
		$summaryField = [string] $proofFieldMap[$statusField]
		if ([int] (Get-ObjectPropertyValue $Evidence $statusField) -ne
			[int] (Get-ObjectPropertyValue $summaryProof $summaryField)) {
			throw "$Label.$statusField differs from its summary."
		}
	}
	if ((Get-ObjectPropertyValue $Evidence "finalOrphanCleanupPass") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $Evidence "finalOrphanCleanupPass")) {
		throw "$Label final orphan cleanup proof is not retained."
	}

	$hardCount = [int] (Get-ObjectPropertyValue $summaryDiagnostics "hardDiagnosticCount")
	$scriptErrors = [int] (Get-ObjectPropertyValue $summaryDiagnostics "scriptErrors")
	$engineErrors = [int] (Get-ObjectPropertyValue $summaryDiagnostics "engineErrors")
	$partisanErrors = [int] (Get-ObjectPropertyValue $summaryDiagnostics "partisanErrors")
	$stockCount = [int] (Get-ObjectPropertyValue $summaryDiagnostics "approvedStockDiagnosticCount")
	$intentionalCount = [int] (Get-ObjectPropertyValue $summaryDiagnostics "approvedIntentionalDiagnosticCount")
	$unapprovedCount = [int] (Get-ObjectPropertyValue $summaryDiagnostics "unapprovedHardDiagnosticCount")
	$unapprovedKinds = @(Get-ObjectPropertyValue $summaryDiagnostics "unapprovedHardDiagnosticKinds")
	$unapprovedKindMap = @{}
	foreach ($unapprovedKind in $unapprovedKinds) {
		$kind = Require-Text (Get-ObjectPropertyValue $unapprovedKind "kind") `
			"$Label summary.diagnostics unapproved kind"
		$count = Require-IntegerProperty $unapprovedKind "count" `
			"$Label summary.diagnostics unapproved kind $kind"
		if ($unapprovedKindMap.ContainsKey($kind)) {
			throw "$Label summary repeats unapproved diagnostic kind $kind."
		}
		$unapprovedKindMap[$kind] = [int] $count
	}
	if ((Get-ObjectPropertyValue $summaryDiagnostics "valid") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryDiagnostics "valid") -or
		(Get-ObjectPropertyValue $summaryDiagnostics "classificationValid") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryDiagnostics "classificationValid") -or
		(Get-ObjectPropertyValue $summaryDiagnostics "hardDiagnosticFree") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryDiagnostics "hardDiagnosticFree") -or
		$hardCount -ne [int] $expected.HardDiagnosticCount -or
		$scriptErrors -ne [int] $expected.ScriptErrors -or
		$engineErrors -ne [int] $expected.EngineErrors -or
		$partisanErrors -ne [int] $expected.PartisanErrors -or
		$hardCount -ne ($scriptErrors + $engineErrors) -or
		$stockCount -ne [int] $expected.ApprovedStockDiagnosticCount -or
		$intentionalCount -ne [int] $expected.ApprovedIntentionalDiagnosticCount -or
		$unapprovedCount -ne [int] $expected.UnapprovedHardDiagnosticCount -or
		$hardCount -ne ($stockCount + $intentionalCount + $unapprovedCount) -or
		$unapprovedKinds.Count -ne $expected.UnapprovedHardDiagnosticKinds.Count -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "classifierSelfTestCount") -ne
			$expectedClassifierSelfTestCount -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "crashMarkers") -ne 0 -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "partisanSeverityLineCount") -ne 0 -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "malformedHardDiagnosticCount") -ne 0 -or
		(Get-ObjectPropertyValue $summaryDiagnostics "channelArithmeticValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryDiagnostics "channelArithmeticValid") -or
		(Get-ObjectPropertyValue $summaryDiagnostics "categoryArithmeticValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryDiagnostics "categoryArithmeticValid") -or
		(Get-ObjectPropertyValue $summaryDiagnostics "lifecycleMarkersValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryDiagnostics "lifecycleMarkersValid") -or
		(Get-ObjectPropertyValue $summaryDiagnostics "identityBaselinePairValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryDiagnostics "identityBaselinePairValid") -or
		(Get-ObjectPropertyValue $summaryDiagnostics "intentionalFixtureStructureExact") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryDiagnostics "intentionalFixtureStructureExact") -ne
			[bool] $expected.IntentionalFixtureStructureExact -or
		(Get-ObjectPropertyValue $summaryDiagnostics "intentionalFixtureSetValid") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryDiagnostics "intentionalFixtureSetValid") -ne
			[bool] $expected.IntentionalFixtureSetValid -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "canonicalScriptLogCount") -ne 1 -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "canonicalConsoleLogCount") -ne 1 -or
		(Get-ObjectPropertyValue $summaryDiagnostics "canonicalLogPairSameDirectory") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryDiagnostics "canonicalLogPairSameDirectory")) {
		throw "$Label diagnostic census is inconsistent."
	}
	foreach ($expectedUnapprovedKind in $expected.UnapprovedHardDiagnosticKinds.Keys) {
		if (-not $unapprovedKindMap.ContainsKey([string] $expectedUnapprovedKind) -or
			[int] $unapprovedKindMap[[string] $expectedUnapprovedKind] -ne
				[int] $expected.UnapprovedHardDiagnosticKinds[$expectedUnapprovedKind]) {
			throw "$Label diagnostic census differs from its candidate-specific unapproved-kind totals."
		}
	}
	foreach ($diagnosticField in @(
		"hardDiagnosticCount", "scriptErrors", "engineErrors", "partisanErrors",
		"approvedStockDiagnosticCount", "approvedIntentionalDiagnosticCount",
		"unapprovedHardDiagnosticCount", "classifierSelfTestCount",
		"canonicalScriptLogCount", "canonicalConsoleLogCount")) {
		if ([int] (Get-ObjectPropertyValue $Evidence $diagnosticField) -ne
			[int] (Get-ObjectPropertyValue $summaryDiagnostics $diagnosticField)) {
			throw "$Label.$diagnosticField differs from its summary."
		}
	}
	if ((Get-ObjectPropertyValue $Evidence "diagnosticClassificationValid") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $Evidence "diagnosticClassificationValid") -or
		(Get-ObjectPropertyValue $Evidence "hardDiagnosticFree") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $Evidence "hardDiagnosticFree") -or
		(Get-ObjectPropertyValue $Evidence "canonicalLogPairSameDirectory") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $Evidence "canonicalLogPairSameDirectory")) {
		throw "$Label retained diagnostic disposition is inconsistent."
	}

	foreach ($cleanupField in @(
		"guardRemaining", "ownedProcessesRemaining", "newEngineProcessesRemaining",
		"unclaimedEngineProcessesObserved", "newDefaultEntriesRemaining",
		"modifiedDefaultFiles", "deletedDefaultEntries", "missingDefaultRoots",
		"externalSpillEntriesRemaining", "modifiedSpillFiles", "deletedSpillEntries",
		"missingSpillRoots", "cleanupPhaseErrorCount")) {
		if ([int] (Get-ObjectPropertyValue $summaryCleanup $cleanupField) -ne 0) {
			throw "$Label cleanup field $cleanupField must be zero."
		}
	}
	if ((Get-ObjectPropertyValue $summaryCleanup "monitoringRootsAreDetectionOnly") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryCleanup "monitoringRootsAreDetectionOnly") -or
		(Get-ObjectPropertyValue $Evidence "cleanupAndSpillZero") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $Evidence "cleanupAndSpillZero")) {
		throw "$Label cleanup and spill boundary is not clean."
	}

	$envelopeSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "envelopeSha256") "$Label.envelopeSha256").ToLowerInvariant()
	$runSummarySha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "runSummarySha256") "$Label.runSummarySha256").ToLowerInvariant()
	$rawArtifactSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $summaryIntegrity "rawArtifactSha256") `
		"$Label summary.integrity.rawArtifactSha256").ToLowerInvariant()
	if ($envelopeSha -cne [string] $expected.EnvelopeSha256 -or
		$runSummarySha -cne $envelopeSha -or
		[string] (Get-ObjectPropertyValue $summaryIntegrity "envelopeSha256") -cne
			$envelopeSha -or
		[string] (Get-ObjectPropertyValue $summaryIntegrity "runSummarySha256") -cne
			$runSummarySha -or
		$rawArtifactSha -cne [string] $expected.RawArtifactSha256 -or
		[int] (Get-ObjectPropertyValue $summaryIntegrity "envelopeFileCount") -ne 10 -or
		[int] (Get-ObjectPropertyValue $Evidence "envelopeFileCount") -ne 10 -or
		(Get-ObjectPropertyValue $summaryIntegrity "envelopeFilesRehashed") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryIntegrity "envelopeFilesRehashed") -or
		(Get-ObjectPropertyValue $Evidence "envelopeFilesRehashed") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $Evidence "envelopeFilesRehashed") -or
		[string] (Get-ObjectPropertyValue $summaryFinding "status") -cne
			"release-blocking-red-full-profile" -or
		[string] (Get-ObjectPropertyValue $summaryFinding "defect") -cne
			"Full certification and diagnostic acceptance both failed." -or
		[string]::IsNullOrWhiteSpace(
			[string] (Get-ObjectPropertyValue $summaryFinding "nextStep")) -or
		[string]::IsNullOrWhiteSpace([string] (Get-ObjectPropertyValue $Evidence "summary"))) {
		throw "$Label integrity or release-blocking disposition is inconsistent."
	}

	return [PSCustomObject] @{
		SummaryPath = $summaryPath
		SummarySha256 = $summarySha
		HarnessGitHead = $harnessHead
		StartedUtc = $startedUtc
		CompletedUtc = $completedUtc
		RunId = $runId
		RunLeafId = $runLeafId
		EnvelopeSha256 = $envelopeSha
		RunSummarySha256 = $runSummarySha
	}
}

function Get-GitBlobSha256 {
	param(
		[string] $Revision,
		[string] $RepoRelativePath,
		[string] $Label
	)

	if ($Revision -cnotmatch '^[0-9a-f]{40}$') {
		throw "$Label revision must be a lowercase full Git SHA."
	}
	$path = Require-RepoRelativePath $RepoRelativePath "$Label path"
	$startInfo = New-Object Diagnostics.ProcessStartInfo
	$startInfo.FileName = "git"
	$escapedRoot = $root.Replace('"', '\"')
	$escapedSpec = ("{0}:{1}" -f $Revision, $path).Replace('"', '\"')
	$startInfo.Arguments = "-C `"$escapedRoot`" cat-file blob `"$escapedSpec`""
	$startInfo.UseShellExecute = $false
	$startInfo.CreateNoWindow = $true
	$startInfo.RedirectStandardOutput = $true
	$startInfo.RedirectStandardError = $true
	$process = New-Object Diagnostics.Process
	$process.StartInfo = $startInfo
	try {
		if (-not $process.Start()) {
			throw "$Label Git blob reader could not start."
		}
		$memory = New-Object IO.MemoryStream
		try {
			$process.StandardOutput.BaseStream.CopyTo($memory)
			$errorText = $process.StandardError.ReadToEnd()
			$process.WaitForExit()
			if ($process.ExitCode -ne 0) {
				throw "$Label is not an immutable blob at harness revision $Revision`: $errorText"
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

function Get-GitBlobTextAndSha256 {
	param(
		[string] $Revision,
		[string] $RepoRelativePath,
		[string] $Label
	)

	if ($Revision -cnotmatch '^[0-9a-f]{40}$') {
		throw "$Label revision must be a lowercase full Git SHA."
	}
	$path = Require-RepoRelativePath $RepoRelativePath "$Label path"
	$startInfo = New-Object Diagnostics.ProcessStartInfo
	$startInfo.FileName = "git"
	$escapedRoot = $root.Replace('"', '\"')
	$escapedSpec = ("{0}:{1}" -f $Revision, $path).Replace('"', '\"')
	$startInfo.Arguments = "-C `"$escapedRoot`" cat-file blob `"$escapedSpec`""
	$startInfo.UseShellExecute = $false
	$startInfo.CreateNoWindow = $true
	$startInfo.RedirectStandardOutput = $true
	$startInfo.RedirectStandardError = $true
	$process = New-Object Diagnostics.Process
	$process.StartInfo = $startInfo
	try {
		if (-not $process.Start()) {
			throw "$Label Git blob reader could not start."
		}
		$memory = New-Object IO.MemoryStream
		try {
			$process.StandardOutput.BaseStream.CopyTo($memory)
			$errorText = $process.StandardError.ReadToEnd()
			$process.WaitForExit()
			if ($process.ExitCode -ne 0) {
				throw "$Label is not an immutable blob at harness revision $Revision`: $errorText"
			}
			$bytes = $memory.ToArray()
			$sha = [Security.Cryptography.SHA256]::Create()
			try {
				$hash = ([BitConverter]::ToString(
					$sha.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant()
			}
			finally {
				$sha.Dispose()
			}
			return [PSCustomObject] @{
				Text = [Text.Encoding]::UTF8.GetString($bytes)
				Sha256 = $hash
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

function Assert-PortableFullCampaignDebugEvidence {
	param(
		[object] $Evidence,
		[object] $CandidateIdentity,
		[string] $Label,
		[DateTimeOffset] $StatusAsOfUtc,
		[int] $SourceSettingsSchema,
		[switch] $AllowUntrackedSummaryForSelfTest,
		[object] $TrustedToolBindingsForSelfTest,
		[string] $PortableEvidenceRoot
	)
	$externalRequiredAdvisoryContracts = [ordered] @{
		"isolation.world_scope" = [ordered] @{
			caseId = "cleanup.state_isolation_restore"
			category = "cleanup"
			feature = "campaign_debug"
			stage = "state_restore"
			expected = "runtime certification remains scoped to the disposable development session"
			actual = "world runtime, player inventory, health, and service caches require session restart before another certifying run"
			reason = "restart the disposable development session before another certification run"
		}
		"persistence.real_restart" = [ordered] @{
			caseId = "persistence.seeded_roundtrip.phase12"
			category = "persistence"
			feature = "persistence_smoke"
			stage = "early_phase"
			expected = "external process restart / reconnect remains an explicit later-gate scenario"
			actual = "non-certifying external advisory | restart/fault gate"
			reason = "run the immutable package through the external restart matrix before claiming restart certification"
		}
		"phase25.real_restart" = [ordered] @{
			caseId = "phase25.manual_external_gaps"
			category = "soak"
			feature = "external_harness"
			stage = "final"
			expected = "real restart-after-primitive remains an explicit later-gate external scenario"
			actual = "non-certifying external advisory | restart/fault gate"
			reason = "run the immutable package through the external restart matrix before claiming restart certification"
		}
		"phase25.second_client" = [ordered] @{
			caseId = "phase25.manual_external_gaps"
			category = "soak"
			feature = "external_harness"
			stage = "final"
			expected = "second-client join/reconnect remains an explicit later-gate external scenario"
			actual = "non-certifying external advisory | multiplayer/JIP gate"
			reason = "run the immutable package with the required clients before claiming multiplayer certification"
		}
		"phase25.two_hour_soak" = [ordered] @{
			caseId = "phase25.manual_external_gaps"
			category = "soak"
			feature = "external_harness"
			stage = "final"
			expected = "two-hour endurance remains an explicit later-gate external scenario"
			actual = "non-certifying external advisory | soak gate"
			reason = "run the immutable package for the required duration before claiming soak certification"
		}
	}
	$externalRequiredAdvisoryIds = @($externalRequiredAdvisoryContracts.Keys)
	$expectedPortableStatusFields = @(
		"status", "summaryPath", "summarySha256", "candidateId",
		"candidateSourceHead", "packageSha256", "manifestSha256", "readySha256",
		"settingsSha256", "harnessGitHead", "campaignRunnerSha256",
		"candidateModuleSha256", "runLeafId", "runId", "startedUtc",
		"completedUtc", "runtimeSeconds", "wrapperCaptureSuccess",
		"runtimeOutcomeSuccess", "error", "caseCount", "pass", "warn", "fail",
		"blocked", "skipped", "requiredAssertions", "provenAssertions",
		"failedAssertions", "blockedAssertions", "certificationPassed",
		"artifactSchemaValidationValid", "candidateBoundaryVerified", "mountPacked",
		"artifactsStable", "diagnosticClassificationValid", "hardDiagnosticFree",
		"hardDiagnosticCount", "scriptErrors", "engineErrors", "partisanErrors",
		"approvedStockDiagnosticCount", "approvedIntentionalDiagnosticCount",
		"unapprovedHardDiagnosticCount", "classifierSelfTestCount",
		"canonicalScriptLogCount", "canonicalConsoleLogCount",
		"canonicalLogPairSameDirectory", "stateDiffRows", "nonzeroStateDiffRows",
		"finalOrphanCleanupPass", "cleanupAndSpillZero", "envelopeFileCount",
		"envelopeFilesRehashed", "envelopeSha256", "runSummarySha256",
		"externalRequiredAdvisoryIds", "acceptanceDisposition")
	if ($null -eq $Evidence -or $Evidence -is [string] -or $Evidence -is [Array]) {
		throw "$Label portable status must be an object."
	}
	Assert-EqualSet $expectedPortableStatusFields `
		@($Evidence.PSObject.Properties.Name) "$Label portable status fields"

	$allowedStatuses = @(
		"passed-full-certification",
		"passed-internal-profile-external-required",
		"failed-full-profile")
	$statusValue = Require-Text `
		(Get-ObjectPropertyValue $Evidence "status") "$Label.status"
	if ($statusValue -cnotin $allowedStatuses) {
		throw "$Label.status is not a portable full-profile disposition."
	}

	$summaryPath = Require-RepoRelativePath `
		(Get-ObjectPropertyValue $Evidence "summaryPath") "$Label.summaryPath"
	$summarySha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "summarySha256") `
		"$Label.summarySha256").ToLowerInvariant()
	$repoPrefix = [IO.Path]::GetFullPath($root).TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
	$indexPath = [IO.Path]::GetFullPath((Join-Path $root $summaryPath))
	if (-not $indexPath.StartsWith($repoPrefix, [StringComparison]::OrdinalIgnoreCase) -or
		-not (Test-Path -LiteralPath $indexPath -PathType Leaf) -or
		(Split-Path -Leaf $indexPath) -cne "release-index.json") {
		throw "$Label portable release index is missing, outside the repository, or misnamed."
	}
	$indexItem = Get-Item -LiteralPath $indexPath -Force
	if (($indexItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
		throw "$Label portable release index must not be a reparse point."
	}
	$usePortableEvidenceRoot = -not [string]::IsNullOrWhiteSpace($PortableEvidenceRoot)
	if (-not $AllowUntrackedSummaryForSelfTest) {
		& git -C $root ls-files --error-unmatch -- $summaryPath *> $null
		if ($LASTEXITCODE -ne 0) {
			throw "$Label portable release index must be tracked."
		}
	}
	$actualIndexSha = (Get-FileHash -LiteralPath $indexPath -Algorithm SHA256).Hash.ToLowerInvariant()
	if ($actualIndexSha -cne $summarySha) {
		throw "$Label portable release-index SHA-256 does not match release status."
	}
	$indexText = Get-Content -Raw -LiteralPath $indexPath
	try {
		$index = $indexText | ConvertFrom-Json
	}
	catch {
		throw "$Label portable release index is invalid JSON: $($_.Exception.Message)"
	}
	Assert-NoLocalAbsolutePathValue $index "$Label portable release index"
	if ((Require-IntegerProperty $index "schemaVersion" "$Label index") -ne 2 -or
		[string] (Get-ObjectPropertyValue $index "evidenceKind") -cne
			"packaged-campaign-debug-full-profile" -or
		[string] (Get-ObjectPropertyValue $index "policyId") -cne
			"partisan-campaign-debug-full-profile-v2") {
		throw "$Label portable release index uses an unsupported schema or policy."
	}

	$source = Get-ObjectPropertyValue $index "source"
	$indexCandidate = Get-ObjectPropertyValue $index "candidate"
	$indexHarness = Get-ObjectPropertyValue $index "harness"
	$indexSettings = Get-ObjectPropertyValue $index "settings"
	$capture = Get-ObjectPropertyValue $index "capture"
	$result = Get-ObjectPropertyValue $index "result"
	$proof = Get-ObjectPropertyValue $index "proof"
	$diagnostics = Get-ObjectPropertyValue $index "diagnostics"
	$indexCleanup = Get-ObjectPropertyValue $index "cleanup"
	$integrity = Get-ObjectPropertyValue $index "integrity"
	$finding = Get-ObjectPropertyValue $index "finding"
	if ($null -eq $source -or $null -eq $indexCandidate -or
		$null -eq $indexHarness -or $null -eq $indexSettings -or
		$null -eq $capture -or $null -eq $result -or $null -eq $proof -or
		$null -eq $diagnostics -or $null -eq $indexCleanup -or
		$null -eq $integrity -or $null -eq $finding) {
		throw "$Label portable release index is structurally incomplete."
	}

	$trackedIndexRoot = Split-Path -Parent $indexPath
	$runLeafId = Split-Path -Leaf $trackedIndexRoot
	if ($runLeafId -cnotmatch '^\d{8}T\d{6}Z-[0-9a-f]{32}$' -or
		[string] (Get-ObjectPropertyValue $capture "runLeafId") -cne $runLeafId -or
		[string] (Get-ObjectPropertyValue $source "runEnvelopePath") -cne "run.json") {
		throw "$Label portable bundle/run-leaf identity is inconsistent."
	}
	$bundleRoot = $trackedIndexRoot
	$externalIndexPath = $indexPath
	if (-not $AllowUntrackedSummaryForSelfTest -or $usePortableEvidenceRoot) {
		if ([string]::IsNullOrWhiteSpace($PortableEvidenceRoot) -or
			-not (Test-Path -LiteralPath $PortableEvidenceRoot -PathType Container)) {
			throw "$Label requires the portable evidence-bundle root to re-open retained raw evidence."
		}
		$evidenceRootPath = [IO.Path]::GetFullPath($PortableEvidenceRoot).TrimEnd(
			[IO.Path]::DirectorySeparatorChar,
			[IO.Path]::AltDirectorySeparatorChar)
		$evidenceRootItem = Get-Item -LiteralPath $evidenceRootPath -Force
		if (($evidenceRootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
			throw "$Label portable evidence-bundle root must not be a reparse point."
		}
		$bundleRelativePath = Require-RepoRelativePath `
			(Get-ObjectPropertyValue $source "bundleRelativePath") `
			"$Label source.bundleRelativePath"
		$expectedBundleRelativePath = "{0}/campaign-debug/{1}" -f
			[string] $CandidateIdentity.CandidateId, $runLeafId
		if ($bundleRelativePath -cne $expectedBundleRelativePath) {
			throw "$Label portable bundle path differs from its candidate/run-leaf identity."
		}
		$evidencePrefix = $evidenceRootPath + [IO.Path]::DirectorySeparatorChar
		$bundleRoot = [IO.Path]::GetFullPath(
			(Join-Path $evidenceRootPath $bundleRelativePath))
		if (-not $bundleRoot.StartsWith(
				$evidencePrefix,
				[StringComparison]::OrdinalIgnoreCase) -or
			-not (Test-Path -LiteralPath $bundleRoot -PathType Container)) {
			throw "$Label portable raw bundle is missing or escapes its supplied root."
		}
		$bundlePathCursor = $evidenceRootPath
		foreach ($bundlePathSegment in $bundleRelativePath.Split('/')) {
			$bundlePathCursor = Join-Path $bundlePathCursor $bundlePathSegment
			$bundlePathItem = Get-Item -LiteralPath $bundlePathCursor -Force
			if (($bundlePathItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
				throw "$Label portable raw bundle path must not traverse a reparse point."
			}
		}
		$externalIndexPath = Join-Path $bundleRoot "release-index.json"
		if (-not (Test-Path -LiteralPath $externalIndexPath -PathType Leaf) -or
			(Get-FileHash -LiteralPath $externalIndexPath -Algorithm SHA256).Hash.ToLowerInvariant() -cne
				$summarySha) {
			throw "$Label tracked release index differs from the retained portable-bundle index."
		}
	}
	$runPath = Join-Path $bundleRoot "run.json"
	if (-not (Test-Path -LiteralPath $runPath -PathType Leaf)) {
		throw "$Label retained authoritative run.json is missing."
	}
	$runItem = Get-Item -LiteralPath $runPath -Force
	if (($runItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
		throw "$Label retained run.json must not be a reparse point."
	}
	$runSha = (Get-FileHash -LiteralPath $runPath -Algorithm SHA256).Hash.ToLowerInvariant()
	if ($runSha -cne [string] (Get-ObjectPropertyValue $source "runEnvelopeSha256") -or
		$runSha -cne [string] (Get-ObjectPropertyValue $integrity "envelopeSha256") -or
		$runSha -cne [string] (Get-ObjectPropertyValue $integrity "runSummarySha256") -or
		$runSha -cne [string] (Get-ObjectPropertyValue $Evidence "envelopeSha256") -or
		$runSha -cne [string] (Get-ObjectPropertyValue $Evidence "runSummarySha256")) {
		throw "$Label retained run-envelope hashes differ."
	}
	$runText = Get-Content -Raw -LiteralPath $runPath
	try {
		$run = $runText | ConvertFrom-Json
	}
	catch {
		throw "$Label retained run envelope is invalid JSON: $($_.Exception.Message)"
	}
	Assert-NoLocalAbsolutePathValue $run "$Label retained run envelope"
	if ((Require-IntegerProperty $run "schemaVersion" "$Label run envelope") -ne 2 -or
		[string] (Get-ObjectPropertyValue $run "evidenceKind") -cne
			"packaged-campaign-debug") {
		throw "$Label retained run envelope uses an unsupported schema."
	}

	$runCandidate = Get-ObjectPropertyValue $run "candidate"
	$runHarness = Get-ObjectPropertyValue $run "harness"
	$runLaunch = Get-ObjectPropertyValue $run "launch"
	$runOutcome = Get-ObjectPropertyValue $run "outcome"
	$runSettings = Get-ObjectPropertyValue $run "settings"
	$runCleanup = Get-ObjectPropertyValue $run "cleanup"
	$semanticHarnessHead = Require-Text `
		(Get-ObjectPropertyValue $runHarness "gitHead") "$Label semantic harness Git HEAD"
	$semanticRunnerBlobSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $runHarness "campaignRunnerGitBlobSha256") `
		"$Label semantic runner Git-blob SHA-256").ToLowerInvariant()
	if ($semanticHarnessHead -cnotmatch '^[0-9a-f]{40}$' -or
		[string] (Get-ObjectPropertyValue $indexHarness "gitHead") -cne
			$semanticHarnessHead -or
		[string] (Get-ObjectPropertyValue $indexHarness `
			"campaignRunnerGitBlobSha256") -cne $semanticRunnerBlobSha -or
		(Get-ObjectPropertyValue $runHarness "dirty") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $runHarness "dirty")) {
		throw "$Label semantic runner is not bound to a clean immutable harness revision."
	}
	$runnerSourceText = ''
	if ($AllowUntrackedSummaryForSelfTest) {
		if ($null -eq $TrustedToolBindingsForSelfTest -or
			[string] (Get-ObjectPropertyValue $TrustedToolBindingsForSelfTest "gitHead") -cne
				$semanticHarnessHead -or
			[string] (Get-ObjectPropertyValue $TrustedToolBindingsForSelfTest `
				"campaignRunnerGitBlobSha256") -cne $semanticRunnerBlobSha) {
			throw "$Label self-test semantic runner differs from its trusted tool bundle."
		}
		$runnerSourceText = Get-Content -Raw -LiteralPath `
			(Join-Path $PSScriptRoot 'run-guarded-campaign-debug.ps1')
	}
	else {
		$runnerBlob = Get-GitBlobTextAndSha256 `
			$semanticHarnessHead 'tools/run-guarded-campaign-debug.ps1' `
			"$Label semantic guarded runner"
		if ([string] $runnerBlob.Sha256 -cne $semanticRunnerBlobSha) {
			throw "$Label semantic runner blob differs from run.json."
		}
		$runnerSourceText = [string] $runnerBlob.Text
	}
	$runFiles = @(Get-ObjectPropertyValue $run "files")
	$indexFiles = @(Get-ObjectPropertyValue $source "files")
	if ($runFiles.Count -ne 10 -or $indexFiles.Count -ne $runFiles.Count -or
		[int] (Get-ObjectPropertyValue $source "fileCount") -ne $runFiles.Count -or
		[int] (Get-ObjectPropertyValue $integrity "envelopeFileCount") -ne
			$runFiles.Count -or
		[int] (Get-ObjectPropertyValue $Evidence "envelopeFileCount") -ne
			$runFiles.Count) {
		throw "$Label portable bundle does not retain the canonical ten-file raw inventory."
	}

	$rowPaths = @()
	$rowMap = @{}
	for ($rowIndex = 0; $rowIndex -lt $runFiles.Count; $rowIndex++) {
		$row = $runFiles[$rowIndex]
		$indexRow = $indexFiles[$rowIndex]
		$path = Require-RepoRelativePath `
			(Get-ObjectPropertyValue $row "path") "$Label run.files[$rowIndex].path"
		if ($rowMap.ContainsKey($path)) {
			throw "$Label run file inventory repeats $path."
		}
		$length = Require-IntegerProperty $row "length" "$Label run.files[$rowIndex]"
		$sha = (Require-Sha256 `
			(Get-ObjectPropertyValue $row "sha256") `
			"$Label run.files[$rowIndex].sha256").ToLowerInvariant()
		if ([string] (Get-ObjectPropertyValue $indexRow "path") -cne $path -or
			[long] (Get-ObjectPropertyValue $indexRow "length") -ne $length -or
			[string] (Get-ObjectPropertyValue $indexRow "sha256") -cne $sha) {
			throw "$Label release-index inventory row $path differs from run.json."
		}
		$fullPath = [IO.Path]::GetFullPath((Join-Path $bundleRoot $path))
		$bundlePrefix = [IO.Path]::GetFullPath($bundleRoot).TrimEnd('\', '/') +
			[IO.Path]::DirectorySeparatorChar
		if (-not $fullPath.StartsWith($bundlePrefix, [StringComparison]::OrdinalIgnoreCase) -or
			-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
			throw "$Label retained raw file $path is missing or escapes its bundle."
		}
		$item = Get-Item -LiteralPath $fullPath -Force
		if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
			[long] $item.Length -ne $length -or
			(Get-FileHash -LiteralPath $fullPath -Algorithm SHA256).Hash.ToLowerInvariant() -cne
				$sha) {
			throw "$Label retained raw file $path fails its independent hash/length check."
		}
		$rowPaths += $path
		$rowMap[$path] = [PSCustomObject] @{ FullPath = $fullPath; Sha256 = $sha }
	}
	$actualBundleEntries = @(Get-ChildItem -LiteralPath $bundleRoot -Recurse -Force)
	if (@($actualBundleEntries | Where-Object {
				($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0
			}).Count -ne 0) {
		throw "$Label retained raw bundle must not contain reparse points."
	}
	$actualBundleRows = @($actualBundleEntries | Where-Object { -not $_.PSIsContainer } |
		Where-Object {
			$_.FullName -cne $runPath -and $_.FullName -cne $externalIndexPath
		} |
		ForEach-Object {
			$_.FullName.Substring($bundleRoot.TrimEnd('\', '/').Length + 1).Replace('\', '/')
		})
	Assert-EqualSet $rowPaths $actualBundleRows "$Label run inventory/raw bundle"

	$artifactPaths = @($rowPaths | Where-Object {
		$_ -cmatch '^raw/campaign-debug/HST_CampaignDebug_[a-zA-Z0-9_]+\.json$'
	})
	$diffPaths = @($rowPaths | Where-Object { $_ -cmatch '_state_diff\.txt$' })
	$summaryTextPaths = @($rowPaths | Where-Object { $_ -cmatch '_summary\.txt$' })
	$settingsPaths = @($rowPaths | Where-Object { $_ -ceq "config/HST_Settings.json" })
	$manifestPaths = @($rowPaths | Where-Object { $_ -ceq "identity/candidate.json" })
	$readyPaths = @($rowPaths | Where-Object { $_ -ceq "identity/candidate.ready.json" })
	$consolePaths = @($rowPaths | Where-Object { $_ -cmatch '^raw/logs/[^/]+/console\.log$' })
	$scriptPaths = @($rowPaths | Where-Object { $_ -cmatch '^raw/logs/[^/]+/script\.log$' })
	$errorPaths = @($rowPaths | Where-Object { $_ -cmatch '^raw/logs/[^/]+/error\.log$' })
	$crashPaths = @($rowPaths | Where-Object { $_ -cmatch '^raw/logs/[^/]+/crash\.log$' })
	foreach ($paths in @(
			$artifactPaths, $diffPaths, $summaryTextPaths, $settingsPaths,
			$manifestPaths, $readyPaths, $consolePaths, $scriptPaths,
			$errorPaths, $crashPaths)) {
		if (@($paths).Count -ne 1) {
			throw "$Label raw bundle is missing or duplicates a canonical retained file role."
		}
	}
	$rawArtifactPath = $artifactPaths[0]
	if ([string] (Get-ObjectPropertyValue $source "rawArtifactPath") -cne $rawArtifactPath -or
		[string] (Get-ObjectPropertyValue $source "rawArtifactSha256") -cne
			$rowMap[$rawArtifactPath].Sha256 -or
		[string] (Get-ObjectPropertyValue $integrity "rawArtifactSha256") -cne
			$rowMap[$rawArtifactPath].Sha256) {
		throw "$Label raw full-profile artifact binding differs."
	}
	if ([string] (Get-ObjectPropertyValue $source "stateDiffPath") -cne $diffPaths[0] -or
		[string] (Get-ObjectPropertyValue $source "stateDiffSha256") -cne
			$rowMap[$diffPaths[0]].Sha256 -or
		[string] (Get-ObjectPropertyValue $source "textSummaryPath") -cne
			$summaryTextPaths[0] -or
		[string] (Get-ObjectPropertyValue $source "textSummarySha256") -cne
			$rowMap[$summaryTextPaths[0]].Sha256 -or
		[string] (Get-ObjectPropertyValue $runCandidate "manifestSha256") -cne
			$rowMap[$manifestPaths[0]].Sha256 -or
		[string] (Get-ObjectPropertyValue $runCandidate "readySha256") -cne
			$rowMap[$readyPaths[0]].Sha256 -or
		[string] (Get-ObjectPropertyValue $runSettings "sha256") -cne
			$rowMap[$settingsPaths[0]].Sha256) {
		throw "$Label canonical retained file hashes differ from run.json or the release index."
	}
	try {
		$bundleManifest = Get-Content -Raw -LiteralPath $rowMap[$manifestPaths[0]].FullPath |
			ConvertFrom-Json
		$bundleReady = Get-Content -Raw -LiteralPath $rowMap[$readyPaths[0]].FullPath |
			ConvertFrom-Json
	}
	catch {
		throw "$Label retained candidate manifest or ready seal is invalid JSON: $($_.Exception.Message)"
	}
	Assert-NoLocalAbsolutePathValue $bundleManifest "$Label retained candidate manifest"
	Assert-NoLocalAbsolutePathValue $bundleReady "$Label retained candidate ready seal"
	$bundleManifestCandidate = Get-ObjectPropertyValue $bundleManifest "candidate"
	$bundleManifestSource = Get-ObjectPropertyValue $bundleManifest "source"
	$bundleManifestEmbedded = Get-ObjectPropertyValue `
		$bundleManifestSource "embeddedImplementation"
	$bundleManifestAddon = Get-ObjectPropertyValue $bundleManifest "addon"
	$bundleManifestToolchain = Get-ObjectPropertyValue $bundleManifest "toolchain"
	$bundleManifestWorkbench = Get-ObjectPropertyValue $bundleManifest "workbench"
	$bundleManifestPackage = Get-ObjectPropertyValue $bundleManifest "package"
	$trustedManifest = Get-ObjectPropertyValue $CandidateIdentity "Manifest"
	$trustedManifestCandidate = Get-ObjectPropertyValue $trustedManifest "candidate"
	$trustedManifestSource = Get-ObjectPropertyValue $trustedManifest "source"
	$trustedManifestEmbedded = Get-ObjectPropertyValue `
		$trustedManifestSource "embeddedImplementation"
	$trustedManifestAddon = Get-ObjectPropertyValue $trustedManifest "addon"
	$trustedManifestToolchain = Get-ObjectPropertyValue $trustedManifest "toolchain"
	$trustedManifestWorkbench = Get-ObjectPropertyValue $trustedManifest "workbench"
	$trustedManifestPackage = Get-ObjectPropertyValue $trustedManifest "package"
	$bundleCandidateVersion = Require-Text `
		(Get-ObjectPropertyValue $bundleManifestCandidate "version") `
		"$Label retained manifest candidate.version"
	$bundleEmbeddedBuildSha = Require-Text `
		(Get-ObjectPropertyValue $bundleManifestEmbedded "sha") `
		"$Label retained manifest embedded build SHA"
	$bundleEmbeddedBuildUtc = Require-Text `
		(Get-ObjectPropertyValue $bundleManifestEmbedded "utc") `
		"$Label retained manifest embedded build UTC"
	$bundleEmbeddedBuildLabel = Require-Text `
		(Get-ObjectPropertyValue $bundleManifestEmbedded "label") `
		"$Label retained manifest embedded build label"
	$bundleCampaignSchema = Require-IntegerProperty `
		$bundleManifestSource "campaignSchema" "$Label retained manifest source"
	$bundleRuntimeSettingsSchema = Require-IntegerProperty `
		$bundleManifestSource "runtimeSettingsSchema" "$Label retained manifest source"
	$bundleAddonId = Require-Text `
		(Get-ObjectPropertyValue $bundleManifestAddon "id") `
		"$Label retained manifest add-on ID"
	$bundleAddonGuid = Require-Text `
		(Get-ObjectPropertyValue $bundleManifestAddon "guid") `
		"$Label retained manifest add-on GUID"
	$bundlePackageHashAlgorithm = Require-Text `
		(Get-ObjectPropertyValue $bundleManifestPackage "hashAlgorithm") `
		"$Label retained manifest package hash algorithm"
	$bundlePackageSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $bundleManifestPackage "sha256") `
		"$Label retained manifest package SHA-256").ToLowerInvariant()
	$bundleWorkbenchCrc = Require-Text `
		(Get-ObjectPropertyValue $bundleManifestWorkbench "crc") `
		"$Label retained manifest Workbench CRC"
	$bundleManifestClient = Get-ObjectPropertyValue $bundleManifestToolchain "client"
	$bundleManifestClientDiagnostic = Get-ObjectPropertyValue `
		$bundleManifestToolchain "clientDiagnostic"
	$trustedManifestClient = Get-ObjectPropertyValue $trustedManifestToolchain "client"
	$trustedManifestClientDiagnostic = Get-ObjectPropertyValue `
		$trustedManifestToolchain "clientDiagnostic"
	if ((Require-IntegerProperty $bundleManifest "manifestSchemaVersion" `
			"$Label retained manifest") -ne 1 -or
		[string] (Get-ObjectPropertyValue $bundleManifestCandidate "id") -cne
			[string] $CandidateIdentity.CandidateId -or
		[string] (Get-ObjectPropertyValue $bundleManifestAddon "version") -cne
			$bundleCandidateVersion -or
		[string] (Get-ObjectPropertyValue $bundleManifestSource "gitHead") -cne
			[string] $CandidateIdentity.CandidateSourceHead -or
		$bundleCandidateVersion -cne [string] $CandidateIdentity.PackageVersion -or
		$bundleCampaignSchema -ne [int] $CandidateIdentity.CampaignSchema -or
		$bundleRuntimeSettingsSchema -ne [int] $CandidateIdentity.RuntimeSettingsSchema -or
		$bundlePackageHashAlgorithm -cne [string] $CandidateIdentity.PackageHashAlgorithm -or
		$bundlePackageHashAlgorithm -cne "sha256-manifest-v1" -or
		$bundlePackageSha -cne [string] $CandidateIdentity.PackageSha256 -or
		$bundleWorkbenchCrc -cne [string] $CandidateIdentity.WorkbenchCrc -or
		$bundleAddonGuid -cnotmatch '^[0-9A-F]{16}$' -or
		$bundleCampaignSchema -le 0 -or $bundleRuntimeSettingsSchema -le 0 -or
		[string] (Get-ObjectPropertyValue $trustedManifestCandidate "id") -cne
			[string] $CandidateIdentity.CandidateId -or
		[string] (Get-ObjectPropertyValue $trustedManifestCandidate "version") -cne
			$bundleCandidateVersion -or
		[string] (Get-ObjectPropertyValue $trustedManifestSource "gitHead") -cne
			[string] $CandidateIdentity.CandidateSourceHead -or
		[string] (Get-ObjectPropertyValue $trustedManifestEmbedded "sha") -cne
			$bundleEmbeddedBuildSha -or
		[string] (Get-ObjectPropertyValue $trustedManifestEmbedded "utc") -cne
			$bundleEmbeddedBuildUtc -or
		[string] (Get-ObjectPropertyValue $trustedManifestEmbedded "label") -cne
			$bundleEmbeddedBuildLabel -or
		(Require-IntegerProperty $trustedManifestSource "campaignSchema" `
			"$Label trusted manifest source") -ne $bundleCampaignSchema -or
		(Require-IntegerProperty $trustedManifestSource "runtimeSettingsSchema" `
			"$Label trusted manifest source") -ne $bundleRuntimeSettingsSchema -or
		[string] (Get-ObjectPropertyValue $trustedManifestAddon "id") -cne $bundleAddonId -or
		[string] (Get-ObjectPropertyValue $trustedManifestAddon "guid") -cne $bundleAddonGuid -or
		[string] (Get-ObjectPropertyValue $trustedManifestPackage "hashAlgorithm") -cne
			$bundlePackageHashAlgorithm -or
		[string] (Get-ObjectPropertyValue $trustedManifestPackage "sha256") -cne
			$bundlePackageSha -or
		[string] (Get-ObjectPropertyValue $trustedManifestWorkbench "crc") -cne
			$bundleWorkbenchCrc) {
		throw "$Label retained bundle manifest differs from its trusted candidate identity."
	}
	if ((Require-IntegerProperty $bundleReady "schemaVersion" `
			"$Label retained ready seal") -ne 1 -or
		[string] (Get-ObjectPropertyValue $bundleReady "candidateId") -cne
			[string] $CandidateIdentity.CandidateId -or
		[string] (Get-ObjectPropertyValue $bundleReady "gitHead") -cne
			[string] $CandidateIdentity.CandidateSourceHead -or
		[string] (Get-ObjectPropertyValue $bundleReady "packageSha256") -cne
			[string] $CandidateIdentity.PackageSha256 -or
		[string] (Get-ObjectPropertyValue $bundleReady "manifestSha256") -cne
			[string] $CandidateIdentity.ManifestSha256 -or
		$rowMap[$manifestPaths[0]].Sha256 -cne [string] $CandidateIdentity.ManifestSha256 -or
		$rowMap[$readyPaths[0]].Sha256 -cne [string] $CandidateIdentity.ReadySha256) {
		throw "$Label retained ready seal differs from its trusted candidate identity."
	}
	Assert-ExecutableIdentityEqual $trustedManifestClient $bundleManifestClient `
		"$Label retained client executable identity"
	Assert-ExecutableIdentityEqual $trustedManifestClientDiagnostic `
		$bundleManifestClientDiagnostic "$Label retained client diagnostic identity"
	try {
		$raw = Get-Content -Raw -LiteralPath $rowMap[$rawArtifactPath].FullPath |
			ConvertFrom-Json
	}
	catch {
		throw "$Label raw full-profile artifact is invalid JSON: $($_.Exception.Message)"
	}
	$runId = Require-Text (Get-ObjectPropertyValue $raw "m_sRunId") "$Label raw run ID"
	if ($runId -cnotmatch '^seed\d+_t\d+_p\d+_u\d+$' -or
		$rawArtifactPath -cne "raw/campaign-debug/HST_CampaignDebug_$runId.json" -or
		[string] (Get-ObjectPropertyValue $capture "runId") -cne $runId -or
		[string] (Get-ObjectPropertyValue $runLaunch "profile") -cne "full_certification" -or
		[string] (Get-ObjectPropertyValue $runLaunch "proofScope") -cne
			"full_certification" -or
		[string] (Get-ObjectPropertyValue $raw "m_sProfile") -cne "full_certification") {
		throw "$Label retained raw/run/capture full-profile identity differs."
	}
	if ([string] (Get-ObjectPropertyValue $raw "m_sBuildSha") -cne
			[string] (Get-ObjectPropertyValue $runCandidate "embeddedBuildSha") -or
		[string] (Get-ObjectPropertyValue $raw "m_sBuildUtc") -cne
			[string] (Get-ObjectPropertyValue $runCandidate "embeddedBuildUtc") -or
		[string] (Get-ObjectPropertyValue $raw "m_sBuildLabel") -cne
			[string] (Get-ObjectPropertyValue $runCandidate "embeddedBuildLabel") -or
		[string] (Get-ObjectPropertyValue $runLaunch "packageSha256") -cne
			[string] (Get-ObjectPropertyValue $runCandidate "packageSha256") -or
		[string] (Get-ObjectPropertyValue $runLaunch "addonGuid") -cne
			[string] (Get-ObjectPropertyValue $runCandidate "addonGuid") -or
		(Get-ObjectPropertyValue $runLaunch "stagedPackage") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $runLaunch "stagedPackage")) {
		throw "$Label raw build provenance or staged launch binding is inconsistent."
	}

	$caseCounts = [ordered] @{ PASS = 0; WARN = 0; FAIL = 0; BLOCKED = 0; SKIPPED = 0 }
	$certCounts = [ordered] @{ PASS = 0; WARN = 0; FAIL = 0; BLOCKED = 0 }
	$blockedRows = New-Object Collections.Generic.List[object]
	$warningRows = New-Object Collections.Generic.List[object]
	$failIds = New-Object Collections.Generic.List[string]
	$warnIds = New-Object Collections.Generic.List[string]
	$skipIds = New-Object Collections.Generic.List[string]
	$cases = @(Get-ObjectPropertyValue $raw "m_aCases")
	$seenCaseIds = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
	foreach ($case in $cases) {
		$caseId = Require-Text (Get-ObjectPropertyValue $case "m_sCaseId") "$Label raw case ID"
		if (-not $seenCaseIds.Add($caseId)) {
			throw "$Label raw full profile duplicates case ID $caseId."
		}
		$caseStatus = Require-Text `
			(Get-ObjectPropertyValue $case "m_sStatus") "$Label raw case $caseId status"
		if ($caseStatus -cnotin @("PASS", "WARN", "FAIL", "BLOCKED", "SKIPPED")) {
			throw "$Label raw case $caseId has unsupported status $caseStatus."
		}
		$caseCounts[$caseStatus]++
		$derivedCaseStatus = "PASS"
		$derivedCaseSeverity = 0
		$caseHasBlockedAssertion = $false
		$caseHasSkippedAssertion = $false
		$seenAssertionIds = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
		foreach ($assertion in @(Get-ObjectPropertyValue $case "m_aAssertions")) {
			$assertionId = Require-Text `
				(Get-ObjectPropertyValue $assertion "m_sAssertionId") `
				"$Label raw assertion ID"
			if (-not $seenAssertionIds.Add($assertionId)) {
				throw "$Label raw case $caseId duplicates assertion ID $assertionId."
			}
			$assertionStatus = Require-Text `
				(Get-ObjectPropertyValue $assertion "m_sStatus") `
				"$Label raw assertion $assertionId status"
			if ($assertionStatus -cnotin @("PASS", "WARN", "FAIL", "BLOCKED", "SKIPPED")) {
				throw "$Label raw assertion $assertionId has unsupported status $assertionStatus."
			}
			$assertionSeverity = switch ($assertionStatus) {
				"SKIPPED" { 1 }
				"WARN" { 2 }
				"BLOCKED" { 3 }
				"FAIL" { 4 }
				default { 0 }
			}
			if ($assertionSeverity -gt $derivedCaseSeverity) {
				$derivedCaseSeverity = $assertionSeverity
				$derivedCaseStatus = $assertionStatus
			}
			$counts = Get-ObjectPropertyValue $assertion "m_bCountsTowardCertification"
			if ($counts -isnot [bool]) {
				throw "$Label raw assertion $assertionId certification flag must be Boolean."
			}
			if ([bool] $counts) {
				if (-not $certCounts.Contains($assertionStatus)) {
					throw "$Label certification assertion $assertionId has unsupported status."
				}
				$certCounts[$assertionStatus]++
			}
			if ($assertionStatus -ceq "FAIL") {
				[void] $failIds.Add($assertionId)
			}
			elseif ($assertionStatus -ceq "BLOCKED") {
				$caseHasBlockedAssertion = $true
				[void] $blockedRows.Add([PSCustomObject] [ordered] @{
					id = $assertionId
					caseId = $caseId
					category = [string] (Get-ObjectPropertyValue $case "m_sCategory")
					feature = [string] (Get-ObjectPropertyValue $case "m_sFeature")
					stage = [string] (Get-ObjectPropertyValue $case "m_sStage")
					expected = [string] (Get-ObjectPropertyValue $assertion "m_sExpected")
					actual = [string] (Get-ObjectPropertyValue $assertion "m_sActual")
					reason = [string] (Get-ObjectPropertyValue $assertion "m_sFailureReason")
					proofLevel = [string] (Get-ObjectPropertyValue $assertion "m_sProofLevel")
					observedPath = [string] (Get-ObjectPropertyValue $assertion "m_sObservedPath")
					requiredPath = [string] (Get-ObjectPropertyValue $assertion "m_sRequiredPath")
					countsTowardCertification = [bool] $counts
				})
			}
			elseif ($assertionStatus -ceq "WARN") {
				[void] $warnIds.Add($assertionId)
				[void] $warningRows.Add([PSCustomObject] [ordered] @{
					id = $assertionId
					caseId = $caseId
					category = [string] (Get-ObjectPropertyValue $case "m_sCategory")
					feature = [string] (Get-ObjectPropertyValue $case "m_sFeature")
					stage = [string] (Get-ObjectPropertyValue $case "m_sStage")
					expected = [string] (Get-ObjectPropertyValue $assertion "m_sExpected")
					actual = [string] (Get-ObjectPropertyValue $assertion "m_sActual")
					reason = [string] (Get-ObjectPropertyValue $assertion "m_sFailureReason")
					proofLevel = [string] (Get-ObjectPropertyValue $assertion "m_sProofLevel")
					observedPath = [string] (Get-ObjectPropertyValue $assertion "m_sObservedPath")
					requiredPath = [string] (Get-ObjectPropertyValue $assertion "m_sRequiredPath")
					countsTowardCertification = [bool] $counts
				})
			}
			elseif ($assertionStatus -ceq "SKIPPED") {
				$caseHasSkippedAssertion = $true
				[void] $skipIds.Add($assertionId)
			}
		}
		if ($caseStatus -cne $derivedCaseStatus) {
			throw "$Label raw case $caseId status $caseStatus differs from its assertion-derived $derivedCaseStatus disposition."
		}
		if ($caseStatus -ceq "BLOCKED" -and -not $caseHasBlockedAssertion) {
			throw "$Label raw blocked case $caseId has no linked BLOCKED assertion."
		}
		if ($caseStatus -ceq "SKIPPED" -and -not $caseHasSkippedAssertion) {
			throw "$Label raw skipped case $caseId has no linked SKIPPED assertion."
		}
	}
	$caseCount = $cases.Count
	$requiredAssertions = $certCounts.PASS + $certCounts.WARN +
		$certCounts.FAIL + $certCounts.BLOCKED
	$certificationPassed = $requiredAssertions -eq $certCounts.PASS -and
		$certCounts.WARN -eq 0 -and $certCounts.FAIL -eq 0 -and
		$certCounts.BLOCKED -eq 0
	$rawCountBindings = [ordered] @{
		m_iPassCount = $caseCounts.PASS
		m_iWarnCount = $caseCounts.WARN
		m_iFailCount = $caseCounts.FAIL
		m_iBlockedCount = $caseCounts.BLOCKED
		m_iSkippedCount = $caseCounts.SKIPPED
		m_iCertificationRequiredCount = $requiredAssertions
		m_iCertificationProvenCount = $certCounts.PASS
		m_iCertificationFailCount = $certCounts.FAIL
		m_iCertificationBlockedCount = $certCounts.BLOCKED
		m_iCertificationWarnCount = $certCounts.WARN
	}
	foreach ($binding in $rawCountBindings.GetEnumerator()) {
		if ((Require-IntegerProperty $raw $binding.Key `
				"$Label raw.$($binding.Key)") -ne [int] $binding.Value) {
			throw "$Label raw $($binding.Key) differs from its assertion/case census."
		}
	}
	if ((Get-ObjectPropertyValue $raw "m_bCertificationPassed") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $raw "m_bCertificationPassed") -ne
			$certificationPassed) {
		throw "$Label raw certification Boolean differs from its assertion census."
	}

	$semanticValidation = Invoke-BoundRunnerSemanticValidation `
		-RunnerSourceText $runnerSourceText `
		-JsonPath $rowMap[$rawArtifactPath].FullPath `
		-SummaryPath $rowMap[$summaryTextPaths[0]].FullPath `
		-StateDiffPath $rowMap[$diffPaths[0]].FullPath `
		-GuardRoot (Join-Path $bundleRoot 'raw') `
		-ExpectedSha $bundleEmbeddedBuildSha `
		-ExpectedUtc $bundleEmbeddedBuildUtc `
		-ExpectedLabel $bundleEmbeddedBuildLabel
	$derivedValidation = $semanticValidation.ArtifactValidation
	$derivedErrorCensus = $semanticValidation.ErrorCensus
	$recordedValidation = Get-ObjectPropertyValue $runOutcome "validation"
	$recordedValidationValue = Get-ObjectPropertyValue $recordedValidation "Valid"
	if ($recordedValidationValue -isnot [bool] -or
		[bool] $recordedValidationValue -ne [bool] $derivedValidation.Valid) {
		throw "$Label recorded artifact-validation disposition differs from semantic re-derivation."
	}
	Assert-EqualSet `
		@((Get-ObjectPropertyValue $recordedValidation "Problems")) `
		@($derivedValidation.Problems) `
		"$Label recorded/re-derived artifact-validation problems"
	$validation = $derivedValidation
	$artifactValidationValid = [bool] $derivedValidation.Valid
	if (
		[string] (Get-ObjectPropertyValue $validation "RunId") -cne $runId -or
		[string] (Get-ObjectPropertyValue $validation "Profile") -cne
			"full_certification" -or
		[string] (Get-ObjectPropertyValue $validation "ProofScope") -cne
			"full_certification" -or
		(Get-ObjectPropertyValue $validation "FullCertification") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $validation "FullCertification")) {
		throw "$Label re-derived guarded-runner artifact validation identity is inconsistent."
	}
	$validationBindings = [ordered] @{
		CaseCount = $caseCount; Pass = $caseCounts.PASS; Warn = $caseCounts.WARN
		Fail = $caseCounts.FAIL; Blocked = $caseCounts.BLOCKED
		Skipped = $caseCounts.SKIPPED; CertificationRequired = $requiredAssertions
		CertificationProven = $certCounts.PASS; CertificationFail = $certCounts.FAIL
		CertificationBlocked = $certCounts.BLOCKED; CertificationWarn = $certCounts.WARN
	}
	foreach ($binding in $validationBindings.GetEnumerator()) {
		if ((Require-IntegerProperty $validation $binding.Key `
				"$Label runner validation.$($binding.Key)") -ne
			[int] $binding.Value) {
			throw "$Label runner validation $($binding.Key) differs from raw JSON."
		}
	}
	$indexProofBindings = [ordered] @{
		caseCount = $caseCount; pass = $caseCounts.PASS; warn = $caseCounts.WARN
		fail = $caseCounts.FAIL; blocked = $caseCounts.BLOCKED
		skipped = $caseCounts.SKIPPED; certificationRequired = $requiredAssertions
		certificationProven = $certCounts.PASS; certificationFail = $certCounts.FAIL
		certificationBlocked = $certCounts.BLOCKED; certificationWarn = $certCounts.WARN
	}
	foreach ($binding in $indexProofBindings.GetEnumerator()) {
		if ((Require-IntegerProperty $proof $binding.Key `
				"$Label release-index proof.$($binding.Key)") -ne
			[int] $binding.Value) {
			throw "$Label release-index proof $($binding.Key) differs from raw JSON."
		}
	}
	if ((Get-ObjectPropertyValue $result "certificationPassed") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $result "certificationPassed") -ne
			$certificationPassed) {
		throw "$Label release-index certification Boolean differs from raw JSON."
	}
	$proofBooleanBindings = [ordered] @{
		intentionalMissionConvoyAdmissionDiagnosticsProven =
			"IntentionalMissionConvoyAdmissionDiagnosticsProven"
		intentionalMissionConvoySettlementDiagnosticProven =
			"IntentionalMissionConvoySettlementDiagnosticProven"
		intentionalMissionConvoyCorruptionDiagnosticsProven =
			"IntentionalMissionConvoyCorruptionDiagnosticsProven"
		intentionalMissionConvoyWatchdogDiagnosticProven =
			"IntentionalMissionConvoyWatchdogDiagnosticProven"
		finalOrphanCleanupPass = "FinalOrphanCleanupPass"
	}
	foreach ($binding in $proofBooleanBindings.GetEnumerator()) {
		$proofValue = Get-ObjectPropertyValue $proof $binding.Key
		$validationValue = Get-ObjectPropertyValue $validation $binding.Value
		if ($proofValue -isnot [bool] -or $validationValue -isnot [bool] -or
			[bool] $proofValue -ne [bool] $validationValue) {
			throw "$Label release-index proof $($binding.Key) differs from runner validation."
		}
	}
	$rawStartedAtSecond = Require-IntegerProperty `
		$raw "m_iStartedAtSecond" "$Label raw.m_iStartedAtSecond"
	$rawEndedAtSecond = Require-IntegerProperty `
		$raw "m_iEndedAtSecond" "$Label raw.m_iEndedAtSecond"
	if ((Require-IntegerProperty $proof "startedAtSecond" `
			"$Label release-index proof.startedAtSecond") -ne $rawStartedAtSecond -or
		(Require-IntegerProperty $proof "endedAtSecond" `
			"$Label release-index proof.endedAtSecond") -ne $rawEndedAtSecond) {
		throw "$Label release-index proof timing differs from raw JSON."
	}
	if ((Require-IntegerProperty $proof "finalOrphanActiveGroups" `
			"$Label release-index proof.finalOrphanActiveGroups") -ne
		(Require-IntegerScalar `
			(Get-ObjectPropertyValue $validation "FinalOrphanActiveGroups") `
			"$Label runner validation.FinalOrphanActiveGroups")) {
		throw "$Label release-index final orphan count differs from runner validation."
	}

	$diffText = Get-Content -Raw -LiteralPath $rowMap[$diffPaths[0]].FullPath
	$deltaMatches = @([regex]::Matches(
		$diffText,
		'(?m)^.+\|\s*delta\s+(?<delta>-?\d+)\s*$'))
	$nonzeroDeltaCount = @($deltaMatches | Where-Object {
		[int64] $_.Groups['delta'].Value -ne 0
	}).Count
	$validatedStateDiffRows = Require-IntegerProperty `
		$validation "StateDiffRows" "$Label runner validation.StateDiffRows"
	$validatedNonzeroStateDiffRows = Require-IntegerProperty `
		$validation "NonzeroStateDiffRows" "$Label runner validation.NonzeroStateDiffRows"
	if ($deltaMatches.Count -ne $validatedStateDiffRows -or
		$nonzeroDeltaCount -ne
			$validatedNonzeroStateDiffRows) {
		throw "$Label retained state diff differs from the runner census."
	}
	if ((Require-IntegerProperty $proof "stateDiffRows" `
			"$Label release-index proof.stateDiffRows") -ne $deltaMatches.Count -or
		(Require-IntegerProperty $proof "nonzeroStateDiffRows" `
			"$Label release-index proof.nonzeroStateDiffRows") -ne $nonzeroDeltaCount) {
		throw "$Label release-index state-diff census differs from retained raw evidence."
	}

	$derivedBlockedJson = ConvertTo-Json -InputObject @($blockedRows.ToArray()) `
		-Depth 12 -Compress
	$indexBlockedJson = ConvertTo-Json -InputObject `
		@((Get-ObjectPropertyValue $proof "blockedAssertions")) -Depth 12 -Compress
	if ($derivedBlockedJson -cne $indexBlockedJson) {
		throw "$Label release index did not derive its blocked assertion linkage from raw JSON."
	}
	$derivedExternalAdvisoryRows = @($warningRows | Where-Object {
		$externalRequiredAdvisoryIds -ccontains $_.id
	})
	$derivedExternalAdvisoryIds = @($derivedExternalAdvisoryRows |
		ForEach-Object { $_.id })
	if (@($derivedExternalAdvisoryIds | Group-Object -CaseSensitive |
			Where-Object Count -ne 1).Count -ne 0) {
		throw "$Label raw full profile duplicates an external-required advisory assertion."
	}
	$indexExternalAdvisoryIds = @((Get-ObjectPropertyValue `
		$proof "externalRequiredAdvisoryIds"))
	Assert-EqualSet $derivedExternalAdvisoryIds $indexExternalAdvisoryIds `
		"$Label raw/index external-required advisory IDs"
	$derivedExternalAdvisoryJson = ConvertTo-Json -InputObject `
		@($derivedExternalAdvisoryRows) -Depth 12 -Compress
	$indexExternalAdvisoryJson = ConvertTo-Json -InputObject `
		@((Get-ObjectPropertyValue $proof "externalRequiredAdvisories")) `
		-Depth 12 -Compress
	if ($derivedExternalAdvisoryJson -cne $indexExternalAdvisoryJson) {
		throw "$Label release index did not derive its external advisory linkage from raw JSON."
	}
	$externalAdvisoryLinkageValid = $true
	foreach ($advisoryRow in $derivedExternalAdvisoryRows) {
		$contract = $externalRequiredAdvisoryContracts[$advisoryRow.id]
		if ($advisoryRow.caseId -cne $contract.caseId -or
			$advisoryRow.category -cne $contract.category -or
			$advisoryRow.feature -cne $contract.feature -or
			$advisoryRow.stage -cne $contract.stage -or
			$advisoryRow.expected -cne $contract.expected -or
			$advisoryRow.actual -cne $contract.actual -or
			$advisoryRow.reason -cne $contract.reason -or
			$advisoryRow.proofLevel -cne "EXTERNAL_PROCESS" -or
			$advisoryRow.observedPath -cne "manual_external_gap" -or
			$advisoryRow.requiredPath -cne
				"external process restart, reconnect, or long-soak harness" -or
			$advisoryRow.countsTowardCertification) {
			$externalAdvisoryLinkageValid = $false
		}
	}
	$unsupportedWarningIds = @($warnIds | Where-Object {
		$externalRequiredAdvisoryIds -cnotcontains $_
	})
	$approvedSkipIds = @(
		"phase24.escalation.support_physicalization",
		"phase24.escalation.group_physicalization")
	$unsupportedSkipIds = @($skipIds | Where-Object { $_ -cnotin $approvedSkipIds })
	if (@($skipIds | Group-Object -CaseSensitive | Where-Object Count -ne 1).Count -ne 0) {
		$unsupportedSkipIds += "<duplicate-skip-id>"
	}
	Assert-EqualSet $warnIds.ToArray() `
		@((Get-ObjectPropertyValue $proof "warningAssertionIds")) `
		"$Label raw/index warning assertion IDs"
	Assert-EqualSet $unsupportedWarningIds `
		@((Get-ObjectPropertyValue $proof "unsupportedWarningAssertionIds")) `
		"$Label raw/index unsupported warning assertion IDs"
	Assert-EqualSet $failIds.ToArray() `
		@((Get-ObjectPropertyValue $proof "failedAssertionIds")) `
		"$Label raw/index failed assertion IDs"
	Assert-EqualSet $skipIds.ToArray() `
		@((Get-ObjectPropertyValue $proof "skippedAssertionIds")) `
		"$Label raw/index skipped assertion IDs"
	Assert-EqualSet @($skipIds | Where-Object { $_ -cin $approvedSkipIds }) `
		@((Get-ObjectPropertyValue $proof "approvedNoncertifyingSkipIds")) `
		"$Label raw/index approved noncertifying skipped assertion IDs"
	Assert-EqualSet $unsupportedSkipIds `
		@((Get-ObjectPropertyValue $proof "unsupportedSkippedAssertionIds")) `
		"$Label raw/index unsupported skipped assertion IDs"

	$recordedErrorCensus = Get-ObjectPropertyValue $runOutcome "errorCensus"
	foreach ($field in @(
			"HardDiagnosticCount", "ScriptErrors", "EngineErrors", "PartisanErrors",
			"CrashMarkers", "PartisanSeverityLineCount", "ApprovedStockDiagnosticCount",
			"ApprovedIntentionalDiagnosticCount", "MalformedHardDiagnosticCount",
			"UnapprovedHardDiagnosticCount", "CanonicalScriptLogCount",
			"CanonicalConsoleLogCount")) {
		if ((Require-IntegerProperty $recordedErrorCensus $field `
				"$Label recorded error census.$field") -ne
			(Require-IntegerProperty $derivedErrorCensus $field `
				"$Label re-derived error census.$field")) {
			throw "$Label recorded diagnostic census $field differs from retained logs."
		}
	}
	foreach ($field in @(
			"Valid", "HardDiagnosticFree", "ChannelArithmeticValid",
			"CategoryArithmeticValid", "LifecycleMarkersValid",
			"IdentityBaselinePairValid", "IntentionalFixtureStructureExact",
			"IntentionalFixtureSetValid", "CanonicalLogPairSameDirectory",
			"IntentionalMissionConvoyAdmissionDiagnosticsProven",
			"IntentionalMissionConvoySettlementDiagnosticProven",
			"IntentionalMissionConvoyCorruptionDiagnosticsProven",
			"IntentionalMissionConvoyWatchdogDiagnosticProven")) {
		$recordedValue = Get-ObjectPropertyValue $recordedErrorCensus $field
		$derivedValue = Get-ObjectPropertyValue $derivedErrorCensus $field
		if ($recordedValue -isnot [bool] -or $derivedValue -isnot [bool] -or
			[bool] $recordedValue -ne [bool] $derivedValue) {
			throw "$Label recorded diagnostic census $field differs from retained logs."
		}
	}
	$recordedKindRows = @(@(Get-ObjectPropertyValue $recordedErrorCensus `
		"UnapprovedHardDiagnosticKinds") | ForEach-Object {
			'{0}={1}' -f
				(Require-Text (Get-ObjectPropertyValue $_ "kind") `
					"$Label recorded diagnostic kind"),
				(Require-IntegerProperty $_ "count" "$Label recorded diagnostic kind")
	})
	$derivedKindRows = @(@(Get-ObjectPropertyValue $derivedErrorCensus `
		"UnapprovedHardDiagnosticKinds") | ForEach-Object {
			'{0}={1}' -f
				(Require-Text (Get-ObjectPropertyValue $_ "kind") `
					"$Label re-derived diagnostic kind"),
				(Require-IntegerProperty $_ "count" "$Label re-derived diagnostic kind")
	})
	Assert-EqualSet $recordedKindRows $derivedKindRows `
		"$Label recorded/re-derived diagnostic kinds"
	$errorCensus = $derivedErrorCensus
	$diagnosticMap = [ordered] @{
		hardDiagnosticCount = "HardDiagnosticCount"
		scriptErrors = "ScriptErrors"
		engineErrors = "EngineErrors"
		partisanErrors = "PartisanErrors"
		crashMarkers = "CrashMarkers"
		partisanSeverityLineCount = "PartisanSeverityLineCount"
		approvedStockDiagnosticCount = "ApprovedStockDiagnosticCount"
		approvedIntentionalDiagnosticCount = "ApprovedIntentionalDiagnosticCount"
		unapprovedHardDiagnosticCount = "UnapprovedHardDiagnosticCount"
		malformedHardDiagnosticCount = "MalformedHardDiagnosticCount"
		canonicalScriptLogCount = "CanonicalScriptLogCount"
		canonicalConsoleLogCount = "CanonicalConsoleLogCount"
	}
	foreach ($binding in $diagnosticMap.GetEnumerator()) {
		$indexDiagnosticValue = Require-IntegerProperty `
			$diagnostics $binding.Key "$Label index diagnostics.$($binding.Key)"
		$runDiagnosticValue = Require-IntegerProperty `
			$errorCensus $binding.Value "$Label run error census.$($binding.Value)"
		if ($indexDiagnosticValue -lt 0 -or $runDiagnosticValue -lt 0 -or
			$indexDiagnosticValue -ne $runDiagnosticValue) {
			throw "$Label index diagnostic $($binding.Key) differs from run.json."
		}
	}
	$diagnosticBooleanMap = [ordered] @{
		valid = "Valid"; classificationValid = "Valid"
		hardDiagnosticFree = "HardDiagnosticFree"
		channelArithmeticValid = "ChannelArithmeticValid"
		categoryArithmeticValid = "CategoryArithmeticValid"
		lifecycleMarkersValid = "LifecycleMarkersValid"
		identityBaselinePairValid = "IdentityBaselinePairValid"
		intentionalFixtureStructureExact = "IntentionalFixtureStructureExact"
		intentionalFixtureSetValid = "IntentionalFixtureSetValid"
		canonicalLogPairSameDirectory = "CanonicalLogPairSameDirectory"
	}
	foreach ($binding in $diagnosticBooleanMap.GetEnumerator()) {
		$indexValue = Get-ObjectPropertyValue $diagnostics $binding.Key
		$runValue = Get-ObjectPropertyValue $errorCensus $binding.Value
		if ($indexValue -isnot [bool] -or $runValue -isnot [bool] -or
			[bool] $indexValue -ne [bool] $runValue) {
			throw "$Label index diagnostic $($binding.Key) differs from run.json."
		}
	}
	foreach ($binding in @(
			@("IntentionalMissionConvoyAdmissionDiagnosticsProven",
				"intentionalMissionConvoyAdmissionDiagnosticsProven"),
			@("IntentionalMissionConvoySettlementDiagnosticProven",
				"intentionalMissionConvoySettlementDiagnosticProven"),
			@("IntentionalMissionConvoyCorruptionDiagnosticsProven",
				"intentionalMissionConvoyCorruptionDiagnosticsProven"),
			@("IntentionalMissionConvoyWatchdogDiagnosticProven",
				"intentionalMissionConvoyWatchdogDiagnosticProven"))) {
		$runValue = Get-ObjectPropertyValue $errorCensus $binding[0]
		$validationValue = Get-ObjectPropertyValue $validation $binding[0]
		$proofValue = Get-ObjectPropertyValue $proof $binding[1]
		if ($runValue -isnot [bool] -or $validationValue -isnot [bool] -or
			$proofValue -isnot [bool] -or
			[bool] $runValue -ne [bool] $validationValue -or
			[bool] $runValue -ne [bool] $proofValue) {
			throw "$Label intentional diagnostic proof differs between raw validation and census."
		}
	}
	$hardCount = [int] (Get-ObjectPropertyValue $diagnostics "hardDiagnosticCount")
	$scriptErrors = [int] (Get-ObjectPropertyValue $diagnostics "scriptErrors")
	$engineErrors = [int] (Get-ObjectPropertyValue $diagnostics "engineErrors")
	$stockCount = [int] (Get-ObjectPropertyValue $diagnostics "approvedStockDiagnosticCount")
	$intentionalCount = [int] (Get-ObjectPropertyValue $diagnostics "approvedIntentionalDiagnosticCount")
	$unapprovedCount = [int] (Get-ObjectPropertyValue $diagnostics "unapprovedHardDiagnosticCount")
	$indexKinds = @(Get-ObjectPropertyValue $diagnostics "unapprovedHardDiagnosticKinds")
	$runKinds = @(Get-ObjectPropertyValue $errorCensus "UnapprovedHardDiagnosticKinds")
	if (($indexKinds | ConvertTo-Json -Depth 8 -Compress) -cne
		($runKinds | ConvertTo-Json -Depth 8 -Compress)) {
		throw "$Label release-index diagnostic kinds differ from run.json."
	}
	$kindTotal = 0
	foreach ($kind in $indexKinds) {
		$kindCount = Require-IntegerProperty $kind "count" "$Label unapproved diagnostic kind"
		if ($kindCount -le 0) {
			throw "$Label unapproved diagnostic kind count must be positive."
		}
		$kindTotal += $kindCount
	}
	$channelArithmeticDerived = $hardCount -eq ($scriptErrors + $engineErrors)
	$categoryArithmeticDerived = $hardCount -eq
		($stockCount + $intentionalCount + $unapprovedCount)
	$kindArithmeticDerived = $kindTotal -eq $unapprovedCount
	$hardDiagnosticFreeDerived =
		[bool] (Get-ObjectPropertyValue $diagnostics "hardDiagnosticFree") -eq
		($hardCount -eq 0)
	$classifierCount = Require-IntegerProperty `
		$diagnostics "classifierSelfTestCount" "$Label diagnostics.classifierSelfTestCount"
	$runClassifierCount = Require-IntegerProperty `
		$runOutcome "hardDiagnosticClassifierChecks" `
		"$Label run outcome.hardDiagnosticClassifierChecks"
	if ($classifierCount -ne
		$runClassifierCount) {
		throw "$Label diagnostic classifier count differs from run.json."
	}
	$diagnosticAxisPassed =
		[bool] (Get-ObjectPropertyValue $diagnostics "valid") -and
		[bool] (Get-ObjectPropertyValue $diagnostics "classificationValid") -and
		[bool] (Get-ObjectPropertyValue $diagnostics "channelArithmeticValid") -and
		[bool] (Get-ObjectPropertyValue $diagnostics "categoryArithmeticValid") -and
		$channelArithmeticDerived -and $categoryArithmeticDerived -and
		$kindArithmeticDerived -and $hardDiagnosticFreeDerived -and
		[bool] (Get-ObjectPropertyValue $diagnostics "lifecycleMarkersValid") -and
		[bool] (Get-ObjectPropertyValue $diagnostics "identityBaselinePairValid") -and
		[bool] (Get-ObjectPropertyValue $diagnostics "intentionalFixtureStructureExact") -and
		[bool] (Get-ObjectPropertyValue $diagnostics "intentionalFixtureSetValid") -and
		[bool] (Get-ObjectPropertyValue $diagnostics "canonicalLogPairSameDirectory") -and
		[bool] (Get-ObjectPropertyValue $proof "intentionalMissionConvoyAdmissionDiagnosticsProven") -and
		[bool] (Get-ObjectPropertyValue $proof "intentionalMissionConvoySettlementDiagnosticProven") -and
		[bool] (Get-ObjectPropertyValue $proof "intentionalMissionConvoyCorruptionDiagnosticsProven") -and
		[bool] (Get-ObjectPropertyValue $proof "intentionalMissionConvoyWatchdogDiagnosticProven") -and
		[int] (Get-ObjectPropertyValue $diagnostics "crashMarkers") -eq 0 -and
		[int] (Get-ObjectPropertyValue $diagnostics "partisanSeverityLineCount") -eq 0 -and
		[int] (Get-ObjectPropertyValue $diagnostics "malformedHardDiagnosticCount") -eq 0 -and
		[int] (Get-ObjectPropertyValue $diagnostics "canonicalScriptLogCount") -eq 1 -and
		[int] (Get-ObjectPropertyValue $diagnostics "canonicalConsoleLogCount") -eq 1 -and
		$unapprovedCount -eq 0 -and $classifierCount -eq 36 -and
		$hardCount -eq 15 -and $scriptErrors -eq 15 -and $engineErrors -eq 0 -and
		[int] (Get-ObjectPropertyValue $diagnostics "partisanErrors") -eq 13 -and
		$stockCount -eq 2 -and $intentionalCount -eq 13

	$cleanupFields = @(
		"guardRemaining", "ownedProcessesRemaining", "newEngineProcessesRemaining",
		"unclaimedEngineProcessesObserved", "newDefaultEntriesRemaining",
		"modifiedDefaultFiles", "deletedDefaultEntries", "missingDefaultRoots",
		"externalSpillEntriesRemaining", "modifiedSpillFiles", "deletedSpillEntries",
		"missingSpillRoots", "cleanupPhaseErrorCount")
	foreach ($field in $cleanupFields) {
		if ((Require-IntegerProperty $runCleanup $field `
				"$Label run cleanup.$field") -ne 0 -or
			(Require-IntegerProperty $indexCleanup $field `
				"$Label index cleanup.$field") -ne 0) {
			throw "$Label cleanup field $field must be zero in run.json and the index."
		}
	}
	if (@((Get-ObjectPropertyValue $runCleanup "cleanupPhaseErrors")).Count -ne 0 -or
		(Get-ObjectPropertyValue $runCleanup "monitoringRootsAreDetectionOnly") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $runCleanup "monitoringRootsAreDetectionOnly") -or
		@((Get-ObjectPropertyValue $indexCleanup "cleanupPhaseErrors")).Count -ne 0 -or
		(Get-ObjectPropertyValue $indexCleanup "monitoringRootsAreDetectionOnly") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $indexCleanup "monitoringRootsAreDetectionOnly")) {
		throw "$Label run cleanup boundary is not clean."
	}

	$mount = Get-ObjectPropertyValue $runOutcome "mountAttestation"
	foreach ($field in @(
			"armed", "started", "completed", "candidateBoundaryVerified",
			"artifactsStable", "evidenceCaptured", "success")) {
		if ((Get-ObjectPropertyValue $runOutcome $field) -isnot [bool]) {
			throw "$Label run outcome.$field must be Boolean."
		}
	}
	foreach ($field in @("Valid", "Packed")) {
		if ((Get-ObjectPropertyValue $mount $field) -isnot [bool]) {
			throw "$Label run mount attestation.$field must be Boolean."
		}
	}
	$wrapperCaptureSuccess =
		[bool] (Get-ObjectPropertyValue $runOutcome "armed") -and
		[bool] (Get-ObjectPropertyValue $runOutcome "started") -and
		[bool] (Get-ObjectPropertyValue $runOutcome "completed") -and
		[bool] (Get-ObjectPropertyValue $runOutcome "candidateBoundaryVerified") -and
		[bool] (Get-ObjectPropertyValue $runOutcome "artifactsStable") -and
		[bool] (Get-ObjectPropertyValue $runOutcome "evidenceCaptured") -and
		[bool] (Get-ObjectPropertyValue $mount "Valid") -and
		[bool] (Get-ObjectPropertyValue $mount "Packed")
	if (-not $wrapperCaptureSuccess) {
		throw "$Label wrapper capture is structurally incomplete."
	}
	$guardedRunSucceeded = [bool] (Get-ObjectPropertyValue $runOutcome "success")
	$outcomeError = [string] (Get-ObjectPropertyValue $runOutcome "error")
	$resultBooleanBindings = [ordered] @{
		wrapperCaptureSuccess = $wrapperCaptureSuccess
		guardedRunSucceeded = $guardedRunSucceeded
		runtimeOutcomeSuccess = $guardedRunSucceeded
		armed = [bool] (Get-ObjectPropertyValue $runOutcome "armed")
		started = [bool] (Get-ObjectPropertyValue $runOutcome "started")
		completed = [bool] (Get-ObjectPropertyValue $runOutcome "completed")
		candidateBoundaryVerified = [bool] (Get-ObjectPropertyValue $runOutcome "candidateBoundaryVerified")
		mountPacked = [bool] (Get-ObjectPropertyValue $mount "Packed")
		artifactsStable = [bool] (Get-ObjectPropertyValue $runOutcome "artifactsStable")
		evidenceCaptured = [bool] (Get-ObjectPropertyValue $runOutcome "evidenceCaptured")
		artifactSchemaValidationValid = [bool] (Get-ObjectPropertyValue $validation "Valid")
		certificationPassed = $certificationPassed
	}
	foreach ($binding in $resultBooleanBindings.GetEnumerator()) {
		$value = Get-ObjectPropertyValue $result $binding.Key
		if ($value -isnot [bool] -or [bool] $value -ne [bool] $binding.Value) {
			throw "$Label release-index result.$($binding.Key) differs from run.json."
		}
	}
	if ([string] (Get-ObjectPropertyValue $result "error") -cne $outcomeError) {
		throw "$Label release-index result error differs from run.json."
	}
	$proofCommonPassed = $caseCounts.FAIL -eq 0 -and $caseCounts.BLOCKED -eq 0 -and
		$failIds.Count -eq 0 -and $blockedRows.Count -eq 0 -and
		$unsupportedWarningIds.Count -eq 0 -and
		$unsupportedSkipIds.Count -eq 0 -and
		$artifactValidationValid -and
		$certificationPassed -and $certCounts.FAIL -eq 0 -and
		$certCounts.BLOCKED -eq 0 -and $certCounts.WARN -eq 0 -and
		$nonzeroDeltaCount -eq 0 -and
		[bool] (Get-ObjectPropertyValue $proof "finalOrphanCleanupPass") -and
		[int] (Get-ObjectPropertyValue $proof "finalOrphanActiveGroups") -eq 0
	$acceptedFull = $proofCommonPassed -and $warnIds.Count -eq 0 -and
		$caseCounts.WARN -eq 0 -and $guardedRunSucceeded -and
		[string]::IsNullOrWhiteSpace($outcomeError) -and $diagnosticAxisPassed
	$acceptedInternal = $false
	if (-not $acceptedFull -and $proofCommonPassed -and
		$externalAdvisoryLinkageValid -and
		$guardedRunSucceeded -and [string]::IsNullOrWhiteSpace($outcomeError) -and
		$diagnosticAxisPassed) {
		try {
			Assert-EqualSet $externalRequiredAdvisoryIds `
				$derivedExternalAdvisoryIds `
				"$Label sealed external-required advisory set"
			$acceptedInternal = $caseCounts.WARN -gt 0
		}
		catch {
			$acceptedInternal = $false
		}
	}
	$derivedStatus = "failed-full-profile"
	$derivedAcceptance = "rejected-full-profile"
	$derivedRelease = "remain-no-go"
	$derivedFinding = "release-blocking-red-full-profile"
	if ($acceptedFull) {
		$derivedStatus = "passed-full-certification"
		$derivedAcceptance = "accepted-full-profile"
		$derivedRelease = "advance-external-gates"
		$derivedFinding = "accepted-full-profile"
	}
	elseif ($acceptedInternal) {
		$derivedStatus = "passed-internal-profile-external-required"
		$derivedAcceptance = "accepted-internal-profile"
		$derivedRelease = "advance-external-required"
		$derivedFinding = "accepted-internal-profile-external-required"
	}
	$derivedFindingDefect = "One or more full-profile acceptance axes failed."
	$derivedFindingNextStep =
		"Repair every retained proof or diagnostic rejection and seal a new immutable candidate."
	if ($acceptedFull) {
		$derivedFindingDefect = "none"
		$derivedFindingNextStep =
			"Advance the unchanged package to the external release gates."
	}
	elseif ($acceptedInternal) {
		$derivedFindingDefect = "none"
		$derivedFindingNextStep =
			"Run the exact retained external-required advisory set against the unchanged package."
	}
	if ($statusValue -cne $derivedStatus -or
		[string] (Get-ObjectPropertyValue $result "status") -cne $derivedStatus -or
		[string] (Get-ObjectPropertyValue $result "acceptanceDisposition") -cne
			$derivedAcceptance -or
		[string] (Get-ObjectPropertyValue $result "releaseDisposition") -cne
			$derivedRelease -or
		[string] (Get-ObjectPropertyValue $finding "status") -cne $derivedFinding) {
		throw "$Label status was not derived from the retained raw proof/diagnostic axes."
	}
	$findingDefect = Require-Text `
		(Get-ObjectPropertyValue $finding "defect") "$Label finding.defect"
	$findingNextStep = Require-Text `
		(Get-ObjectPropertyValue $finding "nextStep") "$Label finding.nextStep"
	if ($findingDefect -cne $derivedFindingDefect -or
		$findingNextStep -cne $derivedFindingNextStep) {
		throw "$Label finding defect/next step does not match the derived disposition."
	}

	$harnessHead = Require-Text `
		(Get-ObjectPropertyValue $indexHarness "gitHead") "$Label harness Git HEAD"
	if ($harnessHead -cnotmatch '^[0-9a-f]{40}$' -or
		[string] (Get-ObjectPropertyValue $runHarness "gitHead") -cne $harnessHead -or
		(Get-ObjectPropertyValue $runHarness "dirty") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $runHarness "dirty") -or
		(Get-ObjectPropertyValue $indexHarness "dirty") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $indexHarness "dirty")) {
		throw "$Label harness identity is not a clean immutable revision."
	}
	$toolBindings = @(
		[PSCustomObject] @{
			WorktreeField = "campaignRunnerSha256"
			BlobField = "campaignRunnerGitBlobSha256"
			Path = "tools/run-guarded-campaign-debug.ps1"
		},
		[PSCustomObject] @{
			WorktreeField = "candidateModuleSha256"
			BlobField = "candidateModuleGitBlobSha256"
			Path = "tools/Partisan.ReleaseCandidate.psm1"
		},
		[PSCustomObject] @{
			WorktreeField = "releaseIndexProducerSha256"
			BlobField = "releaseIndexProducerGitBlobSha256"
			Path = "tools/New-PartisanCampaignDebugReleaseIndex.ps1"
		},
		[PSCustomObject] @{
			WorktreeField = "releaseDocsConsumerSha256"
			BlobField = "releaseDocsConsumerGitBlobSha256"
			Path = "tools/update-release-docs.ps1"
		})
	if ($AllowUntrackedSummaryForSelfTest -and
		($null -eq $TrustedToolBindingsForSelfTest -or
			[string] (Get-ObjectPropertyValue $TrustedToolBindingsForSelfTest "gitHead") -cne
				$harnessHead)) {
		throw "$Label self-test harness Git HEAD differs from its trusted tool bundle."
	}
	foreach ($binding in $toolBindings) {
		$recordedWorktree = (Require-Sha256 `
			(Get-ObjectPropertyValue $runHarness $binding.WorktreeField) `
			"$Label run harness $($binding.WorktreeField)").ToLowerInvariant()
		$recordedBlob = (Require-Sha256 `
			(Get-ObjectPropertyValue $runHarness $binding.BlobField) `
			"$Label run harness $($binding.BlobField)").ToLowerInvariant()
		if ([string] (Get-ObjectPropertyValue $indexHarness $binding.WorktreeField) -cne
				$recordedWorktree -or
			[string] (Get-ObjectPropertyValue $indexHarness $binding.BlobField) -cne
				$recordedBlob) {
			throw "$Label index/run harness $($binding.WorktreeField) differs."
		}
		$expectedToolSha = $null
		if ($AllowUntrackedSummaryForSelfTest) {
			if ($null -eq $TrustedToolBindingsForSelfTest -or
				$null -eq $TrustedToolBindingsForSelfTest.PSObject.Properties[$binding.BlobField]) {
				throw "$Label self-test tool trust bundle is incomplete."
			}
			$expectedToolSha = [string] $TrustedToolBindingsForSelfTest.($binding.BlobField)
		}
		else {
			$expectedToolSha = Get-GitBlobSha256 `
				$harnessHead $binding.Path "$Label immutable $($binding.Path)"
		}
		if ($recordedBlob -cne $expectedToolSha) {
			throw "$Label recorded $($binding.BlobField) differs from its immutable harness blob."
		}
	}

	$identityChecks = [ordered] @{
		candidateId = [string] $CandidateIdentity.CandidateId
		candidateSourceHead = [string] $CandidateIdentity.CandidateSourceHead
		packageSha256 = [string] $CandidateIdentity.PackageSha256
		manifestSha256 = [string] $CandidateIdentity.ManifestSha256
		readySha256 = [string] $CandidateIdentity.ReadySha256
	}
	foreach ($binding in $identityChecks.GetEnumerator()) {
		if ([string] (Get-ObjectPropertyValue $indexCandidate $binding.Key) -cne
				$binding.Value -or
			[string] (Get-ObjectPropertyValue $Evidence $binding.Key) -cne
				$binding.Value) {
			throw "$Label $($binding.Key) differs from the retained candidate identity."
		}
	}
	if ([string] (Get-ObjectPropertyValue $runCandidate "candidateId") -cne
			[string] $CandidateIdentity.CandidateId -or
		[string] (Get-ObjectPropertyValue $runCandidate "gitHead") -cne
			[string] $CandidateIdentity.CandidateSourceHead -or
		[string] (Get-ObjectPropertyValue $runCandidate "packageSha256") -cne
			[string] $CandidateIdentity.PackageSha256 -or
		[string] (Get-ObjectPropertyValue $runCandidate "manifestSha256") -cne
			[string] $CandidateIdentity.ManifestSha256 -or
		[string] (Get-ObjectPropertyValue $runCandidate "readySha256") -cne
			[string] $CandidateIdentity.ReadySha256 -or
		[string] (Get-ObjectPropertyValue $runCandidate "workbenchCrc") -cne
			[string] $CandidateIdentity.WorkbenchCrc -or
		[string] (Get-ObjectPropertyValue $runCandidate "runtimeUseDisposition") -cne
			"active-runtime-candidate" -or
		[string] (Get-ObjectPropertyValue $indexCandidate "workbenchCrc") -cne
			[string] $CandidateIdentity.WorkbenchCrc -or
		[string] (Get-ObjectPropertyValue $indexCandidate "runtimeUseDispositionAtCapture") -cne
			"active-runtime-candidate") {
		throw "$Label run/index candidate binding is inconsistent."
	}
	$candidateStringBindings = [ordered] @{
		candidateVersion = $bundleCandidateVersion
		embeddedBuildSha = $bundleEmbeddedBuildSha
		embeddedBuildUtc = $bundleEmbeddedBuildUtc
		embeddedBuildLabel = $bundleEmbeddedBuildLabel
		addonId = $bundleAddonId
		addonGuid = $bundleAddonGuid
		packageHashAlgorithm = $bundlePackageHashAlgorithm
		runtimeRole = "client"
	}
	foreach ($binding in $candidateStringBindings.GetEnumerator()) {
		if ([string] (Get-ObjectPropertyValue $runCandidate $binding.Key) -cne
				[string] $binding.Value -or
			[string] (Get-ObjectPropertyValue $indexCandidate $binding.Key) -cne
				[string] $binding.Value) {
			throw "$Label run/index $($binding.Key) differs from the retained candidate manifest."
		}
	}
	$candidateIntegerBindings = [ordered] @{
		campaignSchema = $bundleCampaignSchema
		runtimeSettingsSchema = $bundleRuntimeSettingsSchema
	}
	foreach ($binding in $candidateIntegerBindings.GetEnumerator()) {
		if ((Require-IntegerProperty $runCandidate $binding.Key `
				"$Label run candidate $($binding.Key)") -ne [long] $binding.Value -or
			(Require-IntegerProperty $indexCandidate $binding.Key `
				"$Label index candidate $($binding.Key)") -ne [long] $binding.Value) {
			throw "$Label run/index $($binding.Key) differs from the retained candidate manifest."
		}
	}
	Assert-ExecutableIdentityEqual $bundleManifestClientDiagnostic `
		(Get-ObjectPropertyValue $runCandidate "diagnosticExecutable") `
		"$Label run candidate diagnostic executable"
	Assert-ExecutableIdentityEqual $bundleManifestClientDiagnostic `
		(Get-ObjectPropertyValue $runCandidate "recordedDiagnosticExecutable") `
		"$Label run candidate recorded diagnostic executable"
	Assert-ExecutableIdentityEqual $bundleManifestClient `
		(Get-ObjectPropertyValue $runCandidate "recordedRuntimeExecutable") `
		"$Label run candidate recorded runtime executable"
	Assert-ExecutableIdentityEqual $bundleManifestClientDiagnostic `
		(Get-ObjectPropertyValue $runLaunch "diagnosticExecutable") `
		"$Label launch diagnostic executable"
	Assert-ExecutableIdentityEqual $bundleManifestClient `
		(Get-ObjectPropertyValue $runLaunch "recordedRuntimeExecutable") `
		"$Label launch recorded runtime executable"
	Assert-ExecutableIdentityEqual $bundleManifestClientDiagnostic `
		(Get-ObjectPropertyValue $indexCandidate "diagnosticExecutable") `
		"$Label index candidate diagnostic executable"
	Assert-ExecutableIdentityEqual $bundleManifestClientDiagnostic `
		(Get-ObjectPropertyValue $indexCandidate "recordedDiagnosticExecutable") `
		"$Label index candidate recorded diagnostic executable"
	Assert-ExecutableIdentityEqual $bundleManifestClient `
		(Get-ObjectPropertyValue $indexCandidate "recordedRuntimeExecutable") `
		"$Label index candidate recorded runtime executable"
	if ((Require-IntegerProperty $runSettings "schemaVersion" `
			"$Label run settings.schemaVersion") -ne
			$SourceSettingsSchema -or
		(Require-IntegerProperty $runSettings "schemaVersion" `
			"$Label run settings.schemaVersion") -ne
			$bundleRuntimeSettingsSchema -or
		(Require-IntegerProperty $indexSettings "schemaVersion" `
			"$Label index settings.schemaVersion") -ne
			$SourceSettingsSchema -or
		(Require-IntegerProperty $indexSettings "schemaVersion" `
			"$Label index settings.schemaVersion") -ne
			$bundleRuntimeSettingsSchema -or
		[string] (Get-ObjectPropertyValue $runSettings "sha256") -cne
			[string] (Get-ObjectPropertyValue $Evidence "settingsSha256") -or
		[string] (Get-ObjectPropertyValue $indexSettings "sha256") -cne
			[string] (Get-ObjectPropertyValue $Evidence "settingsSha256") -or
		(Get-ObjectPropertyValue $runSettings "guardedRuntimeCopy") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $runSettings "guardedRuntimeCopy") -or
		(Get-ObjectPropertyValue $indexSettings "guardedRuntimeCopy") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $indexSettings "guardedRuntimeCopy")) {
		throw "$Label settings schema/hash binding is inconsistent."
	}

	$startedUtc = Require-UtcTimestamp `
		(Get-ObjectPropertyValue $capture "startedUtc") "$Label start time"
	$completedUtc = Require-UtcTimestamp `
		(Get-ObjectPropertyValue $capture "completedUtc") "$Label completion time"
	$runtimeSeconds = Require-IntegerProperty `
		$capture "runtimeSeconds" "$Label capture.runtimeSeconds"
	$runRuntimeSeconds = Require-IntegerProperty `
		$runOutcome "runtimeSeconds" "$Label run outcome.runtimeSeconds"
	if ($completedUtc -lt $startedUtc -or $StatusAsOfUtc -lt $completedUtc -or
		$runtimeSeconds -le 0 -or
		[Math]::Abs(($completedUtc - $startedUtc).TotalSeconds - $runtimeSeconds) -gt 5 -or
		$runRuntimeSeconds -ne $runtimeSeconds -or
		[string] (Get-ObjectPropertyValue $run "startedUtc") -cne
			[string] (Get-ObjectPropertyValue $capture "startedUtc") -or
		[string] (Get-ObjectPropertyValue $run "completedUtc") -cne
			[string] (Get-ObjectPropertyValue $capture "completedUtc") -or
		[string] (Get-ObjectPropertyValue $Evidence "runLeafId") -cne $runLeafId -or
		[string] (Get-ObjectPropertyValue $Evidence "runId") -cne $runId -or
		[string] (Get-ObjectPropertyValue $Evidence "startedUtc") -cne
			[string] (Get-ObjectPropertyValue $capture "startedUtc") -or
		[string] (Get-ObjectPropertyValue $Evidence "completedUtc") -cne
			[string] (Get-ObjectPropertyValue $capture "completedUtc") -or
		(Require-IntegerProperty $Evidence "runtimeSeconds" `
			"$Label.runtimeSeconds") -ne $runtimeSeconds) {
		throw "$Label capture identity/timestamps are inconsistent."
	}

	$evidenceIntegerChecks = [ordered] @{
		caseCount = $caseCount; pass = $caseCounts.PASS; warn = $caseCounts.WARN
		fail = $caseCounts.FAIL; blocked = $caseCounts.BLOCKED
		skipped = $caseCounts.SKIPPED; requiredAssertions = $requiredAssertions
		provenAssertions = $certCounts.PASS; failedAssertions = $certCounts.FAIL
		blockedAssertions = $certCounts.BLOCKED
		hardDiagnosticCount = $hardCount; scriptErrors = $scriptErrors
		engineErrors = $engineErrors
		partisanErrors = [int] (Get-ObjectPropertyValue $diagnostics "partisanErrors")
		approvedStockDiagnosticCount = $stockCount
		approvedIntentionalDiagnosticCount = $intentionalCount
		unapprovedHardDiagnosticCount = $unapprovedCount
		classifierSelfTestCount = $classifierCount
		canonicalScriptLogCount = [int] (Get-ObjectPropertyValue $diagnostics "canonicalScriptLogCount")
		canonicalConsoleLogCount = [int] (Get-ObjectPropertyValue $diagnostics "canonicalConsoleLogCount")
		stateDiffRows = $deltaMatches.Count; nonzeroStateDiffRows = $nonzeroDeltaCount
		envelopeFileCount = $runFiles.Count
	}
	foreach ($binding in $evidenceIntegerChecks.GetEnumerator()) {
		if ((Require-IntegerProperty $Evidence $binding.Key `
				"$Label.$($binding.Key)") -ne
			[int] $binding.Value) {
			throw "$Label.$($binding.Key) differs from the retained raw bundle."
		}
	}
	$evidenceBooleanChecks = [ordered] @{
		wrapperCaptureSuccess = $wrapperCaptureSuccess
		runtimeOutcomeSuccess = $guardedRunSucceeded
		certificationPassed = $certificationPassed
		artifactSchemaValidationValid = $artifactValidationValid
		candidateBoundaryVerified = $true
		mountPacked = $true
		artifactsStable = $true
		diagnosticClassificationValid = [bool] (Get-ObjectPropertyValue $diagnostics "valid")
		hardDiagnosticFree = [bool] (Get-ObjectPropertyValue $diagnostics "hardDiagnosticFree")
		canonicalLogPairSameDirectory = [bool] (Get-ObjectPropertyValue $diagnostics "canonicalLogPairSameDirectory")
		finalOrphanCleanupPass = [bool] (Get-ObjectPropertyValue $proof "finalOrphanCleanupPass")
		cleanupAndSpillZero = $true
		envelopeFilesRehashed = $true
	}
	foreach ($binding in $evidenceBooleanChecks.GetEnumerator()) {
		$value = Get-ObjectPropertyValue $Evidence $binding.Key
		if ($value -isnot [bool] -or [bool] $value -ne [bool] $binding.Value) {
			throw "$Label.$($binding.Key) differs from the retained raw bundle."
		}
	}
	if ([string] (Get-ObjectPropertyValue $Evidence "harnessGitHead") -cne $harnessHead -or
		[string] (Get-ObjectPropertyValue $Evidence "campaignRunnerSha256") -cne
			[string] (Get-ObjectPropertyValue $indexHarness "campaignRunnerSha256") -or
		[string] (Get-ObjectPropertyValue $Evidence "candidateModuleSha256") -cne
			[string] (Get-ObjectPropertyValue $indexHarness "candidateModuleSha256") -or
		[string] (Get-ObjectPropertyValue $Evidence "acceptanceDisposition") -cne
			$derivedAcceptance -or
		[string] (Get-ObjectPropertyValue $Evidence "error") -cne
			[string] (Get-ObjectPropertyValue $result "error")) {
		throw "$Label retained harness/result disposition is inconsistent."
	}
	$evidenceExternalAdvisoryIds = @((Get-ObjectPropertyValue `
		$Evidence "externalRequiredAdvisoryIds"))
	Assert-EqualSet $derivedExternalAdvisoryIds $evidenceExternalAdvisoryIds `
		"$Label status/raw external-required advisory IDs"
	if ((Get-ObjectPropertyValue $integrity "envelopeFilesRehashed") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $integrity "envelopeFilesRehashed") -or
		(Get-ObjectPropertyValue $source "filesRehashed") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $source "filesRehashed")) {
		throw "$Label release index does not attest its independently rehashed raw bundle."
	}
	if ([string] (Get-ObjectPropertyValue $integrity "releaseIndexProducerSha256") -cne
			[string] (Get-ObjectPropertyValue $indexHarness "releaseIndexProducerSha256") -or
		[string] (Get-ObjectPropertyValue $integrity "releaseDocsConsumerSha256") -cne
			[string] (Get-ObjectPropertyValue $indexHarness "releaseDocsConsumerSha256")) {
		throw "$Label release-index integrity/tool bindings differ."
	}

	return [PSCustomObject] @{
		SummaryPath = $summaryPath
		SummarySha256 = $summarySha
		HarnessGitHead = $harnessHead
		StartedUtc = $startedUtc
		CompletedUtc = $completedUtc
		RunId = $runId
		RunLeafId = $runLeafId
		EnvelopeSha256 = $runSha
		RunSummarySha256 = $runSha
		Accepted = $acceptedFull -or $acceptedInternal
		AcceptedFull = $acceptedFull
		AcceptedInternal = $acceptedInternal
		Rejected = -not ($acceptedFull -or $acceptedInternal)
		AcceptanceDisposition = $derivedAcceptance
		ExternalRequiredAdvisoryIds = $derivedExternalAdvisoryIds
		LegacyHistoricalFixture = $false
	}
}

function Assert-ActiveFullCampaignDebugEvidence {
	param(
		[object] $Evidence,
		[object] $CandidateIdentity,
		[string] $Label,
		[DateTimeOffset] $StatusAsOfUtc,
		[int] $SourceSettingsSchema,
		[switch] $AllowUntrackedSummaryForSelfTest,
		[object] $TrustedToolBindingsForSelfTest,
		[string] $PortableEvidenceRoot = $EvidenceBundleRoot
	)

	if ($null -eq $Evidence) {
		throw "$Label is missing."
	}

	$statusValue = Require-Text `
		(Get-ObjectPropertyValue $Evidence "status") "$Label.status"
	if ($statusValue -ceq "failed-certification-and-unapproved-diagnostics") {
		$legacyValidation = Assert-HistoricalRejectedFullCampaignDebugEvidence `
			$Evidence `
			$CandidateIdentity `
			$Label `
			$StatusAsOfUtc `
			$SourceSettingsSchema
		return [PSCustomObject] @{
			SummaryPath = $legacyValidation.SummaryPath
			SummarySha256 = $legacyValidation.SummarySha256
			HarnessGitHead = $legacyValidation.HarnessGitHead
			StartedUtc = $legacyValidation.StartedUtc
			CompletedUtc = $legacyValidation.CompletedUtc
			RunId = $legacyValidation.RunId
			RunLeafId = $legacyValidation.RunLeafId
			EnvelopeSha256 = $legacyValidation.EnvelopeSha256
			RunSummarySha256 = $legacyValidation.RunSummarySha256
			Accepted = $false
			AcceptedFull = $false
			AcceptedInternal = $false
			Rejected = $true
			AcceptanceDisposition = "rejected-red-full-profile"
			LegacyHistoricalFixture = $true
		}
	}
	if ($statusValue -cin @(
			"passed-full-certification",
			"passed-internal-profile-external-required",
			"failed-full-profile")) {
		return Assert-PortableFullCampaignDebugEvidence `
			$Evidence `
			$CandidateIdentity `
			$Label `
			$StatusAsOfUtc `
			$SourceSettingsSchema `
			-AllowUntrackedSummaryForSelfTest:$AllowUntrackedSummaryForSelfTest `
			-TrustedToolBindingsForSelfTest $TrustedToolBindingsForSelfTest `
			-PortableEvidenceRoot $PortableEvidenceRoot
	}

	$acceptedFull = $false
	$acceptedInternal = $false
	$rejected = $false
	$expectedAcceptanceDisposition = ""
	$expectedReleaseDisposition = ""
	$expectedFindingStatus = ""
	if ($statusValue -ceq "passed-full-certification") {
		$acceptedFull = $true
		$expectedAcceptanceDisposition = "accepted-full-profile"
		$expectedReleaseDisposition = "advance-external-gates"
		$expectedFindingStatus = "accepted-full-profile"
	}
	elseif ($statusValue -ceq "passed-internal-profile-external-required") {
		$acceptedInternal = $true
		$expectedAcceptanceDisposition = "accepted-internal-profile"
		$expectedReleaseDisposition = "advance-external-required"
		$expectedFindingStatus = "accepted-internal-profile-external-required"
	}
	elseif ($statusValue -ceq "failed-full-profile") {
		$rejected = $true
		$expectedAcceptanceDisposition = "rejected-full-profile"
		$expectedReleaseDisposition = "remain-no-go"
		$expectedFindingStatus = "release-blocking-red-full-profile"
	}
	else {
		throw "$Label.status is not a closed full-profile disposition."
	}
	$accepted = $acceptedFull -or $acceptedInternal

	$summaryPath = Require-RepoRelativePath `
		(Get-ObjectPropertyValue $Evidence "summaryPath") "$Label.summaryPath"
	$summarySha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "summarySha256") `
		"$Label.summarySha256").ToLowerInvariant()
	$harnessHead = Require-Text `
		(Get-ObjectPropertyValue $Evidence "harnessGitHead") "$Label.harnessGitHead"
	if ($harnessHead -cnotmatch '^[0-9a-f]{40}$') {
		throw "$Label.harnessGitHead must be a lowercase full Git SHA."
	}

	$repositoryPrefix = [IO.Path]::GetFullPath($root).TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
	$summaryFullPath = [IO.Path]::GetFullPath((Join-Path $root $summaryPath))
	if (-not $summaryFullPath.StartsWith(
		$repositoryPrefix,
		[StringComparison]::OrdinalIgnoreCase) -or
		-not (Test-Path -LiteralPath $summaryFullPath -PathType Leaf)) {
		throw "$Label summary is missing or outside the repository."
	}
	$summaryItem = Get-Item -LiteralPath $summaryFullPath -Force
	if (($summaryItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
		throw "$Label summary must not be a reparse point."
	}
	if (-not $AllowUntrackedSummaryForSelfTest) {
		& git -C $root ls-files --error-unmatch -- $summaryPath *> $null
		if ($LASTEXITCODE -ne 0) {
			throw "$Label summary must be a tracked repository artifact."
		}
	}
	$actualSummarySha = (Get-FileHash `
		-LiteralPath $summaryFullPath -Algorithm SHA256).Hash.ToLowerInvariant()
	if ($actualSummarySha -cne $summarySha) {
		throw "$Label summary SHA-256 does not match release status."
	}
	$summaryText = Get-Content -Raw -LiteralPath $summaryFullPath
	if ($summaryText -match '(?i)[A-Z]:[\\/]') {
		throw "$Label summary contains a local absolute path."
	}
	try {
		$summary = $summaryText | ConvertFrom-Json
	}
	catch {
		throw "$Label summary is not valid JSON: $($_.Exception.Message)"
	}

	$summaryCandidate = Get-ObjectPropertyValue $summary "candidate"
	$summaryHarness = Get-ObjectPropertyValue $summary "harness"
	$summarySettings = Get-ObjectPropertyValue $summary "settings"
	$summaryCapture = Get-ObjectPropertyValue $summary "capture"
	$summaryResult = Get-ObjectPropertyValue $summary "result"
	$summaryProof = Get-ObjectPropertyValue $summary "proof"
	$summaryDiagnostics = Get-ObjectPropertyValue $summary "diagnostics"
	$summaryCleanup = Get-ObjectPropertyValue $summary "cleanup"
	$summaryIntegrity = Get-ObjectPropertyValue $summary "integrity"
	$summaryFinding = Get-ObjectPropertyValue $summary "finding"
	if ((Require-IntegerProperty $summary "schemaVersion" "$Label summary.schemaVersion") -ne 1 -or
		[string] (Get-ObjectPropertyValue $summary "evidenceKind") -cne
			"packaged-campaign-debug-full-profile" -or
		$null -eq $summaryCandidate -or $null -eq $summaryHarness -or
		$null -eq $summarySettings -or $null -eq $summaryCapture -or
		$null -eq $summaryResult -or $null -eq $summaryProof -or
		$null -eq $summaryDiagnostics -or $null -eq $summaryCleanup -or
		$null -eq $summaryIntegrity -or $null -eq $summaryFinding) {
		throw "$Label summary is structurally incomplete."
	}

	Assert-IntegerProperties $summarySettings @("schemaVersion") "$Label summary.settings"
	Assert-IntegerProperties $summaryCapture @("runtimeSeconds") "$Label summary.capture"
	Assert-IntegerProperties $summaryProof @(
		"startedAtSecond", "endedAtSecond", "caseCount", "pass", "warn", "fail",
		"blocked", "skipped", "certificationRequired", "certificationProven",
		"certificationFail", "certificationBlocked", "certificationWarn",
		"stateDiffRows", "nonzeroStateDiffRows", "phase17ProjectionCheckCount",
		"phase17ProjectionChecksPassed", "phase24CheckCount", "phase24ChecksPassed",
		"stagedCleanupCheckCount", "stagedCleanupChecksPassed",
		"finalOrphanActiveGroups") "$Label summary.proof"
	Assert-IntegerProperties $summaryDiagnostics @(
		"hardDiagnosticCount", "scriptErrors", "engineErrors", "partisanErrors",
		"crashMarkers", "partisanSeverityLineCount", "approvedStockDiagnosticCount",
		"approvedIntentionalDiagnosticCount", "unapprovedHardDiagnosticCount",
		"classifierSelfTestCount", "malformedHardDiagnosticCount",
		"canonicalScriptLogCount", "canonicalConsoleLogCount") "$Label summary.diagnostics"
	Assert-IntegerProperties $summaryCleanup @(
		"guardRemaining", "ownedProcessesRemaining", "newEngineProcessesRemaining",
		"unclaimedEngineProcessesObserved", "newDefaultEntriesRemaining",
		"modifiedDefaultFiles", "deletedDefaultEntries", "missingDefaultRoots",
		"externalSpillEntriesRemaining", "modifiedSpillFiles", "deletedSpillEntries",
		"missingSpillRoots", "cleanupPhaseErrorCount") "$Label summary.cleanup"
	Assert-IntegerProperties $summaryIntegrity @("envelopeFileCount") "$Label summary.integrity"
	Assert-IntegerProperties $Evidence @(
		"runtimeSeconds", "caseCount", "pass", "warn", "fail", "blocked", "skipped",
		"requiredAssertions", "provenAssertions", "failedAssertions",
		"blockedAssertions", "hardDiagnosticCount", "scriptErrors", "engineErrors",
		"partisanErrors", "approvedStockDiagnosticCount",
		"approvedIntentionalDiagnosticCount", "unapprovedHardDiagnosticCount",
		"classifierSelfTestCount", "canonicalScriptLogCount",
		"canonicalConsoleLogCount", "stateDiffRows", "nonzeroStateDiffRows",
		"envelopeFileCount") $Label

	if ([string] (Get-ObjectPropertyValue $Evidence "candidateId") -cne
			[string] $CandidateIdentity.CandidateId -or
		[string] (Get-ObjectPropertyValue $Evidence "candidateSourceHead") -cne
			[string] $CandidateIdentity.CandidateSourceHead -or
		[string] (Get-ObjectPropertyValue $Evidence "packageSha256") -cne
			[string] $CandidateIdentity.PackageSha256 -or
		[string] (Get-ObjectPropertyValue $Evidence "manifestSha256") -cne
			[string] $CandidateIdentity.ManifestSha256 -or
		[string] (Get-ObjectPropertyValue $Evidence "readySha256") -cne
			[string] $CandidateIdentity.ReadySha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "candidateId") -cne
			[string] $CandidateIdentity.CandidateId -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "candidateSourceHead") -cne
			[string] $CandidateIdentity.CandidateSourceHead -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "packageSha256") -cne
			[string] $CandidateIdentity.PackageSha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "manifestSha256") -cne
			[string] $CandidateIdentity.ManifestSha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "readySha256") -cne
			[string] $CandidateIdentity.ReadySha256 -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "workbenchCrc") -cne
			[string] $CandidateIdentity.WorkbenchCrc -or
		[string] (Get-ObjectPropertyValue $summaryCandidate "runtimeUseDispositionAtCapture") -cne
			"active-runtime-candidate") {
		throw "$Label differs from the retained candidate identity."
	}

	$runnerSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "campaignRunnerSha256") `
		"$Label.campaignRunnerSha256").ToLowerInvariant()
	$runnerPolicy = Resolve-CampaignRunnerPolicy `
		$runnerSha "$Label.campaignRunnerSha256"
	if (-not $runnerPolicy.AllowsGenericFullProfileOutcome) {
		throw "$Label generic outcome requires the current sealed guarded-runner policy."
	}
	$expectedClassifierSelfTestCount = [int] $runnerPolicy.ClassifierSelfTestCount
	$candidateModuleSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "candidateModuleSha256") `
		"$Label.candidateModuleSha256").ToLowerInvariant()
	$settingsSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "settingsSha256") `
		"$Label.settingsSha256").ToLowerInvariant()
	if ([string] (Get-ObjectPropertyValue $summaryHarness "gitHead") -cne $harnessHead -or
		(Get-ObjectPropertyValue $summaryHarness "dirty") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryHarness "dirty") -or
		[string] (Get-ObjectPropertyValue $summaryHarness "campaignRunnerSha256") -cne
			$runnerSha -or
		[string] (Get-ObjectPropertyValue $summaryHarness "candidateModuleSha256") -cne
			$candidateModuleSha -or
		[int] (Get-ObjectPropertyValue $summarySettings "schemaVersion") -ne
			$SourceSettingsSchema -or
		[string] (Get-ObjectPropertyValue $summarySettings "sha256") -cne $settingsSha -or
		(Get-ObjectPropertyValue $summarySettings "guardedRuntimeCopy") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summarySettings "guardedRuntimeCopy")) {
		throw "$Label harness or settings identity is inconsistent."
	}

	$runLeafId = Require-Text `
		(Get-ObjectPropertyValue $summaryCapture "runLeafId") "$Label run-leaf ID"
	$runId = Require-Text `
		(Get-ObjectPropertyValue $summaryCapture "runId") "$Label run ID"
	$startedUtc = Require-UtcTimestamp `
		(Get-ObjectPropertyValue $summaryCapture "startedUtc") "$Label start time"
	$completedUtc = Require-UtcTimestamp `
		(Get-ObjectPropertyValue $summaryCapture "completedUtc") "$Label completion time"
	$captureRuntimeSeconds = [int] (Get-ObjectPropertyValue $summaryCapture "runtimeSeconds")
	$wallClockSeconds = ($completedUtc - $startedUtc).TotalSeconds
	if ($completedUtc -lt $startedUtc -or $StatusAsOfUtc -lt $completedUtc -or
		[string] (Get-ObjectPropertyValue $Evidence "startedUtc") -cne
			[string] (Get-ObjectPropertyValue $summaryCapture "startedUtc") -or
		[string] (Get-ObjectPropertyValue $Evidence "completedUtc") -cne
			[string] (Get-ObjectPropertyValue $summaryCapture "completedUtc") -or
		[string] (Get-ObjectPropertyValue $Evidence "runLeafId") -cne $runLeafId -or
		$runLeafId -cnotmatch '^\d{8}T\d{6}Z-[0-9a-f]{32}$' -or
		[string] (Get-ObjectPropertyValue $Evidence "runId") -cne $runId -or
		$runId -cnotmatch '^seed\d+_t\d+_p\d+_u\d+$' -or
		[string] (Get-ObjectPropertyValue $summaryCapture "profile") -cne
			"full_certification" -or
		[string] (Get-ObjectPropertyValue $summaryCapture "proofScope") -cne
			"full_certification" -or
		$captureRuntimeSeconds -le 0 -or
		[Math]::Abs($wallClockSeconds - $captureRuntimeSeconds) -gt 5 -or
		[int] (Get-ObjectPropertyValue $Evidence "runtimeSeconds") -ne
			$captureRuntimeSeconds) {
		throw "$Label capture identity, timestamps, or runtime are inconsistent."
	}

	if ([string] (Get-ObjectPropertyValue $summaryResult "status") -cne $statusValue -or
		[string] (Get-ObjectPropertyValue $summaryResult "acceptanceDisposition") -cne
			$expectedAcceptanceDisposition -or
		[string] (Get-ObjectPropertyValue $Evidence "acceptanceDisposition") -cne
			$expectedAcceptanceDisposition -or
		[string] (Get-ObjectPropertyValue $summaryResult "releaseDisposition") -cne
			$expectedReleaseDisposition -or
		(Get-ObjectPropertyValue $summaryResult "wrapperCaptureSuccess") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryResult "wrapperCaptureSuccess") -or
		(Get-ObjectPropertyValue $Evidence "wrapperCaptureSuccess") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $Evidence "wrapperCaptureSuccess")) {
		throw "$Label result disposition or wrapper capture is inconsistent."
	}
	foreach ($requiredTrueResultField in @(
		"armed", "started", "completed", "candidateBoundaryVerified", "mountPacked",
		"artifactsStable", "evidenceCaptured", "artifactSchemaValidationValid")) {
		if ((Get-ObjectPropertyValue $summaryResult $requiredTrueResultField) -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $summaryResult $requiredTrueResultField)) {
			throw "$Label result.$requiredTrueResultField must be true."
		}
	}
	foreach ($retainedResultField in @(
		"candidateBoundaryVerified", "mountPacked", "artifactsStable",
		"artifactSchemaValidationValid")) {
		if ((Get-ObjectPropertyValue $Evidence $retainedResultField) -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $Evidence $retainedResultField)) {
			throw "$Label.$retainedResultField must be true."
		}
	}
	$summaryRuntimeOutcomeSuccess = Get-ObjectPropertyValue `
		$summaryResult "runtimeOutcomeSuccess"
	$evidenceRuntimeOutcomeSuccess = Get-ObjectPropertyValue `
		$Evidence "runtimeOutcomeSuccess"
	$summaryCertificationPassed = Get-ObjectPropertyValue `
		$summaryResult "certificationPassed"
	$evidenceCertificationPassed = Get-ObjectPropertyValue `
		$Evidence "certificationPassed"
	if ($summaryRuntimeOutcomeSuccess -isnot [bool] -or
		$evidenceRuntimeOutcomeSuccess -isnot [bool] -or
		$summaryCertificationPassed -isnot [bool] -or
		$evidenceCertificationPassed -isnot [bool] -or
		[bool] $summaryRuntimeOutcomeSuccess -ne $accepted -or
		[bool] $evidenceRuntimeOutcomeSuccess -ne $accepted) {
		throw "$Label runtime outcome does not match its disposition."
	}
	$summaryError = [string] (Get-ObjectPropertyValue $summaryResult "error")
	$evidenceError = [string] (Get-ObjectPropertyValue $Evidence "error")
	if ($summaryError -cne $evidenceError -or
		($accepted -and -not [string]::IsNullOrEmpty($summaryError)) -or
		($rejected -and [string]::IsNullOrWhiteSpace($summaryError))) {
		throw "$Label error disposition is inconsistent."
	}

	$caseCount = [int] (Get-ObjectPropertyValue $summaryProof "caseCount")
	$pass = [int] (Get-ObjectPropertyValue $summaryProof "pass")
	$warn = [int] (Get-ObjectPropertyValue $summaryProof "warn")
	$fail = [int] (Get-ObjectPropertyValue $summaryProof "fail")
	$blocked = [int] (Get-ObjectPropertyValue $summaryProof "blocked")
	$skipped = [int] (Get-ObjectPropertyValue $summaryProof "skipped")
	$requiredAssertions = [int] (Get-ObjectPropertyValue $summaryProof "certificationRequired")
	$provenAssertions = [int] (Get-ObjectPropertyValue $summaryProof "certificationProven")
	$failedAssertions = [int] (Get-ObjectPropertyValue $summaryProof "certificationFail")
	$blockedAssertions = [int] (Get-ObjectPropertyValue $summaryProof "certificationBlocked")
	$warningAssertions = [int] (Get-ObjectPropertyValue $summaryProof "certificationWarn")
	foreach ($nonnegativeValue in @(
		$caseCount, $pass, $warn, $fail, $blocked, $skipped, $requiredAssertions,
		$provenAssertions, $failedAssertions, $blockedAssertions, $warningAssertions)) {
		if ($nonnegativeValue -lt 0) {
			throw "$Label proof counts must be nonnegative."
		}
	}
	$phase17Count = [int] (Get-ObjectPropertyValue $summaryProof "phase17ProjectionCheckCount")
	$phase17Passed = [int] (Get-ObjectPropertyValue $summaryProof "phase17ProjectionChecksPassed")
	$phase24Count = [int] (Get-ObjectPropertyValue $summaryProof "phase24CheckCount")
	$phase24Passed = [int] (Get-ObjectPropertyValue $summaryProof "phase24ChecksPassed")
	$stagedCleanupCount = [int] (Get-ObjectPropertyValue $summaryProof "stagedCleanupCheckCount")
	$stagedCleanupPassed = [int] (Get-ObjectPropertyValue $summaryProof "stagedCleanupChecksPassed")
	if ((Get-ObjectPropertyValue $summaryProof "fullCertification") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryProof "fullCertification") -or
		[int] (Get-ObjectPropertyValue $summaryProof "startedAtSecond") -ne 0 -or
		[int] (Get-ObjectPropertyValue $summaryProof "endedAtSecond") -le 0 -or
		$caseCount -le 0 -or
		$caseCount -ne ($pass + $warn + $fail + $blocked + $skipped) -or
		$requiredAssertions -le 0 -or
		$requiredAssertions -ne
			($provenAssertions + $failedAssertions + $blockedAssertions +
				$warningAssertions) -or
		[int] (Get-ObjectPropertyValue $summaryProof "stateDiffRows") -le 0 -or
		[int] (Get-ObjectPropertyValue $summaryProof "nonzeroStateDiffRows") -ne 0 -or
		$phase17Count -le 0 -or $phase17Passed -ne $phase17Count -or
		$phase24Count -le 0 -or $phase24Passed -ne $phase24Count -or
		$stagedCleanupCount -le 0 -or $stagedCleanupPassed -ne $stagedCleanupCount -or
		(Get-ObjectPropertyValue $summaryProof "finalOrphanCleanupPass") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryProof "finalOrphanCleanupPass") -or
		[int] (Get-ObjectPropertyValue $summaryProof "finalOrphanActiveGroups") -ne 0) {
		throw "$Label proof totals or cleanup checks are inconsistent."
	}
	$fixturePolicy = [ordered] @{
		intentionalMissionConvoyAdmissionDiagnosticsProven =
			[bool] $runnerPolicy.IntentionalAdmissionProven
		intentionalMissionConvoySettlementDiagnosticProven =
			[bool] $runnerPolicy.IntentionalSettlementProven
		intentionalMissionConvoyCorruptionDiagnosticsProven =
			[bool] $runnerPolicy.IntentionalCorruptionProven
		intentionalMissionConvoyWatchdogDiagnosticProven =
			[bool] $runnerPolicy.IntentionalWatchdogProven
	}
	foreach ($fixtureField in $fixturePolicy.Keys) {
		$fixtureValue = Get-ObjectPropertyValue $summaryProof $fixtureField
		if ($fixtureValue -isnot [bool] -or
			($accepted -and [bool] $fixtureValue -ne
				[bool] $fixturePolicy[$fixtureField])) {
			throw "$Label proof.$fixtureField is invalid for its sealed runner disposition (actual $fixtureValue, expected $($fixturePolicy[$fixtureField]), runner $runnerSha)."
		}
	}
	$proofFieldMap = [ordered] @{
		caseCount = "caseCount"
		pass = "pass"
		warn = "warn"
		fail = "fail"
		blocked = "blocked"
		skipped = "skipped"
		requiredAssertions = "certificationRequired"
		provenAssertions = "certificationProven"
		failedAssertions = "certificationFail"
		blockedAssertions = "certificationBlocked"
		stateDiffRows = "stateDiffRows"
		nonzeroStateDiffRows = "nonzeroStateDiffRows"
	}
	foreach ($statusField in $proofFieldMap.Keys) {
		$summaryField = [string] $proofFieldMap[$statusField]
		if ([int] (Get-ObjectPropertyValue $Evidence $statusField) -ne
			[int] (Get-ObjectPropertyValue $summaryProof $summaryField)) {
			throw "$Label.$statusField differs from its summary."
		}
	}
	if ((Get-ObjectPropertyValue $Evidence "finalOrphanCleanupPass") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $Evidence "finalOrphanCleanupPass")) {
		throw "$Label final orphan cleanup proof is not retained."
	}
	$proofCertificationPassed = $failedAssertions -eq 0 -and
		$blockedAssertions -eq 0 -and $warningAssertions -eq 0 -and
		$provenAssertions -eq $requiredAssertions
	if ([bool] $summaryCertificationPassed -ne $proofCertificationPassed -or
		[bool] $evidenceCertificationPassed -ne $proofCertificationPassed) {
		throw "$Label retained certification flag differs from its proof totals."
	}
	$externalRequiredBlockerIds = @()
	$externalBlockerValue = Get-ObjectPropertyValue `
		$summaryProof "externalRequiredBlockerIds"
	if ($null -ne $externalBlockerValue) {
		if ($externalBlockerValue -is [string]) {
			throw "$Label proof.externalRequiredBlockerIds must be a JSON array."
		}
		foreach ($blockerIdValue in @($externalBlockerValue)) {
			$blockerId = Require-Text $blockerIdValue `
				"$Label proof.externalRequiredBlockerIds"
			if ($blockerId -cnotmatch '^[a-z0-9]+(?:[._-][a-z0-9]+)*$') {
				throw "$Label external-required blocker ID contains unsupported characters."
			}
			$externalRequiredBlockerIds += $blockerId
		}
	}
	Assert-UniqueStrings $externalRequiredBlockerIds `
		"$Label external-required blocker IDs"
	$evidenceExternalRequiredBlockerIds = @()
	$evidenceExternalBlockerValue = Get-ObjectPropertyValue `
		$Evidence "externalRequiredBlockerIds"
	if ($null -ne $evidenceExternalBlockerValue) {
		if ($evidenceExternalBlockerValue -is [string]) {
			throw "$Label.externalRequiredBlockerIds must be a JSON array."
		}
		foreach ($blockerIdValue in @($evidenceExternalBlockerValue)) {
			$evidenceExternalRequiredBlockerIds += Require-Text `
				$blockerIdValue "$Label.externalRequiredBlockerIds"
		}
	}
	Assert-UniqueStrings $evidenceExternalRequiredBlockerIds `
		"$Label retained external-required blocker IDs"
	Assert-EqualSet $externalRequiredBlockerIds `
		$evidenceExternalRequiredBlockerIds `
		"$Label summary/status external-required blocker IDs"
	if ($acceptedFull) {
		if ($externalRequiredBlockerIds.Count -ne 0 -or $fail -ne 0 -or
			$blocked -ne 0 -or -not $proofCertificationPassed) {
			throw "$Label accepted full profile retains failed or blocked work."
		}
	}
	elseif ($acceptedInternal) {
		Assert-EqualSet `
			@($runnerPolicy.ExternalRequiredBlockerIds) `
			$externalRequiredBlockerIds `
			"$Label sealed external-required blocker set"
		if ($fail -ne 0 -or $blocked -le 0 -or $failedAssertions -ne 0 -or
			$warningAssertions -ne 0 -or $blockedAssertions -ne
				@($runnerPolicy.ExternalRequiredBlockerIds).Count -or
			$proofCertificationPassed) {
			throw "$Label internal acceptance contains a non-external blocker or certification failure."
		}
	}
	$certificationAxisFailed = -not $proofCertificationPassed

	$hardCount = [int] (Get-ObjectPropertyValue $summaryDiagnostics "hardDiagnosticCount")
	$scriptErrors = [int] (Get-ObjectPropertyValue $summaryDiagnostics "scriptErrors")
	$engineErrors = [int] (Get-ObjectPropertyValue $summaryDiagnostics "engineErrors")
	$partisanErrors = [int] (Get-ObjectPropertyValue $summaryDiagnostics "partisanErrors")
	$stockCount = [int] (Get-ObjectPropertyValue $summaryDiagnostics "approvedStockDiagnosticCount")
	$intentionalCount = [int] (Get-ObjectPropertyValue $summaryDiagnostics "approvedIntentionalDiagnosticCount")
	$unapprovedCount = [int] (Get-ObjectPropertyValue $summaryDiagnostics "unapprovedHardDiagnosticCount")
	foreach ($nonnegativeValue in @(
		$hardCount, $scriptErrors, $engineErrors, $partisanErrors, $stockCount,
		$intentionalCount, $unapprovedCount)) {
		if ($nonnegativeValue -lt 0) {
			throw "$Label diagnostic counts must be nonnegative."
		}
	}
	$unapprovedKinds = @(Get-ObjectPropertyValue `
		$summaryDiagnostics "unapprovedHardDiagnosticKinds")
	$unapprovedKindNames = @()
	$unapprovedKindsTotal = 0
	foreach ($unapprovedKind in $unapprovedKinds) {
		$kind = Require-Text (Get-ObjectPropertyValue $unapprovedKind "kind") `
			"$Label summary.diagnostics unapproved kind"
		$count = Require-IntegerProperty $unapprovedKind "count" `
			"$Label summary.diagnostics unapproved kind $kind"
		if ($count -le 0) {
			throw "$Label summary diagnostic kind $kind must have a positive count."
		}
		$unapprovedKindNames += $kind
		$unapprovedKindsTotal += [int] $count
	}
	Assert-UniqueStrings $unapprovedKindNames "$Label summary diagnostic kinds"
	$summaryDiagnosticsValid = Get-ObjectPropertyValue $summaryDiagnostics "valid"
	$summaryClassificationValid = Get-ObjectPropertyValue `
		$summaryDiagnostics "classificationValid"
	$summaryHardDiagnosticFree = Get-ObjectPropertyValue `
		$summaryDiagnostics "hardDiagnosticFree"
	$evidenceClassificationValid = Get-ObjectPropertyValue `
		$Evidence "diagnosticClassificationValid"
	$evidenceHardDiagnosticFree = Get-ObjectPropertyValue $Evidence "hardDiagnosticFree"
	if ($summaryDiagnosticsValid -isnot [bool] -or
		$summaryClassificationValid -isnot [bool] -or
		$summaryHardDiagnosticFree -isnot [bool] -or
		$evidenceClassificationValid -isnot [bool] -or
		$evidenceHardDiagnosticFree -isnot [bool] -or
		[bool] $summaryDiagnosticsValid -ne
			[bool] $summaryClassificationValid -or
		[bool] $evidenceClassificationValid -ne
			[bool] $summaryClassificationValid -or
		[bool] $summaryHardDiagnosticFree -ne ($hardCount -eq 0) -or
		[bool] $evidenceHardDiagnosticFree -ne ($hardCount -eq 0) -or
		$hardCount -ne ($scriptErrors + $engineErrors) -or
		$hardCount -ne ($stockCount + $intentionalCount + $unapprovedCount) -or
		$unapprovedKindsTotal -ne $unapprovedCount -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "classifierSelfTestCount") -ne
			$expectedClassifierSelfTestCount -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "crashMarkers") -ne 0 -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "partisanSeverityLineCount") -ne 0 -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "malformedHardDiagnosticCount") -ne 0 -or
		(Get-ObjectPropertyValue $summaryDiagnostics "channelArithmeticValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryDiagnostics "channelArithmeticValid") -or
		(Get-ObjectPropertyValue $summaryDiagnostics "categoryArithmeticValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryDiagnostics "categoryArithmeticValid") -or
		(Get-ObjectPropertyValue $summaryDiagnostics "lifecycleMarkersValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryDiagnostics "lifecycleMarkersValid") -or
		(Get-ObjectPropertyValue $summaryDiagnostics "identityBaselinePairValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryDiagnostics "identityBaselinePairValid") -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "canonicalScriptLogCount") -ne 1 -or
		[int] (Get-ObjectPropertyValue $summaryDiagnostics "canonicalConsoleLogCount") -ne 1 -or
		(Get-ObjectPropertyValue $summaryDiagnostics "canonicalLogPairSameDirectory") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryDiagnostics "canonicalLogPairSameDirectory")) {
		throw "$Label diagnostic census is inconsistent."
	}
	$fixtureStructureExact = Get-ObjectPropertyValue `
		$summaryDiagnostics "intentionalFixtureStructureExact"
	$fixtureSetValid = Get-ObjectPropertyValue `
		$summaryDiagnostics "intentionalFixtureSetValid"
	if ($fixtureStructureExact -isnot [bool] -or $fixtureSetValid -isnot [bool]) {
		throw "$Label summary diagnostic fixture disposition must be Boolean."
	}
	foreach ($diagnosticField in @(
		"hardDiagnosticCount", "scriptErrors", "engineErrors", "partisanErrors",
		"approvedStockDiagnosticCount", "approvedIntentionalDiagnosticCount",
		"unapprovedHardDiagnosticCount", "classifierSelfTestCount",
		"canonicalScriptLogCount", "canonicalConsoleLogCount")) {
		if ([int] (Get-ObjectPropertyValue $Evidence $diagnosticField) -ne
			[int] (Get-ObjectPropertyValue $summaryDiagnostics $diagnosticField)) {
			throw "$Label.$diagnosticField differs from its summary."
		}
	}
	$diagnosticAxisPassed = [bool] $summaryDiagnosticsValid -and
		[bool] $summaryClassificationValid -and
		[bool] $fixtureStructureExact -and [bool] $fixtureSetValid -and
		$unapprovedCount -eq 0
	if (-not $diagnosticAxisPassed -and $unapprovedCount -eq 0 -and
		[bool] $fixtureStructureExact -and [bool] $fixtureSetValid) {
		throw "$Label diagnostic rejection has no sealed invalid fixture or unapproved diagnostic."
	}
	if ($accepted -and (-not $diagnosticAxisPassed -or
		$hardCount -ne [int] $runnerPolicy.HardDiagnosticCount -or
		$scriptErrors -ne [int] $runnerPolicy.ScriptErrors -or
		$engineErrors -ne [int] $runnerPolicy.EngineErrors -or
		$partisanErrors -ne [int] $runnerPolicy.PartisanErrors -or
		$stockCount -ne [int] $runnerPolicy.ApprovedStockDiagnosticCount -or
		$intentionalCount -ne
			[int] $runnerPolicy.ApprovedIntentionalDiagnosticCount)) {
		throw "$Label accepted profile differs from the sealed runner diagnostic distribution."
	}
	if ($rejected -and -not $certificationAxisFailed -and $diagnosticAxisPassed) {
		throw "$Label rejected full profile has neither a certification nor diagnostic failure."
	}

	foreach ($cleanupField in @(
		"guardRemaining", "ownedProcessesRemaining", "newEngineProcessesRemaining",
		"unclaimedEngineProcessesObserved", "newDefaultEntriesRemaining",
		"modifiedDefaultFiles", "deletedDefaultEntries", "missingDefaultRoots",
		"externalSpillEntriesRemaining", "modifiedSpillFiles", "deletedSpillEntries",
		"missingSpillRoots", "cleanupPhaseErrorCount")) {
		if ([int] (Get-ObjectPropertyValue $summaryCleanup $cleanupField) -ne 0) {
			throw "$Label cleanup field $cleanupField must be zero."
		}
	}
	if ((Get-ObjectPropertyValue $summaryCleanup "monitoringRootsAreDetectionOnly") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryCleanup "monitoringRootsAreDetectionOnly") -or
		(Get-ObjectPropertyValue $Evidence "cleanupAndSpillZero") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $Evidence "cleanupAndSpillZero")) {
		throw "$Label cleanup and spill boundary is not clean."
	}

	$envelopeSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "envelopeSha256") `
		"$Label.envelopeSha256").ToLowerInvariant()
	$runSummarySha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "runSummarySha256") `
		"$Label.runSummarySha256").ToLowerInvariant()
	$rawArtifactSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $summaryIntegrity "rawArtifactSha256") `
		"$Label summary.integrity.rawArtifactSha256").ToLowerInvariant()
	$envelopeFileCount = [int] (Get-ObjectPropertyValue `
		$summaryIntegrity "envelopeFileCount")
	if ($runSummarySha -cne $envelopeSha -or
		[string] (Get-ObjectPropertyValue $summaryIntegrity "envelopeSha256") -cne
			$envelopeSha -or
		[string] (Get-ObjectPropertyValue $summaryIntegrity "runSummarySha256") -cne
			$runSummarySha -or
		[string]::IsNullOrWhiteSpace($rawArtifactSha) -or
		$envelopeFileCount -le 0 -or
		[int] (Get-ObjectPropertyValue $Evidence "envelopeFileCount") -ne
			$envelopeFileCount -or
		(Get-ObjectPropertyValue $summaryIntegrity "envelopeFilesRehashed") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryIntegrity "envelopeFilesRehashed") -or
		(Get-ObjectPropertyValue $Evidence "envelopeFilesRehashed") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $Evidence "envelopeFilesRehashed") -or
		[string] (Get-ObjectPropertyValue $summaryFinding "status") -cne
			$expectedFindingStatus -or
		[string]::IsNullOrWhiteSpace(
			[string] (Get-ObjectPropertyValue $summaryFinding "nextStep")) -or
		[string]::IsNullOrWhiteSpace([string] (Get-ObjectPropertyValue $Evidence "summary"))) {
		throw "$Label integrity or finding disposition is inconsistent."
	}
	$findingDefect = Require-Text `
		(Get-ObjectPropertyValue $summaryFinding "defect") "$Label finding.defect"
	if (($accepted -and $findingDefect -cne "none") -or
		($rejected -and $findingDefect -ceq "none")) {
		throw "$Label finding defect does not match its disposition."
	}

	return [PSCustomObject] @{
		SummaryPath = $summaryPath
		SummarySha256 = $summarySha
		HarnessGitHead = $harnessHead
		StartedUtc = $startedUtc
		CompletedUtc = $completedUtc
		RunId = $runId
		RunLeafId = $runLeafId
		EnvelopeSha256 = $envelopeSha
		RunSummarySha256 = $runSummarySha
		Accepted = $accepted
		AcceptedFull = $acceptedFull
		AcceptedInternal = $acceptedInternal
		Rejected = $rejected
		AcceptanceDisposition = $expectedAcceptanceDisposition
		ExternalRequiredBlockerIds = $externalRequiredBlockerIds
		LegacyHistoricalFixture = $false
	}
}

function Assert-ExactObjectProperties {
	param(
		[object] $Object,
		[string[]] $Expected,
		[string] $Label
	)

	if ($null -eq $Object -or $Object -is [string] -or $Object -is [Array]) {
		throw "$Label must be a JSON object."
	}
	$actual = @($Object.PSObject.Properties.Name)
	Assert-EqualSet $Expected $actual "$Label properties"
}

function Assert-GitAncestor {
	param(
		[string] $Ancestor,
		[string] $Descendant,
		[string] $Label
	)

	& git -C $root merge-base --is-ancestor $Ancestor $Descendant
	$gitExitCode = $LASTEXITCODE
	if ($gitExitCode -ne 0) {
		throw "$Label ($Ancestor -> $Descendant)."
	}
}

function Assert-HistoricalExternalRuntimeEvidence {
	param(
		[object] $Evidence,
		[object] $CandidateIdentity,
		[string] $Label,
		[DateTimeOffset] $StatusAsOfUtc
	)

	# External runtime retirement stays unadmitted until a portable guarded-runtime
	# receipt/index binds executable and tool provenance, process ownership, every
	# artifact row, journal identity, teardown, and the rehashed envelope.
	throw "$Label cannot admit external-runtime retirement without the trusted portable receipt/index contract."
}

function Assert-HistoricalCandidateEvidenceEntry {
	param(
		[object] $Entry,
		[int] $Index,
		[DateTimeOffset] $StatusAsOfUtc
	)

	$label = "release_status.historicalCandidateEvidence[$Index]"
	Assert-ExactObjectProperties $Entry `
		@("retirementDisposition", "candidate", "evidence") $label
	$retirementDisposition = Require-Text `
		(Get-ObjectPropertyValue $Entry "retirementDisposition") `
		"$label.retirementDisposition"
	if ($retirementDisposition -cnotin @(
			"rejected-after-full-profile",
			"rejected-after-corrected-canary",
			"rejected-after-external-runtime")) {
		throw "$label.retirementDisposition is unsupported."
	}

	$candidate = Get-ObjectPropertyValue $Entry "candidate"
	Assert-ExactObjectProperties $candidate @(
		"candidateId",
		"candidateSourceHead",
		"manifestPath",
		"manifestSha256",
		"readySha256",
		"packageHashAlgorithm",
		"packageSha256",
		"packageVersion",
		"workbenchCrc") "$label.candidate"
	$identity = Get-RetainedCandidateIdentity $candidate "$label.candidate"
	$identityLabel = "$label ($($identity.CandidateId))"

	$evidence = Get-ObjectPropertyValue $Entry "evidence"
	$expectedEvidenceProperties = @(
		"packagedFocusedAutotests",
		"correctedForceAuthorityCanary")
	if ($retirementDisposition -cin @(
		"rejected-after-full-profile", "rejected-after-external-runtime")) {
		$expectedEvidenceProperties += "fullCampaignDebug"
	}
	if ($retirementDisposition -ceq "rejected-after-external-runtime") {
		$expectedEvidenceProperties += "externalRuntime"
	}
	Assert-ExactObjectProperties $evidence $expectedEvidenceProperties "$identityLabel.evidence"

	$packagedFocused = Get-ObjectPropertyValue $evidence "packagedFocusedAutotests"
	$packagedFocusedStatus = Require-Text `
		(Get-ObjectPropertyValue $packagedFocused "status") `
		"$identityLabel.evidence.packagedFocusedAutotests.status"
	if ($packagedFocusedStatus -cne "historical-passed-noncertifying") {
		throw "$identityLabel packaged focused status must be historical-passed-noncertifying."
	}
	$packagedFocusedValidation = Assert-PackagedFocusedEvidence `
		$packagedFocused `
		$identity `
		$packagedFocusedStatus `
		"$identityLabel.evidence.packagedFocusedAutotests"
	$historicalFocusedPreliminaryValid =
		($packagedFocusedValidation.PreliminaryStatus -ceq "superseded-for-acceptance" -and
			$packagedFocusedValidation.PreliminaryCaseCount -eq 5) -or
		($packagedFocusedValidation.PreliminaryStatus -ceq "none" -and
			$packagedFocusedValidation.PreliminaryCaseCount -eq 0)
	if (-not $historicalFocusedPreliminaryValid) {
		throw "$identityLabel packaged focused evidence has an invalid preliminary-run disposition."
	}

	$correctedCanary = Get-ObjectPropertyValue $evidence "correctedForceAuthorityCanary"
	$correctedCanaryStatus = Require-Text `
		(Get-ObjectPropertyValue $correctedCanary "status") `
		"$identityLabel.evidence.correctedForceAuthorityCanary.status"
	$correctedCanaryOutcome = ""
	if ($retirementDisposition -cin @(
		"rejected-after-full-profile", "rejected-after-external-runtime")) {
		if ($correctedCanaryStatus -cne "historical-passed-noncertifying") {
			throw "$identityLabel post-canary retirement requires an accepted corrected canary."
		}
		$correctedCanaryOutcome = "accepted"
	}
	elseif ($correctedCanaryStatus -ceq "historical-failed-proof-validation") {
		$correctedCanaryOutcome = "rejected-proof"
	}
	elseif ($correctedCanaryStatus -ceq "historical-failed-unapproved-hard-diagnostic") {
		$correctedCanaryOutcome = "rejected"
	}
	else {
		throw "$identityLabel corrected-canary retirement requires rejected corrected-canary evidence."
	}
	$correctedCanaryValidation = Assert-CorrectedCanaryEvidence `
		$correctedCanary `
		$identity `
		$correctedCanaryStatus `
		$correctedCanaryOutcome `
		"$identityLabel.evidence.correctedForceAuthorityCanary" `
		$StatusAsOfUtc `
		$identity.RuntimeSettingsSchema
	if ($correctedCanaryValidation.StartedUtc -lt
			$packagedFocusedValidation.AcceptedCompletedUtc) {
		throw "$identityLabel corrected canary started before its packaged focused set completed."
	}
	if ($identity.CreatedUtc -gt $packagedFocusedValidation.AcceptedStartedUtc) {
		throw "$identityLabel packaged focused evidence predates candidate creation."
	}

	$fullCampaignDebug = $null
	$fullCampaignDebugValidation = $null
	$externalRuntime = $null
	$externalRuntimeValidation = $null
	$terminalValidation = $correctedCanaryValidation
	if ($retirementDisposition -ceq "rejected-after-full-profile") {
		$fullCampaignDebug = Get-ObjectPropertyValue $evidence "fullCampaignDebug"
		$fullCampaignDebugValidation = Assert-ActiveFullCampaignDebugEvidence `
			$fullCampaignDebug `
			$identity `
			"$identityLabel.evidence.fullCampaignDebug" `
			$StatusAsOfUtc `
			$identity.RuntimeSettingsSchema
		if (-not $fullCampaignDebugValidation.Rejected) {
			throw "$identityLabel rejected-after-full-profile requires a rejected full profile."
		}
		if ($fullCampaignDebugValidation.StartedUtc -lt
				$correctedCanaryValidation.CompletedUtc) {
			throw "$identityLabel full profile started before its corrected canary completed."
		}
		$terminalValidation = $fullCampaignDebugValidation
	}
	elseif ($retirementDisposition -ceq "rejected-after-external-runtime") {
		$fullCampaignDebug = Get-ObjectPropertyValue $evidence "fullCampaignDebug"
		$fullCampaignDebugValidation = Assert-ActiveFullCampaignDebugEvidence `
			$fullCampaignDebug `
			$identity `
			"$identityLabel.evidence.fullCampaignDebug" `
			$StatusAsOfUtc `
			$identity.RuntimeSettingsSchema
		if (-not $fullCampaignDebugValidation.Accepted) {
			throw "$identityLabel external-runtime retirement requires an accepted full profile."
		}
		if ($fullCampaignDebugValidation.StartedUtc -lt
			$correctedCanaryValidation.CompletedUtc) {
			throw "$identityLabel full profile started before its corrected canary completed."
		}
		$externalRuntime = Get-ObjectPropertyValue $evidence "externalRuntime"
		$externalRuntimeValidation = Assert-HistoricalExternalRuntimeEvidence `
			$externalRuntime `
			$identity `
			"$identityLabel.evidence.externalRuntime" `
			$StatusAsOfUtc
		if ($externalRuntimeValidation.StartedUtc -lt
			$fullCampaignDebugValidation.CompletedUtc) {
			throw "$identityLabel external runtime started before its accepted full profile completed."
		}
		$terminalValidation = $externalRuntimeValidation
	}

	return [PSCustomObject] @{
		Index = $Index
		RetirementDisposition = $retirementDisposition
		Candidate = $candidate
		Evidence = $evidence
		Identity = $identity
		PackagedFocused = $packagedFocused
		PackagedFocusedValidation = $packagedFocusedValidation
		CorrectedCanary = $correctedCanary
		CorrectedCanaryStatus = $correctedCanaryStatus
		CorrectedCanaryOutcome = $correctedCanaryOutcome
		CorrectedCanaryValidation = $correctedCanaryValidation
		FullCampaignDebug = $fullCampaignDebug
		FullCampaignDebugValidation = $fullCampaignDebugValidation
		ExternalRuntime = $externalRuntime
		ExternalRuntimeValidation = $externalRuntimeValidation
		TerminalHarnessGitHead = $terminalValidation.HarnessGitHead
		TerminalCompletedUtc = $terminalValidation.CompletedUtc
	}
}

function Invoke-ReleaseDocsEvidenceSelfTest {
	$statusForTest = Read-JsonFile $statusDataPath
	$statusAsOfForTest = Require-UtcTimestamp `
		(Get-ObjectPropertyValue $statusForTest "statusAsOfUtc") `
		"self-test release status time"
	$candidateForTest = Get-RetainedCandidateIdentity `
		(Get-ObjectPropertyValue $statusForTest "artifact") `
		"self-test active candidate"
	$sourceEvidence = Get-ObjectPropertyValue `
		(Get-ObjectPropertyValue $statusForTest "evidence") "fullCampaignDebug"
	if ($null -eq $sourceEvidence) {
		throw "Release-doc self-test requires the retained active full-profile fixture."
	}
	$trackedRedValidation = Assert-ActiveFullCampaignDebugEvidence `
		$sourceEvidence `
		$candidateForTest `
		"self-test tracked red full profile" `
		$statusAsOfForTest `
		$candidateForTest.RuntimeSettingsSchema
	if (-not $trackedRedValidation.Rejected -or $trackedRedValidation.Accepted) {
		throw "Release-doc self-test did not classify the tracked red fixture as rejected."
	}

	$tempLeaf = ".release-docs-selftest-$([Guid]::NewGuid().ToString('N')).json"
	$tempRelativePath = "tools/$tempLeaf"
	$tempFullPath = Join-Path $PSScriptRoot $tempLeaf
	$cloneJson = {
		param([object] $Value)
		return $Value | ConvertTo-Json -Depth 32 | ConvertFrom-Json
	}
	$writeSummary = {
		param([object] $Value)
		$json = $Value | ConvertTo-Json -Depth 32
		$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
		[IO.File]::WriteAllText($tempFullPath, $json + "`n", $utf8NoBom)
		return (Get-FileHash -LiteralPath $tempFullPath `
			-Algorithm SHA256).Hash.ToLowerInvariant()
	}
	$assertRejected = {
		param(
			[string] $Name,
			[scriptblock] $Action
		)
		$didReject = $false
		try {
			& $Action | Out-Null
		}
		catch {
			$didReject = $true
		}
		if (-not $didReject) {
			throw "Release-doc self-test expected fail-closed rejection for $Name."
		}
	}

	try {
		$sourceSummaryPath = Join-Path $root `
			([string] (Get-ObjectPropertyValue $sourceEvidence "summaryPath"))
		$redSummary = Get-Content -Raw -LiteralPath $sourceSummaryPath | ConvertFrom-Json
		$redEvidence = & $cloneJson $sourceEvidence
		$redEvidence.summaryPath = $tempRelativePath

		$greenSummary = & $cloneJson $redSummary
		$currentRunnerSha = (Get-FileHash -LiteralPath `
			(Join-Path $root "tools/run-guarded-campaign-debug.ps1") `
			-Algorithm SHA256).Hash.ToLowerInvariant()
		$currentRunnerPolicy = Resolve-CampaignRunnerPolicy `
			$currentRunnerSha "self-test current campaign runner"
		$greenSummary.harness.campaignRunnerSha256 = $currentRunnerSha
		$greenSummary.result.status = "passed-full-certification"
		$greenSummary.result.runtimeOutcomeSuccess = $true
		$greenSummary.result.acceptanceDisposition = "accepted-full-profile"
		$greenSummary.result.releaseDisposition = "advance-external-gates"
		$greenSummary.result.certificationPassed = $true
		$greenSummary.result.error = ""
		$greenSummary.proof.pass = [int] $greenSummary.proof.caseCount -
			[int] $greenSummary.proof.warn - [int] $greenSummary.proof.skipped
		$greenSummary.proof.fail = 0
		$greenSummary.proof.blocked = 0
		$greenSummary.proof.certificationProven =
			[int] $greenSummary.proof.certificationRequired
		$greenSummary.proof.certificationFail = 0
		$greenSummary.proof.certificationBlocked = 0
		$greenSummary.proof.certificationWarn = 0
		$greenSummary.proof.intentionalMissionConvoyAdmissionDiagnosticsProven =
			[bool] $currentRunnerPolicy.IntentionalAdmissionProven
		$greenSummary.proof.intentionalMissionConvoySettlementDiagnosticProven =
			[bool] $currentRunnerPolicy.IntentionalSettlementProven
		$greenSummary.proof.intentionalMissionConvoyCorruptionDiagnosticsProven =
			[bool] $currentRunnerPolicy.IntentionalCorruptionProven
		$greenSummary.proof.intentionalMissionConvoyWatchdogDiagnosticProven =
			[bool] $currentRunnerPolicy.IntentionalWatchdogProven
		$greenSummary.diagnostics.valid = $true
		$greenSummary.diagnostics.classificationValid = $true
		$greenSummary.diagnostics.hardDiagnosticFree = $false
		$greenSummary.diagnostics.hardDiagnosticCount =
			[int] $currentRunnerPolicy.HardDiagnosticCount
		$greenSummary.diagnostics.scriptErrors =
			[int] $currentRunnerPolicy.ScriptErrors
		$greenSummary.diagnostics.engineErrors =
			[int] $currentRunnerPolicy.EngineErrors
		$greenSummary.diagnostics.partisanErrors =
			[int] $currentRunnerPolicy.PartisanErrors
		$greenSummary.diagnostics.approvedStockDiagnosticCount =
			[int] $currentRunnerPolicy.ApprovedStockDiagnosticCount
		$greenSummary.diagnostics.approvedIntentionalDiagnosticCount =
			[int] $currentRunnerPolicy.ApprovedIntentionalDiagnosticCount
		$greenSummary.diagnostics.unapprovedHardDiagnosticCount = 0
		$greenSummary.diagnostics.unapprovedHardDiagnosticKinds = @()
		$greenSummary.diagnostics.classifierSelfTestCount =
			[int] $currentRunnerPolicy.ClassifierSelfTestCount
		$greenSummary.diagnostics.intentionalFixtureStructureExact = $true
		$greenSummary.diagnostics.intentionalFixtureSetValid = $true
		$greenSummary.finding.status = "accepted-full-profile"
		$greenSummary.finding.defect = "none"
		$greenSummary.finding.nextStep =
			"Advance the unchanged package to the external release gates."

		$greenEvidence = & $cloneJson $redEvidence
		$greenEvidence.status = "passed-full-certification"
		$greenEvidence.campaignRunnerSha256 = $currentRunnerSha
		$greenEvidence.runtimeOutcomeSuccess = $true
		$greenEvidence.acceptanceDisposition = "accepted-full-profile"
		$greenEvidence.certificationPassed = $true
		$greenEvidence.error = ""
		foreach ($field in @(
			"caseCount", "pass", "warn", "fail", "blocked", "skipped")) {
			$greenEvidence.$field = [int] $greenSummary.proof.$field
		}
		$greenEvidence.requiredAssertions =
			[int] $greenSummary.proof.certificationRequired
		$greenEvidence.provenAssertions =
			[int] $greenSummary.proof.certificationProven
		$greenEvidence.failedAssertions = [int] $greenSummary.proof.certificationFail
		$greenEvidence.blockedAssertions =
			[int] $greenSummary.proof.certificationBlocked
		$greenEvidence.diagnosticClassificationValid = $true
		$greenEvidence.hardDiagnosticFree = $false
		foreach ($field in @(
			"hardDiagnosticCount", "scriptErrors", "engineErrors", "partisanErrors",
			"approvedStockDiagnosticCount", "approvedIntentionalDiagnosticCount",
			"unapprovedHardDiagnosticCount", "classifierSelfTestCount",
			"canonicalScriptLogCount", "canonicalConsoleLogCount")) {
			$greenEvidence.$field = [int] $greenSummary.diagnostics.$field
		}
		$greenEvidence.summary =
			"Synthetic accepted full-profile fixture for release-doc validator self-test."
		$greenEvidence.summarySha256 = & $writeSummary $greenSummary
		$syntheticGreenValidation = Assert-ActiveFullCampaignDebugEvidence `
			$greenEvidence `
			$candidateForTest `
			"self-test synthetic green full profile" `
			$statusAsOfForTest `
			$candidateForTest.RuntimeSettingsSchema `
			-AllowUntrackedSummaryForSelfTest
		if (-not $syntheticGreenValidation.Accepted -or
			-not $syntheticGreenValidation.AcceptedFull -or
			$syntheticGreenValidation.AcceptedInternal -or
			$syntheticGreenValidation.Rejected) {
			throw "Release-doc self-test did not classify the synthetic green fixture as accepted."
		}

		$certificationRedSummary = & $cloneJson $greenSummary
		$certificationRedSummary.result.status = "failed-full-profile"
		$certificationRedSummary.result.runtimeOutcomeSuccess = $false
		$certificationRedSummary.result.acceptanceDisposition =
			"rejected-full-profile"
		$certificationRedSummary.result.releaseDisposition = "remain-no-go"
		$certificationRedSummary.result.certificationPassed = $false
		$certificationRedSummary.result.error = "Certification proof failed."
		$certificationRedSummary.proof.pass =
			[int] $certificationRedSummary.proof.pass - 1
		$certificationRedSummary.proof.fail = 1
		$certificationRedSummary.proof.certificationProven =
			[int] $certificationRedSummary.proof.certificationRequired - 1
		$certificationRedSummary.proof.certificationFail = 1
		$certificationRedSummary.finding.status =
			"release-blocking-red-full-profile"
		$certificationRedSummary.finding.defect =
			"Internal certification proof failed."
		$certificationRedSummary.finding.nextStep =
			"Repair the certification failure and seal a new candidate."
		$certificationRedEvidence = & $cloneJson $greenEvidence
		$certificationRedEvidence.status = "failed-full-profile"
		$certificationRedEvidence.runtimeOutcomeSuccess = $false
		$certificationRedEvidence.acceptanceDisposition = "rejected-full-profile"
		$certificationRedEvidence.certificationPassed = $false
		$certificationRedEvidence.error = "Certification proof failed."
		$certificationRedEvidence.pass = [int] $certificationRedSummary.proof.pass
		$certificationRedEvidence.fail = 1
		$certificationRedEvidence.provenAssertions =
			[int] $certificationRedSummary.proof.certificationProven
		$certificationRedEvidence.failedAssertions = 1
		$certificationRedEvidence.summary =
			"Synthetic certification-axis rejection for release-doc validator self-test."
		$certificationRedEvidence.summarySha256 =
			& $writeSummary $certificationRedSummary
		$certificationRedValidation = Assert-ActiveFullCampaignDebugEvidence `
			$certificationRedEvidence $candidateForTest `
			"self-test certification-axis red full profile" `
			$statusAsOfForTest $candidateForTest.RuntimeSettingsSchema `
			-AllowUntrackedSummaryForSelfTest
		if (-not $certificationRedValidation.Rejected -or
			$certificationRedValidation.Accepted) {
			throw "Release-doc self-test did not accept the certification-axis rejection."
		}

		$diagnosticRedSummary = & $cloneJson $greenSummary
		$diagnosticRedSummary.result.status = "failed-full-profile"
		$diagnosticRedSummary.result.runtimeOutcomeSuccess = $false
		$diagnosticRedSummary.result.acceptanceDisposition = "rejected-full-profile"
		$diagnosticRedSummary.result.releaseDisposition = "remain-no-go"
		$diagnosticRedSummary.result.certificationPassed = $true
		$diagnosticRedSummary.result.error = "Diagnostic acceptance failed."
		$diagnosticRedSummary.diagnostics.valid = $false
		$diagnosticRedSummary.diagnostics.classificationValid = $false
		$diagnosticRedSummary.diagnostics.hardDiagnosticCount =
			[int] $greenSummary.diagnostics.hardDiagnosticCount + 1
		$diagnosticRedSummary.diagnostics.scriptErrors =
			[int] $greenSummary.diagnostics.scriptErrors + 1
		$diagnosticRedSummary.diagnostics.partisanErrors =
			[int] $greenSummary.diagnostics.partisanErrors + 1
		$diagnosticRedSummary.diagnostics.unapprovedHardDiagnosticCount = 1
		$diagnosticRedSummary.diagnostics.unapprovedHardDiagnosticKinds = @(
			[PSCustomObject] @{ kind = "partisan-script-error"; count = 1 })
		$diagnosticRedSummary.finding.status =
			"release-blocking-red-full-profile"
		$diagnosticRedSummary.finding.defect = "Diagnostic acceptance failed."
		$diagnosticRedSummary.finding.nextStep =
			"Repair the diagnostic failure and seal a new candidate."
		$diagnosticRedEvidence = & $cloneJson $greenEvidence
		$diagnosticRedEvidence.status = "failed-full-profile"
		$diagnosticRedEvidence.runtimeOutcomeSuccess = $false
		$diagnosticRedEvidence.acceptanceDisposition = "rejected-full-profile"
		$diagnosticRedEvidence.certificationPassed = $true
		$diagnosticRedEvidence.error = "Diagnostic acceptance failed."
		$diagnosticRedEvidence.diagnosticClassificationValid = $false
		foreach ($field in @(
			"hardDiagnosticCount", "scriptErrors", "engineErrors", "partisanErrors",
			"approvedStockDiagnosticCount", "approvedIntentionalDiagnosticCount",
			"unapprovedHardDiagnosticCount")) {
			$diagnosticRedEvidence.$field =
				[int] $diagnosticRedSummary.diagnostics.$field
		}
		$diagnosticRedEvidence.summary =
			"Synthetic diagnostic-axis rejection for release-doc validator self-test."
		$diagnosticRedEvidence.summarySha256 = & $writeSummary $diagnosticRedSummary
		$diagnosticRedValidation = Assert-ActiveFullCampaignDebugEvidence `
			$diagnosticRedEvidence $candidateForTest `
			"self-test diagnostic-axis red full profile" `
			$statusAsOfForTest $candidateForTest.RuntimeSettingsSchema `
			-AllowUntrackedSummaryForSelfTest
		if (-not $diagnosticRedValidation.Rejected -or
			$diagnosticRedValidation.Accepted) {
			throw "Release-doc self-test did not accept the diagnostic-axis rejection."
		}

		$internalSummary = & $cloneJson $greenSummary
		$internalSummary.result.status =
			"passed-internal-profile-external-required"
		$internalSummary.result.acceptanceDisposition = "accepted-internal-profile"
		$internalSummary.result.releaseDisposition = "advance-external-required"
		$internalSummary.result.certificationPassed = $false
		$internalSummary.proof.pass = [int] $internalSummary.proof.pass - 3
		$internalSummary.proof.blocked = 3
		$internalSummary.proof.certificationProven =
			[int] $internalSummary.proof.certificationRequired -
			@($currentRunnerPolicy.ExternalRequiredBlockerIds).Count
		$internalSummary.proof.certificationBlocked =
			@($currentRunnerPolicy.ExternalRequiredBlockerIds).Count
		$internalSummary.proof | Add-Member -NotePropertyName `
			externalRequiredBlockerIds -NotePropertyValue `
			@($currentRunnerPolicy.ExternalRequiredBlockerIds)
		$internalSummary.finding.status =
			"accepted-internal-profile-external-required"
		$internalSummary.finding.nextStep =
			"Run the exact sealed external-required blocker set."
		$internalEvidence = & $cloneJson $greenEvidence
		$internalEvidence.status = "passed-internal-profile-external-required"
		$internalEvidence.acceptanceDisposition = "accepted-internal-profile"
		$internalEvidence.certificationPassed = $false
		$internalEvidence.pass = [int] $internalSummary.proof.pass
		$internalEvidence.blocked = 3
		$internalEvidence.provenAssertions =
			[int] $internalSummary.proof.certificationProven
		$internalEvidence.blockedAssertions =
			[int] $internalSummary.proof.certificationBlocked
		$internalEvidence | Add-Member -NotePropertyName `
			externalRequiredBlockerIds -NotePropertyValue `
			@($currentRunnerPolicy.ExternalRequiredBlockerIds)
		$internalEvidence.summary =
			"Synthetic internally accepted profile with only sealed external blockers."
		$internalEvidence.summarySha256 = & $writeSummary $internalSummary
		$internalValidation = Assert-ActiveFullCampaignDebugEvidence `
			$internalEvidence $candidateForTest `
			"self-test accepted internal profile" `
			$statusAsOfForTest $candidateForTest.RuntimeSettingsSchema `
			-AllowUntrackedSummaryForSelfTest
		if (-not $internalValidation.Accepted -or
			-not $internalValidation.AcceptedInternal -or
			$internalValidation.AcceptedFull -or $internalValidation.Rejected) {
			throw "Release-doc self-test did not classify the internal profile correctly."
		}

		$badExternalSummary = & $cloneJson $internalSummary
		$badExternalEvidence = & $cloneJson $internalEvidence
		$badExternalSummary.proof.externalRequiredBlockerIds[0] =
			"phase25.untrusted"
		$badExternalEvidence.externalRequiredBlockerIds[0] =
			"phase25.untrusted"
		$badExternalEvidence.summarySha256 = & $writeSummary $badExternalSummary
		& $assertRejected "invalid external-required blocker ID" {
			Assert-ActiveFullCampaignDebugEvidence `
				$badExternalEvidence $candidateForTest "self-test bad external blocker" `
				$statusAsOfForTest $candidateForTest.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest
		}

		$badFixtureSummary = & $cloneJson $greenSummary
		$badFixtureSummary.proof.intentionalMissionConvoySettlementDiagnosticProven =
			-not [bool] $currentRunnerPolicy.IntentionalSettlementProven
		$badFixtureEvidence = & $cloneJson $greenEvidence
		$badFixtureEvidence.summarySha256 = & $writeSummary $badFixtureSummary
		& $assertRejected "mismatched settlement assertion proof" {
			Assert-ActiveFullCampaignDebugEvidence `
				$badFixtureEvidence $candidateForTest "self-test bad fixture policy" `
				$statusAsOfForTest $candidateForTest.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest
		}

		$badHashEvidence = & $cloneJson $greenEvidence
		$badHashEvidence.summarySha256 = "0" * 64
		& $assertRejected "summary hash mismatch" {
			Assert-ActiveFullCampaignDebugEvidence `
				$badHashEvidence $candidateForTest "self-test bad hash" `
				$statusAsOfForTest $candidateForTest.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest
		}
		$badCandidateEvidence = & $cloneJson $greenEvidence
		$badCandidateEvidence.candidateId = "$($candidateForTest.CandidateId)-mismatch"
		& $assertRejected "candidate identity mismatch" {
			Assert-ActiveFullCampaignDebugEvidence `
				$badCandidateEvidence $candidateForTest "self-test bad candidate" `
				$statusAsOfForTest $candidateForTest.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest
		}
		$badCountEvidence = & $cloneJson $greenEvidence
		$badCountEvidence.pass = [int] $badCountEvidence.pass + 1
		& $assertRejected "retained count mismatch" {
			Assert-ActiveFullCampaignDebugEvidence `
				$badCountEvidence $candidateForTest "self-test bad count" `
				$statusAsOfForTest $candidateForTest.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest
		}
		$badCleanupSummary = & $cloneJson $greenSummary
		$badCleanupSummary.cleanup.guardRemaining = 1
		$badCleanupEvidence = & $cloneJson $greenEvidence
		$badCleanupEvidence.summarySha256 = & $writeSummary $badCleanupSummary
		& $assertRejected "nonzero cleanup residue" {
			Assert-ActiveFullCampaignDebugEvidence `
				$badCleanupEvidence $candidateForTest "self-test bad cleanup" `
				$statusAsOfForTest $candidateForTest.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest
		}
		$selfAuthoredExternalEvidence = [PSCustomObject] @{
			status = "failed-external-runtime"
			summaryPath = $tempRelativePath
			summarySha256 = $badCleanupEvidence.summarySha256
			candidateId = $candidateForTest.CandidateId
			candidateSourceHead = $candidateForTest.CandidateSourceHead
			packageSha256 = $candidateForTest.PackageSha256
			manifestSha256 = $candidateForTest.ManifestSha256
			readySha256 = $candidateForTest.ReadySha256
			harnessGitHead = $greenEvidence.harnessGitHead
			startedUtc = $greenEvidence.startedUtc
			completedUtc = $greenEvidence.completedUtc
			failedRung = "packaged-dedicated"
			artifactCount = 1
			artifactSetSha256 = $greenEvidence.envelopeSha256
			artifactsRehashed = $true
			cleanupAndSpillZero = $true
			acceptanceDisposition = "rejected-external-runtime"
			summary = "Self-authored external retirement claim."
		}
		& $assertRejected "self-authored external retirement" {
			Assert-HistoricalExternalRuntimeEvidence `
				$selfAuthoredExternalEvidence $candidateForTest `
				"self-test untrusted external retirement" $statusAsOfForTest
		}
	}
	finally {
		if (Test-Path -LiteralPath $tempFullPath -PathType Leaf) {
			Remove-Item -LiteralPath $tempFullPath -Force
		}
	}

	Write-Host "Release-doc evidence self-test passed: legacy red, accepted full/internal, two generic red axes, and 7 fail-closed boundaries."
}

function Resolve-ActionRule {
	param(
		[object] $Parity,
		[string] $ActionId
	)

	foreach ($rule in $Parity.actionRules) {
		if ([regex]::IsMatch($ActionId, [string] $rule.pattern)) {
			return $rule
		}
	}

	return $null
}

function Build-ConfiguredReward {
	param([object] $Mission)

	$parts = @()
	if ($Mission.Money -gt 0) {
		$parts += "`$$($Mission.Money)"
	}
	if ($Mission.HR -gt 0) {
		$parts += "$($Mission.HR) HR"
	}
	if ($parts.Count -eq 0) {
		$parts += "no direct money/HR"
	}

	if (-not [string]::IsNullOrWhiteSpace($Mission.RewardText)) {
		return ($parts -join ", ") + "; " + $Mission.RewardText
	}

	return $parts -join ", "
}

if ($LibraryOnly) {
	return
}
if ($Check -and $SelfTest) {
	throw "Use either -Check or -SelfTest, not both."
}
if ($SelfTest) {
	& (Join-Path $PSScriptRoot "test-partisan-campaign-debug-release-index.ps1")
	return
}

$statusDataText = Get-Content -Raw -LiteralPath $statusDataPath
$status = Read-JsonFile $statusDataPath
$parity = Read-JsonFile $parityDataPath

if ([int] (Get-ObjectPropertyValue $status "schemaVersion") -ne 3) {
	throw "release_status.json must use schemaVersion 3 with ordered historical candidate evidence."
}
$historicalCandidatePropertyMatches = [regex]::Matches(
	$statusDataText,
	'"historicalCandidateEvidence"\s*:')
if ($historicalCandidatePropertyMatches.Count -ne 1 -or
	-not [regex]::IsMatch(
		$statusDataText,
		'"historicalCandidateEvidence"\s*:\s*\[')) {
	throw "release_status.historicalCandidateEvidence must be a JSON array property."
}
$historicalCandidateProperty = $status.PSObject.Properties["historicalCandidateEvidence"]
$historicalCandidateValue = $historicalCandidateProperty.Value
if ($historicalCandidateValue -isnot [Array]) {
	throw "release_status.historicalCandidateEvidence must parse as a JSON array."
}

$buildInfoText = Get-Content -Raw -LiteralPath (Join-Path $root "scripts/Game/HST/HST_BuildInfo.c")
$campaignStateText = Get-Content -Raw -LiteralPath (Join-Path $root "scripts/Game/HST/State/HST_CampaignState.c")
$runtimeSettingsText = Get-Content -Raw -LiteralPath (Join-Path $root "scripts/Game/HST/Config/HST_RuntimeSettings.c")
$commandUIText = Get-Content -Raw -LiteralPath (Join-Path $root "scripts/Game/HST/Services/HST_CommandUIService.c")
$missionConfigText = Get-Content -Raw -LiteralPath (Join-Path $root "configs/HST/Missions/HST_CE311_Missions.conf")
$defaultCatalogText = Get-Content -Raw -LiteralPath (Join-Path $root "scripts/Game/HST/Config/HST_DefaultCatalog.c")

$sourceBuildSha = Get-RequiredMatch $buildInfoText 'BUILD_SHA\s*=\s*"(?<value>[0-9a-f]{40})"' "value" "embedded build SHA"
$sourceBuildUtc = Get-RequiredMatch $buildInfoText 'BUILD_UTC\s*=\s*"(?<value>[^"]+)"' "value" "embedded build UTC"
$sourceBuildLabel = Get-RequiredMatch $buildInfoText 'BUILD_LABEL\s*=\s*"(?<value>[^"]+)"' "value" "embedded build label"
$sourceCampaignSchema = [int] (Get-RequiredMatch $campaignStateText 'static const int SCHEMA_VERSION\s*=\s*(?<value>\d+)' "value" "campaign schema")
$sourceSettingsSchema = [int] (Get-RequiredMatch $runtimeSettingsText 'static const int SCHEMA_VERSION\s*=\s*(?<value>\d+)' "value" "runtime-settings schema")

if ([string] $status.baseline.embeddedImplementationSha -cne $sourceBuildSha) {
	throw "Release status embedded implementation SHA does not match HST_BuildInfo.c."
}
if ([string] $status.baseline.embeddedBuildUtc -cne $sourceBuildUtc) {
	throw "Release status embedded build UTC does not match HST_BuildInfo.c."
}
if ([string] $status.baseline.embeddedBuildLabel -cne $sourceBuildLabel) {
	throw "Release status embedded build label does not match HST_BuildInfo.c."
}
if ([int] $status.baseline.campaignSchema -ne $sourceCampaignSchema) {
	throw "Release status campaign schema does not match HST_CampaignState.c."
}
if ([int] $status.baseline.runtimeSettingsSchema -ne $sourceSettingsSchema) {
	throw "Release status runtime-settings schema does not match HST_RuntimeSettings.c."
}

$auditedRevision = Require-Text $status.auditedRevision "release_status.auditedRevision"
$releaseDecision = Require-Text $status.releaseDecision "release_status.releaseDecision"
if ($releaseDecision -cnotin @("GO", "NO-GO")) {
	throw "release_status.releaseDecision must be GO or NO-GO."
}

Push-Location $root
try {
	$checkoutHead = (& git rev-parse HEAD).Trim()
	if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($checkoutHead)) {
		throw "Could not resolve the checkout Git HEAD."
	}

	& git merge-base --is-ancestor $auditedRevision $checkoutHead
	if ($LASTEXITCODE -ne 0) {
		throw "The audited revision $auditedRevision is not an ancestor of checkout HEAD $checkoutHead."
	}

	$worktreeRows = @(& git status --porcelain)
}
finally {
	Pop-Location
}

$releaseCandidateBuiltValue = Get-ObjectPropertyValue `
	$status.artifact "releaseCandidateBuilt"
if ($releaseCandidateBuiltValue -isnot [bool] -or
	-not [bool] $releaseCandidateBuiltValue) {
	throw "Schema-3 release status requires releaseCandidateBuilt true."
}
$releaseCandidateBuilt = [bool] $releaseCandidateBuiltValue
$runtimeUseDisposition = ""
$candidateId = ""
$candidateSourceHead = ""
$candidateManifestPath = ""
$candidateManifestSha = ""
$candidateReadySha = ""
$packageHashAlgorithm = ""
$packageSha = ""
$packageVersion = ""
$addonGuid = ""
$addonRevision = ""
$workbenchVersion = ""
$workbenchSha = ""
$workbenchCrc = ""
$serverVersion = ""
$clientVersion = ""
$activePackagedFocused = Get-ObjectPropertyValue $status.evidence "packagedFocusedAutotests"
$activeCorrectedCanary = Get-ObjectPropertyValue $status.evidence "correctedForceAuthorityCanary"
$activeFullCampaignDebug = Get-ObjectPropertyValue $status.evidence "fullCampaignDebug"
$activePackagedFocusedValidation = $null
$activePackagedFocusedSummaryPath = ""
$activePackagedFocusedSummarySha = ""
$activePackagedFocusedHarnessHead = ""
$activeCorrectedCanaryValidation = $null
$activeCorrectedCanarySummaryPath = ""
$activeCorrectedCanarySummarySha = ""
$activeCorrectedCanaryHarnessHead = ""
$activeCorrectedCanaryStatus = ""
$activeFullCampaignDebugValidation = $null
$activeFullCampaignDebugSummaryPath = ""
$activeFullCampaignDebugSummarySha = ""
$activeFullCampaignDebugHarnessHead = ""
$historicalCandidateEntries = @($historicalCandidateValue)
$historicalCandidateResults = @()
if ($historicalCandidateEntries.Count -lt 1) {
	throw "release_status.historicalCandidateEvidence must contain one or more ordered entries."
}
if ($releaseCandidateBuilt) {
	$runtimeUseDisposition = Require-Text $status.artifact.runtimeUseDisposition "release_status.artifact.runtimeUseDisposition"
	if ($runtimeUseDisposition -cnotin @(
			"active-runtime-candidate",
			"supersede-before-runtime",
			"rejected-after-runtime")) {
		throw "release_status.artifact.runtimeUseDisposition is unsupported."
	}

	$candidateId = Require-Text $status.artifact.candidateId "release_status.artifact.candidateId"
	if ($candidateId -cnotmatch '^[A-Za-z0-9][A-Za-z0-9._-]*$') {
		throw "release_status.artifact.candidateId must use only letters, numbers, dot, underscore, and hyphen."
	}

	$candidateSourceHead = Require-Text $status.artifact.candidateSourceHead "release_status.artifact.candidateSourceHead"
	if ($candidateSourceHead -cnotmatch '^[0-9a-fA-F]{40}$') {
		throw "release_status.artifact.candidateSourceHead must be a full 40-character Git SHA."
	}

	$candidateManifestPath = Require-RepoRelativePath $status.artifact.manifestPath "release_status.artifact.manifestPath"
	$candidateManifestSha = Require-Sha256 $status.artifact.manifestSha256 "release_status.artifact.manifestSha256"
	$candidateReadySha = Require-Sha256 $status.artifact.readySha256 "release_status.artifact.readySha256"
	$packageHashAlgorithm = Require-Text $status.artifact.packageHashAlgorithm "release_status.artifact.packageHashAlgorithm"
	if ($packageHashAlgorithm -cne "sha256-manifest-v1") {
		throw "release_status.artifact.packageHashAlgorithm must be sha256-manifest-v1."
	}

	$packageSha = Require-Sha256 $status.artifact.packageSha256 "release_status.artifact.packageSha256"
	$packageVersion = Require-Text $status.artifact.packageVersion "release_status.artifact.packageVersion"
	$addonGuid = Require-Text $status.artifact.addonGuid "release_status.artifact.addonGuid"
	if ($addonGuid -cnotmatch '^[0-9a-fA-F]{16}$') {
		throw "release_status.artifact.addonGuid must be a 16-character addon GUID."
	}
	$addonRevision = Require-Text $status.artifact.addonRevision "release_status.artifact.addonRevision"
	$workbenchVersion = Require-Text $status.artifact.workbenchVersion "release_status.artifact.workbenchVersion"
	$workbenchSha = Require-Sha256 $status.artifact.workbenchSha256 "release_status.artifact.workbenchSha256"
	$workbenchCrc = Require-Text $status.artifact.workbenchCrc "release_status.artifact.workbenchCrc"
	if ($workbenchCrc -cnotmatch '^[0-9a-fA-F]{8}$') {
		throw "release_status.artifact.workbenchCrc must be an 8-character CRC32 value."
	}
	$serverVersion = Require-Text $status.artifact.serverVersion "release_status.artifact.serverVersion"
	$clientVersion = Require-Text $status.artifact.clientVersion "release_status.artifact.clientVersion"

	$activeCandidateIdentity = Get-RetainedCandidateIdentity `
		$status.artifact `
		"release_status.artifact"
	$statusAsOfUtc = Require-UtcTimestamp `
		$status.statusAsOfUtc `
		"release_status.statusAsOfUtc"
	if ($activeCandidateIdentity.CreatedUtc -gt $statusAsOfUtc) {
		throw "The active candidate creation time cannot exceed release_status.statusAsOfUtc."
	}

	$candidateManifestFullPath = [IO.Path]::GetFullPath(
		(Join-Path $root $candidateManifestPath))
	$repositoryPrefix = [IO.Path]::GetFullPath($root).TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
	if (-not $candidateManifestFullPath.StartsWith(
			$repositoryPrefix,
			[StringComparison]::OrdinalIgnoreCase) -or
		-not (Test-Path -LiteralPath $candidateManifestFullPath -PathType Leaf)) {
		throw "The tracked release-candidate manifest is missing or outside the repository."
	}
	$candidateManifestItem = Get-Item -LiteralPath $candidateManifestFullPath -Force
	if (($candidateManifestItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
		throw "The tracked release-candidate manifest must not be a reparse point."
	}
	$actualManifestSha = (Get-FileHash `
		-LiteralPath $candidateManifestFullPath `
		-Algorithm SHA256).Hash.ToLowerInvariant()
	if ($actualManifestSha -cne $candidateManifestSha.ToLowerInvariant()) {
		throw "The tracked release-candidate manifest SHA-256 does not match release status."
	}

	$candidateManifestText = Get-Content -Raw -LiteralPath $candidateManifestFullPath
	if ($candidateManifestText -match '(?i)[A-Z]:[\\/]') {
		throw "The tracked release-candidate manifest contains a local absolute path."
	}
	$candidateManifest = $candidateManifestText | ConvertFrom-Json
	$manifestCandidate = Get-ObjectPropertyValue $candidateManifest "candidate"
	$manifestSource = Get-ObjectPropertyValue $candidateManifest "source"
	$manifestAddon = Get-ObjectPropertyValue $candidateManifest "addon"
	$manifestToolchain = Get-ObjectPropertyValue $candidateManifest "toolchain"
	$manifestWorkbench = Get-ObjectPropertyValue $candidateManifest "workbench"
	$manifestPackage = Get-ObjectPropertyValue $candidateManifest "package"
	$manifestEmbedded = Get-ObjectPropertyValue $manifestSource "embeddedImplementation"
	if ([int] (Get-ObjectPropertyValue $candidateManifest "manifestSchemaVersion") -ne 1 -or
		$null -eq $manifestCandidate -or
		$null -eq $manifestSource -or
		$null -eq $manifestAddon -or
		$null -eq $manifestToolchain -or
		$null -eq $manifestWorkbench -or
		$null -eq $manifestPackage -or
		$null -eq $manifestEmbedded) {
		throw "The tracked release-candidate manifest is structurally incomplete."
	}
	$manifestWorkbenchTool = Get-ObjectPropertyValue $manifestToolchain "workbench"
	$manifestServerTool = Get-ObjectPropertyValue $manifestToolchain "server"
	$manifestServerDiagnosticTool = Get-ObjectPropertyValue $manifestToolchain "serverDiagnostic"
	$manifestClientTool = Get-ObjectPropertyValue $manifestToolchain "client"
	$manifestClientDiagnosticTool = Get-ObjectPropertyValue $manifestToolchain "clientDiagnostic"
	if (($null -eq $manifestServerDiagnosticTool) -xor
		($null -eq $manifestClientDiagnosticTool)) {
		throw "The tracked release-candidate manifest has an incomplete diagnostic executable identity pair."
	}
	if ($runtimeUseDisposition -ceq "active-runtime-candidate" -and
		($null -eq $manifestServerDiagnosticTool -or
			$null -eq $manifestClientDiagnosticTool)) {
		throw "An active runtime candidate must seal both diagnostic executable identities."
	}
	if ($null -ne $manifestServerDiagnosticTool) {
		if ([string] (Get-ObjectPropertyValue $manifestServerDiagnosticTool "fileName") -cne "ArmaReforgerServerDiag.exe" -or
			[string] (Get-ObjectPropertyValue $manifestClientDiagnosticTool "fileName") -cne "ArmaReforgerSteamDiag.exe" -or
			[string] (Get-ObjectPropertyValue $manifestServerDiagnosticTool "fileVersion") -cne $serverVersion -or
			[string] (Get-ObjectPropertyValue $manifestServerDiagnosticTool "productVersion") -cne
				[string] (Get-ObjectPropertyValue $manifestServerTool "productVersion") -or
			[long] (Get-ObjectPropertyValue $manifestServerDiagnosticTool "length") -le 0 -or
			[string] (Get-ObjectPropertyValue $manifestServerDiagnosticTool "sha256") -cnotmatch '^[0-9a-f]{64}$' -or
			[string] (Get-ObjectPropertyValue $manifestClientDiagnosticTool "fileVersion") -cne $clientVersion -or
			[string] (Get-ObjectPropertyValue $manifestClientDiagnosticTool "productVersion") -cne
				[string] (Get-ObjectPropertyValue $manifestClientTool "productVersion") -or
			[long] (Get-ObjectPropertyValue $manifestClientDiagnosticTool "length") -le 0 -or
			[string] (Get-ObjectPropertyValue $manifestClientDiagnosticTool "sha256") -cnotmatch '^[0-9a-f]{64}$') {
			throw "The tracked release-candidate diagnostic executable identity is invalid."
		}
	}
	if ([string] (Get-ObjectPropertyValue $manifestCandidate "id") -cne $candidateId -or
		[string] (Get-ObjectPropertyValue $manifestCandidate "version") -cne $packageVersion -or
		[string] (Get-ObjectPropertyValue $manifestCandidate "state") -cne "retained-uncertified" -or
		[string] (Get-ObjectPropertyValue $manifestSource "gitHead") -cne $candidateSourceHead -or
		(Get-ObjectPropertyValue $manifestSource "dirty") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $manifestSource "dirty") -or
		[string] (Get-ObjectPropertyValue $manifestSource "auditedGameplayRevision") -cne $auditedRevision -or
		[string] (Get-ObjectPropertyValue $manifestEmbedded "sha") -cne $sourceBuildSha -or
		[string] (Get-ObjectPropertyValue $manifestEmbedded "utc") -cne $sourceBuildUtc -or
		[string] (Get-ObjectPropertyValue $manifestEmbedded "label") -cne $sourceBuildLabel -or
		[int] (Get-ObjectPropertyValue $manifestSource "campaignSchema") -ne $sourceCampaignSchema -or
		[int] (Get-ObjectPropertyValue $manifestSource "runtimeSettingsSchema") -ne $sourceSettingsSchema -or
		[string] (Get-ObjectPropertyValue $manifestAddon "guid") -cne $addonGuid -or
		[string] (Get-ObjectPropertyValue $manifestAddon "revision") -cne $addonRevision -or
		[string] (Get-ObjectPropertyValue $manifestAddon "version") -cne $packageVersion -or
		$null -eq $manifestWorkbenchTool -or
		$null -eq $manifestServerTool -or
		$null -eq $manifestClientTool -or
		[string] (Get-ObjectPropertyValue $manifestWorkbenchTool "fileVersion") -cne $workbenchVersion -or
		[string] (Get-ObjectPropertyValue $manifestWorkbenchTool "sha256") -cne $workbenchSha -or
		[string] (Get-ObjectPropertyValue $manifestServerTool "fileVersion") -cne $serverVersion -or
		[string] (Get-ObjectPropertyValue $manifestClientTool "fileVersion") -cne $clientVersion -or
		[string] (Get-ObjectPropertyValue $manifestWorkbench "crc") -cne $workbenchCrc -or
		[string] (Get-ObjectPropertyValue $manifestPackage "hashAlgorithm") -cne $packageHashAlgorithm -or
		[string] (Get-ObjectPropertyValue $manifestPackage "sha256") -cne $packageSha) {
		throw "Release status differs from its tracked release-candidate manifest."
	}
	$manifestPackageFiles = @(Get-ObjectPropertyValue $manifestPackage "files")
	$manifestPackagePaths = @($manifestPackageFiles | ForEach-Object {
		[string] (Get-ObjectPropertyValue $_ "path")
	})
	if ($manifestPackagePaths.Count -ne 4) {
		throw "The tracked release-candidate manifest must contain exactly four package files."
	}
	Assert-UniqueStrings $manifestPackagePaths "Tracked release-candidate package files"
	Assert-EqualSet @(
		"package/Partisan/addon.gproj",
		"package/Partisan/data.pak",
		"package/Partisan/resourceDatabase.rdb",
		"package/Partisan/thumbnail.png") `
		$manifestPackagePaths `
		"Tracked release-candidate package files"

	$foundationEvidenceSource = Require-Text `
		$status.evidence.foundation.sourceSha `
		"release_status.evidence.foundation.sourceSha"
	$workbenchEvidenceSource = Require-Text `
		$status.evidence.workbench.sourceSha `
		"release_status.evidence.workbench.sourceSha"
	if ([string] $status.evidence.foundation.status -cne "passed" -or
		[string] $status.evidence.workbench.status -cne "passed" -or
		$foundationEvidenceSource -cne $candidateSourceHead -or
		$workbenchEvidenceSource -cne $candidateSourceHead -or
		[int] $status.evidence.foundation.referenceCount -le 0) {
		throw "Retained Foundation or Workbench status is not bound to the candidate source HEAD."
	}
	$manifestTargets = @(Get-ObjectPropertyValue $manifestWorkbench "targets")
	$manifestTargetNames = @($manifestTargets | ForEach-Object {
		[string] (Get-ObjectPropertyValue $_ "target")
	})
	if ($manifestTargets.Count -ne 5) {
		throw "The tracked release-candidate manifest must contain exactly five Workbench targets."
	}
	Assert-UniqueStrings $manifestTargetNames "Tracked release-candidate Workbench targets"
	Assert-EqualSet @("PC", "PS4", "PS5", "XBOX_ONE", "XBOX_SERIES") `
		$manifestTargetNames `
		"Tracked release-candidate Workbench targets"
	foreach ($manifestTarget in $manifestTargets) {
		if ([string] (Get-ObjectPropertyValue $manifestTarget "status") -cne "passed" -or
			[int] (Get-ObjectPropertyValue $manifestTarget "files") -ne
				[int] $status.evidence.workbench.fileCount -or
			[int] (Get-ObjectPropertyValue $manifestTarget "classes") -ne
				[int] $status.evidence.workbench.classCount -or
			[string] (Get-ObjectPropertyValue $manifestTarget "crc") -cne $workbenchCrc) {
			throw "A tracked release-candidate Workbench target differs from retained status."
		}
	}
	if ([int] $status.evidence.workbench.fileCount -le 0 -or
		[int] $status.evidence.workbench.classCount -le 0 -or
		[string] $status.evidence.workbench.crc -cne $workbenchCrc) {
		throw "Retained Workbench counts or CRC differ from the candidate manifest."
	}

	$candidateReadyFullPath = Join-Path `
		(Split-Path -Parent $candidateManifestFullPath) `
		"candidate.ready.json"
	if (-not (Test-Path -LiteralPath $candidateReadyFullPath -PathType Leaf)) {
		throw "The tracked release candidate is missing its ready seal."
	}
	$candidateReadyItem = Get-Item -LiteralPath $candidateReadyFullPath -Force
	if (($candidateReadyItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
		throw "The tracked release-candidate ready seal must not be a reparse point."
	}
	$actualReadySha = (Get-FileHash `
		-LiteralPath $candidateReadyFullPath `
		-Algorithm SHA256).Hash.ToLowerInvariant()
	if ($actualReadySha -cne $candidateReadySha.ToLowerInvariant()) {
		throw "The tracked release-candidate ready-seal SHA-256 does not match release status."
	}
	$candidateReady = Get-Content -Raw -LiteralPath $candidateReadyFullPath | ConvertFrom-Json
	if ([int] (Get-ObjectPropertyValue $candidateReady "schemaVersion") -ne 1 -or
		[string] (Get-ObjectPropertyValue $candidateReady "candidateId") -cne $candidateId -or
		[string] (Get-ObjectPropertyValue $candidateReady "gitHead") -cne $candidateSourceHead -or
		[string] (Get-ObjectPropertyValue $candidateReady "packageSha256") -cne $packageSha -or
		[string] (Get-ObjectPropertyValue $candidateReady "manifestSha256") -cne $candidateManifestSha) {
		throw "The tracked release-candidate ready seal differs from release status."
	}

	$deterministicServiceRungs = @($status.proofRungs | Where-Object {
		[string] (Get-ObjectPropertyValue $_ "id") -ceq "deterministic-service"
	})
	if ($deterministicServiceRungs.Count -ne 1) {
		throw "Release status must contain exactly one deterministic-service proof rung."
	}
	$deterministicServiceRungStatus = [string] (Get-ObjectPropertyValue `
		$deterministicServiceRungs[0] `
		"status")
	if ($null -eq $activePackagedFocused) {
		if ($deterministicServiceRungStatus -cne "not-run") {
			throw "A missing active packaged focused result requires a not-run deterministic-service rung."
		}
	}
	else {
		if ($deterministicServiceRungStatus -cne "passed-noncertifying" -or
			[string] (Get-ObjectPropertyValue $activePackagedFocused "status") -cne
				"passed-noncertifying") {
			throw "Active packaged focused evidence and its deterministic-service rung must both be passed-noncertifying."
		}
	}
	if ($null -eq $activePackagedFocused -and
		($null -ne $activeCorrectedCanary -or $null -ne $activeFullCampaignDebug)) {
		throw "Active Campaign Debug evidence cannot precede the active packaged focused result."
	}
	if ($null -eq $activeCorrectedCanary -and $null -ne $activeFullCampaignDebug) {
		throw "Active full-profile evidence cannot precede the corrected active canary."
	}
	$nativeEngineWorldRungs = @($status.proofRungs | Where-Object {
		[string] (Get-ObjectPropertyValue $_ "id") -ceq "native-engine-world"
	})
	if ($nativeEngineWorldRungs.Count -ne 1) {
		throw "Release status must contain exactly one native-engine-world proof rung."
	}
	$nativeEngineWorldRungStatus = [string] (Get-ObjectPropertyValue `
		$nativeEngineWorldRungs[0] `
		"status")
	if ($null -eq $activeCorrectedCanary -and $null -eq $activeFullCampaignDebug -and
		$nativeEngineWorldRungStatus -cne "not-run") {
		throw "Missing active Campaign Debug evidence requires a not-run native-engine-world rung."
	}
	if ($null -ne $activeCorrectedCanary) {
		$activeCorrectedCanaryStatus = Require-Text `
			(Get-ObjectPropertyValue $activeCorrectedCanary "status") `
			"release_status.evidence.correctedForceAuthorityCanary.status"
	}
	if ($null -ne $activeCorrectedCanary -and $null -eq $activeFullCampaignDebug) {
		if ($activeCorrectedCanaryStatus -ceq "passed-noncertifying") {
			if ($nativeEngineWorldRungStatus -cne "passed-noncertifying") {
				throw "An accepted scoped canary requires a passed-noncertifying native-engine-world rung."
			}
		}
		elseif ($activeCorrectedCanaryStatus -ceq "failed-proof-validation") {
			if ($nativeEngineWorldRungStatus -cne "failed") {
				throw "A rejected scoped canary requires a failed native-engine-world rung."
			}
		}
		else {
			throw "Active corrected force-authority canary status is unsupported."
		}
	}
	if ($null -ne $activePackagedFocused) {
		$activePackagedFocusedValidation = Assert-PackagedFocusedEvidence `
			$activePackagedFocused `
			$activeCandidateIdentity `
			"passed-noncertifying" `
			"release_status.evidence.packagedFocusedAutotests"
		$activePackagedFocusedSummaryPath = $activePackagedFocusedValidation.SummaryPath
		$activePackagedFocusedSummarySha = $activePackagedFocusedValidation.SummarySha256
		$activePackagedFocusedHarnessHead = $activePackagedFocusedValidation.HarnessGitHead
		if ($statusAsOfUtc -lt $activePackagedFocusedValidation.AcceptedCompletedUtc) {
			throw "Release status cannot predate the active packaged focused evidence window."
		}
		if ($activeCandidateIdentity.CreatedUtc -gt
				$activePackagedFocusedValidation.AcceptedStartedUtc) {
			throw "Active packaged focused evidence cannot predate candidate creation."
		}
	}
	if ($null -ne $activeCorrectedCanary) {
		$activeCorrectedCanaryExpectedOutcome = "accepted"
		if ($activeCorrectedCanaryStatus -ceq "failed-proof-validation") {
			$activeCorrectedCanaryExpectedOutcome = "rejected-proof"
		}
		$activeCorrectedCanaryValidation = Assert-CorrectedCanaryEvidence `
			$activeCorrectedCanary `
			$activeCandidateIdentity `
			$activeCorrectedCanaryStatus `
			$activeCorrectedCanaryExpectedOutcome `
			"release_status.evidence.correctedForceAuthorityCanary" `
			$statusAsOfUtc `
			$activeCandidateIdentity.RuntimeSettingsSchema
		$activeCorrectedCanarySummaryPath = $activeCorrectedCanaryValidation.SummaryPath
		$activeCorrectedCanarySummarySha = $activeCorrectedCanaryValidation.SummarySha256
		$activeCorrectedCanaryHarnessHead = $activeCorrectedCanaryValidation.HarnessGitHead
		if ($null -eq $activePackagedFocusedValidation -or
			$activeCorrectedCanaryValidation.StartedUtc -lt
				$activePackagedFocusedValidation.AcceptedCompletedUtc) {
			throw "Active corrected-canary evidence must start after the packaged focused set completed."
		}
	}
	if ($null -ne $activeFullCampaignDebug) {
		if ($activeCorrectedCanaryStatus -cne "passed-noncertifying") {
			throw "Active full-profile evidence requires an accepted corrected canary."
		}
		$activeFullCampaignDebugValidation = Assert-ActiveFullCampaignDebugEvidence `
			$activeFullCampaignDebug `
			$activeCandidateIdentity `
			"release_status.evidence.fullCampaignDebug" `
			$statusAsOfUtc `
			$activeCandidateIdentity.RuntimeSettingsSchema
		$activeFullCampaignDebugSummaryPath = $activeFullCampaignDebugValidation.SummaryPath
		$activeFullCampaignDebugSummarySha = $activeFullCampaignDebugValidation.SummarySha256
		$activeFullCampaignDebugHarnessHead = $activeFullCampaignDebugValidation.HarnessGitHead
		if ($null -eq $activeCorrectedCanaryValidation -or
			$activeFullCampaignDebugValidation.StartedUtc -lt
				$activeCorrectedCanaryValidation.CompletedUtc) {
			throw "Active full-profile evidence must start after the accepted corrected canary completed."
		}
		$expectedNativeEngineWorldStatus = "failed"
		if ($activeFullCampaignDebugValidation.Accepted) {
			$expectedNativeEngineWorldStatus = "passed-noncertifying"
		}
		if ($nativeEngineWorldRungStatus -cne $expectedNativeEngineWorldStatus) {
			throw "The active full-profile disposition does not match the native-engine-world rung."
		}
	}
	$hasRejectedRuntimeProof = $activeCorrectedCanaryStatus -ceq
		"failed-proof-validation" -or
		($null -ne $activeFullCampaignDebugValidation -and
			$activeFullCampaignDebugValidation.Rejected)
	if ($runtimeUseDisposition -ceq "rejected-after-runtime") {
		if (-not $hasRejectedRuntimeProof) {
			throw "A rejected-after-runtime candidate requires retained rejected runtime proof."
		}
	}
	elseif ($hasRejectedRuntimeProof) {
		throw "Retained rejected runtime proof requires the rejected-after-runtime disposition."
	}
	elseif ($null -ne $activeFullCampaignDebugValidation -and
		$activeFullCampaignDebugValidation.Accepted -and
		$runtimeUseDisposition -cne "active-runtime-candidate") {
		throw "An accepted active full profile requires the active-runtime-candidate disposition."
	}

	for ($historyIndex = 0; $historyIndex -lt $historicalCandidateEntries.Count; $historyIndex++) {
		$historicalCandidateResult = Assert-HistoricalCandidateEvidenceEntry `
			$historicalCandidateEntries[$historyIndex] `
			$historyIndex `
			$statusAsOfUtc
		$historicalCandidateResults += $historicalCandidateResult
	}

	$allCandidateIdentities = @($activeCandidateIdentity) + @(
		$historicalCandidateResults | ForEach-Object { $_.Identity })
	foreach ($identityField in @(
		"CandidateId",
		"CandidateSourceHead",
		"ManifestPath",
		"ManifestSha256",
		"ReadySha256",
		"PackageSha256",
		"PackageVersion")) {
		$identityValues = @($allCandidateIdentities | ForEach-Object {
			([string] $_.$identityField).ToLowerInvariant()
		})
		Assert-UniqueStrings `
			$identityValues `
			"Active and historical candidate $identityField values"
	}

	$evidenceSummaryPaths = @()
	$evidenceSummaryHashes = @()
	foreach ($activeValidation in @(
		$activePackagedFocusedValidation,
		$activeCorrectedCanaryValidation,
		$activeFullCampaignDebugValidation)) {
		if ($null -ne $activeValidation) {
			$evidenceSummaryPaths +=
				([string] $activeValidation.SummaryPath).ToLowerInvariant()
			$evidenceSummaryHashes +=
				([string] $activeValidation.SummarySha256).ToLowerInvariant()
		}
	}
	foreach ($historicalCandidateResult in $historicalCandidateResults) {
		foreach ($historicalValidation in @(
			$historicalCandidateResult.PackagedFocusedValidation,
			$historicalCandidateResult.CorrectedCanaryValidation,
			$historicalCandidateResult.FullCampaignDebugValidation,
			$historicalCandidateResult.ExternalRuntimeValidation)) {
			if ($null -ne $historicalValidation) {
				$evidenceSummaryPaths +=
					([string] $historicalValidation.SummaryPath).ToLowerInvariant()
				$evidenceSummaryHashes +=
					([string] $historicalValidation.SummarySha256).ToLowerInvariant()
			}
		}
	}
	Assert-UniqueStrings `
		$evidenceSummaryPaths `
		"Active and historical evidence summary paths"
	Assert-UniqueStrings `
		$evidenceSummaryHashes `
		"Active and historical evidence summary SHA-256 values"

	$focusedEvidenceValidations = @()
	if ($null -ne $activePackagedFocusedValidation) {
		$focusedEvidenceValidations += $activePackagedFocusedValidation
	}
	$campaignEvidenceValidations = @()
	foreach ($activeCampaignValidation in @(
		$activeCorrectedCanaryValidation,
		$activeFullCampaignDebugValidation)) {
		if ($null -ne $activeCampaignValidation) {
			$campaignEvidenceValidations += $activeCampaignValidation
		}
	}
	foreach ($historicalCandidateResult in $historicalCandidateResults) {
		$focusedEvidenceValidations +=
			$historicalCandidateResult.PackagedFocusedValidation
		foreach ($historicalCampaignValidation in @(
			$historicalCandidateResult.CorrectedCanaryValidation,
			$historicalCandidateResult.FullCampaignDebugValidation)) {
			if ($null -ne $historicalCampaignValidation) {
				$campaignEvidenceValidations += $historicalCampaignValidation
			}
		}
	}

	$focusedCaseRunIds = @()
	$focusedCaseEnvelopeHashes = @()
	foreach ($focusedEvidenceValidation in $focusedEvidenceValidations) {
		$focusedCaseRunIds += @($focusedEvidenceValidation.CaseRunIds)
		$focusedCaseEnvelopeHashes += @(
			$focusedEvidenceValidation.CaseEnvelopeSha256s)
	}
	$campaignRunIds = @($campaignEvidenceValidations | ForEach-Object {
		[string] $_.RunId
	})
	$campaignRunLeafIds = @($campaignEvidenceValidations | ForEach-Object {
		[string] $_.RunLeafId
	})
	$campaignEnvelopeHashes = @($campaignEvidenceValidations | ForEach-Object {
		([string] $_.EnvelopeSha256).ToLowerInvariant()
	})
	$campaignRunSummaryHashes = @($campaignEvidenceValidations | ForEach-Object {
		([string] $_.RunSummarySha256).ToLowerInvariant()
	})
	Assert-UniqueStrings `
		$focusedCaseRunIds `
		"Active and historical focused case run IDs"
	Assert-UniqueStrings `
		$focusedCaseEnvelopeHashes `
		"Active and historical focused case envelope SHA-256 values"
	Assert-UniqueStrings `
		$campaignRunIds `
		"Active and historical Campaign Debug run IDs"
	Assert-UniqueStrings `
		$campaignRunLeafIds `
		"Active and historical Campaign Debug run-leaf IDs"
	Assert-UniqueStrings `
		$campaignEnvelopeHashes `
		"Active and historical Campaign Debug envelope SHA-256 values"
	Assert-UniqueStrings `
		$campaignRunSummaryHashes `
		"Active and historical Campaign Debug run-summary SHA-256 values"
	Assert-UniqueStrings `
		(@($focusedCaseRunIds) + @($campaignRunLeafIds)) `
		"Active and historical evidence run-leaf identities"
	Assert-UniqueStrings `
		(@($focusedCaseEnvelopeHashes) + @($campaignEnvelopeHashes)) `
		"Active and historical canonical evidence envelope SHA-256 values"

	for ($historyIndex = 0; $historyIndex -lt $historicalCandidateResults.Count; $historyIndex++) {
		$historicalCandidateResult = $historicalCandidateResults[$historyIndex]
		$nextIdentity = $activeCandidateIdentity
		$nextFocusedStartedUtc = $null
		if ($historyIndex + 1 -lt $historicalCandidateResults.Count) {
			$nextIdentity = $historicalCandidateResults[$historyIndex + 1].Identity
			$nextFocusedStartedUtc =
				$historicalCandidateResults[$historyIndex + 1].PackagedFocusedValidation.AcceptedStartedUtc
		}
		elseif ($null -ne $activePackagedFocusedValidation) {
			$nextFocusedStartedUtc = $activePackagedFocusedValidation.AcceptedStartedUtc
		}
		if ($historicalCandidateResult.Identity.CreatedUtc -ge $nextIdentity.CreatedUtc) {
			throw "Historical candidate entries must be ordered oldest to newest before the active candidate."
		}
		if ($historicalCandidateResult.TerminalCompletedUtc -gt $nextIdentity.CreatedUtc) {
			throw "A historical terminal proof cannot complete after the next candidate was created."
		}
		if ($null -ne $nextFocusedStartedUtc -and
			$historicalCandidateResult.TerminalCompletedUtc -gt $nextFocusedStartedUtc) {
			throw "A historical terminal proof cannot complete after the next candidate's focused proof started."
		}
	}

	Assert-GitAncestor `
		$auditedRevision `
		$activeCandidateIdentity.CandidateSourceHead `
		"The audited revision is not an ancestor of the active candidate source HEAD"
	Assert-GitAncestor `
		$activeCandidateIdentity.CandidateSourceHead `
		$checkoutHead `
		"The active candidate source HEAD is not an ancestor of checkout HEAD"
	foreach ($historicalCandidateResult in $historicalCandidateResults) {
		$historicalIdentity = $historicalCandidateResult.Identity
		Assert-GitAncestor `
			$auditedRevision `
			$historicalIdentity.CandidateSourceHead `
			"The audited revision is not an ancestor of historical candidate $($historicalIdentity.CandidateId)"
		Assert-GitAncestor `
			$historicalIdentity.CandidateSourceHead `
			$historicalCandidateResult.PackagedFocusedValidation.HarnessGitHead `
			"Historical candidate source is not an ancestor of its packaged-focused harness"
		Assert-GitAncestor `
			$historicalCandidateResult.PackagedFocusedValidation.HarnessGitHead `
			$historicalCandidateResult.CorrectedCanaryValidation.HarnessGitHead `
			"Historical packaged-focused harness is not an ancestor of its corrected-canary harness"
		if ($null -ne $historicalCandidateResult.FullCampaignDebugValidation) {
			Assert-GitAncestor `
				$historicalCandidateResult.CorrectedCanaryValidation.HarnessGitHead `
				$historicalCandidateResult.FullCampaignDebugValidation.HarnessGitHead `
				"Historical corrected-canary harness is not an ancestor of its full-profile harness"
		}
		if ($null -ne $historicalCandidateResult.ExternalRuntimeValidation) {
			Assert-GitAncestor `
				$historicalCandidateResult.FullCampaignDebugValidation.HarnessGitHead `
				$historicalCandidateResult.ExternalRuntimeValidation.HarnessGitHead `
				"Historical full-profile harness is not an ancestor of its external-runtime harness"
		}
		$nextIdentity = $activeCandidateIdentity
		if ($historicalCandidateResult.Index + 1 -lt $historicalCandidateResults.Count) {
			$nextIdentity =
				$historicalCandidateResults[$historicalCandidateResult.Index + 1].Identity
		}
		Assert-GitAncestor `
			$historicalCandidateResult.TerminalHarnessGitHead `
			$nextIdentity.CandidateSourceHead `
			"Historical terminal harness is not an ancestor of the next candidate source"
	}

	$activeTerminalHarnessHead = $activeCandidateIdentity.CandidateSourceHead
	if ($null -ne $activePackagedFocusedValidation) {
		Assert-GitAncestor `
			$activeCandidateIdentity.CandidateSourceHead `
			$activePackagedFocusedValidation.HarnessGitHead `
			"Active candidate source is not an ancestor of its packaged-focused harness"
		$activeTerminalHarnessHead = $activePackagedFocusedValidation.HarnessGitHead
	}
	if ($null -ne $activeCorrectedCanaryValidation) {
		Assert-GitAncestor `
			$activeTerminalHarnessHead `
			$activeCorrectedCanaryValidation.HarnessGitHead `
			"Active packaged-focused harness is not an ancestor of its corrected-canary harness"
		$activeTerminalHarnessHead = $activeCorrectedCanaryValidation.HarnessGitHead
	}
	if ($null -ne $activeFullCampaignDebugValidation) {
		Assert-GitAncestor `
			$activeTerminalHarnessHead `
			$activeFullCampaignDebugValidation.HarnessGitHead `
			"Active corrected-canary harness is not an ancestor of its full-profile harness"
		$activeTerminalHarnessHead = $activeFullCampaignDebugValidation.HarnessGitHead
	}
	Assert-GitAncestor `
		$activeTerminalHarnessHead `
		$checkoutHead `
		"The active candidate terminal harness is not an ancestor of checkout HEAD"

}

$allowedProofRungStatuses = @("passed", "passed-noncertifying", "partial", "historical-failed", "failed", "blocked", "not-run")
$proofRungIds = @()
$proofRungById = @{}
foreach ($rung in $status.proofRungs) {
	$id = Require-Text $rung.id "proof rung id"
	$proofRungIds += $id
	$proofRungById[$id] = $rung
	Require-Text $rung.label "proof rung $id label" | Out-Null
	Require-Text $rung.summary "proof rung $id summary" | Out-Null
	if ([string] $rung.status -cnotin $allowedProofRungStatuses) {
		throw "Proof rung $id uses unknown status '$($rung.status)'."
	}
}
Assert-UniqueStrings $proofRungIds "Proof rung IDs"
$activePackageChainIncomplete = $null -eq $activePackagedFocused -or
	$null -eq $activeCorrectedCanary -or
	$null -eq $activeFullCampaignDebug
if ($activePackageChainIncomplete) {
	$canaryRung = $proofRungById["canary"]
	$stableCertificationRung = $proofRungById["stable-certification"]
	if ($releaseDecision -cne "NO-GO" -or
		$null -eq $canaryRung -or
		[string] (Get-ObjectPropertyValue $canaryRung "status") -cne "blocked" -or
		$null -eq $stableCertificationRung -or
		[string] (Get-ObjectPropertyValue $stableCertificationRung "status") -cne "blocked") {
		throw "An incomplete active package-evidence chain requires NO-GO with blocked canary and stable-certification rungs."
	}
}
if ($null -ne $activeFullCampaignDebug) {
	$nativeEngineWorldRung = $proofRungById["native-engine-world"]
	$canaryRung = $proofRungById["canary"]
	$stableCertificationRung = $proofRungById["stable-certification"]
	$expectedNativeEngineWorldStatus = "failed"
	if ($activeFullCampaignDebugValidation.Accepted) {
		$expectedNativeEngineWorldStatus = "passed-noncertifying"
	}
	if ($releaseDecision -cne "NO-GO" -or
		$null -eq $nativeEngineWorldRung -or
		[string] (Get-ObjectPropertyValue $nativeEngineWorldRung "status") -cne
			$expectedNativeEngineWorldStatus -or
		$null -eq $canaryRung -or
		[string] (Get-ObjectPropertyValue $canaryRung "status") -cne "blocked" -or
		$null -eq $stableCertificationRung -or
		[string] (Get-ObjectPropertyValue $stableCertificationRung "status") -cne "blocked") {
		throw "An active full profile requires NO-GO, its matching native-engine-world disposition, and blocked canary and stable-certification rungs."
	}
}

$baselineCommit = Require-Text $parity.baseline.tagCommit "parity baseline tagCommit"
if ($baselineCommit -cnotmatch '^[0-9a-f]{40}$') {
	throw "Parity baseline tagCommit must be a full 40-character Git SHA."
}

$allowedDispositions = @($parity.dispositions | ForEach-Object { [string] $_ })
Assert-UniqueStrings $allowedDispositions "Parity dispositions"
$requiredDispositionSet = @("exact", "partial", "legacy", "missing", "deferred", "deliberate-divergence", "development-only")
Assert-EqualSet $requiredDispositionSet $allowedDispositions "Parity disposition vocabulary"

$requiredRowFields = @(
	"id", "domain", "priority", "upstreamChangeHistory", "observableContract",
	"formulaContract", "reforgerAdaptation", "partisanStateOwner", "partisanServiceOwner",
	"physicalBehavior", "virtualBehavior", "transactionPolicy", "persistenceMigration",
	"clientProjection", "disposition", "blockingMilestone"
)
$rowIds = @()
$rowById = @{}
foreach ($row in $parity.contractRows) {
	foreach ($field in $requiredRowFields) {
		Require-Text (Get-ObjectPropertyValue $row $field) "parity row field $field" | Out-Null
	}
	Require-Count @($row.upstreamEvidence) 1 "Parity row $($row.id) upstreamEvidence"
	Require-Count @($row.proofIds) 1 "Parity row $($row.id) proofIds"
	if ([string] $row.priority -cnotin @("P0", "P1", "P2")) {
		throw "Parity row $($row.id) uses unknown priority '$($row.priority)'."
	}
	if ([string] $row.disposition -cnotin $allowedDispositions) {
		throw "Parity row $($row.id) uses unknown disposition '$($row.disposition)'."
	}
	foreach ($evidenceUrl in $row.upstreamEvidence) {
		if ([string] $evidenceUrl -cnotmatch '^https://') {
			throw "Parity row $($row.id) has a non-HTTPS upstream evidence link."
		}
	}
	$rowIds += [string] $row.id
	$rowById[[string] $row.id] = $row
}
Assert-UniqueStrings $rowIds "Parity row IDs"

$missionBlocks = [regex]::Matches($missionConfigText, 'HST_MissionDefinition\s*\{(?<body>[^}]*)\}')
$missions = @()
foreach ($missionBlock in $missionBlocks) {
	$body = $missionBlock.Groups["body"].Value
	$id = Get-MissionField $body "m_sMissionId"
	$name = Get-MissionField $body "m_sDisplayName"
	$category = Get-RequiredMatch $body 'm_eCategory\s+(?<value>HST_MISSION_[A-Z_]+)' "value" "mission category for $id"
	$runtime = Get-MissionField $body "m_sRuntimeType"
	$rowId = [string] (Get-ObjectPropertyValue $parity.missionCategoryRows $category)
	if ([string]::IsNullOrWhiteSpace($rowId) -or -not $rowById.ContainsKey($rowId)) {
		throw "Mission $id category $category has no valid parity row mapping."
	}

	$missions += [pscustomobject]@{
		Id = $id
		Name = $name
		Category = $category
		Runtime = $runtime
		Money = Get-MissionIntField $body "m_iRewardMoney"
		HR = Get-MissionIntField $body "m_iRewardHR"
		RewardText = Get-MissionField $body "m_sRewardText"
		RowId = $rowId
	}
}

if ($missions.Count -ne 39) {
	throw "Expected exactly 39 configured CE 3.11.1 mission rows, found $($missions.Count)."
}
$missionIds = @($missions | ForEach-Object { $_.Id })
Assert-UniqueStrings $missionIds "Configured mission IDs"

$catalogMissionIds = @([regex]::Matches($defaultCatalogText, 'NewMission\("(?<id>[a-z0-9_]+)"') | ForEach-Object { $_.Groups["id"].Value } | Sort-Object -Unique)
Assert-EqualSet $missionIds $catalogMissionIds "Configured/runtime mission IDs"

foreach ($overrideProperty in $parity.missionOverrides.PSObject.Properties) {
	if ($overrideProperty.Name -cnotin $missionIds) {
		throw "Mission override references unknown mission ID: $($overrideProperty.Name)"
	}
	$override = $overrideProperty.Value
	if ([string] $override.disposition -cnotin $allowedDispositions) {
		throw "Mission override $($overrideProperty.Name) uses unknown disposition '$($override.disposition)'."
	}
	Require-Text $override.gap "mission override $($overrideProperty.Name) gap" | Out-Null
}

$sourceCommandIds = @()
$sourceCommandIds += [regex]::Matches($commandUIText, 'commandId\s*(?:==|!=)\s*"(?<id>[a-z0-9_]+)"') | ForEach-Object { $_.Groups["id"].Value }
$sourceCommandIds += [regex]::Matches($commandUIText, 'return\s+"(?<id>(?:mission_(?:asset|captive|vehicle)_[a-z0-9_]+|undercover_(?:request|clear)))"') | ForEach-Object { $_.Groups["id"].Value }
$sourceCommandIds = @($sourceCommandIds | Sort-Object -Unique)
$manifestCommandIds = @($parity.commandActionIds | ForEach-Object { [string] $_ } | Sort-Object -Unique)
Assert-UniqueStrings @($parity.commandActionIds | ForEach-Object { [string] $_ }) "Command action manifest"
Assert-EqualSet $manifestCommandIds $sourceCommandIds "Command action manifest"

$resolvedActionRows = @{}
foreach ($commandId in $manifestCommandIds) {
	$rule = Resolve-ActionRule $parity $commandId
	if ($null -eq $rule) {
		throw "Command action $commandId has no parity mapping rule."
	}
	if (-not $rowById.ContainsKey([string] $rule.rowId)) {
		throw "Command action $commandId maps to unknown parity row '$($rule.rowId)'."
	}
	$ruleDisposition = Get-ObjectPropertyValue $rule "disposition"
	if ($null -ne $ruleDisposition -and [string] $ruleDisposition -cnotin $allowedDispositions) {
		throw "Command action $commandId uses unknown disposition override '$ruleDisposition'."
	}
	$resolvedActionRows[$commandId] = $rule
}

$componentText = ""
foreach ($componentFile in Get-ChildItem -LiteralPath (Join-Path $root "scripts/Game/HST/Components") -Filter "*.c" -File) {
	$componentText += "`n" + (Get-Content -Raw -LiteralPath $componentFile.FullName)
}
$sourceContextActionClasses = @([regex]::Matches(
	$componentText,
	'class\s+(?<class>HST_[A-Za-z0-9_]+Action)\s*:\s*HST_(?:MissionUserActionBase|PetrosUserActionBase|ContextualUserActionBase)') |
	ForEach-Object { $_.Groups["class"].Value } |
	Sort-Object -Unique)
$manifestContextActionClasses = @($parity.contextualActionClasses | ForEach-Object { [string] $_.class } | Sort-Object -Unique)
Assert-UniqueStrings @($parity.contextualActionClasses | ForEach-Object { [string] $_.class }) "Contextual action class manifest"
Assert-EqualSet $manifestContextActionClasses $sourceContextActionClasses "Contextual action class manifest"
foreach ($contextAction in $parity.contextualActionClasses) {
	if (-not $rowById.ContainsKey([string] $contextAction.rowId)) {
		throw "Contextual action $($contextAction.class) maps to unknown parity row '$($contextAction.rowId)'."
	}
}

$mdTick = [char] 96
$currentEvidencePrefix = "Active"
$currentAttachmentLabel = "active replacement"
if ($runtimeUseDisposition -ceq "rejected-after-runtime") {
	$currentEvidencePrefix = "Retained rejected"
	$currentAttachmentLabel = "retained rejected candidate"
}
elseif ($runtimeUseDisposition -ceq "supersede-before-runtime") {
	$currentEvidencePrefix = "Retained superseded"
	$currentAttachmentLabel = "retained superseded candidate"
}
$statusBuilder = New-Object System.Text.StringBuilder
Add-Line $statusBuilder "# Partisan Current Status"
Add-Line $statusBuilder
Add-Line $statusBuilder '> Generated from `docs/data/release_status.json` and `docs/data/antistasi_ce311_parity.json`. Do not hand-edit this file; run `tools/update-release-docs.ps1`.'
Add-Line $statusBuilder
Add-Line $statusBuilder "## Release decision"
Add-Line $statusBuilder
Add-Line $statusBuilder "**$releaseDecision - $($status.releaseStage).** No release-candidate package is certified."
Add-Line $statusBuilder
if ($releaseCandidateBuilt) {
	Add-Line $statusBuilder "The retained candidate identity below binds its exact source HEAD, manifest, canonical four-file package index, addon identity, and validation tools. The generator verifies that the candidate source is between the audited gameplay revision and the live checkout HEAD."
}
else {
	Add-Line $statusBuilder "The audited gameplay revision is fixed below. A tracked Markdown file cannot embed the hash of the commit that contains itself; the generator verifies that the audited revision is an ancestor of the checkout and prints the live checkout HEAD when it runs. Gate 1 evidence must record the exact post-checkout Git SHA and package hash together."
}
Add-Line $statusBuilder
Add-Line $statusBuilder "## Identity"
Add-Line $statusBuilder
Add-Line $statusBuilder "| Field | Current value |"
Add-Line $statusBuilder "| --- | --- |"
Add-Line $statusBuilder "| Status data as of | $mdTick$($status.statusAsOfUtc)$mdTick |"
Add-Line $statusBuilder "| Audited gameplay Git HEAD | $mdTick$auditedRevision$mdTick |"
Add-Line $statusBuilder "| Embedded implementation identity | $mdTick$sourceBuildSha$mdTick |"
Add-Line $statusBuilder "| Embedded build UTC / label | $mdTick$sourceBuildUtc$mdTick / $mdTick$sourceBuildLabel$mdTick |"
Add-Line $statusBuilder "| Campaign / runtime-settings schema | $mdTick$sourceCampaignSchema$mdTick / $mdTick$sourceSettingsSchema$mdTick |"
Add-Line $statusBuilder "| Workbench CRC | $mdTick$($status.evidence.workbench.crc)$mdTick |"
if ($releaseCandidateBuilt) {
	Add-Line $statusBuilder "| Release candidate / source HEAD | $mdTick$(Escape-MarkdownCell $candidateId)$mdTick / $mdTick$candidateSourceHead$mdTick |"
	Add-Line $statusBuilder "| Runtime use disposition | $mdTick$runtimeUseDisposition$mdTick |"
	Add-Line $statusBuilder "| Candidate manifest | $mdTick$(Escape-MarkdownCell $candidateManifestPath)$mdTick |"
	Add-Line $statusBuilder "| Manifest / ready-seal SHA-256 | $mdTick$candidateManifestSha$mdTick / $mdTick$candidateReadySha$mdTick |"
	Add-Line $statusBuilder "| Aggregate package SHA-256 | $mdTick$packageSha$mdTick ($packageHashAlgorithm over the canonical four-file package index) |"
	Add-Line $statusBuilder "| Addon GUID / revision / version | $mdTick$addonGuid$mdTick / $mdTick$(Escape-MarkdownCell $addonRevision)$mdTick / $mdTick$(Escape-MarkdownCell $packageVersion)$mdTick |"
	Add-Line $statusBuilder "| Workbench/tool identity | version $mdTick$(Escape-MarkdownCell $workbenchVersion)$mdTick / SHA-256 $mdTick$workbenchSha$mdTick / validation CRC $mdTick$workbenchCrc$mdTick |"
	Add-Line $statusBuilder "| Server / client versions | $mdTick$(Escape-MarkdownCell $serverVersion)$mdTick / $mdTick$(Escape-MarkdownCell $clientVersion)$mdTick |"
}
else {
	Add-Line $statusBuilder "| Release package SHA-256 | not built |"
	Add-Line $statusBuilder "| Server / client versions | not recorded |"
}
Add-Line $statusBuilder
Add-Line $statusBuilder "## Proof ladder"
Add-Line $statusBuilder
Add-Line $statusBuilder 'A pass never inherits upward. `partial` means some scoped evidence exists but the rung is not passed for the enabled campaign.'
Add-Line $statusBuilder
Add-Line $statusBuilder "| Rung | Status | Honest scope |"
Add-Line $statusBuilder "| --- | --- | --- |"
foreach ($rung in $status.proofRungs) {
	Add-Line $statusBuilder "| $(Escape-MarkdownCell $rung.label) | $mdTick$(Escape-MarkdownCell $rung.status)$mdTick | $(Escape-MarkdownCell $rung.summary) |"
}
Add-Line $statusBuilder
Add-Line $statusBuilder "## Retained evidence"
Add-Line $statusBuilder
Add-Line $statusBuilder "- Foundation: **$($status.evidence.foundation.status)** at $($status.evidence.foundation.referenceCount) references for $mdTick$($status.evidence.foundation.sourceSha)$mdTick."
Add-Line $statusBuilder "- Workbench: **$($status.evidence.workbench.status)** at $($status.evidence.workbench.fileCount) files / $($status.evidence.workbench.classCount) classes / CRC $mdTick$($status.evidence.workbench.crc)$mdTick for $mdTick$($status.evidence.workbench.sourceSha)$mdTick."
if ($null -eq $activePackagedFocused) {
	Add-Line $statusBuilder "- $currentEvidencePrefix packaged focused autotests: **not run** for replacement candidate $mdTick$candidateId${mdTick}; no prior-package result transfers to this package."
}
else {
	Add-Line $statusBuilder "- $currentEvidencePrefix packaged focused autotests: **$($activePackagedFocused.passedCases)/$($activePackagedFocused.caseCount)** cases and JUnit **$($activePackagedFocused.junitTests)/$($activePackagedFocused.junitFailures)/$($activePackagedFocused.junitErrors)/$($activePackagedFocused.junitSkipped)** tests/failures/errors/skips against exact candidate $mdTick$candidateId${mdTick}. Hard diagnostics are explicitly not free: $($activePackagedFocused.hardDiagnosticCount) total = $($activePackagedFocused.approvedStockFilterDiagnosticCount) approved stock + $($activePackagedFocused.approvedIntentionalFaultDiagnosticCount) approved intentional + $($activePackagedFocused.unapprovedHardDiagnosticCount) unapproved, with $($activePackagedFocused.envelopeFileCount) envelope files rehashed and zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $activePackagedFocusedSummaryPath)$mdTick / SHA-256 $mdTick$activePackagedFocusedSummarySha$mdTick; clean harness $mdTick$activePackagedFocusedHarnessHead${mdTick}. This exact-package deterministic-service result is non-certifying and does not color the native-engine-world rung."
}
if ($null -eq $activeCorrectedCanary -and $null -eq $activeFullCampaignDebug) {
	if ($null -eq $activePackagedFocused) {
		Add-Line $statusBuilder "- Active Campaign Debug: **not run** for replacement candidate $mdTick$candidateId${mdTick}; the corrected canary follows only after the packaged focused set is accepted."
	}
	else {
		Add-Line $statusBuilder "- Active Campaign Debug: **not run** for replacement candidate $mdTick$candidateId${mdTick}; the packaged focused set is accepted, so the corrected canary is the next gate."
	}
}
elseif ($null -ne $activeCorrectedCanary) {
	if ($activeCorrectedCanaryStatus -ceq "passed-noncertifying") {
		Add-Line $statusBuilder "- $currentEvidencePrefix corrected force-authority canary: **accepted, passed-noncertifying** on exact candidate $mdTick$candidateId${mdTick}. The focused proof passed $($activeCorrectedCanary.focusedAssertionsPassed)/$($activeCorrectedCanary.focusedAssertionCount) assertions and $($activeCorrectedCanary.certificationProven)/$($activeCorrectedCanary.certificationRequired) counted conditions; the 33-check classifier accepted $($activeCorrectedCanary.hardDiagnosticCount) hard diagnostics = $($activeCorrectedCanary.approvedStockDiagnosticCount) approved stock + $($activeCorrectedCanary.approvedIntentionalDiagnosticCount) approved intentional + $($activeCorrectedCanary.unapprovedHardDiagnosticCount) unapproved. All $($activeCorrectedCanary.envelopeFileCount) envelope files were rehashed with an exact $($activeCorrectedCanary.stateDiffRows)-row zero-delta state diff, final orphan cleanup, and zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $activeCorrectedCanarySummaryPath)$mdTick / SHA-256 $mdTick$activeCorrectedCanarySummarySha$mdTick; clean harness $mdTick$activeCorrectedCanaryHarnessHead${mdTick}. This scoped canary does not certify Full Campaign Debug."
	}
	else {
		Add-Line $statusBuilder "- $currentEvidencePrefix corrected force-authority canary: **rejected, focused proof failed** on exact candidate $mdTick$candidateId${mdTick}. The focused proof passed $($activeCorrectedCanary.focusedAssertionsPassed)/$($activeCorrectedCanary.focusedAssertionCount) assertions and $($activeCorrectedCanary.certificationProven)/$($activeCorrectedCanary.certificationRequired) counted conditions, with $($activeCorrectedCanary.fail) failed case and $($activeCorrectedCanary.certificationRequired - $activeCorrectedCanary.certificationProven) failed counted conditions. The 33-check classifier remained valid at $($activeCorrectedCanary.hardDiagnosticCount) approved and $($activeCorrectedCanary.unapprovedHardDiagnosticCount) unapproved hard diagnostics. All $($activeCorrectedCanary.envelopeFileCount) envelope files rehashed with an exact $($activeCorrectedCanary.stateDiffRows)-row zero-delta state diff, final orphan cleanup, and zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $activeCorrectedCanarySummaryPath)$mdTick / SHA-256 $mdTick$activeCorrectedCanarySummarySha$mdTick; clean harness $mdTick$activeCorrectedCanaryHarnessHead${mdTick}. Full Campaign Debug is stopped for this package; the proof-fixture correction requires a new immutable candidate."
	}
}
if ($null -ne $activeFullCampaignDebug) {
	if ($activeFullCampaignDebugValidation.AcceptedFull) {
		Add-Line $statusBuilder "- $currentEvidencePrefix Full Campaign Debug: **accepted, full certification passed** on exact candidate $mdTick$candidateId${mdTick}. The wrapper capture completed with stable artifacts, $($activeFullCampaignDebug.envelopeFileCount) rehashed envelope files, zero cleanup/spill residue, and a successful runtime outcome. Certification closed at $($activeFullCampaignDebug.pass) PASS, $($activeFullCampaignDebug.warn) WARN, $($activeFullCampaignDebug.fail) FAIL, $($activeFullCampaignDebug.blocked) BLOCKED, and $($activeFullCampaignDebug.skipped) SKIPPED with $($activeFullCampaignDebug.provenAssertions)/$($activeFullCampaignDebug.requiredAssertions) required assertions proven. The classifier accepted $($activeFullCampaignDebug.hardDiagnosticCount) hard diagnostics = $($activeFullCampaignDebug.approvedStockDiagnosticCount) approved stock + $($activeFullCampaignDebug.approvedIntentionalDiagnosticCount) approved intentional + $($activeFullCampaignDebug.unapprovedHardDiagnosticCount) unapproved. Summary: $mdTick$(Escape-MarkdownCell $activeFullCampaignDebugSummaryPath)$mdTick / SHA-256 $mdTick$activeFullCampaignDebugSummarySha$mdTick; clean harness $mdTick$activeFullCampaignDebugHarnessHead${mdTick}. Full-profile acceptance advances the unchanged package to the external release gates; it does not certify dedicated-server, restart, JIP, soak, canary, or stable rungs."
	}
	elseif ($activeFullCampaignDebugValidation.AcceptedInternal) {
		$externalAdvisoryText = @(
			$activeFullCampaignDebugValidation.ExternalRequiredAdvisoryIds) -join ", "
		Add-Line $statusBuilder "- $currentEvidencePrefix Full Campaign Debug: **accepted internally, external proof required** on exact candidate $mdTick$candidateId${mdTick}. All internal certification failures, blockers, and diagnostic defects are closed; the only retained warnings are the sealed non-certifying external-required advisories: $mdTick$(Escape-MarkdownCell $externalAdvisoryText)${mdTick}. The wrapper retained $($activeFullCampaignDebug.envelopeFileCount) rehashed envelope files with zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $activeFullCampaignDebugSummaryPath)$mdTick / SHA-256 $mdTick$activeFullCampaignDebugSummarySha$mdTick; clean harness $mdTick$activeFullCampaignDebugHarnessHead${mdTick}. This is passed-noncertifying until the unchanged package closes those external rungs."
	}
	else {
		$redAcceptanceText = if ($activeFullCampaignDebugValidation.LegacyHistoricalFixture) {
			"runtime acceptance remained false"
		}
		else {
			"release acceptance remained red"
		}
		Add-Line $statusBuilder "- $currentEvidencePrefix Full Campaign Debug: **rejected, red full profile** on exact candidate $mdTick$candidateId${mdTick}. The wrapper capture completed mechanically with stable artifacts, $($activeFullCampaignDebug.envelopeFileCount) rehashed envelope files, and zero cleanup/spill residue, while $redAcceptanceText. Certification stayed red at $($activeFullCampaignDebug.pass) PASS, $($activeFullCampaignDebug.warn) WARN, $($activeFullCampaignDebug.fail) FAIL, $($activeFullCampaignDebug.blocked) BLOCKED, and $($activeFullCampaignDebug.skipped) SKIPPED with $($activeFullCampaignDebug.provenAssertions)/$($activeFullCampaignDebug.requiredAssertions) required assertions proven, $($activeFullCampaignDebug.failedAssertions) failed, and $($activeFullCampaignDebug.blockedAssertions) blocked. The fail-closed classifier found $($activeFullCampaignDebug.hardDiagnosticCount) hard diagnostics = $($activeFullCampaignDebug.approvedStockDiagnosticCount) approved stock + $($activeFullCampaignDebug.approvedIntentionalDiagnosticCount) approved intentional + $($activeFullCampaignDebug.unapprovedHardDiagnosticCount) unapproved. Summary: $mdTick$(Escape-MarkdownCell $activeFullCampaignDebugSummaryPath)$mdTick / SHA-256 $mdTick$activeFullCampaignDebugSummarySha$mdTick; clean harness $mdTick$activeFullCampaignDebugHarnessHead${mdTick}. Mechanical capture success is not certification or diagnostic acceptance."
	}
}
Add-Line $statusBuilder "- Focused force-authority profile: **$($status.evidence.focusedForceAuthority.passedCases)/$($status.evidence.focusedForceAuthority.caseCount)** cases and **$($status.evidence.focusedForceAuthority.passedConditions)/$($status.evidence.focusedForceAuthority.countedConditions)** counted conditions for $mdTick$($status.evidence.focusedForceAuthority.sourceSha)$mdTick, with ${mdTick}CertificationPassed:$($status.evidence.focusedForceAuthority.certificationPassed.ToString().ToLowerInvariant())${mdTick}. This is historical state-only, non-package, non-certifying evidence."
foreach ($historicalCandidateResult in $historicalCandidateResults) {
	$historicalIdentity = $historicalCandidateResult.Identity
	$historicalCandidateId = $historicalIdentity.CandidateId
	$packagedFocused = $historicalCandidateResult.PackagedFocused
	$packagedFocusedValidation = $historicalCandidateResult.PackagedFocusedValidation
	$correctedCanary = $historicalCandidateResult.CorrectedCanary
	$correctedCanaryValidation = $historicalCandidateResult.CorrectedCanaryValidation
	$fullCampaignDebug = $historicalCandidateResult.FullCampaignDebug
	$fullCampaignDebugValidation = $historicalCandidateResult.FullCampaignDebugValidation
	$externalRuntime = $historicalCandidateResult.ExternalRuntime
	$externalRuntimeValidation = $historicalCandidateResult.ExternalRuntimeValidation
	Add-Line $statusBuilder "- Historical packaged focused autotests: **$($packagedFocused.passedCases)/$($packagedFocused.caseCount)** cases and JUnit **$($packagedFocused.junitTests)/$($packagedFocused.junitFailures)/$($packagedFocused.junitErrors)/$($packagedFocused.junitSkipped)** tests/failures/errors/skips against prior exact candidate $mdTick$historicalCandidateId$mdTick. Hard diagnostics are explicitly not free: $($packagedFocused.hardDiagnosticCount) total = $($packagedFocused.approvedStockFilterDiagnosticCount) approved stock + $($packagedFocused.approvedIntentionalFaultDiagnosticCount) approved intentional + $($packagedFocused.unapprovedHardDiagnosticCount) unapproved, with $($packagedFocused.envelopeFileCount) envelope files rehashed and zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $packagedFocusedValidation.SummaryPath)$mdTick / SHA-256 $mdTick$($packagedFocusedValidation.SummarySha256)$mdTick; harness $mdTick$($packagedFocusedValidation.HarnessGitHead)$mdTick. This immutable non-certifying result does not attach to the $currentAttachmentLabel."
	if ($historicalCandidateResult.CorrectedCanaryOutcome -ceq "accepted") {
		Add-Line $statusBuilder "- Historical corrected force-authority canary: **accepted non-certifying** on prior exact candidate $mdTick$historicalCandidateId${mdTick}. The focused proof passed $($correctedCanary.focusedAssertionsPassed)/$($correctedCanary.focusedAssertionCount) assertions and $($correctedCanary.certificationProven)/$($correctedCanary.certificationRequired) counted certification conditions. The 33-check classifier found $($correctedCanary.hardDiagnosticCount) hard diagnostics = $($correctedCanary.approvedStockDiagnosticCount) approved stock + $($correctedCanary.approvedIntentionalDiagnosticCount) approved intentional + $($correctedCanary.unapprovedHardDiagnosticCount) unapproved. All $($correctedCanary.envelopeFileCount) envelope files rehashed with zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $correctedCanaryValidation.SummaryPath)$mdTick / SHA-256 $mdTick$($correctedCanaryValidation.SummarySha256)$mdTick; harness $mdTick$($correctedCanaryValidation.HarnessGitHead)$mdTick. This immutable scoped acceptance does not transfer to the $currentAttachmentLabel."
	}
	elseif ($historicalCandidateResult.CorrectedCanaryOutcome -ceq "rejected-proof") {
		Add-Line $statusBuilder "- Historical corrected force-authority canary: **rejected, focused proof failed** on prior exact candidate $mdTick$historicalCandidateId${mdTick}. The focused proof passed $($correctedCanary.focusedAssertionsPassed)/$($correctedCanary.focusedAssertionCount) assertions and $($correctedCanary.certificationProven)/$($correctedCanary.certificationRequired) counted conditions. The classifier remained valid at $($correctedCanary.hardDiagnosticCount) approved and $($correctedCanary.unapprovedHardDiagnosticCount) unapproved hard diagnostics. All $($correctedCanary.envelopeFileCount) envelope files rehashed with zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $correctedCanaryValidation.SummaryPath)$mdTick / SHA-256 $mdTick$($correctedCanaryValidation.SummarySha256)$mdTick; harness $mdTick$($correctedCanaryValidation.HarnessGitHead)$mdTick. This immutable rejection does not transfer to the $currentAttachmentLabel."
	}
	else {
		Add-Line $statusBuilder "- Historical corrected force-authority canary: **rejected fail-closed** on prior exact candidate $mdTick$historicalCandidateId${mdTick}. The focused proof remained $($correctedCanary.focusedAssertionsPassed)/$($correctedCanary.focusedAssertionCount) assertions and $($correctedCanary.certificationProven)/$($correctedCanary.certificationRequired) counted certification conditions, but the classifier found $($correctedCanary.unapprovedHardDiagnosticCount) unapproved $mdTick$($correctedCanary.unapprovedHardDiagnosticKind)${mdTick} diagnostic. All $($correctedCanary.envelopeFileCount) envelope files rehashed with zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $correctedCanaryValidation.SummaryPath)$mdTick / SHA-256 $mdTick$($correctedCanaryValidation.SummarySha256)$mdTick; harness $mdTick$($correctedCanaryValidation.HarnessGitHead)$mdTick. This immutable rejection does not transfer to the $currentAttachmentLabel."
	}
	if ($null -ne $fullCampaignDebug) {
		if ($historicalCandidateResult.RetirementDisposition -ceq
			"rejected-after-external-runtime" -and
			$fullCampaignDebugValidation.AcceptedFull) {
			Add-Line $statusBuilder "- Historical Full Campaign Debug: **accepted, full certification passed** on prior exact candidate $mdTick$historicalCandidateId${mdTick}. All $($fullCampaignDebug.envelopeFileCount) envelope files were rehashed with zero cleanup/spill residue and $($fullCampaignDebug.provenAssertions)/$($fullCampaignDebug.requiredAssertions) required assertions proven. Summary: $mdTick$(Escape-MarkdownCell $fullCampaignDebugValidation.SummaryPath)$mdTick / SHA-256 $mdTick$($fullCampaignDebugValidation.SummarySha256)$mdTick; clean harness $mdTick$($fullCampaignDebugValidation.HarnessGitHead)${mdTick}. This immutable full-profile acceptance advanced only that retired package to external gates and does not attach to the $currentAttachmentLabel."
		}
		elseif ($historicalCandidateResult.RetirementDisposition -ceq
			"rejected-after-external-runtime" -and
			$fullCampaignDebugValidation.AcceptedInternal) {
			Add-Line $statusBuilder "- Historical Full Campaign Debug: **accepted internally, external proof required** on prior exact candidate $mdTick$historicalCandidateId${mdTick}. Its internal certification failures and blockers were zero; only the sealed non-certifying external-required advisories remained. Summary: $mdTick$(Escape-MarkdownCell $fullCampaignDebugValidation.SummaryPath)$mdTick / SHA-256 $mdTick$($fullCampaignDebugValidation.SummarySha256)$mdTick; clean harness $mdTick$($fullCampaignDebugValidation.HarnessGitHead)${mdTick}. This internal acceptance advanced only that retired package to its exact external-required checks and does not attach to the $currentAttachmentLabel."
		}
		else {
			$historicalRedAcceptanceText = if ($fullCampaignDebugValidation.LegacyHistoricalFixture) {
				"runtime acceptance remained false"
			}
			else {
				"release acceptance remained red"
			}
			Add-Line $statusBuilder "- Historical Full Campaign Debug: **rejected, red full profile** on prior exact candidate $mdTick$historicalCandidateId${mdTick}. The wrapper capture completed mechanically with stable artifacts, $($fullCampaignDebug.envelopeFileCount) rehashed envelope files, and zero cleanup/spill residue, while $historicalRedAcceptanceText. Certification stayed red at $($fullCampaignDebug.pass) PASS, $($fullCampaignDebug.warn) WARN, $($fullCampaignDebug.fail) FAIL, $($fullCampaignDebug.blocked) BLOCKED, and $($fullCampaignDebug.skipped) SKIPPED with $($fullCampaignDebug.provenAssertions)/$($fullCampaignDebug.requiredAssertions) required assertions proven, $($fullCampaignDebug.failedAssertions) failed, and $($fullCampaignDebug.blockedAssertions) blocked. The fail-closed classifier found $($fullCampaignDebug.hardDiagnosticCount) hard diagnostics = $($fullCampaignDebug.approvedStockDiagnosticCount) approved stock + $($fullCampaignDebug.approvedIntentionalDiagnosticCount) approved intentional + $($fullCampaignDebug.unapprovedHardDiagnosticCount) unapproved. Summary: $mdTick$(Escape-MarkdownCell $fullCampaignDebugValidation.SummaryPath)$mdTick / SHA-256 $mdTick$($fullCampaignDebugValidation.SummarySha256)$mdTick; clean harness $mdTick$($fullCampaignDebugValidation.HarnessGitHead)${mdTick}. Mechanical capture success is not certification or diagnostic acceptance, and this immutable rejection does not attach to the $currentAttachmentLabel."
		}
	}
	else {
		Add-Line $statusBuilder "- Historical Full Campaign Debug: **stopped and not run** for prior exact candidate $mdTick$historicalCandidateId${mdTick}; its $mdTick$($historicalCandidateResult.RetirementDisposition)$mdTick contract ended the chain at the rejected corrected canary."
	}
	if ($null -ne $externalRuntime) {
		Add-Line $statusBuilder "- Historical external runtime: **rejected at $($externalRuntimeValidation.FailedRung)** on prior exact candidate $mdTick$historicalCandidateId${mdTick}. All $($externalRuntimeValidation.ArtifactCount) retained artifacts were rehashed as set SHA-256 $mdTick$($externalRuntimeValidation.ArtifactSetSha256)$mdTick with zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $externalRuntimeValidation.SummaryPath)$mdTick / SHA-256 $mdTick$($externalRuntimeValidation.SummarySha256)$mdTick; clean harness $mdTick$($externalRuntimeValidation.HarnessGitHead)${mdTick}. This sealed external rejection retired that immutable package and does not attach to the $currentAttachmentLabel."
	}
}
Add-Line $statusBuilder
Add-Line $statusBuilder "## Specification coverage"
Add-Line $statusBuilder
$dispositionGroups = @($parity.contractRows | Group-Object disposition | Sort-Object Name)
$dispositionSummary = @($dispositionGroups | ForEach-Object { "$($_.Name) $($_.Count)" }) -join "; "
Add-Line $statusBuilder "- CE 3.11.1 product contract rows: **$($parity.contractRows.Count)** ($dispositionSummary)."
Add-Line $statusBuilder "- Configured/runtime mission rows: **$($missions.Count)/39**, all mapped to a product contract row."
Add-Line $statusBuilder "- Routed command/action IDs: **$($manifestCommandIds.Count)**, exact source/manifest set match and all mapped."
Add-Line $statusBuilder "- Concrete contextual action classes: **$($manifestContextActionClasses.Count)**, exact source/manifest set match and all mapped."
Add-Line $statusBuilder '- Product specification: `docs/ANTISTASI_CE311_PARITY_MATRIX.md`.'
Add-Line $statusBuilder
Add-Line $statusBuilder "Coverage means the surface is named and classified. It does not mean the behavior is exact or certified."
Add-Line $statusBuilder
Add-Line $statusBuilder "## Active blockers"
Add-Line $statusBuilder
Add-Line $statusBuilder "| ID | Category | Blocker |"
Add-Line $statusBuilder "| --- | --- | --- |"
foreach ($blocker in $status.activeBlockers) {
	Add-Line $statusBuilder "| $mdTick$(Escape-MarkdownCell $blocker.id)$mdTick | $mdTick$(Escape-MarkdownCell $blocker.category)$mdTick | $(Escape-MarkdownCell $blocker.summary) |"
}
Add-Line $statusBuilder
Add-Line $statusBuilder "## Next release-closure step"
Add-Line $statusBuilder
if ($releaseCandidateBuilt) {
	if ($runtimeUseDisposition -ceq "supersede-before-runtime") {
		Add-Line $statusBuilder "Gate 1 retained candidate $mdTick$(Escape-MarkdownCell $candidateId)$mdTick remains sealed but is superseded before runtime use. Build exactly one replacement candidate for the focused-suite registration repair; retain both package identities, and do not combine evidence across their aggregate SHA-256 digests."
	}
	elseif ($runtimeUseDisposition -ceq "rejected-after-runtime") {
		if ($null -ne $activeFullCampaignDebug) {
			Add-Line $statusBuilder "The retained candidate's Full Campaign Debug result is rejected at $($activeFullCampaignDebug.fail) failed cases, $($activeFullCampaignDebug.blocked) blocked cases, $($activeFullCampaignDebug.failedAssertions) failed required assertions, $($activeFullCampaignDebug.blockedAssertions) blocked required assertions, and $($activeFullCampaignDebug.unapprovedHardDiagnosticCount) unapproved diagnostics, and its rejected-after-runtime disposition blocks further runtime consumption. Triage that exact-package result, seal source fixes in a new immutable candidate, and restart focused -> corrected canary -> full from the new package."
		}
		else {
			Add-Line $statusBuilder "The retained candidate's corrected canary is rejected at $($activeCorrectedCanary.focusedAssertionsPassed)/$($activeCorrectedCanary.focusedAssertionCount) focused assertions and $($activeCorrectedCanary.certificationProven)/$($activeCorrectedCanary.certificationRequired) counted conditions, and its rejected-after-runtime disposition blocks further runtime consumption. Keep its valid envelope immutable, seal the proof-fixture correction in a new candidate, and restart focused -> corrected canary -> full from that new package."
		}
	}
	elseif ($null -eq $activePackagedFocused) {
		Add-Line $statusBuilder "Run the individually named packaged focused service suites next against active replacement $mdTick$(Escape-MarkdownCell $candidateId)$mdTick, manifest $mdTick$(Escape-MarkdownCell $candidateManifestPath)$mdTick, and aggregate package SHA-256 $mdTick$packageSha${mdTick}. If that exact-package set is accepted, run the corrected force-authority canary next; do not transfer the historical candidate's pass or rejection into either gate."
	}
	elseif ($null -eq $activeCorrectedCanary) {
		Add-Line $statusBuilder "The active replacement's packaged focused set is retained. Run the corrected force-authority canary next against the unchanged package identified by manifest $mdTick$(Escape-MarkdownCell $candidateManifestPath)$mdTick and aggregate SHA-256 $mdTick$packageSha${mdTick}."
	}
	elseif ($null -eq $activeFullCampaignDebug) {
		Add-Line $statusBuilder "The active replacement's packaged focused set and corrected canary are retained. Run Full Campaign Debug next against the same immutable package identity."
	}
	elseif ($activeFullCampaignDebugValidation.AcceptedFull) {
		Add-Line $statusBuilder "The active replacement's full profile is accepted on the unchanged immutable package. Advance that exact package to packaged dedicated-server, restart, multiplayer/JIP, soak, and canary gates; do not infer any of those external rungs from Full Campaign Debug."
	}
	elseif ($activeFullCampaignDebugValidation.AcceptedInternal) {
		$externalAdvisoryText = @(
			$activeFullCampaignDebugValidation.ExternalRequiredAdvisoryIds) -join ", "
		Add-Line $statusBuilder "The active replacement's internal full profile is accepted with zero certification blockers and only the sealed non-certifying external-required advisory set ($externalAdvisoryText). Run those external scenarios against the unchanged immutable package and retain trusted portable receipts; internal acceptance does not close any external rung."
	}
	else {
		Add-Line $statusBuilder "The active full-profile checkpoint is retained and release-blocking. Triage its $($activeFullCampaignDebug.fail) failed cases, $($activeFullCampaignDebug.blocked) blocked cases, $($activeFullCampaignDebug.failedAssertions) failed required assertions, $($activeFullCampaignDebug.blockedAssertions) blocked required assertions, and $($activeFullCampaignDebug.unapprovedHardDiagnosticCount) unapproved diagnostics against exact package $mdTick$packageSha${mdTick}; rerun the full profile only after the current source fixes are sealed into a new immutable candidate. Do not attach post-capture source changes to this package's evidence chain."
	}
}
else {
	Add-Line $statusBuilder "Gate 0's generated truth surface is complete. Gate 1 is the current work boundary: commit the guarded build-once tooling, build one clean package, and record Git, Workbench, package, addon, server, and client identities in one retained evidence bundle before rerunning the current proof ladder."
}

$parityBuilder = New-Object System.Text.StringBuilder
Add-Line $parityBuilder "# Partisan - Antistasi CE 3.11.1 Parity Matrix"
Add-Line $parityBuilder
Add-Line $parityBuilder '> Generated from `docs/data/antistasi_ce311_parity.json` and current source inventories. Do not hand-edit this file; run `tools/update-release-docs.ps1`.'
Add-Line $parityBuilder
Add-Line $parityBuilder "## Pinned baseline"
Add-Line $parityBuilder
Add-Line $parityBuilder "- Version: **$($parity.baseline.version)**"
Add-Line $parityBuilder "- Tag commit: [$mdTick$baselineCommit$mdTick]($($parity.baseline.sourceUrl))"
Add-Line $parityBuilder "- [Official release]($($parity.baseline.releaseUrl))"
Add-Line $parityBuilder "- [Official detailed guide]($($parity.baseline.guideUrl))"
Add-Line $parityBuilder
Add-Line $parityBuilder "This matrix is a behavioral specification, not a claim that matching names or source paths are complete. Engine-driven differences must be explicit adaptations."
Add-Line $parityBuilder
Add-Line $parityBuilder "## Proof vocabulary"
Add-Line $parityBuilder
Add-Line $parityBuilder "| Term | Meaning |"
Add-Line $parityBuilder "| --- | --- |"
foreach ($term in $parity.proofVocabulary) {
	Add-Line $parityBuilder "| $mdTick$(Escape-MarkdownCell $term.id)$mdTick | $(Escape-MarkdownCell $term.meaning) |"
}
Add-Line $parityBuilder
Add-Line $parityBuilder "## Contract summary"
Add-Line $parityBuilder
Add-Line $parityBuilder "| ID | Domain | Priority | Disposition | Blocking milestone |"
Add-Line $parityBuilder "| --- | --- | --- | --- | --- |"
foreach ($row in ($parity.contractRows | Sort-Object id)) {
	Add-Line $parityBuilder "| $mdTick$(Escape-MarkdownCell $row.id)$mdTick | $(Escape-MarkdownCell $row.domain) | $mdTick$(Escape-MarkdownCell $row.priority)$mdTick | $mdTick$(Escape-MarkdownCell $row.disposition)$mdTick | $(Escape-MarkdownCell $row.blockingMilestone) |"
}
Add-Line $parityBuilder
Add-Line $parityBuilder "## Detailed behavioral contracts"
foreach ($row in ($parity.contractRows | Sort-Object id)) {
	Add-Line $parityBuilder
	Add-Line $parityBuilder "### $($row.id) - $($row.domain)"
	Add-Line $parityBuilder
	Add-Line $parityBuilder "- **Priority / disposition:** $mdTick$($row.priority)$mdTick / $mdTick$($row.disposition)$mdTick"
	Add-Line $parityBuilder "- **Upstream change history:** $($row.upstreamChangeHistory)"
	$evidenceLinks = @()
	$evidenceIndex = 1
	foreach ($url in $row.upstreamEvidence) {
		$evidenceLinks += "[evidence $evidenceIndex]($url)"
		$evidenceIndex++
	}
	Add-Line $parityBuilder "- **Upstream evidence:** $($evidenceLinks -join ', ')"
	Add-Line $parityBuilder "- **Observable contract:** $($row.observableContract)"
	Add-Line $parityBuilder "- **Formula/pacing contract:** $($row.formulaContract)"
	Add-Line $parityBuilder "- **Reforger adaptation:** $($row.reforgerAdaptation)"
	Add-Line $parityBuilder "- **Partisan state owner:** $($row.partisanStateOwner)"
	Add-Line $parityBuilder "- **Partisan service owner:** $($row.partisanServiceOwner)"
	Add-Line $parityBuilder "- **Physical behavior:** $($row.physicalBehavior)"
	Add-Line $parityBuilder "- **Virtual behavior:** $($row.virtualBehavior)"
	Add-Line $parityBuilder "- **Transaction policy:** $($row.transactionPolicy)"
	Add-Line $parityBuilder "- **Persistence/migration:** $($row.persistenceMigration)"
	Add-Line $parityBuilder "- **Client projection:** $($row.clientProjection)"
	$proofIdText = @($row.proofIds) -join "$mdTick, $mdTick"
	Add-Line $parityBuilder "- **Proof IDs:** $mdTick$proofIdText$mdTick"
	Add-Line $parityBuilder "- **Blocking milestone:** $($row.blockingMilestone)"
}

Add-Line $parityBuilder
Add-Line $parityBuilder "## Mission inventory"
Add-Line $parityBuilder
Add-Line $parityBuilder "All 39 configured mission IDs are mapped below. A row means the mission is inventoried; its disposition and gap determine whether it is release-ready."
Add-Line $parityBuilder
Add-Line $parityBuilder "| Mission ID | Display name | Category / runtime | Contract | Configured reward | Disposition | Remaining gap |"
Add-Line $parityBuilder "| --- | --- | --- | --- | --- | --- | --- |"
foreach ($mission in ($missions | Sort-Object Id)) {
	$row = $rowById[$mission.RowId]
	$disposition = [string] $row.disposition
	$gap = "Mission-specific CE 3.11.1 semantics, formulas, native behavior, restart, MP/JIP, and soak proof remain open."
	$override = Get-ObjectPropertyValue $parity.missionOverrides $mission.Id
	if ($null -ne $override) {
		$disposition = [string] $override.disposition
		$gap = [string] $override.gap
	}
	$categoryRuntime = $mission.Category.Replace("HST_MISSION_", "") + " / " + $mission.Runtime
	Add-Line $parityBuilder "| $mdTick$(Escape-MarkdownCell $mission.Id)$mdTick | $(Escape-MarkdownCell $mission.Name) | $mdTick$(Escape-MarkdownCell $categoryRuntime)$mdTick | $mdTick$(Escape-MarkdownCell $mission.RowId)$mdTick | $(Escape-MarkdownCell (Build-ConfiguredReward $mission)) | $mdTick$(Escape-MarkdownCell $disposition)$mdTick | $(Escape-MarkdownCell $gap) |"
}

Add-Line $parityBuilder
Add-Line $parityBuilder "## Routed command/action inventory"
Add-Line $parityBuilder
Add-Line $parityBuilder "This is the exact checked source/manifest set of command IDs handled by the command UI service. Development/admin and removal candidates are retained here so they cannot disappear from release review."
Add-Line $parityBuilder
Add-Line $parityBuilder "| Action ID | Surface | Contract | Disposition |"
Add-Line $parityBuilder "| --- | --- | --- | --- |"
foreach ($commandId in $manifestCommandIds) {
	$rule = $resolvedActionRows[$commandId]
	$row = $rowById[[string] $rule.rowId]
	$disposition = [string] $row.disposition
	$ruleDisposition = Get-ObjectPropertyValue $rule "disposition"
	if ($null -ne $ruleDisposition) {
		$disposition = [string] $ruleDisposition
	}
	Add-Line $parityBuilder "| $mdTick$(Escape-MarkdownCell $commandId)$mdTick | $mdTick$(Escape-MarkdownCell $rule.surface)$mdTick | $mdTick$(Escape-MarkdownCell $rule.rowId)$mdTick | $mdTick$(Escape-MarkdownCell $disposition)$mdTick |"
}

Add-Line $parityBuilder
Add-Line $parityBuilder "## Contextual world-action inventory"
Add-Line $parityBuilder
Add-Line $parityBuilder "| Action class | Contract | Disposition |"
Add-Line $parityBuilder "| --- | --- | --- |"
foreach ($contextAction in ($parity.contextualActionClasses | Sort-Object class)) {
	$row = $rowById[[string] $contextAction.rowId]
	Add-Line $parityBuilder "| $mdTick$(Escape-MarkdownCell $contextAction.class)$mdTick | $mdTick$(Escape-MarkdownCell $contextAction.rowId)$mdTick | $mdTick$(Escape-MarkdownCell $row.disposition)$mdTick |"
}

Add-Line $parityBuilder
Add-Line $parityBuilder "## Coverage rule"
Add-Line $parityBuilder
Add-Line $parityBuilder "Generation fails if the runtime/config mission sets differ, a mission category lacks a contract, the command-ID source set differs from the explicit manifest, a concrete contextual action class differs from its manifest, an action lacks a mapping, or a row uses unknown vocabulary. Coverage is therefore reproducible, while certification remains package- and evidence-dependent."

Publish-GeneratedFile $currentStatusPath $statusBuilder.ToString() $Check.IsPresent
Publish-GeneratedFile $parityMatrixPath $parityBuilder.ToString() $Check.IsPresent

$worktreeLabel = "clean"
if ($worktreeRows.Count -gt 0) {
	$worktreeLabel = "dirty ($($worktreeRows.Count) path(s))"
}

if ($Check) {
	Write-Host "Release documentation is current: $($parity.contractRows.Count) contracts, $($missions.Count) missions, $($manifestCommandIds.Count) command actions, $($manifestContextActionClasses.Count) contextual actions."
}
else {
	Write-Host "Release documentation updated: $($parity.contractRows.Count) contracts, $($missions.Count) missions, $($manifestCommandIds.Count) command actions, $($manifestContextActionClasses.Count) contextual actions."
}
Write-Host "Checkout identity: HEAD $checkoutHead | worktree $worktreeLabel | audited gameplay revision $auditedRevision"
