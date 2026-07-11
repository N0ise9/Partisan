class HST_BuildInfo
{
	static const string BUILD_SHA = "bab5748d817ba434dae701cfbb3b92805d463678";
	static const string BUILD_UTC = "2026-07-11T12:47:17Z";
	static const string BUILD_LABEL = "schema56-exact-traitor-guard";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
