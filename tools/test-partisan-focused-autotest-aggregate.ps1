[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$producer = Join-Path $PSScriptRoot 'New-PartisanFocusedAutotestAggregate.ps1'
$profileOrder = @(
    'HST_EnemyCounterattackAutotestSuite',
    'HST_EnemyGarrisonRebuildAutotestSuite',
    'HST_EnemyPlanningCommitmentAutotestSuite',
    'HST_EnemyQRFAutotestSuite',
    'HST_CampaignProfileJournalAuthorityAutotestSuite'
)
$suiteByProfile = [ordered]@{
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
$testCasesByProfile = [ordered]@{
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
$tempRoot = $null
$checkCount = 0
$passPayload = $null

function Assert-SelfTest {
    param(
        [Parameter(Mandatory = $true)][bool]$Condition,
        [Parameter(Mandatory = $true)][string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
    $script:checkCount++
}

function Get-SelfTestSha256 {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (Get-FileHash `
        -LiteralPath $Path `
        -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Get-SelfTestTextSha256 {
    param(
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)][string]$Text
    )

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = (New-Object Text.UTF8Encoding($false)).GetBytes($Text)
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($bytes))).Replace(
                '-', '').ToLowerInvariant()
    }
    finally {
        $algorithm.Dispose()
    }
}

function Get-SelfTestPackageSha256 {
    param([Parameter(Mandatory = $true)][object[]]$Files)

    $index = (@($Files | Sort-Object `
        @{ Expression = { [string]$_.indexPath }; Ascending = $true } `
        -CaseSensitive | ForEach-Object {
            "{0}`t{1}`t{2}" -f
                ([string]$_.sha256),
                ([long]$_.length),
                ([string]$_.indexPath)
        }) -join "`n") + "`n"
    return Get-SelfTestTextSha256 -Text $index
}

function Get-SelfTestAggregateId {
    param([Parameter(Mandatory = $true)]$Aggregate)

    $integrityText = $Aggregate.integrity |
        ConvertTo-Json -Depth 100 -Compress
    $sourceText = @($Aggregate.sourceRuns | ForEach-Object {
        $_.testCase + '|' + $_.runId + '|' + $_.runEnvelopeSha256 + '|' +
            (@($_.files | ForEach-Object {
                $_.path + ':' + $_.length + ':' + $_.sha256
            }) -join ',')
    }) -join "`n"
    return 'focused-set-' +
        (Get-SelfTestTextSha256 -Text ($integrityText + "`n" + $sourceText)).
            Substring(0, 24)
}

function Write-SelfTestText {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [AllowEmptyString()][Parameter(Mandatory = $true)][string]$Text
    )

    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    [IO.File]::WriteAllText(
        $Path,
        $Text,
        (New-Object Text.UTF8Encoding($false)))
}

function Write-SelfTestJson {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value
    )

    $text = ($Value | ConvertTo-Json -Depth 100) + "`n"
    Write-SelfTestText -Path $Path -Text $text.Replace("`r`n", "`n")
}

function Add-SelfTestUtf8Bom {
    param([Parameter(Mandatory = $true)][string]$Path)

    $original = [IO.File]::ReadAllBytes($Path)
    $bytes = New-Object byte[] ($original.Length + 3)
    $bytes[0] = 0xEF
    $bytes[1] = 0xBB
    $bytes[2] = 0xBF
    [Array]::Copy($original, 0, $bytes, 3, $original.Length)
    [IO.File]::WriteAllBytes($Path, $bytes)
}

function Read-SelfTestJson {
    param([Parameter(Mandatory = $true)][string]$Path)

    return [IO.File]::ReadAllText($Path) | ConvertFrom-Json
}

function New-SelfTestExecutable {
    param(
        [Parameter(Mandatory = $true)][string]$FileName,
        [Parameter(Mandatory = $true)][string]$Sha256,
        [Parameter(Mandatory = $true)][long]$Length
    )

    return [pscustomobject][ordered]@{
        fileName = $FileName
        fileVersion = '1.7.0.54'
        productVersion = '1.7.0.54'
        length = $Length
        sha256 = $Sha256
    }
}

