class HST_ExactEnemyResponsePersistencePositionPlan
{
	ref HST_OperationRecordState m_Operation;
	ref HST_EnemyOrderState m_Order;
	ref HST_ActiveGroupState m_Group;
	vector m_vLivePosition;
	vector m_vPreviousGroupPosition;
	vector m_vPreviousOperationPosition;
	int m_iPreviousOperationRevision;
	int m_iPreviousOperationLastProgressAtSecond;
}

// Observable request state for one production campaign checkpoint. The
// Transient PersistenceSystem staging happens before this object is returned.
// With native authority, the profile mirror is advanced only after native
// commit succeeds so an interrupted request cannot leave a newer fallback
// hidden behind an older-but-valid native row.
class HST_PersistenceCheckpointRequest
{
	ESaveGameType m_eSaveType;
	ESaveGameRequestFlags m_eRequestFlags;
	string m_sDisplayName;
	bool m_bCampaignCaptured;
	bool m_bIsolatedCaptureAccepted;
	bool m_bTransientStateStaged;
	bool m_bProfileFallbackSaved;
	bool m_bSavePointRequested;
	bool m_bCompletionReceived;
	bool m_bNativeCommitSucceeded;
	bool m_bIssuedByScheduler;
	bool m_bSchedulerThresholdCrossed;
	bool m_bSchedulerMajorChangePendingAtAttempt;
	string m_sSchedulerOrigin;
	int m_iSchedulerAttemptSequence;
	int m_iSchedulerTickCountAtAttempt;
	int m_iSchedulerAutosaveIntervalSeconds;
	int m_iSchedulerMajorChangeDebounceSeconds;
	float m_fSchedulerCumulativeSecondsAtAttempt;
	float m_fSchedulerAutosaveElapsedBeforeSeconds;
	float m_fSchedulerAutosaveElapsedAtAttemptSeconds;
	float m_fSchedulerMajorChangeElapsedBeforeSeconds;
	float m_fSchedulerMajorChangeElapsedAtAttemptSeconds;
	string m_sEvidence;

	bool WasAccepted()
	{
		return m_bIsolatedCaptureAccepted
			|| m_bProfileFallbackSaved || m_bSavePointRequested;
	}
}

class HST_PersistenceCheckpointCallbackContext
{
	int m_iRequestSequence;
}

class HST_PersistenceService
{
	static const float CHECKPOINT_COMMIT_TIMEOUT_SECONDS = 120.0;
	static const string SCHEDULER_ORIGIN_MAJOR_CHANGE
		= "major_change_debounce";
	static const string SCHEDULER_ORIGIN_PERIODIC_AUTOSAVE
		= "periodic_autosave";
	protected float m_fAutosaveElapsed;
	protected float m_fMajorChangeElapsed;
	protected bool m_bMajorChangePending;
	protected int m_iSchedulerAttemptSequence;
	protected int m_iSchedulerTickCount;
	protected float m_fSchedulerCumulativeSeconds;
	protected bool m_bCampaignDebugIsolationActive;
	protected ref HST_CampaignSaveData m_LastCapturedSave;
	protected ref HST_CampaignSaveData m_TrackedCampaignSave;
	protected ref HST_CampaignSaveData m_RestoredCampaignSave;
	protected ref HST_CampaignSaveData m_IsolatedCapturedSave;
	protected HST_CampaignPersistentState m_NativeCampaignState;
	protected ref HST_PersistenceSourceResolution m_LastSourceResolution;
	protected bool m_bProfileFallbackSaved;
	protected bool m_bProfileFallbackLoaded;
	protected string m_sProfileFallbackStatus = "profile fallback idle";
	protected bool m_bCheckpointSavePointInFlight;
	protected ref HST_PersistenceCheckpointRequest m_PendingCheckpointRequest;
	protected ref HST_CampaignSaveData m_PendingCheckpointSaveData;
	protected ref SaveGameOperationCallback m_PendingCheckpointObserver;
	protected ref SaveGameOperationCallback m_CheckpointCompletionCallback;
	protected ref HST_PersistenceCheckpointCallbackContext m_CheckpointCallbackContext;
	protected HST_CampaignState m_PendingCheckpointState;
	protected int m_iCheckpointRequestSequence;
	protected float m_fCheckpointCommitElapsedSeconds;
	protected HST_PhysicalWarService m_PhysicalWar;
	protected HST_ForceSpawnQueueService m_ForceSpawnQueue;
	protected HST_ForceSpawnAdapterService m_ForceSpawnAdapter;
	protected HST_EnemyQRFOperationService m_EnemyQRFOperations;
	protected HST_EnemyCounterattackOperationService m_EnemyCounterattackOperations;
	protected HST_EnemyGarrisonRebuildOperationService m_EnemyGarrisonRebuildOperations;
	protected HST_LocalSecurityOperationService m_LocalSecurityPatrolOperations;
	protected HST_MissionGuardOperationService m_MissionGuardOperations;
	protected HST_RescuePOWOperationService m_RescuePOWOperations;
	protected HST_CivilianService m_Civilians;

	void SetPhysicalWarService(HST_PhysicalWarService physicalWar)
	{
		m_PhysicalWar = physicalWar;
	}

	void SetExactInfantryAuthorityServices(
		HST_ForceSpawnQueueService forceSpawnQueue,
		HST_ForceSpawnAdapterService forceSpawnAdapter)
	{
		m_ForceSpawnQueue = forceSpawnQueue;
		m_ForceSpawnAdapter = forceSpawnAdapter;
	}

	void SetExactEnemyResponseAuthorityServices(
		HST_EnemyQRFOperationService enemyQRFOperations,
		HST_EnemyCounterattackOperationService enemyCounterattackOperations,
		HST_EnemyGarrisonRebuildOperationService enemyGarrisonRebuildOperations)
	{
		m_EnemyQRFOperations = enemyQRFOperations;
		m_EnemyCounterattackOperations = enemyCounterattackOperations;
		m_EnemyGarrisonRebuildOperations = enemyGarrisonRebuildOperations;
	}

	void SetMissionGuardOperationService(HST_MissionGuardOperationService missionGuardOperations)
	{
		m_MissionGuardOperations = missionGuardOperations;
	}

	void SetLocalSecurityOperationService(
		HST_LocalSecurityOperationService localSecurityPatrolOperations)
	{
		m_LocalSecurityPatrolOperations = localSecurityPatrolOperations;
	}

	void SetRescuePOWOperationService(HST_RescuePOWOperationService rescuePOWOperations)
	{
		m_RescuePOWOperations = rescuePOWOperations;
	}

	void SetCivilianService(HST_CivilianService civilians)
	{
		m_Civilians = civilians;
	}

	void MarkMajorChange()
	{
		EnsureMajorChangePending();
	}

	// Major changes are coalesced behind one bounded checkpoint deadline. A
	// trailing-edge debounce can be starved forever by periodic gameplay or
	// retry heartbeats; once pending, later marks must not push the deadline
	// back. After a successful checkpoint clears the flag, the next change
	// starts a fresh interval.
	void EnsureMajorChangePending()
	{
		if (m_bCampaignDebugIsolationActive)
			return;
		if (m_bMajorChangePending)
			return;
		m_bMajorChangePending = true;
		m_fMajorChangeElapsed = 0;
	}

	bool IsCampaignDebugIsolationActive()
	{
		return m_bCampaignDebugIsolationActive;
	}

	HST_PersistenceCheckpointRequest Tick(
		HST_CampaignState state,
		float timeSlice,
		int autosaveIntervalSeconds,
		int majorChangeDebounceSeconds,
		SaveGameOperationCallback completionObserver = null)
	{
		float deltaSeconds = Math.Max(0.0, timeSlice);
		TickCheckpointCommitTimeout(deltaSeconds);
		if (m_bCampaignDebugIsolationActive)
			return null;

		int autosaveInterval = Math.Max(1, autosaveIntervalSeconds);
		int majorChangeDebounce = Math.Max(1, majorChangeDebounceSeconds);
		float autosaveElapsedBefore = m_fAutosaveElapsed;
		float majorChangeElapsedBefore = m_fMajorChangeElapsed;
		m_iSchedulerTickCount++;
		m_fSchedulerCumulativeSeconds += deltaSeconds;
		m_fAutosaveElapsed += deltaSeconds;
		if (m_bMajorChangePending)
			m_fMajorChangeElapsed += deltaSeconds;

		// The clocks keep advancing while a checkpoint is in flight, but the
		// scheduler never emits a competing request. Once that request completes,
		// an already-due generation is handled on the next frame.
		if (m_bCheckpointSavePointInFlight)
			return null;

		if (m_bMajorChangePending)
		{
			if (m_fMajorChangeElapsed >= majorChangeDebounce)
			{
				float autosaveElapsedAtAttempt = m_fAutosaveElapsed;
				float majorChangeElapsedAtAttempt = m_fMajorChangeElapsed;
				HST_PersistenceCheckpointRequest majorCheckpoint
					= RequestTypedCheckpointDetailed(
					"Partisan major change",
					ESaveGameType.SCRIPTED,
					0,
					state,
					completionObserver);
				StampScheduledCheckpointRequest(
					majorCheckpoint,
					SCHEDULER_ORIGIN_MAJOR_CHANGE,
					autosaveInterval,
					majorChangeDebounce,
					autosaveElapsedBefore,
					autosaveElapsedAtAttempt,
					majorChangeElapsedBefore,
					majorChangeElapsedAtAttempt,
					true);
				if (!majorCheckpoint || !majorCheckpoint.WasAccepted())
					m_fMajorChangeElapsed = 0;
				// A rejected dirty checkpoint must not rewind the independent
				// periodic clock. If both lanes were due, AUTO receives the next
				// frame instead of being starved by permanent retry lockstep.
				return majorCheckpoint;
			}
		}

		if (m_fAutosaveElapsed < autosaveInterval)
			return null;

		float autosaveElapsedAtAttempt = m_fAutosaveElapsed;
		float majorChangeElapsedAtAttempt = m_fMajorChangeElapsed;
		bool majorChangePendingAtAttempt = m_bMajorChangePending;
		HST_PersistenceCheckpointRequest autosaveCheckpoint
			= RequestTypedCheckpointDetailed(
			"Partisan autosave",
			ESaveGameType.AUTO,
			0,
			state,
			completionObserver);
		StampScheduledCheckpointRequest(
			autosaveCheckpoint,
			SCHEDULER_ORIGIN_PERIODIC_AUTOSAVE,
			autosaveInterval,
			majorChangeDebounce,
			autosaveElapsedBefore,
			autosaveElapsedAtAttempt,
			majorChangeElapsedBefore,
			majorChangeElapsedAtAttempt,
			majorChangePendingAtAttempt);
		if (!autosaveCheckpoint || !autosaveCheckpoint.WasAccepted())
		{
			int retrySeconds = Math.Max(1, majorChangeDebounce);
			m_fAutosaveElapsed = Math.Max(0, autosaveInterval - retrySeconds);
		}
		return autosaveCheckpoint;
	}

	protected void StampScheduledCheckpointRequest(
		HST_PersistenceCheckpointRequest request,
		string schedulerOrigin,
		int autosaveIntervalSeconds,
		int majorChangeDebounceSeconds,
		float autosaveElapsedBeforeSeconds,
		float autosaveElapsedAtAttemptSeconds,
		float majorChangeElapsedBeforeSeconds,
		float majorChangeElapsedAtAttemptSeconds,
		bool majorChangePendingAtAttempt)
	{
		if (!request)
			return;
		m_iSchedulerAttemptSequence++;
		request.m_bIssuedByScheduler = true;
		request.m_sSchedulerOrigin = schedulerOrigin;
		request.m_iSchedulerAttemptSequence = m_iSchedulerAttemptSequence;
		request.m_iSchedulerTickCountAtAttempt = m_iSchedulerTickCount;
		request.m_iSchedulerAutosaveIntervalSeconds = autosaveIntervalSeconds;
		request.m_iSchedulerMajorChangeDebounceSeconds
			= majorChangeDebounceSeconds;
		request.m_fSchedulerCumulativeSecondsAtAttempt
			= m_fSchedulerCumulativeSeconds;
		request.m_fSchedulerAutosaveElapsedBeforeSeconds
			= autosaveElapsedBeforeSeconds;
		request.m_fSchedulerAutosaveElapsedAtAttemptSeconds
			= autosaveElapsedAtAttemptSeconds;
		request.m_fSchedulerMajorChangeElapsedBeforeSeconds
			= majorChangeElapsedBeforeSeconds;
		request.m_fSchedulerMajorChangeElapsedAtAttemptSeconds
			= majorChangeElapsedAtAttemptSeconds;
		request.m_bSchedulerMajorChangePendingAtAttempt
			= majorChangePendingAtAttempt;
		if (schedulerOrigin == SCHEDULER_ORIGIN_PERIODIC_AUTOSAVE)
		{
			request.m_bSchedulerThresholdCrossed
				= request.m_fSchedulerAutosaveElapsedBeforeSeconds
					< autosaveIntervalSeconds
				&& request.m_fSchedulerAutosaveElapsedAtAttemptSeconds
					>= autosaveIntervalSeconds;
		}
		else
		{
			request.m_bSchedulerThresholdCrossed
				= request.m_fSchedulerMajorChangeElapsedBeforeSeconds
					< majorChangeDebounceSeconds
				&& request.m_fSchedulerMajorChangeElapsedAtAttemptSeconds
					>= majorChangeDebounceSeconds;
		}
		request.m_sEvidence += string.Format(
			" | scheduler %1 attempt/tick %2/%3 | auto before/after/interval %4/%5/%6",
			schedulerOrigin,
			request.m_iSchedulerAttemptSequence,
			request.m_iSchedulerTickCountAtAttempt,
			request.m_fSchedulerAutosaveElapsedBeforeSeconds,
			request.m_fSchedulerAutosaveElapsedAtAttemptSeconds,
			autosaveIntervalSeconds);
		request.m_sEvidence += string.Format(
			" | major pending before/after/debounce %1/%2/%3/%4",
			majorChangePendingAtAttempt,
			request.m_fSchedulerMajorChangeElapsedBeforeSeconds,
			request.m_fSchedulerMajorChangeElapsedAtAttemptSeconds,
			majorChangeDebounceSeconds);
	}

