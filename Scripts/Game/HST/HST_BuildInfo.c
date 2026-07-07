class HST_BuildInfo
{
	static const string BUILD_SHA = "r57-enemy-order-resolution-proof";
	static const string BUILD_UTC = "2026-07-07T17:45:00Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r57-enemy-order-resolution-proof";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
