class HST_ConvoyReadinessStatus
{
	int m_iVehicleAssetCount;
	int m_iSpawnedVehicleCount;
	int m_iCrewGroupCount;
	int m_iAliveCrewGroupCount;
	int m_iAliveCrewCount;
	int m_iDriverAvailableCount;
	int m_iMobileVehicleCount;
	int m_iRouteAssignedCount;
	int m_iWaypointAssignedCount;
	bool m_bReadyToMove;
	bool m_bStaticFallbackAvailable;
	bool m_bPendingGrace;
	string m_sReason;
}

class HST_PhysicalWarService
{
	static const int MAX_ACTIVE_INFANTRY_PER_ZONE = 6;
	static const int MAX_ACTIVE_VEHICLES_PER_ZONE = 1;
	static const int QRF_ATTACK_RESOURCE_COST = 15;
	static const int QRF_SUPPORT_RESOURCE_COST = 5;
	static const int QRF_ETA_SECONDS = 180;
	static const int QRF_INBOUND_SPAWN_SECONDS = 30;
	static const int QRF_COOLDOWN_SECONDS = 900;
	static const int ROUTE_STATE_UPDATE_SECONDS = 5;
	static const float HQ_SAFE_RADIUS_METERS = 900;
	static const float QRF_MIN_STANDOFF_METERS = 220.0;
	static const float QRF_EXTRA_STANDOFF_METERS = 140.0;
	static const float QRF_MAX_STANDOFF_METERS = 650.0;
	static const string MISSION_CONVOY_GROUP_PREFIX = "mission_convoy_";
	static const string MISSION_CONVOY_VEHICLE_ROLE = "convoy_vehicle";
	static const string MISSION_CONVOY_PRIMITIVE = "convoy_intercept";
	static const string MISSION_CONVOY_STAGING = "convoy_staging";
	static const string MISSION_CONVOY_MOVING = "convoy_moving";
	static const string MISSION_CONVOY_CONTACT = "convoy_contact";
	static const string MISSION_CONVOY_ELIMINATED = "convoy_eliminated";
	static const string MISSION_CONVOY_ARRIVED = "convoy_arrived";
	static const string MISSION_CONVOY_FAILED = "failed";
	static const string CONVOY_MOVE_EVENT_PENDING = "convoy_moving_pending";
	static const string CONVOY_MOVE_EVENT_SENT = "convoy_moving_sent";
	static const string FIA_CAMPAIGN_FACTION_CONFIG = "Configs/Factions/FIA_Campaign.conf";
	static const string US_CAMPAIGN_FACTION_CONFIG = "{ADFDBDA163950168}Configs/Factions/US_Campaign.conf";
	static const string USSR_CAMPAIGN_FACTION_CONFIG = "{15B582F8FA0B0940}Configs/Factions/USSR_Campaign.conf";
	static const float CONVOY_DESTINATION_RADIUS_METERS = 120.0;
	static const int CONVOY_MARKER_REFRESH_SECONDS = 10;
	static const int CONVOY_CREW_POPULATION_GRACE_SECONDS = 20;
	static const int CONVOY_READINESS_GRACE_SECONDS = 60;
	static const float CONVOY_VEHICLE_SPAWN_LIFT_METERS = 5.0;
	static const float CONVOY_VEHICLE_SPAWN_CLEARANCE_METERS = 36.0;

	protected ref array<string> m_aRuntimeGroupIds = {};
	protected ref array<IEntity> m_aRuntimeGroupEntities = {};
	protected ref array<string> m_aRuntimeVehicleGroupIds = {};
	protected ref array<IEntity> m_aRuntimeVehicleEntities = {};
	protected ref HST_ConvoyVehicleControlAdapter m_ConvoyVehicleControl;
	protected bool m_bMarkerRefreshNeeded;

	bool UpdateZoneActivation(HST_CampaignState state, HST_BalanceConfig balance, HST_CampaignPreset preset = null, HST_EnemyDirectorService enemyDirector = null, HST_ZoneCompositionService compositions = null)
	{
		if (!state || !balance)
			return false;

		EnsureRuntimeGroupEntities(state);
		bool survivorChanged = UpdateRuntimeGroupSurvivors(state);

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return survivorChanged;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		bool changed;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			bool shouldBeActive = !IsZoneInsideHQSafeArea(state, zone) && IsAnyLivingPlayerNearZone(playerManager, playerIds, zone, balance);
			if (zone.m_bActive == shouldBeActive)
			{
				if (shouldBeActive && !HasActiveGarrisonGroup(state, zone))
					changed = ActivateZone(state, zone, compositions) || changed;

				continue;
			}

			zone.m_bActive = shouldBeActive;
			if (shouldBeActive)
				changed = ActivateZone(state, zone, compositions) || changed;
			else
				changed = DeactivateZone(state, zone, compositions) || changed;

			changed = true;
			m_bMarkerRefreshNeeded = true;
			Print(string.Format("h-istasi | zone %1 physical activation = %2", zone.m_sZoneId, shouldBeActive));
		}

		if (UpdateQRF(state, preset, enemyDirector))
			changed = true;

