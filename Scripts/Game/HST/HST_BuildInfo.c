class HST_BuildInfo
{
	static const string BUILD_SHA = "3157ca28b066630ffb87cac292f74e20ce243efd";
	static const string BUILD_UTC = "2026-07-10T18:47:12Z";
	static const string BUILD_LABEL = "schema48-mixed-group-personnel-lifecycle";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
