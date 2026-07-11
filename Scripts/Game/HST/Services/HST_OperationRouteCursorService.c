class HST_OperationRouteCursorResult
{
	bool m_bAccepted;
	bool m_bStateChanged;
	bool m_bArrived;
	bool m_bLapCompleted;
	int m_iAdvancedSeconds;
	float m_fAdvancedMeters;
	string m_sFailureReason;
}

// Generic persisted generated-route cursor used by versioned operations that
// need more than one direct outbound/return leg. Physical adapters mirror the
// current leg; this service owns only strategic position and loop progress.
class HST_OperationRouteCursorService
{
	static const int GENERATED_LOOP_ROUTE_VERSION = 2;
	static const int PROJECTION_CONTRACT_VERSION = 1;
	static const int MAX_CATCHUP_SECONDS_PER_TICK = 30;
	static const float DEFAULT_INFANTRY_SPEED_METERS_PER_SECOND = 2.5;
	static const float ARRIVAL_EPSILON_METERS = 1.0;

	static ref array<vector> BuildOrderedRoutePositions(HST_GeneratedRouteState route)
	{
		ref array<vector> positions = {};
		if (!route || route.m_sRouteId.IsEmpty())
			return positions;

		if (route.m_aWaypoints && route.m_aWaypoints.Count() > 0)
		{
			int lastIndex = -1000000;
			while (true)
			{
				HST_RouteWaypointState selected;
				int selectedIndex = 1000000;
				int selectedIndexCount;
				foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
				{
					if (!waypoint || waypoint.m_sRouteId != route.m_sRouteId
						|| waypoint.m_iIndex < 0 || IsZeroVector(waypoint.m_vPosition))
					{
						positions.Clear();
						return positions;
					}
					if (waypoint.m_iIndex <= lastIndex || waypoint.m_iIndex > selectedIndex)
						continue;
					if (waypoint.m_iIndex < selectedIndex)
					{
						selected = waypoint;
						selectedIndex = waypoint.m_iIndex;
						selectedIndexCount = 1;
					}
					else
						selectedIndexCount++;
				}
				if (!selected)
					break;
				if (selectedIndexCount != 1 || selectedIndex != positions.Count())
				{
					positions.Clear();
					return positions;
				}
				int positionCountBefore = positions.Count();
				AppendPosition(positions, selected.m_vPosition);
				if (positions.Count() == positionCountBefore)
				{
					positions.Clear();
					return positions;
				}
				lastIndex = selectedIndex;
			}
			return positions;
		}

		AppendPosition(positions, route.m_vStartPosition);
		AppendPosition(positions, route.m_vMidPosition);
		AppendPosition(positions, route.m_vEndPosition);
		return positions;
	}

	static string BuildRouteContractHash(HST_GeneratedRouteState route, array<vector> positions)
	{
		if (!route || route.m_sRouteId.IsEmpty() || !positions || positions.Count() < 2)
			return "";
		string canonical = string.Format(
			"%1|%2|%3|%4|%5",
			route.m_sRouteId,
			route.m_sSourceZoneId,
			route.m_sTargetZoneId,
			route.m_sSourceCategory,
			positions.Count());
		foreach (vector position : positions)
			canonical = canonical + string.Format("|%1:%2:%3", position[0], position[1], position[2]);
		return string.Format("orc1_%1_%2", canonical.Hash(), (canonical + "|secondary").Hash());
	}

