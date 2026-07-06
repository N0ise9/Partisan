class HST_BuildInfo
{
	static const string BUILD_SHA = "1986b57+directproof";
	static const string BUILD_UTC = "2026-07-06T05:41:08Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r10-directproof";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
