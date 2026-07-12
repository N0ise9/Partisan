// Schema-63 restore boundary for canonical combat-presence authority. Legacy
// saves become cold baselines without inferring combat from unrelated active,
// vehicle, marker, or group rows. Physical samples are runtime observations and
// are always invalidated on restore before combat presence can be queried.
class HST_CombatPresenceSaveValidationService
{
	static const int SCHEMA_VERSION = 63;
	static const int MAX_CONTRIBUTORS = 24;
	static const int MAX_CONTRIBUTOR_ID_CHARACTERS = 160;
	static const int MAX_DIAGNOSTIC_FACT_CHARACTERS = 192;
	static const int MAX_REASON_CHARACTERS = 192;
	static const int MAX_HASH_CHARACTERS = 64;

	void Normalize(HST_CampaignSaveData saveData, int restoredSchemaVersion)
	{
		if (!saveData)
			return;

		InvalidatePhysicalSamples(saveData);
		if (restoredSchemaVersion < SCHEMA_VERSION)
		{
			foreach (HST_ZoneState legacyZone : saveData.m_aZones)
				ApplyColdBaseline(legacyZone, true);
			return;
		}

		int restoredElapsedSecond = Math.Max(0, saveData.m_iElapsedSeconds);
		foreach (HST_ZoneState zone : saveData.m_aZones)
		{
			if (!zone)
				continue;
			if (!ValidateCurrentZone(zone, restoredElapsedSecond))
				ApplyColdBaseline(zone, false);
		}
	}

	protected void InvalidatePhysicalSamples(HST_CampaignSaveData saveData)
	{
		if (!saveData)
			return;
		foreach (HST_ActiveGroupState group : saveData.m_aActiveGroups)
		{
			if (!group)
				continue;
			group.m_iCombatEffectiveInfantryCount = 0;
			group.m_iOperationalMannedVehicleCount = 0;
			group.m_iCombatEffectiveStaticOperatorCount = 0;
			group.m_iCombatPresenceSampleSecond = -1;
			group.m_bCombatPresenceSampleAuthoritative = false;
			group.m_sCombatPresenceSampleReason = "restore_requires_runtime_sample";
		}
	}

	protected bool ValidateCurrentZone(HST_ZoneState zone, int restoredElapsedSecond)
	{
		if (!zone)
			return false;
		if (!IsKnownState(zone.m_eCombatPresenceState))
			return false;
		if (zone.m_iCombatPresenceRevision <= 0)
			return false;
		if (!HasNonnegativeFields(zone))
			return false;

		if (zone.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT)
			return ValidateHot(zone, restoredElapsedSecond);
		if (zone.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COOLING)
			return ValidateCooling(zone, restoredElapsedSecond);
		return ValidateCold(zone);
	}

	protected bool IsKnownState(HST_ECombatPresenceState state)
	{
		if (state == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COLD)
			return true;
		if (state == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT)
			return true;
		return state == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COOLING;
	}

	protected bool HasNonnegativeFields(HST_ZoneState zone)
	{
		if (!zone)
			return false;
		if (zone.m_iCombatPresenceLastHotSecond < 0)
			return false;
		if (zone.m_iCombatPresenceCoolingUntilSecond < 0)
			return false;
		if (zone.m_iCombatPresenceInfantryCount < 0)
			return false;
		if (zone.m_iCombatPresenceMannedVehicleCount < 0)
			return false;
		if (zone.m_iCombatPresenceStaticOperatorCount < 0)
			return false;
		if (zone.m_iCombatPresenceCurrentOperationCount < 0)
			return false;
		return zone.m_iCombatPresenceRecentFireCount >= 0;
	}

