[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'update-release-docs.ps1') -LibraryOnly -EvidenceBundleRoot ''

$producerPath = Join-Path $PSScriptRoot 'New-PartisanCampaignDebugReleaseIndex.ps1'
$producerSha = (Get-FileHash -LiteralPath $producerPath -Algorithm SHA256).Hash.ToLowerInvariant()
$consumerSha = (Get-FileHash `
    -LiteralPath (Join-Path $PSScriptRoot 'update-release-docs.ps1') `
    -Algorithm SHA256).Hash.ToLowerInvariant()
$runnerSha = (Get-FileHash `
    -LiteralPath (Join-Path $PSScriptRoot 'run-guarded-campaign-debug.ps1') `
    -Algorithm SHA256).Hash.ToLowerInvariant()
$moduleSha = (Get-FileHash `
    -LiteralPath (Join-Path $PSScriptRoot 'Partisan.ReleaseCandidate.psm1') `
    -Algorithm SHA256).Hash.ToLowerInvariant()
$checkoutRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$fixtureCleanupRegistry = New-Object Collections.Generic.List[object]
$fixtureOwnerMarkerName = '.partisan-release-index-selftest-owner'
$fixtureOwnerProcessId = [int]$PID
$fixtureOwnerProcessStartUtc = (Get-Process -Id $PID -ErrorAction Stop).
    StartTime.ToUniversalTime().ToString('o')

function Get-CampaignDebugSelfTestTextSha256 {
    param([Parameter(Mandatory = $true)][string]$Text)

    $encoding = [Text.UTF8Encoding]::new($false)
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString(
                $sha.ComputeHash($encoding.GetBytes($Text)))).
            Replace('-', '').ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
}

$headRows = @(& git -C $checkoutRoot rev-parse HEAD 2>$null)
$headExitCode = $LASTEXITCODE
$harnessHead = ($headRows -join '').Trim()
if ($headExitCode -ne 0 -or $harnessHead -cnotmatch '^[0-9a-f]{40}$') {
    throw 'The full-profile self-test could not resolve the harness Git HEAD.'
}
$runnerGitBlobSha = [string](Get-GitBlobTextAndSha256 `
        $harnessHead `
        'tools/run-guarded-campaign-debug.ps1' `
        'Full-profile self-test guarded runner').Sha256
$candidateModuleGitBlobSha = [string](Get-GitBlobTextAndSha256 `
        $harnessHead `
        'tools/Partisan.ReleaseCandidate.psm1' `
        'Full-profile self-test candidate module').Sha256
$producerGitBlobSha = [string](Get-GitBlobTextAndSha256 `
        $harnessHead `
        'tools/New-PartisanCampaignDebugReleaseIndex.ps1' `
        'Full-profile self-test release-index producer').Sha256
$consumerGitBlobSha = [string](Get-GitBlobTextAndSha256 `
        $harnessHead `
        'tools/update-release-docs.ps1' `
        'Full-profile self-test release-docs consumer').Sha256
$candidateId = 'partisan-rc-0123456789ab-20260719T120000Z'
$candidateHead = '0' * 40
$candidateVersion = '0.1.0-rc.synthetic'
$embeddedBuildSha = '1' * 40
$embeddedBuildUtc = '2026-07-19T11:50:00Z'
$embeddedBuildLabel = 'schema71-settings24-release-index-selftest'
$campaignSchema = 71
$runtimeSettingsSchema = 24
$addonId = 'histasi'
$addonGuid = '698532771130111D'
$packageHashAlgorithm = 'sha256-manifest-v1'
$packageFiles = @(
    [pscustomobject][ordered]@{
        path = 'package/Partisan/addon.gproj'
        indexPath = 'Partisan/addon.gproj'
        length = 499
        sha256 = '3' * 64
    },
    [pscustomobject][ordered]@{
        path = 'package/Partisan/data.pak'
        indexPath = 'Partisan/data.pak'
        length = 4096
        sha256 = '4' * 64
    },
    [pscustomobject][ordered]@{
        path = 'package/Partisan/resourceDatabase.rdb'
        indexPath = 'Partisan/resourceDatabase.rdb'
        length = 2048
        sha256 = '5' * 64
    },
    [pscustomobject][ordered]@{
        path = 'package/Partisan/thumbnail.png'
        indexPath = 'Partisan/thumbnail.png'
        length = 1024
        sha256 = '6' * 64
    })
$packageCanonicalRows = @($packageFiles | Sort-Object indexPath | ForEach-Object {
        "{0}`t{1}`t{2}" -f $_.sha256, ([long]$_.length), $_.indexPath
    })
