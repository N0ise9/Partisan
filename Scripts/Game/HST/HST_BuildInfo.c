class HST_BuildInfo
{
	static const string BUILD_SHA = "514ebdcbeb1ddfb2a383b19590382517113e2ff6";
	static const string BUILD_UTC = "2026-07-11T13:22:35Z";
	static const string BUILD_LABEL = "schema57-exact-specops-guard";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
