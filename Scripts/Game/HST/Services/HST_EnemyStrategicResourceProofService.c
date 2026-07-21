#ifdef ENABLE_DIAG
class HST_EnemyStrategicResourceProofReport
{
	bool m_bLegacyAdoptionExact;
	bool m_bReplayConflictExact;
	bool m_bAtomicityExact;
	bool m_bIncomeCatchupExact;
	bool m_bPoolSeparationExact;
	bool m_bAggressionWarIndependenceExact;
	bool m_bFactionIsolationExact;
	bool m_bRoundtripQuarantineExact;
	string m_sAdoptionEvidence;
	string m_sReplayAtomicityEvidence;
	string m_sCadenceEvidence;
	string m_sSeparationIsolationEvidence;
	string m_sPersistenceEvidence;

	bool AllExact()
	{
		return m_bLegacyAdoptionExact
			&& m_bReplayConflictExact
			&& m_bAtomicityExact
			&& m_bIncomeCatchupExact
			&& m_bPoolSeparationExact
			&& m_bAggressionWarIndependenceExact
			&& m_bFactionIsolationExact
			&& m_bRoundtripQuarantineExact;
	}

	string BuildReport()
	{
		return string.Format(
			"enemy strategic resource proof | all exact %1 | adoption %2 | replay/conflict %3 | atomicity %4 | income/catchup %5 | separation %6 | aggression/war %7 | isolation %8 | persistence/quarantine %9",
			AllExact(),
			m_bLegacyAdoptionExact,
			m_bReplayConflictExact,
			m_bAtomicityExact,
			m_bIncomeCatchupExact,
			m_bPoolSeparationExact,
			m_bAggressionWarIndependenceExact,
			m_bFactionIsolationExact,
			m_bRoundtripQuarantineExact);
	}
}

// Source-only deterministic proof. It uses the production authority and save
// validator; native world entities and Workbench are intentionally unnecessary.
class HST_EnemyStrategicResourceProofService
{
	static const string RESISTANCE_FACTION = "FIA";
	static const string OCCUPIER_FACTION = "US";
	static const string INVADER_FACTION = "USSR";

	HST_EnemyStrategicResourceProofReport BuildReport()
	{
		HST_EnemyStrategicResourceProofReport report
			= new HST_EnemyStrategicResourceProofReport();
		ProveLegacyAdoption(report);
		ProveReplayConflictAndAtomicity(report);
		ProveIncomeCatchupAndFingerprint(report);
		ProveSeparationAndIsolation(report);
		ProveAggressionWarIndependence(report);
		ProveRoundtripAndQuarantine(report);
		return report;
	}

	protected void ProveLegacyAdoption(
		HST_EnemyStrategicResourceProofReport report)
	{
		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_CampaignState legacy = BuildLegacyState();
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(legacy);
		save.m_iSchemaVersion = 66;
		save.m_iLastLoadedSchemaVersion = 66;
		HST_EnemyStrategicResourceSaveValidationService validator
			= new HST_EnemyStrategicResourceSaveValidationService();
		validator.PrepareBeforeGenericNormalization(save, 66);
		validator.Normalize(save, 66);
		bool noInventedSaveReceipt = save.m_aEnemyStrategicMutations.IsEmpty();
		HST_CampaignState restored = save.Restore();
		bool adopted = restored
			&& validator.ValidateRestoredFactionRoles(restored, preset, 66);
		HST_FactionPoolState resistance;
		HST_FactionPoolState occupier;
		HST_FactionPoolState invader;
		if (restored)
		{
			resistance = restored.FindFactionPool(RESISTANCE_FACTION);
			occupier = restored.FindFactionPool(OCCUPIER_FACTION);
			invader = restored.FindFactionPool(INVADER_FACTION);
		}
		bool exact = adopted && noInventedSaveReceipt
			&& resistance && occupier && invader
			&& resistance.m_iStrategicContractVersion == 0
			&& occupier.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& invader.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& occupier.m_iStrategicRevision == 1
			&& invader.m_iStrategicRevision == 1
			&& occupier.m_iResourceAccumulatorSeconds == 120
			&& invader.m_iResourceAccumulatorSeconds == 120
			&& occupier.m_iAggressionAccumulatorSeconds == 45
			&& invader.m_iAggressionAccumulatorSeconds == 45
			&& restored.m_aEnemyStrategicMutations.IsEmpty();
		HST_CampaignState missing = BuildLegacyState();
		for (int poolIndex = missing.m_aFactionPools.Count() - 1; poolIndex >= 0; poolIndex--)
		{
			HST_FactionPoolState candidate = missing.m_aFactionPools[poolIndex];
			if (candidate && candidate.m_sFactionKey == OCCUPIER_FACTION)
				missing.m_aFactionPools.Remove(poolIndex);
		}
		HST_EnemyStrategicResourceSaveValidationService missingValidator
			= new HST_EnemyStrategicResourceSaveValidationService();
		bool missingRejected = !missingValidator.ValidateRestoredFactionRoles(
			missing,
			preset,
			66);
		HST_FactionPoolState missingPlaceholder
			= missing.FindFactionPool(OCCUPIER_FACTION);
		bool placeholderExact = missingRejected && missingPlaceholder
			&& missingPlaceholder.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION
			&& missingPlaceholder.m_iAttackResources == 0
			&& missingPlaceholder.m_iSupportResources == 0;
		exact = exact && placeholderExact;
		report.m_bLegacyAdoptionExact = exact;
		report.m_sAdoptionEvidence = string.Format(
			"adopted/no-invented/placeholder %1/%2/%3 | contracts FIA/US/USSR %4/%5/%6 | cadence %7/%8",
			adopted,
			noInventedSaveReceipt,
			placeholderExact,
			resistance && resistance.m_iStrategicContractVersion,
			occupier && occupier.m_iStrategicContractVersion,
			invader && invader.m_iStrategicContractVersion,
			occupier && occupier.m_iResourceAccumulatorSeconds,
			occupier && occupier.m_iAggressionAccumulatorSeconds);
	}

