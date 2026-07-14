class HST_BuildInfo
{
	static const string BUILD_SHA = "89b7754bcd9ac7e8c41f2a8d7604784b5c1c1c83";
	static const string BUILD_UTC = "2026-07-14T16:01:36Z";
	static const string BUILD_LABEL = "schema70-settings24-current-support-roundtrip";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
