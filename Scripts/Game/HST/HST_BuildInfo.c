class HST_BuildInfo
{
	static const string BUILD_SHA = "339b72ec3ed63132e46f3df84540d74d3e938d16";
	static const string BUILD_UTC = "2026-07-15T22:32:02Z";
	static const string BUILD_LABEL = "schema70-settings24-counterattack-virtual-restart-proof";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
