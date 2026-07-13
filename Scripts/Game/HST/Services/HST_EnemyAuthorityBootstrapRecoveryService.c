class HST_EnemyAuthorityBootstrapRecoveryResult
{
	bool m_bAttempted;
	bool m_bSignatureMatched;
	bool m_bRecovered;
	bool m_bPoolsExact;
	bool m_bPlanningExact;
	string m_sFailureReason;
	string m_sEvidence;

	string BuildReport()
	{
		string report = string.Format(
			"Partisan enemy authority bootstrap recovery | attempted %1 | signature %2 | recovered %3 | pools %4 | planning %5",
			m_bAttempted,
			m_bSignatureMatched,
			m_bRecovered,
			m_bPoolsExact,
			m_bPlanningExact);
		if (!m_sFailureReason.IsEmpty())
			report = report + " | reason " + m_sFailureReason;
		if (!m_sEvidence.IsEmpty())
			report = report + " | " + m_sEvidence;
		return report;
	}
}

// One-time, signature-gated repair for the exact current-schema bootstrap
// quarantine produced when a fresh state reached the restored-role validators
// before its configured enemy authority rows existed. This is not a generic
// quarantine repair: any identity, field, receipt, or planning-order divergence
// leaves the restored state untouched and fail-closed.
class HST_EnemyAuthorityBootstrapRecoveryService
{
	static const int TARGET_SCHEMA_VERSION = 68;
	static const string STRATEGIC_BOOTSTRAP_FAILURE = "schema67 configured enemy pool failed live-role validation";
	static const string PLANNING_BOOTSTRAP_FAILURE = "schema68 planning contract, faction, or failure state is invalid";

	HST_EnemyAuthorityBootstrapRecoveryResult RecoverKnownSchema68BootstrapQuarantine(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		HST_CampaignPreset preset)
	{
		HST_EnemyAuthorityBootstrapRecoveryResult result
			= new HST_EnemyAuthorityBootstrapRecoveryResult();
		result.m_bAttempted = true;

		int occupierPoolIndex;
		int invaderPoolIndex;
		int occupierPlanningIndex;
		int invaderPlanningIndex;
		string mismatch;
		if (!TryMatchKnownBootstrapQuarantine(
			state,
			balance,
			preset,
			occupierPoolIndex,
			invaderPoolIndex,
			occupierPlanningIndex,
			invaderPlanningIndex,
			mismatch))
		{
			result.m_sFailureReason = mismatch;
			result.m_sEvidence = BuildStateEvidence(state, preset);
			return result;
		}
		result.m_bSignatureMatched = true;

		HST_FactionPoolState occupierReplacement = BuildBaselinePool(
			preset.m_sOccupierFactionKey,
			balance.m_iStartingOccupierAttackPool,
			balance.m_iStartingOccupierSupportPool,
			state.m_iElapsedSeconds);
		HST_FactionPoolState invaderReplacement = BuildBaselinePool(
			preset.m_sInvaderFactionKey,
			balance.m_iStartingInvaderAttackPool,
			balance.m_iStartingInvaderSupportPool,
			state.m_iElapsedSeconds);
		HST_EnemyPlanningAuthorityService authority
			= new HST_EnemyPlanningAuthorityService();
		HST_EnemyPlanningState occupierPlanningReplacement
			= authority.BuildBaselineState(
				preset.m_sOccupierFactionKey,
				state.m_iElapsedSeconds);
		HST_EnemyPlanningState invaderPlanningReplacement
			= authority.BuildBaselineState(
				preset.m_sInvaderFactionKey,
				state.m_iElapsedSeconds);

		if (!IsCanonicalPool(
			occupierReplacement,
			preset.m_sOccupierFactionKey,
			balance.m_iStartingOccupierAttackPool,
			balance.m_iStartingOccupierSupportPool,
			state.m_iElapsedSeconds)
			|| !IsCanonicalPool(
				invaderReplacement,
				preset.m_sInvaderFactionKey,
				balance.m_iStartingInvaderAttackPool,
				balance.m_iStartingInvaderSupportPool,
				state.m_iElapsedSeconds)
			|| !IsCanonicalPlanning(
				occupierPlanningReplacement,
				preset.m_sOccupierFactionKey,
				state.m_iElapsedSeconds)
			|| !IsCanonicalPlanning(
				invaderPlanningReplacement,
				preset.m_sInvaderFactionKey,
				state.m_iElapsedSeconds))
		{
			result.m_sFailureReason
				= "canonical enemy authority baseline could not be built";
			result.m_sEvidence = BuildStateEvidence(state, preset);
			return result;
		}

		ref HST_FactionPoolState previousOccupierPool
			= state.m_aFactionPools[occupierPoolIndex];
		ref HST_FactionPoolState previousInvaderPool
			= state.m_aFactionPools[invaderPoolIndex];
		ref HST_EnemyPlanningState previousOccupierPlanning
			= state.m_aEnemyPlanningStates[occupierPlanningIndex];
		ref HST_EnemyPlanningState previousInvaderPlanning
			= state.m_aEnemyPlanningStates[invaderPlanningIndex];

		state.m_aFactionPools[occupierPoolIndex] = occupierReplacement;
		state.m_aFactionPools[invaderPoolIndex] = invaderReplacement;
		state.m_aEnemyPlanningStates[occupierPlanningIndex]
			= occupierPlanningReplacement;
		state.m_aEnemyPlanningStates[invaderPlanningIndex]
			= invaderPlanningReplacement;

		result.m_bPoolsExact = AreCanonicalPools(
			state,
			balance,
			preset,
			state.m_iElapsedSeconds);
		result.m_bPlanningExact = AreCanonicalPlanningRows(
			state,
			preset,
			state.m_iElapsedSeconds);
		result.m_bRecovered = result.m_bPoolsExact
			&& result.m_bPlanningExact;
		if (!result.m_bRecovered)
		{
			// The match was proven before mutation. Still restore the four original
			// references if a future baseline builder changes this contract.
			state.m_aFactionPools[occupierPoolIndex] = previousOccupierPool;
			state.m_aFactionPools[invaderPoolIndex] = previousInvaderPool;
			state.m_aEnemyPlanningStates[occupierPlanningIndex]
				= previousOccupierPlanning;
			state.m_aEnemyPlanningStates[invaderPlanningIndex]
				= previousInvaderPlanning;
			result.m_bPoolsExact = false;
			result.m_bPlanningExact = false;
			result.m_sFailureReason
				= "post-replacement enemy authority verification failed";
		}
		result.m_sEvidence = BuildStateEvidence(state, preset);
		return result;
	}

