class HST_EnemyPlanningProofReport
{
	bool m_bPre68BaselineExact;
	bool m_bIndependentCadenceExact;
	bool m_bBeginReplayConflictExact;
	bool m_bCommitmentPermutationExact;
	bool m_bFrozenDecisionExact;
	bool m_bRetryEnvelopeExact;
	bool m_bPreparedPressureCrashWindowExact;
	bool m_bPreparedOrderAdoptionExact;
	bool m_bRetryTamperQuarantineExact;
	bool m_bZeroTargetSkipExact;
	bool m_bCommittedRoundtripExact;
	bool m_bCurrentQuarantineExact;
	bool m_bFreshBootstrapExact;
	bool m_bUnavailableLogThrottleExact;
	string m_sBaselineCadenceEvidence;
	string m_sDecisionEvidence;
	string m_sFreezeRetryEvidence;
	string m_sRecoveryEvidence;
	string m_sPersistenceQuarantineEvidence;
	string m_sBootstrapThrottleEvidence;

	bool AllExact()
	{
		return m_bPre68BaselineExact
			&& m_bIndependentCadenceExact
			&& m_bBeginReplayConflictExact
			&& m_bCommitmentPermutationExact
			&& m_bFrozenDecisionExact
			&& m_bRetryEnvelopeExact
			&& m_bPreparedPressureCrashWindowExact
			&& m_bPreparedOrderAdoptionExact
			&& m_bRetryTamperQuarantineExact
			&& m_bZeroTargetSkipExact
			&& m_bCommittedRoundtripExact
			&& m_bCurrentQuarantineExact
			&& m_bFreshBootstrapExact
			&& m_bUnavailableLogThrottleExact;
	}

	string BuildReport()
	{
		string report = string.Format(
			"enemy planning proof | all %1 | baseline %2 | cadence %3 | begin/replay/conflict %4 | commitments %5 | frozen %6 | retry %7 | zero-target %8 | roundtrip %9",
			AllExact(),
			m_bPre68BaselineExact,
			m_bIndependentCadenceExact,
			m_bBeginReplayConflictExact,
			m_bCommitmentPermutationExact,
			m_bFrozenDecisionExact,
			m_bRetryEnvelopeExact,
			m_bZeroTargetSkipExact,
			m_bCommittedRoundtripExact);
		return report + string.Format(
			" | pressure-crash %1 | order-adoption %2 | retry-tamper %3 | quarantine %4",
			m_bPreparedPressureCrashWindowExact,
			m_bPreparedOrderAdoptionExact,
			m_bRetryTamperQuarantineExact,
			m_bCurrentQuarantineExact)
			+ string.Format(
				" | fresh-bootstrap %1 | warning-throttle %2",
				m_bFreshBootstrapExact,
				m_bUnavailableLogThrottleExact);
	}
}

// Source-only deterministic Schema-68 proof. Coordinator wiring may call
// BuildAuthorityReport after the production commander integration is available.
class HST_EnemyPlanningProofService
{
	static const string RESISTANCE_FACTION = "FIA";
	static const string OCCUPIER_FACTION = "US";
	static const string INVADER_FACTION = "USSR";
	protected ref HST_EnemyPlanningAuthorityService m_Authority
		= new HST_EnemyPlanningAuthorityService();

	HST_EnemyPlanningProofReport BuildAuthorityReport()
	{
		HST_EnemyPlanningProofReport report
			= new HST_EnemyPlanningProofReport();
		ProvePre68Baseline(report);
		ProveIndependentCadence(report);
		ProveBeginReplayConflict(report);
		ProveCommitmentPermutation(report);
		ProveFrozenDecision(report);
		ProveRetryEnvelope(report);
		ProvePreparedPressureCrashWindow(report);
		ProvePreparedOrderAdoption(report);
		ProveRetryTamperQuarantine(report);
		ProveZeroTargetSkip(report);
		ProveCommittedRoundtrip(report);
		ProveCurrentQuarantine(report);
		ProveFreshBootstrap(report);
		ProveUnavailableLogThrottle(report);
		return report;
	}

	HST_EnemyPlanningProofReport BuildReport()
	{
		return BuildAuthorityReport();
	}

	protected void ProvePre68Baseline(HST_EnemyPlanningProofReport report)
	{
		HST_CampaignPreset preset = BuildPreset();
		HST_CampaignState legacy = BuildExactState(420, false);
		legacy.m_iSchemaVersion = 67;
		legacy.m_iLastLoadedSchemaVersion = 67;
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(legacy);
		save.m_iSchemaVersion = 67;
		save.m_iLastLoadedSchemaVersion = 67;
		HST_EnemyPlanningSaveValidationService validator
			= new HST_EnemyPlanningSaveValidationService();
		validator.PrepareBeforeGenericNormalization(save, 67);
		bool noSavedDecision = save.m_aEnemyPlanningStates.IsEmpty();
		HST_CampaignState restored = save.Restore();
		bool adopted = restored
			&& validator.ValidateRestoredFactionRoles(restored, preset, 67);
		HST_EnemyPlanningState occupier;
		HST_EnemyPlanningState invader;
		if (restored)
		{
			occupier = restored.FindEnemyPlanningState(OCCUPIER_FACTION);
			invader = restored.FindEnemyPlanningState(INVADER_FACTION);
		}
		bool occupierExact = BaselineExact(occupier, 420);
		bool invaderExact = BaselineExact(invader, 420);
		report.m_bPre68BaselineExact = noSavedDecision && adopted
			&& occupierExact && invaderExact
			&& restored.m_aEnemyPlanningStates.Count() == 2;
		report.m_sBaselineCadenceEvidence = string.Format(
			"pre68 empty/adopted %1/%2 | US/USSR baseline %3/%4 | rows %5",
			noSavedDecision,
			adopted,
			occupierExact,
			invaderExact,
			restored && restored.m_aEnemyPlanningStates.Count());
	}

	protected void ProveIndependentCadence(
		HST_EnemyPlanningProofReport report)
	{
		HST_CampaignState state = BuildExactState(0, true);
		state.m_iElapsedSeconds = 179;
		HST_EnemyPlanningState occupier
			= state.FindEnemyPlanningState(OCCUPIER_FACTION);
		HST_EnemyPlanningState invader
			= state.FindEnemyPlanningState(INVADER_FACTION);
		bool beforeDue = occupier && invader
			&& !m_Authority.IsDue(occupier, 179)
			&& !m_Authority.IsDue(invader, 179);
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(state);
		HST_CampaignState resumed = new HST_CampaignState();
		save.ApplyTo(resumed);
		HST_EnemyPlanningSaveValidationService validator
			= new HST_EnemyPlanningSaveValidationService();
		bool restoredExact = validator.ValidateRestoredFactionRoles(
			resumed,
			BuildPreset(),
			68);
		resumed.m_iElapsedSeconds = 180;
		occupier = resumed.FindEnemyPlanningState(OCCUPIER_FACTION);
		invader = resumed.FindEnemyPlanningState(INVADER_FACTION);
		bool dueAtBoundary = restoredExact && occupier && invader
			&& m_Authority.IsDue(occupier, 180)
			&& m_Authority.IsDue(invader, 180);
		HST_EnemyPlanningDecisionCommand occupierCommand
			= BuildCommand(resumed, occupier, "proof_us_target", "proof_us_source", 1);
		HST_EnemyPlanningDecisionResult occupierBegin
			= m_Authority.BeginDecision(occupier, occupierCommand);
		bool rivalUntouched = occupierBegin && occupierBegin.m_bAccepted
			&& invader.m_iDecisionSequence == 0
			&& invader.m_iLastPlanningBucketSecond == 0
			&& invader.m_iNextPlanningBucketSecond == 180;
		HST_EnemyPlanningDecisionCommand invaderCommand
			= BuildCommand(resumed, invader, "proof_ussr_target", "proof_ussr_source", 1);
		HST_EnemyPlanningDecisionResult invaderBegin
			= m_Authority.BeginDecision(invader, invaderCommand);
		bool checkpointsExact = invaderBegin && invaderBegin.m_bAccepted
			&& occupier.m_iDecisionSequence == 1
			&& invader.m_iDecisionSequence == 1
			&& occupier.m_iLastPlanningBucketSecond == 180
			&& invader.m_iLastPlanningBucketSecond == 180
			&& occupier.m_iNextPlanningBucketSecond == 360
			&& invader.m_iNextPlanningBucketSecond == 360;
		report.m_bIndependentCadenceExact = beforeDue && dueAtBoundary
			&& rivalUntouched && checkpointsExact;
		report.m_sBaselineCadenceEvidence
			= report.m_sBaselineCadenceEvidence + string.Format(
				" | 179/due/rival/checkpoints %1/%2/%3/%4",
				beforeDue,
				dueAtBoundary,
				rivalUntouched,
				checkpointsExact);
	}

