[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [Parameter(Mandatory = $true)]
    [string]$RuntimeAddonRoot,

    [Parameter(Mandatory = $true)]
    [string]$WorkbenchExecutable,

    [ValidateSet("delivery_pending", "physical_live_fold")]
    [string]$CutName = "delivery_pending",

    [string]$ProjectPath = "",

    [string]$WorldResource = "Worlds/HST_Dev/HST_Dev.ent",

    [string[]]$WatchedRoots = @(),

    [string[]]$SpillRoots = @(),

    [ValidateRange(30, 3600)]
    [int]$StageTimeoutSeconds = 300,

    [ValidateRange(30, 900)]
    [int]$PackTimeoutSeconds = 180,

    [ValidateRange(100, 5000)]
    [int]$PollMilliseconds = 500,

    [ValidateRange(1, 60)]
    [int]$ResultGraceSeconds = 5,

    [switch]$PreflightOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# Reuse the current packed-process, Windows job, argument-vector, ownership,
# snapshot, hashing, and cleanup implementation. The library-only seam avoids
# creating a second process-control implementation for this proof family.
$ordinaryLibrary = Join-Path $PSScriptRoot `
    "run-ordinary-campaign-persistence-proof.ps1"
. $ordinaryLibrary `
    -Executable $Executable `
    -RuntimeAddonRoot $RuntimeAddonRoot `
    -WorkbenchExecutable $WorkbenchExecutable `
    -ProjectPath $ProjectPath `
    -WorldResource $WorldResource `
    -WatchedRoots $WatchedRoots `
    -SpillRoots $SpillRoots `
    -StageTimeoutSeconds $StageTimeoutSeconds `
    -PackTimeoutSeconds $PackTimeoutSeconds `
    -PollMilliseconds $PollMilliseconds `
    -ResultGraceSeconds $ResultGraceSeconds `
    -PreflightOnly:$PreflightOnly `
    -LibraryOnly

$script:Stages = @("prepare", "recover", "replay")
$script:StageOrdinals = @{
    prepare = 0
    recover = 1
    replay = 2
}
$script:CutName = $CutName.Trim().ToLowerInvariant()
$script:CutOrdinal = @{
    delivery_pending = 0
    physical_live_fold = 1
}[$script:CutName]
if ($null -eq $script:CutOrdinal) {
    throw "Unknown exact garrison rebuild restart cut."
}
$script:PhysicalPositionToleranceMeters = 0.1
$script:MinimumPhysicalMovementMeters = 1.0
$script:MinimumPhysicalClosureMeters = 0.5
$script:ExpectedAcceptedMemberCount = 9
$script:ExpectedLivingMemberCount = 8
$script:ExpectedPreMaterializationRouteProgressMeters = 225.0
$script:ExpectedPreMaterializationRouteTotalMeters = 300.0
$script:MissionHeader = "Missions/HST_Dev.conf"
$script:ScenarioId = "{6985327711302110}Missions/HST_Dev.conf"
$script:ProjectId = "698532771130111D"
$script:OwnerMagic =
    "partisan_exact_garrison_rebuild_restart_owner_v1"
$script:GuardMagic =
    "partisan_exact_garrison_rebuild_restart_guard_v1"
$script:CarrierMagic =
    "partisan_exact_garrison_rebuild_restart_carrier_v1"
$script:ResultMagic =
    "partisan_exact_garrison_rebuild_restart_result_v1"
$script:OwnerPurpose = "exact_garrison_rebuild_external_restart"
$script:AuthorityVersion = 1
$script:EnvelopeMagic = "partisan_campaign_profile_journal_v1"
$script:EnvelopeVersion = 1
$script:SentinelVersion = 1
$script:ExpectedStageCount = 3
$script:ArtifactPrefix = "HST_ExactGarrisonRebuildRestart_"
$script:GuardBaseLeaf = "PartisanExactGarrisonRebuildRestart"
$script:GuardLeafPrefix = "PartisanExactGarrisonRebuildRestartGuard_"
$script:GuardSentinelLeaf = ".partisan-exact-garrison-rebuild-restart-owner"
$script:WorkspacePackScratchLeaf = ".tmp-native-pack"
$script:WorkspacePackScratchSentinelLeaf = ".partisan-native-pack-owner"
# Every proof that packs this checkout owns the same workspace scratch path.
# Sharing the ordinary persistence mutex closes the check-then-create race.
$script:MutexName = "Local\PartisanOrdinaryCampaignPersistenceGuard"

function Assert-RebuildOwner {
    param(
        [Parameter(Mandatory = $true)]$Owner,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld
    )

    $label = "exact garrison rebuild restart owner"
    foreach ($property in @(
        "m_sMagic", "m_iVersion", "m_sPurpose", "m_sSessionNonce",
        "m_sRunId", "m_sRequestedCut", "m_sBuildSha", "m_sBuildUtc",
        "m_sBuildLabel", "m_iCampaignSchemaVersion",
        "m_iSettingsSchemaVersion", "m_sWorld", "m_bDisposableProfile")) {
        Assert-JsonProperty $Owner $property $label
    }
    Assert-LowerHexNonce $SessionNonce "session nonce"
    if ([string]$Owner.m_sMagic -cne $script:OwnerMagic -or
        [int]$Owner.m_iVersion -ne $script:AuthorityVersion -or
        [string]$Owner.m_sPurpose -cne $script:OwnerPurpose -or
        [string]$Owner.m_sSessionNonce -cne $SessionNonce -or
        [string]$Owner.m_sRunId -cne $RunId -or
        [string]$Owner.m_sRequestedCut -cne $script:CutName -or
        [string]$Owner.m_sWorld -cne $ExpectedWorld -or
        $Owner.m_bDisposableProfile -isnot [bool] -or
        -not [bool]$Owner.m_bDisposableProfile) {
        throw "$label identity is not exact."
    }
    Assert-BuildIdentity $Owner $ExpectedBuild $label
    return $Owner
}

function Assert-RebuildGuard {
    param(
        [Parameter(Mandatory = $true)]$Guard,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$StageNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][string]$Stage,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld
    )

    $label = "$Stage exact garrison rebuild one-use guard"
    foreach ($property in @(
        "m_sMagic", "m_iVersion", "m_sSessionNonce", "m_sStageNonce",
        "m_sRunId", "m_sRequestedCut", "m_sRequestedStage",
        "m_iStageOrdinal", "m_sBuildSha", "m_sBuildUtc",
        "m_sBuildLabel", "m_iCampaignSchemaVersion",
        "m_iSettingsSchemaVersion", "m_sWorld",
        "m_bAllowCanonicalCampaignOverwrite")) {
        Assert-JsonProperty $Guard $property $label
    }
    $writeStage = $Stage -cne "replay"
    if ([string]$Guard.m_sMagic -cne $script:GuardMagic -or
        [int]$Guard.m_iVersion -ne $script:AuthorityVersion -or
        [string]$Guard.m_sSessionNonce -cne $SessionNonce -or
        [string]$Guard.m_sStageNonce -cne $StageNonce -or
        [string]$Guard.m_sRunId -cne $RunId -or
        [string]$Guard.m_sRequestedCut -cne $script:CutName -or
        [string]$Guard.m_sRequestedStage -cne $Stage -or
        [int]$Guard.m_iStageOrdinal -ne $script:StageOrdinals[$Stage] -or
        [string]$Guard.m_sWorld -cne $ExpectedWorld -or
        $Guard.m_bAllowCanonicalCampaignOverwrite -isnot [bool] -or
        [bool]$Guard.m_bAllowCanonicalCampaignOverwrite -ne $writeStage) {
        throw "$label identity is not exact."
    }
    Assert-BuildIdentity $Guard $ExpectedBuild $label
    return $Guard
}

function New-RebuildOwnerValue {
    param(
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)]$BuildIdentity,
        [Parameter(Mandatory = $true)][string]$World
    )

    return [ordered]@{
        m_sMagic = $script:OwnerMagic
        m_iVersion = $script:AuthorityVersion
        m_sPurpose = $script:OwnerPurpose
        m_sSessionNonce = $SessionNonce
        m_sRunId = $RunId
        m_sRequestedCut = $script:CutName
        m_sBuildSha = $BuildIdentity.BuildSha
        m_sBuildUtc = $BuildIdentity.BuildUtc
        m_sBuildLabel = $BuildIdentity.BuildLabel
        m_iCampaignSchemaVersion = $BuildIdentity.CampaignSchemaVersion
        m_iSettingsSchemaVersion = $BuildIdentity.SettingsSchemaVersion
        m_sWorld = $World
        m_bDisposableProfile = $true
    }
}

function Get-RebuildStageArgumentVector {
    param(
        [Parameter(Mandatory = $true)][string]$RuntimeAddonPath,
        [Parameter(Mandatory = $true)][string]$PackedAddonsParent,
        [Parameter(Mandatory = $true)][string]$ServerConfigPath,
        [Parameter(Mandatory = $true)][string]$ProfileRoot,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$StageNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][string]$Stage
    )

    if ($script:Stages -cnotcontains $Stage) {
        throw "Unknown exact garrison rebuild restart stage."
    }
    $baseProject = Join-Path $RuntimeAddonPath "data\ArmaReforger.gproj"
    if (-not (Test-Path -LiteralPath $baseProject -PathType Leaf)) {
        throw "The packed base game project is unavailable."
    }
    $arguments = @(
        "-addonsDir", ($RuntimeAddonPath + "," + $PackedAddonsParent),
        "-gproj", $baseProject,
        "-config", $ServerConfigPath,
        "-profile", $ProfileRoot,
        "-rpl-timeout-disable",
        "-noThrow",
        "-backendLocalStorage",
        "-maxFPS", "30",
        "-hstExactGarrisonRebuildRestartProof", "true",
        "-hstExactGarrisonRebuildRestartCut", $script:CutName,
        "-hstExactGarrisonRebuildRestartStage", $Stage,
        "-hstExactGarrisonRebuildRestartRunId", $RunId,
        "-hstExactGarrisonRebuildRestartSessionNonce", $SessionNonce,
        "-hstExactGarrisonRebuildRestartStageNonce", $StageNonce)

    foreach ($forbidden in @(
        "-world", "-addons", "-forceupdate", "-autoshutdown",
        "-keepSessionSave", "-loadSessionSave")) {
        if ($arguments -ccontains $forbidden) {
            throw "$Stage contains forbidden argument $forbidden."
        }
    }
    return $arguments
}

function Get-ProofVectorComponents {
    param([Parameter(Mandatory = $true)]$Value)

    $serialized = if ($Value -is [string]) {
        [string]$Value
    }
    else {
        $Value | ConvertTo-Json -Compress -Depth 4
    }
    $pattern = '[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?'
    $matches = [regex]::Matches($serialized, $pattern)
    if ($matches.Count -ne 3) {
        throw "Proof vector does not contain exactly three finite coordinates."
    }
    $components = New-Object double[] 3
    for ($index = 0; $index -lt 3; $index++) {
        $coordinate = 0.0
        if (-not [double]::TryParse(
            $matches[$index].Value,
            [Globalization.NumberStyles]::Float,
            [Globalization.CultureInfo]::InvariantCulture,
            [ref]$coordinate) -or
            [double]::IsNaN($coordinate) -or
            [double]::IsInfinity($coordinate)) {
            throw "Proof vector contains a non-finite coordinate."
        }
        $components[$index] = $coordinate
    }
    return $components
}

function Test-ProofVectorNonZero {
    param([Parameter(Mandatory = $true)]$Value)

    $components = @(Get-ProofVectorComponents $Value)
    return @($components | Where-Object {
        [Math]::Abs([double]$_) -gt 0.0001
    }).Count -gt 0
}

function Test-ProofVectorZero {
    param([Parameter(Mandatory = $true)]$Value)

    $components = @(Get-ProofVectorComponents $Value)
    return @($components | Where-Object {
        [Math]::Abs([double]$_) -ge 0.01
    }).Count -eq 0
}

function Get-ProofVectorDistance2D {
    param(
        [Parameter(Mandatory = $true)]$Left,
        [Parameter(Mandatory = $true)]$Right
    )

    $leftComponents = @(Get-ProofVectorComponents $Left)
    $rightComponents = @(Get-ProofVectorComponents $Right)
    $x = [double]$leftComponents[0] - [double]$rightComponents[0]
    $z = [double]$leftComponents[2] - [double]$rightComponents[2]
    return [Math]::Sqrt($x * $x + $z * $z)
}

function Test-ProofPositionsMatch {
    param(
        [Parameter(Mandatory = $true)]$Left,
        [Parameter(Mandatory = $true)]$Right,
        [double]$ToleranceMeters = $script:PhysicalPositionToleranceMeters
    )

    $leftComponents = @(Get-ProofVectorComponents $Left)
    $rightComponents = @(Get-ProofVectorComponents $Right)
    for ($index = 0; $index -lt 3; $index++) {
        if ([Math]::Abs(
            [double]$leftComponents[$index] -
            [double]$rightComponents[$index]) -gt $ToleranceMeters) {
            return $false
        }
    }
    return $true
}

function Test-RebuildStageCutArgumentVector {
    param([Parameter(Mandatory = $true)][object[]]$Arguments)

    $cutSwitch = "-hstExactGarrisonRebuildRestartCut"
    $indices = @()
    for ($index = 0; $index -lt $Arguments.Count; $index++) {
        if ([string]$Arguments[$index] -ceq $cutSwitch) {
            $indices += $index
        }
    }
    return $indices.Count -eq 1 -and
        $indices[0] + 1 -lt $Arguments.Count -and
        [string]$Arguments[$indices[0] + 1] -ceq $script:CutName
}

