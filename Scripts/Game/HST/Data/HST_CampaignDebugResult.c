#ifdef ENABLE_DIAG
[BaseContainerProps()]
class HST_CampaignDebugMetric
{
	string m_sMetricId;
	string m_sName;
	string m_sValue;
	string m_sUnit;
	string m_sFeature;
	string m_sStage;
	string m_sMissionId;
	string m_sMissionInstanceId;
	string m_sZoneId;
	string m_sObjectiveId;
	string m_sRuntimePrimitive;
	string m_sOrderId;
}

[BaseContainerProps()]
class HST_CampaignDebugAssertion
{
	string m_sAssertionId;
	string m_sExpected;
	string m_sActual;
	string m_sStatus;
	string m_sFailureReason;
	string m_sProofLevel;
	string m_sObservedPath;
	string m_sRequiredPath;
	bool m_bCountsTowardCertification;
	vector m_vExpectedPosition;
	vector m_vActualPosition;
	float m_fDistanceMeters;
	string m_sEntityId;
	string m_sMissionId;
	string m_sMissionInstanceId;
	string m_sZoneId;
	string m_sObjectiveId;
	string m_sRuntimePrimitive;
	string m_sOrderId;
}

[BaseContainerProps()]
class HST_CampaignDebugCaseResult
{
	string m_sCaseId;
	string m_sCategory;
	string m_sFeature;
	string m_sStage;
	string m_sStatus;
	string m_sReason;
	int m_iStartSecond;
	int m_iEndSecond;
	ref array<ref HST_CampaignDebugAssertion> m_aAssertions = {};
	ref array<ref HST_CampaignDebugMetric> m_aMetrics = {};
	ref array<string> m_aEvidence = {};
}

[BaseContainerProps()]
class HST_CampaignDebugRunResult
{
	string m_sRunId;
	string m_sProfile;
	string m_sCampaignSeed;
	string m_sPlayerIdentityId;
	string m_sWorldName;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	string m_sMarkerPrefix;
	string m_sMissionPrefix;
	string m_sEntityTag;
	int m_iStartedAtSecond;
	int m_iEndedAtSecond;
	int m_iPassCount;
	int m_iWarnCount;
	int m_iFailCount;
	int m_iBlockedCount;
	int m_iSkippedCount;
	int m_iCertificationRequiredCount;
	int m_iCertificationProvenCount;
	int m_iCertificationFailCount;
	int m_iCertificationBlockedCount;
	int m_iCertificationWarnCount;
	bool m_bCertificationPassed;
	ref array<ref HST_CampaignDebugCaseResult> m_aCases = {};
	ref array<ref HST_CampaignDebugMetric> m_aMetrics = {};
	ref array<string> m_aArtifacts = {};
}

class HST_CampaignDebugSupportProbeContext
{
	string m_sLabel;
	HST_ESupportRequestType m_eExpectedType;
	string m_sCommandResult;
	int m_iCountBefore;
	int m_iMoneyBefore;
	int m_iHRBefore;
	int m_iMoneyAfterExactIssue;
	int m_iHRAfterExactIssue;
	int m_iMoneyAfterExactConfirmation;
	int m_iHRAfterExactConfirmation;
	int m_iMoneyAfterExactCleanup;
	int m_iHRAfterExactCleanup;
	string m_sExactIssueRequestId;
	string m_sExactConfirmationRequestId;
	string m_sExactQuoteId;
	bool m_bExactGraphValidated;
	string m_sExactGraphEvidence;
	bool m_bExactCleanupAccepted;
	bool m_bExactCleanupTerminal;
	bool m_bExactStrategicProjectionHeld;
	string m_sExactCleanupEvidence;
	int m_iGarageVehicleCountBefore;
	int m_iGarageVehicleCountAfter;
	string m_sSelectedGarageVehicleId;
	bool m_bGarageVehicleRemoved;
	HST_SupportRequestState m_Request;
	bool m_bMarkerVisibleAfterRequest;
	string m_sMarkerActualAfterRequest;
	string m_sMarkerIconAfterRequest;
	bool m_bRuntimeProbeRan;
	int m_iEtaRemainingBefore = -1;
	int m_iEtaRemainingAfter = -1;
	HST_ESupportRequestStatus m_eStatusBeforeTick = HST_ESupportRequestStatus.HST_SUPPORT_QUEUED;
	HST_ESupportRequestStatus m_eStatusAfterTick = HST_ESupportRequestStatus.HST_SUPPORT_QUEUED;
	string m_sRuntimeStatusBeforeTick;
	string m_sRuntimeStatusAfterTick;
	bool m_bPhysicalizedBeforeTick;
	bool m_bPhysicalizedAfterTick;
	string m_sGroupIdAfterTick;
	string m_sGroupStatusAfterTick;
	bool m_bRuntimeSpawnProbeRanBeforePopulation;
	bool m_bRuntimeSpawnProbeChangedBeforePopulation;
	string m_sGroupStatusAfterSpawnProbe;
	bool m_bRuntimeSpawnedBeforePopulation;
	bool m_bPendingPopulationAttemptedBeforeRoute;
	bool m_bPendingPopulationResolvedBeforeRoute;
	string m_sGroupStatusBeforePopulation;
	string m_sGroupStatusAfterPopulation;
	string m_sPendingPopulationEvidence;
	bool m_bRuntimeGroupSnapshotCapturedBeforeFold;
	vector m_vGroupTargetBeforeFold;
	string m_sGroupActualBeforeFold;
	int m_iEditableSizeBeforeFold = -1;
	bool m_bRuntimeMemberCountsVisibleBeforeFold;
	bool m_bResponseRunBeforeFold;
	string m_sResponseRunActualBeforeFold;
	bool m_bLiveGroupMarkerVisibleAfterPopulation;
	string m_sLiveGroupMarkerActualAfterPopulation;
	bool m_bRoadblockSupportMarkerVisibleAfterPopulation;
	string m_sRoadblockSupportMarkerActualAfterPopulation;
	bool m_bRouteTickChanged;
	bool m_bArrivalRouteTickChanged;
	bool m_bArrivalTickChanged;
	int m_iRouteAdvanceSeconds;
	int m_iArrivalAdvanceSeconds;
	vector m_vGroupPositionBefore;
	vector m_vGroupPositionAfter;
	vector m_vGroupPositionAtArrival;
	float m_fDistanceBefore;
	float m_fDistanceAfter;
	float m_fDistanceAtArrival;
	int m_iRouteSampleCount;
	int m_iRouteMovementCount;
	int m_iRouteDistanceDecreaseCount;
	int m_iRouteTimeoutSeconds;
	float m_fRouteMaxMovementMeters;
	float m_fRouteMaxDistanceClosedMeters;
	bool m_bRouteTimedOut;
	string m_sRouteSampleHistory;
	string m_sRouteLastObserved;
	string m_sRouteTimeoutEvidence;
	string m_sGroupStatusAfterRoute;
	string m_sGroupStatusAtArrival;
	string m_sRequestRuntimeStatusAtArrival;
	bool m_bTerminalStatusInjected;
	bool m_bTerminalTickChanged;
	string m_sGroupStatusBeforeTerminal;
	string m_sGroupStatusAfterTerminal;
	bool m_bLiveGroupMarkerVisibleAfterTerminal;
	string m_sLiveGroupMarkerActualAfterTerminal;
	HST_ESupportRequestStatus m_eStatusAfterTerminal = HST_ESupportRequestStatus.HST_SUPPORT_QUEUED;
	string m_sRequestRuntimeStatusAfterTerminal;
	string m_sResolutionKindAfterTerminal;
	bool m_bOutcomeAppliedAfterTerminal;
	bool m_bAbstractResolvedAfterTerminal;
	int m_iResolvedAtSecondAfterTerminal;
	bool m_bRuntimeEntityCleaned;
	int m_iSharedElapsedSecondBefore = -1;
	int m_iSyntheticElapsedSecondPeak = -1;
	int m_iSharedElapsedSecondAfter = -1;
	int m_iFutureTimestampNormalizationCount;
	bool m_bSyntheticClockRestored;
	bool m_bFutureTimestampIsolationExact;
	string m_sSyntheticClockEvidence;
	string m_sEnemyStrategicAuthorityBefore;
	string m_sEnemyStrategicAuthorityAfter;
	bool m_bEnemyStrategicAuthorityIsolated;
}