	protected void ProveBeginReplayConflict(
		HST_EnemyPlanningProofReport report)
	{
		HST_CampaignState state = BuildExactState(180, true, 0);
		HST_EnemyPlanningState planning
			= state.FindEnemyPlanningState(OCCUPIER_FACTION);
		HST_EnemyPlanningDecisionCommand command = BuildCommand(
			state,
			planning,
			"proof_us_target",
			"proof_us_source",
			1);
		HST_EnemyPlanningDecisionResult begun
			= m_Authority.BeginDecision(planning, command);
		int revisionAfterBegin = planning.m_iRevision;
		string fingerprintAfterBegin = planning.m_sDecisionFingerprint;
		string orderIdAfterBegin = planning.m_sPlannedOrderId;
		int attackCostAfterBegin = planning.m_iAttackCost;
		HST_EnemyPlanningDecisionResult replay
			= m_Authority.BeginDecision(planning, command);
		bool replayExact = begun && begun.m_bAccepted && begun.m_bChanged
			&& replay && replay.m_bAccepted && replay.m_bAlreadyApplied
			&& !replay.m_bChanged && planning.m_iRevision == revisionAfterBegin;
		command.m_iAttackCost++;
		HST_EnemyPlanningDecisionResult conflict
			= m_Authority.BeginDecision(planning, command);
		bool conflictExact = conflict && !conflict.m_bAccepted
			&& planning.m_iContractVersion
				== HST_EnemyPlanningAuthorityService.QUARANTINE_CONTRACT_VERSION
			&& planning.m_sDecisionFingerprint == fingerprintAfterBegin
			&& planning.m_sPlannedOrderId == orderIdAfterBegin
			&& planning.m_iAttackCost == attackCostAfterBegin;
		report.m_bBeginReplayConflictExact = replayExact && conflictExact;
		report.m_sDecisionEvidence = string.Format(
			"begin/replay/conflict %1/%2/%3 | revision %4 | disposition %5",
			begun && begun.m_bAccepted,
			replayExact,
			conflictExact,
			planning.m_iRevision,
			planning.m_sDisposition);
	}

	protected void ProveCommitmentPermutation(
		HST_EnemyPlanningProofReport report)
	{
		HST_CampaignState state = BuildExactState(180, true, 0);
		AddCommitmentFixtures(state);
		int firstCount;
		string first = HST_EnemyPlanningAuthorityService.BuildCommitmentFingerprint(
			state,
			OCCUPIER_FACTION,
			firstCount);
		ReverseCommitmentArrays(state);
		int secondCount;
		string second = HST_EnemyPlanningAuthorityService.BuildCommitmentFingerprint(
			state,
			OCCUPIER_FACTION,
			secondCount);
		report.m_bCommitmentPermutationExact = !first.IsEmpty()
			&& first == second && firstCount == 6 && secondCount == 6;
		report.m_sDecisionEvidence = report.m_sDecisionEvidence + string.Format(
			" | commitment stable/count %1/%2",
			first == second,
			firstCount);
	}

	protected void ProveFrozenDecision(HST_EnemyPlanningProofReport report)
	{
		HST_CampaignState state = BuildExactState(180, true, 0);
		HST_EnemyPlanningState planning
			= state.FindEnemyPlanningState(OCCUPIER_FACTION);
		HST_EnemyPlanningDecisionCommand command = BuildCommand(
			state,
			planning,
			"proof_us_target",
			"proof_us_source",
			1);
		HST_EnemyPlanningDecisionResult begun
			= m_Authority.BeginDecision(planning, command);
		HST_EnemyPlanningState snapshot = CopyPlanningThroughSave(state, OCCUPIER_FACTION);
		state.m_iWarLevel += 4;
		HST_FactionPoolState pool = state.FindFactionPool(OCCUPIER_FACTION);
		pool.m_iAttackResources += 25;
		pool.m_iSupportResources += 15;
		pool.m_iAggression += 7;
		pool.m_iStrategicRevision++;
		HST_ZoneState target = state.FindZone("proof_us_target");
		target.m_sOwnerFactionKey = RESISTANCE_FACTION;
		target.m_iPriority += 50;
		bool frozen = begun && begun.m_bAccepted
			&& SameFrozenDecision(snapshot, planning)
			&& planning.m_sInputFingerprint
				== HST_EnemyPlanningAuthorityService.BuildInputFingerprint(planning)
			&& planning.m_sDecisionFingerprint
				== HST_EnemyPlanningAuthorityService.BuildDecisionFingerprint(planning);
		report.m_bFrozenDecisionExact = frozen;
		report.m_sFreezeRetryEvidence = string.Format(
			"live mutation frozen %1 | war %2 -> %3 | pool revision %4",
			frozen,
			snapshot && snapshot.m_iObservedWarLevel,
			state.m_iWarLevel,
			pool.m_iStrategicRevision);
	}

