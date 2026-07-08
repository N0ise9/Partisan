class HST_BuildInfo
{
	static const string BUILD_SHA = "r92-hq-pressure-response-pacing";
	static const string BUILD_UTC = "2026-07-08T13:55:00Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r92-hq-pressure-response-pacing";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
