class HST_BuildInfo
{
	static const string BUILD_SHA = "60596bf77d056b9e63ed1bbbf4d11c1941330fe6";
	static const string BUILD_UTC = "2026-07-18T14:12:51Z";
	static const string BUILD_LABEL = "schema71-settings24-mixed-native-shutdown-restart";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
