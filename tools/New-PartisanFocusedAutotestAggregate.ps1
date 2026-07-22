[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$EvidenceRoot,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string[]]$RunJson,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$OutputPath,

    [AllowEmptyCollection()]
    [string[]]$HistoricalAggregate = @(),

    [ValidateNotNullOrEmpty()]
    [string]$RepositoryRoot = (Join-Path $PSScriptRoot '..')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:FocusedAggregateSchema = 2
$script:FocusedRunSchema = 1
$script:CandidateManifestSchema = 1
$script:CandidateReadySchema = 1
$script:FocusedRunEvidenceKind = 'packaged-focused-autotest'
$script:FocusedAggregateEvidenceKind = 'packaged-focused-autotest-set'
$script:FocusedAggregateRejectionKind =
    'packaged-focused-autotest-set-rejection'
$script:FocusedAggregateContractId =
    'partisan.focused-autotest.aggregate.v2'
$script:FocusedRunDisposition = 'active-runtime-candidate'
$script:FocusedCandidateState = 'retained-uncertified'
$script:FocusedAcceptedDisposition = 'accepted-noncertifying'
$script:FocusedReplacementDisposition = 'replacement-required'
$script:FocusedAggregateResultStatus = 'passed-noncertifying'
$script:FocusedHardDiagnosticClassifierChecks = 12
$script:FocusedDiagnosticLocalPathToken = '<local-path>'
$script:FocusedCampaignSchema = 71
$script:FocusedRuntimeSettingsSchema = 24
$script:FocusedRequiredPatternMarker =
    'PARTISAN_REQUIRED_LOG_PATTERN_B64 '
$script:FocusedMountProjectSuffix =
    'candidate-addons/Partisan/addon.gproj'
$script:FocusedSemanticBeforeBarrierUsed = $false
$script:FocusedSemanticAfterBarrierUsed = $false
$script:FocusedOutputParentBarrierUsed = $false
$script:FocusedReleaseStatusPath = 'docs/data/release_status.json'
$script:FocusedTrackedEvidencePrefix = 'docs/evidence/focused-autotest'
$script:FocusedLegacyProfileOrder = @(
    'HST_TEST_EnemyCounterattackAuthority',
    'HST_TEST_EnemyGarrisonRebuildAuthority',
    'HST_TEST_EnemyPlanningCommitmentAuthority',
    'HST_TEST_EnemyQRFAuthority',
    'HST_TEST_CampaignProfileJournalAuthority'
)
$script:FocusedLegacySuiteByProfile = [ordered]@{
    HST_TEST_EnemyCounterattackAuthority =
        'HST_EnemyCounterattackAutotestSuite'
    HST_TEST_EnemyGarrisonRebuildAuthority =
        'HST_EnemyGarrisonRebuildAutotestSuite'
    HST_TEST_EnemyPlanningCommitmentAuthority =
        'HST_EnemyPlanningCommitmentAutotestSuite'
    HST_TEST_EnemyQRFAuthority = 'HST_EnemyQRFAutotestSuite'
    HST_TEST_CampaignProfileJournalAuthority =
        'HST_CampaignProfileJournalAuthorityAutotestSuite'
}
$script:FocusedProfileOrder = @(
    'HST_EnemyCounterattackAutotestSuite',
    'HST_EnemyGarrisonRebuildAutotestSuite',
    'HST_EnemyPlanningCommitmentAutotestSuite',
    'HST_EnemyQRFAutotestSuite',
    'HST_CampaignProfileJournalAuthorityAutotestSuite'
)
$script:FocusedSuiteByProfile = [ordered]@{
    HST_EnemyCounterattackAutotestSuite =
        'HST_EnemyCounterattackAutotestSuite'
    HST_EnemyGarrisonRebuildAutotestSuite =
        'HST_EnemyGarrisonRebuildAutotestSuite'
    HST_EnemyPlanningCommitmentAutotestSuite =
        'HST_EnemyPlanningCommitmentAutotestSuite'
    HST_EnemyQRFAutotestSuite = 'HST_EnemyQRFAutotestSuite'
    HST_CampaignProfileJournalAuthorityAutotestSuite =
        'HST_CampaignProfileJournalAuthorityAutotestSuite'
}
$script:FocusedSuiteTestCases = [ordered]@{
    HST_EnemyCounterattackAutotestSuite = @(
        'HST_TEST_EnemyCounterattack_FrozenPlanning',
        'HST_TEST_EnemyCounterattack_Admission',
        'HST_TEST_EnemyCounterattack_VirtualTravel',
        'HST_TEST_EnemyCounterattack_VirtualCombat',
        'HST_TEST_EnemyCounterattack_PhysicalHandoff',
        'HST_TEST_EnemyCounterattack_OwnershipRetry',
        'HST_TEST_EnemyCounterattack_SettlementReplay',
        'HST_TEST_EnemyCounterattack_SupportSettlement',
        'HST_TEST_EnemyCounterattack_RestoreLifecycle',
        'HST_TEST_EnemyCounterattack_ResourceAuthorityQuarantine',
        'HST_TEST_EnemyCounterattack_AmbiguityHold',
        'HST_TEST_EnemyCounterattack_OwnershipCorrelationQuarantine',
        'HST_TEST_EnemyCounterattack_Schema69Quarantine',
        'HST_TEST_EnemyCounterattack_QuarantineRetention'
    )
    HST_EnemyGarrisonRebuildAutotestSuite = @(
        'HST_TEST_EnemyGarrisonRebuild_AdmissionCapacity',
        'HST_TEST_EnemyGarrisonRebuild_DeliveryHeld',
        'HST_TEST_EnemyGarrisonRebuild_CasualtyContinuity',
        'HST_TEST_EnemyGarrisonRebuild_Restore',
        'HST_TEST_EnemyGarrisonRebuild_OwnershipTerminal',
        'HST_TEST_EnemyGarrisonRebuild_AdmissionRollback',
        'HST_TEST_EnemyGarrisonRebuild_PrearrivalRefund',
        'HST_TEST_EnemyGarrisonRebuild_SettlementCrashResume',
        'HST_TEST_EnemyGarrisonRebuild_HistoricalIsolation',
        'HST_TEST_EnemyGarrisonRebuild_Schema70Quarantine',
        'HST_TEST_EnemyGarrisonRebuild_OrphanRuntimeQuarantine',
        'HST_TEST_EnemyGarrisonRebuild_QuarantineRetention',
        'HST_TEST_EnemyGarrisonRebuild_SelectedOwnershipABA'
    )
    HST_EnemyPlanningCommitmentAutotestSuite = @(
        'HST_TEST_EnemyPlanning_Pre68Baseline',
        'HST_TEST_EnemyPlanning_IndependentCadence',
        'HST_TEST_EnemyPlanning_BeginReplayConflict',
        'HST_TEST_EnemyPlanning_CommitmentPermutation',
        'HST_TEST_EnemyPlanning_CommitmentAwareSelection',
        'HST_TEST_EnemyPlanning_AllCommittedSkip',
        'HST_TEST_EnemyPlanning_CommitmentRaceRejection',
        'HST_TEST_EnemyPlanning_FrozenDecision',
        'HST_TEST_EnemyPlanning_RetryEnvelope',
        'HST_TEST_EnemyPlanning_PreparedPressureCrashWindow',
        'HST_TEST_EnemyPlanning_PreparedOrderAdoption',
        'HST_TEST_EnemyPlanning_RetryTamperQuarantine',
        'HST_TEST_EnemyPlanning_ZeroTargetSkip',
        'HST_TEST_EnemyPlanning_CommittedRoundtrip',
        'HST_TEST_EnemyPlanning_CurrentQuarantine',
        'HST_TEST_EnemyPlanning_FreshBootstrap',
        'HST_TEST_EnemyPlanning_UnavailableLogThrottle'
    )
    HST_EnemyQRFAutotestSuite = @(
        'HST_TEST_EnemyQRF_Admission',
        'HST_TEST_EnemyQRF_LegacyIsolation',
        'HST_TEST_EnemyQRF_Projection',
        'HST_TEST_EnemyQRF_Settlement',
        'HST_TEST_EnemyQRF_Restore',
        'HST_TEST_EnemyQRF_Rejection'
    )
    HST_CampaignProfileJournalAuthorityAutotestSuite = @(
        'HST_TEST_CampaignProfileJournalAuthority_GenerationAdvance',
        'HST_TEST_CampaignProfileJournalAuthority_CanonicalGenerationOnePreserved',
        'HST_TEST_CampaignProfileJournalAuthority_TruncatedNewestFallback',
        'HST_TEST_CampaignProfileJournalAuthority_BadFingerprintFallback',
        'HST_TEST_CampaignProfileJournalAuthority_BothInvalidRejected',
        'HST_TEST_CampaignProfileJournalAuthority_BothInvalidSourceFatal',
        'HST_TEST_CampaignProfileJournalAuthority_FutureEnvelopeRejected',
        'HST_TEST_CampaignProfileJournalAuthority_UnknownMagicRejected',
        'HST_TEST_CampaignProfileJournalAuthority_FutureSchemaRejected',
        'HST_TEST_CampaignProfileJournalAuthority_FutureRawRejected',
        'HST_TEST_CampaignProfileJournalAuthority_FutureArtifactWriteNonMutating',
        'HST_TEST_CampaignProfileJournalAuthority_LegacyRawUpgrade',
        'HST_TEST_CampaignProfileJournalAuthority_SplitBrainRejected',
        'HST_TEST_CampaignProfileJournalAuthority_BrokenChainRejected',
        'HST_TEST_CampaignProfileJournalAuthority_GenerationOneParentGenerationRejected',
        'HST_TEST_CampaignProfileJournalAuthority_AdjacentWrongParentRejected',
        'HST_TEST_CampaignProfileJournalAuthority_NonAdjacentParentFingerprintRejected',
        'HST_TEST_CampaignProfileJournalAuthority_DuplicateMetadataRejected',
        'HST_TEST_CampaignProfileJournalAuthority_FutureWriteNonMutating',
        'HST_TEST_CampaignProfileJournalAuthority_SelectedReadOnly',
        'HST_TEST_CampaignProfileJournalAuthority_DegradedNativeRecovery',
        'HST_TEST_CampaignProfileJournalAuthority_FallbackOnlyCheckpoint',
        'HST_TEST_CampaignProfileJournalAuthority_FailedNativeCallbackNonMutating',
        'HST_TEST_CampaignProfileJournalAuthority_ValidNativeInvalidJournal',
        'HST_TEST_CampaignProfileJournalAuthority_ValidNativeFutureJournal',
        'HST_TEST_CampaignProfileJournalAuthority_FutureNativeAuthorityRejected',
        'HST_TEST_CampaignProfileJournalAuthority_LegacyNativeFingerprintAccepted',
        'HST_TEST_CampaignProfileJournalAuthority_NativeV1LoadClassification',
        'HST_TEST_CampaignProfileJournalAuthority_NativeV2LoadClassification',
        'HST_TEST_CampaignProfileJournalAuthority_NativeInvalidFingerprintClassification',
        'HST_TEST_CampaignProfileJournalAuthority_NativeFutureEnvelopeClassification',
        'HST_TEST_CampaignProfileJournalAuthority_NewerJournalAuthority',
        'HST_TEST_CampaignProfileJournalAuthority_NewerNativeAuthority',
        'HST_TEST_CampaignProfileJournalAuthority_EqualOrderConflictRejected',
        'HST_TEST_CampaignProfileJournalAuthority_LastSaveSecondNewerJournal',
        'HST_TEST_CampaignProfileJournalAuthority_LastSaveSecondNewerNative',
        'HST_TEST_CampaignProfileJournalAuthority_EqualOrderSameFingerprintNative',
        'HST_TEST_CampaignProfileJournalAuthority_LegacyRawEqualOrderNative',
        'HST_TEST_CampaignProfileJournalAuthority_CheckpointSequenceOrdering',
        'HST_TEST_CampaignProfileJournalAuthority_AuthorityJournalMetadata',
        'HST_TEST_CampaignProfileJournalAuthority_Cleanup'
    )
}
$script:FocusedExpectedJUnitTotal = 91
$script:FocusedRawLeafNames = @(
    'autotest.log',
    'autotest_failed.log',
    'console.log',
    'error.log',
    'junit.xml',
    'script.log'
)
$script:FocusedPolicyAssertionNames = @(
    'canonical_profile_identity',
    'sealed_candidate_identity',
    'single_clean_harness_identity',
    'portable_eight_blob_index',
    'all_indexed_blobs_rehashed',
    'passing_junit_case',
    'accepted_guarded_outcome'
)

function Throw-PartisanFocusedAggregate {
    param(
        [Parameter(Mandatory = $true)][string]$Code,
        [Parameter(Mandatory = $true)][string]$Message
    )

    $exception = New-Object IO.InvalidDataException($Message)
    $exception.Data['PartisanFocusedAggregateCode'] = $Code
    throw $exception
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

function Test-PartisanFocusedCandidateId {
    param(
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)][string]$Value
    )

    return $Value -cmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$' -and
        -not $Value.EndsWith(
            '.json.replacement-required',
            [StringComparison]::OrdinalIgnoreCase)
}

function Get-PartisanFocusedSha256 {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (Get-FileHash `
        -LiteralPath $Path `
        -Algorithm SHA256 `
        -ErrorAction Stop).Hash.ToLowerInvariant()
}

function Get-PartisanFocusedBytesSha256 {
    param(
        [AllowEmptyCollection()]
        [Parameter(Mandatory = $true)]
        [byte[]]$Bytes
    )

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($Bytes))).Replace(
                '-', '').ToLowerInvariant()
    }
    finally {
        $algorithm.Dispose()
    }
}

function Get-PartisanFocusedTextSha256 {
    param(
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)]
        [string]$Text
    )

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [Text.Encoding]::UTF8.GetBytes($Text)
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($bytes))).Replace(
                '-', '').ToLowerInvariant()
    }
    finally {
        $algorithm.Dispose()
    }
}

function Invoke-PartisanFocusedGitText {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)][string[]]$ArgumentList
    )

    $gitCommand = Get-Command `
        -Name git `
        -CommandType Application `
        -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($null -eq $gitCommand) {
        return [pscustomobject][ordered]@{
            ExitCode = -1
            Output = @()
        }
    }

    $gitPath = [string]$gitCommand.Path
    $output = @()
    $exitCode = -1
    $previousErrorActionPreference = $ErrorActionPreference
    try {
        # Windows PowerShell 5.1 promotes redirected native stderr to a
        # terminating RemoteException when ErrorActionPreference is Stop.
        # Collect only stdout and classify the native exit code ourselves.
        $ErrorActionPreference = 'Continue'
        $output = @(& $gitPath -C $RepositoryRootPath @ArgumentList 2>$null)
        $exitCode = $LASTEXITCODE
    }
    catch {
        $output = @()
        $exitCode = -1
    }
    finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    return [pscustomobject][ordered]@{
        ExitCode = [int]$exitCode
        Output = @($output)
    }
}

function Get-PartisanFocusedGitHead {
    param([Parameter(Mandatory = $true)][string]$RepositoryRootPath)

    $result = Invoke-PartisanFocusedGitText `
        -RepositoryRootPath $RepositoryRootPath `
        -ArgumentList @('rev-parse', 'HEAD')
    $head = ($result.Output -join '').Trim()
    if ($result.ExitCode -ne 0 -or $head -cnotmatch '^[0-9a-f]{40}$') {
        Throw-PartisanFocusedAggregate `
            -Code 'harness_identity_drift' `
            -Message 'The aggregate repository Git HEAD is unavailable.'
    }
    return $head
}

function Get-PartisanFocusedGitBlobBytes {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)][string]$GitHead,
        [Parameter(Mandatory = $true)][string]$RepositoryPath
    )

    if ($GitHead -cnotmatch '^[0-9a-f]{40}$' -or
        $RepositoryPath -cnotmatch '^[A-Za-z0-9._/-]+$' -or
        $RepositoryPath -match '(^|/)\.\.?($|/)') {
        Throw-PartisanFocusedAggregate `
            -Code 'harness_identity_drift' `
            -Message 'A Git-blob provenance request is invalid.'
    }
    $objectResult = Invoke-PartisanFocusedGitText `
        -RepositoryRootPath $RepositoryRootPath `
        -ArgumentList @(
            'rev-parse',
            ($GitHead + ':' + $RepositoryPath))
    $objectId = ($objectResult.Output -join '').Trim()
    if ($objectResult.ExitCode -ne 0 -or
        $objectId -cnotmatch '^[0-9a-f]{40,64}$') {
        Throw-PartisanFocusedAggregate `
            -Code 'harness_identity_drift' `
            -Message 'A required historical harness Git blob is unavailable.'
    }

    $gitCommand = Get-Command git -ErrorAction Stop
    $start = New-Object Diagnostics.ProcessStartInfo
    $start.FileName = $gitCommand.Source
    $start.Arguments = 'cat-file blob ' + $objectId
    $start.WorkingDirectory = $RepositoryRootPath
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardInput = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $start
    $memory = $null
    $outputTask = $null
    $errorTask = $null
    try {
        if (-not $process.Start()) {
            Throw-PartisanFocusedAggregate `
                -Code 'harness_identity_drift' `
                -Message 'The Git-blob reader could not start.'
        }
        $process.StandardInput.Close()
        $memory = New-Object IO.MemoryStream
        $outputTask = $process.StandardOutput.BaseStream.CopyToAsync($memory)
        $errorTask = $process.StandardError.ReadToEndAsync()
        $completed = [Threading.Tasks.Task]::WaitAll(
            [Threading.Tasks.Task[]]@($outputTask, $errorTask),
            30000)
        if (-not $completed) {
            try {
                $process.Kill()
            }
            catch {
                # Preserve the bounded provenance failure below.
            }
            $process.WaitForExit()
            Throw-PartisanFocusedAggregate `
                -Code 'harness_identity_drift' `
                -Message 'The Git-blob reader exceeded its bounded timeout.'
        }
        $process.WaitForExit()
        $stderr = $errorTask.Result
        if ($process.ExitCode -ne 0 -or
            -not [string]::IsNullOrWhiteSpace($stderr)) {
            Throw-PartisanFocusedAggregate `
                -Code 'harness_identity_drift' `
                -Message 'The required historical harness Git blob could not be read.'
        }
        return $memory.ToArray()
    }
    finally {
        if ($memory) {
            $memory.Dispose()
        }
        $process.Dispose()
    }
}

function Get-PartisanFocusedGitBlobSha256 {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)][string]$GitHead,
        [Parameter(Mandatory = $true)][string]$RepositoryPath
    )

    return Get-PartisanFocusedBytesSha256 -Bytes (
        Get-PartisanFocusedGitBlobBytes `
            -RepositoryRootPath $RepositoryRootPath `
            -GitHead $GitHead `
            -RepositoryPath $RepositoryPath)
}

function Assert-PartisanFocusedHistoricalPathImmutable {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)][string]$GitHead,
        [Parameter(Mandatory = $true)][string]$RepositoryPath,
        [Parameter(Mandatory = $true)][byte[]]$WorktreeBytes,
        [string]$Code = 'historical_blob_replacement'
    )

    $headBytes = Get-PartisanFocusedGitBlobBytes `
        -RepositoryRootPath $RepositoryRootPath `
        -GitHead $GitHead `
        -RepositoryPath $RepositoryPath
    if (-not (Test-PartisanFocusedBytesEqual `
            -Expected $headBytes `
            -Actual $WorktreeBytes)) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message 'Historical evidence bytes differ from tracked HEAD.'
    }

    # Every distinct version of a path that exists in a commit reachable from
    # HEAD must be introduced by a path-changing commit. Reopen the blob at
    # each such commit (plus HEAD) and require one byte identity. Ancestors in
    # which the path is absent are intentionally allowed for first creation.
    $changeResult = Invoke-PartisanFocusedGitText `
        -RepositoryRootPath $RepositoryRootPath `
        -ArgumentList @(
            'log',
            '--format=%H',
            '--full-history',
            '-m',
            '--no-renames',
            $GitHead,
            '--',
            $RepositoryPath)
    $changeLines = @($changeResult.Output)
    if ($changeResult.ExitCode -ne 0) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message 'Historical evidence Git history could not be enumerated.'
    }
    $commits = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    [void]$commits.Add($GitHead)
    foreach ($line in $changeLines) {
        $commit = ([string]$line).Trim()
        if ($commit -cnotmatch '^[0-9a-f]{40}$') {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message 'Historical evidence Git history returned an invalid commit.'
        }
        [void]$commits.Add($commit)
    }

    $contentHashes = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($commit in $commits) {
        $treeResult = Invoke-PartisanFocusedGitText `
            -RepositoryRootPath $RepositoryRootPath `
            -ArgumentList @(
                'ls-tree',
                $commit,
                '--',
                $RepositoryPath)
        if ($treeResult.ExitCode -ne 0) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message 'Historical evidence Git tree could not be inspected.'
        }
        $treeLines = @($treeResult.Output | Where-Object {
            -not [string]::IsNullOrWhiteSpace([string]$_)
        })
        if ($treeLines.Count -eq 0) {
            continue
        }
        if ($treeLines.Count -ne 1) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message 'Historical evidence Git tree returned an ambiguous path.'
        }
        $treeMatch = [regex]::Match(
            [string]$treeLines[0],
            '^(?:100644|100755) blob (?<object>[0-9a-f]{40,64})\t(?<path>.+)$',
            [Text.RegularExpressions.RegexOptions]::CultureInvariant)
        if (-not $treeMatch.Success -or
            $treeMatch.Groups['path'].Value -cne $RepositoryPath) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message 'Historical evidence Git tree returned a noncanonical blob.'
        }
        $blobSha = Get-PartisanFocusedGitBlobSha256 `
            -RepositoryRootPath $RepositoryRootPath `
            -GitHead $commit `
            -RepositoryPath $RepositoryPath
        [void]$contentHashes.Add($blobSha)
        if ($contentHashes.Count -gt 1) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message 'A canonical historical evidence path has contained different committed bytes.'
        }
    }
    $headSha = Get-PartisanFocusedBytesSha256 -Bytes $headBytes
    if ($contentHashes.Count -ne 1 -or
        -not $contentHashes.Contains($headSha)) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message 'Historical evidence does not have one reachable committed byte identity.'
    }
}

function Get-PartisanFocusedAggregationIntegrity {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)]$RunHarness,
        [Parameter(Mandatory = $true)][string]$CandidateSourceHead
    )

    $repositoryFull = Resolve-PartisanFocusedExistingDirectory `
        -Path $RepositoryRootPath `
        -Label 'Aggregate repository root'
    $reportedRootResult = Invoke-PartisanFocusedGitText `
        -RepositoryRootPath $repositoryFull `
        -ArgumentList @('rev-parse', '--show-toplevel')
    $reportedRoot = [IO.Path]::GetFullPath(
        (($reportedRootResult.Output -join '').Trim())).TrimEnd('\', '/')
    if ($reportedRootResult.ExitCode -ne 0 -or
        -not $reportedRoot.Equals(
            $repositoryFull,
            [StringComparison]::OrdinalIgnoreCase)) {
        Throw-PartisanFocusedAggregate `
            -Code 'harness_identity_drift' `
            -Message 'RepositoryRoot must be the exact aggregate Git checkout root.'
    }

    $runHead = [string]$RunHarness.gitHead
    $aggregationHead = Get-PartisanFocusedGitHead `
        -RepositoryRootPath $repositoryFull
    Assert-PartisanFocusedReachableCommit `
        -RepositoryRootPath $repositoryFull `
        -Commit $CandidateSourceHead `
        -Descendant $runHead `
        -Code 'harness_identity_drift' `
        -Label 'Candidate source relative to the focused run harness'
    Assert-PartisanFocusedReachableCommit `
        -RepositoryRootPath $repositoryFull `
        -Commit $runHead `
        -Descendant $aggregationHead `
        -Code 'harness_identity_drift' `
        -Label 'Focused run harness'
    $runnerRepositoryPath = 'tools/run-guarded-focused-autotest.ps1'
    $candidateModuleRepositoryPath = 'tools/Partisan.ReleaseCandidate.psm1'
    $runnerBlobSha = Get-PartisanFocusedGitBlobSha256 `
        -RepositoryRootPath $repositoryFull `
        -GitHead $runHead `
        -RepositoryPath $runnerRepositoryPath
    $candidateModuleBlobSha = Get-PartisanFocusedGitBlobSha256 `
        -RepositoryRootPath $repositoryFull `
        -GitHead $runHead `
        -RepositoryPath $candidateModuleRepositoryPath
    if ($runnerBlobSha -cne [string]$RunHarness.focusedRunnerSha256 -or
        $candidateModuleBlobSha -cne
            [string]$RunHarness.candidateModuleSha256) {
        Throw-PartisanFocusedAggregate `
            -Code 'harness_git_blob_mismatch' `
            -Message 'Recorded focused harness hashes differ from their committed Git blobs.'
    }

    $producerRepositoryPath = 'tools/New-PartisanFocusedAutotestAggregate.ps1'
    $consumerRepositoryPath = 'tools/update-release-docs.ps1'
    $producerWorktreePath = Resolve-PartisanFocusedExistingFile `
        -Path (Join-Path $repositoryFull $producerRepositoryPath) `
        -Label 'Focused aggregate producer' `
        -Code 'aggregation_tool_drift'
    $consumerWorktreePath = Resolve-PartisanFocusedExistingFile `
        -Path (Join-Path $repositoryFull $consumerRepositoryPath) `
        -Label 'Release-doc consumer' `
        -Code 'aggregation_tool_drift'
    $executingProducerSha = Get-PartisanFocusedSha256 -Path $PSCommandPath
    $producerWorktreeSha = Get-PartisanFocusedSha256 -Path $producerWorktreePath
    $consumerWorktreeSha = Get-PartisanFocusedSha256 -Path $consumerWorktreePath
    $producerBlobSha = Get-PartisanFocusedGitBlobSha256 `
        -RepositoryRootPath $repositoryFull `
        -GitHead $aggregationHead `
        -RepositoryPath $producerRepositoryPath
    $consumerBlobSha = Get-PartisanFocusedGitBlobSha256 `
        -RepositoryRootPath $repositoryFull `
        -GitHead $aggregationHead `
        -RepositoryPath $consumerRepositoryPath
    if ($executingProducerSha -cne $producerWorktreeSha -or
        $producerWorktreeSha -cne $producerBlobSha -or
        $consumerWorktreeSha -cne $consumerBlobSha) {
        Throw-PartisanFocusedAggregate `
            -Code 'aggregation_tool_drift' `
            -Message 'The aggregate producer or release-doc consumer differs from its committed Git blob.'
    }

    return [pscustomobject][ordered]@{
        aggregationGitHead = $aggregationHead
        focusedRunHarness = [pscustomobject][ordered]@{
            gitHead = $runHead
            focusedRunnerWorktreeSha256 =
                [string]$RunHarness.focusedRunnerSha256
            focusedRunnerGitBlobSha256 = $runnerBlobSha
            candidateModuleWorktreeSha256 =
                [string]$RunHarness.candidateModuleSha256
            candidateModuleGitBlobSha256 = $candidateModuleBlobSha
        }
        aggregateProducer = [pscustomobject][ordered]@{
            repositoryPath = $producerRepositoryPath
            worktreeSha256 = $producerWorktreeSha
            gitBlobSha256 = $producerBlobSha
        }
        releaseDocsConsumer = [pscustomobject][ordered]@{
            repositoryPath = $consumerRepositoryPath
            worktreeSha256 = $consumerWorktreeSha
            gitBlobSha256 = $consumerBlobSha
        }
        allWorktreeHashesMatchGitBlobs = $true
    }
}

function ConvertTo-PartisanFocusedCanonicalJson {
    param([Parameter(Mandatory = $true)]$Value)

    $json = $Value | ConvertTo-Json -Depth 100 -Compress
    return $json.Replace("`r`n", "`n") + "`n"
}

function Write-PartisanFocusedUtf8File {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [AllowEmptyString()][Parameter(Mandatory = $true)][string]$Text
    )

    [IO.File]::WriteAllText(
        $Path,
        $Text,
        (New-Object Text.UTF8Encoding($false)))
}

