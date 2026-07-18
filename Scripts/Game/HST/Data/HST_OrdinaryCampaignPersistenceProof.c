// Disposable, profile-local ordinary campaign persistence proof artifacts.
// These DTOs contain only process-portable evidence and never machine paths or
// live engine/service references.
[BaseContainerProps()]
class HST_OrdinaryCampaignPersistenceOwner
{
	string m_sMagic;
	int m_iVersion;
	string m_sPurpose;
	string m_sSessionNonce;
	string m_sRunId;
	string m_sPayloadNonce;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	int m_iCampaignSchemaVersion;
	int m_iSettingsSchemaVersion;
	string m_sWorld;
	int m_iExpectedStageCount;
	bool m_bDisposableProfile;
	bool m_bMixedNativeProofRequired;
}

[BaseContainerProps()]
class HST_OrdinaryCampaignPersistenceGuard
{
	string m_sMagic;
	int m_iVersion;
	string m_sSessionNonce;
	string m_sStageNonce;
	string m_sRunId;
	string m_sPayloadNonce;
	string m_sRequestedStage;
	int m_iStageOrdinal;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	int m_iCampaignSchemaVersion;
	int m_iSettingsSchemaVersion;
	string m_sWorld;
	string m_sExpectedSource;
	int m_iExpectedSentinelGeneration;
	string m_sExpectedSaveType;
	string m_sExpectedSaveName;
	bool m_bAllowCanonicalCampaignOverwrite;
	bool m_bMixedNativeProofRequired;
}

[BaseContainerProps()]
class HST_OrdinaryCampaignPersistenceCarrier
{
	string m_sMagic;
	int m_iVersion;
	string m_sSessionNonce;
	string m_sRunId;
	string m_sPayloadNonce;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	int m_iCampaignSchemaVersion;
	int m_iSettingsSchemaVersion;
	string m_sWorld;

	string m_sSentinelTaskId;
	int m_iCurrentSentinelGeneration;
	string m_sGeneration1SentinelFingerprint;
	string m_sGeneration2SentinelFingerprint;
	string m_sGeneration3SentinelFingerprint;

	string m_sAutosaveSavePointId;
	string m_sManualSavePointId;
	string m_sShutdownSavePointId;
	string m_sPriorSavePointId;
	string m_sCurrentSavePointId;

	string m_sAutosaveSource;
	string m_sAutosaveSourceFingerprint;
	bool m_bAutosaveWasDataLoaded;
	bool m_bAutosaveNativeRecordPresent;
	bool m_bAutosaveNativeRecordValid;
	string m_sManualSource;
	string m_sManualSourceFingerprint;
	bool m_bManualWasDataLoaded;
	bool m_bManualNativeRecordPresent;
	bool m_bManualNativeRecordValid;
	string m_sShutdownSource;
	string m_sShutdownSourceFingerprint;
	bool m_bShutdownWasDataLoaded;
	bool m_bShutdownNativeRecordPresent;
	bool m_bShutdownNativeRecordValid;

	string m_sAutosaveCreatedSaveType;
	string m_sAutosaveCreatedSaveName;
	string m_sManualCreatedSaveType;
	string m_sManualCreatedSaveName;
	string m_sShutdownCreatedSaveType;
	string m_sShutdownCreatedSaveName;

	string m_sGeneration1ProfileFallbackFingerprint;
	string m_sGeneration2ProfileFallbackFingerprint;
	string m_sGeneration3ProfileFallbackFingerprint;
	bool m_bGeneration1ProfileFallbackExact;
	bool m_bGeneration2ProfileFallbackExact;
	bool m_bGeneration3ProfileFallbackExact;
	string m_sLatestProfileFallbackFingerprint;

