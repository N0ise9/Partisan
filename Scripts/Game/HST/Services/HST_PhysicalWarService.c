class HST_PhysicalWarService
{
	static const int MAX_ACTIVE_INFANTRY_PER_ZONE = 6;
	static const int MAX_ACTIVE_VEHICLES_PER_ZONE = 1;
	static const int QRF_ATTACK_RESOURCE_COST = 15;
	static const int QRF_SUPPORT_RESOURCE_COST = 5;
	static const int QRF_ETA_SECONDS = 180;
	static const int QRF_COOLDOWN_SECONDS = 900;
	static const int ROUTE_STATE_UPDATE_SECONDS = 30;
	static const float HQ_SAFE_RADIUS_METERS = 900;

	protected ref array<string> m_aRuntimeGroupIds = {};
	protected ref array<IEntity> m_aRuntimeGroupEntities = {};

	bool UpdateZoneActivation(HST_CampaignState state, HST_BalanceConfig balance, HST_EnemyDirectorService enemyDirector = null)
	{
		if (!state || !balance)
			return false;

		EnsureRuntimeGroupEntities(state);

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		bool changed;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			bool shouldBeActive = !IsZoneInsideHQSafeArea(state, zone) && IsAnyLivingPlayerNearZone(playerManager, playerIds, zone, balance);
			if (zone.m_bActive == shouldBeActive)
				continue;

			zone.m_bActive = shouldBeActive;
			if (shouldBeActive)
				changed = ActivateZone(state, zone) || changed;
			else
				changed = DeactivateZone(state, zone) || changed;

			changed = true;
			Print(string.Format("h-istasi | zone %1 physical activation = %2", zone.m_sZoneId, shouldBeActive));
		}

		if (UpdateQRF(state, enemyDirector))
			changed = true;
		if (UpdateActiveGroupRoutes(state))
			changed = true;

		return changed;
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

	protected bool ActivateZone(HST_CampaignState state, HST_ZoneState zone)
	{
		if (HasActiveGarrisonGroup(state, zone))
		{
			ApplyActiveZoneCounts(state, zone);
			return false;
		}

		HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, zone.m_sOwnerFactionKey);
		if (!garrison)
		{
			zone.m_iActiveInfantryCount = 0;
			zone.m_iActiveVehicleCount = 0;
			return false;
		}

		int infantryCount = Math.Min(garrison.m_iInfantryCount, ResolveActiveInfantryCap(zone));
		int vehicleCount = Math.Min(garrison.m_iVehicleCount, ResolveActiveVehicleCap(zone));
		if (infantryCount <= 0 && vehicleCount <= 0)
		{
			zone.m_iActiveInfantryCount = 0;
			zone.m_iActiveVehicleCount = 0;
			return false;
		}

		garrison.m_iInfantryCount = Math.Max(0, garrison.m_iInfantryCount - infantryCount);
		garrison.m_iVehicleCount = Math.Max(0, garrison.m_iVehicleCount - vehicleCount);

		HST_ActiveGroupState activeGroup = CreateActiveGroup(state, zone, zone.m_sOwnerFactionKey, infantryCount, vehicleCount, false);
		state.m_aActiveGroups.Insert(activeGroup);
		TrySpawnActiveGroup(activeGroup, state);
		ApplyActiveZoneCounts(state, zone);
		Print(string.Format("h-istasi | activated garrison group %1 for zone %2", activeGroup.m_sGroupId, zone.m_sZoneId));
		return true;
	}

	protected bool DeactivateZone(HST_CampaignState state, HST_ZoneState zone)
	{
		bool changed;
		for (int i = state.m_aActiveGroups.Count() - 1; i >= 0; i--)
		{
			HST_ActiveGroupState activeGroup = state.m_aActiveGroups[i];
			if (!activeGroup || activeGroup.m_sZoneId != zone.m_sZoneId || activeGroup.m_bQRF)
				continue;

			FoldActiveGroup(state, activeGroup);
			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			state.m_aActiveGroups.Remove(i);
			changed = true;
		}

		zone.m_iActiveInfantryCount = 0;
		zone.m_iActiveVehicleCount = 0;
		return changed;
	}

	protected bool UpdateQRF(HST_CampaignState state, HST_EnemyDirectorService enemyDirector)
	{
		bool changed = ResolveArrivedQRFs(state);
		if (!enemyDirector)
			return changed;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone.m_bActive || zone.m_sOwnerFactionKey == "FIA" || zone.m_sQRFRouteId.IsEmpty())
				continue;

			if (IsZoneInsideHQSafeArea(state, zone))
				continue;

			if (state.m_iElapsedSeconds < zone.m_iQrfCooldownUntilSecond)
				continue;

			if (state.FindActiveQRF(zone.m_sZoneId, zone.m_sOwnerFactionKey))
				continue;

			if (!enemyDirector.TrySpend(state, zone.m_sOwnerFactionKey, QRF_ATTACK_RESOURCE_COST, QRF_SUPPORT_RESOURCE_COST))
				continue;

			HST_ActiveGroupState activeGroup = CreateActiveGroup(state, zone, zone.m_sOwnerFactionKey, Math.Min(ResolveActiveInfantryCap(zone), Math.Max(2, zone.m_iGarrisonSlots / 3 + state.m_iWarLevel / 2)), ResolveActiveVehicleCap(zone), true);
			state.m_aActiveGroups.Insert(activeGroup);
			TrySpawnActiveGroup(activeGroup, state);

			HST_QRFState qrf = new HST_QRFState();
			qrf.m_sInstanceId = string.Format("qrf_%1_%2_%3", zone.m_sZoneId, zone.m_sOwnerFactionKey, state.m_iElapsedSeconds);
			qrf.m_sFactionKey = zone.m_sOwnerFactionKey;
			qrf.m_sSourceZoneId = zone.m_sZoneId;
			qrf.m_sTargetZoneId = zone.m_sZoneId;
			qrf.m_sGroupId = activeGroup.m_sGroupId;
			qrf.m_iStartedAtSecond = state.m_iElapsedSeconds;
			qrf.m_iETASeconds = QRF_ETA_SECONDS;
			state.m_aQRFs.Insert(qrf);
			zone.m_iQrfCooldownUntilSecond = state.m_iElapsedSeconds + QRF_COOLDOWN_SECONDS;
			Print(string.Format("h-istasi | dispatched QRF %1 to active zone %2", qrf.m_sInstanceId, zone.m_sZoneId));
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

			qrf.m_bResolved = true;
			qrf.m_bSucceeded = true;
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(qrf.m_sGroupId);
			if (activeGroup)
				activeGroup.m_sRuntimeStatus = "arrived";
			changed = true;
			Print(string.Format("h-istasi | QRF %1 reached zone %2", qrf.m_sInstanceId, qrf.m_sTargetZoneId));
		}

		return changed;
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

			int duration = ResolveRouteDurationSeconds(activeGroup);
			int elapsed = state.m_iElapsedSeconds - activeGroup.m_iSpawnedAtSecond;
			if (elapsed < 0)
				elapsed = 0;

			float progress = Math.Min(1.0, elapsed * 1.0 / duration);
			vector position = LerpPosition(activeGroup.m_vSourcePosition, activeGroup.m_vTargetPosition, progress);
			position = HST_WorldPositionService.ResolveGroundPosition(position, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);
			if (DistanceSq2D(activeGroup.m_vPosition, position) < 25)
			{
				if (progress >= 1.0 && activeGroup.m_sRuntimeStatus != "arrived")
				{
					activeGroup.m_sRuntimeStatus = "arrived";
					changed = true;
				}
				continue;
			}

			activeGroup.m_vPosition = position;
			IEntity entity = GetRuntimeGroupEntity(activeGroup.m_sGroupId);
			if (entity)
				entity.SetOrigin(position);

			if (progress >= 1.0)
				activeGroup.m_sRuntimeStatus = "arrived";

			changed = true;
		}

		return changed;
	}

	protected int ResolveRouteDurationSeconds(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return QRF_ETA_SECONDS;

		if (activeGroup.m_bQRF)
			return QRF_ETA_SECONDS;

		float distance = Math.Sqrt(DistanceSq2D(activeGroup.m_vSourcePosition, activeGroup.m_vTargetPosition));
		return Math.Max(120, Math.Round(distance / 6.0));
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
		activeGroup.m_vPosition = HST_WorldPositionService.ResolveGroundPosition(activeGroup.m_vSourcePosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);
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
		if (qrf && faction.m_aQRFGroupPool.Count() > 0)
			return HST_DefaultCatalog.SelectWeightedPrefab(faction.m_aQRFGroupPool, seed + 701);

		if (!qrf && faction.m_aPatrolGroupPool.Count() > 0)
			return HST_DefaultCatalog.SelectWeightedPrefab(faction.m_aPatrolGroupPool, seed + 503);

		if (faction.m_aGroupPool.Count() > 0)
			return HST_DefaultCatalog.SelectWeightedPrefab(faction.m_aGroupPool, seed + 307);

		if (qrf && faction.m_aQRFGroupPrefabs.Count() > 0)
			return faction.m_aQRFGroupPrefabs[HST_DefaultCatalog.PositiveMod(seed, faction.m_aQRFGroupPrefabs.Count())];

		if (!qrf && faction.m_aPatrolGroupPrefabs.Count() > 0)
			return faction.m_aPatrolGroupPrefabs[HST_DefaultCatalog.PositiveMod(seed, faction.m_aPatrolGroupPrefabs.Count())];

		if (faction.m_aGroupPrefabs.Count() > 0)
			return faction.m_aGroupPrefabs[HST_DefaultCatalog.PositiveMod(seed, faction.m_aGroupPrefabs.Count())];

		return "";
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
		if (!activeGroup || activeGroup.m_sPrefab.IsEmpty() || HasRuntimeGroupEntity(activeGroup.m_sGroupId))
			return false;

		if (activeGroup.m_bSpawnAttempted && !activeGroup.m_bSpawnedEntity)
			return false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
			return false;

		activeGroup.m_bSpawnAttempted = true;
		activeGroup.m_sRuntimeStatus = "spawning";
		GenericEntity entity = respawnSystem.DoSpawn(activeGroup.m_sPrefab, activeGroup.m_vPosition, "0 0 0");
		if (!entity)
		{
			string fallbackPrefab = SelectFallbackGroupPrefab(activeGroup.m_sFactionKey, activeGroup.m_bQRF, activeGroup.m_sGroupId.Length());
			if (!fallbackPrefab.IsEmpty() && fallbackPrefab != activeGroup.m_sPrefab)
			{
				Print(string.Format("h-istasi | active group stock prefab failed for %1; retrying fallback %2", activeGroup.m_sGroupId, fallbackPrefab), LogLevel.WARNING);
				activeGroup.m_sPrefab = fallbackPrefab;
				entity = respawnSystem.DoSpawn(activeGroup.m_sPrefab, activeGroup.m_vPosition, "0 0 0");
			}
		}

		if (!entity)
		{
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			Print(string.Format("h-istasi | active group prefab spawn failed for %1 (%2)", activeGroup.m_sGroupId, activeGroup.m_sPrefab), LogLevel.WARNING);
			return false;
		}

		activeGroup.m_bSpawnedEntity = true;
		activeGroup.m_sRuntimeEntityId = activeGroup.m_sGroupId;
		activeGroup.m_sRuntimeStatus = "routing";
		if (state)
			activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		m_aRuntimeGroupIds.Insert(activeGroup.m_sGroupId);
		m_aRuntimeGroupEntities.Insert(entity);
		return true;
	}

	protected string SelectFallbackGroupPrefab(string factionKey, bool qrf, int seed)
	{
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (!faction)
			return "";

		if (qrf && faction.m_aQRFGroupPrefabs.Count() > 0)
			return faction.m_aQRFGroupPrefabs[HST_DefaultCatalog.PositiveMod(seed, faction.m_aQRFGroupPrefabs.Count())];

		if (!qrf && faction.m_aPatrolGroupPrefabs.Count() > 0)
			return faction.m_aPatrolGroupPrefabs[HST_DefaultCatalog.PositiveMod(seed, faction.m_aPatrolGroupPrefabs.Count())];

		if (faction.m_aGroupPrefabs.Count() > 0)
			return faction.m_aGroupPrefabs[HST_DefaultCatalog.PositiveMod(seed, faction.m_aGroupPrefabs.Count())];

		return "";
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

			TrySpawnActiveGroup(activeGroup, state);
		}
	}

	protected bool HasActiveGarrisonGroup(HST_CampaignState state, HST_ZoneState zone)
	{
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (activeGroup && !activeGroup.m_bQRF && activeGroup.m_sZoneId == zone.m_sZoneId && activeGroup.m_sFactionKey == zone.m_sOwnerFactionKey)
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
			if (!activeGroup || activeGroup.m_bQRF || activeGroup.m_sZoneId != zone.m_sZoneId)
				continue;

			infantryCount += activeGroup.m_iInfantryCount;
			vehicleCount += activeGroup.m_iVehicleCount;
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
		if (survivorInfantry <= 0 && activeGroup.m_iInfantryCount > 0)
			survivorInfantry = activeGroup.m_iInfantryCount;
		if (survivorVehicles <= 0 && activeGroup.m_iVehicleCount > 0)
			survivorVehicles = activeGroup.m_iVehicleCount;

		activeGroup.m_sRuntimeStatus = "folded";
		garrison.m_iInfantryCount += Math.Max(0, survivorInfantry);
		garrison.m_iVehicleCount += Math.Max(0, survivorVehicles);
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
		return m_aRuntimeGroupIds.Find(groupId) >= 0;
	}

	protected IEntity GetRuntimeGroupEntity(string groupId)
	{
		int index = m_aRuntimeGroupIds.Find(groupId);
		if (index < 0 || index >= m_aRuntimeGroupEntities.Count())
			return null;

		return m_aRuntimeGroupEntities[index];
	}

	protected void DeleteRuntimeGroupEntity(string groupId)
	{
		int index = m_aRuntimeGroupIds.Find(groupId);
		if (index < 0)
			return;

		IEntity entity = m_aRuntimeGroupEntities[index];
		if (entity)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);

		m_aRuntimeGroupIds.Remove(index);
		m_aRuntimeGroupEntities.Remove(index);
	}

	protected bool IsZoneInsideHQSafeArea(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone || !state.m_bHQDeployed)
			return false;

		return DistanceSq2D(state.m_vHQPosition, zone.m_vPosition) <= HQ_SAFE_RADIUS_METERS * HQ_SAFE_RADIUS_METERS;
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
