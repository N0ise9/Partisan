class HST_BuildInfo
{
	static const string BUILD_SHA = "r96-mission-expiry-strategic-event";
	static const string BUILD_UTC = "2026-07-08T16:10:00Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r96-mission-expiry-strategic-event";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
