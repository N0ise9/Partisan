modded class SCR_MapMarkerEntity
{
	static const ResourceName PLAYER_MARKER_CONFIG = "{6985327711306212}Configs/Map/HST_PlayerMapMarkerConfig.conf";
	protected static bool s_bHSTPlayerMarkerConfigUnavailable;

	override protected void EOnInit(IEntity owner)
	{
		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerManager)
			return;

		markerManager.RegisterDynamicMarker(this);

		BaseRplComponent rplComp = BaseRplComponent.Cast(FindComponent(BaseRplComponent));
		if (!rplComp || rplComp.IsOwner())
		{
			SetFlags(EntityFlags.ACTIVE, true);
			SetEventMask(EntityEvent.FRAME);
		}

		SCR_MapMarkerEntityClass markerEntityClass = SCR_MapMarkerEntityClass.Cast(GetPrefabData());
		if (markerEntityClass)
			m_fUpdateDelay = markerEntityClass.GetUpdateDelay();

		m_fTimeTracker = m_fUpdateDelay;
	}

	override void OnCreateMarker()
	{
		if (GetType() == SCR_EMapMarkerType.HST_PLAYER)
		{
			bool configReady = HST_EnsurePlayerMarkerConfig();
			if (!configReady)
				return;

			Print(string.Format("h-istasi player map marker debug | create marker widget requested player=%1 configReady=%2", GetMarkerConfigID(), configReady));
		}

		super.OnCreateMarker();

		if (GetType() == SCR_EMapMarkerType.HST_PLAYER)
			Print(string.Format("h-istasi player map marker debug | create marker widget result player=%1 root=%2 widget=%3", GetMarkerConfigID(), m_wRoot != null, m_MarkerWidgetComp != null));
	}

	bool EnsureHSTPlayerMarkerWidget()
	{
		if (GetType() != SCR_EMapMarkerType.HST_PLAYER)
			return false;

		if (!HST_EnsurePlayerMarkerConfig())
			return false;

		SetLocalVisible(true);
		if (m_wRoot && !m_wRoot.GetParent())
		{
			m_wRoot = null;
			m_MarkerWidgetComp = null;
		}
		if (m_wRoot && !m_MarkerWidgetComp)
		{
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
		}
		if (!m_wRoot || !m_MarkerWidgetComp)
			OnCreateMarker();

		HST_PlayerMapMarkerDynamicWComponent playerWidgetComp = HST_PlayerMapMarkerDynamicWComponent.Cast(m_MarkerWidgetComp);
		if (playerWidgetComp)
			playerWidgetComp.ForceVisibleStyle();
		OnUpdate();

		return m_wRoot != null && m_MarkerWidgetComp != null;
	}

	bool HST_EnsurePlayerMarkerConfig()
	{
		if (s_bHSTPlayerMarkerConfigUnavailable)
			return false;

		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerManager)
			return false;

		SCR_MapMarkerConfig markerConfig = markerManager.GetMarkerConfig();
		if (markerConfig && markerConfig.GetMarkerEntryConfigByType(SCR_EMapMarkerType.HST_PLAYER))
			return true;

		if (markerManager.EnsureHSTMarkerConfig(PLAYER_MARKER_CONFIG))
			return true;

		s_bHSTPlayerMarkerConfigUnavailable = true;
		return false;
	}
}
