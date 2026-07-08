class HST_TownService
{
	static const float RADIO_TOWN_INFLUENCE_RADIUS_METERS = 1800.0;
	static const int RADIO_RESISTANCE_FIA_SUPPORT_DELTA = 2;
	static const int RADIO_RESISTANCE_OCCUPIER_SUPPORT_DELTA = -1;
	static const int RADIO_RESISTANCE_REPUTATION_DELTA = 1;
	static const int RADIO_RESISTANCE_HEAT_DELTA = -1;
	static const int RADIO_ENEMY_FIA_SUPPORT_DELTA = -1;
	static const int RADIO_ENEMY_OCCUPIER_SUPPORT_DELTA = 2;
	static const int RADIO_ENEMY_REPUTATION_DELTA = -1;
	static const int RADIO_ENEMY_HEAT_DELTA = 1;

	protected bool m_bLastRadioInfluenceChanged;

	int TickIncome(HST_CampaignState state, HST_EconomyService economy, HST_BalanceConfig balance, HST_CampaignPreset preset, int elapsedSeconds, HST_CivilianService civilians = null)
	{
		m_bLastRadioInfluenceChanged = false;
		if (!state || !economy || !balance || !preset || elapsedSeconds <= 0)
			return 0;

		state.m_iIncomeAccumulatorSeconds += elapsedSeconds;
		if (state.m_iIncomeAccumulatorSeconds < balance.m_iZoneIncomeIntervalSeconds)
			return 0;

		state.m_iIncomeAccumulatorSeconds -= balance.m_iZoneIncomeIntervalSeconds;
		int income = CalculateResistanceIncome(state, preset.m_sResistanceFactionKey);
		economy.AddFactionMoney(state, income);
		economy.AddHR(state, CalculateResistanceHRIncome(state, preset.m_sResistanceFactionKey));
		m_bLastRadioInfluenceChanged = ApplyRadioTowerInfluence(state, preset, civilians);
		return income;
	}

	int ApplyIncomeNow(HST_CampaignState state, HST_EconomyService economy, HST_CampaignPreset preset, HST_CivilianService civilians = null)
	{
		m_bLastRadioInfluenceChanged = false;
		if (!state || !economy || !preset)
			return 0;

		int income = CalculateResistanceIncome(state, preset.m_sResistanceFactionKey);
		economy.AddFactionMoney(state, income);
		economy.AddHR(state, CalculateResistanceHRIncome(state, preset.m_sResistanceFactionKey));
		m_bLastRadioInfluenceChanged = ApplyRadioTowerInfluence(state, preset, civilians);
		return income;
	}

	bool ConsumeRadioInfluenceChanged()
	{
		bool changed = m_bLastRadioInfluenceChanged;
		m_bLastRadioInfluenceChanged = false;
		return changed;
	}

	bool AddSupport(HST_CampaignState state, string zoneId, int amount)
	{
		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return false;

		zone.m_iSupport = Math.Max(-100, Math.Min(100, zone.m_iSupport + amount));
		return true;
	}

	string BuildIncomeReport(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance)
	{
		if (!state || !preset || !balance)
			return "h-istasi income | state/preset/balance not ready";

		string report = string.Format(
			"h-istasi income | interval %1s | timer %2s | next money %3 | next HR %4",
			balance.m_iZoneIncomeIntervalSeconds,
			state.m_iIncomeAccumulatorSeconds,
			CalculateResistanceIncome(state, preset.m_sResistanceFactionKey),
			CalculateResistanceHRIncome(state, preset.m_sResistanceFactionKey)
		);

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
				continue;

			report = report + string.Format(
				"\n%1 | %2 | income %3 | support %4 | HR %5",
				zone.m_sZoneId,
				zone.m_eType,
				CalculateZoneMoneyIncome(zone),
				zone.m_iSupport,
				ResolveZoneHRIncome(zone)
			);
		}

		report = report + string.Format("\nradio influence | radius %1m | one nearest owned tower per town per income tick", Math.Round(RADIO_TOWN_INFLUENCE_RADIUS_METERS));
		int radioRows;
		foreach (HST_ZoneState townZone : state.m_aZones)
		{
			if (!townZone || townZone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
				continue;

			HST_ZoneState radioZone = FindNearestEligibleRadioTower(state, preset, townZone);
			if (!radioZone)
				continue;

			radioRows++;
			string effect = "enemy pressure";
			if (HST_FactionRelationService.IsResistanceFaction(preset, radioZone.m_sOwnerFactionKey))
				effect = "resistance support";
			report = report + string.Format("\n%1 <- %2 | owner %3 | %4", townZone.m_sZoneId, radioZone.m_sZoneId, radioZone.m_sOwnerFactionKey, effect);
		}

		if (radioRows == 0)
			report = report + "\nno towns in radio influence range";

		return report;
	}

	int CalculateResistanceIncome(HST_CampaignState state, string resistanceFactionKey)
	{
		int income;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey != resistanceFactionKey)
				continue;

			income += CalculateZoneMoneyIncome(zone);
		}

		return income;
	}

	int CalculateResistanceHRIncome(HST_CampaignState state, string resistanceFactionKey)
	{
		int income;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey != resistanceFactionKey)
				continue;

			income += ResolveZoneHRIncome(zone);
		}

		return income;
	}

	protected int ResolveZoneHRIncome(HST_ZoneState zone)
	{
		if (!zone)
			return 0;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN && zone.m_iSupport >= 25)
			return 1;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE && zone.m_sResourceKind == "food")
			return 1;
		return 0;
	}

	protected int CalculateZoneMoneyIncome(HST_ZoneState zone)
	{
		if (!zone)
			return 0;

		int income = zone.m_iIncomeValue;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			income += Math.Max(0, zone.m_iSupport / 12);
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			income += 12 + zone.m_iPriority / 2;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			income += 30 + zone.m_iPriority;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT || zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD)
			income += 45;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_BANK)
			income += 50;

		return income;
	}

	protected bool ApplyRadioTowerInfluence(HST_CampaignState state, HST_CampaignPreset preset, HST_CivilianService civilians)
	{
		if (!state || !preset || !civilians)
			return false;

		civilians.EnsureCivilianZones(state);
		bool changed;
		foreach (HST_ZoneState townZone : state.m_aZones)
		{
			if (!townZone || townZone.m_eType != HST_EZoneType.HST_ZONE_TOWN || townZone.m_sZoneId.IsEmpty())
				continue;

			HST_CivilianZoneState civilianZone = state.FindCivilianZone(townZone.m_sZoneId);
			if (!civilianZone)
				continue;

			HST_ZoneState radioZone = FindNearestEligibleRadioTower(state, preset, townZone);
			if (!radioZone)
				continue;

			int fiaDelta;
			int occupierDelta;
			int reputationDelta;
			int heatDelta;
			string reason;
			if (HST_FactionRelationService.IsResistanceFaction(preset, radioZone.m_sOwnerFactionKey))
			{
				fiaDelta = RADIO_RESISTANCE_FIA_SUPPORT_DELTA;
				occupierDelta = RADIO_RESISTANCE_OCCUPIER_SUPPORT_DELTA;
				reputationDelta = RADIO_RESISTANCE_REPUTATION_DELTA;
				heatDelta = RADIO_RESISTANCE_HEAT_DELTA;
				reason = "resistance broadcast from " + ResolveRadioInfluenceLabel(radioZone);
			}
			else if (HST_FactionRelationService.IsEnemyFaction(preset, radioZone.m_sOwnerFactionKey))
			{
				fiaDelta = RADIO_ENEMY_FIA_SUPPORT_DELTA;
				occupierDelta = RADIO_ENEMY_OCCUPIER_SUPPORT_DELTA;
				reputationDelta = RADIO_ENEMY_REPUTATION_DELTA;
				heatDelta = RADIO_ENEMY_HEAT_DELTA;
				reason = "enemy broadcast from " + ResolveRadioInfluenceLabel(radioZone);
			}
			else
			{
				continue;
			}

			if (!WouldRadioInfluenceChange(civilianZone, fiaDelta, occupierDelta, reputationDelta, heatDelta))
				continue;

			if (civilians.RegisterInfluenceEvent(state, townZone.m_sZoneId, "radio_broadcast", fiaDelta, occupierDelta, reputationDelta, heatDelta, 0, 0, 0, reason, preset, 0, radioZone.m_sZoneId))
				changed = true;
		}

		return changed;
	}

	protected HST_ZoneState FindNearestEligibleRadioTower(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState townZone)
	{
		if (!state || !preset || !townZone)
			return null;

		float radiusSq = RADIO_TOWN_INFLUENCE_RADIUS_METERS * RADIO_TOWN_INFLUENCE_RADIUS_METERS;
		float bestSq = radiusSq + 1.0;
		HST_ZoneState bestRadio;
		foreach (HST_ZoneState radioZone : state.m_aZones)
		{
			if (!radioZone || radioZone.m_eType != HST_EZoneType.HST_ZONE_RADIO_TOWER)
				continue;
			if (!HST_FactionRelationService.IsResistanceFaction(preset, radioZone.m_sOwnerFactionKey) && !HST_FactionRelationService.IsEnemyFaction(preset, radioZone.m_sOwnerFactionKey))
				continue;

			float distanceSq = DistanceSq2D(townZone.m_vPosition, radioZone.m_vPosition);
			if (distanceSq > radiusSq || distanceSq >= bestSq)
				continue;

			bestSq = distanceSq;
			bestRadio = radioZone;
		}

		return bestRadio;
	}

	protected bool WouldRadioInfluenceChange(HST_CivilianZoneState civilianZone, int fiaDelta, int occupierDelta, int reputationDelta, int heatDelta)
	{
		if (!civilianZone)
			return false;

		int nextFIA = Math.Max(0, Math.Min(100, civilianZone.m_iFIASupport + fiaDelta));
		int nextOccupier = Math.Max(0, Math.Min(100, civilianZone.m_iOccupierSupport + occupierDelta));
		int nextReputation = Math.Max(0, Math.Min(100, civilianZone.m_iReputation + reputationDelta));
		int nextHeat = Math.Max(0, civilianZone.m_iWantedHeat + heatDelta);
		return nextFIA != civilianZone.m_iFIASupport || nextOccupier != civilianZone.m_iOccupierSupport || nextReputation != civilianZone.m_iReputation || nextHeat != civilianZone.m_iWantedHeat;
	}

	protected string ResolveRadioInfluenceLabel(HST_ZoneState radioZone)
	{
		if (!radioZone)
			return "radio tower";
		if (!radioZone.m_sDisplayName.IsEmpty())
			return radioZone.m_sDisplayName;
		if (!radioZone.m_sZoneId.IsEmpty())
			return radioZone.m_sZoneId;
		return "radio tower";
	}

	protected float DistanceSq2D(vector first, vector second)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz;
	}
}
