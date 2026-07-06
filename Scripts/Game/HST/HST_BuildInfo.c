class HST_BuildInfo
{
	static const string BUILD_SHA = "r39-petros-world-primary-population";
	static const string BUILD_UTC = "2026-07-06T21:41:21Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r39-petros-world-primary-population";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