	protected void AcknowledgeAcceptedCheckpointCoverage()
	{
		m_fAutosaveElapsed = 0;
		m_bMajorChangePending = false;
		m_fMajorChangeElapsed = 0;
	}

	float GetAutosaveElapsedSeconds()
	{
		return m_fAutosaveElapsed;
	}

	float GetMajorChangeElapsedSeconds()
	{
		return m_fMajorChangeElapsed;
	}

	bool IsMajorChangePending()
	{
		return m_bMajorChangePending;
	}

	int GetSchedulerAttemptSequence()
	{
		return m_iSchedulerAttemptSequence;
	}

	bool RequestCheckpoint(string displayName, HST_CampaignState state = null)
	{
		return RequestTypedCheckpoint(
			displayName,
			ESaveGameType.MANUAL,
			state);
	}

	// Production checkpoint seam shared by periodic, scripted-event, operator,
	// and controlled-shutdown saves. PersistenceSystem.Save only stages the native
	// proxy; the internal callback records the later storage-commit outcome,
	// advances the profile mirror only after that success, and serializes native
	// requests so overlapping checkpoints cannot become unobservable.
	bool RequestTypedCheckpoint(
		string displayName,
		ESaveGameType saveType,
		HST_CampaignState state = null)
	{
		HST_PersistenceCheckpointRequest request = RequestTypedCheckpointDetailed(
			displayName,
			saveType,
			0,
			state,
			null);
		return request && request.WasAccepted();
	}

