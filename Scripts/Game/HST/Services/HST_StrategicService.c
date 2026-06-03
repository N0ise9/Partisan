class HST_StrategicService
{
	bool SetZoneOwner(HST_CampaignState state, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, string factionKey)
	{
		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone || !state.FindFactionPool(factionKey) || zone.m_sOwnerFactionKey == factionKey)
			return false;

		zone.m_sOwnerFactionKey = factionKey;
		zone.m_iResistanceCaptureProgress = 0;
		zone.m_iActiveInfantryCount = 0;
		zone.m_iActiveVehicleCount = 0;
		economy.RecalculateWarLevel(state, balance);
		EvaluateCampaignOutcome(state);
		return true;
	}

	bool AddTownSupport(HST_CampaignState state, string zoneId, int amount)
	{
		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
			return false;

		zone.m_iSupport = Math.Max(-100, Math.Min(100, zone.m_iSupport + amount));
		return true;
	}

	bool SetZoneActive(HST_CampaignState state, string zoneId, bool active)
	{
		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
			return false;

		zone.m_bActive = active;
		return true;
	}

	void OnPetrosKilled(HST_CampaignState state)
	{
		state.m_bPetrosAlive = false;
		state.m_iFactionMoney = Math.Max(0, state.m_iFactionMoney / 2);
		state.m_iHR = Math.Max(0, state.m_iHR / 2);
	}

	protected void EvaluateCampaignOutcome(HST_CampaignState state)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone.m_sOwnerFactionKey != "FIA")
				return;
		}

		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_WON;
	}
}