$packageSha = Get-CampaignDebugSelfTestTextSha256 `
    (($packageCanonicalRows -join "`n") + "`n")
$workbenchCrc = '0123abcd'
$clientDiagnosticIdentity = [PSCustomObject][ordered]@{
    fileName = 'runtime-client-diagnostic.exe'
    fileVersion = '1.0.0.0'
    productVersion = '1.0.0.0'
    length = 4096
    sha256 = '4' * 64
}
$clientRuntimeIdentity = [PSCustomObject][ordered]@{
    fileName = 'runtime-client.exe'
    fileVersion = '1.0.0.0'
    productVersion = '1.0.0.0'
    length = 8192
    sha256 = '5' * 64
}
$syntheticCandidateManifest = [PSCustomObject][ordered]@{
    manifestSchemaVersion = 1
    createdUtc = '2026-07-19T11:55:00Z'
    candidate = [PSCustomObject][ordered]@{
        id = $candidateId
        version = $candidateVersion
        state = 'retained-uncertified'
    }
    source = [PSCustomObject][ordered]@{
        gitHead = $candidateHead
        embeddedImplementation = [PSCustomObject][ordered]@{
            sha = $embeddedBuildSha
            utc = $embeddedBuildUtc
            label = $embeddedBuildLabel
        }
        campaignSchema = $campaignSchema
        runtimeSettingsSchema = $runtimeSettingsSchema
    }
    addon = [PSCustomObject][ordered]@{
        id = $addonId
        guid = $addonGuid
        version = $candidateVersion
    }
    toolchain = [PSCustomObject][ordered]@{
        client = $clientRuntimeIdentity
        clientDiagnostic = $clientDiagnosticIdentity
    }
    workbench = [PSCustomObject][ordered]@{
        crc = $workbenchCrc
    }
    package = [PSCustomObject][ordered]@{
        root = 'package/Partisan'
        hashAlgorithm = $packageHashAlgorithm
        sha256 = $packageSha
        canonicalIndexPath = 'evidence/pack/files.sha256'
        files = $packageFiles
    }
}
$backslash = [string][char]92
$syntheticUncPath = $backslash + $backslash + 'synthetic-host' + $backslash + 'share'
$syntheticRootedPath = $backslash + 'synthetic-root' + $backslash + 'evidence'
$syntheticFileUri = 'error opening file:' + '//' + 'synthetic-host/share/item'
$syntheticEmbeddedFileUri = 'prefix:' + 'file:' + '//' + 'synthetic-host/share/item'
$syntheticColonForwardRoot = 'path:' + '/' + 'synthetic-root/evidence'
$syntheticColonForwardUnc = 'path:' + '//' + 'synthetic-host/share/item'
$syntheticUpperHttps = 'HTTPS://example.invalid/evidence'
$syntheticMalformedHttpsRoot = 'https:' + '/' + 'synthetic-root/evidence'
$syntheticMalformedHttpsTriple = 'https:' + '///' + 'synthetic-root/evidence'
$syntheticMalformedHttpsDriveHost = 'https:' + '//' + 'C:' + '/synthetic-root/evidence'
$externalAdvisoryIds = @(
    'isolation.world_scope',
    'persistence.real_restart',
    'phase25.real_restart',
    'phase25.second_client',
    'phase25.two_hour_soak')

$trustedTools = [PSCustomObject] @{
    gitHead = $harnessHead
    campaignRunnerSha256 = $runnerSha
    campaignRunnerGitBlobSha256 = $runnerGitBlobSha
    candidateModuleSha256 = $moduleSha
    candidateModuleGitBlobSha256 = $candidateModuleGitBlobSha
    releaseIndexProducerSha256 = $producerSha
    releaseIndexProducerGitBlobSha256 = $producerGitBlobSha
    releaseDocsConsumerSha256 = $consumerSha
    releaseDocsConsumerGitBlobSha256 = $consumerGitBlobSha
}
$candidateIdentity = [PSCustomObject] @{
    CandidateId = $candidateId
    CandidateSourceHead = $candidateHead
    PackageHashAlgorithm = $packageHashAlgorithm
    PackageSha256 = $packageSha
    PackageVersion = $candidateVersion
    ManifestSha256 = $null
    ReadySha256 = $null
    WorkbenchCrc = $workbenchCrc
    CampaignSchema = $campaignSchema
    RuntimeSettingsSchema = $runtimeSettingsSchema
    Manifest = $syntheticCandidateManifest
}

function Write-Utf8Text {
    param([string]$Path, [string]$Text)

    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    [IO.File]::WriteAllText($Path, $Text, (New-Object Text.UTF8Encoding($false)))
}

function Write-Json {
    param([string]$Path, $Value)

    Write-Utf8Text $Path (($Value | ConvertTo-Json -Depth 32).Replace("`r`n", "`n") + "`n")
}

function New-Assertion {
    param(
        [string]$Id,
        [string]$Status = 'PASS',
        [bool]$Counts = $true,
        [string]$ProofLevel = 'CONTROLLED_RUNTIME',
        [string]$ObservedPath = 'synthetic_runtime_probe',
        [string]$RequiredPath = 'typed runtime proof',
        [string]$Reason = 'synthetic failure reason',
        [string]$Expected = '',
        [string]$Actual = ''
    )

    return [PSCustomObject][ordered]@{
        m_sAssertionId = $Id
        m_sExpected = if ([string]::IsNullOrWhiteSpace($Expected)) { "expected $Id" } else { $Expected }
        m_sActual = if ([string]::IsNullOrWhiteSpace($Actual)) { "actual $Id" } else { $Actual }
        m_sStatus = $Status
        m_sFailureReason = $Reason
        m_sProofLevel = $ProofLevel
        m_sObservedPath = $ObservedPath
        m_sRequiredPath = $RequiredPath
        m_bCountsTowardCertification = $Counts
        m_vExpectedPosition = @(0, 0, 0)
        m_vActualPosition = @(0, 0, 0)
        m_fDistanceMeters = 0
        m_sEntityId = ''
        m_sMissionInstanceId = ''
        m_sZoneId = ''
        m_sOrderId = ''
    }
}

function New-Case {
    param(
        [string]$Id,
        [string]$Status,
        [object[]]$Assertions,
        [string]$Category = 'foundation',
        [string]$Feature = 'synthetic',
        [string]$Stage = 'runtime',
        [object[]]$Metrics = @()
    )

    return [PSCustomObject][ordered]@{
        m_sCaseId = $Id
        m_sCategory = $Category
        m_sFeature = $Feature
        m_sStage = $Stage
        m_sStatus = $Status
        m_sReason = if ($Status -ceq 'PASS') { 'assertions passed' } else { 'synthetic disposition' }
        m_iStartSecond = 0
        m_iEndSecond = 1
        m_aAssertions = $Assertions
        m_aMetrics = $Metrics
        m_aEvidence = @('synthetic portable fixture')
    }
}

function New-Metric {
    param(
        [string]$Id,
        [string]$Value,
        [string]$Unit = '',
        [string]$Feature = '',
        [string]$Stage = ''
    )

    return [PSCustomObject][ordered]@{
        m_sMetricId = $Id
        m_sValue = $Value
        m_sUnit = $Unit
        m_sFeature = $Feature
        m_sStage = $Stage
    }
}

function Get-SyntheticAcceptedDiagnosticLogText {
    param([switch]$StockOnly)

    $lines = New-Object Collections.Generic.List[string]
    foreach ($line in @(
            '17:00:00.000 SCRIPT (E): Virtual Machine Exception',
            'Reason: Wrong parameter value',
            "Class: 'SCR_EditableEntityCore'",
            "Function: 'GetPlayerIdentityId'",
            'Stack trace:',
            'Scripts/Game/Utilities/SCR_PlayerIdentityUtils.c:29 Function GetPlayerIdentityId',
            'Scripts/Game/Editor/Core/SCR_EditableEntityCore.c:1189 Function OnPlayerIdentityAvailable',
            'Scripts/Game/GameMode/SCR_BaseGameMode.c:842 Function OnPlayerAuditSuccess',
            '17:00:00.001 SCRIPT (E): Virtual Machine Exception',
            'Reason: Wrong parameter value',
            "Class: 'SCR_ReconnectComponent'",
            "Function: 'GetPlayerIdentityId'",
            'Stack trace:',
            'Scripts/Game/Utilities/SCR_PlayerIdentityUtils.c:29 Function GetPlayerIdentityId',
            'Scripts/Game/GameMode/Components/SCR_ReconnectComponent.c:135 Function HandlePlayerReconnect',
            'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:534 Function ResolveReconnection',
            'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:329 Function OnPlayerDataLoaded_S',
            'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:298 Function RequestPlayerData_S',
            'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:273 Function ExcuteInitialLoadOrSpawn_S',
            'Scripts/Game/Respawn/Logic/SCR_AutoSpawnLogic.c:18 Function OnPlayerAuditSuccess_S',
            'Scripts/Game/GameMode/Respawn/SCR_RespawnSystemComponent.c:258 Function OnPlayerAuditSuccess_S',
            'Scripts/Game/GameMode/SCR_BaseGameMode.c:845 Function OnPlayerAuditSuccess')) {
        [void]$lines.Add($line)
    }
    if ($StockOnly) {
        $stockText = ($lines.ToArray() -join "`n") + "`n"
        $stockText = $stockText.Replace(
            '17:00:00.000 SCRIPT (E): Virtual Machine Exception',
            'Virtual Machine Exception')
        $stockText = $stockText.Replace(
            '17:00:00.001 SCRIPT (E): Virtual Machine Exception',
            'Virtual Machine Exception')
        return $stockText
    }
    foreach ($line in @(
            '17:00:01.000 SCRIPT : Partisan campaign debug CLI | armed exact HST_Dev full certification run',
            '17:00:01.100 SCRIPT : Partisan campaign debug CLI | started full_certification on attempt 1')) {
        [void]$lines.Add($line)
    }
    $intentionalMessages = @(
        'Partisan exact mission convoy | mission_convoy_proof_admission_rollback failed closed: exact mission convoy road route is missing or too short',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_duplicate failed closed: exact mission convoy admission contains more than one optional cargo row',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_invalid_prefab failed closed: exact mission convoy cargo prefab is missing, invalid, or not an entity prefab',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_wrong_kind failed closed: exact mission convoy cargo role or kind is incompatible with the mission',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_disallowed failed closed: exact mission convoy mission kind does not permit a cargo row',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_missing failed closed: exact mission convoy mission kind requires exactly one compatible cargo row',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_wrong_runtime failed closed: exact mission convoy runtime type is incompatible with frozen convoy authority',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_captive_non_character failed closed: exact mission convoy captive prefab is not a boardable character with compartment access',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_payload_character failed closed: exact mission convoy payload prefab must be a non-character mission-asset entity',
        'Partisan exact mission convoy | mission_convoy_proof_duplicate_corruption failed closed: exact mission convoy authority identity is ambiguous',
        'Partisan exact mission convoy | mission_convoy_proof_hash_corruption failed closed: exact mission convoy manifest hash changed after admission',
        'Partisan exact mission convoy | mission_convoy_proof_missing_backlink_corruption failed closed: exact mission convoy canonical mission authority links conflict',
        'Partisan exact mission convoy | mission_convoy_proof_watchdog failed closed: all-or-nothing convoy materialization did not confirm every required surviving crew/vehicle root before timeout')
    for ($index = 0; $index -lt $intentionalMessages.Count; $index++) {
        [void]$lines.Add(
            ('17:00:02.{0:D3} SCRIPT (E): {1}' -f $index, $intentionalMessages[$index]))
    }
    [void]$lines.Add(
        '17:00:03.000 SCRIPT : Partisan campaign debug | PASS | early_mechanics.force_authority | assertions passed')
    return ($lines.ToArray() -join "`n") + "`n"
}

function Set-RawCounts {
    param($Raw)

    $caseGroups = @($Raw.m_aCases | Group-Object m_sStatus -CaseSensitive)
    $caseCount = {
        param([string]$Status)
        $group = @($caseGroups | Where-Object Name -ceq $Status)
        if ($group.Count -eq 0) { return 0 }
        return [int]$group[0].Count
    }
    $certAssertions = @($Raw.m_aCases | ForEach-Object { $_.m_aAssertions } |
        Where-Object { $_.m_bCountsTowardCertification })
    $certCount = {
        param([string]$Status)
        return @($certAssertions | Where-Object m_sStatus -ceq $Status).Count
    }
    $Raw.m_iPassCount = & $caseCount 'PASS'
    $Raw.m_iWarnCount = & $caseCount 'WARN'
    $Raw.m_iFailCount = & $caseCount 'FAIL'
    $Raw.m_iBlockedCount = & $caseCount 'BLOCKED'
    $Raw.m_iSkippedCount = & $caseCount 'SKIPPED'
    $Raw.m_iCertificationRequiredCount = $certAssertions.Count
    $Raw.m_iCertificationProvenCount = & $certCount 'PASS'
    $Raw.m_iCertificationFailCount = & $certCount 'FAIL'
    $Raw.m_iCertificationBlockedCount = & $certCount 'BLOCKED'
    $Raw.m_iCertificationWarnCount = & $certCount 'WARN'
    $Raw.m_bCertificationPassed =
        $Raw.m_iCertificationRequiredCount -eq $Raw.m_iCertificationProvenCount -and
        $Raw.m_iCertificationFailCount -eq 0 -and
        $Raw.m_iCertificationBlockedCount -eq 0 -and
        $Raw.m_iCertificationWarnCount -eq 0
}

function ConvertTo-RecordedValidationSummary {
    param([Parameter(Mandatory = $true)]$Validation)

    $recordedPhase17 = @($Validation.Phase17 | ForEach-Object {
        [pscustomobject][ordered]@{
            Id = [string]$_.Id
            Pass = [bool]$_.Pass
        }
    })
    $recordedPhase24 = @($Validation.Phase24 | ForEach-Object {
        [pscustomobject][ordered]@{
            Id = [string]$_.Id
            Pass = [bool]$_.Pass
            Accepted = [bool]$_.Accepted
            Status = [string]$_.Status
            Actual = [string]$_.Actual
        }
    })
    $recordedStagedCleanup = @($Validation.StagedCleanup | ForEach-Object {
        [pscustomobject][ordered]@{
            Id = [string]$_.Id
            Pass = [bool]$_.Pass
            CaseStatus = [string]$_.CaseStatus
            ActiveGroupsStatus = [string]$_.ActiveGroupsStatus
            RuntimeFactionsStatus = [string]$_.RuntimeFactionsStatus
            RuntimeGroupPopulationSettledStatus =
                [string]$_.RuntimeGroupPopulationSettledStatus
            ExpectedZeroMemberGraceApplied =
                $_.ExpectedZeroMemberGraceApplied
            OrphanActiveGroups = $_.OrphanActiveGroups
            RuntimeFactionMismatches = $_.RuntimeFactionMismatches
            ZeroMemberGraceCandidates = $_.ZeroMemberGraceCandidates
            PendingPopulationGroups = $_.PendingPopulationGroups
        }
    })
    $recordedFocusedAssertions = @($Validation.FocusedAssertions | ForEach-Object {
        [pscustomobject][ordered]@{
            Id = [string]$_.Id
            Pass = [bool]$_.Pass
            Status = [string]$_.Status
            Actual = [string]$_.Actual
        }
    })
    $recordedPhase17Metrics = [pscustomobject][ordered]@{
        spawn_ticks = $Validation.Phase17Metrics.spawn_ticks
        spawn_tick_limit = $Validation.Phase17Metrics.spawn_tick_limit
        spawn_deferred_ticks = $Validation.Phase17Metrics.spawn_deferred_ticks
        physical_settle_ticks = $Validation.Phase17Metrics.physical_settle_ticks
        casualty_reentry_physical_settle_ticks =
            $Validation.Phase17Metrics.casualty_reentry_physical_settle_ticks
        survivor_reentry_physical_settle_ticks =
            $Validation.Phase17Metrics.survivor_reentry_physical_settle_ticks
        physical_settle_limit = $Validation.Phase17Metrics.physical_settle_limit
        casualty_settle_ticks = $Validation.Phase17Metrics.casualty_settle_ticks
        casualty_settle_limit = $Validation.Phase17Metrics.casualty_settle_limit
        elapsed_peak = $Validation.Phase17Metrics.elapsed_peak
        expected_living = $Validation.Phase17Metrics.expected_living
    }
    $recordedPhase24Metrics = [pscustomobject][ordered]@{
        runtime_owner_expected = $Validation.Phase24Metrics.runtime_owner_expected
        runtime_owner_classified = $Validation.Phase24Metrics.runtime_owner_classified
        runtime_owner_snapshot_invariant_failures =
            $Validation.Phase24Metrics.runtime_owner_snapshot_invariant_failures
        exact_counterattack_orders =
            $Validation.Phase24Metrics.exact_counterattack_orders
        exact_counterattack_open_orders =
            $Validation.Phase24Metrics.exact_counterattack_open_orders
        exact_counterattack_terminal_ledgers =
            $Validation.Phase24Metrics.exact_counterattack_terminal_ledgers
        exact_counterattack_invalid_authority =
            $Validation.Phase24Metrics.exact_counterattack_invalid_authority
        exact_counterattack_projection_groups =
            $Validation.Phase24Metrics.exact_counterattack_projection_groups
        exact_counterattack_virtual_groups =
            $Validation.Phase24Metrics.exact_counterattack_virtual_groups
        exact_counterattack_materializing_groups =
            $Validation.Phase24Metrics.exact_counterattack_materializing_groups
        exact_counterattack_physical_groups =
            $Validation.Phase24Metrics.exact_counterattack_physical_groups
        exact_counterattack_dematerializing_groups =
            $Validation.Phase24Metrics.exact_counterattack_dematerializing_groups
        exact_counterattack_support_leaks =
            $Validation.Phase24Metrics.exact_counterattack_support_leaks
        exact_counterattack_invalid_authority_rows =
            $Validation.Phase24Metrics.exact_counterattack_invalid_authority_rows
    }
    return [pscustomobject][ordered]@{
        Valid = [bool]$Validation.Valid
        Problems = @($Validation.Problems)
        RunId = [string]$Validation.RunId
        Profile = [string]$Validation.Profile
        ProofScope = [string]$Validation.ProofScope
        FullCertification = [bool]$Validation.FullCertification
        BuildProvenanceMatches =
            [string]$Validation.BuildSha -eq $embeddedBuildSha -and
            [string]$Validation.BuildUtc -eq $embeddedBuildUtc -and
            [string]$Validation.BuildLabel -eq $embeddedBuildLabel
        StartedAtSecond = [int]$Validation.StartedAtSecond
        EndedAtSecond = [int]$Validation.EndedAtSecond
        CaseCount = [int]$Validation.CaseCount
        Pass = [int]$Validation.Pass
        Warn = [int]$Validation.Warn
        Fail = [int]$Validation.Fail
        Blocked = [int]$Validation.Blocked
        Skipped = [int]$Validation.Skipped
        CertificationRequired = [int]$Validation.CertificationRequired
        CertificationProven = [int]$Validation.CertificationProven
        CertificationFail = [int]$Validation.CertificationFail
        CertificationBlocked = [int]$Validation.CertificationBlocked
        CertificationWarn = [int]$Validation.CertificationWarn
        CertificationPassed = [bool]$Validation.CertificationPassed
        CorrectedCanaryContract = [bool]$Validation.CorrectedCanaryContract
        Trigger = [string]$Validation.Trigger
        ArtifactCount = [int]$Validation.ArtifactCount
        StateDiffRows = [int]$Validation.StateDiffRows
        NonzeroStateDiffRows = [int]$Validation.NonzeroStateDiffRows
        StateDiffManifestExact = [bool]$Validation.StateDiffManifestExact
        Phase17 = $recordedPhase17
        Phase17Metrics = $recordedPhase17Metrics
        Phase24 = $recordedPhase24
        Phase24Metrics = $recordedPhase24Metrics
        StagedCleanup = $recordedStagedCleanup
        FocusedCaseId = $Validation.FocusedCaseId
        FocusedCaseStatus = $Validation.FocusedCaseStatus
        FocusedAssertions = $recordedFocusedAssertions
        CorrectedCanaryAssertionManifestExact =
            [bool]$Validation.CorrectedCanaryAssertionManifestExact
        CorrectedCanaryCaseSetExact =
            [bool]$Validation.CorrectedCanaryCaseSetExact
        CorrectedCanaryWarningContractExact =
            [bool]$Validation.CorrectedCanaryWarningContractExact
        CorrectedCanaryNoBlockedAssertions =
            [bool]$Validation.CorrectedCanaryNoBlockedAssertions
        CorrectedCanaryOrphanContractExact =
            [bool]$Validation.CorrectedCanaryOrphanContractExact
        IntentionalMissionConvoyAdmissionDiagnosticsProven =
            [bool]$Validation.IntentionalMissionConvoyAdmissionDiagnosticsProven
        IntentionalMissionConvoySettlementDiagnosticProven =
            [bool]$Validation.IntentionalMissionConvoySettlementDiagnosticProven
        IntentionalMissionConvoyCorruptionDiagnosticsProven =
            [bool]$Validation.IntentionalMissionConvoyCorruptionDiagnosticsProven
        IntentionalMissionConvoyWatchdogDiagnosticProven =
            [bool]$Validation.IntentionalMissionConvoyWatchdogDiagnosticProven
        FinalOrphanCleanupPass = [bool]$Validation.FinalOrphanCleanupPass
        FinalOrphanActiveGroups = [string]$Validation.FinalOrphanActiveGroups
    }
}

function Get-RegisteredFixtureRootContract {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet('fixture', 'transition', 'portable')]
        [string]$Kind
    )

    $parent = if ($Kind -ceq 'portable') {
        [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    }
    else {
        [IO.Path]::GetFullPath($PSScriptRoot)
    }
    $parent = $parent.TrimEnd(
        [IO.Path]::DirectorySeparatorChar,
        [IO.Path]::AltDirectorySeparatorChar)

    switch ($Kind) {
        'fixture' {
            $prefix = '.ri-'
            $leafPattern = '^\.ri-[0-9a-f]{12}$'
        }
        'transition' {
            $prefix = '.ri-transition-'
            $leafPattern = '^\.ri-transition-[0-9a-f]{12}$'
        }
        'portable' {
            $prefix = 'partisan-ri-'
            $leafPattern = '^partisan-ri-[0-9a-f]{12}$'
        }
    }

    return [PSCustomObject]@{
        Kind = $Kind
        Parent = $parent
        Prefix = $prefix
        LeafPattern = $leafPattern
    }
}

function Assert-RegisteredFixtureRootTopology {
    param(
        [Parameter(Mandatory = $true)]
        $Record
    )

    $contract = Get-RegisteredFixtureRootContract -Kind ([string]$Record.Kind)
    $rootPath = [IO.Path]::GetFullPath([string]$Record.Path)
    $rootLeaf = [IO.Path]::GetFileName($rootPath)
    $rootParent = [IO.Path]::GetDirectoryName($rootPath)
    if (-not $rootParent.Equals(
            [string]$contract.Parent,
            [StringComparison]::OrdinalIgnoreCase) -or
        $rootLeaf -cnotmatch [string]$contract.LeafPattern -or
        $rootLeaf -cne [string]$Record.Leaf) {
        throw 'A registered release-index self-test root escaped its exact contract.'
    }

    $parentItem = Get-Item -LiteralPath $contract.Parent -Force -ErrorAction Stop
    if (-not $parentItem.PSIsContainer -or
        ($parentItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'A release-index self-test root parent is not a plain directory.'
    }

    $pending = New-Object Collections.Generic.Stack[string]
    $pending.Push($rootPath)
    while ($pending.Count -gt 0) {
        $currentPath = $pending.Pop()
        $currentItem = Get-Item -LiteralPath $currentPath -Force -ErrorAction Stop
        if (($currentItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'A registered release-index self-test root contains a reparse point.'
        }
        if (-not $currentItem.PSIsContainer) {
            continue
        }
        foreach ($childItem in @(Get-ChildItem -LiteralPath $currentPath `
                -Force -ErrorAction Stop)) {
            if (($childItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw 'A registered release-index self-test root contains a reparse point.'
            }
            if ($childItem.PSIsContainer) {
                $pending.Push([string]$childItem.FullName)
            }
        }
    }
}

function Remove-RegisteredFixtureRoot {
    param(
        [Parameter(Mandatory = $true)]
        $Record,
        [switch]$BestEffort
    )

    try {
        if (-not [bool]$Record.Created -or [bool]$Record.Removed) {
            return $null
        }
        if (-not (Test-Path -LiteralPath $Record.Path -PathType Any)) {
            $Record.Removed = $true
            return $null
        }

        Assert-RegisteredFixtureRootTopology -Record $Record
        $children = @(Get-ChildItem -LiteralPath $Record.Path -Force -ErrorAction Stop)
        if (-not [bool]$Record.MarkerCreated) {
            if ($children.Count -ne 0) {
                throw 'An unmarked release-index self-test root is not empty.'
            }
        }
        else {
            $markerRows = @($children | Where-Object {
                $_.Name -ceq $fixtureOwnerMarkerName
            })
            if ($markerRows.Count -ne 1 -or $markerRows[0].PSIsContainer -or
                ($markerRows[0].Attributes -band
                    [IO.FileAttributes]::ReparsePoint) -ne 0 -or
                [IO.Path]::GetFullPath([string]$markerRows[0].FullName) -cne
                    [IO.Path]::GetFullPath([string]$Record.MarkerPath) -or
                [IO.File]::ReadAllText([string]$Record.MarkerPath) -cne
                    [string]$Record.MarkerText) {
                throw 'A release-index self-test root owner marker is invalid.'
            }
        }

        Remove-Item -LiteralPath $Record.Path -Recurse -Force -ErrorAction Stop
        if (Test-Path -LiteralPath $Record.Path -PathType Any) {
            throw 'A registered release-index self-test root cleanup did not converge.'
        }
        $Record.Removed = $true
        return $null
    }
    catch {
        if ($BestEffort) {
            return ('{0} cleanup failed: {1}' -f
                [string]$Record.Kind, $_.Exception.Message)
        }
        throw
    }
}

