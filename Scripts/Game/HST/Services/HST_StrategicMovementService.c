class HST_StrategicMovementResult
{
	bool m_bAccepted;
	bool m_bStateChanged;
	bool m_bArrived;
	int m_iAdvancedSeconds;
	float m_fAdvancedMeters;
	string m_sFailureReason;
}

class HST_StrategicMovementService
{
	static const int EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION = 1;
	static const int DIRECT_ROUTE_VERSION = 1;
	static const float EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND = 2.5;
	static const int MAX_CATCHUP_SECONDS_PER_TICK = 30;
	static const float ARRIVAL_EPSILON_METERS = 1.0;

	static int ResolveExactPlayerQRFETASeconds(vector sourcePosition, vector targetPosition)
	{
		float distance = Distance2D(sourcePosition, targetPosition);
		if (distance <= ARRIVAL_EPSILON_METERS)
			return 1;
		return Math.Max(1, Math.Ceil(distance / EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND));
	}

	bool InitializeExactPlayerQRFRoute(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_SupportRequestState request,
		HST_ForceManifestState manifest,
		HST_ActiveGroupState group)
	{
		if (!request)
			return false;
		return InitializeExactInfantryQRFRoute(state, operation, manifest, group);
	}

	bool InitializeExactInfantryQRFRoute(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ActiveGroupState group)
	{
		if (!state || !operation || !manifest || !group)
			return false;
		if (!IsSupportedExactInfantryManifest(manifest))
			return false;
		if (operation.m_iProjectionContractVersion > EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION)
			return false;

		vector sourcePosition = operation.m_vStrategicPosition;
		if (IsZeroVector(sourcePosition))
			sourcePosition = operation.m_vOriginPosition;
		vector targetPosition = operation.m_vTacticalTargetPosition;
		if (IsZeroVector(targetPosition))
			targetPosition = operation.m_vAssignmentPosition;
		if (IsZeroVector(sourcePosition) || IsZeroVector(targetPosition))
			return false;

		bool routeInitialized = operation.m_iProjectionContractVersion == EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION
			&& operation.m_iRouteVersion == DIRECT_ROUTE_VERSION
			&& !IsZeroVector(operation.m_vRouteStartPosition)
			&& !IsZeroVector(operation.m_vRouteEndPosition);
		bool operationChanged;
		bool groupChanged;
		if (operation.m_iProjectionContractVersion != EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION)
		{
			operation.m_iProjectionContractVersion = EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION;
			operationChanged = true;
		}
		if (operation.m_iRouteVersion != DIRECT_ROUTE_VERSION)
		{
			operation.m_iRouteVersion = DIRECT_ROUTE_VERSION;
			operationChanged = true;
		}
		if (!routeInitialized)
		{
			operationChanged = AssignVector(operation.m_vRouteStartPosition, sourcePosition) || operationChanged;
			operationChanged = AssignVector(operation.m_vRouteEndPosition, targetPosition) || operationChanged;
			float totalDistance = Distance2D(sourcePosition, targetPosition);
			if (Math.AbsFloat(operation.m_fRouteTotalDistanceMeters - totalDistance) > 0.01)
			{
				operation.m_fRouteTotalDistanceMeters = totalDistance;
				operationChanged = true;
			}
		}
		else if (operation.m_vRouteEndPosition != targetPosition)
		{
			operation.m_vRouteEndPosition = targetPosition;
			SyncRouteProgressFromPosition(operation, sourcePosition);
			operationChanged = true;
		}
		float boundedTotalDistance = Math.Max(0.0, operation.m_fRouteTotalDistanceMeters);
		if (operation.m_fRouteProgressMeters < 0 || operation.m_fRouteProgressMeters > boundedTotalDistance)
		{
			operation.m_fRouteProgressMeters = Math.Max(0.0, Math.Min(boundedTotalDistance, operation.m_fRouteProgressMeters));
			operationChanged = true;
		}
		if (operation.m_fStrategicSpeedMetersPerSecond != EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND)
		{
			operation.m_fStrategicSpeedMetersPerSecond = EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND;
			operationChanged = true;
		}
		if (operation.m_iStrategicLastUpdateSecond <= 0)
		{
			operation.m_iStrategicLastUpdateSecond = Math.Max(0, state.m_iElapsedSeconds);
			operationChanged = true;
		}
		if (operation.m_iVirtualCombatLastStepSecond <= 0)
		{
			operation.m_iVirtualCombatLastStepSecond = Math.Max(0, state.m_iElapsedSeconds);
			operationChanged = true;
		}
		operationChanged = AssignVector(operation.m_vStrategicPosition, ResolvePosition(operation)) || operationChanged;
		groupChanged = AssignVector(group.m_vPosition, operation.m_vStrategicPosition) || groupChanged;
		groupChanged = AssignVector(group.m_vSourcePosition, operation.m_vStrategicPosition) || groupChanged;
		groupChanged = AssignVector(group.m_vTargetPosition, targetPosition) || groupChanged;
		if (operationChanged)
		{
			operation.m_iLastProgressAtSecond = Math.Max(0, state.m_iElapsedSeconds);
			operation.m_iRevision++;
		}
		if (groupChanged)
			group.m_iLifecycleRevision++;
		return true;
	}