	bool FreezePatrolRoute(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_GeneratedRouteState route,
		HST_ActiveGroupState group)
	{
		if (!state || !operation || !route || !group || IsZeroVector(operation.m_vOriginPosition))
			return false;
		ref array<vector> positions = BuildOrderedRoutePositions(route);
		string routeHash = BuildRouteContractHash(route, positions);
		if (positions.Count() < 2 || routeHash.IsEmpty())
			return false;

		bool operationChanged;
		operationChanged = AssignString(operation.m_sCurrentRouteId, route.m_sRouteId) || operationChanged;
		operationChanged = AssignString(operation.m_sRouteContractHash, routeHash) || operationChanged;
		if (operation.m_iProjectionContractVersion != PROJECTION_CONTRACT_VERSION)
		{
			operation.m_iProjectionContractVersion = PROJECTION_CONTRACT_VERSION;
			operationChanged = true;
		}
		if (operation.m_iRouteVersion != GENERATED_LOOP_ROUTE_VERSION)
		{
			operation.m_iRouteVersion = GENERATED_LOOP_ROUTE_VERSION;
			operationChanged = true;
		}
		if (operation.m_iRouteWaypointIndex != 0 || operation.m_iRouteLapCount != 0
			|| operation.m_iRouteLegSequence != 0 || operation.m_iRouteLoopStartedAtSecond != 0
			|| operation.m_iRouteLoopCompletedAtSecond != 0)
		{
			operation.m_iRouteWaypointIndex = 0;
			operation.m_iRouteLapCount = 0;
			operation.m_iRouteLegSequence = 0;
			operation.m_iRouteLoopStartedAtSecond = 0;
			operation.m_iRouteLoopCompletedAtSecond = 0;
			operationChanged = true;
		}
		bool legChanged = SetLeg(
			state,
			operation,
			group,
			operation.m_vOriginPosition,
			positions[0],
			"frozen patrol outbound leg");
		if (operationChanged && !legChanged)
			operation.m_iRevision++;
		return IsPatrolRouteContractValid(operation, route);
	}

	bool IsPatrolRouteContractValid(HST_OperationRecordState operation, HST_GeneratedRouteState route)
	{
		if (!operation || !route || operation.m_sCurrentRouteId != route.m_sRouteId
			|| operation.m_iProjectionContractVersion != PROJECTION_CONTRACT_VERSION
			|| operation.m_iRouteVersion != GENERATED_LOOP_ROUTE_VERSION
			|| operation.m_fStrategicSpeedMetersPerSecond <= 0
			|| IsZeroVector(operation.m_vRouteStartPosition)
			|| IsZeroVector(operation.m_vRouteEndPosition))
			return false;
		ref array<vector> positions = BuildOrderedRoutePositions(route);
		if (positions.Count() < 2 || operation.m_sRouteContractHash != BuildRouteContractHash(route, positions))
			return false;
		return operation.m_iRouteWaypointIndex >= -1
			&& operation.m_iRouteWaypointIndex < positions.Count()
			&& operation.m_iRouteLapCount >= 0
			&& operation.m_iRouteLegSequence >= 0;
	}

	bool StartPatrolLoop(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_GeneratedRouteState route,
		HST_ActiveGroupState group)
	{
		if (!state || !operation || !group || !IsPatrolRouteContractValid(operation, route))
			return false;
		ref array<vector> positions = BuildOrderedRoutePositions(route);
		if (operation.m_iRouteWaypointIndex != 0 || positions.Count() < 2)
			return false;
		operation.m_iRouteLoopStartedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iRouteWaypointIndex = 1;
		bool changed = SetLeg(
			state,
			operation,
			group,
			ResolveAuthoritativePosition(operation, group),
			positions[1],
			"patrol loop started");
		if (!changed)
			operation.m_iRevision++;
		return true;
	}

	HST_OperationRouteCursorResult AdvanceLoopAfterArrival(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_GeneratedRouteState route,
		HST_ActiveGroupState group,
		int requiredLaps)
	{
		HST_OperationRouteCursorResult result = new HST_OperationRouteCursorResult();
		if (!state || !operation || !group || !IsPatrolRouteContractValid(operation, route))
		{
			result.m_sFailureReason = "patrol loop route contract is invalid";
			return result;
		}
		ref array<vector> positions = BuildOrderedRoutePositions(route);
		int currentIndex = operation.m_iRouteWaypointIndex;
		if (currentIndex < 0 || currentIndex >= positions.Count())
		{
			result.m_sFailureReason = "patrol loop waypoint cursor is outside the frozen route";
			return result;
		}

		result.m_bAccepted = true;
		int nextIndex = currentIndex + 1;
		if (nextIndex >= positions.Count())
		{
			nextIndex = 0;
			operation.m_iRouteLapCount++;
			result.m_bLapCompleted = true;
			result.m_bStateChanged = true;
		}
		if (result.m_bLapCompleted && operation.m_iRouteLapCount >= Math.Max(1, requiredLaps))
		{
			operation.m_iRouteLoopCompletedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
			operation.m_iLastProgressAtSecond = Math.Max(0, state.m_iElapsedSeconds);
			operation.m_iRevision++;
			return result;
		}

		operation.m_iRouteWaypointIndex = nextIndex;
		result.m_bStateChanged = SetLeg(
			state,
			operation,
			group,
			ResolveAuthoritativePosition(operation, group),
			positions[nextIndex],
			"patrol loop advanced") || result.m_bStateChanged;
		return result;
	}

