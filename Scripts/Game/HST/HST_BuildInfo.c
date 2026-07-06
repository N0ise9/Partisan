class HST_BuildInfo
{
	static const string BUILD_SHA = "r25-group-faction-broadcast-proof";
	static const string BUILD_UTC = "2026-07-06T15:38:44Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r25-group-faction-broadcast-proof";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
