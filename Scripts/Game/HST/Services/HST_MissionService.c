class HST_MissionService
{
	protected ref array<ref HST_MissionDefinition> m_aDefinitions;
	protected int m_iNextInstanceId = 1;

	void HST_MissionService()
	{
		m_aDefinitions = HST_DefaultCatalog.CreateMissionRegistry();
	}

	HST_MissionDefinition FindDefinition(string missionId)
	{
		foreach (HST_MissionDefinition definition : m_aDefinitions)
		{
			if (definition.m_sMissionId == missionId)
				return definition;
		}

		return null;
	}

	array<ref HST_MissionDefinition> GetDefinitions()
	{
		return m_aDefinitions;
	}

	void SyncNextInstanceIdFromState(HST_CampaignState state)
	{
		if (!state)
			return;

		m_iNextInstanceId = Math.Max(m_iNextInstanceId, state.m_aActiveMissions.Count() + 1);
	}

	bool CanStart(HST_CampaignState state, HST_CampaignPreset preset, string missionId, string targetZoneId = "")
	{
		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
			return false;

		HST_MissionDefinition definition = FindDefinition(missionId);
		if (!definition)
			return false;

		if (definition.m_iRequiredWarLevel > 0 && state.m_iWarLevel < definition.m_iRequiredWarLevel)
			return false;

		HST_ZoneState targetZone;
		if (!targetZoneId.IsEmpty())
		{
			targetZone = state.FindZone(targetZoneId);
			if (!targetZone || !MissionCanTargetZone(definition, targetZone, preset))
				return false;
		}

		foreach (string capabilityId : definition.m_aRequiredCapabilities)
		{
			if (!preset.HasCapability(capabilityId))
				return false;
		}

		foreach (HST_ActiveMissionState activeMission : state.m_aActiveMissions)
		{
			if (!activeMission || activeMission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			HST_MissionDefinition activeDefinition = FindDefinition(activeMission.m_sMissionId);
			bool sameFamily = activeDefinition && activeDefinition.m_eCategory == definition.m_eCategory;
			bool sameTarget = !targetZoneId.IsEmpty() && activeMission.m_sTargetZoneId == targetZoneId;
			bool sameUntargetedMission = targetZoneId.IsEmpty() && activeMission.m_sMissionId == missionId;
			if ((sameFamily && sameTarget) || sameUntargetedMission)
				return false;
		}

		return true;
	}

	bool CanForceStart(HST_CampaignState state, HST_CampaignPreset preset, string missionId, string targetZoneId = "")
	{
		if (!state || state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
			return false;

		HST_MissionDefinition definition = FindDefinition(missionId);
		if (!definition)
			return false;

		HST_ZoneState targetZone;
		if (!targetZoneId.IsEmpty())
		{
			targetZone = state.FindZone(targetZoneId);
			if (!targetZone || !MissionTargetTypeMatches(definition, targetZone))
				return false;
		}

		foreach (string capabilityId : definition.m_aRequiredCapabilities)
		{
			if (!preset.HasCapability(capabilityId))
				return false;
		}

		foreach (HST_ActiveMissionState activeMission : state.m_aActiveMissions)
		{
			if (!activeMission || activeMission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			if (activeMission.m_sMissionId == missionId && activeMission.m_sTargetZoneId == targetZoneId)
				return false;
		}

		return true;
	}

	HST_ActiveMissionState Start(HST_CampaignState state, HST_CampaignPreset preset, string missionId, string targetZoneId = "")
	{
		if (!CanStart(state, preset, missionId, targetZoneId))
			return null;

		HST_MissionDefinition definition = FindDefinition(missionId);
		return CreateActiveMission(state, definition, targetZoneId);
	}

	HST_ActiveMissionState StartForced(HST_CampaignState state, HST_CampaignPreset preset, string missionId, string targetZoneId = "")
	{
		if (!CanForceStart(state, preset, missionId, targetZoneId))
			return null;

		HST_MissionDefinition definition = FindDefinition(missionId);
		return CreateActiveMission(state, definition, targetZoneId);
	}

	protected HST_ActiveMissionState CreateActiveMission(HST_CampaignState state, HST_MissionDefinition definition, string targetZoneId)
	{
		HST_ActiveMissionState activeMission = new HST_ActiveMissionState();
		activeMission.m_sInstanceId = string.Format("mission_%1", m_iNextInstanceId++);
		activeMission.m_sMissionId = definition.m_sMissionId;
		activeMission.m_sDisplayName = definition.m_sDisplayName;
		activeMission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		activeMission.m_iRemainingSeconds = definition.m_iDurationSeconds;
		activeMission.m_sTargetZoneId = targetZoneId;
		activeMission.m_sMarkerId = "hst_mission_" + activeMission.m_sInstanceId;
		activeMission.m_sRuntimeType = definition.m_sRuntimeType;
		activeMission.m_sRuntimePhase = "created";
		activeMission.m_iStartedAtSecond = state.m_iElapsedSeconds;
		activeMission.m_iActiveUntilSecond = state.m_iElapsedSeconds + definition.m_iDurationSeconds;
		activeMission.m_iRequiredCargoCount = Math.Max(0, definition.m_iCargoCount);
		activeMission.m_iRequiredCaptiveCount = Math.Max(0, definition.m_iCaptiveCount);
		activeMission.m_iRequiredVehicleCount = Math.Max(0, definition.m_iVehicleCount);
		activeMission.m_bRequested = true;
		activeMission.m_bDynamic = definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DYNAMIC || definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY || definition.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS;
		state.m_aActiveMissions.Insert(activeMission);
		return activeMission;
	}

	bool Complete(HST_CampaignState state, HST_EconomyService economy, string instanceId)
	{
		foreach (HST_ActiveMissionState activeMission : state.m_aActiveMissions)
		{
			if (activeMission.m_sInstanceId != instanceId || activeMission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			HST_MissionDefinition definition = FindDefinition(activeMission.m_sMissionId);
			if (!definition)
				return false;

			activeMission.m_eStatus = HST_EMissionStatus.HST_MISSION_SUCCEEDED;
			activeMission.m_sRuntimePhase = "completed";
			economy.AddFactionMoney(state, definition.m_iRewardMoney);
			economy.AddHR(state, definition.m_iRewardHR);
			return true;
		}

		return false;
	}

	bool Fail(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, string instanceId)
	{
		foreach (HST_ActiveMissionState activeMission : state.m_aActiveMissions)
		{
			if (activeMission.m_sInstanceId != instanceId || activeMission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			HST_MissionDefinition definition = FindDefinition(activeMission.m_sMissionId);
			if (!definition)
				return false;

			activeMission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
			activeMission.m_sRuntimePhase = "failed";
			if (activeMission.m_sRuntimeFailureReason.IsEmpty())
				activeMission.m_sRuntimeFailureReason = definition.m_sFailureText;
			economy.AddAggression(state, preset.m_sOccupierFactionKey, definition.m_iFailureAggression);
			return true;
		}

		return false;
	}

	bool Tick(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, int elapsedSeconds)
	{
		bool changed;
		foreach (HST_ActiveMissionState activeMission : state.m_aActiveMissions)
		{
			if (activeMission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			activeMission.m_iRemainingSeconds = Math.Max(0, activeMission.m_iRemainingSeconds - elapsedSeconds);
			if (activeMission.m_iRemainingSeconds > 0)
				continue;

			HST_MissionDefinition definition = FindDefinition(activeMission.m_sMissionId);
			activeMission.m_eStatus = HST_EMissionStatus.HST_MISSION_EXPIRED;
			activeMission.m_sRuntimePhase = "expired";
			if (definition)
				activeMission.m_sRuntimeFailureReason = definition.m_sFailureText;
			changed = true;
			if (definition)
				economy.AddAggression(state, preset.m_sOccupierFactionKey, definition.m_iFailureAggression);
		}

		return changed;
	}

	protected bool MissionCanTargetZone(HST_MissionDefinition definition, HST_ZoneState zone, HST_CampaignPreset preset)
	{
		if (!definition || !zone)
			return false;

		bool friendlyTarget = preset && zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey;
		if (friendlyTarget && !definition.m_bAllowFriendlyTarget)
			return false;
		if (!friendlyTarget && !definition.m_bAllowEnemyTarget)
			return false;

		return MissionTargetTypeMatches(definition, zone);
	}

	protected bool MissionTargetTypeMatches(HST_MissionDefinition definition, HST_ZoneState zone)
	{
		if (!definition || !zone)
			return false;

		if (definition.m_aTargetZoneTypes.Count() == 0 || definition.m_aTargetZoneTypes.Contains("any") || definition.m_aTargetZoneTypes.Contains("crashsite") || definition.m_aTargetZoneTypes.Contains("support"))
			return true;

		return definition.m_aTargetZoneTypes.Contains(ZoneTypeKey(zone.m_eType));
	}

	protected string ZoneTypeKey(HST_EZoneType zoneType)
	{
		if (zoneType == HST_EZoneType.HST_ZONE_TOWN)
			return "town";
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
		if (zoneType == HST_EZoneType.HST_ZONE_BANK)
			return "bank";
		if (zoneType == HST_EZoneType.HST_ZONE_OUTPOST || zoneType == HST_EZoneType.HST_ZONE_POLICE_STATION)
			return "outpost";

		return "any";
	}
}
