class HST_BuildInfo
{
	static const string BUILD_SHA = "552c2c4ff5ac7608fa248c614480a254769b61a4";
	static const string BUILD_UTC = "2026-07-11T11:57:42Z";
	static const string BUILD_LABEL = "schema55-exact-mission-guard";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
