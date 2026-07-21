#ifdef ENABLE_DIAG
// Disposable, profile-local administrative campaign-reset persistence proof
// artifacts. These DTOs contain process-portable evidence only; no engine
// objects, service references, or machine-local paths cross a process boundary.
[BaseContainerProps()]
class HST_AdminCampaignResetPersistenceProofOwner
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
}

[BaseContainerProps()]
class HST_AdminCampaignResetPersistenceProofGuard
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
	string m_sExpectedLoadSavePointId;
	int m_iExpectedJournalGeneration;
	bool m_bAllowCanonicalCampaignOverwrite;
	bool m_bNoSaveStage;
}

[BaseContainerProps()]
class HST_AdminCampaignResetPersistenceProofCarrier
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
	int m_iCompletedStageOrdinal = -1;

	string m_sOldSentinelTaskId;
	string m_sOldSentinelFingerprint;
	string m_sPreservedPlayerIdentityId;
	string m_sPreservedPlayerFingerprint;
	string m_sPreservedCommanderIdentityId;
	int m_iOldMarkerProjectionEpoch;
	int m_iResetMarkerProjectionEpoch;

	string m_sOldSavePointId;
	string m_sOldSnapshotFingerprint;
	int m_iOldCheckpointSequence;
	int m_iOldRestoreSequence;
	int m_iOldJournalGeneration;
	string m_sOldJournalSlot;
	int m_iOldJournalValidSlotCount;
	bool m_bOldJournalChainExact;

	string m_sBlockerSavePointId;
	string m_sBlockerSnapshotFingerprint;
	int m_iBlockerCheckpointSequence;
	int m_iBlockerRestoreSequence;
	int m_iBlockerJournalGeneration;
	string m_sBlockerJournalSlot;
	int m_iBlockerJournalValidSlotCount;
	bool m_bBlockerJournalChainExact;

	string m_sResetSavePointId;
	string m_sResetSnapshotFingerprint;
	int m_iResetCheckpointSequence;
	int m_iResetRestoreSequence;
	int m_iResetJournalGeneration;
	string m_sResetJournalSlot;
	int m_iResetJournalValidSlotCount;
	bool m_bResetJournalChainExact;

	bool m_bInFlightCheckpointObserved;
	bool m_bInFlightResetRejected;
	bool m_bRejectedResetReturnedNoCheckpoint;
	string m_sRejectedResetBeforeFingerprint;
	string m_sRejectedResetAfterFingerprint;
	bool m_bRejectedResetStateExact;
	bool m_bRejectedResetSentinelExact;
	bool m_bRejectedResetIdentityExact;
	bool m_bRejectedResetEpochExact;
	bool m_bBlockerCompletionReceived;
	bool m_bBlockerNativeCommitSucceeded;
	bool m_bBlockerProfileMirrorSaved;
	bool m_bBlockerOnAfterSaveSucceeded;
	bool m_bBlockerOnSaveCreatedObserved;
	bool m_bResetRemovedSentinel;
	bool m_bResetPreservedIdentity;
	bool m_bResetAdvancedEpoch;
	bool m_bResetRetainedSequenceFloors;
}

[BaseContainerProps()]
class HST_AdminCampaignResetPersistenceProofResult
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
	bool m_bSourceExact;
	bool m_bPersistenceSystemAvailable;
	bool m_bPersistenceSystemLoadedData;
	bool m_bNativeRecordPresent;
	bool m_bNativeRecordValid;
	bool m_bProfileFallbackPresent;
	bool m_bProfileFallbackRead;
	bool m_bDegradedNativeRecovery;
	string m_sDegradedNativeRecoveryReason;
	string m_sNativeSnapshotFingerprint;
	string m_sProfileFallbackSnapshotFingerprint;
	string m_sSelectedSnapshotFingerprint;
	int m_iSourceJournalGeneration = -1;
	string m_sSourceJournalSlot;
	int m_iSourceJournalValidSlotCount;
	bool m_bSourceJournalLegacyRaw;
	bool m_bSourceJournalChainExact;

	int m_iCommittedJournalGeneration = -1;
	string m_sCommittedJournalSlot;
	int m_iCommittedJournalValidSlotCount;
	bool m_bCommittedJournalLegacyRaw;
	bool m_bCommittedJournalChainExact;
	string m_sCommittedJournalFingerprint;

	int m_iLiveOldSentinelCount;
	string m_sLiveOldSentinelFingerprint;
	int m_iStaleJournalOldSentinelCount;
	string m_sStaleJournalOldSentinelFingerprint;
	bool m_bOldSentinelRejected;
	string m_sExpectedPlayerIdentityId;
	string m_sObservedPlayerFingerprint;
	bool m_bPlayerIdentityExact;
	string m_sExpectedCommanderIdentityId;
	string m_sObservedCommanderIdentityId;
	bool m_bCommanderIdentityExact;
	int m_iExpectedMarkerProjectionEpoch;
	int m_iObservedMarkerProjectionEpoch;
	bool m_bMarkerProjectionEpochExact;
	int m_iExpectedCheckpointSequence;
	int m_iObservedCheckpointSequence;
	int m_iExpectedRestoreSequence;
	int m_iObservedRestoreSequence;
	bool m_bDurableOrderExact;

	string m_sExpectedPriorSavePointId;
	string m_sObservedPriorSavePointId;
	string m_sCreatedSavePointId;
	string m_sActiveSavePointId;
	string m_sExpectedSaveType;
	string m_sCreatedSaveType;
	string m_sExpectedSaveName;
	string m_sCreatedSaveName;
	bool m_bCampaignCaptured;
	bool m_bTransientStateStaged;
	bool m_bSavePointRequested;
	bool m_bCompletionReceived;
	bool m_bNativeCommitSucceeded;
	bool m_bProfileMirrorSaved;
	bool m_bCompletionObserverSucceeded;
	bool m_bOnAfterSaveObserved;
	bool m_bOnAfterSaveSucceeded;
	bool m_bOnSaveCreatedObserved;
	bool m_bActiveSaveExact;

	bool m_bInFlightCheckpointObserved;
	bool m_bInFlightResetRejected;
	bool m_bRejectedResetReturnedNoCheckpoint;
	string m_sRejectedResetBeforeFingerprint;
	string m_sRejectedResetAfterFingerprint;
	bool m_bRejectedResetStateExact;
	bool m_bRejectedResetSentinelExact;
	bool m_bRejectedResetIdentityExact;
	bool m_bRejectedResetEpochExact;

	bool m_bNoSaveStage;
	bool m_bSavingDisabledBeforeClose;
	bool m_bNoCheckpointRequested;
	bool m_bNoSaveEventsObserved;
	bool m_bActiveSaveUnchanged;
	string m_sEvidence;
}
#endif