		return changed || survivorChanged;
	}

	bool ConsumeMarkerRefreshNeeded()
	{
		bool result = m_bMarkerRefreshNeeded;
		m_bMarkerRefreshNeeded = false;
		return result;
	}

	string BuildConvoyRuntimeReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi convoy runtime | state not ready";

		int convoyMissions;
		int convoyGroups;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (mission && mission.m_sRuntimePrimitive == MISSION_CONVOY_PRIMITIVE)
				convoyMissions++;
		}
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (IsMissionConvoyGroup(activeGroup))
				convoyGroups++;
		}

		string report = string.Format("h-istasi convoy runtime | missions %1 | groups %2 | crew entities %3 | vehicle entities %4", convoyMissions, convoyGroups, m_aRuntimeGroupIds.Count(), m_aRuntimeVehicleGroupIds.Count());
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
				continue;

			report = report + BuildConvoyRuntimeReport(state, mission);
		}

		return report;
	}

	string BuildGroundVehicleCandidateReport()
	{
		string report = "h-istasi convoy ground vehicle candidates";
		report = report + BuildGroundVehicleCandidateFactionReport("FIA");
		report = report + BuildGroundVehicleCandidateFactionReport("US");
		report = report + BuildGroundVehicleCandidateFactionReport("USSR");
		return report;
	}

	string BuildConvoyRuntimeReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state)
			return "\nconvoy mission | state not ready";
		if (!mission)
			return "\nconvoy mission | mission missing";
		if (mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
			return string.Format("\nconvoy mission | instance %1 | mission %2 | not convoy", ReportText(mission.m_sInstanceId), ReportText(mission.m_sMissionId));

		return BuildMissionConvoyRuntimeReport(state, mission);
	}

	bool UpdateMissionConvoys(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, int elapsedSeconds)
	{
		if (!state)
			return false;

		UpdateRuntimeGroupSurvivors(state);
		bool changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
				continue;

			changed = EnsureMissionConvoyRuntime(state, preset, mission) || changed;
			changed = EnsureMissionConvoyCrewSeating(state, mission) || changed;
			changed = UpdateMissionConvoyPhase(state, mission, elapsedSeconds) || changed;
			changed = UpdateMissionConvoyObjective(state, mission) || changed;
		}

		return changed;
	}

	protected bool EnsureMissionConvoyRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		bool changed;
		int vehicleAssets;
		int crewedVehicles;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE || asset.m_bDestroyed || asset.m_bDelivered)
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, vehicleAssets);
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			if (!activeGroup)
			{
				activeGroup = CreateMissionConvoyGroup(state, preset, mission, asset, vehicleAssets);
				if (activeGroup)
				{
					state.m_aActiveGroups.Insert(activeGroup);
					changed = true;
				}
			}

			if (activeGroup && !activeGroup.m_bSpawnedEntity && activeGroup.m_sRuntimeStatus != "spawn_failed")
				changed = TrySpawnMissionConvoyGroup(state, mission, asset, activeGroup, vehicleAssets) || changed;

			if (activeGroup && activeGroup.m_bSpawnedEntity && activeGroup.m_sRuntimeStatus != "spawn_failed" && Math.Max(activeGroup.m_iSpawnedAgentCount, CountAliveRuntimeCrewAgents(activeGroup)) > 0)
				crewedVehicles++;

			vehicleAssets++;
		}

		if (vehicleAssets > 0 && vehicleAssets < 3)
		{
			SetMissionConvoyFailure(state, mission, "Convoy needs at least three valid vehicle assets.");
			changed = true;
		}
		else if (vehicleAssets >= 3 && crewedVehicles < 3 && AllMissionConvoyGroupsAttempted(state, mission, vehicleAssets) && !HasPendingConvoyCrewPopulation(state, mission, vehicleAssets))
		{
			SetMissionConvoyFailure(state, mission, "Convoy could not spawn three crewed vehicles.");
			changed = true;
		}

		return changed;
	}

	protected bool EnsureMissionConvoyCrewSeating(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || !ShouldRetryMissionConvoyCrewSeating(mission))
			return false;

		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!IsMissionConvoyGroupForMission(activeGroup, mission) || !activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED)
				continue;

			IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
			IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
			if (!crewEntity || !vehicleEntity)
				continue;

			string previousFallbackMode = activeGroup.m_sSpawnFallbackMode;
			string previousReason = activeGroup.m_sSpawnFailureReason;
			string seatingReason;
			bool preserveWaypointMode = activeGroup.m_sSpawnFallbackMode == "convoy_waypoints";
			if (GetConvoyVehicleControlAdapter().TryBindCrewToVehicle(activeGroup, crewEntity, vehicleEntity, seatingReason))
			{
				if (!preserveWaypointMode)
					activeGroup.m_sSpawnFallbackMode = "convoy_driver_available";
				activeGroup.m_sSpawnFailureReason = seatingReason;
				RefreshMissionConvoyCrewCount(activeGroup);
			}
			else
			{
				if (!preserveWaypointMode)
				{
					if (seatingReason.Contains("seating pending yes") || seatingReason.Contains("waiting for animated AI boarding"))
						activeGroup.m_sSpawnFallbackMode = "convoy_seating_pending";
					else
						activeGroup.m_sSpawnFallbackMode = "convoy_vehicle_control_unavailable";
				}

				activeGroup.m_sSpawnFailureReason = seatingReason;
				if (activeGroup.m_sSpawnFailureReason.IsEmpty())
					activeGroup.m_sSpawnFailureReason = "Convoy crew seating has not confirmed a seated AI driver yet.";
			}

			if (activeGroup.m_sSpawnFallbackMode != previousFallbackMode || activeGroup.m_sSpawnFailureReason != previousReason)
				changed = true;
		}

		return changed;
	}

	protected bool ShouldRetryMissionConvoyCrewSeating(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;

		return mission.m_sRuntimePhase == MISSION_CONVOY_STAGING;
	}

	protected HST_ActiveGroupState CreateMissionConvoyGroup(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionAssetState asset, int index)
	{
		HST_ZoneState targetZone = ResolveMissionConvoyTargetZone(state, mission, asset);
		if (!targetZone)
			return null;

		string factionKey = ResolveMissionConvoyFactionKey(state, preset, mission, targetZone);
		HST_ActiveGroupState activeGroup = new HST_ActiveGroupState();
		activeGroup.m_sGroupId = BuildMissionConvoyGroupId(mission, index);
		activeGroup.m_sZoneId = targetZone.m_sZoneId;
		activeGroup.m_sFactionKey = factionKey;
		activeGroup.m_sPrefab = SelectConvoyCrewGroupPrefab(state, targetZone, factionKey, index);
		activeGroup.m_sRouteId = ResolveMissionConvoyRouteId(state, mission);
		activeGroup.m_vSourcePosition = asset.m_vSourcePosition;
		activeGroup.m_vTargetPosition = asset.m_vTargetPosition;
		activeGroup.m_vPosition = asset.m_vSourcePosition;
		activeGroup.m_sRuntimeStatus = ResolveMissionConvoyRuntimeStatus(mission);
		activeGroup.m_iInfantryCount = ResolveMissionConvoyCrewCount(state, mission, index);
		activeGroup.m_iVehicleCount = 1;
		activeGroup.m_iLastSeenAliveCount = activeGroup.m_iInfantryCount;
		activeGroup.m_iSurvivorInfantryCount = activeGroup.m_iInfantryCount;
		activeGroup.m_iSurvivorVehicleCount = 1;
		activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		return activeGroup;
	}

	protected bool TrySpawnMissionConvoyGroup(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ActiveGroupState activeGroup, int index)
	{
		if (!state || !mission || !asset || !activeGroup)
			return false;

		if (HasRuntimeGroupEntity(activeGroup.m_sGroupId))
			return false;

		vector spawnPosition;
		if (!TryResolveMissionConvoyVehicleSpawnPosition(mission, asset, spawnPosition) || HST_WorldPositionService.IsLikelyOpenWater(spawnPosition))
		{
			activeGroup.m_bSpawnAttempted = true;
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			activeGroup.m_sSpawnFailureReason = "No dry vehicle-safe convoy staging position.";
			mission.m_sRuntimeFailureReason = activeGroup.m_sSpawnFailureReason;
			Print(string.Format("h-istasi mission convoy | spawn failed %1 asset %2 at %3: %4", mission.m_sInstanceId, asset.m_sAssetId, asset.m_vSourcePosition, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
			return true;
		}

		activeGroup.m_vPosition = OffsetConvoyCrewSpawnPosition(spawnPosition, asset.m_vTargetPosition, index);
		activeGroup.m_vSourcePosition = activeGroup.m_vPosition;
		if (!TrySpawnActiveGroup(activeGroup, state))
		{
			mission.m_sRuntimeFailureReason = activeGroup.m_sSpawnFailureReason;
			return true;
		}

		string vehiclePrefab = asset.m_sPrefab;
		if (vehiclePrefab.IsEmpty())
			vehiclePrefab = SelectMissionConvoyVehiclePrefab(state, ResolveMissionConvoyTargetZone(state, mission, asset), activeGroup.m_sFactionKey, mission, index);

		GenericEntity vehicleEntity = SpawnMissionConvoyVehicle(state, mission, asset, activeGroup, vehiclePrefab, spawnPosition, index);
		if (!vehicleEntity)
		{
			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			activeGroup.m_bSpawnedEntity = false;
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			activeGroup.m_sSpawnFailureReason = "Convoy vehicle prefab failed to spawn.";
			mission.m_sRuntimeFailureReason = activeGroup.m_sSpawnFailureReason;
			Print(string.Format("h-istasi mission convoy | vehicle spawn failed %1 group %2 prefab %3", mission.m_sInstanceId, activeGroup.m_sGroupId, vehiclePrefab), LogLevel.WARNING);
			return true;
		}

		m_aRuntimeVehicleGroupIds.Insert(activeGroup.m_sGroupId);
		m_aRuntimeVehicleEntities.Insert(vehicleEntity);
		activeGroup.m_sRuntimeStatus = ResolveMissionConvoyRuntimeStatus(mission);
		string adapterBindReason;
		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		if (GetConvoyVehicleControlAdapter().TryBindCrewToVehicle(activeGroup, crewEntity, vehicleEntity, adapterBindReason))
		{
			activeGroup.m_sSpawnFallbackMode = "convoy_driver_available";
			activeGroup.m_sSpawnFailureReason = adapterBindReason;
			RefreshMissionConvoyCrewCount(activeGroup);
		}
		else
		{
			if (adapterBindReason.Contains("seating pending yes") || adapterBindReason.Contains("waiting for animated AI boarding"))
				activeGroup.m_sSpawnFallbackMode = "convoy_seating_pending";
			else
				activeGroup.m_sSpawnFallbackMode = "convoy_vehicle_control_unavailable";
			activeGroup.m_sSpawnFailureReason = adapterBindReason;
			if (activeGroup.m_sSpawnFailureReason.IsEmpty())
				activeGroup.m_sSpawnFailureReason = "Convoy crew seating has not confirmed a seated AI driver yet.";
		}
		activeGroup.m_iLastSeenAliveCount = CountAliveRuntimeCrewAgents(activeGroup);
		activeGroup.m_iSurvivorInfantryCount = Math.Min(activeGroup.m_iInfantryCount, activeGroup.m_iLastSeenAliveCount);
		activeGroup.m_iSurvivorVehicleCount = 1;
		UpdateMissionConvoyAssetPosition(state, asset, spawnPosition);
		Print(string.Format("h-istasi mission convoy | spawned vehicle %1 and crew group %2 for %3 at %4", vehiclePrefab, activeGroup.m_sGroupId, mission.m_sInstanceId, spawnPosition));
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected GenericEntity SpawnMissionConvoyVehicle(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ActiveGroupState activeGroup, string vehiclePrefab, vector spawnPosition, int index)
	{
		string selectedPrefab = SelectMissionConvoyVehiclePrefab(state, ResolveMissionConvoyTargetZone(state, mission, asset), activeGroup.m_sFactionKey, mission, index + 13);
		if (!selectedPrefab.IsEmpty())
			vehiclePrefab = selectedPrefab;
		else if (vehiclePrefab.IsEmpty() || !IsValidVehiclePrefabResource(vehiclePrefab, activeGroup.m_sFactionKey))
			vehiclePrefab = SelectMissionConvoyVehiclePrefab(state, ResolveMissionConvoyTargetZone(state, mission, asset), activeGroup.m_sFactionKey, mission, index + 29);
		if (vehiclePrefab.IsEmpty())
			return null;

		vector angles = BuildConvoyVehicleAngles(spawnPosition, asset.m_vTargetPosition);
		GenericEntity vehicleEntity = HST_WorldPositionService.SpawnPrefab(vehiclePrefab, spawnPosition, angles);
		if (!vehicleEntity)
			return null;

		HST_WorldPositionService.ApplyUprightEntityTransform(vehicleEntity, spawnPosition, angles);
		asset.m_sPrefab = vehiclePrefab;
		asset.m_bSpawned = true;
		asset.m_bAlive = true;
		asset.m_vCurrentPosition = spawnPosition;
		asset.m_vLastKnownPosition = spawnPosition;

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (!runtimeEntity)
		{
			runtimeEntity = new HST_MissionRuntimeEntityState();
			runtimeEntity.m_sRuntimeEntityId = asset.m_sEntityId;
			runtimeEntity.m_sMissionInstanceId = mission.m_sInstanceId;
			state.m_aMissionRuntimeEntities.Insert(runtimeEntity);
		}

		runtimeEntity.m_sKind = asset.m_sRole;
		runtimeEntity.m_sPrefab = vehiclePrefab;
		runtimeEntity.m_vPosition = spawnPosition;
		runtimeEntity.m_vAngles = angles;
		runtimeEntity.m_bSpawned = true;
		return vehicleEntity;
	}

	protected bool TryResolveMissionConvoyVehicleSpawnPosition(HST_ActiveMissionState mission, HST_MissionAssetState asset, out vector spawnPosition)
	{
		spawnPosition = "0 0 0";
		if (!mission || !asset)
			return false;

		if (TryResolveMissionConvoyVehicleSpawnPositionPass(mission, asset, spawnPosition, true))
			return true;

		return TryResolveMissionConvoyVehicleSpawnPositionPass(mission, asset, spawnPosition, false);
	}

	protected bool TryResolveMissionConvoyVehicleSpawnPositionPass(HST_ActiveMissionState mission, HST_MissionAssetState asset, out vector spawnPosition, bool preferHeavyVehicleTerrain)
	{
		spawnPosition = "0 0 0";
		if (!mission || !asset)
			return false;

		vector resolved;
		if (TryResolveMissionConvoyVehicleSpawnCandidate(asset.m_vSourcePosition, resolved, preferHeavyVehicleTerrain) && IsMissionConvoyVehicleSpawnClear(mission, resolved))
		{
			spawnPosition = LiftMissionConvoyVehicleSpawnPosition(resolved);
			return true;
		}

		for (int ring = 1; ring <= 5; ring++)
		{
			float radius = CONVOY_VEHICLE_SPAWN_CLEARANCE_METERS * ring;
			for (int step = 0; step < 8; step++)
			{
				vector candidate = BuildConvoySpawnClearanceCandidate(asset.m_vSourcePosition, step, radius);
				if (!TryResolveMissionConvoyVehicleSpawnCandidate(candidate, resolved, preferHeavyVehicleTerrain))
					continue;
				if (!IsMissionConvoyVehicleSpawnClear(mission, resolved))
					continue;

				spawnPosition = LiftMissionConvoyVehicleSpawnPosition(resolved);
				return true;
			}
		}

		return false;
	}

	protected bool TryResolveMissionConvoyVehicleSpawnCandidate(vector position, out vector resolved, bool preferHeavyVehicleTerrain)
	{
		if (preferHeavyVehicleTerrain)
			return HST_WorldPositionService.TryResolveHeavyVehicleSpawnPosition(position, resolved, true);

		return HST_WorldPositionService.TryResolveLargeVehicleSpawnPosition(position, resolved, true);
	}

	protected vector LiftMissionConvoyVehicleSpawnPosition(vector position)
	{
		vector lifted;
		if (HST_WorldPositionService.TryResolveGroundPosition(position, HST_WorldPositionService.VEHICLE_GROUND_OFFSET + CONVOY_VEHICLE_SPAWN_LIFT_METERS, lifted, true))
			return lifted;

		lifted = position;
		lifted[1] = lifted[1] + CONVOY_VEHICLE_SPAWN_LIFT_METERS;
		return lifted;
	}

	protected vector BuildConvoySpawnClearanceCandidate(vector origin, int step, float radius)
	{
		vector candidate = origin;
		if (step == 0)
			candidate[0] = candidate[0] + radius;
		else if (step == 1)
			candidate[0] = candidate[0] - radius;
		else if (step == 2)
			candidate[2] = candidate[2] + radius;
		else if (step == 3)
			candidate[2] = candidate[2] - radius;
		else if (step == 4)
		{
			candidate[0] = candidate[0] + radius;
			candidate[2] = candidate[2] + radius;
		}
		else if (step == 5)
		{
			candidate[0] = candidate[0] - radius;
			candidate[2] = candidate[2] + radius;
		}
		else if (step == 6)
		{
			candidate[0] = candidate[0] + radius;
			candidate[2] = candidate[2] - radius;
		}
		else
		{
			candidate[0] = candidate[0] - radius;
			candidate[2] = candidate[2] - radius;
		}

		return candidate;
	}

	protected bool IsMissionConvoyVehicleSpawnClear(HST_ActiveMissionState mission, vector position)
	{
		if (!mission)
			return false;
		if (HST_WorldPositionService.IsLikelyOpenWater(position))
			return false;

		float minDistanceSq = CONVOY_VEHICLE_SPAWN_CLEARANCE_METERS * CONVOY_VEHICLE_SPAWN_CLEARANCE_METERS;
		for (int i = 0; i < m_aRuntimeVehicleGroupIds.Count(); i++)
		{
			string groupId = m_aRuntimeVehicleGroupIds[i];
			if (!groupId.Contains(mission.m_sInstanceId))
				continue;

			IEntity vehicle = m_aRuntimeVehicleEntities[i];
			if (!vehicle)
				continue;
			if (DistanceSq2D(vehicle.GetOrigin(), position) < minDistanceSq)
				return false;
		}

		return true;
	}

	protected bool UpdateMissionConvoyPhase(HST_CampaignState state, HST_ActiveMissionState mission, int elapsedSeconds)
	{
		if (!state || !mission)
			return false;

		bool changed;
		if (mission.m_sRuntimePhase.IsEmpty() || mission.m_sRuntimePhase == "created" || mission.m_sRuntimePhase == "active" || mission.m_sRuntimePhase == "convoy_static")
		{
			mission.m_sRuntimePhase = MISSION_CONVOY_STAGING;
			changed = true;
		}

		if (mission.m_sRuntimePhase == MISSION_CONVOY_STAGING && mission.m_iRuntimeCounterB > 0 && mission.m_iRuntimeCounterA >= mission.m_iRuntimeCounterB)
		{
			changed = TryAdvanceMissionConvoyFromStaging(state, mission) || changed;
		}

		if (mission.m_sRuntimePhase == MISSION_CONVOY_MOVING && IsMissionConvoyMovementInterrupted(state, mission))
		{
			SyncMissionConvoyAssetPositions(state, mission);
			SetMissionConvoyStaticFallback(state, mission, "Convoy movement interrupted because every living crew member in a moving convoy group dismounted.");
			return true;
		}

		if (mission.m_sRuntimePhase == MISSION_CONVOY_MOVING && state.m_iElapsedSeconds % CONVOY_MARKER_REFRESH_SECONDS == 0)
		{
			SyncMissionConvoyAssetPositions(state, mission);
			m_bMarkerRefreshNeeded = true;
			changed = true;
		}

		return changed;
	}

	protected bool TryAdvanceMissionConvoyFromStaging(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_ConvoyReadinessStatus readiness = BuildMissionConvoyReadinessStatus(state, mission);
		if (CanAttemptMissionConvoyWaypointAssignment(readiness) && AssignMissionConvoyWaypoints(state, mission))
			readiness = BuildMissionConvoyReadinessStatus(state, mission);

		if (readiness.m_bReadyToMove)
		{
			SetMissionConvoyMoving(state, mission);
			return true;
		}

		if (readiness.m_bPendingGrace)
			return false;

		if (readiness.m_bStaticFallbackAvailable)
		{
			SetMissionConvoyStaticFallback(state, mission, readiness.m_sReason);
			return true;
		}

		SetMissionConvoyFailure(state, mission, readiness.m_sReason);
		return true;
	}

	protected bool IsMissionConvoyMovementInterrupted(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePhase != MISSION_CONVOY_MOVING)
			return false;

		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!IsMissionConvoyGroupForMission(activeGroup, mission) || !activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED)
				continue;
			if (!IsMissionConvoyWaypointAssigned(activeGroup))
				return true;
			if (IsMissionConvoyGroupFullyDismounted(activeGroup))
				return true;
		}

		return false;
	}

	protected bool IsMissionConvoyGroupFullyDismounted(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		string reason;
		bool dismounted = GetConvoyVehicleControlAdapter().AreAllLivingCrewDismounted(crewEntity, vehicleEntity, reason);
		if (dismounted && !reason.IsEmpty())
			activeGroup.m_sSpawnFailureReason = reason;

		return dismounted;
	}

	protected void SetMissionConvoyMoving(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;

		mission.m_sRuntimePhase = MISSION_CONVOY_MOVING;
		mission.m_sLastRuntimeEventKey = CONVOY_MOVE_EVENT_PENDING;
		ApplyMissionConvoyStatusToGroups(state, mission, MISSION_CONVOY_MOVING);
		m_bMarkerRefreshNeeded = true;
	}

	protected void SetMissionConvoyStaticFallback(HST_CampaignState state, HST_ActiveMissionState mission, string reason)
	{
		if (!state || !mission)
			return;

		if (reason.IsEmpty())
			reason = "Convoy readiness failed; convoy staged as a static ambush.";
		else if (!reason.Contains("static ambush"))
			reason = reason + " Convoy staged as a static ambush.";

		mission.m_sRuntimePhase = MISSION_CONVOY_CONTACT;
		mission.m_sRuntimeFailureReason = reason;
		ApplyMissionConvoyStatusToGroups(state, mission, MISSION_CONVOY_CONTACT);
		m_bMarkerRefreshNeeded = true;
		Print(string.Format("h-istasi mission convoy | %1 staged as static ambush: %2", mission.m_sInstanceId, mission.m_sRuntimeFailureReason), LogLevel.WARNING);
	}

	protected bool UpdateMissionConvoyObjective(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		int totalGroups;
		int eliminatedGroups;
		int livingCrew;
		vector aggregatePosition;
		int aggregateCount;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, totalGroups);
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			if (activeGroup)
			{
				int aliveCrew = CountAliveRuntimeCrewAgents(activeGroup);
				if (activeGroup.m_bSpawnedEntity && aliveCrew <= 0 && !IsConvoyCrewPopulationPending(state, activeGroup))
				{
					if (activeGroup.m_sRuntimeStatus != MISSION_CONVOY_ELIMINATED)
						activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
					eliminatedGroups++;
				}
				else
				{
					livingCrew += Math.Max(0, aliveCrew);
				}
			}

			vector position = ResolveMissionConvoyVehiclePosition(asset, groupId);
			aggregatePosition[0] = aggregatePosition[0] + position[0];
			aggregatePosition[1] = aggregatePosition[1] + position[1];
			aggregatePosition[2] = aggregatePosition[2] + position[2];
			aggregateCount++;
			totalGroups++;
		}

		if (aggregateCount > 0)
		{
			aggregatePosition[0] = aggregatePosition[0] / aggregateCount;
			aggregatePosition[1] = aggregatePosition[1] / aggregateCount;
			aggregatePosition[2] = aggregatePosition[2] / aggregateCount;
		}

		bool changed = ApplyMissionConvoyObjectiveProgress(state, mission, eliminatedGroups, totalGroups);
		if (totalGroups > 0 && eliminatedGroups >= totalGroups)
		{
			if (mission.m_sRuntimePhase != MISSION_CONVOY_ELIMINATED)
			{
				mission.m_sRuntimePhase = MISSION_CONVOY_ELIMINATED;
				changed = true;
			}
			return changed;
		}

		if (livingCrew > 0 && mission.m_sRuntimePhase == MISSION_CONVOY_MOVING && IsMissionConvoyAtDestination(mission, aggregatePosition))
		{
			SetMissionConvoyFailure(state, mission, "Convoy reached its destination with living crew.");
			return true;
		}

		return changed;
	}

	protected string BuildMissionConvoyRuntimeReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "";

		int vehicleAssetCount = CountMissionConvoyVehicleAssets(state, mission.m_sInstanceId);
		vector sourcePosition = ResolveMissionConvoySourcePosition(state, mission);
		vector targetPosition = ResolveMissionConvoyTargetPosition(state, mission);
		int travelDistanceMeters = Math.Round(Math.Sqrt(DistanceSq2D(sourcePosition, targetPosition)));
		HST_GeneratedRouteState route = ResolveMissionConvoyRoute(state, mission);
		string routeId = ResolveMissionConvoyRouteId(state, mission);
		string report = string.Format("\nconvoy mission | instance %1 | mission %2 | status %3 | phase %4", ReportText(mission.m_sInstanceId), ReportText(mission.m_sMissionId), mission.m_eStatus, ReportText(mission.m_sRuntimePhase));
		report = report + string.Format(" | ETA %1 | source position %2 | target position %3 | travel distance %4m", mission.m_iRuntimeETASeconds, sourcePosition, targetPosition, travelDistanceMeters);
		report = report + string.Format(" | route/site ID %1 | route ID %2", ReportRouteSite(state, mission.m_sSiteId), ReportRouteSite(state, routeId));
		report = report + string.Format(" | vehicle asset count %1 | mission failure reason %2", vehicleAssetCount, ReportText(mission.m_sRuntimeFailureReason));
		report = report + BuildMissionConvoyRouteReport(route);
		report = report + BuildMissionConvoyReadinessReport(state, mission);

		int assetIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, assetIndex);
			report = report + BuildConvoyVehicleAssetReport(asset, groupId);
			assetIndex++;
		}

		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!IsMissionConvoyGroupForMission(activeGroup, mission))
				continue;

			report = report + BuildConvoyActiveGroupReport(activeGroup);
		}

		return report;
	}

	protected string BuildConvoyVehicleAssetReport(HST_MissionAssetState asset, string groupId)
	{
		if (!asset)
			return "\n  convoy vehicle asset | missing";

		vector vehiclePosition = ResolveMissionConvoyVehiclePosition(asset, groupId);
		string report = string.Format("\n  convoy vehicle asset | asset %1 | prefab %2 | source position %3", ReportText(asset.m_sAssetId), ReportText(asset.m_sPrefab), asset.m_vSourcePosition);
		report = report + string.Format(" | current position %1 | target position %2 | spawned %3", vehiclePosition, asset.m_vTargetPosition, ReportBool(asset.m_bSpawned));
		report = report + string.Format(" | destroyed %1 | delivered/captured %2 | last interaction %3", ReportBool(asset.m_bDestroyed), ReportBool(asset.m_bDelivered), ReportText(asset.m_sLastInteraction));
		return report;
	}

	protected string BuildConvoyActiveGroupReport(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return "\n  convoy group | missing";

		bool crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId) != null;
		bool vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId) != null;
		int aliveCrew = CountAliveRuntimeCrewAgents(activeGroup);
		string report = string.Format("\n  convoy group | group %1 | faction %2 | prefab %3", ReportText(activeGroup.m_sGroupId), ReportText(activeGroup.m_sFactionKey), ReportText(activeGroup.m_sPrefab));
		report = report + string.Format(" | spawned entity %1 | crew entity %2 | vehicle entity %3", ReportBool(activeGroup.m_bSpawnedEntity), ReportBool(crewEntity), ReportBool(vehicleEntity));
		report = report + string.Format(" | runtime status %1 | crew count %2 | alive crew count %3", ReportText(activeGroup.m_sRuntimeStatus), activeGroup.m_iInfantryCount, aliveCrew);
		report = report + string.Format(" | route ID %1 | source position %2 | target position %3", ReportText(activeGroup.m_sRouteId), activeGroup.m_vSourcePosition, activeGroup.m_vTargetPosition);
		report = report + string.Format(" | fallback mode %1 | assigned waypoint count %2", ReportText(activeGroup.m_sSpawnFallbackMode), activeGroup.m_iAssignedWaypointCount);
		report = report + " | spawn failure reason " + ReportText(ResolveConvoyAdapterReportReason(activeGroup));
		report = report + BuildConvoyVehicleControlAdapterReport(activeGroup);
		return report;
	}

	protected string BuildConvoyVehicleControlAdapterReport(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return "\n    convoy adapter | group missing";

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		string mobileReason;
		bool mobile = GetConvoyVehicleControlAdapter().IsVehicleMobile(vehicleEntity, mobileReason);
		bool driver = GetConvoyVehicleControlAdapter().HasLivingDriver(crewEntity, vehicleEntity);
		int adapterCrew = GetConvoyVehicleControlAdapter().CountLivingCrew(crewEntity);
		string seatingReport = GetConvoyVehicleControlAdapter().BuildCrewSeatingReport(crewEntity, vehicleEntity);
		string result = activeGroup.m_sSpawnFallbackMode;
		if (result.IsEmpty())
			result = "none";
		string reason = ResolveConvoyAdapterReportReason(activeGroup);
		if (reason.IsEmpty())
			reason = mobileReason;

		string report = string.Format("\n    convoy adapter | living crew %1 | driver assigned %2 | vehicle mobile %3", adapterCrew, ReportBool(driver), ReportBool(mobile));
		report = report + " | " + seatingReport;
		report = report + string.Format(" | mobile reason %1 | last result %2 | reason %3", ReportText(mobileReason), ReportText(result), ReportText(reason));
		return report;
	}

	protected string ResolveConvoyAdapterReportReason(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return "";

		string reason = activeGroup.m_sSpawnFailureReason;
		if (!IsStaleConvoyCrewPopulationReason(reason))
			return reason;

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		int adapterCrew = GetConvoyVehicleControlAdapter().CountLivingCrew(crewEntity);
		if (adapterCrew <= 0)
			return reason;

		string mobileReason;
		if (!GetConvoyVehicleControlAdapter().IsVehicleMobile(vehicleEntity, mobileReason))
			return "Convoy adapter cannot bind crew: " + mobileReason;

		if (!GetConvoyVehicleControlAdapter().HasLivingDriver(crewEntity, vehicleEntity))
			return "Convoy adapter cannot bind crew: no seated living AI driver is assigned to the vehicle.";

		return "Convoy adapter found seated driver and mobile vehicle.";
	}

	protected bool IsStaleConvoyCrewPopulationReason(string reason)
	{
		return reason == "Convoy adapter cannot bind crew: no living crew agents." || reason == "Convoy adapter cannot bind crew yet: crew group has not populated living agents." || reason.Contains("planned for Phase") || reason.Contains("planned for later convoy phases");
	}

	protected string BuildMissionConvoyRouteReport(HST_GeneratedRouteState route)
	{
		if (!route)
			return "\n  convoy route | route none | road no | vehicle-safe no | waypoint count 0 | distance 0m | validation missing generated route";

		string validation = route.m_sValidationFailureReason;
		if (validation.IsEmpty())
			validation = "none";
		string report = string.Format("\n  convoy route | route %1 | road %2 | vehicle-safe %3", ReportText(route.m_sRouteId), ReportBool(route.m_bRoadRoute), ReportBool(route.m_bValidatedForVehicles));
		report = report + string.Format(" | waypoint count %1 | distance %2m | validation %3", route.m_iWaypointCount, route.m_iDistanceMeters, validation);
		foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
		{
			if (!waypoint)
				continue;

			report = report + string.Format("\n    active route waypoint %1 | hint %2 | radius %3m | position %4", waypoint.m_iIndex, ReportText(waypoint.m_sHint), waypoint.m_iRadiusMeters, waypoint.m_vPosition);
		}

		return report;
	}

	protected string BuildMissionConvoyReadinessReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_ConvoyReadinessStatus readiness = BuildMissionConvoyReadinessStatus(state, mission);
		string report = string.Format("\n  convoy readiness | ready %1 | pending grace %2 | static fallback %3 | reason %4", ReportBool(readiness.m_bReadyToMove), ReportBool(readiness.m_bPendingGrace), ReportBool(readiness.m_bStaticFallbackAvailable), ReportText(readiness.m_sReason));
		report = report + string.Format("\n    convoy readiness vehicle assets %1 | spawned vehicles %2", readiness.m_iVehicleAssetCount, readiness.m_iSpawnedVehicleCount);
		report = report + string.Format("\n    convoy readiness crew groups %1 | alive crew groups %2 | alive crew count %3", readiness.m_iCrewGroupCount, readiness.m_iAliveCrewGroupCount, readiness.m_iAliveCrewCount);
		report = report + string.Format("\n    convoy readiness driver availability %1 | mobile vehicles %2 | route assignment %3 | waypoint assignment %4", readiness.m_iDriverAvailableCount, readiness.m_iMobileVehicleCount, readiness.m_iRouteAssignedCount, readiness.m_iWaypointAssignedCount);
		return report;
	}

	protected HST_ConvoyReadinessStatus BuildMissionConvoyReadinessStatus(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_ConvoyReadinessStatus readiness = new HST_ConvoyReadinessStatus();
		if (!state || !mission)
		{
			readiness.m_sReason = "Convoy readiness state or mission missing.";
			return readiness;
		}

		HST_GeneratedRouteState route = ResolveMissionConvoyRoute(state, mission);
		string routeId = ResolveMissionConvoyRouteId(state, mission);
		int assetIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE || asset.m_bDestroyed || asset.m_bDelivered)
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, assetIndex);
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			readiness.m_iVehicleAssetCount++;
			if (asset.m_bSpawned && GetRuntimeVehicleEntity(groupId) != null)
				readiness.m_iSpawnedVehicleCount++;
			if (IsMissionConvoyCrewGroupSpawned(activeGroup))
				readiness.m_iCrewGroupCount++;

			int aliveCrew = CountAliveRuntimeCrewAgents(activeGroup);
			if (aliveCrew > 0)
				readiness.m_iAliveCrewGroupCount++;
			readiness.m_iAliveCrewCount += Math.Max(0, aliveCrew);

			if (HasMissionConvoyDriverAvailable(activeGroup))
				readiness.m_iDriverAvailableCount++;
			if (IsMissionConvoyVehicleMobile(activeGroup))
				readiness.m_iMobileVehicleCount++;
			if (IsMissionConvoyRouteAssigned(route, routeId, activeGroup))
				readiness.m_iRouteAssignedCount++;
			if (IsMissionConvoyWaypointAssigned(activeGroup))
				readiness.m_iWaypointAssignedCount++;

			assetIndex++;
		}

		readiness.m_bReadyToMove = IsMissionConvoyReadyToMove(readiness);
		readiness.m_bStaticFallbackAvailable = IsMissionConvoyStaticFallbackAvailable(readiness);
		readiness.m_bPendingGrace = IsMissionConvoyReadinessGraceActive(mission);
		readiness.m_sReason = ResolveMissionConvoyReadinessReason(readiness);
		return readiness;
	}

	protected bool IsMissionConvoyCrewGroupSpawned(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup || !activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "spawn_failed")
			return false;

		return GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId) != null;
	}

	protected bool HasMissionConvoyDriverAvailable(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		return GetConvoyVehicleControlAdapter().HasLivingDriver(crewEntity, vehicleEntity);
	}

	protected bool IsMissionConvoyVehicleMobile(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		string reason;
		return GetConvoyVehicleControlAdapter().IsVehicleMobile(GetRuntimeVehicleEntity(activeGroup.m_sGroupId), reason);
	}

	protected bool IsMissionConvoyRouteAssigned(HST_GeneratedRouteState route, string routeId, HST_ActiveGroupState activeGroup)
	{
		if (!route || routeId.IsEmpty() || !activeGroup || activeGroup.m_sRouteId.IsEmpty())
			return false;
		if (route.m_iWaypointCount < 3)
			return false;

		return activeGroup.m_sRouteId == routeId;
	}

	protected bool IsMissionConvoyWaypointAssigned(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		return activeGroup.m_sSpawnFallbackMode == "convoy_waypoints" && activeGroup.m_iAssignedWaypointCount > 1;
	}

	protected bool IsMissionConvoyReadyToMove(HST_ConvoyReadinessStatus readiness)
	{
		if (!readiness || readiness.m_iVehicleAssetCount < 3)
			return false;

		int required = readiness.m_iVehicleAssetCount;
		return readiness.m_iSpawnedVehicleCount >= required && readiness.m_iCrewGroupCount >= required && readiness.m_iAliveCrewGroupCount >= required && readiness.m_iMobileVehicleCount >= required && readiness.m_iDriverAvailableCount >= required && readiness.m_iRouteAssignedCount >= required && readiness.m_iWaypointAssignedCount >= required;
	}

	protected bool IsMissionConvoyStaticFallbackAvailable(HST_ConvoyReadinessStatus readiness)
	{
		if (!readiness || readiness.m_iVehicleAssetCount < 3)
			return false;

		int required = readiness.m_iVehicleAssetCount;
		return readiness.m_iSpawnedVehicleCount >= required && readiness.m_iCrewGroupCount >= required;
	}

	protected bool CanAttemptMissionConvoyWaypointAssignment(HST_ConvoyReadinessStatus readiness)
	{
		if (!readiness || readiness.m_iVehicleAssetCount < 3)
			return false;

		int required = readiness.m_iVehicleAssetCount;
		return readiness.m_iSpawnedVehicleCount >= required && readiness.m_iCrewGroupCount >= required && readiness.m_iAliveCrewGroupCount >= required && readiness.m_iMobileVehicleCount >= required && readiness.m_iDriverAvailableCount >= required && readiness.m_iRouteAssignedCount >= required && readiness.m_iWaypointAssignedCount < required;
	}

	protected bool IsMissionConvoyReadinessGraceActive(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_sRuntimePhase != MISSION_CONVOY_STAGING || mission.m_iRuntimeCounterB <= 0)
			return false;

		int elapsedPastStaging = mission.m_iRuntimeCounterA - mission.m_iRuntimeCounterB;
		return elapsedPastStaging >= 0 && elapsedPastStaging < CONVOY_READINESS_GRACE_SECONDS;
	}

	protected string ResolveMissionConvoyReadinessReason(HST_ConvoyReadinessStatus readiness)
	{
		if (!readiness)
			return "Convoy readiness state missing.";
		if (readiness.m_iVehicleAssetCount <= 0)
			return "No convoy vehicle assets exist; convoy cannot move.";
		if (readiness.m_iVehicleAssetCount < 3)
			return "Convoy needs at least three vehicle assets for readiness.";
		if (readiness.m_iSpawnedVehicleCount <= 0)
			return "No convoy vehicles spawned; convoy cannot move.";
		if (readiness.m_iSpawnedVehicleCount < readiness.m_iVehicleAssetCount)
			return "Not all convoy vehicle assets spawned physical vehicles.";
		if (readiness.m_iCrewGroupCount <= 0)
			return "Convoy crew groups failed to spawn.";
		if (readiness.m_iCrewGroupCount < readiness.m_iVehicleAssetCount)
			return "Not all convoy crew groups spawned.";
		if (readiness.m_iAliveCrewCount <= 0)
			return "Convoy crew groups have no living crew.";
		if (readiness.m_iAliveCrewGroupCount < readiness.m_iVehicleAssetCount)
			return "Not all convoy crew groups have living crew.";
		if (readiness.m_iMobileVehicleCount < readiness.m_iVehicleAssetCount)
			return "Not all convoy vehicles are mobile according to the vehicle-control adapter.";
		if (readiness.m_iDriverAvailableCount < readiness.m_iVehicleAssetCount)
			return "Convoy has no seated living AI driver yet.";
		if (readiness.m_iRouteAssignedCount < readiness.m_iVehicleAssetCount)
			return "Convoy route assignment failed.";
		if (readiness.m_iWaypointAssignedCount < readiness.m_iVehicleAssetCount)
			return "Convoy waypoint assignment failed.";

		return "ready";
	}

	protected int CountMissionConvoyVehicleAssets(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return 0;

		int count;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == instanceId && asset.m_sRole == MISSION_CONVOY_VEHICLE_ROLE)
				count++;
		}

		return count;
	}

	protected vector ResolveMissionConvoySourcePosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == MISSION_CONVOY_VEHICLE_ROLE)
				return asset.m_vSourcePosition;
		}

		return mission.m_vTargetPosition;
	}

	protected vector ResolveMissionConvoyTargetPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == MISSION_CONVOY_VEHICLE_ROLE)
				return asset.m_vTargetPosition;
		}

		return mission.m_vTargetPosition;
	}

	protected string ResolveMissionConvoyRouteId(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_GeneratedRouteState route = ResolveMissionConvoyRoute(state, mission);
		if (route)
			return route.m_sRouteId;

		return "";
	}

	protected HST_GeneratedRouteState ResolveMissionConvoyRoute(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sSiteId.IsEmpty())
			return null;

		HST_GeneratedSiteState site = state.FindGeneratedSite(mission.m_sSiteId);
		if (!site || site.m_sRouteId.IsEmpty())
			return null;

		return state.FindGeneratedRoute(site.m_sRouteId);
	}

	protected bool IsMissionConvoyGroupForMission(HST_ActiveGroupState activeGroup, HST_ActiveMissionState mission)
	{
		if (!IsMissionConvoyGroup(activeGroup) || !mission || mission.m_sInstanceId.IsEmpty())
			return false;

		string prefix = string.Format("%1%2_", MISSION_CONVOY_GROUP_PREFIX, mission.m_sInstanceId);
		return activeGroup.m_sGroupId.Contains(prefix);
	}

	protected string ReportText(string value)
	{
		if (value.IsEmpty())
			return "none";

		return value;
	}

	protected string ReportBool(bool value)
	{
		if (value)
			return "yes";

		return "no";
	}

	protected string ReportRouteSite(HST_CampaignState state, string routeOrSiteId)
	{
		if (routeOrSiteId.IsEmpty())
			return "none";
		if (!state)
			return "missing:" + routeOrSiteId;

		if (state.FindGeneratedSite(routeOrSiteId))
			return routeOrSiteId;
		if (state.FindGeneratedRoute(routeOrSiteId))
			return routeOrSiteId;

		return "missing:" + routeOrSiteId;
	}

	protected bool HasPendingConvoyCrewPopulation(HST_CampaignState state, HST_ActiveMissionState mission, int vehicleAssets)
	{
		if (!state || !mission)
			return false;

		for (int index = 0; index < vehicleAssets; index++)
		{
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			if (IsConvoyCrewPopulationPending(state, activeGroup))
				return true;
		}

		return false;
	}

	protected bool IsConvoyCrewPopulationPending(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup || !activeGroup.m_bSpawnedEntity || activeGroup.m_iSpawnedAgentCount > 0)
			return false;

		return state.m_iElapsedSeconds < activeGroup.m_iSpawnedAtSecond + CONVOY_CREW_POPULATION_GRACE_SECONDS;
	}

	protected string BuildMissionConvoyGroupId(HST_ActiveMissionState mission, int index)
	{
		if (!mission)
			return "";

		return string.Format("%1%2_%3", MISSION_CONVOY_GROUP_PREFIX, mission.m_sInstanceId, index);
	}

	protected bool IsMissionConvoyGroup(HST_ActiveGroupState activeGroup)
	{
		return activeGroup && activeGroup.m_sGroupId.Contains(MISSION_CONVOY_GROUP_PREFIX);
	}

	protected string ResolveMissionConvoyRuntimeStatus(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_sRuntimePhase.IsEmpty() || mission.m_sRuntimePhase == "created" || mission.m_sRuntimePhase == "active" || mission.m_sRuntimePhase == "convoy_static")
			return MISSION_CONVOY_STAGING;

		return mission.m_sRuntimePhase;
	}

	protected HST_ZoneState ResolveMissionConvoyTargetZone(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!state)
			return null;

		if (mission && !mission.m_sTargetZoneId.IsEmpty())
		{
			HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
			if (zone)
				return zone;
		}

		if (asset)
			return FindNearestZone(state, asset.m_vTargetPosition);

		return null;
	}

	protected HST_ZoneState FindNearestZone(HST_CampaignState state, vector position)
	{
		if (!state)
			return null;

		HST_ZoneState bestZone;
		float bestDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			float distanceSq = DistanceSq2D(zone.m_vPosition, position);
			if (distanceSq < bestDistanceSq)
			{
				bestDistanceSq = distanceSq;
				bestZone = zone;
			}
		}

		return bestZone;
	}

	protected string ResolveMissionConvoyFactionKey(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_ZoneState targetZone)
	{
		if (targetZone && !targetZone.m_sOwnerFactionKey.IsEmpty())
		{
			if (!preset || targetZone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
				return targetZone.m_sOwnerFactionKey;
		}

		if (preset && !preset.m_sOccupierFactionKey.IsEmpty())
			return preset.m_sOccupierFactionKey;

		return "US";
	}

	protected string SelectConvoyCrewGroupPrefab(HST_CampaignState state, HST_ZoneState zone, string factionKey, int index)
	{
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (!faction)
			return "";

		array<string> candidates = {};
		AppendUniqueGroupPrefabs(candidates, faction.m_aPatrolGroupPrefabs);
		AppendUniqueGroupPrefabs(candidates, faction.m_aGroupPrefabs);
		int seed = BuildGroupSelectionSeed(state, zone, false) + index * 71 + 9001;
		string compactPrefab = SelectPreferredCompactGroupPrefab(candidates, seed, factionKey, "mission convoy crew");
		if (!compactPrefab.IsEmpty())
			return compactPrefab;

		return SelectValidGroupPrefabFromList(candidates, seed, factionKey, "mission convoy crew");
	}

	protected string SelectPreferredCompactGroupPrefab(array<string> prefabs, int seed, string factionKey, string purpose)
	{
		if (!prefabs || prefabs.Count() == 0)
			return "";

		int startIndex = HST_DefaultCatalog.PositiveMod(seed, prefabs.Count());
		for (int offset = 0; offset < prefabs.Count(); offset++)
		{
			int index = HST_DefaultCatalog.PositiveMod(startIndex + offset, prefabs.Count());
			string prefab = prefabs[index];
			if (!IsCompactCrewGroupPrefab(prefab))
				continue;
			if (!IsValidGroupPrefabResource(prefab, factionKey))
				continue;

			return prefab;
		}

		return "";
	}

	protected bool IsCompactCrewGroupPrefab(string prefab)
	{
		return prefab.Contains("SentryTeam");
	}

	protected int ResolveMissionConvoyCrewCount(HST_CampaignState state, HST_ActiveMissionState mission, int index)
	{
		return 2;
	}

	protected vector OffsetConvoyCrewSpawnPosition(vector vehiclePosition, vector targetPosition, int index)
	{
		vector result = vehiclePosition;
		float dx = targetPosition[0] - vehiclePosition[0];
		float dz = targetPosition[2] - vehiclePosition[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		float sideX = 0.0;
		float sideZ = 1.0;
		if (length > 1.0)
		{
			sideX = -dz / length;
			sideZ = dx / length;
		}

		float sideOffset = 5.5;
		if ((index - (index / 2) * 2) == 1)
			sideOffset = -5.5;

		result[0] = result[0] + sideX * sideOffset;
		result[2] = result[2] + sideZ * sideOffset;
		return HST_WorldPositionService.ResolveSafeGroundPosition(result, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 3.0);
	}

	protected vector BuildConvoyVehicleAngles(vector sourcePosition, vector targetPosition)
	{
		float dx = targetPosition[0] - sourcePosition[0];
		float dz = targetPosition[2] - sourcePosition[2];
		float yaw;
		float absDx = dx;
		if (absDx < 0)
			absDx = -absDx;
		float absDz = dz;
		if (absDz < 0)
			absDz = -absDz;
		if (absDx > absDz)
		{
			if (dx >= 0)
				yaw = 90;
			else
				yaw = 270;
		}
		else if (dz < 0)
		{
			yaw = 180;
		}

		return HST_WorldPositionService.BuildUprightAngles(yaw);
	}

	protected bool AllMissionConvoyGroupsAttempted(HST_CampaignState state, HST_ActiveMissionState mission, int vehicleAssets)
	{
		if (!state || !mission || vehicleAssets <= 0)
			return false;

		for (int i = 0; i < vehicleAssets; i++)
		{
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, i));
			if (!activeGroup || (!activeGroup.m_bSpawnAttempted && !activeGroup.m_bSpawnedEntity))
				return false;
		}

		return true;
	}

	protected void ApplyMissionConvoyStatusToGroups(HST_CampaignState state, HST_ActiveMissionState mission, string status)
	{
		if (!state || !mission || status.IsEmpty())
			return;

		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!IsMissionConvoyGroup(activeGroup) || !activeGroup.m_sGroupId.Contains(mission.m_sInstanceId))
				continue;
			if (activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED || activeGroup.m_sRuntimeStatus == "spawn_failed")
				continue;

			activeGroup.m_sRuntimeStatus = status;
		}
	}

	protected ref array<vector> BuildMissionConvoyWaypointPositions(HST_GeneratedRouteState route)
	{
		ref array<vector> waypoints = {};
		if (!route)
			return waypoints;

		if (!route.m_aWaypoints || route.m_aWaypoints.Count() == 0)
		{
			if (!IsZeroVector(route.m_vEndPosition))
				waypoints.Insert(route.m_vEndPosition);

			return waypoints;
		}

		int lastIndex = -1000000;
		while (true)
		{
			HST_RouteWaypointState selectedWaypoint;
			int selectedIndex = 1000000;
			foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
			{
				if (!waypoint)
					continue;
				if (waypoint.m_iIndex <= lastIndex)
					continue;
				if (waypoint.m_iIndex >= selectedIndex)
					continue;

				selectedWaypoint = waypoint;
				selectedIndex = waypoint.m_iIndex;
			}

			if (!selectedWaypoint)
				break;

			waypoints.Insert(selectedWaypoint.m_vPosition);
			lastIndex = selectedWaypoint.m_iIndex;
		}

		return waypoints;
	}

	protected bool AssignMissionConvoyWaypoints(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		ref array<vector> waypoints = BuildMissionConvoyWaypointPositions(ResolveMissionConvoyRoute(state, mission));
		int eligibleGroups;
		int assignedGroups;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!IsMissionConvoyGroup(activeGroup) || !activeGroup.m_sGroupId.Contains(mission.m_sInstanceId) || !activeGroup.m_bSpawnedEntity)
				continue;
			eligibleGroups++;
			if (IsMissionConvoyWaypointAssigned(activeGroup))
			{
				assignedGroups++;
				continue;
			}

			string previousReason = activeGroup.m_sSpawnFailureReason;
			if (TryAssignConvoyWaypoints(activeGroup, waypoints))
			{
				activeGroup.m_sSpawnFallbackMode = "convoy_waypoints";
				assignedGroups++;
			}
			else
			{
				if (activeGroup.m_sSpawnFallbackMode.IsEmpty() || activeGroup.m_sSpawnFallbackMode == "convoy_crew_near_vehicle" || activeGroup.m_sSpawnFallbackMode == "convoy_seating_pending")
					activeGroup.m_sSpawnFallbackMode = "convoy_vehicle_control_unavailable";
				if (activeGroup.m_sSpawnFailureReason.IsEmpty())
					activeGroup.m_sSpawnFailureReason = "Convoy has no seated living AI driver yet.";
				if (mission.m_sRuntimeFailureReason.IsEmpty())
					mission.m_sRuntimeFailureReason = activeGroup.m_sSpawnFailureReason;
				if (activeGroup.m_sSpawnFailureReason != previousReason)
					Print(string.Format("h-istasi mission convoy | waypoint assignment unavailable for %1: %2", activeGroup.m_sGroupId, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
			}
		}

		return eligibleGroups > 0 && assignedGroups == eligibleGroups;
	}

	protected bool TryAssignConvoyWaypoints(HST_ActiveGroupState activeGroup, array<vector> waypoints)
	{
		if (!activeGroup)
			return false;

		ref array<vector> groupWaypoints = BuildMissionConvoyGroupWaypointPositions(activeGroup, waypoints);
		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		IEntity vehicle = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		int assignedWaypointCount;
		string adapterReason;
		if (GetConvoyVehicleControlAdapter().TryAssignVehicleRoute(activeGroup, crewEntity, vehicle, groupWaypoints, assignedWaypointCount, adapterReason) && assignedWaypointCount > 1)
		{
			activeGroup.m_iAssignedWaypointCount = assignedWaypointCount;
			activeGroup.m_sSpawnFailureReason = adapterReason;
			return true;
		}

		activeGroup.m_iAssignedWaypointCount = 0;
		if (!vehicle)
			activeGroup.m_sSpawnFallbackMode = "convoy_vehicle_missing";
		else
			activeGroup.m_sSpawnFallbackMode = "convoy_vehicle_control_unavailable";
		activeGroup.m_sSpawnFailureReason = adapterReason;
		if (activeGroup.m_sSpawnFailureReason.IsEmpty())
			activeGroup.m_sSpawnFailureReason = "Convoy has no seated living AI driver yet.";
		return false;
	}

	protected ref array<vector> BuildMissionConvoyGroupWaypointPositions(HST_ActiveGroupState activeGroup, array<vector> routeWaypoints)
	{
		ref array<vector> result = {};
		if (!activeGroup)
			return result;

		if (routeWaypoints && routeWaypoints.Count() > 0)
		{
			AppendConvoyLeadInWaypoints(result, activeGroup.m_vSourcePosition, routeWaypoints[0]);
			foreach (vector routeWaypoint : routeWaypoints)
			{
				result.Insert(routeWaypoint);
			}
		}

		if (result.Count() == 0 && !IsZeroVector(activeGroup.m_vTargetPosition))
			result.Insert(activeGroup.m_vTargetPosition);

		return result;
	}

	protected void AppendConvoyLeadInWaypoints(array<vector> waypoints, vector sourcePosition, vector routeStartPosition)
	{
		if (!waypoints || IsZeroVector(sourcePosition) || IsZeroVector(routeStartPosition))
			return;

		float distanceSq = DistanceSq2D(sourcePosition, routeStartPosition);
		if (distanceSq <= 350.0 * 350.0)
			return;

		for (int step = 1; step <= 4; step++)
		{
			float fraction = step / 5.0;
			vector candidate = sourcePosition;
			candidate[0] = sourcePosition[0] + (routeStartPosition[0] - sourcePosition[0]) * fraction;
			candidate[2] = sourcePosition[2] + (routeStartPosition[2] - sourcePosition[2]) * fraction;

			vector resolved;
			if (HST_WorldPositionService.TryResolveLargeVehicleSpawnPosition(candidate, resolved, true))
				waypoints.Insert(resolved);
		}
	}

	protected void SyncMissionConvoyAssetPositions(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;

		int index;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			vector position = ResolveMissionConvoyVehiclePosition(asset, BuildMissionConvoyGroupId(mission, index));
			UpdateMissionConvoyAssetPosition(state, asset, position);
			index++;
		}
	}

	protected vector ResolveMissionConvoyVehiclePosition(HST_MissionAssetState asset, string groupId)
	{
		IEntity vehicle = GetRuntimeVehicleEntity(groupId);
		if (vehicle)
			return vehicle.GetOrigin();

		if (asset)
			return asset.m_vCurrentPosition;

		return "0 0 0";
	}

	protected void UpdateMissionConvoyAssetPosition(HST_CampaignState state, HST_MissionAssetState asset, vector position)
	{
		if (!state || !asset || IsZeroVector(position))
			return;

		asset.m_vCurrentPosition = position;
		asset.m_vLastKnownPosition = position;
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
			runtimeEntity.m_vPosition = position;
	}

	protected bool ApplyMissionConvoyObjectiveProgress(HST_CampaignState state, HST_ActiveMissionState mission, int eliminatedGroups, int totalGroups)
	{
		if (!state || !mission || totalGroups <= 0)
			return false;

		bool changed;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_sTargetId != "convoy")
				continue;

			int required = Math.Max(1, totalGroups);
			int current = Math.Min(required, Math.Max(0, eliminatedGroups));
			if (objective.m_iRequiredCount != required || objective.m_iRequiredProgress != required)
			{
				objective.m_iRequiredCount = required;
				objective.m_iRequiredProgress = required;
				changed = true;
			}

			if (objective.m_iCurrentCount != current || objective.m_iCurrentProgress != current)
			{
				objective.m_iCurrentCount = current;
				objective.m_iCurrentProgress = current;
				changed = true;
			}

			if (current >= required && !objective.m_bComplete)
			{
				objective.m_bComplete = true;
				changed = true;
			}
		}

		return changed;
	}

	protected bool IsMissionConvoyAtDestination(HST_ActiveMissionState mission, vector position)
	{
		if (!mission || IsZeroVector(position) || IsZeroVector(mission.m_vTargetPosition))
			return false;

		return DistanceSq2D(position, mission.m_vTargetPosition) <= CONVOY_DESTINATION_RADIUS_METERS * CONVOY_DESTINATION_RADIUS_METERS;
	}

	protected void SetMissionConvoyFailure(HST_CampaignState state, HST_ActiveMissionState mission, string reason)
	{
		if (!state || !mission || mission.m_sRuntimePhase == MISSION_CONVOY_FAILED)
			return;

		mission.m_sRuntimePhase = MISSION_CONVOY_FAILED;
		mission.m_sRuntimeFailureReason = reason;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete)
				continue;

			objective.m_bFailed = true;
		}

		m_bMarkerRefreshNeeded = true;
		Print(string.Format("h-istasi mission convoy | %1 failed: %2", mission.m_sInstanceId, reason), LogLevel.WARNING);
	}

	protected IEntity GetRuntimeCrewGroupEntity(string groupId)
	{
		int index = m_aRuntimeGroupIds.Find(groupId);
		if (index < 0 || index >= m_aRuntimeGroupEntities.Count())
			return null;

		return m_aRuntimeGroupEntities[index];
	}

	protected IEntity GetRuntimeVehicleEntity(string groupId)
	{
		int index = m_aRuntimeVehicleGroupIds.Find(groupId);
		if (index < 0 || index >= m_aRuntimeVehicleEntities.Count())
			return null;

		return m_aRuntimeVehicleEntities[index];
	}

	protected int CountAliveRuntimeCrewAgents(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return 0;

		int aliveCount = CountAliveRuntimeCrewAgents(activeGroup.m_sGroupId);
		if (activeGroup.m_iInfantryCount <= 0)
			return aliveCount;

		return Math.Min(activeGroup.m_iInfantryCount, aliveCount);
	}

	protected void RefreshMissionConvoyCrewCount(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return;

		int livingCrew = CountAliveRuntimeCrewAgents(activeGroup.m_sGroupId);
		if (livingCrew <= 0)
			return;

		activeGroup.m_iInfantryCount = livingCrew;
		activeGroup.m_iSurvivorInfantryCount = livingCrew;
		activeGroup.m_iLastSeenAliveCount = livingCrew;
		activeGroup.m_iSpawnedAgentCount = Math.Max(activeGroup.m_iSpawnedAgentCount, livingCrew);
	}

	protected int CountAliveRuntimeCrewAgents(string groupId)
	{
		int aliveCount;
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId)
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (!entity)
				continue;

			aliveCount += GetConvoyVehicleControlAdapter().CountLivingCrew(entity);
		}

		return aliveCount;
	}

	protected HST_ActiveMissionState FindMissionForConvoyGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup)
			return null;

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
				continue;
			if (activeGroup.m_sGroupId.Contains(mission.m_sInstanceId))
				return mission;
		}

		return null;
	}

	protected HST_MissionAssetState FindMissionConvoyAssetForGroup(HST_CampaignState state, HST_ActiveMissionState mission, HST_ActiveGroupState activeGroup)
	{
		if (!state || !mission || !activeGroup)
			return null;

		int targetIndex = ResolveMissionConvoyGroupIndex(mission, activeGroup.m_sGroupId);
		int index;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;
			if (index == targetIndex)
				return asset;

			index++;
		}

		return null;
	}

	protected int ResolveMissionConvoyGroupIndex(HST_ActiveMissionState mission, string groupId)
	{
		if (!mission || groupId.IsEmpty())
			return 0;

		string prefix = string.Format("%1%2_", MISSION_CONVOY_GROUP_PREFIX, mission.m_sInstanceId);
		if (!groupId.Contains(prefix))
			return 0;

		string suffix = groupId.Substring(prefix.Length(), groupId.Length() - prefix.Length());
		return suffix.ToInt();
	}


	protected bool IsAnyLivingPlayerNearZone(PlayerManager playerManager, array<int> playerIds, HST_ZoneState zone, HST_BalanceConfig balance)
	{
		if (!zone)
			return false;

		float radius = zone.m_iActivationRadiusMeters;
		if (radius <= 0)
			radius = balance.m_iActivationRadiusMeters;

		float radiusSq = radius * radius;
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (!IsLivingPlayerEntity(playerEntity))
				continue;

			if (DistanceSq2D(playerEntity.GetOrigin(), zone.m_vPosition) <= radiusSq)
				return true;
		}

		return false;
	}

	protected bool ActivateZone(HST_CampaignState state, HST_ZoneState zone, HST_ZoneCompositionService compositions = null)
	{
		bool changed;
		array<ref HST_ZoneSpawnSlotState> slots = {};
		if (compositions)
		{
			changed = compositions.EnsureZoneComposition(state, zone) || changed;
			slots = compositions.BuildZoneSpawnSlots(state, zone);
		}

		if (HasActiveGarrisonGroup(state, zone))
		{
			ApplyActiveZoneCounts(state, zone);
			return changed;
		}

		HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, zone.m_sOwnerFactionKey);
		if (!garrison)
		{
			zone.m_iActiveInfantryCount = 0;
			zone.m_iActiveVehicleCount = 0;
			return changed;
		}

		int garrisonInfantryBefore = garrison.m_iInfantryCount;
		int garrisonVehiclesBefore = garrison.m_iVehicleCount;
		int infantryCount = Math.Min(garrison.m_iInfantryCount, ResolveActiveInfantryCap(zone));
		int vehicleCount = Math.Min(garrison.m_iVehicleCount, ResolveActiveVehicleCap(zone));
		if (infantryCount <= 0 && vehicleCount <= 0)
		{
			zone.m_iActiveInfantryCount = 0;
			zone.m_iActiveVehicleCount = 0;
			return changed;
		}

		garrison.m_iInfantryCount = Math.Max(0, garrison.m_iInfantryCount - infantryCount);
		garrison.m_iVehicleCount = Math.Max(0, garrison.m_iVehicleCount - vehicleCount);

		int spawnedInfantryGroups = SpawnZoneInfantryGroups(state, zone, slots, infantryCount, compositions);
		int spawnedVehicleGroups = SpawnZoneVehicleGroups(state, zone, slots, vehicleCount);
		ApplyActiveZoneCounts(state, zone);
		string activationReport = string.Format("h-istasi | activated zone %1 | requested infantry %2/%3 vehicles %4/%5 | spawned infantry groups %6 vehicle groups %7", zone.m_sZoneId, infantryCount, garrisonInfantryBefore, vehicleCount, garrisonVehiclesBefore, spawnedInfantryGroups, spawnedVehicleGroups);
		activationReport = activationReport + string.Format(" | active now infantry %1 vehicles %2 | abstract garrison now infantry %3 vehicles %4", zone.m_iActiveInfantryCount, zone.m_iActiveVehicleCount, garrison.m_iInfantryCount, garrison.m_iVehicleCount);
		Print(string.Format("%1", activationReport));
		return true;
	}

	protected bool DeactivateZone(HST_CampaignState state, HST_ZoneState zone, HST_ZoneCompositionService compositions = null)
	{
		bool changed;
		int foldedGroups;
		int returnedInfantry;
		int returnedVehicles;
		for (int i = state.m_aActiveGroups.Count() - 1; i >= 0; i--)
		{
			HST_ActiveGroupState activeGroup = state.m_aActiveGroups[i];
			if (!activeGroup || activeGroup.m_sZoneId != zone.m_sZoneId || activeGroup.m_bQRF || IsMissionConvoyGroup(activeGroup))
				continue;

			int beforeInfantry;
			int beforeVehicles;
			HST_GarrisonState beforeGarrison = state.FindGarrison(activeGroup.m_sZoneId, activeGroup.m_sFactionKey);
			if (beforeGarrison)
			{
				beforeInfantry = beforeGarrison.m_iInfantryCount;
				beforeVehicles = beforeGarrison.m_iVehicleCount;
			}

			FoldActiveGroup(state, activeGroup);
			HST_GarrisonState afterGarrison = state.FindGarrison(activeGroup.m_sZoneId, activeGroup.m_sFactionKey);
			if (afterGarrison)
			{
				returnedInfantry += Math.Max(0, afterGarrison.m_iInfantryCount - beforeInfantry);
				returnedVehicles += Math.Max(0, afterGarrison.m_iVehicleCount - beforeVehicles);
			}

			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			state.m_aActiveGroups.Remove(i);
			foldedGroups++;
			changed = true;
		}

		zone.m_iActiveInfantryCount = 0;
		zone.m_iActiveVehicleCount = 0;
		if (compositions)
			changed = compositions.CleanupZoneComposition(zone.m_sZoneId) || changed;
		if (changed)
		{
			HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, zone.m_sOwnerFactionKey);
			int garrisonInfantry;
			int garrisonVehicles;
			if (garrison)
			{
				garrisonInfantry = garrison.m_iInfantryCount;
				garrisonVehicles = garrison.m_iVehicleCount;
			}

			Print(string.Format("h-istasi | deactivated zone %1 | folded groups %2 | returned infantry %3 vehicles %4 | abstract garrison now infantry %5 vehicles %6", zone.m_sZoneId, foldedGroups, returnedInfantry, returnedVehicles, garrisonInfantry, garrisonVehicles));
		}
		return changed;
	}

	protected int SpawnZoneInfantryGroups(HST_CampaignState state, HST_ZoneState zone, array<ref HST_ZoneSpawnSlotState> slots, int infantryCount, HST_ZoneCompositionService compositions)
	{
		if (infantryCount <= 0)
			return 0;

		int groupCount = ResolveInfantryGroupCount(infantryCount);
		int remainingInfantry = infantryCount;
		int spawnedGroups;
		bool patrolAssigned;
		for (int groupIndex = 0; groupIndex < groupCount; groupIndex++)
		{
			int remainingGroups = groupCount - groupIndex;
			int groupInfantry = Math.Max(1, remainingInfantry / remainingGroups);
			remainingInfantry = Math.Max(0, remainingInfantry - groupInfantry);

			HST_ZoneSpawnSlotState slot;
			string runtimeStatus = "guard_center";
			if (compositions)
			{
				if (groupIndex == 0)
				{
					slot = compositions.SelectSlot(slots, HST_ZoneCompositionService.SLOT_INFANTRY, 0);
				}
				else
				{
					slot = compositions.SelectSlot(slots, HST_ZoneCompositionService.SLOT_PATROL, groupIndex - 1);
					runtimeStatus = "patrol_distributed";
					patrolAssigned = true;
				}

				if (!slot)
					slot = compositions.SelectSlot(slots, HST_ZoneCompositionService.SLOT_INFANTRY, groupIndex);
			}

			if (SpawnActiveZoneGroup(state, zone, zone.m_sOwnerFactionKey, groupInfantry, 0, false, slot, runtimeStatus))
				spawnedGroups++;
		}

		if (patrolAssigned && compositions)
			compositions.ReportPatrolWaypointUnavailable(zone.m_sZoneId);

		return spawnedGroups;
	}

	protected int SpawnZoneVehicleGroups(HST_CampaignState state, HST_ZoneState zone, array<ref HST_ZoneSpawnSlotState> slots, int vehicleCount)
	{
		if (vehicleCount <= 0)
			return 0;

		int spawnedVehicles;
		for (int vehicleIndex = 0; vehicleIndex < vehicleCount; vehicleIndex++)
		{
			HST_ZoneSpawnSlotState slot;
			if (slots)
			{
				foreach (HST_ZoneSpawnSlotState candidate : slots)
				{
					if (!candidate || candidate.m_sKind != HST_ZoneCompositionService.SLOT_VEHICLE || candidate.m_bOccupied)
						continue;

					candidate.m_bOccupied = true;
					slot = candidate;
					break;
				}
			}

			if (!slot)
			{
				FoldUnspawnedForces(state, zone.m_sZoneId, zone.m_sOwnerFactionKey, 0, 1);
				Print(string.Format("h-istasi | vehicle spawn skipped for %1: no safe vehicle slot", zone.m_sZoneId), LogLevel.WARNING);
				continue;
			}

			if (SpawnActiveZoneGroup(state, zone, zone.m_sOwnerFactionKey, 0, 1, false, slot, "vehicle_guard"))
				spawnedVehicles++;
		}

		return spawnedVehicles;
	}

	protected bool SpawnActiveZoneGroup(HST_CampaignState state, HST_ZoneState zone, string factionKey, int infantryCount, int vehicleCount, bool qrf, HST_ZoneSpawnSlotState slot, string runtimeStatus)
	{
		HST_ActiveGroupState activeGroup = CreateActiveGroup(state, zone, factionKey, infantryCount, vehicleCount, qrf);
		ApplySpawnSlot(activeGroup, slot, runtimeStatus);
		if (vehicleCount > 0 && infantryCount <= 0)
		{
			activeGroup.m_sPrefab = SelectVehiclePrefab(state, zone, factionKey, state.m_aActiveGroups.Count());
			activeGroup.m_sSpawnFallbackMode = "vehicle";
		}

		state.m_aActiveGroups.Insert(activeGroup);
		bool spawned;
		if (vehicleCount > 0 && infantryCount <= 0)
			spawned = TrySpawnActiveVehicle(activeGroup, state);
		else
			spawned = TrySpawnActiveGroup(activeGroup, state);

		if (!spawned)
		{
			FoldActiveGroup(state, activeGroup);
			state.m_aActiveGroups.Remove(state.m_aActiveGroups.Count() - 1);
			ApplyActiveZoneCounts(state, zone);
			NotifyRuntimeEvent(state, "ai_spawn_failed_" + activeGroup.m_sGroupId, "Enemy Spawn Failed", string.Format("%1 could not spawn at %2. %3", zone.m_sDisplayName, zone.m_sZoneId, activeGroup.m_sSpawnFailureReason), zone.m_sZoneId, activeGroup.m_vPosition, 6.0);
			return false;
		}

		return true;
	}

	protected void ApplySpawnSlot(HST_ActiveGroupState activeGroup, HST_ZoneSpawnSlotState slot, string runtimeStatus)
	{
		if (!activeGroup)
			return;

		if (slot)
		{
			activeGroup.m_vPosition = slot.m_vPosition;
			activeGroup.m_vSourcePosition = slot.m_vPosition;
			activeGroup.m_vTargetPosition = slot.m_vPosition;
			activeGroup.m_sSpawnFallbackMode = slot.m_sSlotId;
		}

		if (!runtimeStatus.IsEmpty())
			activeGroup.m_sRuntimeStatus = runtimeStatus;
	}

	protected int ResolveInfantryGroupCount(int infantryCount)
	{
		if (infantryCount <= 4)
			return 1;
		if (infantryCount <= 8)
			return 2;
		if (infantryCount <= 12)
			return 3;

		return 4;
	}

	protected void FoldUnspawnedForces(HST_CampaignState state, string zoneId, string factionKey, int infantryCount, int vehicleCount)
	{
		if (!state || zoneId.IsEmpty() || factionKey.IsEmpty())
			return;

		HST_GarrisonState garrison = state.FindGarrison(zoneId, factionKey);
		if (!garrison)
		{
			garrison = new HST_GarrisonState();
			garrison.m_sZoneId = zoneId;
			garrison.m_sFactionKey = factionKey;
			state.m_aGarrisons.Insert(garrison);
		}

		garrison.m_iInfantryCount += Math.Max(0, infantryCount);
		garrison.m_iVehicleCount += Math.Max(0, vehicleCount);
	}

	protected bool UpdateQRF(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector)
	{
		bool changed = SpawnPendingQRFs(state);
		changed = ResolveArrivedQRFs(state) || changed;
		if (!enemyDirector)
			return changed;

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone.m_bActive || zone.m_sOwnerFactionKey == resistanceFactionKey || zone.m_sQRFRouteId.IsEmpty())
				continue;

			if (IsZoneInsideHQSafeArea(state, zone))
				continue;

			if (state.m_iElapsedSeconds < zone.m_iQrfCooldownUntilSecond)
				continue;

			if (state.FindActiveQRF(zone.m_sZoneId, zone.m_sOwnerFactionKey))
				continue;

			if (!enemyDirector.TrySpend(state, zone.m_sOwnerFactionKey, QRF_ATTACK_RESOURCE_COST, QRF_SUPPORT_RESOURCE_COST))
				continue;

			HST_QRFState qrf = new HST_QRFState();
			qrf.m_sInstanceId = string.Format("qrf_%1_%2_%3", zone.m_sZoneId, zone.m_sOwnerFactionKey, state.m_iElapsedSeconds);
			qrf.m_sFactionKey = zone.m_sOwnerFactionKey;
			qrf.m_sSourceZoneId = ResolveQRFSourceZoneId(state, zone, zone.m_sOwnerFactionKey);
			qrf.m_sTargetZoneId = zone.m_sZoneId;
			qrf.m_iStartedAtSecond = state.m_iElapsedSeconds;
			qrf.m_iETASeconds = QRF_ETA_SECONDS;
			state.m_aQRFs.Insert(qrf);
			zone.m_iQrfCooldownUntilSecond = state.m_iElapsedSeconds + QRF_COOLDOWN_SECONDS;
			m_bMarkerRefreshNeeded = true;
			Print(string.Format("h-istasi | dispatched QRF %1 from %2 to active zone %3 | physical spawn at T-%4s", qrf.m_sInstanceId, qrf.m_sSourceZoneId, zone.m_sZoneId, QRF_INBOUND_SPAWN_SECONDS));
			NotifyRuntimeEvent(state, "qrf_dispatched_" + qrf.m_sInstanceId, "QRF Dispatched", string.Format("%1 is sending a quick reaction force toward %2.", zone.m_sOwnerFactionKey, ResolveZoneDisplayName(state, zone.m_sZoneId)), zone.m_sZoneId, zone.m_vPosition, 6.0);
			changed = true;
		}

		return changed;
	}

	protected bool SpawnPendingQRFs(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (!qrf || qrf.m_bResolved || !qrf.m_sGroupId.IsEmpty())
				continue;

			int spawnAtSecond = qrf.m_iStartedAtSecond + Math.Max(0, qrf.m_iETASeconds - QRF_INBOUND_SPAWN_SECONDS);
			if (state.m_iElapsedSeconds < spawnAtSecond)
				continue;

			HST_ZoneState targetZone = state.FindZone(qrf.m_sTargetZoneId);
			if (!targetZone)
			{
				qrf.m_bResolved = true;
				qrf.m_bSucceeded = false;
				m_bMarkerRefreshNeeded = true;
				Print(string.Format("h-istasi | QRF %1 failed to stage near objective: missing target zone %2", qrf.m_sInstanceId, qrf.m_sTargetZoneId), LogLevel.WARNING);
				changed = true;
				continue;
			}

			if (qrf.m_sSourceZoneId.IsEmpty())
				qrf.m_sSourceZoneId = ResolveQRFSourceZoneId(state, targetZone, qrf.m_sFactionKey);

			int infantryCount = Math.Min(ResolveActiveInfantryCap(targetZone), Math.Max(2, targetZone.m_iGarrisonSlots / 3 + state.m_iWarLevel / 2));
			int vehicleCount = ResolveActiveVehicleCap(targetZone);
			HST_ActiveGroupState activeGroup = CreateActiveGroup(state, targetZone, qrf.m_sFactionKey, infantryCount, vehicleCount, true);
			vector targetPosition = HST_WorldPositionService.ResolveGroundPosition(targetZone.m_vPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);
			vector sourcePosition = ResolveQRFStagingPosition(state, qrf, targetZone, targetPosition);
			activeGroup.m_sRouteId = qrf.m_sSourceZoneId + "_to_" + qrf.m_sTargetZoneId;
			activeGroup.m_vSourcePosition = sourcePosition;
			activeGroup.m_vTargetPosition = targetPosition;
			activeGroup.m_vPosition = sourcePosition;
			activeGroup.m_sRuntimeStatus = "routing";
			activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
			state.m_aActiveGroups.Insert(activeGroup);
			if (!TrySpawnActiveGroup(activeGroup, state))
			{
				state.m_aActiveGroups.Remove(state.m_aActiveGroups.Count() - 1);
				qrf.m_bResolved = true;
				qrf.m_bSucceeded = false;
				m_bMarkerRefreshNeeded = true;
				NotifyRuntimeEvent(state, "qrf_spawn_failed_" + qrf.m_sInstanceId, "QRF Spawn Failed", string.Format("%1 QRF could not spawn for %2. %3", qrf.m_sFactionKey, ResolveZoneDisplayName(state, targetZone.m_sZoneId), activeGroup.m_sSpawnFailureReason), targetZone.m_sZoneId, activeGroup.m_vPosition, 6.0);
				Print(string.Format("h-istasi | QRF %1 failed to materialize near %2 | prefab %3 | reason %4", qrf.m_sInstanceId, targetZone.m_sZoneId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
				changed = true;
				continue;
			}

			qrf.m_sGroupId = activeGroup.m_sGroupId;
			Print(string.Format("h-istasi | QRF %1 spawned near %2 | source %3 | spawn %4 | objective %5 | group %6", qrf.m_sInstanceId, targetZone.m_sZoneId, qrf.m_sSourceZoneId, activeGroup.m_vPosition, targetPosition, activeGroup.m_sGroupId));
			changed = true;
		}

		return changed;
	}

	protected bool ResolveArrivedQRFs(HST_CampaignState state)
	{
		bool changed;
		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (!qrf || qrf.m_bResolved)
				continue;

			if (state.m_iElapsedSeconds < qrf.m_iStartedAtSecond + qrf.m_iETASeconds)
				continue;

			if (qrf.m_sGroupId.IsEmpty())
			{
				qrf.m_bResolved = true;
				qrf.m_bSucceeded = false;
				m_bMarkerRefreshNeeded = true;
				changed = true;
				Print(string.Format("h-istasi | QRF %1 failed to reach zone %2 | no physical group materialized", qrf.m_sInstanceId, qrf.m_sTargetZoneId), LogLevel.WARNING);
				continue;
			}

			HST_ActiveGroupState activeGroup = state.FindActiveGroup(qrf.m_sGroupId);
			if (activeGroup && state.m_iElapsedSeconds < activeGroup.m_iSpawnedAtSecond + ROUTE_STATE_UPDATE_SECONDS)
				continue;

			qrf.m_bResolved = true;
			if (!IsActiveGroupCombatEffective(activeGroup))
			{
				qrf.m_bSucceeded = false;
				m_bMarkerRefreshNeeded = true;
				changed = true;
				Print(string.Format("h-istasi | QRF %1 failed to reach zone %2 | group %3 status %4 spawned agents %5 alive %6", qrf.m_sInstanceId, qrf.m_sTargetZoneId, qrf.m_sGroupId, ResolveActiveGroupStatus(activeGroup), ResolveSpawnedAgentTotal(activeGroup), ResolveAliveAgentTotal(activeGroup)), LogLevel.WARNING);
				continue;
			}

			qrf.m_bSucceeded = true;
			if (activeGroup)
				activeGroup.m_sRuntimeStatus = "arrived";
			changed = true;
			Print(string.Format("h-istasi | QRF %1 active near zone %2", qrf.m_sInstanceId, qrf.m_sTargetZoneId));
		}

		return changed;
	}

	protected string ResolveQRFSourceZoneId(HST_CampaignState state, HST_ZoneState targetZone, string factionKey)
	{
		if (!state || !targetZone || factionKey.IsEmpty())
			return "";

		HST_ZoneState bestZone;
		float bestDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sZoneId == targetZone.m_sZoneId || zone.m_sOwnerFactionKey != factionKey)
				continue;

			float distanceSq = DistanceSq2D(zone.m_vPosition, targetZone.m_vPosition);
			if (distanceSq < bestDistanceSq)
			{
				bestDistanceSq = distanceSq;
				bestZone = zone;
			}
		}

		if (bestZone)
			return bestZone.m_sZoneId;

		return targetZone.m_sZoneId;
	}

	protected vector ResolveQRFSourcePosition(HST_CampaignState state, HST_QRFState qrf, HST_ZoneState targetZone)
	{
		if (state && qrf && !qrf.m_sSourceZoneId.IsEmpty())
		{
			HST_ZoneState sourceZone = state.FindZone(qrf.m_sSourceZoneId);
			if (sourceZone)
				return sourceZone.m_vPosition;
		}

		if (targetZone)
			return targetZone.m_vPosition;

		return "0 0 0";
	}

	protected vector ResolveQRFStagingPosition(HST_CampaignState state, HST_QRFState qrf, HST_ZoneState targetZone, vector targetPosition)
	{
		vector sourcePosition = ResolveQRFSourcePosition(state, qrf, targetZone);
		float standoff = ResolveQRFStagingDistanceMeters(targetZone);
		int seed = BuildQRFInboundSeed(state, qrf, targetZone);
		for (int attempt = 0; attempt < 32; attempt++)
		{
			vector candidate = BuildQRFApproachCandidate(targetPosition, sourcePosition, seed, attempt, standoff);
			vector resolved;
			if (!HST_WorldPositionService.TryResolveDryStagingPosition(candidate, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, resolved, 3.0))
				continue;
			if (IsInsideHQSafeRadius(state, resolved))
				continue;

			return resolved;
		}

		vector fallback;
		if (HST_WorldPositionService.TryResolveDryStagingPosition(sourcePosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, fallback, 3.0) && !IsInsideHQSafeRadius(state, fallback))
			return fallback;

		if (HST_WorldPositionService.TryResolveDryStagingPosition(targetPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, fallback, 3.0) && !IsInsideHQSafeRadius(state, fallback))
			return fallback;

		Print(string.Format("h-istasi | QRF %1 could not find dry staging near %2, using objective fallback %3", qrf.m_sInstanceId, targetZone.m_sZoneId, targetPosition), LogLevel.WARNING);
		return HST_WorldPositionService.ResolveSafeGroundPosition(targetPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 8.0);
	}

	protected float ResolveQRFStagingDistanceMeters(HST_ZoneState targetZone)
	{
		return Math.Max(QRF_MIN_STANDOFF_METERS, ResolveQRFStandoffMeters(targetZone) * 0.5);
	}

	protected float ResolveQRFStandoffMeters(HST_ZoneState targetZone)
	{
		float radius;
		if (targetZone)
			radius = targetZone.m_iCaptureRadiusMeters;
		if (radius <= 0 && targetZone)
			radius = targetZone.m_iActivationRadiusMeters * 0.35;
		if (radius <= 0)
			radius = QRF_MIN_STANDOFF_METERS;

		return Math.Min(QRF_MAX_STANDOFF_METERS, Math.Max(QRF_MIN_STANDOFF_METERS, radius + QRF_EXTRA_STANDOFF_METERS));
	}

	protected int BuildQRFInboundSeed(HST_CampaignState state, HST_QRFState qrf, HST_ZoneState targetZone)
	{
		int seed = 3371;
		if (state)
			seed += state.m_iCampaignSeed * 17 + state.m_iElapsedSeconds * 5 + state.m_aActiveGroups.Count() * 61;
		if (qrf)
			seed += qrf.m_sInstanceId.Length() * 43 + qrf.m_sFactionKey.Length() * 19 + qrf.m_sSourceZoneId.Length() * 29;
		if (targetZone)
			seed += targetZone.m_iPriority * 37 + targetZone.m_sZoneId.Length() * 83 + Math.Round(targetZone.m_vPosition[0]) + Math.Round(targetZone.m_vPosition[2]);
		if (seed < 0)
			seed = -seed;

		return seed;
	}

	protected vector BuildQRFApproachCandidate(vector target, vector source, int seed, int attempt, float standoffMeters)
	{
		vector candidate = target;
		float dx = target[0] - source[0];
		float dz = target[2] - source[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		if (length > 1.0 && attempt < 2)
		{
			float distance = standoffMeters + attempt * 45.0;
			candidate[0] = target[0] - dx / length * distance;
			candidate[2] = target[2] - dz / length * distance;
			return candidate;
		}

		int slot = HST_DefaultCatalog.PositiveMod(seed + attempt, 8);
		float distanceBySlot = standoffMeters + (attempt / 8) * 55.0;
		float x = 1.0;
		float z = 0.0;
		if (slot == 1)
		{
			x = 0.707;
			z = 0.707;
		}
		else if (slot == 2)
		{
			x = 0.0;
			z = 1.0;
		}
		else if (slot == 3)
		{
			x = -0.707;
			z = 0.707;
		}
		else if (slot == 4)
		{
			x = -1.0;
			z = 0.0;
		}
		else if (slot == 5)
		{
			x = -0.707;
			z = -0.707;
		}
		else if (slot == 6)
		{
			x = 0.0;
			z = -1.0;
		}
		else if (slot == 7)
		{
			x = 0.707;
			z = -0.707;
		}

		candidate[0] = target[0] + x * distanceBySlot;
		candidate[2] = target[2] + z * distanceBySlot;
		return candidate;
	}

	protected bool IsActiveGroupCombatEffective(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		if (activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "folded" || activeGroup.m_sRuntimeStatus == "spawn_failed")
			return false;

		if (activeGroup.m_iSpawnedAgentCount <= 0)
			return false;

		if (activeGroup.m_iLastSeenAliveCount > 0)
			return true;

		return activeGroup.m_iSurvivorInfantryCount > 0 || activeGroup.m_iSurvivorVehicleCount > 0;
	}

	protected string ResolveActiveGroupStatus(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return "missing";

		return activeGroup.m_sRuntimeStatus;
	}

	protected int ResolveSpawnedAgentTotal(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return 0;

		return activeGroup.m_iSpawnedAgentCount;
	}

	protected int ResolveAliveAgentTotal(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return 0;

		return activeGroup.m_iLastSeenAliveCount;
	}

	protected bool UpdateActiveGroupRoutes(HST_CampaignState state)
	{
		if (!state || state.m_iElapsedSeconds % ROUTE_STATE_UPDATE_SECONDS != 0)
			return false;

		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || !activeGroup.m_bSpawnedEntity)
				continue;
			if (activeGroup.m_sRuntimeStatus != "routing" && activeGroup.m_sRuntimeStatus != "support_active")
				continue;

			int duration = ResolveRouteDurationSeconds(state, activeGroup);
			int elapsed = state.m_iElapsedSeconds - activeGroup.m_iSpawnedAtSecond;
			if (elapsed < 0)
				elapsed = 0;

			float progress = Math.Min(1.0, elapsed * 1.0 / duration);
			vector position = LerpPosition(activeGroup.m_vSourcePosition, activeGroup.m_vTargetPosition, progress);
			position = HST_WorldPositionService.ResolveSafeGroundPosition(position, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
			string arrivedStatus = ResolveArrivedRouteStatus(activeGroup);
			if (DistanceSq2D(activeGroup.m_vPosition, position) < 25)
			{
				if (progress >= 1.0 && activeGroup.m_sRuntimeStatus != arrivedStatus)
				{
					activeGroup.m_sRuntimeStatus = arrivedStatus;
					changed = true;
				}
				continue;
			}

			activeGroup.m_vPosition = position;
			SetRuntimeGroupEntitiesOrigin(activeGroup.m_sGroupId, position);

			if (progress >= 1.0)
				activeGroup.m_sRuntimeStatus = arrivedStatus;

			changed = true;
		}

		return changed;
	}

	protected int ResolveRouteDurationSeconds(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return QRF_ETA_SECONDS;

		if (state && activeGroup.m_sRuntimeStatus == "support_active")
		{
			HST_SupportRequestState request = FindSupportRequestByGroupId(state, activeGroup.m_sGroupId);
			if (request)
				return Math.Max(5, request.m_iRequestedAtSecond + request.m_iETASeconds - activeGroup.m_iSpawnedAtSecond);
		}

		if (activeGroup.m_bQRF)
		{
			HST_QRFState qrf = FindQRFByGroupId(state, activeGroup.m_sGroupId);
			if (qrf)
				return Math.Max(5, qrf.m_iStartedAtSecond + qrf.m_iETASeconds - activeGroup.m_iSpawnedAtSecond);

			return QRF_ETA_SECONDS;
		}

		float distance = Math.Sqrt(DistanceSq2D(activeGroup.m_vSourcePosition, activeGroup.m_vTargetPosition));
		return Math.Max(120, Math.Round(distance / 6.0));
	}

	protected string ResolveArrivedRouteStatus(HST_ActiveGroupState activeGroup)
	{
		if (activeGroup && activeGroup.m_sRuntimeStatus == "support_active")
			return "support_arrived";

		return "arrived";
	}

	protected HST_QRFState FindQRFByGroupId(HST_CampaignState state, string groupId)
	{
		if (!state || groupId.IsEmpty())
			return null;

		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (qrf && qrf.m_sGroupId == groupId)
				return qrf;
		}

		return null;
	}

	protected HST_SupportRequestState FindSupportRequestByGroupId(HST_CampaignState state, string groupId)
	{
		if (!state || groupId.IsEmpty())
			return null;

		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (request && request.m_sGroupId == groupId)
				return request;
		}

		return null;
	}

	protected vector LerpPosition(vector sourcePosition, vector targetPosition, float progress)
	{
		vector result;
		result[0] = sourcePosition[0] + (targetPosition[0] - sourcePosition[0]) * progress;
		result[1] = sourcePosition[1] + (targetPosition[1] - sourcePosition[1]) * progress;
		result[2] = sourcePosition[2] + (targetPosition[2] - sourcePosition[2]) * progress;
		return result;
	}

	protected HST_ActiveGroupState CreateActiveGroup(HST_CampaignState state, HST_ZoneState zone, string factionKey, int infantryCount, int vehicleCount, bool qrf)
	{
		HST_ActiveGroupState activeGroup = new HST_ActiveGroupState();
		activeGroup.m_sGroupId = BuildGroupId(state, zone, factionKey, qrf);
		activeGroup.m_sZoneId = zone.m_sZoneId;
		activeGroup.m_sFactionKey = factionKey;
		activeGroup.m_sPrefab = SelectGroupPrefab(state, zone, factionKey, qrf);
		activeGroup.m_sRouteId = ResolveGroupRouteId(zone, qrf);
		activeGroup.m_vSourcePosition = ResolveGroupSourcePosition(state, zone, activeGroup.m_sRouteId, qrf);
		activeGroup.m_vTargetPosition = ResolveGroupTargetPosition(state, zone, activeGroup.m_sRouteId, qrf);
		activeGroup.m_vPosition = HST_WorldPositionService.ResolveSafeGroundPosition(activeGroup.m_vSourcePosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
		activeGroup.m_sRuntimeStatus = "queued";
		activeGroup.m_iInfantryCount = infantryCount;
		activeGroup.m_iVehicleCount = vehicleCount;
		activeGroup.m_iLastSeenAliveCount = infantryCount + vehicleCount;
		activeGroup.m_iSurvivorInfantryCount = infantryCount;
		activeGroup.m_iSurvivorVehicleCount = vehicleCount;
		activeGroup.m_bQRF = qrf;
		return activeGroup;
	}

	protected string BuildGroupId(HST_CampaignState state, HST_ZoneState zone, string factionKey, bool qrf)
	{
		string prefix = "grp";
		if (qrf)
			prefix = "qrf";

		return string.Format("%1_%2_%3_%4_%5", prefix, zone.m_sZoneId, factionKey, state.m_iElapsedSeconds, state.m_aActiveGroups.Count());
	}

	protected string SelectGroupPrefab(HST_CampaignState state, HST_ZoneState zone, string factionKey, bool qrf)
	{
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (!faction)
			return "";

		int seed = BuildGroupSelectionSeed(state, zone, qrf);
		array<string> candidates = {};
		if (qrf)
		{
			AppendUniqueGroupPrefabs(candidates, faction.m_aQRFGroupPrefabs);
			AppendUniqueGroupPrefabs(candidates, faction.m_aGroupPrefabs);
			return SelectValidGroupPrefabFromList(candidates, seed, factionKey, "qrf");
		}

		AppendUniqueGroupPrefabs(candidates, faction.m_aGroupPrefabs);
		AppendUniqueGroupPrefabs(candidates, faction.m_aPatrolGroupPrefabs);
		return SelectValidGroupPrefabFromList(candidates, seed, factionKey, "garrison");
	}

	protected string SelectVehiclePrefab(HST_CampaignState state, HST_ZoneState zone, string factionKey, int index)
	{
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (!faction || faction.m_aVehiclePrefabs.Count() == 0)
			return "";

		int seed = BuildGroupSelectionSeed(state, zone, false) + index * 47;
		int startIndex = HST_DefaultCatalog.PositiveMod(seed, faction.m_aVehiclePrefabs.Count());
		for (int offset = 0; offset < faction.m_aVehiclePrefabs.Count(); offset++)
		{
			int candidateIndex = HST_DefaultCatalog.PositiveMod(startIndex + offset, faction.m_aVehiclePrefabs.Count());
			string prefab = faction.m_aVehiclePrefabs[candidateIndex];
			if (IsValidVehiclePrefabResource(prefab, factionKey))
				return prefab;
		}

		Print(string.Format("h-istasi | no valid active vehicle prefab found for faction %1", factionKey), LogLevel.WARNING);
		return "";
	}

	protected string SelectMissionConvoyVehiclePrefab(HST_CampaignState state, HST_ZoneState zone, string factionKey, HST_ActiveMissionState mission, int index)
	{
		array<string> candidates = {};
		BuildConvoyVehicleCandidates(candidates, factionKey, mission);
		array<string> usableCandidates = {};
		BuildUsableConvoyVehicleCandidates(usableCandidates, candidates, factionKey);
		if (usableCandidates.Count() == 0)
			return "";

		int seed = BuildGroupSelectionSeed(state, zone, false) + index * 53 + ResolveMissionVehicleSelectionSalt(mission);
		return usableCandidates[HST_DefaultCatalog.PositiveMod(seed, usableCandidates.Count())];
	}

	protected void BuildConvoyVehicleCandidates(array<string> candidates, string factionKey, HST_ActiveMissionState mission)
	{
		if (!candidates)
			return;

		AppendRuntimeFactionCatalogVehiclePrefabs(candidates, factionKey);

		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (faction)
			AppendUniqueVehiclePrefabs(candidates, faction.m_aVehiclePrefabs);
		AppendFallbackConvoyVehiclePrefabs(candidates, factionKey, mission);
	}

	protected string BuildGroundVehicleCandidateFactionReport(string factionKey)
	{
		array<string> candidates = {};
		BuildConvoyVehicleCandidates(candidates, factionKey, null);

		array<string> usableCandidates = {};
		BuildUsableConvoyVehicleCandidates(usableCandidates, candidates, factionKey);
		string usableList = "";
		foreach (string prefab : usableCandidates)
		{
			if (usableList.IsEmpty())
				usableList = prefab;
			else
				usableList = usableList + ", " + prefab;
		}

		if (usableList.IsEmpty())
			usableList = "none";

		int catalogCandidates = CountRuntimeFactionCatalogVehicleCandidates(factionKey);
		string catalogSource = BuildFactionCampaignCatalogSource(factionKey);
		if (catalogCandidates <= 0)
			catalogSource = catalogSource + " unavailable; local GUID fallback active";
		string report = string.Format("\n  faction %1 convoy ground vehicle candidates | usable %2/%3", factionKey, usableCandidates.Count(), candidates.Count());
		report = report + string.Format(" | catalog source %1 | catalog candidates %2", catalogSource, catalogCandidates);
		report = report + string.Format(" | unverified path-only %1", CountUnverifiedVehicleCandidates(candidates));
		report = report + " | usable prefabs " + usableList;
		return report;
	}

	protected void BuildUsableConvoyVehicleCandidates(array<string> usableCandidates, array<string> candidates, string factionKey)
	{
		if (!usableCandidates || !candidates)
			return;

		foreach (string prefab : candidates)
		{
			if (!IsValidVehiclePrefabResource(prefab, factionKey, false))
				continue;
			if (usableCandidates.Contains(prefab))
				continue;

			usableCandidates.Insert(prefab);
		}
	}

	protected int AppendRuntimeFactionCatalogVehiclePrefabs(array<string> candidates, string factionKey)
	{
		if (!candidates || factionKey.IsEmpty())
			return 0;

		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalogManager)
			return 0;

		array<SCR_EntityCatalog> catalogs = {};
		FactionKey key = factionKey;
		if (catalogManager.GetAllFactionEntityCatalogs(catalogs, key) <= 0)
			return 0;

		int before = candidates.Count();
		foreach (SCR_EntityCatalog catalog : catalogs)
		{
			if (!catalog)
				continue;

			array<SCR_EntityCatalogEntry> entries = {};
			catalog.GetEntityList(entries);
			foreach (SCR_EntityCatalogEntry entry : entries)
			{
				if (!entry)
					continue;

				string prefab = entry.GetPrefab();
				if (!IsGroundVehicleResource(prefab))
					continue;

				AppendUniqueVehiclePrefab(candidates, prefab);
			}
		}

		return candidates.Count() - before;
	}

	protected int CountRuntimeFactionCatalogVehicleCandidates(string factionKey)
	{
		array<string> candidates = {};
		return AppendRuntimeFactionCatalogVehiclePrefabs(candidates, factionKey);
	}

	protected string BuildFactionCampaignCatalogSource(string factionKey)
	{
		string source = "runtime SCR_EntityCatalog";
		if (factionKey == "US")
			source = source + " via " + US_CAMPAIGN_FACTION_CONFIG;
		else if (factionKey == "USSR")
			source = source + " via " + USSR_CAMPAIGN_FACTION_CONFIG;
		else if (factionKey == "FIA")
			source = source + " via " + FIA_CAMPAIGN_FACTION_CONFIG;

		return source;
	}

	protected void AppendFallbackConvoyVehiclePrefabs(array<string> candidates, string factionKey, HST_ActiveMissionState mission)
	{
		if (!candidates)
			return;

		if (factionKey == "US")
		{
			AppendUniqueVehiclePrefab(candidates, "{4A71F755A4513227}Prefabs/Vehicles/Wheeled/M998/M1025.et");
			AppendUniqueVehiclePrefab(candidates, "{B55C6990A6A9411B}Prefabs/Vehicles/Wheeled/M998/M998_covered.et");
			AppendUniqueVehiclePrefab(candidates, "{5674FAEB9AB7BDD0}Prefabs/Vehicles/Wheeled/M998/M998_uncovered.et");
			AppendUniqueVehiclePrefab(candidates, "{F1FBD0972FA5FE09}Prefabs/Vehicles/Wheeled/M923A1/M923A1_transport.et");
			AppendUniqueVehiclePrefab(candidates, "{81FDAD5EB644CC3D}Prefabs/Vehicles/Wheeled/M923A1/M923A1_transport_covered.et");
			return;
		}

		if (factionKey == "USSR")
		{
			AppendUniqueVehiclePrefab(candidates, "{0B4DEA8078B78A9B}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469_PKM.et");
			AppendUniqueVehiclePrefab(candidates, "{4597626AF36C0858}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320.et");
			AppendUniqueVehiclePrefab(candidates, "{16C1F16C9B053801}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_transport.et");
			AppendUniqueVehiclePrefab(candidates, "{C012BB3488BEA0C2}Prefabs/Vehicles/Wheeled/BTR70/BTR70.et");
			return;
		}

		if (factionKey == "FIA")
		{
			AppendUniqueVehiclePrefab(candidates, "{16C1F16C9B053801}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_transport.et");
			AppendUniqueVehiclePrefab(candidates, "{B47110AA1A806556}Prefabs/Vehicles/Wheeled/BTR70/BTR70_FIA.et");
			AppendUniqueVehiclePrefab(candidates, "{0B4DEA8078B78A9B}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469_PKM.et");
		}
	}

	protected int ResolveMissionVehicleSelectionSalt(HST_ActiveMissionState mission)
	{
		if (!mission)
			return 0;

		int salt = mission.m_sInstanceId.Length() * 89 + mission.m_sMissionId.Length() * 131;
		if (mission.m_sMissionId == "convoy_armored")
			salt += 7001;
		if (mission.m_sMissionId == "convoy_reinforcements")
			salt += 3001;
		return salt;
	}

	protected string SelectValidVehiclePrefabFromList(array<string> prefabs, int seed, string factionKey, string purpose)
	{
		if (!prefabs || prefabs.Count() == 0)
			return "";

		int startIndex = HST_DefaultCatalog.PositiveMod(seed, prefabs.Count());
		for (int offset = 0; offset < prefabs.Count(); offset++)
		{
			int index = HST_DefaultCatalog.PositiveMod(startIndex + offset, prefabs.Count());
			string prefab = prefabs[index];
			if (!IsValidVehiclePrefabResource(prefab, factionKey, false))
				continue;

			return prefab;
		}

		Print(string.Format("h-istasi | no valid %1 prefab found for faction %2", purpose, factionKey), LogLevel.WARNING);
		return "";
	}

	protected void AppendUniqueGroupPrefabs(array<string> candidates, array<string> source)
	{
		if (!candidates || !source)
			return;

		foreach (string prefab : source)
		{
			if (prefab.IsEmpty() || candidates.Contains(prefab))
				continue;

			candidates.Insert(prefab);
		}
	}

	protected void AppendUniqueVehiclePrefabs(array<string> candidates, array<string> source)
	{
		if (!candidates || !source)
			return;

		foreach (string prefab : source)
		{
			AppendUniqueVehiclePrefab(candidates, prefab);
		}
	}

	protected void AppendUniqueVehiclePrefab(array<string> candidates, string prefab)
	{
		if (!candidates || prefab.IsEmpty() || candidates.Contains(prefab))
			return;
		if (IsAircraftVehicleResource(prefab))
			return;
		if (!IsGroundVehicleResource(prefab))
			return;

		candidates.Insert(prefab);
	}

	protected int CountUnverifiedVehicleCandidates(array<string> candidates)
	{
		if (!candidates)
			return 0;

		int count;
		foreach (string prefab : candidates)
		{
			if (!IsGuidQualifiedVehicleResource(prefab))
				count++;
		}

		return count;
	}

	protected int BuildGroupSelectionSeed(HST_CampaignState state, HST_ZoneState zone, bool qrf)
	{
		int seed = 271;
		if (state)
			seed += state.m_iCampaignSeed * 19 + state.m_iElapsedSeconds * 3 + state.m_aActiveGroups.Count() * 97;
		if (zone)
			seed += zone.m_iPriority * 41 + zone.m_sZoneId.Length() * 113 + Math.Round(zone.m_vPosition[0]) + Math.Round(zone.m_vPosition[2]);
		if (qrf)
			seed += 7001;
		if (seed < 0)
			seed = -seed;

		return seed;
	}

	protected bool TrySpawnActiveGroup(HST_ActiveGroupState activeGroup, HST_CampaignState state = null)
	{
		if (!activeGroup || HasRuntimeGroupEntity(activeGroup.m_sGroupId))
			return false;

		if (activeGroup.m_bSpawnAttempted && !activeGroup.m_bSpawnedEntity)
			return false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
		{
			activeGroup.m_sSpawnFailureReason = "Respawn system is unavailable.";
			return false;
		}

		activeGroup.m_bSpawnAttempted = true;
		string requestedStatus = activeGroup.m_sRuntimeStatus;
		activeGroup.m_sRuntimeStatus = "spawning";
		activeGroup.m_sSpawnFallbackMode = "group";
		activeGroup.m_sSpawnFailureReason = "";
		activeGroup.m_iSpawnedAgentCount = 0;

		vector spawnPosition = HST_WorldPositionService.ResolveSafeGroundPosition(activeGroup.m_vPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
		activeGroup.m_vPosition = spawnPosition;

		GenericEntity entity;
		int agentCount = 0;
		string failureReason = "";
		if (activeGroup.m_sPrefab.IsEmpty())
		{
			failureReason = string.Format("No group prefab configured for faction %1.", activeGroup.m_sFactionKey);
		}
		else if (IsValidGroupPrefabResource(activeGroup.m_sPrefab, activeGroup.m_sFactionKey))
		{
			entity = HST_WorldPositionService.SpawnPrefab(activeGroup.m_sPrefab, spawnPosition, "0 0 0");
			if (!entity)
				failureReason = string.Format("Group prefab did not spawn: %1.", activeGroup.m_sPrefab);
			else
				agentCount = ResolveSpawnedAgentCount(entity, activeGroup);
		}
		else
		{
			failureReason = string.Format("Invalid group prefab: %1.", activeGroup.m_sPrefab);
		}

		if (!entity)
		{
			activeGroup.m_sSpawnFailureReason = failureReason;
			if (activeGroup.m_sSpawnFailureReason.IsEmpty())
				activeGroup.m_sSpawnFailureReason = string.Format("Group prefab spawn failed for faction %1.", activeGroup.m_sFactionKey);
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			Print(string.Format("h-istasi | active group prefab spawn failed for %1 (%2): %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
			return false;
		}

		activeGroup.m_bSpawnedEntity = true;
		activeGroup.m_sRuntimeEntityId = activeGroup.m_sGroupId;
		activeGroup.m_sRuntimeStatus = ResolveSpawnedRuntimeStatus(activeGroup, requestedStatus);
		activeGroup.m_sSpawnFailureReason = "";
		activeGroup.m_iSpawnedAgentCount = agentCount;
		activeGroup.m_iLastSeenAliveCount = Math.Max(0, agentCount);
		activeGroup.m_iSurvivorInfantryCount = activeGroup.m_iInfantryCount;
		if (state)
			activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		m_aRuntimeGroupIds.Insert(activeGroup.m_sGroupId);
		m_aRuntimeGroupEntities.Insert(entity);
		Print(string.Format("h-istasi | spawned active group %1 using %2 (%3 agents)", activeGroup.m_sGroupId, activeGroup.m_sSpawnFallbackMode, agentCount));
		return true;
	}

	protected bool TrySpawnActiveVehicle(HST_ActiveGroupState activeGroup, HST_CampaignState state = null)
	{
		if (!activeGroup || HasRuntimeGroupEntity(activeGroup.m_sGroupId))
			return false;

		if (activeGroup.m_bSpawnAttempted && !activeGroup.m_bSpawnedEntity)
			return false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
		{
			activeGroup.m_sSpawnFailureReason = "Respawn system is unavailable.";
			return false;
		}

		activeGroup.m_bSpawnAttempted = true;
		string requestedStatus = activeGroup.m_sRuntimeStatus;
		activeGroup.m_sRuntimeStatus = "spawning";
		activeGroup.m_sSpawnFallbackMode = "vehicle";
		activeGroup.m_sSpawnFailureReason = "";

		vector spawnPosition;
		if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(activeGroup.m_vPosition, spawnPosition, true))
		{
			activeGroup.m_sSpawnFailureReason = "No dry vehicle-safe ground at assigned slot.";
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			Print(string.Format("h-istasi | active vehicle spawn failed for %1 (%2): %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
			return false;
		}

		activeGroup.m_vPosition = spawnPosition;
		string failureReason = "";
		GenericEntity entity;
		if (activeGroup.m_sPrefab.IsEmpty())
		{
			failureReason = string.Format("No vehicle prefab configured for faction %1.", activeGroup.m_sFactionKey);
		}
		else if (IsValidVehiclePrefabResource(activeGroup.m_sPrefab, activeGroup.m_sFactionKey))
		{
			vector spawnAngles = HST_WorldPositionService.BuildUprightAngles(0);
			entity = HST_WorldPositionService.SpawnPrefab(activeGroup.m_sPrefab, spawnPosition, spawnAngles);
			if (!entity)
				failureReason = string.Format("Vehicle prefab did not spawn: %1.", activeGroup.m_sPrefab);
			else
				HST_WorldPositionService.ApplyUprightEntityTransform(entity, spawnPosition, spawnAngles);
		}
		else
		{
			failureReason = string.Format("Invalid vehicle prefab: %1.", activeGroup.m_sPrefab);
		}

		if (!entity)
		{
			activeGroup.m_sSpawnFailureReason = failureReason;
			if (activeGroup.m_sSpawnFailureReason.IsEmpty())
				activeGroup.m_sSpawnFailureReason = string.Format("Vehicle prefab spawn failed for faction %1.", activeGroup.m_sFactionKey);
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			Print(string.Format("h-istasi | active vehicle prefab spawn failed for %1 (%2): %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
			return false;
		}

		activeGroup.m_bSpawnedEntity = true;
		activeGroup.m_sRuntimeEntityId = activeGroup.m_sGroupId;
		activeGroup.m_sRuntimeStatus = ResolveSpawnedRuntimeStatus(activeGroup, requestedStatus);
		activeGroup.m_sSpawnFailureReason = "";
		activeGroup.m_iSpawnedAgentCount = 1;
		activeGroup.m_iLastSeenAliveCount = 1;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_iSurvivorVehicleCount = Math.Max(1, activeGroup.m_iVehicleCount);
		if (state)
			activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		m_aRuntimeVehicleGroupIds.Insert(activeGroup.m_sGroupId);
		m_aRuntimeVehicleEntities.Insert(entity);
		Print(string.Format("h-istasi | spawned active vehicle %1 using %2 at %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, spawnPosition));
		return true;
	}

	protected string ResolveSpawnedRuntimeStatus(HST_ActiveGroupState activeGroup, string requestedStatus)
	{
		if (!requestedStatus.IsEmpty() && requestedStatus != "queued" && requestedStatus != "spawning")
			return requestedStatus;

		if (activeGroup && activeGroup.m_bQRF)
			return "staged";

		return "guard_center";
	}

	protected string SelectValidGroupPrefabFromList(array<string> prefabs, int seed, string factionKey, string purpose)
	{
		if (!prefabs || prefabs.Count() == 0)
			return "";

		int startIndex = HST_DefaultCatalog.PositiveMod(seed, prefabs.Count());
		for (int offset = 0; offset < prefabs.Count(); offset++)
		{
			int index = HST_DefaultCatalog.PositiveMod(startIndex + offset, prefabs.Count());
			string prefab = prefabs[index];
			if (!IsValidGroupPrefabResource(prefab, factionKey))
				continue;

			return prefab;
		}

		Print(string.Format("h-istasi | no valid %1 group prefab found for faction %2", purpose, factionKey), LogLevel.WARNING);
		return "";
	}

	protected bool IsValidGroupPrefabResource(string prefab, string factionKey)
	{
		if (prefab.IsEmpty())
			return false;

		if (prefab.Contains("_NotSpawned") || prefab.Contains("NotSpawned"))
		{
			Print(string.Format("h-istasi | rejected non-spawning group prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (prefab.Contains("PlayableGroup.et"))
		{
			Print(string.Format("h-istasi | rejected player placeholder group prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
		{
			Print(string.Format("h-istasi | rejected missing group prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		return true;
	}

	protected bool IsValidVehiclePrefabResource(string prefab, string factionKey, bool logRejection = true)
	{
		if (prefab.IsEmpty())
			return false;

		if (IsAircraftVehicleResource(prefab))
		{
			if (logRejection)
				Print(string.Format("h-istasi | rejected aircraft vehicle prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (!IsGroundVehicleResource(prefab))
		{
			if (logRejection)
				Print(string.Format("h-istasi | rejected non-ground vehicle prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (!HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
		{
			if (logRejection)
				Print(string.Format("h-istasi | rejected invalid vehicle root prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (!IsGuidQualifiedVehicleResource(prefab))
		{
			if (logRejection)
				Print(string.Format("h-istasi | rejected unverified path-only vehicle prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
		{
			if (logRejection)
				Print(string.Format("h-istasi | rejected missing vehicle prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		return true;
	}

	protected bool IsGuidQualifiedVehicleResource(string prefab)
	{
		return !prefab.IsEmpty() && prefab.Contains("{") && prefab.Contains("}") && prefab.Contains("Prefabs/Vehicles/");
	}

	protected bool IsGroundVehicleResource(string prefab)
	{
		return prefab.Contains("Prefabs/Vehicles/Wheeled/") || prefab.Contains("Prefabs/Vehicles/Tracked/");
	}

	protected bool IsAircraftVehicleResource(string prefab)
	{
		return prefab.Contains("Aircraft") || prefab.Contains("Airplane") || prefab.Contains("Plane") || prefab.Contains("Helicopter") || prefab.Contains("Helicopters") || prefab.Contains("/UH") || prefab.Contains("/AH") || prefab.Contains("/Mi") || prefab.Contains("/KA") || prefab.Contains("/Ka");
	}

	protected int ResolveSpawnedAgentCount(GenericEntity entity, HST_ActiveGroupState activeGroup)
	{
		if (!entity || !activeGroup)
			return 0;

		AIGroup group = AIGroup.Cast(entity);
		if (!group)
		{
			Print(string.Format("h-istasi | spawned prefab %1 for %2 did not create an AIGroup", activeGroup.m_sPrefab, activeGroup.m_sGroupId), LogLevel.WARNING);
			return 0;
		}

		int agentCount = group.GetAgentsCount();
		if (agentCount <= 0)
		{
			Print(string.Format("h-istasi | spawned AIGroup %1 for %2 has no agents yet; keeping spawned group entity", activeGroup.m_sPrefab, activeGroup.m_sGroupId));
			return 0;
		}

		return agentCount;
	}

	protected void NotifyRuntimeEvent(HST_CampaignState state, string eventId, string title, string message, string zoneId, vector position, float durationSeconds)
	{
		if (!ShouldBroadcastRuntimeEvent(state, zoneId, position))
			return;

		string payload = string.Format("HST_NOTIFICATION|%1|enemy|warning|%2|%3|%4||%5|%6", eventId, PayloadText(title), PayloadText(message), zoneId, position, durationSeconds);
		HST_CommandMenuRequestComponent.BroadcastNotification(payload, title + ": " + message);
	}

	protected bool ShouldBroadcastRuntimeEvent(HST_CampaignState state, string zoneId, vector position)
	{
		if (!state)
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (zone && zone.m_bActive)
			return true;

		if (IsZeroVector(position) && zone)
			position = zone.m_vPosition;

		return HST_WorldPositionService.IsPositionInsidePlayerEventBubble(position);
	}

	protected string ResolveZoneDisplayName(HST_CampaignState state, string zoneId)
	{
		if (!state || zoneId.IsEmpty())
			return "unknown location";

		HST_ZoneState zone = state.FindZone(zoneId);
		if (zone)
		{
			if (!zone.m_sDisplayName.IsEmpty())
				return zone.m_sDisplayName;
			return HST_DefaultCatalog.GetZoneDisplayName(zone.m_sZoneId);
		}

		string label = HST_DefaultCatalog.GetZoneDisplayName(zoneId);
		if (!label.IsEmpty() && label != zoneId)
			return label;

		label = zoneId;
		label.Replace("_", " ");
		return label;
	}

	protected string PayloadText(string value)
	{
		if (value.IsEmpty())
			return "";

		string result = value;
		result.Replace("|", "/");
		result.Replace("\n", " ");
		return result;
	}

	protected int ResolveActiveInfantryCap(HST_ZoneState zone)
	{
		if (!zone)
			return MAX_ACTIVE_INFANTRY_PER_ZONE;

		int cap = MAX_ACTIVE_INFANTRY_PER_ZONE + Math.Max(0, zone.m_iPriority / 12);
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			cap += 2;
		return Math.Min(12, cap);
	}

	protected int ResolveActiveVehicleCap(HST_ZoneState zone)
	{
		if (!zone)
			return MAX_ACTIVE_VEHICLES_PER_ZONE;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return 2;

		return MAX_ACTIVE_VEHICLES_PER_ZONE;
	}

	protected void EnsureRuntimeGroupEntities(HST_CampaignState state)
	{
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || HasRuntimeGroupEntity(activeGroup.m_sGroupId))
				continue;

			if (IsMissionConvoyGroup(activeGroup))
			{
				HST_ActiveMissionState mission = FindMissionForConvoyGroup(state, activeGroup);
				HST_MissionAssetState asset = FindMissionConvoyAssetForGroup(state, mission, activeGroup);
				if (mission && asset)
					TrySpawnMissionConvoyGroup(state, mission, asset, activeGroup, ResolveMissionConvoyGroupIndex(mission, activeGroup.m_sGroupId));
				continue;
			}

			if (activeGroup.m_iVehicleCount > 0 && activeGroup.m_iInfantryCount <= 0)
				TrySpawnActiveVehicle(activeGroup, state);
			else
				TrySpawnActiveGroup(activeGroup, state);
		}
	}

	protected bool UpdateRuntimeGroupSurvivors(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || !activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "folded" || activeGroup.m_sRuntimeStatus == "spawn_failed")
				continue;

			int aliveCount;
			if (IsMissionConvoyGroup(activeGroup))
				aliveCount = CountAliveRuntimeCrewAgents(activeGroup);
			else
				aliveCount = CountAliveRuntimeGroupAgents(activeGroup.m_sGroupId);
			if (aliveCount <= 0 && activeGroup.m_iSpawnedAgentCount <= 0)
				continue;
			if (aliveCount > 0 && activeGroup.m_iSpawnedAgentCount <= 0)
			{
				activeGroup.m_iSpawnedAgentCount = aliveCount;
				changed = true;
				Print(string.Format("h-istasi | active group populated %1 | zone %2 | live agents %3 | expected infantry %4 vehicles %5 | status %6", activeGroup.m_sGroupId, activeGroup.m_sZoneId, aliveCount, activeGroup.m_iInfantryCount, activeGroup.m_iVehicleCount, activeGroup.m_sRuntimeStatus));
			}
			if (aliveCount == activeGroup.m_iLastSeenAliveCount)
				continue;

			int previousAliveCount = activeGroup.m_iLastSeenAliveCount;
			activeGroup.m_iLastSeenAliveCount = aliveCount;
			if (IsMissionConvoyGroup(activeGroup))
			{
				activeGroup.m_iSurvivorInfantryCount = Math.Min(activeGroup.m_iInfantryCount, CountAliveRuntimeCrewAgents(activeGroup));
				activeGroup.m_iSurvivorVehicleCount = 1;
			}
			else if (activeGroup.m_iVehicleCount > 0 && activeGroup.m_iInfantryCount <= 0)
			{
				activeGroup.m_iSurvivorInfantryCount = 0;
				activeGroup.m_iSurvivorVehicleCount = Math.Min(activeGroup.m_iVehicleCount, aliveCount);
			}
			else
			{
				activeGroup.m_iSurvivorInfantryCount = Math.Min(activeGroup.m_iInfantryCount, aliveCount);
				activeGroup.m_iSurvivorVehicleCount = 0;
			}
			if (aliveCount <= 0)
			{
				activeGroup.m_sRuntimeStatus = "eliminated";
				if (activeGroup.m_bQRF)
					m_bMarkerRefreshNeeded = true;
			}
			Print(string.Format("h-istasi | active group survivors %1 | zone %2 | alive %3 from %4 | survivors infantry %5/%6 vehicles %7/%8 | status %9", activeGroup.m_sGroupId, activeGroup.m_sZoneId, aliveCount, previousAliveCount, activeGroup.m_iSurvivorInfantryCount, activeGroup.m_iInfantryCount, activeGroup.m_iSurvivorVehicleCount, activeGroup.m_iVehicleCount, activeGroup.m_sRuntimeStatus));
			changed = true;
		}

		if (changed)
		{
			foreach (HST_ZoneState zone : state.m_aZones)
			{
				if (zone && zone.m_bActive)
					ApplyActiveZoneCounts(state, zone);
			}
		}

		return changed;
	}

	protected int CountAliveRuntimeGroupAgents(string groupId)
	{
		int aliveCount;
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId)
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (!entity)
				continue;

			AIGroup group = AIGroup.Cast(entity);
			if (group)
			{
				aliveCount += Math.Max(0, group.GetAgentsCount());
				continue;
			}

			if (IsLivingEntity(entity))
				aliveCount++;
		}

		for (int j = 0; j < m_aRuntimeVehicleGroupIds.Count(); j++)
		{
			if (m_aRuntimeVehicleGroupIds[j] != groupId)
				continue;

			IEntity vehicle = m_aRuntimeVehicleEntities[j];
			if (IsLivingEntity(vehicle))
				aliveCount++;
		}

		return aliveCount;
	}

	protected bool HasActiveGarrisonGroup(HST_CampaignState state, HST_ZoneState zone)
	{
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_bQRF || IsMissionConvoyGroup(activeGroup) || activeGroup.m_sZoneId != zone.m_sZoneId || activeGroup.m_sFactionKey != zone.m_sOwnerFactionKey)
				continue;
			if (activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == "folded")
				continue;

			if (activeGroup.m_iLastSeenAliveCount > 0 || activeGroup.m_iSurvivorInfantryCount > 0 || activeGroup.m_iSurvivorVehicleCount > 0)
				return true;
		}

		return false;
	}

	protected void ApplyActiveZoneCounts(HST_CampaignState state, HST_ZoneState zone)
	{
		int infantryCount;
		int vehicleCount;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_bQRF || IsMissionConvoyGroup(activeGroup) || activeGroup.m_sZoneId != zone.m_sZoneId)
				continue;

			if (activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == "folded")
				continue;

			infantryCount += Math.Max(0, activeGroup.m_iSurvivorInfantryCount);
			vehicleCount += Math.Max(0, activeGroup.m_iSurvivorVehicleCount);
		}

		zone.m_iActiveInfantryCount = infantryCount;
		zone.m_iActiveVehicleCount = vehicleCount;
	}

	protected void FoldActiveGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		HST_GarrisonState garrison = state.FindGarrison(activeGroup.m_sZoneId, activeGroup.m_sFactionKey);
		if (!garrison)
		{
			garrison = new HST_GarrisonState();
			garrison.m_sZoneId = activeGroup.m_sZoneId;
			garrison.m_sFactionKey = activeGroup.m_sFactionKey;
			state.m_aGarrisons.Insert(garrison);
		}

		int survivorInfantry = activeGroup.m_iSurvivorInfantryCount;
		int survivorVehicles = activeGroup.m_iSurvivorVehicleCount;
		if (activeGroup.m_sRuntimeStatus != "eliminated" && !activeGroup.m_bSpawnedEntity && survivorInfantry <= 0 && activeGroup.m_iInfantryCount > 0)
			survivorInfantry = activeGroup.m_iInfantryCount;
		if (activeGroup.m_sRuntimeStatus != "eliminated" && !activeGroup.m_bSpawnedEntity && survivorVehicles <= 0 && activeGroup.m_iVehicleCount > 0)
			survivorVehicles = activeGroup.m_iVehicleCount;

		string previousStatus = activeGroup.m_sRuntimeStatus;
		activeGroup.m_sRuntimeStatus = "folded";
		if (activeGroup.m_bQRF)
			m_bMarkerRefreshNeeded = true;
		garrison.m_iInfantryCount += Math.Max(0, survivorInfantry);
		garrison.m_iVehicleCount += Math.Max(0, survivorVehicles);
		Print(string.Format("h-istasi | folded active group %1 | zone %2 | status %3 | returned infantry %4/%5 vehicles %6/%7 | last alive %8 | spawned agents %9", activeGroup.m_sGroupId, activeGroup.m_sZoneId, previousStatus, survivorInfantry, activeGroup.m_iInfantryCount, survivorVehicles, activeGroup.m_iVehicleCount, activeGroup.m_iLastSeenAliveCount, activeGroup.m_iSpawnedAgentCount));
	}

	protected string ResolveGroupRouteId(HST_ZoneState zone, bool qrf)
	{
		if (!zone)
			return "";

		if (qrf && !zone.m_sQRFRouteId.IsEmpty())
			return zone.m_sQRFRouteId;

		return zone.m_sPatrolRouteId;
	}

	protected vector ResolveGroupSourcePosition(HST_CampaignState state, HST_ZoneState zone, string routeId, bool qrf)
	{
		HST_GeneratedRouteState route;
		if (state && !routeId.IsEmpty())
			route = state.FindGeneratedRoute(routeId);

		if (route)
		{
			if (qrf)
				return route.m_vStartPosition;

			return route.m_vMidPosition;
		}

		if (zone)
			return zone.m_vPosition;

		return "0 0 0";
	}

	protected vector ResolveGroupTargetPosition(HST_CampaignState state, HST_ZoneState zone, string routeId, bool qrf)
	{
		HST_GeneratedRouteState route;
		if (state && !routeId.IsEmpty())
			route = state.FindGeneratedRoute(routeId);

		if (route)
			return route.m_vEndPosition;

		if (zone)
			return zone.m_vPosition;

		return "0 0 0";
	}

	protected bool HasRuntimeGroupEntity(string groupId)
	{
		return m_aRuntimeGroupIds.Find(groupId) >= 0 || m_aRuntimeVehicleGroupIds.Find(groupId) >= 0;
	}

	protected IEntity GetRuntimeGroupEntity(string groupId)
	{
		int index = m_aRuntimeGroupIds.Find(groupId);
		if (index < 0 || index >= m_aRuntimeGroupEntities.Count())
		{
			int vehicleIndex = m_aRuntimeVehicleGroupIds.Find(groupId);
			if (vehicleIndex < 0 || vehicleIndex >= m_aRuntimeVehicleEntities.Count())
				return null;

			return m_aRuntimeVehicleEntities[vehicleIndex];
		}

		return m_aRuntimeGroupEntities[index];
	}

	protected void SetRuntimeGroupEntitiesOrigin(string groupId, vector position)
	{
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId)
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (entity)
				entity.SetOrigin(position);
		}
	}

	protected void DeleteRuntimeGroupEntity(string groupId)
	{
		for (int i = m_aRuntimeGroupIds.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeGroupIds[i] != groupId)
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (entity)
				SCR_EntityHelper.DeleteEntityAndChildren(entity);

			m_aRuntimeGroupIds.Remove(i);
			m_aRuntimeGroupEntities.Remove(i);
		}

		for (int j = m_aRuntimeVehicleGroupIds.Count() - 1; j >= 0; j--)
		{
			if (m_aRuntimeVehicleGroupIds[j] != groupId)
				continue;

			IEntity vehicle = m_aRuntimeVehicleEntities[j];
			if (vehicle)
				SCR_EntityHelper.DeleteEntityAndChildren(vehicle);

			m_aRuntimeVehicleGroupIds.Remove(j);
			m_aRuntimeVehicleEntities.Remove(j);
		}
	}

	protected bool IsZoneInsideHQSafeArea(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone || !state.m_bHQDeployed)
			return false;

		return DistanceSq2D(state.m_vHQPosition, zone.m_vPosition) <= HQ_SAFE_RADIUS_METERS * HQ_SAFE_RADIUS_METERS;
	}

	protected bool IsInsideHQSafeRadius(HST_CampaignState state, vector position)
	{
		if (!state || !state.m_bHQDeployed)
			return false;

		return DistanceSq2D(state.m_vHQPosition, position) <= HQ_SAFE_RADIUS_METERS * HQ_SAFE_RADIUS_METERS;
	}

	protected IEntity GetBestPlayerEntity(PlayerManager playerManager, int playerId)
	{
		if (!playerManager || playerId <= 0)
			return null;

		IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
		if (controlledEntity)
			return controlledEntity;

		return SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
	}

	protected bool IsLivingPlayerEntity(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected HST_ConvoyVehicleControlAdapter GetConvoyVehicleControlAdapter()
	{
		if (!m_ConvoyVehicleControl)
			m_ConvoyVehicleControl = new HST_ConvoyVehicleControlAdapter();

		return m_ConvoyVehicleControl;
	}

	protected bool IsLivingEntity(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}
}
