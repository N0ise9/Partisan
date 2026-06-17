[ComponentEditorProps(category: "h-istasi", description: "Server-authoritative h-istasi campaign coordinator")]
class HST_CampaignCoordinatorComponentClass : SCR_BaseGameModeComponentClass
{
}

class HST_CampaignCoordinatorComponent : SCR_BaseGameModeComponent
{
	protected static HST_CampaignCoordinatorComponent s_Instance;
	static const int MARKER_REFRESH_THROTTLE_SECONDS = 10;
	static const string PERSISTENCE_SMOKE_PREFIX = "hst_smoke";
	static const string PHASE14_FINITE_PREFAB = "{6985327711303750}Prefabs/Objects/HST/HST_MissionProp_CitySupplies.et";
	static const string PHASE14_THRESHOLD_PREFAB = "{6985327711303760}Prefabs/Objects/HST/HST_MissionProp_ConvoyPayload.et";
	static const string PHASE14_BLOCKED_PREFAB = "{6985327711303710}Prefabs/Objects/HST/HST_MissionProp_DestroyTarget.et";
	static const string PHASE14_RAW_ASSET_PREFAB = "{EAE920BF596EBC07}Assets/Objects/Plane.xob";

	protected ref HST_CampaignState m_State;
	protected ref HST_CampaignPreset m_Preset;
	protected ref HST_BalanceConfig m_Balance;
	protected ref HST_RuntimeSettingsService m_SettingsService;
	protected ref HST_RuntimeSettings m_Settings;
	protected ref HST_EconomyService m_Economy;
	protected ref HST_MissionService m_Missions;
	protected ref HST_PersistenceService m_Persistence;
	protected ref HST_PersistenceSmokeTestService m_PersistenceSmokeTest;
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
	protected ref HST_ZoneCompositionService m_ZoneCompositions;
	protected ref HST_MapMarkerService m_MapMarkers;
	protected ref HST_CommandUIService m_CommandUI;
	protected ref HST_LootService m_Loot;
	protected ref HST_BuildModeService m_BuildMode;
	protected ref HST_LoadoutEditorService m_LoadoutEditor;
	protected ref HST_GeneratedContentService m_Content;
	protected ref HST_MissionObjectiveService m_Objectives;
	protected ref HST_MissionRuntimeService m_MissionRuntime;
	protected ref HST_ConvoyOutcomeService m_ConvoyOutcomes;
	protected ref HST_SupportRequestService m_SupportRequests;
	protected ref HST_CivilianService m_Civilians;
	protected ref HST_EnemyCommanderService m_EnemyCommander;
	protected float m_fSecondAccumulator;
	protected float m_fSpawnSweepAccumulator;
	protected int m_iLastMarkerRefreshSecond = -999999;
	protected int m_iSpawnDiagnosticsRemaining;
	protected bool m_bSpawnSweepArmed;
	protected int m_iStableSpawnSweepCount;
	protected bool m_bDeferredMarkerRefresh;
	protected bool m_bPersistentFieldVehicleRestoreChecked;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		s_Instance = this;
		m_Preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		m_Balance = HST_DefaultCatalog.CreateBalance();
		m_SettingsService = new HST_RuntimeSettingsService();
		m_Settings = m_SettingsService.LoadOrCreate();
		if (m_Settings)
			m_Settings.ApplyTo(m_Preset, m_Balance);

		m_Economy = new HST_EconomyService();
		m_Missions = new HST_MissionService();
		m_Persistence = new HST_PersistenceService();
		m_PersistenceSmokeTest = new HST_PersistenceSmokeTestService();
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
		m_ZoneCompositions = new HST_ZoneCompositionService();
		m_MapMarkers = new HST_MapMarkerService();
		m_CommandUI = new HST_CommandUIService();
		m_Loot = new HST_LootService();
		m_BuildMode = new HST_BuildModeService();
		m_LoadoutEditor = new HST_LoadoutEditorService();
		m_Content = new HST_GeneratedContentService();
		m_Objectives = new HST_MissionObjectiveService();
		m_MissionRuntime = new HST_MissionRuntimeService();
		m_ConvoyOutcomes = new HST_ConvoyOutcomeService();
		m_SupportRequests = new HST_SupportRequestService();
		m_Civilians = new HST_CivilianService();
		m_EnemyCommander = new HST_EnemyCommanderService();

		m_State = m_Persistence.RestoreOrCreateCampaignState(CreateInitialCampaignState());
		EnsureCampaignFoundation();
		m_Missions.SyncNextInstanceIdFromState(m_State);
		m_Persistence.CaptureAndTrackState(m_State);
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
		if (m_MapMarkers)
			m_MapMarkers.TickNativePublish(m_State, m_Preset, timeSlice);
		if (!m_bPersistentFieldVehicleRestoreChecked && GetGame().GetWorld())
		{
			m_bPersistentFieldVehicleRestoreChecked = true;
			if (m_Loot && m_State.m_bRestoredFromPersistence)
			{
				int restoredFieldVehicles = m_Loot.RestorePersistentFieldVehicles(m_State);
				if (restoredFieldVehicles > 0)
				{
					Print(string.Format("h-istasi vehicle persistence | restored %1 field vehicle(s) after campaign load", restoredFieldVehicles));
					MarkMajorCampaignChange(false);
				}
			}
		}

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
		if (missionChanged)
			BroadcastPendingMissionOutcomeEvents();
		bool objectiveChanged = m_Objectives.Tick(m_State);
		bool missionRuntimeChanged = m_MissionRuntime.Tick(m_State, m_Preset, m_Objectives, elapsedSeconds);
		bool convoyRuntimeChanged = m_PhysicalWar.UpdateMissionConvoys(m_State, m_Preset, m_Balance, elapsedSeconds);
		bool convoyOutcomeChanged = ApplyConvoyOutcomesNow();
		if (convoyRuntimeChanged)
			BroadcastPendingMissionRuntimeEvents();
		string completedRuntimeMissionId = m_MissionRuntime.FindCompletedActiveMissionId(m_State, m_Objectives);
		if (!completedRuntimeMissionId.IsEmpty())
			missionRuntimeChanged = CompleteMission(completedRuntimeMissionId) || missionRuntimeChanged || convoyRuntimeChanged || convoyOutcomeChanged;
		string failedRuntimeMissionId = m_MissionRuntime.FindFailedActiveMissionId(m_State);
		if (!failedRuntimeMissionId.IsEmpty())
			missionRuntimeChanged = FailMission(failedRuntimeMissionId) || missionRuntimeChanged || convoyRuntimeChanged || convoyOutcomeChanged;
		int income = m_Towns.TickIncome(m_State, m_Economy, m_Balance, m_Preset, elapsedSeconds);
		bool enemyResourcesChanged = m_EnemyDirector.TickResources(m_State, m_Preset, elapsedSeconds);
		bool aggressionChanged = m_Economy.TickAggressionDecay(m_State, m_Preset, elapsedSeconds);
		bool civilianChanged = m_Civilians.Tick(m_State, elapsedSeconds);
		bool supportChanged = m_SupportRequests.Tick(m_State, m_Preset, m_Garrisons);
		bool enemyOrdersChanged = m_EnemyCommander.Tick(m_State, m_Preset, m_EnemyDirector, m_SupportRequests, m_Garrisons, elapsedSeconds);
		bool hqRuntimeChanged = m_HQ.EnsureRuntimeObjects(m_State);
		bool physicalWarChanged = m_PhysicalWar.UpdateZoneActivation(m_State, m_Balance, m_Preset, m_EnemyDirector, m_ZoneCompositions);
		bool captureChanged = m_ZoneCapture.TickContestedCapture(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, m_Garrisons, m_EnemyCommander, m_EnemyDirector, m_SupportRequests, elapsedSeconds);
		bool civilianRuntimeChanged = m_Civilians.UpdatePhysicalTownPopulation(m_State, m_Preset, m_Balance);
		if (supportChanged)
			BroadcastSupportChangeNotifications();
		if (enemyOrdersChanged)
			BroadcastEnemyOrderChangeNotifications();
		bool captureMarkerChanged = BroadcastCaptureChangeNotifications();
		bool supportMarkerChanged = false;
		if (supportChanged && m_SupportRequests)
			supportMarkerChanged = m_SupportRequests.ConsumeMarkerRefreshNeeded();
		bool physicalWarMarkerChanged = false;
		if (physicalWarChanged && m_PhysicalWar)
			physicalWarMarkerChanged = m_PhysicalWar.ConsumeMarkerRefreshNeeded();
		bool anyStateChanged = missionChanged || objectiveChanged || missionRuntimeChanged || convoyRuntimeChanged || convoyOutcomeChanged || income > 0 || enemyResourcesChanged || aggressionChanged || civilianChanged || supportChanged || enemyOrdersChanged || hqRuntimeChanged || physicalWarChanged || captureChanged || civilianRuntimeChanged;
		bool markerStateChanged = missionChanged || missionRuntimeChanged || convoyRuntimeChanged || convoyOutcomeChanged || income > 0 || enemyResourcesChanged || aggressionChanged || supportMarkerChanged || enemyOrdersChanged || hqRuntimeChanged || captureMarkerChanged || physicalWarMarkerChanged;
		bool forceImmediateMarkerRefresh = missionChanged || hqRuntimeChanged;
		markerStateChanged = ResolveThrottledMarkerRefresh(markerStateChanged, forceImmediateMarkerRefresh);
		if (markerStateChanged)
			anyStateChanged = true;
		if (anyStateChanged)
		{
			MarkMajorCampaignChange(markerStateChanged);
			if (markerStateChanged && m_SupportRequests)
				m_SupportRequests.ConsumeMarkerRefreshNeeded();
		}
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

