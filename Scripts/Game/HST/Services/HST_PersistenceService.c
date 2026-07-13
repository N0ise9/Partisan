class HST_PersistenceService
{
	protected float m_fAutosaveElapsed;
	protected float m_fMajorChangeElapsed;
	protected bool m_bMajorChangePending;
	protected bool m_bCampaignDebugIsolationActive;
	protected ref HST_CampaignSaveData m_LastCapturedSave;
	protected ref HST_CampaignSaveData m_TrackedCampaignSave;
	protected ref HST_CampaignSaveData m_RestoredCampaignSave;
	protected ref HST_CampaignSaveData m_IsolatedCapturedSave;
	protected bool m_bProfileFallbackSaved;
	protected bool m_bProfileFallbackLoaded;
	protected string m_sProfileFallbackStatus = "profile fallback idle";
	protected HST_PhysicalWarService m_PhysicalWar;
	protected HST_ForceSpawnQueueService m_ForceSpawnQueue;
	protected HST_ForceSpawnAdapterService m_ForceSpawnAdapter;
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

	void Tick(HST_CampaignState state, float timeSlice, int autosaveIntervalSeconds, int majorChangeDebounceSeconds)
	{
		if (m_bCampaignDebugIsolationActive)
			return;

		m_fAutosaveElapsed += timeSlice;

		if (m_bMajorChangePending)
		{
			m_fMajorChangeElapsed += timeSlice;
			if (m_fMajorChangeElapsed >= majorChangeDebounceSeconds)
			{
				bool majorCheckpointSaved = RequestCheckpoint("Partisan major change", state);
				m_fMajorChangeElapsed = 0;
				if (majorCheckpointSaved)
				{
					m_bMajorChangePending = false;
					m_fAutosaveElapsed = 0;
				}
				else
				{
					int retrySeconds = Math.Max(1, majorChangeDebounceSeconds);
					m_fAutosaveElapsed = Math.Min(m_fAutosaveElapsed, Math.Max(0, autosaveIntervalSeconds - retrySeconds));
				}
				return;
			}
		}

		if (m_fAutosaveElapsed < autosaveIntervalSeconds)
			return;

		if (RequestCheckpoint("Partisan autosave", state))
			m_fAutosaveElapsed = 0;
		else
		{
			int retrySeconds = Math.Max(1, majorChangeDebounceSeconds);
			m_fAutosaveElapsed = Math.Max(0, autosaveIntervalSeconds - retrySeconds);
		}
	}

	bool RequestCheckpoint(string displayName, HST_CampaignState state = null)
	{
		if (m_bCampaignDebugIsolationActive)
			return CaptureIsolatedCampaignDebugState(state, "isolated checkpoint: " + displayName);

		if (state && !CaptureAndTrackState(state, "captured before checkpoint"))
			return false;

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
			CaptureIsolatedCampaignDebugState(state, persistenceStatus);
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
					&& operation.m_iContractVersion == HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION);
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
		if (!ValidatePhysicalEnemyPatrolSnapshots(state, context))
			return false;
		if (!ValidatePhysicalGarrisonPatrolSnapshots(state, context))
			return false;
		if (!ValidatePhysicalMissionGuardSnapshots(state, context))
			return false;
		return ValidatePhysicalPlayerSupportBindings(state, context);
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
		fallbackState.m_iPersistenceRestoreSequence = 0;
		fallbackState.m_iForceSpawnQueueReconciledRestoreSequence = 0;
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
		restoredSave.ApplyTo(targetState, false);
		targetState.m_iPersistenceRestoreSequence = Math.Max(0, targetState.m_iPersistenceRestoreSequence) + 1;
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
		if (m_TrackedCampaignSave)
			tracked = persistence.IsTracked(m_TrackedCampaignSave);

		return string.Format("PersistenceSystem | loaded %1 | state %2 | tracked %3", persistence.WasDataLoaded(), persistence.GetState(), tracked);
	}

	protected string BuildProfileFallbackStatus()
	{
		bool exists = FileIO.FileExists(HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		bool legacyExists = FileIO.FileExists(HST_ProfilePathService.LEGACY_CAMPAIGN_SAVE_FILE);
		return string.Format("profile fallback | canonical exists %1 | legacy available %2 | saved %3 | loaded %4 | %5", exists, legacyExists, m_bProfileFallbackSaved, m_bProfileFallbackLoaded, m_sProfileFallbackStatus);
	}
}
