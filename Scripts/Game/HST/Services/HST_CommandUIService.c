class HST_CommandMenuAction
{
	string m_sTabId;
	string m_sLabel;
	string m_sCommandId;
	string m_sArgument;
	bool m_bEnabled = true;
	string m_sDisabledReason;

	string ToPayloadLine()
	{
		return string.Format("ACTION|%1|%2|%3|%4|%5|%6", m_sTabId, m_sLabel, m_sCommandId, m_sArgument, m_bEnabled, m_sDisabledReason);
	}
}

class HST_CommandUIService
{
	static const string TAB_SETUP = "setup";
	static const string TAB_OVERVIEW = "overview";
	static const string TAB_GENERAL = "general";
	static const string TAB_PETROS = "petros";
	static const string TAB_MISSIONS = "missions";
	static const string TAB_MAP = "map";
	static const string TAB_FORCES = "forces";
	static const string TAB_COMMANDER = "commander";
	static const string TAB_ARSENAL = "arsenal";
	static const string TAB_GARAGE = "garage";
	static const string TAB_MEMBERS = "members";
	static const string TAB_ADMIN = "admin";

	string BuildMemberMenu(HST_CampaignState state, HST_CampaignPreset preset, HST_MapMarkerService markers)
	{
		if (!state)
			return "h-istasi command | campaign state not ready";

		string header = string.Format("h-istasi command | money %1 | HR %2 | war level %3 | training %4", state.m_iFactionMoney, state.m_iHR, state.m_iWarLevel, state.m_iTrainingLevel);
		string hq = string.Format("\nHQ: %1 at %2 | Petros alive %3", state.m_sHQHideoutId, state.m_vHQPosition, state.m_bPetrosAlive);
		string markerSummary;
		if (markers)
			markerSummary = "\n" + markers.BuildMarkerReport(state);

		return header + hq + markerSummary + "\nMember actions: foundation_status, inspect_campaign, inspect_markers, inspect_economy, inspect_zones, inspect_missions, inspect_active_missions, inspect_convoy_runtime, inspect_content, inspect_arsenal, loot_nearby, checkpoint";
	}

	string BuildCommanderMenu(HST_CampaignState state, HST_CampaignPreset preset, HST_MapMarkerService markers)
	{
		string menu = BuildMemberMenu(state, preset, markers);
		return menu + "\nCommander actions: move_hq_here, move_hq <hideout>, income_now, train_troops, recruit_zone <zone>, mission_zone <zone>, complete_mission <id>";
	}

	string BuildAdminMenu(HST_CampaignState state, HST_CampaignPreset preset, HST_MapMarkerService markers)
	{
		string menu = BuildCommanderMenu(state, preset, markers);
		return menu + "\nAdmin actions: activate_zone <zone>, deactivate_zone <zone>, capture_zone <zone>, progress_zone <zone>, debug_mission <zone>, award_small, admin_seed_persistence_test_state, admin_persistence_smoke_test, admin_persistence_smoke_report";
	}

	string BuildVisibleMenuPayload(HST_CampaignState state, HST_CampaignPreset preset, HST_MapMarkerService markers, HST_ArsenalService arsenal, HST_RuntimeSettings settings, HST_BalanceConfig balance, int playerId, string selectedTabId, string lastResult, bool canUseMember, bool canUseCommander, bool canUseAdmin, HST_ZoneCompositionService compositions = null, HST_ZoneCaptureService capture = null)
	{
		selectedTabId = NormalizeTabId(selectedTabId);

		string payload = string.Format("HST_MENU|%1|%2", selectedTabId, playerId);
		payload = payload + "\nTAB|setup|Setup|1";
		payload = payload + "\nTAB|overview|Overview|1";
		payload = payload + "\nTAB|petros|HQ/Petros|1";
		payload = payload + "\nTAB|missions|Missions|1";
		payload = payload + "\nTAB|map|Map/War|1";
		payload = payload + "\nTAB|forces|Forces|1";
		payload = payload + "\nTAB|arsenal|Arsenal/Loot|1";
		payload = payload + "\nTAB|garage|Garage/Build|1";
		payload = payload + "\nTAB|members|Members|1";
		payload = payload + string.Format("\nTAB|admin|Admin|%1", canUseAdmin);
		payload = payload + "\nSTATUS|" + BuildTabStatusText(state, preset, markers, arsenal, settings, selectedTabId, canUseMember, canUseCommander, canUseAdmin);
		payload = AppendTopStats(payload, state, preset);
		payload = AppendTabSections(payload, state, preset, markers, arsenal, settings, balance, selectedTabId, playerId, canUseMember, canUseCommander, canUseAdmin, compositions, capture);
		payload = AppendActivityFeed(payload, state, preset, selectedTabId);

		if (!lastResult.IsEmpty())
			payload = payload + "\nRESULT|" + lastResult;

		array<ref HST_CommandMenuAction> actions = {};
		BuildTabActions(state, preset, selectedTabId, playerId, actions, canUseMember, canUseCommander, canUseAdmin);
		foreach (HST_CommandMenuAction action : actions)
			payload = payload + "\n" + action.ToPayloadLine();

		payload = payload + "\nEND";
		return payload;
	}

	string ExecuteVisibleCommand(HST_CampaignCoordinatorComponent coordinator, int playerId, string commandId, string argument = "")
	{
		if (!coordinator || commandId.IsEmpty())
			return "h-istasi command | invalid request";

		if (commandId == "noop")
			return "h-istasi command | setup values are read from $profile:h-istasi/HST_Settings.json";

		if (commandId == "setup_hideout")
			return BuildBoolResult("select initial hideout " + argument, coordinator.RequestCommanderSelectInitialHideout(playerId, argument));

		if (commandId == "checkpoint")
			return coordinator.RequestMemberManualCheckpointReport(playerId);

		if (commandId == "foundation_status")
			return coordinator.RequestMemberFoundationStatus(playerId);

		if (commandId == "inspect_campaign")
			return coordinator.RequestMemberInspectCampaign(playerId);

		if (commandId == "inspect_markers")
			return coordinator.RequestMemberInspectMarkers(playerId);

		if (commandId == "inspect_economy")
			return coordinator.RequestMemberInspectEconomy(playerId);

		if (commandId == "inspect_zones")
			return coordinator.RequestMemberInspectZones(playerId);

		if (commandId == "inspect_missions")
			return coordinator.RequestMemberInspectMissions(playerId);

		if (commandId == "inspect_active_missions")
			return coordinator.RequestMemberInspectActiveMissions(playerId);

		if (commandId == "inspect_mission")
			return coordinator.RequestMemberInspectMission(playerId, argument);

		if (commandId == "inspect_convoy_runtime")
			return coordinator.RequestMemberInspectConvoyRuntime(playerId);

		if (commandId == "inspect_objectives")
			return coordinator.RequestMemberInspectObjectives(playerId);

		if (commandId == "inspect_arsenal")
			return coordinator.RequestMemberInspectArsenal(playerId);

		if (commandId == "inspect_garage")
			return coordinator.RequestMemberInspectGarage(playerId);

		if (commandId == "inspect_vehicle_cargo")
			return coordinator.RequestMemberInspectVehicleCargo(playerId);

		if (commandId == "inspect_support")
			return coordinator.RequestMemberInspectSupport(playerId);

		if (commandId == "inspect_civilians")
			return coordinator.RequestMemberInspectCivilians(playerId);

		if (commandId == "inspect_undercover")
			return coordinator.RequestMemberInspectUndercover(playerId);

		if (commandId == "inspect_content")
			return coordinator.RequestMemberInspectGeneratedContent(playerId);

		if (commandId == "inspect_persistence")
			return coordinator.RequestMemberInspectPersistence(playerId);

		if (commandId == "inspect_loadout_editor")
			return coordinator.RequestMemberInspectLoadoutEditor(playerId);

		if (commandId == "inspect_mission_runtime")
			return coordinator.RequestMemberInspectMissionRuntime(playerId);

		if (commandId == "loot_nearby")
			return coordinator.RequestMemberLootNearby(playerId);

		if (commandId == "withdraw_arsenal")
			return coordinator.RequestMemberWithdrawBestArsenalItem(playerId);

		if (commandId == "vehicle_collect_loot")
			return coordinator.RequestMemberCollectVehicleLoot(playerId, argument);

		if (commandId == "vehicle_unload_loot")
			return coordinator.RequestMemberUnloadVehicleCargo(playerId, argument);

		if (commandId == "loadout_editor_close")
			return coordinator.RequestMemberCloseLoadoutEditor(playerId);

		if (commandId == "loadout_save" || commandId == "save_loadout")
			return coordinator.RequestMemberSaveLoadoutDraft(playerId, argument);

		if (commandId == "loadout_apply" || commandId == "apply")
			return coordinator.RequestMemberApplySavedLoadout(playerId, argument);

		if (commandId == "loadout_add_item")
			return coordinator.RequestMemberAddLoadoutDraftItem(playerId, argument);

		if (commandId == "loadout_remove_slot" || commandId == "remove_storage_item")
			return coordinator.RequestMemberRemoveLoadoutDraftSlot(playerId, argument);

		if (commandId == "loadout_set_quantity")
			return coordinator.RequestMemberSetLoadoutDraftSlotQuantity(playerId, argument);

		if (commandId == "set_node_item" || commandId == "set_attachment" || commandId == "add_storage_item")
			return coordinator.RequestMemberSetLoadoutNodeItem(playerId, argument);

		if (commandId == "remove_node_item" || commandId == "remove_attachment")
			return coordinator.RequestMemberRemoveLoadoutNodeItem(playerId, argument);

		if (commandId == "loadout_clear_draft")
			return coordinator.RequestMemberClearLoadoutDraft(playerId);

		if (commandId == "loadout_select" || commandId == "load_loadout")
			return coordinator.RequestMemberSelectSavedLoadout(playerId, argument);

		if (commandId == "loadout_delete" || commandId == "delete_loadout")
			return coordinator.RequestMemberDeleteSavedLoadout(playerId, argument);

		if (commandId == "garage_capture_nearby")
			return coordinator.RequestMemberCaptureNearbyVehicle(playerId);

		if (commandId == "garage_redeploy")
			return coordinator.RequestMemberRedeployGarageVehicle(playerId, argument);

		if (commandId == "move_hq")
			return BuildBoolResult("move HQ to " + argument, coordinator.RequestCommanderMoveHQ(playerId, argument));

		if (commandId == "move_hq_here")
			return BuildBoolResult("move HQ to your position", coordinator.RequestCommanderMoveHQToPlayer(playerId));

		if (commandId == "rebuild_hq_assets")
			return BuildBoolResult("rebuild HQ assets", coordinator.RequestCommanderRebuildHQAssets(playerId));

		if (commandId == "income_now")
			return BuildBoolResult("apply income tick", coordinator.RequestCommanderApplyIncomeNow(playerId));

		if (commandId == "train_troops")
			return BuildBoolResult("train FIA troops", coordinator.RequestCommanderTrainTroops(playerId));

		if (commandId == "recruit_zone")
			return coordinator.RequestCommanderRecruitGarrisonReport(playerId, argument, 2, 0, 100, 1);

		if (commandId == "mission_zone")
			return BuildBoolResult("start zone mission at " + argument, coordinator.RequestCommanderStartZoneMission(playerId, argument));

		if (commandId == "mission_random")
			return BuildBoolResult("start random campaign mission", coordinator.RequestCommanderStartRandomMission(playerId));

		if (commandId == "progress_mission")
			return BuildBoolResult("progress active mission", coordinator.RequestCommanderProgressMission(playerId, argument));

		if (commandId == "complete_mission")
			return BuildBoolResult("complete mission " + argument, coordinator.RequestCommanderCompleteMission(playerId, argument));

		if (commandId == "mission_asset_load" || commandId == "mission_asset_unload" || commandId == "mission_asset_deliver" || commandId == "mission_captive_extract" || commandId == "mission_vehicle_capture" || commandId == "mission_asset_sabotage")
			return coordinator.RequestMemberMissionInteraction(playerId, commandId, argument);

		if (commandId == "call_supply")
			return BuildBoolResult("request FIA supply drop", coordinator.RequestCommanderCallSupplyDrop(playerId));

		if (commandId == "support_qrf")
			return BuildBoolResult("request FIA QRF reserve", coordinator.RequestCommanderCallPlayerSupport(playerId, HST_ESupportRequestType.HST_SUPPORT_QRF));

		if (commandId == "support_fire")
			return BuildBoolResult("request suppressive fire", coordinator.RequestCommanderCallPlayerSupport(playerId, HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE));

		if (commandId == "support_gbu")
			return BuildBoolResult("request GBU air strike", coordinator.RequestCommanderCallPlayerSupport(playerId, HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU));

		if (commandId == "support_umpk")
			return BuildBoolResult("request UMPK air strike", coordinator.RequestCommanderCallPlayerSupport(playerId, HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK));

		if (commandId == "support_kh55")
			return BuildBoolResult("request Kh55 strike", coordinator.RequestCommanderCallPlayerSupport(playerId, HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55));

		if (commandId == "cancel_support")
			return BuildBoolResult("cancel active player support", coordinator.RequestCommanderCancelSupport(playerId, argument));

		if (commandId == "civilian_aid")
			return BuildBoolResult("deliver civilian aid", coordinator.RequestCommanderAidNearestTown(playerId));

		if (commandId == "activate_zone")
			return BuildBoolResult("activate zone " + argument, coordinator.RequestAdminSetZoneActive(playerId, argument, true));

		if (commandId == "deactivate_zone")
			return BuildBoolResult("deactivate zone " + argument, coordinator.RequestAdminSetZoneActive(playerId, argument, false));

		if (commandId == "capture_zone")
			return BuildBoolResult("capture zone " + argument, coordinator.RequestAdminCaptureZoneForResistance(playerId, argument, 10));

		if (commandId == "progress_zone")
			return BuildBoolResult("add capture progress at " + argument, coordinator.RequestAdminAddCaptureProgress(playerId, argument, 50));

		if (commandId == "debug_mission")
			return BuildBoolResult("start debug mission at " + argument, coordinator.RequestAdminStartDebugMission(playerId, argument));

		if (commandId == "debug_mission_id")
			return coordinator.RequestAdminStartMissionById(playerId, argument);

		if (commandId == "award_small")
			return BuildBoolResult("award debug resources", coordinator.RequestAdminAwardResources(playerId, 500, 5));

		if (commandId == "admin_seed_persistence_test_state")
			return coordinator.RequestAdminSeedPersistenceTestState(playerId);

		if (commandId == "admin_persistence_smoke_test")
			return coordinator.RequestAdminRunPersistenceSmokeTest(playerId);

		if (commandId == "admin_persistence_smoke_report")
			return coordinator.RequestAdminPersistenceSmokeReport(playerId);

		if (commandId == "inspect_zone_composition")
			return coordinator.RequestAdminInspectZoneComposition(playerId);

		if (commandId == "new_campaign")
			return BuildBoolResult("reset campaign", coordinator.RequestAdminNewCampaignReset(playerId));

		if (commandId == "member_accept")
			return BuildBoolResult("accept member " + argument, coordinator.RequestAdminSetMembership(playerId, argument, true));

		if (commandId == "member_remove")
			return BuildBoolResult("remove member " + argument, coordinator.RequestAdminSetMembership(playerId, argument, false));

		if (commandId == "admin_grant")
			return BuildBoolResult("grant admin " + argument, coordinator.RequestAdminSetAdminRole(playerId, argument, true));

		return "h-istasi command | unknown command: " + commandId;
	}

