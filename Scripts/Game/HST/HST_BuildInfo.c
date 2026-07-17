class HST_BuildInfo
{
	static const string BUILD_SHA = "34fcb8e77726beb61dfb10cf650183b5ef99542c";
	static const string BUILD_UTC = "2026-07-17T04:33:16Z";
	static const string BUILD_LABEL = "schema70-settings24-field-vehicle-restart";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
