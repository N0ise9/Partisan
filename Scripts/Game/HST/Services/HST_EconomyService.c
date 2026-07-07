class HST_EconomyService
{
	void AddFactionMoney(HST_CampaignState state, int amount)
	{
		state.m_iFactionMoney = Math.Max(0, state.m_iFactionMoney + amount);
	}

	bool SpendFactionMoney(HST_CampaignState state, int amount)
	{
		if (!state || amount < 0 || state.m_iFactionMoney < amount)
			return false;

		state.m_iFactionMoney -= amount;
		return true;
	}

	void AddHR(HST_CampaignState state, int amount)
	{
		state.m_iHR = Math.Max(0, state.m_iHR + amount);
	}

	bool SpendHR(HST_CampaignState state, int amount)
	{
		if (!state || amount < 0 || state.m_iHR < amount)
			return false;

		state.m_iHR -= amount;
		return true;
	}

	void AddAggression(HST_CampaignState state, string factionKey, int amount)
	{
		if (!state || factionKey.IsEmpty() || amount == 0)
			return;

		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		if (!pool)
			return;

		pool.m_iAggression = Math.Max(0, pool.m_iAggression + amount);
	}

	bool TickAggressionDecay(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, int elapsedSeconds)
	{
		if (!state || !preset || !balance || elapsedSeconds <= 0)
			return false;

		int interval = Math.Max(60, balance.m_iAggressionDecayIntervalSeconds);
		int decayAmount = Math.Max(0, balance.m_iAggressionDecayAmount);
		if (decayAmount <= 0)
			return false;

		state.m_iAggressionAccumulatorSeconds += elapsedSeconds;
		if (state.m_iAggressionAccumulatorSeconds < interval)
			return false;

		int decaySteps = state.m_iAggressionAccumulatorSeconds / interval;
		state.m_iAggressionAccumulatorSeconds = state.m_iAggressionAccumulatorSeconds % interval;
		int totalDecay = decaySteps * decayAmount;

		bool changed;
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || !HST_FactionRelationService.IsEnemyFaction(preset, pool.m_sFactionKey) || pool.m_iAggression <= 0)
				continue;

			int nextAggression = Math.Max(0, pool.m_iAggression - totalDecay);
			if (nextAggression == pool.m_iAggression)
				continue;

			pool.m_iAggression = nextAggression;
			changed = true;
		}

		return changed;
	}

	void RecalculateWarLevel(HST_CampaignState state, HST_BalanceConfig balance, string resistanceFactionKey = "FIA")
	{
		if (!state || !balance)
			return;

		int ownedScore = CalculateResistanceStrategicScore(state, resistanceFactionKey);
		int nextWarLevel = 1;
		if (ownedScore >= balance.m_iWarLevel2Score)
			nextWarLevel = 2;
		if (ownedScore >= balance.m_iWarLevel3Score)
			nextWarLevel = 3;
		if (ownedScore >= balance.m_iWarLevel4Score)
			nextWarLevel = 4;
		if (ownedScore >= balance.m_iWarLevel5Score)
			nextWarLevel = 5;
		if (ownedScore >= balance.m_iWarLevel6Score)
			nextWarLevel = 6;
		if (ownedScore >= balance.m_iWarLevel7Score)
			nextWarLevel = 7;
		if (ownedScore >= balance.m_iWarLevel8Score)
			nextWarLevel = 8;
		if (ownedScore >= balance.m_iWarLevel9Score)
			nextWarLevel = 9;
		if (ownedScore >= balance.m_iWarLevel10Score)
			nextWarLevel = 10;

		state.m_iWarLevel = Math.Min(balance.m_iWarLevelMaximum, Math.Max(1, nextWarLevel));
	}

	int CalculateResistanceStrategicScore(HST_CampaignState state, string resistanceFactionKey)
	{
		if (!state)
			return 0;

		int score;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey != resistanceFactionKey)
				continue;

			score += CalculateZoneStrategicScore(zone);
		}

		return score;
	}

	int CalculateZoneStrategicScore(HST_ZoneState zone)
	{
		if (!zone)
			return 0;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
			return 0;

		int score = Math.Max(1, zone.m_iPriority);
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			score += 2;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			score += 8;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			score += 12;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST)
			score += 10;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			score += 8;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			score += 18;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD)
			score += 22;

		return score;
	}

	int CalculateTotalStrategicScore(HST_CampaignState state)
	{
		if (!state)
			return 1;

		int score;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone)
				score += CalculateZoneStrategicScore(zone);
		}

		return Math.Max(1, score);
	}

	int ResolveControlPercent(int score, int totalScore)
	{
		if (totalScore <= 0)
			return 0;

		return Math.Round(score * 100.0 / totalScore);
	}

	string BuildPacingReport(HST_CampaignState state, HST_BalanceConfig balance, HST_CampaignPreset preset)
	{
		if (!state || !balance || !preset)
			return "h-istasi pacing | state/balance/preset not ready";

		int score = CalculateResistanceStrategicScore(state, preset.m_sResistanceFactionKey);
		int totalScore = CalculateTotalStrategicScore(state);
		int controlPercent = ResolveControlPercent(score, totalScore);
		string report = string.Format(
			"h-istasi pacing | phase %1 | war level %2/%3 | band %4 | score %5/%6 (%7 pct) | next threshold %8 | money %9",
			state.m_ePhase,
			state.m_iWarLevel,
			balance.m_iWarLevelMaximum,
			ResolvePacingBand(state),
			score,
			totalScore,
			controlPercent,
			ResolveNextThreshold(state, balance),
			state.m_iFactionMoney
		);
		report = report + string.Format(" | HR %1 | training %2 | income timer %3/%4s", state.m_iHR, state.m_iTrainingLevel, state.m_iIncomeAccumulatorSeconds, balance.m_iZoneIncomeIntervalSeconds);

		report = report + string.Format(
			"\nthresholds | WL2 %1 | WL3 %2 | WL4 %3 | WL5 %4 | WL6 %5 | WL7 %6 | WL8 %7 | WL9 %8 | WL10 %9",
			balance.m_iWarLevel2Score,
			balance.m_iWarLevel3Score,
			balance.m_iWarLevel4Score,
			balance.m_iWarLevel5Score,
			balance.m_iWarLevel6Score,
			balance.m_iWarLevel7Score,
			balance.m_iWarLevel8Score,
			balance.m_iWarLevel9Score,
			balance.m_iWarLevel10Score
		);

		report = report + string.Format(
			"\nvictory | population outcome %1 | support required %2 pct | legacy control enabled %3 | legacy control required %4 pct | requires airfields %5 | requires seaports %6 | current %7",
			balance.m_bPopulationOutcomeEnabled,
			balance.m_iVictoryPopulationSupportPercent,
			balance.m_bLegacyControlVictoryEnabled,
			balance.m_iVictoryControlPercent,
			balance.m_bVictoryRequiresAirfields,
			balance.m_bVictoryRequiresSeaports,
			BuildVictoryReadinessLabel(state, balance, preset, controlPercent)
		);
		report = report + "\npacing recommendation | " + BuildPacingRecommendation(state);
		report = report + "\n" + BuildEnemyPressureReport(state, preset);
		return report;
	}

	string ResolvePacingBand(HST_CampaignState state)
	{
		if (!state)
			return "unknown";

		if (state.m_iWarLevel <= 2)
			return "early";
		if (state.m_iWarLevel <= 5)
			return "mid";
		return "late";
	}

	protected string BuildPacingRecommendation(HST_CampaignState state)
	{
		if (!state)
			return "campaign state unavailable";
		if (state.m_iWarLevel <= 2)
			return "loot, unlock basic weapons, build town support, and take weak resources/outposts";
		if (state.m_iWarLevel <= 5)
			return "recruit garrisons, hold resources/factories, and prepare for counterattacks";
		return "build majority town support, take airfields, and survive HQ threat";
	}

	protected string BuildVictoryReadinessLabel(HST_CampaignState state, HST_BalanceConfig balance, HST_CampaignPreset preset, int controlPercent)
	{
		bool controlReady = controlPercent >= balance.m_iVictoryControlPercent;
		bool airfieldsReady = !balance.m_bVictoryRequiresAirfields || AreAllTypeOwnedBy(state, HST_EZoneType.HST_ZONE_AIRFIELD, preset.m_sResistanceFactionKey);
		bool seaportsReady = !balance.m_bVictoryRequiresSeaports || AreAllTypeOwnedBy(state, HST_EZoneType.HST_ZONE_SEAPORT, preset.m_sResistanceFactionKey);
		if (!balance.m_bPopulationOutcomeEnabled)
			return string.Format("legacy control %1 | airfields %2 | seaports %3", controlReady, airfieldsReady, seaportsReady);

		int remainingPopulation;
		int fiaSupportPopulation;
		int killedPopulation;
		int initialPopulation;
		foreach (HST_CivilianZoneState town : state.m_aCivilianZones)
		{
			if (!town)
				continue;

			int remaining = Math.Max(0, town.m_iPopulationRemaining);
			int killed = Math.Max(0, town.m_iPopulationKilled);
			remainingPopulation += remaining;
			killedPopulation += killed;
			initialPopulation += remaining + killed;
			fiaSupportPopulation += Math.Round(remaining * Math.Max(0, Math.Min(100, town.m_iFIASupport)) / 100.0);
		}

		int supportPercent;
		if (remainingPopulation > 0)
			supportPercent = Math.Round(fiaSupportPopulation * 100.0 / remainingPopulation);
		int killedPercent;
		if (initialPopulation > 0)
			killedPercent = Math.Round(killedPopulation * 100.0 / initialPopulation);
		bool supportReady = remainingPopulation > 0 && fiaSupportPopulation * 100 >= remainingPopulation * balance.m_iVictoryPopulationSupportPercent;
		bool lossRisk = initialPopulation > 0 && killedPopulation * 3 > initialPopulation;
		return string.Format("population support %1 pct ready %2 | killed %3 pct loss %4 | airfields %5 | legacy control %6", supportPercent, supportReady, killedPercent, lossRisk, airfieldsReady, controlReady);
	}

	protected string BuildEnemyPressureReport(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return "enemy pressure | state/preset not ready";

		string report = "enemy pressure | current pools";
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || !HST_FactionRelationService.IsEnemyFaction(preset, pool.m_sFactionKey))
				continue;

			report = report + string.Format("\n%1 | attack %2 | support %3 | aggression %4", pool.m_sFactionKey, pool.m_iAttackResources, pool.m_iSupportResources, pool.m_iAggression);
		}

		return report;
	}

	protected bool AreAllTypeOwnedBy(HST_CampaignState state, HST_EZoneType zoneType, string factionKey)
	{
		bool found;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_eType != zoneType)
				continue;

			found = true;
			if (zone.m_sOwnerFactionKey != factionKey)
				return false;
		}

		return found;
	}

	protected int ResolveNextThreshold(HST_CampaignState state, HST_BalanceConfig balance)
	{
		if (!state || !balance)
			return 0;
		if (state.m_iWarLevel < 2)
			return balance.m_iWarLevel2Score;
		if (state.m_iWarLevel < 3)
			return balance.m_iWarLevel3Score;
		if (state.m_iWarLevel < 4)
			return balance.m_iWarLevel4Score;
		if (state.m_iWarLevel < 5)
			return balance.m_iWarLevel5Score;
		if (state.m_iWarLevel < 6)
			return balance.m_iWarLevel6Score;
		if (state.m_iWarLevel < 7)
			return balance.m_iWarLevel7Score;
		if (state.m_iWarLevel < 8)
			return balance.m_iWarLevel8Score;
		if (state.m_iWarLevel < 9)
			return balance.m_iWarLevel9Score;
		if (state.m_iWarLevel < 10)
			return balance.m_iWarLevel10Score;
		return 0;
	}
}
