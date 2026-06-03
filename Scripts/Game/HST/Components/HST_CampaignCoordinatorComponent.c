[ComponentEditorProps(category: "h-istasi", description: "Server-authoritative h-istasi campaign coordinator")]
class HST_CampaignCoordinatorComponentClass : SCR_BaseGameModeComponentClass
{
}

class HST_CampaignCoordinatorComponent : SCR_BaseGameModeComponent
{
	protected static HST_CampaignCoordinatorComponent s_Instance;

	protected ref HST_CampaignState m_State;
	protected ref HST_CampaignPreset m_Preset;
	protected ref HST_BalanceConfig m_Balance;
	protected ref HST_EconomyService m_Economy;
	protected ref HST_MissionService m_Missions;
	protected ref HST_PersistenceService m_Persistence;
	protected ref HST_AuthorizationService m_Authorization;
	protected ref HST_StrategicService m_Strategic;
	protected ref HST_ArsenalService m_Arsenal;
	protected ref HST_EnemyDirectorService m_EnemyDirector;
	protected ref HST_HQService m_HQ;
	protected ref HST_PlayerLifecycleService m_PlayerLifecycle;
	protected ref HST_TownService m_Towns;
	protected ref HST_GarrisonService m_Garrisons;
	protected ref HST_RecruitmentService m_Recruitment;
	protected ref HST_ZoneCaptureService m_ZoneCapture;
	protected ref HST_PlayerSpawnService m_PlayerSpawn;
	protected ref HST_PhysicalWarService m_PhysicalWar;
	protected float m_fSecondAccumulator;
	protected float m_fSpawnSweepAccumulator;
	protected int m_iSpawnDiagnosticsRemaining;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		s_Instance = this;
		m_State = new HST_CampaignState();
		m_Preset = HST_DefaultCatalog.CreateRhsEveronPreset();
		m_Balance = HST_DefaultCatalog.CreateBalance();
		m_Economy = new HST_EconomyService();
		m_Missions = new HST_MissionService();
		m_Persistence = new HST_PersistenceService();
		m_Authorization = new HST_AuthorizationService();
		m_Strategic = new HST_StrategicService();
		m_Arsenal = new HST_ArsenalService();
		m_EnemyDirector = new HST_EnemyDirectorService();
		m_HQ = new HST_HQService();
		m_PlayerLifecycle = new HST_PlayerLifecycleService();
		m_Towns = new HST_TownService();
		m_Garrisons = new HST_GarrisonService();
		m_Recruitment = new HST_RecruitmentService();
		m_ZoneCapture = new HST_ZoneCaptureService();
		m_PlayerSpawn = new HST_PlayerSpawnService();
		m_PhysicalWar = new HST_PhysicalWarService();

		m_State.m_iFactionMoney = m_Balance.m_iStartingFactionMoney;
		m_State.m_iHR = m_Balance.m_iStartingHR;
		HST_DefaultCatalog.AddDefaultFactionPools(m_State, m_Balance, m_Preset);
		HST_DefaultCatalog.AddDefaultZones(m_State, m_Preset);
		HST_DefaultCatalog.AddDefaultGarrisons(m_State, m_Preset);
		m_HQ.SelectInitialHideout(m_State, HST_DefaultCatalog.GetDefaultHideoutId());

