modded class SCR_MapMarkerEntity
{
	static const ResourceName PLAYER_MARKER_CONFIG = "{6985327711306212}Configs/Map/HST_PlayerMapMarkerConfig.conf";

	override void OnCreateMarker()
	{
		if (GetType() == SCR_EMapMarkerType.HST_PLAYER)
		{
			bool configReady = false;
			SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
			if (markerManager)
				configReady = markerManager.EnsureHSTMarkerConfig(PLAYER_MARKER_CONFIG);

			Print(string.Format("h-istasi player map marker debug | create marker widget player=%1 configReady=%2", GetMarkerConfigID(), configReady));
		}

		super.OnCreateMarker();
	}
}
