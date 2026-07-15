class HST_BuildInfo
{
	static const string BUILD_SHA = "22c425f416d607bd8a94027e1d486dc3f06c4c47";
	static const string BUILD_UTC = "2026-07-15T06:31:32Z";
	static const string BUILD_LABEL = "schema70-settings24-native-counterattack-casualty-continuity";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
