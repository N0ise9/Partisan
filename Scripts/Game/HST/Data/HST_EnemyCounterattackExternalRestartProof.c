// Disposable, profile-local exact-counterattack restart proof artifacts. These
// DTOs deliberately contain no machine paths or live service references.
[BaseContainerProps()]
class HST_EnemyCounterattackOutboundVirtualExpectation
{
	string m_sOrderId;
	string m_sOperationId;
	string m_sManifestId;
	string m_sManifestHash;
	string m_sBatchId;
	string m_sGroupId;
	string m_sProjectionId;
	string m_sForceId;
	string m_sFactionKey;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	string m_sDebitMutationId;
	int m_iAttackCost;
	int m_iSupportCost;
	int m_iExpectedAttackPool;
	int m_iExpectedSupportPool;
	int m_iExpectedPoolOperationalMutationCount;
	int m_iAcceptedMemberCount;
	int m_iLivingMemberCount;
	string m_sLivingSlotFingerprint;
}

[BaseContainerProps()]
class HST_EnemyCounterattackExternalRestartOwner
{
	string m_sMagic;
	int m_iVersion;
	string m_sPurpose;
	string m_sSessionNonce;
	string m_sRunId;
	string m_sRequestedCut;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	int m_iCampaignSchemaVersion;
	string m_sWorld;
	bool m_bDisposableProfile;
}

[BaseContainerProps()]
class HST_EnemyCounterattackExternalRestartGuard
{
	string m_sMagic;
	int m_iVersion;
	string m_sSessionNonce;
	string m_sStageNonce;
	string m_sRunId;
	string m_sRequestedCut;
	string m_sRequestedStage;
	int m_iStageOrdinal;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	int m_iCampaignSchemaVersion;
	string m_sWorld;
	bool m_bAllowCanonicalCampaignOverwrite;
}

[BaseContainerProps()]
class HST_EnemyCounterattackExternalRestartCarrier
{
	string m_sMagic;
	string m_sSessionNonce;
	string m_sRunId;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	int m_iCampaignSchemaVersion;
	string m_sWorld;
	string m_sCutName;
	int m_iCut;

	ref HST_EnemyCounterattackOutboundVirtualExpectation m_Expectation;
	int m_iPreparedElapsedSecond;
	float m_fPreparedRouteProgressMeters;
	float m_fPreparedRouteTotalDistanceMeters;
	vector m_vPreparedStrategicPosition;
	string m_sPreparedSemanticFingerprint;
}

[BaseContainerProps()]
class HST_EnemyCounterattackExternalRestartResult
{
	string m_sMagic;
	string m_sSessionNonce;
	string m_sRunId;
	string m_sStage;
	bool m_bSuccess;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	int m_iCampaignSchemaVersion;
	string m_sWorld;
	string m_sCutName;
	int m_iCut;
	bool m_bRestored;
	bool m_bStartupReconcileChanged;
	bool m_bSourceExact;
	bool m_bContinuationExact;
	bool m_bSameStateSemanticNoOp;
	bool m_bRuntimeClaimantsZero;
	bool m_bPersistedReadBackExact;
	float m_fProgressBeforeMeters;
	float m_fProgressAfterMeters;
	string m_sSourceSemanticFingerprint;
	string m_sFinalSemanticFingerprint;
	string m_sEvidence;
}