	protected bool ValidateHot(HST_ZoneState zone, int restoredElapsedSecond)
	{
		if (!zone)
			return false;
		bool hasLivingContributor = zone.m_iCombatPresenceInfantryCount > 0;
		hasLivingContributor = hasLivingContributor
			|| zone.m_iCombatPresenceMannedVehicleCount > 0;
		hasLivingContributor = hasLivingContributor
			|| zone.m_iCombatPresenceStaticOperatorCount > 0;
		if (!hasLivingContributor)
			return false;
		if (zone.m_iCombatPresenceLastHotSecond > restoredElapsedSecond)
			return false;
		if (zone.m_iCombatPresenceCoolingUntilSecond != 0)
			return false;
		if (!IsValidReason(zone.m_sCombatPresenceReason))
			return false;
		if (!ValidateDiagnosticPairs(zone, false))
			return false;

		string expectedHash = HST_CombatPresenceService.BuildContributorHash(
			zone.m_aCombatPresenceContributorIds,
			zone.m_aCombatPresenceContributorFacts,
			zone.m_iCombatPresenceInfantryCount,
			zone.m_iCombatPresenceMannedVehicleCount,
			zone.m_iCombatPresenceStaticOperatorCount,
			zone.m_iCombatPresenceCurrentOperationCount,
			zone.m_iCombatPresenceRecentFireCount);
		if (zone.m_sCombatPresenceContributorHash != expectedHash)
			return false;

		if (zone.m_aCombatPresenceContributorIds.Count() <= MAX_CONTRIBUTORS)
			return true;
		CapDiagnostics(zone);
		zone.m_sCombatPresenceContributorHash = HST_CombatPresenceService.BuildContributorHash(
			zone.m_aCombatPresenceContributorIds,
			zone.m_aCombatPresenceContributorFacts,
			zone.m_iCombatPresenceInfantryCount,
			zone.m_iCombatPresenceMannedVehicleCount,
			zone.m_iCombatPresenceStaticOperatorCount,
			zone.m_iCombatPresenceCurrentOperationCount,
			zone.m_iCombatPresenceRecentFireCount);
		return true;
	}

	protected bool ValidateCooling(HST_ZoneState zone, int restoredElapsedSecond)
	{
		if (!zone)
			return false;
		if (zone.m_iCombatPresenceInfantryCount != 0)
			return false;
		if (zone.m_iCombatPresenceMannedVehicleCount != 0)
			return false;
		if (zone.m_iCombatPresenceStaticOperatorCount != 0)
			return false;
		if (zone.m_iCombatPresenceCurrentOperationCount != 0)
			return false;
		if (zone.m_iCombatPresenceRecentFireCount != 0)
			return false;
		if (zone.m_iCombatPresenceCoolingUntilSecond <= 0)
			return false;
		if (zone.m_iCombatPresenceLastHotSecond > restoredElapsedSecond)
			return false;
		if (zone.m_iCombatPresenceCoolingUntilSecond <= restoredElapsedSecond)
			return false;
		int coolingDuration = zone.m_iCombatPresenceCoolingUntilSecond
			- zone.m_iCombatPresenceLastHotSecond;
		if (coolingDuration < 1 || coolingDuration > 300)
			return false;
		if (zone.m_sCombatPresenceReason != "combat area cooling")
			return false;
		if (!ValidateDiagnosticPairs(zone, true))
			return false;
		return IsCanonicalContributorHash(zone.m_sCombatPresenceContributorHash);
	}

	protected bool ValidateCold(HST_ZoneState zone)
	{
		if (!zone)
			return false;
		if (zone.m_iCombatPresenceLastHotSecond != 0)
			return false;
		if (zone.m_iCombatPresenceCoolingUntilSecond != 0)
			return false;
		if (zone.m_iCombatPresenceInfantryCount != 0)
			return false;
		if (zone.m_iCombatPresenceMannedVehicleCount != 0)
			return false;
		if (zone.m_iCombatPresenceStaticOperatorCount != 0)
			return false;
		if (zone.m_iCombatPresenceCurrentOperationCount != 0)
			return false;
		if (zone.m_iCombatPresenceRecentFireCount != 0)
			return false;
		if (!zone.m_sCombatPresenceContributorHash.IsEmpty())
			return false;
		if (zone.m_sCombatPresenceReason != "cold")
			return false;
		if (!zone.m_aCombatPresenceContributorIds)
			return false;
		if (!zone.m_aCombatPresenceContributorFacts)
			return false;
		if (zone.m_aCombatPresenceContributorIds.Count() != 0)
			return false;
		return zone.m_aCombatPresenceContributorFacts.Count() == 0;
	}

