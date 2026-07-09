class HST_RecruitmentResult
{
	bool m_bSuccess;
	string m_sFailureReason;
	string m_sZoneId;
	string m_sFactionKey;
	int m_iRequestedInfantry;
	int m_iRequestedVehicles;
	int m_iAddedInfantry;
	int m_iAddedVehicles;
	int m_iMoneySpent;
	int m_iHRSpent;
	int m_iTrainingLevel;
	string m_sEquipmentTier;

	string BuildSummary()
	{
		if (!m_bSuccess)
			return "h-istasi recruitment | failed: " + m_sFailureReason;

		return string.Format(
			"h-istasi recruitment | complete | zone %1 | +%2 infantry +%3 vehicles | spent $%4 / HR %5 | training %6 | equipment %7",
			m_sZoneId,
			m_iAddedInfantry,
			m_iAddedVehicles,
			m_iMoneySpent,
			m_iHRSpent,
			m_iTrainingLevel,
			m_sEquipmentTier
		);
	}
}

class HST_TrainingResult
{
	bool m_bSuccess;
	bool m_bAlreadyApplied;
	string m_sFailureReason;
	string m_sCommandRequestId;
	string m_sOperationId;
	string m_sTransactionId;
	int m_iOldTrainingLevel;
	int m_iNewTrainingLevel;
	int m_iMoneySpent;
	int m_iWarLevel;
	int m_iTrainingCap;

	string BuildSummary()
	{
		if (!m_bSuccess)
			return "h-istasi training | failed: " + m_sFailureReason;
		if (m_bAlreadyApplied)
			return string.Format("h-istasi training | already applied | request %1 | level %2", m_sCommandRequestId, m_iNewTrainingLevel);

		string summary = string.Format(
			"h-istasi training | complete | level %1 -> %2 | spent $%3 | war level %4 | cap %5",
			m_iOldTrainingLevel,
			m_iNewTrainingLevel,
			m_iMoneySpent,
			m_iWarLevel,
			m_iTrainingCap
		);
		if (!m_sTransactionId.IsEmpty())
			summary = summary + " | transaction " + m_sTransactionId;
		return summary;
	}
}

class HST_RecruitmentService
{
	static const int TRAINING_QUALITY_BONUS_PER_LEVEL = 5;
	static const int TRAINING_QUALITY_BONUS_MAX = 45;

	int ResolveTrainingCap(HST_CampaignState state)
	{
		if (!state)
			return 3;

		int warLevel = Math.Max(1, state.m_iWarLevel);
		return Math.Max(1, Math.Min(10, warLevel + 2));
	}

	static int ResolveTrainingQualityBonusPercentForLevel(int trainingLevel)
	{
		int normalizedTraining = Math.Max(1, trainingLevel);
		return Math.Min(TRAINING_QUALITY_BONUS_MAX, Math.Max(0, normalizedTraining - 1) * TRAINING_QUALITY_BONUS_PER_LEVEL);
	}

	int ResolveTrainingQualityBonusPercent(HST_CampaignState state)
	{
		if (!state)
			return 0;

		return ResolveTrainingQualityBonusPercentForLevel(state.m_iTrainingLevel);
	}

	static int ResolveTrainingEffectiveInfantryStrengthForLevel(int infantryCount, int trainingLevel)
	{
		int actualInfantry = Math.Max(0, infantryCount);
		if (actualInfantry <= 0)
			return 0;

		int bonusPercent = ResolveTrainingQualityBonusPercentForLevel(trainingLevel);
		return Math.Max(actualInfantry, actualInfantry * (100 + bonusPercent) / 100);
	}

	int ResolveTrainingEffectiveInfantryStrength(HST_CampaignState state, int infantryCount)
	{
		if (!state)
			return Math.Max(0, infantryCount);

		return ResolveTrainingEffectiveInfantryStrengthForLevel(infantryCount, state.m_iTrainingLevel);
	}

