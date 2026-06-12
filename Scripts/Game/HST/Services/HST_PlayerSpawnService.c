// Respawn-system backend for h-istasi's custom FIA HQ spawn path. This avoids
// the stock role-selection menu while still using Reforger's native possession.
[BaseContainerProps()]
class HST_PlayerSpawnLogic : SCR_AutoSpawnLogic
{
	override protected void DoSpawn_S(int playerId)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			Print(string.Format("h-istasi | cannot spawn player %1: campaign coordinator is not ready", playerId), LogLevel.ERROR);
			return;
		}

		coordinator.SpawnOrRespawnPlayer(playerId);
	}

	override void OnPlayerSpawned_S(int playerId, IEntity entity)
	{
		super.OnPlayerSpawned_S(playerId, entity);

		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (coordinator)
			coordinator.OnPlayerSpawned(playerId, entity);
	}

	override void OnPlayerSpawnFailed_S(int playerId)
	{
		super.OnPlayerSpawnFailed_S(playerId);

		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (coordinator)
			coordinator.OnPlayerSpawnFailed(playerId);
	}
}

class HST_PlayerSpawnService
{
	static const string PRIMARY_PLAYER_FACTION = "FIA";
	static const string DEFAULT_PLAYER_PREFAB = "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et";
	static const string DEFAULT_SPAWNPOINT_PREFAB = "{72713ED566A531F3}PrefabsEditable/SpawnPoints/E_SpawnPoint_FIA.et";
	static const float CONNECTED_PLAYER_SPAWN_GRACE_SECONDS = 1.5;
	static const float PENDING_SPAWN_TIMEOUT_SECONDS = 12;
	static const float DEAD_RESPAWN_DELAY_SECONDS = 3;

	protected ref array<int> m_aConnectedPlayerIds = {};
	protected ref array<float> m_aConnectedPlayerAges = {};
	protected ref array<int> m_aConnectedPlayerGraceLogged = {};
	protected ref array<int> m_aPendingSpawnPlayerIds = {};
	protected ref array<float> m_aPendingSpawnAges = {};
	protected ref array<string> m_aPendingSpawnPrefabs = {};
	protected ref array<vector> m_aPendingSpawnPositions = {};
	protected ref array<int> m_aDeadRespawnPlayerIds = {};
	protected ref array<float> m_aDeadRespawnAges = {};
	protected ref array<int> m_aDeadRespawnReadyLogged = {};

	string GetPrimaryPlayerFaction(HST_CampaignPreset preset)
	{
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			return preset.m_sResistanceFactionKey;

		return PRIMARY_PLAYER_FACTION;
	}

	vector GetHQSpawnPosition(HST_CampaignState state)
	{
		if (!state)
			return "0 0 0";

		return state.m_vHQPosition;
	}

	string GetDefaultPlayerPrefab()
	{
		return DEFAULT_PLAYER_PREFAB;
	}

	string GetDefaultSpawnPointPrefab()
	{
		return DEFAULT_SPAWNPOINT_PREFAB;
	}

	void Tick(float timeSlice)
	{
		UpdateConnectedPlayerAges(timeSlice);

		for (int i = m_aPendingSpawnPlayerIds.Count() - 1; i >= 0; i--)
		{
			m_aPendingSpawnAges[i] = m_aPendingSpawnAges[i] + timeSlice;
			if (m_aPendingSpawnAges[i] < PENDING_SPAWN_TIMEOUT_SECONDS)
				continue;

			Print(string.Format("h-istasi | FIA spawn request for player %1 timed out; allowing retry", m_aPendingSpawnPlayerIds[i]), LogLevel.WARNING);
			RemovePendingSpawnAt(i);
		}

		for (int j = m_aDeadRespawnPlayerIds.Count() - 1; j >= 0; j--)
		{
			m_aDeadRespawnAges[j] = m_aDeadRespawnAges[j] + timeSlice;
		}
	}

