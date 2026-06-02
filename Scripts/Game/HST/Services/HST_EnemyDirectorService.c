class HST_EnemyDirectorService
{
	bool CanAfford(HST_CampaignState state, string factionKey, int attackCost, int supportCost)
	{
		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		return pool && pool.m_iAttackResources >= attackCost && pool.m_iSupportResources >= supportCost;
	}

	bool TrySpend(HST_CampaignState state, string factionKey, int attackCost, int supportCost)
	{
		if (attackCost < 0 || supportCost < 0 || !CanAfford(state, factionKey, attackCost, supportCost))
			return false;

		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		pool.m_iAttackResources -= attackCost;
		pool.m_iSupportResources -= supportCost;
		return true;
	}

	void AddResources(HST_CampaignState state, string factionKey, int attackResources, int supportResources)
	{
		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		if (!pool)
			return;

		pool.m_iAttackResources = Math.Max(0, pool.m_iAttackResources + attackResources);
		pool.m_iSupportResources = Math.Max(0, pool.m_iSupportResources + supportResources);
	}
}

