class HST_BuildInfo
{
	static const string BUILD_SHA = "cefbb8a+proofpolicy";
	static const string BUILD_UTC = "2026-07-06T03:10:00Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r3";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
