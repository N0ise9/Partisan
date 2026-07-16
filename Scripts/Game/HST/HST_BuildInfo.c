class HST_BuildInfo
{
	static const string BUILD_SHA = "a6e9069f29f8b844f8545b77b8894170ecd6d3b8";
	static const string BUILD_UTC = "2026-07-16T20:53:27Z";
	static const string BUILD_LABEL = "schema70-settings24-native-persistence-source-selection";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
