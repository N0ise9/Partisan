[CmdletBinding()]
param(
	[switch] $Check,
	[switch] $SelfTest,
	[switch] $FocusedConsumerSelfTest,
	[switch] $LibraryOnly,
	[string] $EvidenceBundleRoot = $env:PARTISAN_RELEASE_EVIDENCE_ROOT
)

$ErrorActionPreference = "Stop"
$script:FocusedDiagnosticLocalPathToken = '<local-path>'
$script:FocusedRequiredPatternMarker =
	'PARTISAN_REQUIRED_LOG_PATTERN_B64 '
$script:FocusedMountProjectSuffix =
	'candidate-addons/Partisan/addon.gproj'

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

function Get-ByteArraySha256 {
	param([byte[]] $Bytes)

	$sha = [Security.Cryptography.SHA256]::Create()
	try {
		return ([BitConverter]::ToString(
			$sha.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant()
	}
	finally {
		$sha.Dispose()
	}
}

function Get-StrictUtf8Text {
	param(
		[byte[]] $Bytes,
		[string] $Label
	)

	$encoding = New-Object Text.UTF8Encoding($false, $true)
	try {
		return $encoding.GetString($Bytes)
	}
	catch {
		throw "$Label is not valid UTF-8."
	}
}

function Read-FileByteSnapshot {
	param(
		[string] $Path,
		[string] $Label
	)

	if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
		throw "$Label is missing."
	}
	$fullPath = [IO.Path]::GetFullPath($Path)
	$itemBefore = Get-Item -LiteralPath $fullPath -Force
	if (($itemBefore.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
		throw "$Label must not be a reparse point."
	}
	$bytes = [IO.File]::ReadAllBytes($fullPath)
	$hash = Get-ByteArraySha256 $bytes
	$itemAfter = Get-Item -LiteralPath $fullPath -Force
	$currentHash = (Get-FileHash -LiteralPath $fullPath -Algorithm SHA256).Hash.ToLowerInvariant()
	if (($itemAfter.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
		[long] $itemAfter.Length -ne [long] $bytes.LongLength -or
		$currentHash -cne $hash) {
		throw "$Label changed while its byte snapshot was captured."
	}

	return [PSCustomObject] @{
		FullPath = $fullPath
		Bytes = $bytes
		Length = [long] $bytes.LongLength
		Sha256 = $hash
	}
}

function Assert-FileByteSnapshotUnchanged {
	param(
		[object] $Snapshot,
		[string] $Label
	)

	$fullPath = [string] $Snapshot.FullPath
	if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
		throw "$Label disappeared during validation."
	}
	$item = Get-Item -LiteralPath $fullPath -Force
	$hash = (Get-FileHash -LiteralPath $fullPath -Algorithm SHA256).Hash.ToLowerInvariant()
	if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
		[long] $item.Length -ne [long] $Snapshot.Length -or
		$hash -cne [string] $Snapshot.Sha256) {
		throw "$Label changed during validation."
	}
}

function Skip-JsonWhitespace {
	param(
		[string] $Text,
		$Position
	)

	while ($Position.Value -lt $Text.Length) {
		$code = [int] $Text[$Position.Value]
		if ($code -ne 0x20 -and $code -ne 0x09 -and
			$code -ne 0x0a -and $code -ne 0x0d) {
			break
		}
		$Position.Value++
	}
}

function Read-JsonStringToken {
	param(
		[string] $Text,
		$Position,
		[string] $Label
	)

	if ($Position.Value -ge $Text.Length -or
		[int] $Text[$Position.Value] -ne 0x22) {
		throw "$Label contains invalid JSON: expected a string."
	}
	$Position.Value++
	$builder = New-Object Text.StringBuilder
	while ($Position.Value -lt $Text.Length) {
		$character = $Text[$Position.Value]
		$Position.Value++
		$code = [int] $character
		if ($code -eq 0x22) {
			return $builder.ToString()
		}
		if ($code -lt 0x20) {
			throw "$Label contains an unescaped control character in a JSON string."
		}
		if ($code -ne 0x5c) {
			[void] $builder.Append($character)
			continue
		}
		if ($Position.Value -ge $Text.Length) {
			throw "$Label contains an incomplete JSON string escape."
		}
		$escape = $Text[$Position.Value]
		$Position.Value++
		switch ([int] $escape) {
			0x22 { [void] $builder.Append([char] 0x22) }
			0x5c { [void] $builder.Append([char] 0x5c) }
			0x2f { [void] $builder.Append([char] 0x2f) }
			0x62 { [void] $builder.Append([char] 0x08) }
			0x66 { [void] $builder.Append([char] 0x0c) }
			0x6e { [void] $builder.Append([char] 0x0a) }
			0x72 { [void] $builder.Append([char] 0x0d) }
			0x74 { [void] $builder.Append([char] 0x09) }
			0x75 {
				if ($Position.Value + 4 -gt $Text.Length) {
					throw "$Label contains an incomplete JSON Unicode escape."
				}
				$hex = $Text.Substring($Position.Value, 4)
				if ($hex -cnotmatch '^[0-9a-fA-F]{4}$') {
					throw "$Label contains an invalid JSON Unicode escape."
				}
				[void] $builder.Append([char] [Convert]::ToUInt16($hex, 16))
				$Position.Value += 4
			}
			default {
				throw "$Label contains an unsupported JSON string escape."
			}
		}
	}
	throw "$Label contains an unterminated JSON string."
}

function Read-JsonNumberToken {
	param(
		[string] $Text,
		$Position,
		[string] $Label
	)

	if ($Position.Value -lt $Text.Length -and
		[int] $Text[$Position.Value] -eq 0x2d) {
		$Position.Value++
	}
	if ($Position.Value -ge $Text.Length) {
		throw "$Label contains an incomplete JSON number."
	}
	$first = [int] $Text[$Position.Value]
	if ($first -eq 0x30) {
		$Position.Value++
		if ($Position.Value -lt $Text.Length -and
			[int] $Text[$Position.Value] -ge 0x30 -and
			[int] $Text[$Position.Value] -le 0x39) {
			throw "$Label contains a JSON number with a leading zero."
		}
	}
	elseif ($first -ge 0x31 -and $first -le 0x39) {
		$Position.Value++
		while ($Position.Value -lt $Text.Length -and
			[int] $Text[$Position.Value] -ge 0x30 -and
			[int] $Text[$Position.Value] -le 0x39) {
			$Position.Value++
		}
	}
	else {
		throw "$Label contains an invalid JSON number."
	}
	if ($Position.Value -lt $Text.Length -and
		[int] $Text[$Position.Value] -eq 0x2e) {
		$Position.Value++
		$fractionStart = $Position.Value
		while ($Position.Value -lt $Text.Length -and
			[int] $Text[$Position.Value] -ge 0x30 -and
			[int] $Text[$Position.Value] -le 0x39) {
			$Position.Value++
		}
		if ($Position.Value -eq $fractionStart) {
			throw "$Label contains an incomplete JSON number fraction."
		}
	}
	if ($Position.Value -lt $Text.Length -and
		[int] $Text[$Position.Value] -in @(0x45, 0x65)) {
		$Position.Value++
		if ($Position.Value -lt $Text.Length -and
			[int] $Text[$Position.Value] -in @(0x2b, 0x2d)) {
			$Position.Value++
		}
		$exponentStart = $Position.Value
		while ($Position.Value -lt $Text.Length -and
			[int] $Text[$Position.Value] -ge 0x30 -and
			[int] $Text[$Position.Value] -le 0x39) {
			$Position.Value++
		}
		if ($Position.Value -eq $exponentStart) {
			throw "$Label contains an incomplete JSON number exponent."
		}
	}
}

function Read-JsonValueWithUniqueProperties {
	param(
		[string] $Text,
		$Position,
		[string] $Label
	)

	Skip-JsonWhitespace $Text $Position
	if ($Position.Value -ge $Text.Length) {
		throw "$Label contains incomplete JSON."
	}
	$character = [int] $Text[$Position.Value]
	if ($character -eq 0x7b) {
		Read-JsonObjectWithUniqueProperties $Text $Position $Label
		return
	}
	if ($character -eq 0x5b) {
		Read-JsonArrayWithUniqueProperties $Text $Position $Label
		return
	}
	if ($character -eq 0x22) {
		[void] (Read-JsonStringToken $Text $Position $Label)
		return
	}
	foreach ($literal in @("true", "false", "null")) {
		if ($Position.Value + $literal.Length -le $Text.Length -and
			$Text.Substring($Position.Value, $literal.Length) -ceq $literal) {
			$Position.Value += $literal.Length
			return
		}
	}
	Read-JsonNumberToken $Text $Position $Label
}

function Read-JsonObjectWithUniqueProperties {
	param(
		[string] $Text,
		$Position,
		[string] $Label
	)

	$Position.Value++
	$names = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
	Skip-JsonWhitespace $Text $Position
	if ($Position.Value -lt $Text.Length -and
		[int] $Text[$Position.Value] -eq 0x7d) {
		$Position.Value++
		return
	}
	while ($true) {
		$name = Read-JsonStringToken $Text $Position $Label
		if (-not $names.Add($name)) {
			throw "$Label contains duplicate JSON property '$name'."
		}
		Skip-JsonWhitespace $Text $Position
		if ($Position.Value -ge $Text.Length -or
			[int] $Text[$Position.Value] -ne 0x3a) {
			throw "$Label contains invalid JSON: expected a property separator."
		}
		$Position.Value++
		Read-JsonValueWithUniqueProperties $Text $Position $Label
		Skip-JsonWhitespace $Text $Position
		if ($Position.Value -ge $Text.Length) {
			throw "$Label contains an unterminated JSON object."
		}
		$delimiter = [int] $Text[$Position.Value]
		$Position.Value++
		if ($delimiter -eq 0x7d) {
			return
		}
		if ($delimiter -ne 0x2c) {
			throw "$Label contains invalid JSON: expected an object delimiter."
		}
		Skip-JsonWhitespace $Text $Position
	}
}

function Read-JsonArrayWithUniqueProperties {
	param(
		[string] $Text,
		$Position,
		[string] $Label
	)

	$Position.Value++
	Skip-JsonWhitespace $Text $Position
	if ($Position.Value -lt $Text.Length -and
		[int] $Text[$Position.Value] -eq 0x5d) {
		$Position.Value++
		return
	}
	while ($true) {
		Read-JsonValueWithUniqueProperties $Text $Position $Label
		Skip-JsonWhitespace $Text $Position
		if ($Position.Value -ge $Text.Length) {
			throw "$Label contains an unterminated JSON array."
		}
		$delimiter = [int] $Text[$Position.Value]
		$Position.Value++
		if ($delimiter -eq 0x5d) {
			return
		}
		if ($delimiter -ne 0x2c) {
			throw "$Label contains invalid JSON: expected an array delimiter."
		}
		Skip-JsonWhitespace $Text $Position
	}
}

function Assert-JsonObjectPropertiesUnique {
	param(
		[string] $Text,
		[string] $Label
	)

	$position = 0
	Read-JsonValueWithUniqueProperties $Text ([ref] $position) $Label
	Skip-JsonWhitespace $Text ([ref] $position)
	if ($position -ne $Text.Length) {
		throw "$Label contains trailing content after its JSON value."
	}
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
		$preElisionValues = New-Object Collections.Generic.List[string]
		[void] $preElisionValues.Add($textValue)
		try {
			$decodedTextValue = [Uri]::UnescapeDataString($textValue)
			if ($decodedTextValue -cne $textValue) {
				[void] $preElisionValues.Add($decodedTextValue)
			}
		}
		catch {
			# Malformed URL escaping cannot make the original text safer. The
			# original bytes remain part of the mandatory pre-elision scan.
		}
		foreach ($preElisionValue in $preElisionValues) {
			if ($preElisionValue -match '(?i)file:(?:/+|\\+)' -or
				$preElisionValue -match '(?i)(?<![a-z0-9_])[a-z]:[\\/]' -or
				$preElisionValue -match '\\\\' -or
				$preElisionValue -match
					'(?i)\bhttps?://(?:localhost|[a-z0-9](?:[a-z0-9.-]*[a-z0-9])?|\[[0-9a-f:.]+\])(?::[0-9]{1,5})?/+(?:Users|home|mnt|tmp|var|etc|opt|root|Volumes)(?:[\\/]|[?#]|$)') {
				throw "$Label contains a local or rooted absolute path."
			}
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

function Get-ExactCampaignDebugStateDiffLabels {
	return @(
		"elapsed",
		"money",
		"HR",
		"training",
		"war level",
		"active missions",
		"objectives",
		"runtime vehicles",
		"mission assets",
		"active groups",
		"support requests",
		"enemy orders",
		"markers",
		"garage vehicles",
		"arsenal items",
		"civilian zones",
		"strategic events",
		"undercover records")
}

function Assert-ExactCampaignDebugStateDiff {
	param(
		[AllowEmptyString()]
		[string] $Text,
		[string] $RunId,
		[string] $Label
	)

	$expectedLabels = @(Get-ExactCampaignDebugStateDiffLabels)
	$lines = @([regex]::Split($Text, '\r?\n'))
	if ($lines.Count -gt 0 -and
		[string] $lines[$lines.Count - 1] -ceq "") {
		if ($lines.Count -eq 1) {
			$lines = @()
		}
		else {
			$lines = @($lines[0..($lines.Count - 2)])
		}
	}
	if ($lines.Count -ne ($expectedLabels.Count + 2)) {
		throw "$Label must contain exactly two headers and 18 state-diff rows."
	}
	if ([string] $lines[0] -cne "Partisan campaign debug state diff" -or
		[string] $lines[1] -cne "run $RunId") {
		throw "$Label does not contain the exact state-diff headers."
	}

	$rows = New-Object Collections.Generic.List[object]
	for ($rowIndex = 0; $rowIndex -lt $expectedLabels.Count; $rowIndex++) {
		$rowPattern = ('^{0} (?<before>-?(?:0|[1-9]\d*)) -> ' +
			'(?<after>-?(?:0|[1-9]\d*)) \| delta ' +
			'(?<delta>-?(?:0|[1-9]\d*))$') -f
			[regex]::Escape($expectedLabels[$rowIndex])
		$rowMatch = [regex]::Match([string] $lines[$rowIndex + 2], $rowPattern)
		if (-not $rowMatch.Success) {
			throw "$Label row $rowIndex does not match its exact ordered integer grammar."
		}
		$beforeValue = [Numerics.BigInteger]::Parse(
			$rowMatch.Groups["before"].Value,
			[Globalization.CultureInfo]::InvariantCulture)
		$afterValue = [Numerics.BigInteger]::Parse(
			$rowMatch.Groups["after"].Value,
			[Globalization.CultureInfo]::InvariantCulture)
		$deltaValue = [Numerics.BigInteger]::Parse(
			$rowMatch.Groups["delta"].Value,
			[Globalization.CultureInfo]::InvariantCulture)
		if (($afterValue - $beforeValue) -ne $deltaValue) {
			throw "$Label row $rowIndex has contradictory before/after/delta arithmetic."
		}
		if ($deltaValue -ne [Numerics.BigInteger]::Zero) {
			throw "$Label row $rowIndex is not a zero-delta restoration row."
		}
		[void] $rows.Add([PSCustomObject] [ordered] @{
			Label = $expectedLabels[$rowIndex]
			Before = $beforeValue
			After = $afterValue
			Delta = $deltaValue
		})
	}

	return [PSCustomObject] [ordered] @{
		Rows = $rows.ToArray()
		RowCount = $rows.Count
		NonzeroRowCount = 0
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
	$manifestEmbeddedValue = Get-ObjectPropertyValue `
		$manifestSourceValue "embeddedImplementation"
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
		$null -eq $manifestEmbeddedValue -or
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
	$embeddedShaValue = Require-Text `
		(Get-ObjectPropertyValue $manifestEmbeddedValue "sha") `
		"$Label manifest.source.embeddedImplementation.sha"
	$embeddedUtcValue = Require-Text `
		(Get-ObjectPropertyValue $manifestEmbeddedValue "utc") `
		"$Label manifest.source.embeddedImplementation.utc"
	$embeddedLabelValue = Require-Text `
		(Get-ObjectPropertyValue $manifestEmbeddedValue "label") `
		"$Label manifest.source.embeddedImplementation.label"
	if ($embeddedShaValue -cnotmatch '^[0-9a-f]{40}$') {
		throw "$Label embedded implementation SHA must be a lowercase full Git SHA."
	}
	$embeddedUtcTimestampValue = Require-UtcTimestamp `
		$embeddedUtcValue "$Label embedded implementation UTC"
	if ($embeddedUtcTimestampValue -gt $createdUtcValue) {
		throw "$Label embedded implementation UTC must not be later than manifest.createdUtc."
	}
	Assert-GitAncestor `
		$embeddedShaValue `
		$candidateSourceValue `
		"$Label embedded implementation build is not an ancestor of its candidate source HEAD"

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

	$retainedCandidateIdentity = [PSCustomObject] @{
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
		EmbeddedSha = $embeddedShaValue
		EmbeddedUtc = $embeddedUtcValue
		EmbeddedLabel = $embeddedLabelValue
		Manifest = $manifestValue
	}
	Assert-ExactPartisanCandidatePackageInventory `
		$retainedCandidateIdentity "$Label retained candidate"
	return $retainedCandidateIdentity
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
		[string] $ExpectedLabel,
		[ValidateSet("full_certification", "force_authority")]
		[string] $ExpectedProfile = "full_certification"
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
	$classifierFunctions = @($runnerAst.FindAll({
			param($node)
			$node -is [Management.Automation.Language.FunctionDefinitionAst] -and
				$node.Name -ceq 'Test-CampaignDebugHardDiagnosticCensus'
		}, $true))
	if ($classifierFunctions.Count -ne 1) {
		throw 'The bound guarded runner does not expose exactly one classifier self-test function.'
	}
	$classifierReturns = @($classifierFunctions[0].Body.FindAll({
			param($node)
			$node -is [Management.Automation.Language.ReturnStatementAst]
		}, $true))
	if ($classifierReturns.Count -ne 1 -or
		$classifierReturns[0].Pipeline.PipelineElements.Count -ne 1 -or
		$classifierReturns[0].Pipeline.PipelineElements[0] -isnot
			[Management.Automation.Language.CommandExpressionAst] -or
		$classifierReturns[0].Pipeline.PipelineElements[0].Expression -isnot
			[Management.Automation.Language.ConstantExpressionAst]) {
		throw 'The bound guarded runner classifier count is not one literal return value.'
	}
	$retainedRunnerClassifierCount = [int] `
		$classifierReturns[0].Pipeline.PipelineElements[0].Expression.Value
	if ($retainedRunnerClassifierCount -le 0) {
		throw 'The bound guarded runner classifier count must be positive.'
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
		ExpectedProfile = $ExpectedProfile
	}
	if ($ExpectedProfile -ceq "force_authority") {
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
	$semanticResult = & $semanticScript $artifactParameters $diagnosticParameters
	return [PSCustomObject] @{
		ArtifactValidation = $semanticResult.ArtifactValidation
		ErrorCensus = $semanticResult.ErrorCensus
		ClassifierSelfTestCount = $retainedRunnerClassifierCount
	}
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

function Assert-LegacyPackagedFocusedEvidence {
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

function Assert-NoDuplicateFocusedJsonProperties {
	param(
		[string] $Text,
		[string] $Label
	)

	$frames = New-Object 'Collections.Generic.Stack[object]'
	for ($index = 0; $index -lt $Text.Length; $index++) {
		$character = $Text[$index]
		if ($character -eq '{') {
			$frames.Push([PSCustomObject] @{
				IsObject = $true
				Keys = New-Object 'Collections.Generic.HashSet[string]' `
					([StringComparer]::Ordinal)
			})
			continue
		}
		if ($character -eq '[') {
			$frames.Push([PSCustomObject] @{
				IsObject = $false
				Keys = $null
			})
			continue
		}
		if ($character -eq '}' -or $character -eq ']') {
			if ($frames.Count -gt 0) {
				[void] $frames.Pop()
			}
			continue
		}
		if ($character -ne [char] 0x22) {
			continue
		}

		$tokenStart = $index
		$escaped = $false
		for ($index++; $index -lt $Text.Length; $index++) {
			$tokenCharacter = $Text[$index]
			if ($escaped) {
				$escaped = $false
				continue
			}
			if ($tokenCharacter -eq [char] 0x5c) {
				$escaped = $true
				continue
			}
			if ($tokenCharacter -eq [char] 0x22) {
				break
			}
		}
		if ($index -ge $Text.Length -or $frames.Count -eq 0 -or
			-not $frames.Peek().IsObject) {
			continue
		}
		$lookahead = $index + 1
		while ($lookahead -lt $Text.Length -and
			[char]::IsWhiteSpace($Text[$lookahead])) {
			$lookahead++
		}
		if ($lookahead -ge $Text.Length -or $Text[$lookahead] -ne ':') {
			continue
		}
		try {
			$propertyName = $Text.Substring(
				$tokenStart,
				$index - $tokenStart + 1) | ConvertFrom-Json
		}
		catch {
			throw "$Label contains an invalid JSON property name."
		}
		if (-not $frames.Peek().Keys.Add([string] $propertyName)) {
			throw "$Label contains a duplicate JSON property."
		}
	}
}

function ConvertFrom-FocusedStrictUtf8JsonSnapshot {
	param(
		[object] $Snapshot,
		[string] $Label
	)

	[byte[]] $bytes = $Snapshot.Bytes
	if (($bytes.Length -ge 3 -and
			$bytes[0] -eq 0xef -and $bytes[1] -eq 0xbb -and
			$bytes[2] -eq 0xbf) -or
		($bytes.Length -ge 2 -and
			(($bytes[0] -eq 0xff -and $bytes[1] -eq 0xfe) -or
			 ($bytes[0] -eq 0xfe -and $bytes[1] -eq 0xff))) -or
		($bytes.Length -ge 4 -and
			(($bytes[0] -eq 0x00 -and $bytes[1] -eq 0x00 -and
			  $bytes[2] -eq 0xfe -and $bytes[3] -eq 0xff) -or
			 ($bytes[0] -eq 0xff -and $bytes[1] -eq 0xfe -and
			  $bytes[2] -eq 0x00 -and $bytes[3] -eq 0x00)))) {
		throw "$Label must be BOM-less UTF-8."
	}
	try {
		$text = (New-Object Text.UTF8Encoding($false, $true)).GetString($bytes)
	}
	catch {
		throw "$Label is not strict BOM-less UTF-8."
	}
	if ($text.IndexOf([char] 0) -ge 0) {
		throw "$Label contains a NUL character."
	}
	Assert-NoDuplicateFocusedJsonProperties $text $Label
	try {
		$value = $text | ConvertFrom-Json
	}
	catch {
		throw "$Label is not valid JSON: $($_.Exception.Message)"
	}
	return [PSCustomObject] @{
		Text = $text
		Value = $value
	}
}

function ConvertFrom-FocusedStrictUtf8TextSnapshot {
	param(
		[object] $Snapshot,
		[string] $Label
	)

	[byte[]] $bytes = $Snapshot.Bytes
	if (($bytes.Length -ge 3 -and
			$bytes[0] -eq 0xef -and $bytes[1] -eq 0xbb -and
			$bytes[2] -eq 0xbf) -or
		($bytes.Length -ge 2 -and
			(($bytes[0] -eq 0xff -and $bytes[1] -eq 0xfe) -or
			 ($bytes[0] -eq 0xfe -and $bytes[1] -eq 0xff))) -or
		($bytes.Length -ge 4 -and
			(($bytes[0] -eq 0x00 -and $bytes[1] -eq 0x00 -and
			  $bytes[2] -eq 0xfe -and $bytes[3] -eq 0xff) -or
			 ($bytes[0] -eq 0xff -and $bytes[1] -eq 0xfe -and
			  $bytes[2] -eq 0x00 -and $bytes[3] -eq 0x00)))) {
		throw "$Label must be BOM-less UTF-8."
	}
	try {
		$text = (New-Object Text.UTF8Encoding($false, $true)).GetString($bytes)
	}
	catch {
		throw "$Label is not strict BOM-less UTF-8."
	}
	if ($text.IndexOf([char] 0) -ge 0) {
		throw "$Label contains a NUL character."
	}
	return $text
}

function Assert-ExactPartisanCandidatePackageInventory {
	param(
		[object] $CandidateIdentity,
		[string] $Label
	)

	$manifest = Get-ObjectPropertyValue $CandidateIdentity "Manifest"
	if ($null -eq $manifest) {
		throw "$Label is missing its retained package manifest."
	}
	$package = Get-ObjectPropertyValue $manifest "package"
	if ($null -eq $package) {
		throw "$Label is missing its retained package manifest."
	}
	Assert-ExactObjectProperties $package @(
		"root", "hashAlgorithm", "sha256", "canonicalIndexPath", "files") `
		"$Label package"
	if ([string] (Get-ObjectPropertyValue $package "root") -cne
			"package/Partisan" -or
		[string] (Get-ObjectPropertyValue $package "hashAlgorithm") -cne
			"sha256-manifest-v1" -or
		[string] (Get-ObjectPropertyValue $package "canonicalIndexPath") -cne
			"evidence/pack/files.sha256") {
		throw "$Label package header is not canonical."
	}

	$expectedTuples = @(
		"package/Partisan/addon.gproj|Partisan/addon.gproj",
		"package/Partisan/data.pak|Partisan/data.pak",
		"package/Partisan/resourceDatabase.rdb|Partisan/resourceDatabase.rdb",
		"package/Partisan/thumbnail.png|Partisan/thumbnail.png")
	$files = @(Get-ObjectPropertyValue $package "files")
	if ($files.Count -ne $expectedTuples.Count) {
		throw "$Label package inventory must contain exactly four canonical files."
	}
	$paths = New-Object Collections.Generic.List[string]
	$indexPaths = New-Object Collections.Generic.List[string]
	$tuples = New-Object Collections.Generic.List[string]
	$canonicalRows = New-Object Collections.Generic.List[object]
	foreach ($row in $files) {
		Assert-ExactObjectProperties $row @("path", "indexPath", "length", "sha256") `
			"$Label package file"
		$path = Require-RepoRelativePath `
			(Get-ObjectPropertyValue $row "path") "$Label package path"
		$indexPath = Require-RepoRelativePath `
			(Get-ObjectPropertyValue $row "indexPath") "$Label package index path"
		$length = Require-IntegerProperty $row "length" "$Label package file"
		$sha = (Require-Sha256 `
			(Get-ObjectPropertyValue $row "sha256") "$Label package file SHA-256").
			ToLowerInvariant()
		if ($length -le 0 -or
			[string] (Get-ObjectPropertyValue $row "sha256") -cne $sha) {
			throw "$Label package file length or SHA-256 is not canonical."
		}
		[void] $paths.Add($path)
		[void] $indexPaths.Add($indexPath)
		[void] $tuples.Add("$path|$indexPath")
		[void] $canonicalRows.Add([PSCustomObject] @{
			IndexPath = $indexPath
			Text = $sha + "`t" +
				$length.ToString([Globalization.CultureInfo]::InvariantCulture) +
				"`t" + $indexPath + "`n"
		})
	}
	Assert-UniqueStrings $paths.ToArray() "$Label package paths"
	Assert-UniqueStrings $indexPaths.ToArray() "$Label package index paths"
	Assert-EqualSet $expectedTuples $tuples.ToArray() "$Label package inventory tuples"

	$canonicalText = (@($canonicalRows | Sort-Object IndexPath -CaseSensitive |
		ForEach-Object { [string] $_.Text })) -join ""
	$computedPackageSha = Get-ByteArraySha256 `
		([Text.Encoding]::UTF8.GetBytes($canonicalText))
	$declaredPackageSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $package "sha256") "$Label package SHA-256").
		ToLowerInvariant()
	$identityPackageSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $CandidateIdentity "PackageSha256") `
		"$Label retained package SHA-256").ToLowerInvariant()
	if ([string] (Get-ObjectPropertyValue $package "sha256") -cne
			$declaredPackageSha -or
		$computedPackageSha -cne $declaredPackageSha -or
		$declaredPackageSha -cne $identityPackageSha) {
		throw "$Label package digest differs from its canonical four-file inventory."
	}
}

function Get-PartisanFocusedRequiredPatternContract {
	param([string] $ConsoleText)

	$patterns = New-Object Collections.Generic.List[string]
	$contentLines = New-Object Collections.Generic.List[string]
	$seen = New-Object 'Collections.Generic.HashSet[string]' `
		([StringComparer]::Ordinal)
	$utf8 = New-Object Text.UTF8Encoding($false, $true)
	foreach ($lineValue in @($ConsoleText -split "`r?`n")) {
		$line = [string] $lineValue
		if (-not $line.StartsWith(
				$script:FocusedRequiredPatternMarker,
				[StringComparison]::Ordinal)) {
			[void] $contentLines.Add($line)
			continue
		}
		$encoded = $line.Substring($script:FocusedRequiredPatternMarker.Length)
		try {
			$bytes = [Convert]::FromBase64String($encoded)
			$pattern = $utf8.GetString($bytes)
		}
		catch {
			throw 'The retained required-pattern contract is not canonical UTF-8 Base64.'
		}
		if ([Convert]::ToBase64String($bytes) -cne $encoded -or
			[string]::IsNullOrWhiteSpace($pattern) -or
			$pattern.Length -gt 4096 -or
			$pattern -match '[\x00\r\n]' -or
			-not $seen.Add($pattern)) {
			throw 'The retained required-pattern contract is empty, duplicated, or noncanonical.'
		}
		[void] $patterns.Add($pattern)
	}
	if ($patterns.Count -lt 1 -or $patterns.Count -gt 64) {
		throw 'The retained required-pattern contract must contain between one and 64 patterns.'
	}
	$contentText = $contentLines.ToArray() -join "`n"
	foreach ($pattern in $patterns) {
		if ($contentText.IndexOf(
				$pattern,
				[StringComparison]::Ordinal) -lt 0) {
			throw 'Retained console evidence does not satisfy its required-pattern contract.'
		}
	}
	return [PSCustomObject] [ordered] @{
		PatternCount = $patterns.Count
		Patterns = $patterns.ToArray()
		EvidenceConsoleText = $contentText
	}
}

function Get-PartisanFocusedRawMountAttestation {
	param(
		[string] $ConsoleText,
		[string] $ExpectedAddonGuid
	)

	$pattern = "(?im)^\s*\d{2}:\d{2}:\d{2}\.\d{3}\s+ENGINE\s+:\s+" +
		"gproj:\s+'(?<path>[^']+)'\s+guid:\s+'(?<guid>[^']+)'" +
		"\s*(?<mode>\([^)]+\))?\s*$"
	$recordCount = 0
	$exactPathCount = 0
	$packedCount = 0
	$invalidModeCount = 0
	$guidExactCount = 0
	foreach ($match in @([regex]::Matches($ConsoleText, $pattern))) {
		$recordCount++
		$recordedProject = $match.Groups['path'].Value.Replace('\', '/')
		if ($recordedProject.Equals(
				$script:FocusedMountProjectSuffix,
				[StringComparison]::OrdinalIgnoreCase) -or
			$recordedProject.EndsWith(
				'/' + $script:FocusedMountProjectSuffix,
				[StringComparison]::OrdinalIgnoreCase)) {
			$exactPathCount++
		}
		if ($match.Groups['guid'].Value.Equals(
				$ExpectedAddonGuid,
				[StringComparison]::Ordinal)) {
			$guidExactCount++
		}
		if ($match.Groups['mode'].Value -ceq '(packed)') {
			$packedCount++
		}
		elseif (-not [string]::IsNullOrEmpty($match.Groups['mode'].Value)) {
			$invalidModeCount++
		}
	}
	$guidExact = $recordCount -gt 0 -and $guidExactCount -eq $recordCount
	$packed = $packedCount -gt 0 -and $invalidModeCount -eq 0
	return [PSCustomObject] [ordered] @{
		Valid = $recordCount -gt 0 -and
			$exactPathCount -eq $recordCount -and
			$packed -and
			$guidExact
		RecordCount = $recordCount
		ExactPathCount = $exactPathCount
		PackedCount = $packedCount
		InvalidModeCount = $invalidModeCount
		GuidExact = $guidExact
		Packed = $packed
	}
}

function Test-FocusedContainedPath {
	param(
		[string] $RootPath,
		[string] $Path
	)

	$rootFull = [IO.Path]::GetFullPath($RootPath).TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar)
	$pathFull = [IO.Path]::GetFullPath($Path)
	return $pathFull.Equals($rootFull, [StringComparison]::OrdinalIgnoreCase) -or
		$pathFull.StartsWith(
			$rootFull + [IO.Path]::DirectorySeparatorChar,
			[StringComparison]::OrdinalIgnoreCase)
}

function Assert-FocusedNoReparseAncestry {
	param(
		[string] $RootPath,
		[string] $Path,
		[string] $Label
	)

	$rootFull = [IO.Path]::GetFullPath($RootPath).TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar)
	$pathFull = [IO.Path]::GetFullPath($Path)
	if (-not (Test-FocusedContainedPath $rootFull $pathFull)) {
		throw "$Label escaped its trusted root."
	}
	foreach ($candidatePath in @($rootFull, $pathFull)) {
		if (-not (Test-Path -LiteralPath $candidatePath)) {
			throw "$Label path is missing."
		}
	}
	$cursor = $rootFull
	$rootItem = Get-Item -LiteralPath $cursor -Force
	if (($rootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
		throw "$Label root must not be a reparse point."
	}
	if (-not $pathFull.Equals($rootFull, [StringComparison]::OrdinalIgnoreCase)) {
		$relative = $pathFull.Substring($rootFull.Length + 1)
		foreach ($segment in $relative.Split(
			@([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar),
			[StringSplitOptions]::RemoveEmptyEntries)) {
			$cursor = Join-Path $cursor $segment
			$item = Get-Item -LiteralPath $cursor -Force
			if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
				throw "$Label must not traverse a reparse point."
			}
		}
	}
}

function Get-FocusedAggregateId {
	param(
		[object] $Integrity,
		[object[]] $SourceRuns
	)

	$bindingText = ($Integrity | ConvertTo-Json -Depth 100 -Compress) + "`n" +
		(@($SourceRuns | ForEach-Object {
			[string] $_.testCase + '|' + [string] $_.runId + '|' +
			[string] $_.runEnvelopeSha256 + '|' +
			(@($_.files | ForEach-Object {
				[string] $_.path + ':' + [string] $_.length + ':' +
				[string] $_.sha256
			}) -join ',')
		}) -join "`n")
	$bytes = [Text.Encoding]::UTF8.GetBytes($bindingText)
	return 'focused-set-' + (Get-ByteArraySha256 $bytes).Substring(0, 24)
}