class HST_CampaignDebugSupportCancelProbeContext
{
	string m_sSeedResult;
	string m_sCancelResult;
	string m_sRequestId;
	int m_iCountBefore;
	int m_iPendingBeforeClear;
	int m_iPendingAfterPreClear;
	int m_iPendingAfterSeed;
	int m_iPendingAfterCancel;
	int m_iPendingAfterCleanup;
	int m_iTotalAfterCleanup;
	bool m_bRequestCreated;
	int m_iCancelSecond;
	HST_ESupportRequestStatus m_eStatusAfterCancel = HST_ESupportRequestStatus.HST_SUPPORT_QUEUED;
	string m_sRuntimeStatusAfterCancel;
	string m_sFailureReasonAfterCancel;
	string m_sResolutionKindAfterCancel;
	int m_iResolvedAtSecondAfterCancel;
	int m_iCooldownAfterCancel;
	string m_sRuntimeStatusAfterCleanup;
	string m_sResolutionKindAfterCleanup;
	int m_iCooldownAfterCleanup;
}

class HST_CampaignDebugSupportRecallProbeContext
{
	string m_sSeedResult;
	string m_sRecallResult;
	string m_sRequestId;
	string m_sGroupId;
	int m_iCountBefore;
	int m_iPendingBeforeClear;
	int m_iPendingAfterPreClear;
	int m_iPendingAfterSeed;
	int m_iPendingAfterRecall;
	int m_iPendingAfterExit;
	int m_iPendingAfterCleanup;
	int m_iMoneyBefore;
	int m_iMoneyAfterSeed;
	int m_iHRBefore;
	int m_iHRAfterSeed;
	int m_iHRAfterRecall;
	int m_iHRAfterExit;
	int m_iHRCost;
	int m_iPlannedInfantryCount;
	int m_iRefundedHR;
	int m_iSurvivorsBeforeRecall;
	int m_iRecallSecond;
	int m_iRecallRouteAdvanceSeconds;
	bool m_bRequestCreated;
	bool m_bGroupCreated;
	bool m_bRecallCommandAccepted;
	bool m_bRecallCommandStateChanged;
	bool m_bRecallRequestedAfterRecall;
	bool m_bRouteTickChanged;
	bool m_bExitTickChanged;
	bool m_bCleanupRan;
	HST_ESupportRequestStatus m_eStatusAfterRecall = HST_ESupportRequestStatus.HST_SUPPORT_QUEUED;
	HST_ESupportRequestStatus m_eStatusAfterExit = HST_ESupportRequestStatus.HST_SUPPORT_QUEUED;
	string m_sRuntimeStatusAfterRecall;
	string m_sRecallDisposition;
	string m_sRecallFailureReason;
	string m_sRecallOperationId;
	string m_sRuntimeStatusAfterExit;
	string m_sGroupStatusBeforeRecall;
	string m_sGroupStatusAfterRecall;
	string m_sGroupStatusAfterExit;
	string m_sResolutionKindAfterExit;
	vector m_vExitPosition;
	string m_sSupportReportAfterRecall;
	string m_sSupportReportAfterExit;
}

class HST_CampaignDebugIncomeProbeContext
{
	string m_sSeedResult;
	string m_sCommandResult;
	string m_sEconomyReport;
	string m_sSeedZoneId;
	int m_iMoneyBefore;
	int m_iMoneyAfter;
	int m_iHRBefore;
	int m_iHRAfter;
	int m_iTimerBefore;
	int m_iTimerAfter;
	int m_iExpectedMoneyIncome;
	int m_iExpectedHRIncome;
	int m_iEnemyMoneyIncome;
	int m_iResistanceIncomeZoneCount;
}

