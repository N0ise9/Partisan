class HST_PersistenceService
{
	static const string PROFILE_SAVE_DIRECTORY = "$profile:h-istasi";
	static const string PROFILE_SAVE_FILE = "$profile:h-istasi/HST_CampaignSaveData.json";

	protected float m_fAutosaveElapsed;
	protected float m_fMajorChangeElapsed;
	protected bool m_bMajorChangePending;
	protected ref HST_CampaignSaveData m_LastCapturedSave;
	protected ref HST_CampaignSaveData m_TrackedCampaignSave;
	protected ref HST_CampaignSaveData m_RestoredCampaignSave;
	protected bool m_bProfileFallbackSaved;
	protected bool m_bProfileFallbackLoaded;
	protected string m_sProfileFallbackStatus = "profile fallback idle";

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
			CaptureAndTrackState(state, "captured before checkpoint");

		bool scriptedStateSaved = FlushTrackedCampaignState(ESaveGameType.MANUAL);
		bool profileFallbackSaved;
		if (!scriptedStateSaved)
			profileFallbackSaved = SaveProfileFallback(m_TrackedCampaignSave);
		SaveGameManager saveManager = SaveGameManager.Get();
		bool savePointRequested;
		if (CanRequestSavePoint(saveManager))
		{
			saveManager.RequestSavePoint(ESaveGameType.MANUAL, displayName);
			savePointRequested = true;
		}

		if (!scriptedStateSaved && !profileFallbackSaved && !savePointRequested)
		{
			if (state)
				state.m_sLastPersistenceStatus = "checkpoint failed: scripted save false | profile fallback false | savepoint false";
			return false;
		}

		if (state)
		{
			state.m_iLastSaveSecond = state.m_iElapsedSeconds;
			state.m_sLastPersistenceStatus = string.Format("checkpoint requested: %1 | scripted save %2 | profile fallback %3 | savepoint %4", displayName, scriptedStateSaved, profileFallbackSaved, savePointRequested);
		}

		return true;
	}

	void CaptureState(HST_CampaignState state)
	{
		CaptureAndTrackState(state);
	}

	HST_CampaignSaveData CaptureAndTrackState(HST_CampaignState state, string persistenceStatus = "captured and tracked")
	{
		if (!state)
			return null;

		if (!m_LastCapturedSave)
			m_LastCapturedSave = new HST_CampaignSaveData();

		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastSaveSecond = state.m_iElapsedSeconds;
		if (!persistenceStatus.IsEmpty())
			state.m_sLastPersistenceStatus = persistenceStatus;
		m_LastCapturedSave.Capture(state);
		m_TrackedCampaignSave = m_LastCapturedSave;
		TrackCampaignSaveData(m_TrackedCampaignSave);
		return m_LastCapturedSave;
	}

	HST_CampaignState RestoreOrCreateCampaignState(HST_CampaignState fallbackState)
	{
		if (!fallbackState)
			return null;

		HST_CampaignSaveData restoredSave = GetRestoredCampaignSaveData();
		if (!restoredSave)
			restoredSave = LoadProfileFallback();
		if (restoredSave)
		{
			ApplyRestoredCampaignState(fallbackState, restoredSave);
			return fallbackState;
		}

		fallbackState.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		fallbackState.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		fallbackState.m_bRestoredFromPersistence = false;
		fallbackState.m_sLastPersistenceStatus = "new campaign state tracked";
		CaptureAndTrackState(fallbackState, fallbackState.m_sLastPersistenceStatus);
		return fallbackState;
	}

	bool ApplyRestoredCampaignState(HST_CampaignState targetState, HST_CampaignSaveData restoredSave)
	{
		if (!targetState || !restoredSave)
			return false;

		restoredSave.MigrateToCurrentSchema();
		restoredSave.ApplyTo(targetState);
		targetState.m_bRestoredFromPersistence = true;
		targetState.m_iLastRestoreSecond = targetState.m_iElapsedSeconds;
		targetState.m_sLastPersistenceStatus = string.Format("restored schema %1 -> %2", targetState.m_iLastLoadedSchemaVersion, targetState.m_iSchemaVersion);
		m_RestoredCampaignSave = restoredSave;
		CaptureAndTrackState(targetState, targetState.m_sLastPersistenceStatus);
		return true;
	}

	HST_CampaignSaveData GetLastCapturedSave()
	{
		return m_LastCapturedSave;
	}

	string BuildPersistenceReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi persistence | state not ready";

		string saveManagerStatus = "save manager unavailable";
		SaveGameManager saveManager = SaveGameManager.Get();
		if (saveManager)
			saveManagerStatus = string.Format("saving enabled %1 | saving allowed %2", saveManager.IsSavingEnabled(), saveManager.IsSavingAllowed());
		string persistenceSystemStatus = BuildPersistenceSystemStatus();
		string profileFallbackStatus = BuildProfileFallbackStatus();

		string tracked = "not tracked";
		if (m_TrackedCampaignSave)
			tracked = string.Format("tracked schema %1 | missions %2 | groups %3", m_TrackedCampaignSave.m_iSchemaVersion, m_TrackedCampaignSave.m_aActiveMissions.Count(), m_TrackedCampaignSave.m_aActiveGroups.Count());

		string restored = "no restored data";
		if (m_RestoredCampaignSave)
			restored = string.Format("restored schema %1 | migrated to %2", m_RestoredCampaignSave.m_iLastLoadedSchemaVersion, m_RestoredCampaignSave.m_iSchemaVersion);

		return string.Format("h-istasi persistence | %1 | last save %2 | last restore %3 | %4\n%5\n%6\n%7\n%8", state.m_sLastPersistenceStatus, state.m_iLastSaveSecond, state.m_iLastRestoreSecond, saveManagerStatus, persistenceSystemStatus, profileFallbackStatus, tracked, restored);
	}

	protected HST_CampaignSaveData GetRestoredCampaignSaveData()
	{
		if (m_RestoredCampaignSave)
			return m_RestoredCampaignSave;

		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (!persistence || !persistence.WasDataLoaded())
			return null;

		Managed persistentState = persistence.GetPersistentState(HST_CampaignSaveData);
		m_RestoredCampaignSave = HST_CampaignSaveData.Cast(persistentState);
		return m_RestoredCampaignSave;
	}

	protected bool CanRequestSavePoint(SaveGameManager saveManager)
	{
		return saveManager && saveManager.IsSavingEnabled() && saveManager.IsSavingAllowed();
	}

	protected bool TrackCampaignSaveData(HST_CampaignSaveData saveData)
	{
		if (!saveData)
			return false;

		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (!persistence)
			return false;

		if (persistence.IsTracked(saveData))
			return true;

		return persistence.StartTracking(saveData);
	}

	protected bool FlushTrackedCampaignState(ESaveGameType saveType)
	{
		if (!m_TrackedCampaignSave)
			return false;

		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (!persistence)
			return false;

		TrackCampaignSaveData(m_TrackedCampaignSave);
		return persistence.Save(m_TrackedCampaignSave, saveType);
	}

	protected bool SaveProfileFallback(HST_CampaignSaveData saveData)
	{
		if (!saveData)
		{
			m_sProfileFallbackStatus = "profile fallback save skipped: no tracked save";
			return false;
		}

		FileIO.MakeDirectory(PROFILE_SAVE_DIRECTORY);
		JsonSaveContext context = new JsonSaveContext();
		if (context.WriteValue("", saveData) && context.SaveToFile(PROFILE_SAVE_FILE))
		{
			m_bProfileFallbackSaved = true;
			m_sProfileFallbackStatus = string.Format("profile fallback saved schema %1 to %2", saveData.m_iSchemaVersion, PROFILE_SAVE_FILE);
			Print("h-istasi persistence | " + m_sProfileFallbackStatus);
			return true;
		}

		m_sProfileFallbackStatus = string.Format("profile fallback save failed at %1", PROFILE_SAVE_FILE);
		Print("h-istasi persistence | " + m_sProfileFallbackStatus, LogLevel.WARNING);
		return false;
	}

	protected HST_CampaignSaveData LoadProfileFallback()
	{
		if (!FileIO.FileExists(PROFILE_SAVE_FILE))
		{
			m_sProfileFallbackStatus = string.Format("profile fallback missing at %1", PROFILE_SAVE_FILE);
			return null;
		}

		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(PROFILE_SAVE_FILE))
		{
			m_sProfileFallbackStatus = string.Format("profile fallback load failed at %1", PROFILE_SAVE_FILE);
			Print("h-istasi persistence | " + m_sProfileFallbackStatus, LogLevel.WARNING);
			return null;
		}

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		if (!context.ReadValue("", saveData))
		{
			m_sProfileFallbackStatus = string.Format("profile fallback read failed at %1", PROFILE_SAVE_FILE);
			Print("h-istasi persistence | " + m_sProfileFallbackStatus, LogLevel.WARNING);
			return null;
		}

		m_bProfileFallbackLoaded = true;
		m_sProfileFallbackStatus = string.Format("profile fallback loaded schema %1 from %2", saveData.m_iSchemaVersion, PROFILE_SAVE_FILE);
		Print("h-istasi persistence | " + m_sProfileFallbackStatus);
		return saveData;
	}

	protected string BuildPersistenceSystemStatus()
	{
		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (!persistence)
			return "PersistenceSystem unavailable";

		bool tracked;
		if (m_TrackedCampaignSave)
			tracked = persistence.IsTracked(m_TrackedCampaignSave);

		return string.Format("PersistenceSystem | loaded %1 | state %2 | tracked %3", persistence.WasDataLoaded(), persistence.GetState(), tracked);
	}

	protected string BuildProfileFallbackStatus()
	{
		bool exists = FileIO.FileExists(PROFILE_SAVE_FILE);
		return string.Format("profile fallback | exists %1 | saved %2 | loaded %3 | %4", exists, m_bProfileFallbackSaved, m_bProfileFallbackLoaded, m_sProfileFallbackStatus);
	}
}