function Get-FocusedRawDiagnosticCensus {
	param(
		[string] $ConsoleText,
		[string] $ExpectedProfile,
		[string] $ExpectedSuite
	)

	$lines = @($ConsoleText -split "`r?`n")
	$suiteStartedIndex = -1
	$testSuccessIndex = -1
	$runnerFinishedIndex = -1
	$junitSavedIndex = -1
	$failedListSavedIndex = -1
	$nonMutatingIndex = -1
	$exactSeamIndex = -1
	$nonMutatingCount = 0
	$exactSeamCount = 0
	$suiteStartedCount = 0
	$testSuccessCount = 0
	$allSuiteStartedCount = 0
	$allTestSuccessCount = 0
	$runnerFinishedCount = 0
	$junitSavedCount = 0
	$failedListSavedCount = 0
	$timestampedScriptPrefix =
		'^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+'
	$suiteStartedPattern = $timestampedScriptPrefix + 'TestSuite #' +
		[regex]::Escape($ExpectedSuite) + ' started\s*$'
	$testSuccessPattern = $timestampedScriptPrefix +
		[regex]::Escape($ExpectedProfile) + ': SUCCESS\s*$'
	$allSuiteStartedPattern = $timestampedScriptPrefix +
		'TestSuite #[^\r\n]+ started\s*$'
	$allTestSuccessPattern = $timestampedScriptPrefix +
		'[^\r\n]+: SUCCESS\s*$'
	$runnerFinishedPattern = $timestampedScriptPrefix +
		'SCR_TestRunner has finished running\s*$'
	$junitSavedPattern =
		'^\s*(?:\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+)?' +
		'Autotest JUnit XML saved to:(?:\s+.*)?$'
	$failedListSavedPattern =
		'^\s*(?:\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+)?' +
		'Autotest failed list saved to:(?:\s+.*)?$'
	$nonMutatingPattern = '^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+(?:.* \| )?failed native callback non-mutating 1(?: \| .*)?$'
	$exactSeamPattern = '^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+setup/seam/request/bytes/journal 1/1/1/1/1(?: \| .*)?$'
	for ($index = 0; $index -lt $lines.Count; $index++) {
		$line = [string] $lines[$index]
		if ($line -cmatch $allSuiteStartedPattern) {
			$allSuiteStartedCount++
		}
		if ($line -cmatch $allTestSuccessPattern) {
			$allTestSuccessCount++
		}
		if ($line -cmatch $suiteStartedPattern) {
			$suiteStartedIndex = $index
			$suiteStartedCount++
		}
		if ($line -cmatch $testSuccessPattern) {
			$testSuccessIndex = $index
			$testSuccessCount++
		}
		if ($line -cmatch $runnerFinishedPattern) {
			$runnerFinishedIndex = $index
			$runnerFinishedCount++
		}
		if ($line -cmatch $junitSavedPattern) {
			$junitSavedIndex = $index
			$junitSavedCount++
		}
		if ($line -cmatch $failedListSavedPattern) {
			$failedListSavedIndex = $index
			$failedListSavedCount++
		}
		if ($line -cmatch $nonMutatingPattern) {
			$nonMutatingIndex = $index; $nonMutatingCount++
		}
		if ($line -cmatch $exactSeamPattern) {
			$exactSeamIndex = $index; $exactSeamCount++
		}
	}
	$isJournal = $ExpectedProfile -ceq 'HST_TEST_CampaignProfileJournalAuthority'
	$proofTokens = $nonMutatingCount -eq 1 -and $exactSeamCount -eq 1
	$hardPattern = '\b(?:SCRIPT|ENGINE)\s+\(E\):'
	$stockPattern = "^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+\(E\): Can't instantiate class 'SCR_FilterCategory', constructor is not public\s*$"
	$intentionalPattern = "^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+\(E\): string failureDetail = 'Partisan persistence \| native save callback failure \| sequence/type/flags 1/0/0 \| manager/enabled/allowed/busy/active/playthrough 1/1/1/0/0/0 \| types/persistence/state/loaded/tracked/config/staged 5/1/2/0/0/0/1 \| replication mode 0 \| snapshot fingerprint '\s*$"
	$completionFloor = [Math]::Max(
		$runnerFinishedIndex,
		[Math]::Max($junitSavedIndex, $failedListSavedIndex))
	$hard = 0; $stock = 0; $intentional = 0; $unapproved = 0
	for ($index = 0; $index -lt $lines.Count; $index++) {
		$line = [string] $lines[$index]
		if ($line -cnotmatch $hardPattern) { continue }
		$hard++
		if ($line -cmatch $stockPattern) {
			if ($completionFloor -ge 0 -and $index -gt $completionFloor -and
				$stock -lt 2) { $stock++ } else { $unapproved++ }
			continue
		}
		if ($line -cmatch $intentionalPattern) {
			if ($isJournal -and $proofTokens -and $suiteStartedIndex -ge 0 -and
				$testSuccessIndex -gt $suiteStartedIndex -and
				$index -gt $suiteStartedIndex -and $index -lt $testSuccessIndex -and
				$nonMutatingIndex -gt $index -and $nonMutatingIndex -lt $testSuccessIndex -and
				$exactSeamIndex -gt $index -and $exactSeamIndex -lt $testSuccessIndex -and
				$intentional -lt 1) { $intentional++ } else { $unapproved++ }
			continue
		}
		$unapproved++
	}
	$expectedIntentional = if ($isJournal) { 1 } else { 0 }
	$markerOrder = $suiteStartedCount -eq 1 -and
		$testSuccessCount -eq 1 -and
		$allSuiteStartedCount -eq 1 -and
		$allTestSuccessCount -eq 1 -and
		$runnerFinishedCount -eq 1 -and
		$junitSavedCount -eq 1 -and
		$failedListSavedCount -eq 1 -and
		$suiteStartedIndex -ge 0 -and
		$testSuccessIndex -gt $suiteStartedIndex -and
		$runnerFinishedIndex -gt $testSuccessIndex -and
		$junitSavedIndex -gt $runnerFinishedIndex -and
		$failedListSavedIndex -gt $junitSavedIndex
	return [PSCustomObject] @{
		Valid = $markerOrder -and $stock -eq 2 -and
			$intentional -eq $expectedIntentional -and $unapproved -eq 0
		HardDiagnosticFree = $hard -eq 0
		HardDiagnosticCount = $hard
		ApprovedStockFilterCount = $stock
		ApprovedIntentionalFaultCount = $intentional
		UnapprovedHardDiagnosticCount = $unapproved
		MarkerOrderExact = $markerOrder
		ProfileProofTokensSeen = $proofTokens
	}
}

function ConvertTo-PartisanFocusedSafeDiagnosticText {
	param(
		[AllowEmptyString()]
		[Parameter(Mandatory = $true)]
		[string]$Text
	)

	$safe = [string]$Text
	$marker = [regex]::Match(
		$safe,
		'(?i)^(?<prefix>.*Autotest (?:JUnit XML|failed list) saved to:)' +
			'(?<suffix>\s+.+)$')
	if ($marker.Success) {
		$safe = $marker.Groups['prefix'].Value + ' ' +
			$script:FocusedDiagnosticLocalPathToken
	}
	$safe = [regex]::Replace(
		$safe,
		'(?i)(?<![A-Z0-9._%+-])[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}' +
			'(?![A-Z0-9.-])',
		'<email>')
	$safe = [regex]::Replace(
		$safe,
		'(?<!\d)\d{15,20}(?!\d)',
		'<identity>')
	$safe = [regex]::Replace(
		$safe,
		'(?i)(?<![A-Z0-9_])file://[^<>\r\n|"'']+',
		$script:FocusedDiagnosticLocalPathToken)
	$safe = [regex]::Replace(
		$safe,
		'(?i)(?:(?<![A-Z0-9_])[A-Z]:[\\/]|\\\\)[^<>\r\n|"'']+',
		$script:FocusedDiagnosticLocalPathToken)
	$safe = [regex]::Replace(
		$safe,
		'(?i)(?<![A-Z0-9_])/(?:Users|home|mnt)/[^<>\r\n|"'']+',
		$script:FocusedDiagnosticLocalPathToken)
	return $safe
}

