class HST_BuildInfo
{
	static const string BUILD_SHA = "r93-hq-knowledge-response-counts";
	static const string BUILD_UTC = "2026-07-08T14:35:00Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r93-hq-knowledge-response-counts";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