	protected void ProveReplayConflictAndAtomicity(
		HST_EnemyStrategicResourceProofReport report)
	{
		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_EnemyStrategicResourceService authority
			= new HST_EnemyStrategicResourceService();
		HST_CampaignState state = BuildExactState("replay", 1, false);
		HST_FactionPoolState pool = state.FindFactionPool(OCCUPIER_FACTION);
		HST_EnemyStrategicMutationResult applied = authority.DebitResources(
			state,
			preset,
			"proof_enemy_strategic_replay",
			OCCUPIER_FACTION,
			10,
			4,
			"proof_order_debit",
			"proof_replay_source",
			"proof_order",
			"proof_operation",
			"proof_manifest",
			"proof_us_town");
		int attackAfter = pool.m_iAttackResources;
		int supportAfter = pool.m_iSupportResources;
		int revisionAfter = pool.m_iStrategicRevision;
		int receiptsAfter = state.m_aEnemyStrategicMutations.Count();
		HST_EnemyStrategicMutationResult replay = authority.DebitResources(
			state,
			preset,
			"proof_enemy_strategic_replay",
			OCCUPIER_FACTION,
			10,
			4,
			"proof_order_debit",
			"proof_replay_source",
			"proof_order",
			"proof_operation",
			"proof_manifest",
			"proof_us_town");
		bool replayExact = applied && applied.m_bAccepted && applied.m_bChanged
			&& replay && replay.m_bAccepted && replay.m_bAlreadyApplied
			&& !replay.m_bChanged
			&& pool.m_iAttackResources == attackAfter
			&& pool.m_iSupportResources == supportAfter
			&& pool.m_iStrategicRevision == revisionAfter
			&& state.m_aEnemyStrategicMutations.Count() == receiptsAfter;

		HST_EnemyStrategicMutationResult conflict = authority.DebitResources(
			state,
			preset,
			"proof_enemy_strategic_replay",
			OCCUPIER_FACTION,
			11,
			4,
			"proof_order_debit",
			"proof_replay_source",
			"proof_order",
			"proof_operation",
			"proof_manifest",
			"proof_us_town");
		bool conflictExact = conflict && !conflict.m_bAccepted
			&& conflict.m_sFailureReason.Contains("different fingerprint")
			&& pool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION
			&& pool.m_iAttackResources == attackAfter
			&& pool.m_iSupportResources == supportAfter
			&& pool.m_iStrategicRevision == revisionAfter
			&& state.m_aEnemyStrategicMutations.Count() == receiptsAfter;
		HST_CampaignState zeroState = BuildExactState("zero", 1, false);
		HST_FactionPoolState zeroPool = zeroState.FindFactionPool(OCCUPIER_FACTION);
		HST_EnemyStrategicMutationResult zeroApplied = authority.ApplyResourceDelta(
			zeroState,
			preset,
			"proof_zero_delta_exact",
			OCCUPIER_FACTION,
			0,
			0,
			"proof_zero_delta",
			"proof_zero_delta_source");
		int zeroRevision = zeroPool.m_iStrategicRevision;
		HST_EnemyStrategicMutationResult zeroReplay = authority.ApplyResourceDelta(
			zeroState,
			preset,
			"proof_zero_delta_exact",
			OCCUPIER_FACTION,
			0,
			0,
			"proof_zero_delta",
			"proof_zero_delta_source");
		HST_EnemyStrategicMutationResult zeroConflict = authority.ApplyResourceDelta(
			zeroState,
			preset,
			"proof_zero_delta_exact",
			OCCUPIER_FACTION,
			1,
			0,
			"proof_zero_delta",
			"proof_zero_delta_source");
		bool zeroExact = zeroApplied && zeroApplied.m_bAccepted
			&& zeroApplied.m_bChanged && zeroApplied.m_Mutation
			&& zeroApplied.m_Mutation.m_iOperationalSequence == 1
			&& zeroReplay && zeroReplay.m_bAccepted
			&& zeroReplay.m_bAlreadyApplied
			&& zeroPool.m_iStrategicRevision == zeroRevision
			&& zeroConflict && !zeroConflict.m_bAccepted
			&& zeroPool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION;
		report.m_bReplayConflictExact = replayExact && conflictExact && zeroExact;

		HST_CampaignState atomic = BuildExactState("atomic", 1, false);
		HST_FactionPoolState atomicPool = atomic.FindFactionPool(OCCUPIER_FACTION);
		atomicPool.m_iAttackResources = 5;
		atomicPool.m_iSupportResources = 3;
		int atomicRevision = atomicPool.m_iStrategicRevision;
		HST_EnemyStrategicMutationResult underflow = authority.DebitResources(
			atomic,
			preset,
			"proof_enemy_strategic_underflow",
			OCCUPIER_FACTION,
			6,
			1,
			"proof_atomic_debit",
			"proof_atomic_source");
		bool underflowExact = underflow && !underflow.m_bAccepted
			&& atomicPool.m_iAttackResources == 5
			&& atomicPool.m_iSupportResources == 3
			&& atomicPool.m_iStrategicRevision == atomicRevision
			&& atomicPool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& atomic.m_aEnemyStrategicMutations.IsEmpty();
		atomicPool.m_iAttackResources = int.MAX;
		HST_EnemyStrategicMutationResult overflow = authority.RefundResources(
			atomic,
			preset,
			"proof_enemy_strategic_overflow",
			OCCUPIER_FACTION,
			1,
			0,
			"proof_atomic_refund",
			"proof_atomic_source");
		bool overflowExact = overflow && !overflow.m_bAccepted
			&& atomicPool.m_iAttackResources == int.MAX
			&& atomicPool.m_iSupportResources == 3
			&& atomicPool.m_iStrategicRevision == atomicRevision
			&& atomic.m_aEnemyStrategicMutations.IsEmpty();
		report.m_bAtomicityExact = underflowExact && overflowExact;
		report.m_sReplayAtomicityEvidence = string.Format(
			"replay/conflict/zero %1/%2/%3 | underflow/overflow %4/%5 | revision %6",
			replayExact,
			conflictExact,
			zeroExact,
			underflowExact,
			overflowExact,
			revisionAfter);
	}

