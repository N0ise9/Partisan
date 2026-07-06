class HST_BuildInfo
{
	static const string BUILD_SHA = "80f635b+scr-aigroup-compile-fix";
	static const string BUILD_UTC = "2026-07-06T13:48:00Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r18-scr-aigroup-compile-fix";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
