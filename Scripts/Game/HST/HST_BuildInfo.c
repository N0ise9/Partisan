class HST_BuildInfo
{
	static const string BUILD_SHA = "r113-enemy-local-front-gate";
	static const string BUILD_UTC = "2026-07-08T19:18:04Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r113-enemy-local-front-gate";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
