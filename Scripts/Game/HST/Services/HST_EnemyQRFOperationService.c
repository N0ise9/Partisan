class HST_EnemyQRFAdmissionResult
{
	bool m_bSuccess;
	bool m_bStateChanged;
	string m_sFailureReason;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
}

class HST_EnemyQRFOperationService
{
	static const int EXACT_QRF_PRIORITY = 80;
	static const int EXACT_QRF_MAX_RETRIES = 3;
	static const int EXACT_QRF_DEPLOYMENT_GRACE_SECONDS = 180;
	static const string EXACT_QRF_GROUP_MODE = "exact_enemy_defensive_qrf";

	protected ref HST_OperationService m_Operations = new HST_OperationService();
	protected ref HST_StrategicMovementService m_StrategicMovement = new HST_StrategicMovementService();
	protected ref HST_MaterializationService m_Materialization = new HST_MaterializationService();
	protected ref HST_ForcePlanningIntegrityService m_Integrity = new HST_ForcePlanningIntegrityService();
	protected ref HST_ForceSpawnQueueService m_SpawnQueue;
	protected ref HST_ForceSpawnAdapterService m_SpawnAdapter;
	protected ref HST_PhysicalWarService m_PhysicalWar;

	void SetRuntimeServices(
		HST_ForceSpawnQueueService spawnQueue,
		HST_ForceSpawnAdapterService spawnAdapter,
		HST_PhysicalWarService physicalWar)
	{
		m_SpawnQueue = spawnQueue;
		m_SpawnAdapter = spawnAdapter;
		m_PhysicalWar = physicalWar;
	}

	bool IsExactEnemyDefensiveQRF(HST_EnemyOrderState order)
	{
		return order && order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF
			&& order.m_iOperationContractVersion == HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION;
	}

