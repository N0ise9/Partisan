class HST_BuildInfo
{
	static const string BUILD_SHA = "2efb6ce+petrosrefresh";
	static const string BUILD_UTC = "2026-07-06T05:27:35Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r8-petrosrefresh";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
