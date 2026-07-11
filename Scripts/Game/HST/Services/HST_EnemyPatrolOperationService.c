class HST_EnemyPatrolAdmissionResult
{
	bool m_bSuccess;
	bool m_bStateChanged;
	string m_sFailureReason;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
}

// Typed authority for newly queued enemy patrol orders. Historical contract-zero
// patrols remain owned by the legacy commander path.
class HST_EnemyPatrolOperationService
{
	static const int EXACT_CONTRACT_VERSION = 1;
	static const int QUARANTINED_CONTRACT_VERSION = -53;
	static const string EXACT_FORCE_KIND = "enemy_patrol";
	static const string EXACT_POLICY_ID = "exact_enemy_patrol_v1";
	static const string EXACT_MANIFEST_INTENT = "enemy_patrol";
	static const string EXACT_GROUP_MODE = "exact_enemy_patrol";
	static const string ASSIGNMENT_KIND = "patrol_route";
	static const string RECALL_POLICY_ID = "return_to_origin_then_refund_patrol_survivors";
	static const string SETTLEMENT_POLICY_ID = "exact_enemy_patrol_ledger";
	static const int EXACT_PRIORITY = 75;
	static const int EXACT_MAX_RETRIES = 3;
	static const int DEPLOYMENT_GRACE_SECONDS = 180;
	static const int REQUIRED_PATROL_LAPS = 1;
	static const int CONTACT_CLEAR_SECONDS = 30;
	static const float CONTACT_RADIUS_METERS = 225.0;
	static const float PHYSICAL_ARRIVAL_RADIUS_METERS = 35.0;

	protected ref HST_OperationRouteCursorService m_RouteCursor = new HST_OperationRouteCursorService();
	protected ref HST_MaterializationService m_Materialization = new HST_MaterializationService();
	protected ref HST_ForcePlanningIntegrityService m_Integrity = new HST_ForcePlanningIntegrityService();
	protected ref HST_StrategicMovementService m_StrategicMovement = new HST_StrategicMovementService();
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

	bool IsExactEnemyPatrol(HST_EnemyOrderState order)
	{
		return order && order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL
			&& order.m_iOperationContractVersion == EXACT_CONTRACT_VERSION;
	}

