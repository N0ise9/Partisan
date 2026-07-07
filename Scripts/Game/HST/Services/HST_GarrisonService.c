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

	bool RemoveAbstractForces(HST_CampaignState state, string zoneId, string factionKey, int infantryCount, int vehicleCount)
	{
		if (!state || zoneId.IsEmpty() || factionKey.IsEmpty())
			return false;

		if (infantryCount < 0 || vehicleCount < 0)
			return false;

		HST_GarrisonState garrison = state.FindGarrison(zoneId, factionKey);
		if (!garrison)
			return false;

		int beforeInfantry = garrison.m_iInfantryCount;
		int beforeVehicles = garrison.m_iVehicleCount;

		garrison.m_iInfantryCount = Math.Max(0, garrison.m_iInfantryCount - infantryCount);
		garrison.m_iVehicleCount = Math.Max(0, garrison.m_iVehicleCount - vehicleCount);

		return garrison.m_iInfantryCount != beforeInfantry || garrison.m_iVehicleCount != beforeVehicles;
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

	HST_ForceCompositionResult ComposeGarrisonForce(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState zone, HST_GarrisonState garrison, HST_ForceCompositionService forceCompositions = null)
	{
		HST_ForceCompositionService compositions = forceCompositions;
		if (!compositions)
			compositions = new HST_ForceCompositionService();

		return compositions.Compose(state, preset, BuildGarrisonForceRequest(state, preset, zone, garrison));
	}

	HST_ForceRequest BuildGarrisonForceRequest(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState zone, HST_GarrisonState garrison)
	{
		HST_ForceRequest request = new HST_ForceRequest();
		if (!state || !zone || !garrison)
			return request;

		request.m_sRequestId = "garrison_" + garrison.m_sZoneId + "_" + garrison.m_sFactionKey;
		request.m_sFactionKey = garrison.m_sFactionKey;
		request.m_sIntentId = HST_ForceCompositionService.INTENT_GARRISON;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			request.m_sIntentId = HST_ForceCompositionService.INTENT_TOWN_POLICE;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST || zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			request.m_sIntentId = HST_ForceCompositionService.INTENT_CHECKPOINT;
		request.m_sTargetZoneId = zone.m_sZoneId;
		request.m_vTargetPosition = zone.m_vPosition;
		request.m_iWarLevel = Math.Max(1, state.m_iWarLevel);
		request.m_iMinManpower = Math.Max(1, Math.Min(4, garrison.m_iInfantryCount));
		request.m_iMaxManpower = Math.Max(request.m_iMinManpower, Math.Max(4, garrison.m_iInfantryCount));
		request.m_iBudget = Math.Max(16, request.m_iMaxManpower * 3 + Math.Max(0, garrison.m_iVehicleCount) * 12);
		request.m_bAllowInfantry = true;
		request.m_bAllowVehicles = garrison.m_iVehicleCount > 0;
		request.m_bAllowArmedVehicles = request.m_bAllowVehicles && state.m_iWarLevel >= 5;
		request.m_bAllowLightArmor = request.m_bAllowVehicles && state.m_iWarLevel >= 6;
		request.m_bExplain = true;
		request.m_sReason = "garrison activation";
		return request;
	}
}
