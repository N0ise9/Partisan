class HST_BuildInfo
{
	static const string BUILD_SHA = "3361f59b8cc863bd47c0e78858ae601cf1512fb4";
	static const string BUILD_UTC = "2026-07-11T00:05:32Z";
	static const string BUILD_LABEL = "schema50-strategic-projection-feedback-repairs";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
