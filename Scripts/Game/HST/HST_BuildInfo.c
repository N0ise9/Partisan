class HST_BuildInfo
{
	static const string BUILD_SHA = "356b0d47f96111c3b09eb7ede3cb34f0661c2b6e";
	static const string BUILD_UTC = "2026-07-13T01:04:41Z";
	static const string BUILD_LABEL = "schema68-settings24-enemy-planning-authority";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
