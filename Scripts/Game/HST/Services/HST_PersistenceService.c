class HST_PersistenceService
{
	protected float m_fAutosaveElapsed;
	protected float m_fMajorChangeElapsed;
	protected bool m_bMajorChangePending;
	protected ref HST_CampaignSaveData m_LastCapturedSave;

	void MarkMajorChange()
	{
		m_bMajorChangePending = true;
		m_fMajorChangeElapsed = 0;
	}

	void Tick(HST_CampaignState state, float timeSlice, int autosaveIntervalSeconds, int majorChangeDebounceSeconds)
	{
		m_fAutosaveElapsed += timeSlice;

		if (m_bMajorChangePending)
		{
			m_fMajorChangeElapsed += timeSlice;
			if (m_fMajorChangeElapsed >= majorChangeDebounceSeconds)
			{
				RequestCheckpoint("h-istasi major change", state);
				m_bMajorChangePending = false;
			}
		}

		if (m_fAutosaveElapsed < autosaveIntervalSeconds)
			return;

		RequestCheckpoint("h-istasi autosave", state);
		m_fAutosaveElapsed = 0;
	}

	bool RequestCheckpoint(string displayName, HST_CampaignState state = null)
	{
		if (state)
			CaptureState(state);

		SaveGameManager saveManager = SaveGameManager.Get();
		if (!saveManager || !saveManager.IsSavingEnabled() || !saveManager.IsSavingAllowed())
			return false;

		saveManager.RequestSavePoint(ESaveGameType.MANUAL, displayName);
		return true;
	}

	void CaptureState(HST_CampaignState state)
	{
		if (!state)
			return;

		if (!m_LastCapturedSave)
			m_LastCapturedSave = new HST_CampaignSaveData();

		m_LastCapturedSave.Capture(state);
	}

	HST_CampaignSaveData GetLastCapturedSave()
	{
		return m_LastCapturedSave;
	}
}
