class HST_BuildInfo
{
	static const string BUILD_SHA = "r110-town-security-pressure";
	static const string BUILD_UTC = "2026-07-08T17:53:45Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r110-town-security-pressure";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
