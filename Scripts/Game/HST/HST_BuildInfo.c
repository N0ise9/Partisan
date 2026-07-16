class HST_BuildInfo
{
	static const string BUILD_SHA = "02f64410670a3ffced10c8e099c05eaf5a469cb0";
	static const string BUILD_UTC = "2026-07-16T12:17:23Z";
	static const string BUILD_LABEL = "schema70-settings24-counterattack-prepared-settlement-restart-proof";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