	protected bool ValidateDiagnosticPairs(HST_ZoneState zone, bool enforceMaximum)
	{
		if (!zone)
			return false;
		if (!zone.m_aCombatPresenceContributorIds)
			return false;
		if (!zone.m_aCombatPresenceContributorFacts)
			return false;
		int pairCount = zone.m_aCombatPresenceContributorIds.Count();
		if (pairCount <= 0)
			return false;
		if (zone.m_aCombatPresenceContributorFacts.Count() != pairCount)
			return false;
		if (enforceMaximum && pairCount > MAX_CONTRIBUTORS)
			return false;

		map<string, bool> seenIds = new map<string, bool>();
		string previousId;
		for (int pairIndex; pairIndex < pairCount; pairIndex++)
		{
			string contributorId = zone.m_aCombatPresenceContributorIds[pairIndex];
			string contributorFact = zone.m_aCombatPresenceContributorFacts[pairIndex];
			if (!IsValidContributorId(contributorId))
				return false;
			if (!IsValidContributorFact(contributorFact))
				return false;
			if (seenIds.Contains(contributorId))
				return false;
			if (pairIndex > 0 && contributorId.Compare(previousId) <= 0)
				return false;
			seenIds.Set(contributorId, true);
			previousId = contributorId;
		}
		return true;
	}

	protected bool IsValidContributorId(string value)
	{
		if (value.IsEmpty() || value.Length() > MAX_CONTRIBUTOR_ID_CHARACTERS)
			return false;
		return !value.Trim().IsEmpty();
	}

	protected bool IsValidContributorFact(string value)
	{
		if (value.IsEmpty() || value.Length() > MAX_DIAGNOSTIC_FACT_CHARACTERS)
			return false;
		return !value.Trim().IsEmpty();
	}

	protected bool IsValidReason(string value)
	{
		if (value.IsEmpty() || value.Length() > MAX_REASON_CHARACTERS)
			return false;
		return !value.Trim().IsEmpty();
	}

	protected bool IsCanonicalContributorHash(string value)
	{
		if (value.IsEmpty() || value.Length() > MAX_HASH_CHARACTERS)
			return false;
		array<string> parts = {};
		value.Split("_", parts, true);
		if (parts.Count() != 3 || parts[0] != "cp1")
			return false;
		if (!IsCanonicalSignedInteger(parts[1]))
			return false;
		return IsCanonicalSignedInteger(parts[2]);
	}

	protected bool IsCanonicalSignedInteger(string value)
	{
		if (value.IsEmpty())
			return false;
		int startIndex;
		if (value.StartsWith("-"))
		{
			if (value.Length() == 1)
				return false;
			startIndex = 1;
		}
		for (int characterIndex = startIndex; characterIndex < value.Length(); characterIndex++)
		{
			if (!value.IsDigitAt(characterIndex))
				return false;
		}
		int parsed = value.ToInt();
		return parsed.ToString() == value;
	}

	protected void CapDiagnostics(HST_ZoneState zone)
	{
		if (!zone || !zone.m_aCombatPresenceContributorIds
			|| !zone.m_aCombatPresenceContributorFacts)
			return;
		while (zone.m_aCombatPresenceContributorIds.Count() > MAX_CONTRIBUTORS)
			zone.m_aCombatPresenceContributorIds.Remove(
				zone.m_aCombatPresenceContributorIds.Count() - 1);
		while (zone.m_aCombatPresenceContributorFacts.Count() > MAX_CONTRIBUTORS)
			zone.m_aCombatPresenceContributorFacts.Remove(
				zone.m_aCombatPresenceContributorFacts.Count() - 1);
	}

	protected void ApplyColdBaseline(HST_ZoneState zone, bool resetRevision)
	{
		if (!zone)
			return;
		zone.m_eCombatPresenceState = HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COLD;
		zone.m_iCombatPresenceLastHotSecond = 0;
		zone.m_iCombatPresenceCoolingUntilSecond = 0;
		if (resetRevision || zone.m_iCombatPresenceRevision <= 0)
			zone.m_iCombatPresenceRevision = 1;
		zone.m_iCombatPresenceInfantryCount = 0;
		zone.m_iCombatPresenceMannedVehicleCount = 0;
		zone.m_iCombatPresenceStaticOperatorCount = 0;
		zone.m_iCombatPresenceCurrentOperationCount = 0;
		zone.m_iCombatPresenceRecentFireCount = 0;
		zone.m_sCombatPresenceContributorHash = "";
		zone.m_sCombatPresenceReason = "cold";
		EnsureDiagnosticArrays(zone);
		zone.m_aCombatPresenceContributorIds.Clear();
		zone.m_aCombatPresenceContributorFacts.Clear();
	}

	protected void EnsureDiagnosticArrays(HST_ZoneState zone)
	{
		if (!zone)
			return;
		if (!zone.m_aCombatPresenceContributorIds)
			zone.m_aCombatPresenceContributorIds = new array<string>();
		if (!zone.m_aCombatPresenceContributorFacts)
			zone.m_aCombatPresenceContributorFacts = new array<string>();
	}
}
