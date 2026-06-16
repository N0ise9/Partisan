class HST_MissionRuntimeService
{
	static const string PRIMITIVE_KILL_HVT = "kill_hvt";
	static const string PRIMITIVE_HOLD_AREA = "hold_area";
	static const string PRIMITIVE_CLEAR_AREA = "clear_area";
	static const string PRIMITIVE_DESTROY_TARGET = "destroy_target";
	static const string PRIMITIVE_RECOVER_CARGO = "recover_cargo";
	static const string PRIMITIVE_RESCUE_EXTRACT = "rescue_extract";
	static const string PRIMITIVE_DELIVER_SUPPLIES = "deliver_supplies";
	static const string PRIMITIVE_CONVOY_INTERCEPT = "convoy_intercept";
	static const string PRIMITIVE_ABSTRACT_FALLBACK = "abstract_fallback";
	static const string PHASE_CREATED = "created";
	static const string PHASE_ACTIVE = "active";
	static const string PHASE_CONTACT = "contact";
	static const string PHASE_FREED = "freed";
	static const string PHASE_EXTRACTING = "extracting";
	static const string PHASE_FOLLOWING = "following";
	static const string PHASE_DELIVERING = "delivering";
	static const string PHASE_HOLDING = "holding";
	static const string PHASE_FAILED = "failed";
	static const string PHASE_LOADED = "loaded";
	static const string PHASE_UNLOADED = "unloaded";
	static const string PHASE_DELIVERED = "delivered";
	static const string PHASE_DESTROYED = "destroyed";
	static const string PHASE_CAPTURED = "captured";
	static const string PHASE_CONVOY_STAGING = "convoy_staging";
	static const string PHASE_CONVOY_MOVING = "convoy_moving";
	static const string PHASE_CONVOY_CONTACT = "convoy_contact";
	static const string PHASE_CONVOY_ELIMINATED = "convoy_eliminated";
	static const string PHASE_CONVOY_ARRIVED = "convoy_arrived";
	static const string PROP_HVT = "{6985327711303700}Prefabs/Objects/HST/HST_MissionProp_HVT.et";
	static const string PROP_DESTROY_TARGET = "{6985327711303710}Prefabs/Objects/HST/HST_MissionProp_DestroyTarget.et";
	static const string PROP_CARGO = "{6985327711303720}Prefabs/Objects/HST/HST_MissionProp_Cargo.et";
	static const string PROP_CAPTIVES = "{6985327711303730}Prefabs/Objects/HST/HST_MissionProp_Captives.et";
	static const string PROP_HOLD_MARKER = "{6985327711303740}Prefabs/Objects/HST/HST_MissionProp_HoldMarker.et";
	static const string PROP_CITY_SUPPLIES = "{6985327711303750}Prefabs/Objects/HST/HST_MissionProp_CitySupplies.et";
	static const string PROP_CONVOY_PAYLOAD = "{6985327711303760}Prefabs/Objects/HST/HST_MissionProp_ConvoyPayload.et";
	static const string PROP_BANK_MONEY = "{6985327711303770}Prefabs/Objects/HST/HST_MissionProp_BankMoney.et";
	static const string PROP_RESOURCE_CACHE = "{6985327711303780}Prefabs/Objects/HST/HST_MissionProp_ResourceCache.et";
	static const string PROP_CONVOY_VEHICLE = "{4AE9D080927D3CB9}Prefabs/Vehicles/Wheeled/S1203/S1203_base.et";
	static const string ASSET_KIND_CHARACTER = "character";
	static const string ASSET_KIND_CARGO = "cargo";
	static const string ASSET_KIND_CAPTIVE = "captive";
	static const string ASSET_KIND_TARGET = "target";
	static const string ASSET_KIND_VEHICLE = "vehicle";
	static const string ASSET_KIND_AREA = "area";
	static const string ROLE_HVT = "hvt";
	static const string ROLE_DESTROY_TARGET = "destroy_target";
	static const string ROLE_HOLD_MARKER = "hold_marker";
	static const string ROLE_LOGISTICS_CARGO = "logistics_cargo";
	static const string ROLE_CITY_SUPPLIES = "city_supplies";
	static const string ROLE_CAPTIVE = "captive";
	static const string ROLE_CONVOY_VEHICLE = "convoy_vehicle";
	static const string ROLE_CONVOY_PAYLOAD = "convoy_payload";
	static const string ROLE_CONVOY_CAPTIVE = "convoy_captive";
	static const float PLAYER_OBJECTIVE_RADIUS_METERS = 35.0;
	static const float PLAYER_ASSET_RADIUS_METERS = 18.0;
	static const float PLAYER_DELIVERY_RADIUS_METERS = 45.0;
	static const float HOSTILE_OBJECTIVE_RADIUS_METERS = 90.0;
	static const float VEHICLE_CARRIER_RADIUS_METERS = 10.0;
	static const float MAX_CONVOY_ZONE_START_DISTANCE_METERS = 5000.0;
	static const int DEFAULT_HOLD_SECONDS = 45;
	static const int DEFAULT_ASSET_INTERACTION_RADIUS_METERS = 18;
	static const int DEFAULT_DELIVERY_RADIUS_METERS = 45;
	static const int DEFAULT_MISSION_CARRIER_CAPACITY = 6;
	static const int MAX_MISSION_VEHICLE_SCAN_ENTITIES = 96;
	static const float MIN_CONVOY_START_DISTANCE_METERS = 2000.0;
	static const float MAX_CONVOY_START_DISTANCE_METERS = 5000.0;
	static const float MIN_CONVOY_STAGING_ZONE_CLEARANCE_METERS = 260.0;
	static const float MIN_CONVOY_STAGING_SITE_CLEARANCE_METERS = 140.0;
	static const int CONVOY_ROUTE_SAMPLE_COUNT = 6;
	static const int CONVOY_START_PROBE_ATTEMPTS = 72;
	static const float CONVOY_DESTINATION_ROAD_SEARCH_RADIUS_METERS = 300.0;
	static const float CONVOY_START_ROAD_SEARCH_RADIUS_METERS = 250.0;
	static const float CONVOY_SLOT_ROAD_SEARCH_RADIUS_METERS = 40.0;
	static const float CONVOY_EXPANDED_START_ROAD_SEARCH_RADIUS_METERS = 700.0;
	static const float CONVOY_EXPANDED_SLOT_ROAD_SEARCH_RADIUS_METERS = 90.0;
	static const float CONVOY_VEHICLE_START_SPACING_METERS = 24.0;
	static const float MIN_CONVOY_VEHICLE_START_SEPARATION_METERS = 18.0;
	static const int MIN_CONVOY_VEHICLES = 3;
	static const int MAX_CONVOY_VEHICLES = 6;
	static const int MIN_CONVOY_IDLE_SECONDS = 300;
	static const int MAX_CONVOY_IDLE_SECONDS = 600;
	static const float CAPTIVE_FOLLOW_NEAR_DISTANCE_METERS = 5.0;
	static const float CAPTIVE_FOLLOW_BREAK_DISTANCE_METERS = 100.0;
	static const float CAPTIVE_DISEMBARK_RADIUS_METERS = 35.0;
	static const int CAPTIVE_FOLLOW_WAYPOINT_INTERVAL_SECONDS = 4;
	static const string CAPTIVE_FOLLOW_WAYPOINT_PREFAB = "{FBA8DC8FDA0E770D}Prefabs/AI/Waypoints/AIWaypoint_Patrol_Hierarchy.et";
	static const string MISSION_CONVOY_GROUP_PREFIX = "mission_convoy_";
	static const string PERSISTENCE_SMOKE_PREFIX = "hst_smoke";

	protected ref array<string> m_aRuntimeEntityIds = {};
	protected ref array<IEntity> m_aRuntimeEntities = {};
	protected ref array<IEntity> m_aMissionVehicleScanEntities = {};
	protected ref array<string> m_aRestoredMissionCarrierRestoreMissionIds = {};
	protected ref array<string> m_aCaptiveFollowWaypointAssetIds = {};
	protected ref array<int> m_aCaptiveFollowWaypointSeconds = {};

	bool InitializeMissionRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_MissionDefinition definition, HST_ActiveMissionState mission, HST_GeneratedContentService content)
	{
		if (!state || !definition || !mission)
			return false;

		string primitive = PrimitiveForMission(definition);
		mission.m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_STATE_MACHINE;
		mission.m_sRuntimePrimitive = primitive;
		mission.m_sRuntimeType = definition.m_sRuntimeType;
		if (mission.m_sRuntimeType.IsEmpty())
			mission.m_sRuntimeType = primitive;
		mission.m_sRuntimePhase = PHASE_CREATED;
		mission.m_sRuntimeEntityId = string.Format("runtime_%1_%2", mission.m_sInstanceId, primitive);
		mission.m_iRuntimeStartedAtSecond = state.m_iElapsedSeconds;
		mission.m_iRuntimeHoldSeconds = ResolveHoldSeconds(primitive, definition);
		mission.m_bRuntimeFallback = primitive == PRIMITIVE_ABSTRACT_FALLBACK;
		mission.m_bRuntimeCleanupComplete = false;
		if (primitive == PRIMITIVE_CONVOY_INTERCEPT)
		{
			mission.m_sRuntimePhase = PHASE_CONVOY_STAGING;
			mission.m_iRuntimeCounterA = 0;
			mission.m_iRuntimeCounterB = ResolveConvoyIdleDelaySeconds(state, mission);
			mission.m_iRuntimeCounterC = Math.Max(mission.m_iRuntimeCounterB + 300, ResolveConvoyArrivalSeconds(mission));
			mission.m_iRuntimeETASeconds = mission.m_iRuntimeCounterB;
			mission.m_iRequiredCargoCount = 0;
			mission.m_iRequiredCaptiveCount = 0;
		}

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			objective.m_sRuntimePrimitive = primitive;
			if (objective.m_sPhysicalEntityId.IsEmpty())
				objective.m_sPhysicalEntityId = string.Format("%1_%2", mission.m_sRuntimeEntityId, objective.m_sTargetId);
			if (objective.m_sLinkedRuntimeEntityId.IsEmpty())
				objective.m_sLinkedRuntimeEntityId = mission.m_sRuntimeEntityId;
			if (objective.m_iRequiredHoldSeconds <= 0)
				objective.m_iRequiredHoldSeconds = ResolveObjectiveHoldSeconds(objective, primitive, definition);
			objective.m_bAbstractFallback = mission.m_bRuntimeFallback;
		}

		EnsureMissionAssetsInitialized(state, definition, mission);
		LinkObjectivesToMissionAssets(state, mission);
		mission.m_bRuntimeSpawned = TrySpawnMissionRuntimeAssets(state, mission);
		if (!mission.m_bRuntimeSpawned)
		{
			mission.m_bRuntimeSpawned = TrySpawnMissionRuntimeProp(state, mission);
			mission.m_bRuntimeFallback = true;
		}
		else
		{
			mission.m_bRuntimeFallback = false;
		}

		EnsureMissionHostileGroup(state, preset, definition, mission);
		return true;
	}

	bool Tick(HST_CampaignState state, HST_CampaignPreset preset, HST_MissionObjectiveService objectives, int elapsedSeconds)
	{
		if (!state || elapsedSeconds <= 0)
			return false;

		bool changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;

			if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			{
				changed = CleanupMissionRuntime(state, mission) || changed;
				continue;
			}

			changed = RepairActiveMissionRuntimeAfterRestore(state, preset, mission) || changed;
			if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT && state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_VEHICLE) <= 0)
				changed = EnsureConvoyMissionAssetsInitialized(state, mission, null) || changed;
			if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
			{
				changed = EnsureConvoyMissionSpecificAssets(state, mission) || changed;
				changed = SyncConvoyPayloadAssetPositions(state, mission) || changed;
			}
			changed = EnsureMissionRuntimeProp(state, mission) || changed;
			EnsureMissionCaptivesNeutralized(state, mission);
			changed = UpdateFollowingCaptives(state, mission) || changed;
			changed = SyncMissionAssetRuntimePositions(state, mission) || changed;
			changed = AdvanceMissionStateMachine(state, preset, mission, elapsedSeconds) || changed;
			foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
			{
				if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete || objective.m_bFailed)
					continue;

				if (PollObjective(state, preset, mission, objective, elapsedSeconds))
					changed = true;
			}
		}

		return changed;
	}

	protected bool AdvanceMissionStateMachine(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, int elapsedSeconds)
	{
		if (!state || !mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;

		bool changed;
		string previousPhase = mission.m_sRuntimePhase;
		if (mission.m_sRuntimePhase.IsEmpty() || mission.m_sRuntimePhase == PHASE_CREATED)
			mission.m_sRuntimePhase = PHASE_ACTIVE;

		if (mission.m_sRuntimeType == "dynamic_defend_petros" && !state.m_bPetrosAlive)
		{
			MarkRuntimeMissionFailed(state, mission, "Petros was killed during the defense.");
			return true;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
		{
			mission.m_iRuntimeCounterA += elapsedSeconds;
			if (mission.m_iRuntimeCounterB <= 0)
				mission.m_iRuntimeCounterB = ResolveConvoyIdleDelaySeconds(state, mission);
			if (mission.m_iRuntimeCounterC <= mission.m_iRuntimeCounterB)
				mission.m_iRuntimeCounterC = Math.Max(mission.m_iRuntimeCounterB + 300, ResolveConvoyArrivalSeconds(mission));

			if (mission.m_sRuntimePhase.IsEmpty() || mission.m_sRuntimePhase == PHASE_CREATED || mission.m_sRuntimePhase == PHASE_ACTIVE || mission.m_sRuntimePhase == "convoy_static")
				mission.m_sRuntimePhase = PHASE_CONVOY_STAGING;

			if (mission.m_sRuntimePhase == PHASE_CONVOY_STAGING)
				mission.m_iRuntimeETASeconds = Math.Max(0, mission.m_iRuntimeCounterB - mission.m_iRuntimeCounterA);
			else
				mission.m_iRuntimeETASeconds = Math.Max(0, mission.m_iRuntimeCounterC - mission.m_iRuntimeCounterA);

			if (mission.m_sRuntimePhase != PHASE_CONVOY_CONTACT && mission.m_iRuntimeCounterA >= mission.m_iRuntimeCounterC && !AreRuntimeObjectivesComplete(state, mission.m_sInstanceId))
			{
				mission.m_sRuntimePhase = PHASE_CONVOY_ARRIVED;
				MarkRuntimeMissionFailed(state, mission, "Convoy reached its destination.");
				return true;
			}
		}

		UpdateMissionCountersFromObjectives(state, mission);
		UpdateMissionCountersFromAssets(state, mission);
		if (previousPhase != mission.m_sRuntimePhase)
			changed = true;

		return changed;
	}

	protected bool EnsureMissionRuntimeProp(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimeEntityId.IsEmpty())
			return false;

		bool spawned = TrySpawnMissionRuntimeAssets(state, mission);
		if (!spawned && !HasRuntimeEntity(mission.m_sRuntimeEntityId))
			spawned = TrySpawnMissionRuntimeProp(state, mission);

		if (mission.m_bRuntimeSpawned == spawned)
			return spawned;

		mission.m_bRuntimeSpawned = spawned;
		if (spawned)
			mission.m_bRuntimeFallback = false;
		else
			mission.m_bRuntimeFallback = true;

		return true;
	}

	protected void EnsureMissionCaptivesNeutralized(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (asset.m_sKind != ASSET_KIND_CAPTIVE && asset.m_sRole != ROLE_CAPTIVE && asset.m_sRole != ROLE_CONVOY_CAPTIVE)
				continue;
			if (asset.m_bDestroyed || asset.m_bDelivered)
				continue;

			IEntity entity = GetRuntimeEntity(asset.m_sEntityId);
			if (entity)
				ApplyMissionAssetIdentity(entity, asset);
		}
	}

	protected bool EnsureMissionAssetsInitialized(HST_CampaignState state, HST_MissionDefinition definition, HST_ActiveMissionState mission)
	{
		if (!state || !definition || !mission || mission.m_sRuntimePrimitive == PRIMITIVE_ABSTRACT_FALLBACK)
			return false;

		vector targetPosition = ResolveRuntimePropPosition(state, mission);
		vector hqPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vHQPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		int cargoCount = Math.Max(1, Math.Max(definition.m_iCargoCount, mission.m_iRequiredCargoCount));
		int captiveCount = Math.Max(1, Math.Max(definition.m_iCaptiveCount, mission.m_iRequiredCaptiveCount));
		int vehicleCount = Math.Max(1, Math.Max(definition.m_iVehicleCount, mission.m_iRequiredVehicleCount));
		bool changed;

		if (mission.m_sRuntimePrimitive == PRIMITIVE_KILL_HVT)
		{
			changed = AddMissionAsset(state, mission, ASSET_KIND_CHARACTER, ROLE_HVT, PROP_HVT, targetPosition, targetPosition, 0) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_DESTROY_TARGET)
		{
			if (mission.m_sMissionId == "destroy_or_steal_armor")
			{
				string vehiclePrefab = ResolveMissionVehiclePrefab(state, mission);
				for (int i = 0; i < vehicleCount; i++)
					changed = AddMissionAsset(state, mission, ASSET_KIND_VEHICLE, ROLE_DESTROY_TARGET, vehiclePrefab, targetPosition, hqPosition, i) || changed;
				return changed;
			}

			changed = AddMissionAsset(state, mission, ASSET_KIND_TARGET, ROLE_DESTROY_TARGET, PROP_DESTROY_TARGET, targetPosition, targetPosition, 0) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_HOLD_AREA || mission.m_sRuntimePrimitive == PRIMITIVE_CLEAR_AREA)
		{
			changed = AddMissionAsset(state, mission, ASSET_KIND_AREA, ROLE_HOLD_MARKER, PROP_HOLD_MARKER, targetPosition, targetPosition, 0) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_DELIVER_SUPPLIES)
		{
			for (int i = 0; i < cargoCount; i++)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_CITY_SUPPLIES, PROP_CITY_SUPPLIES, hqPosition, targetPosition, i) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO)
		{
			for (int i = 0; i < cargoCount; i++)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_LOGISTICS_CARGO, ResolveLogisticsCargoPrefab(mission), targetPosition, hqPosition, i) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_RESCUE_EXTRACT)
		{
			for (int i = 0; i < captiveCount; i++)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CAPTIVE, ROLE_CAPTIVE, PROP_CAPTIVES, targetPosition, hqPosition, i) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
		{
			return EnsureConvoyMissionAssetsInitialized(state, mission, definition);
		}

		changed = AddMissionAsset(state, mission, ASSET_KIND_AREA, ROLE_HOLD_MARKER, PROP_HOLD_MARKER, targetPosition, targetPosition, 0);
		return changed;
	}

	protected bool EnsureConvoyMissionAssetsInitialized(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionDefinition definition)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;
		if (state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_VEHICLE) > 0)
			return false;

		HST_GeneratedRouteState convoyRoute = ResolveMissionRoute(state, mission);
		vector convoyEnd;
		string convoyDestinationReason;
		if (!TryResolveConvoyEndPosition(state, mission, convoyEnd, convoyDestinationReason))
		{
			mission.m_sRuntimeFailureReason = "No road-resolved convoy destination: " + convoyDestinationReason;
			return false;
		}
		mission.m_vTargetPosition = convoyEnd;
		string convoyVehiclePrefab = ResolveMissionVehiclePrefab(state, mission);
		int convoyVehicleCount = ResolveConvoyVehicleCount(state, mission, definition);
		vector convoyStart;
		string convoySpawnPlanReason;
		array<vector> convoyStartSlots = {};
		if (!TryResolveConvoySpawnPlan(state, mission, convoyRoute, convoyEnd, convoyVehicleCount, convoyStart, convoyStartSlots, convoySpawnPlanReason))
		{
			mission.m_sRuntimeFailureReason = "No road-resolved convoy spawn plan found in required 2000-5000m band: " + convoySpawnPlanReason;
			return false;
		}
		if (IsZeroVector(convoyStart) || convoyStartSlots.Count() != convoyVehicleCount)
		{
			mission.m_sRuntimeFailureReason = "No road-resolved convoy spawn plan found in required 2000-5000m band: full-column probe returned incomplete slots.";
			return false;
		}

		for (int i = 0; i < convoyVehicleCount; i++)
		{
			vector startSlot = convoyStartSlots[i];
			AddMissionAsset(state, mission, ASSET_KIND_VEHICLE, ROLE_CONVOY_VEHICLE, convoyVehiclePrefab, startSlot, convoyEnd, i);
		}

		mission.m_iRequiredVehicleCount = convoyVehicleCount;
		mission.m_iRequiredCargoCount = 0;
		mission.m_iRequiredCaptiveCount = 0;
		EnsureConvoyMissionSpecificAssets(state, mission);
		mission.m_sRuntimeFailureReason = "";
		ConfigureConvoyObjectiveRequirements(state, mission, convoyVehicleCount);
		DeleteRuntimeEntity(mission.m_sRuntimeEntityId);
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(mission.m_sRuntimeEntityId);
		if (runtimeEntity)
			runtimeEntity.m_bDestroyed = true;
		mission.m_bRuntimeSpawned = true;
		mission.m_bRuntimeFallback = false;
		Print(string.Format("h-istasi mission runtime | convoy asset plan ready for %1 | vehicles %2 | start %3 | end %4", mission.m_sInstanceId, convoyVehicleCount, convoyStart, convoyEnd));
		return true;
	}

	protected bool EnsureConvoyMissionSpecificAssets(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;
		if (state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_VEHICLE) <= 0)
			return false;

		bool changed;
		vector sourcePosition = ResolveConvoyPayloadSourcePosition(state, mission);
		if (mission.m_sMissionId == "convoy_money")
		{
			mission.m_iRequiredCargoCount = 1;
			if (state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_PAYLOAD) <= 0)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_CONVOY_PAYLOAD, PROP_BANK_MONEY, sourcePosition, ResolveConvoyPayloadDeliveryPosition(state, mission), 0) || changed;
		}
		else if (mission.m_sMissionId == "convoy_supplies")
		{
			mission.m_iRequiredCargoCount = 1;
			if (state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_PAYLOAD) <= 0)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_CONVOY_PAYLOAD, PROP_CONVOY_PAYLOAD, sourcePosition, ResolveConvoyPayloadDeliveryPosition(state, mission), 0) || changed;
		}
		else if (mission.m_sMissionId == "convoy_prisoners")
		{
			mission.m_iRequiredCaptiveCount = 1;
			if (state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_CAPTIVE) <= 0)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CAPTIVE, ROLE_CONVOY_CAPTIVE, PROP_CAPTIVES, sourcePosition, ResolveConvoyPayloadDeliveryPosition(state, mission), 0) || changed;
		}

		return changed;
	}

	protected vector ResolveConvoyPayloadSourcePosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != ROLE_CONVOY_VEHICLE)
				continue;
			if (!IsZeroVector(asset.m_vCurrentPosition))
				return asset.m_vCurrentPosition;
			if (!IsZeroVector(asset.m_vSourcePosition))
				return asset.m_vSourcePosition;
		}

		if (!IsZeroVector(mission.m_vTargetPosition))
			return mission.m_vTargetPosition;
		return state.m_vHQPosition;
	}

	protected vector ResolveConvoyPayloadDeliveryPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		if (mission.m_sMissionId == "convoy_supplies")
		{
			HST_ZoneState supportTown = ResolveConvoySupportTown(state, mission);
			if (supportTown)
				return supportTown.m_vPosition;
		}

		return state.m_vHQPosition;
	}

	protected HST_ZoneState ResolveConvoySupportTown(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return null;

		HST_ZoneState targetZone = state.FindZone(mission.m_sTargetZoneId);
		if (targetZone && targetZone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			return targetZone;

		vector searchPosition = mission.m_vTargetPosition;
		if (IsZeroVector(searchPosition) && targetZone)
			searchPosition = targetZone.m_vPosition;
		if (IsZeroVector(searchPosition))
			searchPosition = state.m_vHQPosition;

		HST_ZoneState bestTown;
		float bestDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
				continue;

			float distanceSq = DistanceSq2D(searchPosition, zone.m_vPosition);
			if (distanceSq >= bestDistanceSq)
				continue;

			bestDistanceSq = distanceSq;
			bestTown = zone;
		}

		return bestTown;
	}

	protected bool SyncConvoyPayloadAssetPositions(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;

		vector payloadPosition = ResolveConvoyPayloadSourcePosition(state, mission);
		if (IsZeroVector(payloadPosition))
			return false;

		bool changed;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (asset.m_sRole != ROLE_CONVOY_PAYLOAD && asset.m_sRole != ROLE_CONVOY_CAPTIVE)
				continue;
			if (asset.m_bPickedUp || asset.m_bDelivered || asset.m_bDestroyed)
				continue;
			if (DistanceSq2D(asset.m_vCurrentPosition, payloadPosition) <= 1.0)
				continue;

			asset.m_vCurrentPosition = payloadPosition;
			asset.m_vLastKnownPosition = payloadPosition;
			changed = true;
		}

		return changed;
	}

	protected bool AddMissionAsset(HST_CampaignState state, HST_ActiveMissionState mission, string kind, string role, string prefab, vector sourcePosition, vector targetPosition, int index)
	{
		if (!state || !mission || role.IsEmpty())
			return false;

		string assetId = string.Format("asset_%1_%2_%3", mission.m_sInstanceId, role, index);
		HST_MissionAssetState existingAsset = state.FindMissionAsset(assetId);
		if (existingAsset)
			return RepairMissionAssetRecord(existingAsset, mission, kind, role, prefab, sourcePosition, targetPosition, index);

		HST_MissionAssetState asset = new HST_MissionAssetState();
		asset.m_sAssetId = assetId;
		asset.m_sEntityId = assetId;
		asset.m_sMissionInstanceId = mission.m_sInstanceId;
		asset.m_sKind = kind;
		asset.m_sRole = role;
		asset.m_sPrefab = prefab;
		if (role == ROLE_CONVOY_VEHICLE)
			asset.m_vSourcePosition = sourcePosition;
		else
			asset.m_vSourcePosition = OffsetMissionAssetPosition(sourcePosition, index);
		asset.m_vTargetPosition = targetPosition;
		asset.m_vCurrentPosition = asset.m_vSourcePosition;
		asset.m_vLastKnownPosition = asset.m_vSourcePosition;
		asset.m_iDeadlineSecond = mission.m_iActiveUntilSecond;
		asset.m_iCargoCapacityCost = ResolveAssetCapacityCost(kind, role);
		asset.m_iInteractionRadiusMeters = ResolveAssetInteractionRadius(kind, role);
		asset.m_bAlive = true;
		state.m_aMissionAssets.Insert(asset);
		return true;
	}

	protected bool RepairMissionAssetRecord(HST_MissionAssetState asset, HST_ActiveMissionState mission, string kind, string role, string prefab, vector sourcePosition, vector targetPosition, int index)
	{
		if (!asset || !mission)
			return false;

		bool changed;
		vector resolvedSourcePosition;
		if (role == ROLE_CONVOY_VEHICLE)
			resolvedSourcePosition = sourcePosition;
		else
			resolvedSourcePosition = OffsetMissionAssetPosition(sourcePosition, index);

		if (asset.m_sMissionInstanceId != mission.m_sInstanceId)
		{
			asset.m_sMissionInstanceId = mission.m_sInstanceId;
			changed = true;
		}
		if (asset.m_sKind != kind)
		{
			asset.m_sKind = kind;
			changed = true;
		}
		if (asset.m_sRole != role)
		{
			asset.m_sRole = role;
			changed = true;
		}
		if (asset.m_sPrefab.IsEmpty() || (kind == ASSET_KIND_CAPTIVE && asset.m_sPrefab != prefab))
		{
			asset.m_sPrefab = prefab;
			changed = true;
		}
		if (asset.m_sEntityId.IsEmpty())
		{
			asset.m_sEntityId = asset.m_sAssetId;
			changed = true;
		}
		if (IsZeroVector(asset.m_vSourcePosition) && !IsZeroVector(resolvedSourcePosition))
		{
			asset.m_vSourcePosition = resolvedSourcePosition;
			changed = true;
		}
		if (IsZeroVector(asset.m_vTargetPosition) && !IsZeroVector(targetPosition))
		{
			asset.m_vTargetPosition = targetPosition;
			changed = true;
		}
		if (!asset.m_bPickedUp && !IsTerminalMissionAssetState(asset))
		{
			vector repairedPosition = asset.m_vCurrentPosition;
			if (IsZeroVector(repairedPosition))
				repairedPosition = asset.m_vSourcePosition;
			if (IsZeroVector(repairedPosition))
				repairedPosition = resolvedSourcePosition;
			if (!IsZeroVector(repairedPosition) && DistanceSq2D(asset.m_vCurrentPosition, repairedPosition) > 1.0)
			{
				asset.m_vCurrentPosition = repairedPosition;
				changed = true;
			}
			if (!IsZeroVector(repairedPosition) && IsZeroVector(asset.m_vLastKnownPosition))
			{
				asset.m_vLastKnownPosition = repairedPosition;
				changed = true;
			}
		}
		if (asset.m_iDeadlineSecond <= 0 && mission.m_iActiveUntilSecond > 0)
		{
			asset.m_iDeadlineSecond = mission.m_iActiveUntilSecond;
			changed = true;
		}
		int capacityCost = ResolveAssetCapacityCost(kind, role);
		if (asset.m_iCargoCapacityCost <= 0 && capacityCost > 0)
		{
			asset.m_iCargoCapacityCost = capacityCost;
			changed = true;
		}
		int interactionRadius = ResolveAssetInteractionRadius(kind, role);
		if (asset.m_iInteractionRadiusMeters <= 0 && interactionRadius > 0)
		{
			asset.m_iInteractionRadiusMeters = interactionRadius;
			changed = true;
		}
		if (!IsTerminalMissionAssetState(asset) && !asset.m_bAlive)
		{
			asset.m_bAlive = true;
			changed = true;
		}

		return changed;
	}

	protected bool IsTerminalMissionAssetState(HST_MissionAssetState asset)
	{
		if (!asset)
			return true;

		return asset.m_bDestroyed || asset.m_bDelivered || asset.m_sLastInteraction == PHASE_CAPTURED || asset.m_bOutcomeApplied;
	}

	protected void LinkObjectivesToMissionAssets(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			HST_MissionAssetState asset = FindRepresentativeAssetForObjective(state, mission, objective);
			if (!asset)
				continue;

			objective.m_sLinkedRuntimeEntityId = asset.m_sEntityId;
			objective.m_sPhysicalEntityId = asset.m_sEntityId;
			if (objective.m_vPosition[0] == 0 && objective.m_vPosition[1] == 0 && objective.m_vPosition[2] == 0)
				objective.m_vPosition = asset.m_vSourcePosition;
		}
	}

	protected bool TrySpawnMissionRuntimeAssets(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		bool foundAsset;
		bool hasUsableAsset;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			foundAsset = true;
			if (asset.m_bDelivered || asset.m_bDestroyed)
			{
				hasUsableAsset = true;
				continue;
			}

			if (asset.m_bPickedUp && !asset.m_bDelivered && asset.m_sKind != ASSET_KIND_CAPTIVE)
			{
				hasUsableAsset = true;
				continue;
			}

			if (asset.m_bPickedUp && !asset.m_bDelivered && asset.m_sKind == ASSET_KIND_CAPTIVE && asset.m_sLastInteraction == PHASE_LOADED)
			{
				hasUsableAsset = true;
				continue;
			}

			if (asset.m_sPrefab.IsEmpty())
				continue;

			if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT && asset.m_sRole == ROLE_CONVOY_VEHICLE)
			{
				hasUsableAsset = true;
				continue;
			}

			if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT && (asset.m_sRole == ROLE_CONVOY_PAYLOAD || asset.m_sRole == ROLE_CONVOY_CAPTIVE))
			{
				hasUsableAsset = true;
				continue;
			}

			if (HasRuntimeEntity(asset.m_sEntityId))
			{
				hasUsableAsset = true;
				continue;
			}

			vector position;
			if (asset.m_sKind == ASSET_KIND_VEHICLE)
			{
				if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(asset.m_vCurrentPosition, position, true))
				{
					mission.m_sRuntimeFailureReason = "No dry vehicle-safe position for " + asset.m_sRole;
					Print(string.Format("h-istasi mission runtime | vehicle asset spawn skipped for %1 at %2: %3", asset.m_sAssetId, asset.m_vCurrentPosition, mission.m_sRuntimeFailureReason), LogLevel.WARNING);
					continue;
				}
			}
			else
			{
				float groundOffset = HST_WorldPositionService.PROP_GROUND_OFFSET;
				if (asset.m_sKind == ASSET_KIND_CAPTIVE)
					groundOffset = HST_WorldPositionService.CHARACTER_GROUND_OFFSET;
				position = HST_WorldPositionService.ResolveSafeGroundPosition(asset.m_vCurrentPosition, groundOffset, false, 2.0);
			}
			vector angles = BuildRuntimePropAngles(state, mission);
			GenericEntity entity = HST_WorldPositionService.SpawnPrefab(asset.m_sPrefab, position, angles);
			if (!entity && asset.m_sKind == ASSET_KIND_CAPTIVE && asset.m_sPrefab != PROP_CAPTIVES)
				entity = HST_WorldPositionService.SpawnPrefab(PROP_CAPTIVES, position, angles);
			if (!entity && asset.m_sPrefab != PROP_HOLD_MARKER)
				entity = HST_WorldPositionService.SpawnPrefab(PROP_HOLD_MARKER, position, angles);

			if (!entity)
			{
				Print(string.Format("h-istasi mission runtime | asset spawn failed for %1 using %2", asset.m_sAssetId, asset.m_sPrefab), LogLevel.WARNING);
				continue;
			}
			if (ShouldApplyUprightMissionAssetTransform(asset))
				HST_WorldPositionService.ApplyUprightEntityTransform(entity, position, angles);
			ApplyMissionAssetIdentity(entity, asset);

			HST_MissionAssetComponent assetComponent = HST_MissionAssetComponent.Cast(entity.FindComponent(HST_MissionAssetComponent));
			if (assetComponent)
				assetComponent.ConfigureMissionAsset(asset.m_sAssetId, mission.m_sInstanceId, asset.m_sRole);

			asset.m_bSpawned = true;
			asset.m_vCurrentPosition = position;
			asset.m_vLastKnownPosition = position;
			m_aRuntimeEntityIds.Insert(asset.m_sEntityId);
			m_aRuntimeEntities.Insert(entity);
			RegisterAssetRuntimeEntityState(state, asset, position, angles);
			if (asset.m_sRole == ROLE_CONVOY_VEHICLE && asset.m_sLastInteraction.IsEmpty())
			{
				asset.m_sLastInteraction = "convoy_static";
				if (mission.m_sRuntimePhase == PHASE_ACTIVE || mission.m_sRuntimePhase == PHASE_CREATED)
					mission.m_sRuntimePhase = "convoy_static";
			}
			hasUsableAsset = true;
			Print(string.Format("h-istasi mission runtime | spawned asset %1 role %2 for %3 at %4", asset.m_sPrefab, asset.m_sRole, mission.m_sInstanceId, position));
			LogMissionCaptiveProjection(entity, asset);
		}

		return foundAsset && hasUsableAsset;
	}

	protected bool TrySpawnMissionRuntimeProp(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimeEntityId.IsEmpty() || mission.m_sRuntimePrimitive == PRIMITIVE_ABSTRACT_FALLBACK)
			return false;

		vector position = ResolveRuntimePropPosition(state, mission);
		string prefab = SelectRuntimePropPrefab(mission.m_sRuntimePrimitive);
		if (prefab.IsEmpty())
			return false;

		vector angles = BuildRuntimePropAngles(state, mission);
		GenericEntity entity = HST_WorldPositionService.SpawnPrefab(prefab, position, angles);
		if (!entity && prefab != PROP_HOLD_MARKER)
			entity = HST_WorldPositionService.SpawnPrefab(PROP_HOLD_MARKER, position, angles);

		if (!entity)
		{
			Print(string.Format("h-istasi mission runtime | prop spawn failed for %1 using %2", mission.m_sInstanceId, prefab), LogLevel.WARNING);
			return false;
		}

		HST_WorldPositionService.ApplyUprightEntityTransform(entity, position, angles);
		m_aRuntimeEntityIds.Insert(mission.m_sRuntimeEntityId);
		m_aRuntimeEntities.Insert(entity);
		RegisterRuntimeEntityState(state, mission, prefab, position, angles);
		Print(string.Format("h-istasi mission runtime | spawned prop %1 for %2 at %3", prefab, mission.m_sInstanceId, position));
		return true;
	}

	protected bool ShouldApplyUprightMissionAssetTransform(HST_MissionAssetState asset)
	{
		if (!asset)
			return false;

		return asset.m_sKind == ASSET_KIND_VEHICLE || asset.m_sKind == ASSET_KIND_TARGET || asset.m_sKind == ASSET_KIND_AREA || asset.m_sKind == ASSET_KIND_CARGO;
	}

	protected void RegisterRuntimeEntityState(HST_CampaignState state, HST_ActiveMissionState mission, string prefab, vector position, vector angles)
	{
		if (!state || !mission)
			return;

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(mission.m_sRuntimeEntityId);
		if (!runtimeEntity)
		{
			runtimeEntity = new HST_MissionRuntimeEntityState();
			runtimeEntity.m_sRuntimeEntityId = mission.m_sRuntimeEntityId;
			runtimeEntity.m_sMissionInstanceId = mission.m_sInstanceId;
			state.m_aMissionRuntimeEntities.Insert(runtimeEntity);
		}

		runtimeEntity.m_sKind = mission.m_sRuntimePrimitive;
		runtimeEntity.m_sPrefab = prefab;
		runtimeEntity.m_vPosition = position;
		runtimeEntity.m_vAngles = angles;
		runtimeEntity.m_bSpawned = true;
	}

	protected void RegisterAssetRuntimeEntityState(HST_CampaignState state, HST_MissionAssetState asset, vector position, vector angles)
	{
		if (!state || !asset)
			return;

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (!runtimeEntity)
		{
			runtimeEntity = new HST_MissionRuntimeEntityState();
			runtimeEntity.m_sRuntimeEntityId = asset.m_sEntityId;
			runtimeEntity.m_sMissionInstanceId = asset.m_sMissionInstanceId;
			state.m_aMissionRuntimeEntities.Insert(runtimeEntity);
		}

		runtimeEntity.m_sKind = asset.m_sRole;
		runtimeEntity.m_sPrefab = asset.m_sPrefab;
		runtimeEntity.m_vPosition = position;
		runtimeEntity.m_vAngles = angles;
		runtimeEntity.m_bSpawned = true;
		runtimeEntity.m_bDestroyed = asset.m_bDestroyed;
		runtimeEntity.m_bRecovered = asset.m_bPickedUp || asset.m_bDelivered;
		if (asset.m_sKind == ASSET_KIND_VEHICLE)
			RegisterMissionRuntimeVehicle(state, asset, position, angles);
	}

	protected void RegisterMissionRuntimeVehicle(HST_CampaignState state, HST_MissionAssetState asset, vector position, vector angles)
	{
		if (!state || !asset || asset.m_sEntityId.IsEmpty())
			return;

		HST_RuntimeVehicleState vehicle = state.FindRuntimeVehicle(asset.m_sEntityId);
		if (!vehicle)
		{
			vehicle = new HST_RuntimeVehicleState();
			vehicle.m_sVehicleRuntimeId = asset.m_sEntityId;
			state.m_aRuntimeVehicles.Insert(vehicle);
		}

		vehicle.m_sPrefab = asset.m_sPrefab;
		vehicle.m_sDisplayName = asset.m_sRole;
		vehicle.m_sRuntimeKind = asset.m_sRole;
		vehicle.m_vPosition = position;
		vehicle.m_vAngles = angles;
		vehicle.m_bDeleted = asset.m_bDestroyed || asset.m_bDelivered;
	}

	protected void ApplyMissionAssetIdentity(IEntity entity, HST_MissionAssetState asset)
	{
		if (!entity || !asset)
			return;
		if (asset.m_sKind != ASSET_KIND_CAPTIVE && asset.m_sRole != ROLE_CAPTIVE && asset.m_sRole != ROLE_CONVOY_CAPTIVE)
			return;

		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (factionComponent)
			factionComponent.SetAffiliatedFactionByKey("CIV");

		int removedWeaponItems = StripMissionCaptiveWeapons(entity);
		if (removedWeaponItems > 0)
			Print(string.Format("h-istasi mission runtime | disarmed captive projection | asset %1 | removed %2 weapon item(s)", asset.m_sAssetId, removedWeaponItems));
	}

	protected int StripMissionCaptiveWeapons(IEntity entity)
	{
		if (!entity)
			return 0;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return 0;

		array<IEntity> items = {};
		inventory.GetItems(items, EStoragePurpose.PURPOSE_ANY);
		int removed;
		foreach (IEntity item : items)
		{
			if (!item || !IsMissionCaptiveWeaponItem(item))
				continue;

			if (RemoveMissionCaptiveWeaponItem(inventory, item))
				removed++;
		}

		return removed;
	}

	protected bool IsMissionCaptiveWeaponItem(IEntity item)
	{
		if (!item)
			return false;

		if (BaseWeaponComponent.Cast(item.FindComponent(BaseWeaponComponent)))
			return true;
		if (MagazineComponent.Cast(item.FindComponent(MagazineComponent)) || BaseMagazineComponent.Cast(item.FindComponent(BaseMagazineComponent)))
			return true;
		if (GrenadeMoveComponent.Cast(item.FindComponent(GrenadeMoveComponent)))
			return true;

		string prefab = ResolveEntityPrefabName(item);
		prefab.ToLower();
		if (prefab.Contains("/weapons/") || prefab.Contains("weapon_") || prefab.Contains("rifle") || prefab.Contains("carbine") || prefab.Contains("smg"))
			return true;
		if (prefab.Contains("launcher") || prefab.Contains("m16") || prefab.Contains("ak74") || prefab.Contains("rpg"))
			return true;
		if (prefab.Contains("magazine") || prefab.Contains("mag_") || prefab.Contains("_mag") || prefab.Contains("stanag") || prefab.Contains("pmag") || prefab.Contains("belt"))
			return true;
		if (prefab.Contains("grenade") || prefab.Contains("explosive") || prefab.Contains("mine"))
			return true;

		return false;
	}

	protected bool RemoveMissionCaptiveWeaponItem(SCR_InventoryStorageManagerComponent inventory, IEntity item)
	{
		if (!inventory || !item)
			return false;

		if (inventory.TryDeleteItem(item))
			return true;

		BaseInventoryStorageComponent storage = inventory.FindStorageForItem(item, EStoragePurpose.PURPOSE_ANY);
		if (storage && inventory.TryRemoveItemFromStorage(item, storage))
			return true;

		InventoryItemComponent itemComponent = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (itemComponent && itemComponent.GetParentSlot())
		{
			SCR_EntityHelper.DeleteEntityAndChildren(item);
			return true;
		}

		if (!item.GetParent())
		{
			SCR_EntityHelper.DeleteEntityAndChildren(item);
			return true;
		}

		return false;
	}

	protected void LogMissionCaptiveProjection(IEntity entity, HST_MissionAssetState asset)
	{
		if (!entity || !asset)
			return;
		if (asset.m_sKind != ASSET_KIND_CAPTIVE && asset.m_sRole != ROLE_CAPTIVE && asset.m_sRole != ROLE_CONVOY_CAPTIVE)
			return;

		bool hasActions = ActionsManagerComponent.Cast(entity.FindComponent(ActionsManagerComponent)) != null;
		bool hasRpl = BaseRplComponent.Cast(entity.FindComponent(BaseRplComponent)) != null;
		bool hasMissionAsset = HST_MissionAssetComponent.Cast(entity.FindComponent(HST_MissionAssetComponent)) != null;
		Print(string.Format("h-istasi mission runtime | captive projection live | asset %1 | entity %2 | origin %3 | actions %4 | rpl %5 | mission asset component %6", asset.m_sAssetId, entity.GetName(), entity.GetOrigin(), hasActions, hasRpl, hasMissionAsset));
	}

	protected bool EnsureRestoredMissionCarrierVehicles(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		bool changed;
		array<string> restoredCarrierIds = {};
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || !asset.m_bPickedUp || asset.m_bDelivered || asset.m_bDestroyed)
				continue;
			if (!asset.m_bAttachedToCarrier || asset.m_sCarriedByVehicleId.IsEmpty() || IsPlayerCarrierId(asset.m_sCarriedByVehicleId))
				continue;
			if (restoredCarrierIds.Contains(asset.m_sCarriedByVehicleId))
				continue;

			if (EnsureRestoredMissionCarrierVehicle(state, asset))
				changed = true;
			restoredCarrierIds.Insert(asset.m_sCarriedByVehicleId);
		}

		return changed;
	}

	protected bool ShouldRunRestoredMissionCarrierRestorePass(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || !state.m_bRestoredFromPersistence || mission.m_sInstanceId.IsEmpty())
			return false;
		if (m_aRestoredMissionCarrierRestoreMissionIds.Contains(mission.m_sInstanceId))
			return false;

		m_aRestoredMissionCarrierRestoreMissionIds.Insert(mission.m_sInstanceId);
		return true;
	}

	protected bool EnsureRestoredMissionCarrierVehicle(HST_CampaignState state, HST_MissionAssetState asset)
	{
		if (!state || !asset || asset.m_sCarriedByVehicleId.IsEmpty())
			return false;
		if (HasRuntimeEntity(asset.m_sCarriedByVehicleId))
			return false;

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(asset.m_sCarriedByVehicleId);
		if (!runtimeVehicle || runtimeVehicle.m_bDeleted || runtimeVehicle.m_sPrefab.IsEmpty())
			return false;

		vector sourcePosition = runtimeVehicle.m_vPosition;
		if (IsZeroVector(sourcePosition))
			sourcePosition = asset.m_vCurrentPosition;
		if (IsZeroVector(sourcePosition))
			sourcePosition = asset.m_vLastKnownPosition;
		if (IsZeroVector(sourcePosition))
			sourcePosition = asset.m_vSourcePosition;

		vector spawnPosition;
		if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(sourcePosition, spawnPosition, true))
		{
			Print(string.Format("h-istasi mission runtime | carrier restore skipped for %1 at %2", asset.m_sCarriedByVehicleId, sourcePosition), LogLevel.WARNING);
			return false;
		}

		vector angles = runtimeVehicle.m_vAngles;
		GenericEntity entity = HST_WorldPositionService.SpawnPrefab(runtimeVehicle.m_sPrefab, spawnPosition, angles);
		if (!entity)
		{
			Print(string.Format("h-istasi mission runtime | carrier restore failed for %1 using %2", asset.m_sCarriedByVehicleId, runtimeVehicle.m_sPrefab), LogLevel.WARNING);
			return false;
		}

		HST_WorldPositionService.ApplyUprightEntityTransform(entity, spawnPosition, angles);
		m_aRuntimeEntityIds.Insert(asset.m_sCarriedByVehicleId);
		m_aRuntimeEntities.Insert(entity);
		runtimeVehicle.m_vPosition = spawnPosition;
		runtimeVehicle.m_bDeleted = false;
		Print(string.Format("h-istasi mission runtime | restored carrier %1 for mission cargo at %2", asset.m_sCarriedByVehicleId, spawnPosition));
		return true;
	}

	bool HandlePlayerMissionInteraction(HST_CampaignState state, HST_CampaignPreset preset, HST_ArsenalService arsenal, int playerId, IEntity playerEntity, string commandId, string argument, out string result, out string eventType, out string missionInstanceId)
	{
		result = "h-istasi mission | no change";
		eventType = "";
		missionInstanceId = "";
		if (!state || !playerEntity)
		{
			result = "h-istasi mission | failed: no controlled player entity";
			return false;
		}

		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
		{
			result = "h-istasi mission | failed: campaign is not active";
			return false;
		}

		if (!IsLivingPlayerEntity(playerEntity))
		{
			result = "h-istasi mission | failed: player is not alive";
			return false;
		}

		vector playerPosition = playerEntity.GetOrigin();
		HST_MissionAssetState asset = ResolveInteractionAsset(state, commandId, argument, playerId, playerPosition);
		if (!asset)
		{
			result = "h-istasi mission | failed: no eligible mission asset nearby";
			return false;
		}

		HST_ActiveMissionState mission = state.FindActiveMission(asset.m_sMissionInstanceId);
		bool allowPostCompletionConvoyInteraction = IsPostCompletionConvoyInteractionAllowed(mission, asset, commandId);
		if (!mission || (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE && !allowPostCompletionConvoyInteraction))
		{
			result = "h-istasi mission | failed: mission is no longer active";
			return false;
		}
		if (AreMissionObjectivesComplete(state, mission) && !allowPostCompletionConvoyInteraction)
		{
			result = "h-istasi mission | failed: mission objectives are already complete";
			return false;
		}
		if (IsConvoyPayloadOrCaptiveAsset(asset) && HasLivingConvoyCrewForMission(state, mission))
		{
			result = "h-istasi mission | failed: neutralize the convoy crew before recovering this convoy asset";
			return false;
		}

		if (asset.m_bPickedUp && !asset.m_sCarriedByVehicleId.IsEmpty())
		{
			vector carrierPosition;
			if (TryResolveCarrierPosition(state, asset.m_sCarriedByVehicleId, asset.m_vCurrentPosition, carrierPosition))
			{
				asset.m_vCurrentPosition = carrierPosition;
				asset.m_vLastKnownPosition = carrierPosition;
			}
			else if (commandId == "mission_asset_unload" || commandId == "mission_asset_deliver" || commandId == "mission_captive_extract" || commandId == "mission_captive_follow")
			{
				asset.m_vCurrentPosition = playerPosition;
				asset.m_vLastKnownPosition = playerPosition;
			}
		}

		vector validationPosition = ResolveInteractionValidationPosition(asset, commandId);
		vector convoyAccessPosition;
		if (TryResolveConvoyAssetAccessPosition(state, asset, playerPosition, convoyAccessPosition))
		{
			asset.m_vCurrentPosition = convoyAccessPosition;
			asset.m_vLastKnownPosition = convoyAccessPosition;
			validationPosition = convoyAccessPosition;
		}
		float radius = ResolveInteractionRadius(asset, commandId);
		if (DistanceSq2D(playerPosition, validationPosition) > radius * radius)
		{
			if (commandId == "mission_asset_deliver" || (commandId == "mission_captive_extract" && asset.m_bPickedUp))
				result = string.Format("h-istasi mission | failed: bring %1 within %2m of the delivery zone", BuildAssetShortLabel(asset), Math.Round(radius));
			else
				result = string.Format("h-istasi mission | failed: move within %1m of %2", Math.Round(radius), BuildAssetShortLabel(asset));
			return false;
		}

		bool changed;
		if (commandId == "mission_asset_load")
			changed = ApplyLoadInteraction(state, mission, asset, playerId, playerEntity, playerPosition, result, eventType);
		else if (commandId == "mission_asset_unload")
			changed = ApplyUnloadInteraction(state, mission, asset, playerId, playerPosition, result, eventType);
		else if (commandId == "mission_asset_deliver")
			changed = ApplyDeliverInteraction(state, mission, asset, playerPosition, result, eventType);
		else if (commandId == "mission_captive_extract")
			changed = ApplyCaptiveExtractInteraction(state, mission, asset, playerId, playerEntity, playerPosition, result, eventType);
		else if (commandId == "mission_captive_follow")
			changed = ApplyCaptiveFollowInteraction(state, mission, asset, playerId, playerEntity, playerPosition, result, eventType);
		else if (commandId == "mission_vehicle_capture")
			changed = ApplyVehicleCaptureInteraction(state, mission, asset, arsenal, playerPosition, result, eventType);
		else if (commandId == "mission_asset_sabotage")
			changed = ApplySabotageInteraction(state, mission, asset, playerPosition, result, eventType);
		else
		{
			result = "h-istasi mission | failed: unknown mission interaction";
			return false;
		}

		if (!changed)
			return false;

		missionInstanceId = mission.m_sInstanceId;
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
		{
			mission.m_sLastRuntimeEventKey = eventType;
			RefreshMissionObjectivesFromAssets(state, mission);
			UpdateMissionCountersFromAssets(state, mission);
		}
		return true;
	}

	bool MarkMissionAssetDestroyedByRuntimeEntity(HST_CampaignState state, string assetId, vector position, out string result, out string eventType, out string missionInstanceId)
	{
		result = "h-istasi mission | no change";
		eventType = "";
		missionInstanceId = "";
		if (!state || assetId.IsEmpty())
		{
			result = "h-istasi mission | failed: no mission asset supplied";
			return false;
		}

		HST_MissionAssetState asset = state.FindMissionAsset(assetId);
		if (!asset)
		{
			result = "h-istasi mission | failed: mission asset not found";
			return false;
		}

		HST_ActiveMissionState mission = state.FindActiveMission(asset.m_sMissionInstanceId);
		bool allowPostCompletionConvoyVehicleDestruction = IsPreservedConvoyVehicleAfterCrewElimination(mission, asset);
		if (!mission || (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE && !allowPostCompletionConvoyVehicleDestruction))
		{
			result = "h-istasi mission | failed: mission is no longer active";
			return false;
		}

		if (asset.m_bDestroyed)
		{
			result = "h-istasi mission | already destroyed " + BuildAssetShortLabel(asset);
			return false;
		}

		MarkMissionAssetDestroyed(state, mission, asset, position);
		result = "h-istasi mission | destroyed " + BuildAssetShortLabel(asset);
		eventType = "sabotaged";
		if (asset.m_sRole == ROLE_HVT)
			eventType = "neutralized";
		missionInstanceId = mission.m_sInstanceId;
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
		{
			RefreshMissionObjectivesFromAssets(state, mission);
			UpdateMissionCountersFromAssets(state, mission);
		}
		return true;
	}

	protected HST_MissionAssetState ResolveInteractionAsset(HST_CampaignState state, string commandId, string argument, int playerId, vector playerPosition)
	{
		if (!argument.IsEmpty())
		{
			HST_MissionAssetState explicitAsset = state.FindMissionAsset(argument);
			if (explicitAsset && IsInteractionAssetEligible(explicitAsset, commandId, playerId))
				return explicitAsset;
		}

		HST_MissionAssetState bestAsset;
		float bestDistanceSq = 999999999.0;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || !IsInteractionAssetEligible(asset, commandId, playerId))
				continue;

			HST_ActiveMissionState mission = state.FindActiveMission(asset.m_sMissionInstanceId);
			if (!mission || (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE && !IsPostCompletionConvoyInteractionAllowed(mission, asset, commandId)))
				continue;

			vector position = ResolveInteractionValidationPosition(asset, commandId);
			vector convoyAccessPosition;
			if (TryResolveConvoyAssetAccessPosition(state, asset, playerPosition, convoyAccessPosition))
				position = convoyAccessPosition;
			float distanceSq = DistanceSq2D(playerPosition, position);
			if (distanceSq >= bestDistanceSq)
				continue;

			bestDistanceSq = distanceSq;
			bestAsset = asset;
		}

		return bestAsset;
	}

	protected bool IsInteractionAssetEligible(HST_MissionAssetState asset, string commandId, int playerId)
	{
		if (!asset || asset.m_bDelivered)
			return false;

		if (commandId == "mission_asset_load")
			return IsTransportableAsset(asset) && !asset.m_bPickedUp && !asset.m_bDestroyed;

		if (commandId == "mission_asset_unload")
			return IsTransportableAsset(asset) && asset.m_bPickedUp && !asset.m_bDestroyed;

		if (commandId == "mission_asset_deliver")
			return IsTransportableAsset(asset) && asset.m_bPickedUp && !asset.m_bDestroyed;

		if (commandId == "mission_captive_extract")
			return asset.m_sKind == ASSET_KIND_CAPTIVE && !asset.m_bDestroyed;

		if (commandId == "mission_captive_follow")
			return asset.m_sKind == ASSET_KIND_CAPTIVE && asset.m_bPickedUp && !asset.m_bDelivered && !asset.m_bDestroyed;

		if (commandId == "mission_vehicle_capture")
			return asset.m_sKind == ASSET_KIND_VEHICLE && !asset.m_bDestroyed;

		if (commandId == "mission_asset_sabotage")
			return (asset.m_sKind == ASSET_KIND_TARGET || asset.m_sKind == ASSET_KIND_CHARACTER || asset.m_sKind == ASSET_KIND_VEHICLE) && !asset.m_bDestroyed;

		return false;
	}

	protected bool ApplyLoadInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, int playerId, IEntity playerEntity, vector playerPosition, out string result, out string eventType)
	{
		string carrierId;
		string carrierName;
		vector carrierPosition;
		string failureReason;
		if (!TryResolveMissionAssetCarrier(state, asset, playerId, playerEntity, playerPosition, carrierId, carrierName, carrierPosition, failureReason))
		{
			result = "h-istasi mission | failed: " + failureReason;
			return false;
		}

		MarkMissionAssetPickedUp(state, asset);
		asset.m_sCarriedByVehicleId = carrierId;
		asset.m_bAttachedToCarrier = true;
		asset.m_vCurrentPosition = carrierPosition;
		asset.m_vLastKnownPosition = carrierPosition;
		asset.m_sLastInteraction = PHASE_LOADED;
		mission.m_sRuntimePhase = PHASE_LOADED;
		result = string.Format("h-istasi mission | loaded %1 into %2", BuildAssetShortLabel(asset), carrierName);
		eventType = "loaded";
		return true;
	}

	protected bool ApplyUnloadInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, int playerId, vector playerPosition, out string result, out string eventType)
	{
		asset.m_bPickedUp = false;
		asset.m_bAttachedToCarrier = false;
		asset.m_sCarriedByVehicleId = "";
		asset.m_vSourcePosition = HST_WorldPositionService.ResolveGroundPosition(playerPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		asset.m_vCurrentPosition = asset.m_vSourcePosition;
		asset.m_vLastKnownPosition = asset.m_vSourcePosition;
		asset.m_sLastInteraction = PHASE_UNLOADED;
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
			runtimeEntity.m_bRecovered = false;
		mission.m_sRuntimePhase = PHASE_UNLOADED;
		result = "h-istasi mission | unloaded " + BuildAssetShortLabel(asset);
		eventType = "unloaded";
		return true;
	}

	protected bool ApplyDeliverInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, vector playerPosition, out string result, out string eventType)
	{
		vector deliveryPosition = ResolveDeliveryPosition(asset, playerPosition);
		if (DistanceSq2D(playerPosition, deliveryPosition) > DEFAULT_DELIVERY_RADIUS_METERS * DEFAULT_DELIVERY_RADIUS_METERS)
		{
			result = "h-istasi mission | failed: bring " + BuildAssetShortLabel(asset) + " to the delivery zone";
			return false;
		}

		StopCaptiveFollowController(asset);
		MarkMissionAssetDelivered(state, asset, deliveryPosition);
		asset.m_bAttachedToCarrier = false;
		asset.m_sCarriedByVehicleId = "";
		asset.m_sLastInteraction = PHASE_DELIVERED;
		mission.m_sRuntimePhase = PHASE_DELIVERED;
		result = "h-istasi mission | delivered " + BuildAssetShortLabel(asset);
		eventType = "delivered";
		return true;
	}

	protected bool ApplyCaptiveExtractInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, int playerId, IEntity playerEntity, vector playerPosition, out string result, out string eventType)
	{
		if (!asset.m_bPickedUp)
		{
			asset.m_bPickedUp = true;
			asset.m_sCarriedByVehicleId = "";
			asset.m_bAttachedToCarrier = false;
			IEntity captiveEntity = GetRuntimeEntity(asset.m_sEntityId);
			vector captivePosition = playerPosition;
			if (captiveEntity)
				captivePosition = captiveEntity.GetOrigin();
			asset.m_vCurrentPosition = captivePosition;
			asset.m_vLastKnownPosition = captivePosition;
			asset.m_sLastInteraction = PHASE_FREED;
			HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtimeEntity)
			{
				runtimeEntity.m_bRecovered = false;
				runtimeEntity.m_vPosition = captivePosition;
			}
			mission.m_sRuntimePhase = PHASE_FREED;
			result = "h-istasi mission | freed " + BuildAssetShortLabel(asset) + "; order them to follow";
			eventType = "freed";
			return true;
		}

		if (!asset.m_bAttachedToCarrier || asset.m_sCarriedByVehicleId.IsEmpty())
		{
			result = "h-istasi mission | failed: order " + BuildAssetShortLabel(asset) + " to follow before extraction";
			return false;
		}

		if (IsEntityInVehicle(playerEntity))
		{
			result = "h-istasi mission | failed: dismount before extracting captives at HQ";
			return false;
		}

		return ApplyDeliverInteraction(state, mission, asset, playerPosition, result, eventType);
	}

	protected bool ApplyCaptiveFollowInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, int playerId, IEntity playerEntity, vector playerPosition, out string result, out string eventType)
	{
		if (!asset.m_bPickedUp)
		{
			result = "h-istasi mission | failed: free " + BuildAssetShortLabel(asset) + " first";
			return false;
		}
		if (asset.m_bDelivered || asset.m_bDestroyed)
		{
			result = "h-istasi mission | failed: captive is no longer available";
			return false;
		}

		int followed;
		foreach (HST_MissionAssetState candidate : state.m_aMissionAssets)
		{
			if (!candidate || candidate.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (candidate.m_sKind != ASSET_KIND_CAPTIVE || !candidate.m_bPickedUp || candidate.m_bDelivered || candidate.m_bDestroyed)
				continue;
			if (DistanceSq2D(playerPosition, ResolveInteractionValidationPosition(candidate, "mission_captive_follow")) > ResolveInteractionRadius(candidate, "mission_captive_follow") * ResolveInteractionRadius(candidate, "mission_captive_follow"))
				continue;

			MarkCaptiveFollowing(state, candidate, playerId, playerEntity);
			followed++;
		}

		if (followed <= 0)
		{
			MarkCaptiveFollowing(state, asset, playerId, playerEntity);
			followed = 1;
		}

		mission.m_sRuntimePhase = PHASE_EXTRACTING;
		if (followed == 1)
			result = "h-istasi mission | " + BuildAssetShortLabel(asset) + " is following you";
		else
			result = string.Format("h-istasi mission | %1 POWs are following you", followed);
		eventType = "following";
		return true;
	}

	protected void MarkCaptiveFollowing(HST_CampaignState state, HST_MissionAssetState asset, int playerId, IEntity playerEntity)
	{
		if (!state || !asset)
			return;

		vector position = asset.m_vCurrentPosition;
		IEntity captiveEntity = GetRuntimeEntity(asset.m_sEntityId);
		if (captiveEntity)
			position = captiveEntity.GetOrigin();

		asset.m_sCarriedByVehicleId = BuildPlayerCarrierId(playerId);
		asset.m_bAttachedToCarrier = true;
		asset.m_vCurrentPosition = position;
		asset.m_vLastKnownPosition = position;
		asset.m_sLastInteraction = PHASE_FOLLOWING;
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			runtimeEntity.m_bRecovered = false;
			runtimeEntity.m_vPosition = position;
		}

		StartCaptiveFollowController(asset, playerEntity);
	}

	protected bool StopCaptiveFollowing(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, vector stopPosition)
	{
		if (!state || !asset)
			return false;

		bool changed;
		IEntity captiveEntity = GetRuntimeEntity(asset.m_sEntityId);
		StopCaptiveFollowController(asset);
		IEntity captiveVehicle = ResolveEntityVehicle(captiveEntity);
		if (captiveVehicle)
		{
			stopPosition = captiveVehicle.GetOrigin();
			DeleteRuntimeEntity(asset.m_sEntityId);
			captiveEntity = null;
			changed = true;
		}

		if (IsZeroVector(stopPosition))
			stopPosition = ResolveCaptiveStopPosition(asset, captiveEntity);
		if (IsZeroVector(stopPosition))
			stopPosition = asset.m_vCurrentPosition;

		if (asset.m_bAttachedToCarrier)
		{
			asset.m_bAttachedToCarrier = false;
			changed = true;
		}
		if (!asset.m_sCarriedByVehicleId.IsEmpty())
		{
			asset.m_sCarriedByVehicleId = "";
			changed = true;
		}
		if (asset.m_sLastInteraction != PHASE_FREED)
		{
			asset.m_sLastInteraction = PHASE_FREED;
			changed = true;
		}
		if (!IsZeroVector(stopPosition) && DistanceSq2D(asset.m_vCurrentPosition, stopPosition) > 0.5)
		{
			asset.m_vCurrentPosition = stopPosition;
			asset.m_vLastKnownPosition = stopPosition;
			changed = true;
		}

		if (!captiveEntity && mission && !asset.m_bDelivered && !asset.m_bDestroyed && !IsZeroVector(stopPosition))
		{
			captiveEntity = SpawnCaptiveFollowerProjection(state, mission, asset, stopPosition, "0 0 0");
			if (captiveEntity)
				changed = true;
		}

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			if (!IsZeroVector(stopPosition))
				runtimeEntity.m_vPosition = stopPosition;
			runtimeEntity.m_bRecovered = false;
			runtimeEntity.m_bSpawned = captiveEntity != null;
		}

		return changed;
	}

	protected vector ResolveCaptiveStopPosition(HST_MissionAssetState asset, IEntity captiveEntity)
	{
		if (captiveEntity)
		{
			IEntity captiveVehicle = ResolveEntityVehicle(captiveEntity);
			if (captiveVehicle)
				return captiveVehicle.GetOrigin();

			return captiveEntity.GetOrigin();
		}

		if (asset && !IsZeroVector(asset.m_vLastKnownPosition))
			return asset.m_vLastKnownPosition;
		if (asset)
			return asset.m_vCurrentPosition;

		return "0 0 0";
	}

	protected bool StartCaptiveFollowController(HST_MissionAssetState asset, IEntity targetEntity)
	{
		if (!asset || !targetEntity)
			return false;

		IEntity captiveEntity = GetRuntimeEntity(asset.m_sEntityId);
		if (!captiveEntity)
			return false;

		HST_MissionCaptiveFollowComponent follow = HST_MissionCaptiveFollowComponent.Cast(captiveEntity.FindComponent(HST_MissionCaptiveFollowComponent));
		if (!follow)
			return false;

		follow.StartFollowing(targetEntity);
		return true;
	}

	protected void StopCaptiveFollowController(HST_MissionAssetState asset)
	{
		if (!asset)
			return;

		IEntity captiveEntity = GetRuntimeEntity(asset.m_sEntityId);
		if (!captiveEntity)
			return;

		HST_MissionCaptiveFollowComponent follow = HST_MissionCaptiveFollowComponent.Cast(captiveEntity.FindComponent(HST_MissionCaptiveFollowComponent));
		if (follow)
			follow.StopFollowing();
	}

	protected bool ApplyVehicleCaptureInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ArsenalService arsenal, vector playerPosition, out string result, out string eventType)
	{
		if (asset.m_sRole == ROLE_CONVOY_VEHICLE && HasLivingConvoyCrewForVehicle(state, mission, asset))
		{
			result = "h-istasi mission | failed: neutralize the convoy crew before capturing this vehicle";
			return false;
		}

		asset.m_bPickedUp = true;
		asset.m_bDelivered = true;
		asset.m_bAttachedToCarrier = false;
		asset.m_vCurrentPosition = playerPosition;
		asset.m_vLastKnownPosition = playerPosition;
		asset.m_sLastInteraction = PHASE_CAPTURED;
		DeleteRuntimeEntity(asset.m_sEntityId);
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			runtimeEntity.m_bRecovered = true;
			runtimeEntity.m_vPosition = playerPosition;
		}

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(asset.m_sEntityId);
		if (runtimeVehicle)
		{
			runtimeVehicle.m_bDeleted = true;
			runtimeVehicle.m_vPosition = playerPosition;
		}

		if (arsenal && !asset.m_sPrefab.IsEmpty())
		{
			HST_GarageVehicleState vehicle = new HST_GarageVehicleState();
			vehicle.m_sVehicleId = "mission_vehicle_" + asset.m_sAssetId;
			vehicle.m_sPrefab = asset.m_sPrefab;
			vehicle.m_sDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(asset.m_sPrefab);
			vehicle.m_sSourceZoneId = mission.m_sTargetZoneId;
			vehicle.m_vPosition = playerPosition;
			vehicle.m_vAngles = "0 0 0";
			vehicle.m_fFuel = 1.0;
			vehicle.m_sDamageState = "captured";
			vehicle.m_bUnlocked = true;
			arsenal.StoreVehicle(state, vehicle);
		}

		if (ShouldKeepConvoyContactPhase(mission, asset))
			mission.m_sRuntimePhase = PHASE_CONVOY_CONTACT;
		else if (mission && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
			mission.m_sRuntimePhase = PHASE_CAPTURED;
		result = "h-istasi mission | captured " + BuildAssetShortLabel(asset);
		eventType = "captured";
		return true;
	}

	protected bool IsPostCompletionConvoyInteractionAllowed(HST_ActiveMissionState mission, HST_MissionAssetState asset, string commandId)
	{
		if (!IsPreservedConvoyAssetAfterCrewElimination(mission, asset))
			return false;

		if (asset.m_sRole == ROLE_CONVOY_VEHICLE)
			return commandId == "mission_vehicle_capture";
		if (asset.m_sRole == ROLE_CONVOY_PAYLOAD)
			return commandId == "mission_asset_load" || commandId == "mission_asset_deliver" || commandId == "mission_asset_unload";
		if (asset.m_sRole == ROLE_CONVOY_CAPTIVE)
			return commandId == "mission_captive_extract" || commandId == "mission_captive_follow";

		return false;
	}

	protected bool IsPreservedConvoyVehicleAfterCrewElimination(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		return IsPreservedConvoyAssetAfterCrewElimination(mission, asset) && asset.m_sRole == ROLE_CONVOY_VEHICLE;
	}

	protected bool IsPreservedConvoyAssetAfterCrewElimination(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!mission || !asset)
			return false;
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_SUCCEEDED)
			return false;
		if (mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT || !HasConvoyCrewEliminatedForPostCompletion(mission))
			return false;
		if (asset.m_sMissionInstanceId != mission.m_sInstanceId)
			return false;

		return !asset.m_bDestroyed && !asset.m_bDelivered;
	}

	protected bool HasConvoyCrewEliminatedForPostCompletion(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;
		if (mission.m_bConvoyCrewEliminatedOutcomeApplied)
			return true;

		return IsConvoyCrewEliminationCompletionEvent(mission.m_sLastRuntimeEventKey);
	}

	protected bool IsConvoyPayloadOrCaptiveAsset(HST_MissionAssetState asset)
	{
		if (!asset)
			return false;

		return asset.m_sRole == ROLE_CONVOY_PAYLOAD || asset.m_sRole == ROLE_CONVOY_CAPTIVE;
	}

	protected bool IsConvoyCrewEliminationCompletionEvent(string eventKey)
	{
		return eventKey == PHASE_CONVOY_ELIMINATED || eventKey == "convoy_complete" || eventKey == "convoy_secured_sent";
	}

	protected bool ShouldKeepConvoyContactPhase(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!mission || !asset || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT || asset.m_sRole != ROLE_CONVOY_VEHICLE)
			return false;
		if (mission.m_sRuntimePhase == PHASE_FAILED || mission.m_sRuntimePhase == PHASE_CONVOY_ELIMINATED)
			return false;

		return true;
	}

	protected bool HasLivingConvoyCrewForVehicle(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState vehicleAsset)
	{
		if (!state || !mission || !vehicleAsset || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;

		int vehicleIndex = ResolveMissionConvoyVehicleIndex(state, mission, vehicleAsset);
		if (vehicleIndex < 0)
			return false;

		string groupId = string.Format("mission_convoy_%1_%2", mission.m_sInstanceId, vehicleIndex);
		HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
		if (!activeGroup)
			return false;

		if (activeGroup.m_sRuntimeStatus == "convoy_eliminated" || activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "folded" || activeGroup.m_sRuntimeStatus == "spawn_failed")
			return false;

		return activeGroup.m_iSurvivorInfantryCount > 0 || activeGroup.m_iLastSeenAliveCount > 0;
	}

	protected bool HasLivingConvoyCrewForMission(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;

		string groupPrefix = string.Format("%1%2_", MISSION_CONVOY_GROUP_PREFIX, mission.m_sInstanceId);
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || !activeGroup.m_sGroupId.Contains(groupPrefix))
				continue;
			if (activeGroup.m_sRuntimeStatus == "convoy_eliminated" || activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "folded" || activeGroup.m_sRuntimeStatus == "spawn_failed")
				continue;
			if (activeGroup.m_iSurvivorInfantryCount > 0 || activeGroup.m_iLastSeenAliveCount > 0)
				return true;
		}

		return false;
	}

	protected int ResolveMissionConvoyVehicleIndex(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState vehicleAsset)
	{
		if (!state || !mission || !vehicleAsset)
			return -1;

		int index;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != ROLE_CONVOY_VEHICLE)
				continue;
			if (asset == vehicleAsset || asset.m_sAssetId == vehicleAsset.m_sAssetId)
				return index;

			index++;
		}

		return -1;
	}

	protected bool ApplySabotageInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, vector playerPosition, out string result, out string eventType)
	{
		MarkMissionAssetDestroyed(state, mission, asset, playerPosition);
		if (ShouldKeepConvoyContactPhase(mission, asset))
			mission.m_sRuntimePhase = PHASE_CONVOY_CONTACT;
		else
			mission.m_sRuntimePhase = PHASE_DESTROYED;
		result = "h-istasi mission | sabotaged " + BuildAssetShortLabel(asset);
		eventType = "sabotaged";
		return true;
	}

	protected void MarkMissionAssetDestroyed(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, vector position)
	{
		if (!state || !asset)
			return;

		asset.m_bDestroyed = true;
		asset.m_bAlive = false;
		asset.m_bAttachedToCarrier = false;
		asset.m_sCarriedByVehicleId = "";
		if (!IsZeroVector(position))
		{
			asset.m_vCurrentPosition = position;
			asset.m_vLastKnownPosition = position;
		}
		asset.m_sLastInteraction = PHASE_DESTROYED;
		DeleteRuntimeEntity(asset.m_sEntityId);
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			runtimeEntity.m_bDestroyed = true;
			runtimeEntity.m_vPosition = asset.m_vCurrentPosition;
		}

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(asset.m_sEntityId);
		if (runtimeVehicle)
		{
			runtimeVehicle.m_bDeleted = true;
			runtimeVehicle.m_vPosition = asset.m_vCurrentPosition;
		}

		if (mission)
		{
			if (ShouldKeepConvoyContactPhase(mission, asset))
				mission.m_sRuntimePhase = PHASE_CONVOY_CONTACT;
			else if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				mission.m_sRuntimePhase = PHASE_DESTROYED;
		}
	}

	protected vector ResolveInteractionValidationPosition(HST_MissionAssetState asset, string commandId)
	{
		if (!asset)
			return "0 0 0";

		if (commandId == "mission_asset_deliver")
			return ResolveDeliveryPosition(asset, asset.m_vCurrentPosition);

		if (commandId == "mission_captive_extract" && asset.m_bPickedUp)
			return ResolveDeliveryPosition(asset, asset.m_vCurrentPosition);

		if (asset.m_bPickedUp && !IsZeroVector(asset.m_vCurrentPosition))
			return asset.m_vCurrentPosition;

		if (!IsZeroVector(asset.m_vCurrentPosition))
			return asset.m_vCurrentPosition;

		return asset.m_vSourcePosition;
	}

	protected bool TryResolveConvoyAssetAccessPosition(HST_CampaignState state, HST_MissionAssetState asset, vector playerPosition, out vector accessPosition)
	{
		accessPosition = "0 0 0";
		if (!state || !IsConvoyPayloadOrCaptiveAsset(asset) || asset.m_bPickedUp || asset.m_bDelivered || asset.m_bDestroyed)
			return false;

		HST_ActiveMissionState mission = state.FindActiveMission(asset.m_sMissionInstanceId);
		if (!mission || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;
		if (HasLivingConvoyCrewForMission(state, mission))
			return false;

		bool found;
		float bestDistanceSq = 999999999.0;
		foreach (HST_MissionAssetState vehicleAsset : state.m_aMissionAssets)
		{
			if (!vehicleAsset || vehicleAsset.m_sMissionInstanceId != asset.m_sMissionInstanceId || vehicleAsset.m_sRole != ROLE_CONVOY_VEHICLE)
				continue;
			if (vehicleAsset.m_bDestroyed || vehicleAsset.m_bDelivered)
				continue;

			vector vehiclePosition = vehicleAsset.m_vCurrentPosition;
			if (IsZeroVector(vehiclePosition))
				vehiclePosition = vehicleAsset.m_vLastKnownPosition;
			if (IsZeroVector(vehiclePosition))
				vehiclePosition = vehicleAsset.m_vSourcePosition;
			if (IsZeroVector(vehiclePosition))
				continue;

			float distanceSq = DistanceSq2D(playerPosition, vehiclePosition);
			if (found && distanceSq >= bestDistanceSq)
				continue;

			found = true;
			bestDistanceSq = distanceSq;
			accessPosition = vehiclePosition;
		}

		return found;
	}

	protected vector ResolveDeliveryPosition(HST_MissionAssetState asset, vector fallbackPosition)
	{
		if (asset && !IsZeroVector(asset.m_vTargetPosition))
			return asset.m_vTargetPosition;

		return fallbackPosition;
	}

	protected float ResolveInteractionRadius(HST_MissionAssetState asset, string commandId)
	{
		if (commandId == "mission_asset_deliver" || (commandId == "mission_captive_extract" && asset && asset.m_bPickedUp))
			return DEFAULT_DELIVERY_RADIUS_METERS;

		if (asset && asset.m_iInteractionRadiusMeters > 0)
			return asset.m_iInteractionRadiusMeters;

		return DEFAULT_ASSET_INTERACTION_RADIUS_METERS;
	}

	protected bool IsTransportableAsset(HST_MissionAssetState asset)
	{
		if (!asset)
			return false;

		return asset.m_sKind == ASSET_KIND_CARGO || asset.m_sKind == ASSET_KIND_CAPTIVE;
	}

	protected bool CanCarrierAcceptAsset(HST_CampaignState state, int playerId, HST_MissionAssetState asset)
	{
		string carrierId = BuildPlayerCarrierId(playerId);
		int usedCapacity;
		foreach (HST_MissionAssetState carriedAsset : state.m_aMissionAssets)
		{
			if (!carriedAsset || carriedAsset.m_bDelivered || carriedAsset.m_bDestroyed || carriedAsset.m_sCarriedByVehicleId != carrierId)
				continue;

			usedCapacity += Math.Max(1, carriedAsset.m_iCargoCapacityCost);
		}

		return usedCapacity + Math.Max(1, asset.m_iCargoCapacityCost) <= DEFAULT_MISSION_CARRIER_CAPACITY;
	}

	protected bool TryResolveMissionAssetCarrier(HST_CampaignState state, HST_MissionAssetState asset, int playerId, IEntity playerEntity, vector playerPosition, out string carrierId, out string carrierName, out vector carrierPosition, out string failureReason)
	{
		carrierId = "";
		carrierName = "";
		carrierPosition = playerPosition;
		failureReason = "";
		if (!asset)
		{
			failureReason = "mission asset missing";
			return false;
		}

		IEntity vehicle = FindNearestMissionVehicleCarrier(state, playerEntity, VEHICLE_CARRIER_RADIUS_METERS, carrierId, carrierName);
		if (vehicle)
		{
			carrierPosition = vehicle.GetOrigin();
			EnsureMissionCarrierVehicleRecord(state, vehicle, carrierId, carrierName);
			return true;
		}

		failureReason = "no vehicle nearby";
		return false;
	}

	protected IEntity FindNearestMissionVehicleCarrier(HST_CampaignState state, IEntity playerEntity, float radiusMeters, out string carrierId, out string carrierName)
	{
		carrierId = "";
		carrierName = "";
		if (!playerEntity)
			return null;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		m_aMissionVehicleScanEntities.Clear();
		world.QueryEntitiesBySphere(playerEntity.GetOrigin(), radiusMeters, AddMissionVehicleScanCandidate, null, EQueryEntitiesFlags.ALL);

		float bestDistanceSq = 999999999.0;
		IEntity bestVehicle;
		string bestId;
		string bestName;
		foreach (IEntity candidate : m_aMissionVehicleScanEntities)
		{
			if (!candidate || candidate == playerEntity)
				continue;

			string prefab;
			IEntity root = ResolveMissionVehicleRoot(candidate, prefab);
			if (!root || root == playerEntity)
				continue;

			float distanceSq = DistanceSq2D(playerEntity.GetOrigin(), root.GetOrigin());
			if (distanceSq >= bestDistanceSq)
				continue;

			bestDistanceSq = distanceSq;
			bestVehicle = root;
			bestId = ResolveEntityRuntimeId(root);
			bestName = HST_DisplayNameService.ResolveVehicleDisplayName(prefab, root.GetName());
		}

		if (!bestVehicle || bestId.IsEmpty())
			return null;

		carrierId = bestId;
		carrierName = bestName;
		if (carrierName.IsEmpty())
			carrierName = "vehicle";
		return bestVehicle;
	}

	protected bool AddMissionVehicleScanCandidate(IEntity entity)
	{
		if (entity && m_aMissionVehicleScanEntities.Count() < MAX_MISSION_VEHICLE_SCAN_ENTITIES)
			m_aMissionVehicleScanEntities.Insert(entity);

		return m_aMissionVehicleScanEntities.Count() < MAX_MISSION_VEHICLE_SCAN_ENTITIES;
	}

	protected IEntity ResolveMissionVehicleRoot(IEntity entity, out string prefab)
	{
		prefab = "";
		IEntity cursor = entity;
		while (cursor)
		{
			prefab = ResolveEntityPrefabName(cursor);
			if (!prefab.IsEmpty() && HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
				return cursor;

			if (HST_VehicleRootPolicy.IsKnownVehicleRootName(cursor.GetName()) && !HST_VehicleRootPolicy.IsVehiclePartName(cursor.GetName()))
				return cursor;

			cursor = cursor.GetParent();
		}

		return null;
	}

	protected void EnsureMissionCarrierVehicleRecord(HST_CampaignState state, IEntity vehicle, string vehicleId, string vehicleName)
	{
		if (!state || !vehicle || vehicleId.IsEmpty())
			return;

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(vehicleId);
		if (!runtimeVehicle)
		{
			runtimeVehicle = new HST_RuntimeVehicleState();
			runtimeVehicle.m_sVehicleRuntimeId = vehicleId;
			state.m_aRuntimeVehicles.Insert(runtimeVehicle);
		}

		string prefab = ResolveEntityPrefabName(vehicle);
		runtimeVehicle.m_sPrefab = prefab;
		runtimeVehicle.m_sDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(prefab, vehicleName);
		runtimeVehicle.m_sRuntimeKind = "mission_carrier";
		runtimeVehicle.m_vPosition = vehicle.GetOrigin();
		runtimeVehicle.m_vAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(vehicle.GetYawPitchRoll());
		runtimeVehicle.m_bDeleted = false;
	}

	protected string BuildPlayerCarrierId(int playerId)
	{
		return string.Format("player_%1_mission_carrier", playerId);
	}

	protected bool IsPlayerCarrierId(string carrierId)
	{
		return !carrierId.IsEmpty() && carrierId.Contains("_mission_carrier") && carrierId.Contains("player_");
	}

	protected bool IsPlayerCarrier(HST_MissionAssetState asset, int playerId)
	{
		if (!asset || asset.m_sCarriedByVehicleId.IsEmpty())
			return false;

		return asset.m_sCarriedByVehicleId == BuildPlayerCarrierId(playerId);
	}

	protected string BuildAssetShortLabel(HST_MissionAssetState asset)
	{
		if (!asset)
			return "mission asset";

		string label = asset.m_sRole;
		if (label.IsEmpty())
			label = asset.m_sKind;
		if (label.IsEmpty())
			label = asset.m_sAssetId;
		label.Replace("_", " ");
		return label;
	}

	protected int ResolveAssetCapacityCost(string kind, string role)
	{
		if (kind == ASSET_KIND_CAPTIVE)
			return 2;
		if (kind == ASSET_KIND_VEHICLE)
			return DEFAULT_MISSION_CARRIER_CAPACITY;
		if (role == ROLE_CITY_SUPPLIES || role == ROLE_CONVOY_PAYLOAD)
			return 2;

		return 1;
	}

	protected int ResolveAssetInteractionRadius(string kind, string role)
	{
		if (kind == ASSET_KIND_VEHICLE)
			return 28;
		if (kind == ASSET_KIND_TARGET || kind == ASSET_KIND_CHARACTER)
			return 22;

		return DEFAULT_ASSET_INTERACTION_RADIUS_METERS;
	}

	protected void RefreshMissionObjectivesFromAssets(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete || objective.m_bFailed)
				continue;

			string role = ResolveObjectiveAssetRole(mission, objective);
			if (role.IsEmpty())
				continue;

			int satisfied = CountSatisfiedAssetsForObjective(state, mission, objective, role);
			objective.m_iCurrentCount = Math.Min(Math.Max(1, objective.m_iRequiredCount), satisfied);
			objective.m_iCurrentProgress = Math.Min(objective.m_iRequiredProgress, satisfied);
			if (satisfied >= Math.Max(1, objective.m_iRequiredCount))
				CompleteWorldObjective(objective);
		}
	}

	protected bool AreMissionObjectivesComplete(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		if (HasPendingRequiredConvoyOutcome(state, mission))
			return false;

		bool found;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			found = true;
			if (!objective.m_bComplete || objective.m_bFailed)
				return false;
		}

		return found;
	}

	protected bool HasPendingRequiredConvoyOutcome(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;

		if (mission.m_sMissionId == "convoy_money")
			return HasPendingConvoyAssetOutcome(state, mission, ROLE_CONVOY_PAYLOAD);
		if (mission.m_sMissionId == "convoy_supplies")
			return !mission.m_bConvoyCrewEliminatedOutcomeApplied && HasPendingConvoyAssetOutcome(state, mission, ROLE_CONVOY_PAYLOAD);
		if (mission.m_sMissionId == "convoy_prisoners")
			return HasPendingConvoyAssetOutcome(state, mission, ROLE_CONVOY_CAPTIVE);
		if (mission.m_sMissionId == "convoy_ammo" || mission.m_sMissionId == "convoy_armored")
			return HasPendingConvoyVehicleCaptureOutcome(state, mission);

		return false;
	}

	protected bool HasPendingConvoyAssetOutcome(HST_CampaignState state, HST_ActiveMissionState mission, string role)
	{
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != role)
				continue;

			if (!asset.m_bDelivered || !asset.m_bOutcomeApplied)
				return true;
		}

		return false;
	}

	protected bool HasPendingConvoyVehicleCaptureOutcome(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (mission.m_bConvoyVehicleCapturedOutcomeApplied)
			return false;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != ROLE_CONVOY_VEHICLE)
				continue;
			if (!asset.m_bDestroyed && !asset.m_bDelivered)
				return true;
		}

		return false;
	}

	protected int CountSatisfiedAssetsForObjective(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, string role)
	{
		int satisfied;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != role)
				continue;

			if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO)
			{
				if (asset.m_bDelivered)
					satisfied++;
				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET)
			{
				if (asset.m_bDestroyed || (asset.m_sKind == ASSET_KIND_VEHICLE && asset.m_bDelivered))
					satisfied++;
				continue;
			}

			if (objective.m_sTargetId == "convoy")
			{
				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT)
			{
				if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO)
				{
					if (asset.m_bDelivered)
						satisfied++;
					continue;
				}

				if (asset.m_bPickedUp || asset.m_bDelivered)
					satisfied++;
				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
			{
				if (asset.m_bDelivered)
					satisfied++;
				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES)
			{
				if (asset.m_bDelivered)
					satisfied++;
				continue;
			}
		}

		return satisfied;
	}

	protected bool HasSatisfiedConvoyPayload(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;

		bool hasPayload;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			if (asset.m_sRole != ROLE_CONVOY_PAYLOAD && asset.m_sRole != ROLE_CONVOY_CAPTIVE)
				continue;

			hasPayload = true;
			if (asset.m_sRole == ROLE_CONVOY_CAPTIVE)
			{
				if (!asset.m_bDelivered)
					return false;
			}
			else if (!asset.m_bPickedUp && !asset.m_bDelivered)
			{
				return false;
			}
		}

		return hasPayload;
	}

	protected void UpdateMissionCountersFromAssets(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;

		int picked;
		int delivered;
		int captives;
		int vehicles;
		int destroyed;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			if (asset.m_bPickedUp)
				picked++;
			if (asset.m_bDelivered)
				delivered++;
			if (asset.m_bDestroyed)
				destroyed++;
			if (asset.m_sKind == ASSET_KIND_CAPTIVE && asset.m_bDelivered)
				captives++;
			if (asset.m_sKind == ASSET_KIND_VEHICLE && asset.m_bDelivered)
				vehicles++;
		}

		mission.m_iRuntimePickupCount = picked;
		mission.m_iRuntimeDeliveryCount = delivered;
		mission.m_iRuntimeDestroyedCount = destroyed;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO || mission.m_sRuntimePrimitive == PRIMITIVE_DELIVER_SUPPLIES)
			mission.m_iRecoveredCargoCount = Math.Min(mission.m_iRequiredCargoCount, delivered);
		else
			mission.m_iRecoveredCargoCount = Math.Min(mission.m_iRequiredCargoCount, picked);
		mission.m_iExtractedCaptiveCount = Math.Min(mission.m_iRequiredCaptiveCount, captives);
		mission.m_iCapturedVehicleCount = Math.Min(mission.m_iRequiredVehicleCount, vehicles);
	}

	protected bool SyncMissionAssetRuntimePositions(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		bool changed;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_bDelivered || asset.m_bDestroyed)
				continue;

			vector position;
			bool resolved;
			IEntity entity = GetRuntimeEntity(asset.m_sEntityId);
			if (entity)
			{
				position = entity.GetOrigin();
				resolved = true;
			}
			else if (asset.m_bPickedUp && !asset.m_sCarriedByVehicleId.IsEmpty())
			{
				if (asset.m_sKind == ASSET_KIND_CAPTIVE && asset.m_sLastInteraction == PHASE_LOADED && IsPlayerCarrierId(asset.m_sCarriedByVehicleId))
				{
					IEntity playerEntity = ResolvePlayerEntityForCarrierId(asset.m_sCarriedByVehicleId);
					if (!ResolveEntityVehicle(playerEntity))
					{
						if (!IsZeroVector(asset.m_vLastKnownPosition))
							position = asset.m_vLastKnownPosition;
						else
							position = asset.m_vCurrentPosition;
						resolved = true;
					}
				}

				if (!resolved)
					resolved = TryResolveCarrierPosition(state, asset.m_sCarriedByVehicleId, asset.m_vCurrentPosition, position);
				if (!resolved && !IsZeroVector(asset.m_vLastKnownPosition))
				{
					position = asset.m_vLastKnownPosition;
					resolved = true;
				}
			}

			if (!resolved || IsZeroVector(position) || DistanceSq2D(asset.m_vCurrentPosition, position) <= 1.0)
				continue;

			asset.m_vCurrentPosition = position;
			asset.m_vLastKnownPosition = position;
			HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtimeEntity)
				runtimeEntity.m_vPosition = position;
			HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(asset.m_sEntityId);
			if (runtimeVehicle)
				runtimeVehicle.m_vPosition = position;
			changed = true;
		}

		return changed;
	}

	protected bool UpdateFollowingCaptives(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		bool changed;
		int followIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (asset.m_sKind != ASSET_KIND_CAPTIVE || !asset.m_bPickedUp || asset.m_bDelivered || asset.m_bDestroyed)
				continue;
			if (!asset.m_bAttachedToCarrier || asset.m_sCarriedByVehicleId.IsEmpty() || !IsPlayerCarrierId(asset.m_sCarriedByVehicleId))
				continue;

			IEntity playerEntity = ResolvePlayerEntityForCarrierId(asset.m_sCarriedByVehicleId);
			HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			IEntity captiveEntity = GetRuntimeEntity(asset.m_sEntityId);
			if (!IsLivingPlayerEntity(playerEntity))
			{
				changed = StopCaptiveFollowing(state, mission, asset, ResolveCaptiveStopPosition(asset, captiveEntity)) || changed;
				continue;
			}

			vector playerPosition = playerEntity.GetOrigin();
			IEntity playerVehicle = ResolveEntityVehicle(playerEntity);
			if (!playerVehicle)
			{
				vector captiveRangePosition = ResolveCaptiveStopPosition(asset, captiveEntity);
				if (!IsZeroVector(captiveRangePosition) && DistanceSq2D(captiveRangePosition, playerPosition) > CAPTIVE_FOLLOW_BREAK_DISTANCE_METERS * CAPTIVE_FOLLOW_BREAK_DISTANCE_METERS)
				{
					changed = StopCaptiveFollowing(state, mission, asset, captiveRangePosition) || changed;
					continue;
				}
			}

			if (playerVehicle)
			{
				vector vehiclePosition = playerVehicle.GetOrigin();
				if (!captiveEntity)
				{
					captiveEntity = SpawnCaptiveFollowerProjection(state, mission, asset, BuildCaptiveFollowPosition(vehiclePosition, followIndex), "0 0 0");
					runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
					if (captiveEntity)
						changed = true;
				}

				string seatReason;
				if (captiveEntity)
				{
					if (TryMoveCaptiveIntoVehicle(captiveEntity, playerVehicle, seatReason))
					{
						StopCaptiveFollowController(asset);
						asset.m_bSpawned = true;
						if (DistanceSq2D(asset.m_vCurrentPosition, vehiclePosition) > 0.5)
							changed = true;
						asset.m_vCurrentPosition = vehiclePosition;
						asset.m_vLastKnownPosition = vehiclePosition;
						if (runtimeEntity)
						{
							runtimeEntity.m_bSpawned = true;
							runtimeEntity.m_bRecovered = false;
							runtimeEntity.m_vPosition = vehiclePosition;
						}
					}
					else
					{
						vector vehicleFollowPosition = BuildCaptiveFollowPosition(vehiclePosition, followIndex);
						if (!StartCaptiveFollowController(asset, playerVehicle))
							TryIssueCaptiveFollowWaypoint(state, asset, captiveEntity, vehicleFollowPosition);
						vector captivePosition = captiveEntity.GetOrigin();
						if (DistanceSq2D(asset.m_vCurrentPosition, captivePosition) > 0.5)
							changed = true;
						asset.m_vCurrentPosition = captivePosition;
						asset.m_vLastKnownPosition = captivePosition;
						if (runtimeEntity)
						{
							runtimeEntity.m_bSpawned = true;
							runtimeEntity.m_bRecovered = false;
							runtimeEntity.m_vPosition = captivePosition;
						}
					}
				}

				if (asset.m_sLastInteraction != PHASE_LOADED)
				{
					asset.m_sLastInteraction = PHASE_LOADED;
					changed = true;
				}
				followIndex++;
				continue;
			}

			vector followPosition = BuildCaptiveFollowPosition(playerPosition, followIndex);
			followIndex++;
			if (!captiveEntity)
			{
				if (asset.m_sLastInteraction == PHASE_LOADED && DistanceSq2D(asset.m_vCurrentPosition, playerPosition) > CAPTIVE_DISEMBARK_RADIUS_METERS * CAPTIVE_DISEMBARK_RADIUS_METERS)
				{
					if (asset.m_bSpawned)
					{
						asset.m_bSpawned = false;
						changed = true;
					}
					if (runtimeEntity && runtimeEntity.m_bSpawned)
					{
						runtimeEntity.m_bSpawned = false;
						changed = true;
					}
					continue;
				}

				captiveEntity = SpawnCaptiveFollowerProjection(state, mission, asset, followPosition, "0 0 0");
				runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
				if (captiveEntity)
					changed = true;
			}

			if (!captiveEntity)
			{
				if (asset.m_bSpawned)
				{
					asset.m_bSpawned = false;
					changed = true;
				}
				if (runtimeEntity && runtimeEntity.m_bSpawned)
				{
					runtimeEntity.m_bSpawned = false;
					changed = true;
				}
				continue;
			}

			IEntity captiveVehicle = ResolveEntityVehicle(captiveEntity);
			if (captiveVehicle)
			{
				vector captiveVehiclePosition = captiveVehicle.GetOrigin();
				if (DistanceSq2D(playerPosition, captiveVehiclePosition) > CAPTIVE_DISEMBARK_RADIUS_METERS * CAPTIVE_DISEMBARK_RADIUS_METERS)
				{
					if (asset.m_sLastInteraction != PHASE_LOADED)
					{
						asset.m_sLastInteraction = PHASE_LOADED;
						changed = true;
					}
					if (DistanceSq2D(asset.m_vCurrentPosition, captiveVehiclePosition) > 0.5)
					{
						asset.m_vCurrentPosition = captiveVehiclePosition;
						asset.m_vLastKnownPosition = captiveVehiclePosition;
						changed = true;
					}
					if (runtimeEntity)
					{
						runtimeEntity.m_bSpawned = true;
						runtimeEntity.m_bRecovered = false;
						runtimeEntity.m_vPosition = captiveVehiclePosition;
					}
					continue;
				}

				StopCaptiveFollowController(asset);
				DeleteRuntimeEntity(asset.m_sEntityId);
				captiveEntity = SpawnCaptiveFollowerProjection(state, mission, asset, followPosition, "0 0 0");
				runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
				if (!captiveEntity)
					continue;
				changed = true;
			}

			if (asset.m_sLastInteraction != PHASE_FOLLOWING)
			{
				asset.m_sLastInteraction = PHASE_FOLLOWING;
				changed = true;
			}

			vector captivePosition = captiveEntity.GetOrigin();
			float followDistanceSq = DistanceSq2D(captivePosition, followPosition);
			if (followDistanceSq > CAPTIVE_FOLLOW_NEAR_DISTANCE_METERS * CAPTIVE_FOLLOW_NEAR_DISTANCE_METERS)
			{
				if (!StartCaptiveFollowController(asset, playerEntity))
					TryIssueCaptiveFollowWaypoint(state, asset, captiveEntity, followPosition);
			}

			if (DistanceSq2D(asset.m_vCurrentPosition, captivePosition) > 0.5)
			{
				asset.m_vCurrentPosition = captivePosition;
				asset.m_vLastKnownPosition = captivePosition;
				changed = true;
			}
			if (runtimeEntity)
			{
				runtimeEntity.m_bSpawned = true;
				runtimeEntity.m_bRecovered = false;
				runtimeEntity.m_vPosition = captivePosition;
			}
		}

		return changed;
	}

	protected vector BuildCaptiveFollowPosition(vector playerPosition, int followIndex)
	{
		vector offset = "0 0 0";
		if (followIndex == 0)
			offset[0] = 1.5;
		else if (followIndex == 1)
			offset[0] = -1.5;
		else
			offset[0] = 0.0;
		offset[2] = -2.0 - (followIndex * 0.75);

		return HST_WorldPositionService.ResolveSafeGroundPosition(playerPosition + offset, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 1.0);
	}

	protected IEntity SpawnCaptiveFollowerProjection(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, vector position, vector angles)
	{
		if (!state || !mission || !asset || asset.m_sEntityId.IsEmpty())
			return null;

		IEntity existing = GetRuntimeEntity(asset.m_sEntityId);
		if (existing)
			return existing;

		string prefab = asset.m_sPrefab;
		if (prefab.IsEmpty())
			prefab = PROP_CAPTIVES;

		vector spawnPosition = HST_WorldPositionService.ResolveSafeGroundPosition(position, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
		GenericEntity entity = HST_WorldPositionService.SpawnPrefab(prefab, spawnPosition, angles);
		if (!entity && prefab != PROP_CAPTIVES)
			entity = HST_WorldPositionService.SpawnPrefab(PROP_CAPTIVES, spawnPosition, angles);
		if (!entity)
			return null;

		ApplyMissionAssetIdentity(entity, asset);
		HST_MissionAssetComponent assetComponent = HST_MissionAssetComponent.Cast(entity.FindComponent(HST_MissionAssetComponent));
		if (assetComponent)
			assetComponent.ConfigureMissionAsset(asset.m_sAssetId, mission.m_sInstanceId, asset.m_sRole);

		asset.m_bSpawned = true;
		asset.m_vCurrentPosition = spawnPosition;
		asset.m_vLastKnownPosition = spawnPosition;
		m_aRuntimeEntityIds.Insert(asset.m_sEntityId);
		m_aRuntimeEntities.Insert(entity);
		RegisterAssetRuntimeEntityState(state, asset, spawnPosition, angles);
		return entity;
	}

	protected bool TryIssueCaptiveFollowWaypoint(HST_CampaignState state, HST_MissionAssetState asset, IEntity captiveEntity, vector followPosition)
	{
		if (!state || !asset || !captiveEntity)
			return false;

		if (!ShouldIssueCaptiveFollowWaypoint(state, asset))
			return false;

		AIGroup group = ResolveCaptiveAIGroup(captiveEntity);
		if (!group)
			return false;

		GenericEntity waypointEntity = HST_WorldPositionService.SpawnPrefab(CAPTIVE_FOLLOW_WAYPOINT_PREFAB, followPosition, "0 0 0");
		AIWaypoint waypoint = AIWaypoint.Cast(waypointEntity);
		if (!waypoint)
		{
			if (waypointEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
			return false;
		}

		group.AddWaypoint(waypoint);
		return true;
	}

	protected bool ShouldIssueCaptiveFollowWaypoint(HST_CampaignState state, HST_MissionAssetState asset)
	{
		if (!state || !asset || asset.m_sAssetId.IsEmpty())
			return false;

		int index = m_aCaptiveFollowWaypointAssetIds.Find(asset.m_sAssetId);
		if (index >= 0 && index < m_aCaptiveFollowWaypointSeconds.Count())
		{
			if (state.m_iElapsedSeconds - m_aCaptiveFollowWaypointSeconds[index] < CAPTIVE_FOLLOW_WAYPOINT_INTERVAL_SECONDS)
				return false;

			m_aCaptiveFollowWaypointSeconds[index] = state.m_iElapsedSeconds;
			return true;
		}

		m_aCaptiveFollowWaypointAssetIds.Insert(asset.m_sAssetId);
		m_aCaptiveFollowWaypointSeconds.Insert(state.m_iElapsedSeconds);
		return true;
	}

	protected AIGroup ResolveCaptiveAIGroup(IEntity captiveEntity)
	{
		if (!captiveEntity)
			return null;

		IEntity cursor = captiveEntity;
		int depth;
		while (cursor && depth < 6)
		{
			AIGroup group = AIGroup.Cast(cursor);
			if (group)
				return group;

			cursor = cursor.GetParent();
			depth++;
		}

		AIControlComponent controlComponent = AIControlComponent.Cast(captiveEntity.FindComponent(AIControlComponent));
		if (controlComponent)
		{
			controlComponent.ActivateAI();
			AIAgent agent = controlComponent.GetControlAIAgent();
			if (agent)
			{
				AIGroup parentGroup = agent.GetParentGroup();
				if (parentGroup)
					return parentGroup;
			}
		}

		return null;
	}

	protected bool TryMoveCaptiveIntoVehicle(IEntity captiveEntity, IEntity vehicleEntity, out string reason)
	{
		reason = "";
		if (!captiveEntity || !vehicleEntity)
		{
			reason = "captive or vehicle missing";
			return false;
		}

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(captiveEntity.FindComponent(SCR_CompartmentAccessComponent));
		if (!access)
		{
			reason = "captive has no compartment access component";
			return false;
		}
		if (access.IsInCompartment() && access.GetVehicle() == vehicleEntity)
		{
			reason = "captive already seated";
			return true;
		}
		if (access.IsGettingIn())
		{
			reason = "captive already boarding";
			return true;
		}

		BaseCompartmentManagerComponent compartmentManager = ResolveCompartmentManager(vehicleEntity);
		if (!compartmentManager)
		{
			reason = "vehicle has no compartment manager";
			return false;
		}

		array<BaseCompartmentSlot> slots = {};
		compartmentManager.GetCompartments(slots);
		BaseCompartmentSlot slot = FindAvailableCaptiveSeat(slots, captiveEntity);
		if (!slot)
		{
			reason = "vehicle has no accessible cargo seat";
			return false;
		}

		if (!access.MoveInVehicle(vehicleEntity, ECompartmentType.CARGO, false, slot))
		{
			reason = "captive vehicle boarding order rejected";
			return false;
		}

		reason = "captive vehicle boarding order issued";
		return true;
	}

	protected BaseCompartmentManagerComponent ResolveCompartmentManager(IEntity vehicleEntity)
	{
		if (!vehicleEntity)
			return null;

		SCR_BaseCompartmentManagerComponent scrManager = SCR_BaseCompartmentManagerComponent.Cast(vehicleEntity.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (scrManager)
			return scrManager;

		return BaseCompartmentManagerComponent.Cast(vehicleEntity.FindComponent(BaseCompartmentManagerComponent));
	}

	protected BaseCompartmentSlot FindAvailableCaptiveSeat(array<BaseCompartmentSlot> slots, IEntity captiveEntity)
	{
		if (!slots || !captiveEntity)
			return null;

		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot)
				continue;
			if (slot.GetType() != ECompartmentType.CARGO)
				continue;
			if (!slot.IsCompartmentAccessible())
				continue;
			if (slot.IsOccupied())
				continue;
			if (slot.IsReserved() && !slot.IsReservedBy(captiveEntity))
				continue;
			if (slot.IsGetInLockedFor(captiveEntity))
				continue;

			return slot;
		}

		return null;
	}

	protected IEntity ResolveEntityVehicle(IEntity entity)
	{
		if (!entity)
			return null;

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(entity.FindComponent(SCR_CompartmentAccessComponent));
		if (!access || !access.IsInCompartment())
			return null;

		return access.GetVehicle();
	}

	protected IEntity ResolvePlayerEntityForCarrierId(string carrierId)
	{
		if (carrierId.IsEmpty())
			return null;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return null;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (carrierId != BuildPlayerCarrierId(playerId))
				continue;

			return playerManager.GetPlayerControlledEntity(playerId);
		}

		return null;
	}

	protected bool TryResolveCarrierPosition(HST_CampaignState state, string carrierId, vector fallbackPosition, out vector position)
	{
		position = fallbackPosition;
		if (carrierId.IsEmpty())
			return false;

		IEntity runtimeEntity = GetRuntimeEntity(carrierId);
		if (runtimeEntity)
		{
			position = runtimeEntity.GetOrigin();
			return true;
		}

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			array<int> playerIds = {};
			playerManager.GetPlayers(playerIds);
			foreach (int playerId : playerIds)
			{
				if (carrierId != BuildPlayerCarrierId(playerId))
					continue;

				IEntity playerEntity = playerManager.GetPlayerControlledEntity(playerId);
				if (!IsLivingPlayerEntity(playerEntity))
					return false;

				position = playerEntity.GetOrigin();
				return true;
			}
		}

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(carrierId);
		if (runtimeVehicle && !runtimeVehicle.m_bDeleted)
		{
			IEntity vehicle = FindMissionVehicleByRuntimeId(carrierId, runtimeVehicle.m_vPosition);
			if (vehicle)
			{
				position = vehicle.GetOrigin();
				runtimeVehicle.m_vPosition = position;
				return true;
			}

			if (!IsZeroVector(runtimeVehicle.m_vPosition))
			{
				position = runtimeVehicle.m_vPosition;
				return true;
			}
		}

		return false;
	}

	protected IEntity FindMissionVehicleByRuntimeId(string vehicleId, vector nearPosition)
	{
		if (vehicleId.IsEmpty() || IsZeroVector(nearPosition))
			return null;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		m_aMissionVehicleScanEntities.Clear();
		world.QueryEntitiesBySphere(nearPosition, 30.0, AddMissionVehicleScanCandidate, null, EQueryEntitiesFlags.ALL);
		foreach (IEntity candidate : m_aMissionVehicleScanEntities)
		{
			string prefab;
			IEntity root = ResolveMissionVehicleRoot(candidate, prefab);
			if (root && ResolveEntityRuntimeId(root) == vehicleId)
				return root;
		}

		return null;
	}

	protected bool IsConvoyStaticFallback(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;

		if (mission.m_sRuntimePhase == "convoy_static")
			return true;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == ROLE_CONVOY_VEHICLE && asset.m_sLastInteraction == "convoy_static")
				return true;
		}

		return false;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected string ResolveEntityRuntimeId(IEntity entity)
	{
		if (!entity)
			return "";

		BaseRplComponent rpl = BaseRplComponent.Cast(entity.FindComponent(BaseRplComponent));
		if (rpl)
			return string.Format("vehicle_%1", rpl.Id());

		return string.Format("vehicle_local_%1_%2", entity.GetName(), entity.GetOrigin());
	}

	protected string ResolveEntityPrefabName(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return "";

		return entity.GetPrefabData().GetPrefabName();
	}

	protected HST_MissionAssetState FindRepresentativeAssetForObjective(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		if (!state || !mission || !objective)
			return null;

		string role = ResolveObjectiveAssetRole(mission, objective);
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (!role.IsEmpty() && asset.m_sRole != role)
				continue;

			return asset;
		}

		return null;
	}

	protected string ResolveObjectiveAssetRole(HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		if (!mission || !objective)
			return "";

		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET)
			return ROLE_HVT;
		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET)
			return ROLE_DESTROY_TARGET;
		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA)
			return ROLE_HOLD_MARKER;

		if (objective.m_sTargetId == "convoy")
			return ROLE_CONVOY_VEHICLE;
		if (objective.m_sTargetId == "convoy_prisoners")
			return ROLE_CONVOY_CAPTIVE;
		if (objective.m_sTargetId == "convoy_cargo")
			return ROLE_CONVOY_PAYLOAD;
		if (objective.m_sTargetId == "supply_pickup")
			return ROLE_CITY_SUPPLIES;
		if (objective.m_sTargetId == "extract_captives")
			return ROLE_CAPTIVE;
		if (objective.m_sTargetId == "hq_delivery")
			return ROLE_LOGISTICS_CARGO;

		if (mission.m_sRuntimePrimitive == PRIMITIVE_DELIVER_SUPPLIES)
			return ROLE_CITY_SUPPLIES;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_RESCUE_EXTRACT)
			return ROLE_CAPTIVE;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO)
			return ROLE_LOGISTICS_CARGO;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
			return ROLE_CONVOY_VEHICLE;

		return "";
	}

	protected int ResolveConvoyVehicleCount(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionDefinition definition)
	{
		int seed = BuildConvoyVehicleCountSeed(state, mission, definition);
		int resolved = MIN_CONVOY_VEHICLES + HST_DefaultCatalog.PositiveMod(seed, MAX_CONVOY_VEHICLES - MIN_CONVOY_VEHICLES + 1);
		return Math.Max(MIN_CONVOY_VEHICLES, Math.Min(MAX_CONVOY_VEHICLES, resolved));
	}

	protected int BuildConvoyVehicleCountSeed(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionDefinition definition)
	{
		int seed = 7;
		if (state)
			seed += state.m_iCampaignSeed * 13 + state.m_iElapsedSeconds * 3 + state.m_aActiveMissions.Count() * 101 + state.m_aMissionAssets.Count() * 17;
		if (mission)
			seed += mission.m_sInstanceId.Length() * 41 + mission.m_sMissionId.Length() * 53 + ResolveMissionInstanceNumericSeed(mission.m_sInstanceId) * 127;
		if (definition)
			seed += definition.m_sMissionId.Length() * 17 + definition.m_iRewardMoney;

		return seed;
	}

	protected int ResolveMissionInstanceNumericSeed(string instanceId)
	{
		string prefix = "mission_";
		if (instanceId.Contains(prefix) && instanceId.Length() > prefix.Length())
		{
			string suffix = instanceId.Substring(prefix.Length(), instanceId.Length() - prefix.Length());
			return suffix.ToInt();
		}

		return instanceId.Length();
	}

	protected int ResolveConvoyIdleDelaySeconds(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		int seed = 991;
		if (state)
			seed += state.m_iCampaignSeed * 19 + state.m_iElapsedSeconds * 5;
		if (mission)
			seed += mission.m_sInstanceId.Length() * 73 + mission.m_sMissionId.Length() * 37;

		return MIN_CONVOY_IDLE_SECONDS + HST_DefaultCatalog.PositiveMod(seed, MAX_CONVOY_IDLE_SECONDS - MIN_CONVOY_IDLE_SECONDS + 1);
	}

	protected void ConfigureConvoyObjectiveRequirements(HST_CampaignState state, HST_ActiveMissionState mission, int convoyVehicleCount)
	{
		if (!state || !mission)
			return;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_sTargetId != "convoy")
				continue;

			objective.m_sLabel = "Neutralize convoy crew";
			if (mission.m_sMissionId == "convoy_ammo")
				objective.m_sRequirementText = "Ambush the convoy. Kill all convoy soldiers, then capture a surviving vehicle to establish the ammo point.";
			else if (mission.m_sMissionId == "convoy_armored")
				objective.m_sRequirementText = "Ambush the convoy. Kill all convoy soldiers, then capture a surviving vehicle for the garage.";
			else if (mission.m_sMissionId == "convoy_money")
				objective.m_sRequirementText = "Ambush the convoy. Kill all convoy soldiers, then recover and deliver the money payload to HQ.";
			else if (mission.m_sMissionId == "convoy_prisoners")
				objective.m_sRequirementText = "Ambush the convoy. Kill all convoy soldiers, then free and extract the prisoners.";
			else if (mission.m_sMissionId == "convoy_supplies")
				objective.m_sRequirementText = "Ambush the convoy. Kill all convoy soldiers, then recover and deliver the supplies.";
			else
				objective.m_sRequirementText = "Ambush the convoy. Kill all convoy soldiers before they reach destination.";
			objective.m_iRequiredCount = Math.Max(1, convoyVehicleCount);
			objective.m_iRequiredProgress = Math.Max(1, convoyVehicleCount);
			objective.m_iCurrentCount = 0;
			objective.m_iCurrentProgress = 0;
		}
	}

	protected bool TryResolveConvoySpawnPlan(HST_CampaignState state, HST_ActiveMissionState mission, HST_GeneratedRouteState route, vector convoyEnd, int vehicleCount, out vector convoyStart, array<vector> convoyStartSlots, out string reason)
	{
		convoyStart = "0 0 0";
		reason = "";
		if (convoyStartSlots)
			convoyStartSlots.Clear();
		if (!state || !mission || IsZeroVector(convoyEnd))
		{
			reason = "mission state or convoy destination missing";
			return false;
		}
		if (vehicleCount <= 0)
		{
			reason = "convoy vehicle count is zero";
			return false;
		}
		if (!convoyStartSlots)
		{
			reason = "convoy start slot buffer missing";
			return false;
		}

		int seed = BuildConvoySpawnPlanSeed(state, mission, vehicleCount);
		for (int attempt = 0; attempt < CONVOY_START_PROBE_ATTEMPTS; attempt++)
		{
			float distanceMeters = ResolveConvoyRandomStartDistanceMeters(seed, attempt);
			vector preferred = BuildConvoySpawnPlanCandidate(route, convoyEnd, distanceMeters, seed, attempt);
			vector leadStart;
			string candidateReason;
			if (!TryResolveConvoyLeadStartCandidate(state, mission, preferred, convoyEnd, leadStart, candidateReason))
			{
				reason = candidateReason;
				continue;
			}

			if (!TryBuildConvoyVehicleStartSlots(leadStart, convoyEnd, vehicleCount, convoyStartSlots, candidateReason))
			{
				reason = candidateReason;
				continue;
			}
			if (convoyStartSlots.Count() != vehicleCount)
			{
				reason = string.Format("convoy spawn plan probed %1/%2 vehicle-safe slots", convoyStartSlots.Count(), vehicleCount);
				convoyStartSlots.Clear();
				continue;
			}

			convoyStart = convoyStartSlots[0];
			return true;
		}

		if (reason.IsEmpty())
			reason = string.Format("no full-column convoy spawn candidate passed %1 probes", CONVOY_START_PROBE_ATTEMPTS);
		return false;
	}

	protected int BuildConvoySpawnPlanSeed(HST_CampaignState state, HST_ActiveMissionState mission, int vehicleCount)
	{
		int seed = 503;
		if (state)
			seed += state.m_iCampaignSeed * 17 + state.m_iElapsedSeconds * 29 + state.m_aActiveMissions.Count() * 37 + state.m_aMissionAssets.Count() * 41;
		if (mission)
			seed += mission.m_sInstanceId.Length() * 43 + mission.m_sMissionId.Length() * 47 + ResolveMissionInstanceNumericSeed(mission.m_sInstanceId) * 131;

		seed += vehicleCount * 59;
		return seed;
	}

	protected float ResolveConvoyRandomStartDistanceMeters(int seed, int attempt)
	{
		int minDistance = Math.Round(MIN_CONVOY_START_DISTANCE_METERS);
		int maxDistance = Math.Round(MAX_CONVOY_START_DISTANCE_METERS);
		int span = Math.Max(1, maxDistance - minDistance + 1);
		return minDistance + HST_DefaultCatalog.PositiveMod(seed + attempt * 149 + attempt * attempt * 17, span);
	}

	protected vector BuildConvoySpawnPlanCandidate(HST_GeneratedRouteState route, vector convoyEnd, float distanceMeters, int seed, int attempt)
	{
		if (route)
		{
			if (attempt % 5 == 0 && !IsZeroVector(route.m_vStartPosition))
				return BuildConvoyStartCandidateFromAnchor(convoyEnd, route.m_vStartPosition, distanceMeters);
			if (attempt % 5 == 1 && !IsZeroVector(route.m_vMidPosition))
				return BuildConvoyStartCandidateFromAnchor(convoyEnd, route.m_vMidPosition, distanceMeters);
		}

		int direction = HST_DefaultCatalog.PositiveMod(seed + attempt * 3, 8);
		return BuildConvoyStartCandidate(convoyEnd, direction, distanceMeters);
	}

	protected vector BuildConvoyStartCandidateFromAnchor(vector targetPosition, vector anchorPosition, float distanceMeters)
	{
		float dx = anchorPosition[0] - targetPosition[0];
		float dz = anchorPosition[2] - targetPosition[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		if (length <= 1.0)
			return BuildConvoyStartCandidate(targetPosition, 0, distanceMeters);

		vector candidate = targetPosition;
		candidate[0] = candidate[0] + (dx / length) * distanceMeters;
		candidate[2] = candidate[2] + (dz / length) * distanceMeters;
		return candidate;
	}

	protected bool TryResolveConvoyLeadStartCandidate(HST_CampaignState state, HST_ActiveMissionState mission, vector preferred, vector convoyEnd, out vector resolved, out string reason)
	{
		reason = "";
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		if (!TryResolveConvoyRoadVehiclePosition(preferred, CONVOY_START_ROAD_SEARCH_RADIUS_METERS, CONVOY_EXPANDED_START_ROAD_SEARCH_RADIUS_METERS, convoyEnd, resolved, roadForward, roadWidth, roadDistance, roadReason))
		{
			reason = "lead convoy start is not road-resolved: " + roadReason;
			return false;
		}
		if (!HST_WorldPositionService.IsVehicleFootprintStableWithForward(resolved, roadForward))
		{
			reason = "lead road-resolved convoy start failed the flat vehicle footprint check";
			return false;
		}
		if (!IsConvoyStagingPositionClear(state, mission, resolved))
		{
			reason = "lead convoy start is too close to another zone or generated site";
			return false;
		}
		if (!IsUsableConvoyRouteSegment(resolved, convoyEnd))
		{
			reason = "lead convoy start failed the 2000-5000m road endpoint band";
			return false;
		}

		reason = roadReason;
		return true;
	}

	protected bool TryResolveConvoyRoadVehiclePosition(vector preferred, float primarySearchRadius, float expandedSearchRadius, vector destination, out vector resolved, out vector roadForward, out float roadWidth, out float roadDistance, out string reason)
	{
		string primaryReason;
		if (HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(preferred, primarySearchRadius, destination, resolved, roadForward, roadWidth, roadDistance, primaryReason))
		{
			reason = primaryReason;
			return true;
		}

		reason = primaryReason;
		if (expandedSearchRadius <= primarySearchRadius)
			return false;

		string expandedReason;
		if (!HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(preferred, expandedSearchRadius, destination, resolved, roadForward, roadWidth, roadDistance, expandedReason))
		{
			reason = string.Format("%1; expanded %2m search also failed: %3", primaryReason, Math.Round(expandedSearchRadius), expandedReason);
			return false;
		}

		reason = string.Format("%1 after expanded %2m road search; primary %3m search failed: %4", expandedReason, Math.Round(expandedSearchRadius), Math.Round(primarySearchRadius), primaryReason);
		return true;
	}

	protected bool TryBuildConvoyVehicleStartSlots(vector leadStart, vector convoyEnd, int vehicleCount, array<vector> convoyStartSlots, out string reason)
	{
		reason = "";
		if (!convoyStartSlots)
		{
			reason = "convoy start slot buffer missing";
			return false;
		}

		convoyStartSlots.Clear();
		for (int index = 0; index < vehicleCount; index++)
		{
			vector preferred = BuildConvoyColumnSlotPosition(leadStart, convoyEnd, index);
			vector resolved;
			if (!TryResolveConvoyVehicleStartSlot(preferred, convoyEnd, convoyStartSlots, resolved, reason))
			{
				convoyStartSlots.Clear();
				return false;
			}

			convoyStartSlots.Insert(resolved);
		}

		return true;
	}

	protected vector BuildConvoyColumnSlotPosition(vector leadStart, vector convoyEnd, int index)
	{
		vector slot = leadStart;
		if (index <= 0)
			return slot;

		float dx = leadStart[0] - convoyEnd[0];
		float dz = leadStart[2] - convoyEnd[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		if (length <= 1.0)
		{
			slot[2] = slot[2] - CONVOY_VEHICLE_START_SPACING_METERS * index;
			return slot;
		}

		float distanceBehindLead = CONVOY_VEHICLE_START_SPACING_METERS * index;
		slot[0] = slot[0] + (dx / length) * distanceBehindLead;
		slot[2] = slot[2] + (dz / length) * distanceBehindLead;
		return slot;
	}

	protected vector ResolveConvoyEndPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		vector resolved;
		string reason;
		if (TryResolveConvoyEndPosition(state, mission, resolved, reason))
			return resolved;

		return "0 0 0";
	}

	protected bool TryResolveConvoyEndPosition(HST_CampaignState state, HST_ActiveMissionState mission, out vector resolved, out string reason)
	{
		resolved = "0 0 0";
		reason = "";
		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		vector preferred;
		vector forwardTarget;
		if (route)
		{
			preferred = route.m_vEndPosition;
			forwardTarget = route.m_vMidPosition;
		}
		else if (mission)
		{
			preferred = ResolveRuntimePropPosition(state, mission);
			forwardTarget = mission.m_vTargetPosition;
		}

		if (IsZeroVector(preferred))
		{
			reason = "convoy destination preferred position missing";
			return false;
		}
		if (IsZeroVector(forwardTarget))
			forwardTarget = preferred;

		vector roadForward;
		float roadWidth;
		float roadDistance;
		if (!HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(preferred, CONVOY_DESTINATION_ROAD_SEARCH_RADIUS_METERS, forwardTarget, resolved, roadForward, roadWidth, roadDistance, reason))
			return false;

		reason = string.Format("destination road-resolved | distance %1m | width %2m", Math.Round(roadDistance), Math.Round(roadWidth));
		return true;
	}

	protected HST_GeneratedRouteState ResolveMissionRoute(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sSiteId.IsEmpty())
			return null;

		HST_GeneratedSiteState site = state.FindGeneratedSite(mission.m_sSiteId);
		if (!site || site.m_sRouteId.IsEmpty())
			return null;

		return state.FindGeneratedRoute(site.m_sRouteId);
	}

	protected vector BuildConvoyStartCandidate(vector targetPosition, int direction, float distance)
	{
		vector candidate = targetPosition;
		float diagonal = distance * 0.707;
		if (direction == 0)
			candidate[0] = candidate[0] + distance;
		else if (direction == 1)
			candidate[0] = candidate[0] - distance;
		else if (direction == 2)
			candidate[2] = candidate[2] + distance;
		else if (direction == 3)
			candidate[2] = candidate[2] - distance;
		else if (direction == 4)
		{
			candidate[0] = candidate[0] + diagonal;
			candidate[2] = candidate[2] + diagonal;
		}
		else if (direction == 5)
		{
			candidate[0] = candidate[0] - diagonal;
			candidate[2] = candidate[2] + diagonal;
		}
		else if (direction == 6)
		{
			candidate[0] = candidate[0] + diagonal;
			candidate[2] = candidate[2] - diagonal;
		}
		else
		{
			candidate[0] = candidate[0] - diagonal;
			candidate[2] = candidate[2] - diagonal;
		}

		return candidate;
	}

	protected bool IsConvoyStagingPositionClear(HST_CampaignState state, HST_ActiveMissionState mission, vector position)
	{
		if (!state)
			return false;

		float zoneClearanceSq = MIN_CONVOY_STAGING_ZONE_CLEARANCE_METERS * MIN_CONVOY_STAGING_ZONE_CLEARANCE_METERS;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || (mission && zone.m_sZoneId == mission.m_sTargetZoneId))
				continue;

			if (DistanceSq2D(position, zone.m_vPosition) < zoneClearanceSq)
				return false;
		}

		float siteClearanceSq = MIN_CONVOY_STAGING_SITE_CLEARANCE_METERS * MIN_CONVOY_STAGING_SITE_CLEARANCE_METERS;
		foreach (HST_GeneratedSiteState site : state.m_aGeneratedSites)
		{
			if (!site || (mission && site.m_sZoneId == mission.m_sTargetZoneId))
				continue;

			if (DistanceSq2D(position, site.m_vPosition) < siteClearanceSq)
				return false;
		}

		return true;
	}

	protected bool IsUsableConvoyRouteSegment(vector startPosition, vector endPosition)
	{
		if (IsZeroVector(startPosition) || IsZeroVector(endPosition))
			return false;
		if (!IsConvoyDistanceInsideBand(startPosition, endPosition))
			return false;
		return true;
	}

	protected bool IsConvoyDistanceInsideBand(vector startPosition, vector endPosition)
	{
		if (IsZeroVector(startPosition) || IsZeroVector(endPosition))
			return false;

		float distanceSq = DistanceSq2D(startPosition, endPosition);
		return distanceSq >= MIN_CONVOY_START_DISTANCE_METERS * MIN_CONVOY_START_DISTANCE_METERS && distanceSq <= MAX_CONVOY_START_DISTANCE_METERS * MAX_CONVOY_START_DISTANCE_METERS;
	}

	protected bool IsDryConvoyRouteSegment(vector startPosition, vector endPosition)
	{
		for (int i = 1; i <= CONVOY_ROUTE_SAMPLE_COUNT; i++)
		{
			float t = i;
			t = t / (CONVOY_ROUTE_SAMPLE_COUNT + 1);
			vector sample = InterpolatePosition(startPosition, endPosition, t);
			vector resolved;
			if (!HST_WorldPositionService.TryResolveGroundPosition(sample, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, resolved, true))
				return false;
			if (HST_WorldPositionService.IsLikelyOpenWater(resolved))
				return false;
		}

		return true;
	}

	protected vector InterpolatePosition(vector startPosition, vector endPosition, float t)
	{
		vector result = startPosition;
		result[0] = startPosition[0] + (endPosition[0] - startPosition[0]) * t;
		result[1] = startPosition[1] + (endPosition[1] - startPosition[1]) * t;
		result[2] = startPosition[2] + (endPosition[2] - startPosition[2]) * t;
		return result;
	}

	protected vector OffsetConvoyVehicleStartPosition(HST_GeneratedRouteState route, vector startPosition, vector endPosition, int index, array<vector> reservedPositions)
	{
		float distanceAlongRoute = CONVOY_VEHICLE_START_SPACING_METERS * index;
		vector preferred = ResolveConvoyRouteStagingPosition(route, startPosition, endPosition, distanceAlongRoute);
		vector resolved;
		string slotReason;
		if (TryResolveConvoyVehicleStartSlot(preferred, endPosition, reservedPositions, resolved, slotReason))
			return resolved;

		for (int attempt = 1; attempt <= 8; attempt++)
		{
			float forwardDistance = distanceAlongRoute + MIN_CONVOY_VEHICLE_START_SEPARATION_METERS * attempt;
			vector forwardCandidate = ResolveConvoyRouteStagingPosition(route, startPosition, endPosition, forwardDistance);
			if (TryResolveConvoyVehicleStartSlot(forwardCandidate, endPosition, reservedPositions, resolved, slotReason))
				return resolved;

			float backwardDistance = distanceAlongRoute - MIN_CONVOY_VEHICLE_START_SEPARATION_METERS * attempt;
			if (backwardDistance < 0.0)
				continue;

			vector backwardCandidate = ResolveConvoyRouteStagingPosition(route, startPosition, endPosition, backwardDistance);
			if (TryResolveConvoyVehicleStartSlot(backwardCandidate, endPosition, reservedPositions, resolved, slotReason))
				return resolved;
		}

		if (TryResolveConvoyVehicleStartSlot(preferred, endPosition, reservedPositions, resolved, slotReason))
			return resolved;

		return startPosition;
	}

	protected bool TryResolveConvoyVehicleStartSlot(vector preferred, vector convoyEnd, array<vector> reservedPositions, out vector resolved, out string reason)
	{
		reason = "";
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		if (!TryResolveConvoyRoadVehiclePosition(preferred, CONVOY_SLOT_ROAD_SEARCH_RADIUS_METERS, CONVOY_EXPANDED_SLOT_ROAD_SEARCH_RADIUS_METERS, convoyEnd, resolved, roadForward, roadWidth, roadDistance, roadReason))
		{
			reason = "convoy vehicle slot is not road-resolved: " + roadReason;
			return false;
		}
		if (HST_WorldPositionService.IsLikelyOpenWater(resolved))
		{
			reason = "convoy vehicle slot resolved to open water";
			return false;
		}
		if (!HST_WorldPositionService.IsVehicleFootprintStableWithForward(resolved, roadForward))
		{
			reason = "convoy road vehicle slot failed the flat vehicle footprint check";
			return false;
		}
		if (!IsConvoyDistanceInsideBand(resolved, convoyEnd))
		{
			reason = "convoy vehicle slot is outside the required 2000-5000m destination band";
			return false;
		}
		if (!IsConvoyVehicleStartSeparated(resolved, reservedPositions))
		{
			reason = "convoy vehicle slot overlaps another pre-probed convoy vehicle slot";
			return false;
		}

		reason = roadReason;
		return true;
	}

	protected bool TryResolveConvoyVehicleStartCandidate(vector preferred, out vector resolved, bool preferHeavyVehicleTerrain)
	{
		if (preferHeavyVehicleTerrain)
			return HST_WorldPositionService.TryResolveHeavyVehicleSpawnPosition(preferred, resolved, true);

		return HST_WorldPositionService.TryResolveLargeVehicleSpawnPosition(preferred, resolved, true);
	}

	protected vector ResolveConvoyRouteStagingPosition(HST_GeneratedRouteState route, vector startPosition, vector endPosition, float distanceAlongRoute)
	{
		if (distanceAlongRoute <= 0.0)
			return startPosition;

		ref array<vector> routePoints = BuildConvoyRouteStagingPoints(route, startPosition, endPosition);
		if (!routePoints || routePoints.Count() == 0)
			return startPosition;
		if (routePoints.Count() == 1)
			return routePoints[0];

		vector previous = routePoints[0];
		float remaining = distanceAlongRoute;
		for (int i = 1; i < routePoints.Count(); i++)
		{
			vector current = routePoints[i];
			float segmentLength = Math.Sqrt(DistanceSq2D(previous, current));
			if (segmentLength <= 0.1)
			{
				previous = current;
				continue;
			}

			if (remaining <= segmentLength)
			{
				float t = remaining / segmentLength;
				return InterpolatePosition(previous, current, t);
			}

			remaining = remaining - segmentLength;
			previous = current;
		}

		return routePoints[routePoints.Count() - 1];
	}

	protected ref array<vector> BuildConvoyRouteStagingPoints(HST_GeneratedRouteState route, vector startPosition, vector endPosition)
	{
		ref array<vector> points = {};
		AppendConvoyRouteStagingPoint(points, startPosition);

		if (route && route.m_aWaypoints)
		{
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

				AppendConvoyRouteStagingPoint(points, selectedWaypoint.m_vPosition);
				lastIndex = selectedWaypoint.m_iIndex;
			}
		}

		AppendConvoyRouteStagingPoint(points, endPosition);
		return points;
	}

	protected void AppendConvoyRouteStagingPoint(array<vector> points, vector point)
	{
		if (!points || IsZeroVector(point))
			return;
		if (points.Count() > 0 && DistanceSq2D(points[points.Count() - 1], point) < 9.0)
			return;

		points.Insert(point);
	}

	protected bool IsConvoyVehicleStartSeparated(vector candidate, array<vector> reservedPositions)
	{
		if (!reservedPositions)
			return true;

		float minDistanceSq = MIN_CONVOY_VEHICLE_START_SEPARATION_METERS * MIN_CONVOY_VEHICLE_START_SEPARATION_METERS;
		foreach (vector reservedPosition : reservedPositions)
		{
			if (DistanceSq2D(candidate, reservedPosition) < minDistanceSq)
				return false;
		}

		return true;
	}

	protected vector OffsetMissionAssetPosition(vector position, int index)
	{
		vector offsetPosition = position;
		int safeIndex = index;
		if (safeIndex < 0)
			safeIndex = -safeIndex;

		int column = safeIndex - (safeIndex / 3) * 3;
		int row = safeIndex / 3;
		offsetPosition[0] = offsetPosition[0] + (column - 1) * 4.0;
		offsetPosition[2] = offsetPosition[2] + row * 5.0;
		return HST_WorldPositionService.ResolveSafeGroundPosition(offsetPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, true, 2.0);
	}

	protected vector ResolveRuntimePropPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == mission.m_sInstanceId)
				return HST_WorldPositionService.ResolveSafeGroundPosition(objective.m_vPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, true, 2.0);
		}

		if (!mission.m_sTargetZoneId.IsEmpty())
		{
			HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
			if (zone)
				return HST_WorldPositionService.ResolveSafeGroundPosition(zone.m_vPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, true, 2.0);
		}

		return HST_WorldPositionService.ResolveSafeGroundPosition(state.m_vHQPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, true, 2.0);
	}

	protected string SelectRuntimePropPrefab(string primitive)
	{
		if (primitive == PRIMITIVE_KILL_HVT)
			return PROP_HVT;
		if (primitive == PRIMITIVE_DESTROY_TARGET)
			return PROP_DESTROY_TARGET;
		if (primitive == PRIMITIVE_RECOVER_CARGO || primitive == PRIMITIVE_DELIVER_SUPPLIES)
			return PROP_CARGO;
		if (primitive == PRIMITIVE_RESCUE_EXTRACT)
			return PROP_CAPTIVES;
		if (primitive == PRIMITIVE_CONVOY_INTERCEPT)
			return PROP_CONVOY_VEHICLE;
		if (primitive == PRIMITIVE_HOLD_AREA || primitive == PRIMITIVE_CLEAR_AREA)
			return PROP_HOLD_MARKER;

		return "";
	}

	protected string ResolveLogisticsCargoPrefab(HST_ActiveMissionState mission)
	{
		if (!mission)
			return PROP_CARGO;

		if (mission.m_sMissionId == "logistics_bank")
			return PROP_BANK_MONEY;

		if (mission.m_sMissionId.Contains("resource") || mission.m_sMissionId.Contains("factory") || mission.m_sMissionId.Contains("seaport") || mission.m_sMissionId.Contains("support") || mission.m_sMissionId.Contains("salvage") || mission.m_sMissionId == "dynamic_gun_shop")
			return PROP_RESOURCE_CACHE;

		return PROP_CARGO;
	}

	protected string ResolveMissionVehiclePrefab(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		string factionKey = ResolveMissionOwnerFaction(state, mission);
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		int seed = 0;
		if (state)
			seed += state.m_iCampaignSeed + state.m_iElapsedSeconds * 11;
		if (mission)
			seed += mission.m_sInstanceId.Length() * 29 + mission.m_sMissionId.Length() * 37;

		if (faction && faction.m_aVehiclePrefabs.Count() > 0)
		{
			int startIndex = HST_DefaultCatalog.PositiveMod(seed, faction.m_aVehiclePrefabs.Count());
			for (int i = 0; i < faction.m_aVehiclePrefabs.Count(); i++)
			{
				int index = HST_DefaultCatalog.PositiveMod(startIndex + i, faction.m_aVehiclePrefabs.Count());
				string candidate = faction.m_aVehiclePrefabs[index];
				if (IsValidMissionVehiclePrefab(candidate))
					return candidate;
			}
		}

		if (IsValidMissionVehiclePrefab(PROP_CONVOY_VEHICLE))
			return PROP_CONVOY_VEHICLE;

		return "";
	}

	protected string ResolveMissionOwnerFaction(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sTargetZoneId.IsEmpty())
			return "";

		HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
		if (!zone)
			return "";

		return zone.m_sOwnerFactionKey;
	}

	protected bool IsValidMissionVehiclePrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		ResourceName resourceName = prefab;
		Resource loaded = Resource.Load(resourceName);
		return loaded && loaded.IsValid();
	}

	protected vector BuildRuntimePropAngles(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		vector angles;
		int seed = 313;
		if (state)
			seed += state.m_iCampaignSeed * 19 + state.m_iElapsedSeconds * 3;
		if (mission)
			seed += mission.m_sInstanceId.Length() * 97 + mission.m_sMissionId.Length() * 53;
		if (seed < 0)
			seed = -seed;

		angles[0] = seed - (seed / 360) * 360;
		return HST_WorldPositionService.BuildUprightAnglesFromVector(angles);
	}

	string FindCompletedActiveMissionId(HST_CampaignState state, HST_MissionObjectiveService objectives)
	{
		if (!state || !objectives)
			return "";

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;

			if (objectives.AreMissionObjectivesComplete(state, mission.m_sInstanceId))
				return mission.m_sInstanceId;
		}

		return "";
	}

	string FindFailedActiveMissionId(HST_CampaignState state)
	{
		if (!state)
			return "";

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;

			if (mission.m_sRuntimePhase == PHASE_FAILED)
				return mission.m_sInstanceId;
		}

		return "";
	}

	string BuildRuntimeReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi mission runtime | state not ready";

		int physical;
		int fallback;
		int spawned;
		int objectiveCount;
		int assetCount;
		string details = "";
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;

			if (mission.m_eRuntimeMode == HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_PHYSICAL_MVP || mission.m_eRuntimeMode == HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_STATE_MACHINE)
				physical++;
			if (mission.m_bRuntimeFallback)
				fallback++;
			if (mission.m_bRuntimeSpawned)
				spawned++;
			objectiveCount += CountMissionObjectivesForReport(state, mission.m_sInstanceId);
			assetCount += CountMissionAssetsForReport(state, mission.m_sInstanceId);

			details = details + BuildMissionRuntimeReport(state, mission);
		}

		return string.Format("h-istasi mission runtime | physical %1 | spawned %2 | fallback %3 | objectives %4 | assets %5", physical, spawned, fallback, objectiveCount, assetCount) + details;
	}

	string BuildRuntimeReportForMission(HST_CampaignState state, string instanceId)
	{
		if (!state)
			return "h-istasi mission runtime | state not ready";
		if (instanceId.IsEmpty())
			return "h-istasi mission runtime | mission instance ID required";

		HST_ActiveMissionState mission = state.FindActiveMission(instanceId);
		if (!mission)
			return "h-istasi mission runtime | mission not found: " + instanceId;

		return "h-istasi mission runtime | selected mission" + BuildMissionRuntimeReport(state, mission);
	}

	protected string BuildMissionRuntimeReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "";

		int objectiveTotal;
		int objectiveComplete;
		int objectiveFailed;
		CountMissionObjectives(state, mission.m_sInstanceId, objectiveTotal, objectiveComplete, objectiveFailed);

		int assetTotal;
		int assetSpawned;
		int assetDestroyed;
		int assetDelivered;
		CountMissionAssets(state, mission.m_sInstanceId, assetTotal, assetSpawned, assetDestroyed, assetDelivered);

		int runtimeEntityCount = CountMissionRuntimeEntities(state, mission.m_sInstanceId);
		string failureReason = ReportText(mission.m_sRuntimeFailureReason);

		string line = string.Format("\ninstance %1 | mission %2 | name %3 | target zone %4 | site %5", ReportText(mission.m_sInstanceId), ReportText(mission.m_sMissionId), ReportMissionDisplayName(mission), ReportTargetZone(state, mission.m_sTargetZoneId), ReportSite(state, mission.m_sSiteId));
		line = line + string.Format(" | status %1 | primitive %2 | runtime %3 | phase %4 | remaining %5s", mission.m_eStatus, ReportText(mission.m_sRuntimePrimitive), ReportText(mission.m_sRuntimeType), ReportText(mission.m_sRuntimePhase), mission.m_iRemainingSeconds);
		line = line + string.Format(" | objective count %1 | complete objectives %2 | failed objectives %3", objectiveTotal, objectiveComplete, objectiveFailed);
		line = line + string.Format(" | mission asset count %1 | runtime entity count %2 | failure reason %3", assetTotal, runtimeEntityCount, failureReason);
		line = line + string.Format(" | assets spawned %1/%2", assetSpawned, assetTotal);
		line = line + string.Format(" destroyed %1 delivered %2", assetDestroyed, assetDelivered);
		line = line + string.Format(" | fallback %1 | cleanup %2", mission.m_bRuntimeFallback, mission.m_bRuntimeCleanupComplete);

		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
			line = line + BuildConvoyRouteReport(state, mission);

		line = line + BuildMissionObjectiveRuntimeReport(state, mission);
		line = line + BuildMissionAssetRuntimeReport(state, mission);
		return line;
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

	protected string ReportMissionDisplayName(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "none";
		if (!mission.m_sDisplayName.IsEmpty())
			return mission.m_sDisplayName;

		return ReportText(mission.m_sMissionId);
	}

	protected string ReportTargetZone(HST_CampaignState state, string zoneId)
	{
		if (zoneId.IsEmpty())
			return "none";
		if (!state)
			return "missing:" + zoneId;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
			return "missing:" + zoneId;

		return zoneId;
	}

	protected string ReportSite(HST_CampaignState state, string siteId)
	{
		if (siteId.IsEmpty())
			return "none";
		if (!state)
			return "missing:" + siteId;

		HST_GeneratedSiteState site = state.FindGeneratedSite(siteId);
		if (!site)
			return "missing:" + siteId;

		return siteId;
	}

	protected void CountMissionObjectives(HST_CampaignState state, string instanceId, out int total, out int complete, out int failed)
	{
		total = 0;
		complete = 0;
		failed = 0;
		if (!state || instanceId.IsEmpty())
			return;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != instanceId)
				continue;

			total++;
			if (objective.m_bComplete)
				complete++;
			if (objective.m_bFailed)
				failed++;
		}
	}

	protected void CountMissionAssets(HST_CampaignState state, string instanceId, out int total, out int spawned, out int destroyed, out int delivered)
	{
		total = 0;
		spawned = 0;
		destroyed = 0;
		delivered = 0;
		if (!state || instanceId.IsEmpty())
			return;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != instanceId)
				continue;

			total++;
			if (asset.m_bSpawned)
				spawned++;
			if (asset.m_bDestroyed)
				destroyed++;
			if (asset.m_bDelivered)
				delivered++;
		}
	}

	protected int CountMissionRuntimeEntities(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return 0;

		int count;
		foreach (HST_MissionRuntimeEntityState runtimeEntity : state.m_aMissionRuntimeEntities)
		{
			if (runtimeEntity && runtimeEntity.m_sMissionInstanceId == instanceId)
				count++;
		}

		return count;
	}

	protected int CountMissionObjectivesForReport(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return 0;

		int count;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == instanceId)
				count++;
		}

		return count;
	}

	protected int CountMissionAssetsForReport(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return 0;

		int count;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == instanceId)
				count++;
		}

		return count;
	}

	protected bool IsPersistenceSmokeMission(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;

		return mission.m_sInstanceId.Contains(PERSISTENCE_SMOKE_PREFIX) || mission.m_sMissionId.Contains(PERSISTENCE_SMOKE_PREFIX);
	}

	protected string BuildConvoyRouteReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		string routeId = "none";
		vector start;
		vector mid;
		vector end = mission.m_vTargetPosition;
		int distanceMeters;
		int waypointCount;
		bool roadRoute;
		bool vehicleSafe;
		string validationReason = "none";
		string waypointReport = "";
		if (route)
		{
			routeId = route.m_sRouteId;
			start = route.m_vStartPosition;
			mid = route.m_vMidPosition;
			end = route.m_vEndPosition;
			distanceMeters = route.m_iDistanceMeters;
			waypointCount = route.m_iWaypointCount;
			roadRoute = route.m_bRoadRoute;
			vehicleSafe = route.m_bValidatedForVehicles;
			if (!route.m_sValidationFailureReason.IsEmpty())
				validationReason = route.m_sValidationFailureReason;
			waypointReport = BuildConvoyRouteWaypointReport(route);
		}

		vector convoySource = ResolveFirstConvoyAssetSourcePosition(state, mission);
		vector convoyTarget = ResolveFirstConvoyAssetTargetPosition(state, mission);
		int plannedDistanceMeters = Math.Round(Math.Sqrt(DistanceSq2D(convoySource, convoyTarget)));
		bool bandValid = IsConvoyDistanceInsideBand(convoySource, convoyTarget);
		string roadManagerReason;
		bool roadManagerAvailable = HST_WorldPositionService.IsRoadNetworkAvailable(roadManagerReason);
		string report = string.Format("\n  convoy route | route %1 | road %2 | vehicle-safe %3", routeId, roadRoute, vehicleSafe);
		report = report + string.Format(" | waypoint count %1 | distance %2m | validation %3 | road manager %4 %5", waypointCount, distanceMeters, validationReason, ReportBool(roadManagerAvailable), ReportText(roadManagerReason));
		report = report + string.Format(" | start %1 | mid %2 | end %3", start, mid, end);
		report = report + string.Format(" | planned source %1 | planned target %2 | planned distance %3m", convoySource, convoyTarget, plannedDistanceMeters);
		report = report + string.Format(" | required band 2000-5000m | band valid %1", ReportBool(bandValid));
		report = report + string.Format(" | required vehicles %1 | captured %2 | eta %3s", mission.m_iRequiredVehicleCount, mission.m_iCapturedVehicleCount, mission.m_iRuntimeETASeconds);
		report = report + string.Format(" | timers %1/%2/%3", mission.m_iRuntimeCounterA, mission.m_iRuntimeCounterB, mission.m_iRuntimeCounterC);
		report = report + BuildConvoyRoadResolverReport("planned source road", convoySource, convoyTarget, CONVOY_SLOT_ROAD_SEARCH_RADIUS_METERS);
		report = report + BuildConvoyRoadResolverReport("planned target road", convoyTarget, convoySource, CONVOY_DESTINATION_ROAD_SEARCH_RADIUS_METERS);
		return report + waypointReport;
	}

	protected string BuildConvoyRouteWaypointReport(HST_GeneratedRouteState route)
	{
		if (!route)
			return "";

		string report = "";
		foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
		{
			if (!waypoint)
				continue;

			report = report + string.Format("\n    route waypoint template %1 | hint %2 | radius %3m | position %4 | road check assigned-runtime-chain", waypoint.m_iIndex, waypoint.m_sHint, waypoint.m_iRadiusMeters, waypoint.m_vPosition);
		}

		return report;
	}

	protected string BuildConvoyRoadResolverReport(string label, vector position, vector destination, float searchRadius)
	{
		vector roadPosition;
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		bool roadResolved = HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(position, searchRadius, destination, roadPosition, roadForward, roadWidth, roadDistance, roadReason);
		if (!roadResolved)
			return string.Format("\n    %1 | road-resolved no | reason %2", label, ReportText(roadReason));

		return string.Format("\n    %1 | road-resolved yes | road position %2 | forward %3 | width %4m | resolver distance %5m", label, roadPosition, roadForward, Math.Round(roadWidth), Math.Round(roadDistance));
	}

	protected vector ResolveFirstConvoyAssetSourcePosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == ROLE_CONVOY_VEHICLE)
				return asset.m_vSourcePosition;
		}

		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		if (route && !IsZeroVector(route.m_vStartPosition))
			return route.m_vStartPosition;

		return mission.m_vTargetPosition;
	}

	protected vector ResolveFirstConvoyAssetTargetPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == ROLE_CONVOY_VEHICLE)
				return asset.m_vTargetPosition;
		}

		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		if (route && !IsZeroVector(route.m_vEndPosition))
			return route.m_vEndPosition;

		return mission.m_vTargetPosition;
	}

	protected string BuildMissionObjectiveRuntimeReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "";

		string report = "";
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			string status = "active";
			if (objective.m_bComplete)
				status = "complete";
			else if (objective.m_bFailed)
				status = "failed";

			report = report + string.Format("\n  objective | %1 | %2 | target %3 | status %4", objective.m_sObjectiveId, objective.m_sLabel, objective.m_sTargetId, status);
			report = report + string.Format(" | progress %1/%2 | count %3/%4", objective.m_iCurrentProgress, objective.m_iRequiredProgress, objective.m_iCurrentCount, objective.m_iRequiredCount);
			report = report + string.Format(" | hold %1/%2 | entity %3", objective.m_iHoldSeconds, objective.m_iRequiredHoldSeconds, objective.m_sLinkedRuntimeEntityId);
		}

		return report;
	}

	protected string BuildMissionAssetRuntimeReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "";

		string report = "";
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			string status = "active";
			if (asset.m_bDestroyed)
				status = "destroyed";
			else if (asset.m_bDelivered)
				status = "delivered";
			else if (asset.m_bPickedUp)
				status = "picked_up";
			else if (!asset.m_bSpawned)
				status = "queued";

			report = report + string.Format("\n  asset | %1 | %2/%3 | status %4", asset.m_sAssetId, asset.m_sKind, asset.m_sRole, status);
			report = report + string.Format(" | source %1 | current %2 | target %3", asset.m_vSourcePosition, asset.m_vCurrentPosition, asset.m_vTargetPosition);
			report = report + string.Format(" | radius %1m | capacity %2 | attached %3 | outcome %4", asset.m_iInteractionRadiusMeters, asset.m_iCargoCapacityCost, ReportBool(asset.m_bAttachedToCarrier), ReportBool(asset.m_bOutcomeApplied));
			if (!asset.m_sCarriedByVehicleId.IsEmpty())
				report = report + " | carrier " + asset.m_sCarriedByVehicleId;
			if (!asset.m_sOutcomeKind.IsEmpty())
				report = report + " | outcome kind " + asset.m_sOutcomeKind;
			if (!IsZeroVector(asset.m_vCurrentPosition) && !IsZeroVector(asset.m_vTargetPosition))
				report = report + string.Format(" | delivery distance %1m", Math.Round(Math.Sqrt(DistanceSq2D(asset.m_vCurrentPosition, asset.m_vTargetPosition))));
			if (!asset.m_sLastInteraction.IsEmpty())
				report = report + " | last " + asset.m_sLastInteraction;
		}

		return report;
	}

	string PrimitiveForMission(HST_MissionDefinition definition)
	{
		if (!definition)
			return PRIMITIVE_ABSTRACT_FALLBACK;

		return PrimitiveForMissionId(definition.m_sMissionId, definition.m_eCategory);
	}

	string PrimitiveForMissionId(string missionId, HST_EMissionCategory category)
	{
		if (missionId == "assassinate_officer" || missionId == "assassinate_traitor" || missionId == "assassinate_specops")
			return PRIMITIVE_KILL_HVT;

		if (missionId == "conquest_town" || missionId == "conquest_resource" || missionId == "conquest_factory" || missionId == "conquest_outpost" || missionId == "conquest_airfield" || missionId == "conquest_seaport" || missionId == "dynamic_defend_petros" || missionId == "dynamic_city_flip_battle")
			return PRIMITIVE_HOLD_AREA;

		if (missionId == "destroy_radio_tower" || missionId == "destroy_downed_helicopter" || missionId == "destroy_outpost_cache" || missionId == "destroy_factory_asset" || missionId == "destroy_airfield_asset" || missionId == "destroy_seaport_asset" || missionId == "destroy_or_steal_armor" || missionId == "dynamic_stop_tower_rebuild")
			return PRIMITIVE_DESTROY_TARGET;

		if (missionId == "logistics_bank" || missionId == "logistics_resource_cache" || missionId == "logistics_factory_supplies" || missionId == "logistics_airfield_intel" || missionId == "logistics_seaport_supplies" || missionId == "logistics_support_cache" || missionId == "logistics_salvage_supplies" || missionId == "logistics_ammo_truck" || missionId == "logistics_weapons_truck" || missionId == "dynamic_gun_shop")
			return PRIMITIVE_RECOVER_CARGO;

		if (missionId == "rescue_pows" || missionId == "rescue_refugees")
			return PRIMITIVE_RESCUE_EXTRACT;

		if (missionId == "support_city_supplies")
			return PRIMITIVE_DELIVER_SUPPLIES;

		if (missionId == "convoy_ammo" || missionId == "convoy_armored" || missionId == "convoy_money" || missionId == "convoy_prisoners" || missionId == "convoy_reinforcements" || missionId == "convoy_supplies")
			return PRIMITIVE_CONVOY_INTERCEPT;

		if (missionId == "dynamic_minor_city_task")
			return PRIMITIVE_CLEAR_AREA;

		if (category == HST_EMissionCategory.HST_MISSION_CONVOY)
			return PRIMITIVE_CONVOY_INTERCEPT;
		if (category == HST_EMissionCategory.HST_MISSION_ASSASSINATION)
			return PRIMITIVE_KILL_HVT;
		if (category == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return PRIMITIVE_HOLD_AREA;
		if (category == HST_EMissionCategory.HST_MISSION_DESTROY)
			return PRIMITIVE_DESTROY_TARGET;
		if (category == HST_EMissionCategory.HST_MISSION_LOGISTICS)
			return PRIMITIVE_RECOVER_CARGO;
		if (category == HST_EMissionCategory.HST_MISSION_RESCUE)
			return PRIMITIVE_RESCUE_EXTRACT;
		if (category == HST_EMissionCategory.HST_MISSION_SUPPORT)
			return PRIMITIVE_DELIVER_SUPPLIES;

		return PRIMITIVE_CLEAR_AREA;
	}

	protected bool PollObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, int elapsedSeconds)
	{
		return PollPrimitiveObjective(state, preset, mission, objective, elapsedSeconds);
	}

	protected bool PollPrimitiveObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, int elapsedSeconds)
	{
		if (!state || !mission || !objective)
			return false;

		string primitive = objective.m_sRuntimePrimitive;
		if (primitive.IsEmpty())
			primitive = mission.m_sRuntimePrimitive;

		if (primitive == PRIMITIVE_HOLD_AREA)
			return PollHoldAreaObjective(state, preset, mission, objective, elapsedSeconds);
		if (primitive == PRIMITIVE_CLEAR_AREA)
			return PollClearAreaObjective(state, preset, mission, objective);
		if (primitive == PRIMITIVE_KILL_HVT)
			return PollKillHVTObjective(state, preset, mission, objective);
		if (primitive == PRIMITIVE_DESTROY_TARGET)
			return PollDestroyTargetObjective(state, preset, mission, objective);
		if (primitive == PRIMITIVE_RECOVER_CARGO)
			return PollRecoverCargoObjective(state, preset, mission, objective);
		if (primitive == PRIMITIVE_RESCUE_EXTRACT)
			return PollRescueExtractObjective(state, preset, mission, objective);
		if (primitive == PRIMITIVE_DELIVER_SUPPLIES)
			return PollDeliverSuppliesObjective(state, preset, mission, objective);
		if (primitive == PRIMITIVE_CONVOY_INTERCEPT)
			return PollConvoyInterceptObjective(state, preset, mission, objective);

		return PollFallbackObjective(state, preset, mission, objective, elapsedSeconds);
	}

	protected bool PollKillHVTObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		bool handledByAssets;
		bool assetChanged = PollMissionAssetObjective(state, preset, mission, objective, handledByAssets);
		if (handledByAssets)
			return assetChanged;

		if (IsRuntimeEntityDestroyed(objective.m_sLinkedRuntimeEntityId))
			return CompleteWorldObjective(objective);

		return false;
	}

	protected bool PollDestroyTargetObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		bool handledByAssets;
		bool assetChanged = PollMissionAssetObjective(state, preset, mission, objective, handledByAssets);
		if (handledByAssets)
			return assetChanged;

		if (IsRuntimeEntityDestroyed(objective.m_sLinkedRuntimeEntityId))
			return CompleteWorldObjective(objective);

		return false;
	}

	protected bool PollRecoverCargoObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		bool handledByAssets;
		bool assetChanged = PollMissionAssetObjective(state, preset, mission, objective, handledByAssets);
		if (handledByAssets)
			return assetChanged;

		return false;
	}

	protected bool PollRescueExtractObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		bool handledByAssets;
		bool assetChanged = PollMissionAssetObjective(state, preset, mission, objective, handledByAssets);
		if (handledByAssets)
			return assetChanged;

		return false;
	}

	protected bool PollDeliverSuppliesObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		bool handledByAssets;
		bool assetChanged = PollMissionAssetObjective(state, preset, mission, objective, handledByAssets);
		if (handledByAssets)
			return assetChanged;

		return false;
	}

	protected bool PollConvoyInterceptObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		bool handledByAssets;
		bool assetChanged = PollMissionAssetObjective(state, preset, mission, objective, handledByAssets);
		if (handledByAssets)
			return assetChanged;

		return false;
	}

	protected bool PollHoldAreaObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, int elapsedSeconds)
	{
		float playerRadius = ResolveAreaObjectiveRadiusMeters(state, mission, objective, PLAYER_OBJECTIVE_RADIUS_METERS);
		bool playerNear = IsAnyLivingPlayerNearObjective(objective.m_vPosition, playerRadius);
		if (!playerNear)
			return false;

		objective.m_bWorldDetected = true;
		if (mission.m_sRuntimePhase == PHASE_ACTIVE)
			mission.m_sRuntimePhase = PHASE_CONTACT;

		float hostileRadius = ResolveAreaObjectiveRadiusMeters(state, mission, objective, HOSTILE_OBJECTIVE_RADIUS_METERS);
		if (mission.m_sMissionId != "dynamic_defend_petros" && HasHostileActiveGroupNear(state, preset, objective.m_vPosition, hostileRadius))
		{
			mission.m_sRuntimePhase = PHASE_CONTACT;
			return false;
		}

		mission.m_sRuntimePhase = PHASE_HOLDING;
		return ProgressHoldObjective(objective, elapsedSeconds);
	}

	protected bool PollClearAreaObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		float playerRadius = ResolveAreaObjectiveRadiusMeters(state, mission, objective, PLAYER_OBJECTIVE_RADIUS_METERS);
		bool playerNear = IsAnyLivingPlayerNearObjective(objective.m_vPosition, playerRadius);
		if (!playerNear)
			return false;

		objective.m_bWorldDetected = true;
		float hostileRadius = ResolveAreaObjectiveRadiusMeters(state, mission, objective, HOSTILE_OBJECTIVE_RADIUS_METERS);
		if (HasUnresolvedClearAreaHostiles(state, preset, mission, objective, hostileRadius))
		{
			mission.m_sRuntimePhase = PHASE_CONTACT;
			return false;
		}

		return CompleteWorldObjective(objective);
	}

	protected float ResolveAreaObjectiveRadiusMeters(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, float defaultRadiusMeters)
	{
		float radius = defaultRadiusMeters;
		if (!state || !objective)
			return radius;

		if (objective.m_eType != HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA && objective.m_eType != HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA)
			return radius;

		HST_ZoneState zone = state.FindZone(objective.m_sTargetId);
		if (!zone && mission)
			zone = state.FindZone(mission.m_sTargetZoneId);
		if (zone && zone.m_iCaptureRadiusMeters > 0)
			radius = Math.Max(radius, zone.m_iCaptureRadiusMeters);

		return radius;
	}

	protected bool PollFallbackObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, int elapsedSeconds)
	{
		bool playerNear = IsAnyLivingPlayerNearObjective(objective.m_vPosition, PLAYER_OBJECTIVE_RADIUS_METERS);
		if (!playerNear)
			return false;

		objective.m_bWorldDetected = true;
		if (mission.m_sRuntimePhase == PHASE_ACTIVE)
			mission.m_sRuntimePhase = PHASE_CONTACT;

		objective.m_bAbstractFallback = true;
		return CompleteWorldObjective(objective);
	}

	protected bool PollMissionAssetObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, out bool handled)
	{
		handled = false;
		if (!state || !mission || !objective)
			return false;

		string role = ResolveObjectiveAssetRole(mission, objective);
		if (role.IsEmpty())
			return false;
		if (!HasMatchingObjectiveAssets(state, mission, objective) && mission.m_bRuntimeFallback && HasRuntimeEntity(mission.m_sRuntimeEntityId))
			return false;
		if (objective.m_sTargetId == "convoy")
		{
			handled = true;
			return false;
		}
		if ((objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA) && objective.m_sTargetId != "convoy")
			return false;

		bool changed;
		int satisfiedCount;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != role)
				continue;

			handled = true;
			if (SyncMissionAssetDestructionState(state, asset))
				changed = true;

			if (asset.m_bDestroyed)
			{
				if (asset.m_sKind == ASSET_KIND_CAPTIVE)
				{
					MarkRuntimeMissionFailed(state, mission, "Required captives were killed.");
					return true;
				}

				if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET || objective.m_sTargetId == "convoy")
					satisfiedCount++;

				continue;
			}

			if (objective.m_sTargetId == "convoy")
			{
				if (asset.m_bDelivered)
					satisfiedCount++;
				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET)
				continue;

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT)
			{
				if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO)
				{
					if (asset.m_bDelivered)
						satisfiedCount++;

					continue;
				}

				if (asset.m_bPickedUp)
					satisfiedCount++;

				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
			{
				if (asset.m_sRole == ROLE_CONVOY_CAPTIVE)
				{
					if (asset.m_bDelivered)
						satisfiedCount++;

					continue;
				}

				if (asset.m_bDelivered)
					satisfiedCount++;

				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES)
			{
				if (asset.m_bDelivered)
					satisfiedCount++;
			}
		}

		if (!handled)
			return false;

		if (objective.m_sTargetId == "convoy" && HasSatisfiedConvoyPayload(state, mission))
			satisfiedCount = Math.Max(satisfiedCount, Math.Max(1, objective.m_iRequiredCount));

		objective.m_iCurrentCount = Math.Min(Math.Max(1, objective.m_iRequiredCount), satisfiedCount);
		objective.m_iCurrentProgress = Math.Min(objective.m_iRequiredProgress, satisfiedCount);
		if (satisfiedCount >= Math.Max(1, objective.m_iRequiredCount))
			return CompleteWorldObjective(objective) || changed;

		return changed;
	}

	protected bool HasMatchingObjectiveAssets(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		if (!state || !mission || !objective)
			return false;

		string role = ResolveObjectiveAssetRole(mission, objective);
		if (role.IsEmpty())
			return false;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == role)
				return true;
		}

		return false;
	}

	protected bool SyncMissionAssetDestructionState(HST_CampaignState state, HST_MissionAssetState asset)
	{
		if (!state || !asset || asset.m_bDestroyed)
			return false;

		if (!IsRuntimeEntityDestroyed(asset.m_sEntityId))
			return false;

		asset.m_bDestroyed = true;
		asset.m_bAlive = false;
		asset.m_sLastInteraction = PHASE_DESTROYED;
		if (!IsZeroVector(asset.m_vCurrentPosition))
			asset.m_vLastKnownPosition = asset.m_vCurrentPosition;
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
			runtimeEntity.m_bDestroyed = true;

		return true;
	}

	protected void MarkMissionAssetPickedUp(HST_CampaignState state, HST_MissionAssetState asset)
	{
		if (!asset || asset.m_bPickedUp)
			return;

		asset.m_bPickedUp = true;
		asset.m_vCurrentPosition = asset.m_vTargetPosition;
		asset.m_vLastKnownPosition = asset.m_vCurrentPosition;
		asset.m_sLastInteraction = PHASE_LOADED;
		DeleteRuntimeEntity(asset.m_sEntityId);
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
			runtimeEntity.m_bRecovered = true;
	}

	protected void MarkMissionAssetDelivered(HST_CampaignState state, HST_MissionAssetState asset, vector deliveryPosition)
	{
		if (!asset || asset.m_bDelivered)
			return;

		asset.m_bPickedUp = true;
		asset.m_bDelivered = true;
		asset.m_vCurrentPosition = deliveryPosition;
		asset.m_vLastKnownPosition = deliveryPosition;
		asset.m_bAttachedToCarrier = false;
		asset.m_sCarriedByVehicleId = "";
		asset.m_sLastInteraction = PHASE_DELIVERED;
		if (asset.m_sKind == ASSET_KIND_CAPTIVE)
			DeleteRuntimeEntity(asset.m_sEntityId);
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			runtimeEntity.m_bRecovered = true;
			runtimeEntity.m_vPosition = deliveryPosition;
		}
	}

	protected bool ProgressHoldObjective(HST_MissionObjectiveState objective, int elapsedSeconds)
	{
		int requiredSeconds = objective.m_iRequiredHoldSeconds;
		if (requiredSeconds <= 0)
			requiredSeconds = DEFAULT_HOLD_SECONDS;

		objective.m_iHoldSeconds = Math.Min(requiredSeconds, objective.m_iHoldSeconds + elapsedSeconds);
		if (objective.m_iHoldSeconds < requiredSeconds)
			return true;

		objective.m_iCurrentProgress = objective.m_iRequiredProgress;
		objective.m_iCurrentCount = objective.m_iRequiredCount;
		objective.m_bComplete = true;
		return true;
	}

	protected bool CompleteWorldObjective(HST_MissionObjectiveState objective)
	{
		if (!objective || objective.m_bComplete)
			return false;

		objective.m_iCurrentProgress = objective.m_iRequiredProgress;
		objective.m_bComplete = true;
		return true;
	}

	protected bool RepairActiveMissionRuntimeAfterRestore(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;

		bool changed;
		string resolvedPrimitive = PrimitiveForMissionId(mission.m_sMissionId, HST_EMissionCategory.HST_MISSION_DYNAMIC);
		if (resolvedPrimitive.IsEmpty())
			resolvedPrimitive = PRIMITIVE_ABSTRACT_FALLBACK;

		if (mission.m_sRuntimePrimitive.IsEmpty() || mission.m_sRuntimePrimitive == PRIMITIVE_ABSTRACT_FALLBACK)
		{
			mission.m_sRuntimePrimitive = resolvedPrimitive;
			changed = true;
		}

		if (mission.m_eRuntimeMode == HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_ABSTRACT && mission.m_sRuntimePrimitive != PRIMITIVE_ABSTRACT_FALLBACK)
		{
			mission.m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_STATE_MACHINE;
			changed = true;
		}

		if (mission.m_sRuntimeType.IsEmpty() || mission.m_sRuntimeType == PRIMITIVE_ABSTRACT_FALLBACK)
		{
			mission.m_sRuntimeType = mission.m_sRuntimePrimitive;
			changed = true;
		}

		if (mission.m_sRuntimePhase.IsEmpty() || mission.m_sRuntimePhase == PHASE_CREATED)
		{
			if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
				mission.m_sRuntimePhase = PHASE_CONVOY_STAGING;
			else
				mission.m_sRuntimePhase = PHASE_ACTIVE;
			changed = true;
		}

		if (mission.m_sRuntimeEntityId.IsEmpty())
		{
			mission.m_sRuntimeEntityId = string.Format("runtime_%1_%2", mission.m_sInstanceId, mission.m_sRuntimePrimitive);
			changed = true;
		}

		if (mission.m_iRuntimeStartedAtSecond <= 0)
		{
			if (mission.m_iStartedAtSecond > 0)
				mission.m_iRuntimeStartedAtSecond = mission.m_iStartedAtSecond;
			else
				mission.m_iRuntimeStartedAtSecond = state.m_iElapsedSeconds;
			changed = true;
		}

		if (mission.m_iRuntimeHoldSeconds <= 0)
		{
			mission.m_iRuntimeHoldSeconds = DEFAULT_HOLD_SECONDS;
			changed = true;
		}

		if (mission.m_bRuntimeCleanupComplete)
		{
			mission.m_bRuntimeCleanupComplete = false;
			changed = true;
		}

		changed = EnsureRestoredMissionAssetsFromState(state, mission) || changed;
		changed = CompleteLegacyRecoverCargoDeliveryObjectives(state, mission) || changed;
		LinkObjectivesToMissionAssets(state, mission);
		if (ShouldRunRestoredMissionCarrierRestorePass(state, mission))
			changed = EnsureRestoredMissionCarrierVehicles(state, mission) || changed;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			changed = EnsureMissionAssetRuntimeEntityRecord(state, mission, asset) || changed;
			if (IsPhysicalConvoyAsset(mission, asset))
				continue;
			if (!state.m_bRestoredFromPersistence)
				continue;
			if (IsTerminalMissionAssetState(asset))
				continue;
			if (asset.m_sEntityId.IsEmpty() || HasRuntimeEntity(asset.m_sEntityId))
				continue;

			if (asset.m_bSpawned)
			{
				asset.m_bSpawned = false;
				changed = true;
			}

			HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtimeEntity)
			{
				if (runtimeEntity.m_bSpawned)
				{
					runtimeEntity.m_bSpawned = false;
					changed = true;
				}
				runtimeEntity.m_vPosition = ResolveRestoredMissionAssetPosition(asset);
			}
		}

		RefreshMissionObjectivesFromAssets(state, mission);
		UpdateMissionCountersFromAssets(state, mission);
		return changed;
	}

	protected bool IsPhysicalConvoyAsset(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!mission || !asset || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;

		return asset.m_sRole == ROLE_CONVOY_VEHICLE || asset.m_sRole == ROLE_CONVOY_PAYLOAD || asset.m_sRole == ROLE_CONVOY_CAPTIVE;
	}

	protected bool CompleteLegacyRecoverCargoDeliveryObjectives(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_RECOVER_CARGO)
			return false;

		bool changed;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (objective.m_sTargetId != "hq_delivery" || objective.m_eType != HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES)
				continue;

			if (objective.m_bFailed)
			{
				objective.m_bFailed = false;
				changed = true;
			}
			if (!objective.m_bComplete)
			{
				objective.m_iCurrentProgress = objective.m_iRequiredProgress;
				objective.m_iCurrentCount = Math.Max(1, objective.m_iRequiredCount);
				objective.m_bComplete = true;
				changed = true;
			}
		}

		return changed;
	}

	protected bool EnsureRestoredMissionAssetsFromState(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive == PRIMITIVE_ABSTRACT_FALLBACK)
			return false;

		vector targetPosition = ResolveRuntimePropPosition(state, mission);
		vector hqPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vHQPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		int cargoCount = Math.Max(1, mission.m_iRequiredCargoCount);
		int captiveCount = Math.Max(1, mission.m_iRequiredCaptiveCount);
		int vehicleCount = Math.Max(1, mission.m_iRequiredVehicleCount);
		bool changed;

		if (mission.m_sRuntimePrimitive == PRIMITIVE_KILL_HVT)
			return AddMissionAsset(state, mission, ASSET_KIND_CHARACTER, ROLE_HVT, PROP_HVT, targetPosition, targetPosition, 0);

		if (mission.m_sRuntimePrimitive == PRIMITIVE_DESTROY_TARGET)
		{
			if (mission.m_sMissionId == "destroy_or_steal_armor")
			{
				string vehiclePrefab = ResolveMissionVehiclePrefab(state, mission);
				for (int i = 0; i < vehicleCount; i++)
					changed = AddMissionAsset(state, mission, ASSET_KIND_VEHICLE, ROLE_DESTROY_TARGET, vehiclePrefab, targetPosition, hqPosition, i) || changed;
				return changed;
			}

			return AddMissionAsset(state, mission, ASSET_KIND_TARGET, ROLE_DESTROY_TARGET, PROP_DESTROY_TARGET, targetPosition, targetPosition, 0);
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_HOLD_AREA || mission.m_sRuntimePrimitive == PRIMITIVE_CLEAR_AREA)
			return AddMissionAsset(state, mission, ASSET_KIND_AREA, ROLE_HOLD_MARKER, PROP_HOLD_MARKER, targetPosition, targetPosition, 0);

		if (mission.m_sRuntimePrimitive == PRIMITIVE_DELIVER_SUPPLIES)
		{
			for (int i = 0; i < cargoCount; i++)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_CITY_SUPPLIES, PROP_CITY_SUPPLIES, hqPosition, targetPosition, i) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO)
		{
			for (int i = 0; i < cargoCount; i++)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_LOGISTICS_CARGO, ResolveLogisticsCargoPrefab(mission), targetPosition, hqPosition, i) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_RESCUE_EXTRACT)
		{
			for (int i = 0; i < captiveCount; i++)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CAPTIVE, ROLE_CAPTIVE, PROP_CAPTIVES, targetPosition, hqPosition, i) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
			return EnsureConvoyMissionSpecificAssets(state, mission);

		return AddMissionAsset(state, mission, ASSET_KIND_AREA, ROLE_HOLD_MARKER, PROP_HOLD_MARKER, targetPosition, targetPosition, 0);
	}

	protected bool EnsureMissionAssetRuntimeEntityRecord(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!state || !mission || !asset)
			return false;
		if (IsTerminalMissionAssetState(asset))
			return false;

		bool changed;
		if (asset.m_sEntityId.IsEmpty())
		{
			asset.m_sEntityId = asset.m_sAssetId;
			changed = true;
		}
		if (asset.m_sEntityId.IsEmpty())
			return changed;

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (!runtimeEntity)
		{
			runtimeEntity = new HST_MissionRuntimeEntityState();
			runtimeEntity.m_sRuntimeEntityId = asset.m_sEntityId;
			runtimeEntity.m_sMissionInstanceId = mission.m_sInstanceId;
			state.m_aMissionRuntimeEntities.Insert(runtimeEntity);
			changed = true;
		}

		vector position = ResolveRestoredMissionAssetPosition(asset);
		if (runtimeEntity.m_sMissionInstanceId.IsEmpty())
		{
			runtimeEntity.m_sMissionInstanceId = mission.m_sInstanceId;
			changed = true;
		}
		if (runtimeEntity.m_sKind != asset.m_sRole)
		{
			runtimeEntity.m_sKind = asset.m_sRole;
			changed = true;
		}
		if (runtimeEntity.m_sPrefab != asset.m_sPrefab)
		{
			runtimeEntity.m_sPrefab = asset.m_sPrefab;
			changed = true;
		}
		if (DistanceSq2D(runtimeEntity.m_vPosition, position) > 1.0)
		{
			runtimeEntity.m_vPosition = position;
			changed = true;
		}

		runtimeEntity.m_bDestroyed = asset.m_bDestroyed;
		runtimeEntity.m_bRecovered = asset.m_bPickedUp || asset.m_bDelivered;
		return changed;
	}

	protected vector ResolveRestoredMissionAssetPosition(HST_MissionAssetState asset)
	{
		if (!asset)
			return "0 0 0";
		if (!IsZeroVector(asset.m_vCurrentPosition))
			return asset.m_vCurrentPosition;
		if (!IsZeroVector(asset.m_vLastKnownPosition))
			return asset.m_vLastKnownPosition;
		if (!IsZeroVector(asset.m_vSourcePosition))
			return asset.m_vSourcePosition;
		return asset.m_vTargetPosition;
	}

	protected bool CleanupMissionRuntime(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_bRuntimeCleanupComplete)
			return false;

		DeleteMissionRuntimeEntities(state, mission);
		DeleteRuntimeEntity(mission.m_sRuntimeEntityId);
		CleanupMissionRuntimeRecords(state, mission);
		CleanupMissionAssetRecords(state, mission);
		mission.m_bRuntimeCleanupComplete = true;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == mission.m_sInstanceId)
				objective.m_bCleanupComplete = true;
		}

		return true;
	}

	protected void DeleteMissionRuntimeEntities(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sInstanceId.IsEmpty())
			return;

		foreach (HST_MissionRuntimeEntityState runtimeEntity : state.m_aMissionRuntimeEntities)
		{
			if (!runtimeEntity || runtimeEntity.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			HST_MissionAssetState runtimeAsset = FindMissionAssetByEntityId(state, runtimeEntity.m_sRuntimeEntityId);
			if (IsPreservedConvoyAssetAfterCrewElimination(mission, runtimeAsset))
				continue;

			DeleteRuntimeEntity(runtimeEntity.m_sRuntimeEntityId);
		}

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (IsPreservedConvoyAssetAfterCrewElimination(mission, asset))
				continue;

			DeleteRuntimeEntity(asset.m_sEntityId);
		}
	}

	protected bool HasRuntimeEntity(string runtimeEntityId)
	{
		return GetRuntimeEntity(runtimeEntityId) != null;
	}

	protected void DeleteRuntimeEntity(string runtimeEntityId)
	{
		if (runtimeEntityId.IsEmpty())
			return;

		for (int i = m_aRuntimeEntityIds.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeEntityIds[i] != runtimeEntityId)
				continue;

			if (i < m_aRuntimeEntities.Count())
			{
				IEntity entity = m_aRuntimeEntities[i];
				if (entity)
					SCR_EntityHelper.DeleteEntityAndChildren(entity);
				m_aRuntimeEntities.Remove(i);
			}
			m_aRuntimeEntityIds.Remove(i);
		}
	}

	protected void CleanupMissionRuntimeRecords(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sInstanceId.IsEmpty())
			return;

		for (int i = state.m_aMissionRuntimeEntities.Count() - 1; i >= 0; i--)
		{
			HST_MissionRuntimeEntityState runtimeEntity = state.m_aMissionRuntimeEntities[i];
			if (!runtimeEntity || runtimeEntity.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			HST_MissionAssetState runtimeAsset = FindMissionAssetByEntityId(state, runtimeEntity.m_sRuntimeEntityId);
			if (IsPreservedConvoyAssetAfterCrewElimination(mission, runtimeAsset))
				continue;

			runtimeEntity.m_bDestroyed = true;
			state.m_aMissionRuntimeEntities.Remove(i);
		}
	}

	protected void CleanupMissionAssetRecords(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sInstanceId.IsEmpty())
			return;

		for (int i = state.m_aMissionAssets.Count() - 1; i >= 0; i--)
		{
			HST_MissionAssetState asset = state.m_aMissionAssets[i];
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (IsPreservedConvoyAssetAfterCrewElimination(mission, asset))
				continue;

			asset.m_bDestroyed = asset.m_bDestroyed || !asset.m_bDelivered;
			state.m_aMissionAssets.Remove(i);
		}
	}

	protected HST_MissionAssetState FindMissionAssetByEntityId(HST_CampaignState state, string entityId)
	{
		if (!state || entityId.IsEmpty())
			return null;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sEntityId == entityId)
				return asset;
		}

		return null;
	}

	protected bool IsRuntimeEntityDestroyed(string runtimeEntityId)
	{
		IEntity entity = GetRuntimeEntity(runtimeEntityId);
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return damageManager && damageManager.GetState() == EDamageState.DESTROYED;
	}

	protected IEntity GetRuntimeEntity(string runtimeEntityId)
	{
		if (runtimeEntityId.IsEmpty())
			return null;

		IEntity resolved;
		for (int i = m_aRuntimeEntityIds.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeEntityIds[i] != runtimeEntityId)
				continue;

			if (i >= m_aRuntimeEntities.Count())
			{
				m_aRuntimeEntityIds.Remove(i);
				continue;
			}

			IEntity entity = m_aRuntimeEntities[i];
			if (!entity)
			{
				m_aRuntimeEntityIds.Remove(i);
				m_aRuntimeEntities.Remove(i);
				continue;
			}

			if (!resolved)
			{
				resolved = entity;
				continue;
			}

			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			m_aRuntimeEntityIds.Remove(i);
			m_aRuntimeEntities.Remove(i);
		}

		return resolved;
	}

	protected int ResolveConvoyArrivalSeconds(HST_ActiveMissionState mission)
	{
		int duration = mission.m_iActiveUntilSecond - mission.m_iStartedAtSecond;
		if (duration <= 0)
			duration = 3600;

		return Math.Max(300, duration * 2 / 3);
	}

	protected bool AreRuntimeObjectivesComplete(HST_CampaignState state, string instanceId)
	{
		bool found;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != instanceId)
				continue;

			found = true;
			if (!objective.m_bComplete || objective.m_bFailed)
				return false;
		}

		return found;
	}

	protected void MarkRuntimeMissionFailed(HST_CampaignState state, HST_ActiveMissionState mission, string reason)
	{
		if (!state || !mission)
			return;

		mission.m_sRuntimePhase = PHASE_FAILED;
		mission.m_sRuntimeFailureReason = reason;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete)
				continue;

			objective.m_bFailed = true;
		}
	}

	protected void UpdateMissionCountersFromObjectives(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;

		int cargo;
		int captives;
		int vehicles;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || !objective.m_bComplete)
				continue;

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES)
				cargo += Math.Max(1, objective.m_iRequiredCount);
			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
				captives += Math.Max(1, objective.m_iRequiredCount);
			if (objective.m_sTargetId.Contains("armor"))
				vehicles += Math.Max(1, objective.m_iRequiredCount);
		}

		mission.m_iRecoveredCargoCount = Math.Min(mission.m_iRequiredCargoCount, cargo);
		mission.m_iExtractedCaptiveCount = Math.Min(mission.m_iRequiredCaptiveCount, captives);
		mission.m_iCapturedVehicleCount = Math.Min(mission.m_iRequiredVehicleCount, vehicles);
	}

	protected void EnsureMissionHostileGroup(HST_CampaignState state, HST_CampaignPreset preset, HST_MissionDefinition definition, HST_ActiveMissionState mission)
	{
		if (!state || !preset || !definition || !mission || mission.m_sTargetZoneId.IsEmpty())
			return;

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT || definition.m_sMissionId == "dynamic_minor_city_task")
			return;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
			return;

		string groupId = "mission_group_" + mission.m_sInstanceId;
		if (state.FindActiveGroup(groupId))
			return;

		HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
		if (!zone)
			return;

		string factionKey = zone.m_sOwnerFactionKey;
		if (factionKey.IsEmpty() || factionKey == preset.m_sResistanceFactionKey)
			factionKey = preset.m_sOccupierFactionKey;

		HST_ActiveGroupState group = new HST_ActiveGroupState();
		vector guardPosition = ResolveMissionGuardPosition(state, mission);
		group.m_sGroupId = groupId;
		group.m_sZoneId = zone.m_sZoneId;
		group.m_sFactionKey = factionKey;
		group.m_sPrefab = SelectMissionGroupPrefab(state, zone, factionKey, definition);
		group.m_sRouteId = zone.m_sPatrolRouteId;
		group.m_vSourcePosition = guardPosition;
		group.m_vTargetPosition = guardPosition;
		group.m_vPosition = HST_WorldPositionService.ResolveSafeGroundPosition(guardPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 4.0);
		group.m_sRuntimeStatus = "mission_guard";
		group.m_iInfantryCount = ResolveMissionGuardInfantryCount(state, definition);
		group.m_iVehicleCount = ResolveMissionGuardVehicleCount(definition);
		group.m_iLastSeenAliveCount = group.m_iInfantryCount + group.m_iVehicleCount;
		group.m_iSurvivorInfantryCount = group.m_iInfantryCount;
		group.m_iSurvivorVehicleCount = group.m_iVehicleCount;
		state.m_aActiveGroups.Insert(group);
	}

	protected vector ResolveMissionGuardPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
		{
			foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
			{
				if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != ROLE_CONVOY_VEHICLE)
					continue;

				return OffsetMissionAssetPosition(asset.m_vSourcePosition, 3);
			}
		}

		return ResolveRuntimePropPosition(state, mission);
	}

	protected string SelectMissionGroupPrefab(HST_CampaignState state, HST_ZoneState zone, string factionKey, HST_MissionDefinition definition)
	{
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (!faction)
			return "";

		int seed = state.m_iCampaignSeed + state.m_iElapsedSeconds + zone.m_sZoneId.Length() * 17 + definition.m_sMissionId.Length() * 31;
		array<string> candidates = {};
		AppendUniqueGroupPrefabs(candidates, faction.m_aPatrolGroupPrefabs);
		AppendUniqueGroupPrefabs(candidates, faction.m_aGroupPrefabs);
		return SelectRandomGroupPrefab(candidates, seed);
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

	protected string SelectRandomGroupPrefab(array<string> prefabs, int seed)
	{
		if (!prefabs || prefabs.Count() == 0)
			return "";

		return prefabs[HST_DefaultCatalog.PositiveMod(seed, prefabs.Count())];
	}

	protected int ResolveMissionGuardInfantryCount(HST_CampaignState state, HST_MissionDefinition definition)
	{
		int count = 3 + state.m_iWarLevel;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			count += 3;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			count += 2;
		if (definition.m_sMissionId == "assassinate_specops" || definition.m_sMissionId == "dynamic_defend_petros")
			count += 4;
		return Math.Min(12, count);
	}

	protected int ResolveMissionGuardVehicleCount(HST_MissionDefinition definition)
	{
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return 1;
		if (definition.m_sMissionId == "destroy_or_steal_armor")
			return 1;
		return 0;
	}

	protected int ResolveHoldSeconds(string primitive, HST_MissionDefinition definition)
	{
		if (primitive == PRIMITIVE_HOLD_AREA)
		{
			if (definition && definition.m_sMissionId == "dynamic_defend_petros")
				return 90;

			return DEFAULT_HOLD_SECONDS;
		}

		return 0;
	}

	protected int ResolveObjectiveHoldSeconds(HST_MissionObjectiveState objective, string primitive, HST_MissionDefinition definition)
	{
		if (objective && objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA)
			return ResolveHoldSeconds(primitive, definition);

		return 0;
	}

	protected bool IsAnyLivingPlayerNearObjective(vector position, float radiusMeters)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		float radiusSq = radiusMeters * radiusMeters;
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (!IsLivingPlayerEntity(playerEntity))
				continue;

			if (DistanceSq2D(playerEntity.GetOrigin(), position) <= radiusSq)
				return true;
		}

		return false;
	}

	protected bool HasHostileActiveGroupNear(HST_CampaignState state, HST_CampaignPreset preset, vector position, float radiusMeters)
	{
		if (!state)
			return false;

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		float radiusSq = radiusMeters * radiusMeters;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (!group || group.m_sFactionKey == resistanceFactionKey)
				continue;
			if (group.m_sGroupId.Contains(PERSISTENCE_SMOKE_PREFIX))
				continue;
			int hostileStrength = ResolveHostileActiveGroupStrength(group);
			if (hostileStrength <= 0)
				continue;

			if (DistanceSq2D(group.m_vPosition, position) <= radiusSq)
				return true;
		}

		return false;
	}

	protected bool HasUnresolvedClearAreaHostiles(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, float radiusMeters)
	{
		if (!state || !mission || !objective)
			return false;

		if (HasHostileActiveGroupNear(state, preset, objective.m_vPosition, radiusMeters))
			return true;

		HST_ZoneState targetZone = ResolveObjectiveTargetZone(state, mission, objective);
		if (!targetZone)
			return false;

		if (HasHostileActiveGroupForZone(state, preset, targetZone.m_sZoneId))
			return true;

		return HasPendingTargetZoneHostileProjection(state, preset, targetZone);
	}

	protected HST_ZoneState ResolveObjectiveTargetZone(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		if (!state || !objective)
			return null;

		HST_ZoneState zone;
		if (!objective.m_sTargetZoneId.IsEmpty())
			zone = state.FindZone(objective.m_sTargetZoneId);
		if (!zone && mission && !mission.m_sTargetZoneId.IsEmpty())
			zone = state.FindZone(mission.m_sTargetZoneId);
		if (!zone && !objective.m_sTargetId.IsEmpty())
			zone = state.FindZone(objective.m_sTargetId);

		return zone;
	}

	protected bool HasHostileActiveGroupForZone(HST_CampaignState state, HST_CampaignPreset preset, string zoneId)
	{
		if (!state || zoneId.IsEmpty())
			return false;

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (!group || group.m_bQRF || group.m_sZoneId != zoneId || group.m_sFactionKey == resistanceFactionKey)
				continue;
			if (group.m_sGroupId.Contains(PERSISTENCE_SMOKE_PREFIX))
				continue;
			if (ResolveHostileActiveGroupStrength(group) > 0)
				return true;
		}

		return false;
	}

	protected bool HasPendingTargetZoneHostileProjection(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState targetZone)
	{
		if (!state || !targetZone)
			return false;

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;
		if (targetZone.m_sOwnerFactionKey.IsEmpty() || targetZone.m_sOwnerFactionKey == resistanceFactionKey)
			return false;

		if (targetZone.m_iActiveInfantryCount > 0 || targetZone.m_iActiveVehicleCount > 0)
			return true;
		if (targetZone.m_bActive)
			return false;

		HST_GarrisonState garrison = state.FindGarrison(targetZone.m_sZoneId, targetZone.m_sOwnerFactionKey);
		if (!garrison)
			return false;

		return garrison.m_iInfantryCount > 0 || garrison.m_iVehicleCount > 0;
	}

	protected int ResolveHostileActiveGroupStrength(HST_ActiveGroupState group)
	{
		if (!group)
			return 0;
		if (group.m_sRuntimeStatus == "eliminated" || group.m_sRuntimeStatus == "folded" || group.m_sRuntimeStatus == "spawn_failed" || group.m_sRuntimeStatus == PHASE_CONVOY_ELIMINATED)
			return 0;
		if (group.m_bSpawnAttempted && !group.m_bSpawnedEntity)
			return 0;

		int liveStrength = Math.Max(group.m_iLastSeenAliveCount, group.m_iSurvivorInfantryCount + group.m_iSurvivorVehicleCount);
		if (liveStrength > 0)
			return liveStrength;

		if (group.m_bSpawnedEntity)
			return Math.Max(group.m_iInfantryCount, group.m_iVehicleCount);

		return 0;
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

	protected bool IsEntityInVehicle(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(entity.FindComponent(SCR_CompartmentAccessComponent));
		return access && access.IsInCompartment() && access.GetVehicle() != null;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}
}
