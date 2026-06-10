class HST_EconomyService
{
	static const int AGGRESSION_DECAY_SECONDS = 300;

	void AddFactionMoney(HST_CampaignState state, int amount)
	{
		state.m_iFactionMoney = Math.Max(0, state.m_iFactionMoney + amount);
	}

	bool SpendFactionMoney(HST_CampaignState state, int amount)
	{
		if (!state || amount < 0 || state.m_iFactionMoney < amount)
			return false;

		state.m_iFactionMoney -= amount;
		return true;
	}

	void AddHR(HST_CampaignState state, int amount)
	{
		state.m_iHR = Math.Max(0, state.m_iHR + amount);
	}

	bool SpendHR(HST_CampaignState state, int amount)
	{
		if (!state || amount < 0 || state.m_iHR < amount)
			return false;

		state.m_iHR -= amount;
		return true;
	}

	void AddAggression(HST_CampaignState state, string factionKey, int amount)
	{
		if (!state || factionKey.IsEmpty() || amount == 0)
			return;

		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		if (!pool)
			return;

		pool.m_iAggression = Math.Max(0, pool.m_iAggression + amount);
	}

	bool TickAggressionDecay(HST_CampaignState state, HST_CampaignPreset preset, int elapsedSeconds)
	{
		if (!state || !preset || elapsedSeconds <= 0)
			return false;

		state.m_iAggressionAccumulatorSeconds += elapsedSeconds;
		if (state.m_iAggressionAccumulatorSeconds < AGGRESSION_DECAY_SECONDS)
			return false;

		int decaySteps = state.m_iAggressionAccumulatorSeconds / AGGRESSION_DECAY_SECONDS;
		state.m_iAggressionAccumulatorSeconds = state.m_iAggressionAccumulatorSeconds % AGGRESSION_DECAY_SECONDS;

		bool changed;
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || pool.m_sFactionKey == preset.m_sResistanceFactionKey || pool.m_iAggression <= 0)
				continue;

			int nextAggression = Math.Max(0, pool.m_iAggression - decaySteps);
			if (nextAggression == pool.m_iAggression)
				continue;

			pool.m_iAggression = nextAggression;
			changed = true;
		}

		return changed;
	}

	void RecalculateWarLevel(HST_CampaignState state, HST_BalanceConfig balance, string resistanceFactionKey = "FIA")
	{
		int ownedWeight;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey != resistanceFactionKey)
				continue;

			ownedWeight += Math.Max(1, zone.m_iPriority);
			if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
				ownedWeight += 8;
			else if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
				ownedWeight += 5;
			else if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
				ownedWeight += 3;
		}

		int nextWarLevel = 1 + ownedWeight / 28;
		nextWarLevel = Math.Max(1, nextWarLevel);
		state.m_iWarLevel = Math.Min(balance.m_iWarLevelMaximum, nextWarLevel);
	}
}