	HST_PersistenceCheckpointRequest RequestTypedCheckpointDetailed(
		string displayName,
		ESaveGameType saveType,
		ESaveGameRequestFlags requestFlags,
		HST_CampaignState state = null,
		SaveGameOperationCallback completionObserver = null)
	{
		HST_PersistenceCheckpointRequest request
			= new HST_PersistenceCheckpointRequest();
		request.m_eSaveType = saveType;
		request.m_eRequestFlags = requestFlags;
		request.m_sDisplayName = displayName;

		if (m_bCampaignDebugIsolationActive)
		{
			request.m_bCampaignCaptured = CaptureIsolatedCampaignDebugState(
				state,
				"isolated checkpoint: " + displayName);
			request.m_bIsolatedCaptureAccepted = request.m_bCampaignCaptured;
			request.m_sEvidence = "campaign-debug checkpoint remained isolated";
			return request;
		}
		SaveGameManager saveManager = SaveGameManager.Get();
		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		bool nativeCheckpointExpected = persistence
			&& persistence.GetState() == EPersistenceSystemState.ACTIVE;
		if (nativeCheckpointExpected
			&& (!saveManager || !saveManager.IsSavingEnabled()
				|| m_bCheckpointSavePointInFlight || saveManager.IsBusy()
				|| !saveManager.IsSavingAllowed()))
		{
			if (!saveManager)
				request.m_sEvidence
					= "checkpoint deferred before capture: native save manager unavailable";
			else
			{
				request.m_sEvidence = string.Format(
					"checkpoint deferred before capture: native enabled/in flight/busy/allowed %1/%2/%3/%4",
					saveManager.IsSavingEnabled(),
					m_bCheckpointSavePointInFlight,
					saveManager.IsBusy(),
					saveManager.IsSavingAllowed());
			}
			return request;
		}

		if (state && !CaptureAndTrackState(state, "captured before checkpoint"))
		{
			request.m_sEvidence = "campaign capture failed";
			return request;
		}
		request.m_bCampaignCaptured = m_TrackedCampaignSave != null;

		request.m_bTransientStateStaged = FlushTrackedCampaignState(saveType);
		if (nativeCheckpointExpected
			&& !request.m_bTransientStateStaged)
		{
			request.m_sEvidence
				= "checkpoint deferred: native transient staging failed";
			return request;
		}
		// PersistenceSystem.Save only stages the scripted state for the next
		// storage flush. When native authority is active, re-check save-point
		// readiness after capture/staging and queue that native request. The profile
		// mirror advances only from the success callback, after native commit. This
		// prevents an interrupted request from leaving a newer fallback hidden by an
		// older valid native row.
		bool nativeCheckpointRequired = nativeCheckpointExpected
			|| request.m_bTransientStateStaged;
		if (nativeCheckpointRequired)
		{
			saveManager = SaveGameManager.Get();
			if (m_bCheckpointSavePointInFlight
				|| !CanRequestSavePoint(saveManager))
			{
				if (!saveManager)
				{
					request.m_sEvidence
						= "checkpoint deferred after staging: native save manager unavailable";
				}
				else
				{
					request.m_sEvidence = string.Format(
						"checkpoint deferred after staging: native enabled/in flight/busy/allowed %1/%2/%3/%4",
						saveManager.IsSavingEnabled(),
						m_bCheckpointSavePointInFlight,
						saveManager.IsBusy(),
						saveManager.IsSavingAllowed());
				}
				EnsureMajorChangePending();
				return request;
			}

			m_bCheckpointSavePointInFlight = true;
			m_PendingCheckpointRequest = request;
			m_PendingCheckpointSaveData = m_TrackedCampaignSave;
			m_PendingCheckpointObserver = completionObserver;
			m_PendingCheckpointState = state;
			m_iCheckpointRequestSequence++;
			m_fCheckpointCommitElapsedSeconds = 0;
			m_CheckpointCallbackContext
				= new HST_PersistenceCheckpointCallbackContext();
			m_CheckpointCallbackContext.m_iRequestSequence
				= m_iCheckpointRequestSequence;
			m_CheckpointCompletionCallback = new SaveGameOperationCallback(
				OnCheckpointSavePointCompleted,
				m_CheckpointCallbackContext);
			request.m_bSavePointRequested = true;
			saveManager.RequestSavePoint(
				saveType,
				displayName,
				requestFlags,
				m_CheckpointCompletionCallback);
		}
		if (!nativeCheckpointRequired)
		{
			request.m_bProfileFallbackSaved
				= SaveProfileFallback(m_TrackedCampaignSave);
		}

		bool checkpointAccepted = request.WasAccepted();
		if (nativeCheckpointRequired)
			checkpointAccepted = request.m_bSavePointRequested;
		if (!checkpointAccepted)
		{
			EnsureMajorChangePending();
			if (state)
			{
				state.m_sLastPersistenceStatus = string.Format(
					"checkpoint failed: type %1 | transient %2 | profile fallback %3 | savepoint %4",
					saveType,
					request.m_bTransientStateStaged,
					request.m_bProfileFallbackSaved,
					request.m_bSavePointRequested);
			}
			if (state)
				request.m_sEvidence = state.m_sLastPersistenceStatus;
			else
				request.m_sEvidence = "checkpoint failed without campaign state";
			return request;
		}
		// Every accepted checkpoint captures the same complete campaign authority.
		// It therefore covers both scheduler lanes. A mutation that occurs after
		// acceptance starts a fresh first-edge major-change interval, while a later
		// commit/mirror failure rearms that interval from the callback.
		AcknowledgeAcceptedCheckpointCoverage();

		if (state)
		{
			state.m_iLastSaveSecond = state.m_iElapsedSeconds;
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint requested: %1 | type %2 | transient save %3 | profile fallback %4 | savepoint %5 | native in flight %6",
				displayName,
				saveType,
				request.m_bTransientStateStaged,
				request.m_bProfileFallbackSaved,
				request.m_bSavePointRequested,
				m_bCheckpointSavePointInFlight);
		}
		if (state)
			request.m_sEvidence = state.m_sLastPersistenceStatus;
		else
		{
			request.m_sEvidence
				= "checkpoint request accepted without campaign status target";
		}
		return request;
	}

	HST_PersistenceCheckpointRequest RequestAutosaveCheckpointDetailed(
		HST_CampaignState state,
		SaveGameOperationCallback completionObserver = null)
	{
		return RequestTypedCheckpointDetailed(
			"Partisan autosave",
			ESaveGameType.AUTO,
			0,
			state,
			completionObserver);
	}

	HST_PersistenceCheckpointRequest RequestManualCheckpointDetailed(
		HST_CampaignState state,
		SaveGameOperationCallback completionObserver = null)
	{
		return RequestTypedCheckpointDetailed(
			"Partisan manual checkpoint",
			ESaveGameType.MANUAL,
			0,
			state,
			completionObserver);
	}

	HST_PersistenceCheckpointRequest RequestShutdownCheckpointDetailed(
		HST_CampaignState state,
		SaveGameOperationCallback completionObserver = null)
	{
		return RequestTypedCheckpointDetailed(
			"Partisan controlled shutdown",
			ESaveGameType.SHUTDOWN,
			ESaveGameRequestFlags.BLOCKING,
			state,
			completionObserver);
	}

	protected void OnCheckpointSavePointCompleted(
		bool success,
		Managed context = null)
	{
		HST_PersistenceCheckpointCallbackContext callbackContext
			= HST_PersistenceCheckpointCallbackContext.Cast(context);
		if (!m_bCheckpointSavePointInFlight || !callbackContext
			|| callbackContext.m_iRequestSequence
				!= m_iCheckpointRequestSequence)
			return;
		HST_PersistenceCheckpointRequest request = m_PendingCheckpointRequest;
		HST_CampaignSaveData pendingSaveData = m_PendingCheckpointSaveData;
		HST_CampaignState state = m_PendingCheckpointState;
		ref SaveGameOperationCallback observer = m_PendingCheckpointObserver;
		bool profileMirrorSaved;
		if (success)
			profileMirrorSaved = SaveProfileFallback(pendingSaveData);
		if (request)
		{
			request.m_bCompletionReceived = true;
			request.m_bNativeCommitSucceeded = success;
			request.m_bProfileFallbackSaved = profileMirrorSaved;
			request.m_sEvidence += string.Format(
				" | native commit completed %1 | post-commit profile mirror %2",
				success,
				profileMirrorSaved);
		}
		if (!success || !profileMirrorSaved)
			EnsureMajorChangePending();
		if (state)
		{
			state.m_sLastPersistenceStatus += string.Format(
				" | native commit %1 | post-commit profile mirror %2",
				success,
				profileMirrorSaved);
		}
		ClearPendingCheckpointRequest();
		if (observer)
			observer.InvokeDelegate(success && profileMirrorSaved);
	}

	protected void TickCheckpointCommitTimeout(float timeSlice)
	{
		if (!m_bCheckpointSavePointInFlight)
			return;
		m_fCheckpointCommitElapsedSeconds += Math.Max(0.0, timeSlice);
		if (m_fCheckpointCommitElapsedSeconds
			< CHECKPOINT_COMMIT_TIMEOUT_SECONDS)
			return;

		HST_PersistenceCheckpointRequest request = m_PendingCheckpointRequest;
		HST_CampaignState state = m_PendingCheckpointState;
		ref SaveGameOperationCallback observer = m_PendingCheckpointObserver;
		if (request)
		{
			request.m_bCompletionReceived = true;
			request.m_bNativeCommitSucceeded = false;
			request.m_sEvidence += string.Format(
				" | native commit timed out after %1 seconds",
				CHECKPOINT_COMMIT_TIMEOUT_SECONDS);
		}
		if (state)
		{
			state.m_sLastPersistenceStatus += string.Format(
				" | native commit timed out after %1 seconds",
				CHECKPOINT_COMMIT_TIMEOUT_SECONDS);
		}
		EnsureMajorChangePending();
		ClearPendingCheckpointRequest();
		if (observer)
			observer.InvokeDelegate(false);
	}

	protected void ClearPendingCheckpointRequest()
	{
		m_bCheckpointSavePointInFlight = false;
		m_fCheckpointCommitElapsedSeconds = 0;
		m_PendingCheckpointRequest = null;
		m_PendingCheckpointSaveData = null;
		m_PendingCheckpointObserver = null;
		m_CheckpointCompletionCallback = null;
		m_CheckpointCallbackContext = null;
		m_PendingCheckpointState = null;
	}

	bool IsCheckpointSavePointInFlight()
	{
		return m_bCheckpointSavePointInFlight;
	}

	// Controlled campaign-end quiescence stops the normal coordinator cadence,
	// but an in-flight save still needs its real-time timeout to advance.
	void TickPendingCheckpoint(float timeSlice)
	{
		TickCheckpointCommitTimeout(timeSlice);
	}

	void DetachCheckpointCompletionObserver()
	{
		m_PendingCheckpointObserver = null;
	}

	// Coordinator teardown cannot cancel an engine save already in progress,
	// but it must invalidate that request's callback context before releasing
	// campaign-owned references. Incrementing the sequence makes any late native
	// callback harmless; the request remains observably incomplete and failed.
	void CancelPendingCheckpointRequest()
	{
		m_iCheckpointRequestSequence++;
		if (m_PendingCheckpointRequest)
		{
			m_PendingCheckpointRequest.m_bCompletionReceived = false;
			m_PendingCheckpointRequest.m_bNativeCommitSucceeded = false;
			m_PendingCheckpointRequest.m_sEvidence
				+= " | native commit observation cancelled during teardown";
		}
		if (m_PendingCheckpointState)
		{
			m_PendingCheckpointState.m_sLastPersistenceStatus
				+= " | native commit observation cancelled during teardown";
		}
		ClearPendingCheckpointRequest();
	}

	// Proof-only bridge for a guarded disposable profile. It prepares the
	// engine-owned proxy and queues its transient payload, but deliberately does
	// not fall back to the canonical JSON or request a save point. The guarded
	// coordinator owns the blocking SaveGameManager callback and exact UUID.
	bool PrepareNativeSessionSavePoint(
		HST_CampaignState state,
		out string evidence)
	{
		evidence = "native session save-point preparation failed";
		if (!state || m_bCampaignDebugIsolationActive)
			return false;
		HST_CampaignSaveData captured = CaptureAndTrackState(
			state,
			"native session save-point payload prepared");
		if (!captured)
			return false;
		string trackingEvidence;
		if (!IsNativeCampaignStateTracked(trackingEvidence))
		{
			evidence = trackingEvidence;
			return false;
		}
		if (!FlushTrackedCampaignState(ESaveGameType.MANUAL))
		{
			evidence = trackingEvidence
				+ " | native transient save request failed";
			return false;
		}
		evidence = trackingEvidence
			+ " | native transient payload queued for blocking save point";
		return true;
	}

	void CaptureState(HST_CampaignState state)
	{
		if (m_bCampaignDebugIsolationActive)
		{
			CaptureIsolatedCampaignDebugState(state);
			return;
		}

		CaptureAndTrackState(state);
	}

	bool PrepareCampaignDebugIsolation(HST_CampaignState state)
	{
		if (!state)
			return false;
		if (!PrepareStateForCapture(state, "campaign debug isolation baseline"))
			return false;

		if (!m_TrackedCampaignSave)
			m_TrackedCampaignSave = new HST_CampaignSaveData();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		m_TrackedCampaignSave.Capture(state);
		m_LastCapturedSave = m_TrackedCampaignSave;
		TrackCampaignSaveData(m_TrackedCampaignSave);
		bool scriptedStateSaved = FlushTrackedCampaignState(ESaveGameType.MANUAL);
		bool profileFallbackSaved = SaveProfileFallback(m_TrackedCampaignSave);
		m_bMajorChangePending = false;
		m_fMajorChangeElapsed = 0;
		m_fAutosaveElapsed = 0;
		m_bCampaignDebugIsolationActive = scriptedStateSaved || profileFallbackSaved;
		return m_bCampaignDebugIsolationActive;
	}

	bool CaptureIsolatedCampaignDebugState(HST_CampaignState state, string persistenceStatus = "isolated campaign debug checkpoint")
	{
		if (!state)
			return false;
		if (!PrepareStateForCapture(state, persistenceStatus))
			return false;

		m_IsolatedCapturedSave = new HST_CampaignSaveData();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastSaveSecond = state.m_iElapsedSeconds;
		if (!persistenceStatus.IsEmpty())
			state.m_sLastPersistenceStatus = persistenceStatus;
		m_IsolatedCapturedSave.Capture(state);
		m_LastCapturedSave = m_IsolatedCapturedSave;
		return true;
	}

	// The external-restart proof owns a disposable profile and a detached state.
	// Keep this path independent from native persistence tracking and the live
	// campaign save carrier so proof setup cannot replace normal save authority.
	bool WriteProfileFallbackProofSnapshot(
		HST_CampaignState state,
		string persistenceStatus,
		out HST_CampaignState readBackState,
		out string evidence)
	{
		readBackState = null;
		evidence = "profile fallback proof snapshot was not written";
		if (!state)
		{
			evidence = "profile fallback proof snapshot write rejected: state is null";
			return false;
		}

		string captureStatus = persistenceStatus;
		if (captureStatus.IsEmpty())
			captureStatus = "external restart proof snapshot prepared";
		if (!PrepareStateForCapture(state, captureStatus))
		{
			evidence = string.Format(
				"profile fallback proof snapshot preparation failed | status %1",
				state.m_sLastPersistenceStatus);
			return false;
		}

		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastSaveSecond = state.m_iElapsedSeconds;
		state.m_sLastPersistenceStatus = captureStatus;
		HST_CampaignSaveData detachedSave = new HST_CampaignSaveData();
		detachedSave.Capture(state);
		if (!SaveProfileFallback(detachedSave))
		{
			evidence = string.Format(
				"profile fallback proof snapshot write failed | schema %1 | status %2",
				detachedSave.m_iSchemaVersion,
				m_sProfileFallbackStatus);
			return false;
		}

		string readEvidence;
		if (!ReadProfileFallbackProofSnapshot(readBackState, readEvidence))
		{
			evidence = string.Format(
				"profile fallback proof snapshot wrote schema %1 but canonical readback failed | %2",
				detachedSave.m_iSchemaVersion,
				readEvidence);
			return false;
		}

		evidence = string.Format(
			"profile fallback proof snapshot write/readback passed | captured schema %1 | captured status %2 | %3",
			detachedSave.m_iSchemaVersion,
			captureStatus,
			readEvidence);
		return true;
	}

	bool ReadProfileFallbackProofSnapshot(
		out HST_CampaignState readBackState,
		out string evidence)
	{
		readBackState = null;
		evidence = "profile fallback proof snapshot read was not attempted";
		if (!FileIO.FileExists(HST_ProfilePathService.CAMPAIGN_SAVE_FILE))
		{
			evidence = "profile fallback proof snapshot is missing from the canonical profile";
			return false;
		}

		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE))
		{
			evidence = "profile fallback proof snapshot canonical file load failed";
			return false;
		}

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		if (!context.ReadValue("", saveData))
		{
			evidence = "profile fallback proof snapshot canonical JSON read failed";
			return false;
		}

		int storedSchema = saveData.m_iSchemaVersion;
		int storedLastSaveSecond = saveData.m_iLastSaveSecond;
		string storedStatus = saveData.m_sLastPersistenceStatus;
		readBackState = saveData.Restore();
		if (!readBackState)
		{
			evidence = string.Format(
				"profile fallback proof snapshot restore failed | stored schema %1 | status %2",
				storedSchema,
				storedStatus);
			return false;
		}

		evidence = string.Format(
			"canonical readback restored | stored schema %1 | loaded schema %2 | current schema %3 | save second %4 | status %5 | qrf %6 | operations %7 | orders %8 | mutations %9",
			storedSchema,
			readBackState.m_iLastLoadedSchemaVersion,
			readBackState.m_iSchemaVersion,
			storedLastSaveSecond,
			storedStatus,
			readBackState.m_aQRFs.Count(),
			readBackState.m_aOperations.Count(),
			readBackState.m_aEnemyOrders.Count(),
			readBackState.m_aEnemyStrategicMutations.Count());
		if (readBackState.m_iSchemaVersion != HST_CampaignState.SCHEMA_VERSION)
		{
			evidence = evidence + string.Format(
				" | expected current schema %1",
				HST_CampaignState.SCHEMA_VERSION);
			return false;
		}
		return true;
	}

	// Read-only production proof observation. The fingerprint is calculated from
	// the serialized DTO before Restore mutates any live bookkeeping fields.
	bool ReadCanonicalProfileFallbackSnapshot(
		out HST_CampaignState readBackState,
		out string snapshotFingerprint,
		out string evidence)
	{
		readBackState = null;
		snapshotFingerprint = "";
		evidence = "canonical profile fallback observation was not attempted";
		if (!FileIO.FileExists(HST_ProfilePathService.CAMPAIGN_SAVE_FILE))
		{
			evidence = "canonical profile fallback is missing";
			return false;
		}
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE))
		{
			evidence = "canonical profile fallback JSON load failed";
			return false;
		}
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		if (!context.ReadValue("", saveData))
		{
			evidence = "canonical profile fallback DTO read failed";
			return false;
		}
		snapshotFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(saveData);
		readBackState = saveData.Restore();
		if (snapshotFingerprint.IsEmpty() || !readBackState
			|| readBackState.m_iSchemaVersion
				!= HST_CampaignState.SCHEMA_VERSION)
		{
			evidence = string.Format(
				"canonical profile fallback validation failed | fingerprint %1 | restored %2",
				!snapshotFingerprint.IsEmpty(),
				readBackState != null);
			return false;
		}
		evidence = string.Format(
			"canonical profile fallback observed | schema %1 | fingerprint %2",
			readBackState.m_iSchemaVersion,
			snapshotFingerprint);
		return true;
	}

	string GetLastCapturedSnapshotFingerprint()
	{
		return HST_CampaignPersistentState.BuildSnapshotFingerprint(
			m_LastCapturedSave);
	}

	// Controlled shutdown compares gameplay state after an asynchronous commit
	// with the state present when that commit was queued. Save bookkeeping is
	// deliberately normalized because RequestTypedCheckpointDetailed updates it
	// after the native DTO has already been captured.
	bool BuildCampaignStabilityFingerprint(
		HST_CampaignState state,
		bool prepareCurrentAuthority,
		out string snapshotFingerprint,
		out string evidence)
	{
		snapshotFingerprint = "";
		evidence = "campaign stability fingerprint was not attempted";
		if (!state)
		{
			evidence = "campaign stability fingerprint has no state";
			return false;
		}
		if (prepareCurrentAuthority
			&& !PrepareStateForCapture(
				state,
				"controlled campaign end stability check"))
		{
			evidence = state.m_sLastPersistenceStatus;
			return false;
		}

		HST_CampaignSaveData snapshot = new HST_CampaignSaveData();
		snapshot.Capture(state);
		snapshot.m_iLastSaveSecond = 0;
		snapshot.m_sLastPersistenceStatus = "";
		snapshotFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(snapshot);
		if (snapshotFingerprint.IsEmpty())
		{
			evidence = "campaign stability fingerprint serialization failed";
			return false;
		}
		evidence = "normalized campaign stability fingerprint "
			+ snapshotFingerprint;
		return true;
	}

	bool RestoreTrackedStateAfterCampaignDebug(HST_CampaignState state)
	{
		if (!state)
			return false;
		if (!PrepareStateForCapture(state, "campaign debug tracked-state restore"))
			return false;
		m_bCampaignDebugIsolationActive = false;

		if (!m_TrackedCampaignSave)
			m_TrackedCampaignSave = new HST_CampaignSaveData();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		m_TrackedCampaignSave.Capture(state);
		m_LastCapturedSave = m_TrackedCampaignSave;
		m_IsolatedCapturedSave = null;
		TrackCampaignSaveData(m_TrackedCampaignSave);
		bool scriptedStateSaved = FlushTrackedCampaignState(ESaveGameType.MANUAL);
		bool profileFallbackSaved = SaveProfileFallback(m_TrackedCampaignSave);
		m_bMajorChangePending = false;
		m_fMajorChangeElapsed = 0;
		m_fAutosaveElapsed = 0;
		return scriptedStateSaved || profileFallbackSaved;
	}

	HST_CampaignSaveData CaptureAndTrackState(HST_CampaignState state, string persistenceStatus = "captured and tracked")
	{
		if (!state)
			return null;
		if (m_bCampaignDebugIsolationActive)
		{
			if (!CaptureIsolatedCampaignDebugState(state, persistenceStatus))
				return null;
			return m_IsolatedCapturedSave;
		}
		if (!PrepareStateForCapture(state, persistenceStatus))
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

	protected bool PrepareStateForCapture(HST_CampaignState state, string context)
	{
		if (!state)
			return false;
		HST_OperationRecordState dematerializingExactEnemyResponse
			= FindDematerializingExactEnemyResponse(state);
		if (dematerializingExactEnemyResponse)
		{
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint deferred: exact enemy response dematerialization is in progress during %1 | operation %2",
				BoundExactEnemyResponsePersistenceStatusPart(context, 72),
				BoundExactEnemyResponsePersistenceStatusPart(
					dematerializingExactEnemyResponse.m_sOperationId,
					72));
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		if (!PreflightPhysicalExactEnemyResponseGraphs(state, context))
			return false;
		if (!m_Civilians)
		{
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint deferred: ambient civilian persistence authority is unavailable during %1",
				context);
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		if (!m_Civilians.PrepareAmbientVehiclePersistence(state))
		{
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint deferred: ambient civilian persistence reconciliation failed during %1",
				context);
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		bool hasExactRescueAuthority = HasExactRescuePOWAuthority(state);
		if (hasExactRescueAuthority && !m_RescuePOWOperations)
		{
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint deferred: exact rescue POW persistence authority is unavailable during %1",
				context);
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		if (hasExactRescueAuthority)
		{
			string rescueFailure;
			if (!m_RescuePOWOperations.PrepareQuarantinedAuthorityForPersistence(state, rescueFailure)
				|| !m_RescuePOWOperations.PrepareOpenPhysicalAuthorityForPersistence(state, rescueFailure))
			{
				state.m_sLastPersistenceStatus = string.Format(
					"checkpoint deferred: exact rescue POW authority reconciliation failed during %1 | %2",
					context,
					rescueFailure);
				Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
				return false;
			}
		}
		string quarantineFailure;
		if (!NormalizeRetiredQuarantinedEnemyPatrolAuthority(state, quarantineFailure))
		{
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint deferred: exact patrol quarantine cleanup is incomplete during %1 | %2",
				context,
				quarantineFailure);
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		if (!NormalizeRetiredQuarantinedGarrisonPatrolAuthority(state, quarantineFailure))
		{
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint deferred: exact garrison patrol quarantine cleanup is incomplete during %1 | %2",
				context,
				quarantineFailure);
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		bool hasOpenExactLocalSecurity;
		bool hasQuarantinedLocalSecurity;
		foreach (HST_LocalSecurityPatrolState localSecurityPatrol : state.m_aLocalSecurityPatrols)
		{
			if (!localSecurityPatrol)
				continue;
			if (localSecurityPatrol.m_iContractVersion
					== HST_LocalSecurityOperationService.EXACT_CONTRACT_VERSION
				&& localSecurityPatrol.m_sStatus == "active")
				hasOpenExactLocalSecurity = true;
			if (localSecurityPatrol.m_iContractVersion
					== HST_LocalSecurityOperationService.QUARANTINED_CONTRACT_VERSION
				|| localSecurityPatrol.m_sStatus == "quarantined")
				hasQuarantinedLocalSecurity = true;
		}
		if ((hasOpenExactLocalSecurity || hasQuarantinedLocalSecurity)
			&& !m_LocalSecurityPatrolOperations)
		{
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint deferred: exact local-security persistence authority is unavailable during %1",
				context);
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		if (hasQuarantinedLocalSecurity)
		{
			string localSecurityQuarantineFailure;
			if (!m_LocalSecurityPatrolOperations.PrepareQuarantinedAuthorityForPersistence(
				state,
				localSecurityQuarantineFailure))
			{
				state.m_sLastPersistenceStatus = string.Format(
					"checkpoint deferred: quarantined local-security cleanup failed during %1 | %2",
					context,
					localSecurityQuarantineFailure);
				Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
				return false;
			}
		}
		if (hasOpenExactLocalSecurity)
		{
			string localSecurityFailure;
			if (!m_LocalSecurityPatrolOperations.PrepareOpenPhysicalAuthorityForPersistence(
				state,
				localSecurityFailure))
			{
				state.m_sLastPersistenceStatus = string.Format(
					"checkpoint deferred: exact local-security roster reconciliation failed during %1 | %2",
					context,
					localSecurityFailure);
				Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
				return false;
			}
		}
		bool hasQuarantinedMissionGuard = HasQuarantinedMissionGuardAuthority(state);
		if (!m_MissionGuardOperations && hasQuarantinedMissionGuard)
		{
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint deferred: exact mission guard quarantine cleanup is unavailable during %1",
				context);
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		if (hasQuarantinedMissionGuard && m_MissionGuardOperations
			&& !m_MissionGuardOperations.PrepareQuarantinedAuthorityForPersistence(state, quarantineFailure))
		{
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint deferred: exact mission guard quarantine cleanup is incomplete during %1 | %2",
				context,
				quarantineFailure);
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		if (hasQuarantinedMissionGuard
			&& !ValidateQuarantinedMissionGuardCleanup(state, quarantineFailure))
		{
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint deferred: exact mission guard quarantine residue remains during %1 | %2",
				context,
				quarantineFailure);
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		bool hasExactMissionConvoy;
		bool hasPhysicalExactEnemyResponse;
		bool hasPhysicalExactEnemyPatrol;
		bool hasPhysicalExactLocalSecurity;
		bool hasPhysicalExactGarrisonPatrol;
		bool hasPhysicalExactMissionGuard;
		bool hasPhysicalExactPlayerSupport;
		bool hasMaterializingExactInfantry;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (mission && mission.m_sRuntimePrimitive == "convoy_intercept"
				&& mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION)
			{
				hasExactMissionConvoy = true;
				break;
			}
		}
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
				continue;
			bool exactEnemyPatrol = operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL
				&& operation.m_iContractVersion == HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION;
			bool exactEnemyResponse = (operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF
					&& operation.m_iContractVersion == HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION)
				|| (operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK
					&& operation.m_iContractVersion == HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION)
				|| (operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_GARRISON_REBUILD
					&& operation.m_iContractVersion
						== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_CONTRACT_VERSION);
			bool exactGarrisonPatrol = operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL
				&& operation.m_iContractVersion == HST_GarrisonPatrolOperationService.EXACT_CONTRACT_VERSION;
			bool exactLocalSecurity = operation.m_eType
					== HST_EOperationType.HST_OPERATION_TYPE_LOCAL_SECURITY_PATROL
				&& operation.m_iContractVersion
					== HST_LocalSecurityOperationService.EXACT_CONTRACT_VERSION;
			bool exactMissionGuard = operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
				&& HST_MissionGuardOperationService.IsSupportedExactContractVersion(
					operation.m_iContractVersion);
			bool exactPlayerSupport = HST_OperationService.IsExactPlayerSupportOperationType(operation.m_eType)
				&& operation.m_iContractVersion != 0;
			if (!exactEnemyResponse && !exactEnemyPatrol && !exactLocalSecurity && !exactGarrisonPatrol
				&& !exactMissionGuard && !exactPlayerSupport)
				continue;
			if (operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
			{
				hasMaterializingExactInfantry = true;
				continue;
			}
			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
			{
				if (exactEnemyResponse)
					hasPhysicalExactEnemyResponse = true;
				else if (exactEnemyPatrol)
					hasPhysicalExactEnemyPatrol = true;
				else if (exactLocalSecurity)
					hasPhysicalExactLocalSecurity = true;
				else if (exactGarrisonPatrol)
					hasPhysicalExactGarrisonPatrol = true;
				else if (exactMissionGuard)
					hasPhysicalExactMissionGuard = true;
				else
					hasPhysicalExactPlayerSupport = true;
			}
		}
		if (hasMaterializingExactInfantry)
		{
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint deferred: exact infantry materialization is in progress during %1",
				context);
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		if (!m_PhysicalWar)
		{
			if (!hasExactMissionConvoy && !hasPhysicalExactEnemyResponse && !hasPhysicalExactEnemyPatrol
				&& !hasPhysicalExactLocalSecurity && !hasPhysicalExactGarrisonPatrol
				&& !hasPhysicalExactMissionGuard
				&& !hasPhysicalExactPlayerSupport)
				return true;
			state.m_sLastPersistenceStatus = "checkpoint deferred: exact physical roster reconciler is unavailable";
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}

		string reconcileFailure;
		if (!m_PhysicalWar.PrepareExactMissionConvoyAuthorityForPersistence(state, reconcileFailure))
		{
			state.m_sLastPersistenceStatus = string.Format("checkpoint deferred: exact convoy roster reconciliation failed during %1 | %2", context, reconcileFailure);
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		if (hasPhysicalExactMissionGuard)
		{
			if (!m_MissionGuardOperations)
			{
				state.m_sLastPersistenceStatus = "checkpoint deferred: exact mission guard persistence authority is unavailable";
				Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
				return false;
			}
			if (!m_MissionGuardOperations.PrepareOpenPhysicalAuthorityForPersistence(state, reconcileFailure))
			{
				state.m_sLastPersistenceStatus = string.Format(
					"checkpoint deferred: exact mission guard roster reconciliation failed during %1 | %2",
					context,
					reconcileFailure);
				Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
				return false;
			}
		}
		if (!hasPhysicalExactEnemyResponse && !hasPhysicalExactEnemyPatrol && !hasPhysicalExactLocalSecurity
			&& !hasPhysicalExactGarrisonPatrol
			&& !hasPhysicalExactMissionGuard && !hasPhysicalExactPlayerSupport)
			return true;
		if (!m_ForceSpawnQueue || !m_ForceSpawnAdapter)
		{
			state.m_sLastPersistenceStatus = "checkpoint deferred: exact infantry roster authority is unavailable";
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}

		HST_ForceSpawnAdapterTickResult exactInfantryRoster = m_ForceSpawnAdapter.ReconcileExactInfantryAuthorityForPersistence(
			state,
			m_ForceSpawnQueue,
			m_PhysicalWar,
			Math.Max(0, state.m_iElapsedSeconds));
		if (!exactInfantryRoster || exactInfantryRoster.m_iFailedCount > 0)
		{
			string rosterEvidence = "missing adapter reconciliation result";
			if (exactInfantryRoster && !exactInfantryRoster.m_sSummary.IsEmpty())
				rosterEvidence = exactInfantryRoster.m_sSummary;
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint deferred: exact infantry roster reconciliation failed during %1 | %2",
				context,
				rosterEvidence);
			Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		array<ref HST_ExactEnemyResponsePersistencePositionPlan> exactEnemyResponsePositionPlans = {};
		if (!ValidatePhysicalExactEnemyResponseSnapshots(
			state,
			context,
			exactEnemyResponsePositionPlans))
			return false;
		if (!ValidatePhysicalEnemyPatrolSnapshots(state, context))
			return false;
		if (!ValidatePhysicalGarrisonPatrolSnapshots(state, context))
			return false;
		if (!ValidatePhysicalMissionGuardSnapshots(state, context))
			return false;
		if (!ValidatePhysicalPlayerSupportBindings(state, context))
			return false;
		return ApplyPhysicalExactEnemyResponseSnapshots(
			state,
			context,
			exactEnemyResponsePositionPlans);
	}

	protected bool HasQuarantinedMissionGuardAuthority(HST_CampaignState state)
	{
		if (!state)
			return false;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (HST_MissionGuardOperationService.IsQuarantinedMission(mission))
				return true;
		}
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
				&& HST_MissionGuardOperationService.IsQuarantinedOperationContractVersion(
					operation.m_iContractVersion))
				return true;
		}
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && (group.m_sRuntimeStatus == HST_MissionGuardOperationService.QUARANTINE_STATUS
				|| group.m_sSpawnFallbackMode == HST_MissionGuardOperationService.QUARANTINE_STATUS))
				return true;
		}
		return false;
	}

	protected bool ValidateQuarantinedMissionGuardCleanup(
		HST_CampaignState state,
		out string failure)
	{
		failure = "";
		if (!state)
		{
			failure = "campaign state is unavailable";
			return false;
		}
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
				|| !HST_MissionGuardOperationService.IsQuarantinedOperationContractVersion(
					operation.m_iContractVersion))
				continue;
			HST_ForceSpawnResultState batch;
			int batchMatches;
			foreach (HST_ForceSpawnResultState candidateBatch : state.m_aForceSpawnResults)
			{
				if (!QuarantinedMissionGuardBatchClaimsOperation(candidateBatch, operation))
					continue;
				batch = candidateBatch;
				batchMatches++;
			}
			if (batchMatches > 1)
			{
				failure = "quarantined exact mission guard batch authority is ambiguous " + operation.m_sOperationId;
				return false;
			}
			HST_ActiveGroupState group;
			int groupMatches;
			foreach (HST_ActiveGroupState candidateGroup : state.m_aActiveGroups)
			{
				if (!QuarantinedMissionGuardGroupClaimsOperation(candidateGroup, operation))
					continue;
				group = candidateGroup;
				groupMatches++;
			}
			if (groupMatches > 1)
			{
				failure = "quarantined exact mission guard group authority is ambiguous " + operation.m_sOperationId;
				return false;
			}
			if (batch && !IsQuarantinedMissionGuardBatchCleanupComplete(batch))
			{
				failure = "quarantined exact mission guard queue cleanup is incomplete " + operation.m_sOperationId;
				return false;
			}
			if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				&& (operation.m_eMaterializationState
					!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
					|| operation.m_ePositionAuthority
						!= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC))
			{
				failure = "quarantined exact mission guard retains stale live operation authority " + operation.m_sOperationId;
				return false;
			}
			if (m_ForceSpawnAdapter)
			{
				if ((!operation.m_sProjectionId.IsEmpty()
					&& m_ForceSpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0)
					|| (batch && !batch.m_sResultId.IsEmpty()
						&& m_ForceSpawnAdapter.CountHandlesForResultId(batch.m_sResultId) > 0))
				{
					failure = "quarantined exact mission guard retains adapter bindings " + operation.m_sOperationId;
					return false;
				}
			}
			if (group)
			{
				if (group.m_bSpawnedEntity || !group.m_sRuntimeEntityId.IsEmpty()
					|| group.m_iSpawnedAgentCount > 0 || group.m_iAssignedWaypointCount > 0)
				{
					failure = "quarantined exact mission guard retains process-local group authority " + operation.m_sOperationId;
					return false;
				}
				if (!m_PhysicalWar || m_PhysicalWar.GetForceSpawnGroupRoot(group)
					|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0)
				{
					failure = "quarantined exact mission guard PhysicalWar cleanup is unproven " + operation.m_sOperationId;
					return false;
				}
			}
		}
		foreach (HST_ActiveGroupState orphanGroup : state.m_aActiveGroups)
		{
			if (!orphanGroup || (orphanGroup.m_sRuntimeStatus != HST_MissionGuardOperationService.QUARANTINE_STATUS
				&& orphanGroup.m_sSpawnFallbackMode != HST_MissionGuardOperationService.QUARANTINE_STATUS))
				continue;
			HST_OperationRecordState operation = state.FindOperation(orphanGroup.m_sOperationId);
			if (operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
				&& HST_MissionGuardOperationService.IsQuarantinedOperationContractVersion(
					operation.m_iContractVersion)
				&& QuarantinedMissionGuardGroupClaimsOperation(orphanGroup, operation))
				continue;
			if (orphanGroup.m_bSpawnedEntity || !orphanGroup.m_sRuntimeEntityId.IsEmpty()
				|| orphanGroup.m_iSpawnedAgentCount > 0 || orphanGroup.m_iAssignedWaypointCount > 0)
			{
				failure = "orphan quarantined exact mission guard retains process-local group authority " + orphanGroup.m_sGroupId;
				return false;
			}
			if (!m_PhysicalWar || m_PhysicalWar.GetForceSpawnGroupRoot(orphanGroup)
				|| m_PhysicalWar.CountForceSpawnRuntimeMembers(orphanGroup) > 0)
			{
				failure = "orphan quarantined exact mission guard PhysicalWar cleanup is unproven " + orphanGroup.m_sGroupId;
				return false;
			}
			if (m_ForceSpawnAdapter && ((!orphanGroup.m_sProjectionId.IsEmpty()
				&& m_ForceSpawnAdapter.CountHandlesForProjection(orphanGroup.m_sProjectionId) > 0)
				|| (!orphanGroup.m_sSpawnResultId.IsEmpty()
					&& m_ForceSpawnAdapter.CountHandlesForResultId(orphanGroup.m_sSpawnResultId) > 0)))
			{
				failure = "orphan quarantined exact mission guard retains adapter bindings " + orphanGroup.m_sGroupId;
				return false;
			}
			HST_ForceSpawnResultState batch = state.FindForceSpawnResult(orphanGroup.m_sSpawnResultId);
			if (batch && batch.m_sOperationId == orphanGroup.m_sOperationId
				&& batch.m_sManifestId == orphanGroup.m_sManifestId
				&& !IsQuarantinedMissionGuardBatchCleanupComplete(batch))
			{
				failure = "orphan quarantined exact mission guard queue cleanup is incomplete " + orphanGroup.m_sGroupId;
				return false;
			}
		}
		return true;
	}

	protected bool HasExactRescuePOWAuthority(HST_CampaignState state)
	{
		if (!state)
			return false;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
				return true;
		}
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE)
				return true;
		}
		return false;
	}

	protected bool QuarantinedMissionGuardBatchClaimsOperation(
		HST_ForceSpawnResultState batch,
		HST_OperationRecordState operation)
	{
		if (!batch || !operation || operation.m_sOperationId.IsEmpty()
			|| batch.m_sOperationId != operation.m_sOperationId)
			return false;
		return (!operation.m_sSpawnResultId.IsEmpty() && batch.m_sResultId == operation.m_sSpawnResultId)
			|| (!operation.m_sManifestId.IsEmpty() && batch.m_sManifestId == operation.m_sManifestId)
			|| (!operation.m_sForceId.IsEmpty() && batch.m_sForceId == operation.m_sForceId)
			|| (!operation.m_sProjectionId.IsEmpty() && batch.m_sProjectionId == operation.m_sProjectionId);
	}

	protected bool QuarantinedMissionGuardGroupClaimsOperation(
		HST_ActiveGroupState group,
		HST_OperationRecordState operation)
	{
		if (!group || !operation || operation.m_sOperationId.IsEmpty()
			|| group.m_sOperationId != operation.m_sOperationId)
			return false;
		return (!operation.m_sGroupId.IsEmpty() && group.m_sGroupId == operation.m_sGroupId)
			|| (!operation.m_sManifestId.IsEmpty() && group.m_sManifestId == operation.m_sManifestId)
			|| (!operation.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == operation.m_sSpawnResultId)
			|| (!operation.m_sForceId.IsEmpty() && group.m_sForceId == operation.m_sForceId)
			|| (!operation.m_sProjectionId.IsEmpty() && group.m_sProjectionId == operation.m_sProjectionId);
	}

	protected bool IsQuarantinedMissionGuardBatchCleanupComplete(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return true;
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED
			&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL)
			return false;
		if (!batch.m_sNativeGroupId.IsEmpty())
			return false;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot)
				continue;
			if (!slot.m_sEntityId.IsEmpty() || !slot.m_sAssignedVehicleEntityId.IsEmpty()
				|| !slot.m_sNativeGroupId.IsEmpty())
				return false;
			if (slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
				|| slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING
				|| slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING)
				return false;
		}
		return true;
	}

	protected bool NormalizeRetiredQuarantinedEnemyPatrolAuthority(
		HST_CampaignState state,
		out string failure)
	{
		failure = "";
		if (!state)
		{
			failure = "campaign state is unavailable";
			return false;
		}
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order
				|| order.m_iOperationContractVersion != HST_EnemyPatrolOperationService.QUARANTINED_CONTRACT_VERSION)
				continue;
			HST_OperationRecordState operation;
			int operationMatches;
			foreach (HST_OperationRecordState candidateOperation : state.m_aOperations)
			{
				if (!QuarantinedOperationClaimsOrder(candidateOperation, order))
					continue;
				operationMatches++;
				operation = candidateOperation;
			}
			if (operationMatches <= 0)
			{
				bool orphanClaimant;
				foreach (HST_ActiveGroupState orphanGroup : state.m_aActiveGroups)
				{
					if (QuarantinedGroupClaimsOrder(orphanGroup, order, null))
					{
						orphanClaimant = true;
						break;
					}
				}
				if (!orphanClaimant)
				{
					foreach (HST_ForceSpawnResultState orphanBatch : state.m_aForceSpawnResults)
					{
						if (QuarantinedBatchClaimsOrder(orphanBatch, order, null, null))
						{
							orphanClaimant = true;
							break;
						}
					}
				}
				if (!orphanClaimant && m_ForceSpawnAdapter)
				{
					int orphanHandleCount;
					if (!order.m_sGroupId.IsEmpty())
						orphanHandleCount = m_ForceSpawnAdapter.CountHandlesForProjection(order.m_sGroupId);
					if (!order.m_sSpawnResultId.IsEmpty())
						orphanHandleCount += m_ForceSpawnAdapter.CountHandlesForResultId(order.m_sSpawnResultId);
					orphanClaimant = orphanHandleCount > 0;
				}
				if (orphanClaimant)
				{
					failure = "quarantined patrol claimant has no recognized exact operation " + order.m_sOrderId;
					return false;
				}
				continue;
			}
			if (operationMatches != 1 || !operation)
			{
				failure = "quarantined exact patrol operation authority is ambiguous " + order.m_sOrderId;
				return false;
			}
			bool openOperation = operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
			HST_ActiveGroupState group;
			int groupMatches;
			foreach (HST_ActiveGroupState candidateGroup : state.m_aActiveGroups)
			{
				if (!QuarantinedGroupClaimsOrder(candidateGroup, order, operation))
					continue;
				groupMatches++;
				group = candidateGroup;
			}
			if (groupMatches > 1)
			{
				failure = "quarantined exact patrol group authority is ambiguous " + order.m_sOrderId;
				return false;
			}
			HST_ForceSpawnResultState batch;
			int batchMatches;
			foreach (HST_ForceSpawnResultState candidateBatch : state.m_aForceSpawnResults)
			{
				if (!QuarantinedBatchClaimsOrder(candidateBatch, order, operation, group))
					continue;
				batchMatches++;
				batch = candidateBatch;
			}
			if (batchMatches > 1)
			{
				failure = "quarantined exact patrol spawn authority is ambiguous " + order.m_sOrderId;
				return false;
			}
			string projectionId = operation.m_sProjectionId;
			if (projectionId.IsEmpty() && group)
				projectionId = group.m_sProjectionId;
			if (projectionId.IsEmpty() && batch)
				projectionId = batch.m_sProjectionId;
			int handleCount;
			if (m_ForceSpawnAdapter && !projectionId.IsEmpty())
				handleCount = m_ForceSpawnAdapter.CountHandlesForProjection(projectionId);
			bool physicalRegistryOwned;
			if (m_PhysicalWar && group)
				physicalRegistryOwned = m_PhysicalWar.GetForceSpawnGroupRoot(group)
					|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0;
			bool liveAuthority = operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
				|| (group && (group.m_bSpawnedEntity || !group.m_sRuntimeEntityId.IsEmpty()))
				|| (batch && !batch.m_sNativeGroupId.IsEmpty())
				|| handleCount > 0 || physicalRegistryOwned;
			if (!liveAuthority)
				continue;
			if (!m_ForceSpawnAdapter || projectionId.IsEmpty())
			{
				failure = "quarantined exact patrol runtime binding authority is unavailable " + order.m_sOrderId;
				return false;
			}
			if (handleCount > 0)
			{
				failure = string.Format("quarantined exact patrol still owns %1 runtime bindings %2", handleCount, order.m_sOrderId);
				return false;
			}
			if (openOperation && group
				&& (group.m_bSpawnedEntity || !group.m_sRuntimeEntityId.IsEmpty()))
			{
				failure = "quarantined exact patrol still reports a process-local group " + order.m_sOrderId;
				return false;
			}
			if (!m_PhysicalWar || !group)
			{
				failure = "quarantined exact patrol physical registry cannot be proven empty " + order.m_sOrderId;
				return false;
			}
			if (m_PhysicalWar.GetForceSpawnGroupRoot(group)
				|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0)
			{
				failure = "quarantined exact patrol still owns PhysicalWar runtime entities " + order.m_sOrderId;
				return false;
			}
			if (!openOperation)
			{
				if (batch)
					batch.m_sNativeGroupId = "";
				if (group)
				{
					group.m_bSpawnedEntity = false;
					group.m_sRuntimeEntityId = "";
					group.m_iSpawnedAgentCount = 0;
					group.m_iAssignedWaypointCount = 0;
					group.m_iLifecycleRevision++;
				}
				continue;
			}
			if (group && HasNonZeroPosition(group.m_vPosition))
				operation.m_vStrategicPosition = group.m_vPosition;
			operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
			operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
			operation.m_iMaterializationStateEnteredAtSecond = Math.Max(0, state.m_iElapsedSeconds);
			operation.m_iLastProgressAtSecond = Math.Max(0, state.m_iElapsedSeconds);
			operation.m_iRevision++;
			if (batch)
			{
				batch.m_sNativeGroupId = "";
				batch.m_bStrategicProjectionHeld = true;
			}
			if (group)
			{
				group.m_bSpawnedEntity = false;
				group.m_sRuntimeEntityId = "";
				group.m_iSpawnedAgentCount = 0;
				group.m_iAssignedWaypointCount = 0;
				group.m_sRuntimeStatus = "exact_patrol_quarantined";
				group.m_iLifecycleRevision++;
			}
		}
		return true;
	}

	protected bool NormalizeRetiredQuarantinedGarrisonPatrolAuthority(
		HST_CampaignState state,
		out string failure)
	{
		failure = "";
		if (!state)
		{
			failure = "campaign state is unavailable";
			return false;
		}
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation
				|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL
				|| operation.m_iContractVersion != HST_GarrisonPatrolOperationService.QUARANTINED_CONTRACT_VERSION)
				continue;

			HST_ActiveGroupState group;
			int groupMatches;
			foreach (HST_ActiveGroupState candidateGroup : state.m_aActiveGroups)
			{
				if (!QuarantinedGarrisonGroupClaimsOperation(candidateGroup, operation))
					continue;
				groupMatches++;
				group = candidateGroup;
			}
			if (groupMatches > 1)
			{
				failure = "quarantined exact garrison patrol group authority is ambiguous " + operation.m_sOperationId;
				return false;
			}

			HST_ForceSpawnResultState batch;
			int batchMatches;
			foreach (HST_ForceSpawnResultState candidateBatch : state.m_aForceSpawnResults)
			{
				if (!QuarantinedGarrisonBatchClaimsOperation(candidateBatch, operation, group))
					continue;
				batchMatches++;
				batch = candidateBatch;
			}
			if (batchMatches > 1)
			{
				failure = "quarantined exact garrison patrol spawn authority is ambiguous " + operation.m_sOperationId;
				return false;
			}

			string projectionId = operation.m_sProjectionId;
			if (projectionId.IsEmpty() && group)
				projectionId = group.m_sProjectionId;
			if (projectionId.IsEmpty() && batch)
				projectionId = batch.m_sProjectionId;
			int projectionHandles;
			int resultHandles;
			if (m_ForceSpawnAdapter)
			{
				if (!projectionId.IsEmpty())
					projectionHandles = m_ForceSpawnAdapter.CountHandlesForProjection(projectionId);
				if (batch && !batch.m_sResultId.IsEmpty())
					resultHandles = m_ForceSpawnAdapter.CountHandlesForResultId(batch.m_sResultId);
			}
			if (projectionHandles > 0 || resultHandles > 0)
			{
				failure = string.Format(
					"quarantined exact garrison patrol still owns adapter bindings %1 | projection %2 | result %3",
					operation.m_sOperationId,
					projectionHandles,
					resultHandles);
				return false;
			}

			bool operationLive = operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
				|| operation.m_eMaterializationState
					== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
				|| operation.m_eMaterializationState
					== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState
					== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
			bool groupProcessEvidence = QuarantinedGarrisonGroupHasProcessAuthority(group);
			bool batchCleanupIncomplete = !IsQuarantinedGarrisonBatchCleanupComplete(batch);
			bool runtimeShaped = operationLive || groupProcessEvidence || batchCleanupIncomplete;
			if (runtimeShaped && !m_ForceSpawnAdapter)
			{
				failure = "quarantined exact garrison patrol adapter authority is unavailable " + operation.m_sOperationId;
				return false;
			}
			if (group)
			{
				if (!m_PhysicalWar)
				{
					failure = "quarantined exact garrison patrol PhysicalWar authority is unavailable " + operation.m_sOperationId;
					return false;
				}
				if (m_PhysicalWar.GetForceSpawnGroupRoot(group)
					|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0)
				{
					failure = "quarantined exact garrison patrol still owns PhysicalWar runtime entities " + operation.m_sOperationId;
					return false;
				}
			}
			else if (operationLive)
			{
				failure = "quarantined exact garrison patrol live authority has no group registry key " + operation.m_sOperationId;
				return false;
			}
			if (batchCleanupIncomplete)
			{
				failure = "quarantined exact garrison patrol queue cleanup is incomplete " + operation.m_sOperationId;
				return false;
			}
			if (!operationLive && !groupProcessEvidence)
				continue;

			NormalizeQuarantinedGarrisonPatrolProcessResidue(state, operation, batch, group);
		}
		return true;
	}

	protected bool QuarantinedGarrisonGroupClaimsOperation(
		HST_ActiveGroupState group,
		HST_OperationRecordState operation)
	{
		if (!group || !operation)
			return false;
		if (operation.m_sOperationId.IsEmpty()
			|| group.m_sOperationId != operation.m_sOperationId)
			return false;
		return (!operation.m_sGroupId.IsEmpty() && group.m_sGroupId == operation.m_sGroupId)
			|| (!operation.m_sManifestId.IsEmpty() && group.m_sManifestId == operation.m_sManifestId)
			|| (!operation.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == operation.m_sSpawnResultId)
			|| (!operation.m_sForceId.IsEmpty() && group.m_sForceId == operation.m_sForceId)
			|| (!operation.m_sProjectionId.IsEmpty() && group.m_sProjectionId == operation.m_sProjectionId);
	}

	protected bool QuarantinedGarrisonBatchClaimsOperation(
		HST_ForceSpawnResultState batch,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group)
	{
		if (!batch || !operation)
			return false;
		if (operation.m_sOperationId.IsEmpty()
			|| batch.m_sOperationId != operation.m_sOperationId)
			return false;
		return (!operation.m_sSpawnResultId.IsEmpty() && batch.m_sResultId == operation.m_sSpawnResultId)
			|| (!operation.m_sManifestId.IsEmpty() && batch.m_sManifestId == operation.m_sManifestId)
			|| (!operation.m_sForceId.IsEmpty() && batch.m_sForceId == operation.m_sForceId)
			|| (!operation.m_sProjectionId.IsEmpty() && batch.m_sProjectionId == operation.m_sProjectionId)
			|| (group && !group.m_sSpawnResultId.IsEmpty()
				&& batch.m_sResultId == group.m_sSpawnResultId);
	}

	protected bool QuarantinedGarrisonGroupHasProcessAuthority(HST_ActiveGroupState group)
	{
		return group && (group.m_bSpawnedEntity
			|| !group.m_sRuntimeEntityId.IsEmpty()
			|| group.m_iSpawnedAgentCount > 0
			|| group.m_iAssignedWaypointCount > 0);
	}

	protected bool IsQuarantinedGarrisonBatchCleanupComplete(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return true;
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED
			&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL)
			return false;
		if (!batch.m_sNativeGroupId.IsEmpty())
			return false;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot)
				continue;
			if (!slot.m_sEntityId.IsEmpty()
				|| !slot.m_sAssignedVehicleEntityId.IsEmpty()
				|| !slot.m_sNativeGroupId.IsEmpty())
				return false;
			if (slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
				|| slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING
				|| slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING)
				return false;
		}
		return true;
	}

	protected void NormalizeQuarantinedGarrisonPatrolProcessResidue(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!state || !operation)
			return;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			if (group && HasNonZeroPosition(group.m_vPosition))
				operation.m_vStrategicPosition = group.m_vPosition;
			operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
			operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
			operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
			operation.m_iLastProgressAtSecond = nowSecond;
			operation.m_iRevision++;
		}
		if (batch)
			batch.m_sNativeGroupId = "";
		if (!group)
			return;
		group.m_bSpawnedEntity = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
		group.m_sRuntimeStatus = "exact_garrison_patrol_quarantined";
		group.m_iLifecycleRevision++;
	}

	protected bool QuarantinedOperationClaimsOrder(
		HST_OperationRecordState operation,
		HST_EnemyOrderState order)
	{
		if (!operation || !order
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL
			|| operation.m_iContractVersion != HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION)
			return false;
		if (!order.m_sOrderId.IsEmpty() && operation.m_sEnemyOrderId == order.m_sOrderId)
			return true;
		if (!order.m_sOperationId.IsEmpty() && operation.m_sOperationId == order.m_sOperationId)
			return true;
		if (!order.m_sManifestId.IsEmpty() && operation.m_sManifestId == order.m_sManifestId)
			return true;
		if (!order.m_sSpawnResultId.IsEmpty() && operation.m_sSpawnResultId == order.m_sSpawnResultId)
			return true;
		return !order.m_sGroupId.IsEmpty() && operation.m_sGroupId == order.m_sGroupId;
	}

	protected bool QuarantinedGroupClaimsOrder(
		HST_ActiveGroupState group,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		if (!group || !order)
			return false;
		if (!order.m_sOrderId.IsEmpty() && group.m_sEnemyOrderId == order.m_sOrderId)
			return true;
		if (!order.m_sGroupId.IsEmpty() && group.m_sGroupId == order.m_sGroupId)
			return true;
		if (!order.m_sOperationId.IsEmpty() && group.m_sOperationId == order.m_sOperationId)
			return true;
		if (!order.m_sManifestId.IsEmpty() && group.m_sManifestId == order.m_sManifestId)
			return true;
		if (!order.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == order.m_sSpawnResultId)
			return true;
		return operation && !operation.m_sGroupId.IsEmpty()
			&& group.m_sGroupId == operation.m_sGroupId;
	}

	protected bool QuarantinedBatchClaimsOrder(
		HST_ForceSpawnResultState batch,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group)
	{
		if (!batch || !order)
			return false;
		if (!order.m_sOrderId.IsEmpty() && batch.m_sRequestId == order.m_sOrderId)
			return true;
		if (!order.m_sSpawnResultId.IsEmpty() && batch.m_sResultId == order.m_sSpawnResultId)
			return true;
		if (!order.m_sOperationId.IsEmpty() && batch.m_sOperationId == order.m_sOperationId)
			return true;
		if (!order.m_sManifestId.IsEmpty() && batch.m_sManifestId == order.m_sManifestId)
			return true;
		if (operation && !operation.m_sSpawnResultId.IsEmpty()
			&& batch.m_sResultId == operation.m_sSpawnResultId)
			return true;
		return group && !group.m_sSpawnResultId.IsEmpty()
			&& batch.m_sResultId == group.m_sSpawnResultId;
	}

	protected bool HasNonZeroPosition(vector position)
	{
		return Math.AbsFloat(position[0]) > 0.01
			|| Math.AbsFloat(position[1]) > 0.01
			|| Math.AbsFloat(position[2]) > 0.01;
	}

	protected bool IsOpenExactEnemyResponseOperation(HST_OperationRecordState operation)
	{
		if (!operation
			|| operation.m_eSettlementState
				!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult
				!= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
			return false;
		return (operation.m_eType
				== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF
				&& operation.m_iContractVersion
					== HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION)
			|| (operation.m_eType
					== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK
				&& operation.m_iContractVersion
					== HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION)
			|| (operation.m_eType
					== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_GARRISON_REBUILD
				&& operation.m_iContractVersion
					== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_CONTRACT_VERSION);
	}

	protected HST_OperationRecordState FindDematerializingExactEnemyResponse(
		HST_CampaignState state)
	{
		if (!state)
			return null;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (IsOpenExactEnemyResponseOperation(operation)
				&& operation.m_eMaterializationState
					== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
				return operation;
		}
		return null;
	}

	protected string ResolvePhysicalExactEnemyResponseRuntimeAuthority(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		out HST_EnemyOrderState order,
		out HST_ForceSpawnResultState batch,
		out HST_ActiveGroupState group)
	{
		order = null;
		batch = null;
		group = null;
		if (!IsOpenExactEnemyResponseOperation(operation))
			return "operation is not one open exact enemy response";
		if (operation.m_eType
			== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF)
		{
			if (!m_EnemyQRFOperations)
				return "defensive QRF persistence authority service is unavailable";
			return m_EnemyQRFOperations.ValidateOpenPersistenceRuntimeAuthority(
				state,
				operation,
				order,
				batch,
				group);
		}
		if (operation.m_eType
			== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK)
		{
			if (!m_EnemyCounterattackOperations)
				return "counterattack persistence authority service is unavailable";
			return m_EnemyCounterattackOperations.ValidateOpenPersistenceRuntimeAuthority(
				state,
				operation,
				order,
				batch,
				group);
		}
		if (!m_EnemyGarrisonRebuildOperations)
			return "garrison rebuild persistence authority service is unavailable";
		return m_EnemyGarrisonRebuildOperations.ValidateOpenPersistenceRuntimeAuthority(
			state,
			operation,
			order,
			batch,
			group);
	}

	protected bool PreflightPhysicalExactEnemyResponseGraphs(
		HST_CampaignState state,
		string context)
	{
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!IsOpenExactEnemyResponseOperation(operation)
				|| operation.m_eMaterializationState
					!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
				continue;
			if (operation.m_ePositionAuthority
				!= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
			{
				return DeferPhysicalExactEnemyResponseSnapshot(
					state,
					context,
					operation,
					"physical operation does not own live-position authority");
			}
			HST_EnemyOrderState order;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			string authorityFailure = ResolvePhysicalExactEnemyResponseRuntimeAuthority(
				state,
				operation,
				order,
				batch,
				group);
			if (!authorityFailure.IsEmpty())
			{
				return DeferPhysicalExactEnemyResponseSnapshot(
					state,
					context,
					operation,
					authorityFailure);
			}
		}
		return true;
	}

	protected bool ValidatePhysicalExactEnemyResponseSnapshots(
		HST_CampaignState state,
		string context,
		array<ref HST_ExactEnemyResponsePersistencePositionPlan> positionPlans)
	{
		positionPlans.Clear();
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!IsOpenExactEnemyResponseOperation(operation)
				|| operation.m_eMaterializationState
					!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
				continue;

			HST_EnemyOrderState order;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			string authorityFailure = ResolvePhysicalExactEnemyResponseRuntimeAuthority(
				state,
				operation,
				order,
				batch,
				group);
			if (!authorityFailure.IsEmpty())
			{
				return DeferPhysicalExactEnemyResponseSnapshot(
					state,
					context,
					operation,
					authorityFailure);
			}
			if (!batch
				|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
				|| batch.m_bStrategicProjectionHeld
				|| batch.m_iSuccessfulHandoffCount <= 0)
			{
				return DeferPhysicalExactEnemyResponseSnapshot(
					state,
					context,
					operation,
					"exact infantry batch is not one successful non-held projection");
			}
			if (m_ForceSpawnQueue.CountDurableLivingMemberSlots(batch) <= 0)
			{
				return DeferPhysicalExactEnemyResponseSnapshot(
					state,
					context,
					operation,
					"durable living roster is unavailable");
			}

			string bindingEvidence;
			if (!m_ForceSpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
				state,
				batch,
				m_ForceSpawnQueue,
				m_PhysicalWar,
				bindingEvidence))
			{
				return DeferPhysicalExactEnemyResponseSnapshot(
					state,
					context,
					operation,
					bindingEvidence);
			}

			vector livePosition;
			string livePositionEvidence;
			if (!m_PhysicalWar.TryResolveExactEnemyResponseLivePosition(
				state,
				group,
				livePosition,
				livePositionEvidence))
			{
				return DeferPhysicalExactEnemyResponseSnapshot(
					state,
					context,
					operation,
					livePositionEvidence);
			}

			HST_ExactEnemyResponsePersistencePositionPlan plan
				= new HST_ExactEnemyResponsePersistencePositionPlan();
			plan.m_Operation = operation;
			plan.m_Order = order;
			plan.m_Group = group;
			plan.m_vLivePosition = livePosition;
			plan.m_vPreviousGroupPosition = group.m_vPosition;
			plan.m_vPreviousOperationPosition = operation.m_vStrategicPosition;
			plan.m_iPreviousOperationRevision = operation.m_iRevision;
			plan.m_iPreviousOperationLastProgressAtSecond
				= operation.m_iLastProgressAtSecond;
			positionPlans.Insert(plan);
		}
		return true;
	}

	protected string RefreshExactEnemyResponsePersistencePosition(
		HST_CampaignState state,
		HST_ExactEnemyResponsePersistencePositionPlan plan)
	{
		if (!plan || !plan.m_Operation || !plan.m_Order || !plan.m_Group)
			return "exact enemy response position plan is incomplete";
		if (plan.m_Operation.m_eType
			== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF)
		{
			if (!m_EnemyQRFOperations)
				return "defensive QRF persistence authority service is unavailable";
			return m_EnemyQRFOperations.RefreshPhysicalPersistencePosition(
				state,
				plan.m_Order,
				plan.m_Group);
		}
		if (plan.m_Operation.m_eType
			== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK)
		{
			if (!m_EnemyCounterattackOperations)
				return "counterattack persistence authority service is unavailable";
			return m_EnemyCounterattackOperations.RefreshPhysicalPersistencePosition(
				state,
				plan.m_Order,
				plan.m_Group);
		}
		if (!m_EnemyGarrisonRebuildOperations)
			return "garrison rebuild persistence authority service is unavailable";
		return m_EnemyGarrisonRebuildOperations.RefreshPhysicalPersistencePosition(
			state,
			plan.m_Order,
			plan.m_Group);
	}

	protected void RollBackExactEnemyResponsePersistencePositions(
		array<ref HST_ExactEnemyResponsePersistencePositionPlan> positionPlans,
		int lastAppliedIndex)
	{
		for (int index = lastAppliedIndex; index >= 0; index--)
		{
			HST_ExactEnemyResponsePersistencePositionPlan plan = positionPlans[index];
			if (!plan)
				continue;
			if (plan.m_Group && m_PhysicalWar)
			{
				m_PhysicalWar.ApplyExactEnemyResponsePersistencePosition(
					plan.m_Group,
					plan.m_vPreviousGroupPosition);
			}
			if (!plan.m_Operation)
				continue;
			plan.m_Operation.m_vStrategicPosition = plan.m_vPreviousOperationPosition;
			plan.m_Operation.m_iRevision = plan.m_iPreviousOperationRevision;
			plan.m_Operation.m_iLastProgressAtSecond
				= plan.m_iPreviousOperationLastProgressAtSecond;
		}
	}

	protected bool ApplyPhysicalExactEnemyResponseSnapshots(
		HST_CampaignState state,
		string context,
		array<ref HST_ExactEnemyResponsePersistencePositionPlan> positionPlans)
	{
		for (int index = 0; index < positionPlans.Count(); index++)
		{
			HST_ExactEnemyResponsePersistencePositionPlan plan = positionPlans[index];
			if (!plan || !plan.m_Group || !plan.m_Operation)
			{
				RollBackExactEnemyResponsePersistencePositions(positionPlans, index - 1);
				return DeferPhysicalExactEnemyResponseSnapshot(
					state,
					context,
					null,
					"exact enemy response position plan is incomplete");
			}
			m_PhysicalWar.ApplyExactEnemyResponsePersistencePosition(
				plan.m_Group,
				plan.m_vLivePosition);
			string positionRefreshFailure
				= RefreshExactEnemyResponsePersistencePosition(state, plan);
			if (!positionRefreshFailure.IsEmpty())
			{
				RollBackExactEnemyResponsePersistencePositions(positionPlans, index);
				return DeferPhysicalExactEnemyResponseSnapshot(
					state,
					context,
					plan.m_Operation,
					positionRefreshFailure);
			}
		}
		return true;
	}

	protected bool DeferPhysicalExactEnemyResponseSnapshot(
		HST_CampaignState state,
		string context,
		HST_OperationRecordState operation,
		string evidence)
	{
		string family = "unknown";
		string operationId = "unavailable";
		if (operation)
		{
			operationId = operation.m_sOperationId;
			if (operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF)
				family = "defensive QRF";
			else if (operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK)
				family = "counterattack";
			else if (operation.m_eType
				== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_GARRISON_REBUILD)
				family = "garrison rebuild";
		}
		state.m_sLastPersistenceStatus = string.Format(
			"checkpoint deferred: exact enemy response live snapshot failed during %1 | %2 operation %3 | %4",
			BoundExactEnemyResponsePersistenceStatusPart(context, 72),
			BoundExactEnemyResponsePersistenceStatusPart(family, 32),
			BoundExactEnemyResponsePersistenceStatusPart(operationId, 72),
			BoundExactEnemyResponsePersistenceStatusPart(evidence, 192));
		Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
		return false;
	}

	protected string BoundExactEnemyResponsePersistenceStatusPart(string value, int maxCharacters)
	{
		if (value.IsEmpty())
			return "unavailable";
		maxCharacters = Math.Max(4, maxCharacters);
		if (value.Length() <= maxCharacters)
			return value;
		return value.Substring(0, maxCharacters - 3) + "...";
	}

	protected bool ValidatePhysicalPlayerSupportBindings(HST_CampaignState state, string context)
	{
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || !HST_OperationService.IsExactPlayerSupportOperationType(operation.m_eType)
				|| operation.m_iContractVersion == 0
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
				continue;
			if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
				continue;

			HST_SupportRequestState request = FindUniquePlayerSupportPersistenceRequest(state, operation.m_sSupportRequestId);
			if (!request || CountPlayerSupportPersistenceRequests(state, operation) != 1)
			{
				return DeferPhysicalPlayerSupportBinding(
					state,
					context,
					operation.m_sOperationId,
					"reciprocal support request is missing or ambiguous");
			}
			if (!PlayerSupportPersistenceRequestLinksMatch(operation, request))
			{
				return DeferPhysicalPlayerSupportBinding(
					state,
					context,
					operation.m_sOperationId,
					"support request backlinks conflict");
			}

			HST_ForceSpawnResultState batch = FindUniquePlayerSupportPersistenceBatch(state, operation.m_sSpawnResultId);
			if (!batch || CountPlayerSupportPersistenceBatches(state, operation, request, batch) != 1)
			{
				return DeferPhysicalPlayerSupportBinding(
					state,
					context,
					operation.m_sOperationId,
					"reciprocal exact-infantry batch is missing or ambiguous");
			}
			if (!PlayerSupportPersistenceBatchLinksMatch(operation, request, batch))
			{
				return DeferPhysicalPlayerSupportBinding(
					state,
					context,
					operation.m_sOperationId,
					"exact-infantry batch backlinks conflict");
			}

			HST_ActiveGroupState group = FindUniquePlayerSupportPersistenceGroup(state, operation.m_sGroupId);
			if (!group || CountPlayerSupportPersistenceGroups(state, operation, request, group) != 1)
			{
				return DeferPhysicalPlayerSupportBinding(
					state,
					context,
					operation.m_sOperationId,
					"reciprocal active group is missing or ambiguous");
			}
			if (!PlayerSupportPersistenceGroupLinksMatch(operation, request, batch, group))
			{
				return DeferPhysicalPlayerSupportBinding(
					state,
					context,
					operation.m_sOperationId,
					"active-group backlinks conflict");
			}

			if (m_ForceSpawnQueue.CountDurableLivingMemberSlots(batch) <= 0)
				continue;

			string bindingEvidence;
			if (!m_ForceSpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
				state,
				batch,
				m_ForceSpawnQueue,
				m_PhysicalWar,
				bindingEvidence))
			{
				return DeferPhysicalPlayerSupportBinding(
					state,
					context,
					operation.m_sOperationId,
					bindingEvidence);
			}

			string livePositionEvidence;
			if (!m_PhysicalWar.TryRefreshActiveSupportGroupLivePosition(group, livePositionEvidence))
			{
				return DeferPhysicalPlayerSupportBinding(
					state,
					context,
					operation.m_sOperationId,
					livePositionEvidence);
			}
		}
		return true;
	}

	protected HST_SupportRequestState FindUniquePlayerSupportPersistenceRequest(
		HST_CampaignState state,
		string requestId)
	{
		HST_SupportRequestState match;
		if (!state || requestId.IsEmpty())
			return null;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || request.m_sRequestId != requestId)
				continue;
			if (match)
				return null;
			match = request;
		}
		return match;
	}

	protected HST_ForceSpawnResultState FindUniquePlayerSupportPersistenceBatch(
		HST_CampaignState state,
		string resultId)
	{
		HST_ForceSpawnResultState match;
		if (!state || resultId.IsEmpty())
			return null;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (!batch || batch.m_sResultId != resultId)
				continue;
			if (match)
				return null;
			match = batch;
		}
		return match;
	}

	protected HST_ActiveGroupState FindUniquePlayerSupportPersistenceGroup(
		HST_CampaignState state,
		string groupId)
	{
		HST_ActiveGroupState match;
		if (!state || groupId.IsEmpty())
			return null;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (!group || group.m_sGroupId != groupId)
				continue;
			if (match)
				return null;
			match = group;
		}
		return match;
	}

	protected int CountPlayerSupportPersistenceRequests(
		HST_CampaignState state,
		HST_OperationRecordState operation)
	{
		int count;
		if (!state || !operation)
			return count;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request)
				continue;
			bool matches = PersistenceIdentityMatches(request.m_sRequestId, operation.m_sSupportRequestId);
			matches = matches || PersistenceIdentityMatches(request.m_sOperationId, operation.m_sOperationId);
			matches = matches || PersistenceIdentityMatches(request.m_sQuoteId, operation.m_sQuoteId);
			matches = matches || PersistenceIdentityMatches(request.m_sManifestId, operation.m_sManifestId);
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountPlayerSupportPersistenceBatches(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_SupportRequestState request,
		HST_ForceSpawnResultState expected)
	{
		int count;
		if (!state || !operation || !request || !expected)
			return count;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (!batch)
				continue;
			bool matches = PersistenceIdentityMatches(batch.m_sResultId, expected.m_sResultId);
			matches = matches || PersistenceIdentityMatches(batch.m_sRequestId, request.m_sRequestId);
			matches = matches || PersistenceIdentityMatches(batch.m_sManifestId, operation.m_sManifestId);
			matches = matches || PersistenceIdentityMatches(batch.m_sOperationId, operation.m_sOperationId);
			matches = matches || PersistenceIdentityMatches(batch.m_sProjectionId, expected.m_sProjectionId);
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountPlayerSupportPersistenceGroups(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_SupportRequestState request,
		HST_ActiveGroupState expected)
	{
		int count;
		if (!state || !operation || !request || !expected)
			return count;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (!group)
				continue;
			bool matches = PersistenceIdentityMatches(group.m_sGroupId, expected.m_sGroupId);
			matches = matches || PersistenceIdentityMatches(group.m_sSupportRequestId, request.m_sRequestId);
			matches = matches || PersistenceIdentityMatches(group.m_sManifestId, operation.m_sManifestId);
			matches = matches || PersistenceIdentityMatches(group.m_sOperationId, operation.m_sOperationId);
			matches = matches || PersistenceIdentityMatches(group.m_sProjectionId, expected.m_sProjectionId);
			if (matches)
				count++;
		}
		return count;
	}

	protected bool PlayerSupportPersistenceRequestLinksMatch(
		HST_OperationRecordState operation,
		HST_SupportRequestState request)
	{
		if (!operation || !request || !HST_OperationService.IsExactPlayerSupportType(request.m_eType))
			return false;
		bool linksMatch = request.m_iOperationContractVersion != 0;
		linksMatch = linksMatch && request.m_iOperationContractVersion == operation.m_iContractVersion;
		linksMatch = linksMatch && HST_OperationService.ResolveExactPlayerSupportOperationType(request.m_eType) == operation.m_eType;
		linksMatch = linksMatch && request.m_sRequestId == operation.m_sSupportRequestId;
		linksMatch = linksMatch && request.m_sOperationId == operation.m_sOperationId;
		linksMatch = linksMatch && request.m_sQuoteId == operation.m_sQuoteId;
		linksMatch = linksMatch && request.m_sManifestId == operation.m_sManifestId;
		linksMatch = linksMatch && request.m_sSpawnResultId == operation.m_sSpawnResultId;
		linksMatch = linksMatch && request.m_sGroupId == operation.m_sGroupId;
		return linksMatch;
	}

	protected bool PlayerSupportPersistenceBatchLinksMatch(
		HST_OperationRecordState operation,
		HST_SupportRequestState request,
		HST_ForceSpawnResultState batch)
	{
		if (!operation || !request || !batch)
			return false;
		bool linksMatch = batch.m_sResultId == operation.m_sSpawnResultId;
		linksMatch = linksMatch && batch.m_sRequestId == request.m_sRequestId;
		linksMatch = linksMatch && batch.m_sManifestId == operation.m_sManifestId;
		linksMatch = linksMatch && batch.m_sOperationId == operation.m_sOperationId;
		linksMatch = linksMatch && batch.m_sForceId == operation.m_sForceId;
		linksMatch = linksMatch && batch.m_sProjectionId == operation.m_sProjectionId;
		return linksMatch;
	}

	protected bool PlayerSupportPersistenceGroupLinksMatch(
		HST_OperationRecordState operation,
		HST_SupportRequestState request,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!operation || !request || !batch || !group)
			return false;
		bool linksMatch = group.m_sGroupId == operation.m_sGroupId;
		linksMatch = linksMatch && group.m_sSupportRequestId == request.m_sRequestId;
		linksMatch = linksMatch && group.m_sManifestId == operation.m_sManifestId;
		linksMatch = linksMatch && group.m_sOperationId == operation.m_sOperationId;
		linksMatch = linksMatch && group.m_sSpawnResultId == batch.m_sResultId;
		linksMatch = linksMatch && group.m_sForceId == batch.m_sForceId;
		linksMatch = linksMatch && group.m_sProjectionId == batch.m_sProjectionId;
		return linksMatch;
	}

	protected bool PersistenceIdentityMatches(string left, string right)
	{
		return !left.IsEmpty() && left == right;
	}

	protected bool DeferPhysicalPlayerSupportBinding(
		HST_CampaignState state,
		string context,
		string operationId,
		string evidence)
	{
		state.m_sLastPersistenceStatus = string.Format(
			"checkpoint deferred: exact player support live roster failed during %1 | operation %2 | %3",
			BoundPlayerSupportPersistenceStatusPart(context, 72),
			BoundPlayerSupportPersistenceStatusPart(operationId, 72),
			BoundPlayerSupportPersistenceStatusPart(evidence, 192));
		Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
		return false;
	}

	protected string BoundPlayerSupportPersistenceStatusPart(string value, int maxCharacters)
	{
		if (value.IsEmpty())
			return "unavailable";
		maxCharacters = Math.Max(4, maxCharacters);
		if (value.Length() <= maxCharacters)
			return value;
		return value.Substring(0, maxCharacters - 3) + "...";
	}

	protected bool ValidatePhysicalEnemyPatrolSnapshots(HST_CampaignState state, string context)
	{
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL
				|| operation.m_iContractVersion != HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				|| (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
					&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING))
				continue;
			HST_ActiveGroupState group = state.FindActiveGroup(operation.m_sGroupId);
			HST_ForceSpawnResultState batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
			if (!group || !batch || m_ForceSpawnQueue.CountDurableLivingMemberSlots(batch) <= 0)
				return DeferPhysicalEnemyPatrolSnapshot(state, context, operation.m_sOperationId, "durable living roster is unavailable");
			string bindingEvidence;
			if (!m_ForceSpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
				state,
				batch,
				m_ForceSpawnQueue,
				m_PhysicalWar,
				bindingEvidence))
			{
				return DeferPhysicalEnemyPatrolSnapshot(state, context, operation.m_sOperationId, bindingEvidence);
			}
			vector livePosition;
			string liveEvidence;
			if (!m_PhysicalWar.TryResolveExactEnemyPatrolLivePosition(state, group, livePosition, liveEvidence))
				return DeferPhysicalEnemyPatrolSnapshot(state, context, operation.m_sOperationId, liveEvidence);
		}
		return true;
	}

	protected bool DeferPhysicalEnemyPatrolSnapshot(
		HST_CampaignState state,
		string context,
		string operationId,
		string evidence)
	{
		state.m_sLastPersistenceStatus = string.Format(
			"checkpoint deferred: exact patrol live snapshot failed during %1 | operation %2 | %3",
			context,
			operationId,
			evidence);
		Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
		return false;
	}

	protected bool ValidatePhysicalGarrisonPatrolSnapshots(HST_CampaignState state, string context)
	{
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL
				|| operation.m_iContractVersion != HST_GarrisonPatrolOperationService.EXACT_CONTRACT_VERSION
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				|| (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
					&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING))
				continue;
			HST_ActiveGroupState group = state.FindActiveGroup(operation.m_sGroupId);
			HST_ForceSpawnResultState batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
			if (!group || !batch || m_ForceSpawnQueue.CountDurableLivingMemberSlots(batch) <= 0)
				return DeferPhysicalGarrisonPatrolSnapshot(state, context, operation.m_sOperationId, "durable living roster is unavailable");
			string bindingEvidence;
			if (!m_ForceSpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
				state,
				batch,
				m_ForceSpawnQueue,
				m_PhysicalWar,
				bindingEvidence))
			{
				return DeferPhysicalGarrisonPatrolSnapshot(state, context, operation.m_sOperationId, bindingEvidence);
			}
			vector livePosition;
			string liveEvidence;
			if (!m_PhysicalWar.TryResolveExactGarrisonPatrolLivePosition(state, group, livePosition, liveEvidence))
				return DeferPhysicalGarrisonPatrolSnapshot(state, context, operation.m_sOperationId, liveEvidence);
		}
		return true;
	}

	protected bool DeferPhysicalGarrisonPatrolSnapshot(
		HST_CampaignState state,
		string context,
		string operationId,
		string evidence)
	{
		state.m_sLastPersistenceStatus = string.Format(
			"checkpoint deferred: exact garrison patrol live snapshot failed during %1 | operation %2 | %3",
			context,
			operationId,
			evidence);
		Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
		return false;
	}

	protected bool ValidatePhysicalMissionGuardSnapshots(HST_CampaignState state, string context)
	{
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
				|| !HST_MissionGuardOperationService.IsSupportedExactContractVersion(
					operation.m_iContractVersion)
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				|| (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
					&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING))
				continue;
			HST_ActiveMissionState mission = state.FindActiveMission(operation.m_sMissionInstanceId);
			HST_ActiveGroupState group = state.FindActiveGroup(operation.m_sGroupId);
			HST_ForceSpawnResultState batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
			if (!HST_MissionGuardOperationService.IsExactMission(mission)
				|| !HST_MissionGuardOperationService.IsExactMissionGuardGroup(state, group)
				|| !batch || m_ForceSpawnQueue.CountDurableLivingMemberSlots(batch) <= 0)
			{
				return DeferPhysicalMissionGuardSnapshot(
					state,
					context,
					operation.m_sOperationId,
					"durable reciprocal mission-guard roster is unavailable");
			}
			string bindingEvidence;
			if (!m_ForceSpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
				state,
				batch,
				m_ForceSpawnQueue,
				m_PhysicalWar,
				bindingEvidence))
			{
				return DeferPhysicalMissionGuardSnapshot(
					state,
					context,
					operation.m_sOperationId,
					bindingEvidence);
			}
			vector livePosition;
			string liveEvidence;
			if (!m_PhysicalWar.TryResolveExactMissionGuardLivePosition(
				state,
				group,
				livePosition,
				liveEvidence))
			{
				return DeferPhysicalMissionGuardSnapshot(
					state,
					context,
					operation.m_sOperationId,
					liveEvidence);
			}
		}
		return true;
	}

	protected bool DeferPhysicalMissionGuardSnapshot(
		HST_CampaignState state,
		string context,
		string operationId,
		string evidence)
	{
		state.m_sLastPersistenceStatus = string.Format(
			"checkpoint deferred: exact mission guard live snapshot failed during %1 | operation %2 | %3",
			context,
			operationId,
			evidence);
		Print("Partisan persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
		return false;
	}

	HST_PersistenceSourceResolution ResolveCampaignStateSource(
		HST_CampaignState fallbackState)
	{
		HST_PersistenceSourceResolution result
			= new HST_PersistenceSourceResolution();
		if (!fallbackState)
		{
			result.m_eSource
				= HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_FATAL;
			result.m_sEvidence = "campaign source resolution has no fallback state";
			m_LastSourceResolution = result;
			return result;
		}

		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (persistence)
		{
			result.m_bPersistenceSystemAvailable = true;
			EPersistenceSystemState persistenceState = persistence.GetState();
			if (persistenceState == EPersistenceSystemState.INIT
				|| persistenceState == EPersistenceSystemState.SETUP)
			{
				result.m_sEvidence = string.Format(
					"native persistence is not active yet | state %1",
					persistenceState);
				m_LastSourceResolution = result;
				return result;
			}
			if (persistenceState == EPersistenceSystemState.FAILURE
				|| persistenceState == EPersistenceSystemState.SHUTDOWN)
			{
				result.m_eSource
					= HST_ECampaignPersistenceSource
						.HST_PERSISTENCE_SOURCE_FATAL;
				result.m_sEvidence = string.Format(
					"native persistence entered terminal state %1",
					persistenceState);
				m_LastSourceResolution = result;
				return result;
			}
			if (persistenceState != EPersistenceSystemState.ACTIVE)
			{
				result.m_eSource
					= HST_ECampaignPersistenceSource
						.HST_PERSISTENCE_SOURCE_FATAL;
				result.m_sEvidence = string.Format(
					"native persistence state %1 is unsupported",
					persistenceState);
				m_LastSourceResolution = result;
				return result;
			}

			result.m_bPersistenceSystemLoadedData = persistence.WasDataLoaded();
			m_NativeCampaignState = ResolveNativeCampaignState(persistence);
			if (!m_NativeCampaignState)
			{
				result.m_eSource
					= HST_ECampaignPersistenceSource
						.HST_PERSISTENCE_SOURCE_FATAL;
				result.m_sEvidence
					= "native persistence is active without the campaign state proxy";
				m_LastSourceResolution = result;
				return result;
			}

			result.m_bNativeRecordPresent
				= m_NativeCampaignState.HasLoadedRecord();
			result.m_bNativeRecordValid
				= m_NativeCampaignState.IsLoadedRecordValid();
			if (result.m_bNativeRecordPresent
				&& !result.m_bNativeRecordValid)
			{
				result.m_eSource
					= HST_ECampaignPersistenceSource
						.HST_PERSISTENCE_SOURCE_FATAL;
				result.m_sEvidence
					= "native campaign record is present but invalid: "
						+ m_NativeCampaignState.GetValidationFailure();
				m_LastSourceResolution = result;
				return result;
			}
			if (result.m_bNativeRecordValid)
			{
				HST_CampaignSaveData nativeSave
					= m_NativeCampaignState.GetSnapshot();
				result.m_sNativeSnapshotFingerprint
					= m_NativeCampaignState.GetSnapshotFingerprint();
				result.m_sSelectedSnapshotFingerprint
					= result.m_sNativeSnapshotFingerprint;
				if (!nativeSave
					|| result.m_sNativeSnapshotFingerprint.IsEmpty()
					|| !ApplyRestoredCampaignState(fallbackState, nativeSave))
				{
					result.m_eSource
						= HST_ECampaignPersistenceSource
							.HST_PERSISTENCE_SOURCE_FATAL;
					result.m_sEvidence
						= "native campaign record could not be applied";
					m_LastSourceResolution = result;
					return result;
				}

				result.m_eSource
					= HST_ECampaignPersistenceSource
						.HST_PERSISTENCE_SOURCE_NATIVE;
				result.m_State = fallbackState;
				result.m_sEvidence
					= "valid native campaign record selected before profile fallback";
				m_LastSourceResolution = result;
				return result;
			}
		}

		string fallbackSourcePath = HST_ProfilePathService.ResolveReadableFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
			HST_ProfilePathService.LEGACY_CAMPAIGN_SAVE_FILE);
		result.m_bProfileFallbackPresent
			= FileIO.FileExists(fallbackSourcePath);
		HST_CampaignSaveData restoredSave;
		if (result.m_bProfileFallbackPresent)
			restoredSave = LoadProfileFallback();
		if (result.m_bProfileFallbackPresent && !restoredSave)
		{
			result.m_eSource
				= HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_FATAL;
			result.m_sEvidence
				= "profile fallback is present but could not be read";
			m_LastSourceResolution = result;
			return result;
		}

		if (restoredSave)
		{
			result.m_bProfileFallbackRead = true;
			result.m_sProfileFallbackSnapshotFingerprint
				= HST_CampaignPersistentState.BuildSnapshotFingerprint(restoredSave);
			result.m_sSelectedSnapshotFingerprint
				= result.m_sProfileFallbackSnapshotFingerprint;
			if (result.m_sProfileFallbackSnapshotFingerprint.IsEmpty()
				|| !ApplyRestoredCampaignState(fallbackState, restoredSave))
			{
				result.m_eSource
					= HST_ECampaignPersistenceSource
						.HST_PERSISTENCE_SOURCE_FATAL;
				result.m_sEvidence
					= "profile fallback could not be validated or applied";
				m_LastSourceResolution = result;
				return result;
			}

			result.m_eSource
				= HST_ECampaignPersistenceSource
					.HST_PERSISTENCE_SOURCE_PROFILE_FALLBACK;
			result.m_State = fallbackState;
			result.m_sEvidence
				= "profile fallback selected because no native campaign record exists";
			m_LastSourceResolution = result;
			return result;
		}

		// A loaded engine save with no valid HST row is not a fresh campaign.
		// Older builds can still migrate through a valid profile fallback above,
		// but absent both records we must fail closed instead of resetting the
		// campaign while presenting that reset as a successful load.
		if (result.m_bPersistenceSystemLoadedData)
		{
			result.m_eSource
				= HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_FATAL;
			result.m_sEvidence
				= "loaded native save is missing the campaign state record and no valid profile fallback exists";
			m_LastSourceResolution = result;
			return result;
		}

		fallbackState.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		fallbackState.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		fallbackState.m_iPersistenceRestoreSequence = 0;
		fallbackState.m_iForceSpawnQueueReconciledRestoreSequence = 0;
		fallbackState.m_bRestoredFromPersistence = false;
		fallbackState.m_sLastPersistenceStatus = "new campaign source selected";
		result.m_eSource
			= HST_ECampaignPersistenceSource
				.HST_PERSISTENCE_SOURCE_NEW_CAMPAIGN;
		result.m_State = fallbackState;
		result.m_sEvidence
			= "new campaign selected because native and profile sources are absent";
		m_LastSourceResolution = result;
		return result;
	}

	HST_CampaignState RestoreOrCreateCampaignState(HST_CampaignState fallbackState)
	{
		HST_PersistenceSourceResolution result
			= ResolveCampaignStateSource(fallbackState);
		if (!result || !result.HasSelectedState())
			return null;
		return result.m_State;
	}

	bool ApplyRestoredCampaignState(HST_CampaignState targetState, HST_CampaignSaveData restoredSave)
	{
		if (!targetState || !restoredSave)
			return false;

		restoredSave.MigrateToCurrentSchema();
		restoredSave.ApplyTo(targetState, false);
		targetState.m_iPersistenceRestoreSequence = Math.Max(0, targetState.m_iPersistenceRestoreSequence) + 1;
		targetState.m_bRestoredFromPersistence = true;
		targetState.m_iLastRestoreSecond = targetState.m_iElapsedSeconds;
		targetState.m_sLastPersistenceStatus = string.Format("restored schema %1 -> %2", targetState.m_iLastLoadedSchemaVersion, targetState.m_iSchemaVersion);
		m_RestoredCampaignSave = restoredSave;
		return true;
	}

	HST_PersistenceSourceResolution GetLastSourceResolution()
	{
		return m_LastSourceResolution;
	}

	bool IsNativeCampaignStateTracked(out string evidence)
	{
		evidence = "native campaign state is unavailable";
		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (!persistence)
			return false;
		if (persistence.GetState() != EPersistenceSystemState.ACTIVE)
		{
			evidence = string.Format(
				"native persistence state is %1",
				persistence.GetState());
			return false;
		}
		HST_CampaignPersistentState nativeState
			= ResolveNativeCampaignState(persistence);
		if (!nativeState)
			return false;
		bool tracked = persistence.IsTracked(nativeState);
		bool configured = persistence.GetConfig(nativeState) != null;
		evidence = string.Format(
			"native campaign proxy tracked/configured %1/%2",
			tracked,
			configured);
		return tracked && configured;
	}

	HST_CampaignSaveData GetLastCapturedSave()
	{
		return m_LastCapturedSave;
	}

	string BuildPersistenceReport(HST_CampaignState state)
	{
		if (!state)
			return "Partisan persistence | state not ready";

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

		return string.Format("Partisan persistence | %1 | last save %2 | last restore %3 | %4\n%5\n%6\n%7\n%8", state.m_sLastPersistenceStatus, state.m_iLastSaveSecond, state.m_iLastRestoreSecond, saveManagerStatus, persistenceSystemStatus, profileFallbackStatus, tracked, restored);
	}

	protected HST_CampaignSaveData GetRestoredCampaignSaveData()
	{
		if (m_RestoredCampaignSave)
			return m_RestoredCampaignSave;

		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (!persistence || !persistence.WasDataLoaded())
			return null;

		HST_CampaignPersistentState persistentState
			= ResolveNativeCampaignState(persistence);
		if (!persistentState || !persistentState.IsLoadedRecordValid())
			return null;
		m_RestoredCampaignSave = persistentState.GetSnapshot();
		return m_RestoredCampaignSave;
	}

	protected bool CanRequestSavePoint(SaveGameManager saveManager)
	{
		return saveManager && saveManager.IsSavingEnabled()
			&& saveManager.IsSavingAllowed() && !saveManager.IsBusy();
	}

	protected bool TrackCampaignSaveData(HST_CampaignSaveData saveData)
	{
		if (!saveData)
			return false;

		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (!persistence
			|| persistence.GetState() != EPersistenceSystemState.ACTIVE)
			return false;

		m_NativeCampaignState = ResolveNativeCampaignState(persistence);
		if (!m_NativeCampaignState)
			return false;
		m_NativeCampaignState.SetSnapshotForSave(saveData);
		if (!persistence.IsTracked(m_NativeCampaignState)
			&& !persistence.StartTracking(m_NativeCampaignState, false))
			return false;
		return persistence.IsTracked(m_NativeCampaignState)
			&& persistence.GetConfig(m_NativeCampaignState) != null;
	}

	protected HST_CampaignPersistentState ResolveNativeCampaignState(
		PersistenceSystem persistence)
	{
		if (!persistence)
			return null;
		Managed persistentState
			= persistence.GetPersistentState(HST_CampaignPersistentState);
		return HST_CampaignPersistentState.Cast(persistentState);
	}

	protected bool FlushTrackedCampaignState(ESaveGameType saveType)
	{
		if (!m_TrackedCampaignSave)
			return false;

		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (!persistence)
			return false;

		if (!TrackCampaignSaveData(m_TrackedCampaignSave)
			|| !m_NativeCampaignState)
			return false;
		return persistence.Save(m_NativeCampaignState, saveType);
	}

	protected bool SaveProfileFallback(HST_CampaignSaveData saveData)
	{
		if (!saveData)
		{
			m_sProfileFallbackStatus = "profile fallback save skipped: no tracked save";
			return false;
		}

		HST_ProfilePathService.EnsureProfileDirectory();
		JsonSaveContext context = new JsonSaveContext();
		if (context.WriteValue("", saveData) && context.SaveToFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE))
		{
			m_bProfileFallbackSaved = true;
			m_sProfileFallbackStatus = string.Format("profile fallback saved schema %1 to %2", saveData.m_iSchemaVersion, HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
			Print("Partisan persistence | " + m_sProfileFallbackStatus);
			return true;
		}

		m_sProfileFallbackStatus = string.Format("profile fallback save failed at %1", HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		Print("Partisan persistence | " + m_sProfileFallbackStatus, LogLevel.WARNING);
		return false;
	}

	protected HST_CampaignSaveData LoadProfileFallback()
	{
		string sourcePath = HST_ProfilePathService.ResolveReadableFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
			HST_ProfilePathService.LEGACY_CAMPAIGN_SAVE_FILE);
		if (!FileIO.FileExists(sourcePath))
		{
			m_sProfileFallbackStatus = string.Format("profile fallback missing at %1", HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
			return null;
		}

		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(sourcePath))
		{
			m_sProfileFallbackStatus = string.Format("profile fallback load failed at %1", sourcePath);
			Print("Partisan persistence | " + m_sProfileFallbackStatus, LogLevel.WARNING);
			return null;
		}

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		if (!context.ReadValue("", saveData))
		{
			m_sProfileFallbackStatus = string.Format("profile fallback read failed at %1", sourcePath);
			Print("Partisan persistence | " + m_sProfileFallbackStatus, LogLevel.WARNING);
			return null;
		}

		m_bProfileFallbackLoaded = true;
		if (HST_ProfilePathService.IsLegacyPath(sourcePath))
		{
			saveData.MigrateToCurrentSchema();
			bool adopted = SaveProfileFallback(saveData);
			m_sProfileFallbackStatus = string.Format(
				"profile fallback loaded schema %1 from legacy source %2 | adopted to %3: %4",
				saveData.m_iSchemaVersion,
				sourcePath,
				HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
				adopted);
		}
		else
		{
			m_sProfileFallbackStatus = string.Format("profile fallback loaded schema %1 from %2", saveData.m_iSchemaVersion, sourcePath);
		}
		Print("Partisan persistence | " + m_sProfileFallbackStatus);
		return saveData;
	}

	protected string BuildPersistenceSystemStatus()
	{
		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (!persistence)
			return "PersistenceSystem unavailable";

		bool tracked;
		HST_CampaignPersistentState nativeState
			= ResolveNativeCampaignState(persistence);
		if (nativeState)
			tracked = persistence.IsTracked(nativeState);

		return string.Format("PersistenceSystem | loaded %1 | state %2 | tracked %3", persistence.WasDataLoaded(), persistence.GetState(), tracked);
	}

	protected string BuildProfileFallbackStatus()
	{
		bool exists = FileIO.FileExists(HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		bool legacyExists = FileIO.FileExists(HST_ProfilePathService.LEGACY_CAMPAIGN_SAVE_FILE);
		return string.Format("profile fallback | canonical exists %1 | legacy available %2 | saved %3 | loaded %4 | %5", exists, legacyExists, m_bProfileFallbackSaved, m_bProfileFallbackLoaded, m_sProfileFallbackStatus);
	}
}
