class HST_BuildInfo
{
	static const string BUILD_SHA = "ad18c4f0c7f89c62887aa9b52d728f59c5278766";
	static const string BUILD_UTC = "2026-07-10T13:43:10Z";
	static const string BUILD_LABEL = "schema48-debug-observation-repairs";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