function Test-PartisanFocusedBytesEqual {
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

function Assert-PartisanFocusedNoDuplicateJsonProperties {
    param(
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'schema_drift'
    )

    $frames = New-Object 'Collections.Generic.Stack[object]'
    for ($index = 0; $index -lt $Text.Length; $index++) {
        $character = $Text[$index]
        if ($character -eq '{') {
            $frames.Push([pscustomobject]@{
                IsObject = $true
                Keys = New-Object 'Collections.Generic.HashSet[string]' `
                    ([StringComparer]::Ordinal)
            })
            continue
        }
        if ($character -eq '[') {
            $frames.Push([pscustomobject]@{
                IsObject = $false
                Keys = $null
            })
            continue
        }
        if ($character -eq '}' -or $character -eq ']') {
            if ($frames.Count -gt 0) {
                [void]$frames.Pop()
            }
            continue
        }
        if ($character -ne [char]0x22) {
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
            if ($tokenCharacter -eq [char]0x5C) {
                $escaped = $true
                continue
            }
            if ($tokenCharacter -eq [char]0x22) {
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
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message "$Label contains an invalid JSON property name."
        }
        if (-not $frames.Peek().Keys.Add([string]$propertyName)) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message "$Label contains a duplicate JSON property."
        }
    }
}

function Read-PartisanFocusedUtf8Text {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'encoding_invalid'
    )

    try {
        [byte[]]$bytes = [IO.File]::ReadAllBytes($Path)
        if (($bytes.Length -ge 3 -and
                $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and
                $bytes[2] -eq 0xBF) -or
            ($bytes.Length -ge 2 -and
                (($bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) -or
                 ($bytes[0] -eq 0xFE -and $bytes[1] -eq 0xFF))) -or
            ($bytes.Length -ge 4 -and
                (($bytes[0] -eq 0x00 -and $bytes[1] -eq 0x00 -and
                  $bytes[2] -eq 0xFE -and $bytes[3] -eq 0xFF) -or
                 ($bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE -and
                  $bytes[2] -eq 0x00 -and $bytes[3] -eq 0x00)))) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message "$Label must be BOM-less UTF-8."
        }
        $encoding = New-Object Text.UTF8Encoding($false, $true)
        $text = if ($bytes.Length -eq 0) {
            ''
        }
        else {
            $encoding.GetString($bytes)
        }
        if ($text.IndexOf([char]0) -ge 0) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message "$Label contains a NUL character."
        }
        return [pscustomobject][ordered]@{
            Text = $text
            Bytes = $bytes
            Length = [long]$bytes.Length
            Sha256 = Get-PartisanFocusedBytesSha256 -Bytes $bytes
        }
    }
    catch {
        if ($_.Exception.Data.Contains('PartisanFocusedAggregateCode')) {
            throw
        }
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label is not strict BOM-less UTF-8."
    }
}

function Invoke-PartisanFocusedSelfTestIndexedSemanticBarrier {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label,
        [Parameter(Mandatory = $true)][ValidateSet('Before', 'After')]
        [string]$Phase
    )

    $token = [string]$env:PARTISAN_FOCUSED_AGGREGATE_SELFTEST_SEMANTIC_TOKEN
    if ([string]::IsNullOrEmpty($token) -or
        $Label -cne 'Focused console blob') {
        return
    }
    if (($Phase -ceq 'Before' -and
            $script:FocusedSemanticBeforeBarrierUsed) -or
        ($Phase -ceq 'After' -and
            $script:FocusedSemanticAfterBarrierUsed)) {
        return
    }
    if ($Phase -ceq 'Before') {
        $script:FocusedSemanticBeforeBarrierUsed = $true
    }
    else {
        $script:FocusedSemanticAfterBarrierUsed = $true
    }
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') +
        [IO.Path]::DirectorySeparatorChar
    $full = [IO.Path]::GetFullPath($Path)
    $relative = if ($full.StartsWith(
            $temp,
            [StringComparison]::OrdinalIgnoreCase)) {
        $full.Substring($temp.Length)
    }
    else {
        ''
    }
    $firstSegment = @($relative -split '[\\/]' | Where-Object { $_ } |
        Select-Object -First 1)
    if ($token -cnotmatch '^[0-9a-f]{32}$' -or
        $firstSegment.Count -ne 1 -or
        $firstSegment[0] -cnotmatch
            '^PartisanFocusedAggregateSelfTest-[0-9a-f]{32}$') {
        Throw-PartisanFocusedAggregate `
            -Code 'raw_result_tampering' `
            -Message 'The bounded semantic-snapshot self-test seam was denied.'
    }
    $ready = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanFocusedAggregateSemantic' + $Phase + 'Ready-' + $token))
    $continued = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanFocusedAggregateSemantic' + $Phase + 'Continue-' + $token))
    try {
        [void]$ready.Set()
        if (-not $continued.WaitOne(15000)) {
            Throw-PartisanFocusedAggregate `
                -Code 'raw_result_tampering' `
                -Message 'The bounded semantic-snapshot self-test seam timed out.'
        }
    }
    finally {
        $ready.Dispose()
        $continued.Dispose()
    }
}

function Read-PartisanFocusedIndexedUtf8Text {
    param(
        [Parameter(Mandatory = $true)]$Row,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'raw_blob_tampering'
    )

    Invoke-PartisanFocusedSelfTestIndexedSemanticBarrier `
        -Path ([string]$Row.fullPath) `
        -Label $Label `
        -Phase 'Before'
    $input = Read-PartisanFocusedUtf8Text `
        -Path ([string]$Row.fullPath) `
        -Label $Label `
        -Code $Code
    Invoke-PartisanFocusedSelfTestIndexedSemanticBarrier `
        -Path ([string]$Row.fullPath) `
        -Label $Label `
        -Phase 'After'
    if ([long]$input.Length -ne [long]$Row.length -or
        [string]$input.Sha256 -cne [string]$Row.sha256) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label bytes differ from the indexed blob used for semantic validation."
    }
    return $input
}

function Read-PartisanFocusedJsonBytes {
    param(
        [AllowEmptyCollection()]
        [Parameter(Mandatory = $true)][byte[]]$Bytes,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'schema_drift'
    )

    try {
        if (($Bytes.Length -ge 3 -and
                $Bytes[0] -eq 0xEF -and $Bytes[1] -eq 0xBB -and
                $Bytes[2] -eq 0xBF) -or
            ($Bytes.Length -ge 2 -and
                (($Bytes[0] -eq 0xFF -and $Bytes[1] -eq 0xFE) -or
                 ($Bytes[0] -eq 0xFE -and $Bytes[1] -eq 0xFF))) -or
            ($Bytes.Length -ge 4 -and
                (($Bytes[0] -eq 0x00 -and $Bytes[1] -eq 0x00 -and
                  $Bytes[2] -eq 0xFE -and $Bytes[3] -eq 0xFF) -or
                 ($Bytes[0] -eq 0xFF -and $Bytes[1] -eq 0xFE -and
                  $Bytes[2] -eq 0x00 -and $Bytes[3] -eq 0x00)))) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message "$Label must be BOM-less UTF-8."
        }
        $text = (New-Object Text.UTF8Encoding($false, $true)).GetString($Bytes)
        if ($text.IndexOf([char]0) -ge 0) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message "$Label contains a NUL character."
        }
        Assert-PartisanFocusedNoDuplicateJsonProperties `
            -Text $text `
            -Label $Label `
            -Code $Code
        return $text | ConvertFrom-Json
    }
    catch {
        if ($_.Exception.Data.Contains('PartisanFocusedAggregateCode')) {
            throw
        }
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label is not strict BOM-less UTF-8 JSON."
    }
}

function Initialize-PartisanFocusedOutputParent {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)][string]$OutputPath
    )

    $repository = Resolve-PartisanFocusedExistingDirectory `
        -Path $RepositoryRootPath `
        -Label 'Aggregate repository root'
    $full = [IO.Path]::GetFullPath($OutputPath)
    if (-not (Test-PartisanFocusedContainedPath `
            -Root $repository `
            -Path $full)) {
        Throw-PartisanFocusedAggregate `
            -Code 'output_path_invalid' `
            -Message 'The aggregate output escaped its repository.'
    }
    $parent = Split-Path -Parent $full
    $relativeParent = $parent.Substring($repository.Length).TrimStart('\', '/')
    $cursor = $repository
    foreach ($segment in @($relativeParent -split '[\\/]' | Where-Object { $_ })) {
        $cursor = Join-Path $cursor $segment
        if (-not (Test-Path -LiteralPath $cursor)) {
            New-Item -ItemType Directory -Path $cursor -ErrorAction Stop |
                Out-Null
        }
        $item = Get-Item -LiteralPath $cursor -Force -ErrorAction Stop
        if (-not $item.PSIsContainer -or
            ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            Throw-PartisanFocusedAggregate `
                -Code 'output_reparse_path' `
                -Message 'The aggregate output parent has unsafe ancestry.'
        }
    }
    Assert-PartisanFocusedNoReparseAncestry `
        -Root $repository `
        -Path $parent `
        -Label 'Aggregate output parent'
    if (Test-Path -LiteralPath $full) {
        $outputItem = Get-Item -LiteralPath $full -Force -ErrorAction Stop
        if ($outputItem.PSIsContainer -or
            ($outputItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            Throw-PartisanFocusedAggregate `
                -Code 'output_reparse_path' `
                -Message 'The aggregate output path is not a regular file.'
        }
    }
    return $full
}

function Enter-PartisanFocusedCandidatePublicationLock {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)][string]$CandidateId
    )

    if (-not (Test-PartisanFocusedCandidateId -Value $CandidateId)) {
        Throw-PartisanFocusedAggregate `
            -Code 'output_path_invalid' `
            -Message 'The candidate publication lock identity is invalid.'
    }
    $path = Join-Path $RepositoryRootPath (
        $script:FocusedTrackedEvidencePrefix.Replace('/', '\') + '\.' +
        $CandidateId + '.publication.lock')
    $stopwatch = [Diagnostics.Stopwatch]::StartNew()
    while ($true) {
        [void](Initialize-PartisanFocusedOutputParent `
            -RepositoryRootPath $RepositoryRootPath `
            -OutputPath $path)
        $stream = $null
        try {
            $stream = [IO.File]::Open(
                $path,
                [IO.FileMode]::OpenOrCreate,
                [IO.FileAccess]::ReadWrite,
                [IO.FileShare]::None)
            [void](Initialize-PartisanFocusedOutputParent `
                -RepositoryRootPath $RepositoryRootPath `
                -OutputPath $path)
            $result = [pscustomobject][ordered]@{
                Path = $path
                Stream = $stream
            }
            $stream = $null
            return $result
        }
        catch [IO.IOException] {
            if ($null -ne $stream) {
                $stream.Dispose()
            }
            if ($stopwatch.ElapsedMilliseconds -ge 15000) {
                Throw-PartisanFocusedAggregate `
                    -Code 'publication_lock_timeout' `
                    -Message 'The candidate publication lock timed out.'
            }
            Start-Sleep -Milliseconds 50
        }
        catch {
            if ($null -ne $stream) {
                $stream.Dispose()
            }
            throw
        }
    }
}

function Exit-PartisanFocusedCandidatePublicationLock {
    param(
        [Parameter(Mandatory = $true)]$Lock,
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath
    )

    if ($null -ne $Lock.Stream) {
        $Lock.Stream.Dispose()
    }
    try {
        [void](Initialize-PartisanFocusedOutputParent `
            -RepositoryRootPath $RepositoryRootPath `
            -OutputPath ([string]$Lock.Path))
        if (Test-Path -LiteralPath ([string]$Lock.Path) -PathType Leaf) {
            Remove-Item -LiteralPath ([string]$Lock.Path) -Force -ErrorAction Stop
        }
    }
    catch {
        # A waiting publisher may already own the same stable lock file.
    }
}

function Write-PartisanFocusedImmutableJson {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string]$ConflictCode,
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [scriptblock]$BeforeCommit,
        [scriptblock]$AfterCommit
    )

    $full = Initialize-PartisanFocusedOutputParent `
        -RepositoryRootPath $RepositoryRootPath `
        -OutputPath $Path
    $parent = Split-Path -Parent $full
    $text = ConvertTo-PartisanFocusedCanonicalJson -Value $Value
    $expectedBytes = (New-Object Text.UTF8Encoding($false, $true)).GetBytes($text)
    $expectedSha = Get-PartisanFocusedBytesSha256 -Bytes $expectedBytes
    if (Test-Path -LiteralPath $full -PathType Leaf) {
        [void](Initialize-PartisanFocusedOutputParent `
            -RepositoryRootPath $RepositoryRootPath `
            -OutputPath $full)
        $existingBytes = [IO.File]::ReadAllBytes($full)
        if (-not (Test-PartisanFocusedBytesEqual `
                -Expected $expectedBytes `
                -Actual $existingBytes)) {
            Throw-PartisanFocusedAggregate `
                -Code $ConflictCode `
                -Message 'An immutable aggregate path already contains different bytes.'
        }
        if ((Get-PartisanFocusedSha256 -Path $full) -cne $expectedSha) {
            Throw-PartisanFocusedAggregate `
                -Code $ConflictCode `
                -Message 'The immutable aggregate changed while it was verified.'
        }
        if ($null -ne $BeforeCommit) {
            & $BeforeCommit
        }
        [void](Initialize-PartisanFocusedOutputParent `
            -RepositoryRootPath $RepositoryRootPath `
            -OutputPath $full)
        if (-not (Test-PartisanFocusedBytesEqual `
                -Expected $expectedBytes `
                -Actual ([IO.File]::ReadAllBytes($full)))) {
            Throw-PartisanFocusedAggregate `
                -Code $ConflictCode `
                -Message 'The immutable aggregate changed at its idempotent publication barrier.'
        }
        if ($null -ne $AfterCommit) {
            & $AfterCommit
        }
        return [pscustomobject][ordered]@{
            Path = $full
            Sha256 = $expectedSha
            Created = $false
        }
    }
    if (Test-Path -LiteralPath $full) {
        Throw-PartisanFocusedAggregate `
            -Code 'output_path_invalid' `
            -Message 'The aggregate output path is not a regular file path.'
    }

    $temporaryNamePattern = '^\.' +
        [regex]::Escape((Split-Path -Leaf $full)) +
        '\.[0-9a-f]{32}\.tmp$'
    $existingTemporary = @(Get-ChildItem `
        -LiteralPath $parent `
        -File `
        -Force `
        -ErrorAction Stop | Where-Object {
            $_.Name -cmatch $temporaryNamePattern
        })
    if ($existingTemporary.Count -ne 0) {
        Throw-PartisanFocusedAggregate `
            -Code 'publication_cleanup_failed' `
            -Message 'Immutable aggregate publication found temporary-file residue.'
    }
    $temporary = Join-Path $parent (
        '.' + (Split-Path -Leaf $full) + '.' +
        [Guid]::NewGuid().ToString('N') + '.tmp')
    $created = $true
    try {
        [void](Initialize-PartisanFocusedOutputParent `
            -RepositoryRootPath $RepositoryRootPath `
            -OutputPath $full)
        $temporaryStream = [IO.File]::Open(
            $temporary,
            [IO.FileMode]::CreateNew,
            [IO.FileAccess]::Write,
            [IO.FileShare]::None)
        try {
            $temporaryStream.Write($expectedBytes, 0, $expectedBytes.Length)
            $temporaryStream.Flush($true)
        }
        finally {
            $temporaryStream.Dispose()
        }
        [void](Initialize-PartisanFocusedOutputParent `
            -RepositoryRootPath $RepositoryRootPath `
            -OutputPath $full)
        if ($null -ne $BeforeCommit) {
            & $BeforeCommit
        }
        [void](Initialize-PartisanFocusedOutputParent `
            -RepositoryRootPath $RepositoryRootPath `
            -OutputPath $full)
        Invoke-PartisanFocusedSelfTestOutputParentBarrier `
            -RepositoryRootPath $RepositoryRootPath `
            -OutputPath $full
        [void](Initialize-PartisanFocusedOutputParent `
            -RepositoryRootPath $RepositoryRootPath `
            -OutputPath $full)
        try {
            [IO.File]::Move($temporary, $full)
        }
        catch {
            $created = $false
            [void](Initialize-PartisanFocusedOutputParent `
                -RepositoryRootPath $RepositoryRootPath `
                -OutputPath $full)
            if (-not (Test-Path -LiteralPath $full -PathType Leaf) -or
                -not (Test-PartisanFocusedBytesEqual `
                    -Expected $expectedBytes `
                    -Actual ([IO.File]::ReadAllBytes($full)))) {
                Throw-PartisanFocusedAggregate `
                    -Code $ConflictCode `
                    -Message 'Concurrent aggregate publication changed the immutable output.'
            }
        }
    }
    finally {
        if (Test-Path -LiteralPath $temporary) {
            try {
                Remove-Item `
                    -LiteralPath $temporary `
                    -Force `
                    -ErrorAction Stop
            }
            catch {
                Throw-PartisanFocusedAggregate `
                    -Code 'publication_cleanup_failed' `
                    -Message 'Immutable aggregate temporary-file cleanup failed.'
            }
        }
        if (Test-Path -LiteralPath $temporary) {
            Throw-PartisanFocusedAggregate `
                -Code 'publication_cleanup_failed' `
                -Message 'Immutable aggregate temporary-file cleanup left residue.'
        }
    }

    [void](Initialize-PartisanFocusedOutputParent `
        -RepositoryRootPath $RepositoryRootPath `
        -OutputPath $full)
    $publishedBytes = [IO.File]::ReadAllBytes($full)
    if (-not (Test-PartisanFocusedBytesEqual `
            -Expected $expectedBytes `
            -Actual $publishedBytes) -or
        (Get-PartisanFocusedSha256 -Path $full) -cne $expectedSha) {
        Throw-PartisanFocusedAggregate `
            -Code $ConflictCode `
            -Message 'The immutable aggregate failed its post-publication rehash.'
    }
    if ($null -ne $AfterCommit) {
        & $AfterCommit
    }
    [void](Initialize-PartisanFocusedOutputParent `
        -RepositoryRootPath $RepositoryRootPath `
        -OutputPath $full)
    if (-not (Test-PartisanFocusedBytesEqual `
            -Expected $expectedBytes `
            -Actual ([IO.File]::ReadAllBytes($full)))) {
        Throw-PartisanFocusedAggregate `
            -Code $ConflictCode `
            -Message 'The immutable aggregate changed after its publication barrier.'
    }
    return [pscustomobject][ordered]@{
        Path = $full
        Sha256 = $expectedSha
        Created = $created
    }
}

function Assert-PartisanFocusedProperties {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string[]]$Names,
        [Parameter(Mandatory = $true)][string]$Label,
        [switch]$Exact,
        [string]$Code = 'schema_drift'
    )

    if ($null -eq $Value) {
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label is missing."
    }
    $actual = @($Value.PSObject.Properties.Name)
    foreach ($name in $Names) {
        if ($actual -cnotcontains $name) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message "$Label is missing required property $name."
        }
    }
    if ($Exact) {
        $difference = @(Compare-Object `
            -ReferenceObject @($Names | Sort-Object) `
            -DifferenceObject @($actual | Sort-Object) `
            -CaseSensitive)
        if ($difference.Count -ne 0) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message "$Label has schema drift."
        }
    }
}

function Assert-PartisanFocusedArray {
    param(
        [AllowEmptyCollection()]
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'schema_drift'
    )

    if ($Value -isnot [Array]) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label is not an array."
    }
}

function Require-PartisanFocusedText {
    param(
        $Value,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'schema_drift'
    )

    if ($Value -isnot [string] -or [string]::IsNullOrWhiteSpace([string]$Value)) {
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label is invalid."
    }
    return [string]$Value
}

function Require-PartisanFocusedInteger {
    param(
        $Value,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'schema_drift'
    )

    if ($Value -isnot [byte] -and
        $Value -isnot [int16] -and
        $Value -isnot [int32] -and
        $Value -isnot [int64]) {
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label is not an integer."
    }
    return [long]$Value
}

function Require-PartisanFocusedBoolean {
    param(
        $Value,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'schema_drift'
    )

    if ($Value -isnot [bool]) {
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label is not Boolean."
    }
    return [bool]$Value
}

function Require-PartisanFocusedSha256 {
    param(
        $Value,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'candidate_tampering'
    )

    $text = Require-PartisanFocusedText -Value $Value -Label $Label -Code $Code
    if ($text -cnotmatch '^[0-9a-f]{64}$') {
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label is not SHA-256."
    }
    return $text
}

function Require-PartisanFocusedUtc {
    param(
        $Value,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'policy_drift'
    )

    $text = Require-PartisanFocusedText -Value $Value -Label $Label -Code $Code
    $parsed = [DateTimeOffset]::MinValue
    if ($text -cnotmatch
            '^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d{1,7})?Z$' -or
        -not [DateTimeOffset]::TryParse(
            $text,
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::AssumeUniversal,
            [ref]$parsed) -or
        $parsed.Offset -ne [TimeSpan]::Zero) {
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label is not UTC."
    }
    return $parsed
}

function Assert-PartisanFocusedPortablePath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'index_tampering'
    )

    if ($Path.Length -gt 512 -or
        $Path -cnotmatch '^[A-Za-z0-9._-]+(?:/[A-Za-z0-9._-]+)*$' -or
        $Path -match '(^|/)\.\.?($|/)') {
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label is not portable."
    }
    return $Path
}

function Get-PartisanFocusedCanonicalPackageSha256 {
    param(
        [Parameter(Mandatory = $true)]$Package,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'candidate_tampering'
    )

    Assert-PartisanFocusedProperties `
        -Value $Package `
        -Names @('root', 'hashAlgorithm', 'sha256', 'canonicalIndexPath', 'files') `
        -Label $Label `
        -Exact `
        -Code $Code
    Assert-PartisanFocusedArray `
        -Value $Package.files `
        -Label "$Label files" `
        -Code $Code
    $packageRoot = Require-PartisanFocusedText `
        -Value $Package.root `
        -Label "$Label root" `
        -Code $Code
    $hashAlgorithm = Require-PartisanFocusedText `
        -Value $Package.hashAlgorithm `
        -Label "$Label hash algorithm" `
        -Code $Code
    $canonicalIndexPath = Require-PartisanFocusedText `
        -Value $Package.canonicalIndexPath `
        -Label "$Label canonical index path" `
        -Code $Code
    $declaredPackageSha = Require-PartisanFocusedSha256 `
        -Value $Package.sha256 `
        -Label "$Label SHA-256" `
        -Code $Code
    if ($packageRoot -cne 'package/Partisan' -or
        $hashAlgorithm -cne 'sha256-manifest-v1' -or
        $canonicalIndexPath -cne 'evidence/pack/files.sha256' -or
        @($Package.files).Count -ne 4) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label does not use the canonical package contract."
    }

    $expectedIndexPathByPath =
        New-Object 'Collections.Generic.Dictionary[string,string]' `
            ([StringComparer]::Ordinal)
    $expectedIndexPathByPath.Add(
        'package/Partisan/addon.gproj',
        'Partisan/addon.gproj')
    $expectedIndexPathByPath.Add(
        'package/Partisan/data.pak',
        'Partisan/data.pak')
    $expectedIndexPathByPath.Add(
        'package/Partisan/resourceDatabase.rdb',
        'Partisan/resourceDatabase.rdb')
    $expectedIndexPathByPath.Add(
        'package/Partisan/thumbnail.png',
        'Partisan/thumbnail.png')
    $packagePaths = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $packageIndexPaths = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $canonicalRows = New-Object Collections.Generic.List[object]
    foreach ($packageRow in @($Package.files)) {
        Assert-PartisanFocusedProperties `
            -Value $packageRow `
            -Names @('path', 'indexPath', 'length', 'sha256') `
            -Label "$Label row" `
            -Exact `
            -Code $Code
        $packagePath = Require-PartisanFocusedText `
            -Value $packageRow.path `
            -Label "$Label path" `
            -Code $Code
        $packageIndexPath = Require-PartisanFocusedText `
            -Value $packageRow.indexPath `
            -Label "$Label index path" `
            -Code $Code
        Assert-PartisanFocusedPortablePath `
            -Path $packagePath `
            -Label "$Label path" `
            -Code $Code | Out-Null
        Assert-PartisanFocusedPortablePath `
            -Path $packageIndexPath `
            -Label "$Label index path" `
            -Code $Code | Out-Null
        $packageLength = Require-PartisanFocusedInteger `
            -Value $packageRow.length `
            -Label "$Label length" `
            -Code $Code
        $packageFileSha = Require-PartisanFocusedSha256 `
            -Value $packageRow.sha256 `
            -Label "$Label file SHA-256" `
            -Code $Code
        $expectedIndexPath = ''
        if (-not $expectedIndexPathByPath.TryGetValue(
                $packagePath,
                [ref]$expectedIndexPath) -or
            $packageIndexPath -cne $expectedIndexPath -or
            -not $packagePaths.Add($packagePath) -or
            -not $packageIndexPaths.Add($packageIndexPath) -or
            $packageLength -le 0) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message "$Label is not the exact canonical four-file tuple set."
        }
        [void]$canonicalRows.Add([pscustomobject][ordered]@{
            indexPath = $packageIndexPath
            length = $packageLength
            sha256 = $packageFileSha
        })
    }
    if ($packagePaths.Count -ne 4 -or $packageIndexPaths.Count -ne 4) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label does not contain four unique canonical paths."
    }
    $canonicalIndex = (@($canonicalRows | Sort-Object `
        @{ Expression = { [string]$_.indexPath }; Ascending = $true } `
        -CaseSensitive | ForEach-Object {
            "{0}`t{1}`t{2}" -f
                ([string]$_.sha256),
                ([long]$_.length),
                ([string]$_.indexPath)
        }) -join "`n") + "`n"
    $recomputedPackageSha = Get-PartisanFocusedTextSha256 `
        -Text $canonicalIndex
    if ($declaredPackageSha -cne $recomputedPackageSha) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label declared SHA-256 differs from its canonical index."
    }
    # Downstream seals bind the declared value only after byte-for-byte equality.
    return $declaredPackageSha
}

function Resolve-PartisanFocusedExistingFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'index_tampering'
    )

    $full = [IO.Path]::GetFullPath($Path)
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label is missing."
    }
    return $full
}

function Resolve-PartisanFocusedExistingDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $full = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    if (-not (Test-Path -LiteralPath $full -PathType Container)) {
        Throw-PartisanFocusedAggregate `
            -Code 'evidence_root_invalid' `
            -Message "$Label is missing."
    }
    return $full
}

function Test-PartisanFocusedContainedPath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Path,
        [switch]$AllowEqual
    )

    $rootRaw = [IO.Path]::GetFullPath($Root)
    $rootIsVolume = $rootRaw.Equals(
        [IO.Path]::GetPathRoot($rootRaw),
        [StringComparison]::OrdinalIgnoreCase)
    $rootFull = if ($rootIsVolume) {
        $rootRaw
    }
    else {
        $rootRaw.TrimEnd('\', '/')
    }
    $pathRaw = [IO.Path]::GetFullPath($Path)
    $pathFull = if ($pathRaw.Equals(
            [IO.Path]::GetPathRoot($pathRaw),
            [StringComparison]::OrdinalIgnoreCase)) {
        $pathRaw
    }
    else {
        $pathRaw.TrimEnd('\', '/')
    }
    if ($AllowEqual -and $pathFull.Equals(
            $rootFull,
            [StringComparison]::OrdinalIgnoreCase)) {
        return $true
    }
    $rootPrefix = if ($rootIsVolume) {
        $rootFull
    }
    else {
        $rootFull + [IO.Path]::DirectorySeparatorChar
    }
    return $pathFull.StartsWith(
        $rootPrefix,
        [StringComparison]::OrdinalIgnoreCase)
}

function Get-PartisanFocusedPortableRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $pathFull = [IO.Path]::GetFullPath($Path)
    if (-not (Test-PartisanFocusedContainedPath -Root $rootFull -Path $pathFull)) {
        Throw-PartisanFocusedAggregate `
            -Code 'path_escape' `
            -Message "$Label is outside the evidence root."
    }
    $relative = $pathFull.Substring($rootFull.Length + 1).Replace('\', '/')
    return Assert-PartisanFocusedPortablePath -Path $relative -Label $Label
}

function Assert-PartisanFocusedNoReparseAncestry {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $rootRaw = [IO.Path]::GetFullPath($Root)
    $rootFull = if ($rootRaw.Equals(
            [IO.Path]::GetPathRoot($rootRaw),
            [StringComparison]::OrdinalIgnoreCase)) {
        $rootRaw
    }
    else {
        $rootRaw.TrimEnd('\', '/')
    }
    $cursorRaw = [IO.Path]::GetFullPath($Path)
    $cursor = if ($cursorRaw.Equals(
            [IO.Path]::GetPathRoot($cursorRaw),
            [StringComparison]::OrdinalIgnoreCase)) {
        $cursorRaw
    }
    else {
        $cursorRaw.TrimEnd('\', '/')
    }
    if (-not (Test-PartisanFocusedContainedPath `
            -Root $rootFull `
            -Path $cursor `
            -AllowEqual)) {
        Throw-PartisanFocusedAggregate `
            -Code 'path_escape' `
            -Message "$Label is outside its evidence root."
    }
    while ($true) {
        $item = Get-Item -LiteralPath $cursor -Force -ErrorAction Stop
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            Throw-PartisanFocusedAggregate `
                -Code 'reparse_path' `
                -Message "$Label uses a reparse point."
        }
        if ($cursor.Equals($rootFull, [StringComparison]::OrdinalIgnoreCase)) {
            break
        }
        $parent = Split-Path -Parent $cursor
        if ([string]::IsNullOrWhiteSpace($parent) -or
            $parent.Equals($cursor, [StringComparison]::OrdinalIgnoreCase)) {
            Throw-PartisanFocusedAggregate `
                -Code 'path_escape' `
                -Message "$Label has invalid ancestry."
        }
        $cursor = if ($parent.Equals(
                [IO.Path]::GetPathRoot($parent),
                [StringComparison]::OrdinalIgnoreCase)) {
            $parent
        }
        else {
            $parent.TrimEnd('\', '/')
        }
    }
}

function ConvertTo-PartisanFocusedComparableJson {
    param($Value)

    if ($null -eq $Value) {
        return 'null'
    }
    return ($Value | ConvertTo-Json -Depth 100 -Compress)
}

function Get-PartisanFocusedAggregateId {
    param(
        [Parameter(Mandatory = $true)]$Integrity,
        [Parameter(Mandatory = $true)][object[]]$SourceRuns
    )

    $bindingText = (ConvertTo-PartisanFocusedComparableJson $Integrity) + "`n" +
        (@($SourceRuns | ForEach-Object {
            $_.testCase + '|' + $_.runId + '|' + $_.runEnvelopeSha256 + '|' +
            (@($_.files | ForEach-Object {
                $_.path + ':' + $_.length + ':' + $_.sha256
            }) -join ',')
        }) -join "`n")
    return 'focused-set-' +
        (Get-PartisanFocusedTextSha256 -Text $bindingText).Substring(0, 24)
}

function Assert-PartisanFocusedSameValue {
    param(
        $Expected,
        $Actual,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'candidate_identity_drift'
    )

    if ((ConvertTo-PartisanFocusedComparableJson $Expected) -cne
        (ConvertTo-PartisanFocusedComparableJson $Actual)) {
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label differs."
    }
}

function Assert-PartisanFocusedExecutableIdentity {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string]$Label
    )

    Assert-PartisanFocusedProperties `
        -Value $Value `
        -Names @('fileName', 'fileVersion', 'productVersion', 'length', 'sha256') `
        -Label $Label `
        -Exact `
        -Code 'tool_identity_drift'
    $fileName = Require-PartisanFocusedText `
        -Value $Value.fileName `
        -Label "$Label.fileName" `
        -Code 'tool_identity_drift'
    if ($fileName -cnotmatch '^[A-Za-z0-9._-]+\.exe$' -or
        (Require-PartisanFocusedInteger `
            -Value $Value.length `
            -Label "$Label.length" `
            -Code 'tool_identity_drift') -le 0) {
        Throw-PartisanFocusedAggregate `
            -Code 'tool_identity_drift' `
            -Message "$Label is invalid."
    }
    Require-PartisanFocusedText `
        -Value $Value.fileVersion `
        -Label "$Label.fileVersion" `
        -Code 'tool_identity_drift' | Out-Null
    Require-PartisanFocusedText `
        -Value $Value.productVersion `
        -Label "$Label.productVersion" `
        -Code 'tool_identity_drift' | Out-Null
    Require-PartisanFocusedSha256 `
        -Value $Value.sha256 `
        -Label "$Label.sha256" `
        -Code 'tool_identity_drift' | Out-Null
}

function Read-PartisanFocusedJson {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'schema_drift'
    )

    try {
        $input = Read-PartisanFocusedUtf8Text `
            -Path $Path `
            -Label $Label `
            -Code $Code
        Assert-PartisanFocusedNoDuplicateJsonProperties `
            -Text $input.Text `
            -Label $Label `
            -Code $Code
        $value = $input.Text | ConvertFrom-Json
        return [pscustomobject][ordered]@{
            Text = $input.Text
            Bytes = $input.Bytes
            Length = $input.Length
            Sha256 = $input.Sha256
            Value = $value
        }
    }
    catch {
        if ($_.Exception.Data.Contains('PartisanFocusedAggregateCode')) {
            throw
        }
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label is not valid JSON."
    }
}