	int SpawnMissingConnectedPlayers(HST_CampaignState state, HST_AuthorizationService authorization, HST_PlayerLifecycleService lifecycle, bool diagnostics = false)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
		{
			if (diagnostics)
				Print("h-istasi | FIA spawn sweep: no PlayerManager yet");

			return 0;
		}

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		if (diagnostics)
			Print(string.Format("h-istasi | FIA spawn sweep: %1 connected player(s)", playerIds.Count()));

		int spawned;
		foreach (int playerId : playerIds)
		{
			RegisterPlayerOnly(state, authorization, lifecycle, playerId);
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (IsLivingPlayerEntity(playerEntity))
			{
				ClearDeadRespawn(playerId);
				continue;
			}

			if (HasPendingSpawn(playerId))
			{
				if (diagnostics)
					Print(string.Format("h-istasi | FIA spawn sweep: player %1 already has a pending spawn request", playerId));

				continue;
			}

			if (playerEntity && !IsDeadRespawnReady(playerId))
				continue;

			if (!IsConnectedPlayerSpawnGraceElapsed(playerId, diagnostics))
				continue;

			if (RequestPlayerSpawn(state, authorization, lifecycle, playerId, diagnostics))
				spawned++;
		}

		if (spawned > 0)
			SCR_RespawnSystemComponent.CloseRespawnMenu();

