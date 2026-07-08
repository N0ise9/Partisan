class HST_BuildInfo
{
	static const string BUILD_SHA = "r85-hide-hq-move-menu-actions";
	static const string BUILD_UTC = "2026-07-08T05:22:00Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r85-hide-hq-move-menu-actions";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