function Get-PartisanFocusedIndexedFiles {
    param(
        [Parameter(Mandatory = $true)][string]$EvidenceRootPath,
        [Parameter(Mandatory = $true)][string]$RunPath,
        [Parameter(Mandatory = $true)]$Run
    )

    $runRoot = Split-Path -Parent $RunPath
    Assert-PartisanFocusedNoReparseAncestry `
        -Root $EvidenceRootPath `
        -Path $runRoot `
        -Label 'Focused run root'
    $indexed = @($Run.files)
    if ($indexed.Count -ne 8) {
        Throw-PartisanFocusedAggregate `
            -Code 'index_tampering' `
            -Message 'A focused run must index exactly eight blobs.'
    }

    $rows = New-Object Collections.Generic.List[object]
    $seen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $seenInsensitive = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::OrdinalIgnoreCase)
    foreach ($row in $indexed) {
        Assert-PartisanFocusedProperties `
            -Value $row `
            -Names @('path', 'length', 'sha256') `
            -Label 'Focused blob index row' `
            -Exact `
            -Code 'index_tampering'
        $portable = Assert-PartisanFocusedPortablePath `
            -Path (Require-PartisanFocusedText `
                -Value $row.path `
                -Label 'Focused blob index path' `
                -Code 'index_tampering') `
            -Label 'Focused blob index path'
        if (-not $seen.Add($portable) -or
            -not $seenInsensitive.Add($portable)) {
            Throw-PartisanFocusedAggregate `
                -Code 'index_tampering' `
                -Message 'The focused blob index contains a duplicate path.'
        }
        $length = Require-PartisanFocusedInteger `
            -Value $row.length `
            -Label 'Focused blob length' `
            -Code 'index_tampering'
        if ($length -lt 0) {
            Throw-PartisanFocusedAggregate `
                -Code 'index_tampering' `
                -Message 'A focused blob length is negative.'
        }
        $sha = Require-PartisanFocusedSha256 `
            -Value $row.sha256 `
            -Label 'Focused blob SHA-256' `
            -Code 'index_tampering'
        $full = [IO.Path]::GetFullPath(
            (Join-Path $runRoot $portable.Replace('/', '\')))
        if (-not (Test-PartisanFocusedContainedPath `
                -Root $runRoot `
                -Path $full) -or
            -not (Test-Path -LiteralPath $full -PathType Leaf)) {
            Throw-PartisanFocusedAggregate `
                -Code 'index_tampering' `
                -Message 'A focused indexed blob is missing or escaped its run root.'
        }
        Assert-PartisanFocusedNoReparseAncestry `
            -Root $runRoot `
            -Path $full `
            -Label 'Focused indexed blob'
        $item = Get-Item -LiteralPath $full -Force -ErrorAction Stop
        if ([long]$item.Length -ne $length -or
            (Get-PartisanFocusedSha256 -Path $full) -cne $sha) {
            Throw-PartisanFocusedAggregate `
                -Code 'raw_blob_tampering' `
                -Message 'A focused indexed blob differs from its recorded hash.'
        }
        [void]$rows.Add([pscustomobject][ordered]@{
            path = $portable
            length = $length
            sha256 = $sha
            fullPath = $full
        })
    }

    $actualPaths = @(
        Get-ChildItem `
            -LiteralPath $runRoot `
            -Recurse `
            -File `
            -Force `
            -ErrorAction Stop | Where-Object {
                -not $_.FullName.Equals(
                    $RunPath,
                    [StringComparison]::OrdinalIgnoreCase)
            } | ForEach-Object {
                Assert-PartisanFocusedNoReparseAncestry `
                    -Root $runRoot `
                    -Path $_.FullName `
                    -Label 'Focused run file'
                Get-PartisanFocusedPortableRelativePath `
                    -Root $runRoot `
                    -Path $_.FullName `
                    -Label 'Focused run file'
            } | Sort-Object
    )
    $indexedPaths = @($rows | ForEach-Object { $_.path } | Sort-Object)
    if (@(Compare-Object `
            -ReferenceObject $indexedPaths `
            -DifferenceObject $actualPaths `
            -CaseSensitive).Count -ne 0) {
        Throw-PartisanFocusedAggregate `
            -Code 'index_tampering' `
            -Message 'The focused blob index is not an exact run-directory census.'
    }

    $expectedIdentity = @(
        'identity/candidate.json',
        'identity/candidate.ready.json'
    )
    foreach ($identityPath in $expectedIdentity) {
        if ($indexedPaths -cnotcontains $identityPath) {
            Throw-PartisanFocusedAggregate `
                -Code 'candidate_tampering' `
                -Message 'A focused run is missing a sealed candidate identity blob.'
        }
    }
    foreach ($leaf in $script:FocusedRawLeafNames) {
        $matching = @($rows | Where-Object {
            $_.path -cmatch ('^raw/(?:[A-Za-z0-9._-]+/)+' +
                [regex]::Escape($leaf) + '$')
        })
        if ($matching.Count -ne 1) {
            Throw-PartisanFocusedAggregate `
                -Code 'index_tampering' `
                -Message "The focused run must contain exactly one $leaf blob."
        }
    }

    return $rows.ToArray()
}

function Assert-PartisanFocusedReachableCommit {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)][string]$Commit,
        [Parameter(Mandatory = $true)][string]$Descendant,
        [string]$Code = 'candidate_tampering',
        [string]$Label = 'Candidate source'
    )

    if ($Commit -cnotmatch '^[0-9a-f]{40}$' -or
        $Descendant -cnotmatch '^[0-9a-f]{40}$') {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label commit identity is invalid."
    }
    $commitResult = Invoke-PartisanFocusedGitText `
        -RepositoryRootPath $RepositoryRootPath `
        -ArgumentList @('cat-file', '-e', ($Commit + '^{commit}'))
    if ($commitResult.ExitCode -ne 0) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label commit is unavailable."
    }
    $ancestorResult = Invoke-PartisanFocusedGitText `
        -RepositoryRootPath $RepositoryRootPath `
        -ArgumentList @('merge-base', '--is-ancestor', $Commit, $Descendant)
    if ($ancestorResult.ExitCode -ne 0) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label commit is not reachable from its required descendant."
    }
}

function Assert-PartisanFocusedHistoricalProvenance {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)]$Aggregate,
        [Parameter(Mandatory = $true)][string]$CurrentHead
    )

    $candidateHead = [string]$Aggregate.candidate.candidateSourceHead
    $runHead = [string]$Aggregate.integrity.focusedRunHarness.gitHead
    $aggregationHead = [string]$Aggregate.integrity.aggregationGitHead
    Assert-PartisanFocusedReachableCommit `
        -RepositoryRootPath $RepositoryRootPath `
        -Commit $candidateHead `
        -Descendant $runHead `
        -Code 'historical_blob_replacement' `
        -Label 'Historical candidate source'
    Assert-PartisanFocusedReachableCommit `
        -RepositoryRootPath $RepositoryRootPath `
        -Commit $runHead `
        -Descendant $aggregationHead `
        -Code 'historical_blob_replacement' `
        -Label 'Historical focused run harness'
    Assert-PartisanFocusedReachableCommit `
        -RepositoryRootPath $RepositoryRootPath `
        -Commit $aggregationHead `
        -Descendant $CurrentHead `
        -Code 'historical_blob_replacement' `
        -Label 'Historical aggregation head'

    $producerPath = 'tools/New-PartisanFocusedAutotestAggregate.ps1'
    $consumerPath = 'tools/update-release-docs.ps1'
    if ([string]$Aggregate.integrity.aggregateProducer.repositoryPath -cne
            $producerPath -or
        [string]$Aggregate.integrity.releaseDocsConsumer.repositoryPath -cne
            $consumerPath) {
        Throw-PartisanFocusedAggregate `
            -Code 'historical_blob_replacement' `
            -Message 'Historical aggregate tool paths are not canonical.'
    }
    $checks = @(
        @(
            $runHead,
            'tools/run-guarded-focused-autotest.ps1',
            [string]$Aggregate.integrity.focusedRunHarness.focusedRunnerGitBlobSha256),
        @(
            $runHead,
            'tools/Partisan.ReleaseCandidate.psm1',
            [string]$Aggregate.integrity.focusedRunHarness.candidateModuleGitBlobSha256),
        @(
            $aggregationHead,
            $producerPath,
            [string]$Aggregate.integrity.aggregateProducer.gitBlobSha256),
        @(
            $aggregationHead,
            $consumerPath,
            [string]$Aggregate.integrity.releaseDocsConsumer.gitBlobSha256)
    )
    foreach ($check in $checks) {
        try {
            $actual = Get-PartisanFocusedGitBlobSha256 `
                -RepositoryRootPath $RepositoryRootPath `
                -GitHead ([string]$check[0]) `
                -RepositoryPath ([string]$check[1])
        }
        catch {
            Throw-PartisanFocusedAggregate `
                -Code 'historical_blob_replacement' `
                -Message 'A historical aggregate Git blob could not be reopened.'
        }
        if ($actual -cne [string]$check[2]) {
            Throw-PartisanFocusedAggregate `
                -Code 'historical_blob_replacement' `
                -Message 'Historical aggregate provenance differs from its Git blob.'
        }
    }
}

