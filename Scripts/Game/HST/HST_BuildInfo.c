class HST_BuildInfo
{
	static const string BUILD_SHA = "8ab694fdf61e56c6a5e343782f2225660d3aeeb7";
	static const string BUILD_UTC = "2026-07-11T07:37:08Z";
	static const string BUILD_LABEL = "schema53-exact-enemy-patrol";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
