class HST_BuildInfo
{
	static const string BUILD_SHA = "r33-primary-group-post-pop-faction";
	static const string BUILD_UTC = "2026-07-06T20:31:19Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r33-primary-group-post-pop-faction";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
