class HST_EnemyDirectorService
{
	static const int RESOURCE_TICK_SECONDS = 300;

	bool TickResources(HST_CampaignState state, HST_CampaignPreset preset, int elapsedSeconds)
	{
		if (!state || !preset || elapsedSeconds <= 0)
			return false;

		state.m_iEnemyResourceAccumulatorSeconds += elapsedSeconds;
		if (state.m_iEnemyResourceAccumulatorSeconds < RESOURCE_TICK_SECONDS)
			return false;

		state.m_iEnemyResourceAccumulatorSeconds -= RESOURCE_TICK_SECONDS;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
				continue;

			int attackIncome = Math.Max(1, zone.m_iIncomeValue / 50) + Math.Max(0, zone.m_iPriority / 8);
			int supportIncome = 1 + Math.Max(0, state.m_iWarLevel / 3);
			if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT || zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
				attackIncome += 2;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE && zone.m_sResourceKind == "fuel")
				attackIncome += 2;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE && zone.m_sResourceKind == "supplies")
				supportIncome += 2;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER || zone.m_eType == HST_EZoneType.HST_ZONE_POLICE_STATION)
				supportIncome += 1;

			if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
				supportIncome += Math.Max(0, 1 - zone.m_iSupport / 50);

			AddResources(state, zone.m_sOwnerFactionKey, attackIncome, supportIncome);
		}

		return true;
	}

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
