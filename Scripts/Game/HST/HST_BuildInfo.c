class HST_BuildInfo
{
	static const string BUILD_SHA = "412640a96a4b514b105ff6050bcabc6167b9ddfa";
	static const string BUILD_UTC = "2026-07-09T22:24:06Z";
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
