class HST_CampaignOutcomeResult
{
	bool m_bEnded;
	HST_ECampaignPhase m_ePhase;
	string m_sReason;
	string m_sSummary;
	int m_iControlPercent;
	int m_iWarLevel;
	int m_iFIAZones;
	int m_iEnemyZones;
	string m_sOutcomeMode;
	int m_iInitialPopulation;
	int m_iRemainingPopulation;
	int m_iKilledPopulation;
	int m_iFIASupportPopulation;
	int m_iSupportPercent;
	int m_iAirfieldsControlled;
	int m_iAirfieldsTotal;
}

class HST_StrategicService
{
	bool SetZoneOwner(HST_CampaignState state, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, string factionKey, string resistanceFactionKey = "FIA")
	{
		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone || !state.FindFactionPool(factionKey) || zone.m_sOwnerFactionKey == factionKey)
			return false;

		string previousOwner = zone.m_sOwnerFactionKey;
		zone.m_sOwnerFactionKey = factionKey;
		zone.m_iResistanceCaptureProgress = 0;
		zone.m_iActiveInfantryCount = 0;
		zone.m_iActiveVehicleCount = 0;
		ClearZoneGarrison(state, zoneId, previousOwner);
		economy.RecalculateWarLevel(state, balance, resistanceFactionKey);
		EvaluateCampaignOutcome(state, economy, balance, resistanceFactionKey);
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

