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
	bool m_bPreparedDetachedSnapshotAccepted;
	string m_sPreparedSnapshotFingerprint;
	int m_iPreparedSnapshotCheckpointSequence;
	int m_iPreparedSnapshotRestoreSequence;
	bool m_bIsolatedCaptureAccepted;
	bool m_bTransientStateStaged;
	bool m_bProfileFallbackSaved;
	bool m_bSavePointRequested;
	bool m_bControlledShutdownPlayerReleaseRequired;
	bool m_bControlledShutdownRuntimeQuiescenceApplied;
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
	bool m_bPreparedDetachedCheckpoint;
#ifdef ENABLE_DIAG
	bool m_bCampaignDebugRestoreCheckpoint;
#endif
}

#ifdef ENABLE_DIAG
// Exact, cumulative receipt for restoring the live campaign authority after an
// isolated Campaign Debug run. The verified profile journal is always the
// synchronous durable recovery channel. When native authority is active, both
// transient staging and the SaveGameManager storage-commit callback must succeed
// before that journal mirror is accepted. A partial attempt keeps the frozen
// candidate and channel receipts so the same payload can be retried without
// advancing its checkpoint sequence or overwriting the pre-run tracked snapshot.
class HST_CampaignDebugPersistenceRestoreResult
{
	bool m_bAttempted;
	bool m_bIsolationAuthorityPresent;
	bool m_bCandidatePrepared;
	bool m_bNativeStageRequired;
	bool m_bNativeStageComplete;
	bool m_bNativeCommitRequired;
	bool m_bNativeCommitComplete;
	bool m_bProfileJournalRequired = true;
	bool m_bProfileJournalComplete;
	bool m_bRecoveryAuthorityRetained;
	bool m_bRestoreComplete;
	string m_sCandidateFingerprint;
	string m_sEvidence;

	bool AllRequiredChannelsExact()
	{
		return m_bAttempted && m_bIsolationAuthorityPresent
			&& m_bCandidatePrepared
			&& (!m_bNativeStageRequired || m_bNativeStageComplete)
			&& (!m_bNativeCommitRequired || m_bNativeCommitComplete)
			&& (!m_bProfileJournalRequired || m_bProfileJournalComplete)
			&& m_bRestoreComplete && !m_bRecoveryAuthorityRetained;
	}

	string BuildReport()
	{
		string report = string.Format(
			"attempted %1 | isolation authority %2 | candidate %3 | native stage required/complete %4/%5",
			m_bAttempted,
			m_bIsolationAuthorityPresent,
			m_bCandidatePrepared,
			m_bNativeStageRequired,
			m_bNativeStageComplete);
		report += string.Format(
			" | native commit required/complete %1/%2 | profile required/complete %3/%4 | recovery retained %5",
			m_bNativeCommitRequired,
			m_bNativeCommitComplete,
			m_bProfileJournalRequired,
			m_bProfileJournalComplete,
			m_bRecoveryAuthorityRetained);
		report += string.Format(
			" | complete %1 | fingerprint %2",
			m_bRestoreComplete,
			m_sCandidateFingerprint);
		return report + " | " + m_sEvidence;
	}
}
#endif

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
	protected ref HST_CampaignSaveData m_LastCapturedSave;
	protected ref HST_CampaignSaveData m_TrackedCampaignSave;
	protected ref HST_CampaignSaveData m_RestoredCampaignSave;
#ifdef ENABLE_DIAG
	protected bool m_bCampaignDebugIsolationActive;
	protected ref HST_CampaignSaveData m_IsolatedCapturedSave;
	protected bool m_bCampaignDebugIsolationNativeStageRequired;
	protected ref HST_CampaignSaveData m_CampaignDebugRestoreCandidateSave;
	protected HST_CampaignState m_CampaignDebugRestoreStateAuthority;
	protected bool m_bCampaignDebugRestoreNativeStageRequired;
	protected bool m_bCampaignDebugRestoreNativeStageComplete;
	protected bool m_bCampaignDebugRestoreNativeCommitRequired;
	protected bool m_bCampaignDebugRestoreNativeCommitComplete;
	protected bool m_bCampaignDebugRestoreProfileJournalComplete;
	protected string m_sCampaignDebugRestoreCandidateFingerprint;
	protected ref HST_CampaignDebugPersistenceRestoreResult
		m_LastCampaignDebugPersistenceRestoreResult;
#endif
	protected HST_CampaignPersistentState m_NativeCampaignState;
	protected ref HST_PersistenceSourceResolution m_LastSourceResolution;
	protected ref HST_CampaignProfileSaveJournalService m_ProfileJournal
		= new HST_CampaignProfileSaveJournalService();
	protected ref HST_CampaignProfileSaveResolution
		m_LastProfileJournalResolution;
	protected ref HST_CampaignProfileSaveWriteReceipt
		m_LastProfileJournalWriteReceipt;
	protected bool m_bProfileFallbackSaved;
	protected bool m_bProfileFallbackLoaded;
	protected bool m_bProfileFallbackOnlyForSession;
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
	protected HST_LootService m_Loot;

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

	void SetLootService(HST_LootService loot)
	{
		m_Loot = loot;
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
#ifdef ENABLE_DIAG
		if (m_bCampaignDebugIsolationActive)
			return;
#endif
		if (m_bMajorChangePending)
			return;
		m_bMajorChangePending = true;
		m_fMajorChangeElapsed = 0;
	}

#ifdef ENABLE_DIAG
	bool IsCampaignDebugIsolationActive()
	{
		return m_bCampaignDebugIsolationActive;
	}