	protected void ProveIncomeCatchupAndFingerprint(
		HST_EnemyStrategicResourceProofReport report)
	{
		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_BalanceConfig balance = HST_DefaultCatalog.CreateBalance();
		HST_EnemyStrategicResourceService firstAuthority
			= new HST_EnemyStrategicResourceService();
		HST_EnemyStrategicResourceService secondAuthority
			= new HST_EnemyStrategicResourceService();
		HST_CampaignState first = BuildExactState("income", 2, false);
		HST_CampaignState second = BuildExactState("income", 2, true);
		HST_FactionPoolState firstPool = first.FindFactionPool(OCCUPIER_FACTION);
		HST_FactionPoolState secondPool = second.FindFactionPool(OCCUPIER_FACTION);
		int attackBefore = firstPool.m_iAttackResources;
		int supportBefore = firstPool.m_iSupportResources;
		first.m_iElapsedSeconds = 900;
		second.m_iElapsedSeconds = 900;
		bool firstProcessed = firstAuthority.TickIncome(
			first,
			preset,
			balance,
			900);
		bool secondProcessed = secondAuthority.TickIncome(
			second,
			preset,
			balance,
			900);
		HST_EnemyStrategicMutationState firstIncome = FindPeriodicMutation(
			first,
			OCCUPIER_FACTION,
			HST_EnemyStrategicResourceService.KIND_PERIODIC_INCOME);
		HST_EnemyStrategicMutationState secondIncome = FindPeriodicMutation(
			second,
			OCCUPIER_FACTION,
			HST_EnemyStrategicResourceService.KIND_PERIODIC_INCOME);
		string shapeFailure;
		bool deterministic = firstProcessed && secondProcessed
			&& firstPool.m_iAttackResources == secondPool.m_iAttackResources
			&& firstPool.m_iSupportResources == secondPool.m_iSupportResources
			&& firstPool.m_iStrategicRevision == secondPool.m_iStrategicRevision
			&& firstIncome && secondIncome
			&& firstIncome.m_sMutationId == secondIncome.m_sMutationId
			&& firstIncome.m_sContributionHash == secondIncome.m_sContributionHash
			&& firstIncome.m_sFingerprint == secondIncome.m_sFingerprint
			&& HST_EnemyStrategicResourceSaveValidationService.ValidateMutationShape(
				firstIncome,
				shapeFailure);
		bool compactExact = CountPeriodicMutations(
			first,
			HST_EnemyStrategicResourceService.KIND_PERIODIC_INCOME) == 2
			&& firstIncome.m_iCreatedAtSecond == 900
			&& firstPool.m_iStrategicRevision == 4
			&& firstPool.m_iResourceAccumulatorSeconds == 0
			&& firstPool.m_iAttackResources > attackBefore
			&& firstPool.m_iSupportResources > supportBefore;

		HST_CampaignState catchup = BuildExactState("catchup", 2, false);
		HST_FactionPoolState catchupPool = catchup.FindFactionPool(OCCUPIER_FACTION);
		catchup.m_iElapsedSeconds = 9000;
		HST_EnemyStrategicResourceService catchupAuthority
			= new HST_EnemyStrategicResourceService();
		bool catchupProcessed = catchupAuthority.TickIncome(
			catchup,
			preset,
			balance,
			9000);
		bool boundedCatchup = catchupProcessed
			&& catchupPool.m_iStrategicRevision
				== 1 + HST_EnemyStrategicResourceService.MAX_CATCHUP_STEPS_PER_TICK
			&& catchupPool.m_iResourceAccumulatorSeconds == 1800
			&& CountPeriodicMutations(
				catchup,
				HST_EnemyStrategicResourceService.KIND_PERIODIC_INCOME) == 2;
		report.m_bIncomeCatchupExact
			= deterministic && compactExact && boundedCatchup;
		report.m_sCadenceEvidence = string.Format(
			"deterministic/compact/bounded %1/%2/%3 | revision %4 | backlog %5 | hash %6",
			deterministic,
			compactExact,
			boundedCatchup,
			firstPool.m_iStrategicRevision,
			catchupPool.m_iResourceAccumulatorSeconds,
			firstIncome && firstIncome.m_sContributionHash);
	}

	protected void ProveSeparationAndIsolation(
		HST_EnemyStrategicResourceProofReport report)
	{
		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_CampaignState state = BuildExactState("separation", 1, false);
		HST_EnemyStrategicResourceService authority
			= new HST_EnemyStrategicResourceService();
		HST_FactionPoolState us = state.FindFactionPool(OCCUPIER_FACTION);
		HST_FactionPoolState ussr = state.FindFactionPool(INVADER_FACTION);
		HST_FactionPoolState fia = state.FindFactionPool(RESISTANCE_FACTION);
		int attackBefore = us.m_iAttackResources;
		int supportBefore = us.m_iSupportResources;
		int aggressionBefore = us.m_iAggression;
		int rivalAttackBefore = ussr.m_iAttackResources;
		HST_EnemyStrategicMutationResult attack = authority.DebitResources(
			state,
			preset,
			"proof_attack_only",
			OCCUPIER_FACTION,
			7,
			0,
			"proof_attack_debit",
			"proof_attack_source");
		bool attackOnly = attack && attack.m_bAccepted
			&& us.m_iAttackResources == attackBefore - 7
			&& us.m_iSupportResources == supportBefore
			&& us.m_iAggression == aggressionBefore;
		HST_EnemyStrategicMutationResult support = authority.DebitResources(
			state,
			preset,
			"proof_support_only",
			OCCUPIER_FACTION,
			0,
			5,
			"proof_support_debit",
			"proof_support_source");
		bool supportOnly = support && support.m_bAccepted
			&& us.m_iAttackResources == attackBefore - 7
			&& us.m_iSupportResources == supportBefore - 5
			&& us.m_iAggression == aggressionBefore;
		HST_EnemyStrategicMutationResult aggression
			= authority.ApplyAggressionDelta(
				state,
				preset,
				"proof_aggression_only",
				OCCUPIER_FACTION,
				3,
				"proof_aggression_gain",
				"proof_aggression_source");
		bool aggressionOnly = aggression && aggression.m_bAccepted
			&& us.m_iAttackResources == attackBefore - 7
			&& us.m_iSupportResources == supportBefore - 5
			&& us.m_iAggression == aggressionBefore + 3;
		report.m_bPoolSeparationExact
			= attackOnly && supportOnly && aggressionOnly;

		HST_EnemyStrategicMutationResult resistanceRejected
			= authority.ApplyAggressionDelta(
				state,
				preset,
				"proof_resistance_rejected",
				RESISTANCE_FACTION,
				3,
				"proof_invalid_target",
				"proof_resistance_source");
		HST_EnemyStrategicMutationResult rivalApplied
			= authority.ApplyAggressionDelta(
				state,
				preset,
				"proof_rival_independent",
				INVADER_FACTION,
				2,
				"proof_rival_gain",
				"proof_rival_source");
		bool isolation = resistanceRejected && !resistanceRejected.m_bAccepted
			&& rivalApplied && rivalApplied.m_bAccepted
			&& fia.m_iAttackResources == 0
			&& fia.m_iSupportResources == 0
			&& fia.m_iAggression == 0
			&& fia.m_iStrategicContractVersion == 0
			&& ussr.m_iAttackResources == rivalAttackBefore
			&& ussr.m_iAggression == 14
			&& us.m_iAggression == aggressionBefore + 3;
		HST_CampaignState capState = BuildExactState("cap_isolation", 1, false);
		HST_FactionPoolState cappedOccupier = capState.FindFactionPool(OCCUPIER_FACTION);
		HST_FactionPoolState uncappedInvader = capState.FindFactionPool(INVADER_FACTION);
		cappedOccupier.m_iStrategicOperationalMutationCount
			= HST_EnemyStrategicResourceService.MAX_OPERATIONAL_MUTATIONS;
		HST_EnemyStrategicMutationResult cappedRejected
			= authority.ApplyResourceDelta(
				capState,
				preset,
				"proof_cap_occupier",
				OCCUPIER_FACTION,
				0,
				0,
				"proof_cap",
				"proof_cap_occupier_source");
		HST_EnemyStrategicMutationResult rivalAtCapApplied
			= authority.ApplyResourceDelta(
				capState,
				preset,
				"proof_cap_invader",
				INVADER_FACTION,
				0,
				0,
				"proof_cap",
				"proof_cap_invader_source");
		bool capIsolation = cappedRejected && !cappedRejected.m_bAccepted
			&& rivalAtCapApplied && rivalAtCapApplied.m_bAccepted
			&& cappedOccupier.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& uncappedInvader.m_iStrategicOperationalMutationCount == 1;
		report.m_bFactionIsolationExact = isolation && capIsolation;
		report.m_sSeparationIsolationEvidence = string.Format(
			"attack/support/aggression isolated %1/%2/%3 | resistance rejected %4 | rival independent/cap %5/%6",
			attackOnly,
			supportOnly,
			aggressionOnly,
			resistanceRejected && !resistanceRejected.m_bAccepted,
			isolation,
			capIsolation);
	}