	protected void ProveRetryEnvelope(HST_EnemyPlanningProofReport report)
	{
		HST_CampaignState state = BuildExactState(180, true, 0);
		HST_EnemyPlanningState planning
			= state.FindEnemyPlanningState(OCCUPIER_FACTION);
		HST_EnemyPlanningDecisionCommand command = BuildCommand(
			state,
			planning,
			"proof_us_target",
			"proof_us_source",
			1);
		command.m_iTargetPressureBefore = 5;
		command.m_iTargetPressureDelta = 3;
		command.m_iTargetPressureAfter = 8;
		HST_EnemyPlanningDecisionResult begun = m_Authority.BeginDecision(
			planning,
			command);
		string inputFingerprint = planning.m_sInputFingerprint;
		string decisionFingerprint = planning.m_sDecisionFingerprint;
		string decisionId = planning.m_sDecisionId;
		int pressureRevisionBefore = planning.m_iRevision;
		HST_EnemyPlanningDecisionResult pressure
			= m_Authority.MarkTargetPressureApplied(planning);
		int pressureRevisionAfter = planning.m_iRevision;
		HST_EnemyPlanningDecisionResult pressureReplay
			= m_Authority.MarkTargetPressureApplied(planning);
		bool pressureExact = pressure && pressure.m_bAccepted && pressure.m_bChanged
			&& pressureReplay && pressureReplay.m_bAccepted
			&& pressureReplay.m_bAlreadyApplied && !pressureReplay.m_bChanged
			&& planning.m_bTargetPressureApplied
			&& pressureRevisionAfter == pressureRevisionBefore + 1
			&& planning.m_iRevision == pressureRevisionAfter
			&& planning.m_sInputFingerprint == inputFingerprint
			&& planning.m_sDecisionFingerprint == decisionFingerprint
			&& planning.m_sDecisionId == decisionId;
		HST_EnemyPlanningState snapshot = CopyPlanningThroughSave(state, OCCUPIER_FACTION);
		int revisionBefore = planning.m_iRevision;
		HST_EnemyPlanningDecisionResult retry
			= m_Authority.RecordRetry(planning, "proof retry", 200);
		bool firstExact = begun && begun.m_bAccepted && retry
			&& retry.m_bAccepted && retry.m_bChanged
			&& planning.m_iNextRetrySecond == 230
			&& planning.m_iRevision == revisionBefore + 1
			&& planning.m_sFailureReason == "proof retry"
			&& SameFrozenDecision(snapshot, planning);
		int revisionAfter = planning.m_iRevision;
		HST_EnemyPlanningDecisionResult early
			= m_Authority.RecordRetry(planning, "changed too early", 201);
		bool earlyExact = early && early.m_bAccepted
			&& early.m_bAlreadyApplied && !early.m_bChanged
			&& planning.m_iRevision == revisionAfter
			&& planning.m_sFailureReason == "proof retry"
			&& SameFrozenDecision(snapshot, planning);
		HST_EnemyPlanningDecisionResult due
			= m_Authority.RecordRetry(planning, "proof retry due", 230);
		bool dueExact = due && due.m_bAccepted && due.m_bChanged
			&& !due.m_bAlreadyApplied
			&& planning.m_iNextRetrySecond == 260
			&& planning.m_iRevision == revisionAfter + 1
			&& planning.m_sFailureReason == "proof retry due"
			&& SameFrozenDecision(snapshot, planning);
		int revisionAfterDue = planning.m_iRevision;
		int retryAfterDue = planning.m_iNextRetrySecond;
		string failureAfterDue = planning.m_sFailureReason;
		HST_EnemyPlanningDecisionResult overflow
			= m_Authority.RecordRetry(planning, "overflow", int.MAX);
		bool overflowExact = overflow && !overflow.m_bAccepted
			&& !overflow.m_bChanged && !overflow.m_bAlreadyApplied
			&& planning.m_iRevision == revisionAfterDue
			&& planning.m_iNextRetrySecond == retryAfterDue
			&& planning.m_sFailureReason == failureAfterDue
			&& SameFrozenDecision(snapshot, planning);
		report.m_bRetryEnvelopeExact = pressureExact && firstExact && earlyExact
			&& dueExact && overflowExact;
		report.m_sFreezeRetryEvidence
			= report.m_sFreezeRetryEvidence + string.Format(
				" | pressure/replay/retry/early/due/overflow/frozen %1/%2/%3/%4/%5/%6/%7 | next %8",
				pressure && pressure.m_bAccepted,
				pressureReplay && pressureReplay.m_bAlreadyApplied,
				firstExact,
				earlyExact,
				dueExact,
				overflowExact,
				SameFrozenDecision(snapshot, planning),
				planning.m_iNextRetrySecond);
	}

	protected void ProvePreparedPressureCrashWindow(
		HST_EnemyPlanningProofReport report)
	{
		HST_CampaignState state = BuildExactState(180, true, 0);
		HST_EnemyPlanningState planning
			= state.FindEnemyPlanningState(OCCUPIER_FACTION);
		HST_EnemyPlanningDecisionCommand command = BuildCommand(
			state,
			planning,
			"proof_us_target",
			"proof_us_source",
			1);
		command.m_iTargetPressureBefore = 5;
		command.m_iTargetPressureDelta = 3;
		command.m_iTargetPressureAfter = 8;
		command.m_bTargetPressureApplied = false;
		HST_EnemyPlanningDecisionResult begun
			= m_Authority.BeginDecision(planning, command);
		HST_EnemyPlanningState frozen = CopyPlanningThroughSave(
			state,
			OCCUPIER_FACTION);
		int revisionBeforeSave = planning.m_iRevision;
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(state);
		HST_EnemyPlanningSaveValidationService validator
			= new HST_EnemyPlanningSaveValidationService();
		validator.PrepareBeforeGenericNormalization(save, 68);
		validator.Normalize(save, 68);
		HST_CampaignState restored = save.Restore();
		bool rolesExact = restored && validator.ValidateRestoredFactionRoles(
			restored,
			BuildPreset(),
			68);
		HST_EnemyPlanningState restoredPlanning;
		if (restored)
			restoredPlanning = restored.FindEnemyPlanningState(OCCUPIER_FACTION);
		bool crashWindowExact = begun && begun.m_bAccepted && frozen
			&& rolesExact && restoredPlanning
			&& restoredPlanning.m_iContractVersion
				== HST_EnemyPlanningAuthorityService.CONTRACT_VERSION
			&& restoredPlanning.m_sDisposition == "prepared"
			&& !restoredPlanning.m_bTargetPressureApplied
			&& restoredPlanning.m_iTargetPressureBefore == 5
			&& restoredPlanning.m_iTargetPressureDelta == 3
			&& restoredPlanning.m_iTargetPressureAfter == 8
			&& restoredPlanning.m_iRevision == revisionBeforeSave
			&& SameFrozenDecision(frozen, restoredPlanning)
			&& restored.m_aEnemyOrders.IsEmpty()
			&& restored.m_aEnemyStrategicMutations.IsEmpty();
		report.m_bPreparedPressureCrashWindowExact = crashWindowExact;
		report.m_sRecoveryEvidence = string.Format(
			"prepared pressure crash begun/roles/prepared/unapplied/frozen/empty %1/%2/%3/%4/%5/%6",
			begun && begun.m_bAccepted,
			rolesExact,
			restoredPlanning && restoredPlanning.m_sDisposition == "prepared",
			restoredPlanning && !restoredPlanning.m_bTargetPressureApplied,
			SameFrozenDecision(frozen, restoredPlanning),
			restored && restored.m_aEnemyOrders.IsEmpty()
				&& restored.m_aEnemyStrategicMutations.IsEmpty());
	}

	protected void ProvePreparedOrderAdoption(
		HST_EnemyPlanningProofReport report)
	{
		HST_CampaignState state;
		HST_EnemyPlanningState planning;
		HST_EnemyOrderState order;
		bool built = BuildPreparedOrderFixture(state, planning, order);
		HST_EnemyPlanningState frozen;
		int revisionBeforeSave;
		if (planning)
		{
			frozen = CopyPlanningThroughSave(state, OCCUPIER_FACTION);
			revisionBeforeSave = planning.m_iRevision;
		}
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		if (state)
			save.Capture(state);
		HST_EnemyPlanningSaveValidationService validator
			= new HST_EnemyPlanningSaveValidationService();
		validator.PrepareBeforeGenericNormalization(save, 68);
		validator.Normalize(save, 68);
		HST_CampaignState restored = save.Restore();
		bool rolesExact = restored && validator.ValidateRestoredFactionRoles(
			restored,
			BuildPreset(),
			68);
		HST_EnemyPlanningState restoredPlanning;
		HST_EnemyOrderState restoredOrder;
		HST_EnemyStrategicMutationState restoredDebit;
		if (restored && planning)
		{
			restoredPlanning = restored.FindEnemyPlanningState(OCCUPIER_FACTION);
			restoredOrder = FindOrderForDecision(restored, planning.m_sDecisionId);
			restoredDebit = restored.FindEnemyStrategicMutation(
				planning.m_sPlannedDebitMutationId);
		}
		bool backlinkExact = ExactPlanningOrderBacklink(
			restoredPlanning,
			restoredOrder);
		bool adoptionExact = built && rolesExact && frozen
			&& restoredPlanning && restoredOrder && restoredDebit
			&& restoredPlanning.m_sDisposition == "committed"
			&& restoredPlanning.m_iRevision == revisionBeforeSave
			&& SameFrozenDecision(frozen, restoredPlanning)
			&& backlinkExact
			&& restoredDebit.m_sMutationId
				== restoredPlanning.m_sPlannedDebitMutationId
			&& restoredDebit.m_sOrderId == restoredOrder.m_sOrderId
			&& restoredDebit.m_sOperationId == restoredOrder.m_sOperationId
			&& restoredDebit.m_sManifestId == restoredOrder.m_sManifestId
			&& restoredDebit.m_sZoneId == restoredOrder.m_sTargetZoneId;
		report.m_bPreparedOrderAdoptionExact = adoptionExact;
		report.m_sRecoveryEvidence = report.m_sRecoveryEvidence + string.Format(
			" | prepared order built/roles/adopted/revision/backlink/debit %1/%2/%3/%4/%5/%6",
			built,
			rolesExact,
			restoredPlanning && restoredPlanning.m_sDisposition == "committed",
			restoredPlanning && restoredPlanning.m_iRevision == revisionBeforeSave,
			backlinkExact,
			restoredDebit != null);
	}

