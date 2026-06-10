class HST_CivilianService
{
	static const int MIN_CIVILIAN_CHARACTER_PREFABS = 6;

	static const int HEAT_DECAY_SECONDS = 300;
	static const int UNDERCOVER_RECHECK_SECONDS = 20;
	static const float PLAYER_USED_VEHICLE_DETACH_DISTANCE_METERS = 35.0;

	protected ref array<string> m_aRuntimeZoneIds = {};
	protected ref array<string> m_aRuntimeEntityZoneIds = {};
	protected ref array<string> m_aRuntimeEntityKinds = {};
	protected ref array<vector> m_aRuntimeEntitySpawnPositions = {};
	protected ref array<IEntity> m_aRuntimeEntities = {};
	protected int m_iRuntimeSpawnFailureCount;
	protected string m_sLastRuntimeSpawnFailurePrefab;
	protected bool m_bWarnedMissingCivilianCharacterPool;

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
		PruneDeletedRuntimeEntities(state);

		if (!balance.m_bCivilianPopulationEnabled)
		{
			bool cleaned = CleanupAllRuntimeEntities(state);
			return PublishRuntimeDiagnostics(state) || cleaned;
		}

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

			if (CleanupZoneRuntimeEntities(state, zone.m_sZoneId))
				changed = true;
		}

		return PublishRuntimeDiagnostics(state) || changed;
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
				if (civilianCount > 0 && CountGuidQualifiedCivilianCharacterPrefabs(balance) < MIN_CIVILIAN_CHARACTER_PREFABS)
					WarnMissingCivilianCharacterPool(zone, civilianCount);

				for (int i = 0; i < civilianCount; i++)
				{
					int civilianSeed = BuildZoneSeed(state, zone, 1000 + i);
					string civilianPrefab = SelectCivilianCharacterPrefab(balance, i, civilianSeed);
					if (civilianPrefab.IsEmpty())
						continue;

					vector position = ResolveTownCharacterSpawnPosition(zone, i, civilianSeed);
					vector angles = BuildSpawnAngles(civilianSeed);
					if (SpawnRuntimeEntity(state, zone.m_sZoneId, civilianPrefab, position, angles, "CIV", "CIV_CHARACTER"))
						spawned++;
				}
			}

			civilianVehicleCount = ResolveDeterministicCount(balance.m_iCivilianVehicleMinPerTown, balance.m_iCivilianVehicleMaxPerTown, BuildZoneSeed(state, zone, 101));
			for (int j = 0; j < civilianVehicleCount; j++)
			{
				int civilianVehicleSeed = BuildZoneSeed(state, zone, 200 + j);
				vector vehiclePosition = ResolveTownVehicleSpawnPosition(zone, j, false);
				vector vehicleAngles = BuildSpawnAngles(civilianVehicleSeed);
				if (SpawnRuntimeEntity(state, zone.m_sZoneId, SelectCivilianVehiclePrefab(balance, j, civilianVehicleSeed), vehiclePosition, vehicleAngles, "CIV", "CIV_VEHICLE"))
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
			vector ownerAngles = BuildSpawnAngles(BuildZoneSeed(state, zone, 700 + k));
			if (SpawnRuntimeEntity(state, zone.m_sZoneId, ownerPrefab, ownerPosition, ownerAngles, zone.m_sOwnerFactionKey, "MILITARY_VEHICLE"))
				spawned++;
		}

		Print(string.Format("h-istasi civilians | zone %1 active | spawned %2 runtime civilian/military ambience entity(s)", zone.m_sZoneId, spawned));
		return true;
	}

	protected bool SpawnRuntimeEntity(HST_CampaignState state, string zoneId, string prefab, vector position, vector angles, string factionKey, string runtimeKind)
	{
		if (zoneId.IsEmpty() || prefab.IsEmpty())
			return false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
			return false;

		if (IsRuntimeVehicle(runtimeKind))
		{
			vector safeVehiclePosition;
			if (HST_WorldPositionService.TryResolveVehicleSpawnPosition(position, safeVehiclePosition, true))
				position = safeVehiclePosition;
			else
				position = HST_WorldPositionService.ResolveGroundPosition(position, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, true);
			angles = HST_WorldPositionService.BuildUprightAnglesFromVector(angles);
		}
		else
		{
			position = HST_WorldPositionService.ResolveSafeGroundPosition(position, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 2.0);
		}

		GenericEntity entity = respawnSystem.DoSpawn(prefab, position, angles);
		if (!entity)
		{
			RecordSpawnFailure(zoneId, prefab, runtimeKind, position, angles);
			return false;
		}

		ApplyFaction(entity, factionKey, runtimeKind);
		m_aRuntimeEntityZoneIds.Insert(zoneId);
		m_aRuntimeEntityKinds.Insert(runtimeKind);
		m_aRuntimeEntitySpawnPositions.Insert(position);
		m_aRuntimeEntities.Insert(entity);
		RegisterRuntimeVehicle(state, entity, zoneId, prefab, position, angles, factionKey, runtimeKind);
		Print(string.Format("h-istasi civilians | spawn ok | zone %1 | kind %2 | faction %3 | prefab %4 | pos %5 | yaw %6", zoneId, runtimeKind, factionKey, prefab, position, angles[0]));
		return true;
	}

	protected bool CleanupZoneRuntimeEntities(HST_CampaignState state, string zoneId)
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
				MarkRuntimeVehicleDetached(state, entity);
				Print(string.Format("h-istasi civilians | detached player-used runtime %1 from town cleanup for %2", runtimeKind, zoneId));
			}
			else if (entity)
			{
				MarkRuntimeVehicleDeleted(state, entity);
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			}

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

	protected bool CleanupAllRuntimeEntities(HST_CampaignState state)
	{
		bool changed;
		for (int i = m_aRuntimeEntities.Count() - 1; i >= 0; i--)
		{
			IEntity entity = m_aRuntimeEntities[i];
			string runtimeKind = m_aRuntimeEntityKinds[i];
			vector spawnPosition = m_aRuntimeEntitySpawnPositions[i];
			if (entity && ShouldDetachFromTownCleanup(entity, runtimeKind, spawnPosition))
			{
				MarkRuntimeVehicleDetached(state, entity);
				Print(string.Format("h-istasi civilians | detached player-used runtime %1 while civilian runtime disabled", runtimeKind));
			}
			else if (entity)
			{
				MarkRuntimeVehicleDeleted(state, entity);
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			}

			changed = true;
		}

		m_aRuntimeEntities.Clear();
		m_aRuntimeEntityZoneIds.Clear();
		m_aRuntimeEntityKinds.Clear();
		m_aRuntimeEntitySpawnPositions.Clear();
		m_aRuntimeZoneIds.Clear();
		return changed;
	}

	protected void PruneDeletedRuntimeEntities(HST_CampaignState state)
	{
		for (int i = m_aRuntimeEntities.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeEntities[i])
				continue;

			MarkRuntimeVehicleDeletedBySpawnRecord(state, m_aRuntimeEntitySpawnPositions[i], m_aRuntimeEntityKinds[i]);
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

	protected vector ResolveTownCharacterSpawnPosition(HST_ZoneState zone, int index, int seed)
	{
		if (!zone)
			return "0 0 0";

		vector offset = "0 0 0";
		float radius = 8.0 + ModInt(seed, 24);
		float angle = ModInt(seed * 29 + index * 43, 360) * 0.0174533;
		offset[0] = Math.Sin(angle) * radius;
		offset[2] = Math.Cos(angle) * radius;
		return HST_WorldPositionService.ResolveGroundPosition(zone.m_vPosition + offset, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true);
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

	protected vector BuildSpawnAngles(int seed)
	{
		vector angles;
		angles[0] = ModInt(seed * 47 + 91, 360);
		return HST_WorldPositionService.BuildUprightAnglesFromVector(angles);
	}

	protected string SelectCivilianCharacterPrefab(HST_BalanceConfig balance, int index, int seed)
	{
		if (!balance || CountGuidQualifiedCivilianCharacterPrefabs(balance) < MIN_CIVILIAN_CHARACTER_PREFABS)
			return "";

		for (int i = 0; i < balance.m_aCivilianCharacterPrefabs.Count(); i++)
		{
			string prefab = balance.m_aCivilianCharacterPrefabs[ModInt(seed + index + i, balance.m_aCivilianCharacterPrefabs.Count())];
			if (IsGuidQualifiedResource(prefab))
				return prefab;
		}

		return "";
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
				if (IsTownGroundVehicleResource(factionVehicle) && !IsKnownInvalidVehicleResource(factionVehicle))
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

	protected bool IsGuidQualifiedResource(string prefab)
	{
		return !prefab.IsEmpty() && prefab.Contains("{") && prefab.Contains("}") && prefab.Contains("Prefabs/");
	}

	protected int CountGuidQualifiedCivilianCharacterPrefabs(HST_BalanceConfig balance)
	{
		if (!balance)
			return 0;

		int validCount;
		foreach (string prefab : balance.m_aCivilianCharacterPrefabs)
		{
			if (IsGuidQualifiedResource(prefab))
				validCount++;
		}

		return validCount;
	}

	protected bool IsKnownInvalidVehicleResource(string prefab)
	{
		return prefab.Contains("Car_M1151.et");
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

		if (runtimeKind == "CIV_VEHICLE" || runtimeKind == "CIV_CHARACTER")
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

	protected void RegisterRuntimeVehicle(HST_CampaignState state, IEntity entity, string zoneId, string prefab, vector position, vector angles, string factionKey, string runtimeKind)
	{
		if (!state || !entity || !IsRuntimeVehicle(runtimeKind) || !HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
			return;

		string vehicleRuntimeId = ResolveRuntimeVehicleId(entity);
		if (vehicleRuntimeId.IsEmpty())
			return;

		state.RemoveRuntimeVehicle(vehicleRuntimeId);
		HST_RuntimeVehicleState vehicle = new HST_RuntimeVehicleState();
		vehicle.m_sVehicleRuntimeId = vehicleRuntimeId;
		vehicle.m_sPrefab = prefab;
		vehicle.m_sDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(prefab);
		vehicle.m_sFactionKey = factionKey;
		vehicle.m_sZoneId = zoneId;
		vehicle.m_sRuntimeKind = runtimeKind;
		vehicle.m_vPosition = position;
		vehicle.m_vAngles = angles;
		vehicle.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		state.m_aRuntimeVehicles.Insert(vehicle);
	}

	protected void MarkRuntimeVehicleDetached(HST_CampaignState state, IEntity entity)
	{
		HST_RuntimeVehicleState vehicle = ResolveRuntimeVehicleRecord(state, entity);
		if (!vehicle)
			return;

		vehicle.m_bDetached = true;
		vehicle.m_vPosition = entity.GetOrigin();
		vehicle.m_vAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(entity.GetYawPitchRoll());
	}

	protected void MarkRuntimeVehicleDeleted(HST_CampaignState state, IEntity entity)
	{
		HST_RuntimeVehicleState vehicle = ResolveRuntimeVehicleRecord(state, entity);
		if (!vehicle)
			return;

		vehicle.m_bDeleted = true;
		vehicle.m_vPosition = entity.GetOrigin();
		vehicle.m_vAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(entity.GetYawPitchRoll());
	}

	protected void MarkRuntimeVehicleDeletedBySpawnRecord(HST_CampaignState state, vector spawnPosition, string runtimeKind)
	{
		if (!state || !IsRuntimeVehicle(runtimeKind))
			return;

		foreach (HST_RuntimeVehicleState vehicle : state.m_aRuntimeVehicles)
		{
			if (!vehicle || vehicle.m_bDeleted || vehicle.m_sRuntimeKind != runtimeKind)
				continue;

			if (DistanceSq2D(vehicle.m_vPosition, spawnPosition) > 4.0)
				continue;

			vehicle.m_bDeleted = true;
			return;
		}
	}

	protected HST_RuntimeVehicleState ResolveRuntimeVehicleRecord(HST_CampaignState state, IEntity entity)
	{
		if (!state || !entity)
			return null;

		string vehicleRuntimeId = ResolveRuntimeVehicleId(entity);
		if (!vehicleRuntimeId.IsEmpty())
			return state.FindRuntimeVehicle(vehicleRuntimeId);

		return null;
	}

	protected string ResolveRuntimeVehicleId(IEntity entity)
	{
		if (!entity)
			return "";

		BaseRplComponent rpl = BaseRplComponent.Cast(entity.FindComponent(BaseRplComponent));
		if (rpl)
			return string.Format("rpl_%1", rpl.Id());

		string prefab = "";
		if (entity.GetPrefabData())
			prefab = entity.GetPrefabData().GetPrefabName();

		return string.Format("local_%1_%2", prefab, entity.GetOrigin());
	}

	protected void WarnMissingCivilianCharacterPool(HST_ZoneState zone, int requestedCount)
	{
		if (!m_bWarnedMissingCivilianCharacterPool)
		{
			Print(string.Format("h-istasi civilians | fewer than %1 GUID-qualified civilian character prefabs configured; skipping civilian character ambience instead of spawning broken CIV group shells", MIN_CIVILIAN_CHARACTER_PREFABS), LogLevel.WARNING);
			m_bWarnedMissingCivilianCharacterPool = true;
		}

		if (zone)
			RecordSpawnFailure(zone.m_sZoneId, "<missing minimum GUID-qualified civilian character pool>", "CIV_CHARACTER", zone.m_vPosition, BuildSpawnAngles(0));
	}

	protected void RecordSpawnFailure(string zoneId, string prefab, string runtimeKind, vector position, vector angles)
	{
		m_iRuntimeSpawnFailureCount++;
		m_sLastRuntimeSpawnFailurePrefab = prefab;
		Print(string.Format("h-istasi civilians | spawn failed | zone %1 | kind %2 | prefab %3 | pos %4 | yaw %5", zoneId, runtimeKind, prefab, position, angles[0]), LogLevel.WARNING);
	}

	protected bool PublishRuntimeDiagnostics(HST_CampaignState state)
	{
		if (!state)
			return false;

		int civilianCharacters;
		int civilianVehicles;
		int militaryVehicles;
		for (int i = 0; i < m_aRuntimeEntityKinds.Count(); i++)
		{
			string runtimeKind = m_aRuntimeEntityKinds[i];
			if (runtimeKind == "CIV_CHARACTER")
				civilianCharacters++;
			else if (runtimeKind == "CIV_VEHICLE")
				civilianVehicles++;
			else if (runtimeKind == "MILITARY_VEHICLE")
				militaryVehicles++;
		}

		bool changed;
		if (state.m_iRuntimeCivilianCharacterCount != civilianCharacters)
		{
			state.m_iRuntimeCivilianCharacterCount = civilianCharacters;
			changed = true;
		}

		if (state.m_iRuntimeCivilianVehicleCount != civilianVehicles)
		{
			state.m_iRuntimeCivilianVehicleCount = civilianVehicles;
			changed = true;
		}

		if (state.m_iRuntimeMilitaryVehicleCount != militaryVehicles)
		{
			state.m_iRuntimeMilitaryVehicleCount = militaryVehicles;
			changed = true;
		}

		if (state.m_iRuntimeSpawnFailureCount != m_iRuntimeSpawnFailureCount)
		{
			state.m_iRuntimeSpawnFailureCount = m_iRuntimeSpawnFailureCount;
			changed = true;
		}

		if (state.m_sLastRuntimeSpawnFailurePrefab != m_sLastRuntimeSpawnFailurePrefab)
		{
			state.m_sLastRuntimeSpawnFailurePrefab = m_sLastRuntimeSpawnFailurePrefab;
			changed = true;
		}

		return changed;
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
