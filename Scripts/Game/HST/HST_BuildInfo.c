class HST_BuildInfo
{
	static const string BUILD_SHA = "r122-training-quality";
	static const string BUILD_UTC = "2026-07-09T03:52:00Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r122-training-quality";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