function Assert-PartisanFocusedTrackedCandidate {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)]$Manifest,
        [Parameter(Mandatory = $true)]$Ready,
        [Parameter(Mandatory = $true)][byte[]]$ManifestBytes,
        [Parameter(Mandatory = $true)][byte[]]$ReadyBytes,
        [Parameter(Mandatory = $true)]$PublicIdentity
    )

    $repository = Resolve-PartisanFocusedExistingDirectory `
        -Path $RepositoryRootPath `
        -Label 'Aggregate repository root'
    $aggregationHead = Get-PartisanFocusedGitHead `
        -RepositoryRootPath $repository
    $statusFull = Resolve-PartisanFocusedExistingFile `
        -Path (Join-Path $repository $script:FocusedReleaseStatusPath) `
        -Label 'Tracked release status' `
        -Code 'candidate_status_drift'
    Assert-PartisanFocusedNoReparseAncestry `
        -Root $repository `
        -Path $statusFull `
        -Label 'Tracked release status'
    $statusInput = Read-PartisanFocusedJson `
        -Path $statusFull `
        -Label 'Tracked release status' `
        -Code 'candidate_status_drift'
    $statusGitBytes = Get-PartisanFocusedGitBlobBytes `
        -RepositoryRootPath $repository `
        -GitHead $aggregationHead `
        -RepositoryPath $script:FocusedReleaseStatusPath
    if (-not (Test-PartisanFocusedBytesEqual `
            -Expected $statusGitBytes `
            -Actual $statusInput.Bytes)) {
        Throw-PartisanFocusedAggregate `
            -Code 'candidate_status_drift' `
            -Message 'The active release status differs from its HEAD blob.'
    }

    $status = $statusInput.Value
    Assert-PartisanFocusedProperties `
        -Value $status `
        -Names @(
            'schemaVersion',
            'statusAsOfUtc',
            'releaseStage',
            'releaseDecision',
            'auditedRevision',
            'baseline',
            'artifact',
            'evidence',
            'historicalCandidateEvidence',
            'proofRungs',
            'activeBlockers') `
        -Label 'Tracked release status' `
        -Exact `
        -Code 'candidate_status_drift'
    $artifact = $status.artifact
    Assert-PartisanFocusedProperties `
        -Value $artifact `
        -Names @(
            'releaseCandidateBuilt',
            'runtimeUseDisposition',
            'candidateId',
            'candidateSourceHead',
            'manifestPath',
            'manifestSha256',
            'readySha256',
            'packageHashAlgorithm',
            'packageSha256',
            'packageVersion',
            'addonGuid',
            'addonRevision',
            'workbenchVersion',
            'workbenchSha256',
            'workbenchCrc',
            'serverVersion',
            'clientVersion',
            'note') `
        -Label 'Tracked active candidate' `
        -Exact `
        -Code 'candidate_status_drift'
    if ((Require-PartisanFocusedInteger `
            -Value $status.schemaVersion `
            -Label 'Tracked release-status schema' `
            -Code 'candidate_status_drift') -ne 3 -or
        -not (Require-PartisanFocusedBoolean `
            -Value $artifact.releaseCandidateBuilt `
            -Label 'Tracked candidate-built flag' `
            -Code 'candidate_status_drift')) {
        Throw-PartisanFocusedAggregate `
            -Code 'candidate_status_drift' `
            -Message 'The tracked release status does not expose an active candidate.'
    }

    $candidateId = [string]$PublicIdentity.candidateId
    $expectedManifestPath = 'docs/evidence/release-candidates/' +
        $candidateId + '/candidate.json'
    $manifestPath = Assert-PartisanFocusedPortablePath `
        -Path ([string]$artifact.manifestPath) `
        -Label 'Tracked candidate manifest path' `
        -Code 'candidate_status_drift'
    if ($manifestPath -cne $expectedManifestPath) {
        Throw-PartisanFocusedAggregate `
            -Code 'candidate_status_drift' `
            -Message 'The tracked candidate manifest path is not canonical.'
    }
    $readyPath = $manifestPath.Substring(
        0,
        $manifestPath.Length - 'candidate.json'.Length) + 'candidate.ready.json'
    $trackedManifestFull = Resolve-PartisanFocusedExistingFile `
        -Path (Join-Path $repository $manifestPath.Replace('/', '\')) `
        -Label 'Tracked candidate manifest' `
        -Code 'candidate_status_drift'
    $trackedReadyFull = Resolve-PartisanFocusedExistingFile `
        -Path (Join-Path $repository $readyPath.Replace('/', '\')) `
        -Label 'Tracked candidate ready seal' `
        -Code 'candidate_status_drift'
    foreach ($trackedPath in @($trackedManifestFull, $trackedReadyFull)) {
        Assert-PartisanFocusedNoReparseAncestry `
            -Root $repository `
            -Path $trackedPath `
            -Label 'Tracked candidate identity blob'
    }
    $trackedManifestBytes = [IO.File]::ReadAllBytes($trackedManifestFull)
    $trackedReadyBytes = [IO.File]::ReadAllBytes($trackedReadyFull)
    $manifestGitBytes = Get-PartisanFocusedGitBlobBytes `
        -RepositoryRootPath $repository `
        -GitHead $aggregationHead `
        -RepositoryPath $manifestPath
    $readyGitBytes = Get-PartisanFocusedGitBlobBytes `
        -RepositoryRootPath $repository `
        -GitHead $aggregationHead `
        -RepositoryPath $readyPath
    if (-not (Test-PartisanFocusedBytesEqual $manifestGitBytes $trackedManifestBytes) -or
        -not (Test-PartisanFocusedBytesEqual $readyGitBytes $trackedReadyBytes) -or
        -not (Test-PartisanFocusedBytesEqual $manifestGitBytes $ManifestBytes) -or
        -not (Test-PartisanFocusedBytesEqual $readyGitBytes $ReadyBytes)) {
        Throw-PartisanFocusedAggregate `
            -Code 'candidate_status_drift' `
            -Message 'The focused run candidate identity differs from tracked HEAD.'
    }

    $statusAsOf = Require-PartisanFocusedUtc `
        -Value $status.statusAsOfUtc `
        -Label 'Tracked release-status UTC' `
        -Code 'candidate_status_drift'
    $createdUtc = Require-PartisanFocusedUtc `
        -Value $Manifest.createdUtc `
        -Label 'Tracked candidate creation UTC' `
        -Code 'candidate_status_drift'
    if ($statusAsOf -lt $createdUtc -or
        [string]$artifact.runtimeUseDisposition -cne
            $script:FocusedRunDisposition -or
        [string]$artifact.runtimeUseDisposition -cne
            [string]$PublicIdentity.runtimeUseDisposition -or
        [string]$artifact.candidateId -cne $candidateId -or
        [string]$artifact.candidateSourceHead -cne
            [string]$PublicIdentity.gitHead -or
        [string]$artifact.manifestSha256 -cne
            [string]$PublicIdentity.manifestSha256 -or
        [string]$artifact.readySha256 -cne
            [string]$PublicIdentity.readySha256 -or
        [string]$artifact.packageHashAlgorithm -cne
            [string]$PublicIdentity.packageHashAlgorithm -or
        [string]$artifact.packageSha256 -cne
            [string]$PublicIdentity.packageSha256 -or
        [string]$artifact.packageVersion -cne
            [string]$PublicIdentity.candidateVersion -or
        [string]$artifact.addonGuid -cne [string]$PublicIdentity.addonGuid -or
        [string]$artifact.addonRevision -cne [string]$Manifest.addon.revision -or
        [string]$artifact.workbenchVersion -cne
            [string]$Manifest.toolchain.workbench.productVersion -or
        [string]$artifact.workbenchSha256 -cne
            [string]$Manifest.toolchain.workbench.sha256 -or
        [string]$artifact.workbenchCrc -cne
            [string]$PublicIdentity.workbenchCrc -or
        [string]$artifact.serverVersion -cne
            [string]$Manifest.toolchain.server.productVersion -or
        [string]$artifact.clientVersion -cne
            [string]$Manifest.toolchain.client.productVersion) {
        Throw-PartisanFocusedAggregate `
            -Code 'candidate_status_drift' `
            -Message 'The focused candidate differs from the tracked active candidate.'
    }
    Assert-PartisanFocusedReachableCommit `
        -RepositoryRootPath $repository `
        -Commit ([string]$PublicIdentity.gitHead) `
        -Descendant $aggregationHead
}

function Get-PartisanFocusedCandidateBinding {
    param(
        [Parameter(Mandatory = $true)]$Run,
        [Parameter(Mandatory = $true)][object[]]$FileRows,
        [Parameter(Mandatory = $true)][string]$StartedUtcText,
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath
    )

    $candidateRow = @($FileRows | Where-Object {
        $_.path -ceq 'identity/candidate.json'
    })
    $readyRow = @($FileRows | Where-Object {
        $_.path -ceq 'identity/candidate.ready.json'
    })
    if ($candidateRow.Count -ne 1 -or $readyRow.Count -ne 1) {
        Throw-PartisanFocusedAggregate `
            -Code 'candidate_tampering' `
            -Message 'The candidate identity blob set is incomplete.'
    }

    $manifestInput = Read-PartisanFocusedJson `
        -Path $candidateRow[0].fullPath `
        -Label 'Focused candidate manifest' `
        -Code 'candidate_tampering'
    $readyInput = Read-PartisanFocusedJson `
        -Path $readyRow[0].fullPath `
        -Label 'Focused candidate ready seal' `
        -Code 'candidate_tampering'
    $manifest = $manifestInput.Value
    $ready = $readyInput.Value
    if ($manifestInput.Text -match
        '(?i)(?:[A-Z]:[\\/]|\\\\|file://|/(?:Users|home|mnt)/)') {
        Throw-PartisanFocusedAggregate `
            -Code 'candidate_tampering' `
            -Message 'The focused candidate manifest is not portable.'
    }

    Assert-PartisanFocusedProperties `
        -Value $manifest `
        -Names @(
            'manifestSchemaVersion',
            'createdUtc',
            'candidate',
            'source',
            'addon',
            'toolchain',
            'workbench',
            'package',
            'evidence') `
        -Label 'Focused candidate manifest' `
        -Exact `
        -Code 'candidate_tampering'
    Assert-PartisanFocusedProperties `
        -Value $manifest.candidate `
        -Names @('id', 'version', 'state') `
        -Label 'Focused candidate manifest identity' `
        -Exact `
        -Code 'candidate_tampering'
    Assert-PartisanFocusedProperties `
        -Value $manifest.source `
        -Names @(
            'gitHead',
            'dirty',
            'auditedGameplayRevision',
            'auditedGameplayRelation',
            'auditedGameplayDistance',
            'embeddedImplementation',
            'campaignSchema',
            'runtimeSettingsSchema') `
        -Label 'Focused candidate source' `
        -Exact `
        -Code 'candidate_tampering'
    Assert-PartisanFocusedProperties `
        -Value $manifest.source.embeddedImplementation `
        -Names @('sha', 'utc', 'label', 'relation', 'distance') `
        -Label 'Focused embedded implementation' `
        -Exact `
        -Code 'candidate_tampering'
    Assert-PartisanFocusedProperties `
        -Value $manifest.addon `
        -Names @('id', 'guid', 'title', 'revision', 'version', 'dependencies') `
        -Label 'Focused candidate addon' `
        -Exact `
        -Code 'candidate_tampering'
    Assert-PartisanFocusedProperties `
        -Value $manifest.toolchain `
        -Names @(
            'workbench',
            'server',
            'serverDiagnostic',
            'client',
            'clientDiagnostic') `
        -Label 'Focused candidate toolchain' `
        -Exact `
        -Code 'candidate_tampering'
    Assert-PartisanFocusedProperties `
        -Value $manifest.workbench `
        -Names @('crc', 'targets') `
        -Label 'Focused candidate Workbench identity' `
        -Exact `
        -Code 'candidate_tampering'
    Assert-PartisanFocusedProperties `
        -Value $manifest.package `
        -Names @('root', 'hashAlgorithm', 'sha256', 'canonicalIndexPath', 'files') `
        -Label 'Focused candidate package' `
        -Exact `
        -Code 'candidate_tampering'
    Assert-PartisanFocusedProperties `
        -Value $manifest.evidence `
        -Names @('root', 'files') `
        -Label 'Focused candidate evidence' `
        -Exact `
        -Code 'candidate_tampering'
    Assert-PartisanFocusedProperties `
        -Value $ready `
        -Names @(
            'schemaVersion',
            'candidateId',
            'gitHead',
            'packageSha256',
            'manifestSha256') `
        -Label 'Focused candidate ready seal' `
        -Exact `
        -Code 'candidate_tampering'
    foreach ($arrayContract in @(
            [pscustomobject]@{
                Value = $manifest.addon.dependencies
                Label = 'Focused candidate dependencies'
            },
            [pscustomobject]@{
                Value = $manifest.workbench.targets
                Label = 'Focused Workbench targets'
            },
            [pscustomobject]@{
                Value = $manifest.package.files
                Label = 'Focused candidate package files'
            },
            [pscustomobject]@{
                Value = $manifest.evidence.files
                Label = 'Focused candidate evidence files'
            })) {
        Assert-PartisanFocusedArray `
            -Value $arrayContract.Value `
            -Label $arrayContract.Label `
            -Code 'candidate_tampering'
    }

    foreach ($toolName in @(
            'workbench',
            'server',
            'serverDiagnostic',
            'client',
            'clientDiagnostic')) {
        Assert-PartisanFocusedExecutableIdentity `
            -Value $manifest.toolchain.$toolName `
            -Label "Focused candidate $toolName executable"
    }
    $expectedWorkbenchTargets = @('PC', 'PS4', 'PS5', 'XBOX_ONE', 'XBOX_SERIES')
    $actualWorkbenchTargets = New-Object Collections.Generic.List[string]
    foreach ($targetRow in @($manifest.workbench.targets)) {
        Assert-PartisanFocusedProperties `
            -Value $targetRow `
            -Names @('target', 'status', 'files', 'classes', 'crc', 'evidencePath') `
            -Label 'Focused candidate Workbench target' `
            -Exact `
            -Code 'candidate_tampering'
        $target = Require-PartisanFocusedText `
            -Value $targetRow.target `
            -Label 'Focused candidate Workbench target ID' `
            -Code 'candidate_tampering'
        [void]$actualWorkbenchTargets.Add($target)
        if ([string]$targetRow.status -cne 'passed' -or
            (Require-PartisanFocusedInteger `
                -Value $targetRow.files `
                -Label 'Focused candidate Workbench target file count' `
                -Code 'candidate_tampering') -le 0 -or
            (Require-PartisanFocusedInteger `
                -Value $targetRow.classes `
                -Label 'Focused candidate Workbench target class count' `
                -Code 'candidate_tampering') -le 0 -or
            [string]$targetRow.crc -cne [string]$manifest.workbench.crc) {
            Throw-PartisanFocusedAggregate `
                -Code 'candidate_tampering' `
                -Message 'A focused candidate Workbench target is invalid.'
        }
        Assert-PartisanFocusedPortablePath `
            -Path ([string]$targetRow.evidencePath) `
            -Label 'Focused candidate Workbench evidence path' `
            -Code 'candidate_tampering' | Out-Null
    }
    if ((@($actualWorkbenchTargets) -join '|') -cne
        ($expectedWorkbenchTargets -join '|')) {
        Throw-PartisanFocusedAggregate `
            -Code 'candidate_tampering' `
            -Message 'The focused candidate Workbench target set is not canonical.'
    }
    $packageSha = Get-PartisanFocusedCanonicalPackageSha256 `
        -Package $manifest.package `
        -Label 'Focused candidate package' `
        -Code 'candidate_tampering'
    foreach ($evidenceRow in @($manifest.evidence.files)) {
        Assert-PartisanFocusedProperties `
            -Value $evidenceRow `
            -Names @('path', 'length', 'sha256') `
            -Label 'Focused candidate evidence row' `
            -Exact `
            -Code 'candidate_tampering'
        Assert-PartisanFocusedPortablePath `
            -Path ([string]$evidenceRow.path) `
            -Label 'Focused candidate evidence path' `
            -Code 'candidate_tampering' | Out-Null
        if ((Require-PartisanFocusedInteger `
                -Value $evidenceRow.length `
                -Label 'Focused candidate evidence length' `
                -Code 'candidate_tampering') -lt 0) {
            Throw-PartisanFocusedAggregate `
                -Code 'candidate_tampering' `
                -Message 'A focused candidate evidence length is negative.'
        }
        Require-PartisanFocusedSha256 `
            -Value $evidenceRow.sha256 `
            -Label 'Focused candidate evidence SHA-256' | Out-Null
    }

    $manifestSchema = Require-PartisanFocusedInteger `
        -Value $manifest.manifestSchemaVersion `
        -Label 'Focused candidate manifest schema' `
        -Code 'schema_drift'
    $readySchema = Require-PartisanFocusedInteger `
        -Value $ready.schemaVersion `
        -Label 'Focused candidate ready schema' `
        -Code 'schema_drift'
    if ($manifestSchema -ne $script:CandidateManifestSchema -or
        $readySchema -ne $script:CandidateReadySchema) {
        Throw-PartisanFocusedAggregate `
            -Code 'schema_drift' `
            -Message 'The focused candidate schema is unsupported.'
    }

    $manifestSha = [string]$manifestInput.Sha256
    $readySha = [string]$readyInput.Sha256
    Require-PartisanFocusedSha256 `
        -Value $Run.candidate.manifestSha256 `
        -Label 'Focused run candidate manifest SHA-256' | Out-Null
    Require-PartisanFocusedSha256 `
        -Value $Run.candidate.readySha256 `
        -Label 'Focused run candidate ready SHA-256' | Out-Null
    if ([long]$candidateRow[0].length -ne [long]$manifestInput.Length -or
        [long]$readyRow[0].length -ne [long]$readyInput.Length -or
        [string]$candidateRow[0].sha256 -cne $manifestSha -or
        [string]$readyRow[0].sha256 -cne $readySha -or
        [string]$Run.candidate.manifestSha256 -cne $manifestSha -or
        [string]$Run.candidate.readySha256 -cne $readySha -or
        [string]$ready.manifestSha256 -cne $manifestSha) {
        Throw-PartisanFocusedAggregate `
            -Code 'candidate_tampering' `
            -Message 'The focused candidate identity hashes do not bind one seal.'
    }

    $candidateId = Require-PartisanFocusedText `
        -Value $manifest.candidate.id `
        -Label 'Focused candidate ID' `
        -Code 'candidate_tampering'
    $candidateVersion = Require-PartisanFocusedText `
        -Value $manifest.candidate.version `
        -Label 'Focused candidate version' `
        -Code 'candidate_tampering'
    $sourceHead = Require-PartisanFocusedText `
        -Value $manifest.source.gitHead `
        -Label 'Focused candidate source Git HEAD' `
        -Code 'candidate_tampering'
    $embeddedSha = Require-PartisanFocusedText `
        -Value $manifest.source.embeddedImplementation.sha `
        -Label 'Focused embedded implementation SHA' `
        -Code 'candidate_tampering'
    $embeddedUtcText = Require-PartisanFocusedText `
        -Value $manifest.source.embeddedImplementation.utc `
        -Label 'Focused embedded implementation UTC' `
        -Code 'candidate_tampering'
    $embeddedLabel = Require-PartisanFocusedText `
        -Value $manifest.source.embeddedImplementation.label `
        -Label 'Focused embedded implementation label' `
        -Code 'candidate_tampering'
    $campaignSchema = Require-PartisanFocusedInteger `
        -Value $manifest.source.campaignSchema `
        -Label 'Focused campaign schema' `
        -Code 'candidate_tampering'
    $settingsSchema = Require-PartisanFocusedInteger `
        -Value $manifest.source.runtimeSettingsSchema `
        -Label 'Focused runtime settings schema' `
        -Code 'candidate_tampering'
    $auditedRevision = Require-PartisanFocusedText `
        -Value $manifest.source.auditedGameplayRevision `
        -Label 'Focused audited gameplay revision' `
        -Code 'candidate_tampering'
    $auditedDistance = Require-PartisanFocusedInteger `
        -Value $manifest.source.auditedGameplayDistance `
        -Label 'Focused audited gameplay distance' `
        -Code 'candidate_tampering'
    $embeddedDistance = Require-PartisanFocusedInteger `
        -Value $manifest.source.embeddedImplementation.distance `
        -Label 'Focused embedded implementation distance' `
        -Code 'candidate_tampering'
    if (-not (Test-PartisanFocusedCandidateId -Value $candidateId) -or
        $candidateVersion -cnotmatch '^[0-9A-Za-z][0-9A-Za-z._-]{0,95}$' -or
        [string]$manifest.candidate.state -cne $script:FocusedCandidateState -or
        (Require-PartisanFocusedBoolean `
            -Value $manifest.source.dirty `
            -Label 'Focused candidate source dirty flag' `
            -Code 'candidate_tampering') -or
        $sourceHead -cnotmatch '^[0-9a-f]{40}$' -or
        $embeddedSha -cnotmatch '^[0-9a-f]{40}$' -or
        $auditedRevision -cnotmatch '^[0-9a-f]{40}$' -or
        [string]$manifest.source.auditedGameplayRelation -cnotin
            @('equal', 'ancestor') -or
        [string]$manifest.source.embeddedImplementation.relation -cnotin
            @('equal', 'ancestor') -or
        $auditedDistance -lt 0 -or
        $embeddedDistance -lt 0 -or
        ([string]$manifest.source.auditedGameplayRelation -ceq 'equal' -and
            $auditedDistance -ne 0) -or
        ([string]$manifest.source.auditedGameplayRelation -ceq 'ancestor' -and
            $auditedDistance -le 0) -or
        ([string]$manifest.source.embeddedImplementation.relation -ceq 'equal' -and
            $embeddedDistance -ne 0) -or
        ([string]$manifest.source.embeddedImplementation.relation -ceq 'ancestor' -and
            $embeddedDistance -le 0) -or
        $campaignSchema -ne $script:FocusedCampaignSchema -or
        $settingsSchema -ne $script:FocusedRuntimeSettingsSchema -or
        [string]$manifest.addon.id -cnotmatch '^[A-Za-z_][A-Za-z0-9_]*$' -or
        [string]$manifest.addon.guid -cnotmatch '^[0-9A-F]{16}$' -or
        [string]$manifest.addon.revision -cne 'unpublished-local-pack' -or
        [string]$manifest.addon.version -cne $candidateVersion -or
        @($manifest.addon.dependencies).Count -lt 1 -or
        [string]$manifest.workbench.crc -cnotmatch '^[0-9a-f]{8}$' -or
        [string]$manifest.evidence.root -cne 'evidence' -or
        @($manifest.evidence.files).Count -lt 1) {
        Throw-PartisanFocusedAggregate `
            -Code 'candidate_tampering' `
            -Message 'The focused candidate manifest contract is invalid.'
    }

    $createdUtc = Require-PartisanFocusedUtc `
        -Value $manifest.createdUtc `
        -Label 'Focused candidate creation UTC' `
        -Code 'candidate_tampering'
    $embeddedUtc = Require-PartisanFocusedUtc `
        -Value $embeddedUtcText `
        -Label 'Focused embedded implementation UTC' `
        -Code 'candidate_tampering'
    $startedUtc = Require-PartisanFocusedUtc `
        -Value $StartedUtcText `
        -Label 'Focused run start UTC'
    if ($embeddedUtc -gt $createdUtc -or $createdUtc -gt $startedUtc) {
        Throw-PartisanFocusedAggregate `
            -Code 'candidate_tampering' `
            -Message 'The focused candidate chronology is invalid.'
    }

    Assert-PartisanFocusedExecutableIdentity `
        -Value $manifest.toolchain.client `
        -Label 'Focused candidate client executable'
    Assert-PartisanFocusedExecutableIdentity `
        -Value $manifest.toolchain.clientDiagnostic `
        -Label 'Focused candidate diagnostic client executable'

    $publicIdentity = [pscustomobject][ordered]@{
        candidateId = $candidateId
        candidateVersion = $candidateVersion
        runtimeUseDisposition = $script:FocusedRunDisposition
        gitHead = $sourceHead
        embeddedBuildSha = $embeddedSha
        embeddedBuildUtc = $embeddedUtcText
        embeddedBuildLabel = $embeddedLabel
        campaignSchema = $campaignSchema
        runtimeSettingsSchema = $settingsSchema
        addonId = [string]$manifest.addon.id
        addonGuid = [string]$manifest.addon.guid
        packageHashAlgorithm = [string]$manifest.package.hashAlgorithm
        packageSha256 = $packageSha
        manifestSha256 = $manifestSha
        readySha256 = $readySha
        workbenchCrc = [string]$manifest.workbench.crc
        runtimeRole = 'client'
        diagnosticExecutable = $manifest.toolchain.clientDiagnostic
        recordedDiagnosticExecutable = $manifest.toolchain.clientDiagnostic
        recordedRuntimeExecutable = $manifest.toolchain.client
    }
    Assert-PartisanFocusedSameValue `
        -Expected $publicIdentity `
        -Actual $Run.candidate `
        -Label 'Focused run candidate identity'
    Assert-PartisanFocusedSameValue `
        -Expected $publicIdentity `
        -Actual $Run.outcome.result.Candidate `
        -Label 'Focused result candidate identity'

    if ([string]$ready.candidateId -cne $candidateId -or
        [string]$ready.gitHead -cne $sourceHead -or
        [string]$ready.packageSha256 -cne $packageSha -or
        [string]$Run.launch.addonGuid -cne [string]$manifest.addon.guid -or
        [string]$Run.launch.packageSha256 -cne $packageSha) {
        Throw-PartisanFocusedAggregate `
            -Code 'candidate_tampering' `
            -Message 'The focused run differs from its candidate ready seal.'
    }
    Assert-PartisanFocusedSameValue `
        -Expected $manifest.toolchain.clientDiagnostic `
        -Actual $Run.launch.diagnosticExecutable `
        -Label 'Focused launch diagnostic executable' `
        -Code 'tool_identity_drift'
    Assert-PartisanFocusedSameValue `
        -Expected $manifest.toolchain.client `
        -Actual $Run.launch.recordedRuntimeExecutable `
        -Label 'Focused launch runtime executable' `
        -Code 'tool_identity_drift'
    Assert-PartisanFocusedTrackedCandidate `
        -RepositoryRootPath $RepositoryRootPath `
        -Manifest $manifest `
        -Ready $ready `
        -ManifestBytes $manifestInput.Bytes `
        -ReadyBytes $readyInput.Bytes `
        -PublicIdentity $publicIdentity

    return [pscustomobject][ordered]@{
        PublicIdentity = $publicIdentity
        SummaryIdentity = [pscustomobject][ordered]@{
            candidateId = $candidateId
            candidateSourceHead = $sourceHead
            packageSha256 = $packageSha
            manifestSha256 = $manifestSha
            readySha256 = $readySha
            workbenchCrc = [string]$manifest.workbench.crc
        }
        ManifestSha256 = $manifestSha
        ReadySha256 = $readySha
        Toolchain = [pscustomobject][ordered]@{
            client = $manifest.toolchain.client
            clientDiagnostic = $manifest.toolchain.clientDiagnostic
        }
    }
}

function Get-PartisanFocusedJUnitEvidence {
    param(
        [Parameter(Mandatory = $true)]$JUnitInput,
        [Parameter(Mandatory = $true)][string]$ExpectedProfile,
        [Parameter(Mandatory = $true)][string]$ExpectedSuite
    )

    if ($null -eq $JUnitInput -or $JUnitInput -is [Array]) {
        Throw-PartisanFocusedAggregate `
            -Code 'raw_junit_tampering' `
            -Message 'The focused JUnit snapshot is not one scalar input.'
    }
    Assert-PartisanFocusedProperties `
        -Value $JUnitInput `
        -Names @('Text', 'Bytes', 'Length', 'Sha256') `
        -Label 'Focused JUnit snapshot' `
        -Exact `
        -Code 'raw_junit_tampering'
    try {
        [xml]$document = [string]$JUnitInput.Text
    }
    catch {
        Throw-PartisanFocusedAggregate `
            -Code 'raw_junit_tampering' `
            -Message 'The focused JUnit blob is not valid XML.'
    }
    if ($null -eq $document.testsuites) {
        Throw-PartisanFocusedAggregate `
            -Code 'raw_junit_tampering' `
            -Message 'The focused JUnit root is unavailable.'
    }
    $suites = @($document.testsuites.testsuite)
    $cases = @($suites | ForEach-Object { @($_.testcase) })
    $expectedCases = @($script:FocusedSuiteTestCases[$ExpectedProfile])
    if ($expectedCases.Count -eq 0) {
        Throw-PartisanFocusedAggregate `
            -Code 'raw_junit_tampering' `
            -Message 'The focused suite lacks an exact testcase manifest.'
    }
    $actualCaseNames = @($cases | ForEach-Object { [string]$_.name })
    $actualCaseSet = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $caseManifestExact = $actualCaseNames.Count -eq $expectedCases.Count
    foreach ($caseName in $actualCaseNames) {
        if (-not $actualCaseSet.Add([string]$caseName)) {
            $caseManifestExact = $false
        }
    }
    foreach ($expectedCase in $expectedCases) {
        if (-not $actualCaseSet.Contains([string]$expectedCase)) {
            $caseManifestExact = $false
        }
    }
    $tests = 0
    $failures = 0
    $errors = 0
    $skipped = 0
    foreach ($suite in $suites) {
        $tests += [int]$suite.tests
        $failures += [int]$suite.failures
        $errors += [int]$suite.errors
        $skipped += [int]$suite.skipped
    }
    if ($suites.Count -ne 1 -or
        $cases.Count -ne $expectedCases.Count -or
        $tests -ne $expectedCases.Count -or
        $failures -ne 0 -or
        $errors -ne 0 -or
        $skipped -ne 0 -or
        [string]$suites[0].name -cne $ExpectedSuite -or
        -not $caseManifestExact -or
        @($cases | Where-Object {
            [string]$_.classname -cne $ExpectedSuite -or
            @($_.SelectNodes('failure')).Count -ne 0 -or
            @($_.SelectNodes('error')).Count -ne 0 -or
            @($_.SelectNodes('skipped')).Count -ne 0
        }).Count -ne 0) {
        Throw-PartisanFocusedAggregate `
            -Code 'raw_junit_tampering' `
            -Message 'The focused JUnit blob does not contain its exact passing suite manifest.'
    }
    return [pscustomobject][ordered]@{
        Tests = $expectedCases.Count
        Failures = 0
        Errors = 0
        Skipped = 0
        TestCase = $ExpectedProfile
        Suite = $ExpectedSuite
    }
}

function Get-PartisanFocusedRequiredPatternContract {
    param([Parameter(Mandatory = $true)][string]$ConsoleText)

    $patterns = New-Object Collections.Generic.List[string]
    $contentLines = New-Object Collections.Generic.List[string]
    $seen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $utf8 = New-Object Text.UTF8Encoding($false, $true)
    foreach ($lineValue in @($ConsoleText -split "`r?`n")) {
        $line = [string]$lineValue
        if (-not $line.StartsWith(
                $script:FocusedRequiredPatternMarker,
                [StringComparison]::Ordinal)) {
            [void]$contentLines.Add($line)
            continue
        }
        $encoded = $line.Substring($script:FocusedRequiredPatternMarker.Length)
        try {
            $bytes = [Convert]::FromBase64String($encoded)
            $pattern = $utf8.GetString($bytes)
        }
        catch {
            Throw-PartisanFocusedAggregate `
                -Code 'raw_result_tampering' `
                -Message 'The retained required-pattern contract is not canonical UTF-8 Base64.'
        }
        if ([Convert]::ToBase64String($bytes) -cne $encoded -or
            [string]::IsNullOrWhiteSpace($pattern) -or
            $pattern.Length -gt 4096 -or
            $pattern -match '[\x00\r\n]' -or
            -not $seen.Add($pattern)) {
            Throw-PartisanFocusedAggregate `
                -Code 'raw_result_tampering' `
                -Message 'The retained required-pattern contract is empty, duplicated, or noncanonical.'
        }
        [void]$patterns.Add($pattern)
    }
    if ($patterns.Count -lt 1 -or $patterns.Count -gt 64) {
        Throw-PartisanFocusedAggregate `
            -Code 'raw_result_tampering' `
            -Message 'The retained required-pattern contract must contain between one and 64 patterns.'
    }
    $contentText = $contentLines.ToArray() -join "`n"
    foreach ($pattern in $patterns) {
        if ($contentText.IndexOf(
                $pattern,
                [StringComparison]::Ordinal) -lt 0) {
            Throw-PartisanFocusedAggregate `
                -Code 'raw_result_tampering' `
                -Message 'Retained console evidence does not satisfy its required-pattern contract.'
        }
    }
    return [pscustomobject][ordered]@{
        PatternCount = $patterns.Count
        Patterns = $patterns.ToArray()
        EvidenceConsoleText = $contentText
    }
}

function Get-PartisanFocusedRawMountAttestation {
    param(
        [Parameter(Mandatory = $true)][string]$ConsoleText,
        [Parameter(Mandatory = $true)][string]$ExpectedAddonGuid,
        [Parameter(Mandatory = $true)]
        [ValidatePattern('^[0-9a-f]{32}$')]
        [string]$ExpectedRunNonce
    )

    $pattern = "(?im)^\s*\d{2}:\d{2}:\d{2}\.\d{3}\s+ENGINE\s+:\s+" +
        "gproj:\s+'(?<path>[^']+)'\s+guid:\s+'(?-i:" +
        [regex]::Escape($ExpectedAddonGuid) +
        ")'\s*(?<mode>\([^)]+\))?\s*$"
    $expectedProjectSuffix = 'PartisanFocusedAutotest/' +
        $ExpectedRunNonce + '/' + $script:FocusedMountProjectSuffix
    $recordCount = 0
    $exactPathCount = 0
    $packedCount = 0
    $invalidModeCount = 0
    foreach ($match in @([regex]::Matches($ConsoleText, $pattern))) {
        $recordCount++
        $recordedProject = $match.Groups['path'].Value.Replace('\', '/')
        if ($recordedProject.Equals(
                $expectedProjectSuffix,
                [StringComparison]::OrdinalIgnoreCase) -or
            $recordedProject.EndsWith(
                '/' + $expectedProjectSuffix,
                [StringComparison]::OrdinalIgnoreCase)) {
            $exactPathCount++
        }
        if ($match.Groups['mode'].Value -ceq '(packed)') {
            $packedCount++
        }
        elseif (-not [string]::IsNullOrEmpty($match.Groups['mode'].Value)) {
            $invalidModeCount++
        }
    }
    $guidExact = $recordCount -gt 0
    $packed = $packedCount -eq 1 -and $invalidModeCount -eq 0
    return [pscustomobject][ordered]@{
        Valid = $recordCount -eq 2 -and
            $exactPathCount -eq 2 -and
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

function Get-PartisanFocusedRawDiagnosticCensus {
    param(
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)][string]$ConsoleText,
        [Parameter(Mandatory = $true)][string]$ExpectedProfile,
        [Parameter(Mandatory = $true)][string]$ExpectedSuite
    )

    $lines = @($ConsoleText -split "`r?`n")
    $expectedCases = @($script:FocusedSuiteTestCases[$ExpectedProfile])
    $expectedCaseSet = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($expectedCase in $expectedCases) {
        [void]$expectedCaseSet.Add([string]$expectedCase)
    }
    $successfulCaseSet = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $successfulCaseRows = New-Object Collections.Generic.List[object]
    $profileNonMutatingTokenIndices = New-Object Collections.Generic.List[int]
    $profileExactSeamTokenIndices = New-Object Collections.Generic.List[int]
    $suiteStartedIndex = -1
    $testSuccessIndex = -1
    $runnerFinishedIndex = -1
    $junitSavedIndex = -1
    $failedListSavedIndex = -1
    $profileNonMutatingTokenIndex = -1
    $profileExactSeamTokenIndex = -1
    $profileNonMutatingTokenCount = 0
    $profileExactSeamTokenCount = 0
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
    $allSuiteStartedPattern = $timestampedScriptPrefix +
        'TestSuite #[^\r\n]+ started\s*$'
    $allTestSuccessPattern = $timestampedScriptPrefix +
        '(?:✅\s+)?(?<case>HST_TEST_[A-Za-z0-9_]+): SUCCESS\s*$'
    $runnerFinishedPattern = $timestampedScriptPrefix +
        'SCR_TestRunner has finished running\s*$'
    $junitSavedPattern =
        '^\s*(?:\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+)?' +
        'Autotest JUnit XML saved to:(?:\s+.*)?$'
    $failedListSavedPattern =
        '^\s*(?:\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+)?' +
        'Autotest failed list saved to:(?:\s+.*)?$'
    $profileNonMutatingPattern =
        '^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+' +
        '(?:.* \| )?failed native callback non-mutating 1(?: \| .*)?$'
    $profileExactSeamPattern =
        '^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+' +
        'setup/seam/request/bytes/journal 1/1/1/1/1(?: \| .*)?$'
    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = [string]$lines[$index]
        if ($line -cmatch $allSuiteStartedPattern) {
            $allSuiteStartedCount++
        }
        $successMatch = [regex]::Match($line, $allTestSuccessPattern)
        if ($successMatch.Success) {
            $allTestSuccessCount++
            $successfulCaseName =
                [string]$successMatch.Groups['case'].Value
            if ($expectedCaseSet.Contains($successfulCaseName)) {
                $testSuccessIndex = $index
                $testSuccessCount++
                [void]$successfulCaseSet.Add($successfulCaseName)
                [void]$successfulCaseRows.Add(
                    [pscustomobject][ordered]@{
                        Name = $successfulCaseName
                        Index = $index
                    })
            }
        }
        if ($line -cmatch $suiteStartedPattern) {
            $suiteStartedIndex = $index
            $suiteStartedCount++
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
        if ($line -cmatch $profileNonMutatingPattern) {
            $profileNonMutatingTokenIndex = $index
            $profileNonMutatingTokenCount++
            [void]$profileNonMutatingTokenIndices.Add($index)
        }
        if ($line -cmatch $profileExactSeamPattern) {
            $profileExactSeamTokenIndex = $index
            $profileExactSeamTokenCount++
            [void]$profileExactSeamTokenIndices.Add($index)
        }
    }

    $profileJournalCase = $ExpectedProfile -ceq
        'HST_CampaignProfileJournalAuthorityAutotestSuite'
    $expectedIntentional = if ($profileJournalCase) {
        $expectedCases.Count
    }
    else {
        0
    }
    $hardPattern = '\b(?:SCRIPT|ENGINE)\s+\(E\):'
    $stockFilterPattern =
        "^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+\(E\): " +
        "Can't instantiate class 'SCR_FilterCategory', constructor is not public\s*$"
    $intentionalNativeFailurePattern =
        "^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+\(E\): " +
        "string failureDetail = 'Partisan persistence \| native save " +
        'callback failure \| sequence/type/flags 1/0/0 \| ' +
        'manager/enabled/allowed/busy/active/playthrough 1/1/1/0/0/0 \| ' +
        'types/persistence/state/loaded/tracked/config/staged ' +
        '5/1/2/0/0/0/1 \| replication mode 0 \| snapshot fingerprint ''\s*$'
    $intentionalIndices = New-Object Collections.Generic.List[int]
    for ($index = 0; $index -lt $lines.Count; $index++) {
        if ([string]$lines[$index] -cmatch
            $intentionalNativeFailurePattern) {
            [void]$intentionalIndices.Add($index)
        }
    }
    $approvedIntentionalIndices = New-Object `
        'Collections.Generic.HashSet[int]'
    $profileProofTokensSeen = -not $profileJournalCase -and
        $profileNonMutatingTokenCount -eq 0 -and
        $profileExactSeamTokenCount -eq 0 -and
        $intentionalIndices.Count -eq 0
    if ($profileJournalCase -and
        $successfulCaseRows.Count -eq $expectedCases.Count -and
        $profileNonMutatingTokenCount -eq $expectedCases.Count -and
        $profileExactSeamTokenCount -eq $expectedCases.Count -and
        $intentionalIndices.Count -eq $expectedCases.Count) {
        $profileProofTokensSeen = $true
        $intervalFloor = $suiteStartedIndex
        foreach ($successRow in $successfulCaseRows) {
            $intervalCeiling = [int]$successRow.Index
            $intervalIntentional = @($intentionalIndices | Where-Object {
                $_ -gt $intervalFloor -and $_ -lt $intervalCeiling
            })
            $intervalNonMutating = @(
                $profileNonMutatingTokenIndices | Where-Object {
                    $_ -gt $intervalFloor -and $_ -lt $intervalCeiling
                })
            $intervalExactSeam = @(
                $profileExactSeamTokenIndices | Where-Object {
                    $_ -gt $intervalFloor -and $_ -lt $intervalCeiling
                })
            if ($intervalIntentional.Count -ne 1 -or
                $intervalNonMutating.Count -ne 1 -or
                $intervalExactSeam.Count -ne 1 -or
                $intervalIntentional[0] -ge $intervalNonMutating[0] -or
                $intervalIntentional[0] -ge $intervalExactSeam[0]) {
                $profileProofTokensSeen = $false
                break
            }
            [void]$approvedIntentionalIndices.Add(
                [int]$intervalIntentional[0])
            $intervalFloor = $intervalCeiling
        }
    }
    $approvedStockFilterCount = 0
    $approvedIntentionalFaultCount = 0
    $hardDiagnosticCount = 0
    $unapproved = New-Object Collections.Generic.List[string]
    $completionFloor = [Math]::Max(
        $runnerFinishedIndex,
        [Math]::Max($junitSavedIndex, $failedListSavedIndex))
    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = [string]$lines[$index]
        if ($line -cnotmatch $hardPattern) {
            continue
        }
        $hardDiagnosticCount++
        if ($line -cmatch $stockFilterPattern) {
            if ($completionFloor -ge 0 -and
                $index -gt $completionFloor -and
                $approvedStockFilterCount -lt 2) {
                $approvedStockFilterCount++
            }
            else {
                [void]$unapproved.Add($line)
            }
            continue
        }
        if ($line -cmatch $intentionalNativeFailurePattern) {
            if ($profileJournalCase -and $profileProofTokensSeen -and
                $approvedIntentionalIndices.Contains($index)) {
                $approvedIntentionalFaultCount++
            }
            else {
                [void]$unapproved.Add($line)
            }
            continue
        }
        [void]$unapproved.Add($line)
    }

    $markerOrderExact = $suiteStartedCount -eq 1 -and
        $testSuccessCount -eq $expectedCases.Count -and
        $successfulCaseSet.Count -eq $expectedCaseSet.Count -and
        $allSuiteStartedCount -eq 1 -and
        $allTestSuccessCount -eq $expectedCases.Count -and
        $runnerFinishedCount -eq 1 -and
        $junitSavedCount -eq 1 -and
        $failedListSavedCount -eq 1 -and
        $suiteStartedIndex -ge 0 -and
        $testSuccessIndex -gt $suiteStartedIndex -and
        $runnerFinishedIndex -gt $testSuccessIndex -and
        $junitSavedIndex -gt $runnerFinishedIndex -and
        $failedListSavedIndex -gt $junitSavedIndex
    return [pscustomobject][ordered]@{
        Valid = $markerOrderExact -and
            $approvedStockFilterCount -eq 2 -and
            $approvedIntentionalFaultCount -eq $expectedIntentional -and
            $unapproved.Count -eq 0
        HardDiagnosticFree = $hardDiagnosticCount -eq 0
        HardDiagnosticCount = $hardDiagnosticCount
        ApprovedStockFilterCount = $approvedStockFilterCount
        ApprovedIntentionalFaultCount = $approvedIntentionalFaultCount
        UnapprovedHardDiagnosticCount = $unapproved.Count
        UnapprovedHardDiagnosticLines = $unapproved.ToArray()
        MarkerOrderExact = $markerOrderExact
        ProfileProofTokensSeen = $profileProofTokensSeen
    }
}

function Get-PartisanFocusedRunBinding {
    param(
        [Parameter(Mandatory = $true)][string]$EvidenceRootPath,
        [Parameter(Mandatory = $true)][string]$RunPath,
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath
    )

    $resolvedRun = Resolve-PartisanFocusedExistingFile `
        -Path $RunPath `
        -Label 'Focused run envelope'
    if ((Split-Path -Leaf $resolvedRun) -cne 'run.json' -or
        -not (Test-PartisanFocusedContainedPath `
            -Root $EvidenceRootPath `
            -Path $resolvedRun)) {
        Throw-PartisanFocusedAggregate `
            -Code 'path_escape' `
            -Message 'Every focused input must be a contained run.json file.'
    }
    Assert-PartisanFocusedNoReparseAncestry `
        -Root $EvidenceRootPath `
        -Path $resolvedRun `
        -Label 'Focused run envelope'
    $portableRunPath = Get-PartisanFocusedPortableRelativePath `
        -Root $EvidenceRootPath `
        -Path $resolvedRun `
        -Label 'Focused run envelope'
    $runId = Split-Path -Leaf (Split-Path -Parent $resolvedRun)
    if ($runId -cnotmatch '^\d{8}T\d{6}Z-[0-9a-f]{32}$') {
        Throw-PartisanFocusedAggregate `
            -Code 'policy_drift' `
            -Message 'The focused run ID is not canonical.'
    }
    $runNonce = $runId.Substring($runId.Length - 32)

    $runInput = Read-PartisanFocusedJson `
        -Path $resolvedRun `
        -Label 'Focused run envelope' `
        -Code 'schema_drift'
    $run = $runInput.Value
    Assert-PartisanFocusedProperties `
        -Value $run `
        -Names @(
            'schemaVersion',
            'evidenceKind',
            'startedUtc',
            'completedUtc',
            'candidate',
            'harness',
            'launch',
            'outcome',
            'cleanup',
            'files') `
        -Label 'Focused run envelope' `
        -Exact
    Assert-PartisanFocusedArray `
        -Value $run.files `
        -Label 'Focused run file index'
    $runSchema = Require-PartisanFocusedInteger `
        -Value $run.schemaVersion `
        -Label 'Focused run schema'
    if ($runSchema -ne $script:FocusedRunSchema) {
        Throw-PartisanFocusedAggregate `
            -Code 'schema_drift' `
            -Message 'The focused run schema is unsupported.'
    }
    if ([string]$run.evidenceKind -cne $script:FocusedRunEvidenceKind) {
        Throw-PartisanFocusedAggregate `
            -Code 'policy_drift' `
            -Message 'The focused run evidence kind is unsupported.'
    }
    Assert-PartisanFocusedProperties `
        -Value $run.candidate `
        -Names @(
            'candidateId',
            'candidateVersion',
            'runtimeUseDisposition',
            'gitHead',
            'embeddedBuildSha',
            'embeddedBuildUtc',
            'embeddedBuildLabel',
            'campaignSchema',
            'runtimeSettingsSchema',
            'addonId',
            'addonGuid',
            'packageHashAlgorithm',
            'packageSha256',
            'manifestSha256',
            'readySha256',
            'workbenchCrc',
            'runtimeRole',
            'diagnosticExecutable',
            'recordedDiagnosticExecutable',
            'recordedRuntimeExecutable') `
        -Label 'Focused run candidate identity' `
        -Exact `
        -Code 'candidate_tampering'

    $startedUtcText = Require-PartisanFocusedText `
        -Value $run.startedUtc `
        -Label 'Focused run started UTC' `
        -Code 'policy_drift'
    $completedUtcText = Require-PartisanFocusedText `
        -Value $run.completedUtc `
        -Label 'Focused run completed UTC' `
        -Code 'policy_drift'
    $startedUtc = Require-PartisanFocusedUtc `
        -Value $startedUtcText `
        -Label 'Focused run started UTC'
    $completedUtc = Require-PartisanFocusedUtc `
        -Value $completedUtcText `
        -Label 'Focused run completed UTC'
    if ($startedUtc -ge $completedUtc -or
        -not $runId.StartsWith(
            $startedUtc.UtcDateTime.ToString(
                'yyyyMMddTHHmmssZ',
                [Globalization.CultureInfo]::InvariantCulture),
            [StringComparison]::Ordinal)) {
        Throw-PartisanFocusedAggregate `
            -Code 'policy_drift' `
            -Message 'The focused run time window is invalid.'
    }

    Assert-PartisanFocusedProperties `
        -Value $run.launch `
        -Names @(
            'testCase',
            'stagedPackage',
            'addonSearchRootCount',
            'addonGuid',
            'packageSha256',
            'diagnosticExecutable',
            'recordedRuntimeExecutable') `
        -Label 'Focused launch policy' `
        -Exact `
        -Code 'policy_drift'
    $profile = Require-PartisanFocusedText `
        -Value $run.launch.testCase `
        -Label 'Focused testcase ID' `
        -Code 'profile_set_invalid'
    if ($script:FocusedProfileOrder -cnotcontains $profile) {
        Throw-PartisanFocusedAggregate `
            -Code 'profile_set_invalid' `
            -Message 'A focused run uses a noncanonical testcase ID.'
    }
    if ([string]$run.candidate.runtimeUseDisposition -cne
            $script:FocusedRunDisposition) {
        Throw-PartisanFocusedAggregate `
            -Code 'disposition_drift' `
            -Message 'The focused run was not captured while the candidate was active.'
    }
    $suite = [string]$script:FocusedSuiteByProfile[$profile]
    $expectedTestCases = @($script:FocusedSuiteTestCases[$profile])
    $expectedJUnitCount = $expectedTestCases.Count
    if ($expectedJUnitCount -le 0) {
        Throw-PartisanFocusedAggregate `
            -Code 'profile_set_invalid' `
            -Message 'A focused suite lacks an exact testcase manifest.'
    }
    if ((Require-PartisanFocusedBoolean `
            -Value $run.launch.stagedPackage `
            -Label 'Focused staged-package policy' `
            -Code 'policy_drift') -ne $true -or
        (Require-PartisanFocusedInteger `
            -Value $run.launch.addonSearchRootCount `
            -Label 'Focused addon-search-root count' `
            -Code 'policy_drift') -ne 2) {
        Throw-PartisanFocusedAggregate `
            -Code 'policy_drift' `
            -Message 'The focused launch policy drifted.'
    }

    Assert-PartisanFocusedProperties `
        -Value $run.harness `
        -Names @(
            'gitHead',
            'dirty',
            'focusedRunnerSha256',
            'candidateModuleSha256') `
        -Label 'Focused harness identity' `
        -Exact `
        -Code 'harness_identity_drift'
    $harnessHead = Require-PartisanFocusedText `
        -Value $run.harness.gitHead `
        -Label 'Focused harness Git HEAD' `
        -Code 'harness_identity_drift'
    if ($harnessHead -cnotmatch '^[0-9a-f]{40}$' -or
        (Require-PartisanFocusedBoolean `
            -Value $run.harness.dirty `
            -Label 'Focused harness dirty flag' `
            -Code 'harness_identity_drift')) {
        Throw-PartisanFocusedAggregate `
            -Code 'harness_identity_drift' `
            -Message 'The focused harness is not one clean committed checkout.'
    }
    $runnerSha = Require-PartisanFocusedSha256 `
        -Value $run.harness.focusedRunnerSha256 `
        -Label 'Focused runner SHA-256' `
        -Code 'harness_identity_drift'
    $candidateModuleSha = Require-PartisanFocusedSha256 `
        -Value $run.harness.candidateModuleSha256 `
        -Label 'Candidate module SHA-256' `
        -Code 'harness_identity_drift'
    $harness = [pscustomobject][ordered]@{
        gitHead = $harnessHead
        dirty = $false
        focusedRunnerSha256 = $runnerSha
        candidateModuleSha256 = $candidateModuleSha
    }

    Assert-PartisanFocusedProperties `
        -Value $run.outcome `
        -Names @(
            'success',
            'candidateBoundaryVerified',
            'mountAttestation',
            'evidenceCaptured',
            'result',
            'diagnosticTail',
            'error') `
        -Label 'Focused guarded outcome' `
        -Exact `
        -Code 'status_drift'
    $diagnosticTailProperty = $run.outcome.PSObject.Properties['diagnosticTail']
    if (-not $diagnosticTailProperty -or
        $diagnosticTailProperty.Value -isnot [Array] -or
        @($run.outcome.diagnosticTail).Count -gt 80) {
        Throw-PartisanFocusedAggregate `
            -Code 'diagnostic_tail_invalid' `
            -Message 'The focused diagnostic tail schema is invalid.'
    }
    foreach ($diagnosticLine in @($run.outcome.diagnosticTail)) {
        if ($diagnosticLine -isnot [string] -or
            ([string]$diagnosticLine).Length -gt 4096 -or
            [string]$diagnosticLine -match '[\x00\r\n]' -or
            [string]$diagnosticLine -match
                '(?i)(?:[A-Z]:[\\/]|\\\\|file://|/(?:Users|home|mnt)/)' -or
            [string]$diagnosticLine -cnotmatch
                'HST_|Autotest|autotest|Test Result|SCRIPT\s+\(E\)|ENGINE\s+\(E\)') {
            Throw-PartisanFocusedAggregate `
                -Code 'diagnostic_tail_invalid' `
                -Message 'The focused diagnostic tail is not portable or canonical.'
        }
    }
    Assert-PartisanFocusedProperties `
        -Value $run.outcome.mountAttestation `
        -Names @(
            'Valid',
            'RecordCount',
            'ExactPathCount',
            'PackedCount',
            'InvalidModeCount',
            'GuidExact',
            'Packed') `
        -Label 'Focused mount attestation' `
        -Exact `
        -Code 'status_drift'
    $mount = $run.outcome.mountAttestation
    $mountRecordCount = Require-PartisanFocusedInteger `
        -Value $mount.RecordCount `
        -Label 'Focused mount record count' `
        -Code 'status_drift'
    $mountExactPathCount = Require-PartisanFocusedInteger `
        -Value $mount.ExactPathCount `
        -Label 'Focused mount exact-path count' `
        -Code 'status_drift'
    $mountPackedCount = Require-PartisanFocusedInteger `
        -Value $mount.PackedCount `
        -Label 'Focused mount packed count' `
        -Code 'status_drift'
    $mountInvalidModeCount = Require-PartisanFocusedInteger `
        -Value $mount.InvalidModeCount `
        -Label 'Focused mount invalid-mode count' `
        -Code 'status_drift'
    if (-not (Require-PartisanFocusedBoolean `
            -Value $run.outcome.success `
            -Label 'Focused outcome success' `
            -Code 'status_drift') -or
        -not (Require-PartisanFocusedBoolean `
            -Value $run.outcome.candidateBoundaryVerified `
            -Label 'Focused candidate boundary' `
            -Code 'status_drift') -or
        -not (Require-PartisanFocusedBoolean `
            -Value $run.outcome.evidenceCaptured `
            -Label 'Focused evidence capture' `
            -Code 'status_drift') -or
        $null -ne $run.outcome.error -or
        -not (Require-PartisanFocusedBoolean `
            -Value $mount.Valid `
            -Label 'Focused mount validity' `
            -Code 'status_drift') -or
        -not (Require-PartisanFocusedBoolean `
            -Value $mount.GuidExact `
            -Label 'Focused mount GUID identity' `
            -Code 'status_drift') -or
        -not (Require-PartisanFocusedBoolean `
            -Value $mount.Packed `
            -Label 'Focused packed mount' `
            -Code 'status_drift') -or
        $mountRecordCount -ne 2 -or
        $mountExactPathCount -ne 2 -or
        $mountPackedCount -ne 1 -or
        $mountInvalidModeCount -ne 0) {
        Throw-PartisanFocusedAggregate `
            -Code 'status_drift' `
            -Message 'The focused mount/outcome boundary is not accepted.'
    }

    Assert-PartisanFocusedProperties `
        -Value $run.outcome.result `
        -Names @(
            'Candidate',
            'CandidateBoundaryVerified',
            'MountAttestation',
            'Success',
            'ExitCode',
            'Tests',
            'Failures',
            'Errors',
            'JUnitTestCaseCount',
            'JUnitCaseName',
            'JUnitCaseClassName',
            'JUnitCaseIdentityExact',
            'JUnitCaseFailures',
            'JUnitCaseErrors',
            'JUnitCaseSkipped',
            'JUnitFailureEvidence',
            'JUnitErrorEvidence',
            'FailedListFileCount',
            'FailedListBytes',
            'RequiredPatternsSeen',
            'BuildProvenanceSeen',
            'ConsoleTestCaseSeen',
            'HardDiagnosticClassifierChecks',
            'HardDiagnosticClassificationValid',
            'HardDiagnosticFree',
            'HardDiagnosticCount',
            'ApprovedStockFilterDiagnosticCount',
            'ApprovedIntentionalFaultDiagnosticCount',
            'UnapprovedHardDiagnosticCount',
            'UnapprovedHardDiagnosticEvidence') `
        -Label 'Focused result' `
        -Exact `
        -Code 'status_drift'
    $result = $run.outcome.result
    Assert-PartisanFocusedArray `
        -Value $result.UnapprovedHardDiagnosticEvidence `
        -Label 'Focused unapproved diagnostic evidence' `
        -Code 'status_drift'
    $classifierChecks = Require-PartisanFocusedInteger `
        -Value $result.HardDiagnosticClassifierChecks `
        -Label 'Focused classifier check count' `
        -Code 'status_drift'
    $hardDiagnostics = Require-PartisanFocusedInteger `
        -Value $result.HardDiagnosticCount `
        -Label 'Focused hard diagnostic count' `
        -Code 'status_drift'
    $stockDiagnostics = Require-PartisanFocusedInteger `
        -Value $result.ApprovedStockFilterDiagnosticCount `
        -Label 'Focused stock diagnostic count' `
        -Code 'status_drift'
    $intentionalDiagnostics = Require-PartisanFocusedInteger `
        -Value $result.ApprovedIntentionalFaultDiagnosticCount `
        -Label 'Focused intentional diagnostic count' `
        -Code 'status_drift'
    $unapprovedDiagnostics = Require-PartisanFocusedInteger `
        -Value $result.UnapprovedHardDiagnosticCount `
        -Label 'Focused unapproved diagnostic count' `
        -Code 'status_drift'
    $hardFree = Require-PartisanFocusedBoolean `
        -Value $result.HardDiagnosticFree `
        -Label 'Focused hard-diagnostic-free flag' `
        -Code 'status_drift'
    $resultCandidateBoundary = Require-PartisanFocusedBoolean `
        -Value $result.CandidateBoundaryVerified `
        -Label 'Focused result candidate boundary' `
        -Code 'status_drift'
    $resultSuccess = Require-PartisanFocusedBoolean `
        -Value $result.Success `
        -Label 'Focused result success' `
        -Code 'status_drift'
    $resultCaseIdentity = Require-PartisanFocusedBoolean `
        -Value $result.JUnitCaseIdentityExact `
        -Label 'Focused JUnit case identity' `
        -Code 'status_drift'
    $resultRequiredPatterns = Require-PartisanFocusedBoolean `
        -Value $result.RequiredPatternsSeen `
        -Label 'Focused required-pattern result' `
        -Code 'status_drift'
    $resultBuildProvenance = Require-PartisanFocusedBoolean `
        -Value $result.BuildProvenanceSeen `
        -Label 'Focused build-provenance result' `
        -Code 'status_drift'
    $resultConsoleCase = Require-PartisanFocusedBoolean `
        -Value $result.ConsoleTestCaseSeen `
        -Label 'Focused console testcase result' `
        -Code 'status_drift'
    $resultClassifierValid = Require-PartisanFocusedBoolean `
        -Value $result.HardDiagnosticClassificationValid `
        -Label 'Focused classifier validity' `
        -Code 'status_drift'
    $resultExitCode = Require-PartisanFocusedInteger `
        -Value $result.ExitCode `
        -Label 'Focused result exit code' `
        -Code 'status_drift'
    $resultTests = Require-PartisanFocusedInteger `
        -Value $result.Tests `
        -Label 'Focused result JUnit tests' `
        -Code 'status_drift'
    $resultFailures = Require-PartisanFocusedInteger `
        -Value $result.Failures `
        -Label 'Focused result JUnit failures' `
        -Code 'status_drift'
    $resultErrors = Require-PartisanFocusedInteger `
        -Value $result.Errors `
        -Label 'Focused result JUnit errors' `
        -Code 'status_drift'
    $resultCaseCount = Require-PartisanFocusedInteger `
        -Value $result.JUnitTestCaseCount `
        -Label 'Focused result testcase count' `
        -Code 'status_drift'
    $resultCaseFailures = Require-PartisanFocusedInteger `
        -Value $result.JUnitCaseFailures `
        -Label 'Focused testcase failures' `
        -Code 'status_drift'
    $resultCaseErrors = Require-PartisanFocusedInteger `
        -Value $result.JUnitCaseErrors `
        -Label 'Focused testcase errors' `
        -Code 'status_drift'
    $resultCaseSkipped = Require-PartisanFocusedInteger `
        -Value $result.JUnitCaseSkipped `
        -Label 'Focused testcase skipped count' `
        -Code 'status_drift'
    $resultFailedListCount = Require-PartisanFocusedInteger `
        -Value $result.FailedListFileCount `
        -Label 'Focused failed-list file count' `
        -Code 'status_drift'
    $resultFailedListBytes = Require-PartisanFocusedInteger `
        -Value $result.FailedListBytes `
        -Label 'Focused failed-list bytes' `
        -Code 'status_drift'
    Assert-PartisanFocusedSameValue `
        -Expected $mount `
        -Actual $result.MountAttestation `
        -Label 'Focused result mount attestation' `
        -Code 'status_drift'
    if ($classifierChecks -ne $script:FocusedHardDiagnosticClassifierChecks -or
        $hardDiagnostics -lt 0 -or
        $stockDiagnostics -lt 0 -or
        $intentionalDiagnostics -lt 0 -or
        $unapprovedDiagnostics -ne 0 -or
        $hardDiagnostics -ne
            ($stockDiagnostics + $intentionalDiagnostics + $unapprovedDiagnostics) -or
        $hardFree -ne ($hardDiagnostics -eq 0) -or
        -not $resultCandidateBoundary -or
        -not $resultSuccess -or
        $resultExitCode -ne 0 -or
        $resultTests -ne $expectedJUnitCount -or
        $resultFailures -ne 0 -or
        $resultErrors -ne 0 -or
        $resultCaseCount -ne $expectedJUnitCount -or
        [string]$result.JUnitCaseName -cne $profile -or
        [string]$result.JUnitCaseClassName -cne $suite -or
        -not $resultCaseIdentity -or
        $resultCaseFailures -ne 0 -or
        $resultCaseErrors -ne 0 -or
        $resultCaseSkipped -ne 0 -or
        -not [string]::IsNullOrEmpty([string]$result.JUnitFailureEvidence) -or
        -not [string]::IsNullOrEmpty([string]$result.JUnitErrorEvidence) -or
        $resultFailedListCount -ne 1 -or
        $resultFailedListBytes -ne 0 -or
        -not $resultRequiredPatterns -or
        -not $resultBuildProvenance -or
        -not $resultConsoleCase -or
        -not $resultClassifierValid -or
        @($result.UnapprovedHardDiagnosticEvidence).Count -ne 0) {
        Throw-PartisanFocusedAggregate `
            -Code 'status_drift' `
            -Message 'The focused result is not an exact accepted case.'
    }

    Assert-PartisanFocusedProperties `
        -Value $run.cleanup `
        -Names @(
            'GuardRemaining',
            'GuardBaseRemaining',
            'EngineProcessesRemaining',
            'OwnedProcessesRemaining',
            'UnclaimedEngineProcessesObserved',
            'NewDefaultEntriesRemaining',
            'ModifiedDefaultFiles',
            'DeletedDefaultEntries',
            'MissingDefaultRoots',
            'ExternalSpillEntriesRemaining',
            'ModifiedSpillFiles',
            'DeletedSpillEntries',
            'MissingSpillRoots',
            'MonitoringRootsAreDetectionOnly',
            'CleanupErrors') `
        -Label 'Focused cleanup receipt' `
        -Exact `
        -Code 'status_drift'
    Assert-PartisanFocusedArray `
        -Value $run.cleanup.CleanupErrors `
        -Label 'Focused cleanup errors' `
        -Code 'status_drift'
    foreach ($cleanupName in @(
            'GuardRemaining',
            'GuardBaseRemaining',
            'EngineProcessesRemaining',
            'OwnedProcessesRemaining',
            'UnclaimedEngineProcessesObserved',
            'NewDefaultEntriesRemaining',
            'ModifiedDefaultFiles',
            'DeletedDefaultEntries',
            'MissingDefaultRoots',
            'ExternalSpillEntriesRemaining',
            'ModifiedSpillFiles',
            'DeletedSpillEntries',
            'MissingSpillRoots')) {
        if ((Require-PartisanFocusedInteger `
                -Value $run.cleanup.$cleanupName `
                -Label "Focused cleanup $cleanupName" `
                -Code 'status_drift') -ne 0) {
            Throw-PartisanFocusedAggregate `
                -Code 'status_drift' `
                -Message 'The focused cleanup receipt contains residue.'
        }
    }
    if (-not (Require-PartisanFocusedBoolean `
            -Value $run.cleanup.MonitoringRootsAreDetectionOnly `
            -Label 'Focused monitoring-root policy' `
            -Code 'policy_drift') -or
        @($run.cleanup.CleanupErrors).Count -ne 0) {
        Throw-PartisanFocusedAggregate `
            -Code 'status_drift' `
            -Message 'The focused cleanup policy or receipt is invalid.'
    }

    $fileRows = @(Get-PartisanFocusedIndexedFiles `
        -EvidenceRootPath $EvidenceRootPath `
        -RunPath $resolvedRun `
        -Run $run)
    $candidateBinding = Get-PartisanFocusedCandidateBinding `
        -Run $run `
        -FileRows $fileRows `
        -StartedUtcText $startedUtcText `
        -RepositoryRootPath $RepositoryRootPath
    $expectedPortableRunPath =
        $candidateBinding.SummaryIdentity.candidateId +
        '/focused-autotest/' + $profile + '/' + $runId + '/run.json'
    if ($portableRunPath -cne $expectedPortableRunPath) {
        Throw-PartisanFocusedAggregate `
            -Code 'source_path_invalid' `
            -Message 'A focused run does not use its canonical evidence-bundle path.'
    }
    $junitRow = @($fileRows | Where-Object { $_.path -cmatch '/junit\.xml$' })
    $failedRow = @($fileRows | Where-Object {
        $_.path -cmatch '/autotest_failed\.log$'
    })
    $consoleRow = @($fileRows | Where-Object {
        $_.path -cmatch '/console\.log$'
    })
    $autotestRow = @($fileRows | Where-Object {
        $_.path -cmatch '/autotest\.log$'
    })
    $errorRow = @($fileRows | Where-Object {
        $_.path -cmatch '/error\.log$'
    })
    $scriptRow = @($fileRows | Where-Object {
        $_.path -cmatch '/script\.log$'
    })
    if ($junitRow.Count -ne 1 -or
        $failedRow.Count -ne 1 -or
        $consoleRow.Count -ne 1 -or
        $autotestRow.Count -ne 1 -or
        $errorRow.Count -ne 1 -or
        $scriptRow.Count -ne 1 -or
        $failedRow[0].length -ne 0) {
        Throw-PartisanFocusedAggregate `
            -Code 'raw_junit_tampering' `
            -Message 'The focused raw result roles are invalid.'
    }
    $junitInput = Read-PartisanFocusedIndexedUtf8Text `
        -Row $junitRow[0] `
        -Label 'Focused JUnit blob' `
        -Code 'raw_junit_tampering'
    $junit = Get-PartisanFocusedJUnitEvidence `
        -JUnitInput $junitInput `
        -ExpectedProfile $profile `
        -ExpectedSuite $suite
    $consoleInput = Read-PartisanFocusedIndexedUtf8Text `
        -Row $consoleRow[0] `
        -Label 'Focused console blob' `
        -Code 'raw_result_tampering'
    $failedInput = Read-PartisanFocusedIndexedUtf8Text `
        -Row $failedRow[0] `
        -Label 'Focused failed-list blob' `
        -Code 'raw_result_tampering'
    foreach ($textRole in @(
            @($autotestRow[0], 'Focused autotest blob'),
            @($errorRow[0], 'Focused error blob'),
            @($scriptRow[0], 'Focused script blob'))) {
        [void](Read-PartisanFocusedIndexedUtf8Text `
            -Row $textRole[0] `
            -Label $textRole[1] `
            -Code 'raw_result_tampering')
    }
    $consoleText = $consoleInput.Text
    $requiredPatternContract = Get-PartisanFocusedRequiredPatternContract `
        -ConsoleText $consoleText
    if (-not $resultRequiredPatterns -or
        $requiredPatternContract.PatternCount -le 0) {
        Throw-PartisanFocusedAggregate `
            -Code 'raw_result_tampering' `
            -Message 'The focused required-pattern result differs from retained raw evidence.'
    }
    $rawMount = Get-PartisanFocusedRawMountAttestation `
        -ConsoleText $consoleText `
        -ExpectedAddonGuid ([string]$candidateBinding.PublicIdentity.addonGuid) `
        -ExpectedRunNonce $runNonce
    foreach ($mountProperty in @(
            'Valid', 'RecordCount', 'ExactPathCount', 'PackedCount',
            'InvalidModeCount', 'GuidExact', 'Packed')) {
        if ($rawMount.$mountProperty -ne $mount.$mountProperty) {
            Throw-PartisanFocusedAggregate `
                -Code 'raw_mount_tampering' `
                -Message 'Recorded mount attestation differs from retained console evidence.'
        }
    }
    $rawDiagnosticCensus = Get-PartisanFocusedRawDiagnosticCensus `
        -ConsoleText $consoleText `
        -ExpectedProfile $profile `
        -ExpectedSuite $suite
    if (-not $rawDiagnosticCensus.Valid -or
        $classifierChecks -ne $script:FocusedHardDiagnosticClassifierChecks -or
        $hardDiagnostics -ne $rawDiagnosticCensus.HardDiagnosticCount -or
        $stockDiagnostics -ne $rawDiagnosticCensus.ApprovedStockFilterCount -or
        $intentionalDiagnostics -ne
            $rawDiagnosticCensus.ApprovedIntentionalFaultCount -or
        $unapprovedDiagnostics -ne
            $rawDiagnosticCensus.UnapprovedHardDiagnosticCount -or
        $hardFree -ne $rawDiagnosticCensus.HardDiagnosticFree -or
        $resultClassifierValid -ne $rawDiagnosticCensus.Valid -or
        @($result.UnapprovedHardDiagnosticEvidence).Count -ne
            @($rawDiagnosticCensus.UnapprovedHardDiagnosticLines).Count) {
        Throw-PartisanFocusedAggregate `
            -Code 'raw_diagnostic_tampering' `
            -Message 'Recorded focused diagnostics differ from retained console evidence.'
    }
    $derivedDiagnosticTail = @($consoleText -split "`r?`n" | Where-Object {
        $_ -match 'HST_|Autotest|autotest|Test Result|SCRIPT\s+\(E\)|ENGINE\s+\(E\)'
    } | Select-Object -Last 80 | ForEach-Object {
        ConvertTo-PartisanFocusedSafeDiagnosticText -Text ([string]$_)
    })
    if ((@($run.outcome.diagnosticTail) -join "`n") -cne
        ($derivedDiagnosticTail -join "`n")) {
        Throw-PartisanFocusedAggregate `
            -Code 'diagnostic_tail_invalid' `
            -Message 'The focused diagnostic tail differs from retained console evidence.'
    }
    $buildSummary = 'sha ' +
        $candidateBinding.PublicIdentity.embeddedBuildSha +
        ' | utc ' + $candidateBinding.PublicIdentity.embeddedBuildUtc +
        ' | label ' + $candidateBinding.PublicIdentity.embeddedBuildLabel
    foreach ($requiredConsoleText in @(
            $profile,
            $buildSummary,
            'Autotest JUnit XML saved to:',
            'Autotest failed list saved to:')) {
        if ($consoleText.IndexOf(
                $requiredConsoleText,
                [StringComparison]::Ordinal) -lt 0) {
            Throw-PartisanFocusedAggregate `
                -Code 'raw_result_tampering' `
                -Message 'The focused console blob omits required accepted evidence.'
        }
    }
    foreach ($expectedTestCase in $expectedTestCases) {
        if ($consoleText.IndexOf(
                ([string]$expectedTestCase + ': SUCCESS'),
                [StringComparison]::Ordinal) -lt 0) {
            Throw-PartisanFocusedAggregate `
                -Code 'raw_result_tampering' `
                -Message 'The focused console omits an expected passing testcase.'
        }
    }
    if ($resultTests -ne $junit.Tests -or
        $resultFailures -ne $junit.Failures -or
        $resultErrors -ne $junit.Errors -or
        $resultCaseCount -ne $expectedJUnitCount -or
        $resultCaseFailures -ne $junit.Failures -or
        $resultCaseErrors -ne $junit.Errors -or
        $resultCaseSkipped -ne $junit.Skipped -or
        $resultFailedListCount -ne 1 -or
        $resultFailedListBytes -ne $failedInput.Length) {
        Throw-PartisanFocusedAggregate `
            -Code 'raw_result_tampering' `
            -Message 'Recorded focused result counts differ from retained raw evidence.'
    }

    # Close the per-envelope snapshot after all semantic reads. This repeats
    # ancestry, exact census, length, and SHA verification, and binds the
    # parsed run envelope to the same retained bytes used for its identity.
    $closingRunInput = Read-PartisanFocusedJson `
        -Path $resolvedRun `
        -Label 'Closing focused run envelope' `
        -Code 'evidence_snapshot_drift'
    if (-not (Test-PartisanFocusedBytesEqual `
            -Expected $runInput.Bytes `
            -Actual $closingRunInput.Bytes)) {
        Throw-PartisanFocusedAggregate `
            -Code 'evidence_snapshot_drift' `
            -Message 'A focused run envelope changed during validation.'
    }
    $closingRows = @(Get-PartisanFocusedIndexedFiles `
        -EvidenceRootPath $EvidenceRootPath `
        -RunPath $resolvedRun `
        -Run $closingRunInput.Value)
    if ($closingRows.Count -ne $fileRows.Count) {
        Throw-PartisanFocusedAggregate `
            -Code 'evidence_snapshot_drift' `
            -Message 'A focused run census changed during validation.'
    }
    for ($closingIndex = 0; $closingIndex -lt $fileRows.Count; $closingIndex++) {
        if ([string]$closingRows[$closingIndex].path -cne
                [string]$fileRows[$closingIndex].path -or
            [long]$closingRows[$closingIndex].length -ne
                [long]$fileRows[$closingIndex].length -or
            [string]$closingRows[$closingIndex].sha256 -cne
                [string]$fileRows[$closingIndex].sha256) {
            Throw-PartisanFocusedAggregate `
                -Code 'evidence_snapshot_drift' `
                -Message 'A focused indexed blob changed during validation.'
        }
    }

    $assertions = New-Object Collections.Generic.List[object]
    foreach ($assertionName in $script:FocusedPolicyAssertionNames) {
        [void]$assertions.Add([pscustomobject][ordered]@{
            assertionId = $profile + '.' + $assertionName
            testCase = $profile
            status = 'PASS'
            assertionClass = 'aggregate-policy'
        })
    }

    $portableRows = @($fileRows | Sort-Object path | ForEach-Object {
        [pscustomobject][ordered]@{
            path = $_.path
            length = [long]$_.length
            sha256 = $_.sha256
        }
    })
    $envelopeSha = $runInput.Sha256
    return [pscustomobject][ordered]@{
        TestCase = $profile
        SuiteClass = $suite
        RunId = $runId
        StartedUtcText = $startedUtcText
        StartedUtc = $startedUtc
        CompletedUtcText = $completedUtcText
        CompletedUtc = $completedUtc
        EnvelopePath = $portableRunPath
        EnvelopeSha256 = $envelopeSha
        Files = $portableRows
        Candidate = $candidateBinding.SummaryIdentity
        CandidatePublic = $candidateBinding.PublicIdentity
        Toolchain = $candidateBinding.Toolchain
        Harness = $harness
        JUnit = $junit
        ClassifierChecks = $script:FocusedHardDiagnosticClassifierChecks
        HardDiagnosticFree = $rawDiagnosticCensus.HardDiagnosticFree
        HardDiagnosticCount = $rawDiagnosticCensus.HardDiagnosticCount
        StockDiagnosticCount = $rawDiagnosticCensus.ApprovedStockFilterCount
        IntentionalDiagnosticCount = $rawDiagnosticCensus.ApprovedIntentionalFaultCount
        UnapprovedDiagnosticCount = $rawDiagnosticCensus.UnapprovedHardDiagnosticCount
        Assertions = $assertions.ToArray()
    }
}

function Assert-PartisanFocusedAggregateValue {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$Code = 'aggregate_schema_drift',
        [string]$ExpectedCandidateId = '',
        [switch]$AllowTrackedLegacyAggregate
    )

    Assert-PartisanFocusedProperties `
        -Value $Value `
        -Names @(
            'schemaVersion', 'evidenceKind', 'aggregateId',
            'aggregationPolicy', 'admission', 'candidate', 'harness',
            'integrity', 'acceptedWindow', 'result',
            'aggregatePolicyAssertions', 'cases', 'sourceRuns',
            'preliminaryRuns') `
        -Label $Label `
        -Exact `
        -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $Value.aggregationPolicy `
        -Names @(
            'contractId', 'policyVersion', 'inputSchemaVersion',
            'inputEvidenceKind', 'requiredRuntimeUseDisposition',
            'requiredCandidateState', 'profileOrder', 'profileCount',
            'filesPerProfile', 'totalFileCount', 'rawReopenRequired',
            'exactDirectoryCensusRequired', 'candidateSealReopenRequired',
            'historicalBlobImmutabilityRequired',
            'aggregatePolicyAssertionsPerProfile',
            'aggregatePolicyAssertionCount', 'acceptedResultStatus',
            'acceptedDisposition', 'rejectionDisposition') `
        -Label "$Label aggregation policy" `
        -Exact `
        -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $Value.admission `
        -Names @('status', 'disposition', 'releaseDecision', 'certifying') `
        -Label "$Label admission" -Exact -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $Value.candidate `
        -Names @(
            'candidateId', 'candidateSourceHead', 'packageSha256',
            'manifestSha256', 'readySha256', 'workbenchCrc') `
        -Label "$Label candidate" -Exact -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $Value.harness `
        -Names @(
            'gitHead', 'dirty', 'focusedRunnerSha256',
            'candidateModuleSha256', 'gitBlobProvenanceVerified') `
        -Label "$Label harness" -Exact -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $Value.integrity `
        -Names @(
            'aggregationGitHead', 'focusedRunHarness', 'aggregateProducer',
            'releaseDocsConsumer', 'allWorktreeHashesMatchGitBlobs') `
        -Label "$Label integrity" -Exact -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $Value.integrity.focusedRunHarness `
        -Names @(
            'gitHead', 'focusedRunnerWorktreeSha256',
            'focusedRunnerGitBlobSha256', 'candidateModuleWorktreeSha256',
            'candidateModuleGitBlobSha256') `
        -Label "$Label focused harness integrity" -Exact -Code $Code
    foreach ($toolIntegrityName in @('aggregateProducer', 'releaseDocsConsumer')) {
        Assert-PartisanFocusedProperties `
            -Value $Value.integrity.$toolIntegrityName `
            -Names @('repositoryPath', 'worktreeSha256', 'gitBlobSha256') `
            -Label "$Label $toolIntegrityName integrity" -Exact -Code $Code
    }
    Assert-PartisanFocusedProperties `
        -Value $Value.acceptedWindow `
        -Names @('startedUtc', 'completedUtc') `
        -Label "$Label accepted window" -Exact -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $Value.result `
        -Names @(
            'status', 'caseCount', 'passedCases', 'junitTests',
            'junitFailures', 'junitErrors', 'junitSkipped',
            'candidateBoundaryVerified', 'allMountsPacked',
            'allCleanupAndSpillZero', 'allEnvelopeFilesRehashed',
            'envelopeFileCount', 'hardDiagnosticClassifierChecksPerRun',
            'hardDiagnosticClassificationValid', 'hardDiagnosticFree',
            'hardDiagnosticCount', 'approvedStockFilterDiagnosticCount',
            'approvedIntentionalFaultDiagnosticCount',
            'unapprovedHardDiagnosticCount', 'scope') `
        -Label "$Label result" -Exact -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $Value.aggregatePolicyAssertions `
        -Names @('assertionClass', 'scope', 'total', 'passed', 'failed', 'assertions') `
        -Label "$Label aggregate assertions" -Exact -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $Value.preliminaryRuns `
        -Names @('caseCount', 'status', 'note') `
        -Label "$Label preliminary runs" -Exact -Code $Code
    foreach ($arrayContract in @(
            [pscustomobject]@{
                Value = $Value.aggregationPolicy.profileOrder
                Label = "$Label profile order"
            },
            [pscustomobject]@{
                Value = $Value.cases
                Label = "$Label cases"
            },
            [pscustomobject]@{
                Value = $Value.sourceRuns
                Label = "$Label source runs"
            },
            [pscustomobject]@{
                Value = $Value.aggregatePolicyAssertions.assertions
                Label = "$Label aggregate assertions"
            })) {
        Assert-PartisanFocusedArray `
            -Value $arrayContract.Value `
            -Label $arrayContract.Label `
            -Code $Code
    }

    $aggregateJUnitTests = Require-PartisanFocusedInteger `
        -Value $Value.result.junitTests `
        -Label "$Label result JUnit tests" `
        -Code $Code
    $legacyAggregate = $aggregateJUnitTests -eq 5
    if ($legacyAggregate -and -not $AllowTrackedLegacyAggregate) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label uses the retired five-wrapper contract outside immutable tracked history."
    }
    $expectedProfileOrder = if ($legacyAggregate) {
        @($script:FocusedLegacyProfileOrder)
    }
    else {
        @($script:FocusedProfileOrder)
    }
    $expectedSuiteByProfile = if ($legacyAggregate) {
        $script:FocusedLegacySuiteByProfile
    }
    else {
        $script:FocusedSuiteByProfile
    }
    $expectedJUnitTotal = if ($legacyAggregate) {
        5
    }
    else {
        $script:FocusedExpectedJUnitTotal
    }

    $policy = $Value.aggregationPolicy
    foreach ($requiredBoolean in @(
            'rawReopenRequired', 'exactDirectoryCensusRequired',
            'candidateSealReopenRequired', 'historicalBlobImmutabilityRequired')) {
        if (-not (Require-PartisanFocusedBoolean `
                -Value $policy.$requiredBoolean `
                -Label "$Label policy $requiredBoolean" `
                -Code $Code)) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message "$Label disables a mandatory schema-2 policy."
        }
    }
    $candidateId = Require-PartisanFocusedText `
        -Value $Value.candidate.candidateId `
        -Label "$Label candidate ID" `
        -Code $Code
    if (-not (Test-PartisanFocusedCandidateId -Value $candidateId) -or
        (-not [string]::IsNullOrEmpty($ExpectedCandidateId) -and
            $candidateId -cne $ExpectedCandidateId) -or
        (Require-PartisanFocusedInteger $Value.schemaVersion "$Label schema" $Code) -ne 2 -or
        [string]$Value.evidenceKind -cne $script:FocusedAggregateEvidenceKind -or
        [string]$policy.contractId -cne $script:FocusedAggregateContractId -or
        (Require-PartisanFocusedInteger $policy.policyVersion "$Label policy version" $Code) -ne 2 -or
        (Require-PartisanFocusedInteger $policy.inputSchemaVersion "$Label input schema" $Code) -ne 1 -or
        [string]$policy.inputEvidenceKind -cne $script:FocusedRunEvidenceKind -or
        [string]$policy.requiredRuntimeUseDisposition -cne $script:FocusedRunDisposition -or
        [string]$policy.requiredCandidateState -cne $script:FocusedCandidateState -or
        (@($policy.profileOrder) -join '|') -cne ($expectedProfileOrder -join '|') -or
        (Require-PartisanFocusedInteger $policy.profileCount "$Label profile count" $Code) -ne 5 -or
        (Require-PartisanFocusedInteger $policy.filesPerProfile "$Label files per profile" $Code) -ne 8 -or
        (Require-PartisanFocusedInteger $policy.totalFileCount "$Label total file count" $Code) -ne 40 -or
        (Require-PartisanFocusedInteger $policy.aggregatePolicyAssertionsPerProfile "$Label assertions per profile" $Code) -ne 7 -or
        (Require-PartisanFocusedInteger $policy.aggregatePolicyAssertionCount "$Label assertion count" $Code) -ne 35 -or
        [string]$policy.acceptedResultStatus -cne $script:FocusedAggregateResultStatus -or
        [string]$policy.acceptedDisposition -cne $script:FocusedAcceptedDisposition -or
        [string]$policy.rejectionDisposition -cne $script:FocusedReplacementDisposition -or
        [string]$Value.admission.status -cne 'accepted' -or
        [string]$Value.admission.disposition -cne $script:FocusedAcceptedDisposition -or
        [string]$Value.admission.releaseDecision -cne 'NO-GO' -or
        (Require-PartisanFocusedBoolean $Value.admission.certifying "$Label certifying" $Code) -or
        (Require-PartisanFocusedBoolean $Value.harness.dirty "$Label harness dirty" $Code) -or
        -not (Require-PartisanFocusedBoolean $Value.harness.gitBlobProvenanceVerified "$Label Git provenance" $Code) -or
        -not (Require-PartisanFocusedBoolean $Value.integrity.allWorktreeHashesMatchGitBlobs "$Label integrity" $Code)) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label has schema-2 policy or disposition drift."
    }

    foreach ($shaValue in @(
            $Value.candidate.packageSha256,
            $Value.candidate.manifestSha256,
            $Value.candidate.readySha256,
            $Value.harness.focusedRunnerSha256,
            $Value.harness.candidateModuleSha256,
            $Value.integrity.focusedRunHarness.focusedRunnerWorktreeSha256,
            $Value.integrity.focusedRunHarness.focusedRunnerGitBlobSha256,
            $Value.integrity.focusedRunHarness.candidateModuleWorktreeSha256,
            $Value.integrity.focusedRunHarness.candidateModuleGitBlobSha256,
            $Value.integrity.aggregateProducer.worktreeSha256,
            $Value.integrity.aggregateProducer.gitBlobSha256,
            $Value.integrity.releaseDocsConsumer.worktreeSha256,
            $Value.integrity.releaseDocsConsumer.gitBlobSha256)) {
        Require-PartisanFocusedSha256 `
            -Value $shaValue `
            -Label "$Label SHA-256" `
            -Code $Code | Out-Null
    }
    foreach ($headValue in @(
            $Value.candidate.candidateSourceHead,
            $Value.harness.gitHead,
            $Value.integrity.aggregationGitHead,
            $Value.integrity.focusedRunHarness.gitHead)) {
        if ([string]$headValue -cnotmatch '^[0-9a-f]{40}$') {
            Throw-PartisanFocusedAggregate -Code $Code -Message "$Label has an invalid Git head."
        }
    }
    if ([string]$Value.harness.gitHead -cne
            [string]$Value.integrity.focusedRunHarness.gitHead -or
        [string]$Value.harness.focusedRunnerSha256 -cne
            [string]$Value.integrity.focusedRunHarness.focusedRunnerWorktreeSha256 -or
        [string]$Value.harness.candidateModuleSha256 -cne
            [string]$Value.integrity.focusedRunHarness.candidateModuleWorktreeSha256 -or
        [string]$Value.integrity.focusedRunHarness.focusedRunnerWorktreeSha256 -cne
            [string]$Value.integrity.focusedRunHarness.focusedRunnerGitBlobSha256 -or
        [string]$Value.integrity.focusedRunHarness.candidateModuleWorktreeSha256 -cne
            [string]$Value.integrity.focusedRunHarness.candidateModuleGitBlobSha256 -or
        [string]$Value.integrity.aggregateProducer.worktreeSha256 -cne
            [string]$Value.integrity.aggregateProducer.gitBlobSha256 -or
        [string]$Value.integrity.releaseDocsConsumer.worktreeSha256 -cne
            [string]$Value.integrity.releaseDocsConsumer.gitBlobSha256) {
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label has inconsistent Git provenance."
    }

    $cases = @($Value.cases)
    $sourceRuns = @($Value.sourceRuns)
    $assertions = @($Value.aggregatePolicyAssertions.assertions)
    if ($cases.Count -ne 5 -or $sourceRuns.Count -ne 5 -or $assertions.Count -ne 35) {
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label has an invalid 5/40/35 census."
    }
    $runIds = New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    $envelopeHashes = New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    $assertionIds = New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    $totalFiles = 0
    $totalHard = 0
    $totalStock = 0
    $totalIntentional = 0
    $totalUnapproved = 0
    $previousCompleted = $null
    for ($profileIndex = 0; $profileIndex -lt 5; $profileIndex++) {
        $profile = $expectedProfileOrder[$profileIndex]
        $suite = [string]$expectedSuiteByProfile[$profile]
        $case = $cases[$profileIndex]
        $source = $sourceRuns[$profileIndex]
        Assert-PartisanFocusedProperties `
            -Value $case `
            -Names @(
                'testCase', 'suiteClass', 'runId', 'startedUtc', 'completedUtc',
                'envelopeSha256', 'fileCount', 'success',
                'candidateBoundaryVerified', 'mountPacked', 'junitTests',
                'junitFailures', 'junitErrors', 'junitSkipped',
                'hardDiagnosticClassifierChecks',
                'hardDiagnosticClassificationValid', 'hardDiagnosticFree',
                'hardDiagnosticCount', 'approvedStockFilterDiagnosticCount',
                'approvedIntentionalFaultDiagnosticCount',
                'unapprovedHardDiagnosticCount', 'cleanupAndSpillZero',
                'envelopeFilesRehashed') `
            -Label "$Label case" -Exact -Code $Code
        Assert-PartisanFocusedProperties `
            -Value $source `
            -Names @(
                'testCase', 'suiteClass', 'runId', 'runEnvelopePath',
                'runEnvelopeSha256', 'fileCount', 'files') `
            -Label "$Label source run" -Exact -Code $Code
        Assert-PartisanFocusedArray `
            -Value $source.files `
            -Label "$Label source files" `
            -Code $Code
        $runId = [string]$source.runId
        $envelopeSha = Require-PartisanFocusedSha256 `
            -Value $source.runEnvelopeSha256 `
            -Label "$Label envelope SHA-256" `
            -Code $Code
        $caseFileCount = Require-PartisanFocusedInteger `
            -Value $case.fileCount `
            -Label "$Label case file count" `
            -Code $Code
        $sourceFileCount = Require-PartisanFocusedInteger `
            -Value $source.fileCount `
            -Label "$Label source file count" `
            -Code $Code
        $expectedRunPath = $candidateId + '/focused-autotest/' +
            $profile + '/' + $runId + '/run.json'
        if ([string]$case.testCase -cne $profile -or
            [string]$source.testCase -cne $profile -or
            [string]$case.suiteClass -cne $suite -or
            [string]$source.suiteClass -cne $suite -or
            [string]$case.runId -cne $runId -or
            $runId -cnotmatch '^\d{8}T\d{6}Z-[0-9a-f]{32}$' -or
            -not $runIds.Add($runId) -or
            -not $envelopeHashes.Add($envelopeSha) -or
            [string]$case.envelopeSha256 -cne $envelopeSha -or
            [string]$source.runEnvelopePath -cne $expectedRunPath -or
            $caseFileCount -ne 8 -or
            $sourceFileCount -ne 8 -or
            @($source.files).Count -ne 8) {
            Throw-PartisanFocusedAggregate -Code $Code -Message "$Label has invalid source-run identity."
        }
        $caseStarted = Require-PartisanFocusedUtc `
            -Value $case.startedUtc `
            -Label "$Label case start UTC" `
            -Code $Code
        $caseCompleted = Require-PartisanFocusedUtc `
            -Value $case.completedUtc `
            -Label "$Label case completed UTC" `
            -Code $Code
        if ($caseStarted -ge $caseCompleted -or
            ($null -ne $previousCompleted -and
                $caseStarted -lt $previousCompleted)) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message "$Label has invalid case chronology."
        }
        $previousCompleted = $caseCompleted
        $filePaths = New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
        foreach ($file in @($source.files)) {
            Assert-PartisanFocusedProperties `
                -Value $file `
                -Names @('path', 'length', 'sha256') `
                -Label "$Label source blob" -Exact -Code $Code
            $filePath = Assert-PartisanFocusedPortablePath `
                -Path ([string]$file.path) `
                -Label "$Label source blob path" `
                -Code $Code
            if (-not $filePaths.Add($filePath) -or
                (Require-PartisanFocusedInteger $file.length "$Label blob length" $Code) -lt 0) {
                Throw-PartisanFocusedAggregate -Code $Code -Message "$Label has an invalid source blob."
            }
            Require-PartisanFocusedSha256 $file.sha256 "$Label blob SHA-256" $Code | Out-Null
        }
        $requiredBlobPatterns = @(
            '^identity/candidate\.json$',
            '^identity/candidate\.ready\.json$',
            '^raw/logs/[^/]+/autotest\.log$',
            '^raw/logs/[^/]+/autotest_failed\.log$',
            '^raw/logs/[^/]+/console\.log$',
            '^raw/logs/[^/]+/error\.log$',
            '^raw/logs/[^/]+/junit\.xml$',
            '^raw/logs/[^/]+/script\.log$')
        foreach ($requiredBlobPattern in $requiredBlobPatterns) {
            if (@($source.files | Where-Object {
                    [string]$_.path -cmatch $requiredBlobPattern
                }).Count -ne 1) {
                Throw-PartisanFocusedAggregate `
                    -Code $Code `
                    -Message "$Label has an invalid eight-blob role census."
            }
        }
        $failedBlob = @($source.files | Where-Object {
            [string]$_.path -cmatch
                '^raw/logs/[^/]+/autotest_failed\.log$'
        })[0]
        if ([long]$failedBlob.length -ne 0) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message "$Label has a nonempty accepted failed-list blob."
        }
        $totalFiles += @($source.files).Count
        foreach ($booleanName in @(
                'success', 'candidateBoundaryVerified', 'mountPacked',
                'hardDiagnosticClassificationValid', 'cleanupAndSpillZero',
                'envelopeFilesRehashed')) {
            if (-not (Require-PartisanFocusedBoolean $case.$booleanName "$Label case $booleanName" $Code)) {
                Throw-PartisanFocusedAggregate -Code $Code -Message "$Label has a false accepted-case flag."
            }
        }
        $caseJunitTests = Require-PartisanFocusedInteger `
            $case.junitTests "$Label case JUnit tests" $Code
        $caseJunitFailures = Require-PartisanFocusedInteger `
            $case.junitFailures "$Label case JUnit failures" $Code
        $caseJunitErrors = Require-PartisanFocusedInteger `
            $case.junitErrors "$Label case JUnit errors" $Code
        $caseJunitSkipped = Require-PartisanFocusedInteger `
            $case.junitSkipped "$Label case JUnit skipped" $Code
        $caseClassifierChecks = Require-PartisanFocusedInteger `
            $case.hardDiagnosticClassifierChecks "$Label classifier checks" $Code
        $caseHard = Require-PartisanFocusedInteger `
            $case.hardDiagnosticCount "$Label hard diagnostics" $Code
        $caseStock = Require-PartisanFocusedInteger `
            $case.approvedStockFilterDiagnosticCount "$Label stock diagnostics" $Code
        $caseIntentional = Require-PartisanFocusedInteger `
            $case.approvedIntentionalFaultDiagnosticCount "$Label intentional diagnostics" $Code
        $caseUnapproved = Require-PartisanFocusedInteger `
            $case.unapprovedHardDiagnosticCount "$Label unapproved diagnostics" $Code
        $caseHardFree = Require-PartisanFocusedBoolean `
            $case.hardDiagnosticFree "$Label hard-free flag" $Code
        $expectedCaseJunitTests = if ($legacyAggregate) {
            1
        }
        else {
            @($script:FocusedSuiteTestCases[$profile]).Count
        }
        $expectedIntentional = if ($profileIndex -eq 4) {
            $expectedCaseJunitTests
        }
        else {
            0
        }
        if ($caseJunitTests -ne $expectedCaseJunitTests -or
            $caseJunitFailures -ne 0 -or
            $caseJunitErrors -ne 0 -or
            $caseJunitSkipped -ne 0 -or
            $caseClassifierChecks -ne
                $script:FocusedHardDiagnosticClassifierChecks -or
            $caseStock -ne 2 -or
            $caseIntentional -ne $expectedIntentional -or
            $caseUnapproved -ne 0 -or
            $caseHard -ne ($caseStock + $caseIntentional + $caseUnapproved) -or
            $caseHardFree -ne ($caseHard -eq 0)) {
            Throw-PartisanFocusedAggregate -Code $Code -Message "$Label has invalid accepted-case counts."
        }
        $totalHard += $caseHard
        $totalStock += $caseStock
        $totalIntentional += $caseIntentional
        $totalUnapproved += $caseUnapproved
        for ($assertionOffset = 0; $assertionOffset -lt 7; $assertionOffset++) {
            $assertion = $assertions[($profileIndex * 7) + $assertionOffset]
            Assert-PartisanFocusedProperties `
                -Value $assertion `
                -Names @('assertionId', 'testCase', 'status', 'assertionClass') `
                -Label "$Label aggregate assertion" -Exact -Code $Code
            $expectedAssertionId = $profile + '.' +
                $script:FocusedPolicyAssertionNames[$assertionOffset]
            if ([string]$assertion.assertionId -cne $expectedAssertionId -or
                -not $assertionIds.Add([string]$assertion.assertionId) -or
                [string]$assertion.testCase -cne $profile -or
                [string]$assertion.status -cne 'PASS' -or
                [string]$assertion.assertionClass -cne 'aggregate-policy') {
                Throw-PartisanFocusedAggregate -Code $Code -Message "$Label has aggregate assertion drift."
            }
        }
    }
    $result = $Value.result
    $resultCaseCount = Require-PartisanFocusedInteger `
        $result.caseCount "$Label result case count" $Code
    $resultPassedCases = Require-PartisanFocusedInteger `
        $result.passedCases "$Label result passed cases" $Code
    $resultJunitTests = Require-PartisanFocusedInteger `
        $result.junitTests "$Label result JUnit tests" $Code
    $resultJunitFailures = Require-PartisanFocusedInteger `
        $result.junitFailures "$Label result JUnit failures" $Code
    $resultJunitErrors = Require-PartisanFocusedInteger `
        $result.junitErrors "$Label result JUnit errors" $Code
    $resultJunitSkipped = Require-PartisanFocusedInteger `
        $result.junitSkipped "$Label result JUnit skipped" $Code
    $resultEnvelopeFiles = Require-PartisanFocusedInteger `
        $result.envelopeFileCount "$Label result envelope files" $Code
    $resultClassifierChecks = Require-PartisanFocusedInteger `
        $result.hardDiagnosticClassifierChecksPerRun "$Label result classifier checks" $Code
    $resultHard = Require-PartisanFocusedInteger `
        $result.hardDiagnosticCount "$Label result hard diagnostics" $Code
    $resultStock = Require-PartisanFocusedInteger `
        $result.approvedStockFilterDiagnosticCount "$Label result stock diagnostics" $Code
    $resultIntentional = Require-PartisanFocusedInteger `
        $result.approvedIntentionalFaultDiagnosticCount "$Label result intentional diagnostics" $Code
    $resultUnapproved = Require-PartisanFocusedInteger `
        $result.unapprovedHardDiagnosticCount "$Label result unapproved diagnostics" $Code
    $assertionTotal = Require-PartisanFocusedInteger `
        $Value.aggregatePolicyAssertions.total "$Label assertion total" $Code
    $assertionPassed = Require-PartisanFocusedInteger `
        $Value.aggregatePolicyAssertions.passed "$Label assertion passed" $Code
    $assertionFailed = Require-PartisanFocusedInteger `
        $Value.aggregatePolicyAssertions.failed "$Label assertion failed" $Code
    $preliminaryCount = Require-PartisanFocusedInteger `
        $Value.preliminaryRuns.caseCount "$Label preliminary count" $Code
    if ([string]$result.status -cne $script:FocusedAggregateResultStatus -or
        $resultCaseCount -ne $cases.Count -or
        $resultPassedCases -ne 5 -or
        $resultJunitTests -ne $expectedJUnitTotal -or
        $resultJunitFailures -ne 0 -or
        $resultJunitErrors -ne 0 -or
        $resultJunitSkipped -ne 0 -or
        $resultEnvelopeFiles -ne $totalFiles -or
        $resultClassifierChecks -ne
            $script:FocusedHardDiagnosticClassifierChecks -or
        $resultHard -ne $totalHard -or
        $resultStock -ne $totalStock -or
        $resultIntentional -ne $totalIntentional -or
        $resultUnapproved -ne $totalUnapproved -or
        (Require-PartisanFocusedBoolean $result.hardDiagnosticFree "$Label hard-free" $Code) -ne
            ($totalHard -eq 0) -or
        $assertionTotal -ne $assertions.Count -or
        $assertionPassed -ne $assertions.Count -or
        $assertionFailed -ne 0 -or
        [string]$Value.aggregatePolicyAssertions.assertionClass -cne 'aggregate-policy' -or
        $preliminaryCount -ne 0 -or
        [string]$Value.preliminaryRuns.status -cne 'none') {
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label has aggregate result drift."
    }
    foreach ($resultBoolean in @(
            'candidateBoundaryVerified', 'allMountsPacked',
            'allCleanupAndSpillZero', 'allEnvelopeFilesRehashed',
            'hardDiagnosticClassificationValid')) {
        if (-not (Require-PartisanFocusedBoolean $result.$resultBoolean "$Label result $resultBoolean" $Code)) {
            Throw-PartisanFocusedAggregate -Code $Code -Message "$Label has a false result contract flag."
        }
    }
    $started = Require-PartisanFocusedUtc $Value.acceptedWindow.startedUtc "$Label start UTC" $Code
    $completed = Require-PartisanFocusedUtc $Value.acceptedWindow.completedUtc "$Label completed UTC" $Code
    if ($started -ge $completed -or
        [string]$Value.acceptedWindow.startedUtc -cne [string]$cases[0].startedUtc -or
        [string]$Value.acceptedWindow.completedUtc -cne [string]$cases[4].completedUtc -or
        [string]$Value.aggregateId -cne
            (Get-PartisanFocusedAggregateId -Integrity $Value.integrity -SourceRuns $sourceRuns)) {
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label has chronology or aggregate-ID drift."
    }
    $portableText = ConvertTo-PartisanFocusedCanonicalJson -Value $Value
    if ($portableText -match '(?i)(?:[A-Z]:[\\/]|\\\\|file://|/(?:Users|home|mnt)/)') {
        Throw-PartisanFocusedAggregate -Code $Code -Message "$Label contains a nonportable path."
    }
}

function Assert-PartisanFocusedRejectionReceiptValue {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string]$Label,
        [Parameter(Mandatory = $true)]$ExpectedCandidate,
        [string]$Code = 'historical_blob_replacement'
    )

    Assert-PartisanFocusedProperties `
        -Value $Value `
        -Names @(
            'schemaVersion', 'evidenceKind', 'aggregationPolicy', 'admission',
            'candidate', 'precedence', 'attemptedInput') `
        -Label $Label `
        -Exact `
        -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $Value.aggregationPolicy `
        -Names @(
            'contractId', 'policyVersion', 'requiredProfileCount',
            'requiredFilesPerProfile', 'requiredTotalFileCount') `
        -Label "$Label policy" -Exact -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $Value.admission `
        -Names @(
            'status', 'disposition', 'releaseDecision', 'certifying',
            'reasonCode') `
        -Label "$Label admission" -Exact -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $Value.candidate `
        -Names @(
            'candidateId', 'packageSha256', 'manifestSha256', 'readySha256') `
        -Label "$Label candidate" -Exact -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $Value.precedence `
        -Names @(
            'acceptedAggregatePresentBeforeRejection',
            'acceptedAggregateSha256') `
        -Label "$Label precedence" -Exact -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $Value.attemptedInput `
        -Names @('runEnvelopeCount', 'availableRunEnvelopeSha256') `
        -Label "$Label attempted input" -Exact -Code $Code
    Assert-PartisanFocusedArray `
        -Value $Value.attemptedInput.availableRunEnvelopeSha256 `
        -Label "$Label attempted hashes" `
        -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $ExpectedCandidate `
        -Names @(
            'candidateId', 'packageSha256', 'manifestSha256', 'readySha256') `
        -Label "$Label expected candidate" `
        -Code $Code

    $candidateId = Require-PartisanFocusedText `
        -Value $Value.candidate.candidateId `
        -Label "$Label candidate ID" `
        -Code $Code
    $expectedCandidateId = Require-PartisanFocusedText `
        -Value $ExpectedCandidate.candidateId `
        -Label "$Label expected candidate ID" `
        -Code $Code
    $acceptedBefore = Require-PartisanFocusedBoolean `
        -Value $Value.precedence.acceptedAggregatePresentBeforeRejection `
        -Label "$Label accepted-before-red flag" `
        -Code $Code
    $attemptedCount = Require-PartisanFocusedInteger `
        -Value $Value.attemptedInput.runEnvelopeCount `
        -Label "$Label attempted run count" `
        -Code $Code
    if ((Require-PartisanFocusedInteger `
            -Value $Value.schemaVersion `
            -Label "$Label schema" `
            -Code $Code) -ne $script:FocusedAggregateSchema -or
        [string]$Value.evidenceKind -cne
            $script:FocusedAggregateRejectionKind -or
        [string]$Value.aggregationPolicy.contractId -cne
            $script:FocusedAggregateContractId -or
        (Require-PartisanFocusedInteger `
            $Value.aggregationPolicy.policyVersion "$Label policy version" $Code) -ne 2 -or
        (Require-PartisanFocusedInteger `
            $Value.aggregationPolicy.requiredProfileCount "$Label profiles" $Code) -ne 5 -or
        (Require-PartisanFocusedInteger `
            $Value.aggregationPolicy.requiredFilesPerProfile "$Label files" $Code) -ne 8 -or
        (Require-PartisanFocusedInteger `
            $Value.aggregationPolicy.requiredTotalFileCount "$Label total files" $Code) -ne 40 -or
        [string]$Value.admission.status -cne 'rejected' -or
        [string]$Value.admission.disposition -cne
            $script:FocusedReplacementDisposition -or
        [string]$Value.admission.releaseDecision -cne 'RED' -or
        (Require-PartisanFocusedBoolean `
            $Value.admission.certifying "$Label certifying" $Code) -or
        [string]::IsNullOrWhiteSpace([string]$Value.admission.reasonCode) -or
        -not (Test-PartisanFocusedCandidateId -Value $candidateId) -or
        -not (Test-PartisanFocusedCandidateId -Value $expectedCandidateId) -or
        $candidateId -cne $expectedCandidateId -or
        $attemptedCount -ne 5) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label has rejection contract drift."
    }
    foreach ($sealName in @(
            'packageSha256',
            'manifestSha256',
            'readySha256')) {
        $receiptSha = Require-PartisanFocusedSha256 `
            -Value $Value.candidate.$sealName `
            -Label "$Label candidate $sealName" `
            -Code $Code
        $expectedSha = Require-PartisanFocusedSha256 `
            -Value $ExpectedCandidate.$sealName `
            -Label "$Label expected candidate $sealName" `
            -Code $Code
        if ($receiptSha -cne $expectedSha) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message "$Label does not bind the exact candidate seal identity."
        }
    }
    $seen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($sha in @($Value.attemptedInput.availableRunEnvelopeSha256)) {
        $validated = Require-PartisanFocusedSha256 `
            -Value $sha `
            -Label "$Label attempted SHA-256" `
            -Code $Code
        if (-not $seen.Add($validated)) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message "$Label repeats an attempted run hash."
        }
    }
    if ($seen.Count -ne $attemptedCount) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label does not contain the exact attempted run hash set."
    }
    if ($acceptedBefore) {
        Require-PartisanFocusedSha256 `
            -Value $Value.precedence.acceptedAggregateSha256 `
            -Label "$Label preceding accepted aggregate SHA-256" `
            -Code $Code | Out-Null
    }
    elseif ($null -ne $Value.precedence.acceptedAggregateSha256) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label claims a preceding accepted hash without an accepted aggregate."
    }
    $portable = ConvertTo-PartisanFocusedCanonicalJson -Value $Value
    if ($portable -match
        '(?i)(?:[A-Z]:[\\/]|\\\\|file://|/(?:Users|home|mnt)/)') {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message "$Label contains a nonportable path."
    }
}