	protected void ProveRetryTamperQuarantine(
		HST_EnemyPlanningProofReport report)
	{
		HST_CampaignState state = BuildExactState(180, true, 0);
		HST_EnemyPlanningState planning
			= state.FindEnemyPlanningState(OCCUPIER_FACTION);
		HST_EnemyPlanningDecisionResult begun = m_Authority.BeginDecision(
			planning,
			BuildCommand(
				state,
				planning,
				"proof_us_target",
				"proof_us_source",
				1));
		HST_EnemyPlanningDecisionResult retry
			= m_Authority.RecordRetry(planning, "proof retry", 200);
		HST_FactionPoolState pool = state.FindFactionPool(OCCUPIER_FACTION);
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		int poolRevisionBefore = pool.m_iStrategicRevision;
		int planningRevisionBefore = planning.m_iRevision;
		int tamperedAttackCost = planning.m_iAttackCost + 1;
		planning.m_iAttackCost = tamperedAttackCost;
		HST_EnemyPlanningSaveValidationService validator
			= new HST_EnemyPlanningSaveValidationService();
		bool valid = validator.ValidateRestoredFactionRoles(
			state,
			BuildPreset(),
			68);
		bool tamperExact = begun && begun.m_bAccepted
			&& retry && retry.m_bAccepted && retry.m_bChanged
			&& !valid
			&& planning.m_iContractVersion
				== HST_EnemyPlanningAuthorityService.QUARANTINE_CONTRACT_VERSION
			&& planning.m_sDisposition == "quarantined"
			&& planning.m_sAuthorityFailure.Contains("fingerprint")
			&& planning.m_iRevision == planningRevisionBefore + 1
			&& planning.m_iAttackCost == tamperedAttackCost
			&& pool.m_iAttackResources == attackBefore
			&& pool.m_iSupportResources == supportBefore
			&& pool.m_iStrategicRevision == poolRevisionBefore
			&& state.m_aEnemyOrders.IsEmpty()
			&& state.m_aEnemyStrategicMutations.IsEmpty();
		report.m_bRetryTamperQuarantineExact = tamperExact;
		report.m_sRecoveryEvidence = report.m_sRecoveryEvidence + string.Format(
			" | retry tamper begin/retry/rejected/quarantined/revision/pool-clean %1/%2/%3/%4/%5/%6",
			begun && begun.m_bAccepted,
			retry && retry.m_bAccepted,
			!valid,
			planning.m_iContractVersion
				== HST_EnemyPlanningAuthorityService.QUARANTINE_CONTRACT_VERSION,
			planning.m_iRevision == planningRevisionBefore + 1,
			pool.m_iAttackResources == attackBefore
				&& pool.m_iSupportResources == supportBefore
				&& pool.m_iStrategicRevision == poolRevisionBefore);
	}

	protected void ProveZeroTargetSkip(HST_EnemyPlanningProofReport report)
	{
		HST_CampaignState state = BuildExactState(180, true, 0);
		HST_EnemyPlanningState planning
			= state.FindEnemyPlanningState(OCCUPIER_FACTION);
		HST_EnemyPlanningDecisionCommand command = BuildCommand(
			state,
			planning,
			"",
			"",
			0);
		HST_EnemyPlanningDecisionResult begun
			= m_Authority.BeginDecision(planning, command);
		HST_EnemyPlanningDecisionResult skipped = m_Authority.CompleteDecision(
			planning,
			"skipped",
			null,
			"no eligible target");
		HST_EnemyPlanningSaveValidationService validator
			= new HST_EnemyPlanningSaveValidationService();
		bool shapeExact = validator.ValidateRestoredFactionRoles(
			state,
			BuildPreset(),
			68);
		report.m_bZeroTargetSkipExact = begun && begun.m_bAccepted
			&& skipped && skipped.m_bAccepted && shapeExact
			&& planning.m_iDecisionSequence == 1
			&& planning.m_iLastPlanningBucketSecond == 180
			&& planning.m_iNextPlanningBucketSecond == 360
			&& planning.m_sDisposition == "skipped"
			&& planning.m_iTargetCandidateCount == 0
			&& planning.m_sSelectedTargetZoneId.IsEmpty()
			&& state.m_aEnemyOrders.IsEmpty();
		report.m_sDecisionEvidence = report.m_sDecisionEvidence + string.Format(
			" | zero-target skipped/shape/bucket %1/%2/%3",
			skipped && skipped.m_bAccepted,
			shapeExact,
			planning.m_iLastPlanningBucketSecond);
	}

	protected void ProveCommittedRoundtrip(
		HST_EnemyPlanningProofReport report)
	{
		HST_CampaignState state;
		HST_EnemyPlanningState planning;
		HST_EnemyOrderState order;
		bool built = BuildCommittedFixture(state, planning, order);
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		if (state)
			save.Capture(state);
		HST_EnemyPlanningSaveValidationService validator
			= new HST_EnemyPlanningSaveValidationService();
		validator.PrepareBeforeGenericNormalization(save, 68);
		validator.Normalize(save, 68);
		HST_CampaignState restored = save.Restore();
		bool rolesExact = restored && validator.ValidateRestoredFactionRoles(
			restored,
			BuildPreset(),
			68);
		HST_EnemyPlanningState restoredPlanning;
		HST_EnemyOrderState restoredOrder;
		HST_EnemyStrategicMutationState restoredDebit;
		if (restored)
		{
			restoredPlanning = restored.FindEnemyPlanningState(OCCUPIER_FACTION);
			restoredOrder = FindOrderForDecision(restored, planning.m_sDecisionId);
			restoredDebit = restored.FindEnemyStrategicMutation(
				planning.m_sPlannedDebitMutationId);
		}
		bool roundtrip = built && rolesExact && restoredPlanning
			&& restoredOrder && restoredDebit
			&& restoredPlanning.m_sDisposition == "committed"
			&& SameFrozenDecision(planning, restoredPlanning)
			&& restoredOrder.m_sPlanningDecisionId
				== restoredPlanning.m_sDecisionId
			&& restoredDebit.m_sOrderId == restoredOrder.m_sOrderId;
		report.m_bCommittedRoundtripExact = roundtrip;
		report.m_sPersistenceQuarantineEvidence = string.Format(
			"committed built/roles/plan/order/debit/roundtrip %1/%2/%3/%4/%5/%6",
			built,
			rolesExact,
			restoredPlanning != null,
			restoredOrder != null,
			restoredDebit != null,
			roundtrip);
	}

