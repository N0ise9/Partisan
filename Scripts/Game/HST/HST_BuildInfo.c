class HST_BuildInfo
{
	static const string BUILD_SHA = "d97c0e03a222d8681e6a47d5e01a593324564e05";
	static const string BUILD_UTC = "2026-07-16T02:24:44Z";
	static const string BUILD_LABEL = "schema70-settings24-counterattack-materializing-deferred-restart-proof";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