function Get-PartisanFocusedTrackedReceiptCandidate {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)][string]$GitHead,
        [Parameter(Mandatory = $true)][string]$CandidateId,
        [string]$Code = 'historical_blob_replacement'
    )

    if (-not (Test-PartisanFocusedCandidateId -Value $CandidateId)) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message 'A rejection receipt candidate path identity is invalid.'
    }
    $candidatePrefix = 'docs/evidence/release-candidates/' +
        $CandidateId + '/'
    $manifestRelative = $candidatePrefix + 'candidate.json'
    $readyRelative = $candidatePrefix + 'candidate.ready.json'
    $inputs = New-Object Collections.Generic.List[object]
    foreach ($row in @(
            @($manifestRelative, 'Tracked receipt candidate manifest'),
            @($readyRelative, 'Tracked receipt candidate ready seal'))) {
        $full = Resolve-PartisanFocusedExistingFile `
            -Path (Join-Path $RepositoryRootPath $row[0].Replace('/', '\')) `
            -Label $row[1] `
            -Code $Code
        Assert-PartisanFocusedNoReparseAncestry `
            -Root $RepositoryRootPath `
            -Path $full `
            -Label $row[1]
        $input = Read-PartisanFocusedJson `
            -Path $full `
            -Label $row[1] `
            -Code $Code
        try {
            $headBytes = Get-PartisanFocusedGitBlobBytes `
                -RepositoryRootPath $RepositoryRootPath `
                -GitHead $GitHead `
                -RepositoryPath $row[0]
        }
        catch {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message 'A rejection receipt candidate seal is not available at tracked HEAD.'
        }
        if (-not (Test-PartisanFocusedBytesEqual `
                -Expected $headBytes `
                -Actual $input.Bytes)) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message 'A rejection receipt candidate seal differs from tracked HEAD.'
        }
        [void]$inputs.Add($input)
    }
    $manifestInput = $inputs[0]
    $readyInput = $inputs[1]
    $manifest = $manifestInput.Value
    $ready = $readyInput.Value
    Assert-PartisanFocusedProperties `
        -Value $manifest `
        -Names @('candidate', 'package') `
        -Label 'Tracked receipt candidate manifest' `
        -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $manifest.candidate `
        -Names @('id') `
        -Label 'Tracked receipt candidate identity' `
        -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $manifest.package `
        -Names @('files') `
        -Label 'Tracked receipt candidate package' `
        -Code $Code
    Assert-PartisanFocusedProperties `
        -Value $ready `
        -Names @(
            'schemaVersion', 'candidateId', 'gitHead', 'packageSha256',
            'manifestSha256') `
        -Label 'Tracked receipt candidate ready seal' `
        -Exact `
        -Code $Code
    $packageSha = Get-PartisanFocusedCanonicalPackageSha256 `
        -Package $manifest.package `
        -Label 'Tracked receipt candidate package' `
        -Code $Code
    $manifestCandidateId = Require-PartisanFocusedText `
        -Value $manifest.candidate.id `
        -Label 'Tracked receipt manifest candidate ID' `
        -Code $Code
    $readyCandidateId = Require-PartisanFocusedText `
        -Value $ready.candidateId `
        -Label 'Tracked receipt ready candidate ID' `
        -Code $Code
    $readyPackageSha = Require-PartisanFocusedSha256 `
        -Value $ready.packageSha256 `
        -Label 'Tracked receipt ready package SHA-256' `
        -Code $Code
    $readyManifestSha = Require-PartisanFocusedSha256 `
        -Value $ready.manifestSha256 `
        -Label 'Tracked receipt ready manifest SHA-256' `
        -Code $Code
    if ((Require-PartisanFocusedInteger `
            -Value $ready.schemaVersion `
            -Label 'Tracked receipt candidate ready schema' `
            -Code $Code) -ne $script:CandidateReadySchema -or
        $manifestCandidateId -cne $CandidateId -or
        $readyCandidateId -cne $CandidateId -or
        $readyPackageSha -cne $packageSha -or
        $readyManifestSha -cne [string]$manifestInput.Sha256) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message 'A rejection receipt candidate identity differs from its tracked seals.'
    }
    return [pscustomobject][ordered]@{
        candidateId = $CandidateId
        packageSha256 = $packageSha
        manifestSha256 = [string]$manifestInput.Sha256
        readySha256 = [string]$readyInput.Sha256
    }
}

function Get-PartisanFocusedTrackedHistoricalAggregates {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [AllowEmptyCollection()]
        [Parameter(Mandatory = $true)][string[]]$HistoricalPaths,
        [Parameter(Mandatory = $true)][string]$CurrentCandidateId
    )

    $repository = Resolve-PartisanFocusedExistingDirectory `
        -Path $RepositoryRootPath `
        -Label 'Aggregate repository root'
    $head = Get-PartisanFocusedGitHead -RepositoryRootPath $repository
    $trackedResult = Invoke-PartisanFocusedGitText `
        -RepositoryRootPath $repository `
        -ArgumentList @(
            'ls-tree',
            '-r',
            '--name-only',
            $head,
            '--',
            $script:FocusedTrackedEvidencePrefix)
    $trackedRows = @($trackedResult.Output)
    if ($trackedResult.ExitCode -ne 0) {
        Throw-PartisanFocusedAggregate `
            -Code 'history_census_drift' `
            -Message 'Tracked focused aggregate history could not be enumerated.'
    }
    $canonicalPattern = '^' + [regex]::Escape(
        $script:FocusedTrackedEvidencePrefix) +
        '/([A-Za-z0-9][A-Za-z0-9._-]{0,127})\.json$'
    $receiptPattern = '^' + [regex]::Escape(
        $script:FocusedTrackedEvidencePrefix) +
        '/([A-Za-z0-9][A-Za-z0-9._-]{0,127})' +
        '\.json\.replacement-required\.json$'
    $trackedSet = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $trackedMain = New-Object Collections.Generic.List[object]
    $trackedReceipts = New-Object Collections.Generic.List[object]
    foreach ($trackedRow in @($trackedRows | Sort-Object -CaseSensitive)) {
        $relative = [string]$trackedRow
        if (-not $trackedSet.Add($relative)) {
            Throw-PartisanFocusedAggregate `
                -Code 'history_census_drift' `
                -Message 'Tracked focused aggregate history repeats a path.'
        }
        $mainMatch = [regex]::Match(
            $relative,
            $canonicalPattern,
            [Text.RegularExpressions.RegexOptions]::CultureInvariant)
        $receiptMatch = [regex]::Match(
            $relative,
            $receiptPattern,
            [Text.RegularExpressions.RegexOptions]::CultureInvariant)
        # Receipt names also satisfy the generic aggregate pattern because a
        # candidate ID may contain dots and hyphens. Give the longer,
        # contract-specific suffix deterministic precedence.
        $isReceipt = $receiptMatch.Success
        $isMain = $mainMatch.Success -and -not $isReceipt
        if (-not $isMain -and -not $isReceipt) {
            Throw-PartisanFocusedAggregate `
                -Code 'history_census_drift' `
                -Message 'Tracked focused aggregate history contains a noncanonical path.'
        }
        $full = Resolve-PartisanFocusedExistingFile `
            -Path (Join-Path $repository $relative.Replace('/', '\')) `
            -Label 'Tracked focused history file' `
            -Code 'historical_blob_replacement'
        Assert-PartisanFocusedNoReparseAncestry `
            -Root $repository `
            -Path $full `
            -Label 'Tracked focused history file'
        $input = Read-PartisanFocusedJson `
            -Path $full `
            -Label 'Tracked focused history file' `
            -Code 'historical_blob_replacement'
        Assert-PartisanFocusedHistoricalPathImmutable `
            -RepositoryRootPath $repository `
            -GitHead $head `
            -RepositoryPath $relative `
            -WorktreeBytes $input.Bytes
        if ($isMain) {
            Assert-PartisanFocusedProperties `
                -Value $input.Value `
                -Names @('schemaVersion', 'evidenceKind') `
                -Label 'Tracked focused aggregate candidate' `
                -Code 'historical_blob_replacement'
            $schema = Require-PartisanFocusedInteger `
                -Value $input.Value.schemaVersion `
                -Label 'Tracked focused aggregate candidate schema' `
                -Code 'historical_blob_replacement'
            if ($schema -notin @(1, 2) -or
                [string]$input.Value.evidenceKind -cne
                    $script:FocusedAggregateEvidenceKind) {
                Throw-PartisanFocusedAggregate `
                    -Code 'historical_blob_replacement' `
                    -Message 'Tracked focused aggregate history has an unsupported contract.'
            }
            [void]$trackedMain.Add([pscustomobject][ordered]@{
                Relative = $relative
                CandidateId = $mainMatch.Groups[1].Value
                Schema = $schema
                Input = $input
            })
        }
        else {
            [void]$trackedReceipts.Add([pscustomobject][ordered]@{
                Relative = $relative
                CandidateId = $receiptMatch.Groups[1].Value
                Input = $input
            })
        }
    }

    $expectedRows = @($trackedMain | Where-Object {
        $_.Schema -eq 2 -and $_.CandidateId -cne $CurrentCandidateId
    } | ForEach-Object { $_.Relative } | Sort-Object -CaseSensitive)
    $providedRows = New-Object Collections.Generic.List[string]
    $providedSeen = New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    foreach ($historicalPath in $HistoricalPaths) {
        $full = Resolve-PartisanFocusedExistingFile `
            -Path $historicalPath `
            -Label 'Historical focused aggregate' `
            -Code 'history_census_drift'
        Assert-PartisanFocusedNoReparseAncestry `
            -Root $repository `
            -Path $full `
            -Label 'Historical focused aggregate'
        $relative = Get-PartisanFocusedPortableRelativePath `
            -Root $repository `
            -Path $full `
            -Label 'Historical focused aggregate'
        $providedMatch = [regex]::Match(
            $relative,
            $canonicalPattern,
            [Text.RegularExpressions.RegexOptions]::CultureInvariant)
        $providedReceiptMatch = [regex]::Match(
            $relative,
            $receiptPattern,
            [Text.RegularExpressions.RegexOptions]::CultureInvariant)
        if (-not $providedMatch.Success -or
            $providedReceiptMatch.Success -or
            $providedMatch.Groups[1].Value -ceq $CurrentCandidateId -or
            -not $providedSeen.Add($relative)) {
            Throw-PartisanFocusedAggregate `
                -Code 'history_census_drift' `
                -Message 'A historical focused aggregate argument is noncanonical or duplicated.'
        }
        [void]$providedRows.Add($relative)
    }
    $providedSorted = @($providedRows | Sort-Object -CaseSensitive)
    if (($expectedRows -join "`n") -cne ($providedSorted -join "`n")) {
        Throw-PartisanFocusedAggregate `
            -Code 'history_census_drift' `
            -Message 'HistoricalAggregate arguments do not equal tracked schema-2 history.'
    }

    $historyRoot = Join-Path $repository $script:FocusedTrackedEvidencePrefix
    if (-not (Test-Path -LiteralPath $historyRoot -PathType Container)) {
        Throw-PartisanFocusedAggregate `
            -Code 'history_census_drift' `
            -Message 'The focused aggregate history directory is unavailable.'
    }
    Assert-PartisanFocusedNoReparseAncestry `
        -Root $repository `
        -Path $historyRoot `
        -Label 'Focused aggregate history directory'
    $worktreeReceipts = New-Object Collections.Generic.List[object]
    foreach ($item in @(Get-ChildItem -LiteralPath $historyRoot -Force -ErrorAction Stop)) {
        if ($item.PSIsContainer -or
            ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            Throw-PartisanFocusedAggregate `
                -Code 'history_census_drift' `
                -Message 'Focused aggregate history contains unsafe directory ancestry.'
        }
        $relative = Get-PartisanFocusedPortableRelativePath `
            -Root $repository `
            -Path $item.FullName `
            -Label 'Focused aggregate history file'
        if ($relative -cmatch (
                '^' + [regex]::Escape($script:FocusedTrackedEvidencePrefix) +
                '/\.[A-Za-z0-9][A-Za-z0-9._-]{0,127}' +
                '\.publication\.lock$')) {
            continue
        }
        if ($relative -cmatch (
                '^' + [regex]::Escape($script:FocusedTrackedEvidencePrefix) +
                '/\.[A-Za-z0-9][A-Za-z0-9._-]{0,127}\.json' +
                '(?:\.replacement-required\.json)?\.[0-9a-f]{32}\.tmp$')) {
            continue
        }
        $mainMatch = [regex]::Match(
            $relative,
            $canonicalPattern,
            [Text.RegularExpressions.RegexOptions]::CultureInvariant)
        $receiptMatch = [regex]::Match(
            $relative,
            $receiptPattern,
            [Text.RegularExpressions.RegexOptions]::CultureInvariant)
        $isReceipt = $receiptMatch.Success
        $isMain = $mainMatch.Success -and -not $isReceipt
        if (-not $isMain -and -not $isReceipt) {
            Throw-PartisanFocusedAggregate `
                -Code 'history_census_drift' `
                -Message 'Focused aggregate history contains a noncanonical worktree path.'
        }
        if ($isMain) {
            if (-not $trackedSet.Contains($relative) -and
                $mainMatch.Groups[1].Value -cne $CurrentCandidateId) {
                Throw-PartisanFocusedAggregate `
                    -Code 'history_census_drift' `
                    -Message 'Focused aggregate history contains an untracked noncurrent aggregate.'
            }
            if (-not $trackedSet.Contains($relative)) {
                $currentInput = Read-PartisanFocusedJson `
                    -Path $item.FullName `
                    -Label 'Current focused aggregate' `
                    -Code 'historical_blob_replacement'
                Assert-PartisanFocusedAggregateValue `
                    -Value $currentInput.Value `
                    -Label 'Current focused aggregate' `
                    -Code 'historical_blob_replacement' `
                    -ExpectedCandidateId $CurrentCandidateId
            }
            continue
        }
        $receiptInput = if ($trackedSet.Contains($relative)) {
            @($trackedReceipts | Where-Object {
                $_.Relative -ceq $relative
            })[0].Input
        }
        else {
            Read-PartisanFocusedJson `
                -Path $item.FullName `
                -Label 'Focused rejection receipt' `
                -Code 'historical_blob_replacement'
        }
        $receiptCandidate = Get-PartisanFocusedTrackedReceiptCandidate `
            -RepositoryRootPath $repository `
            -GitHead $head `
            -CandidateId $receiptMatch.Groups[1].Value `
            -Code 'historical_blob_replacement'
        Assert-PartisanFocusedRejectionReceiptValue `
            -Value $receiptInput.Value `
            -Label 'Focused rejection receipt' `
            -ExpectedCandidate $receiptCandidate `
            -Code 'historical_blob_replacement'
        [void]$worktreeReceipts.Add([pscustomobject][ordered]@{
            FullPath = $item.FullName
            CandidateId = $receiptMatch.Groups[1].Value
            Candidate = $receiptCandidate
        })
    }
    foreach ($trackedRow in $trackedRows) {
        if (-not (Test-Path -LiteralPath (
                Join-Path $repository ([string]$trackedRow).Replace('/', '\')) `
                -PathType Leaf)) {
            Throw-PartisanFocusedAggregate `
                -Code 'history_census_drift' `
                -Message 'A tracked focused history path is absent from the worktree.'
        }
    }
    foreach ($receiptRow in $worktreeReceipts) {
        Assert-PartisanFocusedRedPrecedence `
            -ReceiptPath $receiptRow.FullPath `
            -AggregatePath ($receiptRow.FullPath.Substring(
                0,
                $receiptRow.FullPath.Length -
                    '.replacement-required.json'.Length)) `
            -RepositoryRootPath $repository `
            -ExpectedCandidate $receiptRow.Candidate
    }

    $results = New-Object Collections.Generic.List[object]
    $candidateIds = New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    $aggregateIds = New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    foreach ($trackedAggregate in $trackedMain) {
        if ($trackedAggregate.Schema -ne 2) {
            continue
        }
        $input = $trackedAggregate.Input
        $fileCandidateId = $trackedAggregate.CandidateId
        Assert-PartisanFocusedAggregateValue `
            -Value $input.Value `
            -Label 'Tracked historical focused aggregate' `
            -Code 'historical_blob_replacement' `
            -ExpectedCandidateId $fileCandidateId `
            -AllowTrackedLegacyAggregate
        Assert-PartisanFocusedHistoricalProvenance `
            -RepositoryRootPath $repository `
            -Aggregate $input.Value `
            -CurrentHead $head
        if (-not $candidateIds.Add([string]$input.Value.candidate.candidateId) -or
            -not $aggregateIds.Add([string]$input.Value.aggregateId)) {
            Throw-PartisanFocusedAggregate `
                -Code 'historical_blob_replacement' `
                -Message 'Tracked focused aggregate history repeats an identity.'
        }
        if ($fileCandidateId -cne $CurrentCandidateId) {
            [void]$results.Add($input.Value)
        }
    }
    return $results.ToArray()
}

