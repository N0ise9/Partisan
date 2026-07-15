class HST_BuildInfo
{
	static const string BUILD_SHA = "2a17dd992dcffa90dd189b9103792ca8cc903fe6";
	static const string BUILD_UTC = "2026-07-15T16:11:11Z";
	static const string BUILD_LABEL = "schema70-settings24-phase24-owner-snapshot";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
