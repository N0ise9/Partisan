class HST_GarrisonPatrolAdmissionResult
{
	bool m_bSuccess;
	bool m_bAlreadyApplied;
	bool m_bStateChanged;
	string m_sFailureReason;
	ref HST_GarrisonState m_Garrison;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
	ref HST_GeneratedRouteState m_Route;
}

// Typed admission authority for newly issued, purchased garrison patrols.
// Historical policy-v1 purchases remain aggregate-only legacy garrisons.
class HST_GarrisonPatrolOperationService
{
	static const int EXACT_CONTRACT_VERSION = 1;
	static const int QUARANTINED_CONTRACT_VERSION = -54;
	static const int EXACT_PROJECTION_CONTRACT_VERSION = 1;
	static const string EXACT_POLICY_ID = "garrison_exact_patrol_2";
	static const string LEGACY_POLICY_ID = "garrison_exact_all_or_nothing_1";
	static const string EXACT_FORCE_KIND = "strategic_garrison";
	static const string EXACT_INTENT_ID = "garrison_recruitment";
	static const string EXACT_GROUP_MODE = "exact_garrison_patrol";
	static const string ASSIGNMENT_KIND = "garrison_patrol";
	static const string RECALL_POLICY_ID = "remain_assigned_to_garrison";
	static const string SETTLEMENT_POLICY_ID = "exact_garrison_patrol_roster";
	static const string SETTLEMENT_KIND = "exact_garrison_patrol_terminal";
	static const int EXACT_PRIORITY = 65;
	static const int EXACT_MAX_RETRIES = 3;
	static const int DEPLOYMENT_GRACE_SECONDS = 180;
	static const int CASUALTY_CLEAR_SECONDS = 30;
	static const int LOOP_COUNTER_WRAP_LAPS = 1000000;
	static const float PHYSICAL_ARRIVAL_RADIUS_METERS = 35.0;

	protected ref HST_ForcePlanningIntegrityService m_Integrity = new HST_ForcePlanningIntegrityService();
	protected ref HST_OperationRouteCursorService m_RouteCursor = new HST_OperationRouteCursorService();
	protected ref HST_MaterializationService m_Materialization = new HST_MaterializationService();
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

	bool IsCurrentPolicyQuote(HST_ForceQuoteState quote)
	{
		return quote && quote.m_sQuoteKind == HST_ForcePlanningService.QUOTE_KIND_GARRISON
			&& quote.m_sPolicyId == EXACT_POLICY_ID;
	}

	bool IsCurrentPolicyManifest(HST_ForceManifestState manifest)
	{
		return manifest && manifest.m_sPolicyId == EXACT_POLICY_ID
			&& manifest.m_sForceKind == EXACT_FORCE_KIND
			&& manifest.m_sIntentId == EXACT_INTENT_ID;
	}