function Get-PartisanFocusedCandidateTreeCensus {
    param(
        [Parameter(Mandatory = $true)][string]$EvidenceRootPath,
        [Parameter(Mandatory = $true)][string]$CandidateId,
        [Parameter(Mandatory = $true)][object[]]$Bindings,
        [string]$Code = 'focused_tree_census_drift'
    )

    if (-not (Test-PartisanFocusedCandidateId -Value $CandidateId) -or
        $Bindings.Count -ne 5) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message 'The focused candidate-tree census lacks one canonical five-run identity.'
    }
    $evidenceRoot = [IO.Path]::GetFullPath($EvidenceRootPath)
    $focusedRoot = [IO.Path]::GetFullPath((Join-Path `
        (Join-Path $evidenceRoot $CandidateId) `
        'focused-autotest'))
    if (-not (Test-PartisanFocusedContainedPath `
            -Root $evidenceRoot `
            -Path $focusedRoot) -or
        -not (Test-Path -LiteralPath $focusedRoot -PathType Container)) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message 'The current candidate focused evidence tree is missing or escaped.'
    }
    Assert-PartisanFocusedNoReparseAncestry `
        -Root $evidenceRoot `
        -Path $focusedRoot `
        -Label 'Current candidate focused evidence tree'

    $expectedFiles = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $expectedDirectories = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $expectedRunPaths = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $addExpectedFile = {
        param([string]$FullPath, [switch]$RunEnvelope)

        $full = [IO.Path]::GetFullPath($FullPath)
        if (-not (Test-PartisanFocusedContainedPath `
                -Root $focusedRoot `
                -Path $full)) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message 'An expected focused-tree file escaped the current candidate.'
        }
        $relative = Get-PartisanFocusedPortableRelativePath `
            -Root $focusedRoot `
            -Path $full `
            -Label 'Expected focused-tree file'
        if (-not $expectedFiles.Add($relative)) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message 'The focused-tree contract repeats an expected file.'
        }
        if ($RunEnvelope) {
            [void]$expectedRunPaths.Add($relative)
        }
        $parent = Split-Path -Parent $full
        while (-not $parent.Equals(
                $focusedRoot,
                [StringComparison]::OrdinalIgnoreCase)) {
            if (-not (Test-PartisanFocusedContainedPath `
                    -Root $focusedRoot `
                    -Path $parent)) {
                Throw-PartisanFocusedAggregate `
                    -Code $Code `
                    -Message 'An expected focused-tree directory escaped the current candidate.'
            }
            [void]$expectedDirectories.Add((
                Get-PartisanFocusedPortableRelativePath `
                    -Root $focusedRoot `
                    -Path $parent `
                    -Label 'Expected focused-tree directory'))
            $nextParent = Split-Path -Parent $parent
            if ($nextParent.Equals(
                    $parent,
                    [StringComparison]::OrdinalIgnoreCase)) {
                Throw-PartisanFocusedAggregate `
                    -Code $Code `
                    -Message 'The focused-tree directory contract did not converge.'
            }
            $parent = $nextParent
        }
    }

    $indexedBlobCount = 0
    foreach ($binding in $Bindings) {
        $envelopeFull = [IO.Path]::GetFullPath((Join-Path `
            $evidenceRoot `
            ([string]$binding.EnvelopePath).Replace('/', '\')))
        & $addExpectedFile -FullPath $envelopeFull -RunEnvelope
        $runRoot = Split-Path -Parent $envelopeFull
        foreach ($row in @($binding.Files)) {
            $indexedBlobCount++
            & $addExpectedFile -FullPath ([IO.Path]::GetFullPath((Join-Path `
                $runRoot `
                ([string]$row.path).Replace('/', '\'))))
        }
    }

    $actualFiles = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $actualDirectories = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($item in @(Get-ChildItem `
            -LiteralPath $focusedRoot `
            -Recurse `
            -Force `
            -ErrorAction Stop)) {
        Assert-PartisanFocusedNoReparseAncestry `
            -Root $focusedRoot `
            -Path $item.FullName `
            -Label 'Current candidate focused-tree entry'
        $relative = Get-PartisanFocusedPortableRelativePath `
            -Root $focusedRoot `
            -Path $item.FullName `
            -Label 'Current candidate focused-tree entry'
        if ($item.PSIsContainer) {
            if (-not $actualDirectories.Add($relative)) {
                Throw-PartisanFocusedAggregate `
                    -Code $Code `
                    -Message 'The focused-tree census repeats a directory.'
            }
        }
        elseif (-not $actualFiles.Add($relative)) {
            Throw-PartisanFocusedAggregate `
                -Code $Code `
                -Message 'The focused-tree census repeats a file.'
        }
    }
    $actualRunPaths = @($actualFiles | Where-Object {
        $_ -cmatch '(^|/)run\.json$'
    } | Sort-Object -CaseSensitive)
    $unselectedRunCount = @(Compare-Object `
        -ReferenceObject @($expectedRunPaths | Sort-Object -CaseSensitive) `
        -DifferenceObject $actualRunPaths `
        -CaseSensitive | Where-Object { $_.SideIndicator -ceq '=>' }).Count
    if ($expectedFiles.Count -ne 45 -or
        $indexedBlobCount -ne 40 -or
        $expectedRunPaths.Count -ne 5 -or
        $actualRunPaths.Count -ne 5 -or
        @(Compare-Object `
            -ReferenceObject @($expectedFiles | Sort-Object -CaseSensitive) `
            -DifferenceObject @($actualFiles | Sort-Object -CaseSensitive) `
            -CaseSensitive).Count -ne 0 -or
        @(Compare-Object `
            -ReferenceObject @($expectedDirectories | Sort-Object -CaseSensitive) `
            -DifferenceObject @($actualDirectories | Sort-Object -CaseSensitive) `
            -CaseSensitive).Count -ne 0) {
        Throw-PartisanFocusedAggregate `
            -Code $Code `
            -Message 'The current candidate focused tree is not exactly five runs and 40 indexed blobs.'
    }
    return [pscustomobject][ordered]@{
        runEnvelopeCount = $expectedRunPaths.Count
        indexedBlobCount = $indexedBlobCount
        totalFileCount = $expectedFiles.Count
        directoryCount = $expectedDirectories.Count
        preliminaryCaseCount = $unselectedRunCount
        preliminaryStatus = if ($unselectedRunCount -eq 0) { 'none' } else { 'present' }
    }
}

function New-PartisanFocusedAggregateValue {
    param(
        [Parameter(Mandatory = $true)][string]$EvidenceRootPath,
        [Parameter(Mandatory = $true)][string[]]$RunPaths,
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath
    )

    if ($RunPaths.Count -ne 5) {
        Throw-PartisanFocusedAggregate `
            -Code 'profile_set_invalid' `
            -Message 'Exactly five focused run.json inputs are required.'
    }
    $resolvedSeen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::OrdinalIgnoreCase)
    $bindings = New-Object Collections.Generic.List[object]
    foreach ($runPath in $RunPaths) {
        $resolved = [IO.Path]::GetFullPath($runPath)
        if (-not $resolvedSeen.Add($resolved)) {
            Throw-PartisanFocusedAggregate `
                -Code 'profile_set_invalid' `
                -Message 'A focused run.json input was supplied more than once.'
        }
        [void]$bindings.Add((Get-PartisanFocusedRunBinding `
            -EvidenceRootPath $EvidenceRootPath `
            -RunPath $resolved `
            -RepositoryRootPath $RepositoryRootPath))
    }

    $profileSeen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($binding in $bindings) {
        if (-not $profileSeen.Add([string]$binding.TestCase)) {
            Throw-PartisanFocusedAggregate `
                -Code 'profile_set_invalid' `
                -Message 'A canonical focused profile was supplied more than once.'
        }
    }
    if (@(Compare-Object `
            -ReferenceObject @($script:FocusedProfileOrder | Sort-Object) `
            -DifferenceObject @($profileSeen | Sort-Object) `
            -CaseSensitive).Count -ne 0) {
        Throw-PartisanFocusedAggregate `
            -Code 'profile_set_invalid' `
            -Message 'The exact canonical five-profile set was not supplied.'
    }

    $orderedBindings = @($script:FocusedProfileOrder | ForEach-Object {
        $expected = $_
        @($bindings | Where-Object { $_.TestCase -ceq $expected })[0]
    })
    $referenceCandidate = $orderedBindings[0].Candidate
    $referenceCandidatePublic = $orderedBindings[0].CandidatePublic
    $referenceHarness = $orderedBindings[0].Harness
    $referenceToolchain = $orderedBindings[0].Toolchain
    $classifierChecks = [long]$orderedBindings[0].ClassifierChecks
    $focusedTreeCensus = Get-PartisanFocusedCandidateTreeCensus `
        -EvidenceRootPath $EvidenceRootPath `
        -CandidateId ([string]$referenceCandidatePublic.candidateId) `
        -Bindings @($orderedBindings)
    $previousCompleted = $null
    foreach ($binding in $orderedBindings) {
        Assert-PartisanFocusedSameValue `
            -Expected $referenceCandidate `
            -Actual $binding.Candidate `
            -Label 'Focused aggregate candidate identity'
        Assert-PartisanFocusedSameValue `
            -Expected $referenceCandidatePublic `
            -Actual $binding.CandidatePublic `
            -Label 'Focused aggregate public candidate identity'
        Assert-PartisanFocusedSameValue `
            -Expected $referenceHarness `
            -Actual $binding.Harness `
            -Label 'Focused aggregate harness identity' `
            -Code 'harness_identity_drift'
        Assert-PartisanFocusedSameValue `
            -Expected $referenceToolchain `
            -Actual $binding.Toolchain `
            -Label 'Focused aggregate toolchain identity' `
            -Code 'tool_identity_drift'
        if ([long]$binding.ClassifierChecks -ne
                $script:FocusedHardDiagnosticClassifierChecks -or
            [long]$binding.ClassifierChecks -ne $classifierChecks) {
            Throw-PartisanFocusedAggregate `
                -Code 'policy_drift' `
                -Message 'Focused classifier policy changed within the aggregate.'
        }
        if ($null -ne $previousCompleted -and
            $binding.StartedUtc -lt $previousCompleted) {
            Throw-PartisanFocusedAggregate `
                -Code 'policy_drift' `
                -Message 'Focused run windows overlap or do not follow canonical order.'
        }
        $previousCompleted = $binding.CompletedUtc
    }
    $integrity = Get-PartisanFocusedAggregationIntegrity `
        -RepositoryRootPath $RepositoryRootPath `
        -RunHarness $referenceHarness `
        -CandidateSourceHead ([string]$referenceCandidatePublic.gitHead)

    $envelopeHashes = @($orderedBindings | ForEach-Object {
        $_.EnvelopeSha256
    })
    $runIds = @($orderedBindings | ForEach-Object { $_.RunId })
    if (@($envelopeHashes | Sort-Object -Unique).Count -ne 5 -or
        @($runIds | Sort-Object -Unique).Count -ne 5) {
        Throw-PartisanFocusedAggregate `
            -Code 'index_tampering' `
            -Message 'Focused envelope hashes and run IDs must be unique.'
    }

    $hardDiagnosticCount = [long](($orderedBindings | Measure-Object `
        -Property HardDiagnosticCount `
        -Sum).Sum)
    $stockDiagnosticCount = [long](($orderedBindings | Measure-Object `
        -Property StockDiagnosticCount `
        -Sum).Sum)
    $intentionalDiagnosticCount = [long](($orderedBindings | Measure-Object `
        -Property IntentionalDiagnosticCount `
        -Sum).Sum)
    $unapprovedDiagnosticCount = [long](($orderedBindings | Measure-Object `
        -Property UnapprovedDiagnosticCount `
        -Sum).Sum)
    $assertions = @($orderedBindings | ForEach-Object { @($_.Assertions) })
    if ($assertions.Count -ne 35 -or
        @($assertions | Where-Object { $_.status -ceq 'PASS' }).Count -ne 35 -or
        @($assertions.assertionId | Sort-Object -Unique).Count -ne 35) {
        Throw-PartisanFocusedAggregate `
            -Code 'policy_drift' `
            -Message 'The focused aggregate policy does not prove exactly 35 of 35 checks.'
    }
    $profileCount = $orderedBindings.Count
    $totalFileCount = [long](($orderedBindings | ForEach-Object {
        @($_.Files).Count
    } | Measure-Object -Sum).Sum)
    if ($profileCount -ne 5 -or $totalFileCount -ne 40) {
        Throw-PartisanFocusedAggregate `
            -Code 'index_tampering' `
            -Message 'The focused aggregate does not retain exactly five times eight blobs.'
    }

    $cases = @($orderedBindings | ForEach-Object {
        [pscustomobject][ordered]@{
            testCase = $_.TestCase
            suiteClass = $_.SuiteClass
            runId = $_.RunId
            startedUtc = $_.StartedUtcText
            completedUtc = $_.CompletedUtcText
            envelopeSha256 = $_.EnvelopeSha256
            fileCount = @($_.Files).Count
            success = $true
            candidateBoundaryVerified = $true
            mountPacked = $true
            junitTests = [long]$_.JUnit.Tests
            junitFailures = [long]$_.JUnit.Failures
            junitErrors = [long]$_.JUnit.Errors
            junitSkipped = [long]$_.JUnit.Skipped
            hardDiagnosticClassifierChecks = [long]$_.ClassifierChecks
            hardDiagnosticClassificationValid = $true
            hardDiagnosticFree = [bool]$_.HardDiagnosticFree
            hardDiagnosticCount = [long]$_.HardDiagnosticCount
            approvedStockFilterDiagnosticCount = [long]$_.StockDiagnosticCount
            approvedIntentionalFaultDiagnosticCount =
                [long]$_.IntentionalDiagnosticCount
            unapprovedHardDiagnosticCount = [long]$_.UnapprovedDiagnosticCount
            cleanupAndSpillZero = $true
            envelopeFilesRehashed = $true
        }
    })
    $sourceRuns = @($orderedBindings | ForEach-Object {
        [pscustomobject][ordered]@{
            testCase = $_.TestCase
            suiteClass = $_.SuiteClass
            runId = $_.RunId
            runEnvelopePath = $_.EnvelopePath
            runEnvelopeSha256 = $_.EnvelopeSha256
            fileCount = @($_.Files).Count
            files = @($_.Files)
        }
    })
    $aggregateId = Get-PartisanFocusedAggregateId `
        -Integrity $integrity `
        -SourceRuns $sourceRuns

    $aggregate = [pscustomobject][ordered]@{
        schemaVersion = $script:FocusedAggregateSchema
        evidenceKind = $script:FocusedAggregateEvidenceKind
        aggregateId = $aggregateId
        aggregationPolicy = [pscustomobject][ordered]@{
            contractId = $script:FocusedAggregateContractId
            policyVersion = 2
            inputSchemaVersion = $script:FocusedRunSchema
            inputEvidenceKind = $script:FocusedRunEvidenceKind
            requiredRuntimeUseDisposition = $script:FocusedRunDisposition
            requiredCandidateState = $script:FocusedCandidateState
            profileOrder = @($script:FocusedProfileOrder)
            profileCount = $profileCount
            filesPerProfile = @($orderedBindings[0].Files).Count
            totalFileCount = $totalFileCount
            rawReopenRequired = $true
            exactDirectoryCensusRequired = $true
            candidateSealReopenRequired = $true
            historicalBlobImmutabilityRequired = $true
            aggregatePolicyAssertionsPerProfile =
                @($orderedBindings[0].Assertions).Count
            aggregatePolicyAssertionCount = $assertions.Count
            acceptedResultStatus = $script:FocusedAggregateResultStatus
            acceptedDisposition = $script:FocusedAcceptedDisposition
            rejectionDisposition = $script:FocusedReplacementDisposition
        }
        admission = [pscustomobject][ordered]@{
            status = 'accepted'
            disposition = $script:FocusedAcceptedDisposition
            releaseDecision = 'NO-GO'
            certifying = $false
        }
        candidate = $referenceCandidate
        harness = [pscustomobject][ordered]@{
            gitHead = $referenceHarness.gitHead
            dirty = $referenceHarness.dirty
            focusedRunnerSha256 = $referenceHarness.focusedRunnerSha256
            candidateModuleSha256 = $referenceHarness.candidateModuleSha256
            gitBlobProvenanceVerified = $true
        }
        integrity = $integrity
        acceptedWindow = [pscustomobject][ordered]@{
            startedUtc = $orderedBindings[0].StartedUtcText
            completedUtc = $orderedBindings[4].CompletedUtcText
        }
        result = [pscustomobject][ordered]@{
            status = $script:FocusedAggregateResultStatus
            caseCount = $cases.Count
            passedCases = @($cases | Where-Object { $_.success }).Count
            junitTests = [long](($cases | Measure-Object `
                -Property junitTests -Sum).Sum)
            junitFailures = [long](($cases | Measure-Object `
                -Property junitFailures -Sum).Sum)
            junitErrors = [long](($cases | Measure-Object `
                -Property junitErrors -Sum).Sum)
            junitSkipped = [long](($cases | Measure-Object `
                -Property junitSkipped -Sum).Sum)
            candidateBoundaryVerified = $true
            allMountsPacked = $true
            allCleanupAndSpillZero = $true
            allEnvelopeFilesRehashed = $true
            envelopeFileCount = $totalFileCount
            hardDiagnosticClassifierChecksPerRun = $classifierChecks
            hardDiagnosticClassificationValid = $true
            hardDiagnosticFree = $hardDiagnosticCount -eq 0
            hardDiagnosticCount = $hardDiagnosticCount
            approvedStockFilterDiagnosticCount = $stockDiagnosticCount
            approvedIntentionalFaultDiagnosticCount =
                $intentionalDiagnosticCount
            unapprovedHardDiagnosticCount = $unapprovedDiagnosticCount
            scope = 'Five exact-package focused service suites passed a non-certifying deterministic-service gate. Aggregate-policy assertions are not Campaign Debug assertions; broader runtime and release gates remain separate.'
        }
        aggregatePolicyAssertions = [pscustomobject][ordered]@{
            assertionClass = 'aggregate-policy'
            scope = 'Exactly seven fail-closed aggregate contract checks per canonical profile; these 35 checks are distinct from Campaign Debug force-authority assertions.'
            total = $assertions.Count
            passed = @($assertions | Where-Object { $_.status -ceq 'PASS' }).Count
            failed = @($assertions | Where-Object { $_.status -cne 'PASS' }).Count
            assertions = $assertions
        }
        cases = $cases
        sourceRuns = $sourceRuns
        preliminaryRuns = [pscustomobject][ordered]@{
            caseCount = [long]$focusedTreeCensus.preliminaryCaseCount
            status = [string]$focusedTreeCensus.preliminaryStatus
            note = 'The exact current-candidate focused-tree census contains only the admitted five-run set; no preliminary or superseded sibling run exists.'
        }
    }

    $portableText = ConvertTo-PartisanFocusedCanonicalJson -Value $aggregate
    if ($portableText -match
        '(?i)(?:[A-Z]:[\\/]|\\\\|file://|/(?:Users|home|mnt)/)') {
        Throw-PartisanFocusedAggregate `
            -Code 'path_escape' `
            -Message 'The focused aggregate contains a nonportable path.'
    }
    Assert-PartisanFocusedAggregateValue `
        -Value $aggregate `
        -Label 'Focused aggregate'
    return $aggregate
}

function Get-PartisanFocusedAuthenticatedRejectionCandidate {
    param(
        [Parameter(Mandatory = $true)][string]$EvidenceRootPath,
        [Parameter(Mandatory = $true)][string[]]$RunPaths,
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath
    )

    if ($RunPaths.Count -ne 5) {
        Throw-PartisanFocusedAggregate `
            -Code 'red_candidate_unavailable' `
            -Message 'A durable rejection receipt requires five candidate-bound runs.'
    }
    $evidenceRoot = Resolve-PartisanFocusedExistingDirectory `
        -Path $EvidenceRootPath `
        -Label 'Rejection evidence root'
    $repository = Resolve-PartisanFocusedExistingDirectory `
        -Path $RepositoryRootPath `
        -Label 'Rejection repository root'
    $resolvedSeen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::OrdinalIgnoreCase)
    $profiles = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $bindings = New-Object Collections.Generic.List[object]
    $envelopeHashes = New-Object Collections.Generic.List[string]

    foreach ($runPath in $RunPaths) {
        $resolvedRun = Resolve-PartisanFocusedExistingFile `
            -Path $runPath `
            -Label 'Rejection focused run' `
            -Code 'red_candidate_unavailable'
        if ((Split-Path -Leaf $resolvedRun) -cne 'run.json' -or
            -not (Test-PartisanFocusedContainedPath `
                -Root $evidenceRoot `
                -Path $resolvedRun) -or
            -not $resolvedSeen.Add($resolvedRun)) {
            Throw-PartisanFocusedAggregate `
                -Code 'red_candidate_unavailable' `
                -Message 'A durable rejection run is noncanonical, escaped, or duplicated.'
        }
        Assert-PartisanFocusedNoReparseAncestry `
            -Root $evidenceRoot `
            -Path $resolvedRun `
            -Label 'Rejection focused run'
        $runInput = Read-PartisanFocusedJson `
            -Path $resolvedRun `
            -Label 'Rejection focused run' `
            -Code 'red_candidate_unavailable'
        $run = $runInput.Value
        Assert-PartisanFocusedProperties `
            -Value $run `
            -Names @(
                'schemaVersion', 'evidenceKind', 'startedUtc', 'candidate',
                'launch', 'outcome', 'files') `
            -Label 'Rejection focused run identity' `
            -Code 'red_candidate_unavailable'
        Assert-PartisanFocusedArray `
            -Value $run.files `
            -Label 'Rejection focused run file index' `
            -Code 'red_candidate_unavailable'
        if ((Require-PartisanFocusedInteger `
                -Value $run.schemaVersion `
                -Label 'Rejection focused run schema' `
                -Code 'red_candidate_unavailable') -ne $script:FocusedRunSchema -or
            [string]$run.evidenceKind -cne $script:FocusedRunEvidenceKind) {
            Throw-PartisanFocusedAggregate `
                -Code 'red_candidate_unavailable' `
                -Message 'A durable rejection run does not identify the focused contract.'
        }
        $startedUtcText = Require-PartisanFocusedText `
            -Value $run.startedUtc `
            -Label 'Rejection focused run start' `
            -Code 'red_candidate_unavailable'
        Require-PartisanFocusedUtc `
            -Value $startedUtcText `
            -Label 'Rejection focused run start' `
            -Code 'red_candidate_unavailable' | Out-Null
        $profile = Require-PartisanFocusedText `
            -Value $run.launch.testCase `
            -Label 'Rejection focused profile' `
            -Code 'red_candidate_unavailable'
        if ($script:FocusedProfileOrder -cnotcontains $profile -or
            -not $profiles.Add($profile)) {
            Throw-PartisanFocusedAggregate `
                -Code 'red_candidate_unavailable' `
                -Message 'Durable rejection runs do not identify five unique profiles.'
        }

        $identityRows = New-Object Collections.Generic.List[object]
        foreach ($identityPath in @(
                'identity/candidate.json',
                'identity/candidate.ready.json')) {
            $matches = @($run.files | Where-Object {
                [string]$_.path -ceq $identityPath
            })
            if ($matches.Count -ne 1) {
                Throw-PartisanFocusedAggregate `
                    -Code 'red_candidate_unavailable' `
                    -Message 'A durable rejection run has an ambiguous candidate seal.'
            }
            $row = $matches[0]
            Assert-PartisanFocusedProperties `
                -Value $row `
                -Names @('path', 'length', 'sha256') `
                -Label 'Rejection candidate seal row' `
                -Exact `
                -Code 'red_candidate_unavailable'
            $length = Require-PartisanFocusedInteger `
                -Value $row.length `
                -Label 'Rejection candidate seal length' `
                -Code 'red_candidate_unavailable'
            $sha = Require-PartisanFocusedSha256 `
                -Value $row.sha256 `
                -Label 'Rejection candidate seal SHA-256' `
                -Code 'red_candidate_unavailable'
            $full = [IO.Path]::GetFullPath((Join-Path `
                (Split-Path -Parent $resolvedRun) `
                $identityPath.Replace('/', '\')))
            if (-not (Test-PartisanFocusedContainedPath `
                    -Root (Split-Path -Parent $resolvedRun) `
                    -Path $full)) {
                Throw-PartisanFocusedAggregate `
                    -Code 'red_candidate_unavailable' `
                    -Message 'A rejection candidate seal escaped its run root.'
            }
            Assert-PartisanFocusedNoReparseAncestry `
                -Root (Split-Path -Parent $resolvedRun) `
                -Path $full `
                -Label 'Rejection candidate seal'
            $sealInput = Read-PartisanFocusedUtf8Text `
                -Path $full `
                -Label 'Rejection candidate seal' `
                -Code 'red_candidate_unavailable'
            if ($length -ne [long]$sealInput.Length -or
                $sha -cne [string]$sealInput.Sha256) {
                Throw-PartisanFocusedAggregate `
                    -Code 'red_candidate_unavailable' `
                    -Message 'A rejection candidate seal differs from its run index.'
            }
            [void]$identityRows.Add([pscustomobject][ordered]@{
                path = $identityPath
                length = $length
                sha256 = $sha
                fullPath = $full
            })
        }
        $binding = Get-PartisanFocusedCandidateBinding `
            -Run $run `
            -FileRows $identityRows.ToArray() `
            -StartedUtcText $startedUtcText `
            -RepositoryRootPath $repository
        [void]$bindings.Add($binding)
        [void]$envelopeHashes.Add([string]$runInput.Sha256)
    }
    if (@(Compare-Object `
            -ReferenceObject @($script:FocusedProfileOrder | Sort-Object) `
            -DifferenceObject @($profiles | Sort-Object) `
            -CaseSensitive).Count -ne 0) {
        Throw-PartisanFocusedAggregate `
            -Code 'red_candidate_unavailable' `
            -Message 'Durable rejection runs do not identify the canonical profile set.'
    }
    $reference = $bindings[0]
    foreach ($binding in $bindings) {
        Assert-PartisanFocusedSameValue `
            -Expected $reference.PublicIdentity `
            -Actual $binding.PublicIdentity `
            -Label 'Rejection public candidate identity' `
            -Code 'red_candidate_unavailable'
        Assert-PartisanFocusedSameValue `
            -Expected $reference.SummaryIdentity `
            -Actual $binding.SummaryIdentity `
            -Label 'Rejection summary candidate identity' `
            -Code 'red_candidate_unavailable'
    }
    if (@($envelopeHashes | Sort-Object -Unique).Count -ne 5) {
        Throw-PartisanFocusedAggregate `
            -Code 'red_candidate_unavailable' `
            -Message 'Durable rejection runs repeat an envelope identity.'
    }
    return [pscustomobject][ordered]@{
        Candidate = [pscustomobject][ordered]@{
            candidateId = [string]$reference.SummaryIdentity.candidateId
            packageSha256 = [string]$reference.SummaryIdentity.packageSha256
            manifestSha256 = [string]$reference.SummaryIdentity.manifestSha256
            readySha256 = [string]$reference.SummaryIdentity.readySha256
        }
        RunEnvelopeSha256 = @($envelopeHashes | Sort-Object -CaseSensitive)
    }
}

function New-PartisanFocusedRejectionReceipt {
    param(
        [Parameter(Mandatory = $true)][string]$ReasonCode,
        [Parameter(Mandatory = $true)]$Candidate,
        [Parameter(Mandatory = $true)][string[]]$RunEnvelopeSha256,
        [AllowEmptyString()][string]$AcceptedAggregateSha256 = ''
    )

    $acceptedBefore = -not [string]::IsNullOrEmpty($AcceptedAggregateSha256)
    $receipt = [pscustomobject][ordered]@{
        schemaVersion = $script:FocusedAggregateSchema
        evidenceKind = $script:FocusedAggregateRejectionKind
        aggregationPolicy = [pscustomobject][ordered]@{
            contractId = $script:FocusedAggregateContractId
            policyVersion = 2
            requiredProfileCount = 5
            requiredFilesPerProfile = 8
            requiredTotalFileCount = 40
        }
        admission = [pscustomobject][ordered]@{
            status = 'rejected'
            disposition = $script:FocusedReplacementDisposition
            releaseDecision = 'RED'
            certifying = $false
            reasonCode = $ReasonCode
        }
        candidate = [pscustomobject][ordered]@{
            candidateId = [string]$Candidate.candidateId
            packageSha256 = [string]$Candidate.packageSha256
            manifestSha256 = [string]$Candidate.manifestSha256
            readySha256 = [string]$Candidate.readySha256
        }
        precedence = [pscustomobject][ordered]@{
            acceptedAggregatePresentBeforeRejection = $acceptedBefore
            acceptedAggregateSha256 = if ($acceptedBefore) {
                $AcceptedAggregateSha256
            }
            else {
                $null
            }
        }
        attemptedInput = [pscustomobject][ordered]@{
            runEnvelopeCount = $RunEnvelopeSha256.Count
            availableRunEnvelopeSha256 = @($RunEnvelopeSha256 | Sort-Object -CaseSensitive)
        }
    }
    Assert-PartisanFocusedRejectionReceiptValue `
        -Value $receipt `
        -Label 'Focused rejection receipt' `
        -ExpectedCandidate $Candidate `
        -Code 'red_candidate_unavailable'
    return $receipt
}

function Get-PartisanFocusedRejectionReceiptPath {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)][string]$OutputPath,
        [Parameter(Mandatory = $true)][string]$CandidateId
    )

    $repository = Resolve-PartisanFocusedExistingDirectory `
        -Path $RepositoryRootPath `
        -Label 'Aggregate repository root'
    $outputFull = [IO.Path]::GetFullPath($OutputPath)
    if (-not (Test-PartisanFocusedContainedPath `
            -Root $repository `
            -Path $outputFull)) {
        Throw-PartisanFocusedAggregate `
            -Code 'output_path_invalid' `
            -Message 'A rejection receipt cannot escape the aggregate repository.'
    }
    $relative = Get-PartisanFocusedPortableRelativePath `
        -Root $repository `
        -Path $outputFull `
        -Label 'Aggregate output path'
    $canonicalRelative = $script:FocusedTrackedEvidencePrefix + '/' +
        $CandidateId + '.json'
    if (-not (Test-PartisanFocusedCandidateId -Value $CandidateId) -or
        $relative -cne $canonicalRelative) {
        Throw-PartisanFocusedAggregate `
            -Code 'output_path_invalid' `
            -Message 'A rejection receipt requires a canonical aggregate output path.'
    }
    return $outputFull + '.replacement-required.json'
}

function Get-PartisanFocusedAcceptedAggregateSha256 {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)]$ExpectedCandidate,
        [switch]$AllowMissing
    )

    $full = [IO.Path]::GetFullPath($Path)
    if (-not (Test-Path -LiteralPath $full)) {
        if ($AllowMissing) {
            return ''
        }
        Throw-PartisanFocusedAggregate `
            -Code 'historical_blob_replacement' `
            -Message 'The preceding accepted aggregate is unavailable.'
    }
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
        Throw-PartisanFocusedAggregate `
            -Code 'historical_blob_replacement' `
            -Message 'The accepted aggregate path is not a regular file.'
    }
    Assert-PartisanFocusedNoReparseAncestry `
        -Root $RepositoryRootPath `
        -Path $full `
        -Label 'Accepted focused aggregate'
    $input = Read-PartisanFocusedJson `
        -Path $full `
        -Label 'Accepted focused aggregate' `
        -Code 'historical_blob_replacement'
    Assert-PartisanFocusedAggregateValue `
        -Value $input.Value `
        -Label 'Accepted focused aggregate' `
        -Code 'historical_blob_replacement' `
        -ExpectedCandidateId ([string]$ExpectedCandidate.candidateId)
    foreach ($sealName in @(
            'packageSha256',
            'manifestSha256',
            'readySha256')) {
        if ([string]$input.Value.candidate.$sealName -cne
            [string]$ExpectedCandidate.$sealName) {
            Throw-PartisanFocusedAggregate `
                -Code 'historical_blob_replacement' `
                -Message 'The accepted aggregate differs from the exact candidate seal identity.'
        }
    }
    return [string]$input.Sha256
}

function Assert-PartisanFocusedRedPrecedence {
    param(
        [Parameter(Mandatory = $true)][string]$ReceiptPath,
        [Parameter(Mandatory = $true)][string]$AggregatePath,
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)]$ExpectedCandidate,
        [switch]$EnforceFirstPublication
    )

    if (-not (Test-Path -LiteralPath $ReceiptPath)) {
        return
    }
    Assert-PartisanFocusedNoReparseAncestry `
        -Root $RepositoryRootPath `
        -Path $ReceiptPath `
        -Label 'Focused rejection receipt'
    $receipt = Read-PartisanFocusedJson `
        -Path $ReceiptPath `
        -Label 'Focused rejection receipt' `
        -Code 'historical_blob_replacement'
    Assert-PartisanFocusedRejectionReceiptValue `
        -Value $receipt.Value `
        -Label 'Focused rejection receipt' `
        -ExpectedCandidate $ExpectedCandidate `
        -Code 'historical_blob_replacement'
    $candidateId = [string]$ExpectedCandidate.candidateId
    if (-not [bool]$receipt.Value.precedence.acceptedAggregatePresentBeforeRejection) {
        if (Test-Path -LiteralPath $AggregatePath) {
            Throw-PartisanFocusedAggregate `
                -Code 'historical_blob_replacement' `
                -Message 'An accepted aggregate was published after a first-published RED receipt.'
        }
        if ($EnforceFirstPublication) {
            Throw-PartisanFocusedAggregate `
                -Code 'replacement_required' `
                -Message 'A first-published RED receipt blocks later aggregate acceptance.'
        }
        return
    }
    $acceptedSha = Get-PartisanFocusedAcceptedAggregateSha256 `
        -Path $AggregatePath `
        -RepositoryRootPath $RepositoryRootPath `
        -ExpectedCandidate $ExpectedCandidate
    if ($acceptedSha -cne
            [string]$receipt.Value.precedence.acceptedAggregateSha256) {
        Throw-PartisanFocusedAggregate `
            -Code 'historical_blob_replacement' `
            -Message 'The RED receipt does not bind the preceding accepted aggregate.'
    }
}

