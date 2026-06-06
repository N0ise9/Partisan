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
	static const string PROP_HVT = "{6985327711303700}Prefabs/Objects/HST/HST_MissionProp_HVT.et";
	static const string PROP_DESTROY_TARGET = "{6985327711303710}Prefabs/Objects/HST/HST_MissionProp_DestroyTarget.et";
	static const string PROP_CARGO = "{6985327711303720}Prefabs/Objects/HST/HST_MissionProp_Cargo.et";
	static const string PROP_CAPTIVES = "{6985327711303730}Prefabs/Objects/HST/HST_MissionProp_Captives.et";
	static const string PROP_HOLD_MARKER = "{6985327711303740}Prefabs/Objects/HST/HST_MissionProp_HoldMarker.et";
	static const string PROP_CONVOY_VEHICLE = "Prefabs/Vehicles/Wheeled/S1203/S1203_base.et";
	static const float PLAYER_OBJECTIVE_RADIUS_METERS = 35.0;
	static const float HOSTILE_OBJECTIVE_RADIUS_METERS = 90.0;
	static const int DEFAULT_HOLD_SECONDS = 45;

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
			objective.m_sLinkedRuntimeEntityId = mission.m_sRuntimeEntityId;
			if (objective.m_iRequiredHoldSeconds <= 0)
				objective.m_iRequiredHoldSeconds = ResolveObjectiveHoldSeconds(objective, primitive, definition);
			objective.m_bAbstractFallback = mission.m_bRuntimeFallback;
		}

		mission.m_bRuntimeSpawned = TrySpawnMissionRuntimeProp(state, mission);
		if (!mission.m_bRuntimeSpawned)
			mission.m_bRuntimeFallback = true;

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

			if (mission.m_iRuntimeCounterA >= mission.m_iRuntimeCounterB && !AreRuntimeObjectivesComplete(state, mission.m_sInstanceId))
			{
				MarkRuntimeMissionFailed(state, mission, "Convoy reached its destination.");
				return true;
			}
		}

		UpdateMissionCountersFromObjectives(state, mission);
		if (previousPhase != mission.m_sRuntimePhase)
			changed = true;

		return changed;
	}

	protected bool EnsureMissionRuntimeProp(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimeEntityId.IsEmpty() || HasRuntimeEntity(mission.m_sRuntimeEntityId))
			return false;

		bool spawned = TrySpawnMissionRuntimeProp(state, mission);
		if (mission.m_bRuntimeSpawned == spawned)
			return spawned;

		mission.m_bRuntimeSpawned = spawned;
		if (!spawned)
			mission.m_bRuntimeFallback = true;

		return true;
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

	protected vector ResolveRuntimePropPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == mission.m_sInstanceId)
				return HST_WorldPositionService.ResolveGroundPosition(objective.m_vPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		}

		if (!mission.m_sTargetZoneId.IsEmpty())
		{
			HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
			if (zone)
				return HST_WorldPositionService.ResolveGroundPosition(zone.m_vPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		}

		return HST_WorldPositionService.ResolveGroundPosition(state.m_vHQPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
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
		return angles;
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

		return string.Format("h-istasi mission runtime | physical %1 | spawned %2 | fallback %3 | objectives %4", physical, spawned, fallback, state.m_aMissionObjectives.Count()) + details;
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

		DeleteRuntimeEntity(mission.m_sRuntimeEntityId);
		CleanupMissionRuntimeRecords(state, mission.m_sInstanceId);
		mission.m_bRuntimeCleanupComplete = true;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == mission.m_sInstanceId)
				objective.m_bCleanupComplete = true;
		}

		return true;
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
