class HST_BuildInfo
{
	static const string BUILD_SHA = "r80-relation-unclaimed-vehicle-proof";
	static const string BUILD_UTC = "2026-07-07T23:33:33Z";
	static const string BUILD_LABEL = "h-istasi-live-runtime-proof-r80-relation-unclaimed-vehicle-proof";

	static string BuildSummary()
	{
		return string.Format("sha %1 | utc %2 | label %3", BUILD_SHA, BUILD_UTC, BUILD_LABEL);
	}
}
