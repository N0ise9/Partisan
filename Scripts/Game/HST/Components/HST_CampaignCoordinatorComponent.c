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
	protected ref HST_RuntimeSettingsService m_SettingsService;
	protected ref HST_RuntimeSettings m_Settings;
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
	protected ref HST_MapMarkerService m_MapMarkers;
	protected ref HST_CommandUIService m_CommandUI;
	protected ref HST_LootService m_Loot;
	protected float m_fSecondAccumulator;
	protected float m_fSpawnSweepAccumulator;
	protected int m_iSpawnDiagnosticsRemaining;
	protected bool m_bSpawnSweepArmed;
	protected int m_iStableSpawnSweepCount;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		s_Instance = this;
		m_State = new HST_CampaignState();
		m_Preset = HST_DefaultCatalog.CreateRhsEveronPreset();
		m_Balance = HST_DefaultCatalog.CreateBalance();
		m_SettingsService = new HST_RuntimeSettingsService();
		m_Settings = m_SettingsService.LoadOrCreate();
		if (m_Settings)
			m_Settings.ApplyTo(m_Preset, m_Balance);

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
		m_MapMarkers = new HST_MapMarkerService();
		m_CommandUI = new HST_CommandUIService();
		m_Loot = new HST_LootService();

		m_State.m_sPresetId = m_Preset.m_sPresetId;
		if (m_Settings)
			m_State.m_iCampaignSeed = m_Settings.m_Campaign.m_iCampaignSeed;
		m_State.m_iFactionMoney = m_Balance.m_iStartingFactionMoney;
		m_State.m_iHR = m_Balance.m_iStartingHR;
		HST_DefaultCatalog.AddDefaultFactionPools(m_State, m_Balance, m_Preset);
		HST_DefaultCatalog.AddDefaultZones(m_State, m_Preset);
		HST_DefaultCatalog.AddDefaultGarrisons(m_State, m_Preset);
		if (m_Settings && HST_DefaultCatalog.IsKnownHideout(m_Settings.m_Campaign.m_sDefaultHideoutId))
			m_HQ.SelectInitialHideout(m_State, m_Settings.m_Campaign.m_sDefaultHideoutId);
		else
			m_HQ.SelectInitialHideout(m_State, HST_DefaultCatalog.GetDefaultHideoutId());
		RefreshCampaignMarkers();

		ArmPlayerSpawnSweep(6);
		SetEventMask(owner, EntityEvent.FRAME);
		Print("h-istasi | campaign coordinator initialized");
	}

	override void OnGameModeStart()
	{
		super.OnGameModeStart();
		ArmPlayerSpawnSweep(4);
		ProcessPlayerSpawnSweep("game-mode-start", true);
	}

	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		super.OnGameStateChanged(state);
		ArmPlayerSpawnSweep(4);
		ProcessPlayerSpawnSweep("game-state-changed", true);
	}

	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		ArmPlayerSpawnSweep(4);
		ProcessPlayerSpawnSweep(string.Format("player-connected-%1", playerId), true);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer() || !m_State)
			return;

		m_PlayerSpawn.Tick(timeSlice);
		m_fSpawnSweepAccumulator += timeSlice;
		if (m_fSpawnSweepAccumulator >= 0.25 && ShouldProcessFrameSpawnSweep())
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
		bool enemyResourcesChanged = m_EnemyDirector.TickResources(m_State, m_Preset, elapsedSeconds);
		bool hqRuntimeChanged = m_HQ.EnsureRuntimeObjects(m_State);
		bool physicalWarChanged = m_PhysicalWar.UpdateZoneActivation(m_State, m_Balance, m_EnemyDirector);
		if (missionChanged || income > 0 || enemyResourcesChanged || hqRuntimeChanged || physicalWarChanged)
			MarkMajorCampaignChange();
	}

	HST_CampaignState GetState()
	{
		return m_State;
	}

	string GetAlphaMemberMenu(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_CommandUI)
			return "";

		return m_CommandUI.BuildMemberMenu(m_State, m_Preset, m_MapMarkers);
	}

	string GetAlphaCommanderMenu(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId) || !m_CommandUI)
			return "";

		return m_CommandUI.BuildCommanderMenu(m_State, m_Preset, m_MapMarkers);
	}

	string GetAlphaAdminMenu(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId) || !m_CommandUI)
			return "";

		return m_CommandUI.BuildAdminMenu(m_State, m_Preset, m_MapMarkers);
	}

	bool RequestAlphaUICommand(int playerId, string commandId, string argument = "")
	{
		if (!Replication.IsServer() || !m_CommandUI)
			return false;

		return m_CommandUI.ExecuteQuickCommand(this, playerId, commandId, argument);
	}

	string BuildVisibleMenuPayload(int playerId, string selectedTabId, string lastResult = "")
	{
		if (!Replication.IsServer() || !m_CommandUI)
			return "HST_MENU|offline|0\nSTATUS|h-istasi menu | server coordinator not ready\nEND";

		return m_CommandUI.BuildVisibleMenuPayload(m_State, m_Preset, m_MapMarkers, m_Arsenal, m_Settings, playerId, selectedTabId, lastResult, CanPlayerUseMemberActions(playerId), CanPlayerUseCommanderActions(playerId), CanPlayerUseAdminActions(playerId));
	}

	string RequestVisibleMenuCommand(int playerId, string selectedTabId, string commandId, string argument = "")
	{
		if (!Replication.IsServer() || !m_CommandUI)
			return "h-istasi command | server coordinator not ready";

		return m_CommandUI.ExecuteVisibleCommand(this, playerId, commandId, argument);
	}

	int ResolveAuthoritativePlayerId(IEntity requestOwner)
	{
		if (!Replication.IsServer() || !requestOwner)
			return 0;

		PlayerController controller = PlayerController.Cast(requestOwner);
		if (controller && controller.GetPlayerId() > 0)
			return controller.GetPlayerId();

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		int controlledPlayerId = playerManager.GetPlayerIdFromControlledEntity(requestOwner);
		if (controlledPlayerId > 0)
			return controlledPlayerId;

		BaseRplComponent rpl = BaseRplComponent.Cast(requestOwner.FindComponent(BaseRplComponent));
		if (rpl)
			return playerManager.GetPlayerIdFromEntityRplId(rpl.Id());

		return 0;
	}

	static HST_CampaignCoordinatorComponent GetInstance()
	{
		return s_Instance;
	}

	HST_PlayerState RegisterPlayer(string identityId, bool isAdmin = false)
	{
		if (!Replication.IsServer())
			return null;

		HST_PlayerState player = m_Authorization.RegisterPlayer(m_State, identityId, isAdmin || IsSettingsAdminIdentity(identityId));
		ApplyRuntimeMembershipDefaults(player);
		return player;
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

		string resolvedIdentityId = m_PlayerLifecycle.ResolveIdentityId(playerId, identityId);
		HST_PlayerState player = m_PlayerLifecycle.RegisterConnectedPlayer(m_State, m_Authorization, playerId, identityId, isAdmin || IsSettingsAdminIdentity(resolvedIdentityId));
		ApplyRuntimeMembershipDefaults(player);
		if (player)
			MarkMajorCampaignChange();
		return player;
	}

	bool SpawnOrRespawnPlayer(int playerId)
	{
		if (!Replication.IsServer() || !m_PlayerSpawn)
			return false;

		bool requested = m_PlayerSpawn.SpawnOrRespawnPlayer(m_State, m_Authorization, m_PlayerLifecycle, playerId);
		if (requested)
			ArmPlayerSpawnSweep(2);

		return requested;
	}

	void OnPlayerSpawned(int playerId, IEntity entity)
	{
		if (!Replication.IsServer() || !m_PlayerSpawn)
			return;

		if (m_PlayerSpawn.OnPlayerSpawned(m_State, m_Authorization, m_PlayerLifecycle, playerId, entity))
			MarkMajorCampaignChange();
	}

	void OnPlayerSpawnFailed(int playerId)
	{
		if (!Replication.IsServer() || !m_PlayerSpawn)
			return;

		m_PlayerSpawn.OnPlayerSpawnFailed(playerId);
		ArmPlayerSpawnSweep(4);
	}

	bool SetMembership(string actorIdentityId, string targetIdentityId, bool isMember)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Authorization.SetMembership(m_State, actorIdentityId, targetIdentityId, isMember);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	bool SetZoneOwner(string zoneId, string factionKey)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Strategic.SetZoneOwner(m_State, m_Economy, m_Balance, zoneId, factionKey);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	bool CaptureZoneForResistance(string zoneId, int supportReward = 10)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_ZoneCapture.CaptureForResistance(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, zoneId, supportReward);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	HST_ArsenalItemState DepositArsenalItem(string prefab, int amount, string category = "equipment", string displayName = "")
	{
		if (!Replication.IsServer())
			return null;

		HST_ArsenalItemState item = m_Arsenal.DepositItem(m_State, m_Balance, prefab, amount, category, displayName);
		if (item)
			MarkMajorCampaignChange();
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

		HST_ActiveMissionState activeMission = m_State.FindActiveMission(instanceId);
		HST_MissionDefinition definition;
		if (activeMission)
			definition = m_Missions.FindDefinition(activeMission.m_sMissionId);

		bool changed = m_Missions.Complete(m_State, m_Economy, instanceId);
		if (changed && ApplyCompletedMissionOutcome(definition, activeMission))
			changed = true;

		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	bool FailMission(string instanceId)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Missions.Fail(m_State, m_Preset, m_Economy, instanceId);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	bool MoveHQ(string hideoutId)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_HQ.MoveHQ(m_State, hideoutId);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	bool MoveHQToPlayer(int playerId)
	{
		if (!Replication.IsServer() || !m_HQ || playerId <= 0)
			return false;

		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		if (!playerEntity)
			return false;

		vector hqPosition = HST_WorldPositionService.ResolveGroundPosition(playerEntity.GetOrigin(), HST_WorldPositionService.HQ_GROUND_OFFSET, true);
		bool changed = m_HQ.MoveHQToPosition(m_State, hqPosition, "field_hq");
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	void OnPetrosKilled()
	{
		if (!Replication.IsServer())
			return;

		m_HQ.OnPetrosKilled(m_State, m_Economy, 250, 5);
		MarkMajorCampaignChange();
	}

	bool AddTownSupport(string zoneId, int amount)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Towns.AddSupport(m_State, zoneId, amount);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	int ApplyIncomeNow()
	{
		if (!Replication.IsServer())
			return 0;

		int income = m_Towns.ApplyIncomeNow(m_State, m_Economy, m_Preset);
		if (income > 0)
			MarkMajorCampaignChange();
		return income;
	}

	bool AddAbstractGarrison(string zoneId, string factionKey, int infantryCount, int vehicleCount = 0)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Garrisons.AddAbstractForces(m_State, zoneId, factionKey, infantryCount, vehicleCount);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	bool FoldGarrisonSurvivors(string zoneId, string factionKey, int infantryCount, int vehicleCount = 0)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Garrisons.FoldSurvivors(m_State, zoneId, factionKey, infantryCount, vehicleCount);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	bool TrainTroops(int moneyCost = 250)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Recruitment.TrainTroops(m_State, m_Economy, moneyCost);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	bool RecruitResistanceGarrison(string zoneId, int infantryCount, int vehicleCount = 0, int moneyCost = 100, int hrCost = 1)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Recruitment.RecruitGarrison(m_State, m_Economy, m_Garrisons, zoneId, m_Preset.m_sResistanceFactionKey, infantryCount, vehicleCount, moneyCost, hrCost);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	void AwardFactionResources(int money, int hr)
	{
		if (!Replication.IsServer())
			return;

		m_Economy.AddFactionMoney(m_State, money);
		m_Economy.AddHR(m_State, hr);
		MarkMajorCampaignChange();
	}

	bool AwardPlayerResources(string identityId, int money, int rank)
	{
		if (!Replication.IsServer())
			return false;

		bool changedMoney = m_PlayerLifecycle.AddPersonalMoney(m_State, identityId, money);
		bool changedRank = m_PlayerLifecycle.AddRank(m_State, identityId, rank);
		if (changedMoney || changedRank)
			MarkMajorCampaignChange();
		return changedMoney || changedRank;
	}

	bool RequestCommanderMoveHQ(int playerId, string hideoutId)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId))
			return false;

		return MoveHQ(hideoutId);
	}

	bool RequestCommanderMoveHQToPlayer(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId))
			return false;

		return MoveHQToPlayer(playerId);
	}

	bool RequestCommanderStartMission(int playerId, string missionId, string targetZoneId = "")
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId))
			return false;

		return StartMission(missionId, targetZoneId);
	}

	bool RequestCommanderRecruitGarrison(int playerId, string zoneId, int infantryCount, int vehicleCount = 0, int moneyCost = 100, int hrCost = 1)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId))
			return false;

		return RecruitResistanceGarrison(zoneId, infantryCount, vehicleCount, moneyCost, hrCost);
	}

	bool RequestCommanderTrainTroops(int playerId, int moneyCost = 250)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId))
			return false;

		return TrainTroops(moneyCost);
	}

	bool RequestCommanderApplyIncomeNow(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId))
			return false;

		return ApplyIncomeNow() > 0;
	}

	bool RequestCommanderStartZoneMission(int playerId, string zoneId)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId))
			return false;

		HST_ZoneState zone = m_State.FindZone(zoneId);
		if (!zone)
			return false;

		return StartMission(SelectDefaultMissionForZone(zone), zoneId);
	}

	bool RequestCommanderCompleteMission(int playerId, string instanceId)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId))
			return false;

		return CompleteMission(instanceId);
	}

	bool RequestCommanderDepositArsenalItem(int playerId, string prefab, int amount)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId))
			return false;

		return DepositArsenalItem(prefab, amount) != null;
	}

	bool RequestCommanderStoreGarageVehicle(int playerId, string vehicleId, string prefab, vector position, vector angles, float fuel = 1, bool armed = false)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId) || vehicleId.IsEmpty() || prefab.IsEmpty())
			return false;

		HST_GarageVehicleState vehicle = new HST_GarageVehicleState();
		vehicle.m_sVehicleId = vehicleId;
		vehicle.m_sPrefab = prefab;
		vehicle.m_vPosition = position;
		vehicle.m_vAngles = angles;
		vehicle.m_fFuel = Math.Max(0.0, Math.Min(1.0, fuel));
		vehicle.m_bArmed = armed;
		bool changed = m_Arsenal.StoreVehicle(m_State, vehicle);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	bool RequestMemberManualCheckpoint(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return false;

		return RequestManualCheckpoint();
	}

	string RequestMemberInspectCampaign(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "";

		return BuildCampaignReport();
	}

	string RequestMemberInspectMarkers(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_MapMarkers)
			return "";

		return m_MapMarkers.BuildMarkerReport(m_State);
	}

	string RequestMemberInspectEconomy(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_CommandUI)
			return "";

		return m_CommandUI.BuildEconomyReport(m_State);
	}

	string RequestMemberInspectZones(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_CommandUI)
			return "";

		return m_CommandUI.BuildZoneListReport(m_State, m_Preset);
	}

	string RequestMemberInspectMissions(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_CommandUI)
			return "";

		return m_CommandUI.BuildMissionReport(m_State);
	}

	string RequestMemberInspectArsenal(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_Arsenal)
			return "";

		return m_Arsenal.BuildArsenalReport(m_State);
	}

	string RequestMemberLootNearby(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loot | membership required";

		if (!m_Settings || !m_Settings.m_Features.m_bAreaLootEnabled)
			return "h-istasi loot | area loot disabled by config";

		if (!m_Loot || !m_Arsenal)
			return "h-istasi loot | service not ready";

		HST_LootResult result = m_Loot.LootNearbyToArsenal(m_State, m_Preset, m_Balance, m_Arsenal, playerId);
		if (result && result.m_iItemsDeposited > 0)
			MarkMajorCampaignChange();

		if (!result)
			return "h-istasi loot | no result";

		return result.BuildSummary();
	}

	bool RequestAdminSetZoneActive(int playerId, string zoneId, bool active)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return false;

		bool changed = m_Strategic.SetZoneActive(m_State, zoneId, active);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	bool RequestAdminCaptureZone(int playerId, string zoneId, string factionKey)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return false;

		return SetZoneOwner(zoneId, factionKey);
	}

	bool RequestAdminCaptureZoneForResistance(int playerId, string zoneId, int supportReward = 10)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return false;

		return CaptureZoneForResistance(zoneId, supportReward);
	}

	bool RequestAdminAddCaptureProgress(int playerId, string zoneId, int progress = 50)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return false;

		bool changed = m_ZoneCapture.AddResistanceCaptureProgress(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, zoneId, progress, 10);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	bool RequestAdminStartDebugMission(int playerId, string zoneId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return false;

		HST_ZoneState zone = m_State.FindZone(zoneId);
		if (!zone)
			return StartMission("dynamic_minor_city_task", "");

		return StartMission(SelectDefaultMissionForZone(zone), zoneId);
	}

	bool RequestAdminCompleteMission(int playerId, string instanceId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return false;

		return CompleteMission(instanceId);
	}

	bool RequestAdminFailMission(int playerId, string instanceId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return false;

		return FailMission(instanceId);
	}

	bool RequestAdminAwardResources(int playerId, int money, int hr)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return false;

		AwardFactionResources(money, hr);
		return true;
	}

	bool RequestAdminAddEnemyResources(int playerId, string factionKey, int attackResources, int supportResources)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return false;

		m_EnemyDirector.AddResources(m_State, factionKey, attackResources, supportResources);
		MarkMajorCampaignChange();
		return true;
	}

	string RequestAdminInspectZone(int playerId, string zoneId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "";

		return BuildZoneReport(zoneId);
	}

	protected bool SelectInitialHideout_S(string hideoutId)
	{
		bool changed = m_HQ.SelectInitialHideout(m_State, hideoutId);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	protected bool StartMission_S(string missionId, string targetZoneId)
	{
		if (!m_State || !m_Missions.Start(m_State, m_Preset, missionId, targetZoneId))
			return false;

		MarkMajorCampaignChange();
		return true;
	}

	protected void MarkMajorCampaignChange(bool refreshMarkers = true)
	{
		if (refreshMarkers)
			RefreshCampaignMarkers();

		if (m_Persistence)
			m_Persistence.MarkMajorChange();
	}

	protected void RefreshCampaignMarkers()
	{
		if (m_MapMarkers)
			m_MapMarkers.RebuildAllMarkers(m_State, m_Preset);
	}

	protected string ResolveTrustedIdentityId(int playerId)
	{
		if (!m_PlayerLifecycle || playerId <= 0)
			return "";

		return m_PlayerLifecycle.ResolveIdentityId(playerId, "");
	}

	protected bool CanPlayerUseCommanderActions(int playerId)
	{
		if (!m_Authorization)
			return false;

		return m_Authorization.CanUseCommanderActions(m_State, ResolveTrustedIdentityId(playerId));
	}

	protected bool CanPlayerUseMemberActions(int playerId)
	{
		if (m_Settings && !m_Settings.m_Membership.m_bMembershipEnabled)
			return true;

		string identityId = ResolveTrustedIdentityId(playerId);
		HST_PlayerState player = m_State.FindPlayer(identityId);
		return player && player.m_bMember;
	}

	protected bool CanPlayerUseAdminActions(int playerId)
	{
		if (!m_Authorization)
			return false;

		return m_Authorization.CanUseAdminActions(m_State, ResolveTrustedIdentityId(playerId));
	}

	protected bool IsSettingsAdminIdentity(string identityId)
	{
		if (!m_Settings || !m_Settings.m_Membership || identityId.IsEmpty())
			return false;

		return m_Settings.m_Membership.m_aAdminIdentityIds.Contains(identityId);
	}

	protected void ApplyRuntimeMembershipDefaults(HST_PlayerState player)
	{
		if (!player || !m_Settings)
			return;

		if (!m_Settings.m_Membership.m_bMembershipEnabled)
		{
			player.m_bMember = true;
			player.m_bGuest = false;
		}

		if (IsSettingsAdminIdentity(player.m_sIdentityId))
			player.m_bAdmin = true;
	}

	protected IEntity ResolveControlledPlayerEntity(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || playerId <= 0)
			return null;

		IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
		if (controlledEntity)
			return controlledEntity;

		return SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
	}

	protected bool ApplyCompletedMissionOutcome(HST_MissionDefinition definition, HST_ActiveMissionState activeMission)
	{
		if (!definition || !activeMission || activeMission.m_sTargetZoneId.IsEmpty())
			return false;

		HST_ZoneState zone = m_State.FindZone(activeMission.m_sTargetZoneId);
		if (!zone)
			return false;

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return m_ZoneCapture.AddResistanceCaptureProgress(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, zone.m_sZoneId, 60, 15);

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			return m_Towns.AddSupport(m_State, zone.m_sZoneId, 25);

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
		{
			m_EnemyDirector.AddResources(m_State, zone.m_sOwnerFactionKey, -12, -6);
			if (zone.m_sOwnerFactionKey != m_Preset.m_sResistanceFactionKey)
				m_ZoneCapture.AddResistanceCaptureProgress(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, zone.m_sZoneId, 35, 10);
			return true;
		}

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY || definition.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS)
		{
			AwardFactionResources(150, 1);
			m_EnemyDirector.AddResources(m_State, zone.m_sOwnerFactionKey, -8, -4);
			return true;
		}

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DYNAMIC)
		{
			if (definition.m_sMissionId == "dynamic_city_flip_battle")
				return m_ZoneCapture.AddResistanceCaptureProgress(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, zone.m_sZoneId, 50, 10);

			if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
				return m_Towns.AddSupport(m_State, zone.m_sZoneId, 10);

			return m_ZoneCapture.AddResistanceCaptureProgress(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, zone.m_sZoneId, 20, 5);
		}

		return false;
	}

	protected string SelectDefaultMissionForZone(HST_ZoneState zone)
	{
		if (!zone)
			return "dynamic_minor_city_task";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			return "conquest_resource";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST)
			return "conquest_outpost";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return "destroy_radio_tower";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT || zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			return "destroy_or_steal_armor";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN && zone.m_sOwnerFactionKey == m_Preset.m_sResistanceFactionKey)
			return "support_city_supplies";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			return "dynamic_city_flip_battle";

		return "dynamic_minor_city_task";
	}

	protected string BuildCampaignReport()
	{
		int resistanceZones;
		int enemyZones;
		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (zone.m_sOwnerFactionKey == m_Preset.m_sResistanceFactionKey)
				resistanceZones++;
			else
				enemyZones++;
		}

		string economySummary = string.Format("h-istasi campaign | money %1 | HR %2 | war level %3 | training %4", m_State.m_iFactionMoney, m_State.m_iHR, m_State.m_iWarLevel, m_State.m_iTrainingLevel);
		string zoneSummary = string.Format(" | resistance zones %1 | enemy zones %2 | active missions %3", resistanceZones, enemyZones, m_State.m_aActiveMissions.Count());
		string runtimeSummary = string.Format(" | active groups %1 | QRFs %2 | markers %3 | HQ %4", m_State.m_aActiveGroups.Count(), m_State.m_aQRFs.Count(), m_State.m_aMapMarkers.Count(), m_State.m_sHQHideoutId);
		return economySummary + zoneSummary + runtimeSummary;
	}

	protected string BuildZoneReport(string zoneId)
	{
		HST_ZoneState zone = m_State.FindZone(zoneId);
		if (!zone)
			return "h-istasi zone report | zone not found";

		HST_GarrisonState ownerGarrison = m_State.FindGarrison(zone.m_sZoneId, zone.m_sOwnerFactionKey);
		int infantry;
		int vehicles;
		if (ownerGarrison)
		{
			infantry = ownerGarrison.m_iInfantryCount;
			vehicles = ownerGarrison.m_iVehicleCount;
		}

		string zoneSummary = string.Format("h-istasi zone %1 | owner %2 | type %3 | support %4", zone.m_sZoneId, zone.m_sOwnerFactionKey, ZoneTypeToLabel(zone.m_eType), zone.m_iSupport);
		string captureSummary = string.Format(" | capture %1/%2 | income %3", zone.m_iResistanceCaptureProgress, HST_ZoneCaptureService.CAPTURE_PROGRESS_REQUIRED, zone.m_iIncomeValue);
		string forceSummary = string.Format(" | garrison %1 infantry/%2 vehicles | active %3/%4", infantry, vehicles, zone.m_iActiveInfantryCount, zone.m_iActiveVehicleCount);
		string qrfSummary = string.Format(" | QRF cooldown %1", zone.m_iQrfCooldownUntilSecond);
		return zoneSummary + captureSummary + forceSummary + qrfSummary;
	}

	protected string ZoneTypeToLabel(HST_EZoneType zoneType)
	{
		if (zoneType == HST_EZoneType.HST_ZONE_TOWN)
			return "town";

		if (zoneType == HST_EZoneType.HST_ZONE_OUTPOST)
			return "outpost";

		if (zoneType == HST_EZoneType.HST_ZONE_RESOURCE)
			return "resource";

		if (zoneType == HST_EZoneType.HST_ZONE_FACTORY)
			return "factory";

		if (zoneType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return "radio";

		if (zoneType == HST_EZoneType.HST_ZONE_AIRFIELD)
			return "airfield";

		if (zoneType == HST_EZoneType.HST_ZONE_SEAPORT)
			return "seaport";

		if (zoneType == HST_EZoneType.HST_ZONE_HIDEOUT)
			return "hideout";

		return "zone";
	}

	protected int ProcessPlayerSpawnSweep(string reason = "", bool forceDiagnostics = false)
	{
		if (!Replication.IsServer() || !m_State || !m_PlayerSpawn)
			return 0;

		bool isFrameSweep = reason == "frame";
		bool diagnostics = forceDiagnostics || (!isFrameSweep && m_iSpawnDiagnosticsRemaining > 0);
		if (diagnostics && !reason.IsEmpty())
			Print("h-istasi | FIA spawn sweep triggered by " + reason);

		int spawnRequests = m_PlayerSpawn.SpawnMissingConnectedPlayers(m_State, m_Authorization, m_PlayerLifecycle, diagnostics);
		if (diagnostics && m_iSpawnDiagnosticsRemaining > 0)
			m_iSpawnDiagnosticsRemaining--;

		UpdateSpawnSweepArmedState();
		return spawnRequests;
	}

	protected void ArmPlayerSpawnSweep(int diagnosticsBudget = 0)
	{
		m_bSpawnSweepArmed = true;
		m_iStableSpawnSweepCount = 0;
		if (diagnosticsBudget > 0)
			m_iSpawnDiagnosticsRemaining = Math.Max(m_iSpawnDiagnosticsRemaining, diagnosticsBudget);
	}

	protected bool ShouldProcessFrameSpawnSweep()
	{
		if (!m_PlayerSpawn)
			return false;

		return m_bSpawnSweepArmed || m_PlayerSpawn.NeedsSpawnSweep(m_State);
	}

	protected void UpdateSpawnSweepArmedState()
	{
		if (!m_PlayerSpawn)
			return;

		if (!m_PlayerSpawn.AreConnectedPlayersSpawnStable(m_State))
		{
			m_bSpawnSweepArmed = true;
			m_iStableSpawnSweepCount = 0;
			return;
		}

		m_iStableSpawnSweepCount++;
		if (m_iStableSpawnSweepCount >= 2)
			m_bSpawnSweepArmed = false;
	}
}
