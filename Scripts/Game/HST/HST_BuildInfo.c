class HST_BuildInfo
{
	static const string BUILD_SHA = "09a1470a4c27dbef866e8cbdba182a7df65fa027";
	static const string BUILD_UTC = "2026-07-11T10:21:27Z";
	static const string BUILD_LABEL = "schema54-exact-garrison-patrol";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
