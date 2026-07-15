class HST_EnemyQRFOperationProofReport
{
	bool m_bAdmissionExact;
	bool m_bLegacyIsolationExact;
	bool m_bProjectionExact;
	bool m_bSettlementExact;
	bool m_bRestoreExact;
	bool m_bRejectionExact;
	string m_sAdmissionEvidence;
	string m_sLegacyIsolationEvidence;
	string m_sProjectionEvidence;
	string m_sSettlementEvidence;
	string m_sRestoreEvidence;
	string m_sRejectionEvidence;
}

class HST_EnemyQRFOperationProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_EnemyDirectorService m_EnemyDirector;
	ref HST_ForcePlanningService m_Planning;
	ref HST_ForceSpawnQueueService m_Queue;
	ref HST_ForceSpawnAdapterService m_Adapter;
	ref HST_PhysicalWarService m_PhysicalWar;
	ref HST_EnemyQRFOperationService m_ExactQRF;
	ref HST_EnemyOrderState m_Order;
	ref HST_ForceManifestState m_Manifest;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
	ref HST_OperationRecordState m_Operation;
	ref HST_EnemyQRFAdmissionResult m_Admission;
	int m_iAttackBeforeDebit;
	int m_iSupportBeforeDebit;
	int m_iAttackAfterDebit;
	int m_iSupportAfterDebit;
	int m_iAttackAfterAdmission;
	int m_iSupportAfterAdmission;
	bool m_bDebitAccepted;
	string m_sDebitReason;
}

class HST_EnemyQRFRestoreCorruptionProofSummary
{
	bool m_bPartialReceiptQuarantined;
	bool m_bMissingBacklinkRejected;
	string m_sEvidence;
}

class HST_EnemyQRFReplayAmbiguityProofSummary
{
	bool m_bMissingCanonicalRejected;
	bool m_bShadowRejected;
	string m_sEvidence;
}

class HST_EnemyQRFPreparedRecoveryProofSummary
{
	bool m_bPreRefundExact;
	bool m_bPostRefundExact;
	bool m_bPostReceiptExact;
	bool m_bUncommittedFullRefundExact;
	bool m_bCorruptedBaselineFailClosed;
	bool m_bCorruptedPoolTailFailClosed;
	bool m_bTamperedSurvivorTupleFailClosed;
	bool m_bSettledPoolTailFailClosed;
	bool m_bHistoricalMutationlessPreserved;
	string m_sEvidence;
}

class HST_EnemyQRFPreparedRecoveryBoundaryProofSummary
{
	bool m_bCorruptedBaselineFailClosed;
	bool m_bCorruptedPoolTailFailClosed;
	bool m_bTamperedSurvivorTupleFailClosed;
	bool m_bSettledPoolTailFailClosed;
	bool m_bHistoricalMutationlessPreserved;
	string m_sEvidence;
}

class HST_EnemyQRFPreparedRecoveryCutContext
{
	ref HST_EnemyQRFOperationProofFixture m_Fixture;
	ref HST_EnemyQRFPreparedRecoveryExpectation m_Expectation;
	int m_iRestartCut;
	int m_iAccepted;
	int m_iCasualties;
	int m_iSurvivors;
	int m_iAttackRefund;
	int m_iSupportRefund;
	int m_iAttackBeforeRefund;
	int m_iSupportBeforeRefund;
	int m_iLedgerAttackSpentBeforeRefund;
	int m_iLedgerSupportSpentBeforeRefund;
	int m_iLedgerAttackRefundedBefore;
	int m_iLedgerSupportRefundedBefore;
	int m_iPreparedAtSecond;
	int m_iPrefixRevision;
	int m_iExpectedPrefixMutationCount;
	int m_iExpectedPrefixAttackDelta;
	int m_iExpectedPrefixSupportDelta;
	bool m_bExpectedPrefixReceiptApplied;
	bool m_bPrefixExactBeforeSave;
	bool m_bRetainDeterministicResiduals;
	string m_sSettlementKind;
	string m_sSettlementId;
	string m_sRefundMutationId;
	string m_sReason;
	string m_sFailure;
}

class HST_EnemyQRFPreparedRecoveryRunResult
{
	bool m_bPrefixSurvived;
	bool m_bFirstChanged;
	bool m_bSecondChanged;
	bool m_bFirstResumeExact;
	bool m_bSameStateReplayExact;
	bool m_bSecondRestoreExact;
	bool m_bSecondReceiptApplied;
	bool m_bSecondOperationPresent;
	int m_iSecondMutationCount = -1;
	HST_EOperationSettlementState m_eSecondSettlementState
		= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_UNKNOWN;

	bool AllExact()
	{
		if (!m_bPrefixSurvived)
			return false;
		if (!m_bFirstResumeExact)
			return false;
		if (!m_bSameStateReplayExact)
			return false;
		if (!m_bSecondRestoreExact)
			return false;
		return true;
	}
}

class HST_EnemyQRFHistoricalMutationlessContext
{
	ref HST_CampaignSaveData m_LegacySave;
	string m_sOrderId;
	string m_sOperationId;
	string m_sManifestId;
	string m_sManifestHash;
	string m_sSettlementId;
	string m_sSettlementKind;
	string m_sTerminalReason;
	int m_iAccepted;
	int m_iSurvivors;
	int m_iAttackRefund;
	int m_iSupportRefund;
	int m_iResolvedAtSecond;
	int m_iSettledAtSecond;
	int m_iOperationRevision;
	int m_iAttackPool;
	int m_iSupportPool;
	string m_sFailure;
}

class HST_EnemyQRFOperationProofService
{
	static const string PROOF_FACTION_KEY = "US";
	static const string PROOF_SOURCE_ZONE_ID = "enemy_qrf_proof_source";
	static const string PROOF_TARGET_ZONE_ID = "enemy_qrf_proof_target";
	static const int PROOF_ATTACK_COST = 12;
	static const int PROOF_SUPPORT_COST = 8;
	static const int PREPARED_CUT_BEFORE_REFUND = 0;
	static const int PREPARED_CUT_AFTER_REFUND = 1;
	static const int PREPARED_CUT_AFTER_RECEIPT = 2;

	bool PrepareExternalRestartCarrier(
		string runId,
		string world,
		string cutName,
		out HST_CampaignState stagedState,
		out HST_EnemyQRFExternalRestartCarrier carrier,
		out string evidence)
	{
		stagedState = null;
		carrier = null;
		evidence = "external exact-QRF preparation rejected";
		if (!HST_EnemyQRFExternalRestartProofService.ValidateRunId(runId)
			|| world.IsEmpty())
			return false;
		int restartCut
			= HST_EnemyQRFExternalRestartProofService.ResolveCut(cutName);
		if (restartCut < PREPARED_CUT_BEFORE_REFUND
			|| restartCut > PREPARED_CUT_AFTER_RECEIPT)
			return false;

		HST_EnemyQRFPreparedRecoveryCutContext context
			= BuildPreparedRecoveryCutContext(
				restartCut,
				PROOF_ATTACK_COST,
				PROOF_SUPPORT_COST);
		if (!context || !context.m_sFailure.IsEmpty()
			|| !context.m_bPrefixExactBeforeSave || !context.m_Expectation)
		{
			if (context && !context.m_sFailure.IsEmpty())
				evidence = context.m_sFailure;
			return false;
		}

		carrier = BuildExternalRestartCarrier(
			runId,
			world,
			cutName,
			context);
		if (!carrier)
			return false;
		stagedState = context.m_Fixture.m_State;
		string preparedFingerprint;
		string preparedEvidence;
		bool exact = ValidateExternalPreparedState(
			stagedState,
			carrier,
			preparedFingerprint,
			preparedEvidence);
		if (!exact)
		{
			stagedState = null;
			carrier = null;
			evidence = preparedEvidence;
			return false;
		}
		evidence = string.Format(
			"external exact-QRF prepared cut %1 | roster %2/%3 | refund %4/%5",
			cutName,
			context.m_iAccepted,
			context.m_iSurvivors,
			context.m_iAttackRefund,
			context.m_iSupportRefund);
		return true;
	}

	bool ValidateExternalPreparedState(
		HST_CampaignState state,
		HST_EnemyQRFExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "prepared-recovery-state-unavailable";
		evidence = "external exact-QRF prepared state rejected";
		string carrierEvidence;
		if (!state || !carrier
			|| !HST_EnemyQRFExternalRestartProofService.ValidateCarrier(
				carrier,
				carrier.m_sRunId,
				carrier.m_sCutName,
				carrierEvidence))
			return false;
		if (state.m_iSchemaVersion != carrier.m_iCampaignSchemaVersion)
			return false;

		HST_EnemyQRFPreparedRecoveryCutContext context
			= BuildExternalRestartCutContext(carrier);
		HST_EnemyOrderState order = FindOrder(
			state,
			carrier.m_Expectation.m_sOrderId);
		HST_OperationRecordState operation = state.FindOperation(
			carrier.m_Expectation.m_sOperationId);
		HST_FactionPoolState pool = state.FindFactionPool(PROOF_FACTION_KEY);
		HST_EnemySupportLedgerState ledger = state.FindEnemySupportLedger(
			PROOF_FACTION_KEY,
			PROOF_TARGET_ZONE_ID);
		fingerprint = BuildPreparedRecoveryPrefixFingerprint(
			state,
			carrier.m_Expectation,
			context);
		if (!IsPreparedRecoveryPrefixExact(
			state,
			order,
			operation,
			pool,
			ledger,
			context))
			return false;
		HST_CampaignSaveData snapshot = new HST_CampaignSaveData();
		snapshot.Capture(state);
		string authorityFailure
			= HST_EnemyQRFSaveValidationService.ValidatePreparedSaveAuthority(
				snapshot,
				order);
		if (!authorityFailure.IsEmpty())
		{
			evidence = authorityFailure;
			return false;
		}
		if (fingerprint != carrier.m_sPreparedFingerprint)
		{
			evidence = "external exact-QRF prepared fingerprint mismatch";
			return false;
		}
		evidence = "external exact-QRF prepared state exact";
		return true;
	}

	bool ValidateExternalTerminalState(
		HST_CampaignState state,
		HST_EnemyQRFExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "prepared-recovery-state-unavailable";
		evidence = "external exact-QRF terminal state rejected";
		string carrierEvidence;
		if (!state || !carrier
			|| !HST_EnemyQRFExternalRestartProofService.ValidateCarrier(
				carrier,
				carrier.m_sRunId,
				carrier.m_sCutName,
				carrierEvidence))
			return false;
		if (state.m_iSchemaVersion != carrier.m_iCampaignSchemaVersion)
			return false;

		HST_EnemyOrderState order = FindOrder(
			state,
			carrier.m_Expectation.m_sOrderId);
		fingerprint = BuildPreparedRecoveryTerminalFingerprint(
			state,
			carrier.m_Expectation);
		if (!IsPreparedRecoveryTerminalExact(state, carrier.m_Expectation))
			return false;
		HST_CampaignSaveData snapshot = new HST_CampaignSaveData();
		snapshot.Capture(state);
		string authorityFailure
			= HST_EnemyQRFSaveValidationService.ValidateSettledSaveAuthority(
				snapshot,
				order);
		if (!authorityFailure.IsEmpty())
		{
			evidence = authorityFailure;
			return false;
		}
		evidence = "external exact-QRF terminal state exact";
		return true;
	}

	HST_EnemyQRFOperationProofReport Run()
	{
		HST_EnemyQRFOperationProofReport report = new HST_EnemyQRFOperationProofReport();
		ProveAdmission(report);
		ProveLegacyIsolation(report);
		ProveProjection(report);
		ProveSettlement(report);
		ProveRestore(report);
		ProveRejection(report);
		return report;
	}

	protected void ProveAdmission(HST_EnemyQRFOperationProofReport report)
	{
		HST_EnemyQRFOperationProofFixture fixture = BuildAdmittedFixture("admission");
		if (!Ready(fixture))
		{
			report.m_sAdmissionEvidence = BuildFixtureFailure(fixture);
			return;
		}

		fixture.m_State.m_iElapsedSeconds += 37;
		fixture.m_State.m_iWarLevel = 5;
		HST_EnemyDefensiveQRFManifestResult replay = fixture.m_Planning.PlanExactEnemyDefensiveQRF(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Order);
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		int attackBeforeReplay = pool.m_iAttackResources;
		int supportBeforeReplay = pool.m_iSupportResources;
		HST_EnemyQRFAdmissionResult replayAdmission;
		if (replay && replay.m_bSuccess && replay.m_Manifest)
		{
			replayAdmission = fixture.m_ExactQRF.AdmitPreparedOrder(
				fixture.m_State,
				fixture.m_Order,
				replay.m_Manifest,
				fixture.m_EnemyDirector);
		}
		HST_EnemySupportLedgerState ledger = fixture.m_State.FindEnemySupportLedger(PROOF_FACTION_KEY, PROOF_TARGET_ZONE_ID);
		HST_StrategicMovementService movement = new HST_StrategicMovementService();
		bool deterministic = replay && replay.m_bSuccess && replay.m_Manifest
			&& replay.m_Manifest.m_sManifestHash == fixture.m_Manifest.m_sManifestHash
			&& replay.m_Manifest.m_iAcceptedMemberCount == fixture.m_Manifest.m_iAcceptedMemberCount;
		bool frozenInfantry = fixture.m_Manifest.m_bFrozen
			&& fixture.m_Manifest.m_aGroups.Count() == 1
			&& fixture.m_Manifest.m_aMembers.Count() == fixture.m_Manifest.m_iAcceptedMemberCount
			&& fixture.m_Manifest.m_aVehicles.Count() == 0
			&& movement.IsSupportedExactInfantryManifest(fixture.m_Manifest);
		bool debitExact = fixture.m_bDebitAccepted
			&& fixture.m_iAttackAfterDebit == fixture.m_iAttackBeforeDebit - fixture.m_Order.m_iAttackCost
			&& fixture.m_iSupportAfterDebit == fixture.m_iSupportBeforeDebit - fixture.m_Order.m_iSupportCost
			&& fixture.m_iAttackAfterAdmission == fixture.m_iAttackAfterDebit
			&& fixture.m_iSupportAfterAdmission == fixture.m_iSupportAfterDebit
			&& replayAdmission && replayAdmission.m_bSuccess
			&& pool.m_iAttackResources == attackBeforeReplay
			&& pool.m_iSupportResources == supportBeforeReplay;
		bool ledgerExact = ledger
			&& ledger.m_iAttackSpent == fixture.m_Order.m_iAttackCost
			&& ledger.m_iSupportSpent == fixture.m_Order.m_iSupportCost;
		bool rowsExact = CountManifestId(fixture.m_State, fixture.m_Manifest.m_sManifestId) == 1
			&& CountOperationId(fixture.m_State, fixture.m_Operation.m_sOperationId) == 1
			&& CountBatchId(fixture.m_State, fixture.m_Batch.m_sResultId) == 1
			&& CountGroupId(fixture.m_State, fixture.m_Group.m_sGroupId) == 1;
		bool linksExact = fixture.m_Order.m_sManifestId == fixture.m_Manifest.m_sManifestId
			&& fixture.m_Order.m_sOperationId == fixture.m_Operation.m_sOperationId
			&& fixture.m_Order.m_sSpawnResultId == fixture.m_Batch.m_sResultId
			&& fixture.m_Order.m_sGroupId == fixture.m_Group.m_sGroupId
			&& fixture.m_Batch.m_bStrategicProjectionHeld
			&& fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;

		report.m_bAdmissionExact = deterministic && frozenInfantry && debitExact && ledgerExact && rowsExact && linksExact;
		report.m_sAdmissionEvidence = string.Format(
			"debit %1/%2 -> %3/%4 -> %5/%6 | manifest %7 members %8",
			fixture.m_iAttackBeforeDebit,
			fixture.m_iSupportBeforeDebit,
			fixture.m_iAttackAfterDebit,
			fixture.m_iSupportAfterDebit,
			pool && pool.m_iAttackResources,
			pool && pool.m_iSupportResources,
			fixture.m_Manifest.m_sManifestHash,
			fixture.m_Manifest.m_iAcceptedMemberCount);
		report.m_sAdmissionEvidence = report.m_sAdmissionEvidence + string.Format(
			" | rows manifest/operation/batch/group %1/%2/%3/%4 | held %5 | replay accepted %6",
			CountManifestId(fixture.m_State, fixture.m_Manifest.m_sManifestId),
			CountOperationId(fixture.m_State, fixture.m_Operation.m_sOperationId),
			CountBatchId(fixture.m_State, fixture.m_Batch.m_sResultId),
			CountGroupId(fixture.m_State, fixture.m_Group.m_sGroupId),
			fixture.m_Batch.m_bStrategicProjectionHeld,
			replayAdmission && replayAdmission.m_bSuccess);
	}

	protected void ProveLegacyIsolation(HST_EnemyQRFOperationProofReport report)
	{
		HST_EnemyQRFOperationProofFixture fixture = BuildAdmittedFixture("isolation");
		if (!Ready(fixture))
		{
			report.m_sLegacyIsolationEvidence = BuildFixtureFailure(fixture);
			return;
		}

		bool noLegacyRows = fixture.m_State.m_aSupportRequests.Count() == 0
			&& fixture.m_State.m_aQRFs.Count() == 0
			&& fixture.m_Order.m_sSupportRequestId.IsEmpty();
		bool exactOwnsTarget = fixture.m_ExactQRF.HasOpenExactEnemyDefensiveQRF(
			fixture.m_State,
			fixture.m_Order.m_sFactionKey,
			fixture.m_Order.m_sTargetZoneId);
		bool exactOnly = fixture.m_Order.m_iOperationContractVersion == HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION
			&& fixture.m_Operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF
			&& fixture.m_Manifest.m_sForceKind == HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_FORCE_KIND;
		report.m_bLegacyIsolationExact = noLegacyRows && exactOwnsTarget && exactOnly;
		report.m_sLegacyIsolationEvidence = string.Format(
			"support/QRF rows %1/%2 | support link '%3' | exact target owner %4",
			fixture.m_State.m_aSupportRequests.Count(),
			fixture.m_State.m_aQRFs.Count(),
			fixture.m_Order.m_sSupportRequestId,
			exactOwnsTarget);
	}

