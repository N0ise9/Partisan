[CmdletBinding(DefaultParameterSetName = "Create")]
param(
    [Parameter(Mandatory = $true, ParameterSetName = "Create")]
    [Parameter(Mandatory = $true, ParameterSetName = "Check")]
    [string]$CandidateRoot,

    [Parameter(Mandatory = $true, ParameterSetName = "Create")]
    [Parameter(Mandatory = $true, ParameterSetName = "Check")]
    [string]$EvidenceRoot,

    [Parameter(Mandatory = $true, ParameterSetName = "Create")]
    [Parameter(Mandatory = $true, ParameterSetName = "Check")]
    [string]$ManifestPath,

    [Parameter(Mandatory = $true, ParameterSetName = "Create")]
    [Parameter(Mandatory = $true, ParameterSetName = "Check")]
    [string]$WorkbenchExecutable,

    [Parameter(Mandatory = $true, ParameterSetName = "Create")]
    [Parameter(Mandatory = $true, ParameterSetName = "Check")]
    [string]$ServerExecutable,

    [Parameter(Mandatory = $true, ParameterSetName = "Create")]
    [Parameter(Mandatory = $true, ParameterSetName = "Check")]
    [string]$ClientExecutable,

    [Parameter(Mandatory = $true, ParameterSetName = "Create")]
    [string]$CandidateId,

    [Parameter(Mandatory = $true, ParameterSetName = "Create")]
    [string]$CandidateVersion,

    [Parameter(Mandatory = $true, ParameterSetName = "Create")]
    [string]$ExpectedPackageSha256,

    [Parameter(Mandatory = $true, ParameterSetName = "Create")]
    [string[]]$WorkbenchResultPath,

    [Parameter(Mandatory = $true, ParameterSetName = "Check")]
    [switch]$Check,

    [Parameter(ParameterSetName = "Check")]
    [switch]$AllowUnsealedPublished,

    [Parameter(Mandatory = $true, ParameterSetName = "SelfTest")]
    [switch]$SelfTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$script:ManifestSchemaVersion = 1
$script:PackageHashAlgorithm = "sha256-manifest-v1"
$script:RequiredTargets = @("PC", "XBOX_ONE", "XBOX_SERIES", "PS4", "PS5")
$script:RequiredPackageFiles = @(
    "Partisan/addon.gproj",
    "Partisan/data.pak",
    "Partisan/resourceDatabase.rdb")

function Write-TextUtf8NoBom {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [AllowEmptyString()][Parameter(Mandatory = $true)][string]$Text
    )

    [IO.File]::WriteAllText(
        $Path,
        $Text.Replace("`r`n", "`n").Replace("`r", "`n"),
        (New-Object Text.UTF8Encoding($false)))
}

function Get-Sha256Text {
    param([AllowEmptyString()][Parameter(Mandatory = $true)][string]$Text)

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $hex = [BitConverter]::ToString(
            $algorithm.ComputeHash([Text.Encoding]::UTF8.GetBytes($Text)))
        return $hex.Replace("-", "").ToLowerInvariant()
    }
    finally {
        $algorithm.Dispose()
    }
}

function Get-FullPath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [ValidateSet("Leaf", "Container", "Any")][string]$Kind = "Any"
    )

    $full = [IO.Path]::GetFullPath($Path)
    $exists = switch ($Kind) {
        "Leaf" { Test-Path -LiteralPath $full -PathType Leaf }
        "Container" { Test-Path -LiteralPath $full -PathType Container }
        default { Test-Path -LiteralPath $full }
    }
    if (-not $exists) {
        throw "A required release-manifest path does not exist."
    }
    return $full
}

function Test-ContainedPath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Candidate,
        [switch]$AllowEqual
    )

    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $candidateFull = [IO.Path]::GetFullPath($Candidate).TrimEnd('\', '/')
    if ($AllowEqual -and $candidateFull.Equals(
        $rootFull,
        [StringComparison]::OrdinalIgnoreCase)) {
        return $true
    }
    return $candidateFull.StartsWith(
        $rootFull + [IO.Path]::DirectorySeparatorChar,
        [StringComparison]::OrdinalIgnoreCase)
}

function Assert-NoReparseTree {
    param([Parameter(Mandatory = $true)][string]$Root)

    $current = [IO.Path]::GetFullPath($Root)
    while ($current) {
        if (Test-Path -LiteralPath $current) {
            $item = Get-Item -LiteralPath $current -Force -ErrorAction Stop
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "A release-manifest path crosses a reparse point."
            }
        }
        $parent = Split-Path -Parent $current
        if ([string]::IsNullOrWhiteSpace($parent) -or
            $parent.Equals($current, [StringComparison]::OrdinalIgnoreCase)) {
            break
        }
        $current = $parent
    }
    $descendant = Get-ChildItem `
        -LiteralPath $Root `
        -Recurse `
        -Force `
        -ErrorAction Stop | Where-Object {
            ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0
        } | Select-Object -First 1
    if ($descendant) {
        throw "A release-manifest tree contains a reparse point."
    }
}