	string BuildTrainingQualitySummary(HST_CampaignState state)
	{
		if (!state)
			return "training quality unknown";

		return string.Format(
			"training quality +%1 pct | 4 FIA count as %2 strength",
			ResolveTrainingQualityBonusPercent(state),
			ResolveTrainingEffectiveInfantryStrength(state, 4)
		);
	}

	HST_TrainingResult TrainTroopsDetailed(HST_CampaignState state, HST_EconomyService economy, int moneyCost, HST_ResourceLedgerService resourceLedger = null, string commandRequestId = "", string actorIdentityId = "")
	{
		HST_TrainingResult result = new HST_TrainingResult();

		if (!state || !economy)
		{
			result.m_sFailureReason = "service not ready";
			return result;
		}

		if (moneyCost < 0)
		{
			result.m_sFailureReason = "negative money cost";
			return result;
		}

		result.m_iOldTrainingLevel = state.m_iTrainingLevel;
		result.m_iNewTrainingLevel = state.m_iTrainingLevel;
		result.m_iWarLevel = Math.Max(1, state.m_iWarLevel);
		result.m_iTrainingCap = ResolveTrainingCap(state);

		if (state.m_iTrainingLevel >= result.m_iTrainingCap)
		{
			result.m_sFailureReason = string.Format("training capped by war level %1 at level %2", result.m_iWarLevel, result.m_iTrainingCap);
			return result;
		}

		if (state.m_iFactionMoney < moneyCost)
		{
			result.m_sFailureReason = string.Format("need $%1, have $%2", moneyCost, state.m_iFactionMoney);
			return result;
		}

		result.m_sCommandRequestId = commandRequestId;
		if (resourceLedger)
		{
			if (commandRequestId.IsEmpty())
				commandRequestId = HST_StableIdService.NextId(state, "training_command");
			result.m_sCommandRequestId = commandRequestId;
			result.m_sOperationId = HST_StableIdService.BuildOperationId("training", commandRequestId);
			result.m_sTransactionId = HST_StableIdService.BuildTransactionId(result.m_sOperationId, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY);
			HST_ResourceTransactionResult reservation = resourceLedger.ReserveCost(state, economy, result.m_sTransactionId, commandRequestId, result.m_sOperationId, actorIdentityId, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, moneyCost, "resistance training purchase");
			if (!reservation || !reservation.m_bSuccess)
			{
				result.m_sFailureReason = "money reservation failed";
				if (reservation && !reservation.m_sFailureReason.IsEmpty())
					result.m_sFailureReason = reservation.m_sFailureReason;
				return result;
			}
			if (reservation.m_bAlreadyApplied)
			{
				result.m_bSuccess = true;
				result.m_bAlreadyApplied = true;
				result.m_iNewTrainingLevel = state.m_iTrainingLevel;
				return result;
			}
		}
		else if (!economy.SpendFactionMoney(state, moneyCost))
		{
			result.m_sFailureReason = "money spend failed";
			return result;
		}

		state.m_iTrainingLevel++;
		if (resourceLedger && !resourceLedger.CommitReserved(state, result.m_sTransactionId))
		{
			state.m_iTrainingLevel = result.m_iOldTrainingLevel;
			resourceLedger.CancelReservation(state, economy, result.m_sTransactionId, "training_commit_failed_" + commandRequestId, "training mutation could not commit");
			result.m_sFailureReason = "resource transaction commit failed";
			return result;
		}
		result.m_bSuccess = true;
		result.m_iNewTrainingLevel = state.m_iTrainingLevel;
		result.m_iMoneySpent = moneyCost;
		return result;
	}

	bool TrainTroops(HST_CampaignState state, HST_EconomyService economy, int moneyCost)
	{
		HST_TrainingResult result = TrainTroopsDetailed(state, economy, moneyCost);
		return result && result.m_bSuccess;
	}

