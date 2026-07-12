class HST_BuildInfo
{
	static const string BUILD_SHA = "7c93e0a485bcabe5a364c0b0cfeca235accb50f7";
	static const string BUILD_UTC = "2026-07-12T06:11:19Z";
	static const string BUILD_LABEL = "schema63-canonical-combat-presence";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
