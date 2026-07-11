class HST_BuildInfo
{
	static const string BUILD_SHA = "fdf78493dd15915afe8d53f61a8ad1efd65b5635";
	static const string BUILD_UTC = "2026-07-11T23:24:55Z";
	static const string BUILD_LABEL = "schema60-exact-search-destroy";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
