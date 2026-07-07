class HST_BuildInfo
{
	static const string BUILD_SHA = "r42-infinite-stamina-setting";
	static const string BUILD_UTC = "2026-07-07T02:15:00Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r42-infinite-stamina-setting";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
