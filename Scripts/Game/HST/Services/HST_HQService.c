class HST_HQService
{
	static const string PETROS_BASE_PREFAB = "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et";
	static const string PETROS_PREFAB = "{6985327711303300}Prefabs/Characters/HST/Character_HST_Petros.et";
	static const string PETROS_GROUP_PREFAB = "{6985327711303900}Prefabs/Groups/HST/HST_PetrosGroup.et";
	static const string HQ_CACHE_PREFAB = "{AB1A97B1BAE8C395}Prefabs/Compositions/Slotted/SlotFlatSmall/SupplyCache_S_FIA_01.et";
	static const string ARSENAL_PREFAB = "{6985327711303400}Prefabs/Objects/HST/HST_HQArsenal.et";
	static const string HQ_TENT_PREFAB = "{01AE5FD77A9A4C21}Prefabs/Structures/Military/Camps/TentSmallUS_01/TentSmallUS_01.et";
	static const string HQ_SPAWN_POINT_PREFAB = "{72713ED566A531F3}PrefabsEditable/SpawnPoints/E_SpawnPoint_FIA.et";
	static const float ARSENAL_POSITION_TOLERANCE_METERS = 4.0;
	static const float PETROS_REATTACH_RADIUS_METERS = 8.0;
	static const float PETROS_WORLD_UNIQUE_RADIUS_METERS = 10.0;
	static const float HQ_ENEMY_ACTIVITY_AWARENESS_RADIUS_METERS = 1000.0;
	static const float HQ_CIVILIAN_HEAT_AWARENESS_RADIUS_METERS = 1250.0;
	static const int HQ_THREAT_SCAN_SECONDS = 30;
	static const int HQ_PASSIVE_KNOWLEDGE_SCAN_SECONDS = 180;
	static const int HQ_PASSIVE_KNOWLEDGE_THREAT_THRESHOLD = 35;
	static const int HQ_PASSIVE_KNOWLEDGE_MAX_GAIN = 2;
	static const int PETROS_RESPAWN_DEBOUNCE_SECONDS = 10;
	static const int PETROS_STABILITY_SAMPLE_SECONDS = 15;
	static const string CUSTOM_SETUP_HQ_ID = "custom_setup_hq";
	static const float SETUP_ZONE_FALLBACK_RADIUS_METERS = 150.0;

	protected IEntity m_PetrosEntity;
	protected IEntity m_PetrosGroupEntity;
	protected IEntity m_CacheEntity;
	protected IEntity m_ArsenalEntity;
	protected IEntity m_TentEntity;
	protected IEntity m_SpawnPointEntity;
	protected ref array<IEntity> m_aWorldScanCandidates = {};
	protected string m_sWorldScanPrefab;
	protected bool m_bWarnedPetrosResourceFailure;
	protected bool m_bWarnedArsenalResourceFailure;
	protected bool m_bWarnedRuntimeSpawnIncomplete;
	protected bool m_bWarnedPetrosRemovalRetry;
	protected bool m_bWarnedPetrosAIGroupFallback;
	protected bool m_bLoggedPetrosSpawned;
	protected bool m_bLoggedCacheSpawned;
	protected bool m_bLoggedArsenalSpawned;
	protected bool m_bLoggedTentSpawned;
	protected bool m_bLoggedSpawnPointSpawned;
	protected bool m_bDebugLoggingEnabled;
	protected bool m_bPetrosRelocationInProgress;
	protected int m_iPetrosMissingSinceSecond = -1;
	protected int m_iPetrosLastSpawnSecond = -999999;
	protected int m_iPetrosSpawnCount;
	protected string m_sPetrosStableRuntimeKey;

	void SetDebugLoggingEnabled(bool enabled)
	{
		m_bDebugLoggingEnabled = enabled;
	}

	void SetPetrosRelocationInProgress(bool active)
	{
		m_bPetrosRelocationInProgress = active;
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

		ClearRuntimeObjects(state, "reset initial HQ selection");
		state.m_sHQHideoutId = "";
		state.m_vHQPosition = "0 0 0";
		state.m_vPetrosPosition = "0 0 0";
		state.m_vHQCachePosition = "0 0 0";
		state.m_vArsenalPosition = "0 0 0";
		state.m_vHQTentPosition = "0 0 0";
		state.m_vHQSpawnPointPosition = "0 0 0";
		state.m_bHQDeployed = false;
		state.m_bHQRuntimeObjectsSpawned = false;
		state.m_bPetrosAlive = true;
		state.m_sHQSpawnPointPrefab = "";
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

	bool MoveHQToPosition(HST_CampaignState state, vector hqPosition, string hideoutId = "field_hq", bool keepPetrosAtHQPosition = false)
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
		if (keepPetrosAtHQPosition)
			state.m_vPetrosPosition = ResolveHQObjectPosition(resolvedPosition, "0 0 0", HST_WorldPositionService.CHARACTER_GROUND_OFFSET);
		return true;
	}

	void OnPetrosKilled(HST_CampaignState state, HST_EconomyService economy, int moneyPenalty, int hrPenalty)
	{
		if (!state || !state.m_bPetrosAlive)
			return;

		state.m_bPetrosAlive = false;
		state.m_iPetrosDeaths++;
		ClearRuntimeObjects(state, "Petros killed");
		economy.AddFactionMoney(state, -moneyPenalty);
		economy.AddHR(state, -hrPenalty);
	}

	bool EnsureRuntimeObjects(HST_CampaignState state)
	{
		if (!state || !state.m_bHQDeployed || !state.m_bPetrosAlive)
			return false;

		if (state.m_bHQRuntimeObjectsSpawned && !IsLivingRuntimeEntity(m_PetrosEntity) && IsPetrosRuntimeTracked(state))
			ReattachUniqueLivingWorldPetros(state, "ready check");

		if (state.m_bHQRuntimeObjectsSpawned && AreRuntimeObjectsTracked(state) && IsUsableArsenalEntity(m_ArsenalEntity) && AreRuntimeObjectsWorldProven(state) && IsPetrosRuntimeTracked(state))
			return false;

		if (state.m_bHQRuntimeObjectsSpawned && !AreRuntimeObjectsTracked(state))
			state.m_bHQRuntimeObjectsSpawned = false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
			return false;

		EnsureRuntimeGroundPlacement(state);
		bool changed;
		bool logDetails = !m_bWarnedRuntimeSpawnIncomplete;

		if (ReattachUniqueLivingWorldPetros(state, "reattach"))
			changed = true;

		if (!m_PetrosEntity && TryResolvePetrosFromTrackedGroup(state.m_vPetrosPosition, "tracked group"))
		{
			m_bLoggedPetrosSpawned = LogRuntimeObjectSpawnSuccess("Petros", ResolvePetrosPrefab(state), state.m_vPetrosPosition, m_bLoggedPetrosSpawned);
			changed = true;
		}

		if (m_PetrosEntity && !IsLivingRuntimeEntity(m_PetrosEntity))
		{
			if (!m_bWarnedPetrosRemovalRetry)
			{
				Print("h-istasi | Petros runtime is no longer alive; retrying real Petros spawn", LogLevel.WARNING);
				m_bWarnedPetrosRemovalRetry = true;
			}
			DeleteRuntimeEntity(m_PetrosEntity);
			m_PetrosEntity = null;
			DeleteRuntimeEntity(m_PetrosGroupEntity);
			m_PetrosGroupEntity = null;
			m_bLoggedPetrosSpawned = false;
			changed = true;
		}
		else if (m_PetrosEntity)
		{
			if (m_bPetrosRelocationInProgress)
				PreparePetrosRelocationEntity(m_PetrosEntity);
			else
				PreparePetrosEntity(m_PetrosEntity, state.m_vPetrosPosition);
			m_iPetrosMissingSinceSecond = -1;
			m_sPetrosStableRuntimeKey = BuildRuntimeEntityKey("petros", m_PetrosEntity);
			if (!IsPetrosAIGroupTracked() && !EnsurePetrosAIGroup(m_PetrosEntity, state.m_vPetrosPosition, "runtime refresh"))
				WarnPetrosAIGroupFallback("runtime refresh");
		}

		if (!m_PetrosEntity && m_bLoggedPetrosSpawned)
		{
			DeleteRuntimeEntity(m_PetrosGroupEntity);
			m_PetrosGroupEntity = null;
			if (!m_bWarnedPetrosRemovalRetry)
			{
				Print("h-istasi | Petros character runtime was removed after spawn; retrying real Petros spawn", LogLevel.WARNING);
				m_bWarnedPetrosRemovalRetry = true;
			}
			m_bLoggedPetrosSpawned = false;
			changed = true;
		}

		if (!m_PetrosEntity)
		{
			if (m_iPetrosMissingSinceSecond < 0)
				m_iPetrosMissingSinceSecond = state.m_iElapsedSeconds;

			int missingFor = state.m_iElapsedSeconds - m_iPetrosMissingSinceSecond;
			int sinceLastSpawn = state.m_iElapsedSeconds - m_iPetrosLastSpawnSecond;
			bool firstSpawn = m_iPetrosSpawnCount <= 0 && !m_bLoggedPetrosSpawned;
			bool canSpawnPetros = firstSpawn || (missingFor >= PETROS_RESPAWN_DEBOUNCE_SECONDS && sinceLastSpawn >= PETROS_RESPAWN_DEBOUNCE_SECONDS);
			if (!canSpawnPetros)
			{
				DebugLog(string.Format("lifecycle Petros missing but respawn debounced | missingFor=%1 | sinceLastSpawn=%2 | spawnCount=%3", missingFor, sinceLastSpawn, m_iPetrosSpawnCount));
			}
			else
			{
				m_PetrosEntity = SpawnPetros(respawnSystem, state);
				if (m_PetrosEntity)
				{
					PreparePetrosEntity(m_PetrosEntity, state.m_vPetrosPosition);
					m_iPetrosLastSpawnSecond = state.m_iElapsedSeconds;
					m_iPetrosSpawnCount++;
					m_iPetrosMissingSinceSecond = -1;
					m_sPetrosStableRuntimeKey = BuildRuntimeEntityKey("petros", m_PetrosEntity);
					m_bLoggedPetrosSpawned = LogRuntimeObjectSpawnSuccess("Petros", ResolvePetrosPrefab(state), state.m_vPetrosPosition, m_bLoggedPetrosSpawned);
					changed = true;
				}
				else if (IsPetrosGroupSpawnPending())
				{
					if (m_iPetrosSpawnCount <= 0 || sinceLastSpawn >= PETROS_RESPAWN_DEBOUNCE_SECONDS)
					{
						m_iPetrosLastSpawnSecond = state.m_iElapsedSeconds;
						m_iPetrosSpawnCount++;
					}
					if (logDetails)
						DebugLog(string.Format("lifecycle Petros group-owned spawn pending | group=%1 | %2", m_PetrosGroupEntity, BuildPetrosAIGroupDebugSummary()));
				}
				else if (logDetails)
					LogRuntimeObjectSpawnFailure("Petros", ResolvePetrosPrefab(state), state.m_vPetrosPosition);
			}
		}

		if (!m_CacheEntity)
		{
			IEntity cacheEntity = ResolveReusableHQRuntimeObject(state.m_vHQCachePosition, ResolveStatePrefab(state.m_sHQCachePrefab, HQ_CACHE_PREFAB), 10.0, "cache", "reattach");
			if (cacheEntity)
			{
				m_CacheEntity = cacheEntity;
				m_bLoggedCacheSpawned = true;
				changed = true;
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
			IEntity arsenalEntity = ResolveReusableHQRuntimeObject(state.m_vArsenalPosition, ResolveStatePrefab(state.m_sArsenalPrefab, ARSENAL_PREFAB), 10.0, "arsenal", "reattach");
			if (arsenalEntity)
			{
				m_ArsenalEntity = arsenalEntity;
				m_bLoggedArsenalSpawned = true;
				changed = true;
			}
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
			IEntity tentEntity = ResolveReusableHQRuntimeObject(state.m_vHQTentPosition, ResolveStatePrefab(state.m_sHQTentPrefab, HQ_TENT_PREFAB), 10.0, "tent", "reattach");
			if (tentEntity)
			{
				m_TentEntity = tentEntity;
				m_bLoggedTentSpawned = true;
				changed = true;
			}
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

		if (!m_SpawnPointEntity)
		{
			IEntity spawnPointEntity = ResolveReusableHQRuntimeObject(state.m_vHQSpawnPointPosition, ResolveStatePrefab(state.m_sHQSpawnPointPrefab, HQ_SPAWN_POINT_PREFAB), 10.0, "spawn point", "reattach");
			if (spawnPointEntity)
			{
				m_SpawnPointEntity = spawnPointEntity;
				ApplyFaction(m_SpawnPointEntity);
				state.m_sHQSpawnPointPrefab = HQ_SPAWN_POINT_PREFAB;
				m_bLoggedSpawnPointSpawned = true;
				changed = true;
			}
		}

		if (!m_SpawnPointEntity)
		{
			m_SpawnPointEntity = HST_WorldPositionService.SpawnPrefab(HQ_SPAWN_POINT_PREFAB, state.m_vHQSpawnPointPosition, "0 0 0");
			if (m_SpawnPointEntity)
			{
				ApplyFaction(m_SpawnPointEntity);
				state.m_sHQSpawnPointPrefab = HQ_SPAWN_POINT_PREFAB;
				m_bLoggedSpawnPointSpawned = LogRuntimeObjectSpawnSuccess("spawn point", HQ_SPAWN_POINT_PREFAB, state.m_vHQSpawnPointPosition, m_bLoggedSpawnPointSpawned);
				changed = true;
			}
			else if (logDetails)
				LogRuntimeObjectSpawnFailure("spawn point", HQ_SPAWN_POINT_PREFAB, state.m_vHQSpawnPointPosition);
		}

		if (!IsLivingRuntimeEntity(m_PetrosEntity) && ReattachUniqueLivingWorldPetros(state, "final runtime proof"))
			changed = true;

		if (!IsLivingRuntimeEntity(m_PetrosEntity) && IsPetrosRuntimeTracked(state))
			ReattachUniqueLivingWorldPetros(state, "final ready check");

		bool allRuntimeObjectsTracked = AreRuntimeObjectsTracked(state);
		bool petrosRuntimeReady = IsPetrosRuntimeTracked(state);
		bool runtimeObjectsWorldProven = AreRuntimeObjectsWorldProven(state);
		bool runtimeObjectsReady = allRuntimeObjectsTracked && runtimeObjectsWorldProven && IsUsableArsenalEntity(m_ArsenalEntity) && petrosRuntimeReady;
		bool runtimeFlagChanged = state.m_bHQRuntimeObjectsSpawned != runtimeObjectsReady;
		state.m_bHQRuntimeObjectsSpawned = runtimeObjectsReady;
		if (!runtimeObjectsReady)
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

		ClearRuntimeObjects(state, "manual rebuild requested");
		EnsureRuntimeGroundPlacement(state);
		state.m_sArsenalPrefab = ARSENAL_PREFAB;
		state.m_sHQSpawnPointPrefab = HQ_SPAWN_POINT_PREFAB;
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
		if (state.m_iElapsedSeconds < state.m_iLastHQThreatScanSecond + HQ_THREAT_SCAN_SECONDS)
			return false;

		state.m_iLastHQThreatScanSecond = state.m_iElapsedSeconds;

		string enemyReason;
		string aggressionReason;
		string civilianReason;
		int enemyThreat = ResolveEnemyActivityThreatNearHQ(state, preset, enemyReason);
		int aggressionThreat = ResolveAggressionThreat(state, preset, aggressionReason);
		int civilianThreat = ResolveCivilianHeatThreatNearHQ(state, civilianReason);
		int localThreat = enemyThreat + civilianThreat;
		int threat = localThreat + aggressionThreat;

		string reason = AppendThreatReason("", enemyReason);
		reason = AppendThreatReason(reason, aggressionReason);
		reason = AppendThreatReason(reason, civilianReason);
		if (reason.IsEmpty())
			reason = "quiet";

		string localReason = AppendThreatReason("", enemyReason);
		localReason = AppendThreatReason(localReason, civilianReason);
		if (localReason.IsEmpty())
			localReason = "local activity quiet";

		int beforeThreat = state.m_iHQThreatLevel;
		state.m_iHQThreatLevel = Math.Max(0, Math.Min(100, threat));
		state.m_sLastHQThreatReason = reason;
		if (localThreat > 0)
			state.m_iLastHQActivitySecond = state.m_iElapsedSeconds;

		bool knowledgeChanged;
		bool passiveKnowledgeWindow = state.m_iHQKnowledgeLastChangedSecond <= 0 || state.m_iElapsedSeconds >= state.m_iHQKnowledgeLastChangedSecond + HQ_PASSIVE_KNOWLEDGE_SCAN_SECONDS;
		if (localThreat >= HQ_PASSIVE_KNOWLEDGE_THREAT_THRESHOLD && passiveKnowledgeWindow)
		{
			int knowledgeGain = Math.Min(HQ_PASSIVE_KNOWLEDGE_MAX_GAIN, Math.Max(1, localThreat / 40));
			knowledgeChanged = AddHQKnowledge(state, knowledgeGain, "HQ local activity scan: " + localReason);
		}

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

	string GetSpawnPointPrefab()
	{
		return HQ_SPAWN_POINT_PREFAB;
	}

	int GetTrackedRuntimeObjectCount(HST_CampaignState state = null)
	{
		int count;
		if (IsPetrosRuntimeTracked(state))
			count++;
		if (m_CacheEntity)
			count++;
		if (m_ArsenalEntity)
			count++;
		if (m_TentEntity)
			count++;
		if (m_SpawnPointEntity)
			count++;

		return count;
	}

	bool HasPetrosRuntimeEntity()
	{
		return IsLivingRuntimeEntity(m_PetrosEntity);
	}

	IEntity GetPetrosRuntimeEntity()
	{
		return ResolvePetrosRuntimeEntity();
	}

	bool HasCacheRuntimeEntity()
	{
		return m_CacheEntity != null;
	}

	bool HasArsenalRuntimeEntity()
	{
		return m_ArsenalEntity != null;
	}

	bool HasTentRuntimeEntity()
	{
		return m_TentEntity != null;
	}

	bool HasSpawnPointRuntimeEntity()
	{
		return m_SpawnPointEntity != null;
	}

	bool IsArsenalRuntimeEntityUsable()
	{
		return IsUsableArsenalEntity(m_ArsenalEntity);
	}

	bool IsLoadoutEditorFirstArsenalSelectableAction(IEntity userEntity)
	{
		string firstActionKind;
		string firstActionName;
		int firstActionIndex;
		return ResolveFirstArsenalSelectableAction(userEntity, firstActionKind, firstActionName, firstActionIndex) && firstActionKind == "loadout_editor";
	}

	bool IsRelocateHQLastPetrosSelectableAction(IEntity userEntity)
	{
		EnsurePetrosActionFilterApplied();
		string lastActionKind;
		string lastActionName;
		int lastActionIndex;
		return ResolveLastPetrosSelectableAction(userEntity, lastActionKind, lastActionName, lastActionIndex) && lastActionKind == "relocate_hq";
	}

	string BuildArsenalActionSurfaceReport(IEntity userEntity)
	{
		if (!m_ArsenalEntity)
			return "h-istasi HQ arsenal actions | entity missing";

		ActionsManagerComponent actionsManager = ActionsManagerComponent.Cast(m_ArsenalEntity.FindComponent(ActionsManagerComponent));
		if (!actionsManager)
			return "h-istasi HQ arsenal actions | actions manager missing";

		array<BaseUserAction> actions = {};
		int actionCount = actionsManager.GetActionsList(actions);
		string firstActionKind;
		string firstActionName;
		int firstActionIndex;
		bool firstResolved = ResolveFirstArsenalSelectableAction(userEntity, firstActionKind, firstActionName, firstActionIndex);

		int loadoutIndex = -1;
		int hqMenuIndex = -1;
		int disabledCount;
		int shownCount;
		int selectableCount;
		string rows;
		for (int i = 0; i < actions.Count(); i++)
		{
			BaseUserAction action = actions[i];
			if (!action)
				continue;

			string kind = ResolveArsenalActionKind(action);
			if (kind == "loadout_editor")
				loadoutIndex = i;
			else if (kind == "hq_menu")
				hqMenuIndex = i;

			bool disabled = action.WasDisabledByServer();
			bool shown = IsArsenalActionShown(action, userEntity);
			bool selectable = IsArsenalActionSelectable(action, userEntity);
			if (disabled)
				disabledCount++;
			if (shown)
				shownCount++;
			if (selectable)
				selectableCount++;

			rows = rows + string.Format("\n  #%1 %2 | %3 | disabled %4 | shown %5 | selectable %6", i, ResolveActionName(action), kind, disabled, shown, selectable);
		}

		string firstSummary = "none";
		if (firstResolved)
			firstSummary = string.Format("#%1 %2 | %3", firstActionIndex, firstActionName, firstActionKind);

		return string.Format("h-istasi HQ arsenal actions | count %1 | disabled %2 | shown %3 | selectable %4 | first selectable %5 | loadout index %6 | HQ menu index %7%8", actionCount, disabledCount, shownCount, selectableCount, firstSummary, loadoutIndex, hqMenuIndex, rows);
	}

	string BuildPetrosActionSurfaceReport(IEntity userEntity)
	{
		if (!m_PetrosEntity)
			return "h-istasi Petros actions | entity missing";

		EnsurePetrosActionFilterApplied();
		ActionsManagerComponent actionsManager = ActionsManagerComponent.Cast(m_PetrosEntity.FindComponent(ActionsManagerComponent));
		if (!actionsManager)
			return "h-istasi Petros actions | actions manager missing";

		array<BaseUserAction> actions = {};
		int actionCount = actionsManager.GetActionsList(actions);
		string lastActionKind;
		string lastActionName;
		int lastActionIndex;
		bool lastResolved = ResolveLastPetrosSelectableAction(userEntity, lastActionKind, lastActionName, lastActionIndex);

		int menuIndex = -1;
		int arsenalIndex = -1;
		int relocateIndex = -1;
		int disabledCount;
		int shownCount;
		int selectableCount;
		string rows;
		for (int i = 0; i < actions.Count(); i++)
		{
			BaseUserAction action = actions[i];
			if (!action)
				continue;

			string kind = ResolvePetrosActionKind(action);
			if (kind == "hq_menu")
				menuIndex = i;
			else if (kind == "arsenal_menu")
				arsenalIndex = i;
			else if (kind == "relocate_hq")
				relocateIndex = i;

			bool disabled = action.WasDisabledByServer();
			bool shown = IsActionShown(action, userEntity);
			bool selectable = IsActionSelectable(action, userEntity);
			if (disabled)
				disabledCount++;
			if (shown)
				shownCount++;
			if (selectable)
				selectableCount++;

			rows = rows + string.Format("\n  #%1 %2 | %3 | disabled %4 | shown %5 | selectable %6", i, ResolveActionName(action), kind, disabled, shown, selectable);
		}

		string lastSummary = "none";
		if (lastResolved)
			lastSummary = string.Format("#%1 %2 | %3", lastActionIndex, lastActionName, lastActionKind);

		return string.Format("h-istasi Petros actions | count %1 | disabled %2 | shown %3 | selectable %4 | last selectable %5 | HQ menu index %6 | arsenal index %7 | relocate index %8%9", actionCount, disabledCount, shownCount, selectableCount, lastSummary, menuIndex, arsenalIndex, relocateIndex, rows);
	}

	protected void EnsurePetrosActionFilterApplied()
	{
		if (!m_PetrosEntity)
			return;

		HST_PetrosActionFilterComponent filter = HST_PetrosActionFilterComponent.Cast(m_PetrosEntity.FindComponent(HST_PetrosActionFilterComponent));
		if (filter)
			filter.EnsureFilteredNow();
	}

	vector GetPetrosRuntimeEntityPosition()
	{
		return ResolveRuntimeEntityPosition(ResolvePetrosRuntimeEntity());
	}

	vector GetCacheRuntimeEntityPosition()
	{
		return ResolveRuntimeEntityPosition(m_CacheEntity);
	}

	vector GetArsenalRuntimeEntityPosition()
	{
		return ResolveRuntimeEntityPosition(m_ArsenalEntity);
	}

	vector GetTentRuntimeEntityPosition()
	{
		return ResolveRuntimeEntityPosition(m_TentEntity);
	}

	vector GetSpawnPointRuntimeEntityPosition()
	{
		return ResolveRuntimeEntityPosition(m_SpawnPointEntity);
	}

	string GetPetrosRuntimeEntityKey()
	{
		return BuildRuntimeEntityKey("petros", m_PetrosEntity);
	}

	int GetPetrosSpawnCount()
	{
		return m_iPetrosSpawnCount;
	}

	int GetPetrosRespawnDebounceSeconds()
	{
		return PETROS_RESPAWN_DEBOUNCE_SECONDS;
	}

	int GetPetrosStabilitySampleSeconds()
	{
		return PETROS_STABILITY_SAMPLE_SECONDS;
	}

	string GetPetrosStableRuntimeKey()
	{
		if (!m_sPetrosStableRuntimeKey.IsEmpty())
			return m_sPetrosStableRuntimeKey;

		return GetPetrosRuntimeEntityKey();
	}

	bool HasPetrosRuntimeAIGroup()
	{
		return IsPetrosAIGroupTracked();
	}

	string BuildPetrosAIGroupDebugSummary()
	{
		AIAgent agent = ResolvePetrosAIAgent(m_PetrosEntity);
		SCR_AIGroup group = SCR_AIGroup.Cast(m_PetrosGroupEntity);
		AIGroup parentGroup;
		if (agent)
			parentGroup = agent.GetParentGroup();

		bool parentMatches = group != null && parentGroup == group;
		return string.Format("petros %1 | alive %2 | group %3 | agent %4 | parent matches %5", GetPetrosRuntimeEntityKey(), IsLivingRuntimeEntity(m_PetrosEntity), BuildRuntimeEntityKey("petros_group", m_PetrosGroupEntity), agent != null, parentMatches);
	}

	string GetCacheRuntimeEntityKey()
	{
		return BuildRuntimeEntityKey("cache", m_CacheEntity);
	}

	string GetArsenalRuntimeEntityKey()
	{
		return BuildRuntimeEntityKey("arsenal", m_ArsenalEntity);
	}

	string GetTentRuntimeEntityKey()
	{
		return BuildRuntimeEntityKey("tent", m_TentEntity);
	}

	string GetSpawnPointRuntimeEntityKey()
	{
		return BuildRuntimeEntityKey("spawn_point", m_SpawnPointEntity);
	}

	string BuildRuntimeObjectDebugSummary()
	{
		return string.Format("petros %1 | cache %2 | arsenal %3 | tent %4 | spawn_point %5", GetPetrosRuntimeEntityKey(), GetCacheRuntimeEntityKey(), GetArsenalRuntimeEntityKey(), GetTentRuntimeEntityKey(), GetSpawnPointRuntimeEntityKey());
	}

	int CountPetrosWorldRuntimeEntities(HST_CampaignState state)
	{
		if (!state)
			return -1;

		return CountLivingPetrosWorldRuntimeEntities(state);
	}

	int CountCacheWorldRuntimeEntities(HST_CampaignState state)
	{
		if (!state)
			return -1;

		return CountWorldRuntimeEntitiesNear(state.m_vHQCachePosition, ResolveRuntimeScanPrefab(m_CacheEntity, ResolveStatePrefab(state.m_sHQCachePrefab, HQ_CACHE_PREFAB)), 10.0);
	}

	int CountArsenalWorldRuntimeEntities(HST_CampaignState state)
	{
		if (!state)
			return -1;

		return CountWorldRuntimeEntitiesNear(state.m_vArsenalPosition, ResolveRuntimeScanPrefab(m_ArsenalEntity, ResolveStatePrefab(state.m_sArsenalPrefab, ARSENAL_PREFAB)), 10.0);
	}

	int CountTentWorldRuntimeEntities(HST_CampaignState state)
	{
		if (!state)
			return -1;

		return CountWorldRuntimeEntitiesNear(state.m_vHQTentPosition, ResolveRuntimeScanPrefab(m_TentEntity, ResolveStatePrefab(state.m_sHQTentPrefab, HQ_TENT_PREFAB)), 10.0);
	}

	int CountSpawnPointWorldRuntimeEntities(HST_CampaignState state)
	{
		if (!state)
			return -1;

		return CountWorldRuntimeEntitiesNear(state.m_vHQSpawnPointPosition, ResolveRuntimeScanPrefab(m_SpawnPointEntity, ResolveStatePrefab(state.m_sHQSpawnPointPrefab, HQ_SPAWN_POINT_PREFAB)), 10.0);
	}

	string BuildRuntimeWorldScanSummary(HST_CampaignState state)
	{
		return string.Format("petros %1 | cache %2 | arsenal %3 | tent %4 | spawn_point %5", CountPetrosWorldRuntimeEntities(state), CountCacheWorldRuntimeEntities(state), CountArsenalWorldRuntimeEntities(state), CountTentWorldRuntimeEntities(state), CountSpawnPointWorldRuntimeEntities(state));
	}

	protected int ResolveEnemyActivityThreatNearHQ(HST_CampaignState state, HST_CampaignPreset preset, out string reason)
	{
		reason = "no enemy activity near HQ";
		if (!state || !preset || !state.m_bHQDeployed)
			return 0;

		int threat;
		float scanRadiusSq = HQ_ENEMY_ACTIVITY_AWARENESS_RADIUS_METERS * HQ_ENEMY_ACTIVITY_AWARENESS_RADIUS_METERS;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (!group)
				continue;
			if (!HST_FactionRelationService.IsEnemyFaction(preset, group.m_sFactionKey))
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
			if (!pool || (preset && !HST_FactionRelationService.IsEnemyFaction(preset, pool.m_sFactionKey)))
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
			if (DistanceSq2D(zone.m_vPosition, state.m_vHQPosition) > HQ_CIVILIAN_HEAT_AWARENESS_RADIUS_METERS * HQ_CIVILIAN_HEAT_AWARENESS_RADIUS_METERS)
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
		ClearRuntimeObjects(state, "HQ moved or placed");
		vector petrosOffset = "2 0 2";
		vector cacheOffset = "4 0 -2";
		vector tentOffset = "-4 0 2";
		vector spawnPointOffset = "12 0 0";
		state.m_sHQHideoutId = hideoutId;
		state.m_vHQPosition = hqPosition;
		state.m_vPetrosPosition = ResolveHQObjectPosition(hqPosition, petrosOffset, HST_WorldPositionService.CHARACTER_GROUND_OFFSET);
		state.m_vHQCachePosition = ResolveHQObjectPosition(hqPosition, cacheOffset, HST_WorldPositionService.PROP_GROUND_OFFSET);
		state.m_vArsenalPosition = ResolvePrimaryArsenalPosition(state);
		state.m_vHQTentPosition = ResolveHQObjectPosition(hqPosition, tentOffset, HST_WorldPositionService.PROP_GROUND_OFFSET);
		state.m_vHQSpawnPointPosition = ResolveHQObjectPosition(hqPosition, spawnPointOffset, HST_WorldPositionService.PROP_GROUND_OFFSET);
		state.m_bHQDeployed = true;
		state.m_bHQRuntimeObjectsSpawned = false;
		state.m_bPetrosAlive = true;
		state.m_sPetrosPrefab = PETROS_PREFAB;
		state.m_sHQCachePrefab = HQ_CACHE_PREFAB;
		state.m_sArsenalPrefab = ARSENAL_PREFAB;
		state.m_sHQTentPrefab = HQ_TENT_PREFAB;
		state.m_sHQSpawnPointPrefab = HQ_SPAWN_POINT_PREFAB;
		state.m_sHQArsenalRuntimeStatus = "pending spawn";
		state.m_sLastHQArsenalFailure = "";
		Print(string.Format("h-istasi | HQ %1 placed at %2; Petros %3 cache %4 arsenal %5 tent %6 spawn %7", hideoutId, state.m_vHQPosition, state.m_vPetrosPosition, state.m_vHQCachePosition, state.m_vArsenalPosition, state.m_vHQTentPosition, state.m_vHQSpawnPointPosition));
		DebugLog(string.Format("HQ position set hideout=%1 hq=%2 petros=%3 cache=%4 arsenal=%5 tent=%6 spawn=%7", hideoutId, state.m_vHQPosition, state.m_vPetrosPosition, state.m_vHQCachePosition, state.m_vArsenalPosition, state.m_vHQTentPosition, state.m_vHQSpawnPointPosition));
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
		state.m_vHQSpawnPointPosition = ResolveRuntimeObjectGroundPosition(state.m_vHQSpawnPointPosition, state.m_vHQPosition, HST_WorldPositionService.PROP_GROUND_OFFSET);
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

		bool hqDeployed;
		bool petrosAlive;
		bool runtimeSpawned;
		if (state)
		{
			hqDeployed = state.m_bHQDeployed;
			petrosAlive = state.m_bPetrosAlive;
			runtimeSpawned = state.m_bHQRuntimeObjectsSpawned;
		}

		DebugLog(string.Format("lifecycle spawning Petros prefab=%1 pos=%2 hqDeployed=%3 petrosAlive=%4 runtimeSpawned=%5", petrosPrefab, petrosPosition, hqDeployed, petrosAlive, runtimeSpawned));
		GenericEntity petros = SpawnPetrosViaGroupPrefab(petrosPosition, "dedicated Petros group spawn");
		if (petros)
			return petros;
		if (IsPetrosGroupSpawnPending())
			return null;

		if (petrosPrefab != PETROS_BASE_PREFAB)
		{
			if (!m_bWarnedPetrosResourceFailure)
			{
				Print(string.Format("h-istasi | dedicated Petros prefab %1 failed to spawn; using base FIA fallback", petrosPrefab), LogLevel.WARNING);
				m_bWarnedPetrosResourceFailure = true;
			}

			petros = SpawnPetrosCharacterPrefab(PETROS_BASE_PREFAB, petrosPosition);
			if (petros && PreparePetrosRuntimeEntity(petros, petrosPosition, "base FIA Petros fallback"))
				return petros;
			if (petros)
				DeleteRuntimeEntity(petros);

			return null;
		}

		return null;
	}

	protected GenericEntity SpawnPetrosViaGroupPrefab(vector position, string source)
	{
		SCR_AIGroup group = ResolvePetrosAIGroup(position, source);
		if (!group)
			return null;

		group.SetOrigin(position);
		group.SetSpawnImmediately(false);
		group.SetMaxUnitsToSpawn(1);
		group.SetMemberSpawnDelay(0);
		group.SetDeleteWhenEmpty(false);
		ApplyPetrosGroupFaction(group, source);

		if (group.GetAgentsCount() <= 0 && group.GetSpawnQueueSize() <= 0)
		{
			group.SpawnUnits();
			DebugLog(string.Format("lifecycle queued Petros group-owned SpawnUnits via %1 | group=%2 | queue=%3", source, group, group.GetSpawnQueueSize()));
		}

		group.ActivateAI();
		if (TryResolvePetrosFromTrackedGroup(position, source))
			return GenericEntity.Cast(m_PetrosEntity);

		return null;
	}

	protected GenericEntity SpawnPetrosCharacterPrefab(string prefab, vector position)
	{
		if (prefab.IsEmpty())
			return null;

		ResourceName resourceName = prefab;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded || !loaded.IsValid())
			return null;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = position;
		IEntity entity = GetGame().SpawnEntityPrefabEx(resourceName, true, world, params);
		GenericEntity genericEntity = GenericEntity.Cast(entity);
		if (genericEntity)
			genericEntity.SetOrigin(position);

		return genericEntity;
	}

	protected bool PreparePetrosRuntimeEntity(IEntity petros, vector position, string source)
	{
		if (!petros)
			return false;

		PreparePetrosEntity(petros, position);
		if (EnsurePetrosAIGroup(petros, position, source))
			return true;

		WarnPetrosAIGroupFallback(source);
		DeleteRuntimeEntity(m_PetrosGroupEntity);
		m_PetrosGroupEntity = null;
		return IsLivingRuntimeEntity(petros);
	}

	protected bool TryResolvePetrosFromTrackedGroup(vector position, string source)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(m_PetrosGroupEntity);
		if (!group)
			return false;

		array<AIAgent> agents = new array<AIAgent>;
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			IEntity controlledEntity = agent.GetControlledEntity();
			if (!controlledEntity || !IsLivingRuntimeEntity(controlledEntity))
				continue;

			m_PetrosEntity = controlledEntity;
			PreparePetrosEntity(m_PetrosEntity, position);
			m_iPetrosMissingSinceSecond = -1;
			m_sPetrosStableRuntimeKey = BuildRuntimeEntityKey("petros", m_PetrosEntity);
			ApplyPetrosGroupFaction(group, source);
			agent.ActivateAI();
			DebugLog(string.Format("lifecycle resolved Petros from group-owned slot via %1 | petros=%2 | group=%3 | agents=%4 | queue=%5", source, m_PetrosEntity, group, group.GetAgentsCount(), group.GetSpawnQueueSize()));
			return true;
		}

		if (group.GetAgentsCount() <= 0 && group.GetSpawnQueueSize() <= 0)
		{
			group.SetMaxUnitsToSpawn(1);
			group.SetMemberSpawnDelay(0);
			group.SetDeleteWhenEmpty(false);
			group.SpawnUnits();
			DebugLog(string.Format("lifecycle requeued Petros group-owned SpawnUnits via %1 | group=%2 | queue=%3", source, group, group.GetSpawnQueueSize()));
		}

		return false;
	}

	protected void WarnPetrosAIGroupFallback(string source)
	{
		if (m_bWarnedPetrosAIGroupFallback)
			return;

		m_bWarnedPetrosAIGroupFallback = true;
		Print(string.Format("h-istasi | Petros spawned via %1 without durable AIGroup attachment; preserving living HQ character and reporting AIGroup as diagnostic WARN", source), LogLevel.WARNING);
	}

	protected void PreparePetrosEntity(IEntity petros, vector position)
	{
		if (!petros)
			return;

		petros.SetName("HST_Petros");
		petros.SetOrigin(position);
		petros.SetFlags(EntityFlags.VISIBLE | EntityFlags.TRACEABLE | EntityFlags.ACTIVE, true);
		ApplyFaction(petros);

		ChimeraCharacter character = ChimeraCharacter.Cast(petros);
		if (!character)
			return;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return;

		controller.SetMovement(0, vector.Zero);
		controller.SetDisableMovementControls(true);
		controller.SetDisableWeaponControls(true);
	}

	protected void PreparePetrosRelocationEntity(IEntity petros)
	{
		if (!petros)
			return;

		petros.SetName("HST_Petros");
		petros.SetFlags(EntityFlags.VISIBLE | EntityFlags.TRACEABLE | EntityFlags.ACTIVE, true);
		ApplyFaction(petros);

		ChimeraCharacter character = ChimeraCharacter.Cast(petros);
		if (!character)
			return;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return;

		controller.SetDisableMovementControls(false);
		controller.SetDisableWeaponControls(true);
	}

	protected bool EnsurePetrosAIGroup(IEntity petros, vector position, string source)
	{
		if (!petros)
			return false;

		SCR_AIGroup group = ResolvePetrosAIGroup(position, source);
		if (!group)
			return false;

		ApplyPetrosGroupFaction(group, source);
		if (IsPetrosAgentInGroup(petros, group))
		{
			ApplyFaction(petros);
			DebugLog(string.Format("lifecycle confirmed Petros already in AIGroup via %1 | petros=%2 group=%3", source, petros, m_PetrosGroupEntity));
			return true;
		}

		if (!AttachPetrosToAIGroup(petros, group, source))
		{
			Print(string.Format("h-istasi | Petros AI group attach unverified via %1 | petros %2 | group %3", source, petros, group), LogLevel.WARNING);
			return false;
		}

		group.ActivateAI();
		ApplyFaction(petros);
		DebugLog(string.Format("lifecycle attached Petros to AIGroup via %1 | petros=%2 group=%3", source, petros, m_PetrosGroupEntity));
		return true;
	}

	protected bool AttachPetrosToAIGroup(IEntity petros, SCR_AIGroup group, string source)
	{
		if (!petros || !group)
			return false;

		AIAgent agent = ResolvePetrosAIAgent(petros);
		if (!agent)
		{
			DebugLog(string.Format("lifecycle Petros AIGroup attach skipped via %1: Petros has no AIAgent", source));
			return false;
		}

		AIGroup parentGroup = agent.GetParentGroup();
		if (parentGroup == group)
			return true;

		group.AddAgentFromControlledEntity(petros);
		if (IsPetrosAgentInGroup(petros, group))
			return true;

		if (parentGroup && parentGroup != group)
			group.AddAgent(agent);
		else if (!group.AddAIEntityToGroup(petros))
			group.AddAgent(agent);

		agent.ActivateAI();
		return IsPetrosAgentInGroup(petros, group);
	}

	protected SCR_AIGroup ResolvePetrosAIGroup(vector position, string source)
	{
		SCR_AIGroup existingGroup = SCR_AIGroup.Cast(m_PetrosGroupEntity);
		if (existingGroup)
			return existingGroup;

		if (m_PetrosGroupEntity)
		{
			DeleteRuntimeEntity(m_PetrosGroupEntity);
			m_PetrosGroupEntity = null;
		}

		ResourceName resourceName = PETROS_GROUP_PREFAB;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded || !loaded.IsValid())
		{
			Print(string.Format("h-istasi | Petros AI group spawn failed via %1: missing group prefab %2", source, PETROS_GROUP_PREFAB), LogLevel.WARNING);
			return null;
		}

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = position;
		IEntity groupEntity = GetGame().SpawnEntityPrefabEx(resourceName, false, world, params);
		SCR_AIGroup group = SCR_AIGroup.Cast(groupEntity);
		if (!group)
		{
			DeleteRuntimeEntity(groupEntity);
			Print(string.Format("h-istasi | Petros AI group spawn failed via %1: prefab %2 did not spawn an AIGroup", source, PETROS_GROUP_PREFAB), LogLevel.WARNING);
			return null;
		}

		group.SetName("HST_Petros_Group");
		group.SetOrigin(position);
		group.SetSpawnImmediately(false);
		group.SetMaxUnitsToSpawn(1);
		group.SetMemberSpawnDelay(0);
		group.SetDeleteWhenEmpty(false);
		m_PetrosGroupEntity = groupEntity;
		DebugLog(string.Format("lifecycle spawned Petros AIGroup via %1 prefab=%2 pos=%3 entity=%4", source, PETROS_GROUP_PREFAB, position, groupEntity));
		return group;
	}

	protected void ApplyPetrosGroupFaction(SCR_AIGroup group, string source)
	{
		if (!group || !GetGame())
			return;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return;

		Faction faction = factionManager.GetFactionByKey("FIA");
		if (!faction)
			return;

		if (!group.SetFaction(faction))
			DebugLog(string.Format("lifecycle Petros AIGroup faction unchanged via %1", source));
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

	protected bool ResolveFirstArsenalSelectableAction(IEntity userEntity, out string actionKind, out string actionName, out int actionIndex)
	{
		actionKind = "";
		actionName = "";
		actionIndex = -1;
		if (!m_ArsenalEntity)
			return false;

		ActionsManagerComponent actionsManager = ActionsManagerComponent.Cast(m_ArsenalEntity.FindComponent(ActionsManagerComponent));
		if (!actionsManager)
			return false;

		array<BaseUserAction> actions = {};
		actionsManager.GetActionsList(actions);
		for (int i = 0; i < actions.Count(); i++)
		{
			BaseUserAction action = actions[i];
			if (!action || !IsArsenalActionSelectable(action, userEntity))
				continue;

			actionKind = ResolveArsenalActionKind(action);
			actionName = ResolveActionName(action);
			actionIndex = i;
			return true;
		}

		return false;
	}

	protected bool ResolveLastPetrosSelectableAction(IEntity userEntity, out string actionKind, out string actionName, out int actionIndex)
	{
		actionKind = "";
		actionName = "";
		actionIndex = -1;
		if (!m_PetrosEntity)
			return false;

		ActionsManagerComponent actionsManager = ActionsManagerComponent.Cast(m_PetrosEntity.FindComponent(ActionsManagerComponent));
		if (!actionsManager)
			return false;

		array<BaseUserAction> actions = {};
		actionsManager.GetActionsList(actions);
		for (int i = 0; i < actions.Count(); i++)
		{
			BaseUserAction action = actions[i];
			if (!action || !IsActionSelectable(action, userEntity))
				continue;

			actionKind = ResolvePetrosActionKind(action);
			actionName = ResolveActionName(action);
			actionIndex = i;
		}

		return actionIndex >= 0;
	}

	protected bool IsArsenalActionShown(BaseUserAction action, IEntity userEntity)
	{
		return IsActionShown(action, userEntity);
	}

	protected bool IsActionShown(BaseUserAction action, IEntity userEntity)
	{
		if (!action || action.WasDisabledByServer())
			return false;

		if (!userEntity)
			return true;

		return action.CanBeShown(userEntity);
	}

	protected bool IsArsenalActionSelectable(BaseUserAction action, IEntity userEntity)
	{
		return IsActionSelectable(action, userEntity);
	}

	protected bool IsActionSelectable(BaseUserAction action, IEntity userEntity)
	{
		if (!IsActionShown(action, userEntity))
			return false;

		if (!userEntity)
			return true;

		return action.CanBePerformed(userEntity);
	}

	protected string ResolvePetrosActionKind(BaseUserAction action)
	{
		if (HST_PetrosCommandMenuAction.Cast(action))
			return "hq_menu";

		if (HST_PetrosArsenalMenuAction.Cast(action))
			return "arsenal_menu";

		if (HST_PetrosMoveBaseHereAction.Cast(action))
			return "relocate_hq";

		if (action && action.WasDisabledByServer())
			return "disabled_inherited";

		return "other";
	}

	protected string ResolveArsenalActionKind(BaseUserAction action)
	{
		if (HST_HQArsenalLoadoutEditorAction.Cast(action))
			return "loadout_editor";

		if (HST_PetrosCommandMenuAction.Cast(action))
			return "hq_menu";

		if (action && action.WasDisabledByServer())
			return "disabled_inherited";

		return "other";
	}

	protected string ResolveActionName(BaseUserAction action)
	{
		if (!action)
			return "missing";

		string name = action.GetActionName();
		if (name.IsEmpty())
			name = "unnamed";

		return name;
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

	protected bool AreRuntimeObjectsTracked(HST_CampaignState state = null)
	{
		return IsPetrosRuntimeTracked(state) && m_CacheEntity && m_ArsenalEntity && m_TentEntity && m_SpawnPointEntity;
	}

	protected bool AreRuntimeObjectsWorldProven(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool petrosWorldProven = CountPetrosWorldRuntimeEntities(state) == 1;
		if (m_bPetrosRelocationInProgress)
			petrosWorldProven = IsLivingRuntimeEntity(m_PetrosEntity);

		return petrosWorldProven
			&& CountCacheWorldRuntimeEntities(state) == 1
			&& CountArsenalWorldRuntimeEntities(state) == 1
			&& CountTentWorldRuntimeEntities(state) == 1
			&& CountSpawnPointWorldRuntimeEntities(state) == 1;
	}

	protected bool IsPetrosRuntimeTracked(HST_CampaignState state = null)
	{
		if (m_bPetrosRelocationInProgress && IsLivingRuntimeEntity(m_PetrosEntity))
			return true;

		if (!state)
			return IsLivingRuntimeEntity(m_PetrosEntity);

		return CountLivingPetrosWorldRuntimeEntities(state) == 1;
	}

	protected bool IsLivingRuntimeEntity(IEntity entity)
	{
		if (!entity)
			return false;

		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (character)
		{
			CharacterControllerComponent controller = character.GetCharacterController();
			if (controller)
				return controller.GetLifeState() != ECharacterLifeState.DEAD;
		}

		DamageManagerComponent damageManager = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));
		if (!damageManager)
			return true;

		return damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected IEntity ResolvePetrosRuntimeEntity()
	{
		return m_PetrosEntity;
	}

	protected bool ReattachUniqueLivingWorldPetros(HST_CampaignState state, string source)
	{
		if (!state)
			return false;

		int livingPetrosCount = CountLivingPetrosWorldRuntimeEntities(state);
		if (livingPetrosCount != 1)
		{
			if (livingPetrosCount > 1)
				DebugLog(string.Format("lifecycle Petros reattach skipped via %1: expected exactly one living world Petros, found %2", source, livingPetrosCount));
			return false;
		}

		IEntity worldPetros = ResolveLivingPetrosWorldEntity(state);
		if (!worldPetros || worldPetros == m_PetrosEntity)
			return false;

		m_PetrosEntity = worldPetros;
		PreparePetrosEntity(m_PetrosEntity, state.m_vPetrosPosition);
		m_iPetrosMissingSinceSecond = -1;
		m_sPetrosStableRuntimeKey = BuildRuntimeEntityKey("petros", m_PetrosEntity);
		if (EnsurePetrosAIGroup(m_PetrosEntity, state.m_vPetrosPosition, source))
		{
			DebugLog(string.Format("lifecycle reattached living world Petros via %1 prefab=%2 entity=%3 pos=%4", source, ResolvePetrosPrefab(state), m_PetrosEntity, ResolveRuntimeEntityPosition(m_PetrosEntity)));
		}
		else
		{
			WarnPetrosAIGroupFallback(source);
			DebugLog(string.Format("lifecycle reattached living world Petros via %1 without durable AIGroup prefab=%2 entity=%3 pos=%4", source, ResolvePetrosPrefab(state), m_PetrosEntity, ResolveRuntimeEntityPosition(m_PetrosEntity)));
		}

		return true;
	}

	protected IEntity ResolveReusableHQRuntimeObject(vector position, string prefab, float radiusMeters, string label, string source)
	{
		if (prefab.IsEmpty())
			return null;

		IEntity entity = FindWorldRuntimeEntityNear(position, prefab, radiusMeters);
		if (!entity)
			return null;

		DebugLog(string.Format("lifecycle reattached HQ %1 world entity via %2 prefab=%3 entity=%4 pos=%5", label, source, prefab, entity, ResolveRuntimeEntityPosition(entity)));
		return entity;
	}

	protected IEntity ResolveLivingPetrosWorldEntity(HST_CampaignState state)
	{
		if (!state)
			return null;

		IEntity found = FindWorldRuntimeEntityNear(state.m_vPetrosPosition, ResolvePetrosPrefab(state), PETROS_WORLD_UNIQUE_RADIUS_METERS);
		if (found && IsLivingRuntimeEntity(found))
			return found;

		return null;
	}

	protected int CountLivingPetrosWorldRuntimeEntities(HST_CampaignState state)
	{
		if (!state)
			return -1;

		int count;
		string prefab = ResolveRuntimeScanPrefab(ResolvePetrosRuntimeEntity(), ResolvePetrosPrefab(state));
		if (prefab.IsEmpty())
			prefab = PETROS_PREFAB;

		if (!GetGame())
			return -1;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return -1;

		m_sWorldScanPrefab = prefab;
		m_aWorldScanCandidates.Clear();
		world.QueryEntitiesBySphere(state.m_vPetrosPosition, PETROS_WORLD_UNIQUE_RADIUS_METERS, AddWorldScanCandidate, null, EQueryEntitiesFlags.ALL);
		foreach (IEntity candidate : m_aWorldScanCandidates)
		{
			if (IsLivingRuntimeEntity(candidate))
				count++;
		}

		m_aWorldScanCandidates.Clear();
		m_sWorldScanPrefab = "";
		return count;
	}

	protected bool IsPetrosAIGroupTracked()
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(m_PetrosGroupEntity);
		return IsLivingRuntimeEntity(m_PetrosEntity) && group && IsPetrosAgentInGroup(m_PetrosEntity, group);
	}

	protected bool IsPetrosGroupSpawnPending()
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(m_PetrosGroupEntity);
		if (!group || IsLivingRuntimeEntity(m_PetrosEntity))
			return false;

		return group.GetSpawnQueueSize() > 0 || group.GetAgentsCount() == 0;
	}

	protected bool IsPetrosAgentInGroup(IEntity petros, SCR_AIGroup group)
	{
		if (!petros || !group)
			return false;

		AIAgent agent = ResolvePetrosAIAgent(petros);
		AIGroup parentGroup;
		if (agent)
			parentGroup = agent.GetParentGroup();
		return parentGroup && parentGroup == group;
	}

	protected AIAgent ResolvePetrosAIAgent(IEntity petros)
	{
		if (!petros)
			return null;

		AIControlComponent control = AIControlComponent.Cast(petros.FindComponent(AIControlComponent));
		if (!control)
			return null;

		return control.GetControlAIAgent();
	}

	protected vector ResolveRuntimeEntityPosition(IEntity entity)
	{
		if (!entity)
			return "0 0 0";

		return entity.GetOrigin();
	}

	protected string BuildRuntimeEntityKey(string label, IEntity entity)
	{
		if (!entity)
			return label + ":missing";

		string prefab;
		if (entity.GetPrefabData())
			prefab = entity.GetPrefabData().GetPrefabName();
		if (prefab.IsEmpty())
			prefab = label;

		BaseRplComponent rpl = BaseRplComponent.Cast(entity.FindComponent(BaseRplComponent));
		if (rpl)
			return string.Format("%1:%2", prefab, rpl.Id());

		return string.Format("%1:%2", prefab, entity);
	}

	protected int CountWorldRuntimeEntitiesNear(vector center, string prefab, float radiusMeters)
	{
		if (prefab.IsEmpty() || radiusMeters <= 0 || !GetGame())
			return -1;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return -1;

		m_sWorldScanPrefab = prefab;
		m_aWorldScanCandidates.Clear();
		world.QueryEntitiesBySphere(center, radiusMeters, AddWorldScanCandidate, null, EQueryEntitiesFlags.ALL);
		int count = m_aWorldScanCandidates.Count();
		m_aWorldScanCandidates.Clear();
		m_sWorldScanPrefab = "";
		return count;
	}

	protected IEntity FindWorldRuntimeEntityNear(vector center, string prefab, float radiusMeters)
	{
		if (prefab.IsEmpty() || radiusMeters <= 0 || !GetGame())
			return null;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		m_sWorldScanPrefab = prefab;
		m_aWorldScanCandidates.Clear();
		world.QueryEntitiesBySphere(center, radiusMeters, AddWorldScanCandidate, null, EQueryEntitiesFlags.ALL);

		IEntity found;
		foreach (IEntity candidate : m_aWorldScanCandidates)
		{
			if (!candidate)
				continue;

			if (IsPetrosScanPrefab(m_sWorldScanPrefab) && !IsLivingRuntimeEntity(candidate))
				continue;

			found = candidate;
			break;
		}

		m_aWorldScanCandidates.Clear();
		m_sWorldScanPrefab = "";
		return found;
	}

	protected bool AddWorldScanCandidate(IEntity entity)
	{
		if (!entity || m_sWorldScanPrefab.IsEmpty())
			return true;

		if (ResolveEntityPrefabName(entity) == m_sWorldScanPrefab)
		{
			m_aWorldScanCandidates.Insert(entity);
			return true;
		}

		if (IsPetrosScanPrefab(m_sWorldScanPrefab) && IsPetrosRuntimeCandidate(entity))
			m_aWorldScanCandidates.Insert(entity);

		return true;
	}

	protected bool IsPetrosScanPrefab(string prefab)
	{
		return prefab == PETROS_PREFAB || prefab == PETROS_BASE_PREFAB || prefab.Contains("Character_HST_Petros") || prefab.Contains("Character_FIA_Rifleman");
	}

	protected bool IsPetrosRuntimeCandidate(IEntity entity)
	{
		if (!entity)
			return false;

		string name = entity.GetName();
		if (name == "HST_Petros")
			return true;

		string prefab = ResolveEntityPrefabName(entity);
		if (prefab == PETROS_PREFAB || prefab.Contains("Character_HST_Petros"))
			return true;

		return false;
	}

	protected string ResolveRuntimeScanPrefab(IEntity entity, string fallbackPrefab)
	{
		string prefab = ResolveEntityPrefabName(entity);
		if (!prefab.IsEmpty())
			return prefab;

		return fallbackPrefab;
	}

	protected string ResolveStatePrefab(string statePrefab, string fallbackPrefab)
	{
		if (!statePrefab.IsEmpty())
			return statePrefab;

		return fallbackPrefab;
	}

	protected string ResolveEntityPrefabName(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return "";

		return entity.GetPrefabData().GetPrefabName();
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

	protected void ClearRuntimeObjects(HST_CampaignState state, string reason = "unspecified")
	{
		DebugLog(string.Format("lifecycle clearing runtime objects reason=%1 petros=%2 petrosGroup=%3 cache=%4 arsenal=%5 tent=%6 spawn=%7", reason, m_PetrosEntity, m_PetrosGroupEntity, m_CacheEntity, m_ArsenalEntity, m_TentEntity, m_SpawnPointEntity));
		DeleteRuntimeEntity(m_PetrosEntity);
		DeleteRuntimeEntity(m_PetrosGroupEntity);
		DeleteRuntimeEntity(m_CacheEntity);
		DeleteRuntimeEntity(m_ArsenalEntity);
		DeleteRuntimeEntity(m_TentEntity);
		DeleteRuntimeEntity(m_SpawnPointEntity);

		m_PetrosEntity = null;
		m_PetrosGroupEntity = null;
		m_CacheEntity = null;
		m_ArsenalEntity = null;
		m_TentEntity = null;
		m_SpawnPointEntity = null;
		m_bLoggedPetrosSpawned = false;
		m_bLoggedCacheSpawned = false;
		m_bLoggedArsenalSpawned = false;
		m_bLoggedTentSpawned = false;
		m_bLoggedSpawnPointSpawned = false;
		m_bWarnedPetrosRemovalRetry = false;
		m_bWarnedPetrosAIGroupFallback = false;
		m_bPetrosRelocationInProgress = false;
		ResetPetrosRespawnState();

		if (state)
		{
			state.m_bHQRuntimeObjectsSpawned = false;
			state.m_sHQArsenalRuntimeStatus = "cleared";
		}
	}

	protected void ResetPetrosRespawnState()
	{
		m_iPetrosMissingSinceSecond = -1;
		m_iPetrosLastSpawnSecond = -999999;
		m_iPetrosSpawnCount = 0;
		m_sPetrosStableRuntimeKey = "";
	}

	protected void DeleteRuntimeEntity(IEntity entity)
	{
		if (entity)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}
}
