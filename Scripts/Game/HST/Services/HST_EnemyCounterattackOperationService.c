class HST_EnemyCounterattackAdmissionResult
{
	bool m_bSuccess;
	bool m_bStateChanged;
	string m_sFailureReason;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
}

class HST_EnemyCounterattackOperationService
{
	static const int EXACT_CONTRACT_VERSION = 1;
	static const int QUARANTINED_CONTRACT_VERSION = -69;
	static const string EXACT_FORCE_KIND = "enemy_counterattack";
	static const string EXACT_POLICY_ID = "exact_enemy_counterattack_v1";
	static const string EXACT_MANIFEST_INTENT = "enemy_counterattack";
	static const string EXACT_GROUP_MODE = "exact_enemy_counterattack";
	static const string ASSIGNMENT_KIND = "capture_zone";
	static const string RECALL_POLICY_ID = "return_to_origin_then_refund_survivors";
	static const string SETTLEMENT_POLICY_ID = "exact_enemy_counterattack_ledger";
	static const string OWNERSHIP_CAUSE = "military_capture";
	static const string OWNERSHIP_SOURCE_TYPE = "enemy_counterattack";
	static const int EXACT_COUNTERATTACK_PRIORITY = 80;
	static const int EXACT_COUNTERATTACK_MAX_RETRIES = 3;
	static const int EXACT_COUNTERATTACK_DEPLOYMENT_GRACE_SECONDS = 180;

	protected ref HST_OperationService m_Operations = new HST_OperationService();
	protected ref HST_StrategicMovementService m_StrategicMovement = new HST_StrategicMovementService();
	protected ref HST_MaterializationService m_Materialization = new HST_MaterializationService();
	protected ref HST_VirtualCombatService m_VirtualCombat = new HST_VirtualCombatService();
	protected ref HST_GarrisonService m_Garrisons = new HST_GarrisonService();
	protected ref HST_ForcePlanningIntegrityService m_Integrity = new HST_ForcePlanningIntegrityService();
	protected ref HST_ForceSpawnQueueService m_SpawnQueue;
	protected ref HST_ForceSpawnAdapterService m_SpawnAdapter;
	protected ref HST_PhysicalWarService m_PhysicalWar;
	protected ref HST_CombatPresenceService m_CombatPresence;
	protected ref HST_OwnershipTransitionService m_OwnershipTransitions;

	void SetRuntimeServices(
		HST_ForceSpawnQueueService spawnQueue,
		HST_ForceSpawnAdapterService spawnAdapter,
		HST_PhysicalWarService physicalWar,
		HST_CombatPresenceService combatPresence,
		HST_OwnershipTransitionService ownershipTransitions)
	{
		m_SpawnQueue = spawnQueue;
		m_SpawnAdapter = spawnAdapter;
		m_PhysicalWar = physicalWar;
		m_CombatPresence = combatPresence;
		m_OwnershipTransitions = ownershipTransitions;
	}

	bool IsExactEnemyCounterattack(HST_EnemyOrderState order)
	{
		return order && order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK
			&& order.m_iOperationContractVersion == EXACT_CONTRACT_VERSION;
	}