function Assert-BundleLayout {
    param(
        [Parameter(Mandatory = $true)][string]$BundleRoot,
        [switch]$ManifestRequired
    )

    $directories = @(Get-ChildItem `
        -LiteralPath $BundleRoot `
        -Directory `
        -Force `
        -ErrorAction Stop)
    $files = @(Get-ChildItem `
        -LiteralPath $BundleRoot `
        -File `
        -Force `
        -ErrorAction Stop)
    $directoryNames = @($directories | ForEach-Object { $_.Name } | Sort-Object)
    if ($directories.Count -ne 2 -or
        @(Compare-Object `
            -ReferenceObject @("evidence", "package") `
            -DifferenceObject $directoryNames `
            -CaseSensitive).Count -ne 0) {
        throw "The release bundle must contain exactly package and evidence directories."
    }
    $allowedFiles = @("candidate.ready.json", "candidate.json")
    $unexpectedFiles = @($files | Where-Object {
        $_.Name -cnotin $allowedFiles
    })
    if ($unexpectedFiles.Count -ne 0 -or
        ($ManifestRequired -and
            @($files | Where-Object { $_.Name -ceq "candidate.json" }).Count -ne 1)) {
        throw "The release bundle contains an unexpected or missing root file."
    }
}

function Get-PortableRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Path
    )

    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $pathFull = [IO.Path]::GetFullPath($Path)
    if (-not (Test-ContainedPath `
        -Root $rootFull `
        -Candidate $pathFull)) {
        throw "A release-manifest file escaped its bundle root."
    }
    $relative = $pathFull.Substring($rootFull.Length).TrimStart('\', '/')
    $portable = $relative.Replace('\', '/')
    if ([string]::IsNullOrWhiteSpace($portable) -or
        $portable.Contains(":") -or
        $portable.StartsWith("/", [StringComparison]::Ordinal) -or
        $portable.Contains("//") -or
        $portable -match '(^|/)\.\.?(/|$)') {
        throw "A release-manifest relative path is not portable."
    }
    return $portable
}

function Get-RequiredMatch {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Group,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $match = [regex]::Match($Text, $Pattern)
    if (-not $match.Success -or
        [string]::IsNullOrWhiteSpace($match.Groups[$Group].Value)) {
        throw "Could not derive $Label for the release manifest."
    }
    return $match.Groups[$Group].Value
}

function Get-ProjectIdentity {
    param([Parameter(Mandatory = $true)][string]$ProjectFile)

    $text = Get-Content -Raw -LiteralPath $ProjectFile
    $dependencyBody = Get-RequiredMatch `
        $text `
        'Dependencies\s*\{(?<body>[\s\S]*?)\}' `
        "body" `
        "project dependencies"
    $dependencies = @([regex]::Matches(
        $dependencyBody,
        '"(?<value>[0-9A-F]{16})"') | ForEach-Object {
            $_.Groups["value"].Value
        } | Sort-Object -Unique)
    if ($dependencies.Count -eq 0) {
        throw "The release-manifest project dependency set is empty."
    }
    return [ordered]@{
        id = Get-RequiredMatch $text '(?m)^\s*ID\s+"(?<value>[^"]+)"' "value" "project ID"
        guid = Get-RequiredMatch $text '(?m)^\s*GUID\s+"(?<value>[0-9A-F]{16})"' "value" "project GUID"
        title = Get-RequiredMatch $text '(?m)^\s*TITLE\s+"(?<value>[^"]+)"' "value" "project title"
        dependencies = $dependencies
    }
}

function Assert-ProjectIdentityEqual {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual
    )

    if ([string]$Expected.id -cne [string]$Actual.id -or
        [string]$Expected.guid -cne [string]$Actual.guid -or
        [string]$Expected.title -cne [string]$Actual.title -or
        @(Compare-Object `
            -ReferenceObject @($Expected.dependencies | Sort-Object) `
            -DifferenceObject @($Actual.dependencies | Sort-Object) `
            -CaseSensitive).Count -ne 0) {
        throw "The packed add-on identity does not match the source project."
    }
}

