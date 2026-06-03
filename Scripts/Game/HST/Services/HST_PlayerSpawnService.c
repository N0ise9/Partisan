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

		if (!coordinator.SpawnOrRespawnPlayer(playerId))
			Print(string.Format("h-istasi | custom FIA spawn failed for player %1", playerId), LogLevel.ERROR);
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
	static const float PENDING_SPAWN_TIMEOUT_SECONDS = 12;

	protected ref array<int> m_aPendingSpawnPlayerIds = {};
	protected ref array<float> m_aPendingSpawnAges = {};
	protected ref array<string> m_aPendingSpawnPrefabs = {};
	protected ref array<vector> m_aPendingSpawnPositions = {};

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
		for (int i = m_aPendingSpawnPlayerIds.Count() - 1; i >= 0; i--)
		{
			m_aPendingSpawnAges[i] = m_aPendingSpawnAges[i] + timeSlice;
			if (m_aPendingSpawnAges[i] < PENDING_SPAWN_TIMEOUT_SECONDS)
				continue;

			Print(string.Format("h-istasi | FIA spawn request for player %1 timed out; allowing retry", m_aPendingSpawnPlayerIds[i]), LogLevel.WARNING);
			RemovePendingSpawnAt(i);
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
			if (HasPlayerEntity(playerManager, playerId))
			{
				if (diagnostics)
					Print(string.Format("h-istasi | FIA spawn sweep: player %1 already has a controlled entity", playerId));

				continue;
			}

			if (HasPendingSpawn(playerId))
			{
				if (diagnostics)
					Print(string.Format("h-istasi | FIA spawn sweep: player %1 already has a pending spawn request", playerId));

				continue;
			}

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

		HST_PlayerState player = RegisterPlayerOnly(state, authorization, lifecycle, playerId);
		if (!player)
		{
			if (diagnostics)
				Print(string.Format("h-istasi | cannot spawn player %1: registration failed", playerId), LogLevel.WARNING);

			return false;
		}

		if (HasPlayerEntity(playerManager, playerId))
		{
			SCR_RespawnSystemComponent.CloseRespawnMenu();
			return true;
		}

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
		if (diagnostics)
			Print(string.Format("h-istasi | requesting FIA spawn for player %1 at HQ hideout", playerId));

		SCR_FreeSpawnData spawnData = new SCR_FreeSpawnData(DEFAULT_PLAYER_PREFAB, spawnPosition, "0 0 0");
		SetPendingSpawn(playerId, DEFAULT_PLAYER_PREFAB, spawnPosition);
		if (!respawnComponent.RequestSpawn(spawnData))
		{
			ClearPendingSpawn(playerId);
			Print(string.Format("h-istasi | native FIA spawn request rejected for player %1", playerId), LogLevel.ERROR);
			return false;
		}

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
		SCR_RespawnSystemComponent.CloseRespawnMenu();
		Print(string.Format("h-istasi | FIA player %1 spawned through native respawn pipeline", playerId));
		return true;
	}

	void OnPlayerSpawnFailed(int playerId)
	{
		if (playerId <= 0)
			return;

		ClearPendingSpawn(playerId);
		Print(string.Format("h-istasi | FIA spawn failed for player %1; pending request cleared", playerId), LogLevel.WARNING);
	}

	bool HasPendingSpawn(int playerId)
	{
		return FindPendingSpawnIndex(playerId) >= 0;
	}

	protected HST_PlayerState RegisterPlayerOnly(HST_CampaignState state, HST_AuthorizationService authorization, HST_PlayerLifecycleService lifecycle, int playerId)
	{
		if (!state || !authorization || !lifecycle || playerId <= 0)
			return null;

		return lifecycle.RegisterConnectedPlayer(state, authorization, playerId, "", false);
	}

	protected bool HasPlayerEntity(PlayerManager playerManager, int playerId)
	{
		if (!playerManager || playerId <= 0)
			return false;

		if (playerManager.GetPlayerControlledEntity(playerId))
			return true;

		if (SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId))
			return true;

		return false;
	}

	protected int FindPendingSpawnIndex(int playerId)
	{
		return m_aPendingSpawnPlayerIds.Find(playerId);
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

	protected void RemovePendingSpawnAt(int index)
	{
		m_aPendingSpawnPlayerIds.Remove(index);
		m_aPendingSpawnAges.Remove(index);
		m_aPendingSpawnPrefabs.Remove(index);
		m_aPendingSpawnPositions.Remove(index);
	}

	protected vector GetPlayerSpawnPosition(HST_CampaignState state, int playerId)
	{
		vector spawnPosition = GetHQSpawnPosition(state);
		if (!state || !state.m_bHQDeployed)
			spawnPosition = HST_DefaultCatalog.GetHideoutPosition(HST_DefaultCatalog.GetDefaultHideoutId());

		int slot = PositiveModulo(playerId, 9);
		vector offset = "0 0 0";
		offset[0] = (PositiveModulo(slot, 3) - 1) * 2;
		offset[2] = ((slot / 3) - 1) * 2;
		return spawnPosition + offset;
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
