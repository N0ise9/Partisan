class HST_PersistenceService
{
	protected float m_fAutosaveElapsed;
	protected float m_fMajorChangeElapsed;
	protected bool m_bMajorChangePending;

	void MarkMajorChange()
	{
		m_bMajorChangePending = true;
		m_fMajorChangeElapsed = 0;
	}

	void Tick(float timeSlice, int autosaveIntervalSeconds, int majorChangeDebounceSeconds)
	{
		m_fAutosaveElapsed += timeSlice;

		if (m_bMajorChangePending)
		{
			m_fMajorChangeElapsed += timeSlice;
			if (m_fMajorChangeElapsed >= majorChangeDebounceSeconds)
			{
				RequestCheckpoint("h-istasi major change");
				m_bMajorChangePending = false;
			}
		}

		if (m_fAutosaveElapsed < autosaveIntervalSeconds)
			return;

		RequestCheckpoint("h-istasi autosave");
		m_fAutosaveElapsed = 0;
	}

	bool RequestCheckpoint(string displayName)
	{
		SaveGameManager saveManager = SaveGameManager.Get();
		if (!saveManager || !saveManager.IsSavingEnabled() || !saveManager.IsSavingAllowed())
			return false;

		saveManager.RequestSavePoint(ESaveGameType.MANUAL, displayName);
		return true;
	}
}

