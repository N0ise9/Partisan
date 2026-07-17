class HST_BuildInfo
{
	static const string BUILD_SHA = "dceefed3eb3c8f9c93210d4d9b5dcd9510d549c1";
	static const string BUILD_UTC = "2026-07-16T23:52:22Z";
	static const string BUILD_LABEL = "schema70-settings24-controlled-campaign-persistence";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