function New-SelfTestFixture {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$HarnessGitHead,
        [Parameter(Mandatory = $true)][string]$RunnerSha256,
        [Parameter(Mandatory = $true)][string]$CandidateModuleSha256,
        [Parameter(Mandatory = $true)][string]$TrackedManifestPath,
        [Parameter(Mandatory = $true)][string]$TrackedReadyPath
    )

    $evidenceRoot = Join-Path $Root 'evidence'
    New-Item -ItemType Directory -Path $evidenceRoot -Force | Out-Null
    $manifest = Read-SelfTestJson -Path $TrackedManifestPath
    $ready = Read-SelfTestJson -Path $TrackedReadyPath
    $manifestSha = Get-SelfTestSha256 -Path $TrackedManifestPath
    $readySha = Get-SelfTestSha256 -Path $TrackedReadyPath
    $client = $manifest.toolchain.client
    $diagnostic = $manifest.toolchain.clientDiagnostic
    $publicCandidate = [pscustomobject][ordered]@{
        candidateId = $manifest.candidate.id
        candidateVersion = $manifest.candidate.version
        runtimeUseDisposition = 'active-runtime-candidate'
        gitHead = $manifest.source.gitHead
        embeddedBuildSha = $manifest.source.embeddedImplementation.sha
        embeddedBuildUtc = $manifest.source.embeddedImplementation.utc
        embeddedBuildLabel = $manifest.source.embeddedImplementation.label
        campaignSchema = [long]$manifest.source.campaignSchema
        runtimeSettingsSchema = [long]$manifest.source.runtimeSettingsSchema
        addonId = $manifest.addon.id
        addonGuid = $manifest.addon.guid
        packageHashAlgorithm = $manifest.package.hashAlgorithm
        packageSha256 = $manifest.package.sha256
        manifestSha256 = $manifestSha
        readySha256 = $readySha
        workbenchCrc = $manifest.workbench.crc
        runtimeRole = 'client'
        diagnosticExecutable = $diagnostic
        recordedDiagnosticExecutable = $diagnostic
        recordedRuntimeExecutable = $client
    }
    $harness = [pscustomobject][ordered]@{
        gitHead = $HarnessGitHead
        dirty = $false
        focusedRunnerSha256 = $RunnerSha256
        candidateModuleSha256 = $CandidateModuleSha256
    }
    $runPaths = New-Object Collections.Generic.List[string]
    for ($index = 0; $index -lt $profileOrder.Count; $index++) {
        $profile = $profileOrder[$index]
        $suite = [string]$suiteByProfile[$profile]
        $expectedTestCases = @($testCasesByProfile[$profile])
        $second = 10 + ($index * 10)
        $started = '2026-07-19T00:02:{0:D2}.0000000Z' -f $second
        $completed = '2026-07-19T00:02:{0:D2}.5000000Z' -f ($second + 5)
        $runId = '20260719T0002{0:D2}Z-{1}' -f $second, ($index + 1).ToString('x32')
        $runNonce = $runId.Substring($runId.Length - 32)
        $candidateProject = 'fixture/PartisanFocusedAutotest/' + $runNonce +
            '/candidate-addons/Partisan/addon.gproj'
        $candidateEvidenceRoot = Join-Path $evidenceRoot $manifest.candidate.id
        $focusedEvidenceRoot = Join-Path $candidateEvidenceRoot 'focused-autotest'
        $runRoot = Join-Path (Join-Path $focusedEvidenceRoot $profile) $runId
        $identityRoot = Join-Path $runRoot 'identity'
        $rawRoot = Join-Path $runRoot ('raw/logs/logs-' + ($index + 1))
        New-Item `
            -ItemType Directory `
            -Path $identityRoot, $rawRoot `
            -Force | Out-Null
        Copy-Item `
            -LiteralPath $TrackedManifestPath `
            -Destination (Join-Path $identityRoot 'candidate.json')
        Copy-Item `
            -LiteralPath $TrackedReadyPath `
            -Destination (Join-Path $identityRoot 'candidate.ready.json')

        Write-SelfTestText `
            -Path (Join-Path $rawRoot 'autotest.log') `
            -Text ("$profile accepted`n")
        Write-SelfTestText `
            -Path (Join-Path $rawRoot 'autotest_failed.log') `
            -Text ''
        $buildSummary = 'sha ' + $publicCandidate.embeddedBuildSha +
            ' | utc ' + $publicCandidate.embeddedBuildUtc +
            ' | label ' + $publicCandidate.embeddedBuildLabel
        $consoleLines = New-Object Collections.Generic.List[string]
        [void]$consoleLines.Add("CLI autotest case: $profile")
        [void]$consoleLines.Add(
            "00:01:57.000 ENGINE : gproj: 'runtime-addons/core/core.gproj' guid: '5614BBCCBB55ED1C' (packed)")
        [void]$consoleLines.Add(
            "00:01:57.500 ENGINE : gproj: 'runtime-addons/data/ArmaReforger.gproj' guid: '58D0FB3206B6F859' (packed)")
        [void]$consoleLines.Add(
            "00:01:58.000 ENGINE : gproj: '$candidateProject' guid: '$($publicCandidate.addonGuid)' (packed)")
        [void]$consoleLines.Add(
            "00:01:59.000 ENGINE : gproj: '$candidateProject' guid: '$($publicCandidate.addonGuid)'")
        [void]$consoleLines.Add(
            "00:01:59.500 ENGINE : gproj: 'runtime-addons/core/core.gproj' guid: '5614BBCCBB55ED1C'")
        [void]$consoleLines.Add(
            "00:01:59.750 ENGINE : gproj: 'runtime-addons/data/ArmaReforger.gproj' guid: '58D0FB3206B6F859'")
        [void]$consoleLines.Add("00:02:00.000 SCRIPT : TestSuite #$suite started")
        [void]$consoleLines.Add($buildSummary)
        foreach ($expectedTestCase in $expectedTestCases) {
            if ($index -eq 4) {
                [void]$consoleLines.Add(
                    "00:02:01.000 SCRIPT (E): string failureDetail = 'Partisan persistence | native save callback failure | sequence/type/flags 1/0/0 | manager/enabled/allowed/busy/active/playthrough 1/1/1/0/0/0 | types/persistence/state/loaded/tracked/config/staged 5/1/2/0/0/0/1 | replication mode 0 | snapshot fingerprint '")
                [void]$consoleLines.Add(
                    '00:02:02.000 SCRIPT : failed native callback non-mutating 1')
                [void]$consoleLines.Add(
                    '00:02:03.000 SCRIPT : setup/seam/request/bytes/journal 1/1/1/1/1')
            }
            [void]$consoleLines.Add(
                "00:02:04.000 SCRIPT : $expectedTestCase`: SUCCESS")
        }
        [void]$consoleLines.Add('00:02:05.000 SCRIPT : SCR_TestRunner has finished running')
        [void]$consoleLines.Add(
            'Autotest JUnit XML saved to: ' +
                (Join-Path $rawRoot 'junit.xml'))
        [void]$consoleLines.Add(
            'Autotest failed list saved to: ' +
                (Join-Path $rawRoot 'autotest_failed.log'))
        [void]$consoleLines.Add(
            "00:02:06.000 SCRIPT (E): Can't instantiate class 'SCR_FilterCategory', constructor is not public")
        [void]$consoleLines.Add(
            "00:02:07.000 SCRIPT (E): Can't instantiate class 'SCR_FilterCategory', constructor is not public")
        $requiredPattern = "CLI autotest case: $profile"
        $requiredPatternBytes = (New-Object Text.UTF8Encoding($false, $true)).
            GetBytes($requiredPattern)
        [void]$consoleLines.Add(
            'PARTISAN_REQUIRED_LOG_PATTERN_B64 ' +
                [Convert]::ToBase64String($requiredPatternBytes))
        $console = @($consoleLines) -join "`n"
        $diagnosticTail = @($consoleLines | Where-Object {
            $_ -match 'HST_|Autotest|autotest|Test Result|SCRIPT\s+\(E\)|ENGINE\s+\(E\)'
        } | Select-Object -Last 80 | ForEach-Object {
            $line = [string]$_
            $marker = [regex]::Match(
                $line,
                '(?i)^(?<prefix>.*Autotest (?:JUnit XML|failed list) saved to:)' +
                    '(?<suffix>\s+.+)$')
            if ($marker.Success) {
                $marker.Groups['prefix'].Value + ' <local-path>'
            }
            else {
                [regex]::Replace(
                    $line,
                    '(?<!\d)\d{15,20}(?!\d)',
                    '<identity>')
            }
        })
        Write-SelfTestText `
            -Path (Join-Path $rawRoot 'console.log') `
            -Text ($console + "`n")
        Write-SelfTestText `
            -Path (Join-Path $rawRoot 'error.log') `
            -Text "approved diagnostic inventory`n"
        $junitCases = @($expectedTestCases | ForEach-Object {
            '    <testcase classname="' + $suite + '" name="' +
                [string]$_ + '" time="1" />'
        }) -join "`n"
        $junit = @"
<testsuites time="1" timestamp="$started">
  <testsuite name="$suite" tests="$($expectedTestCases.Count)" failures="0" errors="0" skipped="0">
$junitCases
  </testsuite>
</testsuites>
"@
        Write-SelfTestText `
            -Path (Join-Path $rawRoot 'junit.xml') `
            -Text $junit
        Write-SelfTestText `
            -Path (Join-Path $rawRoot 'script.log') `
            -Text "script evidence $profile`n"

        $rows = @(
            Get-ChildItem `
                -LiteralPath $runRoot `
                -Recurse `
                -File | Sort-Object FullName | ForEach-Object {
                    $relative = $_.FullName.Substring($runRoot.Length + 1).
                        Replace('\', '/')
                    [pscustomobject][ordered]@{
                        path = $relative
                        length = [long]$_.Length
                        sha256 = Get-SelfTestSha256 -Path $_.FullName
                    }
                }
        )
        $hardCount = if ($index -eq 4) {
            2 + $expectedTestCases.Count
        }
        else {
            2
        }
        $intentionalCount = if ($index -eq 4) {
            $expectedTestCases.Count
        }
        else {
            0
        }
        $stockCount = $hardCount - $intentionalCount
        $mount = [pscustomobject][ordered]@{
            Valid = $true
            RecordCount = 2
            ExactPathCount = 2
            PackedCount = 1
            InvalidModeCount = 0
            GuidExact = $true
            Packed = $true
        }
        $result = [pscustomobject][ordered]@{
            Candidate = $publicCandidate
            CandidateBoundaryVerified = $true
            MountAttestation = $mount
            Success = $true
            ExitCode = 0
            Tests = $expectedTestCases.Count
            Failures = 0
            Errors = 0
            JUnitTestCaseCount = $expectedTestCases.Count
            JUnitCaseName = $profile
            JUnitCaseClassName = $suite
            JUnitCaseIdentityExact = $true
            JUnitCaseFailures = 0
            JUnitCaseErrors = 0
            JUnitCaseSkipped = 0
            JUnitFailureEvidence = ''
            JUnitErrorEvidence = ''
            FailedListFileCount = 1
            FailedListBytes = 0
            RequiredPatternsSeen = $true
            BuildProvenanceSeen = $true
            ConsoleTestCaseSeen = $true
            HardDiagnosticClassifierChecks = 12
            HardDiagnosticClassificationValid = $true
            HardDiagnosticFree = $false
            HardDiagnosticCount = $hardCount
            ApprovedStockFilterDiagnosticCount = $stockCount
            ApprovedIntentionalFaultDiagnosticCount = $intentionalCount
            UnapprovedHardDiagnosticCount = 0
            UnapprovedHardDiagnosticEvidence = @()
        }
        $cleanup = [pscustomobject][ordered]@{
            GuardRemaining = 0
            GuardBaseRemaining = 0
            EngineProcessesRemaining = 0
            OwnedProcessesRemaining = 0
            UnclaimedEngineProcessesObserved = 0
            NewDefaultEntriesRemaining = 0
            ModifiedDefaultFiles = 0
            DeletedDefaultEntries = 0
            MissingDefaultRoots = 0
            ExternalSpillEntriesRemaining = 0
            ModifiedSpillFiles = 0
            DeletedSpillEntries = 0
            MissingSpillRoots = 0
            MonitoringRootsAreDetectionOnly = $true
            CleanupErrors = @()
        }
        $run = [pscustomobject][ordered]@{
            schemaVersion = 1
            evidenceKind = 'packaged-focused-autotest'
            startedUtc = $started
            completedUtc = $completed
            candidate = $publicCandidate
            harness = $harness
            launch = [pscustomobject][ordered]@{
                testCase = $profile
                stagedPackage = $true
                addonSearchRootCount = 2
                addonGuid = $publicCandidate.addonGuid
                packageSha256 = $publicCandidate.packageSha256
                diagnosticExecutable = $diagnostic
                recordedRuntimeExecutable = $client
            }
            outcome = [pscustomobject][ordered]@{
                success = $true
                candidateBoundaryVerified = $true
                mountAttestation = $mount
                evidenceCaptured = $true
                result = $result
                diagnosticTail = $diagnosticTail
                error = $null
            }
            cleanup = $cleanup
            files = $rows
        }
        $runPath = Join-Path $runRoot 'run.json'
        Write-SelfTestJson -Path $runPath -Value $run
        [void]$runPaths.Add($runPath)
    }
    return [pscustomobject][ordered]@{
        EvidenceRoot = $evidenceRoot
        RunPaths = $runPaths.ToArray()
    }
}

function Copy-SelfTestFixture {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    Copy-Item `
        -Path (Join-Path $Source '*') `
        -Destination $Destination `
        -Recurse `
        -Force
}

function Copy-SelfTestWritableFile {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    Copy-Item `
        -LiteralPath $Source `
        -Destination $Destination `
        -Force
    $destinationItem = Get-Item `
        -LiteralPath $Destination `
        -Force `
        -ErrorAction Stop
    if ($destinationItem.IsReadOnly) {
        $destinationItem.IsReadOnly = $false
        $destinationItem.Refresh()
    }
    if ($destinationItem.IsReadOnly) {
        throw 'Focused aggregate self-test copy remained read-only.'
    }
}

function Test-SelfTestWritableFileCopy {
    param([Parameter(Mandatory = $true)][string]$Root)

    $source = Join-Path $Root 'writable-copy-probe-source.txt'
    $destination = Join-Path $Root 'writable-copy-probe-destination.txt'
    if ((Test-Path -LiteralPath $source) -or
        (Test-Path -LiteralPath $destination)) {
        throw 'Focused aggregate self-test writable-copy probe collided with an existing path.'
    }

    Write-SelfTestText -Path $source -Text "source`n"
    try {
        $sourceItem = Get-Item -LiteralPath $source -Force
        $sourceItem.IsReadOnly = $true
        $sourceItem.Refresh()
        Assert-SelfTest `
            -Condition $sourceItem.IsReadOnly `
            -Message 'Focused aggregate self-test could not arrange its read-only copy probe.'
        Copy-SelfTestWritableFile `
            -Source $source `
            -Destination $destination
        $sourceItem.Refresh()
        $destinationItem = Get-Item -LiteralPath $destination -Force
        Assert-SelfTest `
            -Condition $sourceItem.IsReadOnly `
            -Message 'Focused aggregate self-test writable-copy helper changed its source attribute.'
        Assert-SelfTest `
            -Condition (-not $destinationItem.IsReadOnly) `
            -Message 'Focused aggregate self-test did not normalize a copied read-only harness file.'
        [IO.File]::AppendAllText(
            $destination,
            "append`n",
            (New-Object Text.UTF8Encoding($false)))
        Assert-SelfTest `
            -Condition ([IO.File]::ReadAllText($destination) -ceq
                "source`nappend`n") `
            -Message 'Focused aggregate self-test writable-copy probe could not mutate its destination.'
    }
    finally {
        foreach ($path in @($source, $destination)) {
            if (Test-Path -LiteralPath $path -PathType Leaf) {
                $item = Get-Item -LiteralPath $path -Force
                if ($item.IsReadOnly) {
                    $item.IsReadOnly = $false
                    $item.Refresh()
                }
                Remove-Item `
                    -LiteralPath $path `
                    -Force `
                    -ErrorAction Stop
            }
        }
    }
    Assert-SelfTest `
        -Condition ((-not (Test-Path -LiteralPath $source)) -and
            (-not (Test-Path -LiteralPath $destination))) `
        -Message 'Focused aggregate self-test writable-copy probe cleanup did not converge.'
}

function Get-SelfTestRunPaths {
    param([Parameter(Mandatory = $true)][string]$EvidenceRoot)

    $candidateRoots = @(Get-ChildItem `
        -LiteralPath $EvidenceRoot `
        -Directory `
        -ErrorAction Stop)
    Assert-SelfTest `
        -Condition ($candidateRoots.Count -eq 1) `
        -Message 'A copied fixture did not contain one candidate root.'
    $focusedRoot = Join-Path $candidateRoots[0].FullName 'focused-autotest'
    return @($profileOrder | ForEach-Object {
        $profileRoot = Join-Path $focusedRoot $_
        $runRoot = @(Get-ChildItem `
            -LiteralPath $profileRoot `
            -Directory `
            -ErrorAction Stop)
        Assert-SelfTest `
            -Condition ($runRoot.Count -eq 1) `
            -Message 'A copied fixture profile did not contain one run.'
        Join-Path $runRoot[0].FullName 'run.json'
    })
}

function Invoke-SelfTestProducer {
    param(
        [Parameter(Mandatory = $true)][string]$EvidenceRoot,
        [Parameter(Mandatory = $true)][string[]]$RunPaths,
        [Parameter(Mandatory = $true)][string]$OutputPath,
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [string[]]$Historical = @()
    )

    $succeeded = $false
    $errorText = ''
    $output = New-Object Collections.Generic.List[object]
    try {
        & $producer `
            -EvidenceRoot $EvidenceRoot `
            -RunJson $RunPaths `
            -OutputPath $OutputPath `
            -HistoricalAggregate $Historical `
            -RepositoryRoot $RepositoryRoot | ForEach-Object {
                [void]$output.Add($_)
            }
        $succeeded = $true
    }
    catch {
        $errorText = $_.Exception.Message
    }
    return [pscustomobject][ordered]@{
        Succeeded = $succeeded
        Error = $errorText
        Output = @($output | ForEach-Object { $_ })
    }
}

function Assert-SelfTestRejected {
    param(
        [Parameter(Mandatory = $true)]$Invocation,
        [Parameter(Mandatory = $true)][string]$ReceiptPath,
        [Parameter(Mandatory = $true)][string]$ExpectedReason,
        [switch]$NoReceipt
    )

    Assert-SelfTest `
        -Condition (-not $Invocation.Succeeded) `
        -Message "Negative fixture unexpectedly passed: $ExpectedReason"
    Assert-SelfTest `
        -Condition ([string]$Invocation.Error -cmatch
            ('\(' + [regex]::Escape($ExpectedReason) + '\)')) `
        -Message ("Negative fixture did not fail for $ExpectedReason. Invocation: " +
            [string]$Invocation.Error)
    $resolvedReceiptPath = if ($ReceiptPath.EndsWith(
            '.replacement-required.json',
            [StringComparison]::Ordinal)) {
        $ReceiptPath
    }
    else {
        $ReceiptPath + '.replacement-required.json'
    }
    if ($NoReceipt) {
        Assert-SelfTest `
            -Condition (-not (Test-Path -LiteralPath $resolvedReceiptPath)) `
            -Message "Unauthenticated negative fixture published a red receipt: $ExpectedReason"
        return
    }
    Assert-SelfTest `
        -Condition (Test-Path -LiteralPath $resolvedReceiptPath -PathType Leaf) `
        -Message "Negative fixture omitted its red receipt: $ExpectedReason"
    $receipt = Read-SelfTestJson -Path $resolvedReceiptPath
    Assert-SelfTest `
        -Condition ([string]$receipt.evidenceKind -ceq
            'packaged-focused-autotest-set-rejection') `
        -Message "Negative fixture wrote the wrong receipt kind: $ExpectedReason"
    Assert-SelfTest `
        -Condition ([string]$receipt.admission.status -ceq 'rejected' -and
            [string]$receipt.admission.disposition -ceq 'replacement-required' -and
            [string]$receipt.admission.releaseDecision -ceq 'RED') `
        -Message "Negative fixture did not stay red/replacement-required: $ExpectedReason"
    Assert-SelfTest `
        -Condition ([string]$receipt.admission.reasonCode -ceq $ExpectedReason) `
        -Message ("Negative fixture expected reason $ExpectedReason but received " +
            [string]$receipt.admission.reasonCode + '. Invocation: ' +
            [string]$Invocation.Error)
    $candidateId = [IO.Path]::GetFileName($resolvedReceiptPath).
        Replace('.json.replacement-required.json', '')
    Assert-SelfTest `
        -Condition ([string]$receipt.candidate.candidateId -ceq $candidateId -and
            [string]$receipt.candidate.packageSha256 -cmatch '^[0-9a-f]{64}$' -and
            [string]$receipt.candidate.manifestSha256 -cmatch '^[0-9a-f]{64}$' -and
            [string]$receipt.candidate.readySha256 -cmatch '^[0-9a-f]{64}$') `
        -Message "Negative fixture receipt was not bound to its candidate: $ExpectedReason"
    Assert-SelfTest `
        -Condition ([long]$receipt.aggregationPolicy.requiredProfileCount -eq 5 -and
            [long]$receipt.aggregationPolicy.requiredFilesPerProfile -eq 8 -and
            [long]$receipt.aggregationPolicy.requiredTotalFileCount -eq 40 -and
            [long]$receipt.attemptedInput.runEnvelopeCount -eq 5 -and
            @($receipt.attemptedInput.availableRunEnvelopeSha256).Count -eq 5 -and
            @($receipt.attemptedInput.availableRunEnvelopeSha256 |
                Sort-Object -Unique).Count -eq 5) `
        -Message "Negative fixture receipt lost its five/40 binding: $ExpectedReason"
    $receiptText = [IO.File]::ReadAllText($resolvedReceiptPath)
    Assert-SelfTest `
        -Condition ($receiptText -notmatch
            '(?i)(?:[A-Z]:[\\/]|\\\\|file://|/(?:Users|home|mnt)/)') `
        -Message "Negative fixture receipt leaked a local path: $ExpectedReason"
}

function Update-SelfTestRunIndex {
    param(
        [Parameter(Mandatory = $true)][string]$RunPath,
        [Parameter(Mandatory = $true)][string]$PortablePath
    )

    $run = Read-SelfTestJson -Path $RunPath
    $row = @($run.files | Where-Object { $_.path -ceq $PortablePath })
    Assert-SelfTest `
        -Condition ($row.Count -eq 1) `
        -Message 'The mutation helper did not find one indexed blob.'
    $full = Join-Path (Split-Path -Parent $RunPath) $PortablePath.Replace('/', '\')
    $item = Get-Item -LiteralPath $full
    $row[0].length = [long]$item.Length
    $row[0].sha256 = Get-SelfTestSha256 -Path $full
    Write-SelfTestJson -Path $RunPath -Value $run
}

function New-SelfTestRepository {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [int]$CampaignSchema = 71,
        [int]$RuntimeSettingsSchema = 24,
        [string]$CandidateId = 'partisan-rc-aaaaaaaaaaaa-20260719T000000Z'
    )

    $candidateId = $CandidateId
    $candidateVersion = '0.1.0-rc.20260719T000000Z.aaaaaaaa'
    $toolsRoot = Join-Path $Root 'tools'
    New-Item -ItemType Directory -Path $toolsRoot -Force | Out-Null
    foreach ($name in @(
            'New-PartisanFocusedAutotestAggregate.ps1',
            'update-release-docs.ps1',
            'run-guarded-focused-autotest.ps1',
            'Partisan.ReleaseCandidate.psm1')) {
        $source = if ($name -ceq
            'New-PartisanFocusedAutotestAggregate.ps1') {
            $producer
        }
        else {
            Join-Path $PSScriptRoot $name
        }
        Copy-SelfTestWritableFile `
            -Source $source `
            -Destination (Join-Path $toolsRoot $name)
    }
    & git -C $Root init --quiet
    if ($LASTEXITCODE -ne 0) {
        throw 'Focused aggregate self-test could not initialize its temporary Git repository.'
    }
    & git -C $Root config core.autocrlf false
    & git -C $Root config user.name 'Partisan Focused Aggregate Self-Test'
    & git -C $Root config user.email 'focused-aggregate-selftest@invalid.example'
    & git -C $Root add -- tools
    & git -C $Root commit --quiet -m 'focused aggregate self-test source harness'
    if ($LASTEXITCODE -ne 0) {
        throw 'Focused aggregate self-test could not commit its temporary harness.'
    }
    $runHarnessHead = ((& git -C $Root rev-parse HEAD) -join '').Trim()
    if ($LASTEXITCODE -ne 0 -or $runHarnessHead -cnotmatch '^[0-9a-f]{40}$') {
        throw 'Focused aggregate self-test could not resolve its temporary harness HEAD.'
    }
    $runnerSha = Get-SelfTestSha256 `
        -Path (Join-Path $toolsRoot 'run-guarded-focused-autotest.ps1')
    $candidateModuleSha = Get-SelfTestSha256 `
        -Path (Join-Path $toolsRoot 'Partisan.ReleaseCandidate.psm1')

    $workbench = New-SelfTestExecutable `
        -FileName 'ArmaReforgerWorkbenchSteam.exe' `
        -Sha256 ('4' * 64) `
        -Length 1004
    $server = New-SelfTestExecutable `
        -FileName 'ArmaReforgerServer.exe' `
        -Sha256 ('5' * 64) `
        -Length 1005
    $serverDiagnostic = New-SelfTestExecutable `
        -FileName 'ArmaReforgerServerDiag.exe' `
        -Sha256 ('6' * 64) `
        -Length 1006
    $client = New-SelfTestExecutable `
        -FileName 'ArmaReforgerSteam.exe' `
        -Sha256 ('1' * 64) `
        -Length 1001
    $clientDiagnostic = New-SelfTestExecutable `
        -FileName 'ArmaReforgerSteamDiag.exe' `
        -Sha256 ('2' * 64) `
        -Length 1002
    $workbenchCrc = '1234abcd'
    $workbenchTargets = @('PC', 'PS4', 'PS5', 'XBOX_ONE', 'XBOX_SERIES') |
        ForEach-Object {
            [pscustomobject][ordered]@{
                target = $_
                status = 'passed'
                files = 10
                classes = 20
                crc = $workbenchCrc
                evidencePath = 'evidence/workbench/' + $_ + '.json'
            }
        }
    $packageFiles = @(
        [pscustomobject][ordered]@{
            path = 'package/Partisan/addon.gproj'
            indexPath = 'Partisan/addon.gproj'
            length = 101
            sha256 = '7' * 64
        },
        [pscustomobject][ordered]@{
            path = 'package/Partisan/data.pak'
            indexPath = 'Partisan/data.pak'
            length = 102
            sha256 = '8' * 64
        },
        [pscustomobject][ordered]@{
            path = 'package/Partisan/resourceDatabase.rdb'
            indexPath = 'Partisan/resourceDatabase.rdb'
            length = 103
            sha256 = '9' * 64
        },
        [pscustomobject][ordered]@{
            path = 'package/Partisan/thumbnail.png'
            indexPath = 'Partisan/thumbnail.png'
            length = 104
            sha256 = 'a' * 64
        })
    $packageSha = Get-SelfTestPackageSha256 -Files $packageFiles
    Assert-SelfTest `
        -Condition ($packageSha -ceq
            'c1878d1454e106299590001362106d9b0d4a3cd5086d762842378575aab39dd9') `
        -Message 'Focused aggregate self-test package digest is not canonical.'
    $manifest = [pscustomobject][ordered]@{
        manifestSchemaVersion = 1
        createdUtc = '2026-07-19T00:01:00.0000000Z'
        candidate = [pscustomobject][ordered]@{
            id = $candidateId
            version = $candidateVersion
            state = 'retained-uncertified'
        }
        source = [pscustomobject][ordered]@{
            gitHead = $runHarnessHead
            dirty = $false
            auditedGameplayRevision = $runHarnessHead
            auditedGameplayRelation = 'equal'
            auditedGameplayDistance = 0
            embeddedImplementation = [pscustomobject][ordered]@{
                sha = $runHarnessHead
                utc = '2026-07-18T23:59:00Z'
                label = 'focused-aggregate-selftest'
                relation = 'equal'
                distance = 0
            }
            campaignSchema = $CampaignSchema
            runtimeSettingsSchema = $RuntimeSettingsSchema
        }
        addon = [pscustomobject][ordered]@{
            id = 'histasi'
            guid = '698532771130111D'
            title = 'Partisan'
            revision = 'unpublished-local-pack'
            version = $candidateVersion
            dependencies = @('591AF5BDA9F7CE8B')
        }
        toolchain = [pscustomobject][ordered]@{
            workbench = $workbench
            server = $server
            serverDiagnostic = $serverDiagnostic
            client = $client
            clientDiagnostic = $clientDiagnostic
        }
        workbench = [pscustomobject][ordered]@{
            crc = $workbenchCrc
            targets = $workbenchTargets
        }
        package = [pscustomobject][ordered]@{
            root = 'package/Partisan'
            hashAlgorithm = 'sha256-manifest-v1'
            sha256 = $packageSha
            canonicalIndexPath = 'evidence/pack/files.sha256'
            files = $packageFiles
        }
        evidence = [pscustomobject][ordered]@{
            root = 'evidence'
            files = @([pscustomobject][ordered]@{
                path = 'evidence/foundation.json'
                length = 105
                sha256 = 'b' * 64
            })
        }
    }
    $candidateRoot = Join-Path $Root (
        'docs/evidence/release-candidates/' + $candidateId)
    $manifestPath = Join-Path $candidateRoot 'candidate.json'
    $readyPath = Join-Path $candidateRoot 'candidate.ready.json'
    Write-SelfTestJson -Path $manifestPath -Value $manifest
    $manifestSha = Get-SelfTestSha256 -Path $manifestPath
    $ready = [pscustomobject][ordered]@{
        schemaVersion = 1
        candidateId = $candidateId
        gitHead = $runHarnessHead
        packageSha256 = $manifest.package.sha256
        manifestSha256 = $manifestSha
    }
    Write-SelfTestJson -Path $readyPath -Value $ready
    $readySha = Get-SelfTestSha256 -Path $readyPath
    $releaseStatus = [pscustomobject][ordered]@{
        schemaVersion = 3
        statusAsOfUtc = '2026-07-19T00:01:30.0000000Z'
        releaseStage = 'focused-aggregate-selftest'
        releaseDecision = 'NO-GO'
        auditedRevision = $runHarnessHead
        baseline = [pscustomobject][ordered]@{}
        artifact = [pscustomobject][ordered]@{
            releaseCandidateBuilt = $true
            runtimeUseDisposition = 'active-runtime-candidate'
            candidateId = $candidateId
            candidateSourceHead = $runHarnessHead
            manifestPath = 'docs/evidence/release-candidates/' +
                $candidateId + '/candidate.json'
            manifestSha256 = $manifestSha
            readySha256 = $readySha
            packageHashAlgorithm = $manifest.package.hashAlgorithm
            packageSha256 = $manifest.package.sha256
            packageVersion = $candidateVersion
            addonGuid = $manifest.addon.guid
            addonRevision = $manifest.addon.revision
            workbenchVersion = $workbench.productVersion
            workbenchSha256 = $workbench.sha256
            workbenchCrc = $workbenchCrc
            serverVersion = $server.productVersion
            clientVersion = $client.productVersion
            note = 'Self-test active candidate.'
        }
        evidence = [pscustomobject][ordered]@{}
        historicalCandidateEvidence = @()
        proofRungs = @()
        activeBlockers = @()
    }
    $statusPath = Join-Path $Root 'docs/data/release_status.json'
    Write-SelfTestJson -Path $statusPath -Value $releaseStatus
    & git -C $Root add -- docs
    & git -C $Root commit --quiet -m 'focused aggregate self-test candidate'
    if ($LASTEXITCODE -ne 0) {
        throw 'Focused aggregate self-test could not commit its candidate fixture.'
    }
    $aggregationHead = ((& git -C $Root rev-parse HEAD) -join '').Trim()
    if ($LASTEXITCODE -ne 0 -or $aggregationHead -cnotmatch '^[0-9a-f]{40}$') {
        throw 'Focused aggregate self-test could not resolve aggregate HEAD.'
    }
    return [pscustomobject][ordered]@{
        Root = $Root
        GitHead = $aggregationHead
        RunHarnessGitHead = $runHarnessHead
        RunnerSha256 = $runnerSha
        CandidateModuleSha256 = $candidateModuleSha
        ManifestPath = $manifestPath
        ReadyPath = $readyPath
        StatusPath = $statusPath
        CandidateId = $candidateId
        OutputPath = Join-Path $Root (
            'docs/evidence/focused-autotest/' + $candidateId + '.json')
    }
}

function Set-SelfTestInvalidPackageCandidate {
    param([Parameter(Mandatory = $true)]$Repository)

    $manifest = Read-SelfTestJson -Path $Repository.ManifestPath
    $manifest.package.files[0].indexPath =
        'Partisan/noncanonical-addon.gproj'
    $manifest.package.sha256 = Get-SelfTestPackageSha256 `
        -Files @($manifest.package.files)
    Write-SelfTestJson -Path $Repository.ManifestPath -Value $manifest
    $manifestSha = Get-SelfTestSha256 -Path $Repository.ManifestPath

    $ready = Read-SelfTestJson -Path $Repository.ReadyPath
    $ready.packageSha256 = $manifest.package.sha256
    $ready.manifestSha256 = $manifestSha
    Write-SelfTestJson -Path $Repository.ReadyPath -Value $ready
    $readySha = Get-SelfTestSha256 -Path $Repository.ReadyPath

    $status = Read-SelfTestJson -Path $Repository.StatusPath
    $status.artifact.packageSha256 = $manifest.package.sha256
    $status.artifact.manifestSha256 = $manifestSha
    $status.artifact.readySha256 = $readySha
    Write-SelfTestJson -Path $Repository.StatusPath -Value $status

    & git -C $Repository.Root add -- docs
    & git -C $Repository.Root commit --quiet -m `
        'coherently sealed noncanonical package fixture'
    if ($LASTEXITCODE -ne 0) {
        throw 'Focused aggregate self-test could not commit its invalid package fixture.'
    }
    $head = ((& git -C $Repository.Root rev-parse HEAD) -join '').Trim()
    if ($LASTEXITCODE -ne 0 -or $head -cnotmatch '^[0-9a-f]{40}$') {
        throw 'Focused aggregate self-test could not resolve invalid package HEAD.'
    }
    $Repository.GitHead = $head
}

function New-SelfTestTrackedHistory {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$CurrentAggregatePath
    )

    $historical = Read-SelfTestJson -Path $CurrentAggregatePath
    $currentCandidateId = [string]$historical.candidate.candidateId
    $historicalCandidateId =
        'partisan-rc-historical-20260718T000000Z'
    $historical.candidate.candidateId = $historicalCandidateId
    foreach ($sourceRun in @($historical.sourceRuns)) {
        $expectedPrefix = $currentCandidateId + '/focused-autotest/'
        Assert-SelfTest `
            -Condition (([string]$sourceRun.runEnvelopePath).StartsWith(
                $expectedPrefix,
                [StringComparison]::Ordinal)) `
            -Message 'Historical fixture source path did not use the current candidate prefix.'
        $sourceRun.runEnvelopePath = $historicalCandidateId + '/' +
            ([string]$sourceRun.runEnvelopePath).Substring(
                $currentCandidateId.Length + 1)
    }
    $historicalEnvelopeSha = 'c' * 64
    $historical.cases[0].envelopeSha256 = $historicalEnvelopeSha
    $historical.sourceRuns[0].runEnvelopeSha256 = $historicalEnvelopeSha
    $historical.aggregateId = Get-SelfTestAggregateId -Aggregate $historical

    $historicalPath = Join-Path $RepositoryRoot (
        'docs/evidence/focused-autotest/' + $historicalCandidateId + '.json')
    Write-SelfTestJson -Path $historicalPath -Value $historical
    $historicalRepositoryPath = 'docs/evidence/focused-autotest/' +
        $historicalCandidateId + '.json'
    & git -C $RepositoryRoot add -- $historicalRepositoryPath
    & git -C $RepositoryRoot commit --quiet -m 'focused aggregate self-test history'
    if ($LASTEXITCODE -ne 0) {
        throw 'Focused aggregate self-test could not commit tracked history.'
    }
    return $historicalPath
}

function New-SelfTestTrackedSchemaOneHistory {
    param([Parameter(Mandatory = $true)][string]$RepositoryRoot)

    $candidateId = 'partisan-schema1-historical-20260717T000000Z'
    $relative = 'docs/evidence/focused-autotest/' + $candidateId + '.json'
    $full = Join-Path $RepositoryRoot $relative.Replace('/', '\')
    $value = [pscustomobject][ordered]@{
        schemaVersion = 1
        evidenceKind = 'packaged-focused-autotest-set'
        aggregateId = 'legacy-focused-set-preserved'
        candidate = [pscustomobject][ordered]@{
            candidateId = $candidateId
        }
        preservationToken = 'schema-one-byte-history'
    }
    Write-SelfTestJson -Path $full -Value $value
    & git -C $RepositoryRoot add -- $relative
    & git -C $RepositoryRoot commit --quiet -m 'focused schema-one history'
    if ($LASTEXITCODE -ne 0) {
        throw 'Focused aggregate self-test could not commit schema-one history.'
    }
    return $full
}

function Copy-SelfTestRepository {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    & git -c core.autocrlf=false clone `
        --quiet `
        --no-hardlinks `
        -- `
        $Source `
        $Destination
    if ($LASTEXITCODE -ne 0) {
        throw 'Focused aggregate self-test could not clone an isolated history case.'
    }
    & git -C $Destination config core.autocrlf false
    & git -C $Destination config user.name 'Partisan Focused Aggregate Self-Test'
    & git -C $Destination config user.email 'focused-aggregate-selftest@invalid.example'
    return $Destination
}

function Reset-SelfTestOutput {
    param([Parameter(Mandatory = $true)][string]$OutputPath)

    foreach ($path in @(
            $OutputPath,
            ($OutputPath + '.replacement-required.json'))) {
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            Remove-Item -LiteralPath $path -Force
        }
    }
}

function Remove-SelfTestJunction {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$ContainmentRoot
    )

    $full = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    $root = [IO.Path]::GetFullPath($ContainmentRoot).TrimEnd('\', '/')
    $prefix = $root + [IO.Path]::DirectorySeparatorChar
    Assert-SelfTest `
        -Condition ($full.StartsWith(
            $prefix,
            [StringComparison]::OrdinalIgnoreCase)) `
        -Message 'Self-test junction cleanup escaped its temporary root.'
    if (-not (Test-Path -LiteralPath $full)) {
        return
    }
    $item = Get-Item -LiteralPath $full -Force
    Assert-SelfTest `
        -Condition ($item.PSIsContainer -and
            ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) `
        -Message 'Self-test junction cleanup target is not a reparse directory.'
    [IO.Directory]::Delete($full)
    Assert-SelfTest `
        -Condition (-not (Test-Path -LiteralPath $full)) `
        -Message 'Self-test junction cleanup did not remove the bounded link.'
}

function Remove-SelfTestContainedDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$ContainmentRoot
    )

    $full = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    $root = [IO.Path]::GetFullPath($ContainmentRoot).TrimEnd('\', '/')
    $prefix = $root + [IO.Path]::DirectorySeparatorChar
    Assert-SelfTest `
        -Condition ($full.StartsWith(
            $prefix,
            [StringComparison]::OrdinalIgnoreCase)) `
        -Message 'Self-test directory cleanup escaped its temporary root.'
    if (-not (Test-Path -LiteralPath $full)) {
        return
    }
    $item = Get-Item -LiteralPath $full -Force
    Assert-SelfTest `
        -Condition ($item.PSIsContainer -and
            ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -eq 0) `
        -Message 'Self-test directory cleanup target is not a regular directory.'
    Remove-Item `
        -LiteralPath $full `
        -Recurse `
        -Force `
        -ErrorAction Stop
    Assert-SelfTest `
        -Condition (-not (Test-Path -LiteralPath $full)) `
        -Message 'Self-test contained directory cleanup did not converge.'
}

function New-SelfTestCaseFixture {
    param(
        [Parameter(Mandatory = $true)][string]$PristineEvidence,
        [Parameter(Mandatory = $true)][string]$CaseRoot
    )

    $evidence = Join-Path $CaseRoot 'evidence'
    Copy-SelfTestFixture `
        -Source $PristineEvidence `
        -Destination $evidence
    return [pscustomobject][ordered]@{
        EvidenceRoot = $evidence
        RunPaths = @(Get-SelfTestRunPaths -EvidenceRoot $evidence)
    }
}

try {
    $producerTokens = $null
    $producerErrors = $null
    [void][Management.Automation.Language.Parser]::ParseFile(
        (Resolve-Path $producer),
        [ref]$producerTokens,
        [ref]$producerErrors)
    Assert-SelfTest `
        -Condition ($producerErrors.Count -eq 0) `
        -Message 'Focused aggregate producer does not parse.'

    $tempRoot = Join-Path `
        ([IO.Path]::GetTempPath()) `
        ('PartisanFocusedAggregateSelfTest-' + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $tempRoot | Out-Null
    Test-SelfTestWritableFileCopy -Root $tempRoot
    $repository = New-SelfTestRepository `
        -Root (Join-Path $tempRoot 'repository')
    $pristine = New-SelfTestFixture `
        -Root (Join-Path $tempRoot 'pristine') `
        -HarnessGitHead $repository.RunHarnessGitHead `
        -RunnerSha256 $repository.RunnerSha256 `
        -CandidateModuleSha256 $repository.CandidateModuleSha256 `
        -TrackedManifestPath $repository.ManifestPath `
        -TrackedReadyPath $repository.ReadyPath

    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $accepted = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTest `
        -Condition $accepted.Succeeded `
        -Message ('Valid focused aggregate fixture was rejected: ' + $accepted.Error)
    Assert-SelfTest `
        -Condition (Test-Path -LiteralPath $repository.OutputPath -PathType Leaf) `
        -Message 'Valid focused aggregate fixture did not publish its canonical output.'
    $acceptedHash = Get-SelfTestSha256 -Path $repository.OutputPath
    $aggregate = Read-SelfTestJson -Path $repository.OutputPath
    Assert-SelfTest `
        -Condition ([long]$aggregate.schemaVersion -eq 2 -and
            [string]$aggregate.evidenceKind -ceq
                'packaged-focused-autotest-set' -and
            [string]$aggregate.aggregationPolicy.contractId -ceq
                'partisan.focused-autotest.aggregate.v2' -and
            [long]$aggregate.aggregationPolicy.policyVersion -eq 2) `
        -Message 'Accepted focused aggregate did not use the canonical v2 contract.'
    Assert-SelfTest `
        -Condition ([string]$aggregate.admission.status -ceq 'accepted' -and
            [string]$aggregate.admission.disposition -ceq
                'accepted-noncertifying' -and
            [string]$aggregate.admission.releaseDecision -ceq 'NO-GO' -and
            -not [bool]$aggregate.admission.certifying) `
        -Message 'Accepted focused aggregate widened its non-certifying disposition.'
    Assert-SelfTest `
        -Condition ([long]$aggregate.result.caseCount -eq 5 -and
            [long]$aggregate.result.passedCases -eq 5 -and
            [long]$aggregate.result.junitTests -eq 91 -and
            [long]$aggregate.result.junitFailures -eq 0 -and
            [long]$aggregate.result.junitErrors -eq 0 -and
            [long]$aggregate.result.junitSkipped -eq 0) `
        -Message 'Accepted focused aggregate did not prove JUnit 91/0/0/0.'
    Assert-SelfTest `
        -Condition ([long]$aggregate.result.envelopeFileCount -eq 40 -and
            @($aggregate.sourceRuns).Count -eq 5 -and
            @($aggregate.sourceRuns | ForEach-Object { @($_.files) }).Count -eq 40 -and
            @($aggregate.sourceRuns | Where-Object {
                [long]$_.fileCount -ne 8
            }).Count -eq 0) `
        -Message 'Accepted focused aggregate did not bind five times eight blobs.'
    Assert-SelfTest `
        -Condition ([long]$aggregate.preliminaryRuns.caseCount -eq 0 -and
            [string]$aggregate.preliminaryRuns.status -ceq 'none' -and
            [string]$aggregate.preliminaryRuns.note -clike
                'The exact current-candidate focused-tree census*') `
        -Message 'Accepted focused aggregate did not retain a truthful full-tree census.'
    Assert-SelfTest `
        -Condition ([long]$aggregate.aggregatePolicyAssertions.total -eq 35 -and
            [long]$aggregate.aggregatePolicyAssertions.passed -eq 35 -and
            [long]$aggregate.aggregatePolicyAssertions.failed -eq 0 -and
            @($aggregate.aggregatePolicyAssertions.assertions).Count -eq 35 -and
            [string]$aggregate.aggregatePolicyAssertions.assertionClass -ceq
                'aggregate-policy') `
        -Message 'Accepted focused aggregate did not prove distinct aggregate-policy 35/35.'
    Assert-SelfTest `
        -Condition ((@($aggregate.cases | ForEach-Object { $_.testCase }) -join '|') -ceq
            ($profileOrder -join '|')) `
        -Message 'Accepted focused aggregate did not retain canonical profile order.'
    $aggregateText = [IO.File]::ReadAllText($repository.OutputPath)
    Assert-SelfTest `
        -Condition ($aggregateText -notmatch
            '(?i)(?:[A-Z]:[\\/]|\\\\|file://|/(?:Users|home|mnt)/)') `
        -Message 'Accepted focused aggregate leaked a nonportable path.'
    Assert-SelfTest `
        -Condition ([bool]$aggregate.integrity.allWorktreeHashesMatchGitBlobs -and
            [string]$aggregate.integrity.focusedRunHarness.focusedRunnerWorktreeSha256 -ceq
                [string]$aggregate.integrity.focusedRunHarness.focusedRunnerGitBlobSha256 -and
            [string]$aggregate.integrity.focusedRunHarness.candidateModuleWorktreeSha256 -ceq
                [string]$aggregate.integrity.focusedRunHarness.candidateModuleGitBlobSha256 -and
            [string]$aggregate.integrity.aggregateProducer.worktreeSha256 -ceq
                [string]$aggregate.integrity.aggregateProducer.gitBlobSha256 -and
            [string]$aggregate.integrity.releaseDocsConsumer.worktreeSha256 -ceq
                [string]$aggregate.integrity.releaseDocsConsumer.gitBlobSha256) `
        -Message 'Accepted focused aggregate omitted exact worktree/Git-blob provenance.'
    Assert-SelfTest `
        -Condition ([string]$aggregate.harness.gitHead -ceq
                $repository.RunHarnessGitHead -and
            [string]$aggregate.integrity.focusedRunHarness.gitHead -ceq
                $repository.RunHarnessGitHead -and
            [string]$aggregate.integrity.aggregationGitHead -ceq
                $repository.GitHead -and
            $repository.RunHarnessGitHead -cne $repository.GitHead) `
        -Message 'Accepted focused aggregate did not retain distinct run and aggregation heads.'
    Assert-SelfTest `
        -Condition ((@($aggregate.sourceRuns | ForEach-Object {
                    $_.runEnvelopePath
                }) -join '|') -ceq
            (@($aggregate.sourceRuns | ForEach-Object {
                    $repository.CandidateId + '/focused-autotest/' +
                        $_.testCase + '/' + $_.runId + '/run.json'
                }) -join '|')) `
        -Message 'Accepted focused aggregate did not retain canonical external run paths.'
    Assert-SelfTest `
        -Condition ([string]$aggregate.aggregateId -ceq
                (Get-SelfTestAggregateId -Aggregate $aggregate) -and
            [long]$aggregate.result.hardDiagnosticClassifierChecksPerRun -eq 12 -and
            [long]$aggregate.result.hardDiagnosticCount -eq 51 -and
            [long]$aggregate.result.approvedStockFilterDiagnosticCount -eq 10 -and
            [long]$aggregate.result.approvedIntentionalFaultDiagnosticCount -eq 41 -and
            [long]$aggregate.result.unapprovedHardDiagnosticCount -eq 0) `
        -Message 'Accepted focused aggregate did not re-derive its ID and raw diagnostic census.'

    $legacyCurrentRoot = Copy-SelfTestRepository `
        -Source $repository.Root `
        -Destination (Join-Path $tempRoot 'current-legacy-wrapper-shape')
    $legacyCurrentOutput = Join-Path $legacyCurrentRoot (
        'docs/evidence/focused-autotest/' + $repository.CandidateId + '.json')
    $legacyCurrentOutputParent = Split-Path -Parent $legacyCurrentOutput
    if (-not (Test-Path -LiteralPath $legacyCurrentOutputParent)) {
        New-Item -ItemType Directory -Path $legacyCurrentOutputParent -Force |
            Out-Null
    }
    Copy-SelfTestWritableFile `
        -Source $repository.OutputPath `
        -Destination $legacyCurrentOutput
    $legacyCurrentAggregate = Read-SelfTestJson -Path $legacyCurrentOutput
    $legacyCurrentAggregate.result.junitTests = 5
    Write-SelfTestJson `
        -Path $legacyCurrentOutput `
        -Value $legacyCurrentAggregate
    $legacyCurrent = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $legacyCurrentOutput `
        -RepositoryRoot $legacyCurrentRoot
    Assert-SelfTest `
        -Condition (-not $legacyCurrent.Succeeded -and
            $legacyCurrent.Error -cmatch
                'retired five-wrapper contract outside immutable tracked history') `
        -Message 'A current governed candidate accepted the retired five-wrapper shape.'

    $idempotent = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTest `
        -Condition ($idempotent.Succeeded -and
            (Get-SelfTestSha256 -Path $repository.OutputPath) -ceq $acceptedHash) `
        -Message 'Identical focused aggregate publication was not byte-idempotent.'

    $badPackageRepository = New-SelfTestRepository `
        -Root (Join-Path $tempRoot 'invalid-package-repository')
    Set-SelfTestInvalidPackageCandidate -Repository $badPackageRepository
    $badPackageEvidence = New-SelfTestFixture `
        -Root (Join-Path $tempRoot 'invalid-package-evidence') `
        -HarnessGitHead $badPackageRepository.RunHarnessGitHead `
        -RunnerSha256 $badPackageRepository.RunnerSha256 `
        -CandidateModuleSha256 $badPackageRepository.CandidateModuleSha256 `
        -TrackedManifestPath $badPackageRepository.ManifestPath `
        -TrackedReadyPath $badPackageRepository.ReadyPath
    Reset-SelfTestOutput -OutputPath $badPackageRepository.OutputPath
    $badPackage = Invoke-SelfTestProducer `
        -EvidenceRoot $badPackageEvidence.EvidenceRoot `
        -RunPaths $badPackageEvidence.RunPaths `
        -OutputPath $badPackageRepository.OutputPath `
        -RepositoryRoot $badPackageRepository.Root
    Assert-SelfTest `
        -Condition (-not $badPackage.Succeeded -and
            $badPackage.Error -cmatch '\(candidate_tampering\)' -and
            -not (Test-Path -LiteralPath $badPackageRepository.OutputPath)) `
        -Message 'A coherently sealed and committed noncanonical package was accepted.'
    Assert-SelfTestRejected `
        $badPackage `
        $badPackageRepository.OutputPath `
        'candidate_tampering' `
        -NoReceipt

    $reservedCandidateRepository = New-SelfTestRepository `
        -Root (Join-Path $tempRoot 'reserved-candidate-repository') `
        -CandidateId 'partisan-rc-reserved.JSON.Replacement-Required'
    $reservedCandidateEvidence = New-SelfTestFixture `
        -Root (Join-Path $tempRoot 'reserved-candidate-evidence') `
        -HarnessGitHead $reservedCandidateRepository.RunHarnessGitHead `
        -RunnerSha256 $reservedCandidateRepository.RunnerSha256 `
        -CandidateModuleSha256 $reservedCandidateRepository.CandidateModuleSha256 `
        -TrackedManifestPath $reservedCandidateRepository.ManifestPath `
        -TrackedReadyPath $reservedCandidateRepository.ReadyPath
    $reservedCandidate = Invoke-SelfTestProducer `
        -EvidenceRoot $reservedCandidateEvidence.EvidenceRoot `
        -RunPaths $reservedCandidateEvidence.RunPaths `
        -OutputPath $reservedCandidateRepository.OutputPath `
        -RepositoryRoot $reservedCandidateRepository.Root
    Assert-SelfTest `
        -Condition (-not (Test-Path `
            -LiteralPath $reservedCandidateRepository.OutputPath)) `
        -Message 'Reserved-suffix candidate ID published an ambiguous aggregate path.'
    Assert-SelfTestRejected `
        $reservedCandidate `
        $reservedCandidateRepository.OutputPath `
        'candidate_tampering' `
        -NoReceipt

    $wrongOutputPath = Join-Path `
        $repository.Root `
        'docs/evidence/focused-autotest/wrong-candidate.json'
    Reset-SelfTestOutput -OutputPath $wrongOutputPath
    $wrongOutput = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $wrongOutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTest `
        -Condition (-not $wrongOutput.Succeeded -and
            $wrongOutput.Error -match '\(output_path_invalid\):') `
        -Message 'Noncanonical output path did not fail with output_path_invalid.'
    $wrongOutputRejectionLines = @($wrongOutput.Output | Where-Object {
        $_ -is [string] -and
        $_.StartsWith(
            'FOCUSED_AGGREGATE_REJECTED ',
            [StringComparison]::Ordinal)
    })
    Assert-SelfTest `
        -Condition ($wrongOutputRejectionLines.Count -eq 1) `
        -Message 'Noncanonical output path did not emit one rejection payload.'
    $wrongOutputRejection = $wrongOutputRejectionLines[0].Substring(
        'FOCUSED_AGGREGATE_REJECTED '.Length) | ConvertFrom-Json
    $durableProperty = $wrongOutputRejection.PSObject.Properties[
        'durableReceiptAvailable']
    Assert-SelfTest `
        -Condition ($durableProperty -and
            $durableProperty.Value -is [bool] -and
            -not $durableProperty.Value) `
        -Message 'Noncanonical output path incorrectly claimed a durable receipt.'
    Assert-SelfTest `
        -Condition (-not (Test-Path -LiteralPath (
                $wrongOutputPath + '.replacement-required.json'))) `
        -Message 'Noncanonical output path wrote a durable rejection receipt.'
    Reset-SelfTestOutput -OutputPath $wrongOutputPath

    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $publicationResiduePath = Join-Path `
        (Split-Path -Parent $repository.OutputPath) `
        ('.' + (Split-Path -Leaf $repository.OutputPath) + '.' +
            ('e' * 32) + '.tmp')
    Write-SelfTestText -Path $publicationResiduePath -Text "residue`n"
    try {
        $publicationResidue = Invoke-SelfTestProducer `
            -EvidenceRoot $pristine.EvidenceRoot `
            -RunPaths $pristine.RunPaths `
            -OutputPath $repository.OutputPath `
            -RepositoryRoot $repository.Root
        Assert-SelfTestRejected `
            $publicationResidue `
            $repository.OutputPath `
            'publication_cleanup_failed'
    }
    finally {
        Remove-Item `
            -LiteralPath $publicationResiduePath `
            -Force `
            -ErrorAction Stop
        if (Test-Path -LiteralPath $publicationResiduePath) {
            throw 'Focused aggregate self-test could not remove publication residue.'
        }
    }
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $concurrentArgument = [pscustomobject][ordered]@{
        ProducerPath = $producer
        EvidenceRoot = $pristine.EvidenceRoot
        RunPaths = @($pristine.RunPaths)
        OutputPath = $repository.OutputPath
        RepositoryRoot = $repository.Root
    }
    $concurrentJobs = @(1..2 | ForEach-Object {
        Start-Job -ScriptBlock {
            param($Fixture)
            $ErrorActionPreference = 'Stop'
            & $Fixture.ProducerPath `
                -EvidenceRoot $Fixture.EvidenceRoot `
                -RunJson @($Fixture.RunPaths) `
                -OutputPath $Fixture.OutputPath `
                -RepositoryRoot $Fixture.RepositoryRoot
        } -ArgumentList $concurrentArgument
    })
    try {
        $concurrentJobs | Wait-Job | Out-Null
        Assert-SelfTest `
            -Condition (@($concurrentJobs | Where-Object {
                $_.State -cne 'Completed'
            }).Count -eq 0) `
            -Message 'Concurrent identical aggregate publication did not complete.'
        $concurrentLines = @($concurrentJobs | Receive-Job)
        $concurrentReceipts = @($concurrentLines | Where-Object {
            [string]$_ -clike 'FOCUSED_AGGREGATE *'
        } | ForEach-Object {
            ([string]$_).Substring('FOCUSED_AGGREGATE '.Length) |
                ConvertFrom-Json
        })
        Assert-SelfTest `
            -Condition ($concurrentReceipts.Count -eq 2 -and
                @($concurrentReceipts | Where-Object {
                    [bool]$_.created
                }).Count -eq 1 -and
                @($concurrentReceipts | Where-Object {
                    -not [bool]$_.created
                }).Count -eq 1 -and
                @($concurrentReceipts.outputSha256 | Sort-Object -Unique).
                    Count -eq 1) `
            -Message 'Concurrent identical publication did not produce one creator and one byte-identical race loser.'
    }
    finally {
        $concurrentJobs | Remove-Job -Force -ErrorAction SilentlyContinue
    }

    $canonicalBytes = [IO.File]::ReadAllBytes($repository.OutputPath)
    $bomBytes = New-Object byte[] ($canonicalBytes.Length + 3)
    $bomBytes[0] = 0xEF
    $bomBytes[1] = 0xBB
    $bomBytes[2] = 0xBF
    [Array]::Copy($canonicalBytes, 0, $bomBytes, 3, $canonicalBytes.Length)
    [IO.File]::WriteAllBytes($repository.OutputPath, $bomBytes)
    $bomOutput = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $bomOutput $repository.OutputPath 'historical_blob_replacement' -NoReceipt

    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $duplicateRuns = @(
        $pristine.RunPaths[0],
        $pristine.RunPaths[1],
        $pristine.RunPaths[2],
        $pristine.RunPaths[3],
        $pristine.RunPaths[3]
    )
    $duplicate = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $duplicateRuns `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $duplicate $repository.OutputPath 'profile_set_invalid' -NoReceipt

    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $missing = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths @($pristine.RunPaths[0..3]) `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $missing $repository.OutputPath 'profile_set_invalid' -NoReceipt

    $extraRunCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'unselected-sibling-run')
    $selectedRunRoot = Split-Path -Parent $extraRunCase.RunPaths[0]
    $unselectedRunRoot = Join-Path `
        (Split-Path -Parent $selectedRunRoot) `
        ('20260719T000219Z-' + ('f' * 32))
    Copy-Item `
        -LiteralPath $selectedRunRoot `
        -Destination $unselectedRunRoot `
        -Recurse `
        -Force
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $extraRun = Invoke-SelfTestProducer `
        -EvidenceRoot $extraRunCase.EvidenceRoot `
        -RunPaths $extraRunCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $extraRun $repository.OutputPath 'focused_tree_census_drift'

    $runBomCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'run-bom')
    Add-SelfTestUtf8Bom -Path $runBomCase.RunPaths[0]
    $runBomBytesBefore = [IO.File]::ReadAllBytes($runBomCase.RunPaths[0])
    $runBomShaBefore = Get-SelfTestSha256 -Path $runBomCase.RunPaths[0]
    Assert-SelfTest `
        -Condition ($runBomBytesBefore.Length -ge 3 -and
            $runBomBytesBefore[0] -eq 0xEF -and
            $runBomBytesBefore[1] -eq 0xBB -and
            $runBomBytesBefore[2] -eq 0xBF) `
        -Message 'Run-BOM negative fixture did not contain a UTF-8 BOM.'
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $runBom = Invoke-SelfTestProducer `
        -EvidenceRoot $runBomCase.EvidenceRoot `
        -RunPaths $runBomCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    $runBomBytesAfter = [IO.File]::ReadAllBytes($runBomCase.RunPaths[0])
    $runBomShaAfter = Get-SelfTestSha256 -Path $runBomCase.RunPaths[0]
    Assert-SelfTest `
        -Condition ($runBomBytesAfter.Length -ge 3 -and
            $runBomBytesAfter[0] -eq 0xEF -and
            $runBomBytesAfter[1] -eq 0xBB -and
            $runBomBytesAfter[2] -eq 0xBF -and
            $runBomShaAfter -ceq $runBomShaBefore) `
        -Message 'Focused producer changed the Run-BOM negative fixture.'
    Assert-SelfTest `
        -Condition (-not $runBom.Succeeded) `
        -Message 'Run-BOM negative fixture unexpectedly passed schema validation.'
    Assert-SelfTestRejected `
        $runBom $repository.OutputPath 'schema_drift' -NoReceipt

    $duplicateJsonCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'duplicate-json-property')
    $duplicateJsonText = [IO.File]::ReadAllText($duplicateJsonCase.RunPaths[0])
    $schemaVersionLinePattern =
        '(?m)^(?<indent>[ \t]*)"schemaVersion"[ \t]*:[ \t]*1[ \t]*,[ \t]*$'
    $schemaVersionLineMatches = @([regex]::Matches(
        $duplicateJsonText,
        $schemaVersionLinePattern))
    Assert-SelfTest `
        -Condition ($schemaVersionLineMatches.Count -eq 1) `
        -Message 'Duplicate-property fixture did not contain one schemaVersion line.'
    $schemaVersionLine = $schemaVersionLineMatches[0]
    $duplicateSchemaLine = "`n" +
        $schemaVersionLine.Groups['indent'].Value + '"schemaVersion": 1,'
    $duplicateJsonText = $duplicateJsonText.Insert(
        $schemaVersionLine.Index + $schemaVersionLine.Length,
        $duplicateSchemaLine)
    Assert-SelfTest `
        -Condition ([regex]::Matches(
            $duplicateJsonText,
            '"schemaVersion"[ \t]*:').Count -eq 2) `
        -Message 'Duplicate-property fixture did not insert exactly one duplicate.'
    Write-SelfTestText `
        -Path $duplicateJsonCase.RunPaths[0] `
        -Text $duplicateJsonText
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $duplicateJson = Invoke-SelfTestProducer `
        -EvidenceRoot $duplicateJsonCase.EvidenceRoot `
        -RunPaths $duplicateJsonCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTest `
        -Condition (-not $duplicateJson.Succeeded) `
        -Message 'Duplicate-property run envelope unexpectedly passed schema validation.'
    Assert-SelfTestRejected `
        $duplicateJson $repository.OutputPath 'schema_drift' -NoReceipt

    $rawBomCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'raw-bom')
    $rawBomRun = Read-SelfTestJson -Path $rawBomCase.RunPaths[0]
    $rawBomPortable = [string](@($rawBomRun.files | Where-Object {
        $_.path -cmatch '/console\.log$'
    })[0].path)
    $rawBomFull = Join-Path `
        (Split-Path -Parent $rawBomCase.RunPaths[0]) `
        $rawBomPortable.Replace('/', '\')
    Add-SelfTestUtf8Bom -Path $rawBomFull
    Update-SelfTestRunIndex `
        -RunPath $rawBomCase.RunPaths[0] `
        -PortablePath $rawBomPortable
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $rawBom = Invoke-SelfTestProducer `
        -EvidenceRoot $rawBomCase.EvidenceRoot `
        -RunPaths $rawBomCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $rawBom $repository.OutputPath 'raw_result_tampering'

    $sourcePathCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'source-path')
    $sourceRunRoot = Split-Path -Parent $sourcePathCase.RunPaths[0]
    $sourceRunParent = Split-Path -Parent $sourceRunRoot
    $noncanonicalParent = Join-Path $sourceRunParent 'noncanonical'
    New-Item -ItemType Directory -Path $noncanonicalParent -Force | Out-Null
    $movedRunRoot = Join-Path $noncanonicalParent (Split-Path -Leaf $sourceRunRoot)
    Move-Item -LiteralPath $sourceRunRoot -Destination $movedRunRoot
    $sourcePathCase.RunPaths[0] = Join-Path $movedRunRoot 'run.json'
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $sourcePathDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $sourcePathCase.EvidenceRoot `
        -RunPaths $sourcePathCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $sourcePathDrift $repository.OutputPath 'source_path_invalid'

    $inputJunctionCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'input-junction')
    $inputJunctionRunRoot = Split-Path -Parent $inputJunctionCase.RunPaths[0]
    $inputJunctionPath = Join-Path $inputJunctionRunRoot 'raw'
    $inputJunctionTarget = Join-Path $tempRoot 'input-junction-target'
    Move-Item `
        -LiteralPath $inputJunctionPath `
        -Destination $inputJunctionTarget
    try {
        New-Item `
            -ItemType Junction `
            -Path $inputJunctionPath `
            -Value $inputJunctionTarget | Out-Null
        Reset-SelfTestOutput -OutputPath $repository.OutputPath
        $inputJunction = Invoke-SelfTestProducer `
            -EvidenceRoot $inputJunctionCase.EvidenceRoot `
            -RunPaths $inputJunctionCase.RunPaths `
            -OutputPath $repository.OutputPath `
            -RepositoryRoot $repository.Root
        Assert-SelfTestRejected `
            $inputJunction $repository.OutputPath 'reparse_path'
    }
    finally {
        if (Test-Path -LiteralPath $inputJunctionPath) {
            Remove-SelfTestJunction `
                -Path $inputJunctionPath `
                -ContainmentRoot $tempRoot
        }
        if (Test-Path -LiteralPath $inputJunctionTarget -PathType Container) {
            Move-Item `
                -LiteralPath $inputJunctionTarget `
                -Destination $inputJunctionPath
        }
    }

    $diagnosticTailCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'diagnostic-tail')
    $diagnosticTailRun = Read-SelfTestJson -Path $diagnosticTailCase.RunPaths[0]
    $diagnosticTailRun.outcome.diagnosticTail[0] = 'HST_tampered_tail'
    Write-SelfTestJson `
        -Path $diagnosticTailCase.RunPaths[0] `
        -Value $diagnosticTailRun
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $diagnosticTailDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $diagnosticTailCase.EvidenceRoot `
        -RunPaths $diagnosticTailCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $diagnosticTailDrift $repository.OutputPath 'diagnostic_tail_invalid'

    $candidateCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'candidate-drift')
    $candidateRun = Read-SelfTestJson -Path $candidateCase.RunPaths[1]
    $candidateRun.candidate.packageSha256 = 'f' * 64
    Write-SelfTestJson -Path $candidateCase.RunPaths[1] -Value $candidateRun
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $candidateDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $candidateCase.EvidenceRoot `
        -RunPaths $candidateCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $candidateDrift $repository.OutputPath 'candidate_identity_drift' -NoReceipt

    $harnessCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'harness-drift')
    $harnessRun = Read-SelfTestJson -Path $harnessCase.RunPaths[2]
    $harnessRun.harness.focusedRunnerSha256 = 'f' * 64
    Write-SelfTestJson -Path $harnessCase.RunPaths[2] -Value $harnessRun
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $harnessDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $harnessCase.EvidenceRoot `
        -RunPaths $harnessCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $harnessDrift $repository.OutputPath 'harness_identity_drift'

    $missingHarnessCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'harness-missing-commit')
    foreach ($runPath in $missingHarnessCase.RunPaths) {
        $missingHarnessRun = Read-SelfTestJson -Path $runPath
        $missingHarnessRun.harness.gitHead = 'f' * 40
        Write-SelfTestJson -Path $runPath -Value $missingHarnessRun
    }
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $missingHarness = Invoke-SelfTestProducer `
        -EvidenceRoot $missingHarnessCase.EvidenceRoot `
        -RunPaths $missingHarnessCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $missingHarness $repository.OutputPath 'harness_identity_drift'

    $harnessTree = ((& git -C $repository.Root rev-parse `
        ($repository.RunHarnessGitHead + '^{tree}')) -join '').Trim()
    $orphanHarnessHead = ((Write-Output 'orphan focused harness' |
        & git -C $repository.Root commit-tree $harnessTree) -join '').Trim()
    Assert-SelfTest `
        -Condition ($LASTEXITCODE -eq 0 -and
            $orphanHarnessHead -cmatch '^[0-9a-f]{40}$') `
        -Message 'Focused aggregate self-test could not create an orphan harness commit.'
    $orphanHarnessCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'harness-orphan-commit')
    foreach ($runPath in $orphanHarnessCase.RunPaths) {
        $orphanHarnessRun = Read-SelfTestJson -Path $runPath
        $orphanHarnessRun.harness.gitHead = $orphanHarnessHead
        Write-SelfTestJson -Path $runPath -Value $orphanHarnessRun
    }
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $orphanHarness = Invoke-SelfTestProducer `
        -EvidenceRoot $orphanHarnessCase.EvidenceRoot `
        -RunPaths $orphanHarnessCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $orphanHarness $repository.OutputPath 'harness_identity_drift'

    $blobHarnessCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'harness-blob-drift')
    foreach ($runPath in $blobHarnessCase.RunPaths) {
        $blobRun = Read-SelfTestJson -Path $runPath
        $blobRun.harness.focusedRunnerSha256 = 'f' * 64
        Write-SelfTestJson -Path $runPath -Value $blobRun
    }
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $blobHarnessDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $blobHarnessCase.EvidenceRoot `
        -RunPaths $blobHarnessCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $blobHarnessDrift $repository.OutputPath 'harness_git_blob_mismatch'

    $toolCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'tool-drift')
    $toolRun = Read-SelfTestJson -Path $toolCase.RunPaths[3]
    $toolRun.launch.diagnosticExecutable.sha256 = 'f' * 64
    Write-SelfTestJson -Path $toolCase.RunPaths[3] -Value $toolRun
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $toolDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $toolCase.EvidenceRoot `
        -RunPaths $toolCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $toolDrift $repository.OutputPath 'tool_identity_drift' -NoReceipt

    $rawCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'raw-tamper')
    $rawRun = Read-SelfTestJson -Path $rawCase.RunPaths[0]
    $consolePath = [string](@($rawRun.files | Where-Object {
        $_.path -cmatch '/console\.log$'
    })[0].path)
    $consoleFull = Join-Path `
        (Split-Path -Parent $rawCase.RunPaths[0]) `
        $consolePath.Replace('/', '\')
    [IO.File]::AppendAllText($consoleFull, "tampered`n")
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $rawTamper = Invoke-SelfTestProducer `
        -EvidenceRoot $rawCase.EvidenceRoot `
        -RunPaths $rawCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected $rawTamper $repository.OutputPath 'raw_blob_tampering'

    $requiredContractCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'required-pattern-contract')
    $requiredContractRun = Read-SelfTestJson `
        -Path $requiredContractCase.RunPaths[0]
    $requiredContractPortable = [string](@($requiredContractRun.files |
        Where-Object { $_.path -cmatch '/console\.log$' })[0].path)
    $requiredContractFull = Join-Path `
        (Split-Path -Parent $requiredContractCase.RunPaths[0]) `
        $requiredContractPortable.Replace('/', '\')
    $requiredContractText = @(
        [IO.File]::ReadAllText($requiredContractFull) -split "`r?`n" |
            Where-Object {
                -not ([string]$_).StartsWith(
                    'PARTISAN_REQUIRED_LOG_PATTERN_B64 ',
                    [StringComparison]::Ordinal)
            }) -join "`n"
    Write-SelfTestText `
        -Path $requiredContractFull `
        -Text $requiredContractText
    Update-SelfTestRunIndex `
        -RunPath $requiredContractCase.RunPaths[0] `
        -PortablePath $requiredContractPortable
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $requiredContractDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $requiredContractCase.EvidenceRoot `
        -RunPaths $requiredContractCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $requiredContractDrift $repository.OutputPath 'raw_result_tampering'

    $rawMountCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'raw-mount-attestation')
    $rawMountRun = Read-SelfTestJson -Path $rawMountCase.RunPaths[0]
    $rawMountPortable = [string](@($rawMountRun.files |
        Where-Object { $_.path -cmatch '/console\.log$' })[0].path)
    $rawMountFull = Join-Path `
        (Split-Path -Parent $rawMountCase.RunPaths[0]) `
        $rawMountPortable.Replace('/', '\')
    $rawMountText = @(
        [IO.File]::ReadAllText($rawMountFull) -split "`r?`n" |
            Where-Object {
                ([string]$_).IndexOf(
                    "guid: '$([string] $rawMountRun.candidate.addonGuid)'",
                    [StringComparison]::Ordinal) -lt 0
            }) -join "`n"
    Write-SelfTestText -Path $rawMountFull -Text $rawMountText
    Update-SelfTestRunIndex `
        -RunPath $rawMountCase.RunPaths[0] `
        -PortablePath $rawMountPortable
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $rawMountDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $rawMountCase.EvidenceRoot `
        -RunPaths $rawMountCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $rawMountDrift $repository.OutputPath 'raw_mount_tampering'

    $rawMountGuidCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'raw-mount-guid-case')
    $rawMountGuidRun = Read-SelfTestJson -Path $rawMountGuidCase.RunPaths[0]
    $rawMountGuidPortable = [string](@($rawMountGuidRun.files |
        Where-Object { $_.path -cmatch '/console\.log$' })[0].path)
    $rawMountGuidFull = Join-Path `
        (Split-Path -Parent $rawMountGuidCase.RunPaths[0]) `
        $rawMountGuidPortable.Replace('/', '\')
    $rawMountCanonicalGuid = [string]$rawMountGuidRun.candidate.addonGuid
    $rawMountGuidText = [IO.File]::ReadAllText($rawMountGuidFull).Replace(
        "guid: '$rawMountCanonicalGuid'",
        "guid: '$($rawMountCanonicalGuid.ToLowerInvariant())'")
    Write-SelfTestText -Path $rawMountGuidFull -Text $rawMountGuidText
    Update-SelfTestRunIndex `
        -RunPath $rawMountGuidCase.RunPaths[0] `
        -PortablePath $rawMountGuidPortable
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $rawMountGuidDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $rawMountGuidCase.EvidenceRoot `
        -RunPaths $rawMountGuidCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $rawMountGuidDrift $repository.OutputPath 'raw_mount_tampering'

    $rawMountGuardCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'raw-mount-guard-path')
    $rawMountGuardRun = Read-SelfTestJson -Path $rawMountGuardCase.RunPaths[0]
    $rawMountGuardPortable = [string](@($rawMountGuardRun.files |
        Where-Object { $_.path -cmatch '/console\.log$' })[0].path)
    $rawMountGuardFull = Join-Path `
        (Split-Path -Parent $rawMountGuardCase.RunPaths[0]) `
        $rawMountGuardPortable.Replace('/', '\')
    $rawMountGuardLeaf = Split-Path -Leaf `
        (Split-Path -Parent $rawMountGuardCase.RunPaths[0])
    $rawMountGuardNonce = $rawMountGuardLeaf.Substring(
        $rawMountGuardLeaf.Length - 32)
    $rawMountExpectedGuard = 'PartisanFocusedAutotest/' +
        $rawMountGuardNonce + '/candidate-addons'
    $rawMountWrongGuard = 'OtherFocusedAutotest/' +
        $rawMountGuardNonce + '/candidate-addons'
    $rawMountGuardText = [IO.File]::ReadAllText($rawMountGuardFull).Replace(
        $rawMountExpectedGuard,
        $rawMountWrongGuard)
    if ($rawMountGuardText.IndexOf(
            $rawMountExpectedGuard,
            [StringComparison]::Ordinal) -ge 0) {
        throw 'Focused wrong-guard fixture did not replace the candidate mount path.'
    }
    Write-SelfTestText -Path $rawMountGuardFull -Text $rawMountGuardText
    Update-SelfTestRunIndex `
        -RunPath $rawMountGuardCase.RunPaths[0] `
        -PortablePath $rawMountGuardPortable
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $rawMountGuardDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $rawMountGuardCase.EvidenceRoot `
        -RunPaths $rawMountGuardCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $rawMountGuardDrift $repository.OutputPath 'raw_mount_tampering'

    $rawMountNonceCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'raw-mount-wrong-nonce')
    $rawMountNonceRun = Read-SelfTestJson -Path $rawMountNonceCase.RunPaths[0]
    $rawMountNoncePortable = [string](@($rawMountNonceRun.files |
        Where-Object { $_.path -cmatch '/console\.log$' })[0].path)
    $rawMountNonceFull = Join-Path `
        (Split-Path -Parent $rawMountNonceCase.RunPaths[0]) `
        $rawMountNoncePortable.Replace('/', '\')
    $rawMountNonceLeaf = Split-Path -Leaf `
        (Split-Path -Parent $rawMountNonceCase.RunPaths[0])
    $rawMountExpectedNonce = $rawMountNonceLeaf.Substring(
        $rawMountNonceLeaf.Length - 32)
    $rawMountWrongNonce = '00000000000000000000000000000000'
    if ($rawMountWrongNonce -ceq $rawMountExpectedNonce) {
        $rawMountWrongNonce = 'ffffffffffffffffffffffffffffffff'
    }
    $rawMountNonceText = [IO.File]::ReadAllText($rawMountNonceFull).Replace(
        'PartisanFocusedAutotest/' + $rawMountExpectedNonce +
            '/candidate-addons',
        'PartisanFocusedAutotest/' + $rawMountWrongNonce +
            '/candidate-addons')
    if ($rawMountNonceText.IndexOf(
            'PartisanFocusedAutotest/' + $rawMountExpectedNonce +
                '/candidate-addons',
            [StringComparison]::Ordinal) -ge 0) {
        throw 'Focused wrong-nonce fixture retained the expected guard path.'
    }
    Write-SelfTestText -Path $rawMountNonceFull -Text $rawMountNonceText
    Update-SelfTestRunIndex `
        -RunPath $rawMountNonceCase.RunPaths[0] `
        -PortablePath $rawMountNoncePortable
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $rawMountNonceDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $rawMountNonceCase.EvidenceRoot `
        -RunPaths $rawMountNonceCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $rawMountNonceDrift $repository.OutputPath 'raw_mount_tampering'

    $rawDiagnosticCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'raw-diagnostic-tamper')
    $rawDiagnosticRun = Read-SelfTestJson -Path $rawDiagnosticCase.RunPaths[0]
    $rawDiagnosticPortable = [string](@($rawDiagnosticRun.files | Where-Object {
        $_.path -cmatch '/console\.log$'
    })[0].path)
    $rawDiagnosticFull = Join-Path `
        (Split-Path -Parent $rawDiagnosticCase.RunPaths[0]) `
        $rawDiagnosticPortable.Replace('/', '\')
    [IO.File]::AppendAllText(
        $rawDiagnosticFull,
        "00:02:08.000 SCRIPT (E): unapproved focused diagnostic`n",
        (New-Object Text.UTF8Encoding($false)))
    Update-SelfTestRunIndex `
        -RunPath $rawDiagnosticCase.RunPaths[0] `
        -PortablePath $rawDiagnosticPortable
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $rawDiagnosticTamper = Invoke-SelfTestProducer `
        -EvidenceRoot $rawDiagnosticCase.EvidenceRoot `
        -RunPaths $rawDiagnosticCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $rawDiagnosticTamper $repository.OutputPath 'raw_diagnostic_tampering'

    $duplicateMarkerCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'duplicate-console-marker')
    $duplicateMarkerRun = Read-SelfTestJson -Path $duplicateMarkerCase.RunPaths[0]
    $duplicateMarkerPortable = [string](@($duplicateMarkerRun.files |
        Where-Object { $_.path -cmatch '/console\.log$' })[0].path)
    $duplicateMarkerFull = Join-Path `
        (Split-Path -Parent $duplicateMarkerCase.RunPaths[0]) `
        $duplicateMarkerPortable.Replace('/', '\')
    [IO.File]::AppendAllText(
        $duplicateMarkerFull,
        "00:02:08.000 SCRIPT : SCR_TestRunner has finished running`n",
        (New-Object Text.UTF8Encoding($false)))
    Update-SelfTestRunIndex `
        -RunPath $duplicateMarkerCase.RunPaths[0] `
        -PortablePath $duplicateMarkerPortable
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $duplicateMarker = Invoke-SelfTestProducer `
        -EvidenceRoot $duplicateMarkerCase.EvidenceRoot `
        -RunPaths $duplicateMarkerCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $duplicateMarker $repository.OutputPath 'raw_diagnostic_tampering'

    $mismatchedMarkerCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'mismatched-console-suite')
    $mismatchedMarkerRun = Read-SelfTestJson -Path $mismatchedMarkerCase.RunPaths[0]
    $mismatchedMarkerPortable = [string](@($mismatchedMarkerRun.files |
        Where-Object { $_.path -cmatch '/console\.log$' })[0].path)
    $mismatchedMarkerFull = Join-Path `
        (Split-Path -Parent $mismatchedMarkerCase.RunPaths[0]) `
        $mismatchedMarkerPortable.Replace('/', '\')
    $mismatchedMarkerText = [IO.File]::ReadAllText($mismatchedMarkerFull).
        Replace(
            'TestSuite #HST_EnemyCounterattackAutotestSuite started',
            'TestSuite #HST_MismatchedAutotestSuite started')
    Write-SelfTestText `
        -Path $mismatchedMarkerFull `
        -Text $mismatchedMarkerText
    Update-SelfTestRunIndex `
        -RunPath $mismatchedMarkerCase.RunPaths[0] `
        -PortablePath $mismatchedMarkerPortable
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $mismatchedMarker = Invoke-SelfTestProducer `
        -EvidenceRoot $mismatchedMarkerCase.EvidenceRoot `
        -RunPaths $mismatchedMarkerCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $mismatchedMarker $repository.OutputPath 'raw_diagnostic_tampering'

    $foreignMarkerCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'additive-foreign-console-markers')
    $foreignMarkerRun = Read-SelfTestJson -Path $foreignMarkerCase.RunPaths[0]
    $foreignMarkerPortable = [string](@($foreignMarkerRun.files |
        Where-Object { $_.path -cmatch '/console\.log$' })[0].path)
    $foreignMarkerFull = Join-Path `
        (Split-Path -Parent $foreignMarkerCase.RunPaths[0]) `
        $foreignMarkerPortable.Replace('/', '\')
    $foreignMarkerLines = @(
        '00:02:08.000 SCRIPT : TestSuite #HST_ForeignAutotestSuite started',
        '00:02:09.000 SCRIPT : HST_TEST_ForeignAuthority: SUCCESS')
    [IO.File]::AppendAllText(
        $foreignMarkerFull,
        (($foreignMarkerLines -join "`n") + "`n"),
        (New-Object Text.UTF8Encoding($false)))
    $foreignMarkerRun.outcome.diagnosticTail = @(
        @($foreignMarkerRun.outcome.diagnosticTail) + $foreignMarkerLines)
    Write-SelfTestJson `
        -Path $foreignMarkerCase.RunPaths[0] `
        -Value $foreignMarkerRun
    Update-SelfTestRunIndex `
        -RunPath $foreignMarkerCase.RunPaths[0] `
        -PortablePath $foreignMarkerPortable
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $foreignMarker = Invoke-SelfTestProducer `
        -EvidenceRoot $foreignMarkerCase.EvidenceRoot `
        -RunPaths $foreignMarkerCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $foreignMarker $repository.OutputPath 'raw_diagnostic_tampering'

    $semanticSwapCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'semantic-swap-restore')
    $semanticSwapRun = Read-SelfTestJson -Path $semanticSwapCase.RunPaths[0]
    $semanticSwapPortable = [string](@($semanticSwapRun.files |
        Where-Object { $_.path -cmatch '/console\.log$' })[0].path)
    $semanticSwapFull = Join-Path `
        (Split-Path -Parent $semanticSwapCase.RunPaths[0]) `
        $semanticSwapPortable.Replace('/', '\')
    [IO.File]::AppendAllText(
        $semanticSwapFull,
        "00:02:08.000 SCRIPT : SCR_TestRunner has finished running`n",
        (New-Object Text.UTF8Encoding($false)))
    Update-SelfTestRunIndex `
        -RunPath $semanticSwapCase.RunPaths[0] `
        -PortablePath $semanticSwapPortable
    $semanticInvalidBytes = [IO.File]::ReadAllBytes($semanticSwapFull)
    $pristineFirstRun = Read-SelfTestJson -Path $pristine.RunPaths[0]
    $pristineConsolePortable = [string](@($pristineFirstRun.files |
        Where-Object { $_.path -cmatch '/console\.log$' })[0].path)
    $pristineConsoleFull = Join-Path `
        (Split-Path -Parent $pristine.RunPaths[0]) `
        $pristineConsolePortable.Replace('/', '\')
    $semanticValidBytes = [IO.File]::ReadAllBytes($pristineConsoleFull)
    $semanticToken = [Guid]::NewGuid().ToString('N')
    $semanticBeforeReady = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanFocusedAggregateSemanticBeforeReady-' + $semanticToken))
    $semanticBeforeContinue = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanFocusedAggregateSemanticBeforeContinue-' + $semanticToken))
    $semanticAfterReady = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanFocusedAggregateSemanticAfterReady-' + $semanticToken))
    $semanticAfterContinue = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanFocusedAggregateSemanticAfterContinue-' + $semanticToken))
    $semanticJob = $null
    try {
        Reset-SelfTestOutput -OutputPath $repository.OutputPath
        $semanticFixture = [pscustomobject][ordered]@{
            ProducerPath = $producer
            EvidenceRoot = $semanticSwapCase.EvidenceRoot
            RunPaths = @($semanticSwapCase.RunPaths)
            OutputPath = $repository.OutputPath
            RepositoryRoot = $repository.Root
            Token = $semanticToken
        }
        $semanticJob = Start-Job -ScriptBlock {
            param($Fixture)
            $env:PARTISAN_FOCUSED_AGGREGATE_SELFTEST_SEMANTIC_TOKEN =
                $Fixture.Token
            try {
                & $Fixture.ProducerPath `
                    -EvidenceRoot $Fixture.EvidenceRoot `
                    -RunJson @($Fixture.RunPaths) `
                    -OutputPath $Fixture.OutputPath `
                    -RepositoryRoot $Fixture.RepositoryRoot
                [pscustomobject][ordered]@{ Succeeded = $true; Error = '' }
            }
            catch {
                [pscustomobject][ordered]@{
                    Succeeded = $false
                    Error = $_.Exception.Message
                }
            }
            finally {
                Remove-Item `
                    Env:PARTISAN_FOCUSED_AGGREGATE_SELFTEST_SEMANTIC_TOKEN `
                    -ErrorAction SilentlyContinue
            }
        } -ArgumentList $semanticFixture
        Assert-SelfTest `
            -Condition $semanticBeforeReady.WaitOne(15000) `
            -Message 'Semantic-swap producer did not reach its pre-read barrier.'
        [IO.File]::WriteAllBytes($semanticSwapFull, $semanticValidBytes)
        [void]$semanticBeforeContinue.Set()
        Assert-SelfTest `
            -Condition $semanticAfterReady.WaitOne(15000) `
            -Message 'Semantic-swap producer did not capture its alternate bytes.'
        [IO.File]::WriteAllBytes($semanticSwapFull, $semanticInvalidBytes)
        [void]$semanticAfterContinue.Set()
        $semanticCompleted = $semanticJob | Wait-Job -Timeout 30
        Assert-SelfTest `
            -Condition ($null -ne $semanticCompleted -and
                $semanticJob.State -ceq 'Completed') `
            -Message 'Semantic-swap producer did not finish.'
        $semanticSwap = @($semanticJob | Receive-Job)[-1]
        Assert-SelfTestRejected `
            $semanticSwap $repository.OutputPath 'raw_result_tampering'
    }
    finally {
        [void]$semanticBeforeContinue.Set()
        [void]$semanticAfterContinue.Set()
        if ($semanticJob) {
            $semanticJob | Remove-Job -Force -ErrorAction SilentlyContinue
        }
        $semanticBeforeReady.Dispose()
        $semanticBeforeContinue.Dispose()
        $semanticAfterReady.Dispose()
        $semanticAfterContinue.Dispose()
    }

    $redReceiptPath = $repository.OutputPath + '.replacement-required.json'
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $redBeforeGreen = Invoke-SelfTestProducer `
        -EvidenceRoot $duplicateMarkerCase.EvidenceRoot `
        -RunPaths $duplicateMarkerCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $redBeforeGreen $repository.OutputPath 'raw_diagnostic_tampering'
    $redBeforeGreenReceipt = Read-SelfTestJson -Path $redReceiptPath
    $redBeforeGreenHash = Get-SelfTestSha256 -Path $redReceiptPath
    $trackedManifest = Read-SelfTestJson -Path $repository.ManifestPath
    Assert-SelfTest `
        -Condition (-not [bool]$redBeforeGreenReceipt.precedence.
                acceptedAggregatePresentBeforeRejection -and
            $null -eq $redBeforeGreenReceipt.precedence.acceptedAggregateSha256 -and
            [string]$redBeforeGreenReceipt.candidate.packageSha256 -ceq
                [string]$trackedManifest.package.sha256 -and
            [string]$redBeforeGreenReceipt.candidate.manifestSha256 -ceq
                (Get-SelfTestSha256 -Path $repository.ManifestPath) -and
            [string]$redBeforeGreenReceipt.candidate.readySha256 -ceq
                (Get-SelfTestSha256 -Path $repository.ReadyPath)) `
        -Message 'RED-before-green receipt did not bind the authenticated candidate seals.'
    $receiptAsHistorical = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root `
        -Historical @($redReceiptPath)
    Assert-SelfTest `
        -Condition (-not $receiptAsHistorical.Succeeded) `
        -Message 'Receipt-as-history fixture unexpectedly passed.'
    Assert-SelfTest `
        -Condition ($receiptAsHistorical.Error -cmatch
            '\(history_census_drift\)') `
        -Message ('Receipt-as-history fixture used the wrong rejection: ' +
            $receiptAsHistorical.Error)
    Assert-SelfTest `
        -Condition (-not (Test-Path -LiteralPath $repository.OutputPath)) `
        -Message 'Receipt-as-history rejection left a green aggregate on disk.'
    Assert-SelfTest `
        -Condition ((Get-SelfTestSha256 -Path $redReceiptPath) -ceq
            $redBeforeGreenHash) `
        -Message 'Receipt-as-history rejection changed the first-published RED.'
    $blockedGreen = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTest `
        -Condition (-not $blockedGreen.Succeeded) `
        -Message 'A first-published RED unexpectedly allowed later green publication.'
    Assert-SelfTest `
        -Condition ($blockedGreen.Error -cmatch '\(replacement_required\)') `
        -Message ('A first-published RED used the wrong later-green rejection: ' +
            $blockedGreen.Error)
    Assert-SelfTest `
        -Condition (-not (Test-Path -LiteralPath $repository.OutputPath)) `
        -Message 'A first-published RED left a later green aggregate on disk.'
    Assert-SelfTest `
        -Condition ((Get-SelfTestSha256 -Path $redReceiptPath) -ceq
            $redBeforeGreenHash) `
        -Message 'A first-published RED receipt changed while blocking later green.'

    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $greenBeforeRed = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTest `
        -Condition $greenBeforeRed.Succeeded `
        -Message ('Green-before-red baseline failed: ' + $greenBeforeRed.Error)
    $greenBeforeRedHash = Get-SelfTestSha256 -Path $repository.OutputPath
    $redAfterGreen = Invoke-SelfTestProducer `
        -EvidenceRoot $duplicateMarkerCase.EvidenceRoot `
        -RunPaths $duplicateMarkerCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $redAfterGreen $repository.OutputPath 'raw_diagnostic_tampering'
    $redAfterGreenReceipt = Read-SelfTestJson -Path $redReceiptPath
    $redAfterGreenReceiptHash = Get-SelfTestSha256 -Path $redReceiptPath
    Assert-SelfTest `
        -Condition ([bool]$redAfterGreenReceipt.precedence.
                acceptedAggregatePresentBeforeRejection -and
            [string]$redAfterGreenReceipt.precedence.acceptedAggregateSha256 -ceq
                $greenBeforeRedHash) `
        -Message 'Green-before-red receipt did not bind the preceding accepted bytes.'
    $greenAfterRed = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTest `
        -Condition ($greenAfterRed.Succeeded -and
            (Get-SelfTestSha256 -Path $repository.OutputPath) -ceq
                $greenBeforeRedHash -and
            (Get-SelfTestSha256 -Path $redReceiptPath) -ceq
                $redAfterGreenReceiptHash) `
        -Message 'A later RED attempt displaced a first-published green aggregate.'

    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    Write-SelfTestJson `
        -Path $redReceiptPath `
        -Value $redBeforeGreenReceipt
    $forgedReceipt = Read-SelfTestJson -Path $redReceiptPath
    $forgedReceipt.candidate.candidateId = 'wrong-candidate'
    Write-SelfTestJson -Path $redReceiptPath -Value $forgedReceipt
    $forgedReceiptHash = Get-SelfTestSha256 -Path $redReceiptPath
    $forgedRedAttempt = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTest `
        -Condition (-not $forgedRedAttempt.Succeeded -and
            $forgedRedAttempt.Error -cmatch '\(historical_blob_replacement\)' -and
            -not (Test-Path -LiteralPath $repository.OutputPath) -and
            (Get-SelfTestSha256 -Path $redReceiptPath) -ceq
                $forgedReceiptHash) `
        -Message 'A forged wrong-candidate RED receipt was accepted or overwritten.'

    $wrongSealReceipts = [ordered]@{}
    foreach ($receiptSealName in @(
            'packageSha256',
            'manifestSha256',
            'readySha256')) {
        Reset-SelfTestOutput -OutputPath $repository.OutputPath
        $wrongSealReceipt = $redBeforeGreenReceipt |
            ConvertTo-Json -Depth 100 | ConvertFrom-Json
        $wrongSealValue = 'f' * 64
        if ([string]$wrongSealReceipt.candidate.$receiptSealName -ceq
            $wrongSealValue) {
            $wrongSealValue = 'e' * 64
        }
        $wrongSealReceipt.candidate.$receiptSealName = $wrongSealValue
        $wrongSealReceipts[$receiptSealName] = $wrongSealReceipt
        Write-SelfTestJson `
            -Path $redReceiptPath `
            -Value $wrongSealReceipt
        $wrongSealReceiptHash = Get-SelfTestSha256 -Path $redReceiptPath
        $wrongSealAttempt = Invoke-SelfTestProducer `
            -EvidenceRoot $pristine.EvidenceRoot `
            -RunPaths $pristine.RunPaths `
            -OutputPath $repository.OutputPath `
            -RepositoryRoot $repository.Root
        Assert-SelfTest `
            -Condition (-not $wrongSealAttempt.Succeeded -and
                $wrongSealAttempt.Error -cmatch
                    '\(historical_blob_replacement\)' -and
                -not (Test-Path -LiteralPath $repository.OutputPath) -and
                (Get-SelfTestSha256 -Path $redReceiptPath) -ceq
                    $wrongSealReceiptHash) `
            -Message ("A same-ID wrong $receiptSealName RED receipt was " +
                'trusted, overwritten, or allowed green.')
    }

    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $redConcurrentFixture = [pscustomobject][ordered]@{
        ProducerPath = $producer
        EvidenceRoot = $duplicateMarkerCase.EvidenceRoot
        RunPaths = @($duplicateMarkerCase.RunPaths)
        OutputPath = $repository.OutputPath
        RepositoryRoot = $repository.Root
    }
    $redConcurrentJobs = @(1..2 | ForEach-Object {
        Start-Job -ScriptBlock {
            param($Fixture)
            try {
                & $Fixture.ProducerPath `
                    -EvidenceRoot $Fixture.EvidenceRoot `
                    -RunJson @($Fixture.RunPaths) `
                    -OutputPath $Fixture.OutputPath `
                    -RepositoryRoot $Fixture.RepositoryRoot
                [pscustomobject][ordered]@{ Succeeded = $true; Error = '' }
            }
            catch {
                [pscustomobject][ordered]@{
                    Succeeded = $false
                    Error = $_.Exception.Message
                }
            }
        } -ArgumentList $redConcurrentFixture
    })
    try {
        $redConcurrentJobs | Wait-Job | Out-Null
        $redConcurrentResults = @($redConcurrentJobs | ForEach-Object {
            @($_ | Receive-Job)[-1]
        })
        Assert-SelfTest `
            -Condition ($redConcurrentResults.Count -eq 2 -and
                @($redConcurrentResults | Where-Object {
                    -not $_.Succeeded -and
                    $_.Error -cmatch '\(raw_diagnostic_tampering\)'
                }).Count -eq 2 -and
                -not (Test-Path -LiteralPath $repository.OutputPath)) `
            -Message 'Concurrent RED publication did not reject both invalid attempts.'
        Assert-SelfTestRejected `
            $redConcurrentResults[0] `
            $repository.OutputPath `
            'raw_diagnostic_tampering'
    }
    finally {
        $redConcurrentJobs | Remove-Job -Force -ErrorAction SilentlyContinue
    }

    $indexCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'index-tamper')
    $indexRun = Read-SelfTestJson -Path $indexCase.RunPaths[0]
    $indexRun.files = @($indexRun.files | Select-Object -First 7)
    Write-SelfTestJson -Path $indexCase.RunPaths[0] -Value $indexRun
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $indexTamper = Invoke-SelfTestProducer `
        -EvidenceRoot $indexCase.EvidenceRoot `
        -RunPaths $indexCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected $indexTamper $repository.OutputPath 'index_tampering'

    $extraCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'extra-blob')
    Write-SelfTestText `
        -Path (Join-Path (Split-Path -Parent $extraCase.RunPaths[0]) 'raw/extra.log') `
        -Text "extra`n"
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $extraBlob = Invoke-SelfTestProducer `
        -EvidenceRoot $extraCase.EvidenceRoot `
        -RunPaths $extraCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected $extraBlob $repository.OutputPath 'index_tampering'

    $candidateBlobCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'candidate-blob-tamper')
    $candidateBlobRun = Read-SelfTestJson -Path $candidateBlobCase.RunPaths[0]
    $candidatePortable = 'identity/candidate.json'
    $candidateFull = Join-Path `
        (Split-Path -Parent $candidateBlobCase.RunPaths[0]) `
        $candidatePortable.Replace('/', '\')
    $candidateManifest = Read-SelfTestJson -Path $candidateFull
    $candidateManifest.candidate.version = '0.1.0-rc.tampered'
    Write-SelfTestJson -Path $candidateFull -Value $candidateManifest
    Update-SelfTestRunIndex `
        -RunPath $candidateBlobCase.RunPaths[0] `
        -PortablePath $candidatePortable
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $candidateBlobTamper = Invoke-SelfTestProducer `
        -EvidenceRoot $candidateBlobCase.EvidenceRoot `
        -RunPaths $candidateBlobCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $candidateBlobTamper $repository.OutputPath 'candidate_tampering' -NoReceipt

    $manifestSchemaCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'manifest-schema-drift')
    $manifestSchemaPortable = 'identity/candidate.json'
    $manifestSchemaFull = Join-Path `
        (Split-Path -Parent $manifestSchemaCase.RunPaths[0]) `
        $manifestSchemaPortable.Replace('/', '\')
    $manifestSchemaValue = Read-SelfTestJson -Path $manifestSchemaFull
    $manifestSchemaValue | Add-Member `
        -MemberType NoteProperty `
        -Name unexpectedManifestField `
        -Value $true
    Write-SelfTestJson -Path $manifestSchemaFull -Value $manifestSchemaValue
    Update-SelfTestRunIndex `
        -RunPath $manifestSchemaCase.RunPaths[0] `
        -PortablePath $manifestSchemaPortable
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $manifestSchemaDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $manifestSchemaCase.EvidenceRoot `
        -RunPaths $manifestSchemaCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $manifestSchemaDrift $repository.OutputPath 'candidate_tampering' -NoReceipt

    foreach ($frozenSchemaCase in @(
            [pscustomobject][ordered]@{
                Name = 'campaign-schema-72'
                CampaignSchema = 72
                RuntimeSettingsSchema = 24
            },
            [pscustomobject][ordered]@{
                Name = 'runtime-settings-schema-25'
                CampaignSchema = 71
                RuntimeSettingsSchema = 25
            })) {
        $schemaRepository = New-SelfTestRepository `
            -Root (Join-Path $tempRoot ([string]$frozenSchemaCase.Name + '-repository')) `
            -CampaignSchema ([int]$frozenSchemaCase.CampaignSchema) `
            -RuntimeSettingsSchema ([int]$frozenSchemaCase.RuntimeSettingsSchema)
        $frozenSchemaFixture = New-SelfTestFixture `
            -Root (Join-Path $tempRoot ([string]$frozenSchemaCase.Name + '-fixture')) `
            -HarnessGitHead $schemaRepository.RunHarnessGitHead `
            -RunnerSha256 $schemaRepository.RunnerSha256 `
            -CandidateModuleSha256 $schemaRepository.CandidateModuleSha256 `
            -TrackedManifestPath $schemaRepository.ManifestPath `
            -TrackedReadyPath $schemaRepository.ReadyPath
        $frozenSchemaDrift = Invoke-SelfTestProducer `
            -EvidenceRoot $frozenSchemaFixture.EvidenceRoot `
            -RunPaths $frozenSchemaFixture.RunPaths `
            -OutputPath $schemaRepository.OutputPath `
            -RepositoryRoot $schemaRepository.Root
        Assert-SelfTest `
            -Condition (-not $frozenSchemaDrift.Succeeded -and
                $frozenSchemaDrift.Error -cmatch '\(candidate_tampering\)') `
            -Message ('Frozen schema drift did not fail as candidate_tampering: ' +
                [string]$frozenSchemaCase.Name)
        Assert-SelfTestRejected `
            $frozenSchemaDrift `
            $schemaRepository.OutputPath `
            'candidate_tampering' `
            -NoReceipt
    }

    $schemaCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'schema-drift')
    $schemaRun = Read-SelfTestJson -Path $schemaCase.RunPaths[0]
    $schemaRun.schemaVersion = 2
    Write-SelfTestJson -Path $schemaCase.RunPaths[0] -Value $schemaRun
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $schemaDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $schemaCase.EvidenceRoot `
        -RunPaths $schemaCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTest `
        -Condition (-not $schemaDrift.Succeeded) `
        -Message 'Wrong-schema run envelope unexpectedly passed schema validation.'
    Assert-SelfTestRejected `
        $schemaDrift $repository.OutputPath 'schema_drift' -NoReceipt

    $resultSchemaCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'result-schema-drift')
    $resultSchemaRun = Read-SelfTestJson -Path $resultSchemaCase.RunPaths[0]
    $resultSchemaRun.outcome.result | Add-Member `
        -MemberType NoteProperty `
        -Name unexpectedResultField `
        -Value $true
    Write-SelfTestJson `
        -Path $resultSchemaCase.RunPaths[0] `
        -Value $resultSchemaRun
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $resultSchemaDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $resultSchemaCase.EvidenceRoot `
        -RunPaths $resultSchemaCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $resultSchemaDrift $repository.OutputPath 'status_drift'

    $policyCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'policy-drift')
    $policyRun = Read-SelfTestJson -Path $policyCase.RunPaths[0]
    $policyRun.evidenceKind = 'packaged-focused-autotest-v2'
    Write-SelfTestJson -Path $policyCase.RunPaths[0] -Value $policyRun
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $policyDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $policyCase.EvidenceRoot `
        -RunPaths $policyCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $policyDrift $repository.OutputPath 'policy_drift' -NoReceipt

    $statusCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'status-drift')
    $statusRun = Read-SelfTestJson -Path $statusCase.RunPaths[0]
    $statusRun.outcome.result.Failures = 1
    Write-SelfTestJson -Path $statusCase.RunPaths[0] -Value $statusRun
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $statusDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $statusCase.EvidenceRoot `
        -RunPaths $statusCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected $statusDrift $repository.OutputPath 'status_drift'

    $dispositionCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'disposition-drift')
    $dispositionRun = Read-SelfTestJson -Path $dispositionCase.RunPaths[0]
    $dispositionRun.candidate.runtimeUseDisposition = 'rejected-after-runtime'
    Write-SelfTestJson -Path $dispositionCase.RunPaths[0] -Value $dispositionRun
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $dispositionDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $dispositionCase.EvidenceRoot `
        -RunPaths $dispositionCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $dispositionDrift $repository.OutputPath 'disposition_drift' -NoReceipt

    [IO.File]::AppendAllText(
        $repository.StatusPath,
        "`n",
        (New-Object Text.UTF8Encoding($false)))
    try {
        Reset-SelfTestOutput -OutputPath $repository.OutputPath
        $trackedStatusDrift = Invoke-SelfTestProducer `
            -EvidenceRoot $pristine.EvidenceRoot `
            -RunPaths $pristine.RunPaths `
            -OutputPath $repository.OutputPath `
            -RepositoryRoot $repository.Root
        Assert-SelfTestRejected `
            $trackedStatusDrift $repository.OutputPath 'candidate_status_drift' -NoReceipt
    }
    finally {
        & git -C $repository.Root checkout --quiet HEAD -- `
            docs/data/release_status.json
        if ($LASTEXITCODE -ne 0) {
            throw 'Focused aggregate self-test could not restore tracked release status.'
        }
    }

    [IO.File]::AppendAllText(
        $repository.ManifestPath,
        "`n",
        (New-Object Text.UTF8Encoding($false)))
    try {
        Reset-SelfTestOutput -OutputPath $repository.OutputPath
        $trackedManifestDrift = Invoke-SelfTestProducer `
            -EvidenceRoot $pristine.EvidenceRoot `
            -RunPaths $pristine.RunPaths `
            -OutputPath $repository.OutputPath `
            -RepositoryRoot $repository.Root
        Assert-SelfTestRejected `
            $trackedManifestDrift $repository.OutputPath 'candidate_status_drift' -NoReceipt
    }
    finally {
        & git -C $repository.Root checkout --quiet HEAD -- `
            ('docs/evidence/release-candidates/' + $repository.CandidateId +
                '/candidate.json')
        if ($LASTEXITCODE -ne 0) {
            throw 'Focused aggregate self-test could not restore the tracked candidate manifest.'
        }
    }

    $junitCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'junit-tamper')
    $junitRun = Read-SelfTestJson -Path $junitCase.RunPaths[0]
    $junitPortable = [string](@($junitRun.files | Where-Object {
        $_.path -cmatch '/junit\.xml$'
    })[0].path)
    $junitFull = Join-Path `
        (Split-Path -Parent $junitCase.RunPaths[0]) `
        $junitPortable.Replace('/', '\')
    $junitText = [IO.File]::ReadAllText($junitFull).Replace(
        'HST_EnemyCounterattackAutotestSuite',
        'HST_TamperedSuite')
    Write-SelfTestText -Path $junitFull -Text $junitText
    Update-SelfTestRunIndex `
        -RunPath $junitCase.RunPaths[0] `
        -PortablePath $junitPortable
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $junitTamper = Invoke-SelfTestProducer `
        -EvidenceRoot $junitCase.EvidenceRoot `
        -RunPaths $junitCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $junitTamper $repository.OutputPath 'raw_junit_tampering'

    $producerWorktree = Join-Path `
        $repository.Root `
        'tools/New-PartisanFocusedAutotestAggregate.ps1'
    [IO.File]::AppendAllText($producerWorktree, "`n# drift`n")
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $producerDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $producerDrift $repository.OutputPath 'aggregation_tool_drift'
    Copy-SelfTestWritableFile `
        -Source $producer `
        -Destination $producerWorktree

    $consumerWorktree = Join-Path $repository.Root 'tools/update-release-docs.ps1'
    [IO.File]::AppendAllText($consumerWorktree, "`n# drift`n")
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $consumerDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $consumerDrift $repository.OutputPath 'aggregation_tool_drift'
    & git -C $repository.Root checkout --quiet HEAD -- tools/update-release-docs.ps1
    if ($LASTEXITCODE -ne 0) {
        throw 'Focused aggregate self-test could not restore its temporary consumer.'
    }

    $lateDriftCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'late-snapshot-drift')
    $lateDriftRun = Read-SelfTestJson -Path $lateDriftCase.RunPaths[0]
    $lateDriftPortable = [string](@($lateDriftRun.files | Where-Object {
        $_.path -cmatch '/script\.log$'
    })[0].path)
    $lateDriftFull = Join-Path `
        (Split-Path -Parent $lateDriftCase.RunPaths[0]) `
        $lateDriftPortable.Replace('/', '\')
    $lateDriftToken = [Guid]::NewGuid().ToString('N')
    $lateReady = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanFocusedAggregateReady-' + $lateDriftToken))
    $lateMutated = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanFocusedAggregateMutated-' + $lateDriftToken))
    $lateJob = $null
    $env:PARTISAN_FOCUSED_AGGREGATE_SELFTEST_LATE_DRIFT_TOKEN =
        $lateDriftToken
    try {
        Reset-SelfTestOutput -OutputPath $repository.OutputPath
        $lateFixture = [pscustomobject][ordered]@{
            ProducerPath = $producer
            EvidenceRoot = $lateDriftCase.EvidenceRoot
            RunPaths = @($lateDriftCase.RunPaths)
            OutputPath = $repository.OutputPath
            RepositoryRoot = $repository.Root
        }
        $lateJob = Start-Job -ScriptBlock {
            param($Fixture)
            $succeeded = $false
            $errorText = ''
            try {
                $producerOutput = @(& $Fixture.ProducerPath `
                    -EvidenceRoot $Fixture.EvidenceRoot `
                    -RunJson @($Fixture.RunPaths) `
                    -OutputPath $Fixture.OutputPath `
                    -RepositoryRoot $Fixture.RepositoryRoot)
                $succeeded = $true
            }
            catch {
                $errorText = $_.Exception.Message
            }
            [pscustomobject][ordered]@{
                Succeeded = $succeeded
                Error = $errorText
            }
        } -ArgumentList $lateFixture
        Assert-SelfTest `
            -Condition $lateReady.WaitOne(15000) `
            -Message 'Late-drift producer did not reach its bounded publication barrier.'
        [IO.File]::AppendAllText(
            $lateDriftFull,
            "late publication mutation`n",
            (New-Object Text.UTF8Encoding($false)))
        Update-SelfTestRunIndex `
            -RunPath $lateDriftCase.RunPaths[0] `
            -PortablePath $lateDriftPortable
        [void]$lateMutated.Set()
        $lateCompleted = $lateJob | Wait-Job -Timeout 30
        Assert-SelfTest `
            -Condition ($null -ne $lateCompleted -and
                $lateJob.State -ceq 'Completed') `
            -Message 'Late-drift producer did not finish after its bounded barrier.'
        $lateSnapshotDrift = @($lateJob | Receive-Job)[-1]
        Assert-SelfTestRejected `
            $lateSnapshotDrift `
            $repository.OutputPath `
            'evidence_snapshot_drift'
    }
    finally {
        [void]$lateMutated.Set()
        if ($lateJob) {
            $lateJob | Remove-Job -Force -ErrorAction SilentlyContinue
        }
        Remove-Item `
            Env:PARTISAN_FOCUSED_AGGREGATE_SELFTEST_LATE_DRIFT_TOKEN `
            -ErrorAction SilentlyContinue
        $lateReady.Dispose()
        $lateMutated.Dispose()
    }

    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $noncanonicalOutput = Join-Path $repository.Root (
        'docs/evidence/focused-autotest/noncanonical/' +
        $repository.CandidateId + '.json')
    $outputPathDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $noncanonicalOutput `
        -RepositoryRoot $repository.Root
    Assert-SelfTest `
        -Condition (-not $outputPathDrift.Succeeded -and
            $outputPathDrift.Error -cmatch '\(output_path_invalid\)' -and
            -not (Test-Path -LiteralPath $noncanonicalOutput) -and
            -not (Test-Path -LiteralPath (
                $noncanonicalOutput + '.replacement-required.json'))) `
        -Message 'A noncanonical aggregate output path was not rejected without publication.'

    $outputParent = Split-Path -Parent $repository.OutputPath
    $parentBarrierToken = [Guid]::NewGuid().ToString('N')
    $parentBarrierReady = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanFocusedAggregateParentReady-' + $parentBarrierToken))
    $parentBarrierContinue = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanFocusedAggregateParentContinue-' + $parentBarrierToken))
    $parentBarrierTarget = Join-Path $tempRoot 'parent-barrier-target'
    $parentBarrierBackup = Join-Path $tempRoot 'parent-barrier-backup'
    New-Item -ItemType Directory -Path $parentBarrierTarget -Force | Out-Null
    $parentBarrierJob = $null
    $parentBarrierSwapped = $false
    $parentBarrierMoved = $false
    $parentBarrierPartialMoveObserved = $false
    try {
        Reset-SelfTestOutput -OutputPath $repository.OutputPath
        $parentBarrierFixture = [pscustomobject][ordered]@{
            ProducerPath = $producer
            EvidenceRoot = $pristine.EvidenceRoot
            RunPaths = @($pristine.RunPaths)
            OutputPath = $repository.OutputPath
            RepositoryRoot = $repository.Root
            Token = $parentBarrierToken
        }
        $parentBarrierJob = Start-Job -ScriptBlock {
            param($Fixture)
            $env:PARTISAN_FOCUSED_AGGREGATE_SELFTEST_PARENT_TOKEN = $Fixture.Token
            try {
                & $Fixture.ProducerPath `
                    -EvidenceRoot $Fixture.EvidenceRoot `
                    -RunJson @($Fixture.RunPaths) `
                    -OutputPath $Fixture.OutputPath `
                    -RepositoryRoot $Fixture.RepositoryRoot
                [pscustomobject][ordered]@{ Succeeded = $true; Error = '' }
            }
            catch {
                [pscustomobject][ordered]@{
                    Succeeded = $false
                    Error = $_.Exception.Message
                }
            }
            finally {
                Remove-Item `
                    Env:PARTISAN_FOCUSED_AGGREGATE_SELFTEST_PARENT_TOKEN `
                    -ErrorAction SilentlyContinue
            }
        } -ArgumentList $parentBarrierFixture
        Assert-SelfTest `
            -Condition $parentBarrierReady.WaitOne(15000) `
            -Message 'Output-parent producer did not reach its final move barrier.'
        $heldMutationDenied = $false
        try {
            [IO.File]::AppendAllText(
                $pristineConsoleFull,
                "last-window mutation`n",
                (New-Object Text.UTF8Encoding($false)))
        }
        catch [IO.IOException] {
            $heldMutationDenied = $true
        }
        Assert-SelfTest `
            -Condition $heldMutationDenied `
            -Message 'Retained source handles allowed a last-window evidence write.'
        try {
            Move-Item `
                -LiteralPath $outputParent `
                -Destination $parentBarrierBackup `
                -ErrorAction Stop
            $parentBarrierMoved = $true
            New-Item `
                -ItemType Junction `
                -Path $outputParent `
                -Value $parentBarrierTarget `
                -ErrorAction Stop | Out-Null
            $parentBarrierSwapped = $true
        }
        catch {
            $parentBarrierSwapped = $false
            if ($parentBarrierMoved -and
                (Test-Path `
                    -LiteralPath $parentBarrierBackup `
                    -PathType Container)) {
                if (Test-Path -LiteralPath $outputParent) {
                    Remove-Item -LiteralPath $outputParent -Force
                }
                Move-Item `
                    -LiteralPath $parentBarrierBackup `
                    -Destination $outputParent
                $parentBarrierMoved = $false
            }
            elseif (Test-Path `
                    -LiteralPath $parentBarrierBackup `
                    -PathType Container) {
                $parentBarrierPartialMoveObserved = @(
                    Get-ChildItem `
                        -LiteralPath $parentBarrierBackup `
                        -Force `
                        -ErrorAction Stop).Count -gt 0
            }
        }
        [void]$parentBarrierContinue.Set()
        $parentBarrierCompleted = $parentBarrierJob | Wait-Job -Timeout 30
        Assert-SelfTest `
            -Condition ($null -ne $parentBarrierCompleted -and
                $parentBarrierJob.State -ceq 'Completed') `
            -Message 'Output-parent barrier producer did not finish.'
        $parentBarrierResult = @($parentBarrierJob | Receive-Job)[-1]
        if ($parentBarrierSwapped) {
            Assert-SelfTest `
                -Condition (-not $parentBarrierResult.Succeeded -and
                    $parentBarrierResult.Error -cmatch '\(output_reparse_path\)' -and
                    -not (Test-Path -LiteralPath (
                        Join-Path $parentBarrierTarget (
                            [IO.Path]::GetFileName($repository.OutputPath))))) `
                -Message 'A final-window output-parent junction escaped the reparse fence.'
        }
        else {
            if ($parentBarrierResult.Succeeded) {
                Assert-SelfTest `
                    -Condition (Test-Path `
                        -LiteralPath $repository.OutputPath `
                        -PathType Leaf) `
                    -Message 'Denied output-parent swap reported success without an aggregate.'
            }
            else {
                Assert-SelfTest `
                    -Condition ($parentBarrierPartialMoveObserved -and
                        $parentBarrierResult.Error -cmatch
                        '\(historical_blob_replacement\)' -and
                        -not (Test-Path -LiteralPath $repository.OutputPath) -and
                        -not (Test-Path -LiteralPath (
                            Join-Path $parentBarrierTarget (
                                [IO.Path]::GetFileName(
                                    $repository.OutputPath))))) `
                    -Message 'Partially denied output-parent swap did not fail closed.'
                Assert-SelfTestRejected `
                    $parentBarrierResult `
                    $repository.OutputPath `
                    'historical_blob_replacement'
            }
        }
        $parentBarrierLeaf = [IO.Path]::GetFileName(
            $repository.OutputPath)
        Assert-SelfTest `
            -Condition (-not (Test-Path -LiteralPath (
                    Join-Path $parentBarrierTarget $parentBarrierLeaf)) -and
                -not (Test-Path -LiteralPath (
                    Join-Path $parentBarrierBackup $parentBarrierLeaf))) `
            -Message 'Output-parent barrier stranded a green aggregate outside its canonical path.'
    }
    finally {
        [void]$parentBarrierContinue.Set()
        if ($parentBarrierJob) {
            $parentBarrierJob | Remove-Job -Force -ErrorAction SilentlyContinue
        }
        if ($parentBarrierSwapped -and
            (Test-Path -LiteralPath $outputParent)) {
            Remove-SelfTestJunction `
                -Path $outputParent `
                -ContainmentRoot $tempRoot
        }
        foreach ($cleanupDirectory in @(
                $outputParent,
                $parentBarrierBackup)) {
            Remove-SelfTestContainedDirectory `
                -Path $cleanupDirectory `
                -ContainmentRoot $tempRoot
        }
        New-Item -ItemType Directory -Path $outputParent -Force | Out-Null
        $parentBarrierReady.Dispose()
        $parentBarrierContinue.Dispose()
    }

    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $junctionTarget = Join-Path $tempRoot 'output-junction-target'
    New-Item -ItemType Directory -Path $junctionTarget -Force | Out-Null
    if (Test-Path -LiteralPath $outputParent -PathType Container) {
        Remove-Item -LiteralPath $outputParent -Force
    }
    try {
        New-Item `
            -ItemType Junction `
            -Path $outputParent `
            -Value $junctionTarget | Out-Null
        $outputReparse = Invoke-SelfTestProducer `
            -EvidenceRoot $pristine.EvidenceRoot `
            -RunPaths $pristine.RunPaths `
            -OutputPath $repository.OutputPath `
            -RepositoryRoot $repository.Root
        Assert-SelfTest `
            -Condition (-not $outputReparse.Succeeded -and
                $outputReparse.Error -cmatch '\(output_reparse_path\)' -and
                -not (Test-Path -LiteralPath (
                    $repository.OutputPath + '.replacement-required.json'))) `
            -Message 'Output-parent reparse ancestry was not rejected without following it.'
    }
    finally {
        if (Test-Path -LiteralPath $outputParent) {
            Remove-SelfTestJunction `
                -Path $outputParent `
                -ContainmentRoot $tempRoot
        }
        New-Item -ItemType Directory -Path $outputParent -Force | Out-Null
    }

    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $preHistoryBaseline = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTest `
        -Condition $preHistoryBaseline.Succeeded `
        -Message ('Baseline could not be republished before tracked-history test: ' +
            $preHistoryBaseline.Error)

    $trackedHistoryPath = New-SelfTestTrackedHistory `
        -RepositoryRoot $repository.Root `
        -CurrentAggregatePath $repository.OutputPath
    $trackedHistoryRepositoryPath = 'docs/evidence/focused-autotest/' +
        [IO.Path]::GetFileName($trackedHistoryPath)

    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $omittedHistory = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root
    Assert-SelfTestRejected `
        $omittedHistory $repository.OutputPath 'history_census_drift'

    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $trackedHistoryBaseline = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root `
        -Historical @($trackedHistoryPath)
    Assert-SelfTest `
        -Condition $trackedHistoryBaseline.Succeeded `
        -Message ('Tracked history rejected a valid aggregate: ' +
            $trackedHistoryBaseline.Error)

    $historicalTamper = Read-SelfTestJson -Path $trackedHistoryPath
    $historicalTamper.sourceRuns[0].files[0].sha256 = 'f' * 64
    Write-SelfTestJson -Path $trackedHistoryPath -Value $historicalTamper
    try {
        Reset-SelfTestOutput -OutputPath $repository.OutputPath
        $historicalBlobDrift = Invoke-SelfTestProducer `
            -EvidenceRoot $pristine.EvidenceRoot `
            -RunPaths $pristine.RunPaths `
            -OutputPath $repository.OutputPath `
            -RepositoryRoot $repository.Root `
            -Historical @($trackedHistoryPath)
        Assert-SelfTestRejected `
            $historicalBlobDrift `
            $repository.OutputPath `
            'historical_blob_replacement'
    }
    finally {
        & git -C $repository.Root checkout --quiet HEAD -- `
            $trackedHistoryRepositoryPath
        if ($LASTEXITCODE -ne 0) {
            throw 'Focused aggregate self-test could not restore tracked history.'
        }
    }

    $schemaOnePath = New-SelfTestTrackedSchemaOneHistory `
        -RepositoryRoot $repository.Root
    $schemaOneRelative = 'docs/evidence/focused-autotest/' +
        [IO.Path]::GetFileName($schemaOnePath)
    $schemaOneBytes = [IO.File]::ReadAllBytes($schemaOnePath)
    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $schemaOneBaseline = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root `
        -Historical @($trackedHistoryPath)
    Assert-SelfTest `
        -Condition ($schemaOneBaseline.Succeeded -and
            [Convert]::ToBase64String([IO.File]::ReadAllBytes($schemaOnePath)) -ceq
                [Convert]::ToBase64String($schemaOneBytes)) `
        -Message ('Schema-one history was not preserved byte-for-byte: ' +
            $schemaOneBaseline.Error)

    [IO.File]::AppendAllText(
        $schemaOnePath,
        "`n",
        (New-Object Text.UTF8Encoding($false)))
    try {
        Reset-SelfTestOutput -OutputPath $repository.OutputPath
        $schemaOneWorktreeDrift = Invoke-SelfTestProducer `
            -EvidenceRoot $pristine.EvidenceRoot `
            -RunPaths $pristine.RunPaths `
            -OutputPath $repository.OutputPath `
            -RepositoryRoot $repository.Root `
            -Historical @($trackedHistoryPath)
        Assert-SelfTestRejected `
            $schemaOneWorktreeDrift `
            $repository.OutputPath `
            'historical_blob_replacement'
    }
    finally {
        & git -C $repository.Root checkout --quiet HEAD -- $schemaOneRelative
        if ($LASTEXITCODE -ne 0) {
            throw 'Focused aggregate self-test could not restore schema-one history.'
        }
    }

    $committedDriftRoot = Copy-SelfTestRepository `
        -Source $repository.Root `
        -Destination (Join-Path $tempRoot 'committed-history-drift')
    $committedDriftHistory = Join-Path `
        $committedDriftRoot `
        $trackedHistoryRepositoryPath.Replace('/', '\')
    $committedDriftValue = Read-SelfTestJson -Path $committedDriftHistory
    $committedDriftValue.sourceRuns[0].files[0].sha256 = 'f' * 64
    Write-SelfTestJson `
        -Path $committedDriftHistory `
        -Value $committedDriftValue
    & git -C $committedDriftRoot add -- $trackedHistoryRepositoryPath
    & git -C $committedDriftRoot commit --quiet -m 'committed history replacement'
    if ($LASTEXITCODE -ne 0) {
        throw 'Focused aggregate self-test could not commit isolated history drift.'
    }
    $committedDriftOutput = Join-Path $committedDriftRoot (
        'docs/evidence/focused-autotest/' + $repository.CandidateId + '.json')
    $committedHistoryDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $committedDriftOutput `
        -RepositoryRoot $committedDriftRoot `
        -Historical @($committedDriftHistory)
    Assert-SelfTestRejected `
        $committedHistoryDrift `
        $committedDriftOutput `
        'historical_blob_replacement'

    $schemaOneCommitDriftRoot = Copy-SelfTestRepository `
        -Source $repository.Root `
        -Destination (Join-Path $tempRoot 'schema-one-commit-drift')
    $schemaOneCommitDriftPath = Join-Path `
        $schemaOneCommitDriftRoot `
        $schemaOneRelative.Replace('/', '\')
    [IO.File]::AppendAllText(
        $schemaOneCommitDriftPath,
        "`n",
        (New-Object Text.UTF8Encoding($false)))
    & git -C $schemaOneCommitDriftRoot add -- $schemaOneRelative
    & git -C $schemaOneCommitDriftRoot commit --quiet -m 'schema-one replacement'
    if ($LASTEXITCODE -ne 0) {
        throw 'Focused aggregate self-test could not commit schema-one drift.'
    }
    $schemaOneCommitDriftHistory = Join-Path `
        $schemaOneCommitDriftRoot `
        $trackedHistoryRepositoryPath.Replace('/', '\')
    $schemaOneCommitDriftOutput = Join-Path $schemaOneCommitDriftRoot (
        'docs/evidence/focused-autotest/' + $repository.CandidateId + '.json')
    $schemaOneCommitDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $schemaOneCommitDriftOutput `
        -RepositoryRoot $schemaOneCommitDriftRoot `
        -Historical @($schemaOneCommitDriftHistory)
    Assert-SelfTestRejected `
        $schemaOneCommitDrift `
        $schemaOneCommitDriftOutput `
        'historical_blob_replacement'

    $falsePackageReceiptRoot = Copy-SelfTestRepository `
        -Source $repository.Root `
        -Destination (Join-Path $tempRoot 'tracked-false-package-red')
    $falsePackageCandidateId =
        'partisan-rc-false-package-20260716T000000Z'
    $falsePackageCandidateRelative =
        'docs/evidence/release-candidates/' + $falsePackageCandidateId
    $falsePackageManifestPath = Join-Path `
        $falsePackageReceiptRoot `
        ($falsePackageCandidateRelative + '/candidate.json').Replace('/', '\')
    $falsePackageReadyPath = Join-Path `
        $falsePackageReceiptRoot `
        ($falsePackageCandidateRelative + '/candidate.ready.json').Replace('/', '\')
    $clonedCurrentCandidateRelative = 'docs/evidence/release-candidates/' +
        $repository.CandidateId
    $clonedCurrentManifestPath = Join-Path `
        $falsePackageReceiptRoot `
        ($clonedCurrentCandidateRelative + '/candidate.json').Replace('/', '\')
    $clonedCurrentReadyPath = Join-Path `
        $falsePackageReceiptRoot `
        ($clonedCurrentCandidateRelative + '/candidate.ready.json').Replace('/', '\')
    $falsePackageManifest = Read-SelfTestJson `
        -Path $clonedCurrentManifestPath
    $falsePackageManifest.candidate.id = $falsePackageCandidateId
    $falseDeclaredPackageSha = 'd' * 64
    Assert-SelfTest `
        -Condition ($falseDeclaredPackageSha -cne
            (Get-SelfTestPackageSha256 `
                -Files @($falsePackageManifest.package.files))) `
        -Message 'False-package history fixture accidentally used the canonical digest.'
    $falsePackageManifest.package.sha256 = $falseDeclaredPackageSha
    Write-SelfTestJson `
        -Path $falsePackageManifestPath `
        -Value $falsePackageManifest
    $falsePackageManifestSha = Get-SelfTestSha256 `
        -Path $falsePackageManifestPath
    $falsePackageReady = Read-SelfTestJson -Path $clonedCurrentReadyPath
    $falsePackageReady.candidateId = $falsePackageCandidateId
    $falsePackageReady.packageSha256 = $falseDeclaredPackageSha
    $falsePackageReady.manifestSha256 = $falsePackageManifestSha
    Write-SelfTestJson `
        -Path $falsePackageReadyPath `
        -Value $falsePackageReady
    $falsePackageReadySha = Get-SelfTestSha256 `
        -Path $falsePackageReadyPath

    $falsePackageReceipt = $redBeforeGreenReceipt |
        ConvertTo-Json -Depth 100 | ConvertFrom-Json
    $falsePackageReceipt.candidate.candidateId = $falsePackageCandidateId
    $falsePackageReceipt.candidate.packageSha256 = $falseDeclaredPackageSha
    $falsePackageReceipt.candidate.manifestSha256 = $falsePackageManifestSha
    $falsePackageReceipt.candidate.readySha256 = $falsePackageReadySha
    $falsePackageReceiptRelative = 'docs/evidence/focused-autotest/' +
        $falsePackageCandidateId + '.json.replacement-required.json'
    $falsePackageReceiptPath = Join-Path `
        $falsePackageReceiptRoot `
        $falsePackageReceiptRelative.Replace('/', '\')
    Write-SelfTestJson `
        -Path $falsePackageReceiptPath `
        -Value $falsePackageReceipt

    $currentGuardReceiptRelative = 'docs/evidence/focused-autotest/' +
        $repository.CandidateId + '.json.replacement-required.json'
    $currentGuardReceiptPath = Join-Path `
        $falsePackageReceiptRoot `
        $currentGuardReceiptRelative.Replace('/', '\')
    Write-SelfTestJson `
        -Path $currentGuardReceiptPath `
        -Value $redBeforeGreenReceipt
    & git -C $falsePackageReceiptRoot add -- `
        $falsePackageCandidateRelative `
        $falsePackageReceiptRelative `
        $currentGuardReceiptRelative
    & git -C $falsePackageReceiptRoot commit --quiet -m `
        'tracked coherently sealed false package receipt'
    if ($LASTEXITCODE -ne 0) {
        throw 'Focused aggregate self-test could not commit false-package RED history.'
    }
    $falsePackageReceiptHash = Get-SelfTestSha256 `
        -Path $falsePackageReceiptPath
    $currentGuardReceiptHash = Get-SelfTestSha256 `
        -Path $currentGuardReceiptPath
    $falsePackageHistoryPath = Join-Path `
        $falsePackageReceiptRoot `
        $trackedHistoryRepositoryPath.Replace('/', '\')
    $falsePackageOutput = Join-Path $falsePackageReceiptRoot (
        'docs/evidence/focused-autotest/' + $repository.CandidateId + '.json')
    $falsePackageHistory = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $falsePackageOutput `
        -RepositoryRoot $falsePackageReceiptRoot `
        -Historical @($falsePackageHistoryPath)
    $falsePackageRejectionLines = @($falsePackageHistory.Output |
        Where-Object {
            $_ -is [string] -and
            $_.StartsWith(
                'FOCUSED_AGGREGATE_REJECTED ',
                [StringComparison]::Ordinal)
        })
    $falsePackageRejection = if ($falsePackageRejectionLines.Count -eq 1) {
        $falsePackageRejectionLines[0].Substring(
            'FOCUSED_AGGREGATE_REJECTED '.Length) | ConvertFrom-Json
    }
    else {
        $null
    }
    $falsePackageReceiptRows = @(Get-ChildItem `
        -LiteralPath (Split-Path -Parent $falsePackageOutput) `
        -File `
        -Filter '*.replacement-required.json')
    Assert-SelfTest `
        -Condition (-not $falsePackageHistory.Succeeded -and
            $falsePackageHistory.Error -cmatch
                '\(historical_blob_replacement\)' -and
            -not (Test-Path -LiteralPath $falsePackageOutput) -and
            $null -ne $falsePackageRejection -and
            -not [bool]$falsePackageRejection.durableReceiptCreated -and
            $falsePackageReceiptRows.Count -eq 2 -and
            (Get-SelfTestSha256 -Path $falsePackageReceiptPath) -ceq
                $falsePackageReceiptHash -and
            (Get-SelfTestSha256 -Path $currentGuardReceiptPath) -ceq
                $currentGuardReceiptHash) `
        -Message 'Tracked false package digest was trusted or caused RED/green publication.'

    foreach ($receiptSealName in @(
            'packageSha256',
            'manifestSha256',
            'readySha256')) {
        $trackedWrongSealRoot = Copy-SelfTestRepository `
            -Source $repository.Root `
            -Destination (Join-Path $tempRoot (
                'tracked-wrong-seal-red-' +
                $receiptSealName.ToLowerInvariant()))
        $trackedWrongSealRelative = 'docs/evidence/focused-autotest/' +
            $repository.CandidateId + '.json.replacement-required.json'
        $trackedWrongSealPath = Join-Path `
            $trackedWrongSealRoot `
            $trackedWrongSealRelative.Replace('/', '\')
        Write-SelfTestJson `
            -Path $trackedWrongSealPath `
            -Value $wrongSealReceipts[$receiptSealName]
        & git -C $trackedWrongSealRoot add -- $trackedWrongSealRelative
        & git -C $trackedWrongSealRoot commit --quiet -m `
            "tracked same-ID wrong $receiptSealName red"
        if ($LASTEXITCODE -ne 0) {
            throw "Focused aggregate self-test could not commit wrong $receiptSealName RED history."
        }
        $trackedWrongSealHash = Get-SelfTestSha256 `
            -Path $trackedWrongSealPath
        $trackedWrongSealHistoryPath = Join-Path `
            $trackedWrongSealRoot `
            $trackedHistoryRepositoryPath.Replace('/', '\')
        $trackedWrongSealOutput = Join-Path $trackedWrongSealRoot (
            'docs/evidence/focused-autotest/' +
            $repository.CandidateId + '.json')
        $trackedWrongSeal = Invoke-SelfTestProducer `
            -EvidenceRoot $pristine.EvidenceRoot `
            -RunPaths $pristine.RunPaths `
            -OutputPath $trackedWrongSealOutput `
            -RepositoryRoot $trackedWrongSealRoot `
            -Historical @($trackedWrongSealHistoryPath)
        Assert-SelfTest `
            -Condition (-not $trackedWrongSeal.Succeeded -and
                $trackedWrongSeal.Error -cmatch
                    '\(historical_blob_replacement\)' -and
                -not (Test-Path -LiteralPath $trackedWrongSealOutput) -and
                (Get-SelfTestSha256 -Path $trackedWrongSealPath) -ceq
                    $trackedWrongSealHash) `
            -Message ("Tracked wrong $receiptSealName RED history was " +
                'trusted, overwritten, or allowed green.')
    }

    $trackedRedRoot = Copy-SelfTestRepository `
        -Source $repository.Root `
        -Destination (Join-Path $tempRoot 'tracked-red-history')
    $trackedRedRelative = 'docs/evidence/focused-autotest/' +
        $repository.CandidateId + '.json.replacement-required.json'
    $trackedRedPath = Join-Path `
        $trackedRedRoot `
        $trackedRedRelative.Replace('/', '\')
    Write-SelfTestJson `
        -Path $trackedRedPath `
        -Value $redBeforeGreenReceipt
    & git -C $trackedRedRoot add -- $trackedRedRelative
    & git -C $trackedRedRoot commit --quiet -m 'tracked candidate-bound red'
    if ($LASTEXITCODE -ne 0) {
        throw 'Focused aggregate self-test could not commit tracked RED history.'
    }
    $trackedRedHistoryPath = Join-Path `
        $trackedRedRoot `
        $trackedHistoryRepositoryPath.Replace('/', '\')
    $trackedRedOutput = Join-Path $trackedRedRoot (
        'docs/evidence/focused-autotest/' + $repository.CandidateId + '.json')
    $trackedRedFirst = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $trackedRedOutput `
        -RepositoryRoot $trackedRedRoot `
        -Historical @($trackedRedHistoryPath)
    Assert-SelfTest `
        -Condition (-not $trackedRedFirst.Succeeded -and
            $trackedRedFirst.Error -cmatch '\(replacement_required\)' -and
            -not (Test-Path -LiteralPath $trackedRedOutput)) `
        -Message 'A valid tracked RED did not retain first-publication precedence.'
    $trackedRedChanged = Read-SelfTestJson -Path $trackedRedPath
    $trackedRedChanged.admission.reasonCode = 'second-red-reason'
    Write-SelfTestJson -Path $trackedRedPath -Value $trackedRedChanged
    & git -C $trackedRedRoot add -- $trackedRedRelative
    & git -C $trackedRedRoot commit --quiet -m 'replace tracked red bytes'
    if ($LASTEXITCODE -ne 0) {
        throw 'Focused aggregate self-test could not commit tracked RED drift.'
    }
    $trackedRedChangedHash = Get-SelfTestSha256 -Path $trackedRedPath
    $trackedRedDrift = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $trackedRedOutput `
        -RepositoryRoot $trackedRedRoot `
        -Historical @($trackedRedHistoryPath)
    Assert-SelfTest `
        -Condition (-not $trackedRedDrift.Succeeded -and
            $trackedRedDrift.Error -cmatch '\(historical_blob_replacement\)' -and
            (Get-SelfTestSha256 -Path $trackedRedPath) -ceq
                $trackedRedChangedHash) `
        -Message 'Tracked RED history allowed committed byte replacement.'

    Reset-SelfTestOutput -OutputPath $repository.OutputPath
    $immutableBaseline = Invoke-SelfTestProducer `
        -EvidenceRoot $pristine.EvidenceRoot `
        -RunPaths $pristine.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root `
        -Historical @($trackedHistoryPath)
    Assert-SelfTest `
        -Condition $immutableBaseline.Succeeded `
        -Message ('Baseline could not be republished before immutable-output test: ' +
            $immutableBaseline.Error)
    $immutableHash = Get-SelfTestSha256 -Path $repository.OutputPath
    $replacementCase = New-SelfTestCaseFixture `
        -PristineEvidence $pristine.EvidenceRoot `
        -CaseRoot (Join-Path $tempRoot 'historical-replacement')
    $replacementRun = Read-SelfTestJson -Path $replacementCase.RunPaths[0]
    $scriptPortable = [string](@($replacementRun.files | Where-Object {
        $_.path -cmatch '/script\.log$'
    })[0].path)
    $scriptFull = Join-Path `
        (Split-Path -Parent $replacementCase.RunPaths[0]) `
        $scriptPortable.Replace('/', '\')
    [IO.File]::AppendAllText($scriptFull, "replacement attempt`n")
    Update-SelfTestRunIndex `
        -RunPath $replacementCase.RunPaths[0] `
        -PortablePath $scriptPortable
    $replacement = Invoke-SelfTestProducer `
        -EvidenceRoot $replacementCase.EvidenceRoot `
        -RunPaths $replacementCase.RunPaths `
        -OutputPath $repository.OutputPath `
        -RepositoryRoot $repository.Root `
        -Historical @($trackedHistoryPath)
    $replacementReceipt = $repository.OutputPath + '.replacement-required.json'
    Assert-SelfTestRejected `
        $replacement $replacementReceipt 'historical_blob_replacement'
    Assert-SelfTest `
        -Condition ((Get-SelfTestSha256 -Path $repository.OutputPath) -ceq
            $immutableHash) `
        -Message 'Historical blob replacement overwrote the accepted aggregate.'

    $focusedHistoryRoot = Split-Path -Parent $repository.OutputPath
    $publicationResidue = @(Get-ChildItem `
        -LiteralPath $focusedHistoryRoot `
        -File `
        -Force `
        -ErrorAction Stop | Where-Object {
            $_.Name -cmatch '\.publication\.lock$' -or
            $_.Name -cmatch '\.[0-9a-f]{32}\.tmp$'
        })
    Assert-SelfTest `
        -Condition ($publicationResidue.Count -eq 0) `
        -Message 'Focused aggregate self-test found publication lock or temporary residue.'
    $passPayload = [pscustomobject][ordered]@{
        status = 'PASS'
        checks = $checkCount
        acceptedSchemaVersion = 2
        acceptedContractId = 'partisan.focused-autotest.aggregate.v2'
        canonicalProfiles = 5
        individuallyNamedCases = 91
        filesPerProfile = 8
        totalFiles = 40
        aggregatePolicyAssertions = '35/35'
        junit = '91/0/0/0'
        gitBlobProvenance = 'runner,module,producer,consumer'
        trackedHistory = 'schema-1/schema-2/RED reachable-history immutable blobs and exact census'
        publicationSnapshot = 'held read handles with full current-candidate tree census and byte-hash sweeps'
        strictEncoding = 'captured BOM-less UTF-8 bytes'
        consoleMarkers = 'exact markers, mount records, and nonempty required-pattern contract'
        concurrentPublication = 'immutable green and candidate-bound RED first-publication races'
        rejectionDisposition = 'RED replacement-required with 5/40 sealed inputs'
        temporaryFixtureCleanup = 'verified before PASS emission'
    }
}
finally {
    if ($tempRoot -and (Test-Path -LiteralPath $tempRoot)) {
        $tempFull = [IO.Path]::GetFullPath($tempRoot).TrimEnd('\', '/')
        $tempBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd(
            '\', '/')
        $expectedPrefix = $tempBase + [IO.Path]::DirectorySeparatorChar +
            'PartisanFocusedAggregateSelfTest-'
        if (-not $tempFull.StartsWith(
                $expectedPrefix,
                [StringComparison]::OrdinalIgnoreCase)) {
            throw 'Focused aggregate self-test refused unsafe temporary cleanup.'
        }
        Remove-Item -LiteralPath $tempFull -Recurse -Force
        if (Test-Path -LiteralPath $tempFull) {
            throw 'Focused aggregate self-test temporary cleanup did not converge.'
        }
    }
}

if ($null -eq $passPayload) {
    throw 'Focused aggregate self-test completed without a PASS payload.'
}
Write-Output ('FOCUSED_AGGREGATE_SELFTEST ' +
    ($passPayload | ConvertTo-Json -Compress))