	bool HasOpenExactEnemyDefensiveQRF(
		HST_CampaignState state,
		string factionKey,
		string targetZoneId)
	{
		if (!state || factionKey.IsEmpty() || targetZoneId.IsEmpty())
			return false;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsExactEnemyDefensiveQRF(order) || order.m_sFactionKey != factionKey
				|| !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					order.m_sTargetZoneId, targetZoneId))
				continue;
			if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				continue;
			HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
			if (operation && operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				return true;
		}
		return false;
	}

	HST_EnemyQRFAdmissionResult CanAdmitPreparedOrder(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_EnemyDirectorService enemyDirector)
	{
		HST_EnemyQRFAdmissionResult result = new HST_EnemyQRFAdmissionResult();
		string failure = ValidateAdmissionContext(state, order, manifest, enemyDirector);
		if (!failure.IsEmpty())
		{
			result.m_sFailureReason = failure;
			return result;
		}
		if (state.FindOperation(order.m_sOperationId) || state.FindForceManifest(manifest.m_sManifestId)
			|| state.FindActiveGroup(BuildProjectionId(order)) || state.FindForceSpawnResultByRequest(order.m_sOrderId)
			|| state.FindForceSpawnResult(BuildSpawnResultId(order)))
		{
			result.m_sFailureReason = "exact enemy defensive QRF durable admission identity already exists";
			return result;
		}

		HST_ForceSpawnQueueRequest request = new HST_ForceSpawnQueueRequest();
		request.m_sResultId = BuildSpawnResultId(order);
		request.m_sRequestId = order.m_sOrderId;
		request.m_sForceId = BuildForceId(order);
		request.m_sProjectionId = BuildProjectionId(order);
		request.m_iPriority = EXACT_QRF_PRIORITY;
		request.m_iMaxRetries = EXACT_QRF_MAX_RETRIES;
		request.m_iDeadlineSecond = state.m_iElapsedSeconds + EXACT_QRF_DEPLOYMENT_GRACE_SECONDS;
		HST_ForceSpawnQueueEnqueueResult queuePreflight = m_SpawnQueue.CanEnqueue(
			state.m_aForceSpawnResults,
			manifest,
			request,
			state.m_iElapsedSeconds);
		if (!queuePreflight || !queuePreflight.m_bSuccess)
		{
			result.m_sFailureReason = "exact enemy defensive QRF spawn admission preflight failed";
			if (queuePreflight && !queuePreflight.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = result.m_sFailureReason + ": " + queuePreflight.m_sFailureReason;
			return result;
		}
		result.m_bSuccess = true;
		return result;
	}

	protected bool HasCommittedAdmissionAuthority(HST_CampaignState state, HST_EnemyOrderState order)
	{
		if (!order)
			return false;
		if (order.m_bStrategicServiceCommitted || !order.m_sSpawnResultId.IsEmpty() || !order.m_sGroupId.IsEmpty())
			return true;
		if (!state || order.m_sOperationId.IsEmpty())
			return false;
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		return operation && (!operation.m_sSpawnResultId.IsEmpty() || !operation.m_sForceId.IsEmpty()
			|| !operation.m_sProjectionId.IsEmpty() || !operation.m_sGroupId.IsEmpty());
	}

	protected HST_EnemyQRFAdmissionResult ResolveCommittedAdmissionReplay(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState suppliedManifest)
	{
		HST_EnemyQRFAdmissionResult result = new HST_EnemyQRFAdmissionResult();
		if (!state || !order || !suppliedManifest)
		{
			result.m_sFailureReason = "exact enemy defensive QRF committed replay context is incomplete";
			return result;
		}
		HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
		if (!manifest || !suppliedManifest.m_bFrozen
			|| suppliedManifest.m_sManifestId != manifest.m_sManifestId
			|| suppliedManifest.m_sOperationId != manifest.m_sOperationId
			|| suppliedManifest.m_sManifestHash != manifest.m_sManifestHash
			|| m_Integrity.BuildManifestHash(suppliedManifest) != suppliedManifest.m_sManifestHash)
		{
			result.m_sFailureReason = "exact enemy defensive QRF committed replay manifest conflicts";
			return result;
		}
		HST_OperationRecordState operation;
		HST_ForceManifestState resolvedManifest;
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		string failure = ResolveRuntimeContext(state, order, operation, resolvedManifest, batch, group);
		if (!failure.IsEmpty() || resolvedManifest != manifest
			|| order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			|| !order.m_bStrategicServiceCommitted
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| order.m_bResourceSettlementApplied)
		{
			result.m_sFailureReason = "exact enemy defensive QRF committed replay authority conflicts";
			if (!failure.IsEmpty())
				result.m_sFailureReason = result.m_sFailureReason + ": " + failure;
			return result;
		}
		result.m_bSuccess = true;
		result.m_bStateChanged = false;
		result.m_Operation = operation;
		result.m_Batch = batch;
		result.m_Group = group;
		return result;
	}

	HST_EnemyQRFAdmissionResult AdmitPreparedOrder(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_EnemyDirectorService enemyDirector)
	{
		HST_EnemyQRFAdmissionResult result = new HST_EnemyQRFAdmissionResult();
		if (HasCommittedAdmissionAuthority(state, order))
			return ResolveCommittedAdmissionReplay(state, order, manifest);
		HST_EnemyQRFAdmissionResult admissionPreflight = CanAdmitPreparedOrder(state, order, manifest, enemyDirector);
		if (!admissionPreflight || !admissionPreflight.m_bSuccess)
		{
			string failure = "exact enemy defensive QRF admission preflight failed";
			if (admissionPreflight && !admissionPreflight.m_sFailureReason.IsEmpty())
				failure = admissionPreflight.m_sFailureReason;
			result.m_sFailureReason = failure;
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
			result.m_bStateChanged = true;
			return result;
		}
		string failure;

		order.m_sManifestId = manifest.m_sManifestId;
		order.m_sManifestHash = manifest.m_sManifestHash;
		order.m_iCompositionManpower = manifest.m_iAcceptedMemberCount;
		order.m_iCompositionVehicleCount = 0;
		order.m_iCompositionArmedVehicleCount = 0;
		order.m_sCompositionIntentId = manifest.m_sIntentId;
		order.m_sCompositionTier = "exact";
		order.m_sCompositionSummary = string.Format("%1 exact infantry", manifest.m_iAcceptedMemberCount);
		order.m_iResolveAtSecond = 0;

		HST_ForceManifestState existingManifest = state.FindForceManifest(manifest.m_sManifestId);
		if (existingManifest)
		{
			if (existingManifest.m_sManifestHash != manifest.m_sManifestHash
				|| existingManifest.m_sOperationId != order.m_sOperationId)
			{
				failure = "exact enemy defensive QRF manifest identity conflicts with existing authority";
				result.m_sFailureReason = failure;
				FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
				result.m_bStateChanged = true;
				return result;
			}
			manifest = existingManifest;
		}
		else
		{
			state.m_aForceManifests.Insert(manifest);
			result.m_bStateChanged = true;
		}

		HST_OperationTransitionResult registered = m_Operations.RegisterExactEnemyDefensiveQRF(state, order, manifest);
		if (!registered || !registered.m_bAccepted || !registered.m_Operation)
		{
			failure = "exact enemy defensive QRF operation registration failed";
			if (registered && !registered.m_sFailureReason.IsEmpty())
				failure = failure + ": " + registered.m_sFailureReason;
			result.m_sFailureReason = failure;
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
			result.m_bStateChanged = true;
			return result;
		}
		result.m_Operation = registered.m_Operation;
		result.m_bStateChanged = registered.m_bStateChanged || result.m_bStateChanged;

		string resultId = BuildSpawnResultId(order);
		string forceId = BuildForceId(order);
		string projectionId = BuildProjectionId(order);
		HST_ActiveGroupState group = state.FindActiveGroup(projectionId);
		if (group)
		{
			failure = ValidateActiveGroup(order, manifest, group, resultId, forceId, projectionId);
			if (!failure.IsEmpty())
			{
				result.m_sFailureReason = failure;
				FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
				result.m_bStateChanged = true;
				return result;
			}
		}
		else
		{
			group = BuildActiveGroup(state, order, manifest, resultId, forceId, projectionId);
			if (!group)
			{
				failure = "exact enemy defensive QRF active-group projection could not be built";
				result.m_sFailureReason = failure;
				FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
				result.m_bStateChanged = true;
				return result;
			}
			state.m_aActiveGroups.Insert(group);
			result.m_bStateChanged = true;
		}
		result.m_Group = group;

		HST_ForceSpawnQueueRequest spawnRequest = new HST_ForceSpawnQueueRequest();
		spawnRequest.m_sResultId = resultId;
		spawnRequest.m_sRequestId = order.m_sOrderId;
		spawnRequest.m_sForceId = forceId;
		spawnRequest.m_sProjectionId = projectionId;
		spawnRequest.m_iPriority = EXACT_QRF_PRIORITY;
		spawnRequest.m_iMaxRetries = EXACT_QRF_MAX_RETRIES;
		HST_ForceSpawnResultState existingBatch = state.FindForceSpawnResult(resultId);
		if (!existingBatch)
			existingBatch = state.FindForceSpawnResultByRequest(order.m_sOrderId);
		if (existingBatch)
			spawnRequest.m_iDeadlineSecond = existingBatch.m_iDeadlineSecond;
		else
			spawnRequest.m_iDeadlineSecond = state.m_iElapsedSeconds + EXACT_QRF_DEPLOYMENT_GRACE_SECONDS;
		HST_ForceSpawnQueueEnqueueResult enqueue = m_SpawnQueue.Enqueue(
			state.m_aForceSpawnResults,
			manifest,
			spawnRequest,
			state.m_iElapsedSeconds);
		if (!enqueue || !enqueue.m_bSuccess || !enqueue.m_Batch)
		{
			failure = "exact enemy defensive QRF spawn admission failed";
			if (enqueue && !enqueue.m_sFailureReason.IsEmpty())
				failure = failure + ": " + enqueue.m_sFailureReason;
			result.m_sFailureReason = failure;
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
			result.m_bStateChanged = true;
			return result;
		}
		result.m_Batch = enqueue.m_Batch;
		result.m_bStateChanged = enqueue.m_bStateChanged || result.m_bStateChanged;

		HST_ForceSpawnQueueCallbackResult held = m_SpawnQueue.HoldPendingProjectionForStrategicSimulation(
			state.m_aForceSpawnResults,
			manifest,
			enqueue.m_Batch.m_sResultId,
			enqueue.m_Batch.m_sProjectionId,
			state.m_iElapsedSeconds);
		if (!held || !held.m_bAccepted)
		{
			failure = "exact enemy defensive QRF strategic hold failed";
			if (held && !held.m_sFailureReason.IsEmpty())
				failure = failure + ": " + held.m_sFailureReason;
			result.m_sFailureReason = failure;
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
			result.m_bStateChanged = true;
			return result;
		}

		bool routeInitialized = m_StrategicMovement.InitializeExactInfantryQRFRoute(
			state,
			registered.m_Operation,
			manifest,
			group);
		HST_OperationTransitionResult linked;
		if (routeInitialized)
			linked = m_Operations.LinkExactEnemyDefensiveQRFOutboundVirtual(state, order, group, enqueue.m_Batch);
		if (!routeInitialized || !linked || !linked.m_bAccepted)
		{
			failure = "exact enemy defensive QRF strategic projection link failed";
			if (linked && !linked.m_sFailureReason.IsEmpty())
				failure = failure + ": " + linked.m_sFailureReason;
			result.m_sFailureReason = failure;
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
			result.m_bStateChanged = true;
			return result;
		}

		order.m_sSpawnResultId = enqueue.m_Batch.m_sResultId;
		order.m_sGroupId = group.m_sGroupId;
		order.m_bStrategicServiceCommitted = true;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
		order.m_sRuntimeStatus = "exact_virtual_outbound";
		order.m_sFailureReason = "";
		result.m_bSuccess = true;
		result.m_bStateChanged = true;
		return result;
	}

	bool TickOrder(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order)
	{
		if (!IsExactEnemyDefensiveQRF(order) || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return false;
		if (!state || !preset || !enemyDirector || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar)
			return SetRuntimeConflict(order, "exact enemy defensive QRF runtime services are unavailable");
		string ambiguousAuthority = FindAmbiguousAuthorityRows(state, order);
		if (!ambiguousAuthority.IsEmpty())
			return AbortAmbiguousAuthority(state, order, ambiguousAuthority);
		HST_OperationRecordState settledOperation = state.FindOperation(order.m_sOperationId);
		if (settledOperation
			&& settledOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			return FinalizeSettledOrder(
				state,
				order,
				settledOperation,
				state.FindForceSpawnResult(order.m_sSpawnResultId),
				state.FindActiveGroup(order.m_sGroupId));
		}

		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		string failure = ResolveRuntimeContext(state, order, operation, manifest, batch, group);
		if (!failure.IsEmpty())
			return QuarantineRuntimeAuthority(order, failure);

		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
			return ContinueDematerialization(state, enemyDirector, order, operation, manifest, batch, group, operation.m_sLastProjectionReason);

		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
		{
			bool changed = ApplyDefensiveArrivalOutcome(state, preset, order, operation, batch);
			HST_OperationTransitionResult returning = m_Operations.BeginExactEnemyDefensiveQRFReturnToOrigin(state, order, group);
			if (!returning || !returning.m_bAccepted)
			{
				if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
				{
					return RetireAndSettlePhysicalFailure(
						state,
						enemyDirector,
						order,
						operation,
						manifest,
						batch,
						group,
						HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
						"invalidated_survivors",
						ResolveSettlementSurvivors(operation, manifest, batch, group),
						"exact enemy defensive QRF return transition failed") || changed;
				}
				return SettleOperation(
					state,
					enemyDirector,
					order,
					operation,
					manifest,
					batch,
					group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
					"invalidated_survivors",
					ResolveSettlementSurvivors(operation, manifest, batch, group),
					"exact enemy defensive QRF return transition failed") || changed;
			}
			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			{
				bool routeRestarted = m_PhysicalWar.RestartExactEnemyQRFInfantryRoute(
					state,
					group,
					operation.m_vOriginPosition,
					"Exact enemy defensive QRF returning to its immutable origin.");
				if (!routeRestarted)
				{
					return RetireAndSettlePhysicalFailure(
						state,
						enemyDirector,
						order,
						operation,
						manifest,
						batch,
						group,
						HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
						"route_failed_survivors",
						ResolveSettlementSurvivors(operation, manifest, batch, group),
						"exact enemy defensive QRF return route could not be issued") || changed;
				}
			}
			else
				group.m_sRuntimeStatus = "enemy_qrf_virtual_returning";
			return true;
		}

		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL)
			return TickVirtual(state, preset, enemyDirector, order, operation, manifest, batch, group);
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
			return TickMaterializing(state, enemyDirector, order, operation, manifest, batch, group);
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			return TickPhysical(state, preset, enemyDirector, order, operation, manifest, batch, group);

		return FailClosedActiveOrder(
			state,
			enemyDirector,
			order,
			operation,
			manifest,
			batch,
			group,
			"exact enemy defensive QRF materialization authority is invalid");
	}

	bool ReconcileAfterRestore(HST_CampaignState state, HST_EnemyDirectorService enemyDirector)
	{
		if (!state || !enemyDirector)
			return false;
		bool changed;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsExactEnemyDefensiveQRF(order))
				continue;
			string ambiguousAuthority = FindAmbiguousAuthorityRows(state, order);
			if (!ambiguousAuthority.IsEmpty())
			{
				changed = AbortAmbiguousAuthority(state, order, ambiguousAuthority) || changed;
				continue;
			}
			HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
			HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
			HST_ForceSpawnResultState batch = state.FindForceSpawnResult(order.m_sSpawnResultId);
			HST_ActiveGroupState group = state.FindActiveGroup(order.m_sGroupId);
			bool restoreCommitted = HasDurableRestoreCommitment(order, operation, batch, group);
			if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED)
			{
				order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
				order.m_sRuntimeStatus = "exact_restore_interrupted_admission";
				order.m_sFailureReason = "exact enemy defensive QRF restore found an interrupted queued admission";
				changed = true;
			}
			if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
			{
				bool resourcesSettled = ValidateAppliedResourceSettlement(order, manifest);
				if (HasPartialResourceSettlementAuthority(order))
				{
					order.m_sRuntimeStatus = "exact_restore_settlement_conflict";
					order.m_sFailureReason = "exact enemy defensive QRF restore contains a partial resource receipt";
					changed = true;
					continue;
				}
				if (operation
					&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
					&& !resourcesSettled)
				{
					order.m_sRuntimeStatus = "exact_restore_settlement_conflict";
					order.m_sFailureReason = "settled exact enemy defensive QRF lacks a matching durable resource receipt";
					changed = true;
					continue;
				}
				string restoreSettlementKind = order.m_sResourceSettlementKind;
				if (!resourcesSettled && !order.m_bResourceSettlementApplied)
				{
					int survivors = ResolveRestoreSettlementSurvivors(operation, order, manifest, batch, group, restoreCommitted);
					restoreSettlementKind = "restore_invalidated_full";
					if (restoreCommitted)
						restoreSettlementKind = "restore_invalidated_survivors";
					resourcesSettled = ApplyRestoreInvalidationResourceSettlement(
						state,
						enemyDirector,
						order,
						manifest,
						restoreSettlementKind,
						survivors,
						!restoreCommitted,
						"exact enemy defensive QRF restore invalidation");
					changed = resourcesSettled || changed;
				}
				bool restoreAuthorityChanged;
				if (resourcesSettled)
					restoreAuthorityChanged = SettleInvalidatedRestoreAuthority(state, order, restoreSettlementKind);
				changed = restoreAuthorityChanged || changed;
				operation = state.FindOperation(order.m_sOperationId);
				bool operationSettled = !operation && !restoreCommitted;
				if (operation)
				{
					operationSettled = operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
						&& operation.m_sSettlementId == order.m_sResourceSettlementId;
				}
				if (!resourcesSettled || !operationSettled)
				{
					order.m_sRuntimeStatus = "exact_restore_settlement_retry";
					order.m_sFailureReason = "exact enemy defensive QRF restore settlement is waiting for complete refund and terminal authority";
					changed = true;
					continue;
				}
				if (batch)
				{
					int batchIndex = state.m_aForceSpawnResults.Find(batch);
					if (batchIndex >= 0)
					{
						state.m_aForceSpawnResults.Remove(batchIndex);
						changed = true;
					}
				}
				if (group)
				{
					int groupIndex = state.m_aActiveGroups.Find(group);
					if (groupIndex >= 0)
					{
						state.m_aActiveGroups.Remove(groupIndex);
						changed = true;
					}
				}
				continue;
			}
			if (operation && operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			{
				if (HasValidMatchingSettledResourceAuthority(state, order, operation, manifest))
					changed = FinalizeSettledOrder(state, order, operation, batch, group) || changed;
				else
				{
					order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
					order.m_bPhysicalized = false;
					order.m_sRuntimeStatus = "exact_restore_settlement_conflict";
					order.m_sFailureReason = "settled exact enemy defensive QRF restore authority conflicts with its resource settlement";
					changed = true;
				}
				continue;
			}
			if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE || !operation || !manifest || !batch || !group)
				continue;
			order.m_bPhysicalized = false;
			order.m_bAbstractResolved = false;
			order.m_sRuntimeStatus = "exact_enemy_qrf_restore_virtual";
			group.m_sRuntimeStatus = "enemy_qrf_virtual";
			group.m_bSpawnedEntity = false;
			group.m_sRuntimeEntityId = "";
			group.m_iSpawnedAgentCount = 0;
			changed = true;
		}
		return changed;
	}

	bool ReconcileSettledRuntimeCleanup(HST_CampaignState state)
	{
		if (!state)
			return false;
		bool changed;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsExactEnemyDefensiveQRF(order))
				continue;
			HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
			if (!operation || operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				continue;
			HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
			if (!HasValidMatchingSettledResourceAuthority(state, order, operation, manifest))
				continue;
			changed = FinalizeSettledOrder(
				state,
				order,
				operation,
				state.FindForceSpawnResult(order.m_sSpawnResultId),
				state.FindActiveGroup(order.m_sGroupId)) || changed;
		}
		return changed;
	}

	protected bool SettleInvalidatedRestoreAuthority(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		string settlementKind)
	{
		if (!state || !order || settlementKind.IsEmpty())
			return false;
		HST_OperationRecordState canonical;
		bool identityConflict;
		foreach (HST_OperationRecordState candidate : state.m_aOperations)
		{
			if (!candidate)
				continue;
			bool matchesOperation = candidate.m_sOperationId == order.m_sOperationId;
			bool matchesOrder = candidate.m_sEnemyOrderId == order.m_sOrderId;
			if (!matchesOperation && !matchesOrder)
				continue;
			bool exactContract = candidate.m_eType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF
				&& candidate.m_iContractVersion == HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION;
			if (matchesOperation && matchesOrder && exactContract && !canonical)
			{
				canonical = candidate;
				continue;
			}
			identityConflict = true;
		}
		if (!canonical || identityConflict)
		{
			order.m_sRuntimeStatus = "exact_restore_authority_conflict";
			order.m_sFailureReason = "exact enemy defensive QRF restore operation identity is missing, ambiguous, or conflicting";
			return false;
		}

		string settlementId = HST_OperationService.BuildSettlementId(order.m_sOperationId, settlementKind);
		if (canonical.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (canonical.m_sSettlementId == settlementId)
				return false;
			order.m_sRuntimeStatus = "exact_restore_settlement_conflict";
			order.m_sFailureReason = "settled exact enemy defensive QRF has a conflicting terminal receipt";
			return false;
		}
		HST_OperationTransitionResult terminal = m_Operations.SettleExactEnemyDefensiveQRF(
			state,
			order,
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
			settlementId,
			"exact enemy defensive QRF restore invalidation");
		if (!terminal || !terminal.m_bAccepted)
		{
			if (!ForceSettleInvalidatedRestore(canonical, order, settlementId, "exact enemy defensive QRF restore invalidation", state.m_iElapsedSeconds))
			{
				order.m_sRuntimeStatus = "exact_restore_authority_conflict";
				order.m_sFailureReason = "exact enemy defensive QRF restore operation ownership conflicts";
				return false;
			}
		}
		return true;
	}

	protected bool ForceSettleInvalidatedRestore(
		HST_OperationRecordState operation,
		HST_EnemyOrderState order,
		string settlementId,
		string reason,
		int nowSecond)
	{
		if (!operation || !order
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF
			|| operation.m_iContractVersion != HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION
			|| operation.m_sOperationId != order.m_sOperationId
			|| operation.m_sEnemyOrderId != order.m_sOrderId)
			return false;
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return false;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		operation.m_eTerminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED;
		operation.m_sSettlementId = settlementId;
		operation.m_sTerminalReason = reason;
		operation.m_iDutyStateEnteredAtSecond = Math.Max(0, nowSecond);
		operation.m_iEngagementStateEnteredAtSecond = Math.Max(0, nowSecond);
		operation.m_iMaterializationStateEnteredAtSecond = Math.Max(0, nowSecond);
		operation.m_iLastProgressAtSecond = Math.Max(0, nowSecond);
		operation.m_iSettledAtSecond = Math.Max(0, nowSecond);
		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		operation.m_iRevision++;
		return true;
	}

	bool SettleOpenOrdersForCampaignStop(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		string reason)
	{
		if (!state || !enemyDirector)
			return false;
		if (reason.IsEmpty())
			reason = "campaign phase no longer permits an active enemy QRF";
		bool changed;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsExactEnemyDefensiveQRF(order)
				|| (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
					&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED))
				continue;
			HST_OperationRecordState operation;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			string failure = ResolveRuntimeContext(state, order, operation, manifest, batch, group);
			if (!failure.IsEmpty())
				changed = QuarantineRuntimeAuthority(order, reason + ": " + failure) || changed;
			else
				changed = FailClosedActiveOrder(state, enemyDirector, order, operation, manifest, batch, group, reason) || changed;
		}
		return changed;
	}

	protected bool ApplyRestoreInvalidationResourceSettlement(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		string settlementKind,
		int survivorCount,
		bool fullRefund,
		string reason)
	{
		if (!state || !enemyDirector || !order || settlementKind.IsEmpty())
			return false;
		int accepted = ResolveRestoreAcceptedMemberCount(order, manifest);
		int survivors = Math.Max(0, Math.Min(accepted, survivorCount));
		int attackRefund;
		int supportRefund;
		if (fullRefund)
		{
			attackRefund = Math.Max(0, order.m_iAttackCost);
			supportRefund = Math.Max(0, order.m_iSupportCost);
		}
		else if (accepted > 0)
		{
			attackRefund = Math.Max(0, order.m_iAttackCost) * survivors / accepted;
			supportRefund = Math.Max(0, order.m_iSupportCost) * survivors / accepted;
		}
		string settlementId = HST_OperationService.BuildSettlementId(order.m_sOperationId, settlementKind);
		if (order.m_bResourceSettlementApplied)
			return order.m_sResourceSettlementId == settlementId;
		if (HasPartialResourceSettlementAuthority(order))
			return false;

		if ((attackRefund > 0 || supportRefund > 0) && !state.FindFactionPool(order.m_sFactionKey))
			return false;
		enemyDirector.RefundDefenseResources(
			state,
			order.m_sFactionKey,
			order.m_sTargetZoneId,
			attackRefund,
			supportRefund,
			reason);
		order.m_sResourceSettlementId = settlementId;
		order.m_sResourceSettlementKind = settlementKind;
		order.m_iSettlementAcceptedMemberCount = accepted;
		order.m_iSettlementSurvivorMemberCount = survivors;
		order.m_iRefundedAttackResources = attackRefund;
		order.m_iRefundedSupportResources = supportRefund;
		order.m_bResourceSettlementApplied = true;
		return true;
	}

	protected int ResolveRestoreAcceptedMemberCount(
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!order)
			return 0;
		int paidCount = Math.Max(0, order.m_iCompositionManpower);
		if (paidCount > 0)
			return paidCount;
		if (IsTrustedRestoreManifest(order, manifest))
			return Math.Max(0, manifest.m_iAcceptedMemberCount);
		return 0;
	}

	protected bool HasDurableRestoreCommitment(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!order)
			return false;
		if (order.m_bStrategicServiceCommitted)
			return true;
		if (!order.m_sSpawnResultId.IsEmpty() || !order.m_sGroupId.IsEmpty())
			return true;
		if (operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF
			&& operation.m_iContractVersion == HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION
			&& operation.m_sOperationId == order.m_sOperationId
			&& operation.m_sEnemyOrderId == order.m_sOrderId)
		{
			if (!operation.m_sSpawnResultId.IsEmpty() || !operation.m_sForceId.IsEmpty()
				|| !operation.m_sProjectionId.IsEmpty() || !operation.m_sGroupId.IsEmpty())
				return true;
		}
		if (batch && batch.m_sOperationId == order.m_sOperationId)
			return true;
		return group && group.m_sOperationId == order.m_sOperationId;
	}

	protected bool IsTrustedRestoreManifest(
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!order || !manifest)
			return false;
		if (!manifest.m_bFrozen || manifest.m_iAcceptedMemberCount <= 0)
			return false;
		if (manifest.m_sManifestId != order.m_sManifestId || manifest.m_sOperationId != order.m_sOperationId)
			return false;
		if (manifest.m_sManifestHash.IsEmpty() || manifest.m_sManifestHash != order.m_sManifestHash)
			return false;
		if (manifest.m_sFactionKey != order.m_sFactionKey)
			return false;
		if (manifest.m_sSourceZoneId != order.m_sSourceZoneId || manifest.m_sTargetZoneId != order.m_sTargetZoneId)
			return false;
		if (manifest.m_iAttackResourceCost != order.m_iAttackCost
			|| manifest.m_iSupportResourceCost != order.m_iSupportCost)
			return false;
		if (m_Integrity.BuildManifestHash(manifest) != manifest.m_sManifestHash)
			return false;
		return order.m_iCompositionManpower <= 0
			|| order.m_iCompositionManpower == manifest.m_iAcceptedMemberCount;
	}

	protected int ResolveRestoreSettlementSurvivors(
		HST_OperationRecordState operation,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		bool committed)
	{
		int accepted = ResolveRestoreAcceptedMemberCount(order, manifest);
		if (accepted <= 0)
			return 0;
		if (batch && m_SpawnQueue && batch.m_sOperationId == order.m_sOperationId
			&& batch.m_sManifestId == order.m_sManifestId)
		{
			if (batch.m_bStrategicProjectionHeld)
				return Math.Max(0, Math.Min(accepted, m_SpawnQueue.CountStrategicLivingMemberSlots(batch)));
			if (batch.m_iSuccessfulHandoffCount > 0)
				return Math.Max(0, Math.Min(accepted, m_SpawnQueue.CountDurableLivingMemberSlots(batch)));
		}
		if (group && group.m_sOperationId == order.m_sOperationId
			&& group.m_sEnemyOrderId == order.m_sOrderId)
		{
			if (group.m_sRuntimeStatus == "eliminated")
				return 0;
			int groupSurvivors = Math.Max(Math.Max(0, group.m_iDurableLivingInfantryCount), Math.Max(0, group.m_iSurvivorInfantryCount));
			return Math.Max(0, Math.Min(accepted, groupSurvivors));
		}
		if (operation && operation.m_sOperationId == order.m_sOperationId
			&& operation.m_sEnemyOrderId == order.m_sOrderId)
			return Math.Max(0, Math.Min(accepted, operation.m_iLastVirtualFriendlyCount));
		if (!committed)
			return accepted;
		return 0;
	}

	protected bool ValidateAppliedResourceSettlement(
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!order || !order.m_bResourceSettlementApplied || order.m_sResourceSettlementKind.IsEmpty())
			return false;
		int accepted = ResolveRestoreAcceptedMemberCount(order, manifest);
		int survivors = order.m_iSettlementSurvivorMemberCount;
		if (accepted <= 0 || order.m_iSettlementAcceptedMemberCount != accepted
			|| survivors < 0 || survivors > accepted)
			return false;
		string expectedId = HST_OperationService.BuildSettlementId(order.m_sOperationId, order.m_sResourceSettlementKind);
		if (order.m_sResourceSettlementId != expectedId)
			return false;
		int expectedAttackRefund = Math.Max(0, order.m_iAttackCost) * survivors / accepted;
		int expectedSupportRefund = Math.Max(0, order.m_iSupportCost) * survivors / accepted;
		if (order.m_sResourceSettlementKind.Contains("_full"))
		{
			expectedAttackRefund = Math.Max(0, order.m_iAttackCost);
			expectedSupportRefund = Math.Max(0, order.m_iSupportCost);
		}
		return order.m_iRefundedAttackResources == expectedAttackRefund
			&& order.m_iRefundedSupportResources == expectedSupportRefund;
	}

	protected bool HasPartialResourceSettlementAuthority(HST_EnemyOrderState order)
	{
		if (!order || order.m_bResourceSettlementApplied)
			return false;
		return !order.m_sResourceSettlementId.IsEmpty()
			|| !order.m_sResourceSettlementKind.IsEmpty()
			|| order.m_iSettlementAcceptedMemberCount != 0
			|| order.m_iSettlementSurvivorMemberCount != 0
			|| order.m_iRefundedAttackResources != 0
			|| order.m_iRefundedSupportResources != 0;
	}

	protected bool HasValidMatchingSettledResourceAuthority(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		if (!state || !order || !operation || !manifest || !order.m_bResourceSettlementApplied
			|| order.m_sResourceSettlementKind.IsEmpty() || order.m_sResourceSettlementId.IsEmpty())
			return false;
		string expectedId = HST_OperationService.BuildSettlementId(order.m_sOperationId, order.m_sResourceSettlementKind);
		if (order.m_sResourceSettlementId != expectedId || operation.m_sSettlementId != expectedId)
			return false;
		return m_Operations.ValidateExactEnemyDefensiveQRF(state, operation, order, manifest).IsEmpty();
	}

	protected bool TickVirtual(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
		{
			return SettleOperation(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"virtual_projection_failed_survivors",
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy defensive QRF held projection failed");
		}
		if (!batch.m_bStrategicProjectionHeld)
		{
			return SettleOperation(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"invalidated_survivors",
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy defensive QRF virtual batch is not strategically held");
		}

		int living = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
		SyncGroupRoster(group, living);
		operation.m_iLastVirtualFriendlyCount = living;
		if (living <= 0)
		{
			m_SpawnQueue.CompleteStrategicProjectionElimination(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				state.m_iElapsedSeconds,
				"exact enemy defensive QRF virtual roster eliminated");
			return SettleOperation(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
				"destroyed",
				0,
				"exact enemy defensive QRF roster was eliminated");
		}

		if (!m_StrategicMovement.InitializeExactInfantryQRFRoute(state, operation, manifest, group))
		{
			return SettleOperation(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
				"route_failed_survivors",
				living,
				"exact enemy defensive QRF strategic route is invalid");
		}
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN
			&& Distance2D(operation.m_vStrategicPosition, operation.m_vOriginPosition) <= HST_StrategicMovementService.ARRIVAL_EPSILON_METERS)
		{
			return SettleOperation(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED,
				"returned_survivors",
				living,
				"exact enemy defensive QRF returned to its origin");
		}

		bool changed;
		HST_StrategicMovementResult movement = m_StrategicMovement.AdvanceExactPlayerQRF(state, operation, group);
		if (!movement || !movement.m_bAccepted)
		{
			return SettleOperation(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
				"route_failed_survivors",
				living,
				"exact enemy defensive QRF strategic movement was rejected");
		}
		changed = movement.m_bStateChanged;
		if (movement.m_bArrived)
		{
			if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
			{
				HST_OperationTransitionResult onStation = m_Operations.MarkExactEnemyDefensiveQRFOnStation(state, order, group);
				if (!onStation || !onStation.m_bAccepted)
				{
					return SettleOperation(
						state,
						enemyDirector,
						order,
						operation,
						manifest,
						batch,
						group,
						HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
						"invalidated_survivors",
						living,
						"exact enemy defensive QRF virtual arrival transition failed") || changed;
				}
				group.m_sRuntimeStatus = "enemy_qrf_virtual_on_station";
				changed = ApplyDefensiveArrivalOutcome(state, preset, order, operation, batch) || true;
			}
			else if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
			{
				return SettleOperation(
					state,
					enemyDirector,
					order,
					operation,
					manifest,
					batch,
					group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED,
					"returned_survivors",
					living,
					"exact enemy defensive QRF returned to its origin") || changed;
			}
		}

		HST_OperationProjectionDecision decision = m_Materialization.EvaluateExactPlayerQRF(operation, operation.m_vStrategicPosition);
		operation.m_sLastProjectionReason = decision.m_sReason;
		if (decision.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE)
			return BeginMaterialization(state, enemyDirector, order, operation, manifest, batch, group, decision.m_sReason) || changed;

		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
		{
			order.m_sRuntimeStatus = "exact_virtual_returning";
			group.m_sRuntimeStatus = "enemy_qrf_virtual_returning";
		}
		else
		{
			order.m_sRuntimeStatus = "exact_virtual_outbound";
			group.m_sRuntimeStatus = "enemy_qrf_virtual";
		}
		return changed;
	}

	protected bool TickMaterializing(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED
			|| group.m_sRuntimeStatus == "spawn_failed")
		{
			return SettleOperation(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"materialization_failed_survivors",
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy defensive QRF materialization failed");
		}
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED || !group.m_bSpawnedEntity)
			return false;

		HST_OperationTransitionResult physical = m_Operations.MarkExactEnemyDefensiveQRFPhysical(
			state,
			order,
			group,
			batch,
			"exact enemy defensive QRF materialized at the strategic cursor");
		if (!physical || !physical.m_bAccepted)
		{
			return RetireAndSettlePhysicalFailure(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"invalidated_survivors",
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy defensive QRF physical handoff conflicts with operation authority");
		}
		bool routeRestarted = m_PhysicalWar.RestartExactEnemyQRFInfantryRoute(
			state,
			group,
			operation.m_vRouteEndPosition,
			"Exact enemy defensive QRF following its authoritative live route.");
		if (!routeRestarted)
		{
			return RetireAndSettlePhysicalFailure(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
				"route_failed_survivors",
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy defensive QRF live route could not be issued");
		}
		order.m_sRuntimeStatus = "exact_physical";
		return true;
	}

	protected bool TickPhysical(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			return SettlePhysicalFailure(state, enemyDirector, order, operation, manifest, batch, group, 0, "exact enemy defensive QRF physical batch is not successful");
		if (!group.m_bSpawnedEntity || group.m_sRuntimeStatus == "spawn_failed"
			|| group.m_sRuntimeStatus.Contains("runtime_binding_missing"))
		{
			return SettlePhysicalFailure(state, enemyDirector, order, operation, manifest, batch, group, 0, "exact enemy defensive QRF physical runtime binding is missing or ambiguous");
		}

		HST_OperationTransitionResult live = m_Operations.UpdateExactEnemyDefensiveQRFPhysicalPosition(state, order, group);
		if (!live || !live.m_bAccepted)
		{
			return RetireAndSettlePhysicalFailure(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"invalidated_survivors",
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy defensive QRF live-position handoff failed");
		}
		bool changed = live.m_bStateChanged;
		int living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
		SyncGroupRoster(group, living);
		if (living <= 0 || group.m_sRuntimeStatus == "eliminated")
		{
			string eliminationReason;
			bool eliminationFinalized = m_PhysicalWar.FinalizeEliminatedForceSpawnProjection(
				state,
				group,
				state.m_iElapsedSeconds,
				eliminationReason);
			if (!eliminationFinalized)
			{
				return SetRuntimeConflict(
					order,
					"exact enemy defensive QRF elimination cleanup is pending: " + eliminationReason) || changed;
			}
			return SettleOperation(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
				"destroyed",
				0,
				"exact enemy defensive QRF was eliminated") || changed;
		}
		if (m_PhysicalWar.IsExactEnemyQRFRouteRecoveryExhausted(group, state.m_iElapsedSeconds))
		{
			return RetireAndSettlePhysicalFailure(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
				"route_failed_survivors",
				living,
				"exact enemy defensive QRF exhausted bounded live-route recovery") || changed;
		}

		HST_OperationTransitionResult sample = m_Operations.ConfirmExactEnemyDefensiveQRFArrivalSample(state, order, group);
		if (!sample || !sample.m_bAccepted)
		{
			return RetireAndSettlePhysicalFailure(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"invalidated_survivors",
				living,
				"exact enemy defensive QRF live arrival sampling failed") || changed;
		}
		changed = sample.m_bStateChanged || changed;
		if (operation.m_iArrivalConfirmationCount >= 2)
		{
			if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
			{
				HST_OperationTransitionResult onStation = m_Operations.MarkExactEnemyDefensiveQRFOnStation(state, order, group);
				if (!onStation || !onStation.m_bAccepted)
				{
					return RetireAndSettlePhysicalFailure(
						state,
						enemyDirector,
						order,
						operation,
						manifest,
						batch,
						group,
						HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
						"invalidated_survivors",
						living,
						"exact enemy defensive QRF physical arrival transition failed") || changed;
				}
				group.m_sRuntimeStatus = "enemy_qrf_on_station";
				changed = ApplyDefensiveArrivalOutcome(state, preset, order, operation, batch) || true;
				return changed;
			}
			if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
				return TryDematerialize(state, enemyDirector, order, operation, manifest, batch, group, "return arrival folded for exact survivor settlement") || changed;
		}

		HST_OperationProjectionDecision decision = m_Materialization.EvaluateExactPlayerQRF(operation, group.m_vPosition);
		operation.m_sLastProjectionReason = decision.m_sReason;
		if (decision.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_DEMATERIALIZE)
			return TryDematerialize(state, enemyDirector, order, operation, manifest, batch, group, decision.m_sReason) || changed;
		return changed;
	}

	protected bool BeginMaterialization(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		int deadlineSecond = state.m_iElapsedSeconds + EXACT_QRF_DEPLOYMENT_GRACE_SECONDS;
		HST_ForceSpawnQueueCallbackResult released = m_SpawnQueue.ReleaseStrategicProjectionForMaterialization(
			state.m_aForceSpawnResults,
			manifest,
			batch.m_sResultId,
			batch.m_sProjectionId,
			state.m_iElapsedSeconds,
			deadlineSecond);
		if (!released || !released.m_bAccepted)
		{
			return SettleOperation(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"invalidated_survivors",
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy defensive QRF materialization release failed");
		}
		group.m_vPosition = operation.m_vStrategicPosition;
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		group.m_vTargetPosition = operation.m_vRouteEndPosition;
		group.m_sRuntimeStatus = "enemy_qrf_materializing";
		HST_OperationTransitionResult materializing = m_Operations.MarkExactEnemyDefensiveQRFMaterializingFromVirtual(
			state,
			order,
			group,
			batch,
			reason);
		if (!materializing || !materializing.m_bAccepted)
		{
			m_SpawnQueue.HoldPendingProjectionForStrategicSimulation(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				state.m_iElapsedSeconds);
			group.m_sRuntimeStatus = "enemy_qrf_virtual";
			return SettleOperation(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"invalidated_survivors",
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy defensive QRF materialization transition failed");
		}
		order.m_sRuntimeStatus = "exact_materializing";
		return true;
	}

	protected bool TryDematerialize(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		int deadlineSecond = state.m_iElapsedSeconds + EXACT_QRF_DEPLOYMENT_GRACE_SECONDS;
		HST_ForceSpawnQueueCallbackResult preflight = m_SpawnQueue.CanRequeueSuccessfulProjectionForStrategicHold(
			state.m_aForceSpawnResults,
			manifest,
			batch.m_sResultId,
			batch.m_sProjectionId,
			state.m_iElapsedSeconds,
			deadlineSecond);
		if (!preflight || !preflight.m_bAccepted)
			return SetRuntimeConflict(order, "exact enemy defensive QRF dematerialization is waiting for queue capacity");
		HST_OperationTransitionResult begun = m_Operations.BeginExactEnemyDefensiveQRFDematerialization(state, order, group, reason);
		if (!begun || !begun.m_bAccepted)
		{
			return RetireAndSettlePhysicalFailure(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"invalidated_survivors",
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy defensive QRF dematerialization transition failed");
		}
		return ContinueDematerialization(state, enemyDirector, order, begun.m_Operation, manifest, batch, group, reason);
	}

	protected bool ContinueDematerialization(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		if (!batch.m_bStrategicProjectionHeld)
		{
			HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
				state,
				m_PhysicalWar,
				group.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return SetRuntimeConflict(order, "exact enemy defensive QRF runtime retirement is pending");
			int deadlineSecond = state.m_iElapsedSeconds + EXACT_QRF_DEPLOYMENT_GRACE_SECONDS;
			HST_ForceSpawnQueueCallbackResult held = m_SpawnQueue.RequeueSuccessfulProjectionForStrategicHold(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				state.m_iElapsedSeconds,
				deadlineSecond);
			if (!held || !held.m_bAccepted)
				return SetRuntimeConflict(order, "exact enemy defensive QRF survivor projection could not enter strategic hold");
		}

		m_StrategicMovement.SyncRouteProgressFromPosition(operation, group.m_vPosition);
		group.m_bSpawnedEntity = false;
		group.m_sRuntimeEntityId = "";
		group.m_sRuntimeStatus = "enemy_qrf_virtual";
		group.m_iSpawnedAgentCount = 0;
		SyncGroupRoster(group, m_SpawnQueue.CountStrategicLivingMemberSlots(batch));
		group.m_iLifecycleRevision++;
		HST_OperationTransitionResult completed = m_Operations.CompleteExactEnemyDefensiveQRFDematerialization(
			state,
			order,
			group,
			batch,
			reason);
		if (!completed || !completed.m_bAccepted)
		{
			return SettleOperation(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"invalidated_survivors",
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy defensive QRF dematerialization completion failed");
		}
		order.m_bPhysicalized = false;
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
			order.m_sRuntimeStatus = "exact_virtual_returning";
		else
			order.m_sRuntimeStatus = "exact_virtual_outbound";
		return true;
	}

	protected bool ApplyDefensiveArrivalOutcome(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch)
	{
		if (!state || !order || order.m_bOutcomeApplied)
			return false;
		HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
		int living = ResolveSettlementSurvivors(operation, state.FindForceManifest(order.m_sManifestId), batch, state.FindActiveGroup(order.m_sGroupId));
		if (!targetZone || targetZone.m_sOwnerFactionKey != order.m_sFactionKey)
			order.m_sResolutionKind = "exact_defensive_qrf_target_changed_no_pressure";
		else
		{
			int pressureReduction = Math.Max(1, Math.Min(20, living * 3));
			targetZone.m_iResistanceCaptureProgress = Math.Max(0, targetZone.m_iResistanceCaptureProgress - pressureReduction);
			order.m_sResolutionKind = string.Format("exact_defensive_qrf_arrival_pressure_%1", pressureReduction);
		}
		order.m_bOutcomeApplied = true;
		order.m_bAbstractResolved = operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		return true;
	}

	protected bool SettleOperation(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_EOperationTerminalResult terminalResult,
		string settlementKind,
		int survivors,
		string reason)
	{
		if (!state || !enemyDirector || !order || !operation || !manifest)
			return false;
		string settlementId = HST_OperationService.BuildSettlementId(operation.m_sOperationId, settlementKind);
		HST_OperationTransitionResult prepared = m_Operations.CanPrepareExactEnemyDefensiveQRFSettlement(
			state,
			order,
			terminalResult,
			settlementId);
		if (!prepared || !prepared.m_bAccepted)
			return SetRuntimeConflict(order, "exact enemy defensive QRF settlement eligibility preflight failed");
		if (prepared.m_bAlreadyApplied)
		{
			HST_OperationTransitionResult replay = m_Operations.CanSettleExactEnemyDefensiveQRF(
				state,
				order,
				terminalResult,
				settlementId);
			if (!replay || !replay.m_bAccepted)
				return SetRuntimeConflict(order, "exact enemy defensive QRF settled receipt conflicts with its resource ledger");
			return FinalizeSettledOrder(state, order, replay.m_Operation, batch, group);
		}
		if (!ApplyResourceSettlement(state, enemyDirector, order, manifest, settlementKind, survivors, false, reason))
			return SetRuntimeConflict(order, "exact enemy defensive QRF resource settlement conflicted");
		HST_OperationTransitionResult preflight = m_Operations.CanSettleExactEnemyDefensiveQRF(
			state,
			order,
			terminalResult,
			settlementId);
		if (!preflight || !preflight.m_bAccepted)
			return SetRuntimeConflict(order, "exact enemy defensive QRF settlement preflight failed");
		if (preflight.m_bAlreadyApplied)
			return FinalizeSettledOrder(state, order, preflight.m_Operation, batch, group);
		HST_OperationTransitionResult settled = m_Operations.SettleExactEnemyDefensiveQRF(
			state,
			order,
			terminalResult,
			settlementId,
			reason);
		if (!settled || !settled.m_bAccepted)
			return SetRuntimeConflict(order, "exact enemy defensive QRF operation settlement failed");

		return FinalizeSettledOrder(state, order, settled.m_Operation, batch, group);
	}

	protected bool FinalizeSettledOrder(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!state || !order || !operation
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return false;
		bool changed;
		HST_EEnemyOrderStatus status = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		if (operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
			|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED)
			status = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED;
		if (order.m_eStatus != status)
		{
			order.m_eStatus = status;
			changed = true;
		}
		int resolvedSecond = Math.Max(0, operation.m_iSettledAtSecond);
		if (order.m_iResolvedAtSecond != resolvedSecond)
		{
			order.m_iResolvedAtSecond = resolvedSecond;
			changed = true;
		}
		if (order.m_bPhysicalized || order.m_bAbstractResolved)
		{
			order.m_bPhysicalized = false;
			order.m_bAbstractResolved = false;
			changed = true;
		}
		string runtimeStatus = "resolved_exact_terminal";
		if (operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED)
			runtimeStatus = "resolved_exact_returned";
		if (order.m_sRuntimeStatus != runtimeStatus)
		{
			order.m_sRuntimeStatus = runtimeStatus;
			changed = true;
		}
		if (order.m_sResolutionKind.IsEmpty())
		{
			order.m_sResolutionKind = order.m_sResourceSettlementKind;
			changed = true;
		}
		if (order.m_sFailureReason != operation.m_sTerminalReason)
		{
			order.m_sFailureReason = operation.m_sTerminalReason;
			changed = true;
		}
		bool runtimeReleased = true;
		if (batch && (!m_SpawnAdapter || m_SpawnAdapter.CountHandlesForProjection(batch.m_sProjectionId) > 0))
			runtimeReleased = false;
		if (group && (!m_PhysicalWar || m_PhysicalWar.GetForceSpawnGroupRoot(group)
			|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0))
			runtimeReleased = false;
		if (batch && runtimeReleased)
		{
			int batchIndex = state.m_aForceSpawnResults.Find(batch);
			if (batchIndex >= 0)
			{
				state.m_aForceSpawnResults.Remove(batchIndex);
				changed = true;
			}
		}
		if (group && runtimeReleased)
		{
			int groupIndex = state.m_aActiveGroups.Find(group);
			if (groupIndex >= 0)
			{
				state.m_aActiveGroups.Remove(groupIndex);
				changed = true;
			}
		}
		return changed;
	}

	protected bool FailClosedActiveOrder(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		if (!state || !enemyDirector || !order)
			return SetRuntimeConflict(order, reason);
		if (operation && operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			&& manifest && batch && group)
		{
			return SettlePhysicalFailure(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				reason);
		}
		if (operation && manifest)
		{
			return SettleOperation(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"invalidated_survivors",
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				reason);
		}
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		order.m_iResolvedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		order.m_sRuntimeStatus = "exact_operation_invalidated";
		order.m_sFailureReason = reason;
		string invalidatedSettlementKind = "invalidated_full";
		if (order.m_bStrategicServiceCommitted)
			invalidatedSettlementKind = "invalidated_survivors";
		bool refunded = ApplyResourceSettlement(
			state,
			enemyDirector,
			order,
			manifest,
			invalidatedSettlementKind,
			ResolveSettlementSurvivors(operation, manifest, batch, group),
			!order.m_bStrategicServiceCommitted,
			reason);
		if (!refunded)
		{
			order.m_sRuntimeStatus = "exact_settlement_conflict";
			order.m_sFailureReason = reason + ": resource settlement was not applied";
		}
		return true;
	}

	protected bool SettlePhysicalFailure(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		int provenSurvivors,
		string reason)
	{
		return RetireAndSettlePhysicalFailure(
			state,
			enemyDirector,
			order,
			operation,
			manifest,
			batch,
			group,
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
			"invalidated_survivors",
			provenSurvivors,
			reason);
	}

	protected bool RetireAndSettlePhysicalFailure(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_EOperationTerminalResult terminalResult,
		string settlementKind,
		int provenSurvivors,
		string reason)
	{
		if (!state || !enemyDirector || !order || !operation || !manifest || !group)
			return SetRuntimeConflict(order, reason);
		int handleCount;
		if (m_SpawnAdapter)
			handleCount = m_SpawnAdapter.CountHandlesForProjection(group.m_sProjectionId);
		if ((group.m_bSpawnedEntity || handleCount > 0) && batch
			&& batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
				state,
				m_PhysicalWar,
				group.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return SetRuntimeConflict(order, "exact enemy defensive QRF fail-closed runtime retirement is pending: " + reason);
		}
		else if (handleCount > 0)
			return SetRuntimeConflict(order, "exact enemy defensive QRF ambiguous handles cannot be retired safely: " + reason);
		group.m_bSpawnedEntity = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		return SettleOperation(
			state,
			enemyDirector,
			order,
			operation,
			manifest,
			batch,
			group,
			terminalResult,
			settlementKind,
			Math.Max(0, provenSurvivors),
			reason);
	}

	protected void FailAdmissionAfterDebit(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_EnemyDirectorService enemyDirector,
		string reason)
	{
		if (!state || !order || !enemyDirector)
			return;
		if (HasCommittedAdmissionAuthority(state, order))
		{
			SetRuntimeConflict(order, "committed exact enemy defensive QRF admission cannot be rolled back: " + reason);
			return;
		}
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(order.m_sSpawnResultId);
		if (!batch)
			batch = state.FindForceSpawnResultByRequest(order.m_sOrderId);
		if (batch && m_SpawnQueue && batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			m_SpawnQueue.RequestCancel(state.m_aForceSpawnResults, batch.m_sResultId, state.m_iElapsedSeconds, reason);
		HST_ActiveGroupState group = state.FindActiveGroup(BuildProjectionId(order));
		if (group && !group.m_bSpawnedEntity)
		{
			int groupIndex = state.m_aActiveGroups.Find(group);
			if (groupIndex >= 0)
				state.m_aActiveGroups.Remove(groupIndex);
		}
		int admissionAcceptedCount = Math.Max(0, order.m_iCompositionManpower);
		if (manifest)
			admissionAcceptedCount = manifest.m_iAcceptedMemberCount;
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		string operationSettlementId = HST_OperationService.BuildSettlementId(order.m_sOperationId, "admission_failed_full");
		if (operation && manifest)
		{
			HST_OperationTransitionResult eligibility = m_Operations.CanPrepareExactEnemyDefensiveQRFSettlement(
				state,
				order,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				operationSettlementId);
			if (!eligibility || !eligibility.m_bAccepted)
			{
				HST_OperationTransitionResult removed = m_Operations.RemoveUncommittedExactEnemyDefensiveQRF(state, order, manifest);
				if (removed && removed.m_bAccepted)
					operation = null;
			}
		}
		bool refunded = ApplyResourceSettlement(
			state,
			enemyDirector,
			order,
			manifest,
			"admission_failed_full",
			admissionAcceptedCount,
			true,
			reason);
		if (operation && manifest)
		{
			HST_OperationTransitionResult preflight = m_Operations.CanSettleExactEnemyDefensiveQRF(
				state,
				order,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				operationSettlementId);
			if (preflight && preflight.m_bAccepted && !preflight.m_bAlreadyApplied)
				m_Operations.SettleExactEnemyDefensiveQRF(
					state,
					order,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
					operationSettlementId,
					reason);
		}
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		order.m_iResolvedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		if (refunded)
		{
			order.m_sRuntimeStatus = "exact_admission_failed_refunded";
			order.m_sResolutionKind = "exact_admission_failed_full_refund";
			order.m_sFailureReason = reason;
		}
		else
		{
			order.m_sRuntimeStatus = "exact_admission_settlement_conflict";
			order.m_sResolutionKind = "exact_admission_failed_unsettled";
			order.m_sFailureReason = reason + ": resource settlement was not applied";
		}
	}

	protected bool ApplyResourceSettlement(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		string settlementKind,
		int survivorCount,
		bool fullRefund,
		string reason)
	{
		if (!state || !enemyDirector || !order || settlementKind.IsEmpty())
			return false;
		int accepted = Math.Max(0, order.m_iCompositionManpower);
		if (manifest)
			accepted = manifest.m_iAcceptedMemberCount;
		if (accepted <= 0)
			return false;
		int survivors = Math.Max(0, Math.Min(accepted, survivorCount));
		int attackRefund;
		int supportRefund;
		if (fullRefund)
		{
			attackRefund = Math.Max(0, order.m_iAttackCost);
			supportRefund = Math.Max(0, order.m_iSupportCost);
		}
		else
		{
			attackRefund = Math.Max(0, order.m_iAttackCost) * survivors / accepted;
			supportRefund = Math.Max(0, order.m_iSupportCost) * survivors / accepted;
		}
		string settlementId = HST_OperationService.BuildSettlementId(order.m_sOperationId, settlementKind);
		if (order.m_bResourceSettlementApplied)
		{
			return order.m_sResourceSettlementId == settlementId
				&& order.m_sResourceSettlementKind == settlementKind
				&& order.m_iSettlementAcceptedMemberCount == accepted
				&& order.m_iSettlementSurvivorMemberCount == survivors
				&& order.m_iRefundedAttackResources == attackRefund
				&& order.m_iRefundedSupportResources == supportRefund;
		}
		if (HasPartialResourceSettlementAuthority(order))
			return false;
		if ((attackRefund > 0 || supportRefund > 0) && !state.FindFactionPool(order.m_sFactionKey))
			return false;

		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		if (operation)
		{
			HST_OperationTransitionResult recorded = m_Operations.RecordExactEnemyDefensiveQRFResourceSettlement(
				state,
				order,
				settlementKind,
				accepted,
				survivors);
			if (!recorded || !recorded.m_bAccepted)
			{
				if (!fullRefund || order.m_bStrategicServiceCommitted)
					return false;
				order.m_sResourceSettlementId = settlementId;
				order.m_sResourceSettlementKind = settlementKind;
				order.m_iSettlementAcceptedMemberCount = accepted;
				order.m_iSettlementSurvivorMemberCount = survivors;
				order.m_bResourceSettlementApplied = true;
			}
		}
		else
		{
			order.m_sResourceSettlementId = settlementId;
			order.m_sResourceSettlementKind = settlementKind;
			order.m_iSettlementAcceptedMemberCount = accepted;
			order.m_iSettlementSurvivorMemberCount = survivors;
			order.m_bResourceSettlementApplied = true;
		}

		enemyDirector.RefundDefenseResources(
			state,
			order.m_sFactionKey,
			order.m_sTargetZoneId,
			attackRefund,
			supportRefund,
			reason);
		order.m_iRefundedAttackResources = attackRefund;
		order.m_iRefundedSupportResources = supportRefund;
		return true;
	}

	protected string ValidateAdmissionContext(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_EnemyDirectorService enemyDirector)
	{
		if (!state || !order || !manifest || !enemyDirector || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar)
			return "exact enemy defensive QRF admission services are unavailable";
		if (!IsExactEnemyDefensiveQRF(order) || order.m_sOrderId.IsEmpty()
			|| order.m_sOperationId != HST_StableIdService.BuildOperationId("enemy_order", order.m_sOrderId))
			return "exact enemy defensive QRF order identity is invalid";
		if (!manifest.m_bFrozen || manifest.m_sOperationId != order.m_sOperationId
			|| manifest.m_sForceKind != HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_FORCE_KIND
			|| manifest.m_sFactionKey != order.m_sFactionKey
			|| manifest.m_sSourceZoneId != order.m_sSourceZoneId || manifest.m_sTargetZoneId != order.m_sTargetZoneId
			|| manifest.m_iAttackResourceCost != order.m_iAttackCost || manifest.m_iSupportResourceCost != order.m_iSupportCost
			|| !m_StrategicMovement.IsSupportedExactInfantryManifest(manifest)
			|| manifest.m_sManifestHash.IsEmpty() || m_Integrity.BuildManifestHash(manifest) != manifest.m_sManifestHash)
			return "exact enemy defensive QRF frozen manifest is invalid";
		HST_QRFState legacy = state.FindActiveQRF(order.m_sTargetZoneId, order.m_sFactionKey);
		if (legacy)
			return "legacy QRF already owns this faction and target";
		foreach (HST_EnemyOrderState other : state.m_aEnemyOrders)
		{
			if (!other || other == order || other.m_eType != HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF)
				continue;
			bool otherOpen = other.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
				|| other.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED;
			if (other.m_sOperationId == order.m_sOperationId || (otherOpen
				&& other.m_sFactionKey == order.m_sFactionKey
				&& HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					other.m_sTargetZoneId, order.m_sTargetZoneId)))
				return "another enemy QRF already owns this identity or target";
		}
		return "";
	}

	protected string ResolveRuntimeContext(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		out HST_OperationRecordState operation,
		out HST_ForceManifestState manifest,
		out HST_ForceSpawnResultState batch,
		out HST_ActiveGroupState group)
	{
		operation = null;
		manifest = null;
		batch = null;
		group = null;
		string ambiguity = FindAmbiguousAuthorityRows(state, order);
		if (!ambiguity.IsEmpty())
			return ambiguity;
		operation = state.FindOperation(order.m_sOperationId);
		manifest = state.FindForceManifest(order.m_sManifestId);
		batch = state.FindForceSpawnResult(order.m_sSpawnResultId);
		group = state.FindActiveGroup(order.m_sGroupId);
		if (!operation || !manifest || !batch || !group)
			return "exact enemy defensive QRF runtime authority is incomplete";
		string failure = m_Operations.ValidateExactEnemyDefensiveQRF(state, operation, order, manifest);
		if (!failure.IsEmpty())
			return failure;
		if (operation.m_sSpawnResultId != batch.m_sResultId || operation.m_sGroupId != group.m_sGroupId
			|| batch.m_sOperationId != operation.m_sOperationId || batch.m_sManifestId != manifest.m_sManifestId
			|| group.m_sOperationId != operation.m_sOperationId || group.m_sEnemyOrderId != order.m_sOrderId
			|| group.m_sManifestId != manifest.m_sManifestId || group.m_sSpawnResultId != batch.m_sResultId
			|| group.m_sProjectionId != batch.m_sProjectionId || group.m_sForceId != batch.m_sForceId)
			return "exact enemy defensive QRF reciprocal runtime links conflict";
		if (CountForceSpawnResultId(state, batch.m_sResultId) != 1)
			return "exact enemy defensive QRF spawn-result identity is ambiguous";
		if (CountActiveGroupId(state, group.m_sGroupId) != 1)
			return "exact enemy defensive QRF active-group identity is ambiguous";
		return "";
	}

	protected HST_ActiveGroupState BuildActiveGroup(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		string resultId,
		string forceId,
		string projectionId)
	{
		if (!state || !order || !manifest || manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
			return null;
		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = projectionId;
		group.m_sOperationId = order.m_sOperationId;
		group.m_sEnemyOrderId = order.m_sOrderId;
		group.m_sManifestId = manifest.m_sManifestId;
		group.m_sSpawnResultId = resultId;
		group.m_sForceId = forceId;
		group.m_sProjectionId = projectionId;
		group.m_sZoneId = order.m_sTargetZoneId;
		group.m_sFactionKey = order.m_sFactionKey;
		group.m_sPrefab = manifest.m_aGroups[0].m_sPrefab;
		group.m_sCompositionRequestId = manifest.m_sManifestId;
		group.m_sCompositionIntentId = manifest.m_sIntentId;
		group.m_sCompositionTier = "exact";
		group.m_sCompositionSummary = string.Format("%1 exact infantry", manifest.m_iAcceptedMemberCount);
		group.m_sSpawnFallbackMode = EXACT_QRF_GROUP_MODE;
		group.m_sRouteId = order.m_sOperationId + "_outbound";
		group.m_vSourcePosition = order.m_vSourcePosition;
		group.m_vTargetPosition = order.m_vTargetPosition;
		group.m_vPosition = order.m_vSourcePosition;
		group.m_sRuntimeStatus = "enemy_qrf_virtual";
		group.m_iInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iOriginalInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iCompositionCost = manifest.m_iAttackResourceCost + manifest.m_iSupportResourceCost;
		group.m_iCompositionManpower = manifest.m_iAcceptedMemberCount;
		group.m_iLastSeenAliveCount = manifest.m_iAcceptedMemberCount;
		group.m_iSurvivorInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iDurableLivingInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iSpawnedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		group.m_bQRF = true;
		return group;
	}

	protected string ValidateActiveGroup(
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_ActiveGroupState group,
		string resultId,
		string forceId,
		string projectionId)
	{
		if (!order || !manifest || !group)
			return "exact enemy defensive QRF active-group context is missing";
		if (group.m_sGroupId != projectionId || group.m_sProjectionId != projectionId
			|| group.m_sForceId != forceId || group.m_sSpawnResultId != resultId
			|| group.m_sOperationId != order.m_sOperationId || group.m_sEnemyOrderId != order.m_sOrderId
			|| group.m_sManifestId != manifest.m_sManifestId || group.m_sFactionKey != order.m_sFactionKey
			|| group.m_iOriginalInfantryCount != manifest.m_iAcceptedMemberCount || group.m_iVehicleCount != 0)
			return "exact enemy defensive QRF active-group identity or roster conflicts";
		return "";
	}

	protected int ResolveSettlementSurvivors(
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		int acceptedFallback = 0)
	{
		int accepted = Math.Max(0, acceptedFallback);
		if (group)
			accepted = Math.Max(0, group.m_iOriginalInfantryCount);
		if (operation && operation.m_iLastVirtualFriendlyCount > accepted)
			accepted = operation.m_iLastVirtualFriendlyCount;
		if (manifest)
			accepted = manifest.m_iAcceptedMemberCount;
		int survivors;
		bool authoritativeBatchRoster;
		if (batch && m_SpawnQueue)
		{
			if (batch.m_bStrategicProjectionHeld)
			{
				survivors = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
				authoritativeBatchRoster = true;
			}
			else if (batch.m_iSuccessfulHandoffCount > 0)
			{
				survivors = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
				authoritativeBatchRoster = true;
			}
		}
		if (authoritativeBatchRoster)
			return Math.Max(0, Math.Min(accepted, survivors));
		if (group && group.m_sRuntimeStatus == "eliminated")
			return 0;
		if (survivors <= 0 && operation)
			survivors = operation.m_iLastVirtualFriendlyCount;
		if (survivors <= 0 && group && group.m_sRuntimeStatus != "eliminated")
			survivors = Math.Max(group.m_iDurableLivingInfantryCount, group.m_iSurvivorInfantryCount);
		return Math.Max(0, Math.Min(accepted, survivors));
	}

	protected void SyncGroupRoster(HST_ActiveGroupState group, int living)
	{
		if (!group)
			return;
		int bounded = Math.Max(0, Math.Min(group.m_iOriginalInfantryCount, living));
		group.m_iDurableLivingInfantryCount = bounded;
		group.m_iLastSeenAliveCount = bounded;
		group.m_iSurvivorInfantryCount = bounded;
	}

	protected bool SetRuntimeConflict(HST_EnemyOrderState order, string reason)
	{
		if (!order)
			return false;
		string status = "exact_runtime_conflict";
		if (order.m_sRuntimeStatus == status && order.m_sFailureReason == reason)
			return false;
		order.m_sRuntimeStatus = status;
		order.m_sFailureReason = reason;
		return true;
	}

	protected bool QuarantineRuntimeAuthority(HST_EnemyOrderState order, string reason)
	{
		if (!order)
			return false;
		string status = "exact_runtime_authority_quarantined";
		string failure = "exact enemy defensive QRF runtime authority is quarantined; no roster refund or runtime retirement was attempted: " + reason;
		if (order.m_sRuntimeStatus == status && order.m_sFailureReason == failure)
			return false;
		order.m_sRuntimeStatus = status;
		order.m_sFailureReason = failure;
		return true;
	}

	protected string FindAmbiguousAuthorityRows(HST_CampaignState state, HST_EnemyOrderState order)
	{
		if (!state || !order)
			return "exact enemy defensive QRF authority context is missing";
		if (state.FindEnemyOrder(order.m_sOrderId) != order
			|| CountEnemyOrdersByAnyAuthorityIdentity(state, order) != 1)
			return "exact enemy defensive QRF enemy-order identity is ambiguous";
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		int operationCount = CountOperationsByAnyAuthorityIdentity(state, order);
		if ((operation && operationCount != 1) || (!operation && operationCount > 0))
			return "exact enemy defensive QRF operation identity is ambiguous";
		HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
		int manifestCount = CountForceManifestsByAnyAuthorityIdentity(state, order);
		if ((manifest && manifestCount != 1) || (!manifest && manifestCount > 0))
			return "exact enemy defensive QRF manifest identity is ambiguous";
		string resultId = order.m_sSpawnResultId;
		string groupId = order.m_sGroupId;
		if (operation)
		{
			if (resultId.IsEmpty())
				resultId = operation.m_sSpawnResultId;
			if (groupId.IsEmpty())
				groupId = operation.m_sGroupId;
		}
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(resultId);
		int batchCount = CountForceSpawnResultsByAnyAuthorityIdentity(state, order, operation, batch);
		if ((batch && batchCount != 1) || (!batch && batchCount > 0))
			return "exact enemy defensive QRF spawn-result identity is ambiguous";
		HST_ActiveGroupState group = state.FindActiveGroup(groupId);
		int groupCount = CountActiveGroupsByAnyAuthorityIdentity(state, order, operation, group);
		if ((group && groupCount != 1) || (!group && groupCount > 0))
			return "exact enemy defensive QRF active-group identity is ambiguous";
		return "";
	}

	protected int CountEnemyOrdersByAnyAuthorityIdentity(HST_CampaignState state, HST_EnemyOrderState expected)
	{
		int count;
		if (!state || !expected)
			return count;
		foreach (HST_EnemyOrderState candidate : state.m_aEnemyOrders)
		{
			if (!candidate)
				continue;
			bool matches = candidate.m_sOrderId == expected.m_sOrderId;
			if (!matches && !expected.m_sOperationId.IsEmpty())
				matches = candidate.m_sOperationId == expected.m_sOperationId;
			if (!matches && !expected.m_sManifestId.IsEmpty())
				matches = candidate.m_sManifestId == expected.m_sManifestId;
			if (!matches && !expected.m_sSpawnResultId.IsEmpty())
				matches = candidate.m_sSpawnResultId == expected.m_sSpawnResultId;
			if (!matches && !expected.m_sGroupId.IsEmpty())
				matches = candidate.m_sGroupId == expected.m_sGroupId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountOperationsByAnyAuthorityIdentity(HST_CampaignState state, HST_EnemyOrderState order)
	{
		int count;
		if (!state || !order)
			return count;
		foreach (HST_OperationRecordState candidate : state.m_aOperations)
		{
			if (!candidate)
				continue;
			bool matches = candidate.m_sOperationId == order.m_sOperationId;
			if (!matches && !order.m_sOrderId.IsEmpty())
				matches = candidate.m_sEnemyOrderId == order.m_sOrderId;
			if (!matches && !order.m_sManifestId.IsEmpty())
				matches = candidate.m_sManifestId == order.m_sManifestId;
			if (!matches && !order.m_sSpawnResultId.IsEmpty())
				matches = candidate.m_sSpawnResultId == order.m_sSpawnResultId;
			if (!matches && !order.m_sGroupId.IsEmpty())
				matches = candidate.m_sGroupId == order.m_sGroupId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountForceManifestsByAnyAuthorityIdentity(HST_CampaignState state, HST_EnemyOrderState order)
	{
		int count;
		if (!state || !order)
			return count;
		foreach (HST_ForceManifestState candidate : state.m_aForceManifests)
		{
			if (!candidate)
				continue;
			bool matches = candidate.m_sManifestId == order.m_sManifestId;
			if (!matches && !order.m_sOperationId.IsEmpty())
				matches = candidate.m_sOperationId == order.m_sOperationId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountForceSpawnResultsByAnyAuthorityIdentity(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState expected)
	{
		int count;
		if (!state || !order)
			return count;
		string resultId = order.m_sSpawnResultId;
		string projectionId;
		string forceId;
		if (operation)
		{
			if (resultId.IsEmpty())
				resultId = operation.m_sSpawnResultId;
			projectionId = operation.m_sProjectionId;
			forceId = operation.m_sForceId;
		}
		if (expected)
		{
			if (resultId.IsEmpty())
				resultId = expected.m_sResultId;
			if (projectionId.IsEmpty())
				projectionId = expected.m_sProjectionId;
			if (forceId.IsEmpty())
				forceId = expected.m_sForceId;
		}
		foreach (HST_ForceSpawnResultState candidate : state.m_aForceSpawnResults)
		{
			if (!candidate)
				continue;
			bool matches = !resultId.IsEmpty() && candidate.m_sResultId == resultId;
			if (!matches && !order.m_sOrderId.IsEmpty())
				matches = candidate.m_sRequestId == order.m_sOrderId;
			if (!matches && !order.m_sOperationId.IsEmpty())
				matches = candidate.m_sOperationId == order.m_sOperationId;
			if (!matches && !order.m_sManifestId.IsEmpty())
				matches = candidate.m_sManifestId == order.m_sManifestId;
			if (!matches && !projectionId.IsEmpty())
				matches = candidate.m_sProjectionId == projectionId;
			if (!matches && !forceId.IsEmpty())
				matches = candidate.m_sForceId == forceId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountActiveGroupsByAnyAuthorityIdentity(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ActiveGroupState expected)
	{
		int count;
		if (!state || !order)
			return count;
		string groupId = order.m_sGroupId;
		string resultId = order.m_sSpawnResultId;
		string projectionId;
		string forceId;
		if (operation)
		{
			if (groupId.IsEmpty())
				groupId = operation.m_sGroupId;
			if (resultId.IsEmpty())
				resultId = operation.m_sSpawnResultId;
			projectionId = operation.m_sProjectionId;
			forceId = operation.m_sForceId;
		}
		if (expected)
		{
			if (groupId.IsEmpty())
				groupId = expected.m_sGroupId;
			if (resultId.IsEmpty())
				resultId = expected.m_sSpawnResultId;
			if (projectionId.IsEmpty())
				projectionId = expected.m_sProjectionId;
			if (forceId.IsEmpty())
				forceId = expected.m_sForceId;
		}
		foreach (HST_ActiveGroupState candidate : state.m_aActiveGroups)
		{
			if (!candidate)
				continue;
			bool matches = !groupId.IsEmpty() && candidate.m_sGroupId == groupId;
			if (!matches && !order.m_sOrderId.IsEmpty())
				matches = candidate.m_sEnemyOrderId == order.m_sOrderId;
			if (!matches && !order.m_sOperationId.IsEmpty())
				matches = candidate.m_sOperationId == order.m_sOperationId;
			if (!matches && !order.m_sManifestId.IsEmpty())
				matches = candidate.m_sManifestId == order.m_sManifestId;
			if (!matches && !resultId.IsEmpty())
				matches = candidate.m_sSpawnResultId == resultId;
			if (!matches && !projectionId.IsEmpty())
				matches = candidate.m_sProjectionId == projectionId;
			if (!matches && !forceId.IsEmpty())
				matches = candidate.m_sForceId == forceId;
			if (matches)
				count++;
		}
		return count;
	}

	protected bool AbortAmbiguousAuthority(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		string reason)
	{
		if (!state || !order)
			return false;
		bool changed;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
		{
			order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
			changed = true;
		}
		if (order.m_bPhysicalized)
		{
			order.m_bPhysicalized = false;
			changed = true;
		}
		if (order.m_sRuntimeStatus != "exact_authority_ambiguous" || order.m_sFailureReason != reason)
		{
			order.m_sRuntimeStatus = "exact_authority_ambiguous";
			order.m_sFailureReason = reason;
			changed = true;
		}
		if (order.m_iResolvedAtSecond <= 0)
		{
			order.m_iResolvedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
			changed = true;
		}
		return changed;
	}

	protected int CountEnemyOrderId(HST_CampaignState state, string orderId)
	{
		int count;
		if (!state || orderId.IsEmpty())
			return count;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (order && order.m_sOrderId == orderId)
				count++;
		}
		return count;
	}

	protected int CountOperationId(HST_CampaignState state, string operationId)
	{
		int count;
		if (!state || operationId.IsEmpty())
			return count;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (operation && operation.m_sOperationId == operationId)
				count++;
		}
		return count;
	}

	protected int CountForceManifestId(HST_CampaignState state, string manifestId)
	{
		int count;
		if (!state || manifestId.IsEmpty())
			return count;
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
		{
			if (manifest && manifest.m_sManifestId == manifestId)
				count++;
		}
		return count;
	}

	protected int CountForceSpawnResultId(HST_CampaignState state, string resultId)
	{
		int count;
		if (!state || resultId.IsEmpty())
			return count;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && batch.m_sResultId == resultId)
				count++;
		}
		return count;
	}

	protected int CountActiveGroupId(HST_CampaignState state, string groupId)
	{
		int count;
		if (!state || groupId.IsEmpty())
			return count;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && group.m_sGroupId == groupId)
				count++;
		}
		return count;
	}

	protected string BuildSpawnResultId(HST_EnemyOrderState order)
	{
		if (!order)
			return "";
		return "spawn_" + order.m_sOrderId;
	}

	protected string BuildForceId(HST_EnemyOrderState order)
	{
		if (!order)
			return "";
		return "force_" + order.m_sOperationId;
	}

	protected string BuildProjectionId(HST_EnemyOrderState order)
	{
		if (!order)
			return "";
		return "projection_" + order.m_sOperationId;
	}

	protected float Distance2D(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}
}
