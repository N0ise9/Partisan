[ComponentEditorProps(category: "h-istasi", description: "Server-authoritative h-istasi campaign coordinator")]
class HST_CampaignCoordinatorComponentClass : SCR_BaseGameModeComponentClass
{
}

class HST_CampaignCoordinatorComponent : SCR_BaseGameModeComponent
{
	protected static HST_CampaignCoordinatorComponent s_Instance;
	static const int MARKER_REFRESH_THROTTLE_SECONDS = 10;
	static const float SETUP_MAP_WORLD_MIN_X = 0.0;
	static const float SETUP_MAP_WORLD_MIN_Z = 0.0;
	static const float SETUP_MAP_WORLD_MAX_X = 12800.0;
	static const float SETUP_MAP_WORLD_MAX_Z = 12800.0;
	static const float SETUP_ZONE_FALLBACK_RADIUS_METERS = 150.0;
	static const string PERSISTENCE_SMOKE_PREFIX = "hst_smoke";
	static const string PHASE14_FINITE_PREFAB = "{6985327711303750}Prefabs/Objects/HST/HST_MissionProp_CitySupplies.et";
	static const string PHASE14_THRESHOLD_PREFAB = "{6985327711303760}Prefabs/Objects/HST/HST_MissionProp_ConvoyPayload.et";
	static const string PHASE14_BLOCKED_PREFAB = "{6985327711303710}Prefabs/Objects/HST/HST_MissionProp_DestroyTarget.et";
	static const string PHASE14_RAW_ASSET_PREFAB = "{EAE920BF596EBC07}Assets/Objects/Plane.xob";
	static const string PHASE15_SMOKE_VEHICLE_PREFAB = "{4AE9D080927D3CB9}Prefabs/Vehicles/Wheeled/S1203/S1203_base.et";
	static const string PHASE15_SMOKE_CARGO_PREFAB = "{6985327711303720}Prefabs/Objects/HST/HST_MissionProp_Cargo.et";

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
	protected ref HST_PlayerMapMarkerService m_PlayerMapMarkers;
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
	protected int m_iSetupPayloadDebugCount;
	protected int m_iSetupValidationDebugCount;
	protected int m_iSetupConfirmDebugCount;
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
		{
			m_Settings.ApplyTo(m_Preset, m_Balance);
			if (m_Settings.m_Debug)
				HST_UIDebug.SetRuntimeDebugEnabled(m_Settings.m_Debug.m_bDebugLoggingEnabled);

			SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
			if (gameMode && m_Settings.m_Features)
				gameMode.SetHistasiGameMasterBudgetsEnabled(m_Settings.m_Features.m_bGameMasterBudgetsEnabled, "campaign coordinator settings");
		}

		m_Economy = new HST_EconomyService();
		m_Missions = new HST_MissionService();
		m_Persistence = new HST_PersistenceService();
		m_PersistenceSmokeTest = new HST_PersistenceSmokeTestService();
		m_Authorization = new HST_AuthorizationService();
		m_Strategic = new HST_StrategicService();
		m_Arsenal = new HST_ArsenalService();
		m_EnemyDirector = new HST_EnemyDirectorService();
		m_HQ = new HST_HQService();
		if (m_HQ && m_Settings && m_Settings.m_Debug)
			m_HQ.SetDebugLoggingEnabled(m_Settings.m_Debug.m_bDebugLoggingEnabled);
		m_PlayerLifecycle = new HST_PlayerLifecycleService();
		m_Towns = new HST_TownService();
		m_Garrisons = new HST_GarrisonService();
		m_Recruitment = new HST_RecruitmentService();
		m_ZoneCapture = new HST_ZoneCaptureService();
		m_PlayerSpawn = new HST_PlayerSpawnService();
		m_PhysicalWar = new HST_PhysicalWarService();
		if (m_PhysicalWar && m_Settings && m_Settings.m_Debug)
			m_PhysicalWar.SetDebugLoggingEnabled(m_Settings.m_Debug.m_bDebugLoggingEnabled);
		m_ZoneCompositions = new HST_ZoneCompositionService();
		m_MapMarkers = new HST_MapMarkerService();
		if (m_MapMarkers && m_Settings && m_Settings.m_Debug)
			m_MapMarkers.SetDebugLoggingEnabled(m_Settings.m_Debug.m_bDebugLoggingEnabled);
		m_MapMarkers.BindNativeMapRefresh();
		m_PlayerMapMarkers = new HST_PlayerMapMarkerService();
		if (m_PlayerMapMarkers && m_Settings && m_Settings.m_Debug)
			m_PlayerMapMarkers.SetDebugLoggingEnabled(m_Settings.m_Debug.m_bDebugLoggingEnabled);
		if (m_PlayerMapMarkers && m_Settings && m_Settings.m_Features)
			m_PlayerMapMarkers.SetEnabled(m_Settings.m_Features.m_bShowPlayerMapMarkers);
		m_CommandUI = new HST_CommandUIService();
		m_Loot = new HST_LootService();
		m_BuildMode = new HST_BuildModeService();
		m_LoadoutEditor = new HST_LoadoutEditorService();
		if (m_LoadoutEditor && m_Settings)
			m_LoadoutEditor.SetDebugSettings(m_Settings.m_Debug);
		m_Content = new HST_GeneratedContentService();
		m_Objectives = new HST_MissionObjectiveService();
		m_MissionRuntime = new HST_MissionRuntimeService();
		if (m_MissionRuntime && m_Settings && m_Settings.m_Debug)
			m_MissionRuntime.SetDebugLoggingEnabled(m_Settings.m_Debug.m_bDebugLoggingEnabled);
		m_ConvoyOutcomes = new HST_ConvoyOutcomeService();
		m_SupportRequests = new HST_SupportRequestService();
		m_Civilians = new HST_CivilianService();
		m_EnemyCommander = new HST_EnemyCommanderService();

		m_State = m_Persistence.RestoreOrCreateCampaignState(CreateInitialCampaignState());
		EnsureCampaignFoundation();
		EvaluateCampaignOutcomeNow();
		m_Missions.SyncNextInstanceIdFromState(m_State);
		m_Persistence.CaptureAndTrackState(m_State);
		RefreshCampaignMarkers();