#endif

	HST_PersistenceCheckpointRequest Tick(
		HST_CampaignState state,
		float timeSlice,
		int autosaveIntervalSeconds,
		int majorChangeDebounceSeconds,
		SaveGameOperationCallback completionObserver = null)
	{
		float deltaSeconds = Math.Max(0.0, timeSlice);
		TickCheckpointCommitTimeout(deltaSeconds);
#ifdef ENABLE_DIAG
		if (m_bCampaignDebugIsolationActive)
			return null;
#endif

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
		string controlledShutdownLootEvidence;
		string controlledShutdownRescueEvidence;
		string controlledShutdownVehicleEvidence;
		string controlledShutdownActiveGroupEvidence;
		if (saveType == ESaveGameType.SHUTDOWN)
		{
			StampControlledShutdownRequestFlags(
				request,
				controlledShutdownActiveGroupEvidence,
				controlledShutdownVehicleEvidence,
				controlledShutdownLootEvidence,
				controlledShutdownRescueEvidence);
		}

#ifdef ENABLE_DIAG
		if (m_bCampaignDebugIsolationActive)
		{
			request.m_bCampaignCaptured = CaptureIsolatedCampaignDebugState(
				state,
				"isolated checkpoint: " + displayName);
			request.m_bIsolatedCaptureAccepted = request.m_bCampaignCaptured;
			request.m_sEvidence = "campaign-debug checkpoint remained isolated";
			return request;
		}
#endif
		SaveGameManager saveManager = SaveGameManager.Get();
		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		bool nativeCheckpointExpected = persistence
			&& persistence.GetState() == EPersistenceSystemState.ACTIVE
			&& !m_bProfileFallbackOnlyForSession;
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
		if (saveType == ESaveGameType.SHUTDOWN && state)
		{
			// Treat controlled shutdown as one ordered, retry-safe transaction. Every
			// live authority domain is inspected read-only before mutation. One complete
			// fallible preparation pass then closes player-driven promotion and rejects
			// malformed live authority before any latch is published. Nearby roots are
			// adopted next, followed by a second full pass that includes those roots and
			// repeats on retries: latched domains maintain their pins while unfenced
			// domains refresh durable authority. Strict repeated preflights then guard the
			// native fences and final rescue DTO-graph latch used by capture.
			bool controlledShutdownFenceApplied
				= (m_PhysicalWar
						&& m_PhysicalWar.HasControlledShutdownActiveGroupQuiescenceApplied())
					|| (m_Civilians
						&& m_Civilians.HasControlledShutdownVehiclePersistenceApplied())
					|| (m_RescuePOWOperations
						&& m_RescuePOWOperations.HasControlledShutdownPersistenceSampleApplied());
			bool controlledShutdownNearbySnapshotApplied
				= m_Loot
					&& m_Loot.HasControlledShutdownNearbyPersistentVehicleSnapshotApplied();
			if (controlledShutdownFenceApplied
				&& !controlledShutdownNearbySnapshotApplied)
			{
				StampControlledShutdownRequestFlags(
					request,
					controlledShutdownActiveGroupEvidence,
					controlledShutdownVehicleEvidence,
					controlledShutdownLootEvidence,
					controlledShutdownRescueEvidence);
				request.m_sEvidence
					= "checkpoint deferred: controlled-shutdown runtime fence exists without its required nearby-vehicle scope latch";
				return request;
			}
			int nearbyVehicleCandidateCount;
			if (!m_Loot
				|| !m_Loot.PreflightControlledShutdownNearbyPersistentVehicles(
					state,
					nearbyVehicleCandidateCount,
					controlledShutdownLootEvidence))
			{
				StampControlledShutdownRequestFlags(
					request,
					controlledShutdownActiveGroupEvidence,
					controlledShutdownVehicleEvidence,
					controlledShutdownLootEvidence,
					controlledShutdownRescueEvidence);
				request.m_sEvidence
					= "checkpoint deferred: controlled-shutdown nearby field vehicle preflight failed";
				if (!controlledShutdownLootEvidence.IsEmpty())
					request.m_sEvidence += " | "
						+ controlledShutdownLootEvidence;
				return request;
			}
			if (!m_RescuePOWOperations
				|| !m_RescuePOWOperations.PreflightControlledShutdownPersistenceSample(
					state,
					controlledShutdownRescueEvidence,
					false))
			{
				StampControlledShutdownRequestFlags(
					request,
					controlledShutdownActiveGroupEvidence,
					controlledShutdownVehicleEvidence,
					controlledShutdownLootEvidence,
					controlledShutdownRescueEvidence);
				request.m_sEvidence
					= "checkpoint deferred: controlled-shutdown rescue persistence preflight failed";
				if (!controlledShutdownRescueEvidence.IsEmpty())
					request.m_sEvidence += " | "
						+ controlledShutdownRescueEvidence;
				return request;
			}
			// A previous adoption attempt may have pinned a new durable row before a
			// fallible native-detach call returned. Resume that already locked scope
			// before the ordinary field graph preflight, which correctly rejects an
			// active row until its exact live binding has been completed.
			if (controlledShutdownNearbySnapshotApplied
				&& !m_Loot.IsControlledShutdownNearbyPersistentVehicleSnapshotExact())
			{
				if (!m_PhysicalWar
					|| !m_PhysicalWar.PreflightControlledShutdownActiveGroupQuiescence(
						state,
						controlledShutdownActiveGroupEvidence))
				{
					StampControlledShutdownRequestFlags(
						request,
						controlledShutdownActiveGroupEvidence,
						controlledShutdownVehicleEvidence,
						controlledShutdownLootEvidence,
						controlledShutdownRescueEvidence);
					request.m_sEvidence
						= "checkpoint deferred: controlled-shutdown active-group preflight failed before nearby vehicle adoption resumed";
					if (!controlledShutdownActiveGroupEvidence.IsEmpty())
						request.m_sEvidence += " | "
							+ controlledShutdownActiveGroupEvidence;
					return request;
				}
				int resumedNearbyVehicles;
				if (!m_Loot.SnapshotNearbyPersistentVehiclesForControlledShutdown(
					state,
					resumedNearbyVehicles,
					controlledShutdownLootEvidence))
				{
					StampControlledShutdownRequestFlags(
						request,
						controlledShutdownActiveGroupEvidence,
						controlledShutdownVehicleEvidence,
						controlledShutdownLootEvidence,
						controlledShutdownRescueEvidence);
					request.m_sEvidence
						= "checkpoint deferred: controlled-shutdown nearby field vehicle adoption retry failed";
					if (!controlledShutdownLootEvidence.IsEmpty())
						request.m_sEvidence += " | "
							+ controlledShutdownLootEvidence;
					return request;
				}
			}
			if (!m_Civilians
				|| !m_Civilians.PreflightControlledShutdownVehiclePersistence(
					state,
					controlledShutdownVehicleEvidence,
					false))
			{
				StampControlledShutdownRequestFlags(
					request,
					controlledShutdownActiveGroupEvidence,
					controlledShutdownVehicleEvidence,
					controlledShutdownLootEvidence,
					controlledShutdownRescueEvidence);
				request.m_sEvidence
					= "checkpoint deferred: controlled-shutdown field vehicle preflight failed";
				if (!controlledShutdownVehicleEvidence.IsEmpty())
					request.m_sEvidence += " | "
						+ controlledShutdownVehicleEvidence;
				return request;
			}
			if (!m_PhysicalWar
				|| !m_PhysicalWar.PreflightControlledShutdownActiveGroupQuiescence(
					state,
					controlledShutdownActiveGroupEvidence))
			{
				StampControlledShutdownRequestFlags(
					request,
					controlledShutdownActiveGroupEvidence,
					controlledShutdownVehicleEvidence,
					controlledShutdownLootEvidence,
					controlledShutdownRescueEvidence);
				request.m_sEvidence
					= "checkpoint deferred: controlled-shutdown active-group preflight failed";
				if (!controlledShutdownActiveGroupEvidence.IsEmpty())
					request.m_sEvidence += " | "
						+ controlledShutdownActiveGroupEvidence;
				return request;
			}
			StampControlledShutdownRequestFlags(
				request,
				controlledShutdownActiveGroupEvidence,
				controlledShutdownVehicleEvidence,
				controlledShutdownLootEvidence,
				controlledShutdownRescueEvidence);
			// Run one complete fallible preparation before publishing the nearby binding
			// plan. This closes ambient promotion and rejects malformed rescue/infantry
			// authority before any process-local latch can make the attempt one-way. A
			// retry with an existing plan skips this pre-latch pass.
			if (!controlledShutdownNearbySnapshotApplied
				&& !PrepareStateForCapture(
					state,
					"controlled-shutdown admission before nearby scope lock"))
			{
				request.m_sEvidence = state.m_sLastPersistenceStatus;
				if (request.m_sEvidence.IsEmpty())
				{
					request.m_sEvidence
						= "checkpoint deferred: controlled-shutdown authority preparation failed before nearby scope lock";
				}
				StampControlledShutdownRequestFlags(
					request,
					controlledShutdownActiveGroupEvidence,
					controlledShutdownVehicleEvidence,
					controlledShutdownLootEvidence,
					controlledShutdownRescueEvidence);
				return request;
			}
			// Preparation may legitimately change live ownership (for example, an
			// on-foot rescue follower can become BOARDING when its escort is already
			// seated). Re-run every relevant read-only/player gate against that
			// prepared graph before Loot publishes the first one-way scope latch.
			// This is the last point at which canonical player-release evidence can
			// safely return controls without leaving a partially fenced transaction.
			if (!controlledShutdownNearbySnapshotApplied)
			{
				int preparedNearbyVehicleCandidateCount;
				if (!m_Loot.PreflightControlledShutdownNearbyPersistentVehicles(
					state,
					preparedNearbyVehicleCandidateCount,
					controlledShutdownLootEvidence))
				{
					StampControlledShutdownRequestFlags(
						request,
						controlledShutdownActiveGroupEvidence,
						controlledShutdownVehicleEvidence,
						controlledShutdownLootEvidence,
						controlledShutdownRescueEvidence);
					request.m_sEvidence
						= "checkpoint deferred: prepared controlled-shutdown nearby field vehicle preflight failed before scope lock";
					if (!controlledShutdownLootEvidence.IsEmpty())
						request.m_sEvidence += " | "
							+ controlledShutdownLootEvidence;
					return request;
				}
				if (!m_RescuePOWOperations.PreflightControlledShutdownPersistenceSample(
					state,
					controlledShutdownRescueEvidence,
					true))
				{
					StampControlledShutdownRequestFlags(
						request,
						controlledShutdownActiveGroupEvidence,
						controlledShutdownVehicleEvidence,
						controlledShutdownLootEvidence,
						controlledShutdownRescueEvidence);
					request.m_sEvidence
						= "checkpoint deferred: prepared controlled-shutdown rescue preflight failed before scope lock";
					if (!controlledShutdownRescueEvidence.IsEmpty())
						request.m_sEvidence += " | "
							+ controlledShutdownRescueEvidence;
					return request;
				}
				if (!m_Civilians.PreflightControlledShutdownVehiclePersistence(
					state,
					controlledShutdownVehicleEvidence,
					true))
				{
					StampControlledShutdownRequestFlags(
						request,
						controlledShutdownActiveGroupEvidence,
						controlledShutdownVehicleEvidence,
						controlledShutdownLootEvidence,
						controlledShutdownRescueEvidence);
					request.m_sEvidence
						= "checkpoint deferred: prepared controlled-shutdown field vehicle preflight failed before scope lock";
					if (!controlledShutdownVehicleEvidence.IsEmpty())
						request.m_sEvidence += " | "
							+ controlledShutdownVehicleEvidence;
					return request;
				}
				if (!m_PhysicalWar.PreflightControlledShutdownActiveGroupQuiescence(
					state,
					controlledShutdownActiveGroupEvidence))
				{
					StampControlledShutdownRequestFlags(
						request,
						controlledShutdownActiveGroupEvidence,
						controlledShutdownVehicleEvidence,
						controlledShutdownLootEvidence,
						controlledShutdownRescueEvidence);
					request.m_sEvidence
						= "checkpoint deferred: prepared controlled-shutdown active-group preflight failed before scope lock";
					if (!controlledShutdownActiveGroupEvidence.IsEmpty())
						request.m_sEvidence += " | "
							+ controlledShutdownActiveGroupEvidence;
					return request;
				}
			}
			if (!controlledShutdownNearbySnapshotApplied)
			{
				int snapshottedNearbyVehicles;
				if (!m_Loot.SnapshotNearbyPersistentVehiclesForControlledShutdown(
					state,
					snapshottedNearbyVehicles,
					controlledShutdownLootEvidence))
				{
					StampControlledShutdownRequestFlags(
						request,
						controlledShutdownActiveGroupEvidence,
						controlledShutdownVehicleEvidence,
						controlledShutdownLootEvidence,
						controlledShutdownRescueEvidence);
					request.m_sEvidence
						= "checkpoint deferred: controlled-shutdown nearby field vehicle adoption failed";
					if (!controlledShutdownLootEvidence.IsEmpty())
						request.m_sEvidence += " | "
							+ controlledShutdownLootEvidence;
					return request;
				}
				controlledShutdownNearbySnapshotApplied = true;
			}
			// This post-latch full-state sampler runs after the nearby scope is complete,
			// and it is intentionally repeated
			// on retries: already applied domain fences maintain their original pins,
			// while an unfenced domain can refresh durable authority that moved between
			// attempts. The binding lock makes civilian/Loot discovery a no-op here.
			if (!PrepareStateForCapture(
				state,
				"controlled-shutdown prepared authority after nearby scope lock"))
			{
				request.m_sEvidence = state.m_sLastPersistenceStatus;
				if (request.m_sEvidence.IsEmpty())
				{
					request.m_sEvidence
						= "checkpoint deferred: controlled-shutdown campaign capture preflight failed";
				}
				StampControlledShutdownRequestFlags(
					request,
					controlledShutdownActiveGroupEvidence,
					controlledShutdownVehicleEvidence,
					controlledShutdownLootEvidence,
					controlledShutdownRescueEvidence);
				return request;
			}
			if (!m_Civilians.PreflightControlledShutdownVehiclePersistence(
				state,
				controlledShutdownVehicleEvidence,
				true))
			{
				StampControlledShutdownRequestFlags(
					request,
					controlledShutdownActiveGroupEvidence,
					controlledShutdownVehicleEvidence,
					controlledShutdownLootEvidence,
					controlledShutdownRescueEvidence);
				request.m_sEvidence
					= "checkpoint deferred: controlled-shutdown prepared field vehicle preflight failed";
				if (!controlledShutdownVehicleEvidence.IsEmpty())
					request.m_sEvidence += " | "
						+ controlledShutdownVehicleEvidence;
				return request;
			}
			if (!m_RescuePOWOperations.PreflightControlledShutdownPersistenceSample(
				state,
				controlledShutdownRescueEvidence,
				true))
			{
				StampControlledShutdownRequestFlags(
					request,
					controlledShutdownActiveGroupEvidence,
					controlledShutdownVehicleEvidence,
					controlledShutdownLootEvidence,
					controlledShutdownRescueEvidence);
				request.m_sEvidence
					= "checkpoint deferred: controlled-shutdown prepared rescue persistence preflight failed";
				if (!controlledShutdownRescueEvidence.IsEmpty())
					request.m_sEvidence += " | "
						+ controlledShutdownRescueEvidence;
				return request;
			}
			if (!m_PhysicalWar.PrepareControlledShutdownActiveGroupQuiescence(
				state,
				controlledShutdownActiveGroupEvidence))
			{
				StampControlledShutdownRequestFlags(
					request,
					controlledShutdownActiveGroupEvidence,
					controlledShutdownVehicleEvidence,
					controlledShutdownLootEvidence,
					controlledShutdownRescueEvidence);
				request.m_sEvidence
					= "checkpoint deferred: controlled-shutdown active-group quiescence failed";
				if (!controlledShutdownActiveGroupEvidence.IsEmpty())
					request.m_sEvidence += " | "
						+ controlledShutdownActiveGroupEvidence;
				return request;
			}
			StampControlledShutdownRequestFlags(
				request,
				controlledShutdownActiveGroupEvidence,
				controlledShutdownVehicleEvidence,
				controlledShutdownLootEvidence,
				controlledShutdownRescueEvidence);
			if (!m_Civilians
				|| !m_Civilians.PrepareControlledShutdownVehiclePersistence(
					state,
					controlledShutdownVehicleEvidence))
			{
				StampControlledShutdownRequestFlags(
					request,
					controlledShutdownActiveGroupEvidence,
					controlledShutdownVehicleEvidence,
					controlledShutdownLootEvidence,
					controlledShutdownRescueEvidence);
				request.m_sEvidence
					= "checkpoint deferred: controlled-shutdown field vehicle quiescence failed";
				if (!controlledShutdownVehicleEvidence.IsEmpty())
					request.m_sEvidence += " | "
						+ controlledShutdownVehicleEvidence;
				return request;
			}
			StampControlledShutdownRequestFlags(
				request,
				controlledShutdownActiveGroupEvidence,
				controlledShutdownVehicleEvidence,
				controlledShutdownLootEvidence,
				controlledShutdownRescueEvidence);
			if (!m_RescuePOWOperations.PrepareControlledShutdownPersistenceSample(
				state,
				controlledShutdownRescueEvidence))
			{
				StampControlledShutdownRequestFlags(
					request,
					controlledShutdownActiveGroupEvidence,
					controlledShutdownVehicleEvidence,
					controlledShutdownLootEvidence,
					controlledShutdownRescueEvidence);
				request.m_sEvidence
					= "checkpoint deferred: controlled-shutdown rescue persistence fence failed";
				if (!controlledShutdownRescueEvidence.IsEmpty())
					request.m_sEvidence += " | "
						+ controlledShutdownRescueEvidence;
				return request;
			}
			StampControlledShutdownRequestFlags(
				request,
				controlledShutdownActiveGroupEvidence,
				controlledShutdownVehicleEvidence,
				controlledShutdownLootEvidence,
				controlledShutdownRescueEvidence);
		}

		if (state && !CaptureAndTrackState(state, "captured before checkpoint"))
		{
			request.m_sEvidence = "campaign capture failed";
			return request;
		}
		request.m_bCampaignCaptured = m_TrackedCampaignSave != null;

		if (!m_bProfileFallbackOnlyForSession)
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
		bool completionReceivedInline = false;
		string inlineCompletionEvidence;
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
			// BLOCKING shutdown saves may invoke their completion callback before
			// RequestSavePoint returns. Preserve that terminal result across the
			// normal accepted-request bookkeeping below instead of rewriting it as
			// an in-flight request and clearing the callback's dirty-state rearm.
			completionReceivedInline = request.m_bCompletionReceived;
			inlineCompletionEvidence = request.m_sEvidence;
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
		if (completionReceivedInline
			&& (!request.m_bNativeCommitSucceeded
				|| !request.m_bProfileFallbackSaved))
		{
			EnsureMajorChangePending();
		}

		if (state)
		{
			state.m_iLastSaveSecond = state.m_iElapsedSeconds;
			if (completionReceivedInline)
			{
				state.m_sLastPersistenceStatus = string.Format(
					"checkpoint completed inline: %1 | type %2 | transient save %3 | native commit %4 | profile fallback %5 | savepoint %6",
					displayName,
					saveType,
					request.m_bTransientStateStaged,
					request.m_bNativeCommitSucceeded,
					request.m_bProfileFallbackSaved,
					request.m_bSavePointRequested);
			}
			else
			{
				state.m_sLastPersistenceStatus = string.Format(
					"checkpoint requested: %1 | type %2 | transient save %3 | profile fallback %4 | savepoint %5 | native in flight %6",
					displayName,
					saveType,
					request.m_bTransientStateStaged,
					request.m_bProfileFallbackSaved,
					request.m_bSavePointRequested,
					m_bCheckpointSavePointInFlight);
			}
		}
		if (state)
			request.m_sEvidence = state.m_sLastPersistenceStatus;
		else
		{
			request.m_sEvidence
				= "checkpoint request accepted without campaign status target";
		}
		if (!inlineCompletionEvidence.IsEmpty())
			request.m_sEvidence += inlineCompletionEvidence;
		if (!controlledShutdownActiveGroupEvidence.IsEmpty())
			request.m_sEvidence += " | "
				+ controlledShutdownActiveGroupEvidence;
		if (!controlledShutdownVehicleEvidence.IsEmpty())
			request.m_sEvidence += " | "
				+ controlledShutdownVehicleEvidence;
		if (!controlledShutdownLootEvidence.IsEmpty())
			request.m_sEvidence += " | "
				+ controlledShutdownLootEvidence;
		if (!controlledShutdownRescueEvidence.IsEmpty())
			request.m_sEvidence += " | "
				+ controlledShutdownRescueEvidence;
		return request;
	}

	protected void StampControlledShutdownRequestFlags(
		HST_PersistenceCheckpointRequest request,
		string activeGroupEvidence,
		string fieldVehicleEvidence,
		string lootEvidence,
		string rescueEvidence)
	{
		if (!request)
			return;
		bool activeGroupApplied
			= m_PhysicalWar
				&& m_PhysicalWar.HasControlledShutdownActiveGroupQuiescenceApplied();
		bool fieldVehicleApplied
			= m_Civilians
				&& m_Civilians.HasControlledShutdownVehiclePersistenceApplied();
		bool rescueApplied
			= m_RescuePOWOperations
				&& m_RescuePOWOperations.HasControlledShutdownPersistenceSampleApplied();
		bool nearbyVehicleSnapshotApplied
			= m_Loot
				&& m_Loot.HasControlledShutdownNearbyPersistentVehicleSnapshotApplied();
		request.m_bControlledShutdownRuntimeQuiescenceApplied
			= activeGroupApplied || fieldVehicleApplied || rescueApplied
				|| nearbyVehicleSnapshotApplied;
		request.m_bControlledShutdownPlayerReleaseRequired
			= !request.m_bControlledShutdownRuntimeQuiescenceApplied
				&& (activeGroupEvidence.Contains(
						HST_PhysicalWarService.CONTROLLED_SHUTDOWN_PLAYER_RELEASE_EVIDENCE)
					|| fieldVehicleEvidence.Contains(
						HST_PhysicalWarService.CONTROLLED_SHUTDOWN_PLAYER_RELEASE_EVIDENCE)
					|| lootEvidence.Contains(
						HST_PhysicalWarService.CONTROLLED_SHUTDOWN_PLAYER_RELEASE_EVIDENCE)
					|| rescueEvidence.Contains(
						HST_PhysicalWarService.CONTROLLED_SHUTDOWN_PLAYER_RELEASE_EVIDENCE));
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

	// Administrative reset prepares its prospective authority while the old
	// campaign is still live. Accept that already-detached DTO directly so the
	// checkpoint can be admitted before irreversible runtime cleanup. This path
	// deliberately does not call PrepareStateForCapture: the caller owns all
	// reset-specific reconciliation and must provide one complete current-schema
	// snapshot whose durable checkpoint sequence is newer than every authority
	// visible to this service.
	//
	// The supplied DTO is serialized and read into a private exact clone, then
	// committed to the verified profile journal as a write-ahead record before
	// native staging. The callback fires at that journal commit boundary so the
	// caller can retire old roots in the same script turn; native replication then
	// saves the already-detached DTO and the cleaned world. The caller may keep or
	// discard its copy without changing either durable payload.
	HST_PersistenceCheckpointRequest RequestPreparedManualCheckpointDetailed(
		HST_CampaignSaveData preparedSnapshot,
		HST_CampaignState statusState = null,
		SaveGameOperationCallback completionObserver = null)
	{
		HST_PersistenceCheckpointRequest request
			= new HST_PersistenceCheckpointRequest();
		request.m_eSaveType = ESaveGameType.MANUAL;
		request.m_eRequestFlags = 0;
		request.m_sDisplayName = "Partisan manual checkpoint";

#ifdef ENABLE_DIAG
		if (m_bCampaignDebugIsolationActive)
		{
			request.m_sEvidence
				= "prepared manual checkpoint rejected: campaign debug isolation is active";
			return request;
		}
#endif
		if (m_bCheckpointSavePointInFlight)
		{
			request.m_sEvidence
				= "prepared manual checkpoint rejected: a native checkpoint is already in flight";
			return request;
		}

		HST_CampaignSaveData detachedSnapshot;
		string snapshotFingerprint;
		string snapshotEvidence;
		if (!CloneAndValidatePreparedCheckpointSnapshot(
			preparedSnapshot,
			detachedSnapshot,
			snapshotFingerprint,
			snapshotEvidence))
		{
			request.m_sEvidence
				= "prepared manual checkpoint rejected: " + snapshotEvidence;
			return request;
		}

		string readinessEvidence;
		if (!ResolvePreparedCheckpointReadiness(
			detachedSnapshot,
			readinessEvidence))
		{
			request.m_sEvidence
				= "prepared manual checkpoint rejected: " + readinessEvidence;
			return request;
		}

		request.m_bCampaignCaptured = true;
		request.m_bPreparedDetachedSnapshotAccepted = true;
		request.m_sPreparedSnapshotFingerprint = snapshotFingerprint;
		request.m_iPreparedSnapshotCheckpointSequence
			= detachedSnapshot.m_iPersistenceCheckpointSequence;
		request.m_iPreparedSnapshotRestoreSequence
			= detachedSnapshot.m_iPersistenceRestoreSequence;

		// The verified profile journal is the write-ahead commit record for an
		// administrative reset. Once this synchronous write succeeds, either the
		// journal or the native save can recover the same reset after a crash. A
		// later native failure is therefore degraded replication, not grounds for
		// resurrecting the old campaign in memory.
		request.m_bProfileFallbackSaved
			= SaveProfileFallback(detachedSnapshot);
		if (!request.m_bProfileFallbackSaved)
		{
			request.m_bCampaignCaptured = false;
			request.m_bPreparedDetachedSnapshotAccepted = false;
			request.m_sEvidence
				= "prepared manual checkpoint rejected before native staging: verified write-ahead journal commit failed | "
					+ snapshotEvidence + " | " + readinessEvidence;
			return request;
		}

		m_LastCapturedSave = detachedSnapshot;
		m_TrackedCampaignSave = detachedSnapshot;
		AcknowledgeAcceptedCheckpointCoverage();
		request.m_sEvidence = string.Format(
			"prepared reset write-ahead journal committed | schema %1 | checkpoint/restore sequence %2/%3 | fingerprint %4 | %5 | %6",
			detachedSnapshot.m_iSchemaVersion,
			detachedSnapshot.m_iPersistenceCheckpointSequence,
			detachedSnapshot.m_iPersistenceRestoreSequence,
			snapshotFingerprint,
			snapshotEvidence,
			readinessEvidence);
		if (statusState)
		{
			statusState.m_iLastSaveSecond = statusState.m_iElapsedSeconds;
			statusState.m_sLastPersistenceStatus = request.m_sEvidence;
		}
		// The coordinator may now retire old runtime roots synchronously. Native
		// staging happens later in this same script call, after that cleanup, and
		// uses the already-detached DTO rather than mutable live state.
		if (completionObserver)
			completionObserver.InvokeDelegate(true);

		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		bool nativeCheckpointRequired = persistence
			&& persistence.GetState() == EPersistenceSystemState.ACTIVE
			&& !m_bProfileFallbackOnlyForSession;
		if (!nativeCheckpointRequired)
		{
			request.m_bCompletionReceived = true;
			request.m_sEvidence
				+= " | profile-journal authority committed; native persistence unavailable for this session";
			return request;
		}

		HST_CampaignPersistentState nativeState
			= ResolveNativeCampaignState(persistence);
		SaveGameManager saveManager = SaveGameManager.Get();
		if (!nativeState || !saveManager)
		{
			request.m_bCompletionReceived = true;
			request.m_sEvidence
				+= " | native authority disappeared after journal commit; reset remains committed and native repair is pending";
			EnsureMajorChangePending();
			return request;
		}
		nativeState.SetSnapshotForSave(detachedSnapshot);
		if (nativeState.GetSnapshotFingerprint() != snapshotFingerprint
			|| !persistence.Save(nativeState, ESaveGameType.MANUAL))
		{
			request.m_bCompletionReceived = true;
			request.m_sEvidence
				+= " | native transient staging failed after journal commit; reset remains committed and native repair is pending";
			EnsureMajorChangePending();
			return request;
		}
		request.m_bTransientStateStaged = true;
		m_NativeCampaignState = nativeState;
		m_bCheckpointSavePointInFlight = true;
		m_PendingCheckpointRequest = request;
		m_PendingCheckpointSaveData = detachedSnapshot;
		// The write-ahead observer was already notified exactly once. Native
		// completion updates the request and repair scheduler, but must not repeat
		// the campaign reset callback.
		m_PendingCheckpointObserver = null;
		m_PendingCheckpointState = statusState;
		m_iCheckpointRequestSequence++;
		m_fCheckpointCommitElapsedSeconds = 0;
		m_CheckpointCallbackContext
			= new HST_PersistenceCheckpointCallbackContext();
		m_CheckpointCallbackContext.m_iRequestSequence
			= m_iCheckpointRequestSequence;
		m_CheckpointCallbackContext.m_bPreparedDetachedCheckpoint = true;
		m_CheckpointCompletionCallback = new SaveGameOperationCallback(
			OnCheckpointSavePointCompleted,
			m_CheckpointCallbackContext);
		request.m_bSavePointRequested = true;
		saveManager.RequestSavePoint(
			ESaveGameType.MANUAL,
			request.m_sDisplayName,
			0,
			m_CheckpointCompletionCallback);
		request.m_sEvidence
			+= " | native manual save point requested from the committed reset DTO";
		return request;
	}

	protected bool CloneAndValidatePreparedCheckpointSnapshot(
		HST_CampaignSaveData preparedSnapshot,
		out HST_CampaignSaveData detachedSnapshot,
		out string snapshotFingerprint,
		out string evidence)
	{
		detachedSnapshot = null;
		snapshotFingerprint = "";
		evidence = "prepared snapshot validation was not attempted";
		if (!preparedSnapshot)
		{
			evidence = "prepared snapshot is null";
			return false;
		}
		if (preparedSnapshot.m_iSchemaVersion
			!= HST_CampaignState.SCHEMA_VERSION)
		{
			evidence = string.Format(
				"prepared snapshot schema %1 is not current schema %2",
				preparedSnapshot.m_iSchemaVersion,
				HST_CampaignState.SCHEMA_VERSION);
			return false;
		}
		if (preparedSnapshot.m_iPersistenceCheckpointSequence <= 0)
		{
			evidence
				= "prepared snapshot has no positive durable checkpoint sequence";
			return false;
		}
		if (preparedSnapshot.m_iPersistenceRestoreSequence < 0)
		{
			evidence
				= "prepared snapshot has a negative durable restore sequence";
			return false;
		}

		string exactPayload;
		if (!HST_CampaignPersistentState.TrySerializeSnapshot(
			preparedSnapshot,
			exactPayload,
			snapshotFingerprint))
		{
			evidence = "prepared snapshot exact serialization failed";
			return false;
		}
		JsonLoadContext cloneContext = new JsonLoadContext();
		if (!cloneContext.LoadFromString(exactPayload))
		{
			evidence = "prepared snapshot exact clone JSON load failed";
			return false;
		}
		detachedSnapshot = new HST_CampaignSaveData();
		if (!cloneContext.ReadValue("", detachedSnapshot))
		{
			detachedSnapshot = null;
			evidence = "prepared snapshot exact clone DTO read failed";
			return false;
		}
		string cloneFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(
				detachedSnapshot);
		if (cloneFingerprint.IsEmpty()
			|| cloneFingerprint != snapshotFingerprint
			|| detachedSnapshot.m_iSchemaVersion
				!= preparedSnapshot.m_iSchemaVersion
			|| detachedSnapshot.m_iPersistenceCheckpointSequence
				!= preparedSnapshot.m_iPersistenceCheckpointSequence
			|| detachedSnapshot.m_iPersistenceRestoreSequence
				!= preparedSnapshot.m_iPersistenceRestoreSequence)
		{
			detachedSnapshot = null;
			evidence
				= "prepared snapshot exact clone identity or durable ordering changed";
			return false;
		}

		evidence = string.Format(
			"prepared snapshot exact clone verified | schema %1 | checkpoint/restore sequence %2/%3 | fingerprint %4",
			detachedSnapshot.m_iSchemaVersion,
			detachedSnapshot.m_iPersistenceCheckpointSequence,
			detachedSnapshot.m_iPersistenceRestoreSequence,
			snapshotFingerprint);
		return true;
	}

	protected bool ResolvePreparedCheckpointReadiness(
		HST_CampaignSaveData detachedSnapshot,
		out string evidence)
	{
		evidence = "prepared checkpoint readiness was not attempted";
		if (!detachedSnapshot)
		{
			evidence = "prepared checkpoint has no detached snapshot";
			return false;
		}

		string journalEvidence;
		if (!m_ProfileJournal.CanAdvanceVerifiedSnapshot(journalEvidence))
		{
			evidence = journalEvidence;
			return false;
		}
		HST_CampaignProfileSaveResolution journalResolution
			= m_ProfileJournal.ResolveJournal();
		if (!journalResolution)
		{
			evidence = "profile journal resolution is unavailable";
			return false;
		}

		int highestKnownCheckpointSequence;
		string highestKnownSource = "empty authority";
		if (m_TrackedCampaignSave
			&& m_TrackedCampaignSave.m_iPersistenceCheckpointSequence
				> highestKnownCheckpointSequence)
		{
			highestKnownCheckpointSequence
				= m_TrackedCampaignSave.m_iPersistenceCheckpointSequence;
			highestKnownSource = "tracked campaign snapshot";
		}
		if (m_LastCapturedSave
			&& m_LastCapturedSave.m_iPersistenceCheckpointSequence
				> highestKnownCheckpointSequence)
		{
			highestKnownCheckpointSequence
				= m_LastCapturedSave.m_iPersistenceCheckpointSequence;
			highestKnownSource = "last captured campaign snapshot";
		}
		if (m_RestoredCampaignSave
			&& m_RestoredCampaignSave.m_iPersistenceCheckpointSequence
				> highestKnownCheckpointSequence)
		{
			highestKnownCheckpointSequence
				= m_RestoredCampaignSave.m_iPersistenceCheckpointSequence;
			highestKnownSource = "restored campaign snapshot";
		}
		if (journalResolution.m_bHasSelection
			&& journalResolution.m_Selected
			&& journalResolution.m_Selected.m_SaveData
			&& journalResolution.m_Selected.m_SaveData
				.m_iPersistenceCheckpointSequence
					> highestKnownCheckpointSequence)
		{
			highestKnownCheckpointSequence
				= journalResolution.m_Selected.m_SaveData
					.m_iPersistenceCheckpointSequence;
			highestKnownSource = string.Format(
				"profile journal %1 generation %2",
				journalResolution.m_Selected.m_sSlotLabel,
				journalResolution.m_Selected.m_iGeneration);
		}

		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		bool nativeCheckpointRequired = persistence
			&& persistence.GetState() == EPersistenceSystemState.ACTIVE
			&& !m_bProfileFallbackOnlyForSession;
		HST_CampaignPersistentState nativeState;
		if (nativeCheckpointRequired)
		{
			SaveGameManager saveManager = SaveGameManager.Get();
			if (!CanRequestSavePoint(saveManager))
			{
				if (!saveManager)
					evidence = "native save manager is unavailable";
				else
				{
					evidence = string.Format(
						"native checkpoint is not ready | enabled/busy/allowed %1/%2/%3",
						saveManager.IsSavingEnabled(),
						saveManager.IsBusy(),
						saveManager.IsSavingAllowed());
				}
				return false;
			}

			nativeState = ResolveNativeCampaignState(persistence);
			if (!nativeState)
			{
				evidence = "native campaign state proxy is unavailable";
				return false;
			}
			HST_CampaignSaveData nativeSnapshot = nativeState.GetSnapshot();
			if (nativeSnapshot
				&& nativeSnapshot.m_iPersistenceCheckpointSequence
					> highestKnownCheckpointSequence)
			{
				highestKnownCheckpointSequence
					= nativeSnapshot.m_iPersistenceCheckpointSequence;
				highestKnownSource = "native campaign state proxy";
			}
		}

		if (detachedSnapshot.m_iPersistenceCheckpointSequence
			<= highestKnownCheckpointSequence)
		{
			evidence = string.Format(
				"prepared checkpoint sequence %1 does not supersede known sequence %2 from %3",
				detachedSnapshot.m_iPersistenceCheckpointSequence,
				highestKnownCheckpointSequence,
				highestKnownSource);
			return false;
		}
		if (nativeCheckpointRequired)
		{
			if (!persistence.IsTracked(nativeState)
				&& !persistence.StartTracking(nativeState, false))
			{
				evidence = "native campaign state proxy could not be tracked";
				return false;
			}
			if (!persistence.IsTracked(nativeState)
				|| persistence.GetConfig(nativeState) == null)
			{
				evidence
					= "native campaign state proxy is not tracked and configured";
				return false;
			}
		}

		evidence = string.Format(
			"prepared checkpoint readiness passed | known checkpoint floor/source %1/%2 | native required %3 | %4",
			highestKnownCheckpointSequence,
			highestKnownSource,
			nativeCheckpointRequired,
			journalEvidence);
		return true;
	}

	HST_PersistenceCheckpointRequest RequestShutdownCheckpointDetailed(
		HST_CampaignState state,
		SaveGameOperationCallback completionObserver = null)
	{
		ESaveGameRequestFlags requestFlags;
		// Stock save UI uses BLOCKING only without replication. A controlled
		// multiplayer end already owns quiescence and waits for the callback, so
		// keep its save asynchronous instead of asking the dedicated server to
		// synchronously flush a live replicated session.
		if (RplSession.Mode() == RplMode.None)
			requestFlags = ESaveGameRequestFlags.BLOCKING;
		return RequestTypedCheckpointDetailed(
			"Partisan controlled shutdown",
			ESaveGameType.SHUTDOWN,
			requestFlags,
			state,
			completionObserver);
	}

	protected void ReleasePreparedCheckpointContext(
		HST_PersistenceCheckpointCallbackContext callbackContext)
	{
		if (!callbackContext)
			return;
		callbackContext.m_bPreparedDetachedCheckpoint = false;
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
		bool preparedCheckpoint
			= callbackContext.m_bPreparedDetachedCheckpoint;
#ifdef ENABLE_DIAG
		bool campaignDebugRestoreCheckpoint
			= callbackContext.m_bCampaignDebugRestoreCheckpoint;
		if (campaignDebugRestoreCheckpoint
			&& (pendingSaveData != m_CampaignDebugRestoreCandidateSave
				|| HST_CampaignPersistentState.BuildSnapshotFingerprint(
					pendingSaveData)
					!= m_sCampaignDebugRestoreCandidateFingerprint))
		{
			success = false;
		}
#endif
		// A verified journal receipt is cumulative transaction authority. Native
		// completion must not rewrite that same immutable candidate or regress the
		// completed channel if the redundant write fails.
		bool profileMirrorSaved;
#ifdef ENABLE_DIAG
		bool campaignDebugProfileJournalAlreadyComplete
			= campaignDebugRestoreCheckpoint
				&& m_bCampaignDebugRestoreProfileJournalComplete;
		profileMirrorSaved = campaignDebugProfileJournalAlreadyComplete;
#endif
		if (preparedCheckpoint && request)
			profileMirrorSaved = request.m_bProfileFallbackSaved;
		else if (success
#ifdef ENABLE_DIAG
			&& !campaignDebugProfileJournalAlreadyComplete)
#else
			)
#endif
			profileMirrorSaved = SaveProfileFallback(pendingSaveData);
		bool durableSuccess;
		if (preparedCheckpoint)
			durableSuccess = profileMirrorSaved;
		else
			durableSuccess = success && profileMirrorSaved;
#ifdef ENABLE_DIAG
		if (campaignDebugRestoreCheckpoint)
		{
			m_bCampaignDebugRestoreNativeCommitComplete = success;
			if (!success)
				m_bCampaignDebugRestoreNativeStageComplete = false;
			m_bCampaignDebugRestoreProfileJournalComplete
				= m_bCampaignDebugRestoreProfileJournalComplete
					|| profileMirrorSaved;
		}
#endif
		if (request)
		{
			request.m_bCompletionReceived = true;
			request.m_bNativeCommitSucceeded = success;
			request.m_bProfileFallbackSaved = profileMirrorSaved;
			if (preparedCheckpoint)
			{
				request.m_sEvidence += string.Format(
					" | native replication completed %1 after verified write-ahead journal commit %2",
					success,
					profileMirrorSaved);
			}
			else
			{
				request.m_sEvidence += string.Format(
					" | native commit completed %1 | post-commit profile mirror %2",
					success,
					profileMirrorSaved);
			}
			if (!success)
			{
				request.m_sEvidence
					+= " | native storage commit callback reported failure";
			}
			else if (!profileMirrorSaved
				&& !m_sProfileFallbackStatus.IsEmpty())
			{
				request.m_sEvidence += " | " + m_sProfileFallbackStatus;
			}
		}
		if (!success)
		{
			SaveGameManager saveManager = SaveGameManager.Get();
			PersistenceSystem persistence = PersistenceSystem.GetInstance();
			ESaveGameType requestSaveType;
			ESaveGameRequestFlags requestFlags;
			EPersistenceSystemState persistenceState;
			bool persistenceDataLoaded;
			bool transientStateStaged;
			if (request)
			{
				requestSaveType = request.m_eSaveType;
				requestFlags = request.m_eRequestFlags;
				transientStateStaged = request.m_bTransientStateStaged;
			}
			if (persistence)
			{
				persistenceState = persistence.GetState();
				persistenceDataLoaded = persistence.WasDataLoaded();
			}
			bool nativeTracked = persistence && m_NativeCampaignState
				&& persistence.IsTracked(m_NativeCampaignState);
			bool nativeConfigured = nativeTracked
				&& persistence.GetConfig(m_NativeCampaignState) != null;
			string snapshotFingerprint;
			if (m_NativeCampaignState)
				snapshotFingerprint
					= m_NativeCampaignState.GetSnapshotFingerprint();
			int saveTypes;
			int playthrough = -1;
			bool savingEnabled;
			bool savingAllowed;
			bool saveManagerBusy;
			bool activeSavePresent;
			if (saveManager)
			{
				saveTypes = saveManager.GetEnabledSaveTypes();
				playthrough = saveManager.GetCurrentPlaythroughNumber();
				savingEnabled = saveManager.IsSavingEnabled();
				savingAllowed = saveManager.IsSavingAllowed();
				saveManagerBusy = saveManager.IsBusy();
				activeSavePresent = saveManager.GetActiveSave() != null;
			}
			string failureDetail = string.Format(
				"Partisan persistence | native save callback failure | sequence/type/flags %1/%2/%3 | manager/enabled/allowed/busy/active/playthrough %4/%5/%6/%7/%8/%9",
				m_iCheckpointRequestSequence,
				requestSaveType,
				requestFlags,
				saveManager != null,
				savingEnabled,
				savingAllowed,
				saveManagerBusy,
				activeSavePresent,
				playthrough);
			failureDetail += string.Format(
				" | types/persistence/state/loaded/tracked/config/staged %1/%2/%3/%4/%5/%6/%7 | replication mode %8 | snapshot fingerprint %9",
				saveTypes,
				persistence != null,
				persistenceState,
				persistenceDataLoaded,
				nativeTracked,
				nativeConfigured,
				transientStateStaged,
				RplSession.Mode(),
				snapshotFingerprint);
			Print(failureDetail, LogLevel.ERROR);
		}
		if (preparedCheckpoint)
			ReleasePreparedCheckpointContext(callbackContext);
		if (!success || !profileMirrorSaved)
			EnsureMajorChangePending();
		if (state
#ifdef ENABLE_DIAG
			&& !campaignDebugRestoreCheckpoint
#endif
		)
		{
			if (preparedCheckpoint)
			{
				state.m_sLastPersistenceStatus += string.Format(
					" | native replication %1 | write-ahead journal committed %2",
					success,
					profileMirrorSaved);
			}
			else
			{
				state.m_sLastPersistenceStatus += string.Format(
					" | native commit %1 | post-commit profile mirror %2",
					success,
					profileMirrorSaved);
			}
		}
#ifdef ENABLE_DIAG
		if (campaignDebugRestoreCheckpoint
			&& m_LastCampaignDebugPersistenceRestoreResult)
		{
			string restoreCallbackEvidence
				= "native Campaign Debug restore callback completed";
			if (request)
				restoreCallbackEvidence = request.m_sEvidence;
			PopulateCampaignDebugPersistenceRestoreResult(
				m_LastCampaignDebugPersistenceRestoreResult,
				restoreCallbackEvidence);
		}
#endif
		ClearPendingCheckpointRequest();
		if (observer)
			observer.InvokeDelegate(durableSuccess);
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
		bool preparedCheckpoint = m_CheckpointCallbackContext
			&& m_CheckpointCallbackContext.m_bPreparedDetachedCheckpoint;
#ifdef ENABLE_DIAG
		bool campaignDebugRestoreCheckpoint = m_CheckpointCallbackContext
			&& m_CheckpointCallbackContext.m_bCampaignDebugRestoreCheckpoint;
		if (campaignDebugRestoreCheckpoint)
		{
			m_bCampaignDebugRestoreNativeStageComplete = false;
			m_bCampaignDebugRestoreNativeCommitComplete = false;
		}
#endif
		if (preparedCheckpoint)
			ReleasePreparedCheckpointContext(m_CheckpointCallbackContext);
		if (request)
		{
			request.m_bCompletionReceived = true;
			request.m_bNativeCommitSucceeded = false;
			if (preparedCheckpoint)
			{
				request.m_sEvidence += string.Format(
					" | native replication timed out after %1 seconds; verified write-ahead journal remains authoritative and native repair is pending",
					CHECKPOINT_COMMIT_TIMEOUT_SECONDS);
			}
			else
			{
				request.m_sEvidence += string.Format(
					" | native commit timed out after %1 seconds",
					CHECKPOINT_COMMIT_TIMEOUT_SECONDS);
			}
		}
		if (state
#ifdef ENABLE_DIAG
			&& !campaignDebugRestoreCheckpoint
#endif
		)
		{
			if (preparedCheckpoint)
			{
				state.m_sLastPersistenceStatus += string.Format(
					" | native replication timed out after %1 seconds; journal authority retained",
					CHECKPOINT_COMMIT_TIMEOUT_SECONDS);
			}
			else
			{
				state.m_sLastPersistenceStatus += string.Format(
					" | native commit timed out after %1 seconds",
					CHECKPOINT_COMMIT_TIMEOUT_SECONDS);
			}
		}
		EnsureMajorChangePending();
#ifdef ENABLE_DIAG
		if (campaignDebugRestoreCheckpoint
			&& m_LastCampaignDebugPersistenceRestoreResult)
		{
			string timeoutEvidence
				= "native Campaign Debug restore checkpoint timed out";
			if (request)
				timeoutEvidence = request.m_sEvidence;
			PopulateCampaignDebugPersistenceRestoreResult(
				m_LastCampaignDebugPersistenceRestoreResult,
				timeoutEvidence);
		}
#endif
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

	bool CanAcceptImmediateCheckpoint(out string evidence)
	{
		evidence = "immediate checkpoint is available";
		if (m_bCheckpointSavePointInFlight)
		{
			evidence = "a native checkpoint is already in flight";
			return false;
		}
#ifdef ENABLE_DIAG
		if (m_bCampaignDebugIsolationActive)
		{
			evidence
				= "campaign debug isolation cannot commit an administrative reset";
			return false;
		}
#endif
		if (!m_Civilians)
		{
			evidence
				= "ambient civilian persistence authority is unavailable";
			return false;
		}

		string journalEvidence;
		if (!m_ProfileJournal.CanAdvanceVerifiedSnapshot(journalEvidence))
		{
			evidence = journalEvidence;
			return false;
		}

		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		bool nativeCheckpointExpected = persistence
			&& persistence.GetState() == EPersistenceSystemState.ACTIVE
			&& !m_bProfileFallbackOnlyForSession;
		if (!nativeCheckpointExpected)
		{
			evidence = "a synchronous profile-journal checkpoint is available";
			return true;
		}

		SaveGameManager saveManager = SaveGameManager.Get();
		if (!CanRequestSavePoint(saveManager))
		{
			if (!saveManager)
				evidence = "native save manager is unavailable";
			else
			{
				evidence = string.Format(
					"native checkpoint is not ready | enabled/busy/allowed %1/%2/%3",
					saveManager.IsSavingEnabled(),
					saveManager.IsBusy(),
					saveManager.IsSavingAllowed());
			}
			return false;
		}
		return true;
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
		bool preparedCheckpoint = m_CheckpointCallbackContext
			&& m_CheckpointCallbackContext.m_bPreparedDetachedCheckpoint;
#ifdef ENABLE_DIAG
		bool campaignDebugRestoreCheckpoint = m_CheckpointCallbackContext
			&& m_CheckpointCallbackContext.m_bCampaignDebugRestoreCheckpoint;
		if (campaignDebugRestoreCheckpoint)
		{
			m_bCampaignDebugRestoreNativeStageComplete = false;
			m_bCampaignDebugRestoreNativeCommitComplete = false;
		}
#endif
		if (preparedCheckpoint)
			ReleasePreparedCheckpointContext(m_CheckpointCallbackContext);
		m_iCheckpointRequestSequence++;
		if (m_PendingCheckpointRequest)
		{
			m_PendingCheckpointRequest.m_bCompletionReceived = false;
			m_PendingCheckpointRequest.m_bNativeCommitSucceeded = false;
			if (preparedCheckpoint)
			{
				m_PendingCheckpointRequest.m_sEvidence
					+= " | native replication observation cancelled during teardown; verified write-ahead journal remains authoritative";
			}
			else
			{
				m_PendingCheckpointRequest.m_sEvidence
					+= " | native commit observation cancelled during teardown";
			}
		}
		if (m_PendingCheckpointState
#ifdef ENABLE_DIAG
			&& !campaignDebugRestoreCheckpoint
#endif
		)
		{
			if (preparedCheckpoint)
			{
				m_PendingCheckpointState.m_sLastPersistenceStatus
					+= " | native replication observation cancelled during teardown; journal authority retained";
			}
			else
			{
				m_PendingCheckpointState.m_sLastPersistenceStatus
					+= " | native commit observation cancelled during teardown";
			}
		}
#ifdef ENABLE_DIAG
		if (campaignDebugRestoreCheckpoint
			&& m_LastCampaignDebugPersistenceRestoreResult)
		{
			string cancellationEvidence
				= "native Campaign Debug restore checkpoint observation cancelled; recovery authority retained";
			if (m_PendingCheckpointRequest)
				cancellationEvidence = m_PendingCheckpointRequest.m_sEvidence;
			PopulateCampaignDebugPersistenceRestoreResult(
				m_LastCampaignDebugPersistenceRestoreResult,
				cancellationEvidence);
		}
#endif
		ClearPendingCheckpointRequest();
	}

#ifdef ENABLE_DIAG
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
#endif

	void CaptureState(HST_CampaignState state)
	{
#ifdef ENABLE_DIAG
		if (m_bCampaignDebugIsolationActive)
		{
			CaptureIsolatedCampaignDebugState(state);
			return;
		}
#endif

		CaptureAndTrackState(state);
	}

#ifdef ENABLE_DIAG
	bool PrepareCampaignDebugIsolation(HST_CampaignState state)
	{
		if (!state)
			return false;
		if (m_bCampaignDebugIsolationActive
			|| m_CampaignDebugRestoreCandidateSave)
		{
			state.m_sLastPersistenceStatus
				= "campaign debug isolation rejected while prior isolation or restore authority remains active";
			return false;
		}
		if (m_bCheckpointSavePointInFlight)
		{
			state.m_sLastPersistenceStatus
				= "campaign debug isolation deferred while a native checkpoint is in flight";
			return false;
		}
		// A receipt describes one immutable isolation generation only. Clear the
		// prior generation before any new baseline work can create authority.
		m_LastCampaignDebugPersistenceRestoreResult = null;
		if (!PrepareStateForCapture(state, "campaign debug isolation baseline"))
			return false;

		HST_CampaignSaveData preparedSave = new HST_CampaignSaveData();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		if (!TryAdvancePersistenceCheckpointSequence(
			state,
			"campaign debug isolation baseline"))
			return false;
		state.m_sLastPersistenceStatus
			= "campaign debug isolation baseline prepared";
		preparedSave.Capture(state);
		string preparedFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(
				preparedSave);
		if (preparedFingerprint.IsEmpty())
			return false;

		// One immutable monotonic candidate owns every external attempt. A false
		// channel result can still follow partial native staging or a partial
		// journal write, so the candidate must never be compensated with an older
		// checkpoint order. Any incomplete attempt is retained and forward-completed
		// through the ordinary exact restore transaction.
		bool nativeStageRequired = IsNativeCampaignSaveChannelRequired();
		bool scriptedStateSaved = !nativeStageRequired;
		if (nativeStageRequired)
			scriptedStateSaved
				= FlushCampaignSaveData(
					preparedSave,
					ESaveGameType.MANUAL);
		bool profileFallbackSaved = SaveProfileFallback(preparedSave);
		m_TrackedCampaignSave = preparedSave;
		m_LastCapturedSave = preparedSave;
		m_IsolatedCapturedSave = null;
		m_bCampaignDebugIsolationNativeStageRequired
			= nativeStageRequired;
		m_bCampaignDebugIsolationActive = true;
		m_bMajorChangePending = false;
		m_fMajorChangeElapsed = 0;
		m_fAutosaveElapsed = 0;

		if (scriptedStateSaved && profileFallbackSaved)
		{
			ResetCampaignDebugPersistenceRestoreAttempt();
			return true;
		}

		ResetCampaignDebugPersistenceRestoreAttempt();
		m_CampaignDebugRestoreCandidateSave = preparedSave;
		m_CampaignDebugRestoreStateAuthority = state;
		m_bCampaignDebugRestoreNativeStageRequired
			= nativeStageRequired;
		m_bCampaignDebugRestoreNativeStageComplete
			= scriptedStateSaved;
		m_bCampaignDebugRestoreNativeCommitRequired
			= nativeStageRequired;
		m_bCampaignDebugRestoreNativeCommitComplete = false;
		m_bCampaignDebugRestoreProfileJournalComplete
			= profileFallbackSaved;
		m_sCampaignDebugRestoreCandidateFingerprint
			= preparedFingerprint;
		return false;
	}

	bool HasCampaignDebugPersistenceIsolationAuthority()
	{
		return m_bCampaignDebugIsolationActive
			&& m_TrackedCampaignSave != null;
	}

	bool CaptureIsolatedCampaignDebugState(HST_CampaignState state, string persistenceStatus = "isolated campaign debug checkpoint")
	{
		if (!state)
			return false;
		if (m_bCheckpointSavePointInFlight)
		{
			state.m_sLastPersistenceStatus
				= "isolated capture deferred while a native checkpoint is in flight";
			return false;
		}
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
		if (!TryAdvancePersistenceCheckpointSequence(state, captureStatus))
		{
			evidence = state.m_sLastPersistenceStatus;
			return false;
		}
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
		HST_CampaignSaveData saveData;
		HST_CampaignProfileSaveResolution resolution;
		string journalEvidence;
		if (!m_ProfileJournal.ReadSelectedSnapshot(
			saveData,
			resolution,
			journalEvidence))
		{
			evidence = "profile fallback proof journal read failed | "
				+ journalEvidence;
			return false;
		}
		m_LastProfileJournalResolution = resolution;
		if (!HST_CampaignPersistentState.IsSnapshotSchemaSupported(saveData))
		{
			evidence = string.Format(
				"profile fallback proof snapshot schema %1 is unsupported",
				saveData.m_iSchemaVersion);
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
			"journal readback restored | slot/generation %1/%2 | valid/chain/legacy %3/%4/%5",
			resolution.m_Selected.m_sSlotLabel,
			resolution.m_Selected.m_iGeneration,
			resolution.m_iValidCandidateCount,
			resolution.m_bChainExact,
			resolution.m_Selected.m_bLegacyRaw);
		evidence += string.Format(
			" | stored/loaded/current schema %1/%2/%3 | save second %4 | status %5",
			storedSchema,
			readBackState.m_iLastLoadedSchemaVersion,
			readBackState.m_iSchemaVersion,
			storedLastSaveSecond,
			storedStatus);
		evidence += string.Format(
			" | qrf %1 | operations %2 | orders %3 | mutations %4",
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
#endif

	// Read-only production proof observation. The fingerprint is the exact stored
	// payload fingerprint selected by the journal before Restore mutates the DTO.
	bool ReadCanonicalProfileFallbackSnapshot(
		out HST_CampaignState readBackState,
		out string snapshotFingerprint,
		out string evidence)
	{
		readBackState = null;
		snapshotFingerprint = "";
		evidence = "profile journal observation was not attempted";
		HST_CampaignSaveData saveData;
		HST_CampaignProfileSaveResolution resolution;
		string journalEvidence;
		if (!m_ProfileJournal.ReadSelectedSnapshot(
			saveData,
			resolution,
			journalEvidence))
		{
			evidence = "profile journal observation failed | " + journalEvidence;
			return false;
		}
		m_LastProfileJournalResolution = resolution;
		if (!HST_CampaignPersistentState.IsSnapshotSchemaSupported(saveData))
		{
			evidence = string.Format(
				"profile journal selected unsupported schema %1",
				saveData.m_iSchemaVersion);
			return false;
		}
		snapshotFingerprint = resolution.m_Selected.m_sSnapshotFingerprint;
		readBackState = saveData.Restore();
		if (snapshotFingerprint.IsEmpty() || !readBackState
			|| readBackState.m_iSchemaVersion
				!= HST_CampaignState.SCHEMA_VERSION)
		{
			evidence = string.Format(
				"profile journal validation failed | fingerprint %1 | restored %2",
				!snapshotFingerprint.IsEmpty(),
				readBackState != null);
			return false;
		}
		evidence = string.Format(
			"profile journal observed | schema %1 | fingerprint %2 | slot/generation %3/%4 | valid/chain/legacy %5/%6/%7",
			readBackState.m_iSchemaVersion,
			snapshotFingerprint,
			resolution.m_Selected.m_sSlotLabel,
			resolution.m_Selected.m_iGeneration,
			resolution.m_iValidCandidateCount,
			resolution.m_bChainExact,
			resolution.m_Selected.m_bLegacyRaw);
		return true;
	}

	HST_CampaignProfileSaveResolution GetLastProfileJournalResolution()
	{
		return m_LastProfileJournalResolution;
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

#ifdef ENABLE_DIAG
	protected bool IsCampaignDebugRestoreStateAuthorityExact(
		HST_CampaignState state,
		HST_CampaignSaveData candidate,
		out string evidence)
	{
		evidence = "Campaign Debug restore state/candidate comparison rejected";
		if (!state || !candidate)
			return false;
		HST_CampaignSaveData current = new HST_CampaignSaveData();
		current.Capture(state);
		string candidatePayload;
		string candidateFingerprint;
		string currentPayload;
		string currentFingerprint;
		if (!HST_CampaignPersistentState.TrySerializeSnapshot(
			candidate,
			candidatePayload,
			candidateFingerprint)
			|| !HST_CampaignPersistentState.TrySerializeSnapshot(
				current,
				currentPayload,
				currentFingerprint))
			return false;
		bool exact = !candidatePayload.IsEmpty()
			&& candidatePayload == currentPayload
			&& candidateFingerprint == currentFingerprint;
		evidence = string.Format(
			"Campaign Debug restore state/candidate exact %1 | candidate/current %2/%3",
			exact,
			candidateFingerprint,
			currentFingerprint);
		return exact;
	}

	bool RestoreTrackedStateAfterCampaignDebug(HST_CampaignState state)
	{
		HST_CampaignDebugPersistenceRestoreResult result
			= new HST_CampaignDebugPersistenceRestoreResult();
		m_LastCampaignDebugPersistenceRestoreResult = result;
		result.m_bAttempted = true;
		result.m_bIsolationAuthorityPresent
			= m_bCampaignDebugIsolationActive;
		if (!state)
		{
			PopulateCampaignDebugPersistenceRestoreResult(
				result,
				"tracked-state restore rejected: live state authority is null");
			return false;
		}
		if (m_bCheckpointSavePointInFlight)
		{
			PopulateCampaignDebugPersistenceRestoreResult(
				result,
				"tracked-state restore deferred while a native checkpoint is in flight");
			return false;
		}
		if (!m_bCampaignDebugIsolationActive)
		{
			PopulateCampaignDebugPersistenceRestoreResult(
				result,
				"tracked-state restore rejected: Campaign Debug persistence isolation authority is not active");
			return false;
		}

		if (m_CampaignDebugRestoreCandidateSave)
		{
			if (state != m_CampaignDebugRestoreStateAuthority)
			{
				PopulateCampaignDebugPersistenceRestoreResult(
					result,
					"tracked-state restore rejected: retry did not provide the frozen live-state authority");
				return false;
			}
			string retryCandidateFingerprint
				= HST_CampaignPersistentState.BuildSnapshotFingerprint(
					m_CampaignDebugRestoreCandidateSave);
			if (retryCandidateFingerprint.IsEmpty()
				|| retryCandidateFingerprint
					!= m_sCampaignDebugRestoreCandidateFingerprint)
			{
				PopulateCampaignDebugPersistenceRestoreResult(
					result,
					"tracked-state restore rejected: frozen candidate identity changed; recovery authority retained");
				return false;
			}
			string retryStateEvidence;
			if (!IsCampaignDebugRestoreStateAuthorityExact(
				state,
				m_CampaignDebugRestoreCandidateSave,
				retryStateEvidence))
			{
				PopulateCampaignDebugPersistenceRestoreResult(
					result,
					"tracked-state restore rejected: live state changed after the frozen candidate was selected | "
						+ retryStateEvidence);
				return false;
			}
		}
		else
		{
			if (!m_TrackedCampaignSave)
			{
				PopulateCampaignDebugPersistenceRestoreResult(
					result,
					"tracked-state restore rejected: immutable pre-run save is unavailable");
				return false;
			}
			string baselineEvidence;
			if (!IsCampaignDebugRestoreStateAuthorityExact(
				state,
				m_TrackedCampaignSave,
				baselineEvidence))
			{
				PopulateCampaignDebugPersistenceRestoreResult(
					result,
					"tracked-state restore rejected: live state does not match the immutable pre-run save | "
						+ baselineEvidence);
				return false;
			}
			m_CampaignDebugRestoreCandidateSave = m_TrackedCampaignSave;
			m_sCampaignDebugRestoreCandidateFingerprint
				= HST_CampaignPersistentState.BuildSnapshotFingerprint(
					m_CampaignDebugRestoreCandidateSave);
			if (m_sCampaignDebugRestoreCandidateFingerprint.IsEmpty())
			{
				ResetCampaignDebugPersistenceRestoreAttempt();
				PopulateCampaignDebugPersistenceRestoreResult(
					result,
					"tracked-state restore candidate serialization failed; pre-run and isolated recovery authority retained");
				return false;
			}
			m_CampaignDebugRestoreStateAuthority = state;
			m_bCampaignDebugRestoreNativeStageRequired
				= m_bCampaignDebugIsolationNativeStageRequired
					|| IsNativeCampaignSaveChannelRequired();
			m_bCampaignDebugRestoreNativeCommitRequired
				= m_bCampaignDebugRestoreNativeStageRequired;
			m_bCampaignDebugRestoreNativeStageComplete = false;
			m_bCampaignDebugRestoreNativeCommitComplete = false;
			m_bCampaignDebugRestoreProfileJournalComplete = false;
		}

		if (m_bCampaignDebugRestoreNativeStageRequired
			&& !m_bCampaignDebugRestoreNativeStageComplete)
		{
			m_bCampaignDebugRestoreNativeStageComplete
				= FlushCampaignSaveData(
					m_CampaignDebugRestoreCandidateSave,
					ESaveGameType.MANUAL);
		}
		if (m_bCampaignDebugRestoreNativeCommitRequired
			&& !m_bCampaignDebugRestoreNativeStageComplete)
		{
			string nativeStageStatus
				= BuildCampaignDebugPersistenceRestoreChannelStatus(
					"restore retained: native transient staging failed");
			PopulateCampaignDebugPersistenceRestoreResult(
				result,
				nativeStageStatus);
			return false;
		}
		if (m_bCampaignDebugRestoreNativeCommitRequired
			&& !m_bCampaignDebugRestoreNativeCommitComplete)
		{
			string checkpointEvidence;
			if (!QueueCampaignDebugPersistenceRestoreCheckpoint(
				state,
				checkpointEvidence))
			{
				string checkpointStatus
					= BuildCampaignDebugPersistenceRestoreChannelStatus(
						"restore retained: " + checkpointEvidence);
				PopulateCampaignDebugPersistenceRestoreResult(
					result,
					checkpointStatus);
				return false;
			}
			if (!m_bCampaignDebugRestoreNativeCommitComplete)
			{
				string nativeCommitStatus
					= "restore retained: native storage commit did not complete";
				if (m_bCheckpointSavePointInFlight)
					nativeCommitStatus
						= "restore retained: native storage commit pending";
				string pendingCommitStatus
					= BuildCampaignDebugPersistenceRestoreChannelStatus(
						nativeCommitStatus);
				PopulateCampaignDebugPersistenceRestoreResult(
					result,
					pendingCommitStatus);
				return false;
			}
		}
		if ((!m_bCampaignDebugRestoreNativeCommitRequired
				|| m_bCampaignDebugRestoreNativeCommitComplete)
			&& !m_bCampaignDebugRestoreProfileJournalComplete)
		{
			m_bCampaignDebugRestoreProfileJournalComplete
				= SaveProfileFallback(
					m_CampaignDebugRestoreCandidateSave);
		}

		bool requiredChannelsExact
			= m_bCampaignDebugRestoreProfileJournalComplete
				&& (!m_bCampaignDebugRestoreNativeStageRequired
					|| m_bCampaignDebugRestoreNativeStageComplete)
				&& (!m_bCampaignDebugRestoreNativeCommitRequired
					|| m_bCampaignDebugRestoreNativeCommitComplete);
		if (!requiredChannelsExact)
		{
			string incompleteStatus
				= BuildCampaignDebugPersistenceRestoreChannelStatus(
					"restore retained: required persistence channel incomplete");
			PopulateCampaignDebugPersistenceRestoreResult(
				result,
				incompleteStatus);
			return false;
		}

		string committedFingerprint
			= m_sCampaignDebugRestoreCandidateFingerprint;
		m_TrackedCampaignSave = m_CampaignDebugRestoreCandidateSave;
		m_LastCapturedSave = m_CampaignDebugRestoreCandidateSave;
		m_IsolatedCapturedSave = null;
		m_bCampaignDebugIsolationActive = false;
		m_bCampaignDebugIsolationNativeStageRequired = false;
		m_bMajorChangePending = false;
		m_fMajorChangeElapsed = 0;
		m_fAutosaveElapsed = 0;
		string exactStatus
			= BuildCampaignDebugPersistenceRestoreChannelStatus(
				"restore exact: required persistence channels committed");
		result.m_bCandidatePrepared = true;
		result.m_bNativeStageRequired
			= m_bCampaignDebugRestoreNativeStageRequired;
		result.m_bNativeStageComplete
			= m_bCampaignDebugRestoreNativeStageComplete;
		result.m_bNativeCommitRequired
			= m_bCampaignDebugRestoreNativeCommitRequired;
		result.m_bNativeCommitComplete
			= m_bCampaignDebugRestoreNativeCommitComplete;
		result.m_bProfileJournalComplete
			= m_bCampaignDebugRestoreProfileJournalComplete;
		result.m_sCandidateFingerprint = committedFingerprint;
		result.m_bRestoreComplete = true;
		result.m_bRecoveryAuthorityRetained = false;
		result.m_sEvidence = exactStatus;
		ResetCampaignDebugPersistenceRestoreAttempt();
		return result.AllRequiredChannelsExact();
	}

	HST_CampaignDebugPersistenceRestoreResult GetLastCampaignDebugPersistenceRestoreResult()
	{
		return m_LastCampaignDebugPersistenceRestoreResult;
	}

	bool HasPendingCampaignDebugPersistenceRestore()
	{
		return m_bCampaignDebugIsolationActive
			&& m_CampaignDebugRestoreCandidateSave != null;
	}

	bool HasRetainedCampaignDebugPersistenceRestoreAuthority()
	{
		return m_bCampaignDebugIsolationActive
			&& m_CampaignDebugRestoreCandidateSave
			&& m_LastCampaignDebugPersistenceRestoreResult
			&& m_LastCampaignDebugPersistenceRestoreResult.m_bAttempted
			&& m_LastCampaignDebugPersistenceRestoreResult.m_sCandidateFingerprint
				== m_sCampaignDebugRestoreCandidateFingerprint;
	}

	bool CanRetryCampaignDebugPersistenceRestore()
	{
		return HasRetainedCampaignDebugPersistenceRestoreAuthority()
			&& !m_bCheckpointSavePointInFlight;
	}

	string BuildLastCampaignDebugPersistenceRestoreReport()
	{
		if (!m_LastCampaignDebugPersistenceRestoreResult)
			return "Campaign Debug persistence restore has not been attempted";
		return m_LastCampaignDebugPersistenceRestoreResult.BuildReport();
	}

	protected void PopulateCampaignDebugPersistenceRestoreResult(
		HST_CampaignDebugPersistenceRestoreResult result,
		string evidence)
	{
		if (!result)
			return;
		result.m_bCandidatePrepared
			= m_CampaignDebugRestoreCandidateSave != null;
		result.m_bNativeStageRequired
			= m_bCampaignDebugRestoreNativeStageRequired;
		result.m_bNativeStageComplete
			= m_bCampaignDebugRestoreNativeStageComplete;
		result.m_bNativeCommitRequired
			= m_bCampaignDebugRestoreNativeCommitRequired;
		result.m_bNativeCommitComplete
			= m_bCampaignDebugRestoreNativeCommitComplete;
		result.m_bProfileJournalComplete
			= m_bCampaignDebugRestoreProfileJournalComplete;
		result.m_bRecoveryAuthorityRetained
			= m_bCampaignDebugIsolationActive
				|| m_CampaignDebugRestoreCandidateSave != null;
		result.m_sCandidateFingerprint
			= m_sCampaignDebugRestoreCandidateFingerprint;
		result.m_sEvidence = evidence;
	}

	protected string BuildCampaignDebugPersistenceRestoreChannelStatus(
		string prefix)
	{
		string status = string.Format(
			"%1 | native stage required/complete %2/%3 | native commit required/complete %4/%5",
			prefix,
			m_bCampaignDebugRestoreNativeStageRequired,
			m_bCampaignDebugRestoreNativeStageComplete,
			m_bCampaignDebugRestoreNativeCommitRequired,
			m_bCampaignDebugRestoreNativeCommitComplete);
		return status + string.Format(
			" | profile journal required/complete true/%1 | candidate %2",
			m_bCampaignDebugRestoreProfileJournalComplete,
			m_sCampaignDebugRestoreCandidateFingerprint);
	}

	protected void ResetCampaignDebugPersistenceRestoreAttempt()
	{
		m_CampaignDebugRestoreCandidateSave = null;
		m_CampaignDebugRestoreStateAuthority = null;
		m_bCampaignDebugRestoreNativeStageRequired = false;
		m_bCampaignDebugRestoreNativeStageComplete = false;
		m_bCampaignDebugRestoreNativeCommitRequired = false;
		m_bCampaignDebugRestoreNativeCommitComplete = false;
		m_bCampaignDebugRestoreProfileJournalComplete = false;
		m_sCampaignDebugRestoreCandidateFingerprint = "";
	}

	protected bool QueueCampaignDebugPersistenceRestoreCheckpoint(
		HST_CampaignState state,
		out string evidence)
	{
		evidence = "native Campaign Debug restore checkpoint was not queued";
		if (!state || state != m_CampaignDebugRestoreStateAuthority
			|| !m_CampaignDebugRestoreCandidateSave)
		{
			evidence
				= "frozen Campaign Debug restore authority is unavailable";
			return false;
		}
		if (m_bCheckpointSavePointInFlight)
		{
			evidence = "another native checkpoint is already in flight";
			return false;
		}
		SaveGameManager saveManager = SaveGameManager.Get();
		if (!CanRequestSavePoint(saveManager))
		{
			if (!saveManager)
				evidence = "native save manager is unavailable";
			else
			{
				evidence = string.Format(
					"native save point is not ready | enabled/busy/allowed %1/%2/%3",
					saveManager.IsSavingEnabled(),
					saveManager.IsBusy(),
					saveManager.IsSavingAllowed());
			}
			return false;
		}

		HST_PersistenceCheckpointRequest request
			= new HST_PersistenceCheckpointRequest();
		request.m_eSaveType = ESaveGameType.MANUAL;
		request.m_eRequestFlags = 0;
		request.m_sDisplayName
			= "Partisan Campaign Debug tracked-state restore";
		request.m_bCampaignCaptured = true;
		request.m_bTransientStateStaged
			= m_bCampaignDebugRestoreNativeStageComplete;
		request.m_bSavePointRequested = true;
		request.m_sEvidence
			= "Campaign Debug restore candidate staged; native storage commit requested";
		m_bCheckpointSavePointInFlight = true;
		m_PendingCheckpointRequest = request;
		m_PendingCheckpointSaveData
			= m_CampaignDebugRestoreCandidateSave;
		m_PendingCheckpointObserver = null;
		m_PendingCheckpointState = state;
		m_iCheckpointRequestSequence++;
		m_fCheckpointCommitElapsedSeconds = 0;
		m_CheckpointCallbackContext
			= new HST_PersistenceCheckpointCallbackContext();
		m_CheckpointCallbackContext.m_iRequestSequence
			= m_iCheckpointRequestSequence;
		m_CheckpointCallbackContext.m_bCampaignDebugRestoreCheckpoint
			= true;
		m_CheckpointCompletionCallback = new SaveGameOperationCallback(
			OnCheckpointSavePointCompleted,
			m_CheckpointCallbackContext);
		saveManager.RequestSavePoint(
			ESaveGameType.MANUAL,
			request.m_sDisplayName,
			request.m_eRequestFlags,
			m_CheckpointCompletionCallback);
		evidence = request.m_sEvidence;
		return true;
	}

#endif
	protected bool IsNativeCampaignSaveChannelRequired()
	{
		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		return persistence
			&& persistence.GetState() == EPersistenceSystemState.ACTIVE
			&& !m_bProfileFallbackOnlyForSession;
	}

	protected bool TryAdvancePersistenceCheckpointSequence(
		HST_CampaignState state,
		string context)
	{
		if (!state)
			return false;
		if (state.m_iPersistenceCheckpointSequence >= int.MAX)
		{
			state.m_sLastPersistenceStatus
				= "checkpoint rejected: persistence checkpoint sequence exhausted";
			if (!context.IsEmpty())
				state.m_sLastPersistenceStatus += " during " + context;
			return false;
		}
		state.m_iPersistenceCheckpointSequence
			= Math.Max(0, state.m_iPersistenceCheckpointSequence) + 1;
		return true;
	}

	HST_CampaignSaveData CaptureAndTrackState(HST_CampaignState state, string persistenceStatus = "captured and tracked")
	{
		if (!state)
			return null;
		if (m_bCheckpointSavePointInFlight)
		{
			state.m_sLastPersistenceStatus
				= "capture deferred while a native checkpoint is in flight";
			return null;
		}
#ifdef ENABLE_DIAG
		if (m_bCampaignDebugIsolationActive)
		{
			if (!CaptureIsolatedCampaignDebugState(state, persistenceStatus))
				return null;
			return m_IsolatedCapturedSave;
		}
#endif
		if (!PrepareStateForCapture(state, persistenceStatus))
			return null;
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastSaveSecond = state.m_iElapsedSeconds;
		if (!TryAdvancePersistenceCheckpointSequence(
			state,
			persistenceStatus))
			return null;
		if (!persistenceStatus.IsEmpty())
			state.m_sLastPersistenceStatus = persistenceStatus;
		HST_CampaignSaveData detachedCapture = new HST_CampaignSaveData();
		detachedCapture.Capture(state);
		m_LastCapturedSave = detachedCapture;
		m_TrackedCampaignSave = detachedCapture;
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
		bool requiresExhaustiveInfantrySampling
			= HST_PhysicalWarService.RequiresExhaustiveInfantryPersistenceSampling(
				state);
		if (!m_PhysicalWar)
		{
			if (!hasExactMissionConvoy && !requiresExhaustiveInfantrySampling)
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
		if (!requiresExhaustiveInfantrySampling)
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

	protected void PopulateProfileJournalResolution(
		HST_PersistenceSourceResolution result,
		HST_CampaignProfileSaveResolution journal)
	{
		if (!result || !journal)
			return;
		result.m_bProfileFallbackPresent = journal.m_bArtifactsPresent;
		result.m_iProfileJournalValidSlotCount
			= journal.m_iValidCandidateCount;
		result.m_bProfileJournalChainExact = journal.m_bChainExact;
		if (!journal.m_bHasSelection || !journal.m_Selected)
			return;
		result.m_iProfileJournalGeneration
			= journal.m_Selected.m_iGeneration;
		result.m_sProfileJournalSlot = journal.m_Selected.m_sSlotLabel;
		result.m_bProfileJournalLegacyRaw = journal.m_Selected.m_bLegacyRaw;
	}

	protected HST_PersistenceSourceResolution ResolveProfileFallbackAfterNativeFailure(
			HST_CampaignState fallbackState,
			HST_PersistenceSourceResolution result,
			string nativeFailure,
			bool fallbackOnlyForSession)
	{
		if (fallbackOnlyForSession)
			m_bProfileFallbackOnlyForSession = true;

		HST_CampaignSaveData restoredSave = LoadProfileFallback();
		PopulateProfileJournalResolution(
			result,
			m_LastProfileJournalResolution);
		if (restoredSave && m_LastProfileJournalResolution
			&& m_LastProfileJournalResolution.m_bHasSelection
			&& m_LastProfileJournalResolution.m_Selected)
		{
			result.m_bProfileFallbackRead = true;
			result.m_sProfileFallbackSnapshotFingerprint
				= m_LastProfileJournalResolution.m_Selected
					.m_sSnapshotFingerprint;
			result.m_sSelectedSnapshotFingerprint
				= result.m_sProfileFallbackSnapshotFingerprint;
			if (!result.m_sProfileFallbackSnapshotFingerprint.IsEmpty()
				&& ApplyRestoredCampaignState(fallbackState, restoredSave))
			{
				result.m_eSource
					= HST_ECampaignPersistenceSource
						.HST_PERSISTENCE_SOURCE_PROFILE_FALLBACK;
				result.m_State = fallbackState;
				result.m_bDegradedNativeRecovery = true;
				result.m_sDegradedNativeRecoveryReason = nativeFailure;
				result.m_sEvidence = "degraded native recovery selected profile journal | "
					+ nativeFailure + " | "
					+ m_LastProfileJournalResolution.m_sEvidence;
				m_LastSourceResolution = result;
				return result;
			}
		}

		result.m_eSource
			= HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_FATAL;
		result.m_sEvidence = "native authority failed and no applicable profile journal generation exists | "
			+ nativeFailure + " | " + m_sProfileFallbackStatus;
		m_LastSourceResolution = result;
		return result;
	}

	protected HST_PersistenceSourceResolution RejectUnsupportedFutureNativeAuthority(
		HST_PersistenceSourceResolution result,
		string failure)
	{
		if (!result)
			return null;
		result.m_bNativeRecordPresent = true;
		result.m_bNativeRecordValid = false;
		result.m_bNativeRecordUnsupportedFuture = true;
		result.m_eSource
			= HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_FATAL;
		result.m_sEvidence
			= "unsupported future native campaign authority was preserved and startup failed closed | "
				+ failure;
		m_LastSourceResolution = result;
		return result;
	}

	// Production routing seam that must classify an inspected future native row
	// before any terminal/unusable-native path is allowed to read the journal.
	protected HST_PersistenceSourceResolution ResolveInspectedNativeAuthorityBeforeActiveFlow(
		HST_CampaignState fallbackState,
		HST_PersistenceSourceResolution result,
		EPersistenceSystemState persistenceState,
		bool nativeUnsupportedFuture,
		string nativeFailure)
	{
		if (!result)
			return null;
		if (nativeUnsupportedFuture)
		{
			if (nativeFailure.IsEmpty())
				nativeFailure = "native campaign authority has an unsupported future format";
			return RejectUnsupportedFutureNativeAuthority(
				result,
				nativeFailure);
		}
		if (persistenceState == EPersistenceSystemState.FAILURE
			|| persistenceState == EPersistenceSystemState.SHUTDOWN)
		{
			return ResolveProfileFallbackAfterNativeFailure(
				fallbackState,
				result,
				string.Format(
					"native persistence entered terminal state %1",
					persistenceState),
				true);
		}
		if (persistenceState != EPersistenceSystemState.ACTIVE)
		{
			return ResolveProfileFallbackAfterNativeFailure(
				fallbackState,
				result,
				string.Format(
					"native persistence state %1 is unsupported",
					persistenceState),
				true);
		}
		return null;
	}

	protected int CompareCampaignSnapshotOrder(
		HST_CampaignSaveData first,
		HST_CampaignSaveData second)
	{
		if (!first || !second)
			return 0;
		if (first.m_iPersistenceCheckpointSequence
			< second.m_iPersistenceCheckpointSequence)
			return -1;
		if (first.m_iPersistenceCheckpointSequence
			> second.m_iPersistenceCheckpointSequence)
			return 1;
		if (first.m_iPersistenceRestoreSequence
			< second.m_iPersistenceRestoreSequence)
			return -1;
		if (first.m_iPersistenceRestoreSequence
			> second.m_iPersistenceRestoreSequence)
			return 1;
		if (first.m_iLastSaveSecond < second.m_iLastSaveSecond)
			return -1;
		if (first.m_iLastSaveSecond > second.m_iLastSaveSecond)
			return 1;
		return 0;
	}

	protected HST_PersistenceSourceResolution ResolveValidNativeAuthority(
		HST_CampaignState fallbackState,
		HST_PersistenceSourceResolution result,
		HST_CampaignSaveData nativeSave,
		string nativeFingerprint)
	{
		result.m_sNativeSnapshotFingerprint = nativeFingerprint;
		if (!nativeSave || nativeFingerprint.IsEmpty())
		{
			return ResolveProfileFallbackAfterNativeFailure(
				fallbackState,
				result,
				"valid native campaign record has no applicable snapshot identity",
				false);
		}

		HST_CampaignSaveData profileSave = LoadProfileFallback();
		PopulateProfileJournalResolution(
			result,
			m_LastProfileJournalResolution);
		if (profileSave && m_LastProfileJournalResolution
			&& m_LastProfileJournalResolution.m_bHasSelection
			&& m_LastProfileJournalResolution.m_Selected)
		{
			result.m_bProfileFallbackRead = true;
			result.m_sProfileFallbackSnapshotFingerprint
				= m_LastProfileJournalResolution.m_Selected
					.m_sSnapshotFingerprint;
			string profileComparisonFingerprint
				= result.m_sProfileFallbackSnapshotFingerprint;
			if (m_LastProfileJournalResolution.m_Selected.m_bLegacyRaw)
			{
				// Generation-zero lineage retains the exact legacy file bytes, but
				// native authority was fingerprinted from the normalized snapshot
				// payload. Compare those equivalent serializations so whitespace or
				// legacy file formatting cannot manufacture an equal-order conflict.
				profileComparisonFingerprint
					= HST_CampaignPersistentState.BuildSnapshotFingerprint(
						profileSave);
			}
			int profileOrder = CompareCampaignSnapshotOrder(
				profileSave,
				nativeSave);
			if (profileOrder > 0)
			{
				if (!result.m_sProfileFallbackSnapshotFingerprint.IsEmpty()
					&& ApplyRestoredCampaignState(fallbackState, profileSave))
				{
					result.m_eSource
						= HST_ECampaignPersistenceSource
							.HST_PERSISTENCE_SOURCE_PROFILE_FALLBACK;
					result.m_State = fallbackState;
					result.m_bDegradedNativeRecovery = true;
					result.m_sDegradedNativeRecoveryReason
						= "profile journal ordering is newer than the valid native record";
					result.m_sSelectedSnapshotFingerprint
						= result.m_sProfileFallbackSnapshotFingerprint;
					result.m_sEvidence
						= "newer profile journal selected over a stale valid native record | "
							+ m_LastProfileJournalResolution.m_sEvidence;
					m_LastSourceResolution = result;
					return result;
				}
				return ResolveProfileFallbackAfterNativeFailure(
					fallbackState,
					result,
					"newer profile journal could not be applied",
					false);
			}
			if (profileOrder == 0
				&& profileComparisonFingerprint != nativeFingerprint)
			{
				result.m_eSource
					= HST_ECampaignPersistenceSource
						.HST_PERSISTENCE_SOURCE_FATAL;
				result.m_sEvidence
					= "native and profile journal snapshots have equal durable order but different fingerprints";
				m_LastSourceResolution = result;
				return result;
			}
		}

		result.m_sSelectedSnapshotFingerprint = nativeFingerprint;
		if (!ApplyRestoredCampaignState(fallbackState, nativeSave))
		{
			return ResolveProfileFallbackAfterNativeFailure(
				fallbackState,
				result,
				"native campaign record could not be applied",
				false);
		}
		result.m_eSource
			= HST_ECampaignPersistenceSource
				.HST_PERSISTENCE_SOURCE_NATIVE;
		result.m_State = fallbackState;
		if (profileSave)
		{
			result.m_sEvidence
				= "valid native campaign record selected after durable-order comparison with profile journal";
		}
		else
		{
			result.m_sEvidence
				= "valid native campaign record selected; no valid profile journal generation superseded it";
		}
		m_LastSourceResolution = result;
		return result;
	}

#ifdef ENABLE_DIAG
	HST_PersistenceSourceResolution ResolveProfileFallbackAfterNativeFailureForProof(
		HST_CampaignState fallbackState,
		string proofFailure,
		bool fallbackOnlyForSession = false)
	{
		if (!fallbackState
			|| proofFailure != "journal_autotest_native_failure")
			return null;
		HST_PersistenceSourceResolution result
			= new HST_PersistenceSourceResolution();
		result.m_bPersistenceSystemAvailable = true;
		result.m_bPersistenceSystemLoadedData = true;
		result.m_bNativeRecordPresent = true;
		return ResolveProfileFallbackAfterNativeFailure(
			fallbackState,
			result,
			proofFailure,
			fallbackOnlyForSession);
	}

	HST_PersistenceSourceResolution ResolveValidNativeAuthorityForProof(
		HST_CampaignState fallbackState,
		HST_CampaignSaveData nativeSave,
		string nativeFingerprint)
	{
		if (!fallbackState || !nativeSave || nativeFingerprint.IsEmpty())
			return null;
		HST_PersistenceSourceResolution result
			= new HST_PersistenceSourceResolution();
		result.m_bPersistenceSystemAvailable = true;
		result.m_bPersistenceSystemLoadedData = true;
		result.m_bNativeRecordPresent = true;
		result.m_bNativeRecordValid = true;
		return ResolveValidNativeAuthority(
			fallbackState,
			result,
			nativeSave,
			nativeFingerprint);
	}

	HST_PersistenceSourceResolution ResolveInspectedFutureNativeAuthorityForProof(
		HST_CampaignState fallbackState,
		string failure)
	{
		if (!fallbackState || failure != "journal-autotest-future-native")
			return null;
		HST_PersistenceSourceResolution result
			= new HST_PersistenceSourceResolution();
		result.m_bPersistenceSystemAvailable = true;
		result.m_bPersistenceSystemLoadedData = true;
		result.m_bNativeRecordPresent = true;
		result.m_bNativeRecordUnsupportedFuture = true;
		return ResolveInspectedNativeAuthorityBeforeActiveFlow(
			fallbackState,
			result,
			EPersistenceSystemState.FAILURE,
			true,
			failure);
	}

	bool SimulateFailedNativeCheckpointCallbackForProof(
		HST_CampaignState state,
		HST_CampaignSaveData pendingSaveData,
		out HST_PersistenceCheckpointRequest request)
	{
		request = null;
		if (!state || !pendingSaveData || m_bCheckpointSavePointInFlight)
			return false;

		m_bMajorChangePending = false;
		m_fMajorChangeElapsed = 0;
		request = new HST_PersistenceCheckpointRequest();
		request.m_bCampaignCaptured = true;
		request.m_bTransientStateStaged = true;
		request.m_bSavePointRequested = true;
		m_PendingCheckpointRequest = request;
		m_PendingCheckpointSaveData = pendingSaveData;
		m_PendingCheckpointState = state;
		m_iCheckpointRequestSequence++;
		m_bCheckpointSavePointInFlight = true;
		HST_PersistenceCheckpointCallbackContext callbackContext
			= new HST_PersistenceCheckpointCallbackContext();
		callbackContext.m_iRequestSequence = m_iCheckpointRequestSequence;
		OnCheckpointSavePointCompleted(false, callbackContext);
		return request.m_bCompletionReceived
			&& !request.m_bNativeCommitSucceeded
			&& !request.m_bProfileFallbackSaved
			&& !m_bCheckpointSavePointInFlight
			&& m_bMajorChangePending;
	}
#endif

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
		if (!persistence)
		{
			result.m_sEvidence
				= "native persistence system is not available yet";
			m_LastSourceResolution = result;
			return result;
		}
		if (persistence)
		{
			result.m_bPersistenceSystemAvailable = true;
			result.m_bPersistenceSystemLoadedData = persistence.WasDataLoaded();
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

			// Inspect a typed future row before treating a terminal persistence
			// state as generic native failure. A newer game/mod build may make the
			// native system fail setup; that must never authorize an older JSON
			// generation to overwrite the newer native authority.
			m_NativeCampaignState = ResolveNativeCampaignState(persistence);
			if (m_NativeCampaignState)
			{
				result.m_bNativeRecordPresent
					= m_NativeCampaignState.HasLoadedRecord();
				result.m_bNativeRecordValid
					= m_NativeCampaignState.IsLoadedRecordValid();
				result.m_bNativeRecordUnsupportedFuture
					= m_NativeCampaignState.IsLoadedRecordUnsupportedFuture();
			}
			string inspectedNativeFailure;
			if (m_NativeCampaignState)
				inspectedNativeFailure
					= m_NativeCampaignState.GetValidationFailure();
			HST_PersistenceSourceResolution preActiveResolution
				= ResolveInspectedNativeAuthorityBeforeActiveFlow(
					fallbackState,
					result,
					persistenceState,
					result.m_bNativeRecordUnsupportedFuture,
					inspectedNativeFailure);
			if (preActiveResolution)
				return preActiveResolution;

			if (!m_NativeCampaignState)
			{
				return ResolveProfileFallbackAfterNativeFailure(
					fallbackState,
					result,
					"native persistence is active without the campaign state proxy",
					true);
			}

			if (result.m_bNativeRecordPresent
				&& !result.m_bNativeRecordValid)
			{
				return ResolveProfileFallbackAfterNativeFailure(
					fallbackState,
					result,
					"native campaign record is present but invalid: "
						+ m_NativeCampaignState.GetValidationFailure(),
					true);
			}
			if (result.m_bNativeRecordValid)
			{
				HST_CampaignSaveData nativeSave
					= m_NativeCampaignState.GetSnapshot();
				return ResolveValidNativeAuthority(
					fallbackState,
					result,
					nativeSave,
					m_NativeCampaignState.GetSnapshotFingerprint());
			}

			if (result.m_bPersistenceSystemLoadedData)
			{
				return ResolveProfileFallbackAfterNativeFailure(
					fallbackState,
					result,
					"loaded native save is missing the campaign state record",
					true);
			}
		}

		HST_CampaignSaveData restoredSave = LoadProfileFallback();
		PopulateProfileJournalResolution(
			result,
			m_LastProfileJournalResolution);
		if (result.m_bProfileFallbackPresent && !restoredSave)
		{
			result.m_eSource
				= HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_FATAL;
			result.m_sEvidence
				= "profile journal artifacts are present but no valid generation could be selected | "
					+ m_sProfileFallbackStatus;
			m_LastSourceResolution = result;
			return result;
		}

		if (restoredSave)
		{
			result.m_bProfileFallbackRead = true;
			result.m_sProfileFallbackSnapshotFingerprint
				= m_LastProfileJournalResolution.m_Selected
					.m_sSnapshotFingerprint;
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
				= "profile journal selected because no native campaign record exists | "
					+ m_LastProfileJournalResolution.m_sEvidence;
			m_LastSourceResolution = result;
			return result;
		}

		fallbackState.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		fallbackState.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		fallbackState.m_iPersistenceRestoreSequence = 0;
		fallbackState.m_iPersistenceCheckpointSequence = 0;
		fallbackState.m_iForceSpawnQueueReconciledRestoreSequence = 0;
		fallbackState.m_bRestoredFromPersistence = false;
		fallbackState.m_sLastPersistenceStatus = "new campaign source selected";
		result.m_eSource
			= HST_ECampaignPersistenceSource
				.HST_PERSISTENCE_SOURCE_NEW_CAMPAIGN;
		result.m_State = fallbackState;
		result.m_sEvidence
			= "new campaign selected because native data and profile journal artifacts are absent";
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
		if (!HST_CampaignPersistentState.IsSnapshotSchemaSupported(restoredSave))
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

	bool IsProfileFallbackOnlyForSession()
	{
		return m_bProfileFallbackOnlyForSession;
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
		return FlushCampaignSaveData(m_TrackedCampaignSave, saveType);
	}

	protected bool FlushCampaignSaveData(
		HST_CampaignSaveData saveData,
		ESaveGameType saveType)
	{
		if (!saveData)
			return false;

		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (!persistence)
			return false;

		if (!TrackCampaignSaveData(saveData)
			|| !m_NativeCampaignState)
			return false;
		if (m_NativeCampaignState.GetSnapshotFingerprint().IsEmpty())
		{
			Print(
				"Partisan persistence | native transient staging rejected an unserializable campaign snapshot",
				LogLevel.ERROR);
			return false;
		}
		return persistence.Save(m_NativeCampaignState, saveType);
	}

	protected bool SaveProfileFallback(HST_CampaignSaveData saveData)
	{
		m_bProfileFallbackSaved = false;
		m_LastProfileJournalWriteReceipt = null;
		if (!saveData)
		{
			m_sProfileFallbackStatus = "profile fallback save skipped: no tracked save";
			return false;
		}

		HST_ProfilePathService.EnsureProfileDirectory();
		HST_CampaignProfileSaveWriteReceipt receipt;
		string evidence;
		if (m_ProfileJournal.WriteVerifiedSnapshot(
			saveData,
			receipt,
			evidence))
		{
			m_LastProfileJournalWriteReceipt = receipt;
			m_LastProfileJournalResolution = receipt.m_After;
			m_bProfileFallbackSaved = true;
			m_sProfileFallbackStatus = string.Format(
				"profile journal saved schema %1 | slot %2 | generation %3 | parent %4 | %5",
				saveData.m_iSchemaVersion,
				receipt.m_sWrittenSlotLabel,
				receipt.m_iGeneration,
				receipt.m_iPreviousGeneration,
				evidence);
			Print("Partisan persistence | " + m_sProfileFallbackStatus);
			return true;
		}

		m_LastProfileJournalWriteReceipt = receipt;
		if (receipt)
		{
			m_LastProfileJournalResolution = receipt.m_After;
			if (!m_LastProfileJournalResolution)
				m_LastProfileJournalResolution = receipt.m_Before;
		}
		m_sProfileFallbackStatus = "profile journal save failed | " + evidence;
		Print("Partisan persistence | " + m_sProfileFallbackStatus, LogLevel.WARNING);
		return false;
	}

	protected HST_CampaignSaveData LoadProfileFallback()
	{
		m_bProfileFallbackLoaded = false;
		HST_CampaignSaveData saveData;
		HST_CampaignProfileSaveResolution resolution;
		string evidence;
		if (!m_ProfileJournal.ReadSelectedSnapshot(
			saveData,
			resolution,
			evidence))
		{
			m_LastProfileJournalResolution = resolution;
			m_sProfileFallbackStatus = "profile journal load failed | " + evidence;
			if (resolution && resolution.m_bArtifactsPresent)
				Print("Partisan persistence | " + m_sProfileFallbackStatus, LogLevel.WARNING);
			return null;
		}

		m_LastProfileJournalResolution = resolution;
		m_bProfileFallbackLoaded = true;
		m_sProfileFallbackStatus = string.Format(
			"profile journal loaded schema %1 | slot %2 | generation %3 | legacy raw %4 | chain exact %5",
			saveData.m_iSchemaVersion,
			resolution.m_Selected.m_sSlotLabel,
			resolution.m_Selected.m_iGeneration,
			resolution.m_Selected.m_bLegacyRaw,
			resolution.m_bChainExact);
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
		bool recoveryExists
			= FileIO.FileExists(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		bool legacyExists = FileIO.FileExists(HST_ProfilePathService.LEGACY_CAMPAIGN_SAVE_FILE);
		return string.Format(
			"profile journal | canonical/recovery %1/%2 | retired legacy available %3 | saved %4 | loaded %5 | fallback-only session %6 | %7",
			exists,
			recoveryExists,
			legacyExists,
			m_bProfileFallbackSaved,
			m_bProfileFallbackLoaded,
			m_bProfileFallbackOnlyForSession,
			m_sProfileFallbackStatus);
	}
}
