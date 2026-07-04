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
	HST_SupportRequestState m_Request;
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
	bool m_bStoreResult;
	bool m_bTeleportedToHQ;
	bool m_bStoredRecordFound;
	bool m_bStoredPrefabValid;
	bool m_bStoredPrefixValid;
	bool m_bRuntimeRecordFound;
	bool m_bCapturedRecordFound;
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
	int m_iGarageCleanupRemoved;
	int m_iRuntimeCleanupRemoved;
	int m_iCargoCleanupRemoved;
	int m_iMoneyBeforeRedeploy;
	int m_iMoneyAfterRedeploy;
	int m_iRedeployCost;
	int m_iSavedLoadoutsBefore;
	int m_iSavedLoadoutsAfterOpen;
	int m_iSavedLoadoutsAfterClose;
	int m_iIssuedItemsBefore;
	int m_iIssuedItemsAfterOpen;
	int m_iIssuedItemsAfterClose;
	int m_iDraftSlotsAfterOpen;
	int m_iDraftNodesAfterOpen;
	int m_iOpenPlayerId;
	string m_sLoadoutStatusAfterOpen;
	string m_sLoadoutStatusAfterClose;
	bool m_bLoadoutSessionAfterOpen;
	bool m_bLoadoutSessionAfterClose;
	bool m_bLiveCharacterAfterOpen;
}

class HST_CampaignDebugGarrisonProbeContext
{
	string m_sResult;
	string m_sRecruitResult;
	string m_sRemoveResult;
	string m_sZoneId;
	string m_sResistanceFactionKey;
	string m_sOriginalOwnerFactionKey;
	string m_sOwnerAfterCleanup;
	bool m_bOriginalActive;
	bool m_bArranged;
	bool m_bHadGarrisonBefore;
	bool m_bRemovedCreatedEmptyGarrison;
	bool m_bRestoredZoneState;
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
	int m_iHRBefore;
	int m_iHRAfterRecruit;
	int m_iHRAfterRemove;
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
	int m_iAttackIncomeDelta;
	int m_iSupportIncomeDelta;
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
	bool m_bDecayChanged;
	int m_iDecayBefore;
	int m_iDecayAfter;
	int m_iDecayElapsedSeconds;
	int m_iDecayAmount;
	int m_iDecayEnemyPoolCount;
	int m_iExpectedDecayTotal;
	string m_sReport;
}
