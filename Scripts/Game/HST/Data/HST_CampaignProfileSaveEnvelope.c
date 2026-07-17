[BaseContainerProps()]
class HST_CampaignProfileSaveEnvelope
{
	string m_sMagic;
	int m_iEnvelopeVersion;
	int m_iGeneration;
	int m_iSnapshotSchemaVersion;
	string m_sSnapshotFingerprint;
	int m_iPreviousGeneration;
	string m_sPreviousSnapshotFingerprint;
	string m_sSnapshotPayload;
}

// Runtime-only observation of one alternating profile-save slot. The stored
// envelope keeps only process-portable values; parsed DTO and path references
// remain transient and are never written into campaign data.
class HST_CampaignProfileSaveCandidate
{
	string m_sSlotLabel;
	string m_sPath;
	bool m_bArtifactPresent;
	bool m_bEnvelopeRecognized;
	bool m_bLegacyRaw;
	bool m_bValid;
	bool m_bUnsupportedFuture;
	int m_iGeneration;
	int m_iSnapshotSchemaVersion;
	string m_sSnapshotFingerprint;
	int m_iPreviousGeneration;
	string m_sPreviousSnapshotFingerprint;
	string m_sSnapshotPayload;
	ref HST_CampaignSaveData m_SaveData;
	string m_sEvidence;
}

// Complete read-only resolution of the two profile slots. Higher-level
// persistence authority reconciles a valid native snapshot and the selected
// journal generation by durable snapshot order; neither wins by storage type.
class HST_CampaignProfileSaveResolution
{
	ref HST_CampaignProfileSaveCandidate m_Canonical;
	ref HST_CampaignProfileSaveCandidate m_Recovery;
	ref HST_CampaignProfileSaveCandidate m_Selected;
	bool m_bArtifactsPresent;
	bool m_bHasSelection;
	bool m_bUnsupportedFuture;
	bool m_bAmbiguous;
	bool m_bChainExact;
	int m_iValidCandidateCount;
	string m_sEvidence;
}

// Verified outcome of one journal advance. Success means the inactive slot was
// written, read back, and then selected as the newest valid generation without
// modifying the previously selected generation.
class HST_CampaignProfileSaveWriteReceipt
{
	bool m_bSuccess;
	string m_sWrittenSlotLabel;
	int m_iGeneration;
	int m_iSnapshotSchemaVersion;
	string m_sSnapshotFingerprint;
	int m_iPreviousGeneration;
	string m_sPreviousSnapshotFingerprint;
	ref HST_CampaignProfileSaveResolution m_Before;
	ref HST_CampaignProfileSaveResolution m_After;
	string m_sEvidence;
}