	HST_CampaignOutcomeResult EvaluateCampaignOutcomeDetailed(HST_CampaignState state, HST_EconomyService economy, HST_BalanceConfig balance, string resistanceFactionKey)
	{
		HST_CampaignOutcomeResult result = new HST_CampaignOutcomeResult();
		if (!state || !economy || !balance || state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
			return result;

		int fiaZones = CountZonesOwnedBy(state, resistanceFactionKey);
		int enemyZones = CountZonesNotOwnedBy(state, resistanceFactionKey);
		int score = economy.CalculateResistanceStrategicScore(state, resistanceFactionKey);
		int totalScore = CalculateTotalStrategicScoreForOutcome(state, economy);
		int controlPercent;
		if (totalScore > 0)
			controlPercent = Math.Round(score * 100.0 / totalScore);

		result.m_iControlPercent = controlPercent;
		result.m_iWarLevel = state.m_iWarLevel;
		result.m_iFIAZones = fiaZones;
		result.m_iEnemyZones = enemyZones;
		result.m_iInitialPopulation = GetTotalInitialPopulation(state);
		result.m_iRemainingPopulation = GetTotalRemainingPopulation(state);
		result.m_iKilledPopulation = GetTotalKilledPopulation(state);
		result.m_iFIASupportPopulation = GetTotalFIASupportPopulation(state);
		result.m_iSupportPercent = CalculatePopulationSupportPercent(result.m_iFIASupportPopulation, result.m_iRemainingPopulation);
		result.m_iAirfieldsTotal = CountZonesOfType(state, HST_EZoneType.HST_ZONE_AIRFIELD);
		result.m_iAirfieldsControlled = CountTypeOwnedBy(state, HST_EZoneType.HST_ZONE_AIRFIELD, resistanceFactionKey);
		result.m_sOutcomeMode = ResolveOutcomeMode(balance);

		if (!IsCampaignOutcomeCheckDue(state, balance))
			return result;

		if (balance.m_bPopulationOutcomeEnabled)
		{
			if (ShouldLosePopulationCampaign(result, balance))
			{
				result.m_bEnded = true;
				result.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_LOST;
				result.m_sReason = "loss: civilian catastrophe";
				result.m_sSummary = string.Format("Civilian killed population %1/%2 exceeded one third of initial population.", result.m_iKilledPopulation, result.m_iInitialPopulation);
				return result;
			}

			bool populationSupportReady = result.m_iRemainingPopulation > 0 && result.m_iFIASupportPopulation * 100 >= result.m_iRemainingPopulation * balance.m_iVictoryPopulationSupportPercent;
			bool populationAirfieldsReady = !balance.m_bVictoryRequiresAirfields || AreAllAirfieldsControlledBy(state, resistanceFactionKey);
			if (populationSupportReady && populationAirfieldsReady)
			{
				result.m_bEnded = true;
				result.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_WON;
				result.m_sReason = "victory: population support and airfield control";
				result.m_sSummary = string.Format("FIA support population %1/%2 (%3 pct) reached the %4 pct threshold with airfields %5/%6 controlled.", result.m_iFIASupportPopulation, result.m_iRemainingPopulation, result.m_iSupportPercent, balance.m_iVictoryPopulationSupportPercent, result.m_iAirfieldsControlled, result.m_iAirfieldsTotal);
				return result;
			}

			return result;
		}

		bool airfieldsReady = !balance.m_bVictoryRequiresAirfields || AreAllAirfieldsControlledBy(state, resistanceFactionKey);
		bool seaportsReady = !balance.m_bVictoryRequiresSeaports || AreAllTypeOwnedBy(state, HST_EZoneType.HST_ZONE_SEAPORT, resistanceFactionKey);
		bool controlReady = controlPercent >= balance.m_iVictoryControlPercent;
		if (balance.m_bLegacyControlVictoryEnabled && controlReady && airfieldsReady && seaportsReady)
		{
			result.m_bEnded = true;
			result.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_WON;
			result.m_sReason = "victory: strategic control achieved";
			result.m_sSummary = string.Format("FIA controls %1 percent of strategic score with required airfields %2 and seaports %3.", controlPercent, airfieldsReady, seaportsReady);
			return result;
		}

		if (ShouldLoseCampaign(state, balance, resistanceFactionKey))
		{
			result.m_bEnded = true;
			result.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_LOST;
			result.m_sReason = "loss: FIA collapse";
			result.m_sSummary = string.Format("FIA collapsed with money %1 HR %2 Petros deaths %3.", state.m_iFactionMoney, state.m_iHR, state.m_iPetrosDeaths);
			return result;
		}

		return result;
	}

	void ApplyCampaignOutcome(HST_CampaignState state, HST_CampaignOutcomeResult outcome)
	{
		if (!state || !outcome || !outcome.m_bEnded)
			return;

		state.m_ePhase = outcome.m_ePhase;
		state.m_sCampaignEndReason = outcome.m_sReason;
		state.m_sCampaignEndSummary = outcome.m_sSummary;
		state.m_iCampaignEndedAtSecond = state.m_iElapsedSeconds;
		state.m_iCampaignEndControlPercent = outcome.m_iControlPercent;
		state.m_iCampaignEndWarLevel = outcome.m_iWarLevel;
		state.m_iCampaignEndFIAZones = outcome.m_iFIAZones;
		state.m_iCampaignEndEnemyZones = outcome.m_iEnemyZones;
		state.m_sCampaignEndOutcomeMode = outcome.m_sOutcomeMode;
		state.m_iCampaignEndInitialPopulation = outcome.m_iInitialPopulation;
		state.m_iCampaignEndRemainingPopulation = outcome.m_iRemainingPopulation;
		state.m_iCampaignEndKilledPopulation = outcome.m_iKilledPopulation;
		state.m_iCampaignEndFIASupportPopulation = outcome.m_iFIASupportPopulation;
		state.m_iCampaignEndSupportPercent = outcome.m_iSupportPercent;
		state.m_iCampaignEndAirfieldsControlled = outcome.m_iAirfieldsControlled;
		state.m_iCampaignEndAirfieldsTotal = outcome.m_iAirfieldsTotal;
		state.m_bCampaignEndReportGenerated = true;
	}

	string BuildCampaignEndReport(HST_CampaignState state, HST_EconomyService economy, HST_BalanceConfig balance, HST_CampaignPreset preset)
	{
		if (!state)
			return "h-istasi campaign end | state not ready";

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		string phase = string.Format("%1", state.m_ePhase);
		int score;
		int totalScore;
		int controlPercent;
		if (economy)
		{
			score = economy.CalculateResistanceStrategicScore(state, resistanceFactionKey);
			totalScore = CalculateTotalStrategicScoreForOutcome(state, economy);
			if (totalScore > 0)
				controlPercent = Math.Round(score * 100.0 / totalScore);
		}

		int initialPopulation = GetTotalInitialPopulation(state);
		int remainingPopulation = GetTotalRemainingPopulation(state);
		int killedPopulation = GetTotalKilledPopulation(state);
		int fiaSupportPopulation = GetTotalFIASupportPopulation(state);
		int supportPercent = CalculatePopulationSupportPercent(fiaSupportPopulation, remainingPopulation);
		int killedPercent;
		if (initialPopulation > 0)
			killedPercent = Math.Round(killedPopulation * 100.0 / initialPopulation);
		int airfieldsTotal = CountZonesOfType(state, HST_EZoneType.HST_ZONE_AIRFIELD);
		int airfieldsControlled = CountTypeOwnedBy(state, HST_EZoneType.HST_ZONE_AIRFIELD, resistanceFactionKey);
		int nextOutcomeCheckSecond = GetNextOutcomeCheckSecond(state, balance);
		string outcomeMode = "population";
		bool airfieldsRequired;
		if (balance)
		{
			outcomeMode = ResolveOutcomeMode(balance);
			airfieldsRequired = balance.m_bVictoryRequiresAirfields;
		}

		string report = string.Format(
			"h-istasi campaign end | phase %1 | ended %2 | reason %3 | summary %4 | control %5 pct | score %6/%7 | war %8 | FIA zones %9",
			phase,
			state.m_iCampaignEndedAtSecond,
			state.m_sCampaignEndReason,
			state.m_sCampaignEndSummary,
			controlPercent,
			score,
			totalScore,
			state.m_iWarLevel,
			CountZonesOwnedBy(state, resistanceFactionKey)
		);
		report = report + string.Format(" | enemy zones %1", CountZonesNotOwnedBy(state, resistanceFactionKey));
		report = report + string.Format("\noutcome | mode %1 | next check second %2", outcomeMode, nextOutcomeCheckSecond);
		report = report + string.Format("\npopulation | initial %1 | remaining %2 | killed %3 (%4 pct) | FIA support population %5 (%6 pct of remaining)", initialPopulation, remainingPopulation, killedPopulation, killedPercent, fiaSupportPopulation, supportPercent);
		report = report + string.Format("\nairfields | controlled %1/%2 | required %3", airfieldsControlled, airfieldsTotal, airfieldsRequired);

		if (state.m_bCampaignEndReportGenerated)
		{
			report = report + string.Format(
				"\nfinal persisted | control %1 pct | war %2 | FIA zones %3 | enemy zones %4 | mode %5",
				state.m_iCampaignEndControlPercent,
				state.m_iCampaignEndWarLevel,
				state.m_iCampaignEndFIAZones,
				state.m_iCampaignEndEnemyZones,
				EmptyReportField(state.m_sCampaignEndOutcomeMode)
			);
			report = report + string.Format("\nfinal population | initial %1 | remaining %2 | killed %3 | FIA support %4 (%5 pct) | airfields %6/%7", state.m_iCampaignEndInitialPopulation, state.m_iCampaignEndRemainingPopulation, state.m_iCampaignEndKilledPopulation, state.m_iCampaignEndFIASupportPopulation, state.m_iCampaignEndSupportPercent, state.m_iCampaignEndAirfieldsControlled, state.m_iCampaignEndAirfieldsTotal);
		}

		if (balance)
		{
			report = report + string.Format(
				"\nrequirements | population outcome %1 | support %2 pct | legacy control enabled %3 | legacy victory %4 pct | airfields %5 | seaports %6 | loss enabled %7 | Petros death limit %8 | grace %9s",
				balance.m_bPopulationOutcomeEnabled,
				balance.m_iVictoryPopulationSupportPercent,
				balance.m_bLegacyControlVictoryEnabled,
				balance.m_iVictoryControlPercent,
				balance.m_bVictoryRequiresAirfields,
				balance.m_bVictoryRequiresSeaports,
				balance.m_bLossConditionEnabled,
				balance.m_iLossPetrosDeathLimit,
				balance.m_iLossGraceSeconds
			);
		}

		return report;
	}

	int CountZonesOwnedBy(HST_CampaignState state, string factionKey)
	{
		int count;
		if (!state)
			return count;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (IsStrategicZoneCounted(zone) && zone.m_sOwnerFactionKey == factionKey)
				count++;
		}

		return count;
	}