function Assert-RebuildExpectation {
    param(
        [Parameter(Mandatory = $true)]$Expectation,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $properties = @(
        "m_sOrderId", "m_sOperationId", "m_sManifestId",
        "m_sManifestHash", "m_sBatchId", "m_sGroupId",
        "m_sProjectionId", "m_sForceId", "m_sFactionKey",
        "m_sSourceZoneId", "m_sTargetZoneId",
        "m_sExpectedSourceOwnerFactionKey",
        "m_iExpectedSourceOwnershipRevision",
        "m_sExpectedTargetOwnerFactionKey",
        "m_iExpectedTargetOwnershipRevision", "m_sDebitMutationId",
        "m_sDeliverySettlementKind", "m_sDeliverySettlementId",
        "m_sRefundMutationId", "m_iAttackCost", "m_iSupportCost",
        "m_iExpectedAttackPool", "m_iExpectedSupportPool",
        "m_iExpectedPendingPoolRevision",
        "m_iExpectedPendingPoolOperationalMutationCount",
        "m_iAcceptedMemberCount", "m_iLivingMemberCount",
        "m_sLivingSlotFingerprint", "m_sConfirmedCasualtySlotId",
        "m_sCasualtyTombstoneFingerprint",
        "m_iExpectedAggregateInfantry",
        "m_iExpectedAuthoritativePendingInfantry",
        "m_iExpectedAuthoritativeDeliveredInfantry")
    foreach ($property in $properties) {
        Assert-JsonProperty $Expectation $property $Label
    }
    foreach ($property in @(
        "m_sOrderId", "m_sOperationId", "m_sManifestId",
        "m_sManifestHash", "m_sBatchId", "m_sGroupId",
        "m_sProjectionId", "m_sForceId", "m_sFactionKey",
        "m_sSourceZoneId", "m_sTargetZoneId",
        "m_sExpectedSourceOwnerFactionKey",
        "m_sExpectedTargetOwnerFactionKey", "m_sDebitMutationId",
        "m_sDeliverySettlementKind", "m_sDeliverySettlementId",
        "m_sRefundMutationId", "m_sLivingSlotFingerprint",
        "m_sConfirmedCasualtySlotId",
        "m_sCasualtyTombstoneFingerprint")) {
        if ([string]::IsNullOrWhiteSpace([string]$Expectation.$property)) {
            throw "$Label identity is incomplete."
        }
    }

    $accepted = [int]$Expectation.m_iAcceptedMemberCount
    $living = [int]$Expectation.m_iLivingMemberCount
    $aggregate = [int]$Expectation.m_iExpectedAggregateInfantry
    $settlementId = [string]$Expectation.m_sDeliverySettlementId
    if ([string]$Expectation.m_sSourceZoneId -ceq
            [string]$Expectation.m_sTargetZoneId -or
        [int]$Expectation.m_iExpectedSourceOwnershipRevision -le 0 -or
        [int]$Expectation.m_iExpectedTargetOwnershipRevision -le 0 -or
        [int]$Expectation.m_iAttackCost -ne 0 -or
        [int]$Expectation.m_iSupportCost -le 0 -or
        [int]$Expectation.m_iExpectedAttackPool -lt 0 -or
        [int]$Expectation.m_iExpectedSupportPool -lt 0 -or
        [int]$Expectation.m_iExpectedPendingPoolRevision -le 0 -or
        [int]$Expectation.m_iExpectedPendingPoolOperationalMutationCount -ne 1 -or
        [string]$Expectation.m_sRefundMutationId -cne
            ("enemy_resource_refund_" + $settlementId) -or
        $accepted -ne $script:ExpectedAcceptedMemberCount -or
        $living -ne $script:ExpectedLivingMemberCount -or
        $living -ne ($accepted - 1) -or
        -not ([string]$Expectation.m_sCasualtyTombstoneFingerprint).StartsWith(
            ([string]$Expectation.m_sConfirmedCasualtySlotId) + "|") -or
        $aggregate -lt 0 -or
        [int]$Expectation.m_iExpectedAuthoritativePendingInfantry -ne
            ($aggregate + $living) -or
        [int]$Expectation.m_iExpectedAuthoritativeDeliveredInfantry -ne
            ($aggregate + $living)) {
        throw "$Label resource, casualty, or garrison authority is not exact."
    }
    return $Expectation
}

function Assert-RebuildCarrier {
    param(
        [Parameter(Mandatory = $true)]$Carrier,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld
    )

    $label = "$($script:CutName) exact garrison rebuild carrier"
    foreach ($property in @(
        "m_sMagic", "m_sSessionNonce", "m_sRunId", "m_sBuildSha",
        "m_sBuildUtc", "m_sBuildLabel", "m_iCampaignSchemaVersion",
        "m_iSettingsSchemaVersion", "m_sWorld", "m_sCutName", "m_iCut",
        "m_Expectation", "m_iPreparedElapsedSecond",
        "m_fPreMaterializationRouteProgressMeters",
        "m_fPreMaterializationRouteTotalDistanceMeters",
        "m_fPreparedRouteProgressMeters",
        "m_fPreparedRouteTotalDistanceMeters",
        "m_vPreparedStrategicPosition",
        "m_iExpectedPhysicalRootCount",
        "m_iExpectedPhysicalAdapterHandleCount",
        "m_iExpectedPhysicalRuntimeMemberCount",
        "m_iLivePositionSampleCount", "m_vInitialLivePosition",
        "m_vFinalLivePosition", "m_fInitialDistanceToTargetMeters",
        "m_fFinalDistanceToTargetMeters", "m_fLiveMovementMeters",
        "m_fDistanceClosedMeters", "m_vFoldPosition",
        "m_fFoldRouteProgressMeters", "m_bPhysicalMovementExact",
        "m_bPhysicalFoldExact",
        "m_sPreparedSemanticFingerprint")) {
        Assert-JsonProperty $Carrier $property $label
    }
    foreach ($property in @(
        "m_bPhysicalMovementExact", "m_bPhysicalFoldExact")) {
        if ($Carrier.$property -isnot [bool]) {
            throw "$label property $property is not a JSON boolean."
        }
    }
    if ([string]$Carrier.m_sMagic -cne $script:CarrierMagic -or
        [string]$Carrier.m_sSessionNonce -cne $SessionNonce -or
        [string]$Carrier.m_sRunId -cne $RunId -or
        [string]$Carrier.m_sWorld -cne $ExpectedWorld -or
        [string]$Carrier.m_sCutName -cne $script:CutName -or
        [int]$Carrier.m_iCut -ne $script:CutOrdinal) {
        throw "$label identity is not exact."
    }
    Assert-BuildIdentity $Carrier $ExpectedBuild $label
    [void](Assert-RebuildExpectation `
        -Expectation $Carrier.m_Expectation `
        -Label "$label expectation")

    $progress = [double]$Carrier.m_fPreparedRouteProgressMeters
    $total = [double]$Carrier.m_fPreparedRouteTotalDistanceMeters
    $routeShapeExact = if ($script:CutName -ceq "delivery_pending") {
        [Math]::Abs(
            $progress -
            $script:ExpectedPreMaterializationRouteProgressMeters) -le
            $script:PhysicalPositionToleranceMeters -and
        [Math]::Abs(
            $total -
            $script:ExpectedPreMaterializationRouteTotalMeters) -le
            $script:PhysicalPositionToleranceMeters
    }
    else {
        [Math]::Abs($progress) -le
            $script:PhysicalPositionToleranceMeters -and $total -gt 0.0
    }
    if ([int]$Carrier.m_iPreparedElapsedSecond -le 0 -or
        [double]::IsNaN($progress) -or [double]::IsInfinity($progress) -or
        [double]::IsNaN($total) -or [double]::IsInfinity($total) -or
        -not $routeShapeExact -or
        -not (Test-ProofVectorNonZero $Carrier.m_vPreparedStrategicPosition) -or
        [string]::IsNullOrWhiteSpace(
            [string]$Carrier.m_sPreparedSemanticFingerprint)) {
        throw "$label route or fingerprint authority is not exact."
    }

    $physicalMetrics = @(
        [double]$Carrier.m_fInitialDistanceToTargetMeters,
        [double]$Carrier.m_fFinalDistanceToTargetMeters,
        [double]$Carrier.m_fLiveMovementMeters,
        [double]$Carrier.m_fDistanceClosedMeters,
        [double]$Carrier.m_fFoldRouteProgressMeters)
    if (@($physicalMetrics | Where-Object {
        [double]::IsNaN($_) -or [double]::IsInfinity($_)
    }).Count -ne 0) {
        throw "$label contains a non-finite physical metric."
    }

    if ($script:CutName -ceq "delivery_pending") {
        if ([Math]::Abs(
                [double]$Carrier.m_fPreMaterializationRouteProgressMeters) -gt
                $script:PhysicalPositionToleranceMeters -or
            [Math]::Abs(
                [double]$Carrier.m_fPreMaterializationRouteTotalDistanceMeters) -gt
                $script:PhysicalPositionToleranceMeters -or
            [int]$Carrier.m_iExpectedPhysicalRootCount -ne 0 -or
            [int]$Carrier.m_iExpectedPhysicalAdapterHandleCount -ne 0 -or
            [int]$Carrier.m_iExpectedPhysicalRuntimeMemberCount -ne 0 -or
            [int]$Carrier.m_iLivePositionSampleCount -ne 0 -or
            -not (Test-ProofVectorZero $Carrier.m_vInitialLivePosition) -or
            -not (Test-ProofVectorZero $Carrier.m_vFinalLivePosition) -or
            -not (Test-ProofVectorZero $Carrier.m_vFoldPosition) -or
            @($physicalMetrics | Where-Object {
                [Math]::Abs([double]$_) -gt 0.0001
            }).Count -ne 0 -or
            [bool]$Carrier.m_bPhysicalMovementExact -or
            [bool]$Carrier.m_bPhysicalFoldExact) {
            throw "$label physical evidence must remain exactly zero and false."
        }
    }
    else {
        $living = [int]$Carrier.m_Expectation.m_iLivingMemberCount
        $measuredMovement = Get-ProofVectorDistance2D `
            -Left $Carrier.m_vInitialLivePosition `
            -Right $Carrier.m_vFinalLivePosition
        $measuredClosure =
            [double]$Carrier.m_fInitialDistanceToTargetMeters -
            [double]$Carrier.m_fFinalDistanceToTargetMeters
        if ([Math]::Abs(
                [double]$Carrier.m_fPreMaterializationRouteProgressMeters -
                $script:ExpectedPreMaterializationRouteProgressMeters) -gt
                $script:PhysicalPositionToleranceMeters -or
            [Math]::Abs(
                [double]$Carrier.m_fPreMaterializationRouteTotalDistanceMeters -
                $script:ExpectedPreMaterializationRouteTotalMeters) -gt
                $script:PhysicalPositionToleranceMeters -or
            [Math]::Abs($progress) -gt
                $script:PhysicalPositionToleranceMeters -or
            [Math]::Abs(
                $total -
                [double]$Carrier.m_fFinalDistanceToTargetMeters) -gt
                $script:PhysicalPositionToleranceMeters -or
            [int]$Carrier.m_iExpectedPhysicalRootCount -ne 1 -or
            [int]$Carrier.m_iExpectedPhysicalAdapterHandleCount -ne
                ($living + 1) -or
            [int]$Carrier.m_iExpectedPhysicalRuntimeMemberCount -ne $living -or
            [int]$Carrier.m_iLivePositionSampleCount -lt 2 -or
            [int]$Carrier.m_iLivePositionSampleCount -gt 64 -or
            -not (Test-ProofVectorNonZero $Carrier.m_vInitialLivePosition) -or
            -not (Test-ProofVectorNonZero $Carrier.m_vFinalLivePosition) -or
            -not (Test-ProofVectorNonZero $Carrier.m_vFoldPosition) -or
            $measuredMovement -lt $script:MinimumPhysicalMovementMeters -or
            [double]$Carrier.m_fLiveMovementMeters -lt
                $script:MinimumPhysicalMovementMeters -or
            [Math]::Abs(
                [double]$Carrier.m_fLiveMovementMeters -
                $measuredMovement) -gt
                $script:PhysicalPositionToleranceMeters -or
            [double]$Carrier.m_fInitialDistanceToTargetMeters -le 0.0 -or
            [double]$Carrier.m_fFinalDistanceToTargetMeters -lt 0.0 -or
            $measuredClosure -lt $script:MinimumPhysicalClosureMeters -or
            [double]$Carrier.m_fDistanceClosedMeters -lt
                $script:MinimumPhysicalClosureMeters -or
            [Math]::Abs(
                [double]$Carrier.m_fDistanceClosedMeters -
                $measuredClosure) -gt
                $script:PhysicalPositionToleranceMeters -or
            -not (Test-ProofPositionsMatch `
                -Left $Carrier.m_vFoldPosition `
                -Right $Carrier.m_vFinalLivePosition) -or
            -not (Test-ProofPositionsMatch `
                -Left $Carrier.m_vFoldPosition `
                -Right $Carrier.m_vPreparedStrategicPosition) -or
            [Math]::Abs(
                [double]$Carrier.m_fFoldRouteProgressMeters -
                $progress) -gt $script:PhysicalPositionToleranceMeters -or
            -not [bool]$Carrier.m_bPhysicalMovementExact -or
            -not [bool]$Carrier.m_bPhysicalFoldExact) {
            throw "$label native counts, movement, closure, or fold authority is not exact."
        }
    }
    return $Carrier
}

function Assert-RebuildResult {
    param(
        [Parameter(Mandatory = $true)]$Result,
        [Parameter(Mandatory = $true)]$Carrier,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$StageNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][string]$Stage,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld,
        [AllowEmptyString()][string]$ExpectedDeliveredFingerprint = ""
    )

    $label = "$Stage exact garrison rebuild restart result"
    $properties = @(
        "m_sMagic", "m_sSessionNonce", "m_sStageNonce", "m_sRunId",
        "m_sStage", "m_bSuccess", "m_sBuildSha", "m_sBuildUtc",
        "m_sBuildLabel", "m_iCampaignSchemaVersion",
        "m_iSettingsSchemaVersion", "m_sWorld", "m_sCutName", "m_iCut",
        "m_bRestored", "m_bStartupReconcileChanged", "m_bSourceExact",
        "m_bContinuationExact", "m_bSameStateSemanticNoOp",
        "m_bRuntimeClaimantsZero", "m_bPersistedReadBackExact",
        "m_bPreparedCutExact", "m_bCasualtyContinuityExact",
        "m_bDeliveryReceiptExact", "m_bHeldGarrisonExact",
        "m_bAggregateNotDoubleCounted", "m_bResourceExactlyOnce",
        "m_bPhysicalBindingsExact", "m_bPhysicalMovementExact",
        "m_bPhysicalFoldExact", "m_iPhysicalRootCount",
        "m_iPhysicalAdapterHandleCount", "m_iPhysicalRuntimeMemberCount",
        "m_iLivePositionSampleCount", "m_vInitialLivePosition",
        "m_vFinalLivePosition", "m_fInitialDistanceToTargetMeters",
        "m_fFinalDistanceToTargetMeters", "m_fLiveMovementMeters",
        "m_fDistanceClosedMeters", "m_vFoldPosition",
        "m_fFoldRouteProgressMeters",
        "m_fProgressBeforeMeters", "m_fProgressAfterMeters",
        "m_sSourceSemanticFingerprint", "m_sFinalSemanticFingerprint",
        "m_sEvidence")
    foreach ($property in $properties) {
        Assert-JsonProperty $Result $property $label
    }
    foreach ($property in @($properties | Where-Object {
        $_ -clike "m_b*"
    })) {
        if ($Result.$property -isnot [bool]) {
            throw "$label property $property is not a JSON boolean."
        }
    }
    if ([string]$Result.m_sMagic -cne $script:ResultMagic -or
        [string]$Result.m_sSessionNonce -cne $SessionNonce -or
        [string]$Result.m_sStageNonce -cne $StageNonce -or
        [string]$Result.m_sRunId -cne $RunId -or
        [string]$Result.m_sStage -cne $Stage -or
        [string]$Result.m_sWorld -cne $ExpectedWorld -or
        [string]$Result.m_sCutName -cne $script:CutName -or
        [int]$Result.m_iCut -ne $script:CutOrdinal -or
        [string]::IsNullOrWhiteSpace([string]$Result.m_sEvidence)) {
        throw "$label identity is not exact."
    }
    Assert-BuildIdentity $Result $ExpectedBuild $label
    if (-not [bool]$Result.m_bSuccess) {
        $safeEvidence = ConvertTo-SafeEvidenceLine `
            -Line ([string]$Result.m_sEvidence)
        if ($safeEvidence.Length -gt 260) {
            $safeEvidence = $safeEvidence.Substring(0, 260)
        }
        # Keep the stage receipt ahead of the guarded process diagnostics. The
        # outer safety formatter bounds the complete line, so front-loading a
        # compact receipt preserves the actual failed invariants and evidence.
        $failureFlags = (
            ("src={0},rst={1},start={2},cont={3},noop={4}," +
                "claim={5},read={6},cut={7},cas={8},receipt={9}," +
                "held={10},agg={11},res={12},phys={13}/{14}/{15}," +
                "samples={16},move={17},close={18},p={19}->{20}") -f
            [int][bool]$Result.m_bSourceExact,
            [int][bool]$Result.m_bRestored,
            [int][bool]$Result.m_bStartupReconcileChanged,
            [int][bool]$Result.m_bContinuationExact,
            [int][bool]$Result.m_bSameStateSemanticNoOp,
            [int][bool]$Result.m_bRuntimeClaimantsZero,
            [int][bool]$Result.m_bPersistedReadBackExact,
            [int][bool]$Result.m_bPreparedCutExact,
            [int][bool]$Result.m_bCasualtyContinuityExact,
            [int][bool]$Result.m_bDeliveryReceiptExact,
            [int][bool]$Result.m_bHeldGarrisonExact,
            [int][bool]$Result.m_bAggregateNotDoubleCounted,
            [int][bool]$Result.m_bResourceExactlyOnce,
            [int]$Result.m_iPhysicalRootCount,
            [int]$Result.m_iPhysicalAdapterHandleCount,
            [int]$Result.m_iPhysicalRuntimeMemberCount,
            [int]$Result.m_iLivePositionSampleCount,
            [Math]::Round([double]$Result.m_fLiveMovementMeters, 1),
            [Math]::Round([double]$Result.m_fDistanceClosedMeters, 1),
            [Math]::Round([double]$Result.m_fProgressBeforeMeters, 1),
            [Math]::Round([double]$Result.m_fProgressAfterMeters, 1))
        throw "$label failed | $failureFlags | evidence=$safeEvidence"
    }

    $source = [string]$Result.m_sSourceSemanticFingerprint
    $final = [string]$Result.m_sFinalSemanticFingerprint
    $prepared = [string]$Carrier.m_sPreparedSemanticFingerprint
    $before = [double]$Result.m_fProgressBeforeMeters
    $after = [double]$Result.m_fProgressAfterMeters
    $physicalMetrics = @(
        [double]$Result.m_fInitialDistanceToTargetMeters,
        [double]$Result.m_fFinalDistanceToTargetMeters,
        [double]$Result.m_fLiveMovementMeters,
        [double]$Result.m_fDistanceClosedMeters,
        [double]$Result.m_fFoldRouteProgressMeters)
    if (-not [bool]$Result.m_bSourceExact -or
        -not [bool]$Result.m_bRuntimeClaimantsZero -or
        -not [bool]$Result.m_bPersistedReadBackExact -or
        -not [bool]$Result.m_bPreparedCutExact -or
        -not [bool]$Result.m_bCasualtyContinuityExact -or
        [int]$Result.m_iPhysicalRootCount -ne
            [int]$Carrier.m_iExpectedPhysicalRootCount -or
        [int]$Result.m_iPhysicalAdapterHandleCount -ne
            [int]$Carrier.m_iExpectedPhysicalAdapterHandleCount -or
        [int]$Result.m_iPhysicalRuntimeMemberCount -ne
            [int]$Carrier.m_iExpectedPhysicalRuntimeMemberCount -or
        [string]::IsNullOrWhiteSpace($source) -or
        [string]::IsNullOrWhiteSpace($final) -or
        [double]::IsNaN($before) -or [double]::IsInfinity($before) -or
        [double]::IsNaN($after) -or [double]::IsInfinity($after) -or
        @($physicalMetrics | Where-Object {
            [double]::IsNaN($_) -or [double]::IsInfinity($_)
        }).Count -ne 0) {
        throw "$label omitted common exact-state authority."
    }

    if ($script:CutName -ceq "delivery_pending") {
        if ([int]$Result.m_iLivePositionSampleCount -ne 0 -or
            -not (Test-ProofVectorZero $Result.m_vInitialLivePosition) -or
            -not (Test-ProofVectorZero $Result.m_vFinalLivePosition) -or
            -not (Test-ProofVectorZero $Result.m_vFoldPosition) -or
            @($physicalMetrics | Where-Object {
                [Math]::Abs([double]$_) -gt 0.0001
            }).Count -ne 0 -or
            [bool]$Result.m_bPhysicalBindingsExact -or
            [bool]$Result.m_bPhysicalMovementExact -or
            [bool]$Result.m_bPhysicalFoldExact) {
            throw "$label delivery-pending physical evidence is not zero and false."
        }
    }
    else {
        if ([int]$Result.m_iLivePositionSampleCount -ne
                [int]$Carrier.m_iLivePositionSampleCount -or
            -not (Test-ProofPositionsMatch `
                -Left $Result.m_vInitialLivePosition `
                -Right $Carrier.m_vInitialLivePosition) -or
            -not (Test-ProofPositionsMatch `
                -Left $Result.m_vFinalLivePosition `
                -Right $Carrier.m_vFinalLivePosition) -or
            -not (Test-ProofPositionsMatch `
                -Left $Result.m_vFoldPosition `
                -Right $Carrier.m_vFoldPosition) -or
            [Math]::Abs(
                [double]$Result.m_fInitialDistanceToTargetMeters -
                [double]$Carrier.m_fInitialDistanceToTargetMeters) -gt
                $script:PhysicalPositionToleranceMeters -or
            [Math]::Abs(
                [double]$Result.m_fFinalDistanceToTargetMeters -
                [double]$Carrier.m_fFinalDistanceToTargetMeters) -gt
                $script:PhysicalPositionToleranceMeters -or
            [double]$Result.m_fLiveMovementMeters -lt
                $script:MinimumPhysicalMovementMeters -or
            [Math]::Abs(
                [double]$Result.m_fLiveMovementMeters -
                [double]$Carrier.m_fLiveMovementMeters) -gt
                $script:PhysicalPositionToleranceMeters -or
            [double]$Result.m_fDistanceClosedMeters -lt
                $script:MinimumPhysicalClosureMeters -or
            [Math]::Abs(
                [double]$Result.m_fDistanceClosedMeters -
                [double]$Carrier.m_fDistanceClosedMeters) -gt
                $script:PhysicalPositionToleranceMeters -or
            [Math]::Abs(
                [double]$Result.m_fFoldRouteProgressMeters -
                [double]$Carrier.m_fFoldRouteProgressMeters) -gt
                $script:PhysicalPositionToleranceMeters -or
            [Math]::Abs(
                [double]$Result.m_fFoldRouteProgressMeters) -gt
                $script:PhysicalPositionToleranceMeters -or
            -not [bool]$Result.m_bPhysicalBindingsExact -or
            [bool]$Result.m_bPhysicalMovementExact -ne
                [bool]$Carrier.m_bPhysicalMovementExact -or
            [bool]$Result.m_bPhysicalFoldExact -ne
                [bool]$Carrier.m_bPhysicalFoldExact) {
            throw "$label physical carrier evidence is not exact."
        }
    }

    if ($Stage -ceq "prepare") {
        if ([bool]$Result.m_bRestored -or
            [bool]$Result.m_bStartupReconcileChanged -or
            [bool]$Result.m_bContinuationExact -or
            [bool]$Result.m_bSameStateSemanticNoOp -or
            [bool]$Result.m_bDeliveryReceiptExact -or
            [bool]$Result.m_bHeldGarrisonExact -or
            [bool]$Result.m_bAggregateNotDoubleCounted -or
            [bool]$Result.m_bResourceExactlyOnce -or
            $source -cne $prepared -or $final -cne $prepared -or
            [Math]::Abs($before -
                [double]$Carrier.m_fPreparedRouteProgressMeters) -gt 0.1 -or
            [Math]::Abs($after - $before) -gt 0.1) {
            throw "$label violates the fresh prepared-cut invariant."
        }
    }
    elseif ($Stage -ceq "recover") {
        if (-not [bool]$Result.m_bRestored -or
            -not [bool]$Result.m_bContinuationExact -or
            [bool]$Result.m_bSameStateSemanticNoOp -or
            -not [bool]$Result.m_bDeliveryReceiptExact -or
            -not [bool]$Result.m_bHeldGarrisonExact -or
            -not [bool]$Result.m_bAggregateNotDoubleCounted -or
            -not [bool]$Result.m_bResourceExactlyOnce -or
            $source -cne $prepared -or $final -ceq $prepared -or
            [Math]::Abs($before -
                [double]$Carrier.m_fPreparedRouteProgressMeters) -gt 0.1 -or
            $after -le $before -or
            [Math]::Abs($after -
                [double]$Carrier.m_fPreparedRouteTotalDistanceMeters) -gt 0.1) {
            throw "$label violates the one-time delivery continuation invariant."
        }
    }
    else {
        if (-not [bool]$Result.m_bRestored -or
            [bool]$Result.m_bStartupReconcileChanged -or
            [bool]$Result.m_bContinuationExact -or
            -not [bool]$Result.m_bSameStateSemanticNoOp -or
            -not [bool]$Result.m_bDeliveryReceiptExact -or
            -not [bool]$Result.m_bHeldGarrisonExact -or
            -not [bool]$Result.m_bAggregateNotDoubleCounted -or
            -not [bool]$Result.m_bResourceExactlyOnce -or
            [string]::IsNullOrWhiteSpace($ExpectedDeliveredFingerprint) -or
            $source -cne $ExpectedDeliveredFingerprint -or
            $final -cne $ExpectedDeliveredFingerprint -or
            [Math]::Abs($before -
                [double]$Carrier.m_fPreparedRouteTotalDistanceMeters) -gt 0.1 -or
            [Math]::Abs($after - $before) -gt 0.1) {
            throw "$label violates the delivered-state semantic no-op invariant."
        }
    }
    return $Result
}

function Read-RebuildJournalEnvelope {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][int]$ExpectedGeneration,
        [Parameter(Mandatory = $true)][int]$ExpectedSchema,
        [Parameter(Mandatory = $true)][int]$ExpectedPreviousGeneration,
        [AllowEmptyString()][string]$ExpectedPreviousFingerprint = ""
    )

    $label = "campaign profile journal generation $ExpectedGeneration"
    $value = Read-JsonArtifact -Path $Path
    foreach ($property in @(
        "m_sMagic", "m_iEnvelopeVersion", "m_iGeneration",
        "m_iSnapshotSchemaVersion", "m_sSnapshotFingerprint",
        "m_iPreviousGeneration", "m_sPreviousSnapshotFingerprint",
        "m_sSnapshotPayload")) {
        Assert-JsonProperty $value $property $label
    }
    if ([string]$value.m_sMagic -cne $script:EnvelopeMagic -or
        [int]$value.m_iEnvelopeVersion -ne $script:EnvelopeVersion -or
        [int]$value.m_iGeneration -ne $ExpectedGeneration -or
        [int]$value.m_iSnapshotSchemaVersion -ne $ExpectedSchema -or
        [int]$value.m_iPreviousGeneration -ne $ExpectedPreviousGeneration -or
        [string]$value.m_sPreviousSnapshotFingerprint -cne
            $ExpectedPreviousFingerprint -or
        [string]::IsNullOrWhiteSpace([string]$value.m_sSnapshotPayload)) {
        $previousFingerprintExact =
            [string]$value.m_sPreviousSnapshotFingerprint -ceq
                $ExpectedPreviousFingerprint
        throw (
            "$label envelope metadata is not exact | " +
            "magic/version/generation/schema/previous/previous-fingerprint/payload " +
            "{0}/{1}/{2}/{3}/{4}/{5}/{6}" -f
            ([int]([string]$value.m_sMagic -ceq $script:EnvelopeMagic)),
            [int]$value.m_iEnvelopeVersion,
            [int]$value.m_iGeneration,
            [int]$value.m_iSnapshotSchemaVersion,
            [int]$value.m_iPreviousGeneration,
            [int]$previousFingerprintExact,
            [int](-not [string]::IsNullOrWhiteSpace(
                [string]$value.m_sSnapshotPayload)))
    }
    $fingerprint = [string]$value.m_sSnapshotFingerprint
    if ($fingerprint -cnotmatch
        '^uuidv8-sha256-v1:[1-9][0-9]*:[0-9a-f]{8}-[0-9a-f]{4}-8[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$') {
        throw "$label fingerprint is not a bounded exact-payload identity."
    }
    $payloadLength = [int](($fingerprint -split ':')[1])
    if ($payloadLength -ne ([string]$value.m_sSnapshotPayload).Length) {
        throw "$label payload length does not match its fingerprint."
    }
    return $value
}

function Invoke-RebuildRestartStage {
    param(
        [Parameter(Mandatory = $true)][string]$ExecutablePath,
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$RuntimeAddonPath,
        [Parameter(Mandatory = $true)][string]$PackedAddonsParent,
        [Parameter(Mandatory = $true)][string]$ServerConfigPath,
        [Parameter(Mandatory = $true)][string]$ProfileRoot,
        [Parameter(Mandatory = $true)][string]$DebugDirectory,
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][string]$TempDirectory,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][string]$Stage,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld,
        [AllowEmptyString()][string]$ExpectedDeliveredFingerprint = "",
        [Parameter(Mandatory = $true)]$UnclaimedEngineProcessesObserved
    )

    if ($script:Stages -cnotcontains $Stage) {
        throw "Unknown exact garrison rebuild restart stage."
    }
    Assert-LowerHexNonce $SessionNonce "session nonce"
    $stageNonce = [Guid]::NewGuid().ToString("N")
    $resultPath = Join-Path $DebugDirectory (
        "{0}{1}.{2}.json" -f $script:ArtifactPrefix, $RunId, $Stage)
    $guardPath = Join-Path $DebugDirectory (
        "{0}{1}.guard.json" -f $script:ArtifactPrefix, $RunId)
    $carrierPath = Join-Path $DebugDirectory (
        "{0}{1}.carrier.json" -f $script:ArtifactPrefix, $RunId)
    if (Test-Path -LiteralPath $resultPath) {
        throw "$Stage result path was not fresh."
    }
    if (Test-Path -LiteralPath $guardPath) {
        throw "$Stage guard path was not fresh."
    }
    $firstStage = $Stage -ceq "prepare"
    if ($firstStage -and (Test-Path -LiteralPath $carrierPath)) {
        throw "$Stage carrier path was not fresh."
    }
    if (-not $firstStage -and
        -not (Test-Path -LiteralPath $carrierPath -PathType Leaf)) {
        throw "$Stage requires the prior exact carrier."
    }

    $allowCanonicalWrite = $Stage -cne "replay"
    $guard = [ordered]@{
        m_sMagic = $script:GuardMagic
        m_iVersion = $script:AuthorityVersion
        m_sSessionNonce = $SessionNonce
        m_sStageNonce = $stageNonce
        m_sRunId = $RunId
        m_sRequestedCut = $script:CutName
        m_sRequestedStage = $Stage
        m_iStageOrdinal = $script:StageOrdinals[$Stage]
        m_sBuildSha = $ExpectedBuild.BuildSha
        m_sBuildUtc = $ExpectedBuild.BuildUtc
        m_sBuildLabel = $ExpectedBuild.BuildLabel
        m_iCampaignSchemaVersion = $ExpectedBuild.CampaignSchemaVersion
        m_iSettingsSchemaVersion = $ExpectedBuild.SettingsSchemaVersion
        m_sWorld = $ExpectedWorld
        m_bAllowCanonicalCampaignOverwrite = $allowCanonicalWrite
    }
    Write-JsonUtf8NoBom -Path $guardPath -Value $guard
    [void](Assert-RebuildGuard `
        -Guard (Read-JsonArtifact -Path $guardPath) `
        -SessionNonce $SessionNonce `
        -StageNonce $stageNonce `
        -RunId $RunId `
        -Stage $Stage `
        -ExpectedBuild $ExpectedBuild `
        -ExpectedWorld $ExpectedWorld)

    $canonicalPath = Join-Path $ProfileRoot (
        "profile\Partisan\HST_CampaignSaveData.json")
    $recoveryPath = Join-Path $ProfileRoot (
        "profile\Partisan\HST_CampaignSaveData.recovery.json")
    $journalBefore = $null
    $carrierBeforeSignature = ""
    if (-not $allowCanonicalWrite) {
        $journalBefore = Get-CampaignJournalFileState `
            -CanonicalPath $canonicalPath `
            -RecoveryPath $recoveryPath
        if ([int]$journalBefore.FileCount -ne 2) {
            throw "$Stage requires both profile journal generations."
        }
        $carrierBeforeSignature = Get-FileSignature -Path $carrierPath
    }

    $arguments = Get-RebuildStageArgumentVector `
        -RuntimeAddonPath $RuntimeAddonPath `
        -PackedAddonsParent $PackedAddonsParent `
        -ServerConfigPath $ServerConfigPath `
        -ProfileRoot $ProfileRoot `
        -SessionNonce $SessionNonce `
        -StageNonce $stageNonce `
        -RunId $RunId `
        -Stage $Stage
    $assertCarrierCommand = ${function:Assert-RebuildCarrier}
    $assertResultCommand = ${function:Assert-RebuildResult}
    $readJsonArtifactCommand = ${function:Read-JsonArtifact}
    $validator = {
        param($candidate)
        $candidateCarrier = & $readJsonArtifactCommand -Path $carrierPath
        $candidateCarrier = & $assertCarrierCommand `
            -Carrier $candidateCarrier `
            -SessionNonce $SessionNonce `
            -RunId $RunId `
            -ExpectedBuild $ExpectedBuild `
            -ExpectedWorld $ExpectedWorld
        & $assertResultCommand `
            -Result $candidate `
            -Carrier $candidateCarrier `
            -SessionNonce $SessionNonce `
            -StageNonce $stageNonce `
            -RunId $RunId `
            -Stage $Stage `
            -ExpectedBuild $ExpectedBuild `
            -ExpectedWorld $ExpectedWorld `
            -ExpectedDeliveredFingerprint $ExpectedDeliveredFingerprint
    }.GetNewClosure()
    try {
        $processOutcome = Invoke-GuardedProcess `
            -Label "exact garrison rebuild restart/$Stage" `
            -ExecutablePath $ExecutablePath `
            -Arguments $arguments `
            -WorkingDirectory $WorkingDirectory `
            -TempDirectory $TempDirectory `
            -TimeoutSeconds $StageTimeoutSeconds `
            -ResultPath $resultPath `
            -ResultValidator $validator `
            -DiagnosticProjectDirectory $RepositoryRoot `
            -DiagnosticAddonRoots @($RuntimeAddonPath) `
            -UnclaimedEngineProcessesObserved $UnclaimedEngineProcessesObserved
    }
    catch {
        $processFailure = $_
        $rawResultDiagnostic = "raw result absent"
        if (Test-Path -LiteralPath $resultPath -PathType Leaf) {
            try {
                $rawResult = Read-JsonArtifact -Path $resultPath
				$rawEvidence = [string]$rawResult.m_sEvidence
				if ($rawEvidence.Length -gt 360) {
					$rawEvidence = $rawEvidence.Substring(
						$rawEvidence.Length - 360)
				}
                $rawResultDiagnostic =
                    "raw result success={0} flags restored/startup/source/continue/runtime/readback/prepared/casualty/delivery/held/resource={1}/{2}/{3}/{4}/{5}/{6}/{7}/{8}/{9}/{10}/{11} progress={12}/{13} fold={14} evidence={15}" -f
                    [bool]$rawResult.m_bSuccess,
					[int][bool]$rawResult.m_bRestored,
					[int][bool]$rawResult.m_bStartupReconcileChanged,
					[int][bool]$rawResult.m_bSourceExact,
					[int][bool]$rawResult.m_bContinuationExact,
					[int][bool]$rawResult.m_bRuntimeClaimantsZero,
					[int][bool]$rawResult.m_bPersistedReadBackExact,
					[int][bool]$rawResult.m_bPreparedCutExact,
					[int][bool]$rawResult.m_bCasualtyContinuityExact,
					[int][bool]$rawResult.m_bDeliveryReceiptExact,
					[int][bool]$rawResult.m_bHeldGarrisonExact,
					[int][bool]$rawResult.m_bResourceExactlyOnce,
					[Math]::Round([double]$rawResult.m_fProgressBeforeMeters, 3),
					[Math]::Round([double]$rawResult.m_fProgressAfterMeters, 3),
					[Math]::Round([double]$rawResult.m_fFoldRouteProgressMeters, 3),
					$rawEvidence
            }
            catch {
                $rawResultText = ""
                try {
                    $rawResultText = Read-SharedFileText -Path $resultPath
                }
                catch {}
                $rawResultHead = ConvertTo-SafeEvidenceLine -Line (
                    $rawResultText.Substring(
                        0,
                        [Math]::Min(160, $rawResultText.Length)))
                $rawResultDiagnostic =
                    "raw result parse failed bytes={0} head={1}" -f
                    $rawResultText.Length,
                    $rawResultHead
            }
        }
        $rawCarrierDiagnostic = "raw carrier absent"
        if (Test-Path -LiteralPath $carrierPath -PathType Leaf) {
            try {
                $rawCarrier = Read-JsonArtifact -Path $carrierPath
                $rawCarrierDiagnostic =
                    "raw carrier cut={0} samples={1} movement={2} closure={3}" -f
                    [string]$rawCarrier.m_sCutName,
                    [int]$rawCarrier.m_iLivePositionSampleCount,
                    [double]$rawCarrier.m_fLiveMovementMeters,
                    [double]$rawCarrier.m_fDistanceClosedMeters
            }
            catch {
                $rawCarrierText = ""
                try {
                    $rawCarrierText = Read-SharedFileText -Path $carrierPath
                }
                catch {}
                $rawCarrierHead = ConvertTo-SafeEvidenceLine -Line (
                    $rawCarrierText.Substring(
                        0,
                        [Math]::Min(160, $rawCarrierText.Length)))
                $rawCarrierDiagnostic =
                    "raw carrier parse failed bytes={0} head={1}" -f
                    $rawCarrierText.Length,
                    $rawCarrierHead
            }
        }
        throw ($rawResultDiagnostic + " | " + $rawCarrierDiagnostic +
            " | process " + $processFailure.Exception.Message)
    }

    if (Test-Path -LiteralPath $guardPath) {
        throw "$Stage did not consume its one-use engine guard."
    }
    $journalAfter = Get-CampaignJournalFileState `
        -CanonicalPath $canonicalPath `
        -RecoveryPath $recoveryPath
    $journalUnchanged = $null
    $carrierUnchanged = $null
    if (-not $allowCanonicalWrite) {
        $journalUnchanged = Test-CampaignJournalFileStateExact `
            -Expected $journalBefore `
            -Actual $journalAfter
        if (-not $journalUnchanged) {
            throw "$Stage changed the byte-hashed campaign journal."
        }
        $carrierUnchanged = (Get-FileSignature -Path $carrierPath) -ceq
            $carrierBeforeSignature
        if (-not $carrierUnchanged) {
            throw "$Stage changed the byte-hashed prepared carrier."
        }
    }

    $carrier = Assert-RebuildCarrier `
        -Carrier (Read-JsonArtifact -Path $carrierPath) `
        -SessionNonce $SessionNonce `
        -RunId $RunId `
        -ExpectedBuild $ExpectedBuild `
        -ExpectedWorld $ExpectedWorld
    $result = Assert-RebuildResult `
        -Result $processOutcome.Result `
        -Carrier $carrier `
        -SessionNonce $SessionNonce `
        -StageNonce $stageNonce `
        -RunId $RunId `
        -Stage $Stage `
        -ExpectedBuild $ExpectedBuild `
        -ExpectedWorld $ExpectedWorld `
        -ExpectedDeliveredFingerprint $ExpectedDeliveredFingerprint

    return [pscustomobject]@{
        Result = $result
        Carrier = $carrier
        ExitCode = $processOutcome.ExitCode
        JournalBefore = $journalBefore
        JournalAfter = $journalAfter
        CampaignJournalUnchanged = $journalUnchanged
        CarrierUnchanged = $carrierUnchanged
        SafeSummary = [pscustomobject]@{
            Stage = $Stage
            Success = [bool]$result.m_bSuccess
            Exit = $processOutcome.ExitCode
            Build = ([string]$result.m_sBuildSha).Substring(0, 12)
            Schema = [int]$result.m_iCampaignSchemaVersion
            Settings = [int]$result.m_iSettingsSchemaVersion
            Restored = [bool]$result.m_bRestored
            StartupChanged = [bool]$result.m_bStartupReconcileChanged
            ContinuationExact = [bool]$result.m_bContinuationExact
            SemanticNoOp = [bool]$result.m_bSameStateSemanticNoOp
            DeliveryReceiptExact = [bool]$result.m_bDeliveryReceiptExact
            HeldGarrisonExact = [bool]$result.m_bHeldGarrisonExact
            ResourceExactlyOnce = [bool]$result.m_bResourceExactlyOnce
            PhysicalBindingsExact = [bool]$result.m_bPhysicalBindingsExact
            PhysicalMovementExact = [bool]$result.m_bPhysicalMovementExact
            PhysicalFoldExact = [bool]$result.m_bPhysicalFoldExact
            PhysicalRootCount = [int]$result.m_iPhysicalRootCount
            PhysicalHandleCount = [int]$result.m_iPhysicalAdapterHandleCount
            PhysicalRuntimeMemberCount =
                [int]$result.m_iPhysicalRuntimeMemberCount
            LivePositionSampleCount = [int]$result.m_iLivePositionSampleCount
            LiveMovementMeters =
                [Math]::Round([double]$result.m_fLiveMovementMeters, 3)
            DistanceClosedMeters =
                [Math]::Round([double]$result.m_fDistanceClosedMeters, 3)
            FoldRouteProgressMeters =
                [Math]::Round([double]$result.m_fFoldRouteProgressMeters, 3)
            SourceFingerprintDigest = Get-FileNameSafeDigest (
                [string]$result.m_sSourceSemanticFingerprint)
            FinalFingerprintDigest = Get-FileNameSafeDigest (
                [string]$result.m_sFinalSemanticFingerprint)
            JournalFileCount = [int]$journalAfter.FileCount
            JournalReadOnly = $journalUnchanged
            CarrierReadOnly = $carrierUnchanged
        }
    }
}

function Invoke-RebuildContractSelfTests {
    param(
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld
    )

    $expectation = [pscustomobject]@{
        m_sOrderId = "self_order"
        m_sOperationId = "self_operation"
        m_sManifestId = "self_manifest"
        m_sManifestHash = "self_manifest_hash"
        m_sBatchId = "self_batch"
        m_sGroupId = "self_group"
        m_sProjectionId = "self_projection"
        m_sForceId = "self_force"
        m_sFactionKey = "self_faction"
        m_sSourceZoneId = "self_source"
        m_sTargetZoneId = "self_target"
        m_sExpectedSourceOwnerFactionKey = "self_source_owner"
        m_iExpectedSourceOwnershipRevision = 1
        m_sExpectedTargetOwnerFactionKey = "self_target_owner"
        m_iExpectedTargetOwnershipRevision = 2
        m_sDebitMutationId = "self_debit"
        m_sDeliverySettlementKind = "delivery"
        m_sDeliverySettlementId = "self_settlement"
        m_sRefundMutationId = "enemy_resource_refund_self_settlement"
        m_iAttackCost = 0
        m_iSupportCost = 20
        m_iExpectedAttackPool = 30
        m_iExpectedSupportPool = 40
        m_iExpectedPendingPoolRevision = 3
        m_iExpectedPendingPoolOperationalMutationCount = 1
        m_iAcceptedMemberCount = 9
        m_iLivingMemberCount = 8
        m_sLivingSlotFingerprint =
            "slot_1,slot_2,slot_3,slot_4,slot_5,slot_6,slot_7,slot_8"
        m_sConfirmedCasualtySlotId = "slot_9"
        m_sCasualtyTombstoneFingerprint = "slot_9|confirmed"
        m_iExpectedAggregateInfantry = 5
        m_iExpectedAuthoritativePendingInfantry = 13
        m_iExpectedAuthoritativeDeliveredInfantry = 13
    }
    $physicalCut = $script:CutName -ceq "physical_live_fold"
    $carrier = [pscustomobject]@{
        m_sMagic = $script:CarrierMagic
        m_sSessionNonce = $SessionNonce
        m_sRunId = $RunId
        m_sBuildSha = $ExpectedBuild.BuildSha
        m_sBuildUtc = $ExpectedBuild.BuildUtc
        m_sBuildLabel = $ExpectedBuild.BuildLabel
        m_iCampaignSchemaVersion = $ExpectedBuild.CampaignSchemaVersion
        m_iSettingsSchemaVersion = $ExpectedBuild.SettingsSchemaVersion
        m_sWorld = $ExpectedWorld
        m_sCutName = $script:CutName
        m_iCut = $script:CutOrdinal
        m_Expectation = $expectation
        m_iPreparedElapsedSecond = 10
        m_fPreMaterializationRouteProgressMeters = if ($physicalCut) {
            225.0
        }
        else {
            0.0
        }
        m_fPreMaterializationRouteTotalDistanceMeters = if ($physicalCut) {
            300.0
        }
        else {
            0.0
        }
        m_fPreparedRouteProgressMeters = if ($physicalCut) { 0.0 } else { 225.0 }
        m_fPreparedRouteTotalDistanceMeters = if ($physicalCut) { 200.0 } else { 300.0 }
        m_vPreparedStrategicPosition = "100 0 200"
        m_iExpectedPhysicalRootCount = if ($physicalCut) { 1 } else { 0 }
        m_iExpectedPhysicalAdapterHandleCount = if ($physicalCut) { 9 } else { 0 }
        m_iExpectedPhysicalRuntimeMemberCount = if ($physicalCut) { 8 } else { 0 }
        m_iLivePositionSampleCount = if ($physicalCut) { 2 } else { 0 }
        m_vInitialLivePosition = if ($physicalCut) {
            "96 0 200"
        }
        else {
            "0 0 0"
        }
        m_vFinalLivePosition = if ($physicalCut) {
            "100 0 200"
        }
        else {
            "0 0 0"
        }
        m_fInitialDistanceToTargetMeters = if ($physicalCut) { 204.0 } else { 0.0 }
        m_fFinalDistanceToTargetMeters = if ($physicalCut) { 200.0 } else { 0.0 }
        m_fLiveMovementMeters = if ($physicalCut) { 4.0 } else { 0.0 }
        m_fDistanceClosedMeters = if ($physicalCut) { 4.0 } else { 0.0 }
        m_vFoldPosition = if ($physicalCut) { "100 0 200" } else { "0 0 0" }
        m_fFoldRouteProgressMeters = 0.0
        m_bPhysicalMovementExact = $physicalCut
        m_bPhysicalFoldExact = $physicalCut
        m_sPreparedSemanticFingerprint = "prepared:self:v1"
    }
    [void](Assert-RebuildCarrier `
        -Carrier $carrier `
        -SessionNonce $SessionNonce `
        -RunId $RunId `
        -ExpectedBuild $ExpectedBuild `
        -ExpectedWorld $ExpectedWorld)

    $stageNonces = @{
        prepare = [Guid]::NewGuid().ToString("N")
        recover = [Guid]::NewGuid().ToString("N")
        replay = [Guid]::NewGuid().ToString("N")
    }
    $common = [ordered]@{
        m_sMagic = $script:ResultMagic
        m_sSessionNonce = $SessionNonce
        m_sStageNonce = $stageNonces.prepare
        m_sRunId = $RunId
        m_sStage = "prepare"
        m_bSuccess = $true
        m_sBuildSha = $ExpectedBuild.BuildSha
        m_sBuildUtc = $ExpectedBuild.BuildUtc
        m_sBuildLabel = $ExpectedBuild.BuildLabel
        m_iCampaignSchemaVersion = $ExpectedBuild.CampaignSchemaVersion
        m_iSettingsSchemaVersion = $ExpectedBuild.SettingsSchemaVersion
        m_sWorld = $ExpectedWorld
        m_sCutName = $script:CutName
        m_iCut = $script:CutOrdinal
        m_bRestored = $false
        m_bStartupReconcileChanged = $false
        m_bSourceExact = $true
        m_bContinuationExact = $false
        m_bSameStateSemanticNoOp = $false
        m_bRuntimeClaimantsZero = $true
        m_bPersistedReadBackExact = $true
        m_bPreparedCutExact = $true
        m_bCasualtyContinuityExact = $true
        m_bDeliveryReceiptExact = $false
        m_bHeldGarrisonExact = $false
        m_bAggregateNotDoubleCounted = $false
        m_bResourceExactlyOnce = $false
        m_bPhysicalBindingsExact = $physicalCut
        m_bPhysicalMovementExact = $physicalCut
        m_bPhysicalFoldExact = $physicalCut
        m_iPhysicalRootCount = if ($physicalCut) { 1 } else { 0 }
        m_iPhysicalAdapterHandleCount = if ($physicalCut) { 9 } else { 0 }
        m_iPhysicalRuntimeMemberCount = if ($physicalCut) { 8 } else { 0 }
        m_iLivePositionSampleCount = if ($physicalCut) { 2 } else { 0 }
        m_vInitialLivePosition = if ($physicalCut) { "96 0 200" } else { "0 0 0" }
        m_vFinalLivePosition = if ($physicalCut) { "100 0 200" } else { "0 0 0" }
        m_fInitialDistanceToTargetMeters = if ($physicalCut) { 204.0 } else { 0.0 }
        m_fFinalDistanceToTargetMeters = if ($physicalCut) { 200.0 } else { 0.0 }
        m_fLiveMovementMeters = if ($physicalCut) { 4.0 } else { 0.0 }
        m_fDistanceClosedMeters = if ($physicalCut) { 4.0 } else { 0.0 }
        m_vFoldPosition = if ($physicalCut) { "100 0 200" } else { "0 0 0" }
        m_fFoldRouteProgressMeters = 0.0
        m_fProgressBeforeMeters = if ($physicalCut) { 0.0 } else { 225.0 }
        m_fProgressAfterMeters = if ($physicalCut) { 0.0 } else { 225.0 }
        m_sSourceSemanticFingerprint = "prepared:self:v1"
        m_sFinalSemanticFingerprint = "prepared:self:v1"
        m_sEvidence = "synthetic prepare exact"
    }
    $prepareResult = [pscustomobject]$common
    [void](Assert-RebuildResult `
        -Result $prepareResult `
        -Carrier $carrier `
        -SessionNonce $SessionNonce `
        -StageNonce $stageNonces.prepare `
        -RunId $RunId `
        -Stage "prepare" `
        -ExpectedBuild $ExpectedBuild `
        -ExpectedWorld $ExpectedWorld)

    $recoverResult = $prepareResult | ConvertTo-Json -Depth 12 |
        ConvertFrom-Json
    $recoverResult.m_sStageNonce = $stageNonces.recover
    $recoverResult.m_sStage = "recover"
    $recoverResult.m_bRestored = $true
    $recoverResult.m_bStartupReconcileChanged = $true
    $recoverResult.m_bContinuationExact = $true
    $recoverResult.m_bDeliveryReceiptExact = $true
    $recoverResult.m_bHeldGarrisonExact = $true
    $recoverResult.m_bAggregateNotDoubleCounted = $true
    $recoverResult.m_bResourceExactlyOnce = $true
    $recoverResult.m_fProgressAfterMeters = if ($physicalCut) { 200.0 } else { 300.0 }
    $recoverResult.m_sFinalSemanticFingerprint = "delivered:self:v1"
    $recoverResult.m_sEvidence = "synthetic recover exact"
    [void](Assert-RebuildResult `
        -Result $recoverResult `
        -Carrier $carrier `
        -SessionNonce $SessionNonce `
        -StageNonce $stageNonces.recover `
        -RunId $RunId `
        -Stage "recover" `
        -ExpectedBuild $ExpectedBuild `
        -ExpectedWorld $ExpectedWorld)

    $replayResult = $recoverResult | ConvertTo-Json -Depth 12 |
        ConvertFrom-Json
    $replayResult.m_sStageNonce = $stageNonces.replay
    $replayResult.m_sStage = "replay"
    $replayResult.m_bStartupReconcileChanged = $false
    $replayResult.m_bContinuationExact = $false
    $replayResult.m_bSameStateSemanticNoOp = $true
    $replayResult.m_fProgressBeforeMeters = if ($physicalCut) { 200.0 } else { 300.0 }
    $replayResult.m_sSourceSemanticFingerprint = "delivered:self:v1"
    $replayResult.m_sEvidence = "synthetic replay exact"
    [void](Assert-RebuildResult `
        -Result $replayResult `
        -Carrier $carrier `
        -SessionNonce $SessionNonce `
        -StageNonce $stageNonces.replay `
        -RunId $RunId `
        -Stage "replay" `
        -ExpectedBuild $ExpectedBuild `
        -ExpectedWorld $ExpectedWorld `
        -ExpectedDeliveredFingerprint "delivered:self:v1")

    $tamperedResult = $replayResult | ConvertTo-Json -Depth 12 |
        ConvertFrom-Json
    $tamperedResult.m_sStageNonce = [Guid]::NewGuid().ToString("N")
    $tamperedRejected = $false
    try {
        [void](Assert-RebuildResult `
            -Result $tamperedResult `
            -Carrier $carrier `
            -SessionNonce $SessionNonce `
            -StageNonce $stageNonces.replay `
            -RunId $RunId `
            -Stage "replay" `
            -ExpectedBuild $ExpectedBuild `
            -ExpectedWorld $ExpectedWorld `
            -ExpectedDeliveredFingerprint "delivered:self:v1")
    }
    catch {
        $tamperedRejected = $true
    }
    if (-not $tamperedRejected) {
        throw "Result stage-nonce negative self-test failed."
    }
    $failedResult = $recoverResult | ConvertTo-Json -Depth 12 |
        ConvertFrom-Json
    $failedResult.m_bSuccess = $false
    $failedResult.m_bContinuationExact = $false
    $failedResult.m_sEvidence = "synthetic recover failure evidence"
    $failureMessage = ""
    try {
        [void](Assert-RebuildResult `
            -Result $failedResult `
            -Carrier $carrier `
            -SessionNonce $SessionNonce `
            -StageNonce $stageNonces.recover `
            -RunId $RunId `
            -Stage "recover" `
            -ExpectedBuild $ExpectedBuild `
            -ExpectedWorld $ExpectedWorld)
    }
    catch {
        $failureMessage = $_.Exception.Message
    }
    $flagsIndex = $failureMessage.IndexOf("src=1,rst=1,start=1,cont=0")
    $evidenceIndex = $failureMessage.IndexOf(
        "evidence=synthetic recover failure evidence")
    if ($flagsIndex -lt 0 -or $evidenceIndex -le $flagsIndex -or
        $failureMessage.Length -gt 500) {
        throw "Failed-result front-loaded evidence self-test failed."
    }
    return [pscustomobject]@{
        PositiveStageCount = 3
        TamperedResultRejected = $tamperedRejected
        FailureEvidenceFrontLoaded = $true
    }
}

