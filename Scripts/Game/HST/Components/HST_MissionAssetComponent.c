[ComponentEditorProps(category: "h-istasi", description: "Marker component for h-istasi mission-owned physical assets")]
class HST_MissionAssetComponentClass : ScriptComponentClass
{
}

class HST_MissionAssetComponent : ScriptComponent
{
	[RplProp()]
	protected string m_sAssetId;
	[RplProp()]
	protected string m_sMissionInstanceId;
	[RplProp()]
	protected string m_sRole;
	[RplProp()]
	protected bool m_bRescueActionProjectionEvaluated;
	[RplProp()]
	protected bool m_bRescueActionProjectionConfigured;
	[RplProp()]
	protected int m_iRescueActionContractVersion;
	[RplProp()]
	protected HST_ERescueCaptiveDisposition m_eRescueActionDisposition = HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_UNKNOWN;
	[RplProp()]
	protected bool m_bRescueActionMissionActive;
	[RplProp()]
	protected bool m_bRescueActionAuthorityQuarantined;
	[RplProp()]
	protected int m_iRescueActionRevision;

	void ConfigureMissionAsset(string assetId, string missionInstanceId, string role)
	{
		if (m_sAssetId == assetId && m_sMissionInstanceId == missionInstanceId && m_sRole == role)
			return;

		m_sAssetId = assetId;
		m_sMissionInstanceId = missionInstanceId;
		m_sRole = role;
		if (Replication.IsServer())
			Replication.BumpMe();
	}

	void ConfigureRescueActionProjection(
		bool configured,
		int contractVersion,
		HST_ERescueCaptiveDisposition disposition,
		bool missionActive,
		bool authorityQuarantined,
		int revision)
	{
		revision = Math.Max(0, revision);
		if (m_bRescueActionProjectionEvaluated
			&& m_bRescueActionProjectionConfigured == configured
			&& m_iRescueActionContractVersion == contractVersion
			&& m_eRescueActionDisposition == disposition
			&& m_bRescueActionMissionActive == missionActive
			&& m_bRescueActionAuthorityQuarantined == authorityQuarantined
			&& m_iRescueActionRevision == revision)
			return;

		m_bRescueActionProjectionEvaluated = true;
		m_bRescueActionProjectionConfigured = configured;
		m_iRescueActionContractVersion = contractVersion;
		m_eRescueActionDisposition = disposition;
		m_bRescueActionMissionActive = missionActive;
		m_bRescueActionAuthorityQuarantined = authorityQuarantined;
		m_iRescueActionRevision = revision;
		if (Replication.IsServer())
			Replication.BumpMe();
	}

	void RefreshRescueActionProjectionFromAuthority()
	{
		if (!Replication.IsServer() || m_sAssetId.IsEmpty())
			return;

		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		HST_CampaignState state;
		if (coordinator)
			state = coordinator.GetState();
		if (!state)
			return;

		HST_MissionAssetState asset = state.FindMissionAsset(m_sAssetId);
		if (!asset)
		{
			ConfigureRescueActionProjection(true, m_iRescueActionContractVersion,
				m_eRescueActionDisposition, false, true, m_iRescueActionRevision);
			return;
		}

		HST_ActiveMissionState mission = state.FindActiveMission(asset.m_sMissionInstanceId);
		bool rescueClaimant = asset.m_iRescueContractVersion != 0
			|| asset.m_eRescueDisposition
				!= HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_UNKNOWN
			|| HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission);
		if (!rescueClaimant)
		{
			ConfigureRescueActionProjection(false, 0,
				HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_UNKNOWN,
				false, false, 0);
			return;
		}

		bool quarantined = asset.m_iRescueContractVersion
			== HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION
			|| HST_RescuePOWOperationService.IsQuarantinedMission(mission);
		bool missionActive = mission
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			&& !quarantined;
		ConfigureRescueActionProjection(true, asset.m_iRescueContractVersion,
			asset.m_eRescueDisposition, missionActive, quarantined,
			asset.m_iRescueRevision);
	}

	string GetAssetId()
	{
		return m_sAssetId;
	}

	string GetMissionInstanceId()
	{
		return m_sMissionInstanceId;
	}

	string GetRole()
	{
		return m_sRole;
	}

	bool HasRescueActionProjection()
	{
		return m_bRescueActionProjectionConfigured;
	}

	bool IsRescueActionProjectionEvaluated()
	{
		return m_bRescueActionProjectionEvaluated;
	}

	int GetRescueActionContractVersion()
	{
		return m_iRescueActionContractVersion;
	}

	HST_ERescueCaptiveDisposition GetRescueActionDisposition()
	{
		return m_eRescueActionDisposition;
	}

	bool IsRescueActionMissionActive()
	{
		return m_bRescueActionMissionActive;
	}

	bool IsRescueActionAuthorityQuarantined()
	{
		return m_bRescueActionAuthorityQuarantined;
	}

	int GetRescueActionRevision()
	{
		return m_iRescueActionRevision;
	}
}