class HST_CampaignDebugVehicleLoadoutProbeContext
{
	string m_sReport;
	string m_sVehicleId;
	string m_sCapturedVehicleId;
	string m_sRuntimeVehicleId;
	string m_sIdentityId;
	string m_sGarageReportBefore;
	string m_sVehicleCargoReportBefore;
	string m_sLoadoutReportBefore;
	string m_sGarageReportAfter;
	string m_sVehicleCargoReportAfter;
	string m_sLoadoutReportAfter;
	string m_sRedeployResult;
	string m_sCaptureResult;
	string m_sOpenLoadoutResult;
	string m_sCloseLoadoutResult;
	string m_sPostApplyCloseResult;
	string m_sValidLoadoutId;
	string m_sInvalidLoadoutId;
	string m_sPhysicalLoadoutId;
	string m_sRestoreLoadoutId;
	string m_sValidLoadoutApplyResult;
	string m_sInvalidLoadoutApplyResult;
	string m_sPhysicalLoadoutApplyResult;
	string m_sRestoreLoadoutApplyResult;
	string m_sPhysicalCleanupRestoreResult;
	string m_sOriginalSerializedLoadout;
	string m_sSerializedLoadoutFailure;
	string m_sPhysicalCarrierPrefabBefore;
	string m_sPhysicalCarrierPreconditionFailure;
	string m_sPhysicalRestoreVerificationFailure;
	string m_sPhysicalDraftLocationsAfterApply;
	string m_sPhysicalDraftLocationsAfterRestore;
	string m_sStoredVehicleHeatReasonBeforeRedeploy;
	string m_sRuntimeVehicleHeatReasonAfterRedeploy;
	string m_sCapturedVehicleHeatReasonAfterCapture;
	string m_sCapturedVehicleHeatRoundTripActual;
	string m_sFiniteArsenalCategoryBefore;
	string m_sFiniteArsenalDisplayNameBefore;
	string m_sStorageArsenalCategoryBefore;
	string m_sStorageArsenalDisplayNameBefore;
	string m_sFiniteIssuedCategoryBefore;
	string m_sFiniteIssuedDisplayNameBefore;
	string m_sStorageIssuedCategoryBefore;
	string m_sStorageIssuedDisplayNameBefore;
	bool m_bStoreResult;
	bool m_bTeleportedToHQ;
	bool m_bStoredRecordFound;
	bool m_bStoredPrefabValid;
	bool m_bStoredPrefixValid;
	bool m_bRuntimeRecordFound;
	bool m_bCapturedRecordFound;
	bool m_bStoredVehicleHeatArranged;
	bool m_bStoredVehicleReportedBeforeRedeploy;
	bool m_bStoredVehicleCoverBeforeRedeploy;
	bool m_bRuntimeVehicleHeatPreserved;
	bool m_bRuntimeVehicleReportedAfterRedeploy;
	bool m_bRuntimeVehicleCoverAfterRedeploy;
	bool m_bCapturedVehicleHeatPreserved;
	bool m_bCapturedVehicleReportedAfterCapture;
	bool m_bCapturedVehicleCoverAfterCapture;
	bool m_bCapturedVehicleHeatRoundTrip;
	bool m_bSerializedLoadoutAvailable;
	bool m_bValidLoadoutSeeded;
	bool m_bInvalidLoadoutSeeded;
	bool m_bPhysicalLoadoutSeeded;
	bool m_bRestoreLoadoutSeeded;
	bool m_bPhysicalInventoryAccessible;
	bool m_bPhysicalDirectCapacityAvailable;
	bool m_bPhysicalTemporaryCarrierCapacityAvailable;
	bool m_bPhysicalInventoryCapacityAvailable;
	bool m_bPhysicalInventoryReflected;
	bool m_bPhysicalInventoryRestored;
	bool m_bPhysicalCarrierPreconditioned;
	bool m_bPhysicalPrimarySerializedLoadoutRestored;
	bool m_bPhysicalSerializedLoadoutRestored;
	bool m_bPhysicalCarrierCleanupComplete;
	bool m_bPhysicalDraftAccessible;
	bool m_bPhysicalDraftReflected;
	bool m_bPhysicalDraftRestored;
	bool m_bFiniteArsenalExistedBefore;
	bool m_bFiniteArsenalUnlockedBefore;
	bool m_bFiniteArsenalUnlockedAfterSeed;
	bool m_bStorageArsenalExistedBefore;
	bool m_bStorageArsenalUnlockedBefore;
	bool m_bFiniteIssuedExistedBefore;
	bool m_bFiniteIssuedInfiniteBefore;
	bool m_bStorageIssuedExistedBefore;
	bool m_bStorageIssuedInfiniteBefore;
	bool m_bFiniteIssuedInfiniteAfterValidApply;
	bool m_bLoadoutApplySnapshotCaptured;
	bool m_bLoadoutCleanupRestoredArsenal;
	bool m_bLoadoutCleanupRestoredStorageArsenal;
	bool m_bLoadoutCleanupRestoredIssued;
	IEntity m_PhysicalDetachedCarrierEntity;
	int m_iGarageCountBefore;
	int m_iGarageCountAfterStore;
	int m_iGarageCountAfterRedeploy;
	int m_iGarageCountAfterCapture;
	int m_iGarageCountAfterCleanup;
	int m_iRuntimeVehicleCountBefore;
	int m_iRuntimeVehicleCountAfterRedeploy;
	int m_iRuntimeVehicleCountAfterCleanup;
	int m_iVehicleCargoCountBefore;
	int m_iVehicleCargoCountAfterRedeploy;
	int m_iVehicleCargoCountAfterCleanup;
	int m_iStoredCargoAfterArrange;
	int m_iRestoredCargoAfterRedeploy;
	int m_iStoredVehicleHeatBeforeRedeploy;
	int m_iRuntimeVehicleHeatAfterRedeploy;
	int m_iCapturedVehicleHeatAfterCapture;
	int m_iStoredPassengerCompromisesBeforeRedeploy;
	int m_iRuntimePassengerCompromisesAfterRedeploy;
	int m_iCapturedPassengerCompromisesAfterCapture;
	int m_iGarageCleanupRemoved;
	int m_iRuntimeCleanupRemoved;
	int m_iCargoCleanupRemoved;
	int m_iMoneyBeforeRedeploy;
	int m_iMoneyAfterRedeploy;
	int m_iRedeployCost;
	int m_iSavedLoadoutsBefore;
	int m_iSavedLoadoutsAfterOpen;
	int m_iSavedLoadoutsAfterClose;
	int m_iSavedLoadoutsAfterApplySeed;
	int m_iSavedLoadoutsAfterLoadoutCleanup;
	int m_iIssuedItemsBefore;
	int m_iIssuedItemsAfterOpen;
	int m_iIssuedItemsAfterClose;
	int m_iIssuedItemsBeforeApply;
	int m_iIssuedItemsAfterValidApply;
	int m_iIssuedItemsAfterInvalidApply;
	int m_iIssuedItemsAfterLoadoutCleanup;
	int m_iFiniteArsenalCountBefore;
	int m_iFiniteArsenalCountAfterSeed;
	int m_iFiniteArsenalCountAfterValidApply;
	int m_iFiniteArsenalCountAfterInvalidApply;
	int m_iFiniteArsenalCountAfterPhysicalSeed;
	int m_iFiniteArsenalCountAfterPhysicalApply;
	int m_iFiniteArsenalCountAfterLoadoutCleanup;
	int m_iStorageArsenalCountBefore;
	int m_iStorageArsenalCountAfterPhysicalSeed;
	int m_iStorageArsenalCountAfterLoadoutCleanup;
	int m_iFiniteIssuedCountBefore;
	int m_iStorageIssuedCountBefore;
	int m_iFiniteIssuedCountAfterValidApply;
	int m_iFiniteIssuedCountAfterInvalidApply;
	int m_iFiniteIssuedCountAfterPhysicalApply;
	int m_iFiniteIssuedCountAfterLoadoutCleanup;
	int m_iPhysicalInventoryCountBefore;
	int m_iPhysicalInventoryCountAfterApply;
	int m_iPhysicalInventoryCountAfterRestore;
	int m_iPhysicalDraftCountBefore;
	int m_iPhysicalDraftCountAfterApply;
	int m_iPhysicalDraftCountAfterRestore;
	int m_iPhysicalDraftStorageItemCountBefore;
	int m_iPhysicalDraftStorageItemCountAfterApply;
	int m_iPhysicalDraftStorageItemCountAfterRestore;
	int m_iLoadoutSavedCleanupRemoved;
	int m_iDraftSlotsAfterOpen;
	int m_iDraftNodesAfterOpen;
	int m_iOpenPlayerId;
	string m_sLoadoutStatusAfterOpen;
	string m_sLoadoutStatusAfterClose;
	bool m_bLoadoutSessionAfterOpen;
	bool m_bLoadoutSessionAfterClose;
	bool m_bLiveCharacterAfterOpen;
	ref array<ref HST_ArsenalItemState> m_aArsenalSnapshot = {};
	ref array<ref HST_IssuedLoadoutItemState> m_aIssuedLoadoutSnapshot = {};
}

