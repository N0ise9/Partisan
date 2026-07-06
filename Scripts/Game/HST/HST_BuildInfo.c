class HST_BuildInfo
{
	static const string BUILD_SHA = "r27-gm-faction-root-proof";
	static const string BUILD_UTC = "2026-07-06T16:00:13Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r27-gm-faction-root-proof";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