$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$expectedProjectPath = [IO.Path]::GetFullPath(
    (Join-Path $repositoryRoot "addon.gproj"))
$workspacePackScratchPath = Join-Path `
    $repositoryRoot `
    $script:WorkspacePackScratchLeaf
$workspacePackScratchSentinelPath = Join-Path `
    $workspacePackScratchPath `
    $script:WorkspacePackScratchSentinelLeaf
$nonce = [Guid]::NewGuid().ToString("N")
$runId = "garrisonrebuild_{0}_{1}" -f
    [DateTime]::UtcNow.ToString(
        "yyyyMMddHHmmss", [Globalization.CultureInfo]::InvariantCulture),
    $nonce.Substring(0, 20)
$guardBase = [IO.Path]::GetFullPath(
    (Join-Path ([IO.Path]::GetTempPath()) $script:GuardBaseLeaf))
$guardLeaf = $script:GuardLeafPrefix + $nonce
$guardRoot = [IO.Path]::GetFullPath((Join-Path $guardBase $guardLeaf))
$guardSentinelPath = Join-Path $guardRoot $script:GuardSentinelLeaf
$packProfileRoot = Join-Path $guardRoot "pack-profile"
$packedAddonsParent = Join-Path $guardRoot "packed-addons"
$packedAddonPath = Join-Path $packedAddonsParent "Partisan"
$profileRoot = Join-Path $guardRoot "proof-backend"
$partisanRoot = Join-Path $profileRoot "profile\Partisan"
$debugRoot = Join-Path $partisanRoot "debug"
$workingRoot = Join-Path $guardRoot "working"
$tempRoot = Join-Path $guardRoot "temp"
$serverConfigPath = Join-Path $guardRoot "garrison-rebuild-server-config.json"
$ownerPath = Join-Path $debugRoot (
    "{0}{1}.owner.json" -f $script:ArtifactPrefix, $runId)
