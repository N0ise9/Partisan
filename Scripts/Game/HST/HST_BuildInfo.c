class HST_BuildInfo
{
	static const string BUILD_SHA = "4dfb91ae57ada20eaf16d0bc482c8fb55877eafa";
	static const string BUILD_UTC = "2026-07-10T12:12:28Z";
	static const string BUILD_LABEL = "schema46-exact-paid-qrf";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
