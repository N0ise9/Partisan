class HST_PersistenceService
{
	static const string PROFILE_SAVE_DIRECTORY = "$profile:h-istasi";
	static const string PROFILE_SAVE_FILE = "$profile:h-istasi/HST_CampaignSaveData.json";

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

	void MarkMajorChange()
	{
		if (m_bCampaignDebugIsolationActive)
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
				bool majorCheckpointSaved = RequestCheckpoint("h-istasi major change", state);
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

		if (RequestCheckpoint("h-istasi autosave", state))
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
		string quarantineFailure;
		if (!NormalizeRetiredQuarantinedEnemyPatrolAuthority(state, quarantineFailure))
		{
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint deferred: exact patrol quarantine cleanup is incomplete during %1 | %2",
				context,
				quarantineFailure);
			Print("h-istasi persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		bool hasExactMissionConvoy;
		bool hasPhysicalExactEnemyPatrol;
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
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL
				|| operation.m_iContractVersion != HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
				continue;
			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
			{
				hasPhysicalExactEnemyPatrol = true;
				break;
			}
		}
		if (!m_PhysicalWar)
		{
			if (!hasExactMissionConvoy && !hasPhysicalExactEnemyPatrol)
				return true;
			state.m_sLastPersistenceStatus = "checkpoint deferred: exact physical roster reconciler is unavailable";
			Print("h-istasi persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}

		string reconcileFailure;
		if (!m_PhysicalWar.PrepareExactMissionConvoyAuthorityForPersistence(state, reconcileFailure))
		{
			state.m_sLastPersistenceStatus = string.Format("checkpoint deferred: exact convoy roster reconciliation failed during %1 | %2", context, reconcileFailure);
			Print("h-istasi persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		if (!hasPhysicalExactEnemyPatrol)
			return true;
		if (!m_ForceSpawnQueue || !m_ForceSpawnAdapter)
		{
			state.m_sLastPersistenceStatus = "checkpoint deferred: exact patrol roster authority is unavailable";
			Print("h-istasi persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}

		HST_ForceSpawnAdapterTickResult patrolRoster = m_ForceSpawnAdapter.ReconcileExactInfantryAuthorityForPersistence(
			state,
			m_ForceSpawnQueue,
			m_PhysicalWar,
			Math.Max(0, state.m_iElapsedSeconds));
		if (!patrolRoster || patrolRoster.m_iFailedCount > 0)
		{
			string rosterEvidence = "missing adapter reconciliation result";
			if (patrolRoster && !patrolRoster.m_sSummary.IsEmpty())
				rosterEvidence = patrolRoster.m_sSummary;
			state.m_sLastPersistenceStatus = string.Format(
				"checkpoint deferred: exact patrol roster reconciliation failed during %1 | %2",
				context,
				rosterEvidence);
			Print("h-istasi persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
			return false;
		}
		return ValidatePhysicalEnemyPatrolSnapshots(state, context);
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
		Print("h-istasi persistence | " + state.m_sLastPersistenceStatus, LogLevel.WARNING);
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
