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

	string BuildIncomeReport(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance)
	{
		if (!state || !preset || !balance)
			return "h-istasi income | state/preset/balance not ready";

		string report = string.Format(
			"h-istasi income | interval %1s | timer %2s | next money %3 | next HR %4",
			balance.m_iZoneIncomeIntervalSeconds,
			state.m_iIncomeAccumulatorSeconds,
			CalculateResistanceIncome(state, preset.m_sResistanceFactionKey),
			CalculateResistanceHRIncome(state, preset.m_sResistanceFactionKey)
		);

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
				continue;

			report = report + string.Format(
				"\n%1 | %2 | income %3 | support %4 | HR %5",
				zone.m_sZoneId,
				zone.m_eType,
				CalculateZoneMoneyIncome(zone),
				zone.m_iSupport,
				ResolveZoneHRIncome(zone)
			);
		}

		return report;
	}

	int CalculateResistanceIncome(HST_CampaignState state, string resistanceFactionKey)
	{
		int income;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey != resistanceFactionKey)
				continue;

			income += CalculateZoneMoneyIncome(zone);
		}

		return income;
	}

	int CalculateResistanceHRIncome(HST_CampaignState state, string resistanceFactionKey)
	{
		int income;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey != resistanceFactionKey)
				continue;

			income += ResolveZoneHRIncome(zone);
		}

		return income;
	}

	protected int ResolveZoneHRIncome(HST_ZoneState zone)
	{
		if (!zone)
			return 0;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN && zone.m_iSupport >= 25)
			return 1;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE && zone.m_sResourceKind == "food")
			return 1;
		return 0;
	}

	protected int CalculateZoneMoneyIncome(HST_ZoneState zone)
	{
		if (!zone)
			return 0;

		int income = zone.m_iIncomeValue;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			income += Math.Max(0, zone.m_iSupport / 12);
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			income += 12 + zone.m_iPriority / 2;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			income += 30 + zone.m_iPriority;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT || zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD)
			income += 45;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_BANK)
			income += 50;

		return income;
	}
}