	protected void ProveCurrentQuarantine(
		HST_EnemyPlanningProofReport report)
	{
		bool fingerprint = ProveFingerprintQuarantine();
		bool backlink = ProveBacklinkQuarantine();
		bool duplicate = ProveDuplicateQuarantine();
		bool missing = ProveMissingQuarantine();
		bool wrongRole = ProveWrongRoleQuarantine();
		report.m_bCurrentQuarantineExact = fingerprint && backlink
			&& duplicate && missing && wrongRole;
		report.m_sPersistenceQuarantineEvidence
			= report.m_sPersistenceQuarantineEvidence + string.Format(
				" | quarantine fingerprint/backlink/duplicate/missing/wrong-role %1/%2/%3/%4/%5",
				fingerprint,
				backlink,
				duplicate,
				missing,
				wrongRole);
	}

	protected void ProveFreshBootstrap(HST_EnemyPlanningProofReport report)
	{
		HST_CampaignPreset preset = BuildPreset();
		HST_BalanceConfig balance = HST_DefaultCatalog.CreateBalance();
		HST_RuntimeSettings settings = new HST_RuntimeSettings();
		HST_CampaignState state
			= HST_CampaignBootstrapService.CreateInitialCampaignState(
				preset,
				balance,
				settings);

		HST_EnemyStrategicResourceSaveValidationService strategicValidator
			= new HST_EnemyStrategicResourceSaveValidationService();
		HST_EnemyPlanningSaveValidationService planningValidator
			= new HST_EnemyPlanningSaveValidationService();
		bool strategicValid = strategicValidator.ValidateRestoredFactionRoles(
			state,
			preset,
			HST_CampaignState.SCHEMA_VERSION);
		bool planningValid = planningValidator.ValidateRestoredFactionRoles(
			state,
			preset,
			HST_CampaignState.SCHEMA_VERSION);

		HST_FactionPoolState occupierPool
			= state.FindFactionPool(preset.m_sOccupierFactionKey);
		HST_FactionPoolState invaderPool
			= state.FindFactionPool(preset.m_sInvaderFactionKey);
		HST_EnemyPlanningState occupierPlanning
			= state.FindEnemyPlanningState(preset.m_sOccupierFactionKey);
		HST_EnemyPlanningState invaderPlanning
			= state.FindEnemyPlanningState(preset.m_sInvaderFactionKey);
		bool occupierPoolExact = FreshPoolExact(
			occupierPool,
			balance.m_iStartingOccupierAttackPool,
			balance.m_iStartingOccupierSupportPool);
		bool invaderPoolExact = FreshPoolExact(
			invaderPool,
			balance.m_iStartingInvaderAttackPool,
			balance.m_iStartingInvaderSupportPool);
		bool planningExact = BaselineExact(occupierPlanning, 0)
			&& BaselineExact(invaderPlanning, 0)
			&& state.m_aEnemyPlanningStates.Count() == 2;
		report.m_bFreshBootstrapExact = strategicValid && planningValid
			&& state.m_aFactionPools.Count() == 3
			&& occupierPoolExact && invaderPoolExact && planningExact;
		report.m_sBootstrapThrottleEvidence = string.Format(
			"fresh bootstrap | validators %1/%2 | rows %3/%4 | pools %5/%6 | planning %7",
			strategicValid,
			planningValid,
			state.m_aFactionPools.Count(),
			state.m_aEnemyPlanningStates.Count(),
			occupierPoolExact,
			invaderPoolExact,
			planningExact);
	}

	protected void ProveUnavailableLogThrottle(
		HST_EnemyPlanningProofReport report)
	{
		HST_EnemyCommanderService commander = new HST_EnemyCommanderService();
		bool first = commander.ShouldReportPlanningUnavailable(
			OCCUPIER_FACTION,
			"proof_failure_a",
			10);
		bool repeatSuppressed = !commander.ShouldReportPlanningUnavailable(
			OCCUPIER_FACTION,
			"proof_failure_a",
			11);
		bool beforeReminderSuppressed
			= !commander.ShouldReportPlanningUnavailable(
				OCCUPIER_FACTION,
				"proof_failure_a",
				309);
		bool reminder = commander.ShouldReportPlanningUnavailable(
			OCCUPIER_FACTION,
			"proof_failure_a",
			310);
		bool changedFailure = commander.ShouldReportPlanningUnavailable(
			OCCUPIER_FACTION,
			"proof_failure_b",
			311);
		bool cleared = commander.ClearPlanningUnavailableReportState(
			OCCUPIER_FACTION);
		bool afterRecovery = commander.ShouldReportPlanningUnavailable(
			OCCUPIER_FACTION,
			"proof_failure_b",
			312);
		report.m_bUnavailableLogThrottleExact = first && repeatSuppressed
			&& beforeReminderSuppressed && reminder && changedFailure
			&& cleared && afterRecovery;
		report.m_sBootstrapThrottleEvidence
			= report.m_sBootstrapThrottleEvidence + string.Format(
				" | warning first/repeat/pre300/300/changed/clear/rearm %1/%2/%3/%4/%5/%6/%7",
				first,
				repeatSuppressed,
				beforeReminderSuppressed,
				reminder,
				changedFailure,
				cleared,
				afterRecovery);
	}

	protected bool ProveFingerprintQuarantine()
	{
		HST_CampaignState state = BuildExactState(180, true, 0);
		HST_EnemyPlanningState planning
			= state.FindEnemyPlanningState(OCCUPIER_FACTION);
		HST_EnemyPlanningDecisionResult begun = m_Authority.BeginDecision(
			planning,
			BuildCommand(
				state,
				planning,
				"proof_us_target",
				"proof_us_source",
				1));
		if (!begun || !begun.m_bAccepted)
			return false;
		planning.m_sDecisionFingerprint = planning.m_sDecisionFingerprint + "_tamper";
		HST_EnemyPlanningSaveValidationService validator
			= new HST_EnemyPlanningSaveValidationService();
		bool valid = validator.ValidateRestoredFactionRoles(
			state,
			BuildPreset(),
			68);
		return !valid && planning.m_iContractVersion
			== HST_EnemyPlanningAuthorityService.QUARANTINE_CONTRACT_VERSION;
	}

	protected bool ProveBacklinkQuarantine()
	{
		HST_CampaignState state;
		HST_EnemyPlanningState planning;
		HST_EnemyOrderState order;
		if (!BuildCommittedFixture(state, planning, order))
			return false;
		order.m_iAttackCost++;
		HST_EnemyPlanningSaveValidationService validator
			= new HST_EnemyPlanningSaveValidationService();
		bool valid = validator.ValidateRestoredFactionRoles(
			state,
			BuildPreset(),
			68);
		return !valid
			&& planning.m_iContractVersion
				== HST_EnemyPlanningAuthorityService.QUARANTINE_CONTRACT_VERSION
			&& order.m_iPlanningContractVersion
				== HST_EnemyPlanningAuthorityService.QUARANTINE_CONTRACT_VERSION;
	}

	protected bool ProveDuplicateQuarantine()
	{
		HST_CampaignState state = BuildExactState(180, true, 0);
		state.m_aEnemyPlanningStates.Insert(
			m_Authority.BuildBaselineState(OCCUPIER_FACTION, 0));
		HST_EnemyPlanningSaveValidationService validator
			= new HST_EnemyPlanningSaveValidationService();
		bool valid = validator.ValidateRestoredFactionRoles(
			state,
			BuildPreset(),
			68);
		int quarantined;
		foreach (HST_EnemyPlanningState planning : state.m_aEnemyPlanningStates)
		{
			if (planning && planning.m_sFactionKey == OCCUPIER_FACTION
				&& planning.m_iContractVersion
					== HST_EnemyPlanningAuthorityService.QUARANTINE_CONTRACT_VERSION)
				quarantined++;
		}
		return !valid && quarantined == 2;
	}

