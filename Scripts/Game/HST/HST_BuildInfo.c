class HST_BuildInfo
{
	static const string BUILD_SHA = "269e4738ac46f8667b3e05b2cffa8d3991b53e25";
	static const string BUILD_UTC = "2026-07-09T22:45:47Z";
	static const string BUILD_LABEL = "campaign-runtime-integrity-debug-isolation";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