	protected void ProveProjection(HST_EnemyQRFOperationProofReport report)
	{
		HST_EnemyQRFOperationProofFixture fixture = BuildAdmittedFixture("projection");
		if (!Ready(fixture))
		{
			report.m_sProjectionEvidence = BuildFixtureFailure(fixture);
			return;
		}

		HST_StrategicMovementService movement = new HST_StrategicMovementService();
		HST_MaterializationService materialization = new HST_MaterializationService();
		HST_OperationService operations = new HST_OperationService();
		int livingBefore = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		int attackBefore = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY).m_iAttackResources;
		int supportBefore = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY).m_iSupportResources;

		fixture.m_State.m_iElapsedSeconds += 100;
		bool routeValid = movement.InitializeExactInfantryQRFRoute(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Manifest,
			fixture.m_Group);
		HST_StrategicMovementResult advanced = movement.AdvanceExactPlayerQRF(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Group);
		float expectedAdvance = HST_StrategicMovementService.EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND
			* HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
		bool movementExact = routeValid && advanced && advanced.m_bAccepted
			&& advanced.m_iAdvancedSeconds == HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK
			&& Math.AbsFloat(advanced.m_fAdvancedMeters - expectedAdvance) < 0.1;

		HST_OperationProjectionDecision enter = materialization.EvaluateExactPlayerQRFForProximity(
			fixture.m_Operation,
			true,
			true,
			1800.0,
			2160.0);
		HST_ForceSpawnQueueCallbackResult released = fixture.m_Queue.ReleaseStrategicProjectionForMaterialization(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			fixture.m_State.m_iElapsedSeconds,
			fixture.m_State.m_iElapsedSeconds + 180);
		HST_OperationTransitionResult materializing = operations.MarkExactEnemyDefensiveQRFMaterializingFromVirtual(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Group,
			fixture.m_Batch,
			"enemy QRF proof materialize");
		ApplySyntheticSuccessfulProjection(fixture);
		fixture.m_Group.m_bSpawnedEntity = true;
		fixture.m_Group.m_iSpawnedAgentCount = livingBefore;
		HST_OperationTransitionResult physical = operations.MarkExactEnemyDefensiveQRFPhysical(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Group,
			fixture.m_Batch,
			"enemy QRF proof live authority");
		HST_OperationProjectionDecision band = materialization.EvaluateExactPlayerQRFForProximity(
			fixture.m_Operation,
			false,
			true,
			1800.0,
			2160.0);
		HST_OperationProjectionDecision leave = materialization.EvaluateExactPlayerQRFForProximity(
			fixture.m_Operation,
			false,
			false,
			1800.0,
			2160.0);
		HST_OperationTransitionResult folding = operations.BeginExactEnemyDefensiveQRFDematerialization(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Group,
			"enemy QRF proof fold");
		HST_ForceSpawnQueueCallbackResult held = fixture.m_Queue.RequeueSuccessfulProjectionForStrategicHold(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			fixture.m_State.m_iElapsedSeconds + 1,
			fixture.m_State.m_iElapsedSeconds + 181);
		fixture.m_Group.m_bSpawnedEntity = false;
		fixture.m_Group.m_iSpawnedAgentCount = 0;
		HST_OperationTransitionResult virtualized = operations.CompleteExactEnemyDefensiveQRFDematerialization(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Group,
			fixture.m_Batch,
			"enemy QRF proof fold complete");

		int livingAfter = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		bool decisionsExact = enter.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE
			&& band.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_RETAIN
			&& leave.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_DEMATERIALIZE
			&& enter.m_fMaterializeOutDistanceMeters > enter.m_fMaterializeInDistanceMeters;
		bool transitionsExact = released && released.m_bAccepted
			&& materializing && materializing.m_bAccepted
			&& physical && physical.m_bAccepted
			&& folding && folding.m_bAccepted
			&& held && held.m_bAccepted
			&& virtualized && virtualized.m_bAccepted;
		bool authorityExact = fixture.m_Batch.m_bStrategicProjectionHeld
			&& fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& fixture.m_Operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& livingAfter == livingBefore;
		bool noFoldRefund = pool.m_iAttackResources == attackBefore
			&& pool.m_iSupportResources == supportBefore
			&& !fixture.m_Order.m_bResourceSettlementApplied;
		report.m_bProjectionExact = movementExact && decisionsExact && transitionsExact && authorityExact && noFoldRefund;
		report.m_sProjectionEvidence = string.Format(
			"advance %1s/%2m | decisions %3/%4/%5 | transitions %6/%7/%8/%9",
			advanced && advanced.m_iAdvancedSeconds,
			advanced && Math.Round(advanced.m_fAdvancedMeters),
			enter.m_eDecision,
			band.m_eDecision,
			leave.m_eDecision,
			materializing && materializing.m_bAccepted,
			physical && physical.m_bAccepted,
			folding && folding.m_bAccepted,
			virtualized && virtualized.m_bAccepted);
		report.m_sProjectionEvidence = report.m_sProjectionEvidence + string.Format(
			" | living %1/%2 | held/strategic %3/%4 | fold refund %5/%6",
			livingBefore,
			livingAfter,
			fixture.m_Batch.m_bStrategicProjectionHeld,
			fixture.m_Operation.m_ePositionAuthority,
			pool.m_iAttackResources - attackBefore,
			pool.m_iSupportResources - supportBefore);
	}

	protected void ProveSettlement(HST_EnemyQRFOperationProofReport report)
	{
		HST_EnemyQRFOperationProofFixture fixture = BuildAdmittedFixture("settlement");
		if (!Ready(fixture))
		{
			report.m_sSettlementEvidence = BuildFixtureFailure(fixture);
			return;
		}

		int initialLiving = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		int casualties = Math.Min(2, Math.Max(0, initialLiving - 1));
		bool casualtiesAccepted = ConfirmStrategicCasualties(fixture, casualties);
		int survivors = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		HST_ZoneState target = fixture.m_State.FindZone(PROOF_TARGET_ZONE_ID);
		int captureProgressBefore;
		if (target)
			captureProgressBefore = target.m_iResistanceCaptureProgress;
		HST_OperationService operations = new HST_OperationService();
		fixture.m_Operation.m_fRouteProgressMeters = fixture.m_Operation.m_fRouteTotalDistanceMeters;
		fixture.m_Operation.m_vStrategicPosition = fixture.m_Operation.m_vRouteEndPosition;
		fixture.m_Group.m_vPosition = fixture.m_Operation.m_vRouteEndPosition;
		fixture.m_State.m_iElapsedSeconds++;
		HST_OperationTransitionResult arrived = operations.MarkExactEnemyDefensiveQRFOnStation(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Group);
		bool onStationTick = fixture.m_ExactQRF.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		bool outcomeExact = onStationTick && fixture.m_Order.m_bOutcomeApplied && target
			&& target.m_iResistanceCaptureProgress < captureProgressBefore
			&& fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN;
		fixture.m_Operation.m_fRouteProgressMeters = fixture.m_Operation.m_fRouteTotalDistanceMeters;
		fixture.m_Operation.m_vStrategicPosition = fixture.m_Operation.m_vOriginPosition;
		fixture.m_Group.m_vPosition = fixture.m_Operation.m_vOriginPosition;
		fixture.m_State.m_iElapsedSeconds++;

		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		bool settlementTick = fixture.m_ExactQRF.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		int attackAfter = pool.m_iAttackResources;
		int supportAfter = pool.m_iSupportResources;
		string settlementId = fixture.m_Order.m_sResourceSettlementId;
		bool replayTick = fixture.m_ExactQRF.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		int attackAfterReplay = pool.m_iAttackResources;
		int supportAfterReplay = pool.m_iSupportResources;

		int expectedAttackRefund = fixture.m_Order.m_iAttackCost * survivors / fixture.m_Manifest.m_iAcceptedMemberCount;
		int expectedSupportRefund = fixture.m_Order.m_iSupportCost * survivors / fixture.m_Manifest.m_iAcceptedMemberCount;
		bool routeExact = arrived && arrived.m_bAccepted && outcomeExact;
		bool refundExact = attackAfter - attackBefore == expectedAttackRefund
			&& supportAfter - supportBefore == expectedSupportRefund
			&& fixture.m_Order.m_iRefundedAttackResources == expectedAttackRefund
			&& fixture.m_Order.m_iRefundedSupportResources == expectedSupportRefund
			&& fixture.m_Order.m_iSettlementAcceptedMemberCount == initialLiving
			&& fixture.m_Order.m_iSettlementSurvivorMemberCount == survivors;
		bool oneTime = !replayTick
			&& attackAfterReplay == attackAfter
			&& supportAfterReplay == supportAfter
			&& fixture.m_Order.m_sResourceSettlementId == settlementId;
		bool terminalExact = settlementTick
			&& fixture.m_Order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED
			&& fixture.m_Order.m_bResourceSettlementApplied
			&& fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& fixture.m_Operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
			&& fixture.m_Operation.m_sSettlementId == settlementId;
		HST_CampaignSaveData terminalSave = new HST_CampaignSaveData();
		terminalSave.Capture(fixture.m_State);
		HST_CampaignState terminalRestored = terminalSave.Restore();
		HST_EnemyOrderState terminalRestoredOrder;
		HST_OperationRecordState terminalRestoredOperation;
		if (terminalRestored)
		{
			terminalRestoredOrder = FindOrder(terminalRestored, fixture.m_Order.m_sOrderId);
			terminalRestoredOperation = terminalRestored.FindOperation(fixture.m_Operation.m_sOperationId);
		}
		bool terminalRestoreExact = terminalRestoredOrder && terminalRestoredOperation
			&& terminalRestoredOrder.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED
			&& terminalRestoredOrder.m_sRuntimeStatus != "exact_operation_invalidated"
			&& terminalRestoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& terminalRestoredOperation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
			&& terminalRestoredOperation.m_sSettlementId == settlementId
			&& !terminalRestored.FindForceSpawnResult(fixture.m_Order.m_sSpawnResultId)
			&& !terminalRestored.FindActiveGroup(fixture.m_Order.m_sGroupId);
		int captureProgressAfter = captureProgressBefore;
		if (target)
			captureProgressAfter = target.m_iResistanceCaptureProgress;
		string refundReceiptReplayEvidence;
		bool refundReceiptReplayExact = ProveRefundAppliedReceiptMissingReplay(refundReceiptReplayEvidence);
		string supportOnlyEvidence;
		bool supportOnlyExact = ProveSupportOnlySettlement(supportOnlyEvidence);
		report.m_bSettlementExact = casualtiesAccepted && routeExact && refundExact && oneTime
			&& terminalExact && terminalRestoreExact && refundReceiptReplayExact
			&& supportOnlyExact;
		report.m_sSettlementEvidence = string.Format(
			"living %1 -> %2 | refund attack %3/%4 support %5/%6",
			initialLiving,
			survivors,
			attackAfter - attackBefore,
			expectedAttackRefund,
			supportAfter - supportBefore,
			expectedSupportRefund);
		report.m_sSettlementEvidence = report.m_sSettlementEvidence + string.Format(
			" | pressure %1 -> %2 | terminal/status %3/%4 | settlement %5 | replay delta %6/%7 | terminal restore %8",
			captureProgressBefore,
			captureProgressAfter,
			fixture.m_Operation.m_eTerminalResult,
			fixture.m_Order.m_eStatus,
			settlementId,
			attackAfterReplay - attackAfter,
			supportAfterReplay - supportAfter,
			terminalRestoreExact);
		report.m_sSettlementEvidence = report.m_sSettlementEvidence
			+ " | refund-applied receipt-missing replay " + refundReceiptReplayEvidence;
		report.m_sSettlementEvidence = report.m_sSettlementEvidence
			+ " | support-only " + supportOnlyEvidence;
	}

	protected bool ProveRefundAppliedReceiptMissingReplay(out string evidence)
	{
		HST_EnemyQRFOperationProofFixture fixture
			= BuildAdmittedFixture("settlement_refund_receipt_replay");
		if (!Ready(fixture))
		{
			evidence = BuildFixtureFailure(fixture);
			return false;
		}

		int initialLiving = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		int casualties = Math.Min(2, Math.Max(0, initialLiving - 1));
		bool casualtiesAccepted = ConfirmStrategicCasualties(fixture, casualties);
		int survivors = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		HST_OperationService operations = new HST_OperationService();
		fixture.m_Operation.m_fRouteProgressMeters = fixture.m_Operation.m_fRouteTotalDistanceMeters;
		fixture.m_Operation.m_vStrategicPosition = fixture.m_Operation.m_vRouteEndPosition;
		fixture.m_Group.m_vPosition = fixture.m_Operation.m_vRouteEndPosition;
		fixture.m_State.m_iElapsedSeconds++;
		HST_OperationTransitionResult arrived = operations.MarkExactEnemyDefensiveQRFOnStation(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Group);
		bool onStationTick = fixture.m_ExactQRF.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		fixture.m_Operation.m_fRouteProgressMeters = fixture.m_Operation.m_fRouteTotalDistanceMeters;
		fixture.m_Operation.m_vStrategicPosition = fixture.m_Operation.m_vOriginPosition;
		fixture.m_Group.m_vPosition = fixture.m_Operation.m_vOriginPosition;
		fixture.m_State.m_iElapsedSeconds++;
		bool returnReadyBeforeRefund = onStationTick
			&& fixture.m_Order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& fixture.m_Operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN
			&& fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;

		string settlementKind = "returned_survivors";
		string settlementId = HST_OperationService.BuildSettlementId(
			fixture.m_Order.m_sOperationId,
			settlementKind);
		string refundMutationId = "enemy_resource_refund_" + settlementId;
		int expectedAttackRefund = fixture.m_Order.m_iAttackCost * survivors
			/ fixture.m_Manifest.m_iAcceptedMemberCount;
		int expectedSupportRefund = fixture.m_Order.m_iSupportCost * survivors
			/ fixture.m_Manifest.m_iAcceptedMemberCount;
		bool receiptCleanBeforeRefund = !fixture.m_Order.m_bResourceSettlementApplied
			&& fixture.m_Order.m_sResourceRefundMutationId.IsEmpty()
			&& fixture.m_Order.m_sResourceSettlementId.IsEmpty()
			&& fixture.m_Order.m_sResourceSettlementKind.IsEmpty()
			&& fixture.m_Order.m_iSettlementAcceptedMemberCount == 0
			&& fixture.m_Order.m_iSettlementSurvivorMemberCount == 0
			&& fixture.m_Order.m_iRefundedAttackResources == 0
			&& fixture.m_Order.m_iRefundedSupportResources == 0;
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		HST_EnemySupportLedgerState ledger = fixture.m_State.FindEnemySupportLedger(
			PROOF_FACTION_KEY,
			PROOF_TARGET_ZONE_ID);
		if (!pool || !ledger)
		{
			evidence = "returned-survivor pool or support ledger unavailable";
			return false;
		}
		int attackBeforeDirectRefund = pool.m_iAttackResources;
		int supportBeforeDirectRefund = pool.m_iSupportResources;
		int ledgerAttackSpentBeforeDirectRefund = ledger.m_iAttackSpent;
		int ledgerSupportSpentBeforeDirectRefund = ledger.m_iSupportSpent;
		int ledgerAttackBeforeDirectRefund = ledger.m_iRefundedAttackResources;
		int ledgerSupportBeforeDirectRefund = ledger.m_iRefundedSupportResources;
		bool directRefundApplied = fixture.m_EnemyDirector.RefundDefenseResources(
			fixture.m_State,
			fixture.m_Order.m_sFactionKey,
			fixture.m_Order.m_sTargetZoneId,
			expectedAttackRefund,
			expectedSupportRefund,
			"enemy QRF refund-applied receipt-missing replay proof",
			refundMutationId,
			settlementId,
			fixture.m_Order.m_sOrderId,
			fixture.m_Order.m_sOperationId,
			fixture.m_Order.m_sManifestId);
		int attackAfterDirectRefund = pool.m_iAttackResources;
		int supportAfterDirectRefund = pool.m_iSupportResources;
		int ledgerAttackSpentAfterDirectRefund = ledger.m_iAttackSpent;
		int ledgerSupportSpentAfterDirectRefund = ledger.m_iSupportSpent;
		int ledgerAttackAfterDirectRefund = ledger.m_iRefundedAttackResources;
		int ledgerSupportAfterDirectRefund = ledger.m_iRefundedSupportResources;
		bool receiptCleanAfterRefund = !fixture.m_Order.m_bResourceSettlementApplied
			&& fixture.m_Order.m_sResourceRefundMutationId.IsEmpty()
			&& fixture.m_Order.m_sResourceSettlementId.IsEmpty()
			&& fixture.m_Order.m_sResourceSettlementKind.IsEmpty()
			&& fixture.m_Order.m_iSettlementAcceptedMemberCount == 0
			&& fixture.m_Order.m_iSettlementSurvivorMemberCount == 0
			&& fixture.m_Order.m_iRefundedAttackResources == 0
			&& fixture.m_Order.m_iRefundedSupportResources == 0;

		bool firstTick = fixture.m_ExactQRF.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		int attackAfterFirstTick = pool.m_iAttackResources;
		int supportAfterFirstTick = pool.m_iSupportResources;
		int ledgerAttackSpentAfterFirstTick = ledger.m_iAttackSpent;
		int ledgerSupportSpentAfterFirstTick = ledger.m_iSupportSpent;
		int ledgerAttackAfterFirstTick = ledger.m_iRefundedAttackResources;
		int ledgerSupportAfterFirstTick = ledger.m_iRefundedSupportResources;
		bool secondTick = fixture.m_ExactQRF.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		int mutationCount = CountStrategicMutations(
			fixture.m_State,
			refundMutationId);
		HST_EnemyStrategicMutationState mutation
			= fixture.m_State.FindEnemyStrategicMutation(refundMutationId);

		bool returnedFixtureExact = casualtiesAccepted && arrived && arrived.m_bAccepted
			&& returnReadyBeforeRefund
			&& fixture.m_Operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED;
		bool directRefundExact = directRefundApplied
			&& attackAfterDirectRefund - attackBeforeDirectRefund == expectedAttackRefund
			&& supportAfterDirectRefund - supportBeforeDirectRefund == expectedSupportRefund
			&& ledgerAttackSpentAfterDirectRefund
				== Math.Max(0, ledgerAttackSpentBeforeDirectRefund - expectedAttackRefund)
			&& ledgerSupportSpentAfterDirectRefund
				== Math.Max(0, ledgerSupportSpentBeforeDirectRefund - expectedSupportRefund)
			&& ledgerAttackAfterDirectRefund - ledgerAttackBeforeDirectRefund
				== expectedAttackRefund
			&& ledgerSupportAfterDirectRefund - ledgerSupportBeforeDirectRefund
				== expectedSupportRefund;
		bool receiptComplete = firstTick
			&& fixture.m_Order.m_bResourceSettlementApplied
			&& fixture.m_Order.m_sResourceRefundMutationId == refundMutationId
			&& fixture.m_Order.m_sResourceSettlementId == settlementId
			&& fixture.m_Order.m_sResourceSettlementKind == settlementKind
			&& fixture.m_Order.m_iSettlementAcceptedMemberCount == initialLiving
			&& fixture.m_Order.m_iSettlementSurvivorMemberCount == survivors
			&& fixture.m_Order.m_iRefundedAttackResources == expectedAttackRefund
			&& fixture.m_Order.m_iRefundedSupportResources == expectedSupportRefund
			&& fixture.m_Order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED
			&& fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& fixture.m_Operation.m_sSettlementId == settlementId;
		bool oneMutation = mutationCount == 1 && mutation && mutation.m_bApplied
			&& mutation.m_sMutationId == refundMutationId
			&& mutation.m_sSourceId == settlementId
			&& mutation.m_sOrderId == fixture.m_Order.m_sOrderId
			&& mutation.m_sOperationId == fixture.m_Order.m_sOperationId
			&& mutation.m_sManifestId == fixture.m_Order.m_sManifestId
			&& mutation.m_iAttackDelta == expectedAttackRefund
			&& mutation.m_iSupportDelta == expectedSupportRefund;
		bool noReplayDelta = attackAfterFirstTick == attackAfterDirectRefund
			&& supportAfterFirstTick == supportAfterDirectRefund
			&& ledgerAttackSpentAfterFirstTick == ledgerAttackSpentAfterDirectRefund
			&& ledgerSupportSpentAfterFirstTick == ledgerSupportSpentAfterDirectRefund
			&& ledgerAttackAfterFirstTick == ledgerAttackAfterDirectRefund
			&& ledgerSupportAfterFirstTick == ledgerSupportAfterDirectRefund
			&& pool.m_iAttackResources == attackAfterFirstTick
			&& pool.m_iSupportResources == supportAfterFirstTick
			&& ledger.m_iAttackSpent == ledgerAttackSpentAfterFirstTick
			&& ledger.m_iSupportSpent == ledgerSupportSpentAfterFirstTick
			&& ledger.m_iRefundedAttackResources == ledgerAttackAfterFirstTick
			&& ledger.m_iRefundedSupportResources == ledgerSupportAfterFirstTick;
		bool secondTickStable = !secondTick
			&& CountStrategicMutations(fixture.m_State, refundMutationId) == 1
			&& fixture.m_Order.m_sResourceSettlementId == settlementId
			&& fixture.m_Operation.m_sSettlementId == settlementId;

		evidence = string.Format(
			"returned %1/%2 | direct/clean %3/%4/%5 | refund %6/%7 | first/second tick %8/%9",
			initialLiving,
			survivors,
			directRefundApplied,
			receiptCleanBeforeRefund,
			receiptCleanAfterRefund,
			expectedAttackRefund,
			expectedSupportRefund,
			firstTick,
			secondTick);
		evidence = evidence + string.Format(
			" | receipt/terminal/mutation %1/%2/%3 | replay pool delta %4/%5 ledger delta %6/%7",
			receiptComplete,
			fixture.m_Operation.m_eTerminalResult,
			mutationCount,
			attackAfterFirstTick - attackAfterDirectRefund,
			supportAfterFirstTick - supportAfterDirectRefund,
			ledgerAttackAfterFirstTick - ledgerAttackAfterDirectRefund,
			ledgerSupportAfterFirstTick - ledgerSupportAfterDirectRefund);
		evidence = evidence + string.Format(
			" | ledger spent direct/replay %1/%2/%3/%4",
			ledgerAttackSpentAfterDirectRefund - ledgerAttackSpentBeforeDirectRefund,
			ledgerSupportSpentAfterDirectRefund - ledgerSupportSpentBeforeDirectRefund,
			ledgerAttackSpentAfterFirstTick - ledgerAttackSpentAfterDirectRefund,
			ledgerSupportSpentAfterFirstTick - ledgerSupportSpentAfterDirectRefund);
		return returnedFixtureExact && receiptCleanBeforeRefund && receiptCleanAfterRefund
			&& directRefundExact && receiptComplete && oneMutation && noReplayDelta
			&& secondTickStable;
	}

	protected bool ProveSupportOnlySettlement(out string evidence)
	{
		HST_EnemyQRFOperationProofFixture fixture
			= BuildAdmittedFixture("settlement_support_only", 0, PROOF_SUPPORT_COST);
		if (!Ready(fixture))
		{
			evidence = BuildFixtureFailure(fixture);
			return false;
		}

		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		bool firstSettlement = fixture.m_ExactQRF.SettleTrackedOpenOrderForAdministrativeStop(
			fixture.m_State,
			fixture.m_EnemyDirector,
			fixture.m_Order,
			"exact enemy defensive QRF support-only proof stop");
		int attackAfter = pool.m_iAttackResources;
		int supportAfter = pool.m_iSupportResources;
		int refundMutationCount = CountStrategicMutations(
			fixture.m_State,
			fixture.m_Order.m_sResourceRefundMutationId);
		bool replaySettlement = fixture.m_ExactQRF.SettleTrackedOpenOrderForAdministrativeStop(
			fixture.m_State,
			fixture.m_EnemyDirector,
			fixture.m_Order,
			"exact enemy defensive QRF support-only proof stop");
		bool exact = firstSettlement && replaySettlement
			&& fixture.m_Order.m_iAttackCost == 0
			&& fixture.m_Order.m_iSupportCost == PROOF_SUPPORT_COST
			&& fixture.m_Order.m_bResourceSettlementApplied
			&& fixture.m_Order.m_iRefundedAttackResources == 0
			&& fixture.m_Order.m_iRefundedSupportResources == PROOF_SUPPORT_COST
			&& fixture.m_Order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& fixture.m_Operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED
			&& attackAfter == attackBefore
			&& supportAfter == supportBefore + PROOF_SUPPORT_COST
			&& pool.m_iAttackResources == attackAfter
			&& pool.m_iSupportResources == supportAfter
			&& refundMutationCount == 1
			&& CountStrategicMutations(
				fixture.m_State,
				fixture.m_Order.m_sResourceRefundMutationId) == 1;
		evidence = string.Format(
			"cost %1/%2 | pool %3/%4 -> %5/%6 | refund %7/%8 | first/replay %9",
			fixture.m_Order.m_iAttackCost,
			fixture.m_Order.m_iSupportCost,
			attackBefore,
			supportBefore,
			attackAfter,
			supportAfter,
			fixture.m_Order.m_iRefundedAttackResources,
			fixture.m_Order.m_iRefundedSupportResources,
			firstSettlement && replaySettlement);
		evidence = evidence + string.Format(
			" | terminal/status/mutation/exact %1/%2/%3/%4",
			fixture.m_Operation.m_eTerminalResult,
			fixture.m_Order.m_eStatus,
			refundMutationCount,
			exact);
		return exact;
	}

	protected void ProveRestore(HST_EnemyQRFOperationProofReport report)
	{
		HST_EnemyQRFOperationProofFixture fixture = BuildAdmittedFixture("restore");
		if (!Ready(fixture))
		{
			report.m_sRestoreEvidence = BuildFixtureFailure(fixture);
			return;
		}

		ConfirmStrategicCasualties(fixture, 1);
		fixture.m_State.m_iElapsedSeconds += 90;
		HST_StrategicMovementService movement = new HST_StrategicMovementService();
		movement.AdvanceExactPlayerQRF(fixture.m_State, fixture.m_Operation, fixture.m_Group);
		int livingBefore = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		float progressBefore = fixture.m_Operation.m_fRouteProgressMeters;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_ForceSpawnQueueService restoredQueue = new HST_ForceSpawnQueueService();
		bool restoreReconciled;
		if (restored)
		{
			restoredQueue.ReconcileCampaignAfterRestore(restored);
			HST_EnemyQRFOperationService restoredExactQRF = new HST_EnemyQRFOperationService();
			restoredExactQRF.SetRuntimeServices(restoredQueue, new HST_ForceSpawnAdapterService(), new HST_PhysicalWarService());
			restoreReconciled = restoredExactQRF.ReconcileAfterRestore(restored, new HST_EnemyDirectorService());
		}
		HST_EnemyOrderState restoredOrder;
		HST_ForceManifestState restoredManifest;
		HST_ForceSpawnResultState restoredBatch;
		HST_ActiveGroupState restoredGroup;
		HST_OperationRecordState restoredOperation;
		if (restored)
		{
			restoredOrder = FindOrder(restored, fixture.m_Order.m_sOrderId);
			restoredManifest = restored.FindForceManifest(fixture.m_Manifest.m_sManifestId);
			restoredBatch = restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
			restoredGroup = restored.FindActiveGroup(fixture.m_Group.m_sGroupId);
			restoredOperation = restored.FindOperation(fixture.m_Operation.m_sOperationId);
		}
		int livingAfter;
		if (restoredBatch)
			livingAfter = fixture.m_Queue.CountStrategicLivingMemberSlots(restoredBatch);
		bool recordsPresent = restoredOrder && restoredManifest && restoredBatch && restoredGroup && restoredOperation;
		bool recordsUnique = recordsPresent
			&& CountManifestId(restored, restoredManifest.m_sManifestId) == 1
			&& CountOperationId(restored, restoredOperation.m_sOperationId) == 1
			&& CountBatchId(restored, restoredBatch.m_sResultId) == 1
			&& CountGroupId(restored, restoredGroup.m_sGroupId) == 1;
		bool identityExact;
		bool authorityExact;
		bool routeExact;
		if (recordsPresent)
		{
			identityExact = restoredOrder.m_sOperationId == restoredOperation.m_sOperationId
				&& restoredOrder.m_sManifestId == restoredManifest.m_sManifestId
				&& restoredOrder.m_sSpawnResultId == restoredBatch.m_sResultId
				&& restoredOrder.m_sGroupId == restoredGroup.m_sGroupId
				&& restoredOperation.m_sEnemyOrderId == restoredOrder.m_sOrderId
				&& restoredGroup.m_sEnemyOrderId == restoredOrder.m_sOrderId;
			authorityExact = restoredOrder.m_iOperationContractVersion == HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION
				&& restoredManifest.m_bFrozen
				&& restoredManifest.m_sManifestHash == fixture.m_Manifest.m_sManifestHash
				&& restoredBatch.m_bStrategicProjectionHeld
				&& restoredOperation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				&& restoredOperation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
			routeExact = Math.AbsFloat(restoredOperation.m_fRouteProgressMeters - progressBefore) < 0.1
				&& restoredOperation.m_eDutyState == fixture.m_Operation.m_eDutyState;
		}
		bool schemaExact = restored && restored.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION
			&& restored.m_iLastLoadedSchemaVersion == HST_CampaignState.SCHEMA_VERSION;
		bool rosterExact = livingAfter == livingBefore
			&& restoredBatch
			&& fixture.m_Queue.CountConfirmedCasualtyMemberSlots(restoredBatch) == 1;
		bool legacyExact = restored && restored.m_aSupportRequests.Count() == 0 && restored.m_aQRFs.Count() == 0;
		HST_EnemyQRFRestoreCorruptionProofSummary corruption = ProveRestoreCorruptionGuards();
		HST_EnemyQRFPreparedRecoveryProofSummary preparedRecovery
			= ProvePreparedSettlementRecovery();
		report.m_bRestoreExact = schemaExact;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = restoreReconciled;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = recordsUnique;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = identityExact;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = authorityExact;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = routeExact;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = rosterExact;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = legacyExact;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = corruption.m_bPartialReceiptQuarantined;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = corruption.m_bMissingBacklinkRejected;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = preparedRecovery.m_bPreRefundExact;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = preparedRecovery.m_bPostRefundExact;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = preparedRecovery.m_bPostReceiptExact;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = preparedRecovery.m_bUncommittedFullRefundExact;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = preparedRecovery.m_bCorruptedBaselineFailClosed;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = preparedRecovery.m_bCorruptedPoolTailFailClosed;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = preparedRecovery.m_bTamperedSurvivorTupleFailClosed;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = preparedRecovery.m_bSettledPoolTailFailClosed;
		if (report.m_bRestoreExact)
			report.m_bRestoreExact = preparedRecovery.m_bHistoricalMutationlessPreserved;
		report.m_sRestoreEvidence = string.Format(
			"schema %1/%2 | records %3 | living %4/%5 | progress %6/%7",
			restored && restored.m_iSchemaVersion,
			HST_CampaignState.SCHEMA_VERSION,
			recordsPresent,
			livingBefore,
			livingAfter,
			Math.Round(progressBefore),
			restoredOperation && Math.Round(restoredOperation.m_fRouteProgressMeters));
		report.m_sRestoreEvidence = report.m_sRestoreEvidence + string.Format(
			" | reconciled/unique/identity/authority/legacy %1/%2/%3/%4/%5",
			restoreReconciled,
			recordsUnique,
			identityExact,
			authorityExact,
			legacyExact);
		report.m_sRestoreEvidence = report.m_sRestoreEvidence + " | corruption guards " + corruption.m_sEvidence;
		report.m_sRestoreEvidence = report.m_sRestoreEvidence
			+ " | prepared recovery " + preparedRecovery.m_sEvidence;
	}

	protected void ProveRejection(HST_EnemyQRFOperationProofReport report)
	{
		HST_EnemyQRFOperationProofFixture fixture = BuildAdmittedFixture("rejection");
		if (!Ready(fixture))
		{
			report.m_sRejectionEvidence = BuildFixtureFailure(fixture);
			return;
		}

		int operationsBefore = fixture.m_State.m_aOperations.Count();
		int manifestsBefore = fixture.m_State.m_aForceManifests.Count();
		int batchesBefore = fixture.m_State.m_aForceSpawnResults.Count();
		int groupsBefore = fixture.m_State.m_aActiveGroups.Count();
		int ordersBefore = fixture.m_State.m_aEnemyOrders.Count();
		HST_EnemyOrderState duplicate = BuildOrder(fixture.m_State, "duplicate");
		HST_EnemyDefensiveQRFManifestResult duplicatePlan = fixture.m_Planning.PlanExactEnemyDefensiveQRF(
			fixture.m_State,
			fixture.m_Preset,
			duplicate);
		HST_EnemySupportLedgerState ledger = fixture.m_State.FindEnemySupportLedger(PROOF_FACTION_KEY, PROOF_TARGET_ZONE_ID);
		int cooldownBeforePreflight;
		int lastSpendBeforePreflight;
		if (ledger)
		{
			cooldownBeforePreflight = ledger.m_iCooldownUntilSecond;
			lastSpendBeforePreflight = ledger.m_iLastSpendSecond;
		}
		HST_ForceManifestState duplicateManifest;
		if (duplicatePlan && duplicatePlan.m_bSuccess)
			duplicateManifest = duplicatePlan.m_Manifest;
		HST_EnemyQRFAdmissionResult duplicatePreflight = fixture.m_ExactQRF.CanAdmitPreparedOrder(
			fixture.m_State,
			duplicate,
			duplicateManifest,
			fixture.m_EnemyDirector);
		bool preflightSideEffectFree = duplicatePreflight && !duplicatePreflight.m_bSuccess && ledger
			&& ledger.m_iCooldownUntilSecond == cooldownBeforePreflight
			&& ledger.m_iLastSpendSecond == lastSpendBeforePreflight;
		if (ledger)
			ledger.m_iCooldownUntilSecond = fixture.m_State.m_iElapsedSeconds;
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		int attackBeforeDuplicate = pool.m_iAttackResources;
		int supportBeforeDuplicate = pool.m_iSupportResources;
		string duplicateSpendReason;
		duplicate.m_sResourceDebitMutationId
			= "enemy_resource_debit_" + duplicate.m_sOrderId;
		bool duplicateSpent = duplicatePlan && duplicatePlan.m_bSuccess
			&& fixture.m_EnemyDirector.TrySpendDefense(
				fixture.m_State,
				fixture.m_State.FindZone(PROOF_TARGET_ZONE_ID),
				PROOF_FACTION_KEY,
				duplicate.m_iAttackCost,
				duplicate.m_iSupportCost,
				duplicateSpendReason,
				duplicate.m_sResourceDebitMutationId,
				duplicate.m_sOrderId,
				duplicate.m_sOrderId,
				duplicate.m_sOperationId,
				duplicateManifest.m_sManifestId);
		HST_EnemyQRFAdmissionResult duplicateAdmission;
		if (duplicateSpent)
		{
			fixture.m_State.m_aEnemyOrders.Insert(duplicate);
			duplicateAdmission = fixture.m_ExactQRF.AdmitPreparedOrder(
				fixture.m_State,
				duplicate,
				duplicatePlan.m_Manifest,
				fixture.m_EnemyDirector);
		}
		bool duplicateRejected = preflightSideEffectFree && duplicateSpent && duplicateAdmission && !duplicateAdmission.m_bSuccess
			&& duplicate.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& duplicate.m_bResourceSettlementApplied
			&& pool.m_iAttackResources == attackBeforeDuplicate
			&& pool.m_iSupportResources == supportBeforeDuplicate
			&& fixture.m_State.m_aOperations.Count() == operationsBefore
			&& fixture.m_State.m_aForceManifests.Count() == manifestsBefore
			&& fixture.m_State.m_aForceSpawnResults.Count() == batchesBefore
			&& fixture.m_State.m_aActiveGroups.Count() == groupsBefore
			&& fixture.m_State.m_aEnemyOrders.Count() == ordersBefore + 1;

		HST_EnemyOrderState unsupported = BuildOrder(fixture.m_State, "unsupported");
		unsupported.m_iOperationContractVersion = 0;
		HST_EnemyDefensiveQRFManifestResult unsupportedPlan = fixture.m_Planning.PlanExactEnemyDefensiveQRF(
			fixture.m_State,
			fixture.m_Preset,
			unsupported);
		bool unsupportedRejected = unsupportedPlan && !unsupportedPlan.m_bSuccess
			&& !fixture.m_ExactQRF.IsExactEnemyDefensiveQRF(unsupported)
			&& !HST_OperationService.RequiresOperation(unsupported);
		bool legacyCollisionRejected;
		string legacyCollisionEvidence = ProveLegacyCollision(legacyCollisionRejected);
		HST_EnemyQRFReplayAmbiguityProofSummary replayAmbiguity = ProveCommittedReplayAmbiguity();
		report.m_bRejectionExact = duplicateRejected && unsupportedRejected && legacyCollisionRejected
			&& replayAmbiguity.m_bMissingCanonicalRejected && replayAmbiguity.m_bShadowRejected;
		report.m_sRejectionEvidence = string.Format(
			"duplicate preflight/spent/rejected/refunded %1/%2/%3/%4 | terminal receipt + exact row deltas %5/%6/%7/%8/%9",
			preflightSideEffectFree,
			duplicateSpent,
			duplicateAdmission && !duplicateAdmission.m_bSuccess,
			pool.m_iAttackResources == attackBeforeDuplicate && pool.m_iSupportResources == supportBeforeDuplicate,
			fixture.m_State.m_aEnemyOrders.Count() - ordersBefore,
			fixture.m_State.m_aOperations.Count() - operationsBefore,
			fixture.m_State.m_aForceManifests.Count() - manifestsBefore,
			fixture.m_State.m_aForceSpawnResults.Count() - batchesBefore,
			fixture.m_State.m_aActiveGroups.Count() - groupsBefore);
		report.m_sRejectionEvidence = report.m_sRejectionEvidence + string.Format(
			" | unsupported rejected %1 | reasons duplicate '%2' unsupported '%3'",
			unsupportedRejected,
			duplicateAdmission && duplicateAdmission.m_sFailureReason,
			unsupportedPlan && unsupportedPlan.m_sFailureReason);
		report.m_sRejectionEvidence = report.m_sRejectionEvidence + " | " + legacyCollisionEvidence;
		report.m_sRejectionEvidence = report.m_sRejectionEvidence + " | replay ambiguity " + replayAmbiguity.m_sEvidence;
	}

	protected HST_EnemyQRFRestoreCorruptionProofSummary ProveRestoreCorruptionGuards()
	{
		HST_EnemyQRFRestoreCorruptionProofSummary summary = new HST_EnemyQRFRestoreCorruptionProofSummary();
		string partialEvidence;
		string backlinkEvidence;
		summary.m_bPartialReceiptQuarantined = ProvePartialReceiptRestoreQuarantine(partialEvidence);
		summary.m_bMissingBacklinkRejected = ProveMissingGroupBacklinkRestore(backlinkEvidence);
		summary.m_sEvidence = "partial " + partialEvidence + " | backlink " + backlinkEvidence;
		return summary;
	}

	protected bool ProvePreparedFullRefundRecoveryMatrix(out string evidence)
	{
		string preRefundEvidence;
		string postRefundEvidence;
		string postReceiptEvidence;
		bool preRefundExact = ProvePreparedFullRefundRestartCut(
			PREPARED_CUT_BEFORE_REFUND,
			false,
			preRefundEvidence);
		bool postRefundExact = ProvePreparedFullRefundRestartCut(
			PREPARED_CUT_AFTER_REFUND,
			true,
			postRefundEvidence);
		bool postReceiptExact = ProvePreparedFullRefundRestartCut(
			PREPARED_CUT_AFTER_RECEIPT,
			false,
			postReceiptEvidence);
		evidence = "pre-refund " + preRefundEvidence
			+ " | post-refund residual fallback " + postRefundEvidence
			+ " | post-receipt " + postReceiptEvidence;
		if (!preRefundExact)
			return false;
		if (!postRefundExact)
			return false;
		return postReceiptExact;
	}

	protected bool ProvePreparedFullRefundRestartCut(
		int restartCut,
		bool retainDeterministicResiduals,
		out string evidence)
	{
		HST_EnemyQRFPreparedRecoveryCutContext context
			= BuildPreparedFullRefundCutContext(
				restartCut,
				retainDeterministicResiduals);
		if (!context)
		{
			evidence = "uncommitted full-refund context unavailable";
			return false;
		}
		if (!context.m_sFailure.IsEmpty())
		{
			evidence = context.m_sFailure;
			return false;
		}
		HST_EnemyQRFPreparedRecoveryRunResult result
			= RunPreparedFullRefundRecoveryCut(context);
		evidence = BuildPreparedRecoveryEvidence(context, result);
		evidence = evidence + string.Format(
			" | uncommitted/residual %1/%2",
			!context.m_Fixture.m_Order.m_bStrategicServiceCommitted,
			context.m_bRetainDeterministicResiduals);
		if (!context.m_bPrefixExactBeforeSave)
			return false;
		if (!result)
			return false;
		return result.AllExact();
	}

	protected HST_EnemyQRFPreparedRecoveryCutContext BuildPreparedFullRefundCutContext(
		int restartCut,
		bool retainDeterministicResiduals)
	{
		HST_EnemyQRFPreparedRecoveryCutContext context
			= new HST_EnemyQRFPreparedRecoveryCutContext();
		context.m_iRestartCut = restartCut;
		context.m_bRetainDeterministicResiduals
			= retainDeterministicResiduals;
		if (restartCut < PREPARED_CUT_BEFORE_REFUND)
		{
			context.m_sFailure = "unsupported uncommitted full-refund cut";
			return context;
		}
		if (restartCut > PREPARED_CUT_AFTER_RECEIPT)
		{
			context.m_sFailure = "unsupported uncommitted full-refund cut";
			return context;
		}
		string suffix = "full_refund_pre";
		if (restartCut == PREPARED_CUT_AFTER_REFUND)
			suffix = "full_refund_post_refund";
		else if (restartCut == PREPARED_CUT_AFTER_RECEIPT)
			suffix = "full_refund_post_receipt";
		context.m_Fixture = BuildAdmittedFixture(suffix);
		if (!Ready(context.m_Fixture))
		{
			context.m_sFailure = BuildFixtureFailure(context.m_Fixture);
			return context;
		}
		context.m_iAccepted
			= context.m_Fixture.m_Manifest.m_iAcceptedMemberCount;
		if (context.m_iAccepted <= 0)
		{
			context.m_sFailure = "uncommitted full-refund roster is empty";
			return context;
		}
		context.m_iSurvivors = context.m_iAccepted;
		context.m_iAttackRefund
			= Math.Max(0, context.m_Fixture.m_Order.m_iAttackCost);
		context.m_iSupportRefund
			= Math.Max(0, context.m_Fixture.m_Order.m_iSupportCost);
		context.m_sSettlementKind = "admission_failed_full";
		context.m_sSettlementId = HST_OperationService.BuildSettlementId(
			context.m_Fixture.m_Order.m_sOperationId,
			context.m_sSettlementKind);
		context.m_sRefundMutationId
			= "enemy_resource_refund_" + context.m_sSettlementId;
		context.m_sReason
			= "exact enemy defensive QRF uncommitted full-refund recovery proof";
		HST_FactionPoolState pool = context.m_Fixture.m_State.FindFactionPool(
			PROOF_FACTION_KEY);
		HST_EnemySupportLedgerState ledger
			= context.m_Fixture.m_State.FindEnemySupportLedger(
				PROOF_FACTION_KEY,
				PROOF_TARGET_ZONE_ID);
		if (!pool)
		{
			context.m_sFailure = "uncommitted full-refund pool unavailable";
			return context;
		}
		if (!ledger)
		{
			context.m_sFailure = "uncommitted full-refund ledger unavailable";
			return context;
		}
		CapturePreparedRecoveryResourceBaseline(context, pool, ledger);
		if (!UncommitPreparedFullRefundRuntime(context))
			return context;
		if (!StagePreparedFullRefundCut(context))
			return context;
		BuildPreparedRecoveryExpectation(context, pool, ledger);
		context.m_bPrefixExactBeforeSave = IsPreparedFullRefundPrefixExact(
			context.m_Fixture.m_State,
			context.m_Fixture.m_Order,
			context.m_Fixture.m_Operation,
			pool,
			ledger,
			context);
		return context;
	}

	protected bool UncommitPreparedFullRefundRuntime(
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!context)
			return false;
		if (!context.m_Fixture)
			return false;
		HST_EnemyQRFOperationProofFixture fixture = context.m_Fixture;
		fixture.m_Order.m_bStrategicServiceCommitted = false;
		fixture.m_Order.m_sSpawnResultId = "";
		fixture.m_Order.m_sGroupId = "";
		fixture.m_Operation.m_sSpawnResultId = "";
		fixture.m_Operation.m_sForceId = "";
		fixture.m_Operation.m_sProjectionId = "";
		fixture.m_Operation.m_sGroupId = "";
		if (context.m_bRetainDeterministicResiduals)
		{
			if (fixture.m_Batch.m_iSuccessfulHandoffCount != 0)
			{
				context.m_sFailure = "uncommitted residual batch was already handed off";
				return false;
			}
			if (fixture.m_Group.m_bSpawnedEntity)
			{
				context.m_sFailure = "uncommitted residual group is physically live";
				return false;
			}
			if (fixture.m_Group.m_iSpawnedAgentCount != 0)
			{
				context.m_sFailure = "uncommitted residual group has spawned agents";
				return false;
			}
			if (!fixture.m_Group.m_sRuntimeEntityId.IsEmpty())
			{
				context.m_sFailure = "uncommitted residual group has a runtime entity";
				return false;
			}
			return true;
		}
		int batchIndex = fixture.m_State.m_aForceSpawnResults.Find(fixture.m_Batch);
		if (batchIndex >= 0)
			fixture.m_State.m_aForceSpawnResults.Remove(batchIndex);
		int groupIndex = fixture.m_State.m_aActiveGroups.Find(fixture.m_Group);
		if (groupIndex >= 0)
			fixture.m_State.m_aActiveGroups.Remove(groupIndex);
		return true;
	}

	protected bool StagePreparedFullRefundCut(
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!context)
			return false;
		if (!context.m_Fixture)
			return false;
		HST_EnemyQRFOperationProofFixture fixture = context.m_Fixture;
		fixture.m_State.m_iElapsedSeconds++;
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult prepared
			= operations.PrepareExactEnemyDefensiveQRFSettlement(
				fixture.m_State,
				fixture.m_Order,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				context.m_sSettlementId,
				context.m_sReason);
		if (!prepared)
		{
			context.m_sFailure = "uncommitted full-refund intent returned no result";
			return false;
		}
		if (!prepared.m_bAccepted)
		{
			context.m_sFailure = "uncommitted full-refund intent was rejected";
			return false;
		}
		if (!prepared.m_Operation)
		{
			context.m_sFailure = "uncommitted full-refund intent lost its operation";
			return false;
		}
		fixture.m_Order.m_sResourceSettlementId = context.m_sSettlementId;
		fixture.m_Order.m_sResourceSettlementKind = context.m_sSettlementKind;
		fixture.m_Order.m_sResourceRefundMutationId = context.m_sRefundMutationId;
		fixture.m_Order.m_iSettlementAcceptedMemberCount = context.m_iAccepted;
		fixture.m_Order.m_iSettlementSurvivorMemberCount = context.m_iSurvivors;
		fixture.m_Order.m_iRefundedAttackResources = context.m_iAttackRefund;
		fixture.m_Order.m_iRefundedSupportResources = context.m_iSupportRefund;
		fixture.m_Order.m_bResourceSettlementApplied = false;
		context.m_iPreparedAtSecond = fixture.m_Operation.m_iSettledAtSecond;
		if (context.m_iRestartCut >= PREPARED_CUT_AFTER_REFUND)
		{
			bool refunded = fixture.m_EnemyDirector.RefundDefenseResources(
				fixture.m_State,
				fixture.m_Order.m_sFactionKey,
				fixture.m_Order.m_sTargetZoneId,
				context.m_iAttackRefund,
				context.m_iSupportRefund,
				context.m_sReason,
				context.m_sRefundMutationId,
				context.m_sSettlementId,
				fixture.m_Order.m_sOrderId,
				fixture.m_Order.m_sOperationId,
				fixture.m_Order.m_sManifestId);
			if (!refunded)
			{
				context.m_sFailure = "uncommitted full-refund mutation setup failed";
				return false;
			}
		}
		if (context.m_iRestartCut >= PREPARED_CUT_AFTER_RECEIPT)
		{
			HST_OperationTransitionResult recorded
				= operations.RecordExactEnemyDefensiveQRFResourceSettlement(
					fixture.m_State,
					fixture.m_Order,
					context.m_sSettlementKind,
					context.m_iAccepted,
					context.m_iSurvivors,
					context.m_sRefundMutationId,
					context.m_iAttackRefund,
					context.m_iSupportRefund);
			if (!recorded)
			{
				context.m_sFailure = "uncommitted full-refund receipt returned no result";
				return false;
			}
			if (!recorded.m_bAccepted)
			{
				context.m_sFailure = "uncommitted full-refund receipt was rejected";
				return false;
			}
			if (!fixture.m_Order.m_bResourceSettlementApplied)
			{
				context.m_sFailure = "uncommitted full-refund receipt was not applied";
				return false;
			}
		}
		context.m_iPrefixRevision = fixture.m_Operation.m_iRevision;
		context.m_bExpectedPrefixReceiptApplied
			= context.m_iRestartCut >= PREPARED_CUT_AFTER_RECEIPT;
		if (context.m_iRestartCut >= PREPARED_CUT_AFTER_REFUND)
		{
			context.m_iExpectedPrefixMutationCount = 1;
			context.m_iExpectedPrefixAttackDelta = context.m_iAttackRefund;
			context.m_iExpectedPrefixSupportDelta = context.m_iSupportRefund;
		}
		return true;
	}

	protected bool IsPreparedFullRefundPrefixExact(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_FactionPoolState pool,
		HST_EnemySupportLedgerState ledger,
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!state)
			return false;
		if (!order)
			return false;
		if (!operation)
			return false;
		if (!pool)
			return false;
		if (!ledger)
			return false;
		if (!context)
			return false;
		if (!context.m_Expectation)
			return false;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(state);
		string aggregateFailure
			= HST_EnemyQRFSaveValidationService.ValidatePreparedSaveAuthority(
				saveData,
				order);
		string debitFailure
			= HST_EnemyQRFSaveValidationService.ValidateOriginalResourceDebitAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		string refundFailure
			= HST_EnemyQRFSaveValidationService.ValidatePreparedRefundAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		if (!IsPreparedFullRefundPrefixOperationExact(operation, context))
			return false;
		if (!IsPreparedFullRefundPrefixOrderExact(order, context))
			return false;
		if (!IsPreparedRecoveryPrefixResourceExact(
				state,
				pool,
				ledger,
				context))
			return false;
		if (!IsPreparedFullRefundResidualPrefixExact(state, context))
			return false;
		if (!aggregateFailure.IsEmpty())
			return false;
		if (!debitFailure.IsEmpty())
			return false;
		if (!refundFailure.IsEmpty())
			return false;
		return true;
	}

	protected bool IsPreparedFullRefundPrefixOperationExact(
		HST_OperationRecordState operation,
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!operation)
			return false;
		if (!context)
			return false;
		if (!context.m_Expectation)
			return false;
		if (operation.m_sOperationId != context.m_Expectation.m_sOperationId)
			return false;
		if (operation.m_sEnemyOrderId != context.m_Expectation.m_sOrderId)
			return false;
		if (operation.m_sManifestId != context.m_Expectation.m_sManifestId)
			return false;
		if (!operation.m_sSpawnResultId.IsEmpty())
			return false;
		if (!operation.m_sForceId.IsEmpty())
			return false;
		if (!operation.m_sProjectionId.IsEmpty())
			return false;
		if (!operation.m_sGroupId.IsEmpty())
			return false;
		if (operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
			return false;
		if (operation.m_eTerminalResult
			!= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED)
			return false;
		if (operation.m_sSettlementId != context.m_sSettlementId)
			return false;
		if (operation.m_sTerminalReason != context.m_sReason)
			return false;
		if (operation.m_iSettledAtSecond != context.m_iPreparedAtSecond)
			return false;
		if (context.m_iPreparedAtSecond <= 0)
			return false;
		if (operation.m_iRevision != context.m_iPrefixRevision)
			return false;
		return true;
	}

	protected bool IsPreparedFullRefundPrefixOrderExact(
		HST_EnemyOrderState order,
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!IsPreparedRecoveryPrefixOrderExact(order, context))
			return false;
		if (order.m_bStrategicServiceCommitted)
			return false;
		if (!order.m_sSpawnResultId.IsEmpty())
			return false;
		if (!order.m_sGroupId.IsEmpty())
			return false;
		if (order.m_iSettlementAcceptedMemberCount
			!= order.m_iSettlementSurvivorMemberCount)
			return false;
		if (order.m_iRefundedAttackResources
			!= Math.Max(0, order.m_iAttackCost))
			return false;
		if (order.m_iRefundedSupportResources
			!= Math.Max(0, order.m_iSupportCost))
			return false;
		return true;
	}

	protected bool IsPreparedFullRefundResidualPrefixExact(
		HST_CampaignState state,
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!state)
			return false;
		if (!context)
			return false;
		if (!context.m_Expectation)
			return false;
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(
			context.m_Expectation.m_sBatchId);
		HST_ActiveGroupState group = state.FindActiveGroup(
			context.m_Expectation.m_sGroupId);
		if (!context.m_bRetainDeterministicResiduals)
		{
			if (batch)
				return false;
			if (group)
				return false;
			return true;
		}
		if (!IsPreparedFullRefundRetainedBatchExact(batch, context.m_Expectation))
			return false;
		if (!IsPreparedFullRefundRetainedGroupExact(group, context.m_Expectation))
			return false;
		if (CountBatchId(state, context.m_Expectation.m_sBatchId) != 1)
			return false;
		if (CountGroupId(state, context.m_Expectation.m_sGroupId) != 1)
			return false;
		return true;
	}

	protected bool IsPreparedFullRefundRetainedBatchExact(
		HST_ForceSpawnResultState batch,
		HST_EnemyQRFPreparedRecoveryExpectation expectation)
	{
		if (!batch)
			return false;
		if (!expectation)
			return false;
		if (batch.m_sResultId != expectation.m_sBatchId)
			return false;
		if (batch.m_sRequestId != expectation.m_sOrderId)
			return false;
		if (batch.m_sOperationId != expectation.m_sOperationId)
			return false;
		if (batch.m_sManifestId != expectation.m_sManifestId)
			return false;
		if (batch.m_sManifestHash != expectation.m_sManifestHash)
			return false;
		if (batch.m_sProjectionId != expectation.m_sGroupId)
			return false;
		if (batch.m_sForceId != "force_" + expectation.m_sOperationId)
			return false;
		if (batch.m_iSuccessfulHandoffCount != 0)
			return false;
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			return false;
		return true;
	}

	protected bool IsPreparedFullRefundRetainedGroupExact(
		HST_ActiveGroupState group,
		HST_EnemyQRFPreparedRecoveryExpectation expectation)
	{
		if (!group)
			return false;
		if (!expectation)
			return false;
		if (group.m_sGroupId != expectation.m_sGroupId)
			return false;
		if (group.m_sProjectionId != expectation.m_sGroupId)
			return false;
		if (group.m_sForceId != "force_" + expectation.m_sOperationId)
			return false;
		if (group.m_sSpawnResultId != expectation.m_sBatchId)
			return false;
		if (group.m_sEnemyOrderId != expectation.m_sOrderId)
			return false;
		if (group.m_sOperationId != expectation.m_sOperationId)
			return false;
		if (group.m_sManifestId != expectation.m_sManifestId)
			return false;
		if (group.m_sFactionKey != PROOF_FACTION_KEY)
			return false;
		if (group.m_bSpawnedEntity)
			return false;
		if (group.m_iSpawnedAgentCount != 0)
			return false;
		if (!group.m_sRuntimeEntityId.IsEmpty())
			return false;
		return true;
	}

	protected HST_EnemyQRFPreparedRecoveryRunResult RunPreparedFullRefundRecoveryCut(
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		HST_EnemyQRFPreparedRecoveryRunResult result
			= new HST_EnemyQRFPreparedRecoveryRunResult();
		if (!context)
			return result;
		if (!context.m_Fixture)
			return result;
		if (!context.m_Expectation)
			return result;
		HST_CampaignSaveData interruptedSave = new HST_CampaignSaveData();
		interruptedSave.Capture(context.m_Fixture.m_State);
		HST_CampaignState restored = interruptedSave.Restore();
		result.m_bPrefixSurvived = IsPreparedFullRefundRestoredPrefixExact(
			restored,
			context);
		if (!restored)
			return result;

		HST_ForceSpawnQueueService restoredQueue = new HST_ForceSpawnQueueService();
		restoredQueue.ReconcileCampaignAfterRestore(restored);
		HST_EnemyQRFOperationService restoredQRF
			= new HST_EnemyQRFOperationService();
		restoredQRF.SetRuntimeServices(
			restoredQueue,
			new HST_ForceSpawnAdapterService(),
			new HST_PhysicalWarService());
		HST_EnemyDirectorService restoredDirector = new HST_EnemyDirectorService();
		result.m_bFirstChanged = restoredQRF.ReconcileAfterRestore(
			restored,
			restoredDirector);
		result.m_bFirstResumeExact = result.m_bFirstChanged;
		if (result.m_bFirstResumeExact)
		{
			result.m_bFirstResumeExact = IsPreparedFullRefundTerminalExact(
				restored,
				context.m_Expectation);
		}
		string firstTerminalFingerprint
			= BuildPreparedRecoveryTerminalFingerprint(
				restored,
				context.m_Expectation);
		result.m_bSecondChanged = restoredQRF.ReconcileAfterRestore(
			restored,
			restoredDirector);
		string secondTerminalFingerprint
			= BuildPreparedRecoveryTerminalFingerprint(
				restored,
				context.m_Expectation);
		result.m_bSameStateReplayExact = !result.m_bSecondChanged;
		if (result.m_bSameStateReplayExact)
		{
			result.m_bSameStateReplayExact = IsPreparedFullRefundTerminalExact(
				restored,
				context.m_Expectation);
		}
		if (result.m_bSameStateReplayExact)
		{
			result.m_bSameStateReplayExact
				= secondTerminalFingerprint == firstTerminalFingerprint;
		}
		ProvePreparedFullRefundSecondRestore(
			restored,
			firstTerminalFingerprint,
			context,
			result);
		return result;
	}

	protected bool IsPreparedFullRefundRestoredPrefixExact(
		HST_CampaignState restored,
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!restored)
			return false;
		if (!context)
			return false;
		if (!context.m_Expectation)
			return false;
		HST_EnemyOrderState order = FindOrder(
			restored,
			context.m_Expectation.m_sOrderId);
		HST_OperationRecordState operation = restored.FindOperation(
			context.m_Expectation.m_sOperationId);
		HST_FactionPoolState pool = restored.FindFactionPool(PROOF_FACTION_KEY);
		HST_EnemySupportLedgerState ledger = restored.FindEnemySupportLedger(
			PROOF_FACTION_KEY,
			PROOF_TARGET_ZONE_ID);
		return IsPreparedFullRefundPrefixExact(
			restored,
			order,
			operation,
			pool,
			ledger,
			context);
	}

	protected void ProvePreparedFullRefundSecondRestore(
		HST_CampaignState restored,
		string firstTerminalFingerprint,
		HST_EnemyQRFPreparedRecoveryCutContext context,
		HST_EnemyQRFPreparedRecoveryRunResult result)
	{
		if (!restored)
			return;
		if (!context)
			return;
		if (!context.m_Expectation)
			return;
		if (!result)
			return;
		HST_CampaignSaveData terminalSave = new HST_CampaignSaveData();
		terminalSave.Capture(restored);
		HST_CampaignState secondRestored = terminalSave.Restore();
		if (!secondRestored)
			return;
		HST_ForceSpawnQueueService secondQueue = new HST_ForceSpawnQueueService();
		secondQueue.ReconcileCampaignAfterRestore(secondRestored);
		HST_EnemyQRFOperationService secondQRF
			= new HST_EnemyQRFOperationService();
		secondQRF.SetRuntimeServices(
			secondQueue,
			new HST_ForceSpawnAdapterService(),
			new HST_PhysicalWarService());
		bool secondChanged = secondQRF.ReconcileAfterRestore(
			secondRestored,
			new HST_EnemyDirectorService());
		result.m_bSecondRestoreExact = !secondChanged;
		HST_EnemyOrderState secondOrder = FindOrder(
			secondRestored,
			context.m_Expectation.m_sOrderId);
		HST_OperationRecordState secondOperation = secondRestored.FindOperation(
			context.m_Expectation.m_sOperationId);
		result.m_bSecondReceiptApplied = secondOrder != null;
		if (result.m_bSecondReceiptApplied)
			result.m_bSecondReceiptApplied = secondOrder.m_bResourceSettlementApplied;
		result.m_bSecondOperationPresent = secondOperation != null;
		result.m_iSecondMutationCount = CountStrategicMutations(
			secondRestored,
			context.m_sRefundMutationId);
		if (secondOperation)
			result.m_eSecondSettlementState = secondOperation.m_eSettlementState;
		string secondRestoreFingerprint
			= BuildPreparedRecoveryTerminalFingerprint(
				secondRestored,
				context.m_Expectation);
		if (result.m_bSecondRestoreExact)
		{
			result.m_bSecondRestoreExact = IsPreparedFullRefundTerminalExact(
				secondRestored,
				context.m_Expectation);
		}
		if (result.m_bSecondRestoreExact)
		{
			result.m_bSecondRestoreExact
				= secondRestoreFingerprint == firstTerminalFingerprint;
		}
	}

	protected bool IsPreparedFullRefundTerminalExact(
		HST_CampaignState state,
		HST_EnemyQRFPreparedRecoveryExpectation expectation)
	{
		if (!state)
			return false;
		if (!expectation)
			return false;
		HST_EnemyOrderState order = FindOrder(state, expectation.m_sOrderId);
		HST_OperationRecordState operation = state.FindOperation(
			expectation.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(
			expectation.m_sManifestId);
		HST_FactionPoolState pool = state.FindFactionPool(PROOF_FACTION_KEY);
		HST_EnemySupportLedgerState ledger = state.FindEnemySupportLedger(
			PROOF_FACTION_KEY,
			PROOF_TARGET_ZONE_ID);
		if (!order)
			return false;
		if (!operation)
			return false;
		if (!manifest)
			return false;
		if (!pool)
			return false;
		if (!ledger)
			return false;
		string debitFailure
			= HST_EnemyQRFSaveValidationService.ValidateOriginalResourceDebitAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		string refundFailure
			= HST_EnemyQRFSaveValidationService.ValidateSettledResourceRefundAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		if (!IsPreparedFullRefundTerminalOrderExact(order, expectation))
			return false;
		if (!IsPreparedFullRefundTerminalOperationExact(operation, expectation))
			return false;
		if (!IsPreparedRecoveryTerminalResourceExact(
				state,
				manifest,
				pool,
				ledger,
				expectation))
			return false;
		if (!debitFailure.IsEmpty())
			return false;
		if (!refundFailure.IsEmpty())
			return false;
		if (state.FindForceSpawnResult(expectation.m_sBatchId))
			return false;
		if (state.FindActiveGroup(expectation.m_sGroupId))
			return false;
		return true;
	}

	protected bool IsPreparedFullRefundTerminalOrderExact(
		HST_EnemyOrderState order,
		HST_EnemyQRFPreparedRecoveryExpectation expectation)
	{
		if (!order)
			return false;
		if (!expectation)
			return false;
		if (order.m_sOrderId != expectation.m_sOrderId)
			return false;
		if (order.m_sOperationId != expectation.m_sOperationId)
			return false;
		if (order.m_sManifestId != expectation.m_sManifestId)
			return false;
		if (order.m_sManifestHash != expectation.m_sManifestHash)
			return false;
		if (!order.m_sSpawnResultId.IsEmpty())
			return false;
		if (!order.m_sGroupId.IsEmpty())
			return false;
		if (order.m_iAttackCost != expectation.m_iAttackCost)
			return false;
		if (order.m_iSupportCost != expectation.m_iSupportCost)
			return false;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
			return false;
		if (order.m_sRuntimeStatus != "resolved_exact_terminal")
			return false;
		if (order.m_sResolutionKind != expectation.m_sSettlementKind)
			return false;
		if (order.m_sFailureReason != expectation.m_sReason)
			return false;
		if (order.m_iResolvedAtSecond != expectation.m_iPreparedAtSecond)
			return false;
		if (order.m_bStrategicServiceCommitted)
			return false;
		if (order.m_bPhysicalized)
			return false;
		if (order.m_bAbstractResolved)
			return false;
		if (order.m_bOutcomeApplied)
			return false;
		if (order.m_bResourceRefundApplied)
			return false;
		if (!order.m_bResourceSettlementApplied)
			return false;
		if (order.m_sResourceSettlementId != expectation.m_sSettlementId)
			return false;
		if (order.m_sResourceSettlementKind != expectation.m_sSettlementKind)
			return false;
		if (order.m_sResourceRefundMutationId != expectation.m_sRefundMutationId)
			return false;
		if (order.m_iSettlementAcceptedMemberCount != expectation.m_iAccepted)
			return false;
		if (order.m_iSettlementSurvivorMemberCount != expectation.m_iSurvivors)
			return false;
		if (order.m_iRefundedAttackResources != expectation.m_iAttackRefund)
			return false;
		if (order.m_iRefundedSupportResources != expectation.m_iSupportRefund)
			return false;
		return true;
	}

	protected bool IsPreparedFullRefundTerminalOperationExact(
		HST_OperationRecordState operation,
		HST_EnemyQRFPreparedRecoveryExpectation expectation)
	{
		if (!operation)
			return false;
		if (!expectation)
			return false;
		if (operation.m_sOperationId != expectation.m_sOperationId)
			return false;
		if (operation.m_eType
			!= HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF)
			return false;
		if (operation.m_iContractVersion
			!= HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION)
			return false;
		if (operation.m_sEnemyOrderId != expectation.m_sOrderId)
			return false;
		if (operation.m_sManifestId != expectation.m_sManifestId)
			return false;
		if (!operation.m_sSpawnResultId.IsEmpty())
			return false;
		if (!operation.m_sForceId.IsEmpty())
			return false;
		if (!operation.m_sProjectionId.IsEmpty())
			return false;
		if (!operation.m_sGroupId.IsEmpty())
			return false;
		if (operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return false;
		if (operation.m_eTerminalResult
			!= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED)
			return false;
		if (operation.m_sSettlementId != expectation.m_sSettlementId)
			return false;
		if (operation.m_sTerminalReason != expectation.m_sReason)
			return false;
		if (operation.m_iSettledAtSecond != expectation.m_iPreparedAtSecond)
			return false;
		if (operation.m_iRevision != expectation.m_iExpectedTerminalRevision)
			return false;
		if (operation.m_eDutyState
			!= HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED)
			return false;
		if (operation.m_eEngagementMode
			!= HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
			return false;
		if (operation.m_eMaterializationState
			!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
			return false;
		if (operation.m_ePositionAuthority
			!= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return false;
		if (operation.m_iDutyStateEnteredAtSecond != expectation.m_iPreparedAtSecond)
			return false;
		if (operation.m_iEngagementStateEnteredAtSecond
			!= expectation.m_iPreparedAtSecond)
			return false;
		if (operation.m_iMaterializationStateEnteredAtSecond
			!= expectation.m_iPreparedAtSecond)
			return false;
		if (operation.m_iLastProgressAtSecond != expectation.m_iPreparedAtSecond)
			return false;
		if (operation.m_iArrivalConfirmationCount != 0)
			return false;
		if (operation.m_iLastArrivalConfirmationSecond != 0)
			return false;
		return true;
	}

	protected HST_EnemyQRFPreparedRecoveryProofSummary ProvePreparedSettlementRecovery()
	{
		HST_EnemyQRFPreparedRecoveryProofSummary summary
			= new HST_EnemyQRFPreparedRecoveryProofSummary();
		string dualPreRefundEvidence;
		string supportPreRefundEvidence;
		string dualPostRefundEvidence;
		string supportPostRefundEvidence;
		string dualPostReceiptEvidence;
		string supportPostReceiptEvidence;
		bool dualPreRefundExact = ProvePreparedSettlementRestartCut(
			PREPARED_CUT_BEFORE_REFUND,
			PROOF_ATTACK_COST,
			PROOF_SUPPORT_COST,
			dualPreRefundEvidence);
		bool supportPreRefundExact = ProvePreparedSettlementRestartCut(
			PREPARED_CUT_BEFORE_REFUND,
			0,
			PROOF_SUPPORT_COST,
			supportPreRefundEvidence);
		bool dualPostRefundExact = ProvePreparedSettlementRestartCut(
			PREPARED_CUT_AFTER_REFUND,
			PROOF_ATTACK_COST,
			PROOF_SUPPORT_COST,
			dualPostRefundEvidence);
		bool supportPostRefundExact = ProvePreparedSettlementRestartCut(
			PREPARED_CUT_AFTER_REFUND,
			0,
			PROOF_SUPPORT_COST,
			supportPostRefundEvidence);
		bool dualPostReceiptExact = ProvePreparedSettlementRestartCut(
			PREPARED_CUT_AFTER_RECEIPT,
			PROOF_ATTACK_COST,
			PROOF_SUPPORT_COST,
			dualPostReceiptEvidence);
		bool supportPostReceiptExact = ProvePreparedSettlementRestartCut(
			PREPARED_CUT_AFTER_RECEIPT,
			0,
			PROOF_SUPPORT_COST,
			supportPostReceiptEvidence);
		string fullRefundEvidence;
		bool fullRefundExact = ProvePreparedFullRefundRecoveryMatrix(
			fullRefundEvidence);
		HST_EnemyQRFPreparedRecoveryBoundaryProofSummary boundary
			= ProvePreparedRecoveryBoundaryGuards();
		summary.m_bPreRefundExact = dualPreRefundExact && supportPreRefundExact;
		summary.m_bPostRefundExact = dualPostRefundExact && supportPostRefundExact;
		summary.m_bPostReceiptExact = dualPostReceiptExact && supportPostReceiptExact;
		summary.m_bUncommittedFullRefundExact = fullRefundExact;
		summary.m_bCorruptedBaselineFailClosed
			= boundary.m_bCorruptedBaselineFailClosed;
		summary.m_bCorruptedPoolTailFailClosed
			= boundary.m_bCorruptedPoolTailFailClosed;
		summary.m_bTamperedSurvivorTupleFailClosed
			= boundary.m_bTamperedSurvivorTupleFailClosed;
		summary.m_bSettledPoolTailFailClosed
			= boundary.m_bSettledPoolTailFailClosed;
		summary.m_bHistoricalMutationlessPreserved
			= boundary.m_bHistoricalMutationlessPreserved;
		summary.m_sEvidence = "in-memory dual pre-refund " + dualPreRefundEvidence
			+ " | support-only pre-refund " + supportPreRefundEvidence
			+ " | dual post-refund " + dualPostRefundEvidence
			+ " | support-only post-refund " + supportPostRefundEvidence
			+ " | dual post-receipt " + dualPostReceiptEvidence
			+ " | support-only post-receipt " + supportPostReceiptEvidence
			+ " | uncommitted full-refund " + fullRefundEvidence
			+ " | boundary guards " + boundary.m_sEvidence;
		return summary;
	}

	protected HST_EnemyQRFPreparedRecoveryBoundaryProofSummary ProvePreparedRecoveryBoundaryGuards()
	{
		HST_EnemyQRFPreparedRecoveryBoundaryProofSummary summary
			= new HST_EnemyQRFPreparedRecoveryBoundaryProofSummary();
		string corruptedEvidence;
		string poolTailEvidence;
		string survivorTupleEvidence;
		string settledPoolTailEvidence;
		string historicalEvidence;
		summary.m_bCorruptedBaselineFailClosed
			= ProveCorruptedPreparedBaselineFailClosed(corruptedEvidence);
		summary.m_bCorruptedPoolTailFailClosed
			= ProveCorruptedPreparedPoolTailFailClosed(poolTailEvidence);
		summary.m_bTamperedSurvivorTupleFailClosed
			= ProveTamperedPreparedSurvivorTupleFailClosed(survivorTupleEvidence);
		summary.m_bSettledPoolTailFailClosed
			= ProveCorruptedSettledPoolTailFailClosed(settledPoolTailEvidence);
		summary.m_bHistoricalMutationlessPreserved
			= ProveHistoricalMutationlessSettlementMigration(historicalEvidence);
		summary.m_sEvidence = "corrupted PREPARED " + corruptedEvidence
			+ " | corrupted pool tail " + poolTailEvidence
			+ " | tampered survivor tuple " + survivorTupleEvidence
			+ " | corrupted SETTLED pool tail " + settledPoolTailEvidence
			+ " | mutationless history " + historicalEvidence;
		return summary;
	}

	protected bool ProveCorruptedSettledPoolTailFailClosed(out string evidence)
	{
		evidence = "current-provenance SETTLED pool-tail fixture unavailable";
		HST_EnemyQRFPreparedRecoveryCutContext context
			= BuildPreparedRecoveryCutContext(
				PREPARED_CUT_AFTER_RECEIPT,
				PROOF_ATTACK_COST,
				PROOF_SUPPORT_COST);
		if (!IsPreparedBoundaryContextReady(context, evidence))
			return false;
		HST_CampaignState sourceState = context.m_Fixture.m_State;
		HST_EnemyOrderState sourceOrder = context.m_Fixture.m_Order;
		HST_OperationRecordState sourceOperation = context.m_Fixture.m_Operation;
		HST_FactionPoolState sourcePool = sourceState.FindFactionPool(
			PROOF_FACTION_KEY);
		if (!sourcePool)
		{
			evidence = "current-provenance SETTLED pool is unavailable";
			return false;
		}
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult finalized
			= operations.FinalizePreparedExactEnemyDefensiveQRFSettlement(
				sourceState,
				sourceOrder,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				context.m_sSettlementId,
				context.m_sReason);
		if (!finalized)
		{
			evidence = "current-provenance SETTLED finalization returned no result";
			return false;
		}
		if (!finalized.m_bAccepted)
		{
			evidence = "current-provenance SETTLED finalization was rejected";
			return false;
		}
		if (!finalized.m_bStateChanged)
		{
			evidence = "current-provenance SETTLED finalization made no transition";
			return false;
		}
		if (sourceOperation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			evidence = "current-provenance operation did not settle";
			return false;
		}
		if (sourceOperation.m_iRevision != context.m_iPrefixRevision + 1)
		{
			evidence = "current-provenance SETTLED revision is not exact";
			return false;
		}
		string canonicalTailId = sourcePool.m_sLastStrategicMutationId;
		if (canonicalTailId != context.m_sRefundMutationId)
		{
			evidence = "current-provenance SETTLED pool has no refund tail";
			return false;
		}
		if (CountStrategicMutations(
			sourceState,
			sourceOrder.m_sResourceDebitMutationId) != 1)
		{
			evidence = "current-provenance SETTLED debit is not unique";
			return false;
		}
		if (CountStrategicMutations(
			sourceState,
			context.m_sRefundMutationId) != 1)
		{
			evidence = "current-provenance SETTLED refund is not unique";
			return false;
		}
		sourcePool.m_sLastStrategicMutationId
			= canonicalTailId + "_proof_tail_conflict";
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(sourceState);
		bool currentProvenance
			= HST_EnemyQRFSaveValidationService.HasStrategicReceiptProvenance(
				saveData,
				sourceOrder);
		string earlySettledFailure
			= HST_EnemyQRFSaveValidationService.ValidateSettledSaveAuthority(
				saveData,
				sourceOrder);
		HST_CampaignState restored = saveData.Restore();
		if (!restored)
		{
			evidence = "current-provenance SETTLED pool-tail restore returned no state";
			return false;
		}
		bool restoreExact = IsSettledPoolTailRestoreFailClosed(
			restored,
			context,
			sourceOrder,
			sourceOperation,
			sourcePool);
		string resourceBefore = BuildSettledPoolTailResourceFingerprint(
			restored,
			context);
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		queue.ReconcileCampaignAfterRestore(restored);
		HST_EnemyQRFOperationService exactQRF = new HST_EnemyQRFOperationService();
		exactQRF.SetRuntimeServices(
			queue,
			new HST_ForceSpawnAdapterService(),
			new HST_PhysicalWarService());
		HST_EnemyDirectorService enemyDirector = new HST_EnemyDirectorService();
		bool firstChanged = exactQRF.ReconcileAfterRestore(
			restored,
			enemyDirector);
		bool firstQuarantineExact = IsSettledPoolTailRuntimeQuarantineExact(
			restored,
			context,
			sourceOrder,
			sourceOperation,
			sourcePool,
			resourceBefore);
		string firstFingerprint = BuildSettledPoolTailQuarantineFingerprint(
			restored,
			context);
		bool secondChanged = exactQRF.ReconcileAfterRestore(
			restored,
			enemyDirector);
		bool sameStateReplayExact = IsSettledPoolTailRuntimeQuarantineExact(
			restored,
			context,
			sourceOrder,
			sourceOperation,
			sourcePool,
			resourceBefore);
		string secondFingerprint = BuildSettledPoolTailQuarantineFingerprint(
			restored,
			context);
		if (sameStateReplayExact)
			sameStateReplayExact = !secondChanged;
		if (sameStateReplayExact)
			sameStateReplayExact = secondFingerprint == firstFingerprint;
		string secondRestoreEvidence;
		bool secondRestoreExact = ProveSettledPoolTailSecondRestoreStable(
			restored,
			firstFingerprint,
			resourceBefore,
			context,
			sourceOrder,
			sourceOperation,
			sourcePool,
			secondRestoreEvidence);
		evidence = string.Format(
			"provenance/early-valid %1/%2 | restore/reconcile/replay %3/%4/%5/%6",
			currentProvenance,
			earlySettledFailure.IsEmpty(),
			restoreExact,
			firstChanged,
			firstQuarantineExact,
			sameStateReplayExact);
		evidence = evidence + string.Format(
			" | mutations debit/refund/total %1/%2/%3 | tail %4 -> %5",
			CountStrategicMutations(restored, sourceOrder.m_sResourceDebitMutationId),
			CountStrategicMutations(restored, context.m_sRefundMutationId),
			restored.m_aEnemyStrategicMutations.Count(),
			canonicalTailId,
			sourcePool.m_sLastStrategicMutationId);
		evidence = evidence + " | second restore " + secondRestoreEvidence;
		if (!currentProvenance)
			return false;
		if (!earlySettledFailure.IsEmpty())
			return false;
		if (!restoreExact)
			return false;
		if (!firstChanged)
			return false;
		if (!firstQuarantineExact)
			return false;
		if (!sameStateReplayExact)
			return false;
		return secondRestoreExact;
	}

	protected bool IsSettledPoolTailRestoreFailClosed(
		HST_CampaignState restored,
		HST_EnemyQRFPreparedRecoveryCutContext context,
		HST_EnemyOrderState sourceOrder,
		HST_OperationRecordState sourceOperation,
		HST_FactionPoolState sourcePool)
	{
		if (!restored)
			return false;
		if (!context)
			return false;
		if (!context.m_Expectation)
			return false;
		if (!sourceOrder)
			return false;
		if (!sourceOperation)
			return false;
		if (!sourcePool)
			return false;
		if (restored.m_iSchemaVersion != HST_CampaignState.SCHEMA_VERSION)
			return false;
		if (restored.m_iLastLoadedSchemaVersion != HST_CampaignState.SCHEMA_VERSION)
			return false;
		HST_EnemyOrderState order = FindOrder(restored, sourceOrder.m_sOrderId);
		HST_OperationRecordState operation = restored.FindOperation(
			sourceOperation.m_sOperationId);
		HST_ForceManifestState manifest = restored.FindForceManifest(
			context.m_Expectation.m_sManifestId);
		HST_ForceSpawnResultState batch = restored.FindForceSpawnResult(
			context.m_Expectation.m_sBatchId);
		HST_ActiveGroupState group = restored.FindActiveGroup(
			context.m_Expectation.m_sGroupId);
		HST_FactionPoolState pool = restored.FindFactionPool(PROOF_FACTION_KEY);
		HST_EnemySupportLedgerState ledger = restored.FindEnemySupportLedger(
			PROOF_FACTION_KEY,
			PROOF_TARGET_ZONE_ID);
		if (!order)
			return false;
		if (!operation)
			return false;
		if (!manifest)
			return false;
		if (!batch)
			return false;
		if (!group)
			return false;
		if (!pool)
			return false;
		if (!ledger)
			return false;
		if (!IsSettledPoolTailAppliedTuplePreserved(order, sourceOrder))
			return false;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
			return false;
		if (order.m_sRuntimeStatus != "exact_operation_invalidated")
			return false;
		if (order.m_sFailureReason
			!= "exact enemy defensive QRF original debit is missing or ambiguous")
			return false;
		if (order.m_bPhysicalized)
			return false;
		if (order.m_iResolvedAtSecond != sourceOrder.m_iResolvedAtSecond)
			return false;
		if (!IsSettledPoolTailDisarmedOperationExact(operation, sourceOperation))
			return false;
		if (!IsSettledPoolTailManifestExact(manifest, context.m_Expectation))
			return false;
		if (!IsSettledPoolTailHeldRuntimeExact(
				batch,
				group,
				order,
				operation,
				context.m_Expectation))
			return false;
		if (!IsSettledPoolTailQuarantinedPoolExact(pool, sourcePool))
			return false;
		if (restored.m_aEnemyStrategicMutations.Count() != 0)
			return false;
		if (CountStrategicMutations(
			restored,
			sourceOrder.m_sResourceDebitMutationId) != 0)
			return false;
		if (CountStrategicMutations(
			restored,
			context.m_sRefundMutationId) != 0)
			return false;
		return true;
	}

	protected bool IsSettledPoolTailRuntimeQuarantineExact(
		HST_CampaignState state,
		HST_EnemyQRFPreparedRecoveryCutContext context,
		HST_EnemyOrderState sourceOrder,
		HST_OperationRecordState sourceOperation,
		HST_FactionPoolState sourcePool,
		string expectedResourceFingerprint)
	{
		if (!state)
			return false;
		if (!context)
			return false;
		if (!context.m_Expectation)
			return false;
		HST_EnemyOrderState order = FindOrder(state, sourceOrder.m_sOrderId);
		HST_OperationRecordState operation = state.FindOperation(
			sourceOperation.m_sOperationId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(
			context.m_Expectation.m_sBatchId);
		HST_ActiveGroupState group = state.FindActiveGroup(
			context.m_Expectation.m_sGroupId);
		HST_FactionPoolState pool = state.FindFactionPool(PROOF_FACTION_KEY);
		HST_EnemySupportLedgerState ledger = state.FindEnemySupportLedger(
			PROOF_FACTION_KEY,
			PROOF_TARGET_ZONE_ID);
		if (!order)
			return false;
		if (!operation)
			return false;
		if (!batch)
			return false;
		if (!group)
			return false;
		if (!pool)
			return false;
		if (!ledger)
			return false;
		if (!IsSettledPoolTailAppliedTuplePreserved(order, sourceOrder))
			return false;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
			return false;
		if (order.m_sRuntimeStatus
			!= "exact_restore_resource_authority_quarantined")
			return false;
		if (order.m_sFailureReason
			!= "exact enemy defensive QRF restore retained a rejected strategic-resource settlement")
			return false;
		if (order.m_bPhysicalized)
			return false;
		if (order.m_iResolvedAtSecond != sourceOrder.m_iResolvedAtSecond)
			return false;
		if (!IsSettledPoolTailDisarmedOperationExact(operation, sourceOperation))
			return false;
		if (!IsSettledPoolTailHeldRuntimeExact(
				batch,
				group,
				order,
				operation,
				context.m_Expectation))
			return false;
		if (!IsSettledPoolTailQuarantinedPoolExact(pool, sourcePool))
			return false;
		if (BuildSettledPoolTailResourceFingerprint(state, context)
			!= expectedResourceFingerprint)
			return false;
		return true;
	}

	protected bool ProveSettledPoolTailSecondRestoreStable(
		HST_CampaignState state,
		string expectedFingerprint,
		string expectedResourceFingerprint,
		HST_EnemyQRFPreparedRecoveryCutContext context,
		HST_EnemyOrderState sourceOrder,
		HST_OperationRecordState sourceOperation,
		HST_FactionPoolState sourcePool,
		out string evidence)
	{
		evidence = "unavailable";
		if (!state)
			return false;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(state);
		HST_CampaignState restored = saveData.Restore();
		if (!restored)
			return false;
		bool restoreExact = IsSettledPoolTailRuntimeQuarantineExact(
			restored,
			context,
			sourceOrder,
			sourceOperation,
			sourcePool,
			expectedResourceFingerprint);
		string restoreFingerprint = BuildSettledPoolTailQuarantineFingerprint(
			restored,
			context);
		if (restoreExact)
			restoreExact = restoreFingerprint == expectedFingerprint;
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		queue.ReconcileCampaignAfterRestore(restored);
		HST_EnemyQRFOperationService exactQRF = new HST_EnemyQRFOperationService();
		exactQRF.SetRuntimeServices(
			queue,
			new HST_ForceSpawnAdapterService(),
			new HST_PhysicalWarService());
		bool reconcileChanged = exactQRF.ReconcileAfterRestore(
			restored,
			new HST_EnemyDirectorService());
		bool reconcileExact = IsSettledPoolTailRuntimeQuarantineExact(
			restored,
			context,
			sourceOrder,
			sourceOperation,
			sourcePool,
			expectedResourceFingerprint);
		string reconcileFingerprint = BuildSettledPoolTailQuarantineFingerprint(
			restored,
			context);
		if (reconcileExact)
			reconcileExact = !reconcileChanged;
		if (reconcileExact)
			reconcileExact = reconcileFingerprint == expectedFingerprint;
		evidence = string.Format(
			"restore/reconcile/stable %1/%2/%3",
			restoreExact,
			reconcileChanged,
			reconcileExact);
		if (!restoreExact)
			return false;
		return reconcileExact;
	}

	protected bool IsSettledPoolTailAppliedTuplePreserved(
		HST_EnemyOrderState order,
		HST_EnemyOrderState sourceOrder)
	{
		if (!order)
			return false;
		if (!sourceOrder)
			return false;
		if (order.m_sOrderId != sourceOrder.m_sOrderId)
			return false;
		if (order.m_sOperationId != sourceOrder.m_sOperationId)
			return false;
		if (order.m_sManifestId != sourceOrder.m_sManifestId)
			return false;
		if (order.m_sManifestHash != sourceOrder.m_sManifestHash)
			return false;
		if (order.m_sSpawnResultId != sourceOrder.m_sSpawnResultId)
			return false;
		if (order.m_sGroupId != sourceOrder.m_sGroupId)
			return false;
		if (order.m_bStrategicServiceCommitted
			!= sourceOrder.m_bStrategicServiceCommitted)
			return false;
		if (order.m_iAttackCost != sourceOrder.m_iAttackCost)
			return false;
		if (order.m_iSupportCost != sourceOrder.m_iSupportCost)
			return false;
		if (order.m_sResourceDebitMutationId
			!= sourceOrder.m_sResourceDebitMutationId)
			return false;
		if (!order.m_bResourceSettlementApplied)
			return false;
		if (order.m_bResourceSettlementApplied
			!= sourceOrder.m_bResourceSettlementApplied)
			return false;
		if (order.m_sResourceSettlementId
			!= sourceOrder.m_sResourceSettlementId)
			return false;
		if (order.m_sResourceSettlementKind
			!= sourceOrder.m_sResourceSettlementKind)
			return false;
		if (order.m_sResourceRefundMutationId
			!= sourceOrder.m_sResourceRefundMutationId)
			return false;
		if (order.m_iSettlementAcceptedMemberCount
			!= sourceOrder.m_iSettlementAcceptedMemberCount)
			return false;
		if (order.m_iSettlementSurvivorMemberCount
			!= sourceOrder.m_iSettlementSurvivorMemberCount)
			return false;
		if (order.m_iRefundedAttackResources
			!= sourceOrder.m_iRefundedAttackResources)
			return false;
		if (order.m_iRefundedSupportResources
			!= sourceOrder.m_iRefundedSupportResources)
			return false;
		return true;
	}

	protected bool IsSettledPoolTailDisarmedOperationExact(
		HST_OperationRecordState operation,
		HST_OperationRecordState sourceOperation)
	{
		if (!operation)
			return false;
		if (!sourceOperation)
			return false;
		if (operation.m_sOperationId != sourceOperation.m_sOperationId)
			return false;
		if (operation.m_sEnemyOrderId != sourceOperation.m_sEnemyOrderId)
			return false;
		if (operation.m_sManifestId != sourceOperation.m_sManifestId)
			return false;
		if (operation.m_sSpawnResultId != sourceOperation.m_sSpawnResultId)
			return false;
		if (operation.m_sForceId != sourceOperation.m_sForceId)
			return false;
		if (operation.m_sProjectionId != sourceOperation.m_sProjectionId)
			return false;
		if (operation.m_sGroupId != sourceOperation.m_sGroupId)
			return false;
		if (operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_UNKNOWN)
			return false;
		if (operation.m_eTerminalResult != sourceOperation.m_eTerminalResult)
			return false;
		if (operation.m_sSettlementId != sourceOperation.m_sSettlementId)
			return false;
		if (operation.m_sTerminalReason != sourceOperation.m_sTerminalReason)
			return false;
		if (operation.m_iSettledAtSecond != sourceOperation.m_iSettledAtSecond)
			return false;
		if (operation.m_iRevision != sourceOperation.m_iRevision)
			return false;
		if (operation.m_eDutyState != sourceOperation.m_eDutyState)
			return false;
		if (operation.m_eEngagementMode != sourceOperation.m_eEngagementMode)
			return false;
		if (operation.m_eMaterializationState
			!= sourceOperation.m_eMaterializationState)
			return false;
		if (operation.m_ePositionAuthority != sourceOperation.m_ePositionAuthority)
			return false;
		if (operation.m_iDutyStateEnteredAtSecond
			!= sourceOperation.m_iDutyStateEnteredAtSecond)
			return false;
		if (operation.m_iEngagementStateEnteredAtSecond
			!= sourceOperation.m_iEngagementStateEnteredAtSecond)
			return false;
		if (operation.m_iMaterializationStateEnteredAtSecond
			!= sourceOperation.m_iMaterializationStateEnteredAtSecond)
			return false;
		if (operation.m_iLastProgressAtSecond
			!= sourceOperation.m_iLastProgressAtSecond)
			return false;
		if (operation.m_iArrivalConfirmationCount
			!= sourceOperation.m_iArrivalConfirmationCount)
			return false;
		if (operation.m_iLastArrivalConfirmationSecond
			!= sourceOperation.m_iLastArrivalConfirmationSecond)
			return false;
		return true;
	}

	protected bool IsSettledPoolTailManifestExact(
		HST_ForceManifestState manifest,
		HST_EnemyQRFPreparedRecoveryExpectation expectation)
	{
		if (!manifest)
			return false;
		if (!expectation)
			return false;
		if (manifest.m_sManifestId != expectation.m_sManifestId)
			return false;
		if (manifest.m_sOperationId != expectation.m_sOperationId)
			return false;
		if (manifest.m_sManifestHash != expectation.m_sManifestHash)
			return false;
		if (manifest.m_iAcceptedMemberCount != expectation.m_iAccepted)
			return false;
		if (!manifest.m_bFrozen)
			return false;
		return true;
	}

	protected bool IsSettledPoolTailHeldRuntimeExact(
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_EnemyQRFPreparedRecoveryExpectation expectation)
	{
		if (!batch)
			return false;
		if (!group)
			return false;
		if (!order)
			return false;
		if (!operation)
			return false;
		if (!expectation)
			return false;
		if (batch.m_sResultId != expectation.m_sBatchId)
			return false;
		if (batch.m_sRequestId != order.m_sOrderId)
			return false;
		if (batch.m_sOperationId != order.m_sOperationId)
			return false;
		if (batch.m_sManifestId != order.m_sManifestId)
			return false;
		if (batch.m_sManifestHash != order.m_sManifestHash)
			return false;
		if (batch.m_sForceId != operation.m_sForceId)
			return false;
		if (batch.m_sProjectionId != operation.m_sProjectionId)
			return false;
		if (!batch.m_bCancelRequested)
			return false;
		if (!batch.m_bStrategicProjectionHeld)
			return false;
		if (group.m_sGroupId != expectation.m_sGroupId)
			return false;
		if (group.m_sProjectionId != operation.m_sProjectionId)
			return false;
		if (group.m_sForceId != operation.m_sForceId)
			return false;
		if (group.m_sSpawnResultId != batch.m_sResultId)
			return false;
		if (group.m_sEnemyOrderId != order.m_sOrderId)
			return false;
		if (group.m_sOperationId != order.m_sOperationId)
			return false;
		if (group.m_sManifestId != order.m_sManifestId)
			return false;
		if (group.m_sFactionKey != order.m_sFactionKey)
			return false;
		return true;
	}

	protected bool IsSettledPoolTailQuarantinedPoolExact(
		HST_FactionPoolState pool,
		HST_FactionPoolState sourcePool)
	{
		if (!pool)
			return false;
		if (!sourcePool)
			return false;
		if (pool.m_sFactionKey != sourcePool.m_sFactionKey)
			return false;
		if (pool.m_iStrategicContractVersion
			!= HST_EnemyStrategicResourceSaveValidationService.QUARANTINE_CONTRACT_VERSION)
			return false;
		if (pool.m_iStrategicRevision != sourcePool.m_iStrategicRevision)
			return false;
		if (pool.m_iStrategicOperationalMutationCount
			!= sourcePool.m_iStrategicOperationalMutationCount)
			return false;
		if (pool.m_iAttackResources != sourcePool.m_iAttackResources)
			return false;
		if (pool.m_iSupportResources != sourcePool.m_iSupportResources)
			return false;
		if (pool.m_iAggression != sourcePool.m_iAggression)
			return false;
		if (pool.m_sLastStrategicMutationId
			!= sourcePool.m_sLastStrategicMutationId)
			return false;
		if (pool.m_sStrategicAuthorityFailure
			!= "schema67 strategic pool projection diverged from its latest receipt")
			return false;
		return true;
	}

	protected string BuildSettledPoolTailResourceFingerprint(
		HST_CampaignState state,
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!state)
			return "settled-pool-tail-resource-unavailable";
		if (!context)
			return "settled-pool-tail-resource-unavailable";
		HST_FactionPoolState pool = state.FindFactionPool(PROOF_FACTION_KEY);
		HST_EnemySupportLedgerState ledger = state.FindEnemySupportLedger(
			PROOF_FACTION_KEY,
			PROOF_TARGET_ZONE_ID);
		if (!pool)
			return "settled-pool-tail-resource-unavailable";
		if (!ledger)
			return "settled-pool-tail-resource-unavailable";
		string fingerprint = BuildStrategicPoolFingerprint(pool);
		fingerprint = fingerprint + string.Format(
			" | ledger %1/%2/%3/%4/%5/%6/%7/%8/%9",
			ledger.m_sFactionKey,
			ledger.m_sZoneId,
			ledger.m_iAttackSpent,
			ledger.m_iSupportSpent,
			ledger.m_iRefundedAttackResources,
			ledger.m_iRefundedSupportResources,
			ledger.m_iLastSpendSecond,
			ledger.m_iCooldownUntilSecond,
			ledger.m_sLastDecisionReason);
		fingerprint = fingerprint + string.Format(
			" | mutations %1/%2/%3",
			state.m_aEnemyStrategicMutations.Count(),
			CountStrategicMutations(
				state,
				context.m_Fixture.m_Order.m_sResourceDebitMutationId),
			CountStrategicMutations(state, context.m_sRefundMutationId));
		return fingerprint;
	}

	protected string BuildSettledPoolTailQuarantineFingerprint(
		HST_CampaignState state,
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!state)
			return "settled-pool-tail-quarantine-unavailable";
		if (!context)
			return "settled-pool-tail-quarantine-unavailable";
		if (!context.m_Expectation)
			return "settled-pool-tail-quarantine-unavailable";
		HST_EnemyOrderState order = FindOrder(
			state,
			context.m_Expectation.m_sOrderId);
		HST_OperationRecordState operation = state.FindOperation(
			context.m_Expectation.m_sOperationId);
		if (!order)
			return "settled-pool-tail-quarantine-incomplete";
		if (!operation)
			return "settled-pool-tail-quarantine-incomplete";
		string fingerprint = string.Format(
			"order %1/%2/%3/%4/%5/%6/%7/%8/%9",
			order.m_sOrderId,
			order.m_sOperationId,
			order.m_sManifestId,
			order.m_sSpawnResultId,
			order.m_sGroupId,
			order.m_eStatus,
			order.m_sRuntimeStatus,
			order.m_sFailureReason,
			order.m_bStrategicServiceCommitted);
		fingerprint = fingerprint + string.Format(
			" | receipt %1/%2/%3/%4/%5/%6/%7/%8",
			order.m_bResourceSettlementApplied,
			order.m_sResourceSettlementId,
			order.m_sResourceSettlementKind,
			order.m_sResourceRefundMutationId,
			order.m_iSettlementAcceptedMemberCount,
			order.m_iSettlementSurvivorMemberCount,
			order.m_iRefundedAttackResources,
			order.m_iRefundedSupportResources);
		fingerprint = fingerprint + string.Format(
			" | operation %1/%2/%3/%4/%5/%6/%7/%8/%9",
			operation.m_sOperationId,
			operation.m_sEnemyOrderId,
			operation.m_sManifestId,
			operation.m_sSpawnResultId,
			operation.m_sForceId,
			operation.m_sProjectionId,
			operation.m_sGroupId,
			operation.m_eSettlementState,
			operation.m_eTerminalResult);
		fingerprint = fingerprint + string.Format(
			" | terminal %1/%2/%3/%4/%5/%6/%7",
			operation.m_sSettlementId,
			operation.m_sTerminalReason,
			operation.m_iSettledAtSecond,
			operation.m_iRevision,
			operation.m_eDutyState,
			operation.m_eMaterializationState,
			operation.m_ePositionAuthority);
		fingerprint = fingerprint + string.Format(
			" | runtime %1/%2 | %3",
			state.FindForceSpawnResult(context.m_Expectation.m_sBatchId) != null,
			state.FindActiveGroup(context.m_Expectation.m_sGroupId) != null,
			BuildSettledPoolTailResourceFingerprint(state, context));
		return fingerprint;
	}

	protected bool ProveCorruptedPreparedPoolTailFailClosed(out string evidence)
	{
		HST_EnemyQRFPreparedRecoveryCutContext context
			= BuildPreparedRecoveryCutContext(
				PREPARED_CUT_BEFORE_REFUND,
				PROOF_ATTACK_COST,
				PROOF_SUPPORT_COST);
		if (!IsPreparedBoundaryContextReady(context, evidence))
			return false;
		HST_CampaignState sourceState = context.m_Fixture.m_State;
		HST_EnemyOrderState sourceOrder = context.m_Fixture.m_Order;
		HST_FactionPoolState sourcePool = sourceState.FindFactionPool(
			PROOF_FACTION_KEY);
		if (!sourcePool)
		{
			evidence = "PREPARED pool-tail fixture has no owning faction pool";
			return false;
		}
		if (sourcePool.m_sLastStrategicMutationId.IsEmpty())
		{
			evidence = "PREPARED pool-tail fixture has no canonical chain tail";
			return false;
		}

		string canonicalTailId = sourcePool.m_sLastStrategicMutationId;
		sourcePool.m_sLastStrategicMutationId
			= canonicalTailId + "_proof_tail_conflict";
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(sourceState);
		string earlyPreparedFailure
			= HST_EnemyQRFSaveValidationService.ValidatePreparedSaveAuthority(
				saveData,
				sourceOrder);
		string debitFailure
			= HST_EnemyQRFSaveValidationService.ValidateOriginalResourceDebitAuthority(
				saveData.m_aEnemyStrategicMutations,
				sourceOrder);
		string restoreEvidence;
		bool restoreExact = ProvePreparedBoundaryRestoreNoRefund(
			saveData,
			sourceOrder.m_sOrderId,
			sourceOrder.m_sOperationId,
			context.m_sRefundMutationId,
			"original debit is missing or ambiguous",
			true,
			"projection diverged from its latest receipt",
			0,
			restoreEvidence);
		evidence = string.Format(
			"schema %1 | early/debit valid %2/%3 | tail %4 -> %5",
			saveData.m_iSchemaVersion,
			earlyPreparedFailure.IsEmpty(),
			debitFailure.IsEmpty(),
			canonicalTailId,
			sourcePool.m_sLastStrategicMutationId);
		evidence = evidence + " | " + restoreEvidence;
		if (saveData.m_iSchemaVersion != HST_CampaignState.SCHEMA_VERSION)
			return false;
		if (!earlyPreparedFailure.IsEmpty())
			return false;
		if (!debitFailure.IsEmpty())
			return false;
		return restoreExact;
	}

	protected bool ProveTamperedPreparedSurvivorTupleFailClosed(out string evidence)
	{
		HST_EnemyQRFPreparedRecoveryCutContext context
			= BuildPreparedRecoveryCutContext(
				PREPARED_CUT_BEFORE_REFUND,
				PROOF_ATTACK_COST,
				PROOF_SUPPORT_COST);
		if (!IsPreparedBoundaryContextReady(context, evidence))
			return false;
		HST_EnemyOrderState sourceOrder = context.m_Fixture.m_Order;
		HST_ForceSpawnResultState sourceBatch = context.m_Fixture.m_Batch;
		if (!sourceOrder.m_bStrategicServiceCommitted)
		{
			evidence = "PREPARED survivor fixture is not committed";
			return false;
		}
		int durableSurvivors
			= context.m_Fixture.m_Queue.CountStrategicLivingMemberSlots(sourceBatch);
		if (durableSurvivors != context.m_iSurvivors)
		{
			evidence = "PREPARED survivor fixture durable roster drifted before tamper";
			return false;
		}
		int tamperedSurvivors = context.m_iAccepted;
		if (tamperedSurvivors == durableSurvivors)
		{
			evidence = "PREPARED survivor fixture has no distinct bounded tamper value";
			return false;
		}

		sourceOrder.m_iSettlementSurvivorMemberCount = tamperedSurvivors;
		sourceOrder.m_iRefundedAttackResources
			= Math.Max(0, sourceOrder.m_iAttackCost)
				* tamperedSurvivors / context.m_iAccepted;
		sourceOrder.m_iRefundedSupportResources
			= Math.Max(0, sourceOrder.m_iSupportCost)
				* tamperedSurvivors / context.m_iAccepted;
		bool tupleInternallyConsistent = IsTamperedPreparedTupleInternallyConsistent(
			sourceOrder,
			context.m_iAccepted,
			tamperedSurvivors);
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(context.m_Fixture.m_State);
		string validationFailure
			= HST_EnemyQRFSaveValidationService.ValidatePreparedSaveAuthority(
				saveData,
				sourceOrder);
		string restoreEvidence;
		bool restoreExact = ProvePreparedBoundaryRestoreNoRefund(
			saveData,
			sourceOrder.m_sOrderId,
			sourceOrder.m_sOperationId,
			context.m_sRefundMutationId,
			"survivor receipt diverges from durable runtime authority",
			false,
			"",
			1,
			restoreEvidence);
		evidence = string.Format(
			"committed/durable/staged %1/%2/%3 | refunds %4/%5 | tuple exact %6",
			sourceOrder.m_bStrategicServiceCommitted,
			durableSurvivors,
			tamperedSurvivors,
			sourceOrder.m_iRefundedAttackResources,
			sourceOrder.m_iRefundedSupportResources,
			tupleInternallyConsistent);
		evidence = evidence + " | validator " + validationFailure
			+ " | " + restoreEvidence;
		if (!tupleInternallyConsistent)
			return false;
		if (!validationFailure.Contains(
			"survivor receipt diverges from durable runtime authority"))
			return false;
		return restoreExact;
	}

	protected bool IsPreparedBoundaryContextReady(
		HST_EnemyQRFPreparedRecoveryCutContext context,
		out string evidence)
	{
		evidence = "";
		if (!context)
		{
			evidence = "PREPARED boundary fixture unavailable";
			return false;
		}
		if (!context.m_sFailure.IsEmpty())
		{
			evidence = context.m_sFailure;
			return false;
		}
		if (!context.m_bPrefixExactBeforeSave)
		{
			evidence = "PREPARED boundary fixture prefix is not exact";
			return false;
		}
		return true;
	}

	protected bool IsTamperedPreparedTupleInternallyConsistent(
		HST_EnemyOrderState order,
		int accepted,
		int stagedSurvivors)
	{
		if (!order)
			return false;
		if (accepted <= 0)
			return false;
		if (stagedSurvivors < 0)
			return false;
		if (stagedSurvivors > accepted)
			return false;
		if (order.m_iSettlementAcceptedMemberCount != accepted)
			return false;
		if (order.m_iSettlementSurvivorMemberCount != stagedSurvivors)
			return false;
		int expectedAttack = Math.Max(0, order.m_iAttackCost)
			* stagedSurvivors / accepted;
		int expectedSupport = Math.Max(0, order.m_iSupportCost)
			* stagedSurvivors / accepted;
		if (order.m_iRefundedAttackResources != expectedAttack)
			return false;
		if (order.m_iRefundedSupportResources != expectedSupport)
			return false;
		return true;
	}

	protected bool ProvePreparedBoundaryRestoreNoRefund(
		HST_CampaignSaveData saveData,
		string orderId,
		string operationId,
		string refundMutationId,
		string expectedOrderFailure,
		bool expectPoolQuarantine,
		string expectedPoolFailure,
		int expectedMutationCount,
		out string evidence)
	{
		evidence = "PREPARED boundary restore unavailable";
		if (!saveData)
			return false;
		HST_CampaignState restored = saveData.Restore();
		if (!restored)
			return false;
		HST_EnemyOrderState order = FindOrder(restored, orderId);
		HST_OperationRecordState operation = restored.FindOperation(operationId);
		HST_ForceSpawnResultState batch;
		if (order)
			batch = restored.FindForceSpawnResult(order.m_sSpawnResultId);
		HST_FactionPoolState pool = restored.FindFactionPool(PROOF_FACTION_KEY);
		if (!order || !operation || !batch || !pool)
		{
			evidence = "PREPARED boundary aggregate was not retained for diagnosis";
			return false;
		}
		bool disarmedAndHeld = IsPreparedRestoreDisarmedAndHeld(
			order,
			operation,
			batch,
			expectedOrderFailure);
		bool poolStateExact = IsPreparedBoundaryPoolStateExact(
			pool,
			expectPoolQuarantine,
			expectedPoolFailure);
		string restoreFailure = order.m_sFailureReason;
		string poolFailure = pool.m_sStrategicAuthorityFailure;
		string poolBefore = BuildStrategicPoolFingerprint(pool);
		int mutationCountBefore = restored.m_aEnemyStrategicMutations.Count();
		int refundCountBefore = CountStrategicMutations(
			restored,
			refundMutationId);
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		queue.ReconcileCampaignAfterRestore(restored);
		HST_EnemyQRFOperationService exactQRF = new HST_EnemyQRFOperationService();
		exactQRF.SetRuntimeServices(
			queue,
			new HST_ForceSpawnAdapterService(),
			new HST_PhysicalWarService());
		bool reconcileChanged = exactQRF.ReconcileAfterRestore(
			restored,
			new HST_EnemyDirectorService());
		string poolAfter = BuildStrategicPoolFingerprint(
			restored.FindFactionPool(PROOF_FACTION_KEY));
		bool reconcileSideEffectFree = IsCorruptedPreparedReconcileSideEffectFree(
			restored,
			order,
			operation,
			refundMutationId,
			refundCountBefore,
			mutationCountBefore,
			poolBefore,
			poolAfter);
		evidence = string.Format(
			"disarmed/cancel/held/pool %1/%2/%3/%4 | mutations %5/%6 | reconcile changed/stable %7/%8",
			disarmedAndHeld,
			batch.m_bCancelRequested,
			batch.m_bStrategicProjectionHeld,
			poolStateExact,
			mutationCountBefore,
			expectedMutationCount,
			reconcileChanged,
			reconcileSideEffectFree);
		evidence = evidence + " | restore failure " + restoreFailure
			+ " | pool failure " + poolFailure;
		if (!disarmedAndHeld)
			return false;
		if (!poolStateExact)
			return false;
		if (mutationCountBefore != expectedMutationCount)
			return false;
		if (refundCountBefore != 0)
			return false;
		return reconcileSideEffectFree;
	}

	protected bool IsPreparedRestoreDisarmedAndHeld(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		string expectedFailure)
	{
		if (!order)
			return false;
		if (!operation)
			return false;
		if (!batch)
			return false;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
			return false;
		if (order.m_sRuntimeStatus != "exact_operation_invalidated")
			return false;
		if (order.m_bPhysicalized)
			return false;
		if (order.m_bResourceSettlementApplied)
			return false;
		if (!order.m_sFailureReason.Contains(expectedFailure))
			return false;
		if (operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_UNKNOWN)
			return false;
		if (!batch.m_bCancelRequested)
			return false;
		if (!batch.m_bStrategicProjectionHeld)
			return false;
		return true;
	}

	protected bool IsPreparedBoundaryPoolStateExact(
		HST_FactionPoolState pool,
		bool expectQuarantine,
		string expectedFailure)
	{
		if (!pool)
			return false;
		if (expectQuarantine)
		{
			if (pool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.CONTRACT_VERSION)
				return false;
			if (pool.m_sStrategicAuthorityFailure.IsEmpty())
				return false;
			if (!expectedFailure.IsEmpty())
			{
				if (!pool.m_sStrategicAuthorityFailure.Contains(expectedFailure))
					return false;
			}
			return true;
		}
		if (pool.m_iStrategicContractVersion
			!= HST_EnemyStrategicResourceService.CONTRACT_VERSION)
			return false;
		if (!pool.m_sStrategicAuthorityFailure.IsEmpty())
			return false;
		return true;
	}

	protected bool ProveCorruptedPreparedBaselineFailClosed(out string evidence)
	{
		HST_EnemyQRFPreparedRecoveryCutContext context
			= BuildPreparedRecoveryCutContext(
				PREPARED_CUT_BEFORE_REFUND,
				PROOF_ATTACK_COST,
				PROOF_SUPPORT_COST);
		if (!context || !context.m_sFailure.IsEmpty()
			|| !context.m_bPrefixExactBeforeSave)
		{
			evidence = "valid PREPARED corruption fixture unavailable";
			if (context && !context.m_sFailure.IsEmpty())
				evidence = context.m_sFailure;
			return false;
		}

		HST_EnemyOrderState sourceOrder = context.m_Fixture.m_Order;
		HST_EnemyStrategicMutationState sourceDebit
			= context.m_Fixture.m_State.FindEnemyStrategicMutation(
				sourceOrder.m_sResourceDebitMutationId);
		if (!sourceDebit)
		{
			evidence = "PREPARED original debit unavailable for corruption";
			return false;
		}
		// Corrupt only the persisted pre-debit baseline. Do not recompute the
		// arithmetic or fingerprint: the current-schema row must fail closed.
		sourceDebit.m_iAttackBefore++;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(context.m_Fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		if (!restored)
		{
			evidence = "corrupted PREPARED restore returned no state";
			return false;
		}

		HST_EnemyOrderState order = FindOrder(restored, sourceOrder.m_sOrderId);
		HST_OperationRecordState operation = restored.FindOperation(
			sourceOrder.m_sOperationId);
		HST_FactionPoolState pool = restored.FindFactionPool(PROOF_FACTION_KEY);
		if (!order || !operation || !pool)
		{
			evidence = "corrupted PREPARED aggregate was not retained for diagnosis";
			return false;
		}
		bool restoredFailClosed = IsCorruptedPreparedRestoreFailClosed(
			order,
			operation,
			pool);

		string poolBefore = BuildStrategicPoolFingerprint(pool);
		int mutationCountBefore = restored.m_aEnemyStrategicMutations.Count();
		int refundCountBefore = CountStrategicMutations(
			restored,
			context.m_sRefundMutationId);
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		queue.ReconcileCampaignAfterRestore(restored);
		HST_EnemyQRFOperationService exactQRF = new HST_EnemyQRFOperationService();
		exactQRF.SetRuntimeServices(
			queue,
			new HST_ForceSpawnAdapterService(),
			new HST_PhysicalWarService());
		bool reconcileChanged = exactQRF.ReconcileAfterRestore(
			restored,
			new HST_EnemyDirectorService());
		string poolAfter = BuildStrategicPoolFingerprint(
			restored.FindFactionPool(PROOF_FACTION_KEY));
		bool reconcileSideEffectFree = IsCorruptedPreparedReconcileSideEffectFree(
			restored,
			order,
			operation,
			context.m_sRefundMutationId,
			refundCountBefore,
			mutationCountBefore,
			poolBefore,
			poolAfter);
		evidence = string.Format(
			"restore aborted/disarmed %1 | pool contract/revision/count %2/%3/%4 | mutations/refund before %5/%6 | reconcile changed/no-resource-side-effect %7/%8",
			restoredFailClosed,
			pool.m_iStrategicContractVersion,
			pool.m_iStrategicRevision,
			pool.m_iStrategicOperationalMutationCount,
			mutationCountBefore,
			refundCountBefore,
			reconcileChanged,
			reconcileSideEffectFree);
		if (!restoredFailClosed)
			return false;
		return reconcileSideEffectFree;
	}

	protected bool IsCorruptedPreparedRestoreFailClosed(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_FactionPoolState pool)
	{
		if (!order)
			return false;
		if (!operation)
			return false;
		if (!pool)
			return false;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
			return false;
		if (order.m_sRuntimeStatus != "exact_operation_invalidated")
			return false;
		if (order.m_bPhysicalized)
			return false;
		if (!order.m_sFailureReason.Contains("original debit shape conflicts"))
			return false;
		if (order.m_bResourceSettlementApplied)
			return false;
		if (operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_UNKNOWN)
			return false;
		if (pool.m_iStrategicContractVersion
			== HST_EnemyStrategicResourceService.CONTRACT_VERSION)
			return false;
		if (pool.m_sStrategicAuthorityFailure.IsEmpty())
			return false;
		return true;
	}

	protected bool IsCorruptedPreparedReconcileSideEffectFree(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		string refundMutationId,
		int refundCountBefore,
		int mutationCountBefore,
		string poolBefore,
		string poolAfter)
	{
		if (!state)
			return false;
		if (!order)
			return false;
		if (!operation)
			return false;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
			return false;
		if (order.m_bPhysicalized)
			return false;
		if (order.m_bResourceSettlementApplied)
			return false;
		if (operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_UNKNOWN)
			return false;
		if (CountStrategicMutations(state, refundMutationId) != refundCountBefore)
			return false;
		if (refundCountBefore != 0)
			return false;
		if (state.m_aEnemyStrategicMutations.Count() != mutationCountBefore)
			return false;
		if (poolAfter != poolBefore)
			return false;
		return true;
	}

	protected string BuildStrategicPoolFingerprint(HST_FactionPoolState pool)
	{
		if (!pool)
			return "strategic-pool-unavailable";
		return string.Format(
			"%1/%2/%3/%4/%5/%6/%7/%8",
			pool.m_sFactionKey,
			pool.m_iStrategicContractVersion,
			pool.m_iStrategicRevision,
			pool.m_iStrategicOperationalMutationCount,
			pool.m_iAttackResources,
			pool.m_iSupportResources,
			pool.m_sLastStrategicMutationId,
			pool.m_sStrategicAuthorityFailure);
	}

	protected bool ProveHistoricalMutationlessSettlementMigration(out string evidence)
	{
		HST_EnemyQRFHistoricalMutationlessContext context
			= BuildHistoricalMutationlessContext();
		if (!context || !context.m_sFailure.IsEmpty() || !context.m_LegacySave)
		{
			evidence = "mutationless historical fixture unavailable";
			if (context && !context.m_sFailure.IsEmpty())
				evidence = context.m_sFailure;
			return false;
		}

		HST_CampaignState firstRestored = context.m_LegacySave.Restore();
		bool firstExact = IsHistoricalMutationlessRestoreExact(
			firstRestored,
			context,
			66);
		string firstFingerprint = BuildHistoricalMutationlessFingerprint(
			firstRestored,
			context);
		HST_CampaignSaveData currentSave = new HST_CampaignSaveData();
		if (firstRestored)
			currentSave.Capture(firstRestored);
		HST_EnemyOrderState firstOrder;
		if (firstRestored)
			firstOrder = FindOrder(firstRestored, context.m_sOrderId);
		bool firstNoProvenance = firstOrder
			&& !HST_EnemyQRFSaveValidationService.HasStrategicReceiptProvenance(
				currentSave,
				firstOrder);
		HST_CampaignState secondRestored = currentSave.Restore();
		bool secondExact = IsHistoricalMutationlessRestoreExact(
			secondRestored,
			context,
			HST_CampaignState.SCHEMA_VERSION);
		string secondFingerprint = BuildHistoricalMutationlessFingerprint(
			secondRestored,
			context);
		HST_CampaignSaveData secondSave = new HST_CampaignSaveData();
		if (secondRestored)
			secondSave.Capture(secondRestored);
		HST_EnemyOrderState secondOrder;
		if (secondRestored)
			secondOrder = FindOrder(secondRestored, context.m_sOrderId);
		bool secondNoProvenance = secondOrder
			&& !HST_EnemyQRFSaveValidationService.HasStrategicReceiptProvenance(
				secondSave,
				secondOrder);
		bool fingerprintExact = firstFingerprint == secondFingerprint
			&& firstFingerprint != "mutationless-history-unavailable";
		evidence = string.Format(
			"schema loaded %1/%2 | exact %3/%4 | mutation counts %5/%6 | no provenance %7/%8 | stable %9",
			firstRestored && firstRestored.m_iLastLoadedSchemaVersion,
			secondRestored && secondRestored.m_iLastLoadedSchemaVersion,
			firstExact,
			secondExact,
			firstRestored && firstRestored.m_aEnemyStrategicMutations.Count(),
			secondRestored && secondRestored.m_aEnemyStrategicMutations.Count(),
			firstNoProvenance,
			secondNoProvenance,
			fingerprintExact);
		if (!firstExact)
			return false;
		if (!secondExact)
			return false;
		if (!firstNoProvenance)
			return false;
		if (!secondNoProvenance)
			return false;
		if (!fingerprintExact)
			return false;
		return true;
	}

	protected HST_EnemyQRFHistoricalMutationlessContext BuildHistoricalMutationlessContext()
	{
		HST_EnemyQRFHistoricalMutationlessContext context
			= new HST_EnemyQRFHistoricalMutationlessContext();
		HST_EnemyQRFOperationProofFixture fixture
			= BuildAdmittedFixture("historical_mutationless_settled");
		if (!Ready(fixture))
		{
			context.m_sFailure = BuildFixtureFailure(fixture);
			return context;
		}
		string reason = "historical mutationless exact QRF proof stop";
		bool settled = fixture.m_ExactQRF.SettleTrackedOpenOrderForAdministrativeStop(
			fixture.m_State,
			fixture.m_EnemyDirector,
			fixture.m_Order,
			reason);
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			PROOF_FACTION_KEY);
		if (!settled || !pool || !fixture.m_Order.m_bResourceSettlementApplied
			|| fixture.m_Operation.m_eSettlementState
				!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			context.m_sFailure = "legitimate settled exact QRF fixture failed";
			return context;
		}

		context.m_sOrderId = fixture.m_Order.m_sOrderId;
		context.m_sOperationId = fixture.m_Order.m_sOperationId;
		context.m_sManifestId = fixture.m_Order.m_sManifestId;
		context.m_sManifestHash = fixture.m_Order.m_sManifestHash;
		context.m_sSettlementId = fixture.m_Order.m_sResourceSettlementId;
		context.m_sSettlementKind = fixture.m_Order.m_sResourceSettlementKind;
		context.m_sTerminalReason = fixture.m_Operation.m_sTerminalReason;
		context.m_iAccepted = fixture.m_Order.m_iSettlementAcceptedMemberCount;
		context.m_iSurvivors = fixture.m_Order.m_iSettlementSurvivorMemberCount;
		context.m_iAttackRefund = fixture.m_Order.m_iRefundedAttackResources;
		context.m_iSupportRefund = fixture.m_Order.m_iRefundedSupportResources;
		context.m_iResolvedAtSecond = fixture.m_Order.m_iResolvedAtSecond;
		context.m_iSettledAtSecond = fixture.m_Operation.m_iSettledAtSecond;
		context.m_iOperationRevision = fixture.m_Operation.m_iRevision;
		context.m_iAttackPool = pool.m_iAttackResources;
		context.m_iSupportPool = pool.m_iSupportResources;

		// Schema 66 predates strategic mutation receipts. Preserve the legitimate
		// terminal aggregate and pool snapshot without inventing debit/refund rows.
		fixture.m_State.m_aEnemyStrategicMutations.Clear();
		fixture.m_Order.m_sResourceDebitMutationId = "";
		fixture.m_Order.m_sResourceRefundMutationId = "";
		pool.m_iStrategicContractVersion = 0;
		pool.m_iStrategicRevision = 0;
		pool.m_iStrategicOperationalMutationCount = 0;
		pool.m_iResourceAccumulatorSeconds = 0;
		pool.m_iAggressionAccumulatorSeconds = 0;
		pool.m_iLastResourceBucketSecond = 0;
		pool.m_iLastAggressionBucketSecond = 0;
		pool.m_sLastStrategicMutationId = "";
		pool.m_sStrategicAuthorityFailure = "";
		fixture.m_State.m_iSchemaVersion = 66;
		fixture.m_State.m_iLastLoadedSchemaVersion = 66;
		context.m_LegacySave = new HST_CampaignSaveData();
		context.m_LegacySave.Capture(fixture.m_State);
		return context;
	}

	protected bool IsHistoricalMutationlessRestoreExact(
		HST_CampaignState state,
		HST_EnemyQRFHistoricalMutationlessContext context,
		int expectedLoadedSchema)
	{
		if (!state)
			return false;
		if (!context)
			return false;
		HST_EnemyOrderState order = FindOrder(state, context.m_sOrderId);
		HST_OperationRecordState operation = state.FindOperation(
			context.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(
			context.m_sManifestId);
		HST_FactionPoolState pool = state.FindFactionPool(PROOF_FACTION_KEY);
		if (state.m_iSchemaVersion != HST_CampaignState.SCHEMA_VERSION)
			return false;
		if (state.m_iLastLoadedSchemaVersion != expectedLoadedSchema)
			return false;
		if (!IsHistoricalMutationlessOrderExact(order, context))
			return false;
		if (!IsHistoricalMutationlessOperationExact(operation, context))
			return false;
		if (!IsHistoricalMutationlessResourceExact(
			state,
			manifest,
			pool,
			context))
			return false;
		return true;
	}

	protected bool IsHistoricalMutationlessOrderExact(
		HST_EnemyOrderState order,
		HST_EnemyQRFHistoricalMutationlessContext context)
	{
		if (!order)
			return false;
		if (!context)
			return false;
		if (order.m_sOperationId != context.m_sOperationId)
			return false;
		if (order.m_sManifestId != context.m_sManifestId)
			return false;
		if (order.m_sManifestHash != context.m_sManifestHash)
			return false;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
			return false;
		if (order.m_sRuntimeStatus != "resolved_exact_terminal")
			return false;
		if (order.m_sResourceSettlementId != context.m_sSettlementId)
			return false;
		if (order.m_sResourceSettlementKind != context.m_sSettlementKind)
			return false;
		if (!order.m_sResourceDebitMutationId.IsEmpty())
			return false;
		if (!order.m_sResourceRefundMutationId.IsEmpty())
			return false;
		if (!order.m_bResourceSettlementApplied)
			return false;
		if (order.m_bResourceRefundApplied)
			return false;
		if (order.m_iSettlementAcceptedMemberCount != context.m_iAccepted)
			return false;
		if (order.m_iSettlementSurvivorMemberCount != context.m_iSurvivors)
			return false;
		if (order.m_iRefundedAttackResources != context.m_iAttackRefund)
			return false;
		if (order.m_iRefundedSupportResources != context.m_iSupportRefund)
			return false;
		if (order.m_iResolvedAtSecond != context.m_iResolvedAtSecond)
			return false;
		return true;
	}

	protected bool IsHistoricalMutationlessOperationExact(
		HST_OperationRecordState operation,
		HST_EnemyQRFHistoricalMutationlessContext context)
	{
		if (!operation)
			return false;
		if (!context)
			return false;
		if (operation.m_sOperationId != context.m_sOperationId)
			return false;
		if (operation.m_sEnemyOrderId != context.m_sOrderId)
			return false;
		if (operation.m_sManifestId != context.m_sManifestId)
			return false;
		if (operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return false;
		if (operation.m_eTerminalResult
			!= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED)
			return false;
		if (operation.m_sSettlementId != context.m_sSettlementId)
			return false;
		if (operation.m_sTerminalReason != context.m_sTerminalReason)
			return false;
		if (operation.m_iSettledAtSecond != context.m_iSettledAtSecond)
			return false;
		if (operation.m_iRevision != context.m_iOperationRevision)
			return false;
		if (operation.m_eDutyState
			!= HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED)
			return false;
		if (operation.m_eMaterializationState
			!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
			return false;
		return true;
	}

	protected bool IsHistoricalMutationlessResourceExact(
		HST_CampaignState state,
		HST_ForceManifestState manifest,
		HST_FactionPoolState pool,
		HST_EnemyQRFHistoricalMutationlessContext context)
	{
		if (!state)
			return false;
		if (!manifest)
			return false;
		if (!pool)
			return false;
		if (!context)
			return false;
		if (!manifest.m_bFrozen)
			return false;
		if (manifest.m_sOperationId != context.m_sOperationId)
			return false;
		if (manifest.m_sManifestHash != context.m_sManifestHash)
			return false;
		if (manifest.m_iAcceptedMemberCount != context.m_iAccepted)
			return false;
		if (state.m_aEnemyStrategicMutations.Count() != 0)
			return false;
		if (pool.m_iAttackResources != context.m_iAttackPool)
			return false;
		if (pool.m_iSupportResources != context.m_iSupportPool)
			return false;
		if (pool.m_iStrategicContractVersion != 0)
			return false;
		if (pool.m_iStrategicRevision != 0)
			return false;
		if (pool.m_iStrategicOperationalMutationCount != 0)
			return false;
		if (!pool.m_sLastStrategicMutationId.IsEmpty())
			return false;
		if (!pool.m_sStrategicAuthorityFailure.IsEmpty())
			return false;
		return true;
	}

	protected string BuildHistoricalMutationlessFingerprint(
		HST_CampaignState state,
		HST_EnemyQRFHistoricalMutationlessContext context)
	{
		if (!state || !context)
			return "mutationless-history-unavailable";
		HST_EnemyOrderState order = FindOrder(state, context.m_sOrderId);
		HST_OperationRecordState operation = state.FindOperation(
			context.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(
			context.m_sManifestId);
		HST_FactionPoolState pool = state.FindFactionPool(PROOF_FACTION_KEY);
		if (!order || !operation || !manifest || !pool)
			return "mutationless-history-unavailable";
		string fingerprint = string.Format(
			"order %1/%2/%3/%4/%5/%6/%7/%8/%9",
			order.m_sOrderId,
			order.m_sOperationId,
			order.m_sManifestId,
			order.m_sManifestHash,
			order.m_eStatus,
			order.m_sRuntimeStatus,
			order.m_sResourceSettlementId,
			order.m_sResourceSettlementKind,
			order.m_iResolvedAtSecond);
		fingerprint += string.Format(
			" | receipt %1/%2/%3/%4/%5/%6/%7/%8",
			order.m_sResourceDebitMutationId,
			order.m_sResourceRefundMutationId,
			order.m_bResourceSettlementApplied,
			order.m_iSettlementAcceptedMemberCount,
			order.m_iSettlementSurvivorMemberCount,
			order.m_iRefundedAttackResources,
			order.m_iRefundedSupportResources,
			order.m_bResourceRefundApplied);
		fingerprint += string.Format(
			" | operation %1/%2/%3/%4/%5/%6/%7/%8",
			operation.m_sOperationId,
			operation.m_sEnemyOrderId,
			operation.m_sManifestId,
			operation.m_eSettlementState,
			operation.m_eTerminalResult,
			operation.m_sSettlementId,
			operation.m_iSettledAtSecond,
			operation.m_iRevision);
		fingerprint += string.Format(
			" | manifest %1/%2/%3/%4 | pool %5 | mutations %6",
			manifest.m_sManifestId,
			manifest.m_sOperationId,
			manifest.m_sManifestHash,
			manifest.m_iAcceptedMemberCount,
			BuildStrategicPoolFingerprint(pool),
			state.m_aEnemyStrategicMutations.Count());
		return fingerprint;
	}

	protected HST_EnemyQRFExternalRestartCarrier BuildExternalRestartCarrier(
		string runId,
		string world,
		string cutName,
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!context || !context.m_Fixture || !context.m_Expectation)
			return null;
		HST_EnemyQRFExternalRestartCarrier carrier
			= new HST_EnemyQRFExternalRestartCarrier();
		carrier.m_sMagic = HST_EnemyQRFExternalRestartProofService.CARRIER_MAGIC;
		carrier.m_sRunId = runId;
		carrier.m_sBuildSha = HST_BuildInfo.BUILD_SHA;
		carrier.m_sBuildUtc = HST_BuildInfo.BUILD_UTC;
		carrier.m_sBuildLabel = HST_BuildInfo.BUILD_LABEL;
		carrier.m_iCampaignSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		carrier.m_sWorld = world;
		carrier.m_sCutName = cutName;
		carrier.m_iCut = context.m_iRestartCut;
		carrier.m_iAccepted = context.m_iAccepted;
		carrier.m_iCasualties = context.m_iCasualties;
		carrier.m_iSurvivors = context.m_iSurvivors;
		carrier.m_iAttackRefund = context.m_iAttackRefund;
		carrier.m_iSupportRefund = context.m_iSupportRefund;
		carrier.m_iAttackBeforeRefund = context.m_iAttackBeforeRefund;
		carrier.m_iSupportBeforeRefund = context.m_iSupportBeforeRefund;
		carrier.m_iLedgerAttackSpentBeforeRefund
			= context.m_iLedgerAttackSpentBeforeRefund;
		carrier.m_iLedgerSupportSpentBeforeRefund
			= context.m_iLedgerSupportSpentBeforeRefund;
		carrier.m_iLedgerAttackRefundedBefore
			= context.m_iLedgerAttackRefundedBefore;
		carrier.m_iLedgerSupportRefundedBefore
			= context.m_iLedgerSupportRefundedBefore;
		carrier.m_iPreparedAtSecond = context.m_iPreparedAtSecond;
		carrier.m_iPrefixRevision = context.m_iPrefixRevision;
		carrier.m_iExpectedPrefixMutationCount
			= context.m_iExpectedPrefixMutationCount;
		carrier.m_iExpectedPrefixAttackDelta
			= context.m_iExpectedPrefixAttackDelta;
		carrier.m_iExpectedPrefixSupportDelta
			= context.m_iExpectedPrefixSupportDelta;
		carrier.m_bExpectedPrefixReceiptApplied
			= context.m_bExpectedPrefixReceiptApplied;
		carrier.m_sSettlementKind = context.m_sSettlementKind;
		carrier.m_sSettlementId = context.m_sSettlementId;
		carrier.m_sRefundMutationId = context.m_sRefundMutationId;
		carrier.m_sReason = context.m_sReason;
		carrier.m_Expectation = context.m_Expectation;
		carrier.m_sPreparedFingerprint = BuildPreparedRecoveryPrefixFingerprint(
			context.m_Fixture.m_State,
			context.m_Expectation,
			context);
		return carrier;
	}

	protected HST_EnemyQRFPreparedRecoveryCutContext BuildExternalRestartCutContext(
		HST_EnemyQRFExternalRestartCarrier carrier)
	{
		if (!carrier)
			return null;
		HST_EnemyQRFPreparedRecoveryCutContext context
			= new HST_EnemyQRFPreparedRecoveryCutContext();
		context.m_Expectation = carrier.m_Expectation;
		context.m_iRestartCut = carrier.m_iCut;
		context.m_iAccepted = carrier.m_iAccepted;
		context.m_iCasualties = carrier.m_iCasualties;
		context.m_iSurvivors = carrier.m_iSurvivors;
		context.m_iAttackRefund = carrier.m_iAttackRefund;
		context.m_iSupportRefund = carrier.m_iSupportRefund;
		context.m_iAttackBeforeRefund = carrier.m_iAttackBeforeRefund;
		context.m_iSupportBeforeRefund = carrier.m_iSupportBeforeRefund;
		context.m_iLedgerAttackSpentBeforeRefund
			= carrier.m_iLedgerAttackSpentBeforeRefund;
		context.m_iLedgerSupportSpentBeforeRefund
			= carrier.m_iLedgerSupportSpentBeforeRefund;
		context.m_iLedgerAttackRefundedBefore
			= carrier.m_iLedgerAttackRefundedBefore;
		context.m_iLedgerSupportRefundedBefore
			= carrier.m_iLedgerSupportRefundedBefore;
		context.m_iPreparedAtSecond = carrier.m_iPreparedAtSecond;
		context.m_iPrefixRevision = carrier.m_iPrefixRevision;
		context.m_iExpectedPrefixMutationCount
			= carrier.m_iExpectedPrefixMutationCount;
		context.m_iExpectedPrefixAttackDelta
			= carrier.m_iExpectedPrefixAttackDelta;
		context.m_iExpectedPrefixSupportDelta
			= carrier.m_iExpectedPrefixSupportDelta;
		context.m_bExpectedPrefixReceiptApplied
			= carrier.m_bExpectedPrefixReceiptApplied;
		context.m_sSettlementKind = carrier.m_sSettlementKind;
		context.m_sSettlementId = carrier.m_sSettlementId;
		context.m_sRefundMutationId = carrier.m_sRefundMutationId;
		context.m_sReason = carrier.m_sReason;
		return context;
	}

	protected string BuildPreparedRecoveryPrefixFingerprint(
		HST_CampaignState state,
		HST_EnemyQRFPreparedRecoveryExpectation expectation,
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!state || !expectation || !context)
			return "prepared-recovery-prefix-unavailable";
		HST_EnemyOrderState order = FindOrder(state, expectation.m_sOrderId);
		HST_OperationRecordState operation
			= state.FindOperation(expectation.m_sOperationId);
		HST_ForceManifestState manifest
			= state.FindForceManifest(expectation.m_sManifestId);
		HST_FactionPoolState pool = state.FindFactionPool(PROOF_FACTION_KEY);
		HST_EnemySupportLedgerState ledger = state.FindEnemySupportLedger(
			PROOF_FACTION_KEY,
			PROOF_TARGET_ZONE_ID);
		HST_ForceSpawnResultState batch
			= state.FindForceSpawnResult(expectation.m_sBatchId);
		HST_ActiveGroupState group = state.FindActiveGroup(expectation.m_sGroupId);
		if (!order || !operation || !manifest || !pool || !ledger
			|| !batch || !group)
			return "prepared-recovery-prefix-aggregate-incomplete";

		string fingerprint = string.Format(
			"state %1/%2/%3 | cut %4/%5/%6/%7/%8/%9",
			state.m_iSchemaVersion,
			state.m_iLastLoadedSchemaVersion,
			state.m_iElapsedSeconds,
			context.m_iRestartCut,
			context.m_iExpectedPrefixMutationCount,
			context.m_iExpectedPrefixAttackDelta,
			context.m_iExpectedPrefixSupportDelta,
			context.m_bExpectedPrefixReceiptApplied,
			context.m_iPrefixRevision);
		fingerprint += string.Format(
			" | order %1/%2/%3/%4/%5/%6/%7/%8/%9",
			order.m_sOrderId,
			order.m_sOperationId,
			order.m_sManifestId,
			order.m_sManifestHash,
			order.m_sSpawnResultId,
			order.m_sGroupId,
			order.m_eStatus,
			order.m_sRuntimeStatus,
			order.m_sResourceDebitMutationId);
		fingerprint += string.Format(
			" | receipt %1/%2/%3/%4/%5/%6/%7/%8/%9",
			order.m_sResourceSettlementId,
			order.m_sResourceSettlementKind,
			order.m_sResourceRefundMutationId,
			order.m_bResourceSettlementApplied,
			order.m_iSettlementAcceptedMemberCount,
			order.m_iSettlementSurvivorMemberCount,
			order.m_iRefundedAttackResources,
			order.m_iRefundedSupportResources,
			order.m_bResourceRefundApplied);
		fingerprint += string.Format(
			" | operation %1/%2/%3/%4/%5/%6/%7/%8/%9",
			operation.m_sOperationId,
			operation.m_sEnemyOrderId,
			operation.m_sManifestId,
			operation.m_sSpawnResultId,
			operation.m_sGroupId,
			operation.m_eSettlementState,
			operation.m_eTerminalResult,
			operation.m_sSettlementId,
			operation.m_sTerminalReason);
		fingerprint += string.Format(
			" | operation time %1/%2/%3/%4/%5/%6",
			operation.m_iSettledAtSecond,
			operation.m_iRevision,
			operation.m_eDutyState,
			operation.m_eEngagementMode,
			operation.m_eMaterializationState,
			operation.m_ePositionAuthority);
		fingerprint += string.Format(
			" | manifest %1/%2/%3/%4/%5 | pool %6/%7/%8/%9",
			manifest.m_sManifestId,
			manifest.m_sOperationId,
			manifest.m_sManifestHash,
			manifest.m_iAcceptedMemberCount,
			manifest.m_bFrozen,
			pool.m_iAttackResources,
			pool.m_iSupportResources,
			pool.m_iStrategicRevision,
			pool.m_iStrategicOperationalMutationCount);
		fingerprint += string.Format(
			"/%1 | ledger %2/%3/%4/%5/%6/%7/%8/%9",
			pool.m_sLastStrategicMutationId,
			ledger.m_sFactionKey,
			ledger.m_sZoneId,
			ledger.m_iAttackSpent,
			ledger.m_iSupportSpent,
			ledger.m_iRefundedAttackResources,
			ledger.m_iRefundedSupportResources,
			ledger.m_iLastSpendSecond,
			ledger.m_iCooldownUntilSecond);
		fingerprint += string.Format(
			" | ledger tail %1/%2/%3 | batch %4/%5/%6/%7/%8/%9",
			ledger.m_sLastDecisionReason,
			ledger.m_iRecentDamageScore,
			ledger.m_iLastDamageSecond,
			batch.m_sResultId,
			batch.m_sManifestId,
			batch.m_sOperationId,
			batch.m_eStatus,
			batch.m_iExpectedSlotCount,
			batch.m_aSlotResults.Count());
		fingerprint += string.Format(
			" | group %1/%2/%3/%4/%5/%6/%7/%8/%9",
			group.m_sGroupId,
			group.m_sOperationId,
			group.m_sManifestId,
			group.m_sSpawnResultId,
			group.m_sEnemyOrderId,
			group.m_iOriginalInfantryCount,
			group.m_iDurableLivingInfantryCount,
			group.m_iSurvivorInfantryCount,
			group.m_sRuntimeStatus);
		HST_EnemyStrategicMutationState debit
			= state.FindEnemyStrategicMutation(order.m_sResourceDebitMutationId);
		HST_EnemyStrategicMutationState refund
			= state.FindEnemyStrategicMutation(context.m_sRefundMutationId);
		fingerprint += " | debit " + BuildExternalMutationFingerprint(debit);
		fingerprint += " | refund " + BuildExternalMutationFingerprint(refund);
		fingerprint += string.Format(
			" | mutation counts %1/%2",
			CountStrategicMutations(state, order.m_sResourceDebitMutationId),
			CountStrategicMutations(state, context.m_sRefundMutationId));
		return fingerprint;
	}

	protected string BuildExternalMutationFingerprint(
		HST_EnemyStrategicMutationState mutation)
	{
		if (!mutation)
			return "none";
		string fingerprint = string.Format(
			"%1/%2/%3/%4/%5/%6/%7/%8/%9",
			mutation.m_sMutationId,
			mutation.m_sFactionKey,
			mutation.m_sKind,
			mutation.m_sSourceId,
			mutation.m_sOrderId,
			mutation.m_sOperationId,
			mutation.m_sManifestId,
			mutation.m_sZoneId,
			mutation.m_bApplied);
		fingerprint += string.Format(
			"/%1/%2/%3/%4/%5/%6/%7/%8/%9",
			mutation.m_iCreatedAtSecond,
			mutation.m_iPoolRevisionBefore,
			mutation.m_iPoolRevisionAfter,
			mutation.m_iOperationalSequence,
			mutation.m_iAttackBefore,
			mutation.m_iAttackDelta,
			mutation.m_iAttackAfter,
			mutation.m_iSupportBefore,
			mutation.m_iSupportDelta);
		fingerprint += string.Format(
			"/%1/%2/%3/%4/%5/%6",
			mutation.m_iSupportAfter,
			mutation.m_iAggressionBefore,
			mutation.m_iAggressionDelta,
			mutation.m_iAggressionAfter,
			mutation.m_sContributionHash,
			mutation.m_sFingerprint);
		return fingerprint;
	}

	// This is deliberately an in-memory save-data capture/restore proof. External
	// process-restart certification remains a separate runtime gate.
	protected bool ProvePreparedSettlementRestartCut(
		int restartCut,
		int attackCost,
		int supportCost,
		out string evidence)
	{
		HST_EnemyQRFPreparedRecoveryCutContext context
			= BuildPreparedRecoveryCutContext(restartCut, attackCost, supportCost);
		if (!context || !context.m_sFailure.IsEmpty())
		{
			evidence = "prepared recovery cut context unavailable";
			if (context)
				evidence = context.m_sFailure;
			return false;
		}
		HST_EnemyQRFPreparedRecoveryRunResult result
			= RunPreparedRecoveryCut(context);
		evidence = BuildPreparedRecoveryEvidence(context, result);
		if (!context.m_bPrefixExactBeforeSave)
			return false;
		if (!result)
			return false;
		return result.AllExact();
	}

	protected HST_EnemyQRFPreparedRecoveryCutContext BuildPreparedRecoveryCutContext(
		int restartCut,
		int attackCost,
		int supportCost)
	{
		HST_EnemyQRFPreparedRecoveryCutContext context
			= new HST_EnemyQRFPreparedRecoveryCutContext();
		context.m_iRestartCut = restartCut;
		if (restartCut < PREPARED_CUT_BEFORE_REFUND
			|| restartCut > PREPARED_CUT_AFTER_RECEIPT)
		{
			context.m_sFailure = "unsupported in-memory settlement cut";
			return context;
		}
		if (attackCost < 0 || supportCost <= 0)
		{
			context.m_sFailure = "invalid in-memory defense-resource fixture cost";
			return context;
		}

		string resourceLabel = "dual";
		if (attackCost == 0)
			resourceLabel = "support_only";
		string fixtureSuffix = resourceLabel + "_prepared_pre_refund";
		if (restartCut == PREPARED_CUT_AFTER_REFUND)
			fixtureSuffix = resourceLabel + "_prepared_post_refund";
		else if (restartCut == PREPARED_CUT_AFTER_RECEIPT)
			fixtureSuffix = resourceLabel + "_prepared_post_receipt";
		context.m_Fixture = BuildAdmittedFixture(
			fixtureSuffix,
			attackCost,
			supportCost);
		if (!Ready(context.m_Fixture))
		{
			context.m_sFailure = BuildFixtureFailure(context.m_Fixture);
			return context;
		}

		context.m_iAccepted = context.m_Fixture.m_Manifest.m_iAcceptedMemberCount;
		int initialLiving = context.m_Fixture.m_Queue.CountStrategicLivingMemberSlots(
			context.m_Fixture.m_Batch);
		if (context.m_iAccepted <= 1 || initialLiving != context.m_iAccepted)
		{
			context.m_sFailure = string.Format(
				"nondegenerate admitted roster unavailable: accepted/living %1/%2",
				context.m_iAccepted,
				initialLiving);
			return context;
		}
		context.m_iCasualties = Math.Min(2, context.m_iAccepted - 1);
		bool casualtiesAccepted = ConfirmStrategicCasualties(
			context.m_Fixture,
			context.m_iCasualties);
		context.m_iSurvivors = context.m_Fixture.m_Queue.CountStrategicLivingMemberSlots(
			context.m_Fixture.m_Batch);
		if (!casualtiesAccepted || context.m_iCasualties <= 0
			|| context.m_iSurvivors <= 0
			|| context.m_iSurvivors != context.m_iAccepted - context.m_iCasualties)
		{
			context.m_sFailure = string.Format(
				"nondegenerate casualty roster failed: accepted/casualties/survivors %1/%2/%3",
				context.m_iAccepted,
				context.m_iCasualties,
				context.m_iSurvivors);
			return context;
		}

		context.m_sSettlementKind = "invalidated_survivors";
		context.m_sSettlementId = HST_OperationService.BuildSettlementId(
			context.m_Fixture.m_Order.m_sOperationId,
			context.m_sSettlementKind);
		context.m_sRefundMutationId
			= "enemy_resource_refund_" + context.m_sSettlementId;
		context.m_sReason
			= "exact enemy defensive QRF durable prepared recovery proof";
		HST_FactionPoolState pool = context.m_Fixture.m_State.FindFactionPool(
			PROOF_FACTION_KEY);
		HST_EnemySupportLedgerState ledger
			= context.m_Fixture.m_State.FindEnemySupportLedger(
				PROOF_FACTION_KEY,
				PROOF_TARGET_ZONE_ID);
		if (!pool || !ledger)
		{
			context.m_sFailure = "fixture pool or support ledger unavailable";
			return context;
		}
		context.m_iAttackRefund = Math.Max(
			0,
			context.m_Fixture.m_Order.m_iAttackCost)
			* context.m_iSurvivors / context.m_iAccepted;
		context.m_iSupportRefund = Math.Max(
			0,
			context.m_Fixture.m_Order.m_iSupportCost)
			* context.m_iSurvivors / context.m_iAccepted;
		CapturePreparedRecoveryResourceBaseline(context, pool, ledger);
		if (!StagePreparedRecoveryCut(context))
			return context;
		BuildPreparedRecoveryExpectation(context, pool, ledger);
		context.m_bPrefixExactBeforeSave = IsPreparedRecoveryPrefixExact(
			context.m_Fixture.m_State,
			context.m_Fixture.m_Order,
			context.m_Fixture.m_Operation,
			pool,
			ledger,
			context);
		return context;
	}

	protected void CapturePreparedRecoveryResourceBaseline(
		HST_EnemyQRFPreparedRecoveryCutContext context,
		HST_FactionPoolState pool,
		HST_EnemySupportLedgerState ledger)
	{
		if (!context || !pool || !ledger)
			return;
		context.m_iAttackBeforeRefund = pool.m_iAttackResources;
		context.m_iSupportBeforeRefund = pool.m_iSupportResources;
		context.m_iLedgerAttackSpentBeforeRefund = ledger.m_iAttackSpent;
		context.m_iLedgerSupportSpentBeforeRefund = ledger.m_iSupportSpent;
		context.m_iLedgerAttackRefundedBefore
			= ledger.m_iRefundedAttackResources;
		context.m_iLedgerSupportRefundedBefore
			= ledger.m_iRefundedSupportResources;
	}

	protected bool StagePreparedRecoveryCut(
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!context || !context.m_Fixture)
			return false;
		HST_EnemyQRFOperationProofFixture fixture = context.m_Fixture;
		fixture.m_State.m_iElapsedSeconds++;
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult prepared
			= operations.PrepareExactEnemyDefensiveQRFSettlement(
				fixture.m_State,
				fixture.m_Order,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				context.m_sSettlementId,
				context.m_sReason);
		if (!prepared || !prepared.m_bAccepted || !prepared.m_Operation)
		{
			context.m_sFailure = "terminal intent preparation failed";
			return false;
		}

		fixture.m_Order.m_sResourceSettlementId = context.m_sSettlementId;
		fixture.m_Order.m_sResourceSettlementKind = context.m_sSettlementKind;
		fixture.m_Order.m_sResourceRefundMutationId = context.m_sRefundMutationId;
		fixture.m_Order.m_iSettlementAcceptedMemberCount = context.m_iAccepted;
		fixture.m_Order.m_iSettlementSurvivorMemberCount = context.m_iSurvivors;
		fixture.m_Order.m_iRefundedAttackResources = context.m_iAttackRefund;
		fixture.m_Order.m_iRefundedSupportResources = context.m_iSupportRefund;
		fixture.m_Order.m_bResourceSettlementApplied = false;
		context.m_iPreparedAtSecond = fixture.m_Operation.m_iSettledAtSecond;

		if (context.m_iRestartCut >= PREPARED_CUT_AFTER_REFUND
			&& !fixture.m_EnemyDirector.RefundDefenseResources(
				fixture.m_State,
				fixture.m_Order.m_sFactionKey,
				fixture.m_Order.m_sTargetZoneId,
				context.m_iAttackRefund,
				context.m_iSupportRefund,
				context.m_sReason,
				context.m_sRefundMutationId,
				context.m_sSettlementId,
				fixture.m_Order.m_sOrderId,
				fixture.m_Order.m_sOperationId,
				fixture.m_Order.m_sManifestId))
		{
			context.m_sFailure = "direct deterministic refund setup failed";
			return false;
		}
		if (context.m_iRestartCut >= PREPARED_CUT_AFTER_RECEIPT)
		{
			HST_OperationTransitionResult recorded
				= operations.RecordExactEnemyDefensiveQRFResourceSettlement(
					fixture.m_State,
					fixture.m_Order,
					context.m_sSettlementKind,
					context.m_iAccepted,
					context.m_iSurvivors,
					context.m_sRefundMutationId,
					context.m_iAttackRefund,
					context.m_iSupportRefund);
			if (!recorded || !recorded.m_bAccepted
				|| !fixture.m_Order.m_bResourceSettlementApplied)
			{
				context.m_sFailure = "applied receipt setup failed";
				return false;
			}
		}

		context.m_iPrefixRevision = fixture.m_Operation.m_iRevision;
		context.m_bExpectedPrefixReceiptApplied
			= context.m_iRestartCut >= PREPARED_CUT_AFTER_RECEIPT;
		if (context.m_iRestartCut >= PREPARED_CUT_AFTER_REFUND)
		{
			context.m_iExpectedPrefixMutationCount = 1;
			context.m_iExpectedPrefixAttackDelta = context.m_iAttackRefund;
			context.m_iExpectedPrefixSupportDelta = context.m_iSupportRefund;
		}
		return true;
	}

	protected void BuildPreparedRecoveryExpectation(
		HST_EnemyQRFPreparedRecoveryCutContext context,
		HST_FactionPoolState pool,
		HST_EnemySupportLedgerState ledger)
	{
		if (!context || !context.m_Fixture || !pool || !ledger)
			return;
		HST_EnemyQRFPreparedRecoveryExpectation expectation
			= new HST_EnemyQRFPreparedRecoveryExpectation();
		expectation.m_sOrderId = context.m_Fixture.m_Order.m_sOrderId;
		expectation.m_sOperationId = context.m_Fixture.m_Operation.m_sOperationId;
		expectation.m_sManifestId = context.m_Fixture.m_Manifest.m_sManifestId;
		expectation.m_sManifestHash = context.m_Fixture.m_Manifest.m_sManifestHash;
		expectation.m_sBatchId = context.m_Fixture.m_Batch.m_sResultId;
		expectation.m_sGroupId = context.m_Fixture.m_Group.m_sGroupId;
		expectation.m_sSettlementKind = context.m_sSettlementKind;
		expectation.m_sSettlementId = context.m_sSettlementId;
		expectation.m_sRefundMutationId = context.m_sRefundMutationId;
		expectation.m_sReason = context.m_sReason;
		expectation.m_iAttackCost = context.m_Fixture.m_Order.m_iAttackCost;
		expectation.m_iSupportCost = context.m_Fixture.m_Order.m_iSupportCost;
		expectation.m_iAccepted = context.m_iAccepted;
		expectation.m_iSurvivors = context.m_iSurvivors;
		expectation.m_iAttackRefund = context.m_iAttackRefund;
		expectation.m_iSupportRefund = context.m_iSupportRefund;
		expectation.m_iExpectedAttackPool
			= context.m_iAttackBeforeRefund + context.m_iAttackRefund;
		expectation.m_iExpectedSupportPool
			= context.m_iSupportBeforeRefund + context.m_iSupportRefund;
		expectation.m_iExpectedPoolRevision = pool.m_iStrategicRevision;
		expectation.m_iExpectedPoolOperationalMutationCount
			= pool.m_iStrategicOperationalMutationCount;
		if (context.m_iRestartCut == PREPARED_CUT_BEFORE_REFUND)
		{
			expectation.m_iExpectedPoolRevision++;
			expectation.m_iExpectedPoolOperationalMutationCount++;
		}
		expectation.m_sExpectedLastStrategicMutationId
			= context.m_sRefundMutationId;
		expectation.m_sExpectedLedgerDecisionReason = string.Format(
			"%1 | refund attack %2 support %3",
			context.m_sReason,
			context.m_iAttackRefund,
			context.m_iSupportRefund);
		expectation.m_iExpectedLedgerRecentDamageScore
			= ledger.m_iRecentDamageScore;
		expectation.m_iExpectedLedgerLastDamageSecond = ledger.m_iLastDamageSecond;
		expectation.m_iExpectedLedgerAttackSpent = Math.Max(
			0,
			context.m_iLedgerAttackSpentBeforeRefund - context.m_iAttackRefund);
		expectation.m_iExpectedLedgerSupportSpent = Math.Max(
			0,
			context.m_iLedgerSupportSpentBeforeRefund - context.m_iSupportRefund);
		expectation.m_iExpectedLedgerLastSpendSecond = ledger.m_iLastSpendSecond;
		expectation.m_iExpectedLedgerCooldownUntilSecond
			= ledger.m_iCooldownUntilSecond;
		expectation.m_iExpectedLedgerAttackRefunded
			= context.m_iLedgerAttackRefundedBefore + context.m_iAttackRefund;
		expectation.m_iExpectedLedgerSupportRefunded
			= context.m_iLedgerSupportRefundedBefore + context.m_iSupportRefund;
		expectation.m_iPreparedAtSecond = context.m_iPreparedAtSecond;
		expectation.m_iExpectedTerminalRevision = context.m_iPrefixRevision + 2;
		if (context.m_bExpectedPrefixReceiptApplied)
			expectation.m_iExpectedTerminalRevision = context.m_iPrefixRevision + 1;
		context.m_Expectation = expectation;
	}

	protected bool IsPreparedRecoveryPrefixExact(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_FactionPoolState pool,
		HST_EnemySupportLedgerState ledger,
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!state || !order || !operation || !pool || !ledger || !context)
			return false;
		string debitFailure
			= HST_EnemyQRFSaveValidationService.ValidateOriginalResourceDebitAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		string refundFailure
			= HST_EnemyQRFSaveValidationService.ValidatePreparedRefundAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		if (!IsPreparedRecoveryPrefixOperationExact(operation, context))
			return false;
		if (!IsPreparedRecoveryPrefixOrderExact(order, context))
			return false;
		if (!IsPreparedRecoveryPrefixResourceExact(
				state,
				pool,
				ledger,
				context))
			return false;
		if (!debitFailure.IsEmpty())
			return false;
		if (!refundFailure.IsEmpty())
			return false;
		return true;
	}

	protected bool IsPreparedRecoveryPrefixOperationExact(
		HST_OperationRecordState operation,
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!operation)
			return false;
		if (!context)
			return false;
		if (operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
			return false;
		if (operation.m_eTerminalResult
			!= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED)
			return false;
		if (operation.m_sSettlementId != context.m_sSettlementId)
			return false;
		if (operation.m_sTerminalReason != context.m_sReason)
			return false;
		if (operation.m_iSettledAtSecond != context.m_iPreparedAtSecond)
			return false;
		if (context.m_iPreparedAtSecond <= 0)
			return false;
		if (operation.m_iRevision != context.m_iPrefixRevision)
			return false;
		return true;
	}

	protected bool IsPreparedRecoveryPrefixOrderExact(
		HST_EnemyOrderState order,
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!order)
			return false;
		if (!context)
			return false;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return false;
		if (order.m_sRuntimeStatus != "exact_virtual_outbound")
			return false;
		if (order.m_sResourceSettlementId != context.m_sSettlementId)
			return false;
		if (order.m_sResourceSettlementKind != context.m_sSettlementKind)
			return false;
		if (order.m_sResourceRefundMutationId != context.m_sRefundMutationId)
			return false;
		if (order.m_iSettlementAcceptedMemberCount != context.m_iAccepted)
			return false;
		if (order.m_iSettlementSurvivorMemberCount != context.m_iSurvivors)
			return false;
		if (order.m_iRefundedAttackResources != context.m_iAttackRefund)
			return false;
		if (order.m_iRefundedSupportResources != context.m_iSupportRefund)
			return false;
		if (order.m_bResourceSettlementApplied
			!= context.m_bExpectedPrefixReceiptApplied)
			return false;
		return true;
	}

	protected bool IsPreparedRecoveryPrefixResourceExact(
		HST_CampaignState state,
		HST_FactionPoolState pool,
		HST_EnemySupportLedgerState ledger,
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!state)
			return false;
		if (!pool)
			return false;
		if (!ledger)
			return false;
		if (!context)
			return false;
		if (CountStrategicMutations(state, context.m_sRefundMutationId)
			!= context.m_iExpectedPrefixMutationCount)
			return false;
		if (pool.m_iAttackResources
			!= context.m_iAttackBeforeRefund
				+ context.m_iExpectedPrefixAttackDelta)
			return false;
		if (pool.m_iSupportResources
			!= context.m_iSupportBeforeRefund
				+ context.m_iExpectedPrefixSupportDelta)
			return false;
		if (ledger.m_iAttackSpent != Math.Max(
			0,
			context.m_iLedgerAttackSpentBeforeRefund
				- context.m_iExpectedPrefixAttackDelta))
			return false;
		if (ledger.m_iSupportSpent != Math.Max(
			0,
			context.m_iLedgerSupportSpentBeforeRefund
				- context.m_iExpectedPrefixSupportDelta))
			return false;
		if (ledger.m_iRefundedAttackResources
			!= context.m_iLedgerAttackRefundedBefore
				+ context.m_iExpectedPrefixAttackDelta)
			return false;
		if (ledger.m_iRefundedSupportResources
			!= context.m_iLedgerSupportRefundedBefore
				+ context.m_iExpectedPrefixSupportDelta)
			return false;
		return true;
	}

	protected HST_EnemyQRFPreparedRecoveryRunResult RunPreparedRecoveryCut(
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		HST_EnemyQRFPreparedRecoveryRunResult result
			= new HST_EnemyQRFPreparedRecoveryRunResult();
		if (!context || !context.m_Fixture || !context.m_Expectation)
			return result;
		HST_CampaignSaveData interruptedSave = new HST_CampaignSaveData();
		interruptedSave.Capture(context.m_Fixture.m_State);
		HST_CampaignState restored = interruptedSave.Restore();
		result.m_bPrefixSurvived = IsPreparedRecoveryRestoredPrefixExact(
			restored,
			context);
		if (!restored)
			return result;

		HST_ForceSpawnQueueService restoredQueue = new HST_ForceSpawnQueueService();
		restoredQueue.ReconcileCampaignAfterRestore(restored);
		HST_EnemyQRFOperationService restoredQRF
			= new HST_EnemyQRFOperationService();
		restoredQRF.SetRuntimeServices(
			restoredQueue,
			new HST_ForceSpawnAdapterService(),
			new HST_PhysicalWarService());
		HST_EnemyDirectorService restoredDirector = new HST_EnemyDirectorService();
		result.m_bFirstChanged = restoredQRF.ReconcileAfterRestore(
			restored,
			restoredDirector);
		result.m_bFirstResumeExact = result.m_bFirstChanged
			&& IsPreparedRecoveryTerminalExact(restored, context.m_Expectation);
		string firstTerminalFingerprint
			= BuildPreparedRecoveryTerminalFingerprint(
				restored,
				context.m_Expectation);
		result.m_bSecondChanged = restoredQRF.ReconcileAfterRestore(
			restored,
			restoredDirector);
		string secondTerminalFingerprint
			= BuildPreparedRecoveryTerminalFingerprint(
				restored,
				context.m_Expectation);
		result.m_bSameStateReplayExact = !result.m_bSecondChanged
			&& IsPreparedRecoveryTerminalExact(restored, context.m_Expectation)
			&& secondTerminalFingerprint == firstTerminalFingerprint;
		ProvePreparedRecoverySecondRestore(
			restored,
			firstTerminalFingerprint,
			context,
			result);
		return result;
	}

	protected bool IsPreparedRecoveryRestoredPrefixExact(
		HST_CampaignState restored,
		HST_EnemyQRFPreparedRecoveryCutContext context)
	{
		if (!restored || !context || !context.m_Fixture)
			return false;
		HST_EnemyOrderState order = FindOrder(
			restored,
			context.m_Fixture.m_Order.m_sOrderId);
		HST_OperationRecordState operation = restored.FindOperation(
			context.m_Fixture.m_Operation.m_sOperationId);
		HST_FactionPoolState pool = restored.FindFactionPool(PROOF_FACTION_KEY);
		HST_EnemySupportLedgerState ledger = restored.FindEnemySupportLedger(
			PROOF_FACTION_KEY,
			PROOF_TARGET_ZONE_ID);
		return IsPreparedRecoveryPrefixExact(
			restored,
			order,
			operation,
			pool,
			ledger,
			context);
	}

	protected void ProvePreparedRecoverySecondRestore(
		HST_CampaignState restored,
		string firstTerminalFingerprint,
		HST_EnemyQRFPreparedRecoveryCutContext context,
		HST_EnemyQRFPreparedRecoveryRunResult result)
	{
		if (!restored || !context || !context.m_Expectation || !result)
			return;
		HST_CampaignSaveData terminalSave = new HST_CampaignSaveData();
		terminalSave.Capture(restored);
		HST_CampaignState secondRestored = terminalSave.Restore();
		if (!secondRestored)
			return;
		HST_ForceSpawnQueueService secondQueue = new HST_ForceSpawnQueueService();
		secondQueue.ReconcileCampaignAfterRestore(secondRestored);
		HST_EnemyQRFOperationService secondQRF
			= new HST_EnemyQRFOperationService();
		secondQRF.SetRuntimeServices(
			secondQueue,
			new HST_ForceSpawnAdapterService(),
			new HST_PhysicalWarService());
		result.m_bSecondRestoreExact = !secondQRF.ReconcileAfterRestore(
			secondRestored,
			new HST_EnemyDirectorService());
		HST_EnemyOrderState secondOrder = FindOrder(
			secondRestored,
			context.m_Expectation.m_sOrderId);
		HST_OperationRecordState secondOperation = secondRestored.FindOperation(
			context.m_Expectation.m_sOperationId);
		result.m_bSecondReceiptApplied
			= secondOrder && secondOrder.m_bResourceSettlementApplied;
		result.m_bSecondOperationPresent = secondOperation != null;
		result.m_iSecondMutationCount = CountStrategicMutations(
			secondRestored,
			context.m_sRefundMutationId);
		if (secondOperation)
			result.m_eSecondSettlementState = secondOperation.m_eSettlementState;
		string secondRestoreFingerprint
			= BuildPreparedRecoveryTerminalFingerprint(
				secondRestored,
				context.m_Expectation);
		result.m_bSecondRestoreExact = result.m_bSecondRestoreExact
			&& IsPreparedRecoveryTerminalExact(
				secondRestored,
				context.m_Expectation)
			&& secondRestoreFingerprint == firstTerminalFingerprint;
	}

	protected string BuildPreparedRecoveryEvidence(
		HST_EnemyQRFPreparedRecoveryCutContext context,
		HST_EnemyQRFPreparedRecoveryRunResult result)
	{
		if (!context || !context.m_Fixture || !context.m_Expectation || !result)
			return "prepared recovery result unavailable";
		string evidence = string.Format(
			"cost %1/%2 roster %3/%4/%5 | prefix %6/%7 mutations %8",
			context.m_Fixture.m_Order.m_iAttackCost,
			context.m_Fixture.m_Order.m_iSupportCost,
			context.m_iAccepted,
			context.m_iCasualties,
			context.m_iSurvivors,
			context.m_bPrefixExactBeforeSave,
			result.m_bPrefixSurvived,
			context.m_iExpectedPrefixMutationCount);
		evidence += string.Format(
			" | resume/replay changed %1/%2 exact %3/%4 | terminal mutation/state/present %5/%6/%7",
			result.m_bFirstChanged,
			result.m_bSecondChanged,
			result.m_bFirstResumeExact,
			result.m_bSameStateReplayExact,
			result.m_iSecondMutationCount,
			result.m_eSecondSettlementState,
			result.m_bSecondOperationPresent);
		evidence += string.Format(
			" | pool %1/%2 + %3/%4 | receipt/second-restore %5/%6 | revision %7 -> %8",
			context.m_iAttackBeforeRefund,
			context.m_iSupportBeforeRefund,
			context.m_iAttackRefund,
			context.m_iSupportRefund,
			result.m_bSecondReceiptApplied,
			result.m_bSecondRestoreExact,
			context.m_iPrefixRevision,
			context.m_Expectation.m_iExpectedTerminalRevision);
		return evidence;
	}
	protected bool IsPreparedRecoveryTerminalExact(
		HST_CampaignState state,
		HST_EnemyQRFPreparedRecoveryExpectation expectation)
	{
		if (!state)
			return false;
		if (!expectation)
			return false;
		HST_EnemyOrderState order = FindOrder(state, expectation.m_sOrderId);
		HST_OperationRecordState operation = state.FindOperation(expectation.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(expectation.m_sManifestId);
		HST_FactionPoolState pool = state.FindFactionPool(PROOF_FACTION_KEY);
		HST_EnemySupportLedgerState ledger = state.FindEnemySupportLedger(
			PROOF_FACTION_KEY,
			PROOF_TARGET_ZONE_ID);
		if (!order)
			return false;
		if (!operation)
			return false;
		if (!manifest)
			return false;
		if (!pool)
			return false;
		if (!ledger)
			return false;
		string debitFailure
			= HST_EnemyQRFSaveValidationService.ValidateOriginalResourceDebitAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		string refundFailure
			= HST_EnemyQRFSaveValidationService.ValidateSettledResourceRefundAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		if (!IsPreparedRecoveryTerminalOrderExact(order, expectation))
			return false;
		if (!IsPreparedRecoveryTerminalOperationExact(operation, expectation))
			return false;
		if (!IsPreparedRecoveryTerminalResourceExact(
				state,
				manifest,
				pool,
				ledger,
				expectation))
			return false;
		if (!debitFailure.IsEmpty())
			return false;
		if (!refundFailure.IsEmpty())
			return false;
		if (state.FindForceSpawnResult(expectation.m_sBatchId))
			return false;
		if (state.FindActiveGroup(expectation.m_sGroupId))
			return false;
		return true;
	}

	protected bool IsPreparedRecoveryTerminalOrderExact(
		HST_EnemyOrderState order,
		HST_EnemyQRFPreparedRecoveryExpectation expectation)
	{
		if (!order)
			return false;
		if (!expectation)
			return false;
		if (order.m_sOperationId != expectation.m_sOperationId)
			return false;
		if (order.m_sManifestId != expectation.m_sManifestId)
			return false;
		if (order.m_sManifestHash != expectation.m_sManifestHash)
			return false;
		if (order.m_sSpawnResultId != expectation.m_sBatchId)
			return false;
		if (order.m_sGroupId != expectation.m_sGroupId)
			return false;
		if (order.m_iAttackCost != expectation.m_iAttackCost)
			return false;
		if (order.m_iSupportCost != expectation.m_iSupportCost)
			return false;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
			return false;
		if (order.m_sRuntimeStatus != "resolved_exact_terminal")
			return false;
		if (order.m_sResolutionKind != expectation.m_sSettlementKind)
			return false;
		if (order.m_sFailureReason != expectation.m_sReason)
			return false;
		if (order.m_iResolvedAtSecond != expectation.m_iPreparedAtSecond)
			return false;
		if (!order.m_bStrategicServiceCommitted)
			return false;
		if (order.m_bPhysicalized)
			return false;
		if (order.m_bAbstractResolved)
			return false;
		if (order.m_bOutcomeApplied)
			return false;
		if (order.m_bResourceRefundApplied)
			return false;
		if (!order.m_bResourceSettlementApplied)
			return false;
		if (order.m_sResourceSettlementId != expectation.m_sSettlementId)
			return false;
		if (order.m_sResourceSettlementKind != expectation.m_sSettlementKind)
			return false;
		if (order.m_sResourceRefundMutationId != expectation.m_sRefundMutationId)
			return false;
		if (order.m_iSettlementAcceptedMemberCount != expectation.m_iAccepted)
			return false;
		if (order.m_iSettlementSurvivorMemberCount != expectation.m_iSurvivors)
			return false;
		if (order.m_iRefundedAttackResources != expectation.m_iAttackRefund)
			return false;
		if (order.m_iRefundedSupportResources != expectation.m_iSupportRefund)
			return false;
		return true;
	}

	protected bool IsPreparedRecoveryTerminalOperationExact(
		HST_OperationRecordState operation,
		HST_EnemyQRFPreparedRecoveryExpectation expectation)
	{
		if (!operation)
			return false;
		if (!expectation)
			return false;
		if (operation.m_eType
			!= HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF)
			return false;
		if (operation.m_iContractVersion
			!= HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION)
			return false;
		if (operation.m_sEnemyOrderId != expectation.m_sOrderId)
			return false;
		if (operation.m_sManifestId != expectation.m_sManifestId)
			return false;
		if (operation.m_sSpawnResultId != expectation.m_sBatchId)
			return false;
		if (operation.m_sGroupId != expectation.m_sGroupId)
			return false;
		if (operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return false;
		if (operation.m_eTerminalResult
			!= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED)
			return false;
		if (operation.m_sSettlementId != expectation.m_sSettlementId)
			return false;
		if (operation.m_sTerminalReason != expectation.m_sReason)
			return false;
		if (operation.m_iSettledAtSecond != expectation.m_iPreparedAtSecond)
			return false;
		if (operation.m_iRevision != expectation.m_iExpectedTerminalRevision)
			return false;
		if (operation.m_eDutyState
			!= HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED)
			return false;
		if (operation.m_eEngagementMode
			!= HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
			return false;
		if (operation.m_eMaterializationState
			!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
			return false;
		if (operation.m_ePositionAuthority
			!= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return false;
		if (operation.m_iDutyStateEnteredAtSecond != expectation.m_iPreparedAtSecond)
			return false;
		if (operation.m_iEngagementStateEnteredAtSecond != expectation.m_iPreparedAtSecond)
			return false;
		if (operation.m_iMaterializationStateEnteredAtSecond
			!= expectation.m_iPreparedAtSecond)
			return false;
		if (operation.m_iLastProgressAtSecond != expectation.m_iPreparedAtSecond)
			return false;
		if (operation.m_iArrivalConfirmationCount != 0)
			return false;
		if (operation.m_iLastArrivalConfirmationSecond != 0)
			return false;
		return true;
	}

	protected bool IsPreparedRecoveryTerminalResourceExact(
		HST_CampaignState state,
		HST_ForceManifestState manifest,
		HST_FactionPoolState pool,
		HST_EnemySupportLedgerState ledger,
		HST_EnemyQRFPreparedRecoveryExpectation expectation)
	{
		if (!state)
			return false;
		if (!manifest)
			return false;
		if (!pool)
			return false;
		if (!ledger)
			return false;
		if (!expectation)
			return false;
		if (!manifest.m_bFrozen)
			return false;
		if (manifest.m_sOperationId != expectation.m_sOperationId)
			return false;
		if (manifest.m_sManifestHash != expectation.m_sManifestHash)
			return false;
		if (manifest.m_iAcceptedMemberCount != expectation.m_iAccepted)
			return false;
		if (pool.m_iAttackResources != expectation.m_iExpectedAttackPool)
			return false;
		if (pool.m_iSupportResources != expectation.m_iExpectedSupportPool)
			return false;
		if (pool.m_iStrategicRevision != expectation.m_iExpectedPoolRevision)
			return false;
		if (pool.m_iStrategicOperationalMutationCount
			!= expectation.m_iExpectedPoolOperationalMutationCount)
			return false;
		if (pool.m_sLastStrategicMutationId
			!= expectation.m_sExpectedLastStrategicMutationId)
			return false;
		if (ledger.m_sFactionKey != PROOF_FACTION_KEY)
			return false;
		if (ledger.m_sZoneId != PROOF_TARGET_ZONE_ID)
			return false;
		if (ledger.m_sLastDecisionReason
			!= expectation.m_sExpectedLedgerDecisionReason)
			return false;
		if (ledger.m_iRecentDamageScore
			!= expectation.m_iExpectedLedgerRecentDamageScore)
			return false;
		if (ledger.m_iLastDamageSecond
			!= expectation.m_iExpectedLedgerLastDamageSecond)
			return false;
		if (ledger.m_iAttackSpent != expectation.m_iExpectedLedgerAttackSpent)
			return false;
		if (ledger.m_iSupportSpent != expectation.m_iExpectedLedgerSupportSpent)
			return false;
		if (ledger.m_iLastSpendSecond
			!= expectation.m_iExpectedLedgerLastSpendSecond)
			return false;
		if (ledger.m_iCooldownUntilSecond
			!= expectation.m_iExpectedLedgerCooldownUntilSecond)
			return false;
		if (ledger.m_iRefundedAttackResources
			!= expectation.m_iExpectedLedgerAttackRefunded)
			return false;
		if (ledger.m_iRefundedSupportResources
			!= expectation.m_iExpectedLedgerSupportRefunded)
			return false;
		if (CountStrategicMutations(
			state,
			expectation.m_sRefundMutationId) != 1)
			return false;
		return true;
	}

	protected string BuildPreparedRecoveryTerminalFingerprint(
		HST_CampaignState state,
		HST_EnemyQRFPreparedRecoveryExpectation expectation)
	{
		if (!state || !expectation)
			return "prepared-recovery-state-unavailable";
		HST_EnemyOrderState order = FindOrder(state, expectation.m_sOrderId);
		HST_OperationRecordState operation = state.FindOperation(expectation.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(expectation.m_sManifestId);
		HST_FactionPoolState pool = state.FindFactionPool(PROOF_FACTION_KEY);
		HST_EnemySupportLedgerState ledger = state.FindEnemySupportLedger(
			PROOF_FACTION_KEY,
			PROOF_TARGET_ZONE_ID);
		HST_EnemyStrategicMutationState mutation
			= state.FindEnemyStrategicMutation(expectation.m_sRefundMutationId);
		if (!order || !operation || !manifest || !pool || !ledger || !mutation)
			return "prepared-recovery-terminal-aggregate-incomplete";

		string fingerprint = string.Format(
			"order %1/%2/%3/%4/%5/%6/%7/%8/%9",
			order.m_sOrderId,
			order.m_sOperationId,
			order.m_sManifestId,
			order.m_sManifestHash,
			order.m_sSpawnResultId,
			order.m_sGroupId,
			order.m_eStatus,
			order.m_sRuntimeStatus,
			order.m_sResolutionKind);
		fingerprint += string.Format(
			" | order terminal %1/%2/%3/%4/%5/%6/%7/%8/%9",
			order.m_sFailureReason,
			order.m_iResolvedAtSecond,
			order.m_iAttackCost,
			order.m_iSupportCost,
			order.m_bStrategicServiceCommitted,
			order.m_bPhysicalized,
			order.m_bAbstractResolved,
			order.m_bOutcomeApplied,
			order.m_bResourceRefundApplied);
		fingerprint += string.Format(
			" | receipt %1/%2/%3/%4/%5/%6/%7/%8",
			order.m_bResourceSettlementApplied,
			order.m_sResourceSettlementId,
			order.m_sResourceSettlementKind,
			order.m_sResourceRefundMutationId,
			order.m_iSettlementAcceptedMemberCount,
			order.m_iSettlementSurvivorMemberCount,
			order.m_iRefundedAttackResources,
			order.m_iRefundedSupportResources);
		fingerprint += string.Format(
			" | operation identity %1/%2/%3/%4/%5/%6/%7/%8/%9",
			operation.m_sOperationId,
			operation.m_eType,
			operation.m_iContractVersion,
			operation.m_sEnemyOrderId,
			operation.m_sManifestId,
			operation.m_sSpawnResultId,
			operation.m_sForceId,
			operation.m_sProjectionId,
			operation.m_sGroupId);
		fingerprint += string.Format(
			" | operation terminal %1/%2/%3/%4/%5/%6/%7/%8/%9",
			operation.m_eDutyState,
			operation.m_eEngagementMode,
			operation.m_eMaterializationState,
			operation.m_ePositionAuthority,
			operation.m_eSettlementState,
			operation.m_eTerminalResult,
			operation.m_sSettlementId,
			operation.m_sTerminalReason,
			operation.m_iSettledAtSecond);
		fingerprint += string.Format(
			" | operation chronology %1/%2/%3/%4/%5/%6/%7",
			operation.m_iRevision,
			operation.m_iDutyStateEnteredAtSecond,
			operation.m_iEngagementStateEnteredAtSecond,
			operation.m_iMaterializationStateEnteredAtSecond,
			operation.m_iLastProgressAtSecond,
			operation.m_iArrivalConfirmationCount,
			operation.m_iLastArrivalConfirmationSecond);
		fingerprint += string.Format(
			" | manifest %1/%2/%3/%4 | pool %5/%6/%7/%8/%9",
			manifest.m_sManifestId,
			manifest.m_sManifestHash,
			manifest.m_iAcceptedMemberCount,
			manifest.m_bFrozen,
			pool.m_iAttackResources,
			pool.m_iSupportResources,
			pool.m_iStrategicRevision,
			pool.m_iStrategicOperationalMutationCount,
			pool.m_sLastStrategicMutationId);
		fingerprint += string.Format(
			" | ledger %1/%2/%3/%4/%5/%6/%7/%8/%9",
			ledger.m_sFactionKey,
			ledger.m_sZoneId,
			ledger.m_iAttackSpent,
			ledger.m_iSupportSpent,
			ledger.m_iRefundedAttackResources,
			ledger.m_iRefundedSupportResources,
			ledger.m_iLastSpendSecond,
			ledger.m_iCooldownUntilSecond,
			ledger.m_sLastDecisionReason);
		fingerprint += string.Format(
			" | mutation identity %1/%2/%3/%4/%5/%6/%7/%8/%9",
			mutation.m_sMutationId,
			mutation.m_sFactionKey,
			mutation.m_sKind,
			mutation.m_sSourceId,
			mutation.m_sOrderId,
			mutation.m_sOperationId,
			mutation.m_sManifestId,
			mutation.m_sZoneId,
			mutation.m_bApplied);
		fingerprint += string.Format(
			" | mutation values %1/%2/%3/%4/%5/%6/%7/%8/%9",
			mutation.m_iCreatedAtSecond,
			mutation.m_iPoolRevisionBefore,
			mutation.m_iPoolRevisionAfter,
			mutation.m_iOperationalSequence,
			mutation.m_iAttackBefore,
			mutation.m_iAttackDelta,
			mutation.m_iAttackAfter,
			mutation.m_iSupportBefore,
			mutation.m_iSupportDelta);
		fingerprint += string.Format(
			"/%1/%2/%3/%4/%5 | runtime %6/%7 | count %8",
			mutation.m_iSupportAfter,
			mutation.m_iAggressionBefore,
			mutation.m_iAggressionDelta,
			mutation.m_iAggressionAfter,
			mutation.m_sFingerprint,
			state.FindForceSpawnResult(expectation.m_sBatchId) != null,
			state.FindActiveGroup(expectation.m_sGroupId) != null,
			CountStrategicMutations(state, expectation.m_sRefundMutationId));
		return fingerprint;
	}

	protected bool ProvePartialReceiptRestoreQuarantine(out string evidence)
	{
		HST_EnemyQRFOperationProofFixture fixture = BuildAdmittedFixture("restore_partial_receipt");
		if (!Ready(fixture))
		{
			evidence = BuildFixtureFailure(fixture);
			return false;
		}
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		fixture.m_Order.m_sResourceRefundMutationId
			= "enemy_resource_refund_proof_partial_" + fixture.m_Order.m_sOperationId;
		fixture.m_EnemyDirector.RefundDefenseResources(
			fixture.m_State,
			fixture.m_Order.m_sFactionKey,
			fixture.m_Order.m_sTargetZoneId,
			1,
			1,
			"enemy QRF partial-receipt proof mutation",
			fixture.m_Order.m_sResourceRefundMutationId,
			"proof_partial_receipt_" + fixture.m_Order.m_sOrderId,
			fixture.m_Order.m_sOrderId,
			fixture.m_Order.m_sOperationId,
			fixture.m_Order.m_sManifestId);
		fixture.m_Order.m_iRefundedAttackResources = 1;
		fixture.m_Order.m_iRefundedSupportResources = 1;
		HST_OperationService operationService = new HST_OperationService();
		string directFailure = operationService.ValidateExactEnemyDefensiveQRF(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Order,
			fixture.m_Manifest);
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_EnemyOrderState restoredOrder;
		HST_OperationRecordState restoredOperation;
		HST_ForceSpawnResultState restoredBatch;
		HST_ActiveGroupState restoredGroup;
		HST_FactionPoolState restoredPool;
		if (restored)
		{
			restoredOrder = FindOrder(restored, fixture.m_Order.m_sOrderId);
			restoredOperation = restored.FindOperation(fixture.m_Operation.m_sOperationId);
			restoredBatch = restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
			restoredGroup = restored.FindActiveGroup(fixture.m_Group.m_sGroupId);
			restoredPool = restored.FindFactionPool(PROOF_FACTION_KEY);
		}
		int attackBeforeReconcile;
		int supportBeforeReconcile;
		if (restoredPool)
		{
			attackBeforeReconcile = restoredPool.m_iAttackResources;
			supportBeforeReconcile = restoredPool.m_iSupportResources;
		}
		if (restored)
		{
			HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
			queue.ReconcileCampaignAfterRestore(restored);
			HST_EnemyQRFOperationService exactQRF = new HST_EnemyQRFOperationService();
			exactQRF.SetRuntimeServices(queue, new HST_ForceSpawnAdapterService(), new HST_PhysicalWarService());
			exactQRF.ReconcileAfterRestore(restored, new HST_EnemyDirectorService());
			exactQRF.ReconcileAfterRestore(restored, new HST_EnemyDirectorService());
		}
		bool resourceAuthorityRejected
			= directFailure.Contains("resource settlement authority");
		bool exact = pool && restoredOrder && restoredOperation && restoredBatch && restoredGroup && restoredPool
			&& resourceAuthorityRejected
			&& restoredOrder.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& restoredOrder.m_sRuntimeStatus == "exact_restore_settlement_conflict"
			&& !restoredOrder.m_bResourceSettlementApplied
			&& restoredOrder.m_iRefundedAttackResources == 1
			&& restoredOrder.m_iRefundedSupportResources == 1
			&& restoredPool.m_iAttackResources == attackBeforeReconcile
			&& restoredPool.m_iSupportResources == supportBeforeReconcile
			&& restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		int restoredStatus = -1;
		string restoredRuntimeStatus = "<missing-order>";
		int restoredAttackRefund = -1;
		int restoredSupportRefund = -1;
		int restoredAttackResources = -1;
		int restoredSupportResources = -1;
		if (restoredOrder)
		{
			restoredStatus = restoredOrder.m_eStatus;
			restoredRuntimeStatus = restoredOrder.m_sRuntimeStatus;
			restoredAttackRefund = restoredOrder.m_iRefundedAttackResources;
			restoredSupportRefund = restoredOrder.m_iRefundedSupportResources;
		}
		if (restoredPool)
		{
			restoredAttackResources = restoredPool.m_iAttackResources;
			restoredSupportResources = restoredPool.m_iSupportResources;
		}
		evidence = string.Format(
			"validator '%1' classified %2 | status %3/'%4' | refund %5/%6",
			directFailure,
			resourceAuthorityRejected,
			restoredStatus,
			restoredRuntimeStatus,
			restoredAttackRefund,
			restoredSupportRefund);
		evidence = evidence + string.Format(
			" | pool %1/%2 -> %3/%4 | exact %5",
			attackBeforeReconcile,
			supportBeforeReconcile,
			restoredAttackResources,
			restoredSupportResources,
			exact);
		evidence = evidence + string.Format(
			" | rows %1/%2/%3",
			restoredOperation != null,
			restoredBatch != null,
			restoredGroup != null);
		return exact;
	}

	protected bool ProveMissingGroupBacklinkRestore(out string evidence)
	{
		HST_EnemyQRFOperationProofFixture fixture = BuildAdmittedFixture("restore_missing_group_backlink");
		if (!Ready(fixture))
		{
			evidence = BuildFixtureFailure(fixture);
			return false;
		}
		fixture.m_Group.m_sEnemyOrderId = "";
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_EnemyOrderState order;
		HST_ActiveGroupState group;
		HST_OperationRecordState operation;
		HST_ForceSpawnResultState batch;
		HST_FactionPoolState restoredPool;
		if (restored)
		{
			order = FindOrder(restored, fixture.m_Order.m_sOrderId);
			group = restored.FindActiveGroup(fixture.m_Group.m_sGroupId);
			operation = restored.FindOperation(fixture.m_Operation.m_sOperationId);
			batch = restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
			restoredPool = restored.FindFactionPool(PROOF_FACTION_KEY);
		}
		bool exact = order && group && operation && batch && restoredPool
			&& group.m_sEnemyOrderId.IsEmpty()
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& order.m_sRuntimeStatus == "exact_operation_invalidated"
			&& order.m_sFailureReason.Contains("runtime backlinks conflict")
			&& !order.m_bResourceSettlementApplied
			&& restoredPool.m_iAttackResources == attackBefore
			&& restoredPool.m_iSupportResources == supportBefore
			&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		string restoredBacklink = "<missing-group>";
		int restoredOrderStatus = -1;
		string restoredRuntimeStatus = "<missing-order>";
		string restoredFailureReason = "<missing-order>";
		int restoredAttackResources = -1;
		int restoredSupportResources = -1;
		int restoredSettlementState = -1;
		if (group)
			restoredBacklink = group.m_sEnemyOrderId;
		if (order)
		{
			restoredOrderStatus = order.m_eStatus;
			restoredRuntimeStatus = order.m_sRuntimeStatus;
			restoredFailureReason = order.m_sFailureReason;
		}
		if (restoredPool)
		{
			restoredAttackResources = restoredPool.m_iAttackResources;
			restoredSupportResources = restoredPool.m_iSupportResources;
		}
		if (operation)
			restoredSettlementState = operation.m_eSettlementState;
		evidence = string.Format(
			"backlink '%1' | status %2/'%3' | reason '%4' | rows %5/%6/%7",
			restoredBacklink,
			restoredOrderStatus,
			restoredRuntimeStatus,
			restoredFailureReason,
			operation != null,
			batch != null,
			group != null);
		evidence = evidence + string.Format(
			" | pool %1/%2 -> %3/%4 | settlement %5 | exact %6",
			attackBefore,
			supportBefore,
			restoredAttackResources,
			restoredSupportResources,
			restoredSettlementState,
			exact);
		return exact;
	}

	protected HST_EnemyQRFReplayAmbiguityProofSummary ProveCommittedReplayAmbiguity()
	{
		HST_EnemyQRFReplayAmbiguityProofSummary summary = new HST_EnemyQRFReplayAmbiguityProofSummary();
		string missingEvidence;
		string shadowEvidence;
		summary.m_bMissingCanonicalRejected = ProveMissingCanonicalReplayRejection(missingEvidence);
		summary.m_bShadowRejected = ProveShadowReplayRejection(shadowEvidence);
		summary.m_sEvidence = "missing " + missingEvidence + " | shadow " + shadowEvidence;
		return summary;
	}

	protected bool ProveMissingCanonicalReplayRejection(out string evidence)
	{
		HST_EnemyQRFOperationProofFixture fixture = BuildAdmittedFixture("replay_missing_canonical");
		if (!Ready(fixture))
		{
			evidence = BuildFixtureFailure(fixture);
			return false;
		}
		int canonicalIndex = fixture.m_State.m_aOperations.Find(fixture.m_Operation);
		if (canonicalIndex >= 0)
			fixture.m_State.m_aOperations.Remove(canonicalIndex);
		HST_OperationRecordState foreign = new HST_OperationRecordState();
		foreign.m_sOperationId = fixture.m_Operation.m_sOperationId + "_foreign";
		foreign.m_sEnemyOrderId = fixture.m_Order.m_sOrderId;
		fixture.m_State.m_aOperations.Insert(foreign);
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		HST_EnemyQRFAdmissionResult replay = fixture.m_ExactQRF.AdmitPreparedOrder(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Manifest,
			fixture.m_EnemyDirector);
		bool exact = replay && !replay.m_bSuccess && !replay.m_bStateChanged
			&& replay.m_sFailureReason.Contains("operation identity is ambiguous")
			&& fixture.m_State.m_aOperations.Count() == 1
			&& fixture.m_State.m_aOperations[0] == foreign
			&& fixture.m_State.FindForceSpawnResult(fixture.m_Batch.m_sResultId) == fixture.m_Batch
			&& fixture.m_State.FindActiveGroup(fixture.m_Group.m_sGroupId) == fixture.m_Group
			&& pool.m_iAttackResources == attackBefore && pool.m_iSupportResources == supportBefore
			&& !fixture.m_Order.m_bResourceSettlementApplied;
		evidence = string.Format(
			"accepted/changed %1/%2 | reason '%3' | rows %4 | pool %5/%6",
			replay && replay.m_bSuccess,
			replay && replay.m_bStateChanged,
			replay && replay.m_sFailureReason,
			fixture.m_State.m_aOperations.Count(),
			pool && pool.m_iAttackResources,
			pool && pool.m_iSupportResources);
		return exact;
	}

	protected bool ProveShadowReplayRejection(out string evidence)
	{
		HST_EnemyQRFOperationProofFixture fixture = BuildAdmittedFixture("replay_shadow");
		if (!Ready(fixture))
		{
			evidence = BuildFixtureFailure(fixture);
			return false;
		}
		HST_OperationRecordState shadow = new HST_OperationRecordState();
		shadow.m_sOperationId = fixture.m_Operation.m_sOperationId + "_shadow";
		shadow.m_sEnemyOrderId = fixture.m_Order.m_sOrderId;
		fixture.m_State.m_aOperations.Insert(shadow);
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		HST_EnemyQRFAdmissionResult replay = fixture.m_ExactQRF.AdmitPreparedOrder(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Manifest,
			fixture.m_EnemyDirector);
		bool exact = replay && !replay.m_bSuccess && !replay.m_bStateChanged
			&& replay.m_sFailureReason.Contains("operation identity is ambiguous")
			&& fixture.m_State.m_aOperations.Count() == 2
			&& fixture.m_State.FindOperation(fixture.m_Operation.m_sOperationId) == fixture.m_Operation
			&& fixture.m_State.m_aOperations.Find(shadow) >= 0
			&& fixture.m_State.FindForceSpawnResult(fixture.m_Batch.m_sResultId) == fixture.m_Batch
			&& fixture.m_State.FindActiveGroup(fixture.m_Group.m_sGroupId) == fixture.m_Group
			&& pool.m_iAttackResources == attackBefore && pool.m_iSupportResources == supportBefore
			&& !fixture.m_Order.m_bResourceSettlementApplied;
		evidence = string.Format(
			"accepted/changed %1/%2 | reason '%3' | rows %4 | pool %5/%6",
			replay && replay.m_bSuccess,
			replay && replay.m_bStateChanged,
			replay && replay.m_sFailureReason,
			fixture.m_State.m_aOperations.Count(),
			pool && pool.m_iAttackResources,
			pool && pool.m_iSupportResources);
		return exact;
	}

	protected string ProveLegacyCollision(out bool rejectedExactly)
	{
		rejectedExactly = false;
		HST_CampaignState state = BuildState();
		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_EnemyDirectorService enemyDirector = new HST_EnemyDirectorService();
		HST_ForcePlanningService planning = new HST_ForcePlanningService();
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		HST_EnemyQRFOperationService exactQRF = new HST_EnemyQRFOperationService();
		exactQRF.SetRuntimeServices(queue, new HST_ForceSpawnAdapterService(), new HST_PhysicalWarService());
		HST_EnemyOrderState order = BuildOrder(state, "legacy_collision");
		HST_EnemyDefensiveQRFManifestResult planned = planning.PlanExactEnemyDefensiveQRF(state, preset, order);
		if (!planned || !planned.m_bSuccess || !planned.m_Manifest)
			return "legacy collision fixture planning failed";

		HST_QRFState legacy = new HST_QRFState();
		legacy.m_sInstanceId = "enemy_qrf_proof_legacy_collision";
		legacy.m_sFactionKey = PROOF_FACTION_KEY;
		legacy.m_sSourceZoneId = PROOF_SOURCE_ZONE_ID;
		legacy.m_sTargetZoneId = PROOF_TARGET_ZONE_ID;
		legacy.m_iStartedAtSecond = state.m_iElapsedSeconds;
		legacy.m_iETASeconds = 60;
		state.m_aQRFs.Insert(legacy);

		HST_FactionPoolState pool = state.FindFactionPool(PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		string spendReason;
		order.m_sResourceDebitMutationId
			= "enemy_resource_debit_" + order.m_sOrderId;
		bool spent = enemyDirector.TrySpendDefense(
			state,
			state.FindZone(PROOF_TARGET_ZONE_ID),
			PROOF_FACTION_KEY,
			order.m_iAttackCost,
			order.m_iSupportCost,
			spendReason,
			order.m_sResourceDebitMutationId,
			order.m_sOrderId,
			order.m_sOrderId,
			order.m_sOperationId,
			planned.m_Manifest.m_sManifestId);
		HST_EnemyQRFAdmissionResult admission;
		if (spent)
		{
			state.m_aEnemyOrders.Insert(order);
			admission = exactQRF.AdmitPreparedOrder(state, order, planned.m_Manifest, enemyDirector);
		}

		rejectedExactly = spent && admission && !admission.m_bSuccess
			&& admission.m_sFailureReason.Contains("legacy QRF")
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& order.m_bResourceSettlementApplied
			&& pool.m_iAttackResources == attackBefore
			&& pool.m_iSupportResources == supportBefore
			&& state.m_aQRFs.Count() == 1 && state.m_aQRFs[0] == legacy
			&& state.m_aOperations.Count() == 0
			&& state.m_aForceManifests.Count() == 0
			&& state.m_aForceSpawnResults.Count() == 0
			&& state.m_aActiveGroups.Count() == 0;
		return string.Format(
			"legacy collision spent/rejected/refunded %1/%2/%3 | legacy/exact rows %4/%5/%6/%7/%8 | reason '%9'",
			spent,
			admission && !admission.m_bSuccess,
			pool.m_iAttackResources == attackBefore && pool.m_iSupportResources == supportBefore,
			state.m_aQRFs.Count(),
			state.m_aOperations.Count(),
			state.m_aForceManifests.Count(),
			state.m_aForceSpawnResults.Count(),
			state.m_aActiveGroups.Count(),
			admission && admission.m_sFailureReason);
	}

	protected HST_EnemyQRFOperationProofFixture BuildAdmittedFixture(
		string suffix,
		int attackCost = PROOF_ATTACK_COST,
		int supportCost = PROOF_SUPPORT_COST)
	{
		HST_EnemyQRFOperationProofFixture fixture = new HST_EnemyQRFOperationProofFixture();
		fixture.m_State = BuildState();
		fixture.m_Preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		fixture.m_EnemyDirector = new HST_EnemyDirectorService();
		fixture.m_Planning = new HST_ForcePlanningService();
		fixture.m_Queue = new HST_ForceSpawnQueueService();
		fixture.m_Adapter = new HST_ForceSpawnAdapterService();
		fixture.m_PhysicalWar = new HST_PhysicalWarService();
		fixture.m_ExactQRF = new HST_EnemyQRFOperationService();
		fixture.m_ExactQRF.SetRuntimeServices(fixture.m_Queue, fixture.m_Adapter, fixture.m_PhysicalWar);
		fixture.m_Order = BuildOrder(fixture.m_State, suffix);
		fixture.m_Order.m_iAttackCost = Math.Max(0, attackCost);
		fixture.m_Order.m_iSupportCost = Math.Max(0, supportCost);
		HST_EnemyDefensiveQRFManifestResult planned = fixture.m_Planning.PlanExactEnemyDefensiveQRF(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Order);
		if (!planned || !planned.m_bSuccess || !planned.m_Manifest)
			return fixture;
		fixture.m_Manifest = planned.m_Manifest;

		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		fixture.m_iAttackBeforeDebit = pool.m_iAttackResources;
		fixture.m_iSupportBeforeDebit = pool.m_iSupportResources;
		fixture.m_Order.m_sResourceDebitMutationId
			= "enemy_resource_debit_" + fixture.m_Order.m_sOrderId;
		fixture.m_bDebitAccepted = fixture.m_EnemyDirector.TrySpendDefense(
			fixture.m_State,
			fixture.m_State.FindZone(PROOF_TARGET_ZONE_ID),
			PROOF_FACTION_KEY,
			fixture.m_Order.m_iAttackCost,
			fixture.m_Order.m_iSupportCost,
			fixture.m_sDebitReason,
			fixture.m_Order.m_sResourceDebitMutationId,
			fixture.m_Order.m_sOrderId,
			fixture.m_Order.m_sOrderId,
			fixture.m_Order.m_sOperationId,
			fixture.m_Manifest.m_sManifestId);
		fixture.m_iAttackAfterDebit = pool.m_iAttackResources;
		fixture.m_iSupportAfterDebit = pool.m_iSupportResources;
		if (!fixture.m_bDebitAccepted)
			return fixture;

		fixture.m_State.m_aEnemyOrders.Insert(fixture.m_Order);
		fixture.m_Admission = fixture.m_ExactQRF.AdmitPreparedOrder(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Manifest,
			fixture.m_EnemyDirector);
		fixture.m_iAttackAfterAdmission = pool.m_iAttackResources;
		fixture.m_iSupportAfterAdmission = pool.m_iSupportResources;
		if (!fixture.m_Admission || !fixture.m_Admission.m_bSuccess)
			return fixture;
		fixture.m_Operation = fixture.m_Admission.m_Operation;
		fixture.m_Batch = fixture.m_Admission.m_Batch;
		fixture.m_Group = fixture.m_Admission.m_Group;
		return fixture;
	}

	protected HST_CampaignState BuildState()
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iCampaignSeed = 515151;
		state.m_iElapsedSeconds = 100;
		state.m_iWarLevel = 3;
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		HST_FactionPoolState pool = new HST_FactionPoolState();
		pool.m_sFactionKey = PROOF_FACTION_KEY;
		pool.m_iStrategicContractVersion = HST_EnemyStrategicResourceService.CONTRACT_VERSION;
		pool.m_iStrategicRevision = 1;
		pool.m_iAttackResources = 200;
		pool.m_iSupportResources = 200;
		pool.m_iAggression = 50;
		state.m_aFactionPools.Insert(pool);

		HST_ZoneState source = new HST_ZoneState();
		source.m_sZoneId = PROOF_SOURCE_ZONE_ID;
		source.m_sDisplayName = "Enemy QRF Proof Source";
		source.m_sOwnerFactionKey = PROOF_FACTION_KEY;
		source.m_eType = HST_EZoneType.HST_ZONE_OUTPOST;
		source.m_vPosition = "1000 20 1000";
		state.m_aZones.Insert(source);
		HST_ZoneState target = new HST_ZoneState();
		target.m_sZoneId = PROOF_TARGET_ZONE_ID;
		target.m_sDisplayName = "Enemy QRF Proof Target";
		target.m_sOwnerFactionKey = PROOF_FACTION_KEY;
		target.m_eType = HST_EZoneType.HST_ZONE_TOWN;
		target.m_vPosition = "2200 20 1800";
		target.m_iResistanceCaptureProgress = 50;
		state.m_aZones.Insert(target);
		return state;
	}

	protected HST_EnemyOrderState BuildOrder(HST_CampaignState state, string suffix)
	{
		HST_EnemyOrderState order = new HST_EnemyOrderState();
		order.m_sOrderId = "enemy_qrf_proof_" + suffix;
		order.m_sOperationId = HST_StableIdService.BuildOperationId("enemy_order", order.m_sOrderId);
		order.m_iOperationContractVersion = HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION;
		order.m_sFactionKey = PROOF_FACTION_KEY;
		order.m_eType = HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED;
		order.m_sSourceZoneId = PROOF_SOURCE_ZONE_ID;
		order.m_sTargetZoneId = PROOF_TARGET_ZONE_ID;
		order.m_vSourcePosition = state.FindZone(PROOF_SOURCE_ZONE_ID).m_vPosition;
		order.m_vTargetPosition = state.FindZone(PROOF_TARGET_ZONE_ID).m_vPosition;
		order.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		order.m_iAttackCost = PROOF_ATTACK_COST;
		order.m_iSupportCost = PROOF_SUPPORT_COST;
		order.m_sRuntimeStatus = "proof_prepaid_pending";
		return order;
	}

	protected bool ConfirmStrategicCasualties(HST_EnemyQRFOperationProofFixture fixture, int casualtyCount)
	{
		if (!Ready(fixture) || casualtyCount < 0)
			return false;
		for (int index = 0; index < casualtyCount; index++)
		{
			string slotId = fixture.m_Queue.SelectStrategicLivingMemberSlotId(
				fixture.m_Batch,
				fixture.m_Operation.m_iDeterministicSeed + index);
			if (slotId.IsEmpty())
				return false;
			HST_ForceSpawnQueueCallbackResult casualty = fixture.m_Queue.ConfirmStrategicMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				slotId,
				fixture.m_State.m_iElapsedSeconds + index + 1,
				"enemy QRF proof casualty");
			if (!casualty || !casualty.m_bAccepted)
				return false;
		}
		return true;
	}

	protected void ApplySyntheticSuccessfulProjection(HST_EnemyQRFOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_Batch)
			return;
		fixture.m_Batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
		fixture.m_Batch.m_iSuccessfulHandoffCount = 1;
		foreach (HST_ForceSpawnSlotResultState slot : fixture.m_Batch.m_aSlotResults)
		{
			if (!slot || (slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED && slot.m_bCasualtyConfirmed))
				continue;
			slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
			slot.m_sEntityId = "enemy_qrf_proof_entity_" + slot.m_sSlotId;
			slot.m_sNativeGroupId = "enemy_qrf_proof_native_group";
			slot.m_bAliveVerified = true;
			if (slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
				slot.m_bEverAlive = true;
		}
	}

	protected HST_EnemyOrderState FindOrder(HST_CampaignState state, string orderId)
	{
		if (!state || orderId.IsEmpty())
			return null;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (order && order.m_sOrderId == orderId)
				return order;
		}
		return null;
	}

	protected int CountStrategicMutations(HST_CampaignState state, string mutationId)
	{
		if (!state || mutationId.IsEmpty())
			return 0;
		int count;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_sMutationId == mutationId)
				count++;
		}
		return count;
	}

	protected int CountManifestId(HST_CampaignState state, string manifestId)
	{
		int count;
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
		{
			if (manifest && manifest.m_sManifestId == manifestId)
				count++;
		}
		return count;
	}

	protected int CountOperationId(HST_CampaignState state, string operationId)
	{
		int count;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (operation && operation.m_sOperationId == operationId)
				count++;
		}
		return count;
	}

	protected int CountBatchId(HST_CampaignState state, string resultId)
	{
		int count;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && batch.m_sResultId == resultId)
				count++;
		}
		return count;
	}

	protected int CountGroupId(HST_CampaignState state, string groupId)
	{
		int count;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && group.m_sGroupId == groupId)
				count++;
		}
		return count;
	}

	protected bool Ready(HST_EnemyQRFOperationProofFixture fixture)
	{
		return fixture && fixture.m_State && fixture.m_Preset && fixture.m_EnemyDirector
			&& fixture.m_Planning && fixture.m_Queue && fixture.m_ExactQRF
			&& fixture.m_Order && fixture.m_Manifest && fixture.m_Admission
			&& fixture.m_Admission.m_bSuccess && fixture.m_Batch && fixture.m_Group && fixture.m_Operation;
	}

	protected string BuildFixtureFailure(HST_EnemyQRFOperationProofFixture fixture)
	{
		if (!fixture)
			return "enemy QRF proof fixture unavailable";
		if (!fixture.m_bDebitAccepted)
			return "enemy QRF proof debit failed: " + fixture.m_sDebitReason;
		if (fixture.m_Admission && !fixture.m_Admission.m_sFailureReason.IsEmpty())
			return "enemy QRF proof admission failed: " + fixture.m_Admission.m_sFailureReason;
		return "enemy QRF proof fixture incomplete";
	}
}
