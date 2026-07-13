class HST_BuildInfo
{
	static const string BUILD_SHA = "f97b12ef6ab00f6997ee16001eea74eb876e94b1";
	static const string BUILD_UTC = "2026-07-13T01:59:08Z";
	static const string BUILD_LABEL = "schema68-settings24-partisan-profile-namespace";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