function Get-ExecutableIdentity {
    param([Parameter(Mandatory = $true)][string]$Path)

    $file = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    if ($file.Length -le 0) {
        throw "A release-manifest executable is empty."
    }
    return [ordered]@{
        fileName = $file.Name
        fileVersion = [string]$file.VersionInfo.FileVersion
        productVersion = [string]$file.VersionInfo.ProductVersion
        length = [long]$file.Length
        sha256 = (Get-FileHash `
            -LiteralPath $file.FullName `
            -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}

function Get-FileRows {
    param(
        [Parameter(Mandatory = $true)][string]$BundleRoot,
        [Parameter(Mandatory = $true)][string]$Root
    )

    Assert-NoReparseTree -Root $Root
    $files = @(Get-ChildItem `
        -LiteralPath $Root `
        -Recurse `
        -File `
        -Force `
        -ErrorAction Stop)
    $rows = New-Object Collections.Generic.List[object]
    foreach ($file in $files) {
        [void]$rows.Add([pscustomobject][ordered]@{
            path = Get-PortableRelativePath `
                -Root $BundleRoot `
                -Path $file.FullName
            length = [long]$file.Length
            sha256 = (Get-FileHash `
                -LiteralPath $file.FullName `
                -Algorithm SHA256).Hash.ToLowerInvariant()
        })
    }
    return @($rows | Sort-Object path)
}

function Get-PackageIdentity {
    param(
        [Parameter(Mandatory = $true)][string]$BundleRoot,
        [Parameter(Mandatory = $true)][string]$CandidateRoot
    )

    $addonRoot = Join-Path $CandidateRoot "Partisan"
    if (-not (Test-Path -LiteralPath $addonRoot -PathType Container)) {
        throw "The release candidate is missing its Partisan package directory."
    }
    Assert-NoReparseTree -Root $CandidateRoot
    $directories = @(Get-ChildItem `
        -LiteralPath $CandidateRoot `
        -Directory `
        -Force `
        -ErrorAction Stop)
    $parentFiles = @(Get-ChildItem `
        -LiteralPath $CandidateRoot `
        -File `
        -Force `
        -ErrorAction Stop)
    if ($directories.Count -ne 1 -or
        [string]$directories[0].Name -cne "Partisan" -or
        $parentFiles.Count -ne 0) {
        throw "The release-candidate package root must contain one exact add-on."
    }
    $childDirectories = @(Get-ChildItem `
        -LiteralPath $addonRoot `
        -Directory `
        -Force `
        -ErrorAction Stop)
    $files = @(Get-ChildItem `
        -LiteralPath $addonRoot `
        -File `
        -Force `
        -ErrorAction Stop)
    $actualNames = @($files | ForEach-Object {
        "Partisan/" + $_.Name
    } | Sort-Object)
    if ($childDirectories.Count -ne 0 -or
        $files.Count -ne 3 -or
        @(Compare-Object `
            -ReferenceObject @($script:RequiredPackageFiles | Sort-Object) `
            -DifferenceObject $actualNames `
            -CaseSensitive).Count -ne 0) {
        throw "The release candidate does not contain the exact three-file package."
    }

    $rows = New-Object Collections.Generic.List[object]
    $indexRows = New-Object Collections.Generic.List[string]
    foreach ($expectedPath in $script:RequiredPackageFiles) {
        $name = Split-Path -Leaf $expectedPath
        $file = Get-Item `
            -LiteralPath (Join-Path $addonRoot $name) `
            -Force `
            -ErrorAction Stop
        if ($file.Length -le 0) {
            throw "The release candidate contains an empty package file."
        }
        $sha = (Get-FileHash `
            -LiteralPath $file.FullName `
            -Algorithm SHA256).Hash.ToLowerInvariant()
        [void]$indexRows.Add(("{0}`t{1}`t{2}" -f
            $sha,
            [long]$file.Length,
            $expectedPath))
        [void]$rows.Add([pscustomobject][ordered]@{
            path = Get-PortableRelativePath `
                -Root $BundleRoot `
                -Path $file.FullName
            indexPath = $expectedPath
            length = [long]$file.Length
            sha256 = $sha
        })
    }
    $canonicalIndex = ($indexRows -join "`n") + "`n"
    return [pscustomobject]@{
        Root = Get-PortableRelativePath `
            -Root $BundleRoot `
            -Path $addonRoot
        HashAlgorithm = $script:PackageHashAlgorithm
        Sha256 = Get-Sha256Text -Text $canonicalIndex
        CanonicalIndex = $canonicalIndex
        Files = $rows.ToArray()
    }
}

function Get-SourceIdentity {
    param([Parameter(Mandatory = $true)][string]$RepositoryRoot)

    $buildInfo = Get-Content -Raw -LiteralPath (
        Join-Path $RepositoryRoot "Scripts/Game/HST/HST_BuildInfo.c")
    $campaignState = Get-Content -Raw -LiteralPath (
        Join-Path $RepositoryRoot "Scripts/Game/HST/State/HST_CampaignState.c")
    $runtimeSettings = Get-Content -Raw -LiteralPath (
        Join-Path $RepositoryRoot "Scripts/Game/HST/Config/HST_RuntimeSettings.c")
    $releaseStatus = Get-Content -Raw -LiteralPath (
        Join-Path $RepositoryRoot "docs/data/release_status.json") | ConvertFrom-Json

    $head = (& git -C $RepositoryRoot rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $head -cnotmatch '^[0-9a-f]{40}$') {
        throw "Could not derive the release-manifest Git HEAD."
    }
    $dirtyRows = @(& git -C $RepositoryRoot status --porcelain --untracked-files=all)
    if ($LASTEXITCODE -ne 0 -or $dirtyRows.Count -ne 0) {
        throw "The release manifest requires a clean checkout."
    }

    $embeddedSha = Get-RequiredMatch `
        $buildInfo `
        'BUILD_SHA\s*=\s*"(?<value>[0-9a-f]{40})"' `
        "value" `
        "embedded implementation SHA"
    $embeddedUtc = Get-RequiredMatch `
        $buildInfo `
        'BUILD_UTC\s*=\s*"(?<value>[^"]+)"' `
        "value" `
        "embedded build UTC"
    $embeddedLabel = Get-RequiredMatch `
        $buildInfo `
        'BUILD_LABEL\s*=\s*"(?<value>[^"]+)"' `
        "value" `
        "embedded build label"
    $campaignSchema = [int](Get-RequiredMatch `
        $campaignState `
        'static const int SCHEMA_VERSION\s*=\s*(?<value>\d+)' `
        "value" `
        "campaign schema")
    $settingsSchema = [int](Get-RequiredMatch `
        $runtimeSettings `
        'static const int SCHEMA_VERSION\s*=\s*(?<value>\d+)' `
        "value" `
        "runtime-settings schema")
    $auditedRevision = [string]$releaseStatus.auditedRevision
    if ($auditedRevision -cnotmatch '^[0-9a-f]{40}$') {
        throw "The release status audited revision is invalid."
    }

    & git -C $RepositoryRoot merge-base --is-ancestor $embeddedSha $head
    if ($LASTEXITCODE -ne 0) {
        throw "The embedded implementation SHA is not an ancestor of the candidate source."
    }
    $embeddedDistance = [int]((& git -C $RepositoryRoot rev-list `
        --count "$embeddedSha..$head").Trim())
    if ($LASTEXITCODE -ne 0) {
        throw "Could not derive the embedded implementation distance."
    }
    & git -C $RepositoryRoot merge-base --is-ancestor $auditedRevision $head
    if ($LASTEXITCODE -ne 0) {
        throw "The audited gameplay revision is not an ancestor of the candidate source."
    }
    $auditedDistance = [int]((& git -C $RepositoryRoot rev-list `
        --count "$auditedRevision..$head").Trim())
    if ($LASTEXITCODE -ne 0) {
        throw "Could not derive the audited gameplay revision distance."
    }

    return [ordered]@{
        gitHead = $head
        dirty = $false
        auditedGameplayRevision = $auditedRevision
        auditedGameplayRelation = if ($auditedDistance -eq 0) { "equal" } else { "ancestor" }
        auditedGameplayDistance = $auditedDistance
        embeddedImplementation = [ordered]@{
            sha = $embeddedSha
            utc = $embeddedUtc
            label = $embeddedLabel
            relation = if ($embeddedDistance -eq 0) { "equal" } else { "ancestor" }
            distance = $embeddedDistance
        }
        campaignSchema = $campaignSchema
        runtimeSettingsSchema = $settingsSchema
    }
}

function Get-WorkbenchRecords {
    param(
        [Parameter(Mandatory = $true)][string]$BundleRoot,
        [Parameter(Mandatory = $true)][string]$EvidenceRoot,
        [Parameter(Mandatory = $true)][string[]]$Paths
    )

    $records = New-Object Collections.Generic.List[object]
    foreach ($path in $Paths) {
        $resolved = Get-FullPath -Path $path -Kind Leaf
        if (-not (Test-ContainedPath `
            -Root $EvidenceRoot `
            -Candidate $resolved)) {
            throw "A Workbench result is outside the evidence root."
        }
        $value = Get-Content -Raw -LiteralPath $resolved | ConvertFrom-Json
        foreach ($requiredProperty in @("Result", "Cleanup")) {
            if ($value.PSObject.Properties.Name -notcontains $requiredProperty) {
                throw "A retained Workbench result is incomplete."
            }
        }
        $result = $value.Result
        $cleanup = $value.Cleanup
        $requiredResultProperties = @(
            "Target",
            "Success",
            "ExitCode",
            "TimedOut",
            "ProjectPathGuidPair",
            "ProjectId",
            "ModuleGame",
            "Files",
            "Classes",
            "Crc",
            "ScriptValidation",
            "HardErrorCount",
            "FirstHstError",
            "FirstScriptError",
            "FirstHardError")
        $requiredCleanupProperties = @(
            "CleanupPhaseErrorCount",
            "GuardRemaining",
            "GuardBaseRemaining",
            "OwnedGuardRootsRemaining",
            "OwnedProcessesRemaining",
            "NewEngineProcessesRemaining",
            "UnclaimedEngineProcessesObserved",
            "NewDefaultEntriesRemaining",
            "ModifiedDefaultFiles",
            "DeletedDefaultEntries",
            "MissingDefaultRoots",
            "ExternalSpillEntriesRemaining",
            "ModifiedSpillFiles",
            "DeletedSpillEntries",
            "MissingSpillRoots")
        foreach ($property in $requiredResultProperties) {
            if ($result.PSObject.Properties.Name -notcontains $property) {
                throw "A retained Workbench RESULT record is missing $property."
            }
        }
        foreach ($property in $requiredCleanupProperties) {
            if ($cleanup.PSObject.Properties.Name -notcontains $property) {
                throw "A retained Workbench CLEANUP record is missing $property."
            }
        }
        $target = [string]$result.Target
        if ($target -cnotin $script:RequiredTargets -or
            $result.Success -isnot [bool] -or -not $result.Success -or
            $result.TimedOut -isnot [bool] -or $result.TimedOut -or
            $result.ProjectPathGuidPair -isnot [bool] -or -not $result.ProjectPathGuidPair -or
            $result.ProjectId -isnot [bool] -or -not $result.ProjectId -or
            $result.ModuleGame -isnot [bool] -or -not $result.ModuleGame -or
            $result.ScriptValidation -isnot [bool] -or -not $result.ScriptValidation -or
            [int]$result.ExitCode -ne 0 -or
            [int]$result.Files -le 0 -or
            [int]$result.Classes -le 0 -or
            [string]$result.Crc -cnotmatch '^[0-9a-fA-F]{8}$' -or
            [int]$result.HardErrorCount -ne 0 -or
            -not [string]::IsNullOrWhiteSpace([string]$result.FirstHstError) -or
            -not [string]::IsNullOrWhiteSpace([string]$result.FirstScriptError) -or
            -not [string]::IsNullOrWhiteSpace([string]$result.FirstHardError) -or
            [int]$cleanup.CleanupPhaseErrorCount -ne 0 -or
            [int]$cleanup.GuardRemaining -ne 0 -or
            [int]$cleanup.GuardBaseRemaining -ne 0 -or
            [int]$cleanup.OwnedGuardRootsRemaining -ne 0 -or
            [int]$cleanup.OwnedProcessesRemaining -ne 0 -or
            [int]$cleanup.NewEngineProcessesRemaining -ne 0 -or
            [int]$cleanup.UnclaimedEngineProcessesObserved -ne 0 -or
            [int]$cleanup.NewDefaultEntriesRemaining -ne 0 -or
            [int]$cleanup.ModifiedDefaultFiles -ne 0 -or
            [int]$cleanup.DeletedDefaultEntries -ne 0 -or
            [int]$cleanup.MissingDefaultRoots -ne 0 -or
            [int]$cleanup.ExternalSpillEntriesRemaining -ne 0 -or
            [int]$cleanup.ModifiedSpillFiles -ne 0 -or
            [int]$cleanup.DeletedSpillEntries -ne 0 -or
            [int]$cleanup.MissingSpillRoots -ne 0) {
            throw "A retained Workbench result is not a green exact-target result."
        }
        [void]$records.Add([pscustomobject][ordered]@{
            target = $target
            status = "passed"
            files = [int]$result.Files
            classes = [int]$result.Classes
            crc = ([string]$result.Crc).ToLowerInvariant()
            evidencePath = Get-PortableRelativePath `
                -Root $BundleRoot `
                -Path $resolved
        })
    }
    $sorted = @($records | Sort-Object target)
    $targets = @($sorted | ForEach-Object { $_.target })
    if ($sorted.Count -ne $script:RequiredTargets.Count -or
        @(Compare-Object `
            -ReferenceObject @($script:RequiredTargets | Sort-Object) `
            -DifferenceObject @($targets | Sort-Object) `
            -CaseSensitive).Count -ne 0) {
        throw "The release manifest requires one result for every supported Workbench target."
    }
    $crcValues = @($sorted | ForEach-Object { $_.crc } | Sort-Object -Unique)
    if ($crcValues.Count -ne 1) {
        throw "Supported Workbench targets did not produce one common source CRC."
    }
    return [pscustomobject]@{
        Crc = $crcValues[0]
        Targets = $sorted
    }
}

