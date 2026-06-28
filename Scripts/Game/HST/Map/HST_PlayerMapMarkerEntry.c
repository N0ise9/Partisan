[BaseContainerProps(), SCR_MapMarkerTitle()]
class HST_PlayerMapMarkerEntry : SCR_MapMarkerEntryDynamic
{
	static const ResourceName PLAYER_MARKER_IMAGESET = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
	static const string PLAYER_MARKER_ICON = "circle";
	static const int PLAYER_MARKER_LABEL_RETRY_MS = 250;
	static const int PLAYER_MARKER_LABEL_RETRY_COUNT = 4;

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
		ApplyPlayerMarkerLabel(marker, widgetComp, 0);
	}

	protected void ApplyPlayerMarkerLabel(SCR_MapMarkerEntity marker, SCR_MapMarkerDynamicWComponent widgetComp, int attempt)
	{
		if (!marker || !widgetComp)
			return;

		string label = ResolvePlayerMarkerLabel(marker);
		widgetComp.SetText(label);

		if (attempt >= PLAYER_MARKER_LABEL_RETRY_COUNT)
			return;
		if (!ShouldRetryPlayerMarkerLabel(marker, label))
			return;

		GetGame().GetCallqueue().CallLater(ApplyPlayerMarkerLabel, PLAYER_MARKER_LABEL_RETRY_MS, false, marker, widgetComp, attempt + 1);
	}

	protected bool ShouldRetryPlayerMarkerLabel(notnull SCR_MapMarkerEntity marker, string label)
	{
		int playerId = marker.GetMarkerConfigID();
		if (playerId <= 0)
			return true;
		if (label.IsEmpty())
			return true;

		return label == string.Format("Player %1", playerId);
	}

	protected string ResolvePlayerMarkerLabel(notnull SCR_MapMarkerEntity marker)
	{
		int playerId = marker.GetMarkerConfigID();
		if (playerId > 0)
		{
			string playerName = SCR_PlayerNamesFilterCache.GetInstance().GetPlayerDisplayName(playerId);
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