	// Stable fixture plan and monotonic cross-process proof receipts for the
	// physical durable-field-vehicle portion of the same persistence chain.
	string m_sFieldVehiclePrefab;
	string m_sFieldVehicleCargoPrefab;
	string m_sFieldVehicleAId;
	string m_sFieldVehicleBId;
	vector m_vFieldVehicleAInitialPosition;
	vector m_vFieldVehicleBInitialPosition;
	vector m_vFieldVehicleAMovedPosition;
	vector m_vFieldVehicleAShutdownPosition;
	vector m_vFieldVehicleAInitialAngles;
	vector m_vFieldVehicleBInitialAngles;
	vector m_vFieldVehicleAMovedAngles;
	vector m_vFieldVehicleAShutdownAngles;
	int m_iFieldVehicleACargoCount;
	int m_iFieldVehicleBCargoCount;
	bool m_bFieldVehiclePrepared;
	bool m_bFieldVehicleRecoveredAndMutated;
	bool m_bFieldVehicleReplayVerified;

	// Generation-three contract for the mixed native controlled-shutdown
	// fixture. The explicit required bit makes older/default JSON fail closed
	// once this proof is part of the ordinary persistence chain.
	bool m_bMixedNativeProofRequired;
	string m_sMixedNativeMissionInstanceId;
	string m_sMixedNativeOperationId;
	string m_sMixedNativeManifestId;
	string m_sMixedNativeBatchId;
	string m_sMixedNativeGuardGroupId;
	string m_sMixedNativeCarrierRuntimeId;
	string m_sMixedNativeCarrierPrefab;
	string m_sMixedNativeFollowingCaptiveId;
	string m_sMixedNativeBoardingCaptiveId;
	string m_sMixedNativeBoardedCaptiveId;
	string m_sMixedNativeSeatToken;
	string m_sMixedNativeShutdownFingerprint;
	vector m_vMixedNativeCarrierShutdownPosition;
	vector m_vMixedNativeCarrierShutdownAngles;
	vector m_vMixedNativeFollowingShutdownPosition;
	vector m_vMixedNativeBoardingShutdownPosition;
	vector m_vMixedNativeBoardedShutdownPosition;
	int m_iMixedNativeCaptiveCount;
	int m_iMixedNativeCarrierCount;
	int m_iMixedNativeActiveGroupCount;
	int m_iMixedNativeGuardLivingCount;
	int m_iMixedNativeAdapterHandleCount;
	bool m_bMixedNativeShutdownPrepared;
}

[BaseContainerProps()]
class HST_OrdinaryCampaignPersistenceResult
{
	string m_sMagic;
	int m_iVersion;
	string m_sSessionNonce;
	string m_sStageNonce;
	string m_sRunId;
	string m_sPayloadNonce;
	string m_sStage;
	int m_iStageOrdinal;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	int m_iCampaignSchemaVersion;
	int m_iSettingsSchemaVersion;
	string m_sWorld;
	bool m_bSuccess;

	string m_sExpectedSource;
	string m_sSource;
	string m_sSourceFingerprint;
	bool m_bSourceExact;
	bool m_bPersistenceSystemAvailable;
	bool m_bWasDataLoaded;
	bool m_bNativeRecordPresent;
	bool m_bNativeRecordValid;
	bool m_bDegradedNativeRecovery;
	string m_sDegradedNativeRecoveryReason;
	int m_iSourceJournalGeneration = -1;
	string m_sSourceJournalSlot;
	int m_iSourceJournalValidSlotCount;
	bool m_bSourceJournalLegacyRaw;
	bool m_bSourceJournalChainExact;

	int m_iExpectedSentinelGeneration;
	int m_iSentinelGeneration;
	string m_sExpectedSentinelFingerprint;
	string m_sSentinelFingerprint;
	bool m_bSentinelExact;

	string m_sExpectedPriorSavePointId;
	string m_sObservedPriorSavePointId;
	string m_sActiveSaveType;
	string m_sActiveSaveName;
	string m_sExpectedCurrentSavePointId;
	string m_sCreatedSavePointId;
	string m_sActiveSavePointId;
	bool m_bPriorSavePointExact;
	bool m_bActiveSavePointExact;

