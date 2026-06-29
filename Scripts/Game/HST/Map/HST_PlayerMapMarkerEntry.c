[BaseContainerProps(), SCR_MapMarkerTitle()]
class HST_PlayerMapMarkerEntry : SCR_MapMarkerEntryDynamic
{
	static const ResourceName PLAYER_MARKER_IMAGESET = "{3262679C50EF4F01}UI/Textures/Icons/icons_wrapperUI.imageset";
	static const string PLAYER_MARKER_ICON = "whisper";
	static const int PLAYER_MARKER_LABEL_RETRY_MS = 250;
	static const int PLAYER_MARKER_LABEL_RETRY_COUNT = 12;

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
		widgetComp.SetColor(ResolvePlayerMarkerColor(marker.GetMarkerConfigID()));
		widgetComp.SetTextVisible(true);
		ApplyPlayerMarkerLabel(marker, widgetComp, 0);

		HST_PlayerMapMarkerDynamicWComponent playerWidgetComp = HST_PlayerMapMarkerDynamicWComponent.Cast(widgetComp);
		if (playerWidgetComp)
			playerWidgetComp.TrackPlayerFacing(marker);
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
			string playerName = ResolvePlayerDisplayName(playerId);
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

	protected string ResolvePlayerDisplayName(int playerId)
	{
		if (playerId <= 0)
			return "";

		string fallbackName = string.Format("Player %1", playerId);
		string filteredName = SCR_PlayerNamesFilterCache.GetInstance().GetPlayerDisplayName(playerId);
		filteredName = filteredName.Trim();
		if (!filteredName.IsEmpty() && filteredName != fallbackName)
			return filteredName;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			string managerName = playerManager.GetPlayerName(playerId);
			managerName = managerName.Trim();
			if (!managerName.IsEmpty() && managerName != fallbackName)
				return managerName;
		}

		return filteredName;
	}

	protected Color ResolvePlayerMarkerColor(int playerId)
	{
		Faction faction;
		if (playerId > 0)
			faction = SCR_FactionManager.SGetPlayerFaction(playerId);

		if (!faction)
		{
			FactionManager factionManager = GetGame().GetFactionManager();
			if (factionManager)
				faction = factionManager.GetFactionByKey("FIA");
		}

		if (faction)
			return faction.GetFactionColor();

		return Color.FromInt(0xFF7FD36B);
	}
}
