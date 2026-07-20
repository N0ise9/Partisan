[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$producerPath = Join-Path $PSScriptRoot 'New-PartisanCampaignDebugReleaseIndex.ps1'
$runnerPath = Join-Path $PSScriptRoot 'run-guarded-campaign-debug.ps1'
$candidateModulePath = Join-Path $PSScriptRoot 'Partisan.ReleaseCandidate.psm1'
$consumerPath = Join-Path $PSScriptRoot 'update-release-docs.ps1'
$checkoutRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$producerSha = (Get-FileHash -LiteralPath $producerPath -Algorithm SHA256).Hash.ToLowerInvariant()
$runnerSha = (Get-FileHash -LiteralPath $runnerPath -Algorithm SHA256).Hash.ToLowerInvariant()
$candidateModuleSha = (Get-FileHash `
    -LiteralPath $candidateModulePath -Algorithm SHA256).Hash.ToLowerInvariant()
$consumerSha = (Get-FileHash -LiteralPath $consumerPath -Algorithm SHA256).Hash.ToLowerInvariant()

function Get-ImmutableGitBlobSha256 {
    param(
        [Parameter(Mandatory = $true)][string]$Revision,
        [Parameter(Mandatory = $true)][string]$FilePath
    )

    $checkout = $checkoutRoot.TrimEnd('\', '/')
    $prefix = $checkout + [IO.Path]::DirectorySeparatorChar
    $fullPath = [IO.Path]::GetFullPath($FilePath)
    if (-not $fullPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'A corrected-canary harness fixture tool escaped the checkout.'
    }
    $relativePath = $fullPath.Substring($prefix.Length).Replace('\', '/')
    $startInfo = New-Object Diagnostics.ProcessStartInfo
    $startInfo.FileName = 'git'
    $escapedRoot = $checkout.Replace('"', '\"')
    $escapedSpec = ("{0}:{1}" -f $Revision, $relativePath).Replace('"', '\"')
    $startInfo.Arguments = "-C `"$escapedRoot`" cat-file blob `"$escapedSpec`""
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $startInfo
    try {
        if (-not $process.Start()) {
            throw 'The corrected-canary harness fixture blob reader could not start.'
        }
        $memory = New-Object IO.MemoryStream
        try {
            $process.StandardOutput.BaseStream.CopyTo($memory)
            $errorText = $process.StandardError.ReadToEnd()
            $process.WaitForExit()
            if ($process.ExitCode -ne 0) {
                throw "The corrected-canary fixture blob $relativePath could not be read: $errorText"
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

$candidateId = 'partisan-rc-0123456789ab-20260719T120000Z'
$candidateHead = '0' * 40
$candidateVersion = '0.1.0-rc.corrected-canary-selftest'
$embeddedBuildSha = '1' * 40
$embeddedBuildUtc = '2026-07-19T11:50:00Z'
$embeddedBuildLabel = 'schema71-settings24-corrected-canary-v2-selftest'
$campaignSchema = 71
$runtimeSettingsSchema = 24
$addonId = 'histasi'
$addonGuid = '698532771130111D'
$packageHashAlgorithm = 'sha256-manifest-v1'
$packageSha = '3' * 64
$workbenchCrc = '0123abcd'
$headRows = @(& git -C $checkoutRoot rev-parse HEAD 2>$null)
$headExitCode = $LASTEXITCODE
$harnessHead = ($headRows -join '').Trim()
if ($headExitCode -ne 0 -or $harnessHead -cnotmatch '^[0-9a-f]{40}$') {
    throw 'The corrected-canary self-test could not resolve the harness Git HEAD.'
}
$runnerGitBlobSha = Get-ImmutableGitBlobSha256 $harnessHead $runnerPath
$candidateModuleGitBlobSha = Get-ImmutableGitBlobSha256 $harnessHead $candidateModulePath
$producerGitBlobSha = Get-ImmutableGitBlobSha256 $harnessHead $producerPath
$consumerGitBlobSha = Get-ImmutableGitBlobSha256 $harnessHead $consumerPath
$runId = 'seed1985_t0_p1_u1784500000'

$clientDiagnosticIdentity = [pscustomobject][ordered]@{
    fileName = 'runtime-client-diagnostic.exe'
    fileVersion = '1.0.0.0'
    productVersion = '1.0.0.0'
    length = 4096
    sha256 = '4' * 64
}
$clientRuntimeIdentity = [pscustomobject][ordered]@{
    fileName = 'runtime-client.exe'
    fileVersion = '1.0.0.0'
    productVersion = '1.0.0.0'
    length = 8192
    sha256 = '5' * 64
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

    Write-Utf8Text `
        -Path $Path `
        -Text (($Value | ConvertTo-Json -Depth 32).Replace("`r`n", "`n") + "`n")
}

function New-Assertion {
    param(
        [string]$Id,
        [string]$Status = 'PASS',
        [bool]$CountsTowardCertification = $false,
        [string]$ProofLevel = 'CONTROLLED_RUNTIME',
        [string]$ObservedPath = 'synthetic_runtime_probe',
        [string]$RequiredPath = 'typed runtime proof',
        [string]$Expected = '',
        [string]$Actual = '',
        [string]$FailureReason = ''
    )

    return [pscustomobject][ordered]@{
        m_sAssertionId = $Id
        m_sExpected = if ([string]::IsNullOrEmpty($Expected)) { "expected $Id" } else { $Expected }
        m_sActual = if ([string]::IsNullOrEmpty($Actual)) { "actual $Id" } else { $Actual }
        m_sStatus = $Status
        m_sFailureReason = if ([string]::IsNullOrEmpty($FailureReason)) {
            "synthetic $Status disposition for $Id"
        }
        else {
            $FailureReason
        }
        m_sProofLevel = $ProofLevel
        m_sObservedPath = $ObservedPath
        m_sRequiredPath = $RequiredPath
        m_bCountsTowardCertification = $CountsTowardCertification
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
        [string]$Status = 'PASS',
        [object[]]$Assertions = @(),
        [string]$Category = 'foundation',
        [string]$Feature = 'corrected_canary',
        [string]$Stage = 'runtime',
        [object[]]$Metrics = @()
    )

    return [pscustomobject][ordered]@{
        m_sCaseId = $Id
        m_sCategory = $Category
        m_sFeature = $Feature
        m_sStage = $Stage
        m_sStatus = $Status
        m_sReason = if ($Status -ceq 'PASS') {
            'assertions passed'
        }
        else {
            'synthetic corrected-canary disposition'
        }
        m_iStartSecond = 0
        m_iEndSecond = 1
        m_aAssertions = $Assertions
        m_aMetrics = $Metrics
        m_aEvidence = @('synthetic portable corrected-canary fixture')
    }
}

function New-Metric {
    param(
        [string]$Id,
        [string]$Value,
        [string]$Unit = '',
        [string]$Feature = '',
        [string]$Stage = '')

    return [pscustomobject][ordered]@{
        m_sMetricId = $Id
        m_sName = $Id
        m_sValue = $Value
        m_sUnit = $Unit
        m_sFeature = $Feature
        m_sStage = $Stage
    }
}

function Get-RunnerCorrectedCanaryContracts {
    param([Parameter(Mandatory = $true)][string]$RunId)

    $tokens = $null
    $parseErrors = $null
    $ast = [Management.Automation.Language.Parser]::ParseFile(
        $runnerPath,
        [ref]$tokens,
        [ref]$parseErrors)
    if (@($parseErrors).Count -ne 0) {
        throw 'The guarded runner could not be parsed for corrected-canary fixture contracts.'
    }
    $source = New-Object Collections.Generic.List[string]
    foreach ($name in @(
            'Get-FocusedForceAuthorityAssertionIds',
            'Get-CorrectedCanaryCaseManifest',
            'Get-CorrectedCanaryAssertionManifest',
            'Get-CampaignDebugStateDiffLabels')) {
        $matches = @($ast.FindAll({
                    param($node)
                    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
                        $node.Name -ceq $name
                }, $true))
        if ($matches.Count -ne 1) {
            throw "The guarded runner does not expose exactly one $name fixture contract."
        }
        [void]$source.Add($matches[0].Extent.Text)
    }
    $contractScript = [scriptblock]::Create(@"
param(`$FixtureRunId)
$($source.ToArray() -join "`n`n")
[pscustomobject]@{
    Cases = @(Get-CorrectedCanaryCaseManifest -RunId `$FixtureRunId)
    Assertions = @(Get-CorrectedCanaryAssertionManifest -RunId `$FixtureRunId)
    StateDiffLabels = @(Get-CampaignDebugStateDiffLabels)
}
"@)
    return & $contractScript $RunId
}

function ConvertTo-RecordedValidationSummary {
    param([Parameter(Mandatory = $true)]$Validation)

    return [pscustomobject][ordered]@{
        Valid = [bool]$Validation.Valid
        Problems = @($Validation.Problems)
        RunId = [string]$Validation.RunId
        Profile = [string]$Validation.Profile
        ProofScope = [string]$Validation.ProofScope
        FullCertification = [bool]$Validation.FullCertification
        BuildProvenanceMatches =
            [string]$Validation.BuildSha -ceq $embeddedBuildSha -and
            [string]$Validation.BuildUtc -ceq $embeddedBuildUtc -and
            [string]$Validation.BuildLabel -ceq $embeddedBuildLabel
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
        Phase17 = @($Validation.Phase17)
        Phase17Metrics = $Validation.Phase17Metrics
        Phase24 = @($Validation.Phase24)
        Phase24Metrics = $Validation.Phase24Metrics
        StagedCleanup = @($Validation.StagedCleanup)
        FocusedCaseId = [string]$Validation.FocusedCaseId
        FocusedCaseStatus = [string]$Validation.FocusedCaseStatus
        FocusedAssertions = @($Validation.FocusedAssertions)
        CorrectedCanaryAssertionManifestExact =
            [bool]$Validation.CorrectedCanaryAssertionManifestExact
        CorrectedCanaryCaseSetExact = [bool]$Validation.CorrectedCanaryCaseSetExact
        CorrectedCanaryWarningContractExact =
            [bool]$Validation.CorrectedCanaryWarningContractExact
        CorrectedCanaryNoBlockedAssertions =
            [bool]$Validation.CorrectedCanaryNoBlockedAssertions
        CorrectedCanaryOrphanContractExact =
            [bool]$Validation.CorrectedCanaryOrphanContractExact
        IntentionalMissionConvoyAdmissionDiagnosticsProven = $false
        IntentionalMissionConvoySettlementDiagnosticProven = $false
        IntentionalMissionConvoyCorruptionDiagnosticsProven = $false
        IntentionalMissionConvoyWatchdogDiagnosticProven = $false
        FinalOrphanCleanupPass = [bool]$Validation.FinalOrphanCleanupPass
        FinalOrphanActiveGroups = [string]$Validation.FinalOrphanActiveGroups
    }
}

function Set-RawCounts {
    param($Raw)

    $cases = @($Raw.m_aCases)
    $Raw.m_iPassCount = @($cases | Where-Object m_sStatus -CEQ 'PASS').Count
    $Raw.m_iWarnCount = @($cases | Where-Object m_sStatus -CEQ 'WARN').Count
    $Raw.m_iFailCount = @($cases | Where-Object m_sStatus -CEQ 'FAIL').Count
    $Raw.m_iBlockedCount = @($cases | Where-Object m_sStatus -CEQ 'BLOCKED').Count
    $Raw.m_iSkippedCount = @($cases | Where-Object m_sStatus -CEQ 'SKIPPED').Count
    $certificationAssertions = @($cases | ForEach-Object { @($_.m_aAssertions) } |
        Where-Object { [bool]$_.m_bCountsTowardCertification })
    $Raw.m_iCertificationRequiredCount = $certificationAssertions.Count
    $Raw.m_iCertificationProvenCount = @($certificationAssertions |
        Where-Object m_sStatus -CEQ 'PASS').Count
    $Raw.m_iCertificationFailCount = @($certificationAssertions |
        Where-Object m_sStatus -CEQ 'FAIL').Count
    $Raw.m_iCertificationBlockedCount = @($certificationAssertions |
        Where-Object m_sStatus -CEQ 'BLOCKED').Count
    $Raw.m_iCertificationWarnCount = @($certificationAssertions |
        Where-Object m_sStatus -CEQ 'WARN').Count
    $Raw.m_bCertificationPassed = $false
}

function Get-FocusedAssertionIds {
    return @(
        'combat_presence.aggregate',
        'combat_presence.empty_vehicle',
        'combat_presence.authoritative_samples',
        'combat_presence.rejected_rows',
        'combat_presence.heat_lifecycle',
        'combat_presence.schema62_migration',
        'combat_presence.schema63_restore',
        'combat_presence.malformed_fail_cold',
        'combat_presence.deterministic_diagnostics',
        'ownership_transition.aggregate',
        'ownership_transition.military',
        'ownership_transition.political',
        'ownership_transition.recapture',
        'ownership_transition.replay',
        'ownership_transition.restore',
        'ownership_transition.restore_queue_order',
        'ownership_transition.persistence_deadline',
        'ownership_transition.projection_revision',
        'ownership_transition.location_identity',
        'ownership_transition.linked_support',
        'ownership_transition.causes',
        'ownership_transition.security_fail_closed',
        'ownership_transition.migration_retention',
        'town_influence.aggregate',
        'town_influence.scaling',
        'town_influence.hysteresis',
        'town_influence.idempotency',
        'town_influence.projection',
        'town_influence.population',
        'town_influence.rejection',
        'town_influence.ownership_authority',
        'town_influence.external_completion',
        'town_influence.pre64_invader',
        'town_influence.migration',
        'town_influence.current_restore')
}

function Get-AcceptedDiagnosticLogText {
    return ((@(
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
        'Scripts/Game/GameMode/SCR_BaseGameMode.c:845 Function OnPlayerAuditSuccess',
        '17:00:01.000 SCRIPT : Partisan campaign debug CLI | armed focused force_authority run (not full certification)',
        '17:00:01.100 SCRIPT : Partisan campaign debug CLI | started force_authority on attempt 1',
        '17:00:03.000 SCRIPT : Partisan campaign debug | PASS | early_mechanics.force_authority | assertions passed'
    ) -join "`n") + "`n")
}

function Invoke-RunnerSemanticValidation {
    param(
        [string]$JsonPath,
        [string]$SummaryPath,
        [string]$StateDiffPath,
        [string]$GuardRoot
    )

    $tokens = $null
    $parseErrors = $null
    $ast = [Management.Automation.Language.Parser]::ParseFile(
        $runnerPath,
        [ref]$tokens,
        [ref]$parseErrors)
    if (@($parseErrors).Count -ne 0) {
        throw 'The guarded runner could not be parsed by the corrected-canary self-test.'
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
        $matches = @($ast.FindAll({
                    param($node)
                    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
                        $node.Name -ceq $functionName
                }, $true))
        if ($matches.Count -ne 1) {
            throw "The guarded runner does not expose exactly one $functionName function."
        }
        [void]$functionSource.Add($matches[0].Extent.Text)
    }
    $semanticScript = [scriptblock]::Create(@"
param(`$ArtifactParameters, `$DiagnosticParameters)
$($functionSource.ToArray() -join "`n`n")
`$artifactValidation = Test-CampaignDebugArtifacts @ArtifactParameters
`$errorCensus = Get-GuardErrorCensus @DiagnosticParameters
[pscustomobject]@{
    ArtifactValidation = `$artifactValidation
    ErrorCensus = `$errorCensus
}
"@)
    return & $semanticScript @{
        JsonPath = $JsonPath
        SummaryPath = $SummaryPath
        StateDiffPath = $StateDiffPath
        ExpectedSha = $embeddedBuildSha
        ExpectedUtc = $embeddedBuildUtc
        ExpectedLabel = $embeddedBuildLabel
        ExpectedProfile = 'force_authority'
        RequireCorrectedCanaryContract = $true
    } @{
        GuardRoot = $GuardRoot
        Profile = 'force_authority'
        IntentionalMissionConvoyAdmissionDiagnosticsProven = $false
        IntentionalMissionConvoySettlementDiagnosticProven = $false
        IntentionalMissionConvoyCorruptionDiagnosticsProven = $false
        IntentionalMissionConvoyWatchdogDiagnosticProven = $false
    }
}

function New-CorrectedCanaryFixture {
    param(
        [string]$FixtureRoot,
        [ValidateSet(
            'green', 'proof-red', 'warning-red', 'diagnostic-red',
            'hidden-skip', 'focused-certification-swap',
            'balanced-certification-swap', 'nonfocused-id-substitution',
            'diff-missing', 'diff-duplicate', 'diff-renamed', 'diff-order',
            'diff-arithmetic', 'diff-nonnumeric', 'diff-nonzero',
            'diff-extra-line',
            'orphan-id-red', 'orphan-metric-id-red', 'orphan-metric-case-red',
            'orphan-metric-name-red', 'orphan-metadata-red', 'orphan-actual-red',
            'orphan-certification-red', 'error-log-red', 'crash-log-red')]
        [string]$Mode = 'green'
    )

    $leaf = '20260719T120000Z-' + [Guid]::NewGuid().ToString('N')
    $bundle = Join-Path $FixtureRoot $leaf
    New-Item -ItemType Directory -Path $bundle -Force | Out-Null

    $contracts = Get-RunnerCorrectedCanaryContracts -RunId $runId
    $cases = New-Object Collections.Generic.List[object]
    foreach ($caseContract in @($contracts.Cases)) {
        $assertions = New-Object Collections.Generic.List[object]
        foreach ($assertionContract in @($contracts.Assertions | Where-Object {
                    [string]$_.CaseId -ceq [string]$caseContract.Id
                })) {
            $parameters = @{
                Id = [string]$assertionContract.Id
                Status = [string]$assertionContract.Status
                CountsTowardCertification =
                    [bool]$assertionContract.CountsTowardCertification
            }
            if ([string]$assertionContract.Id -ceq 'cleanup.player_marker.live') {
                $parameters.Expected =
                    'enabled player marker service has desired/tracked/live marker after cleanup'
                $parameters.Actual =
                    'enabled 1 | desired 0 | tracked 0 | live 0 | entry 1'
                $parameters.FailureReason =
                    'player marker did not reconcile after campaign debug completion'
                $parameters.ProofLevel = 'STATE_ONLY'
                $parameters.ObservedPath = 'diagnostic_only'
                $parameters.RequiredPath = 'no debug-owned state or world leak'
            }
            elseif ([string]$assertionContract.Id -ceq 'isolation.world_scope') {
                $parameters.Expected =
                    'runtime certification remains scoped to the disposable development session'
                $parameters.Actual =
                    'world runtime, player inventory, health, and service caches require session restart before another certifying run'
                $parameters.FailureReason =
                    'restart the disposable development session before another certification run'
                $parameters.ProofLevel = 'EXTERNAL_PROCESS'
                $parameters.ObservedPath = 'manual_external_gap'
                $parameters.RequiredPath =
                    'external process restart, reconnect, or long-soak harness'
            }
            elseif ([string]$assertionContract.Id -ceq
                'cleanup.orphan_active_groups') {
                $parameters.Expected =
                    'no active groups without zone/mission/support/order/QRF backing'
                $parameters.Actual = '0 | total 0 | debug 0 | smoke 0 | other 0'
                $parameters.FailureReason =
                    'orphan active groups remain after debug run'
                $parameters.ProofLevel = 'STATE_ONLY'
                $parameters.ObservedPath = 'cleanup_probe'
                $parameters.RequiredPath = 'no debug-owned state or world leak'
            }
            [void]$assertions.Add((New-Assertion @parameters))
        }
        $metrics = @()
        if ([string]$caseContract.Id -ceq 'cleanup.run_leak_snapshot') {
            $metrics = @((New-Metric `
                -Id 'cleanup.orphan_active_groups' `
                -Value '0' `
                -Unit 'count' `
                -Feature 'campaign_debug' `
                -Stage 'final'))
        }
        [void]$cases.Add((New-Case `
            -Id ([string]$caseContract.Id) `
            -Status ([string]$caseContract.Status) `
            -Assertions $assertions.ToArray() `
            -Category ([string]$caseContract.Category) `
            -Feature ([string]$caseContract.Feature) `
            -Stage ([string]$caseContract.Stage) `
            -Metrics $metrics))
    }

    $focusedCase = @($cases | Where-Object {
        $_.m_sCaseId -ceq 'early_mechanics.force_authority'
    })[0]
    if ($Mode -ceq 'proof-red') {
        foreach ($assertionId in @(
                'ownership_transition.military',
                'ownership_transition.political')) {
            (@($focusedCase.m_aAssertions | Where-Object {
                $_.m_sAssertionId -ceq $assertionId
            })[0]).m_sStatus = 'FAIL'
        }
        $focusedCase.m_sStatus = 'FAIL'
    }
    if ($Mode -ceq 'focused-certification-swap') {
        $focusedCase.m_aAssertions[0].m_bCountsTowardCertification = $false
        (@($focusedCase.m_aAssertions | Where-Object {
            $_.m_sAssertionId -ceq 'town_influence.external_completion'
        })[0]).m_bCountsTowardCertification = $true
    }
    $markerCase = @($cases | Where-Object {
        $_.m_sCaseId -ceq 'cleanup.player_marker_completion'
    })[0]
    $markerWarning = @($markerCase.m_aAssertions | Where-Object {
        $_.m_sAssertionId -ceq 'cleanup.player_marker.live'
    })[0]
    if ($Mode -ceq 'warning-red') {
        $markerWarning.m_sAssertionId = 'cleanup.player_marker.unexpected'
    }
    if ($Mode -ceq 'hidden-skip') {
        $markerCase.m_aAssertions += New-Assertion `
            -Id 'cleanup.player_marker.hidden_skip' `
            -Status 'SKIPPED'
    }
    if ($Mode -ceq 'balanced-certification-swap') {
        $cases[0].m_aAssertions[0].m_bCountsTowardCertification = $false
        $smokeAssertion = @($cases | ForEach-Object { @($_.m_aAssertions) } |
            Where-Object m_sAssertionId -CEQ 'cleanup.smoke_prefixed_records')[0]
        $smokeAssertion.m_bCountsTowardCertification = $true
    }
    if ($Mode -ceq 'nonfocused-id-substitution') {
        $cases[0].m_aAssertions[0].m_sAssertionId =
            'isolation.development_world.substituted'
    }
    $orphanCase = @($cases | Where-Object {
        $_.m_sCaseId -ceq 'cleanup.run_leak_snapshot'
    })[0]
    $orphanAssertion = @($orphanCase.m_aAssertions | Where-Object {
        $_.m_sAssertionId -ceq 'cleanup.orphan_active_groups'
    })[0]
    if ($Mode -ceq 'orphan-id-red') {
        $orphanAssertion.m_sAssertionId = 'cleanup.orphan_Active_groups'
    }
    if ($Mode -ceq 'orphan-certification-red') {
        $orphanAssertion.m_bCountsTowardCertification = $false
    }
    if ($Mode -ceq 'orphan-metric-id-red') {
        $orphanCase.m_aMetrics[0].m_sMetricId = 'cleanup.orphan_active_group'
    }
    if ($Mode -ceq 'orphan-metric-case-red') {
        $orphanCase.m_aMetrics[0].m_sMetricId = 'cleanup.Orphan_active_groups'
    }
    if ($Mode -ceq 'orphan-metric-name-red') {
        $orphanCase.m_aMetrics[0].m_sName = 'cleanup.Orphan_active_groups'
    }
    if ($Mode -ceq 'orphan-metadata-red') {
        $orphanAssertion.m_sObservedPath = 'Cleanup_probe'
    }
    if ($Mode -ceq 'orphan-actual-red') {
        $orphanAssertion.m_sActual =
            '0 | total 1 | debug 0 | smoke 0 | other 1'
    }

    $artifactName = "HST_CampaignDebug_$runId.json"
    $summaryName = "HST_CampaignDebug_${runId}_summary.txt"
    $diffName = "HST_CampaignDebug_${runId}_state_diff.txt"
    $raw = [pscustomobject][ordered]@{
        m_sRunId = $runId
        m_sProfile = 'force_authority'
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
        m_aCases = $cases.ToArray()
        m_aMetrics = @(
            [pscustomobject]@{ m_sMetricId = 'run.build.sha'; m_sValue = $embeddedBuildSha; m_sUnit = 'sha'; m_sFeature = 'campaign_debug'; m_sStage = 'run' },
            [pscustomobject]@{ m_sMetricId = 'run.build.utc'; m_sValue = $embeddedBuildUtc; m_sUnit = 'utc'; m_sFeature = 'campaign_debug'; m_sStage = 'run' },
            [pscustomobject]@{ m_sMetricId = 'run.build.label'; m_sValue = $embeddedBuildLabel; m_sUnit = 'label'; m_sFeature = 'campaign_debug'; m_sStage = 'run' },
            [pscustomobject]@{ m_sMetricId = 'run.trigger'; m_sValue = 'cli_autostart'; m_sUnit = 'source'; m_sFeature = 'campaign_debug'; m_sStage = 'run' })
        m_aArtifacts = @($artifactName, $summaryName, $diffName)
    }
    Set-RawCounts $raw

    $artifactRelative = "raw/campaign-debug/$artifactName"
    $summaryRelative = "raw/campaign-debug/$summaryName"
    $diffRelative = "raw/campaign-debug/$diffName"
    Write-Json (Join-Path $bundle $artifactRelative) $raw
    Write-Utf8Text `
        -Path (Join-Path $bundle $summaryRelative) `
        -Text ("Partisan campaign debug complete`nrun $runId`n" +
            "profile force_authority`nbuild source $embeddedBuildSha | " +
            "UTC $embeddedBuildUtc | label $embeddedBuildLabel`n")
    $diffLines = New-Object Collections.Generic.List[string]
    [void]$diffLines.Add('Partisan campaign debug state diff')
    [void]$diffLines.Add("run $runId")
    $diffLabels = @($contracts.StateDiffLabels)
    if ($Mode -ceq 'diff-missing') {
        $diffLabels = @($diffLabels | Select-Object -First 17)
    }
    elseif ($Mode -ceq 'diff-duplicate') {
        $diffLabels[17] = $diffLabels[16]
    }
    elseif ($Mode -ceq 'diff-renamed') {
        $diffLabels[8] = 'mission asset'
    }
    elseif ($Mode -ceq 'diff-order') {
        $swap = $diffLabels[3]
        $diffLabels[3] = $diffLabels[4]
        $diffLabels[4] = $swap
    }
    foreach ($label in $diffLabels) {
        [void]$diffLines.Add("$label 0 -> 0 | delta 0")
    }
    if ($Mode -ceq 'diff-arithmetic') {
        $diffLines[2] = 'elapsed 0 -> 1 | delta 0'
    }
    elseif ($Mode -ceq 'diff-nonnumeric') {
        $diffLines[2] = 'elapsed before -> 0 | delta 0'
    }
    elseif ($Mode -ceq 'diff-nonzero') {
        $diffLines[2] = 'elapsed 0 -> 1 | delta 1'
    }
    elseif ($Mode -ceq 'diff-extra-line') {
        [void]$diffLines.Add('unexpected 0 -> 0 | delta 0')
    }
    Write-Utf8Text `
        -Path (Join-Path $bundle $diffRelative) `
        -Text (($diffLines.ToArray() -join "`n") + "`n")

    Write-Utf8Text `
        -Path (Join-Path $bundle 'config/HST_Settings.json') `
        -Text "{`"schemaVersion`":$runtimeSettingsSchema}`n"
    $manifest = [pscustomobject][ordered]@{
        manifestSchemaVersion = 1
        createdUtc = '2026-07-19T11:55:00Z'
        candidate = [pscustomobject][ordered]@{
            id = $candidateId
            version = $candidateVersion
            state = 'retained-uncertified'
        }
        source = [pscustomobject][ordered]@{
            gitHead = $candidateHead
            embeddedImplementation = [pscustomobject][ordered]@{
                sha = $embeddedBuildSha
                utc = $embeddedBuildUtc
                label = $embeddedBuildLabel
            }
            campaignSchema = $campaignSchema
            runtimeSettingsSchema = $runtimeSettingsSchema
        }
        addon = [pscustomobject][ordered]@{
            id = $addonId
            guid = $addonGuid
            version = $candidateVersion
        }
        toolchain = [pscustomobject][ordered]@{
            client = $clientRuntimeIdentity
            clientDiagnostic = $clientDiagnosticIdentity
        }
        workbench = [pscustomobject][ordered]@{ crc = $workbenchCrc }
        package = [pscustomobject][ordered]@{
            hashAlgorithm = $packageHashAlgorithm
            sha256 = $packageSha
        }
    }
    $manifestPath = Join-Path $bundle 'identity/candidate.json'
    Write-Json $manifestPath $manifest
    $manifestSha = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $ready = [pscustomobject][ordered]@{
        schemaVersion = 1
        candidateId = $candidateId
        gitHead = $candidateHead
        packageSha256 = $packageSha
        manifestSha256 = $manifestSha
    }
    $readyPath = Join-Path $bundle 'identity/candidate.ready.json'
    Write-Json $readyPath $ready
    $readySha = (Get-FileHash -LiteralPath $readyPath -Algorithm SHA256).Hash.ToLowerInvariant()

    $acceptedDiagnosticText = Get-AcceptedDiagnosticLogText
    $diagnosticText = $acceptedDiagnosticText
    if ($Mode -ceq 'diagnostic-red') {
        $diagnosticText += '17:00:04.000 SCRIPT (E): Partisan synthetic unapproved diagnostic' + "`n"
    }
    foreach ($name in @('script.log', 'console.log')) {
        Write-Utf8Text `
            -Path (Join-Path $bundle "raw/logs/logs_synthetic/$name") `
            -Text $diagnosticText
    }
    $errorLogText = $acceptedDiagnosticText
    $crashLogText = $acceptedDiagnosticText
    if ($Mode -ceq 'error-log-red') {
        $errorLogText +=
            '17:00:04.000 SCRIPT (E): unique auxiliary error-log event' + "`n"
    }
    if ($Mode -ceq 'crash-log-red') {
        $crashLogText += 'fatal error: unique auxiliary crash-log event' + "`n"
    }
    Write-Utf8Text `
        -Path (Join-Path $bundle 'raw/logs/logs_synthetic/error.log') `
        -Text $errorLogText
    Write-Utf8Text `
        -Path (Join-Path $bundle 'raw/logs/logs_synthetic/crash.log') `
        -Text $crashLogText

    $settingsSha = (Get-FileHash `
        -LiteralPath (Join-Path $bundle 'config/HST_Settings.json') `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    $semantic = Invoke-RunnerSemanticValidation `
        -JsonPath (Join-Path $bundle $artifactRelative) `
        -SummaryPath (Join-Path $bundle $summaryRelative) `
        -StateDiffPath (Join-Path $bundle $diffRelative) `
        -GuardRoot (Join-Path $bundle 'raw')
    $validation = ConvertTo-RecordedValidationSummary `
        -Validation $semantic.ArtifactValidation
    $errorCensus = $semantic.ErrorCensus
    $outcomeError = ''
    if (-not $validation.Valid) {
        $outcomeError = 'Campaign Debug artifacts completed but failed the exact validation contract.'
    }
    elseif (-not $errorCensus.Valid) {
        $outcomeError = 'Campaign Debug runtime completed with unapproved hard diagnostics.'
    }

    $fileRows = @()
    foreach ($file in @(Get-ChildItem -LiteralPath $bundle -Recurse -File -Force |
            Sort-Object FullName)) {
        $fileRows += [pscustomobject][ordered]@{
            path = $file.FullName.Substring($bundle.Length + 1).Replace('\', '/')
            length = [long]$file.Length
            sha256 = (Get-FileHash `
                -LiteralPath $file.FullName `
                -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }
    $run = [pscustomobject][ordered]@{
        schemaVersion = 2
        evidenceKind = 'packaged-campaign-debug'
        startedUtc = '2026-07-19T12:00:00Z'
        completedUtc = '2026-07-19T12:00:10Z'
        candidate = [pscustomobject][ordered]@{
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
        harness = [pscustomobject][ordered]@{
            gitHead = $harnessHead
            dirty = $false
            campaignRunnerSha256 = $runnerSha
            campaignRunnerGitBlobSha256 = $runnerGitBlobSha
            candidateModuleSha256 = $candidateModuleSha
            candidateModuleGitBlobSha256 = $candidateModuleGitBlobSha
            releaseIndexProducerSha256 = $producerSha
            releaseIndexProducerGitBlobSha256 = $producerGitBlobSha
            releaseDocsConsumerSha256 = $consumerSha
            releaseDocsConsumerGitBlobSha256 = $consumerGitBlobSha
        }
        launch = [pscustomobject][ordered]@{
            profile = 'force_authority'
            proofScope = 'focused_force_authority'
            worldResource = 'Worlds/HST_Dev/HST_Dev.ent'
            stagedPackage = $true
            addonSearchRootCount = 2
            addonGuid = $addonGuid
            packageSha256 = $packageSha
            diagnosticExecutable = $clientDiagnosticIdentity
            recordedRuntimeExecutable = $clientRuntimeIdentity
        }
        outcome = [pscustomobject][ordered]@{
            success = [string]::IsNullOrWhiteSpace($outcomeError)
            armed = $true
            started = $true
            completed = $true
            candidateBoundaryVerified = $true
            mountAttestation = [pscustomobject][ordered]@{
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
            hardDiagnosticClassifierChecks = 38
            runtimeSeconds = 10
            error = $outcomeError
            validation = $validation
            errorCensus = $errorCensus
        }
        settings = [pscustomobject][ordered]@{
            schemaVersion = $runtimeSettingsSchema
            sha256 = $settingsSha
            guardedRuntimeCopy = $true
        }
        cleanup = [pscustomobject][ordered]@{
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
        files = $fileRows
    }
    $runPath = Join-Path $bundle 'run.json'
    Write-Json $runPath $run
    return [pscustomobject]@{
        Bundle = $bundle
        RunPath = $runPath
        IndexPath = Join-Path $bundle 'release-index.json'
        RawPath = Join-Path $bundle $artifactRelative
    }
}

function Invoke-Producer {
    param($Fixture)

    $rows = @(& $producerPath -RunEnvelopePath $Fixture.RunPath)
    if ($rows.Count -ne 1) {
        throw 'The corrected-canary producer returned an unexpected result count.'
    }
    return $rows[0]
}

function Assert-ProducerRejected {
    param([string]$Label, [scriptblock]$Action)

    $rejected = $false
    try {
        & $Action
    }
    catch {
        $rejected = $true
    }
    if (-not $rejected) {
        throw "Corrected-canary fail-closed self-test did not reject $Label."
    }
}

$tempParent = [IO.Path]::GetFullPath((Join-Path `
    ([IO.Path]::GetTempPath()) `
    ('PartisanCorrectedCanaryV2-' + [Guid]::NewGuid().ToString('N'))))
$expectedTempPrefix = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') +
    [IO.Path]::DirectorySeparatorChar
if (-not $tempParent.StartsWith(
        $expectedTempPrefix,
        [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Corrected-canary self-test temporary-root containment failed.'
}
New-Item -ItemType Directory -Path $tempParent -Force | Out-Null

try {
    $green = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'green')
    $greenReceipt = Invoke-Producer $green
    $greenIndex = Get-Content -Raw -LiteralPath $green.IndexPath | ConvertFrom-Json
    if ([int]$greenIndex.schemaVersion -ne 2 -or
        [string]$greenIndex.evidenceKind -cne
            'packaged-campaign-debug-corrected-canary' -or
        [string]$greenIndex.policyId -cne
            'partisan-campaign-debug-corrected-canary-v2' -or
        [string]$greenIndex.result.status -cne 'passed-noncertifying' -or
        [string]$greenIndex.result.acceptanceDisposition -cne
            'accepted-noncertifying' -or
        [string]$greenIndex.result.releaseDisposition -cne 'proceed-full-profile' -or
        [bool]$greenIndex.result.certificationPassed -or
        [int]$greenIndex.proof.caseCount -ne 11 -or
        [int]$greenIndex.proof.pass -ne 9 -or
        [int]$greenIndex.proof.warn -ne 2 -or
        [int]$greenIndex.proof.fail -ne 0 -or
        [int]$greenIndex.proof.blocked -ne 0 -or
        [int]$greenIndex.proof.skipped -ne 0 -or
        [string]$greenIndex.proof.focusedCaseId -cne
            'early_mechanics.force_authority' -or
        [string]$greenIndex.proof.focusedCaseStatus -cne 'PASS' -or
        [int]$greenIndex.proof.focusedAssertionCount -ne 35 -or
        [int]$greenIndex.proof.focusedAssertionsPassed -ne 35 -or
        [int]$greenIndex.proof.certificationRequired -ne 87 -or
        [int]$greenIndex.proof.certificationProven -ne 87 -or
        [int]$greenIndex.proof.certificationFail -ne 0 -or
        [int]$greenIndex.proof.certificationBlocked -ne 0 -or
        [int]$greenIndex.proof.certificationWarn -ne 0 -or
        [int]$greenIndex.proof.assertionCount -ne 91 -or
        [int]$greenIndex.proof.stateDiffRows -ne 18 -or
        [int]$greenIndex.proof.nonzeroStateDiffRows -ne 0 -or
        -not [bool]$greenIndex.proof.finalOrphanCleanupPass -or
        [int]$greenIndex.proof.finalOrphanActiveGroups -ne 0 -or
        -not [bool]$greenIndex.proof.focusedAssertionSetExact -or
        -not [bool]$greenIndex.proof.focusedAssertionsCertificationExact -or
        -not [bool]$greenIndex.proof.correctedCanaryCaseSetExact -or
        -not [bool]$greenIndex.proof.correctedCanaryAssertionManifestExact -or
        -not [bool]$greenIndex.proof.correctedCanaryWarningContractExact -or
        -not [bool]$greenIndex.proof.correctedCanaryNoBlockedAssertions -or
        -not [bool]$greenIndex.proof.correctedCanaryStateDiffManifestExact -or
        -not [bool]$greenIndex.proof.correctedCanaryOrphanContractExact -or
        -not [bool]$greenIndex.proof.correctedCanaryAssertionSkipFree -or
        -not [bool]$greenIndex.proof.correctedCanaryProofAxisPassed -or
        [int]$greenIndex.diagnostics.hardDiagnosticCount -ne 2 -or
        [int]$greenIndex.diagnostics.approvedStockDiagnosticCount -ne 2 -or
        [int]$greenIndex.diagnostics.approvedIntentionalDiagnosticCount -ne 0 -or
        [int]$greenIndex.diagnostics.unapprovedHardDiagnosticCount -ne 0 -or
        [int]$greenIndex.diagnostics.canonicalErrorLogCount -ne 1 -or
        [int]$greenIndex.diagnostics.canonicalCrashLogCount -ne 1 -or
        -not [bool]$greenIndex.diagnostics.auxiliaryDiagnosticsValid -or
        -not [bool]$greenIndex.diagnostics.errorLogProjectionExact -or
        -not [bool]$greenIndex.diagnostics.crashLogProjectionExact -or
        [int]$greenIndex.diagnostics.auxiliaryUnapprovedEventCount -ne 0 -or
        [int]$greenIndex.diagnostics.classifierSelfTestCount -ne 38 -or
        [int]$greenIndex.integrity.envelopeFileCount -ne 10 -or
        -not [bool]$greenIndex.integrity.envelopeFilesRehashed -or
        [string]$greenReceipt.Status -cne 'passed-noncertifying') {
        throw 'The portable corrected-canary green contract self-test failed.'
    }
    $warningIds = @($greenIndex.proof.warningAssertionIds)
    $warningRows = @($greenIndex.proof.warningAssertions)
    $blockedIds = @($greenIndex.proof.blockedAssertions | ForEach-Object { $_.id })
    if ($warningIds.Count -ne 2 -or
        [string]$warningIds[0] -cne 'cleanup.player_marker.live' -or
        [string]$warningIds[1] -cne 'isolation.world_scope' -or
        $warningRows.Count -ne 2 -or
        [string]$warningRows[0].caseId -cne 'cleanup.player_marker_completion' -or
        [string]$warningRows[1].caseId -cne 'cleanup.state_isolation_restore' -or
        $blockedIds.Count -ne 0) {
        throw 'The portable corrected-canary advisory identity self-test failed.'
    }
    $greenJsonText = Get-Content -Raw -LiteralPath $green.IndexPath
    if ($greenJsonText -match '(?i)[a-z]:[\\/]' -or
        $greenJsonText -match '(?i)file:(?:/+|\\+)') {
        throw 'The portable corrected-canary index leaked a local absolute path.'
    }
    $syntheticDrivePath = ([char]67) + ':/synthetic-root/item'
    $urlWrappedPathCases = @(
        [pscustomobject]@{
            Name = 'URL-wrapped drive path'
            Value = 'https://example.invalid/evidence/' + $syntheticDrivePath
        },
        [pscustomobject]@{
            Name = 'URL-wrapped UNC path'
            Value = 'https://example.invalid//synthetic-host/share/item'
        },
        [pscustomobject]@{
            Name = 'URL-wrapped file URI'
            Value = 'https://example.invalid/redirect?target=file:///' +
                $syntheticDrivePath
        })
    foreach ($pathCase in $urlWrappedPathCases) {
        $pathFixture = New-CorrectedCanaryFixture `
            -FixtureRoot (Join-Path $tempParent (
                'path-' + $pathCase.Name.Replace(' ', '-').ToLowerInvariant()))
        $pathRun = Get-Content -Raw -LiteralPath $pathFixture.RunPath |
            ConvertFrom-Json
        $pathRun.outcome.error = [string]$pathCase.Value
        Write-Json $pathFixture.RunPath $pathRun
        Assert-ProducerRejected $pathCase.Name {
            [void](Invoke-Producer $pathFixture)
        }
    }
    $greenIndexSha = (Get-FileHash -LiteralPath $green.IndexPath -Algorithm SHA256).Hash
    $greenRepeatReceipt = Invoke-Producer $green
    $greenRepeatSha = (Get-FileHash -LiteralPath $green.IndexPath -Algorithm SHA256).Hash
    if ($greenRepeatSha -cne $greenIndexSha -or
        [string]$greenRepeatReceipt.ReleaseIndexSha256 -cne
            $greenIndexSha.ToLowerInvariant()) {
        throw 'The portable corrected-canary byte-identical reuse self-test failed.'
    }

    $immutableConflict = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'immutable-conflict')
    [void](Invoke-Producer $immutableConflict)
    [IO.File]::AppendAllText(
        $immutableConflict.IndexPath,
        " `n",
        (New-Object Text.UTF8Encoding($false)))
    $immutableConflictSha = (Get-FileHash `
        -LiteralPath $immutableConflict.IndexPath -Algorithm SHA256).Hash
    Assert-ProducerRejected 'immutable release-index replacement' {
        [void](Invoke-Producer $immutableConflict)
    }
    $immutableConflictShaAfter = (Get-FileHash `
        -LiteralPath $immutableConflict.IndexPath -Algorithm SHA256).Hash
    if ($immutableConflictShaAfter -cne $immutableConflictSha) {
        throw 'The portable corrected-canary immutable-conflict bytes changed after rejection.'
    }

    $lateDrift = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'late-drift')
    $lateDriftRun = Get-Content -Raw -LiteralPath $lateDrift.RunPath |
        ConvertFrom-Json
    $lateDriftRelative = [string](@($lateDriftRun.files | Where-Object {
        [string]$_.path -cmatch '/script\.log$'
    })[0].path)
    $lateDriftPath = Join-Path `
        $lateDrift.Bundle `
        $lateDriftRelative.Replace('/', '\')
    $lateDriftToken = [Guid]::NewGuid().ToString('N')
    $lateReady = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexReady-' + $lateDriftToken))
    $lateMutated = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexMutated-' + $lateDriftToken))
    $lateJob = $null
    $env:PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_LATE_DRIFT_TOKEN =
        $lateDriftToken
    try {
        $lateFixture = [pscustomobject]@{
            ProducerPath = $producerPath
            RunPath = $lateDrift.RunPath
        }
        $lateJob = Start-Job -ScriptBlock {
            param($Fixture)
            $succeeded = $false
            $errorText = ''
            try {
                [void](& $Fixture.ProducerPath -RunEnvelopePath $Fixture.RunPath)
                $succeeded = $true
            }
            catch {
                $errorText = $_.Exception.Message
            }
            [pscustomobject]@{
                Succeeded = $succeeded
                Error = $errorText
            }
        } -ArgumentList $lateFixture
        if (-not $lateReady.WaitOne(15000)) {
            throw 'The corrected-canary late-drift producer did not reach its bounded publication barrier.'
        }
        [IO.File]::AppendAllText(
            $lateDriftPath,
            "late publication mutation`n",
            (New-Object Text.UTF8Encoding($false)))
        [void]$lateMutated.Set()
        $lateCompleted = $lateJob | Wait-Job -Timeout 30
        if ($null -eq $lateCompleted -or $lateJob.State -cne 'Completed') {
            throw 'The corrected-canary late-drift producer did not finish after its bounded barrier.'
        }
        $lateResult = @($lateJob | Receive-Job)[-1]
        if ([bool]$lateResult.Succeeded -or
            [string]$lateResult.Error -cnotmatch
                'changed immediately before release-index publication' -or
            (Test-Path -LiteralPath $lateDrift.IndexPath)) {
            throw 'The corrected-canary late-drift publication self-test failed closed.'
        }
    }
    finally {
        [void]$lateMutated.Set()
        if ($lateJob) {
            $lateJob | Remove-Job -Force -ErrorAction SilentlyContinue
        }
        [Environment]::SetEnvironmentVariable(
            'PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_LATE_DRIFT_TOKEN',
            $null,
            [EnvironmentVariableTarget]::Process)
        $lateReady.Dispose()
        $lateMutated.Dispose()
    }

    $publicationWindow = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'publication-window')
    $publicationRun = Get-Content -Raw -LiteralPath $publicationWindow.RunPath |
        ConvertFrom-Json
    $publicationRelative = [string](@($publicationRun.files | Where-Object {
        [string]$_.path -cmatch '/script\.log$'
    })[0].path)
    $publicationInputPath = Join-Path `
        $publicationWindow.Bundle `
        $publicationRelative.Replace('/', '\')
    $publicationInputSha = (Get-FileHash `
        -LiteralPath $publicationInputPath `
        -Algorithm SHA256).Hash
    $publicationToken = [Guid]::NewGuid().ToString('N')
    $publicationReady = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexPublicationReady-' +
            $publicationToken))
    $publicationAttempted = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexPublicationAttempted-' +
            $publicationToken))
    $publicationJob = $null
    $publicationMutationRejected = $false
    $env:PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_PUBLICATION_TOKEN =
        $publicationToken
    try {
        $publicationFixture = [pscustomobject]@{
            ProducerPath = $producerPath
            RunPath = $publicationWindow.RunPath
        }
        $publicationJob = Start-Job -ScriptBlock {
            param($Fixture)
            $succeeded = $false
            $errorText = ''
            try {
                [void](& $Fixture.ProducerPath -RunEnvelopePath $Fixture.RunPath)
                $succeeded = $true
            }
            catch {
                $errorText = $_.Exception.Message
            }
            [pscustomobject]@{
                Succeeded = $succeeded
                Error = $errorText
            }
        } -ArgumentList $publicationFixture
        if (-not $publicationReady.WaitOne(15000)) {
            throw 'The corrected-canary producer did not enter its bounded publication window.'
        }
        if (-not (Test-Path -LiteralPath $publicationWindow.IndexPath -PathType Leaf)) {
            throw 'The corrected-canary publication seam opened before immutable output existed.'
        }
        try {
            [IO.File]::AppendAllText(
                $publicationInputPath,
                "forbidden publication-window mutation`n",
                (New-Object Text.UTF8Encoding($false)))
        }
        catch [IO.IOException] {
            $publicationMutationRejected = $true
        }
        catch [UnauthorizedAccessException] {
            $publicationMutationRejected = $true
        }
        [void]$publicationAttempted.Set()
        $publicationCompleted = $publicationJob | Wait-Job -Timeout 30
        if ($null -eq $publicationCompleted -or
            $publicationJob.State -cne 'Completed') {
            throw 'The corrected-canary producer did not leave its bounded publication window.'
        }
        $publicationResult = @($publicationJob | Receive-Job)[-1]
        $publicationInputShaAfter = (Get-FileHash `
            -LiteralPath $publicationInputPath `
            -Algorithm SHA256).Hash
        if (-not $publicationMutationRejected -or
            -not [bool]$publicationResult.Succeeded -or
            -not [string]::IsNullOrEmpty([string]$publicationResult.Error) -or
            $publicationInputShaAfter -cne $publicationInputSha -or
            -not (Test-Path -LiteralPath $publicationWindow.IndexPath -PathType Leaf)) {
            throw 'The corrected-canary publication-window input-lock self-test failed.'
        }
    }
    finally {
        [void]$publicationAttempted.Set()
        if ($publicationJob) {
            $publicationJob | Remove-Job -Force -ErrorAction SilentlyContinue
        }
        [Environment]::SetEnvironmentVariable(
            'PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_PUBLICATION_TOKEN',
            $null,
            [EnvironmentVariableTarget]::Process)
        $publicationReady.Dispose()
        $publicationAttempted.Dispose()
    }

    $concurrentReuse = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'concurrent-byte-identical-reuse')
    $concurrentToken = [Guid]::NewGuid().ToString('N')
    $concurrentReady = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexConcurrentReady-' +
            $concurrentToken))
    $concurrentPublished = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexConcurrentPublished-' +
            $concurrentToken))
    $reusePublicationReady = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexPublicationReady-' +
            $concurrentToken))
    $reusePublicationAttempted = New-Object Threading.EventWaitHandle(
        $false,
        [Threading.EventResetMode]::ManualReset,
        ('Local\PartisanCampaignDebugReleaseIndexPublicationAttempted-' +
            $concurrentToken))
    $concurrentJob = $null
    $concurrentExtraPath = Join-Path `
        $concurrentReuse.Bundle `
        'raw/concurrent-publication-extra.txt'
    try {
        $concurrentFixture = [pscustomobject]@{
            ProducerPath = $producerPath
            RunPath = $concurrentReuse.RunPath
            Token = $concurrentToken
        }
        $concurrentJob = Start-Job -ScriptBlock {
            param($Fixture)
            $succeeded = $false
            $errorText = ''
            try {
                $env:PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_CONCURRENT_TOKEN =
                    $Fixture.Token
                $env:PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_PUBLICATION_TOKEN =
                    $Fixture.Token
                [void](& $Fixture.ProducerPath -RunEnvelopePath $Fixture.RunPath)
                $succeeded = $true
            }
            catch {
                $errorText = $_.Exception.Message
            }
            finally {
                [Environment]::SetEnvironmentVariable(
                    'PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_CONCURRENT_TOKEN',
                    $null,
                    [EnvironmentVariableTarget]::Process)
                [Environment]::SetEnvironmentVariable(
                    'PARTISAN_CAMPAIGN_DEBUG_RELEASE_INDEX_SELFTEST_PUBLICATION_TOKEN',
                    $null,
                    [EnvironmentVariableTarget]::Process)
            }
            [pscustomobject]@{
                Succeeded = $succeeded
                Error = $errorText
            }
        } -ArgumentList $concurrentFixture
        if (-not $concurrentReady.WaitOne(15000)) {
            throw 'The corrected-canary producer did not reach its concurrent publication barrier.'
        }
        $concurrentTemporaryRows = @(Get-ChildItem `
            -LiteralPath $concurrentReuse.Bundle `
            -File `
            -Force | Where-Object {
                $_.Name -cmatch '^\.release-index\.json\.[0-9a-f]{32}\.tmp$'
            })
        if ($concurrentTemporaryRows.Count -ne 1 -or
            (Test-Path -LiteralPath $concurrentReuse.IndexPath)) {
            throw 'The concurrent publication fixture did not expose one unpublished exact-byte temporary index.'
        }
        [IO.File]::Copy(
            $concurrentTemporaryRows[0].FullName,
            $concurrentReuse.IndexPath,
            $false)
        $concurrentWinnerSha = (Get-FileHash `
            -LiteralPath $concurrentReuse.IndexPath `
            -Algorithm SHA256).Hash
        [void]$concurrentPublished.Set()
        if (-not $reusePublicationReady.WaitOne(15000)) {
            throw 'The corrected-canary concurrent reuser did not enter its publication window.'
        }
        Write-Utf8Text `
            -Path $concurrentExtraPath `
            -Text "synthetic concurrent publication drift`n"
        [void]$reusePublicationAttempted.Set()
        $concurrentCompleted = $concurrentJob | Wait-Job -Timeout 30
        if ($null -eq $concurrentCompleted -or
            $concurrentJob.State -cne 'Completed') {
            throw 'The corrected-canary concurrent reuser did not fail closed after publication drift.'
        }
        $concurrentResult = @($concurrentJob | Receive-Job)[-1]
        $concurrentWinnerShaAfter = (Get-FileHash `
            -LiteralPath $concurrentReuse.IndexPath `
            -Algorithm SHA256).Hash
        if ([bool]$concurrentResult.Succeeded -or
            [string]$concurrentResult.Error -cnotmatch
                'Run-envelope inventory and retained raw file set after publication' -or
            $concurrentWinnerShaAfter -cne $concurrentWinnerSha -or
            -not (Test-Path -LiteralPath $concurrentReuse.IndexPath -PathType Leaf)) {
            throw 'The concurrent byte-identical publication rollback self-test deleted or changed the winning index.'
        }
    }
    finally {
        [void]$concurrentPublished.Set()
        [void]$reusePublicationAttempted.Set()
        if ($concurrentJob) {
            $concurrentJob | Remove-Job -Force -ErrorAction SilentlyContinue
        }
        if (Test-Path -LiteralPath $concurrentExtraPath -PathType Leaf) {
            Remove-Item -LiteralPath $concurrentExtraPath -Force
        }
        $concurrentReady.Dispose()
        $concurrentPublished.Dispose()
        $reusePublicationReady.Dispose()
        $reusePublicationAttempted.Dispose()
    }

    $proofRed = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'proof-red') `
        -Mode 'proof-red'
    [void](Invoke-Producer $proofRed)
    $proofRedIndex = Get-Content -Raw -LiteralPath $proofRed.IndexPath | ConvertFrom-Json
    if ([string]$proofRedIndex.result.status -cne 'failed-corrected-canary' -or
        [string]$proofRedIndex.result.acceptanceDisposition -cne
            'rejected-corrected-canary' -or
        [string]$proofRedIndex.result.releaseDisposition -cne
            'replacement-required') {
        throw 'The portable corrected-canary proof-red disposition self-test failed.'
    }

    $warningRed = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'warning-red') `
        -Mode 'warning-red'
    [void](Invoke-Producer $warningRed)
    $warningRedIndex = Get-Content -Raw -LiteralPath $warningRed.IndexPath | ConvertFrom-Json
    if ([string]$warningRedIndex.result.status -cne 'failed-corrected-canary' -or
        [bool]$warningRedIndex.proof.correctedCanaryWarningContractExact) {
        throw 'The portable corrected-canary warning-red disposition self-test failed.'
    }

    $hiddenSkip = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'hidden-skip') `
        -Mode 'hidden-skip'
    [void](Invoke-Producer $hiddenSkip)
    $hiddenSkipIndex = Get-Content -Raw -LiteralPath $hiddenSkip.IndexPath |
        ConvertFrom-Json
    if ([string]$hiddenSkipIndex.result.status -cne 'failed-corrected-canary' -or
        [bool]$hiddenSkipIndex.proof.correctedCanaryAssertionSkipFree -or
        @($hiddenSkipIndex.proof.skippedAssertionIds) -cnotcontains
            'cleanup.player_marker.hidden_skip') {
        throw 'The portable corrected-canary hidden-skip rejection self-test failed.'
    }

    $focusedCertificationSwap = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'focused-certification-swap') `
        -Mode 'focused-certification-swap'
    [void](Invoke-Producer $focusedCertificationSwap)
    $focusedCertificationSwapIndex = Get-Content -Raw `
        -LiteralPath $focusedCertificationSwap.IndexPath | ConvertFrom-Json
    if ([string]$focusedCertificationSwapIndex.result.status -cne
            'failed-corrected-canary' -or
        [bool]$focusedCertificationSwapIndex.proof.focusedAssertionsCertificationExact -or
        [int]$focusedCertificationSwapIndex.proof.focusedAssertionsPassed -ne 35 -or
        [int]$focusedCertificationSwapIndex.proof.certificationRequired -ne 87 -or
        [int]$focusedCertificationSwapIndex.proof.certificationProven -ne 87) {
        throw 'The portable corrected-canary focused certification-swap self-test failed.'
    }

    $diagnosticRed = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'diagnostic-red') `
        -Mode 'diagnostic-red'
    [void](Invoke-Producer $diagnosticRed)
    $diagnosticRedIndex = Get-Content -Raw -LiteralPath $diagnosticRed.IndexPath |
        ConvertFrom-Json
    if ([string]$diagnosticRedIndex.result.status -cne 'failed-corrected-canary' -or
        [string]$diagnosticRedIndex.result.acceptanceDisposition -cne
            'rejected-corrected-canary' -or
        [string]$diagnosticRedIndex.result.releaseDisposition -cne
            'replacement-required' -or
        [int]$diagnosticRedIndex.diagnostics.unapprovedHardDiagnosticCount -ne 1) {
        throw 'The portable corrected-canary diagnostic-red disposition self-test failed.'
    }

    $tableDrivenRedContracts = @(
        [pscustomobject]@{ Mode = 'balanced-certification-swap'; Section = 'proof'; Field = 'correctedCanaryAssertionManifestExact' },
        [pscustomobject]@{ Mode = 'nonfocused-id-substitution'; Section = 'proof'; Field = 'correctedCanaryAssertionManifestExact' },
        [pscustomobject]@{ Mode = 'diff-missing'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'diff-duplicate'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'diff-renamed'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'diff-order'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'diff-arithmetic'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'diff-nonnumeric'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'diff-nonzero'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'diff-extra-line'; Section = 'proof'; Field = 'correctedCanaryStateDiffManifestExact' },
        [pscustomobject]@{ Mode = 'orphan-id-red'; Section = 'proof'; Field = 'correctedCanaryOrphanContractExact' },
        [pscustomobject]@{ Mode = 'orphan-metric-id-red'; Section = 'proof'; Field = 'correctedCanaryOrphanContractExact' },
        [pscustomobject]@{ Mode = 'orphan-metric-case-red'; Section = 'proof'; Field = 'correctedCanaryOrphanContractExact' },
        [pscustomobject]@{ Mode = 'orphan-metric-name-red'; Section = 'proof'; Field = 'correctedCanaryOrphanContractExact' },
        [pscustomobject]@{ Mode = 'orphan-metadata-red'; Section = 'proof'; Field = 'correctedCanaryOrphanContractExact' },
        [pscustomobject]@{ Mode = 'orphan-actual-red'; Section = 'proof'; Field = 'correctedCanaryOrphanContractExact' },
        [pscustomobject]@{ Mode = 'orphan-certification-red'; Section = 'proof'; Field = 'correctedCanaryOrphanContractExact' },
        [pscustomobject]@{ Mode = 'error-log-red'; Section = 'diagnostics'; Field = 'auxiliaryDiagnosticsValid' },
        [pscustomobject]@{ Mode = 'crash-log-red'; Section = 'diagnostics'; Field = 'auxiliaryDiagnosticsValid' })
    foreach ($contract in $tableDrivenRedContracts) {
        $fixture = New-CorrectedCanaryFixture `
            -FixtureRoot (Join-Path $tempParent ("table-" + $contract.Mode)) `
            -Mode $contract.Mode
        [void](Invoke-Producer $fixture)
        $redIndex = Get-Content -Raw -LiteralPath $fixture.IndexPath |
            ConvertFrom-Json
        if ([string]$redIndex.result.status -cne 'failed-corrected-canary' -or
            [bool]$redIndex.($contract.Section).($contract.Field)) {
            throw "The portable corrected-canary $($contract.Mode) table contract self-test failed."
        }
    }

    $harnessTamper = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'harness-tamper')
    $harnessTamperRun = Get-Content -Raw -LiteralPath $harnessTamper.RunPath |
        ConvertFrom-Json
    $harnessTamperRun.harness.campaignRunnerSha256 = 'f' * 64
    Write-Json $harnessTamper.RunPath $harnessTamperRun
    Assert-ProducerRejected 'harness tool hash tamper' {
        [void](Invoke-Producer $harnessTamper)
    }

    $headTamper = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'head-tamper')
    $headTamperRun = Get-Content -Raw -LiteralPath $headTamper.RunPath |
        ConvertFrom-Json
    $headTamperRun.harness.gitHead = 'b' * 40
    Write-Json $headTamper.RunPath $headTamperRun
    Assert-ProducerRejected 'harness Git HEAD tamper' {
        [void](Invoke-Producer $headTamper)
    }

    foreach ($blobField in @(
            'campaignRunnerGitBlobSha256',
            'candidateModuleGitBlobSha256',
            'releaseIndexProducerGitBlobSha256',
            'releaseDocsConsumerGitBlobSha256')) {
        $blobTamper = New-CorrectedCanaryFixture `
            -FixtureRoot (Join-Path $tempParent "blob-tamper-$blobField")
        $blobTamperRun = Get-Content -Raw -LiteralPath $blobTamper.RunPath |
            ConvertFrom-Json
        $blobTamperRun.harness.($blobField) = 'f' * 64
        Write-Json $blobTamper.RunPath $blobTamperRun
        Assert-ProducerRejected "harness $blobField tamper" {
            [void](Invoke-Producer $blobTamper)
        }
    }

    $candidateTamper = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'candidate-tamper')
    $candidateTamperRun = Get-Content -Raw -LiteralPath $candidateTamper.RunPath |
        ConvertFrom-Json
    $candidateTamperRun.launch.packageSha256 = 'f' * 64
    Write-Json $candidateTamper.RunPath $candidateTamperRun
    Assert-ProducerRejected 'candidate package identity tamper' {
        [void](Invoke-Producer $candidateTamper)
    }

    $inventoryTamper = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'inventory-tamper')
    [IO.File]::AppendAllText(
        $inventoryTamper.RawPath,
        " `n",
        (New-Object Text.UTF8Encoding($false)))
    Assert-ProducerRejected 'raw artifact hash tamper' {
        [void](Invoke-Producer $inventoryTamper)
    }

    $duplicateRunKey = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'duplicate-run-key')
    $duplicateRunText = Get-Content -Raw -LiteralPath $duplicateRunKey.RunPath
    $duplicateRunMatch = [regex]::Match(
        $duplicateRunText,
        '(?m)^(?<row>\s*"schemaVersion"\s*:\s*2\s*,\r?\n)')
    if (-not $duplicateRunMatch.Success) {
        throw 'The duplicate run-envelope key fixture could not find schemaVersion.'
    }
    $duplicateRunText = $duplicateRunText.Insert(
        $duplicateRunMatch.Index + $duplicateRunMatch.Length,
        $duplicateRunMatch.Groups['row'].Value)
    Write-Utf8Text -Path $duplicateRunKey.RunPath -Text $duplicateRunText
    Assert-ProducerRejected 'duplicate run-envelope JSON key' {
        [void](Invoke-Producer $duplicateRunKey)
    }

    $duplicateRawKey = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'duplicate-raw-key')
    $duplicateRawText = Get-Content -Raw -LiteralPath $duplicateRawKey.RawPath
    $duplicateRawMatch = [regex]::Match(
        $duplicateRawText,
        '(?m)^(?<row>\s*"m_sWorldName"\s*:\s*"HST_Dev"\s*,\r?\n)')
    if (-not $duplicateRawMatch.Success) {
        throw 'The duplicate raw-artifact key fixture could not find m_sWorldName.'
    }
    $duplicateRawText = $duplicateRawText.Insert(
        $duplicateRawMatch.Index + $duplicateRawMatch.Length,
        $duplicateRawMatch.Groups['row'].Value)
    Write-Utf8Text -Path $duplicateRawKey.RawPath -Text $duplicateRawText
    $duplicateRawRun = Get-Content -Raw -LiteralPath $duplicateRawKey.RunPath |
        ConvertFrom-Json
    $duplicateRawRelativePath = $duplicateRawKey.RawPath.Substring(
        $duplicateRawKey.Bundle.Length + 1).Replace('\', '/')
    $duplicateRawInventoryRows = @($duplicateRawRun.files | Where-Object {
        [string]$_.path -ceq $duplicateRawRelativePath
    })
    if ($duplicateRawInventoryRows.Count -ne 1) {
        throw 'The duplicate raw-artifact key fixture lacks one inventory row.'
    }
    $duplicateRawItem = Get-Item -LiteralPath $duplicateRawKey.RawPath
    $duplicateRawInventoryRows[0].length = [long]$duplicateRawItem.Length
    $duplicateRawInventoryRows[0].sha256 = (Get-FileHash `
        -LiteralPath $duplicateRawKey.RawPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    Write-Json $duplicateRawKey.RunPath $duplicateRawRun
    Assert-ProducerRejected 'duplicate raw-artifact JSON key' {
        [void](Invoke-Producer $duplicateRawKey)
    }

    $dirtyHarness = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'dirty-harness')
    $dirtyHarnessRun = Get-Content -Raw -LiteralPath $dirtyHarness.RunPath |
        ConvertFrom-Json
    $dirtyHarnessRun.harness.dirty = $true
    Write-Json $dirtyHarness.RunPath $dirtyHarnessRun
    Assert-ProducerRejected 'dirty harness identity' {
        [void](Invoke-Producer $dirtyHarness)
    }

    $orphanTamper = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'recorded-orphan-tamper')
    $orphanTamperRun = Get-Content -Raw -LiteralPath $orphanTamper.RunPath |
        ConvertFrom-Json
    $orphanTamperRun.outcome.validation.FinalOrphanActiveGroups = '1'
    Write-Json $orphanTamper.RunPath $orphanTamperRun
    Assert-ProducerRejected 'recorded final-orphan tamper' {
        [void](Invoke-Producer $orphanTamper)
    }

    $timestampTamper = New-CorrectedCanaryFixture `
        -FixtureRoot (Join-Path $tempParent 'timestamp-runtime-tamper')
    $timestampTamperRun = Get-Content -Raw -LiteralPath $timestampTamper.RunPath |
        ConvertFrom-Json
    $timestampTamperRun.outcome.runtimeSeconds = 11
    Write-Json $timestampTamper.RunPath $timestampTamperRun
    Assert-ProducerRejected 'corrected-canary timestamp/runtime tamper' {
        [void](Invoke-Producer $timestampTamper)
    }

    [pscustomobject][ordered]@{
        status = 'passed'
        schemaVersion = 2
        evidenceKind = 'packaged-campaign-debug-corrected-canary'
        policyId = 'partisan-campaign-debug-corrected-canary-v2'
        greenStatus = [string]$greenIndex.result.status
        redStatus = [string]$proofRedIndex.result.status
        caseCensus = '11/9/2/0/0/0'
        focused = '35/35'
        certification = '87/87/0/0/0/false'
        stateDiff = '18/0'
        orphan = 'true/0'
        diagnostics = '2/2/0/0'
        negativeCanaryDispositionChecks = 5
        idempotentPublicationChecks = 1
        immutableConflictChecks = 1
        lateDriftPublicationChecks = 1
        publicationWindowLockChecks = 1
        concurrentPublicationRollbackChecks = 1
        localPathNegativeChecks = $urlWrappedPathCases.Count
        stateDiffNegativeChecks = 8
        failClosedChecks = $tableDrivenRedContracts.Count
    }
}
finally {
    $resolvedTempParent = [IO.Path]::GetFullPath($tempParent)
    if ($resolvedTempParent.StartsWith(
            $expectedTempPrefix,
            [StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path -Leaf $resolvedTempParent) -like 'PartisanCorrectedCanaryV2-*' -and
        (Test-Path -LiteralPath $resolvedTempParent -PathType Container)) {
        Remove-Item -LiteralPath $resolvedTempParent -Recurse -Force
    }
}
