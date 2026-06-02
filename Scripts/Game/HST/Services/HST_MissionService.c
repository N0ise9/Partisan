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

	bool CanStart(HST_CampaignState state, HST_CampaignPreset preset, string missionId)
	{
		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
			return false;

		HST_MissionDefinition definition = FindDefinition(missionId);
		if (!definition)
			return false;

		foreach (string capabilityId : definition.m_aRequiredCapabilities)
		{
			if (!preset.HasCapability(capabilityId))
				return false;
		}

		foreach (HST_ActiveMissionState activeMission : state.m_aActiveMissions)
		{
			if (activeMission.m_sMissionId == missionId && activeMission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				return false;
		}

		return true;
	}

	HST_ActiveMissionState Start(HST_CampaignState state, HST_CampaignPreset preset, string missionId, string targetZoneId = "")
	{
		if (!CanStart(state, preset, missionId))
			return null;

		HST_MissionDefinition definition = FindDefinition(missionId);
		HST_ActiveMissionState activeMission = new HST_ActiveMissionState();
		activeMission.m_sInstanceId = string.Format("mission_%1", m_iNextInstanceId++);
		activeMission.m_sMissionId = definition.m_sMissionId;
		activeMission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		activeMission.m_iRemainingSeconds = definition.m_iDurationSeconds;
		activeMission.m_sTargetZoneId = targetZoneId;
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
			economy.AddFactionMoney(state, definition.m_iRewardMoney);
			economy.AddHR(state, definition.m_iRewardHR);
			return true;
		}

		return false;
	}

	void Tick(HST_CampaignState state, HST_CampaignPreset preset, int elapsedSeconds)
	{
		foreach (HST_ActiveMissionState activeMission : state.m_aActiveMissions)
		{
			if (activeMission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			activeMission.m_iRemainingSeconds = Math.Max(0, activeMission.m_iRemainingSeconds - elapsedSeconds);
			if (activeMission.m_iRemainingSeconds == 0)
				activeMission.m_eStatus = HST_EMissionStatus.HST_MISSION_EXPIRED;
		}
	}
}
