class HST_BuildInfo
{
	static const string BUILD_SHA = "451cf76+compilefix";
	static const string BUILD_UTC = "2026-07-06T05:21:21Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r6-compilefix";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