$carrierPath = Join-Path $debugRoot (
    "{0}{1}.carrier.json" -f $script:ArtifactPrefix, $runId)
$canonicalPath = Join-Path $partisanRoot "HST_CampaignSaveData.json"
$recoveryPath = Join-Path $partisanRoot "HST_CampaignSaveData.recovery.json"
$settingsPath = Join-Path $partisanRoot "HST_Settings.json"

$mutex = $null
$mutexAcquired = $false
$guardOwnership = $null
$workspacePackScratchCreated = $false
$workspacePackScratchOwnership = $null
$runError = $null
$runSucceeded = $false
$cleanupErrors = New-Object Collections.Generic.List[string]
$watchSnapshots = New-Object Collections.Generic.List[object]
$spillSnapshots = New-Object Collections.Generic.List[object]
$stageOutcomes = New-Object Collections.Generic.List[object]
$unclaimedEngineProcessesObserved =
    New-Object Collections.Generic.HashSet[string]
$cleanupResult = $null
$executablePath = ""
$workbenchPath = ""
$runtimeAddonPath = ""
$projectFile = ""
$buildIdentity = $null
$wrapperStartUtc = [DateTime]::MinValue
$packSummary = $null
$proofSummary = $null

try {
    $executablePath = Resolve-ExistingPath $Executable Leaf
    if ((Split-Path -Leaf $executablePath) -cne
        "ArmaReforgerServerDiag.exe") {
        throw "Exact garrison rebuild restart proof requires the dedicated diagnostic server."
    }
    $workbenchPath = Resolve-ExistingPath $WorkbenchExecutable Leaf
    if ((Split-Path -Leaf $workbenchPath) -cne
        "ArmaReforgerWorkbenchSteamDiag.exe") {
        throw "Exact garrison rebuild restart proof requires diagnostic Steam Workbench."
    }
    $runtimeAddonPath = Resolve-ExistingPath $RuntimeAddonRoot Container
    foreach ($marker in @("core\data.pak", "data\data.pak")) {
        $runtimeMarkerPath = Join-Path $runtimeAddonPath $marker
        if (-not (Test-Path -LiteralPath $runtimeMarkerPath -PathType Leaf)) {
            throw "RuntimeAddonRoot is not an installed packed game add-on root."
        }
    }
    $projectFile = if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
        Resolve-ExistingPath $expectedProjectPath Leaf
    }
    else {
        Resolve-ExistingPath $ProjectPath Leaf
    }
    if (-not $projectFile.Equals(
        $expectedProjectPath, [StringComparison]::OrdinalIgnoreCase)) {
        throw "The proof must pack this checkout's addon.gproj."
    }
    if ($WorldResource -cne "Worlds/HST_Dev/HST_Dev.ent") {
        throw "Exact garrison rebuild restart proof requires the canonical proof world."
    }
    foreach ($resource in @($WorldResource, $script:MissionHeader)) {
        $resourcePath = Join-Path $repositoryRoot (
            $resource.Replace('/', [IO.Path]::DirectorySeparatorChar))
        [void](Resolve-ExistingPath $resourcePath Leaf)
    }
    $buildIdentity = Read-CheckoutBuildIdentity $repositoryRoot

    $normalizedWatchedRoots = @($WatchedRoots | Where-Object {
        -not [string]::IsNullOrWhiteSpace([string]$_)
    })
    $normalizedSpillRoots = @($SpillRoots | Where-Object {
        -not [string]::IsNullOrWhiteSpace([string]$_)
    })
    if (-not $PreflightOnly -and
        ($normalizedWatchedRoots.Count -eq 0 -or
            $normalizedSpillRoots.Count -eq 0)) {
        throw "A real proof requires explicit watched and spill roots."
    }
    $checkoutCovered = $false
    foreach ($root in $normalizedWatchedRoots) {
        $resolvedWatchRoot = Resolve-ExistingPath $root Container
        if (Test-ContainedPath `
            -Root $resolvedWatchRoot `
            -Candidate $repositoryRoot `
            -AllowEqual) {
            $checkoutCovered = $true
            break
        }
    }
    if (-not $PreflightOnly -and -not $checkoutCovered) {
        throw "A real proof requires watched-root coverage of this checkout."
    }
    if (-not $PreflightOnly) {
        $documentsRoot = [Environment]::GetFolderPath(
            [Environment+SpecialFolder]::MyDocuments)
        if ([string]::IsNullOrWhiteSpace($documentsRoot)) {
            throw "The external game-state monitoring boundary is unavailable."
        }
        $expectedSpillBoundary = Resolve-ExistingPath `
            -Path (Join-Path $documentsRoot "My Games") `
            -Kind Container
        $spillBoundaryCovered = $false
        foreach ($root in $normalizedSpillRoots) {
            $resolvedSpillRoot = Resolve-ExistingPath $root Container
            if (Test-ContainedPath `
                -Root $resolvedSpillRoot `
                -Candidate $expectedSpillBoundary `
                -AllowEqual) {
                $spillBoundaryCovered = $true
                break
            }
        }
        if (-not $spillBoundaryCovered) {
            throw "A real proof requires spill-root coverage of the external game-state boundary."
        }
    }
    if (-not $PreflightOnly -and
        (Test-Path -LiteralPath $workspacePackScratchPath)) {
        throw "Workbench packing requires a fresh workspace scratch boundary."
    }

    $mutex = New-Object Threading.Mutex($false, $script:MutexName)
    try {
        $mutexAcquired = $mutex.WaitOne(0)
    }
    catch [Threading.AbandonedMutexException] {
        $mutexAcquired = $true
    }
    if (-not $mutexAcquired) {
        throw "Another guarded persistence proof wrapper is active."
    }
    if (@(Get-EngineProcessRows).Count -ne 0) {
        throw "Refusing launch while an engine or Workbench process is active."
    }

    foreach ($root in $normalizedWatchedRoots) {
        $watchRoot = Resolve-ExistingPath -Path $root -Kind Container
        $watchExclusions = @(Get-CheckoutWatchExclusions `
            -RepositoryRoot $repositoryRoot `
            -WatchRoot $watchRoot)
        $watchSnapshots.Add((New-RootSnapshot `
            -Root $watchRoot `
            -ExcludedRoots $watchExclusions))
    }
    $spillExclusions = New-Object Collections.Generic.List[string]
    $spillExclusions.Add((Split-Path -Parent $projectFile))
    $spillExclusions.Add($guardBase)
    foreach ($snapshot in $watchSnapshots) {
        if (-not $spillExclusions.Contains($snapshot.Root)) {
            $spillExclusions.Add($snapshot.Root)
        }
    }
    foreach ($root in $normalizedSpillRoots) {
        $spillSnapshots.Add((New-RootSnapshot `
            -Root $root `
            -ExcludedRoots $spillExclusions.ToArray()))
    }

    Assert-NoReparsePathAncestry -Path $guardBase
    if (-not (Test-Path -LiteralPath $guardBase -PathType Container)) {
        [void](New-Item -ItemType Directory -Path $guardBase)
    }
    Assert-NoReparsePathAncestry -Path $guardBase
    if (Test-Path -LiteralPath $guardRoot) {
        throw "The nonce-owned guard root was not fresh."
    }
    [void](New-Item -ItemType Directory -Path $guardRoot)
    $wrapper = Get-Process -Id $PID -ErrorAction Stop
    $wrapperStartUtc = $wrapper.StartTime.ToUniversalTime()
    Write-JsonUtf8NoBom -Path $guardSentinelPath -Value ([ordered]@{
        version = $script:SentinelVersion
        purpose = $script:OwnerPurpose
        nonce = $nonce
        guardLeaf = $guardLeaf
        ownerPid = $PID
        ownerStartUtc = $wrapperStartUtc.ToString(
            "o", [Globalization.CultureInfo]::InvariantCulture)
    })
    $guardOwnership = Read-GuardOwnership $guardRoot $guardBase
    if (-not $guardOwnership -or
        $guardOwnership.Nonce -cne $nonce -or
        $guardOwnership.OwnerPid -ne $PID -or
        $guardOwnership.OwnerStartUtc.Ticks -ne $wrapperStartUtc.Ticks) {
        throw "Guard ownership validation failed."
    }

    if (-not $PreflightOnly) {
        if (Test-Path -LiteralPath $workspacePackScratchPath) {
            throw "Workbench packing requires a fresh workspace scratch boundary."
        }
        [void](New-Item `
            -ItemType Directory `
            -Path $workspacePackScratchPath)
        $workspacePackScratchCreated = $true
        Assert-NoReparsePathAncestry -Path $workspacePackScratchPath
        Write-JsonUtf8NoBom `
            -Path $workspacePackScratchSentinelPath `
            -Value ([ordered]@{
                version = $script:SentinelVersion
                purpose = "native_workbench_pack_scratch"
                nonce = $nonce
                ownerPid = $PID
                ownerStartUtc = $wrapperStartUtc.ToString(
                    "o", [Globalization.CultureInfo]::InvariantCulture)
            })
        $workspacePackScratchOwnership =
            Read-WorkspacePackScratchOwnership `
                -Directory $workspacePackScratchPath `
                -RepositoryRoot $repositoryRoot
        if (-not $workspacePackScratchOwnership -or
            $workspacePackScratchOwnership.Nonce -cne $nonce -or
            $workspacePackScratchOwnership.OwnerPid -ne $PID -or
            $workspacePackScratchOwnership.OwnerStartUtc.Ticks -ne
                $wrapperStartUtc.Ticks) {
            throw "Workspace pack scratch ownership validation failed."
        }
    }

    foreach ($directory in @(
        $packProfileRoot,
        $packedAddonsParent,
        $debugRoot,
        $workingRoot,
        $tempRoot)) {
        [void](New-Item -ItemType Directory -Path $directory -Force)
        Assert-NoReparsePathAncestry -Path $directory
    }
    if (Test-Path -LiteralPath $packedAddonPath) {
        throw "The packed add-on output path was not fresh."
    }

    $schedulerSettings = [ordered]@{
        schemaVersion = $buildIdentity.SettingsSchemaVersion
        persistence = [ordered]@{
            autosaveIntervalSeconds =
                $script:HSTAutosaveSchedulerIntervalSeconds
            majorChangeDebounceSeconds =
                $script:HSTAutosaveSchedulerDebounceSeconds
        }
    }
    Write-JsonUtf8NoBom -Path $settingsPath -Value $schedulerSettings
    $validatedSettings = Read-JsonArtifact -Path $settingsPath
    if ([int]$validatedSettings.schemaVersion -ne
            $buildIdentity.SettingsSchemaVersion -or
        [int]$validatedSettings.persistence.autosaveIntervalSeconds -ne
            $script:HSTAutosaveSchedulerIntervalSeconds -or
        [int]$validatedSettings.persistence.majorChangeDebounceSeconds -ne
            $script:HSTAutosaveSchedulerDebounceSeconds) {
        throw "The guarded scheduler settings failed their exact schema gate."
    }

    $serverConfig = [ordered]@{
        game = [ordered]@{
            name = "Partisan exact garrison rebuild restart proof"
            password = ""
            passwordAdmin = ""
            scenarioId = $script:ScenarioId
            maxPlayers = 1
            visible = $false
            gameProperties = [ordered]@{
                fastValidation = $true
                battlEye = $false
                persistence = [ordered]@{
                    autoSaveInterval = 60
                    saveRetention = 10
                    loadSessionSave = $false
                    keepSessionSave = $false
                }
                missionHeader = [ordered]@{
                    m_eSaveTypes = 15
                }
            }
            mods = @(
                [ordered]@{
                    modId = $script:ProjectId
                    name = "Partisan"
                    required = $true
                })
        }
    }
    Write-JsonUtf8NoBom -Path $serverConfigPath -Value $serverConfig
    $validatedConfig = Read-JsonArtifact -Path $serverConfigPath
    $validatedMods = @($validatedConfig.game.mods)
    foreach ($property in @(
        "autoSaveInterval", "saveRetention", "loadSessionSave",
        "keepSessionSave")) {
        Assert-JsonProperty `
            -Value $validatedConfig.game.gameProperties.persistence `
            -PropertyName $property `
            -ArtifactLabel "guarded server persistence config"
    }
    if ([string]$validatedConfig.game.scenarioId -cne $script:ScenarioId -or
        [int]$validatedConfig.game.gameProperties.missionHeader.m_eSaveTypes -ne
            15 -or
        [bool]$validatedConfig.game.visible -or
        [int]$validatedConfig.game.gameProperties.persistence.autoSaveInterval -ne 60 -or
        [int]$validatedConfig.game.gameProperties.persistence.saveRetention -ne 10 -or
        $validatedConfig.game.gameProperties.persistence.loadSessionSave -isnot [bool] -or
        [bool]$validatedConfig.game.gameProperties.persistence.loadSessionSave -or
        $validatedConfig.game.gameProperties.persistence.keepSessionSave -isnot [bool] -or
        [bool]$validatedConfig.game.gameProperties.persistence.keepSessionSave -or
        $validatedMods.Count -ne 1 -or
        [string]$validatedMods[0].modId -cne $script:ProjectId -or
        [string]$validatedMods[0].name -cne "Partisan" -or
        $validatedMods[0].required -isnot [bool] -or
        -not [bool]$validatedMods[0].required) {
        throw "The guarded server config failed its exact gate."
    }

    $owner = New-RebuildOwnerValue `
        -SessionNonce $nonce `
        -RunId $runId `
        -BuildIdentity $buildIdentity `
        -World $WorldResource
    Write-JsonUtf8NoBom -Path $ownerPath -Value $owner
    [void](Assert-RebuildOwner `
        -Owner (Read-JsonArtifact -Path $ownerPath) `
        -SessionNonce $nonce `
        -RunId $runId `
        -ExpectedBuild $buildIdentity `
        -ExpectedWorld $WorldResource)
    $contractSelfTests = Invoke-RebuildContractSelfTests `
        -SessionNonce $nonce `
        -RunId $runId `
        -ExpectedBuild $buildIdentity `
        -ExpectedWorld $WorldResource
    if ([int]$contractSelfTests.PositiveStageCount -ne 3 -or
        -not [bool]$contractSelfTests.TamperedResultRejected -or
        -not [bool]$contractSelfTests.FailureEvidenceFrontLoaded) {
        throw "Exact garrison rebuild restart contract self-tests did not close."
    }

    $packArguments = Get-PackArgumentVector `
        -ProjectFile $projectFile `
        -RuntimeAddonPath $runtimeAddonPath `
        -PackProfilePath $packProfileRoot `
        -PackedAddonPath $packedAddonPath
    $preflightVectors = New-Object Collections.Generic.List[object]
    foreach ($stage in $script:Stages) {
        $stageNonce = [Guid]::NewGuid().ToString("N")
        $preflightVectors.Add([pscustomobject]@{
            Stage = $stage
            Arguments = @(Get-RebuildStageArgumentVector `
                -RuntimeAddonPath $runtimeAddonPath `
                -PackedAddonsParent $packedAddonsParent `
                -ServerConfigPath $serverConfigPath `
                -ProfileRoot $profileRoot `
                -SessionNonce $nonce `
                -StageNonce $stageNonce `
                -RunId $runId `
                -Stage $stage)
        })
    }
    $forbiddenRetentionCount = @($preflightVectors | Where-Object {
        $_.Arguments -icontains "-keepSessionSave"
    }).Count
    $nativeLoadCount = @($preflightVectors | Where-Object {
        $_.Arguments -icontains "-loadSessionSave"
    }).Count
    $autoShutdownCount = @($preflightVectors | Where-Object {
        $_.Arguments -icontains "-autoshutdown"
    }).Count
    $backendLocalStorageCount = @($preflightVectors | Where-Object {
        $_.Arguments -ccontains "-backendLocalStorage"
    }).Count
    $cutArgumentVectorCount = @($preflightVectors | Where-Object {
        Test-RebuildStageCutArgumentVector -Arguments @($_.Arguments)
    }).Count
    if ($preflightVectors.Count -ne $script:ExpectedStageCount -or
        $forbiddenRetentionCount -ne 0 -or $nativeLoadCount -ne 0 -or
        $autoShutdownCount -ne 0 -or
        $backendLocalStorageCount -ne $script:ExpectedStageCount -or
        $cutArgumentVectorCount -ne $script:ExpectedStageCount) {
        throw "Preflight stage vectors failed their profile-journal isolation gate."
    }

    if ($PreflightOnly) {
        $runSucceeded = $true
        Write-Output ("PREFLIGHT " + ([pscustomobject]@{
            Build = $buildIdentity.BuildSha.Substring(0, 12)
            Schema = $buildIdentity.CampaignSchemaVersion
            Settings = $buildIdentity.SettingsSchemaVersion
            World = $WorldResource
            Scenario = $script:ScenarioId
            Cut = $script:CutName
            PackArgumentTokenCount = $packArguments.Count
            StageCount = $preflightVectors.Count
            CutArgumentVectors = $cutArgumentVectorCount
            PackedLaunch = $true
            NativeStageLoads = $nativeLoadCount
            KeepSessionSaveCLIAbsent = $forbiddenRetentionCount -eq 0
            LoadSessionSaveConfig =
                [bool]$validatedConfig.game.gameProperties.persistence.loadSessionSave
            NoAutoShutdown = $autoShutdownCount -eq 0
            BackendLocalStorage =
                $backendLocalStorageCount -eq $script:ExpectedStageCount
            CampaignWritingStages = @("prepare", "recover")
            ReadOnlyReplay = $true
        } | ConvertTo-Json -Compress))
    }
    else {
        $packOutcome = Invoke-GuardedProcess `
            -Label "exact garrison rebuild restart Workbench pack" `
            -ExecutablePath $workbenchPath `
            -Arguments $packArguments `
            -WorkingDirectory $workingRoot `
            -TempDirectory $tempRoot `
            -TimeoutSeconds $PackTimeoutSeconds `
            -DiagnosticProjectDirectory $repositoryRoot `
            -DiagnosticAddonRoots @($runtimeAddonPath) `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $workspacePackScratchOwnership =
            Read-WorkspacePackScratchOwnership `
                -Directory $workspacePackScratchPath `
                -RepositoryRoot $repositoryRoot
        if (-not $workspacePackScratchOwnership -or
            $workspacePackScratchOwnership.Nonce -cne $nonce -or
            $workspacePackScratchOwnership.OwnerPid -ne $PID -or
            $workspacePackScratchOwnership.OwnerStartUtc.Ticks -ne
                $wrapperStartUtc.Ticks) {
            throw "Workbench packing changed its owned workspace scratch boundary."
        }
        $packedLayout = Assert-PackedAddonLayout `
            -GuardRoot $guardRoot `
            -PackedAddonsParent $packedAddonsParent
        $packSummary = [pscustomobject]@{
            Exit = $packOutcome.ExitCode
            EngineAfter = $packOutcome.EngineAfter
            OwnedProcessesRemaining = $packOutcome.OwnedProcessesRemaining
            FileCount = $packedLayout.FileCount
            PakCount = $packedLayout.PakCount
            ProjectCount = $packedLayout.ProjectCount
            ResourceDatabaseCount = $packedLayout.ResourceDatabaseCount
        }
        Write-Output ("PACK " + ($packSummary | ConvertTo-Json -Compress))

        $prepare = Invoke-RebuildRestartStage `
            -ExecutablePath $executablePath `
            -RepositoryRoot $repositoryRoot `
            -RuntimeAddonPath $runtimeAddonPath `
            -PackedAddonsParent $packedAddonsParent `
            -ServerConfigPath $serverConfigPath `
            -ProfileRoot $profileRoot `
            -DebugDirectory $debugRoot `
            -WorkingDirectory $workingRoot `
            -TempDirectory $tempRoot `
            -SessionNonce $nonce `
            -RunId $runId `
            -Stage "prepare" `
            -ExpectedBuild $buildIdentity `
            -ExpectedWorld $WorldResource `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $stageOutcomes.Add($prepare.SafeSummary)
        Write-Output ("STAGE " +
            ($prepare.SafeSummary | ConvertTo-Json -Compress))
        if ([int]$prepare.JournalAfter.FileCount -ne 1 -or
            -not [bool]$prepare.JournalAfter.CanonicalPresent -or
            [bool]$prepare.JournalAfter.RecoveryPresent) {
            throw "Prepare did not create one canonical journal generation."
        }
        $prepareEnvelope = Read-RebuildJournalEnvelope `
            -Path $canonicalPath `
            -ExpectedGeneration 1 `
            -ExpectedSchema $buildIdentity.CampaignSchemaVersion `
            -ExpectedPreviousGeneration 0 `
            -ExpectedPreviousFingerprint ""
        $carrierAfterPrepareSignature = Get-FileSignature -Path $carrierPath
        $preparedFingerprint =
            [string]$prepare.Carrier.m_sPreparedSemanticFingerprint

        $recover = Invoke-RebuildRestartStage `
            -ExecutablePath $executablePath `
            -RepositoryRoot $repositoryRoot `
            -RuntimeAddonPath $runtimeAddonPath `
            -PackedAddonsParent $packedAddonsParent `
            -ServerConfigPath $serverConfigPath `
            -ProfileRoot $profileRoot `
            -DebugDirectory $debugRoot `
            -WorkingDirectory $workingRoot `
            -TempDirectory $tempRoot `
            -SessionNonce $nonce `
            -RunId $runId `
            -Stage "recover" `
            -ExpectedBuild $buildIdentity `
            -ExpectedWorld $WorldResource `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $stageOutcomes.Add($recover.SafeSummary)
        Write-Output ("STAGE " +
            ($recover.SafeSummary | ConvertTo-Json -Compress))
        $deliveredFingerprint =
            [string]$recover.Result.m_sFinalSemanticFingerprint
        if ([string]$recover.Result.m_sSourceSemanticFingerprint -cne
                $preparedFingerprint -or
            $deliveredFingerprint -ceq $preparedFingerprint) {
            throw "Recover did not consume the exact prepared state into one distinct delivered state."
        }
        if ((Get-FileSignature -Path $carrierPath) -cne
            $carrierAfterPrepareSignature) {
            throw "Recover changed the immutable prepared carrier."
        }
        if ([int]$recover.JournalAfter.FileCount -ne 2 -or
            -not [bool]$recover.JournalAfter.CanonicalPresent -or
            -not [bool]$recover.JournalAfter.RecoveryPresent) {
            throw "Recover did not create the exact rotating two-generation journal."
        }
        $recoveryEnvelope = Read-RebuildJournalEnvelope `
            -Path $recoveryPath `
            -ExpectedGeneration 2 `
            -ExpectedSchema $buildIdentity.CampaignSchemaVersion `
            -ExpectedPreviousGeneration 1 `
            -ExpectedPreviousFingerprint (
                [string]$prepareEnvelope.m_sSnapshotFingerprint)
        if ([string]$recoveryEnvelope.m_sSnapshotFingerprint -ceq
            [string]$prepareEnvelope.m_sSnapshotFingerprint) {
            throw "Recover did not advance the automatic checkpoint beyond prepare."
        }
        $canonicalEnvelope = Read-RebuildJournalEnvelope `
            -Path $canonicalPath `
            -ExpectedGeneration 1 `
            -ExpectedSchema $buildIdentity.CampaignSchemaVersion `
            -ExpectedPreviousGeneration 0 `
            -ExpectedPreviousFingerprint ""
        if ([string]$canonicalEnvelope.m_sSnapshotFingerprint -cne
            [string]$prepareEnvelope.m_sSnapshotFingerprint) {
            throw "Recover changed the retained prepare journal slot."
        }
        $journalBeforeReplay = Get-CampaignJournalFileState `
            -CanonicalPath $canonicalPath `
            -RecoveryPath $recoveryPath
        $carrierBeforeReplaySignature = Get-FileSignature -Path $carrierPath

        $replay = Invoke-RebuildRestartStage `
            -ExecutablePath $executablePath `
            -RepositoryRoot $repositoryRoot `
            -RuntimeAddonPath $runtimeAddonPath `
            -PackedAddonsParent $packedAddonsParent `
            -ServerConfigPath $serverConfigPath `
            -ProfileRoot $profileRoot `
            -DebugDirectory $debugRoot `
            -WorkingDirectory $workingRoot `
            -TempDirectory $tempRoot `
            -SessionNonce $nonce `
            -RunId $runId `
            -Stage "replay" `
            -ExpectedBuild $buildIdentity `
            -ExpectedWorld $WorldResource `
            -ExpectedDeliveredFingerprint $deliveredFingerprint `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $stageOutcomes.Add($replay.SafeSummary)
        Write-Output ("STAGE " +
            ($replay.SafeSummary | ConvertTo-Json -Compress))
        $journalAfterReplay = Get-CampaignJournalFileState `
            -CanonicalPath $canonicalPath `
            -RecoveryPath $recoveryPath
        if (-not (Test-CampaignJournalFileStateExact `
            -Expected $journalBeforeReplay `
            -Actual $journalAfterReplay) -or
            (Get-FileSignature -Path $carrierPath) -cne
                $carrierBeforeReplaySignature -or
            -not [bool]$replay.CampaignJournalUnchanged -or
            -not [bool]$replay.CarrierUnchanged) {
            throw "Replay changed its canonical journal, recovery journal, or carrier."
        }
        [void](Read-RebuildJournalEnvelope `
            -Path $recoveryPath `
            -ExpectedGeneration 2 `
            -ExpectedSchema $buildIdentity.CampaignSchemaVersion `
            -ExpectedPreviousGeneration 1 `
            -ExpectedPreviousFingerprint (
                [string]$prepareEnvelope.m_sSnapshotFingerprint))
        [void](Read-RebuildJournalEnvelope `
            -Path $canonicalPath `
            -ExpectedGeneration 1 `
            -ExpectedSchema $buildIdentity.CampaignSchemaVersion `
            -ExpectedPreviousGeneration 0 `
            -ExpectedPreviousFingerprint "")

        if ($stageOutcomes.Count -ne $script:ExpectedStageCount -or
            $unclaimedEngineProcessesObserved.Count -ne 0 -or
            [string]$replay.Result.m_sSourceSemanticFingerprint -cne
                $deliveredFingerprint -or
            [string]$replay.Result.m_sFinalSemanticFingerprint -cne
                $deliveredFingerprint) {
            throw "The three-process exact rebuild proof did not close its semantic chain."
        }
        $proofSummary = [pscustomobject]@{
            Success = $true
            Build = $buildIdentity.BuildSha.Substring(0, 12)
            Schema = $buildIdentity.CampaignSchemaVersion
            Settings = $buildIdentity.SettingsSchemaVersion
            World = $WorldResource
            Cut = $script:CutName
            StageCount = $stageOutcomes.Count
            PreparedFingerprintDigest =
                Get-FileNameSafeDigest $preparedFingerprint
            DeliveredFingerprintDigest =
                Get-FileNameSafeDigest $deliveredFingerprint
            RecoverSourceExact =
                [bool]$recover.Result.m_bSourceExact
            RecoverContinuationExact =
                [bool]$recover.Result.m_bContinuationExact
            RecoverDeliveryReceiptExact =
                [bool]$recover.Result.m_bDeliveryReceiptExact
            RecoverHeldGarrisonExact =
                [bool]$recover.Result.m_bHeldGarrisonExact
            RecoverResourceExactlyOnce =
                [bool]$recover.Result.m_bResourceExactlyOnce
            PhysicalBindingsExact =
                [bool]$prepare.Result.m_bPhysicalBindingsExact
            PhysicalMovementExact =
                [bool]$prepare.Result.m_bPhysicalMovementExact
            PhysicalFoldExact =
                [bool]$prepare.Result.m_bPhysicalFoldExact
            PhysicalRootCount =
                [int]$prepare.Result.m_iPhysicalRootCount
            PhysicalHandleCount =
                [int]$prepare.Result.m_iPhysicalAdapterHandleCount
            PhysicalRuntimeMemberCount =
                [int]$prepare.Result.m_iPhysicalRuntimeMemberCount
            LivePositionSampleCount =
                [int]$prepare.Result.m_iLivePositionSampleCount
            LiveMovementMeters = [Math]::Round(
                [double]$prepare.Result.m_fLiveMovementMeters,
                3)
            DistanceClosedMeters = [Math]::Round(
                [double]$prepare.Result.m_fDistanceClosedMeters,
                3)
            FoldRouteProgressMeters = [Math]::Round(
                [double]$prepare.Result.m_fFoldRouteProgressMeters,
                3)
            ReplaySemanticNoOp =
                [bool]$replay.Result.m_bSameStateSemanticNoOp
            ReplayJournalReadOnly =
                [bool]$replay.CampaignJournalUnchanged
            ReplayCarrierReadOnly = [bool]$replay.CarrierUnchanged
            RecoveryGeneration = [int]$recoveryEnvelope.m_iGeneration
            CanonicalGeneration = [int]$canonicalEnvelope.m_iGeneration
            NewestGeneration = [int]$recoveryEnvelope.m_iGeneration
            JournalChainExact = $true
        }
        $runSucceeded = $true
    }
}
catch {
    $runError = $_.Exception.Message
}
finally {
    try {
        if ($workspacePackScratchCreated) {
            if (@(Get-EngineProcessRows).Count -ne 0) {
                throw "Owned workspace scratch cannot be removed while an engine process is active."
            }
            $candidateScratchOwnership =
                Read-WorkspacePackScratchOwnership `
                    -Directory $workspacePackScratchPath `
                    -RepositoryRoot $repositoryRoot
            if ($candidateScratchOwnership -and
                $candidateScratchOwnership.Nonce -ceq $nonce -and
                $candidateScratchOwnership.OwnerPid -eq $PID -and
                $wrapperStartUtc -ne [DateTime]::MinValue -and
                $candidateScratchOwnership.OwnerStartUtc.Ticks -eq
                    $wrapperStartUtc.Ticks) {
                $workspacePackScratchOwnership = $candidateScratchOwnership
            }
            else {
                $workspacePackScratchOwnership = $null
            }
            if (-not $workspacePackScratchOwnership -or
                -not (Remove-WorkspacePackScratch `
                    -Ownership $workspacePackScratchOwnership `
                    -RepositoryRoot $repositoryRoot)) {
                throw "Owned Workbench workspace scratch removal failed."
            }
        }
    }
    catch {
        [void]$cleanupErrors.Add("remove-owned-workspace-pack-scratch")
    }
    try {
        if ($guardOwnership -and
            -not (Remove-ExactOwnedGuard $guardOwnership $guardBase)) {
            throw "The nonce-owned guard could not be removed."
        }
    }
    catch { [void]$cleanupErrors.Add("remove-owned-guard") }
    try {
        if (Test-Path -LiteralPath $guardBase -PathType Container) {
            Assert-NoReparsePathAncestry -Path $guardBase
            if (@(Get-ChildItem -LiteralPath $guardBase -Force).Count -eq 0) {
                Remove-Item -LiteralPath $guardBase -Force -ErrorAction Stop
            }
        }
    }
    catch { [void]$cleanupErrors.Add("remove-empty-guard-base") }

    $watchNew = 0
    $watchModified = 0
    $watchDeleted = 0
    $watchMissing = 0
    $spillNew = 0
    $spillModified = 0
    $spillDeleted = 0
    $spillMissing = 0
    try {
        foreach ($snapshot in $watchSnapshots) {
            $delta = Get-RootSnapshotDelta $snapshot
            $watchNew += $delta.NewEntries
            $watchModified += $delta.ModifiedFiles
            $watchDeleted += $delta.DeletedEntries
            $watchMissing += $delta.MissingRoot
        }
    }
    catch { [void]$cleanupErrors.Add("audit-watched-roots") }
    try {
        foreach ($snapshot in $spillSnapshots) {
            $delta = Get-RootSnapshotDelta $snapshot
            $spillNew += $delta.NewEntries
            $spillModified += $delta.ModifiedFiles
            $spillDeleted += $delta.DeletedEntries
            $spillMissing += $delta.MissingRoot
        }
    }
    catch { [void]$cleanupErrors.Add("audit-spill-roots") }

    $workspacePackScratchRemaining = -1
    $guardRemaining = -1
    $guardBaseRemaining = -1
    $engineProcessesRemaining = -1
    $unclaimedEngineProcessCount = -1
    $unclaimedEngineProcessKinds = @()
    try {
        $workspacePackScratchRemaining = [int](
            $workspacePackScratchCreated -and
            (Test-Path -LiteralPath $workspacePackScratchPath))
        $guardRemaining = [int](Test-Path -LiteralPath $guardRoot)
        $guardBaseRemaining = [int](Test-Path -LiteralPath $guardBase)
        $engineProcessesRemaining = @(Get-EngineProcessRows).Count
        $unclaimedEngineProcessCount =
            $unclaimedEngineProcessesObserved.Count
        $unclaimedEngineProcessKinds = @(
            $unclaimedEngineProcessesObserved |
                ForEach-Object {
                    $kind = ([string]$_ -split ':', 2)[0]
                    if ($kind -cmatch '^[A-Za-z][A-Za-z0-9_]{0,63}$') {
                        $kind
                    }
                    else {
                        "unknown"
                    }
                } |
                Sort-Object -Unique)
    }
    catch { [void]$cleanupErrors.Add("audit-final-boundaries") }
    try {
        if ($mutexAcquired -and $mutex) {
            $mutex.ReleaseMutex()
            $mutexAcquired = $false
        }
        if ($mutex) {
            $mutex.Dispose()
            $mutex = $null
        }
    }
    catch { [void]$cleanupErrors.Add("release-mutex") }

    $cleanupResult = [pscustomobject]@{
        WorkspacePackScratchRemaining = $workspacePackScratchRemaining
        GuardRemaining = $guardRemaining
        GuardBaseRemaining = $guardBaseRemaining
        EngineProcessesRemaining = $engineProcessesRemaining
        UnclaimedEngineProcessesObserved = $unclaimedEngineProcessCount
        UnclaimedEngineProcessKinds = $unclaimedEngineProcessKinds
        NewWatchedEntries = $watchNew
        ModifiedWatchedFiles = $watchModified
        DeletedWatchedEntries = $watchDeleted
        MissingWatchedRoots = $watchMissing
        NewSpillEntries = $spillNew
        ModifiedSpillFiles = $spillModified
        DeletedSpillEntries = $spillDeleted
        MissingSpillRoots = $spillMissing
        CleanupPhaseErrorCount = $cleanupErrors.Count
        CleanupPhaseErrors = $cleanupErrors.ToArray()
    }
    Write-Output ("CLEANUP " + ($cleanupResult | ConvertTo-Json -Compress))
}

