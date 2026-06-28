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
		widgetComp.SetText(marker.GetText());
	}
}