	bool HasOpenExactEnemyPatrol(
		HST_CampaignState state,
		string factionKey,
		string targetZoneId)
	{
		if (!state || factionKey.IsEmpty() || targetZoneId.IsEmpty())
			return false;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsExactEnemyPatrol(order) || order.m_sFactionKey != factionKey
				|| !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					order.m_sTargetZoneId, targetZoneId))
				continue;
			if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				continue;
			HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
			if (!operation || operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				return true;
		}
		return false;
	}

	HST_GeneratedRouteState ResolvePatrolRoute(
		HST_CampaignState state,
		HST_EnemyOrderState order)
	{
		if (!state || !order || order.m_sTargetZoneId.IsEmpty())
			return null;
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		if (operation && !operation.m_sCurrentRouteId.IsEmpty())
		{
			HST_GeneratedRouteState frozen = state.FindGeneratedRoute(operation.m_sCurrentRouteId);
			if (frozen)
				return frozen;
		}
		HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
		if (targetZone && !targetZone.m_sPatrolRouteId.IsEmpty())
		{
			HST_GeneratedRouteState configured = state.FindGeneratedRoute(targetZone.m_sPatrolRouteId);
			if (configured)
				return configured;
		}
		return state.FindGeneratedRoute("route_" + order.m_sTargetZoneId + "_alpha");
	}

	HST_EnemyPatrolAdmissionResult CanAdmitPreparedOrder(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_GeneratedRouteState route,
		HST_EnemyDirectorService enemyDirector)
	{
		HST_EnemyPatrolAdmissionResult result = new HST_EnemyPatrolAdmissionResult();
		string failure = ValidateAdmissionContext(state, order, manifest, route, enemyDirector);
		if (!failure.IsEmpty())
		{
			result.m_sFailureReason = failure;
			return result;
		}
		string identityCollision = FindAdmissionIdentityCollision(state, order, manifest);
		if (!identityCollision.IsEmpty())
		{
			result.m_sFailureReason = identityCollision;
			return result;
		}

		HST_ForceSpawnQueueRequest request = BuildSpawnRequest(state, order);
		HST_ForceSpawnQueueEnqueueResult preflight = m_SpawnQueue.CanEnqueue(
			state.m_aForceSpawnResults,
			manifest,
			request,
			state.m_iElapsedSeconds);
		if (!preflight || !preflight.m_bSuccess)
		{
			result.m_sFailureReason = "exact enemy patrol spawn admission preflight failed";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = result.m_sFailureReason + ": " + preflight.m_sFailureReason;
			return result;
		}
		result.m_bSuccess = true;
		return result;
	}

	HST_EnemyPatrolAdmissionResult AdmitPreparedOrder(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_GeneratedRouteState route,
		HST_EnemyDirectorService enemyDirector)
	{
		HST_EnemyPatrolAdmissionResult result = new HST_EnemyPatrolAdmissionResult();
		if (HasCommittedAdmissionAuthority(state, order))
			return ResolveCommittedAdmissionReplay(state, order, manifest, route);
		HST_EnemyPatrolAdmissionResult preflight = CanAdmitPreparedOrder(
			state,
			order,
			manifest,
			route,
			enemyDirector);
		if (!preflight || !preflight.m_bSuccess)
		{
			string preflightFailure = "exact enemy patrol admission preflight failed";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				preflightFailure = preflight.m_sFailureReason;
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, null, null, null, preflightFailure);
			result.m_sFailureReason = preflightFailure;
			result.m_bStateChanged = true;
			return result;
		}

		ApplyManifestComposition(order, manifest);
		state.m_aForceManifests.Insert(manifest);
		HST_OperationRecordState operation = BuildOperation(state, order, manifest);
		HST_ActiveGroupState group = BuildActiveGroup(state, order, manifest);
		if (!operation || !group || !m_RouteCursor.FreezePatrolRoute(state, operation, route, group))
		{
			RemoveInsertedAdmissionRows(state, operation, manifest, group);
			string routeFailure = "exact enemy patrol frozen route initialization failed";
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, null, null, null, routeFailure);
			result.m_sFailureReason = routeFailure;
			result.m_bStateChanged = true;
			return result;
		}
		state.m_aOperations.Insert(operation);
		state.m_aActiveGroups.Insert(group);
		result.m_Operation = operation;
		result.m_Group = group;

		HST_ForceSpawnQueueRequest request = BuildSpawnRequest(state, order);
		HST_ForceSpawnQueueEnqueueResult enqueue = m_SpawnQueue.Enqueue(
			state.m_aForceSpawnResults,
			manifest,
			request,
			state.m_iElapsedSeconds);
		if (!enqueue || !enqueue.m_bSuccess || !enqueue.m_Batch)
		{
			string enqueueFailure = "exact enemy patrol spawn admission failed";
			if (enqueue && !enqueue.m_sFailureReason.IsEmpty())
				enqueueFailure = enqueueFailure + ": " + enqueue.m_sFailureReason;
			HST_ForceSpawnResultState insertedBatch;
			if (enqueue && enqueue.m_bStateChanged)
				insertedBatch = enqueue.m_Batch;
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, operation, insertedBatch, group, enqueueFailure);
			result.m_sFailureReason = enqueueFailure;
			result.m_bStateChanged = true;
			return result;
		}
		result.m_Batch = enqueue.m_Batch;

		HST_ForceSpawnQueueCallbackResult held = m_SpawnQueue.HoldPendingProjectionForStrategicSimulation(
			state.m_aForceSpawnResults,
			manifest,
			enqueue.m_Batch.m_sResultId,
			enqueue.m_Batch.m_sProjectionId,
			state.m_iElapsedSeconds);
		if (!held || !held.m_bAccepted)
		{
			string holdFailure = "exact enemy patrol strategic hold failed";
			if (held && !held.m_sFailureReason.IsEmpty())
				holdFailure = holdFailure + ": " + held.m_sFailureReason;
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, operation, enqueue.m_Batch, group, holdFailure);
			result.m_sFailureReason = holdFailure;
			result.m_bStateChanged = true;
			return result;
		}

		LinkCommittedAdmission(state, order, operation, enqueue.m_Batch, group);
		result.m_bSuccess = true;
		result.m_bStateChanged = true;
		return result;
	}

	protected string ValidateAdmissionContext(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_GeneratedRouteState route,
		HST_EnemyDirectorService enemyDirector)
	{
		if (!state || !order || !manifest || !route || !enemyDirector || !m_SpawnQueue
			|| !m_SpawnAdapter || !m_PhysicalWar || !m_RouteCursor || !m_Integrity)
			return "exact enemy patrol admission services are unavailable";
		if (!IsExactEnemyPatrol(order) || order.m_sOrderId.IsEmpty()
			|| order.m_sOperationId != HST_StableIdService.BuildOperationId("enemy_order", order.m_sOrderId)
			|| order.m_sFactionKey.IsEmpty() || order.m_sSourceZoneId.IsEmpty()
			|| order.m_sTargetZoneId.IsEmpty() || order.m_sSourceZoneId == order.m_sTargetZoneId
			|| IsZeroVector(order.m_vSourcePosition) || IsZeroVector(order.m_vTargetPosition))
			return "exact enemy patrol order identity is invalid";
		if (order.m_iSupportCost != 0 || order.m_iAttackCost < 0)
			return "exact enemy patrol must use the proactive attack ledger only";
		if (!ValidatePreparedManifest(order, manifest))
			return "exact enemy patrol frozen manifest is invalid";
		ref array<vector> positions = HST_OperationRouteCursorService.BuildOrderedRoutePositions(route);
		if (positions.Count() < 2
			|| HST_OperationRouteCursorService.BuildRouteContractHash(route, positions).IsEmpty()
			|| route.m_sRouteId.IsEmpty()
			|| !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
				route.m_sTargetZoneId, order.m_sTargetZoneId)
			|| state.FindGeneratedRoute(route.m_sRouteId) != route)
			return "exact enemy patrol generated route contract is invalid";
		if (CountGeneratedRouteId(state, route.m_sRouteId) != 1)
			return "exact enemy patrol generated route identity is ambiguous";
		foreach (HST_EnemyOrderState other : state.m_aEnemyOrders)
		{
			if (!other || other == order)
				continue;
			bool open = other.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				|| other.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
			if (other.m_sOperationId == order.m_sOperationId || (open
				&& IsExactEnemyPatrol(other) && other.m_sFactionKey == order.m_sFactionKey
				&& HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					other.m_sTargetZoneId, order.m_sTargetZoneId)))
				return "another exact enemy patrol already owns this identity or target";
		}
		return "";
	}

	protected string FindAdmissionIdentityCollision(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!state || !order || !manifest)
			return "exact enemy patrol admission identity context is missing";
		string orderId = order.m_sOrderId;
		string operationId = order.m_sOperationId;
		string manifestId = manifest.m_sManifestId;
		string resultId = BuildSpawnResultId(order);
		string forceId = BuildForceId(order);
		string projectionId = BuildProjectionId(order);
		foreach (HST_EnemyOrderState candidateOrder : state.m_aEnemyOrders)
		{
			if (!candidateOrder || candidateOrder == order)
				continue;
			if (candidateOrder.m_sOrderId == orderId
				|| candidateOrder.m_sOperationId == operationId
				|| candidateOrder.m_sManifestId == manifestId
				|| candidateOrder.m_sSpawnResultId == resultId
				|| candidateOrder.m_sGroupId == projectionId)
				return "exact enemy patrol admission identity is already claimed by another order";
		}
		foreach (HST_OperationRecordState candidateOperation : state.m_aOperations)
		{
			if (!candidateOperation)
				continue;
			if (candidateOperation.m_sOperationId == operationId
				|| candidateOperation.m_sEnemyOrderId == orderId
				|| candidateOperation.m_sManifestId == manifestId
				|| candidateOperation.m_sSpawnResultId == resultId
				|| candidateOperation.m_sForceId == forceId
				|| candidateOperation.m_sProjectionId == projectionId
				|| candidateOperation.m_sGroupId == projectionId)
				return "exact enemy patrol admission identity is already claimed by an operation";
		}
		foreach (HST_ForceManifestState candidateManifest : state.m_aForceManifests)
		{
			if (candidateManifest && (candidateManifest.m_sManifestId == manifestId
				|| candidateManifest.m_sOperationId == operationId))
				return "exact enemy patrol admission identity is already claimed by a manifest";
		}
		foreach (HST_ForceSpawnResultState candidateBatch : state.m_aForceSpawnResults)
		{
			if (!candidateBatch)
				continue;
			if (candidateBatch.m_sResultId == resultId
				|| candidateBatch.m_sRequestId == orderId
				|| candidateBatch.m_sOperationId == operationId
				|| candidateBatch.m_sManifestId == manifestId
				|| candidateBatch.m_sForceId == forceId
				|| candidateBatch.m_sProjectionId == projectionId)
				return "exact enemy patrol admission identity is already claimed by a spawn batch";
		}
		foreach (HST_ActiveGroupState candidateGroup : state.m_aActiveGroups)
		{
			if (!candidateGroup)
				continue;
			if (candidateGroup.m_sGroupId == projectionId
				|| candidateGroup.m_sEnemyOrderId == orderId
				|| candidateGroup.m_sOperationId == operationId
				|| candidateGroup.m_sManifestId == manifestId
				|| candidateGroup.m_sSpawnResultId == resultId
				|| candidateGroup.m_sForceId == forceId
				|| candidateGroup.m_sProjectionId == projectionId)
				return "exact enemy patrol admission identity is already claimed by an active group";
		}
		return "";
	}

	protected bool ValidatePreparedManifest(
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!order || !manifest || !manifest.m_bFrozen
			|| manifest.m_sManifestId != "manifest_" + order.m_sOperationId
			|| manifest.m_sOperationId != order.m_sOperationId
			|| manifest.m_sForceKind != EXACT_FORCE_KIND || manifest.m_sPolicyId != EXACT_POLICY_ID)
			return false;
		if (manifest.m_sIntentId != EXACT_MANIFEST_INTENT
			|| manifest.m_sFactionKey != order.m_sFactionKey
			|| manifest.m_sSourceZoneId != order.m_sSourceZoneId
			|| manifest.m_sTargetZoneId != order.m_sTargetZoneId)
			return false;
		if (manifest.m_iAttackResourceCost != order.m_iAttackCost
			|| manifest.m_iSupportResourceCost != 0
			|| manifest.m_sManifestHash.IsEmpty()
			|| m_Integrity.BuildManifestHash(manifest) != manifest.m_sManifestHash)
			return false;
		return m_StrategicMovement.IsSupportedExactInfantryManifest(manifest)
			&& manifest.m_aGroups.Count() == 1 && manifest.m_aGroups[0]
			&& manifest.m_aGroups[0].m_iExpectedMemberCount == manifest.m_iAcceptedMemberCount;
	}

	protected HST_ForceSpawnQueueRequest BuildSpawnRequest(
		HST_CampaignState state,
		HST_EnemyOrderState order)
	{
		HST_ForceSpawnQueueRequest request = new HST_ForceSpawnQueueRequest();
		if (!state || !order)
			return request;
		request.m_sResultId = BuildSpawnResultId(order);
		request.m_sRequestId = order.m_sOrderId;
		request.m_sForceId = BuildForceId(order);
		request.m_sProjectionId = BuildProjectionId(order);
		request.m_iPriority = EXACT_PRIORITY;
		request.m_iMaxRetries = EXACT_MAX_RETRIES;
		request.m_iDeadlineSecond = state.m_iElapsedSeconds + DEPLOYMENT_GRACE_SECONDS;
		return request;
	}

	protected void ApplyManifestComposition(
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!order || !manifest)
			return;
		order.m_sManifestId = manifest.m_sManifestId;
		order.m_sManifestHash = manifest.m_sManifestHash;
		order.m_iCompositionManpower = manifest.m_iAcceptedMemberCount;
		order.m_iCompositionVehicleCount = 0;
		order.m_iCompositionArmedVehicleCount = 0;
		order.m_sCompositionIntentId = manifest.m_sIntentId;
		order.m_sCompositionTier = "exact";
		order.m_sCompositionSummary = string.Format("%1 exact patrol infantry", manifest.m_iAcceptedMemberCount);
		order.m_iResolveAtSecond = 0;
	}

	protected HST_OperationRecordState BuildOperation(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!state || !order || !manifest)
			return null;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		HST_OperationRecordState operation = new HST_OperationRecordState();
		operation.m_sOperationId = order.m_sOperationId;
		operation.m_eType = HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL;
		operation.m_iContractVersion = EXACT_CONTRACT_VERSION;
		operation.m_sOwnerFactionKey = order.m_sFactionKey;
		operation.m_sEnemyOrderId = order.m_sOrderId;
		operation.m_sManifestId = manifest.m_sManifestId;
		operation.m_sOriginZoneId = order.m_sSourceZoneId;
		operation.m_vOriginPosition = order.m_vSourcePosition;
		operation.m_sAssignmentKind = ASSIGNMENT_KIND;
		operation.m_sAssignmentZoneId = order.m_sTargetZoneId;
		operation.m_vAssignmentPosition = order.m_vTargetPosition;
		operation.m_sTacticalTargetZoneId = order.m_sTargetZoneId;
		operation.m_vTacticalTargetPosition = order.m_vTargetPosition;
		operation.m_vStrategicPosition = order.m_vSourcePosition;
		operation.m_sRecallPolicyId = RECALL_POLICY_ID;
		operation.m_sSettlementPolicyId = SETTLEMENT_POLICY_ID;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING;
		operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		operation.m_eTerminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE;
		operation.m_iDeterministicSeed = manifest.m_iDeterministicSeed;
		operation.m_iLastVirtualFriendlyCount = manifest.m_iAcceptedMemberCount;
		operation.m_iCreatedAtSecond = nowSecond;
		operation.m_iDutyStateEnteredAtSecond = nowSecond;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iLastProgressAtSecond = nowSecond;
		return operation;
	}

	protected HST_ActiveGroupState BuildActiveGroup(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!state || !order || !manifest || manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
			return null;
		string projectionId = BuildProjectionId(order);
		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = projectionId;
		group.m_sOperationId = order.m_sOperationId;
		group.m_sEnemyOrderId = order.m_sOrderId;
		group.m_sManifestId = manifest.m_sManifestId;
		group.m_sSpawnResultId = BuildSpawnResultId(order);
		group.m_sForceId = BuildForceId(order);
		group.m_sProjectionId = projectionId;
		group.m_sZoneId = order.m_sTargetZoneId;
		group.m_sFactionKey = order.m_sFactionKey;
		group.m_sPrefab = manifest.m_aGroups[0].m_sPrefab;
		group.m_sCompositionRequestId = manifest.m_sManifestId;
		group.m_sCompositionIntentId = manifest.m_sIntentId;
		group.m_sCompositionTier = "exact";
		group.m_sCompositionSummary = string.Format("%1 exact patrol infantry", manifest.m_iAcceptedMemberCount);
		group.m_sSpawnFallbackMode = EXACT_GROUP_MODE;
		group.m_vSourcePosition = order.m_vSourcePosition;
		group.m_vTargetPosition = order.m_vTargetPosition;
		group.m_vPosition = order.m_vSourcePosition;
		group.m_sRuntimeStatus = "enemy_patrol_virtual";
		group.m_iInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iOriginalInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iCompositionCost = manifest.m_iAttackResourceCost;
		group.m_iCompositionManpower = manifest.m_iAcceptedMemberCount;
		group.m_iLastSeenAliveCount = manifest.m_iAcceptedMemberCount;
		group.m_iSurvivorInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iDurableLivingInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iSpawnedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		group.m_bQRF = false;
		return group;
	}

	protected void LinkCommittedAdmission(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!state || !order || !operation || !batch || !group)
			return;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_sSpawnResultId = batch.m_sResultId;
		operation.m_sForceId = batch.m_sForceId;
		operation.m_sProjectionId = batch.m_sProjectionId;
		operation.m_sGroupId = group.m_sGroupId;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		operation.m_eResumeDutyState = operation.m_eDutyState;
		operation.m_iDutyStateEnteredAtSecond = nowSecond;
		operation.m_iLastProgressAtSecond = nowSecond;
		operation.m_iRevision++;
		order.m_sSpawnResultId = batch.m_sResultId;
		order.m_sGroupId = group.m_sGroupId;
		order.m_bStrategicServiceCommitted = true;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
		order.m_sRuntimeStatus = "exact_patrol_virtual_outbound";
		order.m_sFailureReason = "";
	}

	protected bool HasCommittedAdmissionAuthority(
		HST_CampaignState state,
		HST_EnemyOrderState order)
	{
		if (!order)
			return false;
		if (order.m_bStrategicServiceCommitted || !order.m_sSpawnResultId.IsEmpty()
			|| !order.m_sGroupId.IsEmpty())
			return true;
		if (!state || order.m_sOperationId.IsEmpty())
			return false;
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		return operation && (!operation.m_sSpawnResultId.IsEmpty() || !operation.m_sForceId.IsEmpty()
			|| !operation.m_sProjectionId.IsEmpty() || !operation.m_sGroupId.IsEmpty());
	}

	protected HST_EnemyPatrolAdmissionResult ResolveCommittedAdmissionReplay(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState suppliedManifest,
		HST_GeneratedRouteState suppliedRoute)
	{
		HST_EnemyPatrolAdmissionResult result = new HST_EnemyPatrolAdmissionResult();
		if (!state || !order || !suppliedManifest || !suppliedRoute)
		{
			result.m_sFailureReason = "exact enemy patrol committed replay context is incomplete";
			return result;
		}
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		string failure = ResolveRuntimeContext(state, order, operation, manifest, batch, group);
		HST_GeneratedRouteState frozenRoute;
		if (operation)
			frozenRoute = state.FindGeneratedRoute(operation.m_sCurrentRouteId);
		if (!failure.IsEmpty() || manifest != state.FindForceManifest(suppliedManifest.m_sManifestId)
			|| suppliedManifest.m_sManifestHash != manifest.m_sManifestHash
			|| m_Integrity.BuildManifestHash(suppliedManifest) != suppliedManifest.m_sManifestHash
			|| suppliedRoute != frozenRoute || CountGeneratedRouteId(state, suppliedRoute.m_sRouteId) != 1
			|| !m_RouteCursor.IsPatrolRouteContractValid(operation, suppliedRoute)
			|| order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			|| !order.m_bStrategicServiceCommitted || order.m_bResourceSettlementApplied
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			result.m_sFailureReason = "exact enemy patrol committed replay authority conflicts";
			if (!failure.IsEmpty())
				result.m_sFailureReason = result.m_sFailureReason + ": " + failure;
			return result;
		}
		result.m_bSuccess = true;
		result.m_Operation = operation;
		result.m_Batch = batch;
		result.m_Group = group;
		return result;
	}

	bool QuarantineUnsupportedPatrolAuthority(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		string reason)
	{
		if (!state || !order)
			return false;
		if (order.m_iOperationContractVersion == QUARANTINED_CONTRACT_VERSION
			&& order.m_sRuntimeStatus == "exact_patrol_quarantined"
			&& !order.m_sFailureReason.IsEmpty())
		{
			return QuarantineRuntimeAuthority(state, order, order.m_sFailureReason);
		}
		return QuarantineRuntimeAuthority(state, order, "unsupported patrol dispatch: " + reason);
	}

	bool PrepareOpenPhysicalAuthorityForSettlement(HST_CampaignState state, out string failure)
	{
		failure = "";
		if (!state || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar)
		{
			failure = "exact enemy patrol settlement reconciliation services are unavailable";
			return false;
		}
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL
				|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				|| (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
					&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING))
				continue;
			HST_EnemyOrderState order = state.FindEnemyOrder(operation.m_sEnemyOrderId);
			if (!order || !IsExactEnemyPatrol(order)
				|| order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				continue;
			HST_ForceSpawnResultState batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
			HST_ActiveGroupState group = state.FindActiveGroup(operation.m_sGroupId);
			if (!batch || !group || batch.m_sProjectionId != operation.m_sProjectionId
				|| group.m_sProjectionId != operation.m_sProjectionId)
			{
				failure = "exact enemy patrol settlement runtime graph is incomplete " + operation.m_sOperationId;
				return false;
			}
			HST_ForceSpawnAdapterTickResult reconciled = m_SpawnAdapter.ReconcileExactInfantryProjectionAuthority(
				state,
				m_SpawnQueue,
				m_PhysicalWar,
				Math.Max(0, state.m_iElapsedSeconds),
				operation.m_sProjectionId);
			if (!reconciled || reconciled.m_iFailedCount > 0)
			{
				failure = "exact enemy patrol settlement roster reconciliation failed " + operation.m_sOperationId;
				if (reconciled && !reconciled.m_sSummary.IsEmpty())
					failure = failure + ": " + reconciled.m_sSummary;
				return false;
			}
			int living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
			if (living <= 0)
				continue;
			string bindingFailure;
			if (!m_SpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
				state,
				batch,
				m_SpawnQueue,
				m_PhysicalWar,
				bindingFailure))
			{
				failure = "exact enemy patrol settlement live bindings are incomplete " + operation.m_sOperationId + ": " + bindingFailure;
				return false;
			}
		}
		return true;
	}

	bool TickOrder(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order)
	{
		if (!IsExactEnemyPatrol(order) || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return false;
		if (!state || !preset || !enemyDirector || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar)
			return SetRuntimeConflict(order, "exact enemy patrol runtime services are unavailable");
		string ambiguity = FindAmbiguousAuthorityRows(state, order);
		if (!ambiguity.IsEmpty())
			return QuarantineRuntimeAuthority(state, order, ambiguity);
		HST_OperationRecordState settledOperation = state.FindOperation(order.m_sOperationId);
		if (settledOperation
			&& settledOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			return FinalizeSettledOrder(
				state,
				order,
				settledOperation,
				FindLinkedBatch(state, order),
				FindLinkedGroup(state, order));
		}

		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		string failure = ResolveRuntimeContext(state, order, operation, manifest, batch, group);
		if (!failure.IsEmpty())
			return QuarantineRuntimeAuthority(state, order, failure);
		HST_GeneratedRouteState route = ResolvePatrolRoute(state, order);
		if (!route || !m_RouteCursor.IsPatrolRouteContractValid(operation, route))
			return QuarantineRuntimeAuthority(state, order,
				"exact enemy patrol frozen route authority is invalid");

		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
			return ContinueDematerialization(state, enemyDirector, order, operation, manifest, batch, group,
				operation.m_sLastProjectionReason);
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL)
			return TickVirtual(state, enemyDirector, order, operation, manifest, route, batch, group);
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
			return TickMaterializing(state, enemyDirector, order, operation, manifest, batch, group);
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			return TickPhysical(state, enemyDirector, order, operation, manifest, route, batch, group);
		return FailClosedActiveOrder(state, enemyDirector, order, operation, manifest, batch, group,
			"exact enemy patrol materialization authority is invalid");
	}

	bool ReconcileAfterRestore(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector)
	{
		if (!state || !enemyDirector)
			return false;
		bool changed;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsExactEnemyPatrol(order))
				continue;
			string ambiguity = FindAmbiguousAuthorityRows(state, order);
			if (!ambiguity.IsEmpty())
			{
				changed = QuarantineRuntimeAuthority(state, order, ambiguity) || changed;
				continue;
			}
			HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
			HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
			HST_ForceSpawnResultState batch = FindLinkedBatch(state, order);
			HST_ActiveGroupState group = FindLinkedGroup(state, order);
			if (operation && operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			{
				changed = FinalizeSettledOrder(state, order, operation, batch, group) || changed;
				continue;
			}
			if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			{
				changed = QuarantineRuntimeAuthority(state, order,
					"open exact enemy patrol restore has a terminal order status") || changed;
				continue;
			}
			if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				&& !HasCommittedAdmissionAuthority(state, order))
			{
				changed = SettleInterruptedAdmission(state, enemyDirector, order, operation, manifest,
					"restore found an interrupted exact enemy patrol admission") || changed;
				continue;
			}
			HST_OperationRecordState resolvedOperation;
			HST_ForceManifestState resolvedManifest;
			HST_ForceSpawnResultState resolvedBatch;
			HST_ActiveGroupState resolvedGroup;
			string failure = ResolveRuntimeContext(
				state,
				order,
				resolvedOperation,
				resolvedManifest,
				resolvedBatch,
				resolvedGroup);
			if (!failure.IsEmpty())
			{
				changed = QuarantineRuntimeAuthority(state, order, failure) || changed;
				continue;
			}
			changed = NormalizeRestoredOpenRuntime(
				state,
				order,
				resolvedOperation,
				resolvedManifest,
				resolvedBatch,
				resolvedGroup) || changed;
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
			if (order && order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL
				&& order.m_iOperationContractVersion == EXACT_CONTRACT_VERSION)
			{
				HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
				if (operation && operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
					changed = FinalizeSettledOrder(state, order, operation,
						FindLinkedBatch(state, order),
						FindLinkedGroup(state, order)) || changed;
			}
		}
		return changed;
	}

	bool SettleOpenOrdersForCampaignStop(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		string reason)
	{
		if (!state || !enemyDirector)
			return false;
		if (reason.IsEmpty())
			reason = "campaign phase no longer permits an active enemy patrol";
		bool changed;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsExactEnemyPatrol(order)
				|| (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
					&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED))
				continue;
			if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				&& !HasCommittedAdmissionAuthority(state, order))
			{
				changed = SettleInterruptedAdmission(
					state,
					enemyDirector,
					order,
					state.FindOperation(order.m_sOperationId),
					state.FindForceManifest(order.m_sManifestId),
					reason) || changed;
				continue;
			}
			HST_OperationRecordState operation;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			string failure = ResolveRuntimeContext(state, order, operation, manifest, batch, group);
			if (!failure.IsEmpty())
				changed = QuarantineRuntimeAuthority(state, order, reason + ": " + failure) || changed;
			else
				changed = FailClosedActiveOrder(state, enemyDirector, order, operation, manifest, batch, group, reason) || changed;
		}
		return changed;
	}

	protected bool TickVirtual(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_GeneratedRouteState route,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
			return SettleOperation(state, enemyDirector, order, operation, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"virtual_projection_failed_survivors", ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy patrol held projection failed");
		if (!batch.m_bStrategicProjectionHeld)
			return SettleOperation(state, enemyDirector, order, operation, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"invalidated_survivors", ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy patrol virtual batch is not strategically held");

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
				"exact enemy patrol virtual roster eliminated");
			return SettleOperation(state, enemyDirector, order, operation, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
				"destroyed", 0, "exact enemy patrol roster was eliminated");
		}

		HST_OperationRouteCursorResult movement = m_RouteCursor.AdvanceVirtualLeg(state, operation, group);
		if (!movement || !movement.m_bAccepted)
			return SettleOperation(state, enemyDirector, order, operation, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
				"route_failed_survivors", living, "exact enemy patrol strategic movement was rejected");
		bool changed = movement.m_bStateChanged;
		if (movement.m_bArrived)
		{
			if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
				return SettleOperation(state, enemyDirector, order, operation, manifest, batch, group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED,
					"returned_survivors", living, "exact enemy patrol returned to its origin") || changed;
			if (!AdvancePatrolLifecycleAtArrival(state, order, operation, route, group, false))
				return SettleOperation(state, enemyDirector, order, operation, manifest, batch, group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
					"route_failed_survivors", living, "exact enemy patrol loop transition failed") || changed;
			changed = true;
		}

		HST_OperationProjectionDecision decision = m_Materialization.EvaluateExactPlayerQRF(
			operation,
			operation.m_vStrategicPosition);
		operation.m_sLastProjectionReason = decision.m_sReason;
		if (decision.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE)
			return BeginMaterialization(state, enemyDirector, order, operation, manifest, batch, group,
				decision.m_sReason) || changed;
		ApplyVirtualRuntimeStatus(order, operation, group);
		return changed;
	}

	protected bool AdvancePatrolLifecycleAtArrival(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_GeneratedRouteState route,
		HST_ActiveGroupState group,
		bool physical)
	{
		if (!state || !order || !operation || !route || !group)
			return false;
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
		{
			SetDutyState(state, operation, HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION);
			if (!m_RouteCursor.StartPatrolLoop(state, operation, route, group))
				return false;
		}
		else if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
		{
			if (operation.m_iRouteWaypointIndex == 0
				&& operation.m_iRouteLapCount >= REQUIRED_PATROL_LAPS)
			{
				operation.m_iRouteLoopCompletedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
				SetDutyState(state, operation, HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN);
				if (!m_RouteCursor.BeginReturnLeg(state, operation, group))
					return false;
			}
			else
			{
				HST_OperationRouteCursorResult advanced = m_RouteCursor.AdvanceLoopAfterArrival(
					state,
					operation,
					route,
					group,
					REQUIRED_PATROL_LAPS + 1);
				if (!advanced || !advanced.m_bAccepted)
					return false;
			}
		}
		else
			return false;

		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		ApplyVirtualRuntimeStatus(order, operation, group);
		if (!physical)
			return true;
		return m_PhysicalWar.RestartExactEnemyPatrolInfantryRoute(
			state,
			group,
			operation.m_vRouteEndPosition,
			"Exact enemy patrol following its frozen route cursor.");
	}

	protected void SetDutyState(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_EOperationDutyState dutyState)
	{
		if (!state || !operation)
			return;
		operation.m_eDutyState = dutyState;
		operation.m_eResumeDutyState = dutyState;
		operation.m_iDutyStateEnteredAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iLastProgressAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iRevision++;
	}

	protected void ApplyVirtualRuntimeStatus(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group)
	{
		if (!order || !operation || !group)
			return;
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
		{
			order.m_sRuntimeStatus = "exact_patrol_virtual_returning";
			group.m_sRuntimeStatus = "enemy_patrol_virtual_returning";
		}
		else if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
		{
			order.m_sRuntimeStatus = "exact_patrol_virtual_loop";
			group.m_sRuntimeStatus = "enemy_patrol_virtual_loop";
		}
		else
		{
			order.m_sRuntimeStatus = "exact_patrol_virtual_outbound";
			group.m_sRuntimeStatus = "enemy_patrol_virtual";
		}
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
		int deadlineSecond = state.m_iElapsedSeconds + DEPLOYMENT_GRACE_SECONDS;
		HST_ForceSpawnQueueCallbackResult released = m_SpawnQueue.ReleaseStrategicProjectionForMaterialization(
			state.m_aForceSpawnResults,
			manifest,
			batch.m_sResultId,
			batch.m_sProjectionId,
			state.m_iElapsedSeconds,
			deadlineSecond);
		if (!released || !released.m_bAccepted)
			return SettleOperation(state, enemyDirector, order, operation, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"invalidated_survivors", ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy patrol materialization release failed");

		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_iMaterializationStateEnteredAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		group.m_vPosition = operation.m_vStrategicPosition;
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		group.m_vTargetPosition = operation.m_vRouteEndPosition;
		group.m_sRuntimeStatus = "enemy_patrol_materializing";
		group.m_iLifecycleRevision++;
		order.m_sRuntimeStatus = "exact_patrol_materializing";
		return true;
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
			return SettleOperation(state, enemyDirector, order, operation, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"materialization_failed_survivors", ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy patrol materialization failed");
		}
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| !group.m_bSpawnedEntity)
			return false;

		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iMaterializationStateEnteredAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iEngagementStateEnteredAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		operation.m_iRevision++;
		if (!m_PhysicalWar.RestartExactEnemyPatrolInfantryRoute(
			state,
			group,
			operation.m_vRouteEndPosition,
			"Exact enemy patrol materialized on its frozen current leg."))
		{
			return RetireAndSettlePhysicalFailure(state, enemyDirector, order, operation, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
				"route_failed_survivors", ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy patrol live route could not be issued");
		}
		order.m_bPhysicalized = true;
		order.m_sRuntimeStatus = "exact_patrol_physical";
		return true;
	}

	protected bool TickPhysical(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_GeneratedRouteState route,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			return SettlePhysicalFailure(state, enemyDirector, order, operation, manifest, batch, group,
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy patrol physical batch is not successful");
		if (group.m_sRuntimeStatus.Contains("runtime_binding_missing"))
			return SetRuntimeConflict(order,
				"exact enemy patrol physical runtime binding disappeared without casualty proof");
		int living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
		if (living > 0 || group.m_sRuntimeStatus != "eliminated")
		{
			string bindingFailure;
			if (!m_SpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
				state,
				batch,
				m_SpawnQueue,
				m_PhysicalWar,
				bindingFailure))
			{
				return SetRuntimeConflict(order,
					"exact enemy patrol physical binding graph is unresolved: " + bindingFailure);
			}
		}
		if (living > 0 && (!group.m_bSpawnedEntity || group.m_sRuntimeStatus == "spawn_failed"))
			return SetRuntimeConflict(order,
				"exact enemy patrol physical runtime projection is missing despite a durable living roster");
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
				return SetRuntimeConflict(order, "exact enemy patrol elimination cleanup is pending: " + eliminationReason);
			return SettleOperation(state, enemyDirector, order, operation, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
				"destroyed", 0, "exact enemy patrol was eliminated");
		}

		vector livePosition;
		string liveEvidence;
		if (!m_PhysicalWar.TryResolveExactEnemyPatrolLivePosition(state, group, livePosition, liveEvidence))
			return SettlePhysicalFailure(state, enemyDirector, order, operation, manifest, batch, group,
				ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy patrol live position is unavailable: " + liveEvidence);
		operation.m_vStrategicPosition = livePosition;
		operation.m_iLastProgressAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iRevision++;
		bool changed = true;

		bool contactActive;
		bool contactCleared;
		changed = UpdatePhysicalContactState(state, operation, group, contactActive, contactCleared) || changed;
		if (contactActive)
		{
			operation.m_iArrivalConfirmationCount = 0;
			operation.m_iLastArrivalConfirmationSecond = 0;
			order.m_sRuntimeStatus = "exact_patrol_contact_hold";
			return changed;
		}
		if (contactCleared)
		{
			if (!m_PhysicalWar.RestartExactEnemyPatrolInfantryRoute(
				state,
				group,
				operation.m_vRouteEndPosition,
				"Exact enemy patrol resumed its frozen leg after contact cleared."))
			{
				return RetireAndSettlePhysicalFailure(state, enemyDirector, order, operation, manifest, batch, group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
					"route_failed_survivors", living,
					"exact enemy patrol could not resume its live route after contact");
			}
			order.m_sRuntimeStatus = "exact_patrol_physical";
			return true;
		}

		if (m_PhysicalWar.IsExactEnemyPatrolRouteRecoveryExhausted(state, group, state.m_iElapsedSeconds))
		{
			return RetireAndSettlePhysicalFailure(state, enemyDirector, order, operation, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
				"route_failed_survivors", living,
				"exact enemy patrol exhausted bounded live-route recovery");
		}

		bool arrived;
		changed = ConfirmPhysicalArrivalSample(state, operation, group, arrived) || changed;
		if (arrived)
		{
			if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
				return TryDematerialize(state, enemyDirector, order, operation, manifest, batch, group,
					"return arrival folded for exact patrol survivor settlement") || changed;
			if (!AdvancePatrolLifecycleAtArrival(state, order, operation, route, group, true))
			{
				return RetireAndSettlePhysicalFailure(state, enemyDirector, order, operation, manifest, batch, group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
					"route_failed_survivors", living,
					"exact enemy patrol physical loop transition failed") || changed;
			}
			order.m_sRuntimeStatus = "exact_patrol_physical";
			return true;
		}

		HST_OperationProjectionDecision decision = m_Materialization.EvaluateExactPlayerQRF(
			operation,
			livePosition);
		operation.m_sLastProjectionReason = decision.m_sReason;
		if (decision.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_DEMATERIALIZE)
			return TryDematerialize(state, enemyDirector, order, operation, manifest, batch, group,
				decision.m_sReason) || changed;
		order.m_sRuntimeStatus = "exact_patrol_physical";
		return changed;
	}

	protected bool UpdatePhysicalContactState(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		out bool contactActive,
		out bool contactCleared)
	{
		contactActive = false;
		contactCleared = false;
		if (!state || !operation || !group)
			return false;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		string evidence;
		bool proximityContact = m_PhysicalWar.HasExactEnemyPatrolLiveContactEvidence(
			state,
			group,
			CONTACT_RADIUS_METERS,
			evidence);
		bool casualtyContact = group.m_iLastCasualtySecond > 0
			&& nowSecond - group.m_iLastCasualtySecond <= CONTACT_CLEAR_SECONDS;
		operation.m_iStrategicLastUpdateSecond = nowSecond;
		if (proximityContact || casualtyContact)
		{
			contactActive = true;
			bool changed = operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT
				|| operation.m_iLastContactAtSecond != nowSecond
				|| group.m_sRuntimeStatus != "enemy_patrol_contact_hold";
			operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT;
			operation.m_iEngagementStateEnteredAtSecond = nowSecond;
			operation.m_iLastContactAtSecond = nowSecond;
			operation.m_sLastProjectionReason = evidence;
			group.m_sRuntimeStatus = "enemy_patrol_contact_hold";
			if (changed)
			{
				operation.m_iRevision++;
				group.m_iLifecycleRevision++;
			}
			return changed;
		}
		if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
			return false;
		if (nowSecond - operation.m_iLastContactAtSecond < CONTACT_CLEAR_SECONDS)
		{
			contactActive = true;
			group.m_sRuntimeStatus = "enemy_patrol_contact_hold";
			return false;
		}
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_sLastProjectionReason = "exact enemy patrol contact cleared";
		operation.m_iRevision++;
		contactCleared = true;
		return true;
	}

	protected bool ConfirmPhysicalArrivalSample(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		out bool arrived)
	{
		arrived = false;
		if (!state || !operation || !group)
			return false;
		if (Distance2D(group.m_vPosition, operation.m_vRouteEndPosition) > PHYSICAL_ARRIVAL_RADIUS_METERS)
		{
			if (operation.m_iArrivalConfirmationCount == 0 && operation.m_iLastArrivalConfirmationSecond == 0)
				return false;
			operation.m_iArrivalConfirmationCount = 0;
			operation.m_iLastArrivalConfirmationSecond = 0;
			operation.m_iRevision++;
			return true;
		}
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		if (operation.m_iArrivalConfirmationCount > 0
			&& operation.m_iLastArrivalConfirmationSecond == nowSecond)
		{
			arrived = operation.m_iArrivalConfirmationCount >= 2;
			return false;
		}
		operation.m_iArrivalConfirmationCount++;
		operation.m_iLastArrivalConfirmationSecond = nowSecond;
		operation.m_iRevision++;
		arrived = operation.m_iArrivalConfirmationCount >= 2;
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
		int deadlineSecond = state.m_iElapsedSeconds + DEPLOYMENT_GRACE_SECONDS;
		HST_ForceSpawnQueueCallbackResult preflight = m_SpawnQueue.CanRequeueSuccessfulProjectionForStrategicHold(
			state.m_aForceSpawnResults,
			manifest,
			batch.m_sResultId,
			batch.m_sProjectionId,
			state.m_iElapsedSeconds,
			deadlineSecond);
		if (!preflight || !preflight.m_bAccepted)
			return SetRuntimeConflict(order, "exact enemy patrol dematerialization is waiting for queue capacity");
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		operation.m_iMaterializationStateEnteredAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		order.m_sRuntimeStatus = "exact_patrol_dematerializing";
		return ContinueDematerialization(state, enemyDirector, order, operation, manifest, batch, group, reason);
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
		vector foldedPosition = group.m_vPosition;
		if (IsZeroVector(foldedPosition))
			foldedPosition = operation.m_vStrategicPosition;
		if (!batch.m_bStrategicProjectionHeld)
		{
			HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
				state,
				m_PhysicalWar,
				group.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return SetRuntimeConflict(order, "exact enemy patrol runtime retirement is pending");
			int deadlineSecond = state.m_iElapsedSeconds + DEPLOYMENT_GRACE_SECONDS;
			HST_ForceSpawnQueueCallbackResult held = m_SpawnQueue.RequeueSuccessfulProjectionForStrategicHold(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				state.m_iElapsedSeconds,
				deadlineSecond);
			if (!held || !held.m_bAccepted)
				return SetRuntimeConflict(order, "exact enemy patrol survivor projection could not enter strategic hold");
		}

		if (!m_RouteCursor.SyncLegFromPosition(state, operation, group, foldedPosition))
			return SettleOperation(state, enemyDirector, order, operation, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
				"route_failed_survivors", ResolveSettlementSurvivors(operation, manifest, batch, group),
				"exact enemy patrol physical position could not fold into its route cursor");
		group.m_bSpawnedEntity = false;
		group.m_bSpawnAttempted = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
		SyncGroupRoster(group, m_SpawnQueue.CountStrategicLivingMemberSlots(batch));
		group.m_iLifecycleRevision++;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iMaterializationStateEnteredAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iEngagementStateEnteredAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_vStrategicPosition = foldedPosition;
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		order.m_bPhysicalized = false;
		ApplyVirtualRuntimeStatus(order, operation, group);
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
		if (!state || !enemyDirector || !order || !operation || !manifest || settlementKind.IsEmpty())
			return false;
		string settlementId = HST_OperationService.BuildSettlementId(operation.m_sOperationId, settlementKind);
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (operation.m_sSettlementId != settlementId || operation.m_eTerminalResult != terminalResult
				|| !ValidateAppliedResourceSettlement(order, manifest, settlementKind, survivors))
				return SetRuntimeConflict(order, "exact enemy patrol settled receipt conflicts");
			return FinalizeSettledOrder(state, order, operation, batch, group);
		}
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION)
			return SetRuntimeConflict(order, "exact enemy patrol settlement authority is invalid");
		if (!ApplyResourceSettlement(state, enemyDirector, order, manifest, settlementKind, survivors,
			settlementKind.Contains("_full"), reason))
			return SetRuntimeConflict(order, "exact enemy patrol proactive resource settlement conflicted");

		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		operation.m_eTerminalResult = terminalResult;
		operation.m_sSettlementId = settlementId;
		operation.m_sTerminalReason = reason;
		operation.m_iDutyStateEnteredAtSecond = nowSecond;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iLastProgressAtSecond = nowSecond;
		operation.m_iSettledAtSecond = nowSecond;
		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		operation.m_iRevision++;
		return FinalizeSettledOrder(state, order, operation, batch, group);
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
		if (!state || !enemyDirector || !order || !manifest || settlementKind.IsEmpty())
			return false;
		int accepted = Math.Max(0, manifest.m_iAcceptedMemberCount);
		if (accepted <= 0 || order.m_iSupportCost != 0 || manifest.m_iSupportResourceCost != 0)
			return false;
		int survivors = Math.Max(0, Math.Min(accepted, survivorCount));
		int attackRefund = Math.Max(0, order.m_iAttackCost) * survivors / accepted;
		if (fullRefund)
			attackRefund = Math.Max(0, order.m_iAttackCost);
		string settlementId = HST_OperationService.BuildSettlementId(order.m_sOperationId, settlementKind);
		if (order.m_bResourceSettlementApplied)
			return ValidateAppliedResourceSettlement(order, manifest, settlementKind, survivors);
		if (HasPartialResourceSettlementAuthority(order) || order.m_bResourceRefundApplied)
			return false;
		if (attackRefund > 0 && !state.FindFactionPool(order.m_sFactionKey))
			return false;

		order.m_sResourceSettlementId = settlementId;
		order.m_sResourceSettlementKind = settlementKind;
		order.m_iSettlementAcceptedMemberCount = accepted;
		order.m_iSettlementSurvivorMemberCount = survivors;
		order.m_iRefundedAttackResources = attackRefund;
		order.m_iRefundedSupportResources = 0;
		order.m_bResourceSettlementApplied = true;
		if (!enemyDirector.RefundProactiveAttackResources(
			state,
			order.m_sFactionKey,
			attackRefund,
			reason))
		{
			ClearResourceSettlementReceipt(order);
			return false;
		}
		return true;
	}

	protected bool ValidateAppliedResourceSettlement(
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		string settlementKind,
		int survivorCount)
	{
		if (!order || !manifest || !order.m_bResourceSettlementApplied
			|| order.m_sResourceSettlementKind != settlementKind)
			return false;
		int accepted = Math.Max(0, manifest.m_iAcceptedMemberCount);
		int survivors = Math.Max(0, Math.Min(accepted, survivorCount));
		int expectedRefund = Math.Max(0, order.m_iAttackCost) * survivors / Math.Max(1, accepted);
		if (settlementKind.Contains("_full"))
			expectedRefund = Math.Max(0, order.m_iAttackCost);
		return accepted > 0
			&& order.m_sResourceSettlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, settlementKind)
			&& order.m_iSettlementAcceptedMemberCount == accepted
			&& order.m_iSettlementSurvivorMemberCount == survivors
			&& order.m_iRefundedAttackResources == expectedRefund
			&& order.m_iRefundedSupportResources == 0
			&& !order.m_bResourceRefundApplied;
	}

	protected bool HasPartialResourceSettlementAuthority(HST_EnemyOrderState order)
	{
		if (!order)
			return false;
		return !order.m_sResourceSettlementId.IsEmpty() || !order.m_sResourceSettlementKind.IsEmpty()
			|| order.m_iSettlementAcceptedMemberCount != 0 || order.m_iSettlementSurvivorMemberCount != 0
			|| order.m_iRefundedAttackResources != 0 || order.m_iRefundedSupportResources != 0;
	}

	protected void ClearResourceSettlementReceipt(HST_EnemyOrderState order)
	{
		if (!order)
			return;
		order.m_sResourceSettlementId = "";
		order.m_sResourceSettlementKind = "";
		order.m_iSettlementAcceptedMemberCount = 0;
		order.m_iSettlementSurvivorMemberCount = 0;
		order.m_iRefundedAttackResources = 0;
		order.m_iRefundedSupportResources = 0;
		order.m_bResourceSettlementApplied = false;
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
		if (order.m_iResolvedAtSecond != operation.m_iSettledAtSecond)
		{
			order.m_iResolvedAtSecond = Math.Max(0, operation.m_iSettledAtSecond);
			changed = true;
		}
		if (order.m_bPhysicalized || order.m_bAbstractResolved)
		{
			order.m_bPhysicalized = false;
			order.m_bAbstractResolved = false;
			changed = true;
		}
		if (!order.m_bOutcomeApplied)
		{
			order.m_bOutcomeApplied = true;
			changed = true;
		}
		string runtimeStatus = "resolved_exact_patrol_terminal";
		if (operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED)
			runtimeStatus = "resolved_exact_patrol_returned";
		else if (operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED)
			runtimeStatus = "resolved_exact_patrol_destroyed";
		if (order.m_sRuntimeStatus != runtimeStatus)
		{
			order.m_sRuntimeStatus = runtimeStatus;
			changed = true;
		}
		if (order.m_sResolutionKind != order.m_sResourceSettlementKind)
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
		if (runtimeReleased && batch)
		{
			int batchIndex = state.m_aForceSpawnResults.Find(batch);
			if (batchIndex >= 0)
			{
				state.m_aForceSpawnResults.Remove(batchIndex);
				changed = true;
			}
		}
		if (runtimeReleased && group)
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
		if (operation && manifest && batch && group
			&& (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING))
		{
			return SettlePhysicalFailure(state, enemyDirector, order, operation, manifest, batch, group,
				ResolveSettlementSurvivors(operation, manifest, batch, group), reason);
		}
		if (operation && manifest)
			return SettleOperation(state, enemyDirector, order, operation, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"invalidated_survivors", ResolveSettlementSurvivors(operation, manifest, batch, group), reason);
		return QuarantineRuntimeAuthority(state, order, reason);
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
		return RetireAndSettlePhysicalFailure(state, enemyDirector, order, operation, manifest, batch, group,
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
			"invalidated_survivors", provenSurvivors, reason);
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
				return SetRuntimeConflict(order, "exact enemy patrol fail-closed runtime retirement is pending: " + reason);
		}
		else if (handleCount > 0)
			return SetRuntimeConflict(order, "exact enemy patrol ambiguous handles cannot be retired safely: " + reason);
		group.m_bSpawnedEntity = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		operation.m_vStrategicPosition = group.m_vPosition;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		return SettleOperation(state, enemyDirector, order, operation, manifest, batch, group,
			terminalResult, settlementKind, Math.Max(0, provenSurvivors), reason);
	}

	protected void FailAdmissionAfterDebit(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_EnemyDirectorService enemyDirector,
		HST_OperationRecordState insertedOperation,
		HST_ForceSpawnResultState insertedBatch,
		HST_ActiveGroupState insertedGroup,
		string reason)
	{
		if (!state || !order || !manifest || !enemyDirector)
			return;
		int handleCount;
		if (m_SpawnAdapter)
			handleCount = m_SpawnAdapter.CountHandlesForProjection(BuildProjectionId(order));
		if ((insertedGroup && insertedGroup.m_bSpawnedEntity) || handleCount > 0
			|| (insertedBatch && insertedBatch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED))
		{
			QuarantineRuntimeAuthority(state, order,
				"committed runtime appeared during exact enemy patrol admission failure: " + reason);
			return;
		}
		if (insertedBatch && state.m_aForceSpawnResults.Find(insertedBatch) >= 0 && m_SpawnQueue)
			m_SpawnQueue.RequestCancel(state.m_aForceSpawnResults, insertedBatch.m_sResultId, state.m_iElapsedSeconds, reason);
		RemoveRuntimeRows(state, insertedBatch, insertedGroup);
		RemoveInsertedAdmissionRows(state, insertedOperation, manifest, null);

		bool refunded = ApplyResourceSettlement(state, enemyDirector, order, manifest,
			"admission_failed_full", manifest.m_iAcceptedMemberCount, true, reason);
		order.m_iOperationContractVersion = QUARANTINED_CONTRACT_VERSION;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		order.m_iResolvedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		if (refunded)
		{
			order.m_sRuntimeStatus = "exact_patrol_admission_failed_refunded";
			order.m_sResolutionKind = "exact_patrol_admission_failed_full_refund";
		}
		else
		{
			order.m_sRuntimeStatus = "exact_patrol_admission_settlement_conflict";
			order.m_sResolutionKind = "exact_patrol_admission_failed_unsettled";
		}
		order.m_sFailureReason = reason;
	}

	protected bool SettleInterruptedAdmission(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		string reason)
	{
		if (!state || !enemyDirector || !order)
			return false;
		if (operation && manifest && ValidatePreparedManifest(order, manifest))
			return SettleOperation(state, enemyDirector, order, operation, manifest, null, null,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"restore_admission_interrupted_full", manifest.m_iAcceptedMemberCount, reason);
		if (!manifest || !ValidatePreparedManifest(order, manifest))
			return QuarantineRuntimeAuthority(state, order, reason + ": frozen manifest is unavailable");
		bool refunded = ApplyResourceSettlement(state, enemyDirector, order, manifest,
			"restore_admission_interrupted_full", manifest.m_iAcceptedMemberCount, true, reason);
		if (!refunded)
			return QuarantineRuntimeAuthority(state, order, reason + ": proactive refund authority conflicted");
		order.m_iOperationContractVersion = QUARANTINED_CONTRACT_VERSION;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		order.m_iResolvedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		order.m_sRuntimeStatus = "exact_patrol_restore_admission_refunded";
		order.m_sFailureReason = reason;
		return true;
	}

	protected bool NormalizeRestoredOpenRuntime(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!state || !order || !operation || !manifest || !batch || !group)
			return false;
		bool changed;
		vector savedPosition = group.m_vPosition;
		bool wasLive = operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		if (wasLive && !IsZeroVector(savedPosition))
			changed = m_RouteCursor.SyncLegFromPosition(state, operation, group, savedPosition) || changed;

		if (!batch.m_bStrategicProjectionHeld
			&& batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
				state,
				m_PhysicalWar,
				group.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return QuarantineRuntimeAuthority(state, order, "restored exact enemy patrol runtime could not retire safely");
			HST_ForceSpawnQueueCallbackResult held = m_SpawnQueue.RequeueSuccessfulProjectionForStrategicHold(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				state.m_iElapsedSeconds,
				state.m_iElapsedSeconds + DEPLOYMENT_GRACE_SECONDS);
			if (!held || !held.m_bAccepted)
				return QuarantineRuntimeAuthority(state, order, "restored exact enemy patrol could not enter strategic hold");
			changed = true;
		}
		else if (!batch.m_bStrategicProjectionHeld
			&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
		{
			HST_ForceSpawnQueueCallbackResult heldPending = m_SpawnQueue.HoldPendingProjectionForStrategicSimulation(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				state.m_iElapsedSeconds);
			if (!heldPending || !heldPending.m_bAccepted)
				return QuarantineRuntimeAuthority(state, order, "restored exact enemy patrol pending roster could not enter strategic hold");
			changed = true;
		}

		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_iStrategicLastUpdateSecond = nowSecond;
		operation.m_iLastProgressAtSecond = nowSecond;
		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		operation.m_sLastProjectionReason = "restored exact patrol as strategic authority";
		operation.m_iRevision++;
		group.m_bSpawnedEntity = false;
		group.m_bSpawnAttempted = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
		if (batch.m_bStrategicProjectionHeld)
		{
			int living = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
			operation.m_iLastVirtualFriendlyCount = living;
			SyncGroupRoster(group, living);
		}
		ApplyVirtualRuntimeStatus(order, operation, group);
		order.m_bPhysicalized = false;
		order.m_bAbstractResolved = false;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
		return true;
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
		batch = FindLinkedBatch(state, order);
		group = FindLinkedGroup(state, order);
		if (!operation || !manifest || !batch || !group)
			return "exact enemy patrol runtime authority is incomplete";
		if (!ValidatePreparedManifest(order, manifest))
			return "exact enemy patrol runtime manifest conflicts";
		if (operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| operation.m_sOperationId != order.m_sOperationId
			|| operation.m_sEnemyOrderId != order.m_sOrderId
			|| operation.m_sManifestId != manifest.m_sManifestId
			|| operation.m_sOwnerFactionKey != order.m_sFactionKey
			|| operation.m_sOriginZoneId != order.m_sSourceZoneId
			|| operation.m_sAssignmentKind != ASSIGNMENT_KIND
			|| operation.m_sAssignmentZoneId != order.m_sTargetZoneId
			|| operation.m_sRecallPolicyId != RECALL_POLICY_ID
			|| operation.m_sSettlementPolicyId != SETTLEMENT_POLICY_ID)
			return "exact enemy patrol operation identity or policy conflicts";
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| operation.m_eDutyState != operation.m_eResumeDutyState)
			return "exact enemy patrol open lifecycle conflicts";
		bool exactGroupMode = group.m_sSpawnFallbackMode == EXACT_GROUP_MODE
			|| group.m_sSpawnFallbackMode.StartsWith(EXACT_GROUP_MODE + "_");
		if (operation.m_sSpawnResultId != batch.m_sResultId || operation.m_sGroupId != group.m_sGroupId
			|| operation.m_sForceId != batch.m_sForceId || operation.m_sProjectionId != batch.m_sProjectionId
			|| batch.m_sOperationId != operation.m_sOperationId || batch.m_sManifestId != manifest.m_sManifestId)
			return "exact enemy patrol reciprocal runtime links conflict";
		if (batch.m_sRequestId != order.m_sOrderId || batch.m_sManifestHash != manifest.m_sManifestHash
			|| group.m_sOperationId != operation.m_sOperationId || group.m_sEnemyOrderId != order.m_sOrderId)
			return "exact enemy patrol reciprocal runtime links conflict";
		if (group.m_sManifestId != manifest.m_sManifestId || group.m_sSpawnResultId != batch.m_sResultId
			|| group.m_sProjectionId != batch.m_sProjectionId || group.m_sForceId != batch.m_sForceId
			|| group.m_sGroupId != group.m_sProjectionId || group.m_sRouteId != operation.m_sCurrentRouteId
			|| !exactGroupMode || group.m_bQRF)
			return "exact enemy patrol reciprocal runtime links conflict";
		if (batch.m_aSlotResults.Count() != manifest.m_aMembers.Count() + manifest.m_aGroups.Count())
			return "exact enemy patrol durable roster slot count conflicts";
		if (order.m_bResourceSettlementApplied || order.m_bResourceRefundApplied)
			return "open exact enemy patrol contains terminal resource authority";
		return "";
	}

	protected int ResolveSettlementSurvivors(
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		int accepted;
		if (manifest)
			accepted = Math.Max(0, manifest.m_iAcceptedMemberCount);
		else if (group)
			accepted = Math.Max(0, group.m_iOriginalInfantryCount);
		int survivors;
		bool authoritativeRoster;
		if (batch && m_SpawnQueue)
		{
			if (batch.m_bStrategicProjectionHeld)
			{
				survivors = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
				authoritativeRoster = true;
			}
			else if (batch.m_iSuccessfulHandoffCount > 0)
			{
				survivors = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
				authoritativeRoster = true;
			}
		}
		if (authoritativeRoster)
			return Math.Max(0, Math.Min(accepted, survivors));
		if (group && group.m_sRuntimeStatus == "eliminated")
			return 0;
		if (operation)
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
		if (order.m_sRuntimeStatus == "exact_patrol_runtime_conflict" && order.m_sFailureReason == reason)
			return false;
		order.m_sRuntimeStatus = "exact_patrol_runtime_conflict";
		order.m_sFailureReason = reason;
		return true;
	}

	protected bool QuarantineRuntimeAuthority(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		string reason)
	{
		if (!state || !order)
			return false;
		string quarantinePrefix = "exact enemy patrol authority quarantined without guessed refund or runtime retirement: ";
		string failure = reason;
		if (!failure.StartsWith(quarantinePrefix))
			failure = quarantinePrefix + reason;
		bool changed = order.m_iOperationContractVersion != QUARANTINED_CONTRACT_VERSION
			|| order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			|| order.m_sRuntimeStatus != "exact_patrol_quarantined"
			|| order.m_sFailureReason != failure;
		order.m_iOperationContractVersion = QUARANTINED_CONTRACT_VERSION;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		order.m_bPhysicalized = false;
		order.m_sRuntimeStatus = "exact_patrol_quarantined";
		order.m_sFailureReason = failure;
		if (order.m_iResolvedAtSecond <= 0)
			order.m_iResolvedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		HST_OperationRecordState operation;
		int operationMatches;
		foreach (HST_OperationRecordState candidateOperation : state.m_aOperations)
		{
			if (!OperationClaimsPatrolOrderAuthority(candidateOperation, order))
				continue;
			operationMatches++;
			operation = candidateOperation;
		}
		if (operationMatches == 1 && operation)
		{
			operation.m_sLastProjectionReason = failure;
			operation.m_iRevision++;
		}
		else
			operation = null;
		HST_ForceSpawnResultState batch;
		int batchMatches;
		foreach (HST_ForceSpawnResultState candidateBatch : state.m_aForceSpawnResults)
		{
			if (!BatchClaimsPatrolOrderAuthority(candidateBatch, order, operation))
				continue;
			batchMatches++;
			batch = candidateBatch;
		}
		if (batchMatches == 1 && batch)
		{
			if (m_SpawnQueue)
				m_SpawnQueue.RequestCancel(state.m_aForceSpawnResults, batch.m_sResultId, state.m_iElapsedSeconds, failure);
			else
			{
				batch.m_bCancelRequested = true;
				batch.m_sLastFailureReason = failure;
			}
		}
		else
			batch = null;
		HST_ActiveGroupState group;
		int groupMatches;
		foreach (HST_ActiveGroupState candidateGroup : state.m_aActiveGroups)
		{
			if (!GroupClaimsPatrolOrderAuthority(candidateGroup, order, operation, batch))
				continue;
			groupMatches++;
			group = candidateGroup;
		}
		if (groupMatches == 1 && group)
		{
			group.m_sRuntimeStatus = "exact_patrol_quarantined";
			group.m_sSpawnFailureReason = failure;
			group.m_iLifecycleRevision++;
		}
		return changed;
	}

	protected bool OperationClaimsPatrolOrderAuthority(
		HST_OperationRecordState operation,
		HST_EnemyOrderState order)
	{
		if (!operation || !order
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL)
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

	protected bool BatchClaimsPatrolOrderAuthority(
		HST_ForceSpawnResultState batch,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
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
		return operation && !operation.m_sProjectionId.IsEmpty()
			&& batch.m_sProjectionId == operation.m_sProjectionId;
	}

	protected bool GroupClaimsPatrolOrderAuthority(
		HST_ActiveGroupState group,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch)
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
		if (operation && !operation.m_sGroupId.IsEmpty() && group.m_sGroupId == operation.m_sGroupId)
			return true;
		return batch && !batch.m_sProjectionId.IsEmpty()
			&& group.m_sProjectionId == batch.m_sProjectionId;
	}

	protected string FindAmbiguousAuthorityRows(
		HST_CampaignState state,
		HST_EnemyOrderState order)
	{
		if (!state || !order)
			return "exact enemy patrol authority context is missing";
		if (state.FindEnemyOrder(order.m_sOrderId) != order
			|| CountEnemyOrdersByAnyAuthorityIdentity(state, order) != 1)
			return "exact enemy patrol order identity is ambiguous";
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		if (operation && CountOperationsByAnyAuthorityIdentity(state, order, operation) != 1)
			return "exact enemy patrol operation identity is ambiguous";
		HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
		if (manifest && CountManifestsByAnyAuthorityIdentity(state, order, manifest) != 1)
			return "exact enemy patrol manifest identity is ambiguous";
		HST_ForceSpawnResultState batch = FindLinkedBatch(state, order);
		if (batch && CountBatchesByAnyAuthorityIdentity(state, order, batch) != 1)
			return "exact enemy patrol spawn-result identity is ambiguous";
		HST_ActiveGroupState group = FindLinkedGroup(state, order);
		if (group && CountGroupsByAnyAuthorityIdentity(state, order, group) != 1)
			return "exact enemy patrol active-group identity is ambiguous";
		return "";
	}

	protected int CountEnemyOrdersByAnyAuthorityIdentity(
		HST_CampaignState state,
		HST_EnemyOrderState expected)
	{
		int count;
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

	protected int CountOperationsByAnyAuthorityIdentity(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState expected)
	{
		int count;
		foreach (HST_OperationRecordState candidate : state.m_aOperations)
		{
			if (!candidate)
				continue;
			bool matches = candidate.m_sOperationId == expected.m_sOperationId;
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

	protected int CountManifestsByAnyAuthorityIdentity(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState expected)
	{
		int count;
		foreach (HST_ForceManifestState candidate : state.m_aForceManifests)
		{
			if (!candidate)
				continue;
			if (candidate.m_sManifestId == expected.m_sManifestId
				|| candidate.m_sOperationId == order.m_sOperationId)
				count++;
		}
		return count;
	}

	protected int CountBatchesByAnyAuthorityIdentity(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceSpawnResultState expected)
	{
		int count;
		foreach (HST_ForceSpawnResultState candidate : state.m_aForceSpawnResults)
		{
			if (!candidate)
				continue;
			bool matches = candidate.m_sResultId == expected.m_sResultId;
			if (!matches && !order.m_sOperationId.IsEmpty())
				matches = candidate.m_sOperationId == order.m_sOperationId;
			if (!matches && !order.m_sManifestId.IsEmpty())
				matches = candidate.m_sManifestId == order.m_sManifestId;
			if (!matches && !order.m_sOrderId.IsEmpty())
				matches = candidate.m_sRequestId == order.m_sOrderId;
			if (!matches && !expected.m_sProjectionId.IsEmpty())
				matches = candidate.m_sProjectionId == expected.m_sProjectionId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountGroupsByAnyAuthorityIdentity(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState expected)
	{
		int count;
		foreach (HST_ActiveGroupState candidate : state.m_aActiveGroups)
		{
			if (!candidate)
				continue;
			bool matches = candidate.m_sGroupId == expected.m_sGroupId;
			if (!matches && !order.m_sOperationId.IsEmpty())
				matches = candidate.m_sOperationId == order.m_sOperationId;
			if (!matches && !order.m_sManifestId.IsEmpty())
				matches = candidate.m_sManifestId == order.m_sManifestId;
			if (!matches && !order.m_sSpawnResultId.IsEmpty())
				matches = candidate.m_sSpawnResultId == order.m_sSpawnResultId;
			if (!matches && !expected.m_sProjectionId.IsEmpty())
				matches = candidate.m_sProjectionId == expected.m_sProjectionId;
			if (matches)
				count++;
		}
		return count;
	}

	protected void RemoveInsertedAdmissionRows(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ActiveGroupState group)
	{
		if (!state)
			return;
		if (operation)
		{
			int operationIndex = state.m_aOperations.Find(operation);
			if (operationIndex >= 0)
				state.m_aOperations.Remove(operationIndex);
		}
		if (group)
		{
			int groupIndex = state.m_aActiveGroups.Find(group);
			if (groupIndex >= 0)
				state.m_aActiveGroups.Remove(groupIndex);
		}
		if (manifest)
		{
			int manifestIndex = state.m_aForceManifests.Find(manifest);
			if (manifestIndex >= 0)
				state.m_aForceManifests.Remove(manifestIndex);
		}
	}

	protected void RemoveRuntimeRows(
		HST_CampaignState state,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!state)
			return;
		if (batch)
		{
			int batchIndex = state.m_aForceSpawnResults.Find(batch);
			if (batchIndex >= 0)
				state.m_aForceSpawnResults.Remove(batchIndex);
		}
		if (group)
		{
			int groupIndex = state.m_aActiveGroups.Find(group);
			if (groupIndex >= 0)
				state.m_aActiveGroups.Remove(groupIndex);
		}
	}

	protected HST_ForceSpawnResultState FindLinkedBatch(
		HST_CampaignState state,
		HST_EnemyOrderState order)
	{
		if (!state || !order || order.m_sSpawnResultId.IsEmpty())
			return null;
		return state.FindForceSpawnResult(order.m_sSpawnResultId);
	}

	protected HST_ActiveGroupState FindLinkedGroup(
		HST_CampaignState state,
		HST_EnemyOrderState order)
	{
		if (!state || !order || order.m_sGroupId.IsEmpty())
			return null;
		return state.FindActiveGroup(order.m_sGroupId);
	}

	protected int CountGeneratedRouteId(HST_CampaignState state, string routeId)
	{
		if (!state || routeId.IsEmpty())
			return 0;
		int count;
		foreach (HST_GeneratedRouteState route : state.m_aGeneratedRoutes)
		{
			if (route && route.m_sRouteId == routeId)
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

	protected bool IsZeroVector(vector value)
	{
		return Math.AbsFloat(value[0]) < 0.01
			&& Math.AbsFloat(value[1]) < 0.01
			&& Math.AbsFloat(value[2]) < 0.01;
	}
}
