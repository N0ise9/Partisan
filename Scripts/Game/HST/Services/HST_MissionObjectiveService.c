class HST_MissionObjectiveService
{
	bool InitializeMission(HST_CampaignState state, HST_CampaignPreset preset, HST_MissionDefinition definition, HST_ActiveMissionState mission, HST_GeneratedContentService content)
	{
		if (!state || !definition || !mission)
			return false;

		if (HasObjectiveForMission(state, mission.m_sInstanceId))
			return false;

		HST_GeneratedSiteState site;
		if (content)
			site = content.FindMissionSiteForZone(state, mission.m_sTargetZoneId, definition.m_eCategory, definition.m_sMissionId);

		if (!site && content)
			site = content.FindPrimarySiteForZone(state, mission.m_sTargetZoneId);

		if (site)
			mission.m_sSiteId = site.m_sSiteId;

		vector objectivePosition = ResolveMissionPosition(state, mission, site);
		mission.m_vTargetPosition = objectivePosition;
		CreateMissionTask(state, definition, mission, objectivePosition);
		AddObjective(state, mission, PrimaryObjectiveForMission(definition), BuildObjectiveTarget(definition, mission), objectivePosition, RequiredProgressForMission(definition), LabelForMission(definition), definition.m_sRequirementText, RequiredCountForMission(definition));

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
		{
			if (definition.m_sMissionId == "convoy_prisoners")
				AddObjective(state, mission, HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES, "convoy_prisoners", objectivePosition, 1, "Extract prisoners", "Extract the convoy prisoners alive.", definition.m_iCaptiveCount);
			else
				AddObjective(state, mission, HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT, "convoy_cargo", objectivePosition, 1, "Recover convoy cargo", "Recover the convoy payload after stopping the vehicles.", definition.m_iCargoCount);
		}

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS)
			AddObjective(state, mission, HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES, "hq_delivery", state.m_vHQPosition, 1, "Deliver to HQ", "Bring the recovered cargo back to HQ.", definition.m_iCargoCount);

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_RESCUE)
			AddObjective(state, mission, HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES, "extract_captives", state.m_vHQPosition, 1, "Extract captives", "Escort the captives back to HQ or a friendly zone.", definition.m_iCaptiveCount);

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			AddObjective(state, mission, HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT, "supply_pickup", state.m_vHQPosition, 1, "Pick up supplies", "Collect FIA supplies from HQ before delivery.", definition.m_iCargoCount);

		return true;
	}

	bool Tick(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			{
				changed = MarkMissionObjectiveCleanupComplete(state, mission) || changed;
				continue;
			}

			if (AreMissionObjectivesComplete(state, mission.m_sInstanceId))
			{
				CompleteTaskForMission(state, mission.m_sInstanceId, false);
				changed = true;
			}
		}

		return changed;
	}

	bool ProgressMission(HST_CampaignState state, string instanceId, int amount = 1)
	{
		if (!state || instanceId.IsEmpty() || amount <= 0)
			return false;

		bool changed;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != instanceId || objective.m_bComplete || objective.m_bFailed)
				continue;

			objective.m_iCurrentProgress = Math.Min(objective.m_iRequiredProgress, objective.m_iCurrentProgress + amount);
			if (objective.m_iCurrentProgress >= objective.m_iRequiredProgress)
				objective.m_bComplete = true;

			changed = true;
			break;
		}

		if (AreMissionObjectivesComplete(state, instanceId))
			CompleteTaskForMission(state, instanceId, false);

		return changed;
	}

	bool FailMissionObjectives(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return false;

		bool changed;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != instanceId || objective.m_bComplete || objective.m_bFailed)
				continue;

			objective.m_bFailed = true;
			changed = true;
		}

		CompleteTaskForMission(state, instanceId, true);
		return changed;
	}

	bool AreMissionObjectivesComplete(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return false;

		bool found;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != instanceId)
				continue;

			found = true;
			if (!objective.m_bComplete || objective.m_bFailed)
				return false;
		}

		return found;
	}

	string BuildObjectiveReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi objectives | state not ready";

		int active;
		int complete;
		int failed;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective)
				continue;

			if (objective.m_bFailed)
				failed++;
			else if (objective.m_bComplete)
				complete++;
			else
				active++;
		}

		return string.Format("h-istasi objectives | active %1 | complete %2 | failed %3 | tasks %4", active, complete, failed, state.m_aCampaignTasks.Count());
	}

	protected bool HasObjectiveForMission(HST_CampaignState state, string instanceId)
	{
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == instanceId)
				return true;
		}

		return false;
	}

	protected void AddObjective(HST_CampaignState state, HST_ActiveMissionState mission, HST_EMissionObjectiveType objectiveType, string targetId, vector position, int requiredProgress, string label = "", string requirementText = "", int requiredCount = 1)
	{
		HST_MissionObjectiveState objective = new HST_MissionObjectiveState();
		objective.m_sObjectiveId = string.Format("obj_%1_%2", mission.m_sInstanceId, state.m_aMissionObjectives.Count());
		objective.m_sMissionInstanceId = mission.m_sInstanceId;
		objective.m_eType = objectiveType;
		objective.m_sLabel = label;
		if (objective.m_sLabel.IsEmpty())
			objective.m_sLabel = ObjectiveTypeLabel(objectiveType);
		objective.m_sRequirementText = requirementText;
		if (objective.m_sRequirementText.IsEmpty())
			objective.m_sRequirementText = objective.m_sLabel;
		objective.m_sTargetId = targetId;
		objective.m_sTargetZoneId = mission.m_sTargetZoneId;
		objective.m_sPhysicalEntityId = string.Format("phys_%1_%2", mission.m_sInstanceId, targetId);
		objective.m_sLinkedRuntimeEntityId = objective.m_sPhysicalEntityId;
		objective.m_vPosition = position;
		objective.m_iRequiredProgress = Math.Max(1, requiredProgress);
		objective.m_iRequiredCount = Math.Max(1, requiredCount);
		state.m_aMissionObjectives.Insert(objective);
	}

	protected HST_EMissionObjectiveType PrimaryObjectiveForMission(HST_MissionDefinition definition)
	{
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_ASSASSINATION)
			return HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET;

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA;

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
			return HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET;

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS)
			return HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT;

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_RESCUE)
			return HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES;

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			return HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES;

		if (definition.m_sMissionId == "dynamic_city_flip_battle" || definition.m_sMissionId == "dynamic_defend_petros")
			return HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA;

		return HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA;
	}

	protected int RequiredProgressForMission(HST_MissionDefinition definition)
	{
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return 3;

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
		{
			return 1;
		}

		if (definition.m_sMissionId == "dynamic_city_flip_battle")
			return 3;

		return 1;
	}

	protected string BuildObjectiveTarget(HST_MissionDefinition definition, HST_ActiveMissionState mission)
	{
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_ASSASSINATION)
			return "hvt";

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return "convoy";

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
			return "asset";

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_RESCUE)
			return "captives";

		if (!mission.m_sTargetZoneId.IsEmpty())
			return mission.m_sTargetZoneId;

		return definition.m_sMissionId;
	}

	protected vector ResolveMissionPosition(HST_CampaignState state, HST_ActiveMissionState mission, HST_GeneratedSiteState site)
	{
		if (site)
			return site.m_vPosition;

		if (mission && !mission.m_sTargetZoneId.IsEmpty())
		{
			HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
			if (zone)
				return zone.m_vPosition;
		}

		return state.m_vHQPosition;
	}

	protected void CreateMissionTask(HST_CampaignState state, HST_MissionDefinition definition, HST_ActiveMissionState mission, vector position)
	{
		if (state.FindCampaignTask("task_" + mission.m_sInstanceId))
			return;

		HST_CampaignTaskState task = new HST_CampaignTaskState();
		task.m_sTaskId = "task_" + mission.m_sInstanceId;
		task.m_sLinkedId = mission.m_sInstanceId;
		task.m_sTitle = definition.m_sDisplayName;
		task.m_sDescription = string.Format("%1\nRequirements: %2\nFailure: %3", definition.m_sBriefingText, definition.m_sRequirementText, definition.m_sFailureText);
		task.m_sCategory = "mission";
		task.m_vPosition = position;
		task.m_bActive = true;
		state.m_aCampaignTasks.Insert(task);
	}

	protected string LabelForMission(HST_MissionDefinition definition)
	{
		if (!definition)
			return "Complete mission";

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_ASSASSINATION)
			return "Eliminate HVT";
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return "Clear and hold";
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return "Stop convoy";
		if (definition.m_sMissionId == "destroy_or_steal_armor")
			return "Destroy or capture armor";
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
			return "Destroy target";
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS)
			return "Recover cargo";
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_RESCUE)
			return "Make contact";
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			return "Deliver supplies";
		if (definition.m_sMissionId == "dynamic_defend_petros")
			return "Defend HQ";
		if (definition.m_sMissionId == "dynamic_stop_tower_rebuild")
			return "Stop rebuild";
		if (definition.m_sMissionId == "dynamic_city_flip_battle")
			return "Win city fight";
		if (definition.m_sMissionId == "dynamic_gun_shop")
			return "Secure weapons";

		return "Resolve task";
	}

	protected int RequiredCountForMission(HST_MissionDefinition definition)
	{
		if (!definition)
			return 1;

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return 1;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_RESCUE)
			return Math.Max(1, definition.m_iCaptiveCount);
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS || definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			return Math.Max(1, definition.m_iCargoCount);
		if (definition.m_sMissionId == "destroy_or_steal_armor")
			return Math.Max(1, definition.m_iVehicleCount);

		return 1;
	}

	protected string ObjectiveTypeLabel(HST_EMissionObjectiveType objectiveType)
	{
		if (objectiveType == HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA)
			return "Clear area";
		if (objectiveType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET)
			return "Kill target";
		if (objectiveType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET)
			return "Destroy target";
		if (objectiveType == HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT)
			return "Recover loot";
		if (objectiveType == HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES)
			return "Deliver supplies";
		if (objectiveType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
			return "Rescue captives";
		if (objectiveType == HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA)
			return "Hold area";

		return "Find site";
	}

	protected void CompleteTaskForMission(HST_CampaignState state, string instanceId, bool failed)
	{
		HST_CampaignTaskState task = state.FindCampaignTask("task_" + instanceId);
		if (!task)
			return;

		task.m_bActive = false;
		task.m_bFailed = failed;
		task.m_bSucceeded = !failed;
	}

	protected bool MarkMissionObjectiveCleanupComplete(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		bool changed;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bCleanupComplete)
				continue;

			objective.m_bCleanupComplete = true;
			changed = true;
		}

		return changed;
	}
}
