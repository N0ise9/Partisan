class HST_BuildInfo
{
	static const string BUILD_SHA = "6a7943a37bd9338f176724718ec132ff108e9c82";
	static const string BUILD_UTC = "2026-07-15T12:54:09Z";
	static const string BUILD_LABEL = "schema70-settings24-spawn-adapter-proof-backing";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}

	static string BuildRuntimeSummary()
	{
		return BuildSummary() + string.Format(" | campaign schema %1 | settings schema %2", HST_CampaignState.SCHEMA_VERSION, HST_RuntimeSettings.SCHEMA_VERSION);
	}
}
