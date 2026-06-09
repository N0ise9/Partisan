[ComponentEditorProps(category: "h-istasi", description: "Marker component for h-istasi mission-owned physical assets")]
class HST_MissionAssetComponentClass : ScriptComponentClass
{
}

class HST_MissionAssetComponent : ScriptComponent
{
	protected string m_sAssetId;
	protected string m_sMissionInstanceId;
	protected string m_sRole;

	void ConfigureMissionAsset(string assetId, string missionInstanceId, string role)
	{
		m_sAssetId = assetId;
		m_sMissionInstanceId = missionInstanceId;
		m_sRole = role;
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
}