function New-RegisteredFixtureRoot {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet('fixture', 'transition', 'portable')]
        [string]$Kind
    )

    $contract = Get-RegisteredFixtureRootContract -Kind $Kind
    for ($attempt = 0; $attempt -lt 32; $attempt++) {
        $token = [Guid]::NewGuid().ToString('N').Substring(0, 12)
        $leaf = [string]$contract.Prefix + $token
        $rootPath = [IO.Path]::GetFullPath((Join-Path $contract.Parent $leaf))
        if (-not [IO.Path]::GetDirectoryName($rootPath).Equals(
                [string]$contract.Parent,
                [StringComparison]::OrdinalIgnoreCase) -or
            [IO.Path]::GetFileName($rootPath) -cne $leaf -or
            $leaf -cnotmatch [string]$contract.LeafPattern) {
            throw 'A release-index self-test root candidate escaped its exact contract.'
        }
        if (Test-Path -LiteralPath $rootPath -PathType Any) {
            continue
        }

        $markerPath = Join-Path $rootPath $fixtureOwnerMarkerName
        $markerText = (@(
                'partisan-release-index-selftest-owner-v1',
                "processId=$fixtureOwnerProcessId",
                "processStartUtc=$fixtureOwnerProcessStartUtc",
                "kind=$Kind",
                "token=$token") -join "`n") + "`n"
        $record = [PSCustomObject]@{
            Kind = $Kind
            Path = $rootPath
            Parent = [string]$contract.Parent
            Leaf = $leaf
            Token = $token
            MarkerPath = $markerPath
            MarkerText = $markerText
            Created = $false
            MarkerCreated = $false
            Removed = $false
        }
        [void]$fixtureCleanupRegistry.Add($record)

        try {
            $rootItem = New-Item -ItemType Directory -Path $rootPath `
                -ErrorAction Stop
            $record.Created = $true
            if (-not $rootItem.PSIsContainer -or
                ($rootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw 'A release-index self-test root was not created as a plain directory.'
            }

            $markerBytes = (New-Object Text.UTF8Encoding($false)).GetBytes($markerText)
            $markerStream = [IO.File]::Open(
                $markerPath,
                [IO.FileMode]::CreateNew,
                [IO.FileAccess]::Write,
                [IO.FileShare]::None)
            try {
                $markerStream.Write($markerBytes, 0, $markerBytes.Length)
                $markerStream.Flush()
            }
            finally {
                $markerStream.Dispose()
            }
            $record.MarkerCreated = $true
            Assert-RegisteredFixtureRootTopology -Record $record
            return $record
        }
        catch {
            $primaryFailure = $_
            $cleanupFailure = Remove-RegisteredFixtureRoot `
                -Record $record -BestEffort
            if ($cleanupFailure) {
                $primaryFailure.Exception.Data['FixtureCleanupFailure'] =
                    [string]$cleanupFailure
            }
            throw $primaryFailure
        }
    }

    throw 'The release-index self-test could not allocate a unique fixture root.'
}

