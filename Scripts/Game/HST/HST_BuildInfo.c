class HST_BuildInfo
{
	static const string BUILD_SHA = "fb7226a+petrosfaction";
	static const string BUILD_UTC = "2026-07-06T12:45:58Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r11-petros-faction";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
