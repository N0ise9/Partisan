class HST_EconomyService
{
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
		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		if (!pool)
			return;

		pool.m_iAggression = Math.Max(0, pool.m_iAggression + amount);
	}

	void RecalculateWarLevel(HST_CampaignState state, HST_BalanceConfig balance)
	{
		int ownedZones;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone.m_sOwnerFactionKey == "FIA")
				ownedZones++;
		}

		int nextWarLevel = 1 + ownedZones / 4;
		nextWarLevel = Math.Max(1, nextWarLevel);
		state.m_iWarLevel = Math.Min(balance.m_iWarLevelMaximum, nextWarLevel);
	}
}
