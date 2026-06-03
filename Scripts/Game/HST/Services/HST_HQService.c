class HST_HQService
{
	static const string PETROS_PREFAB = "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et";
	static const string HQ_CACHE_PREFAB = "{AB1A97B1BAE8C395}Prefabs/Compositions/Slotted/SlotFlatSmall/SupplyCache_S_FIA_01.et";
	static const string HQ_TENT_PREFAB = "{01AE5FD77A9A4C21}Prefabs/Structures/Military/Camps/TentSmallUS_01/TentSmallUS_01.et";

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

	void OnPetrosKilled(HST_CampaignState state, HST_EconomyService economy, int moneyPenalty, int hrPenalty)
	{
		if (!state || !state.m_bPetrosAlive)
			return;

		state.m_bPetrosAlive = false;
		state.m_iPetrosDeaths++;
		state.m_bHQRuntimeObjectsSpawned = false;
		economy.AddFactionMoney(state, -moneyPenalty);
		economy.AddHR(state, -hrPenalty);
	}

	bool EnsureRuntimeObjects(HST_CampaignState state)
	{
		if (!state || !state.m_bHQDeployed || state.m_bHQRuntimeObjectsSpawned || !state.m_bPetrosAlive)
			return false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
			return false;

		EnsureRuntimeGroundPlacement(state);
		GenericEntity petros = respawnSystem.DoSpawn(PETROS_PREFAB, state.m_vPetrosPosition, "0 0 0");
		GenericEntity cache = respawnSystem.DoSpawn(HQ_CACHE_PREFAB, state.m_vHQCachePosition, "0 0 0");
		GenericEntity tent = respawnSystem.DoSpawn(HQ_TENT_PREFAB, state.m_vHQTentPosition, "0 0 0");
		state.m_bHQRuntimeObjectsSpawned = true;
		if (!petros || !cache || !tent)
		{
			Print("h-istasi | HQ runtime object spawn incomplete; inspect placeholder prefab resources", LogLevel.WARNING);
			return true;
		}

		ApplyFaction(petros);
		return true;
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

		vector emergencyPosition = HST_DefaultCatalog.GetHideoutPosition(requestedHideoutId);
		resolvedPosition = HST_WorldPositionService.ResolveGroundPosition(emergencyPosition, HST_WorldPositionService.HQ_GROUND_OFFSET, false);
		resolvedHideoutId = requestedHideoutId;
		Print(string.Format("h-istasi | no dry HQ hideout surface found; using emergency snapped position %1", resolvedPosition), LogLevel.ERROR);
		return true;
	}

	protected bool TryResolveHideout(HST_HideoutDefinition hideout, out string resolvedHideoutId, out vector resolvedPosition)
	{
		resolvedHideoutId = "";
		resolvedPosition = "0 0 0";
		if (!hideout)
			return false;

		if (!HST_WorldPositionService.TryResolveGroundPosition(hideout.m_vPosition, HST_WorldPositionService.HQ_GROUND_OFFSET, resolvedPosition, true))
		{
			Print(string.Format("h-istasi | HQ hideout %1 rejected: no dry ground at %2", hideout.m_sHideoutId, hideout.m_vPosition), LogLevel.WARNING);
			return false;
		}

		resolvedHideoutId = hideout.m_sHideoutId;
		return true;
	}

	protected void SetHQPosition(HST_CampaignState state, string hideoutId, vector hqPosition)
	{
		vector petrosOffset = "2 0 2";
		vector cacheOffset = "4 0 -2";
		vector tentOffset = "-4 0 2";
		state.m_sHQHideoutId = hideoutId;
		state.m_vHQPosition = hqPosition;
		state.m_vPetrosPosition = ResolveHQObjectPosition(hqPosition, petrosOffset, HST_WorldPositionService.CHARACTER_GROUND_OFFSET);
		state.m_vHQCachePosition = ResolveHQObjectPosition(hqPosition, cacheOffset, HST_WorldPositionService.PROP_GROUND_OFFSET);
		state.m_vHQTentPosition = ResolveHQObjectPosition(hqPosition, tentOffset, HST_WorldPositionService.PROP_GROUND_OFFSET);
		state.m_bHQDeployed = true;
		state.m_bHQRuntimeObjectsSpawned = false;
		state.m_bPetrosAlive = true;
		state.m_sPetrosPrefab = PETROS_PREFAB;
		state.m_sHQCachePrefab = HQ_CACHE_PREFAB;
		state.m_sHQTentPrefab = HQ_TENT_PREFAB;
		Print(string.Format("h-istasi | HQ %1 placed at %2; Petros %3 cache %4 tent %5", hideoutId, state.m_vHQPosition, state.m_vPetrosPosition, state.m_vHQCachePosition, state.m_vHQTentPosition));
	}

	protected void EnsureRuntimeGroundPlacement(HST_CampaignState state)
	{
		state.m_vHQPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vHQPosition, HST_WorldPositionService.HQ_GROUND_OFFSET, false);
		state.m_vPetrosPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vPetrosPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);
		state.m_vHQCachePosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vHQCachePosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		state.m_vHQTentPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vHQTentPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
	}

	protected vector ResolveHQObjectPosition(vector hqPosition, vector offset, float verticalOffset)
	{
		vector source = hqPosition + offset;
		vector resolved;
		if (HST_WorldPositionService.TryResolveGroundPosition(source, verticalOffset, resolved, false))
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
}
