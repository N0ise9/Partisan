class HST_BuildInfo
{
	static const string BUILD_SHA = "r109-radio-town-influence";
	static const string BUILD_UTC = "2026-07-08T17:46:36Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r109-radio-town-influence";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
