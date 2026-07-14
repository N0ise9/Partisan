class HST_EnemyGarrisonRebuildAdmissionResult
{
	bool m_bSuccess;
	bool m_bStateChanged;
	string m_sFailureReason;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
}

class HST_EnemyGarrisonRebuildOperationService
{
	protected static const int MAX_DELIVERY_DIAGNOSTIC_OPERATIONS = 64;

	static const int EXACT_CONTRACT_VERSION = 1;
	static const int QUARANTINED_CONTRACT_VERSION = -70;
	static const int EXACT_PRIORITY = 78;
	static const int EXACT_MAX_RETRIES = 3;
	static const int DEPLOYMENT_GRACE_SECONDS = 180;
	static const string EXACT_GROUP_MODE = "exact_enemy_garrison_rebuild";
	static const string DELIVERY_KIND = "delivered_garrison_transfer";
	static const string QUARANTINE_STATUS = "exact_garrison_rebuild_quarantined";

	protected ref HST_OperationService m_Operations = new HST_OperationService();
	protected ref HST_StrategicMovementService m_StrategicMovement = new HST_StrategicMovementService();
	protected ref HST_MaterializationService m_Materialization = new HST_MaterializationService();
	protected ref HST_ForcePlanningIntegrityService m_Integrity = new HST_ForcePlanningIntegrityService();
	protected ref HST_ForceSpawnQueueService m_SpawnQueue;
	protected ref HST_ForceSpawnAdapterService m_SpawnAdapter;
	protected ref HST_PhysicalWarService m_PhysicalWar;
	protected ref HST_GarrisonService m_Garrisons;
	protected ref HST_EnemyDirectorService m_EnemyDirector;
	protected ref map<string, string> m_mFirstDeliveryFailureByOperation
		= new map<string, string>();
	protected ref map<string, string> m_mLastDeliveryFailureByOperation
		= new map<string, string>();

	void SetRuntimeServices(
		HST_ForceSpawnQueueService spawnQueue,
		HST_ForceSpawnAdapterService spawnAdapter,
		HST_PhysicalWarService physicalWar,
		HST_GarrisonService garrisons)
	{
		m_SpawnQueue = spawnQueue;
		m_SpawnAdapter = spawnAdapter;
		m_PhysicalWar = physicalWar;
		m_Garrisons = garrisons;
	}

	void SetEnemyDirectorService(HST_EnemyDirectorService enemyDirector)
	{
		m_EnemyDirector = enemyDirector;
	}

	string GetFirstDeliveryFailure(HST_EnemyOrderState order)
	{
		string key = ResolveDeliveryDiagnosticKey(order);
		if (key.IsEmpty() || !m_mFirstDeliveryFailureByOperation.Contains(key))
			return "";
		return m_mFirstDeliveryFailureByOperation.Get(key);
	}

	string GetLastDeliveryFailure(HST_EnemyOrderState order)
	{
		string key = ResolveDeliveryDiagnosticKey(order);
		if (key.IsEmpty() || !m_mLastDeliveryFailureByOperation.Contains(key))
			return "";
		return m_mLastDeliveryFailureByOperation.Get(key);
	}

	bool IsExactEnemyGarrisonRebuild(HST_EnemyOrderState order)
	{
		return HST_OperationService.RequiresExactEnemyGarrisonRebuild(order);
	}