	protected bool ProveMissingQuarantine()
	{
		HST_CampaignState state = BuildExactState(180, true, 0);
		for (int index = state.m_aEnemyPlanningStates.Count() - 1; index >= 0; index--)
		{
			HST_EnemyPlanningState planning = state.m_aEnemyPlanningStates[index];
			if (planning && planning.m_sFactionKey == OCCUPIER_FACTION)
				state.m_aEnemyPlanningStates.Remove(index);
		}
		HST_EnemyPlanningSaveValidationService validator
			= new HST_EnemyPlanningSaveValidationService();
		bool valid = validator.ValidateRestoredFactionRoles(
			state,
			BuildPreset(),
			68);
		HST_EnemyPlanningState placeholder
			= state.FindEnemyPlanningState(OCCUPIER_FACTION);
		return !valid && placeholder
			&& placeholder.m_iContractVersion
				== HST_EnemyPlanningAuthorityService.QUARANTINE_CONTRACT_VERSION;
	}

	protected bool ProveWrongRoleQuarantine()
	{
		HST_CampaignState state = BuildExactState(180, true, 0);
		HST_EnemyPlanningState wrong
			= m_Authority.BuildBaselineState(RESISTANCE_FACTION, 0);
		state.m_aEnemyPlanningStates.Insert(wrong);
		HST_EnemyPlanningSaveValidationService validator
			= new HST_EnemyPlanningSaveValidationService();
		bool valid = validator.ValidateRestoredFactionRoles(
			state,
			BuildPreset(),
			68);
		return !valid && wrong.m_iContractVersion
			== HST_EnemyPlanningAuthorityService.QUARANTINE_CONTRACT_VERSION;
	}

	protected bool BuildPreparedOrderFixture(
		out HST_CampaignState state,
		out HST_EnemyPlanningState planning,
		out HST_EnemyOrderState order)
	{
		state = BuildExactState(180, true, 0);
		planning = state.FindEnemyPlanningState(OCCUPIER_FACTION);
		HST_EnemyPlanningDecisionCommand command = BuildCommand(
			state,
			planning,
			"proof_us_target",
			"proof_us_source",
			1);
		HST_EnemyPlanningDecisionResult begun
			= m_Authority.BeginDecision(planning, command);
		if (!begun || !begun.m_bAccepted)
			return false;
		order = BuildOrderFromPlanning(planning);
		state.m_aEnemyOrders.Insert(order);
		HST_EnemyStrategicResourceService resources
			= new HST_EnemyStrategicResourceService();
		HST_EnemyStrategicMutationResult debit = resources.DebitResources(
			state,
			BuildPreset(),
			planning.m_sPlannedDebitMutationId,
			planning.m_sFactionKey,
			planning.m_iAttackCost,
			planning.m_iSupportCost,
			"proactive_attack_debit",
			order.m_sOrderId,
			order.m_sOrderId,
			order.m_sOperationId,
			order.m_sManifestId,
			order.m_sTargetZoneId);
		return debit && debit.m_bAccepted
			&& planning.m_sDisposition == "prepared";
	}

	protected bool BuildCommittedFixture(
		out HST_CampaignState state,
		out HST_EnemyPlanningState planning,
		out HST_EnemyOrderState order)
	{
		state = BuildExactState(180, true, 0);
		planning = state.FindEnemyPlanningState(OCCUPIER_FACTION);
		HST_EnemyPlanningDecisionCommand command = BuildCommand(
			state,
			planning,
			"proof_us_target",
			"proof_us_source",
			1);
		HST_EnemyPlanningDecisionResult begun
			= m_Authority.BeginDecision(planning, command);
		if (!begun || !begun.m_bAccepted)
			return false;
		order = BuildOrderFromPlanning(planning);
		state.m_aEnemyOrders.Insert(order);
		HST_EnemyStrategicResourceService resources
			= new HST_EnemyStrategicResourceService();
		HST_EnemyStrategicMutationResult debit = resources.DebitResources(
			state,
			BuildPreset(),
			planning.m_sPlannedDebitMutationId,
			planning.m_sFactionKey,
			planning.m_iAttackCost,
			planning.m_iSupportCost,
			"proactive_attack_debit",
			order.m_sOrderId,
			order.m_sOrderId,
			order.m_sOperationId,
			order.m_sManifestId,
			order.m_sTargetZoneId);
		HST_EnemyPlanningDecisionResult completed
			= m_Authority.CompleteDecision(
				planning,
				"committed",
				order,
				"");
		return debit && debit.m_bAccepted && completed
			&& completed.m_bAccepted
			&& planning.m_sDisposition == "committed";
	}

	protected bool ExactPlanningOrderBacklink(
		HST_EnemyPlanningState planning,
		HST_EnemyOrderState order)
	{
		if (!planning || !order)
			return false;
		bool authorityExact = order.m_iPlanningContractVersion
				== HST_EnemyPlanningAuthorityService.CONTRACT_VERSION
			&& order.m_iPlanningDecisionSequence
				== planning.m_iDecisionSequence
			&& order.m_iPlanningBucketSecond
				== planning.m_iDecisionBucketSecond
			&& order.m_sPlanningDecisionId == planning.m_sDecisionId
			&& order.m_sPlanningInputFingerprint
				== planning.m_sInputFingerprint
			&& order.m_sPlanningDecisionFingerprint
				== planning.m_sDecisionFingerprint;
		bool identityExact = order.m_sFactionKey == planning.m_sFactionKey
			&& order.m_sOrderId == planning.m_sPlannedOrderId
			&& order.m_sOperationId == planning.m_sPlannedOperationId
			&& order.m_sManifestId == planning.m_sPlannedManifestId
			&& order.m_sManifestHash == planning.m_sPlannedManifestHash;
		bool selectionExact = order.m_sSourceZoneId
				== planning.m_sSelectedSourceZoneId
			&& order.m_sTargetZoneId == planning.m_sSelectedTargetZoneId
			&& order.m_eType == planning.m_eSelectedOrderType
			&& order.m_ePlannedSupportType
				== planning.m_ePlannedSupportType
			&& order.m_sPlanningCapabilityHash
				== planning.m_sPlanningCapabilityHash;
		bool accountingExact = order.m_iAttackCost == planning.m_iAttackCost
			&& order.m_iSupportCost == planning.m_iSupportCost
			&& order.m_sResourceDebitMutationId
				== planning.m_sPlannedDebitMutationId;
		return authorityExact && identityExact && selectionExact
			&& accountingExact;
	}