	bool IsCanonicalBaselineAtSecond(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		HST_CampaignPreset preset,
		int baselineSecond)
	{
		return AreCanonicalPools(state, balance, preset, baselineSecond)
			&& AreCanonicalPlanningRows(state, preset, baselineSecond);
	}

	protected bool TryMatchKnownBootstrapQuarantine(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		HST_CampaignPreset preset,
		out int occupierPoolIndex,
		out int invaderPoolIndex,
		out int occupierPlanningIndex,
		out int invaderPlanningIndex,
		out string mismatch)
	{
		occupierPoolIndex = -1;
		invaderPoolIndex = -1;
		occupierPlanningIndex = -1;
		invaderPlanningIndex = -1;
		mismatch = "";
		if (!state || !balance || !preset)
		{
			mismatch = "state, balance, or preset is unavailable";
			return false;
		}
		if (state.m_iSchemaVersion != TARGET_SCHEMA_VERSION
			|| state.m_iLastLoadedSchemaVersion != TARGET_SCHEMA_VERSION
			|| HST_CampaignState.SCHEMA_VERSION != TARGET_SCHEMA_VERSION
			|| !state.m_bRestoredFromPersistence)
		{
			mismatch = "state is not an exact restored Schema-68 envelope";
			return false;
		}
		string presetId = preset.m_sPresetId.Trim();
		if (presetId.IsEmpty() || presetId != preset.m_sPresetId
			|| state.m_sPresetId != presetId)
		{
			mismatch = "restored campaign preset identity is not exact";
			return false;
		}
		if (!PresetRolesExact(preset))
		{
			mismatch = "configured enemy roles are not exact and distinct";
			return false;
		}
		if (balance.m_iStartingOccupierAttackPool < 0
			|| balance.m_iStartingOccupierSupportPool < 0
			|| balance.m_iStartingInvaderAttackPool < 0
			|| balance.m_iStartingInvaderSupportPool < 0)
		{
			mismatch = "configured enemy starting resources are invalid";
			return false;
		}
		if (state.m_iElapsedSeconds < 0
			|| state.m_iElapsedSeconds
				> int.MAX - HST_EnemyCommanderService.ORDER_TICK_SECONDS
			|| HST_EnemyCommanderService.ORDER_TICK_SECONDS
				!= HST_EnemyPlanningAuthorityService.PLANNING_INTERVAL_SECONDS)
		{
			mismatch = "campaign clock or planning interval is invalid";
			return false;
		}
		if (!state.m_aFactionPools || !state.m_aEnemyPlanningStates
			|| !state.m_aEnemyStrategicMutations || !state.m_aEnemyOrders)
		{
			mismatch = "enemy authority arrays are unavailable";
			return false;
		}
		if (!state.m_aEnemyStrategicMutations.IsEmpty())
		{
			mismatch = "strategic mutation evidence exists";
			return false;
		}
		if (!state.m_aEnemyOrders.IsEmpty())
		{
			mismatch = "enemy planning order evidence exists";
			return false;
		}

		HST_FactionPoolState resistancePool;
		if (state.m_aFactionPools.Count() != 3
			|| !FindUniquePoisonPool(
			state,
			preset.m_sOccupierFactionKey,
			occupierPoolIndex)
			|| !FindUniquePoisonPool(
			state,
			preset.m_sInvaderFactionKey,
			invaderPoolIndex)
			|| !FindUniquePool(
				state,
				preset.m_sResistanceFactionKey,
				resistancePool)
			|| !IsNeutralResistancePool(
				resistancePool,
				preset.m_sResistanceFactionKey))
		{
			mismatch = "three-role faction-pool quarantine topology is not exact";
			return false;
		}

		if (state.m_aEnemyPlanningStates.Count() != 2
			|| !FindUniquePoisonPlanning(
				state,
				preset.m_sOccupierFactionKey,
				occupierPlanningIndex)
			|| !FindUniquePoisonPlanning(
				state,
				preset.m_sInvaderFactionKey,
				invaderPlanningIndex))
		{
			mismatch = "configured enemy planning quarantine is not exact";
			return false;
		}
		return true;
	}

	protected bool PresetRolesExact(HST_CampaignPreset preset)
	{
		if (!preset)
			return false;
		string resistance = preset.m_sResistanceFactionKey.Trim();
		string occupier = preset.m_sOccupierFactionKey.Trim();
		string invader = preset.m_sInvaderFactionKey.Trim();
		return !resistance.IsEmpty() && !occupier.IsEmpty() && !invader.IsEmpty()
			&& resistance == preset.m_sResistanceFactionKey
			&& occupier == preset.m_sOccupierFactionKey
			&& invader == preset.m_sInvaderFactionKey
			&& resistance != occupier && resistance != invader
			&& occupier != invader;
	}