	string m_sExpectedSaveType;
	string m_sCreatedSaveType;
	string m_sExpectedSaveName;
	string m_sCreatedSaveName;
	bool m_bNativePayloadPrepared;
	bool m_bSavePointRequested;
	int m_iExpectedRequestFlags;
	int m_iObservedRequestFlags;
	bool m_bRequestFlagsExact;
	bool m_bCompletionCallbackSucceeded;
	bool m_bOnAfterSaveObserved;
	bool m_bOnAfterSaveSucceeded;
	bool m_bOnSaveCreatedObserved;
	bool m_bSchedulerExercised;
	bool m_bSchedulerThresholdCrossed;
	bool m_bSchedulerMajorChangePendingAtAttempt;
	bool m_bSchedulerDebounceRemarked;
	bool m_bSchedulerDebounceHeld;
	string m_sSchedulerOrigin;
	int m_iSchedulerAttemptSequence;
	int m_iSchedulerTickCountAtAttempt;
	int m_iSchedulerAutosaveIntervalSeconds;
	int m_iSchedulerMajorChangeDebounceSeconds;
	float m_fSchedulerCumulativeSecondsAtAttempt;
	float m_fSchedulerDebounceRemarkElapsedSeconds;
	float m_fSchedulerAutosaveElapsedBeforeSeconds;
	float m_fSchedulerAutosaveElapsedAtAttemptSeconds;
	float m_fSchedulerMajorChangeElapsedBeforeSeconds;
	float m_fSchedulerMajorChangeElapsedAtAttemptSeconds;

	string m_sExpectedProfileFallbackFingerprint;
	string m_sProfileFallbackReadBackFingerprint;
	bool m_bProfileFallbackReadBackExact;
	int m_iProfileJournalGeneration = -1;
	string m_sProfileJournalSlot;
	int m_iProfileJournalValidSlotCount;
	bool m_bProfileJournalLegacyRaw;
	bool m_bProfileJournalChainExact;

	string m_sFieldVehicleProofPhase;
	int m_iFieldVehicleExpectedDurableRows;
	int m_iFieldVehicleObservedDurableRows;
	int m_iFieldVehicleExpectedLiveRoots;
	int m_iFieldVehicleObservedLiveRoots;
	int m_iFieldVehicleExpectedDeletedRows;
	int m_iFieldVehicleObservedDeletedRows;
	int m_iFieldVehicleExpectedCargoRows;
	int m_iFieldVehicleObservedCargoRows;
	int m_iFieldVehicleRestoreEligibleRows;
	int m_iFieldVehicleRestoreInactiveRows;
	int m_iFieldVehicleRetiredNativeTombstoneRoots;
	int m_iFieldVehicleRestoreAdoptedRoots;
	int m_iFieldVehicleRestoreSpawnedRoots;
	int m_iFieldVehicleRestoreTrackedRoots;
	int m_iFieldVehicleRestoreFailedRows;
	int m_iFieldVehicleRestoreAmbiguousRows;
	int m_iFieldVehicleNativeTrackedRoots;
	int m_iFieldVehicleShutdownQuiescedRoots;
	bool m_bFieldVehicleRestoreExact;
	bool m_bFieldVehicleStateExact;
	bool m_bFieldVehiclePhysicalExact;
	bool m_bFieldVehicleCargoExact;
	bool m_bFieldVehicleNoDuplicateRoots;
	bool m_bFieldVehicleNativeAuthorityDetached;
	bool m_bFieldVehicleShutdownQuiescenceRequired;
	bool m_bFieldVehicleShutdownQuiescenceExact;
	bool m_bFieldVehicleMutationApplied;
	bool m_bFieldVehicleProofExact;
	string m_sFieldVehicleEvidence;