class HST_CampaignDebugGarrisonProbeContext
{
	string m_sResult;
	string m_sRecruitResult;
	string m_sRemoveResult;
	string m_sRecruitRoundTripActual;
	string m_sZoneId;
	string m_sResistanceFactionKey;
	string m_sOriginalOwnerFactionKey;
	string m_sOwnerAfterCleanup;
	bool m_bOriginalActive;
	bool m_bArranged;
	bool m_bHadGarrisonBefore;
	bool m_bRemovedCreatedEmptyGarrison;
	bool m_bRestoredZoneState;
	bool m_bRecruitSaveRoundTrip;
	int m_iOriginalActiveInfantry;
	int m_iOriginalActiveVehicles;
	int m_iGarrisonSlots;
	int m_iGarrisonRecordsBefore;
	int m_iGarrisonRecordsAfterRecruit;
	int m_iGarrisonRecordsAfterRemove;
	int m_iGarrisonRecordsAfterCleanup;
	int m_iInfantryBefore;
	int m_iVehiclesBefore;
	int m_iInfantryAfterRecruit;
	int m_iVehiclesAfterRecruit;
	int m_iInfantryAfterRemove;
	int m_iVehiclesAfterRemove;
	int m_iInfantryAfterCleanup;
	int m_iVehiclesAfterCleanup;
	int m_iMoneyBefore;
	int m_iMoneyAfterRecruit;
	int m_iMoneyAfterRemove;
	int m_iRestoredMoneyAfterRecruit;
	int m_iHRBefore;
	int m_iHRAfterRecruit;
	int m_iHRAfterRemove;
	int m_iRestoredHRAfterRecruit;
	int m_iRestoredInfantryAfterRecruit;
	int m_iRestoredVehiclesAfterRecruit;
	int m_iRestoredSchemaVersionAfterRecruit;
}

class HST_CampaignDebugEnemyOrderPhysicalProbeContext
{
	HST_EnemyOrderState m_Order;
	HST_SupportRequestState m_SupportRequest;
	HST_ActiveGroupState m_Group;
	bool m_bPhysicalizeTickChanged;
	bool m_bSupportTickChanged;
	bool m_bSyncTickChanged;
	bool m_bSpawnTickChanged;
	bool m_bRouteTickChanged;
	bool m_bTargetWasActive;
	bool m_bTargetActiveRestored;
	bool m_bPendingPopulationAttemptedBeforeRoute;
	bool m_bPendingPopulationResolvedBeforeRoute;
	int m_iElapsedBefore;
	int m_iElapsedAfter;
	int m_iSyntheticElapsedPeak;
	int m_iFutureTimestampNormalizationCount;
	bool m_bSyntheticClockRestored;
	bool m_bFutureTimestampIsolationExact;
	string m_sEnemyStrategicAuthorityBefore;
	string m_sEnemyStrategicAuthorityAfter;
	bool m_bEnemyStrategicAuthorityIsolated;
	string m_sSyntheticClockEvidence;
	int m_iRouteAdvanceSeconds;
	vector m_vGroupPositionBefore;
	vector m_vGroupPositionAfter;
	float m_fDistanceBefore;
	float m_fDistanceAfter;
	int m_iRouteSampleCount;
	int m_iRouteMovementCount;
	int m_iRouteDistanceDecreaseCount;
	int m_iRouteTimeoutSeconds;
	float m_fRouteMaxMovementMeters;
	float m_fRouteMaxDistanceClosedMeters;
	bool m_bRouteTimedOut;
	string m_sRouteSampleHistory;
	string m_sRouteLastObserved;
	string m_sRouteTimeoutEvidence;
	string m_sGroupStatusBeforePopulation;
	string m_sGroupStatusAfterPopulation;
	string m_sPendingPopulationEvidence;
	string m_sGroupStatusAfterTick;
	string m_sGroupStatusAfterRoute;
	string m_sFailureReason;
}

class HST_CampaignDebugClockIsolationContext
{
	int m_iSharedElapsedSecondBefore = -1;
	int m_iSyntheticElapsedSecondPeak = -1;
	int m_iSharedElapsedSecondAfter = -1;
	string m_sEnemyStrategicAuthorityBefore = "not_captured";
	string m_sEnemyStrategicAuthorityAfter = "not_captured";
	bool m_bSharedClockIsolated;
	bool m_bEnemyStrategicAuthorityIsolated;
}

// Frozen pointer and origin authority for one target-zone group created by the
// synchronous mission-start/zone-activation transaction. Cleanup may fold this
// exact row, but never a later target-zone addition inferred only by location.
class HST_CampaignDebugRenderBubbleZoneGroupOwnership
{
	ref HST_ActiveGroupState m_Group;
	string m_sGroupId;
	string m_sOperationId;
	string m_sZoneId;
	string m_sFactionKey;
	string m_sMissionInstanceId;
	string m_sGarrisonZoneId;
	bool m_bMissionOwned;
	bool m_bZoneActivationOwned;