	protected bool FindUniquePoisonPool(
		HST_CampaignState state,
		string factionKey,
		out int matchIndex)
	{
		matchIndex = -1;
		for (int poolIndex = 0; poolIndex < state.m_aFactionPools.Count(); poolIndex++)
		{
			HST_FactionPoolState pool = state.m_aFactionPools[poolIndex];
			if (!pool || pool.m_sFactionKey != factionKey)
				continue;
			if (matchIndex >= 0 || !IsExactPoisonPool(pool, factionKey))
				return false;
			matchIndex = poolIndex;
		}
		return matchIndex >= 0;
	}

	protected bool FindUniquePoisonPlanning(
		HST_CampaignState state,
		string factionKey,
		out int matchIndex)
	{
		matchIndex = -1;
		for (int planningIndex = 0; planningIndex < state.m_aEnemyPlanningStates.Count(); planningIndex++)
		{
			HST_EnemyPlanningState planning
				= state.m_aEnemyPlanningStates[planningIndex];
			if (!planning || planning.m_sFactionKey != factionKey)
				continue;
			if (matchIndex >= 0
				|| !IsExactPoisonPlanning(planning, factionKey))
				return false;
			matchIndex = planningIndex;
		}
		return matchIndex >= 0;
	}

	protected bool IsExactPoisonPool(
		HST_FactionPoolState pool,
		string factionKey)
	{
		return pool && pool.m_sFactionKey == factionKey
			&& pool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceSaveValidationService.QUARANTINE_CONTRACT_VERSION
			&& pool.m_iStrategicRevision == 0
			&& pool.m_iStrategicOperationalMutationCount == 0
			&& pool.m_iAttackResources == 0
			&& pool.m_iSupportResources == 0
			&& pool.m_iMoney == 0 && pool.m_iHR == 0
			&& pool.m_iAggression == 0
			&& pool.m_iResourceAccumulatorSeconds == 0
			&& pool.m_iAggressionAccumulatorSeconds == 0
			&& pool.m_iLastResourceBucketSecond == 0
			&& pool.m_iLastAggressionBucketSecond == 0
			&& pool.m_sLastStrategicMutationId.IsEmpty()
			&& pool.m_sStrategicAuthorityFailure
				== STRATEGIC_BOOTSTRAP_FAILURE;
	}

	protected bool IsExactPoisonPlanning(
		HST_EnemyPlanningState planning,
		string factionKey)
	{
		return planning && planning.m_sFactionKey == factionKey
			&& planning.m_iContractVersion
				== HST_EnemyPlanningSaveValidationService.QUARANTINE_CONTRACT_VERSION
			&& planning.m_iRevision == 1
			&& planning.m_iLastPlanningBucketSecond == 0
			&& planning.m_iNextPlanningBucketSecond == 0
			&& HasUntouchedDecisionFields(planning)
			&& planning.m_sDisposition == "quarantined"
			&& planning.m_sFailureReason.IsEmpty()
			&& planning.m_sAuthorityFailure == PLANNING_BOOTSTRAP_FAILURE;
	}

	protected bool IsNeutralResistancePool(
		HST_FactionPoolState pool,
		string factionKey)
	{
		return pool && pool.m_sFactionKey == factionKey
			&& pool.m_iStrategicContractVersion == 0
			&& pool.m_sStrategicAuthorityFailure.IsEmpty();
	}

	protected bool HasUntouchedDecisionFields(HST_EnemyPlanningState planning)
	{
		if (!planning)
			return false;
		if (planning.m_iDecisionSequence != 0
			|| planning.m_iDecisionBucketSecond != 0
			|| planning.m_iNextRetrySecond != 0
			|| planning.m_iObservedWarLevel != 0
			|| planning.m_iObservedAggression != 0)
			return false;
		if (planning.m_iObservedPoolRevision != 0
			|| planning.m_iObservedOperationalMutationCount != 0
			|| planning.m_iObservedAttackResources != 0
			|| planning.m_iObservedSupportResources != 0
			|| planning.m_iCommitmentCount != 0)
			return false;
		if (!planning.m_sCommitmentFingerprint.IsEmpty()
			|| planning.m_iTargetCandidateCount != 0
			|| !planning.m_sTargetCandidateFingerprint.IsEmpty()
			|| planning.m_iSourceCandidateCount != 0
			|| !planning.m_sSourceCandidateFingerprint.IsEmpty())
			return false;
		if (!planning.m_sSelectedTargetZoneId.IsEmpty()
			|| !planning.m_sSelectedSourceZoneId.IsEmpty()
			|| planning.m_eSelectedOrderType != HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL
			|| planning.m_ePlannedSupportType != HST_ESupportRequestType.HST_SUPPORT_QRF
			|| !planning.m_sPlanningCapabilityHash.IsEmpty())
			return false;
		if (!planning.m_sSpendMode.IsEmpty()
			|| planning.m_iAttackCost != 0
			|| planning.m_iSupportCost != 0
			|| planning.m_iTargetPressureBefore != 0
			|| planning.m_iTargetPressureDelta != 0)
			return false;
		if (planning.m_iTargetPressureAfter != 0
			|| planning.m_bTargetPressureApplied
			|| !planning.m_sDecisionId.IsEmpty()
			|| !planning.m_sPlannedOrderId.IsEmpty()
			|| !planning.m_sPlannedOperationId.IsEmpty())
			return false;
		if (!planning.m_sPlannedManifestId.IsEmpty()
			|| !planning.m_sPlannedManifestHash.IsEmpty()
			|| !planning.m_sPlannedDebitMutationId.IsEmpty()
			|| !planning.m_sInputFingerprint.IsEmpty()
			|| !planning.m_sDecisionFingerprint.IsEmpty())
			return false;
		return true;
	}