		m_iSpawnDiagnosticsRemaining = 12;
		SetEventMask(owner, EntityEvent.FRAME);
		Print("h-istasi | campaign coordinator initialized");
	}

	override void OnGameModeStart()
	{
		super.OnGameModeStart();
		ProcessPlayerSpawnSweep("game-mode-start", true);
	}

	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		super.OnGameStateChanged(state);
		m_iSpawnDiagnosticsRemaining = Math.Max(m_iSpawnDiagnosticsRemaining, 8);
		ProcessPlayerSpawnSweep("game-state-changed", true);
	}

	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		m_iSpawnDiagnosticsRemaining = Math.Max(m_iSpawnDiagnosticsRemaining, 8);
		ProcessPlayerSpawnSweep(string.Format("player-connected-%1", playerId), true);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer() || !m_State)
			return;

		m_PlayerSpawn.Tick(timeSlice);
		m_fSpawnSweepAccumulator += timeSlice;
		if (m_fSpawnSweepAccumulator >= 0.25)
		{
			m_fSpawnSweepAccumulator = 0;
			ProcessPlayerSpawnSweep("frame");
		}

		m_Persistence.Tick(m_State, timeSlice, m_Balance.m_iAutosaveIntervalSeconds, m_Balance.m_iMajorChangeDebounceSeconds);
		m_fSecondAccumulator += timeSlice;
		if (m_fSecondAccumulator < 1)
			return;

		int elapsedSeconds = m_fSecondAccumulator;
		m_fSecondAccumulator -= elapsedSeconds;
		m_State.m_iElapsedSeconds += elapsedSeconds;
		bool missionChanged = m_Missions.Tick(m_State, m_Preset, m_Economy, elapsedSeconds);
		int income = m_Towns.TickIncome(m_State, m_Economy, m_Balance, m_Preset, elapsedSeconds);
		bool hqRuntimeChanged = m_HQ.EnsureRuntimeObjects(m_State);
		m_PhysicalWar.UpdateZoneActivation(m_State, m_Balance);
		if (missionChanged || income > 0 || hqRuntimeChanged)
			m_Persistence.MarkMajorChange();
	}

	HST_CampaignState GetState()
	{
		return m_State;
	}

	static HST_CampaignCoordinatorComponent GetInstance()
	{
		return s_Instance;
	}

	HST_PlayerState RegisterPlayer(string identityId, bool isAdmin = false)
	{
		if (!Replication.IsServer())
			return null;

		return m_Authorization.RegisterPlayer(m_State, identityId, isAdmin);
	}

	string GetPlayerSpawnFactionKey()
	{
		if (!m_PlayerSpawn)
			return "";

		return m_PlayerSpawn.GetPrimaryPlayerFaction(m_Preset);
	}

	vector GetPlayerHQSpawnPosition()
	{
		if (!m_PlayerSpawn)
			return "0 0 0";

		return m_PlayerSpawn.GetHQSpawnPosition(m_State);
	}

	string GetDefaultPlayerPrefab()
	{
		if (!m_PlayerSpawn)
			return "";

		return m_PlayerSpawn.GetDefaultPlayerPrefab();
	}

	string GetDefaultSpawnPointPrefab()
	{
		if (!m_PlayerSpawn)
			return "";

		return m_PlayerSpawn.GetDefaultSpawnPointPrefab();
	}

	HST_PlayerState RegisterConnectedPlayer(int playerId, string identityId, bool isAdmin = false)
	{
		if (!Replication.IsServer())
			return null;

		HST_PlayerState player = m_PlayerLifecycle.RegisterConnectedPlayer(m_State, m_Authorization, playerId, identityId, isAdmin);
		if (player)
			m_Persistence.MarkMajorChange();
		return player;
	}

	bool SpawnOrRespawnPlayer(int playerId)
	{
		if (!Replication.IsServer() || !m_PlayerSpawn)
			return false;

		return m_PlayerSpawn.SpawnOrRespawnPlayer(m_State, m_Authorization, m_PlayerLifecycle, playerId);
	}

	void OnPlayerSpawned(int playerId, IEntity entity)
	{
		if (!Replication.IsServer() || !m_PlayerSpawn)
			return;

		if (m_PlayerSpawn.OnPlayerSpawned(m_State, m_Authorization, m_PlayerLifecycle, playerId, entity))
			m_Persistence.MarkMajorChange();
	}

	void OnPlayerSpawnFailed(int playerId)
	{
		if (!Replication.IsServer() || !m_PlayerSpawn)
			return;

		m_PlayerSpawn.OnPlayerSpawnFailed(playerId);
	}

	bool SetMembership(string actorIdentityId, string targetIdentityId, bool isMember)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Authorization.SetMembership(m_State, actorIdentityId, targetIdentityId, isMember);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	bool SetZoneOwner(string zoneId, string factionKey)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Strategic.SetZoneOwner(m_State, m_Economy, m_Balance, zoneId, factionKey);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	bool CaptureZoneForResistance(string zoneId, int supportReward = 10)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_ZoneCapture.CaptureForResistance(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, zoneId, supportReward);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	HST_ArsenalItemState DepositArsenalItem(string prefab, int amount)
	{
		if (!Replication.IsServer())
			return null;

		HST_ArsenalItemState item = m_Arsenal.DepositItem(m_State, m_Balance, prefab, amount);
		if (item)
			m_Persistence.MarkMajorChange();
		return item;
	}

	bool RequestManualCheckpoint()
	{
		if (!Replication.IsServer())
			return false;

		return m_Persistence.RequestCheckpoint("h-istasi manual checkpoint", m_State);
	}

	bool SelectInitialHideout(string hideoutId)
	{
		if (!Replication.IsServer())
			return false;

		return SelectInitialHideout_S(hideoutId);
	}

	bool StartMission(string missionId, string targetZoneId = "")
	{
		if (!Replication.IsServer())
			return false;

		return StartMission_S(missionId, targetZoneId);
	}

	bool CompleteMission(string instanceId)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Missions.Complete(m_State, m_Economy, instanceId);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	bool FailMission(string instanceId)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Missions.Fail(m_State, m_Preset, m_Economy, instanceId);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	bool MoveHQ(string hideoutId)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_HQ.MoveHQ(m_State, hideoutId);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	void OnPetrosKilled()
	{
		if (!Replication.IsServer())
			return;

		m_HQ.OnPetrosKilled(m_State, m_Economy, 250, 5);
		m_Persistence.MarkMajorChange();
	}

	bool AddTownSupport(string zoneId, int amount)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Towns.AddSupport(m_State, zoneId, amount);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	int ApplyIncomeNow()
	{
		if (!Replication.IsServer())
			return 0;

		int income = m_Towns.ApplyIncomeNow(m_State, m_Economy, m_Preset);
		if (income > 0)
			m_Persistence.MarkMajorChange();
		return income;
	}

	bool AddAbstractGarrison(string zoneId, string factionKey, int infantryCount, int vehicleCount = 0)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Garrisons.AddAbstractForces(m_State, zoneId, factionKey, infantryCount, vehicleCount);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	bool FoldGarrisonSurvivors(string zoneId, string factionKey, int infantryCount, int vehicleCount = 0)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Garrisons.FoldSurvivors(m_State, zoneId, factionKey, infantryCount, vehicleCount);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	bool TrainTroops(int moneyCost = 250)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Recruitment.TrainTroops(m_State, m_Economy, moneyCost);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	bool RecruitResistanceGarrison(string zoneId, int infantryCount, int vehicleCount = 0, int moneyCost = 100, int hrCost = 1)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Recruitment.RecruitGarrison(m_State, m_Economy, m_Garrisons, zoneId, m_Preset.m_sResistanceFactionKey, infantryCount, vehicleCount, moneyCost, hrCost);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	void AwardFactionResources(int money, int hr)
	{
		if (!Replication.IsServer())
			return;

		m_Economy.AddFactionMoney(m_State, money);
		m_Economy.AddHR(m_State, hr);
		m_Persistence.MarkMajorChange();
	}

	bool AwardPlayerResources(string identityId, int money, int rank)
	{
		if (!Replication.IsServer())
			return false;

		bool changedMoney = m_PlayerLifecycle.AddPersonalMoney(m_State, identityId, money);
		bool changedRank = m_PlayerLifecycle.AddRank(m_State, identityId, rank);
		if (changedMoney || changedRank)
			m_Persistence.MarkMajorChange();
		return changedMoney || changedRank;
	}

	protected bool SelectInitialHideout_S(string hideoutId)
	{
		bool changed = m_HQ.SelectInitialHideout(m_State, hideoutId);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	protected bool StartMission_S(string missionId, string targetZoneId)
	{
		if (!m_State || !m_Missions.Start(m_State, m_Preset, missionId, targetZoneId))
			return false;

		m_Persistence.MarkMajorChange();
		return true;
	}

	protected int ProcessPlayerSpawnSweep(string reason = "", bool forceDiagnostics = false)
	{
		if (!Replication.IsServer() || !m_State || !m_PlayerSpawn)
			return 0;

		bool diagnostics = forceDiagnostics || m_iSpawnDiagnosticsRemaining > 0;
		if (diagnostics && !reason.IsEmpty())
			Print("h-istasi | FIA spawn sweep triggered by " + reason);

		int spawnRequests = m_PlayerSpawn.SpawnMissingConnectedPlayers(m_State, m_Authorization, m_PlayerLifecycle, diagnostics);
		if (diagnostics && m_iSpawnDiagnosticsRemaining > 0)
			m_iSpawnDiagnosticsRemaining--;

		return spawnRequests;
	}
}
