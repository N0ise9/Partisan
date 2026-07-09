class HST_BuildInfo
{
	static const string BUILD_SHA = "r116-police-presence-projections";
	static const string BUILD_UTC = "2026-07-08T19:56:38Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r116-police-presence-projections";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
