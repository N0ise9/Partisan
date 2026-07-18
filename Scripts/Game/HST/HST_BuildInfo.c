class HST_BuildInfo
{
	static const string BUILD_SHA = "a8e261d00e13ecc62cd974a0badb2f89eaa45918";
	static const string BUILD_UTC = "2026-07-18T00:30:10Z";
	static const string BUILD_LABEL = "schema71-settings24-controlled-shutdown-native-fence";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
