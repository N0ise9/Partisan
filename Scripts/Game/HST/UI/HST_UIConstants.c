enum HST_EUIScreenMode
{
	NONE,
	SETUP_MAP,
	COMMAND_MENU,
	LOADOUT_EDITOR,
	ACTION_DIALOG,
	MISSION_DIALOG,
	GAMEPLAY_MAP_OVERLAY
}

class HST_UIConstants
{
	// Keep the native map root low enough that the engine-owned map cursor stays usable.
	static const int Z_SETUP_MAP = 9;
	static const int Z_SETUP_PROMPT = 2300;
	static const int Z_SETUP_MODAL = 2400;
	static const int Z_COMMAND_MENU = 2500;
	static const int Z_NOTIFICATION = 2850;
	static const int Z_LOADOUT_EDITOR = 3600;
	static const int Z_ACTION_DIALOG = 4300;
	static const int Z_MISSION_DIALOG = 4300;

	static string ModeName(HST_EUIScreenMode mode)
	{
		switch (mode)
		{
			case HST_EUIScreenMode.SETUP_MAP:
				return "setup_map";
			case HST_EUIScreenMode.COMMAND_MENU:
				return "command_menu";
			case HST_EUIScreenMode.LOADOUT_EDITOR:
				return "loadout_editor";
			case HST_EUIScreenMode.ACTION_DIALOG:
				return "action_dialog";
			case HST_EUIScreenMode.MISSION_DIALOG:
				return "mission_dialog";
			case HST_EUIScreenMode.GAMEPLAY_MAP_OVERLAY:
				return "gameplay_map_overlay";
		}

		return "none";
	}
}
