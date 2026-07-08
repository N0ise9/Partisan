class HST_BuildInfo
{
	static const string BUILD_SHA = "r114-undercover-live-equipment-gates";
	static const string BUILD_UTC = "2026-07-08T19:34:01Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r114-undercover-live-equipment-gates";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
