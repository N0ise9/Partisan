class HST_BuildInfo
{
	static const string BUILD_SHA = "22ca457+uirenderproof";
	static const string BUILD_UTC = "2026-07-06T13:05:16Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r14-ui-render-proof";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
