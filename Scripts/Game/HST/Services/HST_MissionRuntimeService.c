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
	static const float VEHICLE_CARRIER_RADIUS_METERS = 10.0;
	static const float MAX_CONVOY_ZONE_START_DISTANCE_METERS = 1800.0;
	static const int DEFAULT_HOLD_SECONDS = 45;
	static const int DEFAULT_ASSET_INTERACTION_RADIUS_METERS = 18;
	static const int DEFAULT_DELIVERY_RADIUS_METERS = 45;
	static const int DEFAULT_MISSION_CARRIER_CAPACITY = 6;
	static const int MAX_MISSION_VEHICLE_SCAN_ENTITIES = 96;
	static const float MIN_CONVOY_START_DISTANCE_METERS = 650.0;
	static const float MAX_CONVOY_START_DISTANCE_METERS = 1400.0;
	static const float MIN_CONVOY_ROUTE_DISTANCE_METERS = 650.0;
	static const int CONVOY_ROUTE_SAMPLE_COUNT = 6;
	static const int MIN_CONVOY_VEHICLES = 3;
	static const int MAX_CONVOY_VEHICLES = 6;
	static const int MIN_CONVOY_IDLE_SECONDS = 300;
	static const int MAX_CONVOY_IDLE_SECONDS = 600;

	protected ref array<string> m_aRuntimeEntityIds = {};
	protected ref array<IEntity> m_aRuntimeEntities = {};
	protected ref array<IEntity> m_aMissionVehicleScanEntities = {};

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

			if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			{
				changed = CleanupMissionRuntime(state, mission) || changed;
				continue;
			}

			changed = EnsureRestoredMissionRuntime(state, mission) || changed;
			changed = EnsureMissionRuntimeProp(state, mission) || changed;
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

	protected bool EnsureMissionAssetsInitialized(HST_CampaignState state, HST_MissionDefinition definition, HST_ActiveMissionState mission)
	{
		if (!state || !definition || !mission || mission.m_sRuntimePrimitive == PRIMITIVE_ABSTRACT_FALLBACK)
			return false;

		if (state.CountMissionAssets(mission.m_sInstanceId) > 0)
			return false;

		vector targetPosition = ResolveRuntimePropPosition(state, mission);
		vector hqPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vHQPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		int cargoCount = Math.Max(0, definition.m_iCargoCount);
		int captiveCount = Math.Max(0, definition.m_iCaptiveCount);
		int vehicleCount = Math.Max(0, definition.m_iVehicleCount);
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
				string vehiclePrefab = ResolveMissionVehiclePrefab(state, mission);
				for (int i = 0; i < vehicleCount; i++)
					AddMissionAsset(state, mission, ASSET_KIND_VEHICLE, ROLE_DESTROY_TARGET, vehiclePrefab, targetPosition, hqPosition, i);
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
			string convoyVehiclePrefab = ResolveMissionVehiclePrefab(state, mission);
			int convoyVehicleCount = ResolveConvoyVehicleCount(state, mission, definition);
			for (int i = 0; i < convoyVehicleCount; i++)
			{
				vector startSlot = OffsetConvoyVehicleStartPosition(convoyStart, convoyEnd, i);
				AddMissionAsset(state, mission, ASSET_KIND_VEHICLE, ROLE_CONVOY_VEHICLE, convoyVehiclePrefab, startSlot, convoyEnd, i);
			}

			mission.m_iRequiredVehicleCount = convoyVehicleCount;
			mission.m_iRequiredCargoCount = 0;
			mission.m_iRequiredCaptiveCount = 0;
			ConfigureConvoyObjectiveRequirements(state, mission, convoyVehicleCount);
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

			if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT && asset.m_sRole == ROLE_CONVOY_VEHICLE)
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
				position = HST_WorldPositionService.ResolveSafeGroundPosition(asset.m_vCurrentPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false, 2.0);
			}
			vector angles = BuildRuntimePropAngles(state, mission);
			GenericEntity entity = HST_WorldPositionService.SpawnPrefab(asset.m_sPrefab, position, angles);
			if (!entity && asset.m_sPrefab != PROP_HOLD_MARKER)
				entity = HST_WorldPositionService.SpawnPrefab(PROP_HOLD_MARKER, position, angles);

			if (!entity)
			{
				Print(string.Format("h-istasi mission runtime | asset spawn failed for %1 using %2", asset.m_sAssetId, asset.m_sPrefab), LogLevel.WARNING);
				continue;
			}
			if (asset.m_sKind == ASSET_KIND_VEHICLE)
				HST_WorldPositionService.ApplyUprightEntityTransform(entity, position, angles);

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
		if (AreMissionObjectivesComplete(state, mission))
		{
			result = "h-istasi mission | failed: mission objectives are already complete";
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
			else if (commandId == "mission_asset_unload" || commandId == "mission_asset_deliver" || commandId == "mission_captive_extract")
			{
				asset.m_vCurrentPosition = playerPosition;
				asset.m_vLastKnownPosition = playerPosition;
			}
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
			changed = ApplyLoadInteraction(state, mission, asset, playerId, playerEntity, playerPosition, result, eventType);
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
		if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
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
			return IsTransportableAsset(asset) && asset.m_bPickedUp && !asset.m_bDestroyed;

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

		MarkMissionAssetDelivered(state, asset, deliveryPosition);
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
		{
			asset.m_bPickedUp = true;
			asset.m_sCarriedByVehicleId = BuildPlayerCarrierId(playerId);
			asset.m_bAttachedToCarrier = true;
			asset.m_vCurrentPosition = playerPosition;
			asset.m_vLastKnownPosition = playerPosition;
			asset.m_sLastInteraction = PHASE_EXTRACTING;
			mission.m_sRuntimePhase = PHASE_EXTRACTING;
			result = "h-istasi mission | freed " + BuildAssetShortLabel(asset);
			eventType = "freed";
			return true;
		}

		return ApplyDeliverInteraction(state, mission, asset, playerPosition, result, eventType);
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

		mission.m_sRuntimePhase = PHASE_CAPTURED;
		result = "h-istasi mission | captured " + BuildAssetShortLabel(asset);
		eventType = "captured";
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

		return activeGroup.m_iSurvivorInfantryCount > 0 || activeGroup.m_iLastSeenAliveCount > 0 || activeGroup.m_iSpawnedAgentCount > 0;
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
			mission.m_sRuntimePhase = PHASE_DESTROYED;
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

		if (!CanCarrierAcceptAsset(state, playerId, asset))
		{
			failureReason = "no vehicle nearby and player mission carrier is full";
			return false;
		}

		carrierId = BuildPlayerCarrierId(playerId);
		carrierName = "player";
		carrierPosition = playerPosition;
		return true;
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
				resolved = TryResolveCarrierPosition(state, asset.m_sCarriedByVehicleId, asset.m_vCurrentPosition, position);
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

	protected bool TryResolveCarrierPosition(HST_CampaignState state, string carrierId, vector fallbackPosition, out vector position)
	{
		position = fallbackPosition;
		if (carrierId.IsEmpty())
			return false;

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
		int seed = 7;
		if (state)
			seed += state.m_iCampaignSeed * 13 + state.m_iElapsedSeconds * 3;
		if (mission)
			seed += mission.m_sInstanceId.Length() * 41 + mission.m_sMissionId.Length() * 53;
		if (definition)
			seed += definition.m_sMissionId.Length() * 17 + definition.m_iRewardMoney;

		int resolved = MIN_CONVOY_VEHICLES + HST_DefaultCatalog.PositiveMod(seed, MAX_CONVOY_VEHICLES - MIN_CONVOY_VEHICLES + 1);
		if (definition && definition.m_iVehicleCount > 0)
			resolved = Math.Max(resolved, definition.m_iVehicleCount);

		return Math.Max(MIN_CONVOY_VEHICLES, Math.Min(MAX_CONVOY_VEHICLES, resolved));
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
			objective.m_sRequirementText = "Ambush the convoy. Kill all convoy soldiers before they reach destination. Vehicles may be captured after the crew is neutralized.";
			objective.m_iRequiredCount = Math.Max(1, convoyVehicleCount);
			objective.m_iRequiredProgress = Math.Max(1, convoyVehicleCount);
			objective.m_iCurrentCount = 0;
			objective.m_iCurrentProgress = 0;
		}
	}

	protected vector ResolveConvoyStartPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		vector convoyEnd = ResolveConvoyEndPosition(state, mission);
		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		if (route)
		{
			vector routeStart;
			if (HST_WorldPositionService.TryResolveVehicleSpawnPosition(route.m_vStartPosition, routeStart, true) && IsUsableConvoyRouteSegment(routeStart, convoyEnd, MIN_CONVOY_ROUTE_DISTANCE_METERS))
				return routeStart;

			vector routeMid;
			if (HST_WorldPositionService.TryResolveVehicleSpawnPosition(route.m_vMidPosition, routeMid, true) && IsUsableConvoyRouteSegment(routeMid, convoyEnd, MIN_CONVOY_ROUTE_DISTANCE_METERS * 0.6))
				return routeMid;
		}

		vector distantStart;
		if (TryResolveDistantConvoyStartPosition(state, mission, convoyEnd, distantStart))
			return distantStart;

		vector fallback = HST_WorldPositionService.ResolveSafeGroundPosition(OffsetMissionAssetPosition(ResolveRuntimePropPosition(state, mission), -2), HST_WorldPositionService.VEHICLE_GROUND_OFFSET, true, 8.0);
		if (IsUsableConvoyRouteSegment(fallback, convoyEnd, 120.0))
			return fallback;

		return convoyEnd;
	}

	protected bool TryResolveZoneBasedConvoyStartPosition(HST_CampaignState state, HST_ActiveMissionState mission, vector targetPosition, out vector resolved)
	{
		resolved = targetPosition;
		if (!state || !mission || IsZeroVector(targetPosition))
			return false;

		float bestDistanceSq = MAX_CONVOY_ZONE_START_DISTANCE_METERS * MAX_CONVOY_ZONE_START_DISTANCE_METERS;
		bool found;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sZoneId == mission.m_sTargetZoneId)
				continue;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN || zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT)
				continue;

			vector candidate;
			if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(zone.m_vPosition, candidate, true))
				continue;
			if (HST_WorldPositionService.IsLikelyOpenWater(candidate))
				continue;

			float distanceSq = DistanceSq2D(candidate, targetPosition);
			if (distanceSq < MIN_CONVOY_START_DISTANCE_METERS * MIN_CONVOY_START_DISTANCE_METERS || distanceSq > bestDistanceSq)
				continue;
			if (!IsUsableConvoyRouteSegment(candidate, targetPosition, MIN_CONVOY_START_DISTANCE_METERS))
				continue;

			bestDistanceSq = distanceSq;
			resolved = candidate;
			found = true;
		}

		return found;
	}

	protected vector ResolveConvoyEndPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		if (route)
			return HST_WorldPositionService.ResolveSafeGroundPosition(route.m_vEndPosition, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, true, 5.0);

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

	protected bool TryResolveDistantConvoyStartPosition(HST_CampaignState state, HST_ActiveMissionState mission, vector targetPosition, out vector resolved)
	{
		resolved = targetPosition;
		if (!state || !mission || IsZeroVector(targetPosition))
			return false;

		int seed = state.m_iCampaignSeed + state.m_iElapsedSeconds * 7 + mission.m_sInstanceId.Length() * 31 + mission.m_sMissionId.Length() * 43;
		float baseDistance = MIN_CONVOY_START_DISTANCE_METERS + HST_DefaultCatalog.PositiveMod(seed, 5) * 80.0;
		baseDistance = Math.Min(MAX_CONVOY_START_DISTANCE_METERS, baseDistance);
		for (int step = 0; step < 16; step++)
		{
			int direction = HST_DefaultCatalog.PositiveMod(seed + step, 8);
			float distance = Math.Min(MAX_CONVOY_START_DISTANCE_METERS, baseDistance + (step / 8) * 120.0);
			vector candidate = BuildConvoyStartCandidate(targetPosition, direction, distance);
			if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(candidate, resolved, true))
				continue;
			if (HST_WorldPositionService.IsLikelyOpenWater(resolved))
				continue;
			if (!IsUsableConvoyRouteSegment(resolved, targetPosition, MIN_CONVOY_START_DISTANCE_METERS))
				continue;

			return true;
		}

		return false;
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

	protected bool IsUsableConvoyRouteSegment(vector startPosition, vector endPosition, float minDistanceMeters)
	{
		if (IsZeroVector(startPosition) || IsZeroVector(endPosition))
			return false;
		if (DistanceSq2D(startPosition, endPosition) < minDistanceMeters * minDistanceMeters)
			return false;
		if (HST_WorldPositionService.IsLikelyOpenWater(startPosition) || HST_WorldPositionService.IsLikelyOpenWater(endPosition))
			return false;

		return IsDryConvoyRouteSegment(startPosition, endPosition);
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

	protected vector OffsetConvoyVehicleStartPosition(vector startPosition, vector endPosition, int index)
	{
		vector result = startPosition;
		float dx = endPosition[0] - startPosition[0];
		float dz = endPosition[2] - startPosition[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		float forwardX = 1.0;
		float forwardZ = 0.0;
		if (length > 1.0)
		{
			forwardX = dx / length;
			forwardZ = dz / length;
		}

		float sideX = -forwardZ;
		float sideZ = forwardX;
		float spacing = 18.0;
		float sideOffset = 4.0;
		if ((index - (index / 2) * 2) == 1)
			sideOffset = -4.0;

		result[0] = result[0] - forwardX * spacing * index + sideX * sideOffset;
		result[2] = result[2] - forwardZ * spacing * index + sideZ * sideOffset;

		vector resolved;
		if (HST_WorldPositionService.TryResolveVehicleSpawnPosition(result, resolved, true) && !HST_WorldPositionService.IsLikelyOpenWater(resolved))
			return resolved;

		return HST_WorldPositionService.ResolveSafeGroundPosition(result, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, true, 8.0);
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

			details = details + BuildMissionRuntimeReport(state, mission);
		}

		return string.Format("h-istasi mission runtime | physical %1 | spawned %2 | fallback %3 | objectives %4 | assets %5", physical, spawned, fallback, state.m_aMissionObjectives.Count(), state.m_aMissionAssets.Count()) + details;
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

		string line = string.Format("\n%1 | %2 | target %3 | site %4", mission.m_sInstanceId, mission.m_sMissionId, mission.m_sTargetZoneId, mission.m_sSiteId);
		line = line + string.Format(" | primitive %1 | runtime %2 | phase %3 | entity %4", mission.m_sRuntimePrimitive, mission.m_sRuntimeType, mission.m_sRuntimePhase, mission.m_sRuntimeEntityId);
		line = line + string.Format(" | objectives %1/%2 failed %3 | assets %4/%5", objectiveComplete, objectiveTotal, objectiveFailed, assetSpawned, assetTotal);
		line = line + string.Format(" destroyed %1 delivered %2", assetDestroyed, assetDelivered);
		line = line + string.Format(" | fallback %1 | cleanup %2", mission.m_bRuntimeFallback, mission.m_bRuntimeCleanupComplete);
		if (!mission.m_sRuntimeFailureReason.IsEmpty())
			line = line + " | problem " + mission.m_sRuntimeFailureReason;

		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
			line = line + BuildConvoyRouteReport(state, mission);

		line = line + BuildMissionObjectiveRuntimeReport(state, mission);
		line = line + BuildMissionAssetRuntimeReport(state, mission);
		return line;
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

	protected string BuildConvoyRouteReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		string routeId = "none";
		vector start;
		vector mid;
		vector end = mission.m_vTargetPosition;
		int distanceMeters;
		bool roadRoute;
		if (route)
		{
			routeId = route.m_sRouteId;
			start = route.m_vStartPosition;
			mid = route.m_vMidPosition;
			end = route.m_vEndPosition;
			distanceMeters = route.m_iDistanceMeters;
			roadRoute = route.m_bRoadRoute;
		}

		string report = string.Format("\n  convoy route | route %1 | road %2 | distance %3m", routeId, roadRoute, distanceMeters);
		report = report + string.Format(" | start %1 | mid %2 | end %3", start, mid, end);
		report = report + string.Format(" | required vehicles %1 | captured %2 | eta %3s", mission.m_iRequiredVehicleCount, mission.m_iCapturedVehicleCount, mission.m_iRuntimeETASeconds);
		report = report + string.Format(" | timers %1/%2/%3", mission.m_iRuntimeCounterA, mission.m_iRuntimeCounterB, mission.m_iRuntimeCounterC);
		return report;
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
			if (!asset.m_sCarriedByVehicleId.IsEmpty())
				report = report + " | carrier " + asset.m_sCarriedByVehicleId;
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

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
			{
				if (asset.m_sRole == ROLE_CONVOY_CAPTIVE || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
				{
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
			int hostilePersonnel = Math.Max(group.m_iLastSeenAliveCount, group.m_iSurvivorInfantryCount);
			if (hostilePersonnel <= 0)
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