	// Stage-scoped mixed native shutdown/restart proof. Shutdown-only receipts
	// stay false in restart verification; portable logical evidence remains
	// exact across native and profile-fallback recovery without pointer identity.
	bool m_bMixedNativeProofRequired;
	string m_sMixedNativeProofPhase;
	string m_sMixedNativeExpectedFingerprint;
	string m_sMixedNativeObservedFingerprint;
	int m_iMixedNativeExpectedCaptiveCount;
	int m_iMixedNativeObservedCaptiveCount;
	int m_iMixedNativeExpectedCarrierCount;
	int m_iMixedNativeObservedCarrierCount;
	int m_iMixedNativeExpectedActiveGroupCount;
	int m_iMixedNativeObservedActiveGroupCount;
	int m_iMixedNativeExpectedGuardLivingCount;
	int m_iMixedNativeObservedGuardLivingCount;
	int m_iMixedNativeExpectedAdapterHandleCount;
	int m_iMixedNativeObservedAdapterHandleCount;
	bool m_bMixedNativeClientConnected;
	bool m_bMixedNativePlayerSpawned;
	bool m_bMixedNativeForeignOccupantRejected;
	bool m_bMixedNativeForeignOccupantCleanupExact;
	bool m_bMixedNativePlayerReleaseRejected;
	bool m_bMixedNativePlayerReleased;
	bool m_bMixedNativeProductionRetryObserved;
	bool m_bMixedNativeReadOnlyPreflightExact;
	bool m_bMixedNativeLatchesClearOnRejection;
	bool m_bMixedNativeFollowingExact;
	bool m_bMixedNativeSeatlessBoardingExact;
	bool m_bMixedNativeBoardedSeatExact;
	bool m_bMixedNativeCarrierScopeExact;
	bool m_bMixedNativeActiveGroupExact;
	bool m_bMixedNativeLootLatchExact;
	bool m_bMixedNativeActiveGroupLatchExact;
	bool m_bMixedNativeFieldVehicleLatchExact;
	bool m_bMixedNativeRescueLatchExact;
	bool m_bMixedNativeMaintainExact;
	bool m_bMixedNativeQuiescenceExact;
	bool m_bMixedNativeFieldVehicleCorrelationExact;
	bool m_bMixedNativeDurableCountsExact;
	bool m_bMixedNativePoseExact;
	bool m_bMixedNativeTopologyExact;
	bool m_bMixedNativeLogicalFingerprintExact;
	bool m_bMixedNativeProofExact;
	string m_sMixedNativeEvidence;
	string m_sEvidence;
}

// Process-portable signal emitted by the shutdown fixture only after its
// service-owned identity contract is exact and immediately before the server
// starts waiting for the real client. The runner can therefore launch the
// client from a correlated artifact instead of timing against a log line.
[BaseContainerProps()]
class HST_OrdinaryCampaignMixedNativeReadyReceipt
{
	string m_sMagic;
	int m_iVersion;
	string m_sSessionNonce;
	string m_sStageNonce;
	string m_sRunId;
	string m_sPayloadNonce;
	string m_sStage;
	int m_iStageOrdinal;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	int m_iCampaignSchemaVersion;
	int m_iSettingsSchemaVersion;
	string m_sWorld;
	string m_sPhase;
	bool m_bReady;
	string m_sEvidence;
}

// Process-portable receipt emitted by the real game-mode end bridge after its
// retention hook has run. The disposable proof runner correlates this artifact
// with the shutdown stage result without depending on logs or live references.
[BaseContainerProps()]
class HST_OrdinaryCampaignEndBridgeReceipt
{
	string m_sMagic;
	int m_iVersion;
	string m_sSessionNonce;
	string m_sStageNonce;
	string m_sRunId;
	string m_sPayloadNonce;
	string m_sStage;
	int m_iStageOrdinal;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	int m_iCampaignSchemaVersion;
	int m_iSettingsSchemaVersion;
	string m_sWorld;
	bool m_bSuccess;

	bool m_bEndGameModeIntercepted;
	bool m_bStableCheckpointObserved;
	bool m_bRetentionHandlerExecuted;
	bool m_bKeepSessionSaveCLIAbsent;
	bool m_bPersistenceKeepSessionDataDisabled;
	string m_sExpectedShutdownSavePointId;
	string m_sObservedShutdownSavePointId;
	bool m_bShutdownSavePointExact;
	string m_sEvidence;
}