function ConvertTo-CanonicalJson {
    param([Parameter(Mandatory = $true)]$Value)

    return (($Value | ConvertTo-Json -Depth 30).Replace("`r`n", "`n") + "`n")
}

function Assert-ObjectJsonEqual {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $expectedJson = ConvertTo-CanonicalJson -Value $Expected
    $actualJson = ConvertTo-CanonicalJson -Value $Actual
    if ($expectedJson -cne $actualJson) {
        throw "$Label differs from the retained release manifest."
    }
}

function Invoke-SelfTest {
    $rows = @(
        [pscustomobject]@{ Path = "Partisan/addon.gproj"; Length = 1; Sha256 = "a" * 64 },
        [pscustomobject]@{ Path = "Partisan/data.pak"; Length = 2; Sha256 = "b" * 64 },
        [pscustomobject]@{ Path = "Partisan/resourceDatabase.rdb"; Length = 3; Sha256 = "c" * 64 })
    $indexRows = @($rows | ForEach-Object {
        "{0}`t{1}`t{2}" -f $_.Sha256, $_.Length, $_.Path
    })
    $canonical = ($indexRows -join "`n") + "`n"
    $digest = Get-Sha256Text -Text $canonical
    $selfTestRepositoryRoot = [IO.Path]::GetFullPath(
        (Join-Path $PSScriptRoot ".."))
    $selfTestCampaignText = Get-Content -Raw -LiteralPath (
        Join-Path $selfTestRepositoryRoot "Scripts/Game/HST/State/HST_CampaignState.c")
    $selfTestSettingsText = Get-Content -Raw -LiteralPath (
        Join-Path $selfTestRepositoryRoot "Scripts/Game/HST/Config/HST_RuntimeSettings.c")
    $selfTestCampaignSchema = [int](Get-RequiredMatch `
        $selfTestCampaignText `
        'static const int SCHEMA_VERSION\s*=\s*(?<value>\d+)' `
        "value" `
        "self-test campaign schema")
    $selfTestSettingsSchema = [int](Get-RequiredMatch `
        $selfTestSettingsText `
        'static const int SCHEMA_VERSION\s*=\s*(?<value>\d+)' `
        "value" `
        "self-test runtime-settings schema")
    if ($digest -cnotmatch '^[0-9a-f]{64}$' -or
        $digest -cne (Get-Sha256Text -Text $canonical) -or
        $script:RequiredPackageFiles.Count -ne 3 -or
        $script:RequiredTargets.Count -ne 5 -or
        $selfTestCampaignSchema -le 0 -or
        $selfTestSettingsSchema -le 0) {
        throw "Release-manifest self-test failed."
    }
    Write-Output ("SELFTEST " + ([pscustomobject]@{
        Success = $true
        ManifestSchemaVersion = $script:ManifestSchemaVersion
        PackageHashAlgorithm = $script:PackageHashAlgorithm
        PackageFileCount = $script:RequiredPackageFiles.Count
        WorkbenchTargetCount = $script:RequiredTargets.Count
        CampaignSchema = $selfTestCampaignSchema
        RuntimeSettingsSchema = $selfTestSettingsSchema
        Digest = $digest
    } | ConvertTo-Json -Compress))
}

