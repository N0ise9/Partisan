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
$packagedFocused = $null
$packagedFocusedSummaryPath = ""
$packagedFocusedSummarySha = ""
$packagedFocusedHarnessHead = ""
$fullCampaignDebug = Get-ObjectPropertyValue $status.evidence "fullCampaignDebug"
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

	$packagedFocused = Get-ObjectPropertyValue $status.evidence "packagedFocusedAutotests"
	$deterministicServiceRungs = @($status.proofRungs | Where-Object {
		[string] (Get-ObjectPropertyValue $_ "id") -ceq "deterministic-service"
	})
	if ($deterministicServiceRungs.Count -ne 1) {
		throw "Release status must contain exactly one deterministic-service proof rung."
	}
	$deterministicServiceRungStatus = [string] (Get-ObjectPropertyValue `
		$deterministicServiceRungs[0] `
		"status")
	if ($null -eq $packagedFocused) {
		if ($deterministicServiceRungStatus -cin @("passed", "passed-noncertifying")) {
			throw "A passed deterministic-service rung requires packaged focused-autotest evidence."
		}
	}
	else {
	if ($deterministicServiceRungStatus -cne "passed-noncertifying") {
		throw "Packaged focused-autotest evidence requires a passed-noncertifying deterministic-service rung."
	}
	if ([string] (Get-ObjectPropertyValue $packagedFocused "status") -cne "passed-noncertifying") {
		throw "Packaged focused-autotest evidence must be passed-noncertifying."
	}

	$packagedFocusedSummaryPath = Require-RepoRelativePath `
		(Get-ObjectPropertyValue $packagedFocused "summaryPath") `
		"release_status.evidence.packagedFocusedAutotests.summaryPath"
	$packagedFocusedSummarySha = Require-Sha256 `
		(Get-ObjectPropertyValue $packagedFocused "summarySha256") `
		"release_status.evidence.packagedFocusedAutotests.summarySha256"
	$packagedFocusedHarnessHead = Require-Text `
		(Get-ObjectPropertyValue $packagedFocused "harnessGitHead") `
		"release_status.evidence.packagedFocusedAutotests.harnessGitHead"
	if ($packagedFocusedHarnessHead -cnotmatch '^[0-9a-f]{40}$') {
		throw "The packaged focused harness HEAD must be a lowercase full Git SHA."
	}

	$packagedFocusedSummaryFullPath = [IO.Path]::GetFullPath(
		(Join-Path $root $packagedFocusedSummaryPath))
	if (-not $packagedFocusedSummaryFullPath.StartsWith(
		$repositoryPrefix,
		[StringComparison]::OrdinalIgnoreCase) -or
		-not (Test-Path -LiteralPath $packagedFocusedSummaryFullPath -PathType Leaf)) {
		throw "The tracked packaged focused summary is missing or outside the repository."
	}
	$packagedFocusedSummaryItem = Get-Item -LiteralPath $packagedFocusedSummaryFullPath -Force
	if (($packagedFocusedSummaryItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
		throw "The tracked packaged focused summary must not be a reparse point."
	}
	$actualPackagedFocusedSummarySha = (Get-FileHash `
		-LiteralPath $packagedFocusedSummaryFullPath `
		-Algorithm SHA256).Hash.ToLowerInvariant()
	if ($actualPackagedFocusedSummarySha -cne $packagedFocusedSummarySha.ToLowerInvariant()) {
		throw "The packaged focused summary SHA-256 does not match release status."
	}

	$packagedFocusedSummaryText = Get-Content -Raw -LiteralPath $packagedFocusedSummaryFullPath
	if ($packagedFocusedSummaryText -match '(?i)[A-Z]:[\\/]') {
		throw "The tracked packaged focused summary contains a local absolute path."
	}
	$packagedFocusedSummary = $packagedFocusedSummaryText | ConvertFrom-Json
	$focusedSummaryCandidate = Get-ObjectPropertyValue $packagedFocusedSummary "candidate"
	$focusedSummaryHarness = Get-ObjectPropertyValue $packagedFocusedSummary "harness"
	$focusedSummaryResult = Get-ObjectPropertyValue $packagedFocusedSummary "result"
	$focusedSummaryCases = @(Get-ObjectPropertyValue $packagedFocusedSummary "cases")
	$focusedSummaryPreliminary = Get-ObjectPropertyValue $packagedFocusedSummary "preliminaryRuns"
	if ([int] (Get-ObjectPropertyValue $packagedFocusedSummary "schemaVersion") -ne 1 -or
		[string] (Get-ObjectPropertyValue $packagedFocusedSummary "evidenceKind") -cne "packaged-focused-autotest-set" -or
		$null -eq $focusedSummaryCandidate -or
		$null -eq $focusedSummaryHarness -or
		$null -eq $focusedSummaryResult -or
		$focusedSummaryCases.Count -ne 5 -or
		$null -eq $focusedSummaryPreliminary) {
		throw "The tracked packaged focused summary is structurally incomplete."
	}

	if ([string] (Get-ObjectPropertyValue $packagedFocused "candidateId") -cne $candidateId -or
		[string] (Get-ObjectPropertyValue $packagedFocused "candidateSourceHead") -cne $candidateSourceHead -or
		[string] (Get-ObjectPropertyValue $packagedFocused "packageSha256") -cne $packageSha -or
		[string] (Get-ObjectPropertyValue $packagedFocused "manifestSha256") -cne $candidateManifestSha -or
		[string] (Get-ObjectPropertyValue $packagedFocused "readySha256") -cne $candidateReadySha -or
		[string] (Get-ObjectPropertyValue $focusedSummaryCandidate "candidateId") -cne $candidateId -or
		[string] (Get-ObjectPropertyValue $focusedSummaryCandidate "candidateSourceHead") -cne $candidateSourceHead -or
		[string] (Get-ObjectPropertyValue $focusedSummaryCandidate "packageSha256") -cne $packageSha -or
		[string] (Get-ObjectPropertyValue $focusedSummaryCandidate "manifestSha256") -cne $candidateManifestSha -or
		[string] (Get-ObjectPropertyValue $focusedSummaryCandidate "readySha256") -cne $candidateReadySha -or
		[string] (Get-ObjectPropertyValue $focusedSummaryCandidate "workbenchCrc") -cne $workbenchCrc) {
		throw "Packaged focused evidence differs from the active candidate identity."
	}

	$focusedRunnerSha = Require-Sha256 `
		(Get-ObjectPropertyValue $packagedFocused "focusedRunnerSha256") `
		"release_status.evidence.packagedFocusedAutotests.focusedRunnerSha256"
	$focusedCandidateModuleSha = Require-Sha256 `
		(Get-ObjectPropertyValue $packagedFocused "candidateModuleSha256") `
		"release_status.evidence.packagedFocusedAutotests.candidateModuleSha256"
	if ([string] (Get-ObjectPropertyValue $focusedSummaryHarness "gitHead") -cne $packagedFocusedHarnessHead -or
		(Get-ObjectPropertyValue $focusedSummaryHarness "dirty") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $focusedSummaryHarness "dirty") -or
		[string] (Get-ObjectPropertyValue $focusedSummaryHarness "focusedRunnerSha256") -cne $focusedRunnerSha -or
		[string] (Get-ObjectPropertyValue $focusedSummaryHarness "candidateModuleSha256") -cne $focusedCandidateModuleSha) {
		throw "Packaged focused evidence does not bind one clean exact harness."
	}

	$expectedFocusedSuites = @{
		"HST_TEST_EnemyCounterattackAuthority" = "HST_EnemyCounterattackAutotestSuite"
		"HST_TEST_EnemyGarrisonRebuildAuthority" = "HST_EnemyGarrisonRebuildAutotestSuite"
		"HST_TEST_EnemyPlanningCommitmentAuthority" = "HST_EnemyPlanningCommitmentAutotestSuite"
		"HST_TEST_EnemyQRFAuthority" = "HST_EnemyQRFAutotestSuite"
		"HST_TEST_CampaignProfileJournalAuthority" = "HST_CampaignProfileJournalAuthorityAutotestSuite"
	}
	$focusedCaseNames = @($focusedSummaryCases | ForEach-Object {
		[string] (Get-ObjectPropertyValue $_ "testCase")
	})
	Assert-UniqueStrings $focusedCaseNames "Packaged focused testcase IDs"
	Assert-EqualSet @($expectedFocusedSuites.Keys) $focusedCaseNames "Packaged focused testcase IDs"

	$focusedEnvelopeHashes = @()
	$focusedRunIds = @()
	$focusedJunitTests = 0
	$focusedJunitFailures = 0
	$focusedJunitErrors = 0
	$focusedJunitSkipped = 0
	$focusedHardDiagnostics = 0
	$focusedStockDiagnostics = 0
	$focusedIntentionalDiagnostics = 0
	$focusedUnapprovedDiagnostics = 0
	$focusedEnvelopeFileCount = 0
	$focusedClassifierChecksPerRun = [int] (Get-ObjectPropertyValue `
		$packagedFocused `
		"hardDiagnosticClassifierChecksPerRun")
	if ($focusedClassifierChecksPerRun -le 0) {
		throw "Packaged focused evidence must record a positive classifier self-check count."
	}
	foreach ($focusedCase in $focusedSummaryCases) {
		$focusedCaseName = Require-Text `
			(Get-ObjectPropertyValue $focusedCase "testCase") `
			"packaged focused testcase ID"
		$focusedRunId = Require-Text `
			(Get-ObjectPropertyValue $focusedCase "runId") `
			"packaged focused run ID"
		$focusedEnvelopeSha = Require-Sha256 `
			(Get-ObjectPropertyValue $focusedCase "envelopeSha256") `
			"packaged focused envelope SHA-256"
		$focusedCaseHardDiagnostics = [int] (Get-ObjectPropertyValue $focusedCase "hardDiagnosticCount")
		$focusedCaseStockDiagnostics = [int] (Get-ObjectPropertyValue $focusedCase "approvedStockFilterDiagnosticCount")
		$focusedCaseIntentionalDiagnostics = [int] (Get-ObjectPropertyValue $focusedCase "approvedIntentionalFaultDiagnosticCount")
		$focusedCaseUnapprovedDiagnostics = [int] (Get-ObjectPropertyValue $focusedCase "unapprovedHardDiagnosticCount")
		$focusedCaseHardDiagnosticFree = Get-ObjectPropertyValue $focusedCase "hardDiagnosticFree"
		if ([string] (Get-ObjectPropertyValue $focusedCase "suiteClass") -cne
			[string] $expectedFocusedSuites[$focusedCaseName] -or
			$focusedRunId -cnotmatch '^\d{8}T\d{6}Z-[0-9a-f]{32}$' -or
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
				$focusedClassifierChecksPerRun -or
			(Get-ObjectPropertyValue $focusedCase "hardDiagnosticClassificationValid") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $focusedCase "hardDiagnosticClassificationValid") -or
			$focusedCaseHardDiagnosticFree -isnot [bool] -or
			[bool] $focusedCaseHardDiagnosticFree -ne ($focusedCaseHardDiagnostics -eq 0) -or
			$focusedCaseHardDiagnostics -lt 0 -or
			$focusedCaseStockDiagnostics -lt 0 -or
			$focusedCaseIntentionalDiagnostics -lt 0 -or
			$focusedCaseUnapprovedDiagnostics -ne 0 -or
			$focusedCaseHardDiagnostics -ne
				($focusedCaseStockDiagnostics + $focusedCaseIntentionalDiagnostics +
					$focusedCaseUnapprovedDiagnostics) -or
			(Get-ObjectPropertyValue $focusedCase "cleanupAndSpillZero") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $focusedCase "cleanupAndSpillZero") -or
			(Get-ObjectPropertyValue $focusedCase "envelopeFilesRehashed") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $focusedCase "envelopeFilesRehashed")) {
			throw "Packaged focused testcase $focusedCaseName does not satisfy the accepted result contract."
		}
		Require-Text (Get-ObjectPropertyValue $focusedCase "startedUtc") "packaged focused started UTC" | Out-Null
		Require-Text (Get-ObjectPropertyValue $focusedCase "completedUtc") "packaged focused completed UTC" | Out-Null
		$focusedRunIds += $focusedRunId
		$focusedEnvelopeHashes += $focusedEnvelopeSha
		$focusedJunitTests += [int] (Get-ObjectPropertyValue $focusedCase "junitTests")
		$focusedJunitFailures += [int] (Get-ObjectPropertyValue $focusedCase "junitFailures")
		$focusedJunitErrors += [int] (Get-ObjectPropertyValue $focusedCase "junitErrors")
		$focusedJunitSkipped += [int] (Get-ObjectPropertyValue $focusedCase "junitSkipped")
		$focusedHardDiagnostics += $focusedCaseHardDiagnostics
		$focusedStockDiagnostics += $focusedCaseStockDiagnostics
		$focusedIntentionalDiagnostics += $focusedCaseIntentionalDiagnostics
		$focusedUnapprovedDiagnostics += $focusedCaseUnapprovedDiagnostics
		$focusedEnvelopeFileCount += [int] (Get-ObjectPropertyValue $focusedCase "fileCount")
	}
	Assert-UniqueStrings $focusedRunIds "Packaged focused run IDs"
	Assert-UniqueStrings $focusedEnvelopeHashes "Packaged focused envelope hashes"

	if ([string] (Get-ObjectPropertyValue $focusedSummaryResult "status") -cne "passed-noncertifying" -or
		[int] (Get-ObjectPropertyValue $focusedSummaryResult "caseCount") -ne 5 -or
		[int] (Get-ObjectPropertyValue $focusedSummaryResult "passedCases") -ne 5 -or
		[int] (Get-ObjectPropertyValue $focusedSummaryResult "junitTests") -ne $focusedJunitTests -or
		[int] (Get-ObjectPropertyValue $focusedSummaryResult "junitFailures") -ne $focusedJunitFailures -or
		[int] (Get-ObjectPropertyValue $focusedSummaryResult "junitErrors") -ne $focusedJunitErrors -or
		[int] (Get-ObjectPropertyValue $focusedSummaryResult "junitSkipped") -ne $focusedJunitSkipped -or
		[int] (Get-ObjectPropertyValue $focusedSummaryResult "hardDiagnosticClassifierChecksPerRun") -ne
			$focusedClassifierChecksPerRun -or
		(Get-ObjectPropertyValue $focusedSummaryResult "hardDiagnosticClassificationValid") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $focusedSummaryResult "hardDiagnosticClassificationValid") -or
		(Get-ObjectPropertyValue $focusedSummaryResult "hardDiagnosticFree") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $focusedSummaryResult "hardDiagnosticFree") -ne
			($focusedHardDiagnostics -eq 0) -or
		[int] (Get-ObjectPropertyValue $focusedSummaryResult "hardDiagnosticCount") -ne $focusedHardDiagnostics -or
		[int] (Get-ObjectPropertyValue $focusedSummaryResult "approvedStockFilterDiagnosticCount") -ne $focusedStockDiagnostics -or
		[int] (Get-ObjectPropertyValue $focusedSummaryResult "approvedIntentionalFaultDiagnosticCount") -ne $focusedIntentionalDiagnostics -or
		[int] (Get-ObjectPropertyValue $focusedSummaryResult "unapprovedHardDiagnosticCount") -ne $focusedUnapprovedDiagnostics -or
		[int] (Get-ObjectPropertyValue $focusedSummaryResult "envelopeFileCount") -ne $focusedEnvelopeFileCount -or
		(Get-ObjectPropertyValue $focusedSummaryResult "candidateBoundaryVerified") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $focusedSummaryResult "candidateBoundaryVerified") -or
		(Get-ObjectPropertyValue $focusedSummaryResult "allMountsPacked") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $focusedSummaryResult "allMountsPacked") -or
		(Get-ObjectPropertyValue $focusedSummaryResult "allCleanupAndSpillZero") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $focusedSummaryResult "allCleanupAndSpillZero") -or
		(Get-ObjectPropertyValue $focusedSummaryResult "allEnvelopeFilesRehashed") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $focusedSummaryResult "allEnvelopeFilesRehashed") -or
		[string] (Get-ObjectPropertyValue $focusedSummaryPreliminary "status") -cne "superseded-for-acceptance") {
		throw "The packaged focused aggregate does not equal its five accepted runs."
	}

	if ([int] (Get-ObjectPropertyValue $packagedFocused "caseCount") -ne 5 -or
		[int] (Get-ObjectPropertyValue $packagedFocused "passedCases") -ne 5 -or
		[int] (Get-ObjectPropertyValue $packagedFocused "junitTests") -ne $focusedJunitTests -or
		[int] (Get-ObjectPropertyValue $packagedFocused "junitFailures") -ne $focusedJunitFailures -or
		[int] (Get-ObjectPropertyValue $packagedFocused "junitErrors") -ne $focusedJunitErrors -or
		[int] (Get-ObjectPropertyValue $packagedFocused "junitSkipped") -ne $focusedJunitSkipped -or
		[int] (Get-ObjectPropertyValue $packagedFocused "hardDiagnosticClassifierChecksPerRun") -ne
			$focusedClassifierChecksPerRun -or
		(Get-ObjectPropertyValue $packagedFocused "hardDiagnosticFree") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $packagedFocused "hardDiagnosticFree") -ne
			($focusedHardDiagnostics -eq 0) -or
		[int] (Get-ObjectPropertyValue $packagedFocused "hardDiagnosticCount") -ne $focusedHardDiagnostics -or
		[int] (Get-ObjectPropertyValue $packagedFocused "approvedStockFilterDiagnosticCount") -ne $focusedStockDiagnostics -or
		[int] (Get-ObjectPropertyValue $packagedFocused "approvedIntentionalFaultDiagnosticCount") -ne $focusedIntentionalDiagnostics -or
		[int] (Get-ObjectPropertyValue $packagedFocused "unapprovedHardDiagnosticCount") -ne $focusedUnapprovedDiagnostics -or
		[int] (Get-ObjectPropertyValue $packagedFocused "envelopeFileCount") -ne $focusedEnvelopeFileCount -or
		(Get-ObjectPropertyValue $packagedFocused "cleanupAndSpillZero") -isnot [bool] -or
		-not [bool] (Get-ObjectPropertyValue $packagedFocused "cleanupAndSpillZero") -or
		$focusedJunitTests -ne 5 -or $focusedJunitFailures -ne 0 -or
		$focusedJunitErrors -ne 0 -or $focusedJunitSkipped -ne 0 -or
		$focusedUnapprovedDiagnostics -ne 0 -or $focusedEnvelopeFileCount -le 0) {
		throw "Release status does not equal the packaged focused summary."
	}
	}

	if ($null -eq $fullCampaignDebug) {
		throw "Release status must contain Full Campaign Debug evidence."
	}
	$fullCampaignDebugStatus = Require-Text `
		(Get-ObjectPropertyValue $fullCampaignDebug "status") `
		"release_status.evidence.fullCampaignDebug.status"
	if ($fullCampaignDebugStatus -ceq "failed") {
		$fullCampaignSummaryPath = Require-RepoRelativePath `
			(Get-ObjectPropertyValue $fullCampaignDebug "summaryPath") `
			"release_status.evidence.fullCampaignDebug.summaryPath"
		$fullCampaignSummarySha = Require-Sha256 `
			(Get-ObjectPropertyValue $fullCampaignDebug "summarySha256") `
			"release_status.evidence.fullCampaignDebug.summarySha256"
		$fullCampaignHarnessHead = Require-Text `
			(Get-ObjectPropertyValue $fullCampaignDebug "harnessGitHead") `
			"release_status.evidence.fullCampaignDebug.harnessGitHead"
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
		$fullSummaryAcceptedWindow = Get-ObjectPropertyValue $fullCampaignSummary "acceptedWindow"
		$fullSummaryResult = Get-ObjectPropertyValue $fullCampaignSummary "result"
		$fullSummaryRuns = @(Get-ObjectPropertyValue $fullCampaignSummary "runs")
		if ([int] (Get-ObjectPropertyValue $fullCampaignSummary "schemaVersion") -ne 1 -or
			[string] (Get-ObjectPropertyValue $fullCampaignSummary "evidenceKind") -cne "packaged-campaign-debug-set" -or
			$null -eq $fullSummaryCandidate -or
			$null -eq $fullSummaryHarness -or
			$null -eq $fullSummarySettings -or
			$null -eq $fullSummaryAcceptedWindow -or
			$null -eq $fullSummaryResult -or
			$fullSummaryRuns.Count -ne 2) {
			throw "The tracked Full Campaign Debug summary is structurally incomplete."
		}

		if ([string] (Get-ObjectPropertyValue $fullCampaignDebug "candidateId") -cne $candidateId -or
			[string] (Get-ObjectPropertyValue $fullCampaignDebug "candidateSourceHead") -cne $candidateSourceHead -or
			[string] (Get-ObjectPropertyValue $fullCampaignDebug "sourceSha") -cne $candidateSourceHead -or
			[string] (Get-ObjectPropertyValue $fullCampaignDebug "packageSha256") -cne $packageSha -or
			[string] (Get-ObjectPropertyValue $fullCampaignDebug "manifestSha256") -cne $candidateManifestSha -or
			[string] (Get-ObjectPropertyValue $fullCampaignDebug "readySha256") -cne $candidateReadySha -or
			[string] (Get-ObjectPropertyValue $fullSummaryCandidate "candidateId") -cne $candidateId -or
			[string] (Get-ObjectPropertyValue $fullSummaryCandidate "candidateSourceHead") -cne $candidateSourceHead -or
			[string] (Get-ObjectPropertyValue $fullSummaryCandidate "packageSha256") -cne $packageSha -or
			[string] (Get-ObjectPropertyValue $fullSummaryCandidate "manifestSha256") -cne $candidateManifestSha -or
			[string] (Get-ObjectPropertyValue $fullSummaryCandidate "readySha256") -cne $candidateReadySha -or
			[string] (Get-ObjectPropertyValue $fullSummaryCandidate "workbenchCrc") -cne $workbenchCrc) {
			throw "Full Campaign Debug evidence differs from the active candidate identity."
		}

		$fullCampaignRunnerSha = Require-Sha256 `
			(Get-ObjectPropertyValue $fullCampaignDebug "campaignRunnerSha256") `
			"release_status.evidence.fullCampaignDebug.campaignRunnerSha256"
		$fullCandidateModuleSha = Require-Sha256 `
			(Get-ObjectPropertyValue $fullCampaignDebug "candidateModuleSha256") `
			"release_status.evidence.fullCampaignDebug.candidateModuleSha256"
		$fullSettingsSha = Require-Sha256 `
			(Get-ObjectPropertyValue $fullCampaignDebug "settingsSha256") `
			"release_status.evidence.fullCampaignDebug.settingsSha256"
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

		$acceptedWindowStarted = Require-UtcTimestamp `
			(Get-ObjectPropertyValue $fullSummaryAcceptedWindow "startedUtc") `
			"Campaign Debug accepted-window start"
		$acceptedWindowCompleted = Require-UtcTimestamp `
			(Get-ObjectPropertyValue $fullSummaryAcceptedWindow "completedUtc") `
			"Campaign Debug accepted-window completion"
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
		if ($forceStarted -ne $acceptedWindowStarted -or
			$fullCompleted -ne $acceptedWindowCompleted -or
			$forceCompleted -lt $forceStarted -or
			$fullStarted -lt $forceCompleted -or
			$fullCompleted -lt $fullStarted) {
			throw "Campaign Debug run chronology does not equal its accepted window or canary-first order."
		}

		$summaryEnvelopeFileCount = [int] (Get-ObjectPropertyValue $forceAuthorityRun "fileCount") +
			[int] (Get-ObjectPropertyValue $fullCertificationRun "fileCount")
		if ([string] (Get-ObjectPropertyValue $forceAuthorityRun "proofScope") -cne "focused_force_authority" -or
			(Get-ObjectPropertyValue $forceAuthorityRun "success") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "success") -or
			(Get-ObjectPropertyValue $forceAuthorityRun "candidateBoundaryVerified") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "candidateBoundaryVerified") -or
			(Get-ObjectPropertyValue $forceAuthorityRun "mountPacked") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "mountPacked") -or
			(Get-ObjectPropertyValue $forceAuthorityRun "artifactsStable") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "artifactsStable") -or
			(Get-ObjectPropertyValue $forceAuthorityRun "validationValid") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "validationValid") -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "fail") -ne 0 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "certificationRequired") -ne
				[int] (Get-ObjectPropertyValue $forceAuthorityRun "certificationProven") -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "certificationFail") -ne 0 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "certificationBlocked") -ne 0 -or
			(Get-ObjectPropertyValue $forceAuthorityRun "certificationPassed") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $forceAuthorityRun "certificationPassed") -or
			[string] (Get-ObjectPropertyValue $forceAuthorityRun "focusedCaseStatus") -cne "PASS" -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "focusedAssertionCount") -le 0 -or
			[int] (Get-ObjectPropertyValue $forceAuthorityRun "nonzeroStateDiffRows") -ne 0 -or
			(Get-ObjectPropertyValue $forceAuthorityRun "finalOrphanCleanupPass") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $forceAuthorityRun "finalOrphanCleanupPass")) {
			throw "The packaged force-authority Campaign Debug canary is not accepted."
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

		if ([string] (Get-ObjectPropertyValue $fullCertificationRun "proofScope") -cne "full_certification" -or
			(Get-ObjectPropertyValue $fullCertificationRun "success") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCertificationRun "success") -or
			(Get-ObjectPropertyValue $fullCertificationRun "candidateBoundaryVerified") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCertificationRun "candidateBoundaryVerified") -or
			(Get-ObjectPropertyValue $fullCertificationRun "mountPacked") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCertificationRun "mountPacked") -or
			(Get-ObjectPropertyValue $fullCertificationRun "artifactsStable") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCertificationRun "artifactsStable") -or
			(Get-ObjectPropertyValue $fullCertificationRun "validationValid") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCertificationRun "validationValid") -or
			(Get-ObjectPropertyValue $fullCertificationRun "certificationPassed") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $fullCertificationRun "certificationPassed") -or
			(Get-ObjectPropertyValue $fullCampaignDebug "certificationPassed") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $fullCampaignDebug "certificationPassed") -or
			[int] (Get-ObjectPropertyValue $fullCertificationRun "nonzeroStateDiffRows") -ne 0 -or
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

		foreach ($campaignRun in $fullSummaryRuns) {
			if ([int] (Get-ObjectPropertyValue $campaignRun "fileCount") -le 0 -or
				[int] (Get-ObjectPropertyValue $campaignRun "scriptErrors") -ne 0 -or
				[int] (Get-ObjectPropertyValue $campaignRun "partisanErrors") -ne 0 -or
				[int] (Get-ObjectPropertyValue $campaignRun "crashMarkers") -ne 0 -or
				(Get-ObjectPropertyValue $campaignRun "cleanupAndSpillZero") -isnot [bool] -or
				-not [bool] (Get-ObjectPropertyValue $campaignRun "cleanupAndSpillZero") -or
				(Get-ObjectPropertyValue $campaignRun "envelopeFilesRehashed") -isnot [bool] -or
				-not [bool] (Get-ObjectPropertyValue $campaignRun "envelopeFilesRehashed")) {
				throw "A Campaign Debug run lacks clean error, cleanup, or rehash evidence."
			}
		}

		if ([string] (Get-ObjectPropertyValue $fullSummaryResult "status") -cne "failed" -or
			(Get-ObjectPropertyValue $fullSummaryResult "forceAuthorityCanaryAccepted") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullSummaryResult "forceAuthorityCanaryAccepted") -or
			(Get-ObjectPropertyValue $fullSummaryResult "fullArtifactsValid") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullSummaryResult "fullArtifactsValid") -or
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
			(Get-ObjectPropertyValue $fullSummaryResult "allErrorCensusesZero") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullSummaryResult "allErrorCensusesZero") -or
			(Get-ObjectPropertyValue $fullSummaryResult "allCleanupAndSpillZero") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullSummaryResult "allCleanupAndSpillZero") -or
			$summaryEnvelopeFileCount -le 0 -or
			(Get-ObjectPropertyValue $fullCampaignDebug "wrapperSuccess") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCampaignDebug "wrapperSuccess") -or
			(Get-ObjectPropertyValue $fullCampaignDebug "validationValid") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCampaignDebug "validationValid") -or
			(Get-ObjectPropertyValue $fullCampaignDebug "candidateBoundaryVerified") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCampaignDebug "candidateBoundaryVerified") -or
			(Get-ObjectPropertyValue $fullCampaignDebug "mountPacked") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCampaignDebug "mountPacked") -or
			(Get-ObjectPropertyValue $fullCampaignDebug "artifactsStable") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCampaignDebug "artifactsStable") -or
			(Get-ObjectPropertyValue $fullCampaignDebug "errorCensusZero") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCampaignDebug "errorCensusZero") -or
			(Get-ObjectPropertyValue $fullCampaignDebug "cleanupAndSpillZero") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $fullCampaignDebug "cleanupAndSpillZero")) {
			throw "The Full Campaign Debug aggregate does not represent a trustworthy failed certification result."
		}
	}
	elseif ($fullCampaignDebugStatus -cne "historical-failed") {
		throw "Full Campaign Debug status must be failed or historical-failed."
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
			& git merge-base --is-ancestor $packagedFocusedHarnessHead $checkoutHead
			if ($LASTEXITCODE -ne 0) {
				throw "Packaged focused harness HEAD $packagedFocusedHarnessHead is not an ancestor of checkout HEAD $checkoutHead."
			}
		}

		if ($fullCampaignDebugStatus -ceq "failed") {
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
if ($fullCampaignDebugStatus -ceq "failed") {
	$nativeEngineWorldRung = $proofRungById["native-engine-world"]
	$stableCertificationRung = $proofRungById["stable-certification"]
	if ($releaseDecision -cne "NO-GO" -or
		$null -eq $nativeEngineWorldRung -or
		[string] (Get-ObjectPropertyValue $nativeEngineWorldRung "status") -cne "failed" -or
		$null -eq $stableCertificationRung -or
		[string] (Get-ObjectPropertyValue $stableCertificationRung "status") -cne "blocked") {
		throw "A current failed Full Campaign Debug result requires NO-GO, a failed native-engine-world rung, and blocked stable certification."
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
if ($null -ne $packagedFocused) {
	Add-Line $statusBuilder "- Packaged focused autotests: **$($packagedFocused.passedCases)/$($packagedFocused.caseCount)** cases and JUnit **$($packagedFocused.junitTests)/$($packagedFocused.junitFailures)/$($packagedFocused.junitErrors)/$($packagedFocused.junitSkipped)** tests/failures/errors/skips against exact candidate $mdTick$candidateId$mdTick. Hard diagnostics are explicitly not free: $($packagedFocused.hardDiagnosticCount) total = $($packagedFocused.approvedStockFilterDiagnosticCount) approved stock + $($packagedFocused.approvedIntentionalFaultDiagnosticCount) approved intentional + $($packagedFocused.unapprovedHardDiagnosticCount) unapproved, with $($packagedFocused.envelopeFileCount) envelope files rehashed and zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $packagedFocusedSummaryPath)$mdTick / SHA-256 $mdTick$packagedFocusedSummarySha$mdTick; harness $mdTick$packagedFocusedHarnessHead$mdTick. This is passed non-certifying service evidence."
}
Add-Line $statusBuilder "- Focused force-authority profile: **$($status.evidence.focusedForceAuthority.passedCases)/$($status.evidence.focusedForceAuthority.caseCount)** cases and **$($status.evidence.focusedForceAuthority.passedConditions)/$($status.evidence.focusedForceAuthority.countedConditions)** counted conditions for $mdTick$($status.evidence.focusedForceAuthority.sourceSha)$mdTick, with ${mdTick}CertificationPassed:$($status.evidence.focusedForceAuthority.certificationPassed.ToString().ToLowerInvariant())${mdTick}. This is historical state-only, non-package, non-certifying evidence."
if ($fullCampaignDebugStatus -ceq "failed") {
	Add-Line $statusBuilder "- Full Campaign Debug: **current package-bound run failed certification** on exact candidate $mdTick$candidateId${mdTick}: $($fullCampaignDebug.pass) PASS, $($fullCampaignDebug.warn) WARN, $($fullCampaignDebug.fail) FAIL, $($fullCampaignDebug.blocked) BLOCKED, and $($fullCampaignDebug.skipped) SKIPPED; $($fullCampaignDebug.provenAssertions)/$($fullCampaignDebug.requiredAssertions) required assertions proven, with $($fullCampaignDebug.failedAssertions) failed and $($fullCampaignDebug.blockedAssertions) blocked. The wrapper, candidate boundary, packed mount, artifact stability, error census, cleanup, and envelope rehash checks passed, so this is trustworthy red evidence rather than a release pass. Summary: $mdTick$(Escape-MarkdownCell $fullCampaignSummaryPath)$mdTick / SHA-256 $mdTick$fullCampaignSummarySha$mdTick; harness $mdTick$fullCampaignHarnessHead$mdTick."
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
	elseif ($fullCampaignDebugStatus -ceq "failed") {
		Add-Line $statusBuilder "Gate 1 retained exact candidate $mdTick$(Escape-MarkdownCell $candidateId)$mdTick through a trustworthy but red Full Campaign Debug result. Classify and reproduce the smallest concrete root among its $($fullCampaignDebug.fail) failed and $($fullCampaignDebug.blocked) blocked cases before changing source. Any correction must be validated and sealed as a new candidate; it cannot extend this immutable package's evidence chain."
	}
	elseif ($null -ne $packagedFocused) {
		Add-Line $statusBuilder "Gate 1 retained candidate $mdTick$(Escape-MarkdownCell $candidateId)$mdTick, and its five-case packaged focused service rung is accepted. Run current Full Campaign Debug next against the unchanged package identified by manifest $mdTick$(Escape-MarkdownCell $candidateManifestPath)$mdTick and aggregate SHA-256 $mdTick$packageSha$mdTick. Treat a valid-but-red integrated report as retained diagnostic evidence, not a pass; rebuilding creates a different candidate rather than extending this evidence chain."
	}
	else {
		Add-Line $statusBuilder "Gate 1 retained candidate $mdTick$(Escape-MarkdownCell $candidateId)$mdTick. Run the individually named packaged focused service suites next against the package identified by manifest $mdTick$(Escape-MarkdownCell $candidateManifestPath)$mdTick and aggregate SHA-256 $mdTick$packageSha$mdTick; rebuilding creates a different candidate rather than extending this evidence chain."
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
