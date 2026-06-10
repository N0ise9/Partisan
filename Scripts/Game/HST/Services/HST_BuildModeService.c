class HST_BuildModePlacement
{
	bool m_bValid;
	string m_sKind;
	string m_sPrefab;
	string m_sStatus;
	vector m_vPosition;
	vector m_vAngles;
	int m_iCost;
}

class HST_BuildModeService
{
	static const int GARAGE_REDEPLOY_DISTANCE_METERS = 6;
	static const int HQ_REBUILD_DISTANCE_METERS = 2;

	HST_BuildModePlacement ResolveGarageRedeployPlacement(HST_CampaignState state, int playerId, HST_GarageVehicleState vehicle)
	{
		HST_BuildModePlacement placement = CreatePlacement("garage_vehicle", vehicle);
		if (!state || !vehicle)
			return RejectPlacement(state, placement, "garage vehicle selection not ready");

		if (!HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(vehicle.m_sPrefab))
			return RejectPlacement(state, placement, "stored garage prefab is not a vehicle root");

		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		if (!playerEntity)
			return RejectPlacement(state, placement, "no controlled player entity");

		vector preferred = playerEntity.GetOrigin() + BuildPlayerForwardOffset(playerEntity, GARAGE_REDEPLOY_DISTANCE_METERS);
		vector resolved;
		if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(preferred, resolved, true))
			return RejectPlacement(state, placement, "no dry ground for garage redeploy placement");

		placement.m_bValid = true;
		placement.m_vPosition = resolved;
		placement.m_vAngles = ResolvePlacementAngles(playerEntity, vehicle.m_sVehicleId + vehicle.m_sPrefab, resolved, vehicle.m_vAngles);
		placement.m_sStatus = string.Format("garage redeploy placement ready | %1 | %2", vehicle.m_sDisplayName, resolved);
		PublishPlacement(state, placement, "");
		return placement;
	}

	HST_BuildModePlacement ResolveHQRebuildPlacement(HST_CampaignState state, int playerId)
	{
		HST_BuildModePlacement placement = new HST_BuildModePlacement();
		placement.m_sKind = "hq_assets";
		placement.m_sPrefab = "h-istasi HQ runtime assets";

		if (!state || !state.m_bHQDeployed)
			return RejectPlacement(state, placement, "HQ is not deployed");

		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		vector preferred = state.m_vHQPosition;
		if (playerEntity)
			preferred = playerEntity.GetOrigin() + BuildPlayerForwardOffset(playerEntity, HQ_REBUILD_DISTANCE_METERS);

		vector resolved;
		if (!HST_WorldPositionService.TryResolveGroundPosition(preferred, HST_WorldPositionService.PROP_GROUND_OFFSET, resolved, true))
			return RejectPlacement(state, placement, "no dry ground for HQ rebuild placement");

		placement.m_bValid = true;
		placement.m_vPosition = resolved;
		placement.m_vAngles = ResolvePlacementAngles(playerEntity, "hq_assets", resolved, "0 0 0");
		placement.m_sStatus = string.Format("HQ rebuild placement ready | %1", resolved);
		PublishPlacement(state, placement, "");
		return placement;
	}

	protected HST_BuildModePlacement CreatePlacement(string kind, HST_GarageVehicleState vehicle)
	{
		HST_BuildModePlacement placement = new HST_BuildModePlacement();
		placement.m_sKind = kind;
		if (vehicle)
		{
			placement.m_sPrefab = vehicle.m_sPrefab;
			placement.m_iCost = vehicle.m_iRedeployCost;
		}

		return placement;
	}

	protected HST_BuildModePlacement RejectPlacement(HST_CampaignState state, HST_BuildModePlacement existingPlacement, string reason)
	{
		HST_BuildModePlacement placement = existingPlacement;
		if (!placement)
			placement = new HST_BuildModePlacement();

		placement.m_bValid = false;
		placement.m_sStatus = reason;
		PublishPlacement(state, placement, reason);
		return placement;
	}

	protected void PublishPlacement(HST_CampaignState state, HST_BuildModePlacement placement, string failure)
	{
		if (!state || !placement)
			return;

		state.m_sBuildModeStatus = placement.m_sStatus;
		state.m_sLastBuildModeFailure = failure;
		state.m_sLastBuildModePrefab = placement.m_sPrefab;
		state.m_vLastBuildModePosition = placement.m_vPosition;
		state.m_fLastBuildModeYaw = placement.m_vAngles[0];

		if (failure.IsEmpty())
			Print(string.Format("h-istasi build mode | %1 | prefab %2 | position %3 | yaw %4 | cost %5", placement.m_sKind, placement.m_sPrefab, placement.m_vPosition, placement.m_vAngles[0], placement.m_iCost));
		else
			Print(string.Format("h-istasi build mode | %1 failed | prefab %2 | reason %3", placement.m_sKind, placement.m_sPrefab, failure), LogLevel.WARNING);
	}

	protected vector BuildPlayerForwardOffset(IEntity playerEntity, int distanceMeters)
	{
		vector offset = "4 0 4";
		if (!playerEntity)
			return offset;

		vector angles = playerEntity.GetYawPitchRoll();
		float yaw = angles[0] * 0.017453292;
		offset[0] = Math.Sin(yaw) * distanceMeters;
		offset[1] = 0;
		offset[2] = Math.Cos(yaw) * distanceMeters;
		return offset;
	}

	protected vector ResolvePlacementAngles(IEntity playerEntity, string salt, vector position, vector storedAngles)
	{
		if (storedAngles[0] != 0 || storedAngles[1] != 0 || storedAngles[2] != 0)
			return HST_WorldPositionService.BuildUprightAnglesFromVector(storedAngles);

		vector angles;
		if (playerEntity)
		{
			vector playerAngles = playerEntity.GetYawPitchRoll();
			angles[0] = playerAngles[0];
			return HST_WorldPositionService.BuildUprightAnglesFromVector(angles);
		}

		int seed = salt.Length() * 37 + Math.Round(position[0]) * 13 + Math.Round(position[2]) * 19;
		if (seed < 0)
			seed = -seed;

		angles[0] = seed - (seed / 360) * 360;
		return HST_WorldPositionService.BuildUprightAnglesFromVector(angles);
	}

	protected IEntity ResolveControlledPlayerEntity(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || playerId <= 0)
			return null;

		IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
		if (controlledEntity)
			return controlledEntity;

		return SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
	}
}
