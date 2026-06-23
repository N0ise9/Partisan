class HST_HQService
{
	static const string PETROS_BASE_PREFAB = "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et";
	static const string PETROS_PREFAB = "{6985327711303300}Prefabs/Characters/HST/Character_HST_Petros.et";
	static const string HQ_CACHE_PREFAB = "{AB1A97B1BAE8C395}Prefabs/Compositions/Slotted/SlotFlatSmall/SupplyCache_S_FIA_01.et";
	static const string ARSENAL_PREFAB = "{6985327711303400}Prefabs/Objects/HST/HST_HQArsenal.et";
	static const string HQ_TENT_PREFAB = "{01AE5FD77A9A4C21}Prefabs/Structures/Military/Camps/TentSmallUS_01/TentSmallUS_01.et";
	static const float ARSENAL_POSITION_TOLERANCE_METERS = 4.0;
	static const string CUSTOM_SETUP_HQ_ID = "custom_setup_hq";
	static const float SETUP_ZONE_FALLBACK_RADIUS_METERS = 150.0;

	protected IEntity m_PetrosEntity;
	protected IEntity m_CacheEntity;
	protected IEntity m_ArsenalEntity;
	protected IEntity m_TentEntity;
	protected bool m_bWarnedPetrosResourceFailure;
	protected bool m_bWarnedArsenalResourceFailure;
	protected bool m_bWarnedRuntimeSpawnIncomplete;
	protected bool m_bLoggedPetrosSpawned;
	protected bool m_bLoggedCacheSpawned;
	protected bool m_bLoggedArsenalSpawned;
	protected bool m_bLoggedTentSpawned;
	protected bool m_bDebugLoggingEnabled;

	void SetDebugLoggingEnabled(bool enabled)
	{
		m_bDebugLoggingEnabled = enabled;
	}

	bool BootstrapInitialHideout(HST_CampaignState state, string hideoutId)
	{
		if (!state || state.m_bHQDeployed || !HST_DefaultCatalog.IsKnownHideout(hideoutId))
			return false;

		string resolvedHideoutId;
		vector hqPosition;
		if (!ResolveHideoutPlacement(hideoutId, resolvedHideoutId, hqPosition))
			return false;

		SetHQPosition(state, resolvedHideoutId, hqPosition);
		Print(string.Format("h-istasi | starter HQ bootstrapped at %1 while campaign remains in setup", resolvedHideoutId));
		return true;
	}

	bool SelectInitialHideout(HST_CampaignState state, string hideoutId)
	{
		if (!state || state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_SETUP || !HST_DefaultCatalog.IsKnownHideout(hideoutId))
			return false;

		string resolvedHideoutId;
		vector hqPosition;
		if (!ResolveHideoutPlacement(hideoutId, resolvedHideoutId, hqPosition))
			return false;

		SetHQPosition(state, resolvedHideoutId, hqPosition);
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		return true;
	}

	bool ValidateInitialHQPosition(HST_CampaignState state, vector requestedPosition, out vector resolvedPosition, out string failure)
	{
		resolvedPosition = requestedPosition;
		failure = "";
		DebugLog(string.Format("setup HQ validate requested=%1", requestedPosition));
		if (!state)
		{
			failure = "campaign state not ready";
			DebugLog("setup HQ validate rejected: campaign state not ready");
			return false;
		}

		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_SETUP)
		{
			failure = "campaign setup is already complete";
			DebugLog(string.Format("setup HQ validate rejected: phase=%1", state.m_ePhase));
			return false;
		}

		string zoneFailure;
		if (IsInsideSetupBlockedZone(state, requestedPosition, zoneFailure))
		{
			failure = zoneFailure;
			DebugLog(string.Format("setup HQ validate rejected before ground resolve: %1", failure));
			return false;
		}

		vector requestedGroundPosition;
		if (!HST_WorldPositionService.TryResolveGroundPosition(requestedPosition, HST_WorldPositionService.HQ_GROUND_OFFSET, requestedGroundPosition, true))
		{
			if (HST_WorldPositionService.IsLikelyOpenWater(requestedPosition))
				failure = "selected position is open water";
			else
				failure = "selected position is not dry land";

			DebugLog(string.Format("setup HQ validate rejected: requested point not dry requested=%1 failure=%2", requestedPosition, failure));
			return false;
		}

		if (HST_WorldPositionService.IsLikelyOpenWater(requestedGroundPosition))
		{
			failure = "selected position is open water";
			DebugLog(string.Format("setup HQ validate rejected: requested point open water resolved=%1", requestedGroundPosition));
			return false;
		}

		if (!HST_WorldPositionService.TryResolveSafeGroundPosition(requestedPosition, HST_WorldPositionService.HQ_GROUND_OFFSET, resolvedPosition, true, 6.0))
		{
			failure = "selected position is not dry land";
			DebugLog(string.Format("setup HQ validate rejected: no safe dry ground requested=%1", requestedPosition));
			return false;
		}

		if (HST_WorldPositionService.IsLikelyOpenWater(resolvedPosition))
		{
			failure = "selected position is open water";
			DebugLog(string.Format("setup HQ validate rejected: open water resolved=%1", resolvedPosition));
			return false;
		}

		if (IsInsideSetupBlockedZone(state, resolvedPosition, zoneFailure))
		{
			failure = zoneFailure;
			DebugLog(string.Format("setup HQ validate rejected after ground resolve: %1 resolved=%2", failure, resolvedPosition));
			return false;
		}

		DebugLog(string.Format("setup HQ validate accepted requested=%1 resolved=%2", requestedPosition, resolvedPosition));
		return true;
	}

	bool SelectInitialHQPosition(HST_CampaignState state, vector requestedPosition, out vector resolvedPosition, out string failure)
	{
		if (!ValidateInitialHQPosition(state, requestedPosition, resolvedPosition, failure))
			return false;

		SetHQPosition(state, CUSTOM_SETUP_HQ_ID, resolvedPosition);
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		DebugLog(string.Format("setup HQ selected custom position=%1", resolvedPosition));
		return true;
	}

	void ResetInitialHQSelection(HST_CampaignState state)
	{
		if (!state || state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_SETUP)
			return;

		ClearRuntimeObjects(state);
		state.m_sHQHideoutId = "";
		state.m_vHQPosition = "0 0 0";
		state.m_vPetrosPosition = "0 0 0";
		state.m_vHQCachePosition = "0 0 0";
		state.m_vArsenalPosition = "0 0 0";
		state.m_vHQTentPosition = "0 0 0";
		state.m_bHQDeployed = false;
		state.m_bHQRuntimeObjectsSpawned = false;
		state.m_bPetrosAlive = true;
		state.m_sHQArsenalRuntimeStatus = "setup pending";
		state.m_sLastHQArsenalFailure = "";
		DebugLog("setup HQ selection reset to pending");
	}

	bool MoveHQ(HST_CampaignState state, string hideoutId)
	{
		if (!state || state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE || !state.m_bPetrosAlive || !HST_DefaultCatalog.IsKnownHideout(hideoutId))
			return false;

		string resolvedHideoutId;
		vector hqPosition;
		if (!ResolveHideoutPlacement(hideoutId, resolvedHideoutId, hqPosition))
			return false;

		SetHQPosition(state, resolvedHideoutId, hqPosition);
		return true;
	}

	bool MoveHQToPosition(HST_CampaignState state, vector hqPosition, string hideoutId = "field_hq")
	{
		if (!state || state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE || !state.m_bPetrosAlive)
			return false;

		vector resolvedPosition;
		if (!HST_WorldPositionService.TryResolveSafeGroundPosition(hqPosition, HST_WorldPositionService.HQ_GROUND_OFFSET, resolvedPosition, true, 6.0))
		{
			Print(string.Format("h-istasi | requested HQ move rejected: no dry ground at %1", hqPosition), LogLevel.WARNING);
			return false;
		}

		SetHQPosition(state, hideoutId, resolvedPosition);
		return true;
	}

	void OnPetrosKilled(HST_CampaignState state, HST_EconomyService economy, int moneyPenalty, int hrPenalty)
	{
		if (!state || !state.m_bPetrosAlive)
			return;

		state.m_bPetrosAlive = false;
		state.m_iPetrosDeaths++;
		ClearRuntimeObjects(state);
		economy.AddFactionMoney(state, -moneyPenalty);
		economy.AddHR(state, -hrPenalty);
	}

	bool EnsureRuntimeObjects(HST_CampaignState state)
	{
		if (!state || !state.m_bHQDeployed || !state.m_bPetrosAlive)
			return false;

		if (state.m_bHQRuntimeObjectsSpawned && AreRuntimeObjectsTracked() && IsUsableArsenalEntity(m_ArsenalEntity))
			return false;

		if (state.m_bHQRuntimeObjectsSpawned && !AreRuntimeObjectsTracked())
			state.m_bHQRuntimeObjectsSpawned = false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
			return false;

		EnsureRuntimeGroundPlacement(state);
		bool changed;
		bool logDetails = !m_bWarnedRuntimeSpawnIncomplete;

		if (!m_PetrosEntity)
		{
			m_PetrosEntity = SpawnPetros(respawnSystem, state);
			if (m_PetrosEntity)
			{
				ApplyFaction(m_PetrosEntity);
				m_bLoggedPetrosSpawned = LogRuntimeObjectSpawnSuccess("Petros", ResolvePetrosPrefab(state), state.m_vPetrosPosition, m_bLoggedPetrosSpawned);
				changed = true;
			}
			else if (logDetails)
			{
				LogRuntimeObjectSpawnFailure("Petros", ResolvePetrosPrefab(state), state.m_vPetrosPosition);
			}
		}

		if (!m_CacheEntity)
		{
			m_CacheEntity = HST_WorldPositionService.SpawnPrefab(HQ_CACHE_PREFAB, state.m_vHQCachePosition, "0 0 0");
			if (m_CacheEntity)
			{
				m_bLoggedCacheSpawned = LogRuntimeObjectSpawnSuccess("cache", HQ_CACHE_PREFAB, state.m_vHQCachePosition, m_bLoggedCacheSpawned);
				changed = true;
			}
			else if (logDetails)
				LogRuntimeObjectSpawnFailure("cache", HQ_CACHE_PREFAB, state.m_vHQCachePosition);
		}

		if (!m_ArsenalEntity)
		{
			m_ArsenalEntity = SpawnArsenal(respawnSystem, state);
			if (m_ArsenalEntity)
			{
				m_bLoggedArsenalSpawned = LogRuntimeObjectSpawnSuccess("arsenal", ResolveArsenalPrefab(state), state.m_vArsenalPosition, m_bLoggedArsenalSpawned);
				changed = true;
			}
			else if (logDetails)
				LogRuntimeObjectSpawnFailure("arsenal", ResolveArsenalPrefab(state), state.m_vArsenalPosition);
		}
		else if (!IsUsableArsenalEntity(m_ArsenalEntity))
		{
			string failure = ResolveArsenalReadinessFailure(m_ArsenalEntity, state.m_vArsenalPosition, true);
			Print(string.Format("h-istasi | HQ arsenal runtime entity failed readiness check: %1; deleting only arsenal for retry", failure), LogLevel.WARNING);
			state.m_sLastHQArsenalFailure = failure;
			state.m_sHQArsenalRuntimeStatus = "retrying after readiness failure";
			DeleteRuntimeEntity(m_ArsenalEntity);
			m_ArsenalEntity = null;
			m_bLoggedArsenalSpawned = false;
			changed = true;
		}

		if (!m_TentEntity)
		{
			m_TentEntity = HST_WorldPositionService.SpawnPrefab(HQ_TENT_PREFAB, state.m_vHQTentPosition, "0 0 0");
			if (m_TentEntity)
			{
				m_bLoggedTentSpawned = LogRuntimeObjectSpawnSuccess("tent", HQ_TENT_PREFAB, state.m_vHQTentPosition, m_bLoggedTentSpawned);
				changed = true;
			}
			else if (logDetails)
				LogRuntimeObjectSpawnFailure("tent", HQ_TENT_PREFAB, state.m_vHQTentPosition);
		}

		bool allRuntimeObjectsTracked = AreRuntimeObjectsTracked();
		bool runtimeFlagChanged = state.m_bHQRuntimeObjectsSpawned != allRuntimeObjectsTracked;
		state.m_bHQRuntimeObjectsSpawned = allRuntimeObjectsTracked;
		if (!allRuntimeObjectsTracked)
		{
			if (!m_bWarnedRuntimeSpawnIncomplete)
			{
				Print("h-istasi | HQ runtime object spawn incomplete; successful pieces were preserved for retry", LogLevel.WARNING);
				m_bWarnedRuntimeSpawnIncomplete = true;
			}
		}
		else
		{
			m_bWarnedRuntimeSpawnIncomplete = false;
		}

		return changed || runtimeFlagChanged;
	}

	bool RebuildRuntimeObjects(HST_CampaignState state)
	{
		if (!state || !state.m_bHQDeployed || !state.m_bPetrosAlive)
			return false;

		ClearRuntimeObjects(state);
		EnsureRuntimeGroundPlacement(state);
		state.m_sArsenalPrefab = ARSENAL_PREFAB;
		state.m_sHQArsenalRuntimeStatus = "rebuild requested";
		state.m_sLastHQArsenalFailure = "";
		m_bWarnedRuntimeSpawnIncomplete = false;
		m_bWarnedArsenalResourceFailure = false;
		return EnsureRuntimeObjects(state);
	}

	bool AddHQKnowledge(HST_CampaignState state, int amount, string reason)
	{
		if (!state || amount <= 0)
			return false;

		int before = state.m_iHQKnowledge;
		state.m_iHQKnowledge = Math.Max(0, Math.Min(100, state.m_iHQKnowledge + amount));
		state.m_iHQThreatLevel = Math.Max(state.m_iHQThreatLevel, state.m_iHQKnowledge);
		state.m_iHQKnowledgeLastChangedSecond = state.m_iElapsedSeconds;
		state.m_sLastHQKnowledgeReason = reason;
		return state.m_iHQKnowledge != before;
	}

	bool ReduceHQKnowledge(HST_CampaignState state, int amount, string reason)
	{
		if (!state || amount <= 0)
			return false;

		int before = state.m_iHQKnowledge;
		state.m_iHQKnowledge = Math.Max(0, state.m_iHQKnowledge - amount);
		state.m_iHQThreatLevel = Math.Max(0, Math.Min(state.m_iHQThreatLevel, state.m_iHQKnowledge));
		state.m_iHQKnowledgeLastChangedSecond = state.m_iElapsedSeconds;
		state.m_sLastHQKnowledgeReason = reason;
		return state.m_iHQKnowledge != before;
	}

	bool TickHQThreat(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !state.m_bHQDeployed || !state.m_bPetrosAlive)
			return false;
		if (state.m_iElapsedSeconds < state.m_iLastHQThreatScanSecond + 30)
			return false;

		state.m_iLastHQThreatScanSecond = state.m_iElapsedSeconds;

		string enemyReason;
		string aggressionReason;
		string civilianReason;
		int threat;
		threat += ResolveEnemyActivityThreatNearHQ(state, preset, enemyReason);
		threat += ResolveAggressionThreat(state, preset, aggressionReason);
		threat += ResolveCivilianHeatThreatNearHQ(state, civilianReason);

		string reason = AppendThreatReason("", enemyReason);
		reason = AppendThreatReason(reason, aggressionReason);
		reason = AppendThreatReason(reason, civilianReason);
		if (reason.IsEmpty())
			reason = "quiet";

		int beforeThreat = state.m_iHQThreatLevel;
		state.m_iHQThreatLevel = Math.Max(0, Math.Min(100, threat));
		state.m_sLastHQThreatReason = reason;
		state.m_iLastHQActivitySecond = state.m_iElapsedSeconds;

		bool knowledgeChanged;
		if (state.m_iHQThreatLevel >= 50)
			knowledgeChanged = AddHQKnowledge(state, Math.Max(1, state.m_iHQThreatLevel / 20), "HQ threat scan: " + reason);

		return knowledgeChanged || state.m_iHQThreatLevel != beforeThreat;
	}

	string BuildHQThreatReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi HQ threat | state not ready";

		string report = string.Format(
			"h-istasi HQ threat | knowledge %1/100 | threat %2 | last knowledge %3s | reason %4 | last scan %5s | scan reason %6 | defend active %7 | status %8 | mission %9",
			state.m_iHQKnowledge,
			state.m_iHQThreatLevel,
			state.m_iHQKnowledgeLastChangedSecond,
			state.m_sLastHQKnowledgeReason,
			state.m_iLastHQThreatScanSecond,
			state.m_sLastHQThreatReason,
			state.m_bDefendPetrosActive,
			state.m_sDefendPetrosStatus,
			state.m_sDefendPetrosMissionId
		);
		report = report + string.Format(" | order %1 | support %2 | group %3", state.m_sDefendPetrosOrderId, state.m_sDefendPetrosSupportRequestId, state.m_sDefendPetrosAttackerGroupId);
		report = report + string.Format(" | attackers %1 alive %2 killed %3 | Petros alive %4 deaths %5", state.m_iDefendPetrosAttackerCount, state.m_iDefendPetrosAliveAttackerCount, state.m_iDefendPetrosKilledCount, state.m_bPetrosAlive, state.m_iPetrosDeaths);
		return report;
	}
	string GetPetrosPrefab()
	{
		return PETROS_PREFAB;
	}

	string GetArsenalPrefab()
	{
		return ARSENAL_PREFAB;
	}

	protected int ResolveEnemyActivityThreatNearHQ(HST_CampaignState state, HST_CampaignPreset preset, out string reason)
	{
		reason = "no enemy activity near HQ";
		if (!state || !preset || !state.m_bHQDeployed)
			return 0;

		int threat;
		float scanRadiusSq = 1200 * 1200;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (!group)
				continue;
			if (group.m_sFactionKey == preset.m_sResistanceFactionKey || group.m_sFactionKey == "CIV")
				continue;
			if (group.m_sRuntimeStatus == "eliminated" || group.m_sRuntimeStatus == "folded" || group.m_sRuntimeStatus == "spawn_failed")
				continue;
			if (DistanceSq2D(group.m_vPosition, state.m_vHQPosition) > scanRadiusSq)
				continue;

			threat += 20 + Math.Max(0, group.m_iSurvivorInfantryCount) * 2 + Math.Max(0, group.m_iSurvivorVehicleCount) * 8;
			reason = string.Format("enemy active group %1 near HQ", group.m_sGroupId);
		}

		return Math.Min(60, threat);
	}

	protected int ResolveAggressionThreat(HST_CampaignState state, HST_CampaignPreset preset, out string reason)
	{
		int threat;
		reason = "aggression quiet";
		if (!state)
			return 0;

		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || (preset && pool.m_sFactionKey == preset.m_sResistanceFactionKey))
				continue;
			if (pool.m_iAggression <= 0)
				continue;

			threat += Math.Min(25, pool.m_iAggression / 4);
			reason = string.Format("%1 aggression %2", pool.m_sFactionKey, pool.m_iAggression);
		}

		return threat;
	}

	protected int ResolveCivilianHeatThreatNearHQ(HST_CampaignState state, out string reason)
	{
		reason = "civilian heat quiet";
		if (!state)
			return 0;

		int threat;
		foreach (HST_CivilianZoneState town : state.m_aCivilianZones)
		{
			if (!town || town.m_iWantedHeat <= 0)
				continue;

			HST_ZoneState zone = state.FindZone(town.m_sZoneId);
			if (!zone)
				continue;
			if (DistanceSq2D(zone.m_vPosition, state.m_vHQPosition) > 1500 * 1500)
				continue;

			threat += Math.Min(15, town.m_iWantedHeat * 3);
			reason = string.Format("town heat near HQ at %1", town.m_sZoneId);
		}

		return threat;
	}

	protected string AppendThreatReason(string current, string reason)
	{
		if (reason.IsEmpty() || reason == "no enemy activity near HQ" || reason == "aggression quiet" || reason == "civilian heat quiet")
			return current;
		if (current.IsEmpty())
			return reason;
		return current + "; " + reason;
	}
	protected bool ResolveHideoutPlacement(string requestedHideoutId, out string resolvedHideoutId, out vector resolvedPosition)
	{
		resolvedHideoutId = "";
		resolvedPosition = "0 0 0";

		foreach (HST_HideoutDefinition preferredHideout : HST_DefaultCatalog.CreateHideouts())
		{
			if (preferredHideout.m_sHideoutId != requestedHideoutId)
				continue;

			if (TryResolveHideout(preferredHideout, resolvedHideoutId, resolvedPosition))
				return true;
		}

		foreach (HST_HideoutDefinition fallbackHideout : HST_DefaultCatalog.CreateHideouts())
		{
			if (fallbackHideout.m_sHideoutId == requestedHideoutId)
				continue;

			if (TryResolveHideout(fallbackHideout, resolvedHideoutId, resolvedPosition))
			{
				Print(string.Format("h-istasi | requested HQ hideout %1 was not dry/valid; using %2", requestedHideoutId, resolvedHideoutId), LogLevel.WARNING);
				return true;
			}
		}

		vector emergencyPosition = HST_DefaultCatalog.GetEmergencySpawnPosition();
		if (HST_WorldPositionService.TryResolveSafeGroundPosition(emergencyPosition, HST_WorldPositionService.HQ_GROUND_OFFSET, resolvedPosition, true, 6.0))
		{
			resolvedHideoutId = "hideout_emergency";
			Print(string.Format("h-istasi | no dry HQ hideout surface found for %1; using emergency dry HQ position %2", requestedHideoutId, resolvedPosition), LogLevel.WARNING);
			return true;
		}

		emergencyPosition[1] = emergencyPosition[1] + HST_WorldPositionService.HQ_GROUND_OFFSET;
		resolvedHideoutId = "hideout_emergency";
		resolvedPosition = emergencyPosition;
		Print(string.Format("h-istasi | no dry HQ hideout surface found for %1; using positive emergency HQ position %2", requestedHideoutId, resolvedPosition), LogLevel.WARNING);
		return true;
	}

	protected bool TryResolveHideout(HST_HideoutDefinition hideout, out string resolvedHideoutId, out vector resolvedPosition)
	{
		resolvedHideoutId = "";
		resolvedPosition = "0 0 0";
		if (!hideout)
			return false;

		if (!HST_WorldPositionService.TryResolveSafeGroundPosition(hideout.m_vPosition, HST_WorldPositionService.HQ_GROUND_OFFSET, resolvedPosition, true, 6.0))
		{
			Print(string.Format("h-istasi | HQ hideout %1 rejected: no dry ground at %2", hideout.m_sHideoutId, hideout.m_vPosition), LogLevel.WARNING);
			return false;
		}

		resolvedHideoutId = hideout.m_sHideoutId;
		return true;
	}

	protected bool IsInsideSetupBlockedZone(HST_CampaignState state, vector position, out string failure)
	{
		failure = "";
		if (!state)
			return false;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			float radius = ResolveSetupBlockedZoneRadius(zone);
			if (DistanceSq2D(position, zone.m_vPosition) > radius * radius)
				continue;

			string label = zone.m_sDisplayName;
			if (label.IsEmpty())
				label = zone.m_sZoneId;
			failure = "selected position is inside " + label;
			DebugLog(string.Format("setup HQ zone block position=%1 zone=%2 center=%3 radius=%4", position, zone.m_sZoneId, zone.m_vPosition, radius));
			return true;
		}

		return false;
	}

	protected float ResolveSetupBlockedZoneRadius(HST_ZoneState zone)
	{
		if (zone && zone.m_iCaptureRadiusMeters > 0)
			return zone.m_iCaptureRadiusMeters;

		return SETUP_ZONE_FALLBACK_RADIUS_METERS;
	}

	protected void SetHQPosition(HST_CampaignState state, string hideoutId, vector hqPosition)
	{
		ClearRuntimeObjects(state);
		vector petrosOffset = "2 0 2";
		vector cacheOffset = "4 0 -2";
		vector tentOffset = "-4 0 2";
		state.m_sHQHideoutId = hideoutId;
		state.m_vHQPosition = hqPosition;
		state.m_vPetrosPosition = ResolveHQObjectPosition(hqPosition, petrosOffset, HST_WorldPositionService.CHARACTER_GROUND_OFFSET);
		state.m_vHQCachePosition = ResolveHQObjectPosition(hqPosition, cacheOffset, HST_WorldPositionService.PROP_GROUND_OFFSET);
		state.m_vArsenalPosition = ResolvePrimaryArsenalPosition(state);
		state.m_vHQTentPosition = ResolveHQObjectPosition(hqPosition, tentOffset, HST_WorldPositionService.PROP_GROUND_OFFSET);
		state.m_bHQDeployed = true;
		state.m_bHQRuntimeObjectsSpawned = false;
		state.m_bPetrosAlive = true;
		state.m_sPetrosPrefab = PETROS_PREFAB;
		state.m_sHQCachePrefab = HQ_CACHE_PREFAB;
		state.m_sArsenalPrefab = ARSENAL_PREFAB;
		state.m_sHQTentPrefab = HQ_TENT_PREFAB;
		state.m_sHQArsenalRuntimeStatus = "pending spawn";
		state.m_sLastHQArsenalFailure = "";
		Print(string.Format("h-istasi | HQ %1 placed at %2; Petros %3 cache %4 arsenal %5 tent %6", hideoutId, state.m_vHQPosition, state.m_vPetrosPosition, state.m_vHQCachePosition, state.m_vArsenalPosition, state.m_vHQTentPosition));
		DebugLog(string.Format("HQ position set hideout=%1 hq=%2 petros=%3 cache=%4 arsenal=%5 tent=%6", hideoutId, state.m_vHQPosition, state.m_vPetrosPosition, state.m_vHQCachePosition, state.m_vArsenalPosition, state.m_vHQTentPosition));
	}

	protected void DebugLog(string message)
	{
		if (!m_bDebugLoggingEnabled)
			return;

		Print("h-istasi HQ debug | " + message);
	}

	protected void EnsureRuntimeGroundPlacement(HST_CampaignState state)
	{
		// TryResolveGroundPosition remains the underlying dry-ground contract through TryResolveSafeGroundPosition.
		vector resolvedHQ;
		if (HST_WorldPositionService.TryResolveSafeGroundPosition(state.m_vHQPosition, HST_WorldPositionService.HQ_GROUND_OFFSET, resolvedHQ, true, 6.0))
		{
			state.m_vHQPosition = resolvedHQ;
		}
		else
		{
			vector fallbackHQ = HST_DefaultCatalog.GetHideoutPosition(HST_DefaultCatalog.GetDefaultHideoutId());
			if (HST_WorldPositionService.TryResolveSafeGroundPosition(fallbackHQ, HST_WorldPositionService.HQ_GROUND_OFFSET, resolvedHQ, true, 6.0))
			{
				Print(string.Format("h-istasi | restored HQ position %1 was not dry; re-seating HQ at default hideout %2", state.m_vHQPosition, resolvedHQ), LogLevel.WARNING);
				state.m_vHQPosition = resolvedHQ;
			}
			else
			{
				vector emergencyHQ = HST_DefaultCatalog.GetEmergencySpawnPosition();
				if (HST_WorldPositionService.TryResolveSafeGroundPosition(emergencyHQ, HST_WorldPositionService.HQ_GROUND_OFFSET, resolvedHQ, true, 6.0))
				{
					Print(string.Format("h-istasi | restored HQ position %1 and default hideout were not dry; re-seating HQ at emergency position %2", state.m_vHQPosition, resolvedHQ), LogLevel.WARNING);
					state.m_vHQPosition = resolvedHQ;
				}
				else
				{
					emergencyHQ[1] = emergencyHQ[1] + HST_WorldPositionService.HQ_GROUND_OFFSET;
					Print(string.Format("h-istasi | restored HQ position %1 and default hideout were not dry; using positive emergency HQ position %2", state.m_vHQPosition, emergencyHQ), LogLevel.WARNING);
					state.m_vHQPosition = emergencyHQ;
				}
			}
		}

		state.m_vPetrosPosition = ResolveRuntimeObjectGroundPosition(state.m_vPetrosPosition, state.m_vHQPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET);
		state.m_vHQCachePosition = ResolveRuntimeObjectGroundPosition(state.m_vHQCachePosition, state.m_vHQPosition, HST_WorldPositionService.PROP_GROUND_OFFSET);
		state.m_vArsenalPosition = ResolveRuntimeObjectGroundPosition(state.m_vArsenalPosition, state.m_vHQPosition, HST_WorldPositionService.PROP_GROUND_OFFSET);
		state.m_vHQTentPosition = ResolveRuntimeObjectGroundPosition(state.m_vHQTentPosition, state.m_vHQPosition, HST_WorldPositionService.PROP_GROUND_OFFSET);
	}

	protected vector ResolveRuntimeObjectGroundPosition(vector source, vector fallback, float verticalOffset)
	{
		vector resolved;
		if (HST_WorldPositionService.TryResolveSafeGroundPosition(source, verticalOffset, resolved, true, 2.0))
			return resolved;

		if (HST_WorldPositionService.TryResolveSafeGroundPosition(fallback, verticalOffset, resolved, true, 2.0))
			return resolved;

		vector emergencyPosition = HST_DefaultCatalog.GetEmergencySpawnPosition();
		if (HST_WorldPositionService.TryResolveSafeGroundPosition(emergencyPosition, verticalOffset, resolved, true, 2.0))
			return resolved;

		emergencyPosition[1] = emergencyPosition[1] + verticalOffset;
		return emergencyPosition;
	}

	protected string ResolvePetrosPrefab(HST_CampaignState state)
	{
		if (!state)
			return PETROS_BASE_PREFAB;

		if (state.m_sPetrosPrefab.IsEmpty() || state.m_sPetrosPrefab == PETROS_BASE_PREFAB)
			state.m_sPetrosPrefab = PETROS_PREFAB;

		return state.m_sPetrosPrefab;
	}

	protected GenericEntity SpawnPetros(SCR_RespawnSystemComponent respawnSystem, HST_CampaignState state)
	{
		if (!respawnSystem)
			return null;

		string petrosPrefab = ResolvePetrosPrefab(state);
		vector petrosPosition = "0 0 0";
		if (state)
			petrosPosition = state.m_vPetrosPosition;

		GenericEntity petros = HST_WorldPositionService.SpawnPrefab(petrosPrefab, petrosPosition, "0 0 0");
		if (petros)
			return petros;

		if (petrosPrefab != PETROS_BASE_PREFAB)
		{
			if (!m_bWarnedPetrosResourceFailure)
			{
				Print(string.Format("h-istasi | dedicated Petros prefab %1 failed to spawn; using base FIA fallback", petrosPrefab), LogLevel.WARNING);
				m_bWarnedPetrosResourceFailure = true;
			}

			return HST_WorldPositionService.SpawnPrefab(PETROS_BASE_PREFAB, petrosPosition, "0 0 0");
		}

		return null;
	}

	protected string ResolveArsenalPrefab(HST_CampaignState state)
	{
		if (!state)
			return ARSENAL_PREFAB;

		if (state.m_sArsenalPrefab.IsEmpty() || state.m_sArsenalPrefab != ARSENAL_PREFAB)
			state.m_sArsenalPrefab = ARSENAL_PREFAB;

		return state.m_sArsenalPrefab;
	}

	protected GenericEntity SpawnArsenal(SCR_RespawnSystemComponent respawnSystem, HST_CampaignState state)
	{
		if (!respawnSystem)
			return null;

		string arsenalPrefab = ResolveArsenalPrefab(state);
		vector arsenalPosition = ResolvePrimaryArsenalPosition(state);

		GenericEntity arsenal = HST_WorldPositionService.SpawnPrefab(arsenalPrefab, arsenalPosition, "0 0 0");
		string failure = ResolveArsenalReadinessFailure(arsenal, arsenalPosition, true);
		if (failure.IsEmpty())
		{
			if (state)
			{
				state.m_vArsenalPosition = arsenalPosition;
				state.m_sArsenalPrefab = ARSENAL_PREFAB;
				state.m_sHQArsenalRuntimeStatus = string.Format("visible/action surface ready at %1 using %2", arsenalPosition, arsenalPrefab);
				state.m_sLastHQArsenalFailure = "";
			}

			Print(string.Format("h-istasi | HQ arsenal visual/action surface ready at %1 using %2; item authority remains h-istasi services", arsenalPosition, arsenalPrefab));
			return arsenal;
		}

		if (arsenal)
		{
			Print(string.Format("h-istasi | HQ arsenal prefab %1 spawned at %2 but failed readiness check: %3", arsenalPrefab, arsenalPosition, failure), LogLevel.WARNING);
			DeleteRuntimeEntity(arsenal);
		}

		if (!m_bWarnedArsenalResourceFailure)
		{
			Print(string.Format("h-istasi | HQ arsenal prefab %1 failed to spawn ready at %2 | %3", arsenalPrefab, arsenalPosition, failure), LogLevel.WARNING);
			m_bWarnedArsenalResourceFailure = true;
		}

		if (state)
		{
			state.m_sHQArsenalRuntimeStatus = "failed";
			state.m_sLastHQArsenalFailure = failure;
		}

		return null;
	}

	protected bool IsUsableArsenalEntity(IEntity arsenal)
	{
		string failure = ResolveArsenalReadinessFailure(arsenal, "0 0 0", false);
		return failure.IsEmpty();
	}

	protected string ResolveArsenalReadinessFailure(IEntity arsenal, vector intendedPosition, bool checkPosition)
	{
		if (!arsenal)
			return "entity missing after spawn";

		if (checkPosition)
		{
			vector origin = arsenal.GetOrigin();
			if (DistanceSq2D(origin, intendedPosition) > ARSENAL_POSITION_TOLERANCE_METERS * ARSENAL_POSITION_TOLERANCE_METERS)
				return string.Format("moved away from intended position %1 to %2", intendedPosition, origin);

			float heightDelta = origin[1] - intendedPosition[1];
			if (heightDelta < -1.0)
				return string.Format("below intended visible height %1 at %2", intendedPosition, origin);
		}

		return "";
	}

	protected vector ResolvePrimaryArsenalPosition(HST_CampaignState state)
	{
		if (!state)
			return "0 0 0";

		vector arsenalOffset = "3 0 4";
		return ResolveHQObjectPosition(state.m_vHQPosition, arsenalOffset, HST_WorldPositionService.PROP_GROUND_OFFSET);
	}

	protected vector ResolveHQObjectPosition(vector hqPosition, vector offset, float verticalOffset)
	{
		vector source = hqPosition + offset;
		vector resolved;
		if (HST_WorldPositionService.TryResolveSafeGroundPosition(source, verticalOffset, resolved, true, 2.0))
			return resolved;

		if (HST_WorldPositionService.TryResolveSafeGroundPosition(hqPosition, verticalOffset, resolved, true, 2.0))
			return resolved;

		source[1] = hqPosition[1] + verticalOffset;
		return source;
	}

	protected void ApplyFaction(IEntity entity)
	{
		if (!entity)
			return;

		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (factionComponent)
			factionComponent.SetAffiliatedFactionByKey("FIA");
	}

	protected bool AreRuntimeObjectsTracked()
	{
		return m_PetrosEntity && m_CacheEntity && m_ArsenalEntity && m_TentEntity;
	}

	protected float DistanceSq2D(vector first, vector second)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz;
	}

	protected void LogRuntimeObjectSpawnFailure(string label, string prefab, vector position)
	{
		Print(string.Format("h-istasi | HQ %1 spawn failed at %2 using %3", label, position, prefab), LogLevel.WARNING);
	}

	protected bool LogRuntimeObjectSpawnSuccess(string label, string prefab, vector position, bool alreadyLogged)
	{
		if (alreadyLogged)
			return true;

		Print(string.Format("h-istasi | HQ %1 spawned at %2 using %3", label, position, prefab));
		return true;
	}

	protected void ClearRuntimeObjects(HST_CampaignState state)
	{
		DeleteRuntimeEntity(m_PetrosEntity);
		DeleteRuntimeEntity(m_CacheEntity);
		DeleteRuntimeEntity(m_ArsenalEntity);
		DeleteRuntimeEntity(m_TentEntity);

		m_PetrosEntity = null;
		m_CacheEntity = null;
		m_ArsenalEntity = null;
		m_TentEntity = null;
		m_bLoggedPetrosSpawned = false;
		m_bLoggedCacheSpawned = false;
		m_bLoggedArsenalSpawned = false;
		m_bLoggedTentSpawned = false;

		if (state)
		{
			state.m_bHQRuntimeObjectsSpawned = false;
			state.m_sHQArsenalRuntimeStatus = "cleared";
		}
	}

	protected void DeleteRuntimeEntity(IEntity entity)
	{
		if (entity)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}
}