	HST_RecruitmentResult RecruitGarrisonDetailed(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_EconomyService economy, HST_GarrisonService garrisons, HST_ArsenalService arsenal, string zoneId, int requestedInfantry, int requestedVehicles, int moneyCost, int hrCost)
	{
		HST_RecruitmentResult result = new HST_RecruitmentResult();
		result.m_sZoneId = zoneId;
		result.m_iRequestedInfantry = requestedInfantry;
		result.m_iRequestedVehicles = requestedVehicles;

		if (!state || !preset || !economy || !garrisons)
		{
			result.m_sFailureReason = "service not ready";
			return result;
		}

		if (requestedInfantry < 0 || requestedVehicles < 0 || moneyCost < 0 || hrCost < 0)
		{
			result.m_sFailureReason = "negative request or cost";
			return result;
		}

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
		{
			result.m_sFailureReason = "zone not found";
			return result;
		}

		string resistanceFactionKey = ResolveResistanceFactionKey(preset);
		result.m_sFactionKey = resistanceFactionKey;

		if (zone.m_sOwnerFactionKey != resistanceFactionKey)
		{
			result.m_sFailureReason = string.Format("zone is owned by %1", zone.m_sOwnerFactionKey);
			return result;
		}

		return RecruitGarrisonForFaction(state, economy, garrisons, arsenal, zone, resistanceFactionKey, requestedInfantry, requestedVehicles, moneyCost, hrCost, result);
	}

	bool RecruitGarrison(HST_CampaignState state, HST_EconomyService economy, HST_GarrisonService garrisons, string zoneId, string factionKey, int infantryCount, int vehicleCount, int moneyCost, int hrCost)
	{
		HST_RecruitmentResult result = new HST_RecruitmentResult();
		result.m_sZoneId = zoneId;
		result.m_sFactionKey = factionKey;
		result.m_iRequestedInfantry = infantryCount;
		result.m_iRequestedVehicles = vehicleCount;

		if (!state || !economy || !garrisons || infantryCount < 0 || vehicleCount < 0 || moneyCost < 0 || hrCost < 0)
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone || zone.m_sOwnerFactionKey != factionKey)
			return false;

