class HST_ZoneCaptureService
{
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
}
