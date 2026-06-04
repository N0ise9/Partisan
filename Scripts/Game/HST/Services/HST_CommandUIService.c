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

		return header + hq + markerSummary + "\nMember actions: inspect_campaign, inspect_markers, inspect_economy, inspect_zones, inspect_missions, inspect_arsenal, loot_nearby, checkpoint";
	}

	string BuildCommanderMenu(HST_CampaignState state, HST_CampaignPreset preset, HST_MapMarkerService markers)
	{
		string menu = BuildMemberMenu(state, preset, markers);
		return menu + "\nCommander actions: move_hq_here, move_hq <hideout>, income_now, train_troops, recruit_zone <zone>, mission_zone <zone>, complete_mission <id>";
	}

	string BuildAdminMenu(HST_CampaignState state, HST_CampaignPreset preset, HST_MapMarkerService markers)
	{
		string menu = BuildCommanderMenu(state, preset, markers);
		return menu + "\nAdmin actions: activate_zone <zone>, deactivate_zone <zone>, capture_zone <zone>, progress_zone <zone>, debug_mission <zone>, award_small";
	}

	string BuildVisibleMenuPayload(HST_CampaignState state, HST_CampaignPreset preset, HST_MapMarkerService markers, HST_ArsenalService arsenal, HST_RuntimeSettings settings, int playerId, string selectedTabId, string lastResult, bool canUseMember, bool canUseCommander, bool canUseAdmin)
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
		payload = payload + "\nTAB|garage|Garage|1";
		payload = payload + "\nTAB|members|Members|1";
		payload = payload + "\nTAB|admin|Admin|1";
		payload = payload + "\nSTATUS|" + BuildTabStatusText(state, preset, markers, arsenal, settings, selectedTabId, canUseMember, canUseCommander, canUseAdmin);
		payload = AppendTopStats(payload, state, preset);
		payload = AppendTabSections(payload, state, preset, markers, arsenal, settings, selectedTabId, canUseMember, canUseCommander, canUseAdmin);
		payload = AppendActivityFeed(payload, state, preset, selectedTabId);

		if (!lastResult.IsEmpty())
			payload = payload + "\nRESULT|" + lastResult;

		array<ref HST_CommandMenuAction> actions = {};
		BuildTabActions(state, preset, selectedTabId, actions, canUseMember, canUseCommander, canUseAdmin);
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
			return BuildBoolResult("manual checkpoint", coordinator.RequestMemberManualCheckpoint(playerId));

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

		if (commandId == "garage_capture_nearby")
			return coordinator.RequestMemberCaptureNearbyVehicle(playerId);

		if (commandId == "garage_redeploy")
			return coordinator.RequestMemberRedeployGarageVehicle(playerId, argument);

		if (commandId == "move_hq")
			return BuildBoolResult("move HQ to " + argument, coordinator.RequestCommanderMoveHQ(playerId, argument));

		if (commandId == "move_hq_here")
			return BuildBoolResult("move HQ to your position", coordinator.RequestCommanderMoveHQToPlayer(playerId));

		if (commandId == "income_now")
			return BuildBoolResult("apply income tick", coordinator.RequestCommanderApplyIncomeNow(playerId));

		if (commandId == "train_troops")
			return BuildBoolResult("train FIA troops", coordinator.RequestCommanderTrainTroops(playerId));

		if (commandId == "recruit_zone")
			return BuildBoolResult("recruit FIA garrison at " + argument, coordinator.RequestCommanderRecruitGarrison(playerId, argument, 2, 0, 100, 1));

		if (commandId == "mission_zone")
			return BuildBoolResult("start zone mission at " + argument, coordinator.RequestCommanderStartZoneMission(playerId, argument));

		if (commandId == "mission_random")
			return BuildBoolResult("start random campaign mission", coordinator.RequestCommanderStartRandomMission(playerId));

		if (commandId == "progress_mission")
			return BuildBoolResult("progress active mission", coordinator.RequestCommanderProgressMission(playerId, argument));

		if (commandId == "complete_mission")
			return BuildBoolResult("complete mission " + argument, coordinator.RequestCommanderCompleteMission(playerId, argument));

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

		if (commandId == "award_small")
			return BuildBoolResult("award debug resources", coordinator.RequestAdminAwardResources(playerId, 500, 5));

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

			report = report + string.Format("\n%1 | %2 | target %3 | site %4 | status %5 | remaining %6s | primitive %7", mission.m_sInstanceId, mission.m_sMissionId, mission.m_sTargetZoneId, mission.m_sSiteId, mission.m_eStatus, mission.m_iRemainingSeconds, mission.m_sRuntimePrimitive);
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

		if (commandId == "inspect_markers")
			return !coordinator.RequestMemberInspectMarkers(playerId).IsEmpty();

		if (commandId == "inspect_economy")
			return !coordinator.RequestMemberInspectEconomy(playerId).IsEmpty();

		if (commandId == "inspect_zones")
			return !coordinator.RequestMemberInspectZones(playerId).IsEmpty();

		if (commandId == "inspect_missions")
			return !coordinator.RequestMemberInspectMissions(playerId).IsEmpty();

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

		if (commandId == "garage_capture_nearby")
			return IsActionResultComplete(coordinator.RequestMemberCaptureNearbyVehicle(playerId));

		if (commandId == "garage_redeploy")
			return IsActionResultComplete(coordinator.RequestMemberRedeployGarageVehicle(playerId, argument));

		if (commandId == "move_hq")
			return coordinator.RequestCommanderMoveHQ(playerId, argument);

		if (commandId == "move_hq_here")
			return coordinator.RequestCommanderMoveHQToPlayer(playerId);

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

		if (commandId == "award_small")
			return coordinator.RequestAdminAwardResources(playerId, 500, 5);

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

	protected string AppendTabSections(string payload, HST_CampaignState state, HST_CampaignPreset preset, HST_MapMarkerService markers, HST_ArsenalService arsenal, HST_RuntimeSettings settings, string selectedTabId, bool canUseMember, bool canUseCommander, bool canUseAdmin)
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
			return AppendHQSections(payload, state, canUseCommander);

		if (selectedTabId == TAB_MISSIONS)
			return AppendMissionSections(payload, state);

		if (selectedTabId == TAB_MAP)
			return AppendMapSections(payload, state, preset);

		if (selectedTabId == TAB_FORCES)
			return AppendForcesSections(payload, state, preset, canUseCommander);

		if (selectedTabId == TAB_ARSENAL)
			return AppendArsenalSections(payload, state, settings);

		if (selectedTabId == TAB_GARAGE)
			return AppendGarageSections(payload, state, settings);

		if (selectedTabId == TAB_MEMBERS)
			return AppendMembersSections(payload, state, canUseCommander);

		if (selectedTabId == TAB_ADMIN)
			return AppendAdminSections(payload, state, preset, canUseAdmin);

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

	protected string AppendHQSections(string payload, HST_CampaignState state, bool canUseCommander)
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
		payload = AppendRow(payload, "hq", "Runtime objects", BuildRuntimeObjectLabel(state), BuildRuntimeObjectTone(state));

		payload = AppendSection(payload, "moves", "Move Base");
		payload = AppendRow(payload, "moves", "Move here", "Uses your current position as the new HQ.", CommanderGateTone(canUseCommander));
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
		if (state.m_aActiveMissions.Count() == 0)
			payload = AppendRow(payload, "active_missions", "No active mission", "Use Start mission at Morton to create the first alpha task.", "neutral");

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission)
				continue;

			payload = AppendRow(payload, "active_missions", mission.m_sInstanceId, string.Format("%1 / target %2 / site %3 / %4s", mission.m_sMissionId, mission.m_sTargetZoneId, mission.m_sSiteId, mission.m_iRemainingSeconds), MissionTone(mission));
			if (!mission.m_sRuntimePrimitive.IsEmpty())
				payload = AppendRow(payload, "active_missions", "Runtime", string.Format("%1 / spawned %2 / fallback %3", mission.m_sRuntimePrimitive, mission.m_bRuntimeSpawned, mission.m_bRuntimeFallback), MissionTone(mission));
		}

		payload = AppendSection(payload, "objectives", "Objective State");
		payload = AppendRow(payload, "objectives", "Objectives", string.Format("%1 tracked", state.m_aMissionObjectives.Count()), "warn");
		payload = AppendRow(payload, "objectives", "Tasks", string.Format("%1 campaign tasks", state.m_aCampaignTasks.Count()), "neutral");

		payload = AppendSection(payload, "families", "Mission Board");
		payload = AppendRow(payload, "families", "Convoys", "Ammo, money, prisoners, reinforcements, supplies.", "warn");
		payload = AppendRow(payload, "families", "Logistics", "Recover trucks, rob banks, salvage equipment.", "good");
		payload = AppendRow(payload, "families", "Conquest", "Capture resources, outposts, factories, ports.", "bad");
		payload = AppendRow(payload, "families", "Dynamic", "Defend Petros, city flip battles, tower rebuilds.", "warn");
		payload = AppendRow(payload, "families", "Support", "Build town support before taking the fight wider.", "good");
		return payload;
	}

	protected string AppendMapSections(string payload, HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return payload;

		payload = AppendSection(payload, "war_map", "Map Control");
		payload = AppendRow(payload, "war_map", "Resistance zones", string.Format("%1", CountResistanceZones(state, preset)), "good");
		payload = AppendRow(payload, "war_map", "Hostile zones", string.Format("%1", CountEnemyZones(state, preset)), "bad");
		payload = AppendRow(payload, "war_map", "Active zones", string.Format("%1", CountActiveZones(state)), "warn");
		payload = AppendRow(payload, "war_map", "Projected income", string.Format("$%1 per tick", CountResistanceIncome(state, preset)), "good");
		payload = AppendRow(payload, "war_map", "Generated sites", string.Format("%1", state.m_aGeneratedSites.Count()), "neutral");
		payload = AppendRow(payload, "war_map", "Generated routes", string.Format("%1", state.m_aGeneratedRoutes.Count()), "neutral");

		payload = AppendSection(payload, "zones", "Zone Pressure");
		int emitted;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			string label = string.Format("%1 / %2", DisplayZoneName(zone.m_sZoneId), ZoneTypeLabel(zone.m_eType));
			string value = string.Format("owner %1 / support %2 / capture %3 / active %4", zone.m_sOwnerFactionKey, zone.m_iSupport, zone.m_iResistanceCaptureProgress, zone.m_bActive);
			payload = AppendRow(payload, "zones", label, value, ZoneTone(zone, preset));
			emitted++;
			if (emitted >= 10)
				break;
		}

		return payload;
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

	protected string AppendArsenalSections(string payload, HST_CampaignState state, HST_RuntimeSettings settings)
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
			payload = AppendRow(payload, "arsenal", "Vehicle loot", string.Format("%1 / rear %2m / max %3", settings.m_VehicleLoot.m_bEnabled, settings.m_VehicleLoot.m_iRadiusMeters, settings.m_VehicleLoot.m_iMaxItemsPerAction), "neutral");
			payload = AppendRow(payload, "arsenal", "Unlock threshold", string.Format("%1 / magazines x%2", settings.m_ArsenalLoot.m_iArsenalUnlockThreshold, settings.m_ArsenalLoot.m_iMagazineUnlockMultiplier), "neutral");
			payload = AppendRow(payload, "arsenal", "Loot rule", string.Format("locked only %1 / remove source %2", settings.m_ArsenalLoot.m_bLootOnlyLockedItems, settings.m_ArsenalLoot.m_bRemoveLootedItems), "warn");
		}

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

	protected string AppendGarageSections(string payload, HST_CampaignState state, HST_RuntimeSettings settings)
	{
		if (!state)
			return payload;

		payload = AppendSection(payload, "garage", "Virtual Garage");
		payload = AppendRow(payload, "garage", "Stored vehicles", string.Format("%1", state.m_aGarageVehicles.Count()), GarageTone(state));
		payload = AppendRow(payload, "garage", "Cargo entries", string.Format("%1 entries / %2 item(s)", CountVehicleCargoEntries(state), CountVehicleCargoItems(state)), "warn");
		payload = AppendRow(payload, "garage", "Captured emplacements", string.Format("%1", state.m_aCapturedEmplacements.Count()), "neutral");
		payload = AppendRow(payload, "garage", "Ammo points", string.Format("%1", state.m_aAmmoPoints.Count()), "neutral");
		payload = AppendRow(payload, "garage", "Active zone vehicles", string.Format("%1 physical / %2 abstract", CountActiveZoneVehicles(state), CountGarrisonVehicles(state)), "neutral");
		if (settings)
			payload = AppendRow(payload, "garage", "HQ action radius", string.Format("%1m", settings.m_ArsenalLoot.m_iHQInteractionRadiusMeters), "good");

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
		payload = AppendRow(payload, "garage_actions", "Redeploy", "Each stored vehicle gets its own selected redeploy action.", GarageTone(state));
		payload = AppendRow(payload, "garage_actions", "Vehicle target", "Nearest top-level vehicle root within action range.", "neutral");
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

	protected string AppendAdminSections(string payload, HST_CampaignState state, HST_CampaignPreset preset, bool canUseAdmin)
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

	protected void BuildTabActions(HST_CampaignState state, HST_CampaignPreset preset, string selectedTabId, notnull array<ref HST_CommandMenuAction> actions, bool canUseMember, bool canUseCommander, bool canUseAdmin)
	{
		actions.Clear();
		selectedTabId = NormalizeTabId(selectedTabId);
		string primaryTargetId = SelectPriorityMissionZoneId(state, preset);
		string hostileTownId = SelectFirstZoneIdByType(state, preset, HST_EZoneType.HST_ZONE_TOWN, true);
		string resourceTargetId = SelectFirstZoneIdByType(state, preset, HST_EZoneType.HST_ZONE_RESOURCE, true);
		string outpostTargetId = SelectFirstZoneIdByType(state, preset, HST_EZoneType.HST_ZONE_OUTPOST, true);
		string recruitTargetId = SelectRecruitZoneId(state, preset);
		string adminTargetId = SelectAdminTargetZoneId(state);
		string guestIdentityId = SelectFirstGuestIdentity(state);
		string memberIdentityId = SelectFirstMemberIdentity(state);
		bool airSupportReady = HasResistanceAirSupportCapability(state);
		string firstGarageVehicleId = SelectFirstGarageVehicleId(state);
		if (selectedTabId == TAB_SETUP)
		{
			AddMenuAction(actions, TAB_SETUP, "Config path / source of truth", "noop", "", true, "");
			AddMenuAction(actions, TAB_SETUP, "Start HQ: north forest", "setup_hideout", "hideout_north_forest", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_SETUP, "Start HQ: central hills", "setup_hideout", "hideout_central_hills", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_SETUP, "Start HQ: south woods", "setup_hideout", "hideout_south_woods", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_SETUP, "Persistence status", "inspect_persistence", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_SETUP, "Manual checkpoint", "checkpoint", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_OVERVIEW)
		{
			AddMenuAction(actions, TAB_OVERVIEW, "Campaign overview", "inspect_campaign", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_OVERVIEW, "Marker status", "inspect_markers", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_OVERVIEW, "Economy report", "inspect_economy", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_OVERVIEW, "Persistence status", "inspect_persistence", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_OVERVIEW, "Manual checkpoint", "checkpoint", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_PETROS)
		{
			AddMenuAction(actions, TAB_PETROS, "Move base to my position", "move_hq_here", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_PETROS, "Move HQ: north forest", "move_hq", "hideout_north_forest", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_PETROS, "Move HQ: central hills", "move_hq", "hideout_central_hills", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_PETROS, "Move HQ: south woods", "move_hq", "hideout_south_woods", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_PETROS, "Open Arsenal/Loot", "inspect_arsenal", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_MISSIONS)
		{
			AddMenuAction(actions, TAB_MISSIONS, "Start priority mission", "mission_random", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_MISSIONS, BuildZoneActionLabel("Start mission", state, primaryTargetId), "mission_zone", primaryTargetId, canUseCommander && !primaryTargetId.IsEmpty(), "no valid target");
			AddMenuAction(actions, TAB_MISSIONS, BuildZoneActionLabel("Start town mission", state, hostileTownId), "mission_zone", hostileTownId, canUseCommander && !hostileTownId.IsEmpty(), "no hostile town");
			AddMenuAction(actions, TAB_MISSIONS, BuildZoneActionLabel("Start resource mission", state, resourceTargetId), "mission_zone", resourceTargetId, canUseCommander && !resourceTargetId.IsEmpty(), "no hostile resource");
			AddMenuAction(actions, TAB_MISSIONS, BuildZoneActionLabel("Start outpost mission", state, outpostTargetId), "mission_zone", outpostTargetId, canUseCommander && !outpostTargetId.IsEmpty(), "no hostile outpost");
			AddMenuAction(actions, TAB_MISSIONS, "Progress active mission", "progress_mission", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_MISSIONS, "Mission report", "inspect_missions", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_MISSIONS, "Objective report", "inspect_objectives", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_MISSIONS, "Runtime report", "inspect_mission_runtime", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_MISSIONS, "Zone report", "inspect_zones", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_MAP)
		{
			AddMenuAction(actions, TAB_MAP, "Zone report", "inspect_zones", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_MAP, "Marker status", "inspect_markers", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_MAP, "Generated content", "inspect_content", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_MAP, "Civilian status", "inspect_civilians", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_MAP, "Campaign overview", "inspect_campaign", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_FORCES)
		{
			AddMenuAction(actions, TAB_FORCES, "Apply income tick", "income_now", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_FORCES, "Train FIA troops", "train_troops", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_FORCES, BuildZoneActionLabel("Recruit FIA", state, recruitTargetId), "recruit_zone", recruitTargetId, canUseCommander && !recruitTargetId.IsEmpty(), "no recruit target");
			AddMenuAction(actions, TAB_FORCES, "Request supply drop", "call_supply", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_FORCES, "Request QRF reserve", "support_qrf", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_FORCES, "Request suppressive fire", "support_fire", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_FORCES, "Request GBU air strike", "support_gbu", "", canUseCommander && airSupportReady, AirSupportDisabledReason(canUseCommander, airSupportReady));
			AddMenuAction(actions, TAB_FORCES, "Request UMPK air strike", "support_umpk", "", canUseCommander && airSupportReady, AirSupportDisabledReason(canUseCommander, airSupportReady));
			AddMenuAction(actions, TAB_FORCES, "Cancel player support", "cancel_support", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_FORCES, "Deliver civilian aid", "civilian_aid", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_FORCES, "Economy report", "inspect_economy", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_FORCES, "Support report", "inspect_support", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_ARSENAL)
		{
			AddMenuAction(actions, TAB_ARSENAL, "Loot nearby to arsenal", "loot_nearby", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_ARSENAL, "Load loot to vehicle", "vehicle_collect_loot", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_ARSENAL, "Unload vehicle loot to arsenal", "vehicle_unload_loot", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_ARSENAL, "Withdraw first available item", "withdraw_arsenal", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_ARSENAL, "Arsenal report", "inspect_arsenal", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_ARSENAL, "Vehicle cargo report", "inspect_vehicle_cargo", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_ARSENAL, "Manual checkpoint", "checkpoint", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_GARAGE)
		{
			AddMenuAction(actions, TAB_GARAGE, "Capture nearest vehicle", "garage_capture_nearby", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_GARAGE, "Unload vehicle loot to arsenal", "vehicle_unload_loot", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_GARAGE, "Inspect vehicle cargo", "inspect_vehicle_cargo", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_GARAGE, "Garage report", "inspect_garage", "", canUseMember, "membership required");

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

			AddMenuAction(actions, TAB_GARAGE, "Manual checkpoint", "checkpoint", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_MEMBERS)
		{
			AddMenuAction(actions, TAB_MEMBERS, "Accept first guest", "member_accept", guestIdentityId, canUseAdmin && !guestIdentityId.IsEmpty(), "admin required or no guest");
			AddMenuAction(actions, TAB_MEMBERS, "Remove first member", "member_remove", memberIdentityId, canUseAdmin && !memberIdentityId.IsEmpty(), "admin required or no member");
			AddMenuAction(actions, TAB_MEMBERS, "Grant admin to member", "admin_grant", memberIdentityId, canUseAdmin && !memberIdentityId.IsEmpty(), "admin required or no member");
			AddMenuAction(actions, TAB_MEMBERS, "Undercover status", "inspect_undercover", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_MEMBERS, "Campaign overview", "inspect_campaign", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_MEMBERS, "Manual checkpoint", "checkpoint", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_ADMIN)
		{
			AddMenuAction(actions, TAB_ADMIN, BuildZoneActionLabel("Debug capture", state, adminTargetId), "capture_zone", adminTargetId, canUseAdmin && !adminTargetId.IsEmpty(), "no zone");
			AddMenuAction(actions, TAB_ADMIN, BuildZoneActionLabel("Debug activate", state, adminTargetId), "activate_zone", adminTargetId, canUseAdmin && !adminTargetId.IsEmpty(), "no zone");
			AddMenuAction(actions, TAB_ADMIN, BuildZoneActionLabel("Debug deactivate", state, adminTargetId), "deactivate_zone", adminTargetId, canUseAdmin && !adminTargetId.IsEmpty(), "no zone");
			AddMenuAction(actions, TAB_ADMIN, BuildZoneActionLabel("Debug mission", state, adminTargetId), "debug_mission", adminTargetId, canUseAdmin && !adminTargetId.IsEmpty(), "no zone");
			AddMenuAction(actions, TAB_ADMIN, "Debug award resources", "award_small", "", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Persistence status", "inspect_persistence", "", canUseAdmin, "admin required");
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

	protected string SelectRecruitZoneId(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return "";

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && preset && zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
				return zone.m_sZoneId;
		}

		return SelectPriorityMissionZoneId(state, preset);
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

	protected string DisplayZoneName(string zoneId)
	{
		if (zoneId.IsEmpty())
			return "unknown";

		return HST_DefaultCatalog.GetZoneDisplayName(zoneId);
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
			if (mission && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				count++;
		}

		return count;
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
}
