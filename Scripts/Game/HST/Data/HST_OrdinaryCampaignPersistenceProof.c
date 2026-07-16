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

	string m_sExpectedProfileFallbackFingerprint;
	string m_sProfileFallbackReadBackFingerprint;
	bool m_bProfileFallbackReadBackExact;
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
