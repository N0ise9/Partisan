class HST_BuildInfo
{
	static const string BUILD_SHA = "952a2d33245074867df6afad1ffe25ce49fc9a11";
	static const string BUILD_UTC = "2026-07-17T01:12:37Z";
	static const string BUILD_LABEL = "schema70-settings24-periodic-autosave-scheduler";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
