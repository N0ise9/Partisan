[CmdletBinding()]
param(
	[switch] $Check
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

	$diff = @(Compare-Object @($Expected | Sort-Object) @($Actual | Sort-Object))
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
	}
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

$status = Read-JsonFile $statusDataPath
$parity = Read-JsonFile $parityDataPath

if ([int] (Get-ObjectPropertyValue $status "schemaVersion") -ne 2) {
	throw "release_status.json must use schemaVersion 2 with separate active and historical candidate evidence."
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

$releaseCandidateBuilt = [bool] $status.artifact.releaseCandidateBuilt
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
$historicalCandidateEvidence = Get-ObjectPropertyValue $status "historicalCandidateEvidence"
$historicalCandidate = $null
$historicalEvidence = $null
$historicalCandidateId = ""
$historicalCandidateSourceHead = ""
$historicalCandidateManifestPath = ""
$historicalCandidateManifestSha = ""
$historicalCandidateReadySha = ""
$historicalPackageSha = ""
$historicalPackageVersion = ""
$historicalWorkbenchCrc = ""
$packagedFocused = $null
$packagedFocusedSummaryPath = ""
$packagedFocusedSummarySha = ""
$packagedFocusedHarnessHead = ""
$correctedCanary = $null
$correctedCanaryStatus = ""
$correctedCanarySummaryPath = ""
$correctedCanarySummarySha = ""
$correctedCanaryHarnessHead = ""
$fullCampaignDebug = $null
$fullCampaignDebugStatus = ""
$fullCampaignSummaryPath = ""
$fullCampaignSummarySha = ""
$fullCampaignHarnessHead = ""
if ($releaseCandidateBuilt) {
	$runtimeUseDisposition = Require-Text $status.artifact.runtimeUseDisposition "release_status.artifact.runtimeUseDisposition"
	if ($runtimeUseDisposition -cnotin @("active-runtime-candidate", "supersede-before-runtime")) {
		throw "release_status.artifact.runtimeUseDisposition must be active-runtime-candidate or supersede-before-runtime."
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
	$historicalCandidate = Get-ObjectPropertyValue $historicalCandidateEvidence "candidate"
	$historicalEvidence = Get-ObjectPropertyValue $historicalCandidateEvidence "evidence"
	if ($null -eq $historicalCandidate -or $null -eq $historicalEvidence) {
		throw "release_status.historicalCandidateEvidence must contain candidate and evidence objects."
	}
	$historicalIdentity = Get-RetainedCandidateIdentity `
		$historicalCandidate `
		"release_status.historicalCandidateEvidence.candidate"
	$historicalCandidateId = $historicalIdentity.CandidateId
	$historicalCandidateSourceHead = $historicalIdentity.CandidateSourceHead
	$historicalCandidateManifestPath = $historicalIdentity.ManifestPath
	$historicalCandidateManifestSha = $historicalIdentity.ManifestSha256
	$historicalCandidateReadySha = $historicalIdentity.ReadySha256
	$historicalPackageSha = $historicalIdentity.PackageSha256
	$historicalPackageVersion = $historicalIdentity.PackageVersion
	$historicalWorkbenchCrc = $historicalIdentity.WorkbenchCrc
	if ($historicalCandidateId -ceq $activeCandidateIdentity.CandidateId -or
		$historicalCandidateManifestPath -ceq $activeCandidateIdentity.ManifestPath -or
		$historicalPackageSha -ceq $activeCandidateIdentity.PackageSha256) {
		throw "Historical candidate evidence must bind a distinct retained package identity."
	}

	$packagedFocused = Get-ObjectPropertyValue $historicalEvidence "packagedFocusedAutotests"
	$correctedCanary = Get-ObjectPropertyValue $historicalEvidence "correctedForceAuthorityCanary"
	$fullCampaignDebug = Get-ObjectPropertyValue $historicalEvidence "fullCampaignDebug"
	if ($null -eq $packagedFocused -or $null -eq $correctedCanary -or
		$null -eq $fullCampaignDebug) {
		throw "Historical candidate evidence must retain focused, corrected-canary, and full-profile results."
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
	if ($null -ne $activePackagedFocused) {
		$activePackagedFocusedValidation = Assert-PackagedFocusedEvidence `
			$activePackagedFocused `
			$activeCandidateIdentity `
			"passed-noncertifying" `
			"release_status.evidence.packagedFocusedAutotests"
		$activePackagedFocusedSummaryPath = $activePackagedFocusedValidation.SummaryPath
		$activePackagedFocusedSummarySha = $activePackagedFocusedValidation.SummarySha256
		$activePackagedFocusedHarnessHead = $activePackagedFocusedValidation.HarnessGitHead
		$focusedStatusAsOfUtc = Require-UtcTimestamp `
			$status.statusAsOfUtc `
			"release_status.statusAsOfUtc"
		if ($focusedStatusAsOfUtc -lt $activePackagedFocusedValidation.AcceptedCompletedUtc) {
			throw "Release status cannot predate the active packaged focused evidence window."
		}
	}
	$historicalPackagedFocusedValidation = Assert-PackagedFocusedEvidence `
		$packagedFocused `
		$historicalIdentity `
		"historical-passed-noncertifying" `
		"release_status.historicalCandidateEvidence.evidence.packagedFocusedAutotests"
	$packagedFocusedSummaryPath = $historicalPackagedFocusedValidation.SummaryPath
	$packagedFocusedSummarySha = $historicalPackagedFocusedValidation.SummarySha256
	$packagedFocusedHarnessHead = $historicalPackagedFocusedValidation.HarnessGitHead
	if ($historicalPackagedFocusedValidation.PreliminaryStatus -cne
			"superseded-for-acceptance" -or
		$historicalPackagedFocusedValidation.PreliminaryCaseCount -ne 5) {
		throw "Historical packaged focused evidence must retain its superseded preliminary-run disposition."
	}

	if ($null -eq $correctedCanary) {
		throw "Release status must contain the corrected force-authority canary evidence."
	}
	$correctedCanaryStatus = Require-Text `
		(Get-ObjectPropertyValue $correctedCanary "status") `
		"release_status.historicalCandidateEvidence.evidence.correctedForceAuthorityCanary.status"
	if ($correctedCanaryStatus -cne "historical-failed-unapproved-hard-diagnostic") {
		throw "The historical corrected canary must retain its fail-closed diagnostic result."
	}
	$correctedCanarySummaryPath = Require-RepoRelativePath `
		(Get-ObjectPropertyValue $correctedCanary "summaryPath") `
		"release_status.historicalCandidateEvidence.evidence.correctedForceAuthorityCanary.summaryPath"
	$correctedCanarySummarySha = Require-Sha256 `
		(Get-ObjectPropertyValue $correctedCanary "summarySha256") `
		"release_status.historicalCandidateEvidence.evidence.correctedForceAuthorityCanary.summarySha256"
	$correctedCanaryHarnessHead = Require-Text `
		(Get-ObjectPropertyValue $correctedCanary "harnessGitHead") `
		"release_status.historicalCandidateEvidence.evidence.correctedForceAuthorityCanary.harnessGitHead"
	if ($correctedCanaryHarnessHead -cnotmatch '^[0-9a-f]{40}$') {
		throw "The corrected canary harness HEAD must be a lowercase full Git SHA."
	}

	$correctedCanarySummaryFullPath = [IO.Path]::GetFullPath(
		(Join-Path $root $correctedCanarySummaryPath))
	if (-not $correctedCanarySummaryFullPath.StartsWith(
		$repositoryPrefix,
		[StringComparison]::OrdinalIgnoreCase) -or
		-not (Test-Path -LiteralPath $correctedCanarySummaryFullPath -PathType Leaf)) {
		throw "The tracked corrected canary summary is missing or outside the repository."
	}
	$correctedCanarySummaryItem = Get-Item -LiteralPath $correctedCanarySummaryFullPath -Force
	if (($correctedCanarySummaryItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
		throw "The tracked corrected canary summary must not be a reparse point."
	}
	$actualCorrectedCanarySummarySha = (Get-FileHash `
		-LiteralPath $correctedCanarySummaryFullPath `
		-Algorithm SHA256).Hash.ToLowerInvariant()
	if ($actualCorrectedCanarySummarySha -cne $correctedCanarySummarySha.ToLowerInvariant()) {
		throw "The corrected canary summary SHA-256 does not match release status."
	}

	$correctedCanarySummaryText = Get-Content -Raw -LiteralPath $correctedCanarySummaryFullPath
	if ($correctedCanarySummaryText -match '(?i)[A-Z]:[\\/]') {
		throw "The tracked corrected canary summary contains a local absolute path."
	}
	$correctedCanarySummary = $correctedCanarySummaryText | ConvertFrom-Json
	$correctedSummaryCandidate = Get-ObjectPropertyValue $correctedCanarySummary "candidate"
	$correctedSummaryHarness = Get-ObjectPropertyValue $correctedCanarySummary "harness"
	$correctedSummarySettings = Get-ObjectPropertyValue $correctedCanarySummary "settings"
	$correctedSummaryCapture = Get-ObjectPropertyValue $correctedCanarySummary "capture"
	$correctedSummaryResult = Get-ObjectPropertyValue $correctedCanarySummary "result"
	$correctedSummaryProof = Get-ObjectPropertyValue $correctedCanarySummary "proof"
	$correctedSummaryDiagnostics = Get-ObjectPropertyValue $correctedCanarySummary "diagnostics"
	$correctedSummaryCleanup = Get-ObjectPropertyValue $correctedCanarySummary "cleanup"
	$correctedSummaryIntegrity = Get-ObjectPropertyValue $correctedCanarySummary "integrity"
	$correctedSummaryFinding = Get-ObjectPropertyValue $correctedCanarySummary "finding"
	$correctedSummarySchemaVersion = Require-IntegerProperty `
		$correctedCanarySummary "schemaVersion" "corrected canary summary.schemaVersion"
	if ($correctedSummarySchemaVersion -ne 1 -or
		[string] (Get-ObjectPropertyValue $correctedCanarySummary "evidenceKind") -cne
			"packaged-campaign-debug-corrected-canary" -or
		$null -eq $correctedSummaryCandidate -or
		$null -eq $correctedSummaryHarness -or
		$null -eq $correctedSummarySettings -or
		$null -eq $correctedSummaryCapture -or
		$null -eq $correctedSummaryResult -or
		$null -eq $correctedSummaryProof -or
		$null -eq $correctedSummaryDiagnostics -or
		$null -eq $correctedSummaryCleanup -or
		$null -eq $correctedSummaryIntegrity -or
		$null -eq $correctedSummaryFinding) {
		throw "The tracked corrected canary summary is structurally incomplete."
	}
	Assert-IntegerProperties $correctedSummarySettings @("schemaVersion") `
		"corrected canary summary.settings"
	Assert-IntegerProperties $correctedSummaryCapture @("runtimeSeconds") `
		"corrected canary summary.capture"
	Assert-IntegerProperties $correctedSummaryProof @(
		"caseCount", "pass", "warn", "fail", "blocked", "skipped",
		"focusedAssertionCount", "focusedAssertionsPassed",
		"certificationRequired", "certificationProven", "certificationFail",
		"certificationBlocked", "stateDiffRows", "nonzeroStateDiffRows",
		"finalOrphanActiveGroups") "corrected canary summary.proof"
	Assert-IntegerProperties $correctedSummaryDiagnostics @(
		"hardDiagnosticCount", "scriptErrors", "engineErrors", "partisanErrors",
		"crashMarkers", "partisanSeverityLineCount", "approvedStockDiagnosticCount",
		"approvedIntentionalDiagnosticCount", "unapprovedHardDiagnosticCount",
		"classifierSelfTestCount", "malformedHardDiagnosticCount",
		"canonicalScriptLogCount", "canonicalConsoleLogCount") `
		"corrected canary summary.diagnostics"
	Assert-IntegerProperties $correctedSummaryCleanup @(
		"guardRemaining", "ownedProcessesRemaining", "newEngineProcessesRemaining",
		"unclaimedEngineProcessesObserved", "newDefaultEntriesRemaining",
		"modifiedDefaultFiles", "deletedDefaultEntries", "missingDefaultRoots",
		"externalSpillEntriesRemaining", "modifiedSpillFiles", "deletedSpillEntries",
		"missingSpillRoots", "cleanupPhaseErrorCount") "corrected canary summary.cleanup"
	Assert-IntegerProperties $correctedSummaryIntegrity @("envelopeFileCount") `
		"corrected canary summary.integrity"
	Assert-IntegerProperties $correctedCanary @(
		"runtimeSeconds", "caseCount", "pass", "warn", "fail", "blocked", "skipped",
		"focusedAssertionCount", "focusedAssertionsPassed", "certificationRequired",
		"certificationProven", "stateDiffRows", "nonzeroStateDiffRows",
		"hardDiagnosticCount", "scriptErrors", "engineErrors", "partisanErrors",
		"approvedStockDiagnosticCount", "approvedIntentionalDiagnosticCount",
		"unapprovedHardDiagnosticCount", "classifierSelfTestCount",
		"canonicalScriptLogCount", "canonicalConsoleLogCount", "envelopeFileCount") `
		"release_status.historicalCandidateEvidence.evidence.correctedForceAuthorityCanary"

	if ([string] (Get-ObjectPropertyValue $correctedCanary "candidateId") -cne $historicalCandidateId -or
		[string] (Get-ObjectPropertyValue $correctedCanary "candidateSourceHead") -cne $historicalCandidateSourceHead -or
		[string] (Get-ObjectPropertyValue $correctedCanary "packageSha256") -cne $historicalPackageSha -or
		[string] (Get-ObjectPropertyValue $correctedCanary "manifestSha256") -cne $historicalCandidateManifestSha -or
		[string] (Get-ObjectPropertyValue $correctedCanary "readySha256") -cne $historicalCandidateReadySha -or
		[string] (Get-ObjectPropertyValue $correctedSummaryCandidate "candidateId") -cne $historicalCandidateId -or
		[string] (Get-ObjectPropertyValue $correctedSummaryCandidate "candidateSourceHead") -cne $historicalCandidateSourceHead -or
		[string] (Get-ObjectPropertyValue $correctedSummaryCandidate "packageSha256") -cne $historicalPackageSha -or
		[string] (Get-ObjectPropertyValue $correctedSummaryCandidate "manifestSha256") -cne $historicalCandidateManifestSha -or
		[string] (Get-ObjectPropertyValue $correctedSummaryCandidate "readySha256") -cne $historicalCandidateReadySha -or
		[string] (Get-ObjectPropertyValue $correctedSummaryCandidate "workbenchCrc") -cne $historicalWorkbenchCrc -or
		[string] (Get-ObjectPropertyValue $correctedSummaryCandidate "runtimeUseDispositionAtCapture") -cne
			"active-runtime-candidate") {
		throw "Historical corrected canary evidence differs from its retained candidate identity."
	}

	$correctedCampaignRunnerSha = Require-Sha256 `
		(Get-ObjectPropertyValue $correctedCanary "campaignRunnerSha256") `
		"release_status.historicalCandidateEvidence.evidence.correctedForceAuthorityCanary.campaignRunnerSha256"
	$correctedCandidateModuleSha = Require-Sha256 `
		(Get-ObjectPropertyValue $correctedCanary "candidateModuleSha256") `
		"release_status.historicalCandidateEvidence.evidence.correctedForceAuthorityCanary.candidateModuleSha256"
	$correctedSettingsSha = Require-Sha256 `
		(Get-ObjectPropertyValue $correctedCanary "settingsSha256") `
		"release_status.historicalCandidateEvidence.evidence.correctedForceAuthorityCanary.settingsSha256"
	if ([string] (Get-ObjectPropertyValue $correctedSummaryHarness "gitHead") -cne
			$correctedCanaryHarnessHead -or
		(Get-ObjectPropertyValue $correctedSummaryHarness "dirty") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $correctedSummaryHarness "dirty") -or
		[string] (Get-ObjectPropertyValue $correctedSummaryHarness "campaignRunnerSha256") -cne
			$correctedCampaignRunnerSha -or
		[string] (Get-ObjectPropertyValue $correctedSummaryHarness "candidateModuleSha256") -cne
			$correctedCandidateModuleSha -or
		[int] (Get-ObjectPropertyValue $correctedSummarySettings "schemaVersion") -ne $sourceSettingsSchema -or
		[string] (Get-ObjectPropertyValue $correctedSummarySettings "sha256") -cne $correctedSettingsSha -or
		(Get-ObjectPropertyValue $correctedSummarySettings "guardedRuntimeCopy") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummarySettings "guardedRuntimeCopy")) {
		throw "The corrected canary harness or settings identity is inconsistent."
	}

	$correctedStartedUtc = Require-UtcTimestamp `
		(Get-ObjectPropertyValue $correctedSummaryCapture "startedUtc") `
		"corrected canary start time"
	$correctedCompletedUtc = Require-UtcTimestamp `
		(Get-ObjectPropertyValue $correctedSummaryCapture "completedUtc") `
		"corrected canary completion time"
	$statusAsOfUtc = Require-UtcTimestamp $status.statusAsOfUtc "release_status.statusAsOfUtc"
	if ($correctedCompletedUtc -lt $correctedStartedUtc -or $statusAsOfUtc -lt $correctedCompletedUtc -or
		[string] (Get-ObjectPropertyValue $correctedCanary "startedUtc") -cne
			[string] (Get-ObjectPropertyValue $correctedSummaryCapture "startedUtc") -or
		[string] (Get-ObjectPropertyValue $correctedCanary "completedUtc") -cne
			[string] (Get-ObjectPropertyValue $correctedSummaryCapture "completedUtc") -or
		[string] (Get-ObjectPropertyValue $correctedCanary "runLeafId") -cne
			[string] (Get-ObjectPropertyValue $correctedSummaryCapture "runLeafId") -or
		[string] (Get-ObjectPropertyValue $correctedSummaryCapture "runLeafId") -cnotmatch
			'^\d{8}T\d{6}Z-[0-9a-f]{32}$' -or
		[string] (Get-ObjectPropertyValue $correctedCanary "runId") -cne
			[string] (Get-ObjectPropertyValue $correctedSummaryCapture "runId") -or
		[string] (Get-ObjectPropertyValue $correctedSummaryCapture "runId") -cne
			"seed1985_t0_p1_u1784420040" -or
		[string] (Get-ObjectPropertyValue $correctedSummaryCapture "profile") -cne "force_authority" -or
		[string] (Get-ObjectPropertyValue $correctedSummaryCapture "proofScope") -cne
			"focused_force_authority" -or
		[int] (Get-ObjectPropertyValue $correctedCanary "runtimeSeconds") -ne 45 -or
		[int] (Get-ObjectPropertyValue $correctedSummaryCapture "runtimeSeconds") -ne 45) {
		throw "The corrected canary capture identity, timestamps, or runtime are inconsistent."
	}

	$correctedCanaryError = "Campaign Debug runtime completed with unapproved hard diagnostics."
	if ([string] (Get-ObjectPropertyValue $correctedSummaryResult "status") -cne
			"failed-unapproved-hard-diagnostic" -or
		(Get-ObjectPropertyValue $correctedCanary "outcomeSuccess") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $correctedCanary "outcomeSuccess") -or
		(Get-ObjectPropertyValue $correctedSummaryResult "success") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $correctedSummaryResult "success") -or
		[string] (Get-ObjectPropertyValue $correctedCanary "error") -cne $correctedCanaryError -or
		[string] (Get-ObjectPropertyValue $correctedSummaryResult "error") -cne $correctedCanaryError -or
		[string] (Get-ObjectPropertyValue $correctedCanary "acceptanceDisposition") -cne
			"rejected-fail-closed" -or
		[string] (Get-ObjectPropertyValue $correctedSummaryResult "acceptanceDisposition") -cne
			"rejected-fail-closed" -or
		[string] (Get-ObjectPropertyValue $correctedSummaryResult "releaseDisposition") -cne
			"replacement-required" -or
		(Get-ObjectPropertyValue $correctedSummaryResult "armed") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryResult "armed") -or
		(Get-ObjectPropertyValue $correctedSummaryResult "started") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryResult "started") -or
		(Get-ObjectPropertyValue $correctedSummaryResult "completed") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryResult "completed") -or
		(Get-ObjectPropertyValue $correctedSummaryResult "candidateBoundaryVerified") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryResult "candidateBoundaryVerified") -or
		(Get-ObjectPropertyValue $correctedSummaryResult "mountPacked") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryResult "mountPacked") -or
		(Get-ObjectPropertyValue $correctedSummaryResult "artifactsStable") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryResult "artifactsStable") -or
		(Get-ObjectPropertyValue $correctedSummaryResult "evidenceCaptured") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryResult "evidenceCaptured") -or
		(Get-ObjectPropertyValue $correctedSummaryResult "artifactSchemaValidationValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryResult "artifactSchemaValidationValid") -or
		(Get-ObjectPropertyValue $correctedSummaryResult "certificationPassed") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $correctedSummaryResult "certificationPassed")) {
		throw "The corrected canary did not retain its exact fail-closed outcome."
	}

	$correctedCaseCount = [int] (Get-ObjectPropertyValue $correctedSummaryProof "caseCount")
	$correctedPass = [int] (Get-ObjectPropertyValue $correctedSummaryProof "pass")
	$correctedWarn = [int] (Get-ObjectPropertyValue $correctedSummaryProof "warn")
	$correctedFail = [int] (Get-ObjectPropertyValue $correctedSummaryProof "fail")
	$correctedBlocked = [int] (Get-ObjectPropertyValue $correctedSummaryProof "blocked")
	$correctedSkipped = [int] (Get-ObjectPropertyValue $correctedSummaryProof "skipped")
	if ($correctedCaseCount -ne 11 -or $correctedPass -ne 9 -or $correctedWarn -ne 1 -or
		$correctedFail -ne 0 -or $correctedBlocked -ne 1 -or $correctedSkipped -ne 0 -or
		$correctedCaseCount -ne
			($correctedPass + $correctedWarn + $correctedFail + $correctedBlocked + $correctedSkipped) -or
		[string] (Get-ObjectPropertyValue $correctedSummaryProof "focusedCaseId") -cne
			"early_mechanics.force_authority" -or
		[string] (Get-ObjectPropertyValue $correctedSummaryProof "focusedCaseStatus") -cne "PASS" -or
		[int] (Get-ObjectPropertyValue $correctedSummaryProof "focusedAssertionCount") -ne 35 -or
		[int] (Get-ObjectPropertyValue $correctedSummaryProof "focusedAssertionsPassed") -ne 35 -or
		[int] (Get-ObjectPropertyValue $correctedSummaryProof "certificationRequired") -ne 87 -or
		[int] (Get-ObjectPropertyValue $correctedSummaryProof "certificationProven") -ne 87 -or
		[int] (Get-ObjectPropertyValue $correctedSummaryProof "certificationFail") -ne 0 -or
		[int] (Get-ObjectPropertyValue $correctedSummaryProof "certificationBlocked") -ne 0 -or
		[int] (Get-ObjectPropertyValue $correctedSummaryProof "stateDiffRows") -ne 18 -or
		[int] (Get-ObjectPropertyValue $correctedSummaryProof "nonzeroStateDiffRows") -ne 0 -or
		(Get-ObjectPropertyValue $correctedSummaryProof "finalOrphanCleanupPass") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryProof "finalOrphanCleanupPass") -or
		[int] (Get-ObjectPropertyValue $correctedSummaryProof "finalOrphanActiveGroups") -ne 0 -or
		[int] (Get-ObjectPropertyValue $correctedCanary "caseCount") -ne $correctedCaseCount -or
		[int] (Get-ObjectPropertyValue $correctedCanary "pass") -ne $correctedPass -or
		[int] (Get-ObjectPropertyValue $correctedCanary "warn") -ne $correctedWarn -or
		[int] (Get-ObjectPropertyValue $correctedCanary "fail") -ne $correctedFail -or
		[int] (Get-ObjectPropertyValue $correctedCanary "blocked") -ne $correctedBlocked -or
		[int] (Get-ObjectPropertyValue $correctedCanary "skipped") -ne $correctedSkipped -or
		[int] (Get-ObjectPropertyValue $correctedCanary "focusedAssertionCount") -ne 35 -or
		[int] (Get-ObjectPropertyValue $correctedCanary "focusedAssertionsPassed") -ne 35 -or
		[int] (Get-ObjectPropertyValue $correctedCanary "certificationRequired") -ne 87 -or
		[int] (Get-ObjectPropertyValue $correctedCanary "certificationProven") -ne 87 -or
		[int] (Get-ObjectPropertyValue $correctedCanary "stateDiffRows") -ne 18 -or
		[int] (Get-ObjectPropertyValue $correctedCanary "nonzeroStateDiffRows") -ne 0 -or
		(Get-ObjectPropertyValue $correctedCanary "finalOrphanCleanupPass") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedCanary "finalOrphanCleanupPass")) {
		throw "The corrected canary proof totals are inconsistent."
	}

	$correctedHardCount = [int] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "hardDiagnosticCount")
	$correctedScriptErrors = [int] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "scriptErrors")
	$correctedEngineErrors = [int] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "engineErrors")
	$correctedPartisanErrors = [int] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "partisanErrors")
	$correctedStockCount = [int] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "approvedStockDiagnosticCount")
	$correctedIntentionalCount = [int] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "approvedIntentionalDiagnosticCount")
	$correctedUnapprovedCount = [int] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "unapprovedHardDiagnosticCount")
	$correctedUnapprovedKinds = @(Get-ObjectPropertyValue $correctedSummaryDiagnostics "unapprovedHardDiagnosticKinds")
	if ($correctedUnapprovedKinds.Count -eq 1) {
		Require-IntegerProperty $correctedUnapprovedKinds[0] "count" `
			"corrected canary summary.diagnostics.unapprovedHardDiagnosticKinds[0].count" | Out-Null
	}
	if ((Get-ObjectPropertyValue $correctedSummaryDiagnostics "valid") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "valid") -or
		(Get-ObjectPropertyValue $correctedSummaryDiagnostics "classificationValid") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "classificationValid") -or
		(Get-ObjectPropertyValue $correctedSummaryDiagnostics "hardDiagnosticFree") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "hardDiagnosticFree") -or
		$correctedHardCount -ne 3 -or $correctedScriptErrors -ne 3 -or
		$correctedEngineErrors -ne 0 -or $correctedPartisanErrors -ne 0 -or
		$correctedHardCount -ne ($correctedScriptErrors + $correctedEngineErrors) -or
		$correctedStockCount -ne 2 -or $correctedIntentionalCount -ne 0 -or
		$correctedUnapprovedCount -ne 1 -or
		$correctedHardCount -ne
			($correctedStockCount + $correctedIntentionalCount + $correctedUnapprovedCount) -or
		$correctedUnapprovedKinds.Count -ne 1 -or
		[string] (Get-ObjectPropertyValue $correctedUnapprovedKinds[0] "kind") -cne
			"virtual-machine-exception" -or
		[int] (Get-ObjectPropertyValue $correctedUnapprovedKinds[0] "count") -ne 1 -or
		[int] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "classifierSelfTestCount") -ne 33 -or
		[int] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "crashMarkers") -ne 0 -or
		[int] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "partisanSeverityLineCount") -ne 0 -or
		[int] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "malformedHardDiagnosticCount") -ne 0 -or
		(Get-ObjectPropertyValue $correctedSummaryDiagnostics "channelArithmeticValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "channelArithmeticValid") -or
		(Get-ObjectPropertyValue $correctedSummaryDiagnostics "categoryArithmeticValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "categoryArithmeticValid") -or
		(Get-ObjectPropertyValue $correctedSummaryDiagnostics "lifecycleMarkersValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "lifecycleMarkersValid") -or
		(Get-ObjectPropertyValue $correctedSummaryDiagnostics "identityBaselinePairValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "identityBaselinePairValid") -or
		(Get-ObjectPropertyValue $correctedSummaryDiagnostics "intentionalFixtureStructureExact") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "intentionalFixtureStructureExact") -or
		(Get-ObjectPropertyValue $correctedSummaryDiagnostics "intentionalFixtureSetValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "intentionalFixtureSetValid") -or
		[int] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "canonicalScriptLogCount") -ne 1 -or
		[int] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "canonicalConsoleLogCount") -ne 1 -or
		(Get-ObjectPropertyValue $correctedSummaryDiagnostics "canonicalLogPairSameDirectory") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryDiagnostics "canonicalLogPairSameDirectory") -or
		(Get-ObjectPropertyValue $correctedCanary "diagnosticClassificationValid") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $correctedCanary "diagnosticClassificationValid") -or
		(Get-ObjectPropertyValue $correctedCanary "hardDiagnosticFree") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $correctedCanary "hardDiagnosticFree") -or
		[int] (Get-ObjectPropertyValue $correctedCanary "hardDiagnosticCount") -ne $correctedHardCount -or
		[int] (Get-ObjectPropertyValue $correctedCanary "scriptErrors") -ne $correctedScriptErrors -or
		[int] (Get-ObjectPropertyValue $correctedCanary "engineErrors") -ne $correctedEngineErrors -or
		[int] (Get-ObjectPropertyValue $correctedCanary "partisanErrors") -ne $correctedPartisanErrors -or
		[int] (Get-ObjectPropertyValue $correctedCanary "approvedStockDiagnosticCount") -ne $correctedStockCount -or
		[int] (Get-ObjectPropertyValue $correctedCanary "approvedIntentionalDiagnosticCount") -ne
			$correctedIntentionalCount -or
		[int] (Get-ObjectPropertyValue $correctedCanary "unapprovedHardDiagnosticCount") -ne
			$correctedUnapprovedCount -or
		[string] (Get-ObjectPropertyValue $correctedCanary "unapprovedHardDiagnosticKind") -cne
			"virtual-machine-exception" -or
		[int] (Get-ObjectPropertyValue $correctedCanary "classifierSelfTestCount") -ne 33 -or
		[int] (Get-ObjectPropertyValue $correctedCanary "canonicalScriptLogCount") -ne 1 -or
		[int] (Get-ObjectPropertyValue $correctedCanary "canonicalConsoleLogCount") -ne 1 -or
		(Get-ObjectPropertyValue $correctedCanary "canonicalLogPairSameDirectory") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedCanary "canonicalLogPairSameDirectory")) {
		throw "The corrected canary diagnostic census is inconsistent."
	}

	$correctedCleanupFields = @(
		"guardRemaining",
		"ownedProcessesRemaining",
		"newEngineProcessesRemaining",
		"unclaimedEngineProcessesObserved",
		"newDefaultEntriesRemaining",
		"modifiedDefaultFiles",
		"deletedDefaultEntries",
		"missingDefaultRoots",
		"externalSpillEntriesRemaining",
		"modifiedSpillFiles",
		"deletedSpillEntries",
		"missingSpillRoots",
		"cleanupPhaseErrorCount")
	foreach ($correctedCleanupField in $correctedCleanupFields) {
		if ($null -eq (Get-ObjectPropertyValue $correctedSummaryCleanup $correctedCleanupField) -or
			[int] (Get-ObjectPropertyValue $correctedSummaryCleanup $correctedCleanupField) -ne 0) {
			throw "The corrected canary cleanup field $correctedCleanupField must be present and zero."
		}
	}
	if ((Get-ObjectPropertyValue $correctedSummaryCleanup "monitoringRootsAreDetectionOnly") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryCleanup "monitoringRootsAreDetectionOnly") -or
		(Get-ObjectPropertyValue $correctedCanary "cleanupAndSpillZero") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedCanary "cleanupAndSpillZero")) {
		throw "The corrected canary cleanup and spill boundary is not clean."
	}

	$correctedEnvelopeSha = Require-Sha256 `
		(Get-ObjectPropertyValue $correctedCanary "envelopeSha256") `
		"release_status.historicalCandidateEvidence.evidence.correctedForceAuthorityCanary.envelopeSha256"
	$correctedRunSummarySha = Require-Sha256 `
		(Get-ObjectPropertyValue $correctedCanary "runSummarySha256") `
		"release_status.historicalCandidateEvidence.evidence.correctedForceAuthorityCanary.runSummarySha256"
	if ($correctedEnvelopeSha -cne $correctedRunSummarySha -or
		[string] (Get-ObjectPropertyValue $correctedSummaryIntegrity "envelopeSha256") -cne
			$correctedEnvelopeSha -or
		[string] (Get-ObjectPropertyValue $correctedSummaryIntegrity "runSummarySha256") -cne
			$correctedRunSummarySha -or
		[int] (Get-ObjectPropertyValue $correctedSummaryIntegrity "envelopeFileCount") -ne 10 -or
		[int] (Get-ObjectPropertyValue $correctedCanary "envelopeFileCount") -ne 10 -or
		(Get-ObjectPropertyValue $correctedSummaryIntegrity "envelopeFilesRehashed") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedSummaryIntegrity "envelopeFilesRehashed") -or
		(Get-ObjectPropertyValue $correctedCanary "envelopeFilesRehashed") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $correctedCanary "envelopeFilesRehashed") -or
		[string] (Get-ObjectPropertyValue $correctedSummaryFinding "status") -cne
			"source-fix-required" -or
		[string] (Get-ObjectPropertyValue $correctedSummaryFinding "defect") -cne
			"MapLocator runtime lifecycle" -or
		[string]::IsNullOrWhiteSpace(
			[string] (Get-ObjectPropertyValue $correctedSummaryFinding "nextStep"))) {
		throw "The corrected canary integrity or source-fix disposition is inconsistent."
	}

	if ($null -eq $fullCampaignDebug) {
		throw "Release status must contain Full Campaign Debug evidence."
	}
	$fullCampaignDebugStatus = Require-Text `
		(Get-ObjectPropertyValue $fullCampaignDebug "status") `
		"release_status.historicalCandidateEvidence.evidence.fullCampaignDebug.status"
	if ($fullCampaignDebugStatus -ceq "historical-preliminary-failed-diagnostic-census") {
		$fullCampaignSummaryPath = Require-RepoRelativePath `
			(Get-ObjectPropertyValue $fullCampaignDebug "summaryPath") `
			"release_status.historicalCandidateEvidence.evidence.fullCampaignDebug.summaryPath"
		$fullCampaignSummarySha = Require-Sha256 `
			(Get-ObjectPropertyValue $fullCampaignDebug "summarySha256") `
			"release_status.historicalCandidateEvidence.evidence.fullCampaignDebug.summarySha256"
		$fullCampaignHarnessHead = Require-Text `
			(Get-ObjectPropertyValue $fullCampaignDebug "harnessGitHead") `
			"release_status.historicalCandidateEvidence.evidence.fullCampaignDebug.harnessGitHead"
		if ($fullCampaignHarnessHead -cnotmatch '^[0-9a-f]{40}$') {
			throw "The Full Campaign Debug harness HEAD must be a lowercase full Git SHA."
		}

		$fullCampaignSummaryFullPath = [IO.Path]::GetFullPath(
			(Join-Path $root $fullCampaignSummaryPath))
		if (-not $fullCampaignSummaryFullPath.StartsWith(
			$repositoryPrefix,
			[StringComparison]::OrdinalIgnoreCase) -or
			-not (Test-Path -LiteralPath $fullCampaignSummaryFullPath -PathType Leaf)) {
			throw "The tracked Full Campaign Debug summary is missing or outside the repository."
		}
		$fullCampaignSummaryItem = Get-Item -LiteralPath $fullCampaignSummaryFullPath -Force
		if (($fullCampaignSummaryItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
			throw "The tracked Full Campaign Debug summary must not be a reparse point."
		}
		$actualFullCampaignSummarySha = (Get-FileHash `
			-LiteralPath $fullCampaignSummaryFullPath `
			-Algorithm SHA256).Hash.ToLowerInvariant()
		if ($actualFullCampaignSummarySha -cne $fullCampaignSummarySha.ToLowerInvariant()) {
			throw "The Full Campaign Debug summary SHA-256 does not match release status."
		}

		$fullCampaignSummaryText = Get-Content -Raw -LiteralPath $fullCampaignSummaryFullPath
		if ($fullCampaignSummaryText -match '(?i)[A-Z]:[\\/]') {
			throw "The tracked Full Campaign Debug summary contains a local absolute path."
		}
		$fullCampaignSummary = $fullCampaignSummaryText | ConvertFrom-Json
		$fullSummaryCandidate = Get-ObjectPropertyValue $fullCampaignSummary "candidate"
		$fullSummaryHarness = Get-ObjectPropertyValue $fullCampaignSummary "harness"
		$fullSummarySettings = Get-ObjectPropertyValue $fullCampaignSummary "settings"
		$fullSummaryDiagnosticAudit = Get-ObjectPropertyValue $fullCampaignSummary "diagnosticAudit"
		$fullSummaryCaptureWindow = Get-ObjectPropertyValue $fullCampaignSummary "captureWindow"
		$fullSummaryResult = Get-ObjectPropertyValue $fullCampaignSummary "result"
		$fullSummaryHistoricalComparison = Get-ObjectPropertyValue $fullCampaignSummary "historicalComparison"
		$fullSummaryRuns = @(Get-ObjectPropertyValue $fullCampaignSummary "runs")
		if ([int] (Get-ObjectPropertyValue $fullCampaignSummary "schemaVersion") -ne 1 -or
			[string] (Get-ObjectPropertyValue $fullCampaignSummary "evidenceKind") -cne "packaged-campaign-debug-set" -or
			$null -eq $fullSummaryCandidate -or
			$null -eq $fullSummaryHarness -or
			$null -eq $fullSummarySettings -or
			$null -eq $fullSummaryDiagnosticAudit -or
			$null -eq $fullSummaryCaptureWindow -or
			$null -eq $fullSummaryResult -or
			$null -eq $fullSummaryHistoricalComparison -or
			$fullSummaryRuns.Count -ne 2) {
			throw "The tracked Full Campaign Debug summary is structurally incomplete."
		}
		$diagnosticAuditPerformedUtc = Require-UtcTimestamp `
			(Get-ObjectPropertyValue $fullSummaryDiagnosticAudit "performedUtc") `
			"Campaign Debug corrected diagnostic-audit time"
		if ([string] (Get-ObjectPropertyValue $fullSummaryDiagnosticAudit "status") -cne
				"completed-corrected-overlay" -or
			[string] (Get-ObjectPropertyValue $fullSummaryDiagnosticAudit "source") -cne
				"one immutable canonical script.log plus console.log per run" -or
			(Get-ObjectPropertyValue $fullSummaryDiagnosticAudit "rawSidecarsModified") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $fullSummaryDiagnosticAudit "rawSidecarsModified") -or
			[string] (Get-ObjectPropertyValue $fullSummaryDiagnosticAudit "classifierContract") -cne
				"timestamp-aware-exact-proof-bound-v1" -or
			[int] (Get-ObjectPropertyValue $fullSummaryDiagnosticAudit "classifierSelfTestCount") -ne 33 -or
			[string] (Get-ObjectPropertyValue $fullSummaryDiagnosticAudit "countingRule") -cne
				"Canonical script.log and same-session console.log are merged as occurrence multisets; mirrored events count once, while same-source duplicates and console-only hard, severity, or crash signals remain visible. Partisan errors are a subset of hard diagnostics, not an additional count." -or
			$diagnosticAuditPerformedUtc -lt [DateTimeOffset]::Parse(
				"2026-07-18T22:47:18.3217797Z")) {
			throw "The tracked Campaign Debug corrected diagnostic audit is incomplete."
		}

		if ([string] (Get-ObjectPropertyValue $fullCampaignDebug "candidateId") -cne $historicalCandidateId -or
			[string] (Get-ObjectPropertyValue $fullCampaignDebug "candidateSourceHead") -cne $historicalCandidateSourceHead -or
			[string] (Get-ObjectPropertyValue $fullCampaignDebug "sourceSha") -cne $historicalCandidateSourceHead -or
			[string] (Get-ObjectPropertyValue $fullCampaignDebug "packageSha256") -cne $historicalPackageSha -or
			[string] (Get-ObjectPropertyValue $fullCampaignDebug "manifestSha256") -cne $historicalCandidateManifestSha -or
			[string] (Get-ObjectPropertyValue $fullCampaignDebug "readySha256") -cne $historicalCandidateReadySha -or
			[string] (Get-ObjectPropertyValue $fullSummaryCandidate "candidateId") -cne $historicalCandidateId -or
			[string] (Get-ObjectPropertyValue $fullSummaryCandidate "candidateSourceHead") -cne $historicalCandidateSourceHead -or
			[string] (Get-ObjectPropertyValue $fullSummaryCandidate "packageSha256") -cne $historicalPackageSha -or
			[string] (Get-ObjectPropertyValue $fullSummaryCandidate "manifestSha256") -cne $historicalCandidateManifestSha -or
			[string] (Get-ObjectPropertyValue $fullSummaryCandidate "readySha256") -cne $historicalCandidateReadySha -or
			[string] (Get-ObjectPropertyValue $fullSummaryCandidate "workbenchCrc") -cne $historicalWorkbenchCrc) {
			throw "Historical Full Campaign Debug evidence differs from its retained candidate identity."
		}

		$fullCampaignRunnerSha = Require-Sha256 `
			(Get-ObjectPropertyValue $fullCampaignDebug "campaignRunnerSha256") `
			"release_status.historicalCandidateEvidence.evidence.fullCampaignDebug.campaignRunnerSha256"
		$fullCandidateModuleSha = Require-Sha256 `
			(Get-ObjectPropertyValue $fullCampaignDebug "candidateModuleSha256") `
			"release_status.historicalCandidateEvidence.evidence.fullCampaignDebug.candidateModuleSha256"
		$fullSettingsSha = Require-Sha256 `
			(Get-ObjectPropertyValue $fullCampaignDebug "settingsSha256") `
			"release_status.historicalCandidateEvidence.evidence.fullCampaignDebug.settingsSha256"
		if ([string] (Get-ObjectPropertyValue $fullSummaryHarness "gitHead") -cne $fullCampaignHarnessHead -or
			(Get-ObjectPropertyValue $fullSummaryHarness "dirty") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $fullSummaryHarness "dirty") -or
			[string] (Get-ObjectPropertyValue $fullSummaryHarness "campaignRunnerSha256") -cne $fullCampaignRunnerSha -or
			[string] (Get-ObjectPropertyValue $fullSummaryHarness "candidateModuleSha256") -cne $fullCandidateModuleSha -or
			[int] (Get-ObjectPropertyValue $fullSummarySettings "schemaVersion") -ne $sourceSettingsSchema -or
			[string] (Get-ObjectPropertyValue $fullSummarySettings "sha256") -cne $fullSettingsSha -or
			(Get-ObjectPropertyValue $fullSummarySettings "guardedRuntimeCopy") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullSummarySettings "guardedRuntimeCopy")) {
			throw "Full Campaign Debug evidence does not bind one clean harness and guarded settings copy."
		}

		$forceAuthorityRuns = @($fullSummaryRuns | Where-Object {
			[string] (Get-ObjectPropertyValue $_ "profile") -ceq "force_authority"
		})
		$fullCertificationRuns = @($fullSummaryRuns | Where-Object {
			[string] (Get-ObjectPropertyValue $_ "profile") -ceq "full_certification"
		})
		if ($forceAuthorityRuns.Count -ne 1 -or $fullCertificationRuns.Count -ne 1) {
			throw "The Full Campaign Debug summary must contain one force canary and one full run."
		}
		$forceAuthorityRun = $forceAuthorityRuns[0]
		$fullCertificationRun = $fullCertificationRuns[0]
		$campaignRunIds = @($fullSummaryRuns | ForEach-Object {
			Require-Text (Get-ObjectPropertyValue $_ "runId") "Campaign Debug run ID"
		})
		$campaignRunLeafIds = @($fullSummaryRuns | ForEach-Object {
			Require-Text (Get-ObjectPropertyValue $_ "runLeafId") "Campaign Debug run leaf ID"
		})
		$campaignEnvelopeHashes = @($fullSummaryRuns | ForEach-Object {
			Require-Sha256 (Get-ObjectPropertyValue $_ "envelopeSha256") "Campaign Debug envelope SHA-256"
		})
		Assert-UniqueStrings $campaignRunIds "Campaign Debug run IDs"
		Assert-UniqueStrings $campaignRunLeafIds "Campaign Debug run leaf IDs"
		Assert-UniqueStrings $campaignEnvelopeHashes "Campaign Debug envelope SHA-256 values"

		$captureWindowStarted = Require-UtcTimestamp `
			(Get-ObjectPropertyValue $fullSummaryCaptureWindow "startedUtc") `
			"Campaign Debug capture-window start"
		$captureWindowCompleted = Require-UtcTimestamp `
			(Get-ObjectPropertyValue $fullSummaryCaptureWindow "completedUtc") `
			"Campaign Debug capture-window completion"
		$forceStarted = Require-UtcTimestamp `
			(Get-ObjectPropertyValue $forceAuthorityRun "startedUtc") `
			"Campaign Debug force-canary start"
		$forceCompleted = Require-UtcTimestamp `
			(Get-ObjectPropertyValue $forceAuthorityRun "completedUtc") `
			"Campaign Debug force-canary completion"
		$fullStarted = Require-UtcTimestamp `
			(Get-ObjectPropertyValue $fullCertificationRun "startedUtc") `
			"Campaign Debug full-run start"
		$fullCompleted = Require-UtcTimestamp `
			(Get-ObjectPropertyValue $fullCertificationRun "completedUtc") `
			"Campaign Debug full-run completion"
		if ($forceStarted -ne $captureWindowStarted -or
			$fullCompleted -ne $captureWindowCompleted -or
			$forceCompleted -lt $forceStarted -or
			$fullStarted -lt $forceCompleted -or
			$fullCompleted -lt $fullStarted) {
			throw "Campaign Debug run chronology does not equal its capture window or canary-first order."
		}

		$summaryEnvelopeFileCount = [int] (Get-ObjectPropertyValue $forceAuthorityRun "fileCount") +
			[int] (Get-ObjectPropertyValue $fullCertificationRun "fileCount")
		$forceWrapperErrorCensus = Get-ObjectPropertyValue `
			$forceAuthorityRun `
			"wrapperReportedErrorCensus"
		if ([string] (Get-ObjectPropertyValue $forceAuthorityRun "proofScope") -cne "focused_force_authority" -or
			[string] (Get-ObjectPropertyValue $forceAuthorityRun "acceptanceDisposition") -cne
				"preliminary-unaccepted" -or
			(Get-ObjectPropertyValue $forceAuthorityRun "wrapperReportedSuccess") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "wrapperReportedSuccess") -or
			(Get-ObjectPropertyValue $forceAuthorityRun "candidateBoundaryVerified") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "candidateBoundaryVerified") -or
			(Get-ObjectPropertyValue $forceAuthorityRun "mountPacked") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "mountPacked") -or
			(Get-ObjectPropertyValue $forceAuthorityRun "artifactsStable") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "artifactsStable") -or
			(Get-ObjectPropertyValue $forceAuthorityRun "artifactSchemaValidationValid") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "artifactSchemaValidationValid") -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "fileCount") -ne 10 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "caseCount") -ne 11 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "pass") -ne 9 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "warn") -ne 1 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "fail") -ne 0 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "blocked") -ne 1 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "skipped") -ne 0 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "certificationRequired") -ne 87 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "certificationProven") -ne 87 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "certificationFail") -ne 0 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "certificationBlocked") -ne 0 -or
			(Get-ObjectPropertyValue $forceAuthorityRun "certificationPassed") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $forceAuthorityRun "certificationPassed") -or
			[string] (Get-ObjectPropertyValue $forceAuthorityRun "focusedCaseStatus") -cne "PASS" -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "focusedAssertionCount") -ne 35 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "stateDiffRows") -ne 18 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "nonzeroStateDiffRows") -ne 0 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "canonicalScriptLogCount") -ne 1 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "canonicalConsoleLogCount") -ne 1 -or
			(Get-ObjectPropertyValue $forceAuthorityRun "canonicalLogPairSameDirectory") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "canonicalLogPairSameDirectory") -or
			(Get-ObjectPropertyValue $forceAuthorityRun "hardDiagnosticFree") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $forceAuthorityRun "hardDiagnosticFree") -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "hardDiagnosticCount") -ne 3 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "scriptErrors") -ne 3 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "engineErrors") -ne 0 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "partisanErrors") -ne 0 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "crashMarkers") -ne 0 -or
			$null -eq (Get-ObjectPropertyValue $forceAuthorityRun "partisanSeverityLineCount") -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "partisanSeverityLineCount") -ne 0 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "approvedStockDiagnosticCount") -ne 2 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "approvedIntentionalDiagnosticCount") -ne 0 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "unapprovedHardDiagnosticCount") -ne 1 -or
			(Get-ObjectPropertyValue $forceAuthorityRun "diagnosticClassificationValid") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $forceAuthorityRun "diagnosticClassificationValid") -or
			(Get-ObjectPropertyValue $forceAuthorityRun "channelArithmeticValid") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "channelArithmeticValid") -or
			(Get-ObjectPropertyValue $forceAuthorityRun "categoryArithmeticValid") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "categoryArithmeticValid") -or
			$null -eq $forceWrapperErrorCensus -or
			[int] (Get-ObjectPropertyValue $forceWrapperErrorCensus "scriptErrors") -ne 0 -or
			[int] (Get-ObjectPropertyValue $forceWrapperErrorCensus "partisanErrors") -ne 0 -or
			[int] (Get-ObjectPropertyValue $forceWrapperErrorCensus "crashMarkers") -ne 0 -or
			(Get-ObjectPropertyValue $forceAuthorityRun "finalOrphanCleanupPass") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "finalOrphanCleanupPass")) {
			throw "The preliminary force-authority Campaign Debug capture is inconsistent."
		}

		$fullCaseCount = [int] (Get-ObjectPropertyValue $fullCertificationRun "caseCount")
		$fullPassCount = [int] (Get-ObjectPropertyValue $fullCertificationRun "pass")
		$fullWarnCount = [int] (Get-ObjectPropertyValue $fullCertificationRun "warn")
		$fullFailCount = [int] (Get-ObjectPropertyValue $fullCertificationRun "fail")
		$fullBlockedCount = [int] (Get-ObjectPropertyValue $fullCertificationRun "blocked")
		$fullSkippedCount = [int] (Get-ObjectPropertyValue $fullCertificationRun "skipped")
		$fullRequiredCount = [int] (Get-ObjectPropertyValue $fullCertificationRun "certificationRequired")
		$fullProvenCount = [int] (Get-ObjectPropertyValue $fullCertificationRun "certificationProven")
		$fullCertificationFailCount = [int] (Get-ObjectPropertyValue $fullCertificationRun "certificationFail")
		$fullCertificationBlockedCount = [int] (Get-ObjectPropertyValue $fullCertificationRun "certificationBlocked")
		if ($fullCaseCount -ne ($fullPassCount + $fullWarnCount + $fullFailCount + $fullBlockedCount + $fullSkippedCount) -or
			$fullRequiredCount -ne ($fullProvenCount + $fullCertificationFailCount + $fullCertificationBlockedCount) -or
			@($fullCaseCount, $fullPassCount, $fullWarnCount, $fullFailCount, $fullBlockedCount, $fullSkippedCount,
				$fullRequiredCount, $fullProvenCount, $fullCertificationFailCount, $fullCertificationBlockedCount |
				Where-Object { $_ -lt 0 }).Count -gt 0 -or
			($fullFailCount + $fullBlockedCount) -le 0) {
			throw "The current failed Full Campaign Debug totals are arithmetically inconsistent."
		}

		$failedCategoryRows = @(Get-ObjectPropertyValue $fullCertificationRun "failedCaseCountsByCategory")
		$blockedCategoryRows = @(Get-ObjectPropertyValue $fullCertificationRun "blockedCaseCountsByCategory")
		$failedCategoryNames = @()
		$blockedCategoryNames = @()
		$failedCategoryTotal = 0
		$blockedCategoryTotal = 0
		foreach ($categoryRow in $failedCategoryRows) {
			$failedCategoryNames += Require-Text `
				(Get-ObjectPropertyValue $categoryRow "category") `
				"Full Campaign Debug failed category"
			$categoryCount = [int] (Get-ObjectPropertyValue $categoryRow "count")
			if ($categoryCount -le 0) {
				throw "Full Campaign Debug failed-category counts must be positive."
			}
			$failedCategoryTotal += $categoryCount
		}
		foreach ($categoryRow in $blockedCategoryRows) {
			$blockedCategoryNames += Require-Text `
				(Get-ObjectPropertyValue $categoryRow "category") `
				"Full Campaign Debug blocked category"
			$categoryCount = [int] (Get-ObjectPropertyValue $categoryRow "count")
			if ($categoryCount -le 0) {
				throw "Full Campaign Debug blocked-category counts must be positive."
			}
			$blockedCategoryTotal += $categoryCount
		}
		Assert-UniqueStrings $failedCategoryNames "Full Campaign Debug failed categories"
		Assert-UniqueStrings $blockedCategoryNames "Full Campaign Debug blocked categories"
		if ($failedCategoryRows.Count -eq 0 -or
			$blockedCategoryRows.Count -eq 0 -or
			$failedCategoryTotal -ne $fullFailCount -or
			$blockedCategoryTotal -ne $fullBlockedCount) {
			throw "Full Campaign Debug category totals do not equal the failed and blocked case totals."
		}

		$fullWrapperErrorCensus = Get-ObjectPropertyValue `
			$fullCertificationRun `
			"wrapperReportedErrorCensus"
		if ([string] (Get-ObjectPropertyValue $fullCertificationRun "proofScope") -cne "full_certification" -or
			[string] (Get-ObjectPropertyValue $fullCertificationRun "acceptanceDisposition") -cne
				"preliminary-unaccepted" -or
			(Get-ObjectPropertyValue $fullCertificationRun "wrapperReportedSuccess") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCertificationRun "wrapperReportedSuccess") -or
			(Get-ObjectPropertyValue $fullCertificationRun "candidateBoundaryVerified") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCertificationRun "candidateBoundaryVerified") -or
			(Get-ObjectPropertyValue $fullCertificationRun "mountPacked") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCertificationRun "mountPacked") -or
			(Get-ObjectPropertyValue $fullCertificationRun "artifactsStable") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCertificationRun "artifactsStable") -or
			(Get-ObjectPropertyValue $fullCertificationRun "artifactSchemaValidationValid") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCertificationRun "artifactSchemaValidationValid") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "fileCount") -ne 10 -or
			(Get-ObjectPropertyValue $fullCertificationRun "certificationPassed") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $fullCertificationRun "certificationPassed") -or
			(Get-ObjectPropertyValue $fullCampaignDebug "certificationPassed") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $fullCampaignDebug "certificationPassed") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "stateDiffRows") -ne 18 -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "nonzeroStateDiffRows") -ne 0 -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "phase17Assertions") -ne 11 -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "phase17Passed") -ne 11 -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "phase24Assertions") -ne 2 -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "phase24Passed") -ne 2 -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "stagedCleanupCases") -ne 6 -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "stagedCleanupPassed") -ne 6 -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "canonicalScriptLogCount") -ne 1 -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "canonicalConsoleLogCount") -ne 1 -or
			(Get-ObjectPropertyValue $fullCertificationRun "canonicalLogPairSameDirectory") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCertificationRun "canonicalLogPairSameDirectory") -or
			(Get-ObjectPropertyValue $fullCertificationRun "hardDiagnosticFree") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $fullCertificationRun "hardDiagnosticFree") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "hardDiagnosticCount") -ne 25 -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "scriptErrors") -ne 25 -or
			$null -eq (Get-ObjectPropertyValue $fullCertificationRun "engineErrors") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "engineErrors") -ne 0 -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "partisanErrors") -ne 19 -or
			$null -eq (Get-ObjectPropertyValue $fullCertificationRun "crashMarkers") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "crashMarkers") -ne 0 -or
			$null -eq (Get-ObjectPropertyValue $fullCertificationRun "partisanSeverityLineCount") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "partisanSeverityLineCount") -ne 0 -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "approvedStockDiagnosticCount") -ne 2 -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "approvedIntentionalDiagnosticCount") -ne 13 -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "unapprovedHardDiagnosticCount") -ne 10 -or
			(Get-ObjectPropertyValue $fullCertificationRun "diagnosticClassificationValid") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $fullCertificationRun "diagnosticClassificationValid") -or
			(Get-ObjectPropertyValue $fullCertificationRun "channelArithmeticValid") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCertificationRun "channelArithmeticValid") -or
			(Get-ObjectPropertyValue $fullCertificationRun "categoryArithmeticValid") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCertificationRun "categoryArithmeticValid") -or
			$null -eq $fullWrapperErrorCensus -or
			$null -eq (Get-ObjectPropertyValue $fullWrapperErrorCensus "scriptErrors") -or
			[int] (Get-ObjectPropertyValue $fullWrapperErrorCensus "scriptErrors") -ne 0 -or
			$null -eq (Get-ObjectPropertyValue $fullWrapperErrorCensus "partisanErrors") -or
			[int] (Get-ObjectPropertyValue $fullWrapperErrorCensus "partisanErrors") -ne 0 -or
			$null -eq (Get-ObjectPropertyValue $fullWrapperErrorCensus "crashMarkers") -or
			[int] (Get-ObjectPropertyValue $fullWrapperErrorCensus "crashMarkers") -ne 0 -or
			(Get-ObjectPropertyValue $fullCertificationRun "finalOrphanCleanupPass") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCertificationRun "finalOrphanCleanupPass") -or
			[string] (Get-ObjectPropertyValue $fullCertificationRun "runId") -cne
				[string] (Get-ObjectPropertyValue $fullCampaignDebug "runId") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "caseCount") -ne
				[int] (Get-ObjectPropertyValue $fullCampaignDebug "totalCases") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "pass") -ne
				[int] (Get-ObjectPropertyValue $fullCampaignDebug "pass") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "warn") -ne
				[int] (Get-ObjectPropertyValue $fullCampaignDebug "warn") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "fail") -ne
				[int] (Get-ObjectPropertyValue $fullCampaignDebug "fail") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "blocked") -ne
				[int] (Get-ObjectPropertyValue $fullCampaignDebug "blocked") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "skipped") -ne
				[int] (Get-ObjectPropertyValue $fullCampaignDebug "skipped") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "certificationRequired") -ne
				[int] (Get-ObjectPropertyValue $fullCampaignDebug "requiredAssertions") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "certificationProven") -ne
				[int] (Get-ObjectPropertyValue $fullCampaignDebug "provenAssertions") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "certificationFail") -ne
				[int] (Get-ObjectPropertyValue $fullCampaignDebug "failedAssertions") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "certificationBlocked") -ne
				[int] (Get-ObjectPropertyValue $fullCampaignDebug "blockedAssertions") -or
			[string] (Get-ObjectPropertyValue $fullCertificationRun "envelopeSha256") -cne
				[string] (Get-ObjectPropertyValue $fullCampaignDebug "envelopeSha256")) {
			throw "Release status does not equal the current full Campaign Debug run."
		}

		$fullStatusHistoricalComparison = Get-ObjectPropertyValue $fullCampaignDebug "historicalComparison"
		if ($null -eq $fullStatusHistoricalComparison) {
			throw "Release status must retain the Full Campaign Debug historical comparison."
		}
		$historicalFields = @(
			"previousRunId",
			"previousPass",
			"previousWarn",
			"previousFail",
			"previousBlocked",
			"previousSkipped",
			"previousCertificationRequired",
			"previousCertificationProven",
			"previousCertificationFail",
			"previousCertificationBlocked",
			"passDelta",
			"warnDelta",
			"failDelta",
			"blockedDelta",
			"skippedDelta",
			"certificationRequiredDelta",
			"certificationProvenDelta",
			"certificationFailDelta",
			"certificationBlockedDelta"
		)
		foreach ($historicalField in $historicalFields) {
			if ([string] (Get-ObjectPropertyValue $fullStatusHistoricalComparison $historicalField) -cne
				[string] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison $historicalField)) {
				throw "Release status historical comparison differs from the tracked summary at $historicalField."
			}
		}

		$previousRunId = Require-Text `
			(Get-ObjectPropertyValue $fullSummaryHistoricalComparison "previousRunId") `
			"Full Campaign Debug historical run ID"
		if ($previousRunId -ceq [string] (Get-ObjectPropertyValue $fullCertificationRun "runId")) {
			throw "Full Campaign Debug current and historical run IDs must differ."
		}
		$previousPass = [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "previousPass")
		$previousWarn = [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "previousWarn")
		$previousFail = [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "previousFail")
		$previousBlocked = [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "previousBlocked")
		$previousSkipped = [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "previousSkipped")
		$previousRequired = [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "previousCertificationRequired")
		$previousProven = [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "previousCertificationProven")
		$previousCertificationFail = [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "previousCertificationFail")
		$previousCertificationBlocked = [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "previousCertificationBlocked")
		if (@($previousPass, $previousWarn, $previousFail, $previousBlocked, $previousSkipped,
				$previousRequired, $previousProven, $previousCertificationFail, $previousCertificationBlocked |
				Where-Object { $_ -lt 0 }).Count -gt 0 -or
			($previousPass + $previousWarn + $previousFail + $previousBlocked + $previousSkipped) -ne $fullCaseCount -or
			$previousRequired -ne ($previousProven + $previousCertificationFail + $previousCertificationBlocked) -or
			$previousPass + [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "passDelta") -ne $fullPassCount -or
			$previousWarn + [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "warnDelta") -ne $fullWarnCount -or
			$previousFail + [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "failDelta") -ne $fullFailCount -or
			$previousBlocked + [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "blockedDelta") -ne $fullBlockedCount -or
			$previousSkipped + [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "skippedDelta") -ne $fullSkippedCount -or
			$previousRequired + [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "certificationRequiredDelta") -ne $fullRequiredCount -or
			$previousProven + [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "certificationProvenDelta") -ne $fullProvenCount -or
			$previousCertificationFail + [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "certificationFailDelta") -ne $fullCertificationFailCount -or
			$previousCertificationBlocked + [int] (Get-ObjectPropertyValue $fullSummaryHistoricalComparison "certificationBlockedDelta") -ne $fullCertificationBlockedCount) {
			throw "Full Campaign Debug historical comparison totals or deltas are inconsistent."
		}

		foreach ($campaignRun in $fullSummaryRuns) {
			$campaignHardDiagnosticCount = [int] (Get-ObjectPropertyValue $campaignRun "hardDiagnosticCount")
			$campaignScriptErrorCount = [int] (Get-ObjectPropertyValue $campaignRun "scriptErrors")
			$campaignEngineErrorCount = [int] (Get-ObjectPropertyValue $campaignRun "engineErrors")
			$campaignPartisanErrorCount = [int] (Get-ObjectPropertyValue $campaignRun "partisanErrors")
			$campaignApprovedStockCount = [int] (Get-ObjectPropertyValue $campaignRun "approvedStockDiagnosticCount")
			$campaignApprovedIntentionalCount = [int] (Get-ObjectPropertyValue $campaignRun "approvedIntentionalDiagnosticCount")
			$campaignUnapprovedCount = [int] (Get-ObjectPropertyValue $campaignRun "unapprovedHardDiagnosticCount")
			$campaignWrapperErrorCensus = Get-ObjectPropertyValue $campaignRun "wrapperReportedErrorCensus"
			if ([int] (Get-ObjectPropertyValue $campaignRun "fileCount") -le 0 -or
				[int] (Get-ObjectPropertyValue $campaignRun "canonicalScriptLogCount") -ne 1 -or
				[int] (Get-ObjectPropertyValue $campaignRun "canonicalConsoleLogCount") -ne 1 -or
				(Get-ObjectPropertyValue $campaignRun "canonicalLogPairSameDirectory") -isnot [bool] -or
				-not [bool] (Get-ObjectPropertyValue $campaignRun "canonicalLogPairSameDirectory") -or
				$campaignHardDiagnosticCount -le 0 -or
				$campaignHardDiagnosticCount -ne ($campaignScriptErrorCount + $campaignEngineErrorCount) -or
				$campaignHardDiagnosticCount -ne
					($campaignApprovedStockCount + $campaignApprovedIntentionalCount + $campaignUnapprovedCount) -or
				$campaignPartisanErrorCount -lt 0 -or
				$campaignPartisanErrorCount -gt $campaignScriptErrorCount -or
				$campaignApprovedStockCount -lt 0 -or
				$campaignApprovedIntentionalCount -lt 0 -or
				$campaignUnapprovedCount -le 0 -or
				$null -eq (Get-ObjectPropertyValue $campaignRun "crashMarkers") -or
				[int] (Get-ObjectPropertyValue $campaignRun "crashMarkers") -ne 0 -or
				$null -eq (Get-ObjectPropertyValue $campaignRun "partisanSeverityLineCount") -or
				[int] (Get-ObjectPropertyValue $campaignRun "partisanSeverityLineCount") -ne 0 -or
				(Get-ObjectPropertyValue $campaignRun "hardDiagnosticFree") -isnot [bool] -or
				[bool] (Get-ObjectPropertyValue $campaignRun "hardDiagnosticFree") -or
				(Get-ObjectPropertyValue $campaignRun "diagnosticClassificationValid") -isnot [bool] -or
				[bool] (Get-ObjectPropertyValue $campaignRun "diagnosticClassificationValid") -or
				(Get-ObjectPropertyValue $campaignRun "channelArithmeticValid") -isnot [bool] -or
				-not [bool] (Get-ObjectPropertyValue $campaignRun "channelArithmeticValid") -or
				(Get-ObjectPropertyValue $campaignRun "categoryArithmeticValid") -isnot [bool] -or
				-not [bool] (Get-ObjectPropertyValue $campaignRun "categoryArithmeticValid") -or
				$null -eq $campaignWrapperErrorCensus -or
				$null -eq (Get-ObjectPropertyValue $campaignWrapperErrorCensus "scriptErrors") -or
				[int] (Get-ObjectPropertyValue $campaignWrapperErrorCensus "scriptErrors") -ne 0 -or
				$null -eq (Get-ObjectPropertyValue $campaignWrapperErrorCensus "partisanErrors") -or
				[int] (Get-ObjectPropertyValue $campaignWrapperErrorCensus "partisanErrors") -ne 0 -or
				$null -eq (Get-ObjectPropertyValue $campaignWrapperErrorCensus "crashMarkers") -or
				[int] (Get-ObjectPropertyValue $campaignWrapperErrorCensus "crashMarkers") -ne 0 -or
				(Get-ObjectPropertyValue $campaignRun "cleanupAndSpillZero") -isnot [bool] -or
				-not [bool] (Get-ObjectPropertyValue $campaignRun "cleanupAndSpillZero") -or
				(Get-ObjectPropertyValue $campaignRun "envelopeFilesRehashed") -isnot [bool] -or
				-not [bool] (Get-ObjectPropertyValue $campaignRun "envelopeFilesRehashed")) {
				throw "A preliminary Campaign Debug run has inconsistent diagnostic, cleanup, or rehash evidence."
			}
		}

		if ([string] (Get-ObjectPropertyValue $fullSummaryResult "status") -cne
				"preliminary-failed-diagnostic-census" -or
			[string] (Get-ObjectPropertyValue $fullSummaryResult "candidateDisposition") -cne
				"active-runtime-candidate" -or
			[string] (Get-ObjectPropertyValue $fullSummaryResult "captureStatus") -cne "completed" -or
			[string] (Get-ObjectPropertyValue $fullSummaryResult "artifactIntegrity") -cne "stable-rehashed" -or
			(Get-ObjectPropertyValue $fullSummaryResult "wrapperReportedSuccess") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullSummaryResult "wrapperReportedSuccess") -or
			[string] (Get-ObjectPropertyValue $fullSummaryResult "diagnosticCensusStatus") -cne
				"corrected-unaccepted" -or
			[string] (Get-ObjectPropertyValue $fullSummaryResult "forceAuthorityCanaryDisposition") -cne
				"preliminary-unaccepted" -or
			[string] (Get-ObjectPropertyValue $fullSummaryResult "fullEvidenceDisposition") -cne
				"preliminary-unaccepted" -or
			[string] (Get-ObjectPropertyValue $fullSummaryResult "fullReportCertificationStatus") -cne
				"failed" -or
			(Get-ObjectPropertyValue $fullSummaryResult "fullCertificationPassed") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $fullSummaryResult "fullCertificationPassed") -or
			(Get-ObjectPropertyValue $fullSummaryResult "allCandidateBoundariesVerified") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullSummaryResult "allCandidateBoundariesVerified") -or
			(Get-ObjectPropertyValue $fullSummaryResult "allMountsPacked") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullSummaryResult "allMountsPacked") -or
			(Get-ObjectPropertyValue $fullSummaryResult "allArtifactsStable") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullSummaryResult "allArtifactsStable") -or
			(Get-ObjectPropertyValue $fullSummaryResult "allEnvelopeFilesRehashed") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullSummaryResult "allEnvelopeFilesRehashed") -or
			[int] (Get-ObjectPropertyValue $fullSummaryResult "envelopeFileCount") -ne $summaryEnvelopeFileCount -or
			(Get-ObjectPropertyValue $fullSummaryResult "allDiagnosticClassificationsValid") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $fullSummaryResult "allDiagnosticClassificationsValid") -or
			(Get-ObjectPropertyValue $fullSummaryResult "allHardDiagnosticsFree") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $fullSummaryResult "allHardDiagnosticsFree") -or
			(Get-ObjectPropertyValue $fullSummaryResult "allCleanupAndSpillZero") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullSummaryResult "allCleanupAndSpillZero") -or
			$summaryEnvelopeFileCount -le 0 -or
			(Get-ObjectPropertyValue $fullCampaignDebug "wrapperReportedSuccess") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCampaignDebug "wrapperReportedSuccess") -or
			(Get-ObjectPropertyValue $fullCampaignDebug "artifactSchemaValidationValid") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCampaignDebug "artifactSchemaValidationValid") -or
			[string] (Get-ObjectPropertyValue $fullCampaignDebug "acceptanceDisposition") -cne
				"preliminary-unaccepted" -or
			[string] (Get-ObjectPropertyValue $fullCampaignDebug "diagnosticCensusStatus") -cne
				"corrected-unaccepted" -or
			(Get-ObjectPropertyValue $fullCampaignDebug "candidateBoundaryVerified") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCampaignDebug "candidateBoundaryVerified") -or
			(Get-ObjectPropertyValue $fullCampaignDebug "mountPacked") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCampaignDebug "mountPacked") -or
			(Get-ObjectPropertyValue $fullCampaignDebug "artifactsStable") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCampaignDebug "artifactsStable") -or
			(Get-ObjectPropertyValue $fullCampaignDebug "diagnosticClassificationValid") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $fullCampaignDebug "diagnosticClassificationValid") -or
			(Get-ObjectPropertyValue $fullCampaignDebug "hardDiagnosticFree") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $fullCampaignDebug "hardDiagnosticFree") -or
			[int] (Get-ObjectPropertyValue $fullCampaignDebug "hardDiagnosticCount") -ne 25 -or
			[int] (Get-ObjectPropertyValue $fullCampaignDebug "scriptErrors") -ne 25 -or
			[int] (Get-ObjectPropertyValue $fullCampaignDebug "partisanErrors") -ne 19 -or
			[int] (Get-ObjectPropertyValue $fullCampaignDebug "approvedStockDiagnosticCount") -ne 2 -or
			[int] (Get-ObjectPropertyValue $fullCampaignDebug "approvedIntentionalDiagnosticCount") -ne 13 -or
			[int] (Get-ObjectPropertyValue $fullCampaignDebug "unapprovedHardDiagnosticCount") -ne 10 -or
			[int] (Get-ObjectPropertyValue $fullCampaignDebug "canaryHardDiagnosticCount") -ne 3 -or
			[int] (Get-ObjectPropertyValue $fullCampaignDebug "canaryUnapprovedHardDiagnosticCount") -ne 1 -or
			(Get-ObjectPropertyValue $fullCampaignDebug "cleanupAndSpillZero") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCampaignDebug "cleanupAndSpillZero")) {
			throw "The Full Campaign Debug aggregate does not represent the corrected preliminary diagnostic capture."
		}
	}
	elseif ($fullCampaignDebugStatus -cne "historical-failed") {
		throw "Historical Full Campaign Debug status is unsupported."
	}

	Push-Location $root
	try {
		& git merge-base --is-ancestor $auditedRevision $candidateSourceHead
		if ($LASTEXITCODE -ne 0) {
			throw "The audited revision $auditedRevision is not an ancestor of candidate source HEAD $candidateSourceHead."
		}

		& git merge-base --is-ancestor $candidateSourceHead $checkoutHead
		if ($LASTEXITCODE -ne 0) {
			throw "Candidate source HEAD $candidateSourceHead is not an ancestor of checkout HEAD $checkoutHead."
		}

		if ($null -ne $packagedFocused) {
			& git merge-base --is-ancestor $historicalCandidateSourceHead $packagedFocusedHarnessHead
			if ($LASTEXITCODE -ne 0) {
				throw "Historical candidate source HEAD $historicalCandidateSourceHead is not an ancestor of packaged focused harness HEAD $packagedFocusedHarnessHead."
			}
			& git merge-base --is-ancestor $packagedFocusedHarnessHead $checkoutHead
			if ($LASTEXITCODE -ne 0) {
				throw "Packaged focused harness HEAD $packagedFocusedHarnessHead is not an ancestor of checkout HEAD $checkoutHead."
			}
		}

		if ($null -ne $activePackagedFocused) {
			& git merge-base --is-ancestor $candidateSourceHead $activePackagedFocusedHarnessHead
			if ($LASTEXITCODE -ne 0) {
				throw "Active candidate source HEAD $candidateSourceHead is not an ancestor of packaged focused harness HEAD $activePackagedFocusedHarnessHead."
			}
			& git merge-base --is-ancestor $activePackagedFocusedHarnessHead $checkoutHead
			if ($LASTEXITCODE -ne 0) {
				throw "Active packaged focused harness HEAD $activePackagedFocusedHarnessHead is not an ancestor of checkout HEAD $checkoutHead."
			}
		}

		if ($null -ne $correctedCanary) {
			& git merge-base --is-ancestor $correctedCanaryHarnessHead $checkoutHead
			if ($LASTEXITCODE -ne 0) {
				throw "Corrected canary harness HEAD $correctedCanaryHarnessHead is not an ancestor of checkout HEAD $checkoutHead."
			}
		}

		if ($fullCampaignDebugStatus -ceq "historical-preliminary-failed-diagnostic-census") {
			& git merge-base --is-ancestor $fullCampaignHarnessHead $checkoutHead
			if ($LASTEXITCODE -ne 0) {
				throw "Full Campaign Debug harness HEAD $fullCampaignHarnessHead is not an ancestor of checkout HEAD $checkoutHead."
			}
		}
	}
	finally {
		Pop-Location
	}
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
if ($fullCampaignDebugStatus -ceq "preliminary-failed-diagnostic-census") {
	$nativeEngineWorldRung = $proofRungById["native-engine-world"]
	$stableCertificationRung = $proofRungById["stable-certification"]
	if ($releaseDecision -cne "NO-GO" -or
		$null -eq $nativeEngineWorldRung -or
		[string] (Get-ObjectPropertyValue $nativeEngineWorldRung "status") -cne "failed" -or
		$null -eq $stableCertificationRung -or
		[string] (Get-ObjectPropertyValue $stableCertificationRung "status") -cne "blocked") {
		throw "A current preliminary Full Campaign Debug result requires NO-GO, a failed native-engine-world rung, and blocked stable certification."
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
	Add-Line $statusBuilder "- Active packaged focused autotests: **not run** for replacement candidate $mdTick$candidateId${mdTick}; no prior-package result transfers to this package."
}
else {
	Add-Line $statusBuilder "- Active packaged focused autotests: **$($activePackagedFocused.passedCases)/$($activePackagedFocused.caseCount)** cases and JUnit **$($activePackagedFocused.junitTests)/$($activePackagedFocused.junitFailures)/$($activePackagedFocused.junitErrors)/$($activePackagedFocused.junitSkipped)** tests/failures/errors/skips against exact candidate $mdTick$candidateId${mdTick}. Hard diagnostics are explicitly not free: $($activePackagedFocused.hardDiagnosticCount) total = $($activePackagedFocused.approvedStockFilterDiagnosticCount) approved stock + $($activePackagedFocused.approvedIntentionalFaultDiagnosticCount) approved intentional + $($activePackagedFocused.unapprovedHardDiagnosticCount) unapproved, with $($activePackagedFocused.envelopeFileCount) envelope files rehashed and zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $activePackagedFocusedSummaryPath)$mdTick / SHA-256 $mdTick$activePackagedFocusedSummarySha$mdTick; clean harness $mdTick$activePackagedFocusedHarnessHead${mdTick}. This exact-package deterministic-service result is non-certifying and does not color the native-engine-world rung."
}
if ($null -eq $activeCorrectedCanary -and $null -eq $activeFullCampaignDebug) {
	Add-Line $statusBuilder "- Active Campaign Debug: **not run** for replacement candidate $mdTick$candidateId${mdTick}; the corrected canary follows only after the packaged focused set is accepted."
}
if ($null -ne $packagedFocused) {
	Add-Line $statusBuilder "- Historical packaged focused autotests: **$($packagedFocused.passedCases)/$($packagedFocused.caseCount)** cases and JUnit **$($packagedFocused.junitTests)/$($packagedFocused.junitFailures)/$($packagedFocused.junitErrors)/$($packagedFocused.junitSkipped)** tests/failures/errors/skips against prior exact candidate $mdTick$historicalCandidateId$mdTick. Hard diagnostics are explicitly not free: $($packagedFocused.hardDiagnosticCount) total = $($packagedFocused.approvedStockFilterDiagnosticCount) approved stock + $($packagedFocused.approvedIntentionalFaultDiagnosticCount) approved intentional + $($packagedFocused.unapprovedHardDiagnosticCount) unapproved, with $($packagedFocused.envelopeFileCount) envelope files rehashed and zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $packagedFocusedSummaryPath)$mdTick / SHA-256 $mdTick$packagedFocusedSummarySha$mdTick; harness $mdTick$packagedFocusedHarnessHead$mdTick. This immutable non-certifying result does not attach to the active replacement."
}
Add-Line $statusBuilder "- Focused force-authority profile: **$($status.evidence.focusedForceAuthority.passedCases)/$($status.evidence.focusedForceAuthority.caseCount)** cases and **$($status.evidence.focusedForceAuthority.passedConditions)/$($status.evidence.focusedForceAuthority.countedConditions)** counted conditions for $mdTick$($status.evidence.focusedForceAuthority.sourceSha)$mdTick, with ${mdTick}CertificationPassed:$($status.evidence.focusedForceAuthority.certificationPassed.ToString().ToLowerInvariant())${mdTick}. This is historical state-only, non-package, non-certifying evidence."
if ($null -ne $correctedCanary) {
	Add-Line $statusBuilder "- Historical corrected force-authority canary: **rejected fail-closed** on prior exact candidate $mdTick$historicalCandidateId${mdTick}. The focused proof remained $($correctedCanary.focusedAssertionsPassed)/$($correctedCanary.focusedAssertionCount) assertions and $($correctedCanary.certificationProven)/$($correctedCanary.certificationRequired) counted certification conditions, but the 33-check classifier found $($correctedCanary.hardDiagnosticCount) hard diagnostics = $($correctedCanary.approvedStockDiagnosticCount) approved stock + $($correctedCanary.approvedIntentionalDiagnosticCount) approved intentional + $($correctedCanary.unapprovedHardDiagnosticCount) unapproved $mdTick$($correctedCanary.unapprovedHardDiagnosticKind)${mdTick}. All $($correctedCanary.envelopeFileCount) envelope files rehashed with zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $correctedCanarySummaryPath)$mdTick / SHA-256 $mdTick$correctedCanarySummarySha$mdTick; harness $mdTick$correctedCanaryHarnessHead$mdTick. This rejection does not color the active replacement's not-run rung."
}
if ($fullCampaignDebugStatus -ceq "historical-preliminary-failed-diagnostic-census") {
	Add-Line $statusBuilder "- Historical Full Campaign Debug capture: **preliminary and unaccepted** on prior exact candidate $mdTick$historicalCandidateId${mdTick}: $($fullCampaignDebug.pass) PASS, $($fullCampaignDebug.warn) WARN, $($fullCampaignDebug.fail) FAIL, $($fullCampaignDebug.blocked) BLOCKED, and $($fullCampaignDebug.skipped) SKIPPED; $($fullCampaignDebug.provenAssertions)/$($fullCampaignDebug.requiredAssertions) required assertions proven, with $($fullCampaignDebug.failedAssertions) failed and $($fullCampaignDebug.blockedAssertions) blocked. Candidate identity, packed mount, artifact stability, cleanup, and envelope rehash were mechanically verified, but the original wrapper missed timestamp-prefixed errors. Its corrected overlay found 3 canary diagnostics with 1 unapproved and 25 full-run diagnostics with 10 unapproved; wrapper-reported success is not acceptance. Summary: $mdTick$(Escape-MarkdownCell $fullCampaignSummaryPath)$mdTick / SHA-256 $mdTick$fullCampaignSummarySha$mdTick; capture harness $mdTick$fullCampaignHarnessHead$mdTick. This result does not attach to the active replacement."
}
else {
	Add-Line $statusBuilder "- Full Campaign Debug: **historical and failed** on $mdTick$($fullCampaignDebug.sourceSha)${mdTick}: $($fullCampaignDebug.pass) PASS, $($fullCampaignDebug.warn) WARN, $($fullCampaignDebug.fail) FAIL, $($fullCampaignDebug.blocked) BLOCKED, and $($fullCampaignDebug.skipped) SKIPPED; $($fullCampaignDebug.provenAssertions)/$($fullCampaignDebug.requiredAssertions) required assertions proven. It predates the audited revision and must be rerun before its individual failures are treated as current."
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
	elseif ($null -eq $activePackagedFocused) {
		Add-Line $statusBuilder "Run the individually named packaged focused service suites next against active replacement $mdTick$(Escape-MarkdownCell $candidateId)$mdTick, manifest $mdTick$(Escape-MarkdownCell $candidateManifestPath)$mdTick, and aggregate package SHA-256 $mdTick$packageSha${mdTick}. If that exact-package set is accepted, run the corrected force-authority canary next; do not transfer the historical candidate's pass or rejection into either gate."
	}
	elseif ($null -eq $activeCorrectedCanary) {
		Add-Line $statusBuilder "The active replacement's packaged focused set is retained. Run the corrected force-authority canary next against the unchanged package identified by manifest $mdTick$(Escape-MarkdownCell $candidateManifestPath)$mdTick and aggregate SHA-256 $mdTick$packageSha${mdTick}."
	}
	elseif ($null -eq $activeFullCampaignDebug) {
		Add-Line $statusBuilder "The active replacement's packaged focused set and corrected canary are retained. Run Full Campaign Debug next against the same immutable package identity."
	}
	else {
		Add-Line $statusBuilder "Continue release-rung closure using only evidence bound to active replacement $mdTick$(Escape-MarkdownCell $candidateId)${mdTick}."
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
