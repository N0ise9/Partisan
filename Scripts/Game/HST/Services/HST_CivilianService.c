class HST_CivilianService
{
	static const int HEAT_DECAY_SECONDS = 300;
	static const int UNDERCOVER_RECHECK_SECONDS = 20;
	static const int CIVILIAN_GROUP_SIZE = 4;
	static const float PLAYER_USED_VEHICLE_DETACH_DISTANCE_METERS = 35.0;
	static const string CIVILIAN_GROUP_PREFAB = "{6985327711303600}Prefabs/Groups/CIV/HST_CivilianTownGroup.et";

	protected ref array<string> m_aRuntimeZoneIds = {};
	protected ref array<string> m_aRuntimeEntityZoneIds = {};
	protected ref array<string> m_aRuntimeEntityKinds = {};
	protected ref array<vector> m_aRuntimeEntitySpawnPositions = {};
	protected ref array<IEntity> m_aRuntimeEntities = {};

	bool EnsureCivilianZones(HST_CampaignState state)
	{
		if (!state || state.m_aCivilianZones.Count() > 0)
			return false;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
				continue;

			HST_CivilianZoneState civilianZone = new HST_CivilianZoneState();
			civilianZone.m_sZoneId = zone.m_sZoneId;
			civilianZone.m_iReputation = 50;
			civilianZone.m_iCivilianPresence = Math.Max(4, zone.m_iIncomeValue / 8);
			civilianZone.m_iPolicePresence = Math.Max(1, zone.m_iGarrisonSlots / 6);
			civilianZone.m_iRoadblockPresence = 1;
			civilianZone.m_bUndercoverRestricted = zone.m_iSupport < 25;
			state.m_aCivilianZones.Insert(civilianZone);
		}

		Print(string.Format("h-istasi | initialized %1 civilian town record(s)", state.m_aCivilianZones.Count()));
		return true;
	}

	bool Tick(HST_CampaignState state, int elapsedSeconds)
	{
		if (!state || elapsedSeconds <= 0)
			return false;

		bool changed;
		foreach (HST_CivilianZoneState civilianZone : state.m_aCivilianZones)
		{
			if (!civilianZone || civilianZone.m_iWantedHeat <= 0)
				continue;

			if (state.m_iElapsedSeconds < civilianZone.m_iLastIncidentSecond + HEAT_DECAY_SECONDS)
				continue;

			civilianZone.m_iWantedHeat = Math.Max(0, civilianZone.m_iWantedHeat - 1);
			civilianZone.m_iLastIncidentSecond = state.m_iElapsedSeconds;
			changed = true;
		}

		foreach (HST_PlayerUndercoverState undercover : state.m_aUndercoverPlayers)
		{
			if (!undercover || undercover.m_iWantedHeat <= 0)
				continue;

			if (undercover.m_eStatus == HST_EUndercoverStatus.HST_UNDERCOVER_COMPROMISED && state.m_iElapsedSeconds < undercover.m_iCompromisedUntilSecond)
				continue;

			undercover.m_iWantedHeat = Math.Max(0, undercover.m_iWantedHeat - 1);
			if (undercover.m_iWantedHeat == 0)
			{
				undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
				undercover.m_sLastReason = "heat cooled";
			}

			changed = true;
		}

		return changed;
	}

	bool UpdatePhysicalTownPopulation(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance)
	{
		if (!state || !balance)
			return false;

		HST_DefaultCatalog.EnsureCivilianPools(balance);
		PruneDeletedRuntimeEntities();

		if (!balance.m_bCivilianPopulationEnabled)
			return CleanupAllRuntimeEntities();

		bool changed;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			if (zone.m_bActive)
			{
				if (!HasRuntimeZone(zone.m_sZoneId))
					changed = SpawnActiveZoneRuntime(state, preset, balance, zone) || changed;

				continue;
			}

			if (CleanupZoneRuntimeEntities(zone.m_sZoneId))
				changed = true;
		}

		return changed;
	}

	HST_PlayerUndercoverState EnsurePlayer(HST_CampaignState state, string identityId)
	{
		if (!state || identityId.IsEmpty())
			return null;

		HST_PlayerUndercoverState undercover = state.FindUndercoverPlayer(identityId);
		if (undercover)
			return undercover;

		undercover = new HST_PlayerUndercoverState();
		undercover.m_sIdentityId = identityId;
		state.m_aUndercoverPlayers.Insert(undercover);
		return undercover;
	}

	bool RegisterIncident(HST_CampaignState state, string zoneId, int reputationDelta, int heatDelta, string reason)
	{
		if (!state || zoneId.IsEmpty())
			return false;

		HST_CivilianZoneState civilianZone = state.FindCivilianZone(zoneId);
		if (!civilianZone)
			return false;

		civilianZone.m_iReputation = Math.Max(0, Math.Min(100, civilianZone.m_iReputation + reputationDelta));
		civilianZone.m_iWantedHeat = Math.Max(0, civilianZone.m_iWantedHeat + heatDelta);
		civilianZone.m_iLastIncidentSecond = state.m_iElapsedSeconds;
		HST_ZoneState zone = state.FindZone(zoneId);
		if (zone)
			zone.m_iSupport = Math.Max(-100, Math.Min(100, zone.m_iSupport + reputationDelta / 2));

		return true;
	}

	bool CheckUndercover(HST_CampaignState state, string identityId, string zoneId, bool visiblyArmed, bool suspiciousVehicle, bool recentCombat)
	{
		HST_PlayerUndercoverState undercover = EnsurePlayer(state, identityId);
		if (!undercover)
			return false;

		if (state.m_iElapsedSeconds < undercover.m_iLastCheckedSecond + UNDERCOVER_RECHECK_SECONDS)
			return false;

		undercover.m_iLastCheckedSecond = state.m_iElapsedSeconds;
		HST_CivilianZoneState civilianZone = state.FindCivilianZone(zoneId);
		if (!civilianZone)
		{
			undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
			undercover.m_sLastReason = "outside civilian zone";
			return true;
		}

		if (recentCombat || visiblyArmed)
		{
			undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_COMPROMISED;
			undercover.m_iWantedHeat = Math.Max(undercover.m_iWantedHeat, 4);
			undercover.m_iCompromisedUntilSecond = state.m_iElapsedSeconds + 180;
			undercover.m_sLastReason = "armed or combat exposure";
			civilianZone.m_iWantedHeat = Math.Max(civilianZone.m_iWantedHeat, 3);
			return true;
		}

		if (suspiciousVehicle || civilianZone.m_bUndercoverRestricted || civilianZone.m_iWantedHeat >= 4)
		{
			undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_SUSPICIOUS;
			undercover.m_iWantedHeat = Math.Max(undercover.m_iWantedHeat, 1);
			undercover.m_sLastReason = "suspicious presence";
			return true;
		}

		undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
		undercover.m_sLastReason = "clear";
		return true;
	}

	string BuildCivilianReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi civilians | state not ready";

		int reputationTotal;
		int heatTotal;
		int policeTotal;
		int roadblocks;
		foreach (HST_CivilianZoneState civilianZone : state.m_aCivilianZones)
		{
			if (!civilianZone)
				continue;

			reputationTotal += civilianZone.m_iReputation;
			heatTotal += civilianZone.m_iWantedHeat;
			policeTotal += civilianZone.m_iPolicePresence;
			roadblocks += civilianZone.m_iRoadblockPresence;
		}

		int averageReputation;
		if (state.m_aCivilianZones.Count() > 0)
			averageReputation = reputationTotal / state.m_aCivilianZones.Count();

		return string.Format("h-istasi civilians | towns %1 | avg reputation %2 | heat %3 | police %4 | roadblocks %5", state.m_aCivilianZones.Count(), averageReputation, heatTotal, policeTotal, roadblocks);
	}

	string BuildUndercoverReport(HST_CampaignState state, string identityId = "")
	{
		if (!state)
			return "h-istasi undercover | state not ready";

		if (!identityId.IsEmpty())
		{
			HST_PlayerUndercoverState undercover = state.FindUndercoverPlayer(identityId);
			if (!undercover)
				return "h-istasi undercover | no record";

			return string.Format("h-istasi undercover | %1 | status %2 | heat %3 | reason %4", identityId, undercover.m_eStatus, undercover.m_iWantedHeat, undercover.m_sLastReason);
		}

		int suspicious;
		int compromised;
		foreach (HST_PlayerUndercoverState undercoverPlayer : state.m_aUndercoverPlayers)
		{
			if (!undercoverPlayer)
				continue;

			if (undercoverPlayer.m_eStatus == HST_EUndercoverStatus.HST_UNDERCOVER_SUSPICIOUS)
				suspicious++;
			else if (undercoverPlayer.m_eStatus == HST_EUndercoverStatus.HST_UNDERCOVER_COMPROMISED || undercoverPlayer.m_eStatus == HST_EUndercoverStatus.HST_UNDERCOVER_WANTED)
				compromised++;
		}

		return string.Format("h-istasi undercover | tracked %1 | suspicious %2 | compromised/wanted %3", state.m_aUndercoverPlayers.Count(), suspicious, compromised);
	}

	protected bool SpawnActiveZoneRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ZoneState zone)
	{
		if (!state || !balance || !zone)
			return false;

		m_aRuntimeZoneIds.Insert(zone.m_sZoneId);

		int spawned;
		int civilianVehicleCount;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
		{
			HST_CivilianZoneState civilianZone = state.FindCivilianZone(zone.m_sZoneId);
			if (civilianZone)
			{
				int civilianCount = Math.Min(civilianZone.m_iCivilianPresence, balance.m_iCivilianMaxActivePerTown);
				int civilianGroupCount = ResolveCivilianGroupCount(civilianCount);
				for (int i = 0; i < civilianGroupCount; i++)
				{
					vector position = ResolveTownSpawnPosition(zone, i, HST_WorldPositionService.CHARACTER_GROUND_OFFSET);
					if (SpawnRuntimeEntity(zone.m_sZoneId, SelectCivilianGroupPrefab(balance, i, BuildZoneSeed(state, zone, i)), position, "CIV", "CIV_GROUP"))
						spawned++;
				}
			}

			civilianVehicleCount = ResolveDeterministicCount(balance.m_iCivilianVehicleMinPerTown, balance.m_iCivilianVehicleMaxPerTown, BuildZoneSeed(state, zone, 101));
			for (int j = 0; j < civilianVehicleCount; j++)
			{
				vector vehiclePosition = ResolveTownVehicleSpawnPosition(zone, j, false);
				if (SpawnRuntimeEntity(zone.m_sZoneId, SelectCivilianVehiclePrefab(balance, j, BuildZoneSeed(state, zone, 200 + j)), vehiclePosition, "CIV", "CIV_VEHICLE"))
					spawned++;
			}
		}

		int militaryVehicleCount = ResolveDeterministicCount(balance.m_iOccupierVehicleMinPerTown, balance.m_iOccupierVehicleMaxPerTown, BuildZoneSeed(state, zone, 401));
		for (int k = 0; k < militaryVehicleCount; k++)
		{
			string ownerPrefab = SelectFactionVehiclePrefab(zone.m_sOwnerFactionKey, k + BuildZoneSeed(state, zone, 503));
			if (ownerPrefab.IsEmpty())
				continue;

			vector ownerPosition = ResolveTownVehicleSpawnPosition(zone, civilianVehicleCount + k, true);
			if (SpawnRuntimeEntity(zone.m_sZoneId, ownerPrefab, ownerPosition, zone.m_sOwnerFactionKey, "MILITARY_VEHICLE"))
				spawned++;
		}

		Print(string.Format("h-istasi civilians | zone %1 active | spawned %2 runtime civilian/military ambience entity(s)", zone.m_sZoneId, spawned));
		return true;
	}

	protected bool SpawnRuntimeEntity(string zoneId, string prefab, vector position, string factionKey, string runtimeKind)
	{
		if (zoneId.IsEmpty() || prefab.IsEmpty())
			return false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
			return false;

		GenericEntity entity = respawnSystem.DoSpawn(prefab, position, "0 0 0");
		if (!entity)
		{
			Print(string.Format("h-istasi civilians | spawn failed for %1 at %2 in %3", prefab, position, zoneId), LogLevel.WARNING);
			return false;
		}

		ApplyFaction(entity, factionKey, runtimeKind);
		m_aRuntimeEntityZoneIds.Insert(zoneId);
		m_aRuntimeEntityKinds.Insert(runtimeKind);
		m_aRuntimeEntitySpawnPositions.Insert(position);
		m_aRuntimeEntities.Insert(entity);
		return true;
	}

	protected bool CleanupZoneRuntimeEntities(string zoneId)
	{
		if (zoneId.IsEmpty())
			return false;

		bool changed;
		for (int i = m_aRuntimeEntities.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeEntityZoneIds[i] != zoneId)
				continue;

			IEntity entity = m_aRuntimeEntities[i];
			string runtimeKind = m_aRuntimeEntityKinds[i];
			vector spawnPosition = m_aRuntimeEntitySpawnPositions[i];
			if (entity && ShouldDetachFromTownCleanup(entity, runtimeKind, spawnPosition))
			{
				Print(string.Format("h-istasi civilians | detached player-used runtime %1 from town cleanup for %2", runtimeKind, zoneId));
			}
			else if (entity)
				SCR_EntityHelper.DeleteEntityAndChildren(entity);

			m_aRuntimeEntities.Remove(i);
			m_aRuntimeEntityZoneIds.Remove(i);
			m_aRuntimeEntityKinds.Remove(i);
			m_aRuntimeEntitySpawnPositions.Remove(i);
			changed = true;
		}

		for (int j = m_aRuntimeZoneIds.Count() - 1; j >= 0; j--)
		{
			if (m_aRuntimeZoneIds[j] == zoneId)
				m_aRuntimeZoneIds.Remove(j);
		}

		if (changed)
			Print(string.Format("h-istasi civilians | cleaned runtime town population for %1", zoneId));

		return changed;
	}

	protected bool CleanupAllRuntimeEntities()
	{
		bool changed;
		for (int i = m_aRuntimeEntities.Count() - 1; i >= 0; i--)
		{
			IEntity entity = m_aRuntimeEntities[i];
			string runtimeKind = m_aRuntimeEntityKinds[i];
			vector spawnPosition = m_aRuntimeEntitySpawnPositions[i];
			if (entity && ShouldDetachFromTownCleanup(entity, runtimeKind, spawnPosition))
			{
				Print(string.Format("h-istasi civilians | detached player-used runtime %1 while civilian runtime disabled", runtimeKind));
			}
			else if (entity)
				SCR_EntityHelper.DeleteEntityAndChildren(entity);

			changed = true;
		}

		m_aRuntimeEntities.Clear();
		m_aRuntimeEntityZoneIds.Clear();
		m_aRuntimeEntityKinds.Clear();
		m_aRuntimeEntitySpawnPositions.Clear();
		m_aRuntimeZoneIds.Clear();
		return changed;
	}

	protected void PruneDeletedRuntimeEntities()
	{
		for (int i = m_aRuntimeEntities.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeEntities[i])
				continue;

			m_aRuntimeEntities.Remove(i);
			m_aRuntimeEntityZoneIds.Remove(i);
			m_aRuntimeEntityKinds.Remove(i);
			m_aRuntimeEntitySpawnPositions.Remove(i);
		}
	}

	protected bool HasRuntimeZone(string zoneId)
	{
		return m_aRuntimeZoneIds.Contains(zoneId);
	}

	protected vector ResolveTownSpawnPosition(HST_ZoneState zone, int index, float verticalOffset)
	{
		vector offset = "0 0 0";
		int column = ModInt(index, 5);
		int row = index / 5;
		offset[0] = (column - 2) * 9;
		offset[2] = (row - 1) * 11;
		return HST_WorldPositionService.ResolveGroundPosition(zone.m_vPosition + offset, verticalOffset, true);
	}

	protected vector ResolveTownVehicleSpawnPosition(HST_ZoneState zone, int index, bool militaryVehicle)
	{
		if (!zone)
			return "0 0 0";

		int seed = BuildTownSpawnSeed(zone, index, militaryVehicle);
		float radius = 18.0 + ModInt(seed, 29);
		if (militaryVehicle)
			radius = 28.0 + ModInt(seed, 34);

		float angle = ModInt(seed * 37, 360) * 0.0174533;
		vector offset = "0 0 0";
		offset[0] = Math.Sin(angle) * radius;
		offset[2] = Math.Cos(angle) * radius;
		return HST_WorldPositionService.ResolveGroundPosition(zone.m_vPosition + offset, HST_WorldPositionService.PROP_GROUND_OFFSET, true);
	}

	protected int BuildTownSpawnSeed(HST_ZoneState zone, int index, bool militaryVehicle)
	{
		int seed = 17 + index * 53;
		if (zone)
		{
			seed = seed + zone.m_iPriority * 31 + zone.m_sZoneId.Length() * 19;
			seed = seed + Math.Round(zone.m_vPosition[0]) + Math.Round(zone.m_vPosition[2]);
		}

		if (militaryVehicle)
			seed = seed + 997;

		if (seed < 0)
			seed = -seed;

		return seed;
	}

	protected int BuildZoneSeed(HST_CampaignState state, HST_ZoneState zone, int salt)
	{
		int seed = salt * 31;
		if (state)
			seed += state.m_iCampaignSeed * 17;
		if (zone)
		{
			seed += zone.m_iPriority * 37;
			seed += zone.m_iIncomeValue * 13;
			seed += zone.m_sZoneId.Length() * 101;
			seed += Math.Round(zone.m_vPosition[0]) + Math.Round(zone.m_vPosition[2]);
		}

		if (seed < 0)
			seed = -seed;

		return seed;
	}

	protected int ResolveDeterministicCount(int minCount, int maxCount, int seed)
	{
		if (maxCount <= 0)
			return 0;

		if (maxCount <= minCount)
			return Math.Max(0, minCount);

		int span = maxCount - minCount + 1;
		int value = ModInt(seed, span);

		return minCount + value;
	}

	protected int ModInt(int value, int divisor)
	{
		if (divisor <= 0)
			return 0;

		int result = value - (value / divisor) * divisor;
		if (result < 0)
			result = result + divisor;

		return result;
	}

	protected int ResolveCivilianGroupCount(int civilianCount)
	{
		if (civilianCount <= 0)
			return 0;

		return (civilianCount + CIVILIAN_GROUP_SIZE - 1) / CIVILIAN_GROUP_SIZE;
	}

	protected string SelectCivilianGroupPrefab(HST_BalanceConfig balance, int index, int seed)
	{
		if (balance && balance.m_aCivilianGroupPrefabs.Count() > 0)
			return balance.m_aCivilianGroupPrefabs[ModInt(seed + index, balance.m_aCivilianGroupPrefabs.Count())];

		return CIVILIAN_GROUP_PREFAB;
	}

	protected string SelectCivilianVehiclePrefab(HST_BalanceConfig balance, int index, int seed)
	{
		if (balance && balance.m_aCivilianVehiclePrefabs.Count() > 0)
			return balance.m_aCivilianVehiclePrefabs[ModInt(seed + index, balance.m_aCivilianVehiclePrefabs.Count())];

		return "Prefabs/Vehicles/Wheeled/S105/S105_base.et";
	}

	protected string SelectFactionVehiclePrefab(string factionKey, int index)
	{
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (faction && faction.m_aVehiclePrefabs.Count() > 0)
		{
			for (int i = 0; i < faction.m_aVehiclePrefabs.Count(); i++)
			{
				string factionVehicle = faction.m_aVehiclePrefabs[ModInt(index + i, faction.m_aVehiclePrefabs.Count())];
				if (IsTownGroundVehicleResource(factionVehicle))
					return factionVehicle;
			}
		}

		Print(string.Format("h-istasi civilians | no non-aircraft faction vehicle available for %1; skipping owner vehicle", factionKey), LogLevel.WARNING);
		return "";
	}

	protected bool IsTownGroundVehicleResource(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		if (!prefab.Contains("Prefabs/Vehicles/"))
			return false;

		return !IsAircraftVehicleResource(prefab);
	}

	protected bool IsAircraftVehicleResource(string prefab)
	{
		return prefab.Contains("Aircraft") || prefab.Contains("Airplane") || prefab.Contains("Plane") || prefab.Contains("Helicopter") || prefab.Contains("Helicopters") || prefab.Contains("/UH") || prefab.Contains("/AH") || prefab.Contains("/Mi") || prefab.Contains("/KA") || prefab.Contains("/Ka");
	}

	protected void ApplyFaction(IEntity entity, string factionKey, string runtimeKind)
	{
		if (!entity || factionKey.IsEmpty())
			return;

		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!factionComponent)
			return;

		if (runtimeKind == "CIV_VEHICLE")
		{
			factionComponent.SetAffiliatedFactionByKey("CIV");
			return;
		}

		factionComponent.SetAffiliatedFactionByKey(factionKey);
	}

	protected bool IsRuntimeVehicle(string runtimeKind)
	{
		return runtimeKind.Contains("VEHICLE");
	}

	protected bool ShouldDetachFromTownCleanup(IEntity entity, string runtimeKind, vector spawnPosition)
	{
		if (!entity || !IsRuntimeVehicle(runtimeKind))
			return false;

		float distanceSq = DistanceSq2D(entity.GetOrigin(), spawnPosition);
		return distanceSq >= PLAYER_USED_VEHICLE_DETACH_DISTANCE_METERS * PLAYER_USED_VEHICLE_DETACH_DISTANCE_METERS;
	}

	protected float DistanceSq2D(vector first, vector second)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz;
	}
}