	bool MatchesExactOrigin(HST_ActiveGroupState group)
	{
		return group && group == m_Group
			&& group.m_sGroupId == m_sGroupId
			&& group.m_sOperationId == m_sOperationId
			&& group.m_sZoneId == m_sZoneId
			&& group.m_sFactionKey == m_sFactionKey
			&& group.m_sMissionInstanceId == m_sMissionInstanceId
			&& group.m_sGarrisonZoneId == m_sGarrisonZoneId;
	}
}

// Exact process-local composition row created by the synchronous far-zone
// activation transaction. The service validates these frozen rows before any
// broad zone cleanup can delete composition entities.
class HST_CampaignDebugRenderBubbleZoneCompositionOwnership
{
	IEntity m_Entity;
	string m_sZoneId;
	string m_sSlotId;
	string m_sPrefab;
}

// Immutable process-local receipt for every typed mission row that is allowed
// to survive render-bubble containment. The filtered save projection proves
// complete row content while the strong Managed references prove that a
// same-ID archive was not removed and replaced between cleanup retries.
class HST_CampaignDebugRenderBubbleMissionSurvivorSnapshot
{
	string m_sMissionInstanceId;
	string m_sProjectionFingerprint;
	string m_sEvidence;
	ref HST_CampaignSaveData m_Projection;
	ref array<ref Managed> m_aRetainedRowPointers = {};
	ref array<string> m_aRetainedRowKinds = {};
	int m_iRetainedAuthorityRows;
	int m_iProjectedAuthorityRows;
	bool m_bDefendPetrosAuthority;
}

// The mission-target render-bubble proof owns production mission/runtime state
// across ordinary coordinator frames. Counts captured here are native runtime
// observations; the probe never advances the shared campaign clock itself.
class HST_CampaignDebugRenderBubbleMissionTargetContext
{
	ref HST_CampaignDebugCaseResult m_Case;
	ref array<string> m_aPreStartMissionInstanceIds = {};
	ref array<string> m_aOwnedNewMissionInstanceIds = {};
	ref array<ref HST_CampaignDebugRenderBubbleZoneGroupOwnership>
		m_aSynchronousOwnedZoneGroups = {};
	ref array<ref HST_CampaignDebugRenderBubbleZoneCompositionOwnership>
		m_aSynchronousOwnedZoneCompositions = {};
	ref array<ref HST_CampaignDebugRenderBubbleMissionSurvivorSnapshot>
		m_aReleasedMissionSurvivors = {};
	ref HST_ActiveMissionState m_Mission;
	ref HST_ZoneState m_OriginalZone;
	ref HST_GarrisonState m_OriginalGarrison;
	IEntity m_OriginalPlayerEntity;
	RplId m_OriginalPlayerReplicationId = RplId.Invalid();
	string m_sMissionDefinitionId;
	string m_sMissionInstanceId;
	string m_sPreStartMissionInstanceIds;
	string m_sPostStartNewMissionInstanceIds;
	string m_sZoneId;
	string m_sZoneOwnerFactionKey;
	string m_sZoneCaseStatus;
	string m_sZoneCaseReason;
	string m_sSampleHistory;
	string m_sLastSample;
	string m_sFailureReason;
	string m_sZoneDeactivationEvidence;
	string m_sSynchronousZoneGroupEvidence;
	string m_sSynchronousZoneCompositionEvidence;
	string m_sZoneRuntimeRegistryEvidence;
	string m_sOwnedMissionAdmissionActual;
	string m_sSettledMissionActual;
	string m_sPlayerRestoreSafetyEvidence;
	string m_sReleasedMissionSurvivorEvidence;
	string m_sCleanupMissionCompletionSummary;
	string m_sCleanupMissionAuthorityExample;
	string m_sCleanupMissionRemainingExample;
	string m_sLastRecoveryFailureFingerprint;
	ref array<string> m_aRecoveryFailureFingerprints = {};
	vector m_vOriginalPlayerPosition;
	vector m_aOriginalPlayerTransform[4];
	vector m_vFarPosition;
	int m_iOriginalActiveInfantry;
	int m_iOriginalActiveVehicles;
	int m_iOriginalGarrisonInfantry;
	int m_iOriginalGarrisonVehicles;
	int m_iOriginalZoneOwnershipRevision;
	int m_iPlayerId = -1;
	int m_iStartSecond = -1;
	int m_iLastSampleSecond = -1;
	int m_iDeadlineSecond = -1;
	int m_iSampleCount;
	int m_iSampleHistoryCount;
	int m_iMissionAssetCount;
	int m_iMissionSpawnedAssetCount;
	int m_iMissionRuntimeEntityCount;
	int m_iMissionRuntimeHandleCount;
	int m_iMissionGroupCount;
	int m_iMissionGroupHandleCount;
	int m_iZoneGroupCount;
	int m_iZoneSpawnedGroupCount;
	int m_iZonePendingGroupCount;
	int m_iPeakMissionAssetCount;
	int m_iPeakMissionSpawnedAssetCount;
	int m_iPeakMissionRuntimeHandleCount;
	int m_iPeakMissionGroupCount;
	int m_iPeakMissionGroupHandleCount;
	int m_iPreStartMissionInstanceCount;
	int m_iPostStartNewMissionInstanceCount;
	int m_iCleanupZoneGroupCountBefore;
	int m_iCleanupZoneRuntimeHandleCountBefore;
	int m_iUnexpectedZoneGroupCount;
	int m_iUnexpectedZoneRuntimeHandleCount;
	int m_iCleanupOwnedMissionCount;
	int m_iCleanupRemainingMissionGroups;
	int m_iCleanupRemainingRuntimeEntities;
	int m_iCleanupRemainingMissionAssets;
	int m_iCleanupRetainedAuthorityRows;
	int m_iCleanupUnsafeAuthorityRows;
	int m_iCleanupRemainingTransientRecords;
	int m_iPlayerRestoreAppliedSecond = -1;
	int m_iPlayerRestoreStableSampleSecond = -1;
	int m_iLastCleanupAttemptSecond = -1;
	int m_iRecoveryFailureRepeatCount;
	float m_fActivationRadius;
	float m_fPlayerEventBubbleRadius;
	float m_fFarDistance;
	float m_fPlayerRestoreMaximumTransformDelta = -1.0;
	bool m_bTerminal;
	bool m_bStartAccepted;
	bool m_bStarted;
	bool m_bGlobalMissionIdentityExact;
	bool m_bNewMissionInstanceOwned;
	bool m_bOriginalPlayerPositionCaptured;
	bool m_bOriginalPlayerTransformCaptured;
	bool m_bOriginalPlayerOnFootGrounded;
	bool m_bOriginalActive;
	bool m_bHadOriginalGarrison;
	bool m_bFarTeleport;
	bool m_bFarTeleportAttempted;
	bool m_bPreStartZoneTickAttempted;
	bool m_bStartAttempted;
	bool m_bFarPlayerSessionExact;
	bool m_bFarPlayerOutsideEventBubble;
	bool m_bTargetOutsideAllPlayerEventBubbles;
	bool m_bTargetOutsideAllPlayerEventBubblesEverySample;
	bool m_bFarInactive;
	bool m_bMissionActiveObserved;
	bool m_bZoneForcedActiveObserved;
	bool m_bAssetsPhysicalObserved;
	bool m_bGuardsPhysicalObserved;
	bool m_bReady;
	bool m_bTimedOut;
	bool m_bPlayerLost;
	bool m_bCleanupAttempted;
	bool m_bCleanupExact;
	bool m_bCleanupCompletedAll;
	bool m_bCleanupMissionSetExact;
	bool m_bCleanupMissionNotActive;
	bool m_bCleanupReleasedMissionSurvivorSetExact;
	bool m_bCleanupPostZoneReleaseSurvivorExact;
	bool m_bCleanupZoneRestored;
	bool m_bCleanupGarrisonRestored;
	bool m_bMissionOwnershipReleased;
	bool m_bReleasedMissionSurvivorSetFrozen;
	bool m_bZoneRuntimeReleased;
	bool m_bZoneOwnershipStable;
	bool m_bSynchronousZoneGroupSetFrozen;
	bool m_bSynchronousZoneCompositionSetFrozen;
	bool m_bPreStartZoneRuntimeEmptyExact;
	bool m_bPreStartZoneCompositionEmptyExact;
	bool m_bRuntimeOwnershipExact;
	bool m_bCompositionOwnershipExact;
	bool m_bSettleOutcomeLatched;
	bool m_bSettleOutcomeExact;
	bool m_bCleanupPendingPlayerRestore;
	bool m_bPlayerRestoreTeleportApplied;
	bool m_bPlayerRestoreApplied;
	bool m_bPlayerRestored;
	bool m_bZoneCompositionReleased;
	bool m_bFatalRetainedOwner;
}

