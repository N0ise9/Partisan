enum HST_ECampaignPersistenceSource
{
	HST_PERSISTENCE_SOURCE_PENDING = 0,
	HST_PERSISTENCE_SOURCE_NATIVE = 1,
	HST_PERSISTENCE_SOURCE_PROFILE_FALLBACK = 2,
	HST_PERSISTENCE_SOURCE_NEW_CAMPAIGN = 3,
	HST_PERSISTENCE_SOURCE_FATAL = 4
}

class HST_PersistenceSourceResolution
{
	HST_ECampaignPersistenceSource m_eSource
		= HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_PENDING;
	ref HST_CampaignState m_State;
	bool m_bPersistenceSystemAvailable;
	bool m_bPersistenceSystemLoadedData;
	bool m_bNativeRecordPresent;
	bool m_bNativeRecordValid;
	bool m_bNativeRecordUnsupportedFuture;
	bool m_bProfileFallbackPresent;
	bool m_bProfileFallbackRead;
	bool m_bDegradedNativeRecovery;
	string m_sDegradedNativeRecoveryReason;
	int m_iProfileJournalGeneration = -1;
	string m_sProfileJournalSlot;
	int m_iProfileJournalValidSlotCount;
	bool m_bProfileJournalLegacyRaw;
	bool m_bProfileJournalChainExact;
	string m_sNativeSnapshotFingerprint;
	string m_sProfileFallbackSnapshotFingerprint;
	string m_sSelectedSnapshotFingerprint;
	string m_sEvidence;

	bool IsPending()
	{
		return m_eSource
			== HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_PENDING;
	}

	bool IsFatal()
	{
		return m_eSource
			== HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_FATAL;
	}

	bool HasSelectedState()
	{
		return m_State
			&& (m_eSource
				== HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_NATIVE
				|| m_eSource
					== HST_ECampaignPersistenceSource
						.HST_PERSISTENCE_SOURCE_PROFILE_FALLBACK
				|| m_eSource
					== HST_ECampaignPersistenceSource
						.HST_PERSISTENCE_SOURCE_NEW_CAMPAIGN);
	}

	string BuildSourceLabel()
	{
		if (m_eSource
			== HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_NATIVE)
			return "native";
		if (m_eSource
			== HST_ECampaignPersistenceSource
				.HST_PERSISTENCE_SOURCE_PROFILE_FALLBACK)
			return "profile_fallback";
		if (m_eSource
			== HST_ECampaignPersistenceSource
				.HST_PERSISTENCE_SOURCE_NEW_CAMPAIGN)
			return "new_campaign";
		if (m_eSource
			== HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_FATAL)
			return "fatal";
		return "pending";
	}
}
