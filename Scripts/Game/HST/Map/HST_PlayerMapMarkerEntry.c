[BaseContainerProps(), SCR_MapMarkerTitle()]
class HST_PlayerMapMarkerEntry : SCR_MapMarkerEntryDynamic
{
	static const ResourceName PLAYER_MARKER_IMAGESET = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
	static const string PLAYER_MARKER_ICON = "circle";

	override SCR_EMapMarkerType GetMarkerType()
	{
		return SCR_EMapMarkerType.HST_PLAYER;
	}

	override void InitServerLogic()
	{
		super.InitServerLogic();
	}

	override void InitClientSettingsDynamic(notnull SCR_MapMarkerEntity marker, notnull SCR_MapMarkerDynamicWComponent widgetComp)
	{
		super.InitClientSettingsDynamic(marker, widgetComp);
		widgetComp.SetImage(PLAYER_MARKER_IMAGESET, PLAYER_MARKER_ICON);
		widgetComp.SetColor(Color.FromInt(0xFF7FD36B));
		widgetComp.SetText(ResolvePlayerMarkerLabel(marker));
	}

	protected string ResolvePlayerMarkerLabel(notnull SCR_MapMarkerEntity marker)
	{
		int playerId = marker.GetMarkerConfigID();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager && playerId > 0)
		{
			string playerName = playerManager.GetPlayerName(playerId);
			playerName = playerName.Trim();
			if (!playerName.IsEmpty())
				return playerName;
		}

		string markerText = marker.GetText();
		markerText = markerText.Trim();
		if (!markerText.IsEmpty())
			return markerText;

		if (playerId > 0)
			return string.Format("Player %1", playerId);

		return "";
	}
}