		result = RecruitGarrisonForFaction(state, economy, garrisons, null, zone, factionKey, infantryCount, vehicleCount, moneyCost, hrCost, result);
		return result && result.m_bSuccess;
	}

	string ResolveRecruitEquipmentTier(HST_CampaignState state, HST_ArsenalService arsenal)
	{
		if (!state)
			return "unknown";

		int unlockedWeapons;
		int unlockedMagazines;
		int unlockedLaunchers;
		int unlockedExplosives;

		foreach (HST_ArsenalItemState item : state.m_aArsenalItems)
		{
			if (!item || !item.m_bUnlocked)
				continue;

			if (item.m_sCategory == "weapon")
				unlockedWeapons++;
			else if (item.m_sCategory == "magazine")
				unlockedMagazines++;
			else if (item.m_sCategory == "launcher")
				unlockedLaunchers++;
			else if (item.m_sCategory == "explosive")
				unlockedExplosives++;
		}

		if (unlockedLaunchers > 0)
			return "AT-capable";

		if (unlockedWeapons >= 3 && unlockedMagazines >= 3)
			return "modernized";

		if (unlockedWeapons > 0 || unlockedMagazines > 0)
			return "armed";

		if (unlockedExplosives > 0)
			return "demolition";

		return "scavenged";
	}

	string BuildRecruitmentReport(HST_CampaignState state, HST_CampaignPreset preset, HST_ArsenalService arsenal)
	{
		if (!state)
			return "h-istasi recruitment | campaign state not ready";

		string resistanceFactionKey = ResolveResistanceFactionKey(preset);
		int friendlyGarrisons;
		int friendlyInfantry;
		int friendlyVehicles;
		int activeFriendlyInfantry;
		int activeFriendlyVehicles;

		foreach (HST_GarrisonState garrison : state.m_aGarrisons)
		{
			if (!garrison || garrison.m_sFactionKey != resistanceFactionKey)
				continue;

			friendlyGarrisons++;
			friendlyInfantry += Math.Max(0, garrison.m_iInfantryCount);
			friendlyVehicles += Math.Max(0, garrison.m_iVehicleCount);
		}

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey != resistanceFactionKey)
				continue;

			activeFriendlyInfantry += Math.Max(0, zone.m_iActiveInfantryCount);
			activeFriendlyVehicles += Math.Max(0, zone.m_iActiveVehicleCount);
		}

		string report = string.Format(
			"h-istasi recruitment | training %1/%2 | quality +%3 pct | money $%4 | HR %5 | equipment %6 | unlocks %7",
			state.m_iTrainingLevel,
			ResolveTrainingCap(state),
			ResolveTrainingQualityBonusPercent(state),
			state.m_iFactionMoney,
			state.m_iHR,
			ResolveRecruitEquipmentTier(state, arsenal),
			BuildRecruitUnlockSummary(state)
		);
		report = report + string.Format(
			" | FIA garrisons %1 | abstract infantry %2 vehicles %3 | active infantry %4 vehicles %5",
			friendlyGarrisons,
			friendlyInfantry,
			friendlyVehicles,
			activeFriendlyInfantry,
			activeFriendlyVehicles
		);

		foreach (HST_GarrisonState friendly : state.m_aGarrisons)
		{
			if (!friendly || friendly.m_sFactionKey != resistanceFactionKey)
				continue;

			HST_ZoneState zone = state.FindZone(friendly.m_sZoneId);
			string zoneName = friendly.m_sZoneId;
			string owner = "";
			int slots;
			int activeInfantry;
			int activeVehicles;

			if (zone)
			{
				if (!zone.m_sDisplayName.IsEmpty())
					zoneName = zone.m_sDisplayName;
				owner = zone.m_sOwnerFactionKey;
				slots = zone.m_iGarrisonSlots;
				activeInfantry = Math.Max(0, zone.m_iActiveInfantryCount);
				activeVehicles = Math.Max(0, zone.m_iActiveVehicleCount);
			}

			report = report + string.Format(
				"\n%1 | owner %2 | abstract %3/%4 | active %5/%6 | cap %7",
				zoneName,
				owner,
				friendly.m_iInfantryCount,
				friendly.m_iVehicleCount,
				activeInfantry,
				activeVehicles,
				BuildRecruitCapacityLabel(friendly, slots, activeInfantry)
			);
		}

		return report;
	}

	protected HST_RecruitmentResult RecruitGarrisonForFaction(HST_CampaignState state, HST_EconomyService economy, HST_GarrisonService garrisons, HST_ArsenalService arsenal, HST_ZoneState zone, string factionKey, int requestedInfantry, int requestedVehicles, int moneyCost, int hrCost, HST_RecruitmentResult result)
	{
		if (state.m_iFactionMoney < moneyCost)
		{
			result.m_sFailureReason = string.Format("need $%1, have $%2", moneyCost, state.m_iFactionMoney);
			return result;
		}

		if (state.m_iHR < hrCost)
		{
			result.m_sFailureReason = string.Format("need %1 HR, have %2", hrCost, state.m_iHR);
			return result;
		}

		HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, factionKey);
		bool hadGarrison = garrison != null;
		int beforeInfantry = 0;
		int beforeVehicles = 0;
		if (garrison)
		{
			beforeInfantry = garrison.m_iInfantryCount;
			beforeVehicles = garrison.m_iVehicleCount;
		}

		int activeInfantry = Math.Max(0, zone.m_iActiveInfantryCount);

		int infantryCapacity = Math.Max(0, requestedInfantry);
		if (zone.m_iGarrisonSlots > 0)
			infantryCapacity = Math.Max(0, zone.m_iGarrisonSlots - beforeInfantry - activeInfantry);

		int infantryToAdd = Math.Min(Math.Max(0, requestedInfantry), infantryCapacity);
		int vehiclesToAdd = Math.Max(0, requestedVehicles);

		if (infantryToAdd <= 0 && vehiclesToAdd <= 0)
		{
			result.m_sFailureReason = "garrison full or empty request";
			return result;
		}

		if (!garrisons.AddAbstractForces(state, zone.m_sZoneId, factionKey, infantryToAdd, vehiclesToAdd))
		{
			result.m_sFailureReason = "garrison add failed";
			return result;
		}

		HST_GarrisonState afterGarrison = state.FindGarrison(zone.m_sZoneId, factionKey);
		if (!afterGarrison)
		{
			result.m_sFailureReason = "garrison missing after add";
			return result;
		}

		int addedInfantry = Math.Max(0, afterGarrison.m_iInfantryCount - beforeInfantry);
		int addedVehicles = Math.Max(0, afterGarrison.m_iVehicleCount - beforeVehicles);

		if (addedInfantry <= 0 && addedVehicles <= 0)
		{
			result.m_sFailureReason = "no garrison change";
			return result;
		}

		if (!economy.SpendFactionMoney(state, moneyCost))
		{
			garrisons.RemoveAbstractForces(state, zone.m_sZoneId, factionKey, addedInfantry, addedVehicles);
			if (!hadGarrison)
				RemoveEmptyRecruitGarrison(state, zone.m_sZoneId, factionKey);
			result.m_sFailureReason = "money spend failed after garrison add; rolled back";
			return result;
		}

		if (!economy.SpendHR(state, hrCost))
		{
			economy.AddFactionMoney(state, moneyCost);
			garrisons.RemoveAbstractForces(state, zone.m_sZoneId, factionKey, addedInfantry, addedVehicles);
			if (!hadGarrison)
				RemoveEmptyRecruitGarrison(state, zone.m_sZoneId, factionKey);
			result.m_sFailureReason = "HR spend failed after garrison add; rolled back";
			return result;
		}

		result.m_bSuccess = true;
		result.m_sZoneId = zone.m_sZoneId;
		result.m_sFactionKey = factionKey;
		result.m_iRequestedInfantry = requestedInfantry;
		result.m_iRequestedVehicles = requestedVehicles;
		result.m_iAddedInfantry = addedInfantry;
		result.m_iAddedVehicles = addedVehicles;
		result.m_iMoneySpent = moneyCost;
		result.m_iHRSpent = hrCost;
		result.m_iTrainingLevel = state.m_iTrainingLevel;
		result.m_sEquipmentTier = ResolveRecruitEquipmentTier(state, arsenal);
		return result;
	}

	protected void RemoveEmptyRecruitGarrison(HST_CampaignState state, string zoneId, string factionKey)
	{
		if (!state)
			return;

		for (int i = state.m_aGarrisons.Count() - 1; i >= 0; i--)
		{
			HST_GarrisonState garrison = state.m_aGarrisons[i];
			if (!garrison || garrison.m_sZoneId != zoneId || garrison.m_sFactionKey != factionKey)
				continue;

			if (garrison.m_iInfantryCount <= 0 && garrison.m_iVehicleCount <= 0)
				state.m_aGarrisons.Remove(i);
			return;
		}
	}

	protected string ResolveResistanceFactionKey(HST_CampaignPreset preset)
	{
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			return preset.m_sResistanceFactionKey;

		return "FIA";
	}

	protected string BuildRecruitCapacityLabel(HST_GarrisonState garrison, int slots, int activeInfantry)
	{
		if (!garrison || slots <= 0)
			return "uncapped";

		return string.Format("%1/%2", garrison.m_iInfantryCount + activeInfantry, slots);
	}

	protected string BuildRecruitUnlockSummary(HST_CampaignState state)
	{
		int unlockedWeapons;
		int unlockedMagazines;
		int unlockedLaunchers;
		int unlockedExplosives;

		foreach (HST_ArsenalItemState item : state.m_aArsenalItems)
		{
			if (!item || !item.m_bUnlocked)
				continue;

			if (item.m_sCategory == "weapon")
				unlockedWeapons++;
			else if (item.m_sCategory == "magazine")
				unlockedMagazines++;
			else if (item.m_sCategory == "launcher")
				unlockedLaunchers++;
			else if (item.m_sCategory == "explosive")
				unlockedExplosives++;
		}

		return string.Format("weapons %1 / magazines %2 / launchers %3 / explosives %4", unlockedWeapons, unlockedMagazines, unlockedLaunchers, unlockedExplosives);
	}
}
