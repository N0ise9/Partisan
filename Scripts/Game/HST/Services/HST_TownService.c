class HST_TownService
{
	int TickIncome(HST_CampaignState state, HST_EconomyService economy, HST_BalanceConfig balance, HST_CampaignPreset preset, int elapsedSeconds)
	{
		if (!state || !economy || !balance || !preset || elapsedSeconds <= 0)
			return 0;

		state.m_iIncomeAccumulatorSeconds += elapsedSeconds;
		if (state.m_iIncomeAccumulatorSeconds < balance.m_iZoneIncomeIntervalSeconds)
			return 0;

		state.m_iIncomeAccumulatorSeconds -= balance.m_iZoneIncomeIntervalSeconds;
		int income = CalculateResistanceIncome(state, preset.m_sResistanceFactionKey);
		economy.AddFactionMoney(state, income);
		economy.AddHR(state, CalculateResistanceHRIncome(state, preset.m_sResistanceFactionKey));
		return income;
	}

	int ApplyIncomeNow(HST_CampaignState state, HST_EconomyService economy, HST_CampaignPreset preset)
	{
		if (!state || !economy || !preset)
			return 0;

		int income = CalculateResistanceIncome(state, preset.m_sResistanceFactionKey);
		economy.AddFactionMoney(state, income);
		economy.AddHR(state, CalculateResistanceHRIncome(state, preset.m_sResistanceFactionKey));
		return income;
	}

	bool AddSupport(HST_CampaignState state, string zoneId, int amount)
	{
		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return false;

		zone.m_iSupport = Math.Max(-100, Math.Min(100, zone.m_iSupport + amount));
		return true;
	}

	protected int CalculateResistanceIncome(HST_CampaignState state, string resistanceFactionKey)
	{
		int income;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone.m_sOwnerFactionKey != resistanceFactionKey)
				continue;

			income += zone.m_iIncomeValue;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN && zone.m_iSupport > 0)
				income += zone.m_iSupport / 10;
		}

		return income;
	}

	protected int CalculateResistanceHRIncome(HST_CampaignState state, string resistanceFactionKey)
	{
		int income;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone.m_sOwnerFactionKey == resistanceFactionKey && zone.m_eType == HST_EZoneType.HST_ZONE_TOWN && zone.m_iSupport >= 25)
				income++;
		}

		return income;
	}
}
