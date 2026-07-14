class HST_BuildInfo
{
	static const string BUILD_SHA = "0e54f6cbc7f7084e5534fc603b491cba0d91b653";
	static const string BUILD_UTC = "2026-07-14T18:31:39Z";
	static const string BUILD_LABEL = "schema70-settings24-active-demolition-witness";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
