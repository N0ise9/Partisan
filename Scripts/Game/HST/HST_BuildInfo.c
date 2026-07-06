class HST_BuildInfo
{
	static const string BUILD_SHA = "908fba8+runtimeproof";
	static const string BUILD_UTC = "2026-07-06T05:13:58Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r5";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
