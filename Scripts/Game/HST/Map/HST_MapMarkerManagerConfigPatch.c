modded class SCR_MapMarkerManagerComponent
{
	bool EnsureHSTMarkerConfig(ResourceName markerConfigPath)
	{
		if (markerConfigPath.IsEmpty())
			return false;

		if (m_MarkerCfg && m_MarkerCfg.GetMarkerEntryConfigByType(SCR_EMapMarkerType.HST_PLAYER))
			return true;

		Resource container = BaseContainerTools.LoadContainer(markerConfigPath);
		if (!container)
			return false;

		SCR_MapMarkerConfig markerConfig = SCR_MapMarkerConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(container.GetResource().ToBaseContainer()));
		if (!markerConfig)
			return false;

		m_MarkerCfg = markerConfig;
		InitHSTMarkerConfigEntries();
		return m_MarkerCfg.GetMarkerEntryConfigByType(SCR_EMapMarkerType.HST_PLAYER) != null;
	}

	protected void InitHSTMarkerConfigEntries()
	{
		if (!m_MarkerCfg)
			return;

		bool isMaster;
		if (m_pGameMode)
			isMaster = m_pGameMode.IsMaster();

		array<ref SCR_MapMarkerEntryConfig> entryConfigs = m_MarkerCfg.GetMarkerEntryConfigs();
		if (!entryConfigs)
			return;

		foreach (SCR_MapMarkerEntryConfig entryConfig : entryConfigs)
		{
			SCR_MapMarkerEntryDynamic dynamicEntry = SCR_MapMarkerEntryDynamic.Cast(entryConfig);
			if (!dynamicEntry)
				continue;

			if (isMaster)
				dynamicEntry.InitServerLogic();
			dynamicEntry.InitClientLogic();
		}
	}
}
