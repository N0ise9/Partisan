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
	static const int SECURITY_PRESSURE_MAX_POLICE = 5;
	static const int SECURITY_PRESSURE_MAX_ROADBLOCKS = 4;

	protected bool m_bLastPeriodicInfluenceChanged;

	int TickIncome(HST_CampaignState state, HST_EconomyService economy, HST_BalanceConfig balance, HST_CampaignPreset preset, int elapsedSeconds, HST_CivilianService civilians = null)
	{
		m_bLastPeriodicInfluenceChanged = false;
		if (!state || !economy || !balance || !preset || elapsedSeconds <= 0)
			return 0;

		state.m_iIncomeAccumulatorSeconds += elapsedSeconds;
		if (state.m_iIncomeAccumulatorSeconds < balance.m_iZoneIncomeIntervalSeconds)
			return 0;

		state.m_iIncomeAccumulatorSeconds -= balance.m_iZoneIncomeIntervalSeconds;
		int income = CalculateResistanceIncome(state, preset.m_sResistanceFactionKey);
		economy.AddFactionMoney(state, income);
		economy.AddHR(state, CalculateResistanceHRIncome(state, preset.m_sResistanceFactionKey));
		m_bLastPeriodicInfluenceChanged = ApplyPeriodicTownInfluence(state, preset, civilians);
		return income;
	}

	int ApplyIncomeNow(HST_CampaignState state, HST_EconomyService economy, HST_CampaignPreset preset, HST_CivilianService civilians = null)
	{
		m_bLastPeriodicInfluenceChanged = false;
		if (!state || !economy || !preset)
			return 0;

		int income = CalculateResistanceIncome(state, preset.m_sResistanceFactionKey);
		economy.AddFactionMoney(state, income);
		economy.AddHR(state, CalculateResistanceHRIncome(state, preset.m_sResistanceFactionKey));
		m_bLastPeriodicInfluenceChanged = ApplyPeriodicTownInfluence(state, preset, civilians);
		return income;
	}

	bool ConsumePeriodicTownInfluenceChanged()
	{
		bool changed = m_bLastPeriodicInfluenceChanged;
		m_bLastPeriodicInfluenceChanged = false;
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
		report = report + "\n" + BuildIncomeSourceBreakdown(state, preset);

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

		report = report + string.Format(
			"\nsecurity pressure | max police %1 | max roadblocks %2 | one-step drift per income tick",
			SECURITY_PRESSURE_MAX_POLICE,
			SECURITY_PRESSURE_MAX_ROADBLOCKS
		);
		int securityRows;
		foreach (HST_CivilianZoneState civilianZone : state.m_aCivilianZones)
		{
			if (!civilianZone)
				continue;

			HST_ZoneState town = state.FindZone(civilianZone.m_sZoneId);
			if (!town || town.m_eType != HST_EZoneType.HST_ZONE_TOWN)
				continue;

			int targetPolice = ResolveTargetPolicePresence(state, preset, town, civilianZone);
			int targetRoadblocks = ResolveTargetRoadblockPresence(state, preset, town, civilianZone);
			if (targetPolice == civilianZone.m_iPolicePresence && targetRoadblocks == civilianZone.m_iRoadblockPresence)
				continue;

			securityRows++;
			report = report + string.Format(
				"\n%1 | police %2 -> %3 | roadblocks %4 -> %5",
				town.m_sZoneId,
				civilianZone.m_iPolicePresence,
				targetPolice,
				civilianZone.m_iRoadblockPresence,
				targetRoadblocks
			);
		}

		if (securityRows == 0)
			report = report + "\nsecurity pressure already settled";

		return report;
	}

	string BuildIncomeSourceBreakdown(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return "income sources | state/preset not ready";

		int townMoney;
		int townHR;
		int townZones;
		int resourceMoney;
		int resourceHR;
		int resourceZones;
		int factoryMoney;
		int factoryHR;
		int factoryZones;
		int seaportMoney;
		int seaportHR;
		int seaportZones;
		int airfieldMoney;
		int airfieldHR;
		int airfieldZones;
		int bankMoney;
		int bankHR;
		int bankZones;
		int otherMoney;
		int otherHR;
		int otherZones;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
				continue;

			int money = CalculateZoneMoneyIncome(zone);
			int hr = ResolveZoneHRIncome(zone);
			if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			{
				townMoney += money;
				townHR += hr;
				townZones++;
			}
			else if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			{
				resourceMoney += money;
				resourceHR += hr;
				resourceZones++;
			}
			else if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			{
				factoryMoney += money;
				factoryHR += hr;
				factoryZones++;
			}
			else if (zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			{
				seaportMoney += money;
				seaportHR += hr;
				seaportZones++;
			}
			else if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD)
			{
				airfieldMoney += money;
				airfieldHR += hr;
				airfieldZones++;
			}
			else if (zone.m_eType == HST_EZoneType.HST_ZONE_BANK)
			{
				bankMoney += money;
				bankHR += hr;
				bankZones++;
			}
			else
			{
				otherMoney += money;
				otherHR += hr;
				otherZones++;
			}
		}

		string report = string.Format("income sources | towns money %1 HR %2 zones %3", townMoney, townHR, townZones);
		report = report + string.Format(" | resources money %1 HR %2 zones %3", resourceMoney, resourceHR, resourceZones);
		report = report + string.Format(" | factories money %1 HR %2 zones %3", factoryMoney, factoryHR, factoryZones);
		report = report + string.Format(" | seaports money %1 HR %2 zones %3", seaportMoney, seaportHR, seaportZones);
		report = report + string.Format(" | airfields money %1 HR %2 zones %3", airfieldMoney, airfieldHR, airfieldZones);
		report = report + string.Format(" | banks money %1 HR %2 zones %3", bankMoney, bankHR, bankZones);
		report = report + string.Format(" | other money %1 HR %2 zones %3", otherMoney, otherHR, otherZones);
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

	protected bool ApplyPeriodicTownInfluence(HST_CampaignState state, HST_CampaignPreset preset, HST_CivilianService civilians)
	{
		if (!state)
			return false;
		if (!preset)
			return false;
		if (!civilians)
			return false;

		civilians.EnsureCivilianZones(state);
		bool radioChanged = ApplyRadioTowerInfluence(state, preset, civilians);
		bool securityChanged = ApplyTownSecurityPressure(state, preset, civilians);
		return radioChanged || securityChanged;
	}

	protected bool ApplyRadioTowerInfluence(HST_CampaignState state, HST_CampaignPreset preset, HST_CivilianService civilians)
	{
		if (!state)
			return false;
		if (!preset)
			return false;
		if (!civilians)
			return false;

		bool changed;
		foreach (HST_ZoneState townZone : state.m_aZones)
		{
			if (!townZone)
				continue;
			if (townZone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
				continue;
			if (townZone.m_sZoneId.IsEmpty())
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

	protected bool ApplyTownSecurityPressure(HST_CampaignState state, HST_CampaignPreset preset, HST_CivilianService civilians)
	{
		if (!state)
			return false;
		if (!preset)
			return false;
		if (!civilians)
			return false;

		bool changed;
		foreach (HST_CivilianZoneState civilianZone : state.m_aCivilianZones)
		{
			if (!civilianZone || civilianZone.m_sZoneId.IsEmpty())
				continue;

			HST_ZoneState townZone = state.FindZone(civilianZone.m_sZoneId);
			if (!townZone || townZone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
				continue;

			int targetPolice = ResolveTargetPolicePresence(state, preset, townZone, civilianZone);
			int targetRoadblocks = ResolveTargetRoadblockPresence(state, preset, townZone, civilianZone);
			int policeDelta = ClampOneStepDelta(targetPolice - civilianZone.m_iPolicePresence);
			int roadblockDelta = ClampOneStepDelta(targetRoadblocks - civilianZone.m_iRoadblockPresence);
			if (policeDelta == 0 && roadblockDelta == 0)
				continue;

			string reason = BuildSecurityPressureReason(townZone, civilianZone, targetPolice, targetRoadblocks);
			if (civilians.RegisterInfluenceEvent(state, townZone.m_sZoneId, "security_pressure", 0, 0, 0, 0, 0, policeDelta, roadblockDelta, reason, preset, 0, townZone.m_sZoneId))
				changed = true;
		}

		return changed;
	}

	protected HST_ZoneState FindNearestEligibleRadioTower(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState townZone)
	{
		if (!state)
			return null;
		if (!preset)
			return null;
		if (!townZone)
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
		if (nextFIA != civilianZone.m_iFIASupport)
			return true;
		if (nextOccupier != civilianZone.m_iOccupierSupport)
			return true;
		if (nextReputation != civilianZone.m_iReputation)
			return true;
		if (nextHeat != civilianZone.m_iWantedHeat)
			return true;
		return false;
	}

	protected int ResolveTargetPolicePresence(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState townZone, HST_CivilianZoneState civilianZone)
	{
		if (!state)
			return 0;
		if (!preset)
			return 0;
		if (!townZone)
			return 0;
		if (!civilianZone)
			return 0;

		if (HST_FactionRelationService.IsResistanceFaction(preset, townZone.m_sOwnerFactionKey))
		{
			if (civilianZone.m_iWantedHeat >= 12)
				return 1;
			return 0;
		}

		if (!HST_FactionRelationService.IsEnemyFaction(preset, townZone.m_sOwnerFactionKey))
			return Math.Max(0, Math.Min(SECURITY_PRESSURE_MAX_POLICE, civilianZone.m_iPolicePresence));

		int target = 1;
		target += Math.Min(2, Math.Max(0, civilianZone.m_iWantedHeat / 6));
		int occupierMargin = civilianZone.m_iOccupierSupport - civilianZone.m_iFIASupport;
		if (occupierMargin >= 20)
			target++;
		if (state.m_iWarLevel >= 4)
			target++;
		if (state.m_iWarLevel >= 7 && civilianZone.m_iWantedHeat >= 6)
			target++;
		return Math.Max(0, Math.Min(SECURITY_PRESSURE_MAX_POLICE, target));
	}

	protected int ResolveTargetRoadblockPresence(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState townZone, HST_CivilianZoneState civilianZone)
	{
		if (!state)
			return 0;
		if (!preset)
			return 0;
		if (!townZone)
			return 0;
		if (!civilianZone)
			return 0;

		if (HST_FactionRelationService.IsResistanceFaction(preset, townZone.m_sOwnerFactionKey))
		{
			if (civilianZone.m_iWantedHeat >= 15)
				return 1;
			return 0;
		}

		if (!HST_FactionRelationService.IsEnemyFaction(preset, townZone.m_sOwnerFactionKey))
			return Math.Max(0, Math.Min(SECURITY_PRESSURE_MAX_ROADBLOCKS, civilianZone.m_iRoadblockPresence));

		int target;
		if (civilianZone.m_iWantedHeat >= 4)
			target++;
		if (civilianZone.m_iWantedHeat >= 12)
			target++;
		int occupierMargin = civilianZone.m_iOccupierSupport - civilianZone.m_iFIASupport;
		if (occupierMargin >= 25)
			target++;
		if (state.m_iWarLevel >= 5 && civilianZone.m_iWantedHeat >= 6)
			target++;
		return Math.Max(0, Math.Min(SECURITY_PRESSURE_MAX_ROADBLOCKS, target));
	}

	protected int ClampOneStepDelta(int delta)
	{
		if (delta > 0)
			return 1;
		if (delta < 0)
			return -1;
		return 0;
	}

	protected string BuildSecurityPressureReason(HST_ZoneState townZone, HST_CivilianZoneState civilianZone, int targetPolice, int targetRoadblocks)
	{
		string label = "town";
		if (townZone && !townZone.m_sZoneId.IsEmpty())
			label = townZone.m_sZoneId;
		if (townZone && !townZone.m_sDisplayName.IsEmpty())
			label = townZone.m_sDisplayName;

		return string.Format(
			"security pressure in %1 | heat %2 | FIA %3 | occupier %4 | target police %5 roadblocks %6",
			label,
			civilianZone.m_iWantedHeat,
			civilianZone.m_iFIASupport,
			civilianZone.m_iOccupierSupport,
			targetPolice,
			targetRoadblocks
		);
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