	string BuildEconomyReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi economy | campaign state not ready";

		string summary = string.Format("h-istasi economy | money %1 | HR %2 | training %3 | income timer %4s", state.m_iFactionMoney, state.m_iHR, state.m_iTrainingLevel, state.m_iIncomeAccumulatorSeconds);
		string enemy = "";
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool)
				continue;

			enemy = enemy + string.Format("\n%1 | attack %2 | support %3 | aggression %4", pool.m_sFactionKey, pool.m_iAttackResources, pool.m_iSupportResources, pool.m_iAggression);
		}

		return summary + enemy;
	}

	string BuildZoneListReport(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return "h-istasi zones | campaign state not ready";

		int resistanceZones;
		int enemyZones;
		string details = "";
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			if (preset && zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
				resistanceZones++;
			else
				enemyZones++;

			details = details + string.Format("\n%1 | owner %2 | support %3 | income %4 | active %5 | capture %6", zone.m_sZoneId, zone.m_sOwnerFactionKey, zone.m_iSupport, zone.m_iIncomeValue, zone.m_bActive, zone.m_iResistanceCaptureProgress);
		}

		string header = string.Format("h-istasi zones | resistance %1 | enemy %2", resistanceZones, enemyZones);
		return header + details;
	}

	string BuildMissionReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi missions | campaign state not ready";

		string report = string.Format("h-istasi missions | active records %1 | objectives %2 | tasks %3", state.m_aActiveMissions.Count(), state.m_aMissionObjectives.Count(), state.m_aCampaignTasks.Count());
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission)
				continue;

			report = report + string.Format("\n%1 | %2 | target %3 | site %4 | status %5 | remaining %6s | phase %7 | progress %8", mission.m_sInstanceId, BuildMissionDisplayTitle(mission), mission.m_sTargetZoneId, mission.m_sSiteId, mission.m_eStatus, mission.m_iRemainingSeconds, mission.m_sRuntimePhase, BuildMissionProgressText(state, mission));
		}

		return report;
	}

	bool ExecuteQuickCommand(HST_CampaignCoordinatorComponent coordinator, int playerId, string commandId, string argument = "")
	{
		if (!coordinator || commandId.IsEmpty())
			return false;

		if (commandId == "checkpoint")
			return coordinator.RequestMemberManualCheckpoint(playerId);

		if (commandId == "setup_hideout")
			return coordinator.RequestCommanderSelectInitialHideout(playerId, argument);

		if (commandId == "inspect_campaign")
			return !coordinator.RequestMemberInspectCampaign(playerId).IsEmpty();

		if (commandId == "foundation_status")
			return !coordinator.RequestMemberFoundationStatus(playerId).IsEmpty();

		if (commandId == "inspect_markers")
			return !coordinator.RequestMemberInspectMarkers(playerId).IsEmpty();

		if (commandId == "inspect_economy")
			return !coordinator.RequestMemberInspectEconomy(playerId).IsEmpty();

		if (commandId == "inspect_zones")
			return !coordinator.RequestMemberInspectZones(playerId).IsEmpty();

		if (commandId == "inspect_missions")
			return !coordinator.RequestMemberInspectMissions(playerId).IsEmpty();

		if (commandId == "inspect_active_missions")
			return !coordinator.RequestMemberInspectActiveMissions(playerId).IsEmpty();

		if (commandId == "inspect_mission")
			return !coordinator.RequestMemberInspectMission(playerId, argument).IsEmpty();

		if (commandId == "inspect_convoy_runtime")
			return !coordinator.RequestMemberInspectConvoyRuntime(playerId).IsEmpty();

		if (commandId == "inspect_objectives")
			return !coordinator.RequestMemberInspectObjectives(playerId).IsEmpty();

		if (commandId == "inspect_arsenal")
			return !coordinator.RequestMemberInspectArsenal(playerId).IsEmpty();

		if (commandId == "inspect_garage")
			return !coordinator.RequestMemberInspectGarage(playerId).IsEmpty();

		if (commandId == "inspect_vehicle_cargo")
			return !coordinator.RequestMemberInspectVehicleCargo(playerId).IsEmpty();

		if (commandId == "inspect_support")
			return !coordinator.RequestMemberInspectSupport(playerId).IsEmpty();

		if (commandId == "inspect_civilians")
			return !coordinator.RequestMemberInspectCivilians(playerId).IsEmpty();

		if (commandId == "inspect_undercover")
			return !coordinator.RequestMemberInspectUndercover(playerId).IsEmpty();

		if (commandId == "inspect_content")
			return !coordinator.RequestMemberInspectGeneratedContent(playerId).IsEmpty();

		if (commandId == "inspect_persistence")
			return !coordinator.RequestMemberInspectPersistence(playerId).IsEmpty();

		if (commandId == "inspect_loadout_editor")
			return !coordinator.RequestMemberInspectLoadoutEditor(playerId).IsEmpty();

		if (commandId == "inspect_mission_runtime")
			return !coordinator.RequestMemberInspectMissionRuntime(playerId).IsEmpty();

		if (commandId == "loot_nearby")
			return !coordinator.RequestMemberLootNearby(playerId).IsEmpty();

		if (commandId == "withdraw_arsenal")
			return !coordinator.RequestMemberWithdrawBestArsenalItem(playerId).IsEmpty();

		if (commandId == "vehicle_collect_loot")
			return !coordinator.RequestMemberCollectVehicleLoot(playerId, argument).IsEmpty();

		if (commandId == "vehicle_unload_loot")
			return !coordinator.RequestMemberUnloadVehicleCargo(playerId, argument).IsEmpty();

		if (commandId == "loadout_editor_close")
			return !coordinator.RequestMemberCloseLoadoutEditor(playerId).IsEmpty();

		if (commandId == "loadout_save" || commandId == "save_loadout")
			return !coordinator.RequestMemberSaveLoadoutDraft(playerId, argument).IsEmpty();

		if (commandId == "loadout_apply" || commandId == "apply")
			return !coordinator.RequestMemberApplySavedLoadout(playerId, argument).IsEmpty();

		if (commandId == "loadout_add_item")
			return !coordinator.RequestMemberAddLoadoutDraftItem(playerId, argument).IsEmpty();

		if (commandId == "loadout_remove_slot" || commandId == "remove_storage_item")
			return !coordinator.RequestMemberRemoveLoadoutDraftSlot(playerId, argument).IsEmpty();

		if (commandId == "loadout_set_quantity")
			return !coordinator.RequestMemberSetLoadoutDraftSlotQuantity(playerId, argument).IsEmpty();

		if (commandId == "set_node_item" || commandId == "set_attachment" || commandId == "add_storage_item")
			return !coordinator.RequestMemberSetLoadoutNodeItem(playerId, argument).IsEmpty();

		if (commandId == "remove_node_item" || commandId == "remove_attachment")
			return !coordinator.RequestMemberRemoveLoadoutNodeItem(playerId, argument).IsEmpty();

		if (commandId == "loadout_clear_draft")
			return !coordinator.RequestMemberClearLoadoutDraft(playerId).IsEmpty();

		if (commandId == "loadout_select" || commandId == "load_loadout")
			return !coordinator.RequestMemberSelectSavedLoadout(playerId, argument).IsEmpty();

		if (commandId == "loadout_delete" || commandId == "delete_loadout")
			return !coordinator.RequestMemberDeleteSavedLoadout(playerId, argument).IsEmpty();

		if (commandId == "garage_capture_nearby")
			return IsActionResultComplete(coordinator.RequestMemberCaptureNearbyVehicle(playerId));

		if (commandId == "garage_redeploy")
			return IsActionResultComplete(coordinator.RequestMemberRedeployGarageVehicle(playerId, argument));

		if (commandId == "move_hq")
			return coordinator.RequestCommanderMoveHQ(playerId, argument);

		if (commandId == "move_hq_here")
			return coordinator.RequestCommanderMoveHQToPlayer(playerId);

		if (commandId == "rebuild_hq_assets")
			return coordinator.RequestCommanderRebuildHQAssets(playerId);

		if (commandId == "income_now")
			return coordinator.RequestCommanderApplyIncomeNow(playerId);

		if (commandId == "train_troops")
			return coordinator.RequestCommanderTrainTroops(playerId);

		if (commandId == "recruit_zone")
			return coordinator.RequestCommanderRecruitGarrison(playerId, argument, 2, 0, 100, 1);

		if (commandId == "mission_zone")
			return coordinator.RequestCommanderStartZoneMission(playerId, argument);

		if (commandId == "mission_random")
			return coordinator.RequestCommanderStartRandomMission(playerId);

		if (commandId == "progress_mission")
			return coordinator.RequestCommanderProgressMission(playerId, argument);

		if (commandId == "complete_mission")
			return coordinator.RequestCommanderCompleteMission(playerId, argument);

		if (commandId == "call_supply")
			return coordinator.RequestCommanderCallSupplyDrop(playerId);

		if (commandId == "support_qrf")
			return coordinator.RequestCommanderCallPlayerSupport(playerId, HST_ESupportRequestType.HST_SUPPORT_QRF);

		if (commandId == "support_fire")
			return coordinator.RequestCommanderCallPlayerSupport(playerId, HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE);

		if (commandId == "support_gbu")
			return coordinator.RequestCommanderCallPlayerSupport(playerId, HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU);

		if (commandId == "support_umpk")
			return coordinator.RequestCommanderCallPlayerSupport(playerId, HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK);

		if (commandId == "support_kh55")
			return coordinator.RequestCommanderCallPlayerSupport(playerId, HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55);

		if (commandId == "cancel_support")
			return coordinator.RequestCommanderCancelSupport(playerId, argument);

		if (commandId == "civilian_aid")
			return coordinator.RequestCommanderAidNearestTown(playerId);

		if (commandId == "activate_zone")
			return coordinator.RequestAdminSetZoneActive(playerId, argument, true);

		if (commandId == "deactivate_zone")
			return coordinator.RequestAdminSetZoneActive(playerId, argument, false);

		if (commandId == "capture_zone")
			return coordinator.RequestAdminCaptureZoneForResistance(playerId, argument, 10);

		if (commandId == "progress_zone")
			return coordinator.RequestAdminAddCaptureProgress(playerId, argument, 50);

		if (commandId == "debug_mission")
			return coordinator.RequestAdminStartDebugMission(playerId, argument);

		if (commandId == "debug_mission_id")
			return !coordinator.RequestAdminStartMissionById(playerId, argument).Contains("failed");

		if (commandId == "award_small")
			return coordinator.RequestAdminAwardResources(playerId, 500, 5);

		if (commandId == "admin_seed_persistence_test_state")
			return !coordinator.RequestAdminSeedPersistenceTestState(playerId).IsEmpty();

		if (commandId == "admin_persistence_smoke_test")
			return !coordinator.RequestAdminRunPersistenceSmokeTest(playerId).IsEmpty();

		if (commandId == "admin_persistence_smoke_report")
			return !coordinator.RequestAdminPersistenceSmokeReport(playerId).IsEmpty();

		if (commandId == "inspect_zone_composition")
			return !coordinator.RequestAdminInspectZoneComposition(playerId).IsEmpty();

		if (commandId == "new_campaign")
			return coordinator.RequestAdminNewCampaignReset(playerId);

		if (commandId == "member_accept")
			return coordinator.RequestAdminSetMembership(playerId, argument, true);

		if (commandId == "member_remove")
			return coordinator.RequestAdminSetMembership(playerId, argument, false);

		if (commandId == "admin_grant")
			return coordinator.RequestAdminSetAdminRole(playerId, argument, true);

		return false;
	}

	protected bool IsActionResultComplete(string result)
	{
		if (result.IsEmpty())
			return false;

		if (result.Contains("failed") || result.Contains("required") || result.Contains("not ready") || result.Contains("no "))
			return false;

		return true;
	}

	protected string NormalizeTabId(string selectedTabId)
	{
		if (selectedTabId.IsEmpty())
			return TAB_OVERVIEW;

		if (selectedTabId == TAB_GENERAL)
			return TAB_OVERVIEW;

		if (selectedTabId == TAB_COMMANDER)
			return TAB_FORCES;

		if (selectedTabId == "hq")
			return TAB_PETROS;

		if (selectedTabId == "vehicles")
			return TAB_GARAGE;

		return selectedTabId;
	}

	protected string BuildTabStatusText(HST_CampaignState state, HST_CampaignPreset preset, HST_MapMarkerService markers, HST_ArsenalService arsenal, HST_RuntimeSettings settings, string selectedTabId, bool canUseMember, bool canUseCommander, bool canUseAdmin)
	{
		if (selectedTabId == TAB_SETUP)
			return BuildSetupStatus(state, settings);

		if (!canUseMember && selectedTabId != TAB_SETUP)
			return "h-istasi command | membership required for campaign actions";

		if (selectedTabId == TAB_OVERVIEW)
			return BuildMemberMenu(state, preset, markers);

		if (selectedTabId == TAB_PETROS)
			return BuildPetrosStatus(state, canUseCommander);

		if (selectedTabId == TAB_MISSIONS)
			return BuildMissionReport(state);

		if (selectedTabId == TAB_MAP)
			return BuildZoneListReport(state, preset);

		if (selectedTabId == TAB_FORCES)
			return BuildEconomyReport(state);

		if (selectedTabId == TAB_ARSENAL)
		{
			if (arsenal)
				return arsenal.BuildArsenalReport(state);

			return "h-istasi arsenal | service not ready";
		}

		if (selectedTabId == TAB_GARAGE)
		{
			if (arsenal)
				return arsenal.BuildGarageReport(state);

			return "h-istasi garage | service not ready";
		}

		if (selectedTabId == TAB_MEMBERS)
			return BuildMembersStatus(state, canUseCommander);

		if (selectedTabId == TAB_ADMIN)
		{
			if (!canUseAdmin)
				return "h-istasi admin | admin role required";

			return BuildZoneListReport(state, preset);
		}

		return BuildMemberMenu(state, preset, markers);
	}

	protected string BuildSetupStatus(HST_CampaignState state, HST_RuntimeSettings settings)
	{
		string status = "h-istasi setup | choose an initial HQ hideout to start the campaign";
		if (state)
			status = status + string.Format("\ncampaign | schema %1 | preset %2 | phase %3 | seed %4 | HQ %5", state.m_iSchemaVersion, state.m_sPresetId, state.m_ePhase, state.m_iCampaignSeed, BuildHQLabel(state));

		if (settings)
			status = status + "\n" + settings.BuildSummary();
		else
			status = status + "\nsettings | built-in defaults active";

		return status;
	}

	protected string BuildPetrosStatus(HST_CampaignState state, bool canUseCommander)
	{
		if (!state)
			return "h-istasi Petros | campaign state not ready";

		string role = "commander role required for HQ moves";
		if (canUseCommander)
			role = "commander controls available";

		return string.Format("h-istasi Petros/HQ | hideout %1 | HQ %2 | Petros alive %3 | arsenal %4 | %5", state.m_sHQHideoutId, state.m_vHQPosition, state.m_bPetrosAlive, state.m_vArsenalPosition, role);
	}

	protected string BuildMembersStatus(HST_CampaignState state, bool canUseCommander)
	{
		if (!state)
			return "h-istasi members | campaign state not ready";

		string role = "member roster";
		if (canUseCommander)
			role = "commander can issue HQ orders";

		return string.Format("h-istasi members | known players %1 | commander %2 | %3", state.m_aPlayers.Count(), BuildCommanderName(state), role);
	}

	protected string AppendTopStats(string payload, HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
		{
			payload = AppendStat(payload, "Campaign", "state offline", "bad");
			return payload;
		}

		payload = AppendStat(payload, "FIA Money", string.Format("$%1", state.m_iFactionMoney), "good");
		payload = AppendStat(payload, "HR", string.Format("%1", state.m_iHR), "good");
		payload = AppendStat(payload, "War Level", string.Format("%1", state.m_iWarLevel), "warn");
		payload = AppendStat(payload, "Training", string.Format("%1", state.m_iTrainingLevel), "good");
		return payload;
	}

	protected string AppendTabSections(string payload, HST_CampaignState state, HST_CampaignPreset preset, HST_MapMarkerService markers, HST_ArsenalService arsenal, HST_RuntimeSettings settings, HST_BalanceConfig balance, string selectedTabId, int playerId, bool canUseMember, bool canUseCommander, bool canUseAdmin, HST_ZoneCompositionService compositions = null, HST_ZoneCaptureService capture = null)
	{
		if (selectedTabId == TAB_SETUP)
			return AppendSetupSections(payload, state, settings);

		if (!canUseMember)
		{
			payload = AppendSection(payload, "access", "Access");
			payload = AppendRow(payload, "access", "Membership", "Join the FIA roster before using campaign actions.", "bad");
			payload = AppendRow(payload, "access", "Available", "Setup tab remains readable.", "neutral");
			return payload;
		}

		if (selectedTabId == TAB_OVERVIEW)
			return AppendOverviewSections(payload, state, preset);

		if (selectedTabId == TAB_PETROS)
			return AppendHQSections(payload, state, settings, playerId, canUseCommander);

		if (selectedTabId == TAB_MISSIONS)
			return AppendMissionSections(payload, state);

		if (selectedTabId == TAB_MAP)
			return AppendMapSections(payload, state, preset, balance, capture);

		if (selectedTabId == TAB_FORCES)
			return AppendForcesSections(payload, state, preset, canUseCommander);

		if (selectedTabId == TAB_ARSENAL)
			return AppendArsenalSections(payload, state, settings, playerId);

		if (selectedTabId == TAB_GARAGE)
			return AppendGarageSections(payload, state, settings, playerId);

		if (selectedTabId == TAB_MEMBERS)
			return AppendMembersSections(payload, state, canUseCommander);

		if (selectedTabId == TAB_ADMIN)
			return AppendAdminSections(payload, state, preset, canUseAdmin, compositions);

		return AppendOverviewSections(payload, state, preset);
	}

	protected string AppendSetupSections(string payload, HST_CampaignState state, HST_RuntimeSettings settings)
	{
		payload = AppendSection(payload, "setup", "Campaign Setup");
		if (state)
		{
			payload = AppendRow(payload, "setup", "Schema", string.Format("%1", state.m_iSchemaVersion), "neutral");
			payload = AppendRow(payload, "setup", "Preset", state.m_sPresetId, "neutral");
			payload = AppendRow(payload, "setup", "Phase", CampaignPhaseLabel(state.m_ePhase), "neutral");
			payload = AppendRow(payload, "setup", "Seed", string.Format("%1", state.m_iCampaignSeed), "neutral");
			payload = AppendRow(payload, "setup", "HQ", BuildHQLabel(state), BuildRuntimeObjectTone(state));
			payload = AppendRow(payload, "setup", "Persistence", state.m_sLastPersistenceStatus, "neutral");
		}

		payload = AppendSection(payload, "hideouts", "Initial Hideout");
		payload = AppendRow(payload, "hideouts", "North Forest", "Woodland start near the northern road net.", "neutral");
		payload = AppendRow(payload, "hideouts", "Central Hills", "Balanced central start for Workbench smoke tests.", "good");
		payload = AppendRow(payload, "hideouts", "South Woods", "Southern staging area closer to invader pressure.", "neutral");

		payload = AppendSection(payload, "source", "Source Of Truth");
		payload = AppendRow(payload, "source", "Config file", "$profile:h-istasi/HST_Settings.json", "good");
		if (settings)
		{
			payload = AppendRow(payload, "source", "Default hideout", settings.m_Campaign.m_sDefaultHideoutId, "neutral");
			payload = AppendRow(payload, "source", "Membership", string.Format("enabled %1 / guests menu %2", settings.m_Membership.m_bMembershipEnabled, settings.m_Membership.m_bGuestsCanOpenMenu), "neutral");
			payload = AppendRow(payload, "source", "Area loot", string.Format("enabled %1 / radius %2m", settings.m_Features.m_bAreaLootEnabled, settings.m_ArsenalLoot.m_iLootRadiusMeters), "neutral");
		}

		return payload;
	}

	protected string AppendOverviewSections(string payload, HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return payload;

		payload = AppendSection(payload, "brief", "War Room");
		payload = AppendRow(payload, "brief", "Campaign", BuildPresetName(preset, state), "neutral");
		payload = AppendRow(payload, "brief", "Commander", BuildCommanderName(state), "neutral");
		payload = AppendRow(payload, "brief", "HQ hideout", BuildHQLabel(state), "good");
		payload = AppendRow(payload, "brief", "Petros", string.Format("%1 / deaths %2", BuildPetrosLabel(state), state.m_iPetrosDeaths), BuildPetrosTone(state));
		payload = AppendRow(payload, "brief", "Current order", BuildStrategicOrder(state, preset), "warn");

		payload = AppendSection(payload, "resources", "Resistance Logistics");
		payload = AppendRow(payload, "resources", "FIA money", string.Format("$%1", state.m_iFactionMoney), "good");
		payload = AppendRow(payload, "resources", "HR pool", string.Format("%1 recruits", state.m_iHR), "good");
		payload = AppendRow(payload, "resources", "Training", string.Format("level %1", state.m_iTrainingLevel), "good");
		payload = AppendRow(payload, "resources", "Income timer", string.Format("%1s accumulated", state.m_iIncomeAccumulatorSeconds), "neutral");

		payload = AppendSection(payload, "pressure", "Enemy Pressure");
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || (preset && pool.m_sFactionKey == preset.m_sResistanceFactionKey))
				continue;

			payload = AppendRow(payload, "pressure", pool.m_sFactionKey, string.Format("attack %1 / support %2 / aggression %3", pool.m_iAttackResources, pool.m_iSupportResources, pool.m_iAggression), "bad");
		}

		payload = AppendSection(payload, "active", "Active Front");
		payload = AppendRow(payload, "active", "Zones", string.Format("%1 FIA footholds / %2 hostile", CountResistanceZones(state, preset), CountEnemyZones(state, preset)), "neutral");
		payload = AppendRow(payload, "active", "Missions", string.Format("%1 active", CountActiveMissions(state)), "warn");
		payload = AppendRow(payload, "active", "QRFs", string.Format("%1 unresolved", CountActiveQRFs(state)), "bad");
		payload = AppendRow(payload, "active", "Arsenal unlocks", string.Format("%1 of %2", CountUnlockedArsenalItems(state), CountTrackedArsenalItems(state)), "good");
		payload = AppendRow(payload, "active", "Support calls", string.Format("%1 tracked", state.m_aSupportRequests.Count()), "warn");
		payload = AppendRow(payload, "active", "Civilian network", string.Format("%1 towns", state.m_aCivilianZones.Count()), "neutral");
		return payload;
	}

	protected string AppendHQSections(string payload, HST_CampaignState state, HST_RuntimeSettings settings, int playerId, bool canUseCommander)
	{
		if (!state)
			return payload;

		payload = AppendSection(payload, "petros", "Petros");
		payload = AppendRow(payload, "petros", "Status", BuildPetrosLabel(state), BuildPetrosTone(state));
		payload = AppendRow(payload, "petros", "Deaths", string.Format("%1", state.m_iPetrosDeaths), "warn");
		payload = AppendRow(payload, "petros", "Prefab", state.m_sPetrosPrefab, "neutral");
		payload = AppendRow(payload, "petros", "Authority", CommanderGateLabel(canUseCommander), CommanderGateTone(canUseCommander));

		payload = AppendSection(payload, "hq", "HQ Assets");
		payload = AppendRow(payload, "hq", "Hideout", BuildHQLabel(state), "good");
		payload = AppendRow(payload, "hq", "HQ position", string.Format("%1", state.m_vHQPosition), "neutral");
		payload = AppendRow(payload, "hq", "Petros position", string.Format("%1", state.m_vPetrosPosition), "neutral");
		payload = AppendRow(payload, "hq", "Arsenal position", string.Format("%1", state.m_vArsenalPosition), "neutral");
		payload = AppendRow(payload, "hq", "Cache position", string.Format("%1", state.m_vHQCachePosition), "neutral");
		payload = AppendRow(payload, "hq", "Arsenal prefab", state.m_sArsenalPrefab, "neutral");
		payload = AppendRow(payload, "hq", "Arsenal status", BuildArsenalRuntimeStatus(state), BuildArsenalRuntimeTone(state));
		if (!state.m_sLastHQArsenalFailure.IsEmpty())
			payload = AppendRow(payload, "hq", "Arsenal failure", state.m_sLastHQArsenalFailure, "bad");
		payload = AppendRow(payload, "hq", "Runtime objects", BuildRuntimeObjectLabel(state), BuildRuntimeObjectTone(state));
		payload = AppendRow(payload, "hq", "HQ radius", BuildHQRadiusStatus(state, settings, playerId), BuildHQRadiusTone(state, settings, playerId));
		payload = AppendRow(payload, "hq", "Enemy knowledge", string.Format("%1/100", state.m_iHQKnowledge), HQKnowledgeTone(state));
		payload = AppendRow(payload, "hq", "Build mode", BuildModeStatusLabel(state), BuildModeTone(state));

		payload = AppendSection(payload, "moves", "Move Base");
		payload = AppendRow(payload, "moves", "Move here", "Uses your current position as the new HQ.", CommanderGateTone(canUseCommander));
		payload = AppendRow(payload, "moves", "Rebuild assets", "Respawns Petros, cache, arsenal, and tent without resetting campaign state.", BuildHQRadiusTone(state, settings, playerId));
		payload = AppendRow(payload, "moves", "North Forest", "Low-profile woodland staging area.", "neutral");
		payload = AppendRow(payload, "moves", "Central Hills", "Default hideout near central roads.", "neutral");
		payload = AppendRow(payload, "moves", "South Woods", "Fallback hideout for southern operations.", "neutral");
		return payload;
	}

	protected string AppendMissionSections(string payload, HST_CampaignState state)
	{
		if (!state)
			return payload;

		payload = AppendSection(payload, "active_missions", "Active Missions");
		int activeMissionCount;
		foreach (HST_ActiveMissionState countMission : state.m_aActiveMissions)
		{
			if (countMission && countMission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE && !AreMissionObjectivesComplete(state, countMission))
				activeMissionCount++;
		}

		if (activeMissionCount == 0)
			payload = AppendRow(payload, "active_missions", "No active mission", "Use Start mission at Morton to create the first alpha task.", "neutral");

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			if (AreMissionObjectivesComplete(state, mission))
				continue;

			string missionTitle = BuildMissionDisplayTitle(mission);
			payload = AppendRow(payload, "active_missions", missionTitle, string.Format("%1 - %2", BuildMissionLocationLabel(state, mission), BuildMissionTimeLabel(mission)), MissionTone(mission));
			payload = AppendRow(payload, "active_missions", "Objective", BuildMissionInstruction(mission), "good");
			payload = AppendRow(payload, "active_missions", "Status", BuildMissionProgressText(state, mission), MissionTone(mission));
			payload = AppendRow(payload, "active_missions", "Next action", BuildMissionNextStepText(state, mission), "warn");
			if (!mission.m_sRuntimeFailureReason.IsEmpty())
				payload = AppendRow(payload, "active_missions", "Problem", mission.m_sRuntimeFailureReason, "bad");
		}

		payload = AppendSection(payload, "objectives", "Objective State");
		payload = AppendRow(payload, "objectives", "Active missions", string.Format("%1 visible", activeMissionCount), "warn");
		payload = AppendRow(payload, "objectives", "Objectives", string.Format("%1 active / %2 total", CountActiveMissionObjectives(state), state.m_aMissionObjectives.Count()), "warn");
		payload = AppendRow(payload, "objectives", "Tasks", string.Format("%1 campaign tasks", state.m_aCampaignTasks.Count()), "neutral");

		payload = AppendSection(payload, "families", "Mission Board");
		payload = AppendRow(payload, "families", "Assassination", "Officers, traitors, and spec-ops HVTs.", "warn");
		payload = AppendRow(payload, "families", "Conquest", "Towns, resources, factories, outposts, airfields, seaports.", "bad");
		payload = AppendRow(payload, "families", "Convoys", "Ammo, armor, money, prisoners, reinforcements, supplies.", "warn");
		payload = AppendRow(payload, "families", "Destroy/Logistics", "Radio, crash, armor, bank, salvage, ammo, weapons, support caches.", "good");
		payload = AppendRow(payload, "families", "Rescue/Dynamic/Support", "POWs, refugees, Petros defense, city fights, supplies.", "good");
		return payload;
	}

	protected string AppendMapSections(string payload, HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ZoneCaptureService capture)
	{
		if (!state)
			return payload;

		payload = AppendSection(payload, "war_map", "Map Control");
		payload = AppendRow(payload, "war_map", "Resistance", string.Format("%1 controlled zones", CountResistanceZones(state, preset)), "good");
		payload = AppendRow(payload, "war_map", "Enemy", string.Format("%1 hostile zones", CountEnemyZones(state, preset)), "bad");
		payload = AppendRow(payload, "war_map", "Active combat", string.Format("%1 zones physically active", CountActiveZones(state)), "warn");
		payload = AppendRow(payload, "war_map", "Income", string.Format("$%1 per tick from FIA zones", CountResistanceIncome(state, preset)), "good");
		payload = AppendRow(payload, "war_map", "Generated world", string.Format("%1 sites / %2 routes", state.m_aGeneratedSites.Count(), state.m_aGeneratedRoutes.Count()), "neutral");

		payload = AppendSection(payload, "capture_status", "Capture Status");
		if (!capture)
		{
			payload = AppendRow(payload, "capture_status", "Diagnostics", "Capture service unavailable.", "bad");
		}
		else
		{
			int captureRows;
			payload = AppendCaptureStatusRows(payload, state, preset, balance, capture, false, captureRows, 5);
			if (captureRows == 0)
				payload = AppendCaptureStatusRows(payload, state, preset, balance, capture, true, captureRows, 5);

			if (captureRows == 0)
				payload = AppendRow(payload, "capture_status", "No active capture", "Approach a hostile military zone to begin contesting it.", "neutral");
		}

		payload = AppendSection(payload, "zones", "Zone Pressure");
		int emitted;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			string label = ShortText(DisplayZoneName(zone.m_sZoneId), 22);
			string value = string.Format("%1 | %2 | support %3 | capture %4 | %5", zone.m_sOwnerFactionKey, ZoneTypeLabel(zone.m_eType), zone.m_iSupport, zone.m_iResistanceCaptureProgress, ActiveZoneLabel(zone.m_bActive));
			payload = AppendRow(payload, "zones", label, value, ZoneTone(zone, preset));
			emitted++;
			if (emitted >= 6)
				break;
		}

		return payload;
	}

	protected string AppendCaptureStatusRows(string payload, HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ZoneCaptureService capture, bool fallbackHighPriority, out int captureRows, int maxRows)
	{
		captureRows = 0;
		if (!state || !capture)
			return payload;

		foreach (HST_ZoneState captureZone : state.m_aZones)
		{
			if (!captureZone || captureRows >= maxRows)
				continue;

			HST_ZoneCaptureStatus status = capture.BuildCaptureStatus(state, preset, balance, captureZone);
			if (!status.m_bCapturable)
				continue;

			bool isActiveCapture = captureZone.m_bActive || captureZone.m_iResistanceCaptureProgress > 0 || status.m_iFIACountNearby > 0;
			if (!fallbackHighPriority && !isActiveCapture)
				continue;
			if (fallbackHighPriority && isActiveCapture)
				continue;
			if (fallbackHighPriority && captureZone.m_iPriority < 5)
				continue;

			payload = AppendRow(payload, "capture_status", ShortText(status.m_sZoneName, 18), BuildCompactCaptureStatusValue(status), CaptureStatusTone(status, preset));
			captureRows++;
		}

		return payload;
	}

	protected string BuildCompactCaptureStatusValue(HST_ZoneCaptureStatus status)
	{
		if (!status)
			return "status unavailable";

		string contested = "quiet";
		if (status.m_bContested)
			contested = "hot";

		string blockedReason = status.m_sBlockedReason;
		if (blockedReason.IsEmpty())
			blockedReason = "holding";
		blockedReason = CompactCaptureReason(blockedReason);

		return string.Format("%1 | %2 | radius %3m | FIA %4 | enemy %5 + %6 vehicles | progress %7 percent | %8", status.m_sOwnerFactionKey, contested, status.m_iCaptureRadiusMeters, status.m_iFIACountNearby, status.m_iEnemyCountNearby, status.m_iEnemyVehicleCountNearby, status.m_iProgressPercent, blockedReason);
	}

	protected string CompactCaptureReason(string reason)
	{
		if (reason == "no FIA in radius")
			return "noFIA";
		if (reason == "hostile vehicles remain")
			return "vehicles";
		if (reason == "enemies remain")
			return "enemies";
		if (reason == "conquest objective incomplete")
			return "objective";
		if (reason == "already FIA")
			return "FIA";
		if (reason == "not a military capture target")
			return "notTarget";

		return ShortText(reason, 12);
	}

	protected string AppendForcesSections(string payload, HST_CampaignState state, HST_CampaignPreset preset, bool canUseCommander)
	{
		if (!state)
			return payload;

		payload = AppendSection(payload, "fia", "FIA Forces");
		payload = AppendRow(payload, "fia", "Training level", string.Format("%1", state.m_iTrainingLevel), "good");
		payload = AppendRow(payload, "fia", "Available HR", string.Format("%1", state.m_iHR), "good");
		payload = AppendRow(payload, "fia", "Commander actions", CommanderGateLabel(canUseCommander), CommanderGateTone(canUseCommander));
		payload = AppendRow(payload, "fia", "Recruit focus", "Town support turns HR into abstract garrisons.", "neutral");

		payload = AppendSection(payload, "garrisons", "Garrisons And Patrols");
		payload = AppendRow(payload, "garrisons", "Abstract infantry", string.Format("%1", CountGarrisonInfantry(state)), "neutral");
		payload = AppendRow(payload, "garrisons", "Abstract vehicles", string.Format("%1", CountGarrisonVehicles(state)), "neutral");
		payload = AppendRow(payload, "garrisons", "Active groups", string.Format("%1", state.m_aActiveGroups.Count()), "warn");
		payload = AppendRow(payload, "garrisons", "Spawn diagnostics", BuildActiveGroupSpawnSummary(state), ActiveGroupSpawnTone(state));
		payload = AppendRow(payload, "garrisons", "Last spawn failure", BuildLastActiveGroupFailure(state), LastActiveGroupFailureTone(state));
		payload = AppendRow(payload, "garrisons", "QRFs", string.Format("%1 unresolved", CountActiveQRFs(state)), "bad");

		payload = AppendSection(payload, "enemy", "Enemy Resources");
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || (preset && pool.m_sFactionKey == preset.m_sResistanceFactionKey))
				continue;

			payload = AppendRow(payload, "enemy", pool.m_sFactionKey, string.Format("attack %1 / support %2 / aggression %3", pool.m_iAttackResources, pool.m_iSupportResources, pool.m_iAggression), "bad");
		}

		payload = AppendSection(payload, "support", "Support And Orders");
		payload = AppendRow(payload, "support", "Support requests", string.Format("%1 tracked", state.m_aSupportRequests.Count()), "warn");
		payload = AppendRow(payload, "support", "Enemy orders", string.Format("%1 tracked", state.m_aEnemyOrders.Count()), "bad");
		payload = AppendRow(payload, "support", "Cooldown/cancel", "Player support requests can be cancelled while queued or active.", "neutral");
		payload = AppendRow(payload, "support", "Air capability", AirSupportCapabilityLabel(state), AirSupportCapabilityTone(state));

		return payload;
	}

	protected string BuildActiveGroupSpawnSummary(HST_CampaignState state)
	{
		HST_ActiveGroupState activeGroup = FindLatestActiveGroup(state);
		if (!activeGroup)
			return "no active groups";

		return string.Format("%1 / %2 / agents %3 / status %4", activeGroup.m_sZoneId, activeGroup.m_sSpawnFallbackMode, activeGroup.m_iSpawnedAgentCount, activeGroup.m_sRuntimeStatus);
	}

	protected string ActiveGroupSpawnTone(HST_CampaignState state)
	{
		HST_ActiveGroupState activeGroup = FindLatestActiveGroup(state);
		if (!activeGroup)
			return "neutral";
		if (activeGroup.m_sRuntimeStatus == "spawn_failed")
			return "bad";

		return "good";
	}

	protected string BuildLastActiveGroupFailure(HST_CampaignState state)
	{
		if (!state)
			return "state not ready";

		for (int i = state.m_aActiveGroups.Count() - 1; i >= 0; i--)
		{
			HST_ActiveGroupState activeGroup = state.m_aActiveGroups[i];
			if (activeGroup && !activeGroup.m_sSpawnFailureReason.IsEmpty())
				return string.Format("%1: %2", activeGroup.m_sZoneId, activeGroup.m_sSpawnFailureReason);
		}

		return "none";
	}

	protected string EmptyLabel(string value)
	{
		if (value.IsEmpty())
			return "none";

		return value;
	}

	protected string LastActiveGroupFailureTone(HST_CampaignState state)
	{
		if (!state)
			return "bad";

		for (int i = state.m_aActiveGroups.Count() - 1; i >= 0; i--)
		{
			HST_ActiveGroupState activeGroup = state.m_aActiveGroups[i];
			if (activeGroup && !activeGroup.m_sSpawnFailureReason.IsEmpty())
				return "bad";
		}

		return "good";
	}

	protected HST_ActiveGroupState FindLatestActiveGroup(HST_CampaignState state)
	{
		if (!state || state.m_aActiveGroups.Count() == 0)
			return null;

		return state.m_aActiveGroups[state.m_aActiveGroups.Count() - 1];
	}

	protected string AppendArsenalSections(string payload, HST_CampaignState state, HST_RuntimeSettings settings, int playerId)
	{
		if (!state)
			return payload;

		payload = AppendSection(payload, "arsenal", "h-istasi Arsenal");
		payload = AppendRow(payload, "arsenal", "Tracked items", string.Format("%1", CountTrackedArsenalItems(state)), "neutral");
		payload = AppendRow(payload, "arsenal", "Unlocked items", string.Format("%1", CountUnlockedArsenalItems(state)), "good");
		payload = AppendRow(payload, "arsenal", "Garage vehicles", string.Format("%1", state.m_aGarageVehicles.Count()), "neutral");
		payload = AppendRow(payload, "arsenal", "Vehicle cargo", string.Format("%1 entries / %2 item(s)", CountVehicleCargoEntries(state), CountVehicleCargoItems(state)), "warn");
		payload = AppendRow(payload, "arsenal", "Captured emplacements", string.Format("%1", state.m_aCapturedEmplacements.Count()), "neutral");
		payload = AppendRow(payload, "arsenal", "Ammo points", string.Format("%1", state.m_aAmmoPoints.Count()), "neutral");
		if (settings)
		{
			payload = AppendRow(payload, "arsenal", "Loot radius", string.Format("%1m", settings.m_ArsenalLoot.m_iLootRadiusMeters), "neutral");
			payload = AppendRow(payload, "arsenal", "HQ action radius", string.Format("%1m", settings.m_ArsenalLoot.m_iHQInteractionRadiusMeters), "good");
			payload = AppendRow(payload, "arsenal", "HQ radius status", BuildHQRadiusStatus(state, settings, playerId), BuildHQRadiusTone(state, settings, playerId));
			payload = AppendRow(payload, "arsenal", "Vehicle loot", string.Format("%1 / rear %2m / max %3", settings.m_VehicleLoot.m_bEnabled, settings.m_VehicleLoot.m_iRadiusMeters, settings.m_VehicleLoot.m_iMaxItemsPerAction), "neutral");
			payload = AppendRow(payload, "arsenal", "Unlock threshold", string.Format("%1 / magazines x%2", settings.m_ArsenalLoot.m_iArsenalUnlockThreshold, settings.m_ArsenalLoot.m_iMagazineUnlockMultiplier), "neutral");
			payload = AppendRow(payload, "arsenal", "Loot rule", string.Format("locked only %1 / remove source %2", settings.m_ArsenalLoot.m_bLootOnlyLockedItems, settings.m_ArsenalLoot.m_bRemoveLootedItems), "warn");
		}

		payload = AppendRow(payload, "arsenal", "Vehicle target", BuildVehicleTargetStatus(state), BuildVehicleTargetTone(state));
		payload = AppendRow(payload, "arsenal", "Target reason", state.m_sLastVehicleTargetReason, BuildVehicleTargetTone(state));
		payload = AppendRow(payload, "arsenal", "Vehicle cargo target", string.Format("%1 entries", state.m_iLastVehicleTargetCargoEntries), "warn");

		payload = AppendSection(payload, "items", "Recovered Equipment");
		if (state.m_aArsenalItems.Count() == 0)
			payload = AppendRow(payload, "items", "Empty", "Loot nearby bodies, crates, and enemy inventories into the HQ arsenal.", "neutral");

		int emitted;
		foreach (HST_ArsenalItemState item : state.m_aArsenalItems)
		{
			if (!item)
				continue;

			string label = item.m_sDisplayName;
			label = HST_DisplayNameService.ResolveItemDisplayName(null, item.m_sPrefab, label);

			payload = AppendRow(payload, "items", label, string.Format("%1 / count %2", item.m_sCategory, ArsenalCountLabel(item)), ArsenalTone(item));
			emitted++;
			if (emitted >= 10)
				break;
		}

		payload = AppendSection(payload, "vehicle_cargo", "Vehicle Cargo");
		if (state.m_aVehicleCargoItems.Count() == 0)
			payload = AppendRow(payload, "vehicle_cargo", "Empty", "Collect nearby loot into a vehicle, then unload at HQ.", "neutral");

		int cargoEmitted;
		foreach (HST_VehicleCargoItemState cargoItem : state.m_aVehicleCargoItems)
		{
			if (!cargoItem)
				continue;

			string cargoItemLabel = HST_DisplayNameService.ResolveItemDisplayName(null, cargoItem.m_sItemPrefab, cargoItem.m_sDisplayName);
			string cargoVehicleLabel = HST_DisplayNameService.ResolveVehicleDisplayName(cargoItem.m_sVehiclePrefab, cargoItem.m_sVehicleDisplayName);
			payload = AppendRow(payload, "vehicle_cargo", cargoItemLabel, string.Format("%1 / %2 / count %3", cargoVehicleLabel, cargoItem.m_sCategory, cargoItem.m_iCount), "warn");
			cargoEmitted++;
			if (cargoEmitted >= 6)
				break;
		}

		return payload;
	}

	protected string AppendGarageSections(string payload, HST_CampaignState state, HST_RuntimeSettings settings, int playerId)
	{
		if (!state)
			return payload;

		payload = AppendSection(payload, "garage", "Virtual Garage");
		payload = AppendRow(payload, "garage", "Stored vehicles", string.Format("%1", state.m_aGarageVehicles.Count()), GarageTone(state));
		payload = AppendRow(payload, "garage", "Cargo entries", string.Format("%1 entries / %2 item(s)", CountVehicleCargoEntries(state), CountVehicleCargoItems(state)), "warn");
		payload = AppendRow(payload, "garage", "Captured emplacements", string.Format("%1", state.m_aCapturedEmplacements.Count()), "neutral");
		payload = AppendRow(payload, "garage", "Ammo points", string.Format("%1", state.m_aAmmoPoints.Count()), "neutral");
		payload = AppendRow(payload, "garage", "Active civilians", string.Format("%1 characters / %2 vehicles", state.m_iRuntimeCivilianCharacterCount, state.m_iRuntimeCivilianVehicleCount), RuntimeSpawnTone(state));
		payload = AppendRow(payload, "garage", "Active military vehicles", string.Format("%1 physical / %2 abstract", state.m_iRuntimeMilitaryVehicleCount, CountGarrisonVehicles(state)), "neutral");
		payload = AppendRow(payload, "garage", "Spawn failures", string.Format("%1", state.m_iRuntimeSpawnFailureCount), RuntimeSpawnTone(state));
		if (!state.m_sLastRuntimeSpawnFailurePrefab.IsEmpty())
			payload = AppendRow(payload, "garage", "Last failed prefab", state.m_sLastRuntimeSpawnFailurePrefab, "bad");
		if (settings)
		{
			payload = AppendRow(payload, "garage", "HQ action radius", string.Format("%1m", settings.m_ArsenalLoot.m_iHQInteractionRadiusMeters), "good");
			payload = AppendRow(payload, "garage", "HQ radius status", BuildHQRadiusStatus(state, settings, playerId), BuildHQRadiusTone(state, settings, playerId));
		}
		payload = AppendRow(payload, "garage", "Nearest vehicle", BuildVehicleTargetStatus(state), BuildVehicleTargetTone(state));
		payload = AppendRow(payload, "garage", "Vehicle reject", state.m_sLastVehicleTargetReason, BuildVehicleTargetTone(state));
		payload = AppendRow(payload, "garage", "Target cargo", string.Format("%1 entries", state.m_iLastVehicleTargetCargoEntries), "warn");
		payload = AppendRow(payload, "garage", "Build mode", BuildModeStatusLabel(state), BuildModeTone(state));
		payload = AppendRow(payload, "garage", "Build position", string.Format("%1 / yaw %2", state.m_vLastBuildModePosition, state.m_fLastBuildModeYaw), BuildModeTone(state));
		if (!state.m_sLastBuildModeFailure.IsEmpty())
			payload = AppendRow(payload, "garage", "Build failure", state.m_sLastBuildModeFailure, "bad");

		payload = AppendSection(payload, "stored_vehicles", "Stored Vehicles");
		if (state.m_aGarageVehicles.Count() == 0)
			payload = AppendRow(payload, "stored_vehicles", "Empty", "Capture a nearby top-level vehicle to virtualize it.", "neutral");

		int emitted;
		foreach (HST_GarageVehicleState vehicle : state.m_aGarageVehicles)
		{
			if (!vehicle)
				continue;

			payload = AppendRow(payload, "stored_vehicles", GarageVehicleLabel(vehicle), GarageVehicleValue(vehicle), GarageVehicleTone(vehicle));
			emitted++;
			if (emitted >= 10)
				break;
		}

		payload = AppendSection(payload, "garage_actions", "Capture And Redeploy");
		payload = AppendRow(payload, "garage_actions", "Capture nearest", "Stores a safe root vehicle and despawns only that vehicle.", "good");
		payload = AppendRow(payload, "garage_actions", "Build redeploy", "Each stored vehicle resolves a Build Mode placement before spawning.", GarageTone(state));
		payload = AppendRow(payload, "garage_actions", "Vehicle target", string.Format("Candidates %1 / %2", state.m_iLastVehicleTargetCandidates, state.m_sLastVehicleTargetReason), BuildVehicleTargetTone(state));
		payload = AppendRow(payload, "garage_actions", "Cargo unload", "Nearest vehicle cargo can be moved into the h-istasi arsenal at HQ.", "warn");

		return payload;
	}

	protected string AppendMembersSections(string payload, HST_CampaignState state, bool canUseCommander)
	{
		if (!state)
			return payload;

		payload = AppendSection(payload, "roster", "Resistance Roster");
		payload = AppendRow(payload, "roster", "Known players", string.Format("%1", state.m_aPlayers.Count()), "neutral");
		payload = AppendRow(payload, "roster", "Commander", BuildCommanderName(state), "good");
		payload = AppendRow(payload, "roster", "Your authority", CommanderGateLabel(canUseCommander), CommanderGateTone(canUseCommander));

		payload = AppendSection(payload, "members", "Members");
		if (state.m_aPlayers.Count() == 0)
			payload = AppendRow(payload, "members", "No players registered", "The first connected player becomes the initial commander.", "neutral");

		foreach (HST_PlayerState player : state.m_aPlayers)
		{
			if (!player)
				continue;

			string role = "guest";
			string tone = "bad";
			if (player.m_bAdmin)
			{
				role = "admin";
				tone = "warn";
			}
			else if (player.m_bMember)
			{
				role = "member";
				tone = "good";
			}

			string suffix = "";
			if (state.m_sCommanderIdentityId == player.m_sIdentityId)
				suffix = " / commander";

			payload = AppendRow(payload, "members", player.m_sIdentityId, string.Format("%1 / money %2 / spawns %3%4", role, player.m_iMoney, player.m_iSpawnCount, suffix), tone);
		}

		return payload;
	}

	protected string AppendAdminSections(string payload, HST_CampaignState state, HST_CampaignPreset preset, bool canUseAdmin, HST_ZoneCompositionService compositions = null)
	{
		payload = AppendSection(payload, "admin", "Admin Console");
		payload = AppendRow(payload, "admin", "Access", CommanderGateLabel(canUseAdmin), CommanderGateTone(canUseAdmin));
		if (!state)
			return payload;

		payload = AppendRow(payload, "admin", "Phase", CampaignPhaseLabel(state.m_ePhase), "neutral");
		payload = AppendRow(payload, "admin", "Schema", string.Format("%1", state.m_iSchemaVersion), "neutral");
		payload = AppendRow(payload, "admin", "Seed", string.Format("%1", state.m_iCampaignSeed), "neutral");
		payload = AppendRow(payload, "admin", "Enemy accumulator", string.Format("%1s", state.m_iEnemyResourceAccumulatorSeconds), "bad");
		payload = AppendRow(payload, "admin", "Native markers", string.Format("%1 tracked", state.m_aMapMarkers.Count()), "neutral");
		if (compositions)
		{
			payload = AppendRow(payload, "admin", "Composition zones", string.Format("%1 active / %2 props", compositions.GetActiveRuntimeZoneCount(), compositions.GetRuntimePropCount()), "neutral");
			payload = AppendRow(payload, "admin", "Composition spawned", string.Format("%1", compositions.GetLastSpawnedCount()), "good");
			payload = AppendRow(payload, "admin", "Skipped prefabs", string.Format("%1", compositions.GetLastSkippedPrefabCount()), RuntimeSpawnTone(state));
			payload = AppendRow(payload, "admin", "Last failed prefab", EmptyLabel(compositions.GetLastFailedPrefab()), "bad");
			payload = AppendRow(payload, "admin", "Last slot reason", EmptyLabel(compositions.GetLastFailedSlotReason()), "warn");
		}

		payload = AppendSection(payload, "debug_zone", "Debug Targets");
		HST_ZoneState morton = state.FindZone("town_morton");
		if (morton)
			payload = AppendRow(payload, "debug_zone", "Morton", string.Format("owner %1 / support %2 / capture %3", morton.m_sOwnerFactionKey, morton.m_iSupport, morton.m_iResistanceCaptureProgress), ZoneTone(morton, preset));

		return payload;
	}

	protected string AppendActivityFeed(string payload, HST_CampaignState state, HST_CampaignPreset preset, string selectedTabId)
	{
		if (!state)
		{
			payload = AppendFeed(payload, "Campaign state is not ready yet.", "bad");
			return payload;
		}

		payload = AppendFeed(payload, string.Format("Briefing: %1", BuildStrategicOrder(state, preset)), "warn");
		payload = AppendFeed(payload, string.Format("HQ: %1, Petros %2.", BuildHQLabel(state), BuildPetrosLabel(state)), BuildPetrosTone(state));
		payload = AppendFeed(payload, string.Format("Arsenal: %1 unlocked items from %2 tracked entries.", CountUnlockedArsenalItems(state), CountTrackedArsenalItems(state)), "good");
		payload = AppendFeed(payload, string.Format("Operations: %1 active missions, %2 QRFs, %3 active zones.", CountActiveMissions(state), CountActiveQRFs(state), CountActiveZones(state)), "neutral");
		payload = AppendFeed(payload, string.Format("Alpha systems: %1 generated sites, %2 support calls, %3 enemy orders.", state.m_aGeneratedSites.Count(), state.m_aSupportRequests.Count(), state.m_aEnemyOrders.Count()), "warn");
		return payload;
	}

	protected void BuildTabActions(HST_CampaignState state, HST_CampaignPreset preset, string selectedTabId, int playerId, notnull array<ref HST_CommandMenuAction> actions, bool canUseMember, bool canUseCommander, bool canUseAdmin)
	{
		actions.Clear();
		selectedTabId = NormalizeTabId(selectedTabId);
		string primaryTargetId = SelectPriorityMissionZoneId(state, preset);
		string hostileTownId = SelectFirstZoneIdByType(state, preset, HST_EZoneType.HST_ZONE_TOWN, true);
		string resourceTargetId = SelectFirstZoneIdByType(state, preset, HST_EZoneType.HST_ZONE_RESOURCE, true);
		string outpostTargetId = SelectFirstZoneIdByType(state, preset, HST_EZoneType.HST_ZONE_OUTPOST, true);
		string recruitTargetId = SelectRecruitZoneId(state, preset, playerId);
		string adminTargetId = SelectAdminTargetZoneId(state);
		string guestIdentityId = SelectFirstGuestIdentity(state);
		string memberIdentityId = SelectFirstMemberIdentity(state);
		bool airSupportReady = HasResistanceAirSupportCapability(state);
		string firstGarageVehicleId = SelectFirstGarageVehicleId(state);
		if (selectedTabId == TAB_SETUP)
		{
			AddMenuAction(actions, TAB_SETUP, "Start HQ: north forest", "setup_hideout", "hideout_north_forest", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_SETUP, "Start HQ: central hills", "setup_hideout", "hideout_central_hills", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_SETUP, "Start HQ: south woods", "setup_hideout", "hideout_south_woods", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_SETUP, "Persistence status", "inspect_persistence", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_OVERVIEW)
		{
			AddMenuAction(actions, TAB_OVERVIEW, "Foundation status", "foundation_status", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_OVERVIEW, "Campaign overview", "inspect_campaign", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_OVERVIEW, "Marker status", "inspect_markers", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_OVERVIEW, "Manual checkpoint", "checkpoint", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_PETROS)
		{
			AddMenuAction(actions, TAB_PETROS, "Move base to my position", "move_hq_here", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_PETROS, "Move HQ: north forest", "move_hq", "hideout_north_forest", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_PETROS, "Move HQ: central hills", "move_hq", "hideout_central_hills", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_PETROS, "Move HQ: south woods", "move_hq", "hideout_south_woods", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_PETROS, "Rebuild HQ assets", "rebuild_hq_assets", "", canUseCommander, "commander required");
			return;
		}

		if (selectedTabId == TAB_MISSIONS)
		{
			AddMenuAction(actions, TAB_MISSIONS, "Inspect Active Missions", "inspect_active_missions", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_MISSIONS, "Convoy Runtime Report", "inspect_convoy_runtime", "", canUseMember, "membership required");
			AddMissionInspectActions(state, actions, canUseMember, "membership required");
			AddMenuAction(actions, TAB_MISSIONS, "Start priority mission", "mission_random", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_MISSIONS, BuildZoneActionLabel("Start mission", state, primaryTargetId), "mission_zone", primaryTargetId, canUseCommander && !primaryTargetId.IsEmpty(), "no valid target");
			AddMenuAction(actions, TAB_MISSIONS, BuildZoneActionLabel("Start town mission", state, hostileTownId), "mission_zone", hostileTownId, canUseCommander && !hostileTownId.IsEmpty(), "no hostile town");
			AddMenuAction(actions, TAB_MISSIONS, BuildZoneActionLabel("Start resource mission", state, resourceTargetId), "mission_zone", resourceTargetId, canUseCommander && !resourceTargetId.IsEmpty(), "no hostile resource");
			AddMenuAction(actions, TAB_MISSIONS, BuildZoneActionLabel("Start outpost mission", state, outpostTargetId), "mission_zone", outpostTargetId, canUseCommander && !outpostTargetId.IsEmpty(), "no hostile outpost");
			AddMissionNextStepActions(state, actions, canUseMember, "membership required");
			AddMenuAction(actions, TAB_MISSIONS, "Load nearest mission cargo", "mission_asset_load", "", canUseMember && HasActiveMissionAssets(state), "no active mission asset");
			AddMenuAction(actions, TAB_MISSIONS, "Unload carried mission cargo", "mission_asset_unload", "", canUseMember && HasActiveMissionAssets(state), "no carried mission cargo");
			AddMenuAction(actions, TAB_MISSIONS, "Deliver mission cargo/captive", "mission_asset_deliver", "", canUseMember && HasActiveMissionAssets(state), "no deliverable mission asset");
			AddMenuAction(actions, TAB_MISSIONS, "Extract nearest captive", "mission_captive_extract", "", canUseMember && HasActiveMissionAssets(state), "no active captive");
			AddMenuAction(actions, TAB_MISSIONS, "Capture mission vehicle", "mission_vehicle_capture", "", canUseMember && HasActiveMissionAssets(state), "no mission vehicle");
			AddMenuAction(actions, TAB_MISSIONS, "Sabotage mission target", "mission_asset_sabotage", "", canUseMember && HasActiveMissionAssets(state), "no mission target");
			return;
		}

		if (selectedTabId == TAB_MAP)
		{
			AddMenuAction(actions, TAB_MAP, "Marker status", "inspect_markers", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_MAP, "Generated content report", "inspect_content", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_FORCES)
		{
			AddMenuAction(actions, TAB_FORCES, "Train FIA troops", "train_troops", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_FORCES, BuildZoneActionLabel("Recruit FIA", state, recruitTargetId), "recruit_zone", recruitTargetId, canUseCommander && !recruitTargetId.IsEmpty(), "no recruit target");
			AddMenuAction(actions, TAB_FORCES, "Request supply drop", "call_supply", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_FORCES, "Request QRF reserve", "support_qrf", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_FORCES, "Request suppressive fire", "support_fire", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_FORCES, "Request GBU air strike", "support_gbu", "", canUseCommander && airSupportReady, AirSupportDisabledReason(canUseCommander, airSupportReady));
			AddMenuAction(actions, TAB_FORCES, "Request UMPK air strike", "support_umpk", "", canUseCommander && airSupportReady, AirSupportDisabledReason(canUseCommander, airSupportReady));
			AddMenuAction(actions, TAB_FORCES, "Cancel player support", "cancel_support", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_FORCES, "Deliver civilian aid", "civilian_aid", "", canUseCommander, "commander required");
			return;
		}

		if (selectedTabId == TAB_ARSENAL)
		{
			AddMenuAction(actions, TAB_ARSENAL, "Loot nearby to arsenal", "loot_nearby", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_ARSENAL, "Load loot to vehicle", "vehicle_collect_loot", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_ARSENAL, "Unload vehicle loot to arsenal", "vehicle_unload_loot", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_GARAGE)
		{
			AddMenuAction(actions, TAB_GARAGE, "Capture nearest vehicle", "garage_capture_nearby", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_GARAGE, "Unload vehicle loot to arsenal", "vehicle_unload_loot", "", canUseMember, "membership required");

			int garageActionCount;
			if (state)
			{
				foreach (HST_GarageVehicleState vehicle : state.m_aGarageVehicles)
				{
					if (!vehicle)
						continue;

					AddMenuAction(actions, TAB_GARAGE, BuildGarageRedeployActionLabel(vehicle), "garage_redeploy", vehicle.m_sVehicleId, canUseMember, "membership required");
					garageActionCount++;
					if (garageActionCount >= 5)
						break;
				}
			}

			if (garageActionCount == 0)
				AddMenuAction(actions, TAB_GARAGE, "Redeploy stored vehicle", "garage_redeploy", firstGarageVehicleId, false, "no stored vehicle");

			return;
		}

		if (selectedTabId == TAB_MEMBERS)
		{
			AddMenuAction(actions, TAB_MEMBERS, "Accept first guest", "member_accept", guestIdentityId, canUseAdmin && !guestIdentityId.IsEmpty(), "admin required or no guest");
			AddMenuAction(actions, TAB_MEMBERS, "Remove first member", "member_remove", memberIdentityId, canUseAdmin && !memberIdentityId.IsEmpty(), "admin required or no member");
			AddMenuAction(actions, TAB_MEMBERS, "Grant admin to member", "admin_grant", memberIdentityId, canUseAdmin && !memberIdentityId.IsEmpty(), "admin required or no member");
			AddMenuAction(actions, TAB_MEMBERS, "Undercover status", "inspect_undercover", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_ADMIN)
		{
			AddMenuAction(actions, TAB_ADMIN, "Force: Ammo convoy", "debug_mission_id", "convoy_ammo", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Force: Supply convoy", "debug_mission_id", "convoy_supplies", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Force: POW rescue", "debug_mission_id", "rescue_pows", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Force: Kill officer", "debug_mission_id", "assassinate_officer", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Force: Conquest outpost", "debug_mission_id", "conquest_outpost", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Force: Radio tower", "debug_mission_id", "destroy_radio_tower", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Force: City supplies", "debug_mission_id", "support_city_supplies", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Force: Resource cache", "debug_mission_id", "logistics_resource_cache", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Force: Minor city task", "debug_mission_id", "dynamic_minor_city_task", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Seed persistence smoke", "admin_seed_persistence_test_state", "", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Run persistence smoke", "admin_persistence_smoke_test", "", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Persistence smoke report", "admin_persistence_smoke_report", "", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Persistence status", "inspect_persistence", "", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Manual checkpoint", "checkpoint", "", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Zone composition report", "inspect_zone_composition", "", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, BuildZoneActionLabel("Debug capture", state, adminTargetId), "capture_zone", adminTargetId, canUseAdmin && !adminTargetId.IsEmpty(), "no zone");
			AddMenuAction(actions, TAB_ADMIN, BuildZoneActionLabel("Debug activate", state, adminTargetId), "activate_zone", adminTargetId, canUseAdmin && !adminTargetId.IsEmpty(), "no zone");
			AddMenuAction(actions, TAB_ADMIN, BuildZoneActionLabel("Debug deactivate", state, adminTargetId), "deactivate_zone", adminTargetId, canUseAdmin && !adminTargetId.IsEmpty(), "no zone");
			AddMenuAction(actions, TAB_ADMIN, BuildZoneActionLabel("Debug mission", state, adminTargetId), "debug_mission", adminTargetId, canUseAdmin && !adminTargetId.IsEmpty(), "no zone");
			AddMenuAction(actions, TAB_ADMIN, "Debug award resources", "award_small", "", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Force income tick", "income_now", "", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Force mission progress", "progress_mission", "", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Rebuild HQ assets", "rebuild_hq_assets", "", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Mission runtime report", "inspect_mission_runtime", "", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Vehicle cargo report", "inspect_vehicle_cargo", "", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Garage report", "inspect_garage", "", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Reset campaign", "new_campaign", "", canUseAdmin, "admin required");
		}
	}

	protected string AppendStat(string payload, string label, string value, string tone)
	{
		return payload + string.Format("\nSTAT|%1|%2|%3", label, value, tone);
	}

	protected string AppendSection(string payload, string sectionId, string title)
	{
		return payload + string.Format("\nSECTION|%1|%2", sectionId, title);
	}

	protected string AppendRow(string payload, string sectionId, string label, string value, string tone)
	{
		return payload + string.Format("\nROW|%1|%2|%3|%4", sectionId, label, value, tone);
	}

	protected string AppendFeed(string payload, string text, string tone)
	{
		return payload + string.Format("\nFEED|%1|%2", text, tone);
	}

	protected void AddMenuAction(notnull array<ref HST_CommandMenuAction> actions, string tabId, string label, string commandId, string argument, bool enabled, string disabledReason)
	{
		HST_CommandMenuAction action = new HST_CommandMenuAction();
		action.m_sTabId = tabId;
		action.m_sLabel = label;
		action.m_sCommandId = commandId;
		action.m_sArgument = argument;
		action.m_bEnabled = enabled;
		action.m_sDisabledReason = disabledReason;
		actions.Insert(action);
	}

	protected string SelectPriorityMissionZoneId(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return "";

		HST_ZoneState bestZone;
		int bestScore = -99999;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || (preset && zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey))
				continue;

			int score = zone.m_iPriority + zone.m_iIncomeValue / 5 + zone.m_iResistanceCaptureProgress;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE || zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
				score += 20;
			if (zone.m_bActive)
				score += 15;

			if (score > bestScore)
			{
				bestZone = zone;
				bestScore = score;
			}
		}

		if (bestZone)
			return bestZone.m_sZoneId;

		return "";
	}

	protected string SelectFirstZoneIdByType(HST_CampaignState state, HST_CampaignPreset preset, HST_EZoneType zoneType, bool hostileOnly)
	{
		if (!state)
			return "";

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_eType != zoneType)
				continue;

			if (hostileOnly && preset && zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
				continue;

			return zone.m_sZoneId;
		}

		return "";
	}

	protected string SelectRecruitZoneId(HST_CampaignState state, HST_CampaignPreset preset, int playerId)
	{
		if (!state || !preset)
			return "";

		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		vector referencePosition = state.m_vHQPosition;
		if (playerEntity)
			referencePosition = playerEntity.GetOrigin();

		HST_ZoneState bestZone;
		float bestDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!IsRecruitableResistanceZone(state, preset, zone))
				continue;

			float distanceSq = DistanceSq2D(referencePosition, zone.m_vPosition);
			if (distanceSq < bestDistanceSq)
			{
				bestDistanceSq = distanceSq;
				bestZone = zone;
			}
		}

		if (bestZone)
			return bestZone.m_sZoneId;

		foreach (HST_ZoneState fallbackZone : state.m_aZones)
		{
			if (fallbackZone && fallbackZone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
				return fallbackZone.m_sZoneId;
		}

		return "";
	}

	protected bool IsRecruitableResistanceZone(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState zone)
	{
		if (!state || !preset || !zone)
			return false;

		if (zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
			return false;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT)
			return false;

		if (zone.m_iGarrisonSlots <= 0)
			return true;

		int currentInfantry;
		HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, preset.m_sResistanceFactionKey);
		if (garrison)
			currentInfantry = garrison.m_iInfantryCount;
		currentInfantry += Math.Max(0, zone.m_iActiveInfantryCount);
		return currentInfantry < zone.m_iGarrisonSlots;
	}

	protected string SelectAdminTargetZoneId(HST_CampaignState state)
	{
		if (!state || state.m_aZones.Count() == 0)
			return "";

		return state.m_aZones[0].m_sZoneId;
	}

	protected string SelectFirstGuestIdentity(HST_CampaignState state)
	{
		if (!state)
			return "";

		foreach (HST_PlayerState player : state.m_aPlayers)
		{
			if (player && player.m_bGuest && !player.m_sIdentityId.IsEmpty())
				return player.m_sIdentityId;
		}

		return "";
	}

	protected string SelectFirstMemberIdentity(HST_CampaignState state)
	{
		if (!state)
			return "";

		foreach (HST_PlayerState player : state.m_aPlayers)
		{
			if (player && player.m_bMember && !player.m_bAdmin && !player.m_sIdentityId.IsEmpty())
				return player.m_sIdentityId;
		}

		foreach (HST_PlayerState fallbackPlayer : state.m_aPlayers)
		{
			if (fallbackPlayer && fallbackPlayer.m_bMember && !fallbackPlayer.m_sIdentityId.IsEmpty())
				return fallbackPlayer.m_sIdentityId;
		}

		return "";
	}

	protected string SelectFirstGarageVehicleId(HST_CampaignState state)
	{
		if (!state)
			return "";

		foreach (HST_GarageVehicleState vehicle : state.m_aGarageVehicles)
		{
			if (vehicle && !vehicle.m_sVehicleId.IsEmpty())
				return vehicle.m_sVehicleId;
		}

		return "";
	}

	protected string SelectFirstSavedLoadoutId(HST_CampaignState state, int playerId)
	{
		if (!state)
			return "";

		string identityId = ResolveMenuIdentityId(state, playerId);
		foreach (HST_SavedLoadoutState loadout : state.m_aSavedLoadouts)
		{
			if (loadout && loadout.m_sOwnerIdentityId == identityId && !loadout.m_sLoadoutId.IsEmpty())
				return loadout.m_sLoadoutId;
		}

		return "";
	}

	protected string ResolveMenuIdentityId(HST_CampaignState state, int playerId)
	{
		if (state)
		{
			foreach (HST_PlayerState player : state.m_aPlayers)
			{
				if (player && player.m_iLastSeenPlayerId == playerId && !player.m_sIdentityId.IsEmpty())
					return player.m_sIdentityId;
			}
		}

		return string.Format("workbench_player_%1", playerId);
	}

	protected int CountPlayerSavedLoadouts(HST_CampaignState state, int playerId)
	{
		if (!state)
			return 0;

		string identityId = ResolveMenuIdentityId(state, playerId);
		int count;
		foreach (HST_SavedLoadoutState loadout : state.m_aSavedLoadouts)
		{
			if (loadout && loadout.m_sOwnerIdentityId == identityId)
				count++;
		}

		return count;
	}

	protected int CountPlayerIssuedFiniteItems(HST_CampaignState state, int playerId)
	{
		if (!state)
			return 0;

		string identityId = ResolveMenuIdentityId(state, playerId);
		int count;
		foreach (HST_IssuedLoadoutItemState issuedItem : state.m_aIssuedLoadoutItems)
		{
			if (issuedItem && issuedItem.m_sOwnerIdentityId == identityId && !issuedItem.m_bInfinite)
				count += issuedItem.m_iCount;
		}

		return count;
	}

	protected int CountPlayerIssuedInfiniteItems(HST_CampaignState state, int playerId)
	{
		if (!state)
			return 0;

		string identityId = ResolveMenuIdentityId(state, playerId);
		int count;
		foreach (HST_IssuedLoadoutItemState issuedItem : state.m_aIssuedLoadoutItems)
		{
			if (issuedItem && issuedItem.m_sOwnerIdentityId == identityId && issuedItem.m_bInfinite)
				count += issuedItem.m_iCount;
		}

		return count;
	}

	protected string BuildLoadoutEditorStatus(HST_CampaignState state, int playerId)
	{
		if (!state)
			return "unknown";

		string identityId = ResolveMenuIdentityId(state, playerId);
		HST_LoadoutEditorSessionState session = state.FindLoadoutEditorSession(identityId);
		if (!session)
			return state.m_sLoadoutEditorStatus;

		return string.Format("%1 | preview %2 | current %3", session.m_sStatus, session.m_bPreviewSpawned, session.m_sCurrentLoadoutId);
	}

	protected string BuildLoadoutEditorTone(HST_CampaignState state)
	{
		if (!state)
			return "warn";

		if (!state.m_sLastLoadoutEditorFailure.IsEmpty())
			return "bad";

		if (state.m_sLoadoutEditorStatus.Contains("open") || state.m_sLoadoutEditorStatus.Contains("saved") || state.m_sLoadoutEditorStatus.Contains("applied"))
			return "good";

		return "neutral";
	}

	protected string BuildGarageRedeployActionLabel(HST_GarageVehicleState vehicle)
	{
		if (!vehicle)
			return "Redeploy stored vehicle";

		return "Redeploy: " + ShortGarageVehicleLabel(vehicle, 22);
	}

	protected string GarageVehicleLabel(HST_GarageVehicleState vehicle)
	{
		if (!vehicle)
			return "unknown";

		string label = HST_DisplayNameService.ResolveVehicleDisplayName(vehicle.m_sPrefab, vehicle.m_sDisplayName);
		if (!label.IsEmpty() && label != "vehicle")
			return ShortGarageText(label, 28);

		return vehicle.m_sVehicleId;
	}

	protected string GarageVehicleValue(HST_GarageVehicleState vehicle)
	{
		if (!vehicle)
			return "missing";

		return string.Format("%1 | cost $%2 | fuel %3 | armed %4", vehicle.m_sVehicleId, vehicle.m_iRedeployCost, vehicle.m_fFuel, vehicle.m_bArmed);
	}

	protected string GarageVehicleTone(HST_GarageVehicleState vehicle)
	{
		if (!vehicle)
			return "bad";

		if (vehicle.m_bArmed)
			return "warn";

		return "good";
	}

	protected string GarageTone(HST_CampaignState state)
	{
		if (state && state.m_aGarageVehicles.Count() > 0)
			return "good";

		return "warn";
	}

	protected string ShortGarageVehicleLabel(HST_GarageVehicleState vehicle, int maxCharacters)
	{
		if (!vehicle)
			return "vehicle";

		string label = vehicle.m_sDisplayName;
		label = HST_DisplayNameService.ResolveVehicleDisplayName(vehicle.m_sPrefab, label);

		if (label.IsEmpty())
			label = vehicle.m_sVehicleId;

		return ShortGarageText(label, maxCharacters);
	}

	protected string ShortGarageText(string text, int maxCharacters)
	{
		if (text.Length() <= maxCharacters)
			return text;

		if (maxCharacters <= 3)
			return text.Substring(0, maxCharacters);

		return text.Substring(0, maxCharacters - 3) + "...";
	}

	protected string BuildZoneActionLabel(string prefix, HST_CampaignState state, string zoneId)
	{
		if (zoneId.IsEmpty())
			return prefix + ": no target";

		if (!state)
			return prefix + ": " + zoneId;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (zone)
			return prefix + ": " + DisplayZoneName(zone);

		return prefix + ": " + zoneId;
	}

	protected string DisplayZoneName(HST_ZoneState zone)
	{
		if (!zone)
			return "unknown";

		if (!zone.m_sDisplayName.IsEmpty())
			return zone.m_sDisplayName;

		return HST_DefaultCatalog.GetZoneDisplayName(zone.m_sZoneId);
	}

	protected string BuildBoolResult(string label, bool success)
	{
		if (success)
			return "h-istasi command | " + label + " | complete";

		return "h-istasi command | " + label + " | denied or failed";
	}

	protected string BuildPresetName(HST_CampaignPreset preset, HST_CampaignState state)
	{
		if (preset && !preset.m_sDisplayName.IsEmpty())
			return preset.m_sDisplayName;

		if (state)
			return state.m_sPresetId;

		return "unknown";
	}

	protected string BuildCommanderName(HST_CampaignState state)
	{
		if (!state || state.m_sCommanderIdentityId.IsEmpty())
			return "unassigned";

		return state.m_sCommanderIdentityId;
	}

	protected string BuildHQLabel(HST_CampaignState state)
	{
		if (!state || !state.m_bHQDeployed)
			return "not deployed";

		if (state.m_sHQHideoutId.IsEmpty())
			return "field HQ";

		return DisplayZoneName(state.m_sHQHideoutId);
	}

	protected string BuildPetrosLabel(HST_CampaignState state)
	{
		if (!state)
			return "unknown";

		if (state.m_bPetrosAlive)
			return "alive";

		return "killed";
	}

	protected string BuildPetrosTone(HST_CampaignState state)
	{
		if (state && state.m_bPetrosAlive)
			return "good";

		return "bad";
	}

	protected string BuildRuntimeObjectLabel(HST_CampaignState state)
	{
		if (!state)
			return "unknown";

		if (state.m_bHQRuntimeObjectsSpawned)
			return "spawned";

		return "pending";
	}

	protected string BuildRuntimeObjectTone(HST_CampaignState state)
	{
		if (state && state.m_bHQRuntimeObjectsSpawned)
			return "good";

		return "warn";
	}

	protected string BuildArsenalRuntimeStatus(HST_CampaignState state)
	{
		if (!state)
			return "unknown";

		if (!state.m_sHQArsenalRuntimeStatus.IsEmpty())
			return state.m_sHQArsenalRuntimeStatus;

		if (state.m_bHQRuntimeObjectsSpawned)
			return "ready";

		return "pending";
	}

	protected string BuildArsenalRuntimeTone(HST_CampaignState state)
	{
		if (!state)
			return "warn";

		if (!state.m_sLastHQArsenalFailure.IsEmpty())
			return "bad";

		if (state.m_sHQArsenalRuntimeStatus.Contains("ready"))
			return "good";

		if (state.m_sHQArsenalRuntimeStatus.Contains("failed"))
			return "bad";

		return "warn";
	}

	protected string BuildVehicleTargetStatus(HST_CampaignState state)
	{
		if (!state)
			return "unknown";

		string target = state.m_sLastVehicleTargetStatus;
		if (target.IsEmpty())
			target = "not scanned";

		if (!state.m_sLastVehicleTargetPrefab.IsEmpty())
			target = target + string.Format(" | %1 | %2m", HST_DisplayNameService.ResolveVehicleDisplayName(state.m_sLastVehicleTargetPrefab), state.m_fLastVehicleTargetDistanceMeters);

		return target;
	}

	protected string BuildVehicleTargetTone(HST_CampaignState state)
	{
		if (!state)
			return "warn";

		if (!state.m_sLastVehicleTargetPrefab.IsEmpty())
			return "good";

		if (state.m_iLastVehicleTargetCandidates > 0)
			return "bad";

		return "neutral";
	}

	protected string BuildModeStatusLabel(HST_CampaignState state)
	{
		if (!state)
			return "unknown";

		if (!state.m_sBuildModeStatus.IsEmpty())
			return state.m_sBuildModeStatus;

		return "not active";
	}

	protected string BuildModeTone(HST_CampaignState state)
	{
		if (!state)
			return "warn";

		if (!state.m_sLastBuildModeFailure.IsEmpty())
			return "bad";

		if (state.m_sBuildModeStatus.Contains("ready"))
			return "good";

		return "neutral";
	}

	protected string BuildHQRadiusStatus(HST_CampaignState state, HST_RuntimeSettings settings, int playerId)
	{
		if (!state || !state.m_bHQDeployed)
			return "HQ not deployed";

		int radius = ResolveHQRadiusMeters(settings);
		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		if (!playerEntity)
			return string.Format("no controlled player entity / radius %1m", radius);

		float distance = Math.Sqrt(DistanceSq2D(playerEntity.GetOrigin(), state.m_vHQPosition));
		if (distance <= radius)
			return string.Format("inside HQ radius | %1m / %2m", Math.Round(distance), radius);

		return string.Format("outside HQ radius | %1m / %2m", Math.Round(distance), radius);
	}

	protected string BuildHQRadiusTone(HST_CampaignState state, HST_RuntimeSettings settings, int playerId)
	{
		if (!state || !state.m_bHQDeployed)
			return "warn";

		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		if (!playerEntity)
			return "warn";

		int radius = ResolveHQRadiusMeters(settings);
		if (DistanceSq2D(playerEntity.GetOrigin(), state.m_vHQPosition) <= radius * radius)
			return "good";

		return "bad";
	}

	protected string HQKnowledgeTone(HST_CampaignState state)
	{
		if (!state)
			return "neutral";
		if (state.m_iHQKnowledge >= 80)
			return "bad";
		if (state.m_iHQKnowledge >= 40)
			return "warn";
		return "good";
	}

	protected int ResolveHQRadiusMeters(HST_RuntimeSettings settings)
	{
		if (settings)
			return Math.Max(1, settings.m_ArsenalLoot.m_iHQInteractionRadiusMeters);

		return 50;
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

	protected float DistanceSq2D(vector first, vector second)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz;
	}

	protected string BuildStrategicOrder(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return "waiting for campaign state";

		if (!state.m_bPetrosAlive)
			return "Recover Petros before ordering new operations.";

		if (CountActiveMissions(state) == 0)
			return "Build support, loot enemy gear, and choose the first mission.";

		if (CountActiveQRFs(state) > 0)
			return "Enemy QRF active. Keep the resistance mobile.";

		if (CountResistanceZones(state, preset) <= 0)
			return "Establish a foothold near a town before escalating.";

		return "Expand support, raid logistics, and pressure weak zones.";
	}

	protected string CommanderGateLabel(bool allowed)
	{
		if (allowed)
			return "authorized";

		return "locked";
	}

	protected string CommanderGateTone(bool allowed)
	{
		if (allowed)
			return "good";

		return "bad";
	}

	protected string CampaignPhaseLabel(HST_ECampaignPhase phase)
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

	protected string ZoneTypeLabel(HST_EZoneType zoneType)
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

	protected string ActiveZoneLabel(bool active)
	{
		if (active)
			return "active";

		return "inactive";
	}

	protected string DisplayZoneName(string zoneId)
	{
		if (zoneId.IsEmpty())
			return "unknown";

		return HST_DefaultCatalog.GetZoneDisplayName(zoneId);
	}

	protected string ShortText(string value, int maxLength)
	{
		if (value.IsEmpty() || maxLength <= 0)
			return "";
		if (value.Length() <= maxLength)
			return value;
		if (maxLength <= 1)
			return value.Substring(0, maxLength);

		return value.Substring(0, maxLength - 1) + ".";
	}

	protected string ZoneTone(HST_ZoneState zone, HST_CampaignPreset preset)
	{
		if (!zone)
			return "neutral";

		if (preset && zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
			return "good";

		if (zone.m_bActive || zone.m_iResistanceCaptureProgress > 0)
			return "warn";

		return "bad";
	}

	protected string CaptureStatusTone(HST_ZoneCaptureStatus status, HST_CampaignPreset preset)
	{
		if (!status)
			return "neutral";

		if (preset && status.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
			return "good";

		if (status.m_iEnemyCountNearby > 0 || status.m_iEnemyVehicleCountNearby > 0)
			return "bad";

		if (status.m_bContested || status.m_iProgressPercent > 0)
			return "warn";

		return "neutral";
	}

	protected string BuildMissionDisplayTitle(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "Mission";
		if (!mission.m_sDisplayName.IsEmpty())
			return mission.m_sDisplayName;
		return mission.m_sMissionId;
	}

	protected string BuildMissionLocationLabel(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!mission)
			return "unknown location";

		if (state && !mission.m_sTargetZoneId.IsEmpty())
		{
			HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
			if (zone)
				return DisplayZoneName(zone);

			string zoneName = HST_DefaultCatalog.GetZoneDisplayName(mission.m_sTargetZoneId);
			if (!zoneName.IsEmpty() && zoneName != mission.m_sTargetZoneId)
				return zoneName;
		}

		if (!mission.m_sSiteId.IsEmpty())
			return HST_DisplayNameService.ResolveReadableDisplayName(mission.m_sSiteId);

		return "field objective";
	}

	protected string BuildMissionTimeLabel(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "timer unknown";

		int remaining = Math.Max(0, mission.m_iRemainingSeconds);
		int minutes = remaining / 60;
		if (minutes > 0)
			return string.Format("%1 min left", minutes);

		return string.Format("%1 sec left", remaining);
	}

	protected string BuildMissionInstruction(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "Complete the marked objective.";

		if (mission.m_sMissionId == "convoy_ammo")
			return "Ambush the ammo convoy. Kill all convoy soldiers before they reach destination. Vehicles may be captured afterward.";
		if (mission.m_sMissionId == "convoy_supplies")
			return "Ambush the supply convoy. Kill all convoy soldiers before they reach destination. Vehicles may be captured afterward.";
		if (mission.m_sMissionId == "convoy_prisoners")
			return "Ambush the convoy. Kill all convoy soldiers before they reach destination. Vehicles may be captured afterward.";
		if (mission.m_sMissionId == "rescue_pows")
			return "Free the POWs, escort them out, and deliver them to extraction.";
		if (mission.m_sMissionId == "assassinate_officer")
			return "Find the marked officer and kill them.";
		if (mission.m_sMissionId == "destroy_radio_tower")
			return "Destroy or sabotage the marked radio target.";
		if (mission.m_sMissionId == "support_city_supplies")
			return "Pick up FIA supplies at HQ and deliver them to the town.";
		if (mission.m_sMissionId == "logistics_resource_cache")
			return "Recover the resource cache and deliver it to HQ.";

		if (mission.m_sRuntimePrimitive == "convoy_intercept")
			return "Ambush the convoy. Kill all convoy soldiers before they reach destination. Vehicles may be captured afterward.";
		if (mission.m_sRuntimePrimitive == "rescue_extract")
			return "Free captives and bring them to extraction.";
		if (mission.m_sRuntimePrimitive == "kill_hvt")
			return "Find and kill the marked HVT.";
		if (mission.m_sRuntimePrimitive == "destroy_target")
			return "Destroy or sabotage the marked target.";
		if (mission.m_sRuntimePrimitive == "recover_cargo")
			return "Recover the marked cargo and deliver it to HQ.";
		if (mission.m_sRuntimePrimitive == "deliver_supplies")
			return "Carry or load supplies and deliver them to the target.";
		if (mission.m_sRuntimePrimitive == "hold_area")
			return "Clear enemies and hold the marked area.";
		if (mission.m_sRuntimePrimitive == "clear_area")
			return "Clear enemies from the marked area.";

		return "Complete the marked objective.";
	}

	protected string BuildMissionNextStepText(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (AreMissionObjectivesComplete(state, mission))
			return "Mission objectives complete. Rewards and cleanup will process on the next campaign tick.";

		if (mission.m_sRuntimePrimitive == "convoy_intercept")
			return BuildConvoyNextStepText(state, mission);

		HST_MissionAssetState asset = SelectMissionNextAsset(state, mission);
		if (!asset)
			return "Follow the orange mission marker. No unresolved physical asset is waiting for an action.";

		string role = BuildMissionAssetRoleLabel(asset);
		if (asset.m_bPickedUp && !asset.m_bDelivered)
		{
			if (asset.m_sRole == "convoy_payload")
				return string.Format("%1 secured. Clear convoy guards or stop the marked vehicle if it is still active.", role);

			if (asset.m_sKind == "captive")
				return string.Format("Escort %1 to the extraction marker, then use Extract captive or Deliver mission cargo/captive.", role);

			return string.Format("Bring %1 to the delivery marker or HQ cache, then use Deliver mission cargo/captive.", role);
		}

		if (asset.m_sKind == "captive")
			return string.Format("At the POW marker, use Free captive. I-menu fallback: Extract nearest captive.");

		if (asset.m_sKind == "cargo")
			return string.Format("At the %1 marker, use Load mission cargo. I-menu fallback: Load nearest mission cargo.", role);

		if (asset.m_sKind == "target")
			return string.Format("At the target marker, destroy it or use Sabotage target. I-menu fallback: Sabotage mission target.");

		if (asset.m_sKind == "character")
			return string.Format("At the HVT marker, kill the officer. If needed, use Confirm HVT neutralized near the body.");

		if (asset.m_sKind == "vehicle")
			return string.Format("Neutralize the convoy crew. Capturing surviving vehicles is optional.");

		return "Use the matching mission action near the marked asset.";
	}

	protected string BuildMissionProgressText(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "unknown";

		int complete;
		int total;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			total++;
			if (objective.m_bComplete)
				complete++;
		}

		string phase = MissionPhaseLabel(mission.m_sRuntimePhase);
		string eta = "";
		if (mission.m_iRuntimeETASeconds > 0 && ShouldShowMissionEta(mission))
			eta = string.Format(" / ETA %1s", mission.m_iRuntimeETASeconds);

		string progress = string.Format("%1%2 | objectives %3/%4", phase, eta, complete, total);
		if (mission.m_sRuntimePrimitive == "convoy_intercept")
		{
			int neutralized;
			int convoyTotal;
			ResolveConvoyCrewProgress(state, mission, neutralized, convoyTotal);
			progress = string.Format("%1%2 | convoy crew groups neutralized %3/%4", phase, eta, neutralized, convoyTotal);
			if (!mission.m_sRuntimeFailureReason.IsEmpty())
				progress = progress + " | " + mission.m_sRuntimeFailureReason;
			return progress;
		}
		if (mission.m_iRequiredCargoCount > 0)
			progress = progress + string.Format(" | cargo %1/%2", mission.m_iRecoveredCargoCount, mission.m_iRequiredCargoCount);
		if (mission.m_iRequiredCaptiveCount > 0)
			progress = progress + string.Format(" | captives %1/%2", mission.m_iExtractedCaptiveCount, mission.m_iRequiredCaptiveCount);
		if (mission.m_iRequiredVehicleCount > 0 && mission.m_sMissionId == "destroy_or_steal_armor")
			progress = progress + string.Format(" | vehicles %1/%2", mission.m_iCapturedVehicleCount, mission.m_iRequiredVehicleCount);
		if (mission.m_iRuntimePickupCount > 0 || mission.m_iRuntimeDeliveryCount > 0 || mission.m_iRuntimeDestroyedCount > 0)
			progress = progress + string.Format(" | moved %1/%2 | destroyed %3", mission.m_iRuntimePickupCount, mission.m_iRuntimeDeliveryCount, mission.m_iRuntimeDestroyedCount);
		return progress;
	}

	protected void AddMissionNextStepActions(HST_CampaignState state, notnull array<ref HST_CommandMenuAction> actions, bool canUseMember, string disabledReason)
	{
		if (!state)
			return;

		int emitted;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || emitted >= 6)
				continue;
			if (AreMissionObjectivesComplete(state, mission))
				continue;

			HST_MissionAssetState asset = SelectMissionNextAsset(state, mission);
			if (!asset)
				continue;

			string commandId = BuildMissionAssetCommandId(asset);
			if (commandId.IsEmpty())
				continue;

			AddMenuAction(actions, TAB_MISSIONS, BuildMissionAssetActionLabel(mission, asset), commandId, asset.m_sAssetId, canUseMember, disabledReason);
			emitted++;
		}
	}

	protected void AddMissionInspectActions(HST_CampaignState state, notnull array<ref HST_CommandMenuAction> actions, bool canUseMember, string disabledReason)
	{
		if (!state)
			return;

		int emitted;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || emitted >= 6)
				continue;

			AddMenuAction(actions, TAB_MISSIONS, "Inspect: " + ShortText(BuildMissionDisplayTitle(mission), 24), "inspect_mission", mission.m_sInstanceId, canUseMember, disabledReason);
			emitted++;
		}
	}

	protected HST_MissionAssetState SelectMissionNextAsset(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return null;
		if (AreMissionObjectivesComplete(state, mission))
			return null;
		if (mission.m_sRuntimePrimitive == "convoy_intercept")
			return null;

		HST_MissionAssetState fallbackVehicle;
		HST_MissionAssetState freedCaptive;
		foreach (HST_MissionAssetState carriedAsset : state.m_aMissionAssets)
		{
			if (!IsMissionAssetActionCandidate(carriedAsset, mission))
				continue;
			if (carriedAsset.m_bPickedUp && !carriedAsset.m_bDelivered && carriedAsset.m_sKind == "cargo")
				return carriedAsset;
			if (carriedAsset.m_bPickedUp && !carriedAsset.m_bDelivered && carriedAsset.m_sKind == "captive" && !freedCaptive)
				freedCaptive = carriedAsset;
		}

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!IsMissionAssetActionCandidate(asset, mission))
				continue;

			if (asset.m_sKind == "captive" && !asset.m_bPickedUp)
				return asset;

			if (asset.m_sKind == "captive")
				continue;

			if (asset.m_sKind == "cargo" || asset.m_sKind == "target" || asset.m_sKind == "character")
				return asset;

			if (asset.m_sKind == "vehicle" && !fallbackVehicle)
				fallbackVehicle = asset;
		}

		if (freedCaptive)
			return freedCaptive;

		return fallbackVehicle;
	}

	protected HST_MissionAssetState SelectMissionConvoyVehicleAsset(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return null;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != "convoy_vehicle" || asset.m_bDestroyed || asset.m_bDelivered)
				continue;

			return asset;
		}

		return null;
	}

	protected string BuildConvoyNextStepText(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		int neutralized;
		int total;
		ResolveConvoyCrewProgress(state, mission, neutralized, total);
		if (mission.m_sRuntimePhase == "convoy_staging")
			return string.Format("Convoy is staging. Prepare an ambush before it moves. Crew groups neutralized %1/%2.", neutralized, total);
		if (mission.m_sRuntimePhase == "convoy_moving")
			return string.Format("Convoy is moving. Kill all convoy soldiers before it reaches destination. Crew groups neutralized %1/%2.", neutralized, total);
		if (mission.m_sRuntimePhase == "convoy_contact")
			return string.Format("Convoy contact started. Neutralize every convoy crew group. Crew groups neutralized %1/%2.", neutralized, total);
		if (mission.m_sRuntimePhase == "convoy_eliminated")
			return "Convoy crew neutralized. Mission completion will process on the next campaign tick; surviving vehicles are optional captures.";
		if (mission.m_sRuntimePhase == "failed")
			return ResolveReadableFailure(mission);

		return string.Format("Neutralize every convoy crew group. Progress %1/%2.", neutralized, total);
	}

	protected void ResolveConvoyCrewProgress(HST_CampaignState state, HST_ActiveMissionState mission, out int neutralized, out int total)
	{
		neutralized = 0;
		total = 0;
		if (!state || !mission)
			return;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_sTargetId != "convoy")
				continue;

			neutralized = objective.m_iCurrentCount;
			total = objective.m_iRequiredCount;
			return;
		}

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == "convoy_vehicle")
				total++;
		}
	}

	protected string ResolveReadableFailure(HST_ActiveMissionState mission)
	{
		if (mission && !mission.m_sRuntimeFailureReason.IsEmpty())
			return mission.m_sRuntimeFailureReason;

		return "Mission failed.";
	}

	protected bool IsMissionAssetActionCandidate(HST_MissionAssetState asset, HST_ActiveMissionState mission)
	{
		if (!asset || !mission || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_bDelivered || asset.m_bDestroyed)
			return false;

		return asset.m_sKind == "cargo" || asset.m_sKind == "captive" || asset.m_sKind == "target" || asset.m_sKind == "character" || asset.m_sKind == "vehicle";
	}

	protected string BuildMissionAssetCommandId(HST_MissionAssetState asset)
	{
		if (!asset)
			return "";

		if (asset.m_sKind == "cargo")
		{
			if (asset.m_bPickedUp)
				return "mission_asset_deliver";

			return "mission_asset_load";
		}

		if (asset.m_sKind == "captive")
			return "mission_captive_extract";

		if (asset.m_sKind == "target" || asset.m_sKind == "character")
			return "mission_asset_sabotage";

		if (asset.m_sKind == "vehicle")
			return "mission_vehicle_capture";

		return "";
	}

	protected string BuildMissionAssetActionLabel(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		string verb = "Use";
		if (asset.m_sKind == "cargo")
		{
			if (asset.m_bPickedUp)
				verb = "Deliver";
			else
				verb = "Load";
		}
		else if (asset.m_sKind == "captive")
		{
			if (asset.m_bPickedUp)
				verb = "Extract";
			else
				verb = "Free";
		}
		else if (asset.m_sKind == "target")
		{
			verb = "Sabotage";
		}
		else if (asset.m_sKind == "character")
		{
			verb = "Confirm kill";
		}
		else if (asset.m_sKind == "vehicle")
		{
			verb = "Capture";
		}

		return string.Format("%1: %2 %3", verb, ShortText(BuildMissionDisplayTitle(mission), 18), BuildMissionAssetRoleLabel(asset));
	}

	protected string BuildMissionAssetRoleLabel(HST_MissionAssetState asset)
	{
		if (!asset)
			return "asset";

		if (asset.m_sRole == "hvt")
			return "officer";
		if (asset.m_sRole == "destroy_target")
			return "target";
		if (asset.m_sRole == "logistics_cargo")
			return "cargo";
		if (asset.m_sRole == "city_supplies")
			return "supplies";
		if (asset.m_sRole == "captive")
			return "captive";
		if (asset.m_sRole == "convoy_vehicle")
			return "vehicle";
		if (asset.m_sRole == "convoy_payload")
			return "payload";
		if (asset.m_sRole == "convoy_captive")
			return "prisoner";

		string role = asset.m_sRole;
		if (role.IsEmpty())
			role = asset.m_sKind;
		if (role.IsEmpty())
			return "asset";
		role.Replace("_", " ");
		return role;
	}

	protected string MissionPhaseLabel(string phase)
	{
		if (phase.IsEmpty() || phase == "created" || phase == "active")
			return "active";
		if (phase == "contact")
			return "contact";
		if (phase == "convoy_static")
			return "static intercept";
		if (phase == "convoy_staging")
			return "convoy staging";
		if (phase == "convoy_moving")
			return "convoy moving";
		if (phase == "convoy_contact")
			return "convoy contact";
		if (phase == "convoy_eliminated")
			return "convoy neutralized";
		if (phase == "convoy_arrived")
			return "convoy arrived";
		if (phase == "holding")
			return "holding";
		if (phase == "extracting")
			return "extracting";
		if (phase == "delivering")
			return "delivering";
		if (phase == "loaded")
			return "loaded";
		if (phase == "unloaded")
			return "unloaded";
		if (phase == "delivered")
			return "delivered";
		if (phase == "destroyed")
			return "destroyed";
		if (phase == "captured")
			return "captured";
		if (phase == "completed")
			return "complete";
		if (phase == "failed")
			return "failed";

		string label = phase;
		label.Replace("_", " ");
		return label;
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

	protected string MissionTone(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "neutral";

		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
			return "warn";

		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED)
			return "good";

		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED || mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED)
			return "bad";

		return "neutral";
	}

	protected string ArsenalTone(HST_ArsenalItemState item)
	{
		if (!item)
			return "neutral";

		if (item.m_bUnlocked)
			return "good";

		return "warn";
	}

	protected string ArsenalCountLabel(HST_ArsenalItemState item)
	{
		if (!item)
			return "0";

		if (item.m_bUnlocked)
			return "INF";

		return string.Format("%1", item.m_iCount);
	}

	protected string AirSupportCapabilityLabel(HST_CampaignState state)
	{
		if (HasResistanceAirSupportCapability(state))
			return "aircraft stored";

		return "requires captured aircraft";
	}

	protected string AirSupportCapabilityTone(HST_CampaignState state)
	{
		if (HasResistanceAirSupportCapability(state))
			return "good";

		return "warn";
	}

	protected string AirSupportDisabledReason(bool canUseCommander, bool airSupportReady)
	{
		if (!canUseCommander)
			return "commander required";

		if (!airSupportReady)
			return "captured aircraft required";

		return "";
	}

	protected bool HasResistanceAirSupportCapability(HST_CampaignState state)
	{
		if (!state)
			return false;

		foreach (HST_GarageVehicleState vehicle : state.m_aGarageVehicles)
		{
			if (!vehicle || vehicle.m_sPrefab.IsEmpty())
				continue;

			if (IsAircraftPrefab(vehicle.m_sPrefab))
				return true;
		}

		return false;
	}

	protected bool IsAircraftPrefab(string prefab)
	{
		return prefab.Contains("Aircraft") || prefab.Contains("Airplane") || prefab.Contains("Plane") || prefab.Contains("Helicopter") || prefab.Contains("Helicopters") || prefab.Contains("UH") || prefab.Contains("AH") || prefab.Contains("Mi-") || prefab.Contains("KA-") || prefab.Contains("Ka-");
	}

	protected int CountResistanceZones(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return 0;

		int count;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
				count++;
		}

		return count;
	}

	protected int CountEnemyZones(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			if (!preset || zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
				count++;
		}

		return count;
	}

	protected int CountActiveZones(HST_CampaignState state)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_bActive)
				count++;
		}

		return count;
	}

	protected int CountResistanceIncome(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return 0;

		int income;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
				income += zone.m_iIncomeValue;
		}

		return income;
	}

	protected int CountActiveMissions(HST_CampaignState state)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (mission && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE && !AreMissionObjectivesComplete(state, mission))
				count++;
		}

		return count;
	}

	protected int CountActiveMissionObjectives(HST_CampaignState state)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective)
				continue;

			HST_ActiveMissionState mission = state.FindActiveMission(objective.m_sMissionInstanceId);
			if (mission && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE && !AreMissionObjectivesComplete(state, mission) && !objective.m_bComplete && !objective.m_bFailed)
				count++;
		}

		return count;
	}

	protected bool HasActiveMissionAssets(HST_CampaignState state)
	{
		if (!state)
			return false;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_bDestroyed || asset.m_bDelivered)
				continue;

			HST_ActiveMissionState mission = state.FindActiveMission(asset.m_sMissionInstanceId);
			if (mission && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE && !AreMissionObjectivesComplete(state, mission))
				return true;
			if (IsPostCompletionConvoyVehicleCaptureCandidate(mission, asset))
				return true;
		}

		return false;
	}

	protected bool IsPostCompletionConvoyVehicleCaptureCandidate(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!mission || !asset)
			return false;
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_SUCCEEDED)
			return false;
		if (mission.m_sRuntimePrimitive != "convoy_intercept" || !IsConvoyCrewEliminationCompletionEvent(mission.m_sLastRuntimeEventKey))
			return false;
		if (asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != "convoy_vehicle" || asset.m_sKind != "vehicle")
			return false;

		return !asset.m_bDestroyed && !asset.m_bDelivered;
	}

	protected bool IsConvoyCrewEliminationCompletionEvent(string eventKey)
	{
		return eventKey == "convoy_eliminated" || eventKey == "convoy_complete";
	}

	protected bool AreMissionObjectivesComplete(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		bool found;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			found = true;
			if (!objective.m_bComplete || objective.m_bFailed)
				return false;
		}

		return found;
	}

	protected int CountActiveQRFs(HST_CampaignState state)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (qrf && !qrf.m_bResolved)
				count++;
		}

		return count;
	}

	protected int CountTrackedArsenalItems(HST_CampaignState state)
	{
		if (!state)
			return 0;

		return state.m_aArsenalItems.Count();
	}

	protected int CountVehicleCargoEntries(HST_CampaignState state)
	{
		if (!state)
			return 0;

		return state.m_aVehicleCargoItems.Count();
	}

	protected int CountVehicleCargoItems(HST_CampaignState state)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_VehicleCargoItemState cargoItem : state.m_aVehicleCargoItems)
		{
			if (cargoItem)
				count += cargoItem.m_iCount;
		}

		return count;
	}

	protected int CountUnlockedArsenalItems(HST_CampaignState state)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_ArsenalItemState item : state.m_aArsenalItems)
		{
			if (item && item.m_bUnlocked)
				count++;
		}

		return count;
	}

	protected int CountGarrisonInfantry(HST_CampaignState state)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_GarrisonState garrison : state.m_aGarrisons)
		{
			if (garrison)
				count += garrison.m_iInfantryCount;
		}

		return count;
	}

	protected int CountGarrisonVehicles(HST_CampaignState state)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_GarrisonState garrison : state.m_aGarrisons)
		{
			if (garrison)
				count += garrison.m_iVehicleCount;
		}

		return count;
	}

	protected int CountActiveZoneVehicles(HST_CampaignState state)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone)
				count += zone.m_iActiveVehicleCount;
		}

		return count;
	}

	protected string RuntimeSpawnTone(HST_CampaignState state)
	{
		if (state && state.m_iRuntimeSpawnFailureCount > 0)
			return "warn";

		return "neutral";
	}
}
