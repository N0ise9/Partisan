class HST_BuildInfo
{
	static const string BUILD_SHA = "89e7ad8e0563c46a36b4169397cdfaeb209915e3";
	static const string BUILD_UTC = "2026-07-09T22:06:12Z";
	static const string BUILD_LABEL = "campaign-runtime-integrity-authority-foundation";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