	protected void ProveAggressionWarIndependence(
		HST_EnemyStrategicResourceProofReport report)
	{
		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_BalanceConfig balance = HST_DefaultCatalog.CreateBalance();
		HST_CampaignState low = BuildExactState("war", 1, false);
		HST_CampaignState high = BuildExactState("war", 5, false);
		low.m_iElapsedSeconds = 300;
		high.m_iElapsedSeconds = 300;
		HST_EnemyStrategicResourceService lowAuthority
			= new HST_EnemyStrategicResourceService();
		HST_EnemyStrategicResourceService highAuthority
			= new HST_EnemyStrategicResourceService();
		bool lowDecay = lowAuthority.TickAggressionDecay(
			low,
			preset,
			balance,
			300);
		bool highDecay = highAuthority.TickAggressionDecay(
			high,
			preset,
			balance,
			300);
		HST_FactionPoolState lowPool = low.FindFactionPool(OCCUPIER_FACTION);
		HST_FactionPoolState highPool = high.FindFactionPool(OCCUPIER_FACTION);
		HST_EnemyStrategicMutationState lowDecayReceipt = FindPeriodicMutation(
			low,
			OCCUPIER_FACTION,
			HST_EnemyStrategicResourceService.KIND_AGGRESSION_DECAY);
		HST_EnemyStrategicMutationState highDecayReceipt = FindPeriodicMutation(
			high,
			OCCUPIER_FACTION,
			HST_EnemyStrategicResourceService.KIND_AGGRESSION_DECAY);
		bool decayIndependent = lowDecay && highDecay
			&& lowPool.m_iAggression == highPool.m_iAggression
			&& lowPool.m_iAggression == 11
			&& lowDecayReceipt && highDecayReceipt
			&& lowDecayReceipt.m_sMutationId
				== highDecayReceipt.m_sMutationId
			&& lowDecayReceipt.m_sContributionHash
				== highDecayReceipt.m_sContributionHash
			&& lowDecayReceipt.m_sFingerprint
				== highDecayReceipt.m_sFingerprint;
		int lowAttackBefore = lowPool.m_iAttackResources;
		int highAttackBefore = highPool.m_iAttackResources;
		low.m_iElapsedSeconds = 600;
		high.m_iElapsedSeconds = 600;
		bool lowIncome = lowAuthority.TickIncome(low, preset, balance, 300);
		bool highIncome = highAuthority.TickIncome(high, preset, balance, 300);
		int lowGain = lowPool.m_iAttackResources - lowAttackBefore;
		int highGain = highPool.m_iAttackResources - highAttackBefore;
		bool warAffectsIncome = lowIncome && highIncome && highGain > lowGain;
		report.m_bAggressionWarIndependenceExact
			= decayIndependent && warAffectsIncome;
		report.m_sCadenceEvidence = report.m_sCadenceEvidence
			+ string.Format(
				" | aggression independent %1 | war income gain %2/%3",
				decayIndependent,
				lowGain,
				highGain);
	}