		return m_CommandUI.BuildVisibleMenuPayload(m_State, m_Preset, m_MapMarkers, m_Arsenal, m_Settings, m_Balance, playerId, selectedTabId, lastResult, CanPlayerUseMemberActions(playerId), CanPlayerUseCommanderActions(playerId), CanPlayerUseAdminActions(playerId), m_ZoneCompositions, m_ZoneCapture);
	}

	string RequestVisibleMenuCommand(int playerId, string selectedTabId, string commandId, string argument = "")
	{
		if (!Replication.IsServer() || !m_CommandUI)
			return "h-istasi command | server coordinator not ready";

		string result = m_CommandUI.ExecuteVisibleCommand(this, playerId, commandId, argument);
		LogVisibleMenuCommandResult(playerId, selectedTabId, commandId, argument, result);
		return result;
	}

	string BuildLoadoutEditorPayload(int playerId)
	{
		if (!Replication.IsServer() || !m_LoadoutEditor)
			return "HST_LOADOUT_EDITOR|offline||false|0|0|0|0\nPREVIEW|false|0 0 0|0|server coordinator not ready\nEND";

		string payload = m_LoadoutEditor.BuildEditorPayload(m_State, ResolveTrustedIdentityId(playerId), playerId);
		if (payload.IsEmpty())
			return "HST_LOADOUT_EDITOR|offline||false|0|0|0|0\nPREVIEW|false|0 0 0|0|editor payload unavailable\nEND";

		return payload + "\nEND";
	}

	string BuildLoadoutEditorCandidatePayload(int playerId, string nodeId)
	{
		if (!Replication.IsServer() || !m_LoadoutEditor)
			return string.Format("HST_LOADOUT_CANDIDATES|%1|unavailable|0|server coordinator not ready\nEND", nodeId);

		string payload = m_LoadoutEditor.BuildEditorCandidatePayload(m_State, ResolveTrustedIdentityId(playerId), playerId, nodeId);
		if (payload.IsEmpty())
			return string.Format("HST_LOADOUT_CANDIDATES|%1|unavailable|0|candidate payload unavailable\nEND", nodeId);

		return payload + "\nEND";
	}

	string BuildMissionIntelPayload(int playerId = 0)
	{
		if (!Replication.IsServer() || !m_State)
			return "HST_MISSION_INTEL|offline|0\nEND";

		string payload = string.Format("HST_MISSION_INTEL|%1|%2", m_State.m_iElapsedSeconds, playerId);
		foreach (HST_ActiveMissionState mission : m_State.m_aActiveMissions)
		{
			if (!mission)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;

			HST_MissionDefinition definition = m_Missions.FindDefinition(mission.m_sMissionId);
			vector position = ResolveMissionIntelPosition(mission);
			string title = ResolveMissionTitle(mission, definition);
			string requirements = "";
			string reward = "";
			string failure = "";
			if (definition)
			{
				requirements = definition.m_sRequirementText;
				reward = definition.m_sRewardText;
				failure = ResolveMissionFailureText(mission, definition);
			}

			payload = payload + "\nMISSION|" + mission.m_sInstanceId;
			payload = payload + "|" + PayloadText(title);
			payload = payload + "|" + MissionStatusLabel(mission.m_eStatus);
			payload = payload + "|" + MissionCategoryLabel(definition);
			payload = payload + "|" + PayloadText(mission.m_sTargetZoneId);
			payload = payload + "|" + PayloadText(mission.m_sSiteId);
			payload = payload + "|" + string.Format("%1", position);
			payload = payload + "|" + string.Format("%1", mission.m_iRemainingSeconds);
			payload = payload + "|" + PayloadText(requirements);
			payload = payload + "|" + PayloadText(BuildMissionProgressText(mission));
			payload = payload + "|" + PayloadText(reward);
			payload = payload + "|" + PayloadText(failure);
			payload = payload + "|" + ResolveMissionMarkerId(mission);

			foreach (HST_MissionObjectiveState objective : m_State.m_aMissionObjectives)
			{
				if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
					continue;

				payload = payload + string.Format("\nOBJECTIVE|%1|%2|%3|%4|%5|%6|%7", mission.m_sInstanceId, objective.m_sObjectiveId, PayloadText(objective.m_sLabel), objective.m_iCurrentProgress, objective.m_iRequiredProgress, objective.m_bComplete, objective.m_bFailed);
			}

			foreach (HST_MissionAssetState asset : m_State.m_aMissionAssets)
			{
				if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
					continue;

				payload = payload + string.Format("\nOBJECTIVE|%1|%2|%3|%4|%5|%6|%7", mission.m_sInstanceId, asset.m_sAssetId, PayloadText(BuildMissionAssetIntelLabel(asset)), MissionAssetProgressValue(asset), 1, MissionAssetCompleteValue(asset), MissionAssetFailedValue(asset));
			}
		}

		return payload + "\nEND";
	}

	string RequestLoadoutEditorCommand(int playerId, string commandId, string argument = "")
	{
		if (!Replication.IsServer())
			return "h-istasi loadout editor | server required";

		if (commandId == "loadout_editor_open_hq_arsenal")
			return RequestMemberOpenLoadoutEditor(playerId);

		if (commandId == "loadout_editor_close")
			return RequestMemberCloseLoadoutEditor(playerId);

		if (commandId == "loadout_editor_candidates")
			return "h-istasi loadout editor | candidate request acknowledged";

		if (commandId == "loadout_editor_refresh")
			return "h-istasi loadout editor | refreshed";

		if (commandId == "loadout_save" || commandId == "save_loadout")
			return RequestMemberSaveLoadoutDraft(playerId, argument);

		if (commandId == "loadout_apply" || commandId == "apply")
			return RequestMemberApplySavedLoadout(playerId, argument);

		if (commandId == "loadout_add_item")
			return RequestMemberAddLoadoutDraftItem(playerId, argument);

		if (commandId == "loadout_remove_slot" || commandId == "remove_storage_item")
			return RequestMemberRemoveLoadoutDraftSlot(playerId, argument);

		if (commandId == "loadout_set_quantity")
			return RequestMemberSetLoadoutDraftSlotQuantity(playerId, argument);

		if (commandId == "loadout_replace_slot")
			return RequestMemberReplaceLoadoutDraftSlotItem(playerId, argument);

		if (commandId == "set_node_item" || commandId == "set_attachment" || commandId == "add_storage_item")
			return RequestMemberSetLoadoutNodeItem(playerId, argument);

		if (commandId == "remove_node_item" || commandId == "remove_attachment")
			return RequestMemberRemoveLoadoutNodeItem(playerId, argument);

		if (commandId == "loadout_clear_draft")
			return RequestMemberClearLoadoutDraft(playerId);

		if (commandId == "loadout_select" || commandId == "load_loadout")
			return RequestMemberSelectSavedLoadout(playerId, argument);

		if (commandId == "loadout_delete" || commandId == "delete_loadout")
			return RequestMemberDeleteSavedLoadout(playerId, argument);

		return "h-istasi loadout editor | unknown command: " + commandId;
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

		HST_PlayerState player = m_Authorization.RegisterPlayer(m_State, identityId, isAdmin || IsSettingsAdminIdentity(identityId) || IsDeveloperFallbackAdminIdentity(identityId));
		ApplyRuntimeMembershipDefaults(player);
		if (player && m_Civilians)
			m_Civilians.EnsurePlayer(m_State, player.m_sIdentityId);
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
		HST_PlayerState player = m_PlayerLifecycle.RegisterConnectedPlayer(m_State, m_Authorization, playerId, identityId, isAdmin || IsSettingsAdminIdentity(resolvedIdentityId) || IsDeveloperFallbackAdminIdentity(resolvedIdentityId));
		ApplyRuntimeMembershipDefaults(player);
		if (player && m_Civilians)
			m_Civilians.EnsurePlayer(m_State, player.m_sIdentityId);
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

		string identityId = ResolveTrustedIdentityId(playerId);
		HST_PlayerState playerBefore = m_State.FindPlayer(identityId);
		int previousSpawnCount;
		if (playerBefore)
			previousSpawnCount = playerBefore.m_iSpawnCount;

		if (m_PlayerSpawn.OnPlayerSpawned(m_State, m_Authorization, m_PlayerLifecycle, playerId, entity))
		{
			if (m_LoadoutEditor && previousSpawnCount > 0 && !identityId.IsEmpty())
				m_LoadoutEditor.MarkIssuedLoadoutLostOnDeath(m_State, identityId);
			MarkMajorCampaignChange();
		}
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

		bool changed = m_Strategic.SetZoneOwner(m_State, m_Economy, m_Balance, zoneId, factionKey, m_Preset.m_sResistanceFactionKey);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	bool CaptureZoneForResistance(string zoneId, int supportReward = 10)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_ZoneCapture.CaptureForResistance(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, zoneId, supportReward, m_Garrisons, m_EnemyCommander, m_EnemyDirector, m_SupportRequests);
		if (changed)
		{
			BroadcastCaptureChangeNotifications();
			MarkMajorCampaignChange();
		}
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
		if (!m_Persistence || !m_State)
			return false;

		if (m_Loot)
		{
			int snapshottedFieldVehicles = m_Loot.SnapshotNearbyPersistentVehicles(m_State);
			if (snapshottedFieldVehicles > 0)
				m_State.m_sLastVehicleTargetStatus = string.Format("persistent field vehicle snapshot %1", snapshottedFieldVehicles);
		}

		return m_Persistence.RequestCheckpoint("h-istasi manual checkpoint", m_State);
	}

	bool SelectInitialHideout(string hideoutId)
	{
		if (!Replication.IsServer())
			return false;

		return SelectInitialHideout_S(hideoutId);
	}

	bool RequestCommanderSelectInitialHideout(int playerId, string hideoutId)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId))
			return false;

		return SelectInitialHideout(hideoutId);
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

		if (m_Objectives)
			m_Objectives.ProgressMission(m_State, instanceId, 999);

		bool applyDefinitionRewards = !ShouldSuppressDefinitionRewardForConvoyCompletion(activeMission);
		bool allowExpiredCompletion = activeMission && activeMission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED && m_MissionRuntime && m_MissionRuntime.CanCompleteExpiredPlayerBoundMission(m_State, activeMission);
		bool changed = m_Missions.Complete(m_State, m_Economy, instanceId, applyDefinitionRewards, allowExpiredCompletion);
		if (changed && ApplyCompletedMissionOutcome(definition, activeMission))
			changed = true;

		if (changed)
		{
			BroadcastCaptureChangeNotifications();
			MarkMajorCampaignChange();
		}
		return changed;
	}

	protected bool ApplyConvoyOutcomesNow()
	{
		if (!m_ConvoyOutcomes)
			return false;

		return m_ConvoyOutcomes.TickConvoyOutcomes(m_State, m_Preset, m_Balance, m_Economy, m_Arsenal, m_Garrisons, m_Towns, m_Strategic);
	}

	protected bool ShouldSuppressDefinitionRewardForConvoyCompletion(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_sRuntimePrimitive != "convoy_intercept")
			return false;

		return mission.m_sMissionId == "convoy_money" || mission.m_sMissionId == "convoy_prisoners";
	}

	bool FailMission(string instanceId)
	{
		if (!Replication.IsServer())
			return false;

		HST_ActiveMissionState activeMission = m_State.FindActiveMission(instanceId);
		HST_MissionDefinition definition;
		if (activeMission)
			definition = m_Missions.FindDefinition(activeMission.m_sMissionId);

		bool changed = m_Missions.Fail(m_State, m_Preset, m_Economy, instanceId);
		if (changed)
			ApplyFailedMissionOutcome(definition, activeMission);
		if (changed && m_Objectives)
			m_Objectives.FailMissionObjectives(m_State, instanceId);
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
		{
			m_HQ.EnsureRuntimeObjects(m_State);
			m_State.m_iHQKnowledge = Math.Max(0, m_State.m_iHQKnowledge - 50);
			MarkMajorCampaignChange();
		}
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
		{
			m_HQ.EnsureRuntimeObjects(m_State);
			MarkMajorCampaignChange();
		}
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

	bool RequestCommanderRebuildHQAssets(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId))
			return false;

		if (!m_HQ || !m_BuildMode)
			return false;

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return false;

		HST_BuildModePlacement placement = m_BuildMode.ResolveHQRebuildPlacement(m_State, playerId);
		if (!placement || !placement.m_bValid)
			return false;

		bool changed = m_HQ.RebuildRuntimeObjects(m_State);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
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

	string RequestCommanderRecruitGarrisonReport(int playerId, string zoneId, int infantryCount, int vehicleCount = 0, int moneyCost = 100, int hrCost = 1)
	{
		if (!Replication.IsServer())
			return "h-istasi command | recruit FIA garrison | failed: server authority unavailable";

		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi command | recruit FIA garrison | failed: commander permission required";

		if (!m_State || !m_Preset)
			return "h-istasi command | recruit FIA garrison | failed: campaign state unavailable";

		HST_ZoneState zone = m_State.FindZone(zoneId);
		if (!zone)
			return "h-istasi command | recruit FIA garrison | failed: zone not found";

		if (zone.m_sOwnerFactionKey != m_Preset.m_sResistanceFactionKey)
			return string.Format("h-istasi command | recruit FIA garrison at %1 | failed: zone is owned by %2", ResolveZoneLabel(zone), zone.m_sOwnerFactionKey);

		if (m_State.m_iFactionMoney < moneyCost)
			return string.Format("h-istasi command | recruit FIA garrison at %1 | failed: need $%2, have $%3", ResolveZoneLabel(zone), moneyCost, m_State.m_iFactionMoney);

		if (m_State.m_iHR < hrCost)
			return string.Format("h-istasi command | recruit FIA garrison at %1 | failed: need %2 HR, have %3", ResolveZoneLabel(zone), hrCost, m_State.m_iHR);

		HST_GarrisonState beforeGarrison = m_State.FindGarrison(zoneId, m_Preset.m_sResistanceFactionKey);
		int beforeInfantry;
		if (beforeGarrison)
			beforeInfantry = beforeGarrison.m_iInfantryCount;

		int currentInfantry = beforeInfantry + Math.Max(0, zone.m_iActiveInfantryCount);
		if (zone.m_iGarrisonSlots > 0 && currentInfantry >= zone.m_iGarrisonSlots)
			return string.Format("h-istasi command | recruit FIA garrison at %1 | failed: garrison full %2/%3", ResolveZoneLabel(zone), currentInfantry, zone.m_iGarrisonSlots);

		bool changed = RecruitResistanceGarrison(zoneId, infantryCount, vehicleCount, moneyCost, hrCost);
		HST_GarrisonState afterGarrison = m_State.FindGarrison(zoneId, m_Preset.m_sResistanceFactionKey);
		int afterInfantry;
		if (afterGarrison)
			afterInfantry = afterGarrison.m_iInfantryCount;

		if (!changed || afterInfantry <= beforeInfantry)
			return string.Format("h-istasi command | recruit FIA garrison at %1 | failed: no garrison change", ResolveZoneLabel(zone));

		string capacity = "uncapped";
		if (zone.m_iGarrisonSlots > 0)
			capacity = string.Format("%1/%2", afterInfantry + Math.Max(0, zone.m_iActiveInfantryCount), zone.m_iGarrisonSlots);

		return string.Format("h-istasi command | recruit FIA garrison at %1 | complete: +%2 infantry, garrison %3, money $%4, HR %5", ResolveZoneLabel(zone), afterInfantry - beforeInfantry, capacity, m_State.m_iFactionMoney, m_State.m_iHR);
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

		return StartMission(SelectMissionForZone(zone), zoneId);
	}

	bool RequestCommanderStartRandomMission(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId))
			return false;

		HST_ZoneState zone = SelectRandomMissionZone();
		if (!zone)
			return false;

		return StartMission(SelectMissionForZone(zone), zone.m_sZoneId);
	}

	bool RequestCommanderProgressMission(int playerId, string instanceId = "")
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId) || !m_Objectives)
			return false;

		string resolvedInstanceId = ResolveMissionInstanceId(instanceId);
		if (resolvedInstanceId.IsEmpty())
			return false;

		bool changed = m_Objectives.ProgressMission(m_State, resolvedInstanceId, 1);
		if (changed && m_Objectives.AreMissionObjectivesComplete(m_State, resolvedInstanceId))
			CompleteMission(resolvedInstanceId);
		else if (changed)
			MarkMajorCampaignChange();

		return changed;
	}

	bool RequestCommanderCallSupplyDrop(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId) || !m_SupportRequests)
			return false;

		string targetZoneId = SelectHQSupportZoneId();
		HST_SupportRequestState request = m_SupportRequests.RequestSupport(m_State, m_Preset, m_Economy, m_EnemyDirector, m_Preset.m_sResistanceFactionKey, HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP, targetZoneId, true);
		if (request)
		{
			MarkMajorCampaignChange();
			m_SupportRequests.ConsumeMarkerRefreshNeeded();
		}
		return request != null;
	}

	bool RequestCommanderCallPlayerSupport(int playerId, HST_ESupportRequestType supportType)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId) || !m_SupportRequests)
			return false;

		if (IsAirSupportType(supportType))
		{
			if (!m_Balance.m_bAirSupportEnabled || !HasResistanceAirSupportCapability())
				return false;
		}

		string targetZoneId = SelectPlayerSupportZoneId(playerId);
		int cooldownSeconds = HST_SupportRequestService.PLAYER_SUPPORT_COOLDOWN_SECONDS;
		if (IsAirSupportType(supportType))
			cooldownSeconds = m_Balance.m_iAirSupportCooldownSeconds;

		HST_SupportRequestState request = m_SupportRequests.RequestSupport(m_State, m_Preset, m_Economy, m_EnemyDirector, m_Preset.m_sResistanceFactionKey, supportType, targetZoneId, true, cooldownSeconds);
		if (request)
		{
			MarkMajorCampaignChange();
			m_SupportRequests.ConsumeMarkerRefreshNeeded();
		}
		return request != null;
	}

	bool RequestCommanderCancelSupport(int playerId, string requestId = "")
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId) || !m_SupportRequests)
			return false;

		bool changed = m_SupportRequests.CancelSupportRequest(m_State, requestId, true);
		if (changed)
		{
			MarkMajorCampaignChange();
			m_SupportRequests.ConsumeMarkerRefreshNeeded();
		}
		return changed;
	}

	bool RequestCommanderAidNearestTown(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId) || !m_Civilians)
			return false;

		string targetZoneId = SelectHQSupportZoneId();
		if (targetZoneId.IsEmpty())
			return false;

		if (!m_Economy.SpendFactionMoney(m_State, 100))
			return false;

		bool changed = m_Civilians.RegisterIncident(m_State, targetZoneId, 12, -2, "aid delivery");
		if (changed)
			MarkMajorCampaignChange();
		return changed;
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

	string RequestMemberManualCheckpointReport(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi checkpoint | not available | server required";
		if (!CanPlayerUseMemberActions(playerId))
			return "h-istasi checkpoint | not available | membership required";
		if (!m_Persistence)
			return "h-istasi checkpoint | not available | persistence service not ready";
		if (!m_State)
			return "h-istasi checkpoint | not available | campaign state not ready";

		bool success = RequestManualCheckpoint();
		if (success)
			return "h-istasi checkpoint | success | " + m_State.m_sLastPersistenceStatus;

		string reason = m_State.m_sLastPersistenceStatus;
		if (reason.IsEmpty())
			reason = "checkpoint request returned false";
		return "h-istasi checkpoint | not available | " + reason;
	}

	string RequestMemberFoundationStatus(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi foundation | server required";
		if (!CanPlayerUseMemberActions(playerId))
			return "h-istasi foundation | membership required";

		return BuildFoundationStatusReport();
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

	string RequestMemberInspectActiveMissions(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi mission runtime | server required";
		if (!CanPlayerUseMemberActions(playerId))
			return "h-istasi mission runtime | membership required";
		if (!m_MissionRuntime)
			return "h-istasi mission runtime | service not ready";

		string report = m_MissionRuntime.BuildRuntimeReport(m_State);
		if (m_PhysicalWar)
			report = report + "\n" + m_PhysicalWar.BuildConvoyRuntimeReport(m_State);
		return report;
	}

	string RequestMemberInspectMission(int playerId, string instanceId)
	{
		if (!Replication.IsServer())
			return "h-istasi mission runtime | server required";
		if (!CanPlayerUseMemberActions(playerId))
			return "h-istasi mission runtime | membership required";
		if (!m_MissionRuntime)
			return "h-istasi mission runtime | service not ready";

		return m_MissionRuntime.BuildRuntimeReportForMission(m_State, instanceId);
	}

	string RequestMemberInspectConvoyRuntime(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi convoy runtime | server required";
		if (!CanPlayerUseMemberActions(playerId))
			return "h-istasi convoy runtime | membership required";
		if (!m_PhysicalWar)
			return "h-istasi convoy runtime | service not ready";

		return m_PhysicalWar.BuildConvoyRuntimeReport(m_State);
	}

	string RequestMemberInspectObjectives(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_Objectives)
			return "";

		return m_Objectives.BuildObjectiveReport(m_State);
	}

	string RequestMemberInspectArsenal(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_Arsenal)
			return "";

		return m_Arsenal.BuildArsenalReport(m_State, m_Balance);
	}

	string RequestMemberInspectGarage(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_Arsenal)
			return "";

		string report = m_Arsenal.BuildGarageReport(m_State);
		if (m_Loot)
			report = report + "\n" + m_Loot.BuildVehicleCargoReport(m_State);
		return report;
	}

	string RequestMemberInspectVehicleCargo(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_Loot)
			return "";

		return m_Loot.BuildVehicleCargoReport(m_State);
	}

	string RequestMemberInspectSupport(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_SupportRequests)
			return "";

		return m_SupportRequests.BuildSupportReport(m_State) + "\n" + m_EnemyCommander.BuildEnemyOrderReport(m_State);
	}

	string RequestMemberInspectCivilians(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_Civilians)
			return "";

		return m_Civilians.BuildCivilianReport(m_State);
	}

	string RequestMemberInspectUndercover(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_Civilians)
			return "";

		return m_Civilians.BuildUndercoverReport(m_State, ResolveTrustedIdentityId(playerId));
	}

	string RequestMemberInspectGeneratedContent(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_Content)
			return "";

		string report = m_Content.BuildContentReport(m_State);
		if (m_PhysicalWar)
			report = report + "\n" + m_PhysicalWar.BuildGroundVehicleCandidateReport();
		return report;
	}

	string RequestMemberInspectPersistence(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_Persistence)
			return "";

		string report = m_Persistence.BuildPersistenceReport(m_State);
		if (m_PersistenceSmokeTest)
			report = report + "\n" + m_PersistenceSmokeTest.BuildReport(m_State);
		return report;
	}

	string RequestMemberInspectLoadoutEditor(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_LoadoutEditor)
			return "";

		return "h-istasi loadout editor | " + m_LoadoutEditor.BuildEditorReport(m_State, ResolveTrustedIdentityId(playerId));
	}

	string RequestMemberOpenLoadoutEditor(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loadout editor | membership required";

		if (!m_LoadoutEditor || !m_Arsenal)
			return "h-istasi loadout editor | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi loadout editor");

		string result = m_LoadoutEditor.OpenEditor(m_State, ResolveTrustedIdentityId(playerId), playerId);
		MarkMajorCampaignChange(false);
		return result;
	}

	string RequestMemberCloseLoadoutEditor(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loadout editor | membership required";

		if (!m_LoadoutEditor)
			return "h-istasi loadout editor | service not ready";

		return m_LoadoutEditor.CloseEditor(m_State, ResolveTrustedIdentityId(playerId));
	}

	string RequestMemberSaveLoadoutDraft(int playerId, string loadoutName = "")
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loadout editor | membership required";

		if (!m_LoadoutEditor)
			return "h-istasi loadout editor | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi loadout editor");

		string result = m_LoadoutEditor.SaveCurrentDraft(m_State, ResolveTrustedIdentityId(playerId), loadoutName, playerId);
		if (!result.Contains("failed"))
			MarkMajorCampaignChange();
		return result;
	}

	string RequestMemberApplySavedLoadout(int playerId, string loadoutId = "")
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loadout editor | membership required";

		if (!m_LoadoutEditor || !m_Arsenal)
			return "h-istasi loadout editor | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi loadout editor");

		string result = m_LoadoutEditor.ApplySavedLoadout(m_State, m_Arsenal, ResolveTrustedIdentityId(playerId), playerId, loadoutId);
		if (result.Contains("applied"))
			MarkMajorCampaignChange();
		return result;
	}

	string RequestMemberAddLoadoutDraftItem(int playerId, string itemPrefab)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loadout editor | membership required";

		if (!m_LoadoutEditor)
			return "h-istasi loadout editor | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi loadout editor");

		string result = m_LoadoutEditor.AddDraftItem(m_State, m_Arsenal, ResolveTrustedIdentityId(playerId), playerId, itemPrefab);
		if (!result.Contains("failed"))
			MarkMajorCampaignChange();
		return result;
	}

	string RequestMemberRemoveLoadoutDraftSlot(int playerId, string slotId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loadout editor | membership required";

		if (!m_LoadoutEditor || !m_Arsenal)
			return "h-istasi loadout editor | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi loadout editor");

		string result = m_LoadoutEditor.RemoveDraftSlot(m_State, m_Arsenal, ResolveTrustedIdentityId(playerId), playerId, slotId);
		if (!result.Contains("failed"))
			MarkMajorCampaignChange();
		return result;
	}

	string RequestMemberSetLoadoutDraftSlotQuantity(int playerId, string argument)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loadout editor | membership required";

		if (!m_LoadoutEditor || !m_Arsenal)
			return "h-istasi loadout editor | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi loadout editor");

		string result = m_LoadoutEditor.SetDraftSlotQuantity(m_State, m_Arsenal, ResolveTrustedIdentityId(playerId), playerId, argument);
		if (!result.Contains("failed"))
			MarkMajorCampaignChange();
		return result;
	}

	string RequestMemberReplaceLoadoutDraftSlotItem(int playerId, string argument)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loadout editor | membership required";

		if (!m_LoadoutEditor || !m_Arsenal)
			return "h-istasi loadout editor | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi loadout editor");

		string result = m_LoadoutEditor.ReplaceDraftSlotItem(m_State, m_Arsenal, ResolveTrustedIdentityId(playerId), playerId, argument);
		if (!result.Contains("failed"))
			MarkMajorCampaignChange();
		return result;
	}

	string RequestMemberSetLoadoutNodeItem(int playerId, string argument)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loadout editor | membership required";

		if (!m_LoadoutEditor || !m_Arsenal)
			return "h-istasi loadout editor | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi loadout editor");

		string result = m_LoadoutEditor.SetNodeItem(m_State, m_Arsenal, ResolveTrustedIdentityId(playerId), playerId, argument);
		if (!result.Contains("failed"))
			MarkMajorCampaignChange();
		return result;
	}

	string RequestMemberRemoveLoadoutNodeItem(int playerId, string nodeId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loadout editor | membership required";

		if (!m_LoadoutEditor || !m_Arsenal)
			return "h-istasi loadout editor | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi loadout editor");

		string result = m_LoadoutEditor.RemoveNodeItem(m_State, m_Arsenal, ResolveTrustedIdentityId(playerId), playerId, nodeId);
		if (!result.Contains("failed"))
			MarkMajorCampaignChange();
		return result;
	}

	string RequestMemberClearLoadoutDraft(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loadout editor | membership required";

		if (!m_LoadoutEditor || !m_Arsenal)
			return "h-istasi loadout editor | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi loadout editor");

		string result = m_LoadoutEditor.ClearDraft(m_State, m_Arsenal, ResolveTrustedIdentityId(playerId), playerId);
		if (!result.Contains("failed"))
			MarkMajorCampaignChange();
		return result;
	}

	string RequestMemberSelectSavedLoadout(int playerId, string loadoutId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loadout editor | membership required";

		if (!m_LoadoutEditor)
			return "h-istasi loadout editor | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi loadout editor");

		return m_LoadoutEditor.SelectSavedLoadout(m_State, ResolveTrustedIdentityId(playerId), loadoutId);
	}

	string RequestMemberDeleteSavedLoadout(int playerId, string loadoutId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loadout editor | membership required";

		if (!m_LoadoutEditor)
			return "h-istasi loadout editor | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi loadout editor");

		string result = m_LoadoutEditor.DeleteSavedLoadout(m_State, ResolveTrustedIdentityId(playerId), loadoutId);
		if (!result.Contains("failed"))
			MarkMajorCampaignChange();
		return result;
	}

	string RequestMemberInspectMissionRuntime(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_MissionRuntime)
			return "";

		string report = m_MissionRuntime.BuildRuntimeReport(m_State);
		if (m_PhysicalWar)
			report = report + "\n" + m_PhysicalWar.BuildConvoyRuntimeReport(m_State);

		return report;
	}

	string RequestMemberMissionInteraction(int playerId, string commandId, string argument = "")
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi mission | membership required";

		if (!m_MissionRuntime)
			return "h-istasi mission | service not ready";

		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		string result;
		string eventType;
		string missionInstanceId;
		bool changed = m_MissionRuntime.HandlePlayerMissionInteraction(m_State, m_Preset, m_Arsenal, playerId, playerEntity, commandId, argument, result, eventType, missionInstanceId);
		if (!changed)
			return result;

		HST_ActiveMissionState mission = m_State.FindActiveMission(missionInstanceId);
		HST_MissionDefinition definition;
		if (mission)
			definition = m_Missions.FindDefinition(mission.m_sMissionId);

		if (!eventType.IsEmpty())
			BroadcastMissionEvent(eventType, mission, definition);

		ApplyConvoyOutcomesNow();

		string completedRuntimeMissionId = m_MissionRuntime.FindCompletedActiveMissionId(m_State, m_Objectives);
		if (!completedRuntimeMissionId.IsEmpty())
			CompleteMission(completedRuntimeMissionId);

		string failedRuntimeMissionId = m_MissionRuntime.FindFailedActiveMissionId(m_State);
		if (!failedRuntimeMissionId.IsEmpty())
			FailMission(failedRuntimeMissionId);

		BroadcastPendingMissionOutcomeEvents();
		MarkMajorCampaignChange();
		return result;
	}

	string RequestServerMissionAssetDestroyed(string assetId, vector position)
	{
		if (!Replication.IsServer())
			return "h-istasi mission | server required";

		if (!m_MissionRuntime)
			return "h-istasi mission | service not ready";

		string result;
		string eventType;
		string missionInstanceId;
		bool changed = m_MissionRuntime.MarkMissionAssetDestroyedByRuntimeEntity(m_State, assetId, position, result, eventType, missionInstanceId);
		if (!changed)
			return result;

		HST_ActiveMissionState mission = m_State.FindActiveMission(missionInstanceId);
		HST_MissionDefinition definition;
		if (mission)
			definition = m_Missions.FindDefinition(mission.m_sMissionId);

		if (!eventType.IsEmpty())
			BroadcastMissionEvent(eventType, mission, definition);

		ApplyConvoyOutcomesNow();

		string completedRuntimeMissionId = m_MissionRuntime.FindCompletedActiveMissionId(m_State, m_Objectives);
		if (!completedRuntimeMissionId.IsEmpty())
			CompleteMission(completedRuntimeMissionId);

		string failedRuntimeMissionId = m_MissionRuntime.FindFailedActiveMissionId(m_State);
		if (!failedRuntimeMissionId.IsEmpty())
			FailMission(failedRuntimeMissionId);

		BroadcastPendingMissionOutcomeEvents();
		MarkMajorCampaignChange();
		return result;
	}

	string RequestMemberLootNearby(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loot | membership required";

		if (!m_Settings || !m_Settings.m_Features.m_bAreaLootEnabled)
			return "h-istasi loot | area loot disabled by config";

		if (!m_Loot || !m_Arsenal)
			return "h-istasi loot | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi loot");

		HST_LootResult result = m_Loot.LootNearbyToArsenal(m_State, m_Preset, m_Balance, m_Arsenal, playerId);
		if (result && result.m_iItemsDeposited > 0)
			MarkMajorCampaignChange();

		if (!result)
			return "h-istasi loot | no result";

		return result.BuildSummary();
	}

	string RequestMemberWithdrawBestArsenalItem(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi arsenal | membership required";

		if (!m_Arsenal)
			return "h-istasi arsenal | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi arsenal");

		string result = m_Arsenal.WithdrawBestAvailableItem(m_State);
		if (!result.IsEmpty())
			MarkMajorCampaignChange();
		return result;
	}

	string RequestMemberCaptureNearbyVehicle(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi garage | failed: membership required";

		if (!m_Loot || !m_Arsenal)
			return "h-istasi garage | failed: service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi garage | failed");

		string result = m_Loot.CaptureNearbyVehicleToGarage(m_State, m_Preset, m_Arsenal, playerId);
		if (result.Contains("complete"))
			MarkMajorCampaignChange();
		BroadcastNotification("garage_capture_" + playerId + "_" + m_State.m_iElapsedSeconds, "garage", ResolveResultSeverity(result), "Garage", result, "", "", "0 0 0", 5.0);
		return result;
	}

	string RequestMemberCollectVehicleLoot(int playerId, string vehicleRuntimeId = "")
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi vehicle loot | membership required";

		if (!m_Settings || !m_Settings.m_Features.m_bAreaLootEnabled)
			return "h-istasi vehicle loot | area loot disabled by config";

		if (!m_Loot || !m_Arsenal)
			return "h-istasi vehicle loot | service not ready";

		HST_LootResult result = m_Loot.CollectNearbyLootToVehicle(m_State, m_Preset, m_Balance, m_Arsenal, playerId, vehicleRuntimeId);
		if (result && result.m_iItemsDeposited > 0)
			MarkMajorCampaignChange();

		if (!result)
			return "h-istasi vehicle loot | no result";

		BroadcastNotification("vehicle_loot_" + playerId + "_" + m_State.m_iElapsedSeconds, "vehicle", ResolveResultSeverity(result.BuildSummary()), "Vehicle Loot", result.BuildSummary(), "", "", "0 0 0", 5.0);
		return result.BuildSummary();
	}

	string RequestMemberUnloadVehicleCargo(int playerId, string vehicleRuntimeId = "")
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi vehicle loot | membership required";

		if (!m_Loot || !m_Arsenal)
			return "h-istasi vehicle loot | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi vehicle loot");

		string result = m_Loot.UnloadNearestVehicleCargoToArsenal(m_State, m_Preset, m_Balance, m_Arsenal, playerId, vehicleRuntimeId);
		if (!result.Contains("no stored cargo") && !result.Contains("no eligible vehicle") && !result.Contains("no living player"))
			MarkMajorCampaignChange();

		BroadcastNotification("vehicle_unload_" + playerId + "_" + m_State.m_iElapsedSeconds, "vehicle", ResolveResultSeverity(result), "Vehicle Cargo", result, "", "", "0 0 0", 5.0);
		return result;
	}

	protected HST_GarageVehicleState SelectGarageVehicleForBuildMode(string vehicleId)
	{
		if (!m_State)
			return null;

		if (!vehicleId.IsEmpty())
			return m_State.FindGarageVehicle(vehicleId);

		foreach (HST_GarageVehicleState vehicle : m_State.m_aGarageVehicles)
		{
			if (vehicle)
				return vehicle;
		}

		return null;
	}

	string RequestMemberRedeployGarageVehicle(int playerId, string vehicleId = "")
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi garage | failed: membership required";

		if (!m_Arsenal || !m_BuildMode)
			return "h-istasi garage | failed: service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi garage | failed");

		HST_GarageVehicleState vehicle = SelectGarageVehicleForBuildMode(vehicleId);
		if (!vehicle)
			return "h-istasi garage | failed: selected vehicle not found";

		HST_BuildModePlacement placement = m_BuildMode.ResolveGarageRedeployPlacement(m_State, playerId, vehicle);
		if (!placement || !placement.m_bValid)
			return string.Format("h-istasi garage | failed: build placement denied | %1", m_State.m_sLastBuildModeFailure);

		vehicle.m_vAngles = placement.m_vAngles;
		string result = m_Arsenal.RedeployGarageVehicle(m_State, m_Economy, vehicle.m_sVehicleId, placement.m_vPosition);
		if (result.Contains("complete"))
			MarkMajorCampaignChange();
		return result;
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

		bool changed = m_ZoneCapture.AddResistanceCaptureProgress(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, zoneId, progress, 10, m_Garrisons, m_EnemyCommander, m_EnemyDirector, m_SupportRequests);
		if (changed)
		{
			bool captureMarkerChanged = BroadcastCaptureChangeNotifications();
			MarkMajorCampaignChange(captureMarkerChanged);
		}
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

	string RequestAdminStartMissionById(int playerId, string missionId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi admin | force mission | failed: admin required";

		if (!m_Missions || missionId.IsEmpty())
			return "h-istasi admin | force mission | failed: mission service not ready";

		HST_MissionDefinition definition = m_Missions.FindDefinition(missionId);
		if (!definition)
			return "h-istasi admin | force mission | failed: mission definition not found";

		bool repairedPhase = EnsureDebugMissionStartReady();
		if (!repairedPhase)
			return "h-istasi admin | force mission | failed: campaign is not active; select an HQ first";

		string targetZoneId = SelectDebugMissionTargetZoneId(definition);
		if (targetZoneId.IsEmpty() && !m_Missions.CanForceStart(m_State, m_Preset, missionId, ""))
			return string.Format("h-istasi admin | force %1 | failed: no compatible target", definition.m_sDisplayName);

		bool started = StartMission_S(missionId, targetZoneId, true);
		if (!started)
			return string.Format("h-istasi admin | force %1 | failed: duplicate active mission or invalid target", definition.m_sDisplayName);

		HST_ZoneState targetZone = m_State.FindZone(targetZoneId);
		string targetName = "open area";
		if (targetZone)
			targetName = ResolveZoneLabel(targetZone);

		return string.Format("h-istasi admin | force %1 at %2 | complete", definition.m_sDisplayName, targetName);
	}

	protected bool EnsureDebugMissionStartReady()
	{
		if (!m_State)
			return false;

		if (m_State.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
			return true;

		if (m_State.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_SETUP && m_State.m_bHQDeployed)
		{
			m_State.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
			Print("h-istasi admin | repaired campaign phase for debug mission start: HQ was already deployed");
			return true;
		}

		return false;
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

	string RequestAdminSeedPersistenceTestState(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi persistence smoke | admin required";

		if (!m_PersistenceSmokeTest)
			return "h-istasi persistence smoke | service not ready";

		string result = m_PersistenceSmokeTest.SeedTestState(m_State, m_Preset, ResolveTrustedIdentityId(playerId));
		bool checkpoint = RequestManualCheckpoint();
		return result + string.Format("\nmanual checkpoint %1", checkpoint);
	}

	string RequestAdminRunPersistenceSmokeTest(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi persistence smoke | admin required";

		if (!m_PersistenceSmokeTest)
			return "h-istasi persistence smoke | service not ready";

		bool checkpoint = RequestManualCheckpoint();
		return m_PersistenceSmokeTest.RunSmokeTest(m_State) + string.Format("\nmanual checkpoint %1", checkpoint);
	}

	string RequestAdminPersistenceSmokeReport(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi persistence smoke | admin required";

		if (!m_PersistenceSmokeTest)
			return "h-istasi persistence smoke | service not ready";

		return m_PersistenceSmokeTest.BuildReport(m_State);
	}

	string RequestAdminPhase14SeedFinite(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 14 smoke | admin required";

		if (!m_Arsenal)
			return "h-istasi phase 14 smoke | arsenal service not ready";

		HST_ArsenalItemState item = m_Arsenal.DepositItem(m_State, m_Balance, PHASE14_FINITE_PREFAB, 1, "utility", "Phase 14 Finite Only");
		if (item)
			MarkMajorCampaignChange();

		return string.Format("h-istasi phase 14 smoke | finite-only seed %1\n%2", item != null, m_Arsenal.BuildArsenalReport(m_State, m_Balance));
	}

	string RequestAdminPhase14SeedThreshold(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 14 smoke | admin required";

		if (!m_Arsenal)
			return "h-istasi phase 14 smoke | arsenal service not ready";

		HST_ArsenalItemState item = m_Arsenal.DepositItem(m_State, m_Balance, PHASE14_THRESHOLD_PREFAB, 2, "utility", "Phase 14 Threshold Item");
		if (item)
			MarkMajorCampaignChange();

		return string.Format("h-istasi phase 14 smoke | threshold seed %1\n%2", item != null, m_Arsenal.BuildArsenalReport(m_State, m_Balance));
	}

	string RequestAdminPhase14SeedBlocked(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 14 smoke | admin required";

		if (!m_Arsenal)
			return "h-istasi phase 14 smoke | arsenal service not ready";

		HST_ArsenalItemState blockedItem = m_Arsenal.DepositItem(m_State, m_Balance, PHASE14_BLOCKED_PREFAB, 1, "utility", "Phase 14 Blocked Item");
		HST_ArsenalItemState rawItem = m_Arsenal.DepositItem(m_State, m_Balance, PHASE14_RAW_ASSET_PREFAB, 1, "utility", "Phase 14 Raw Asset");
		return string.Format("h-istasi phase 14 smoke | blocked prefab accepted %1 | raw asset accepted %2\n%3", blockedItem != null, rawItem != null, m_Arsenal.BuildArsenalReport(m_State, m_Balance));
	}

	string RequestAdminPhase14Report(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 14 smoke | admin required";

		string report = "h-istasi phase 14 smoke report";
		if (m_Arsenal)
			report = report + "\n" + m_Arsenal.BuildArsenalReport(m_State, m_Balance);
		if (m_Loot)
			report = report + "\n" + m_Loot.BuildVehicleCargoReport(m_State);
		if (m_State)
			report = report + string.Format("\nloadout editor status %1 | last failure %2", m_State.m_sLoadoutEditorStatus, m_State.m_sLastLoadoutEditorFailure);
		return report;
	}

	string RequestAdminInspectZoneComposition(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi zone composition | admin required";

		if (!m_ZoneCompositions)
			return "h-istasi zone composition | service not ready";

		return m_ZoneCompositions.BuildCompositionReport(m_State);
	}

	protected void LogVisibleMenuCommandResult(int playerId, string selectedTabId, string commandId, string argument, string result)
	{
		string label = string.Format("h-istasi menu command | player %1 | tab %2 | command %3", playerId, selectedTabId, commandId);
		if (!argument.IsEmpty())
			label = label + string.Format(" | arg %1", argument);

		Print(string.Format("%1", label));
		if (result.IsEmpty())
		{
			Print("h-istasi menu command result | <empty>");
			return;
		}

		int cursor;
		while (cursor < result.Length())
		{
			int lineEnd = result.IndexOfFrom(cursor, "\n");
			string line;
			if (lineEnd < 0)
			{
				line = result.Substring(cursor, result.Length() - cursor);
				cursor = result.Length();
			}
			else
			{
				line = result.Substring(cursor, lineEnd - cursor);
				cursor = lineEnd + 1;
			}

			if (!line.IsEmpty())
				Print("h-istasi menu command result | " + line);
		}
	}

	bool RequestAdminAddEnemyResources(int playerId, string factionKey, int attackResources, int supportResources)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return false;

		m_EnemyDirector.AddResources(m_State, factionKey, attackResources, supportResources);
		MarkMajorCampaignChange();
		return true;
	}

	bool RequestAdminNewCampaignReset(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return false;

		array<ref HST_PlayerState> existingPlayers = {};
		foreach (HST_PlayerState player : m_State.m_aPlayers)
		{
			if (player)
				existingPlayers.Insert(player);
		}

		string commanderIdentityId = m_State.m_sCommanderIdentityId;
		m_State = CreateInitialCampaignState();
		foreach (HST_PlayerState existingPlayer : existingPlayers)
			m_State.m_aPlayers.Insert(existingPlayer);
		m_State.m_sCommanderIdentityId = commanderIdentityId;
		if (m_Authorization)
			m_Authorization.AssignCommanderOnVacancy(m_State);
		EnsureCampaignFoundation();
		m_Missions.SyncNextInstanceIdFromState(m_State);
		if (m_Persistence)
			m_Persistence.CaptureAndTrackState(m_State);
		RefreshCampaignMarkers();
		ArmPlayerSpawnSweep(4);
		MarkMajorCampaignChange(false);
		return true;
	}

	bool RequestAdminSetMembership(int playerId, string targetIdentityId, bool isMember)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return false;

		bool changed = m_Authorization.SetMembership(m_State, ResolveTrustedIdentityId(playerId), targetIdentityId, isMember);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	bool RequestAdminSetAdminRole(int playerId, string targetIdentityId, bool isAdmin)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return false;

		bool changed = m_Authorization.SetAdminRole(m_State, ResolveTrustedIdentityId(playerId), targetIdentityId, isAdmin);
		if (changed)
			MarkMajorCampaignChange();
		return changed;
	}

	string RequestAdminInspectZone(int playerId, string zoneId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "";

		return BuildZoneReport(zoneId);
	}

	protected HST_CampaignState CreateInitialCampaignState()
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		if (m_Preset)
			state.m_sPresetId = m_Preset.m_sPresetId;
		if (m_Settings)
			state.m_iCampaignSeed = m_Settings.m_Campaign.m_iCampaignSeed;
		if (m_Balance)
		{
			state.m_iFactionMoney = m_Balance.m_iStartingFactionMoney;
			state.m_iHR = m_Balance.m_iStartingHR;
		}

		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_SETUP;
		state.m_sLastPersistenceStatus = "new campaign created";
		return state;
	}

	protected void EnsureCampaignFoundation()
	{
		if (!m_State)
			return;

		m_State.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		if (m_Preset)
			m_State.m_sPresetId = m_Preset.m_sPresetId;
		if (m_State.m_iCampaignSeed == 0 && m_Settings)
			m_State.m_iCampaignSeed = m_Settings.m_Campaign.m_iCampaignSeed;
		SanitizeFactionKeys(m_State);

		if (m_State.m_aFactionPools.Count() == 0)
			HST_DefaultCatalog.AddDefaultFactionPools(m_State, m_Balance, m_Preset);
		if (m_State.m_aZones.Count() == 0)
			HST_DefaultCatalog.AddDefaultZones(m_State, m_Preset);
		if (m_State.m_aGarrisons.Count() == 0)
			HST_DefaultCatalog.AddDefaultGarrisons(m_State, m_Preset);
		if (m_Content)
			m_Content.EnsureGeneratedContent(m_State, m_Preset);
		if (m_Civilians)
			m_Civilians.EnsureCivilianZones(m_State);
		if (m_Arsenal && m_Arsenal.CleanupInvalidGarageRecords(m_State) > 0)
			m_State.m_sLastVehicleTargetStatus = "removed invalid saved vehicle/cargo records";

		if (m_State.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_SETUP && !m_State.m_bHQDeployed && m_HQ)
			m_HQ.BootstrapInitialHideout(m_State, HST_DefaultCatalog.GetDefaultHideoutId());
	}

	protected void SanitizeFactionKeys(HST_CampaignState state)
	{
		if (!state || !m_Preset)
			return;

		string occupier = m_Preset.m_sOccupierFactionKey;
		string invader = m_Preset.m_sInvaderFactionKey;
		string resistance = m_Preset.m_sResistanceFactionKey;
		if (resistance.IsEmpty())
			resistance = "FIA";
		bool assignedUnknownOccupier;
		foreach (HST_FactionPoolState factionPool : state.m_aFactionPools)
		{
			if (!factionPool)
				continue;

			if (factionPool.m_sFactionKey != resistance && factionPool.m_sFactionKey != occupier && factionPool.m_sFactionKey != invader)
			{
				if (!assignedUnknownOccupier)
				{
					factionPool.m_sFactionKey = occupier;
					assignedUnknownOccupier = true;
				}
				else
				{
					factionPool.m_sFactionKey = invader;
				}
			}
		}
		int occupierAttackResources;
		int occupierSupportResources;
		int invaderAttackResources;
		int invaderSupportResources;
		if (m_Balance)
		{
			occupierAttackResources = m_Balance.m_iStartingOccupierAttackPool;
			occupierSupportResources = m_Balance.m_iStartingOccupierSupportPool;
			invaderAttackResources = m_Balance.m_iStartingInvaderAttackPool;
			invaderSupportResources = m_Balance.m_iStartingInvaderSupportPool;
		}
		EnsureFactionPool(state, resistance, 0, 0);
		EnsureFactionPool(state, occupier, occupierAttackResources, occupierSupportResources);
		EnsureFactionPool(state, invader, invaderAttackResources, invaderSupportResources);

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			if (zone.m_sOwnerFactionKey != resistance && zone.m_sOwnerFactionKey != occupier && zone.m_sOwnerFactionKey != invader)
				zone.m_sOwnerFactionKey = occupier;
		}

		foreach (HST_GarrisonState garrison : state.m_aGarrisons)
		{
			if (!garrison)
				continue;

			if (garrison.m_sFactionKey != resistance && garrison.m_sFactionKey != occupier && garrison.m_sFactionKey != invader)
				garrison.m_sFactionKey = occupier;
		}
	}

	protected void EnsureFactionPool(HST_CampaignState state, string factionKey, int attackResources, int supportResources)
	{
		if (!state || factionKey.IsEmpty())
			return;

		foreach (HST_FactionPoolState factionPool : state.m_aFactionPools)
		{
			if (factionPool && factionPool.m_sFactionKey == factionKey)
				return;
		}

		HST_FactionPoolState createdPool = new HST_FactionPoolState();
		createdPool.m_sFactionKey = factionKey;
		createdPool.m_iAttackResources = attackResources;
		createdPool.m_iSupportResources = supportResources;
		state.m_aFactionPools.Insert(createdPool);
	}

	protected bool SelectInitialHideout_S(string hideoutId)
	{
		bool changed = m_HQ.SelectInitialHideout(m_State, hideoutId);
		if (changed)
		{
			m_HQ.EnsureRuntimeObjects(m_State);
			MarkMajorCampaignChange();
		}
		return changed;
	}

	protected bool StartMission_S(string missionId, string targetZoneId, bool forceDebug = false)
	{
		if (!m_State)
			return false;

		HST_ActiveMissionState mission;
		if (forceDebug)
			mission = m_Missions.StartForced(m_State, m_Preset, missionId, targetZoneId);
		else
			mission = m_Missions.Start(m_State, m_Preset, missionId, targetZoneId);

		if (!mission)
			return false;

		HST_MissionDefinition definition = m_Missions.FindDefinition(missionId);
		if (m_Objectives)
			m_Objectives.InitializeMission(m_State, m_Preset, definition, mission, m_Content);
		if (m_MissionRuntime)
			m_MissionRuntime.InitializeMissionRuntime(m_State, m_Preset, definition, mission, m_Content);
		if (m_PhysicalWar && ShouldForceMissionTargetZoneActive(mission))
			m_PhysicalWar.EnsureMissionTargetZoneActive(m_State, mission.m_sTargetZoneId, m_ZoneCompositions);

		MarkMajorCampaignChange();
		BroadcastMissionEvent("created", mission, definition);
		return true;
	}

	protected bool ShouldForceMissionTargetZoneActive(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_sTargetZoneId.IsEmpty())
			return false;

		return mission.m_sRuntimePrimitive == "clear_area" || mission.m_sRuntimePrimitive == "hold_area";
	}

	protected void MarkMajorCampaignChange(bool refreshMarkers = true)
	{
		if (refreshMarkers)
		{
			if (m_State)
				m_iLastMarkerRefreshSecond = m_State.m_iElapsedSeconds;
			m_bDeferredMarkerRefresh = false;
			RefreshCampaignMarkers();
		}

		if (m_Persistence)
			m_Persistence.MarkMajorChange();

		PublishMissionIntelToPlayers();
	}

	protected bool ResolveThrottledMarkerRefresh(bool markerStateChanged, bool forceImmediate)
	{
		if (!markerStateChanged && !m_bDeferredMarkerRefresh)
			return false;
		if (!m_State)
			return markerStateChanged;

		if (markerStateChanged)
			m_bDeferredMarkerRefresh = true;

		int elapsedSeconds = m_State.m_iElapsedSeconds;
		if (forceImmediate)
		{
			m_iLastMarkerRefreshSecond = elapsedSeconds;
			m_bDeferredMarkerRefresh = false;
			return true;
		}

		if (elapsedSeconds < m_iLastMarkerRefreshSecond + MARKER_REFRESH_THROTTLE_SECONDS)
			return false;

		m_iLastMarkerRefreshSecond = elapsedSeconds;
		m_bDeferredMarkerRefresh = false;
		return true;
	}

	protected void RefreshCampaignMarkers()
	{
		if (m_MapMarkers)
			m_MapMarkers.RebuildAllMarkers(m_State, m_Preset);
	}

	protected void BroadcastMissionEvent(string eventType, HST_ActiveMissionState mission, HST_MissionDefinition definition)
	{
		if (!mission)
			return;

		string title = ResolveMissionTitle(mission, definition);
		string summary = string.Format("%1: %2", MissionEventLabel(eventType), title);
		if (eventType == "convoy_moving")
			summary = "Convoy on the move: " + BuildConvoyMovingMessage(mission);
		string payload = string.Format("HST_MISSION_EVENT|%1|%2|%3|%4|%5|%6|%7|%8", eventType, mission.m_sInstanceId, PayloadText(title), MissionStatusLabel(mission.m_eStatus), mission.m_iRemainingSeconds, PayloadText(BuildMissionProgressText(mission)), PayloadText(ResolveMissionFailureText(mission, definition)), ResolveMissionMarkerId(mission));
		if (eventType == "created")
			mission.m_bCreatedNotificationSent = true;
		else if (eventType == "completed")
			mission.m_bCompletedNotificationSent = true;
		else if (eventType == "failed")
			mission.m_bFailedNotificationSent = true;
		else if (eventType == "expired")
			mission.m_bExpiredNotificationSent = true;
		HST_CommandMenuRequestComponent.BroadcastMissionEvent(payload, summary);
		PublishMissionIntelToPlayers();
	}

	protected string BuildConvoyMovingMessage(HST_ActiveMissionState mission)
	{
		if (!mission || !m_State)
			return "enemy convoy is moving toward its target.";

		string factionKey = "Enemy";
		HST_ZoneState zone = m_State.FindZone(mission.m_sTargetZoneId);
		if (zone && !zone.m_sOwnerFactionKey.IsEmpty())
			factionKey = zone.m_sOwnerFactionKey;

		string destinationName = ResolveZoneDisplayName(mission.m_sTargetZoneId);
		return string.Format("%1 convoy is moving toward %2.", factionKey, destinationName);
	}

	protected void BroadcastPendingMissionOutcomeEvents()
	{
		foreach (HST_ActiveMissionState mission : m_State.m_aActiveMissions)
		{
			if (!mission)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;

			HST_MissionDefinition definition = m_Missions.FindDefinition(mission.m_sMissionId);
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED && !mission.m_bExpiredNotificationSent)
				BroadcastMissionEvent("expired", mission, definition);
			else if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED && !mission.m_bCompletedNotificationSent)
				BroadcastMissionEvent("completed", mission, definition);
			else if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED && !mission.m_bFailedNotificationSent)
				BroadcastMissionEvent("failed", mission, definition);
		}
	}

	protected void BroadcastPendingMissionRuntimeEvents()
	{
		foreach (HST_ActiveMissionState mission : m_State.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;

			HST_MissionDefinition definition = m_Missions.FindDefinition(mission.m_sMissionId);
			if (mission.m_sLastRuntimeEventKey == "convoy_moving_pending")
			{
				BroadcastMissionEvent("convoy_moving", mission, definition);
				mission.m_sLastRuntimeEventKey = "convoy_moving_sent";
			}
			else if (mission.m_sLastRuntimeEventKey == "convoy_complete")
			{
				BroadcastMissionEvent("convoy_secured", mission, definition);
				mission.m_sLastRuntimeEventKey = "convoy_secured_sent";
			}
		}
	}

	protected void PublishMissionIntelToPlayers()
	{
		HST_CommandMenuRequestComponent.BroadcastMissionIntel(BuildMissionIntelPayload());
	}

	protected void BroadcastSupportChangeNotifications()
	{
		if (!m_State)
			return;

		foreach (HST_SupportRequestState request : m_State.m_aSupportRequests)
		{
			if (!request)
				continue;

			if (!ShouldBroadcastSupportNotification(request))
				continue;

			if (request.m_iRequestedAtSecond > 0 && m_State.m_iElapsedSeconds - request.m_iRequestedAtSecond > 3)
				continue;

			string title = "Enemy Support";
			string sourceName = ResolveZoneDisplayName(request.m_sSourceZoneId);
			string targetName = ResolveZoneDisplayName(request.m_sTargetZoneId);
			string supportLabel = SupportRequestTypeLabel(request.m_eType);
			string message = string.Format("%1 %2 moving from %3 to %4.", request.m_sFactionKey, supportLabel, sourceName, targetName);
			if (!ShouldBroadcastNotificationInPlayerBubble("support_" + request.m_sRequestId, "enemy", request.m_sTargetZoneId, request.m_vTargetPosition))
				continue;

			BroadcastNotification("support_" + request.m_sRequestId + "_" + string.Format("%1", request.m_eStatus), "enemy", "warning", title, message, request.m_sTargetZoneId, "", request.m_vTargetPosition, 6.0);
		}
	}

	protected void BroadcastEnemyOrderChangeNotifications()
	{
		if (!m_State)
			return;

		foreach (HST_EnemyOrderState order : m_State.m_aEnemyOrders)
		{
			if (!order)
				continue;

			if (!ShouldBroadcastEnemyOrderNotification(order))
				continue;

			if (order.m_iCreatedAtSecond > 0 && m_State.m_iElapsedSeconds - order.m_iCreatedAtSecond > 3)
				continue;

			string title = "Enemy Operation";
			string targetName = ResolveZoneDisplayName(order.m_sTargetZoneId);
			string orderLabel = EnemyOrderTypeLabel(order.m_eType);
			string message = string.Format("%1 %2 targeting %3.", order.m_sFactionKey, orderLabel, targetName);
			if (!ShouldBroadcastNotificationInPlayerBubble("enemy_order_" + order.m_sOrderId, "enemy", order.m_sTargetZoneId, ResolveZonePosition(order.m_sTargetZoneId)))
				continue;

			BroadcastNotification("enemy_order_" + order.m_sOrderId + "_" + string.Format("%1", order.m_eStatus), "enemy", "warning", title, message, order.m_sTargetZoneId, "", ResolveZonePosition(order.m_sTargetZoneId), 6.0);
		}
	}

	protected bool ShouldBroadcastSupportNotification(HST_SupportRequestState request)
	{
		if (!request)
			return false;

		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_PATROL_SWEEP || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_TRANSPORT || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP)
			return false;

		if (IsSupportRequestLinkedToEnemyOrder(request.m_sRequestId))
			return false;

		return request.m_eType == HST_ESupportRequestType.HST_SUPPORT_QRF || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_TROOP_LANDING || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55;
	}

	protected bool ShouldBroadcastEnemyOrderNotification(HST_EnemyOrderState order)
	{
		if (!order)
			return false;

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL || order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON || order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK)
			return false;

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK && !order.m_sSupportRequestId.IsEmpty())
			return false;

		return true;
	}

	protected bool IsSupportRequestLinkedToEnemyOrder(string requestId)
	{
		if (!m_State || requestId.IsEmpty())
			return false;

		foreach (HST_EnemyOrderState order : m_State.m_aEnemyOrders)
		{
			if (order && order.m_sSupportRequestId == requestId)
				return true;
		}

		return false;
	}

	protected bool BroadcastCaptureChangeNotifications()
	{
		if (!m_ZoneCapture)
			return false;

		bool markerRefreshNeeded = false;
		array<ref HST_ZoneCaptureNotification> notifications = {};
		m_ZoneCapture.DrainPendingNotifications(notifications);
		foreach (HST_ZoneCaptureNotification notification : notifications)
		{
			if (!notification)
				continue;

			bool ownershipFlip = IsOwnershipFlipNotification(notification.m_sEventId, "capture");
			if (ownershipFlip)
				markerRefreshNeeded = true;
			else if (!ShouldBroadcastNotificationInPlayerBubble(notification.m_sEventId, "capture", notification.m_sZoneId, notification.m_vPosition))
				continue;

			BroadcastNotification(notification.m_sEventId, "capture", notification.m_sSeverity, notification.m_sTitle, notification.m_sMessage, notification.m_sZoneId, "", notification.m_vPosition, 6.0);
		}

		return markerRefreshNeeded;
	}

	protected bool ShouldBroadcastNotificationInPlayerBubble(string eventId, string category, string zoneId, vector position)
	{
		if (IsOwnershipFlipNotification(eventId, category))
			return true;

		if (!m_State)
			return false;

		HST_ZoneState zone = m_State.FindZone(zoneId);
		if (zone && zone.m_bActive)
			return true;

		if (IsZeroVector(position) && zone)
			position = zone.m_vPosition;

		return HST_WorldPositionService.IsPositionInsidePlayerEventBubble(position);
	}

	protected bool IsOwnershipFlipNotification(string eventId, string category)
	{
		if (category == "capture" && eventId.Contains("_secured"))
			return true;

		return false;
	}

	protected void BroadcastNotification(string eventId, string category, string severity, string title, string message, string zoneId, string missionId, vector position, float durationSeconds)
	{
		string payload = string.Format("HST_NOTIFICATION|%1|%2|%3|%4|%5|%6|%7|%8|%9", eventId, category, severity, PayloadText(title), PayloadText(message), zoneId, missionId, position, durationSeconds);
		HST_CommandMenuRequestComponent.BroadcastNotification(payload, title + ": " + message);
		Print(string.Format("h-istasi notification | %1 | %2", title, message));
	}

	protected string ResolveResultSeverity(string result)
	{
		if (result.Contains("failed") || result.Contains("denied") || result.Contains("not ready") || result.Contains("no living player"))
			return "danger";
		if (result.Contains("complete") || result.Contains("unloaded") || result.Contains("deposited") || result.Contains("captured"))
			return "success";
		if (result.Contains("no eligible") || result.Contains("no stored") || result.Contains("stand near"))
			return "warning";

		return "info";
	}

	protected vector ResolveZonePosition(string zoneId)
	{
		if (!m_State || zoneId.IsEmpty())
			return "0 0 0";

		HST_ZoneState zone = m_State.FindZone(zoneId);
		if (!zone)
			return "0 0 0";

		return zone.m_vPosition;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected string ResolveZoneDisplayName(string zoneId)
	{
		if (!m_State || zoneId.IsEmpty())
			return "unknown location";

		HST_ZoneState zone = m_State.FindZone(zoneId);
		if (!zone)
			return HumanizeId(zoneId);
		if (!zone.m_sDisplayName.IsEmpty())
			return zone.m_sDisplayName;

		return HumanizeId(zone.m_sZoneId);
	}

	protected string HumanizeId(string value)
	{
		if (value.IsEmpty())
			return "unknown";

		string text = value;
		text.Replace("_", " ");
		return text;
	}

	protected string SupportRequestTypeLabel(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return "QRF";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_PATROL_SWEEP)
			return "patrol sweep";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_TRANSPORT)
			return "transport";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP)
			return "supply drop";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return "search-and-destroy team";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE)
			return "suppressive fire";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_TROOP_LANDING)
			return "troop landing";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_EVACUATION)
			return "evacuation";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU)
			return "GBU strike";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return "UMPK strike";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return "cruise missile strike";

		return "support";
	}

	protected string EnemyOrderTypeLabel(HST_EEnemyOrderType orderType)
	{
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL)
			return "patrol";
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK)
			return "roadblock";
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF)
			return "QRF";
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON)
			return "garrison rebuild";
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
			return "counterattack";
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
			return "Petros attack";
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL)
			return "support call";

		return "operation";
	}

	protected vector ResolveMissionIntelPosition(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "0 0 0";

		if (mission.m_vTargetPosition[0] != 0 || mission.m_vTargetPosition[1] != 0 || mission.m_vTargetPosition[2] != 0)
			return mission.m_vTargetPosition;

		HST_GeneratedSiteState site = m_State.FindGeneratedSite(mission.m_sSiteId);
		if (site)
			return site.m_vPosition;

		HST_ZoneState zone = m_State.FindZone(mission.m_sTargetZoneId);
		if (zone)
			return zone.m_vPosition;

		return m_State.m_vHQPosition;
	}

	protected string ResolveMissionTitle(HST_ActiveMissionState mission, HST_MissionDefinition definition)
	{
		if (mission && !mission.m_sDisplayName.IsEmpty())
			return mission.m_sDisplayName;
		if (definition)
			return definition.m_sDisplayName;
		if (mission)
			return mission.m_sMissionId;
		return "Mission";
	}

	protected string ResolveMissionMarkerId(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "";
		if (!mission.m_sMarkerId.IsEmpty())
			return mission.m_sMarkerId;
		return "hst_mission_" + mission.m_sInstanceId;
	}

	protected string BuildMissionProgressText(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "unknown";

		int complete;
		int total;
		foreach (HST_MissionObjectiveState objective : m_State.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			total++;
			if (objective.m_bComplete)
				complete++;
		}

		string phase = mission.m_sRuntimePhase;
		if (phase.IsEmpty())
			phase = "active";
		string eta = "";
		if (mission.m_iRuntimeETASeconds > 0 && ShouldShowMissionEta(mission))
			eta = string.Format(" / ETA %1s", mission.m_iRuntimeETASeconds);

		if (phase == "convoy_static")
			phase = "physical convoy staged";
		else if (phase == "convoy_staging")
			phase = "convoy staging";
		else if (phase == "convoy_moving")
			phase = "convoy moving";
		else if (phase == "convoy_contact")
			phase = "convoy contact";
		else if (phase == "convoy_eliminated")
			phase = "convoy neutralized";
		else if (phase == "convoy_arrived")
			phase = "convoy arrived";
		else
			phase.Replace("_", " ");

		string progress = string.Format("%1%2 | objectives %3/%4", phase, eta, complete, total);
		if (mission.m_sRuntimePrimitive == "convoy_intercept")
		{
			int neutralized;
			int required;
			ResolveConvoyCrewObjectiveProgress(mission, neutralized, required);
			progress = string.Format("%1%2 | crew groups neutralized %3/%4", phase, eta, neutralized, required);
			if (!mission.m_sRuntimeFailureReason.IsEmpty())
				progress = progress + " | " + mission.m_sRuntimeFailureReason;
			if (!mission.m_sConvoyOutcomeSummary.IsEmpty())
				progress = progress + " | " + mission.m_sConvoyOutcomeSummary;
			return progress;
		}
		if (mission.m_iRequiredCargoCount > 0)
			progress = progress + string.Format(" | cargo %1/%2", mission.m_iRecoveredCargoCount, mission.m_iRequiredCargoCount);
		if (mission.m_iRequiredCaptiveCount > 0)
			progress = progress + string.Format(" | captives %1/%2", mission.m_iExtractedCaptiveCount, mission.m_iRequiredCaptiveCount);
		if (mission.m_iRuntimeDestroyedCount > 0 || mission.m_iRuntimeDeliveryCount > 0)
			progress = progress + string.Format(" | delivered %1 destroyed %2", mission.m_iRuntimeDeliveryCount, mission.m_iRuntimeDestroyedCount);
		return progress;
	}

	protected bool ShouldShowMissionEta(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;

		if (mission.m_sRuntimePrimitive != "convoy_intercept")
			return true;

		string phase = mission.m_sRuntimePhase;
		return phase == "convoy_staging" || phase == "convoy_moving";
	}

	protected void ResolveConvoyCrewObjectiveProgress(HST_ActiveMissionState mission, out int neutralized, out int required)
	{
		neutralized = 0;
		required = 0;
		if (!mission || !m_State)
			return;

		foreach (HST_MissionObjectiveState objective : m_State.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_sTargetId != "convoy")
				continue;

			neutralized = objective.m_iCurrentCount;
			required = objective.m_iRequiredCount;
			return;
		}
	}

	protected string BuildMissionAssetIntelLabel(HST_MissionAssetState asset)
	{
		if (!asset)
			return "Mission asset";

		string role = MissionAssetReadableRole(asset);
		string state = "at pickup";
		if (asset.m_bDestroyed)
			state = "destroyed";
		else if (asset.m_bDelivered)
			state = "delivered";
		else if (asset.m_bAttachedToCarrier && !asset.m_sCarriedByVehicleId.IsEmpty())
			state = "loaded in carrier";
		else if (asset.m_bPickedUp)
			state = "being carried";
		else if (asset.m_sKind == "character")
			state = "alive";
		else if (asset.m_sKind == "target")
			state = "intact";

		return role + ": " + state;
	}

	protected string MissionAssetReadableRole(HST_MissionAssetState asset)
	{
		if (!asset)
			return "Mission asset";

		if (asset.m_sRole == "hvt")
			return "HVT";
		if (asset.m_sRole == "destroy_target")
			return "Target";
		if (asset.m_sRole == "logistics_cargo")
			return "Recovered cargo";
		if (asset.m_sRole == "city_supplies")
			return "City supplies";
		if (asset.m_sRole == "captive")
			return "Captive";
		if (asset.m_sRole == "convoy_vehicle")
			return "Convoy vehicle";
		if (asset.m_sRole == "convoy_payload")
			return "Convoy payload";
		if (asset.m_sRole == "convoy_captive")
			return "Convoy prisoner";

		string role = asset.m_sRole;
		if (role.IsEmpty())
			role = asset.m_sKind;
		if (role.IsEmpty())
			return "Mission asset";
		role.Replace("_", " ");
		return role;
	}

	protected int MissionAssetProgressValue(HST_MissionAssetState asset)
	{
		if (!asset)
			return 0;
		if (asset.m_bDestroyed || asset.m_bDelivered || asset.m_bPickedUp)
			return 1;
		return 0;
	}

	protected bool MissionAssetCompleteValue(HST_MissionAssetState asset)
	{
		if (!asset)
			return false;
		if (asset.m_sKind == "target" || asset.m_sKind == "character")
			return asset.m_bDestroyed;
		if (asset.m_sRole == "convoy_payload")
			return asset.m_bPickedUp;
		if (asset.m_sRole == "convoy_captive")
			return asset.m_bDelivered;
		if (asset.m_sKind == "vehicle")
			return asset.m_bDelivered || asset.m_bDestroyed;
		if (asset.m_sKind == "cargo" || asset.m_sKind == "captive")
			return asset.m_bDelivered;
		return asset.m_bSpawned;
	}

	protected bool MissionAssetFailedValue(HST_MissionAssetState asset)
	{
		if (!asset)
			return false;
		if (asset.m_sKind == "target" || asset.m_sKind == "character")
			return false;
		return asset.m_bDestroyed;
	}

	protected string ResolveMissionFailureText(HST_ActiveMissionState mission, HST_MissionDefinition definition)
	{
		if (mission && !mission.m_sRuntimeFailureReason.IsEmpty())
			return mission.m_sRuntimeFailureReason;
		if (definition)
			return definition.m_sFailureText;
		return "";
	}

	protected string MissionEventLabel(string eventType)
	{
		if (eventType == "created")
			return "Mission created";
		if (eventType == "completed")
			return "Mission completed";
		if (eventType == "failed")
			return "Mission failed";
		if (eventType == "expired")
			return "Mission expired";
		if (eventType == "loaded")
			return "Mission cargo loaded";
		if (eventType == "unloaded")
			return "Mission cargo unloaded";
		if (eventType == "delivered")
			return "Mission delivery complete";
		if (eventType == "captured")
			return "Mission vehicle captured";
		if (eventType == "sabotaged")
			return "Mission target sabotaged";
		if (eventType == "neutralized")
			return "Mission target neutralized";
		if (eventType == "freed")
			return "Mission captive freed";
		if (eventType == "convoy_moving")
			return "Convoy on the move";
		if (eventType == "convoy_secured")
			return "Convoy secured";
		return "Mission updated";
	}

	protected string MissionStatusLabel(HST_EMissionStatus status)
	{
		if (status == HST_EMissionStatus.HST_MISSION_ACTIVE)
			return "active";
		if (status == HST_EMissionStatus.HST_MISSION_SUCCEEDED)
			return "completed";
		if (status == HST_EMissionStatus.HST_MISSION_FAILED)
			return "failed";
		if (status == HST_EMissionStatus.HST_MISSION_EXPIRED)
			return "expired";
		return "available";
	}

	protected string MissionCategoryLabel(HST_MissionDefinition definition)
	{
		if (!definition)
			return "unknown";
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_ASSASSINATION)
			return "Assassination";
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return "Conquest";
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return "Convoy";
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
			return "Destroy";
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS)
			return "Logistics";
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_RESCUE)
			return "Rescue";
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DYNAMIC)
			return "Dynamic";
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			return "Support";
		return "Mission";
	}

	protected string PayloadText(string value)
	{
		value.Replace("|", "/");
		value.Replace("\n", " ");
		value.Replace("\r", " ");
		return value;
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

		string identityId = ResolveTrustedIdentityId(playerId);
		EnsureDeveloperFallbackAdmin(identityId);
		return m_Authorization.CanUseAdminActions(m_State, identityId);
	}

	bool HasResistanceAirSupportCapability()
	{
		if (!Replication.IsServer() || !m_State)
			return false;

		foreach (HST_GarageVehicleState vehicle : m_State.m_aGarageVehicles)
		{
			if (!vehicle || vehicle.m_sPrefab.IsEmpty())
				continue;

			if (IsAircraftPrefab(vehicle.m_sPrefab))
				return true;
		}

		return false;
	}

	protected bool IsAirSupportType(HST_ESupportRequestType supportType)
	{
		return supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK || supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55;
	}

	protected bool IsAircraftPrefab(string prefab)
	{
		return prefab.Contains("Aircraft") || prefab.Contains("Airplane") || prefab.Contains("Plane") || prefab.Contains("Helicopter") || prefab.Contains("Helicopters") || prefab.Contains("UH") || prefab.Contains("AH") || prefab.Contains("Mi-") || prefab.Contains("KA-") || prefab.Contains("Ka-");
	}

	protected bool IsSettingsAdminIdentity(string identityId)
	{
		if (!m_Settings || !m_Settings.m_Membership || identityId.IsEmpty())
			return false;

		return m_Settings.m_Membership.m_aAdminIdentityIds.Contains(identityId);
	}

	protected bool IsDeveloperFallbackAdminIdentity(string identityId)
	{
		if (!m_Settings || !m_Settings.m_Debug || !m_Settings.m_Debug.m_bDebugMenuEnabled || identityId.IsEmpty())
			return false;

		if (HasAnyAdminPlayer())
			return false;

		return identityId == "workbench_player_1";
	}

	protected void EnsureDeveloperFallbackAdmin(string identityId)
	{
		if (!IsDeveloperFallbackAdminIdentity(identityId))
			return;

		HST_PlayerState player = m_State.FindPlayer(identityId);
		if (!player)
			return;

		player.m_bAdmin = true;
		player.m_bMember = true;
		player.m_bGuest = false;
		Print(string.Format("h-istasi admin | granted Workbench fallback admin to %1", identityId));
		MarkMajorCampaignChange();
	}

	protected bool HasAnyAdminPlayer()
	{
		if (!m_State)
			return false;

		foreach (HST_PlayerState player : m_State.m_aPlayers)
		{
			if (player && player.m_bAdmin)
				return true;
		}

		return false;
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

	protected bool IsLivingEntity(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected bool IsPlayerWithinHQInteractionRadius(int playerId)
	{
		if (!m_State || !m_State.m_bHQDeployed || !m_Balance || playerId <= 0)
			return false;

		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		if (!playerEntity)
			return false;

		int radius = Math.Max(1, m_Balance.m_iHQInteractionRadiusMeters);
		return DistanceSq2D(playerEntity.GetOrigin(), m_State.m_vHQPosition) <= radius * radius;
	}

	protected string BuildHQInteractionDenied(string prefix)
	{
		int radius;
		if (m_Balance)
			radius = Math.Max(1, m_Balance.m_iHQInteractionRadiusMeters);
		else
			radius = 50;

		return string.Format("%1: must be within %2m of FIA HQ", prefix, radius);
	}

	protected bool ApplyCompletedMissionOutcome(HST_MissionDefinition definition, HST_ActiveMissionState activeMission)
	{
		if (!definition || !activeMission || activeMission.m_sTargetZoneId.IsEmpty())
			return false;

		HST_ZoneState zone = m_State.FindZone(activeMission.m_sTargetZoneId);
		if (!zone)
			return false;

		if (zone.m_sOwnerFactionKey != m_Preset.m_sResistanceFactionKey)
			m_Economy.AddAggression(m_State, zone.m_sOwnerFactionKey, ResolveMissionSuccessAggression(definition));

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return m_ZoneCapture.AddResistanceCaptureProgress(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, zone.m_sZoneId, 60, 15, m_Garrisons, m_EnemyCommander, m_EnemyDirector, m_SupportRequests);

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			return m_Towns.AddSupport(m_State, zone.m_sZoneId, 25);

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
		{
			m_EnemyDirector.AddResources(m_State, zone.m_sOwnerFactionKey, -12, -6);
			if (definition.m_sMissionId == "destroy_radio_tower" || definition.m_sMissionId == "dynamic_stop_tower_rebuild")
				m_State.m_iHQKnowledge = Math.Max(0, m_State.m_iHQKnowledge - 20);
			if (zone.m_sOwnerFactionKey != m_Preset.m_sResistanceFactionKey)
				m_ZoneCapture.AddResistanceCaptureProgress(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, zone.m_sZoneId, 35, 10, m_Garrisons, m_EnemyCommander, m_EnemyDirector, m_SupportRequests);
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
			{
				if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
					return m_Towns.AddSupport(m_State, zone.m_sZoneId, 25);

				return m_ZoneCapture.AddResistanceCaptureProgress(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, zone.m_sZoneId, 50, 10, m_Garrisons, m_EnemyCommander, m_EnemyDirector, m_SupportRequests);
			}

			if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
				return m_Towns.AddSupport(m_State, zone.m_sZoneId, 10);

			return m_ZoneCapture.AddResistanceCaptureProgress(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, zone.m_sZoneId, 20, 5, m_Garrisons, m_EnemyCommander, m_EnemyDirector, m_SupportRequests);
		}

		return false;
	}

	protected bool ApplyFailedMissionOutcome(HST_MissionDefinition definition, HST_ActiveMissionState activeMission)
	{
		if (!definition || !activeMission)
			return false;

		if (definition.m_sMissionId == "assassinate_traitor")
		{
			m_State.m_iHQKnowledge = Math.Min(100, m_State.m_iHQKnowledge + 45);
			return true;
		}

		if (definition.m_sMissionId == "dynamic_defend_petros")
		{
			m_State.m_iHQKnowledge = 100;
			m_State.m_iLastHQAttackSecond = m_State.m_iElapsedSeconds;
			return true;
		}

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT && !activeMission.m_sTargetZoneId.IsEmpty())
		{
			HST_ZoneState zone = m_State.FindZone(activeMission.m_sTargetZoneId);
			if (zone)
				return m_Towns.AddSupport(m_State, zone.m_sZoneId, -10);
		}

		if (!activeMission.m_sTargetZoneId.IsEmpty())
		{
			HST_ZoneState targetZone = m_State.FindZone(activeMission.m_sTargetZoneId);
			if (targetZone && targetZone.m_sOwnerFactionKey != m_Preset.m_sResistanceFactionKey)
			{
				m_Economy.AddAggression(m_State, targetZone.m_sOwnerFactionKey, Math.Max(1, definition.m_iFailureAggression / 2));
				return true;
			}
		}

		return false;
	}

	protected int ResolveMissionSuccessAggression(HST_MissionDefinition definition)
	{
		if (!definition)
			return 1;

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return 8;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
			return 6;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return 5;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DYNAMIC)
			return 4;

		return 2;
	}

	protected string SelectDefaultMissionForZone(HST_ZoneState zone)
	{
		if (!zone)
			return "dynamic_minor_city_task";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN && zone.m_sOwnerFactionKey != m_Preset.m_sResistanceFactionKey)
			return "conquest_town";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			return "conquest_resource";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			return "conquest_factory";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST)
			return "conquest_outpost";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return "destroy_radio_tower";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD)
			return "conquest_airfield";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return "conquest_seaport";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT || zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			return "destroy_or_steal_armor";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN && zone.m_sOwnerFactionKey == m_Preset.m_sResistanceFactionKey)
			return "support_city_supplies";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			return "dynamic_city_flip_battle";

		return "dynamic_minor_city_task";
	}

	protected string SelectMissionForZone(HST_ZoneState zone)
	{
		if (!zone || !m_Missions)
			return SelectDefaultMissionForZone(zone);

		array<ref HST_MissionDefinition> definitions = m_Missions.GetDefinitions();
		HST_MissionDefinition selectedDefinition;
		int bestScore = -99999;
		foreach (HST_MissionDefinition definition : definitions)
		{
			if (!definition)
				continue;

			if (!m_Missions.CanStart(m_State, m_Preset, definition.m_sMissionId, zone.m_sZoneId))
				continue;

			int score = MissionCandidateScore(definition, zone);
			score += m_State.m_iCampaignSeed - (m_State.m_iCampaignSeed / 7) * 7;
			score -= CountActiveMissionsAtZone(zone.m_sZoneId) * 100;
			if (score > bestScore)
			{
				bestScore = score;
				selectedDefinition = definition;
			}
		}

		if (selectedDefinition)
			return selectedDefinition.m_sMissionId;

		return SelectDefaultMissionForZone(zone);
	}

	protected int MissionCandidateScore(HST_MissionDefinition definition, HST_ZoneState zone)
	{
		int score = 10 + definition.m_iRewardMoney / 100;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			score += 40;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
			score += 25;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS)
			score += 20;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			score += 35;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			score += 12;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_ASSASSINATION)
			score += 8;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_RESCUE)
			score += 10;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN && definition.m_sMissionId.Contains("town"))
			score += 35;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE && definition.m_sMissionId.Contains("resource"))
			score += 35;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY && definition.m_sMissionId.Contains("factory"))
			score += 35;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER && definition.m_sMissionId.Contains("radio"))
			score += 45;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD && definition.m_sMissionId.Contains("airfield"))
			score += 40;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT && definition.m_sMissionId.Contains("seaport"))
			score += 40;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST && definition.m_sMissionId.Contains("outpost"))
			score += 35;
		if (zone.m_sOwnerFactionKey == m_Preset.m_sResistanceFactionKey && definition.m_eCategory != HST_EMissionCategory.HST_MISSION_SUPPORT)
			score -= 80;
		if (zone.m_sOwnerFactionKey != m_Preset.m_sResistanceFactionKey && definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			score -= 80;

		return score;
	}

	protected HST_ZoneState SelectRandomMissionZone()
	{
		HST_ZoneState selectedZone;
		int bestScore = -99999;
		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (!zone || HasActiveMissionForZone(zone.m_sZoneId))
				continue;

			HST_GeneratedSiteState site;
			if (m_Content)
				site = m_Content.FindPrimarySiteForZone(m_State, zone.m_sZoneId);

			if (site && !site.m_bValid)
				continue;

			int score = zone.m_iIncomeValue + zone.m_iResistanceCaptureProgress;
			if (zone.m_sOwnerFactionKey != m_Preset.m_sResistanceFactionKey)
				score += 50;
			else
				score += 15;

			if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
				score += 20;

			score -= CountActiveMissionsAtZone(zone.m_sZoneId) * 100;
			if (score > bestScore)
			{
				selectedZone = zone;
				bestScore = score;
			}
		}

		return selectedZone;
	}

	protected string SelectDebugMissionTargetZoneId(HST_MissionDefinition definition)
	{
		if (!definition || !m_State || !m_Missions)
			return "";

		HST_ZoneState selectedZone;
		int bestScore = -99999;
		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (!zone || zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
				continue;

			if (!m_Missions.CanForceStart(m_State, m_Preset, definition.m_sMissionId, zone.m_sZoneId))
				continue;

			int score = zone.m_iPriority + zone.m_iIncomeValue / 5;
			if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			{
				if (zone.m_sOwnerFactionKey == m_Preset.m_sResistanceFactionKey)
					score += 100;
				else
					score -= 30;
			}
			else
			{
				if (zone.m_sOwnerFactionKey != m_Preset.m_sResistanceFactionKey)
					score += 100;
				else
					score -= 30;
			}

			if (zone.m_bActive)
				score += 10;

			score -= CountActiveMissionsAtZone(zone.m_sZoneId) * 50;

			if (score > bestScore)
			{
				selectedZone = zone;
				bestScore = score;
			}
		}

		if (selectedZone)
			return selectedZone.m_sZoneId;

		return "";
	}

	protected string ResolveMissionInstanceId(string instanceId)
	{
		if (!instanceId.IsEmpty())
			return instanceId;

		foreach (HST_ActiveMissionState mission : m_State.m_aActiveMissions)
		{
			if (mission && !IsPersistenceSmokeMission(mission) && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				return mission.m_sInstanceId;
		}

		return "";
	}

	protected bool HasActiveMissionForZone(string zoneId)
	{
		return CountActiveMissionsAtZone(zoneId) > 0;
	}

	protected int CountActiveMissionsAtZone(string zoneId)
	{
		int count;
		foreach (HST_ActiveMissionState mission : m_State.m_aActiveMissions)
		{
			if (mission && !IsPersistenceSmokeMission(mission) && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE && mission.m_sTargetZoneId == zoneId)
				count++;
		}

		return count;
	}

	protected string SelectHQSupportZoneId()
	{
		HST_ZoneState bestZone;
		float bestDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (!zone)
				continue;

			float distanceSq = DistanceSq2D(m_State.m_vHQPosition, zone.m_vPosition);
			if (distanceSq < bestDistanceSq)
			{
				bestZone = zone;
				bestDistanceSq = distanceSq;
			}
		}

		if (bestZone)
			return bestZone.m_sZoneId;

		return "";
	}

	protected string SelectPlayerSupportZoneId(int playerId)
	{
		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		if (!IsLivingEntity(playerEntity))
			return SelectHQSupportZoneId();

		string zoneId = SelectSupportZoneNearPosition(playerEntity.GetOrigin());
		if (!zoneId.IsEmpty())
			return zoneId;

		return SelectHQSupportZoneId();
	}

	protected string SelectSupportZoneNearPosition(vector position)
	{
		if (!m_State)
			return "";

		HST_ZoneState bestRelevantZone;
		HST_ZoneState bestAnyZone;
		float bestRelevantDistanceSq = 999999999.0;
		float bestAnyDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (!zone || zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT)
				continue;

			float distanceSq = DistanceSq2D(position, zone.m_vPosition);
			if (distanceSq < bestAnyDistanceSq)
			{
				bestAnyZone = zone;
				bestAnyDistanceSq = distanceSq;
			}

			if (!IsRelevantSupportTarget(zone))
				continue;

			if (distanceSq < bestRelevantDistanceSq)
			{
				bestRelevantZone = zone;
				bestRelevantDistanceSq = distanceSq;
			}
		}

		if (bestRelevantZone)
			return bestRelevantZone.m_sZoneId;
		if (bestAnyZone)
			return bestAnyZone.m_sZoneId;

		return "";
	}

	protected bool IsRelevantSupportTarget(HST_ZoneState zone)
	{
		if (!zone)
			return false;

		if (zone.m_bActive || zone.m_iResistanceCaptureProgress > 0)
			return true;
		if (HasActiveMissionForZone(zone.m_sZoneId))
			return true;
		if (m_Preset && zone.m_sOwnerFactionKey != m_Preset.m_sResistanceFactionKey)
			return true;

		return false;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
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

		string economySummary = string.Format("h-istasi campaign | phase %1 | money %2 | HR %3 | war level %4", m_State.m_ePhase, m_State.m_iFactionMoney, m_State.m_iHR, m_State.m_iWarLevel);
		economySummary = economySummary + string.Format(" | training %1 | persistence %2", m_State.m_iTrainingLevel, m_State.m_sLastPersistenceStatus);
		string zoneSummary = string.Format(" | resistance zones %1 | enemy zones %2 | active missions %3", resistanceZones, enemyZones, CountFoundationActiveMissions());
		string runtimeSummary = string.Format(" | active groups %1 | QRFs %2 | markers %3 | HQ %4", CountVisibleActiveGroups(), m_State.m_aQRFs.Count(), m_State.m_aMapMarkers.Count(), m_State.m_sHQHideoutId);
		runtimeSummary = runtimeSummary + string.Format(" | deployed %1 | runtime objects %2", m_State.m_bHQDeployed, m_State.m_bHQRuntimeObjectsSpawned);
		string alphaSummary = string.Format(" | sites %1 | support requests %2 | enemy orders %3 | civilian towns %4", m_State.m_aGeneratedSites.Count(), m_State.m_aSupportRequests.Count(), m_State.m_aEnemyOrders.Count(), m_State.m_aCivilianZones.Count());
		return economySummary + zoneSummary + runtimeSummary + alphaSummary;
	}

	protected string BuildFoundationStatusReport()
	{
		if (!m_State)
			return "h-istasi foundation | campaign state not ready";

		string hqHideout = m_State.m_sHQHideoutId;
		if (hqHideout.IsEmpty())
			hqHideout = "none";

		string persistence = m_State.m_sLastPersistenceStatus;
		if (persistence.IsEmpty())
			persistence = "none";

		string report = string.Format("h-istasi foundation | phase %1 | schema %2/%3", m_State.m_ePhase, m_State.m_iSchemaVersion, HST_CampaignState.SCHEMA_VERSION);
		report = report + string.Format(" | HQ hideout %1 | deployed %2 | Petros alive %3 | runtime objects %4", hqHideout, m_State.m_bHQDeployed, m_State.m_bPetrosAlive, m_State.m_bHQRuntimeObjectsSpawned);
		report = report + string.Format(" | active missions %1 | active groups %2", CountFoundationActiveMissions(), CountVisibleActiveGroups());
		report = report + " | persistence " + persistence;
		return report;
	}

	protected int CountFoundationActiveMissions()
	{
		if (!m_State)
			return 0;

		int count;
		foreach (HST_ActiveMissionState mission : m_State.m_aActiveMissions)
		{
			if (mission && !IsPersistenceSmokeMission(mission) && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				count++;
		}

		return count;
	}

	protected int CountVisibleActiveGroups()
	{
		if (!m_State)
			return 0;

		int count;
		foreach (HST_ActiveGroupState activeGroup : m_State.m_aActiveGroups)
		{
			if (activeGroup && !IsPersistenceSmokeGroup(activeGroup))
				count++;
		}

		return count;
	}

	protected bool IsPersistenceSmokeMission(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;

		return mission.m_sInstanceId.Contains(PERSISTENCE_SMOKE_PREFIX) || mission.m_sMissionId.Contains(PERSISTENCE_SMOKE_PREFIX);
	}

	protected bool IsPersistenceSmokeGroup(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		return activeGroup.m_sGroupId.Contains(PERSISTENCE_SMOKE_PREFIX);
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
		int captureRequired = HST_ZoneCaptureService.CAPTURE_PROGRESS_REQUIRED;
		if (m_Balance && m_Balance.m_iCaptureProgressRequired > 0)
			captureRequired = m_Balance.m_iCaptureProgressRequired;
		string captureSummary = string.Format(" | capture %1/%2 | income %3", zone.m_iResistanceCaptureProgress, captureRequired, zone.m_iIncomeValue);
		if (m_ZoneCapture)
		{
			HST_ZoneCaptureStatus status = m_ZoneCapture.BuildCaptureStatus(m_State, m_Preset, m_Balance, zone);
			string blockedReason = status.m_sBlockedReason;
			if (blockedReason.IsEmpty())
				blockedReason = "clear";
			captureSummary = captureSummary + string.Format(" | radius %1m | FIA %2 | enemies %3+%4v | %5", status.m_iCaptureRadiusMeters, status.m_iFIACountNearby, status.m_iEnemyCountNearby, status.m_iEnemyVehicleCountNearby, blockedReason);
		}
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

	protected string ResolveZoneLabel(HST_ZoneState zone)
	{
		if (!zone)
			return "unknown zone";

		if (!zone.m_sDisplayName.IsEmpty())
			return zone.m_sDisplayName;

		return HST_DefaultCatalog.GetZoneDisplayName(zone.m_sZoneId);
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
