class HST_BuildInfo
{
	static const string BUILD_SHA = "r68-response-mixed-vehicle";
	static const string BUILD_UTC = "2026-07-07T22:35:00Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r68-response-mixed-vehicle";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
