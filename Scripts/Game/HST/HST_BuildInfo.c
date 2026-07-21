class HST_BuildInfo
{
	static const string BUILD_SHA = "0f914a104a36de53999c8080e73e02d2641bf3ce";
	static const string BUILD_UTC = "2026-07-21T17:57:12Z";
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
