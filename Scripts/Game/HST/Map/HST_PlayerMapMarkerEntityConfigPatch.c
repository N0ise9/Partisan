modded class SCR_MapMarkerEntity
{
	static const ResourceName PLAYER_MARKER_CONFIG = "{6985327711306212}Configs/Map/HST_PlayerMapMarkerConfig.conf";

	override void OnCreateMarker()
	{
		if (GetType() == SCR_EMapMarkerType.HST_PLAYER)
		{
			SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
			if (markerManager)
				markerManager.EnsureHSTMarkerConfig(PLAYER_MARKER_CONFIG);
		}

		super.OnCreateMarker();
	}
}
