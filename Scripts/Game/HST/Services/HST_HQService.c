class HST_HQService
{
	static const string PETROS_BASE_PREFAB = "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et";
	static const string PETROS_PREFAB = "{6985327711303300}Prefabs/Characters/HST/Character_HST_Petros.et";
	static const string HQ_CACHE_PREFAB = "{AB1A97B1BAE8C395}Prefabs/Compositions/Slotted/SlotFlatSmall/SupplyCache_S_FIA_01.et";
	static const string ARSENAL_PREFAB = "{6985327711303400}Prefabs/Objects/HST/HST_HQArsenal.et";
	static const string ARSENAL_FALLBACK_PREFAB = "{AB1A97B1BAE8C395}Prefabs/Compositions/Slotted/SlotFlatSmall/SupplyCache_S_FIA_01.et";
	static const string HQ_TENT_PREFAB = "{01AE5FD77A9A4C21}Prefabs/Structures/Military/Camps/TentSmallUS_01/TentSmallUS_01.et";

	protected IEntity m_PetrosEntity;
	protected IEntity m_CacheEntity;
	protected IEntity m_ArsenalEntity;
	protected IEntity m_TentEntity;
	protected bool m_bWarnedPetrosResourceFailure;
	protected bool m_bWarnedArsenalResourceFailure;
	protected bool m_bWarnedRuntimeSpawnIncomplete;

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

	bool MoveHQToPosition(HST_CampaignState state, vector hqPosition, string hideoutId = "field_hq")
	{
		if (!state || state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE || !state.m_bPetrosAlive)
			return false;

		vector resolvedPosition = HST_WorldPositionService.ResolveGroundPosition(hqPosition, HST_WorldPositionService.HQ_GROUND_OFFSET, true);
		if (!HST_WorldPositionService.IsDryGroundPosition(resolvedPosition))
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

		if (state.m_bHQRuntimeObjectsSpawned && AreRuntimeObjectsTracked())
			return false;

		if (state.m_bHQRuntimeObjectsSpawned && !AreRuntimeObjectsTracked())
			state.m_bHQRuntimeObjectsSpawned = false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
			return false;

		EnsureRuntimeGroundPlacement(state);
		GenericEntity petros = SpawnPetros(respawnSystem, state);
		GenericEntity cache = respawnSystem.DoSpawn(HQ_CACHE_PREFAB, state.m_vHQCachePosition, "0 0 0");
		GenericEntity arsenal = SpawnArsenal(respawnSystem, state);
		GenericEntity tent = respawnSystem.DoSpawn(HQ_TENT_PREFAB, state.m_vHQTentPosition, "0 0 0");
		if (!petros || !cache || !arsenal || !tent)
		{
			DeleteRuntimeEntity(petros);
			DeleteRuntimeEntity(cache);
			DeleteRuntimeEntity(arsenal);
			DeleteRuntimeEntity(tent);
			state.m_bHQRuntimeObjectsSpawned = false;
			if (!m_bWarnedRuntimeSpawnIncomplete)
			{
				Print("h-istasi | HQ runtime object spawn incomplete; inspect prefab resources", LogLevel.WARNING);
				m_bWarnedRuntimeSpawnIncomplete = true;
			}

			return false;
		}

		m_PetrosEntity = petros;
		m_CacheEntity = cache;
		m_ArsenalEntity = arsenal;
		m_TentEntity = tent;
		state.m_bHQRuntimeObjectsSpawned = true;
		m_bWarnedRuntimeSpawnIncomplete = false;
		ApplyFaction(petros);
		return true;
	}

	string GetPetrosPrefab()
	{
		return PETROS_PREFAB;
	}

	string GetArsenalPrefab()
	{
		return ARSENAL_PREFAB;
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
		Print(string.Format("h-istasi | no dry HQ hideout surface found; using emergency snapped position %1", resolvedPosition), LogLevel.WARNING);
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
		ClearRuntimeObjects(state);
		vector petrosOffset = "2 0 2";
		vector cacheOffset = "4 0 -2";
		vector arsenalOffset = "-2 0 -4";
		vector tentOffset = "-4 0 2";
		state.m_sHQHideoutId = hideoutId;
		state.m_vHQPosition = hqPosition;
		state.m_vPetrosPosition = ResolveHQObjectPosition(hqPosition, petrosOffset, HST_WorldPositionService.CHARACTER_GROUND_OFFSET);
		state.m_vHQCachePosition = ResolveHQObjectPosition(hqPosition, cacheOffset, HST_WorldPositionService.PROP_GROUND_OFFSET);
		state.m_vArsenalPosition = ResolveHQObjectPosition(hqPosition, arsenalOffset, HST_WorldPositionService.PROP_GROUND_OFFSET);
		state.m_vHQTentPosition = ResolveHQObjectPosition(hqPosition, tentOffset, HST_WorldPositionService.PROP_GROUND_OFFSET);
		state.m_bHQDeployed = true;
		state.m_bHQRuntimeObjectsSpawned = false;
		state.m_bPetrosAlive = true;
		state.m_sPetrosPrefab = PETROS_PREFAB;
		state.m_sHQCachePrefab = HQ_CACHE_PREFAB;
		state.m_sArsenalPrefab = ARSENAL_PREFAB;
		state.m_sHQTentPrefab = HQ_TENT_PREFAB;
		Print(string.Format("h-istasi | HQ %1 placed at %2; Petros %3 cache %4 arsenal %5 tent %6", hideoutId, state.m_vHQPosition, state.m_vPetrosPosition, state.m_vHQCachePosition, state.m_vArsenalPosition, state.m_vHQTentPosition));
	}

	protected void EnsureRuntimeGroundPlacement(HST_CampaignState state)
	{
		state.m_vHQPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vHQPosition, HST_WorldPositionService.HQ_GROUND_OFFSET, false);
		state.m_vPetrosPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vPetrosPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);
		state.m_vHQCachePosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vHQCachePosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		state.m_vArsenalPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vArsenalPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		state.m_vHQTentPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vHQTentPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
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

		GenericEntity petros = respawnSystem.DoSpawn(petrosPrefab, petrosPosition, "0 0 0");
		if (petros)
			return petros;

		if (petrosPrefab != PETROS_BASE_PREFAB)
		{
			if (!m_bWarnedPetrosResourceFailure)
			{
				Print(string.Format("h-istasi | dedicated Petros prefab %1 failed to spawn; using base FIA fallback", petrosPrefab), LogLevel.WARNING);
				m_bWarnedPetrosResourceFailure = true;
			}

			return respawnSystem.DoSpawn(PETROS_BASE_PREFAB, petrosPosition, "0 0 0");
		}

		return null;
	}

	protected string ResolveArsenalPrefab(HST_CampaignState state)
	{
		if (!state)
			return ARSENAL_PREFAB;

		if (state.m_sArsenalPrefab.IsEmpty() || state.m_sArsenalPrefab == ARSENAL_FALLBACK_PREFAB)
			state.m_sArsenalPrefab = ARSENAL_PREFAB;

		return state.m_sArsenalPrefab;
	}

	protected GenericEntity SpawnArsenal(SCR_RespawnSystemComponent respawnSystem, HST_CampaignState state)
	{
		if (!respawnSystem)
			return null;

		string arsenalPrefab = ResolveArsenalPrefab(state);
		vector arsenalPosition = "0 0 0";
		if (state)
			arsenalPosition = state.m_vArsenalPosition;

		GenericEntity arsenal = respawnSystem.DoSpawn(arsenalPrefab, arsenalPosition, "0 0 0");
		if (arsenal)
			return arsenal;

		if (arsenalPrefab != ARSENAL_FALLBACK_PREFAB)
		{
			if (!m_bWarnedArsenalResourceFailure)
			{
				Print(string.Format("h-istasi | HQ arsenal prefab %1 failed to spawn; using FIA supply-cache fallback", arsenalPrefab), LogLevel.WARNING);
				m_bWarnedArsenalResourceFailure = true;
			}

			return respawnSystem.DoSpawn(ARSENAL_FALLBACK_PREFAB, arsenalPosition, "0 0 0");
		}

		return null;
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

	protected bool AreRuntimeObjectsTracked()
	{
		return m_PetrosEntity && m_CacheEntity && m_ArsenalEntity && m_TentEntity;
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

		if (state)
			state.m_bHQRuntimeObjectsSpawned = false;
	}

	protected void DeleteRuntimeEntity(IEntity entity)
	{
		if (entity)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}
}