	int CountZonesNotOwnedBy(HST_CampaignState state, string factionKey)
	{
		int count;
		if (!state)
			return count;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (IsStrategicZoneCounted(zone) && zone.m_sOwnerFactionKey != factionKey)
				count++;
		}

		return count;
	}

	int CountZonesOfType(HST_CampaignState state, HST_EZoneType zoneType)
	{
		int count;
		if (!state)
			return count;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_eType == zoneType)
				count++;
		}

		return count;
	}

	int CountTypeOwnedBy(HST_CampaignState state, HST_EZoneType zoneType, string factionKey)
	{
		int count;
		if (!state)
			return count;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_eType == zoneType && zone.m_sOwnerFactionKey == factionKey)
				count++;
		}

		return count;
	}

	int GetTotalInitialPopulation(HST_CampaignState state)
	{
		int total;
		if (!state)
			return total;

		foreach (HST_CivilianZoneState town : state.m_aCivilianZones)
		{
			if (!town)
				continue;

			total += Math.Max(0, town.m_iPopulationRemaining) + Math.Max(0, town.m_iPopulationKilled);
		}

		return total;
	}

	int GetTotalRemainingPopulation(HST_CampaignState state)
	{
		int total;
		if (!state)
			return total;

		foreach (HST_CivilianZoneState town : state.m_aCivilianZones)
		{
			if (town)
				total += Math.Max(0, town.m_iPopulationRemaining);
		}

		return total;
	}

	int GetTotalKilledPopulation(HST_CampaignState state)
	{
		int total;
		if (!state)
			return total;

		foreach (HST_CivilianZoneState town : state.m_aCivilianZones)
		{
			if (town)
				total += Math.Max(0, town.m_iPopulationKilled);
		}

		return total;
	}

	int GetTotalFIASupportPopulation(HST_CampaignState state)
	{
		int total;
		if (!state)
			return total;

		foreach (HST_CivilianZoneState town : state.m_aCivilianZones)
		{
			if (!town)
				continue;

			int remaining = Math.Max(0, town.m_iPopulationRemaining);
			int support = Math.Max(0, Math.Min(100, town.m_iFIASupport));
			total += Math.Round(remaining * support / 100.0);
		}

		return total;
	}

	bool AreAllAirfieldsControlledBy(HST_CampaignState state, string factionKey)
	{
		int total = CountZonesOfType(state, HST_EZoneType.HST_ZONE_AIRFIELD);
		return total > 0 && CountTypeOwnedBy(state, HST_EZoneType.HST_ZONE_AIRFIELD, factionKey) == total;
	}

	protected bool IsStrategicZoneCounted(HST_ZoneState zone)
	{
		return zone && zone.m_eType != HST_EZoneType.HST_ZONE_HIDEOUT && zone.m_eType != HST_EZoneType.HST_ZONE_MISSION_SITE;
	}

	protected void ClearZoneGarrison(HST_CampaignState state, string zoneId, string factionKey)
	{
		if (!state || zoneId.IsEmpty() || factionKey.IsEmpty())
			return;

		HST_GarrisonState garrison = state.FindGarrison(zoneId, factionKey);
		if (!garrison)
			return;

		garrison.m_iInfantryCount = 0;
		garrison.m_iVehicleCount = 0;
	}

	protected void EvaluateCampaignOutcome(HST_CampaignState state, HST_EconomyService economy, HST_BalanceConfig balance, string resistanceFactionKey)
	{
		HST_CampaignOutcomeResult outcome = EvaluateCampaignOutcomeDetailed(state, economy, balance, resistanceFactionKey);
		if (!outcome || !outcome.m_bEnded)
			return;

		ApplyCampaignOutcome(state, outcome);
	}

	protected int CalculateTotalStrategicScoreForOutcome(HST_CampaignState state, HST_EconomyService economy)
	{
		if (!economy)
			return 0;

		return economy.CalculateTotalStrategicScore(state);
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

	protected bool IsCampaignOutcomeCheckDue(HST_CampaignState state, HST_BalanceConfig balance)
	{
		if (!state || !balance)
			return true;

		return state.m_iIncomeAccumulatorSeconds == 0;
	}

	protected int GetNextOutcomeCheckSecond(HST_CampaignState state, HST_BalanceConfig balance)
	{
		if (!state || !balance)
			return 0;

		int interval = Math.Max(1, balance.m_iZoneIncomeIntervalSeconds);
		int remaining = interval - Math.Max(0, state.m_iIncomeAccumulatorSeconds);
		if (remaining <= 0)
			remaining = interval;
		return state.m_iElapsedSeconds + remaining;
	}

	protected string ResolveOutcomeMode(HST_BalanceConfig balance)
	{
		if (!balance)
			return "unknown";
		if (balance.m_bPopulationOutcomeEnabled)
			return "population";
		if (balance.m_bLegacyControlVictoryEnabled)
			return "legacy_control";
		return "disabled";
	}

	protected int CalculatePopulationSupportPercent(int supportPopulation, int remainingPopulation)
	{
		if (remainingPopulation <= 0)
			return 0;

		return Math.Round(supportPopulation * 100.0 / remainingPopulation);
	}

	protected bool ShouldLosePopulationCampaign(HST_CampaignOutcomeResult result, HST_BalanceConfig balance)
	{
		if (!result || !balance || !balance.m_bLossConditionEnabled)
			return false;
		if (result.m_iInitialPopulation <= 0)
			return false;

		return result.m_iKilledPopulation * 3 > result.m_iInitialPopulation;
	}

	protected string EmptyReportField(string value)
	{
		if (value.IsEmpty())
			return "-";

		return value;
	}

	protected bool ShouldLoseCampaign(HST_CampaignState state, HST_BalanceConfig balance, string resistanceFactionKey)
	{
		if (!balance.m_bLossConditionEnabled)
			return false;
		if (state.m_iElapsedSeconds < balance.m_iLossGraceSeconds)
			return false;
		if (state.m_iPetrosDeaths >= balance.m_iLossPetrosDeathLimit)
			return true;
		if (state.m_iHR <= balance.m_iLossHRThreshold && state.m_iFactionMoney <= balance.m_iLossMoneyThreshold && !HasRecoverableFriendlyZone(state, resistanceFactionKey))
			return true;

		return false;
	}

	protected bool HasRecoverableFriendlyZone(HST_CampaignState state, string resistanceFactionKey)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_sOwnerFactionKey == resistanceFactionKey)
				return true;
		}

		return false;
	}
}
