class HST_BuildInfo
{
	static const string BUILD_SHA = "a7031797e67d99a99a066038cd8fa39efc03cff1";
	static const string BUILD_UTC = "2026-07-12T20:28:33Z";
	static const string BUILD_LABEL = "schema66-settings24-local-security-marker-integrity";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
