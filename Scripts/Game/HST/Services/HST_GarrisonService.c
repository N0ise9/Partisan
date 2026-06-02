class HST_GarrisonService
{
	HST_GarrisonState FindOrCreate(HST_CampaignState state, string zoneId, string factionKey)
	{
		if (!state || !state.FindZone(zoneId) || !state.FindFactionPool(factionKey))
			return null;

		HST_GarrisonState garrison = state.FindGarrison(zoneId, factionKey);
		if (garrison)
			return garrison;

		garrison = new HST_GarrisonState();
		garrison.m_sZoneId = zoneId;
		garrison.m_sFactionKey = factionKey;
		state.m_aGarrisons.Insert(garrison);
		return garrison;
	}

	bool AddAbstractForces(HST_CampaignState state, string zoneId, string factionKey, int infantryCount, int vehicleCount)
	{
		if (infantryCount < 0 || vehicleCount < 0)
			return false;

		HST_GarrisonState garrison = FindOrCreate(state, zoneId, factionKey);
		HST_ZoneState zone = state.FindZone(zoneId);
		if (!garrison || !zone)
			return false;

		int nextInfantry = garrison.m_iInfantryCount + infantryCount;
		if (zone.m_iGarrisonSlots > 0)
			nextInfantry = Math.Min(zone.m_iGarrisonSlots, nextInfantry);

		garrison.m_iInfantryCount = nextInfantry;
		garrison.m_iVehicleCount = Math.Max(0, garrison.m_iVehicleCount + vehicleCount);
		return true;
	}

	bool FoldSurvivors(HST_CampaignState state, string zoneId, string factionKey, int infantryCount, int vehicleCount)
	{
		if (infantryCount < 0 || vehicleCount < 0)
			return false;

		HST_GarrisonState garrison = FindOrCreate(state, zoneId, factionKey);
		if (!garrison)
			return false;

		garrison.m_iInfantryCount = infantryCount;
		garrison.m_iVehicleCount = vehicleCount;
		return true;
	}
}
