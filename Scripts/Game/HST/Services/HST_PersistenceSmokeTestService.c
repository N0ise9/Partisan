class HST_PersistenceSmokeTestService
{
	static const string EXPECTED_TASK_ID = "hst_smoke_persistence_expected";
	static const string SMOKE_ZONE_ID = "hst_smoke_zone";
	static const string SMOKE_GARRISON_FACTION = "US";
	static const string SMOKE_MISSION_ID = "hst_smoke_mission";
	static const string SMOKE_MISSION_DEF_ID = "dynamic_minor_city_task";
	static const string SMOKE_ASSET_ID = "hst_smoke_asset";
	static const string SMOKE_GARAGE_VEHICLE_ID = "hst_smoke_garage_vehicle";
	static const string SMOKE_ORDER_ID = "hst_smoke_order";
	static const string SMOKE_CARGO_PREFAB = "{6985327711303720}Prefabs/Objects/HST/HST_MissionProp_Cargo.et";
	static const string SMOKE_VEHICLE_PREFAB = "Prefabs/Vehicles/Wheeled/S1203/S1203_base.et";
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
		summary = summary + string.Format("|garage=%1|arsenal=%2|enemy_orders=%3|civilian_zones=%4|undercover=%5", state.m_aGarageVehicles.Count(), state.m_aArsenalItems.Count(), state.m_aEnemyOrders.Count(), state.m_aCivilianZones.Count(), state.m_aUndercoverPlayers.Count());
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
		hash = MixInt(hash, state.m_aGarageVehicles.Count());
		hash = MixInt(hash, state.m_aArsenalItems.Count());
		hash = MixInt(hash, state.m_aEnemyOrders.Count());
		hash = MixInt(hash, state.m_aCivilianZones.Count());
		hash = MixInt(hash, state.m_aUndercoverPlayers.Count());
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
