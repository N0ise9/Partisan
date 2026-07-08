class HST_BuildInfo
{
	static const string BUILD_SHA = "r107-transfer-choice-proof";
	static const string BUILD_UTC = "2026-07-08T23:45:00Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r107-transfer-choice-proof";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
