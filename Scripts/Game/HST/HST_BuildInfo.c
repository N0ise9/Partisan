class HST_BuildInfo
{
	static const string BUILD_SHA = "541a79f7e5f49394c6f78a630d9e05340c8e2959";
	static const string BUILD_UTC = "2026-07-16T15:31:05Z";
	static const string BUILD_LABEL = "schema70-settings24-counterattack-ownership-pre-reconcile-fence";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
