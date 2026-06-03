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
	static const string TAB_GENERAL = "general";
	static const string TAB_PETROS = "petros";
	static const string TAB_COMMANDER = "commander";
	static const string TAB_ARSENAL = "arsenal";
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

		return header + hq + markerSummary + "\nMember actions: inspect_campaign, inspect_markers, inspect_economy, inspect_zones, inspect_missions, checkpoint";
	}

	string BuildCommanderMenu(HST_CampaignState state, HST_CampaignPreset preset, HST_MapMarkerService markers)
	{
		string menu = BuildMemberMenu(state, preset, markers);
		return menu + "\nCommander actions: move_hq <hideout>, income_now, train_troops, recruit_zone <zone>, mission_zone <zone>, complete_mission <id>";
	}

	string BuildAdminMenu(HST_CampaignState state, HST_CampaignPreset preset, HST_MapMarkerService markers)
	{
		string menu = BuildCommanderMenu(state, preset, markers);
		return menu + "\nAdmin actions: activate_zone <zone>, deactivate_zone <zone>, capture_zone <zone>, progress_zone <zone>, debug_mission <zone>, award_small";
	}

	string BuildVisibleMenuPayload(HST_CampaignState state, HST_CampaignPreset preset, HST_MapMarkerService markers, HST_ArsenalService arsenal, HST_RuntimeSettings settings, int playerId, string selectedTabId, string lastResult, bool canUseMember, bool canUseCommander, bool canUseAdmin)
	{
		if (selectedTabId.IsEmpty())
			selectedTabId = TAB_GENERAL;

		string payload = string.Format("HST_MENU|%1|%2", selectedTabId, playerId);
		payload = payload + "\nTAB|setup|Setup|1";
		payload = payload + "\nTAB|general|General|1";
		payload = payload + "\nTAB|petros|Petros/HQ|1";
		payload = payload + "\nTAB|commander|Commander|1";
		payload = payload + "\nTAB|arsenal|Arsenal/Loot|1";
		payload = payload + "\nTAB|admin|Admin|1";
		payload = payload + "\nSTATUS|" + BuildTabStatusText(state, preset, markers, arsenal, settings, selectedTabId, canUseMember, canUseCommander, canUseAdmin);
		if (!lastResult.IsEmpty())
			payload = payload + "\nRESULT|" + lastResult;

		array<ref HST_CommandMenuAction> actions = {};
		BuildTabActions(selectedTabId, actions, canUseMember, canUseCommander, canUseAdmin);
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

		if (commandId == "inspect_arsenal")
			return coordinator.RequestMemberInspectArsenal(playerId);

		if (commandId == "loot_nearby")
			return coordinator.RequestMemberLootNearby(playerId);

		if (commandId == "move_hq")
			return BuildBoolResult("move HQ to " + argument, coordinator.RequestCommanderMoveHQ(playerId, argument));

		if (commandId == "income_now")
			return BuildBoolResult("apply income tick", coordinator.RequestCommanderApplyIncomeNow(playerId));

		if (commandId == "train_troops")
			return BuildBoolResult("train FIA troops", coordinator.RequestCommanderTrainTroops(playerId));

		if (commandId == "recruit_zone")
			return BuildBoolResult("recruit FIA garrison at " + argument, coordinator.RequestCommanderRecruitGarrison(playerId, argument, 2, 0, 100, 1));

		if (commandId == "mission_zone")
			return BuildBoolResult("start zone mission at " + argument, coordinator.RequestCommanderStartZoneMission(playerId, argument));

		if (commandId == "complete_mission")
			return BuildBoolResult("complete mission " + argument, coordinator.RequestCommanderCompleteMission(playerId, argument));

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

		string report = string.Format("h-istasi missions | active records %1", state.m_aActiveMissions.Count());
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission)
				continue;

			report = report + string.Format("\n%1 | %2 | target %3 | status %4 | remaining %5s", mission.m_sInstanceId, mission.m_sMissionId, mission.m_sTargetZoneId, mission.m_eStatus, mission.m_iRemainingSeconds);
		}

		return report;
	}

	bool ExecuteQuickCommand(HST_CampaignCoordinatorComponent coordinator, int playerId, string commandId, string argument = "")
	{
		if (!coordinator || commandId.IsEmpty())
			return false;

		if (commandId == "checkpoint")
			return coordinator.RequestMemberManualCheckpoint(playerId);

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

		if (commandId == "move_hq")
			return coordinator.RequestCommanderMoveHQ(playerId, argument);

		if (commandId == "income_now")
			return coordinator.RequestCommanderApplyIncomeNow(playerId);

		if (commandId == "train_troops")
			return coordinator.RequestCommanderTrainTroops(playerId);

		if (commandId == "recruit_zone")
			return coordinator.RequestCommanderRecruitGarrison(playerId, argument, 2, 0, 100, 1);

		if (commandId == "mission_zone")
			return coordinator.RequestCommanderStartZoneMission(playerId, argument);

		if (commandId == "complete_mission")
			return coordinator.RequestCommanderCompleteMission(playerId, argument);

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

		return false;
	}

	protected string BuildTabStatusText(HST_CampaignState state, HST_CampaignPreset preset, HST_MapMarkerService markers, HST_ArsenalService arsenal, HST_RuntimeSettings settings, string selectedTabId, bool canUseMember, bool canUseCommander, bool canUseAdmin)
	{
		if (selectedTabId == TAB_SETUP)
			return BuildSetupStatus(state, settings);

		if (selectedTabId == TAB_GENERAL)
		{
			if (!canUseMember)
				return "h-istasi general | membership required for campaign actions";

			return BuildMemberMenu(state, preset, markers);
		}

		if (selectedTabId == TAB_PETROS)
			return BuildPetrosStatus(state, canUseCommander);

		if (selectedTabId == TAB_COMMANDER)
		{
			if (!canUseCommander)
				return "h-istasi commander | commander role required";

			return BuildEconomyReport(state);
		}

		if (selectedTabId == TAB_ARSENAL)
		{
			if (!canUseMember)
				return "h-istasi arsenal | membership required for looting";

			if (arsenal)
				return arsenal.BuildArsenalReport(state);

			return "h-istasi arsenal | service not ready";
		}

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
		string status = "h-istasi setup | settings source $profile:h-istasi/HST_Settings.json | read-only in game";
		if (state)
			status = status + string.Format("\ncampaign | schema %1 | preset %2 | phase %3 | seed %4", state.m_iSchemaVersion, state.m_sPresetId, state.m_ePhase, state.m_iCampaignSeed);

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

		return string.Format("h-istasi Petros/HQ | hideout %1 | HQ %2 | Petros alive %3 | %4", state.m_sHQHideoutId, state.m_vHQPosition, state.m_bPetrosAlive, role);
	}

	protected void BuildTabActions(string selectedTabId, notnull array<ref HST_CommandMenuAction> actions, bool canUseMember, bool canUseCommander, bool canUseAdmin)
	{
		actions.Clear();
		if (selectedTabId == TAB_SETUP)
		{
			AddMenuAction(actions, TAB_SETUP, "Config path / source of truth", "noop", "", true, "");
			return;
		}

		if (selectedTabId == TAB_GENERAL)
		{
			AddMenuAction(actions, TAB_GENERAL, "Campaign overview", "inspect_campaign", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_GENERAL, "Marker status", "inspect_markers", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_GENERAL, "Economy report", "inspect_economy", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_GENERAL, "Zone report", "inspect_zones", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_GENERAL, "Mission report", "inspect_missions", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_GENERAL, "Manual checkpoint", "checkpoint", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_PETROS)
		{
			AddMenuAction(actions, TAB_PETROS, "Move HQ: north forest", "move_hq", "hideout_north_forest", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_PETROS, "Move HQ: central hills", "move_hq", "hideout_central_hills", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_PETROS, "Move HQ: south woods", "move_hq", "hideout_south_woods", canUseCommander, "commander required");
			return;
		}

		if (selectedTabId == TAB_COMMANDER)
		{
			AddMenuAction(actions, TAB_COMMANDER, "Apply income tick", "income_now", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_COMMANDER, "Train FIA troops", "train_troops", "", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_COMMANDER, "Recruit FIA at Morton", "recruit_zone", "town_morton", canUseCommander, "commander required");
			AddMenuAction(actions, TAB_COMMANDER, "Start mission at Morton", "mission_zone", "town_morton", canUseCommander, "commander required");
			return;
		}

		if (selectedTabId == TAB_ARSENAL)
		{
			AddMenuAction(actions, TAB_ARSENAL, "Loot nearby to arsenal", "loot_nearby", "", canUseMember, "membership required");
			AddMenuAction(actions, TAB_ARSENAL, "Arsenal report", "inspect_arsenal", "", canUseMember, "membership required");
			return;
		}

		if (selectedTabId == TAB_ADMIN)
		{
			AddMenuAction(actions, TAB_ADMIN, "Debug capture Morton", "capture_zone", "town_morton", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Debug activate Morton", "activate_zone", "town_morton", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Debug deactivate Morton", "deactivate_zone", "town_morton", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Debug mission at Morton", "debug_mission", "town_morton", canUseAdmin, "admin required");
			AddMenuAction(actions, TAB_ADMIN, "Debug award resources", "award_small", "", canUseAdmin, "admin required");
		}
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

	protected string BuildBoolResult(string label, bool success)
	{
		if (success)
			return "h-istasi command | " + label + " | complete";

		return "h-istasi command | " + label + " | denied or failed";
	}
}