// The mission sweep owns the exact set difference produced by one admin start
// request and carries an explicit mission-area combat result into the primitive
// proof. No latest/global result lookup is allowed across these staged frames.
class HST_CampaignDebugMissionSweepContext
{
	ref array<string> m_aPreStartMissionInstanceIds = {};
	ref array<ref HST_ActiveMissionState> m_aPreStartMissionPointers = {};
	ref array<string> m_aPreStartMissionDefinitionIds = {};
	ref array<string> m_aOwnedNewMissionInstanceIds = {};
	ref array<ref HST_ActiveMissionState> m_aOwnedNewMissionPointers = {};
	ref array<string> m_aOwnedNewMissionDefinitionIds = {};
	ref array<bool> m_aOwnedNewMissionCleanupMutationStarted = {};
	ref HST_CampaignDebugCaseResult m_PhysicalCombatCase;
	ref HST_ActiveMissionState m_Mission;
	ref HST_MissionObjectiveState m_Objective;
	ref HST_ZoneState m_Zone;
	string m_sMissionId;
	string m_sMissionInstanceId;
	string m_sZoneId;
	string m_sZoneOwnerFactionKey;
	string m_sRuntimePrimitive;
	string m_sObjectiveId;
	string m_sStartResult;
	string m_sStartCorrelationEvidence;
	string m_sCleanupOwnershipEvidence;
	string m_sPhysicalCombatStartResult;
	string m_sPhysicalCombatFailureReason;
	vector m_vObjectivePosition;
	int m_iPhysicalCombatObserverPlayerId = -1;
	IEntity m_PhysicalCombatObserverEntity;
	RplId m_PhysicalCombatObserverReplicationId = RplId.Invalid();
	float m_fPhysicalCombatObserverCaptureBoundary = -1.0;
	float m_fPhysicalCombatObserverActivationRadius = -1.0;
	int m_iZoneOwnershipRevision = -1;
	HST_EMissionObjectiveType m_eObjectiveType;
	int m_iPreStartMissionInstanceCount;
	int m_iPostStartNewMissionInstanceCount;
	bool m_bStartAccepted;
	bool m_bGlobalMissionIdentityExact;
	bool m_bStartCorrelationExact;
	bool m_bRuntimeCaseRecorded;
	bool m_bPhysicalCombatRequired;
	bool m_bPhysicalCombatStartAttempted;
	bool m_bPhysicalCombatObserverReady;
	bool m_bPhysicalCombatStarted;
	bool m_bPhysicalCombatFinished;
	bool m_bPhysicalCombatCleanupExact;
	bool m_bContainmentExact;
	bool m_bFatalRetainedOwner;
}

// Phase 20 retains one selected town and its ambient runtime across ordinary
// frames so native compartment, engine, and waypoint acknowledgements can run.
class HST_CampaignDebugCivilianPopulationContext
{
	ref HST_CampaignDebugCaseResult m_Case;
	ref HST_CampaignDebugCivilianProbeRuntimeResult m_Probe;
	ref HST_CivilianProjectionProofSummary m_ProjectionProof;
	string m_sLabel;
	string m_sCommandResult;
	string m_sZoneId;
	string m_sLastReadinessEvidence;
	string m_sFailureReason;
	string m_sZoneHoldStartEvidence;
	string m_sZoneHoldObservationEvidence;
	string m_sZoneHoldCleanupEvidence;
	vector m_vTownPosition;
	float m_fPlayerDistance;
	float m_fPopulationRadius;
	int m_iSpawnFailuresBefore;
	int m_iInitialCivilianCharacters;
	int m_iCurrentCivilianCharacters;
	int m_iCurrentTrafficVehicles;
	int m_iAdmittedPedestrians;
	int m_iReadyPedestrians;
	int m_iRecoveringPedestrians;
	int m_iAdmittedTraffic;
	int m_iReadyTraffic;
	int m_iRecoveringTraffic;
	int m_iZoneHoldSampleCount;
	int m_iZoneHoldDemandSampleCount;
	int m_iCleanupAttemptCount;
	bool m_bTerminal;
	bool m_bRuntimeOwned;
	bool m_bZoneHoldOwned;
	bool m_bZoneHoldObservationExact;
	bool m_bAmbientCleanupExact;
	bool m_bZoneHoldCleanupExact;
	bool m_bInitialRuntimeChanged;
	bool m_bTeleported;
	bool m_bReady;
	bool m_bTimedOut;
	bool m_bTerminalCleanupRecorded;
	bool m_bFinalAssertionsRecorded;
	bool m_bCleanupAssertionsRecorded;
	bool m_bCaseRecorded;
	bool m_bCleanupHalted;
}

