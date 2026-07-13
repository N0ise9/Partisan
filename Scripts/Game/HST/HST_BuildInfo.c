class HST_BuildInfo
{
	static const string BUILD_SHA = "4c9a94a1cb4811b6e75a7dca5dba70efffcb523d";
	static const string BUILD_UTC = "2026-07-13T15:43:01Z";
	static const string BUILD_LABEL = "schema69-settings24-exact-enemy-counterattack-provisional";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
