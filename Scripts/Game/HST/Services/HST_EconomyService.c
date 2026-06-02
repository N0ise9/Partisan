class HST_EconomyService
{
	void AddFactionMoney(HST_CampaignState state, int amount)
	{
		state.m_iFactionMoney = Math.Max(0, state.m_iFactionMoney + amount);
	}

	void AddHR(HST_CampaignState state, int amount)
	{
		state.m_iHR = Math.Max(0, state.m_iHR + amount);
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