	protected HST_EnemyPlanningDecisionCommand BuildCommand(
		HST_CampaignState state,
		HST_EnemyPlanningState planning,
		string targetZoneId,
		string sourceZoneId,
		int targetCandidateCount)
	{
		HST_EnemyPlanningDecisionCommand command
			= new HST_EnemyPlanningDecisionCommand();
		command.m_sFactionKey = planning.m_sFactionKey;
		command.m_iExpectedDecisionSequence
			= planning.m_iDecisionSequence + 1;
		command.m_iExpectedNextPlanningBucketSecond
			= planning.m_iNextPlanningBucketSecond;
		command.m_iDecisionBucketSecond
			= planning.m_iNextPlanningBucketSecond;
		HST_FactionPoolState pool = state.FindFactionPool(planning.m_sFactionKey);
		command.m_iObservedWarLevel = state.m_iWarLevel;
		command.m_iObservedAggression = pool.m_iAggression;
		command.m_iObservedPoolRevision = pool.m_iStrategicRevision;
		command.m_iObservedOperationalMutationCount
			= pool.m_iStrategicOperationalMutationCount;
		command.m_iObservedAttackResources = pool.m_iAttackResources;
		command.m_iObservedSupportResources = pool.m_iSupportResources;
		command.m_sCommitmentFingerprint
			= HST_EnemyPlanningAuthorityService.BuildCommitmentFingerprint(
				state,
				planning.m_sFactionKey,
				command.m_iCommitmentCount);
		command.m_iTargetCandidateCount = targetCandidateCount;
		command.m_sTargetCandidateFingerprint = string.Format(
			"proof_targets_%1_%2",
			planning.m_sFactionKey,
			targetCandidateCount);
		if (!sourceZoneId.IsEmpty())
			command.m_iSourceCandidateCount = 1;
		command.m_sSourceCandidateFingerprint = string.Format(
			"proof_sources_%1_%2",
			planning.m_sFactionKey,
			command.m_iSourceCandidateCount);
		command.m_sSelectedTargetZoneId = targetZoneId;
		command.m_sSelectedSourceZoneId = sourceZoneId;
		command.m_eSelectedOrderType
			= HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL;
		command.m_ePlannedSupportType
			= HST_ESupportRequestType.HST_SUPPORT_PATROL_SWEEP;
		command.m_sPlanningCapabilityHash
			= HST_EnemyPlanningAuthorityService.BuildCapabilityHash(
				command.m_eSelectedOrderType,
				command.m_ePlannedSupportType,
				0,
				"",
				"");
		command.m_sSpendMode = "proactive_attack";
		if (targetCandidateCount > 0)
			command.m_iAttackCost = 8;
		command.m_iTargetPressureBefore = 0;
		command.m_iTargetPressureDelta = 0;
		command.m_iTargetPressureAfter = 0;
		command.m_sDecisionId
			= HST_EnemyPlanningAuthorityService.BuildDecisionId(
				command.m_sFactionKey,
				command.m_iExpectedDecisionSequence,
				command.m_iDecisionBucketSecond);
		command.m_sPlannedOrderId
			= HST_EnemyPlanningAuthorityService.BuildOrderId(
				command.m_sDecisionId);
		command.m_sPlannedOperationId
			= HST_EnemyPlanningAuthorityService.BuildOperationId(
				command.m_sPlannedOrderId);
		command.m_sPlannedDebitMutationId
			= HST_EnemyPlanningAuthorityService.BuildDebitMutationId(
				command.m_sPlannedOrderId);
		return command;
	}

	protected HST_EnemyOrderState BuildOrderFromPlanning(
		HST_EnemyPlanningState planning)
	{
		HST_EnemyOrderState order = new HST_EnemyOrderState();
		order.m_sOrderId = planning.m_sPlannedOrderId;
		order.m_sOperationId = planning.m_sPlannedOperationId;
		order.m_iOperationContractVersion = 0;
		order.m_sManifestId = planning.m_sPlannedManifestId;
		order.m_sManifestHash = planning.m_sPlannedManifestHash;
		order.m_sFactionKey = planning.m_sFactionKey;
		order.m_eType = planning.m_eSelectedOrderType;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
		order.m_sSourceZoneId = planning.m_sSelectedSourceZoneId;
		order.m_sTargetZoneId = planning.m_sSelectedTargetZoneId;
		order.m_iCreatedAtSecond = planning.m_iDecisionBucketSecond;
		order.m_iAttackCost = planning.m_iAttackCost;
		order.m_iSupportCost = planning.m_iSupportCost;
		order.m_sResourceDebitMutationId = planning.m_sPlannedDebitMutationId;
		order.m_iPlanningContractVersion
			= HST_EnemyPlanningAuthorityService.CONTRACT_VERSION;
		order.m_iPlanningDecisionSequence = planning.m_iDecisionSequence;
		order.m_iPlanningBucketSecond = planning.m_iDecisionBucketSecond;
		order.m_sPlanningDecisionId = planning.m_sDecisionId;
		order.m_sPlanningInputFingerprint = planning.m_sInputFingerprint;
		order.m_sPlanningDecisionFingerprint
			= planning.m_sDecisionFingerprint;
		order.m_ePlannedSupportType = planning.m_ePlannedSupportType;
		order.m_sPlanningCapabilityHash = planning.m_sPlanningCapabilityHash;
		return order;
	}