	protected void ProveRoundtripAndQuarantine(
		HST_EnemyStrategicResourceProofReport report)
	{
		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_CampaignState state = BuildExactState("roundtrip", 2, false);
		HST_EnemyStrategicResourceService authority
			= new HST_EnemyStrategicResourceService();
		HST_EnemyStrategicMutationResult mutation
			= authority.ApplyResourceDelta(
				state,
				preset,
				"proof_roundtrip_delta",
				OCCUPIER_FACTION,
				-3,
				2,
				"proof_roundtrip",
				"proof_roundtrip_source");
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(state);
		HST_EnemyStrategicResourceSaveValidationService validator
			= new HST_EnemyStrategicResourceSaveValidationService();
		validator.PrepareBeforeGenericNormalization(save, 67);
		validator.Normalize(save, 67);
		HST_CampaignState restored = save.Restore();
		bool rolesValid = restored
			&& validator.ValidateRestoredFactionRoles(restored, preset, 67);
		HST_FactionPoolState originalPool = state.FindFactionPool(OCCUPIER_FACTION);
		HST_FactionPoolState restoredPool;
		HST_EnemyStrategicMutationState restoredMutation;
		if (restored)
		{
			restoredPool = restored.FindFactionPool(OCCUPIER_FACTION);
			restoredMutation = FindMutation(restored, "proof_roundtrip_delta");
		}
		string shapeFailure;
		bool roundtrip = mutation && mutation.m_bAccepted && rolesValid
			&& restoredPool && restoredMutation
			&& restoredPool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& restoredPool.m_iStrategicRevision
				== originalPool.m_iStrategicRevision
			&& restoredPool.m_iAttackResources
				== originalPool.m_iAttackResources
			&& restoredPool.m_iSupportResources
				== originalPool.m_iSupportResources
			&& restoredPool.m_sLastStrategicMutationId
				== originalPool.m_sLastStrategicMutationId
			&& HST_EnemyStrategicResourceSaveValidationService.ValidateMutationShape(
				restoredMutation,
				shapeFailure);

		HST_CampaignSaveData tampered = new HST_CampaignSaveData();
		tampered.Capture(state);
		HST_EnemyStrategicMutationState tamperedMutation
			= FindSavedMutation(tampered, "proof_roundtrip_delta");
		if (tamperedMutation)
			tamperedMutation.m_iAttackAfter++;
		HST_EnemyStrategicResourceSaveValidationService tamperValidator
			= new HST_EnemyStrategicResourceSaveValidationService();
		tamperValidator.PrepareBeforeGenericNormalization(tampered, 67);
		tamperValidator.Normalize(tampered, 67);
		HST_FactionPoolState tamperedPool
			= FindSavedPool(tampered, OCCUPIER_FACTION);
		bool tamperQuarantined = tamperedMutation && tamperedPool
			&& tamperedMutation.m_iContractVersion
				== HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION
			&& !tamperedMutation.m_bApplied
			&& !FindSavedMutation(tampered, "proof_roundtrip_delta")
			&& tampered.m_aEnemyStrategicMutations.IsEmpty()
			&& tamperedPool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION;

		HST_CampaignSaveData duplicated = new HST_CampaignSaveData();
		duplicated.Capture(state);
		HST_FactionPoolState duplicatePool = new HST_FactionPoolState();
		duplicatePool.m_sFactionKey = OCCUPIER_FACTION;
		duplicatePool.m_iStrategicContractVersion
			= HST_EnemyStrategicResourceService.CONTRACT_VERSION;
		duplicatePool.m_iStrategicRevision = 1;
		duplicated.m_aFactionPools.Insert(duplicatePool);
		HST_EnemyStrategicResourceSaveValidationService duplicateValidator
			= new HST_EnemyStrategicResourceSaveValidationService();
		duplicateValidator.PrepareBeforeGenericNormalization(duplicated, 67);
		duplicateValidator.Normalize(duplicated, 67);
		bool duplicateQuarantined = CountQuarantinedSavedPools(
			duplicated,
			OCCUPIER_FACTION) == 2;
		bool middleDeletionQuarantined = ProveMiddleOperationalDeletionQuarantine();
		bool compactedRoundtripExact = ProveCompactedCadenceRoundtripAndTamper();
		bool orderBacklinkExact = ProveOrderBacklinkQuarantine();
		bool aggressionBacklinksExact = ProveAggressionBacklinkQuarantine();
		bool orphanCapacityExact = ProveOrphanReceiptCapacityIsolation();
		report.m_bRoundtripQuarantineExact
			= roundtrip && tamperQuarantined && duplicateQuarantined
				&& middleDeletionQuarantined && compactedRoundtripExact
				&& orderBacklinkExact && aggressionBacklinksExact
				&& orphanCapacityExact;
		report.m_sPersistenceEvidence = string.Format(
			"roundtrip/tamper/duplicate/middle/compact/order/aggression-links %1/%2/%3/%4/%5/%6/%7 | revision %8 | receipt %9",
			roundtrip,
			tamperQuarantined,
			duplicateQuarantined,
			middleDeletionQuarantined,
			compactedRoundtripExact,
			orderBacklinkExact,
			aggressionBacklinksExact,
			restoredPool && restoredPool.m_iStrategicRevision,
			restoredMutation && restoredMutation.m_sMutationId);
		report.m_sPersistenceEvidence = report.m_sPersistenceEvidence
			+ string.Format(" | orphan capacity/purge %1", orphanCapacityExact);
	}

	protected bool ProveMiddleOperationalDeletionQuarantine()
	{
		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_CampaignState state = BuildExactState("middle_delete", 1, false);
		HST_EnemyStrategicResourceService authority
			= new HST_EnemyStrategicResourceService();
		for (int index = 1; index <= 3; index++)
		{
			HST_EnemyStrategicMutationResult result = authority.ApplyResourceDelta(
				state,
				preset,
				string.Format("proof_middle_%1", index),
				OCCUPIER_FACTION,
				1,
				0,
				"proof_middle",
				string.Format("proof_middle_source_%1", index));
			if (!result || !result.m_bAccepted)
				return false;
		}
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(state);
		for (int mutationIndex = save.m_aEnemyStrategicMutations.Count() - 1; mutationIndex >= 0; mutationIndex--)
		{
			HST_EnemyStrategicMutationState mutation
				= save.m_aEnemyStrategicMutations[mutationIndex];
			if (mutation && mutation.m_sFactionKey == OCCUPIER_FACTION
				&& mutation.m_iOperationalSequence == 2)
			{
				save.m_aEnemyStrategicMutations.Remove(mutationIndex);
				break;
			}
		}
		HST_EnemyStrategicResourceSaveValidationService validator
			= new HST_EnemyStrategicResourceSaveValidationService();
		validator.PrepareBeforeGenericNormalization(save, 67);
		validator.Normalize(save, 67);
		HST_FactionPoolState pool = FindSavedPool(save, OCCUPIER_FACTION);
		return pool && pool.m_iStrategicContractVersion
			== HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION;
	}

	protected bool ProveCompactedCadenceRoundtripAndTamper()
	{
		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_BalanceConfig balance = HST_DefaultCatalog.CreateBalance();
		HST_CampaignState state = BuildExactState("compact_roundtrip", 2, false);
		HST_EnemyStrategicResourceService authority
			= new HST_EnemyStrategicResourceService();
		state.m_iElapsedSeconds = 300;
		if (!authority.TickIncome(state, preset, balance, 300))
			return false;
		HST_EnemyStrategicMutationResult operational = authority.ApplyResourceDelta(
			state,
			preset,
			"proof_compact_operational",
			OCCUPIER_FACTION,
			1,
			0,
			"proof_compact",
			"proof_compact_source");
		state.m_iElapsedSeconds = 600;
		if (!operational || !operational.m_bAccepted
			|| !authority.TickIncome(state, preset, balance, 300))
			return false;

		HST_CampaignSaveData exact = new HST_CampaignSaveData();
		exact.Capture(state);
		HST_EnemyStrategicResourceSaveValidationService exactValidator
			= new HST_EnemyStrategicResourceSaveValidationService();
		exactValidator.PrepareBeforeGenericNormalization(exact, 67);
		exactValidator.Normalize(exact, 67);
		HST_FactionPoolState exactPool = FindSavedPool(exact, OCCUPIER_FACTION);
		bool roundtripExact = exactPool
			&& exactPool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& exactPool.m_iLastResourceBucketSecond == 600
			&& exactPool.m_iStrategicOperationalMutationCount == 1;

		HST_CampaignSaveData tampered = new HST_CampaignSaveData();
		tampered.Capture(state);
		HST_FactionPoolState tamperedPool = FindSavedPool(
			tampered,
			OCCUPIER_FACTION);
		if (tamperedPool)
			tamperedPool.m_iResourceAccumulatorSeconds = 1;
		HST_EnemyStrategicResourceSaveValidationService tamperValidator
			= new HST_EnemyStrategicResourceSaveValidationService();
		tamperValidator.PrepareBeforeGenericNormalization(tampered, 67);
		tamperValidator.Normalize(tampered, 67);
		return roundtripExact && tamperedPool
			&& tamperedPool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION;
	}