	HST_StrategicMovementResult AdvanceExactPlayerQRF(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group)
	{
		HST_StrategicMovementResult result = new HST_StrategicMovementResult();
		string failure = ValidateVirtualMovement(state, operation, group);
		if (!failure.IsEmpty())
		{
			result.m_sFailureReason = failure;
			return result;
		}
		result.m_bAccepted = true;
		if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ASSIGNMENT
			&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN
			&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_EXITING)
			return result;

		int elapsedSeconds = Math.Max(0, state.m_iElapsedSeconds - operation.m_iStrategicLastUpdateSecond);
		if (elapsedSeconds <= 0)
			return result;
		int advancedSeconds = Math.Min(MAX_CATCHUP_SECONDS_PER_TICK, elapsedSeconds);
		float remaining = Math.Max(0.0, operation.m_fRouteTotalDistanceMeters - operation.m_fRouteProgressMeters);
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
		{
			operation.m_fRouteProgressMeters = operation.m_fRouteTotalDistanceMeters;
			operation.m_vStrategicPosition = operation.m_vRouteEndPosition;
			group.m_vPosition = operation.m_vStrategicPosition;
			group.m_vSourcePosition = operation.m_vStrategicPosition;
		}
		return result;
	}

	vector ResolvePosition(HST_OperationRecordState operation)
	{
		if (!operation)
			return "0 0 0";
		float total = Math.Max(0.0, operation.m_fRouteTotalDistanceMeters);
		if (total <= ARRIVAL_EPSILON_METERS)
			return operation.m_vRouteEndPosition;
		float fraction = Math.Max(0.0, Math.Min(1.0, operation.m_fRouteProgressMeters / total));
		return operation.m_vRouteStartPosition + (operation.m_vRouteEndPosition - operation.m_vRouteStartPosition) * fraction;
	}

	bool SyncRouteProgressFromPosition(HST_OperationRecordState operation, vector position)
	{
		if (!operation || operation.m_iProjectionContractVersion != EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION)
			return false;
		bool changed = AssignVector(operation.m_vRouteStartPosition, position);
		float remainingDistance = Distance2D(position, operation.m_vRouteEndPosition);
		if (Math.AbsFloat(operation.m_fRouteTotalDistanceMeters - remainingDistance) > 0.01)
		{
			operation.m_fRouteTotalDistanceMeters = remainingDistance;
			changed = true;
		}
		if (operation.m_fRouteProgressMeters != 0)
		{
			operation.m_fRouteProgressMeters = 0;
			changed = true;
		}
		changed = AssignVector(operation.m_vStrategicPosition, position) || changed;
		return changed;
	}

	bool IsSupportedExactInfantryManifest(HST_ForceManifestState manifest)
	{
		return manifest && manifest.m_aGroups.Count() == 1 && manifest.m_aGroups[0]
			&& manifest.m_aMembers.Count() == manifest.m_iAcceptedMemberCount
			&& manifest.m_iAcceptedMemberCount > 0 && manifest.m_aVehicles.Count() == 0
			&& manifest.m_aAssets.Count() == 0 && manifest.m_iAcceptedVehicleCount == 0;
	}

	protected string ValidateVirtualMovement(HST_CampaignState state, HST_OperationRecordState operation, HST_ActiveGroupState group)
	{
		if (!state || !operation || !group)
			return "strategic movement authority is incomplete";
		if (operation.m_iProjectionContractVersion != EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION
			|| operation.m_iRouteVersion != DIRECT_ROUTE_VERSION || operation.m_fStrategicSpeedMetersPerSecond <= 0)
			return "strategic movement route contract is not initialized";
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
			return "settled operation cannot advance strategically";
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return "strategic movement does not own position authority";
		if (group.m_sOperationId != operation.m_sOperationId || group.m_sGroupId != operation.m_sGroupId)
			return "strategic movement projection identity conflicts";
		return "";
	}

	protected static float Distance2D(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}

	protected bool AssignVector(out vector target, vector value)
	{
		if (target == value)
			return false;
		target = value;
		return true;
	}

	protected bool IsZeroVector(vector value)
	{
		return Math.AbsFloat(value[0]) < 0.01 && Math.AbsFloat(value[1]) < 0.01 && Math.AbsFloat(value[2]) < 0.01;
	}
}
