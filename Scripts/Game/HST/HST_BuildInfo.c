class HST_BuildInfo
{
	static const string BUILD_SHA = "0b380f00fde65c4f2e22858faf8ddc6eab794131";
	static const string BUILD_UTC = "2026-07-14T17:40:21Z";
	static const string BUILD_LABEL = "schema70-settings24-spawn-queue-resume";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
