class HST_BuildInfo
{
	static const string BUILD_SHA = "r82-location-qrf-marker-deconflict";
	static const string BUILD_UTC = "2026-07-07T23:47:06Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r82-location-qrf-marker-deconflict";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