	protected bool ProveOrderBacklinkQuarantine()
	{
		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_CampaignState state = BuildExactState("order_link", 1, false);
		HST_EnemyOrderState order = new HST_EnemyOrderState();
		order.m_sOrderId = "proof_linked_order";
		order.m_sOperationId = "proof_linked_operation";
		order.m_sManifestId = "proof_linked_manifest";
		order.m_sFactionKey = OCCUPIER_FACTION;
		order.m_sTargetZoneId = "proof_us_town";
		order.m_iAttackCost = 7;
		order.m_sResourceDebitMutationId = "proof_linked_order_debit";
		state.m_aEnemyOrders.Insert(order);
		HST_EnemyStrategicResourceService authority
			= new HST_EnemyStrategicResourceService();
		HST_EnemyStrategicMutationResult debit = authority.DebitResources(
			state,
			preset,
			order.m_sResourceDebitMutationId,
			order.m_sFactionKey,
			order.m_iAttackCost,
			order.m_iSupportCost,
			"proactive_attack_debit",
			order.m_sOrderId,
			order.m_sOrderId,
			order.m_sOperationId,
			order.m_sManifestId,
			order.m_sTargetZoneId);
		if (!debit || !debit.m_bAccepted)
			return false;

		HST_CampaignSaveData exact = new HST_CampaignSaveData();
		exact.Capture(state);
		HST_EnemyStrategicResourceSaveValidationService exactValidator
			= new HST_EnemyStrategicResourceSaveValidationService();
		exactValidator.PrepareBeforeGenericNormalization(exact, 67);
		exactValidator.Normalize(exact, 67);
		HST_FactionPoolState exactPool = FindSavedPool(exact, OCCUPIER_FACTION);
		bool validAccepted = exactPool && exactPool.m_iStrategicContractVersion
			== HST_EnemyStrategicResourceService.CONTRACT_VERSION;

		HST_CampaignSaveData broken = new HST_CampaignSaveData();
		broken.Capture(state);
		foreach (HST_EnemyOrderState savedOrder : broken.m_aEnemyOrders)
		{
			if (savedOrder && savedOrder.m_sOrderId == order.m_sOrderId)
				savedOrder.m_sResourceDebitMutationId = "";
		}
		HST_EnemyStrategicResourceSaveValidationService brokenValidator
			= new HST_EnemyStrategicResourceSaveValidationService();
		brokenValidator.PrepareBeforeGenericNormalization(broken, 67);
		brokenValidator.Normalize(broken, 67);
		HST_FactionPoolState brokenPool = FindSavedPool(broken, OCCUPIER_FACTION);
		return validAccepted && brokenPool
			&& brokenPool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION;
	}

	protected bool ProveAggressionBacklinkQuarantine()
	{
		return ProveTownInfluenceBacklinkQuarantine()
			&& ProveOwnershipBacklinkQuarantine();
	}

	protected bool ProveTownInfluenceBacklinkQuarantine()
	{
		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_CampaignState state = BuildExactState("town_link", 1, false);
		HST_FactionPoolState pool = state.FindFactionPool(OCCUPIER_FACTION);
		HST_TownInfluenceEventState eventState = new HST_TownInfluenceEventState();
		eventState.m_sEventId = "proof_town_link_event";
		eventState.m_sZoneId = "proof_us_town";
		eventState.m_sAggressionFactionKey = OCCUPIER_FACTION;
		eventState.m_iAggressionDelta = 2;
		eventState.m_iAggressionBefore = pool.m_iAggression;
		eventState.m_iAggressionAfter = pool.m_iAggression + 2;
		eventState.m_bApplied = true;
		state.m_aTownInfluenceEvents.Insert(eventState);
		HST_EnemyStrategicResourceService authority
			= new HST_EnemyStrategicResourceService();
		HST_EnemyStrategicMutationResult result = authority.ApplyAggressionDelta(
			state,
			preset,
			"proof_town_link_mutation",
			OCCUPIER_FACTION,
			2,
			"town_influence",
			eventState.m_sEventId,
			"",
			"",
			"",
			eventState.m_sZoneId);
		if (!result || !result.m_bAccepted)
			return false;
		HST_CampaignSaveData exact = new HST_CampaignSaveData();
		exact.Capture(state);
		HST_EnemyStrategicResourceSaveValidationService exactValidator
			= new HST_EnemyStrategicResourceSaveValidationService();
		exactValidator.PrepareBeforeGenericNormalization(exact, 67);
		exactValidator.Normalize(exact, 67);
		HST_FactionPoolState exactPool = FindSavedPool(exact, OCCUPIER_FACTION);
		bool validAccepted = exactPool && exactPool.m_iStrategicContractVersion
			== HST_EnemyStrategicResourceService.CONTRACT_VERSION;
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(state);
		foreach (HST_TownInfluenceEventState savedEvent : save.m_aTownInfluenceEvents)
		{
			if (savedEvent && savedEvent.m_sEventId == eventState.m_sEventId)
				savedEvent.m_iAggressionAfter++;
		}
		HST_EnemyStrategicResourceSaveValidationService validator
			= new HST_EnemyStrategicResourceSaveValidationService();
		validator.PrepareBeforeGenericNormalization(save, 67);
		validator.Normalize(save, 67);
		HST_FactionPoolState savedPool = FindSavedPool(save, OCCUPIER_FACTION);
		return validAccepted && savedPool && savedPool.m_iStrategicContractVersion
			== HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION;
	}

	protected bool ProveOwnershipBacklinkQuarantine()
	{
		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_CampaignState state = BuildExactState("ownership_link", 1, false);
		HST_OwnershipTransitionState transition
			= new HST_OwnershipTransitionState();
		transition.m_sRequestId = "proof_ownership_link_request";
		transition.m_sZoneId = "proof_us_town";
		transition.m_sPreviousOwnerFactionKey = OCCUPIER_FACTION;
		transition.m_iAggressionApplied = 2;
		transition.m_bEnemyConsequencesApplied = true;
		state.m_aOwnershipTransitions.Insert(transition);
		HST_EnemyStrategicResourceService authority
			= new HST_EnemyStrategicResourceService();
		HST_EnemyStrategicMutationResult result = authority.ApplyAggressionDelta(
			state,
			preset,
			"proof_ownership_link_mutation",
			OCCUPIER_FACTION,
			2,
			"ownership_transition",
			transition.m_sRequestId,
			"",
			"",
			"",
			transition.m_sZoneId);
		if (!result || !result.m_bAccepted)
			return false;
		HST_CampaignSaveData exact = new HST_CampaignSaveData();
		exact.Capture(state);
		HST_EnemyStrategicResourceSaveValidationService exactValidator
			= new HST_EnemyStrategicResourceSaveValidationService();
		exactValidator.PrepareBeforeGenericNormalization(exact, 67);
		exactValidator.Normalize(exact, 67);
		HST_FactionPoolState exactPool = FindSavedPool(exact, OCCUPIER_FACTION);
		bool validAccepted = exactPool && exactPool.m_iStrategicContractVersion
			== HST_EnemyStrategicResourceService.CONTRACT_VERSION;
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(state);
		foreach (HST_OwnershipTransitionState savedTransition : save.m_aOwnershipTransitions)
		{
			if (savedTransition
				&& savedTransition.m_sRequestId == transition.m_sRequestId)
				savedTransition.m_sPreviousOwnerFactionKey = INVADER_FACTION;
		}
		HST_EnemyStrategicResourceSaveValidationService validator
			= new HST_EnemyStrategicResourceSaveValidationService();
		validator.PrepareBeforeGenericNormalization(save, 67);
		validator.Normalize(save, 67);
		HST_FactionPoolState savedPool = FindSavedPool(save, OCCUPIER_FACTION);
		return validAccepted && savedPool && savedPool.m_iStrategicContractVersion
			== HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION;
	}

