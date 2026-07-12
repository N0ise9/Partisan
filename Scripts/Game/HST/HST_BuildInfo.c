class HST_BuildInfo
{
	static const string BUILD_SHA = "85a75c65e9c148a890d8d78b0288ae6483a5ccd9";
	static const string BUILD_UTC = "2026-07-12T08:22:05Z";
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
