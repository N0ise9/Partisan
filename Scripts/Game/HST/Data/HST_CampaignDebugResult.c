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
