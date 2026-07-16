class HST_BuildInfo
{
	static const string BUILD_SHA = "008cd481d5e55b43c7afc902cd5e906cbb297415";
	static const string BUILD_UTC = "2026-07-16T13:07:11Z";
	static const string BUILD_LABEL = "schema70-settings24-counterattack-endpoint-owner-claimant-restart-proof";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