$cleanupPassed = $cleanupResult -and
    $cleanupResult.WorkspacePackScratchRemaining -eq 0 -and
    $cleanupResult.GuardRemaining -eq 0 -and
    $cleanupResult.GuardBaseRemaining -eq 0 -and
    $cleanupResult.EngineProcessesRemaining -eq 0 -and
    $cleanupResult.UnclaimedEngineProcessesObserved -eq 0 -and
    $cleanupResult.NewWatchedEntries -eq 0 -and
    $cleanupResult.ModifiedWatchedFiles -eq 0 -and
    $cleanupResult.DeletedWatchedEntries -eq 0 -and
    $cleanupResult.MissingWatchedRoots -eq 0 -and
    $cleanupResult.NewSpillEntries -eq 0 -and
    $cleanupResult.ModifiedSpillFiles -eq 0 -and
    $cleanupResult.DeletedSpillEntries -eq 0 -and
    $cleanupResult.MissingSpillRoots -eq 0 -and
    $cleanupResult.CleanupPhaseErrorCount -eq 0

$safeRunError = ""
if (-not $runSucceeded) {
    if ([string]::IsNullOrWhiteSpace($runError)) {
        $runError = "Exact garrison rebuild restart proof did not complete."
    }
    $projectDirectory = ""
    if (-not [string]::IsNullOrWhiteSpace($projectFile)) {
        $projectDirectory = Split-Path -Parent $projectFile
    }
    $safeRunError = ConvertTo-SafeEvidenceLine `
        -Line $runError `
        -GuardRoot $guardRoot `
        -ProjectDirectory $projectDirectory `
        -ResolvedAddonRoots @($runtimeAddonPath)
    if ([string]::IsNullOrWhiteSpace($safeRunError)) {
        $safeRunError =
            "Exact garrison rebuild restart proof failed without safe evidence."
    }
}

if (-not $cleanupPassed) {
    # Cleanup remains the terminal exit-2 gate, but never let it erase the
    # earlier guarded-process or assertion failure that caused cleanup to run.
    if (-not [string]::IsNullOrWhiteSpace($safeRunError)) {
        [Console]::Error.WriteLine("RUN_ERROR " + $safeRunError)
    }
    [Console]::Error.WriteLine(
        "Exact garrison rebuild restart proof cleanup did not return every boundary to zero.")
    exit 2
}
if (-not $runSucceeded) {
    [Console]::Error.WriteLine($safeRunError)
    exit 1
}
if (-not $PreflightOnly) {
    if (-not $proofSummary) {
        [Console]::Error.WriteLine(
            "Exact garrison rebuild restart proof completed without a terminal summary.")
        exit 1
    }
    Write-Output ("PROOF " + ($proofSummary | ConvertTo-Json -Compress))
}
exit 0
