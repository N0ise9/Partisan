class HST_BuildInfo
{
	static const string BUILD_SHA = "86fdcdaed9a18353603b7f43ffbb0b5e90de8611";
	static const string BUILD_UTC = "2026-07-10T00:02:01Z";
	static const string BUILD_LABEL = "campaign-runtime-integrity-exact-garrison-authority";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
