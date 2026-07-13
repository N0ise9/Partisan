class HST_BuildInfo
{
	static const string BUILD_SHA = "2f71236bfc02329a3c8000b104f1b7b1043dc99c";
	static const string BUILD_UTC = "2026-07-13T22:20:52Z";
	static const string BUILD_LABEL = "schema70-settings24-exact-enemy-garrison-rebuild-engine-proof";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
