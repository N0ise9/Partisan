class HST_BuildInfo
{
	static const string BUILD_SHA = "27672e67ce4285810f313130293df1ac917c9bdf";
	static const string BUILD_UTC = "2026-07-12T01:02:39Z";
	static const string BUILD_LABEL = "schema61-authoritative-marker-projection";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
