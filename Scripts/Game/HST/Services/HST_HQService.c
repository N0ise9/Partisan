class HST_HQService
{
	bool SelectInitialHideout(HST_CampaignState state, string hideoutId)
	{
		if (!state || state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_SETUP || !HST_DefaultCatalog.IsKnownHideout(hideoutId))
			return false;

		SetHQPosition(state, hideoutId, HST_DefaultCatalog.GetHideoutPosition(hideoutId));
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		return true;
	}

	bool MoveHQ(HST_CampaignState state, string hideoutId)
	{
		if (!state || state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE || !state.m_bPetrosAlive || !HST_DefaultCatalog.IsKnownHideout(hideoutId))
			return false;

		SetHQPosition(state, hideoutId, HST_DefaultCatalog.GetHideoutPosition(hideoutId));
		return true;
	}

	void OnPetrosKilled(HST_CampaignState state, HST_EconomyService economy, int moneyPenalty, int hrPenalty)
	{
		if (!state || !state.m_bPetrosAlive)
			return;

		state.m_bPetrosAlive = false;
		state.m_iPetrosDeaths++;
		economy.AddFactionMoney(state, -moneyPenalty);
		economy.AddHR(state, -hrPenalty);
	}

	protected void SetHQPosition(HST_CampaignState state, string hideoutId, vector hqPosition)
	{
		vector petrosOffset = "2 0 2";
		state.m_sHQHideoutId = hideoutId;
		state.m_vHQPosition = hqPosition;
		state.m_vPetrosPosition = hqPosition + petrosOffset;
		state.m_bHQDeployed = true;
		state.m_bPetrosAlive = true;
	}
}
