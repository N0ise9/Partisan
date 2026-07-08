[BaseContainerProps()]
class HST_CampaignDebugMetric
{
	string m_sMetricId;
	string m_sName;
	string m_sValue;
	string m_sUnit;
	string m_sFeature;
	string m_sStage;
	string m_sMissionInstanceId;
	string m_sZoneId;
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
	string m_sMissionInstanceId;
	string m_sZoneId;
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
	HST_SupportRequestState m_Request;
	bool m_bMarkerVisibleAfterRequest;
	string m_sMarkerActualAfterRequest;
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
	bool m_bLiveGroupMarkerVisibleAfterPopulation;
	string m_sLiveGroupMarkerActualAfterPopulation;
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
	bool m_bRecallRequestedAfterRecall;
	bool m_bRouteTickChanged;
	bool m_bExitTickChanged;
	bool m_bCleanupRan;
	HST_ESupportRequestStatus m_eStatusAfterRecall = HST_ESupportRequestStatus.HST_SUPPORT_QUEUED;
	HST_ESupportRequestStatus m_eStatusAfterExit = HST_ESupportRequestStatus.HST_SUPPORT_QUEUED;
	string m_sRuntimeStatusAfterRecall;
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
	string m_sSerializedLoadoutFailure;
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
	bool m_bPhysicalInventoryCapacityAvailable;
	bool m_bPhysicalInventoryReflected;
	bool m_bPhysicalInventoryRestored;
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
	bool m_bResourceTickChanged;
	bool m_bCommanderTickChanged;
	string m_sOrderIds;
	string m_sOrderTypes;
	string m_sSupportRequestIds;
}

class HST_CampaignDebugEscalationProbeContext
{
	bool m_bArranged;
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
	string m_sReport;
}
