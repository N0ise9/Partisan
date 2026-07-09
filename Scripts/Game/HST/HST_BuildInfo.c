class HST_BuildInfo
{
	static const string BUILD_SHA = "r117-marker-group-civilian-cleanup";
	static const string BUILD_UTC = "2026-07-09T02:46:51Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r117-marker-group-civilian-cleanup";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
