#ifdef ENABLE_DIAG
// Disposable, profile-local exact enemy-garrison-rebuild restart proof
// artifacts. These DTOs contain no machine paths or live service references.
[BaseContainerProps()]
class HST_EnemyGarrisonRebuildExternalRestartExpectation
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
	string m_sDeliverySettlementKind;
	string m_sDeliverySettlementId;
	string m_sRefundMutationId;
	int m_iAttackCost;
	int m_iSupportCost;
	int m_iExpectedAttackPool;
	int m_iExpectedSupportPool;
	int m_iExpectedPendingPoolRevision;
	int m_iExpectedPendingPoolOperationalMutationCount;
	int m_iAcceptedMemberCount;
	int m_iLivingMemberCount;
	string m_sLivingSlotFingerprint;
	string m_sConfirmedCasualtySlotId;
	string m_sCasualtyTombstoneFingerprint;
	int m_iExpectedAggregateInfantry;
	int m_iExpectedAuthoritativePendingInfantry;
	int m_iExpectedAuthoritativeDeliveredInfantry;
}

[BaseContainerProps()]
class HST_EnemyGarrisonRebuildExternalRestartOwner
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
	int m_iSettingsSchemaVersion;
	string m_sWorld;
	bool m_bDisposableProfile;
}

[BaseContainerProps()]
class HST_EnemyGarrisonRebuildExternalRestartGuard
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
	int m_iSettingsSchemaVersion;
	string m_sWorld;
	bool m_bAllowCanonicalCampaignOverwrite;
}

[BaseContainerProps()]
class HST_EnemyGarrisonRebuildExternalRestartCarrier
{
	string m_sMagic;
	string m_sSessionNonce;
	string m_sRunId;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	int m_iCampaignSchemaVersion;
	int m_iSettingsSchemaVersion;
	string m_sWorld;
	string m_sCutName;
	int m_iCut;

	ref HST_EnemyGarrisonRebuildExternalRestartExpectation m_Expectation;
	int m_iPreparedElapsedSecond;
	float m_fPreMaterializationRouteProgressMeters;
	float m_fPreMaterializationRouteTotalDistanceMeters;
	float m_fPreparedRouteProgressMeters;
	float m_fPreparedRouteTotalDistanceMeters;
	vector m_vPreparedStrategicPosition;
	int m_iExpectedPhysicalRootCount;
	int m_iExpectedPhysicalAdapterHandleCount;
	int m_iExpectedPhysicalRuntimeMemberCount;
	int m_iLivePositionSampleCount;
	vector m_vInitialLivePosition;
	vector m_vFinalLivePosition;
	float m_fInitialDistanceToTargetMeters;
	float m_fFinalDistanceToTargetMeters;
	float m_fLiveMovementMeters;
	float m_fDistanceClosedMeters;
	vector m_vFoldPosition;
	float m_fFoldRouteProgressMeters;
	bool m_bPhysicalMovementExact;
	bool m_bPhysicalFoldExact;
	string m_sPreparedSemanticFingerprint;
}

[BaseContainerProps()]
class HST_EnemyGarrisonRebuildExternalRestartResult
{
	string m_sMagic;
	string m_sSessionNonce;
	string m_sStageNonce;
	string m_sRunId;
	string m_sStage;
	bool m_bSuccess;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	int m_iCampaignSchemaVersion;
	int m_iSettingsSchemaVersion;
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
	bool m_bPreparedCutExact;
	bool m_bCasualtyContinuityExact;
	bool m_bDeliveryReceiptExact;
	bool m_bHeldGarrisonExact;
	bool m_bAggregateNotDoubleCounted;
	bool m_bResourceExactlyOnce;
	bool m_bPhysicalBindingsExact;
	bool m_bPhysicalMovementExact;
	bool m_bPhysicalFoldExact;
	int m_iPhysicalRootCount;
	int m_iPhysicalAdapterHandleCount;
	int m_iPhysicalRuntimeMemberCount;
	int m_iLivePositionSampleCount;
	vector m_vInitialLivePosition;
	vector m_vFinalLivePosition;
	float m_fInitialDistanceToTargetMeters;
	float m_fFinalDistanceToTargetMeters;
	float m_fLiveMovementMeters;
	float m_fDistanceClosedMeters;
	vector m_vFoldPosition;
	float m_fFoldRouteProgressMeters;
	float m_fProgressBeforeMeters;
	float m_fProgressAfterMeters;
	string m_sSourceSemanticFingerprint;
	string m_sFinalSemanticFingerprint;
	string m_sEvidence;
}

// Frame-owned state for the one external cut that must cross native spawn,
// live movement, and fold. The carrier is process-portable; this context is
// bounded and never leaves the disposable prepare process.
class HST_EnemyGarrisonRebuildExternalPhysicalPrepareContext
{
	int m_iStage;
	int m_iSpawnTickLimit;
	int m_iSpawnWorkTicks;
	int m_iHandoffWaitTicks;
	int m_iPhysicalSettleTicks;
	int m_iMovementSampleLimit;
	int m_iLivePositionSampleCount;
	bool m_bCompleted;
	bool m_bSucceeded;
	bool m_bPhysicalBindingsExact;
	bool m_bPhysicalMovementExact;
	bool m_bPhysicalFoldExact;
	bool m_bCarrierSaved;
	bool m_bPersisted;
	bool m_bReadBackExact;
	bool m_bCasualtyContinuityExact;
	bool m_bCleanupExact;
	int m_iPhysicalRootCount;
	int m_iPhysicalAdapterHandleCount;
	int m_iPhysicalRuntimeMemberCount;
	vector m_vInitialLivePosition;
	vector m_vFinalLivePosition;
	float m_fInitialDistanceToTargetMeters;
	float m_fFinalDistanceToTargetMeters;
	float m_fLiveMovementMeters;
	float m_fDistanceClosedMeters;
	vector m_vFoldPosition;
	float m_fFoldRouteProgressMeters;
	string m_sLastSpawnSummary;
	string m_sEvidence;
	string m_sFailure;
	ref HST_EnemyGarrisonRebuildExternalRestartCarrier m_Carrier;
}
#endif