	bool HasOpenExactEnemyCounterattack(
		HST_CampaignState state,
		string factionKey,
		string targetZoneId)
	{
		if (!state || factionKey.IsEmpty() || targetZoneId.IsEmpty())
			return false;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsExactEnemyCounterattack(order) || order.m_sFactionKey != factionKey
				|| !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					order.m_sTargetZoneId, targetZoneId))
				continue;
			HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
			// An ABORTED admission rollback can still own an irrevocable prepared
			// refund. Keep the target occupied until that intent is consumed so a
			// second counterattack cannot debit against the same unfinished outcome.
			if (HasPreparedSettlementResumeCandidate(order, operation))
				return true;
			if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				continue;
			if (operation && (operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				|| operation.m_eSettlementState
					== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED))
				return true;
		}
		return false;
	}

	HST_EnemyCounterattackAdmissionResult CanAdmitPreparedOrder(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_EnemyDirectorService enemyDirector)
	{
		HST_EnemyCounterattackAdmissionResult result = new HST_EnemyCounterattackAdmissionResult();
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
			result.m_sFailureReason = "exact enemy counterattack durable admission identity already exists";
			return result;
		}

		HST_ForceSpawnQueueRequest request = new HST_ForceSpawnQueueRequest();
		request.m_sResultId = BuildSpawnResultId(order);
		request.m_sRequestId = order.m_sOrderId;
		request.m_sForceId = BuildForceId(order);
		request.m_sProjectionId = BuildProjectionId(order);
		request.m_iPriority = EXACT_COUNTERATTACK_PRIORITY;
		request.m_iMaxRetries = EXACT_COUNTERATTACK_MAX_RETRIES;
		request.m_iDeadlineSecond = state.m_iElapsedSeconds + EXACT_COUNTERATTACK_DEPLOYMENT_GRACE_SECONDS;
		HST_ForceSpawnQueueEnqueueResult queuePreflight = m_SpawnQueue.CanEnqueue(
			state.m_aForceSpawnResults,
			manifest,
			request,
			state.m_iElapsedSeconds);
		if (!queuePreflight || !queuePreflight.m_bSuccess)
		{
			result.m_sFailureReason = "exact enemy counterattack spawn admission preflight failed";
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

	protected HST_EnemyCounterattackAdmissionResult ResolveCommittedAdmissionReplay(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState suppliedManifest)
	{
		HST_EnemyCounterattackAdmissionResult result = new HST_EnemyCounterattackAdmissionResult();
		if (!state || !order || !suppliedManifest)
		{
			result.m_sFailureReason = "exact enemy counterattack committed replay context is incomplete";
			return result;
		}
		HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
		if (!manifest || !suppliedManifest.m_bFrozen
			|| suppliedManifest.m_sManifestId != manifest.m_sManifestId
			|| suppliedManifest.m_sOperationId != manifest.m_sOperationId
			|| suppliedManifest.m_sManifestHash != manifest.m_sManifestHash
			|| m_Integrity.BuildManifestHash(suppliedManifest) != suppliedManifest.m_sManifestHash)
		{
			result.m_sFailureReason = "exact enemy counterattack committed replay manifest conflicts";
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
			result.m_sFailureReason = "exact enemy counterattack committed replay authority conflicts";
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

	HST_EnemyCounterattackAdmissionResult AdmitPreparedOrder(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_EnemyDirectorService enemyDirector)
	{
		HST_EnemyCounterattackAdmissionResult result = new HST_EnemyCounterattackAdmissionResult();
		if (!state)
		{
			result.m_sFailureReason = "exact enemy counterattack admission state is unavailable";
			return result;
		}
		if (HasCommittedAdmissionAuthority(state, order))
			return ResolveCommittedAdmissionReplay(state, order, manifest);
		string debitFailure = HST_EnemyCounterattackSaveValidationService.ValidateOriginalResourceDebitAuthority(
			state.m_aEnemyStrategicMutations,
			order);
		if (!debitFailure.IsEmpty())
		{
			result.m_sFailureReason = debitFailure;
			result.m_bStateChanged = HoldInvalidResourceAuthority(
				state,
				order,
				debitFailure);
			return result;
		}
		HST_EnemyCounterattackAdmissionResult admissionPreflight = CanAdmitPreparedOrder(state, order, manifest, enemyDirector);
		if (!admissionPreflight || !admissionPreflight.m_bSuccess)
		{
			string failure = "exact enemy counterattack admission preflight failed";
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
				failure = "exact enemy counterattack manifest identity conflicts with existing authority";
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

		HST_OperationTransitionResult registered = m_Operations.RegisterExactEnemyCounterattack(state, order, manifest);
		if (!registered || !registered.m_bAccepted || !registered.m_Operation)
		{
			failure = "exact enemy counterattack operation registration failed";
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
				failure = "exact enemy counterattack active-group projection could not be built";
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
		spawnRequest.m_iPriority = EXACT_COUNTERATTACK_PRIORITY;
		spawnRequest.m_iMaxRetries = EXACT_COUNTERATTACK_MAX_RETRIES;
		HST_ForceSpawnResultState existingBatch = state.FindForceSpawnResult(resultId);
		if (!existingBatch)
			existingBatch = state.FindForceSpawnResultByRequest(order.m_sOrderId);
		if (existingBatch)
			spawnRequest.m_iDeadlineSecond = existingBatch.m_iDeadlineSecond;
		else
			spawnRequest.m_iDeadlineSecond = state.m_iElapsedSeconds + EXACT_COUNTERATTACK_DEPLOYMENT_GRACE_SECONDS;
		HST_ForceSpawnQueueEnqueueResult enqueue = m_SpawnQueue.Enqueue(
			state.m_aForceSpawnResults,
			manifest,
			spawnRequest,
			state.m_iElapsedSeconds);
		if (!enqueue || !enqueue.m_bSuccess || !enqueue.m_Batch)
		{
			failure = "exact enemy counterattack spawn admission failed";
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
			failure = "exact enemy counterattack strategic hold failed";
			if (held && !held.m_sFailureReason.IsEmpty())
				failure = failure + ": " + held.m_sFailureReason;
			result.m_sFailureReason = failure;
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
			result.m_bStateChanged = true;
			return result;
		}

		bool routeInitialized = m_StrategicMovement.InitializeExactInfantryDirectRoute(
			state,
			registered.m_Operation,
			manifest,
			group);
		HST_OperationTransitionResult linked;
		if (routeInitialized)
			linked = m_Operations.LinkExactEnemyCounterattackOutboundVirtual(state, order, group, enqueue.m_Batch);
		if (!routeInitialized || !linked || !linked.m_bAccepted)
		{
			failure = "exact enemy counterattack strategic projection link failed";
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
		if (!IsExactEnemyCounterattack(order))
			return false;
		if (!state || !enemyDirector)
			return SetRuntimeConflict(order, "exact enemy counterattack settlement services are unavailable");
		string ambiguousAuthority = FindAmbiguousAuthorityRows(state, order);
		if (!ambiguousAuthority.IsEmpty())
			return HoldAmbiguousAuthority(state, order, ambiguousAuthority);
		string debitFailure = HST_EnemyCounterattackSaveValidationService.ValidateOriginalResourceDebitAuthority(
			state.m_aEnemyStrategicMutations,
			order);
		if (!debitFailure.IsEmpty())
			return HoldInvalidResourceAuthority(state, order, debitFailure);
		HST_OperationRecordState terminalOperation = state.FindOperation(order.m_sOperationId);
		if (HasPreparedSettlementResumeCandidate(order, terminalOperation))
		{
			HST_ForceSpawnResultState preparedBatch
				= state.FindForceSpawnResult(order.m_sSpawnResultId);
			HST_ActiveGroupState preparedGroup = state.FindActiveGroup(order.m_sGroupId);
			if (IsUncommittedFullSettlementIntent(order))
			{
				if (!preparedBatch)
					preparedBatch = FindUniqueUncommittedBatchClaimant(state, order);
				if (!preparedGroup)
					preparedGroup = FindUniqueUncommittedGroupClaimant(state, order);
			}
			return ResumePreparedSettlement(
				state,
				enemyDirector,
				order,
				terminalOperation,
				state.FindForceManifest(order.m_sManifestId),
				preparedBatch,
				preparedGroup);
		}
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return false;
		if (!preset || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar
			|| !m_CombatPresence || !m_OwnershipTransitions)
			return SetRuntimeConflict(order, "exact enemy counterattack runtime services are unavailable");
		if (terminalOperation
			&& terminalOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			HST_ForceManifestState settledManifest = state.FindForceManifest(order.m_sManifestId);
			if (!HasValidMatchingSettledResourceAuthority(
				state,
				order,
				terminalOperation,
				settledManifest))
				return HoldInvalidResourceAuthority(
					state,
					order,
					"settled exact enemy counterattack resource refund authority is missing or conflicting");
			return FinalizeSettledOrder(
				state,
				order,
				terminalOperation,
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
			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL)
				return TickVirtualOnStation(state, preset, enemyDirector, order, operation, manifest, batch, group);
			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
				return TickMaterializing(state, enemyDirector, order, operation, manifest, batch, group);
			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
				return TickPhysicalOnStation(state, preset, enemyDirector, order, operation, manifest, batch, group);
			return FailClosedActiveOrder(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				"exact enemy counterattack on-station materialization authority is invalid");
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
			"exact enemy counterattack materialization authority is invalid");
	}

	bool ReconcileAfterRestore(HST_CampaignState state, HST_EnemyDirectorService enemyDirector)
	{
		if (!state || !enemyDirector)
			return false;
		bool changed;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsExactEnemyCounterattack(order))
				continue;
			string ambiguousAuthority = FindAmbiguousAuthorityRows(state, order);
			if (!ambiguousAuthority.IsEmpty())
			{
				changed = HoldAmbiguousAuthority(state, order, ambiguousAuthority) || changed;
				continue;
			}
			string debitFailure = HST_EnemyCounterattackSaveValidationService.ValidateOriginalResourceDebitAuthority(
				state.m_aEnemyStrategicMutations,
				order);
			if (!debitFailure.IsEmpty())
			{
				changed = HoldInvalidResourceAuthority(state, order, debitFailure) || changed;
				continue;
			}
			HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
			HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
			HST_ForceSpawnResultState batch = state.FindForceSpawnResult(order.m_sSpawnResultId);
			HST_ActiveGroupState group = state.FindActiveGroup(order.m_sGroupId);
			if (IsUncommittedFullSettlementIntent(order))
			{
				if (!batch)
					batch = FindUniqueUncommittedBatchClaimant(state, order);
				if (!group)
					group = FindUniqueUncommittedGroupClaimant(state, order);
			}
			// A prepared settlement is an irrevocable transaction intent. Consume it
			// before ownership checks, queued-order invalidation, or any ordinary
			// projection reconciliation so restore cannot invent a second outcome.
			if (HasPreparedSettlementResumeCandidate(order, operation))
			{
				changed = ResumePreparedSettlement(
					state,
					enemyDirector,
					order,
					operation,
					manifest,
					batch,
					group) || changed;
				continue;
			}
			if (operation
				&& (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN
					|| operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
					|| order.m_bOutcomeApplied))
			{
				string ownershipFailure = HST_EnemyCounterattackSaveValidationService.ValidateCompletedOwnershipAuthority(
					state.m_aZones,
					state.m_aOwnershipTransitions,
					order,
					operation);
				if (!ownershipFailure.IsEmpty())
				{
					changed = QuarantineRuntimeAuthority(order, ownershipFailure) || changed;
					continue;
				}
			}
			bool restoreCommitted = HasDurableRestoreCommitment(order, operation, batch, group);
			if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED)
			{
				order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
				order.m_sRuntimeStatus = "exact_restore_interrupted_admission";
				order.m_sFailureReason = "exact enemy counterattack restore found an interrupted queued admission";
				changed = true;
			}
			if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
			{
				bool resourcesSettled = ValidateAppliedResourceSettlement(state, order, manifest);
				if (HasPartialResourceSettlementAuthority(order))
				{
					order.m_sRuntimeStatus = "exact_restore_settlement_conflict";
					order.m_sFailureReason = "exact enemy counterattack restore contains a partial resource receipt";
					changed = true;
					continue;
				}
				if (operation
					&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
					&& !resourcesSettled)
				{
					order.m_sRuntimeStatus = "exact_restore_settlement_conflict";
					order.m_sFailureReason = "settled exact enemy counterattack lacks a matching durable resource receipt";
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
						"exact enemy counterattack restore invalidation");
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
					order.m_sFailureReason = "exact enemy counterattack restore settlement is waiting for complete refund and terminal authority";
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
					order.m_sFailureReason = "settled exact enemy counterattack restore authority conflicts with its resource settlement";
					changed = true;
				}
				continue;
			}
			if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE || !operation || !manifest || !batch || !group)
				continue;
			order.m_bPhysicalized = false;
			order.m_bAbstractResolved = false;
			order.m_sRuntimeStatus = "exact_counterattack_restore_virtual";
			group.m_sRuntimeStatus = "enemy_counterattack_virtual";
			group.m_bSpawnedEntity = false;
			group.m_sRuntimeEntityId = "";
			group.m_iSpawnedAgentCount = 0;
			changed = true;
		}
		return changed;
	}

	bool HasPreparedSettlementResumeCandidate(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		if (!IsExactEnemyCounterattack(order) || order.m_sResourceSettlementKind.IsEmpty()
			|| order.m_sResourceSettlementId.IsEmpty())
			return false;
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
			return true;
		// SETTLED without the order's durable resource receipt is an impossible
		// transaction inversion. Keep it on the prepared-resume path so the exact
		// guard holds both runtime authority and target admission fail-closed.
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& !order.m_bResourceSettlementApplied)
			return true;

		if (!IsUncommittedFullSettlementIntent(order))
			return false;
		if (!order.m_bResourceSettlementApplied)
			return !operation || operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		if (!operation || operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return false;
		return order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			|| order.m_sRuntimeStatus != "resolved_exact_terminal";
	}

	protected bool IsUncommittedFullSettlementIntent(HST_EnemyOrderState order)
	{
		if (!order || order.m_bStrategicServiceCommitted
			|| order.m_sResourceSettlementKind.IsEmpty())
			return false;
		HST_EOperationTerminalResult terminalResult;
		bool fullRefund;
		bool requiresRecapture;
		return HST_EnemyCounterattackSaveValidationService.ResolveSettlementPolicy(
			order.m_sResourceSettlementKind,
			terminalResult,
			fullRefund,
			requiresRecapture) && fullRefund;
	}

	protected HST_ForceSpawnResultState FindUniqueUncommittedBatchClaimant(
		HST_CampaignState state,
		HST_EnemyOrderState order)
	{
		HST_ForceSpawnResultState match;
		if (!state || !order)
			return null;
		string resultId;
		if (!order.m_sOrderId.IsEmpty())
			resultId = BuildSpawnResultId(order);
		string projectionId;
		string forceId;
		if (!order.m_sOperationId.IsEmpty())
		{
			projectionId = BuildProjectionId(order);
			forceId = BuildForceId(order);
		}
		foreach (HST_ForceSpawnResultState candidate : state.m_aForceSpawnResults)
		{
			if (!candidate)
				continue;
			bool claimant = candidate.m_sRequestId == order.m_sOrderId
				|| candidate.m_sOperationId == order.m_sOperationId
				|| candidate.m_sManifestId == order.m_sManifestId;
			if (!claimant && !resultId.IsEmpty())
				claimant = candidate.m_sResultId == resultId;
			if (!claimant && !projectionId.IsEmpty())
				claimant = candidate.m_sProjectionId == projectionId;
			if (!claimant && !forceId.IsEmpty())
				claimant = candidate.m_sForceId == forceId;
			if (!claimant)
				continue;
			if (match)
				return null;
			match = candidate;
		}
		return match;
	}

	protected HST_ActiveGroupState FindUniqueUncommittedGroupClaimant(
		HST_CampaignState state,
		HST_EnemyOrderState order)
	{
		HST_ActiveGroupState match;
		if (!state || !order)
			return null;
		string resultId;
		if (!order.m_sOrderId.IsEmpty())
			resultId = BuildSpawnResultId(order);
		string projectionId;
		string forceId;
		if (!order.m_sOperationId.IsEmpty())
		{
			projectionId = BuildProjectionId(order);
			forceId = BuildForceId(order);
		}
		foreach (HST_ActiveGroupState candidate : state.m_aActiveGroups)
		{
			if (!candidate)
				continue;
			bool claimant = candidate.m_sEnemyOrderId == order.m_sOrderId
				|| candidate.m_sOperationId == order.m_sOperationId
				|| candidate.m_sManifestId == order.m_sManifestId;
			if (!claimant && !projectionId.IsEmpty())
				claimant = candidate.m_sGroupId == projectionId
					|| candidate.m_sProjectionId == projectionId;
			if (!claimant && !resultId.IsEmpty())
				claimant = candidate.m_sSpawnResultId == resultId;
			if (!claimant && !forceId.IsEmpty())
				claimant = candidate.m_sForceId == forceId;
			if (!claimant)
				continue;
			if (match)
				return null;
			match = candidate;
		}
		return match;
	}

	protected bool ResumePreparedSettlement(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!state || !enemyDirector || !order)
			return false;
		string tupleFailure
			= HST_EnemyCounterattackSaveValidationService.ValidatePreparedResourceSettlementTuple(order);
		if (!tupleFailure.IsEmpty())
			return SetRuntimeConflict(
				order,
				"exact enemy counterattack prepared settlement tuple conflicts on restore: " + tupleFailure);

		HST_EOperationTerminalResult terminalResult;
		bool fullRefund;
		bool requiresRecapture;
		if (!HST_EnemyCounterattackSaveValidationService.ResolveSettlementPolicy(
			order.m_sResourceSettlementKind,
			terminalResult,
			fullRefund,
			requiresRecapture))
			return SetRuntimeConflict(order, "exact enemy counterattack prepared settlement policy is unsupported");
		if (!operation && (!fullRefund || order.m_bStrategicServiceCommitted))
			return SetRuntimeConflict(order, "exact enemy counterattack prepared settlement lacks terminal operation authority");
		if (requiresRecapture)
		{
			string ownershipFailure
				= HST_EnemyCounterattackSaveValidationService.ValidateCompletedOwnershipAuthority(
					state.m_aZones,
					state.m_aOwnershipTransitions,
					order,
					operation);
			if (!ownershipFailure.IsEmpty())
				return SetRuntimeConflict(
					order,
					"exact enemy counterattack prepared ownership authority conflicts on restore: "
						+ ownershipFailure);
		}
		// Production finalization cannot reach SETTLED before the order carries the
		// exact resource receipt. Preserve and hold any forged/stale inversion
		// instead of allowing the uncommitted-full retry path to create a refund.
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& !order.m_bResourceSettlementApplied)
		{
			return SetRuntimeConflict(
				order,
				"settled exact enemy counterattack precedes its durable resource receipt");
		}
		if (!operation || operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			string pendingFailure
				= HST_EnemyCounterattackSaveValidationService.ValidatePendingResourceRefundAggregateAuthority(
					state.m_aEnemyStrategicMutations,
					order,
					operation,
					manifest,
					batch,
					group);
			if (!pendingFailure.IsEmpty())
				return SetRuntimeConflict(
					order,
					"exact enemy counterattack prepared aggregate conflicts on restore: " + pendingFailure);
		}

		string reason = "exact enemy counterattack resumed prepared "
			+ order.m_sResourceSettlementKind + " settlement";
		if (operation)
		{
			if (operation.m_eTerminalResult != terminalResult
				|| operation.m_sSettlementId != order.m_sResourceSettlementId)
				return SetRuntimeConflict(order, "exact enemy counterattack prepared operation terminal intent conflicts on restore");
			if (!operation.m_sTerminalReason.IsEmpty())
				reason = operation.m_sTerminalReason;
			if (operation.m_eSettlementState
				!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
				&& operation.m_eSettlementState
					!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				return SetRuntimeConflict(order, "exact enemy counterattack prepared operation lifecycle conflicts on restore");
		}

		if (!order.m_bResourceSettlementApplied)
		{
			if (!ApplyResourceSettlement(
				state,
				enemyDirector,
				order,
				manifest,
				order.m_sResourceSettlementKind,
				terminalResult,
				order.m_iSettlementSurvivorMemberCount,
				fullRefund,
				reason))
				return SetRuntimeConflict(order, "exact enemy counterattack prepared resource settlement could not resume");
		}
		string refundFailure
			= HST_EnemyCounterattackSaveValidationService.ValidateSettledResourceRefundAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		if (!refundFailure.IsEmpty())
			return SetRuntimeConflict(
				order,
				"exact enemy counterattack prepared refund authority conflicts on restore: " + refundFailure);

		if (!operation)
			return FinalizeUncommittedFullSettlement(state, order, batch, group, reason);
		if (operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			HST_OperationTransitionResult settled = m_Operations.SettleExactEnemyCounterattack(
				state,
				order,
				terminalResult,
				order.m_sResourceSettlementId,
				reason);
			if (!settled || !settled.m_bAccepted || !settled.m_Operation)
				return SetRuntimeConflict(order, "exact enemy counterattack prepared terminal operation could not finalize");
			operation = settled.m_Operation;
		}
		if (operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return SetRuntimeConflict(order, "exact enemy counterattack prepared terminal operation remains incomplete");
		return FinalizeSettledOrder(state, order, operation, batch, group);
	}

	protected bool FinalizeUncommittedFullSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		if (!state || !order || order.m_bStrategicServiceCommitted
			|| !order.m_bResourceSettlementApplied)
			return false;
		bool changed;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
		{
			order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
			changed = true;
		}
		if (order.m_iResolvedAtSecond <= 0)
		{
			order.m_iResolvedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
			changed = true;
		}
		if (order.m_bPhysicalized || order.m_bAbstractResolved)
		{
			order.m_bPhysicalized = false;
			order.m_bAbstractResolved = false;
			changed = true;
		}
		if (order.m_sRuntimeStatus != "resolved_exact_terminal")
		{
			order.m_sRuntimeStatus = "resolved_exact_terminal";
			changed = true;
		}
		if (order.m_sResolutionKind.IsEmpty())
		{
			order.m_sResolutionKind = order.m_sResourceSettlementKind;
			changed = true;
		}
		if (order.m_sFailureReason.IsEmpty())
		{
			order.m_sFailureReason = reason;
			changed = true;
		}

		bool runtimeReleased = true;
		if (batch && (batch.m_iSuccessfulHandoffCount > 0
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| (m_SpawnAdapter && m_SpawnAdapter.CountHandlesForProjection(batch.m_sProjectionId) > 0)))
			runtimeReleased = false;
		if (group && (group.m_bSpawnedEntity || group.m_iSpawnedAgentCount > 0
			|| (m_PhysicalWar && (m_PhysicalWar.GetForceSpawnGroupRoot(group)
				|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0))))
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

	bool ReconcileSettledRuntimeCleanup(HST_CampaignState state)
	{
		if (!state)
			return false;
		bool changed;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsExactEnemyCounterattack(order))
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

	// Persistence reuses the production ambiguity, resource, reciprocal-link,
	// and transition validators rather than certifying a weaker parallel graph.
	string ValidateOpenPersistenceRuntimeAuthority(
		HST_CampaignState state,
		HST_OperationRecordState expectedOperation,
		out HST_EnemyOrderState order,
		out HST_ForceSpawnResultState batch,
		out HST_ActiveGroupState group)
	{
		order = null;
		batch = null;
		group = null;
		if (!state || !expectedOperation
			|| expectedOperation.m_eType
				!= HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK
			|| expectedOperation.m_iContractVersion
				!= HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION
			|| expectedOperation.m_eSettlementState
				!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| expectedOperation.m_eTerminalResult
				!= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
		{
			return "exact enemy counterattack persistence operation authority is unavailable";
		}

		order = state.FindEnemyOrder(expectedOperation.m_sEnemyOrderId);
		if (!IsExactEnemyCounterattack(order))
			return "exact enemy counterattack persistence order authority is unavailable";

		HST_OperationRecordState resolvedOperation;
		HST_ForceManifestState manifest;
		string failure = ResolveRuntimeContext(
			state,
			order,
			resolvedOperation,
			manifest,
			batch,
			group);
		if (!failure.IsEmpty())
			return failure;
		if (resolvedOperation != expectedOperation)
			return "exact enemy counterattack persistence operation claimant conflicts";
		return "";
	}

	string RefreshPhysicalPersistencePosition(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group)
	{
		HST_OperationTransitionResult refreshed
			= m_Operations.UpdateExactEnemyCounterattackPhysicalPosition(state, order, group);
		if (refreshed && refreshed.m_bAccepted)
			return "";
		if (refreshed && !refreshed.m_sFailureReason.IsEmpty())
			return refreshed.m_sFailureReason;
		return "exact enemy counterattack physical position refresh was rejected";
	}

#ifdef ENABLE_DIAG
	// Campaign Debug read-only hooks reuse the same production validators so
	// telemetry cannot certify a weaker exact-counterattack authority model.
	string DebugValidateOpenRuntimeAuthority(
		HST_CampaignState state,
		HST_EnemyOrderState order)
	{
		if (!state || !IsExactEnemyCounterattack(order))
			return "exact enemy counterattack debug authority is unavailable";
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return "exact enemy counterattack debug authority is not active";
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		return ResolveRuntimeContext(
			state,
			order,
			operation,
			manifest,
			batch,
			group);
	}

	string DebugValidateTerminalLedgerAuthority(
		HST_CampaignState state,
		HST_EnemyOrderState order)
	{
		if (!state || !IsExactEnemyCounterattack(order))
			return "exact enemy counterattack terminal debug authority is unavailable";
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED
			&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
			return "exact enemy counterattack terminal debug order is not terminal";
		string ambiguity = FindAmbiguousAuthorityRows(state, order);
		if (!ambiguity.IsEmpty())
			return ambiguity;
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
		if (!operation || operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return "exact enemy counterattack terminal operation is not settled";
		if (!m_Operations.ValidateExactEnemyCounterattack(
			state,
			operation,
			order,
			manifest).IsEmpty())
			return "exact enemy counterattack terminal operation or manifest identity is invalid";
		HST_EEnemyOrderStatus expectedStatus
			= HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		if (operation.m_eTerminalResult
			== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
			|| operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED)
			expectedStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED;
		if (order.m_eStatus != expectedStatus
			|| order.m_iResolvedAtSecond != Math.Max(0, operation.m_iSettledAtSecond)
			|| order.m_bPhysicalized || order.m_bAbstractResolved)
			return "exact enemy counterattack terminal order lifecycle does not match settlement";
		if (!HasValidMatchingSettledResourceAuthority(
			state,
			order,
			operation,
			manifest))
			return "exact enemy counterattack terminal resource or reciprocal ledger is invalid";
		if (!HasReleasedAdministrativeRuntimeAuthority(state, order, operation))
			return "exact enemy counterattack terminal ledger retains a runtime claimant";
		return "";
	}
#endif

	bool SettleTrackedOpenOrderForAdministrativeStop(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		string reason)
	{
		if (!state || !enemyDirector || !IsExactEnemyCounterattack(order)
			|| !m_SpawnAdapter || !m_PhysicalWar)
			return false;
		if (reason.IsEmpty())
			reason = "administrative stop cancelled exact enemy counterattack";
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			ReconcileSettledRuntimeCleanup(state);
			return HST_EnemyCounterattackSaveValidationService
				.ValidateSettledResourceRefundAuthority(
					state.m_aEnemyStrategicMutations,
					order).IsEmpty()
				&& HasReleasedAdministrativeRuntimeAuthority(state, order, operation);
		}

		HST_ForceManifestState manifest;
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		string failure = ResolveRuntimeContext(
			state,
			order,
			operation,
			manifest,
			batch,
			group);
		if (!failure.IsEmpty())
			return false;
		FailClosedActiveOrder(
			state,
			enemyDirector,
			order,
			operation,
			manifest,
			batch,
			group,
			reason);
		ReconcileSettledRuntimeCleanup(state);
		operation = state.FindOperation(order.m_sOperationId);
		return operation
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& HST_EnemyCounterattackSaveValidationService
				.ValidateSettledResourceRefundAuthority(
					state.m_aEnemyStrategicMutations,
					order).IsEmpty()
			&& HasReleasedAdministrativeRuntimeAuthority(state, order, operation);
	}

	protected bool HasReleasedAdministrativeRuntimeAuthority(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		if (!state || !order || !operation || !m_SpawnAdapter || !m_PhysicalWar
			|| order.m_sOrderId.IsEmpty() || order.m_sOperationId.IsEmpty())
			return false;
		string resultId = BuildSpawnResultId(order);
		string projectionId = BuildProjectionId(order);
		if ((!order.m_sSpawnResultId.IsEmpty() && order.m_sSpawnResultId != resultId)
			|| (!order.m_sGroupId.IsEmpty() && order.m_sGroupId != projectionId)
			|| (!operation.m_sSpawnResultId.IsEmpty() && operation.m_sSpawnResultId != resultId)
			|| (!operation.m_sProjectionId.IsEmpty() && operation.m_sProjectionId != projectionId)
			|| (!operation.m_sGroupId.IsEmpty() && operation.m_sGroupId != projectionId))
			return false;

		HST_ForceSpawnResultState batchProbe = new HST_ForceSpawnResultState();
		batchProbe.m_sResultId = resultId;
		batchProbe.m_sProjectionId = projectionId;
		HST_ActiveGroupState groupProbe = new HST_ActiveGroupState();
		groupProbe.m_sGroupId = projectionId;
		groupProbe.m_sSpawnResultId = resultId;
		groupProbe.m_sProjectionId = projectionId;
		if (CountForceSpawnResultsByAnyAuthorityIdentity(state, order, operation, batchProbe) > 0
			|| CountActiveGroupsByAnyAuthorityIdentity(state, order, operation, groupProbe) > 0
			|| m_SpawnAdapter.CountHandlesForProjection(projectionId) > 0
			|| m_SpawnAdapter.CountHandlesForResultId(resultId) > 0
			|| m_PhysicalWar.GetForceSpawnGroupRoot(groupProbe)
			|| m_PhysicalWar.CountForceSpawnRuntimeMembers(groupProbe) > 0)
			return false;
		return true;
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
			bool exactContract = candidate.m_eType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK
				&& candidate.m_iContractVersion == EXACT_CONTRACT_VERSION;
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
			order.m_sFailureReason = "exact enemy counterattack restore operation identity is missing, ambiguous, or conflicting";
			return false;
		}

		string settlementId = HST_OperationService.BuildSettlementId(order.m_sOperationId, settlementKind);
		if (canonical.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (canonical.m_sSettlementId == settlementId)
				return false;
			order.m_sRuntimeStatus = "exact_restore_settlement_conflict";
			order.m_sFailureReason = "settled exact enemy counterattack has a conflicting terminal receipt";
			return false;
		}
		HST_OperationTransitionResult terminal = m_Operations.SettleExactEnemyCounterattack(
			state,
			order,
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
			settlementId,
			"exact enemy counterattack restore invalidation");
		if (!terminal || !terminal.m_bAccepted)
		{
			// PREPARED is an irrevocable transaction receipt. The legacy repair is
			// limited to an untouched OPEN operation and must never overwrite a
			// prepared terminal result, reason, or timestamp.
			if (canonical.m_eSettlementState
				!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				|| !ForceSettleInvalidatedRestore(
					canonical,
					order,
					settlementId,
					"exact enemy counterattack restore invalidation",
					state.m_iElapsedSeconds))
			{
				order.m_sRuntimeStatus = "exact_restore_authority_conflict";
				order.m_sFailureReason = "exact enemy counterattack restore operation ownership conflicts";
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
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
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
			reason = "campaign phase no longer permits an active enemy counterattack";
		bool changed;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsExactEnemyCounterattack(order))
				continue;
			HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
			if (HasPreparedSettlementResumeCandidate(order, operation))
			{
				string ambiguousAuthority = FindAmbiguousAuthorityRows(state, order);
				if (!ambiguousAuthority.IsEmpty())
				{
					changed = HoldAmbiguousAuthority(state, order, ambiguousAuthority) || changed;
					continue;
				}
				string debitFailure
					= HST_EnemyCounterattackSaveValidationService.ValidateOriginalResourceDebitAuthority(
						state.m_aEnemyStrategicMutations,
						order);
				if (!debitFailure.IsEmpty())
				{
					changed = HoldInvalidResourceAuthority(state, order, debitFailure) || changed;
					continue;
				}
				HST_ForceSpawnResultState preparedBatch
					= state.FindForceSpawnResult(order.m_sSpawnResultId);
				HST_ActiveGroupState preparedGroup = state.FindActiveGroup(order.m_sGroupId);
				if (IsUncommittedFullSettlementIntent(order))
				{
					if (!preparedBatch)
						preparedBatch = FindUniqueUncommittedBatchClaimant(state, order);
					if (!preparedGroup)
						preparedGroup = FindUniqueUncommittedGroupClaimant(state, order);
				}
				changed = ResumePreparedSettlement(
					state,
					enemyDirector,
					order,
					operation,
					state.FindForceManifest(order.m_sManifestId),
					preparedBatch,
					preparedGroup) || changed;
				continue;
			}
			if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
				&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED)
				continue;
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
		return ApplyResourceSettlement(
			state,
			enemyDirector,
			order,
			manifest,
			settlementKind,
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
			survivorCount,
			fullRefund,
			reason);
	}

	protected bool RefundOriginallyChargedResources(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		int attackRefund,
		int supportRefund,
		string reason,
		string mutationId,
		string settlementId)
	{
		if (!state || !enemyDirector || !order || mutationId.IsEmpty() || settlementId.IsEmpty()
			|| attackRefund < 0 || supportRefund < 0)
			return false;
		if (!HST_EnemyCounterattackSaveValidationService.ValidateOriginalResourceDebitAuthority(
			state.m_aEnemyStrategicMutations,
			order).IsEmpty())
			return false;
		bool attackFunded = order.m_iAttackCost > 0 && order.m_iSupportCost == 0;
		bool supportFunded = order.m_iSupportCost > 0 && order.m_iAttackCost == 0;
		if (attackFunded)
		{
			if (supportRefund != 0)
				return false;
			return enemyDirector.RefundProactiveAttackResources(
				state,
				order.m_sFactionKey,
				attackRefund,
				reason,
				mutationId,
				settlementId,
				order.m_sOrderId,
				order.m_sOperationId,
				order.m_sManifestId,
				order.m_sTargetZoneId);
		}
		if (!supportFunded || attackRefund != 0)
			return false;
		return enemyDirector.RefundDefenseResources(
			state,
			order.m_sFactionKey,
			order.m_sTargetZoneId,
			0,
			supportRefund,
			reason,
			mutationId,
			settlementId,
			order.m_sOrderId,
			order.m_sOperationId,
			order.m_sManifestId);
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
		if (operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK
			&& operation.m_iContractVersion == EXACT_CONTRACT_VERSION
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
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!state || !order || !order.m_bResourceSettlementApplied
			|| order.m_sResourceSettlementKind.IsEmpty())
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
		if (HST_EnemyCounterattackSaveValidationService.IsFullRefundSettlementKind(
			order.m_sResourceSettlementKind))
		{
			expectedAttackRefund = Math.Max(0, order.m_iAttackCost);
			expectedSupportRefund = Math.Max(0, order.m_iSupportCost);
		}
		if (order.m_iRefundedAttackResources != expectedAttackRefund
			|| order.m_iRefundedSupportResources != expectedSupportRefund)
			return false;
		return HST_EnemyCounterattackSaveValidationService.ValidateSettledResourceRefundAuthority(
			state.m_aEnemyStrategicMutations,
			order).IsEmpty();
	}

	protected bool HasPartialResourceSettlementAuthority(HST_EnemyOrderState order)
	{
		if (!order || order.m_bResourceSettlementApplied)
			return false;
		// A matching refund mutation ID alone is the retry token for the narrow
		// post-refund/pre-record window. ApplyResourceSettlement validates it
		// against the exact settlement ID before replay; all other partial fields
		// remain conflicts.
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
		if (!HST_EnemyCounterattackSaveValidationService.ValidateOriginalResourceDebitAuthority(
			state.m_aEnemyStrategicMutations,
			order).IsEmpty()
			|| !HST_EnemyCounterattackSaveValidationService.ValidateSettledResourceRefundAuthority(
				state.m_aEnemyStrategicMutations,
				order).IsEmpty())
			return false;
		string expectedId = HST_OperationService.BuildSettlementId(order.m_sOperationId, order.m_sResourceSettlementKind);
		if (order.m_sResourceSettlementId != expectedId || operation.m_sSettlementId != expectedId)
			return false;
		if (!HST_EnemyCounterattackSaveValidationService.ValidateSettlementPolicy(
			order,
			operation).IsEmpty())
			return false;
		if (!HST_EnemyCounterattackSaveValidationService.ValidateCompletedOwnershipAuthority(
			state.m_aZones,
			state.m_aOwnershipTransitions,
			order,
			operation).IsEmpty())
			return false;
		return ValidateSettledAuthority(order, operation, manifest).IsEmpty();
	}

	protected bool TickVirtualOnStation(
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
				"virtual_combat_projection_failed_survivors",
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy counterattack held combat projection failed");
		}
		if (!batch.m_bStrategicProjectionHeld)
			return SetRuntimeConflict(order, "exact enemy counterattack virtual combat projection is not strategically held");

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
				"exact enemy counterattack virtual combat roster eliminated");
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
				"exact enemy counterattack was eliminated in virtual combat");
		}

		HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
		string relationFailure = ValidateCurrentCounterattackTarget(preset, order, targetZone);
		if (!relationFailure.IsEmpty())
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
				"target_invalidated_survivors",
				living,
				relationFailure);
		}
		if (targetZone.m_sOwnerFactionKey == order.m_sFactionKey)
		{
			bool resumedCompleted;
			bool resumedRetryable;
			string resumedFailure;
			TryApplyCounterattackOwnership(
				state,
				order,
				resumedCompleted,
				resumedRetryable,
				resumedFailure);
			if (resumedCompleted)
				return BeginReturnAfterVictory(state, enemyDirector, order, operation, manifest, batch, group, true, resumedFailure);
			if (resumedRetryable)
			{
				SetRuntimeEvidence(
					order,
					group,
					"exact_virtual_ownership_retry",
					"enemy_counterattack_virtual_ownership_retry",
					resumedFailure);
				return true;
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
				"ownership_failed_survivors",
				living,
				resumedFailure);
		}

		HST_OperationProjectionDecision decision = m_Materialization.EvaluateExactEnemyCounterattack(
			operation,
			operation.m_vStrategicPosition);
		operation.m_sLastProjectionReason = decision.m_sReason;
		if (decision.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE)
			return BeginMaterialization(state, enemyDirector, order, operation, manifest, batch, group, decision.m_sReason);

		HST_VirtualCombatResult combat = m_VirtualCombat.TickExactEnemyCounterattack(
			state,
			operation,
			order,
			manifest,
			batch,
			group,
			m_SpawnQueue);
		if (!combat || !combat.m_bAccepted)
		{
			string failure = "exact enemy counterattack virtual combat authority was rejected";
			if (combat && !combat.m_sFailureReason.IsEmpty())
				failure = failure + ": " + combat.m_sFailureReason;
			return SetRuntimeConflict(order, failure);
		}

		living = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
		SyncGroupRoster(group, living);
		operation.m_iLastVirtualFriendlyCount = living;
		if (combat.m_bFriendlyEliminated || living <= 0)
		{
			m_SpawnQueue.CompleteStrategicProjectionElimination(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				state.m_iElapsedSeconds,
				"exact enemy counterattack virtual combat roster eliminated");
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
				"exact enemy counterattack was eliminated in virtual combat") || combat.m_bStateChanged;
		}

		HST_GarrisonVirtualCombatRosterResult defenderRoster = m_Garrisons.ResolveExactVirtualCombatRoster(
			state,
			targetZone.m_sZoneId,
			targetZone.m_sOwnerFactionKey);
		if (!defenderRoster || !defenderRoster.m_bAccepted)
		{
			string rosterFailure = "exact enemy counterattack defender roster failed closed after virtual combat";
			if (defenderRoster && !defenderRoster.m_sFailureReason.IsEmpty())
				rosterFailure = rosterFailure + ": " + defenderRoster.m_sFailureReason;
			return SetRuntimeConflict(order, rosterFailure) || combat.m_bStateChanged;
		}
		if (defenderRoster.m_iTotalDefenderCount > 0)
		{
			bool evidenceChanged = SetRuntimeEvidence(
				order,
				group,
				"exact_virtual_engaged",
				"enemy_counterattack_virtual_engaged",
				combat.m_sEvidence);
			return evidenceChanged || combat.m_bStateChanged;
		}

		bool completed;
		bool retryable;
		string ownershipFailure;
		TryApplyCounterattackOwnership(state, order, completed, retryable, ownershipFailure);
		if (completed)
			return BeginReturnAfterVictory(state, enemyDirector, order, operation, manifest, batch, group, true, ownershipFailure) || combat.m_bStateChanged;
		if (retryable)
		{
			SetRuntimeEvidence(
				order,
				group,
				"exact_virtual_ownership_retry",
				"enemy_counterattack_virtual_ownership_retry",
				ownershipFailure);
			return true;
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
			"ownership_failed_survivors",
			living,
			ownershipFailure) || combat.m_bStateChanged;
	}

	protected bool TickPhysicalOnStation(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| !group.m_bSpawnedEntity || group.m_sRuntimeStatus == "spawn_failed"
			|| group.m_sRuntimeStatus.Contains("runtime_binding_missing"))
		{
			return SettlePhysicalFailure(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				0,
				"exact enemy counterattack physical combat binding is unavailable");
		}

		HST_OperationTransitionResult live = m_Operations.UpdateExactEnemyCounterattackPhysicalPosition(state, order, group);
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
				"exact enemy counterattack live-position handoff failed during combat");
		}
		bool changed = live.m_bStateChanged;
		int living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
		SyncGroupRoster(group, living);
		operation.m_iLastVirtualFriendlyCount = living;
		if (living <= 0 || group.m_sRuntimeStatus == "eliminated")
		{
			string eliminationReason;
			if (!m_PhysicalWar.FinalizeEliminatedForceSpawnProjection(
				state,
				group,
				state.m_iElapsedSeconds,
				eliminationReason))
			{
				return SetRuntimeConflict(order, "exact enemy counterattack elimination cleanup is pending: " + eliminationReason) || changed;
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
				"exact enemy counterattack was eliminated in physical combat") || changed;
		}

		HST_OperationProjectionDecision decision = m_Materialization.EvaluateExactEnemyCounterattack(
			operation,
			group.m_vPosition);
		operation.m_sLastProjectionReason = decision.m_sReason;
		if (decision.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_DEMATERIALIZE)
			return TryDematerialize(state, enemyDirector, order, operation, manifest, batch, group, decision.m_sReason) || changed;

		HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
		string relationFailure = ValidateCurrentCounterattackTarget(preset, order, targetZone);
		if (!relationFailure.IsEmpty())
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
				"target_invalidated_survivors",
				living,
				relationFailure) || changed;
		}
		if (targetZone.m_sOwnerFactionKey == order.m_sFactionKey)
		{
			bool resumedCompleted;
			bool resumedRetryable;
			string resumedFailure;
			TryApplyCounterattackOwnership(
				state,
				order,
				resumedCompleted,
				resumedRetryable,
				resumedFailure);
			if (resumedCompleted)
				return BeginReturnAfterVictory(state, enemyDirector, order, operation, manifest, batch, group, false, resumedFailure) || changed;
			if (resumedRetryable)
			{
				SetRuntimeEvidence(
					order,
					group,
					"exact_physical_ownership_retry",
					"enemy_counterattack_physical_ownership_retry",
					resumedFailure);
				return true;
			}
			return RetireAndSettlePhysicalFailure(
				state,
				enemyDirector,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"ownership_failed_survivors",
				living,
				resumedFailure) || changed;
		}

		HST_CombatPresenceResult presence = m_CombatPresence.QueryZoneHostilePresence(
			state,
			preset,
			order.m_sFactionKey,
			targetZone,
			true);
		if (!presence || !presence.m_bQueryValid)
		{
			string evidence = "exact enemy counterattack defender combat-presence authority is unresolved";
			if (presence && !presence.m_sReason.IsEmpty())
				evidence = evidence + ": " + presence.m_sReason;
			SetRuntimeEvidence(
				order,
				group,
				"exact_physical_authority_wait",
				"enemy_counterattack_physical_authority_wait",
				evidence);
			return true;
		}

		HST_GarrisonVirtualCombatRosterResult defenderRoster = m_Garrisons.ResolveExactVirtualCombatRoster(
			state,
			targetZone.m_sZoneId,
			targetZone.m_sOwnerFactionKey);
		if (!defenderRoster || !defenderRoster.m_bAccepted)
		{
			string rosterFailure = "exact enemy counterattack defender roster failed closed during physical combat";
			if (defenderRoster && !defenderRoster.m_sFailureReason.IsEmpty())
				rosterFailure = rosterFailure + ": " + defenderRoster.m_sFailureReason;
			return SetRuntimeConflict(order, rosterFailure) || changed;
		}
		int abstractStrength = defenderRoster.m_iTotalDefenderCount;
		int exactStrength = ResolveCombatPresenceStrength(presence);
		if (presence.m_bBlocksProgress || presence.m_bHasLiveContributors
			|| abstractStrength > 0 || exactStrength > 0)
		{
			changed = SetEngagementMode(
				state,
				order,
				HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED) || changed;
			string evidence = string.Format(
				"defenders exact %1 abstract %2 blocks %3 | %4",
				exactStrength,
				abstractStrength,
				presence.m_bBlocksProgress,
				presence.m_sReason);
			SetRuntimeEvidence(
				order,
				group,
				"exact_physical_engaged",
				"enemy_counterattack_physical_engaged",
				evidence);
			return true;
		}

		changed = SetEngagementMode(
			state,
			order,
			HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR) || changed;
		bool completed;
		bool retryable;
		string ownershipFailure;
		TryApplyCounterattackOwnership(state, order, completed, retryable, ownershipFailure);
		if (completed)
			return BeginReturnAfterVictory(state, enemyDirector, order, operation, manifest, batch, group, false, ownershipFailure) || changed;
		if (retryable)
		{
			SetRuntimeEvidence(
				order,
				group,
				"exact_physical_ownership_retry",
				"enemy_counterattack_physical_ownership_retry",
				ownershipFailure);
			return true;
		}
		return RetireAndSettlePhysicalFailure(
			state,
			enemyDirector,
			order,
			operation,
			manifest,
			batch,
			group,
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
			"ownership_failed_survivors",
			living,
			ownershipFailure) || changed;
	}

	protected bool BeginReturnAfterVictory(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		bool abstractVictory,
		string reason)
	{
		if (!state || !order || !operation || !manifest || !batch || !group)
			return false;
		order.m_bOutcomeApplied = true;
		order.m_bAbstractResolved = abstractVictory;
		order.m_sResolutionKind = "exact_counterattack_recaptured";
		order.m_sFailureReason = "";
		SetEngagementMode(state, order, HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR);
		HST_OperationTransitionResult returning = m_Operations.BeginExactEnemyCounterattackReturnToOrigin(
			state,
			order,
			group);
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
					"return_transition_failed_survivors",
					ResolveSettlementSurvivors(operation, manifest, batch, group),
					"exact enemy counterattack return transition failed after victory");
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
				"return_transition_failed_survivors",
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy counterattack return transition failed after victory");
		}

		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
		{
			if (!m_PhysicalWar.RestartExactEnemyCounterattackInfantryRoute(
				state,
				group,
				operation.m_vOriginPosition,
				"Exact enemy counterattack returning after canonical recapture."))
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
					"exact enemy counterattack return route could not be issued after victory");
			}
			order.m_sRuntimeStatus = "exact_physical_returning";
		}
		else
		{
			order.m_sRuntimeStatus = "exact_virtual_returning";
			group.m_sRuntimeStatus = "enemy_counterattack_virtual_returning";
		}
		if (!reason.IsEmpty())
			operation.m_sLastVirtualCombatReason = reason;
		return true;
	}

	protected void TryApplyCounterattackOwnership(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		out bool completed,
		out bool retryable,
		out string failure)
	{
		completed = false;
		retryable = false;
		failure = "";
		if (!state || !order || !m_OwnershipTransitions)
		{
			failure = "exact enemy counterattack ownership authority is unavailable";
			return;
		}
		HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
		if (!targetZone)
		{
			failure = "exact enemy counterattack target zone is missing";
			return;
		}
		string requestId = "ownership_counterattack_" + order.m_sOperationId;
		HST_OwnershipTransitionRequest request = m_OwnershipTransitions.BuildRequest(
			state,
			order.m_sTargetZoneId,
			order.m_sFactionKey,
			OWNERSHIP_CAUSE,
			OWNERSHIP_SOURCE_TYPE,
			order.m_sOperationId,
			"exact enemy counterattack recaptured the location",
			0,
			requestId);
		request.m_bApplyEnemyConsequences = false;
		request.m_bReconcileSecurity = true;
		request.m_bCreateSecurity = false;
		request.m_bNotify = true;
		HST_OwnershipTransitionResult result = m_OwnershipTransitions.Apply(state, request);
		HST_OwnershipTransitionState receipt;
		if (result)
			receipt = result.m_Transition;
		bool resultCompleted = result && result.m_bAccepted && result.m_bCompleted;
		bool receiptCompleted = receipt && receipt.m_bCompleted && receipt.m_bOwnerApplied;
		bool receiptIdentityExact = receiptCompleted
			&& receipt.m_sRequestId == requestId
			&& receipt.m_sZoneId == targetZone.m_sZoneId
			&& receipt.m_sCause == OWNERSHIP_CAUSE
			&& receipt.m_sSourceType == OWNERSHIP_SOURCE_TYPE;
		bool receiptSourceExact = receiptIdentityExact
			&& receipt.m_sSourceId == order.m_sOperationId
			&& receipt.m_sNewOwnerFactionKey == order.m_sFactionKey;
		bool zoneReceiptExact = targetZone.m_sOwnerFactionKey == order.m_sFactionKey
			&& targetZone.m_sLastOwnershipTransitionRequestId == requestId
			&& targetZone.m_sActiveOwnershipTransitionRequestId.IsEmpty();
		bool exactReceiptCompleted = resultCompleted && receiptSourceExact && zoneReceiptExact;
		if (exactReceiptCompleted)
		{
			completed = true;
			failure = "canonical ownership transition receipt completed";
			return;
		}
		if (result && result.m_bCompleted)
		{
			failure = "canonical ownership transition returned an invalid completion receipt";
			return;
		}
		if (result && (result.m_bNeedsRetry || result.m_bAccepted))
		{
			retryable = true;
			failure = "canonical ownership transition remains pending";
			if (!result.m_sFailureReason.IsEmpty())
				failure = failure + ": " + result.m_sFailureReason;
			return;
		}
		failure = "canonical ownership transition was rejected";
		if (result && !result.m_sFailureReason.IsEmpty())
			failure = failure + ": " + result.m_sFailureReason;
	}

	protected string ValidateCurrentCounterattackTarget(
		HST_CampaignPreset preset,
		HST_EnemyOrderState order,
		HST_ZoneState targetZone)
	{
		if (!preset || !order || !targetZone)
			return "exact enemy counterattack target authority is missing";
		// Matching live ownership can be the mid-transaction owner effect of the
		// stable counterattack receipt. The caller must replay that receipt until
		// it is fully completed before the operation is allowed to return.
		if (targetZone.m_sOwnerFactionKey == order.m_sFactionKey)
			return "";
		if (targetZone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
			return "exact enemy counterattack frozen target is no longer resistance-owned";
		return "";
	}

	protected int ResolveCombatPresenceStrength(HST_CombatPresenceResult presence)
	{
		if (!presence)
			return 0;
		return Math.Max(0, presence.m_iInfantryCount)
			+ Math.Max(0, presence.m_iMannedVehicleCount)
			+ Math.Max(0, presence.m_iStaticOperatorCount);
	}

	protected bool SetEngagementMode(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationEngagementMode desired)
	{
		if (!state || !order)
			return false;
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		if (!operation || operation.m_eEngagementMode == desired)
			return false;
		bool changed;
		if (desired == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED)
		{
			if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
			{
				HST_OperationTransitionResult contact = m_Operations.RecordExactEnemyCounterattackEngagement(
					state,
					order,
					HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT);
				changed = contact && contact.m_bStateChanged;
			}
			if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT)
			{
				HST_OperationTransitionResult engaged = m_Operations.RecordExactEnemyCounterattackEngagement(
					state,
					order,
					HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED);
				changed = (engaged && engaged.m_bStateChanged) || changed;
			}
		}
		else if (desired == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
		{
			if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT)
			{
				HST_OperationTransitionResult engaged = m_Operations.RecordExactEnemyCounterattackEngagement(
					state,
					order,
					HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED);
				changed = engaged && engaged.m_bStateChanged;
			}
			if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED)
			{
				HST_OperationTransitionResult disengaging = m_Operations.RecordExactEnemyCounterattackEngagement(
					state,
					order,
					HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_DISENGAGING);
				changed = (disengaging && disengaging.m_bStateChanged) || changed;
			}
			if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_DISENGAGING)
			{
				HST_OperationTransitionResult cleared = m_Operations.RecordExactEnemyCounterattackEngagement(
					state,
					order,
					HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR);
				changed = (cleared && cleared.m_bStateChanged) || changed;
			}
		}
		return changed;
	}

	protected bool SetRuntimeEvidence(
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		string orderStatus,
		string groupStatus,
		string evidence)
	{
		bool changed;
		if (order && order.m_sRuntimeStatus != orderStatus)
		{
			order.m_sRuntimeStatus = orderStatus;
			changed = true;
		}
		if (order && order.m_sFailureReason != evidence)
		{
			order.m_sFailureReason = evidence;
			changed = true;
		}
		if (group && group.m_sRuntimeStatus != groupStatus)
		{
			group.m_sRuntimeStatus = groupStatus;
			group.m_iLifecycleRevision++;
			changed = true;
		}
		return changed;
	}

	protected string ValidateSettledAuthority(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		if (!order || !operation || !manifest)
			return "exact enemy counterattack settled authority is incomplete";
		if (!IsExactEnemyCounterattack(order)
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| operation.m_sOperationId != order.m_sOperationId
			|| operation.m_sEnemyOrderId != order.m_sOrderId
			|| operation.m_sManifestId != manifest.m_sManifestId)
			return "exact enemy counterattack settled identity conflicts";
		if (manifest.m_sManifestId != order.m_sManifestId
			|| manifest.m_sOperationId != order.m_sOperationId
			|| manifest.m_sManifestHash != order.m_sManifestHash
			|| manifest.m_sForceKind != EXACT_FORCE_KIND
			|| manifest.m_sPolicyId != EXACT_POLICY_ID
			|| manifest.m_sIntentId != EXACT_MANIFEST_INTENT)
			return "exact enemy counterattack settled manifest conflicts";
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			|| operation.m_sSettlementId != order.m_sResourceSettlementId)
			return "exact enemy counterattack terminal receipt conflicts";
		return "";
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
				"exact enemy counterattack held projection failed");
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
				"exact enemy counterattack virtual batch is not strategically held");
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
				"exact enemy counterattack virtual roster eliminated");
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
				"exact enemy counterattack roster was eliminated");
		}

		if (!m_StrategicMovement.InitializeExactInfantryDirectRoute(state, operation, manifest, group))
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
				"exact enemy counterattack strategic route is invalid");
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
				"exact enemy counterattack returned to its origin");
		}

		bool changed;
		HST_StrategicMovementResult movement = m_StrategicMovement.AdvanceExactInfantryDirectRoute(state, operation, group);
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
				"exact enemy counterattack strategic movement was rejected");
		}
		changed = movement.m_bStateChanged;
		if (movement.m_bArrived)
		{
			if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
			{
				HST_OperationTransitionResult onStation = m_Operations.MarkExactEnemyCounterattackOnStation(state, order, group);
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
						"exact enemy counterattack virtual arrival transition failed") || changed;
				}
				group.m_sRuntimeStatus = "enemy_counterattack_virtual_on_station";
				order.m_sRuntimeStatus = "exact_virtual_on_station";
				changed = true;
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
					"exact enemy counterattack returned to its origin") || changed;
			}
		}

		HST_OperationProjectionDecision decision = m_Materialization.EvaluateExactEnemyCounterattack(operation, operation.m_vStrategicPosition);
		operation.m_sLastProjectionReason = decision.m_sReason;
		if (decision.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE)
			return BeginMaterialization(state, enemyDirector, order, operation, manifest, batch, group, decision.m_sReason) || changed;

		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
		{
			order.m_sRuntimeStatus = "exact_virtual_returning";
			group.m_sRuntimeStatus = "enemy_counterattack_virtual_returning";
		}
		else
		{
			order.m_sRuntimeStatus = "exact_virtual_outbound";
			group.m_sRuntimeStatus = "enemy_counterattack_virtual";
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
				"exact enemy counterattack materialization failed");
		}
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED || !group.m_bSpawnedEntity)
			return false;

		HST_OperationTransitionResult physical = m_Operations.MarkExactEnemyCounterattackPhysical(
			state,
			order,
			group,
			batch,
			"exact enemy counterattack materialized at the strategic cursor");
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
				"exact enemy counterattack physical handoff conflicts with operation authority");
		}
		bool routeRestarted = m_PhysicalWar.RestartExactEnemyCounterattackInfantryRoute(
			state,
			group,
			operation.m_vRouteEndPosition,
			"Exact enemy counterattack following its authoritative live route.");
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
				"exact enemy counterattack live route could not be issued");
		}
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
		{
			order.m_sRuntimeStatus = "exact_physical_on_station";
			group.m_sRuntimeStatus = "enemy_counterattack_on_station";
		}
		else if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
		{
			order.m_sRuntimeStatus = "exact_physical_returning";
			group.m_sRuntimeStatus = "enemy_counterattack_returning";
		}
		else
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
			return SettlePhysicalFailure(state, enemyDirector, order, operation, manifest, batch, group, 0, "exact enemy counterattack physical batch is not successful");
		if (!group.m_bSpawnedEntity || group.m_sRuntimeStatus == "spawn_failed"
			|| group.m_sRuntimeStatus.Contains("runtime_binding_missing"))
		{
			return SettlePhysicalFailure(state, enemyDirector, order, operation, manifest, batch, group, 0, "exact enemy counterattack physical runtime binding is missing or ambiguous");
		}

		HST_OperationTransitionResult live = m_Operations.UpdateExactEnemyCounterattackPhysicalPosition(state, order, group);
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
				"exact enemy counterattack live-position handoff failed");
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
					"exact enemy counterattack elimination cleanup is pending: " + eliminationReason) || changed;
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
				"exact enemy counterattack was eliminated") || changed;
		}
		if (m_PhysicalWar.IsExactEnemyCounterattackRouteRecoveryExhausted(group, state.m_iElapsedSeconds))
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
				"exact enemy counterattack exhausted bounded live-route recovery") || changed;
		}

		HST_OperationTransitionResult sample = m_Operations.ConfirmExactEnemyCounterattackArrivalSample(state, order, group);
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
				"exact enemy counterattack live arrival sampling failed") || changed;
		}
		changed = sample.m_bStateChanged || changed;
		if (operation.m_iArrivalConfirmationCount >= 2)
		{
			if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
			{
				HST_OperationTransitionResult onStation = m_Operations.MarkExactEnemyCounterattackOnStation(state, order, group);
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
						"exact enemy counterattack physical arrival transition failed") || changed;
				}
				group.m_sRuntimeStatus = "enemy_counterattack_on_station";
				order.m_sRuntimeStatus = "exact_physical_on_station";
				changed = true;
				return changed;
			}
			if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
				return TryDematerialize(state, enemyDirector, order, operation, manifest, batch, group, "return arrival folded for exact survivor settlement") || changed;
		}

		HST_OperationProjectionDecision decision = m_Materialization.EvaluateExactEnemyCounterattack(operation, group.m_vPosition);
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
		int deadlineSecond = state.m_iElapsedSeconds + EXACT_COUNTERATTACK_DEPLOYMENT_GRACE_SECONDS;
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
				"exact enemy counterattack materialization release failed");
		}
		group.m_vPosition = operation.m_vStrategicPosition;
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		group.m_vTargetPosition = operation.m_vRouteEndPosition;
		group.m_sRuntimeStatus = "enemy_counterattack_materializing";
		HST_OperationTransitionResult materializing = m_Operations.MarkExactEnemyCounterattackMaterializingFromVirtual(
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
			group.m_sRuntimeStatus = "enemy_counterattack_virtual";
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
				"exact enemy counterattack materialization transition failed");
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
		int deadlineSecond = state.m_iElapsedSeconds + EXACT_COUNTERATTACK_DEPLOYMENT_GRACE_SECONDS;
		HST_ForceSpawnQueueCallbackResult preflight = m_SpawnQueue.CanRequeueSuccessfulProjectionForStrategicHold(
			state.m_aForceSpawnResults,
			manifest,
			batch.m_sResultId,
			batch.m_sProjectionId,
			state.m_iElapsedSeconds,
			deadlineSecond);
		if (!preflight || !preflight.m_bAccepted)
			return SetRuntimeConflict(order, "exact enemy counterattack dematerialization is waiting for queue capacity");
		HST_OperationTransitionResult begun = m_Operations.BeginExactEnemyCounterattackDematerialization(state, order, group, reason);
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
				"exact enemy counterattack dematerialization transition failed");
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
				return SetRuntimeConflict(order, "exact enemy counterattack runtime retirement is pending");
			int deadlineSecond = state.m_iElapsedSeconds + EXACT_COUNTERATTACK_DEPLOYMENT_GRACE_SECONDS;
			HST_ForceSpawnQueueCallbackResult held = m_SpawnQueue.RequeueSuccessfulProjectionForStrategicHold(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				state.m_iElapsedSeconds,
				deadlineSecond);
			if (!held || !held.m_bAccepted)
				return SetRuntimeConflict(order, "exact enemy counterattack survivor projection could not enter strategic hold");
		}

		m_StrategicMovement.SyncRouteProgressFromPosition(operation, group.m_vPosition);
		group.m_bSpawnedEntity = false;
		group.m_sRuntimeEntityId = "";
		group.m_sRuntimeStatus = "enemy_counterattack_virtual";
		group.m_iSpawnedAgentCount = 0;
		SyncGroupRoster(group, m_SpawnQueue.CountStrategicLivingMemberSlots(batch));
		group.m_iLifecycleRevision++;
		HST_OperationTransitionResult completed = m_Operations.CompleteExactEnemyCounterattackDematerialization(
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
				"exact enemy counterattack dematerialization completion failed");
		}
		order.m_bPhysicalized = false;
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
		{
			order.m_sRuntimeStatus = "exact_virtual_returning";
			group.m_sRuntimeStatus = "enemy_counterattack_virtual_returning";
		}
		else if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
		{
			order.m_sRuntimeStatus = "exact_virtual_on_station";
			group.m_sRuntimeStatus = "enemy_counterattack_virtual_on_station";
		}
		else
		{
			order.m_sRuntimeStatus = "exact_virtual_outbound";
			group.m_sRuntimeStatus = "enemy_counterattack_virtual";
		}
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
		HST_OperationTransitionResult prepared = m_Operations.CanPrepareExactEnemyCounterattackSettlement(
			state,
			order,
			terminalResult,
			settlementId);
		if (!prepared || !prepared.m_bAccepted)
			return SetRuntimeConflict(order, "exact enemy counterattack settlement eligibility preflight failed");
		if (prepared.m_bAlreadyApplied)
		{
			HST_OperationTransitionResult replay = m_Operations.CanSettleExactEnemyCounterattack(
				state,
				order,
				terminalResult,
				settlementId);
			if (!replay || !replay.m_bAccepted)
				return SetRuntimeConflict(order, "exact enemy counterattack settled receipt conflicts with its resource ledger");
			return FinalizeSettledOrder(state, order, replay.m_Operation, batch, group);
		}
		if (!ApplyResourceSettlement(
			state,
			enemyDirector,
			order,
			manifest,
			settlementKind,
			terminalResult,
			survivors,
			false,
			reason))
			return SetRuntimeConflict(order, "exact enemy counterattack resource settlement conflicted");
		HST_OperationTransitionResult preflight = m_Operations.CanSettleExactEnemyCounterattack(
			state,
			order,
			terminalResult,
			settlementId);
		if (!preflight || !preflight.m_bAccepted)
			return SetRuntimeConflict(order, "exact enemy counterattack settlement preflight failed");
		if (preflight.m_bAlreadyApplied)
			return FinalizeSettledOrder(state, order, preflight.m_Operation, batch, group);
		HST_OperationTransitionResult settled = m_Operations.SettleExactEnemyCounterattack(
			state,
			order,
			terminalResult,
			settlementId,
			reason);
		if (!settled || !settled.m_bAccepted)
			return SetRuntimeConflict(order, "exact enemy counterattack operation settlement failed");

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
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
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
				return SetRuntimeConflict(order, "exact enemy counterattack fail-closed runtime retirement is pending: " + reason);
		}
		else if (handleCount > 0)
			return SetRuntimeConflict(order, "exact enemy counterattack ambiguous handles cannot be retired safely: " + reason);
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
			SetRuntimeConflict(order, "committed exact enemy counterattack admission cannot be rolled back: " + reason);
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
			HST_OperationTransitionResult eligibility = m_Operations.CanPrepareExactEnemyCounterattackSettlement(
				state,
				order,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				operationSettlementId);
			if (!eligibility || !eligibility.m_bAccepted)
			{
				HST_OperationTransitionResult removed = m_Operations.RemoveUncommittedExactEnemyCounterattack(state, order, manifest);
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
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
			admissionAcceptedCount,
			true,
			reason);
		if (operation && manifest)
		{
			HST_OperationTransitionResult preflight = m_Operations.CanSettleExactEnemyCounterattack(
				state,
				order,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				operationSettlementId);
			if (preflight && preflight.m_bAccepted && !preflight.m_bAlreadyApplied)
				m_Operations.SettleExactEnemyCounterattack(
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
		HST_EOperationTerminalResult terminalResult,
		int survivorCount,
		bool fullRefund,
		string reason)
	{
		if (!state || !enemyDirector || !order || settlementKind.IsEmpty())
			return false;
		if (!HST_EnemyCounterattackSaveValidationService.ValidateOriginalResourceDebitAuthority(
			state.m_aEnemyStrategicMutations,
			order).IsEmpty())
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
		HST_EOperationTerminalResult policyTerminal;
		bool policyFullRefund;
		bool policyRequiresRecapture;
		if (!HST_EnemyCounterattackSaveValidationService.ResolveSettlementPolicy(
			settlementKind,
			policyTerminal,
			policyFullRefund,
			policyRequiresRecapture)
			|| policyTerminal != terminalResult || policyFullRefund != fullRefund
			|| order.m_bStrategicServiceCommitted == fullRefund)
			return false;
		if (policyRequiresRecapture && (!order.m_bOutcomeApplied
			|| order.m_sResolutionKind != "exact_counterattack_recaptured"))
			return false;
		if (order.m_bResourceSettlementApplied)
		{
			bool fieldsMatch = order.m_sResourceSettlementId == settlementId
				&& order.m_sResourceSettlementKind == settlementKind
				&& order.m_iSettlementAcceptedMemberCount == accepted
				&& order.m_iSettlementSurvivorMemberCount == survivors
				&& order.m_iRefundedAttackResources == attackRefund
				&& order.m_iRefundedSupportResources == supportRefund;
			return fieldsMatch
				&& HST_EnemyCounterattackSaveValidationService.ValidateSettledResourceRefundAuthority(
					state.m_aEnemyStrategicMutations,
					order).IsEmpty();
		}
		if ((attackRefund > 0 || supportRefund > 0) && !state.FindFactionPool(order.m_sFactionKey))
			return false;
		string refundMutationId = "enemy_resource_refund_" + settlementId;
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		if (!operation && !fullRefund)
			return false;

		bool stagedTuple = !order.m_sResourceSettlementId.IsEmpty()
			|| !order.m_sResourceSettlementKind.IsEmpty()
			|| !order.m_sResourceRefundMutationId.IsEmpty()
			|| order.m_iSettlementAcceptedMemberCount != 0
			|| order.m_iSettlementSurvivorMemberCount != 0
			|| order.m_iRefundedAttackResources != 0
			|| order.m_iRefundedSupportResources != 0;
		if (!stagedTuple)
		{
			if (operation)
			{
				HST_OperationTransitionResult prepared
					= m_Operations.PrepareExactEnemyCounterattackSettlement(
						state,
						order,
						terminalResult,
						settlementId,
						reason);
				if (!prepared || !prepared.m_bAccepted)
					return false;
			}
			order.m_sResourceSettlementId = settlementId;
			order.m_sResourceSettlementKind = settlementKind;
			order.m_iSettlementAcceptedMemberCount = accepted;
			order.m_iSettlementSurvivorMemberCount = survivors;
			order.m_iRefundedAttackResources = attackRefund;
			order.m_iRefundedSupportResources = supportRefund;
			order.m_sResourceRefundMutationId = refundMutationId;
		}
		else
		{
			bool tupleExact = order.m_sResourceSettlementId == settlementId
				&& order.m_sResourceSettlementKind == settlementKind
				&& order.m_sResourceRefundMutationId == refundMutationId
				&& order.m_iSettlementAcceptedMemberCount == accepted
				&& order.m_iSettlementSurvivorMemberCount == survivors
				&& order.m_iRefundedAttackResources == attackRefund
				&& order.m_iRefundedSupportResources == supportRefund;
			if (!tupleExact)
				return false;
			if (operation)
			{
				HST_OperationTransitionResult preparedReplay
					= m_Operations.CanPrepareExactEnemyCounterattackSettlement(
						state,
						order,
						terminalResult,
						settlementId);
				if (!preparedReplay || !preparedReplay.m_bAccepted
					|| operation.m_eSettlementState
						!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
					return false;
			}
		}
		if (!HST_EnemyCounterattackSaveValidationService.ValidatePreparedResourceSettlementTuple(
			order).IsEmpty())
			return false;

		if (operation)
		{
			HST_OperationTransitionResult resourcePreflight
				= m_Operations.CanRecordExactEnemyCounterattackResourceSettlement(
					state,
					order,
					settlementKind,
					accepted,
					survivors);
			if (!resourcePreflight || !resourcePreflight.m_bAccepted)
				return false;
		}
		if (!RefundOriginallyChargedResources(
			state,
			enemyDirector,
			order,
			attackRefund,
			supportRefund,
			reason,
			refundMutationId,
			settlementId))
			return false;

		if (operation)
		{
			HST_OperationTransitionResult recorded = m_Operations.RecordExactEnemyCounterattackResourceSettlement(
				state,
				order,
				settlementKind,
				accepted,
				survivors);
			if (!recorded || !recorded.m_bAccepted)
				return false;
		}
		else
			order.m_bResourceSettlementApplied = true;
		return HST_EnemyCounterattackSaveValidationService.ValidateSettledResourceRefundAuthority(
			state.m_aEnemyStrategicMutations,
			order).IsEmpty();
	}

	protected string ValidateAdmissionContext(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_EnemyDirectorService enemyDirector)
	{
		if (!state || !order || !manifest || !enemyDirector || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar
			|| !m_CombatPresence || !m_OwnershipTransitions)
			return "exact enemy counterattack admission services are unavailable";
		if (!IsExactEnemyCounterattack(order) || order.m_sOrderId.IsEmpty()
			|| order.m_sOperationId != HST_StableIdService.BuildOperationId("enemy_order", order.m_sOrderId))
			return "exact enemy counterattack order identity is invalid";
		if (order.m_sFactionKey.IsEmpty() || order.m_sSourceZoneId.IsEmpty() || order.m_sTargetZoneId.IsEmpty()
			|| HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
				order.m_sSourceZoneId, order.m_sTargetZoneId))
			return "exact enemy counterattack source, target, or faction identity is invalid";
		bool attackFunded = order.m_iAttackCost > 0 && order.m_iSupportCost == 0;
		bool supportFunded = order.m_iSupportCost > 0 && order.m_iAttackCost == 0;
		if (!attackFunded && !supportFunded)
			return "exact enemy counterattack must charge exactly one positive strategic resource pool";
		if (!manifest.m_bFrozen || manifest.m_sOperationId != order.m_sOperationId
			|| manifest.m_sForceKind != EXACT_FORCE_KIND
			|| manifest.m_sPolicyId != EXACT_POLICY_ID)
			return "exact enemy counterattack frozen manifest is invalid";
		if (manifest.m_sIntentId != EXACT_MANIFEST_INTENT
			|| manifest.m_sFactionKey != order.m_sFactionKey
			|| manifest.m_sSourceZoneId != order.m_sSourceZoneId
			|| manifest.m_sTargetZoneId != order.m_sTargetZoneId)
			return "exact enemy counterattack frozen manifest is invalid";
		if (manifest.m_iAttackResourceCost != order.m_iAttackCost
			|| manifest.m_iSupportResourceCost != order.m_iSupportCost
			|| !m_StrategicMovement.IsSupportedExactInfantryManifest(manifest))
			return "exact enemy counterattack frozen manifest is invalid";
		if (manifest.m_sManifestHash.IsEmpty()
			|| m_Integrity.BuildManifestHash(manifest) != manifest.m_sManifestHash)
			return "exact enemy counterattack frozen manifest is invalid";
		foreach (HST_EnemyOrderState other : state.m_aEnemyOrders)
		{
			if (!other || other == order || other.m_eType != HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
				continue;
			bool otherOpen = other.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
				|| other.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED;
			if (!otherOpen)
			{
				otherOpen = HasPreparedSettlementResumeCandidate(
					other,
					state.FindOperation(other.m_sOperationId));
			}
			if (other.m_sOperationId == order.m_sOperationId || (otherOpen
				&& other.m_sFactionKey == order.m_sFactionKey
				&& HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					other.m_sTargetZoneId, order.m_sTargetZoneId)))
				return "another exact enemy counterattack already owns this identity or target";
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
		string debitFailure = HST_EnemyCounterattackSaveValidationService.ValidateOriginalResourceDebitAuthority(
			state.m_aEnemyStrategicMutations,
			order);
		if (!debitFailure.IsEmpty())
			return debitFailure;
		operation = state.FindOperation(order.m_sOperationId);
		manifest = state.FindForceManifest(order.m_sManifestId);
		batch = state.FindForceSpawnResult(order.m_sSpawnResultId);
		group = state.FindActiveGroup(order.m_sGroupId);
		if (!operation || !manifest || !batch || !group)
			return "exact enemy counterattack runtime authority is incomplete";
		string failure = m_Operations.ValidateExactEnemyCounterattack(state, operation, order, manifest);
		if (!failure.IsEmpty())
			return failure;
		if (operation.m_sSpawnResultId != batch.m_sResultId || operation.m_sGroupId != group.m_sGroupId
			|| batch.m_sOperationId != operation.m_sOperationId || batch.m_sManifestId != manifest.m_sManifestId
			|| group.m_sOperationId != operation.m_sOperationId || group.m_sEnemyOrderId != order.m_sOrderId
			|| group.m_sManifestId != manifest.m_sManifestId || group.m_sSpawnResultId != batch.m_sResultId
			|| group.m_sProjectionId != batch.m_sProjectionId || group.m_sForceId != batch.m_sForceId)
			return "exact enemy counterattack reciprocal runtime links conflict";
		if (CountForceSpawnResultId(state, batch.m_sResultId) != 1)
			return "exact enemy counterattack spawn-result identity is ambiguous";
		if (CountActiveGroupId(state, group.m_sGroupId) != 1)
			return "exact enemy counterattack active-group identity is ambiguous";
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN
			|| order.m_bOutcomeApplied)
		{
			string ownershipFailure = HST_EnemyCounterattackSaveValidationService.ValidateCompletedOwnershipAuthority(
				state.m_aZones,
				state.m_aOwnershipTransitions,
				order,
				operation);
			if (!ownershipFailure.IsEmpty())
				return ownershipFailure;
		}
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
		group.m_sSpawnFallbackMode = EXACT_GROUP_MODE;
		group.m_sRouteId = order.m_sOperationId + "_outbound";
		group.m_vSourcePosition = order.m_vSourcePosition;
		group.m_vTargetPosition = order.m_vTargetPosition;
		group.m_vPosition = order.m_vSourcePosition;
		group.m_sRuntimeStatus = "enemy_counterattack_virtual";
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
			return "exact enemy counterattack active-group context is missing";
		if (group.m_sGroupId != projectionId || group.m_sProjectionId != projectionId
			|| group.m_sForceId != forceId || group.m_sSpawnResultId != resultId
			|| group.m_sOperationId != order.m_sOperationId || group.m_sEnemyOrderId != order.m_sOrderId
			|| group.m_sManifestId != manifest.m_sManifestId || group.m_sFactionKey != order.m_sFactionKey
			|| group.m_iOriginalInfantryCount != manifest.m_iAcceptedMemberCount || group.m_iVehicleCount != 0)
			return "exact enemy counterattack active-group identity or roster conflicts";
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
		group.m_iInfantryCount = bounded;
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
		string failure = "exact enemy counterattack runtime authority is quarantined; no roster refund or runtime retirement was attempted: " + reason;
		if (order.m_sRuntimeStatus == status && order.m_sFailureReason == failure)
			return false;
		order.m_sRuntimeStatus = status;
		order.m_sFailureReason = failure;
		return true;
	}

	protected bool HoldInvalidResourceAuthority(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		string reason)
	{
		if (!state || !order)
			return false;
		if (reason.IsEmpty())
			reason = "exact enemy counterattack resource authority is incomplete";
		string failure = "exact enemy counterattack resource authority is invalid; runtime ownership remains open and no refund, spawn, retirement, settlement, or outcome was attempted: " + reason;
		bool changed = order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			|| order.m_sRuntimeStatus != "exact_resource_authority_held"
			|| order.m_sFailureReason != failure;
		// Invalid resource evidence is not proof that a live projection can be
		// retired or that its prepaid roster can be refunded. Keep the exact
		// runtime owner active so every later tick retries the authority check.
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
		order.m_sRuntimeStatus = "exact_resource_authority_held";
		order.m_sFailureReason = failure;
		if (order.m_iResolvedAtSecond != 0)
		{
			order.m_iResolvedAtSecond = 0;
			changed = true;
		}

		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		if (operation
			&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			// Invalid resource evidence cannot prove a live projection is safe to
			// retire. Preserve its position/materialization authority and every
			// reciprocal identifier while later ticks retry validation.
			if (operation.m_sLastProjectionReason != failure)
			{
				operation.m_sLastProjectionReason = failure;
				operation.m_iRevision++;
				changed = true;
			}
		}

		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(order.m_sSpawnResultId);
		if (!batch && operation)
			batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
		if (batch)
		{
			if (batch.m_bCancelRequested || batch.m_sLastFailureReason != failure)
				changed = true;
			// Preserve the batch's physical/strategic authority class. Converting a
			// live physical handoff into a strategic hold would change which roster
			// is authoritative while the resource evidence is unresolved.
			batch.m_bCancelRequested = false;
			batch.m_sLastFailureReason = failure;
		}

		HST_ActiveGroupState group = state.FindActiveGroup(order.m_sGroupId);
		if (!group && operation)
			group = state.FindActiveGroup(operation.m_sGroupId);
		if (group)
		{
			if (group.m_sSpawnFailureReason != failure)
				changed = true;
			// The order owns the retry hold. Leave the group's materialization/runtime
			// status untouched so a physical projection does not change authority.
			group.m_sSpawnFailureReason = failure;
		}
		return changed;
	}

	protected string FindAmbiguousAuthorityRows(HST_CampaignState state, HST_EnemyOrderState order)
	{
		if (!state || !order)
			return "exact enemy counterattack authority context is missing";
		if (state.FindEnemyOrder(order.m_sOrderId) != order
			|| CountEnemyOrdersByAnyAuthorityIdentity(state, order) != 1)
			return "exact enemy counterattack enemy-order identity is ambiguous";
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		int operationCount = CountOperationsByAnyAuthorityIdentity(state, order);
		if ((operation && operationCount != 1) || (!operation && operationCount > 0))
			return "exact enemy counterattack operation identity is ambiguous";
		HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
		int manifestCount = CountForceManifestsByAnyAuthorityIdentity(state, order);
		if ((manifest && manifestCount != 1) || (!manifest && manifestCount > 0))
			return "exact enemy counterattack manifest identity is ambiguous";
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
		bool optionalUncommittedRuntime = IsUncommittedFullSettlementIntent(order);
		if ((batch && batchCount != 1) || batchCount > 1
			|| (!batch && batchCount > 0 && !optionalUncommittedRuntime))
			return "exact enemy counterattack spawn-result identity is ambiguous";
		HST_ActiveGroupState group = state.FindActiveGroup(groupId);
		int groupCount = CountActiveGroupsByAnyAuthorityIdentity(state, order, operation, group);
		if ((group && groupCount != 1) || groupCount > 1
			|| (!group && groupCount > 0 && !optionalUncommittedRuntime))
			return "exact enemy counterattack active-group identity is ambiguous";
		return "";
	}

	protected int CountEnemyOrdersByAnyAuthorityIdentity(HST_CampaignState state, HST_EnemyOrderState expected)
	{
		int count;
		if (!state || !expected)
			return count;
		string deterministicResultId;
		if (!expected.m_sOrderId.IsEmpty())
			deterministicResultId = BuildSpawnResultId(expected);
		string deterministicProjectionId;
		if (!expected.m_sOperationId.IsEmpty())
			deterministicProjectionId = BuildProjectionId(expected);
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
			if (!matches && !deterministicResultId.IsEmpty())
				matches = candidate.m_sSpawnResultId == deterministicResultId;
			if (!matches && !expected.m_sGroupId.IsEmpty())
				matches = candidate.m_sGroupId == expected.m_sGroupId;
			if (!matches && !deterministicProjectionId.IsEmpty())
				matches = candidate.m_sGroupId == deterministicProjectionId;
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
		string deterministicResultId;
		if (!order.m_sOrderId.IsEmpty())
			deterministicResultId = BuildSpawnResultId(order);
		string deterministicProjectionId;
		string deterministicForceId;
		if (!order.m_sOperationId.IsEmpty())
		{
			deterministicProjectionId = BuildProjectionId(order);
			deterministicForceId = BuildForceId(order);
		}
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
			if (!matches && !deterministicResultId.IsEmpty())
				matches = candidate.m_sSpawnResultId == deterministicResultId;
			if (!matches && !order.m_sGroupId.IsEmpty())
				matches = candidate.m_sGroupId == order.m_sGroupId;
			if (!matches && !deterministicProjectionId.IsEmpty())
				matches = candidate.m_sGroupId == deterministicProjectionId
					|| candidate.m_sProjectionId == deterministicProjectionId;
			if (!matches && !deterministicForceId.IsEmpty())
				matches = candidate.m_sForceId == deterministicForceId;
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
		string deterministicResultId;
		if (!order.m_sOrderId.IsEmpty())
			deterministicResultId = BuildSpawnResultId(order);
		string projectionId;
		string forceId;
		string deterministicProjectionId;
		string deterministicForceId;
		if (!order.m_sOperationId.IsEmpty())
		{
			deterministicProjectionId = BuildProjectionId(order);
			deterministicForceId = BuildForceId(order);
		}
		if (operation)
		{
			if (resultId.IsEmpty())
				resultId = operation.m_sSpawnResultId;
			if (!operation.m_sProjectionId.IsEmpty())
				projectionId = operation.m_sProjectionId;
			if (!operation.m_sForceId.IsEmpty())
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
			if (!matches && !deterministicResultId.IsEmpty())
				matches = candidate.m_sResultId == deterministicResultId;
			if (!matches && !order.m_sOrderId.IsEmpty())
				matches = candidate.m_sRequestId == order.m_sOrderId;
			if (!matches && !order.m_sOperationId.IsEmpty())
				matches = candidate.m_sOperationId == order.m_sOperationId;
			if (!matches && !order.m_sManifestId.IsEmpty())
				matches = candidate.m_sManifestId == order.m_sManifestId;
			if (!matches && !projectionId.IsEmpty())
				matches = candidate.m_sProjectionId == projectionId;
			if (!matches && !deterministicProjectionId.IsEmpty())
				matches = candidate.m_sProjectionId == deterministicProjectionId;
			if (!matches && !forceId.IsEmpty())
				matches = candidate.m_sForceId == forceId;
			if (!matches && !deterministicForceId.IsEmpty())
				matches = candidate.m_sForceId == deterministicForceId;
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
		string deterministicResultId;
		if (!order.m_sOrderId.IsEmpty())
			deterministicResultId = BuildSpawnResultId(order);
		string projectionId;
		string forceId;
		string deterministicProjectionId;
		string deterministicForceId;
		if (!order.m_sOperationId.IsEmpty())
		{
			deterministicProjectionId = BuildProjectionId(order);
			deterministicForceId = BuildForceId(order);
		}
		if (operation)
		{
			if (groupId.IsEmpty())
				groupId = operation.m_sGroupId;
			if (resultId.IsEmpty())
				resultId = operation.m_sSpawnResultId;
			if (!operation.m_sProjectionId.IsEmpty())
				projectionId = operation.m_sProjectionId;
			if (!operation.m_sForceId.IsEmpty())
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
			if (!matches && !deterministicProjectionId.IsEmpty())
				matches = candidate.m_sGroupId == deterministicProjectionId
					|| candidate.m_sProjectionId == deterministicProjectionId;
			if (!matches && !order.m_sOrderId.IsEmpty())
				matches = candidate.m_sEnemyOrderId == order.m_sOrderId;
			if (!matches && !order.m_sOperationId.IsEmpty())
				matches = candidate.m_sOperationId == order.m_sOperationId;
			if (!matches && !order.m_sManifestId.IsEmpty())
				matches = candidate.m_sManifestId == order.m_sManifestId;
			if (!matches && !resultId.IsEmpty())
				matches = candidate.m_sSpawnResultId == resultId;
			if (!matches && !deterministicResultId.IsEmpty())
				matches = candidate.m_sSpawnResultId == deterministicResultId;
			if (!matches && !projectionId.IsEmpty())
				matches = candidate.m_sProjectionId == projectionId;
			if (!matches && !forceId.IsEmpty())
				matches = candidate.m_sForceId == forceId;
			if (!matches && !deterministicForceId.IsEmpty())
				matches = candidate.m_sForceId == deterministicForceId;
			if (matches)
				count++;
		}
		return count;
	}

	protected bool HoldAmbiguousAuthority(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		string reason)
	{
		if (!state || !order)
			return false;

		// Ambiguity is not proof that any physical projection is safe to retire or
		// that any prepaid roster is safe to refund. Keep the exact runtime owner
		// open so every later tick retries the same fail-closed check. Marking this
		// order terminal would orphan any live claimant permanently.
		bool changed;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
		{
			order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
			changed = true;
		}
		string failure = "exact enemy counterattack authority is ambiguous; runtime ownership remains open and no retirement, refund, settlement, or outcome was attempted: " + reason;
		if (order.m_sRuntimeStatus != "exact_authority_ambiguous_held"
			|| order.m_sFailureReason != failure)
		{
			order.m_sRuntimeStatus = "exact_authority_ambiguous_held";
			order.m_sFailureReason = failure;
			changed = true;
		}
		if (order.m_iResolvedAtSecond != 0)
		{
			order.m_iResolvedAtSecond = 0;
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