	protected bool ProveOrphanReceiptCapacityIsolation()
	{
		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_CampaignState state = BuildExactState("orphan_capacity", 1, false);
		HST_EnemyStrategicResourceService authority
			= new HST_EnemyStrategicResourceService();
		HST_EnemyStrategicMutationResult baseline = authority.ApplyResourceDelta(
			state,
			preset,
			"proof_orphan_capacity_baseline",
			OCCUPIER_FACTION,
			0,
			0,
			"proof_orphan_capacity",
			"proof_orphan_capacity_source");
		if (!baseline || !baseline.m_bAccepted)
			return false;

		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(state);
		array<ref HST_EnemyStrategicMutationState> orphanRows = {};
		int capacityStealingRowCount
			= HST_EnemyStrategicResourceSaveValidationService.MAX_PERIODIC_MUTATIONS + 1;
		for (int orphanIndex = 1; orphanIndex <= capacityStealingRowCount; orphanIndex++)
		{
			HST_EnemyStrategicMutationState orphan = BuildAdversarialReceipt(
				string.Format("proof_orphan_capacity_%1", orphanIndex),
				string.Format("proof_retired_enemy_%1", orphanIndex));
			orphanRows.Insert(orphan);
			save.m_aEnemyStrategicMutations.Insert(orphan);
		}
		HST_EnemyStrategicMutationState attributedInvalid
			= BuildAdversarialReceipt(
				"proof_attributed_invalid_receipt",
				INVADER_FACTION);
		attributedInvalid.m_sFingerprint = "forged_attributed_fingerprint";
		save.m_aEnemyStrategicMutations.Insert(attributedInvalid);
		HST_EnemyStrategicMutationState previouslyQuarantined
			= BuildAdversarialReceipt(
				"proof_previous_quarantine_tombstone",
				"proof_retired_enemy_tombstone");
		previouslyQuarantined.m_iContractVersion
			= HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION;
		previouslyQuarantined.m_bApplied = false;
		save.m_aEnemyStrategicMutations.Insert(previouslyQuarantined);
		int rowsBeforeValidation = save.m_aEnemyStrategicMutations.Count();

		HST_EnemyStrategicResourceSaveValidationService validator
			= new HST_EnemyStrategicResourceSaveValidationService();
		validator.PrepareBeforeGenericNormalization(save, 67);
		validator.Normalize(save, 67);
		HST_FactionPoolState acceptedPool = FindSavedPool(
			save,
			OCCUPIER_FACTION);
		HST_FactionPoolState rejectedPool = FindSavedPool(
			save,
			INVADER_FACTION);
		bool orphanRowsPurged = rowsBeforeValidation
			== capacityStealingRowCount + 3
			&& save.m_aEnemyStrategicMutations.Count() == 1
			&& FindSavedMutation(save, "proof_orphan_capacity_baseline") != null
			&& !FindSavedMutation(
				save,
				previouslyQuarantined.m_sMutationId);
		foreach (HST_EnemyStrategicMutationState orphanRow : orphanRows)
		{
			orphanRowsPurged = orphanRowsPurged && orphanRow
				&& orphanRow.m_iContractVersion
					== HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION
				&& !FindSavedMutation(save, orphanRow.m_sMutationId);
		}
		bool invalidGraphRejected = attributedInvalid
			&& attributedInvalid.m_iContractVersion
				== HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION
			&& !FindSavedMutation(
				save,
				attributedInvalid.m_sMutationId)
			&& rejectedPool
			&& rejectedPool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION
			&& acceptedPool
			&& acceptedPool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.CONTRACT_VERSION;

		HST_CampaignState restored = save.Restore();
		HST_EnemyStrategicResourceSaveValidationService roleValidator
			= new HST_EnemyStrategicResourceSaveValidationService();
		bool rolesRejected = restored
			&& !roleValidator.ValidateRestoredFactionRoles(restored, preset, 67);
		int acceptedRowsBefore;
		if (restored)
			acceptedRowsBefore = restored.m_aEnemyStrategicMutations.Count();
		HST_EnemyStrategicMutationResult afterPurge;
		if (restored)
			afterPurge = authority.ApplyResourceDelta(
				restored,
				preset,
				"proof_orphan_capacity_after_purge",
				OCCUPIER_FACTION,
				0,
				0,
				"proof_orphan_capacity",
				"proof_orphan_capacity_after_source");
		bool capacityUnaffected = afterPurge && afterPurge.m_bAccepted
			&& afterPurge.m_bChanged && restored
			&& acceptedRowsBefore == 1
			&& restored.m_aEnemyStrategicMutations.Count() == 2;
		return orphanRowsPurged && invalidGraphRejected
			&& rolesRejected && capacityUnaffected;
	}

	protected HST_EnemyStrategicMutationState BuildAdversarialReceipt(
		string mutationId,
		string factionKey)
	{
		HST_EnemyStrategicMutationState mutation
			= new HST_EnemyStrategicMutationState();
		mutation.m_iContractVersion
			= HST_EnemyStrategicResourceService.CONTRACT_VERSION;
		mutation.m_sMutationId = mutationId;
		mutation.m_sFactionKey = factionKey;
		mutation.m_sKind = "proof_orphan_receipt";
		mutation.m_sSourceId = mutationId + "_source";
		mutation.m_iOperationalSequence = 1;
		mutation.m_iCreatedAtSecond = 0;
		mutation.m_iPoolRevisionBefore = 1;
		mutation.m_iPoolRevisionAfter = 2;
		mutation.m_iAttackBefore = 10;
		mutation.m_iAttackAfter = 10;
		mutation.m_iSupportBefore = 10;
		mutation.m_iSupportAfter = 10;
		mutation.m_iAggressionBefore = 10;
		mutation.m_iAggressionAfter = 10;
		mutation.m_bApplied = true;
		mutation.m_sFingerprint
			= HST_EnemyStrategicResourceService.BuildMutationFingerprint(mutation);
		return mutation;
	}