	protected HST_FactionPoolState BuildBaselinePool(
		string factionKey,
		int attackResources,
		int supportResources,
		int baselineSecond)
	{
		HST_FactionPoolState pool = new HST_FactionPoolState();
		pool.m_sFactionKey = factionKey;
		pool.m_iStrategicContractVersion
			= HST_EnemyStrategicResourceSaveValidationService.CONTRACT_VERSION;
		pool.m_iStrategicRevision = 1;
		pool.m_iAttackResources = attackResources;
		pool.m_iSupportResources = supportResources;
		pool.m_iAggression = 0;
		pool.m_iStrategicOperationalMutationCount = 0;
		pool.m_iResourceAccumulatorSeconds = 0;
		pool.m_iAggressionAccumulatorSeconds = 0;
		pool.m_iLastResourceBucketSecond = baselineSecond;
		pool.m_iLastAggressionBucketSecond = baselineSecond;
		pool.m_sLastStrategicMutationId = "";
		pool.m_sStrategicAuthorityFailure = "";
		return pool;
	}

	protected bool AreCanonicalPools(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		HST_CampaignPreset preset,
		int baselineSecond)
	{
		if (!state || !balance || !preset || !state.m_aFactionPools
			|| state.m_aFactionPools.Count() != 3)
			return false;
		HST_FactionPoolState occupier;
		HST_FactionPoolState invader;
		HST_FactionPoolState resistance;
		if (!FindUniquePool(
			state,
			preset.m_sOccupierFactionKey,
			occupier)
			|| !FindUniquePool(
				state,
				preset.m_sInvaderFactionKey,
				invader)
			|| !FindUniquePool(
				state,
				preset.m_sResistanceFactionKey,
				resistance)
			|| !IsNeutralResistancePool(
				resistance,
				preset.m_sResistanceFactionKey))
			return false;
		return IsCanonicalPool(
			occupier,
			preset.m_sOccupierFactionKey,
			balance.m_iStartingOccupierAttackPool,
			balance.m_iStartingOccupierSupportPool,
			baselineSecond)
			&& IsCanonicalPool(
				invader,
				preset.m_sInvaderFactionKey,
				balance.m_iStartingInvaderAttackPool,
				balance.m_iStartingInvaderSupportPool,
				baselineSecond);
	}

	protected bool IsCanonicalPool(
		HST_FactionPoolState pool,
		string factionKey,
		int attackResources,
		int supportResources,
		int baselineSecond)
	{
		return pool && pool.m_sFactionKey == factionKey
			&& pool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceSaveValidationService.CONTRACT_VERSION
			&& pool.m_iStrategicRevision == 1
			&& pool.m_iStrategicOperationalMutationCount == 0
			&& pool.m_iAttackResources == attackResources
			&& pool.m_iSupportResources == supportResources
			&& pool.m_iMoney == 0 && pool.m_iHR == 0
			&& pool.m_iAggression == 0
			&& pool.m_iResourceAccumulatorSeconds == 0
			&& pool.m_iAggressionAccumulatorSeconds == 0
			&& pool.m_iLastResourceBucketSecond == baselineSecond
			&& pool.m_iLastAggressionBucketSecond == baselineSecond
			&& pool.m_sLastStrategicMutationId.IsEmpty()
			&& pool.m_sStrategicAuthorityFailure.IsEmpty();
	}

	protected bool AreCanonicalPlanningRows(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		int baselineSecond)
	{
		if (!state || !preset || !state.m_aEnemyPlanningStates
			|| state.m_aEnemyPlanningStates.Count() != 2)
			return false;
		HST_EnemyPlanningState occupier;
		HST_EnemyPlanningState invader;
		if (!FindUniquePlanning(
			state,
			preset.m_sOccupierFactionKey,
			occupier)
			|| !FindUniquePlanning(
				state,
				preset.m_sInvaderFactionKey,
				invader))
			return false;
		return IsCanonicalPlanning(
			occupier,
			preset.m_sOccupierFactionKey,
			baselineSecond)
			&& IsCanonicalPlanning(
				invader,
				preset.m_sInvaderFactionKey,
				baselineSecond);
	}

	protected bool IsCanonicalPlanning(
		HST_EnemyPlanningState planning,
		string factionKey,
		int baselineSecond)
	{
		return planning && planning.m_sFactionKey == factionKey
			&& planning.m_iContractVersion
				== HST_EnemyPlanningSaveValidationService.CONTRACT_VERSION
			&& planning.m_iRevision == 1
			&& planning.m_iLastPlanningBucketSecond == baselineSecond
			&& planning.m_iNextPlanningBucketSecond
				== baselineSecond + HST_EnemyCommanderService.ORDER_TICK_SECONDS
			&& HasUntouchedDecisionFields(planning)
			&& planning.m_sDisposition == "idle"
			&& planning.m_sFailureReason.IsEmpty()
			&& planning.m_sAuthorityFailure.IsEmpty();
	}