function Get-FixtureRootResidueSnapshot {
    $rows = New-Object Collections.Generic.List[string]
    foreach ($kind in @('fixture', 'transition', 'portable')) {
        $contract = Get-RegisteredFixtureRootContract -Kind $kind
        foreach ($item in @(Get-ChildItem -LiteralPath $contract.Parent `
                -Force -ErrorAction Stop)) {
            if ($item.PSIsContainer -and
                $item.Name -cmatch [string]$contract.LeafPattern) {
                [void]$rows.Add([IO.Path]::GetFullPath([string]$item.FullName))
            }
        }
    }
    return @($rows.ToArray() | Sort-Object -Unique)
}

function New-Fixture {
    param(
        [string]$Name,
        [ValidateSet(
            'full', 'internal', 'warn', 'unsupported-skip',
            'cert-red', 'unknown-blocker', 'state-diff-red', 'orphan-red',
            'runtime-red', 'outcome-error-red', 'raw-diagnostic-red', 'diagnostic-red',
            'mixed-blocked-fail', 'mixed-skipped-warn', 'build-provenance-red')]
        [string]$Mode = 'full',
        [string]$DiagnosticAxis = ''
    )

    $fixtureRootRecord = New-RegisteredFixtureRoot -Kind 'fixture'
    $fixtureParent = [string]$fixtureRootRecord.Path
    $leaf = '20260719T120000Z-' + [Guid]::NewGuid().ToString('N')
    $bundle = Join-Path $fixtureParent $leaf
    try {
        New-Item -ItemType Directory -Path $bundle -ErrorAction Stop | Out-Null

    $runId = 'seed1985_t0_p1_u1784500000'
    $artifactName = "HST_CampaignDebug_$runId.json"
    $diffName = "HST_CampaignDebug_${runId}_state_diff.txt"
    $summaryName = "HST_CampaignDebug_${runId}_summary.txt"
    $phase17AssertionIds = @(
        'phase17.counterattack.native_projection.baseline',
        'phase17.counterattack.native_projection.materializing',
        'phase17.counterattack.native_projection.physical',
        'phase17.counterattack.native_projection.fold',
        'phase17.counterattack.native_projection.continuity',
        'phase17.counterattack.native_projection.clock_isolation',
        'phase17.counterattack.native_projection.native_casualty',
        'phase17.counterattack.native_projection.casualty_fold',
        'phase17.counterattack.native_projection.casualty_reentry',
        'phase17.counterattack.native_projection.casualty_replay',
        'phase17.counterattack.native_projection.casualty_continuity')
    $phase17Assertions = @($phase17AssertionIds | ForEach-Object { New-Assertion $_ })
    $phase17Metrics = @()
    foreach ($suffix in @(
            'spawn_ticks', 'spawn_tick_limit', 'spawn_deferred_ticks',
            'physical_settle_ticks', 'casualty_reentry_physical_settle_ticks',
            'survivor_reentry_physical_settle_ticks', 'physical_settle_limit',
            'casualty_settle_ticks', 'casualty_settle_limit', 'elapsed_peak',
            'expected_living')) {
        $phase17Metrics += New-Metric "phase17.counterattack.native_projection.$suffix" '1'
    }
    $phase24AuthorityActual =
        'orders/open/terminal/invalid 3/1/2/0 | projections 1 | V/M/P/D 0/1/0/0 | support leaks 0'
    $phase24Assertions = @(
        (New-Assertion 'phase24.escalation.runtime_owner_classification' 'PASS' $true `
            'CONTROLLED_RUNTIME' 'synthetic_runtime_probe' 'typed runtime proof' `
            'synthetic failure reason' 'expected runtime owner classification' `
            'expected/classified/invalid 1/1/0'),
        (New-Assertion 'phase24.escalation.exact_counterattack_authority' 'PASS' $true `
            'CONTROLLED_RUNTIME' 'synthetic_runtime_probe' 'typed runtime proof' `
            'synthetic failure reason' 'expected exact authority' $phase24AuthorityActual),
        (New-Assertion 'phase24.escalation.support_physicalization' 'SKIPPED' $false),
        (New-Assertion 'phase24.escalation.group_physicalization' 'SKIPPED' $false))
    $phase24Metrics = @(
        (New-Metric 'phase24.escalation.runtime_owner_expected' '1'),
        (New-Metric 'phase24.escalation.runtime_owner_classified' '1'),
        (New-Metric 'phase24.escalation.runtime_owner_snapshot_invariant_failures' '0'),
        (New-Metric 'phase24.escalation.exact_counterattack_orders' '3'),
        (New-Metric 'phase24.escalation.exact_counterattack_open_orders' '1'),
        (New-Metric 'phase24.escalation.exact_counterattack_terminal_ledgers' '2'),
        (New-Metric 'phase24.escalation.exact_counterattack_invalid_authority' '0'),
        (New-Metric 'phase24.escalation.exact_counterattack_projection_groups' '1'),
        (New-Metric 'phase24.escalation.exact_counterattack_virtual_groups' '0'),
        (New-Metric 'phase24.escalation.exact_counterattack_materializing_groups' '1'),
        (New-Metric 'phase24.escalation.exact_counterattack_physical_groups' '0'),
        (New-Metric 'phase24.escalation.exact_counterattack_dematerializing_groups' '0'),
        (New-Metric 'phase24.escalation.exact_counterattack_support_leaks' '0'))
    $cases = @(
        (New-Case 'foundation.synthetic' 'PASS' @(
            (New-Assertion 'foundation.synthetic.pass'))),
        (New-Case 'phase24.phase24_escalation_pressure' 'SKIPPED' `
            $phase24Assertions 'legacy' 'escalation' 'phase24' $phase24Metrics),
        (New-Case 'phase25.manual_external_gaps' 'PASS' @(
            (New-Assertion 'phase25.external_gate_catalog')) `
            'soak' 'external_harness' 'final'),
        (New-Case 'observation.final_report' 'PASS' @(
            (New-Assertion 'observation.final_report.complete'))),
        (New-Case 'phase24.phase24_final_report' 'PASS' @(
            (New-Assertion 'phase24.final_report.complete'))),
        (New-Case 'cleanup.enemy_orders.run_completion' 'PASS' @(
            (New-Assertion 'cleanup.enemy_orders.complete'))),
        (New-Case 'cleanup.player_marker_completion' 'PASS' @(
            (New-Assertion 'cleanup.player_markers.complete'))),
        (New-Case 'phase17.phase17_report' 'PASS' $phase17Assertions `
            'phase_smoke' 'counterattack' 'phase17' $phase17Metrics),
        (New-Case 'cleanup.run_leak_snapshot' 'PASS' @(
            (New-Assertion 'cleanup.orphan_active_groups')) `
            'cleanup' 'campaign_debug' 'final' @(
                (New-Metric 'cleanup.orphan_active_groups' '0'))),
        (New-Case 'cleanup.state_isolation_restore' 'PASS' @(
            (New-Assertion 'isolation.snapshot'),
            (New-Assertion 'isolation.state_restore'),
            (New-Assertion 'isolation.persistence_restore')) `
            'cleanup' 'campaign_debug' 'state_restore'),
        (New-Case 'early_mechanics.force_authority' 'PASS' @(
            (New-Assertion 'mission_convoy.admission'),
            (New-Assertion 'mission_convoy.admission_rollback'),
            (New-Assertion 'mission_convoy.settlement'),
            (New-Assertion 'mission_convoy.corruption_rejection'),
            (New-Assertion 'mission_convoy.watchdog'))))
    $stagedGrace = [ordered]@{
        'partial-cancel_start' = 0
        'partial-cancel_transition' = 1
        'success_member_transition' = 1
        'success_and_failure_fixture_transition' = 0
        'failure_member_transition' = 1
        'same-wave_failure_capture' = 1
    }
    foreach ($stagedEntry in $stagedGrace.GetEnumerator()) {
        $stagedActual = 'runtime groups 1 | checked 1 | mismatches 0 | ' +
            "exact staged zero-member grace $($stagedEntry.Value) | first none"
        $cases += New-Case `
            "post_case_cleanup.action_mechanic_exact_spawn_adapter_$($stagedEntry.Key)" `
            'PASS' @(
                (New-Assertion 'post_cleanup.active_groups'),
                (New-Assertion 'post_cleanup.runtime_factions' 'PASS' $true `
                    'CONTROLLED_RUNTIME' 'synthetic_runtime_probe' `
                    'typed runtime proof' 'synthetic failure reason' `
                    'expected runtime factions' $stagedActual),
                (New-Assertion 'post_cleanup.runtime_group_population_settled')) `
            'cleanup' 'exact_spawn_adapter' 'post_case' @(
                (New-Metric 'post_cleanup.orphan_active_groups' '0'),
                (New-Metric 'post_cleanup.runtime_faction_mismatches' '0'),
                (New-Metric 'post_cleanup.runtime_faction_zero_member_grace_candidates' '1'),
                (New-Metric 'post_cleanup.runtime_pending_population_groups' '0'))
    }
    if ($Mode -ceq 'internal') {
        $isolationCase = @($cases | Where-Object m_sCaseId -CEQ `
            'cleanup.state_isolation_restore')[0]
        $isolationCase.m_sStatus = 'WARN'
        $isolationCase.m_aAssertions += New-Assertion 'isolation.world_scope' 'WARN' $false `
                'EXTERNAL_PROCESS' 'manual_external_gap' `
                'external process restart, reconnect, or long-soak harness' `
                'restart the disposable development session before another certification run' `
                'runtime certification remains scoped to the disposable development session' `
                'world runtime, player inventory, health, and service caches require session restart before another certifying run'
        $cases += New-Case 'persistence.seeded_roundtrip.phase12' 'WARN' @(
            (New-Assertion 'persistence.real_restart' 'WARN' $false `
                'EXTERNAL_PROCESS' 'manual_external_gap' `
                'external process restart, reconnect, or long-soak harness' `
                'run the immutable package through the external restart matrix before claiming restart certification' `
                'external process restart / reconnect remains an explicit later-gate scenario' `
                'non-certifying external advisory | restart/fault gate')) `
            'persistence' 'persistence_smoke' 'early_phase'
        $phase25Case = @($cases | Where-Object m_sCaseId -CEQ `
            'phase25.manual_external_gaps')[0]
        $phase25Case.m_sStatus = 'WARN'
        $phase25Case.m_aAssertions = @(
            (New-Assertion 'phase25.real_restart' 'WARN' $false `
                'EXTERNAL_PROCESS' 'manual_external_gap' `
                'external process restart, reconnect, or long-soak harness' `
                'run the immutable package through the external restart matrix before claiming restart certification' `
                'real restart-after-primitive remains an explicit later-gate external scenario' `
                'non-certifying external advisory | restart/fault gate'),
            (New-Assertion 'phase25.second_client' 'WARN' $false `
                'EXTERNAL_PROCESS' 'manual_external_gap' `
                'external process restart, reconnect, or long-soak harness' `
                'run the immutable package with the required clients before claiming multiplayer certification' `
                'second-client join/reconnect remains an explicit later-gate external scenario' `
                'non-certifying external advisory | multiplayer/JIP gate'),
            (New-Assertion 'phase25.two_hour_soak' 'WARN' $false `
                'EXTERNAL_PROCESS' 'manual_external_gap' `
                'external process restart, reconnect, or long-soak harness' `
                'run the immutable package for the required duration before claiming soak certification' `
                'two-hour endurance remains an explicit later-gate external scenario' `
                'non-certifying external advisory | soak gate'))
    }
    elseif ($Mode -ceq 'warn') {
        $cases += New-Case 'warning.synthetic' 'WARN' @(
            (New-Assertion 'warning.synthetic.assertion' 'WARN' $false))
    }
    elseif ($Mode -ceq 'unsupported-skip') {
        $cases += New-Case 'skip.synthetic' 'SKIPPED' @(
            (New-Assertion 'skip.synthetic.unsupported' 'SKIPPED' $false))
    }
    elseif ($Mode -ceq 'cert-red') {
        $cases[0].m_sStatus = 'FAIL'
        $cases[0].m_aAssertions[0].m_sStatus = 'FAIL'
    }
    elseif ($Mode -ceq 'orphan-red') {
        $orphanCase = @($cases | Where-Object m_sCaseId -CEQ `
            'cleanup.run_leak_snapshot')[0]
        $orphanCase.m_sStatus = 'FAIL'
        $orphanCase.m_aAssertions[0].m_sStatus = 'FAIL'
        $orphanCase.m_aMetrics[0].m_sValue = '1'
    }
    elseif ($Mode -ceq 'unknown-blocker') {
        $cases += New-Case 'unknown-blocker.synthetic' 'BLOCKED' @(
            (New-Assertion 'unknown-blocker.synthetic.assertion' 'BLOCKED' $false))
    }
    elseif ($Mode -ceq 'mixed-blocked-fail') {
        $cases += New-Case 'mixed-blocked-fail.synthetic' 'FAIL' @(
            (New-Assertion 'mixed-blocked-fail.synthetic.blocked' 'BLOCKED' $false),
            (New-Assertion 'mixed-blocked-fail.synthetic.fail' 'FAIL' $false))
    }
    elseif ($Mode -ceq 'mixed-skipped-warn') {
        $cases[1].m_sStatus = 'WARN'
        $cases[1].m_aAssertions +=
            New-Assertion 'mixed-skipped-warn.synthetic.warn' 'WARN' $false
    }

    $raw = [PSCustomObject][ordered]@{
        m_sRunId = $runId
        m_sProfile = 'full_certification'
        m_sCampaignSeed = '1985'
        m_sPlayerIdentityId = 'synthetic-admin'
        m_sWorldName = 'HST_Dev'
        m_sBuildSha = $embeddedBuildSha
        m_sBuildUtc = $embeddedBuildUtc
        m_sBuildLabel = $embeddedBuildLabel
        m_sMarkerPrefix = "hst_debug_$runId"
        m_sMissionPrefix = "hst_debug_${runId}_mission_"
        m_sEntityTag = 'HST_CAMPAIGN_DEBUG'
        m_iStartedAtSecond = 0
        m_iEndedAtSecond = 1
        m_iPassCount = 0
        m_iWarnCount = 0
        m_iFailCount = 0
        m_iBlockedCount = 0
        m_iSkippedCount = 0
        m_iCertificationRequiredCount = 0
        m_iCertificationProvenCount = 0
        m_iCertificationFailCount = 0
        m_iCertificationBlockedCount = 0
        m_iCertificationWarnCount = 0
        m_bCertificationPassed = $false
        m_aCases = $cases
        m_aMetrics = @(
            (New-Metric 'run.build.sha' $embeddedBuildSha 'sha' 'campaign_debug' 'run'),
            (New-Metric 'run.build.utc' $embeddedBuildUtc 'utc' 'campaign_debug' 'run'),
            (New-Metric 'run.build.label' $embeddedBuildLabel 'label' 'campaign_debug' 'run'),
            (New-Metric 'run.trigger' 'cli_autostart' 'source' 'campaign_debug' 'run'))
        m_aArtifacts = @($artifactName, $summaryName, $diffName)
    }
    Set-RawCounts $raw

    $artifactRelative = "raw/campaign-debug/$artifactName"
    $diffRelative = "raw/campaign-debug/$diffName"
    $textSummaryRelative = "raw/campaign-debug/$summaryName"
    Write-Json (Join-Path $bundle $artifactRelative) $raw
    $stateDiffDelta = if ($Mode -ceq 'state-diff-red') { 1 } else { 0 }
    $stateDiffLabels = @(Get-ExactCampaignDebugStateDiffLabels)
    if ($stateDiffLabels.Count -ne 18) {
        throw 'Synthetic state-diff fixture did not resolve 18 canonical labels.'
    }
    $stateDiffLines = New-Object Collections.Generic.List[string]
    [void]$stateDiffLines.Add('Partisan campaign debug state diff')
    [void]$stateDiffLines.Add("run $runId")
    for ($stateDiffIndex = 0; $stateDiffIndex -lt 18; $stateDiffIndex++) {
        $delta = if ($stateDiffIndex -eq 0) { $stateDiffDelta } else { 0 }
        [void]$stateDiffLines.Add(
            "$($stateDiffLabels[$stateDiffIndex]) $stateDiffIndex -> " +
            "$($stateDiffIndex + $delta) | delta $delta")
    }
    Write-Utf8Text (Join-Path $bundle $diffRelative) `
        (($stateDiffLines.ToArray() -join "`n") + "`n")
    Write-Utf8Text (Join-Path $bundle $textSummaryRelative) `
        ("Partisan campaign debug complete`nrun $runId`nprofile full_certification`n" +
            "build source $embeddedBuildSha | UTC $embeddedBuildUtc | label $embeddedBuildLabel`n")
    Write-Utf8Text (Join-Path $bundle 'config/HST_Settings.json') `
        "{`"schemaVersion`":$runtimeSettingsSchema}`n"
    Write-Json (Join-Path $bundle 'identity/candidate.json') $syntheticCandidateManifest
    $manifestSha = (Get-FileHash `
        -LiteralPath (Join-Path $bundle 'identity/candidate.json') `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    $syntheticReadySeal = [PSCustomObject][ordered]@{
        schemaVersion = 1
        candidateId = $candidateId
        gitHead = $candidateHead
        packageSha256 = $packageSha
        manifestSha256 = $manifestSha
    }
    Write-Json (Join-Path $bundle 'identity/candidate.ready.json') $syntheticReadySeal
    $acceptedDiagnosticLogText = Get-SyntheticAcceptedDiagnosticLogText
    $auxiliaryDiagnosticLogText = Get-SyntheticAcceptedDiagnosticLogText -StockOnly
    $diagnosticLogText = $acceptedDiagnosticLogText
    if ($Mode -ceq 'raw-diagnostic-red') {
        $diagnosticLogText +=
            "17:00:04.000 SCRIPT (E): Partisan synthetic unapproved diagnostic`n"
    }
    Write-Utf8Text (Join-Path $bundle 'raw/logs/logs_synthetic/script.log') `
        $diagnosticLogText
    Write-Utf8Text (Join-Path $bundle 'raw/logs/logs_synthetic/console.log') `
        $diagnosticLogText
    Write-Utf8Text (Join-Path $bundle 'raw/logs/logs_synthetic/error.log') `
        $auxiliaryDiagnosticLogText
    Write-Utf8Text (Join-Path $bundle 'raw/logs/logs_synthetic/crash.log') `
        $auxiliaryDiagnosticLogText

    $readySha = (Get-FileHash `
        -LiteralPath (Join-Path $bundle 'identity/candidate.ready.json') `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    $settingsSha = (Get-FileHash `
        -LiteralPath (Join-Path $bundle 'config/HST_Settings.json') `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    $candidateIdentity.ManifestSha256 = $manifestSha
    $candidateIdentity.ReadySha256 = $readySha

    $fileRows = @()
    foreach ($file in @(Get-ChildItem -LiteralPath $bundle -Recurse -File -Force |
            Sort-Object FullName)) {
        $fileRows += [PSCustomObject][ordered]@{
            path = $file.FullName.Substring($bundle.Length + 1).Replace('\', '/')
            length = [long]$file.Length
            sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }

    $semanticValidation = Invoke-BoundRunnerSemanticValidation `
        -RunnerSourceText (Get-Content -Raw -LiteralPath `
            (Join-Path $PSScriptRoot 'run-guarded-campaign-debug.ps1')) `
        -JsonPath (Join-Path $bundle $artifactRelative) `
        -SummaryPath (Join-Path $bundle $textSummaryRelative) `
        -StateDiffPath (Join-Path $bundle $diffRelative) `
        -GuardRoot (Join-Path $bundle 'raw') `
        -ExpectedSha $embeddedBuildSha `
        -ExpectedUtc $embeddedBuildUtc `
        -ExpectedLabel $embeddedBuildLabel
    $classifierChecks = [int]$semanticValidation.ClassifierSelfTestCount
    if ($classifierChecks -le 0) {
        throw 'Synthetic fixture did not resolve the immutable runner classifier count.'
    }
    $derivedValidation = $semanticValidation.ArtifactValidation
    $validation = ConvertTo-RecordedValidationSummary `
        -Validation $derivedValidation
    $buildProvenanceProperty =
        $validation.PSObject.Properties['BuildProvenanceMatches']
    if ($null -eq $buildProvenanceProperty -or
        $buildProvenanceProperty.Value -isnot [bool] -or
        -not [bool]$buildProvenanceProperty.Value) {
        throw 'Synthetic recorded validation did not derive exact build provenance.'
    }
    foreach ($rawBuildProperty in @('BuildSha', 'BuildUtc', 'BuildLabel')) {
        if ($null -ne $validation.PSObject.Properties[$rawBuildProperty]) {
            throw 'Synthetic recorded validation retained a raw build identity.'
        }
    }
    foreach ($row in @($validation.Phase17)) {
        Assert-ExactObjectProperties `
            $row @('Id', 'Pass') `
            'Synthetic recorded validation Phase17 row'
    }
    foreach ($row in @($validation.Phase24)) {
        Assert-ExactObjectProperties `
            $row @('Id', 'Pass', 'Accepted', 'Status', 'Actual') `
            'Synthetic recorded validation Phase24 row'
    }
    Assert-ExactObjectProperties `
        $validation.Phase17Metrics @(
            'spawn_ticks', 'spawn_tick_limit', 'spawn_deferred_ticks',
            'physical_settle_ticks', 'casualty_reentry_physical_settle_ticks',
            'survivor_reentry_physical_settle_ticks', 'physical_settle_limit',
            'casualty_settle_ticks', 'casualty_settle_limit', 'elapsed_peak',
            'expected_living') `
        'Synthetic recorded validation Phase17 metrics'
    Assert-ExactObjectProperties `
        $validation.Phase24Metrics @(
            'runtime_owner_expected', 'runtime_owner_classified',
            'runtime_owner_snapshot_invariant_failures',
            'exact_counterattack_orders', 'exact_counterattack_open_orders',
            'exact_counterattack_terminal_ledgers',
            'exact_counterattack_invalid_authority',
            'exact_counterattack_projection_groups',
            'exact_counterattack_virtual_groups',
            'exact_counterattack_materializing_groups',
            'exact_counterattack_physical_groups',
            'exact_counterattack_dematerializing_groups',
            'exact_counterattack_support_leaks',
            'exact_counterattack_invalid_authority_rows') `
        'Synthetic recorded validation Phase24 metrics'
    foreach ($row in @($validation.StagedCleanup)) {
        Assert-ExactObjectProperties `
            $row @(
                'Id', 'Pass', 'CaseStatus', 'ActiveGroupsStatus',
                'RuntimeFactionsStatus', 'RuntimeGroupPopulationSettledStatus',
                'ExpectedZeroMemberGraceApplied', 'OrphanActiveGroups',
                'RuntimeFactionMismatches', 'ZeroMemberGraceCandidates',
                'PendingPopulationGroups') `
            'Synthetic recorded validation staged-cleanup row'
    }
    foreach ($row in @($validation.FocusedAssertions)) {
        Assert-ExactObjectProperties `
            $row @('Id', 'Pass', 'Status', 'Actual') `
            'Synthetic recorded validation focused-assertion row'
    }
    if ($Mode -ceq 'build-provenance-red') {
        $validation.BuildProvenanceMatches = $false
    }
    $errorCensus = $semanticValidation.ErrorCensus
    if ($Mode -ceq 'diagnostic-red') {
        switch ($DiagnosticAxis) {
            'valid' { $errorCensus.Valid = $false }
            'channel-flag' { $errorCensus.ChannelArithmeticValid = $false }
            'channel-arithmetic' {
                $errorCensus.ScriptErrors = 14
            }
            'channel-composition' {
                $errorCensus.ScriptErrors = 14
                $errorCensus.EngineErrors = 1
            }
            'category-flag' { $errorCensus.CategoryArithmeticValid = $false }
            'category-arithmetic' {
                $errorCensus.ApprovedIntentionalDiagnosticCount = 12
            }
            'category-composition' {
                $errorCensus.ApprovedStockDiagnosticCount = 3
                $errorCensus.ApprovedIntentionalDiagnosticCount = 12
            }
            'crash' { $errorCensus.CrashMarkers = 1 }
            'severity' { $errorCensus.PartisanSeverityLineCount = 1 }
            'malformed' { $errorCensus.MalformedHardDiagnosticCount = 1 }
            'lifecycle' { $errorCensus.LifecycleMarkersValid = $false }
            'identity' { $errorCensus.IdentityBaselinePairValid = $false }
            'fixture-structure' { $errorCensus.IntentionalFixtureStructureExact = $false }
            'fixture-set' { $errorCensus.IntentionalFixtureSetValid = $false }
            'canonical-script' { $errorCensus.CanonicalScriptLogCount = 0 }
            'canonical-console' { $errorCensus.CanonicalConsoleLogCount = 0 }
            'canonical-pair' { $errorCensus.CanonicalLogPairSameDirectory = $false }
            'hard-free' { $errorCensus.HardDiagnosticFree = $true }
            'partisan-count' { $errorCensus.PartisanErrors = 20 }
            'unapproved' {
                $errorCensus.HardDiagnosticCount = 16
                $errorCensus.ScriptErrors = 16
                $errorCensus.PartisanErrors = 14
                $errorCensus.UnapprovedHardDiagnosticCount = 1
                $errorCensus.UnapprovedHardDiagnosticKinds = @(
                    [PSCustomObject]@{ kind = 'partisan-script-error'; count = 1 })
            }
            'kind-total' {
                $errorCensus.UnapprovedHardDiagnosticKinds = @(
                    [PSCustomObject]@{ kind = 'synthetic-kind-total'; count = 1 })
            }
            'classifier' { $classifierChecks-- }
            'admission-proof' {
                $errorCensus.IntentionalMissionConvoyAdmissionDiagnosticsProven = $false
                $validation.IntentionalMissionConvoyAdmissionDiagnosticsProven = $false
            }
            'settlement-proof' {
                $errorCensus.IntentionalMissionConvoySettlementDiagnosticProven = $false
                $validation.IntentionalMissionConvoySettlementDiagnosticProven = $false
            }
            'corruption-proof' {
                $errorCensus.IntentionalMissionConvoyCorruptionDiagnosticsProven = $false
                $validation.IntentionalMissionConvoyCorruptionDiagnosticsProven = $false
            }
            'watchdog-proof' {
                $errorCensus.IntentionalMissionConvoyWatchdogDiagnosticProven = $false
                $validation.IntentionalMissionConvoyWatchdogDiagnosticProven = $false
            }
            default { throw "Unknown diagnostic-axis fixture: $DiagnosticAxis" }
        }
    }

    $cleanup = [PSCustomObject][ordered]@{
        guardRemaining = 0
        ownedProcessesRemaining = 0
        newEngineProcessesRemaining = 0
        unclaimedEngineProcessesObserved = 0
        newDefaultEntriesRemaining = 0
        modifiedDefaultFiles = 0
        deletedDefaultEntries = 0
        missingDefaultRoots = 0
        externalSpillEntriesRemaining = 0
        modifiedSpillFiles = 0
        deletedSpillEntries = 0
        missingSpillRoots = 0
        cleanupPhaseErrorCount = 0
        cleanupPhaseErrors = @()
        monitoringRootsAreDetectionOnly = $true
    }
    $run = [PSCustomObject][ordered]@{
        schemaVersion = 2
        evidenceKind = 'packaged-campaign-debug'
        startedUtc = '2026-07-19T12:00:00Z'
        completedUtc = '2026-07-19T12:00:10Z'
        candidate = [PSCustomObject][ordered]@{
            candidateId = $candidateId
            candidateVersion = $candidateVersion
            runtimeUseDisposition = 'active-runtime-candidate'
            gitHead = $candidateHead
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
            runtimeRole = 'client'
            diagnosticExecutable = $clientDiagnosticIdentity
            recordedDiagnosticExecutable = $clientDiagnosticIdentity
            recordedRuntimeExecutable = $clientRuntimeIdentity
        }
        harness = [PSCustomObject][ordered]@{
            gitHead = $harnessHead
            dirty = $false
            campaignRunnerSha256 = $runnerSha
            campaignRunnerGitBlobSha256 = $runnerGitBlobSha
            candidateModuleSha256 = $moduleSha
            candidateModuleGitBlobSha256 = $candidateModuleGitBlobSha
            releaseIndexProducerSha256 = $producerSha
            releaseIndexProducerGitBlobSha256 = $producerGitBlobSha
            releaseDocsConsumerSha256 = $consumerSha
            releaseDocsConsumerGitBlobSha256 = $consumerGitBlobSha
        }
        launch = [PSCustomObject][ordered]@{
            profile = 'full_certification'
            proofScope = 'full_certification'
            worldResource = 'Worlds/HST_Dev/HST_Dev.ent'
            stagedPackage = $true
            addonSearchRootCount = 2
            addonGuid = $addonGuid
            packageSha256 = $packageSha
            diagnosticExecutable = $clientDiagnosticIdentity
            recordedRuntimeExecutable = $clientRuntimeIdentity
        }
        outcome = [PSCustomObject][ordered]@{
            success = $Mode -cne 'runtime-red'
            armed = $true
            started = $true
            completed = $true
            candidateBoundaryVerified = $true
            mountAttestation = [PSCustomObject][ordered]@{
                Valid = $true
                RecordCount = 1
                ExactPathCount = 1
                PackedCount = 1
                InvalidModeCount = 0
                GuidExact = $true
                Packed = $true
            }
            artifactsStable = $true
            evidenceCaptured = $true
            hardDiagnosticClassifierChecks = $classifierChecks
            runtimeSeconds = 10
            error = if ($Mode -ceq 'runtime-red') {
                'Synthetic runtime outcome failed.'
            }
            elseif ($Mode -ceq 'outcome-error-red') {
                'Synthetic contradictory retained outcome error.'
            }
            else {
                ''
            }
            validation = $validation
            errorCensus = $errorCensus
        }
        settings = [PSCustomObject][ordered]@{
            schemaVersion = $runtimeSettingsSchema
            sha256 = $settingsSha
            guardedRuntimeCopy = $true
        }
        cleanup = $cleanup
        files = $fileRows
    }
    $runPath = Join-Path $bundle 'run.json'
    Write-Json $runPath $run
    $producerResult = @(& $producerPath -RunEnvelopePath $runPath)
    if ($producerResult.Count -ne 1) {
        throw "Producer returned an unexpected result count for fixture $Name."
    }
        return [PSCustomObject]@{
            Name = $Name
            Parent = $fixtureParent
            CleanupRoots = @($fixtureParent)
            Bundle = $bundle
            RunPath = $runPath
            IndexPath = Join-Path $bundle 'release-index.json'
            ProducerResult = $producerResult[0]
        }
    }
    catch {
        $primaryFailure = $_
        $cleanupFailure = Remove-RegisteredFixtureRoot `
            -Record $fixtureRootRecord -BestEffort
        if ($cleanupFailure) {
            $primaryFailure.Exception.Data['FixtureCleanupFailure'] =
                [string]$cleanupFailure
        }
        throw $primaryFailure
    }
}

function Get-EvidenceFromFixture {
    param($Fixture)

    $index = Get-Content -Raw -LiteralPath $Fixture.IndexPath | ConvertFrom-Json
    $relativeIndexPath = $Fixture.IndexPath.Substring($root.Length + 1).Replace('\', '/')
    return [PSCustomObject][ordered]@{
        status = [string]$index.result.status
        summaryPath = $relativeIndexPath
        summarySha256 = (Get-FileHash -LiteralPath $Fixture.IndexPath -Algorithm SHA256).Hash.ToLowerInvariant()
        candidateId = $candidateId
        candidateSourceHead = $candidateHead
        packageSha256 = $packageSha
        manifestSha256 = [string]$index.candidate.manifestSha256
        readySha256 = [string]$index.candidate.readySha256
        settingsSha256 = [string]$index.settings.sha256
        harnessGitHead = [string]$index.harness.gitHead
        campaignRunnerSha256 = [string]$index.harness.campaignRunnerSha256
        candidateModuleSha256 = [string]$index.harness.candidateModuleSha256
        runLeafId = [string]$index.capture.runLeafId
        runId = [string]$index.capture.runId
        startedUtc = [string]$index.capture.startedUtc
        completedUtc = [string]$index.capture.completedUtc
        runtimeSeconds = [int]$index.capture.runtimeSeconds
        wrapperCaptureSuccess = [bool]$index.result.wrapperCaptureSuccess
        runtimeOutcomeSuccess = [bool]$index.result.runtimeOutcomeSuccess
        error = [string]$index.result.error
        caseCount = [int]$index.proof.caseCount
        pass = [int]$index.proof.pass
        warn = [int]$index.proof.warn
        fail = [int]$index.proof.fail
        blocked = [int]$index.proof.blocked
        skipped = [int]$index.proof.skipped
        requiredAssertions = [int]$index.proof.certificationRequired
        provenAssertions = [int]$index.proof.certificationProven
        failedAssertions = [int]$index.proof.certificationFail
        blockedAssertions = [int]$index.proof.certificationBlocked
        certificationPassed = [bool]$index.result.certificationPassed
        artifactSchemaValidationValid = [bool]$index.result.artifactSchemaValidationValid
        candidateBoundaryVerified = [bool]$index.result.candidateBoundaryVerified
        mountPacked = [bool]$index.result.mountPacked
        artifactsStable = [bool]$index.result.artifactsStable
        diagnosticClassificationValid = [bool]$index.diagnostics.classificationValid
        hardDiagnosticFree = [bool]$index.diagnostics.hardDiagnosticFree
        hardDiagnosticCount = [int]$index.diagnostics.hardDiagnosticCount
        scriptErrors = [int]$index.diagnostics.scriptErrors
        engineErrors = [int]$index.diagnostics.engineErrors
        partisanErrors = [int]$index.diagnostics.partisanErrors
        approvedStockDiagnosticCount = [int]$index.diagnostics.approvedStockDiagnosticCount
        approvedIntentionalDiagnosticCount = [int]$index.diagnostics.approvedIntentionalDiagnosticCount
        unapprovedHardDiagnosticCount = [int]$index.diagnostics.unapprovedHardDiagnosticCount
        classifierSelfTestCount = [int]$index.diagnostics.classifierSelfTestCount
        canonicalScriptLogCount = [int]$index.diagnostics.canonicalScriptLogCount
        canonicalConsoleLogCount = [int]$index.diagnostics.canonicalConsoleLogCount
        canonicalLogPairSameDirectory = [bool]$index.diagnostics.canonicalLogPairSameDirectory
        stateDiffRows = [int]$index.proof.stateDiffRows
        nonzeroStateDiffRows = [int]$index.proof.nonzeroStateDiffRows
        finalOrphanCleanupPass = [bool]$index.proof.finalOrphanCleanupPass
        cleanupAndSpillZero = $true
        envelopeFileCount = [int]$index.integrity.envelopeFileCount
        envelopeFilesRehashed = [bool]$index.integrity.envelopeFilesRehashed
        envelopeSha256 = [string]$index.integrity.envelopeSha256
        runSummarySha256 = [string]$index.integrity.runSummarySha256
        externalRequiredAdvisoryIds = @($index.proof.externalRequiredAdvisoryIds)
        acceptanceDisposition = [string]$index.result.acceptanceDisposition
    }
}

function Update-FixtureRawForConsumerTamper {
    param(
        $Fixture,
        [scriptblock]$Mutation
    )

    $run = Get-Content -Raw -LiteralPath $Fixture.RunPath | ConvertFrom-Json
    $rawRows = @($run.files | Where-Object {
        $_.path -cmatch '^raw/campaign-debug/HST_CampaignDebug_[a-zA-Z0-9_]+\.json$'
    })
    if ($rawRows.Count -ne 1) {
        throw 'Raw-tamper helper could not resolve the raw full-profile row.'
    }
    $rawRelativePath = [string]$rawRows[0].path
    $rawPath = Join-Path $Fixture.Bundle $rawRelativePath
    $raw = Get-Content -Raw -LiteralPath $rawPath | ConvertFrom-Json
    & $Mutation $raw
    Write-Json $rawPath $raw

    $rawItem = Get-Item -LiteralPath $rawPath
    $rawSha = (Get-FileHash -LiteralPath $rawPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $rawRows[0].length = [long]$rawItem.Length
    $rawRows[0].sha256 = $rawSha
    Write-Json $Fixture.RunPath $run
    $runSha = (Get-FileHash `
        -LiteralPath $Fixture.RunPath -Algorithm SHA256).Hash.ToLowerInvariant()

    $index = Get-Content -Raw -LiteralPath $Fixture.IndexPath | ConvertFrom-Json
    $indexRawRows = @($index.source.files | Where-Object path -CEQ $rawRelativePath)
    if ($indexRawRows.Count -ne 1) {
        throw 'Raw-tamper helper could not resolve the index raw row.'
    }
    $indexRawRows[0].length = [long]$rawItem.Length
    $indexRawRows[0].sha256 = $rawSha
    $index.source.rawArtifactSha256 = $rawSha
    $index.integrity.rawArtifactSha256 = $rawSha
    $index.source.runEnvelopeSha256 = $runSha
    $index.integrity.envelopeSha256 = $runSha
    $index.integrity.runSummarySha256 = $runSha
    Write-Json $Fixture.IndexPath $index
    return Get-EvidenceFromFixture $Fixture
}

function Update-FixtureRunForConsumerTamper {
    param(
        $Fixture,
        [scriptblock]$Mutation
    )

    $run = Get-Content -Raw -LiteralPath $Fixture.RunPath | ConvertFrom-Json
    & $Mutation $run
    Write-Json $Fixture.RunPath $run
    $runSha = (Get-FileHash `
        -LiteralPath $Fixture.RunPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $index = Get-Content -Raw -LiteralPath $Fixture.IndexPath | ConvertFrom-Json
    $index.source.runEnvelopeSha256 = $runSha
    $index.integrity.envelopeSha256 = $runSha
    $index.integrity.runSummarySha256 = $runSha
    Write-Json $Fixture.IndexPath $index
    return Get-EvidenceFromFixture $Fixture
}

function Set-FixtureRawCaseStatusForConsumerTamper {
    param(
        $Fixture,
        [string]$CaseId,
        [string]$NewStatus,
        [string]$AssertionId = '',
        [string]$NewAssertionStatus = '',
        [string]$NewAssertionReason = ''
    )

    $run = Get-Content -Raw -LiteralPath $Fixture.RunPath | ConvertFrom-Json
    $rawRows = @($run.files | Where-Object {
        $_.path -cmatch '^raw/campaign-debug/HST_CampaignDebug_[a-zA-Z0-9_]+\.json$'
    })
    if ($rawRows.Count -ne 1) {
        throw 'Consumer-tamper helper could not resolve the raw full-profile row.'
    }
    $rawRelativePath = [string]$rawRows[0].path
    $rawPath = Join-Path $Fixture.Bundle $rawRelativePath
    $raw = Get-Content -Raw -LiteralPath $rawPath | ConvertFrom-Json
    $cases = @($raw.m_aCases | Where-Object m_sCaseId -CEQ $CaseId)
    if ($cases.Count -ne 1) {
        throw "Consumer-tamper helper could not resolve case $CaseId."
    }
    $cases[0].m_sStatus = $NewStatus
    if (-not [string]::IsNullOrWhiteSpace($AssertionId)) {
        $assertions = @($cases[0].m_aAssertions | Where-Object {
            $_.m_sAssertionId -ceq $AssertionId
        })
        if ($assertions.Count -ne 1) {
            throw "Consumer-tamper helper could not resolve assertion $AssertionId."
        }
        $assertions[0].m_sStatus = $NewAssertionStatus
        if (-not [string]::IsNullOrWhiteSpace($NewAssertionReason)) {
            $assertions[0].m_sFailureReason = $NewAssertionReason
        }
    }
    Write-Json $rawPath $raw
    $rawItem = Get-Item -LiteralPath $rawPath
    $rawSha = (Get-FileHash -LiteralPath $rawPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $rawRows[0].length = [long]$rawItem.Length
    $rawRows[0].sha256 = $rawSha
    Write-Json $Fixture.RunPath $run
    $runSha = (Get-FileHash `
        -LiteralPath $Fixture.RunPath -Algorithm SHA256).Hash.ToLowerInvariant()

    $index = Get-Content -Raw -LiteralPath $Fixture.IndexPath | ConvertFrom-Json
    $indexRawRows = @($index.source.files | Where-Object path -CEQ $rawRelativePath)
    if ($indexRawRows.Count -ne 1) {
        throw 'Consumer-tamper helper could not resolve the index raw row.'
    }
    $indexRawRows[0].length = [long]$rawItem.Length
    $indexRawRows[0].sha256 = $rawSha
    $index.source.rawArtifactSha256 = $rawSha
    $index.integrity.rawArtifactSha256 = $rawSha
    $index.source.runEnvelopeSha256 = $runSha
    $index.integrity.envelopeSha256 = $runSha
    $index.integrity.runSummarySha256 = $runSha
    Write-Json $Fixture.IndexPath $index
    return Get-EvidenceFromFixture $Fixture
}

function Add-FixtureDuplicateAdvisoryForConsumerTamper {
    param($Fixture)

    $run = Get-Content -Raw -LiteralPath $Fixture.RunPath | ConvertFrom-Json
    $rawRows = @($run.files | Where-Object {
        $_.path -cmatch '^raw/campaign-debug/HST_CampaignDebug_[a-zA-Z0-9_]+\.json$'
    })
    if ($rawRows.Count -ne 1) {
        throw 'Duplicate-advisory helper could not resolve the raw full-profile row.'
    }
    $rawRelativePath = [string]$rawRows[0].path
    $rawPath = Join-Path $Fixture.Bundle $rawRelativePath
    $raw = Get-Content -Raw -LiteralPath $rawPath | ConvertFrom-Json
    $cases = @($raw.m_aCases | Where-Object {
        $_.m_sCaseId -ceq 'phase25.manual_external_gaps'
    })
    if ($cases.Count -ne 1) {
        throw 'Duplicate-advisory helper could not resolve the phase25 advisory case.'
    }
    $assertions = @($cases[0].m_aAssertions | Where-Object {
        $_.m_sAssertionId -ceq 'phase25.real_restart'
    })
    if ($assertions.Count -ne 1) {
        throw 'Duplicate-advisory helper could not resolve the retained advisory.'
    }
    $duplicateAssertion = $assertions[0] | ConvertTo-Json -Depth 16 | ConvertFrom-Json
    $cases[0].m_aAssertions += $duplicateAssertion
    Write-Json $rawPath $raw

    $rawItem = Get-Item -LiteralPath $rawPath
    $rawSha = (Get-FileHash -LiteralPath $rawPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $rawRows[0].length = [long]$rawItem.Length
    $rawRows[0].sha256 = $rawSha
    Write-Json $Fixture.RunPath $run
    $runSha = (Get-FileHash `
        -LiteralPath $Fixture.RunPath -Algorithm SHA256).Hash.ToLowerInvariant()

    $index = Get-Content -Raw -LiteralPath $Fixture.IndexPath | ConvertFrom-Json
    $indexRawRows = @($index.source.files | Where-Object path -CEQ $rawRelativePath)
    if ($indexRawRows.Count -ne 1) {
        throw 'Duplicate-advisory helper could not resolve the index raw row.'
    }
    $indexRawRows[0].length = [long]$rawItem.Length
    $indexRawRows[0].sha256 = $rawSha
    $index.source.rawArtifactSha256 = $rawSha
    $index.integrity.rawArtifactSha256 = $rawSha
    $index.source.runEnvelopeSha256 = $runSha
    $index.integrity.envelopeSha256 = $runSha
    $index.integrity.runSummarySha256 = $runSha
    Write-Json $Fixture.IndexPath $index
    return Get-EvidenceFromFixture $Fixture
}

function Invoke-Consumer {
    param(
        $Fixture,
        $Evidence,
        [string]$Label = 'portable self-test',
        [switch]$ViaHistoricalDispatch,
        [string]$PortableEvidenceRoot
    )

    $candidateIdentity.ManifestSha256 = [string]$Evidence.manifestSha256
    $candidateIdentity.ReadySha256 = [string]$Evidence.readySha256
    if ($ViaHistoricalDispatch) {
        return Assert-ActiveFullCampaignDebugEvidence `
            $Evidence $candidateIdentity $Label `
            ([DateTimeOffset]'2026-07-19T13:00:00Z') 24 `
            -AllowUntrackedSummaryForSelfTest `
            -TrustedToolBindingsForSelfTest $trustedTools `
            -PortableEvidenceRoot $PortableEvidenceRoot
    }
    return Assert-PortableFullCampaignDebugEvidence `
        $Evidence $candidateIdentity $Label `
        ([DateTimeOffset]'2026-07-19T13:00:00Z') 24 `
        -AllowUntrackedSummaryForSelfTest `
        -TrustedToolBindingsForSelfTest $trustedTools `
        -PortableEvidenceRoot $PortableEvidenceRoot
}

function Assert-ThrowsLike {
    param([string]$Name, [string]$Pattern, [scriptblock]$Action)

    try {
        & $Action | Out-Null
    }
    catch {
        if ($_.Exception.Message -notmatch $Pattern) {
            throw "$Name rejected for the wrong reason: $($_.Exception.Message)"
        }
        return
    }
    throw "$Name did not reject."
}

function Remove-SyntheticFixtureReleaseIndexForIndependentProducerCase {
    param(
        [Parameter(Mandatory = $true)]$Fixture,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $toolsPath = [IO.Path]::GetFullPath($PSScriptRoot).TrimEnd(
        [IO.Path]::DirectorySeparatorChar,
        [IO.Path]::AltDirectorySeparatorChar)
    $fixtureParentPath = [IO.Path]::GetFullPath([string]$Fixture.Parent)
    $bundlePath = [IO.Path]::GetFullPath([string]$Fixture.Bundle)
    $indexPath = [IO.Path]::GetFullPath([string]$Fixture.IndexPath)
    $toolsPrefix = $toolsPath + [IO.Path]::DirectorySeparatorChar
    $fixtureParentPrefix = $fixtureParentPath + [IO.Path]::DirectorySeparatorChar
    $expectedIndexPath = [IO.Path]::GetFullPath(
        (Join-Path $bundlePath 'release-index.json'))
    if (-not $fixtureParentPath.StartsWith(
            $toolsPrefix,
            [StringComparison]::OrdinalIgnoreCase) -or
        -not [IO.Path]::GetDirectoryName($fixtureParentPath).Equals(
            $toolsPath,
            [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($fixtureParentPath) -cnotmatch '^\.ri-[0-9a-f]{12}$' -or
        -not $bundlePath.StartsWith(
            $fixtureParentPrefix,
            [StringComparison]::OrdinalIgnoreCase) -or
        -not [IO.Path]::GetDirectoryName($bundlePath).Equals(
            $fixtureParentPath,
            [StringComparison]::OrdinalIgnoreCase) -or
        -not $indexPath.Equals(
            $expectedIndexPath,
            [StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-Path -LiteralPath $indexPath -PathType Leaf)) {
        throw "$Label did not resolve one contained synthetic release index."
    }
    foreach ($directoryPath in @($fixtureParentPath, $bundlePath)) {
        $directoryItem = Get-Item -LiteralPath $directoryPath -Force -ErrorAction Stop
        if (-not $directoryItem.PSIsContainer -or
            ($directoryItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "$Label synthetic publication path must not traverse a reparse point."
        }
    }
    $indexItem = Get-Item -LiteralPath $indexPath -Force
    if (($indexItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "$Label synthetic release index must not be a reparse point."
    }
    Remove-Item -LiteralPath $indexPath -Force
    if (Test-Path -LiteralPath $indexPath) {
        throw "$Label could not clear its independent producer publication path."
    }
}

$fixtures = New-Object Collections.Generic.List[object]
$initialFixtureRootSnapshot = @(Get-FixtureRootResidueSnapshot)
$primaryFailure = $null
$cleanupFailures = New-Object Collections.Generic.List[string]
try {
    $transitionHead = (& git -C $root rev-parse HEAD).Trim()
    $transitionParent = (& git -C $root rev-parse HEAD^).Trim()
    if ($transitionHead -cnotmatch '^[0-9a-f]{40}$' -or
        $transitionParent -cnotmatch '^[0-9a-f]{40}$') {
        throw 'Release build transition self-test could not resolve its Git fixture.'
    }
    $transitionOldEmbedded = [PSCustomObject]@{
        sha = $transitionParent
        utc = '2026-07-19T10:00:00Z'
        label = 'schema71-settings24-transition-old'
    }
    $transitionCurrentEmbedded = [PSCustomObject]@{
        sha = $transitionHead
        utc = '2026-07-19T11:00:00Z'
        label = 'schema71-settings24-transition-current'
    }
    foreach ($retainedDisposition in @(
        'rejected-after-runtime',
        'supersede-before-runtime')) {
        Assert-ReleaseBuildTransition `
            -RuntimeUseDisposition $retainedDisposition `
            -ManifestEmbedded $transitionOldEmbedded `
            -CandidateSourceHead $transitionHead `
            -SourceBuildSha $transitionHead `
            -SourceBuildUtc $transitionCurrentEmbedded.utc `
            -SourceBuildLabel $transitionCurrentEmbedded.label `
            -CheckoutHead $transitionHead
        Assert-ReleaseBuildTransition `
            -RuntimeUseDisposition $retainedDisposition `
            -ManifestEmbedded $transitionOldEmbedded `
            -CandidateSourceHead $transitionParent `
            -SourceBuildSha $transitionHead `
            -SourceBuildUtc $transitionCurrentEmbedded.utc `
            -SourceBuildLabel $transitionCurrentEmbedded.label `
            -CheckoutHead $transitionHead
    }
    Assert-ReleaseBuildTransition `
        -RuntimeUseDisposition 'active-runtime-candidate' `
        -ManifestEmbedded $transitionCurrentEmbedded `
        -CandidateSourceHead $transitionHead `
        -SourceBuildSha $transitionHead `
        -SourceBuildUtc $transitionCurrentEmbedded.utc `
        -SourceBuildLabel $transitionCurrentEmbedded.label `
        -CheckoutHead $transitionHead
    foreach ($activeMismatch in @(
        ([PSCustomObject]@{
            sha = $transitionParent
            utc = $transitionCurrentEmbedded.utc
            label = $transitionCurrentEmbedded.label
        }),
        ([PSCustomObject]@{
            sha = $transitionHead
            utc = '2026-07-19T10:30:00Z'
            label = $transitionCurrentEmbedded.label
        }),
        ([PSCustomObject]@{
            sha = $transitionHead
            utc = $transitionCurrentEmbedded.utc
            label = 'schema71-settings24-transition-mismatch'
        }))) {
        Assert-ThrowsLike `
            'active runtime candidate embedded identity mismatch' `
            'must match the live embedded implementation identity' {
                Assert-ReleaseBuildTransition `
                    -RuntimeUseDisposition 'active-runtime-candidate' `
                    -ManifestEmbedded $activeMismatch `
                    -CandidateSourceHead $transitionHead `
                    -SourceBuildSha $transitionHead `
                    -SourceBuildUtc $transitionCurrentEmbedded.utc `
                    -SourceBuildLabel $transitionCurrentEmbedded.label `
                    -CheckoutHead $transitionHead
            }
    }
    Assert-ThrowsLike `
        'live embedded build ancestry inversion' `
        'live embedded implementation build is not an ancestor' {
            Assert-ReleaseBuildTransition `
                -RuntimeUseDisposition 'rejected-after-runtime' `
                -ManifestEmbedded $transitionOldEmbedded `
                -CandidateSourceHead $transitionHead `
                -SourceBuildSha $transitionHead `
                -SourceBuildUtc $transitionCurrentEmbedded.utc `
                -SourceBuildLabel $transitionCurrentEmbedded.label `
                -CheckoutHead $transitionParent
        }
    Assert-ThrowsLike `
        'candidate embedded build ancestry inversion' `
        'candidate embedded implementation build is not an ancestor' {
            Assert-ReleaseBuildTransition `
                -RuntimeUseDisposition 'rejected-after-runtime' `
                -ManifestEmbedded $transitionCurrentEmbedded `
                -CandidateSourceHead $transitionParent `
                -SourceBuildSha $transitionParent `
                -SourceBuildUtc $transitionCurrentEmbedded.utc `
                -SourceBuildLabel $transitionCurrentEmbedded.label `
                -CheckoutHead $transitionHead
        }
    Assert-ThrowsLike `
        'retained candidate live build lineage regression' `
        'did not advance from the retained candidate source HEAD' {
            Assert-ReleaseBuildTransition `
                -RuntimeUseDisposition 'rejected-after-runtime' `
                -ManifestEmbedded $transitionOldEmbedded `
                -CandidateSourceHead $transitionHead `
                -SourceBuildSha $transitionParent `
                -SourceBuildUtc $transitionCurrentEmbedded.utc `
                -SourceBuildLabel $transitionCurrentEmbedded.label `
                -CheckoutHead $transitionHead
        }
    Assert-ThrowsLike `
        'retained candidate live build UTC regression' `
        'live embedded build UTC must advance' {
            Assert-ReleaseBuildTransition `
                -RuntimeUseDisposition 'supersede-before-runtime' `
                -ManifestEmbedded $transitionOldEmbedded `
                -CandidateSourceHead $transitionParent `
                -SourceBuildSha $transitionHead `
                -SourceBuildUtc '2026-07-19T09:59:59Z' `
                -SourceBuildLabel $transitionCurrentEmbedded.label `
                -CheckoutHead $transitionHead
        }
    Assert-ThrowsLike `
        'retained candidate live build UTC equality' `
        'live embedded build UTC must advance' {
            Assert-ReleaseBuildTransition `
                -RuntimeUseDisposition 'rejected-after-runtime' `
                -ManifestEmbedded $transitionOldEmbedded `
                -CandidateSourceHead $transitionParent `
                -SourceBuildSha $transitionHead `
                -SourceBuildUtc $transitionOldEmbedded.utc `
                -SourceBuildLabel $transitionCurrentEmbedded.label `
                -CheckoutHead $transitionHead
        }

    $transitionFixtureRecord = New-RegisteredFixtureRoot -Kind 'transition'
    $transitionFixtureLeaf = [string]$transitionFixtureRecord.Leaf
    $transitionFixtureRoot = [string]$transitionFixtureRecord.Path
    [void]$fixtures.Add([PSCustomObject]@{
        Name = 'historical-retained-manifest-ancestry-tamper'
        CleanupRoots = @($transitionFixtureRoot)
    })
    $transitionManifestRelativePath =
        "tools/$transitionFixtureLeaf/candidate.json"
    $transitionManifestPath = Join-Path $transitionFixtureRoot 'candidate.json'
    $transitionReadyPath = Join-Path $transitionFixtureRoot 'candidate.ready.json'
    $transitionCandidateId = 'partisan-rc-transition-ancestry-tamper'
    $transitionPackageSha = 'a' * 64
    $transitionPackageVersion = '0.1.0-rc.transition-ancestry-tamper'
    $transitionWorkbenchCrc = '0123abcd'
    $transitionManifest = [PSCustomObject][ordered]@{
        manifestSchemaVersion = 1
        createdUtc = '2026-07-19T11:30:00Z'
        candidate = [PSCustomObject][ordered]@{
            id = $transitionCandidateId
            version = $transitionPackageVersion
            state = 'retained-uncertified'
        }
        source = [PSCustomObject][ordered]@{
            gitHead = $transitionParent
            embeddedImplementation = $transitionCurrentEmbedded
            campaignSchema = 71
            runtimeSettingsSchema = 24
        }
        workbench = [PSCustomObject][ordered]@{
            crc = $transitionWorkbenchCrc
        }
        package = [PSCustomObject][ordered]@{
            hashAlgorithm = 'sha256-manifest-v1'
            sha256 = $transitionPackageSha
        }
    }
    Write-Json $transitionManifestPath $transitionManifest
    $transitionManifestSha = (Get-FileHash `
        -LiteralPath $transitionManifestPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    Write-Json $transitionReadyPath ([PSCustomObject][ordered]@{
        schemaVersion = 1
        candidateId = $transitionCandidateId
        gitHead = $transitionParent
        packageSha256 = $transitionPackageSha
        manifestSha256 = $transitionManifestSha
    })
    $transitionReadySha = (Get-FileHash `
        -LiteralPath $transitionReadyPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    $transitionHistoricalCandidate = [PSCustomObject][ordered]@{
        candidateId = $transitionCandidateId
        candidateSourceHead = $transitionParent
        manifestPath = $transitionManifestRelativePath
        manifestSha256 = $transitionManifestSha
        readySha256 = $transitionReadySha
        packageHashAlgorithm = 'sha256-manifest-v1'
        packageSha256 = $transitionPackageSha
        packageVersion = $transitionPackageVersion
        workbenchCrc = $transitionWorkbenchCrc
    }
    Assert-ThrowsLike `
        'historical retained manifest embedded ancestry tamper' `
        'historical retained candidate embedded implementation build is not an ancestor' {
            Get-RetainedCandidateIdentity `
                $transitionHistoricalCandidate `
                'historical retained candidate' | Out-Null
        }

    $transitionManifest.createdUtc = '2026-07-19T10:30:00Z'
    $transitionManifest.source.embeddedImplementation = [PSCustomObject]@{
        sha = $transitionParent
        utc = $transitionCurrentEmbedded.utc
        label = 'schema71-settings24-transition-future-embedded'
    }
    Write-Json $transitionManifestPath $transitionManifest
    $transitionManifestSha = (Get-FileHash `
        -LiteralPath $transitionManifestPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    Write-Json $transitionReadyPath ([PSCustomObject][ordered]@{
        schemaVersion = 1
        candidateId = $transitionCandidateId
        gitHead = $transitionParent
        packageSha256 = $transitionPackageSha
        manifestSha256 = $transitionManifestSha
    })
    $transitionReadySha = (Get-FileHash `
        -LiteralPath $transitionReadyPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    $transitionHistoricalCandidate.manifestSha256 = $transitionManifestSha
    $transitionHistoricalCandidate.readySha256 = $transitionReadySha
    Assert-ThrowsLike `
        'historical retained manifest embedded UTC after creation' `
        'embedded implementation UTC must not be later than manifest.createdUtc' {
            Get-RetainedCandidateIdentity `
                $transitionHistoricalCandidate `
                'historical retained candidate' | Out-Null
        }

    Assert-NoLocalAbsolutePathValue 'https://example.invalid/evidence' `
        'Synthetic HTTPS preservation'
    Assert-NoLocalAbsolutePathValue $syntheticUpperHttps `
        'Synthetic uppercase HTTPS preservation'

    $green = New-Fixture 'green' 'full'
    [void]$fixtures.Add($green)
    $greenEvidence = Get-EvidenceFromFixture $green
    $greenResult = Invoke-Consumer $green $greenEvidence 'synthetic accepted full profile'
    if (-not $greenResult.AcceptedFull -or $greenResult.AcceptedInternal -or $greenResult.Rejected) {
        $greenIndex = Get-Content -Raw -LiteralPath $green.IndexPath |
            ConvertFrom-Json
        throw ('Synthetic supported-skip full profile was not accepted correctly. Result: ' +
            ($greenResult | ConvertTo-Json -Depth 12 -Compress) +
            ' Diagnostics: ' +
            ($greenIndex.diagnostics | ConvertTo-Json -Depth 12 -Compress) +
            ' Proof: ' +
            ($greenIndex.proof | ConvertTo-Json -Depth 12 -Compress))
    }

    Assert-ThrowsLike `
        'recorded build-provenance tamper' `
        'recorded artifact-validation build provenance differs' {
            New-Fixture `
                'build-provenance-tamper' `
                'build-provenance-red' | Out-Null
        }

    $portableSplit = New-Fixture 'portable-split' 'full'
    [void]$fixtures.Add($portableSplit)
    $portableLeaf = Split-Path -Leaf $portableSplit.Bundle
    $portableExternalRecord = New-RegisteredFixtureRoot -Kind 'portable'
    $portableExternalParent = [string]$portableExternalRecord.Path
    $portableExternalRoot = Join-Path $portableExternalParent 'e'
    $portableExternalBundle = Join-Path $portableExternalRoot `
        "$candidateId/campaign-debug/$portableLeaf"
    New-Item -ItemType Directory -Path (Split-Path -Parent $portableExternalBundle) `
        -Force | Out-Null
    Move-Item -LiteralPath $portableSplit.Bundle -Destination $portableExternalBundle
    $portableTrackedIndex = Join-Path $portableSplit.Parent `
        "tracked/$portableLeaf/release-index.json"
    New-Item -ItemType Directory -Path (Split-Path -Parent $portableTrackedIndex) `
        -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $portableExternalBundle 'release-index.json') `
        -Destination $portableTrackedIndex
    $portableSplit.Bundle = $portableExternalBundle
    $portableSplit.RunPath = Join-Path $portableExternalBundle 'run.json'
    $portableSplit.IndexPath = $portableTrackedIndex
    $portableSplit.CleanupRoots += $portableExternalParent
    $portableSplitEvidence = Get-EvidenceFromFixture $portableSplit
    $portableSplitResult = Invoke-Consumer `
        $portableSplit $portableSplitEvidence 'synthetic split portable bundle' `
        -ViaHistoricalDispatch -PortableEvidenceRoot $portableExternalRoot
    if (-not $portableSplitResult.AcceptedFull -or $portableSplitResult.Rejected) {
        throw 'Synthetic tracked-index/external-bundle split was not accepted correctly.'
    }

    $internal = New-Fixture 'internal' 'internal'
    [void]$fixtures.Add($internal)
    $internalEvidence = Get-EvidenceFromFixture $internal
    $internalResult = Invoke-Consumer $internal $internalEvidence 'synthetic accepted internal profile'
    if (-not $internalResult.AcceptedInternal -or $internalResult.AcceptedFull -or
        $internalResult.Rejected) {
        throw 'Synthetic exact external-required profile was not accepted internally.'
    }
    Assert-EqualSet $externalAdvisoryIds $internalResult.ExternalRequiredAdvisoryIds `
        'Synthetic retained external-required advisory IDs'

    $stateDiffRed = New-Fixture 'state-diff-red' 'state-diff-red'
    [void]$fixtures.Add($stateDiffRed)
    $stateDiffRedEvidence = Get-EvidenceFromFixture $stateDiffRed
    Assert-ThrowsLike 'state-diff nonzero-delta rejection' `
        'retained state diff row 0 is not a zero-delta restoration row' {
        Invoke-Consumer `
            $stateDiffRed $stateDiffRedEvidence `
            'synthetic state-diff-red rejection' | Out-Null
    }

    foreach ($mode in @(
            'warn', 'unsupported-skip', 'cert-red', 'unknown-blocker',
            'orphan-red', 'runtime-red', 'outcome-error-red',
            'raw-diagnostic-red', 'mixed-blocked-fail', 'mixed-skipped-warn')) {
        $fixture = New-Fixture $mode $mode
        [void]$fixtures.Add($fixture)
        $evidence = Get-EvidenceFromFixture $fixture
        $result = Invoke-Consumer $fixture $evidence "synthetic $mode rejection"
        if (-not $result.Rejected -or $result.Accepted) {
            throw "Synthetic $mode fixture was not retained as generic red."
        }
    }

    foreach ($axis in @(
            'valid', 'channel-flag', 'channel-arithmetic', 'channel-composition',
            'category-flag', 'category-arithmetic', 'category-composition',
            'crash', 'severity', 'malformed', 'lifecycle', 'identity',
            'fixture-structure', 'fixture-set', 'canonical-script',
            'canonical-console', 'canonical-pair', 'hard-free', 'partisan-count',
            'unapproved', 'kind-total', 'admission-proof',
            'settlement-proof', 'corruption-proof', 'watchdog-proof')) {
        Assert-ThrowsLike "recorded diagnostic $axis tamper" `
            'recorded.*diagnostic|Recorded and re-derived' {
            New-Fixture "diagnostic-$axis" 'diagnostic-red' $axis | Out-Null
        }
    }
    $classifierFixture = New-Fixture 'diagnostic-classifier' 'diagnostic-red' 'classifier'
    [void]$fixtures.Add($classifierFixture)
    $classifierEvidence = Get-EvidenceFromFixture $classifierFixture
    Assert-ThrowsLike 'diagnostic classifier count rejection' `
        'diagnostic classifier count contradicts run.json or the retained immutable runner' {
        Invoke-Consumer `
            $classifierFixture $classifierEvidence `
            'synthetic diagnostic classifier rejection' | Out-Null
    }

    $historicalResult = Invoke-Consumer `
        ($fixtures | Where-Object { $_.Name -ceq 'cert-red' } | Select-Object -First 1) `
        (Get-EvidenceFromFixture ($fixtures | Where-Object {
            $_.Name -ceq 'cert-red'
        } | Select-Object -First 1)) `
        'synthetic historical future generic red' `
        -ViaHistoricalDispatch
    if (-not $historicalResult.Rejected) {
        throw 'Future historical generic red did not validate as a retained rejection.'
    }

    $rawTamper = New-Fixture 'raw-tamper' 'full'
    [void]$fixtures.Add($rawTamper)
    $rawTamperEvidence = Get-EvidenceFromFixture $rawTamper
    Add-Content -LiteralPath `
        (Get-ChildItem (Join-Path $rawTamper.Bundle 'raw/campaign-debug') -Filter '*.json').FullName `
        -Value 'tampered'
    Assert-ThrowsLike 'raw-file tamper' 'independent hash/length check' {
        Invoke-Consumer $rawTamper $rawTamperEvidence | Out-Null
    }

    $indexTamper = New-Fixture 'index-tamper' 'full'
    [void]$fixtures.Add($indexTamper)
    $indexTamperEvidence = Get-EvidenceFromFixture $indexTamper
    $tamperedIndex = Get-Content -Raw $indexTamper.IndexPath | ConvertFrom-Json
    $tamperedIndex.proof.pass++
    Write-Json $indexTamper.IndexPath $tamperedIndex
    $indexTamperEvidence.summarySha256 = (Get-FileHash `
        $indexTamper.IndexPath -Algorithm SHA256).Hash.ToLowerInvariant()
    Assert-ThrowsLike 'release-index count tamper' 'release-index proof pass differs from raw JSON' {
        Invoke-Consumer $indexTamper $indexTamperEvidence | Out-Null
    }

    $runnerTamper = New-Fixture 'runner-binding-tamper' 'full'
    [void]$fixtures.Add($runnerTamper)
    $tamperedRun = Get-Content -Raw $runnerTamper.RunPath | ConvertFrom-Json
    $tamperedRun.harness.campaignRunnerGitBlobSha256 = '9' * 64
    Write-Json $runnerTamper.RunPath $tamperedRun
    Remove-SyntheticFixtureReleaseIndexForIndependentProducerCase `
        $runnerTamper 'immutable runner binding tamper'
    & $producerPath -RunEnvelopePath $runnerTamper.RunPath | Out-Null
    $runnerTamperEvidence = Get-EvidenceFromFixture $runnerTamper
    Assert-ThrowsLike 'immutable runner binding tamper' `
        'trusted tool bundle|immutable harness blob' {
        Invoke-Consumer $runnerTamper $runnerTamperEvidence | Out-Null
    }

    $harnessTamper = New-Fixture 'harness-binding-tamper' 'full'
    [void]$fixtures.Add($harnessTamper)
    $tamperedHarnessRun = Get-Content -Raw $harnessTamper.RunPath | ConvertFrom-Json
    $tamperedHarnessRun.harness.gitHead = 'b' * 40
    Write-Json $harnessTamper.RunPath $tamperedHarnessRun
    Remove-SyntheticFixtureReleaseIndexForIndependentProducerCase `
        $harnessTamper 'immutable harness revision tamper'
    & $producerPath -RunEnvelopePath $harnessTamper.RunPath | Out-Null
    $harnessTamperEvidence = Get-EvidenceFromFixture $harnessTamper
    Assert-ThrowsLike 'immutable harness revision tamper' `
        'trusted tool bundle|Git HEAD differs' {
        Invoke-Consumer $harnessTamper $harnessTamperEvidence | Out-Null
    }

    $candidateVersionTamper = New-Fixture 'candidate-version-binding-tamper' 'full'
    [void]$fixtures.Add($candidateVersionTamper)
    $candidateVersionEvidence = Update-FixtureRunForConsumerTamper `
        $candidateVersionTamper {
            param($run)
            $run.candidate.candidateVersion = '0.1.0-rc.tampered'
        }
    Assert-ThrowsLike 'producer candidate-version manifest binding tamper' `
        'retained manifest identity' {
        & $producerPath -RunEnvelopePath $candidateVersionTamper.RunPath | Out-Null
    }
    Assert-ThrowsLike 'consumer candidate-version manifest binding tamper' `
        'candidateVersion differs from the retained candidate manifest' {
        Invoke-Consumer $candidateVersionTamper $candidateVersionEvidence | Out-Null
    }

    $candidateToolTamper = New-Fixture 'candidate-tool-binding-tamper' 'full'
    [void]$fixtures.Add($candidateToolTamper)
    $candidateToolEvidence = Update-FixtureRunForConsumerTamper `
        $candidateToolTamper {
            param($run)
            $run.candidate.diagnosticExecutable.sha256 = '6' * 64
        }
    Assert-ThrowsLike 'producer candidate-tool manifest binding tamper' `
        'diagnostic executable differs from the retained candidate manifest' {
        & $producerPath -RunEnvelopePath $candidateToolTamper.RunPath | Out-Null
    }
    Assert-ThrowsLike 'consumer candidate-tool manifest binding tamper' `
        'diagnostic executable differs from the retained candidate manifest' {
        Invoke-Consumer $candidateToolTamper $candidateToolEvidence | Out-Null
    }

    $advisoryTamper = New-Fixture 'advisory-tamper' 'internal'
    [void]$fixtures.Add($advisoryTamper)
    $advisoryEvidence = Get-EvidenceFromFixture $advisoryTamper
    $advisoryIndex = Get-Content -Raw $advisoryTamper.IndexPath | ConvertFrom-Json
    $advisoryIndex.proof.externalRequiredAdvisoryIds[0] = 'phase25.untrusted'
    Write-Json $advisoryTamper.IndexPath $advisoryIndex
    $advisoryEvidence.summarySha256 = (Get-FileHash `
        $advisoryTamper.IndexPath -Algorithm SHA256).Hash.ToLowerInvariant()
    Assert-ThrowsLike 'advisory ID tamper' 'external-required advisory IDs' {
        Invoke-Consumer $advisoryTamper $advisoryEvidence | Out-Null
    }

    $blockedParentTamper = New-Fixture 'blocked-parent-tamper' 'unknown-blocker'
    [void]$fixtures.Add($blockedParentTamper)
    $blockedParentEvidence = Set-FixtureRawCaseStatusForConsumerTamper `
        $blockedParentTamper 'unknown-blocker.synthetic' 'PASS'
    Assert-ThrowsLike 'blocked assertion parent tamper' `
        'status PASS differs from its assertion-derived BLOCKED disposition' {
        Invoke-Consumer $blockedParentTamper $blockedParentEvidence | Out-Null
    }

    $skippedParentTamper = New-Fixture 'skipped-parent-tamper' 'full'
    [void]$fixtures.Add($skippedParentTamper)
    $skippedParentEvidence = Set-FixtureRawCaseStatusForConsumerTamper `
        $skippedParentTamper 'phase24.phase24_escalation_pressure' 'PASS'
    Assert-ThrowsLike 'skipped assertion parent tamper' `
        'status PASS differs from its assertion-derived SKIPPED disposition' {
        Invoke-Consumer $skippedParentTamper $skippedParentEvidence | Out-Null
    }

    $failedParentTamper = New-Fixture 'failed-parent-tamper' 'full'
    [void]$fixtures.Add($failedParentTamper)
    $failedParentEvidence = Set-FixtureRawCaseStatusForConsumerTamper `
        $failedParentTamper 'foundation.synthetic' 'PASS' `
        'foundation.synthetic.pass' 'FAIL'
    Assert-ThrowsLike 'failed assertion producer aggregation tamper' `
        'status PASS differs from its assertion-derived FAIL disposition' {
        & $producerPath -RunEnvelopePath $failedParentTamper.RunPath | Out-Null
    }
    Assert-ThrowsLike 'failed assertion consumer aggregation tamper' `
        'status PASS differs from its assertion-derived FAIL disposition' {
        Invoke-Consumer $failedParentTamper $failedParentEvidence | Out-Null
    }

    $lowercaseStatusTamper = New-Fixture 'lowercase-status-tamper' 'full'
    [void]$fixtures.Add($lowercaseStatusTamper)
    $lowercaseStatusEvidence = Set-FixtureRawCaseStatusForConsumerTamper `
        $lowercaseStatusTamper 'foundation.synthetic' 'pass'
    Assert-ThrowsLike 'lowercase producer case status tamper' 'unsupported status pass' {
        & $producerPath -RunEnvelopePath $lowercaseStatusTamper.RunPath | Out-Null
    }
    Assert-ThrowsLike 'lowercase consumer case status tamper' 'unsupported status pass' {
        Invoke-Consumer $lowercaseStatusTamper $lowercaseStatusEvidence | Out-Null
    }

    $duplicateAdvisoryTamper = New-Fixture 'duplicate-advisory-tamper' 'internal'
    [void]$fixtures.Add($duplicateAdvisoryTamper)
    $duplicateAdvisoryEvidence =
        Add-FixtureDuplicateAdvisoryForConsumerTamper $duplicateAdvisoryTamper
    Assert-ThrowsLike 'duplicate external advisory tamper' `
        'duplicates assertion ID' {
        Invoke-Consumer $duplicateAdvisoryTamper $duplicateAdvisoryEvidence | Out-Null
    }

    $duplicateCaseTamper = New-Fixture 'duplicate-case-tamper' 'full'
    [void]$fixtures.Add($duplicateCaseTamper)
    $duplicateCaseEvidence = Update-FixtureRawForConsumerTamper `
        $duplicateCaseTamper {
            param($raw)
            $sourceCase = @($raw.m_aCases | Where-Object {
                $_.m_sCaseId -ceq 'foundation.synthetic'
            })[0]
            $raw.m_aCases += ($sourceCase | ConvertTo-Json -Depth 16 | ConvertFrom-Json)
            Set-RawCounts $raw
        }
    Assert-ThrowsLike 'duplicate case producer tamper' 'duplicates case ID' {
        & $producerPath -RunEnvelopePath $duplicateCaseTamper.RunPath | Out-Null
    }
    Assert-ThrowsLike 'duplicate case consumer tamper' 'duplicates case ID' {
        Invoke-Consumer $duplicateCaseTamper $duplicateCaseEvidence | Out-Null
    }

    $duplicateAssertionTamper = New-Fixture 'duplicate-assertion-tamper' 'full'
    [void]$fixtures.Add($duplicateAssertionTamper)
    $duplicateAssertionEvidence = Update-FixtureRawForConsumerTamper `
        $duplicateAssertionTamper {
            param($raw)
            $targetCase = @($raw.m_aCases | Where-Object {
                $_.m_sCaseId -ceq 'foundation.synthetic'
            })[0]
            $targetCase.m_aAssertions += ($targetCase.m_aAssertions[0] |
                ConvertTo-Json -Depth 16 | ConvertFrom-Json)
            Set-RawCounts $raw
        }
    Assert-ThrowsLike 'duplicate assertion producer tamper' 'duplicates assertion ID' {
        & $producerPath -RunEnvelopePath $duplicateAssertionTamper.RunPath | Out-Null
    }
    Assert-ThrowsLike 'duplicate assertion consumer tamper' 'duplicates assertion ID' {
        Invoke-Consumer $duplicateAssertionTamper $duplicateAssertionEvidence | Out-Null
    }

    $semanticIncompleteTamper = New-Fixture 'semantic-incomplete-tamper' 'full'
    [void]$fixtures.Add($semanticIncompleteTamper)
    $semanticIncompleteEvidence = Update-FixtureRawForConsumerTamper `
        $semanticIncompleteTamper {
            param($raw)
            $raw.m_aCases = @($raw.m_aCases | Where-Object {
                $_.m_sCaseId -cne 'phase17.phase17_report'
            })
            Set-RawCounts $raw
        }
    Assert-ThrowsLike 'semantic-incomplete producer tamper' `
        'recorded artifact-validation disposition differs from portable semantic re-derivation' {
        & $producerPath -RunEnvelopePath $semanticIncompleteTamper.RunPath | Out-Null
    }
    Assert-ThrowsLike 'semantic-incomplete consumer tamper' `
        'run validation Phase17 metrics properties differs from the source inventory' {
        Invoke-Consumer $semanticIncompleteTamper $semanticIncompleteEvidence | Out-Null
    }

    $advisoryMetadataRed = New-Fixture 'advisory-metadata-red' 'internal'
    [void]$fixtures.Add($advisoryMetadataRed)
    [void](Set-FixtureRawCaseStatusForConsumerTamper `
        $advisoryMetadataRed 'persistence.seeded_roundtrip.phase12' 'WARN' `
        'persistence.real_restart' 'WARN' 'synthetic noncanonical advisory reason')
    Remove-SyntheticFixtureReleaseIndexForIndependentProducerCase `
        $advisoryMetadataRed 'advisory metadata rejection'
    & $producerPath -RunEnvelopePath $advisoryMetadataRed.RunPath | Out-Null
    $advisoryMetadataEvidence = Get-EvidenceFromFixture $advisoryMetadataRed
    $advisoryMetadataResult = Invoke-Consumer `
        $advisoryMetadataRed $advisoryMetadataEvidence `
        'synthetic advisory metadata rejection'
    if (-not $advisoryMetadataResult.Rejected -or $advisoryMetadataResult.Accepted) {
        throw 'Synthetic noncanonical external advisory metadata was not retained as generic red.'
    }

    $candidateTamper = New-Fixture 'candidate-tamper' 'full'
    [void]$fixtures.Add($candidateTamper)
    $candidateEvidence = Get-EvidenceFromFixture $candidateTamper
    $candidateEvidence.candidateId = "$candidateId-mismatch"
    Assert-ThrowsLike 'candidate status tamper' 'candidateId differs' {
        Invoke-Consumer $candidateTamper $candidateEvidence | Out-Null
    }

    $findingTamper = New-Fixture 'finding-tamper' 'full'
    [void]$fixtures.Add($findingTamper)
    $findingEvidence = Get-EvidenceFromFixture $findingTamper
    $findingIndex = Get-Content -Raw $findingTamper.IndexPath | ConvertFrom-Json
    $findingIndex.finding.nextStep = 'Continue with a different release action.'
    Write-Json $findingTamper.IndexPath $findingIndex
    $findingEvidence.summarySha256 = (Get-FileHash `
        $findingTamper.IndexPath -Algorithm SHA256).Hash.ToLowerInvariant()
    Assert-ThrowsLike 'finding prose tamper' 'finding defect/next step' {
        Invoke-Consumer $findingTamper $findingEvidence | Out-Null
    }

    $producerPathTamper = New-Fixture 'producer-unc-path-tamper' 'full'
    [void]$fixtures.Add($producerPathTamper)
    $producerPathRun = Get-Content -Raw $producerPathTamper.RunPath | ConvertFrom-Json
    $producerPathRun.outcome.error = $syntheticUncPath
    Write-Json $producerPathTamper.RunPath $producerPathRun
    Assert-ThrowsLike 'producer UNC path tamper' 'local or rooted absolute path' {
        & $producerPath -RunEnvelopePath $producerPathTamper.RunPath | Out-Null
    }

    $producerColonUncTamper = New-Fixture 'producer-colon-unc-tamper' 'full'
    [void]$fixtures.Add($producerColonUncTamper)
    $producerColonUncRun = Get-Content -Raw `
        $producerColonUncTamper.RunPath | ConvertFrom-Json
    $producerColonUncRun.outcome.error = 'path:' + $syntheticUncPath
    Write-Json $producerColonUncTamper.RunPath $producerColonUncRun
    Assert-ThrowsLike 'producer colon UNC path tamper' 'local or rooted absolute path' {
        & $producerPath -RunEnvelopePath $producerColonUncTamper.RunPath | Out-Null
    }

    $producerPropertyNameTamper = New-Fixture 'producer-property-name-tamper' 'full'
    [void]$fixtures.Add($producerPropertyNameTamper)
    $producerPropertyRun = Get-Content -Raw `
        $producerPropertyNameTamper.RunPath | ConvertFrom-Json
    $producerPropertyRun.cleanup | Add-Member `
        -NotePropertyName $syntheticUncPath -NotePropertyValue 0
    Write-Json $producerPropertyNameTamper.RunPath $producerPropertyRun
    Assert-ThrowsLike 'producer property-name path tamper' 'local or rooted absolute path' {
        & $producerPath -RunEnvelopePath $producerPropertyNameTamper.RunPath | Out-Null
    }

    $producerFileUriTamper = New-Fixture 'producer-file-uri-tamper' 'full'
    [void]$fixtures.Add($producerFileUriTamper)
    $producerFileUriRun = Get-Content -Raw `
        $producerFileUriTamper.RunPath | ConvertFrom-Json
    $producerFileUriRun.outcome.error = $syntheticFileUri
    Write-Json $producerFileUriTamper.RunPath $producerFileUriRun
    Assert-ThrowsLike 'producer file URI tamper' 'local or rooted absolute path' {
        & $producerPath -RunEnvelopePath $producerFileUriTamper.RunPath | Out-Null
    }

    foreach ($producerEmbeddedPathCase in @(
            [PSCustomObject]@{ Name = 'embedded-file-uri'; Value = $syntheticEmbeddedFileUri },
            [PSCustomObject]@{ Name = 'colon-forward-root'; Value = $syntheticColonForwardRoot },
            [PSCustomObject]@{ Name = 'colon-forward-unc'; Value = $syntheticColonForwardUnc },
            [PSCustomObject]@{ Name = 'malformed-https-root'; Value = $syntheticMalformedHttpsRoot },
            [PSCustomObject]@{ Name = 'malformed-https-triple'; Value = $syntheticMalformedHttpsTriple },
            [PSCustomObject]@{ Name = 'malformed-https-drive-host'; Value = $syntheticMalformedHttpsDriveHost },
            [PSCustomObject]@{ Name = 'greater-root'; Value = 'note=>/synthetic-root/evidence' },
            [PSCustomObject]@{ Name = 'bracket-root'; Value = 'note[/synthetic-root/evidence' },
            [PSCustomObject]@{ Name = 'semicolon-root'; Value = 'note;/synthetic-root/evidence' })) {
        $fixture = New-Fixture "producer-$($producerEmbeddedPathCase.Name)-tamper" 'full'
        [void]$fixtures.Add($fixture)
        $tamperedRun = Get-Content -Raw $fixture.RunPath | ConvertFrom-Json
        $tamperedRun.outcome.error = $producerEmbeddedPathCase.Value
        Write-Json $fixture.RunPath $tamperedRun
        Assert-ThrowsLike "producer $($producerEmbeddedPathCase.Name) path tamper" `
            'local or rooted absolute path' {
            & $producerPath -RunEnvelopePath $fixture.RunPath | Out-Null
        }
    }

    $uppercaseHttpsFixture = New-Fixture 'uppercase-https-preservation' 'full'
    [void]$fixtures.Add($uppercaseHttpsFixture)
    $uppercaseHttpsRun = Get-Content -Raw `
        $uppercaseHttpsFixture.RunPath | ConvertFrom-Json
    $uppercaseHttpsRun.outcome.error = $syntheticUpperHttps
    Write-Json $uppercaseHttpsFixture.RunPath $uppercaseHttpsRun
    Remove-SyntheticFixtureReleaseIndexForIndependentProducerCase `
        $uppercaseHttpsFixture 'uppercase HTTPS preservation'
    $uppercaseHttpsProducerResult = @(& $producerPath `
        -RunEnvelopePath $uppercaseHttpsFixture.RunPath)
    if ($uppercaseHttpsProducerResult.Count -ne 1 -or
        $uppercaseHttpsProducerResult[0].Status -cne 'failed-full-profile') {
        throw 'Producer did not preserve uppercase HTTPS while deriving generic red.'
    }
    $uppercaseHttpsEvidence = Get-EvidenceFromFixture $uppercaseHttpsFixture
    $uppercaseHttpsResult = Invoke-Consumer `
        $uppercaseHttpsFixture $uppercaseHttpsEvidence `
        'synthetic uppercase HTTPS preservation'
    if (-not $uppercaseHttpsResult.Rejected -or $uppercaseHttpsResult.Accepted) {
        throw 'Consumer did not preserve uppercase HTTPS while retaining generic red.'
    }

    $consumerPathTamper = New-Fixture 'consumer-rooted-path-tamper' 'full'
    [void]$fixtures.Add($consumerPathTamper)
    $consumerPathEvidence = Get-EvidenceFromFixture $consumerPathTamper
    $consumerPathIndex = Get-Content -Raw $consumerPathTamper.IndexPath | ConvertFrom-Json
    $consumerPathIndex.finding.nextStep = $syntheticRootedPath
    Write-Json $consumerPathTamper.IndexPath $consumerPathIndex
    $consumerPathEvidence.summarySha256 = (Get-FileHash `
        $consumerPathTamper.IndexPath -Algorithm SHA256).Hash.ToLowerInvariant()
    Assert-ThrowsLike 'consumer rooted path tamper' 'local or rooted absolute path' {
        Invoke-Consumer $consumerPathTamper $consumerPathEvidence | Out-Null
    }

    $consumerPropertyNameTamper = New-Fixture 'consumer-property-name-tamper' 'full'
    [void]$fixtures.Add($consumerPropertyNameTamper)
    $consumerPropertyEvidence = Get-EvidenceFromFixture $consumerPropertyNameTamper
    $consumerPropertyIndex = Get-Content -Raw `
        $consumerPropertyNameTamper.IndexPath | ConvertFrom-Json
    $consumerPropertyIndex.finding | Add-Member `
        -NotePropertyName $syntheticUncPath -NotePropertyValue 'synthetic'
    Write-Json $consumerPropertyNameTamper.IndexPath $consumerPropertyIndex
    $consumerPropertyEvidence.summarySha256 = (Get-FileHash `
        $consumerPropertyNameTamper.IndexPath -Algorithm SHA256).Hash.ToLowerInvariant()
    Assert-ThrowsLike 'consumer property-name path tamper' 'local or rooted absolute path' {
        Invoke-Consumer $consumerPropertyNameTamper $consumerPropertyEvidence | Out-Null
    }

    $consumerFileUriTamper = New-Fixture 'consumer-file-uri-tamper' 'full'
    [void]$fixtures.Add($consumerFileUriTamper)
    $consumerFileUriEvidence = Get-EvidenceFromFixture $consumerFileUriTamper
    $consumerFileUriIndex = Get-Content -Raw `
        $consumerFileUriTamper.IndexPath | ConvertFrom-Json
    $consumerFileUriIndex.finding.nextStep = $syntheticFileUri
    Write-Json $consumerFileUriTamper.IndexPath $consumerFileUriIndex
    $consumerFileUriEvidence.summarySha256 = (Get-FileHash `
        $consumerFileUriTamper.IndexPath -Algorithm SHA256).Hash.ToLowerInvariant()
    Assert-ThrowsLike 'consumer file URI tamper' 'local or rooted absolute path' {
        Invoke-Consumer $consumerFileUriTamper $consumerFileUriEvidence | Out-Null
    }

    foreach ($consumerEmbeddedPathCase in @(
            [PSCustomObject]@{ Name = 'embedded-file-uri'; Value = $syntheticEmbeddedFileUri },
            [PSCustomObject]@{ Name = 'colon-forward-root'; Value = $syntheticColonForwardRoot },
            [PSCustomObject]@{ Name = 'colon-forward-unc'; Value = $syntheticColonForwardUnc },
            [PSCustomObject]@{ Name = 'malformed-https-root'; Value = $syntheticMalformedHttpsRoot },
            [PSCustomObject]@{ Name = 'malformed-https-triple'; Value = $syntheticMalformedHttpsTriple },
            [PSCustomObject]@{ Name = 'malformed-https-drive-host'; Value = $syntheticMalformedHttpsDriveHost },
            [PSCustomObject]@{ Name = 'greater-root'; Value = 'note=>/synthetic-root/evidence' },
            [PSCustomObject]@{ Name = 'bracket-root'; Value = 'note[/synthetic-root/evidence' },
            [PSCustomObject]@{ Name = 'semicolon-root'; Value = 'note;/synthetic-root/evidence' })) {
        $fixture = New-Fixture "consumer-$($consumerEmbeddedPathCase.Name)-tamper" 'full'
        [void]$fixtures.Add($fixture)
        $evidence = Get-EvidenceFromFixture $fixture
        $tamperedIndex = Get-Content -Raw $fixture.IndexPath | ConvertFrom-Json
        $tamperedIndex.finding.nextStep = $consumerEmbeddedPathCase.Value
        Write-Json $fixture.IndexPath $tamperedIndex
        $evidence.summarySha256 = (Get-FileHash `
            $fixture.IndexPath -Algorithm SHA256).Hash.ToLowerInvariant()
        Assert-ThrowsLike "consumer $($consumerEmbeddedPathCase.Name) path tamper" `
            'local or rooted absolute path' {
            Invoke-Consumer $fixture $evidence | Out-Null
        }
    }

    $cleanupTamper = New-Fixture 'cleanup-tamper' 'full'
    [void]$fixtures.Add($cleanupTamper)
    $cleanupRun = Get-Content -Raw $cleanupTamper.RunPath | ConvertFrom-Json
    $cleanupRun.cleanup.guardRemaining = 1
    Write-Json $cleanupTamper.RunPath $cleanupRun
    Assert-ThrowsLike 'producer cleanup rejection' 'zero cleanup and spill residue' {
        & $producerPath -RunEnvelopePath $cleanupTamper.RunPath | Out-Null
    }

    Write-Host ('Portable Campaign Debug release-index self-test passed: ' +
        'release build-transition monotonic ancestry/time/disposition boundaries, full/internal ' +
        'acceptance, tracked-index/external-bundle split, supported and ' +
        'unsupported skip policy, WARN and mixed-severity red policy, ' +
        'certification/runtime-error red, exact retained semantic/log re-derivation, ' +
        'recorded diagnostic tamper axes, future historical red, immutable tool/harness ' +
        'and candidate-manifest binding, unique proof IDs, raw/index/candidate tampering, ' +
        'advisory derivation, and cleanup.')
}
catch {
    $primaryFailure = $_
}
finally {
    for ($cleanupIndex = $fixtureCleanupRegistry.Count - 1;
        $cleanupIndex -ge 0;
        $cleanupIndex--) {
        $cleanupFailure = Remove-RegisteredFixtureRoot `
            -Record $fixtureCleanupRegistry[$cleanupIndex] -BestEffort
        if ($cleanupFailure) {
            [void]$cleanupFailures.Add([string]$cleanupFailure)
        }
    }
    try {
        $finalFixtureRootSnapshot = @(Get-FixtureRootResidueSnapshot)
        if (($finalFixtureRootSnapshot -join "`n") -cne
            ($initialFixtureRootSnapshot -join "`n")) {
            [void]$cleanupFailures.Add(
                'The release-index self-test changed the fixture-root residue snapshot.')
        }
    }
    catch {
        [void]$cleanupFailures.Add(
            'The release-index self-test could not audit its final fixture-root residue.')
    }
}

if ($primaryFailure) {
    if ($cleanupFailures.Count -ne 0) {
        throw [InvalidOperationException]::new(
            ($primaryFailure.Exception.Message +
                ' Fixture cleanup also failed: ' +
                ($cleanupFailures.ToArray() -join '; ')),
            $primaryFailure.Exception)
    }
    throw $primaryFailure
}
if ($cleanupFailures.Count -ne 0) {
    throw ('Portable release-index self-test cleanup failed: ' +
        ($cleanupFailures.ToArray() -join '; '))
}