	bool BeginReturnLeg(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group)
	{
		if (!state || !operation || !group || IsZeroVector(operation.m_vOriginPosition))
			return false;
		operation.m_iRouteWaypointIndex = -1;
		return SetLeg(
			state,
			operation,
			group,
			ResolveAuthoritativePosition(operation, group),
			operation.m_vOriginPosition,
			"patrol return leg");
	}

	HST_OperationRouteCursorResult AdvanceVirtualLeg(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group)
	{
		HST_OperationRouteCursorResult result = new HST_OperationRouteCursorResult();
		string failure = ValidateVirtualAuthority(state, operation, group);
		if (!failure.IsEmpty())
		{
			result.m_sFailureReason = failure;
			return result;
		}
		result.m_bAccepted = true;
		float remaining = Math.Max(0.0, operation.m_fRouteTotalDistanceMeters - operation.m_fRouteProgressMeters);
		if (remaining <= ARRIVAL_EPSILON_METERS)
		{
			SnapToLegEnd(operation, group);
			result.m_bArrived = true;
			return result;
		}

		int elapsedSeconds = Math.Max(0, state.m_iElapsedSeconds - operation.m_iStrategicLastUpdateSecond);
		if (elapsedSeconds <= 0)
			return result;
		int advancedSeconds = Math.Min(MAX_CATCHUP_SECONDS_PER_TICK, elapsedSeconds);
		float advancedMeters = Math.Min(remaining, operation.m_fStrategicSpeedMetersPerSecond * advancedSeconds);
		operation.m_iStrategicLastUpdateSecond += advancedSeconds;
		operation.m_fRouteProgressMeters += advancedMeters;
		operation.m_vStrategicPosition = ResolvePosition(operation);
		group.m_vPosition = operation.m_vStrategicPosition;
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		operation.m_iLastProgressAtSecond = operation.m_iStrategicLastUpdateSecond;
		operation.m_iRevision++;
		group.m_iLifecycleRevision++;
		result.m_bStateChanged = true;
		result.m_iAdvancedSeconds = advancedSeconds;
		result.m_fAdvancedMeters = advancedMeters;
		result.m_bArrived = operation.m_fRouteTotalDistanceMeters - operation.m_fRouteProgressMeters <= ARRIVAL_EPSILON_METERS;
		if (result.m_bArrived)
			SnapToLegEnd(operation, group);
		return result;
	}

	bool SyncLegFromPosition(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		vector position)
	{
		if (!state)
			return false;
		return SyncLegFromPositionAtSecond(state.m_iElapsedSeconds, operation, group, position);
	}

	bool SyncLegFromPositionAtSecond(
		int nowSecond,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		vector position)
	{
		if (!operation || !group || IsZeroVector(position) || IsZeroVector(operation.m_vRouteEndPosition))
			return false;
		return SetLegAtSecond(
			nowSecond,
			operation,
			group,
			position,
			operation.m_vRouteEndPosition,
			"patrol physical position folded into strategic cursor");
	}

	static vector ResolvePosition(HST_OperationRecordState operation)
	{
		if (!operation)
			return "0 0 0";
		float total = Math.Max(0.0, operation.m_fRouteTotalDistanceMeters);
		if (total <= ARRIVAL_EPSILON_METERS)
			return operation.m_vRouteEndPosition;
		float fraction = Math.Max(0.0, Math.Min(1.0, operation.m_fRouteProgressMeters / total));
		return operation.m_vRouteStartPosition + (operation.m_vRouteEndPosition - operation.m_vRouteStartPosition) * fraction;
	}