	protected bool FindUniquePool(
		HST_CampaignState state,
		string factionKey,
		out HST_FactionPoolState match)
	{
		match = null;
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || pool.m_sFactionKey != factionKey)
				continue;
			if (match)
				return false;
			match = pool;
		}
		return match != null;
	}

	protected bool FindUniquePlanning(
		HST_CampaignState state,
		string factionKey,
		out HST_EnemyPlanningState match)
	{
		match = null;
		foreach (HST_EnemyPlanningState planning : state.m_aEnemyPlanningStates)
		{
			if (!planning || planning.m_sFactionKey != factionKey)
				continue;
			if (match)
				return false;
			match = planning;
		}
		return match != null;
	}

	protected string BuildStateEvidence(
		HST_CampaignState state,
		HST_CampaignPreset preset)
	{
		if (!state)
			return "state unavailable";
		int poolCount;
		int planningCount;
		int mutationCount;
		int orderCount;
		if (state.m_aFactionPools)
			poolCount = state.m_aFactionPools.Count();
		if (state.m_aEnemyPlanningStates)
			planningCount = state.m_aEnemyPlanningStates.Count();
		if (state.m_aEnemyStrategicMutations)
			mutationCount = state.m_aEnemyStrategicMutations.Count();
		if (state.m_aEnemyOrders)
			orderCount = state.m_aEnemyOrders.Count();
		string roles = "roles unavailable";
		if (preset)
			roles = preset.m_sOccupierFactionKey + "/"
				+ preset.m_sInvaderFactionKey;
		string evidence = string.Format(
			"schema %1/%2 | restored %3 | elapsed %4 | roles %5 | pools %6 | planners %7 | mutations %8 | orders %9",
			state.m_iSchemaVersion,
			state.m_iLastLoadedSchemaVersion,
			state.m_bRestoredFromPersistence,
			state.m_iElapsedSeconds,
			roles,
			poolCount,
			planningCount,
			mutationCount,
			orderCount);
		if (!preset)
			return evidence;
		evidence = evidence + " | " + BuildFactionEvidence(
			"occupier",
			FindFirstPoolSafe(state, preset.m_sOccupierFactionKey),
			FindFirstPlanningSafe(state, preset.m_sOccupierFactionKey));
		evidence = evidence + " | " + BuildFactionEvidence(
			"invader",
			FindFirstPoolSafe(state, preset.m_sInvaderFactionKey),
			FindFirstPlanningSafe(state, preset.m_sInvaderFactionKey));
		return evidence;
	}

	protected HST_FactionPoolState FindFirstPoolSafe(
		HST_CampaignState state,
		string factionKey)
	{
		if (!state || !state.m_aFactionPools)
			return null;
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (pool && pool.m_sFactionKey == factionKey)
				return pool;
		}
		return null;
	}

	protected HST_EnemyPlanningState FindFirstPlanningSafe(
		HST_CampaignState state,
		string factionKey)
	{
		if (!state || !state.m_aEnemyPlanningStates)
			return null;
		foreach (HST_EnemyPlanningState planning : state.m_aEnemyPlanningStates)
		{
			if (planning && planning.m_sFactionKey == factionKey)
				return planning;
		}
		return null;
	}

	protected string BuildFactionEvidence(
		string label,
		HST_FactionPoolState pool,
		HST_EnemyPlanningState planning)
	{
		if (!pool || !planning)
			return label + " authority unavailable";
		string evidence = string.Format(
			"%1 %2 | pool c/r %3/%4 resources %5/%6 buckets %7/%8 | planner c %9",
			label,
			pool.m_sFactionKey,
			pool.m_iStrategicContractVersion,
			pool.m_iStrategicRevision,
			pool.m_iAttackResources,
			pool.m_iSupportResources,
			pool.m_iLastResourceBucketSecond,
			pool.m_iLastAggressionBucketSecond,
			planning.m_iContractVersion);
		return evidence + string.Format(
			" revision %1 cadence %2 -> %3 disposition %4 | pool/planner failure %5/%6",
			planning.m_iRevision,
			planning.m_iLastPlanningBucketSecond,
			planning.m_iNextPlanningBucketSecond,
			planning.m_sDisposition,
			!pool.m_sStrategicAuthorityFailure.IsEmpty(),
			!planning.m_sAuthorityFailure.IsEmpty());
	}
}

class HST_EnemyAuthorityBootstrapRecoveryProofReport
{
	bool m_bExactRecovery;
	bool m_bUnrelatedStatePreserved;
	bool m_bNearMissRejected;
	bool m_bVersionedOrderRejected;
	bool m_bIdempotent;
	bool m_bRoundtripExact;
	bool m_bValidatorsAccept;
	string m_sRecoveryEvidence;
	string m_sRejectionEvidence;
	string m_sRoundtripEvidence;

	bool AllExact()
	{
		return m_bExactRecovery
			&& m_bUnrelatedStatePreserved
			&& m_bNearMissRejected
			&& m_bVersionedOrderRejected
			&& m_bIdempotent
			&& m_bRoundtripExact
			&& m_bValidatorsAccept;
	}

	string BuildReport()
	{
		string report = string.Format(
			"enemy bootstrap recovery proof | all %1 | exact %2 | unrelated %3 | near-miss %4 | versioned-order %5 | idempotent %6 | roundtrip %7 | validators %8",
			AllExact(),
			m_bExactRecovery,
			m_bUnrelatedStatePreserved,
			m_bNearMissRejected,
			m_bVersionedOrderRejected,
			m_bIdempotent,
			m_bRoundtripExact,
			m_bValidatorsAccept);
		if (!m_sRecoveryEvidence.IsEmpty())
			report = report + " | " + m_sRecoveryEvidence;
		if (!m_sRejectionEvidence.IsEmpty())
			report = report + " | " + m_sRejectionEvidence;
		if (!m_sRoundtripEvidence.IsEmpty())
			report = report + " | " + m_sRoundtripEvidence;
		return report;
	}
}