class HST_CampaignDebugFailedActionProbeContext
{
	bool m_bRan;
	string m_sMoveHQResult;
	string m_sStartMissionResult;
	string m_sCompleteMissionResult;
	string m_sSnapshotBefore;
	string m_sSnapshotAfter;
	int m_iActiveMissionsBefore;
	int m_iActiveMissionsAfter;
	int m_iObjectivesBefore;
	int m_iObjectivesAfter;
	int m_iMissionAssetsBefore;
	int m_iMissionAssetsAfter;
	int m_iSupportRequestsBefore;
	int m_iSupportRequestsAfter;
	int m_iEnemyOrdersBefore;
	int m_iEnemyOrdersAfter;
	int m_iMarkersBefore;
	int m_iMarkersAfter;
}

class HST_CampaignDebugEscalationProfileResult
{
	string m_sLabel;
	int m_iWarLevel;
	int m_iAggressionSeed;
	int m_iAttackBefore;
	int m_iAttackAfterResourceTick;
	int m_iAttackAfterCommanderTick;
	int m_iSupportBefore;
	int m_iSupportAfterResourceTick;
	int m_iSupportAfterCommanderTick;
	int m_iAggressionBefore;
	int m_iAggressionAfter;
	int m_iOrdersBefore;
	int m_iOrdersAfter;
	int m_iOrdersCreated;
	int m_iSupportRequestsBefore;
	int m_iSupportRequestsAfter;
	int m_iSupportRequestsCreated;
	int m_iActiveGroupsBefore;
	int m_iActiveGroupsAfter;
	int m_iActiveGroupsCreated;
	int m_iPhysicalizationTargetZonesActivated;
	int m_iAttackIncomeDelta;
	int m_iSupportIncomeDelta;
	int m_iCycleCount;
	int m_iResourceTicksChanged;
	int m_iCommanderTicksChanged;
	int m_iOpenOrdersResolvedBetweenCycles;
	int m_iOpenOrdersAfterCycles;
	int m_iDebugTaggedOrders;
	int m_iLedgerStableOrders;
	int m_iVersionedStableOrders;
	int m_iRuntimeOwnerLegacyOrders;
	int m_iRuntimeOwnerExactQRFOrders;
	int m_iRuntimeOwnerExactCounterattackOrders;
	int m_iRuntimeOwnerExactPatrolOrders;
	int m_iRuntimeOwnerExactGarrisonRebuildOrders;
	int m_iRuntimeOwnerQuarantinedOrders;
	int m_iRuntimeOwnerUnsupportedOrders;
	int m_iRuntimeOwnerSnapshotInvariantFailures;
	int m_iLegacyPhysicalizableOrders;
	int m_iLegacySupportLinkedOrders;
	int m_iLegacySupportLinkedGroups;
	int m_iExactCounterattackOpenOrders;
	int m_iExactCounterattackTerminalLedgers;
	int m_iExactCounterattackInvalidAuthorityRows;
	int m_iExactCounterattackProjectionGroups;
	int m_iExactCounterattackVirtualGroups;
	int m_iExactCounterattackMaterializingGroups;
	int m_iExactCounterattackPhysicalGroups;
	int m_iExactCounterattackDematerializingGroups;
	int m_iExactCounterattackSupportLeaks;
	bool m_bOrderIsolationReady;
	bool m_bResourceTickChanged;
	bool m_bCommanderTickChanged;
	bool m_bOrderIdentitiesSafe;
	string m_sOrderIsolationFailure;
	string m_sOrderIds;
	string m_sOrderTypes;
	string m_sSupportRequestIds;
	string m_sRuntimeOwnerEvidence;
	ref array<string> m_aRuntimeOwnerSampleOrderIds = {};
	ref array<string> m_aRuntimeOwnerSampleOwners = {};
}

class HST_CampaignDebugEscalationProbeContext
{
	bool m_bArranged;
	bool m_bOrderIsolationReady;
	string m_sOrderIsolationFailure;
	ref HST_CampaignDebugEscalationProfileResult m_Low;
	ref HST_CampaignDebugEscalationProfileResult m_Mid;
	ref HST_CampaignDebugEscalationProfileResult m_High;
	ref HST_CampaignDebugEscalationProfileResult m_MultiCycle;
	bool m_bDecayChanged;
	int m_iDecayBefore;
	int m_iDecayAfter;
	int m_iDecayElapsedSeconds;
	int m_iDecayAmount;
	int m_iDecayEnemyPoolCount;
	int m_iExpectedDecayTotal;
	int m_iRuntimeOwnerExpectedOrders;
	int m_iRuntimeOwnerClassifiedOrders;
	int m_iRuntimeOwnerInvalidOrders;
	int m_iRuntimeOwnerSnapshotInvariantFailures;
	int m_iExactCounterattackOrders;
	int m_iExactCounterattackOpenOrders;
	int m_iExactCounterattackTerminalLedgers;
	int m_iExactCounterattackInvalidAuthorityRows;
	int m_iExactCounterattackProjectionGroups;
	int m_iExactCounterattackVirtualGroups;
	int m_iExactCounterattackMaterializingGroups;
	int m_iExactCounterattackPhysicalGroups;
	int m_iExactCounterattackDematerializingGroups;
	int m_iExactCounterattackSupportLeaks;
	int m_iLegacyPhysicalizableOrders;
	int m_iLegacySupportLinkedOrders;
	int m_iLegacySupportLinkedGroups;
	string m_sRuntimeOwnerEvidence;
	string m_sReport;
}

