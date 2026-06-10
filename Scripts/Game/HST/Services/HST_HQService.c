class HST_HQService
{
	static const string PETROS_BASE_PREFAB = "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et";
	static const string PETROS_PREFAB = "{6985327711303300}Prefabs/Characters/HST/Character_HST_Petros.et";
	static const string HQ_CACHE_PREFAB = "{AB1A97B1BAE8C395}Prefabs/Compositions/Slotted/SlotFlatSmall/SupplyCache_S_FIA_01.et";
	static const string ARSENAL_PREFAB = "{6985327711303400}Prefabs/Objects/HST/HST_HQArsenal.et";
	static const string HQ_TENT_PREFAB = "{01AE5FD77A9A4C21}Prefabs/Structures/Military/Camps/TentSmallUS_01/TentSmallUS_01.et";
	static const float ARSENAL_POSITION_TOLERANCE_METERS = 4.0;

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
			m_CacheEntity = respawnSystem.DoSpawn(HQ_CACHE_PREFAB, state.m_vHQCachePosition, "0 0 0");
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
			m_TentEntity = respawnSystem.DoSpawn(HQ_TENT_PREFAB, state.m_vHQTentPosition, "0 0 0");
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

		GenericEntity arsenal = respawnSystem.DoSpawn(arsenalPrefab, arsenalPosition, "0 0 0");
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