if ($SelfTest) {
    Invoke-SelfTest
    return
}

$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$candidateRootPath = Get-FullPath -Path $CandidateRoot -Kind Container
$evidenceRootPath = Get-FullPath -Path $EvidenceRoot -Kind Container
$manifestFullPath = [IO.Path]::GetFullPath($ManifestPath)
$bundleRoot = Split-Path -Parent $manifestFullPath
$workbenchPath = Get-FullPath -Path $WorkbenchExecutable -Kind Leaf
$serverPath = Get-FullPath -Path $ServerExecutable -Kind Leaf
$clientPath = Get-FullPath -Path $ClientExecutable -Kind Leaf

if (-not $candidateRootPath.Equals(
        (Join-Path $bundleRoot "package"),
        [StringComparison]::OrdinalIgnoreCase) -or
    -not $evidenceRootPath.Equals(
        (Join-Path $bundleRoot "evidence"),
        [StringComparison]::OrdinalIgnoreCase) -or
    (Split-Path -Leaf $manifestFullPath) -cne "candidate.json") {
    throw "The release bundle must contain package, evidence, and candidate.json siblings."
}
foreach ($root in @($bundleRoot, $candidateRootPath, $evidenceRootPath)) {
    Assert-NoReparseTree -Root $root
}
Assert-BundleLayout -BundleRoot $bundleRoot -ManifestRequired:$Check
if ((Split-Path -Leaf $workbenchPath) -cnotin @(
    "ArmaReforgerWorkbenchDiag.exe",
    "ArmaReforgerWorkbenchSteamDiag.exe") -or
    (Split-Path -Leaf $serverPath) -cne "ArmaReforgerServer.exe" -or
    (Split-Path -Leaf $clientPath) -cne "ArmaReforgerSteam.exe") {
    throw "The release manifest received unsupported toolchain executables."
}

