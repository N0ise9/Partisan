class HST_BuildInfo
{
	static const string BUILD_SHA = "f375a8a4961e1dc0574ef93c6fae564659197f58";
	static const string BUILD_UTC = "2026-07-10T17:41:30Z";
	static const string BUILD_LABEL = "schema48-typed-support-recall-authority";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