	protected HST_CampaignState BuildLegacyState()
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = 66;
		state.m_iLastLoadedSchemaVersion = 66;
		state.m_iElapsedSeconds = 600;
		state.m_iEnemyResourceAccumulatorSeconds = 120;
		state.m_iAggressionAccumulatorSeconds = 45;
		state.m_aFactionPools.Clear();
		state.m_aEnemyStrategicMutations.Clear();
		state.m_aFactionPools.Insert(BuildPool(RESISTANCE_FACTION, 0, 0, 0, false));
		state.m_aFactionPools.Insert(BuildPool(OCCUPIER_FACTION, 70, 80, 12, false));
		state.m_aFactionPools.Insert(BuildPool(INVADER_FACTION, 35, 45, 12, false));
		return state;
	}

	protected HST_CampaignState BuildExactState(
		string suffix,
		int warLevel,
		bool reverseZones)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = HST_EnemyStrategicResourceService.SCHEMA_VERSION;
		state.m_iLastLoadedSchemaVersion
			= HST_EnemyStrategicResourceService.SCHEMA_VERSION;
		state.m_iCampaignSeed = 6700 + suffix.Hash();
		state.m_iWarLevel = warLevel;
		state.m_iElapsedSeconds = 0;
		state.m_aFactionPools.Clear();
		state.m_aEnemyStrategicMutations.Clear();
		state.m_aZones.Clear();
		state.m_aFactionPools.Insert(BuildPool(RESISTANCE_FACTION, 0, 0, 0, false));
		state.m_aFactionPools.Insert(BuildPool(OCCUPIER_FACTION, 100, 100, 12, true));
		state.m_aFactionPools.Insert(BuildPool(INVADER_FACTION, 60, 70, 12, true));
		HST_ZoneState usTown = BuildZone(
			"proof_us_town",
			OCCUPIER_FACTION,
			HST_EZoneType.HST_ZONE_TOWN,
			120,
			20,
			"");
		HST_ZoneState usFactory = BuildZone(
			"proof_us_factory",
			OCCUPIER_FACTION,
			HST_EZoneType.HST_ZONE_FACTORY,
			180,
			30,
			"");
		HST_ZoneState rivalSupplies = BuildZone(
			"proof_ussr_supplies",
			INVADER_FACTION,
			HST_EZoneType.HST_ZONE_RESOURCE,
			90,
			10,
			"supplies");
		if (reverseZones)
		{
			state.m_aZones.Insert(rivalSupplies);
			state.m_aZones.Insert(usFactory);
			state.m_aZones.Insert(usTown);
		}
		else
		{
			state.m_aZones.Insert(usTown);
			state.m_aZones.Insert(usFactory);
			state.m_aZones.Insert(rivalSupplies);
		}
		return state;
	}

	protected HST_FactionPoolState BuildPool(
		string factionKey,
		int attack,
		int support,
		int aggression,
		bool exact)
	{
		HST_FactionPoolState pool = new HST_FactionPoolState();
		pool.m_sFactionKey = factionKey;
		pool.m_iAttackResources = attack;
		pool.m_iSupportResources = support;
		pool.m_iAggression = aggression;
		pool.m_iStrategicContractVersion = 0;
		pool.m_iStrategicRevision = 0;
		if (exact)
		{
			pool.m_iStrategicContractVersion
				= HST_EnemyStrategicResourceService.CONTRACT_VERSION;
			pool.m_iStrategicRevision = 1;
		}
		return pool;
	}

	protected HST_ZoneState BuildZone(
		string zoneId,
		string ownerFactionKey,
		HST_EZoneType type,
		int incomeValue,
		int priority,
		string resourceKind)
	{
		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = zoneId;
		zone.m_sDisplayName = zoneId;
		zone.m_sOwnerFactionKey = ownerFactionKey;
		zone.m_eType = type;
		zone.m_iIncomeValue = incomeValue;
		zone.m_iPriority = priority;
		zone.m_sResourceKind = resourceKind;
		return zone;
	}

	protected HST_EnemyStrategicMutationState FindMutation(
		HST_CampaignState state,
		string mutationId)
	{
		HST_EnemyStrategicMutationState match;
		if (!state || mutationId.IsEmpty())
			return null;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (!mutation || mutation.m_sMutationId != mutationId)
				continue;
			if (match)
				return null;
			match = mutation;
		}
		return match;
	}

	protected HST_EnemyStrategicMutationState FindPeriodicMutation(
		HST_CampaignState state,
		string factionKey,
		string kind)
	{
		HST_EnemyStrategicMutationState match;
		if (!state)
			return null;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (!mutation || mutation.m_sFactionKey != factionKey
				|| mutation.m_sKind != kind)
				continue;
			if (match)
				return null;
			match = mutation;
		}
		return match;
	}

	protected int CountPeriodicMutations(
		HST_CampaignState state,
		string kind)
	{
		int count;
		if (!state)
			return count;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_sKind == kind)
				count++;
		}
		return count;
	}

	protected HST_EnemyStrategicMutationState FindSavedMutation(
		HST_CampaignSaveData save,
		string mutationId)
	{
		HST_EnemyStrategicMutationState match;
		if (!save || mutationId.IsEmpty())
			return null;
		foreach (HST_EnemyStrategicMutationState mutation : save.m_aEnemyStrategicMutations)
		{
			if (!mutation || mutation.m_sMutationId != mutationId)
				continue;
			if (match)
				return null;
			match = mutation;
		}
		return match;
	}

	protected HST_FactionPoolState FindSavedPool(
		HST_CampaignSaveData save,
		string factionKey)
	{
		HST_FactionPoolState match;
		if (!save || factionKey.IsEmpty())
			return null;
		foreach (HST_FactionPoolState pool : save.m_aFactionPools)
		{
			if (!pool || pool.m_sFactionKey != factionKey)
				continue;
			if (match)
				return null;
			match = pool;
		}
		return match;
	}

	protected int CountQuarantinedSavedPools(
		HST_CampaignSaveData save,
		string factionKey)
	{
		int count;
		if (!save)
			return count;
		foreach (HST_FactionPoolState pool : save.m_aFactionPools)
		{
			if (pool && pool.m_sFactionKey == factionKey
				&& pool.m_iStrategicContractVersion
					== HST_EnemyStrategicResourceService.QUARANTINE_CONTRACT_VERSION)
				count++;
		}
		return count;
	}
}
#endif
