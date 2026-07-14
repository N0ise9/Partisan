class HST_BuildInfo
{
	static const string BUILD_SHA = "a81d494cce5beeca1acaff27e3341874b11a7fdb";
	static const string BUILD_UTC = "2026-07-14T14:04:27Z";
	static const string BUILD_LABEL = "schema70-settings24-radio-rebuild-rpl-source";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