class HST_CampaignDebugExactCounterattackProjectionProbeContext
{
	ref HST_ForceSpawnAdapterDebugState m_AdapterStateBefore;
	IEntity m_CasualtyEntity;
	string m_sOrderId;
	string m_sOperationId;
	string m_sManifestId;
	string m_sSpawnResultId;
	string m_sGroupId;
	string m_sTargetOwnerBefore;
	string m_sTargetOwnerAfter;
	string m_sCasualtySlotId;
	string m_sCasualtyEntityId;
	string m_sPhysicalSettleEvidence;
	string m_sCasualtyReentryPhysicalSettleEvidence;
	string m_sSurvivorReentryPhysicalSettleEvidence;
	string m_sCasualtySettleEvidence;
	string m_sFailureReason;
	string m_sEvidence;
	string m_sLivingSlotFingerprintBefore;
	string m_sLivingSlotFingerprintPhysical;
	string m_sLivingSlotFingerprintAfter;
	string m_sLivingSlotFingerprintCasualty;
	string m_sLivingSlotFingerprintCasualtyFold;
	string m_sLivingSlotFingerprintReentry;
	string m_sLivingSlotFingerprintFinal;
	string m_sDebitValidationFailure;
	vector m_vOriginalPlayerPosition;
	vector m_vNearPosition;
	vector m_vFarPosition;
	int m_iExpectedLiving;
	int m_iStrategicLivingBefore;
	int m_iPhysicalLiving;
	int m_iPhysicalRuntimeMembers;
	int m_iStrategicLivingAfter;
	int m_iRuntimeMembersAfter;
	int m_iCasualtyLiving;
	int m_iCasualtyFoldLiving;
	int m_iReentryLiving;
	int m_iReentryRuntimeMembers;
	int m_iFinalLiving;
	int m_iFinalRuntimeMembers;
	int m_iAdapterHandlesBefore;
	int m_iAdapterResultHandlesBefore;
	int m_iAdapterHandlesPhysical;
	int m_iAdapterHandlesAfter;
	int m_iAdapterResultHandlesAfter;
	int m_iCasualtyHandles;
	int m_iCasualtyResultHandles;
	int m_iCasualtyFoldHandles;
	int m_iCasualtyFoldResultHandles;
	int m_iReentryHandles;
	int m_iReentryRetiredSlotHandles;
	int m_iFinalHandles;
	int m_iFinalResultHandles;
	int m_iSupportRowsBefore;
	int m_iSupportRowsAfter;
	int m_iSupportRowsFinal;
	int m_iMutationCountBefore;
	int m_iMutationCountAfter;
	int m_iMutationCountFinal;
	int m_iOwnershipTransitionCountBefore;
	int m_iOwnershipTransitionCountAfter;
	int m_iAttackResourcesBefore;
	int m_iAttackResourcesAfter;
	int m_iAttackResourcesFinal;
	int m_iSupportResourcesBefore;
	int m_iSupportResourcesAfter;
	int m_iSupportResourcesFinal;
	int m_iSpawnTicks;
	int m_iSpawnTickLimit;
	int m_iSpawnDeferredTicks;
	int m_iSpawnHandoffPendingTicks;
	int m_iSpawnCycleTicks;
	int m_iSpawnCycleHandoffPendingTicks;
	int m_iPhysicalSettleTicks;
	int m_iCasualtyReentryPhysicalSettleTicks;
	int m_iSurvivorReentryPhysicalSettleTicks;
	int m_iCasualtySettleTicks;
	int m_iCasualtyCleanupTicks;
	int m_iStage;
	int m_iElapsedBefore;
	int m_iElapsedPeak;
	int m_iElapsedAfter;
	int m_iForceSpawnRuntimeClockBefore;
	int m_iForceSpawnRuntimeClockAfter;
	int m_iEmergencyCleanupRows;
	bool m_bBaselineExact;
	bool m_bNearTeleport;
	bool m_bEnterDecisionExact;
	bool m_bMaterializeTickChanged;
	bool m_bMaterializingExact;
	bool m_bSpawnSucceeded;
	bool m_bPhysicalTickChanged;
	bool m_bPhysicalExact;
	bool m_bPhysicalBindingsExact;
	bool m_bFarTeleport;
	bool m_bLeaveDecisionExact;
	bool m_bFoldTickChanged;
	bool m_bFoldExact;
	bool m_bCasualtyReentryTeleport;
	bool m_bCasualtyReentryDecisionExact;
	bool m_bCasualtyReentryTickChanged;
	bool m_bCasualtyReentryMaterializingExact;
	bool m_bCasualtyReentrySpawnSucceeded;
	bool m_bCasualtyReentryPhysicalTickChanged;
	bool m_bCasualtyReentryPhysicalExact;
	bool m_bCasualtyKillIssued;
	bool m_bCasualtyEntityDead;
	bool m_bCasualtyReconcileExact;
	bool m_bCasualtyReplayExact;
	bool m_bCasualtyRetiredSlotExact;
	bool m_bCasualtyCorpseDetached;
	bool m_bCasualtyCorpseDeleted;
	bool m_bCasualtyFoldTeleport;
	bool m_bCasualtyFoldDecisionExact;
	bool m_bCasualtyFoldTickChanged;
	bool m_bCasualtyFoldExact;
	bool m_bSurvivorReentryTeleport;
	bool m_bSurvivorReentryDecisionExact;
	bool m_bSurvivorReentryTickChanged;
	bool m_bSurvivorReentryMaterializingExact;
	bool m_bSurvivorReentrySpawnSucceeded;
	bool m_bSurvivorReentryPhysicalTickChanged;
	bool m_bSurvivorReentryPhysicalExact;
	bool m_bSurvivorReentryBindingsExact;
	bool m_bSurvivorRetiredSlotAbsent;
	bool m_bFinalFoldTeleport;
	bool m_bFinalFoldDecisionExact;
	bool m_bFinalFoldTickChanged;
	bool m_bFinalFoldExact;
	bool m_bCasualtyContinuityExact;
	bool m_bIdentityExact;
	bool m_bManifestExact;
	bool m_bRosterIdentityExact;
	bool m_bDebitAuthorityExact;
	bool m_bResourceExact;
	bool m_bNoSupportLeak;
	bool m_bFinalCaptured;
	bool m_bSharedClockIsolationExact;
	bool m_bAdapterStateRestored;
	bool m_bOriginalPlayerPositionCaptured;
	bool m_bPlayerRestored;
	bool m_bEmergencyCleanupAttempted;
	bool m_bEmergencyCleanupExact;
	bool m_bCompleted;
	string m_sLastSpawnSummary;
}
#endif