	HST_GarrisonPatrolAdmissionResult CanAdmitPreparedPurchase(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_GarrisonService garrisons,
		string confirmationRequestId)
	{
		HST_GarrisonPatrolAdmissionResult result = new HST_GarrisonPatrolAdmissionResult();
		string failure = ValidateAdmissionContext(
			state,
			quote,
			manifest,
			garrisons,
			confirmationRequestId);
		if (!failure.IsEmpty())
		{
			result.m_sFailureReason = failure;
			return result;
		}
		failure = FindAdmissionIdentityCollision(state, quote, manifest);
		if (!failure.IsEmpty())
		{
			result.m_sFailureReason = failure;
			return result;
		}

		HST_ForceSpawnQueueRequest request = BuildSpawnRequest(state, quote);
		HST_ForceSpawnQueueEnqueueResult queuePreflight = m_SpawnQueue.CanEnqueue(
			state.m_aForceSpawnResults,
			manifest,
			request,
			state.m_iElapsedSeconds);
		if (!queuePreflight || !queuePreflight.m_bSuccess || queuePreflight.m_bAlreadyApplied)
		{
			result.m_sFailureReason = "exact garrison patrol spawn admission preflight failed";
			if (queuePreflight && !queuePreflight.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = result.m_sFailureReason + ": " + queuePreflight.m_sFailureReason;
			return result;
		}
		result.m_bSuccess = true;
		return result;
	}

	HST_GarrisonPatrolAdmissionResult AdmitPreparedPurchase(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_GarrisonService garrisons,
		string confirmationRequestId)
	{
		if (HasCommittedAdmissionAuthority(state, quote, manifest))
			return ResolveCommittedAdmission(state, quote, manifest, garrisons);

		HST_GarrisonPatrolAdmissionResult result = new HST_GarrisonPatrolAdmissionResult();
		HST_GarrisonPatrolAdmissionResult preflight = CanAdmitPreparedPurchase(
			state,
			quote,
			manifest,
			garrisons,
			confirmationRequestId);
		if (!preflight || !preflight.m_bSuccess)
		{
			result.m_sFailureReason = "exact garrison patrol admission preflight failed";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = preflight.m_sFailureReason;
			return result;
		}

		HST_ZoneState zone = state.FindZone(quote.m_sTargetZoneId);
		HST_OperationRecordState operation = BuildOperation(
			state,
			quote,
			manifest,
			zone,
			confirmationRequestId);
		HST_ActiveGroupState group = BuildActiveGroup(state, quote, manifest, zone);
		HST_GeneratedRouteState route = BuildLocalPatrolRoute(quote, manifest, zone);
		if (!operation || !group || !route)
		{
			result.m_sFailureReason = "exact garrison patrol authority rows could not be built";
			return result;
		}
		state.m_aOperations.Insert(operation);
		state.m_aActiveGroups.Insert(group);
		state.m_aGeneratedRoutes.Insert(route);
		result.m_Operation = operation;
		result.m_Group = group;
		result.m_Route = route;
		result.m_bStateChanged = true;
		if (!m_RouteCursor.FreezePatrolRoute(state, operation, route, group)
			|| operation.m_sCurrentRouteId.IsEmpty()
			|| operation.m_sRouteContractHash.IsEmpty())
		{
			string routeFailure = "exact garrison patrol local route could not be frozen";
			RollbackAdmission(state, quote, manifest, garrisons, operation, null, group,
				routeFailure, route);
			result.m_sFailureReason = routeFailure;
			return result;
		}

		HST_ForceSpawnQueueEnqueueResult enqueue = m_SpawnQueue.Enqueue(
			state.m_aForceSpawnResults,
			manifest,
			BuildSpawnRequest(state, quote),
			state.m_iElapsedSeconds);
		if (!enqueue || !enqueue.m_bSuccess || !enqueue.m_Batch || enqueue.m_bAlreadyApplied)
		{
			string enqueueFailure = "exact garrison patrol spawn admission failed";
			if (enqueue && !enqueue.m_sFailureReason.IsEmpty())
				enqueueFailure = enqueueFailure + ": " + enqueue.m_sFailureReason;
			HST_ForceSpawnResultState failedBatch;
			if (enqueue)
				failedBatch = enqueue.m_Batch;
			RollbackAdmission(state, quote, manifest, garrisons, operation,
				failedBatch, group, enqueueFailure, route);
			result.m_sFailureReason = enqueueFailure;
			return result;
		}
		result.m_Batch = enqueue.m_Batch;

		HST_ForceSpawnQueueCallbackResult held = m_SpawnQueue.HoldPendingProjectionForStrategicSimulation(
			state.m_aForceSpawnResults,
			manifest,
			enqueue.m_Batch.m_sResultId,
			enqueue.m_Batch.m_sProjectionId,
			state.m_iElapsedSeconds);
		if (!held || !held.m_bAccepted || !enqueue.m_Batch.m_bStrategicProjectionHeld)
		{
			string holdFailure = "exact garrison patrol strategic hold failed";
			if (held && !held.m_sFailureReason.IsEmpty())
				holdFailure = holdFailure + ": " + held.m_sFailureReason;
			RollbackAdmission(state, quote, manifest, garrisons, operation,
				enqueue.m_Batch, group, holdFailure, route);
			result.m_sFailureReason = holdFailure;
			return result;
		}

		HST_GarrisonState garrison;
		if (!LinkCommittedAdmission(
			state,
			quote,
			manifest,
			garrisons,
			operation,
			enqueue.m_Batch,
			group,
			garrison))
		{
			string linkFailure = "exact garrison patrol committed authority backlink failed";
			RollbackAdmission(state, quote, manifest, garrisons, operation,
				enqueue.m_Batch, group, linkFailure, route);
			result.m_sFailureReason = linkFailure;
			return result;
		}

		result.m_Garrison = garrison;
		result.m_bSuccess = true;
		result.m_bStateChanged = true;
		return result;
	}

	HST_OperationRecordState BuildOperation(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_ZoneState zone,
		string confirmationRequestId)
	{
		if (!state || !quote || !manifest || !zone)
			return null;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		HST_OperationRecordState operation = new HST_OperationRecordState();
		operation.m_sOperationId = quote.m_sOperationId;
		operation.m_eType = HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL;
		operation.m_iContractVersion = EXACT_CONTRACT_VERSION;
		operation.m_iProjectionContractVersion = EXACT_PROJECTION_CONTRACT_VERSION;
		operation.m_sOwnerFactionKey = quote.m_sFactionKey;
		operation.m_sActorIdentityId = quote.m_sActorIdentityId;
		operation.m_sIssueRequestId = quote.m_sCommandRequestId;
		operation.m_sConfirmationRequestId = confirmationRequestId;
		operation.m_sQuoteId = quote.m_sQuoteId;
		operation.m_sManifestId = manifest.m_sManifestId;
		operation.m_sOriginZoneId = zone.m_sZoneId;
		operation.m_vOriginPosition = zone.m_vPosition;
		operation.m_sAssignmentKind = ASSIGNMENT_KIND;
		operation.m_sAssignmentZoneId = zone.m_sZoneId;
		operation.m_vAssignmentPosition = zone.m_vPosition;
		operation.m_sTacticalTargetZoneId = zone.m_sZoneId;
		operation.m_vTacticalTargetPosition = zone.m_vPosition;
		operation.m_vStrategicPosition = zone.m_vPosition;
		operation.m_sRecallPolicyId = RECALL_POLICY_ID;
		operation.m_sSettlementPolicyId = SETTLEMENT_POLICY_ID;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
		operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
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

	HST_ActiveGroupState BuildActiveGroup(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_ZoneState zone)
	{
		if (!state || !quote || !manifest || !zone || manifest.m_aGroups.Count() != 1
			|| !manifest.m_aGroups[0])
			return null;
		string projectionId = BuildProjectionId(quote);
		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = projectionId;
		group.m_sOperationId = quote.m_sOperationId;
		group.m_sManifestId = manifest.m_sManifestId;
		group.m_sSpawnResultId = BuildSpawnResultId(quote);
		group.m_sForceId = BuildForceId(quote);
		group.m_sProjectionId = projectionId;
		group.m_sZoneId = zone.m_sZoneId;
		group.m_sGarrisonZoneId = zone.m_sZoneId;
		group.m_sFactionKey = quote.m_sFactionKey;
		group.m_sPrefab = manifest.m_aGroups[0].m_sPrefab;
		group.m_sCompositionRequestId = manifest.m_sManifestId;
		group.m_sCompositionIntentId = EXACT_INTENT_ID;
		group.m_sCompositionTier = "exact";
		group.m_sCompositionSummary = string.Format("%1 exact purchased garrison infantry", manifest.m_iAcceptedMemberCount);
		group.m_sSpawnFallbackMode = EXACT_GROUP_MODE;
		group.m_vSourcePosition = zone.m_vPosition;
		group.m_vTargetPosition = zone.m_vPosition;
		group.m_vPosition = zone.m_vPosition;
		group.m_sRuntimeStatus = "garrison_patrol_virtual";
		group.m_iInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iOriginalInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iCompositionManpower = manifest.m_iAcceptedMemberCount;
		group.m_iLastSeenAliveCount = manifest.m_iAcceptedMemberCount;
		group.m_iSurvivorInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iDurableLivingInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iSpawnedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		group.m_iLifecycleRevision = 1;
		return group;
	}

	HST_GeneratedRouteState BuildLocalPatrolRoute(
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_ZoneState zone)
	{
		if (!quote || !manifest || !zone || IsZeroVector(zone.m_vPosition))
			return null;
		HST_GeneratedRouteState route = new HST_GeneratedRouteState();
		route.m_sRouteId = BuildRouteId(quote);
		route.m_sSourceZoneId = zone.m_sZoneId;
		route.m_sTargetZoneId = zone.m_sZoneId;
		route.m_sSourceLayerName = "operation_owned";
		route.m_sSourceCategory = EXACT_GROUP_MODE;
		route.m_sSourceLayoutId = quote.m_sOperationId;
		route.m_bRoadRoute = false;
		route.m_bValidatedForVehicles = false;

		float radius = 80.0 + PositiveModulo(manifest.m_iDeterministicSeed, 41);
		int orientation = PositiveModulo(manifest.m_iDeterministicSeed / 41, 4);
		array<float> xOffsets = {radius, 0.0, -radius, 0.0};
		array<float> zOffsets = {0.0, radius, 0.0, -radius};
		array<vector> positions = {};
		for (int index = 0; index < 4; index++)
		{
			int offsetIndex = (index + orientation) % 4;
			vector candidate = zone.m_vPosition;
			candidate[0] = candidate[0] + xOffsets[offsetIndex];
			candidate[2] = candidate[2] + zOffsets[offsetIndex];
			vector resolved = HST_WorldPositionService.ResolveSafeGroundPosition(
				candidate,
				HST_WorldPositionService.CHARACTER_GROUND_OFFSET,
				false,
				4.0);
			if (IsZeroVector(resolved) || Distance2D(resolved, zone.m_vPosition) < 20.0)
				resolved = candidate;
			HST_RouteWaypointState waypoint = new HST_RouteWaypointState();
			waypoint.m_sRouteId = route.m_sRouteId;
			waypoint.m_iIndex = index;
			waypoint.m_vPosition = resolved;
			waypoint.m_iRadiusMeters = 25;
			waypoint.m_sHint = string.Format("garrison_patrol_%1", index + 1);
			route.m_aWaypoints.Insert(waypoint);
			positions.Insert(resolved);
		}
		route.m_vStartPosition = positions[0];
		route.m_vMidPosition = positions[1];
		route.m_vEndPosition = positions[3];
		route.m_iWaypointCount = route.m_aWaypoints.Count();
		float routeDistance;
		for (int distanceIndex = 0; distanceIndex < positions.Count(); distanceIndex++)
			routeDistance += Distance2D(positions[distanceIndex], positions[(distanceIndex + 1) % positions.Count()]);
		route.m_iDistanceMeters = Math.Round(routeDistance);
		ref array<vector> ordered = HST_OperationRouteCursorService.BuildOrderedRoutePositions(route);
		if (ordered.Count() < 3
			|| HST_OperationRouteCursorService.BuildRouteContractHash(route, ordered).IsEmpty())
			return null;
		return route;
	}

	bool LinkCommittedAdmission(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_GarrisonService garrisons,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		out HST_GarrisonState garrison)
	{
		garrison = null;
		if (!state || !quote || !manifest || !garrisons || !operation || !batch || !group)
			return false;
		if (!batch.m_bStrategicProjectionHeld || batch.m_sOperationId != quote.m_sOperationId
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| batch.m_sResultId != BuildSpawnResultId(quote)
			|| batch.m_sForceId != BuildForceId(quote)
			|| batch.m_sProjectionId != BuildProjectionId(quote)
			|| operation.m_sOperationId != quote.m_sOperationId
			|| group.m_sGroupId != batch.m_sProjectionId
			|| group.m_sOperationId != quote.m_sOperationId
			|| group.m_sManifestId != manifest.m_sManifestId)
			return false;
		HST_GeneratedRouteState route = state.FindGeneratedRoute(operation.m_sCurrentRouteId);
		ref array<vector> routePositions = HST_OperationRouteCursorService.BuildOrderedRoutePositions(route);
		if (!route || !IsOwnedRoute(quote, route) || routePositions.Count() < 3
			|| CountRouteIdentity(state, route) != 1
			|| !m_RouteCursor.IsPatrolRouteContractValid(operation, route)
			|| group.m_sRouteId != route.m_sRouteId)
			return false;

		garrison = garrisons.LinkExecutableManifestExact(
			state,
			quote.m_sTargetZoneId,
			quote.m_sFactionKey,
			manifest);
		if (!garrison || CountString(garrison.m_aAcceptedManifestIds, manifest.m_sManifestId) != 1)
			return false;

		operation.m_sSpawnResultId = batch.m_sResultId;
		operation.m_sForceId = batch.m_sForceId;
		operation.m_sProjectionId = batch.m_sProjectionId;
		operation.m_sGroupId = group.m_sGroupId;
		operation.m_sLastProjectionReason = "purchased garrison patrol admitted to strategic hold";
		operation.m_iLastProjectionDecisionSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iRevision++;
		return true;
	}

	bool RollbackAdmission(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_GarrisonService garrisons,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason,
		HST_GeneratedRouteState route = null)
	{
		if (!state || !quote || !manifest || !garrisons)
			return false;
		if (operation && (operation.m_sOperationId != quote.m_sOperationId
			|| operation.m_sManifestId != manifest.m_sManifestId
			|| state.m_aOperations.Find(operation) < 0
			|| CountOperationIdentity(state, operation) != 1))
			return false;
		if (group && (group.m_sOperationId != quote.m_sOperationId
			|| group.m_sManifestId != manifest.m_sManifestId
			|| group.m_bSpawnedEntity || !group.m_sRuntimeEntityId.IsEmpty()
			|| group.m_iSpawnedAgentCount > 0
			|| state.m_aActiveGroups.Find(group) < 0
			|| CountGroupIdentity(state, group) != 1))
			return false;
		if (batch && (batch.m_sOperationId != quote.m_sOperationId
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| state.m_aForceSpawnResults.Find(batch) < 0
			|| CountBatchIdentity(state, batch) != 1
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING))
			return false;
		if (!route)
			route = state.FindGeneratedRoute(BuildRouteId(quote));
		if (route && (!IsOwnedRoute(quote, route)
			|| state.m_aGeneratedRoutes.Find(route) < 0
			|| CountRouteIdentity(state, route) != 1))
			return false;

		HST_GarrisonState garrison = state.FindGarrison(quote.m_sTargetZoneId, quote.m_sFactionKey);
		int backlinkCount = CountGarrisonBacklinks(state, manifest.m_sManifestId);
		if (backlinkCount > 1)
			return false;
		if (backlinkCount == 1 && (!garrison
			|| CountGarrisonIdentity(state, garrison) != 1
			|| CountString(garrison.m_aAcceptedManifestIds, manifest.m_sManifestId) != 1))
			return false;
		if (garrison && garrison.m_aAcceptedManifestIds.Contains(manifest.m_sManifestId))
		{
			if (!garrisons.UnlinkExecutableManifestExact(
				state,
				quote.m_sTargetZoneId,
				quote.m_sFactionKey,
				manifest))
				return false;
		}
		if (batch && state.m_aForceSpawnResults.Find(batch) >= 0)
		{
			if (m_SpawnQueue)
				m_SpawnQueue.RequestCancel(
					state.m_aForceSpawnResults,
					batch.m_sResultId,
					state.m_iElapsedSeconds,
					reason);
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
		if (route)
		{
			int routeIndex = state.m_aGeneratedRoutes.Find(route);
			if (routeIndex >= 0)
				state.m_aGeneratedRoutes.Remove(routeIndex);
		}
		if (operation)
		{
			int operationIndex = state.m_aOperations.Find(operation);
			if (operationIndex >= 0)
				state.m_aOperations.Remove(operationIndex);
		}
		return true;
	}

	HST_GarrisonPatrolAdmissionResult ResolveCommittedAdmission(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_GarrisonService garrisons)
	{
		HST_GarrisonPatrolAdmissionResult result = new HST_GarrisonPatrolAdmissionResult();
		if (!state || !quote || !manifest || !garrisons || !m_SpawnQueue)
		{
			result.m_sFailureReason = "exact garrison patrol committed replay context is incomplete";
			return result;
		}
		HST_OperationRecordState operation = state.FindOperation(quote.m_sOperationId);
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		HST_GeneratedRouteState route;
		if (operation)
		{
			batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
			group = state.FindActiveGroup(operation.m_sGroupId);
			route = state.FindGeneratedRoute(operation.m_sCurrentRouteId);
		}
		HST_GarrisonState garrison = state.FindGarrison(quote.m_sTargetZoneId, quote.m_sFactionKey);
		string failure = ValidateCommittedLinks(state, quote, manifest, garrison, operation, batch, group, route);
		if (!failure.IsEmpty())
		{
			result.m_sFailureReason = failure;
			return result;
		}
		result.m_bSuccess = true;
		result.m_bAlreadyApplied = true;
		result.m_Garrison = garrison;
		result.m_Operation = operation;
		result.m_Batch = batch;
		result.m_Group = group;
		result.m_Route = route;
		return result;
	}

	bool HasInterruptedAdmissionAuthority(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		if (!state || !quote || !manifest || !IsCurrentPolicyQuote(quote)
			|| !IsCurrentPolicyManifest(manifest))
			return false;
		if (state.FindOperation(quote.m_sOperationId))
			return true;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && (batch.m_sOperationId == quote.m_sOperationId
				|| batch.m_sManifestId == manifest.m_sManifestId
				|| batch.m_sResultId == BuildSpawnResultId(quote)))
				return true;
		}
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && (group.m_sOperationId == quote.m_sOperationId
				|| group.m_sManifestId == manifest.m_sManifestId
				|| group.m_sGroupId == BuildProjectionId(quote)))
				return true;
		}
		if (state.FindGeneratedRoute(BuildRouteId(quote)))
			return true;
		foreach (HST_GarrisonState garrison : state.m_aGarrisons)
		{
			if (garrison && garrison.m_aAcceptedManifestIds.Contains(manifest.m_sManifestId))
				return true;
		}
		return false;
	}

	bool RollbackInterruptedAdmission(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_GarrisonService garrisons,
		string reason)
	{
		if (!state || !quote || !manifest || !garrisons)
			return false;
		HST_OperationRecordState operation = state.FindOperation(quote.m_sOperationId);
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		foreach (HST_ForceSpawnResultState candidateBatch : state.m_aForceSpawnResults)
		{
			if (!candidateBatch || (candidateBatch.m_sOperationId != quote.m_sOperationId
				&& candidateBatch.m_sManifestId != manifest.m_sManifestId
				&& candidateBatch.m_sResultId != BuildSpawnResultId(quote)))
				continue;
			if (batch)
				return false;
			batch = candidateBatch;
		}
		foreach (HST_ActiveGroupState candidateGroup : state.m_aActiveGroups)
		{
			if (!candidateGroup || (candidateGroup.m_sOperationId != quote.m_sOperationId
				&& candidateGroup.m_sManifestId != manifest.m_sManifestId
				&& candidateGroup.m_sGroupId != BuildProjectionId(quote)))
				continue;
			if (group)
				return false;
			group = candidateGroup;
		}
		HST_GeneratedRouteState route = state.FindGeneratedRoute(BuildRouteId(quote));
		return RollbackAdmission(
			state,
			quote,
			manifest,
			garrisons,
			operation,
			batch,
			group,
			reason,
			route);
	}

	bool Tick(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return false;
		bool changed;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL)
				continue;
			if (operation.m_iContractVersion == QUARANTINED_CONTRACT_VERSION)
			{
				changed = ApplyQuarantineStatus(state, operation, operation.m_sLastProjectionReason) || changed;
				continue;
			}
			if (operation.m_iContractVersion != EXACT_CONTRACT_VERSION)
			{
				changed = QuarantineOperationAuthority(
					state,
					operation,
					"unsupported exact garrison patrol contract version") || changed;
				continue;
			}
			changed = TickOperation(state, operation) || changed;
		}
		return changed;
	}

	bool ReconcileAfterRestore(HST_CampaignState state)
	{
		if (!state || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar)
			return false;
		bool changed;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL)
				continue;
			if (operation.m_iContractVersion == QUARANTINED_CONTRACT_VERSION)
			{
				changed = ApplyQuarantineStatus(state, operation, operation.m_sLastProjectionReason) || changed;
				continue;
			}
			if (operation.m_iContractVersion != EXACT_CONTRACT_VERSION)
			{
				changed = QuarantineOperationAuthority(
					state,
					operation,
					"restore found an unsupported garrison patrol contract version") || changed;
				continue;
			}
			if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			{
				changed = FinalizeSettledRuntime(state, operation) || changed;
				continue;
			}

			HST_ForceQuoteState quote;
			HST_ForceManifestState manifest;
			HST_GarrisonState garrison;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			HST_GeneratedRouteState route;
			string failure = ResolveRuntimeContext(
				state,
				operation,
				quote,
				manifest,
				garrison,
				batch,
				group,
				route);
			if (!failure.IsEmpty())
			{
				changed = QuarantineOperationAuthority(state, operation, failure) || changed;
				continue;
			}
			HST_ZoneState zone = state.FindZone(operation.m_sAssignmentZoneId);
			if (zone && zone.m_sOwnerFactionKey != operation.m_sOwnerFactionKey)
			{
				changed = RetireAndSettle(
					state,
					operation,
					quote,
					manifest,
					batch,
					group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
					"owner_changed",
					"garrison patrol assignment ended because zone ownership changed") || changed;
				continue;
			}
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
				|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
			{
				changed = RetireAndSettle(
					state,
					operation,
					quote,
					manifest,
					batch,
					group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
					"restore_spawn_failed",
					"restored exact garrison patrol contains a terminal spawn failure") || changed;
				continue;
			}
			changed = NormalizeRestoredOpenRuntime(
				state,
				operation,
				manifest,
				batch,
				group) || changed;
		}
		return changed;
	}

	bool ReconcileSettledRuntimeCleanup(HST_CampaignState state)
	{
		if (!state)
			return false;
		bool changed;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL
				|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				continue;
			changed = RetireSettledRuntimeIfPresent(state, operation) || changed;
			changed = FinalizeSettledRuntime(state, operation) || changed;
		}
		return changed;
	}

	bool PrepareOpenPhysicalAuthorityForSettlement(HST_CampaignState state, out string failure)
	{
		failure = "";
		if (!state || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar)
		{
			failure = "exact garrison patrol settlement reconciliation services are unavailable";
			return false;
		}
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL
				|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				continue;
			bool physical = operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
			bool dematerializing = operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
			if (!physical && !dematerializing)
				continue;

			HST_ForceQuoteState quote;
			HST_ForceManifestState manifest;
			HST_GarrisonState garrison;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			HST_GeneratedRouteState route;
			failure = ResolveRuntimeContext(
				state,
				operation,
				quote,
				manifest,
				garrison,
				batch,
				group,
				route);
			if (!failure.IsEmpty())
			{
				QuarantineOperationAuthority(state, operation, failure);
				failure = "exact garrison patrol settlement graph conflicts: " + failure;
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
				failure = "exact garrison patrol settlement casualty reconciliation failed";
				if (reconciled && !reconciled.m_sSummary.IsEmpty())
					failure = failure + ": " + reconciled.m_sSummary;
				return false;
			}
			int living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
			SyncGroupRoster(group, living);
			operation.m_iLastVirtualFriendlyCount = living;
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
				failure = "exact garrison patrol settlement live bindings are incomplete: " + bindingFailure;
				return false;
			}
		}
		return true;
	}

	bool SettleOpenOperationsForCampaignStop(HST_CampaignState state, string reason)
	{
		if (!state)
			return false;
		if (reason.IsEmpty())
			reason = "campaign phase no longer permits an active garrison patrol";
		bool changed;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL
				|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				continue;
			HST_ForceQuoteState quote;
			HST_ForceManifestState manifest;
			HST_GarrisonState garrison;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			HST_GeneratedRouteState route;
			string failure = ResolveRuntimeContext(
				state,
				operation,
				quote,
				manifest,
				garrison,
				batch,
				group,
				route);
			if (!failure.IsEmpty())
			{
				changed = QuarantineOperationAuthority(state, operation, reason + ": " + failure) || changed;
				continue;
			}
			changed = RetireAndSettle(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED,
				"campaign_stop",
				reason) || changed;
		}
		return changed;
	}

	protected bool TickOperation(
		HST_CampaignState state,
		HST_OperationRecordState operation)
	{
		if (!state || !operation || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar
			|| !m_Materialization || !m_RouteCursor || !m_Integrity)
			return QuarantineOperationAuthority(state, operation,
				"exact garrison patrol runtime services are unavailable");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return FinalizeSettledRuntime(state, operation);

		HST_ForceQuoteState quote;
		HST_ForceManifestState manifest;
		HST_GarrisonState garrison;
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		HST_GeneratedRouteState route;
		string failure = ResolveRuntimeContext(
			state,
			operation,
			quote,
			manifest,
			garrison,
			batch,
			group,
			route);
		if (!failure.IsEmpty())
			return QuarantineOperationAuthority(state, operation, failure);

		// Exact patrol positions remain immutable across location-taxonomy merges.
		// The historical view keeps the frozen ID/position while reflecting the
		// canonical row's current owner.
		HST_ZoneState zone = state.FindFrozenHistoricalZoneView(operation.m_sAssignmentZoneId);
		if (zone.m_sOwnerFactionKey != operation.m_sOwnerFactionKey)
		{
			return RetireAndSettle(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"owner_changed",
				"garrison patrol assignment ended because zone ownership changed");
		}
		if (operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
		{
			return ContinueDematerialization(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				operation.m_sLastProjectionReason);
		}
		if (operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL)
			return TickVirtual(state, operation, quote, manifest, route, batch, group);
		if (operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
			return TickMaterializing(state, operation, quote, manifest, batch, group);
		if (operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			return TickPhysical(state, operation, quote, manifest, route, batch, group);
		return QuarantineOperationAuthority(state, operation,
			"exact garrison patrol materialization authority is invalid");
	}

	protected bool TickVirtual(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_GeneratedRouteState route,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
		{
			return SettleOperation(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"spawn_failed",
				"held exact garrison patrol projection failed");
		}
		if (!batch.m_bStrategicProjectionHeld)
			return QuarantineOperationAuthority(state, operation,
				"virtual exact garrison patrol batch is not strategically held");

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
				"exact garrison patrol virtual roster eliminated");
			return SettleOperation(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
				"destroyed",
				"exact garrison patrol roster was eliminated");
		}

		HST_OperationRouteCursorResult movement = m_RouteCursor.AdvanceVirtualLeg(state, operation, group);
		if (!movement || !movement.m_bAccepted)
		{
			return SettleOperation(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
				"route_failed",
				"exact garrison patrol strategic route advancement failed");
		}
		bool changed = movement.m_bStateChanged;
		if (movement.m_bArrived)
		{
			if (!AdvanceInfinitePatrolLoop(state, operation, route, group, false))
			{
				return SettleOperation(
					state,
					operation,
					quote,
					manifest,
					batch,
					group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
					"route_failed",
					"exact garrison patrol loop transition failed") || changed;
			}
			changed = true;
		}

		HST_OperationProjectionDecision decision = m_Materialization.EvaluateExactPlayerQRF(
			operation,
			operation.m_vStrategicPosition);
		int decisionSecond = Math.Max(0, state.m_iElapsedSeconds);
		if (operation.m_sLastProjectionReason != decision.m_sReason
			|| operation.m_iLastProjectionDecisionSecond != decisionSecond)
		{
			operation.m_sLastProjectionReason = decision.m_sReason;
			operation.m_iLastProjectionDecisionSecond = decisionSecond;
			operation.m_iRevision++;
			changed = true;
		}
		if (decision.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE)
			return BeginMaterialization(state, operation, quote, manifest, batch, group, decision.m_sReason) || changed;
		ApplyVirtualRuntimeStatus(group);
		return changed;
	}

	protected bool BeginMaterialization(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceQuoteState quote,
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
		{
			return SettleOperation(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"spawn_failed",
				"exact garrison patrol materialization release failed");
		}
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iLastProjectionDecisionSecond = nowSecond;
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		group.m_vPosition = operation.m_vStrategicPosition;
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		group.m_vTargetPosition = operation.m_vRouteEndPosition;
		group.m_sRuntimeStatus = "garrison_patrol_materializing";
		group.m_iLifecycleRevision++;
		return true;
	}

	protected bool TickMaterializing(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED
			|| group.m_sRuntimeStatus == "spawn_failed")
		{
			return RetireAndSettle(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"spawn_failed",
				"exact garrison patrol materialization failed");
		}
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| !group.m_bSpawnedEntity)
			return false;

		HST_ForceSpawnAdapterTickResult reconciled = m_SpawnAdapter.ReconcileExactInfantryProjectionAuthority(
			state,
			m_SpawnQueue,
			m_PhysicalWar,
			Math.Max(0, state.m_iElapsedSeconds),
			operation.m_sProjectionId);
		if (!reconciled || reconciled.m_iFailedCount > 0)
			return QuarantineOperationAuthority(state, operation,
				"exact garrison patrol materialization binding reconciliation failed");
		int living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
		SyncGroupRoster(group, living);
		operation.m_iLastVirtualFriendlyCount = living;
		if (living <= 0)
		{
			return RetireAndSettle(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
				"destroyed",
				"exact garrison patrol was eliminated during materialization");
		}
		string bindingFailure;
		if (!m_SpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
			state,
			batch,
			m_SpawnQueue,
			m_PhysicalWar,
			bindingFailure))
		{
			return QuarantineOperationAuthority(state, operation,
				"exact garrison patrol materialization bindings are incomplete: " + bindingFailure);
		}

		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		operation.m_iRevision++;
		if (!m_PhysicalWar.RestartExactGarrisonPatrolInfantryRoute(
			state,
			group,
			operation.m_vRouteEndPosition,
			"Exact garrison patrol materialized on its persisted local-route leg."))
		{
			return RetireAndSettle(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
				"route_failed",
				"exact garrison patrol live route could not be issued");
		}
		group.m_sRuntimeStatus = "garrison_patrol_physical";
		return true;
	}

	protected bool TickPhysical(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_GeneratedRouteState route,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			return RetireAndSettle(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"physical_batch_invalid",
				"exact garrison patrol physical batch lost successful authority");
		}
		if (group.m_sRuntimeStatus.Contains("runtime_binding_missing"))
			return QuarantineOperationAuthority(state, operation,
				"exact garrison patrol runtime binding disappeared without casualty proof");

		HST_ForceSpawnAdapterTickResult reconciled = m_SpawnAdapter.ReconcileExactInfantryProjectionAuthority(
			state,
			m_SpawnQueue,
			m_PhysicalWar,
			Math.Max(0, state.m_iElapsedSeconds),
			operation.m_sProjectionId);
		if (!reconciled || reconciled.m_iFailedCount > 0)
		{
			string reconcileFailure = "exact garrison patrol casualty reconciliation failed";
			if (reconciled && !reconciled.m_sSummary.IsEmpty())
				reconcileFailure = reconcileFailure + ": " + reconciled.m_sSummary;
			return QuarantineOperationAuthority(state, operation, reconcileFailure);
		}
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
				return QuarantineOperationAuthority(state, operation,
					"exact garrison patrol physical binding graph is unresolved: " + bindingFailure);
			}
		}
		if (living > 0 && (!group.m_bSpawnedEntity || group.m_sRuntimeStatus == "spawn_failed"))
			return QuarantineOperationAuthority(state, operation,
				"exact garrison patrol physical projection is missing despite a durable living roster");
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
				return QuarantineOperationAuthority(state, operation,
					"exact garrison patrol elimination cleanup is unresolved: " + eliminationReason);
			}
			return SettleOperation(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
				"destroyed",
				"exact garrison patrol was eliminated");
		}

		vector livePosition;
		string liveEvidence;
		if (!m_PhysicalWar.TryResolveExactGarrisonPatrolLivePosition(
			state,
			group,
			livePosition,
			liveEvidence))
		{
			return RetireAndSettle(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"live_position_failed",
				"exact garrison patrol live position is unavailable: " + liveEvidence);
		}
		operation.m_vStrategicPosition = livePosition;
		operation.m_iLastProgressAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iRevision++;
		bool changed = true;

		bool casualtyHold;
		bool casualtyRecovery;
		changed = UpdateRecentCasualtyHold(
			state,
			operation,
			group,
			casualtyHold,
			casualtyRecovery) || changed;
		if (casualtyHold)
		{
			operation.m_iArrivalConfirmationCount = 0;
			operation.m_iLastArrivalConfirmationSecond = 0;
			return changed;
		}
		if (casualtyRecovery)
		{
			if (!m_PhysicalWar.RestartExactGarrisonPatrolInfantryRoute(
				state,
				group,
				operation.m_vRouteEndPosition,
				"Exact garrison patrol resumed its persisted leg after recent casualties cleared."))
			{
				return RetireAndSettle(
					state,
					operation,
					quote,
					manifest,
					batch,
					group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
					"route_recovery_failed",
					"exact garrison patrol could not recover its live route after casualties");
			}
			return true;
		}

		if (m_PhysicalWar.IsExactGarrisonPatrolRouteRecoveryExhausted(
			state,
			group,
			state.m_iElapsedSeconds))
		{
			return RetireAndSettle(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
				"route_recovery_exhausted",
				"exact garrison patrol exhausted bounded live-route recovery");
		}

		bool arrived;
		changed = ConfirmPhysicalArrivalSample(state, operation, livePosition, arrived) || changed;
		if (arrived)
		{
			if (!AdvanceInfinitePatrolLoop(state, operation, route, group, true))
			{
				return RetireAndSettle(
					state,
					operation,
					quote,
					manifest,
					batch,
					group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
					"route_transition_failed",
					"exact garrison patrol physical loop transition failed");
			}
			return true;
		}

		HST_OperationProjectionDecision decision = m_Materialization.EvaluateExactPlayerQRF(
			operation,
			livePosition);
		int decisionSecond = Math.Max(0, state.m_iElapsedSeconds);
		if (operation.m_sLastProjectionReason != decision.m_sReason
			|| operation.m_iLastProjectionDecisionSecond != decisionSecond)
		{
			operation.m_sLastProjectionReason = decision.m_sReason;
			operation.m_iLastProjectionDecisionSecond = decisionSecond;
			operation.m_iRevision++;
			changed = true;
		}
		if (decision.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_DEMATERIALIZE)
		{
			return TryDematerialize(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				decision.m_sReason) || changed;
		}
		return changed;
	}

	protected bool UpdateRecentCasualtyHold(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		out bool casualtyHold,
		out bool casualtyRecovery)
	{
		casualtyHold = false;
		casualtyRecovery = false;
		if (!state || !operation || !group)
			return false;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		bool recentCasualty = group.m_iLastCasualtySecond > 0;
		if (recentCasualty)
			recentCasualty = nowSecond - group.m_iLastCasualtySecond <= CASUALTY_CLEAR_SECONDS;
		if (recentCasualty)
		{
			casualtyHold = true;
			bool changed = operation.m_eEngagementMode
				!= HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT;
			if (group.m_sRuntimeStatus != "garrison_patrol_casualty_hold")
				changed = true;
			operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT;
			operation.m_iEngagementStateEnteredAtSecond = group.m_iLastCasualtySecond;
			operation.m_iLastContactAtSecond = group.m_iLastCasualtySecond;
			operation.m_sLastProjectionReason = "recent exact garrison patrol casualty holds route advancement";
			group.m_sRuntimeStatus = "garrison_patrol_casualty_hold";
			if (changed)
			{
				operation.m_iRevision++;
				group.m_iLifecycleRevision++;
			}
			return changed;
		}
		if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
			return false;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_sLastProjectionReason = "recent exact garrison patrol casualties cleared";
		operation.m_iRevision++;
		casualtyRecovery = true;
		return true;
	}

	protected bool ConfirmPhysicalArrivalSample(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		vector livePosition,
		out bool arrived)
	{
		arrived = false;
		if (!state || !operation)
			return false;
		if (Distance2D(livePosition, operation.m_vRouteEndPosition) > PHYSICAL_ARRIVAL_RADIUS_METERS)
		{
			if (operation.m_iArrivalConfirmationCount == 0
				&& operation.m_iLastArrivalConfirmationSecond == 0)
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

	protected bool AdvanceInfinitePatrolLoop(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_GeneratedRouteState route,
		HST_ActiveGroupState group,
		bool physical)
	{
		if (!state || !operation || !route || !group
			|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
			return false;
		bool started = operation.m_iRouteWaypointIndex == 0
			&& operation.m_iRouteLapCount == 0
			&& operation.m_iRouteLegSequence <= 1;
		if (started)
		{
			if (!m_RouteCursor.StartPatrolLoop(state, operation, route, group))
				return false;
		}
		else
		{
			ref array<vector> positions = HST_OperationRouteCursorService.BuildOrderedRoutePositions(route);
			int lastIndex = positions.Count() - 1;
			if (operation.m_iRouteWaypointIndex == lastIndex
				&& operation.m_iRouteLapCount >= LOOP_COUNTER_WRAP_LAPS - 1)
			{
				operation.m_iRouteLapCount = 0;
				operation.m_iRevision++;
			}
			HST_OperationRouteCursorResult advanced = m_RouteCursor.AdvanceLoopAfterArrival(
				state,
				operation,
				route,
				group,
				LOOP_COUNTER_WRAP_LAPS);
			if (!advanced || !advanced.m_bAccepted
				|| operation.m_fRouteProgressMeters > 0.01)
				return false;
		}
		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		ApplyVirtualRuntimeStatus(group);
		if (!physical)
			return true;
		return m_PhysicalWar.RestartExactGarrisonPatrolInfantryRoute(
			state,
			group,
			operation.m_vRouteEndPosition,
			"Exact garrison patrol advanced to its next persisted local-route leg.");
	}

	protected bool TryDematerialize(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceQuoteState quote,
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
			return false;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		operation.m_iMaterializationStateEnteredAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		group.m_sRuntimeStatus = "garrison_patrol_dematerializing";
		return ContinueDematerialization(state, operation, quote, manifest, batch, group, reason);
	}

	protected bool ContinueDematerialization(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceQuoteState quote,
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
			HST_ForceSpawnAdapterTickResult reconciled = m_SpawnAdapter.ReconcileExactInfantryProjectionAuthority(
				state,
				m_SpawnQueue,
				m_PhysicalWar,
				Math.Max(0, state.m_iElapsedSeconds),
				operation.m_sProjectionId);
			if (!reconciled || reconciled.m_iFailedCount > 0)
				return QuarantineOperationAuthority(state, operation,
					"exact garrison patrol dematerialization casualty reconciliation failed");
			int living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
			if (living > 0)
			{
				string bindingFailure;
				if (!m_SpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
					state,
					batch,
					m_SpawnQueue,
					m_PhysicalWar,
					bindingFailure))
				{
					return QuarantineOperationAuthority(state, operation,
						"exact garrison patrol dematerialization bindings are incomplete: " + bindingFailure);
				}
			}
			bool casualtyHold;
			bool casualtyRecovery;
			UpdateRecentCasualtyHold(state, operation, group, casualtyHold, casualtyRecovery);
			if (casualtyHold)
				return false;

			HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
				state,
				m_PhysicalWar,
				group.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return false;
			int deadlineSecond = state.m_iElapsedSeconds + DEPLOYMENT_GRACE_SECONDS;
			HST_ForceSpawnQueueCallbackResult held = m_SpawnQueue.RequeueSuccessfulProjectionForStrategicHold(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				state.m_iElapsedSeconds,
				deadlineSecond);
			if (!held || !held.m_bAccepted)
				return QuarantineOperationAuthority(state, operation,
					"exact garrison patrol survivor projection could not enter strategic hold");
		}

		if (!m_RouteCursor.SyncLegFromPosition(state, operation, group, foldedPosition))
		{
			return SettleOperation(
				state,
				operation,
				quote,
				manifest,
				batch,
				group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
				"fold_route_failed",
				"exact garrison patrol live position could not fold into its persisted cursor");
		}
		ClearGroupProcessAuthority(group);
		SyncGroupRoster(group, m_SpawnQueue.CountStrategicLivingMemberSlots(batch));
		group.m_iLifecycleRevision++;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_vStrategicPosition = foldedPosition;
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		ApplyVirtualRuntimeStatus(group);
		return true;
	}

	protected bool RetireAndSettle(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_EOperationTerminalResult terminalResult,
		string outcomeDetail,
		string reason)
	{
		if (!state || !operation || !quote || !manifest || !batch || !group)
			return QuarantineOperationAuthority(state, operation,
				"exact garrison patrol settlement authority is incomplete: " + reason);
		bool liveState = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		bool dematerializing = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
		bool materializing = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		int handleCount = m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId);
		bool physicalRuntime = group.m_bSpawnedEntity || handleCount > 0;
		if (!physicalRuntime)
			physicalRuntime = m_PhysicalWar.GetForceSpawnGroupRoot(group) != null;
		if (!physicalRuntime)
			physicalRuntime = m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0;
		bool establishedLiveAuthority = liveState || dematerializing;
		int durableLiving = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
		if (establishedLiveAuthority && !physicalRuntime && durableLiving > 0)
		{
			return QuarantineOperationAuthority(state, operation,
				"exact garrison patrol terminal transition lost live runtime while durable survivors remain: " + reason);
		}
		if (establishedLiveAuthority && physicalRuntime
			&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			return QuarantineOperationAuthority(state, operation,
				"exact garrison patrol terminal transition found live runtime without successful batch authority: " + reason);
		}
		if ((liveState || dematerializing || materializing) && physicalRuntime)
		{
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			{
				HST_ForceSpawnAdapterTickResult reconciled = m_SpawnAdapter.ReconcileExactInfantryProjectionAuthority(
					state,
					m_SpawnQueue,
					m_PhysicalWar,
					Math.Max(0, state.m_iElapsedSeconds),
					operation.m_sProjectionId);
				if (!reconciled || reconciled.m_iFailedCount > 0)
					return QuarantineOperationAuthority(state, operation,
						"exact garrison patrol terminal casualty reconciliation failed: " + reason);
				int living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
				if (living > 0)
				{
					string bindingFailure;
					if (!m_SpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
						state,
						batch,
						m_SpawnQueue,
						m_PhysicalWar,
						bindingFailure))
					{
						return QuarantineOperationAuthority(state, operation,
							"exact garrison patrol terminal bindings are incomplete: " + bindingFailure);
					}
				}
			}
			HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
				state,
				m_PhysicalWar,
				operation.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return false;
			ClearGroupProcessAuthority(group);
		}
		else if (handleCount > 0)
			return QuarantineOperationAuthority(state, operation,
				"exact garrison patrol terminal found handles outside a live transition: " + reason);
		return SettleOperation(
			state,
			operation,
			quote,
			manifest,
			batch,
			group,
			terminalResult,
			outcomeDetail,
			reason);
	}

	protected bool SettleOperation(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_EOperationTerminalResult terminalResult,
		string outcomeDetail,
		string reason)
	{
		if (!state || !operation || !quote || !manifest || outcomeDetail.IsEmpty())
			return false;
		string settlementId = HST_OperationService.BuildSettlementId(
			operation.m_sOperationId,
			SETTLEMENT_KIND);
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (operation.m_sSettlementId != settlementId
				|| operation.m_eTerminalResult != terminalResult)
			{
				return QuarantineOperationAuthority(state, operation,
					"exact garrison patrol terminal receipt conflicts");
			}
			return FinalizeSettledRuntime(state, operation);
		}
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN)
		{
			return QuarantineOperationAuthority(state, operation,
				"exact garrison patrol settlement authority is invalid");
		}

		int living = ResolveLivingRoster(manifest, batch, group, operation);
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iLastVirtualFriendlyCount = living;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		operation.m_eTerminalResult = terminalResult;
		operation.m_sSettlementId = settlementId;
		operation.m_sTerminalReason = reason;
		operation.m_sLastProjectionReason = string.Format(
			"%1: %2",
			SETTLEMENT_KIND,
			outcomeDetail);
		operation.m_iDutyStateEnteredAtSecond = nowSecond;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iLastProgressAtSecond = nowSecond;
		operation.m_iSettledAtSecond = nowSecond;
		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		operation.m_iRevision++;
		if (group)
		{
			ClearGroupProcessAuthority(group);
			SyncGroupRoster(group, living);
			group.m_sRuntimeStatus = "exact_garrison_patrol_terminal_" + outcomeDetail;
			group.m_iLifecycleRevision++;
		}
		FinalizeSettledRuntime(state, operation);
		return true;
	}

	protected bool RetireSettledRuntimeIfPresent(
		HST_CampaignState state,
		HST_OperationRecordState operation)
	{
		if (!state || !operation || !m_SpawnAdapter || !m_PhysicalWar)
			return false;
		HST_ActiveGroupState group = state.FindActiveGroup(operation.m_sGroupId);
		if (!group)
			return false;
		int handles = m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId);
		bool runtimeExists = handles > 0 || group.m_bSpawnedEntity;
		if (!runtimeExists)
			runtimeExists = m_PhysicalWar.GetForceSpawnGroupRoot(group) != null;
		if (!runtimeExists)
			runtimeExists = m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0;
		if (!runtimeExists)
			return false;
		HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
			state,
			m_PhysicalWar,
			operation.m_sProjectionId);
		if (!retired || !retired.m_bSuccess)
			return false;
		ClearGroupProcessAuthority(group);
		group.m_iLifecycleRevision++;
		return true;
	}

	protected bool FinalizeSettledRuntime(
		HST_CampaignState state,
		HST_OperationRecordState operation)
	{
		if (!state || !operation
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			|| operation.m_sSettlementId != HST_OperationService.BuildSettlementId(
				operation.m_sOperationId,
				SETTLEMENT_KIND)
			|| !m_SpawnAdapter || !m_PhysicalWar)
			return false;
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
		HST_ActiveGroupState group = state.FindActiveGroup(operation.m_sGroupId);
		HST_GeneratedRouteState route = state.FindGeneratedRoute(operation.m_sCurrentRouteId);
		HST_GarrisonState garrison = state.FindGarrison(
			operation.m_sAssignmentZoneId,
			operation.m_sOwnerFactionKey);
		if (m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0)
			return false;
		if (group && (m_PhysicalWar.GetForceSpawnGroupRoot(group)
			|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0))
			return false;
		if (batch)
		{
			bool batchOwned = batch.m_sResultId == operation.m_sSpawnResultId
				&& batch.m_sOperationId == operation.m_sOperationId
				&& batch.m_sManifestId == operation.m_sManifestId
				&& batch.m_sForceId == operation.m_sForceId
				&& batch.m_sProjectionId == operation.m_sProjectionId;
			if (!batchOwned || CountBatchIdentity(state, batch) != 1)
				return false;
		}
		if (group)
		{
			bool groupOwned = group.m_sGroupId == operation.m_sGroupId
				&& group.m_sOperationId == operation.m_sOperationId
				&& group.m_sManifestId == operation.m_sManifestId
				&& group.m_sSpawnResultId == operation.m_sSpawnResultId
				&& group.m_sForceId == operation.m_sForceId
				&& group.m_sProjectionId == operation.m_sProjectionId;
			if (!groupOwned || CountGroupIdentity(state, group) != 1)
				return false;
		}
		if (route)
		{
			bool routeOwned = route.m_sSourceCategory == EXACT_GROUP_MODE
				&& route.m_sSourceLayoutId == operation.m_sOperationId
				&& route.m_sSourceZoneId == operation.m_sAssignmentZoneId
				&& route.m_sTargetZoneId == operation.m_sAssignmentZoneId;
			if (!routeOwned || CountRouteIdentity(state, route) != 1)
				return false;
		}
		int backlinkCount = CountGarrisonBacklinks(state, operation.m_sManifestId);
		if (backlinkCount > 1)
			return false;
		if (garrison && CountGarrisonIdentity(state, garrison) != 1)
			return false;
		if (backlinkCount == 1)
		{
			if (!garrison
				|| CountString(garrison.m_aAcceptedManifestIds, operation.m_sManifestId) != 1)
				return false;
		}

		bool changed;
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
		if (route)
		{
			int routeIndex = state.m_aGeneratedRoutes.Find(route);
			if (routeIndex >= 0)
			{
				state.m_aGeneratedRoutes.Remove(routeIndex);
				changed = true;
			}
		}
		if (garrison)
		{
			int manifestIndex = garrison.m_aAcceptedManifestIds.Find(operation.m_sManifestId);
			if (manifestIndex >= 0)
			{
				garrison.m_aAcceptedManifestIds.Remove(manifestIndex);
				changed = true;
			}
			if (garrison.m_iInfantryCount <= 0 && garrison.m_iVehicleCount <= 0
				&& garrison.m_aAcceptedManifestIds.Count() == 0)
			{
				int garrisonIndex = state.m_aGarrisons.Find(garrison);
				if (garrisonIndex >= 0)
				{
					state.m_aGarrisons.Remove(garrisonIndex);
					changed = true;
				}
			}
		}
		return changed;
	}

	protected bool NormalizeRestoredOpenRuntime(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!state || !operation || !manifest || !batch || !group)
			return false;
		bool changed;
		vector savedPosition = group.m_vPosition;
		bool savedLive = operation.m_ePositionAuthority
			== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		if (savedLive && !IsZeroVector(savedPosition))
			changed = m_RouteCursor.SyncLegFromPosition(state, operation, group, savedPosition) || changed;

		if (!batch.m_bStrategicProjectionHeld
			&& batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			int handles = m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId);
			bool runtimeExists = group.m_bSpawnedEntity || handles > 0;
			if (!runtimeExists)
				runtimeExists = m_PhysicalWar.GetForceSpawnGroupRoot(group) != null;
			if (!runtimeExists)
				runtimeExists = m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0;
			if (runtimeExists)
			{
				HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
					state,
					m_PhysicalWar,
					operation.m_sProjectionId);
				if (!retired || !retired.m_bSuccess)
					return QuarantineOperationAuthority(state, operation,
						"restored exact garrison patrol runtime could not retire safely");
			}
			HST_ForceSpawnQueueCallbackResult held = m_SpawnQueue.RequeueSuccessfulProjectionForStrategicHold(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				state.m_iElapsedSeconds,
				state.m_iElapsedSeconds + DEPLOYMENT_GRACE_SECONDS);
			if (!held || !held.m_bAccepted)
			{
				string quarantineReason = "restored exact garrison patrol could not enter strategic hold";
				HST_ForceSpawnQueueCallbackResult terminal = m_SpawnQueue.CompleteQuarantinedSuccessfulProjectionCancellation(
					state.m_aForceSpawnResults,
					batch.m_sResultId,
					batch.m_sProjectionId,
					Math.Max(0, state.m_iElapsedSeconds),
					quarantineReason);
				if (terminal && terminal.m_bAccepted)
				{
					ClearGroupProcessAuthority(group);
					group.m_iLifecycleRevision++;
				}
				return QuarantineOperationAuthority(state, operation,
					quarantineReason);
			}
			changed = true;
		}
		else if (!batch.m_bStrategicProjectionHeld)
		{
			HST_ForceSpawnQueueCallbackResult heldPending = m_SpawnQueue.HoldPendingProjectionForStrategicSimulation(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				state.m_iElapsedSeconds);
			if (!heldPending || !heldPending.m_bAccepted)
				return QuarantineOperationAuthority(state, operation,
					"restored exact garrison patrol pending roster could not enter strategic hold");
			changed = true;
		}
		NormalizeHeldBatchProcessAuthority(batch, Math.Max(0, state.m_iElapsedSeconds));
		ClearGroupProcessAuthority(group);

		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iStrategicLastUpdateSecond = nowSecond;
		operation.m_iLastProjectionDecisionSecond = nowSecond;
		operation.m_iLastProgressAtSecond = nowSecond;
		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		operation.m_sLastProjectionReason = "restored exact garrison patrol as held strategic authority";
		operation.m_iRevision++;
		int living = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
		operation.m_iLastVirtualFriendlyCount = living;
		SyncGroupRoster(group, living);
		group.m_vPosition = operation.m_vStrategicPosition;
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		group.m_vTargetPosition = operation.m_vRouteEndPosition;
		ApplyVirtualRuntimeStatus(group);
		group.m_iLifecycleRevision++;
		return true;
	}

	protected string ResolveRuntimeContext(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		out HST_ForceQuoteState quote,
		out HST_ForceManifestState manifest,
		out HST_GarrisonState garrison,
		out HST_ForceSpawnResultState batch,
		out HST_ActiveGroupState group,
		out HST_GeneratedRouteState route)
	{
		quote = null;
		manifest = null;
		garrison = null;
		batch = null;
		group = null;
		route = null;
		if (!state || !operation)
			return "exact garrison patrol runtime context is missing";
		if (CountOperationIdentity(state, operation) != 1)
			return "exact garrison patrol operation identity is ambiguous";
		quote = state.FindForceQuote(operation.m_sQuoteId);
		manifest = state.FindForceManifest(operation.m_sManifestId);
		batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
		group = state.FindActiveGroup(operation.m_sGroupId);
		route = state.FindGeneratedRoute(operation.m_sCurrentRouteId);
		if (!quote || !manifest || !batch || !group || !route)
			return "exact garrison patrol reciprocal runtime graph is incomplete";
		if (CountQuoteIdentity(state, quote) != 1 || CountManifestIdentity(state, manifest) != 1
			|| CountBatchIdentity(state, batch) != 1 || CountGroupIdentity(state, group) != 1
			|| CountRouteIdentity(state, route) != 1)
			return "exact garrison patrol reciprocal runtime graph is ambiguous";
		if (!IsCurrentPolicyQuote(quote) || !IsCurrentPolicyManifest(manifest)
			|| quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED)
			return "exact garrison patrol runtime does not use the accepted v2 policy";
		if (quote.m_sQuoteId.IsEmpty() || quote.m_sOperationId.IsEmpty()
			|| quote.m_sManifestId.IsEmpty() || quote.m_sConfirmationRequestId.IsEmpty())
			return "exact garrison patrol planning identity is incomplete";
		if (quote.m_sOperationId != HST_StableIdService.BuildOperationId(
			"garrison_recruitment",
			quote.m_sQuoteId))
			return "exact garrison patrol deterministic operation identity conflicts";
		if (quote.m_sOperationId != operation.m_sOperationId
			|| quote.m_sOperationId != manifest.m_sOperationId
			|| quote.m_sManifestId != operation.m_sManifestId
			|| quote.m_sManifestId != manifest.m_sManifestId
			|| manifest.m_sQuoteId != quote.m_sQuoteId)
			return "exact garrison patrol planning backlinks conflict";
		string integrityFailure;
		if (!m_Integrity.ValidateFrozenGarrisonQuote(manifest, quote, false, integrityFailure))
			return "exact garrison patrol frozen v2 purchase conflicts: " + integrityFailure;

		if (operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| operation.m_iProjectionContractVersion != EXACT_PROJECTION_CONTRACT_VERSION)
			return "exact garrison patrol operation contract conflicts";
		if (operation.m_sOwnerFactionKey != quote.m_sFactionKey
			|| operation.m_sActorIdentityId != quote.m_sActorIdentityId
			|| operation.m_sIssueRequestId != quote.m_sCommandRequestId
			|| operation.m_sConfirmationRequestId != quote.m_sConfirmationRequestId)
			return "exact garrison patrol operation planning authority conflicts";
		if (operation.m_sOriginZoneId != quote.m_sTargetZoneId
			|| operation.m_sAssignmentZoneId != quote.m_sTargetZoneId
			|| operation.m_sTacticalTargetZoneId != quote.m_sTargetZoneId
			|| operation.m_sAssignmentKind != ASSIGNMENT_KIND
			|| operation.m_sRecallPolicyId != RECALL_POLICY_ID
			|| operation.m_sSettlementPolicyId != SETTLEMENT_POLICY_ID)
			return "exact garrison patrol immutable assignment policy conflicts";
		if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			|| operation.m_eResumeDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| !operation.m_sSettlementId.IsEmpty())
			return "exact garrison patrol open lifecycle conflicts";
		bool strategicPair = operation.m_ePositionAuthority
			== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		if (strategicPair)
		{
			strategicPair = operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				|| operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		}
		bool livePair = operation.m_ePositionAuthority
			== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		if (livePair)
		{
			livePair = operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
		}
		if (!strategicPair && !livePair)
			return "exact garrison patrol materialization and position authority conflict";

		HST_ZoneState zone = state.FindFrozenHistoricalZoneView(operation.m_sAssignmentZoneId);
		if (!zone || zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT
			|| zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE
			|| IsZeroVector(zone.m_vPosition))
			return "exact garrison patrol assignment zone is unavailable";
		if (!PositionsMatch(operation.m_vOriginPosition, zone.m_vPosition)
			|| !PositionsMatch(operation.m_vAssignmentPosition, zone.m_vPosition)
			|| !PositionsMatch(operation.m_vTacticalTargetPosition, zone.m_vPosition)
			|| !PositionsMatch(quote.m_vTargetPosition, zone.m_vPosition))
			return "exact garrison patrol immutable assignment position conflicts";

		garrison = state.FindGarrison(operation.m_sAssignmentZoneId, operation.m_sOwnerFactionKey);
		if (!garrison || garrison.m_sGarrisonId != HST_StableIdService.BuildGarrisonId(
			operation.m_sAssignmentZoneId,
			operation.m_sOwnerFactionKey))
			return "exact garrison patrol garrison authority is missing";
		if (CountGarrisonIdentity(state, garrison) != 1
			|| CountString(garrison.m_aAcceptedManifestIds, manifest.m_sManifestId) != 1
			|| CountGarrisonBacklinks(state, manifest.m_sManifestId) != 1)
			return "exact garrison patrol garrison backlink is ambiguous";

		if (!IsOwnedRoute(quote, route)
			|| operation.m_sCurrentRouteId != route.m_sRouteId
			|| group.m_sRouteId != route.m_sRouteId
			|| !m_RouteCursor.IsPatrolRouteContractValid(operation, route))
			return "exact garrison patrol owned local-route contract conflicts";
		ref array<vector> positions = HST_OperationRouteCursorService.BuildOrderedRoutePositions(route);
		if (positions.Count() != 4 || route.m_iWaypointCount != 4
			|| route.m_aWaypoints.Count() != 4)
			return "exact garrison patrol owned local-route geometry conflicts";
		if (operation.m_iRouteWaypointIndex < 0
			|| operation.m_iRouteWaypointIndex >= positions.Count()
			|| !PositionsMatch(operation.m_vRouteEndPosition,
				positions[operation.m_iRouteWaypointIndex], 2.0))
			return "exact garrison patrol persisted route cursor conflicts";

		if (operation.m_sSpawnResultId != BuildSpawnResultId(quote)
			|| operation.m_sForceId != BuildForceId(quote)
			|| operation.m_sProjectionId != BuildProjectionId(quote)
			|| operation.m_sGroupId != BuildProjectionId(quote))
			return "exact garrison patrol deterministic runtime identity conflicts";
		if (batch.m_sResultId != operation.m_sSpawnResultId
			|| batch.m_sRequestId != quote.m_sQuoteId
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| batch.m_sManifestHash != manifest.m_sManifestHash
			|| batch.m_sOperationId != operation.m_sOperationId
			|| batch.m_sForceId != operation.m_sForceId
			|| batch.m_sProjectionId != operation.m_sProjectionId)
			return "exact garrison patrol spawn batch backlinks conflict";
		bool exactGroupMode = group.m_sSpawnFallbackMode == EXACT_GROUP_MODE;
		if (!exactGroupMode)
			exactGroupMode = group.m_sSpawnFallbackMode.StartsWith(EXACT_GROUP_MODE + "_");
		if (group.m_sGroupId != operation.m_sGroupId
			|| group.m_sOperationId != operation.m_sOperationId
			|| group.m_sManifestId != manifest.m_sManifestId
			|| group.m_sSpawnResultId != batch.m_sResultId
			|| group.m_sForceId != batch.m_sForceId
			|| group.m_sProjectionId != batch.m_sProjectionId
			|| group.m_sGroupId != group.m_sProjectionId)
			return "exact garrison patrol active-group backlinks conflict";
		if (group.m_sZoneId != operation.m_sAssignmentZoneId
			|| group.m_sGarrisonZoneId != operation.m_sAssignmentZoneId
			|| group.m_sFactionKey != operation.m_sOwnerFactionKey
			|| group.m_sCompositionIntentId != EXACT_INTENT_ID
			|| group.m_sCompositionRequestId != manifest.m_sManifestId
			|| !exactGroupMode || group.m_bQRF)
			return "exact garrison patrol active-group role conflicts";
		if (group.m_sSpawnFallbackMode == EXACT_GROUP_MODE + "_quarantined"
			|| group.m_sRuntimeStatus == "exact_garrison_patrol_quarantined")
			return "exact garrison patrol group is quarantined while its operation remains current";
		if (!group.m_sEnemyOrderId.IsEmpty() || !group.m_sMissionInstanceId.IsEmpty()
			|| !group.m_sSupportRequestId.IsEmpty() || !group.m_sQRFInstanceId.IsEmpty()
			|| !group.m_sConvoyElementId.IsEmpty() || group.m_iVehicleCount != 0)
			return "exact garrison patrol active-group foreign authority conflicts";
		if (manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0]
			|| group.m_sPrefab != manifest.m_aGroups[0].m_sPrefab
			|| group.m_iOriginalInfantryCount != manifest.m_iAcceptedMemberCount
			|| group.m_iCompositionManpower != manifest.m_iAcceptedMemberCount)
			return "exact garrison patrol active-group roster contract conflicts";

		string rosterFailure = ValidateBatchRosterBijection(manifest, batch);
		if (!rosterFailure.IsEmpty())
			return rosterFailure;
		bool virtual = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		bool materializing = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		bool physical = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		bool dematerializing = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
		if (virtual && (!batch.m_bStrategicProjectionHeld
			|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING))
			return "exact garrison patrol virtual batch is not an idle strategic hold";
		if (materializing && batch.m_bStrategicProjectionHeld)
			return "exact garrison patrol materializing batch remains strategically held";
		if ((physical || dematerializing) && batch.m_bStrategicProjectionHeld)
			return "exact garrison patrol live batch authority conflicts";
		if (physical || dematerializing)
		{
			bool liveBatchStatus = batch.m_eStatus
				== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
			if (!liveBatchStatus)
				liveBatchStatus = batch.m_eStatus
					== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
			if (!liveBatchStatus)
				liveBatchStatus = batch.m_eStatus
					== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED;
			if (!liveBatchStatus)
				return "exact garrison patrol live batch status conflicts";
		}
		return "";
	}

	protected string ValidateBatchRosterBijection(
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		if (!manifest || !batch || manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
			return "exact garrison patrol roster contract is incomplete";
		int expectedSlots = manifest.m_aMembers.Count() + 1;
		if (batch.m_iExpectedSlotCount != expectedSlots
			|| batch.m_aSlotResults.Count() != expectedSlots)
			return "exact garrison patrol durable roster slot count conflicts";
		string rootSlotId = manifest.m_aGroups[0].m_sElementId;
		if (CountBatchSlots(batch, rootSlotId, HST_ForceSpawnQueueService.SLOT_KIND_GROUP) != 1)
			return "exact garrison patrol group-root slot is not unique";
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member || CountManifestMembers(manifest, member.m_sSlotId) != 1
				|| CountBatchSlots(batch, member.m_sSlotId,
					HST_ForceSpawnQueueService.SLOT_KIND_MEMBER) != 1)
				return "exact garrison patrol member-slot bijection conflicts";
		}
		bool terminalBatch = batch.m_eStatus
			== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
		if (!terminalBatch)
			terminalBatch = batch.m_eStatus
				== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_sSlotId.IsEmpty()
				|| slot.m_sProjectionId != batch.m_sProjectionId)
				return "exact garrison patrol roster contains an invalid slot identity";
			bool rootSlot = slot.m_sSlotId == rootSlotId;
			if (rootSlot)
				rootSlot = slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP;
			bool memberSlot = manifest.FindMemberSlot(slot.m_sSlotId) != null;
			if (memberSlot)
				memberSlot = slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER;
			if (!rootSlot && !memberSlot)
				return "exact garrison patrol roster contains a foreign slot";
			if (slot.m_bCasualtyConfirmed)
			{
				if (!memberSlot
					|| slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
					|| !slot.m_bEverAlive
					|| slot.m_iCasualtyAtSecond < batch.m_iCreatedAtSecond)
					return "exact garrison patrol casualty tombstone conflicts";
				continue;
			}
			if (!terminalBatch && (slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				|| slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_FINAL
				|| slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CANCELLED))
				return "exact garrison patrol open roster contains an unproven terminal slot";
			if (memberSlot
				&& slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
				&& !slot.m_bEverAlive)
				return "exact garrison patrol registered member lacks living evidence";
		}
		return "";
	}

	protected bool QuarantineOperationAuthority(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		string reason)
	{
		if (!state || !operation)
			return false;
		string prefix = "exact garrison patrol authority quarantined without refund or legacy conversion: ";
		string failure = reason;
		if (failure.IsEmpty())
			failure = "unspecified authority conflict";
		if (!failure.StartsWith(prefix))
			failure = prefix + failure;
		bool changed = operation.m_iContractVersion != QUARANTINED_CONTRACT_VERSION;
		if (operation.m_sLastProjectionReason != failure)
			changed = true;
		operation.m_iContractVersion = QUARANTINED_CONTRACT_VERSION;
		operation.m_sLastProjectionReason = failure;
		operation.m_iRevision++;
		return ApplyQuarantineStatus(state, operation, failure) || changed;
	}

	protected bool ApplyQuarantineStatus(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		string reason)
	{
		if (!state || !operation)
			return false;
		if (reason.IsEmpty())
			reason = "exact garrison patrol authority remains quarantined";
		bool changed;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (!BatchClaimsOperationAuthority(batch, operation))
				continue;
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			{
				bool retiredProjectionChanged;
				TryRetireQuarantinedSuccessfulProjection(
					state,
					operation,
					batch,
					reason,
					retiredProjectionChanged);
				changed = retiredProjectionChanged || changed;
			}
			if (m_SpawnQueue)
			{
				HST_ForceSpawnQueueCallbackResult cancelled = m_SpawnQueue.RequestCancel(
					state.m_aForceSpawnResults,
					batch.m_sResultId,
					state.m_iElapsedSeconds,
					reason);
				if (cancelled && cancelled.m_bStateChanged)
					changed = true;
			}
			else if (!batch.m_bCancelRequested || batch.m_sLastFailureReason != reason)
			{
				batch.m_bCancelRequested = true;
				batch.m_sLastFailureReason = reason;
				changed = true;
			}
		}
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (!GroupClaimsOperationAuthority(group, operation))
				continue;
			if (group.m_sRuntimeStatus != "exact_garrison_patrol_quarantined"
				|| group.m_sSpawnFailureReason != reason
				|| group.m_sSpawnFallbackMode != EXACT_GROUP_MODE + "_quarantined")
			{
				group.m_sRuntimeStatus = "exact_garrison_patrol_quarantined";
				group.m_sSpawnFailureReason = reason;
				group.m_sSpawnFallbackMode = EXACT_GROUP_MODE + "_quarantined";
				group.m_iLifecycleRevision++;
				changed = true;
			}
		}
		return changed;
	}

	protected bool TryRetireQuarantinedSuccessfulProjection(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		string reason,
		out bool changed)
	{
		changed = false;
		if (!state || !operation || !batch || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar
			|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			return false;
		HST_ActiveGroupState group;
		if (!ResolveQuarantinedSuccessfulProjectionContext(
			state,
			operation,
			batch,
			group))
			return false;

		string originalOperationId = group.m_sOperationId;
		string originalManifestId = group.m_sManifestId;
		string originalSpawnResultId = group.m_sSpawnResultId;
		string originalForceId = group.m_sForceId;
		string originalBatchOperationId = batch.m_sOperationId;
		string originalBatchManifestId = batch.m_sManifestId;
		string originalBatchForceId = batch.m_sForceId;
		string runtimeOperationId = batch.m_sOperationId;
		if (runtimeOperationId.IsEmpty())
			runtimeOperationId = "quarantine_runtime_operation_" + operation.m_sProjectionId;
		string runtimeManifestId = batch.m_sManifestId;
		if (runtimeManifestId.IsEmpty())
			runtimeManifestId = "quarantine_runtime_manifest_" + operation.m_sProjectionId;
		string runtimeForceId = batch.m_sForceId;
		if (runtimeForceId.IsEmpty())
			runtimeForceId = "quarantine_runtime_force_" + operation.m_sProjectionId;
		batch.m_sOperationId = runtimeOperationId;
		batch.m_sManifestId = runtimeManifestId;
		batch.m_sForceId = runtimeForceId;
		group.m_sOperationId = runtimeOperationId;
		group.m_sManifestId = runtimeManifestId;
		group.m_sSpawnResultId = batch.m_sResultId;
		group.m_sForceId = runtimeForceId;
		bool retired = RetireQuarantinedSuccessfulProjectionWithRuntimeIdentity(
			state,
			operation,
			batch,
			group,
			reason,
			changed);
		group.m_sOperationId = originalOperationId;
		group.m_sManifestId = originalManifestId;
		group.m_sSpawnResultId = originalSpawnResultId;
		group.m_sForceId = originalForceId;
		batch.m_sOperationId = originalBatchOperationId;
		batch.m_sManifestId = originalBatchManifestId;
		batch.m_sForceId = originalBatchForceId;
		return retired;
	}

	protected bool RetireQuarantinedSuccessfulProjectionWithRuntimeIdentity(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason,
		out bool changed)
	{
		changed = false;
		string runtimeKeyFailure;
		if (!m_SpawnAdapter.ValidateExactProjectionRuntimeKeys(batch, runtimeKeyFailure))
			return false;
		HST_ForceSpawnAdapterTickResult reconciled = m_SpawnAdapter.ReconcileQuarantinedExactInfantryProjectionAuthority(
			state,
			m_SpawnQueue,
			m_PhysicalWar,
			Math.Max(0, state.m_iElapsedSeconds),
			operation.m_sProjectionId);
		if (reconciled)
			changed = reconciled.m_bStateChanged || reconciled.m_bRuntimeChanged;
		if (!reconciled || reconciled.m_iFailedCount > 0)
			return false;

		int handles = m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId);
		bool physicalRuntime = m_PhysicalWar.GetForceSpawnGroupRoot(group)
			|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0;
		int living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
		if (handles <= 0 && !physicalRuntime && living > 0)
			return false;
		if (handles > 0 || physicalRuntime)
		{
			string bindingFailure;
			if (!m_SpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
				state,
				batch,
				m_SpawnQueue,
				m_PhysicalWar,
				bindingFailure))
				return false;
			HST_ForceSpawnAdapterRetireResult runtimeRetired = m_SpawnAdapter.RetireProjectionRuntime(
				state,
				m_PhysicalWar,
				operation.m_sProjectionId);
			if (!runtimeRetired || !runtimeRetired.m_bSuccess)
				return false;
			changed = runtimeRetired.m_bRuntimeChanged || changed;
		}
		if (m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0
			|| m_SpawnAdapter.CountHandlesForResultId(batch.m_sResultId) > 0
			|| m_PhysicalWar.GetForceSpawnGroupRoot(group)
			|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0)
			return false;

		ClearGroupProcessAuthority(group);
		group.m_iLifecycleRevision++;
		changed = true;
		HST_ForceSpawnQueueCallbackResult terminal = m_SpawnQueue.CompleteQuarantinedSuccessfulProjectionCancellation(
			state.m_aForceSpawnResults,
			batch.m_sResultId,
			batch.m_sProjectionId,
			Math.Max(0, state.m_iElapsedSeconds),
			reason);
		if (!terminal || !terminal.m_bAccepted)
			return false;
		changed = terminal.m_bStateChanged || changed;
		return true;
	}

	protected bool ResolveQuarantinedSuccessfulProjectionContext(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		out HST_ActiveGroupState group)
	{
		group = null;
		if (!state || !operation || !batch
			|| operation.m_sProjectionId.IsEmpty()
			|| batch.m_sResultId.IsEmpty())
			return false;

		int batchMatches;
		int resultMatches;
		foreach (HST_ForceSpawnResultState candidateBatch : state.m_aForceSpawnResults)
		{
			if (!candidateBatch)
				continue;
			if (candidateBatch.m_sProjectionId == operation.m_sProjectionId)
				batchMatches++;
			if (candidateBatch.m_sResultId == batch.m_sResultId)
				resultMatches++;
		}
		if (batchMatches != 1 || resultMatches != 1
			|| batch.m_sProjectionId != operation.m_sProjectionId
			|| !BatchClaimsOperationAuthority(batch, operation))
			return false;

		int groupMatches;
		foreach (HST_ActiveGroupState candidateGroup : state.m_aActiveGroups)
		{
			if (!candidateGroup || candidateGroup.m_sProjectionId != operation.m_sProjectionId)
				continue;
			groupMatches++;
			group = candidateGroup;
		}
		if (groupMatches != 1 || !group || group.m_sGroupId.IsEmpty()
			|| !GroupClaimsOperationAuthority(group, operation))
			return false;
		int groupIdMatches;
		foreach (HST_ActiveGroupState groupIdCandidate : state.m_aActiveGroups)
		{
			if (groupIdCandidate && groupIdCandidate.m_sGroupId == group.m_sGroupId)
				groupIdMatches++;
		}
		if (groupIdMatches != 1)
			return false;

		foreach (HST_OperationRecordState competingOperation : state.m_aOperations)
		{
			if (!competingOperation || competingOperation == operation)
				continue;
			if (competingOperation.m_sProjectionId == operation.m_sProjectionId
				|| (!batch.m_sResultId.IsEmpty()
					&& competingOperation.m_sSpawnResultId == batch.m_sResultId)
				|| (!group.m_sGroupId.IsEmpty()
					&& competingOperation.m_sGroupId == group.m_sGroupId)
				|| (!batch.m_sOperationId.IsEmpty()
					&& competingOperation.m_sOperationId == batch.m_sOperationId)
				|| (!group.m_sOperationId.IsEmpty()
					&& competingOperation.m_sOperationId == group.m_sOperationId)
				|| (!batch.m_sManifestId.IsEmpty()
					&& competingOperation.m_sManifestId == batch.m_sManifestId)
				|| (!group.m_sManifestId.IsEmpty()
					&& competingOperation.m_sManifestId == group.m_sManifestId)
				|| (!batch.m_sForceId.IsEmpty()
					&& competingOperation.m_sForceId == batch.m_sForceId)
				|| (!group.m_sForceId.IsEmpty()
					&& competingOperation.m_sForceId == group.m_sForceId))
				return false;
		}
		return true;
	}

	protected bool BatchClaimsOperationAuthority(
		HST_ForceSpawnResultState batch,
		HST_OperationRecordState operation)
	{
		if (!batch || !operation)
			return false;
		if (operation.m_sOperationId.IsEmpty()
			|| batch.m_sOperationId != operation.m_sOperationId)
			return false;
		return (!operation.m_sSpawnResultId.IsEmpty()
				&& batch.m_sResultId == operation.m_sSpawnResultId)
			|| (!operation.m_sManifestId.IsEmpty()
				&& batch.m_sManifestId == operation.m_sManifestId)
			|| (!operation.m_sForceId.IsEmpty()
				&& batch.m_sForceId == operation.m_sForceId)
			|| (!operation.m_sProjectionId.IsEmpty()
				&& batch.m_sProjectionId == operation.m_sProjectionId);
	}

	protected bool GroupClaimsOperationAuthority(
		HST_ActiveGroupState group,
		HST_OperationRecordState operation)
	{
		if (!group || !operation)
			return false;
		if (operation.m_sOperationId.IsEmpty()
			|| group.m_sOperationId != operation.m_sOperationId)
			return false;
		return (!operation.m_sGroupId.IsEmpty()
				&& group.m_sGroupId == operation.m_sGroupId)
			|| (!operation.m_sManifestId.IsEmpty()
				&& group.m_sManifestId == operation.m_sManifestId)
			|| (!operation.m_sSpawnResultId.IsEmpty()
				&& group.m_sSpawnResultId == operation.m_sSpawnResultId)
			|| (!operation.m_sForceId.IsEmpty()
				&& group.m_sForceId == operation.m_sForceId)
			|| (!operation.m_sProjectionId.IsEmpty()
				&& group.m_sProjectionId == operation.m_sProjectionId);
	}

	protected void ApplyVirtualRuntimeStatus(HST_ActiveGroupState group)
	{
		if (!group)
			return;
		group.m_sRuntimeStatus = "garrison_patrol_virtual";
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

	protected int ResolveLivingRoster(
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_OperationRecordState operation)
	{
		int accepted;
		if (manifest)
			accepted = Math.Max(0, manifest.m_iAcceptedMemberCount);
		else if (group)
			accepted = Math.Max(0, group.m_iOriginalInfantryCount);
		int living;
		bool authoritativeRoster;
		if (batch && m_SpawnQueue)
		{
			if (batch.m_bStrategicProjectionHeld)
			{
				living = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
				authoritativeRoster = true;
			}
			else if (batch.m_iSuccessfulHandoffCount > 0)
			{
				living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
				authoritativeRoster = true;
			}
		}
		if (!authoritativeRoster && operation)
			living = operation.m_iLastVirtualFriendlyCount;
		if (!authoritativeRoster && living <= 0 && group
			&& group.m_sRuntimeStatus != "eliminated")
		{
			living = Math.Max(
				group.m_iDurableLivingInfantryCount,
				group.m_iSurvivorInfantryCount);
		}
		return Math.Max(0, Math.Min(accepted, living));
	}

	protected void ClearGroupProcessAuthority(HST_ActiveGroupState group)
	{
		if (!group)
			return;
		group.m_bSpawnedEntity = false;
		group.m_bSpawnAttempted = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
	}

	protected void NormalizeHeldBatchProcessAuthority(
		HST_ForceSpawnResultState batch,
		int nowSecond)
	{
		if (!batch)
			return;
		batch.m_sNativeGroupId = "";
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING;
		batch.m_bStrategicProjectionHeld = true;
		batch.m_bCancelRequested = false;
		batch.m_iStrategicHoldSinceSecond = nowSecond;
		batch.m_iNextAttemptSecond = 0;
		batch.m_iUpdatedAtSecond = nowSecond;
		batch.m_iCompletedAtSecond = 0;
		batch.m_iLastLifecycleSecond = nowSecond;
		batch.m_iLifecycleRevision++;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot)
				continue;
			slot.m_sSpawnedPrefab = "";
			slot.m_sEntityId = "";
			slot.m_sAssignedVehicleEntityId = "";
			slot.m_sNativeGroupId = "";
			slot.m_bAliveVerified = false;
			slot.m_bFactionVerified = false;
			slot.m_bGroupVerified = false;
			slot.m_bGameMasterVerified = false;
			slot.m_bProjectionVerified = false;
			slot.m_bSeatVerified = false;
			slot.m_iUpdatedAtSecond = nowSecond;
			if (slot.m_bCasualtyConfirmed)
			{
				slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
				continue;
			}
			slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED;
		}
	}

	protected int CountGarrisonIdentity(
		HST_CampaignState state,
		HST_GarrisonState expected)
	{
		int count;
		if (!state || !expected)
			return count;
		foreach (HST_GarrisonState garrison : state.m_aGarrisons)
		{
			if (garrison && (garrison.m_sGarrisonId == expected.m_sGarrisonId
				|| (garrison.m_sZoneId == expected.m_sZoneId
					&& garrison.m_sFactionKey == expected.m_sFactionKey)))
				count++;
		}
		return count;
	}

	protected int CountGarrisonBacklinks(HST_CampaignState state, string manifestId)
	{
		int count;
		if (!state || manifestId.IsEmpty())
			return count;
		foreach (HST_GarrisonState garrison : state.m_aGarrisons)
		{
			if (garrison)
				count += CountString(garrison.m_aAcceptedManifestIds, manifestId);
		}
		return count;
	}

	protected int CountManifestMembers(
		HST_ForceManifestState manifest,
		string slotId)
	{
		int count;
		if (!manifest || slotId.IsEmpty())
			return count;
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (member && member.m_sSlotId == slotId)
				count++;
		}
		return count;
	}

	protected int CountBatchSlots(
		HST_ForceSpawnResultState batch,
		string slotId,
		string slotKind)
	{
		int count;
		if (!batch || slotId.IsEmpty() || slotKind.IsEmpty())
			return count;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotId == slotId && slot.m_sSlotKind == slotKind)
				count++;
		}
		return count;
	}

	protected bool PositionsMatch(
		vector left,
		vector right,
		float toleranceMeters = 0.5)
	{
		return Distance2D(left, right) <= toleranceMeters;
	}

	protected string ValidateAdmissionContext(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_GarrisonService garrisons,
		string confirmationRequestId)
	{
		if (!state || !quote || !manifest || !garrisons || !m_SpawnQueue
			|| !m_SpawnAdapter || !m_PhysicalWar || !m_Integrity)
			return "exact garrison patrol admission services are unavailable";
		if (confirmationRequestId.IsEmpty())
			return "exact garrison patrol confirmation identity is missing";
		if (quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED
			|| !IsCurrentPolicyQuote(quote) || !IsCurrentPolicyManifest(manifest))
			return "garrison purchase did not opt into the current exact patrol policy";
		if (quote.m_sQuoteId.IsEmpty() || quote.m_sOperationId.IsEmpty()
			|| quote.m_sOperationId != HST_StableIdService.BuildOperationId("garrison_recruitment", quote.m_sQuoteId)
			|| quote.m_sManifestId != manifest.m_sManifestId
			|| quote.m_sOperationId != manifest.m_sOperationId
			|| quote.m_sTargetZoneId != manifest.m_sTargetZoneId
			|| quote.m_sFactionKey != manifest.m_sFactionKey)
			return "exact garrison patrol purchase identity is invalid";
		if (state.FindForceQuote(quote.m_sQuoteId) != quote
			|| state.FindForceManifest(manifest.m_sManifestId) != manifest
			|| CountQuoteIdentity(state, quote) != 1 || CountManifestIdentity(state, manifest) != 1)
			return "exact garrison patrol planning authority is ambiguous";
		string integrityFailure;
		if (!m_Integrity.ValidateFrozenGarrisonQuote(manifest, quote, true, integrityFailure))
			return "exact garrison patrol frozen manifest conflicts: " + integrityFailure;
		if (HST_MaidensBayLocationSaveValidationService.IsLegacyZoneId(quote.m_sTargetZoneId))
			return "exact garrison patrol quote targets a retired location; cancel or let the quote expire and issue a new warehouse quote";

		HST_ZoneState zone = state.FindZone(quote.m_sTargetZoneId);
		if (!zone || zone.m_sOwnerFactionKey != quote.m_sFactionKey
			|| zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT
			|| zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
			return "exact garrison patrol assignment zone is invalid";
		HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, quote.m_sFactionKey);
		int legacyInfantry;
		if (garrison)
			legacyInfantry = Math.Max(0, garrison.m_iInfantryCount);
		int exactInfantry = garrisons.CountExecutableManifestInfantry(state, garrison);
		if (zone.m_iGarrisonSlots > 0
			&& legacyInfantry + exactInfantry + Math.Max(0, zone.m_iActiveInfantryCount)
				+ manifest.m_iAcceptedMemberCount > zone.m_iGarrisonSlots)
			return "exact garrison patrol capacity changed before admission";
		return "";
	}

	protected string FindAdmissionIdentityCollision(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		if (!state || !quote || !manifest)
			return "exact garrison patrol collision context is missing";
		string resultId = BuildSpawnResultId(quote);
		string forceId = BuildForceId(quote);
		string projectionId = BuildProjectionId(quote);
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (operation && (operation.m_sOperationId == quote.m_sOperationId
				|| operation.m_sQuoteId == quote.m_sQuoteId
				|| operation.m_sManifestId == manifest.m_sManifestId
				|| operation.m_sSpawnResultId == resultId
				|| operation.m_sForceId == forceId
				|| operation.m_sProjectionId == projectionId
				|| operation.m_sGroupId == projectionId))
				return "exact garrison patrol identity is already claimed by an operation";
		}
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && (batch.m_sResultId == resultId
				|| batch.m_sRequestId == quote.m_sQuoteId
				|| batch.m_sOperationId == quote.m_sOperationId
				|| batch.m_sManifestId == manifest.m_sManifestId
				|| batch.m_sForceId == forceId
				|| batch.m_sProjectionId == projectionId))
				return "exact garrison patrol identity is already claimed by a spawn batch";
		}
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && (group.m_sGroupId == projectionId
				|| group.m_sOperationId == quote.m_sOperationId
				|| group.m_sManifestId == manifest.m_sManifestId
				|| group.m_sSpawnResultId == resultId
				|| group.m_sForceId == forceId
				|| group.m_sProjectionId == projectionId))
				return "exact garrison patrol identity is already claimed by an active group";
		}
		foreach (HST_GarrisonState garrison : state.m_aGarrisons)
		{
			if (garrison && garrison.m_aAcceptedManifestIds.Contains(manifest.m_sManifestId))
				return "exact garrison patrol manifest is already linked to a garrison";
		}
		foreach (HST_GeneratedRouteState route : state.m_aGeneratedRoutes)
		{
			if (route && (route.m_sRouteId == BuildRouteId(quote)
				|| route.m_sSourceLayoutId == quote.m_sOperationId))
				return "exact garrison patrol identity is already claimed by a generated route";
		}
		return "";
	}

	protected string ValidateCommittedLinks(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_GarrisonState garrison,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_GeneratedRouteState route)
	{
		if (!state || !quote || !manifest || !IsCurrentPolicyQuote(quote)
			|| !IsCurrentPolicyManifest(manifest)
			|| quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED
			|| state.FindForceQuote(quote.m_sQuoteId) != quote
			|| state.FindForceManifest(manifest.m_sManifestId) != manifest
			|| CountQuoteIdentity(state, quote) != 1
			|| CountManifestIdentity(state, manifest) != 1)
			return "exact garrison patrol committed v2 planning authority conflicts";
		string integrityFailure;
		if (!m_Integrity.ValidateFrozenGarrisonQuote(manifest, quote, false, integrityFailure))
			return "exact garrison patrol committed manifest conflicts: " + integrityFailure;
		if (!operation
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| operation.m_iProjectionContractVersion != EXACT_PROJECTION_CONTRACT_VERSION
			|| operation.m_sOperationId != quote.m_sOperationId
			|| operation.m_sQuoteId != quote.m_sQuoteId
			|| operation.m_sManifestId != manifest.m_sManifestId)
			return "exact garrison patrol operation identity conflicts";
		if (operation.m_sOwnerFactionKey != quote.m_sFactionKey
			|| operation.m_sActorIdentityId != quote.m_sActorIdentityId
			|| operation.m_sIssueRequestId != quote.m_sCommandRequestId
			|| operation.m_sConfirmationRequestId != quote.m_sConfirmationRequestId)
			return "exact garrison patrol operation planning authority conflicts";
		if (operation.m_sOriginZoneId != quote.m_sTargetZoneId
			|| operation.m_sAssignmentZoneId != quote.m_sTargetZoneId
			|| operation.m_sTacticalTargetZoneId != quote.m_sTargetZoneId
			|| operation.m_sAssignmentKind != ASSIGNMENT_KIND
			|| operation.m_sRecallPolicyId != RECALL_POLICY_ID
			|| operation.m_sSettlementPolicyId != SETTLEMENT_POLICY_ID)
			return "exact garrison patrol operation assignment policy conflicts";
		if (!PositionsMatch(operation.m_vOriginPosition, quote.m_vTargetPosition)
			|| !PositionsMatch(operation.m_vAssignmentPosition, quote.m_vTargetPosition)
			|| !PositionsMatch(operation.m_vTacticalTargetPosition, quote.m_vTargetPosition))
			return "exact garrison patrol operation assignment position conflicts";
		if (operation.m_sSpawnResultId != BuildSpawnResultId(quote)
			|| operation.m_sForceId != BuildForceId(quote)
			|| operation.m_sProjectionId != BuildProjectionId(quote)
			|| operation.m_sGroupId != BuildProjectionId(quote))
			return "exact garrison patrol deterministic runtime identity conflicts";
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return ValidateSettledCommittedLinks(state, quote, manifest, garrison, operation, batch, group, route);
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| !operation.m_sSettlementId.IsEmpty()
			|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			|| operation.m_eResumeDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
			return "exact garrison patrol open lifecycle conflicts";
		if (!garrison || CountString(garrison.m_aAcceptedManifestIds, manifest.m_sManifestId) != 1
			|| garrison.m_sZoneId != quote.m_sTargetZoneId
			|| garrison.m_sFactionKey != quote.m_sFactionKey
			|| CountGarrisonIdentity(state, garrison) != 1
			|| CountGarrisonBacklinks(state, manifest.m_sManifestId) != 1)
			return "exact garrison patrol garrison backlink conflicts";
		if (!batch || !group || !route
			|| operation.m_sSpawnResultId != batch.m_sResultId
			|| operation.m_sForceId != batch.m_sForceId
			|| operation.m_sProjectionId != batch.m_sProjectionId
			|| operation.m_sGroupId != group.m_sGroupId)
			return "exact garrison patrol operation links conflict";
		ref array<vector> routePositions = HST_OperationRouteCursorService.BuildOrderedRoutePositions(route);
		if (!IsOwnedRoute(quote, route) || CountRouteIdentity(state, route) != 1
			|| routePositions.Count() < 3
			|| operation.m_sCurrentRouteId != route.m_sRouteId
			|| operation.m_sRouteContractHash != HST_OperationRouteCursorService.BuildRouteContractHash(route, routePositions)
			|| !m_RouteCursor.IsPatrolRouteContractValid(operation, route)
			|| group.m_sRouteId != route.m_sRouteId)
			return "exact garrison patrol route authority conflicts";
		if (batch.m_sResultId != BuildSpawnResultId(quote)
			|| batch.m_sRequestId != quote.m_sQuoteId
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| batch.m_sManifestHash != manifest.m_sManifestHash
			|| batch.m_sOperationId != quote.m_sOperationId
			|| batch.m_sForceId != BuildForceId(quote)
			|| batch.m_sProjectionId != BuildProjectionId(quote))
			return "exact garrison patrol spawn batch links conflict";
		if (group.m_sGroupId != batch.m_sProjectionId
			|| group.m_sOperationId != quote.m_sOperationId
			|| group.m_sManifestId != manifest.m_sManifestId
			|| group.m_sSpawnResultId != batch.m_sResultId
			|| group.m_sForceId != batch.m_sForceId
			|| group.m_sProjectionId != batch.m_sProjectionId
			|| group.m_sGarrisonZoneId != quote.m_sTargetZoneId)
			return "exact garrison patrol active-group links conflict";
		if (CountOperationIdentity(state, operation) != 1
			|| CountBatchIdentity(state, batch) != 1
			|| CountGroupIdentity(state, group) != 1)
			return "exact garrison patrol committed authority is ambiguous";
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& !batch.m_bStrategicProjectionHeld)
			return "exact virtual garrison patrol projection is not strategically held";
		string rosterFailure = ValidateBatchRosterBijection(manifest, batch);
		if (!rosterFailure.IsEmpty())
			return rosterFailure;
		HST_ForceQuoteState resolvedQuote;
		HST_ForceManifestState resolvedManifest;
		HST_GarrisonState resolvedGarrison;
		HST_ForceSpawnResultState resolvedBatch;
		HST_ActiveGroupState resolvedGroup;
		HST_GeneratedRouteState resolvedRoute;
		string runtimeFailure = ResolveRuntimeContext(
			state,
			operation,
			resolvedQuote,
			resolvedManifest,
			resolvedGarrison,
			resolvedBatch,
			resolvedGroup,
			resolvedRoute);
		if (!runtimeFailure.IsEmpty())
			return "exact garrison patrol committed runtime conflicts: " + runtimeFailure;
		if (resolvedQuote != quote || resolvedManifest != manifest
			|| resolvedGarrison != garrison || resolvedBatch != batch
			|| resolvedGroup != group || resolvedRoute != route)
			return "exact garrison patrol committed runtime graph pointer conflicts";
		return "";
	}

	protected string ValidateSettledCommittedLinks(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_GarrisonState garrison,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_GeneratedRouteState route)
	{
		if (!state || !quote || !manifest || !operation
			|| operation.m_sSettlementId != HST_OperationService.BuildSettlementId(
				operation.m_sOperationId,
				SETTLEMENT_KIND)
			|| operation.m_iSettledAtSecond < operation.m_iCreatedAtSecond
			|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN
			|| CountOperationIdentity(state, operation) != 1)
			return "exact garrison patrol settled receipt conflicts";
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			|| operation.m_eResumeDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			|| operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return "exact garrison patrol settled lifecycle conflicts";
		if (operation.m_iProjectionContractVersion != EXACT_PROJECTION_CONTRACT_VERSION
			|| operation.m_sOwnerFactionKey != quote.m_sFactionKey
			|| operation.m_sActorIdentityId != quote.m_sActorIdentityId
			|| operation.m_sIssueRequestId != quote.m_sCommandRequestId
			|| operation.m_sConfirmationRequestId != quote.m_sConfirmationRequestId)
			return "exact garrison patrol settled planning authority conflicts";
		if (operation.m_sOriginZoneId != quote.m_sTargetZoneId
			|| operation.m_sAssignmentZoneId != quote.m_sTargetZoneId
			|| operation.m_sTacticalTargetZoneId != quote.m_sTargetZoneId
			|| operation.m_sAssignmentKind != ASSIGNMENT_KIND
			|| operation.m_sRecallPolicyId != RECALL_POLICY_ID
			|| operation.m_sSettlementPolicyId != SETTLEMENT_POLICY_ID)
			return "exact garrison patrol settled assignment policy conflicts";
		if (operation.m_sSpawnResultId != BuildSpawnResultId(quote)
			|| operation.m_sForceId != BuildForceId(quote)
			|| operation.m_sProjectionId != BuildProjectionId(quote)
			|| operation.m_sGroupId != BuildProjectionId(quote))
			return "exact garrison patrol settled deterministic runtime identity conflicts";

		int backlinkCount = CountGarrisonBacklinks(state, manifest.m_sManifestId);
		if (backlinkCount > 1)
			return "exact garrison patrol settled garrison backlink conflicts";
		if (backlinkCount == 1 && !garrison)
			return "exact garrison patrol settled garrison backlink conflicts";
		if (garrison)
		{
			int localBacklinkCount = CountString(garrison.m_aAcceptedManifestIds, manifest.m_sManifestId);
			if (localBacklinkCount != backlinkCount
				|| localBacklinkCount > 1 || (localBacklinkCount == 1
				&& (garrison.m_sZoneId != quote.m_sTargetZoneId
					|| garrison.m_sFactionKey != quote.m_sFactionKey
					|| CountGarrisonIdentity(state, garrison) != 1)))
				return "exact garrison patrol settled garrison backlink conflicts";
		}

		if (route)
		{
			ref array<vector> positions = HST_OperationRouteCursorService.BuildOrderedRoutePositions(route);
			if (!IsOwnedRoute(quote, route) || CountRouteIdentity(state, route) != 1
				|| positions.Count() < 3
				|| operation.m_sCurrentRouteId != route.m_sRouteId
				|| operation.m_sRouteContractHash != HST_OperationRouteCursorService.BuildRouteContractHash(route, positions))
				return "exact garrison patrol settled route evidence conflicts";
		}
		if (batch)
		{
			if (batch.m_sResultId != BuildSpawnResultId(quote)
				|| batch.m_sRequestId != quote.m_sQuoteId
				|| batch.m_sManifestId != manifest.m_sManifestId
				|| batch.m_sOperationId != quote.m_sOperationId
				|| batch.m_sForceId != BuildForceId(quote)
				|| batch.m_sProjectionId != BuildProjectionId(quote)
				|| CountBatchIdentity(state, batch) != 1)
				return "exact garrison patrol settled batch evidence conflicts";
		}
		if (group)
		{
			if (group.m_sGroupId != BuildProjectionId(quote)
				|| group.m_sOperationId != quote.m_sOperationId
				|| group.m_sManifestId != manifest.m_sManifestId
				|| group.m_sGarrisonZoneId != quote.m_sTargetZoneId
				|| CountGroupIdentity(state, group) != 1)
				return "exact garrison patrol settled group evidence conflicts";
		}
		return "";
	}

	protected bool HasCommittedAdmissionAuthority(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		if (!state || !quote || !manifest)
			return false;
		HST_GarrisonState garrison = state.FindGarrison(quote.m_sTargetZoneId, quote.m_sFactionKey);
		if (garrison && garrison.m_aAcceptedManifestIds.Contains(manifest.m_sManifestId))
			return true;
		if (state.FindOperation(quote.m_sOperationId))
			return true;
		if (state.FindForceSpawnResultByManifest(manifest.m_sManifestId))
			return true;
		return state.FindActiveGroup(BuildProjectionId(quote)) != null;
	}

	protected HST_ForceSpawnQueueRequest BuildSpawnRequest(
		HST_CampaignState state,
		HST_ForceQuoteState quote)
	{
		HST_ForceSpawnQueueRequest request = new HST_ForceSpawnQueueRequest();
		if (!state || !quote)
			return request;
		request.m_sResultId = BuildSpawnResultId(quote);
		request.m_sRequestId = quote.m_sQuoteId;
		request.m_sForceId = BuildForceId(quote);
		request.m_sProjectionId = BuildProjectionId(quote);
		request.m_iPriority = EXACT_PRIORITY;
		request.m_iMaxRetries = EXACT_MAX_RETRIES;
		request.m_iDeadlineSecond = Math.Max(0, state.m_iElapsedSeconds) + DEPLOYMENT_GRACE_SECONDS;
		return request;
	}

	string BuildSpawnResultId(HST_ForceQuoteState quote)
	{
		if (!quote)
			return "";
		return "spawn_garrison_" + quote.m_sQuoteId;
	}

	string BuildForceId(HST_ForceQuoteState quote)
	{
		if (!quote)
			return "";
		return "force_" + quote.m_sOperationId;
	}

	string BuildProjectionId(HST_ForceQuoteState quote)
	{
		if (!quote)
			return "";
		return "projection_" + quote.m_sOperationId;
	}

	string BuildRouteId(HST_ForceQuoteState quote)
	{
		if (!quote)
			return "";
		return "route_garrison_" + quote.m_sQuoteId;
	}

	protected int CountQuoteIdentity(HST_CampaignState state, HST_ForceQuoteState expected)
	{
		int count;
		foreach (HST_ForceQuoteState quote : state.m_aForceQuotes)
		{
			if (quote && (quote.m_sQuoteId == expected.m_sQuoteId
				|| quote.m_sOperationId == expected.m_sOperationId
				|| quote.m_sManifestId == expected.m_sManifestId))
				count++;
		}
		return count;
	}

	protected int CountManifestIdentity(HST_CampaignState state, HST_ForceManifestState expected)
	{
		int count;
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
		{
			if (manifest && (manifest.m_sManifestId == expected.m_sManifestId
				|| manifest.m_sOperationId == expected.m_sOperationId
				|| (!expected.m_sQuoteId.IsEmpty()
					&& manifest.m_sQuoteId == expected.m_sQuoteId)))
				count++;
		}
		return count;
	}

	protected int CountOperationIdentity(HST_CampaignState state, HST_OperationRecordState expected)
	{
		int count;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (operation && (operation.m_sOperationId == expected.m_sOperationId
				|| (!expected.m_sQuoteId.IsEmpty() && operation.m_sQuoteId == expected.m_sQuoteId)
				|| (!expected.m_sManifestId.IsEmpty() && operation.m_sManifestId == expected.m_sManifestId)
				|| (!expected.m_sSpawnResultId.IsEmpty() && operation.m_sSpawnResultId == expected.m_sSpawnResultId)
				|| (!expected.m_sProjectionId.IsEmpty() && operation.m_sProjectionId == expected.m_sProjectionId)
				|| (!expected.m_sGroupId.IsEmpty() && operation.m_sGroupId == expected.m_sGroupId)))
				count++;
		}
		return count;
	}

	protected int CountBatchIdentity(HST_CampaignState state, HST_ForceSpawnResultState expected)
	{
		int count;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && (batch.m_sResultId == expected.m_sResultId
				|| (!expected.m_sRequestId.IsEmpty() && batch.m_sRequestId == expected.m_sRequestId)
				|| (!expected.m_sOperationId.IsEmpty() && batch.m_sOperationId == expected.m_sOperationId)
				|| (!expected.m_sManifestId.IsEmpty() && batch.m_sManifestId == expected.m_sManifestId)
				|| (!expected.m_sForceId.IsEmpty() && batch.m_sForceId == expected.m_sForceId)
				|| (!expected.m_sProjectionId.IsEmpty() && batch.m_sProjectionId == expected.m_sProjectionId)))
				count++;
		}
		return count;
	}

	protected int CountGroupIdentity(HST_CampaignState state, HST_ActiveGroupState expected)
	{
		int count;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && (group.m_sGroupId == expected.m_sGroupId
				|| (!expected.m_sOperationId.IsEmpty() && group.m_sOperationId == expected.m_sOperationId)
				|| (!expected.m_sManifestId.IsEmpty() && group.m_sManifestId == expected.m_sManifestId)
				|| (!expected.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == expected.m_sSpawnResultId)
				|| (!expected.m_sForceId.IsEmpty() && group.m_sForceId == expected.m_sForceId)
				|| (!expected.m_sProjectionId.IsEmpty() && group.m_sProjectionId == expected.m_sProjectionId)))
				count++;
		}
		return count;
	}

	protected int CountRouteIdentity(HST_CampaignState state, HST_GeneratedRouteState expected)
	{
		int count;
		if (!state || !expected)
			return count;
		foreach (HST_GeneratedRouteState route : state.m_aGeneratedRoutes)
		{
			if (route && (route.m_sRouteId == expected.m_sRouteId
				|| route.m_sSourceLayoutId == expected.m_sSourceLayoutId))
				count++;
		}
		return count;
	}

	protected bool IsOwnedRoute(HST_ForceQuoteState quote, HST_GeneratedRouteState route)
	{
		return quote && route && route.m_sRouteId == BuildRouteId(quote)
			&& route.m_sSourceZoneId == quote.m_sTargetZoneId
			&& route.m_sTargetZoneId == quote.m_sTargetZoneId
			&& route.m_sSourceCategory == EXACT_GROUP_MODE
			&& route.m_sSourceLayoutId == quote.m_sOperationId;
	}

	protected int PositiveModulo(int value, int divisor)
	{
		if (divisor <= 0)
			return 0;
		int result = value % divisor;
		if (result < 0)
			result += divisor;
		return result;
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

	protected int CountString(array<string> values, string expected)
	{
		int count;
		if (!values || expected.IsEmpty())
			return count;
		foreach (string value : values)
		{
			if (value == expected)
				count++;
		}
		return count;
	}
}
