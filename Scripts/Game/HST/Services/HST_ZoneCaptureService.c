class HST_ZoneCaptureService
{
	static const int CAPTURE_PROGRESS_REQUIRED = 100;

	bool CaptureZone(HST_CampaignState state, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, string factionKey)
	{
		if (!state || !strategic || !economy || !balance)
			return false;

		return strategic.SetZoneOwner(state, economy, balance, zoneId, factionKey);
	}

	bool CaptureForResistance(HST_CampaignState state, HST_CampaignPreset preset, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, int supportReward)
	{
		if (!preset || !CaptureZone(state, strategic, economy, balance, zoneId, preset.m_sResistanceFactionKey))
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (zone && zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			zone.m_iSupport = Math.Max(-100, Math.Min(100, zone.m_iSupport + supportReward));

		return true;
	}

	bool AddResistanceCaptureProgress(HST_CampaignState state, HST_CampaignPreset preset, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, int amount, int supportReward = 10)
	{
		if (!state || !preset || amount <= 0)
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone || zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
			return false;

		zone.m_iResistanceCaptureProgress = Math.Min(CAPTURE_PROGRESS_REQUIRED, Math.Max(0, zone.m_iResistanceCaptureProgress + amount));
		Print(string.Format("h-istasi | zone %1 resistance capture progress %2/%3", zoneId, zone.m_iResistanceCaptureProgress, CAPTURE_PROGRESS_REQUIRED));
		if (zone.m_iResistanceCaptureProgress < CAPTURE_PROGRESS_REQUIRED)
			return true;

		return CaptureForResistance(state, preset, strategic, economy, balance, zoneId, supportReward);
	}
}
