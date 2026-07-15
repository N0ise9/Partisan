class HST_BuildInfo
{
	static const string BUILD_SHA = "393733cc165b96ec494c72f96741cf993d400ebd";
	static const string BUILD_UTC = "2026-07-15T10:46:59Z";
	static const string BUILD_LABEL = "schema70-settings24-native-counterattack-proof-ordering";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
