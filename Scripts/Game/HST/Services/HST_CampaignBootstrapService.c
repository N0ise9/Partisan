// Single production factory for a brand-new campaign envelope. Both normal
// startup/admin reset and deterministic proof use this path so enemy authority
// cannot silently drift out of the state created before restore validation.
class HST_CampaignBootstrapService
{
	static HST_CampaignState CreateInitialCampaignState(
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		HST_RuntimeSettings settings)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		if (preset)
			state.m_sPresetId = preset.m_sPresetId;
		if (settings)
			state.m_iCampaignSeed = settings.m_Campaign.m_iCampaignSeed;
		if (balance)
		{
			state.m_iFactionMoney = balance.m_iStartingFactionMoney;
			state.m_iHR = balance.m_iStartingHR;
			state.m_iTrainingLevel = Math.Max(1, balance.m_iStartingTrainingLevel);
		}

		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_SETUP;
		state.m_sLastPersistenceStatus = "new campaign created";
		if (preset && balance)
			HST_DefaultCatalog.AddDefaultFactionPools(state, balance, preset);
		return state;
	}
}
