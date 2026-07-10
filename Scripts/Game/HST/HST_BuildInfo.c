class HST_BuildInfo
{
	static const string BUILD_SHA = "2bdfa8b357dbd12a577ba8694a440db139121c5d";
	static const string BUILD_UTC = "2026-07-10T12:54:03Z";
	static const string BUILD_LABEL = "schema47-exact-force-runtime-lifecycle";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
