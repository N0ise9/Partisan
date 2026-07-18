[CmdletBinding(DefaultParameterSetName = "Build")]
param(
    [Parameter(Mandatory = $true, ParameterSetName = "Build")]
    [string]$WorkbenchExecutable,

    [Parameter(Mandatory = $true, ParameterSetName = "Build")]
    [string]$RuntimeAddonRoot,

    [Parameter(Mandatory = $true, ParameterSetName = "Build")]
    [string]$ServerExecutable,

    [Parameter(Mandatory = $true, ParameterSetName = "Build")]
    [string]$ClientExecutable,

    [Parameter(Mandatory = $true, ParameterSetName = "Build")]
    [string]$OutputRoot,

    [Parameter(Mandatory = $true, ParameterSetName = "Build")]
    [string[]]$DefaultLogRoots,

    [Parameter(Mandatory = $true, ParameterSetName = "Build")]
    [string[]]$SpillRoots,

    [Parameter(ParameterSetName = "Build")]
    [ValidateSet("PC", "XBOX_ONE", "XBOX_SERIES", "PS4", "PS5")]
    [string[]]$Targets = @("PC", "XBOX_ONE", "XBOX_SERIES", "PS4", "PS5"),

    [Parameter(ParameterSetName = "Build")]
    [string]$CandidateVersion = "",

    [Parameter(ParameterSetName = "Build")]
    [ValidateRange(30, 900)]
    [int]$PackTimeoutSeconds = 300,

    [Parameter(ParameterSetName = "Build")]
    [ValidateRange(30, 3600)]
    [int]$WorkbenchTimeoutSeconds = 300,

    [Parameter(ParameterSetName = "Build")]
    [switch]$PreflightOnly,

    [Parameter(Mandatory = $true, ParameterSetName = "SelfTest")]
    [switch]$SelfTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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

function Write-PortableJson {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value
    )

    Write-TextUtf8NoBom `
        -Path $Path `
        -Text (($Value | ConvertTo-Json -Depth 20) + "`n")
}

function Get-Sha256Text {
    param([AllowEmptyString()][Parameter(Mandatory = $true)][string]$Text)

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [Text.Encoding]::UTF8.GetBytes($Text)
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($bytes))).Replace("-", "").ToLowerInvariant()
    }
    finally {
        $algorithm.Dispose()
    }
}