		ArmPlayerSpawnSweep(6);
		SetEventMask(owner, EntityEvent.FRAME);
		Print("h-istasi | campaign coordinator initialized");
	}

	override void OnDelete(IEntity owner)
	{
		if (m_MapMarkers)
			m_MapMarkers.UnbindNativeMapRefresh();
		if (m_PlayerMapMarkers)
			m_PlayerMapMarkers.ClearAll();

		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
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
		if (m_PlayerMapMarkers)
			m_PlayerMapMarkers.RequestRefresh(string.Format("player connected %1", playerId));
	}

	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		if (m_PlayerMapMarkers)
			m_PlayerMapMarkers.RequestRefresh(string.Format("player disconnected %1", playerId));
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer() || !m_State)
			return;

		m_PlayerSpawn.Tick(timeSlice);
		if (m_MapMarkers)
			m_MapMarkers.TickNativePublish(m_State, m_Preset, timeSlice);
		if (m_PlayerMapMarkers)
			m_PlayerMapMarkers.Tick(m_State, timeSlice);
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
		if (m_State.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_WON || m_State.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_LOST)
			return;

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
		bool enemyResourcesChanged = m_EnemyDirector.TickResources(m_State, m_Preset, m_Balance, elapsedSeconds);
		bool aggressionChanged = m_Economy.TickAggressionDecay(m_State, m_Preset, m_Balance, elapsedSeconds);
		bool civilianChanged = m_Civilians.Tick(m_State, elapsedSeconds);
		bool undercoverEnforcementChanged = TickUndercoverEnforcement();
		bool supportChanged = m_SupportRequests.Tick(m_State, m_Preset, m_Garrisons);
		bool enemyOrdersChanged = m_EnemyCommander.Tick(m_State, m_Preset, m_EnemyDirector, m_SupportRequests, m_Garrisons, elapsedSeconds);
		bool hqThreatChanged = m_HQ.TickHQThreat(m_State, m_Preset);
		bool petrosDefenseChanged = TickDefendPetros();
		bool hqRuntimeChanged = m_HQ.EnsureRuntimeObjects(m_State);
		bool physicalWarChanged = m_PhysicalWar.UpdateZoneActivation(m_State, m_Balance, m_Preset, m_EnemyDirector, m_ZoneCompositions);
		bool captureChanged = m_ZoneCapture.TickContestedCapture(m_State, m_Preset, m_Strategic, m_Economy, m_Balance, m_Garrisons, m_EnemyCommander, m_EnemyDirector, m_SupportRequests, elapsedSeconds);
		bool campaignOutcomeChanged = EvaluateCampaignOutcomeNow();
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
		bool anyStateChanged = missionChanged || objectiveChanged || missionRuntimeChanged;
		anyStateChanged = anyStateChanged || convoyRuntimeChanged || convoyOutcomeChanged || income > 0;
		anyStateChanged = anyStateChanged || enemyResourcesChanged || aggressionChanged || civilianChanged;
		anyStateChanged = anyStateChanged || undercoverEnforcementChanged || supportChanged || enemyOrdersChanged;
		anyStateChanged = anyStateChanged || hqThreatChanged || petrosDefenseChanged || hqRuntimeChanged;
		anyStateChanged = anyStateChanged || physicalWarChanged || captureChanged || campaignOutcomeChanged;
		anyStateChanged = anyStateChanged || civilianRuntimeChanged;

		bool markerStateChanged = missionChanged || missionRuntimeChanged || convoyRuntimeChanged;
		markerStateChanged = markerStateChanged || convoyOutcomeChanged || income > 0 || enemyResourcesChanged;
		markerStateChanged = markerStateChanged || aggressionChanged || supportMarkerChanged || enemyOrdersChanged;
		markerStateChanged = markerStateChanged || hqThreatChanged || petrosDefenseChanged || hqRuntimeChanged;
		markerStateChanged = markerStateChanged || captureMarkerChanged || campaignOutcomeChanged || physicalWarMarkerChanged;
		bool forceImmediateMarkerRefresh = missionChanged || hqRuntimeChanged || petrosDefenseChanged;
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

		if (m_Arsenal)
			m_Arsenal.PurgeBlockedArsenalItems(m_State);

		return m_CommandUI.BuildVisibleMenuPayload(m_State, m_Preset, m_MapMarkers, m_Arsenal, m_Recruitment, m_Settings, m_Balance, playerId, selectedTabId, lastResult, CanPlayerUseMemberActions(playerId), CanPlayerUseCommanderActions(playerId), CanPlayerUseAdminActions(playerId), m_ZoneCompositions, m_ZoneCapture);
	}

	string RequestVisibleMenuCommand(int playerId, string selectedTabId, string commandId, string argument = "")
	{
		if (!Replication.IsServer() || !m_CommandUI)
			return "h-istasi command | server coordinator not ready";

		string result = m_CommandUI.ExecuteVisibleCommand(this, playerId, commandId, argument);
		LogVisibleMenuCommandResult(playerId, selectedTabId, commandId, argument, result);
		return result;
	}

	string BuildSetupMapPayload(int playerId, string lastResult = "")
	{
		if (!Replication.IsServer() || !m_State)
			return "HST_SETUP|offline|0|0|0|0|12800|12800|server coordinator not ready|0\nEND";

		EnsureSetupRegisteredPlayer(playerId);
		bool setupActive = m_State.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_SETUP;
		bool isCommander = setupActive && CanPlayerUseCommanderActions(playerId);
		string phase = CampaignPhaseLabelForSetup(m_State.m_ePhase);
		string status = lastResult;
		if (status.IsEmpty())
		{
			if (setupActive && isCommander)
				status = "Select a location on the map to place the HQ";
			else if (setupActive)
				status = "Please wait, the commander is selecting the HQ location...";
			else
				status = "Campaign active";
		}

		string payload = string.Format(
			"HST_SETUP|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			phase,
			BoolPayload(setupActive),
			BoolPayload(isCommander),
			SETUP_MAP_WORLD_MIN_X,
			SETUP_MAP_WORLD_MIN_Z,
			SETUP_MAP_WORLD_MAX_X,
			SETUP_MAP_WORLD_MAX_Z,
			PayloadText(status),
			BoolPayload(IsDebugLoggingEnabled())
		);

		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (!zone)
				continue;

			payload = payload + "\nZONE";
			payload = payload + "|" + PayloadText(zone.m_sZoneId);
			payload = payload + "|" + PayloadText(ResolveZoneLabel(zone));
			payload = payload + "|" + PayloadText(ResolveZoneTypeLabel(zone.m_eType));
			payload = payload + "|" + PayloadText(zone.m_sOwnerFactionKey);
			payload = payload + "|" + string.Format("%1", zone.m_vPosition[0]);
			payload = payload + "|" + string.Format("%1", zone.m_vPosition[2]);
			payload = payload + "|" + string.Format("%1", ResolveSetupZoneRadius(zone));
			payload = payload + "|" + PayloadText(ResolveSetupZoneTone(zone));
		}

		m_iSetupPayloadDebugCount++;
		if (m_iSetupPayloadDebugCount <= 8 || (m_iSetupPayloadDebugCount % 20) == 0)
			DebugLog(string.Format("setup payload #%1 player=%2 phase=%3 setup=%4 commander=%5 zones=%6 status=%7", m_iSetupPayloadDebugCount, playerId, phase, setupActive, isCommander, m_State.m_aZones.Count(), status));

		return payload + "\nEND";
	}

	string RequestSetupValidateHQPosition(int playerId, float worldX, float worldZ)
	{
		if (!Replication.IsServer() || !m_State || !m_HQ)
			return BuildSetupResultPayload("validate", false, "0 0 0", "server coordinator not ready");

		EnsureSetupRegisteredPlayer(playerId);
		if (m_State.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_SETUP)
		{
			DebugLog(string.Format("setup validate rejected player=%1 pos=%2,%3 reason=campaign already complete phase=%4", playerId, worldX, worldZ, m_State.m_ePhase));
			return BuildSetupResultPayload("validate", false, "0 0 0", "campaign setup is already complete");
		}
		if (!CanPlayerUseCommanderActions(playerId))
		{
			DebugLog(string.Format("setup validate rejected player=%1 pos=%2,%3 reason=not commander commander=%4", playerId, worldX, worldZ, m_State.m_sCommanderIdentityId));
			return BuildSetupResultPayload("validate", false, "0 0 0", "commander authority required");
		}

		vector requestedPosition = "0 0 0";
		requestedPosition[0] = worldX;
		requestedPosition[2] = worldZ;

		m_iSetupValidationDebugCount++;
		DebugLog(string.Format("setup validate #%1 player=%2 requested=%3", m_iSetupValidationDebugCount, playerId, requestedPosition));
		vector resolvedPosition;
		string failure;
		if (!m_HQ.ValidateInitialHQPosition(m_State, requestedPosition, resolvedPosition, failure))
		{
			DebugLog(string.Format("setup validate #%1 rejected resolved=%2 failure=%3", m_iSetupValidationDebugCount, resolvedPosition, failure));
			return BuildSetupResultPayload("validate", false, resolvedPosition, failure);
		}

		DebugLog(string.Format("setup validate #%1 accepted resolved=%2", m_iSetupValidationDebugCount, resolvedPosition));
		return BuildSetupResultPayload("validate", true, resolvedPosition, "HQ position is valid");
	}

	string RequestSetupConfirmHQPosition(int playerId, float worldX, float worldZ)
	{
		if (!Replication.IsServer() || !m_State || !m_HQ)
			return BuildSetupResultPayload("confirm", false, "0 0 0", "server coordinator not ready");

		EnsureSetupRegisteredPlayer(playerId);
		if (m_State.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_SETUP)
		{
			DebugLog(string.Format("setup confirm rejected player=%1 pos=%2,%3 reason=campaign already complete phase=%4", playerId, worldX, worldZ, m_State.m_ePhase));
			return BuildSetupResultPayload("confirm", false, "0 0 0", "campaign setup is already complete");
		}
		if (!CanPlayerUseCommanderActions(playerId))
		{
			DebugLog(string.Format("setup confirm rejected player=%1 pos=%2,%3 reason=not commander commander=%4", playerId, worldX, worldZ, m_State.m_sCommanderIdentityId));
			return BuildSetupResultPayload("confirm", false, "0 0 0", "commander authority required");
		}

		vector requestedPosition = "0 0 0";
		requestedPosition[0] = worldX;
		requestedPosition[2] = worldZ;

		m_iSetupConfirmDebugCount++;
		DebugLog(string.Format("setup confirm #%1 player=%2 requested=%3", m_iSetupConfirmDebugCount, playerId, requestedPosition));
		vector resolvedPosition;
		string failure;
		if (!m_HQ.SelectInitialHQPosition(m_State, requestedPosition, resolvedPosition, failure))
		{
			DebugLog(string.Format("setup confirm #%1 rejected resolved=%2 failure=%3", m_iSetupConfirmDebugCount, resolvedPosition, failure));
			return BuildSetupResultPayload("confirm", false, resolvedPosition, failure);
		}

		m_HQ.EnsureRuntimeObjects(m_State);
		RefreshCampaignMarkers();
		if (m_Persistence)
			m_Persistence.RequestCheckpoint("h-istasi setup HQ placed", m_State);
		ArmPlayerSpawnSweep(6);
		MarkMajorCampaignChange(true);
		HST_CommandMenuRequestComponent.BroadcastSetupRefresh();
		DebugLog(string.Format("setup confirm #%1 accepted resolved=%2; campaign phase active and spawn sweep armed", m_iSetupConfirmDebugCount, resolvedPosition));
		return BuildSetupResultPayload("confirm", true, resolvedPosition, "HQ placed");
	}

	string BuildLoadoutEditorPayload(int playerId)
	{
		if (!Replication.IsServer() || !m_LoadoutEditor)
			return "HST_LOADOUT_EDITOR|offline||false|0|0|0|0\nPREVIEW|false|0 0 0|0|server coordinator not ready\nEND";

		if (m_Arsenal)
			m_Arsenal.PurgeBlockedArsenalItems(m_State);

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

		if (commandId == "loadout_rename" || commandId == "rename_loadout")
			return RequestMemberRenameSavedLoadout(playerId, argument);

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
		{
			ArmPlayerSpawnSweep(2);
			if (m_PlayerMapMarkers)
				m_PlayerMapMarkers.RequestRefresh(string.Format("spawn requested %1", playerId));
		}

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
			if (m_PlayerMapMarkers)
				m_PlayerMapMarkers.RequestRefresh(string.Format("player spawned %1", playerId));
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
			m_HQ.ReduceHQKnowledge(m_State, 50, "HQ moved");
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
			m_HQ.ReduceHQKnowledge(m_State, 50, "HQ moved");
			MarkMajorCampaignChange();
		}
		return changed;
	}

	void OnPetrosKilled()
	{
		if (!Replication.IsServer())
			return;

		m_HQ.OnPetrosKilled(m_State, m_Economy, 250, 5);
		if (m_State.m_bDefendPetrosActive)
			FailDefendPetrosMission("Petros killed");
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
		if (!Replication.IsServer() || !m_Recruitment)
			return false;

		HST_TrainingResult result = m_Recruitment.TrainTroopsDetailed(m_State, m_Economy, moneyCost);
		if (result && result.m_bSuccess)
			MarkMajorCampaignChange();
		return result && result.m_bSuccess;
	}

	bool RecruitResistanceGarrison(string zoneId, int infantryCount, int vehicleCount = 0, int moneyCost = 100, int hrCost = 1)
	{
		if (!Replication.IsServer() || !m_Recruitment)
			return false;

		HST_RecruitmentResult result = m_Recruitment.RecruitGarrisonDetailed(m_State, m_Preset, m_Balance, m_Economy, m_Garrisons, m_Arsenal, zoneId, infantryCount, vehicleCount, moneyCost, hrCost);
		if (result && result.m_bSuccess)
			MarkMajorCampaignChange();
		return result && result.m_bSuccess;
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

	string RequestCommanderSelectInitialHideoutReport(int playerId, string hideoutId)
	{
		if (!Replication.IsServer())
			return "h-istasi setup | failed: server required";
		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi setup | failed: commander required";
		if (hideoutId.IsEmpty())
			return "h-istasi setup | failed: hideout id missing";
		if (!HST_DefaultCatalog.IsKnownHideout(hideoutId))
			return "h-istasi setup | failed: unknown hideout " + hideoutId;
		if (!m_State || m_State.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_SETUP)
			return string.Format("h-istasi setup | failed: campaign phase %1 is not setup", m_State.m_ePhase);

		bool changed = SelectInitialHideout(hideoutId);
		if (!changed)
			return string.Format("h-istasi setup | failed: could not select %1", hideoutId);

		return string.Format("h-istasi setup | HQ selected %1 | phase %2 | Petros %3", m_State.m_sHQHideoutId, m_State.m_ePhase, m_State.m_bPetrosAlive);
	}

	string RequestCommanderMoveHQReport(int playerId, string hideoutId)
	{
		if (!Replication.IsServer())
			return "h-istasi HQ | failed: server required";
		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi HQ | failed: commander required";
		if (hideoutId.IsEmpty())
			return "h-istasi HQ | failed: hideout id missing";
		if (!HST_DefaultCatalog.IsKnownHideout(hideoutId))
			return "h-istasi HQ | failed: unknown hideout " + hideoutId;

		bool changed = MoveHQ(hideoutId);
		if (!changed)
			return string.Format("h-istasi HQ | failed: could not move HQ to %1 | Petros alive %2 | phase %3", hideoutId, m_State.m_bPetrosAlive, m_State.m_ePhase);

		return string.Format("h-istasi HQ | moved to %1 | knowledge %2 | Petros %3", m_State.m_sHQHideoutId, m_State.m_iHQKnowledge, m_State.m_bPetrosAlive);
	}

	string RequestCommanderMoveHQToPlayerReport(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi HQ | failed: server required";
		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi HQ | failed: commander required";
		if (!ResolveControlledPlayerEntity(playerId))
			return "h-istasi HQ | failed: player entity not found";

		bool changed = MoveHQToPlayer(playerId);
		if (!changed)
			return string.Format("h-istasi HQ | failed: could not move HQ to player | Petros alive %1 | phase %2", m_State.m_bPetrosAlive, m_State.m_ePhase);

		return string.Format("h-istasi HQ | moved to player position | HQ %1 | knowledge %2", m_State.m_vHQPosition, m_State.m_iHQKnowledge);
	}

	string RequestCommanderRebuildHQAssetsReport(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi HQ assets | failed: server required";
		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi HQ assets | failed: commander required";
		if (!m_HQ || !m_BuildMode)
			return "h-istasi HQ assets | failed: HQ/build service not ready";
		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return "h-istasi HQ assets | failed: player outside HQ interaction radius";

		HST_BuildModePlacement placement = m_BuildMode.ResolveHQRebuildPlacement(m_State, playerId);
		if (!placement || !placement.m_bValid)
			return "h-istasi HQ assets | failed: build placement denied | " + m_State.m_sLastBuildModeFailure;

		bool changed = RequestCommanderRebuildHQAssets(playerId);
		if (!changed)
			return string.Format("h-istasi HQ assets | failed: rebuild returned false | runtime %1 | arsenal %2", m_State.m_bHQRuntimeObjectsSpawned, m_State.m_sHQArsenalRuntimeStatus);

		return string.Format("h-istasi HQ assets | rebuilt | runtime %1 | arsenal %2 | failure %3", m_State.m_bHQRuntimeObjectsSpawned, m_State.m_sHQArsenalRuntimeStatus, m_State.m_sLastHQArsenalFailure);
	}

	string RequestCommanderApplyIncomeNowReport(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi income | failed: server required";
		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi income | failed: commander required";

		int income = ApplyIncomeNow();
		if (income <= 0)
			return string.Format("h-istasi income | failed: no income applied | FIA zones %1 | timer %2", CountResistanceZones(), m_State.m_iIncomeAccumulatorSeconds);

		return string.Format("h-istasi income | applied $%1 | money %2 | HR %3", income, m_State.m_iFactionMoney, m_State.m_iHR);
	}

	string RequestCommanderStartZoneMissionReport(int playerId, string zoneId)
	{
		if (!Replication.IsServer())
			return "h-istasi mission | failed: server required";
		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi mission | failed: commander required";
		if (zoneId.IsEmpty())
			return "h-istasi mission | failed: zone id missing";

		HST_ZoneState zone = m_State.FindZone(zoneId);
		if (!zone)
			return "h-istasi mission | failed: zone not found " + zoneId;

		string missionId = SelectMissionForZone(zone);
		if (!m_Missions || !m_Missions.CanStart(m_State, m_Preset, missionId, zoneId))
			return string.Format("h-istasi mission | failed: %1 cannot start at %2 | phase %3 | active at zone %4", missionId, ResolveZoneLabel(zone), m_State.m_ePhase, CountActiveMissionsAtZone(zoneId));

		bool changed = StartMission(missionId, zoneId);
		if (!changed)
			return string.Format("h-istasi mission | failed: start returned false for %1 at %2", missionId, ResolveZoneLabel(zone));

		return string.Format("h-istasi mission | started %1 at %2 | active %3", missionId, ResolveZoneLabel(zone), CountFoundationActiveMissions());
	}

	string RequestCommanderStartRandomMissionReport(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi mission | failed: server required";
		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi mission | failed: commander required";

		HST_ZoneState zone = SelectRandomMissionZone();
		if (!zone)
			return "h-istasi mission | failed: no compatible random target zone";

		return RequestCommanderStartZoneMissionReport(playerId, zone.m_sZoneId);
	}

	string RequestCommanderProgressMissionReport(int playerId, string instanceId = "")
	{
		if (!Replication.IsServer())
			return "h-istasi mission | failed: server required";
		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi mission | failed: commander required";
		if (!m_Objectives)
			return "h-istasi mission | failed: objective service not ready";

		string resolvedInstanceId = ResolveMissionInstanceId(instanceId);
		if (resolvedInstanceId.IsEmpty())
			return "h-istasi mission | failed: no active mission to progress";

		HST_ActiveMissionState mission = m_State.FindActiveMission(resolvedInstanceId);
		if (!mission)
			return "h-istasi mission | failed: mission not found " + resolvedInstanceId;

		bool changed = RequestCommanderProgressMission(playerId, resolvedInstanceId);
		if (!changed)
			return string.Format("h-istasi mission | failed: progress blocked for %1 | status %2 | phase %3 | failure %4", resolvedInstanceId, mission.m_eStatus, mission.m_sRuntimePhase, mission.m_sRuntimeFailureReason);

		return string.Format("h-istasi mission | progressed %1 | phase %2 | remaining %3s", resolvedInstanceId, mission.m_sRuntimePhase, mission.m_iRemainingSeconds);
	}

	string RequestCommanderCompleteMissionReport(int playerId, string instanceId)
	{
		if (!Replication.IsServer())
			return "h-istasi mission | failed: server required";
		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi mission | failed: commander required";
		if (instanceId.IsEmpty())
			return "h-istasi mission | failed: mission id missing";

		HST_ActiveMissionState mission = m_State.FindActiveMission(instanceId);
		if (!mission)
			return "h-istasi mission | failed: mission not found " + instanceId;

		bool changed = CompleteMission(instanceId);
		if (!changed)
			return string.Format("h-istasi mission | failed: complete blocked for %1 | status %2 | phase %3 | failure %4", instanceId, mission.m_eStatus, mission.m_sRuntimePhase, mission.m_sRuntimeFailureReason);

		return string.Format("h-istasi mission | completed %1 | status %2 | money %3 | HR %4", instanceId, mission.m_eStatus, m_State.m_iFactionMoney, m_State.m_iHR);
	}

	string RequestCommanderCancelSupportReport(int playerId, string requestId = "")
	{
		if (!Replication.IsServer())
			return "h-istasi support | failed: server required";
		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi support | failed: commander required";
		if (!m_SupportRequests)
			return "h-istasi support | failed: support service not ready";

		string target = requestId;
		if (target.IsEmpty())
			target = ResolveFirstCancelablePlayerSupportRequestId();
		if (target.IsEmpty())
			return "h-istasi support | failed: no queued or active player support request";

		bool changed = RequestCommanderCancelSupport(playerId, target);
		if (!changed)
			return "h-istasi support | failed: could not cancel request " + target;

		return "h-istasi support | cancelled " + target + "\n" + m_SupportRequests.BuildSupportReport(m_State);
	}

	string RequestCommanderAidNearestTownReport(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi civilian aid | failed: server required";
		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi civilian aid | failed: commander required";
		if (!m_Civilians)
			return "h-istasi civilian aid | failed: civilian service not ready";

		string targetZoneId = SelectHQSupportZoneId();
		if (targetZoneId.IsEmpty())
			return "h-istasi civilian aid | failed: no nearby town target";
		if (m_State.m_iFactionMoney < 100)
			return string.Format("h-istasi civilian aid | failed: need $100, have $%1", m_State.m_iFactionMoney);

		bool changed = RequestCommanderAidNearestTown(playerId);
		if (!changed)
			return "h-istasi civilian aid | failed: incident registration returned false for " + targetZoneId;

		return string.Format("h-istasi civilian aid | delivered to %1 | money %2", targetZoneId, m_State.m_iFactionMoney);
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
			return "h-istasi recruitment | failed: server authority unavailable";

		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi recruitment | failed: commander permission required";

		if (!m_Recruitment)
			return "h-istasi recruitment | failed: recruitment service not ready";

		HST_RecruitmentResult result = m_Recruitment.RecruitGarrisonDetailed(m_State, m_Preset, m_Balance, m_Economy, m_Garrisons, m_Arsenal, zoneId, infantryCount, vehicleCount, moneyCost, hrCost);
		if (result && result.m_bSuccess)
			MarkMajorCampaignChange();

		string summary = "h-istasi recruitment | failed: no result";
		if (result)
			summary = result.BuildSummary();

		return summary + "\n" + m_Recruitment.BuildRecruitmentReport(m_State, m_Preset, m_Arsenal);
	}

	bool RequestCommanderTrainTroops(int playerId, int moneyCost = 250)
	{
		if (!Replication.IsServer() || !CanPlayerUseCommanderActions(playerId))
			return false;

		return TrainTroops(moneyCost);
	}

	string RequestCommanderTrainTroopsReport(int playerId, int moneyCost = 250)
	{
		if (!Replication.IsServer())
			return "h-istasi training | failed: server authority unavailable";

		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi training | failed: commander permission required";

		if (!m_Recruitment)
			return "h-istasi training | failed: recruitment service not ready";

		HST_TrainingResult result = m_Recruitment.TrainTroopsDetailed(m_State, m_Economy, moneyCost);
		if (result && result.m_bSuccess)
			MarkMajorCampaignChange();

		string summary = "h-istasi training | failed: no result";
		if (result)
			summary = result.BuildSummary();

		return summary + "\n" + m_Recruitment.BuildRecruitmentReport(m_State, m_Preset, m_Arsenal);
	}

	string RequestCommanderRemoveGarrisonReport(int playerId, string zoneId, int infantryCount = 1, int vehicleCount = 0)
	{
		if (!Replication.IsServer())
			return "h-istasi garrison | failed: server authority unavailable";

		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi garrison | failed: commander permission required";

		if (!m_State || !m_Preset || !m_Garrisons)
			return "h-istasi garrison | failed: service not ready";

		HST_ZoneState zone = m_State.FindZone(zoneId);
		if (!zone)
			return "h-istasi garrison | failed: zone not found";

		if (zone.m_bActive || zone.m_iActiveInfantryCount > 0 || zone.m_iActiveVehicleCount > 0)
			return string.Format("h-istasi garrison | failed: %1 is active; fold/deactivate the zone before removing abstract garrison units", ResolveZoneLabel(zone));

		string factionKey = m_Preset.m_sResistanceFactionKey;
		if (factionKey.IsEmpty())
			factionKey = "FIA";

		HST_GarrisonState garrison = m_State.FindGarrison(zoneId, factionKey);
		if (!garrison)
			return string.Format("h-istasi garrison | failed: no FIA garrison at %1", ResolveZoneLabel(zone));

		int beforeInfantry = garrison.m_iInfantryCount;
		int beforeVehicles = garrison.m_iVehicleCount;

		bool changed = m_Garrisons.RemoveAbstractForces(m_State, zoneId, factionKey, infantryCount, vehicleCount);
		if (!changed)
			return string.Format("h-istasi garrison | failed: no removable forces at %1", ResolveZoneLabel(zone));

		MarkMajorCampaignChange();

		return string.Format(
			"h-istasi garrison | removed from %1 | infantry %2 -> %3 | vehicles %4 -> %5",
			ResolveZoneLabel(zone),
			beforeInfantry,
			garrison.m_iInfantryCount,
			beforeVehicles,
			garrison.m_iVehicleCount
		);
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

	string RequestCommanderCallSupplyDropReport(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi support | failed: server required";

		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi support | failed: commander permission required";

		if (!m_SupportRequests)
			return "h-istasi support | failed: service not ready";
		if (!m_State || !m_Preset)
			return "h-istasi support | failed: campaign state or preset not ready";

		string targetZoneId = SelectHQSupportZoneId();
		HST_SupportRequestResult result = m_SupportRequests.RequestSupportDetailed(
			m_State,
			m_Preset,
			m_Economy,
			m_EnemyDirector,
			m_Preset.m_sResistanceFactionKey,
			HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP,
			targetZoneId,
			true,
			HST_SupportRequestService.PLAYER_SUPPORT_COOLDOWN_SECONDS
		);

		if (result && result.m_bSuccess)
		{
			MarkMajorCampaignChange(true);
			m_SupportRequests.ConsumeMarkerRefreshNeeded();
		}

		string report = result.BuildSummary();
		report = report + "\n" + m_SupportRequests.BuildSupportReport(m_State);
		report = report + "\n" + m_SupportRequests.BuildSupportCooldownReport(m_State);
		return report;
	}

	bool RequestCommanderCallSupplyDrop(int playerId)
	{
		string result = RequestCommanderCallSupplyDropReport(playerId);
		return !result.Contains("failed");
	}

	string RequestCommanderCallPlayerSupportReport(int playerId, HST_ESupportRequestType supportType)
	{
		if (!Replication.IsServer())
			return "h-istasi support | failed: server required";

		if (!CanPlayerUseCommanderActions(playerId))
			return "h-istasi support | failed: commander permission required";

		if (!m_SupportRequests)
			return "h-istasi support | failed: service not ready";
		if (!m_State || !m_Preset)
			return "h-istasi support | failed: campaign state or preset not ready";

		if (IsAirSupportType(supportType))
		{
			if (!m_Balance.m_bAirSupportEnabled || !HasResistanceAirSupportCapability())
				return "h-istasi support | failed: air support capability unavailable";
		}

		string targetZoneId = SelectPlayerSupportZoneId(playerId);
		int cooldownSeconds = HST_SupportRequestService.PLAYER_SUPPORT_COOLDOWN_SECONDS;
		if (IsAirSupportType(supportType))
			cooldownSeconds = m_Balance.m_iAirSupportCooldownSeconds;

		HST_SupportRequestResult result = m_SupportRequests.RequestSupportDetailed(
			m_State,
			m_Preset,
			m_Economy,
			m_EnemyDirector,
			m_Preset.m_sResistanceFactionKey,
			supportType,
			targetZoneId,
			true,
			cooldownSeconds
		);

		if (result && result.m_bSuccess)
		{
			MarkMajorCampaignChange(true);
			m_SupportRequests.ConsumeMarkerRefreshNeeded();
		}

		string report = result.BuildSummary();
		report = report + "\n" + m_SupportRequests.BuildSupportReport(m_State);
		report = report + "\n" + m_SupportRequests.BuildSupportCooldownReport(m_State);
		return report;
	}

	bool RequestCommanderCallPlayerSupport(int playerId, HST_ESupportRequestType supportType)
	{
		string result = RequestCommanderCallPlayerSupportReport(playerId, supportType);
		return !result.Contains("failed");
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

	string RequestMemberInspectCapture(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi capture | server required";
		if (!CanPlayerUseMemberActions(playerId) && !CanPlayerUseAdminActions(playerId))
			return "h-istasi capture | membership required";
		if (!m_ZoneCapture)
			return "h-istasi capture | service not ready";

		return m_ZoneCapture.BuildCaptureReport(m_State, m_Preset, m_Balance);
	}

	string RequestMemberInspectCampaignEnd(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi campaign end | server required";
		if (!CanPlayerUseMemberActions(playerId) && !CanPlayerUseAdminActions(playerId))
			return "h-istasi campaign end | membership required";
		if (!m_Strategic)
			return "h-istasi campaign end | strategic service not ready";

		return m_Strategic.BuildCampaignEndReport(m_State, m_Economy, m_Balance, m_Preset);
	}

	string RequestMemberInspectBalancePacing(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi balance | server required";
		if (!CanPlayerUseMemberActions(playerId) && !CanPlayerUseAdminActions(playerId))
			return "h-istasi balance | membership required";

		string report = m_Economy.BuildPacingReport(m_State, m_Balance, m_Preset);
		if (m_Towns)
			report = report + "\n" + m_Towns.BuildIncomeReport(m_State, m_Preset, m_Balance);
		if (m_EnemyDirector)
			report = report + "\n" + m_EnemyDirector.BuildEnemyResourceReport(m_State, m_Preset, m_Balance);
		if (m_Missions)
			report = report + "\n" + m_Missions.BuildMissionRewardBalanceReport(m_State);
		if (m_Strategic)
			report = report + "\n" + m_Strategic.BuildCampaignEndReport(m_State, m_Economy, m_Balance, m_Preset);
		return report;
	}
	string RequestMemberInspectEconomy(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_CommandUI)
			return "";

		return m_CommandUI.BuildEconomyReport(m_State);
	}

	string RequestMemberInspectRecruitment(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi recruitment | server required";
		if (!CanPlayerUseMemberActions(playerId))
			return "h-istasi recruitment | membership required";
		if (!m_Recruitment)
			return "h-istasi recruitment | service not ready";

		return m_Recruitment.BuildRecruitmentReport(m_State, m_Preset, m_Arsenal);
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

	string RequestMemberInspectMissionSummary(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi missions | server required";
		if (!CanPlayerUseMemberActions(playerId))
			return "h-istasi missions | membership required";
		if (!m_CommandUI)
			return "h-istasi missions | UI service not ready";

		return m_CommandUI.BuildActiveMissionSummaryReport(m_State);
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

		string report = m_SupportRequests.BuildSupportReport(m_State);
		report = report + "\n" + m_SupportRequests.BuildSupportCooldownReport(m_State);
		if (m_EnemyCommander)
		{
			report = report + "\n" + m_EnemyCommander.BuildEnemyOrderReport(m_State);
			report = report + "\n" + m_EnemyCommander.BuildPhysicalResponseReport(m_State);
		}
		return report;
	}

	string RequestMemberInspectHQThreat(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi HQ threat | server required";
		if (!CanPlayerUseMemberActions(playerId))
			return "h-istasi HQ threat | membership required";

		string report = "h-istasi phase 22 | HQ threat";
		if (m_HQ)
			report = report + "\n" + m_HQ.BuildHQThreatReport(m_State);
		if (m_EnemyCommander)
			report = report + "\n" + m_EnemyCommander.BuildEnemyOrderReport(m_State);
		if (m_SupportRequests)
			report = report + "\n" + m_SupportRequests.BuildSupportReport(m_State);
		return report;
	}

	string RequestMemberInspectCivilians(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_Civilians)
			return "";

		return m_Civilians.BuildCivilianReport(m_State);
	}

	string RequestMemberInspectTownSupport(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi town support | server required";
		if (!CanPlayerUseMemberActions(playerId))
			return "h-istasi town support | membership required";
		if (!m_Civilians)
			return "h-istasi town support | service not ready";

		return m_Civilians.BuildTownSupportReport(m_State);
	}

	string RequestMemberInspectUndercover(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId) || !m_Civilians)
			return "";

		return m_Civilians.BuildUndercoverReport(m_State, ResolveTrustedIdentityId(playerId));
	}

	string RequestMemberInspectUndercoverEligibility(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi undercover eligibility | server required";
		if (!CanPlayerUseMemberActions(playerId))
			return "h-istasi undercover eligibility | membership required";
		if (!m_Civilians)
			return "h-istasi undercover eligibility | service not ready";

		HST_UndercoverEligibilityResult result = m_Civilians.BuildUndercoverEligibility(m_State, ResolveTrustedIdentityId(playerId), ResolveControlledPlayerEntity(playerId));
		MarkMajorCampaignChange(false);
		if (!result)
			return "h-istasi undercover eligibility | failed: no result";

		return result.BuildReport();
	}

	string RequestMemberRunUndercoverCheck(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi undercover enforcement | server required";
		if (!CanPlayerUseMemberActions(playerId))
			return "h-istasi undercover enforcement | membership required";
		if (!m_Civilians)
			return "h-istasi undercover enforcement | service not ready";

		string identityId = ResolveTrustedIdentityId(playerId);
		HST_UndercoverEnforcementResult result = m_Civilians.EnforceUndercoverForPlayer(m_State, m_Preset, identityId, ResolveControlledPlayerEntity(playerId));
		if (result && result.m_bChanged)
			MarkMajorCampaignChange();

		if (!result)
			return "h-istasi undercover enforcement | failed: no result";

		return result.BuildReport() + "\n" + m_Civilians.BuildUndercoverReport(m_State, identityId);
	}
	string RequestMemberRequestUndercover(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi undercover | server required";
		if (!CanPlayerUseMemberActions(playerId))
			return "h-istasi undercover | membership required";
		if (!m_Civilians)
			return "h-istasi undercover | service not ready";

		string result = m_Civilians.RequestUndercover(m_State, ResolveTrustedIdentityId(playerId), ResolveControlledPlayerEntity(playerId));
		if (!result.Contains("failed"))
			MarkMajorCampaignChange();

		return result;
	}

	string RequestMemberClearUndercover(int playerId)
	{
		if (!Replication.IsServer())
			return "h-istasi undercover | server required";
		if (!CanPlayerUseMemberActions(playerId))
			return "h-istasi undercover | membership required";
		if (!m_Civilians)
			return "h-istasi undercover | service not ready";

		string result = m_Civilians.ClearUndercoverCompromise(m_State, ResolveTrustedIdentityId(playerId));
		if (!result.Contains("failed"))
			MarkMajorCampaignChange();

		return result;
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

	string RequestMemberRenameSavedLoadout(int playerId, string argument)
	{
		if (!Replication.IsServer() || !CanPlayerUseMemberActions(playerId))
			return "h-istasi loadout editor | membership required";

		if (!m_LoadoutEditor)
			return "h-istasi loadout editor | service not ready";

		if (!IsPlayerWithinHQInteractionRadius(playerId))
			return BuildHQInteractionDenied("h-istasi loadout editor");

		string result = m_LoadoutEditor.RenameSavedLoadout(m_State, ResolveTrustedIdentityId(playerId), argument);
		if (!result.Contains("failed"))
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

	string RequestServerMissionAssetExplosiveDamage(string assetId, vector position, float damage, string sourceLabel)
	{
		if (!Replication.IsServer())
			return "h-istasi mission | server required";

		if (!m_MissionRuntime)
			return "h-istasi mission | service not ready";

		string result;
		string eventType;
		string missionInstanceId;
		bool changed = m_MissionRuntime.ApplyMissionAssetExplosiveDamage(m_State, assetId, position, damage, sourceLabel, result, eventType, missionInstanceId);
		if (!changed)
			return result;

		HST_ActiveMissionState mission = m_State.FindActiveMission(missionInstanceId);
		HST_MissionDefinition definition;
		if (mission)
			definition = m_Missions.FindDefinition(mission.m_sMissionId);

		if (!eventType.IsEmpty() && eventType != "demolition_progress")
			BroadcastMissionEvent(eventType, mission, definition);

		ApplyConvoyOutcomesNow();

		string completedRuntimeMissionId = m_MissionRuntime.FindCompletedActiveMissionId(m_State, m_Objectives);
		if (!completedRuntimeMissionId.IsEmpty())
			CompleteMission(completedRuntimeMissionId);

		string failedRuntimeMissionId = m_MissionRuntime.FindFailedActiveMissionId(m_State);
		if (!failedRuntimeMissionId.IsEmpty())
			FailMission(failedRuntimeMissionId);

		BroadcastPendingMissionOutcomeEvents();
		MarkMajorCampaignChange(eventType != "demolition_progress");
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

	string RequestAdminSetZoneActiveReport(int playerId, string zoneId, bool active)
	{
		if (!Replication.IsServer())
			return "h-istasi admin zone | failed: server required";
		if (!CanPlayerUseAdminActions(playerId))
			return "h-istasi admin zone | failed: admin required";
		if (zoneId.IsEmpty())
			return "h-istasi admin zone | failed: zone id missing";

		HST_ZoneState zone = m_State.FindZone(zoneId);
		if (!zone)
			return "h-istasi admin zone | failed: zone not found " + zoneId;

		bool changed = RequestAdminSetZoneActive(playerId, zoneId, active);
		if (!changed)
			return string.Format("h-istasi admin zone | failed: active already %1 at %2", zone.m_bActive, ResolveZoneLabel(zone));

		return string.Format("h-istasi admin zone | %1 active %2 | owner %3", ResolveZoneLabel(zone), zone.m_bActive, zone.m_sOwnerFactionKey);
	}

	string RequestAdminCaptureZoneForResistanceReport(int playerId, string zoneId, int supportReward = 10)
	{
		if (!Replication.IsServer())
			return "h-istasi admin capture | failed: server required";
		if (!CanPlayerUseAdminActions(playerId))
			return "h-istasi admin capture | failed: admin required";
		if (zoneId.IsEmpty())
			return "h-istasi admin capture | failed: zone id missing";

		HST_ZoneState zone = m_State.FindZone(zoneId);
		if (!zone)
			return "h-istasi admin capture | failed: zone not found " + zoneId;

		bool changed = RequestAdminCaptureZoneForResistance(playerId, zoneId, supportReward);
		if (!changed)
			return string.Format("h-istasi admin capture | failed: could not capture %1 | owner %2 | support %3", ResolveZoneLabel(zone), zone.m_sOwnerFactionKey, zone.m_iSupport);

		return string.Format("h-istasi admin capture | captured %1 | owner %2 | support %3", ResolveZoneLabel(zone), zone.m_sOwnerFactionKey, zone.m_iSupport);
	}

	string RequestAdminAddCaptureProgressReport(int playerId, string zoneId, int progress = 50)
	{
		if (!Replication.IsServer())
			return "h-istasi admin capture | failed: server required";
		if (!CanPlayerUseAdminActions(playerId))
			return "h-istasi admin capture | failed: admin required";
		if (zoneId.IsEmpty())
			return "h-istasi admin capture | failed: zone id missing";

		HST_ZoneState zone = m_State.FindZone(zoneId);
		if (!zone)
			return "h-istasi admin capture | failed: zone not found " + zoneId;

		int before = zone.m_iResistanceCaptureProgress;
		bool changed = RequestAdminAddCaptureProgress(playerId, zoneId, progress);
		if (!changed)
			return string.Format("h-istasi admin capture | failed: progress blocked at %1 | owner %2 | progress %3", ResolveZoneLabel(zone), zone.m_sOwnerFactionKey, zone.m_iResistanceCaptureProgress);

		return string.Format("h-istasi admin capture | progress %1 | %2 -> %3", ResolveZoneLabel(zone), before, zone.m_iResistanceCaptureProgress);
	}

	string RequestAdminStartDebugMissionReport(int playerId, string zoneId)
	{
		if (!Replication.IsServer())
			return "h-istasi admin mission | failed: server required";
		if (!CanPlayerUseAdminActions(playerId))
			return "h-istasi admin mission | failed: admin required";

		string target = zoneId;
		if (target.IsEmpty())
			target = SelectFirstAdminZoneId();
		if (target.IsEmpty())
			return "h-istasi admin mission | failed: no debug target zone";

		HST_ZoneState zone = m_State.FindZone(target);
		if (!zone)
			return "h-istasi admin mission | failed: zone not found " + target;

		bool changed = RequestAdminStartDebugMission(playerId, target);
		if (!changed)
			return string.Format("h-istasi admin mission | failed: could not start debug mission at %1 | phase %2", ResolveZoneLabel(zone), m_State.m_ePhase);

		return string.Format("h-istasi admin mission | started debug mission at %1 | active %2", ResolveZoneLabel(zone), CountFoundationActiveMissions());
	}

	string RequestAdminAwardResourcesReport(int playerId, int money, int hr)
	{
		if (!Replication.IsServer())
			return "h-istasi admin resources | failed: server required";
		if (!CanPlayerUseAdminActions(playerId))
			return "h-istasi admin resources | failed: admin required";

		bool changed = RequestAdminAwardResources(playerId, money, hr);
		if (!changed)
			return "h-istasi admin resources | failed: award returned false";

		return string.Format("h-istasi admin resources | awarded $%1 HR %2 | money %3 | HR %4", money, hr, m_State.m_iFactionMoney, m_State.m_iHR);
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
			DebugLog("admin repaired campaign phase for debug mission start: HQ was already deployed");
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

	string RequestAdminPhase16Seed(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 16 smoke | failed: admin required";

		if (!m_State || !m_Preset || !m_Garrisons || !m_Recruitment)
			return "h-istasi phase 16 smoke | failed: service not ready";

		HST_ZoneState zone = SelectPhase16FriendlyRecruitZone();
		if (!zone)
			return "h-istasi phase 16 smoke | failed: no friendly zone available";

		string resistanceFactionKey = m_Preset.m_sResistanceFactionKey;
		if (resistanceFactionKey.IsEmpty())
			resistanceFactionKey = "FIA";

		zone.m_sOwnerFactionKey = resistanceFactionKey;
		zone.m_bActive = false;
		zone.m_iActiveInfantryCount = 0;
		zone.m_iActiveVehicleCount = 0;

		HST_RecruitmentResult result = m_Recruitment.RecruitGarrisonDetailed(
			m_State,
			m_Preset,
			m_Balance,
			m_Economy,
			m_Garrisons,
			m_Arsenal,
			zone.m_sZoneId,
			2,
			0,
			0,
			0
		);

		if (result && result.m_bSuccess)
			MarkMajorCampaignChange();

		string summary = "h-istasi recruitment | failed: no result";
		if (result)
			summary = result.BuildSummary();

		return "h-istasi phase 16 smoke | seed\n" + summary + "\n" + m_Recruitment.BuildRecruitmentReport(m_State, m_Preset, m_Arsenal);
	}

	string RequestAdminPhase16Train(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 16 smoke | failed: admin required";

		if (!m_Recruitment)
			return "h-istasi phase 16 smoke | failed: recruitment service not ready";

		HST_TrainingResult result = m_Recruitment.TrainTroopsDetailed(m_State, m_Economy, 0);
		if (result && result.m_bSuccess)
			MarkMajorCampaignChange();

		string summary = "h-istasi training | failed: no result";
		if (result)
			summary = result.BuildSummary();

		return "h-istasi phase 16 smoke | train\n" + summary + "\n" + m_Recruitment.BuildRecruitmentReport(m_State, m_Preset, m_Arsenal);
	}

	string RequestAdminPhase16Report(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 16 smoke | failed: admin required";

		if (!m_Recruitment)
			return "h-istasi phase 16 smoke | failed: recruitment service not ready";

		string report = "h-istasi phase 16 smoke report";
		report = report + "\n" + m_Recruitment.BuildRecruitmentReport(m_State, m_Preset, m_Arsenal);

		if (m_CommandUI)
			report = report + "\n" + m_CommandUI.BuildEconomyReport(m_State);

		return report;
	}

	string RequestAdminPhase17SeedCapture(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 17 smoke | failed: admin required";

		if (!m_State || !m_Preset || !m_ZoneCapture)
			return "h-istasi phase 17 smoke | failed: service not ready";

		HST_ZoneState zone = SelectPhase17CaptureTarget();
		if (!zone)
			return "h-istasi phase 17 smoke | failed: no capturable target";

		zone.m_sOwnerFactionKey = m_Preset.m_sOccupierFactionKey;
		zone.m_iResistanceCaptureProgress = 0;
		zone.m_bActive = false;
		zone.m_iActiveInfantryCount = 0;
		zone.m_iActiveVehicleCount = 0;

		if (m_Garrisons)
		{
			HST_GarrisonState enemyGarrison = m_State.FindGarrison(zone.m_sZoneId, m_Preset.m_sOccupierFactionKey);
			if (!enemyGarrison)
				enemyGarrison = m_Garrisons.FindOrCreate(m_State, zone.m_sZoneId, m_Preset.m_sOccupierFactionKey);

			if (enemyGarrison)
			{
				enemyGarrison.m_iInfantryCount = 0;
				enemyGarrison.m_iVehicleCount = 0;
			}
		}

		if (m_PhysicalWar)
			m_PhysicalWar.CleanupCapturedZoneHostileRuntime(m_State, zone.m_sZoneId, m_Preset.m_sResistanceFactionKey);

		MarkMajorCampaignChange(true);
		return string.Format(
			"h-istasi phase 17 smoke | seeded empty capturable zone %1 | owner %2 | progress %3",
			ResolveZoneLabel(zone),
			zone.m_sOwnerFactionKey,
			zone.m_iResistanceCaptureProgress
		);
	}

	string RequestAdminPhase17ForceProgress(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 17 smoke | failed: admin required";

		if (!m_State || !m_Preset || !m_ZoneCapture)
			return "h-istasi phase 17 smoke | failed: service not ready";

		HST_ZoneState zone = SelectPhase17CaptureTarget();
		if (!zone)
			return "h-istasi phase 17 smoke | failed: no capturable target";

		int progress = HST_ZoneCaptureService.CAPTURE_PROGRESS_REQUIRED;
		if (m_Balance && m_Balance.m_iCaptureProgressRequired > 0)
			progress = m_Balance.m_iCaptureProgressRequired;

		bool changed = m_ZoneCapture.AddResistanceCaptureProgress(
			m_State,
			m_Preset,
			m_Strategic,
			m_Economy,
			m_Balance,
			zone.m_sZoneId,
			progress,
			10,
			m_Garrisons,
			m_EnemyCommander,
			m_EnemyDirector,
			m_SupportRequests
		);

		bool markerChanged;
		if (changed)
			markerChanged = BroadcastCaptureChangeNotifications();
		if (changed)
			MarkMajorCampaignChange(markerChanged);

		return string.Format(
			"h-istasi phase 17 smoke | force progress %1 | changed %2 | marker %3\n%4",
			zone.m_sZoneId,
			changed,
			markerChanged,
			m_ZoneCapture.BuildCaptureReport(m_State, m_Preset, m_Balance)
		);
	}

	string RequestAdminPhase17ForceCounterattack(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 17 smoke | failed: admin required";

		if (!m_State || !m_Preset || !m_EnemyCommander || !m_EnemyDirector)
			return "h-istasi phase 17 smoke | failed: enemy services not ready";

		HST_ZoneState zone = SelectFirstResistanceCapturableZone();
		if (!zone)
			return "h-istasi phase 17 smoke | failed: no captured FIA zone";

		string factionKey = m_Preset.m_sOccupierFactionKey;
		m_EnemyDirector.AddResources(m_State, factionKey, 100, 100);
		bool queued = m_EnemyCommander.TryQueueImmediateCounterattack(
			m_State,
			m_Preset,
			m_EnemyDirector,
			m_SupportRequests,
			factionKey,
			zone,
			100
		);

		MarkMajorCampaignChange(true);
		return string.Format(
			"h-istasi phase 17 smoke | force counterattack at %1 | queued %2\n%3",
			ResolveZoneLabel(zone),
			queued,
			m_EnemyCommander.BuildEnemyOrderReport(m_State)
		);
	}

	string RequestAdminPhase17Report(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 17 smoke | failed: admin required";

		string report = "h-istasi phase 17 smoke report";
		if (m_ZoneCapture)
			report = report + "\n" + m_ZoneCapture.BuildCaptureReport(m_State, m_Preset, m_Balance);
		if (m_MapMarkers)
			report = report + "\n" + m_MapMarkers.BuildMarkerReport(m_State);
		if (m_EnemyCommander)
			report = report + "\n" + m_EnemyCommander.BuildEnemyOrderReport(m_State);

		HST_ZoneState captured = SelectFirstResistanceCapturableZone();
		if (captured && m_State)
		{
			HST_MapMarkerState marker = m_State.FindMapMarker("hst_zone_" + captured.m_sZoneId);
			if (marker)
			{
				report = report + string.Format(
					"\nphase17 marker check | %1 | owner %2 | color %3 | style %4",
					marker.m_sMarkerId,
					marker.m_sOwnerFactionKey,
					marker.m_sColorHint,
					marker.m_sStyleHint
				);
			}
			else
			{
				report = report + string.Format("\nphase17 marker check | missing hst_zone_%1", captured.m_sZoneId);
			}
		}

		return report;
	}

	string RequestAdminPhase18SeedCounterattack(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 18 smoke | failed: admin required";

		if (!m_EnemyCommander || !m_EnemyDirector || !m_Preset || !m_State)
			return "h-istasi phase 18 smoke | failed: service not ready";

		HST_ZoneState targetZone = SelectFirstResistanceCapturableZone();
		if (!targetZone)
			return "h-istasi phase 18 smoke | failed: no FIA strategic zone";

		string factionKey = m_Preset.m_sOccupierFactionKey;
		m_EnemyDirector.AddResources(m_State, factionKey, 100, 100);
		bool queued = m_EnemyCommander.TryQueueImmediateCounterattack(
			m_State,
			m_Preset,
			m_EnemyDirector,
			m_SupportRequests,
			factionKey,
			targetZone,
			100
		);

		if (queued)
			MarkMajorCampaignChange(true);

		return string.Format(
			"h-istasi phase 18 smoke | counterattack seed target %1 | queued %2\n%3",
			ResolveZoneLabel(targetZone),
			queued,
			m_EnemyCommander.BuildEnemyOrderReport(m_State)
		);
	}

	string RequestAdminPhase18SeedRebuild(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 18 smoke | failed: admin required";

		if (!m_EnemyCommander || !m_EnemyDirector || !m_Preset || !m_State)
			return "h-istasi phase 18 smoke | failed: service not ready";

		HST_ZoneState targetZone = SelectEnemyOrderTargetZone(false);
		if (!targetZone)
			return "h-istasi phase 18 smoke | failed: no enemy target zone";

		HST_EnemyOrderState order = m_EnemyCommander.QueueDebugOrder(
			m_State,
			m_Preset,
			m_EnemyDirector,
			targetZone.m_sOwnerFactionKey,
			targetZone,
			HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON
		);

		if (order)
			MarkMajorCampaignChange(true);

		return string.Format("h-istasi phase 18 smoke | rebuild seed %1\n%2", order != null, m_EnemyCommander.BuildEnemyOrderReport(m_State));
	}

	string RequestAdminPhase18SeedRoadblock(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 18 smoke | failed: admin required";

		if (!m_EnemyCommander || !m_EnemyDirector || !m_Preset || !m_State)
			return "h-istasi phase 18 smoke | failed: service not ready";

		HST_ZoneState targetZone = SelectTownOrderTargetZone();
		if (!targetZone)
			return "h-istasi phase 18 smoke | failed: no town target";

		string factionKey = m_Preset.m_sOccupierFactionKey;
		m_EnemyDirector.AddResources(m_State, factionKey, 50, 50);

		HST_CivilianZoneState civilianZone = m_State.FindCivilianZone(targetZone.m_sZoneId);
		if (!civilianZone)
		{
			civilianZone = new HST_CivilianZoneState();
			civilianZone.m_sZoneId = targetZone.m_sZoneId;
			m_State.m_aCivilianZones.Insert(civilianZone);
		}

		HST_EnemyOrderState order = m_EnemyCommander.QueueDebugOrder(
			m_State,
			m_Preset,
			m_EnemyDirector,
			factionKey,
			targetZone,
			HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK
		);

		if (order)
			MarkMajorCampaignChange(true);

		return string.Format("h-istasi phase 18 smoke | roadblock seed %1\n%2", order != null, m_EnemyCommander.BuildEnemyOrderReport(m_State));
	}

	string RequestAdminPhase18ResolveNow(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 18 smoke | failed: admin required";

		if (!m_EnemyCommander)
			return "h-istasi phase 18 smoke | failed: enemy commander not ready";

		int resolved = m_EnemyCommander.DebugResolveDueOrdersNow(m_State, m_Preset, m_Garrisons);
		if (resolved > 0)
			MarkMajorCampaignChange(true);

		return string.Format("h-istasi phase 18 smoke | resolved %1 order(s)\n%2", resolved, m_EnemyCommander.BuildEnemyOrderReport(m_State));
	}

	string RequestAdminPhase18Report(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 18 smoke | failed: admin required";

		string report = "h-istasi phase 18 smoke report";
		if (m_EnemyCommander)
		{
			report = report + "\n" + m_EnemyCommander.BuildEnemyOrderReport(m_State);
			report = report + "\n" + m_EnemyCommander.BuildPhysicalResponseReport(m_State);
		}
		if (m_SupportRequests)
			report = report + "\n" + m_SupportRequests.BuildSupportReport(m_State);
		if (m_CommandUI)
			report = report + "\n" + m_CommandUI.BuildEconomyReport(m_State);

		return report;
	}

	string RequestAdminPhase19SeedFIASupply(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 19 smoke | failed: admin required";

		if (!m_SupportRequests)
			return "h-istasi phase 19 smoke | failed: support service not ready";
		if (!m_State || !m_Preset)
			return "h-istasi phase 19 smoke | failed: campaign state or preset not ready";

		string targetZoneId = SelectHQSupportZoneId();
		HST_SupportRequestResult result = m_SupportRequests.RequestSupportDetailed(
			m_State,
			m_Preset,
			m_Economy,
			m_EnemyDirector,
			m_Preset.m_sResistanceFactionKey,
			HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP,
			targetZoneId,
			true,
			60
		);

		if (result && result.m_bSuccess)
			MarkMajorCampaignChange(true);

		return "h-istasi phase 19 smoke | FIA supply\n" + result.BuildSummary() + "\n" + m_SupportRequests.BuildSupportReport(m_State);
	}

	string RequestAdminPhase19SeedFIAGround(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 19 smoke | failed: admin required";

		if (!m_SupportRequests)
			return "h-istasi phase 19 smoke | failed: support service not ready";
		if (!m_State || !m_Preset)
			return "h-istasi phase 19 smoke | failed: campaign state or preset not ready";

		string targetZoneId = SelectPlayerSupportZoneId(playerId);
		HST_SupportRequestResult result = m_SupportRequests.RequestSupportDetailed(
			m_State,
			m_Preset,
			m_Economy,
			m_EnemyDirector,
			m_Preset.m_sResistanceFactionKey,
			HST_ESupportRequestType.HST_SUPPORT_QRF,
			targetZoneId,
			true,
			60
		);

		if (result && result.m_bSuccess)
			MarkMajorCampaignChange(true);

		return "h-istasi phase 19 smoke | FIA ground\n" + result.BuildSummary() + "\n" + m_SupportRequests.BuildSupportReport(m_State);
	}

	string RequestAdminPhase19SeedEnemyGround(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 19 smoke | failed: admin required";

		if (!m_EnemyDirector || !m_SupportRequests)
			return "h-istasi phase 19 smoke | failed: services not ready";
		if (!m_State || !m_Preset)
			return "h-istasi phase 19 smoke | failed: campaign state or preset not ready";

		HST_ZoneState targetZone = SelectEnemyOrderTargetZone(false);
		if (!targetZone)
			return "h-istasi phase 19 smoke | failed: no target zone";

		string factionKey = m_Preset.m_sOccupierFactionKey;
		HST_FactionPoolState pool = m_State.FindFactionPool(factionKey);
		if (!pool)
			return "h-istasi phase 19 smoke | failed: enemy resource pool not ready";

		int beforeAttack = pool.m_iAttackResources;
		int beforeSupport = pool.m_iSupportResources;
		int attackCost = 15;
		int supportCost = 5;
		int attackTopUp = Math.Max(0, attackCost - beforeAttack);
		int supportTopUp = Math.Max(0, supportCost - beforeSupport);
		if (attackTopUp > 0 || supportTopUp > 0)
			m_EnemyDirector.AddResources(m_State, factionKey, attackTopUp, supportTopUp);

		HST_SupportRequestResult result = m_SupportRequests.RequestSupportDetailed(
			m_State,
			m_Preset,
			m_Economy,
			m_EnemyDirector,
			factionKey,
			HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY,
			targetZone.m_sZoneId,
			false,
			0
		);

		if (attackTopUp > 0 || supportTopUp > 0)
			m_EnemyDirector.AddResources(m_State, factionKey, -attackTopUp, -supportTopUp);

		pool = m_State.FindFactionPool(factionKey);
		int afterAttack;
		int afterSupport;
		if (pool)
		{
			afterAttack = pool.m_iAttackResources;
			afterSupport = pool.m_iSupportResources;
		}

		if (result && result.m_bSuccess)
			MarkMajorCampaignChange(true);

		string resources = string.Format("enemy resources %1 before %2/%3 after %4/%5", factionKey, beforeAttack, beforeSupport, afterAttack, afterSupport);
		return "h-istasi phase 19 smoke | enemy ground\n" + result.BuildSummary() + "\n" + resources + "\n" + m_SupportRequests.BuildSupportReport(m_State);
	}

	string RequestAdminPhase19ForceSupportETA(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 19 smoke | failed: admin required";

		if (!m_State || !m_SupportRequests)
			return "h-istasi phase 19 smoke | failed: support service not ready";

		int changed;
		foreach (HST_SupportRequestState request : m_State.m_aSupportRequests)
		{
			if (!request)
				continue;

			if (request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_QUEUED && request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
				continue;

			request.m_iRequestedAtSecond = m_State.m_iElapsedSeconds - Math.Max(0, request.m_iETASeconds);
			changed++;
		}

		if (changed > 0)
			MarkMajorCampaignChange(true);

		return string.Format("h-istasi phase 19 smoke | forced ETA on %1 request(s)\n%2", changed, m_SupportRequests.BuildSupportReport(m_State));
	}

	string RequestAdminPhase19Report(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 19 smoke | failed: admin required";

		string report = "h-istasi phase 19 smoke report";
		if (m_SupportRequests)
		{
			report = report + "\n" + m_SupportRequests.BuildSupportReport(m_State);
			report = report + "\n" + m_SupportRequests.BuildSupportCooldownReport(m_State);
		}
		if (m_EnemyCommander)
			report = report + "\n" + m_EnemyCommander.BuildEnemyOrderReport(m_State);
		if (m_CommandUI)
			report = report + "\n" + m_CommandUI.BuildEconomyReport(m_State);

		return report;
	}

	string RequestAdminPhase20SeedTownSupport(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 20 smoke | failed: admin required";
		if (!m_Civilians)
			return "h-istasi phase 20 smoke | failed: civilian service not ready";

		HST_CivilianZoneState town = SelectPhase20SmokeTown();
		if (!town)
			return "h-istasi phase 20 smoke | failed: no civilian town record";

		town.m_iFIASupport = 65;
		town.m_iOccupierSupport = 35;
		town.m_iReputation = 70;
		town.m_iWantedHeat = 1;
		town.m_iPolicePresence = Math.Max(1, town.m_iPolicePresence);
		town.m_iRoadblockPresence = Math.Max(1, town.m_iRoadblockPresence);
		town.m_sLastIncidentReason = "phase20 support smoke";
		town.m_iLastIncidentSecond = m_State.m_iElapsedSeconds;
		town.m_iLastSupportChangeSecond = m_State.m_iElapsedSeconds;

		HST_ZoneState zone = m_State.FindZone(town.m_sZoneId);
		if (zone)
			zone.m_iSupport = Math.Max(-100, Math.Min(100, town.m_iFIASupport - town.m_iOccupierSupport));

		MarkMajorCampaignChange();
		return "h-istasi phase 20 smoke | seeded town support\n" + m_Civilians.BuildTownSupportReport(m_State, 8);
	}

	string RequestAdminPhase20SeedWantedHeat(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 20 smoke | failed: admin required";
		if (!m_Civilians)
			return "h-istasi phase 20 smoke | failed: civilian service not ready";

		HST_CivilianZoneState town = SelectPhase20SmokeTown();
		if (!town)
			return "h-istasi phase 20 smoke | failed: no civilian town record";

		town.m_iWantedHeat = 5;
		town.m_sLastIncidentReason = "phase20 wanted heat smoke";
		town.m_iLastIncidentSecond = m_State.m_iElapsedSeconds;

		HST_PlayerUndercoverState undercover = m_Civilians.EnsurePlayer(m_State, ResolveTrustedIdentityId(playerId));
		if (undercover)
		{
			undercover.m_iWantedHeat = 4;
			undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_WANTED;
			undercover.m_sLastReason = "phase20 wanted heat smoke";
			undercover.m_bLastEligibilityResult = false;
			undercover.m_sWantedHeatReason = "BLOCK player wanted heat 4 smoke";
		}

		MarkMajorCampaignChange();
		return "h-istasi phase 20 smoke | seeded wanted heat\n" + m_Civilians.BuildUndercoverReport(m_State, ResolveTrustedIdentityId(playerId));
	}

	string RequestAdminPhase20SeedEligibleUndercover(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 20 smoke | failed: admin required";
		if (!m_Civilians)
			return "h-istasi phase 20 smoke | failed: civilian service not ready";

		HST_PlayerUndercoverState undercover = m_Civilians.EnsurePlayer(m_State, ResolveTrustedIdentityId(playerId));
		if (!undercover)
			return "h-istasi phase 20 smoke | failed: no undercover record";

		undercover.m_bUndercoverRequested = true;
		undercover.m_bLastEligibilityResult = true;
		undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
		undercover.m_iWantedHeat = 0;
		undercover.m_sClothingReason = "OK civilian clothing smoke";
		undercover.m_sWeaponReason = "OK no visible military weapon smoke";
		undercover.m_sVehicleReason = "OK civilian vehicle/on foot smoke";
		undercover.m_sOffroadReason = "OK off-road smoke";
		undercover.m_sEnemyProximityReason = "OK no enemy nearby smoke";
		undercover.m_sWantedHeatReason = "OK wanted heat clear smoke";
		undercover.m_sLastEligibilitySummary = "phase20 eligible smoke";
		undercover.m_sLastZoneId = SelectPhase20SmokeTownId();
		undercover.m_sLastReason = "phase20 eligible smoke";
		undercover.m_iLastEligibilityCheckSecond = m_State.m_iElapsedSeconds;

		MarkMajorCampaignChange();
		return "h-istasi phase 20 smoke | seeded eligible undercover\n" + m_Civilians.BuildUndercoverReport(m_State, ResolveTrustedIdentityId(playerId));
	}

	string RequestAdminPhase20ClearHeat(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 20 smoke | failed: admin required";
		if (!m_Civilians)
			return "h-istasi phase 20 smoke | failed: civilian service not ready";

		foreach (HST_CivilianZoneState town : m_State.m_aCivilianZones)
		{
			if (!town)
				continue;

			town.m_iWantedHeat = 0;
			town.m_sLastIncidentReason = "phase20 heat cleared";
		}

		HST_PlayerUndercoverState undercover = m_State.FindUndercoverPlayer(ResolveTrustedIdentityId(playerId));
		if (undercover)
		{
			undercover.m_iWantedHeat = 0;
			undercover.m_iCompromisedUntilSecond = 0;
			undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
			undercover.m_sLastReason = "phase20 heat cleared";
		}

		MarkMajorCampaignChange();
		return "h-istasi phase 20 smoke | heat cleared\n" + m_Civilians.BuildCivilianReport(m_State);
	}

	string RequestAdminPhase20Report(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 20 smoke | failed: admin required";

		string report = "h-istasi phase 20 smoke report";
		if (m_Civilians)
		{
			report = report + "\n" + m_Civilians.BuildTownSupportReport(m_State, 12);
			report = report + "\n" + m_Civilians.BuildUndercoverReport(m_State, ResolveTrustedIdentityId(playerId));
			HST_UndercoverEligibilityResult eligibility = m_Civilians.BuildUndercoverEligibility(m_State, ResolveTrustedIdentityId(playerId), ResolveControlledPlayerEntity(playerId));
			if (eligibility)
				report = report + "\n" + eligibility.BuildReport();
		}

		return report;
	}

	string RequestAdminPhase21ApplyUndercover(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 21 smoke | failed: admin required";
		if (!m_Civilians)
			return "h-istasi phase 21 smoke | failed: civilian service not ready";

		string identityId = ResolveTrustedIdentityId(playerId);
		string result = m_Civilians.RequestUndercover(m_State, identityId, ResolveControlledPlayerEntity(playerId));
		MarkMajorCampaignChange();
		return "h-istasi phase 21 smoke | apply undercover\n" + result + "\n" + m_Civilians.BuildUndercoverReport(m_State, identityId);
	}

	string RequestAdminPhase21SimulateWeaponFire(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 21 smoke | failed: admin required";
		if (!m_Civilians)
			return "h-istasi phase 21 smoke | failed: civilian service not ready";

		string identityId = ResolveTrustedIdentityId(playerId);
		string zoneId = ResolveNearestCivilianZoneIdForPlayer(playerId);
		string result = m_Civilians.RegisterUndercoverCombatExposure(m_State, identityId, zoneId, "phase21 simulated weapon fire");
		MarkMajorCampaignChange();
		return "h-istasi phase 21 smoke | weapon fire\n" + result + "\n" + m_Civilians.BuildUndercoverReport(m_State, identityId);
	}

	string RequestAdminPhase21SimulateMilitaryVehicle(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 21 smoke | failed: admin required";
		if (!m_Civilians)
			return "h-istasi phase 21 smoke | failed: civilian service not ready";

		string identityId = ResolveTrustedIdentityId(playerId);
		string zoneId = ResolveNearestCivilianZoneIdForPlayer(playerId);
		string result = m_Civilians.RegisterUndercoverVehicleExposure(m_State, identityId, zoneId, "phase21 simulated military vehicle");
		MarkMajorCampaignChange();
		return "h-istasi phase 21 smoke | military vehicle\n" + result + "\n" + m_Civilians.BuildUndercoverReport(m_State, identityId);
	}

	string RequestAdminPhase21SimulateRoadblock(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 21 smoke | failed: admin required";
		if (!m_Civilians)
			return "h-istasi phase 21 smoke | failed: civilian service not ready";

		string identityId = ResolveTrustedIdentityId(playerId);
		HST_CivilianZoneState town = SelectPhase21SmokeTown(playerId);
		if (!town)
			return "h-istasi phase 21 smoke | failed: no civilian town";

		PreparePhase21SmokeUndercover(identityId);
		town.m_iRoadblockPresence = Math.Max(3, town.m_iRoadblockPresence);
		town.m_iWantedHeat = Math.Max(2, town.m_iWantedHeat);
		town.m_iLastRoadblockScanSecond = m_State.m_iElapsedSeconds - 60;
		town.m_sLastSecurityReason = "phase21 roadblock smoke";

		HST_UndercoverEnforcementResult result = m_Civilians.EnforceUndercoverForPlayer(m_State, m_Preset, identityId, ResolveControlledPlayerEntity(playerId));
		MarkMajorCampaignChange();
		if (!result)
			return "h-istasi phase 21 smoke | failed: no roadblock result";

		return "h-istasi phase 21 smoke | roadblock scan\n" + result.BuildReport() + "\n" + m_Civilians.BuildUndercoverReport(m_State, identityId);
	}

	string RequestAdminPhase21SimulatePolice(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 21 smoke | failed: admin required";
		if (!m_Civilians)
			return "h-istasi phase 21 smoke | failed: civilian service not ready";

		string identityId = ResolveTrustedIdentityId(playerId);
		HST_CivilianZoneState town = SelectPhase21SmokeTown(playerId);
		if (!town)
			return "h-istasi phase 21 smoke | failed: no civilian town";

		PreparePhase21SmokeUndercover(identityId);
		town.m_iPolicePresence = Math.Max(4, town.m_iPolicePresence);
		town.m_iWantedHeat = Math.Max(3, town.m_iWantedHeat);
		town.m_iLastPoliceScanSecond = m_State.m_iElapsedSeconds - 60;
		town.m_sLastSecurityReason = "phase21 police smoke";

		HST_UndercoverEnforcementResult result = m_Civilians.EnforceUndercoverForPlayer(m_State, m_Preset, identityId, ResolveControlledPlayerEntity(playerId));
		MarkMajorCampaignChange();
		if (!result)
			return "h-istasi phase 21 smoke | failed: no police result";

		return "h-istasi phase 21 smoke | police scan\n" + result.BuildReport() + "\n" + m_Civilians.BuildUndercoverReport(m_State, identityId);
	}

	string RequestAdminPhase21ClearHeat(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 21 smoke | failed: admin required";
		if (!m_Civilians)
			return "h-istasi phase 21 smoke | failed: civilian service not ready";

		foreach (HST_CivilianZoneState town : m_State.m_aCivilianZones)
		{
			if (!town)
				continue;

			town.m_iWantedHeat = 0;
			town.m_sLastIncidentReason = "phase21 heat cleared";
			town.m_sLastSecurityReason = "phase21 heat cleared";
		}

		HST_PlayerUndercoverState undercover = m_State.FindUndercoverPlayer(ResolveTrustedIdentityId(playerId));
		if (undercover)
		{
			undercover.m_iWantedHeat = 0;
			undercover.m_iCompromisedUntilSecond = 0;
			undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
			undercover.m_sLastReason = "phase21 heat cleared";
			undercover.m_sLastCompromiseReason = "";
			undercover.m_sLastDetectionSource = "admin_clear_heat";
			undercover.m_iDetectionScore = 0;
			undercover.m_bLastRoadblockScanFailed = false;
			undercover.m_bLastPoliceScanFailed = false;
		}

		MarkMajorCampaignChange();
		return "h-istasi phase 21 smoke | heat cleared\n" + m_Civilians.BuildCivilianReport(m_State);
	}

	string RequestAdminPhase21Report(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 21 smoke | failed: admin required";

		string identityId = ResolveTrustedIdentityId(playerId);
		string report = "h-istasi phase 21 smoke report";
		if (m_Civilians)
		{
			report = report + "\n" + m_Civilians.BuildTownSupportReport(m_State, 12);
			report = report + "\n" + m_Civilians.BuildUndercoverReport(m_State, identityId);

			HST_UndercoverEnforcementResult enforcement = m_Civilians.EnforceUndercoverForPlayer(m_State, m_Preset, identityId, ResolveControlledPlayerEntity(playerId));
			if (enforcement)
			{
				if (enforcement.m_bChanged)
					MarkMajorCampaignChange();
				report = report + "\n" + enforcement.BuildReport();
			}
		}

		return report;
	}

	string RequestAdminPhase22SeedHQKnowledge(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 22 smoke | failed: admin required";
		if (!m_State || !m_HQ)
			return "h-istasi phase 22 smoke | failed: HQ service not ready";

		m_HQ.AddHQKnowledge(m_State, 100, "phase22 admin seed");
		m_State.m_iHQThreatLevel = Math.Max(m_State.m_iHQThreatLevel, m_State.m_iHQKnowledge);
		m_State.m_sLastHQThreatReason = "phase22 admin seed";
		m_State.m_iLastHQActivitySecond = m_State.m_iElapsedSeconds;
		MarkMajorCampaignChange(true);
		return "h-istasi phase 22 smoke | HQ knowledge seeded\n" + BuildPhase22Report();
	}

	string RequestAdminPhase22QueuePetrosAttack(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 22 smoke | failed: admin required";
		if (!m_State || !m_Preset || !m_EnemyCommander || !m_EnemyDirector)
			return "h-istasi phase 22 smoke | failed: enemy services not ready";

		string factionKey = m_Preset.m_sOccupierFactionKey;
		if (factionKey.IsEmpty())
			factionKey = "US";

		HST_EnemyOrderState order = m_EnemyCommander.QueueDebugPetrosAttack(m_State, m_Preset, m_EnemyDirector, factionKey);
		if (order)
		{
			if (m_HQ)
				m_HQ.AddHQKnowledge(m_State, 100, "phase22 Petros attack queued");
			EnsureDefendPetrosMissionForOrder(order);
			MarkMajorCampaignChange(true);
		}

		return string.Format("h-istasi phase 22 smoke | queue Petros attack %1\n%2", order != null, BuildPhase22Report());
	}

	string RequestAdminPhase22StartDefense(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 22 smoke | failed: admin required";
		if (!m_State || !m_Preset || !m_EnemyCommander || !m_EnemyDirector)
			return "h-istasi phase 22 smoke | failed: services not ready";

		HST_EnemyOrderState order = FindActivePetrosAttackOrder();
		if (!order)
		{
			string factionKey = m_Preset.m_sOccupierFactionKey;
			if (factionKey.IsEmpty())
				factionKey = "US";
			order = m_EnemyCommander.QueueDebugPetrosAttack(m_State, m_Preset, m_EnemyDirector, factionKey);
		}

		HST_ActiveMissionState mission = EnsureDefendPetrosMissionForOrder(order);
		if (mission)
			MarkMajorCampaignChange(true);

		return string.Format("h-istasi phase 22 smoke | start defense mission %1 | order %2\n%3", mission != null, order != null, BuildPhase22Report());
	}

	string RequestAdminPhase22KillPetros(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 22 smoke | failed: admin required";

		OnPetrosKilled();
		return "h-istasi phase 22 smoke | Petros killed\n" + BuildPhase22Report();
	}

	string RequestAdminPhase22SucceedDefense(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 22 smoke | failed: admin required";

		bool changed = CompleteDefendPetrosMission("phase22 admin success");
		if (changed)
			MarkMajorCampaignChange(true);
		return string.Format("h-istasi phase 22 smoke | succeed defense %1\n%2", changed, BuildPhase22Report());
	}

	string RequestAdminPhase22Report(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 22 smoke | failed: admin required";

		return BuildPhase22Report();
	}

	string RequestAdminPhase23UICoverageReport(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 23 | failed: admin required";
		if (!m_CommandUI)
			return "h-istasi phase 23 | failed: command UI not ready";

		return m_CommandUI.BuildCommandCoverageReport(m_State);
	}

	string RequestAdminPhase23MarkerAudit(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 23 marker audit | failed: admin required";
		if (!m_MapMarkers)
			return "h-istasi phase 23 marker audit | failed: marker service not ready";

		return m_MapMarkers.BuildMarkerAuditReport(m_State, m_Preset) + "\n" + m_MapMarkers.BuildMarkerReport(m_State);
	}

	string RequestAdminNativeMarkerReport(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi native marker report | failed: admin required";
		if (!m_MapMarkers)
			return "h-istasi native marker report | failed: marker service not ready";

		string report = m_MapMarkers.BuildNativeMarkerRuntimeReport(m_State);
		if (m_PlayerMapMarkers)
			return report + "\n" + m_PlayerMapMarkers.BuildRuntimeReport();

		return report + "\nh-istasi player marker report | service not ready";
	}

	string RequestAdminPurgeNativeHSTMarkers(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi admin | native marker purge failed: admin required";
		if (!m_MapMarkers)
			return "h-istasi admin | native marker purge failed: marker service not ready";

		string report = m_MapMarkers.AdminPurgeNativeHSTMarkers();
		if (!m_PlayerMapMarkers)
			return report + "\nplayer marker purge | service not ready";

		bool playerMarkersChanged = m_PlayerMapMarkers.ClearAll();
		if (m_PlayerMapMarkers.IsEnabled())
			m_PlayerMapMarkers.RequestRefresh("admin native marker purge");

		return report + string.Format("\nplayer marker purge | cleared %1 | enabled %2", playerMarkersChanged, m_PlayerMapMarkers.IsEnabled());
	}

	string RequestAdminPhase23FailedActionSample(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 23 failed-action sample | failed: admin required";

		string report = "h-istasi phase 23 failed-action sample";
		report = report + "\n" + RequestCommanderMoveHQReport(playerId, "missing_hideout_id");
		report = report + "\n" + RequestCommanderStartZoneMissionReport(playerId, "missing_zone_id");
		report = report + "\n" + RequestCommanderCompleteMissionReport(playerId, "missing_mission_id");
		return report;
	}
	string RequestAdminPhase24SeedEarlyGame(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 24 | failed: admin required";

		m_State.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		ResetCampaignEndState();
		SetAllPhase24StrategicZonesOwner(ResolvePhase24EnemyFactionKey());
		m_State.m_iFactionMoney = 750;
		m_State.m_iHR = 10;
		m_State.m_iTrainingLevel = 1;
		m_State.m_iWarLevel = 1;
		foreach (HST_FactionPoolState pool : m_State.m_aFactionPools)
		{
			if (!pool || pool.m_sFactionKey == m_Preset.m_sResistanceFactionKey)
				continue;

			pool.m_iAttackResources = Math.Min(pool.m_iAttackResources, 60);
			pool.m_iSupportResources = Math.Min(pool.m_iSupportResources, 60);
			pool.m_iAggression = 0;
		}

		MarkMajorCampaignChange(true);
		return "h-istasi phase 24 | early game seeded\n" + RequestMemberInspectBalancePacing(playerId);
	}

	string RequestAdminPhase24SeedMidGame(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 24 | failed: admin required";

		m_State.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		ResetCampaignEndState();
		SetAllPhase24StrategicZonesOwner(ResolvePhase24EnemyFactionKey());
		int changed;
		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (!zone)
				continue;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE || zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST)
			{
				zone.m_sOwnerFactionKey = m_Preset.m_sResistanceFactionKey;
				changed++;
			}
			if (changed >= 4)
				break;
		}

		m_Economy.RecalculateWarLevel(m_State, m_Balance, m_Preset.m_sResistanceFactionKey);
		m_State.m_iFactionMoney = Math.Max(m_State.m_iFactionMoney, 1500);
		m_State.m_iHR = Math.Max(m_State.m_iHR, 18);
		m_State.m_iTrainingLevel = Math.Max(m_State.m_iTrainingLevel, 3);
		MarkMajorCampaignChange(true);
		return "h-istasi phase 24 | mid game seeded\n" + RequestMemberInspectBalancePacing(playerId);
	}

	string RequestAdminPhase24SeedLateGame(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 24 | failed: admin required";

		m_State.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		ResetCampaignEndState();
		SetAllPhase24StrategicZonesOwner(ResolvePhase24EnemyFactionKey());
		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (!zone)
				continue;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE || zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY || zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST || zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
				zone.m_sOwnerFactionKey = m_Preset.m_sResistanceFactionKey;
		}

		m_Economy.RecalculateWarLevel(m_State, m_Balance, m_Preset.m_sResistanceFactionKey);
		m_State.m_iFactionMoney = Math.Max(m_State.m_iFactionMoney, 3000);
		m_State.m_iHR = Math.Max(m_State.m_iHR, 30);
		m_State.m_iTrainingLevel = Math.Max(m_State.m_iTrainingLevel, 6);
		MarkMajorCampaignChange(true);
		return "h-istasi phase 24 | late game seeded\n" + RequestMemberInspectBalancePacing(playerId);
	}

	string RequestAdminPhase24ForceVictory(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 24 | failed: admin required";

		m_State.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		ResetCampaignEndState();
		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (!zone)
				continue;
			if (zone.m_eType != HST_EZoneType.HST_ZONE_HIDEOUT && zone.m_eType != HST_EZoneType.HST_ZONE_MISSION_SITE)
				zone.m_sOwnerFactionKey = m_Preset.m_sResistanceFactionKey;
		}

		m_Economy.RecalculateWarLevel(m_State, m_Balance, m_Preset.m_sResistanceFactionKey);
		EvaluateCampaignOutcomeNow();
		MarkMajorCampaignChange(true);
		return "h-istasi phase 24 | victory forced\n" + m_Strategic.BuildCampaignEndReport(m_State, m_Economy, m_Balance, m_Preset);
	}

	string RequestAdminPhase24ForceLoss(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 24 | failed: admin required";

		m_State.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		ResetCampaignEndState();
		SetAllPhase24StrategicZonesOwner(ResolvePhase24EnemyFactionKey());
		m_State.m_iFactionMoney = 0;
		m_State.m_iHR = 0;
		m_State.m_iPetrosDeaths = Math.Max(m_State.m_iPetrosDeaths, m_Balance.m_iLossPetrosDeathLimit);
		m_State.m_iElapsedSeconds = Math.Max(m_State.m_iElapsedSeconds, m_Balance.m_iLossGraceSeconds + 1);
		EvaluateCampaignOutcomeNow();
		MarkMajorCampaignChange(true);
		return "h-istasi phase 24 | loss forced\n" + m_Strategic.BuildCampaignEndReport(m_State, m_Economy, m_Balance, m_Preset);
	}

	string RequestAdminPhase24Report(int playerId)
	{
		return RequestMemberInspectBalancePacing(playerId);
	}

	protected string BuildPhase22Report()
	{
		string report = "h-istasi phase 22 smoke report";
		if (m_HQ)
			report = report + "\n" + m_HQ.BuildHQThreatReport(m_State);
		if (m_EnemyCommander)
		{
			report = report + "\n" + m_EnemyCommander.BuildEnemyOrderReport(m_State);
			report = report + "\n" + m_EnemyCommander.BuildPhysicalResponseReport(m_State);
		}
		if (m_SupportRequests)
			report = report + "\n" + m_SupportRequests.BuildSupportReport(m_State);
		if (m_CommandUI)
			report = report + "\n" + m_CommandUI.BuildMissionReport(m_State);
		return report;
	}

	protected bool TickDefendPetros()
	{
		if (!m_State)
			return false;

		bool changed;
		HST_EnemyOrderState order = FindActivePetrosAttackOrder();
		if (!m_State.m_bDefendPetrosActive && order)
		{
			HST_ActiveMissionState createdMission = EnsureDefendPetrosMissionForOrder(order);
			if (createdMission)
				changed = true;
		}

		changed = SyncDefendPetrosLinks(order) || changed;
		changed = UpdateDefendPetrosOutcome() || changed;
		return changed;
	}

	protected HST_ActiveMissionState EnsureDefendPetrosMissionForOrder(HST_EnemyOrderState order)
	{
		if (!m_State)
			return null;

		HST_ActiveMissionState existingMission = FindActiveDefendPetrosMission();
		if (existingMission)
		{
			m_State.m_bDefendPetrosActive = true;
			m_State.m_sDefendPetrosMissionId = existingMission.m_sInstanceId;
			if (order)
				m_State.m_sDefendPetrosOrderId = order.m_sOrderId;
			SyncDefendPetrosLinks(order);
			return existingMission;
		}

		HST_MissionDefinition definition;
		if (m_Missions)
			definition = m_Missions.FindDefinition("dynamic_defend_petros");

		int durationSeconds = 2400;
		string displayName = "Defend Petros";
		if (definition)
		{
			durationSeconds = Math.Max(900, definition.m_iDurationSeconds);
			displayName = definition.m_sDisplayName;
		}

		string targetZoneId = ResolveHQDefenseZoneId(order);
		vector targetPosition = m_State.m_vPetrosPosition;
		if (IsZeroVector(targetPosition))
			targetPosition = m_State.m_vHQPosition;

		HST_ActiveMissionState mission = new HST_ActiveMissionState();
		mission.m_sInstanceId = string.Format("defend_petros_%1_%2", m_State.m_iElapsedSeconds, m_State.m_aActiveMissions.Count());
		mission.m_sMissionId = "dynamic_defend_petros";
		mission.m_sDisplayName = displayName;
		mission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		mission.m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_STATE_MACHINE;
		mission.m_iRemainingSeconds = durationSeconds;
		mission.m_sTargetZoneId = targetZoneId;
		mission.m_vTargetPosition = targetPosition;
		mission.m_sMarkerId = "hst_mission_" + mission.m_sInstanceId;
		mission.m_sRuntimePrimitive = "hold_area";
		mission.m_sRuntimeType = "dynamic_defend_petros";
		mission.m_sRuntimePhase = "active";
		mission.m_iStartedAtSecond = m_State.m_iElapsedSeconds;
		mission.m_iActiveUntilSecond = m_State.m_iElapsedSeconds + durationSeconds;
		mission.m_iRuntimeStartedAtSecond = m_State.m_iElapsedSeconds;
		mission.m_iRuntimeHoldSeconds = 600;
		mission.m_bDynamic = true;
		mission.m_bRequested = true;
		m_State.m_aActiveMissions.Insert(mission);

		CreateDefendPetrosObjective(mission, targetPosition, targetZoneId);
		UpsertDefendPetrosTask(mission, targetPosition);

		m_State.m_bDefendPetrosActive = true;
		m_State.m_sDefendPetrosMissionId = mission.m_sInstanceId;
		if (order)
			m_State.m_sDefendPetrosOrderId = order.m_sOrderId;
		m_State.m_sDefendPetrosStatus = "active";
		m_State.m_sDefendPetrosFailureReason = "";
		m_State.m_iDefendPetrosStartedSecond = m_State.m_iElapsedSeconds;
		m_State.m_iDefendPetrosEndsSecond = m_State.m_iElapsedSeconds + durationSeconds;
		m_State.m_iDefendPetrosLastUpdateSecond = m_State.m_iElapsedSeconds;
		m_State.m_iDefendPetrosAttackerCount = 0;
		m_State.m_iDefendPetrosAliveAttackerCount = 0;
		m_State.m_iDefendPetrosKilledCount = 0;
		m_State.m_bDefendPetrosOutcomeApplied = false;
		SyncDefendPetrosLinks(order);
		BroadcastMissionEvent("created", mission, definition);
		return mission;
	}

	protected void CreateDefendPetrosObjective(HST_ActiveMissionState mission, vector targetPosition, string targetZoneId)
	{
		if (!m_State || !mission)
			return;

		foreach (HST_MissionObjectiveState existingObjective : m_State.m_aMissionObjectives)
		{
			if (existingObjective && existingObjective.m_sMissionInstanceId == mission.m_sInstanceId)
				return;
		}

		HST_MissionObjectiveState objective = new HST_MissionObjectiveState();
		objective.m_sObjectiveId = "obj_" + mission.m_sInstanceId + "_hold_petros";
		objective.m_sMissionInstanceId = mission.m_sInstanceId;
		objective.m_eType = HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA;
		objective.m_sLabel = "Defend Petros";
		objective.m_sRequirementText = "Keep Petros alive and hold the HQ area until the attack breaks.";
		objective.m_sTargetId = "petros";
		objective.m_sTargetZoneId = targetZoneId;
		objective.m_sPhysicalEntityId = "phys_" + mission.m_sInstanceId + "_petros";
		objective.m_sLinkedRuntimeEntityId = objective.m_sPhysicalEntityId;
		objective.m_sRuntimePrimitive = "hold_area";
		objective.m_vPosition = targetPosition;
		objective.m_iRequiredProgress = 1;
		objective.m_iRequiredHoldSeconds = 600;
		objective.m_iRequiredCount = 1;
		m_State.m_aMissionObjectives.Insert(objective);
	}

	protected void UpsertDefendPetrosTask(HST_ActiveMissionState mission, vector targetPosition)
	{
		if (!m_State || !mission)
			return;

		string taskId = "task_" + mission.m_sInstanceId;
		HST_CampaignTaskState task = m_State.FindCampaignTask(taskId);
		if (!task)
		{
			task = new HST_CampaignTaskState();
			task.m_sTaskId = taskId;
			m_State.m_aCampaignTasks.Insert(task);
		}

		task.m_sLinkedId = mission.m_sInstanceId;
		task.m_sTitle = "Defend Petros";
		task.m_sDescription = "Hold the HQ area and keep Petros alive.";
		task.m_sCategory = "mission";
		task.m_vPosition = targetPosition;
		task.m_bActive = true;
		task.m_bSucceeded = false;
		task.m_bFailed = false;
	}

	protected string ResolveHQDefenseZoneId(HST_EnemyOrderState order)
	{
		if (order && !order.m_sTargetZoneId.IsEmpty())
			return order.m_sTargetZoneId;

		return SelectHQSupportZoneId();
	}

	protected HST_EnemyOrderState FindActivePetrosAttackOrder()
	{
		if (!m_State)
			return null;

		HST_EnemyOrderState linked = FindEnemyOrderById(m_State.m_sDefendPetrosOrderId);
		if (linked && linked.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK && linked.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return linked;

		for (int i = m_State.m_aEnemyOrders.Count() - 1; i >= 0; i--)
		{
			HST_EnemyOrderState order = m_State.m_aEnemyOrders[i];
			if (!order || order.m_eType != HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
				continue;
			if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE || order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED)
				return order;
		}

		return null;
	}

	protected HST_EnemyOrderState FindEnemyOrderById(string orderId)
	{
		if (!m_State || orderId.IsEmpty())
			return null;

		foreach (HST_EnemyOrderState order : m_State.m_aEnemyOrders)
		{
			if (order && order.m_sOrderId == orderId)
				return order;
		}

		return null;
	}

	protected HST_ActiveMissionState FindActiveDefendPetrosMission()
	{
		if (!m_State)
			return null;

		HST_ActiveMissionState linked = m_State.FindActiveMission(m_State.m_sDefendPetrosMissionId);
		if (linked && linked.m_sMissionId == "dynamic_defend_petros" && linked.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
			return linked;

		foreach (HST_ActiveMissionState mission : m_State.m_aActiveMissions)
		{
			if (mission && mission.m_sMissionId == "dynamic_defend_petros" && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				return mission;
		}

		return null;
	}

	protected bool SyncDefendPetrosLinks(HST_EnemyOrderState order)
	{
		if (!m_State || !m_State.m_bDefendPetrosActive)
			return false;

		bool changed;
		if (!order)
			order = FindActivePetrosAttackOrder();
		if (!order)
			order = FindEnemyOrderById(m_State.m_sDefendPetrosOrderId);

		if (order)
		{
			if (m_State.m_sDefendPetrosOrderId != order.m_sOrderId)
			{
				m_State.m_sDefendPetrosOrderId = order.m_sOrderId;
				changed = true;
			}
			if (m_State.m_sDefendPetrosSupportRequestId != order.m_sSupportRequestId)
			{
				m_State.m_sDefendPetrosSupportRequestId = order.m_sSupportRequestId;
				changed = true;
			}
			if (m_State.m_sDefendPetrosAttackerGroupId != order.m_sGroupId)
			{
				m_State.m_sDefendPetrosAttackerGroupId = order.m_sGroupId;
				changed = true;
			}
		}

		HST_SupportRequestState request = m_State.FindSupportRequest(m_State.m_sDefendPetrosSupportRequestId);
		if (request && m_State.m_sDefendPetrosAttackerGroupId != request.m_sGroupId)
		{
			m_State.m_sDefendPetrosAttackerGroupId = request.m_sGroupId;
			changed = true;
		}

		HST_ActiveGroupState group = m_State.FindActiveGroup(m_State.m_sDefendPetrosAttackerGroupId);
		int attackerCount;
		int aliveCount;
		int killedCount;
		if (group)
		{
			attackerCount = Math.Max(0, group.m_iInfantryCount) + Math.Max(0, group.m_iVehicleCount);
			aliveCount = Math.Max(0, group.m_iSurvivorInfantryCount) + Math.Max(0, group.m_iSurvivorVehicleCount);
			if (attackerCount > 0 && aliveCount <= 0 && group.m_sRuntimeStatus != "eliminated" && group.m_sRuntimeStatus != "folded" && group.m_sRuntimeStatus != "spawn_failed")
				aliveCount = attackerCount;
			killedCount = Math.Max(0, attackerCount - aliveCount);
		}

		if (m_State.m_iDefendPetrosAttackerCount != attackerCount)
		{
			m_State.m_iDefendPetrosAttackerCount = attackerCount;
			changed = true;
		}
		if (m_State.m_iDefendPetrosAliveAttackerCount != aliveCount)
		{
			m_State.m_iDefendPetrosAliveAttackerCount = aliveCount;
			changed = true;
		}
		if (m_State.m_iDefendPetrosKilledCount != killedCount)
		{
			m_State.m_iDefendPetrosKilledCount = killedCount;
			changed = true;
		}

		string status = ResolveDefendPetrosStatus(order, request, group);
		if (m_State.m_sDefendPetrosStatus != status)
		{
			m_State.m_sDefendPetrosStatus = status;
			changed = true;
		}

		m_State.m_iDefendPetrosLastUpdateSecond = m_State.m_iElapsedSeconds;
		return changed;
	}

	protected string ResolveDefendPetrosStatus(HST_EnemyOrderState order, HST_SupportRequestState request, HST_ActiveGroupState group)
	{
		if (!m_State.m_bPetrosAlive)
			return "failed_petros_killed";
		if (group)
			return "attackers_" + group.m_sRuntimeStatus;
		if (request)
			return "support_" + request.m_sRuntimeStatus;
		if (order)
			return "order_" + order.m_sRuntimeStatus;
		return "active";
	}

	protected bool UpdateDefendPetrosOutcome()
	{
		if (!m_State || !m_State.m_bDefendPetrosActive)
			return false;

		HST_ActiveMissionState mission = m_State.FindActiveMission(m_State.m_sDefendPetrosMissionId);
		if (!m_State.m_bPetrosAlive)
			return FailDefendPetrosMission("Petros killed");

		if (mission)
		{
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED)
				return CompleteDefendPetrosMission("mission succeeded");
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED)
				return FailDefendPetrosMission("mission failed");
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED)
				return CompleteDefendPetrosMission("defense timer held");
		}

		HST_SupportRequestState request = m_State.FindSupportRequest(m_State.m_sDefendPetrosSupportRequestId);
		if (request && request.m_sGroupId.IsEmpty() && request.m_sRuntimeStatus.Contains("physicalize_failed"))
			return FailDefendPetrosMission(request.m_sFailureReason);

		HST_ActiveGroupState group = m_State.FindActiveGroup(m_State.m_sDefendPetrosAttackerGroupId);
		if (group && (group.m_sRuntimeStatus == "eliminated" || group.m_sRuntimeStatus == "folded"))
			return CompleteDefendPetrosMission("attackers neutralized");
		if (group && group.m_sRuntimeStatus == "spawn_failed")
			return FailDefendPetrosMission(group.m_sSpawnFailureReason);

		if (m_State.m_iDefendPetrosEndsSecond > 0 && m_State.m_iElapsedSeconds >= m_State.m_iDefendPetrosEndsSecond)
			return CompleteDefendPetrosMission("defense timer held");

		return false;
	}

	protected bool FailDefendPetrosMission(string reason)
	{
		if (!m_State)
			return false;
		if (reason.IsEmpty())
			reason = "Defend Petros failed";

		bool changed;
		HST_ActiveMissionState mission = m_State.FindActiveMission(m_State.m_sDefendPetrosMissionId);
		bool missionWasActive = mission && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE;
		if (missionWasActive)
		{
			mission.m_sRuntimeFailureReason = reason;
			m_State.m_bDefendPetrosOutcomeApplied = true;
			changed = FailMission(mission.m_sInstanceId) || changed;
			if (m_HQ)
				changed = m_HQ.AddHQKnowledge(m_State, 25, "Defend Petros failed: " + reason) || changed;
		}
		else if (!m_State.m_bDefendPetrosOutcomeApplied && !missionWasActive)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_FAILED)
			{
				if (m_HQ)
					changed = m_HQ.AddHQKnowledge(m_State, 25, "Defend Petros failed: " + reason) || changed;
			}
			m_State.m_bDefendPetrosOutcomeApplied = true;
		}

		m_State.m_bDefendPetrosActive = false;
		m_State.m_sDefendPetrosStatus = "failed";
		m_State.m_sDefendPetrosFailureReason = reason;
		m_State.m_iDefendPetrosLastUpdateSecond = m_State.m_iElapsedSeconds;
		SetDefendPetrosTaskOutcome(mission, true);
		SetDefendPetrosOrderOutcome(false, reason);
		changed = true;
		return changed;
	}

	protected bool CompleteDefendPetrosMission(string reason)
	{
		if (!m_State)
			return false;
		if (reason.IsEmpty())
			reason = "Defend Petros succeeded";

		bool changed;
		HST_ActiveMissionState mission = m_State.FindActiveMission(m_State.m_sDefendPetrosMissionId);
		if (!mission)
			mission = FindActiveDefendPetrosMission();
		if (!mission && !m_State.m_bDefendPetrosActive)
			return false;

		bool shouldApplyOutcome = !m_State.m_bDefendPetrosOutcomeApplied;
		m_State.m_bDefendPetrosOutcomeApplied = true;
		if (mission)
		{
			foreach (HST_MissionObjectiveState objective : m_State.m_aMissionObjectives)
			{
				if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
					continue;

				objective.m_bFailed = false;
				if (!objective.m_bComplete)
				{
					objective.m_iCurrentProgress = objective.m_iRequiredProgress;
					objective.m_iCurrentCount = Math.Max(1, objective.m_iRequiredCount);
					objective.m_bComplete = true;
					changed = true;
				}
			}

			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				changed = CompleteMission(mission.m_sInstanceId) || changed;
			else if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED && m_Missions)
				changed = m_Missions.Complete(m_State, m_Economy, mission.m_sInstanceId, false, true) || changed;
		}

		if (shouldApplyOutcome && m_HQ)
			changed = m_HQ.ReduceHQKnowledge(m_State, 60, "Defend Petros succeeded: " + reason) || changed;

		m_State.m_bDefendPetrosActive = false;
		m_State.m_sDefendPetrosStatus = "succeeded";
		m_State.m_sDefendPetrosFailureReason = "";
		m_State.m_iDefendPetrosLastUpdateSecond = m_State.m_iElapsedSeconds;
		SetDefendPetrosTaskOutcome(mission, false);
		SetDefendPetrosOrderOutcome(true, reason);
		changed = true;
		return changed;
	}

	protected void SetDefendPetrosTaskOutcome(HST_ActiveMissionState mission, bool failed)
	{
		if (!m_State || !mission)
			return;

		HST_CampaignTaskState task = m_State.FindCampaignTask("task_" + mission.m_sInstanceId);
		if (!task)
			return;

		task.m_bActive = false;
		task.m_bFailed = failed;
		task.m_bSucceeded = !failed;
	}

	protected void SetDefendPetrosOrderOutcome(bool succeeded, string reason)
	{
		HST_EnemyOrderState order = FindEnemyOrderById(m_State.m_sDefendPetrosOrderId);
		if (!order)
			return;

		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED;
		order.m_iResolvedAtSecond = m_State.m_iElapsedSeconds;
		order.m_bOutcomeApplied = true;
		if (succeeded)
		{
			order.m_sResolutionKind = "defend_petros_succeeded";
			order.m_sRuntimeStatus = "resolved_defense_succeeded";
		}
		else
		{
			order.m_sResolutionKind = "defend_petros_failed";
			order.m_sRuntimeStatus = "resolved_defense_failed";
			order.m_sFailureReason = reason;
		}
	}

	protected void PreparePhase21SmokeUndercover(string identityId)
	{
		if (!m_Civilians || !m_State || identityId.IsEmpty())
			return;

		HST_PlayerUndercoverState undercover = m_Civilians.EnsurePlayer(m_State, identityId);
		if (!undercover)
			return;

		undercover.m_bUndercoverRequested = true;
		undercover.m_bUndercoverApplied = true;
		undercover.m_bEnforcementEnabled = true;
		undercover.m_sAppliedMode = "phase21_smoke";
		undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
		undercover.m_iCompromisedUntilSecond = 0;
		undercover.m_iLastEnforcementSecond = m_State.m_iElapsedSeconds - 60;
		undercover.m_sLastReason = "phase21 smoke prepared";
	}

	protected HST_CivilianZoneState SelectPhase21SmokeTown(int playerId)
	{
		if (!m_State)
			return null;

		string zoneId = ResolveNearestCivilianZoneIdForPlayer(playerId);
		if (!zoneId.IsEmpty())
		{
			HST_CivilianZoneState town = m_State.FindCivilianZone(zoneId);
			if (town)
				return town;
		}

		return SelectPhase20SmokeTown();
	}

	protected string ResolveNearestCivilianZoneIdForPlayer(int playerId)
	{
		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		if (!playerEntity || !m_State)
			return "";

		vector position = playerEntity.GetOrigin();
		string bestZoneId;
		float bestDistanceSq = 999999999.0;
		foreach (HST_CivilianZoneState town : m_State.m_aCivilianZones)
		{
			if (!town)
				continue;

			HST_ZoneState zone = m_State.FindZone(town.m_sZoneId);
			if (!zone)
				continue;

			float distanceSq = DistanceSq2D(position, zone.m_vPosition);
			if (distanceSq < bestDistanceSq)
			{
				bestDistanceSq = distanceSq;
				bestZoneId = town.m_sZoneId;
			}
		}

		return bestZoneId;
	}
	protected HST_CivilianZoneState SelectPhase20SmokeTown()
	{
		if (!m_State)
			return null;

		foreach (HST_CivilianZoneState town : m_State.m_aCivilianZones)
		{
			if (town)
				return town;
		}

		return null;
	}

	protected string SelectPhase20SmokeTownId()
	{
		HST_CivilianZoneState town = SelectPhase20SmokeTown();
		if (town)
			return town.m_sZoneId;

		return "";
	}

	protected HST_ZoneState SelectPhase16FriendlyRecruitZone()
	{
		if (!m_State || !m_Preset)
			return null;

		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (!zone)
				continue;

			if (zone.m_sOwnerFactionKey == m_Preset.m_sResistanceFactionKey && zone.m_iGarrisonSlots > 0)
				return zone;
		}

		foreach (HST_ZoneState fallback : m_State.m_aZones)
		{
			if (fallback && fallback.m_iGarrisonSlots > 0)
				return fallback;
		}

		return null;
	}

	protected HST_ZoneState SelectPhase17CaptureTarget()
	{
		if (!m_State || !m_Preset)
			return null;

		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (!zone)
				continue;
			if (zone.m_sOwnerFactionKey == m_Preset.m_sResistanceFactionKey)
				continue;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST)
				return zone;
		}

		foreach (HST_ZoneState fallback : m_State.m_aZones)
		{
			if (!fallback)
				continue;
			if (fallback.m_sOwnerFactionKey == m_Preset.m_sResistanceFactionKey)
				continue;
			if (IsPhase17CapturableZone(fallback))
				return fallback;
		}

		return null;
	}

	protected HST_ZoneState SelectEnemyOrderTargetZone(bool includeResistanceOwned)
	{
		if (!m_State || !m_Preset)
			return null;

		string resistanceFactionKey = m_Preset.m_sResistanceFactionKey;
		if (resistanceFactionKey.IsEmpty())
			resistanceFactionKey = "FIA";

		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (!zone)
				continue;
			if (!includeResistanceOwned && zone.m_sOwnerFactionKey == resistanceFactionKey)
				continue;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
				continue;
			if (zone.m_sOwnerFactionKey == m_Preset.m_sOccupierFactionKey || zone.m_sOwnerFactionKey == m_Preset.m_sInvaderFactionKey)
				return zone;
		}

		foreach (HST_ZoneState fallback : m_State.m_aZones)
		{
			if (!fallback)
				continue;
			if (!includeResistanceOwned && fallback.m_sOwnerFactionKey == resistanceFactionKey)
				continue;
			if (fallback.m_eType != HST_EZoneType.HST_ZONE_HIDEOUT && fallback.m_eType != HST_EZoneType.HST_ZONE_MISSION_SITE)
				return fallback;
		}

		return null;
	}

	protected HST_ZoneState SelectTownOrderTargetZone()
	{
		if (!m_State || !m_Preset)
			return null;

		string resistanceFactionKey = m_Preset.m_sResistanceFactionKey;
		if (resistanceFactionKey.IsEmpty())
			resistanceFactionKey = "FIA";

		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (zone && zone.m_eType == HST_EZoneType.HST_ZONE_TOWN && zone.m_sOwnerFactionKey != resistanceFactionKey)
				return zone;
		}

		foreach (HST_ZoneState fallback : m_State.m_aZones)
		{
			if (fallback && fallback.m_eType == HST_EZoneType.HST_ZONE_TOWN)
				return fallback;
		}

		return null;
	}
	protected HST_ZoneState SelectFirstResistanceCapturableZone()
	{
		if (!m_State || !m_Preset)
			return null;

		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (!zone)
				continue;
			if (zone.m_sOwnerFactionKey != m_Preset.m_sResistanceFactionKey)
				continue;
			if (IsPhase17CapturableZone(zone))
				return zone;
		}

		return null;
	}

	protected bool IsPhase17CapturableZone(HST_ZoneState zone)
	{
		if (!zone)
			return false;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return true;

		return false;
	}

	string RequestAdminPhase15SeedGarage(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 15 smoke | admin required";

		if (!m_State || !m_Arsenal)
			return "h-istasi phase 15 smoke | arsenal service not ready";

		HST_GarageVehicleState vehicle = new HST_GarageVehicleState();
		vehicle.m_sVehicleId = string.Format("phase15_smoke_garage_%1_%2", m_State.m_iElapsedSeconds, m_State.m_aGarageVehicles.Count());
		vehicle.m_sPrefab = PHASE15_SMOKE_VEHICLE_PREFAB;
		vehicle.m_sDisplayName = "Phase 15 Smoke Vehicle";
		vehicle.m_sSourceZoneId = "phase15_smoke";
		vehicle.m_sSourceFactionKey = "US";
		vehicle.m_iStoredAtSecond = m_State.m_iElapsedSeconds;
		vehicle.m_iRedeployCost = 25;
		vehicle.m_vPosition = m_State.m_vHQPosition;
		vehicle.m_vAngles = "0 0 0";
		vehicle.m_fFuel = 1.0;
		vehicle.m_sDamageState = "ok";
		vehicle.m_bArmed = false;
		vehicle.m_bUnlocked = true;
		HST_VehicleCapabilityPolicy.ApplyToGarageVehicle(vehicle);

		HST_StoredVehicleCargoState cargo = new HST_StoredVehicleCargoState();
		cargo.m_sItemPrefab = PHASE15_SMOKE_CARGO_PREFAB;
		cargo.m_sDisplayName = "Phase 15 Smoke Cargo";
		cargo.m_sCategory = "smoke";
		cargo.m_sSource = "phase15";
		cargo.m_iCount = 2;
		vehicle.m_aStoredCargoItems.Insert(cargo);

		if (!m_Arsenal.StoreVehicle(m_State, vehicle))
			return "h-istasi phase 15 smoke | failed: garage vehicle not stored";

		MarkMajorCampaignChange();
		return "h-istasi phase 15 smoke | seeded garage vehicle\n" + m_Arsenal.BuildGarageReport(m_State);
	}

	string RequestAdminPhase15SeedSourceVehicle(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 15 smoke | admin required";

		if (!m_State || !m_Arsenal)
			return "h-istasi phase 15 smoke | arsenal service not ready";

		string sourcePrefab = ResolveFirstLoadablePhase15SourceVehiclePrefab();
		if (sourcePrefab.IsEmpty())
			return "h-istasi phase 15 smoke | failed: no loadable source vehicle prefab candidate configured";

		HST_GarageVehicleState vehicle = new HST_GarageVehicleState();
		vehicle.m_sVehicleId = string.Format("phase15_smoke_source_%1_%2", m_State.m_iElapsedSeconds, m_State.m_aGarageVehicles.Count());
		vehicle.m_sPrefab = sourcePrefab;
		vehicle.m_sDisplayName = "Phase 15 Ammo Source Vehicle";
		vehicle.m_sSourceZoneId = "phase15_smoke";
		vehicle.m_sSourceFactionKey = "US";
		vehicle.m_iStoredAtSecond = m_State.m_iElapsedSeconds;
		vehicle.m_iRedeployCost = 50;
		vehicle.m_vPosition = m_State.m_vHQPosition;
		vehicle.m_vAngles = "0 0 0";
		vehicle.m_fFuel = 1.0;
		vehicle.m_sDamageState = "ok";
		vehicle.m_bUnlocked = true;

		if (!m_Arsenal.StoreVehicle(m_State, vehicle))
			return "h-istasi phase 15 smoke | failed: source vehicle not stored";

		ApplyPhase15SmokeSourceCapability(vehicle);
		MarkMajorCampaignChange();
		return "h-istasi phase 15 smoke | seeded explicit ammo-source metadata\n" + m_Arsenal.BuildGarageReport(m_State);
	}

	string RequestAdminPhase15Report(int playerId)
	{
		if (!Replication.IsServer() || !CanPlayerUseAdminActions(playerId))
			return "h-istasi phase 15 smoke | admin required";

		string report = "h-istasi phase 15 smoke report";
		if (m_Arsenal)
			report = report + "\n" + m_Arsenal.BuildGarageReport(m_State);
		if (m_Loot)
			report = report + "\n" + m_Loot.BuildVehicleCargoReport(m_State);

		report = report + string.Format(
			"\nsource vehicles | garage ammo %1 | garage repair %2 | garage fuel %3 | runtime ammo %4 | runtime repair %5 | runtime fuel %6",
			CountGarageSourceVehicles(true, false, false),
			CountGarageSourceVehicles(false, true, false),
			CountGarageSourceVehicles(false, false, true),
			CountRuntimeSourceVehicles(true, false, false),
			CountRuntimeSourceVehicles(false, true, false),
			CountRuntimeSourceVehicles(false, false, true)
		);

		return report;
	}

	protected string ResolveFirstLoadablePhase15SourceVehiclePrefab()
	{
		array<string> candidates = {};
		candidates.Insert("{F1FBD0972FA5FE09}Prefabs/Vehicles/Wheeled/M923A1/M923A1_transport.et");
		candidates.Insert("{81FDAD5EB644CC3D}Prefabs/Vehicles/Wheeled/M923A1/M923A1_transport_covered.et");
		candidates.Insert("{16C1F16C9B053801}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_transport.et");
		candidates.Insert(PHASE15_SMOKE_VEHICLE_PREFAB);

		foreach (string prefab : candidates)
		{
			if (prefab.IsEmpty())
				continue;

			if (!HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
				continue;

			Resource resource = Resource.Load(prefab);
			if (resource && resource.IsValid())
				return prefab;
		}

		return "";
	}

	protected void ApplyPhase15SmokeSourceCapability(HST_GarageVehicleState vehicle)
	{
		if (!vehicle)
			return;

		vehicle.m_sSourceVehicleKind = "ammo";
		vehicle.m_bAmmoSource = true;
		vehicle.m_bRepairSource = false;
		vehicle.m_bFuelSource = false;
	}

	protected int CountGarageSourceVehicles(bool ammoSource, bool repairSource, bool fuelSource)
	{
		if (!m_State)
			return 0;

		int count;
		foreach (HST_GarageVehicleState vehicle : m_State.m_aGarageVehicles)
		{
			if (!vehicle)
				continue;

			if (ammoSource && !vehicle.m_bAmmoSource)
				continue;
			if (repairSource && !vehicle.m_bRepairSource)
				continue;
			if (fuelSource && !vehicle.m_bFuelSource)
				continue;

			count++;
		}

		return count;
	}

	protected int CountRuntimeSourceVehicles(bool ammoSource, bool repairSource, bool fuelSource)
	{
		if (!m_State)
			return 0;

		int count;
		foreach (HST_RuntimeVehicleState vehicle : m_State.m_aRuntimeVehicles)
		{
			if (!vehicle || vehicle.m_bDeleted)
				continue;

			if (ammoSource && !vehicle.m_bAmmoSource)
				continue;
			if (repairSource && !vehicle.m_bRepairSource)
				continue;
			if (fuelSource && !vehicle.m_bFuelSource)
				continue;

			count++;
		}

		return count;
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
		EvaluateCampaignOutcomeNow();
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
			state.m_iTrainingLevel = Math.Max(1, m_Balance.m_iStartingTrainingLevel);
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

		if (m_State.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_SETUP && m_State.m_bHQDeployed && m_HQ)
		{
			m_HQ.ResetInitialHQSelection(m_State);
			m_State.m_sLastPersistenceStatus = "setup HQ selection pending";
			Print("h-istasi | setup campaign had a preselected HQ; reset to commander placement flow");
		}
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

	protected string ResolvePhase24EnemyFactionKey()
	{
		if (m_Preset && !m_Preset.m_sOccupierFactionKey.IsEmpty())
			return m_Preset.m_sOccupierFactionKey;
		if (m_Preset && !m_Preset.m_sInvaderFactionKey.IsEmpty())
			return m_Preset.m_sInvaderFactionKey;
		return "US";
	}

	protected void SetAllPhase24StrategicZonesOwner(string ownerFactionKey)
	{
		if (!m_State || ownerFactionKey.IsEmpty())
			return;

		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (!zone || zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
				continue;

			zone.m_sOwnerFactionKey = ownerFactionKey;
		}
	}

	protected void ResetCampaignEndState()
	{
		m_State.m_sCampaignEndReason = "";
		m_State.m_sCampaignEndSummary = "";
		m_State.m_iCampaignEndedAtSecond = 0;
		m_State.m_iCampaignEndControlPercent = 0;
		m_State.m_iCampaignEndWarLevel = 0;
		m_State.m_iCampaignEndFIAZones = 0;
		m_State.m_iCampaignEndEnemyZones = 0;
		m_State.m_bCampaignEndReportGenerated = false;
	}

	protected bool EvaluateCampaignOutcomeNow()
	{
		if (!m_State || !m_Strategic || !m_Economy || !m_Balance || !m_Preset || m_State.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
			return false;

		HST_CampaignOutcomeResult outcome = m_Strategic.EvaluateCampaignOutcomeDetailed(m_State, m_Economy, m_Balance, m_Preset.m_sResistanceFactionKey);
		if (!outcome || !outcome.m_bEnded)
			return false;

		m_Strategic.ApplyCampaignOutcome(m_State, outcome);
		return true;
	}

	bool IsCampaignActiveForVisibleMutatingCommand()
	{
		return IsCampaignActiveForMutatingAction();
	}

	protected bool IsCampaignActiveForMutatingAction()
	{
		return m_State && m_State.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
	}

	protected string CampaignInactiveMutationMessage(string prefix)
	{
		return prefix + " | failed: campaign is not active";
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
			{
				markerRefreshNeeded = true;
				if (m_PhysicalWar && m_Preset)
				{
					if (m_PhysicalWar.CleanupCapturedZoneHostileRuntime(m_State, notification.m_sZoneId, m_Preset.m_sResistanceFactionKey))
						markerRefreshNeeded = true;
				}
			}
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

		if ((asset.m_sKind == "target" || asset.m_sRole == "destroy_target") && !asset.m_bDestroyed && asset.m_fDemolitionRequiredDamage > 0.0)
			return string.Format("%1: %2 | demolition %3/%4", role, state, Math.Round(asset.m_fDemolitionDamage), Math.Round(asset.m_fDemolitionRequiredDamage));

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

	protected string BoolPayload(bool value)
	{
		if (value)
			return "1";

		return "0";
	}

	protected string BuildSetupResultPayload(string action, bool accepted, vector resolvedPosition, string message)
	{
		return string.Format(
			"HST_SETUP_RESULT|%1|%2|%3|%4|%5|%6",
			PayloadText(action),
			BoolPayload(accepted),
			resolvedPosition[0],
			resolvedPosition[1],
			resolvedPosition[2],
			PayloadText(message)
		);
	}

	protected HST_PlayerState EnsureSetupRegisteredPlayer(int playerId)
	{
		if (!m_State || !m_PlayerLifecycle || !m_Authorization || playerId <= 0)
			return null;

		string resolvedIdentityId = m_PlayerLifecycle.ResolveIdentityId(playerId, "");
		bool known = m_State.FindPlayer(resolvedIdentityId) != null;
		HST_PlayerState player = m_PlayerLifecycle.RegisterConnectedPlayer(m_State, m_Authorization, playerId, "", IsSettingsAdminIdentity(resolvedIdentityId) || IsDeveloperFallbackAdminIdentity(resolvedIdentityId));
		ApplyRuntimeMembershipDefaults(player);
		if (player && m_Civilians)
			m_Civilians.EnsurePlayer(m_State, player.m_sIdentityId);
		if (player && !known)
			MarkMajorCampaignChange(false);

		return player;
	}

	protected string CampaignPhaseLabelForSetup(HST_ECampaignPhase phase)
	{
		if (phase == HST_ECampaignPhase.HST_CAMPAIGN_SETUP)
			return "setup";
		if (phase == HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
			return "active";
		if (phase == HST_ECampaignPhase.HST_CAMPAIGN_WON)
			return "won";
		if (phase == HST_ECampaignPhase.HST_CAMPAIGN_LOST)
			return "lost";

		return "unknown";
	}

	protected string ResolveZoneTypeLabel(HST_EZoneType zoneType)
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
		if (zoneType == HST_EZoneType.HST_ZONE_POLICE_STATION)
			return "police";
		if (zoneType == HST_EZoneType.HST_ZONE_BANK)
			return "bank";
		if (zoneType == HST_EZoneType.HST_ZONE_HIDEOUT)
			return "hideout";
		if (zoneType == HST_EZoneType.HST_ZONE_MISSION_SITE)
			return "mission";

		return "zone";
	}

	protected float ResolveSetupZoneRadius(HST_ZoneState zone)
	{
		if (zone && zone.m_iCaptureRadiusMeters > 0)
			return zone.m_iCaptureRadiusMeters;

		return SETUP_ZONE_FALLBACK_RADIUS_METERS;
	}

	protected string ResolveSetupZoneTone(HST_ZoneState zone)
	{
		if (!zone)
			return "neutral";
		if (m_Preset && zone.m_sOwnerFactionKey == m_Preset.m_sResistanceFactionKey)
			return "resistance";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			return "town";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return "major";

		return "enemy";
	}

	protected bool TickUndercoverEnforcement()
	{
		if (!m_Civilians || !m_State)
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		bool changed;
		foreach (int playerId : playerIds)
		{
			if (!CanPlayerUseMemberActions(playerId))
				continue;

			string identityId = ResolveTrustedIdentityId(playerId);
			IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
			HST_UndercoverEnforcementResult result = m_Civilians.EnforceUndercoverForPlayer(m_State, m_Preset, identityId, playerEntity);
			if (result && result.m_bChanged)
				changed = true;
		}

		return changed;
	}
	protected string SelectFirstAdminZoneId()
	{
		if (!m_State || m_State.m_aZones.Count() == 0)
			return "";

		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (zone)
				return zone.m_sZoneId;
		}

		return "";
	}
	protected int CountResistanceZones()
	{
		if (!m_State || !m_Preset)
			return 0;

		int count;
		foreach (HST_ZoneState zone : m_State.m_aZones)
		{
			if (zone && zone.m_sOwnerFactionKey == m_Preset.m_sResistanceFactionKey)
				count++;
		}

		return count;
	}

	protected string ResolveFirstCancelablePlayerSupportRequestId()
	{
		if (!m_State)
			return "";

		foreach (HST_SupportRequestState request : m_State.m_aSupportRequests)
		{
			if (!request || !request.m_bPlayerRequested)
				continue;
			if (request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_QUEUED && request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
				continue;

			return request.m_sRequestId;
		}

		return "";
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

		if (definition.m_sMissionId == "dynamic_defend_petros")
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
				m_HQ.ReduceHQKnowledge(m_State, 20, "mission success: " + definition.m_sMissionId);
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

		bool changed;
		if (definition.m_sMissionId == "assassinate_traitor")
			changed = m_HQ.AddHQKnowledge(m_State, 35, "traitor escaped / failed assassination") || changed;

		if (definition.m_sMissionId == "dynamic_defend_petros")
		{
			if (!m_State.m_bDefendPetrosOutcomeApplied)
				changed = m_HQ.AddHQKnowledge(m_State, 25, "Defend Petros failed") || changed;
			if (m_State.m_iLastHQAttackSecond != m_State.m_iElapsedSeconds)
			{
				m_State.m_iLastHQAttackSecond = m_State.m_iElapsedSeconds;
				changed = true;
			}
			return changed;
		}

		if (definition.m_iFailureAggression >= 4)
			changed = m_HQ.AddHQKnowledge(m_State, 8 + definition.m_iFailureAggression, "high aggression mission failure: " + definition.m_sMissionId) || changed;

		if (changed)
			return true;

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
			captureSummary = captureSummary + string.Format(" | radius %1m | players %2 | FIA AI %3 | FIA veh %4", status.m_iCaptureRadiusMeters, status.m_iPlayerCountNearby, status.m_iFriendlyInfantryCountNearby, status.m_iFriendlyVehicleCountNearby);
			captureSummary = captureSummary + string.Format(" | enemies %1+%2v | %3", status.m_iEnemyCountNearby, status.m_iEnemyVehicleCountNearby, blockedReason);
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
		bool diagnostics = IsDebugLoggingEnabled() && (forceDiagnostics || (!isFrameSweep && m_iSpawnDiagnosticsRemaining > 0));
		if (diagnostics && !reason.IsEmpty())
			Print("h-istasi | FIA spawn sweep triggered by " + reason);

		if (m_State.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_SETUP)
		{
			DebugLog(string.Format("spawn sweep setup bootstrap reason=%1 hqDeployed=%2 commander=%3 players=%4", reason, m_State.m_bHQDeployed, m_State.m_sCommanderIdentityId, m_State.m_aPlayers.Count()));
			int setupSpawnRequests;
			int registrations = m_PlayerSpawn.PrepareSetupConnectedPlayers(m_State, m_Authorization, m_PlayerLifecycle, setupSpawnRequests, diagnostics);
			if (registrations > 0)
				MarkMajorCampaignChange(false);
			if (setupSpawnRequests > 0)
				DebugLog(string.Format("setup bootstrap spawn requests=%1", setupSpawnRequests));
			if (diagnostics && m_iSpawnDiagnosticsRemaining > 0)
				m_iSpawnDiagnosticsRemaining--;

			UpdateSpawnSweepArmedState();
			return 0;
		}

		int spawnRequests = m_PlayerSpawn.SpawnMissingConnectedPlayers(m_State, m_Authorization, m_PlayerLifecycle, diagnostics);
		DebugLog(string.Format("spawn sweep active reason=%1 requested=%2 hq=%3 deployed=%4", reason, spawnRequests, m_State.m_vHQPosition, m_State.m_bHQDeployed));
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

	protected bool IsDebugLoggingEnabled()
	{
		return m_Settings && m_Settings.m_Debug && m_Settings.m_Debug.m_bDebugLoggingEnabled;
	}

	protected void DebugLog(string message)
	{
		if (!IsDebugLoggingEnabled())
			return;

		Print("h-istasi debug | " + message);
	}
}