	bool HasPreparedSettlementResumeCandidate(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		if (!IsExactEnemyGarrisonRebuild(order) || !operation)
			return false;
		if (operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
			return true;
		return operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& !order.m_bResourceSettlementApplied;
	}

	bool HasOpenExactEnemyGarrisonRebuild(
		HST_CampaignState state,
		string factionKey,
		string targetZoneId)
	{
		if (!state || factionKey.IsEmpty() || targetZoneId.IsEmpty())
			return false;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsExactEnemyGarrisonRebuild(order)
				|| order.m_sFactionKey != factionKey
				|| !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					order.m_sTargetZoneId,
					targetZoneId))
				continue;
			HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
			if (HasPreparedSettlementResumeCandidate(order, operation))
				return true;
			if (operation && operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				return true;
			if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				|| order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				return true;
		}
		return false;
	}

	HST_EnemyGarrisonRebuildAdmissionResult CanAdmitPreparedOrder(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_EnemyDirectorService enemyDirector)
	{
		HST_EnemyGarrisonRebuildAdmissionResult result
			= new HST_EnemyGarrisonRebuildAdmissionResult();
		string failure = ValidateAdmissionContext(state, order, manifest, enemyDirector);
		if (!failure.IsEmpty())
		{
			result.m_sFailureReason = failure;
			return result;
		}
		failure = FindAmbiguousAuthorityRows(state, order);
		if (!failure.IsEmpty())
		{
			result.m_sFailureReason = failure;
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
			result.m_sFailureReason = "exact enemy garrison rebuild spawn admission preflight failed";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = result.m_sFailureReason + ": " + preflight.m_sFailureReason;
			return result;
		}
		result.m_bSuccess = true;
		return result;
	}

	HST_EnemyGarrisonRebuildAdmissionResult AdmitPreparedOrder(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_EnemyDirectorService enemyDirector)
	{
		HST_EnemyGarrisonRebuildAdmissionResult result
			= new HST_EnemyGarrisonRebuildAdmissionResult();
		if (enemyDirector)
			m_EnemyDirector = enemyDirector;
		if (HasCommittedAdmissionAuthority(state, order))
			return ResolveCommittedAdmissionReplay(state, order, manifest);
		string debitFailure = HST_EnemyGarrisonRebuildSaveValidationService
			.ValidateOriginalResourceDebitAuthority(state.m_aEnemyStrategicMutations, order);
		if (!debitFailure.IsEmpty())
		{
			result.m_sFailureReason = debitFailure;
			return result;
		}
		HST_EnemyGarrisonRebuildAdmissionResult preflight
			= CanAdmitPreparedOrder(state, order, manifest, enemyDirector);
		if (!preflight || !preflight.m_bSuccess)
		{
			string failure = "exact enemy garrison rebuild admission preflight failed";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				failure = preflight.m_sFailureReason;
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
			result.m_sFailureReason = failure;
			result.m_bStateChanged = true;
			return result;
		}

		ApplyManifestComposition(order, manifest);
		state.m_aForceManifests.Insert(manifest);
		HST_OperationTransitionResult registered
			= m_Operations.RegisterExactEnemyGarrisonRebuild(state, order, manifest);
		if (!registered || !registered.m_bAccepted || !registered.m_Operation)
		{
			string failure = "exact enemy garrison rebuild operation registration failed";
			if (registered && !registered.m_sFailureReason.IsEmpty())
				failure = failure + ": " + registered.m_sFailureReason;
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
			result.m_sFailureReason = failure;
			result.m_bStateChanged = true;
			return result;
		}
		HST_ActiveGroupState group = BuildActiveGroup(state, order, manifest);
		if (!group)
		{
			string failure = "exact enemy garrison rebuild active group could not be built";
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
			result.m_sFailureReason = failure;
			result.m_bStateChanged = true;
			return result;
		}
		state.m_aActiveGroups.Insert(group);

		HST_ForceSpawnQueueRequest request = BuildSpawnRequest(state, order);
		HST_ForceSpawnQueueEnqueueResult enqueue = m_SpawnQueue.Enqueue(
			state.m_aForceSpawnResults,
			manifest,
			request,
			state.m_iElapsedSeconds);
		if (!enqueue || !enqueue.m_bSuccess || !enqueue.m_Batch)
		{
			string failure = "exact enemy garrison rebuild spawn admission failed";
			if (enqueue && !enqueue.m_sFailureReason.IsEmpty())
				failure = failure + ": " + enqueue.m_sFailureReason;
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
			result.m_sFailureReason = failure;
			result.m_bStateChanged = true;
			return result;
		}
		HST_ForceSpawnQueueCallbackResult held
			= m_SpawnQueue.HoldPendingProjectionForStrategicSimulation(
				state.m_aForceSpawnResults,
				manifest,
				enqueue.m_Batch.m_sResultId,
				enqueue.m_Batch.m_sProjectionId,
				state.m_iElapsedSeconds);
		if (!held || !held.m_bAccepted
			|| !m_StrategicMovement.InitializeExactInfantryDirectRoute(
				state,
				registered.m_Operation,
				manifest,
				group))
		{
			string failure = "exact enemy garrison rebuild strategic hold or route failed";
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
			result.m_sFailureReason = failure;
			result.m_bStateChanged = true;
			return result;
		}
		HST_OperationTransitionResult linked
			= m_Operations.LinkExactEnemyGarrisonRebuildOutboundVirtual(
				state,
				order,
				group,
				enqueue.m_Batch);
		if (!linked || !linked.m_bAccepted)
		{
			string failure = "exact enemy garrison rebuild reciprocal runtime link failed";
			FailAdmissionAfterDebit(state, order, manifest, enemyDirector, failure);
			result.m_sFailureReason = failure;
			result.m_bStateChanged = true;
			return result;
		}
		order.m_sSpawnResultId = enqueue.m_Batch.m_sResultId;
		order.m_sGroupId = group.m_sGroupId;
		order.m_bStrategicServiceCommitted = true;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
		order.m_sRuntimeStatus = "exact_rebuild_virtual_outbound";
		order.m_sFailureReason = "";
		result.m_bSuccess = true;
		result.m_bStateChanged = true;
		result.m_Operation = registered.m_Operation;
		result.m_Batch = enqueue.m_Batch;
		result.m_Group = group;
		return result;
	}

	protected string ValidateAdmissionContext(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_EnemyDirectorService enemyDirector)
	{
		if (!state || !order || !manifest || !enemyDirector || !m_SpawnQueue
			|| !m_SpawnAdapter || !m_PhysicalWar || !m_Garrisons)
			return "exact enemy garrison rebuild admission services are unavailable";
		if (!IsExactEnemyGarrisonRebuild(order) || order.m_sOrderId.IsEmpty()
			|| order.m_sOperationId
				!= HST_StableIdService.BuildOperationId("enemy_order", order.m_sOrderId)
			|| order.m_sFactionKey.IsEmpty() || order.m_sSourceZoneId.IsEmpty()
			|| order.m_sTargetZoneId.IsEmpty()
			|| HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
				order.m_sSourceZoneId,
				order.m_sTargetZoneId))
			return "exact enemy garrison rebuild order identity is invalid";
		if (order.m_iAttackCost != 0 || order.m_iSupportCost <= 0
			|| order.m_iSupportCost
				!= HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_SUPPORT_COST)
			return "exact enemy garrison rebuild must charge the exact support-only ledger";
		HST_ZoneState source = state.FindZone(order.m_sSourceZoneId);
		HST_ZoneState target = state.FindZone(order.m_sTargetZoneId);
		if (!source || !target || source.m_sOwnerFactionKey != order.m_sFactionKey
			|| target.m_sOwnerFactionKey != order.m_sFactionKey
			|| order.m_iTargetOwnershipRevision <= 0
			|| target.m_iOwnershipRevision != order.m_iTargetOwnershipRevision)
			return "exact enemy garrison rebuild source or target ownership changed";
		bool manifestIdentityExact = manifest.m_bFrozen
			&& manifest.m_sManifestId == "manifest_" + order.m_sOperationId
			&& manifest.m_sOperationId == order.m_sOperationId
			&& manifest.m_sFactionKey == order.m_sFactionKey;
		bool manifestPolicyExact = manifest.m_sForceKind
			== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_FORCE_KIND
			&& manifest.m_sPolicyId
				== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_POLICY_ID
			&& manifest.m_sIntentId
				== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_MANIFEST_INTENT;
		bool manifestRouteExact = manifest.m_sSourceZoneId == order.m_sSourceZoneId
			&& manifest.m_sTargetZoneId == order.m_sTargetZoneId
			&& m_StrategicMovement.IsSupportedExactInfantryManifest(manifest);
		bool manifestLedgerExact = manifest.m_iAttackResourceCost == 0
			&& manifest.m_iSupportResourceCost == order.m_iSupportCost;
		bool manifestHashExact = !manifest.m_sManifestHash.IsEmpty()
			&& m_Integrity.BuildManifestHash(manifest) == manifest.m_sManifestHash;
		if (!manifestIdentityExact || !manifestPolicyExact || !manifestRouteExact
			|| !manifestLedgerExact || !manifestHashExact)
			return "exact enemy garrison rebuild frozen manifest is invalid";
		if (target.m_iGarrisonSlots > 0
			&& ResolveAuthoritativeGarrisonInfantry(
				state,
				order.m_sFactionKey,
				order.m_sTargetZoneId) + manifest.m_iAcceptedMemberCount
					> target.m_iGarrisonSlots)
			return "exact enemy garrison rebuild exceeds target garrison capacity";
		foreach (HST_EnemyOrderState other : state.m_aEnemyOrders)
		{
			if (!other || other == order || !IsExactEnemyGarrisonRebuild(other))
				continue;
			if (other.m_sOperationId == order.m_sOperationId)
				return "exact enemy garrison rebuild operation identity already exists";
			if (other.m_sFactionKey == order.m_sFactionKey
				&& HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					other.m_sTargetZoneId,
					order.m_sTargetZoneId)
				&& HasOpenExactEnemyGarrisonRebuild(
					state,
					other.m_sFactionKey,
					other.m_sTargetZoneId))
				return "another exact enemy garrison rebuild already reserves the target";
		}
		return "";
	}

	protected HST_ForceSpawnQueueRequest BuildSpawnRequest(
		HST_CampaignState state,
		HST_EnemyOrderState order)
	{
		HST_ForceSpawnQueueRequest request = new HST_ForceSpawnQueueRequest();
		request.m_sResultId = BuildSpawnResultId(order);
		request.m_sRequestId = order.m_sOrderId;
		request.m_sForceId = BuildForceId(order);
		request.m_sProjectionId = BuildProjectionId(order);
		request.m_iPriority = EXACT_PRIORITY;
		request.m_iMaxRetries = EXACT_MAX_RETRIES;
		request.m_iDeadlineSecond = state.m_iElapsedSeconds + DEPLOYMENT_GRACE_SECONDS;
		return request;
	}

	protected HST_ActiveGroupState BuildActiveGroup(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!state || !order || !manifest || manifest.m_aGroups.Count() != 1
			|| !manifest.m_aGroups[0])
			return null;
		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = BuildProjectionId(order);
		group.m_sProjectionId = group.m_sGroupId;
		group.m_sOperationId = order.m_sOperationId;
		group.m_sEnemyOrderId = order.m_sOrderId;
		group.m_sManifestId = manifest.m_sManifestId;
		group.m_sSpawnResultId = BuildSpawnResultId(order);
		group.m_sForceId = BuildForceId(order);
		group.m_sZoneId = order.m_sTargetZoneId;
		group.m_sFactionKey = order.m_sFactionKey;
		group.m_sPrefab = manifest.m_aGroups[0].m_sPrefab;
		group.m_sCompositionRequestId = manifest.m_sManifestId;
		group.m_sCompositionIntentId = manifest.m_sIntentId;
		group.m_sCompositionTier = "exact";
		group.m_sCompositionSummary
			= string.Format("%1 exact rebuild infantry", manifest.m_iAcceptedMemberCount);
		group.m_sSpawnFallbackMode = EXACT_GROUP_MODE;
		group.m_sRouteId = order.m_sOperationId + "_outbound";
		group.m_vSourcePosition = order.m_vSourcePosition;
		group.m_vTargetPosition = order.m_vTargetPosition;
		group.m_vPosition = order.m_vSourcePosition;
		group.m_sRuntimeStatus = "enemy_garrison_rebuild_virtual";
		group.m_iInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iOriginalInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iCompositionCost = manifest.m_iSupportResourceCost;
		group.m_iCompositionManpower = manifest.m_iAcceptedMemberCount;
		group.m_iLastSeenAliveCount = manifest.m_iAcceptedMemberCount;
		group.m_iSurvivorInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iDurableLivingInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iSpawnedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		group.m_bQRF = true;
		return group;
	}

	protected void ApplyManifestComposition(
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		order.m_sManifestId = manifest.m_sManifestId;
		order.m_sManifestHash = manifest.m_sManifestHash;
		order.m_iCompositionManpower = manifest.m_iAcceptedMemberCount;
		order.m_iCompositionVehicleCount = 0;
		order.m_iCompositionArmedVehicleCount = 0;
		order.m_sCompositionIntentId = manifest.m_sIntentId;
		order.m_sCompositionTier = "exact";
		order.m_sCompositionSummary
			= string.Format("%1 exact rebuild infantry", manifest.m_iAcceptedMemberCount);
		order.m_iResolveAtSecond = 0;
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
		if (!state)
			return false;
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		return operation && (!operation.m_sSpawnResultId.IsEmpty()
			|| !operation.m_sGroupId.IsEmpty() || !operation.m_sProjectionId.IsEmpty());
	}

	protected HST_EnemyGarrisonRebuildAdmissionResult ResolveCommittedAdmissionReplay(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState suppliedManifest)
	{
		HST_EnemyGarrisonRebuildAdmissionResult result
			= new HST_EnemyGarrisonRebuildAdmissionResult();
		HST_OperationRecordState operation;
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
		if (!failure.IsEmpty() || !suppliedManifest || suppliedManifest != manifest
			|| suppliedManifest.m_sManifestHash != manifest.m_sManifestHash
			|| order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			|| !order.m_bStrategicServiceCommitted)
		{
			result.m_sFailureReason = "exact enemy garrison rebuild committed admission replay conflicts";
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

	protected string FindAmbiguousAuthorityRows(
		HST_CampaignState state,
		HST_EnemyOrderState order)
	{
		if (!state || !order)
			return "exact enemy garrison rebuild authority context is missing";
		int operationClaims;
		int manifestClaims;
		int batchClaims;
		int groupClaims;
		string resultId = BuildSpawnResultId(order);
		string forceId = BuildForceId(order);
		string projectionId = BuildProjectionId(order);
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (operation && (operation.m_sOperationId == order.m_sOperationId
				|| operation.m_sEnemyOrderId == order.m_sOrderId
				|| operation.m_sManifestId == order.m_sManifestId
				|| operation.m_sSpawnResultId == resultId
				|| operation.m_sProjectionId == projectionId))
				operationClaims++;
		}
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
		{
			if (manifest && (manifest.m_sManifestId == order.m_sManifestId
				|| manifest.m_sOperationId == order.m_sOperationId))
				manifestClaims++;
		}
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && (batch.m_sResultId == resultId
				|| batch.m_sRequestId == order.m_sOrderId
				|| batch.m_sOperationId == order.m_sOperationId
				|| batch.m_sManifestId == order.m_sManifestId
				|| batch.m_sForceId == forceId
				|| batch.m_sProjectionId == projectionId))
				batchClaims++;
		}
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && (group.m_sGroupId == projectionId
				|| group.m_sEnemyOrderId == order.m_sOrderId
				|| group.m_sOperationId == order.m_sOperationId
				|| group.m_sManifestId == order.m_sManifestId
				|| group.m_sSpawnResultId == resultId
				|| group.m_sForceId == forceId
				|| group.m_sProjectionId == projectionId))
				groupClaims++;
		}
		if (operationClaims > 1 || manifestClaims > 1 || batchClaims > 1 || groupClaims > 1)
			return "exact enemy garrison rebuild durable claimant identity is ambiguous";
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
			return "exact enemy garrison rebuild reciprocal runtime graph is incomplete";
		string failure = m_Operations.ValidateExactEnemyGarrisonRebuild(
			state,
			operation,
			order,
			manifest);
		if (!failure.IsEmpty())
			return failure;
		if (operation.m_sSpawnResultId != batch.m_sResultId
			|| operation.m_sGroupId != group.m_sGroupId
			|| batch.m_sOperationId != operation.m_sOperationId
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| group.m_sOperationId != operation.m_sOperationId
			|| group.m_sEnemyOrderId != order.m_sOrderId
			|| group.m_sManifestId != manifest.m_sManifestId
			|| group.m_sSpawnResultId != batch.m_sResultId
			|| group.m_sProjectionId != batch.m_sProjectionId
			|| group.m_sForceId != batch.m_sForceId)
			return "exact enemy garrison rebuild reciprocal runtime links conflict";
		return "";
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

	bool TickOrder(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order)
	{
		if (!IsExactEnemyGarrisonRebuild(order))
			return false;
		if (!state || !preset || !enemyDirector || !m_SpawnQueue || !m_SpawnAdapter
			|| !m_PhysicalWar || !m_Garrisons)
			return SetRuntimeConflict(order, "exact enemy garrison rebuild runtime services are unavailable");
		string ambiguity = FindAmbiguousAuthorityRows(state, order);
		if (!ambiguity.IsEmpty())
			return QuarantineRuntimeAuthority(order, ambiguity);
		string debitFailure = HST_EnemyGarrisonRebuildSaveValidationService
			.ValidateOriginalResourceDebitAuthority(state.m_aEnemyStrategicMutations, order);
		if (!debitFailure.IsEmpty())
			return QuarantineRuntimeAuthority(order, debitFailure);
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		if (HasPreparedSettlementResumeCandidate(order, operation))
			return ResumePreparedSettlement(state, enemyDirector, order, operation);
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return FinalizeSettledOrder(state, order, operation);
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& !(order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED
				&& order.m_sResourceSettlementKind == DELIVERY_KIND))
			return false;

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
			return QuarantineRuntimeAuthority(order, failure);

		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& (!TargetOwnershipMatches(state, order) || !SourceOwnershipMatches(state, order)))
			return BeginPrearrivalInvalidationReturn(
				state,
				order,
				operation,
				group,
				"exact enemy garrison rebuild source or target ownership changed before arrival");
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			&& !TargetOwnershipMatches(state, order))
			return SettleDeliveredOperation(
				state,
				order,
				operation,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"exact delivered garrison rebuild ownership authority changed");

		if (operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
			return ContinueDematerialization(state, order, operation, manifest, batch, group,
				operation.m_sLastProjectionReason);
		if (operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL)
			return TickVirtual(state, enemyDirector, order, operation, manifest, batch, group);
		if (operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
			return TickMaterializing(state, enemyDirector, order, operation, manifest, batch, group);
		if (operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			return TickPhysical(state, enemyDirector, order, operation, manifest, batch, group);
		return QuarantineRuntimeAuthority(order,
			"exact enemy garrison rebuild materialization state is invalid");
	}

	protected bool TickVirtual(
		HST_CampaignState state,
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
			if (IsDelivered(order))
				return SettleDeliveredOperation(state, order, operation, manifest, batch, group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
					"exact delivered garrison rebuild virtual roster ended");
			return SettlePrearrivalOperation(state, enemyDirector, order, operation, manifest,
				batch, group, HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"virtual_projection_failed_survivors",
				ResolveLivingRoster(operation, manifest, batch, group),
				"exact enemy garrison rebuild held projection failed");
		}
		if (!batch.m_bStrategicProjectionHeld)
			return QuarantineRuntimeAuthority(order,
				"exact enemy garrison rebuild virtual batch is not strategically held");
		int living = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
		string deliveryFailureStage;
		string deliveryFailureReason;
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
				"exact enemy garrison rebuild virtual roster eliminated");
			if (IsDelivered(order))
				return SettleDeliveredOperation(state, order, operation, manifest, batch, group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
					"exact delivered garrison rebuild was eliminated");
			return SettlePrearrivalOperation(state, enemyDirector, order, operation, manifest,
				batch, group, HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
				"destroyed", 0, "exact enemy garrison rebuild was eliminated before arrival");
		}

		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
		{
			if (!IsDelivered(order))
			{
				if (!CompleteDelivery(
					state,
					enemyDirector,
					order,
					operation,
					manifest,
					batch,
					group,
					living,
					deliveryFailureStage,
					deliveryFailureReason))
					return SetDeliveryRuntimeConflict(
						order,
						deliveryFailureStage,
						deliveryFailureReason);
			}
			HST_OperationProjectionDecision onStationDecision
				= m_Materialization.EvaluateExactEnemyCounterattack(operation, operation.m_vStrategicPosition);
			operation.m_sLastProjectionReason = onStationDecision.m_sReason;
			if (onStationDecision.m_eDecision
				== HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE)
				return BeginMaterialization(state, order, operation, manifest, batch, group,
					onStationDecision.m_sReason);
			group.m_sRuntimeStatus = "enemy_garrison_rebuild_virtual_on_station";
			return false;
		}

		if (!m_StrategicMovement.InitializeExactInfantryDirectRoute(state, operation, manifest, group))
			return SettlePrearrivalOperation(state, enemyDirector, order, operation, manifest,
				batch, group, HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
				"route_failed_survivors", living,
				"exact enemy garrison rebuild strategic route is invalid");
		HST_StrategicMovementResult movement
			= m_StrategicMovement.AdvanceExactInfantryDirectRoute(state, operation, group);
		if (!movement || !movement.m_bAccepted)
			return SettlePrearrivalOperation(state, enemyDirector, order, operation, manifest,
				batch, group, HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
				"route_failed_survivors", living,
				"exact enemy garrison rebuild strategic movement failed");
		bool changed = movement.m_bStateChanged;
		if (movement.m_bArrived)
		{
			if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
			{
				HST_OperationTransitionResult onStation
					= m_Operations.MarkExactEnemyGarrisonRebuildOnStation(state, order, group);
				if (!onStation || !onStation.m_bAccepted)
					return QuarantineRuntimeAuthority(order,
						"exact enemy garrison rebuild virtual arrival transition failed");
				if (!CompleteDelivery(
					state,
					enemyDirector,
					order,
					operation,
					manifest,
					batch,
					group,
					living,
					deliveryFailureStage,
					deliveryFailureReason))
				{
					SetDeliveryRuntimeConflict(
						order,
						deliveryFailureStage,
						deliveryFailureReason);
					return onStation.m_bStateChanged;
				}
				return true;
			}
			if (operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
				return SettlePrearrivalOperation(state, enemyDirector, order, operation, manifest,
					batch, group, HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
					"returned_invalidated_survivors", living,
					order.m_sFailureReason) || changed;
		}
		HST_OperationProjectionDecision decision
			= m_Materialization.EvaluateExactEnemyCounterattack(operation, operation.m_vStrategicPosition);
		operation.m_sLastProjectionReason = decision.m_sReason;
		if (decision.m_eDecision
			== HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE)
			return BeginMaterialization(state, order, operation, manifest, batch, group, decision.m_sReason) || changed;
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
			if (IsDelivered(order))
				return SettleDeliveredOperation(state, order, operation, manifest, batch, group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
					"exact delivered garrison rebuild rematerialization failed");
			return SettlePrearrivalOperation(state, enemyDirector, order, operation, manifest,
				batch, group, HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"materialization_failed_survivors",
				ResolveLivingRoster(operation, manifest, batch, group),
				"exact enemy garrison rebuild materialization failed");
		}
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| !group.m_bSpawnedEntity)
			return false;
		HST_OperationTransitionResult physical
			= m_Operations.MarkExactEnemyGarrisonRebuildPhysical(
				state,
				order,
				group,
				batch,
				"exact enemy garrison rebuild materialized at its strategic cursor");
		if (!physical || !physical.m_bAccepted)
			return QuarantineRuntimeAuthority(order,
				"exact enemy garrison rebuild physical handoff conflicts");
		if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
		{
			if (!m_PhysicalWar.RestartExactEnemyGarrisonRebuildInfantryRoute(
				state,
				group,
				operation.m_vRouteEndPosition,
				"Exact enemy garrison rebuild following its authoritative route."))
				return QuarantineRuntimeAuthority(order,
					"exact enemy garrison rebuild live route could not be issued");
		}
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
		{
			order.m_sRuntimeStatus = "resolved_exact_rebuild_physical_on_station";
			group.m_sRuntimeStatus = "enemy_garrison_rebuild_physical_on_station";
		}
		else if (operation.m_eDutyState
			== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
		{
			order.m_sRuntimeStatus = "exact_rebuild_physical_returning";
			group.m_sRuntimeStatus = "enemy_garrison_rebuild_physical_returning";
		}
		else
			order.m_sRuntimeStatus = "exact_rebuild_physical_outbound";
		return true;
	}

	protected bool TickPhysical(
		HST_CampaignState state,
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
			return QuarantineRuntimeAuthority(order,
				"exact enemy garrison rebuild live runtime binding is missing or ambiguous");
		HST_ForceSpawnAdapterTickResult reconciled
			= m_SpawnAdapter.ReconcileExactInfantryProjectionAuthority(
				state,
				m_SpawnQueue,
				m_PhysicalWar,
				Math.Max(0, state.m_iElapsedSeconds),
				operation.m_sProjectionId);
		if (!reconciled || reconciled.m_iFailedCount > 0)
			return SetRuntimeConflict(order,
				"exact enemy garrison rebuild physical casualty reconciliation is pending");
		HST_OperationTransitionResult live
			= m_Operations.UpdateExactEnemyGarrisonRebuildPhysicalPosition(state, order, group);
		if (!live || !live.m_bAccepted)
			return QuarantineRuntimeAuthority(order,
				"exact enemy garrison rebuild live-position update failed");
		bool changed = live.m_bStateChanged;
		int living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
		string deliveryFailureStage;
		string deliveryFailureReason;
		SyncGroupRoster(group, living);
		operation.m_iLastVirtualFriendlyCount = living;
		if (living <= 0 || group.m_sRuntimeStatus == "eliminated")
		{
			string eliminationFailure;
			if (!m_PhysicalWar.FinalizeEliminatedForceSpawnProjection(
				state,
				group,
				state.m_iElapsedSeconds,
				eliminationFailure))
				return SetRuntimeConflict(order,
					"exact enemy garrison rebuild elimination cleanup is pending: " + eliminationFailure);
			if (IsDelivered(order))
				return SettleDeliveredOperation(state, order, operation, manifest, batch, group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
					"exact delivered garrison rebuild was eliminated") || changed;
			return SettlePrearrivalOperation(state, enemyDirector, order, operation, manifest,
				batch, group, HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
				"destroyed", 0,
				"exact enemy garrison rebuild was eliminated before arrival") || changed;
		}
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
		{
			if (!IsDelivered(order))
			{
				if (!CompleteDelivery(
					state,
					enemyDirector,
					order,
					operation,
					manifest,
					batch,
					group,
					living,
					deliveryFailureStage,
					deliveryFailureReason))
					return SetDeliveryRuntimeConflict(
						order,
						deliveryFailureStage,
						deliveryFailureReason);
			}
			HST_OperationProjectionDecision stationDecision
				= m_Materialization.EvaluateExactEnemyCounterattack(operation, group.m_vPosition);
			operation.m_sLastProjectionReason = stationDecision.m_sReason;
			if (stationDecision.m_eDecision
				== HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_DEMATERIALIZE)
				return TryDematerialize(state, order, operation, manifest, batch, group,
					stationDecision.m_sReason) || changed;
			return changed;
		}
		if (m_PhysicalWar.IsExactEnemyGarrisonRebuildRouteRecoveryExhausted(
			group,
			state.m_iElapsedSeconds))
			return QuarantineRuntimeAuthority(order,
				"exact enemy garrison rebuild exhausted bounded live-route recovery");
		HST_OperationTransitionResult sample
			= m_Operations.ConfirmExactEnemyGarrisonRebuildArrivalSample(state, order, group);
		if (!sample || !sample.m_bAccepted)
			return QuarantineRuntimeAuthority(order,
				"exact enemy garrison rebuild live arrival sampling failed");
		changed = sample.m_bStateChanged || changed;
		if (operation.m_iArrivalConfirmationCount >= 2)
		{
			if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
			{
				HST_OperationTransitionResult onStation
					= m_Operations.MarkExactEnemyGarrisonRebuildOnStation(state, order, group);
				if (!onStation || !onStation.m_bAccepted)
					return QuarantineRuntimeAuthority(order,
						"exact enemy garrison rebuild physical arrival transition failed");
				if (!CompleteDelivery(
					state,
					enemyDirector,
					order,
					operation,
					manifest,
					batch,
					group,
					living,
					deliveryFailureStage,
					deliveryFailureReason))
				{
					SetDeliveryRuntimeConflict(
						order,
						deliveryFailureStage,
						deliveryFailureReason);
					return onStation.m_bStateChanged;
				}
				return true;
			}
			if (operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
				return TryDematerialize(state, order, operation, manifest, batch, group,
					"return arrival folded for survivor settlement") || changed;
		}
		HST_OperationProjectionDecision decision
			= m_Materialization.EvaluateExactEnemyCounterattack(operation, group.m_vPosition);
		operation.m_sLastProjectionReason = decision.m_sReason;
		if (decision.m_eDecision
			== HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_DEMATERIALIZE)
			return TryDematerialize(state, order, operation, manifest, batch, group,
				decision.m_sReason) || changed;
		return changed;
	}

	protected bool BeginMaterialization(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		int deadlineSecond = state.m_iElapsedSeconds + DEPLOYMENT_GRACE_SECONDS;
		HST_ForceSpawnQueueCallbackResult released
			= m_SpawnQueue.ReleaseStrategicProjectionForMaterialization(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				state.m_iElapsedSeconds,
				deadlineSecond);
		if (!released || !released.m_bAccepted)
			return SetRuntimeConflict(order,
				"exact enemy garrison rebuild materialization release failed");
		group.m_vPosition = operation.m_vStrategicPosition;
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		group.m_vTargetPosition = operation.m_vRouteEndPosition;
		group.m_sRuntimeStatus = "enemy_garrison_rebuild_materializing";
		HST_OperationTransitionResult materializing
			= m_Operations.MarkExactEnemyGarrisonRebuildMaterializingFromVirtual(
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
			return QuarantineRuntimeAuthority(order,
				"exact enemy garrison rebuild materialization transition failed");
		}
		order.m_sRuntimeStatus = "exact_rebuild_materializing";
		return true;
	}

	protected bool TryDematerialize(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		int deadlineSecond = state.m_iElapsedSeconds + DEPLOYMENT_GRACE_SECONDS;
		HST_ForceSpawnQueueCallbackResult preflight
			= m_SpawnQueue.CanRequeueSuccessfulProjectionForStrategicHold(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				state.m_iElapsedSeconds,
				deadlineSecond);
		if (!preflight || !preflight.m_bAccepted)
			return SetRuntimeConflict(order,
				"exact enemy garrison rebuild dematerialization is waiting for queue capacity");
		HST_OperationTransitionResult begun
			= m_Operations.BeginExactEnemyGarrisonRebuildDematerialization(
				state,
				order,
				group,
				reason);
		if (!begun || !begun.m_bAccepted)
			return QuarantineRuntimeAuthority(order,
				"exact enemy garrison rebuild dematerialization transition failed");
		return ContinueDematerialization(
			state,
			order,
			begun.m_Operation,
			manifest,
			batch,
			group,
			reason);
	}

	protected bool ContinueDematerialization(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		if (!batch.m_bStrategicProjectionHeld)
		{
			HST_ForceSpawnAdapterTickResult reconciled
				= m_SpawnAdapter.ReconcileExactInfantryProjectionAuthority(
					state,
					m_SpawnQueue,
					m_PhysicalWar,
					Math.Max(0, state.m_iElapsedSeconds),
					operation.m_sProjectionId);
			if (!reconciled || reconciled.m_iFailedCount > 0)
				return SetRuntimeConflict(order,
					"exact enemy garrison rebuild dematerialization casualty reconciliation is pending");
			HST_ForceSpawnAdapterRetireResult retired
				= m_SpawnAdapter.RetireProjectionRuntime(
					state,
					m_PhysicalWar,
					group.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return SetRuntimeConflict(order,
					"exact enemy garrison rebuild runtime retirement is pending");
			int deadlineSecond = state.m_iElapsedSeconds + DEPLOYMENT_GRACE_SECONDS;
			HST_ForceSpawnQueueCallbackResult held
				= m_SpawnQueue.RequeueSuccessfulProjectionForStrategicHold(
					state.m_aForceSpawnResults,
					manifest,
					batch.m_sResultId,
					batch.m_sProjectionId,
					state.m_iElapsedSeconds,
					deadlineSecond);
			if (!held || !held.m_bAccepted)
				return SetRuntimeConflict(order,
					"exact enemy garrison rebuild survivors could not enter strategic hold");
		}
		m_StrategicMovement.SyncRouteProgressFromPosition(operation, group.m_vPosition);
		group.m_bSpawnedEntity = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		SyncGroupRoster(group, m_SpawnQueue.CountStrategicLivingMemberSlots(batch));
		group.m_iLifecycleRevision++;
		HST_OperationTransitionResult completed
			= m_Operations.CompleteExactEnemyGarrisonRebuildDematerialization(
				state,
				order,
				group,
				batch,
				reason);
		if (!completed || !completed.m_bAccepted)
			return QuarantineRuntimeAuthority(order,
				"exact enemy garrison rebuild dematerialization completion failed");
		order.m_bPhysicalized = false;
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
		{
			order.m_sRuntimeStatus = "resolved_exact_rebuild_virtual_on_station";
			group.m_sRuntimeStatus = "enemy_garrison_rebuild_virtual_on_station";
		}
		else if (operation.m_eDutyState
			== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
		{
			order.m_sRuntimeStatus = "exact_rebuild_virtual_returning";
			group.m_sRuntimeStatus = "enemy_garrison_rebuild_virtual_returning";
		}
		else
		{
			order.m_sRuntimeStatus = "exact_rebuild_virtual_outbound";
			group.m_sRuntimeStatus = "enemy_garrison_rebuild_virtual";
		}
		return true;
	}

	protected bool CompleteDelivery(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		int living,
		out string failureStage,
		out string failureReason)
	{
		failureStage = "";
		failureReason = "";
		if (!state)
		{
			failureStage = "delivery_prerequisite";
			failureReason = "campaign state is missing";
			return false;
		}
		if (!enemyDirector)
		{
			failureStage = "delivery_prerequisite";
			failureReason = "enemy director is missing";
			return false;
		}
		if (!m_Garrisons)
		{
			failureStage = "delivery_prerequisite";
			failureReason = "garrison authority service is missing";
			return false;
		}
		if (!order || !operation || !manifest || !batch || !group)
		{
			failureStage = "delivery_prerequisite";
			failureReason = "order, operation, manifest, batch, or group authority is missing";
			return false;
		}
		if (living <= 0)
		{
			failureStage = "delivery_prerequisite";
			failureReason = "delivery has no living exact roster";
			return false;
		}
		if (!TargetOwnershipMatches(state, order))
		{
			HST_ZoneState target = state.FindZone(order.m_sTargetZoneId);
			string currentOwner = "missing";
			int currentRevision;
			if (target)
			{
				currentRevision = target.m_iOwnershipRevision;
				if (!target.m_sOwnerFactionKey.IsEmpty())
					currentOwner = target.m_sOwnerFactionKey;
			}
			failureStage = "delivery_prerequisite";
			failureReason = string.Format(
				"target ownership changed | current owner/revision %1/%2 | frozen %3/%4",
				currentOwner,
				currentRevision,
				order.m_sFactionKey,
				order.m_iTargetOwnershipRevision);
			return false;
		}
		HST_GarrisonState garrison = m_Garrisons.LinkExactEnemyGarrisonRebuildManifest(
			state,
			order.m_sTargetZoneId,
			order.m_sFactionKey,
			manifest,
			living);
		if (!garrison)
		{
			failureStage = "garrison_link";
			failureReason = BuildGarrisonLinkFailureReason(
				state,
				order,
				manifest,
				living);
			return false;
		}
		string settlementFailureStage;
		string settlementFailureReason;
		if (!ApplyResourceSettlement(
			state,
			enemyDirector,
			order,
			manifest,
			DELIVERY_KIND,
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE,
			living,
			false,
			true,
			"exact enemy garrison rebuild delivered its surviving roster",
			settlementFailureStage,
			settlementFailureReason))
		{
			bool unlinked = m_Garrisons.UnlinkExactEnemyGarrisonRebuildManifest(
				state,
				order.m_sTargetZoneId,
				order.m_sFactionKey,
				manifest);
			failureStage = settlementFailureStage;
			failureReason = settlementFailureReason;
			if (!unlinked)
			{
				failureStage = failureStage + ".garrison_link_rollback";
				failureReason = failureReason
					+ " | linked manifest rollback was rejected";
			}
			return false;
		}
		group.m_sGarrisonZoneId = order.m_sTargetZoneId;
		group.m_sZoneId = order.m_sTargetZoneId;
		group.m_vTargetPosition = order.m_vTargetPosition;
		group.m_iLifecycleRevision++;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED;
		order.m_iResolvedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		order.m_bOutcomeApplied = true;
		order.m_bAbstractResolved = true;
		order.m_sResolutionKind = "exact_garrison_rebuild_delivered";
		order.m_sRuntimeStatus = "resolved_exact_rebuild_on_station";
		order.m_sFailureReason = "";
		if (operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			group.m_sRuntimeStatus = "enemy_garrison_rebuild_physical_on_station";
		else
			group.m_sRuntimeStatus = "enemy_garrison_rebuild_virtual_on_station";
		return true;
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
		bool delivery,
		string reason,
		out string failureStage,
		out string failureReason)
	{
		failureStage = "";
		failureReason = "";
		if (!state || !enemyDirector || !order || settlementKind.IsEmpty())
		{
			failureStage = "resource_settlement_prerequisite";
			failureReason = "state, enemy director, order, or settlement kind is missing";
			return false;
		}
		string debitFailure = HST_EnemyGarrisonRebuildSaveValidationService
			.ValidateOriginalResourceDebitAuthority(
			state.m_aEnemyStrategicMutations,
			order);
		if (!debitFailure.IsEmpty())
		{
			failureStage = "resource_settlement_original_debit";
			failureReason = debitFailure;
			return false;
		}
		int accepted = Math.Max(0, order.m_iCompositionManpower);
		if (manifest)
			accepted = manifest.m_iAcceptedMemberCount;
		if (accepted <= 0)
		{
			int manifestAccepted;
			if (manifest)
				manifestAccepted = manifest.m_iAcceptedMemberCount;
			failureStage = "resource_settlement_roster";
			failureReason = string.Format(
				"accepted exact roster is invalid | order/manifest %1/%2",
				order.m_iCompositionManpower,
				manifestAccepted);
			return false;
		}
		int survivors = Math.Max(0, Math.Min(accepted, survivorCount));
		int supportRefund;
		if (fullRefund)
			supportRefund = Math.Max(0, order.m_iSupportCost);
		else if (!delivery)
			supportRefund = Math.Max(0, order.m_iSupportCost) * survivors / accepted;
		string settlementId = HST_OperationService.BuildSettlementId(
			order.m_sOperationId,
			settlementKind);
		string refundMutationId = "enemy_resource_refund_" + settlementId;
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);

		if (order.m_bResourceSettlementApplied)
		{
			bool exact = order.m_sResourceSettlementId == settlementId
				&& order.m_sResourceSettlementKind == settlementKind
				&& order.m_iSettlementAcceptedMemberCount == accepted
				&& order.m_iSettlementSurvivorMemberCount == survivors
				&& order.m_iRefundedAttackResources == 0
				&& order.m_iRefundedSupportResources == supportRefund;
			if (!exact)
			{
				failureStage = "resource_settlement_replay_tuple";
				failureReason = string.Format(
					"applied tuple conflicts | id/kind %1/%2 | accepted/survivors %3/%4 | refund %5/%6",
					order.m_sResourceSettlementId,
					order.m_sResourceSettlementKind,
					order.m_iSettlementAcceptedMemberCount,
					order.m_iSettlementSurvivorMemberCount,
					order.m_iRefundedAttackResources,
					order.m_iRefundedSupportResources);
				return false;
			}
			string replayAuthorityFailure = HST_EnemyGarrisonRebuildSaveValidationService
				.ValidateSettledResourceRefundAuthority(
					state.m_aEnemyStrategicMutations,
					order);
			if (!replayAuthorityFailure.IsEmpty())
			{
				failureStage = "resource_settlement_final_authority";
				failureReason = replayAuthorityFailure;
				return false;
			}
			return true;
		}
		bool staged = !order.m_sResourceSettlementId.IsEmpty()
			|| !order.m_sResourceSettlementKind.IsEmpty()
			|| !order.m_sResourceRefundMutationId.IsEmpty();
		if (!staged)
		{
			if (operation)
			{
				if (delivery)
				{
					HST_OperationTransitionResult canRecord
						= m_Operations.CanRecordExactEnemyGarrisonRebuildResourceSettlement(
							state,
							order,
							settlementKind,
							accepted,
							survivors);
					if (!canRecord || !canRecord.m_bAccepted)
					{
						failureStage = "resource_settlement_operation_preflight";
						failureReason = "delivery receipt preflight returned no result";
						if (canRecord && !canRecord.m_sFailureReason.IsEmpty())
							failureReason = canRecord.m_sFailureReason;
						return false;
					}
				}
				else
				{
					HST_OperationTransitionResult prepared
						= m_Operations.PrepareExactEnemyGarrisonRebuildSettlement(
							state,
							order,
							terminalResult,
							settlementId,
							reason);
					if (!prepared || !prepared.m_bAccepted)
					{
						failureStage = "resource_settlement_operation_preflight";
						failureReason = "terminal settlement preflight returned no result";
						if (prepared && !prepared.m_sFailureReason.IsEmpty())
							failureReason = prepared.m_sFailureReason;
						return false;
					}
				}
			}
			order.m_sResourceSettlementId = settlementId;
			order.m_sResourceSettlementKind = settlementKind;
			order.m_sResourceRefundMutationId = refundMutationId;
			order.m_iSettlementAcceptedMemberCount = accepted;
			order.m_iSettlementSurvivorMemberCount = survivors;
			order.m_iRefundedAttackResources = 0;
			order.m_iRefundedSupportResources = supportRefund;
		}
		else if (order.m_sResourceSettlementId != settlementId
			|| order.m_sResourceSettlementKind != settlementKind
			|| order.m_sResourceRefundMutationId != refundMutationId
			|| order.m_iSettlementAcceptedMemberCount != accepted
			|| order.m_iSettlementSurvivorMemberCount != survivors
			|| order.m_iRefundedAttackResources != 0
			|| order.m_iRefundedSupportResources != supportRefund)
		{
			failureStage = "resource_settlement_staged_tuple";
			failureReason = string.Format(
				"staged tuple conflicts | id/kind %1/%2 | accepted/survivors %3/%4 | refund %5/%6",
				order.m_sResourceSettlementId,
				order.m_sResourceSettlementKind,
				order.m_iSettlementAcceptedMemberCount,
				order.m_iSettlementSurvivorMemberCount,
				order.m_iRefundedAttackResources,
				order.m_iRefundedSupportResources);
			return false;
		}

		if (!enemyDirector.RefundDefenseResources(
			state,
			order.m_sFactionKey,
			order.m_sTargetZoneId,
			0,
			supportRefund,
			reason,
			refundMutationId,
			settlementId,
			order.m_sOrderId,
			order.m_sOperationId,
			order.m_sManifestId))
		{
			if (delivery)
				failureStage = "resource_settlement_zero_refund_mutation";
			else
				failureStage = "resource_settlement_refund_mutation";
			failureReason = string.Format(
				"enemy director rejected refund mutation %1 | attack/support %2/%3 | settlement %4",
				refundMutationId,
				0,
				supportRefund,
				settlementId);
			return false;
		}
		if (operation)
		{
			HST_OperationTransitionResult recorded
				= m_Operations.RecordExactEnemyGarrisonRebuildResourceSettlement(
					state,
					order,
					settlementKind,
					accepted,
					survivors);
			if (!recorded || !recorded.m_bAccepted)
			{
				failureStage = "resource_settlement_operation_record";
				failureReason = "operation receipt recording returned no result";
				if (recorded && !recorded.m_sFailureReason.IsEmpty())
					failureReason = recorded.m_sFailureReason;
				return false;
			}
		}
		else
			order.m_bResourceSettlementApplied = true;
		string finalAuthorityFailure = HST_EnemyGarrisonRebuildSaveValidationService
			.ValidateSettledResourceRefundAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		if (!finalAuthorityFailure.IsEmpty())
		{
			failureStage = "resource_settlement_final_authority";
			failureReason = finalAuthorityFailure;
			return false;
		}
		return true;
	}

	protected bool BeginPrearrivalInvalidationReturn(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		string reason)
	{
		if (!state || !order || !operation || !group)
			return false;
		if (operation.m_eDutyState
			== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
			return false;
		HST_OperationTransitionResult returning
			= m_Operations.BeginExactEnemyGarrisonRebuildReturnToOrigin(state, order, group);
		if (!returning || !returning.m_bAccepted)
			return QuarantineRuntimeAuthority(order,
				"exact enemy garrison rebuild return transition failed: " + reason);
		order.m_sRuntimeStatus = "exact_rebuild_invalidated_returning";
		order.m_sFailureReason = reason;
		group.m_sRuntimeStatus = "enemy_garrison_rebuild_returning";
		if (operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
		{
			if (!m_PhysicalWar.RestartExactEnemyGarrisonRebuildInfantryRoute(
				state,
				group,
				operation.m_vRouteEndPosition,
				"Exact enemy garrison rebuild returning after invalidation."))
				return QuarantineRuntimeAuthority(order,
					"exact enemy garrison rebuild live return route failed");
		}
		return true;
	}

	protected bool SettlePrearrivalOperation(
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
		if (!PrepareRuntimeForTerminal(state, order, operation, batch, group))
			return false;
		// Live casualty reconciliation above may have advanced durable slot truth.
		// Always derive the refund roster after that boundary; the caller's value is
		// only an early hint and must never outrank the batch.
		survivors = ResolveLivingRoster(operation, manifest, batch, group);
		bool fullRefund = !order.m_bStrategicServiceCommitted;
		string settlementFailureStage;
		string settlementFailureReason;
		if (!ApplyResourceSettlement(
			state,
			enemyDirector,
			order,
			manifest,
			settlementKind,
			terminalResult,
			survivors,
			fullRefund,
			false,
			reason,
			settlementFailureStage,
			settlementFailureReason))
			return SetRuntimeConflict(order,
				"exact enemy garrison rebuild resource settlement rejected at "
					+ settlementFailureStage + ": " + settlementFailureReason);
		string settlementId = order.m_sResourceSettlementId;
		HST_OperationTransitionResult canSettle
			= m_Operations.CanSettleExactEnemyGarrisonRebuild(
				state,
				order,
				terminalResult,
				settlementId);
		if (!canSettle || !canSettle.m_bAccepted)
			return SetRuntimeConflict(order,
				"exact enemy garrison rebuild terminal preflight failed");
		HST_OperationTransitionResult settled
			= m_Operations.SettleExactEnemyGarrisonRebuild(
				state,
				order,
				terminalResult,
				settlementId,
				reason);
		if (!settled || !settled.m_bAccepted || !settled.m_Operation)
			return SetRuntimeConflict(order,
				"exact enemy garrison rebuild operation settlement failed");
		return FinalizeSettledOrder(state, order, settled.m_Operation) || true;
	}

	protected bool SettleDeliveredOperation(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_EOperationTerminalResult terminalResult,
		string reason)
	{
		if (!state || !order || !operation || !manifest || !IsDelivered(order))
			return false;
		if (!HST_EnemyGarrisonRebuildSaveValidationService.ValidateSettledResourceRefundAuthority(
			state.m_aEnemyStrategicMutations,
			order).IsEmpty())
			return QuarantineRuntimeAuthority(order,
				"exact delivered garrison rebuild receipt is invalid");
		if (!PrepareRuntimeForTerminal(state, order, operation, batch, group))
			return false;
		string settlementId = order.m_sResourceSettlementId;
		HST_OperationTransitionResult prepared
			= m_Operations.PrepareExactEnemyGarrisonRebuildSettlement(
				state,
				order,
				terminalResult,
				settlementId,
				reason);
		if (!prepared || !prepared.m_bAccepted)
			return SetRuntimeConflict(order,
				"exact delivered garrison rebuild settlement prepare failed");
		// PREPARED is the durable terminal intent. Only unlink the held garrison
		// after that token exists so a crash cannot strand a delivered roster with
		// neither held nor terminal authority.
		HST_GarrisonState garrison = state.FindGarrison(
			order.m_sTargetZoneId,
			order.m_sFactionKey);
		if (garrison && garrison.m_aAcceptedManifestIds.Contains(manifest.m_sManifestId))
			m_Garrisons.UnlinkExactEnemyGarrisonRebuildManifest(
				state,
				order.m_sTargetZoneId,
				order.m_sFactionKey,
				manifest);
		HST_OperationTransitionResult settled
			= m_Operations.SettleExactEnemyGarrisonRebuild(
				state,
				order,
				terminalResult,
				settlementId,
				reason);
		if (!settled || !settled.m_bAccepted || !settled.m_Operation)
			return SetRuntimeConflict(order,
				"exact delivered garrison rebuild settlement failed");
		return FinalizeSettledOrder(state, order, settled.m_Operation) || true;
	}

	protected bool ResumePreparedSettlement(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		if (!state || !enemyDirector || !order || !operation)
			return false;
		if (operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (!order.m_bResourceSettlementApplied)
				return QuarantineRuntimeAuthority(order,
					"settled exact enemy garrison rebuild lacks its resource receipt");
			return FinalizeSettledOrder(state, order, operation);
		}
		HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(order.m_sSpawnResultId);
		HST_ActiveGroupState group = state.FindActiveGroup(order.m_sGroupId);
		if (!order.m_bResourceSettlementApplied)
		{
			string kind = order.m_sResourceSettlementKind;
			bool fullRefund = HST_EnemyGarrisonRebuildSaveValidationService
				.IsFullRefundSettlementKind(kind);
			string settlementFailureStage;
			string settlementFailureReason;
			if (!ApplyResourceSettlement(
				state,
				enemyDirector,
				order,
				manifest,
				kind,
				operation.m_eTerminalResult,
				order.m_iSettlementSurvivorMemberCount,
				fullRefund,
				false,
				operation.m_sTerminalReason,
				settlementFailureStage,
				settlementFailureReason))
				return SetRuntimeConflict(order,
					"prepared exact enemy garrison rebuild resource settlement rejected at "
						+ settlementFailureStage + ": " + settlementFailureReason);
		}
		HST_OperationTransitionResult settled
			= m_Operations.SettleExactEnemyGarrisonRebuild(
				state,
				order,
				operation.m_eTerminalResult,
				operation.m_sSettlementId,
				operation.m_sTerminalReason);
		if (!settled || !settled.m_bAccepted || !settled.m_Operation)
			return SetRuntimeConflict(order,
				"prepared exact enemy garrison rebuild finalization is pending");
		if (order.m_sResourceSettlementKind == DELIVERY_KIND && manifest)
		{
			HST_GarrisonState garrison = state.FindGarrison(
				order.m_sTargetZoneId,
				order.m_sFactionKey);
			if (garrison && garrison.m_aAcceptedManifestIds.Contains(manifest.m_sManifestId))
				m_Garrisons.UnlinkExactEnemyGarrisonRebuildManifest(
					state,
					order.m_sTargetZoneId,
					order.m_sFactionKey,
					manifest);
		}
		return FinalizeSettledOrder(state, order, settled.m_Operation) || true;
	}

	protected bool PrepareRuntimeForTerminal(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!state || !order || !operation)
			return false;
		if (!batch || !group)
			return true;
		bool transitionLive = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING
			|| operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		int handles = m_SpawnAdapter.CountHandlesForProjection(group.m_sProjectionId);
		bool runtime = group.m_bSpawnedEntity || handles > 0
			|| m_PhysicalWar.GetForceSpawnGroupRoot(group)
			|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0;
		if (!transitionLive && !runtime)
			return true;
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			HST_ForceSpawnAdapterTickResult reconciled
				= m_SpawnAdapter.ReconcileExactInfantryProjectionAuthority(
					state,
					m_SpawnQueue,
					m_PhysicalWar,
					Math.Max(0, state.m_iElapsedSeconds),
					operation.m_sProjectionId);
			if (!reconciled || reconciled.m_iFailedCount > 0)
				return SetRuntimeConflict(order,
					"exact enemy garrison rebuild terminal casualty reconciliation is pending");
		}
		if (runtime)
		{
			HST_ForceSpawnAdapterRetireResult retired
				= m_SpawnAdapter.RetireProjectionRuntime(
					state,
					m_PhysicalWar,
					group.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return SetRuntimeConflict(order,
					"exact enemy garrison rebuild terminal runtime retirement is pending");
		}
		group.m_bSpawnedEntity = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		SyncGroupRoster(group, ResolveLivingRoster(operation, state.FindForceManifest(
			order.m_sManifestId), batch, group));
		return true;
	}

	protected bool FinalizeSettledOrder(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		if (!state || !order || !operation
			|| operation.m_eSettlementState
				!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return false;
		string receiptFailure = HST_EnemyGarrisonRebuildSaveValidationService
			.ValidateSettledResourceRefundAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		string policyFailure = HST_EnemyGarrisonRebuildSaveValidationService
			.ValidateTerminalResultPolicy(order, operation);
		if (!receiptFailure.IsEmpty() || !policyFailure.IsEmpty()
			|| operation.m_sSettlementId != order.m_sResourceSettlementId
			|| operation.m_sTerminalReason.IsEmpty())
		{
			string failure = receiptFailure;
			if (failure.IsEmpty())
				failure = policyFailure;
			if (failure.IsEmpty())
				failure = "settled exact enemy garrison rebuild lifecycle authority conflicts";
			return QuarantineRuntimeAuthority(order, failure);
		}
		bool changed = FinalizeSettledOrderLifecycle(order, operation);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(order.m_sSpawnResultId);
		HST_ActiveGroupState group = state.FindActiveGroup(order.m_sGroupId);
		bool runtimeReleased = true;
		if (batch && m_SpawnAdapter.CountHandlesForProjection(batch.m_sProjectionId) > 0)
			runtimeReleased = false;
		if (group && (m_PhysicalWar.GetForceSpawnGroupRoot(group)
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

	protected bool FinalizeSettledOrderLifecycle(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		if (!order || !operation)
			return false;
		bool delivered = order.m_sResourceSettlementKind == DELIVERY_KIND;
		HST_EEnemyOrderStatus expectedStatus
			= HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		if (delivered || operation.m_eTerminalResult
			== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED)
			expectedStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED;
		string expectedResolutionKind = order.m_sResourceSettlementKind;
		if (delivered)
			expectedResolutionKind = "exact_garrison_rebuild_delivered";
		int expectedResolvedSecond = Math.Max(0, operation.m_iSettledAtSecond);
		bool changed;
		if (order.m_eStatus != expectedStatus)
		{
			order.m_eStatus = expectedStatus;
			changed = true;
		}
		if (order.m_iResolvedAtSecond != expectedResolvedSecond)
		{
			order.m_iResolvedAtSecond = expectedResolvedSecond;
			changed = true;
		}
		if (order.m_sResolutionKind != expectedResolutionKind)
		{
			order.m_sResolutionKind = expectedResolutionKind;
			changed = true;
		}
		if (order.m_sFailureReason != operation.m_sTerminalReason)
		{
			order.m_sFailureReason = operation.m_sTerminalReason;
			changed = true;
		}
		if (order.m_sRuntimeStatus != "resolved_exact_rebuild_terminal")
		{
			order.m_sRuntimeStatus = "resolved_exact_rebuild_terminal";
			changed = true;
		}
		if (order.m_bPhysicalized)
		{
			order.m_bPhysicalized = false;
			changed = true;
		}
		if (order.m_bAbstractResolved)
		{
			order.m_bAbstractResolved = false;
			changed = true;
		}
		if (order.m_bOutcomeApplied != delivered)
		{
			order.m_bOutcomeApplied = delivered;
			changed = true;
		}
		return changed;
	}

	protected int ResolveLivingRoster(
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
		int living;
		if (batch && m_SpawnQueue)
		{
			if (batch.m_bStrategicProjectionHeld)
				living = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
			else if (batch.m_eStatus
				== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
				|| batch.m_iSuccessfulHandoffCount > 0)
				living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
			else
				living = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
			return Math.Max(0, Math.Min(accepted, living));
		}
		if (operation)
			living = Math.Max(0, operation.m_iLastVirtualFriendlyCount);
		if (living <= 0 && group && group.m_sRuntimeStatus != "eliminated")
			living = Math.Max(group.m_iDurableLivingInfantryCount,
				group.m_iSurvivorInfantryCount);
		return Math.Max(0, Math.Min(accepted, living));
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

	protected bool IsDelivered(HST_EnemyOrderState order)
	{
		return order && order.m_bResourceSettlementApplied
			&& order.m_sResourceSettlementKind == DELIVERY_KIND
			&& order.m_bOutcomeApplied;
	}

	protected bool TargetOwnershipMatches(HST_CampaignState state, HST_EnemyOrderState order)
	{
		if (!state || !order)
			return false;
		HST_ZoneState target = state.FindZone(order.m_sTargetZoneId);
		return target && target.m_sOwnerFactionKey == order.m_sFactionKey
			&& target.m_iOwnershipRevision == order.m_iTargetOwnershipRevision;
	}

	protected bool SourceOwnershipMatches(HST_CampaignState state, HST_EnemyOrderState order)
	{
		if (!state || !order)
			return false;
		HST_ZoneState source = state.FindZone(order.m_sSourceZoneId);
		return source && source.m_sOwnerFactionKey == order.m_sFactionKey;
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
		HST_ForceSpawnResultState batch = state.FindForceSpawnResultByRequest(order.m_sOrderId);
		if (batch)
		{
			if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
				m_SpawnQueue.RequestCancel(
					state.m_aForceSpawnResults,
					batch.m_sResultId,
					state.m_iElapsedSeconds,
					reason);
			int batchIndex = state.m_aForceSpawnResults.Find(batch);
			if (batchIndex >= 0)
				state.m_aForceSpawnResults.Remove(batchIndex);
		}
		HST_ActiveGroupState group = state.FindActiveGroup(BuildProjectionId(order));
		if (group && !group.m_bSpawnedEntity)
		{
			int groupIndex = state.m_aActiveGroups.Find(group);
			if (groupIndex >= 0)
				state.m_aActiveGroups.Remove(groupIndex);
		}
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		if (operation && manifest)
			m_Operations.RemoveUncommittedExactEnemyGarrisonRebuild(state, order, manifest);
		operation = state.FindOperation(order.m_sOperationId);
		if (operation)
		{
			int operationIndex = state.m_aOperations.Find(operation);
			if (operationIndex >= 0)
				state.m_aOperations.Remove(operationIndex);
		}
		HST_ForceManifestState storedManifest = state.FindForceManifest(order.m_sManifestId);
		if (storedManifest)
		{
			int manifestIndex = state.m_aForceManifests.Find(storedManifest);
			if (manifestIndex >= 0)
				state.m_aForceManifests.Remove(manifestIndex);
		}
		order.m_sSpawnResultId = "";
		order.m_sGroupId = "";
		order.m_bStrategicServiceCommitted = false;
		int accepted = Math.Max(0, order.m_iCompositionManpower);
		if (manifest)
			accepted = manifest.m_iAcceptedMemberCount;
		string settlementFailureStage;
		string settlementFailureReason;
		bool refunded = ApplyResourceSettlement(
			state,
			enemyDirector,
			order,
			manifest,
			"admission_failed_full",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
			accepted,
			true,
			false,
			reason,
			settlementFailureStage,
			settlementFailureReason);
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		order.m_iResolvedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		order.m_sRuntimeStatus = "resolved_exact_admission_failed";
		order.m_sResolutionKind = "admission_failed_full";
		order.m_sFailureReason = reason;
		if (!refunded)
		{
			order.m_sRuntimeStatus = "exact_admission_settlement_conflict";
			order.m_sFailureReason = reason + " | resource settlement rejected at "
				+ settlementFailureStage + ": " + settlementFailureReason;
		}
	}

	bool QuarantineUnsupportedGarrisonRebuildAuthority(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		string reason)
	{
		if (!state || !order)
			return false;
		if (reason.IsEmpty())
			reason = "unsupported exact enemy garrison rebuild contract";
		return QuarantineRuntimeAuthority(order, reason);
	}

	protected bool QuarantineRuntimeAuthority(HST_EnemyOrderState order, string reason)
	{
		if (!order)
			return false;
		string prefix
			= "exact enemy garrison rebuild authority is quarantined; no guessed cleanup or refund was attempted: ";
		string failure = reason;
		if (!failure.StartsWith(prefix))
			failure = prefix + failure;
		bool changed = order.m_sRuntimeStatus != QUARANTINE_STATUS
			|| order.m_sFailureReason != failure
			|| order.m_iOperationContractVersion != QUARANTINED_CONTRACT_VERSION
			|| order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			|| order.m_bPhysicalized;
		order.m_iOperationContractVersion = QUARANTINED_CONTRACT_VERSION;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		order.m_bPhysicalized = false;
		order.m_sRuntimeStatus = QUARANTINE_STATUS;
		order.m_sFailureReason = failure;
		return changed;
	}

	protected bool SetRuntimeConflict(HST_EnemyOrderState order, string reason)
	{
		if (!order)
			return false;
		if (order.m_sRuntimeStatus == "exact_rebuild_runtime_conflict"
			&& order.m_sFailureReason == reason)
			return false;
		order.m_sRuntimeStatus = "exact_rebuild_runtime_conflict";
		order.m_sFailureReason = reason;
		return true;
	}

	protected bool SetDeliveryRuntimeConflict(
		HST_EnemyOrderState order,
		string failureStage,
		string failureReason)
	{
		RecordDeliveryFailure(order, failureStage, failureReason);
		return SetRuntimeConflict(
			order,
			"exact enemy garrison rebuild delivery rejected at "
				+ BuildDeliveryFailureSummary(failureStage, failureReason));
	}

	protected void RecordDeliveryFailure(
		HST_EnemyOrderState order,
		string failureStage,
		string failureReason)
	{
		string key = ResolveDeliveryDiagnosticKey(order);
		if (key.IsEmpty())
			return;
		EnsureDeliveryDiagnosticCapacity(key);
		string summary = BuildDeliveryFailureSummary(failureStage, failureReason);
		if (!m_mFirstDeliveryFailureByOperation.Contains(key))
			m_mFirstDeliveryFailureByOperation.Set(key, summary);
		m_mLastDeliveryFailureByOperation.Set(key, summary);
	}

	protected void EnsureDeliveryDiagnosticCapacity(string incomingKey)
	{
		if (m_mFirstDeliveryFailureByOperation.Contains(incomingKey)
			|| m_mFirstDeliveryFailureByOperation.Count() < MAX_DELIVERY_DIAGNOSTIC_OPERATIONS)
			return;

		string discardKey;
		foreach (string existingKey, string existingValue : m_mFirstDeliveryFailureByOperation)
		{
			discardKey = existingKey;
			break;
		}
		if (discardKey.IsEmpty())
			return;

		m_mFirstDeliveryFailureByOperation.Remove(discardKey);
		m_mLastDeliveryFailureByOperation.Remove(discardKey);
	}

	protected string ResolveDeliveryDiagnosticKey(HST_EnemyOrderState order)
	{
		if (!order)
			return "";
		if (!order.m_sOperationId.IsEmpty())
			return order.m_sOperationId;
		return order.m_sOrderId;
	}

	protected string BuildDeliveryFailureSummary(
		string failureStage,
		string failureReason)
	{
		if (failureStage.IsEmpty())
			failureStage = "unknown_delivery_stage";
		if (failureReason.IsEmpty())
			failureReason = "delivery rejection returned no reason";
		return failureStage + ": " + failureReason;
	}

	protected string BuildGarrisonLinkFailureReason(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		int living)
	{
		return "garrison link rejected | "
			+ BuildGarrisonManifestLinkEvidence(order, manifest, living)
			+ " | " + BuildGarrisonCapacityLinkEvidence(state, order, living);
	}

	protected string BuildGarrisonManifestLinkEvidence(
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		int living)
	{
		if (!manifest)
			return "manifest missing | living " + living.ToString();
		bool factionExact;
		bool targetExact;
		if (order)
		{
			factionExact = manifest.m_sFactionKey == order.m_sFactionKey;
			targetExact = manifest.m_sTargetZoneId == order.m_sTargetZoneId;
		}
		return string.Format(
			"manifest frozen/policy/force/intent/faction/target %1/%2/%3/%4/%5/%6",
			manifest.m_bFrozen,
			manifest.m_sPolicyId
				== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_POLICY_ID,
			manifest.m_sForceKind
				== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_FORCE_KIND,
			manifest.m_sIntentId
				== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_MANIFEST_INTENT,
			factionExact,
			targetExact)
			+ string.Format(
				" | living/accepted %1/%2",
				living,
				manifest.m_iAcceptedMemberCount);
	}

	protected string BuildGarrisonCapacityLinkEvidence(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		int living)
	{
		HST_ZoneState target;
		HST_GarrisonState garrison;
		if (state && order)
		{
			target = state.FindZone(order.m_sTargetZoneId);
			garrison = state.FindGarrison(
				order.m_sTargetZoneId,
				order.m_sFactionKey);
		}
		int aggregateInfantry;
		int exactInfantry;
		int activeInfantry;
		int slots;
		if (garrison)
		{
			aggregateInfantry = Math.Max(0, garrison.m_iInfantryCount);
			exactInfantry = m_Garrisons.CountExecutableManifestInfantry(state, garrison);
		}
		if (target)
		{
			activeInfantry = Math.Max(0, target.m_iActiveInfantryCount);
			slots = target.m_iGarrisonSlots;
		}
		string owner = "missing";
		int ownerRevision;
		string frozenOwner = "missing";
		int frozenRevision;
		if (target)
		{
			owner = target.m_sOwnerFactionKey;
			ownerRevision = target.m_iOwnershipRevision;
		}
		if (order)
		{
			frozenOwner = order.m_sFactionKey;
			frozenRevision = order.m_iTargetOwnershipRevision;
		}
		string result = string.Format(
			"aggregate/exact/active/projected/slots %1/%2/%3/%4/%5",
			aggregateInfantry,
			exactInfantry,
			activeInfantry,
			aggregateInfantry + exactInfantry + activeInfantry + Math.Max(0, living),
			slots);
		return result + string.Format(
			" | owner/revision current %1/%2 frozen %3/%4",
			owner,
			ownerRevision,
			frozenOwner,
			frozenRevision);
	}

	bool ReconcileAfterRestore(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector)
	{
		if (!state || !enemyDirector)
			return false;
		m_EnemyDirector = enemyDirector;
		bool changed;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order || order.m_eType
				!= HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON
				|| order.m_iOperationContractVersion == 0)
				continue;
			if (order.m_iOperationContractVersion == QUARANTINED_CONTRACT_VERSION)
			{
				changed = QuarantineRuntimeAuthority(order, order.m_sFailureReason) || changed;
				continue;
			}
			if (!IsExactEnemyGarrisonRebuild(order))
			{
				changed = QuarantineRuntimeAuthority(order,
					"restore found an unsupported garrison rebuild contract") || changed;
				continue;
			}
			HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
			if (HasPreparedSettlementResumeCandidate(order, operation))
			{
				changed = ResumePreparedSettlement(state, enemyDirector, order, operation) || changed;
				continue;
			}
			if (operation && operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			{
				changed = FinalizeSettledOrder(state, order, operation) || changed;
				continue;
			}
			if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				&& !HasCommittedAdmissionAuthority(state, order))
			{
				FailAdmissionAfterDebit(
					state,
					order,
					state.FindForceManifest(order.m_sManifestId),
					enemyDirector,
					"restore found an interrupted exact garrison rebuild admission");
				changed = true;
				continue;
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
			{
				changed = QuarantineRuntimeAuthority(order, failure) || changed;
				continue;
			}
			if (batch.m_bStrategicProjectionHeld)
			{
				SyncGroupRoster(group, m_SpawnQueue.CountStrategicLivingMemberSlots(batch));
				group.m_bSpawnedEntity = false;
				group.m_sRuntimeEntityId = "";
				group.m_iSpawnedAgentCount = 0;
				if (operation.m_eMaterializationState
					!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL)
				{
					operation.m_eMaterializationState
						= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
					operation.m_ePositionAuthority
						= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
					operation.m_iRevision++;
				}
			}
			if (IsDelivered(order))
			{
				HST_GarrisonState garrison = state.FindGarrison(
					order.m_sTargetZoneId,
					order.m_sFactionKey);
				string ownershipFailure;
				if (!garrison || !ValidateAcceptedManifestOwnershipAuthority(
					state,
					garrison,
					manifest,
					ownershipFailure))
				{
					changed = QuarantineRuntimeAuthority(order, ownershipFailure) || changed;
					continue;
				}
				group.m_sGarrisonZoneId = order.m_sTargetZoneId;
				order.m_sRuntimeStatus = "resolved_exact_rebuild_virtual_on_station";
				group.m_sRuntimeStatus = "enemy_garrison_rebuild_virtual_on_station";
			}
			else
			{
				order.m_sRuntimeStatus = "exact_rebuild_restore_virtual";
				group.m_sRuntimeStatus = "enemy_garrison_rebuild_virtual";
			}
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
			if (!IsExactEnemyGarrisonRebuild(order))
				continue;
			HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
			if (operation && operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				changed = FinalizeSettledOrder(state, order, operation) || changed;
		}
		return changed;
	}

	bool SettleTrackedOpenOrderForAdministrativeStop(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		string reason)
	{
		if (!state || !enemyDirector || !IsExactEnemyGarrisonRebuild(order)
			|| !m_SpawnAdapter || !m_PhysicalWar)
			return false;
		if (reason.IsEmpty())
			reason = "administrative stop cancelled exact enemy garrison rebuild";
		m_EnemyDirector = enemyDirector;
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			ReconcileSettledRuntimeCleanup(state);
			return HST_EnemyGarrisonRebuildSaveValidationService
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
		if (!failure.IsEmpty() || IsDelivered(order))
			return false;
		int living = ResolveLivingRoster(operation, manifest, batch, group);
		SettlePrearrivalOperation(
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
			reason);
		ReconcileSettledRuntimeCleanup(state);
		operation = state.FindOperation(order.m_sOperationId);
		return operation
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& HST_EnemyGarrisonRebuildSaveValidationService
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
		string forceId = BuildForceId(order);
		string projectionId = BuildProjectionId(order);
		if ((!order.m_sSpawnResultId.IsEmpty() && order.m_sSpawnResultId != resultId)
			|| (!order.m_sGroupId.IsEmpty() && order.m_sGroupId != projectionId)
			|| (!operation.m_sSpawnResultId.IsEmpty() && operation.m_sSpawnResultId != resultId)
			|| (!operation.m_sForceId.IsEmpty() && operation.m_sForceId != forceId)
			|| (!operation.m_sProjectionId.IsEmpty() && operation.m_sProjectionId != projectionId)
			|| (!operation.m_sGroupId.IsEmpty() && operation.m_sGroupId != projectionId))
			return false;

		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && (batch.m_sResultId == resultId
				|| batch.m_sRequestId == order.m_sOrderId
				|| batch.m_sOperationId == order.m_sOperationId
				|| batch.m_sManifestId == order.m_sManifestId
				|| batch.m_sForceId == forceId
				|| batch.m_sProjectionId == projectionId))
				return false;
		}
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && (group.m_sGroupId == projectionId
				|| group.m_sEnemyOrderId == order.m_sOrderId
				|| group.m_sOperationId == order.m_sOperationId
				|| group.m_sManifestId == order.m_sManifestId
				|| group.m_sSpawnResultId == resultId
				|| group.m_sForceId == forceId
				|| group.m_sProjectionId == projectionId))
				return false;
		}

		HST_ActiveGroupState groupProbe = new HST_ActiveGroupState();
		groupProbe.m_sGroupId = projectionId;
		if (m_SpawnAdapter.CountHandlesForProjection(projectionId) > 0
			|| m_SpawnAdapter.CountHandlesForResultId(resultId) > 0
			|| m_PhysicalWar.GetForceSpawnGroupRoot(groupProbe)
			|| m_PhysicalWar.CountForceSpawnRuntimeMembers(groupProbe) > 0)
			return false;
		return true;
	}

	bool SettleOpenOrdersForCampaignStop(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		string reason)
	{
		if (!state || !enemyDirector)
			return false;
		m_EnemyDirector = enemyDirector;
		if (reason.IsEmpty())
			reason = "campaign phase no longer permits exact enemy garrison rebuild authority";
		bool changed;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsExactEnemyGarrisonRebuild(order))
				continue;
			HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
			if (!operation || operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				continue;
			if (HasPreparedSettlementResumeCandidate(order, operation))
			{
				changed = ResumePreparedSettlement(state, enemyDirector, order, operation) || changed;
				continue;
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
			{
				changed = QuarantineRuntimeAuthority(order, failure) || changed;
				continue;
			}
			if (IsDelivered(order))
				changed = SettleDeliveredOperation(state, order, operation, manifest, batch, group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED, reason) || changed;
			else
			{
				int living = ResolveLivingRoster(operation, manifest, batch, group);
				changed = SettlePrearrivalOperation(state, enemyDirector, order, operation, manifest,
					batch, group, HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED,
					"campaign_stopped_survivors", living, reason) || changed;
			}
		}
		return changed;
	}

	bool CanReconcileZoneOwnershipChange(
		HST_CampaignState state,
		string zoneId,
		string newOwnerFactionKey,
		out string failureReason)
	{
		failureReason = "";
		if (!state || zoneId.IsEmpty() || newOwnerFactionKey.IsEmpty()
			|| !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar || !m_Garrisons
			|| !m_EnemyDirector)
		{
			failureReason = "exact enemy garrison rebuild ownership preflight services are unavailable";
			return false;
		}
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!OrderClaimsOwnershipZone(order, zoneId)
				|| order.m_sFactionKey == newOwnerFactionKey)
				continue;
			if (order.m_iOperationContractVersion == QUARANTINED_CONTRACT_VERSION)
			{
				failureReason = "quarantined exact garrison rebuild blocks ownership preflight";
				return false;
			}
			if (!IsExactEnemyGarrisonRebuild(order))
			{
				failureReason = "unsupported exact garrison rebuild blocks ownership preflight";
				return false;
			}
			HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
			if (operation && operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				continue;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			string contextFailure = ResolveRuntimeContext(
				state,
				order,
				operation,
				manifest,
				batch,
				group);
			if (!contextFailure.IsEmpty())
			{
				failureReason = "exact garrison rebuild ownership preflight failed: " + contextFailure;
				return false;
			}
		}
		return true;
	}

	bool ReconcileZoneOwnershipChange(
		HST_CampaignState state,
		string zoneId,
		string newOwnerFactionKey,
		out bool stateChanged,
		out string failureReason)
	{
		stateChanged = false;
		if (!CanReconcileZoneOwnershipChange(
			state,
			zoneId,
			newOwnerFactionKey,
			failureReason))
			return false;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!OrderClaimsOwnershipZone(order, zoneId)
				|| order.m_sFactionKey == newOwnerFactionKey)
				continue;
			HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
			if (!operation || operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			{
				if (operation)
					stateChanged = FinalizeSettledOrder(state, order, operation) || stateChanged;
				continue;
			}
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			string contextFailure = ResolveRuntimeContext(
				state,
				order,
				operation,
				manifest,
				batch,
				group);
			if (!contextFailure.IsEmpty())
			{
				failureReason = contextFailure;
				return false;
			}
			bool settled;
			if (IsDelivered(order))
			{
				settled = SettleDeliveredOperation(
					state,
					order,
					operation,
					manifest,
					batch,
					group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
					"exact delivered garrison rebuild ended by canonical ownership transition");
			}
			else
			{
				int living = ResolveLivingRoster(operation, manifest, batch, group);
				settled = SettlePrearrivalOperation(
					state,
					m_EnemyDirector,
					order,
					operation,
					manifest,
					batch,
					group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
					"ownership_invalidated_survivors",
					living,
					"exact enemy garrison rebuild ended by canonical ownership transition");
			}
			if (!settled)
			{
				failureReason = "exact enemy garrison rebuild ownership settlement is pending";
				return false;
			}
			stateChanged = true;
		}
		return true;
	}

	bool ValidateAcceptedManifestOwnershipAuthority(
		HST_CampaignState state,
		HST_GarrisonState garrison,
		HST_ForceManifestState manifest,
		out string failureReason)
	{
		failureReason = "";
		if (!state || !garrison || !manifest
			|| manifest.m_sPolicyId
				!= HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_POLICY_ID)
		{
			failureReason = "accepted exact enemy garrison rebuild authority is incomplete";
			return false;
		}
		if (manifest.m_sTargetZoneId != garrison.m_sZoneId
			|| manifest.m_sFactionKey != garrison.m_sFactionKey)
		{
			failureReason = "accepted exact enemy garrison rebuild conflicts with its garrison";
			return false;
		}
		int backlinkCount;
		foreach (string acceptedManifestId : garrison.m_aAcceptedManifestIds)
		{
			if (acceptedManifestId == manifest.m_sManifestId)
				backlinkCount++;
		}
		if (backlinkCount != 1)
		{
			failureReason = "accepted exact enemy garrison rebuild backlink is not unique";
			return false;
		}
		HST_OperationRecordState operation;
		HST_EnemyOrderState order;
		int operationClaims;
		int orderClaims;
		foreach (HST_OperationRecordState candidateOperation : state.m_aOperations)
		{
			if (candidateOperation && (candidateOperation.m_sOperationId == manifest.m_sOperationId
				|| candidateOperation.m_sManifestId == manifest.m_sManifestId))
			{
				operation = candidateOperation;
				operationClaims++;
			}
		}
		foreach (HST_EnemyOrderState candidateOrder : state.m_aEnemyOrders)
		{
			if (candidateOrder && (candidateOrder.m_sOperationId == manifest.m_sOperationId
				|| candidateOrder.m_sManifestId == manifest.m_sManifestId))
			{
				order = candidateOrder;
				orderClaims++;
			}
		}
		if (operationClaims != 1 || orderClaims != 1 || !operation || !order
			|| !IsExactEnemyGarrisonRebuild(order)
			|| operation.m_eType
				!= HST_EOperationType.HST_OPERATION_TYPE_ENEMY_GARRISON_REBUILD
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| operation.m_eSettlementState
				!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			|| order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED
			|| !IsDelivered(order))
		{
			failureReason = "accepted exact enemy garrison rebuild lacks one open delivered reciprocal aggregate";
			return false;
		}
		return true;
	}

	protected bool OrderClaimsOwnershipZone(HST_EnemyOrderState order, string zoneId)
	{
		if (!order || order.m_eType
			!= HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON)
			return false;
		return HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
			order.m_sSourceZoneId,
			zoneId)
			|| HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
				order.m_sTargetZoneId,
				zoneId);
	}

	int ResolveReservedInfantry(HST_CampaignState state, HST_EnemyOrderState order)
	{
		if (!state || !IsExactEnemyGarrisonRebuild(order))
			return 0;
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return 0;
		HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(order.m_sSpawnResultId);
		HST_ActiveGroupState group = state.FindActiveGroup(order.m_sGroupId);
		return ResolveLivingRoster(operation, manifest, batch, group);
	}

	int ResolveAuthoritativeGarrisonInfantry(
		HST_CampaignState state,
		string factionKey,
		string targetZoneId)
	{
		if (!state || !m_Garrisons || factionKey.IsEmpty() || targetZoneId.IsEmpty())
			return 0;
		HST_GarrisonState garrison = state.FindGarrison(targetZoneId, factionKey);
		int total;
		if (garrison)
		{
			total = Math.Max(0, garrison.m_iInfantryCount);
			total += m_Garrisons.CountExecutableManifestInfantry(state, garrison);
		}
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsExactEnemyGarrisonRebuild(order)
				|| order.m_sFactionKey != factionKey
				|| !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					order.m_sTargetZoneId,
					targetZoneId))
				continue;
			if (garrison && garrison.m_aAcceptedManifestIds.Contains(order.m_sManifestId))
				continue;
			total += ResolveReservedInfantry(state, order);
		}
		return total;
	}
}