		return spawned;
	}

	bool SpawnOrRespawnPlayer(HST_CampaignState state, HST_AuthorizationService authorization, HST_PlayerLifecycleService lifecycle, int playerId, bool diagnostics = false)
	{
		return RequestPlayerSpawn(state, authorization, lifecycle, playerId, diagnostics);
	}

	bool RequestPlayerSpawn(HST_CampaignState state, HST_AuthorizationService authorization, HST_PlayerLifecycleService lifecycle, int playerId, bool diagnostics = false)
	{
		if (!Replication.IsServer() || !state || !authorization || !lifecycle || playerId <= 0)
		{
			if (diagnostics)
				Print(string.Format("h-istasi | cannot spawn player %1: invalid spawn context", playerId), LogLevel.WARNING);

			return false;
		}

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || !playerManager.IsPlayerConnected(playerId))
		{
			if (diagnostics)
				Print(string.Format("h-istasi | cannot spawn player %1: player manager missing or player not connected", playerId), LogLevel.WARNING);

			return false;
		}

		if (!IsConnectedPlayerSpawnGraceElapsed(playerId, diagnostics))
			return false;

		HST_PlayerState player = RegisterPlayerOnly(state, authorization, lifecycle, playerId);
		if (!player)
		{
			if (diagnostics)
				Print(string.Format("h-istasi | cannot spawn player %1: registration failed", playerId), LogLevel.WARNING);

			return false;
		}

		IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
		if (IsLivingPlayerEntity(playerEntity))
		{
			ClearDeadRespawn(playerId);
			SCR_RespawnSystemComponent.CloseRespawnMenu();
			return true;
		}

		if (playerEntity && !IsDeadRespawnReady(playerId))
			return false;

		if (HasPendingSpawn(playerId))
		{
			if (diagnostics)
				Print(string.Format("h-istasi | FIA spawn request skipped for player %1: request already pending", playerId));

			return true;
		}

		SCR_RespawnComponent respawnComponent = SCR_RespawnComponent.Cast(playerManager.GetPlayerRespawnComponent(playerId));
		if (!respawnComponent)
		{
			Print(string.Format("h-istasi | cannot spawn player %1: no SCR_RespawnComponent on player controller", playerId), LogLevel.ERROR);
			return false;
		}

		vector spawnPosition = GetPlayerSpawnPosition(state, playerId);
		Print(string.Format("h-istasi | requesting FIA spawn for player %1 at HQ hideout %2", playerId, spawnPosition));

		SCR_FreeSpawnData spawnData = new SCR_FreeSpawnData(DEFAULT_PLAYER_PREFAB, spawnPosition, "0 0 0");
		SetPendingSpawn(playerId, DEFAULT_PLAYER_PREFAB, spawnPosition);
		if (!respawnComponent.RequestSpawn(spawnData))
		{
			ClearPendingSpawn(playerId);
			Print(string.Format("h-istasi | native FIA spawn request rejected for player %1", playerId), LogLevel.ERROR);
			return false;
		}

		ClearDeadRespawn(playerId);
		SCR_RespawnSystemComponent.CloseRespawnMenu();
		return true;
	}

	bool OnPlayerSpawned(HST_CampaignState state, HST_AuthorizationService authorization, HST_PlayerLifecycleService lifecycle, int playerId, IEntity entity)
	{
		if (!state || !authorization || !lifecycle || playerId <= 0 || !entity)
			return false;

		HST_PlayerState player = RegisterPlayerOnly(state, authorization, lifecycle, playerId);
		if (!player)
			return false;

		int pendingIndex = FindPendingSpawnIndex(playerId);
		vector spawnPosition = GetPlayerSpawnPosition(state, playerId);
		string spawnPrefab = DEFAULT_PLAYER_PREFAB;
		if (pendingIndex >= 0)
		{
			spawnPosition = m_aPendingSpawnPositions[pendingIndex];
			spawnPrefab = m_aPendingSpawnPrefabs[pendingIndex];
		}

		player.m_sFactionKey = PRIMARY_PLAYER_FACTION;
		player.m_bHasSpawnRecord = true;
		player.m_iSpawnCount++;
		player.m_sLastSpawnPrefab = spawnPrefab;
		player.m_vLastSpawnPosition = spawnPosition;
		ApplyFaction(entity, PRIMARY_PLAYER_FACTION);
		ClearPendingSpawn(playerId);
		ClearDeadRespawn(playerId);
		ResetConnectedPlayerGraceLog(playerId);
		SCR_RespawnSystemComponent.CloseRespawnMenu();
		Print(string.Format("h-istasi | FIA player %1 spawned through native respawn pipeline", playerId));
		return true;
	}

	void OnPlayerSpawnFailed(int playerId)
	{
		if (playerId <= 0)
			return;

		ClearPendingSpawn(playerId);
		ClearDeadRespawn(playerId);
		Print(string.Format("h-istasi | FIA spawn failed for player %1; pending request cleared", playerId), LogLevel.WARNING);
	}

	bool HasPendingSpawn(int playerId)
	{
		return FindPendingSpawnIndex(playerId) >= 0;
	}

	bool HasAnyPendingSpawn()
	{
		return m_aPendingSpawnPlayerIds.Count() > 0;
	}

	bool AreConnectedPlayersSpawnStable(HST_CampaignState state)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (HasPendingSpawn(playerId))
				return false;

			if (!HasLivingPlayerEntity(playerManager, playerId))
				return false;
		}

		return true;
	}

	bool NeedsSpawnSweep(HST_CampaignState state)
	{
		return !AreConnectedPlayersSpawnStable(state);
	}

	protected HST_PlayerState RegisterPlayerOnly(HST_CampaignState state, HST_AuthorizationService authorization, HST_PlayerLifecycleService lifecycle, int playerId)
	{
		if (!state || !authorization || !lifecycle || playerId <= 0)
			return null;

		return lifecycle.RegisterConnectedPlayer(state, authorization, playerId, "", false);
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

	protected bool HasLivingPlayerEntity(PlayerManager playerManager, int playerId)
	{
		return IsLivingPlayerEntity(GetBestPlayerEntity(playerManager, playerId));
	}

	protected bool IsLivingPlayerEntity(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected bool IsDeadRespawnReady(int playerId)
	{
		int index = FindDeadRespawnIndex(playerId);
		if (index < 0)
		{
			m_aDeadRespawnPlayerIds.Insert(playerId);
			m_aDeadRespawnAges.Insert(0);
			m_aDeadRespawnReadyLogged.Insert(0);
			Print(string.Format("h-istasi | player %1 detected dead; scheduling FIA respawn in %2s", playerId, DEAD_RESPAWN_DELAY_SECONDS), LogLevel.WARNING);
			return false;
		}

		if (m_aDeadRespawnAges[index] < DEAD_RESPAWN_DELAY_SECONDS)
			return false;

		if (m_aDeadRespawnReadyLogged[index] == 0)
		{
			m_aDeadRespawnReadyLogged[index] = 1;
			Print(string.Format("h-istasi | respawn delay elapsed for player %1; requesting native FIA respawn", playerId), LogLevel.WARNING);
		}

		return true;
	}

	protected int FindPendingSpawnIndex(int playerId)
	{
		return m_aPendingSpawnPlayerIds.Find(playerId);
	}

	protected void UpdateConnectedPlayerAges(float timeSlice)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			int index = EnsureConnectedPlayerTracked(playerId);
			if (index >= 0)
				m_aConnectedPlayerAges[index] = m_aConnectedPlayerAges[index] + timeSlice;
		}

		for (int i = m_aConnectedPlayerIds.Count() - 1; i >= 0; i--)
		{
			if (playerIds.Contains(m_aConnectedPlayerIds[i]))
				continue;

			int disconnectedPlayerId = m_aConnectedPlayerIds[i];
			m_aConnectedPlayerIds.Remove(i);
			m_aConnectedPlayerAges.Remove(i);
			m_aConnectedPlayerGraceLogged.Remove(i);
			ClearPendingSpawn(disconnectedPlayerId);
			ClearDeadRespawn(disconnectedPlayerId);
		}
	}

	protected int EnsureConnectedPlayerTracked(int playerId)
	{
		if (playerId <= 0)
			return -1;

		int index = m_aConnectedPlayerIds.Find(playerId);
		if (index >= 0)
			return index;

		m_aConnectedPlayerIds.Insert(playerId);
		m_aConnectedPlayerAges.Insert(0);
		m_aConnectedPlayerGraceLogged.Insert(0);
		return m_aConnectedPlayerIds.Count() - 1;
	}

	protected bool IsConnectedPlayerSpawnGraceElapsed(int playerId, bool diagnostics = false)
	{
		int index = EnsureConnectedPlayerTracked(playerId);
		if (index < 0)
			return false;

		if (m_aConnectedPlayerAges[index] >= CONNECTED_PLAYER_SPAWN_GRACE_SECONDS)
			return true;

		if (diagnostics && m_aConnectedPlayerGraceLogged[index] == 0)
		{
			m_aConnectedPlayerGraceLogged[index] = 1;
			Print(string.Format("h-istasi | delaying FIA spawn for player %1 until native player state settles", playerId));
		}

		return false;
	}

	protected void ResetConnectedPlayerGraceLog(int playerId)
	{
		int index = m_aConnectedPlayerIds.Find(playerId);
		if (index >= 0)
			m_aConnectedPlayerGraceLogged[index] = 0;
	}

	protected void SetPendingSpawn(int playerId, string prefab, vector position)
	{
		int index = FindPendingSpawnIndex(playerId);
		if (index >= 0)
		{
			m_aPendingSpawnAges[index] = 0;
			m_aPendingSpawnPrefabs[index] = prefab;
			m_aPendingSpawnPositions[index] = position;
			return;
		}

		m_aPendingSpawnPlayerIds.Insert(playerId);
		m_aPendingSpawnAges.Insert(0);
		m_aPendingSpawnPrefabs.Insert(prefab);
		m_aPendingSpawnPositions.Insert(position);
	}

	protected void ClearPendingSpawn(int playerId)
	{
		int index = FindPendingSpawnIndex(playerId);
		if (index >= 0)
			RemovePendingSpawnAt(index);
	}

	protected int FindDeadRespawnIndex(int playerId)
	{
		return m_aDeadRespawnPlayerIds.Find(playerId);
	}

	protected void ClearDeadRespawn(int playerId)
	{
		int index = FindDeadRespawnIndex(playerId);
		if (index < 0)
			return;

		m_aDeadRespawnPlayerIds.Remove(index);
		m_aDeadRespawnAges.Remove(index);
		m_aDeadRespawnReadyLogged.Remove(index);
	}

	protected void RemovePendingSpawnAt(int index)
	{
		m_aPendingSpawnPlayerIds.Remove(index);
		m_aPendingSpawnAges.Remove(index);
		m_aPendingSpawnPrefabs.Remove(index);
		m_aPendingSpawnPositions.Remove(index);
	}

	protected vector GetPlayerSpawnPosition(HST_CampaignState state, int playerId)
	{
		vector fallbackPosition = HST_DefaultCatalog.GetHideoutPosition(HST_DefaultCatalog.GetDefaultHideoutId());
		vector spawnPosition = GetHQSpawnPosition(state);
		if (!state || !state.m_bHQDeployed)
			spawnPosition = fallbackPosition;

		vector offset = GetSpawnRingOffset(PositiveModulo(playerId - 1, 16));
		return ResolveDryPlayerSpawnPosition(spawnPosition + offset, spawnPosition, fallbackPosition);
	}

	protected vector ResolveDryPlayerSpawnPosition(vector preferredPosition, vector hqPosition, vector fallbackPosition)
	{
		vector resolvedPosition;
		if (HST_WorldPositionService.TryResolveGroundPosition(preferredPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, resolvedPosition, true))
			return resolvedPosition;

		if (HST_WorldPositionService.TryResolveGroundPosition(hqPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, resolvedPosition, true))
			return resolvedPosition;

		if (HST_WorldPositionService.TryResolveGroundPosition(fallbackPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, resolvedPosition, true))
			return resolvedPosition;

		vector emergencyPosition = HST_DefaultCatalog.GetEmergencySpawnPosition();
		if (HST_WorldPositionService.TryResolveGroundPosition(emergencyPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, resolvedPosition, true))
			return resolvedPosition;

		emergencyPosition[1] = emergencyPosition[1] + HST_WorldPositionService.CHARACTER_GROUND_OFFSET;
		Print(string.Format("h-istasi | dry spawn resolution failed; using positive emergency spawn %1", emergencyPosition), LogLevel.WARNING);
		return emergencyPosition;
	}

	protected vector GetSpawnRingOffset(int slot)
	{
		if (slot == 0)
			return "12 0 0";

		if (slot == 1)
			return "11 0 11";

		if (slot == 2)
			return "0 0 15";

		if (slot == 3)
			return "-11 0 11";

		if (slot == 4)
			return "-18 0 0";

		if (slot == 5)
			return "-11 0 -11";

		if (slot == 6)
			return "0 0 -15";

		if (slot == 7)
			return "11 0 -11";

		if (slot == 8)
			return "16 0 6";

		if (slot == 9)
			return "6 0 16";

		if (slot == 10)
			return "-6 0 16";

		if (slot == 11)
			return "-16 0 6";

		if (slot == 12)
			return "-16 0 -6";

		if (slot == 13)
			return "-6 0 -16";

		if (slot == 14)
			return "6 0 -16";

		return "16 0 -6";
	}

	protected int PositiveModulo(int value, int divisor)
	{
		if (divisor <= 0)
			return 0;

		int quotient = value / divisor;
		int remainder = value - quotient * divisor;
		if (remainder < 0)
			remainder += divisor;

		return remainder;
	}

	protected void ApplyFaction(IEntity entity, string factionKey)
	{
		if (!entity || factionKey.IsEmpty())
			return;

		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (factionComponent)
			factionComponent.SetAffiliatedFactionByKey(factionKey);
	}
}