// Deterministic source proof for the exact bootstrap signature, rejection
// boundary, idempotence, and save roundtrip. Campaign Debug can surface this
// report without mutating the live campaign state.
class HST_EnemyAuthorityBootstrapRecoveryProofService
{
	static const int TARGET_SCHEMA_VERSION = 68;
	static const string PRESET_ID = "proof_enemy_bootstrap_recovery";
	static const string RESISTANCE_FACTION = "FIA";
	static const string OCCUPIER_FACTION = "US";
	static const string INVADER_FACTION = "USSR";
	static const int BASELINE_SECOND = 295;
	protected ref HST_EnemyAuthorityBootstrapRecoveryService m_Recovery
		= new HST_EnemyAuthorityBootstrapRecoveryService();

	HST_EnemyAuthorityBootstrapRecoveryProofReport BuildReport()
	{
		HST_EnemyAuthorityBootstrapRecoveryProofReport report
			= new HST_EnemyAuthorityBootstrapRecoveryProofReport();
		HST_CampaignPreset preset = BuildPreset();
		HST_BalanceConfig balance = BuildBalance();
		HST_CampaignState exact = BuildPoisonFixture();

		HST_FactionPoolState resistanceBefore
			= exact.FindFactionPool(RESISTANCE_FACTION);
		HST_PlayerState playerBefore = exact.FindPlayer("proof_player");
		HST_EnemyAuthorityBootstrapRecoveryResult recovered
			= m_Recovery.RecoverKnownSchema68BootstrapQuarantine(
				exact,
				balance,
				preset);
		report.m_bExactRecovery = recovered
			&& recovered.m_bAttempted
			&& recovered.m_bSignatureMatched
			&& recovered.m_bRecovered
			&& recovered.m_bPoolsExact
			&& recovered.m_bPlanningExact
			&& m_Recovery.IsCanonicalBaselineAtSecond(
				exact,
				balance,
				preset,
				BASELINE_SECOND);
		report.m_bUnrelatedStatePreserved
			= UnrelatedFixtureExact(exact)
			&& exact.FindFactionPool(RESISTANCE_FACTION) == resistanceBefore
			&& exact.FindPlayer("proof_player") == playerBefore;
		report.m_sRecoveryEvidence = recovered.BuildReport();

		HST_FactionPoolState occupierBefore
			= exact.FindFactionPool(OCCUPIER_FACTION);
		HST_EnemyPlanningState occupierPlanningBefore
			= exact.FindEnemyPlanningState(OCCUPIER_FACTION);
		HST_EnemyAuthorityBootstrapRecoveryResult second
			= m_Recovery.RecoverKnownSchema68BootstrapQuarantine(
				exact,
				balance,
				preset);
		report.m_bIdempotent = second && second.m_bAttempted
			&& !second.m_bSignatureMatched && !second.m_bRecovered
			&& exact.FindFactionPool(OCCUPIER_FACTION) == occupierBefore
			&& exact.FindEnemyPlanningState(OCCUPIER_FACTION)
				== occupierPlanningBefore
			&& m_Recovery.IsCanonicalBaselineAtSecond(
				exact,
				balance,
				preset,
				BASELINE_SECOND)
			&& UnrelatedFixtureExact(exact);

		HST_CampaignState nearMiss = BuildPoisonFixture();
		HST_FactionPoolState nearMissPool
			= nearMiss.FindFactionPool(OCCUPIER_FACTION);
		nearMissPool.m_iSupportResources = 1;
		HST_EnemyAuthorityBootstrapRecoveryResult nearMissResult
			= m_Recovery.RecoverKnownSchema68BootstrapQuarantine(
				nearMiss,
				balance,
				preset);
		HST_EnemyPlanningState nearMissPlanning
			= nearMiss.FindEnemyPlanningState(OCCUPIER_FACTION);
		bool resourceNearMissRejected = nearMissResult
			&& !nearMissResult.m_bSignatureMatched
			&& !nearMissResult.m_bRecovered
			&& nearMiss.FindFactionPool(OCCUPIER_FACTION) == nearMissPool
			&& nearMissPool.m_iSupportResources == 1
			&& nearMissPool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceSaveValidationService.QUARANTINE_CONTRACT_VERSION
			&& nearMissPlanning && nearMissPlanning.m_iContractVersion
				== HST_EnemyPlanningSaveValidationService.QUARANTINE_CONTRACT_VERSION;

		HST_CampaignState topologyNearMiss = BuildPoisonFixture();
		HST_FactionPoolState extraPool = new HST_FactionPoolState();
		extraPool.m_sFactionKey = "EXTRA";
		topologyNearMiss.m_aFactionPools.Insert(extraPool);
		HST_EnemyAuthorityBootstrapRecoveryResult topologyNearMissResult
			= m_Recovery.RecoverKnownSchema68BootstrapQuarantine(
				topologyNearMiss,
				balance,
				preset);
		bool topologyNearMissRejected = topologyNearMissResult
			&& !topologyNearMissResult.m_bSignatureMatched
			&& !topologyNearMissResult.m_bRecovered
			&& topologyNearMiss.m_aFactionPools.Count() == 4
			&& topologyNearMiss.FindFactionPool(OCCUPIER_FACTION)
				.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceSaveValidationService.QUARANTINE_CONTRACT_VERSION;

		HST_CampaignState legacyOrderNearMiss = BuildPoisonFixture();
		HST_EnemyOrderState legacyOrder = new HST_EnemyOrderState();
		legacyOrder.m_sOrderId = "proof_legacy_order";
		legacyOrder.m_iPlanningContractVersion = 0;
		legacyOrderNearMiss.m_aEnemyOrders.Insert(legacyOrder);
		HST_EnemyAuthorityBootstrapRecoveryResult legacyOrderNearMissResult
			= m_Recovery.RecoverKnownSchema68BootstrapQuarantine(
				legacyOrderNearMiss,
				balance,
				preset);
		bool legacyOrderNearMissRejected = legacyOrderNearMissResult
			&& !legacyOrderNearMissResult.m_bSignatureMatched
			&& !legacyOrderNearMissResult.m_bRecovered
			&& legacyOrderNearMiss.m_aEnemyOrders.Count() == 1
			&& legacyOrderNearMiss.m_aEnemyOrders[0] == legacyOrder;

		HST_CampaignState nullPoolNearMiss = BuildPoisonFixture();
		nullPoolNearMiss.m_aFactionPools[2] = null;
		HST_EnemyAuthorityBootstrapRecoveryResult nullPoolNearMissResult
			= m_Recovery.RecoverKnownSchema68BootstrapQuarantine(
				nullPoolNearMiss,
				balance,
				preset);
		bool nullPoolNearMissRejected = nullPoolNearMissResult
			&& !nullPoolNearMissResult.m_bSignatureMatched
			&& !nullPoolNearMissResult.m_bRecovered
			&& nullPoolNearMiss.m_aFactionPools.Count() == 3
			&& !nullPoolNearMiss.m_aFactionPools[2];

		HST_CampaignState presetNearMiss = BuildPoisonFixture();
		presetNearMiss.m_sPresetId = PRESET_ID + "_different";
		HST_EnemyAuthorityBootstrapRecoveryResult presetNearMissResult
			= m_Recovery.RecoverKnownSchema68BootstrapQuarantine(
				presetNearMiss,
				balance,
				preset);
		bool presetNearMissRejected = presetNearMissResult
			&& !presetNearMissResult.m_bSignatureMatched
			&& !presetNearMissResult.m_bRecovered
			&& presetNearMiss.m_sPresetId == PRESET_ID + "_different"
			&& presetNearMiss.FindFactionPool(OCCUPIER_FACTION)
				.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceSaveValidationService.QUARANTINE_CONTRACT_VERSION;
		report.m_bNearMissRejected = resourceNearMissRejected
			&& topologyNearMissRejected
			&& legacyOrderNearMissRejected
			&& nullPoolNearMissRejected
			&& presetNearMissRejected;

		HST_CampaignState versionedOrder = BuildPoisonFixture();
		HST_EnemyOrderState order = new HST_EnemyOrderState();
		order.m_sOrderId = "proof_versioned_order";
		order.m_iPlanningContractVersion
			= HST_EnemyPlanningSaveValidationService.CONTRACT_VERSION;
		versionedOrder.m_aEnemyOrders.Insert(order);
		HST_EnemyAuthorityBootstrapRecoveryResult versionedResult
			= m_Recovery.RecoverKnownSchema68BootstrapQuarantine(
				versionedOrder,
				balance,
				preset);
		HST_FactionPoolState versionedPool
			= versionedOrder.FindFactionPool(OCCUPIER_FACTION);
		report.m_bVersionedOrderRejected = versionedResult
			&& !versionedResult.m_bSignatureMatched
			&& !versionedResult.m_bRecovered
			&& order.m_iPlanningContractVersion
				== HST_EnemyPlanningSaveValidationService.CONTRACT_VERSION
			&& versionedPool && versionedPool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceSaveValidationService.QUARANTINE_CONTRACT_VERSION;
		report.m_sRejectionEvidence = string.Format(
			"resource/topology/legacy-order/null-pool/preset/versioned rejection %1/%2/%3/%4/%5/%6",
			resourceNearMissRejected,
			topologyNearMissRejected,
			legacyOrderNearMissRejected,
			nullPoolNearMissRejected,
			presetNearMissRejected,
			report.m_bVersionedOrderRejected);
		report.m_sRejectionEvidence = report.m_sRejectionEvidence
			+ string.Format(
			" | reasons %1 / %2 / %3 / %4 / %5 / %6",
			nearMissResult.m_sFailureReason,
			topologyNearMissResult.m_sFailureReason,
			legacyOrderNearMissResult.m_sFailureReason,
			nullPoolNearMissResult.m_sFailureReason,
			presetNearMissResult.m_sFailureReason,
			versionedResult.m_sFailureReason);

		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(exact);
		HST_CampaignState restored = save.Restore();
		HST_EnemyStrategicResourceSaveValidationService strategicValidator
			= new HST_EnemyStrategicResourceSaveValidationService();
		bool strategicAccepted = restored
			&& strategicValidator.ValidateRestoredFactionRoles(
				restored,
				preset,
				TARGET_SCHEMA_VERSION);
		HST_EnemyPlanningSaveValidationService planningValidator
			= new HST_EnemyPlanningSaveValidationService();
		bool planningAccepted = restored
			&& planningValidator.ValidateRestoredFactionRoles(
				restored,
				preset,
				TARGET_SCHEMA_VERSION);
		report.m_bValidatorsAccept = strategicAccepted && planningAccepted;
		report.m_bRoundtripExact = restored
			&& m_Recovery.IsCanonicalBaselineAtSecond(
				restored,
				balance,
				preset,
				BASELINE_SECOND)
			&& UnrelatedFixtureExact(restored);
		int restoredElapsed;
		int restoredPoolCount;
		int restoredPlanningCount;
		if (restored)
		{
			restoredElapsed = restored.m_iElapsedSeconds;
			restoredPoolCount = restored.m_aFactionPools.Count();
			restoredPlanningCount = restored.m_aEnemyPlanningStates.Count();
		}
		report.m_sRoundtripEvidence = string.Format(
			"roundtrip/strategic/planning %1/%2/%3 | elapsed %4 | pools %5 | planners %6",
			report.m_bRoundtripExact,
			strategicAccepted,
			planningAccepted,
			restoredElapsed,
			restoredPoolCount,
			restoredPlanningCount);
		return report;
	}

