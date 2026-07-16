class HST_BuildInfo
{
	static const string BUILD_SHA = "db91cb680685fd79019b735fe6d56efc9825a1d3";
	static const string BUILD_UTC = "2026-07-16T12:04:47Z";
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