function Get-PartisanFocusedPublicationInputSnapshot {
    param(
        [Parameter(Mandatory = $true)][string]$EvidenceRootPath,
        [Parameter(Mandatory = $true)][string[]]$RunPaths,
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)][string]$CurrentCandidateId,
        [AllowEmptyCollection()]
        [Parameter(Mandatory = $true)][string[]]$HistoricalPaths,
        [Collections.IList]$HeldStreams,
        [switch]$IncludeCurrentAggregate
    )

    $evidenceRoot = Resolve-PartisanFocusedExistingDirectory `
        -Path $EvidenceRootPath `
        -Label 'Publication evidence root'
    $repository = Resolve-PartisanFocusedExistingDirectory `
        -Path $RepositoryRootPath `
        -Label 'Publication repository root'
    $files = New-Object Collections.Generic.List[object]
    $directories = New-Object Collections.Generic.List[string]
    $addFile = {
        param([string]$FullPath, [string]$SnapshotPath, [string]$Label)

        $resolved = Resolve-PartisanFocusedExistingFile `
            -Path $FullPath `
            -Label $Label `
            -Code 'evidence_snapshot_drift'
        Assert-PartisanFocusedNoReparseAncestry `
            -Root ([IO.Path]::GetPathRoot($resolved)) `
            -Path $resolved `
            -Label $Label
        [byte[]]$bytes = $null
        if ($null -ne $HeldStreams) {
            $stream = $null
            $memory = $null
            try {
                $stream = [IO.File]::Open(
                    $resolved,
                    [IO.FileMode]::Open,
                    [IO.FileAccess]::Read,
                    [IO.FileShare]::Read)
                $memory = New-Object IO.MemoryStream
                $stream.CopyTo($memory)
                $bytes = $memory.ToArray()
                [void]$HeldStreams.Add($stream)
                $stream = $null
            }
            finally {
                if ($null -ne $memory) {
                    $memory.Dispose()
                }
                if ($null -ne $stream) {
                    $stream.Dispose()
                }
            }
        }
        else {
            $bytes = [IO.File]::ReadAllBytes($resolved)
        }
        [void]$files.Add([pscustomobject][ordered]@{
            path = $SnapshotPath
            length = [long]$bytes.Length
            sha256 = Get-PartisanFocusedBytesSha256 -Bytes $bytes
        })
    }

    $runArguments = New-Object Collections.Generic.List[string]
    $snapshotBindings = New-Object Collections.Generic.List[object]
    foreach ($runPath in $RunPaths) {
        $resolvedRun = Resolve-PartisanFocusedExistingFile `
            -Path $runPath `
            -Label 'Publication focused run' `
            -Code 'evidence_snapshot_drift'
        $portableRun = Get-PartisanFocusedPortableRelativePath `
            -Root $evidenceRoot `
            -Path $resolvedRun `
            -Label 'Publication focused run'
        [void]$runArguments.Add($portableRun)
        $runInput = Read-PartisanFocusedJson `
            -Path $resolvedRun `
            -Label 'Publication focused run' `
            -Code 'evidence_snapshot_drift'
        try {
            $indexedRows = @(Get-PartisanFocusedIndexedFiles `
                -EvidenceRootPath $evidenceRoot `
                -RunPath $resolvedRun `
                -Run $runInput.Value)
        }
        catch {
            Throw-PartisanFocusedAggregate `
                -Code 'evidence_snapshot_drift' `
                -Message 'A focused run changed before publication.'
        }
        & $addFile `
            -FullPath $resolvedRun `
            -SnapshotPath ('evidence/' + $portableRun) `
            -Label 'Publication focused run envelope'
        $runPrefix = $portableRun.Substring(
            0,
            $portableRun.Length - 'run.json'.Length)
        foreach ($row in @($indexedRows | Sort-Object path -CaseSensitive)) {
            & $addFile `
                -FullPath ([string]$row.fullPath) `
                -SnapshotPath ('evidence/' + $runPrefix + [string]$row.path) `
                -Label 'Publication indexed focused evidence'
        }
        [void]$snapshotBindings.Add([pscustomobject][ordered]@{
            EnvelopePath = $portableRun
            Files = @($indexedRows | ForEach-Object {
                [pscustomobject][ordered]@{ path = [string]$_.path }
            })
        })
    }
    $focusedTreeCensus = Get-PartisanFocusedCandidateTreeCensus `
        -EvidenceRootPath $evidenceRoot `
        -CandidateId $CurrentCandidateId `
        -Bindings $snapshotBindings.ToArray() `
        -Code 'evidence_snapshot_drift'

    $repositoryPaths = @(
        $script:FocusedReleaseStatusPath,
        ('docs/evidence/release-candidates/' + $CurrentCandidateId +
            '/candidate.json'),
        ('docs/evidence/release-candidates/' + $CurrentCandidateId +
            '/candidate.ready.json'),
        'tools/New-PartisanFocusedAutotestAggregate.ps1',
        'tools/update-release-docs.ps1'
    )
    foreach ($relative in $repositoryPaths) {
        & $addFile `
            -FullPath (Join-Path $repository $relative.Replace('/', '\')) `
            -SnapshotPath ('repository/' + $relative) `
            -Label 'Publication repository input'
    }

    $historicalArguments = New-Object Collections.Generic.List[string]
    foreach ($historicalPath in $HistoricalPaths) {
        $historicalFull = Resolve-PartisanFocusedExistingFile `
            -Path $historicalPath `
            -Label 'Publication historical aggregate' `
            -Code 'evidence_snapshot_drift'
        [void]$historicalArguments.Add((
            Get-PartisanFocusedPortableRelativePath `
                -Root $repository `
                -Path $historicalFull `
                -Label 'Publication historical aggregate'))
    }

    $historyRoot = Join-Path $repository $script:FocusedTrackedEvidencePrefix
    if (Test-Path -LiteralPath $historyRoot -PathType Container) {
        foreach ($item in @(Get-ChildItem `
                -LiteralPath $historyRoot `
                -Recurse `
                -Force `
                -ErrorAction Stop)) {
            $relative = Get-PartisanFocusedPortableRelativePath `
                -Root $repository `
                -Path $item.FullName `
                -Label 'Publication history entry'
            if (-not $item.PSIsContainer -and
                ((-not $IncludeCurrentAggregate -and
                    $item.DirectoryName -eq $historyRoot -and
                    $item.Name -ceq ($CurrentCandidateId + '.json')) -or
                 $item.Name -cmatch (
                    '^\.[A-Za-z0-9][A-Za-z0-9._-]{0,127}' +
                    '\.publication\.lock$') -or
                 $item.Name -cmatch (
                    '^\.[A-Za-z0-9][A-Za-z0-9._-]{0,127}\.json' +
                    '(?:\.replacement-required\.json)?\.[0-9a-f]{32}\.tmp$'))) {
                continue
            }
            if ($item.PSIsContainer) {
                Assert-PartisanFocusedNoReparseAncestry `
                    -Root $repository `
                    -Path $item.FullName `
                    -Label 'Publication history directory'
                [void]$directories.Add($relative)
            }
            else {
                & $addFile `
                    -FullPath $item.FullName `
                    -SnapshotPath ('repository/' + $relative) `
                    -Label 'Publication history file'
            }
        }
    }
    return [pscustomobject][ordered]@{
        repositoryHead = Get-PartisanFocusedGitHead `
            -RepositoryRootPath $repository
        focusedTreeCensus = $focusedTreeCensus
        runArguments = $runArguments.ToArray()
        historicalArguments = @($historicalArguments | Sort-Object -CaseSensitive)
        directories = @($directories | Sort-Object -CaseSensitive)
        files = @($files | Sort-Object path -CaseSensitive)
    }
}

function Invoke-PartisanFocusedSelfTestLateDriftBarrier {
    param([Parameter(Mandatory = $true)][string]$EvidenceRootPath)

    $token = [string]$env:PARTISAN_FOCUSED_AGGREGATE_SELFTEST_LATE_DRIFT_TOKEN
    if ([string]::IsNullOrEmpty($token)) {
        return
    }
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') +
        [IO.Path]::DirectorySeparatorChar
    $evidence = [IO.Path]::GetFullPath($EvidenceRootPath)
    $relative = if ($evidence.StartsWith(
            $temp,
            [StringComparison]::OrdinalIgnoreCase)) {
        $evidence.Substring($temp.Length)
    }
    else {
        ''
    }
    $firstSegment = @($relative -split '[\\/]' | Where-Object { $_ } |
        Select-Object -First 1)
    if ($token -cnotmatch '^[0-9a-f]{32}$' -or
        $firstSegment.Count -ne 1 -or
        $firstSegment[0] -cnotmatch
            '^PartisanFocusedAggregateSelfTest-[0-9a-f]{32}$') {
        Throw-PartisanFocusedAggregate `
            -Code 'evidence_snapshot_drift' `
            -Message 'The bounded late-drift self-test seam was denied.'
    }

    $ready = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanFocusedAggregateReady-' + $token))
    $mutated = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanFocusedAggregateMutated-' + $token))
    try {
        [void]$ready.Set()
        if (-not $mutated.WaitOne(15000)) {
            Throw-PartisanFocusedAggregate `
                -Code 'evidence_snapshot_drift' `
                -Message 'The bounded late-drift self-test seam timed out.'
        }
    }
    finally {
        $ready.Dispose()
        $mutated.Dispose()
    }
}

function Invoke-PartisanFocusedSelfTestOutputParentBarrier {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRootPath,
        [Parameter(Mandatory = $true)][string]$OutputPath
    )

    $token = [string]$env:PARTISAN_FOCUSED_AGGREGATE_SELFTEST_PARENT_TOKEN
    if ([string]::IsNullOrEmpty($token)) {
        return
    }
    if ($script:FocusedOutputParentBarrierUsed) {
        return
    }
    $script:FocusedOutputParentBarrierUsed = $true
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') +
        [IO.Path]::DirectorySeparatorChar
    $repository = [IO.Path]::GetFullPath($RepositoryRootPath)
    $relative = if ($repository.StartsWith(
            $temp,
            [StringComparison]::OrdinalIgnoreCase)) {
        $repository.Substring($temp.Length)
    }
    else {
        ''
    }
    $firstSegment = @($relative -split '[\\/]' | Where-Object { $_ } |
        Select-Object -First 1)
    if ($token -cnotmatch '^[0-9a-f]{32}$' -or
        $firstSegment.Count -ne 1 -or
        $firstSegment[0] -cnotmatch
            '^PartisanFocusedAggregateSelfTest-[0-9a-f]{32}$' -or
        -not (Test-PartisanFocusedContainedPath `
            -Root $repository `
            -Path ([IO.Path]::GetFullPath($OutputPath)))) {
        Throw-PartisanFocusedAggregate `
            -Code 'output_reparse_path' `
            -Message 'The bounded output-parent self-test seam was denied.'
    }
    $ready = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanFocusedAggregateParentReady-' + $token))
    $continued = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanFocusedAggregateParentContinue-' + $token))
    try {
        [void]$ready.Set()
        if (-not $continued.WaitOne(15000)) {
            Throw-PartisanFocusedAggregate `
                -Code 'output_reparse_path' `
                -Message 'The bounded output-parent self-test seam timed out.'
        }
    }
    finally {
        $ready.Dispose()
        $continued.Dispose()
    }
}

function Invoke-PartisanFocusedAggregate {
    param(
        [Parameter(Mandatory = $true)][string]$EvidenceRootArgument,
        [Parameter(Mandatory = $true)][string[]]$RunArguments,
        [Parameter(Mandatory = $true)][string]$OutputArgument,
        [AllowEmptyCollection()]
        [string[]]$HistoricalArguments = @(),
        [Parameter(Mandatory = $true)][string]$RepositoryRootArgument
    )

    $rootPath = Resolve-PartisanFocusedExistingDirectory `
        -Path $EvidenceRootArgument `
        -Label 'Focused evidence root'
    Assert-PartisanFocusedNoReparseAncestry `
        -Root ([IO.Path]::GetPathRoot($rootPath)) `
        -Path $rootPath `
        -Label 'Focused evidence root'
    $repositoryFull = Resolve-PartisanFocusedExistingDirectory `
        -Path $RepositoryRootArgument `
        -Label 'Aggregate repository root'
    $initialAggregate = New-PartisanFocusedAggregateValue `
        -EvidenceRootPath $rootPath `
        -RunPaths $RunArguments `
        -RepositoryRootPath $repositoryFull
    $canonicalOutput = [IO.Path]::GetFullPath((Join-Path `
        $repositoryFull `
        ('docs/evidence/focused-autotest/' +
            $initialAggregate.candidate.candidateId + '.json')))
    $requestedOutput = [IO.Path]::GetFullPath($OutputArgument)
    if (-not (Test-PartisanFocusedContainedPath `
            -Root $repositoryFull `
            -Path $requestedOutput)) {
        Throw-PartisanFocusedAggregate `
            -Code 'output_path_invalid' `
            -Message 'The aggregate output escaped its repository.'
    }
    $requestedRelative = Get-PartisanFocusedPortableRelativePath `
        -Root $repositoryFull `
        -Path $requestedOutput `
        -Label 'Aggregate output path'
    $canonicalRelative = $script:FocusedTrackedEvidencePrefix + '/' +
        $initialAggregate.candidate.candidateId + '.json'
    if (-not $requestedOutput.Equals(
            $canonicalOutput,
            [StringComparison]::OrdinalIgnoreCase) -or
        $requestedRelative -cne $canonicalRelative) {
        Throw-PartisanFocusedAggregate `
            -Code 'output_path_invalid' `
            -Message 'The v2 focused aggregate output must use its canonical tracked candidate path.'
    }
    $candidateId = [string]$initialAggregate.candidate.candidateId
    $receiptPath = Get-PartisanFocusedRejectionReceiptPath `
        -RepositoryRootPath $repositoryFull `
        -OutputPath $requestedOutput `
        -CandidateId $candidateId
    $publicationLock = Enter-PartisanFocusedCandidatePublicationLock `
        -RepositoryRootPath $repositoryFull `
        -CandidateId $candidateId
    try {
        $includeCurrentAggregate = Test-Path `
            -LiteralPath $requestedOutput `
            -PathType Leaf
        $initialHistory = @(Get-PartisanFocusedTrackedHistoricalAggregates `
            -RepositoryRootPath $repositoryFull `
            -HistoricalPaths $HistoricalArguments `
            -CurrentCandidateId $candidateId)
        Assert-PartisanFocusedRedPrecedence `
            -ReceiptPath $receiptPath `
            -AggregatePath $requestedOutput `
            -RepositoryRootPath $repositoryFull `
            -ExpectedCandidate $initialAggregate.candidate `
            -EnforceFirstPublication
        $initialSnapshot = [pscustomobject][ordered]@{
            aggregate = $initialAggregate
            history = $initialHistory
        }
        $initialSnapshotText = ConvertTo-PartisanFocusedCanonicalJson `
            -Value $initialSnapshot
        $initialInputSnapshotText = ConvertTo-PartisanFocusedCanonicalJson `
            -Value (Get-PartisanFocusedPublicationInputSnapshot `
                -EvidenceRootPath $rootPath `
                -RunPaths $RunArguments `
                -RepositoryRootPath $repositoryFull `
                -CurrentCandidateId $candidateId `
                -HistoricalPaths $HistoricalArguments `
                -IncludeCurrentAggregate:$includeCurrentAggregate)

        # Reopen tracked history first, then make the full five-envelope pass
        # before the retained-handle publication barrier.
        $finalHistory = @(Get-PartisanFocusedTrackedHistoricalAggregates `
            -RepositoryRootPath $repositoryFull `
            -HistoricalPaths $HistoricalArguments `
            -CurrentCandidateId $candidateId)
        $aggregate = New-PartisanFocusedAggregateValue `
            -EvidenceRootPath $rootPath `
            -RunPaths $RunArguments `
            -RepositoryRootPath $repositoryFull
        $finalSnapshot = [pscustomobject][ordered]@{
            aggregate = $aggregate
            history = $finalHistory
        }
        if ((ConvertTo-PartisanFocusedCanonicalJson -Value $finalSnapshot) -cne
            $initialSnapshotText) {
            Throw-PartisanFocusedAggregate `
                -Code 'evidence_snapshot_drift' `
                -Message 'Focused evidence changed between retained validation passes.'
        }
        foreach ($historicalAggregate in $finalHistory) {
            if ([string]$historicalAggregate.candidate.candidateId -ceq
                    [string]$aggregate.candidate.candidateId -or
                [string]$historicalAggregate.aggregateId -ceq
                    [string]$aggregate.aggregateId) {
                Throw-PartisanFocusedAggregate `
                    -Code 'historical_blob_replacement' `
                    -Message 'Focused aggregate history repeats the current identity.'
            }
        }
        Invoke-PartisanFocusedSelfTestLateDriftBarrier `
            -EvidenceRootPath $rootPath

        $heldStreams = New-Object Collections.Generic.List[object]
        try {
            $publicationInputSnapshotText = ConvertTo-PartisanFocusedCanonicalJson `
                -Value (Get-PartisanFocusedPublicationInputSnapshot `
                    -EvidenceRootPath $rootPath `
                    -RunPaths $RunArguments `
                    -RepositoryRootPath $repositoryFull `
                    -CurrentCandidateId $candidateId `
                    -HistoricalPaths $HistoricalArguments `
                    -HeldStreams $heldStreams `
                    -IncludeCurrentAggregate:$includeCurrentAggregate)
            if ($publicationInputSnapshotText -cne $initialInputSnapshotText) {
                Throw-PartisanFocusedAggregate `
                    -Code 'evidence_snapshot_drift' `
                    -Message 'Focused input bytes or census changed before publication.'
            }
            $assertPublicationStationary = {
                Assert-PartisanFocusedRedPrecedence `
                    -ReceiptPath $receiptPath `
                    -AggregatePath $requestedOutput `
                    -RepositoryRootPath $repositoryFull `
                    -ExpectedCandidate $aggregate.candidate `
                    -EnforceFirstPublication
                $reopenedSnapshotText = ConvertTo-PartisanFocusedCanonicalJson `
                    -Value (Get-PartisanFocusedPublicationInputSnapshot `
                        -EvidenceRootPath $rootPath `
                        -RunPaths $RunArguments `
                        -RepositoryRootPath $repositoryFull `
                        -CurrentCandidateId $candidateId `
                        -HistoricalPaths $HistoricalArguments `
                        -IncludeCurrentAggregate:$includeCurrentAggregate)
                if ($reopenedSnapshotText -cne $publicationInputSnapshotText) {
                    Throw-PartisanFocusedAggregate `
                        -Code 'evidence_snapshot_drift' `
                        -Message 'Focused input bytes, census, or HEAD changed at publication.'
                }
            }
            & $assertPublicationStationary
            $write = Write-PartisanFocusedImmutableJson `
                -Path $OutputArgument `
                -Value $aggregate `
                -ConflictCode 'historical_blob_replacement' `
                -RepositoryRootPath $repositoryFull `
                -BeforeCommit $assertPublicationStationary `
                -AfterCommit $assertPublicationStationary
            return [pscustomobject][ordered]@{
                Aggregate = $aggregate
                OutputSha256 = $write.Sha256
                Created = $write.Created
            }
        }
        finally {
            for ($streamIndex = $heldStreams.Count - 1;
                    $streamIndex -ge 0;
                    $streamIndex--) {
                $heldStreams[$streamIndex].Dispose()
            }
        }
    }
    finally {
        Exit-PartisanFocusedCandidatePublicationLock `
            -Lock $publicationLock `
            -RepositoryRootPath $repositoryFull
    }
}