	protected HST_CampaignPreset BuildPreset()
	{
		HST_CampaignPreset preset = new HST_CampaignPreset();
		preset.m_sPresetId = PRESET_ID;
		preset.m_sResistanceFactionKey = RESISTANCE_FACTION;
		preset.m_sOccupierFactionKey = OCCUPIER_FACTION;
		preset.m_sInvaderFactionKey = INVADER_FACTION;
		return preset;
	}

	protected HST_BalanceConfig BuildBalance()
	{
		HST_BalanceConfig balance = new HST_BalanceConfig();
		balance.m_iStartingOccupierAttackPool = 73;
		balance.m_iStartingOccupierSupportPool = 82;
		balance.m_iStartingInvaderAttackPool = 37;
		balance.m_iStartingInvaderSupportPool = 46;
		return balance;
	}

	protected HST_CampaignState BuildPoisonFixture()
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = TARGET_SCHEMA_VERSION;
		state.m_iLastLoadedSchemaVersion = TARGET_SCHEMA_VERSION;
		state.m_bRestoredFromPersistence = true;
		state.m_sPresetId = PRESET_ID;
		state.m_iElapsedSeconds = BASELINE_SECOND;
		state.m_iCampaignSeed = 778811;
		state.m_iFactionMoney = 1234;
		state.m_iHR = 9;
		state.m_iWarLevel = 4;
		state.m_sCommanderIdentityId = "proof_commander";
		state.m_sLastPersistenceStatus = "proof unrelated state";
		state.m_aFactionPools.Clear();
		state.m_aFactionPools.Insert(BuildPoisonPool(OCCUPIER_FACTION));
		state.m_aFactionPools.Insert(BuildPoisonPool(INVADER_FACTION));
		HST_FactionPoolState resistance = new HST_FactionPoolState();
		resistance.m_sFactionKey = RESISTANCE_FACTION;
		resistance.m_iMoney = 17;
		resistance.m_iHR = 4;
		state.m_aFactionPools.Insert(resistance);
		state.m_aEnemyPlanningStates.Clear();
		state.m_aEnemyPlanningStates.Insert(
			BuildPoisonPlanning(OCCUPIER_FACTION));
		state.m_aEnemyPlanningStates.Insert(
			BuildPoisonPlanning(INVADER_FACTION));
		state.m_aEnemyStrategicMutations.Clear();
		state.m_aEnemyOrders.Clear();
		HST_PlayerState player = new HST_PlayerState();
		player.m_sIdentityId = "proof_player";
		player.m_sDisplayName = "Proof Player";
		player.m_iMoney = 55;
		state.m_aPlayers.Insert(player);
		return state;
	}

	protected HST_FactionPoolState BuildPoisonPool(string factionKey)
	{
		HST_FactionPoolState pool = new HST_FactionPoolState();
		pool.m_sFactionKey = factionKey;
		pool.m_iStrategicContractVersion
			= HST_EnemyStrategicResourceSaveValidationService.QUARANTINE_CONTRACT_VERSION;
		pool.m_sStrategicAuthorityFailure
			= HST_EnemyAuthorityBootstrapRecoveryService.STRATEGIC_BOOTSTRAP_FAILURE;
		return pool;
	}

	protected HST_EnemyPlanningState BuildPoisonPlanning(string factionKey)
	{
		HST_EnemyPlanningState planning = new HST_EnemyPlanningState();
		planning.m_iContractVersion
			= HST_EnemyPlanningSaveValidationService.QUARANTINE_CONTRACT_VERSION;
		planning.m_iRevision = 1;
		planning.m_sFactionKey = factionKey;
		planning.m_iLastPlanningBucketSecond = 0;
		planning.m_iNextPlanningBucketSecond = 0;
		planning.m_sDisposition = "quarantined";
		planning.m_sAuthorityFailure
			= HST_EnemyAuthorityBootstrapRecoveryService.PLANNING_BOOTSTRAP_FAILURE;
		return planning;
	}

	protected bool UnrelatedFixtureExact(HST_CampaignState state)
	{
		if (!state || state.m_sPresetId != PRESET_ID
			|| state.m_iCampaignSeed != 778811
			|| state.m_iFactionMoney != 1234 || state.m_iHR != 9
			|| state.m_iWarLevel != 4
			|| state.m_sCommanderIdentityId != "proof_commander"
			|| state.m_sLastPersistenceStatus != "proof unrelated state")
			return false;
		HST_FactionPoolState resistance
			= state.FindFactionPool(RESISTANCE_FACTION);
		HST_PlayerState player = state.FindPlayer("proof_player");
		return resistance && resistance.m_iStrategicContractVersion == 0
			&& resistance.m_iMoney == 17 && resistance.m_iHR == 4
			&& player && player.m_sDisplayName == "Proof Player"
			&& player.m_iMoney == 55;
	}
}
