class HST_RecruitmentService
{
	bool TrainTroops(HST_CampaignState state, HST_EconomyService economy, int moneyCost)
	{
		if (!state || !economy || moneyCost < 0 || state.m_iTrainingLevel >= 10)
			return false;

		if (!economy.SpendFactionMoney(state, moneyCost))
			return false;

		state.m_iTrainingLevel++;
		return true;
	}

	bool RecruitGarrison(HST_CampaignState state, HST_EconomyService economy, HST_GarrisonService garrisons, string zoneId, string factionKey, int infantryCount, int vehicleCount, int moneyCost, int hrCost)
	{
		if (!state || !economy || !garrisons || infantryCount < 0 || vehicleCount < 0)
			return false;

		if (moneyCost < 0 || hrCost < 0 || state.m_iFactionMoney < moneyCost || state.m_iHR < hrCost)
			return false;

		if (!garrisons.AddAbstractForces(state, zoneId, factionKey, infantryCount, vehicleCount))
			return false;

		economy.SpendFactionMoney(state, moneyCost);
		economy.SpendHR(state, hrCost);
		return true;
	}
}
