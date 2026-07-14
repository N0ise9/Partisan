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

class HST_EnemyQRFOperationProofService
{
	static const string PROOF_FACTION_KEY = "US";
	static const string PROOF_SOURCE_ZONE_ID = "enemy_qrf_proof_source";
	static const string PROOF_TARGET_ZONE_ID = "enemy_qrf_proof_target";
	static const int PROOF_ATTACK_COST = 12;
	static const int PROOF_SUPPORT_COST = 8;

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
		report.m_bRestoreExact = schemaExact && restoreReconciled && recordsUnique
			&& identityExact && authorityExact && routeExact && rosterExact && legacyExact
			&& corruption.m_bPartialReceiptQuarantined && corruption.m_bMissingBacklinkRejected;
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