function Assert-PortablePackagedFocusedEvidence {
	param(
		[object] $Evidence,
		[object] $CandidateIdentity,
		[string] $ExpectedStatus,
		[string] $Label,
		[string] $PortableEvidenceRoot = $EvidenceBundleRoot,
		[switch] $AllowUntrackedSummaryForSelfTest
	)

	$expectedStatusFields = @(
		"status", "summaryPath", "summarySha256", "aggregateId",
		"candidateId", "candidateSourceHead", "packageSha256",
		"manifestSha256", "readySha256", "workbenchCrc", "harnessGitHead",
		"aggregationGitHead", "focusedRunnerSha256", "candidateModuleSha256",
		"acceptedStartedUtc", "acceptedCompletedUtc", "caseCount",
		"passedCases", "junitTests", "junitFailures", "junitErrors",
		"junitSkipped", "candidateBoundaryVerified", "allMountsPacked",
		"allCleanupAndSpillZero", "allEnvelopeFilesRehashed",
		"envelopeFileCount", "hardDiagnosticClassifierChecksPerRun",
		"hardDiagnosticClassificationValid", "hardDiagnosticFree",
		"hardDiagnosticCount", "approvedStockFilterDiagnosticCount",
		"approvedIntentionalFaultDiagnosticCount",
		"unapprovedHardDiagnosticCount", "aggregatePolicyAssertionCount",
		"aggregatePolicyAssertionsPassed", "aggregatePolicyAssertionsFailed",
		"acceptanceDisposition")
	Assert-ExactObjectProperties $Evidence $expectedStatusFields `
		"$Label portable status"
	if ([string] (Get-ObjectPropertyValue $Evidence "status") -cne
			$ExpectedStatus -or
		$ExpectedStatus -cnotin @(
			"passed-noncertifying", "historical-passed-noncertifying")) {
		throw "$Label.status is not the expected accepted focused disposition."
	}
	$requireCurrentWorktree = $ExpectedStatus -ceq "passed-noncertifying"
	Assert-ExactPartisanCandidatePackageInventory `
		$CandidateIdentity "$Label retained candidate"

	$summaryPath = Require-RepoRelativePath `
		(Get-ObjectPropertyValue $Evidence "summaryPath") "$Label.summaryPath"
	$summarySha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "summarySha256") `
		"$Label.summarySha256").ToLowerInvariant()
	$expectedSummaryPath = "docs/evidence/focused-autotest/{0}.json" -f
		[string] $CandidateIdentity.CandidateId
	if ($summaryPath -cne $expectedSummaryPath) {
		throw "$Label schema-2 summary path is not the canonical candidate path."
	}
	$summaryFullPath = [IO.Path]::GetFullPath((Join-Path $root $summaryPath))
	if (-not (Test-FocusedContainedPath $root $summaryFullPath) -or
		-not (Test-Path -LiteralPath $summaryFullPath -PathType Leaf)) {
		throw "$Label schema-2 summary is missing or outside the repository."
	}
	Assert-FocusedNoReparseAncestry $root $summaryFullPath `
		"$Label schema-2 summary"
	if (-not $AllowUntrackedSummaryForSelfTest) {
		& git -C $root ls-files --error-unmatch -- $summaryPath *> $null
		if ($LASTEXITCODE -ne 0) {
			throw "$Label schema-2 summary must be tracked or staged."
		}
	}
	$summarySnapshot = Read-FileByteSnapshot $summaryFullPath `
		"$Label schema-2 summary"
	if ([string] $summarySnapshot.Sha256 -cne $summarySha) {
		throw "$Label schema-2 summary SHA-256 does not match release status."
	}
	$summaryInput = ConvertFrom-FocusedStrictUtf8JsonSnapshot `
		$summarySnapshot "$Label schema-2 summary"
	$summary = $summaryInput.Value
	Assert-NoLocalAbsolutePathValue $summary "$Label schema-2 summary"
	Assert-ExactObjectProperties $summary @(
		"schemaVersion", "evidenceKind", "aggregateId", "aggregationPolicy",
		"admission", "candidate", "harness", "integrity", "acceptedWindow",
		"result", "aggregatePolicyAssertions", "cases", "sourceRuns",
		"preliminaryRuns") "$Label schema-2 summary"
	$policy = Get-ObjectPropertyValue $summary "aggregationPolicy"
	$admission = Get-ObjectPropertyValue $summary "admission"
	$summaryCandidate = Get-ObjectPropertyValue $summary "candidate"
	$summaryHarness = Get-ObjectPropertyValue $summary "harness"
	$integrity = Get-ObjectPropertyValue $summary "integrity"
	$acceptedWindow = Get-ObjectPropertyValue $summary "acceptedWindow"
	$summaryResult = Get-ObjectPropertyValue $summary "result"
	$aggregateAssertions = Get-ObjectPropertyValue `
		$summary "aggregatePolicyAssertions"
	$summaryCases = @(Get-ObjectPropertyValue $summary "cases")
	$sourceRuns = @(Get-ObjectPropertyValue $summary "sourceRuns")
	$preliminaryRuns = Get-ObjectPropertyValue $summary "preliminaryRuns"
	Assert-ExactObjectProperties $policy @(
		"contractId", "policyVersion", "inputSchemaVersion",
		"inputEvidenceKind", "requiredRuntimeUseDisposition",
		"requiredCandidateState", "profileOrder", "profileCount",
		"filesPerProfile", "totalFileCount", "rawReopenRequired",
		"exactDirectoryCensusRequired", "candidateSealReopenRequired",
		"historicalBlobImmutabilityRequired",
		"aggregatePolicyAssertionsPerProfile",
		"aggregatePolicyAssertionCount", "acceptedResultStatus",
		"acceptedDisposition", "rejectionDisposition") `
		"$Label aggregation policy"
	Assert-ExactObjectProperties $admission `
		@("status", "disposition", "releaseDecision", "certifying") `
		"$Label admission"
	Assert-ExactObjectProperties $summaryCandidate @(
		"candidateId", "candidateSourceHead", "packageSha256",
		"manifestSha256", "readySha256", "workbenchCrc") `
		"$Label candidate"
	Assert-ExactObjectProperties $summaryHarness @(
		"gitHead", "dirty", "focusedRunnerSha256", "candidateModuleSha256",
		"gitBlobProvenanceVerified") "$Label focused harness"
	Assert-ExactObjectProperties $integrity @(
		"aggregationGitHead", "focusedRunHarness", "aggregateProducer",
		"releaseDocsConsumer", "allWorktreeHashesMatchGitBlobs") `
		"$Label aggregate integrity"
	Assert-ExactObjectProperties $integrity.focusedRunHarness @(
		"gitHead", "focusedRunnerWorktreeSha256",
		"focusedRunnerGitBlobSha256", "candidateModuleWorktreeSha256",
		"candidateModuleGitBlobSha256") "$Label focused-run integrity"
	foreach ($toolName in @("aggregateProducer", "releaseDocsConsumer")) {
		Assert-ExactObjectProperties $integrity.$toolName `
			@("repositoryPath", "worktreeSha256", "gitBlobSha256") `
			"$Label $toolName integrity"
	}
	Assert-ExactObjectProperties $acceptedWindow `
		@("startedUtc", "completedUtc") "$Label accepted window"
	Assert-ExactObjectProperties $summaryResult @(
		"status", "caseCount", "passedCases", "junitTests", "junitFailures",
		"junitErrors", "junitSkipped", "candidateBoundaryVerified",
		"allMountsPacked", "allCleanupAndSpillZero",
		"allEnvelopeFilesRehashed", "envelopeFileCount",
		"hardDiagnosticClassifierChecksPerRun",
		"hardDiagnosticClassificationValid", "hardDiagnosticFree",
		"hardDiagnosticCount", "approvedStockFilterDiagnosticCount",
		"approvedIntentionalFaultDiagnosticCount",
		"unapprovedHardDiagnosticCount", "scope") "$Label aggregate result"
	Assert-ExactObjectProperties $aggregateAssertions @(
		"assertionClass", "scope", "total", "passed", "failed",
		"assertions") "$Label aggregate assertions"
	Assert-ExactObjectProperties $preliminaryRuns `
		@("caseCount", "status", "note") "$Label preliminary runs"

	$expectedProfiles = @(
		"HST_TEST_EnemyCounterattackAuthority",
		"HST_TEST_EnemyGarrisonRebuildAuthority",
		"HST_TEST_EnemyPlanningCommitmentAuthority",
		"HST_TEST_EnemyQRFAuthority",
		"HST_TEST_CampaignProfileJournalAuthority")
	$expectedSuites = [ordered] @{
		"HST_TEST_EnemyCounterattackAuthority" =
			"HST_EnemyCounterattackAutotestSuite"
		"HST_TEST_EnemyGarrisonRebuildAuthority" =
			"HST_EnemyGarrisonRebuildAutotestSuite"
		"HST_TEST_EnemyPlanningCommitmentAuthority" =
			"HST_EnemyPlanningCommitmentAutotestSuite"
		"HST_TEST_EnemyQRFAuthority" = "HST_EnemyQRFAutotestSuite"
		"HST_TEST_CampaignProfileJournalAuthority" =
			"HST_CampaignProfileJournalAuthorityAutotestSuite"
	}
	$expectedAssertionNames = @(
		"canonical_profile_identity", "sealed_candidate_identity",
		"single_clean_harness_identity", "portable_eight_blob_index",
		"all_indexed_blobs_rehashed", "passing_junit_case",
		"accepted_guarded_outcome")
	foreach ($requiredBoolean in @(
			"rawReopenRequired", "exactDirectoryCensusRequired",
			"candidateSealReopenRequired", "historicalBlobImmutabilityRequired")) {
		$value = Get-ObjectPropertyValue $policy $requiredBoolean
		if ($value -isnot [bool] -or -not [bool] $value) {
			throw "$Label disables mandatory policy $requiredBoolean."
		}
	}
	if ((Require-IntegerProperty $summary "schemaVersion" "$Label schema") -ne 2 -or
		[string] (Get-ObjectPropertyValue $summary "evidenceKind") -cne
			"packaged-focused-autotest-set" -or
		[string] (Get-ObjectPropertyValue $policy "contractId") -cne
			"partisan.focused-autotest.aggregate.v2" -or
		(Require-IntegerProperty $policy "policyVersion" "$Label policy version") -ne 2 -or
		(Require-IntegerProperty $policy "inputSchemaVersion" "$Label input schema") -ne 1 -or
		[string] (Get-ObjectPropertyValue $policy "inputEvidenceKind") -cne
			"packaged-focused-autotest" -or
		[string] (Get-ObjectPropertyValue $policy `
			"requiredRuntimeUseDisposition") -cne "active-runtime-candidate" -or
		[string] (Get-ObjectPropertyValue $policy "requiredCandidateState") -cne
			"retained-uncertified" -or
		(@(Get-ObjectPropertyValue $policy "profileOrder") -join '|') -cne
			($expectedProfiles -join '|') -or
		(Require-IntegerProperty $policy "profileCount" "$Label profile count") -ne 5 -or
		(Require-IntegerProperty $policy "filesPerProfile" "$Label files per profile") -ne 8 -or
		(Require-IntegerProperty $policy "totalFileCount" "$Label total files") -ne 40 -or
		(Require-IntegerProperty $policy "aggregatePolicyAssertionsPerProfile" `
			"$Label assertions per profile") -ne 7 -or
		(Require-IntegerProperty $policy "aggregatePolicyAssertionCount" `
			"$Label assertion count") -ne 35 -or
		[string] (Get-ObjectPropertyValue $policy "acceptedResultStatus") -cne
			"passed-noncertifying" -or
		[string] (Get-ObjectPropertyValue $policy "acceptedDisposition") -cne
			"accepted-noncertifying" -or
		[string] (Get-ObjectPropertyValue $policy "rejectionDisposition") -cne
			"replacement-required" -or
		[string] (Get-ObjectPropertyValue $admission "status") -cne "accepted" -or
		[string] (Get-ObjectPropertyValue $admission "disposition") -cne
			"accepted-noncertifying" -or
		[string] (Get-ObjectPropertyValue $admission "releaseDecision") -cne
			"NO-GO" -or
		(Get-ObjectPropertyValue $admission "certifying") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $admission "certifying")) {
		throw "$Label schema-2 policy or admission disposition drifted."
	}

	$identityBindings = [ordered] @{
		candidateId = [string] $CandidateIdentity.CandidateId
		candidateSourceHead = [string] $CandidateIdentity.CandidateSourceHead
		packageSha256 = [string] $CandidateIdentity.PackageSha256
		manifestSha256 = [string] $CandidateIdentity.ManifestSha256
		readySha256 = [string] $CandidateIdentity.ReadySha256
		workbenchCrc = [string] $CandidateIdentity.WorkbenchCrc
	}
	foreach ($binding in $identityBindings.GetEnumerator()) {
		if ([string] (Get-ObjectPropertyValue $summaryCandidate $binding.Key) -cne
				[string] $binding.Value -or
			[string] (Get-ObjectPropertyValue $Evidence $binding.Key) -cne
				[string] $binding.Value) {
			throw "$Label $($binding.Key) differs from the retained candidate."
		}
	}

	$rawHarnessHead = Require-Text `
		(Get-ObjectPropertyValue $summaryHarness "gitHead") "$Label raw harness"
	$aggregationHead = Require-Text `
		(Get-ObjectPropertyValue $integrity "aggregationGitHead") `
		"$Label aggregation Git HEAD"
	if ($rawHarnessHead -cnotmatch '^[0-9a-f]{40}$' -or
		$aggregationHead -cnotmatch '^[0-9a-f]{40}$' -or
		[string] (Get-ObjectPropertyValue $integrity.focusedRunHarness "gitHead") -cne
			$rawHarnessHead -or
		(Get-ObjectPropertyValue $summaryHarness "dirty") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $summaryHarness "dirty") -or
		(Get-ObjectPropertyValue $summaryHarness "gitBlobProvenanceVerified") -isnot
			[bool] -or
		-not [bool] (Get-ObjectPropertyValue $summaryHarness `
			"gitBlobProvenanceVerified") -or
		(Get-ObjectPropertyValue $integrity "allWorktreeHashesMatchGitBlobs") -isnot
			[bool] -or
		-not [bool] (Get-ObjectPropertyValue $integrity `
			"allWorktreeHashesMatchGitBlobs")) {
		throw "$Label does not bind clean raw and aggregation harnesses."
	}
	Assert-GitAncestor ([string] $CandidateIdentity.CandidateSourceHead) `
		$rawHarnessHead `
		"$Label candidate source is not an ancestor of its focused raw harness"
	Assert-GitAncestor $rawHarnessHead $aggregationHead `
		"$Label focused raw harness is not an ancestor of its aggregation harness"
	$currentHeadRows = @(& git -C $root rev-parse HEAD 2>$null)
	$currentHead = ($currentHeadRows -join '').Trim()
	if ($LASTEXITCODE -ne 0 -or $currentHead -cnotmatch '^[0-9a-f]{40}$') {
		throw "$Label cannot resolve the current checkout HEAD."
	}
	Assert-GitAncestor $aggregationHead $currentHead `
		"$Label aggregation harness is not an ancestor of the current checkout"

	$rawToolBindings = @(
		[PSCustomObject] @{
			SummaryField = "focusedRunnerSha256"
			WorktreeField = "focusedRunnerWorktreeSha256"
			BlobField = "focusedRunnerGitBlobSha256"
			Path = "tools/run-guarded-focused-autotest.ps1"
		},
		[PSCustomObject] @{
			SummaryField = "candidateModuleSha256"
			WorktreeField = "candidateModuleWorktreeSha256"
			BlobField = "candidateModuleGitBlobSha256"
			Path = "tools/Partisan.ReleaseCandidate.psm1"
		})
	foreach ($binding in $rawToolBindings) {
		$summaryToolSha = (Require-Sha256 `
			(Get-ObjectPropertyValue $summaryHarness $binding.SummaryField) `
			"$Label $($binding.SummaryField)").ToLowerInvariant()
		$recordedWorktree = (Require-Sha256 `
			(Get-ObjectPropertyValue $integrity.focusedRunHarness `
				$binding.WorktreeField) "$Label $($binding.WorktreeField)").
			ToLowerInvariant()
		$recordedBlob = (Require-Sha256 `
			(Get-ObjectPropertyValue $integrity.focusedRunHarness `
				$binding.BlobField) "$Label $($binding.BlobField)").ToLowerInvariant()
		$immutableBlob = Get-GitBlobSha256 `
			$rawHarnessHead $binding.Path "$Label immutable $($binding.Path)"
		if ($summaryToolSha -cne $recordedWorktree -or
			$recordedWorktree -cne $recordedBlob -or
			$recordedBlob -cne $immutableBlob -or
			[string] (Get-ObjectPropertyValue $Evidence $binding.SummaryField) -cne
				$summaryToolSha) {
			throw "$Label raw tool $($binding.Path) differs from its immutable Git blob."
		}
		if ($requireCurrentWorktree) {
			$currentWorktree = (Get-FileHash `
				-LiteralPath (Join-Path $root $binding.Path) `
				-Algorithm SHA256).Hash.ToLowerInvariant()
			if ($currentWorktree -cne $immutableBlob) {
				throw "$Label raw tool $($binding.Path) differs from its stationary Git blob."
			}
		}
	}
	$aggregationToolBindings = @(
		[PSCustomObject] @{
			Object = $integrity.aggregateProducer
			Path = "tools/New-PartisanFocusedAutotestAggregate.ps1"
		},
		[PSCustomObject] @{
			Object = $integrity.releaseDocsConsumer
			Path = "tools/update-release-docs.ps1"
		})
	foreach ($binding in $aggregationToolBindings) {
		if ([string] (Get-ObjectPropertyValue $binding.Object "repositoryPath") -cne
				$binding.Path) {
			throw "$Label aggregation tool path is not canonical."
		}
		$recordedWorktree = (Require-Sha256 `
			(Get-ObjectPropertyValue $binding.Object "worktreeSha256") `
			"$Label aggregation worktree SHA").ToLowerInvariant()
		$recordedBlob = (Require-Sha256 `
			(Get-ObjectPropertyValue $binding.Object "gitBlobSha256") `
			"$Label aggregation blob SHA").ToLowerInvariant()
		$immutableBlob = Get-GitBlobSha256 `
			$aggregationHead $binding.Path "$Label immutable $($binding.Path)"
		if ($recordedWorktree -cne $recordedBlob -or
			$recordedBlob -cne $immutableBlob) {
			throw "$Label aggregation tool $($binding.Path) differs from its immutable Git blob."
		}
		if ($requireCurrentWorktree) {
			$currentWorktree = (Get-FileHash `
				-LiteralPath (Join-Path $root $binding.Path) `
				-Algorithm SHA256).Hash.ToLowerInvariant()
			if ($currentWorktree -cne $immutableBlob) {
				throw "$Label aggregation tool $($binding.Path) differs from its stationary Git blob."
			}
		}
	}

	$manifestFullPath = [IO.Path]::GetFullPath(
		(Join-Path $root ([string] $CandidateIdentity.ManifestPath)))
	$readyFullPath = Join-Path (Split-Path -Parent $manifestFullPath) `
		"candidate.ready.json"
	Assert-FocusedNoReparseAncestry $root $manifestFullPath `
		"$Label tracked candidate manifest"
	Assert-FocusedNoReparseAncestry $root $readyFullPath `
		"$Label tracked candidate ready seal"
	$manifestSnapshot = Read-FileByteSnapshot $manifestFullPath `
		"$Label tracked candidate manifest"
	$readySnapshot = Read-FileByteSnapshot $readyFullPath `
		"$Label tracked candidate ready seal"
	if ([string] $manifestSnapshot.Sha256 -cne
			[string] $CandidateIdentity.ManifestSha256 -or
		[string] $readySnapshot.Sha256 -cne
			[string] $CandidateIdentity.ReadySha256) {
		throw "$Label tracked candidate seal changed."
	}

	if ([string]::IsNullOrWhiteSpace($PortableEvidenceRoot) -or
		-not (Test-Path -LiteralPath $PortableEvidenceRoot -PathType Container)) {
		throw "$Label requires EvidenceBundleRoot to reopen the focused raw set."
	}
	$evidenceRoot = [IO.Path]::GetFullPath($PortableEvidenceRoot).TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar)
	Assert-FocusedNoReparseAncestry $evidenceRoot $evidenceRoot `
		"$Label evidence root"
	if ($summaryCases.Count -ne 5 -or $sourceRuns.Count -ne 5) {
		throw "$Label must contain exactly five cases and five source runs."
	}

	$allSnapshots = New-Object Collections.Generic.List[object]
	$runCensusClosures = New-Object Collections.Generic.List[object]
	$runIds = @()
	$envelopeHashes = @()
	$totalHard = 0; $totalStock = 0; $totalIntentional = 0
	$totalUnapproved = 0; $totalFiles = 0
	$previousCompleted = $null
	for ($caseIndex = 0; $caseIndex -lt 5; $caseIndex++) {
		$profile = $expectedProfiles[$caseIndex]
		$suite = [string] $expectedSuites[$profile]
		$case = $summaryCases[$caseIndex]
		$sourceRun = $sourceRuns[$caseIndex]
		Assert-ExactObjectProperties $case @(
			"testCase", "suiteClass", "runId", "startedUtc", "completedUtc",
			"envelopeSha256", "fileCount", "success",
			"candidateBoundaryVerified", "mountPacked", "junitTests",
			"junitFailures", "junitErrors", "junitSkipped",
			"hardDiagnosticClassifierChecks",
			"hardDiagnosticClassificationValid", "hardDiagnosticFree",
			"hardDiagnosticCount", "approvedStockFilterDiagnosticCount",
			"approvedIntentionalFaultDiagnosticCount",
			"unapprovedHardDiagnosticCount", "cleanupAndSpillZero",
			"envelopeFilesRehashed") "$Label case $profile"
		Assert-ExactObjectProperties $sourceRun @(
			"testCase", "suiteClass", "runId", "runEnvelopePath",
			"runEnvelopeSha256", "fileCount", "files") `
			"$Label source run $profile"
		$runId = Require-Text `
			(Get-ObjectPropertyValue $sourceRun "runId") "$Label run ID"
		$expectedRunPath = "{0}/focused-autotest/{1}/{2}/run.json" -f
			[string] $CandidateIdentity.CandidateId, $profile, $runId
		$runRelativePath = Require-RepoRelativePath `
			(Get-ObjectPropertyValue $sourceRun "runEnvelopePath") `
			"$Label run envelope path"
		if ($runRelativePath -cne $expectedRunPath -or
			$runId -cnotmatch '^\d{8}T\d{6}Z-[0-9a-f]{32}$' -or
			[string] (Get-ObjectPropertyValue $case "testCase") -cne $profile -or
			[string] (Get-ObjectPropertyValue $sourceRun "testCase") -cne $profile -or
			[string] (Get-ObjectPropertyValue $case "suiteClass") -cne $suite -or
			[string] (Get-ObjectPropertyValue $sourceRun "suiteClass") -cne $suite -or
			[string] (Get-ObjectPropertyValue $case "runId") -cne $runId) {
			throw "$Label source-run identity is not canonical for $profile."
		}
		$runFullPath = [IO.Path]::GetFullPath(
			(Join-Path $evidenceRoot $runRelativePath.Replace('/', '\')))
		if (-not (Test-FocusedContainedPath $evidenceRoot $runFullPath) -or
			-not (Test-Path -LiteralPath $runFullPath -PathType Leaf)) {
			throw "$Label focused run is missing or escaped EvidenceBundleRoot."
		}
		Assert-FocusedNoReparseAncestry $evidenceRoot $runFullPath `
			"$Label focused run $profile"
		$runSnapshot = Read-FileByteSnapshot $runFullPath `
			"$Label focused run $profile"
		[void] $allSnapshots.Add($runSnapshot)
		$envelopeSha = (Require-Sha256 `
			(Get-ObjectPropertyValue $sourceRun "runEnvelopeSha256") `
			"$Label run envelope SHA").ToLowerInvariant()
		if ([string] $runSnapshot.Sha256 -cne $envelopeSha -or
			[string] (Get-ObjectPropertyValue $case "envelopeSha256") -cne
				$envelopeSha) {
			throw "$Label focused run envelope hash differs."
		}
		$run = (ConvertFrom-FocusedStrictUtf8JsonSnapshot `
			$runSnapshot "$Label focused run $profile").Value
		Assert-NoLocalAbsolutePathValue $run "$Label focused run $profile"
		Assert-ExactObjectProperties $run @(
			"schemaVersion", "evidenceKind", "startedUtc", "completedUtc",
			"candidate", "harness", "launch", "outcome", "cleanup", "files") `
			"$Label focused run $profile"
		if ((Require-IntegerProperty $run "schemaVersion" "$Label run schema") -ne 1 -or
			[string] (Get-ObjectPropertyValue $run "evidenceKind") -cne
				"packaged-focused-autotest") {
			throw "$Label focused run schema is unsupported."
		}

		$indexedRows = @(Get-ObjectPropertyValue $sourceRun "files")
		$runRows = @(Get-ObjectPropertyValue $run "files")
		if ($indexedRows.Count -ne 8 -or $runRows.Count -ne 8 -or
			(Require-IntegerProperty $sourceRun "fileCount" "$Label source file count") -ne 8 -or
			(Require-IntegerProperty $case "fileCount" "$Label case file count") -ne 8) {
			throw "$Label focused run must index exactly eight blobs."
		}
		$runRowMap = @{}
		foreach ($row in $runRows) {
			Assert-ExactObjectProperties $row @("path", "length", "sha256") `
				"$Label raw run row"
			$rowPath = Require-RepoRelativePath `
				(Get-ObjectPropertyValue $row "path") "$Label raw run row path"
			if ($runRowMap.ContainsKey($rowPath.ToLowerInvariant())) {
				throw "$Label raw run row paths collide."
			}
			$runRowMap[$rowPath.ToLowerInvariant()] = $row
		}
		$sourcePaths = @()
		$sourcePathSet = New-Object 'Collections.Generic.HashSet[string]' `
			([StringComparer]::OrdinalIgnoreCase)
		foreach ($row in $indexedRows) {
			Assert-ExactObjectProperties $row @("path", "length", "sha256") `
				"$Label aggregate source row"
			$rowPath = Require-RepoRelativePath `
				(Get-ObjectPropertyValue $row "path") "$Label aggregate source path"
			if (-not $sourcePathSet.Add($rowPath) -or
				-not $runRowMap.ContainsKey($rowPath.ToLowerInvariant())) {
				throw "$Label aggregate/raw source row census differs."
			}
			$rawRow = $runRowMap[$rowPath.ToLowerInvariant()]
			$rowLength = Require-IntegerProperty $row "length" "$Label row length"
			$rowSha = (Require-Sha256 `
				(Get-ObjectPropertyValue $row "sha256") "$Label row SHA").ToLowerInvariant()
			if ($rowLength -lt 0 -or
				(Require-IntegerProperty $rawRow "length" "$Label raw row length") -ne
					$rowLength -or
				[string] (Get-ObjectPropertyValue $rawRow "sha256") -cne $rowSha) {
				throw "$Label aggregate/raw source row differs."
			}
			$sourcePaths += $rowPath
		}
		$rawPaths = @($runRows | ForEach-Object { [string] $_.path })
		if (($rawPaths -join "`n") -cne ($sourcePaths -join "`n")) {
			throw "$Label aggregate/raw source row ordering differs."
		}
		if (($sourcePaths -join "`n") -cne
			(@($sourcePaths | Sort-Object -CaseSensitive) -join "`n")) {
			throw "$Label aggregate source rows are not canonically ordered."
		}
		$requiredPathPatterns = @(
			'^identity/candidate\.json$',
			'^identity/candidate\.ready\.json$',
			'^raw/logs/[^/]+/autotest\.log$',
			'^raw/logs/[^/]+/autotest_failed\.log$',
			'^raw/logs/[^/]+/console\.log$',
			'^raw/logs/[^/]+/error\.log$',
			'^raw/logs/[^/]+/junit\.xml$',
			'^raw/logs/[^/]+/script\.log$')
		foreach ($pattern in $requiredPathPatterns) {
			if (@($sourcePaths | Where-Object { $_ -cmatch $pattern }).Count -ne 1) {
				throw "$Label focused run has an invalid eight-blob role census."
			}
		}
		$rawParents = @($sourcePaths | Where-Object {
			$_ -cmatch '^raw/logs/'
		} | ForEach-Object { $_.Substring(0, $_.LastIndexOf('/')) } |
			Sort-Object -Unique -CaseSensitive)
		if ($rawParents.Count -ne 1) {
			throw "$Label focused raw logs do not share one canonical directory."
		}

		$runRoot = Split-Path -Parent $runFullPath
		$actualFiles = @(Get-ChildItem -LiteralPath $runRoot -Recurse -File -Force)
		$actualRelative = @()
		foreach ($actualFile in $actualFiles) {
			Assert-FocusedNoReparseAncestry $runRoot $actualFile.FullName `
				"$Label focused run file"
			$relative = $actualFile.FullName.Substring($runRoot.Length + 1).
				Replace('\', '/')
			$actualRelative += $relative
		}
		$expectedFilesystemPaths = @('run.json') + $sourcePaths
		Assert-EqualSet $expectedFilesystemPaths $actualRelative `
			"$Label exact focused run directory census"
		[void] $runCensusClosures.Add([PSCustomObject] @{
			Root = $runRoot
			ExpectedPaths = @($expectedFilesystemPaths)
		})

		$rowSnapshots = @{}
		foreach ($row in $indexedRows) {
			$rowPath = [string] $row.path
			$rowFullPath = [IO.Path]::GetFullPath(
				(Join-Path $runRoot $rowPath.Replace('/', '\')))
			if (-not (Test-FocusedContainedPath $runRoot $rowFullPath) -or
				-not (Test-Path -LiteralPath $rowFullPath -PathType Leaf)) {
				throw "$Label indexed focused blob is missing or escaped its run root."
			}
			Assert-FocusedNoReparseAncestry $runRoot $rowFullPath `
				"$Label indexed focused blob"
			$snapshot = Read-FileByteSnapshot $rowFullPath `
				"$Label focused blob $rowPath"
			[void] $allSnapshots.Add($snapshot)
			if ([long] $snapshot.Length -ne [long] $row.length -or
				[string] $snapshot.Sha256 -cne [string] $row.sha256) {
				throw "$Label indexed focused blob differs from its retained hash."
			}
			$rowSnapshots[$rowPath] = $snapshot
		}
		if (-not [Linq.Enumerable]::SequenceEqual(
				[byte[]] $manifestSnapshot.Bytes,
				[byte[]] $rowSnapshots['identity/candidate.json'].Bytes) -or
			-not [Linq.Enumerable]::SequenceEqual(
				[byte[]] $readySnapshot.Bytes,
				[byte[]] $rowSnapshots['identity/candidate.ready.json'].Bytes)) {
			throw "$Label raw candidate seals differ from the tracked candidate."
		}

		$runCandidate = Get-ObjectPropertyValue $run "candidate"
		$runHarness = Get-ObjectPropertyValue $run "harness"
		$launch = Get-ObjectPropertyValue $run "launch"
		$outcome = Get-ObjectPropertyValue $run "outcome"
		$cleanup = Get-ObjectPropertyValue $run "cleanup"
		Assert-ExactObjectProperties $runCandidate @(
			"candidateId", "candidateVersion", "runtimeUseDisposition", "gitHead",
			"embeddedBuildSha", "embeddedBuildUtc", "embeddedBuildLabel",
			"campaignSchema", "runtimeSettingsSchema", "addonId", "addonGuid",
			"packageHashAlgorithm", "packageSha256", "manifestSha256", "readySha256",
			"workbenchCrc", "runtimeRole", "diagnosticExecutable",
			"recordedDiagnosticExecutable", "recordedRuntimeExecutable") `
			"$Label run candidate"
		Assert-ExactObjectProperties $runHarness @(
			"gitHead", "dirty", "focusedRunnerSha256", "candidateModuleSha256") `
			"$Label run harness"
		Assert-ExactObjectProperties $launch @(
			"testCase", "stagedPackage", "addonSearchRootCount", "addonGuid",
			"packageSha256", "diagnosticExecutable", "recordedRuntimeExecutable") `
			"$Label run launch"
		Assert-ExactObjectProperties $outcome @(
			"success", "candidateBoundaryVerified", "mountAttestation",
			"evidenceCaptured", "result", "diagnosticTail", "error") `
			"$Label run outcome"
		foreach ($executableBinding in @(
				@($runCandidate, "diagnosticExecutable"),
				@($runCandidate, "recordedDiagnosticExecutable"),
				@($runCandidate, "recordedRuntimeExecutable"),
				@($launch, "diagnosticExecutable"),
				@($launch, "recordedRuntimeExecutable"))) {
			Assert-ExactObjectProperties `
				(Get-ObjectPropertyValue $executableBinding[0] $executableBinding[1]) `
				@("fileName", "fileVersion", "productVersion", "length", "sha256") `
				"$Label executable $($executableBinding[1])"
		}
		if ([string] (Get-ObjectPropertyValue $runHarness "gitHead") -cne
				$rawHarnessHead -or
			(Get-ObjectPropertyValue $runHarness "dirty") -isnot [bool] -or
			[bool] (Get-ObjectPropertyValue $runHarness "dirty") -or
			[string] (Get-ObjectPropertyValue $runHarness "focusedRunnerSha256") -cne
				[string] $summaryHarness.focusedRunnerSha256 -or
			[string] (Get-ObjectPropertyValue $runHarness "candidateModuleSha256") -cne
				[string] $summaryHarness.candidateModuleSha256) {
			throw "$Label raw run harness differs from the aggregate harness."
		}
		$candidateStringBindings = [ordered] @{
			candidateId = [string] $CandidateIdentity.CandidateId
			candidateVersion = [string] $CandidateIdentity.PackageVersion
			runtimeUseDisposition = "active-runtime-candidate"
			gitHead = [string] $CandidateIdentity.CandidateSourceHead
			embeddedBuildSha = [string] $CandidateIdentity.EmbeddedSha
			embeddedBuildUtc = [string] $CandidateIdentity.EmbeddedUtc
			embeddedBuildLabel = [string] $CandidateIdentity.EmbeddedLabel
			addonId = [string] $CandidateIdentity.Manifest.addon.id
			addonGuid = [string] $CandidateIdentity.Manifest.addon.guid
			packageHashAlgorithm = [string] $CandidateIdentity.PackageHashAlgorithm
			packageSha256 = [string] $CandidateIdentity.PackageSha256
			manifestSha256 = [string] $CandidateIdentity.ManifestSha256
			readySha256 = [string] $CandidateIdentity.ReadySha256
			workbenchCrc = [string] $CandidateIdentity.WorkbenchCrc
			runtimeRole = "client"
		}
		foreach ($binding in $candidateStringBindings.GetEnumerator()) {
			if ([string] (Get-ObjectPropertyValue $runCandidate $binding.Key) -cne
					[string] $binding.Value) {
				throw "$Label raw candidate $($binding.Key) differs from its tracked manifest."
			}
		}
		if ((Require-IntegerProperty $runCandidate "campaignSchema" `
				"$Label run candidate campaign schema") -ne
				[long] $CandidateIdentity.CampaignSchema -or
			(Require-IntegerProperty $runCandidate "runtimeSettingsSchema" `
				"$Label run candidate settings schema") -ne
				[long] $CandidateIdentity.RuntimeSettingsSchema -or
			[string] (Get-ObjectPropertyValue $launch "testCase") -cne $profile -or
			[string] (Get-ObjectPropertyValue $launch "addonGuid") -cne
				[string] $CandidateIdentity.Manifest.addon.guid -or
			[string] (Get-ObjectPropertyValue $launch "packageSha256") -cne
				[string] $CandidateIdentity.PackageSha256 -or
			(Get-ObjectPropertyValue $launch "stagedPackage") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $launch "stagedPackage") -or
			(Require-IntegerProperty $launch "addonSearchRootCount" `
				"$Label add-on root count") -ne 2) {
			throw "$Label raw candidate or launch identity differs."
		}
		Assert-ExecutableIdentityEqual $CandidateIdentity.Manifest.toolchain.clientDiagnostic `
			(Get-ObjectPropertyValue $runCandidate "diagnosticExecutable") `
			"$Label run diagnostic executable"
		Assert-ExecutableIdentityEqual $CandidateIdentity.Manifest.toolchain.clientDiagnostic `
			(Get-ObjectPropertyValue $runCandidate "recordedDiagnosticExecutable") `
			"$Label run recorded diagnostic executable"
		Assert-ExecutableIdentityEqual $CandidateIdentity.Manifest.toolchain.client `
			(Get-ObjectPropertyValue $runCandidate "recordedRuntimeExecutable") `
			"$Label run runtime executable"
		Assert-ExecutableIdentityEqual $CandidateIdentity.Manifest.toolchain.clientDiagnostic `
			(Get-ObjectPropertyValue $launch "diagnosticExecutable") `
			"$Label launch diagnostic executable"
		Assert-ExecutableIdentityEqual $CandidateIdentity.Manifest.toolchain.client `
			(Get-ObjectPropertyValue $launch "recordedRuntimeExecutable") `
			"$Label launch runtime executable"

		$startedText = Require-Text `
			(Get-ObjectPropertyValue $run "startedUtc") "$Label run start"
		$completedText = Require-Text `
			(Get-ObjectPropertyValue $run "completedUtc") "$Label run completion"
		$started = Require-UtcTimestamp $startedText "$Label run start"
		$completed = Require-UtcTimestamp $completedText "$Label run completion"
		if ($started -ge $completed -or
			($null -ne $previousCompleted -and $started -lt $previousCompleted) -or
			-not $runId.StartsWith(
				$started.UtcDateTime.ToString('yyyyMMddTHHmmssZ'),
				[StringComparison]::Ordinal) -or
			[string] (Get-ObjectPropertyValue $case "startedUtc") -cne $startedText -or
			[string] (Get-ObjectPropertyValue $case "completedUtc") -cne
				$completedText) {
			throw "$Label focused run chronology is invalid."
		}
		$previousCompleted = $completed

		$mount = Get-ObjectPropertyValue $outcome "mountAttestation"
		Assert-ExactObjectProperties $mount @(
			"Valid", "RecordCount", "ExactPathCount", "PackedCount",
			"InvalidModeCount", "GuidExact", "Packed") "$Label mount attestation"
		if ((Get-ObjectPropertyValue $outcome "success") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $outcome "success") -or
			(Get-ObjectPropertyValue $outcome "candidateBoundaryVerified") -isnot
				[bool] -or
			-not [bool] (Get-ObjectPropertyValue $outcome `
				"candidateBoundaryVerified") -or
			(Get-ObjectPropertyValue $outcome "evidenceCaptured") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $outcome "evidenceCaptured") -or
			$null -ne (Get-ObjectPropertyValue $outcome "error") -or
			(Get-ObjectPropertyValue $mount "Valid") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $mount "Valid") -or
			(Get-ObjectPropertyValue $mount "GuidExact") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $mount "GuidExact") -or
			(Get-ObjectPropertyValue $mount "Packed") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $mount "Packed") -or
			(Require-IntegerProperty $mount "RecordCount" "$Label mount records") -le 0 -or
			(Require-IntegerProperty $mount "ExactPathCount" `
				"$Label exact mount paths") -ne
				(Require-IntegerProperty $mount "RecordCount" "$Label mount records") -or
			(Require-IntegerProperty $mount "PackedCount" "$Label packed mounts") -le 0 -or
			(Require-IntegerProperty $mount "InvalidModeCount" `
				"$Label invalid mount modes") -ne 0) {
			throw "$Label focused outcome or mount attestation is not accepted."
		}

		Assert-ExactObjectProperties $cleanup @(
			"GuardRemaining", "GuardBaseRemaining", "EngineProcessesRemaining",
			"OwnedProcessesRemaining", "UnclaimedEngineProcessesObserved",
			"NewDefaultEntriesRemaining", "ModifiedDefaultFiles",
			"DeletedDefaultEntries", "MissingDefaultRoots",
			"ExternalSpillEntriesRemaining", "ModifiedSpillFiles",
			"DeletedSpillEntries", "MissingSpillRoots",
			"MonitoringRootsAreDetectionOnly", "CleanupErrors") `
			"$Label cleanup receipt"
		foreach ($cleanupName in @(
				"GuardRemaining", "GuardBaseRemaining", "EngineProcessesRemaining",
				"OwnedProcessesRemaining", "UnclaimedEngineProcessesObserved",
				"NewDefaultEntriesRemaining", "ModifiedDefaultFiles",
				"DeletedDefaultEntries", "MissingDefaultRoots",
				"ExternalSpillEntriesRemaining", "ModifiedSpillFiles",
				"DeletedSpillEntries", "MissingSpillRoots")) {
			if ((Require-IntegerProperty $cleanup $cleanupName `
					"$Label cleanup $cleanupName") -ne 0) {
				throw "$Label focused cleanup receipt contains residue."
			}
		}
		if ((Get-ObjectPropertyValue $cleanup "MonitoringRootsAreDetectionOnly") -isnot
				[bool] -or
			-not [bool] (Get-ObjectPropertyValue $cleanup `
				"MonitoringRootsAreDetectionOnly") -or
			@(Get-ObjectPropertyValue $cleanup "CleanupErrors").Count -ne 0) {
			throw "$Label focused cleanup policy is invalid."
		}

		$roleSnapshots = @{}
		foreach ($role in @(
				"autotest.log", "autotest_failed.log", "console.log", "error.log",
				"junit.xml", "script.log")) {
			$path = @($sourcePaths | Where-Object {
				$_ -cmatch ('/' + [regex]::Escape($role) + '$')
			})[0]
			$roleSnapshots[$role] = $rowSnapshots[$path]
		}
		$failedText = ConvertFrom-FocusedStrictUtf8TextSnapshot `
			$roleSnapshots['autotest_failed.log'] "$Label failed-list blob"
		if (-not [string]::IsNullOrEmpty($failedText)) {
			throw "$Label accepted focused failed-list blob is not empty."
		}
		foreach ($textRole in @(
				"autotest.log", "error.log", "script.log")) {
			[void] (ConvertFrom-FocusedStrictUtf8TextSnapshot `
				$roleSnapshots[$textRole] "$Label $textRole blob")
		}
		$consoleText = ConvertFrom-FocusedStrictUtf8TextSnapshot `
			$roleSnapshots['console.log'] "$Label console blob"
		$requiredPatternContract = Get-PartisanFocusedRequiredPatternContract `
			-ConsoleText $consoleText
		if ($requiredPatternContract.PatternCount -le 0) {
			throw "$Label focused required-pattern result differs from retained raw evidence."
		}
		$rawMount = Get-PartisanFocusedRawMountAttestation `
			-ConsoleText $consoleText `
			-ExpectedAddonGuid ([string] $CandidateIdentity.Manifest.addon.guid)
		foreach ($mountProperty in @(
				'Valid', 'RecordCount', 'ExactPathCount', 'PackedCount',
				'InvalidModeCount', 'GuidExact', 'Packed')) {
			if ($rawMount.$mountProperty -ne $mount.$mountProperty) {
				throw "$Label recorded mount attestation differs from retained console evidence."
			}
		}
		$diagnostic = Get-FocusedRawDiagnosticCensus `
			$consoleText $profile $suite
		if (-not $diagnostic.Valid) {
			throw "$Label raw diagnostic/marker census is not the accepted contract."
		}
		$expectedBuildSummary = "sha {0} | utc {1} | label {2}" -f
			[string] $CandidateIdentity.EmbeddedSha,
			[string] $CandidateIdentity.EmbeddedUtc,
			[string] $CandidateIdentity.EmbeddedLabel
		foreach ($requiredConsoleText in @(
				$profile, ($profile + ": SUCCESS"), $expectedBuildSummary,
				"Autotest JUnit XML saved to:",
				"Autotest failed list saved to:")) {
			if ($consoleText.IndexOf(
					$requiredConsoleText,
					[StringComparison]::Ordinal) -lt 0) {
				throw "$Label raw console omits required accepted evidence."
			}
		}
		$derivedDiagnosticTail = @($consoleText -split "`r?`n" | Where-Object {
			$_ -match 'HST_|Autotest|autotest|Test Result|SCRIPT\s+\(E\)|ENGINE\s+\(E\)'
		} | Select-Object -Last 80 | ForEach-Object {
			ConvertTo-PartisanFocusedSafeDiagnosticText -Text ([string] $_)
		})
		$recordedDiagnosticTail = Get-ObjectPropertyValue $outcome "diagnosticTail"
		if ($recordedDiagnosticTail -isnot [array] -or
			(@($recordedDiagnosticTail) -join "`n") -cne
			($derivedDiagnosticTail -join "`n")) {
			throw "$Label raw diagnostic tail differs from retained console evidence."
		}

		$junitText = ConvertFrom-FocusedStrictUtf8TextSnapshot `
			$roleSnapshots['junit.xml'] "$Label JUnit blob"
		$xmlReader = $null
		$stringReader = $null
		try {
			$xmlSettings = New-Object Xml.XmlReaderSettings
			$xmlSettings.DtdProcessing = [Xml.DtdProcessing]::Prohibit
			$xmlSettings.XmlResolver = $null
			$stringReader = New-Object IO.StringReader($junitText)
			$xmlReader = [Xml.XmlReader]::Create($stringReader, $xmlSettings)
			$document = New-Object Xml.XmlDocument
			$document.XmlResolver = $null
			$document.Load($xmlReader)
		}
		catch {
			throw "$Label JUnit blob is not safe valid XML."
		}
		finally {
			if ($xmlReader) { $xmlReader.Dispose() }
			if ($stringReader) { $stringReader.Dispose() }
		}
		$suites = @($document.testsuites.testsuite)
		$testCases = @($suites | ForEach-Object { @($_.testcase) })
		if ($suites.Count -ne 1 -or $testCases.Count -ne 1 -or
			[int] $suites[0].tests -ne 1 -or [int] $suites[0].failures -ne 0 -or
			[int] $suites[0].errors -ne 0 -or [int] $suites[0].skipped -ne 0 -or
			[string] $suites[0].name -cne $suite -or
			[string] $testCases[0].name -cne $profile -or
			[string] $testCases[0].classname -cne $suite -or
			@($testCases[0].SelectNodes('failure')).Count -ne 0 -or
			@($testCases[0].SelectNodes('error')).Count -ne 0 -or
			@($testCases[0].SelectNodes('skipped')).Count -ne 0) {
			throw "$Label JUnit blob does not contain its exact passing case."
		}

		$result = Get-ObjectPropertyValue $outcome "result"
		Assert-ExactObjectProperties $result @(
			"Candidate", "CandidateBoundaryVerified", "MountAttestation",
			"Success", "ExitCode", "Tests", "Failures", "Errors",
			"JUnitTestCaseCount", "JUnitCaseName", "JUnitCaseClassName",
			"JUnitCaseIdentityExact", "JUnitCaseFailures", "JUnitCaseErrors",
			"JUnitCaseSkipped", "JUnitFailureEvidence", "JUnitErrorEvidence",
			"FailedListFileCount", "FailedListBytes", "RequiredPatternsSeen",
			"BuildProvenanceSeen", "ConsoleTestCaseSeen",
			"HardDiagnosticClassifierChecks", "HardDiagnosticClassificationValid",
			"HardDiagnosticFree", "HardDiagnosticCount",
			"ApprovedStockFilterDiagnosticCount",
			"ApprovedIntentionalFaultDiagnosticCount",
			"UnapprovedHardDiagnosticCount", "UnapprovedHardDiagnosticEvidence") `
			"$Label focused result"
		foreach ($requiredTrue in @(
				"CandidateBoundaryVerified", "Success", "JUnitCaseIdentityExact",
				"RequiredPatternsSeen", "BuildProvenanceSeen", "ConsoleTestCaseSeen",
				"HardDiagnosticClassificationValid")) {
			$value = Get-ObjectPropertyValue $result $requiredTrue
			if ($value -isnot [bool] -or -not [bool] $value) {
				throw "$Label focused result flag $requiredTrue is false."
			}
		}
		$resultHardFree = Get-ObjectPropertyValue $result "HardDiagnosticFree"
		$failureEvidence = Get-ObjectPropertyValue $result "JUnitFailureEvidence"
		$errorEvidence = Get-ObjectPropertyValue $result "JUnitErrorEvidence"
		# Direct property access preserves a canonical empty JSON array; function
		# output enumeration collapses that value to null in Windows PowerShell.
		$unapprovedEvidence =
			$result.PSObject.Properties["UnapprovedHardDiagnosticEvidence"].Value
		if ((Get-ObjectPropertyValue $result "Success") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $result "Success") -or
			(($result.Candidate | ConvertTo-Json -Depth 100 -Compress) -cne
				($runCandidate | ConvertTo-Json -Depth 100 -Compress)) -or
			(($result.MountAttestation | ConvertTo-Json -Depth 100 -Compress) -cne
				($mount | ConvertTo-Json -Depth 100 -Compress)) -or
			(Require-IntegerProperty $result "ExitCode" "$Label result exit") -ne 0 -or
			(Require-IntegerProperty $result "Tests" "$Label result tests") -ne 1 -or
			(Require-IntegerProperty $result "Failures" "$Label result failures") -ne 0 -or
			(Require-IntegerProperty $result "Errors" "$Label result errors") -ne 0 -or
			(Require-IntegerProperty $result "JUnitTestCaseCount" `
				"$Label testcase count") -ne 1 -or
			[string] (Get-ObjectPropertyValue $result "JUnitCaseName") -cne $profile -or
			[string] (Get-ObjectPropertyValue $result "JUnitCaseClassName") -cne
				$suite -or
			(Require-IntegerProperty $result "JUnitCaseFailures" `
				"$Label testcase failures") -ne 0 -or
			(Require-IntegerProperty $result "JUnitCaseErrors" `
				"$Label testcase errors") -ne 0 -or
			(Require-IntegerProperty $result "JUnitCaseSkipped" `
				"$Label testcase skipped") -ne 0 -or
			$failureEvidence -isnot [string] -or
			[string] $failureEvidence -cne "" -or
			$errorEvidence -isnot [string] -or
			[string] $errorEvidence -cne "" -or
			(Require-IntegerProperty $result "FailedListFileCount" `
				"$Label failed-list file count") -ne 1 -or
			(Require-IntegerProperty $result "FailedListBytes" `
				"$Label failed-list bytes") -ne 0 -or
			(Require-IntegerProperty $result "HardDiagnosticClassifierChecks" `
				"$Label classifier checks") -ne 12 -or
			$resultHardFree -isnot [bool] -or
			[bool] $resultHardFree -ne [bool] $diagnostic.HardDiagnosticFree -or
			(Require-IntegerProperty $result "HardDiagnosticCount" `
				"$Label hard diagnostics") -ne $diagnostic.HardDiagnosticCount -or
			(Require-IntegerProperty $result "ApprovedStockFilterDiagnosticCount" `
				"$Label stock diagnostics") -ne
				$diagnostic.ApprovedStockFilterCount -or
			(Require-IntegerProperty $result `
				"ApprovedIntentionalFaultDiagnosticCount" `
				"$Label intentional diagnostics") -ne
				$diagnostic.ApprovedIntentionalFaultCount -or
			(Require-IntegerProperty $result "UnapprovedHardDiagnosticCount" `
				"$Label unapproved diagnostics") -ne 0 -or
			$unapprovedEvidence -isnot [array] -or
			@($unapprovedEvidence).Count -ne 0) {
			throw "$Label focused result differs from retained JUnit/diagnostics."
		}

		$caseIntegerBindings = [ordered] @{
			junitTests = 1; junitFailures = 0; junitErrors = 0; junitSkipped = 0
			hardDiagnosticClassifierChecks = 12
			hardDiagnosticCount = [int] $diagnostic.HardDiagnosticCount
			approvedStockFilterDiagnosticCount =
				[int] $diagnostic.ApprovedStockFilterCount
			approvedIntentionalFaultDiagnosticCount =
				[int] $diagnostic.ApprovedIntentionalFaultCount
			unapprovedHardDiagnosticCount = 0
		}
		foreach ($binding in $caseIntegerBindings.GetEnumerator()) {
			if ((Require-IntegerProperty $case $binding.Key `
					"$Label case $($binding.Key)") -ne [int] $binding.Value) {
				throw "$Label aggregate case differs from retained raw evidence."
			}
		}
		foreach ($booleanName in @(
				"success", "candidateBoundaryVerified", "mountPacked",
				"hardDiagnosticClassificationValid", "cleanupAndSpillZero",
				"envelopeFilesRehashed")) {
			$value = Get-ObjectPropertyValue $case $booleanName
			if ($value -isnot [bool] -or -not [bool] $value) {
				throw "$Label accepted case flag $booleanName is false."
			}
		}
		$caseHardFree = Get-ObjectPropertyValue $case "hardDiagnosticFree"
		if ($caseHardFree -isnot [bool] -or
			[bool] $caseHardFree -ne [bool] $diagnostic.HardDiagnosticFree) {
			throw "$Label aggregate hard-diagnostic-free flag differs."
		}
		$runIds += $runId
		$envelopeHashes += $envelopeSha
		$totalHard += [int] $diagnostic.HardDiagnosticCount
		$totalStock += [int] $diagnostic.ApprovedStockFilterCount
		$totalIntentional += [int] $diagnostic.ApprovedIntentionalFaultCount
		$totalUnapproved += [int] $diagnostic.UnapprovedHardDiagnosticCount
		$totalFiles += 8
	}

	Assert-UniqueStrings $runIds "$Label focused run IDs"
	Assert-UniqueStrings $envelopeHashes "$Label focused envelope hashes"
	$assertionRows = @(Get-ObjectPropertyValue $aggregateAssertions "assertions")
	if ($assertionRows.Count -ne 35) {
		throw "$Label aggregate assertion census is not 35."
	}
	for ($profileIndex = 0; $profileIndex -lt 5; $profileIndex++) {
		for ($assertionIndex = 0; $assertionIndex -lt 7; $assertionIndex++) {
			$assertion = $assertionRows[($profileIndex * 7) + $assertionIndex]
			Assert-ExactObjectProperties $assertion @(
				"assertionId", "testCase", "status", "assertionClass") `
				"$Label aggregate assertion"
			$profile = $expectedProfiles[$profileIndex]
			$expectedAssertionId = $profile + '.' +
				$expectedAssertionNames[$assertionIndex]
			if ([string] $assertion.assertionId -cne $expectedAssertionId -or
				[string] $assertion.testCase -cne $profile -or
				[string] $assertion.status -cne "PASS" -or
				[string] $assertion.assertionClass -cne "aggregate-policy") {
				throw "$Label aggregate-policy assertion identity drifted."
			}
		}
	}

	$startedText = Require-Text `
		(Get-ObjectPropertyValue $acceptedWindow "startedUtc") `
		"$Label accepted start"
	$completedText = Require-Text `
		(Get-ObjectPropertyValue $acceptedWindow "completedUtc") `
		"$Label accepted completion"
	$acceptedStarted = Require-UtcTimestamp $startedText "$Label accepted start"
	$acceptedCompleted = Require-UtcTimestamp $completedText `
		"$Label accepted completion"
	$expectedAggregateId = Get-FocusedAggregateId $integrity $sourceRuns
	if ($acceptedStarted -ge $acceptedCompleted -or
		$startedText -cne [string] $summaryCases[0].startedUtc -or
		$completedText -cne [string] $summaryCases[4].completedUtc -or
		[string] (Get-ObjectPropertyValue $summary "aggregateId") -cne
			$expectedAggregateId -or
		[string] (Get-ObjectPropertyValue $aggregateAssertions "assertionClass") -cne
			"aggregate-policy" -or
		(Require-IntegerProperty $aggregateAssertions "total" `
			"$Label assertion total") -ne 35 -or
		(Require-IntegerProperty $aggregateAssertions "passed" `
			"$Label assertion passed") -ne 35 -or
		(Require-IntegerProperty $aggregateAssertions "failed" `
			"$Label assertion failed") -ne 0 -or
		(Require-IntegerProperty $preliminaryRuns "caseCount" `
			"$Label preliminary count") -ne 0 -or
		[string] (Get-ObjectPropertyValue $preliminaryRuns "status") -cne "none" -or
		[string]::IsNullOrWhiteSpace(
			[string] (Get-ObjectPropertyValue $preliminaryRuns "note"))) {
		throw "$Label aggregate identity, chronology, assertions, or preliminary set drifted."
	}

	$expectedResultIntegers = [ordered] @{
		caseCount = 5; passedCases = 5; junitTests = 5; junitFailures = 0
		junitErrors = 0; junitSkipped = 0; envelopeFileCount = 40
		hardDiagnosticClassifierChecksPerRun = 12
		hardDiagnosticCount = $totalHard
		approvedStockFilterDiagnosticCount = $totalStock
		approvedIntentionalFaultDiagnosticCount = $totalIntentional
		unapprovedHardDiagnosticCount = $totalUnapproved
	}
	foreach ($binding in $expectedResultIntegers.GetEnumerator()) {
		if ((Require-IntegerProperty $summaryResult $binding.Key `
				"$Label result $($binding.Key)") -ne [int] $binding.Value) {
			throw "$Label aggregate result $($binding.Key) differs from raw evidence."
		}
	}
	foreach ($booleanName in @(
			"candidateBoundaryVerified", "allMountsPacked",
			"allCleanupAndSpillZero", "allEnvelopeFilesRehashed",
			"hardDiagnosticClassificationValid")) {
		$value = Get-ObjectPropertyValue $summaryResult $booleanName
		if ($value -isnot [bool] -or -not [bool] $value) {
			throw "$Label aggregate result $booleanName is false."
		}
	}
	$aggregateHardFree = Get-ObjectPropertyValue $summaryResult "hardDiagnosticFree"
	if ([string] (Get-ObjectPropertyValue $summaryResult "status") -cne
			"passed-noncertifying" -or
		$aggregateHardFree -isnot [bool] -or
		[bool] $aggregateHardFree -ne ($totalHard -eq 0) -or
		$totalHard -ne 11 -or $totalStock -ne 10 -or $totalIntentional -ne 1 -or
		$totalUnapproved -ne 0) {
		throw "$Label aggregate diagnostic/result disposition is not exact."
	}

	$flatStringBindings = [ordered] @{
		aggregateId = $expectedAggregateId
		harnessGitHead = $rawHarnessHead
		aggregationGitHead = $aggregationHead
		acceptedStartedUtc = $startedText
		acceptedCompletedUtc = $completedText
		acceptanceDisposition = "accepted-noncertifying"
	}
	foreach ($binding in $flatStringBindings.GetEnumerator()) {
		if ([string] (Get-ObjectPropertyValue $Evidence $binding.Key) -cne
				[string] $binding.Value) {
			throw "$Label flat $($binding.Key) differs from the aggregate."
		}
	}
	$flatIntegerBindings = [ordered] @{
		caseCount = 5; passedCases = 5; junitTests = 5; junitFailures = 0
		junitErrors = 0; junitSkipped = 0; envelopeFileCount = 40
		hardDiagnosticClassifierChecksPerRun = 12
		hardDiagnosticCount = $totalHard
		approvedStockFilterDiagnosticCount = $totalStock
		approvedIntentionalFaultDiagnosticCount = $totalIntentional
		unapprovedHardDiagnosticCount = $totalUnapproved
		aggregatePolicyAssertionCount = 35
		aggregatePolicyAssertionsPassed = 35
		aggregatePolicyAssertionsFailed = 0
	}
	foreach ($binding in $flatIntegerBindings.GetEnumerator()) {
		if ((Require-IntegerProperty $Evidence $binding.Key `
				"$Label.$($binding.Key)") -ne [int] $binding.Value) {
			throw "$Label flat $($binding.Key) differs from the aggregate."
		}
	}
	$flatBooleanBindings = [ordered] @{
		candidateBoundaryVerified = $true
		allMountsPacked = $true
		allCleanupAndSpillZero = $true
		allEnvelopeFilesRehashed = $true
		hardDiagnosticClassificationValid = $true
		hardDiagnosticFree = ($totalHard -eq 0)
	}
	foreach ($binding in $flatBooleanBindings.GetEnumerator()) {
		$value = Get-ObjectPropertyValue $Evidence $binding.Key
		if ($value -isnot [bool] -or [bool] $value -ne [bool] $binding.Value) {
			throw "$Label flat $($binding.Key) differs from the aggregate."
		}
	}

	foreach ($closure in $runCensusClosures) {
		$closingRelative = @()
		foreach ($closingFile in @(Get-ChildItem `
				-LiteralPath $closure.Root -Recurse -File -Force)) {
			Assert-FocusedNoReparseAncestry `
				$closure.Root $closingFile.FullName "$Label closing focused run file"
			$closingRelative += $closingFile.FullName.Substring(
				$closure.Root.Length + 1).Replace('\', '/')
		}
		Assert-EqualSet @($closure.ExpectedPaths) $closingRelative `
			"$Label closing focused run directory census"
	}
	Assert-FileByteSnapshotUnchanged $summarySnapshot "$Label schema-2 summary"
	Assert-FileByteSnapshotUnchanged $manifestSnapshot `
		"$Label tracked candidate manifest"
	Assert-FileByteSnapshotUnchanged $readySnapshot `
		"$Label tracked candidate ready seal"
	foreach ($snapshot in $allSnapshots) {
		Assert-FileByteSnapshotUnchanged $snapshot "$Label retained focused blob"
	}

	return [PSCustomObject] @{
		SummaryPath = $summaryPath
		SummarySha256 = $summarySha
		AggregateId = $expectedAggregateId
		HarnessGitHead = $rawHarnessHead
		AggregationGitHead = $aggregationHead
		AcceptedStartedUtc = $acceptedStarted
		AcceptedCompletedUtc = $acceptedCompleted
		PreliminaryStatus = "none"
		PreliminaryCaseCount = 0
		CaseCount = 5
		PassedCases = 5
		JunitTests = 5
		JunitFailures = 0
		JunitErrors = 0
		JunitSkipped = 0
		HardDiagnosticCount = $totalHard
		ApprovedStockDiagnosticCount = $totalStock
		ApprovedIntentionalDiagnosticCount = $totalIntentional
		UnapprovedHardDiagnosticCount = $totalUnapproved
		EnvelopeFileCount = $totalFiles
		AggregatePolicyAssertionCount = 35
		AggregatePolicyAssertionsPassed = 35
		AggregatePolicyAssertionsFailed = 0
		AcceptanceDisposition = "accepted-noncertifying"
		CaseRunIds = @($runIds)
		CaseEnvelopeSha256s = @($envelopeHashes)
		PortableSchema = $true
	}
}

function Assert-PackagedFocusedEvidence {
	param(
		[object] $Evidence,
		[object] $CandidateIdentity,
		[string] $ExpectedStatus,
		[string] $Label,
		[string] $PortableEvidenceRoot = $EvidenceBundleRoot,
		[switch] $AllowUntrackedSummaryForSelfTest
	)

	if ($null -eq $Evidence) {
		throw "$Label is missing."
	}
	$summaryPath = Require-RepoRelativePath `
		(Get-ObjectPropertyValue $Evidence "summaryPath") "$Label.summaryPath"
	$summaryFullPath = [IO.Path]::GetFullPath((Join-Path $root $summaryPath))
	if (-not (Test-FocusedContainedPath $root $summaryFullPath) -or
		-not (Test-Path -LiteralPath $summaryFullPath -PathType Leaf)) {
		throw "$Label summary is missing or outside the repository."
	}
	Assert-FocusedNoReparseAncestry $root $summaryFullPath "$Label summary"
	$routeSnapshot = Read-FileByteSnapshot $summaryFullPath "$Label summary route"
	$routeInput = ConvertFrom-FocusedStrictUtf8JsonSnapshot `
		$routeSnapshot "$Label summary route"
	$routeSummary = $routeInput.Value
	$routeSchema = Require-IntegerProperty `
		$routeSummary "schemaVersion" "$Label summary.schemaVersion"
	$routeKind = [string] (Get-ObjectPropertyValue $routeSummary "evidenceKind")
	$usePortableSchema = $routeSchema -eq 2 -and
		$routeKind -ceq "packaged-focused-autotest-set" -and
		[string] (Get-ObjectPropertyValue `
			(Get-ObjectPropertyValue $routeSummary "aggregationPolicy") `
			"contractId") -ceq "partisan.focused-autotest.aggregate.v2"
	if ($routeSchema -eq 2 -and -not $usePortableSchema) {
		throw "$Label schema-2 focused summary has an unsupported identity."
	}
	if ($usePortableSchema) {
		return Assert-PortablePackagedFocusedEvidence `
			$Evidence $CandidateIdentity $ExpectedStatus $Label `
			-PortableEvidenceRoot $PortableEvidenceRoot `
			-AllowUntrackedSummaryForSelfTest:$AllowUntrackedSummaryForSelfTest
	}
	if ($routeSchema -ne 1 -or
		$routeKind -cne "packaged-focused-autotest-set") {
		throw "$Label focused summary uses an unsupported schema."
	}
	return Assert-LegacyPackagedFocusedEvidence `
		$Evidence $CandidateIdentity $ExpectedStatus $Label
}

function Invoke-PortableFocusedEvidenceConsumerSelfTest {
	$tempBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar)
	$tempRoot = Join-Path $tempBase (
		"partisan-focused-consumer-" + [Guid]::NewGuid().ToString("N"))
	$originalRoot = $script:root
	$utf8 = New-Object Text.UTF8Encoding($false, $true)
	$assertRejected = {
		param(
			[string] $CaseLabel,
			[scriptblock] $Action,
			[string] $ExpectedMessage
		)
		$rejected = $false
		try {
			& $Action
		}
		catch {
			if ([string] $_.Exception.Message -cnotmatch $ExpectedMessage) {
				throw "Focused consumer self-test $CaseLabel failed unexpectedly: $($_.Exception.Message)"
			}
			$rejected = $true
		}
		if (-not $rejected) {
			throw "Focused consumer self-test admitted $CaseLabel."
		}
	}
	try {
		[void] [IO.Directory]::CreateDirectory($tempRoot)
		$validPath = Join-Path $tempRoot "valid.json"
		[IO.File]::WriteAllBytes(
			$validPath,
			$utf8.GetBytes('{"outer":{"x":1},"items":[{"x":2}]}'))
		$validSnapshot = Read-FileByteSnapshot $validPath `
			"Focused consumer self-test valid JSON"
		$valid = ConvertFrom-FocusedStrictUtf8JsonSnapshot `
			$validSnapshot "Focused consumer self-test valid JSON"
		if ([long] $valid.Value.outer.x -ne 1 -or
			[long] $valid.Value.items[0].x -ne 2) {
			throw "Focused consumer self-test did not retain valid strict JSON."
		}
		Assert-FocusedNoReparseAncestry `
			$tempRoot $validPath "Focused consumer self-test path"
		if (-not (Test-FocusedContainedPath $tempRoot $validPath) -or
			(Test-FocusedContainedPath $tempRoot ($tempRoot + "-escape"))) {
			throw "Focused consumer self-test containment contract failed."
		}
		$pathBearingMarkers = @(
			"Autotest JUnit XML saved to: $(Join-Path $tempRoot 'raw/junit.xml')",
			"Autotest failed list saved to: $(Join-Path $tempRoot 'raw/autotest_failed.log')"
		)
		$expectedSafeMarkers = @(
			"Autotest JUnit XML saved to: <local-path>",
			"Autotest failed list saved to: <local-path>"
		)
		for ($markerIndex = 0; $markerIndex -lt $pathBearingMarkers.Count;
			$markerIndex++) {
			$normalizedMarker = ConvertTo-PartisanFocusedSafeDiagnosticText `
				-Text $pathBearingMarkers[$markerIndex]
			if ($normalizedMarker -cne $expectedSafeMarkers[$markerIndex]) {
				throw "Focused consumer self-test did not normalize a path-bearing marker."
			}
		}

		$duplicatePath = Join-Path $tempRoot "duplicate.json"
		[IO.File]::WriteAllBytes(
			$duplicatePath,
			$utf8.GetBytes('{"x":1,"x":2}'))
		& $assertRejected "duplicate JSON properties" {
			ConvertFrom-FocusedStrictUtf8JsonSnapshot `
				(Read-FileByteSnapshot $duplicatePath "duplicate JSON") `
				"duplicate JSON" | Out-Null
		} 'duplicate JSON property'

		$bomPath = Join-Path $tempRoot "bom.json"
		$bomPayload = [byte[]] (@(0xef, 0xbb, 0xbf) +
			@($utf8.GetBytes('{"x":1}')))
		[IO.File]::WriteAllBytes($bomPath, $bomPayload)
		& $assertRejected "UTF-8 BOM" {
			ConvertFrom-FocusedStrictUtf8JsonSnapshot `
				(Read-FileByteSnapshot $bomPath "BOM JSON") "BOM JSON" | Out-Null
		} 'BOM-less UTF-8'

		$utf32Path = Join-Path $tempRoot "utf32.txt"
		[IO.File]::WriteAllBytes(
			$utf32Path,
			[byte[]] @(0xff, 0xfe, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00))
		& $assertRejected "UTF-32 BOM" {
			ConvertFrom-FocusedStrictUtf8TextSnapshot `
				(Read-FileByteSnapshot $utf32Path "UTF-32 text") `
				"UTF-32 text" | Out-Null
		} 'BOM-less UTF-8'

		$nulPath = Join-Path $tempRoot "embedded-zero.txt"
		[IO.File]::WriteAllBytes($nulPath, [byte[]] @(0x61, 0x00, 0x62))
		& $assertRejected "NUL text" {
			ConvertFrom-FocusedStrictUtf8TextSnapshot `
				(Read-FileByteSnapshot $nulPath "NUL text") "NUL text" | Out-Null
		} 'NUL character'

		$integrity = [PSCustomObject] [ordered] @{
			aggregationGitHead = ('a' * 40)
			focusedRunHarness = [PSCustomObject] [ordered] @{
				gitHead = ('b' * 40)
			}
		}
		$sourceRuns = @([PSCustomObject] [ordered] @{
			testCase = "P"
			runId = "20260720T000000Z-" + ('c' * 32)
			runEnvelopeSha256 = ('d' * 64)
			files = @([PSCustomObject] [ordered] @{
				path = "identity/candidate.json"
				length = 3
				sha256 = ('e' * 64)
			})
		})
		if ((Get-FocusedAggregateId $integrity $sourceRuns) -cne
				"focused-set-e581fa3ca46b6d45863179fd") {
			throw "Focused consumer self-test aggregate ID oracle failed."
		}

		$focusedFixtureSourceRoot = $PSScriptRoot
		$producerTestPath = Join-Path $PSScriptRoot `
			"test-partisan-focused-autotest-aggregate.ps1"
		$producerTestSource = [IO.File]::ReadAllText($producerTestPath)
		$producerTestTokens = $null
		$producerTestParseErrors = $null
		$producerTestAst = [Management.Automation.Language.Parser]::ParseInput(
			$producerTestSource,
			[ref] $producerTestTokens,
			[ref] $producerTestParseErrors)
		if (@($producerTestParseErrors).Count -ne 0) {
			throw "Focused consumer self-test producer fixture source does not parse."
		}
		$fixtureFunctionNames = @(
			"Assert-SelfTest",
			"Get-SelfTestSha256",
			"Get-SelfTestTextSha256",
			"Get-SelfTestPackageSha256",
			"Write-SelfTestText",
			"Write-SelfTestJson",
			"Read-SelfTestJson",
			"New-SelfTestExecutable",
			"New-SelfTestFixture",
			"Invoke-SelfTestProducer",
			"New-SelfTestRepository")
		$fixtureFunctionSource = New-Object Collections.Generic.List[string]
		foreach ($fixtureFunctionName in $fixtureFunctionNames) {
			$fixtureFunctionMatches = @($producerTestAst.FindAll({
					param($node)
					$node -is [Management.Automation.Language.FunctionDefinitionAst] -and
						$node.Name -ceq $fixtureFunctionName
				}, $true))
			if ($fixtureFunctionMatches.Count -ne 1) {
				throw "Focused consumer self-test requires exactly one $fixtureFunctionName fixture function."
			}
			$fixtureFunctionText = $fixtureFunctionMatches[0].Extent.Text
			if ($fixtureFunctionName -ceq "New-SelfTestRepository") {
				$fixtureFunctionText = $fixtureFunctionText.Replace(
					'$PSScriptRoot', '$focusedFixtureSourceRoot')
			}
			[void] $fixtureFunctionSource.Add($fixtureFunctionText)
		}
		. ([scriptblock]::Create($fixtureFunctionSource.ToArray() -join "`n`n"))

		$profileOrder = @(
			"HST_TEST_EnemyCounterattackAuthority",
			"HST_TEST_EnemyGarrisonRebuildAuthority",
			"HST_TEST_EnemyPlanningCommitmentAuthority",
			"HST_TEST_EnemyQRFAuthority",
			"HST_TEST_CampaignProfileJournalAuthority")
		$suiteByProfile = [ordered] @{
			HST_TEST_EnemyCounterattackAuthority =
				"HST_EnemyCounterattackAutotestSuite"
			HST_TEST_EnemyGarrisonRebuildAuthority =
				"HST_EnemyGarrisonRebuildAutotestSuite"
			HST_TEST_EnemyPlanningCommitmentAuthority =
				"HST_EnemyPlanningCommitmentAutotestSuite"
			HST_TEST_EnemyQRFAuthority = "HST_EnemyQRFAutotestSuite"
			HST_TEST_CampaignProfileJournalAuthority =
				"HST_CampaignProfileJournalAuthorityAutotestSuite"
		}
		$producer = Join-Path $focusedFixtureSourceRoot `
			"New-PartisanFocusedAutotestAggregate.ps1"
		$producerRepositoryRoot = Join-Path $tempRoot "producer-shaped-repository"
		[void] [IO.Directory]::CreateDirectory($producerRepositoryRoot)
		$producerRepository = New-SelfTestRepository `
			-Root $producerRepositoryRoot
		$producerFixture = New-SelfTestFixture `
			-Root (Join-Path $tempRoot "producer-shaped-evidence") `
			-HarnessGitHead $producerRepository.RunHarnessGitHead `
			-RunnerSha256 $producerRepository.RunnerSha256 `
			-CandidateModuleSha256 $producerRepository.CandidateModuleSha256 `
			-TrackedManifestPath $producerRepository.ManifestPath `
			-TrackedReadyPath $producerRepository.ReadyPath
		$producer = Join-Path $producerRepository.Root `
			"tools/New-PartisanFocusedAutotestAggregate.ps1"
		$producerPublication = Invoke-SelfTestProducer `
			-EvidenceRoot $producerFixture.EvidenceRoot `
			-RunPaths $producerFixture.RunPaths `
			-OutputPath $producerRepository.OutputPath `
			-RepositoryRoot $producerRepository.Root
		if (-not $producerPublication.Succeeded) {
			throw "Focused consumer self-test producer-shaped publication failed: $($producerPublication.Error)"
		}

		$script:root = $producerRepository.Root
		$producerStatus = Get-Content -Raw `
			-LiteralPath $producerRepository.StatusPath | ConvertFrom-Json
		$producerCandidateIdentity = Get-RetainedCandidateIdentity `
			$producerStatus.artifact "Focused consumer self-test candidate"
		$producerSummarySnapshot = Read-FileByteSnapshot `
			$producerRepository.OutputPath `
			"Focused consumer self-test producer-shaped summary"
		$producerSummary = (ConvertFrom-FocusedStrictUtf8JsonSnapshot `
			$producerSummarySnapshot `
			"Focused consumer self-test producer-shaped summary").Value
		$newFocusedEvidence = {
			param(
				[object] $Summary,
				[string] $SummarySha256,
				[string] $SummaryPath
			)

			$result = $Summary.result
			$assertions = $Summary.aggregatePolicyAssertions
			return [PSCustomObject] [ordered] @{
				status = "passed-noncertifying"
				summaryPath = $SummaryPath
				summarySha256 = $SummarySha256
				aggregateId = $Summary.aggregateId
				candidateId = $Summary.candidate.candidateId
				candidateSourceHead = $Summary.candidate.candidateSourceHead
				packageSha256 = $Summary.candidate.packageSha256
				manifestSha256 = $Summary.candidate.manifestSha256
				readySha256 = $Summary.candidate.readySha256
				workbenchCrc = $Summary.candidate.workbenchCrc
				harnessGitHead = $Summary.harness.gitHead
				aggregationGitHead = $Summary.integrity.aggregationGitHead
				focusedRunnerSha256 = $Summary.harness.focusedRunnerSha256
				candidateModuleSha256 = $Summary.harness.candidateModuleSha256
				acceptedStartedUtc = $Summary.acceptedWindow.startedUtc
				acceptedCompletedUtc = $Summary.acceptedWindow.completedUtc
				caseCount = $result.caseCount
				passedCases = $result.passedCases
				junitTests = $result.junitTests
				junitFailures = $result.junitFailures
				junitErrors = $result.junitErrors
				junitSkipped = $result.junitSkipped
				candidateBoundaryVerified = $result.candidateBoundaryVerified
				allMountsPacked = $result.allMountsPacked
				allCleanupAndSpillZero = $result.allCleanupAndSpillZero
				allEnvelopeFilesRehashed = $result.allEnvelopeFilesRehashed
				envelopeFileCount = $result.envelopeFileCount
				hardDiagnosticClassifierChecksPerRun =
					$result.hardDiagnosticClassifierChecksPerRun
				hardDiagnosticClassificationValid =
					$result.hardDiagnosticClassificationValid
				hardDiagnosticFree = $result.hardDiagnosticFree
				hardDiagnosticCount = $result.hardDiagnosticCount
				approvedStockFilterDiagnosticCount =
					$result.approvedStockFilterDiagnosticCount
				approvedIntentionalFaultDiagnosticCount =
					$result.approvedIntentionalFaultDiagnosticCount
				unapprovedHardDiagnosticCount =
					$result.unapprovedHardDiagnosticCount
				aggregatePolicyAssertionCount = $assertions.total
				aggregatePolicyAssertionsPassed = $assertions.passed
				aggregatePolicyAssertionsFailed = $assertions.failed
				acceptanceDisposition = $Summary.admission.disposition
			}
		}
		$producerSummaryRelative = "docs/evidence/focused-autotest/{0}.json" -f
			$producerRepository.CandidateId
		$producerEvidence = & $newFocusedEvidence `
			-Summary $producerSummary `
			-SummarySha256 $producerSummarySnapshot.Sha256 `
			-SummaryPath $producerSummaryRelative
		$producerValidation = Assert-PackagedFocusedEvidence `
			$producerEvidence `
			$producerCandidateIdentity `
			"passed-noncertifying" `
			"focused producer-shaped consumer self-test" `
			-PortableEvidenceRoot $producerFixture.EvidenceRoot `
			-AllowUntrackedSummaryForSelfTest
		if (-not $producerValidation.PortableSchema -or
			$producerValidation.CaseCount -ne 5 -or
			$producerValidation.PassedCases -ne 5 -or
			$producerValidation.JunitTests -ne 5 -or
			$producerValidation.JunitFailures -ne 0 -or
			$producerValidation.JunitErrors -ne 0 -or
			$producerValidation.JunitSkipped -ne 0 -or
			$producerValidation.EnvelopeFileCount -ne 40 -or
			$producerValidation.AggregatePolicyAssertionCount -ne 35 -or
			$producerValidation.AggregatePolicyAssertionsPassed -ne 35 -or
			$producerValidation.AggregatePolicyAssertionsFailed -ne 0 -or
			@($producerValidation.CaseRunIds | Sort-Object -Unique).Count -ne 5 -or
			@($producerValidation.CaseEnvelopeSha256s | Sort-Object -Unique).Count -ne 5) {
			throw "Focused producer-shaped schema-2 consumer self-test failed."
		}
		$newCoherentPackageNegative = {
			param(
				[string] $CaseName,
				[ValidateSet("tuple", "digest")]
				[string] $TamperKind
			)

			$producer = Join-Path $focusedFixtureSourceRoot `
				"New-PartisanFocusedAutotestAggregate.ps1"
			$negativeRepositoryRoot = Join-Path $tempRoot `
				("producer-shaped-" + $CaseName + "-repository")
			[void] [IO.Directory]::CreateDirectory($negativeRepositoryRoot)
			$negativeRepository = New-SelfTestRepository `
				-Root $negativeRepositoryRoot
			$negativeProducerPath = Join-Path $negativeRepository.Root `
				"tools/New-PartisanFocusedAutotestAggregate.ps1"
			$negativeProducerSource = [IO.File]::ReadAllText($negativeProducerPath)
			$negativeProducerTokens = $null
			$negativeProducerErrors = $null
			$negativeProducerAst =
				[Management.Automation.Language.Parser]::ParseInput(
					$negativeProducerSource,
					[ref] $negativeProducerTokens,
					[ref] $negativeProducerErrors)
			$packageValidatorMatches = @($negativeProducerAst.FindAll({
					param($node)
					$node -is
						[Management.Automation.Language.FunctionDefinitionAst] -and
						$node.Name -ceq
							"Get-PartisanFocusedCanonicalPackageSha256"
				}, $true))
			if (@($negativeProducerErrors).Count -ne 0 -or
				$packageValidatorMatches.Count -ne 1) {
				throw "Focused consumer self-test cannot isolate the producer package validator."
			}
			$trustDeclaredPackageFunction = @(
				'function Get-PartisanFocusedCanonicalPackageSha256 {',
				'    param(',
				'        [Parameter(Mandatory = $true)]$Package,',
				'        [Parameter(Mandatory = $true)][string]$Label,',
				'        [string]$Code = ''candidate_tampering''',
				'    )',
				'',
				'    return (Require-PartisanFocusedSha256 -Value $Package.sha256 -Label "$Label SHA-256" -Code $Code)',
				'}') -join "`n"
			$packageValidatorExtent = $packageValidatorMatches[0].Extent
			$negativeProducerSource = $negativeProducerSource.Remove(
				$packageValidatorExtent.StartOffset,
				$packageValidatorExtent.EndOffset -
					$packageValidatorExtent.StartOffset).Insert(
						$packageValidatorExtent.StartOffset,
						$trustDeclaredPackageFunction)
			Write-SelfTestText `
				-Path $negativeProducerPath `
				-Text $negativeProducerSource.Replace("`r`n", "`n")

			$negativeManifest = Read-SelfTestJson `
				-Path $negativeRepository.ManifestPath
			$canonicalPackageSha = Get-SelfTestPackageSha256 `
				-Files @($negativeManifest.package.files)
			if ($TamperKind -ceq "tuple") {
				$negativeManifest.package.files[0].indexPath =
					"Partisan/noncanonical-addon.gproj"
				$negativeManifest.package.sha256 = Get-SelfTestPackageSha256 `
					-Files @($negativeManifest.package.files)
			}
			else {
				$falsePackageSha = "d" * 64
				if ($falsePackageSha -ceq $canonicalPackageSha) {
					throw "Focused consumer self-test false package digest is not false."
				}
				$negativeManifest.package.sha256 = $falsePackageSha
			}
			if ([string] $negativeManifest.package.sha256 -ceq
					$canonicalPackageSha) {
				throw "Focused consumer self-test package negative did not change the digest."
			}
			Write-SelfTestJson `
				-Path $negativeRepository.ManifestPath `
				-Value $negativeManifest
			$negativeManifestSha = Get-SelfTestSha256 `
				-Path $negativeRepository.ManifestPath
			$negativeReady = Read-SelfTestJson -Path $negativeRepository.ReadyPath
			$negativeReady.packageSha256 = $negativeManifest.package.sha256
			$negativeReady.manifestSha256 = $negativeManifestSha
			Write-SelfTestJson `
				-Path $negativeRepository.ReadyPath `
				-Value $negativeReady
			$negativeReadySha = Get-SelfTestSha256 `
				-Path $negativeRepository.ReadyPath
			$negativeStatus = Read-SelfTestJson `
				-Path $negativeRepository.StatusPath
			$negativeStatus.artifact.packageSha256 =
				[string] $negativeManifest.package.sha256
			$negativeStatus.artifact.manifestSha256 = $negativeManifestSha
			$negativeStatus.artifact.readySha256 = $negativeReadySha
			Write-SelfTestJson `
				-Path $negativeRepository.StatusPath `
				-Value $negativeStatus

			& git -C $negativeRepository.Root add -- tools docs
			if ($LASTEXITCODE -ne 0) {
				throw "Focused consumer self-test could not stage its package negative."
			}
			& git -C $negativeRepository.Root commit --quiet -m `
				("focused consumer coherent package " + $TamperKind + " negative")
			if ($LASTEXITCODE -ne 0) {
				throw "Focused consumer self-test could not commit its package negative."
			}
			$negativeHead = ((& git -C $negativeRepository.Root rev-parse HEAD) `
				-join '').Trim()
			if ($LASTEXITCODE -ne 0 -or
				$negativeHead -cnotmatch '^[0-9a-f]{40}$') {
				throw "Focused consumer self-test cannot resolve its package-negative HEAD."
			}

			$negativeFixture = New-SelfTestFixture `
				-Root (Join-Path $tempRoot `
					("producer-shaped-" + $CaseName + "-evidence")) `
				-HarnessGitHead $negativeHead `
				-RunnerSha256 (Get-SelfTestSha256 -Path (Join-Path `
					$negativeRepository.Root `
					"tools/run-guarded-focused-autotest.ps1")) `
				-CandidateModuleSha256 (Get-SelfTestSha256 -Path (Join-Path `
					$negativeRepository.Root `
					"tools/Partisan.ReleaseCandidate.psm1")) `
				-TrackedManifestPath $negativeRepository.ManifestPath `
				-TrackedReadyPath $negativeRepository.ReadyPath
			$producer = $negativeProducerPath
			$negativePublication = Invoke-SelfTestProducer `
				-EvidenceRoot $negativeFixture.EvidenceRoot `
				-RunPaths $negativeFixture.RunPaths `
				-OutputPath $negativeRepository.OutputPath `
				-RepositoryRoot $negativeRepository.Root
			if (-not $negativePublication.Succeeded) {
				throw "Focused consumer self-test could not seal its package negative: $($negativePublication.Error)"
			}

			$negativeSummarySnapshot = Read-FileByteSnapshot `
				$negativeRepository.OutputPath `
				"Focused consumer self-test coherent package-negative summary"
			$negativeSummary = (ConvertFrom-FocusedStrictUtf8JsonSnapshot `
				$negativeSummarySnapshot `
				"Focused consumer self-test coherent package-negative summary").Value
			$negativeSummaryRelative =
				"docs/evidence/focused-autotest/{0}.json" -f
					$negativeRepository.CandidateId
			$negativeEvidence = & $newFocusedEvidence `
				-Summary $negativeSummary `
				-SummarySha256 $negativeSummarySnapshot.Sha256 `
				-SummaryPath $negativeSummaryRelative
			$negativeStatus = Read-SelfTestJson `
				-Path $negativeRepository.StatusPath
			$negativeStatus.evidence | Add-Member `
				-MemberType NoteProperty `
				-Name "packagedFocusedAutotest" `
				-Value $negativeEvidence `
				-Force
			Write-SelfTestJson `
				-Path $negativeRepository.StatusPath `
				-Value $negativeStatus
			$sealedNegativeStatus = Read-SelfTestJson `
				-Path $negativeRepository.StatusPath
			$script:root = $negativeRepository.Root
			$negativeIngressMessage = if ($TamperKind -ceq "tuple") {
				"package inventory tuples"
			}
			else {
				"package digest differs"
			}
			& $assertRejected `
				"coherently resealed retained candidate package $TamperKind ingress" {
				Get-RetainedCandidateIdentity `
					$sealedNegativeStatus.artifact `
					"Focused consumer self-test coherently resealed package negative" |
					Out-Null
			} $negativeIngressMessage
			$negativeManifestSource = Get-ObjectPropertyValue `
				$negativeManifest "source"
			$negativeEmbedded = Get-ObjectPropertyValue `
				$negativeManifestSource "embeddedImplementation"
			$negativeCandidateIdentity = [PSCustomObject] @{
				CandidateId = [string] $sealedNegativeStatus.artifact.candidateId
				CandidateSourceHead =
					[string] $sealedNegativeStatus.artifact.candidateSourceHead
				ManifestPath = [string] $sealedNegativeStatus.artifact.manifestPath
				ManifestSha256 =
					[string] $sealedNegativeStatus.artifact.manifestSha256
				ReadySha256 = [string] $sealedNegativeStatus.artifact.readySha256
				PackageHashAlgorithm =
					[string] $sealedNegativeStatus.artifact.packageHashAlgorithm
				PackageSha256 =
					[string] $sealedNegativeStatus.artifact.packageSha256
				PackageVersion =
					[string] $sealedNegativeStatus.artifact.packageVersion
				WorkbenchCrc = [string] $sealedNegativeStatus.artifact.workbenchCrc
				CreatedUtc = Require-UtcTimestamp `
					(Get-ObjectPropertyValue $negativeManifest "createdUtc") `
					"Focused consumer self-test package-negative creation time"
				CampaignSchema = Require-IntegerProperty `
					$negativeManifestSource "campaignSchema" `
					"Focused consumer self-test package-negative source"
				RuntimeSettingsSchema = Require-IntegerProperty `
					$negativeManifestSource "runtimeSettingsSchema" `
					"Focused consumer self-test package-negative source"
				EmbeddedSha = [string] (Get-ObjectPropertyValue `
					$negativeEmbedded "sha")
				EmbeddedUtc = [string] (Get-ObjectPropertyValue `
					$negativeEmbedded "utc")
				EmbeddedLabel = [string] (Get-ObjectPropertyValue `
					$negativeEmbedded "label")
				Manifest = $negativeManifest
			}
			$sealedNegativeEvidence =
				$sealedNegativeStatus.evidence.packagedFocusedAutotest
			if ([string] $negativeCandidateIdentity.PackageSha256 -cne
					[string] $negativeSummary.candidate.packageSha256 -or
				[string] $negativeCandidateIdentity.ManifestSha256 -cne
					[string] $negativeSummary.candidate.manifestSha256 -or
				[string] $negativeCandidateIdentity.ReadySha256 -cne
					[string] $negativeSummary.candidate.readySha256 -or
				[string] $sealedNegativeEvidence.summarySha256 -cne
					[string] $negativeSummarySnapshot.Sha256) {
				throw "Focused consumer self-test package-negative seal chain is incoherent."
			}

			return [PSCustomObject] @{
				Repository = $negativeRepository
				Fixture = $negativeFixture
				CandidateIdentity = $negativeCandidateIdentity
				Evidence = $sealedNegativeEvidence
				SummarySha256 = Get-SelfTestSha256 `
					-Path $negativeRepository.OutputPath
				StatusSha256 = Get-SelfTestSha256 `
					-Path $negativeRepository.StatusPath
			}
		}

		$packageTupleTamper = & $newCoherentPackageNegative `
			-CaseName "package-tuple" `
			-TamperKind "tuple"
		& $assertRejected "coherently resealed package inventory tuple tamper" {
			$script:root = $packageTupleTamper.Repository.Root
			Assert-PackagedFocusedEvidence `
				$packageTupleTamper.Evidence `
				$packageTupleTamper.CandidateIdentity `
				"passed-noncertifying" `
				"focused coherently resealed package inventory tamper" `
				-PortableEvidenceRoot $packageTupleTamper.Fixture.EvidenceRoot `
				-AllowUntrackedSummaryForSelfTest | Out-Null
		} 'package inventory tuples'
		if ((Get-SelfTestSha256 -Path $packageTupleTamper.Repository.OutputPath) -cne
				[string] $packageTupleTamper.SummarySha256 -or
			(Get-SelfTestSha256 -Path $packageTupleTamper.Repository.StatusPath) -cne
				[string] $packageTupleTamper.StatusSha256) {
			throw "Focused consumer tuple rejection changed or published evidence."
		}

		$packageDigestTamper = & $newCoherentPackageNegative `
			-CaseName "false-package-digest" `
			-TamperKind "digest"
		& $assertRejected "coherently resealed false package digest" {
			$script:root = $packageDigestTamper.Repository.Root
			Assert-PackagedFocusedEvidence `
				$packageDigestTamper.Evidence `
				$packageDigestTamper.CandidateIdentity `
				"passed-noncertifying" `
				"focused coherently resealed false package digest" `
				-PortableEvidenceRoot $packageDigestTamper.Fixture.EvidenceRoot `
				-AllowUntrackedSummaryForSelfTest | Out-Null
		} 'package digest differs'
		if ((Get-SelfTestSha256 -Path $packageDigestTamper.Repository.OutputPath) -cne
				[string] $packageDigestTamper.SummarySha256 -or
			(Get-SelfTestSha256 -Path $packageDigestTamper.Repository.StatusPath) -cne
				[string] $packageDigestTamper.StatusSha256) {
			throw "Focused consumer digest rejection changed or published evidence."
		}
		$script:root = $producerRepository.Root

		$firstProfile = $profileOrder[0]
		$firstSuite = [string] $suiteByProfile[$firstProfile]
		$firstRunPath = [string] $producerFixture.RunPaths[0]
		$firstRun = (ConvertFrom-FocusedStrictUtf8JsonSnapshot `
			(Read-FileByteSnapshot $firstRunPath `
				"Focused consumer self-test first run") `
			"Focused consumer self-test first run").Value
		$consoleRows = @($firstRun.files | Where-Object {
			[string] $_.path -cmatch '/console\.log$'
		})
		if ($consoleRows.Count -ne 1) {
			throw "Focused consumer self-test fixture lacks one console row."
		}
		$firstConsolePath = Join-Path (Split-Path -Parent $firstRunPath) `
			([string] $consoleRows[0].path).Replace('/', '\')
		$firstConsoleBytes = [IO.File]::ReadAllBytes($firstConsolePath)
		$firstConsoleText = $utf8.GetString($firstConsoleBytes)
		$firstConsoleLines = @($firstConsoleText -split "`r?`n")
		$markerNeedles = @(
			"TestSuite #$firstSuite started",
			"$firstProfile`: SUCCESS",
			"SCR_TestRunner has finished running",
			"Autotest JUnit XML saved to:",
			"Autotest failed list saved to:")
		foreach ($markerNeedle in $markerNeedles) {
			$markerLines = @($firstConsoleLines | Where-Object {
				([string] $_).Contains($markerNeedle)
			})
			if ($markerLines.Count -ne 1) {
				throw "Focused consumer self-test fixture marker census is not singular."
			}
			$duplicateMarkerText = $firstConsoleText + "`n" +
				[string] $markerLines[0]
			if ((Get-FocusedRawDiagnosticCensus `
					$duplicateMarkerText $firstProfile $firstSuite).Valid) {
				throw "Focused consumer self-test admitted duplicate marker $markerNeedle."
			}
		}
		$foreignMarkerRows = [ordered] @{
			"foreign suite-start marker" =
				"00:09:00.000 SCRIPT : TestSuite #HST_ForeignAutotestSuite started"
			"foreign profile SUCCESS marker" =
				"00:09:01.000 SCRIPT : HST_TEST_ForeignAuthority: SUCCESS"
		}
		foreach ($foreignMarker in $foreignMarkerRows.GetEnumerator()) {
			$foreignMarkerText = $firstConsoleText + "`n" +
				[string] $foreignMarker.Value
			if ((Get-FocusedRawDiagnosticCensus `
					$foreignMarkerText $firstProfile $firstSuite).Valid) {
				throw "Focused consumer self-test admitted $($foreignMarker.Key)."
			}
		}

		try {
			[IO.File]::AppendAllText(
				$firstConsolePath,
				"post-publication raw tamper`n",
				$utf8)
			& $assertRejected "post-publication raw blob tamper" {
				Assert-PackagedFocusedEvidence `
					$producerEvidence $producerCandidateIdentity `
					"passed-noncertifying" "focused raw tamper" `
					-PortableEvidenceRoot $producerFixture.EvidenceRoot `
					-AllowUntrackedSummaryForSelfTest | Out-Null
			} 'indexed focused blob differs from its retained hash'
		}
		finally {
			[IO.File]::WriteAllBytes($firstConsolePath, $firstConsoleBytes)
		}

		$firstRunBytes = [IO.File]::ReadAllBytes($firstRunPath)
		try {
			[IO.File]::AppendAllText($firstRunPath, "`n", $utf8)
			& $assertRejected "post-publication source envelope tamper" {
				Assert-PackagedFocusedEvidence `
					$producerEvidence $producerCandidateIdentity `
					"passed-noncertifying" "focused source envelope tamper" `
					-PortableEvidenceRoot $producerFixture.EvidenceRoot `
					-AllowUntrackedSummaryForSelfTest | Out-Null
			} 'focused run envelope hash differs'
		}
		finally {
			[IO.File]::WriteAllBytes($firstRunPath, $firstRunBytes)
		}

		$publishedSummaryBytes = [IO.File]::ReadAllBytes(
			$producerRepository.OutputPath)
		$publishedSummarySha = [string] $producerEvidence.summarySha256
		try {
			$sourceIndexTamper = (ConvertFrom-FocusedStrictUtf8JsonSnapshot `
				(Read-FileByteSnapshot $producerRepository.OutputPath `
					"Focused consumer self-test source-index summary") `
				"Focused consumer self-test source-index summary").Value
			$sourceIndexTamper.sourceRuns[0].files[0].length =
				[long] $sourceIndexTamper.sourceRuns[0].files[0].length + 1
			Write-SelfTestJson `
				-Path $producerRepository.OutputPath `
				-Value $sourceIndexTamper
			$producerEvidence.summarySha256 = Get-SelfTestSha256 `
				-Path $producerRepository.OutputPath
			& $assertRejected "post-publication source-index tamper" {
				Assert-PackagedFocusedEvidence `
					$producerEvidence $producerCandidateIdentity `
					"passed-noncertifying" "focused source-index tamper" `
					-PortableEvidenceRoot $producerFixture.EvidenceRoot `
					-AllowUntrackedSummaryForSelfTest | Out-Null
			} 'aggregate/raw source row differs'
		}
		finally {
			[IO.File]::WriteAllBytes(
				$producerRepository.OutputPath,
				$publishedSummaryBytes)
			$producerEvidence.summarySha256 = $publishedSummarySha
		}

		$laterAggregatePath = Join-Path $producerRepository.Root `
			"tools/New-PartisanFocusedAutotestAggregate.ps1"
		$laterConsumerPath = Join-Path $producerRepository.Root `
			"tools/update-release-docs.ps1"
		$laterRunnerPath = Join-Path $producerRepository.Root `
			"tools/run-guarded-focused-autotest.ps1"
		$laterCandidateModulePath = Join-Path $producerRepository.Root `
			"tools/Partisan.ReleaseCandidate.psm1"
		[IO.File]::AppendAllText(
			$laterAggregatePath,
			"`n# Legitimate later aggregate maintenance.`n",
			$utf8)
		[IO.File]::AppendAllText(
			$laterConsumerPath,
			"`n# Legitimate later consumer maintenance.`n",
			$utf8)
		[IO.File]::AppendAllText(
			$laterRunnerPath,
			"`n# Legitimate later focused-runner maintenance.`n",
			$utf8)
		[IO.File]::AppendAllText(
			$laterCandidateModulePath,
			"`n# Legitimate later candidate-module maintenance.`n",
			$utf8)
		& git -C $producerRepository.Root add -- `
			"tools/New-PartisanFocusedAutotestAggregate.ps1" `
			"tools/update-release-docs.ps1" `
			"tools/run-guarded-focused-autotest.ps1" `
			"tools/Partisan.ReleaseCandidate.psm1"
		if ($LASTEXITCODE -ne 0) {
			throw "Focused consumer self-test could not stage later legitimate tool edits."
		}
		& git -C $producerRepository.Root commit --quiet -m `
			"focused consumer self-test later tool maintenance"
		if ($LASTEXITCODE -ne 0) {
			throw "Focused consumer self-test could not commit later legitimate tool edits."
		}
		$historicalProducerEvidence = $producerEvidence |
			ConvertTo-Json -Depth 100 | ConvertFrom-Json
		$historicalProducerEvidence.status =
			"historical-passed-noncertifying"
		$historicalProducerValidation = Assert-PackagedFocusedEvidence `
			$historicalProducerEvidence `
			$producerCandidateIdentity `
			"historical-passed-noncertifying" `
			"focused historical evidence after later tool maintenance" `
			-PortableEvidenceRoot $producerFixture.EvidenceRoot `
			-AllowUntrackedSummaryForSelfTest
		if (-not $historicalProducerValidation.PortableSchema -or
			$historicalProducerValidation.CaseCount -ne 5) {
			throw "Focused consumer self-test rejected immutable historical evidence after later tool maintenance."
		}
		& $assertRejected "active focused evidence after later tool maintenance" {
			Assert-PackagedFocusedEvidence `
				$producerEvidence $producerCandidateIdentity `
				"passed-noncertifying" `
				"focused active evidence after later tool maintenance" `
				-PortableEvidenceRoot $producerFixture.EvidenceRoot `
				-AllowUntrackedSummaryForSelfTest | Out-Null
		} 'stationary Git blob'
		$script:root = $originalRoot

		$script:root = $tempRoot
		$routeDirectory = Join-Path $tempRoot "docs/evidence/focused-autotest"
		[void] [IO.Directory]::CreateDirectory($routeDirectory)
		$routePath = Join-Path $routeDirectory "focused-consumer-self-test.json"
		$routeRelative =
			"docs/evidence/focused-autotest/focused-consumer-self-test.json"
		$routeEvidence = [PSCustomObject] @{
			summaryPath = $routeRelative
		}
		$routeCandidate = [PSCustomObject] @{
			CandidateId = "focused-consumer-self-test"
		}
		[IO.File]::WriteAllBytes(
			$routePath,
			$utf8.GetBytes((([PSCustomObject] [ordered] @{
				schemaVersion = 2
				evidenceKind = "packaged-focused-autotest-set"
				aggregationPolicy = [PSCustomObject] @{
					contractId = "unsupported"
				}
			}) | ConvertTo-Json -Depth 10 -Compress)))
		& $assertRejected "schema-2 downgrade" {
			Assert-PackagedFocusedEvidence `
				$routeEvidence $routeCandidate "passed-noncertifying" `
				"focused route" -PortableEvidenceRoot $tempRoot `
				-AllowUntrackedSummaryForSelfTest | Out-Null
		} 'schema-2 focused summary has an unsupported identity'

		[IO.File]::WriteAllBytes(
			$routePath,
			$utf8.GetBytes((([PSCustomObject] [ordered] @{
				schemaVersion = 2
				evidenceKind = "packaged-focused-autotest-set"
				aggregationPolicy = [PSCustomObject] @{
					contractId = "partisan.focused-autotest.aggregate.v2"
				}
			}) | ConvertTo-Json -Depth 10 -Compress)))
		& $assertRejected "schema-2 dispatcher bypass" {
			Assert-PackagedFocusedEvidence `
				$routeEvidence $routeCandidate "passed-noncertifying" `
				"focused route" -PortableEvidenceRoot $tempRoot `
				-AllowUntrackedSummaryForSelfTest | Out-Null
		} 'portable status properties'

		Write-Host "Portable focused-evidence consumer self-test passed."
	}
	finally {
		$script:root = $originalRoot
		if (Test-Path -LiteralPath $tempRoot) {
			$resolvedTemp = [IO.Path]::GetFullPath($tempRoot)
			if ((Test-FocusedContainedPath $tempBase $resolvedTemp) -and
				[IO.Path]::GetFileName($resolvedTemp) -cmatch
					'^partisan-focused-consumer-[0-9a-f]{32}$') {
				Remove-Item -LiteralPath $resolvedTemp -Recurse -Force
			}
		}
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
	$legacyBindings = [ordered] @{
		"partisan-rc-0e632ec4f63e-20260719T004133Z" = [PSCustomObject] @{
			Status = "historical-passed-noncertifying"
			Outcome = "accepted"
			SummaryPath = "docs/evidence/campaign-debug/partisan-rc-0e632ec4f63e-20260719T004133Z-corrected-canary-20260719T012319Z.json"
			SummarySha256 = "f47fa5f0539c0c8c6024e096f3e034699bc6bfaf656734a0a2b32c9fee7b4aa8"
			HarnessGitHead = "20375141f840f74316ca46e7df047fcba3e6e344"
			CampaignRunnerSha256 = "56de386cbdb5677f4315c99e690eefa7158cc6e00e043ded221cec04bde74479"
		}
		"partisan-rc-e11e7ea88a44-20260719T040154Z" = [PSCustomObject] @{
			Status = "historical-failed-proof-validation"
			Outcome = "rejected-proof"
			SummaryPath = "docs/evidence/campaign-debug/partisan-rc-e11e7ea88a44-20260719T040154Z-corrected-canary-20260719T050302Z.json"
			SummarySha256 = "af0aca25a84d8f757dbba8010950a658ce09937aa4048c35b2e372f1183eec69"
			HarnessGitHead = "937c86c5d2259a9da270ea76371001ac1d4c6eed"
			CampaignRunnerSha256 = "56de386cbdb5677f4315c99e690eefa7158cc6e00e043ded221cec04bde74479"
		}
		"partisan-rc-ee0e8add2a29-20260719T063815Z" = [PSCustomObject] @{
			Status = "passed-noncertifying"
			Outcome = "accepted"
			SummaryPath = "docs/evidence/campaign-debug/partisan-rc-ee0e8add2a29-20260719T063815Z-corrected-canary-20260719T071408Z.json"
			SummarySha256 = "f3521fdee20811efd37a260d23498aad43d75435cc01331022ffb8565df34b42"
			HarnessGitHead = "4f8d7e2d7a39896737fd6754060523bf852c5fa8"
			CampaignRunnerSha256 = "56de386cbdb5677f4315c99e690eefa7158cc6e00e043ded221cec04bde74479"
		}
	}
	$legacyCandidateId = [string] $CandidateIdentity.CandidateId
	if (-not $legacyBindings.Contains($legacyCandidateId)) {
		throw "$Label schema-1 corrected canary is not a pinned legacy ledger tuple."
	}
	$legacyBinding = $legacyBindings[$legacyCandidateId]
	$legacyRunnerSha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "campaignRunnerSha256") `
		"$Label.campaignRunnerSha256").ToLowerInvariant()
	if ($ExpectedStatus -cne [string] $legacyBinding.Status -or
		$ExpectedOutcome -cne [string] $legacyBinding.Outcome -or
		$summaryPath -cne [string] $legacyBinding.SummaryPath -or
		$summarySha -cne [string] $legacyBinding.SummarySha256 -or
		$harnessHead -cne [string] $legacyBinding.HarnessGitHead -or
		$legacyRunnerSha -cne [string] $legacyBinding.CampaignRunnerSha256) {
		throw "$Label schema-1 corrected canary differs from its pinned legacy ledger tuple."
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
		LegacySchema1 = $true
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
		[string] $PortableEvidenceRoot,
		[ValidateSet("full_certification", "force_authority")]
		[string] $ContractProfile = "full_certification"
	)
	Assert-ExactPartisanCandidatePackageInventory `
		$CandidateIdentity "$Label retained candidate"
	$isCorrectedCanary = $ContractProfile -ceq "force_authority"
	$expectedProfile = if ($isCorrectedCanary) { "force_authority" } else { "full_certification" }
	$expectedProofScope = if ($isCorrectedCanary) { "focused_force_authority" } else { "full_certification" }
	$expectedIndexEvidenceKind = if ($isCorrectedCanary) {
		"packaged-campaign-debug-corrected-canary"
	}
	else {
		"packaged-campaign-debug-full-profile"
	}
	$expectedPolicyId = if ($isCorrectedCanary) {
		"partisan-campaign-debug-corrected-canary-v2"
	}
	else {
		"partisan-campaign-debug-full-profile-v2"
	}
	$correctedCanaryFocusedAssertionIds = @(
		"combat_presence.aggregate",
		"combat_presence.empty_vehicle",
		"combat_presence.authoritative_samples",
		"combat_presence.rejected_rows",
		"combat_presence.heat_lifecycle",
		"combat_presence.schema62_migration",
		"combat_presence.schema63_restore",
		"combat_presence.malformed_fail_cold",
		"combat_presence.deterministic_diagnostics",
		"ownership_transition.aggregate",
		"ownership_transition.military",
		"ownership_transition.political",
		"ownership_transition.recapture",
		"ownership_transition.replay",
		"ownership_transition.restore",
		"ownership_transition.restore_queue_order",
		"ownership_transition.persistence_deadline",
		"ownership_transition.projection_revision",
		"ownership_transition.location_identity",
		"ownership_transition.linked_support",
		"ownership_transition.causes",
		"ownership_transition.security_fail_closed",
		"ownership_transition.migration_retention",
		"town_influence.aggregate",
		"town_influence.scaling",
		"town_influence.hysteresis",
		"town_influence.idempotency",
		"town_influence.projection",
		"town_influence.population",
		"town_influence.rejection",
		"town_influence.ownership_authority",
		"town_influence.external_completion",
		"town_influence.pre64_invader",
		"town_influence.migration",
		"town_influence.current_restore")
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
	$expectedPortableStatusFields = if ($isCorrectedCanary) {
		@(
			"status", "summaryPath", "summarySha256", "candidateId",
			"candidateSourceHead", "packageSha256", "manifestSha256", "readySha256",
			"settingsSha256", "harnessGitHead", "campaignRunnerSha256",
			"candidateModuleSha256", "runLeafId", "runId", "startedUtc",
			"completedUtc", "runtimeSeconds", "outcomeSuccess", "error",
			"caseCount", "pass", "warn", "fail", "blocked", "skipped",
			"focusedAssertionCount", "focusedAssertionsPassed",
			"certificationRequired", "certificationProven", "stateDiffRows",
			"nonzeroStateDiffRows", "finalOrphanCleanupPass",
			"diagnosticClassificationValid", "hardDiagnosticFree",
			"hardDiagnosticCount", "scriptErrors", "engineErrors", "partisanErrors",
			"approvedStockDiagnosticCount", "approvedIntentionalDiagnosticCount",
			"unapprovedHardDiagnosticCount", "unapprovedHardDiagnosticKind",
			"classifierSelfTestCount", "canonicalScriptLogCount",
			"canonicalConsoleLogCount", "canonicalLogPairSameDirectory",
			"cleanupAndSpillZero", "envelopeFileCount", "envelopeFilesRehashed",
			"envelopeSha256", "runSummarySha256", "acceptanceDisposition", "summary")
	}
	else {
		@(
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
	}
	if ($null -eq $Evidence -or $Evidence -is [string] -or $Evidence -is [Array]) {
		throw "$Label portable status must be an object."
	}
	Assert-EqualSet $expectedPortableStatusFields `
		@($Evidence.PSObject.Properties.Name) "$Label portable status fields"

	$allowedStatuses = if ($isCorrectedCanary) {
		@("passed-noncertifying", "failed-corrected-canary")
	}
	else {
		@(
			"passed-full-certification",
			"passed-internal-profile-external-required",
			"failed-full-profile")
	}
	$statusValue = Require-Text `
		(Get-ObjectPropertyValue $Evidence "status") "$Label.status"
	if ($statusValue -cnotin $allowedStatuses) {
		throw "$Label.status is not a portable $ContractProfile disposition."
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
	$indexSnapshot = Read-FileByteSnapshot $indexPath "$Label portable release index"
	$actualIndexSha = [string] $indexSnapshot.Sha256
	if ($actualIndexSha -cne $summarySha) {
		throw "$Label portable release-index SHA-256 does not match release status."
	}
	$indexText = Get-StrictUtf8Text `
		([byte[]] $indexSnapshot.Bytes) "$Label portable release index"
	Assert-JsonObjectPropertiesUnique $indexText "$Label portable release index"
	try {
		$index = $indexText | ConvertFrom-Json
	}
	catch {
		throw "$Label portable release index is invalid JSON: $($_.Exception.Message)"
	}
	Assert-NoLocalAbsolutePathValue $index "$Label portable release index"
	if ((Require-IntegerProperty $index "schemaVersion" "$Label index") -ne 2 -or
		[string] (Get-ObjectPropertyValue $index "evidenceKind") -cne
			$expectedIndexEvidenceKind -or
		[string] (Get-ObjectPropertyValue $index "policyId") -cne
			$expectedPolicyId) {
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
	Assert-ExactObjectProperties $index @(
		"schemaVersion", "evidenceKind", "policyId", "source", "candidate",
		"harness", "settings", "capture", "result", "proof", "diagnostics",
		"cleanup", "integrity", "finding") "$Label index"
	Assert-ExactObjectProperties $source @(
		"bundleRelativePath", "runEnvelopePath", "runEnvelopeSha256",
		"rawArtifactPath", "rawArtifactSha256", "stateDiffPath",
		"stateDiffSha256", "textSummaryPath", "textSummarySha256",
		"fileCount", "files", "filesRehashed") "$Label index.source"
	Assert-ExactObjectProperties $indexCandidate @(
		"candidateId", "candidateVersion", "candidateSourceHead",
		"embeddedBuildSha", "embeddedBuildUtc", "embeddedBuildLabel",
		"campaignSchema", "runtimeSettingsSchema", "addonId", "addonGuid",
		"packageHashAlgorithm", "packageSha256", "manifestSha256", "readySha256",
		"workbenchCrc", "runtimeUseDispositionAtCapture", "runtimeRole",
		"diagnosticExecutable", "recordedDiagnosticExecutable",
		"recordedRuntimeExecutable") "$Label index.candidate"
	Assert-ExactObjectProperties $indexHarness @(
		"gitHead", "dirty", "campaignRunnerSha256",
		"campaignRunnerGitBlobSha256", "candidateModuleSha256",
		"candidateModuleGitBlobSha256", "releaseIndexProducerSha256",
		"releaseIndexProducerGitBlobSha256", "releaseDocsConsumerSha256",
		"releaseDocsConsumerGitBlobSha256") "$Label index.harness"
	Assert-ExactObjectProperties $indexSettings @(
		"schemaVersion", "sha256", "guardedRuntimeCopy") "$Label index.settings"
	Assert-ExactObjectProperties $capture @(
		"runLeafId", "runId", "profile", "proofScope", "startedUtc",
		"completedUtc", "runtimeSeconds") "$Label index.capture"
	Assert-ExactObjectProperties $result @(
		"status", "acceptanceDisposition", "releaseDisposition",
		"wrapperCaptureSuccess", "guardedRunSucceeded", "runtimeOutcomeSuccess",
		"armed", "started", "completed", "candidateBoundaryVerified",
		"mountPacked", "artifactsStable", "evidenceCaptured",
		"artifactSchemaValidationValid", "certificationPassed", "error") `
		"$Label index.result"
	Assert-ExactObjectProperties $indexCleanup @(
		"guardRemaining", "ownedProcessesRemaining", "newEngineProcessesRemaining",
		"unclaimedEngineProcessesObserved", "newDefaultEntriesRemaining",
		"modifiedDefaultFiles", "deletedDefaultEntries", "missingDefaultRoots",
		"externalSpillEntriesRemaining", "modifiedSpillFiles", "deletedSpillEntries",
		"missingSpillRoots", "cleanupPhaseErrorCount", "cleanupPhaseErrors",
		"monitoringRootsAreDetectionOnly") "$Label index.cleanup"
	Assert-ExactObjectProperties $integrity @(
		"envelopeSha256", "runSummarySha256", "rawArtifactSha256",
		"envelopeFileCount", "envelopeFilesRehashed", "releaseIndexProducerSha256",
		"releaseDocsConsumerSha256") "$Label index.integrity"
	Assert-ExactObjectProperties $finding @(
		"status", "defect", "nextStep") "$Label index.finding"
	$expectedProofFields = @(
		"startedAtSecond", "endedAtSecond", "caseCount", "pass", "warn", "fail",
		"blocked", "skipped", "certificationRequired", "certificationProven",
		"certificationFail", "certificationBlocked", "certificationWarn",
		"assertionCount", "stateDiffRows", "nonzeroStateDiffRows",
		"failedAssertionIds", "warningAssertionIds", "warningAssertions",
		"unsupportedWarningAssertionIds",
		"skippedAssertionIds", "approvedNoncertifyingSkipIds",
		"unsupportedSkippedAssertionIds", "externalRequiredAdvisoryIds",
		"externalRequiredAdvisories", "blockedAssertions",
		"intentionalMissionConvoyAdmissionDiagnosticsProven",
		"intentionalMissionConvoySettlementDiagnosticProven",
		"intentionalMissionConvoyCorruptionDiagnosticsProven",
		"intentionalMissionConvoyWatchdogDiagnosticProven",
		"finalOrphanCleanupPass", "finalOrphanActiveGroups")
	if ($isCorrectedCanary) {
		$expectedProofFields += @(
			"focusedCaseId", "focusedCaseStatus", "focusedAssertionCount",
			"focusedAssertionsPassed", "focusedAssertionSetExact",
			"focusedAssertionsCertificationExact", "correctedCanaryCaseSetExact",
			"correctedCanaryWarningContractExact",
			"correctedCanaryBlockedContractExact", "correctedCanaryAssertionSkipFree",
			"correctedCanaryAssertionManifestExact",
			"correctedCanaryStateDiffManifestExact",
			"correctedCanaryOrphanContractExact", "correctedCanaryProofAxisPassed")
	}
	Assert-ExactObjectProperties $proof $expectedProofFields "$Label index.proof"
	Assert-ExactObjectProperties $diagnostics @(
		"valid", "classificationValid", "hardDiagnosticFree", "hardDiagnosticCount",
		"scriptErrors", "engineErrors", "partisanErrors", "crashMarkers",
		"partisanSeverityLineCount", "approvedStockDiagnosticCount",
		"approvedIntentionalDiagnosticCount", "unapprovedHardDiagnosticCount",
		"unapprovedHardDiagnosticKinds", "classifierSelfTestCount",
		"malformedHardDiagnosticCount", "channelArithmeticValid",
		"categoryArithmeticValid", "lifecycleMarkersValid", "identityBaselinePairValid",
		"intentionalFixtureStructureExact", "intentionalFixtureSetValid",
		"canonicalScriptLogCount", "canonicalConsoleLogCount",
		"canonicalErrorLogCount", "canonicalCrashLogCount",
		"canonicalLogPairSameDirectory", "auxiliaryLogPairSameDirectory",
		"auxiliaryDiagnosticsValid", "errorLogProjectionExact",
		"crashLogProjectionExact", "auxiliaryUnapprovedEventCount") `
		"$Label index.diagnostics"
	foreach ($row in @(Get-ObjectPropertyValue $proof "warningAssertions")) {
		Assert-ExactObjectProperties $row @(
			"id", "caseId", "category", "feature", "stage", "expected", "actual",
			"reason", "proofLevel", "observedPath", "requiredPath",
			"countsTowardCertification") "$Label index warning assertion row"
	}
	foreach ($row in @(Get-ObjectPropertyValue $proof "externalRequiredAdvisories")) {
		Assert-ExactObjectProperties $row @(
			"id", "caseId", "category", "feature", "stage", "expected", "actual",
			"reason", "proofLevel", "observedPath", "requiredPath",
			"countsTowardCertification") "$Label index external advisory row"
	}
	foreach ($row in @(Get-ObjectPropertyValue $proof "blockedAssertions")) {
		Assert-ExactObjectProperties $row @(
			"id", "caseId", "category", "feature", "stage", "expected", "actual",
			"reason", "proofLevel", "observedPath", "requiredPath",
			"countsTowardCertification") "$Label index blocked assertion row"
	}
	foreach ($row in @(Get-ObjectPropertyValue $diagnostics `
			"unapprovedHardDiagnosticKinds")) {
		Assert-ExactObjectProperties $row @("kind", "count") `
			"$Label index diagnostic-kind row"
	}
	foreach ($executableBinding in @(
			@($indexCandidate, "diagnosticExecutable"),
			@($indexCandidate, "recordedDiagnosticExecutable"),
			@($indexCandidate, "recordedRuntimeExecutable"))) {
		Assert-ExactObjectProperties `
			(Get-ObjectPropertyValue $executableBinding[0] $executableBinding[1]) `
			@("fileName", "fileVersion", "productVersion", "length", "sha256") `
			"$Label index.candidate.$($executableBinding[1])"
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
	$externalIndexSnapshot = $indexSnapshot
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
		$externalIndexSnapshot = Read-FileByteSnapshot `
			$externalIndexPath "$Label retained portable-bundle index"
		if ([string] $externalIndexSnapshot.Sha256 -cne $summarySha) {
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
	$runSnapshot = Read-FileByteSnapshot $runPath "$Label retained run envelope"
	$runSha = [string] $runSnapshot.Sha256
	if ($runSha -cne [string] (Get-ObjectPropertyValue $source "runEnvelopeSha256") -or
		$runSha -cne [string] (Get-ObjectPropertyValue $integrity "envelopeSha256") -or
		$runSha -cne [string] (Get-ObjectPropertyValue $integrity "runSummarySha256") -or
		$runSha -cne [string] (Get-ObjectPropertyValue $Evidence "envelopeSha256") -or
		$runSha -cne [string] (Get-ObjectPropertyValue $Evidence "runSummarySha256")) {
		throw "$Label retained run-envelope hashes differ."
	}
	$runText = Get-StrictUtf8Text `
		([byte[]] $runSnapshot.Bytes) "$Label retained run envelope"
	Assert-JsonObjectPropertiesUnique $runText "$Label retained run envelope"
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
	Assert-ExactObjectProperties $run @(
		"schemaVersion", "evidenceKind", "startedUtc", "completedUtc", "candidate",
		"harness", "launch", "outcome", "settings", "cleanup", "files") `
		"$Label run envelope"
	Assert-ExactObjectProperties $runCandidate @(
		"candidateId", "candidateVersion", "runtimeUseDisposition", "gitHead",
		"embeddedBuildSha", "embeddedBuildUtc", "embeddedBuildLabel",
		"campaignSchema", "runtimeSettingsSchema", "addonId", "addonGuid",
		"packageHashAlgorithm", "packageSha256", "manifestSha256", "readySha256",
		"workbenchCrc", "runtimeRole", "diagnosticExecutable",
		"recordedDiagnosticExecutable", "recordedRuntimeExecutable") `
		"$Label run.candidate"
	Assert-ExactObjectProperties $runHarness @(
		"gitHead", "dirty", "campaignRunnerSha256",
		"campaignRunnerGitBlobSha256", "candidateModuleSha256",
		"candidateModuleGitBlobSha256", "releaseIndexProducerSha256",
		"releaseIndexProducerGitBlobSha256", "releaseDocsConsumerSha256",
		"releaseDocsConsumerGitBlobSha256") "$Label run.harness"
	Assert-ExactObjectProperties $runLaunch @(
		"profile", "proofScope", "worldResource", "stagedPackage",
		"addonSearchRootCount", "addonGuid", "packageSha256",
		"diagnosticExecutable", "recordedRuntimeExecutable") "$Label run.launch"
	Assert-ExactObjectProperties $runOutcome @(
		"success", "armed", "started", "completed", "candidateBoundaryVerified",
		"mountAttestation", "artifactsStable", "evidenceCaptured",
		"hardDiagnosticClassifierChecks", "runtimeSeconds", "error", "validation",
		"errorCensus") "$Label run.outcome"
	Assert-ExactObjectProperties $runSettings @(
		"schemaVersion", "sha256", "guardedRuntimeCopy") "$Label run.settings"
	Assert-ExactObjectProperties $runCleanup @(
		"guardRemaining", "ownedProcessesRemaining", "newEngineProcessesRemaining",
		"unclaimedEngineProcessesObserved", "newDefaultEntriesRemaining",
		"modifiedDefaultFiles", "deletedDefaultEntries", "missingDefaultRoots",
		"externalSpillEntriesRemaining", "modifiedSpillFiles", "deletedSpillEntries",
		"missingSpillRoots", "cleanupPhaseErrorCount", "cleanupPhaseErrors",
		"monitoringRootsAreDetectionOnly") "$Label run.cleanup"
	foreach ($executableBinding in @(
			@($runCandidate, "diagnosticExecutable"),
			@($runCandidate, "recordedDiagnosticExecutable"),
			@($runCandidate, "recordedRuntimeExecutable"),
			@($runLaunch, "diagnosticExecutable"),
			@($runLaunch, "recordedRuntimeExecutable"))) {
		Assert-ExactObjectProperties `
			(Get-ObjectPropertyValue $executableBinding[0] $executableBinding[1]) `
			@("fileName", "fileVersion", "productVersion", "length", "sha256") `
			"$Label run executable $($executableBinding[1])"
	}
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
	if (-not $AllowUntrackedSummaryForSelfTest) {
		$checkoutHeadRows = @(& git -C $root rev-parse HEAD 2>$null)
		$checkoutHeadAtConsumption = ($checkoutHeadRows -join '').Trim()
		if ($LASTEXITCODE -ne 0 -or
			$checkoutHeadAtConsumption -cnotmatch '^[0-9a-f]{40}$') {
			throw "$Label cannot resolve the trusted checkout HEAD before loading its semantic runner."
		}
		Assert-GitAncestor `
			([string] $CandidateIdentity.CandidateSourceHead) `
			$semanticHarnessHead `
			"$Label semantic harness is not descended from its candidate source"
		Assert-GitAncestor `
			$semanticHarnessHead `
			$checkoutHeadAtConsumption `
			"$Label semantic harness is not an ancestor of checkout HEAD"
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
		$selfTestCheckoutHeadRows = @(& git -C $root rev-parse HEAD 2>$null)
		$selfTestHeadExitCode = $LASTEXITCODE
		$selfTestCheckoutHead = ($selfTestCheckoutHeadRows -join '').Trim()
		$selfTestRunnerPath = Join-Path $PSScriptRoot 'run-guarded-campaign-debug.ps1'
		$selfTestRunnerSha = (Get-FileHash `
			-LiteralPath $selfTestRunnerPath `
			-Algorithm SHA256).Hash.ToLowerInvariant()
		if ($selfTestHeadExitCode -ne 0 -or
			$selfTestCheckoutHead -cne $semanticHarnessHead -or
			$selfTestRunnerSha -cne $semanticRunnerBlobSha) {
			throw "$Label self-test semantic runner is not the stationary committed harness blob."
		}
		$runnerSourceText = Get-Content -Raw -LiteralPath $selfTestRunnerPath
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

	$tempBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar)
	$tempPrefix = $tempBase + [IO.Path]::DirectorySeparatorChar
	$validationSnapshotRoot = [IO.Path]::GetFullPath((Join-Path $tempBase `
		("PartisanCampaignDebugConsumer-" + [Guid]::NewGuid().ToString("N"))))
	if (-not $validationSnapshotRoot.StartsWith(
			$tempPrefix,
			[StringComparison]::OrdinalIgnoreCase) -or
		(Split-Path -Leaf $validationSnapshotRoot) -cnotlike
			"PartisanCampaignDebugConsumer-*") {
		throw "$Label validation-snapshot containment failed."
	}
	$validationSnapshotCreated = $false
	try {
		New-Item -ItemType Directory -Path $validationSnapshotRoot -Force | Out-Null
		$validationSnapshotCreated = $true
		$validationSnapshotItem = Get-Item -LiteralPath $validationSnapshotRoot -Force
		if (($validationSnapshotItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
			throw "$Label validation-snapshot root must not be a reparse point."
		}

	$rowPaths = @()
	$rowMap = @{}
	for ($rowIndex = 0; $rowIndex -lt $runFiles.Count; $rowIndex++) {
		$row = $runFiles[$rowIndex]
		$indexRow = $indexFiles[$rowIndex]
		Assert-ExactObjectProperties $row @("path", "length", "sha256") `
			"$Label run.files[$rowIndex]"
		Assert-ExactObjectProperties $indexRow @("path", "length", "sha256") `
			"$Label index.source.files[$rowIndex]"
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
		$fileSnapshot = Read-FileByteSnapshot `
			$fullPath "$Label retained raw file $path"
		if ([long] $fileSnapshot.Length -ne $length -or
			[string] $fileSnapshot.Sha256 -cne $sha) {
			throw "$Label retained raw file $path fails its independent hash/length check."
		}
		$snapshotFullPath = [IO.Path]::GetFullPath(
			(Join-Path $validationSnapshotRoot $path))
		$validationSnapshotPrefix = $validationSnapshotRoot.TrimEnd('\', '/') +
			[IO.Path]::DirectorySeparatorChar
		if (-not $snapshotFullPath.StartsWith(
				$validationSnapshotPrefix,
				[StringComparison]::OrdinalIgnoreCase)) {
			throw "$Label validation-snapshot file $path escapes its root."
		}
		$snapshotParent = Split-Path -Parent $snapshotFullPath
		New-Item -ItemType Directory -Path $snapshotParent -Force | Out-Null
		[IO.File]::WriteAllBytes($snapshotFullPath, [byte[]] $fileSnapshot.Bytes)
		$snapshotCopy = Read-FileByteSnapshot `
			$snapshotFullPath "$Label private validation copy $path"
		if ([long] $snapshotCopy.Length -ne $length -or
			[string] $snapshotCopy.Sha256 -cne $sha) {
			throw "$Label private validation copy $path differs from retained evidence."
		}
		$rowPaths += $path
		$rowMap[$path] = [PSCustomObject] @{
			FullPath = $snapshotFullPath
			OriginalSnapshot = $fileSnapshot
			Length = $length
			Sha256 = $sha
		}
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
		$bundleManifestText = Get-StrictUtf8Text `
			([byte[]] $rowMap[$manifestPaths[0]].OriginalSnapshot.Bytes) `
			"$Label retained candidate manifest"
		$bundleReadyText = Get-StrictUtf8Text `
			([byte[]] $rowMap[$readyPaths[0]].OriginalSnapshot.Bytes) `
			"$Label retained candidate ready seal"
		Assert-JsonObjectPropertiesUnique $bundleManifestText `
			"$Label retained candidate manifest"
		Assert-JsonObjectPropertiesUnique $bundleReadyText `
			"$Label retained candidate ready seal"
		$bundleManifest = $bundleManifestText | ConvertFrom-Json
		$bundleReady = $bundleReadyText | ConvertFrom-Json
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
	$bundlePackageIdentity = [PSCustomObject] @{
		Manifest = $bundleManifest
		PackageSha256 = $bundlePackageSha
	}
	Assert-ExactPartisanCandidatePackageInventory `
		$bundlePackageIdentity "$Label retained bundle candidate"
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
		$rawText = Get-StrictUtf8Text `
			([byte[]] $rowMap[$rawArtifactPath].OriginalSnapshot.Bytes) `
			"$Label raw $ContractProfile artifact"
		Assert-JsonObjectPropertiesUnique $rawText `
			"$Label raw $ContractProfile artifact"
		$raw = $rawText | ConvertFrom-Json
	}
	catch {
		throw "$Label raw $ContractProfile artifact is invalid JSON: $($_.Exception.Message)"
	}
	$runId = Require-Text (Get-ObjectPropertyValue $raw "m_sRunId") "$Label raw run ID"
	if ($isCorrectedCanary -and
		([string] (Get-ObjectPropertyValue $capture "profile") -cne $expectedProfile -or
			[string] (Get-ObjectPropertyValue $capture "proofScope") -cne
				$expectedProofScope)) {
		throw "$Label retained corrected-canary capture identity differs."
	}
	if ($runId -cnotmatch '^seed\d+_t\d+_p\d+_u\d+$' -or
		$rawArtifactPath -cne "raw/campaign-debug/HST_CampaignDebug_$runId.json" -or
		[string] (Get-ObjectPropertyValue $capture "runId") -cne $runId -or
		[string] (Get-ObjectPropertyValue $runLaunch "profile") -cne $expectedProfile -or
		[string] (Get-ObjectPropertyValue $runLaunch "proofScope") -cne
			$expectedProofScope -or
		[string] (Get-ObjectPropertyValue $raw "m_sProfile") -cne $expectedProfile) {
		throw "$Label retained raw/run/capture $ContractProfile identity differs."
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
	$rawAssertionCount = 0
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
			$rawAssertionCount++
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
	$certificationCountsPassed = $requiredAssertions -eq $certCounts.PASS -and
		$certCounts.WARN -eq 0 -and $certCounts.FAIL -eq 0 -and
		$certCounts.BLOCKED -eq 0
	$certificationPassed = if ($isCorrectedCanary) { $false } else { $certificationCountsPassed }
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
		-GuardRoot (Join-Path $validationSnapshotRoot 'raw') `
		-ExpectedSha $bundleEmbeddedBuildSha `
		-ExpectedUtc $bundleEmbeddedBuildUtc `
		-ExpectedLabel $bundleEmbeddedBuildLabel `
		-ExpectedProfile $expectedProfile
	$derivedValidation = $semanticValidation.ArtifactValidation
	$derivedErrorCensus = $semanticValidation.ErrorCensus
	$recordedValidation = Get-ObjectPropertyValue $runOutcome "validation"
	Assert-ExactObjectProperties $recordedValidation @(
		"Valid", "Problems", "RunId", "Profile", "ProofScope", "FullCertification",
		"BuildProvenanceMatches", "StartedAtSecond", "EndedAtSecond", "CaseCount",
		"Pass", "Warn", "Fail", "Blocked", "Skipped", "CertificationRequired",
		"CertificationProven", "CertificationFail", "CertificationBlocked",
		"CertificationWarn", "CertificationPassed", "CorrectedCanaryContract",
		"Trigger", "ArtifactCount", "StateDiffRows", "NonzeroStateDiffRows",
		"StateDiffManifestExact", "Phase17", "Phase17Metrics", "Phase24",
		"Phase24Metrics", "StagedCleanup", "FocusedCaseId", "FocusedCaseStatus",
		"FocusedAssertions", "CorrectedCanaryAssertionManifestExact",
		"CorrectedCanaryCaseSetExact",
		"CorrectedCanaryWarningContractExact", "CorrectedCanaryBlockedContractExact",
		"CorrectedCanaryOrphanContractExact",
		"IntentionalMissionConvoyAdmissionDiagnosticsProven",
		"IntentionalMissionConvoySettlementDiagnosticProven",
		"IntentionalMissionConvoyCorruptionDiagnosticsProven",
		"IntentionalMissionConvoyWatchdogDiagnosticProven",
		"FinalOrphanCleanupPass", "FinalOrphanActiveGroups") `
		"$Label run.outcome.validation"
	foreach ($row in @(Get-ObjectPropertyValue $recordedValidation "Phase17")) {
		Assert-ExactObjectProperties $row @("Id", "Pass") `
			"$Label run validation Phase17 row"
	}
	foreach ($row in @(Get-ObjectPropertyValue $recordedValidation "Phase24")) {
		Assert-ExactObjectProperties $row @("Id", "Pass", "Accepted", "Status", "Actual") `
			"$Label run validation Phase24 row"
	}
	foreach ($row in @(Get-ObjectPropertyValue $recordedValidation "StagedCleanup")) {
		Assert-ExactObjectProperties $row @(
			"Id", "Pass", "CaseStatus", "ActiveGroupsStatus",
			"RuntimeFactionsStatus", "RuntimeGroupPopulationSettledStatus",
			"ExpectedZeroMemberGraceApplied", "OrphanActiveGroups",
			"RuntimeFactionMismatches", "ZeroMemberGraceCandidates",
			"PendingPopulationGroups") "$Label run validation staged-cleanup row"
	}
	foreach ($row in @(Get-ObjectPropertyValue $recordedValidation "FocusedAssertions")) {
		Assert-ExactObjectProperties $row @("Id", "Pass", "Status", "Actual") `
			"$Label run validation focused-assertion row"
	}
	$recordedPhase17Metrics = Get-ObjectPropertyValue $recordedValidation "Phase17Metrics"
	$recordedPhase24Metrics = Get-ObjectPropertyValue $recordedValidation "Phase24Metrics"
	Assert-ExactObjectProperties $recordedPhase17Metrics `
		@((Get-ObjectPropertyValue $derivedValidation "Phase17Metrics").PSObject.Properties.Name) `
		"$Label run validation Phase17 metrics"
	Assert-ExactObjectProperties $recordedPhase24Metrics `
		@((Get-ObjectPropertyValue $derivedValidation "Phase24Metrics").PSObject.Properties.Name) `
		"$Label run validation Phase24 metrics"
	$recordedValidationValue = Get-ObjectPropertyValue $recordedValidation "Valid"
	if ($recordedValidationValue -isnot [bool] -or
		[bool] $recordedValidationValue -ne [bool] $derivedValidation.Valid) {
		throw "$Label recorded artifact-validation disposition differs from semantic re-derivation."
	}
	$validationBooleanFields = @(
		"FullCertification", "CertificationPassed", "StateDiffManifestExact",
		"FinalOrphanCleanupPass")
	if ($isCorrectedCanary) {
		$validationBooleanFields += @(
			"CorrectedCanaryContract", "CorrectedCanaryAssertionManifestExact",
			"CorrectedCanaryCaseSetExact", "CorrectedCanaryWarningContractExact",
			"CorrectedCanaryBlockedContractExact", "CorrectedCanaryOrphanContractExact")
	}
	foreach ($field in $validationBooleanFields) {
		$recordedValue = Get-ObjectPropertyValue $recordedValidation $field
		$derivedValue = Get-ObjectPropertyValue $derivedValidation $field
		if ($recordedValue -isnot [bool] -or $derivedValue -isnot [bool] -or
			[bool] $recordedValue -ne [bool] $derivedValue) {
			throw "$Label recorded artifact-validation $field differs from semantic re-derivation."
		}
	}
	Assert-EqualSet `
		@((Get-ObjectPropertyValue $recordedValidation "Problems")) `
		@($derivedValidation.Problems) `
		"$Label recorded/re-derived artifact-validation problems"
	$validation = $derivedValidation
	$artifactValidationValid = [bool] $derivedValidation.Valid
	$expectedFullCertification = -not $isCorrectedCanary
	if (
		[string] (Get-ObjectPropertyValue $validation "RunId") -cne $runId -or
		[string] (Get-ObjectPropertyValue $validation "Profile") -cne
			$expectedProfile -or
		[string] (Get-ObjectPropertyValue $validation "ProofScope") -cne
			$expectedProofScope -or
		(Get-ObjectPropertyValue $validation "FullCertification") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $validation "FullCertification") -ne
			$expectedFullCertification) {
		throw "$Label re-derived guarded-runner artifact validation identity is inconsistent."
	}
	if ($isCorrectedCanary -and
		((Get-ObjectPropertyValue $validation "CorrectedCanaryContract") -isnot [bool] -or
			-not [bool] (Get-ObjectPropertyValue $validation "CorrectedCanaryContract"))) {
		throw "$Label re-derived guarded-runner validation did not prove the corrected-canary contract."
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
		assertionCount = $rawAssertionCount
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

	$diffText = Get-StrictUtf8Text `
		([byte[]] $rowMap[$diffPaths[0]].OriginalSnapshot.Bytes) `
		"$Label retained state diff"
	$stateDiffValidation = Assert-ExactCampaignDebugStateDiff `
		-Text $diffText `
		-RunId $runId `
		-Label "$Label retained state diff"
	$deltaMatches = @($stateDiffValidation.Rows)
	$nonzeroDeltaCount = [int] $stateDiffValidation.NonzeroRowCount
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

	$focusedCaseId = ""
	$focusedCaseStatus = ""
	$focusedAssertionCount = 0
	$focusedAssertionsPassed = 0
	$focusedAssertionSetExact = $false
	$focusedAssertionsCertificationExact = $false
	$correctedCanaryCaseSetExact = $false
	$correctedCanaryWarningContractExact = $false
	$correctedCanaryBlockedContractExact = $false
	$correctedCanaryAssertionSkipFree = $false
	$correctedCanaryAssertionManifestExact = $false
	$correctedCanaryStateDiffManifestExact = $false
	$correctedCanaryOrphanContractExact = $false
	$correctedCanaryProofAxisPassed = $false
	if ($isCorrectedCanary) {
		$focusedCases = @($cases | Where-Object {
			[string] (Get-ObjectPropertyValue $_ "m_sCaseId") -ceq
				"early_mechanics.force_authority"
		})
		if ($focusedCases.Count -eq 1) {
			$focusedCaseId = [string] (Get-ObjectPropertyValue `
				$focusedCases[0] "m_sCaseId")
			$focusedCaseStatus = [string] (Get-ObjectPropertyValue `
				$focusedCases[0] "m_sStatus")
			$focusedAssertions = @(Get-ObjectPropertyValue `
				$focusedCases[0] "m_aAssertions")
			$focusedAssertionCount = $focusedAssertions.Count
			$focusedAssertionsPassed = @($focusedAssertions | Where-Object {
				[string] (Get-ObjectPropertyValue $_ "m_sStatus") -ceq "PASS"
			}).Count
			$focusedAssertionIds = @($focusedAssertions | ForEach-Object {
				[string] (Get-ObjectPropertyValue $_ "m_sAssertionId")
			})
			try {
				Assert-EqualSet $correctedCanaryFocusedAssertionIds `
					$focusedAssertionIds "$Label corrected-canary focused assertion set"
				$focusedAssertionSetExact = $true
			}
			catch {
				$focusedAssertionSetExact = $false
			}
			$focusedNoncertifyingAssertions = @($focusedAssertions | Where-Object {
				(Get-ObjectPropertyValue $_ "m_bCountsTowardCertification") -isnot [bool] -or
				-not [bool] (Get-ObjectPropertyValue $_ "m_bCountsTowardCertification")
			})
			$focusedAssertionsCertificationExact =
				$focusedNoncertifyingAssertions.Count -eq 1 -and
				[string] (Get-ObjectPropertyValue `
					$focusedNoncertifyingAssertions[0] "m_sAssertionId") -ceq
					"town_influence.external_completion"
		}

		$expectedCorrectedCanaryCaseIds = @(
			"preflight.state_isolation",
			"post_case_cleanup.preflight_state_isolation",
			"cleanup.prefixed_state.start_preflight.hst_debug_",
			"early_mechanics.force_authority",
			"post_case_cleanup.early_mechanics_force_authority",
			"cleanup.enemy_orders.run_completion",
			"cleanup.prefixed_state.run_completion.hst_debug_$runId",
			"cleanup.prefixed_state.run_completion_persistence_smoke_cleanup.hst_smoke",
			"cleanup.player_marker_completion",
			"cleanup.run_leak_snapshot",
			"cleanup.state_isolation_restore")
		try {
			Assert-EqualSet $expectedCorrectedCanaryCaseIds `
				@($cases | ForEach-Object {
					[string] (Get-ObjectPropertyValue $_ "m_sCaseId")
				}) "$Label corrected-canary case set"
			$correctedCanaryCaseSetExact = $true
		}
		catch {
			$correctedCanaryCaseSetExact = $false
		}
		$correctedCanaryCaseSetExact = $correctedCanaryCaseSetExact -and
			(Get-ObjectPropertyValue $validation "CorrectedCanaryCaseSetExact") -is [bool] -and
			[bool] (Get-ObjectPropertyValue $validation "CorrectedCanaryCaseSetExact")

		$playerMarkerWarnings = @($warningRows | Where-Object {
			$_.id -ceq "cleanup.player_marker.live" -and
			$_.caseId -ceq "cleanup.player_marker_completion" -and
			$_.category -ceq "cleanup" -and
			$_.feature -ceq "player_markers" -and
			$_.stage -ceq "final" -and
			$_.expected -ceq
				"enabled player marker service has desired/tracked/live marker after cleanup" -and
			$_.actual -cmatch
				'^enabled [01] \| desired \d+ \| tracked \d+ \| live \d+ \| entry [01]$' -and
			$_.reason -ceq
				"player marker did not reconcile after campaign debug completion" -and
			$_.proofLevel -ceq "STATE_ONLY" -and
			$_.observedPath -ceq "diagnostic_only" -and
			$_.requiredPath -ceq "no debug-owned state or world leak" -and
			-not $_.countsTowardCertification
		})
		$worldScopeContract = $externalRequiredAdvisoryContracts[
			"isolation.world_scope"]
		$worldScopeBlocks = @($blockedRows | Where-Object {
			$_.id -ceq "isolation.world_scope" -and
			$_.caseId -ceq $worldScopeContract.caseId -and
			$_.category -ceq $worldScopeContract.category -and
			$_.feature -ceq $worldScopeContract.feature -and
			$_.stage -ceq $worldScopeContract.stage -and
			$_.expected -ceq $worldScopeContract.expected -and
			$_.actual -ceq $worldScopeContract.actual -and
			$_.reason -ceq $worldScopeContract.reason -and
			$_.proofLevel -ceq "EXTERNAL_PROCESS" -and
			$_.observedPath -ceq "manual_external_gap" -and
			$_.requiredPath -ceq
				"external process restart, reconnect, or long-soak harness" -and
			-not $_.countsTowardCertification
		})
		$worldScopeBlockedCases = @($cases | Where-Object {
			[string] (Get-ObjectPropertyValue $_ "m_sCaseId") -ceq
				"cleanup.state_isolation_restore" -and
			[string] (Get-ObjectPropertyValue $_ "m_sStatus") -ceq "BLOCKED"
		})
		$correctedCanaryWarningContractExact =
			$warningRows.Count -eq 1 -and $playerMarkerWarnings.Count -eq 1 -and
			(Get-ObjectPropertyValue $validation `
				"CorrectedCanaryWarningContractExact") -is [bool] -and
			[bool] (Get-ObjectPropertyValue $validation `
				"CorrectedCanaryWarningContractExact")
		$correctedCanaryBlockedContractExact =
			$blockedRows.Count -eq 1 -and $worldScopeBlocks.Count -eq 1 -and
			$worldScopeBlockedCases.Count -eq 1 -and
			(Get-ObjectPropertyValue $validation `
				"CorrectedCanaryBlockedContractExact") -is [bool] -and
			[bool] (Get-ObjectPropertyValue $validation `
				"CorrectedCanaryBlockedContractExact")
		$correctedCanaryAssertionSkipFree = $skipIds.Count -eq 0
		$correctedCanaryAssertionManifestExact =
			(Get-ObjectPropertyValue $validation `
				"CorrectedCanaryAssertionManifestExact") -is [bool] -and
			[bool] (Get-ObjectPropertyValue $validation `
				"CorrectedCanaryAssertionManifestExact")
		$correctedCanaryStateDiffManifestExact =
			(Get-ObjectPropertyValue $validation "StateDiffManifestExact") -is [bool] -and
			[bool] (Get-ObjectPropertyValue $validation "StateDiffManifestExact")
		$correctedCanaryOrphanContractExact =
			(Get-ObjectPropertyValue $validation `
				"CorrectedCanaryOrphanContractExact") -is [bool] -and
			[bool] (Get-ObjectPropertyValue $validation `
				"CorrectedCanaryOrphanContractExact")
		$correctedCanaryProofAxisPassed =
			$artifactValidationValid -and $correctedCanaryCaseSetExact -and
			$rawAssertionCount -eq 91 -and $caseCount -eq 11 -and
			$caseCounts.PASS -eq 9 -and
			$caseCounts.WARN -eq 1 -and $caseCounts.FAIL -eq 0 -and
			$caseCounts.BLOCKED -eq 1 -and $caseCounts.SKIPPED -eq 0 -and
			$correctedCanaryWarningContractExact -and
			$correctedCanaryBlockedContractExact -and
			$focusedAssertionSetExact -and $focusedCaseStatus -ceq "PASS" -and
			$focusedAssertionCount -eq 35 -and $focusedAssertionsPassed -eq 35 -and
			$focusedAssertionsCertificationExact -and
			$correctedCanaryAssertionSkipFree -and
			$correctedCanaryAssertionManifestExact -and
			$correctedCanaryStateDiffManifestExact -and
			$correctedCanaryOrphanContractExact -and $failIds.Count -eq 0 -and
			$requiredAssertions -eq 87 -and $certCounts.PASS -eq 87 -and
			$certCounts.FAIL -eq 0 -and $certCounts.BLOCKED -eq 0 -and
			$certCounts.WARN -eq 0 -and -not $certificationPassed -and
			$deltaMatches.Count -eq 18 -and $nonzeroDeltaCount -eq 0 -and
			[bool] (Get-ObjectPropertyValue $proof "finalOrphanCleanupPass") -and
			[int] (Get-ObjectPropertyValue $proof "finalOrphanActiveGroups") -eq 0

		$canaryProofTextBindings = [ordered] @{
			focusedCaseId = $focusedCaseId
			focusedCaseStatus = $focusedCaseStatus
		}
		foreach ($binding in $canaryProofTextBindings.GetEnumerator()) {
			if ([string] (Get-ObjectPropertyValue $proof $binding.Key) -cne
				[string] $binding.Value) {
				throw "$Label release-index proof $($binding.Key) differs from raw JSON."
			}
		}
		$canaryProofIntegerBindings = [ordered] @{
			focusedAssertionCount = $focusedAssertionCount
			focusedAssertionsPassed = $focusedAssertionsPassed
		}
		foreach ($binding in $canaryProofIntegerBindings.GetEnumerator()) {
			if ((Require-IntegerProperty $proof $binding.Key `
					"$Label release-index proof.$($binding.Key)") -ne
				[int] $binding.Value) {
				throw "$Label release-index proof $($binding.Key) differs from raw JSON."
			}
		}
		$canaryProofBooleanBindings = [ordered] @{
			focusedAssertionSetExact = $focusedAssertionSetExact
			focusedAssertionsCertificationExact = $focusedAssertionsCertificationExact
			correctedCanaryCaseSetExact = $correctedCanaryCaseSetExact
			correctedCanaryWarningContractExact = $correctedCanaryWarningContractExact
			correctedCanaryBlockedContractExact = $correctedCanaryBlockedContractExact
			correctedCanaryAssertionSkipFree = $correctedCanaryAssertionSkipFree
			correctedCanaryAssertionManifestExact = $correctedCanaryAssertionManifestExact
			correctedCanaryStateDiffManifestExact = $correctedCanaryStateDiffManifestExact
			correctedCanaryOrphanContractExact = $correctedCanaryOrphanContractExact
			correctedCanaryProofAxisPassed = $correctedCanaryProofAxisPassed
		}
		foreach ($binding in $canaryProofBooleanBindings.GetEnumerator()) {
			$proofValue = Get-ObjectPropertyValue $proof $binding.Key
			if ($proofValue -isnot [bool] -or
				[bool] $proofValue -ne [bool] $binding.Value) {
				throw "$Label release-index proof $($binding.Key) differs from raw JSON."
			}
		}
	}

	$derivedBlockedJson = ConvertTo-Json -InputObject @($blockedRows.ToArray()) `
		-Depth 12 -Compress
	$indexBlockedJson = ConvertTo-Json -InputObject `
		@((Get-ObjectPropertyValue $proof "blockedAssertions")) -Depth 12 -Compress
	if ($derivedBlockedJson -cne $indexBlockedJson) {
		throw "$Label release index did not derive its blocked assertion linkage from raw JSON."
	}
	$derivedWarningJson = ConvertTo-Json -InputObject @($warningRows.ToArray()) `
		-Depth 12 -Compress
	$indexWarningJson = ConvertTo-Json -InputObject `
		@((Get-ObjectPropertyValue $proof "warningAssertions")) -Depth 12 -Compress
	if ($derivedWarningJson -cne $indexWarningJson) {
		throw "$Label release index did not derive its warning assertion linkage from raw JSON."
	}
	$derivedExternalAdvisoryRows = @($warningRows | Where-Object {
		$externalRequiredAdvisoryIds -ccontains $_.id
	})
	if ($isCorrectedCanary) {
		$derivedExternalAdvisoryRows = @($blockedRows | Where-Object {
			$externalRequiredAdvisoryIds -ccontains $_.id
		})
	}
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
	if ($isCorrectedCanary) {
		$unsupportedWarningIds = @($warnIds | Where-Object {
			$_ -cne "cleanup.player_marker.live"
		})
	}
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
	Assert-ExactObjectProperties $recordedErrorCensus @(
		"Valid", "HardDiagnosticFree", "HardDiagnosticCount", "ScriptErrors",
		"EngineErrors", "PartisanErrors", "CrashMarkers", "PartisanSeverityLineCount",
		"ApprovedStockDiagnosticCount", "ApprovedIntentionalDiagnosticCount",
		"ChannelArithmeticValid", "CategoryArithmeticValid",
		"MalformedHardDiagnosticCount", "UnapprovedHardDiagnosticCount",
		"UnapprovedHardDiagnosticKinds", "LifecycleMarkersValid",
		"IdentityBaselinePairValid", "IntentionalFixtureStructureExact",
		"IntentionalFixtureSetValid",
		"IntentionalMissionConvoyAdmissionDiagnosticsProven",
		"IntentionalMissionConvoySettlementDiagnosticProven",
		"IntentionalMissionConvoyCorruptionDiagnosticsProven",
		"IntentionalMissionConvoyWatchdogDiagnosticProven",
		"CanonicalScriptLogCount", "CanonicalConsoleLogCount",
		"CanonicalErrorLogCount", "CanonicalCrashLogCount",
		"CanonicalLogPairSameDirectory", "AuxiliaryLogPairSameDirectory",
		"AuxiliaryDiagnosticsValid", "ErrorLogProjectionExact",
		"CrashLogProjectionExact", "AuxiliaryUnapprovedEventCount") `
		"$Label run.outcome.errorCensus"
	foreach ($row in @(Get-ObjectPropertyValue $recordedErrorCensus `
			"UnapprovedHardDiagnosticKinds")) {
		Assert-ExactObjectProperties $row @("kind", "count") `
			"$Label run diagnostic-kind row"
	}
	foreach ($field in @(
			"HardDiagnosticCount", "ScriptErrors", "EngineErrors", "PartisanErrors",
			"CrashMarkers", "PartisanSeverityLineCount", "ApprovedStockDiagnosticCount",
			"ApprovedIntentionalDiagnosticCount", "MalformedHardDiagnosticCount",
			"UnapprovedHardDiagnosticCount", "CanonicalScriptLogCount",
			"CanonicalConsoleLogCount", "CanonicalErrorLogCount",
			"CanonicalCrashLogCount", "AuxiliaryUnapprovedEventCount")) {
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
			"AuxiliaryLogPairSameDirectory", "AuxiliaryDiagnosticsValid",
			"ErrorLogProjectionExact", "CrashLogProjectionExact",
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
		canonicalErrorLogCount = "CanonicalErrorLogCount"
		canonicalCrashLogCount = "CanonicalCrashLogCount"
		auxiliaryUnapprovedEventCount = "AuxiliaryUnapprovedEventCount"
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
		auxiliaryLogPairSameDirectory = "AuxiliaryLogPairSameDirectory"
		auxiliaryDiagnosticsValid = "AuxiliaryDiagnosticsValid"
		errorLogProjectionExact = "ErrorLogProjectionExact"
		crashLogProjectionExact = "CrashLogProjectionExact"
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
	$retainedRunnerClassifierCount = Require-IntegerScalar `
		(Get-ObjectPropertyValue $semanticValidation "ClassifierSelfTestCount") `
		"$Label retained runner classifier self-test count"
	if ($classifierCount -ne $runClassifierCount -or
		$classifierCount -ne $retainedRunnerClassifierCount) {
		throw "$Label diagnostic classifier count contradicts run.json or the retained immutable runner."
	}
	$diagnosticAxisCommonPassed =
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
		[bool] (Get-ObjectPropertyValue $diagnostics "auxiliaryLogPairSameDirectory") -and
		[bool] (Get-ObjectPropertyValue $diagnostics "auxiliaryDiagnosticsValid") -and
		[bool] (Get-ObjectPropertyValue $diagnostics "errorLogProjectionExact") -and
		[bool] (Get-ObjectPropertyValue $diagnostics "crashLogProjectionExact") -and
		[int] (Get-ObjectPropertyValue $diagnostics "crashMarkers") -eq 0 -and
		[int] (Get-ObjectPropertyValue $diagnostics "partisanSeverityLineCount") -eq 0 -and
		[int] (Get-ObjectPropertyValue $diagnostics "malformedHardDiagnosticCount") -eq 0 -and
		[int] (Get-ObjectPropertyValue $diagnostics "canonicalScriptLogCount") -eq 1 -and
		[int] (Get-ObjectPropertyValue $diagnostics "canonicalConsoleLogCount") -eq 1 -and
		[int] (Get-ObjectPropertyValue $diagnostics "canonicalErrorLogCount") -eq 1 -and
		[int] (Get-ObjectPropertyValue $diagnostics "canonicalCrashLogCount") -eq 1 -and
		[int] (Get-ObjectPropertyValue $diagnostics "auxiliaryUnapprovedEventCount") -eq 0 -and
		$unapprovedCount -eq 0
	$diagnosticAxisPassed = if ($isCorrectedCanary) {
		$diagnosticAxisCommonPassed -and $classifierCount -eq 38 -and
		$retainedRunnerClassifierCount -eq 38 -and
		$hardCount -eq 2 -and $scriptErrors -eq 2 -and $engineErrors -eq 0 -and
		[int] (Get-ObjectPropertyValue $diagnostics "partisanErrors") -eq 0 -and
		$stockCount -eq 2 -and $intentionalCount -eq 0 -and
		-not [bool] (Get-ObjectPropertyValue $proof `
			"intentionalMissionConvoyAdmissionDiagnosticsProven") -and
		-not [bool] (Get-ObjectPropertyValue $proof `
			"intentionalMissionConvoySettlementDiagnosticProven") -and
		-not [bool] (Get-ObjectPropertyValue $proof `
			"intentionalMissionConvoyCorruptionDiagnosticsProven") -and
		-not [bool] (Get-ObjectPropertyValue $proof `
			"intentionalMissionConvoyWatchdogDiagnosticProven")
	}
	else {
		$diagnosticAxisCommonPassed -and $classifierCount -eq 38 -and
		$retainedRunnerClassifierCount -eq 38 -and
		$hardCount -eq 15 -and $scriptErrors -eq 15 -and $engineErrors -eq 0 -and
		[int] (Get-ObjectPropertyValue $diagnostics "partisanErrors") -eq 13 -and
		$stockCount -eq 2 -and $intentionalCount -eq 13 -and
		[bool] (Get-ObjectPropertyValue $proof `
			"intentionalMissionConvoyAdmissionDiagnosticsProven") -and
		[bool] (Get-ObjectPropertyValue $proof `
			"intentionalMissionConvoySettlementDiagnosticProven") -and
		[bool] (Get-ObjectPropertyValue $proof `
			"intentionalMissionConvoyCorruptionDiagnosticsProven") -and
		[bool] (Get-ObjectPropertyValue $proof `
			"intentionalMissionConvoyWatchdogDiagnosticProven")
	}

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
	Assert-ExactObjectProperties $mount @(
		"Valid", "RecordCount", "ExactPathCount", "PackedCount",
		"InvalidModeCount", "GuidExact", "Packed") `
		"$Label run.outcome.mountAttestation"
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
	$acceptedCorrectedCanary = $false
	$proofCommonPassed = $false
	$acceptedFull = $false
	$acceptedInternal = $false
	if ($isCorrectedCanary) {
		$acceptedCorrectedCanary = $correctedCanaryProofAxisPassed -and
			$diagnosticAxisPassed -and $guardedRunSucceeded -and
			[string]::IsNullOrWhiteSpace($outcomeError)
	}
	else {
		$proofCommonPassed = $caseCounts.FAIL -eq 0 -and
			$caseCounts.BLOCKED -eq 0 -and $failIds.Count -eq 0 -and
			$blockedRows.Count -eq 0 -and $unsupportedWarningIds.Count -eq 0 -and
			$unsupportedSkipIds.Count -eq 0 -and $artifactValidationValid -and
			$certificationPassed -and $certCounts.FAIL -eq 0 -and
			$certCounts.BLOCKED -eq 0 -and $certCounts.WARN -eq 0 -and
			$nonzeroDeltaCount -eq 0 -and
			[bool] (Get-ObjectPropertyValue $proof "finalOrphanCleanupPass") -and
			[int] (Get-ObjectPropertyValue $proof "finalOrphanActiveGroups") -eq 0
		$acceptedFull = $proofCommonPassed -and $warnIds.Count -eq 0 -and
			$caseCounts.WARN -eq 0 -and $guardedRunSucceeded -and
			[string]::IsNullOrWhiteSpace($outcomeError) -and $diagnosticAxisPassed
		if (-not $acceptedFull -and $proofCommonPassed -and
			$externalAdvisoryLinkageValid -and $guardedRunSucceeded -and
			[string]::IsNullOrWhiteSpace($outcomeError) -and $diagnosticAxisPassed) {
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
	}
	$derivedStatus = if ($isCorrectedCanary) {
		"failed-corrected-canary"
	}
	else {
		"failed-full-profile"
	}
	$derivedAcceptance = if ($isCorrectedCanary) {
		"rejected-corrected-canary"
	}
	else {
		"rejected-full-profile"
	}
	$derivedRelease = if ($isCorrectedCanary) { "replacement-required" } else { "remain-no-go" }
	$derivedFinding = if ($isCorrectedCanary) {
		"rejected-corrected-canary"
	}
	else {
		"release-blocking-red-full-profile"
	}
	if ($acceptedCorrectedCanary) {
		$derivedStatus = "passed-noncertifying"
		$derivedAcceptance = "accepted-noncertifying"
		$derivedRelease = "proceed-full-profile"
		$derivedFinding = "accepted-noncertifying"
	}
	elseif ($acceptedFull) {
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
	$derivedFindingDefect = if ($isCorrectedCanary) {
		"One or more corrected-canary acceptance axes failed."
	}
	else {
		"One or more full-profile acceptance axes failed."
	}
	$derivedFindingNextStep = if ($isCorrectedCanary) {
		"Repair the retained canary defect, seal a replacement candidate, and rerun the corrected canary."
	}
	else {
		"Repair every retained proof or diagnostic rejection and seal a new immutable candidate."
	}
	if ($acceptedCorrectedCanary) {
		$derivedFindingDefect = "none"
		$derivedFindingNextStep =
			"Run the full Campaign Debug profile against the unchanged candidate."
	}
	elseif ($acceptedFull) {
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
		if ($recordedWorktree -cne $recordedBlob) {
			throw "$Label recorded $($binding.WorktreeField) is not the clean immutable Git blob."
		}
		$expectedToolSha = $null
		if ($AllowUntrackedSummaryForSelfTest) {
			if ($null -eq $TrustedToolBindingsForSelfTest -or
				$null -eq $TrustedToolBindingsForSelfTest.PSObject.Properties[$binding.BlobField]) {
				throw "$Label self-test tool trust bundle is incomplete."
			}
			$expectedToolSha = [string] $TrustedToolBindingsForSelfTest.($binding.BlobField)
			$selfTestToolPath = Join-Path $root $binding.Path
			$selfTestToolSha = (Get-FileHash `
				-LiteralPath $selfTestToolPath `
				-Algorithm SHA256).Hash.ToLowerInvariant()
			if ($selfTestToolSha -cne $expectedToolSha) {
				throw "$Label self-test $($binding.Path) is not the stationary committed harness blob."
			}
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
	$wallClockSeconds = ($completedUtc - $startedUtc).TotalSeconds
	$runtimeWindowInvalid = if ($isCorrectedCanary) {
		$completedUtc -le $startedUtc -or
			$runtimeSeconds -gt [Math]::Ceiling($wallClockSeconds)
	}
	else {
		[Math]::Abs($wallClockSeconds - $runtimeSeconds) -gt 5
	}
	if ($completedUtc -lt $startedUtc -or $StatusAsOfUtc -lt $completedUtc -or
		$runtimeSeconds -le 0 -or
		$runtimeWindowInvalid -or
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
		skipped = $caseCounts.SKIPPED
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
	if ($isCorrectedCanary) {
		$evidenceIntegerChecks.focusedAssertionCount = $focusedAssertionCount
		$evidenceIntegerChecks.focusedAssertionsPassed = $focusedAssertionsPassed
		$evidenceIntegerChecks.certificationRequired = $requiredAssertions
		$evidenceIntegerChecks.certificationProven = $certCounts.PASS
	}
	else {
		$evidenceIntegerChecks.requiredAssertions = $requiredAssertions
		$evidenceIntegerChecks.provenAssertions = $certCounts.PASS
		$evidenceIntegerChecks.failedAssertions = $certCounts.FAIL
		$evidenceIntegerChecks.blockedAssertions = $certCounts.BLOCKED
	}
	foreach ($binding in $evidenceIntegerChecks.GetEnumerator()) {
		if ((Require-IntegerProperty $Evidence $binding.Key `
				"$Label.$($binding.Key)") -ne
			[int] $binding.Value) {
			throw "$Label.$($binding.Key) differs from the retained raw bundle."
		}
	}
	$evidenceBooleanChecks = [ordered] @{
		diagnosticClassificationValid = [bool] (Get-ObjectPropertyValue $diagnostics "valid")
		hardDiagnosticFree = [bool] (Get-ObjectPropertyValue $diagnostics "hardDiagnosticFree")
		canonicalLogPairSameDirectory = [bool] (Get-ObjectPropertyValue $diagnostics "canonicalLogPairSameDirectory")
		finalOrphanCleanupPass = [bool] (Get-ObjectPropertyValue $proof "finalOrphanCleanupPass")
		cleanupAndSpillZero = $true
		envelopeFilesRehashed = $true
	}
	if (-not $isCorrectedCanary) {
		$evidenceBooleanChecks.wrapperCaptureSuccess = $wrapperCaptureSuccess
		$evidenceBooleanChecks.runtimeOutcomeSuccess = $guardedRunSucceeded
		$evidenceBooleanChecks.certificationPassed = $certificationPassed
		$evidenceBooleanChecks.artifactSchemaValidationValid = $artifactValidationValid
		$evidenceBooleanChecks.candidateBoundaryVerified = $true
		$evidenceBooleanChecks.mountPacked = $true
		$evidenceBooleanChecks.artifactsStable = $true
	}
	foreach ($binding in $evidenceBooleanChecks.GetEnumerator()) {
		$value = Get-ObjectPropertyValue $Evidence $binding.Key
		if ($value -isnot [bool] -or [bool] $value -ne [bool] $binding.Value) {
			throw "$Label.$($binding.Key) differs from the retained raw bundle."
		}
	}
	if ($isCorrectedCanary) {
		$outcomeSuccessValue = Get-ObjectPropertyValue $Evidence "outcomeSuccess"
		if ($outcomeSuccessValue -isnot [bool] -or
			[bool] $outcomeSuccessValue -ne $guardedRunSucceeded) {
			throw "$Label.outcomeSuccess differs from the retained raw bundle."
		}
		$derivedUnapprovedHardDiagnosticKind = if ($indexKinds.Count -eq 0) {
			""
		}
		else {
			@($indexKinds | ForEach-Object {
				Require-Text (Get-ObjectPropertyValue $_ "kind") `
					"$Label unapproved diagnostic kind"
			} | Sort-Object -CaseSensitive) -join ","
		}
		if ([string] (Get-ObjectPropertyValue $Evidence `
				"unapprovedHardDiagnosticKind") -cne
			$derivedUnapprovedHardDiagnosticKind) {
			throw "$Label.unapprovedHardDiagnosticKind differs from retained diagnostics."
		}
		$canarySummary = Require-Text `
			(Get-ObjectPropertyValue $Evidence "summary") "$Label.summary"
		Assert-NoLocalAbsolutePathValue $canarySummary "$Label.summary"
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
	if (-not $isCorrectedCanary) {
		$evidenceExternalAdvisoryIds = @((Get-ObjectPropertyValue `
			$Evidence "externalRequiredAdvisoryIds"))
		Assert-EqualSet $derivedExternalAdvisoryIds $evidenceExternalAdvisoryIds `
			"$Label status/raw external-required advisory IDs"
	}
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

	# Semantic validation reads only the private byte snapshot above. Re-open every
	# retained source at the end so a concurrent replacement cannot splice an
	# index, envelope, or raw inventory from different moments into one admission.
	Assert-FileByteSnapshotUnchanged $indexSnapshot `
		"$Label portable release index"
	Assert-FileByteSnapshotUnchanged $externalIndexSnapshot `
		"$Label retained portable-bundle index"
	Assert-FileByteSnapshotUnchanged $runSnapshot `
		"$Label retained run envelope"
	foreach ($path in $rowPaths) {
		Assert-FileByteSnapshotUnchanged $rowMap[$path].OriginalSnapshot `
			"$Label retained raw file $path"
	}
	$finalBundleItem = Get-Item -LiteralPath $bundleRoot -Force
	$finalBundleEntries = @(Get-ChildItem -LiteralPath $bundleRoot -Recurse -Force)
	if (($finalBundleItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
		@($finalBundleEntries | Where-Object {
				($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0
			}).Count -ne 0) {
		throw "$Label retained raw bundle gained a reparse point during validation."
	}
	if (-not $AllowUntrackedSummaryForSelfTest -or $usePortableEvidenceRoot) {
		$finalBundlePathCursor = $evidenceRootPath
		foreach ($bundlePathSegment in $bundleRelativePath.Split('/')) {
			$finalBundlePathCursor = Join-Path $finalBundlePathCursor $bundlePathSegment
			$finalBundlePathItem = Get-Item -LiteralPath $finalBundlePathCursor -Force
			if (($finalBundlePathItem.Attributes -band
					[IO.FileAttributes]::ReparsePoint) -ne 0) {
				throw "$Label portable raw bundle path changed to a reparse point during validation."
			}
		}
	}
	$finalBundleRows = @($finalBundleEntries |
		Where-Object { -not $_.PSIsContainer } |
		Where-Object {
			$_.FullName -cne $runPath -and $_.FullName -cne $externalIndexPath
		} |
		ForEach-Object {
			$_.FullName.Substring($bundleRoot.TrimEnd('\', '/').Length + 1).Replace('\', '/')
		})
	Assert-EqualSet $rowPaths $finalBundleRows `
		"$Label final run inventory/raw bundle"

	$acceptedPortableEvidence = $acceptedCorrectedCanary -or
		$acceptedFull -or $acceptedInternal
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
		ContractProfile = $ContractProfile
		Status = $derivedStatus
		Accepted = $acceptedPortableEvidence
		AcceptedCorrectedCanary = $acceptedCorrectedCanary
		AcceptedFull = $acceptedFull
		AcceptedInternal = $acceptedInternal
		Rejected = -not $acceptedPortableEvidence
		AcceptanceDisposition = $derivedAcceptance
		ExternalRequiredAdvisoryIds = $derivedExternalAdvisoryIds
		AssertionCount = $rawAssertionCount
		FocusedAssertionCount = $focusedAssertionCount
		FocusedAssertionsPassed = $focusedAssertionsPassed
		CertificationRequired = $requiredAssertions
		CertificationProven = $certCounts.PASS
		CaseCount = $caseCount
		Pass = $caseCounts.PASS
		Warn = $caseCounts.WARN
		Fail = $caseCounts.FAIL
		Blocked = $caseCounts.BLOCKED
		Skipped = $caseCounts.SKIPPED
		DiagnosticAxisPassed = $diagnosticAxisPassed
		ClassifierSelfTestCount = $classifierCount
		StateDiffRows = $deltaMatches.Count
		NonzeroStateDiffRows = $nonzeroDeltaCount
		FinalOrphanCleanupPass = [bool] (Get-ObjectPropertyValue `
			$proof "finalOrphanCleanupPass")
		FinalOrphanActiveGroups = [int] (Get-ObjectPropertyValue `
			$proof "finalOrphanActiveGroups")
		EnvelopeFileCount = $runFiles.Count
		LegacyHistoricalFixture = $false
	}
	}
	finally {
		if ($validationSnapshotCreated -and
			(Test-Path -LiteralPath $validationSnapshotRoot -PathType Container)) {
			$cleanupPath = [IO.Path]::GetFullPath($validationSnapshotRoot)
			$cleanupItem = Get-Item -LiteralPath $cleanupPath -Force
			if (-not $cleanupPath.StartsWith(
					$tempPrefix,
					[StringComparison]::OrdinalIgnoreCase) -or
				(Split-Path -Leaf $cleanupPath) -cnotlike
					"PartisanCampaignDebugConsumer-*" -or
				($cleanupItem.Attributes -band
					[IO.FileAttributes]::ReparsePoint) -ne 0) {
				throw "$Label validation-snapshot cleanup containment failed."
			}
			Remove-Item -LiteralPath $cleanupPath -Recurse -Force
		}
	}
}

function Get-CorrectedCanarySummaryRoute {
	param(
		[object] $Evidence,
		[string] $Label
	)

	if ($null -eq $Evidence) {
		throw "$Label is missing."
	}
	$summaryPath = Require-RepoRelativePath `
		(Get-ObjectPropertyValue $Evidence "summaryPath") "$Label.summaryPath"
	$summarySha = (Require-Sha256 `
		(Get-ObjectPropertyValue $Evidence "summarySha256") `
		"$Label.summarySha256").ToLowerInvariant()
	$summaryFullPath = [IO.Path]::GetFullPath((Join-Path $root $summaryPath))
	if (-not (Test-FocusedContainedPath $root $summaryFullPath) -or
		-not (Test-Path -LiteralPath $summaryFullPath -PathType Leaf)) {
		throw "$Label summary is missing or outside the repository."
	}
	Assert-FocusedNoReparseAncestry $root $summaryFullPath "$Label summary route"
	$summarySnapshot = Read-FileByteSnapshot $summaryFullPath "$Label summary route"
	if ([string] $summarySnapshot.Sha256 -cne $summarySha) {
		throw "$Label summary SHA-256 does not match release status."
	}
	$summary = (ConvertFrom-FocusedStrictUtf8JsonSnapshot `
		$summarySnapshot "$Label summary route").Value
	$schemaVersion = Require-IntegerProperty `
		$summary "schemaVersion" "$Label summary.schemaVersion"
	$evidenceKind = [string] (Get-ObjectPropertyValue $summary "evidenceKind")
	if ($schemaVersion -notin @(1, 2) -or
		$evidenceKind -cne "packaged-campaign-debug-corrected-canary") {
		throw "$Label summary uses an unsupported corrected-canary schema."
	}
	return [PSCustomObject] @{
		SchemaVersion = $schemaVersion
		EvidenceKind = $evidenceKind
	}
}

function Assert-ActiveCorrectedCanaryEvidence {
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
	if ($statusValue -ceq "failed-corrected-canary") {
		return Assert-PortableFullCampaignDebugEvidence `
			$Evidence `
			$CandidateIdentity `
			$Label `
			$StatusAsOfUtc `
			$SourceSettingsSchema `
			-AllowUntrackedSummaryForSelfTest:$AllowUntrackedSummaryForSelfTest `
			-TrustedToolBindingsForSelfTest $TrustedToolBindingsForSelfTest `
			-PortableEvidenceRoot $PortableEvidenceRoot `
			-ContractProfile force_authority
	}

	$usePortableSchema = $false
	if ($statusValue -ceq "passed-noncertifying") {
		$summaryPath = Require-RepoRelativePath `
			(Get-ObjectPropertyValue $Evidence "summaryPath") "$Label.summaryPath"
		$repoPrefix = [IO.Path]::GetFullPath($root).TrimEnd(
			[IO.Path]::DirectorySeparatorChar,
			[IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
		$summaryFullPath = [IO.Path]::GetFullPath((Join-Path $root $summaryPath))
		if (-not $summaryFullPath.StartsWith(
				$repoPrefix,
				[StringComparison]::OrdinalIgnoreCase) -or
			-not (Test-Path -LiteralPath $summaryFullPath -PathType Leaf)) {
			throw "$Label summary is missing or outside the repository."
		}
		$summaryItem = Get-Item -LiteralPath $summaryFullPath -Force
		if (($summaryItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
			throw "$Label summary must not be a reparse point."
		}
		try {
			$summary = Get-Content -Raw -LiteralPath $summaryFullPath | ConvertFrom-Json
		}
		catch {
			throw "$Label summary is invalid JSON: $($_.Exception.Message)"
		}
		$summarySchemaVersion = Require-IntegerProperty `
			$summary "schemaVersion" "$Label summary.schemaVersion"
		$summaryEvidenceKind = [string] (Get-ObjectPropertyValue `
			$summary "evidenceKind")
		if ($summarySchemaVersion -eq 2 -and
			$summaryEvidenceKind -ceq "packaged-campaign-debug-corrected-canary") {
			$usePortableSchema = $true
		}
		elseif ($summarySchemaVersion -ne 1 -or
			$summaryEvidenceKind -cne "packaged-campaign-debug-corrected-canary") {
			throw "$Label summary uses an unsupported corrected-canary schema."
		}
	}
	elseif ($statusValue -cne "failed-proof-validation") {
		throw "$Label.status is not a supported active corrected-canary disposition."
	}

	if ($usePortableSchema) {
		return Assert-PortableFullCampaignDebugEvidence `
			$Evidence `
			$CandidateIdentity `
			$Label `
			$StatusAsOfUtc `
			$SourceSettingsSchema `
			-AllowUntrackedSummaryForSelfTest:$AllowUntrackedSummaryForSelfTest `
			-TrustedToolBindingsForSelfTest $TrustedToolBindingsForSelfTest `
			-PortableEvidenceRoot $PortableEvidenceRoot `
			-ContractProfile force_authority
	}

	$expectedOutcome = if ($statusValue -ceq "passed-noncertifying") {
		"accepted"
	}
	else {
		"rejected-proof"
	}
	$legacyValidation = Assert-CorrectedCanaryEvidence `
		$Evidence `
		$CandidateIdentity `
		$statusValue `
		$expectedOutcome `
		$Label `
		$StatusAsOfUtc `
		$SourceSettingsSchema
	$legacyAccepted = $expectedOutcome -ceq "accepted"
	$legacyValidation | Add-Member -NotePropertyName ContractProfile `
		-NotePropertyValue "force_authority" -Force
	$legacyValidation | Add-Member -NotePropertyName Status `
		-NotePropertyValue $statusValue -Force
	$legacyValidation | Add-Member -NotePropertyName Accepted `
		-NotePropertyValue $legacyAccepted -Force
	$legacyValidation | Add-Member -NotePropertyName AcceptedCorrectedCanary `
		-NotePropertyValue $legacyAccepted -Force
	$legacyValidation | Add-Member -NotePropertyName Rejected `
		-NotePropertyValue (-not $legacyAccepted) -Force
	$legacyValidation | Add-Member -NotePropertyName AcceptanceDisposition `
		-NotePropertyValue ([string] (Get-ObjectPropertyValue `
			$Evidence "acceptanceDisposition")) -Force
	$legacyValidation | Add-Member -NotePropertyName LegacySchema1 `
		-NotePropertyValue $true -Force
	return $legacyValidation
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

function Assert-ReleaseBuildTransition {
	param(
		[string] $RuntimeUseDisposition,
		[object] $ManifestEmbedded,
		[string] $CandidateSourceHead,
		[string] $SourceBuildSha,
		[string] $SourceBuildUtc,
		[string] $SourceBuildLabel,
		[string] $CheckoutHead
	)

	if ($RuntimeUseDisposition -cnotin @(
			"active-runtime-candidate",
			"supersede-before-runtime",
			"rejected-after-runtime")) {
		throw "The release build transition has an unsupported runtime-use disposition."
	}

	$manifestBuildSha = Require-Text `
		(Get-ObjectPropertyValue $ManifestEmbedded "sha") `
		"candidate manifest embedded implementation SHA"
	$manifestBuildUtc = Require-Text `
		(Get-ObjectPropertyValue $ManifestEmbedded "utc") `
		"candidate manifest embedded build UTC"
	$manifestBuildLabel = Require-Text `
		(Get-ObjectPropertyValue $ManifestEmbedded "label") `
		"candidate manifest embedded build label"
	if ($manifestBuildSha -cnotmatch '^[0-9a-f]{40}$') {
		throw "The candidate manifest embedded implementation SHA must be a lowercase full Git SHA."
	}
	if ($CandidateSourceHead -cnotmatch '^[0-9a-f]{40}$' -or
		$SourceBuildSha -cnotmatch '^[0-9a-f]{40}$' -or
		$CheckoutHead -cnotmatch '^[0-9a-f]{40}$') {
		throw "The release build transition requires lowercase full Git SHAs."
	}
	$manifestBuildTime = Require-UtcTimestamp `
		$manifestBuildUtc "candidate manifest embedded build UTC"
	$sourceBuildTime = Require-UtcTimestamp `
		$SourceBuildUtc "live embedded build UTC"
	[void] (Require-Text $SourceBuildLabel "live embedded build label")

	Assert-GitAncestor `
		$manifestBuildSha `
		$CandidateSourceHead `
		"The candidate embedded implementation build is not an ancestor of its candidate source HEAD"
	Assert-GitAncestor `
		$SourceBuildSha `
		$CheckoutHead `
		"The live embedded implementation build is not an ancestor of checkout HEAD"

	$sameBuildIdentity = $manifestBuildSha -ceq $SourceBuildSha `
		-and $manifestBuildUtc -ceq $SourceBuildUtc `
		-and $manifestBuildLabel -ceq $SourceBuildLabel
	if ($RuntimeUseDisposition -ceq "active-runtime-candidate" -and
		-not $sameBuildIdentity) {
		throw "An active runtime candidate must match the live embedded implementation identity."
	}
	if ($RuntimeUseDisposition -cne "active-runtime-candidate" -and
		-not $sameBuildIdentity) {
		Assert-GitAncestor `
			$CandidateSourceHead `
			$SourceBuildSha `
			"The live embedded implementation build did not advance from the retained candidate source HEAD"
		if ($sourceBuildTime -le $manifestBuildTime) {
			throw "The live embedded build UTC must advance after the retained candidate embedded build UTC."
		}
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

function Assert-HistoricalCorrectedCanaryEvidence {
	param(
		[object] $Evidence,
		[object] $CandidateIdentity,
		[ValidateSet(
			"rejected-after-full-profile",
			"rejected-after-corrected-canary",
			"rejected-after-external-runtime")]
		[string] $RetirementDisposition,
		[string] $Label,
		[DateTimeOffset] $StatusAsOfUtc,
		[int] $SourceSettingsSchema,
		[switch] $AllowUntrackedSummaryForSelfTest,
		[object] $TrustedToolBindingsForSelfTest,
		[string] $PortableEvidenceRoot = $EvidenceBundleRoot
	)

	$statusValue = Require-Text `
		(Get-ObjectPropertyValue $Evidence "status") "$Label.status"
	$route = Get-CorrectedCanarySummaryRoute $Evidence $Label
	if ($route.SchemaVersion -eq 2) {
		if ($statusValue -cnotin @(
				"passed-noncertifying", "failed-corrected-canary")) {
			throw "$Label schema-2 corrected canary has an unsupported historical status."
		}
		if ($RetirementDisposition -cin @(
				"rejected-after-full-profile", "rejected-after-external-runtime")) {
			if ($statusValue -cne "passed-noncertifying") {
				throw "$Label post-canary retirement requires an accepted corrected canary."
			}
		}
		elseif ($statusValue -cne "failed-corrected-canary") {
			throw "$Label corrected-canary retirement requires failed corrected-canary evidence."
		}

		$validation = Assert-PortableFullCampaignDebugEvidence `
			$Evidence `
			$CandidateIdentity `
			$Label `
			$StatusAsOfUtc `
			$SourceSettingsSchema `
			-AllowUntrackedSummaryForSelfTest:$AllowUntrackedSummaryForSelfTest `
			-TrustedToolBindingsForSelfTest $TrustedToolBindingsForSelfTest `
			-PortableEvidenceRoot $PortableEvidenceRoot `
			-ContractProfile force_authority
		if ($statusValue -ceq "passed-noncertifying") {
			if (-not $validation.AcceptedCorrectedCanary -or $validation.Rejected) {
				throw "$Label portable corrected canary did not validate as accepted."
			}
			$outcome = "accepted"
		}
		else {
			if (-not $validation.Rejected -or $validation.AcceptedCorrectedCanary) {
				throw "$Label portable corrected canary did not validate as rejected."
			}
			$outcome = "rejected-proof"
		}
		$validation | Add-Member -NotePropertyName LegacySchema1 `
			-NotePropertyValue $false -Force
		return [PSCustomObject] @{
			Status = $statusValue
			Outcome = $outcome
			Validation = $validation
		}
	}

	$outcome = ""
	if ($RetirementDisposition -cin @(
			"rejected-after-full-profile", "rejected-after-external-runtime")) {
		if ($statusValue -cnotin @(
				"historical-passed-noncertifying", "passed-noncertifying")) {
			throw "$Label post-canary retirement requires an accepted corrected canary."
		}
		$outcome = "accepted"
	}
	elseif ($statusValue -ceq "historical-failed-proof-validation") {
		$outcome = "rejected-proof"
	}
	elseif ($statusValue -ceq "historical-failed-unapproved-hard-diagnostic") {
		$outcome = "rejected"
	}
	else {
		throw "$Label corrected-canary retirement requires rejected corrected-canary evidence."
	}
	$validation = Assert-CorrectedCanaryEvidence `
		$Evidence `
		$CandidateIdentity `
		$statusValue `
		$outcome `
		$Label `
		$StatusAsOfUtc `
		$SourceSettingsSchema
	$validation | Add-Member -NotePropertyName LegacySchema1 `
		-NotePropertyValue $true -Force
	return [PSCustomObject] @{
		Status = $statusValue
		Outcome = $outcome
		Validation = $validation
	}
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
	$correctedCanaryTransition = Assert-HistoricalCorrectedCanaryEvidence `
		$correctedCanary `
		$identity `
		$retirementDisposition `
		"$identityLabel.evidence.correctedForceAuthorityCanary" `
		$StatusAsOfUtc `
		$identity.RuntimeSettingsSchema
	$correctedCanaryStatus = [string] $correctedCanaryTransition.Status
	$correctedCanaryOutcome = [string] $correctedCanaryTransition.Outcome
	$correctedCanaryValidation = $correctedCanaryTransition.Validation
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
		$retainedLegacyRunnerSha = (Require-Sha256 `
			(Get-ObjectPropertyValue $redSummary.harness "campaignRunnerSha256") `
			"self-test retained legacy campaign runner").ToLowerInvariant()
		$retainedLegacyRunnerPolicy = Resolve-CampaignRunnerPolicy `
			$retainedLegacyRunnerSha "self-test retained legacy campaign runner"
		$greenSummary.harness.campaignRunnerSha256 = $retainedLegacyRunnerSha
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
			[bool] $retainedLegacyRunnerPolicy.IntentionalAdmissionProven
		$greenSummary.proof.intentionalMissionConvoySettlementDiagnosticProven =
			[bool] $retainedLegacyRunnerPolicy.IntentionalSettlementProven
		$greenSummary.proof.intentionalMissionConvoyCorruptionDiagnosticsProven =
			[bool] $retainedLegacyRunnerPolicy.IntentionalCorruptionProven
		$greenSummary.proof.intentionalMissionConvoyWatchdogDiagnosticProven =
			[bool] $retainedLegacyRunnerPolicy.IntentionalWatchdogProven
		$greenSummary.diagnostics.valid = $true
		$greenSummary.diagnostics.classificationValid = $true
		$greenSummary.diagnostics.hardDiagnosticFree = $false
		$greenSummary.diagnostics.hardDiagnosticCount =
			[int] $retainedLegacyRunnerPolicy.HardDiagnosticCount
		$greenSummary.diagnostics.scriptErrors =
			[int] $retainedLegacyRunnerPolicy.ScriptErrors
		$greenSummary.diagnostics.engineErrors =
			[int] $retainedLegacyRunnerPolicy.EngineErrors
		$greenSummary.diagnostics.partisanErrors =
			[int] $retainedLegacyRunnerPolicy.PartisanErrors
		$greenSummary.diagnostics.approvedStockDiagnosticCount =
			[int] $retainedLegacyRunnerPolicy.ApprovedStockDiagnosticCount
		$greenSummary.diagnostics.approvedIntentionalDiagnosticCount =
			[int] $retainedLegacyRunnerPolicy.ApprovedIntentionalDiagnosticCount
		$greenSummary.diagnostics.unapprovedHardDiagnosticCount = 0
		$greenSummary.diagnostics.unapprovedHardDiagnosticKinds = @()
		$greenSummary.diagnostics.classifierSelfTestCount =
			[int] $retainedLegacyRunnerPolicy.ClassifierSelfTestCount
		$greenSummary.diagnostics.intentionalFixtureStructureExact = $true
		$greenSummary.diagnostics.intentionalFixtureSetValid = $true
		$greenSummary.finding.status = "accepted-full-profile"
		$greenSummary.finding.defect = "none"
		$greenSummary.finding.nextStep =
			"Advance the unchanged package to the external release gates."

		$greenEvidence = & $cloneJson $redEvidence
		$greenEvidence.status = "passed-full-certification"
		$greenEvidence.campaignRunnerSha256 = $retainedLegacyRunnerSha
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
			@($retainedLegacyRunnerPolicy.ExternalRequiredBlockerIds).Count
		$internalSummary.proof.certificationBlocked =
			@($retainedLegacyRunnerPolicy.ExternalRequiredBlockerIds).Count
		$internalSummary.proof | Add-Member -NotePropertyName `
			externalRequiredBlockerIds -NotePropertyValue `
			@($retainedLegacyRunnerPolicy.ExternalRequiredBlockerIds)
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
			@($retainedLegacyRunnerPolicy.ExternalRequiredBlockerIds)
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
			-not [bool] $retainedLegacyRunnerPolicy.IntentionalSettlementProven
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

function Invoke-PortableCorrectedCanaryEvidenceSelfTest {
	$selfTestHeadRows = @(& git -C $root rev-parse HEAD 2>$null)
	$selfTestHead = ($selfTestHeadRows -join '').Trim()
	if ($LASTEXITCODE -ne 0 -or $selfTestHead -cnotmatch '^[0-9a-f]{40}$') {
		throw "Portable corrected-canary consumer self-test cannot resolve checkout HEAD."
	}
	foreach ($toolPath in @(
			"tools/run-guarded-campaign-debug.ps1",
			"tools/Partisan.ReleaseCandidate.psm1",
			"tools/New-PartisanCampaignDebugReleaseIndex.ps1",
			"tools/update-release-docs.ps1")) {
		$currentToolPath = Join-Path $root $toolPath
		$currentToolSha = (Get-FileHash `
			-LiteralPath $currentToolPath `
			-Algorithm SHA256).Hash.ToLowerInvariant()
		$committedToolSha = Get-GitBlobSha256 `
			$selfTestHead $toolPath "Portable corrected-canary self-test $toolPath"
		if ($currentToolSha -cne $committedToolSha) {
			throw "Portable corrected-canary consumer self-test requires stationary committed tool bytes: $toolPath"
		}
	}

	$producerSelfTestRows = @(. (Join-Path $PSScriptRoot `
		"test-partisan-campaign-debug-corrected-canary-release-index.ps1"))
	if ($producerSelfTestRows.Count -ne 1 -or
		[string] (Get-ObjectPropertyValue $producerSelfTestRows[0] "status") -cne
			"passed") {
		throw "Portable corrected-canary producer self-test did not pass."
	}

	$tempLeaf = ".ric-" +
		[Guid]::NewGuid().ToString("N").Substring(0, 12) + ".tmp"
	$tempRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot $tempLeaf))
	$toolsRoot = [IO.Path]::GetFullPath($PSScriptRoot).TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar)
	$toolsPrefix = $toolsRoot + [IO.Path]::DirectorySeparatorChar
	if (-not $tempRoot.StartsWith(
			$toolsPrefix,
			[StringComparison]::OrdinalIgnoreCase) -or
		(Split-Path -Leaf $tempRoot) -cnotmatch
			'^\.ric-[0-9a-f]{12}\.tmp$') {
		throw "Portable corrected-canary consumer self-test containment failed."
	}
	$tempRepoRelative = "tools/" + $tempLeaf
	$ignoredRows = @(& git -C $root check-ignore -- $tempRepoRelative 2>$null)
	if ($LASTEXITCODE -ne 0 -or $ignoredRows.Count -ne 1 -or
		[string] $ignoredRows[0] -cne $tempRepoRelative) {
		throw "Portable corrected-canary consumer self-test requires an ignored temporary root."
	}
	if (Test-Path -LiteralPath $tempRoot) {
		throw "Portable corrected-canary consumer self-test temporary root already exists."
	}
	$tempRootCreated = $false
	try {
		New-Item -ItemType Directory -Path $tempRoot | Out-Null
		$tempRootCreated = $true
		Assert-FocusedNoReparseAncestry `
			$toolsRoot $tempRoot "Portable corrected-canary consumer self-test root"

	$externalRoot = Join-Path $tempRoot "e"
	$fixtureParent = Join-Path $externalRoot `
		("{0}/campaign-debug" -f $candidateId)
	$repoPath = [IO.Path]::GetFullPath($root).TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar)
	$repoPrefix = $repoPath + [IO.Path]::DirectorySeparatorChar
	$statusAsOfForTest = [DateTimeOffset]::Parse(
		"2026-07-19T13:00:00Z",
		[Globalization.CultureInfo]::InvariantCulture,
		[Globalization.DateTimeStyles]::AssumeUniversal -bor
			[Globalization.DateTimeStyles]::AdjustToUniversal)

	$newAdmissionFixture = {
		param([ValidateSet(
			"green", "proof-red", "blocked-parent-red",
			"unexpected-blocker", "certifying-blocker")] [string] $Mode)

		$fixture = New-CorrectedCanaryFixture `
			-FixtureRoot $fixtureParent `
			-Mode $Mode
		[void] (Invoke-Producer $fixture)
		$run = Get-Content -Raw -LiteralPath $fixture.RunPath | ConvertFrom-Json
		$index = Get-Content -Raw -LiteralPath $fixture.IndexPath | ConvertFrom-Json

		$runSha = (Get-FileHash `
			-LiteralPath $fixture.RunPath `
			-Algorithm SHA256).Hash.ToLowerInvariant()
		$indexSha = (Get-FileHash `
			-LiteralPath $fixture.IndexPath `
			-Algorithm SHA256).Hash.ToLowerInvariant()

		$indexFullPath = [IO.Path]::GetFullPath($fixture.IndexPath)
		if (-not $indexFullPath.StartsWith(
				$repoPrefix,
				[StringComparison]::OrdinalIgnoreCase)) {
			throw "Portable corrected-canary self-test index escaped the repository."
		}
		$summaryPath = $indexFullPath.Substring($repoPrefix.Length).Replace("\", "/")
		$manifestPath = Join-Path $fixture.Bundle "identity/candidate.json"
		$readyPath = Join-Path $fixture.Bundle "identity/candidate.ready.json"
		$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
		$manifestSha = (Get-FileHash `
			-LiteralPath $manifestPath `
			-Algorithm SHA256).Hash.ToLowerInvariant()
		$readySha = (Get-FileHash `
			-LiteralPath $readyPath `
			-Algorithm SHA256).Hash.ToLowerInvariant()
		$candidate = [PSCustomObject] @{
			CandidateId = [string] $index.candidate.candidateId
			CandidateSourceHead = [string] $index.candidate.candidateSourceHead
			PackageHashAlgorithm = [string] $index.candidate.packageHashAlgorithm
			PackageSha256 = [string] $index.candidate.packageSha256
			PackageVersion = [string] $index.candidate.candidateVersion
			ManifestSha256 = $manifestSha
			ReadySha256 = $readySha
			WorkbenchCrc = [string] $index.candidate.workbenchCrc
			CampaignSchema = [int] $index.candidate.campaignSchema
			RuntimeSettingsSchema = [int] $index.candidate.runtimeSettingsSchema
			Manifest = $manifest
		}
		$unapprovedKind = @(@($index.diagnostics.unapprovedHardDiagnosticKinds) |
			ForEach-Object { [string] $_.kind } |
			Sort-Object -CaseSensitive) -join ","
		$evidence = [PSCustomObject] [ordered] @{
			status = [string] $index.result.status
			summaryPath = $summaryPath
			summarySha256 = $indexSha
			candidateId = [string] $index.candidate.candidateId
			candidateSourceHead = [string] $index.candidate.candidateSourceHead
			packageSha256 = [string] $index.candidate.packageSha256
			manifestSha256 = $manifestSha
			readySha256 = $readySha
			settingsSha256 = [string] $index.settings.sha256
			harnessGitHead = [string] $index.harness.gitHead
			campaignRunnerSha256 = [string] $index.harness.campaignRunnerSha256
			candidateModuleSha256 = [string] $index.harness.candidateModuleSha256
			runLeafId = [string] $index.capture.runLeafId
			runId = [string] $index.capture.runId
			startedUtc = [string] $index.capture.startedUtc
			completedUtc = [string] $index.capture.completedUtc
			runtimeSeconds = [int] $index.capture.runtimeSeconds
			outcomeSuccess = [bool] $index.result.guardedRunSucceeded
			error = [string] $index.result.error
			caseCount = [int] $index.proof.caseCount
			pass = [int] $index.proof.pass
			warn = [int] $index.proof.warn
			fail = [int] $index.proof.fail
			blocked = [int] $index.proof.blocked
			skipped = [int] $index.proof.skipped
			focusedAssertionCount = [int] $index.proof.focusedAssertionCount
			focusedAssertionsPassed = [int] $index.proof.focusedAssertionsPassed
			certificationRequired = [int] $index.proof.certificationRequired
			certificationProven = [int] $index.proof.certificationProven
			stateDiffRows = [int] $index.proof.stateDiffRows
			nonzeroStateDiffRows = [int] $index.proof.nonzeroStateDiffRows
			finalOrphanCleanupPass = [bool] $index.proof.finalOrphanCleanupPass
			diagnosticClassificationValid = [bool] $index.diagnostics.valid
			hardDiagnosticFree = [bool] $index.diagnostics.hardDiagnosticFree
			hardDiagnosticCount = [int] $index.diagnostics.hardDiagnosticCount
			scriptErrors = [int] $index.diagnostics.scriptErrors
			engineErrors = [int] $index.diagnostics.engineErrors
			partisanErrors = [int] $index.diagnostics.partisanErrors
			approvedStockDiagnosticCount =
				[int] $index.diagnostics.approvedStockDiagnosticCount
			approvedIntentionalDiagnosticCount =
				[int] $index.diagnostics.approvedIntentionalDiagnosticCount
			unapprovedHardDiagnosticCount =
				[int] $index.diagnostics.unapprovedHardDiagnosticCount
			unapprovedHardDiagnosticKind = $unapprovedKind
			classifierSelfTestCount = [int] $index.diagnostics.classifierSelfTestCount
			canonicalScriptLogCount = [int] $index.diagnostics.canonicalScriptLogCount
			canonicalConsoleLogCount = [int] $index.diagnostics.canonicalConsoleLogCount
			canonicalLogPairSameDirectory =
				[bool] $index.diagnostics.canonicalLogPairSameDirectory
			cleanupAndSpillZero = $true
			envelopeFileCount = [int] $index.integrity.envelopeFileCount
			envelopeFilesRehashed = [bool] $index.integrity.envelopeFilesRehashed
			envelopeSha256 = $runSha
			runSummarySha256 = $runSha
			acceptanceDisposition = [string] $index.result.acceptanceDisposition
			summary = "Synthetic portable corrected-canary consumer fixture."
		}
		$trustedTools = [PSCustomObject] @{
			gitHead = [string] $index.harness.gitHead
			campaignRunnerGitBlobSha256 =
				[string] $index.harness.campaignRunnerGitBlobSha256
			candidateModuleGitBlobSha256 =
				[string] $index.harness.candidateModuleGitBlobSha256
			releaseIndexProducerGitBlobSha256 =
				[string] $index.harness.releaseIndexProducerGitBlobSha256
			releaseDocsConsumerGitBlobSha256 =
				[string] $index.harness.releaseDocsConsumerGitBlobSha256
		}
		return [PSCustomObject] @{
			Fixture = $fixture
			Run = $run
			Index = $index
			Evidence = $evidence
			Candidate = $candidate
			TrustedTools = $trustedTools
		}
	}

	$assertRejected = {
		param(
			[string] $Label,
			[scriptblock] $Action,
			[string] $ExpectedMessage = ""
		)
		$rejected = $false
		try {
			& $Action
		}
		catch {
			if (-not [string]::IsNullOrWhiteSpace($ExpectedMessage) -and
				[string] $_.Exception.Message -cnotmatch $ExpectedMessage) {
				throw "Portable corrected-canary consumer self-test $Label failed unexpectedly: $($_.Exception.Message)"
			}
			$rejected = $true
		}
		if (-not $rejected) {
			throw "Portable corrected-canary consumer self-test admitted $Label."
		}
	}
	$rewriteAdmission = {
		param(
			[object] $Admission,
			[bool] $RunChanged
		)

		if ($RunChanged) {
			Write-Json $Admission.Fixture.RunPath $Admission.Run
			$mutatedRunSha = (Get-FileHash `
				-LiteralPath $Admission.Fixture.RunPath `
				-Algorithm SHA256).Hash.ToLowerInvariant()
			$Admission.Index.source.runEnvelopeSha256 = $mutatedRunSha
			$Admission.Index.integrity.envelopeSha256 = $mutatedRunSha
			$Admission.Index.integrity.runSummarySha256 = $mutatedRunSha
			$Admission.Evidence.envelopeSha256 = $mutatedRunSha
			$Admission.Evidence.runSummarySha256 = $mutatedRunSha
		}
		Write-Json $Admission.Fixture.IndexPath $Admission.Index
		$Admission.Evidence.summarySha256 = (Get-FileHash `
			-LiteralPath $Admission.Fixture.IndexPath `
			-Algorithm SHA256).Hash.ToLowerInvariant()
	}

		$legacyEe0CandidateId =
			"partisan-rc-ee0e8add2a29-20260719T063815Z"
		$ledgerStatus = Get-Content -Raw -LiteralPath (
			Join-Path $root "docs/data/release_status.json") | ConvertFrom-Json
		$legacyEe0Candidate = $null
		$legacyEe0Evidence = $null
		if ([string] $ledgerStatus.artifact.candidateId -ceq
				$legacyEe0CandidateId) {
			$legacyEe0Candidate = $ledgerStatus.artifact
			$legacyEe0Evidence = $ledgerStatus.evidence.correctedForceAuthorityCanary
		}
		else {
			foreach ($historicalEntry in @($ledgerStatus.historicalCandidateEvidence)) {
				if ([string] $historicalEntry.candidate.candidateId -ceq
						$legacyEe0CandidateId) {
					$legacyEe0Candidate = $historicalEntry.candidate
					$legacyEe0Evidence =
						$historicalEntry.evidence.correctedForceAuthorityCanary
					break
				}
			}
		}
		if ($null -eq $legacyEe0Candidate -or $null -eq $legacyEe0Evidence) {
			throw "Portable corrected-canary self-test cannot locate the pinned ee0 ledger tuple."
		}
		$legacyEe0Identity = Get-RetainedCandidateIdentity `
			$legacyEe0Candidate "self-test pinned ee0 candidate"
		$legacyEe0History = Assert-HistoricalCorrectedCanaryEvidence `
			$legacyEe0Evidence `
			$legacyEe0Identity `
			"rejected-after-full-profile" `
			"self-test pinned ee0 corrected-canary history" `
			$statusAsOfForTest `
			$legacyEe0Identity.RuntimeSettingsSchema
		if ([string] $legacyEe0History.Status -cne "passed-noncertifying" -or
			[string] $legacyEe0History.Outcome -cne "accepted" -or
			$legacyEe0History.Validation.LegacySchema1 -isnot [bool] -or
			-not [bool] $legacyEe0History.Validation.LegacySchema1) {
			throw "Portable corrected-canary self-test did not preserve the pinned ee0 schema-1 history."
		}

		Assert-NoLocalAbsolutePathValue `
			"https://example.invalid/evidence/release-index.json" `
			"self-test ordinary HTTPS evidence URL"
		$urlBase = "https://example.invalid/"
		$driveLetter = [string] [char] 0x43
		$colon = [string] [char] 0x3a
		$slash = [string] [char] 0x2f
		$percent = [string] [char] 0x25
		$usersSegment = "Us" + "ers"
		$homeSegment = "ho" + "me"
		$wrappedLocalPaths = @(
			$urlBase + $driveLetter + $colon + $slash + $usersSegment +
				$slash + "example" + $slash + "evidence.json",
			$urlBase + $driveLetter + $percent + "3A" + $percent + "2F" +
				$usersSegment + $percent + "2F" + "example" + $percent +
				"2F" + "evidence.json",
			$urlBase + $homeSegment + $slash + "example" + $slash +
				"evidence.json")
		foreach ($wrappedLocalPath in $wrappedLocalPaths) {
			& $assertRejected "URL-wrapped local path $wrappedLocalPath" {
				Assert-NoLocalAbsolutePathValue $wrappedLocalPath `
					"self-test URL-wrapped local path"
			}
		}

		$stateDiffRunId = "seed1985_t0_p1_u1784445266"
		$stateDiffLines = New-Object Collections.Generic.List[string]
		[void] $stateDiffLines.Add("Partisan campaign debug state diff")
		[void] $stateDiffLines.Add("run $stateDiffRunId")
		foreach ($stateDiffLabel in @(Get-ExactCampaignDebugStateDiffLabels)) {
			[void] $stateDiffLines.Add("$stateDiffLabel 0 -> 0 | delta 0")
		}
		$validStateDiffText = ($stateDiffLines.ToArray() -join "`n") + "`n"
		$validStateDiff = Assert-ExactCampaignDebugStateDiff `
			-Text $validStateDiffText `
			-RunId $stateDiffRunId `
			-Label "self-test valid state diff"
		if ([int] $validStateDiff.RowCount -ne 18 -or
			[int] $validStateDiff.NonzeroRowCount -ne 0) {
			throw "Portable corrected-canary state-diff oracle rejected its valid fixture."
		}
		$stateDiffRejections = [ordered] @{
			"wrong first state-diff header" = $validStateDiffText.Replace(
				"Partisan campaign debug state diff", "Partisan campaign state diff")
			"wrong second state-diff header" = $validStateDiffText.Replace(
				"run $stateDiffRunId", "run seed0_t0_p0_u0")
			"missing state-diff row" = $validStateDiffText.Replace(
				"money 0 -> 0 | delta 0`n", "")
			"extra state-diff content" = $validStateDiffText + "extra`n"
			"state-diff label drift" = $validStateDiffText.Replace(
				"money 0 -> 0 | delta 0", "Money 0 -> 0 | delta 0")
			"noninteger state-diff value" = $validStateDiffText.Replace(
				"money 0 -> 0 | delta 0", "money zero -> 0 | delta 0")
			"contradictory state-diff arithmetic" = $validStateDiffText.Replace(
				"money 0 -> 0 | delta 0", "money 0 -> 1 | delta 0")
			"nonzero state-diff delta" = $validStateDiffText.Replace(
				"money 0 -> 0 | delta 0", "money 0 -> 1 | delta 1")
		}
		foreach ($stateDiffRejection in $stateDiffRejections.GetEnumerator()) {
			$stateDiffRejectionText = [string] $stateDiffRejection.Value
			& $assertRejected ([string] $stateDiffRejection.Key) {
				Assert-ExactCampaignDebugStateDiff `
					-Text $stateDiffRejectionText `
					-RunId $stateDiffRunId `
					-Label "self-test rejected state diff" | Out-Null
			}
		}

		$unattachedLegacyEvidence = [PSCustomObject] @{
			status = "historical-failed-unapproved-hard-diagnostic"
			summaryPath = "docs/evidence/campaign-debug/partisan-rc-b8deddc4b631-20260718T213322Z-corrected-canary-20260719T001339Z.json"
			summarySha256 = "2f9656ecf6e8640f4203e231dfe531d0b5eda017619e4f0d83a3e823e66d6951"
			harnessGitHead = "38a094fe223232145801bd60e707adf0c80c13d2"
			campaignRunnerSha256 = "56de386cbdb5677f4315c99e690eefa7158cc6e00e043ded221cec04bde74479"
		}
		$unattachedLegacyCandidate = [PSCustomObject] @{
			CandidateId = "partisan-rc-b8deddc4b631-20260718T213322Z"
		}
		& $assertRejected "unattached exact schema-1 corrected-canary tuple" {
			Assert-CorrectedCanaryEvidence `
				$unattachedLegacyEvidence `
				$unattachedLegacyCandidate `
				"historical-failed-unapproved-hard-diagnostic" `
				"rejected" `
				"self-test unattached legacy corrected canary" `
				$statusAsOfForTest `
				71
		}
		& $assertRejected "unattached schema-1 tuple at active consumer boundary" {
			Assert-ActiveCorrectedCanaryEvidence `
				$unattachedLegacyEvidence `
				$unattachedLegacyCandidate `
				"self-test active unattached legacy corrected canary" `
				$statusAsOfForTest `
				71
		}

		$green = & $newAdmissionFixture "green"
		$greenValidation = Assert-ActiveCorrectedCanaryEvidence `
			$green.Evidence `
			$green.Candidate `
			"self-test portable corrected canary green" `
			$statusAsOfForTest `
			$green.Candidate.RuntimeSettingsSchema `
			-AllowUntrackedSummaryForSelfTest `
			-TrustedToolBindingsForSelfTest $green.TrustedTools `
			-PortableEvidenceRoot $externalRoot
		$greenWarningRows = @($green.Index.proof.warningAssertions)
		$greenBlockedRows = @($green.Index.proof.blockedAssertions)
		$greenExternalAdvisoryIds = @($greenValidation.ExternalRequiredAdvisoryIds)
		if (-not $greenValidation.AcceptedCorrectedCanary -or
			$greenValidation.Rejected -or
			[string] $greenValidation.Status -cne "passed-noncertifying" -or
			[int] $greenValidation.CaseCount -ne 11 -or
			[int] $greenValidation.Pass -ne 9 -or
			[int] $greenValidation.Warn -ne 1 -or
			[int] $greenValidation.Fail -ne 0 -or
			[int] $greenValidation.Blocked -ne 1 -or
			[int] $greenValidation.Skipped -ne 0 -or
			[int] $greenValidation.AssertionCount -ne 91 -or
			[int] $greenValidation.FocusedAssertionsPassed -ne 35 -or
			[int] $greenValidation.CertificationRequired -ne 87 -or
			[int] $greenValidation.CertificationProven -ne 87 -or
			[int] $green.Index.proof.certificationFail -ne 0 -or
			[int] $green.Index.proof.certificationBlocked -ne 0 -or
			[int] $green.Index.proof.certificationWarn -ne 0 -or
			[bool] $green.Index.result.certificationPassed -or
			-not [bool] $green.Index.proof.correctedCanaryWarningContractExact -or
			-not [bool] $green.Index.proof.correctedCanaryBlockedContractExact -or
			$greenWarningRows.Count -ne 1 -or
			[string] $greenWarningRows[0].id -cne "cleanup.player_marker.live" -or
			[string] $greenWarningRows[0].caseId -cne
				"cleanup.player_marker_completion" -or
			[bool] $greenWarningRows[0].countsTowardCertification -or
			$greenBlockedRows.Count -ne 1 -or
			[string] $greenBlockedRows[0].id -cne "isolation.world_scope" -or
			[string] $greenBlockedRows[0].caseId -cne
				"cleanup.state_isolation_restore" -or
			[bool] $greenBlockedRows[0].countsTowardCertification -or
			$greenExternalAdvisoryIds.Count -ne 1 -or
			[string] $greenExternalAdvisoryIds[0] -cne "isolation.world_scope" -or
			[int] $greenValidation.StateDiffRows -ne 18 -or
			-not $greenValidation.FinalOrphanCleanupPass -or
			-not $greenValidation.DiagnosticAxisPassed) {
			throw "Portable corrected-canary green consumer admission self-test failed."
		}
		$portableBundlePackageTamperManifest = $green.Candidate.Manifest |
			ConvertTo-Json -Depth 100 | ConvertFrom-Json
		$portableBundlePackageTamperManifest.package.files[0].indexPath =
			"Partisan/noncanonical-addon.gproj"
		$portableBundlePackageRows = @(
			$portableBundlePackageTamperManifest.package.files |
				Sort-Object indexPath -CaseSensitive |
				ForEach-Object {
					([string] $_.sha256).ToLowerInvariant() + "`t" +
					([long] $_.length).ToString(
						[Globalization.CultureInfo]::InvariantCulture) + "`t" +
					[string] $_.indexPath + "`n"
				}) -join ""
		$portableBundlePackageSha = Get-ByteArraySha256 `
			([Text.Encoding]::UTF8.GetBytes($portableBundlePackageRows))
		$portableBundlePackageTamperManifest.package.sha256 =
			$portableBundlePackageSha
		$portableBundlePackageTamperIdentity = [PSCustomObject] @{
			Manifest = $portableBundlePackageTamperManifest
			PackageSha256 = $portableBundlePackageSha
		}
		& $assertRejected "portable retained bundle package inventory tamper" {
			Assert-ExactPartisanCandidatePackageInventory `
				$portableBundlePackageTamperIdentity `
				"self-test portable retained bundle candidate" | Out-Null
		} 'package inventory tuples'
		$greenHistory = Assert-HistoricalCorrectedCanaryEvidence `
			$green.Evidence `
			$green.Candidate `
			"rejected-after-full-profile" `
			"self-test portable corrected canary green history" `
			$statusAsOfForTest `
			$green.Candidate.RuntimeSettingsSchema `
			-AllowUntrackedSummaryForSelfTest `
			-TrustedToolBindingsForSelfTest $green.TrustedTools `
			-PortableEvidenceRoot $externalRoot
		if ([string] $greenHistory.Outcome -cne "accepted" -or
			-not $greenHistory.Validation.AcceptedCorrectedCanary -or
			$greenHistory.Validation.Rejected -or
			$greenHistory.Validation.LegacySchema1 -isnot [bool] -or
			[bool] $greenHistory.Validation.LegacySchema1) {
			throw "Portable corrected-canary green historical transition self-test failed."
		}
		& $assertRejected "accepted portable canary at corrected-canary retirement" {
			Assert-HistoricalCorrectedCanaryEvidence `
				$green.Evidence `
				$green.Candidate `
				"rejected-after-corrected-canary" `
				"self-test portable corrected canary green mismatched history" `
				$statusAsOfForTest `
				$green.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $green.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		$blockedContractRedCases = @(
			[PSCustomObject] @{
				Mode = "blocked-parent-red"
				CertificationRequired = 87
				CertificationBlocked = 0
			},
			[PSCustomObject] @{
				Mode = "unexpected-blocker"
				CertificationRequired = 87
				CertificationBlocked = 0
			},
			[PSCustomObject] @{
				Mode = "certifying-blocker"
				CertificationRequired = 88
				CertificationBlocked = 1
			})
		foreach ($blockedContractRedCase in $blockedContractRedCases) {
			$blockedContractRed = & $newAdmissionFixture `
				([string] $blockedContractRedCase.Mode)
			$blockedContractRedValidation = Assert-ActiveCorrectedCanaryEvidence `
				$blockedContractRed.Evidence `
				$blockedContractRed.Candidate `
				("self-test portable corrected canary {0}" -f
					[string] $blockedContractRedCase.Mode) `
				$statusAsOfForTest `
				$blockedContractRed.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $blockedContractRed.TrustedTools `
				-PortableEvidenceRoot $externalRoot
			if (-not $blockedContractRedValidation.Rejected -or
				$blockedContractRedValidation.AcceptedCorrectedCanary -or
				[string] $blockedContractRedValidation.Status -cne
					"failed-corrected-canary" -or
				[string] $blockedContractRedValidation.AcceptanceDisposition -cne
					"rejected-corrected-canary" -or
				[int] $blockedContractRedValidation.Pass -ne 9 -or
				[int] $blockedContractRedValidation.Warn -ne 1 -or
				[int] $blockedContractRedValidation.Fail -ne 0 -or
				[int] $blockedContractRedValidation.Blocked -ne 1 -or
				[int] $blockedContractRedValidation.Skipped -ne 0 -or
				[int] $blockedContractRedValidation.CertificationRequired -ne
					[int] $blockedContractRedCase.CertificationRequired -or
				[int] $blockedContractRedValidation.CertificationProven -ne 87 -or
				[int] $blockedContractRed.Index.proof.certificationBlocked -ne
					[int] $blockedContractRedCase.CertificationBlocked -or
				[bool] $blockedContractRed.Index.proof.correctedCanaryBlockedContractExact) {
				throw ("Portable corrected-canary {0} semantic rejection self-test failed." -f
					[string] $blockedContractRedCase.Mode)
			}
		}

		$wrongAssertionCount = & $newAdmissionFixture "green"
		$wrongAssertionCount.Index.proof.assertionCount = 90
		& $rewriteAdmission $wrongAssertionCount $false
		& $assertRejected "wrong proof assertion census" {
			Assert-ActiveCorrectedCanaryEvidence `
				$wrongAssertionCount.Evidence $wrongAssertionCount.Candidate `
				"self-test portable corrected canary wrong assertion census" `
				$statusAsOfForTest $wrongAssertionCount.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $wrongAssertionCount.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		$warningExtraProperty = & $newAdmissionFixture "green"
		$warningExtraProperty.Index.proof.warningAssertions[0] | Add-Member `
			-NotePropertyName harmlessExtra `
			-NotePropertyValue "must be rejected"
		& $rewriteAdmission $warningExtraProperty $false
		& $assertRejected "warning assertion extra property" {
			Assert-ActiveCorrectedCanaryEvidence `
				$warningExtraProperty.Evidence $warningExtraProperty.Candidate `
				"self-test portable corrected canary warning extra property" `
				$statusAsOfForTest $warningExtraProperty.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $warningExtraProperty.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		$warningLinkageTamper = & $newAdmissionFixture "green"
		$warningLinkageTamper.Index.proof.warningAssertions[0].actual =
			[string] $warningLinkageTamper.Index.proof.warningAssertions[0].actual +
			" | tampered"
		& $rewriteAdmission $warningLinkageTamper $false
		& $assertRejected "warning assertion raw-linkage tamper" {
			Assert-ActiveCorrectedCanaryEvidence `
				$warningLinkageTamper.Evidence $warningLinkageTamper.Candidate `
				"self-test portable corrected canary warning raw linkage" `
				$statusAsOfForTest $warningLinkageTamper.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $warningLinkageTamper.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		$blockedExtraProperty = & $newAdmissionFixture "green"
		$blockedExtraProperty.Index.proof.blockedAssertions[0] | Add-Member `
			-NotePropertyName harmlessExtra `
			-NotePropertyValue "must be rejected"
		& $rewriteAdmission $blockedExtraProperty $false
		& $assertRejected "blocked assertion extra property" {
			Assert-ActiveCorrectedCanaryEvidence `
				$blockedExtraProperty.Evidence $blockedExtraProperty.Candidate `
				"self-test portable corrected canary blocked extra property" `
				$statusAsOfForTest $blockedExtraProperty.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $blockedExtraProperty.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		$blockedLinkageTamper = & $newAdmissionFixture "green"
		$blockedLinkageTamper.Index.proof.blockedAssertions[0].actual =
			[string] $blockedLinkageTamper.Index.proof.blockedAssertions[0].actual +
			" | tampered"
		& $rewriteAdmission $blockedLinkageTamper $false
		& $assertRejected "blocked assertion raw-linkage tamper" {
			Assert-ActiveCorrectedCanaryEvidence `
				$blockedLinkageTamper.Evidence $blockedLinkageTamper.Candidate `
				"self-test portable corrected canary blocked raw linkage" `
				$statusAsOfForTest $blockedLinkageTamper.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $blockedLinkageTamper.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		$blockedProofBindingTamper = & $newAdmissionFixture "green"
		$blockedProofBindingTamper.Index.proof.correctedCanaryBlockedContractExact =
			$false
		& $rewriteAdmission $blockedProofBindingTamper $false
		& $assertRejected "blocked-contract proof binding tamper" {
			Assert-ActiveCorrectedCanaryEvidence `
				$blockedProofBindingTamper.Evidence `
				$blockedProofBindingTamper.Candidate `
				"self-test portable corrected canary blocked proof binding" `
				$statusAsOfForTest `
				$blockedProofBindingTamper.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $blockedProofBindingTamper.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		$recordedBlockedBindingTamper = & $newAdmissionFixture "green"
		$recordedBlockedBindingTamper.Run.outcome.validation.CorrectedCanaryBlockedContractExact =
			$false
		& $rewriteAdmission $recordedBlockedBindingTamper $true
		& $assertRejected "recorded blocked-contract semantic binding tamper" {
			Assert-ActiveCorrectedCanaryEvidence `
				$recordedBlockedBindingTamper.Evidence `
				$recordedBlockedBindingTamper.Candidate `
				"self-test portable corrected canary recorded blocked binding" `
				$statusAsOfForTest `
				$recordedBlockedBindingTamper.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $recordedBlockedBindingTamper.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		$extraIndex = & $newAdmissionFixture "green"
		$extraIndex.Index.finding | Add-Member `
			-NotePropertyName harmlessExtra `
			-NotePropertyValue "must be rejected"
		& $rewriteAdmission $extraIndex $false
		& $assertRejected "harmless extra nested release-index field" {
			Assert-ActiveCorrectedCanaryEvidence `
				$extraIndex.Evidence $extraIndex.Candidate `
				"self-test portable corrected canary extra index field" `
				$statusAsOfForTest $extraIndex.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $extraIndex.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		$duplicateIndex = & $newAdmissionFixture "green"
		$duplicateIndexText = [IO.File]::ReadAllText(
			$duplicateIndex.Fixture.IndexPath,
			[Text.Encoding]::UTF8)
		$findingMatches = @([regex]::Matches(
			$duplicateIndexText,
			'"finding"\s*:\s*\{'))
		if ($findingMatches.Count -ne 1) {
			throw "Portable corrected-canary duplicate-key fixture could not locate finding."
		}
		$findingStatusJson = [string] ($duplicateIndex.Index.finding.status |
			ConvertTo-Json -Compress)
		$findingMatch = $findingMatches[0]
		$duplicatePropertyText = $findingMatch.Value + "`n" +
			('    "status": {0},' -f $findingStatusJson)
		$duplicateIndexText = $duplicateIndexText.Remove(
			$findingMatch.Index,
			$findingMatch.Length).Insert(
			$findingMatch.Index,
			$duplicatePropertyText)
		[IO.File]::WriteAllText(
			$duplicateIndex.Fixture.IndexPath,
			$duplicateIndexText,
			(New-Object Text.UTF8Encoding($false)))
		$duplicateIndex.Evidence.summarySha256 = (Get-FileHash `
			-LiteralPath $duplicateIndex.Fixture.IndexPath `
			-Algorithm SHA256).Hash.ToLowerInvariant()
		& $assertRejected "harmless duplicate release-index property" {
			Assert-ActiveCorrectedCanaryEvidence `
				$duplicateIndex.Evidence $duplicateIndex.Candidate `
				"self-test portable corrected canary duplicate index property" `
				$statusAsOfForTest $duplicateIndex.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $duplicateIndex.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		$extraRun = & $newAdmissionFixture "green"
		$extraRun.Run.cleanup | Add-Member `
			-NotePropertyName harmlessExtra `
			-NotePropertyValue 0
		& $rewriteAdmission $extraRun $true
		& $assertRejected "harmless extra nested run-envelope field" {
			Assert-ActiveCorrectedCanaryEvidence `
				$extraRun.Evidence $extraRun.Candidate `
				"self-test portable corrected canary extra run field" `
				$statusAsOfForTest $extraRun.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $extraRun.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		$classifierContradiction = & $newAdmissionFixture "green"
		$classifierContradiction.Run.outcome.hardDiagnosticClassifierChecks = 39
		$classifierContradiction.Index.diagnostics.classifierSelfTestCount = 39
		$classifierContradiction.Evidence.classifierSelfTestCount = 39
		& $rewriteAdmission $classifierContradiction $true
		& $assertRejected "classifier count contradicting the immutable runner" {
			Assert-ActiveCorrectedCanaryEvidence `
				$classifierContradiction.Evidence $classifierContradiction.Candidate `
				"self-test portable corrected canary classifier contradiction" `
				$statusAsOfForTest `
				$classifierContradiction.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $classifierContradiction.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		$dirtyToolPair = & $newAdmissionFixture "green"
		$dirtyToolPair.Run.harness.campaignRunnerSha256 = ("0" * 64)
		$dirtyToolPair.Index.harness.campaignRunnerSha256 = ("0" * 64)
		$dirtyToolPair.Evidence.campaignRunnerSha256 = ("0" * 64)
		& $rewriteAdmission $dirtyToolPair $true
		& $assertRejected "worktree tool bytes differing from the immutable Git blob" {
			Assert-ActiveCorrectedCanaryEvidence `
				$dirtyToolPair.Evidence $dirtyToolPair.Candidate `
				"self-test portable corrected canary dirty tool pair" `
				$statusAsOfForTest $dirtyToolPair.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $dirtyToolPair.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		[IO.File]::AppendAllText(
			$green.Fixture.RawPath,
			" `n",
			(New-Object Text.UTF8Encoding($false)))
		& $assertRejected "tampered external raw artifact" {
			Assert-ActiveCorrectedCanaryEvidence `
				$green.Evidence $green.Candidate `
				"self-test portable corrected canary raw tamper" `
				$statusAsOfForTest $green.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $green.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		$red = & $newAdmissionFixture "proof-red"
		$redValidation = Assert-ActiveCorrectedCanaryEvidence `
			$red.Evidence `
			$red.Candidate `
			"self-test portable corrected canary red" `
			$statusAsOfForTest `
			$red.Candidate.RuntimeSettingsSchema `
			-AllowUntrackedSummaryForSelfTest `
			-TrustedToolBindingsForSelfTest $red.TrustedTools `
			-PortableEvidenceRoot $externalRoot
		if (-not $redValidation.Rejected -or
			$redValidation.AcceptedCorrectedCanary -or
			[string] $redValidation.Status -cne "failed-corrected-canary" -or
			[string] $redValidation.AcceptanceDisposition -cne
				"rejected-corrected-canary") {
			throw "Portable corrected-canary red consumer admission self-test failed."
		}
		$redHistory = Assert-HistoricalCorrectedCanaryEvidence `
			$red.Evidence `
			$red.Candidate `
			"rejected-after-corrected-canary" `
			"self-test portable corrected canary red history" `
			$statusAsOfForTest `
			$red.Candidate.RuntimeSettingsSchema `
			-AllowUntrackedSummaryForSelfTest `
			-TrustedToolBindingsForSelfTest $red.TrustedTools `
			-PortableEvidenceRoot $externalRoot
		if ([string] $redHistory.Outcome -cne "rejected-proof" -or
			-not $redHistory.Validation.Rejected -or
			$redHistory.Validation.AcceptedCorrectedCanary -or
			$redHistory.Validation.LegacySchema1 -isnot [bool] -or
			[bool] $redHistory.Validation.LegacySchema1) {
			throw "Portable corrected-canary red historical transition self-test failed."
		}
		& $assertRejected "rejected portable canary at post-canary retirement" {
			Assert-HistoricalCorrectedCanaryEvidence `
				$red.Evidence `
				$red.Candidate `
				"rejected-after-full-profile" `
				"self-test portable corrected canary red mismatched history" `
				$statusAsOfForTest `
				$red.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $red.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		$red.Index.policyId = "partisan-campaign-debug-corrected-canary-v2-tampered"
		Write-Json $red.Fixture.IndexPath $red.Index
		$red.Evidence.summarySha256 = (Get-FileHash `
			-LiteralPath $red.Fixture.IndexPath `
			-Algorithm SHA256).Hash.ToLowerInvariant()
		& $assertRejected "tampered corrected-canary policy" {
			Assert-ActiveCorrectedCanaryEvidence `
				$red.Evidence $red.Candidate `
				"self-test portable corrected canary policy tamper" `
				$statusAsOfForTest $red.Candidate.RuntimeSettingsSchema `
				-AllowUntrackedSummaryForSelfTest `
				-TrustedToolBindingsForSelfTest $red.TrustedTools `
				-PortableEvidenceRoot $externalRoot
		}

		Write-Host ("Portable corrected-canary consumer self-test passed: " +
			"green/red active and historical transitions admitted, and " +
			"all required fail-closed boundaries rejected.")
	}
	finally {
		if ($tempRootCreated -and
			$tempRoot.StartsWith(
				$toolsPrefix,
				[StringComparison]::OrdinalIgnoreCase) -and
			(Split-Path -Leaf $tempRoot) -cmatch
				'^\.ric-[0-9a-f]{12}\.tmp$' -and
			(Test-Path -LiteralPath $tempRoot -PathType Container)) {
			Assert-FocusedNoReparseAncestry `
				$toolsRoot $tempRoot `
				"Portable corrected-canary consumer self-test cleanup root"
			Remove-Item -LiteralPath $tempRoot -Recurse -Force
		}
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

if ($LibraryOnly) {
	return
}
$validationModeCount = @(@($Check, $SelfTest, $FocusedConsumerSelfTest) |
	Where-Object { [bool] $_ }).Count
if ($validationModeCount -gt 1) {
	throw "Use only one validation mode at a time."
}
if ($FocusedConsumerSelfTest) {
	Invoke-PortableFocusedEvidenceConsumerSelfTest
	return
}
if ($SelfTest) {
	Invoke-PortableFocusedEvidenceConsumerSelfTest
	& (Join-Path $PSScriptRoot "test-partisan-campaign-debug-release-index.ps1")
	Invoke-PortableCorrectedCanaryEvidenceSelfTest
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
	Assert-ReleaseBuildTransition `
		-RuntimeUseDisposition $runtimeUseDisposition `
		-ManifestEmbedded $manifestEmbedded `
		-CandidateSourceHead $candidateSourceHead `
		-SourceBuildSha $sourceBuildSha `
		-SourceBuildUtc $sourceBuildUtc `
		-SourceBuildLabel $sourceBuildLabel `
		-CheckoutHead $checkoutHead
	if ([string] (Get-ObjectPropertyValue $manifestCandidate "id") -cne $candidateId -or
		[string] (Get-ObjectPropertyValue $manifestCandidate "version") -cne $packageVersion -or
		[string] (Get-ObjectPropertyValue $manifestCandidate "state") -cne "retained-uncertified" -or
		[string] (Get-ObjectPropertyValue $manifestSource "gitHead") -cne $candidateSourceHead -or
		(Get-ObjectPropertyValue $manifestSource "dirty") -isnot [bool] -or
		[bool] (Get-ObjectPropertyValue $manifestSource "dirty") -or
		[string] (Get-ObjectPropertyValue $manifestSource "auditedGameplayRevision") -cne $auditedRevision -or
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
		elseif ($activeCorrectedCanaryStatus -cin @(
				"failed-proof-validation",
				"failed-corrected-canary")) {
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
		$activeCorrectedCanaryValidation = Assert-ActiveCorrectedCanaryEvidence `
			$activeCorrectedCanary `
			$activeCandidateIdentity `
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
		if ($null -eq $activeCorrectedCanaryValidation -or
			-not $activeCorrectedCanaryValidation.Accepted) {
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
	$hasRejectedRuntimeProof = $activeCorrectedCanaryStatus -cin @(
		"failed-proof-validation",
		"failed-corrected-canary") -or
		($null -ne $activeCorrectedCanaryValidation -and
			$activeCorrectedCanaryValidation.Rejected) -or
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
		$historicalFocusedTerminalHead =
			$historicalCandidateResult.PackagedFocusedValidation.HarnessGitHead
		$historicalAggregationHead = [string] (Get-ObjectPropertyValue `
			$historicalCandidateResult.PackagedFocusedValidation `
			"AggregationGitHead")
		if (-not [string]::IsNullOrWhiteSpace($historicalAggregationHead)) {
			Assert-GitAncestor `
				$historicalCandidateResult.PackagedFocusedValidation.HarnessGitHead `
				$historicalAggregationHead `
				"Historical packaged-focused raw harness is not an ancestor of its aggregation harness"
			$historicalFocusedTerminalHead = $historicalAggregationHead
		}
		Assert-GitAncestor `
			$historicalFocusedTerminalHead `
			$historicalCandidateResult.CorrectedCanaryValidation.HarnessGitHead `
			"Historical packaged-focused terminal harness is not an ancestor of its corrected-canary harness"
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
		$activeAggregationHead = [string] (Get-ObjectPropertyValue `
			$activePackagedFocusedValidation "AggregationGitHead")
		if (-not [string]::IsNullOrWhiteSpace($activeAggregationHead)) {
			Assert-GitAncestor `
				$activePackagedFocusedValidation.HarnessGitHead `
				$activeAggregationHead `
				"Active packaged-focused raw harness is not an ancestor of its aggregation harness"
			$activeTerminalHarnessHead = $activeAggregationHead
		}
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
	Add-Line $statusBuilder "| Candidate embedded implementation identity | $mdTick$($activeCandidateIdentity.EmbeddedSha)$mdTick |"
	Add-Line $statusBuilder "| Candidate embedded build UTC / label | $mdTick$($activeCandidateIdentity.EmbeddedUtc)$mdTick / $mdTick$(Escape-MarkdownCell $activeCandidateIdentity.EmbeddedLabel)$mdTick |"
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
		Add-Line $statusBuilder "- $currentEvidencePrefix corrected force-authority canary: **accepted, passed-noncertifying** on exact candidate $mdTick$candidateId${mdTick}. The focused proof passed $($activeCorrectedCanary.focusedAssertionsPassed)/$($activeCorrectedCanary.focusedAssertionCount) assertions and $($activeCorrectedCanary.certificationProven)/$($activeCorrectedCanary.certificationRequired) counted conditions; the $($activeCorrectedCanary.classifierSelfTestCount)-check classifier accepted $($activeCorrectedCanary.hardDiagnosticCount) hard diagnostics = $($activeCorrectedCanary.approvedStockDiagnosticCount) approved stock + $($activeCorrectedCanary.approvedIntentionalDiagnosticCount) approved intentional + $($activeCorrectedCanary.unapprovedHardDiagnosticCount) unapproved. All $($activeCorrectedCanary.envelopeFileCount) envelope files were rehashed with an exact $($activeCorrectedCanary.stateDiffRows)-row zero-delta state diff, final orphan cleanup, and zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $activeCorrectedCanarySummaryPath)$mdTick / SHA-256 $mdTick$activeCorrectedCanarySummarySha$mdTick; clean harness $mdTick$activeCorrectedCanaryHarnessHead${mdTick}. This scoped canary does not certify Full Campaign Debug."
	}
	else {
		Add-Line $statusBuilder "- $currentEvidencePrefix corrected force-authority canary: **rejected corrected canary** on exact candidate $mdTick$candidateId${mdTick}. Retained axes: cases $($activeCorrectedCanary.pass) PASS / $($activeCorrectedCanary.warn) WARN / $($activeCorrectedCanary.fail) FAIL / $($activeCorrectedCanary.blocked) BLOCKED / $($activeCorrectedCanary.skipped) SKIPPED; focused assertions $($activeCorrectedCanary.focusedAssertionsPassed)/$($activeCorrectedCanary.focusedAssertionCount); counted conditions $($activeCorrectedCanary.certificationProven)/$($activeCorrectedCanary.certificationRequired); diagnostic classification valid $($activeCorrectedCanary.diagnosticClassificationValid), with $($activeCorrectedCanary.hardDiagnosticCount) hard diagnostics = $($activeCorrectedCanary.approvedStockDiagnosticCount) approved stock + $($activeCorrectedCanary.approvedIntentionalDiagnosticCount) approved intentional + $($activeCorrectedCanary.unapprovedHardDiagnosticCount) unapproved; state diff $($activeCorrectedCanary.stateDiffRows) rows / $($activeCorrectedCanary.nonzeroStateDiffRows) nonzero; final orphan cleanup $($activeCorrectedCanary.finalOrphanCleanupPass); cleanup/spill zero $($activeCorrectedCanary.cleanupAndSpillZero); and $($activeCorrectedCanary.envelopeFileCount) envelope files rehashed $($activeCorrectedCanary.envelopeFilesRehashed). Summary: $mdTick$(Escape-MarkdownCell $activeCorrectedCanarySummaryPath)$mdTick / SHA-256 $mdTick$activeCorrectedCanarySummarySha$mdTick; clean harness $mdTick$activeCorrectedCanaryHarnessHead${mdTick}. Full Campaign Debug is stopped for this package; repair every retained red axis and seal a new immutable candidate."
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
		$legacySchema1 = Get-ObjectPropertyValue `
			$correctedCanaryValidation "LegacySchema1"
		$isLegacySchema1 = $false
		if ($null -ne $legacySchema1) {
			if ($legacySchema1 -isnot [bool]) {
				throw "Historical corrected-canary legacy-schema discriminator is not boolean."
			}
			$isLegacySchema1 = [bool] $legacySchema1
		}
		if ($isLegacySchema1) {
			Add-Line $statusBuilder "- Historical corrected force-authority canary: **rejected, focused proof failed** on prior exact candidate $mdTick$historicalCandidateId${mdTick}. The focused proof passed $($correctedCanary.focusedAssertionsPassed)/$($correctedCanary.focusedAssertionCount) assertions and $($correctedCanary.certificationProven)/$($correctedCanary.certificationRequired) counted conditions. The classifier remained valid at $($correctedCanary.hardDiagnosticCount) approved and $($correctedCanary.unapprovedHardDiagnosticCount) unapproved hard diagnostics. All $($correctedCanary.envelopeFileCount) envelope files rehashed with zero cleanup/spill residue. Summary: $mdTick$(Escape-MarkdownCell $correctedCanaryValidation.SummaryPath)$mdTick / SHA-256 $mdTick$($correctedCanaryValidation.SummarySha256)$mdTick; harness $mdTick$($correctedCanaryValidation.HarnessGitHead)$mdTick. This immutable rejection does not transfer to the $currentAttachmentLabel."
		}
		else {
			Add-Line $statusBuilder "- Historical corrected force-authority canary: **rejected corrected canary** on prior exact candidate $mdTick$historicalCandidateId${mdTick}. Retained axes: cases $($correctedCanary.pass) PASS / $($correctedCanary.warn) WARN / $($correctedCanary.fail) FAIL / $($correctedCanary.blocked) BLOCKED / $($correctedCanary.skipped) SKIPPED; focused assertions $($correctedCanary.focusedAssertionsPassed)/$($correctedCanary.focusedAssertionCount); counted conditions $($correctedCanary.certificationProven)/$($correctedCanary.certificationRequired); diagnostic classification valid $($correctedCanary.diagnosticClassificationValid), with $($correctedCanary.hardDiagnosticCount) hard diagnostics = $($correctedCanary.approvedStockDiagnosticCount) approved stock + $($correctedCanary.approvedIntentionalDiagnosticCount) approved intentional + $($correctedCanary.unapprovedHardDiagnosticCount) unapproved; state diff $($correctedCanary.stateDiffRows) rows / $($correctedCanary.nonzeroStateDiffRows) nonzero; final orphan cleanup $($correctedCanary.finalOrphanCleanupPass); cleanup/spill zero $($correctedCanary.cleanupAndSpillZero); and $($correctedCanary.envelopeFileCount) envelope files rehashed $($correctedCanary.envelopeFilesRehashed). Summary: $mdTick$(Escape-MarkdownCell $correctedCanaryValidation.SummaryPath)$mdTick / SHA-256 $mdTick$($correctedCanaryValidation.SummarySha256)$mdTick; harness $mdTick$($correctedCanaryValidation.HarnessGitHead)$mdTick. This immutable rejection does not transfer to the $currentAttachmentLabel."
		}
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
			Add-Line $statusBuilder "The retained candidate's corrected canary is rejected at $($activeCorrectedCanary.focusedAssertionsPassed)/$($activeCorrectedCanary.focusedAssertionCount) focused assertions and $($activeCorrectedCanary.certificationProven)/$($activeCorrectedCanary.certificationRequired) counted conditions, and its rejected-after-runtime disposition blocks further runtime consumption. Keep its valid envelope immutable, repair every retained canary defect in a new candidate, and restart focused -> corrected canary -> full from that new package."
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
