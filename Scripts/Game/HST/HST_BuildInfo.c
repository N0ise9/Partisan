class HST_BuildInfo
{
	static const string BUILD_SHA = "10c182e+physicalwar-compile-fix";
	static const string BUILD_UTC = "2026-07-06T13:39:00Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r17-physicalwar-compile-fix";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
