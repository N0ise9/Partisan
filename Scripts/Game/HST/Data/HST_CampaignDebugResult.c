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
}
