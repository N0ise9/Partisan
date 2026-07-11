class HST_BuildInfo
{
	static const string BUILD_SHA = "37fb5f0ffbc80c4bba3151ba1e5d8be6ffcf8a21";
	static const string BUILD_UTC = "2026-07-11T20:01:02Z";
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