try {
    $aggregateResult = Invoke-PartisanFocusedAggregate `
        -EvidenceRootArgument $EvidenceRoot `
        -RunArguments $RunJson `
        -OutputArgument $OutputPath `
        -HistoricalArguments $HistoricalAggregate `
        -RepositoryRootArgument $RepositoryRoot
    Write-Output ('FOCUSED_AGGREGATE ' + ([pscustomobject][ordered]@{
        aggregateId = $aggregateResult.Aggregate.aggregateId
        candidateId = $aggregateResult.Aggregate.candidate.candidateId
        disposition = $aggregateResult.Aggregate.admission.disposition
        caseCount = $aggregateResult.Aggregate.result.caseCount
        fileCount = $aggregateResult.Aggregate.result.envelopeFileCount
        aggregatePolicyAssertionsPassed =
            $aggregateResult.Aggregate.aggregatePolicyAssertions.passed
        junitTests = $aggregateResult.Aggregate.result.junitTests
        junitFailures = $aggregateResult.Aggregate.result.junitFailures
        junitErrors = $aggregateResult.Aggregate.result.junitErrors
        junitSkipped = $aggregateResult.Aggregate.result.junitSkipped
        outputSha256 = $aggregateResult.OutputSha256
        created = $aggregateResult.Created
    } | ConvertTo-Json -Compress))
}
catch {
    $failureMessage = [string]$_.Exception.Message
    $reasonCode = 'validation_failed'
    if ($_.Exception.Data.Contains('PartisanFocusedAggregateCode')) {
        $reasonCode = [string]$_.Exception.Data['PartisanFocusedAggregateCode']
    }
    $rejectionOutput = [pscustomobject][ordered]@{
        disposition = $script:FocusedReplacementDisposition
        releaseDecision = 'RED'
        reasonCode = $reasonCode
        candidateId = $null
        durableReceiptAvailable = $false
        durableReceiptCreated = $false
        durableReceiptSha256 = $null
    }
    try {
        $authenticated = Get-PartisanFocusedAuthenticatedRejectionCandidate `
            -EvidenceRootPath $EvidenceRoot `
            -RunPaths $RunJson `
            -RepositoryRootPath $RepositoryRoot
        $candidateId = [string]$authenticated.Candidate.candidateId
        $rejectionOutput.candidateId = $candidateId
        $receiptPath = Get-PartisanFocusedRejectionReceiptPath `
            -RepositoryRootPath $RepositoryRoot `
            -OutputPath $OutputPath `
            -CandidateId $candidateId
        $redLock = Enter-PartisanFocusedCandidatePublicationLock `
            -RepositoryRootPath $RepositoryRoot `
            -CandidateId $candidateId
        try {
            $lockedAuthenticated =
                Get-PartisanFocusedAuthenticatedRejectionCandidate `
                    -EvidenceRootPath $EvidenceRoot `
                    -RunPaths $RunJson `
                    -RepositoryRootPath $RepositoryRoot
            Assert-PartisanFocusedSameValue `
                -Expected $authenticated `
                -Actual $lockedAuthenticated `
                -Label 'Locked rejection candidate identity' `
                -Code 'red_candidate_unavailable'
            $authenticated = $lockedAuthenticated
            $acceptedSha = Get-PartisanFocusedAcceptedAggregateSha256 `
                -Path $OutputPath `
                -RepositoryRootPath $RepositoryRoot `
                -ExpectedCandidate $authenticated.Candidate `
                -AllowMissing
            $receipt = New-PartisanFocusedRejectionReceipt `
                -ReasonCode $reasonCode `
                -Candidate $authenticated.Candidate `
                -RunEnvelopeSha256 $authenticated.RunEnvelopeSha256 `
                -AcceptedAggregateSha256 $acceptedSha
            try {
                $redWrite = Write-PartisanFocusedImmutableJson `
                    -Path $receiptPath `
                    -Value $receipt `
                    -ConflictCode 'historical_blob_replacement' `
                    -RepositoryRootPath $RepositoryRoot
                $rejectionOutput.durableReceiptAvailable = $true
                $rejectionOutput.durableReceiptCreated = $redWrite.Created
                $rejectionOutput.durableReceiptSha256 = $redWrite.Sha256
            }
            catch {
                # Preserve a candidate-bound immutable receipt that won the race.
                if (Test-Path -LiteralPath $receiptPath -PathType Leaf) {
                    $existingReceipt = Read-PartisanFocusedJson `
                        -Path $receiptPath `
                        -Label 'Existing focused rejection receipt' `
                        -Code 'historical_blob_replacement'
                    Assert-PartisanFocusedRejectionReceiptValue `
                        -Value $existingReceipt.Value `
                        -Label 'Existing focused rejection receipt' `
                        -ExpectedCandidate $authenticated.Candidate `
                        -Code 'historical_blob_replacement'
                    $rejectionOutput.durableReceiptAvailable = $true
                    $rejectionOutput.durableReceiptCreated = $false
                    $rejectionOutput.durableReceiptSha256 =
                        [string]$existingReceipt.Sha256
                }
            }
        }
        finally {
            Exit-PartisanFocusedCandidatePublicationLock `
                -Lock $redLock `
                -RepositoryRootPath $RepositoryRoot
        }
    }
    catch {
        # No durable RED is published without a fully authenticated candidate
        # identity and its exact canonical output path.
    }
    Write-Output ('FOCUSED_AGGREGATE_REJECTED ' +
        ($rejectionOutput | ConvertTo-Json -Depth 20 -Compress))
    $safeFailureMessage = $failureMessage -replace
        '(?i)(?:[A-Z]:[\\/][^\r\n]*|\\\\[^\r\n]*|/(?:Users|home|mnt)/[^\r\n]*)',
        '<local-path>'
    throw "Focused aggregate rejected with RED $($script:FocusedReplacementDisposition) disposition ($reasonCode): $safeFailureMessage"
}
