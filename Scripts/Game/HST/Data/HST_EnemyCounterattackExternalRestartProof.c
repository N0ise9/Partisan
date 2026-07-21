#ifdef ENABLE_DIAG
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
	string m_sExpectedSourceOwnerFactionKey;
	int m_iExpectedSourceOwnershipRevision;
	string m_sExpectedTargetOwnerFactionKey;
	int m_iExpectedTargetOwnershipRevision;
	string m_sDebitMutationId;
	int m_iAttackCost;
	int m_iSupportCost;
	int m_iExpectedAttackPool;
	int m_iExpectedSupportPool;
	int m_iExpectedPoolOperationalMutationCount;
	int m_iAcceptedMemberCount;
	int m_iLivingMemberCount;
	string m_sLivingSlotFingerprint;
	bool m_bExpectedLivingSlotsEverAlive;
	int m_iExpectedNormalizedSlotAttemptCount;
	string m_sConfirmedCasualtySlotId;
	string m_sCasualtyTombstoneFingerprint;
	int m_iExpectedNormalizedReprojectionCount;
}

[BaseContainerProps()]
class HST_EnemyCounterattackPreparedSettlementExpectation
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
	string m_sExpectedSourceOwnerFactionKey;
	int m_iExpectedSourceOwnershipRevision;
	string m_sExpectedTargetOwnerFactionKey;
	int m_iExpectedTargetOwnershipRevision;
	string m_sDebitMutationId;
	string m_sSettlementKind;
	string m_sSettlementId;
	string m_sRefundMutationId;
	string m_sReason;
	int m_iAttackCost;
	int m_iSupportCost;
	int m_iAccepted;
	int m_iSurvivors;
	int m_iAttackRefund;
	int m_iSupportRefund;
	int m_iExpectedAttackPool;
	int m_iExpectedSupportPool;
	int m_iExpectedPoolRevision;
	int m_iExpectedPoolOperationalMutationCount;
	string m_sExpectedLastStrategicMutationId;
	int m_iPreparedAtSecond;
	int m_iExpectedTerminalRevision;
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
	bool m_bNativeSourceSelectionProof;
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
	bool m_bNativeSourceSelectionProof;
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
	vector m_vInjectedStalePosition;
	vector m_vPreparedLivePosition;
	int m_iExpectedPhysicalAdapterHandleCount;
	int m_iExpectedPhysicalRuntimeMemberCount;
	string m_sPreparedSemanticFingerprint;
	string m_sRawPreparedCutSemanticFingerprint;
	bool m_bNativeSourceSelectionProof;
	string m_sNativeSavePointId;
	string m_sFallbackConflictSemanticFingerprint;

	int m_iAccepted;
	int m_iCasualties;
	int m_iSurvivors;
	int m_iAttackRefund;
	int m_iSupportRefund;
	int m_iAttackBeforeRefund;
	int m_iSupportBeforeRefund;
	int m_iPreparedAtSecond;
	int m_iPrefixRevision;
	int m_iExpectedPrefixMutationCount;
	int m_iExpectedPrefixAttackDelta;
	int m_iExpectedPrefixSupportDelta;
	bool m_bExpectedPrefixReceiptApplied;
	string m_sSettlementKind;
	string m_sSettlementId;
	string m_sRefundMutationId;
	string m_sReason;

	ref HST_EnemyCounterattackPreparedSettlementExpectation m_SettlementExpectation;
	string m_sPreparedSettlementFingerprint;
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
	bool m_bOwnershipStartupReconcileChanged;
	bool m_bSourceExact;
	bool m_bContinuationExact;
	bool m_bSameStateSemanticNoOp;
	bool m_bRuntimeClaimantsZero;
	bool m_bPersistedReadBackExact;
	bool m_bPreparedCutExact;
	bool m_bCasualtyContinuityExact;
	bool m_bPhysicalBindingsExact;
	bool m_bLivePositionRefreshExact;
	bool m_bPhysicalCaptureNormalizedExact;
	int m_iPhysicalAdapterHandleCount;
	int m_iPhysicalRuntimeMemberCount;
	vector m_vInjectedStalePosition;
	vector m_vPreparedLivePosition;
	float m_fProgressBeforeMeters;
	float m_fProgressAfterMeters;
	string m_sSourceSemanticFingerprint;
	string m_sFinalSemanticFingerprint;
	string m_sRawPreparedCutSemanticFingerprint;
	bool m_bNativeSourceSelectionProof;
	bool m_bNativePersistenceLoaded;
	bool m_bFallbackConflictRejected;
	bool m_bNativeSavePointCommitted;
	string m_sNativeSavePointId;
	string m_sRestoreSource;
	string m_sFallbackConflictSemanticFingerprint;
	string m_sEvidence;
}

// Frame-owned state for the one external cut that must cross the native spawn
// handoff. The carrier remains process-portable; this context never leaves the
// disposable prepare process.
class HST_EnemyCounterattackExternalPhysicalPrepareContext
{
	int m_iStage;
	int m_iSpawnTickLimit;
	int m_iSpawnWorkTicks;
	int m_iHandoffWaitTicks;
	int m_iPhysicalSettleTicks;
	bool m_bCompleted;
	bool m_bSucceeded;
	bool m_bPhysicalBindingsExact;
	bool m_bLivePositionRefreshExact;
	bool m_bPhysicalCaptureNormalizedExact;
	bool m_bCarrierSaved;
	bool m_bPersisted;
	bool m_bReadBackExact;
	bool m_bCasualtyContinuityExact;
	bool m_bCleanupExact;
	int m_iPhysicalAdapterHandleCount;
	int m_iPhysicalRuntimeMemberCount;
	vector m_vInjectedStalePosition;
	vector m_vPreparedLivePosition;
	string m_sLastSpawnSummary;
	string m_sEvidence;
	string m_sFailure;
	ref HST_EnemyCounterattackExternalRestartCarrier m_Carrier;
}
#endif
