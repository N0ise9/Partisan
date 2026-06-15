class HST_PersistenceSmokeTestService
{
	static const string EXPECTED_TASK_ID = "hst_smoke_persistence_expected";
	static const string SMOKE_ZONE_ID = "hst_smoke_zone";
	static const string SMOKE_GARRISON_FACTION = "US";
	static const string SMOKE_MISSION_ID = "hst_smoke_mission";
	static const string SMOKE_MISSION_DEF_ID = "dynamic_minor_city_task";
	static const string SMOKE_CONVOY_STAGING_ID = "hst_smoke_convoy_staging";
	static const string SMOKE_CONVOY_MOVING_ID = "hst_smoke_convoy_moving";
	static const string SMOKE_CONVOY_CONTACT_ID = "hst_smoke_convoy_contact";
	static const string SMOKE_PRIMITIVE_HVT_ID = "hst_smoke_primitive_hvt";
	static const string SMOKE_PRIMITIVE_HOLD_ID = "hst_smoke_primitive_hold";
	static const string SMOKE_PRIMITIVE_CLEAR_ID = "hst_smoke_primitive_clear";
	static const string SMOKE_PRIMITIVE_DESTROY_ID = "hst_smoke_primitive_destroy";
	static const string SMOKE_PRIMITIVE_CARGO_ID = "hst_smoke_primitive_cargo";
	static const string SMOKE_PRIMITIVE_RESCUE_ID = "hst_smoke_primitive_rescue";
	static const string SMOKE_PRIMITIVE_SUPPLIES_ID = "hst_smoke_primitive_supplies";
	static const string SMOKE_ASSET_ID = "hst_smoke_asset";
	static const string SMOKE_GARAGE_VEHICLE_ID = "hst_smoke_garage_vehicle";
	static const string SMOKE_ORDER_ID = "hst_smoke_order";
	static const string SMOKE_CARGO_PREFAB = "{6985327711303720}Prefabs/Objects/HST/HST_MissionProp_Cargo.et";
	static const string SMOKE_HVT_PREFAB = "{6985327711303700}Prefabs/Objects/HST/HST_MissionProp_HVT.et";
	static const string SMOKE_DESTROY_PREFAB = "{6985327711303710}Prefabs/Objects/HST/HST_MissionProp_DestroyTarget.et";
	static const string SMOKE_CAPTIVE_PREFAB = "{6985327711303730}Prefabs/Objects/HST/HST_MissionProp_Captives.et";
	static const string SMOKE_HOLD_PREFAB = "{6985327711303740}Prefabs/Objects/HST/HST_MissionProp_HoldMarker.et";
	static const string SMOKE_SUPPLIES_PREFAB = "{6985327711303750}Prefabs/Objects/HST/HST_MissionProp_CitySupplies.et";
	static const string SMOKE_CONVOY_PAYLOAD_PREFAB = "{6985327711303760}Prefabs/Objects/HST/HST_MissionProp_ConvoyPayload.et";
	static const string SMOKE_VEHICLE_PREFAB = "{4AE9D080927D3CB9}Prefabs/Vehicles/Wheeled/S1203/S1203_base.et";
	protected int m_iSeedChangedCount;

	string SeedTestState(HST_CampaignState state, HST_CampaignPreset preset, string adminIdentityId)
	{
		if (!state)
			return "h-istasi persistence smoke | FAIL | state not ready";

		string before = BuildSummary(state);
		m_iSeedChangedCount = 0;
		HST_ZoneState targetZone = EnsureSmokeZone(state, preset);
		EnsureSmokeGarrison(state, preset, targetZone);
		EnsureSmokeMission(state, targetZone);
		EnsureSmokeMissionAsset(state, targetZone);
		EnsureSmokeConvoyMission(state, preset, targetZone, SMOKE_CONVOY_STAGING_ID, "convoy_staging", false);
		EnsureSmokeConvoyMission(state, preset, targetZone, SMOKE_CONVOY_MOVING_ID, "convoy_moving", false);
		EnsureSmokeConvoyMission(state, preset, targetZone, SMOKE_CONVOY_CONTACT_ID, "convoy_contact", true);
		EnsurePrimitiveSmokeMissions(state, targetZone);
		EnsureSmokeGarageVehicle(state, targetZone);
		EnsureSmokeArsenalItem(state);
		EnsureSmokeEnemyOrder(state, preset, targetZone);
		EnsureSmokeCivilianZone(state, targetZone);
		EnsureSmokeUndercoverRecord(state, adminIdentityId);

		StoreExpectedSummary(state, "pending");
		string after = BuildSummary(state);
		StoreExpectedSummary(state, after);
		return string.Format("h-istasi persistence smoke | seeded %1 record(s)\nbefore %2\nafter  %3\n%4", m_iSeedChangedCount, before, after, BuildReport(state));
	}

	string RunSmokeTest(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi persistence smoke | FAIL | state not ready";

		return BuildReport(state);
	}

	string BuildReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi persistence smoke | FAIL | state not ready";

		string current = BuildSummary(state);
		string expected = GetExpectedSummary(state);
		string status = ResolveStatus(state, expected, current);
		string missing = BuildMissingList(state);
		if (missing.IsEmpty())
			missing = "none";

		string expectedLine = expected;
		if (expectedLine.IsEmpty())
			expectedLine = "missing baseline; run admin_seed_persistence_test_state before restart";

		return string.Format("h-istasi persistence smoke | %1\nexpected %2\ncurrent  %3\nmissing/zero %4", status, expectedLine, current, missing);
	}

	string BuildSummary(HST_CampaignState state)
	{
		if (!state)
			return "state=null";

		int activeMissions = CountActiveMissions(state);
		int hash = BuildSummaryHash(state, activeMissions);
		string summary = string.Format("schema=%1|zones=%2|garrisons=%3|active_missions=%4|mission_assets=%5", state.m_iSchemaVersion, state.m_aZones.Count(), state.m_aGarrisons.Count(), activeMissions, state.m_aMissionAssets.Count());
		summary = summary + string.Format("|runtime_entities=%1|active_groups=%2|runtime_vehicles=%3", state.m_aMissionRuntimeEntities.Count(), state.m_aActiveGroups.Count(), state.m_aRuntimeVehicles.Count());
		summary = summary + string.Format("|garage=%1|arsenal=%2|enemy_orders=%3|civilian_zones=%4|undercover=%5", state.m_aGarageVehicles.Count(), state.m_aArsenalItems.Count(), state.m_aEnemyOrders.Count(), state.m_aCivilianZones.Count(), state.m_aUndercoverPlayers.Count());
		summary = summary + string.Format("|convoy_missions=%1|convoy_groups=%2|convoy_vehicle_assets=%3|convoy_payload_assets=%4|convoy_destroyed_assets=%5", CountSmokeConvoyMissions(state), CountSmokeConvoyGroups(state), CountSmokeConvoyAssetsByRole(state, "convoy_vehicle"), CountSmokeConvoyPayloadAssets(state), CountSmokeConvoyDestroyedAssets(state));
		summary = summary + string.Format("|primitive_missions=%1|duplicate_assets=%2|duplicate_runtime_entities=%3|duplicate_groups=%4|duplicate_runtime_vehicles=%5", CountSmokePrimitiveMissions(state), CountDuplicateMissionAssets(state), CountDuplicateRuntimeEntities(state), CountDuplicateActiveGroups(state), CountDuplicateRuntimeVehicles(state));
		summary = summary + string.Format("|sentinels=%1|hash=%2", BuildSentinelMask(state), hash);
		return summary;
	}

	protected string ResolveStatus(HST_CampaignState state, string expected, string current)
	{
		string missing = BuildMissingList(state);
		if (!missing.IsEmpty())
			return "FAIL";

		if (expected.IsEmpty())
			return "WARN";

		if (expected == current)
			return "PASS";

		return "WARN";
	}

	protected string BuildMissingList(HST_CampaignState state)
	{
		if (!state)
			return "state";

		string missing = "";
		missing = AppendMissing(missing, "zones", state.m_aZones.Count() <= 0);
		missing = AppendMissing(missing, "garrisons", state.m_aGarrisons.Count() <= 0);
		missing = AppendMissing(missing, "active_missions", CountActiveMissions(state) <= 0);
		missing = AppendMissing(missing, "mission_assets", state.m_aMissionAssets.Count() <= 0);
		missing = AppendMissing(missing, "runtime_entities", state.m_aMissionRuntimeEntities.Count() <= 0);
		missing = AppendMissing(missing, "convoy_smoke", CountSmokeConvoyMissions(state) < 3 || CountSmokeConvoyGroups(state) < 9 || CountSmokeConvoyAssetsByRole(state, "convoy_vehicle") < 9);
		missing = AppendMissing(missing, "primitive_smoke", CountSmokePrimitiveMissions(state) < 7);
		missing = AppendMissing(missing, "duplicate_assets", CountDuplicateMissionAssets(state) > 0);
		missing = AppendMissing(missing, "duplicate_runtime_entities", CountDuplicateRuntimeEntities(state) > 0);
		missing = AppendMissing(missing, "duplicate_groups", CountDuplicateActiveGroups(state) > 0);
		missing = AppendMissing(missing, "duplicate_runtime_vehicles", CountDuplicateRuntimeVehicles(state) > 0);
		missing = AppendMissing(missing, "garage", state.m_aGarageVehicles.Count() <= 0);
		missing = AppendMissing(missing, "arsenal", state.m_aArsenalItems.Count() <= 0);
		missing = AppendMissing(missing, "enemy_orders", state.m_aEnemyOrders.Count() <= 0);
		missing = AppendMissing(missing, "civilian_zones", state.m_aCivilianZones.Count() <= 0);
		missing = AppendMissing(missing, "undercover", state.m_aUndercoverPlayers.Count() <= 0);
		return missing;
	}

	protected string AppendMissing(string missing, string label, bool shouldAppend)
	{
		if (!shouldAppend)
			return missing;

		if (!missing.IsEmpty())
			missing = missing + ",";
		missing = missing + label;
		return missing;
	}

	protected void StoreExpectedSummary(HST_CampaignState state, string summary)
	{
		HST_CampaignTaskState task = state.FindCampaignTask(EXPECTED_TASK_ID);
		if (!task)
		{
			task = new HST_CampaignTaskState();
			task.m_sTaskId = EXPECTED_TASK_ID;
			state.m_aCampaignTasks.Insert(task);
		}

		task.m_sLinkedId = "persistence_smoke";
		task.m_sTitle = "Persistence Smoke Expected";
		task.m_sDescription = summary;
		task.m_sCategory = "admin";
		task.m_vPosition = state.m_vHQPosition;
		task.m_bActive = false;
		task.m_bSucceeded = false;
		task.m_bFailed = false;
	}

	protected string GetExpectedSummary(HST_CampaignState state)
	{
		if (!state)
			return "";

		HST_CampaignTaskState task = state.FindCampaignTask(EXPECTED_TASK_ID);
		if (!task)
			return "";

		return task.m_sDescription;
	}

	protected HST_ZoneState EnsureSmokeZone(HST_CampaignState state, HST_CampaignPreset preset)
	{
		HST_ZoneState existing = SelectSmokeTargetZone(state, preset);
		if (existing)
			return existing;

		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = SMOKE_ZONE_ID;
		zone.m_sDisplayName = "Persistence Smoke Site";
		zone.m_sOwnerFactionKey = ResolveOccupierFaction(preset);
		zone.m_eType = HST_EZoneType.HST_ZONE_MISSION_SITE;
		zone.m_vPosition = ResolveSmokePosition(state);
		zone.m_iCaptureRadiusMeters = 60;
		zone.m_iActivationRadiusMeters = 220;
		zone.m_iGarrisonSlots = 4;
		zone.m_sCompositionId = "comp_hst_smoke";
		zone.m_sSpawnProfileId = "spawn_hst_smoke";
		state.m_aZones.Insert(zone);
		m_iSeedChangedCount++;
		return zone;
	}

	protected HST_ZoneState SelectSmokeTargetZone(HST_CampaignState state, HST_CampaignPreset preset)
	{
		HST_ZoneState smokeZone = state.FindZone(SMOKE_ZONE_ID);
		if (smokeZone)
			return smokeZone;

		string resistance = ResolveResistanceFaction(preset);
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_sOwnerFactionKey != resistance)
				return zone;
		}

		foreach (HST_ZoneState fallback : state.m_aZones)
		{
			if (fallback)
				return fallback;
		}

		return null;
	}

	protected void EnsureSmokeGarrison(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState targetZone)
	{
		if (!targetZone)
			return;

		string factionKey = targetZone.m_sOwnerFactionKey;
		if (factionKey.IsEmpty())
			factionKey = ResolveOccupierFaction(preset);

		HST_GarrisonState garrison = state.FindGarrison(targetZone.m_sZoneId, factionKey);
		if (!garrison)
		{
			garrison = new HST_GarrisonState();
			garrison.m_sZoneId = targetZone.m_sZoneId;
			garrison.m_sFactionKey = factionKey;
			state.m_aGarrisons.Insert(garrison);
			m_iSeedChangedCount++;
		}

		if (garrison.m_iInfantryCount <= 0)
		{
			garrison.m_iInfantryCount = 4;
			m_iSeedChangedCount++;
		}

		if (garrison.m_iVehicleCount <= 0)
		{
			garrison.m_iVehicleCount = 1;
			m_iSeedChangedCount++;
		}
	}

	protected void EnsureSmokeMission(HST_CampaignState state, HST_ZoneState targetZone)
	{
		HST_ActiveMissionState mission = state.FindActiveMission(SMOKE_MISSION_ID);
		if (!mission)
		{
			mission = new HST_ActiveMissionState();
			mission.m_sInstanceId = SMOKE_MISSION_ID;
			state.m_aActiveMissions.Insert(mission);
			m_iSeedChangedCount++;
		}

		mission.m_sMissionId = SMOKE_MISSION_DEF_ID;
		mission.m_sDisplayName = "Persistence Smoke Mission";
		mission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		mission.m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_ABSTRACT;
		mission.m_iRemainingSeconds = Math.Max(mission.m_iRemainingSeconds, 86400);
		if (targetZone)
		{
			mission.m_sTargetZoneId = targetZone.m_sZoneId;
			mission.m_vTargetPosition = targetZone.m_vPosition;
		}
		mission.m_sMarkerId = "hst_smoke_marker";
		mission.m_sRuntimePrimitive = "abstract_fallback";
		mission.m_sRuntimeType = "persistence_smoke";
		if (mission.m_sRuntimePhase.IsEmpty())
			mission.m_sRuntimePhase = "smoke";
		mission.m_iStartedAtSecond = state.m_iElapsedSeconds;
		mission.m_iActiveUntilSecond = state.m_iElapsedSeconds + mission.m_iRemainingSeconds;
		mission.m_bDynamic = true;
		mission.m_bRequested = true;
		mission.m_bRuntimeFallback = true;
	}

	protected void EnsureSmokeMissionAsset(HST_CampaignState state, HST_ZoneState targetZone)
	{
		HST_MissionAssetState asset = state.FindMissionAsset(SMOKE_ASSET_ID);
		if (!asset)
		{
			asset = new HST_MissionAssetState();
			asset.m_sAssetId = SMOKE_ASSET_ID;
			state.m_aMissionAssets.Insert(asset);
			m_iSeedChangedCount++;
		}

		vector position = ResolveSmokePosition(state);
		if (targetZone)
			position = targetZone.m_vPosition;

		asset.m_sMissionInstanceId = SMOKE_MISSION_ID;
		asset.m_sKind = "cargo";
		asset.m_sRole = "logistics_cargo";
		asset.m_sPrefab = SMOKE_CARGO_PREFAB;
		asset.m_sEntityId = "hst_smoke_asset_entity";
		asset.m_bSpawned = false;
		asset.m_bPickedUp = false;
		asset.m_bDelivered = false;
		asset.m_bDestroyed = false;
		asset.m_bAlive = true;
		asset.m_vSourcePosition = position;
		asset.m_vTargetPosition = state.m_vHQPosition;
		asset.m_vCurrentPosition = position;
		asset.m_vLastKnownPosition = position;
		asset.m_iInteractionRadiusMeters = 18;
	}

	protected void EnsureSmokeConvoyMission(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState targetZone, string instanceId, string phase, bool contactVariant)
	{
		if (!state || !targetZone || instanceId.IsEmpty())
			return;

		HST_ActiveMissionState mission = EnsureSmokeActiveMission(state, instanceId, "convoy_supplies", "Persistence Smoke Convoy " + phase, "convoy_intercept", "convoy_route_intercept", phase, targetZone);
		mission.m_iRequiredVehicleCount = 3;
		mission.m_iRequiredCargoCount = 1;
		mission.m_iRequiredCaptiveCount = 0;
		mission.m_iRuntimeCounterA = 120;
		mission.m_iRuntimeCounterB = 300;
		mission.m_iRuntimeCounterC = 1800;
		if (phase == "convoy_staging")
			mission.m_iRuntimeETASeconds = 180;
		else
			mission.m_iRuntimeETASeconds = 1200;

		EnsureSmokeObjective(state, mission, "objective_" + instanceId + "_convoy", HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA, "convoy", "Neutralize convoy crew", targetZone.m_vPosition, 3);

		vector convoyTarget = targetZone.m_vPosition;
		for (int i = 0; i < 3; i++)
		{
			vector sourcePosition = BuildSmokeOffsetPosition(convoyTarget, -2600.0 - i * 24.0, -80.0);
			vector currentPosition = sourcePosition;
			if (phase == "convoy_moving" || phase == "convoy_contact")
				currentPosition = BuildSmokeOffsetPosition(convoyTarget, -1200.0 - i * 24.0, -40.0);

			string assetId = string.Format("asset_%1_convoy_vehicle_%2", instanceId, i);
			HST_MissionAssetState asset = EnsureSmokeAsset(state, mission, assetId, "vehicle", "convoy_vehicle", SMOKE_VEHICLE_PREFAB, sourcePosition, convoyTarget, currentPosition);
			asset.m_bSpawned = true;
			asset.m_bAlive = true;

			string groupId = string.Format("mission_convoy_%1_%2", instanceId, i);
			HST_ActiveGroupState group = EnsureSmokeActiveGroup(state, preset, targetZone, groupId, phase, currentPosition, convoyTarget);
			group.m_iInfantryCount = 2;
			group.m_iVehicleCount = 1;
			group.m_iLastSeenAliveCount = 2;
			group.m_iSurvivorInfantryCount = 2;
			group.m_iSurvivorVehicleCount = 1;
			group.m_iSpawnedAgentCount = 2;
			group.m_iAssignedWaypointCount = 0;
			if (phase == "convoy_moving" || phase == "convoy_contact")
				group.m_iAssignedWaypointCount = 3;
			group.m_bSpawnAttempted = true;
			group.m_bSpawnedEntity = true;

			if (contactVariant && i == 0)
			{
				asset.m_bDestroyed = true;
				asset.m_bAlive = false;
				asset.m_bSpawned = false;
				asset.m_sLastInteraction = "destroyed";
				group.m_sRuntimeStatus = "convoy_eliminated";
				group.m_iLastSeenAliveCount = 0;
				group.m_iSurvivorInfantryCount = 0;
				group.m_iSurvivorVehicleCount = 0;
				group.m_iSpawnedAgentCount = 0;
				group.m_bSpawnedEntity = false;
				group.m_iAssignedWaypointCount = 0;
			}

			EnsureSmokeRuntimeEntity(state, asset.m_sEntityId, mission.m_sInstanceId, asset.m_sRole, asset.m_sPrefab, currentPosition, asset.m_bSpawned, asset.m_bDestroyed, asset.m_bDelivered);
			EnsureSmokeRuntimeVehicle(state, asset.m_sEntityId, asset.m_sPrefab, asset.m_sRole, currentPosition, asset.m_bDestroyed || asset.m_bDelivered);
		}

		HST_MissionAssetState payload = EnsureSmokeAsset(state, mission, "asset_" + instanceId + "_convoy_payload_0", "cargo", "convoy_payload", SMOKE_CONVOY_PAYLOAD_PREFAB, BuildSmokeOffsetPosition(convoyTarget, -1180.0, -40.0), convoyTarget, BuildSmokeOffsetPosition(convoyTarget, -1180.0, -40.0));
		payload.m_bSpawned = false;
		payload.m_bAlive = true;
		EnsureSmokeRuntimeEntity(state, payload.m_sEntityId, mission.m_sInstanceId, payload.m_sRole, payload.m_sPrefab, payload.m_vCurrentPosition, false, false, false);
	}

	protected void EnsurePrimitiveSmokeMissions(HST_CampaignState state, HST_ZoneState targetZone)
	{
		if (!state || !targetZone)
			return;

		EnsureSmokePrimitiveMission(state, targetZone, SMOKE_PRIMITIVE_HVT_ID, "assassinate_officer", "kill_hvt", HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET, "hvt", "character", "hvt", SMOKE_HVT_PREFAB, 1, false);
		EnsureSmokePrimitiveMission(state, targetZone, SMOKE_PRIMITIVE_HOLD_ID, "conquest_outpost", "hold_area", HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA, targetZone.m_sZoneId, "area", "hold_marker", SMOKE_HOLD_PREFAB, 1, false);
		EnsureSmokePrimitiveMission(state, targetZone, SMOKE_PRIMITIVE_CLEAR_ID, "dynamic_minor_city_task", "clear_area", HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA, targetZone.m_sZoneId, "area", "hold_marker", SMOKE_HOLD_PREFAB, 1, false);
		EnsureSmokePrimitiveMission(state, targetZone, SMOKE_PRIMITIVE_DESTROY_ID, "destroy_radio_tower", "destroy_target", HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET, "asset", "target", "destroy_target", SMOKE_DESTROY_PREFAB, 1, false);
		EnsureSmokePrimitiveMission(state, targetZone, SMOKE_PRIMITIVE_CARGO_ID, "logistics_resource_cache", "recover_cargo", HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT, "cargo", "cargo", "logistics_cargo", SMOKE_CARGO_PREFAB, 1, false);
		EnsureSmokePrimitiveMission(state, targetZone, SMOKE_PRIMITIVE_RESCUE_ID, "rescue_pows", "rescue_extract", HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES, "extract_captives", "captive", "captive", SMOKE_CAPTIVE_PREFAB, 3, false);
		EnsureSmokePrimitiveMission(state, targetZone, SMOKE_PRIMITIVE_SUPPLIES_ID, "support_city_supplies", "deliver_supplies", HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES, "supply_pickup", "cargo", "city_supplies", SMOKE_SUPPLIES_PREFAB, 1, true);
	}

	protected void EnsureSmokePrimitiveMission(HST_CampaignState state, HST_ZoneState targetZone, string instanceId, string missionId, string primitive, HST_EMissionObjectiveType objectiveType, string targetId, string kind, string role, string prefab, int count, bool sourceAtHQ)
	{
		HST_ActiveMissionState mission = EnsureSmokeActiveMission(state, instanceId, missionId, "Persistence Smoke " + primitive, primitive, primitive, "active", targetZone);
		if (kind == "cargo")
			mission.m_iRequiredCargoCount = Math.Max(1, count);
		if (kind == "captive")
			mission.m_iRequiredCaptiveCount = Math.Max(1, count);
		if (kind == "vehicle")
			mission.m_iRequiredVehicleCount = Math.Max(1, count);

		EnsureSmokeObjective(state, mission, "objective_" + instanceId, objectiveType, targetId, "Smoke " + primitive, targetZone.m_vPosition, Math.Max(1, count));
		for (int i = 0; i < Math.Max(1, count); i++)
		{
			vector sourcePosition = BuildSmokeOffsetPosition(targetZone.m_vPosition, i * 5.0, 20.0);
			if (sourceAtHQ)
				sourcePosition = BuildSmokeOffsetPosition(state.m_vHQPosition, i * 5.0, 12.0);
			vector targetPosition = targetZone.m_vPosition;
			if (primitive == "recover_cargo" || primitive == "rescue_extract")
				targetPosition = state.m_vHQPosition;

			string assetId = string.Format("asset_%1_%2_%3", instanceId, role, i);
			HST_MissionAssetState asset = EnsureSmokeAsset(state, mission, assetId, kind, role, prefab, sourcePosition, targetPosition, sourcePosition);
			asset.m_bSpawned = true;
			asset.m_bAlive = true;
			EnsureSmokeRuntimeEntity(state, asset.m_sEntityId, mission.m_sInstanceId, asset.m_sRole, asset.m_sPrefab, asset.m_vCurrentPosition, true, false, false);
			if (kind == "vehicle")
				EnsureSmokeRuntimeVehicle(state, asset.m_sEntityId, asset.m_sPrefab, asset.m_sRole, asset.m_vCurrentPosition, false);
		}
	}

	protected HST_ActiveMissionState EnsureSmokeActiveMission(HST_CampaignState state, string instanceId, string missionId, string displayName, string primitive, string runtimeType, string phase, HST_ZoneState targetZone)
	{
		HST_ActiveMissionState mission = state.FindActiveMission(instanceId);
		if (!mission)
		{
			mission = new HST_ActiveMissionState();
			mission.m_sInstanceId = instanceId;
			state.m_aActiveMissions.Insert(mission);
			m_iSeedChangedCount++;
		}

		mission.m_sMissionId = missionId;
		mission.m_sDisplayName = displayName;
		mission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		mission.m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_STATE_MACHINE;
		mission.m_sRuntimePrimitive = primitive;
		mission.m_sRuntimeType = runtimeType;
		mission.m_sRuntimePhase = phase;
		mission.m_sRuntimeEntityId = "runtime_" + instanceId + "_" + primitive;
		mission.m_iRemainingSeconds = Math.Max(mission.m_iRemainingSeconds, 3600);
		mission.m_iStartedAtSecond = state.m_iElapsedSeconds;
		mission.m_iActiveUntilSecond = state.m_iElapsedSeconds + mission.m_iRemainingSeconds;
		mission.m_iRuntimeStartedAtSecond = mission.m_iStartedAtSecond;
		mission.m_bDynamic = true;
		mission.m_bRequested = true;
		mission.m_bRuntimeSpawned = true;
		mission.m_bRuntimeFallback = false;
		mission.m_bRuntimeCleanupComplete = false;
		if (targetZone)
		{
			mission.m_sTargetZoneId = targetZone.m_sZoneId;
			mission.m_vTargetPosition = targetZone.m_vPosition;
		}
		return mission;
	}

	protected HST_MissionObjectiveState EnsureSmokeObjective(HST_CampaignState state, HST_ActiveMissionState mission, string objectiveId, HST_EMissionObjectiveType objectiveType, string targetId, string label, vector position, int requiredCount)
	{
		HST_MissionObjectiveState objective = FindSmokeObjective(state, objectiveId);
		if (!objective)
		{
			objective = new HST_MissionObjectiveState();
			objective.m_sObjectiveId = objectiveId;
			state.m_aMissionObjectives.Insert(objective);
			m_iSeedChangedCount++;
		}

		objective.m_sMissionInstanceId = mission.m_sInstanceId;
		objective.m_eType = objectiveType;
		objective.m_sLabel = label;
		objective.m_sRequirementText = label;
		objective.m_sTargetId = targetId;
		objective.m_sTargetZoneId = mission.m_sTargetZoneId;
		objective.m_sRuntimePrimitive = mission.m_sRuntimePrimitive;
		objective.m_sLinkedRuntimeEntityId = "";
		objective.m_sPhysicalEntityId = "";
		objective.m_vPosition = position;
		objective.m_iRequiredProgress = Math.Max(1, requiredCount);
		objective.m_iRequiredCount = Math.Max(1, requiredCount);
		objective.m_bComplete = false;
		objective.m_bFailed = false;
		objective.m_bCleanupComplete = false;
		return objective;
	}

	protected HST_MissionObjectiveState FindSmokeObjective(HST_CampaignState state, string objectiveId)
	{
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sObjectiveId == objectiveId)
				return objective;
		}

		return null;
	}

	protected HST_MissionAssetState EnsureSmokeAsset(HST_CampaignState state, HST_ActiveMissionState mission, string assetId, string kind, string role, string prefab, vector sourcePosition, vector targetPosition, vector currentPosition)
	{
		HST_MissionAssetState asset = state.FindMissionAsset(assetId);
		if (!asset)
		{
			asset = new HST_MissionAssetState();
			asset.m_sAssetId = assetId;
			state.m_aMissionAssets.Insert(asset);
			m_iSeedChangedCount++;
		}

		asset.m_sMissionInstanceId = mission.m_sInstanceId;
		asset.m_sKind = kind;
		asset.m_sRole = role;
		asset.m_sPrefab = prefab;
		asset.m_sEntityId = assetId;
		asset.m_sCarriedByVehicleId = "";
		asset.m_sLastInteraction = "";
		asset.m_bSpawned = false;
		asset.m_bPickedUp = false;
		asset.m_bDelivered = false;
		asset.m_bDestroyed = false;
		asset.m_bAlive = true;
		asset.m_bAttachedToCarrier = false;
		asset.m_bOutcomeApplied = false;
		asset.m_sOutcomeKind = "";
		asset.m_vSourcePosition = sourcePosition;
		asset.m_vTargetPosition = targetPosition;
		asset.m_vCurrentPosition = currentPosition;
		asset.m_vLastKnownPosition = currentPosition;
		asset.m_iDeadlineSecond = mission.m_iActiveUntilSecond;
		asset.m_iCargoCapacityCost = ResolveSmokeCapacityCost(kind, role);
		asset.m_iInteractionRadiusMeters = ResolveSmokeInteractionRadius(kind);
		return asset;
	}

	protected HST_ActiveGroupState EnsureSmokeActiveGroup(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState targetZone, string groupId, string phase, vector position, vector targetPosition)
	{
		HST_ActiveGroupState group = state.FindActiveGroup(groupId);
		if (!group)
		{
			group = new HST_ActiveGroupState();
			group.m_sGroupId = groupId;
			state.m_aActiveGroups.Insert(group);
			m_iSeedChangedCount++;
		}

		group.m_sZoneId = targetZone.m_sZoneId;
		group.m_sFactionKey = ResolveOccupierFaction(preset);
		group.m_sPrefab = ResolveSmokeGroupPrefab(group.m_sFactionKey);
		group.m_sRouteId = "";
		group.m_sRuntimeStatus = phase;
		group.m_vPosition = position;
		group.m_vSourcePosition = position;
		group.m_vTargetPosition = targetPosition;
		group.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		return group;
	}

	protected void EnsureSmokeRuntimeEntity(HST_CampaignState state, string runtimeEntityId, string missionInstanceId, string kind, string prefab, vector position, bool spawned, bool destroyed, bool recovered)
	{
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(runtimeEntityId);
		if (!runtimeEntity)
		{
			runtimeEntity = new HST_MissionRuntimeEntityState();
			runtimeEntity.m_sRuntimeEntityId = runtimeEntityId;
			state.m_aMissionRuntimeEntities.Insert(runtimeEntity);
			m_iSeedChangedCount++;
		}

		runtimeEntity.m_sMissionInstanceId = missionInstanceId;
		runtimeEntity.m_sKind = kind;
		runtimeEntity.m_sPrefab = prefab;
		runtimeEntity.m_vPosition = position;
		runtimeEntity.m_vAngles = "0 0 0";
		runtimeEntity.m_bSpawned = spawned;
		runtimeEntity.m_bDestroyed = destroyed;
		runtimeEntity.m_bRecovered = recovered;
	}

	protected void EnsureSmokeRuntimeVehicle(HST_CampaignState state, string vehicleRuntimeId, string prefab, string kind, vector position, bool deleted)
	{
		HST_RuntimeVehicleState vehicle = state.FindRuntimeVehicle(vehicleRuntimeId);
		if (!vehicle)
		{
			vehicle = new HST_RuntimeVehicleState();
			vehicle.m_sVehicleRuntimeId = vehicleRuntimeId;
			state.m_aRuntimeVehicles.Insert(vehicle);
			m_iSeedChangedCount++;
		}

		vehicle.m_sPrefab = prefab;
		vehicle.m_sDisplayName = kind;
		vehicle.m_sRuntimeKind = kind;
		vehicle.m_vPosition = position;
		vehicle.m_vAngles = "0 0 0";
		vehicle.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		vehicle.m_bDeleted = deleted;
	}

	protected void EnsureSmokeGarageVehicle(HST_CampaignState state, HST_ZoneState targetZone)
	{
		HST_GarageVehicleState vehicle = state.FindGarageVehicle(SMOKE_GARAGE_VEHICLE_ID);
		if (!vehicle)
		{
			vehicle = new HST_GarageVehicleState();
			vehicle.m_sVehicleId = SMOKE_GARAGE_VEHICLE_ID;
			state.m_aGarageVehicles.Insert(vehicle);
			m_iSeedChangedCount++;
		}

		vehicle.m_sPrefab = SMOKE_VEHICLE_PREFAB;
		vehicle.m_sDisplayName = "Smoke Test S1203";
		if (targetZone)
			vehicle.m_sSourceZoneId = targetZone.m_sZoneId;
		vehicle.m_sSourceFactionKey = SMOKE_GARRISON_FACTION;
		vehicle.m_iStoredAtSecond = state.m_iElapsedSeconds;
		vehicle.m_iRedeployCost = 25;
		if (targetZone)
			vehicle.m_vPosition = targetZone.m_vPosition;
		else
			vehicle.m_vPosition = ResolveSmokePosition(state);
		vehicle.m_vAngles = "0 0 0";
		vehicle.m_fFuel = 1.0;
		vehicle.m_sDamageState = "ok";
		vehicle.m_bArmed = false;
		vehicle.m_bUnlocked = true;
	}

	protected void EnsureSmokeArsenalItem(HST_CampaignState state)
	{
		HST_ArsenalItemState item = state.FindArsenalItem(SMOKE_CARGO_PREFAB);
		if (!item)
		{
			item = new HST_ArsenalItemState();
			item.m_sPrefab = SMOKE_CARGO_PREFAB;
			state.m_aArsenalItems.Insert(item);
			m_iSeedChangedCount++;
		}

		item.m_sDisplayName = "Smoke Test Cargo";
		item.m_sCategory = "smoke";
		item.m_iCount = Math.Max(1, item.m_iCount);
		item.m_bUnlocked = false;
	}

	protected void EnsureSmokeEnemyOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState targetZone)
	{
		HST_EnemyOrderState order = FindEnemyOrder(state, SMOKE_ORDER_ID);
		if (!order)
		{
			order = new HST_EnemyOrderState();
			order.m_sOrderId = SMOKE_ORDER_ID;
			state.m_aEnemyOrders.Insert(order);
			m_iSeedChangedCount++;
		}

		order.m_sFactionKey = ResolveOccupierFaction(preset);
		order.m_eType = HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED;
		if (targetZone)
			order.m_sTargetZoneId = targetZone.m_sZoneId;
		order.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		order.m_iResolveAtSecond = state.m_iElapsedSeconds;
		order.m_iAttackCost = 0;
		order.m_iSupportCost = 0;
	}

	protected void EnsureSmokeCivilianZone(HST_CampaignState state, HST_ZoneState targetZone)
	{
		if (!targetZone)
			return;

		HST_CivilianZoneState civilianZone = state.FindCivilianZone(targetZone.m_sZoneId);
		if (!civilianZone)
		{
			civilianZone = new HST_CivilianZoneState();
			civilianZone.m_sZoneId = targetZone.m_sZoneId;
			state.m_aCivilianZones.Insert(civilianZone);
			m_iSeedChangedCount++;
		}

		civilianZone.m_iCivilianPresence = Math.Max(1, civilianZone.m_iCivilianPresence);
		civilianZone.m_iReputation = Math.Max(civilianZone.m_iReputation, 1);
	}

	protected void EnsureSmokeUndercoverRecord(HST_CampaignState state, string adminIdentityId)
	{
		string identityId = adminIdentityId;
		if (identityId.IsEmpty())
			identityId = "hst_smoke_admin";

		HST_PlayerUndercoverState undercover = state.FindUndercoverPlayer(identityId);
		if (!undercover)
		{
			undercover = new HST_PlayerUndercoverState();
			undercover.m_sIdentityId = identityId;
			state.m_aUndercoverPlayers.Insert(undercover);
			m_iSeedChangedCount++;
		}

		undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
		undercover.m_iWantedHeat = 0;
		undercover.m_iLastCheckedSecond = state.m_iElapsedSeconds;
		undercover.m_sLastReason = "persistence smoke";
	}

	protected HST_EnemyOrderState FindEnemyOrder(HST_CampaignState state, string orderId)
	{
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (order && order.m_sOrderId == orderId)
				return order;
		}

		return null;
	}

	protected int CountActiveMissions(HST_CampaignState state)
	{
		int count;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (mission && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				count++;
		}

		return count;
	}

	protected string BuildSentinelMask(HST_CampaignState state)
	{
		string mask = "";
		mask = mask + BoolMask(state.FindActiveMission(SMOKE_MISSION_ID) != null);
		mask = mask + BoolMask(state.FindActiveMission(SMOKE_CONVOY_STAGING_ID) != null);
		mask = mask + BoolMask(state.FindActiveMission(SMOKE_CONVOY_MOVING_ID) != null);
		mask = mask + BoolMask(state.FindActiveMission(SMOKE_CONVOY_CONTACT_ID) != null);
		mask = mask + BoolMask(state.FindActiveMission(SMOKE_PRIMITIVE_HVT_ID) != null);
		mask = mask + BoolMask(state.FindActiveMission(SMOKE_PRIMITIVE_SUPPLIES_ID) != null);
		mask = mask + BoolMask(state.FindMissionAsset(SMOKE_ASSET_ID) != null);
		mask = mask + BoolMask(state.FindGarageVehicle(SMOKE_GARAGE_VEHICLE_ID) != null);
		mask = mask + BoolMask(state.FindArsenalItem(SMOKE_CARGO_PREFAB) != null);
		mask = mask + BoolMask(FindEnemyOrder(state, SMOKE_ORDER_ID) != null);
		mask = mask + BoolMask(state.FindCampaignTask(EXPECTED_TASK_ID) != null);
		return mask;
	}

	protected string BoolMask(bool value)
	{
		if (value)
			return "1";

		return "0";
	}

	protected int BuildSummaryHash(HST_CampaignState state, int activeMissions)
	{
		int hash = 17;
		hash = MixInt(hash, state.m_iSchemaVersion);
		hash = MixInt(hash, state.m_aZones.Count());
		hash = MixInt(hash, state.m_aGarrisons.Count());
		hash = MixInt(hash, activeMissions);
		hash = MixInt(hash, state.m_aMissionAssets.Count());
		hash = MixInt(hash, state.m_aMissionRuntimeEntities.Count());
		hash = MixInt(hash, state.m_aActiveGroups.Count());
		hash = MixInt(hash, state.m_aRuntimeVehicles.Count());
		hash = MixInt(hash, state.m_aGarageVehicles.Count());
		hash = MixInt(hash, state.m_aArsenalItems.Count());
		hash = MixInt(hash, state.m_aEnemyOrders.Count());
		hash = MixInt(hash, state.m_aCivilianZones.Count());
		hash = MixInt(hash, state.m_aUndercoverPlayers.Count());
		hash = MixInt(hash, CountSmokeConvoyMissions(state));
		hash = MixInt(hash, CountSmokeConvoyGroups(state));
		hash = MixInt(hash, CountSmokeConvoyAssetsByRole(state, "convoy_vehicle"));
		hash = MixInt(hash, CountSmokeConvoyPayloadAssets(state));
		hash = MixInt(hash, CountSmokeConvoyDestroyedAssets(state));
		hash = MixInt(hash, CountSmokePrimitiveMissions(state));
		hash = MixInt(hash, CountDuplicateMissionAssets(state));
		hash = MixInt(hash, CountDuplicateRuntimeEntities(state));
		hash = MixInt(hash, CountDuplicateActiveGroups(state));
		hash = MixInt(hash, CountDuplicateRuntimeVehicles(state));
		hash = MixStringLength(hash, BuildSentinelMask(state));
		hash = MixStringLength(hash, FirstZoneId(state));
		hash = MixStringLength(hash, FirstGarageId(state));
		if (hash < 0)
			hash = -hash;
		return hash;
	}

	protected int MixInt(int hash, int value)
	{
		return hash * 31 + value * 7 + 13;
	}

	protected int MixStringLength(int hash, string value)
	{
		return hash * 31 + value.Length() * 19 + 5;
	}

	protected string FirstZoneId(HST_CampaignState state)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone)
				return zone.m_sZoneId;
		}

		return "";
	}

	protected string FirstGarageId(HST_CampaignState state)
	{
		foreach (HST_GarageVehicleState vehicle : state.m_aGarageVehicles)
		{
			if (vehicle)
				return vehicle.m_sVehicleId;
		}

		return "";
	}

	protected int CountSmokeConvoyMissions(HST_CampaignState state)
	{
		int count;
		if (state.FindActiveMission(SMOKE_CONVOY_STAGING_ID))
			count++;
		if (state.FindActiveMission(SMOKE_CONVOY_MOVING_ID))
			count++;
		if (state.FindActiveMission(SMOKE_CONVOY_CONTACT_ID))
			count++;
		return count;
	}

	protected int CountSmokePrimitiveMissions(HST_CampaignState state)
	{
		int count;
		if (state.FindActiveMission(SMOKE_PRIMITIVE_HVT_ID))
			count++;
		if (state.FindActiveMission(SMOKE_PRIMITIVE_HOLD_ID))
			count++;
		if (state.FindActiveMission(SMOKE_PRIMITIVE_CLEAR_ID))
			count++;
		if (state.FindActiveMission(SMOKE_PRIMITIVE_DESTROY_ID))
			count++;
		if (state.FindActiveMission(SMOKE_PRIMITIVE_CARGO_ID))
			count++;
		if (state.FindActiveMission(SMOKE_PRIMITIVE_RESCUE_ID))
			count++;
		if (state.FindActiveMission(SMOKE_PRIMITIVE_SUPPLIES_ID))
			count++;
		return count;
	}

	protected int CountSmokeConvoyGroups(HST_CampaignState state)
	{
		int count;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (!group)
				continue;
			if (group.m_sGroupId.Contains(SMOKE_CONVOY_STAGING_ID) || group.m_sGroupId.Contains(SMOKE_CONVOY_MOVING_ID) || group.m_sGroupId.Contains(SMOKE_CONVOY_CONTACT_ID))
				count++;
		}
		return count;
	}

	protected int CountSmokeConvoyAssetsByRole(HST_CampaignState state, string role)
	{
		int count;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sRole != role)
				continue;
			if (IsSmokeConvoyInstanceId(asset.m_sMissionInstanceId))
				count++;
		}
		return count;
	}

	protected int CountSmokeConvoyPayloadAssets(HST_CampaignState state)
	{
		int count;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || !IsSmokeConvoyInstanceId(asset.m_sMissionInstanceId))
				continue;
			if (asset.m_sRole == "convoy_payload" || asset.m_sRole == "convoy_captive")
				count++;
		}
		return count;
	}

	protected int CountSmokeConvoyDestroyedAssets(HST_CampaignState state)
	{
		int count;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_bDestroyed && IsSmokeConvoyInstanceId(asset.m_sMissionInstanceId))
				count++;
		}
		return count;
	}

	protected bool IsSmokeConvoyInstanceId(string instanceId)
	{
		return instanceId == SMOKE_CONVOY_STAGING_ID || instanceId == SMOKE_CONVOY_MOVING_ID || instanceId == SMOKE_CONVOY_CONTACT_ID;
	}

	protected int CountDuplicateMissionAssets(HST_CampaignState state)
	{
		int duplicates;
		for (int i = 0; i < state.m_aMissionAssets.Count(); i++)
		{
			HST_MissionAssetState left = state.m_aMissionAssets[i];
			if (!left || left.m_sAssetId.IsEmpty())
				continue;
			for (int j = i + 1; j < state.m_aMissionAssets.Count(); j++)
			{
				HST_MissionAssetState right = state.m_aMissionAssets[j];
				if (right && right.m_sAssetId == left.m_sAssetId)
					duplicates++;
			}
		}
		return duplicates;
	}

	protected int CountDuplicateRuntimeEntities(HST_CampaignState state)
	{
		int duplicates;
		for (int i = 0; i < state.m_aMissionRuntimeEntities.Count(); i++)
		{
			HST_MissionRuntimeEntityState left = state.m_aMissionRuntimeEntities[i];
			if (!left || left.m_sRuntimeEntityId.IsEmpty())
				continue;
			for (int j = i + 1; j < state.m_aMissionRuntimeEntities.Count(); j++)
			{
				HST_MissionRuntimeEntityState right = state.m_aMissionRuntimeEntities[j];
				if (right && right.m_sRuntimeEntityId == left.m_sRuntimeEntityId)
					duplicates++;
			}
		}
		return duplicates;
	}

	protected int CountDuplicateActiveGroups(HST_CampaignState state)
	{
		int duplicates;
		for (int i = 0; i < state.m_aActiveGroups.Count(); i++)
		{
			HST_ActiveGroupState left = state.m_aActiveGroups[i];
			if (!left || left.m_sGroupId.IsEmpty())
				continue;
			for (int j = i + 1; j < state.m_aActiveGroups.Count(); j++)
			{
				HST_ActiveGroupState right = state.m_aActiveGroups[j];
				if (right && right.m_sGroupId == left.m_sGroupId)
					duplicates++;
			}
		}
		return duplicates;
	}

	protected int CountDuplicateRuntimeVehicles(HST_CampaignState state)
	{
		int duplicates;
		for (int i = 0; i < state.m_aRuntimeVehicles.Count(); i++)
		{
			HST_RuntimeVehicleState left = state.m_aRuntimeVehicles[i];
			if (!left || left.m_sVehicleRuntimeId.IsEmpty())
				continue;
			for (int j = i + 1; j < state.m_aRuntimeVehicles.Count(); j++)
			{
				HST_RuntimeVehicleState right = state.m_aRuntimeVehicles[j];
				if (right && right.m_sVehicleRuntimeId == left.m_sVehicleRuntimeId)
					duplicates++;
			}
		}
		return duplicates;
	}

	protected int ResolveSmokeCapacityCost(string kind, string role)
	{
		if (kind == "captive")
			return 2;
		if (kind == "vehicle")
			return 6;
		if (role == "city_supplies" || role == "convoy_payload")
			return 2;
		return 1;
	}

	protected int ResolveSmokeInteractionRadius(string kind)
	{
		if (kind == "vehicle")
			return 28;
		if (kind == "target" || kind == "character")
			return 22;
		return 18;
	}

	protected string ResolveSmokeGroupPrefab(string factionKey)
	{
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (faction && faction.m_aGroupPrefabs.Count() > 0)
			return faction.m_aGroupPrefabs[0];

		return "{DDF3799FA1387848}Prefabs/Groups/BLUFOR/Group_US_RifleSquad.et";
	}

	protected vector BuildSmokeOffsetPosition(vector basePosition, float xOffset, float zOffset)
	{
		vector result = basePosition;
		result[0] = result[0] + xOffset;
		result[2] = result[2] + zOffset;
		return result;
	}

	protected string ResolveResistanceFaction(HST_CampaignPreset preset)
	{
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			return preset.m_sResistanceFactionKey;

		return "FIA";
	}

	protected string ResolveOccupierFaction(HST_CampaignPreset preset)
	{
		if (preset && !preset.m_sOccupierFactionKey.IsEmpty())
			return preset.m_sOccupierFactionKey;

		return SMOKE_GARRISON_FACTION;
	}

	protected vector ResolveSmokePosition(HST_CampaignState state)
	{
		if (state && state.m_bHQDeployed)
			return state.m_vHQPosition;

		if (state)
		{
			foreach (HST_ZoneState zone : state.m_aZones)
			{
				if (zone)
					return zone.m_vPosition;
			}
		}

		return "0 0 0";
	}
}