$source = Get-SourceIdentity -RepositoryRoot $repositoryRoot
$sourceProject = Get-ProjectIdentity `
    -ProjectFile (Join-Path $repositoryRoot "addon.gproj")
$packedProject = Get-ProjectIdentity `
    -ProjectFile (Join-Path $candidateRootPath "Partisan/addon.gproj")
Assert-ProjectIdentityEqual -Expected $sourceProject -Actual $packedProject
$package = Get-PackageIdentity `
    -BundleRoot $bundleRoot `
    -CandidateRoot $candidateRootPath
$toolchain = [ordered]@{
    workbench = Get-ExecutableIdentity -Path $workbenchPath
    server = Get-ExecutableIdentity -Path $serverPath
    client = Get-ExecutableIdentity -Path $clientPath
}

if ($Check) {
    if (-not (Test-Path -LiteralPath $manifestFullPath -PathType Leaf)) {
        throw "The retained release manifest is missing."
    }
    $manifest = Get-Content -Raw -LiteralPath $manifestFullPath | ConvertFrom-Json
    if ([int]$manifest.manifestSchemaVersion -ne $script:ManifestSchemaVersion -or
        [string]$manifest.package.hashAlgorithm -cne $script:PackageHashAlgorithm) {
        throw "The retained release-manifest schema or package-hash contract is unsupported."
    }
    $candidateIdValue = [string]$manifest.candidate.id
    $candidateVersionValue = [string]$manifest.candidate.version
    $createdUtcValue = [string]$manifest.createdUtc
    $createdUtcParsed = [DateTime]::MinValue
    if ($candidateIdValue -cnotmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$' -or
        $candidateVersionValue -cnotmatch '^[0-9A-Za-z][0-9A-Za-z._-]{0,95}$' -or
        [string]$manifest.candidate.state -cne "retained-uncertified" -or
        -not [DateTime]::TryParseExact(
            $createdUtcValue,
            "o",
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::RoundtripKind,
            [ref]$createdUtcParsed) -or
        $createdUtcParsed.ToUniversalTime() -gt [DateTime]::UtcNow.AddMinutes(5) -or
        [string]$manifest.package.root -cne "package/Partisan" -or
        [string]$manifest.package.canonicalIndexPath -cne "evidence/pack/files.sha256" -or
        [string]$manifest.evidence.root -cne "evidence") {
        throw "The retained candidate metadata, root, or canonical-index identity is invalid."
    }
    $bundleLeaf = Split-Path -Leaf $bundleRoot
    $publishedLeaf = $bundleLeaf -ceq $candidateIdValue
    $partialPrefix = ".partial-" + $candidateIdValue + "-"
    $partialLeaf = $bundleLeaf.StartsWith(
        $partialPrefix,
        [StringComparison]::Ordinal) -and
        $bundleLeaf.Substring($partialPrefix.Length) -cmatch '^[0-9a-f]{32}$'
    if (-not $publishedLeaf -and -not $partialLeaf) {
        throw "The release bundle leaf is neither its exact candidate ID nor its exact partial identity."
    }
    if ($AllowUnsealedPublished -and -not $publishedLeaf) {
        throw "The unsealed-published check is valid only at the exact candidate leaf."
    }
    Assert-ObjectJsonEqual -Expected $manifest.source -Actual $source -Label "Source identity"
    Assert-ObjectJsonEqual -Expected $manifest.addon -Actual ([ordered]@{
        id = $sourceProject.id
        guid = $sourceProject.guid
        title = $sourceProject.title
        revision = "unpublished-local-pack"
        version = [string]$manifest.candidate.version
        dependencies = $sourceProject.dependencies
    }) -Label "Addon identity"
    Assert-ObjectJsonEqual -Expected $manifest.toolchain -Actual $toolchain -Label "Toolchain identity"

    $expectedPackageFiles = @($manifest.package.files)
    Assert-ObjectJsonEqual -Expected $expectedPackageFiles -Actual $package.Files -Label "Package file inventory"
    if ([string]$manifest.package.root -cne $package.Root -or
        [string]$manifest.package.sha256 -cne $package.Sha256) {
        throw "The aggregate package identity differs from the retained manifest."
    }
    $indexPath = Join-Path $bundleRoot (
        ([string]$manifest.package.canonicalIndexPath).Replace('/', '\'))
    if (-not (Test-ContainedPath `
            -Root $evidenceRootPath `
            -Candidate $indexPath) -or
        -not (Test-Path -LiteralPath $indexPath -PathType Leaf) -or
        (Get-Content -Raw -LiteralPath $indexPath).Replace("`r`n", "`n") -cne
            $package.CanonicalIndex) {
        throw "The retained canonical package index is missing or stale."
    }

    $resultPaths = @($manifest.workbench.targets | ForEach-Object {
        Join-Path $bundleRoot (([string]$_.evidencePath).Replace('/', '\'))
    })
    $workbench = Get-WorkbenchRecords `
        -BundleRoot $bundleRoot `
        -EvidenceRoot $evidenceRootPath `
        -Paths $resultPaths
    if ([string]$manifest.workbench.crc -cne $workbench.Crc) {
        throw "The retained Workbench CRC differs from its target records."
    }
    Assert-ObjectJsonEqual `
        -Expected @($manifest.workbench.targets) `
        -Actual $workbench.Targets `
        -Label "Workbench target records"

    $evidenceRows = Get-FileRows `
        -BundleRoot $bundleRoot `
        -Root $evidenceRootPath
    Assert-ObjectJsonEqual `
        -Expected @($manifest.evidence.files) `
        -Actual $evidenceRows `
        -Label "Evidence inventory"

    $readyPath = Join-Path $bundleRoot "candidate.ready.json"
    if ($publishedLeaf -and -not $AllowUnsealedPublished -and
        -not (Test-Path -LiteralPath $readyPath -PathType Leaf)) {
        throw "A published candidate is missing its ready seal."
    }
    if ($partialLeaf -and
        (Test-Path -LiteralPath $readyPath -PathType Leaf)) {
        throw "A partial release bundle must not contain a ready seal."
    }
    if (Test-Path -LiteralPath $readyPath -PathType Leaf) {
        $ready = Get-Content -Raw -LiteralPath $readyPath | ConvertFrom-Json
        $manifestSha = (Get-FileHash `
            -LiteralPath $manifestFullPath `
            -Algorithm SHA256).Hash.ToLowerInvariant()
        if ([int]$ready.schemaVersion -ne 1 -or
            [string]$ready.candidateId -cne $candidateIdValue -or
            [string]$ready.gitHead -cne [string]$manifest.source.gitHead -or
            [string]$ready.packageSha256 -cne [string]$manifest.package.sha256 -or
            [string]$ready.manifestSha256 -cne $manifestSha) {
            throw "The candidate ready seal does not match its retained manifest."
        }
    }
    Write-Output ("CHECK " + ([pscustomobject]@{
        Success = $true
        CandidateId = [string]$manifest.candidate.id
        GitHead = [string]$manifest.source.gitHead
        PackageSha256 = [string]$manifest.package.sha256
        WorkbenchCrc = [string]$manifest.workbench.crc
        PackageFiles = @($manifest.package.files).Count
        EvidenceFiles = @($manifest.evidence.files).Count
    } | ConvertTo-Json -Compress))
    return
}

if (Test-Path -LiteralPath $manifestFullPath) {
    throw "The release manifest path must be fresh."
}
if ($CandidateId -cnotmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$' -or
    $CandidateVersion -cnotmatch '^[0-9A-Za-z][0-9A-Za-z._-]{0,95}$' -or
    $ExpectedPackageSha256 -cnotmatch '^[0-9a-f]{64}$' -or
    $package.Sha256 -cne $ExpectedPackageSha256) {
    throw "The requested candidate identity or expected package SHA-256 is invalid."
}
$workbench = Get-WorkbenchRecords `
    -BundleRoot $bundleRoot `
    -EvidenceRoot $evidenceRootPath `
    -Paths $WorkbenchResultPath
$indexPath = Join-Path $evidenceRootPath "pack/files.sha256"
if (-not (Test-Path -LiteralPath $indexPath -PathType Leaf) -or
    (Get-Content -Raw -LiteralPath $indexPath).Replace("`r`n", "`n") -cne
        $package.CanonicalIndex) {
    throw "The pack evidence canonical index does not match the three-file package."
}
$evidenceRows = Get-FileRows `
    -BundleRoot $bundleRoot `
    -Root $evidenceRootPath
$manifestValue = [ordered]@{
    manifestSchemaVersion = $script:ManifestSchemaVersion
    createdUtc = [DateTime]::UtcNow.ToString(
        "o",
        [Globalization.CultureInfo]::InvariantCulture)
    candidate = [ordered]@{
        id = $CandidateId
        version = $CandidateVersion
        state = "retained-uncertified"
    }
    source = $source
    addon = [ordered]@{
        id = $sourceProject.id
        guid = $sourceProject.guid
        title = $sourceProject.title
        revision = "unpublished-local-pack"
        version = $CandidateVersion
        dependencies = $sourceProject.dependencies
    }
    toolchain = $toolchain
    workbench = [ordered]@{
        crc = $workbench.Crc
        targets = $workbench.Targets
    }
    package = [ordered]@{
        root = $package.Root
        hashAlgorithm = $package.HashAlgorithm
        sha256 = $package.Sha256
        canonicalIndexPath = Get-PortableRelativePath `
            -Root $bundleRoot `
            -Path $indexPath
        files = $package.Files
    }
    evidence = [ordered]@{
        root = Get-PortableRelativePath `
            -Root $bundleRoot `
            -Path $evidenceRootPath
        files = $evidenceRows
    }
}
Write-TextUtf8NoBom `
    -Path $manifestFullPath `
    -Text (ConvertTo-CanonicalJson -Value $manifestValue)
Write-Output ("MANIFEST " + ([pscustomobject]@{
    Success = $true
    CandidateId = $CandidateId
    GitHead = $source.gitHead
    PackageSha256 = $package.Sha256
    WorkbenchCrc = $workbench.Crc
    PackageFiles = $package.Files.Count
    EvidenceFiles = $evidenceRows.Count
} | ConvertTo-Json -Compress))
