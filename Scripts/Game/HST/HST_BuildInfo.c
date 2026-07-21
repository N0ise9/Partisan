class HST_BuildInfo
{
	static const string BUILD_SHA = "8281b41bd39a7d4cebcdf78d7e163fb6f12e7eef";
	static const string BUILD_UTC = "2026-07-21T18:06:57Z";
	static const string BUILD_LABEL = "schema71-settings24-gate1-release-surface";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