	protected bool SetLeg(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		vector startPosition,
		vector endPosition,
		string reason)
	{
		if (!state || !operation || !group || IsZeroVector(startPosition) || IsZeroVector(endPosition))
			return false;
		return SetLegAtSecond(state.m_iElapsedSeconds, operation, group, startPosition, endPosition, reason);
	}

	protected bool SetLegAtSecond(
		int nowSecond,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		vector startPosition,
		vector endPosition,
		string reason)
	{
		if (!operation || !group || IsZeroVector(startPosition) || IsZeroVector(endPosition))
			return false;
		operation.m_vRouteStartPosition = startPosition;
		operation.m_vRouteEndPosition = endPosition;
		operation.m_fRouteTotalDistanceMeters = Distance2D(startPosition, endPosition);
		operation.m_fRouteProgressMeters = 0;
		operation.m_fStrategicSpeedMetersPerSecond = DEFAULT_INFANTRY_SPEED_METERS_PER_SECOND;
		operation.m_vStrategicPosition = startPosition;
		operation.m_iStrategicLastUpdateSecond = Math.Max(0, nowSecond);
		operation.m_iLastProgressAtSecond = Math.Max(0, nowSecond);
		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		operation.m_iRouteLegSequence++;
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		group.m_sRouteId = operation.m_sCurrentRouteId;
		group.m_vPosition = startPosition;
		group.m_vSourcePosition = startPosition;
		group.m_vTargetPosition = endPosition;
		group.m_iLifecycleRevision++;
		return true;
	}

	protected string ValidateVirtualAuthority(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group)
	{
		if (!state || !operation || !group)
			return "patrol strategic route authority is incomplete";
		if (operation.m_iProjectionContractVersion != PROJECTION_CONTRACT_VERSION
			|| operation.m_iRouteVersion != GENERATED_LOOP_ROUTE_VERSION
			|| operation.m_fStrategicSpeedMetersPerSecond <= 0)
			return "patrol strategic route contract is not initialized";
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
			return "settled patrol operation cannot advance strategically";
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return "patrol strategic movement does not own position authority";
		if (group.m_sOperationId != operation.m_sOperationId || group.m_sGroupId != operation.m_sGroupId)
			return "patrol strategic projection identity conflicts";
		return "";
	}

	protected static vector ResolveAuthoritativePosition(
		HST_OperationRecordState operation,
		HST_ActiveGroupState group)
	{
		if (operation && operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& group && !IsZeroVector(group.m_vPosition))
			return group.m_vPosition;
		if (operation && !IsZeroVector(operation.m_vStrategicPosition))
			return operation.m_vStrategicPosition;
		if (group && !IsZeroVector(group.m_vPosition))
			return group.m_vPosition;
		return "0 0 0";
	}

	protected static void SnapToLegEnd(HST_OperationRecordState operation, HST_ActiveGroupState group)
	{
		if (!operation || !group)
			return;
		operation.m_fRouteProgressMeters = operation.m_fRouteTotalDistanceMeters;
		operation.m_vStrategicPosition = operation.m_vRouteEndPosition;
		group.m_vPosition = operation.m_vStrategicPosition;
		group.m_vSourcePosition = operation.m_vStrategicPosition;
	}

	protected static void AppendPosition(array<vector> positions, vector position)
	{
		if (!positions || IsZeroVector(position))
			return;
		foreach (vector existing : positions)
		{
			if (Distance2D(existing, position) <= ARRIVAL_EPSILON_METERS)
				return;
		}
		positions.Insert(position);
	}

	protected static bool AssignString(out string target, string value)
	{
		if (target == value)
			return false;
		target = value;
		return true;
	}

	protected static float Distance2D(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}

	protected static bool IsZeroVector(vector value)
	{
		return Math.AbsFloat(value[0]) < 0.01
			&& Math.AbsFloat(value[1]) < 0.01
			&& Math.AbsFloat(value[2]) < 0.01;
	}
}