function Test-LexicallyContainedPath {
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

function Get-CanonicalPackageIndex {
    param([Parameter(Mandatory = $true)][object[]]$Files)

    $rows = New-Object Collections.Generic.List[string]
    foreach ($file in @($Files | Sort-Object `
        @{ Expression = { ([string]$_.Path).Replace('\', '/') }; Ascending = $true })) {
        $relativePath = ([string]$file.Path).Replace('\', '/')
        if ($relativePath -notmatch '^Partisan/[^/]+$' -or
            $relativePath.Contains("..") -or
            [string]$file.Sha256 -cnotmatch '^[0-9a-f]{64}$' -or
            [long]$file.Length -le 0) {
            throw "The canonical package index received an invalid file row."
        }
        [void]$rows.Add(("{0}`t{1}`t{2}" -f
            ([string]$file.Sha256).ToLowerInvariant(),
            [long]$file.Length,
            $relativePath))
    }
    return ($rows -join "`n") + "`n"
}

function Get-PackageIdentity {
    param([Parameter(Mandatory = $true)][string]$PackedAddonPath)

    $expectedNames = @("addon.gproj", "data.pak", "resourceDatabase.rdb")
    $files = @(Get-ChildItem `
        -LiteralPath $PackedAddonPath `
        -File `
        -Force `
        -ErrorAction Stop)
    $directories = @(Get-ChildItem `
        -LiteralPath $PackedAddonPath `
        -Directory `
        -Force `
        -ErrorAction Stop)
    $actualNames = @($files | ForEach-Object { $_.Name } | Sort-Object)
    $nameDifference = @(Compare-Object `
        -ReferenceObject @($expectedNames | Sort-Object) `
        -DifferenceObject $actualNames `
        -CaseSensitive)
    if ($directories.Count -ne 0 -or
        $files.Count -ne 3 -or
        $nameDifference.Count -ne 0) {
        throw "The retained package must contain exactly the three release files."
    }

    $fileRows = New-Object Collections.Generic.List[object]
    foreach ($file in @($files | Sort-Object Name)) {
        if ($file.Length -le 0) {
            throw "The retained package contains an empty release file."
        }
        [void]$fileRows.Add([pscustomobject]@{
            Path = "Partisan/" + $file.Name
            Length = [long]$file.Length
            Sha256 = (Get-FileHash `
                -LiteralPath $file.FullName `
                -Algorithm SHA256).Hash.ToLowerInvariant()
        })
    }
    $index = Get-CanonicalPackageIndex -Files $fileRows.ToArray()
    return [pscustomobject]@{
        Algorithm = "sha256-manifest-v1"
        Sha256 = Get-Sha256Text -Text $index
        CanonicalIndex = $index
        Files = $fileRows.ToArray()
    }
}

function Get-RequiredSourceMatch {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Group,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $match = [regex]::Match($Text, $Pattern)
    if (-not $match.Success -or
        [string]::IsNullOrWhiteSpace($match.Groups[$Group].Value)) {
        throw "Could not derive $Label."
    }
    return $match.Groups[$Group].Value
}

function Get-ProjectIdentity {
    param([Parameter(Mandatory = $true)][string]$ProjectFile)

    $text = Get-Content -Raw -LiteralPath $ProjectFile
    $dependenciesMatch = [regex]::Match(
        $text,
        'Dependencies\s*\{(?<body>[\s\S]*?)\}')
    if (-not $dependenciesMatch.Success) {
        throw "Could not derive the project dependency set."
    }
    $dependencies = @([regex]::Matches(
        $dependenciesMatch.Groups["body"].Value,
        '"(?<value>[0-9A-F]{16})"') | ForEach-Object {
            $_.Groups["value"].Value
        } | Sort-Object -Unique)
    if ($dependencies.Count -eq 0) {
        throw "The project dependency set is empty."
    }
    return [pscustomobject]@{
        Id = Get-RequiredSourceMatch $text '(?m)^\s*ID\s+"(?<value>[^"]+)"' "value" "project ID"
        Guid = Get-RequiredSourceMatch $text '(?m)^\s*GUID\s+"(?<value>[0-9A-F]{16})"' "value" "project GUID"
        Title = Get-RequiredSourceMatch $text '(?m)^\s*TITLE\s+"(?<value>[^"]+)"' "value" "project title"
        Dependencies = $dependencies
    }
}

function Assert-ProjectIdentityEqual {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual
    )

    if ([string]$Expected.Id -cne [string]$Actual.Id -or
        [string]$Expected.Guid -cne [string]$Actual.Guid -or
        [string]$Expected.Title -cne [string]$Actual.Title -or
        @(Compare-Object `
            -ReferenceObject @($Expected.Dependencies | Sort-Object) `
            -DifferenceObject @($Actual.Dependencies | Sort-Object) `
            -CaseSensitive).Count -ne 0) {
        throw "The packed project identity differs from the source project."
    }
}

function Get-RunnerRecord {
    param([Parameter(Mandatory = $true)][object[]]$Lines)

    $textLines = @($Lines | ForEach-Object { [string]$_ })
    $resultLine = @($textLines | Where-Object {
        $_.StartsWith("RESULT ", [StringComparison]::Ordinal)
    })
    $cleanupLine = @($textLines | Where-Object {
        $_.StartsWith("CLEANUP ", [StringComparison]::Ordinal)
    })
    if ($resultLine.Count -ne 1 -or $cleanupLine.Count -ne 1) {
        throw "The Workbench runner did not emit exactly one RESULT and CLEANUP record."
    }
    $result = $resultLine[0].Substring(7) | ConvertFrom-Json
    $cleanup = $cleanupLine[0].Substring(8) | ConvertFrom-Json
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
            throw "The Workbench runner RESULT record is missing $property."
        }
    }
    foreach ($property in $requiredCleanupProperties) {
        if ($cleanup.PSObject.Properties.Name -notcontains $property) {
            throw "The Workbench runner CLEANUP record is missing $property."
        }
    }
    if ($result.Success -isnot [bool] -or -not $result.Success -or
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
        throw "The Workbench runner result or cleanup record is not green."
    }
    return [pscustomobject]@{
        Result = $result
        Cleanup = $cleanup
    }
}

function Invoke-SelfTest {
    $first = @(
        [pscustomobject]@{ Path = "Partisan/resourceDatabase.rdb"; Length = 3; Sha256 = "c" * 64 },
        [pscustomobject]@{ Path = "Partisan/addon.gproj"; Length = 1; Sha256 = "a" * 64 },
        [pscustomobject]@{ Path = "Partisan/data.pak"; Length = 2; Sha256 = "b" * 64 })
    $second = @($first[1], $first[2], $first[0])
    $indexA = Get-CanonicalPackageIndex -Files $first
    $indexB = Get-CanonicalPackageIndex -Files $second
    $hashA = Get-Sha256Text -Text $indexA
    $hashB = Get-Sha256Text -Text $indexB
    if ($indexA -cne $indexB -or $hashA -cne $hashB -or
        $hashA -cnotmatch '^[0-9a-f]{64}$' -or
        -not $indexA.EndsWith("`n", [StringComparison]::Ordinal)) {
        throw "Release-candidate canonical package self-test failed."
    }
    Write-Output ("SELFTEST " + ([pscustomobject]@{
        Success = $true
        Algorithm = "sha256-manifest-v1"
        FileCount = 3
        OrderIndependent = $true
        Digest = $hashA
    } | ConvertTo-Json -Compress))
}

if ($SelfTest) {
    Invoke-SelfTest
    return
}

$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$projectFile = [IO.Path]::GetFullPath((Join-Path $repositoryRoot "addon.gproj"))
$workbenchPath = [IO.Path]::GetFullPath($WorkbenchExecutable)
$runtimeAddonPath = [IO.Path]::GetFullPath($RuntimeAddonRoot)
$serverPath = [IO.Path]::GetFullPath($ServerExecutable)
$clientPath = [IO.Path]::GetFullPath($ClientExecutable)
$artifactRoot = [IO.Path]::GetFullPath($OutputRoot)

if ((Test-LexicallyContainedPath `
        -Root $repositoryRoot `
        -Candidate $artifactRoot `
        -AllowEqual) -or
    (Test-LexicallyContainedPath `
        -Root $artifactRoot `
        -Candidate $repositoryRoot `
        -AllowEqual)) {
    throw "The retained release-candidate root must be outside the checkout."
}
$resolvedDefaultLogRoots = @($DefaultLogRoots | Where-Object {
    -not [string]::IsNullOrWhiteSpace([string]$_)
} | ForEach-Object { [IO.Path]::GetFullPath([string]$_) } | Sort-Object -Unique)
$resolvedSpillRoots = @($SpillRoots | Where-Object {
    -not [string]::IsNullOrWhiteSpace([string]$_)
} | ForEach-Object { [IO.Path]::GetFullPath([string]$_) } | Sort-Object -Unique)
if ($resolvedDefaultLogRoots.Count -eq 0 -or
    $resolvedSpillRoots.Count -eq 0) {
    throw "A release candidate requires nonempty default-log and spill monitoring roots."
}
foreach ($monitoringRoot in @(
    $resolvedDefaultLogRoots + $resolvedSpillRoots)) {
    if (-not (Test-Path -LiteralPath $monitoringRoot -PathType Container)) {
        throw "A release-candidate monitoring root does not exist."
    }
    if ((Test-LexicallyContainedPath `
            -Root $monitoringRoot `
            -Candidate $artifactRoot `
            -AllowEqual) -or
        (Test-LexicallyContainedPath `
            -Root $artifactRoot `
            -Candidate $monitoringRoot `
            -AllowEqual)) {
        throw "The retained artifact root must not overlap a monitored log or spill root."
    }
}
foreach ($protectedInputRoot in @(
    $runtimeAddonPath,
    (Split-Path -Parent $workbenchPath),
    (Split-Path -Parent $serverPath),
    (Split-Path -Parent $clientPath))) {
    if ((Test-LexicallyContainedPath `
            -Root $protectedInputRoot `
            -Candidate $artifactRoot `
            -AllowEqual) -or
        (Test-LexicallyContainedPath `
            -Root $artifactRoot `
            -Candidate $protectedInputRoot `
            -AllowEqual)) {
        throw "The retained artifact root must not overlap a runtime or tool input root."
    }
}

foreach ($requiredFile in @($projectFile, $workbenchPath, $serverPath, $clientPath)) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
        throw "A required release-candidate input file is missing."
    }
}
if ((Split-Path -Leaf $workbenchPath) -cnotin @(
    "ArmaReforgerWorkbenchDiag.exe",
    "ArmaReforgerWorkbenchSteamDiag.exe")) {
    throw "The release candidate requires a supported diagnostic Workbench executable."
}
if ((Split-Path -Leaf $serverPath) -cne "ArmaReforgerServer.exe" -or
    (Split-Path -Leaf $clientPath) -cne "ArmaReforgerSteam.exe") {
    throw "The release candidate requires the standard server and client executables."
}
foreach ($runtimeMarker in @("core/data.pak", "data/data.pak")) {
    if (-not (Test-Path `
        -LiteralPath (Join-Path $runtimeAddonPath $runtimeMarker) `
        -PathType Leaf)) {
        throw "The runtime add-on root is missing a required packed dependency."
    }
}
$requiredTargets = @("PC", "XBOX_ONE", "XBOX_SERIES", "PS4", "PS5")
$releaseTargets = @($Targets | Sort-Object -Unique)
if ($releaseTargets.Count -ne $requiredTargets.Count -or
    @(Compare-Object `
        -ReferenceObject @($requiredTargets | Sort-Object) `
        -DifferenceObject @($releaseTargets | Sort-Object) `
        -CaseSensitive).Count -ne 0) {
    throw "A release candidate requires exactly PC, XBOX_ONE, XBOX_SERIES, PS4, and PS5 Workbench targets."
}

$gitHead = (& git -C $repositoryRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $gitHead -cnotmatch '^[0-9a-f]{40}$') {
    throw "Could not resolve the release-candidate Git HEAD."
}
$worktreeRows = @(& git -C $repositoryRoot status --porcelain --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $worktreeRows.Count -ne 0) {
    throw "A release candidate can only be built from a clean checkout."
}
$diffCheck = @(& git -C $repositoryRoot diff --check 2>&1)
if ($LASTEXITCODE -ne 0) {
    throw "Git diff hygiene failed before the release build: $($diffCheck -join ' | ')"
}

$sourceProjectIdentity = Get-ProjectIdentity -ProjectFile $projectFile
$utcToken = [DateTime]::UtcNow.ToString(
    "yyyyMMddTHHmmssZ",
    [Globalization.CultureInfo]::InvariantCulture)
if ([string]::IsNullOrWhiteSpace($CandidateVersion)) {
    $CandidateVersion = "0.1.0-rc.$utcToken.$($gitHead.Substring(0, 8))"
}
if ($CandidateVersion -cnotmatch '^[0-9A-Za-z][0-9A-Za-z._-]{0,95}$') {
    throw "CandidateVersion contains unsupported characters or is too long."
}
$candidateId = "partisan-rc-$($gitHead.Substring(0, 12))-$utcToken"
$nonce = [Guid]::NewGuid().ToString("N")
$partialRoot = Join-Path $artifactRoot ".partial-$candidateId-$nonce"
$finalRoot = Join-Path $artifactRoot $candidateId

$preflight = [pscustomobject]@{
    CandidateId = $candidateId
    CandidateVersion = $CandidateVersion
    GitHead = $gitHead
    ProjectId = $sourceProjectIdentity.Id
    AddonGuid = $sourceProjectIdentity.Guid
    TargetCount = $releaseTargets.Count
    RuntimeMarkersPresent = $true
    WorktreeClean = $true
}
if ($PreflightOnly) {
    Write-Output ("PREFLIGHT " + ($preflight | ConvertTo-Json -Compress))
    return
}

if (-not (Test-Path -LiteralPath $artifactRoot -PathType Container)) {
    New-Item -ItemType Directory -Path $artifactRoot -Force | Out-Null
}
if ((Test-Path -LiteralPath $partialRoot) -or
    (Test-Path -LiteralPath $finalRoot)) {
    throw "The release-candidate output identity is not fresh."
}

# Import the repository's single guarded Windows process/packing implementation.
$ordinaryLibrary = Join-Path $PSScriptRoot `
    "run-ordinary-campaign-persistence-proof.ps1"
. $ordinaryLibrary `
    -Executable $serverPath `
    -RuntimeAddonRoot $runtimeAddonPath `
    -WorkbenchExecutable $workbenchPath `
    -ProjectPath $projectFile `
    -SpillRoots $resolvedSpillRoots `
    -PackTimeoutSeconds $PackTimeoutSeconds `
    -ClientExecutable $clientPath `
    -LibraryOnly

Assert-NoReparsePathAncestry -Path $artifactRoot

New-Item -ItemType Directory -Path $partialRoot | Out-Null
Assert-NoReparsePathAncestry -Path $partialRoot
$evidenceRoot = Join-Path $partialRoot "evidence"
$packageParent = Join-Path $partialRoot "package"
$packedAddonPath = Join-Path $packageParent "Partisan"
$packEvidenceRoot = Join-Path $evidenceRoot "pack"
$packProfileRoot = Join-Path $packEvidenceRoot "profile"
$packWorkingRoot = Join-Path $packEvidenceRoot "working"
$packTempRoot = Join-Path $packEvidenceRoot "temp"
foreach ($directory in @(
    $evidenceRoot,
    $packageParent,
    $packEvidenceRoot,
    $packProfileRoot,
    $packWorkingRoot,
    $packTempRoot)) {
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
}

$workspaceScratchPath = Join-Path `
    $repositoryRoot `
    $script:WorkspacePackScratchLeaf
$wrapperStartUtc = (Get-Process -Id $PID -ErrorAction Stop).StartTime.ToUniversalTime()
$workspaceScratchCreated = $false
$workspaceScratchOwnership = $null
$buildError = $null
$cleanupError = $null
$packageIdentity = $null
$candidateMoved = $false
$candidatePublished = $false
$candidateQuarantined = $false
$workbenchRecords = New-Object Collections.Generic.List[object]

try {
    $foundationDirectory = Join-Path $evidenceRoot "foundation"
    New-Item -ItemType Directory -Path $foundationDirectory | Out-Null
    $foundationTranscriptPath = Join-Path $foundationDirectory "transcript.txt"
    $foundationError = $null
    $foundationTranscriptError = $null
    $foundationTranscriptStarted = $false
    try {
        Start-Transcript -LiteralPath $foundationTranscriptPath -Force | Out-Null
        $foundationTranscriptStarted = $true
        & (Join-Path $PSScriptRoot "validate-foundation.ps1") | Out-Host
    }
    catch {
        $foundationError = $_.Exception.Message
        if ($foundationTranscriptStarted) {
            Write-Host "ERROR $foundationError"
        }
    }
    finally {
        if ($foundationTranscriptStarted) {
            try {
                Stop-Transcript | Out-Null
            }
            catch {
                $foundationTranscriptError = $_.Exception.Message
            }
        }
    }
    if ($foundationTranscriptError) {
        throw "Foundation transcript finalization failed: $foundationTranscriptError"
    }
    if ($foundationError) {
        throw "Foundation failed: $foundationError"
    }

    $workbenchRunner = Join-Path `
        $PSScriptRoot `
        "run-guarded-workbench-validation.ps1"
    foreach ($target in $releaseTargets) {
        $targetRoot = Join-Path (Join-Path $evidenceRoot "workbench") $target
        $rawRoot = Join-Path $targetRoot "raw"
        New-Item -ItemType Directory -Path $rawRoot -Force | Out-Null
        $runnerLines = New-Object Collections.Generic.List[string]
        $runnerError = $null
        try {
            & $workbenchRunner `
                -Executable $workbenchPath `
                -ProjectPath $projectFile `
                -AddonRoots @($runtimeAddonPath) `
                -Target $target `
                -EvidenceDirectory $rawRoot `
                -DefaultLogRoots $resolvedDefaultLogRoots `
                -SpillRoots $resolvedSpillRoots `
                -TimeoutSeconds $WorkbenchTimeoutSeconds `
                -ReturnToCaller 2>&1 |
                ForEach-Object { [void]$runnerLines.Add([string]$_) }
        }
        catch {
            $runnerError = $_.Exception.Message
            [void]$runnerLines.Add("ERROR $runnerError")
        }
        Write-TextUtf8NoBom `
            -Path (Join-Path $targetRoot "transcript.txt") `
            -Text (($runnerLines.ToArray() -join "`n") + "`n")
        if ($runnerError) {
            throw "Workbench target $target failed: $runnerError"
        }
        $record = Get-RunnerRecord -Lines $runnerLines.ToArray()
        if ([string]$record.Result.Target -cne $target) {
            throw "The Workbench runner returned the wrong target identity."
        }
        Write-PortableJson `
            -Path (Join-Path $targetRoot "result.json") `
            -Value $record
        [void]$workbenchRecords.Add($record)
    }

    $postWorkbenchStatus = @(& git -C $repositoryRoot status --porcelain --untracked-files=all)
    if ($LASTEXITCODE -ne 0 -or $postWorkbenchStatus.Count -ne 0 -or
        (& git -C $repositoryRoot rev-parse HEAD).Trim() -cne $gitHead) {
        throw "The source checkout changed during Foundation or Workbench validation."
    }

    if (Test-Path -LiteralPath $workspaceScratchPath) {
        throw "Workbench packing requires a fresh checkout-local scratch boundary."
    }
    $packDefaultSnapshots = New-Object Collections.Generic.List[object]
    foreach ($defaultLogRoot in $resolvedDefaultLogRoots) {
        [void]$packDefaultSnapshots.Add((New-RootSnapshot `
            -Root $defaultLogRoot))
    }
    $packSpillExclusions = New-Object Collections.Generic.List[string]
    [void]$packSpillExclusions.Add($repositoryRoot)
    [void]$packSpillExclusions.Add($artifactRoot)
    foreach ($defaultSnapshot in $packDefaultSnapshots) {
        [void]$packSpillExclusions.Add([string]$defaultSnapshot.Root)
    }
    $packSpillSnapshots = New-Object Collections.Generic.List[object]
    foreach ($spillRoot in $resolvedSpillRoots) {
        [void]$packSpillSnapshots.Add((New-RootSnapshot `
            -Root $spillRoot `
            -ExcludedRoots $packSpillExclusions.ToArray()))
    }
    New-Item -ItemType Directory -Path $workspaceScratchPath | Out-Null
    $workspaceScratchCreated = $true
    Assert-NoReparsePathAncestry -Path $workspaceScratchPath
    Write-JsonUtf8NoBom `
        -Path (Join-Path `
            $workspaceScratchPath `
            $script:WorkspacePackScratchSentinelLeaf) `
        -Value ([ordered]@{
            version = $script:SentinelVersion
            purpose = "native_workbench_pack_scratch"
            nonce = $nonce
            ownerPid = $PID
            ownerStartUtc = $wrapperStartUtc.ToString(
                "o",
                [Globalization.CultureInfo]::InvariantCulture)
        })
    $workspaceScratchOwnership = Read-WorkspacePackScratchOwnership `
        -Directory $workspaceScratchPath `
        -RepositoryRoot $repositoryRoot
    if (-not $workspaceScratchOwnership -or
        $workspaceScratchOwnership.Nonce -cne $nonce -or
        $workspaceScratchOwnership.OwnerPid -ne $PID -or
        $workspaceScratchOwnership.OwnerStartUtc.Ticks -ne $wrapperStartUtc.Ticks) {
        throw "Checkout-local pack scratch ownership could not be established."
    }

    $packArguments = Get-PackArgumentVector `
        -ProjectFile $projectFile `
        -RuntimeAddonPath $runtimeAddonPath `
        -PackProfilePath $packProfileRoot `
        -PackedAddonPath $packedAddonPath
    $unclaimedEngineProcesses =
        New-Object Collections.Generic.HashSet[string]
    $packOutcome = $null
    $packRunError = $null
    $packBoundaryAuditError = $null
    $packBoundary = [ordered]@{
        newDefaultEntries = 0
        modifiedDefaultFiles = 0
        deletedDefaultEntries = 0
        missingDefaultRoots = 0
        newSpillEntries = 0
        modifiedSpillFiles = 0
        deletedSpillEntries = 0
        missingSpillRoots = 0
    }
    try {
        $packOutcome = Invoke-GuardedProcess `
            -Label "retained release-candidate Workbench pack" `
            -ExecutablePath $workbenchPath `
            -Arguments $packArguments `
            -WorkingDirectory $packWorkingRoot `
            -TempDirectory $packTempRoot `
            -TimeoutSeconds $PackTimeoutSeconds `
            -DiagnosticProjectDirectory $repositoryRoot `
            -DiagnosticAddonRoots @($runtimeAddonPath) `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcesses
    }
    catch {
        $packRunError = $_.Exception.Message
    }
    finally {
        try {
            foreach ($snapshot in $packDefaultSnapshots) {
                $delta = Get-RootSnapshotDelta -Snapshot $snapshot
                $packBoundary.newDefaultEntries += $delta.NewEntries
                $packBoundary.modifiedDefaultFiles += $delta.ModifiedFiles
                $packBoundary.deletedDefaultEntries += $delta.DeletedEntries
                $packBoundary.missingDefaultRoots += $delta.MissingRoot
            }
            foreach ($snapshot in $packSpillSnapshots) {
                $delta = Get-RootSnapshotDelta -Snapshot $snapshot
                $packBoundary.newSpillEntries += $delta.NewEntries
                $packBoundary.modifiedSpillFiles += $delta.ModifiedFiles
                $packBoundary.deletedSpillEntries += $delta.DeletedEntries
                $packBoundary.missingSpillRoots += $delta.MissingRoot
            }
            Write-PortableJson `
                -Path (Join-Path $packEvidenceRoot "boundary-audit.json") `
                -Value $packBoundary
        }
        catch {
            $packBoundaryAuditError = $_.Exception.Message
        }
    }
    if ($packBoundaryAuditError) {
        $combinedPackError = if ($packRunError) {
            "$packRunError | boundary audit: $packBoundaryAuditError"
        }
        else {
            $packBoundaryAuditError
        }
        throw "Workbench pack boundary auditing failed: $combinedPackError"
    }
    $packBoundaryMutationCount = 0
    foreach ($value in $packBoundary.Values) {
        $packBoundaryMutationCount += [int]$value
    }
    if ($packBoundaryMutationCount -ne 0) {
        throw "Workbench packing changed a monitored default-log or spill boundary."
    }
    if ($packRunError) {
        throw $packRunError
    }
    if (-not $packOutcome -or $unclaimedEngineProcesses.Count -ne 0) {
        throw "The guarded Workbench pack did not return a clean exact outcome."
    }

    $layout = Assert-PackedAddonLayout `
        -GuardRoot $partialRoot `
        -PackedAddonsParent $packageParent
    if ($layout.FileCount -ne 3) {
        throw "Workbench packing emitted an unexpected release file count."
    }
    $packedProjectIdentity = Get-ProjectIdentity `
        -ProjectFile (Join-Path $packedAddonPath "addon.gproj")
    Assert-ProjectIdentityEqual `
        -Expected $sourceProjectIdentity `
        -Actual $packedProjectIdentity
    $packageIdentity = Get-PackageIdentity -PackedAddonPath $packedAddonPath
    Write-TextUtf8NoBom `
        -Path (Join-Path $packEvidenceRoot "files.sha256") `
        -Text $packageIdentity.CanonicalIndex
    Write-PortableJson `
        -Path (Join-Path $packEvidenceRoot "pack-summary.json") `
        -Value ([ordered]@{
            exitCode = $packOutcome.ExitCode
            elapsedSeconds = $packOutcome.ElapsedSeconds
            engineAfter = $packOutcome.EngineAfter
            ownedProcessesRemaining = $packOutcome.OwnedProcessesRemaining
            fileCount = 3
            packageHashAlgorithm = $packageIdentity.Algorithm
            packageSha256 = $packageIdentity.Sha256
        })

    if (@(Get-EngineProcessRows).Count -ne 0) {
        throw "An engine process remained before pack-scratch cleanup."
    }
    $currentScratchOwnership = Read-WorkspacePackScratchOwnership `
        -Directory $workspaceScratchPath `
        -RepositoryRoot $repositoryRoot
    if (-not $currentScratchOwnership -or
        $currentScratchOwnership.Nonce -cne $nonce -or
        $currentScratchOwnership.OwnerPid -ne $PID -or
        $currentScratchOwnership.OwnerStartUtc.Ticks -ne $wrapperStartUtc.Ticks -or
        -not (Remove-WorkspacePackScratch `
            -Ownership $currentScratchOwnership `
            -RepositoryRoot $repositoryRoot)) {
        throw "Owned checkout-local pack scratch cleanup failed."
    }
    $workspaceScratchCreated = $false
    $workspaceScratchOwnership = $null

    $postPackStatus = @(& git -C $repositoryRoot status --porcelain --untracked-files=all)
    if ($LASTEXITCODE -ne 0 -or $postPackStatus.Count -ne 0 -or
        (& git -C $repositoryRoot rev-parse HEAD).Trim() -cne $gitHead) {
        throw "The source checkout changed while packing the release candidate."
    }

    $manifestScript = Join-Path $PSScriptRoot "new-release-manifest.ps1"
    if (-not (Test-Path -LiteralPath $manifestScript -PathType Leaf)) {
        throw "The release-manifest generator is missing."
    }
    # The manifest script owns source/build/schema/tool identity extraction and
    # hashes every retained package/evidence file using relative paths only.
    & $manifestScript `
        -CandidateRoot $packageParent `
        -EvidenceRoot $evidenceRoot `
        -ManifestPath (Join-Path $partialRoot "candidate.json") `
        -WorkbenchExecutable $workbenchPath `
        -ServerExecutable $serverPath `
        -ClientExecutable $clientPath `
        -CandidateId $candidateId `
        -CandidateVersion $CandidateVersion `
        -ExpectedPackageSha256 $packageIdentity.Sha256 `
        -WorkbenchResultPath @($workbenchRecords | ForEach-Object {
            Join-Path `
                (Join-Path (Join-Path $evidenceRoot "workbench") ([string]$_.Result.Target)) `
                "result.json"
        })

    $manifestPath = Join-Path $partialRoot "candidate.json"
    & $manifestScript `
        -CandidateRoot $packageParent `
        -EvidenceRoot $evidenceRoot `
        -ManifestPath $manifestPath `
        -WorkbenchExecutable $workbenchPath `
        -ServerExecutable $serverPath `
        -ClientExecutable $clientPath `
        -Check

    if (Test-Path -LiteralPath $finalRoot) {
        throw "The final release-candidate identity appeared before publication."
    }
    [IO.Directory]::Move($partialRoot, $finalRoot)
    $candidateMoved = $true
    $finalManifestPath = Join-Path $finalRoot "candidate.json"
    & $manifestScript `
        -CandidateRoot (Join-Path $finalRoot "package") `
        -EvidenceRoot (Join-Path $finalRoot "evidence") `
        -ManifestPath $finalManifestPath `
        -WorkbenchExecutable $workbenchPath `
        -ServerExecutable $serverPath `
        -ClientExecutable $clientPath `
        -Check `
        -AllowUnsealedPublished

    $readyPath = Join-Path $finalRoot "candidate.ready.json"
    $readyPartialPath = Join-Path `
        $finalRoot `
        (".candidate-ready-partial-" + $nonce + ".json")
    if ((Test-Path -LiteralPath $readyPath) -or
        (Test-Path -LiteralPath $readyPartialPath)) {
        throw "The candidate ready-seal path was not fresh."
    }
    $manifestSha256 = (Get-FileHash `
        -LiteralPath $finalManifestPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    $readyValue = [ordered]@{
        schemaVersion = 1
        candidateId = $candidateId
        gitHead = $gitHead
        packageSha256 = $packageIdentity.Sha256
        manifestSha256 = $manifestSha256
    }
    Write-PortableJson `
        -Path $readyPartialPath `
        -Value $readyValue
    $readyReadBack = Get-Content `
        -Raw `
        -LiteralPath $readyPartialPath | ConvertFrom-Json
    if ([int]$readyReadBack.schemaVersion -ne 1 -or
        [string]$readyReadBack.candidateId -cne $candidateId -or
        [string]$readyReadBack.gitHead -cne $gitHead -or
        [string]$readyReadBack.packageSha256 -cne $packageIdentity.Sha256 -or
        [string]$readyReadBack.manifestSha256 -cne $manifestSha256) {
        throw "The candidate ready seal failed exact readback."
    }
    $readySha256 = (Get-FileHash `
        -LiteralPath $readyPartialPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    [IO.File]::Move($readyPartialPath, $readyPath)
    & $manifestScript `
        -CandidateRoot (Join-Path $finalRoot "package") `
        -EvidenceRoot (Join-Path $finalRoot "evidence") `
        -ManifestPath $finalManifestPath `
        -WorkbenchExecutable $workbenchPath `
        -ServerExecutable $serverPath `
        -ClientExecutable $clientPath `
        -Check
    $candidatePublished = $true

    Write-Output ("CANDIDATE " + ([pscustomobject]@{
        CandidateId = $candidateId
        CandidateVersion = $CandidateVersion
        GitHead = $gitHead
        AddonGuid = $sourceProjectIdentity.Guid
        PackageSha256 = $packageIdentity.Sha256
        ManifestSha256 = $manifestSha256
        ReadySha256 = $readySha256
        PackageHashAlgorithm = $packageIdentity.Algorithm
        TargetCount = $workbenchRecords.Count
        Manifest = "candidate.json"
        Ready = "candidate.ready.json"
    } | ConvertTo-Json -Compress))
}
catch {
    $buildError = $_.Exception.Message
    if ($candidateMoved -and -not $candidatePublished -and
        (Test-Path -LiteralPath $finalRoot -PathType Container)) {
        try {
            $failedRoot = Join-Path `
                $artifactRoot `
                (".failed-" + $candidateId + "-" + $nonce)
            if (Test-Path -LiteralPath $failedRoot) {
                throw "The failed-candidate quarantine path was not fresh."
            }
            [IO.Directory]::Move($finalRoot, $failedRoot)
            $candidateMoved = $false
            $candidateQuarantined = $true
        }
        catch {
            $buildError += " | failed-candidate quarantine: $($_.Exception.Message)"
        }
    }
}
finally {
    if ($workspaceScratchCreated) {
        try {
            if (@(Get-EngineProcessRows).Count -ne 0) {
                throw "An engine process remained before pack-scratch cleanup."
            }
            $currentOwnership = Read-WorkspacePackScratchOwnership `
                -Directory $workspaceScratchPath `
                -RepositoryRoot $repositoryRoot
            if (-not $currentOwnership -or
                $currentOwnership.Nonce -cne $nonce -or
                $currentOwnership.OwnerPid -ne $PID -or
                $currentOwnership.OwnerStartUtc.Ticks -ne $wrapperStartUtc.Ticks -or
                -not (Remove-WorkspacePackScratch `
                    -Ownership $currentOwnership `
                    -RepositoryRoot $repositoryRoot)) {
                throw "Owned checkout-local pack scratch cleanup failed."
            }
        }
        catch {
            $cleanupError = $_.Exception.Message
        }
    }
}

if ($cleanupError) {
    throw "Release-candidate cleanup failed: $cleanupError"
}
if ($buildError) {
    $retainedState = if ($candidateQuarantined) {
        "a quarantined failed directory"
    }
    elseif ($candidateMoved -and -not $candidatePublished) {
        "an unsealed final directory"
    }
    else {
        "partial evidence"
    }
    throw "Release-candidate build failed; $retainedState was retained: $buildError"
}
