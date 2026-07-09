class HST_BuildInfo
{
	static const string BUILD_SHA = "r119-economy-income-source-report";
	static const string BUILD_UTC = "2026-07-09T03:12:34Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r119-economy-income-source-report";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
