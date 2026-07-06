class HST_BuildInfo
{
	static const string BUILD_SHA = "r22-global-primary-method-proof";
	static const string BUILD_UTC = "2026-07-06T14:42:19Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r22-global-primary-method-proof";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
