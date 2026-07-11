class HST_BuildInfo
{
	static const string BUILD_SHA = "e25fb40934b241c1a11d22c3e77250206479bf0b";
	static const string BUILD_UTC = "2026-07-11T02:00:35Z";
	static const string BUILD_LABEL = "schema51-exact-enemy-defensive-qrf";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
