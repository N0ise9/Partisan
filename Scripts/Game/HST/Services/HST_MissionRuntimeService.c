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
	static const string PHASE_EXTRACTING = "extracting";
	static const string PHASE_DELIVERING = "delivering";
	static const string PHASE_HOLDING = "holding";
	static const string PHASE_FAILED = "failed";
	static const string PHASE_LOADED = "loaded";
	static const string PHASE_UNLOADED = "unloaded";
	static const string PHASE_DELIVERED = "delivered";
	static const string PHASE_DESTROYED = "destroyed";
	static const string PHASE_CAPTURED = "captured";
	static const string PROP_HVT = "{6985327711303700}Prefabs/Objects/HST/HST_MissionProp_HVT.et";
	static const string PROP_DESTROY_TARGET = "{6985327711303710}Prefabs/Objects/HST/HST_MissionProp_DestroyTarget.et";
	static const string PROP_CARGO = "{6985327711303720}Prefabs/Objects/HST/HST_MissionProp_Cargo.et";
	static const string PROP_CAPTIVES = "{6985327711303730}Prefabs/Objects/HST/HST_MissionProp_Captives.et";
	static const string PROP_HOLD_MARKER = "{6985327711303740}Prefabs/Objects/HST/HST_MissionProp_HoldMarker.et";
	static const string PROP_CITY_SUPPLIES = "{6985327711303750}Prefabs/Objects/HST/HST_MissionProp_CitySupplies.et";
	static const string PROP_CONVOY_PAYLOAD = "{6985327711303760}Prefabs/Objects/HST/HST_MissionProp_ConvoyPayload.et";
	static const string PROP_BANK_MONEY = "{6985327711303770}Prefabs/Objects/HST/HST_MissionProp_BankMoney.et";
	static const string PROP_RESOURCE_CACHE = "{6985327711303780}Prefabs/Objects/HST/HST_MissionProp_ResourceCache.et";
	static const string PROP_CONVOY_VEHICLE = "Prefabs/Vehicles/Wheeled/S1203/S1203_base.et";
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
	static const int DEFAULT_HOLD_SECONDS = 45;
	static const int DEFAULT_ASSET_INTERACTION_RADIUS_METERS = 18;
	static const int DEFAULT_DELIVERY_RADIUS_METERS = 45;
	static const int DEFAULT_MISSION_CARRIER_CAPACITY = 6;

	protected ref array<string> m_aRuntimeEntityIds = {};
	protected ref array<IEntity> m_aRuntimeEntities = {};

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

			if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			{
				changed = CleanupMissionRuntime(state, mission) || changed;
				continue;
			}

			changed = EnsureRestoredMissionRuntime(state, mission) || changed;
			changed = EnsureMissionRuntimeProp(state, mission) || changed;
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
				mission.m_iRuntimeCounterB = ResolveConvoyArrivalSeconds(mission);
			mission.m_iRuntimeETASeconds = Math.Max(0, mission.m_iRuntimeCounterB - mission.m_iRuntimeCounterA);
			if (UpdateConvoyRouteState(state, mission))
				changed = true;

			if (mission.m_iRuntimeCounterA >= mission.m_iRuntimeCounterB && !AreRuntimeObjectivesComplete(state, mission.m_sInstanceId))
			{
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

	protected bool EnsureMissionAssetsInitialized(HST_CampaignState state, HST_MissionDefinition definition, HST_ActiveMissionState mission)
	{
		if (!state || !definition || !mission || mission.m_sRuntimePrimitive == PRIMITIVE_ABSTRACT_FALLBACK)
			return false;

		if (state.CountMissionAssets(mission.m_sInstanceId) > 0)
			return false;

		vector targetPosition = ResolveRuntimePropPosition(state, mission);
		vector hqPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vHQPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		int cargoCount = Math.Max(1, definition.m_iCargoCount);
		int captiveCount = Math.Max(1, definition.m_iCaptiveCount);
		int vehicleCount = Math.Max(1, definition.m_iVehicleCount);
		bool changed;

		if (mission.m_sRuntimePrimitive == PRIMITIVE_KILL_HVT)
		{
			AddMissionAsset(state, mission, ASSET_KIND_CHARACTER, ROLE_HVT, PROP_HVT, targetPosition, targetPosition, 0);
			return true;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_DESTROY_TARGET)
		{
			if (mission.m_sMissionId == "destroy_or_steal_armor")
			{
				for (int i = 0; i < vehicleCount; i++)
					AddMissionAsset(state, mission, ASSET_KIND_VEHICLE, ROLE_DESTROY_TARGET, PROP_CONVOY_VEHICLE, targetPosition, hqPosition, i);
				return true;
			}

			AddMissionAsset(state, mission, ASSET_KIND_TARGET, ROLE_DESTROY_TARGET, PROP_DESTROY_TARGET, targetPosition, targetPosition, 0);
			return true;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_HOLD_AREA || mission.m_sRuntimePrimitive == PRIMITIVE_CLEAR_AREA)
		{
			AddMissionAsset(state, mission, ASSET_KIND_AREA, ROLE_HOLD_MARKER, PROP_HOLD_MARKER, targetPosition, targetPosition, 0);
			return true;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_DELIVER_SUPPLIES)
		{
			for (int i = 0; i < cargoCount; i++)
				AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_CITY_SUPPLIES, PROP_CITY_SUPPLIES, hqPosition, targetPosition, i);
			return true;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO)
		{
			for (int i = 0; i < cargoCount; i++)
				AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_LOGISTICS_CARGO, ResolveLogisticsCargoPrefab(mission), targetPosition, hqPosition, i);
			return true;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_RESCUE_EXTRACT)
		{
			for (int i = 0; i < captiveCount; i++)
				AddMissionAsset(state, mission, ASSET_KIND_CAPTIVE, ROLE_CAPTIVE, PROP_CAPTIVES, targetPosition, hqPosition, i);
			return true;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
		{
			vector convoyStart = ResolveConvoyStartPosition(state, mission);
			vector convoyEnd = ResolveConvoyEndPosition(state, mission);
			for (int i = 0; i < vehicleCount; i++)
				AddMissionAsset(state, mission, ASSET_KIND_VEHICLE, ROLE_CONVOY_VEHICLE, PROP_CONVOY_VEHICLE, convoyStart, convoyEnd, i);

			if (mission.m_sMissionId == "convoy_prisoners")
			{
				for (int i = 0; i < captiveCount; i++)
					AddMissionAsset(state, mission, ASSET_KIND_CAPTIVE, ROLE_CONVOY_CAPTIVE, PROP_CAPTIVES, convoyStart, hqPosition, i);
			}
			else if (mission.m_sMissionId != "convoy_reinforcements")
			{
				for (int i = 0; i < cargoCount; i++)
					AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_CONVOY_PAYLOAD, PROP_CONVOY_PAYLOAD, convoyStart, hqPosition, i);
			}

			return true;
		}

		changed = AddMissionAsset(state, mission, ASSET_KIND_AREA, ROLE_HOLD_MARKER, PROP_HOLD_MARKER, targetPosition, targetPosition, 0);
		return changed;
	}

	protected bool AddMissionAsset(HST_CampaignState state, HST_ActiveMissionState mission, string kind, string role, string prefab, vector sourcePosition, vector targetPosition, int index)
	{
		if (!state || !mission || role.IsEmpty())
			return false;

		string assetId = string.Format("asset_%1_%2_%3", mission.m_sInstanceId, role, index);
		if (state.FindMissionAsset(assetId))
			return false;

		HST_MissionAssetState asset = new HST_MissionAssetState();
		asset.m_sAssetId = assetId;
		asset.m_sEntityId = assetId;
		asset.m_sMissionInstanceId = mission.m_sInstanceId;
		asset.m_sKind = kind;
		asset.m_sRole = role;
		asset.m_sPrefab = prefab;
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

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
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

			if (asset.m_bPickedUp && !asset.m_bDelivered)
			{
				hasUsableAsset = true;
				continue;
			}

			if (asset.m_sPrefab.IsEmpty())
				continue;

			if (HasRuntimeEntity(asset.m_sEntityId))
			{
				hasUsableAsset = true;
				continue;
			}

			vector position;
			if (asset.m_sKind == ASSET_KIND_VEHICLE)
			{
				if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(asset.m_vCurrentPosition, position, false))
					position = HST_WorldPositionService.ResolveGroundPosition(asset.m_vCurrentPosition, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, false);
			}
			else
			{
				position = HST_WorldPositionService.ResolveSafeGroundPosition(asset.m_vCurrentPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false, 2.0);
			}
			vector angles = BuildRuntimePropAngles(state, mission);
			GenericEntity entity = respawnSystem.DoSpawn(asset.m_sPrefab, position, angles);
			if (!entity && asset.m_sPrefab != PROP_HOLD_MARKER)
				entity = respawnSystem.DoSpawn(PROP_HOLD_MARKER, position, angles);

			if (!entity)
			{
				Print(string.Format("h-istasi mission runtime | asset spawn failed for %1 using %2", asset.m_sAssetId, asset.m_sPrefab), LogLevel.WARNING);
				continue;
			}

			HST_MissionAssetComponent assetComponent = HST_MissionAssetComponent.Cast(entity.FindComponent(HST_MissionAssetComponent));
			if (assetComponent)
				assetComponent.ConfigureMissionAsset(asset.m_sAssetId, mission.m_sInstanceId, asset.m_sRole);

			asset.m_bSpawned = true;
			asset.m_vCurrentPosition = position;
			asset.m_vLastKnownPosition = position;
			m_aRuntimeEntityIds.Insert(asset.m_sEntityId);
			m_aRuntimeEntities.Insert(entity);
			RegisterAssetRuntimeEntityState(state, asset, position, angles);
			hasUsableAsset = true;
			Print(string.Format("h-istasi mission runtime | spawned asset %1 role %2 for %3 at %4", asset.m_sPrefab, asset.m_sRole, mission.m_sInstanceId, position));
		}

		return foundAsset && hasUsableAsset;
	}

	protected bool TrySpawnMissionRuntimeProp(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimeEntityId.IsEmpty() || mission.m_sRuntimePrimitive == PRIMITIVE_ABSTRACT_FALLBACK)
			return false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
			return false;

		vector position = ResolveRuntimePropPosition(state, mission);
		string prefab = SelectRuntimePropPrefab(mission.m_sRuntimePrimitive);
		if (prefab.IsEmpty())
			return false;

		vector angles = BuildRuntimePropAngles(state, mission);
		GenericEntity entity = respawnSystem.DoSpawn(prefab, position, angles);
		if (!entity && prefab != PROP_HOLD_MARKER)
			entity = respawnSystem.DoSpawn(PROP_HOLD_MARKER, position, angles);

		if (!entity)
		{
			Print(string.Format("h-istasi mission runtime | prop spawn failed for %1 using %2", mission.m_sInstanceId, prefab), LogLevel.WARNING);
			return false;
		}

		m_aRuntimeEntityIds.Insert(mission.m_sRuntimeEntityId);
		m_aRuntimeEntities.Insert(entity);
		RegisterRuntimeEntityState(state, mission, prefab, position, angles);
		Print(string.Format("h-istasi mission runtime | spawned prop %1 for %2 at %3", prefab, mission.m_sInstanceId, position));
		return true;
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
		if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
		{
			result = "h-istasi mission | failed: mission is no longer active";
			return false;
		}

		vector validationPosition = ResolveInteractionValidationPosition(asset, commandId);
		float radius = ResolveInteractionRadius(asset, commandId);
		if (DistanceSq2D(playerPosition, validationPosition) > radius * radius)
		{
			result = string.Format("h-istasi mission | failed: move within %1m of %2", Math.Round(radius), BuildAssetShortLabel(asset));
			return false;
		}

		bool changed;
		if (commandId == "mission_asset_load")
			changed = ApplyLoadInteraction(state, mission, asset, playerId, playerPosition, result, eventType);
		else if (commandId == "mission_asset_unload")
			changed = ApplyUnloadInteraction(state, mission, asset, playerId, playerPosition, result, eventType);
		else if (commandId == "mission_asset_deliver")
			changed = ApplyDeliverInteraction(state, mission, asset, playerPosition, result, eventType);
		else if (commandId == "mission_captive_extract")
			changed = ApplyCaptiveExtractInteraction(state, mission, asset, playerId, playerPosition, result, eventType);
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
		mission.m_sLastRuntimeEventKey = eventType;
		RefreshMissionObjectivesFromAssets(state, mission);
		UpdateMissionCountersFromAssets(state, mission);
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
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			vector position = ResolveInteractionValidationPosition(asset, commandId);
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
			return IsTransportableAsset(asset) && asset.m_bPickedUp && !asset.m_bDestroyed && IsPlayerCarrier(asset, playerId);

		if (commandId == "mission_asset_deliver")
			return IsTransportableAsset(asset) && asset.m_bPickedUp && !asset.m_bDestroyed;

		if (commandId == "mission_captive_extract")
			return asset.m_sKind == ASSET_KIND_CAPTIVE && !asset.m_bDestroyed;

		if (commandId == "mission_vehicle_capture")
			return asset.m_sKind == ASSET_KIND_VEHICLE && !asset.m_bDestroyed;

		if (commandId == "mission_asset_sabotage")
			return (asset.m_sKind == ASSET_KIND_TARGET || asset.m_sKind == ASSET_KIND_CHARACTER || asset.m_sKind == ASSET_KIND_VEHICLE) && !asset.m_bDestroyed;

		return false;
	}

	protected bool ApplyLoadInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, int playerId, vector playerPosition, out string result, out string eventType)
	{
		if (!CanCarrierAcceptAsset(state, playerId, asset))
		{
			result = "h-istasi mission | failed: mission carrier is full";
			return false;
		}

		MarkMissionAssetPickedUp(state, asset);
		asset.m_sCarriedByVehicleId = BuildPlayerCarrierId(playerId);
		asset.m_bAttachedToCarrier = true;
		asset.m_vCurrentPosition = playerPosition;
		asset.m_vLastKnownPosition = playerPosition;
		asset.m_sLastInteraction = PHASE_LOADED;
		mission.m_sRuntimePhase = PHASE_LOADED;
		result = "h-istasi mission | loaded " + BuildAssetShortLabel(asset);
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
		MarkMissionAssetDelivered(state, asset, ResolveDeliveryPosition(asset, playerPosition));
		asset.m_bAttachedToCarrier = false;
		asset.m_sCarriedByVehicleId = "";
		asset.m_sLastInteraction = PHASE_DELIVERED;
		mission.m_sRuntimePhase = PHASE_DELIVERED;
		result = "h-istasi mission | delivered " + BuildAssetShortLabel(asset);
		eventType = "delivered";
		return true;
	}

	protected bool ApplyCaptiveExtractInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, int playerId, vector playerPosition, out string result, out string eventType)
	{
		if (!asset.m_bPickedUp)
			return ApplyLoadInteraction(state, mission, asset, playerId, playerPosition, result, eventType);

		return ApplyDeliverInteraction(state, mission, asset, playerPosition, result, eventType);
	}

	protected bool ApplyVehicleCaptureInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ArsenalService arsenal, vector playerPosition, out string result, out string eventType)
	{
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

		mission.m_sRuntimePhase = PHASE_CAPTURED;
		result = "h-istasi mission | captured " + BuildAssetShortLabel(asset);
		eventType = "captured";
		return true;
	}

	protected bool ApplySabotageInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, vector playerPosition, out string result, out string eventType)
	{
		asset.m_bDestroyed = true;
		asset.m_bAlive = false;
		asset.m_bAttachedToCarrier = false;
		asset.m_sCarriedByVehicleId = "";
		asset.m_vCurrentPosition = playerPosition;
		asset.m_vLastKnownPosition = playerPosition;
		asset.m_sLastInteraction = PHASE_DESTROYED;
		DeleteRuntimeEntity(asset.m_sEntityId);
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			runtimeEntity.m_bDestroyed = true;
			runtimeEntity.m_vPosition = playerPosition;
		}

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(asset.m_sEntityId);
		if (runtimeVehicle)
		{
			runtimeVehicle.m_bDeleted = true;
			runtimeVehicle.m_vPosition = playerPosition;
		}

		mission.m_sRuntimePhase = PHASE_DESTROYED;
		result = "h-istasi mission | sabotaged " + BuildAssetShortLabel(asset);
		eventType = "sabotaged";
		return true;
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

	protected string BuildPlayerCarrierId(int playerId)
	{
		return string.Format("player_%1_mission_carrier", playerId);
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

	protected int CountSatisfiedAssetsForObjective(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, string role)
	{
		int satisfied;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != role)
				continue;

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET)
			{
				if (asset.m_bDestroyed || (asset.m_sKind == ASSET_KIND_VEHICLE && asset.m_bDelivered))
					satisfied++;
				continue;
			}

			if (objective.m_sTargetId == "convoy")
			{
				if (asset.m_bDestroyed || asset.m_bDelivered)
					satisfied++;
				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT)
			{
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
		mission.m_iRecoveredCargoCount = Math.Min(mission.m_iRequiredCargoCount, picked);
		mission.m_iExtractedCaptiveCount = Math.Min(mission.m_iRequiredCaptiveCount, captives);
		mission.m_iCapturedVehicleCount = Math.Min(mission.m_iRequiredVehicleCount, vehicles);
	}

	protected bool UpdateConvoyRouteState(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_iRuntimeCounterB <= 0)
			return false;

		float progress = Math.Min(1.0, mission.m_iRuntimeCounterA * 1.0 / mission.m_iRuntimeCounterB);
		bool changed;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != ROLE_CONVOY_VEHICLE)
				continue;
			if (asset.m_bDestroyed || asset.m_bDelivered)
				continue;

			vector position = LerpPosition(asset.m_vSourcePosition, asset.m_vTargetPosition, progress);
			position = HST_WorldPositionService.ResolveGroundPosition(position, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
			if (DistanceSq2D(asset.m_vCurrentPosition, position) < 9)
				continue;

			SetMissionAssetPosition(state, asset, position);
			changed = true;
		}

		return changed;
	}

	protected void SetMissionAssetPosition(HST_CampaignState state, HST_MissionAssetState asset, vector position)
	{
		if (!state || !asset)
			return;

		asset.m_vCurrentPosition = position;
		asset.m_vLastKnownPosition = position;
		IEntity entity = GetRuntimeEntity(asset.m_sEntityId);
		if (entity)
			entity.SetOrigin(position);

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
			runtimeEntity.m_vPosition = position;

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(asset.m_sEntityId);
		if (runtimeVehicle)
			runtimeVehicle.m_vPosition = position;
	}

	protected vector LerpPosition(vector sourcePosition, vector targetPosition, float progress)
	{
		vector result;
		result[0] = sourcePosition[0] + (targetPosition[0] - sourcePosition[0]) * progress;
		result[1] = sourcePosition[1] + (targetPosition[1] - sourcePosition[1]) * progress;
		result[2] = sourcePosition[2] + (targetPosition[2] - sourcePosition[2]) * progress;
		return result;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
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

	protected vector ResolveConvoyStartPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		if (route)
			return HST_WorldPositionService.ResolveSafeGroundPosition(route.m_vStartPosition, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, false, 5.0);

		return OffsetMissionAssetPosition(ResolveRuntimePropPosition(state, mission), -2);
	}

	protected vector ResolveConvoyEndPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		if (route)
			return HST_WorldPositionService.ResolveSafeGroundPosition(route.m_vEndPosition, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, false, 5.0);

		return ResolveRuntimePropPosition(state, mission);
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
		return HST_WorldPositionService.ResolveSafeGroundPosition(offsetPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false, 2.0);
	}

	protected vector ResolveRuntimePropPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == mission.m_sInstanceId)
				return HST_WorldPositionService.ResolveSafeGroundPosition(objective.m_vPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false, 2.0);
		}

		if (!mission.m_sTargetZoneId.IsEmpty())
		{
			HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
			if (zone)
				return HST_WorldPositionService.ResolveSafeGroundPosition(zone.m_vPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false, 2.0);
		}

		return HST_WorldPositionService.ResolveSafeGroundPosition(state.m_vHQPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false, 2.0);
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
		string details = "";
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission)
				continue;

			if (mission.m_eRuntimeMode == HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_PHYSICAL_MVP || mission.m_eRuntimeMode == HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_STATE_MACHINE)
				physical++;
			if (mission.m_bRuntimeFallback)
				fallback++;
			if (mission.m_bRuntimeSpawned)
				spawned++;

			details = details + string.Format("\n%1 | %2 | primitive %3 | runtime %4 | phase %5 | spawned %6 | fallback %7 | cleanup %8", mission.m_sInstanceId, mission.m_sMissionId, mission.m_sRuntimePrimitive, mission.m_sRuntimeType, mission.m_sRuntimePhase, mission.m_bRuntimeSpawned, mission.m_bRuntimeFallback, mission.m_bRuntimeCleanupComplete);
		}

		return string.Format("h-istasi mission runtime | physical %1 | spawned %2 | fallback %3 | objectives %4 | assets %5", physical, spawned, fallback, state.m_aMissionObjectives.Count(), state.m_aMissionAssets.Count()) + details;
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
		bool handledByAssets;
		bool assetChanged = PollMissionAssetObjective(state, preset, mission, objective, handledByAssets);
		if (handledByAssets)
			return assetChanged;

		bool playerNear = IsAnyLivingPlayerNearObjective(objective.m_vPosition, PLAYER_OBJECTIVE_RADIUS_METERS);
		if ((objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET) && IsRuntimeEntityDestroyed(objective.m_sLinkedRuntimeEntityId))
			return CompleteWorldObjective(objective);

		if (!playerNear)
			return false;

		objective.m_bWorldDetected = true;
		if (mission.m_sRuntimePhase == PHASE_ACTIVE)
			mission.m_sRuntimePhase = PHASE_CONTACT;
		string primitive = objective.m_sRuntimePrimitive;
		if (primitive.IsEmpty())
			primitive = mission.m_sRuntimePrimitive;

		if (primitive == PRIMITIVE_HOLD_AREA)
		{
			if (mission.m_sMissionId != "dynamic_defend_petros" && HasHostileActiveGroupNear(state, preset, objective.m_vPosition, HOSTILE_OBJECTIVE_RADIUS_METERS))
				return false;

			mission.m_sRuntimePhase = PHASE_HOLDING;
			return ProgressHoldObjective(objective, elapsedSeconds);
		}

		if (primitive == PRIMITIVE_KILL_HVT || primitive == PRIMITIVE_DESTROY_TARGET || primitive == PRIMITIVE_CLEAR_AREA || primitive == PRIMITIVE_CONVOY_INTERCEPT)
		{
			if (HasHostileActiveGroupNear(state, preset, objective.m_vPosition, HOSTILE_OBJECTIVE_RADIUS_METERS))
				return false;

			return CompleteWorldObjective(objective);
		}

		if (primitive == PRIMITIVE_RECOVER_CARGO || primitive == PRIMITIVE_RESCUE_EXTRACT || primitive == PRIMITIVE_DELIVER_SUPPLIES)
		{
			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES)
				mission.m_sRuntimePhase = PHASE_DELIVERING;
			else if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
				mission.m_sRuntimePhase = PHASE_EXTRACTING;

			return CompleteWorldObjective(objective);
		}

		objective.m_bAbstractFallback = true;
		return CompleteWorldObjective(objective);
	}

	protected bool PollMissionAssetObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, out bool handled)
	{
		handled = false;
		if (!state || !mission || !objective)
			return false;
		if (mission.m_bRuntimeFallback && HasRuntimeEntity(mission.m_sRuntimeEntityId))
			return false;

		string role = ResolveObjectiveAssetRole(mission, objective);
		if (role.IsEmpty())
			return false;
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

				if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET)
					satisfiedCount++;

				continue;
			}

			if (objective.m_sTargetId == "convoy")
			{
				if (IsAnyLivingPlayerNearObjective(asset.m_vCurrentPosition, PLAYER_OBJECTIVE_RADIUS_METERS) && !HasHostileActiveGroupNear(state, preset, asset.m_vCurrentPosition, HOSTILE_OBJECTIVE_RADIUS_METERS))
					return CompleteWorldObjective(objective) || changed;

				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET)
				continue;

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
			{
				if (!asset.m_bPickedUp && IsAnyLivingPlayerNearObjective(asset.m_vSourcePosition, PLAYER_ASSET_RADIUS_METERS))
				{
					MarkMissionAssetPickedUp(state, asset);
					changed = true;
					if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
					{
						objective.m_bExtractionStarted = true;
						mission.m_sRuntimePhase = PHASE_EXTRACTING;
					}
					else
					{
						mission.m_sRuntimePhase = PHASE_DELIVERING;
					}
				}

				if (asset.m_sRole == ROLE_CONVOY_CAPTIVE || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
				{
					if (asset.m_bPickedUp && !asset.m_bDelivered && IsAnyLivingPlayerNearObjective(asset.m_vTargetPosition, PLAYER_DELIVERY_RADIUS_METERS))
					{
						MarkMissionAssetDelivered(state, asset, asset.m_vTargetPosition);
						changed = true;
					}

					if (asset.m_bDelivered)
						satisfiedCount++;

					continue;
				}

				if (asset.m_bPickedUp)
					satisfiedCount++;

				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES)
			{
				if (!asset.m_bPickedUp && asset.m_sRole == ROLE_CITY_SUPPLIES && IsAnyLivingPlayerNearObjective(asset.m_vSourcePosition, PLAYER_ASSET_RADIUS_METERS))
				{
					MarkMissionAssetPickedUp(state, asset);
					changed = true;
					mission.m_sRuntimePhase = PHASE_DELIVERING;
				}

				vector deliveryPosition = asset.m_vTargetPosition;
				if (deliveryPosition[0] == 0 && deliveryPosition[1] == 0 && deliveryPosition[2] == 0)
					deliveryPosition = objective.m_vPosition;

				if (asset.m_bPickedUp && !asset.m_bDelivered && IsAnyLivingPlayerNearObjective(deliveryPosition, PLAYER_DELIVERY_RADIUS_METERS))
				{
					MarkMissionAssetDelivered(state, asset, deliveryPosition);
					changed = true;
					objective.m_bDeliveryStarted = true;
					mission.m_sRuntimePhase = PHASE_DELIVERING;
				}

				if (asset.m_bDelivered)
					satisfiedCount++;
			}
		}

		if (!handled)
			return false;

		objective.m_iCurrentCount = Math.Min(Math.Max(1, objective.m_iRequiredCount), satisfiedCount);
		objective.m_iCurrentProgress = Math.Min(objective.m_iRequiredProgress, satisfiedCount);
		if (satisfiedCount >= Math.Max(1, objective.m_iRequiredCount))
			return CompleteWorldObjective(objective) || changed;

		return changed;
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

	protected bool EnsureRestoredMissionRuntime(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!mission || !mission.m_sRuntimePrimitive.IsEmpty())
			return false;

		mission.m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_STATE_MACHINE;
		mission.m_sRuntimePrimitive = PRIMITIVE_ABSTRACT_FALLBACK;
		mission.m_sRuntimeType = PRIMITIVE_ABSTRACT_FALLBACK;
		mission.m_sRuntimePhase = PHASE_ACTIVE;
		mission.m_sRuntimeEntityId = "runtime_" + mission.m_sInstanceId;
		mission.m_iRuntimeStartedAtSecond = state.m_iElapsedSeconds;
		mission.m_iRuntimeHoldSeconds = DEFAULT_HOLD_SECONDS;
		mission.m_bRuntimeSpawned = true;
		mission.m_bRuntimeFallback = true;
		return true;
	}

	protected bool CleanupMissionRuntime(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_bRuntimeCleanupComplete)
			return false;

		DeleteMissionRuntimeEntities(state, mission.m_sInstanceId);
		DeleteRuntimeEntity(mission.m_sRuntimeEntityId);
		CleanupMissionRuntimeRecords(state, mission.m_sInstanceId);
		CleanupMissionAssetRecords(state, mission.m_sInstanceId);
		mission.m_bRuntimeCleanupComplete = true;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == mission.m_sInstanceId)
				objective.m_bCleanupComplete = true;
		}

		return true;
	}

	protected void DeleteMissionRuntimeEntities(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return;

		foreach (HST_MissionRuntimeEntityState runtimeEntity : state.m_aMissionRuntimeEntities)
		{
			if (!runtimeEntity || runtimeEntity.m_sMissionInstanceId != instanceId)
				continue;

			DeleteRuntimeEntity(runtimeEntity.m_sRuntimeEntityId);
		}

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != instanceId)
				continue;

			DeleteRuntimeEntity(asset.m_sEntityId);
		}
	}

	protected bool HasRuntimeEntity(string runtimeEntityId)
	{
		return !runtimeEntityId.IsEmpty() && m_aRuntimeEntityIds.Find(runtimeEntityId) >= 0;
	}

	protected void DeleteRuntimeEntity(string runtimeEntityId)
	{
		int index = m_aRuntimeEntityIds.Find(runtimeEntityId);
		if (index < 0)
			return;

		IEntity entity = m_aRuntimeEntities[index];
		if (entity)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);

		m_aRuntimeEntityIds.Remove(index);
		m_aRuntimeEntities.Remove(index);
	}

	protected void CleanupMissionRuntimeRecords(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return;

		for (int i = state.m_aMissionRuntimeEntities.Count() - 1; i >= 0; i--)
		{
			HST_MissionRuntimeEntityState runtimeEntity = state.m_aMissionRuntimeEntities[i];
			if (!runtimeEntity || runtimeEntity.m_sMissionInstanceId != instanceId)
				continue;

			runtimeEntity.m_bDestroyed = true;
			state.m_aMissionRuntimeEntities.Remove(i);
		}
	}

	protected void CleanupMissionAssetRecords(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return;

		for (int i = state.m_aMissionAssets.Count() - 1; i >= 0; i--)
		{
			HST_MissionAssetState asset = state.m_aMissionAssets[i];
			if (!asset || asset.m_sMissionInstanceId != instanceId)
				continue;

			asset.m_bDestroyed = asset.m_bDestroyed || !asset.m_bDelivered;
			state.m_aMissionAssets.Remove(i);
		}
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
		int index = m_aRuntimeEntityIds.Find(runtimeEntityId);
		if (index < 0 || index >= m_aRuntimeEntities.Count())
			return null;

		return m_aRuntimeEntities[index];
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
		group.m_sGroupId = groupId;
		group.m_sZoneId = zone.m_sZoneId;
		group.m_sFactionKey = factionKey;
		group.m_sPrefab = SelectMissionGroupPrefab(state, zone, factionKey, definition);
		group.m_sRouteId = zone.m_sPatrolRouteId;
		group.m_vSourcePosition = mission.m_vTargetPosition;
		group.m_vTargetPosition = mission.m_vTargetPosition;
		group.m_vPosition = HST_WorldPositionService.ResolveGroundPosition(mission.m_vTargetPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);
		group.m_sRuntimeStatus = "mission_guard";
		group.m_iInfantryCount = ResolveMissionGuardInfantryCount(state, definition);
		group.m_iVehicleCount = ResolveMissionGuardVehicleCount(definition);
		group.m_iLastSeenAliveCount = group.m_iInfantryCount + group.m_iVehicleCount;
		group.m_iSurvivorInfantryCount = group.m_iInfantryCount;
		group.m_iSurvivorVehicleCount = group.m_iVehicleCount;
		state.m_aActiveGroups.Insert(group);
	}

	protected string SelectMissionGroupPrefab(HST_CampaignState state, HST_ZoneState zone, string factionKey, HST_MissionDefinition definition)
	{
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (!faction)
			return "";

		int seed = state.m_iCampaignSeed + state.m_iElapsedSeconds + zone.m_sZoneId.Length() * 17 + definition.m_sMissionId.Length() * 31;
		if (definition.m_sMissionId == "assassinate_specops" && faction.m_aRareGroupPool.Count() > 0)
			return HST_DefaultCatalog.SelectWeightedPrefab(faction.m_aRareGroupPool, seed);
		if (faction.m_aPatrolGroupPool.Count() > 0)
			return HST_DefaultCatalog.SelectWeightedPrefab(faction.m_aPatrolGroupPool, seed);
		if (faction.m_aGroupPool.Count() > 0)
			return HST_DefaultCatalog.SelectWeightedPrefab(faction.m_aGroupPool, seed);

		return "";
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
			return Math.Max(1, definition.m_iVehicleCount);
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
			if (group.m_iLastSeenAliveCount <= 0 && group.m_iSurvivorInfantryCount <= 0 && group.m_iSurvivorVehicleCount <= 0)
				continue;
			if (group.m_bSpawnAttempted && !group.m_bSpawnedEntity)
				continue;

			if (DistanceSq2D(group.m_vPosition, position) <= radiusSq)
				return true;
		}

		return false;
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

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}
}
