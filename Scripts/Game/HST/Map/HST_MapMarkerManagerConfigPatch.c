[BaseContainerProps(), SCR_MapMarkerIconEntryTitle()]
modded class SCR_MarkerIconEntry
{
	void HST_SetIconResource(ResourceName imageset, ResourceName imagesetGlow, string imageQuad, string categoryIdentifier = "general")
	{
		m_sCategoryIdentifier = categoryIdentifier;
		m_sIconImageset = imageset;
		m_sIconGlowImageset = imagesetGlow;
		m_sIconImagesetQuad = imageQuad;
	}
}

[BaseContainerProps(), SCR_MapMarkerTitle()]
modded class SCR_MapMarkerEntryPlaced
{
	static const ResourceName HST_CONFLICT_ICON_IMAGESET = "{F7E8D4834A3AFF2F}UI/Imagesets/Conflict/conflict-icons-bw.imageset";
	static const string HST_RADIO_SIGNAL_QUAD = "radio-signal";

	int HST_EnsureRadioSignalIconEntry()
	{
		return HST_EnsureIconEntry(HST_CONFLICT_ICON_IMAGESET, "", HST_RADIO_SIGNAL_QUAD, "general");
	}

	int HST_EnsureIconEntry(ResourceName imageset, ResourceName imagesetGlow, string imageQuad, string categoryIdentifier)
	{
		if (imageQuad.IsEmpty())
			return -1;

		if (!m_aPlacedMarkerIcons)
			m_aPlacedMarkerIcons = {};

		ResourceName existingImageset;
		ResourceName existingGlow;
		string existingQuad;
		foreach (int i, SCR_MarkerIconEntry existingEntry : m_aPlacedMarkerIcons)
		{
			if (!existingEntry)
				continue;

			existingEntry.GetIconResource(existingImageset, existingGlow, existingQuad);
			if (existingImageset == imageset && existingQuad == imageQuad)
				return i;
		}

		SCR_MarkerIconEntry entry = new SCR_MarkerIconEntry();
		entry.HST_SetIconResource(imageset, imagesetGlow, imageQuad, categoryIdentifier);
		m_aPlacedMarkerIcons.Insert(entry);
		return m_aPlacedMarkerIcons.Count() - 1;
	}
}

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

		SCR_MapMarkerEntryPlaced placedEntry = SCR_MapMarkerEntryPlaced.Cast(m_MarkerCfg.GetMarkerEntryConfigByType(SCR_EMapMarkerType.PLACED_CUSTOM));
		if (placedEntry)
			placedEntry.HST_EnsureRadioSignalIconEntry();

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