	protected HST_CampaignState BuildExactState(
		int elapsedSecond,
		bool includePlanning,
		int baselineSecond = -1)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = 68;
		state.m_iLastLoadedSchemaVersion = 68;
		state.m_iElapsedSeconds = Math.Max(0, elapsedSecond);
		state.m_iWarLevel = 3;
		state.m_aFactionPools.Insert(BuildPool(OCCUPIER_FACTION, state.m_iElapsedSeconds));
		state.m_aFactionPools.Insert(BuildPool(INVADER_FACTION, state.m_iElapsedSeconds));
		AddZone(state, "proof_us_target", OCCUPIER_FACTION, "1000 0 1000", 30);
		AddZone(state, "proof_us_source", OCCUPIER_FACTION, "800 0 800", 20);
		AddZone(state, "proof_ussr_target", INVADER_FACTION, "3000 0 3000", 30);
		AddZone(state, "proof_ussr_source", INVADER_FACTION, "2800 0 2800", 20);
		if (includePlanning)
		{
			int resolvedBaseline = baselineSecond;
			if (resolvedBaseline < 0)
				resolvedBaseline = state.m_iElapsedSeconds;
			state.m_aEnemyPlanningStates.Insert(
				m_Authority.BuildBaselineState(OCCUPIER_FACTION, resolvedBaseline));
			state.m_aEnemyPlanningStates.Insert(
				m_Authority.BuildBaselineState(INVADER_FACTION, resolvedBaseline));
		}
		return state;
	}

	protected HST_FactionPoolState BuildPool(string factionKey, int elapsedSecond)
	{
		HST_FactionPoolState pool = new HST_FactionPoolState();
		pool.m_sFactionKey = factionKey;
		pool.m_iAttackResources = 120;
		pool.m_iSupportResources = 80;
		pool.m_iAggression = 10;
		pool.m_iStrategicContractVersion
			= HST_EnemyStrategicResourceService.CONTRACT_VERSION;
		pool.m_iStrategicRevision = 1;
		pool.m_iLastResourceBucketSecond = Math.Max(0, elapsedSecond);
		pool.m_iLastAggressionBucketSecond = Math.Max(0, elapsedSecond);
		return pool;
	}

	protected bool FreshPoolExact(
		HST_FactionPoolState pool,
		int expectedAttack,
		int expectedSupport)
	{
		return pool
			&& pool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& pool.m_iStrategicRevision == 1
			&& pool.m_iStrategicOperationalMutationCount == 0
			&& pool.m_iAttackResources == expectedAttack
			&& pool.m_iSupportResources == expectedSupport
			&& pool.m_iAggression == 0
			&& pool.m_iResourceAccumulatorSeconds == 0
			&& pool.m_iAggressionAccumulatorSeconds == 0
			&& pool.m_iLastResourceBucketSecond == 0
			&& pool.m_iLastAggressionBucketSecond == 0
			&& pool.m_sLastStrategicMutationId.IsEmpty()
			&& pool.m_sStrategicAuthorityFailure.IsEmpty();
	}

	protected void AddZone(
		HST_CampaignState state,
		string zoneId,
		string ownerFactionKey,
		vector position,
		int priority)
	{
		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = zoneId;
		zone.m_sDisplayName = zoneId;
		zone.m_sOwnerFactionKey = ownerFactionKey;
		zone.m_eType = HST_EZoneType.HST_ZONE_TOWN;
		zone.m_vPosition = position;
		zone.m_iPriority = priority;
		state.m_aZones.Insert(zone);
	}

	protected bool BaselineExact(HST_EnemyPlanningState planning, int elapsedSecond)
	{
		return planning
			&& planning.m_iContractVersion
				== HST_EnemyPlanningAuthorityService.CONTRACT_VERSION
			&& planning.m_iRevision == 1
			&& planning.m_iDecisionSequence == 0
			&& planning.m_iLastPlanningBucketSecond == elapsedSecond
			&& planning.m_iNextPlanningBucketSecond
				== elapsedSecond
					+ HST_EnemyPlanningAuthorityService.PLANNING_INTERVAL_SECONDS
			&& planning.m_sDisposition == "idle"
			&& planning.m_sDecisionId.IsEmpty()
			&& planning.m_sInputFingerprint.IsEmpty()
			&& planning.m_sDecisionFingerprint.IsEmpty();
	}

	protected HST_EnemyPlanningState CopyPlanningThroughSave(
		HST_CampaignState state,
		string factionKey)
	{
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(state);
		HST_CampaignState copied = save.Restore();
		if (!copied)
			return null;
		return copied.FindEnemyPlanningState(factionKey);
	}

	protected bool SameFrozenDecision(
		HST_EnemyPlanningState expected,
		HST_EnemyPlanningState actual)
	{
		if (!expected || !actual)
			return false;
		if (expected.m_iDecisionSequence != actual.m_iDecisionSequence
			|| expected.m_iDecisionBucketSecond != actual.m_iDecisionBucketSecond
			|| expected.m_iObservedWarLevel != actual.m_iObservedWarLevel
			|| expected.m_iObservedAggression != actual.m_iObservedAggression
			|| expected.m_iObservedPoolRevision != actual.m_iObservedPoolRevision
			|| expected.m_iObservedOperationalMutationCount
				!= actual.m_iObservedOperationalMutationCount
			|| expected.m_iObservedAttackResources
				!= actual.m_iObservedAttackResources
			|| expected.m_iObservedSupportResources
				!= actual.m_iObservedSupportResources)
			return false;
		if (expected.m_iCommitmentCount != actual.m_iCommitmentCount
			|| expected.m_sCommitmentFingerprint
				!= actual.m_sCommitmentFingerprint
			|| expected.m_iTargetCandidateCount
				!= actual.m_iTargetCandidateCount
			|| expected.m_sTargetCandidateFingerprint
				!= actual.m_sTargetCandidateFingerprint
			|| expected.m_iSourceCandidateCount
				!= actual.m_iSourceCandidateCount
			|| expected.m_sSourceCandidateFingerprint
				!= actual.m_sSourceCandidateFingerprint)
			return false;
		if (expected.m_sSelectedTargetZoneId != actual.m_sSelectedTargetZoneId
			|| expected.m_sSelectedSourceZoneId != actual.m_sSelectedSourceZoneId
			|| expected.m_eSelectedOrderType != actual.m_eSelectedOrderType
			|| expected.m_ePlannedSupportType != actual.m_ePlannedSupportType
			|| expected.m_sPlanningCapabilityHash
				!= actual.m_sPlanningCapabilityHash
			|| expected.m_sSpendMode != actual.m_sSpendMode
			|| expected.m_iAttackCost != actual.m_iAttackCost
			|| expected.m_iSupportCost != actual.m_iSupportCost)
			return false;
		if (expected.m_iTargetPressureBefore != actual.m_iTargetPressureBefore
			|| expected.m_iTargetPressureDelta != actual.m_iTargetPressureDelta
			|| expected.m_iTargetPressureAfter != actual.m_iTargetPressureAfter
			|| expected.m_bTargetPressureApplied
				!= actual.m_bTargetPressureApplied)
			return false;
		return expected.m_sDecisionId == actual.m_sDecisionId
			&& expected.m_sPlannedOrderId == actual.m_sPlannedOrderId
			&& expected.m_sPlannedOperationId == actual.m_sPlannedOperationId
			&& expected.m_sPlannedManifestId == actual.m_sPlannedManifestId
			&& expected.m_sPlannedManifestHash == actual.m_sPlannedManifestHash
			&& expected.m_sPlannedDebitMutationId
				== actual.m_sPlannedDebitMutationId
			&& expected.m_sInputFingerprint == actual.m_sInputFingerprint
			&& expected.m_sDecisionFingerprint
				== actual.m_sDecisionFingerprint;
	}

	protected void AddCommitmentFixtures(HST_CampaignState state)
	{
		state.m_aEnemyOrders.Insert(BuildCommitmentOrder("proof_order_a", 0));
		state.m_aEnemyOrders.Insert(BuildCommitmentOrder("proof_order_b", 1));
		state.m_aSupportRequests.Insert(BuildCommitmentSupport("proof_support_a", 0));
		state.m_aSupportRequests.Insert(BuildCommitmentSupport("proof_support_b", 1));
		state.m_aOperations.Insert(BuildCommitmentOperation("proof_operation_a", 0));
		state.m_aOperations.Insert(BuildCommitmentOperation("proof_operation_b", 1));
	}

	protected HST_EnemyOrderState BuildCommitmentOrder(string orderId, int ordinal)
	{
		HST_EnemyOrderState order = new HST_EnemyOrderState();
		order.m_sOrderId = orderId;
		order.m_sOperationId = "operation_" + orderId;
		order.m_sFactionKey = OCCUPIER_FACTION;
		order.m_eType = HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
		order.m_sSourceZoneId = "proof_us_source";
		order.m_sTargetZoneId = "proof_us_target";
		order.m_iAttackCost = 5 + ordinal;
		return order;
	}

	protected HST_SupportRequestState BuildCommitmentSupport(
		string requestId,
		int ordinal)
	{
		HST_SupportRequestState request = new HST_SupportRequestState();
		request.m_sRequestId = requestId;
		request.m_sOperationId = "operation_" + requestId;
		request.m_sFactionKey = OCCUPIER_FACTION;
		request.m_eType = HST_ESupportRequestType.HST_SUPPORT_PATROL_SWEEP;
		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE;
		request.m_sSourceZoneId = "proof_us_source";
		request.m_sTargetZoneId = "proof_us_target";
		request.m_sGroupId = string.Format("proof_group_%1", ordinal);
		return request;
	}

	protected HST_OperationRecordState BuildCommitmentOperation(
		string operationId,
		int ordinal)
	{
		HST_OperationRecordState operation = new HST_OperationRecordState();
		operation.m_sOperationId = operationId;
		operation.m_iContractVersion = 1;
		operation.m_sOwnerFactionKey = OCCUPIER_FACTION;
		operation.m_sEnemyOrderId = string.Format("proof_link_%1", ordinal);
		operation.m_sOriginZoneId = "proof_us_source";
		operation.m_sAssignmentZoneId = "proof_us_target";
		operation.m_eSettlementState
			= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		operation.m_eTerminalResult
			= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE;
		return operation;
	}

	protected void ReverseCommitmentArrays(HST_CampaignState state)
	{
		HST_EnemyOrderState order = state.m_aEnemyOrders[0];
		state.m_aEnemyOrders[0] = state.m_aEnemyOrders[1];
		state.m_aEnemyOrders[1] = order;
		HST_SupportRequestState request = state.m_aSupportRequests[0];
		state.m_aSupportRequests[0] = state.m_aSupportRequests[1];
		state.m_aSupportRequests[1] = request;
		HST_OperationRecordState operation = state.m_aOperations[0];
		state.m_aOperations[0] = state.m_aOperations[1];
		state.m_aOperations[1] = operation;
	}

	protected HST_EnemyOrderState FindOrderForDecision(
		HST_CampaignState state,
		string decisionId)
	{
		if (!state || decisionId.IsEmpty())
			return null;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (order && order.m_sPlanningDecisionId == decisionId)
				return order;
		}
		return null;
	}

	protected HST_CampaignPreset BuildPreset()
	{
		return HST_DefaultCatalog.CreateVanillaEveronPreset();
	}
}
