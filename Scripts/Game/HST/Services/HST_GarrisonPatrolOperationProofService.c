#ifdef ENABLE_DIAG
class HST_GarrisonPatrolOperationProofReport
{
	bool m_bAdmissionExact;
	bool m_bReplayRollbackExact;
	bool m_bRosterProjectionExact;
	bool m_bRouteLoopExact;
	bool m_bProjectionHoldExact;
	bool m_bSettlementExact;
	bool m_bRestoreExact;
	bool m_bCorruptionExact;
	bool m_bMarkerExact;
	string m_sAdmissionEvidence;
	string m_sReplayRollbackEvidence;
	string m_sRosterProjectionEvidence;
	string m_sRouteLoopEvidence;
	string m_sProjectionHoldEvidence;
	string m_sSettlementEvidence;
	string m_sRestoreEvidence;
	string m_sCorruptionEvidence;
	string m_sMarkerEvidence;
}

// Source-only seams expose deterministic lifecycle transitions. Native entity
// creation, adapter handles, waypoint execution, and live retirement remain
// packaged-runtime gates.
class HST_GarrisonPatrolOperationProofHarness : HST_GarrisonPatrolOperationService
{
	bool AdvanceLoopForProof(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_GeneratedRouteState route,
		HST_ActiveGroupState group,
		bool physical = false)
	{
		return AdvanceInfinitePatrolLoop(state, operation, route, group, physical);
	}

	bool UpdateCasualtyHoldForProof(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		out bool casualtyHold,
		out bool casualtyRecovery)
	{
		return UpdateRecentCasualtyHold(
			state,
			operation,
			group,
			casualtyHold,
			casualtyRecovery);
	}
}

class HST_GarrisonPatrolRejectLinkProofHarness : HST_GarrisonService
{
	override HST_GarrisonState LinkExecutableManifestExact(
		HST_CampaignState state,
		string zoneId,
		string factionKey,
		HST_ForceManifestState manifest)
	{
		return null;
	}
}

class HST_GarrisonPatrolOperationProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_EconomyService m_Economy;
	ref HST_GarrisonService m_Garrisons;
	ref HST_ResourceLedgerService m_Ledger;
	ref HST_ForcePlanningService m_Planning;
	ref HST_ForceSpawnQueueService m_Queue;
	ref HST_ForceSpawnAdapterService m_Adapter;
	ref HST_PhysicalWarService m_PhysicalWar;
	ref HST_GarrisonPatrolOperationProofHarness m_Service;
	ref HST_ForceQuoteResult m_Issued;
	ref HST_ForceConfirmationResult m_Confirmation;
	ref HST_ForceQuoteState m_Quote;
	ref HST_ForceManifestState m_Manifest;
	ref HST_GarrisonState m_Garrison;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
	ref HST_GeneratedRouteState m_Route;
	bool m_bPreflightAccepted;
	bool m_bPreflightReadOnly;
	string m_sFailureReason;
}

class HST_GarrisonPatrolOperationProofFixtureFactory
{
	static const string PROOF_ACTOR = "garrison_patrol_proof_actor";
	static const string PROOF_FACTION = "FIA";
	static const string PROOF_ZONE_PREFIX = "garrison_patrol_proof_zone_";

	HST_GarrisonPatrolOperationProofFixture BuildIssuedFixture(
		string suffix,
		int memberCount,
		HST_GarrisonService garrisons = null)
	{
		HST_GarrisonPatrolOperationProofFixture fixture = new HST_GarrisonPatrolOperationProofFixture();
		fixture.m_State = BuildState(suffix);
		fixture.m_Preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		fixture.m_Economy = new HST_EconomyService();
		fixture.m_Garrisons = garrisons;
		if (!fixture.m_Garrisons)
			fixture.m_Garrisons = new HST_GarrisonService();
		fixture.m_Ledger = new HST_ResourceLedgerService();
		fixture.m_Planning = new HST_ForcePlanningService();
		fixture.m_Queue = new HST_ForceSpawnQueueService();
		fixture.m_Adapter = new HST_ForceSpawnAdapterService();
		fixture.m_PhysicalWar = new HST_PhysicalWarService();
		fixture.m_Service = new HST_GarrisonPatrolOperationProofHarness();
		fixture.m_Service.SetRuntimeServices(
			fixture.m_Queue,
			fixture.m_Adapter,
			fixture.m_PhysicalWar);
		fixture.m_Planning.SetExactGarrisonPatrolAuthorityService(fixture.m_Service);

		string requestId = "garrison_patrol_proof_quote_" + suffix;
		fixture.m_Issued = fixture.m_Planning.IssueGarrisonQuote(
			fixture.m_State,
			fixture.m_Preset,
			PROOF_ACTOR,
			BuildZoneId(suffix),
			memberCount,
			requestId,
			false);
		if (!fixture.m_Issued || !fixture.m_Issued.m_bSuccess
			|| !fixture.m_Issued.m_Quote || !fixture.m_Issued.m_Manifest)
		{
			fixture.m_sFailureReason = "exact garrison patrol proof quote failed";
			if (fixture.m_Issued && !fixture.m_Issued.m_sFailureReason.IsEmpty())
				fixture.m_sFailureReason = fixture.m_sFailureReason + ": " + fixture.m_Issued.m_sFailureReason;
			return fixture;
		}
		fixture.m_Quote = fixture.m_Issued.m_Quote;
		fixture.m_Manifest = fixture.m_Issued.m_Manifest;

		int operationsBefore = fixture.m_State.m_aOperations.Count();
		int batchesBefore = fixture.m_State.m_aForceSpawnResults.Count();
		int groupsBefore = fixture.m_State.m_aActiveGroups.Count();
		int routesBefore = fixture.m_State.m_aGeneratedRoutes.Count();
		int garrisonsBefore = fixture.m_State.m_aGarrisons.Count();
		int moneyBefore = fixture.m_State.m_iFactionMoney;
		int hrBefore = fixture.m_State.m_iHR;
		HST_GarrisonPatrolAdmissionResult preflight = fixture.m_Service.CanAdmitPreparedPurchase(
			fixture.m_State,
			fixture.m_Quote,
			fixture.m_Manifest,
			fixture.m_Garrisons,
			"garrison_patrol_proof_confirm_" + suffix);
		fixture.m_bPreflightAccepted = preflight && preflight.m_bSuccess;
		fixture.m_bPreflightReadOnly = fixture.m_State.m_aOperations.Count() == operationsBefore
			&& fixture.m_State.m_aForceSpawnResults.Count() == batchesBefore
			&& fixture.m_State.m_aActiveGroups.Count() == groupsBefore
			&& fixture.m_State.m_aGeneratedRoutes.Count() == routesBefore
			&& fixture.m_State.m_aGarrisons.Count() == garrisonsBefore
			&& fixture.m_State.m_iFactionMoney == moneyBefore
			&& fixture.m_State.m_iHR == hrBefore;
		if (!fixture.m_bPreflightAccepted)
		{
			fixture.m_sFailureReason = "exact garrison patrol proof admission preflight failed";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				fixture.m_sFailureReason = fixture.m_sFailureReason + ": " + preflight.m_sFailureReason;
		}
		return fixture;
	}

	HST_GarrisonPatrolOperationProofFixture BuildAcceptedFixture(
		string suffix,
		int memberCount)
	{
		HST_GarrisonPatrolOperationProofFixture fixture = BuildIssuedFixture(suffix, memberCount);
		if (!Issued(fixture))
			return fixture;
		fixture.m_Confirmation = fixture.m_Planning.ConfirmGarrisonQuote(
			fixture.m_State,
			fixture.m_Economy,
			fixture.m_Garrisons,
			fixture.m_Ledger,
			PROOF_ACTOR,
			fixture.m_Quote.m_sQuoteId,
			"garrison_patrol_proof_confirm_" + suffix);
		ResolveCommittedRows(fixture);
		if (!fixture.m_Confirmation || !fixture.m_Confirmation.m_bSuccess)
		{
			fixture.m_sFailureReason = "exact garrison patrol proof confirmation failed";
			if (fixture.m_Confirmation && !fixture.m_Confirmation.m_sFailureReason.IsEmpty())
				fixture.m_sFailureReason = fixture.m_sFailureReason + ": " + fixture.m_Confirmation.m_sFailureReason;
		}
		return fixture;
	}

	bool Issued(HST_GarrisonPatrolOperationProofFixture fixture)
	{
		return fixture && fixture.m_State && fixture.m_Preset && fixture.m_Planning
			&& fixture.m_Queue && fixture.m_Adapter && fixture.m_PhysicalWar
			&& fixture.m_Service && fixture.m_Quote && fixture.m_Manifest
			&& fixture.m_bPreflightAccepted && fixture.m_bPreflightReadOnly;
	}

	bool Ready(HST_GarrisonPatrolOperationProofFixture fixture)
	{
		return Issued(fixture) && fixture.m_Confirmation
			&& fixture.m_Confirmation.m_bSuccess
			&& fixture.m_Garrison && fixture.m_Operation && fixture.m_Batch
			&& fixture.m_Group && fixture.m_Route;
	}

	string Failure(HST_GarrisonPatrolOperationProofFixture fixture)
	{
		if (!fixture)
			return "exact garrison patrol proof fixture is unavailable";
		if (!fixture.m_sFailureReason.IsEmpty())
			return fixture.m_sFailureReason;
		return "exact garrison patrol proof fixture is incomplete";
	}

	void ResolveCommittedRows(HST_GarrisonPatrolOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_State || !fixture.m_Quote)
			return;
		fixture.m_Garrison = fixture.m_State.FindGarrison(
			fixture.m_Quote.m_sTargetZoneId,
			fixture.m_Quote.m_sFactionKey);
		fixture.m_Operation = fixture.m_State.FindOperation(fixture.m_Quote.m_sOperationId);
		if (!fixture.m_Operation)
			return;
		fixture.m_Batch = fixture.m_State.FindForceSpawnResult(fixture.m_Operation.m_sSpawnResultId);
		fixture.m_Group = fixture.m_State.FindActiveGroup(fixture.m_Operation.m_sGroupId);
		fixture.m_Route = fixture.m_State.FindGeneratedRoute(fixture.m_Operation.m_sCurrentRouteId);
	}

	protected HST_CampaignState BuildState(string suffix)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iCampaignSeed = 545454;
		state.m_iElapsedSeconds = 200;
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		state.m_bHQDeployed = true;
		state.m_iFactionMoney = 10000;
		state.m_iHR = 100;
		HST_FactionPoolState pool = new HST_FactionPoolState();
		pool.m_sFactionKey = PROOF_FACTION;
		state.m_aFactionPools.Insert(pool);
		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = BuildZoneId(suffix);
		zone.m_sDisplayName = "Garrison Patrol Proof " + suffix;
		zone.m_sOwnerFactionKey = PROOF_FACTION;
		zone.m_eType = HST_EZoneType.HST_ZONE_TOWN;
		zone.m_iGarrisonSlots = 64;
		zone.m_vPosition = "4000 20 4000";
		state.m_aZones.Insert(zone);
		return state;
	}

	static string BuildZoneId(string suffix)
	{
		return PROOF_ZONE_PREFIX + suffix;
	}
}

class HST_GarrisonPatrolOperationProofService
{
	protected ref HST_GarrisonPatrolOperationProofFixtureFactory m_Fixtures = new HST_GarrisonPatrolOperationProofFixtureFactory();

	HST_GarrisonPatrolOperationProofReport Run()
	{
		HST_GarrisonPatrolOperationProofReport report = new HST_GarrisonPatrolOperationProofReport();
		ProveAdmission(report);
		ProveReplayCollisionAndRollback(report);
		ProveRosterFoldAndReprojection(report);
		ProveInfiniteRouteLoop(report);
		ProveProjectionHysteresisAndCasualtyHold(report);
		ProveTerminalSettlement(report);
		ProveRestoreNormalization(report);
		ProveCorruptGraphQuarantine(report);
		ProveMarkers(report);
		return report;
	}

	protected void ProveAdmission(HST_GarrisonPatrolOperationProofReport report)
	{
		array<int> quantities = {1, 7, 32};
		bool allExact = true;
		string evidence;
		foreach (int quantity : quantities)
		{
			HST_GarrisonPatrolOperationProofFixture fixture = m_Fixtures.BuildAcceptedFixture(
				string.Format("admission_%1", quantity),
				quantity);
			bool exact = m_Fixtures.Ready(fixture) && AdmissionGraphExact(fixture, quantity);
			allExact = allExact && exact;
			if (!evidence.IsEmpty())
				evidence = evidence + " | ";
			if (!m_Fixtures.Ready(fixture))
				evidence = evidence + string.Format("q%1 failed %2", quantity, m_Fixtures.Failure(fixture));
			else
			{
				evidence = evidence + string.Format(
					"q%1 exact=%2 root=%3 slots/living=%4/%5 legacy/backlink=%6/%7",
					quantity,
					exact,
					fixture.m_Manifest.m_aGroups[0].m_sCatalogEntryId,
					fixture.m_Batch.m_aSlotResults.Count(),
					fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch),
					fixture.m_Garrison.m_iInfantryCount,
					CountString(fixture.m_Garrison.m_aAcceptedManifestIds, fixture.m_Manifest.m_sManifestId));
			}
		}
		report.m_bAdmissionExact = allExact;
		report.m_sAdmissionEvidence = evidence;
	}

	protected void ProveReplayCollisionAndRollback(HST_GarrisonPatrolOperationProofReport report)
	{
		HST_GarrisonPatrolOperationProofFixture replay = m_Fixtures.BuildAcceptedFixture("replay", 7);
		if (!m_Fixtures.Ready(replay))
		{
			report.m_sReplayRollbackEvidence = m_Fixtures.Failure(replay);
			return;
		}
		int moneyBeforeReplay = replay.m_State.m_iFactionMoney;
		int hrBeforeReplay = replay.m_State.m_iHR;
		int graphBeforeReplay = CountRuntimeGraphRows(replay.m_State);
		HST_ForceConfirmationResult repeated = replay.m_Planning.ConfirmGarrisonQuote(
			replay.m_State,
			replay.m_Economy,
			replay.m_Garrisons,
			replay.m_Ledger,
			HST_GarrisonPatrolOperationProofFixtureFactory.PROOF_ACTOR,
			replay.m_Quote.m_sQuoteId,
			"garrison_patrol_proof_confirm_replay_again");
		bool replayExact = repeated && repeated.m_bSuccess && repeated.m_bAlreadyApplied
			&& replay.m_State.m_iFactionMoney == moneyBeforeReplay
			&& replay.m_State.m_iHR == hrBeforeReplay
			&& CountRuntimeGraphRows(replay.m_State) == graphBeforeReplay
			&& CountString(replay.m_Garrison.m_aAcceptedManifestIds, replay.m_Manifest.m_sManifestId) == 1;

		HST_GarrisonPatrolOperationProofFixture collision = m_Fixtures.BuildIssuedFixture("collision", 7);
		bool collisionExact;
		if (m_Fixtures.Issued(collision))
		{
			HST_OperationRecordState foreign = new HST_OperationRecordState();
			foreign.m_sOperationId = collision.m_Quote.m_sOperationId;
			foreign.m_eType = HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_QRF;
			collision.m_State.m_aOperations.Insert(foreign);
			int moneyBeforeCollision = collision.m_State.m_iFactionMoney;
			int hrBeforeCollision = collision.m_State.m_iHR;
			HST_ForceConfirmationResult rejected = collision.m_Planning.ConfirmGarrisonQuote(
				collision.m_State,
				collision.m_Economy,
				collision.m_Garrisons,
				collision.m_Ledger,
				HST_GarrisonPatrolOperationProofFixtureFactory.PROOF_ACTOR,
				collision.m_Quote.m_sQuoteId,
				"garrison_patrol_proof_confirm_collision");
			collisionExact = rejected && !rejected.m_bSuccess
				&& collision.m_State.m_iFactionMoney == moneyBeforeCollision
				&& collision.m_State.m_iHR == hrBeforeCollision
				&& collision.m_State.m_aOperations.Count() == 1
				&& collision.m_State.m_aForceSpawnResults.Count() == 0
				&& collision.m_State.m_aActiveGroups.Count() == 0
				&& collision.m_State.m_aGeneratedRoutes.Count() == 0
				&& collision.m_State.m_aResourceTransactions.Count() == 0;
		}

		HST_GarrisonService rejectingGarrison = new HST_GarrisonPatrolRejectLinkProofHarness();
		HST_GarrisonPatrolOperationProofFixture rollback = m_Fixtures.BuildIssuedFixture(
			"rollback",
			7,
			rejectingGarrison);
		bool rollbackExact;
		if (m_Fixtures.Issued(rollback))
		{
			int moneyBeforeRollback = rollback.m_State.m_iFactionMoney;
			int hrBeforeRollback = rollback.m_State.m_iHR;
			HST_ForceConfirmationResult rejectedRollback = rollback.m_Planning.ConfirmGarrisonQuote(
				rollback.m_State,
				rollback.m_Economy,
				rollback.m_Garrisons,
				rollback.m_Ledger,
				HST_GarrisonPatrolOperationProofFixtureFactory.PROOF_ACTOR,
				rollback.m_Quote.m_sQuoteId,
				"garrison_patrol_proof_confirm_rollback");
			rollbackExact = rejectedRollback && !rejectedRollback.m_bSuccess
				&& rollback.m_State.m_iFactionMoney == moneyBeforeRollback
				&& rollback.m_State.m_iHR == hrBeforeRollback
				&& rollback.m_State.m_aOperations.Count() == 0
				&& rollback.m_State.m_aForceSpawnResults.Count() == 0
				&& rollback.m_State.m_aActiveGroups.Count() == 0
				&& rollback.m_State.m_aGeneratedRoutes.Count() == 0
				&& rollback.m_State.m_aGarrisons.Count() == 0
				&& AllReservationsCancelledWithFullRelease(rollback.m_State);
		}

		report.m_bReplayRollbackExact = replayExact && collisionExact && rollbackExact;
		report.m_sReplayRollbackEvidence = string.Format(
			"replay/collision/full-rollback %1/%2/%3 | replay graph %4 | rollback rows op/batch/group/route %5/%6/%7/%8",
			replayExact,
			collisionExact,
			rollbackExact,
			CountRuntimeGraphRows(replay.m_State),
			ResolveOperationCount(rollback),
			ResolveBatchCount(rollback),
			ResolveGroupCount(rollback),
			ResolveRouteCount(rollback));
		report.m_sReplayRollbackEvidence = report.m_sReplayRollbackEvidence + string.Format(
			" | rollback transactions/cancelled-release %1/%2",
			ResolveTransactionCount(rollback),
			rollback && AllReservationsCancelledWithFullRelease(rollback.m_State));
	}

	protected void ProveRosterFoldAndReprojection(HST_GarrisonPatrolOperationProofReport report)
	{
		HST_GarrisonPatrolOperationProofFixture fixture = m_Fixtures.BuildAcceptedFixture("roster", 7);
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sRosterProjectionEvidence = m_Fixtures.Failure(fixture);
			return;
		}
		int moneyAfterPurchase = fixture.m_State.m_iFactionMoney;
		int hrAfterPurchase = fixture.m_State.m_iHR;
		bool casualtiesExact = ConfirmStrategicCasualties(fixture, 6, "roster");
		int virtualLiving = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		int virtualDead = fixture.m_Queue.CountConfirmedCasualtyMemberSlots(fixture.m_Batch);
		bool firstRelease = ReleaseHeldProjection(fixture, 220);
		bool firstProjection = firstRelease && CompleteProjection(fixture, 221, "r1");
		int physicalLiving = fixture.m_Queue.CountDurableLivingMemberSlots(fixture.m_Batch);
		HST_ForceSpawnQueueCallbackResult fold = fixture.m_Queue.RequeueSuccessfulProjectionForStrategicHold(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			224,
			404);
		int foldedLiving = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		bool noFoldRefund = fixture.m_State.m_iFactionMoney == moneyAfterPurchase
			&& fixture.m_State.m_iHR == hrAfterPurchase
			&& AllTransactionsCommittedWithoutRefund(fixture.m_State);
		bool secondRelease = ReleaseHeldProjection(fixture, 225);
		bool secondProjection = secondRelease && CompleteProjection(fixture, 226, "r2");
		int reprojectedLiving = fixture.m_Queue.CountDurableLivingMemberSlots(fixture.m_Batch);
		int reprojectedDead = fixture.m_Queue.CountConfirmedCasualtyMemberSlots(fixture.m_Batch);
		bool survivorOnly = CountRegisteredMemberSlots(fixture.m_Batch) == 1;
		bool tombstonesExact = CountExactCasualtyTombstones(fixture.m_Batch) == 6;
		bool callbacksExact = casualtiesExact && firstProjection && fold && fold.m_bAccepted
			&& secondProjection;
		bool countsExact = virtualLiving == 1 && virtualDead == 6
			&& physicalLiving == 1 && foldedLiving == 1
			&& reprojectedLiving == 1 && reprojectedDead == 6;
		bool lifecycleExact = fixture.m_Batch.m_iSuccessfulHandoffCount == 2
			&& fixture.m_Batch.m_iReprojectionCount == 1
			&& survivorOnly && tombstonesExact;
		report.m_bRosterProjectionExact = callbacksExact && countsExact
			&& lifecycleExact && noFoldRefund;
		report.m_sRosterProjectionEvidence = string.Format(
			"casualties/release1/project1/fold/release2/project2 %1/%2/%3/%4/%5/%6",
			casualtiesExact,
			firstRelease,
			firstProjection,
			fold && fold.m_bAccepted,
			secondRelease,
			secondProjection);
		report.m_sRosterProjectionEvidence = report.m_sRosterProjectionEvidence + string.Format(
			" | virtual/physical/folded/reprojected %1/%2/%3/%4 | dead %5 | handoffs/reprojections %6/%7 | no-refund %8",
			virtualLiving,
			physicalLiving,
			foldedLiving,
			reprojectedLiving,
			reprojectedDead,
			fixture.m_Batch.m_iSuccessfulHandoffCount,
			fixture.m_Batch.m_iReprojectionCount,
			noFoldRefund);
	}

	protected void ProveInfiniteRouteLoop(HST_GarrisonPatrolOperationProofReport report)
	{
		HST_GarrisonPatrolOperationProofFixture fixture = m_Fixtures.BuildAcceptedFixture("route_loop", 4);
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sRouteLoopEvidence = m_Fixtures.Failure(fixture);
			return;
		}
		ref array<vector> positions = HST_OperationRouteCursorService.BuildOrderedRoutePositions(fixture.m_Route);
		int endpointChanges;
		vector previousEndpoint = fixture.m_Operation.m_vRouteEndPosition;
		bool allAdvanced = true;
		for (int advanceIndex = 0; advanceIndex < 9; advanceIndex++)
		{
			fixture.m_State.m_iElapsedSeconds++;
			bool advanced = fixture.m_Service.AdvanceLoopForProof(
				fixture.m_State,
				fixture.m_Operation,
				fixture.m_Route,
				fixture.m_Group,
				false);
			allAdvanced = allAdvanced && advanced;
			if (fixture.m_Operation.m_vRouteEndPosition != previousEndpoint)
				endpointChanges++;
			previousEndpoint = fixture.m_Operation.m_vRouteEndPosition;
		}
		HST_OperationRouteCursorService cursor = new HST_OperationRouteCursorService();
		bool contractExact = cursor.IsPatrolRouteContractValid(fixture.m_Operation, fixture.m_Route);
		bool infiniteExact = positions.Count() == 4 && allAdvanced
			&& fixture.m_Operation.m_iRouteLapCount >= 2
			&& fixture.m_Operation.m_iRouteWaypointIndex >= 0
			&& fixture.m_Operation.m_iRouteWaypointIndex < positions.Count()
			&& fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& fixture.m_Batch.m_bStrategicProjectionHeld
			&& endpointChanges >= positions.Count();
		report.m_bRouteLoopExact = contractExact && infiniteExact;
		report.m_sRouteLoopEvidence = string.Format(
			"route points/advances/endpoint changes %1/%2/%3 | lap/waypoint/leg %4/%5/%6 | contract/open %7/%8",
			positions.Count(),
			allAdvanced,
			endpointChanges,
			fixture.m_Operation.m_iRouteLapCount,
			fixture.m_Operation.m_iRouteWaypointIndex,
			fixture.m_Operation.m_iRouteLegSequence,
			contractExact,
			fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN);
	}

	protected void ProveProjectionHysteresisAndCasualtyHold(HST_GarrisonPatrolOperationProofReport report)
	{
		HST_GarrisonPatrolOperationProofFixture fixture = m_Fixtures.BuildAcceptedFixture("projection_hold", 4);
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sProjectionHoldEvidence = m_Fixtures.Failure(fixture);
			return;
		}
		HST_MaterializationService materialization = new HST_MaterializationService();
		HST_OperationProjectionDecision enter = materialization.EvaluateExactPlayerQRFForProximity(
			fixture.m_Operation,
			true,
			true,
			1800.0,
			2160.0);
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		fixture.m_Operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		fixture.m_Operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
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
		bool hysteresisExact = enter
			&& enter.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE
			&& band
			&& band.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_RETAIN
			&& leave
			&& leave.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_DEMATERIALIZE;

		fixture.m_State.m_iElapsedSeconds = 300;
		fixture.m_Group.m_iLastCasualtySecond = 300;
		bool active;
		bool recovery;
		bool entered = fixture.m_Service.UpdateCasualtyHoldForProof(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Group,
			active,
			recovery);
		bool casualtyOnlyEntered = entered && active && !recovery
			&& fixture.m_Operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT
			&& fixture.m_Group.m_sRuntimeStatus == "garrison_patrol_casualty_hold";
		fixture.m_State.m_iElapsedSeconds += HST_GarrisonPatrolOperationService.CASUALTY_CLEAR_SECONDS;
		bool retainedActive;
		bool retainedRecovery;
		fixture.m_Service.UpdateCasualtyHoldForProof(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Group,
			retainedActive,
			retainedRecovery);
		fixture.m_State.m_iElapsedSeconds++;
		bool clearedActive;
		bool clearedRecovery;
		bool cleared = fixture.m_Service.UpdateCasualtyHoldForProof(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Group,
			clearedActive,
			clearedRecovery);
		bool casualtyOnlyCleared = retainedActive && !retainedRecovery
			&& cleared && !clearedActive && clearedRecovery
			&& fixture.m_Operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		fixture.m_Operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT;
		HST_OperationProjectionDecision contactLeave = materialization.EvaluateExactPlayerQRFForProximity(
			fixture.m_Operation,
			false,
			false,
			1800.0,
			2160.0);
		bool contactRetained = contactLeave
			&& contactLeave.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_RETAIN;
		report.m_bProjectionHoldExact = hysteresisExact && casualtyOnlyEntered
			&& casualtyOnlyCleared && contactRetained;
			report.m_sProjectionHoldEvidence = string.Format(
			"hysteresis enter/band/leave %1/%2/%3 | casualty enter/retain/clear/contact-retain %4/%5/%6/%7",
			enter.m_eDecision,
			band.m_eDecision,
			leave.m_eDecision,
			casualtyOnlyEntered,
			retainedActive,
			casualtyOnlyCleared,
			contactRetained);
	}

	protected void ProveTerminalSettlement(HST_GarrisonPatrolOperationProofReport report)
	{
		HST_GarrisonPatrolOperationProofFixture owner = m_Fixtures.BuildAcceptedFixture("owner_settlement", 4);
		HST_GarrisonPatrolOperationProofFixture dead = m_Fixtures.BuildAcceptedFixture("dead_settlement", 4);
		if (!m_Fixtures.Ready(owner) || !m_Fixtures.Ready(dead))
		{
			report.m_sSettlementEvidence = m_Fixtures.Failure(owner) + " | " + m_Fixtures.Failure(dead);
			return;
		}
		int ownerMoney = owner.m_State.m_iFactionMoney;
		int ownerHR = owner.m_State.m_iHR;
		HST_ZoneState ownerZone = owner.m_State.FindZone(owner.m_Quote.m_sTargetZoneId);
		ownerZone.m_sOwnerFactionKey = "US";
		owner.m_State.m_iElapsedSeconds++;
		bool ownerSettled = owner.m_Service.Tick(owner.m_State, owner.m_Preset);
		string ownerSettlementId = owner.m_Operation.m_sSettlementId;
		int ownerSettledSecond = owner.m_Operation.m_iSettledAtSecond;
		int ownerRevision = owner.m_Operation.m_iRevision;
		bool ownerReplayChanged = owner.m_Service.Tick(owner.m_State, owner.m_Preset);
		bool ownerExact = ownerSettled
			&& owner.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& owner.m_Operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED
			&& ownerSettlementId == HST_OperationService.BuildSettlementId(
				owner.m_Operation.m_sOperationId,
				HST_GarrisonPatrolOperationService.SETTLEMENT_KIND)
			&& owner.m_Operation.m_sSettlementId == ownerSettlementId
			&& owner.m_Operation.m_iSettledAtSecond == ownerSettledSecond
			&& owner.m_Operation.m_iRevision == ownerRevision
			&& !ownerReplayChanged
			&& owner.m_State.FindForceSpawnResult(owner.m_Batch.m_sResultId) == null
			&& owner.m_State.FindActiveGroup(owner.m_Group.m_sGroupId) == null
			&& owner.m_State.FindGeneratedRoute(owner.m_Route.m_sRouteId) == null
			&& owner.m_State.m_iFactionMoney == ownerMoney
			&& owner.m_State.m_iHR == ownerHR
			&& AllTransactionsCommittedWithoutRefund(owner.m_State);

		int deadMoney = dead.m_State.m_iFactionMoney;
		int deadHR = dead.m_State.m_iHR;
		bool allCasualties = ConfirmStrategicCasualties(dead, 4, "all_dead");
		dead.m_State.m_iElapsedSeconds++;
		bool deadSettled = dead.m_Service.Tick(dead.m_State, dead.m_Preset);
		string deadSettlementId = dead.m_Operation.m_sSettlementId;
		int deadSettledSecond = dead.m_Operation.m_iSettledAtSecond;
		int deadRevision = dead.m_Operation.m_iRevision;
		bool deadReplayChanged = dead.m_Service.Tick(dead.m_State, dead.m_Preset);
		bool deadExact = allCasualties && deadSettled
			&& dead.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& dead.m_Operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED
			&& dead.m_Operation.m_iLastVirtualFriendlyCount == 0
			&& deadSettlementId == HST_OperationService.BuildSettlementId(
				dead.m_Operation.m_sOperationId,
				HST_GarrisonPatrolOperationService.SETTLEMENT_KIND)
			&& dead.m_Operation.m_sSettlementId == deadSettlementId
			&& dead.m_Operation.m_iSettledAtSecond == deadSettledSecond
			&& dead.m_Operation.m_iRevision == deadRevision
			&& !deadReplayChanged
			&& dead.m_State.m_iFactionMoney == deadMoney
			&& dead.m_State.m_iHR == deadHR
			&& AllTransactionsCommittedWithoutRefund(dead.m_State);

		HST_GarrisonPatrolOperationProofFixture missingRuntime = m_Fixtures.BuildAcceptedFixture(
			"owner_missing_runtime",
			4);
		bool missingRuntimeProjected = m_Fixtures.Ready(missingRuntime)
			&& ReleaseHeldProjection(missingRuntime, 40)
			&& CompleteProjection(missingRuntime, 41, "owner_missing_runtime");
		int missingRuntimeLiving;
		int missingRuntimeMoney;
		int missingRuntimeHR;
		bool missingRuntimeQuarantined;
		if (missingRuntimeProjected)
		{
			missingRuntimeLiving = missingRuntime.m_Queue.CountDurableLivingMemberSlots(
				missingRuntime.m_Batch);
			missingRuntimeMoney = missingRuntime.m_State.m_iFactionMoney;
			missingRuntimeHR = missingRuntime.m_State.m_iHR;
			missingRuntime.m_Operation.m_eMaterializationState
				= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
			missingRuntime.m_Operation.m_ePositionAuthority
				= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
			missingRuntime.m_Group.m_bSpawnedEntity = false;
			missingRuntime.m_Group.m_bSpawnAttempted = false;
			missingRuntime.m_Group.m_sRuntimeEntityId = "";
			missingRuntime.m_Group.m_iSpawnedAgentCount = missingRuntimeLiving;
			missingRuntime.m_Group.m_sRuntimeStatus = "garrison_patrol_physical";
			HST_ZoneState missingRuntimeZone = missingRuntime.m_State.FindZone(
				missingRuntime.m_Quote.m_sTargetZoneId);
			missingRuntimeZone.m_sOwnerFactionKey = "US";
			missingRuntime.m_State.m_iElapsedSeconds++;
			bool quarantineChanged = missingRuntime.m_Service.Tick(
				missingRuntime.m_State,
				missingRuntime.m_Preset);
			missingRuntimeQuarantined = quarantineChanged
				&& missingRuntime.m_Operation.m_iContractVersion
					== HST_GarrisonPatrolOperationService.QUARANTINED_CONTRACT_VERSION
				&& missingRuntime.m_Operation.m_eSettlementState
					== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				&& missingRuntime.m_Operation.m_eTerminalResult
					== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
				&& missingRuntime.m_State.FindForceSpawnResult(missingRuntime.m_Batch.m_sResultId)
					== missingRuntime.m_Batch
				&& missingRuntime.m_State.FindActiveGroup(missingRuntime.m_Group.m_sGroupId)
					== missingRuntime.m_Group
				&& missingRuntime.m_State.FindGeneratedRoute(missingRuntime.m_Route.m_sRouteId)
					== missingRuntime.m_Route
				&& missingRuntime.m_Garrison.m_aAcceptedManifestIds.Contains(
					missingRuntime.m_Manifest.m_sManifestId)
				&& missingRuntime.m_Queue.CountDurableLivingMemberSlots(missingRuntime.m_Batch)
					== missingRuntimeLiving
				&& missingRuntime.m_State.m_iFactionMoney == missingRuntimeMoney
				&& missingRuntime.m_State.m_iHR == missingRuntimeHR
				&& AllTransactionsCommittedWithoutRefund(missingRuntime.m_State);
		}

		report.m_bSettlementExact = ownerExact && deadExact && missingRuntimeQuarantined;
		report.m_sSettlementEvidence = string.Format(
			"owner changed settled/replay/no-refund %1/%2/%3 | all-dead casualty/settled/replay/no-refund %4/%5/%6/%7",
			ownerExact,
			!ownerReplayChanged,
			AllTransactionsCommittedWithoutRefund(owner.m_State),
			allCasualties,
			deadExact,
			!deadReplayChanged,
			AllTransactionsCommittedWithoutRefund(dead.m_State));
		report.m_sSettlementEvidence = report.m_sSettlementEvidence + string.Format(
			" | missing-runtime projected/living/quarantined %1/%2/%3",
			missingRuntimeProjected,
			missingRuntimeLiving,
			missingRuntimeQuarantined);
	}

	protected void ProveRestoreNormalization(HST_GarrisonPatrolOperationProofReport report)
	{
		HST_GarrisonPatrolOperationProofFixture fixture = m_Fixtures.BuildAcceptedFixture("restore", 7);
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sRestoreEvidence = m_Fixtures.Failure(fixture);
			return;
		}
		bool casualty = ConfirmStrategicCasualties(fixture, 1, "restore");
		bool released = ReleaseHeldProjection(fixture, 240);
		bool projected = released && CompleteProjection(fixture, 241, "restore");
		int livingBefore = fixture.m_Queue.CountDurableLivingMemberSlots(fixture.m_Batch);
		int deadBefore = fixture.m_Queue.CountConfirmedCasualtyMemberSlots(fixture.m_Batch);
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		fixture.m_Operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		fixture.m_Group.m_bSpawnAttempted = true;
		fixture.m_Group.m_bSpawnedEntity = true;
		fixture.m_Group.m_sRuntimeEntityId = "garrison_patrol_proof_stale_runtime";
		fixture.m_Group.m_iSpawnedAgentCount = livingBefore;
		fixture.m_Group.m_iInfantryCount = livingBefore;
		fixture.m_Group.m_iLastSeenAliveCount = livingBefore;
		fixture.m_Group.m_iSurvivorInfantryCount = livingBefore;
		fixture.m_Group.m_iDurableLivingInfantryCount = livingBefore;
		fixture.m_Group.m_sRuntimeStatus = "garrison_patrol_physical";
		fixture.m_Group.m_vPosition = fixture.m_Operation.m_vStrategicPosition + "10 0 10";
		vector savedLivePosition = fixture.m_Group.m_vPosition;
		fixture.m_State.m_bRestoredFromPersistence = true;
		fixture.m_State.m_iPersistenceRestoreSequence = 54;
		fixture.m_State.m_iForceSpawnQueueReconciledRestoreSequence = 53;
		int moneyBefore = fixture.m_State.m_iFactionMoney;
		int hrBefore = fixture.m_State.m_iHR;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_ForceSpawnQueueService restoredQueue = new HST_ForceSpawnQueueService();
		HST_GarrisonPatrolOperationService restoredService = new HST_GarrisonPatrolOperationService();
		if (restored)
		{
			restoredQueue.ReconcileCampaignAfterRestore(restored);
			restoredService.SetRuntimeServices(
				restoredQueue,
				new HST_ForceSpawnAdapterService(),
				new HST_PhysicalWarService());
			restoredService.ReconcileAfterRestore(restored);
		}
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		HST_GeneratedRouteState route;
		HST_GarrisonState garrison;
		if (restored)
		{
			operation = restored.FindOperation(fixture.m_Operation.m_sOperationId);
			manifest = restored.FindForceManifest(fixture.m_Manifest.m_sManifestId);
			batch = restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
			group = restored.FindActiveGroup(fixture.m_Group.m_sGroupId);
			route = restored.FindGeneratedRoute(fixture.m_Route.m_sRouteId);
			garrison = restored.FindGarrison(fixture.m_Quote.m_sTargetZoneId, fixture.m_Quote.m_sFactionKey);
		}
		int livingAfter = restoredQueue.CountStrategicLivingMemberSlots(batch);
		int deadAfter = restoredQueue.CountConfirmedCasualtyMemberSlots(batch);
		bool recordsExact = operation && manifest && batch && group && route && garrison;
		bool authorityExact = recordsExact
			&& operation.m_iContractVersion == HST_GarrisonPatrolOperationService.EXACT_CONTRACT_VERSION
			&& operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& batch.m_bStrategicProjectionHeld
			&& batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			&& group.m_sRuntimeEntityId.IsEmpty()
			&& !group.m_bSpawnedEntity
			&& group.m_iSpawnedAgentCount == 0;
		bool rosterExact = livingBefore == 6 && livingAfter == livingBefore
			&& deadBefore == 1 && deadAfter == deadBefore;
		bool routeExact = recordsExact
			&& operation.m_sCurrentRouteId == route.m_sRouteId
			&& group.m_sRouteId == route.m_sRouteId
			&& Distance2D(operation.m_vStrategicPosition, savedLivePosition) < 0.1;
		bool noRefund = restored && restored.m_iFactionMoney == moneyBefore
			&& restored.m_iHR == hrBefore
			&& AllTransactionsCommittedWithoutRefund(restored);

		HST_GarrisonPatrolOperationProofFixture ownerChanged = m_Fixtures.BuildAcceptedFixture(
			"restore_owner_changed",
			4);
		bool ownerRestoreExact;
		bool ownerValidatorPreserved;
		if (m_Fixtures.Ready(ownerChanged))
		{
			int ownerMoney = ownerChanged.m_State.m_iFactionMoney;
			int ownerHR = ownerChanged.m_State.m_iHR;
			HST_ZoneState changedZone = ownerChanged.m_State.FindZone(ownerChanged.m_Quote.m_sTargetZoneId);
			changedZone.m_sOwnerFactionKey = "US";
			HST_CampaignSaveData ownerSave = new HST_CampaignSaveData();
			ownerSave.Capture(ownerChanged.m_State);
			HST_CampaignState ownerRestored = ownerSave.Restore();
			HST_OperationRecordState ownerOperation;
			if (ownerRestored)
				ownerOperation = ownerRestored.FindOperation(ownerChanged.m_Operation.m_sOperationId);
			ownerValidatorPreserved = ownerOperation
				&& ownerOperation.m_iContractVersion == HST_GarrisonPatrolOperationService.EXACT_CONTRACT_VERSION
				&& ownerRestored.FindForceSpawnResult(ownerChanged.m_Batch.m_sResultId)
				&& ownerRestored.FindActiveGroup(ownerChanged.m_Group.m_sGroupId)
				&& ownerRestored.FindGeneratedRoute(ownerChanged.m_Route.m_sRouteId);
			HST_ForceSpawnQueueService ownerQueue = new HST_ForceSpawnQueueService();
			HST_GarrisonPatrolOperationService ownerService = new HST_GarrisonPatrolOperationService();
			if (ownerRestored)
			{
				ownerQueue.ReconcileCampaignAfterRestore(ownerRestored);
				ownerService.SetRuntimeServices(
					ownerQueue,
					new HST_ForceSpawnAdapterService(),
					new HST_PhysicalWarService());
				ownerService.ReconcileAfterRestore(ownerRestored);
			}
			ownerRestoreExact = ownerValidatorPreserved && ownerOperation
				&& ownerOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
				&& ownerOperation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED
				&& ownerOperation.m_sTerminalReason.Contains("ownership changed")
				&& ownerRestored.m_iFactionMoney == ownerMoney
				&& ownerRestored.m_iHR == ownerHR
				&& AllTransactionsCommittedWithoutRefund(ownerRestored);
		}
		report.m_bRestoreExact = casualty && projected && recordsExact
			&& authorityExact && rosterExact && routeExact && noRefund
			&& ownerRestoreExact;
		report.m_sRestoreEvidence = string.Format(
			"casualty/projected/records %1/%2/%3 | living/dead %4/%5 -> %6/%7",
			casualty,
			projected,
			recordsExact,
			livingBefore,
			deadBefore,
			livingAfter,
			deadAfter);
		report.m_sRestoreEvidence = report.m_sRestoreEvidence + string.Format(
			" | held/virtual/ids-cleared %1/%2/%3 | route/no-refund %4/%5 | owner validator/settlement %6/%7",
			batch && batch.m_bStrategicProjectionHeld,
			operation && operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL,
			group && group.m_sRuntimeEntityId.IsEmpty(),
			routeExact,
			noRefund,
			ownerValidatorPreserved,
			ownerRestoreExact);
	}

	protected void ProveCorruptGraphQuarantine(HST_GarrisonPatrolOperationProofReport report)
	{
		string routeEvidence;
		string backlinkEvidence;
		string terminalEvidence;
		bool routeExact = ProveRouteQuarantine(routeEvidence);
		bool backlinkExact = ProveBacklinkQuarantine(backlinkEvidence);
		bool terminalExact = ProveTypedQuarantineTerminalization(terminalEvidence);
		string capacityEvidence;
		bool capacityExact = ProveForeignSettlementCapacityReservation(capacityEvidence);
		report.m_bCorruptionExact = routeExact && backlinkExact && terminalExact && capacityExact;
		report.m_sCorruptionEvidence = "route " + routeEvidence
			+ " | backlink " + backlinkEvidence
			+ " | terminal " + terminalEvidence
			+ " | capacity " + capacityEvidence;
	}

	protected bool ProveForeignSettlementCapacityReservation(out string evidence)
	{
		HST_GarrisonPatrolOperationProofFixture fixture = m_Fixtures.BuildAcceptedFixture(
			"foreign_settlement_capacity",
			4);
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = m_Fixtures.Failure(fixture);
			return false;
		}

		int ownedOperationIndex = fixture.m_State.m_aOperations.Find(fixture.m_Operation);
		if (ownedOperationIndex >= 0)
			fixture.m_State.m_aOperations.Remove(ownedOperationIndex);
		HST_OperationRecordState foreignSettlement = new HST_OperationRecordState();
		foreignSettlement.m_sOperationId = "foreign_settled_garrison_patrol";
		foreignSettlement.m_sQuoteId = "foreign_settled_quote";
		foreignSettlement.m_sManifestId = "foreign_settled_manifest";
		foreignSettlement.m_sOwnerFactionKey = fixture.m_Quote.m_sFactionKey;
		foreignSettlement.m_sAssignmentZoneId = fixture.m_Quote.m_sTargetZoneId;
		foreignSettlement.m_sAssignmentKind = HST_GarrisonPatrolOperationService.ASSIGNMENT_KIND;
		foreignSettlement.m_sSettlementPolicyId = HST_GarrisonPatrolOperationService.SETTLEMENT_POLICY_ID;
		foreignSettlement.m_eType = HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL;
		foreignSettlement.m_iContractVersion = HST_GarrisonPatrolOperationService.EXACT_CONTRACT_VERSION;
		foreignSettlement.m_iProjectionContractVersion
			= HST_GarrisonPatrolOperationService.EXACT_PROJECTION_CONTRACT_VERSION;
		foreignSettlement.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		foreignSettlement.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		foreignSettlement.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		foreignSettlement.m_eMaterializationState
			= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
		foreignSettlement.m_ePositionAuthority
			= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		foreignSettlement.m_eSettlementState
			= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		foreignSettlement.m_eTerminalResult
			= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED;
		foreignSettlement.m_sSettlementId = HST_OperationService.BuildSettlementId(
			foreignSettlement.m_sOperationId,
			HST_GarrisonPatrolOperationService.SETTLEMENT_KIND);
		fixture.m_State.m_aOperations.Insert(foreignSettlement);
		fixture.m_Manifest.m_sOperationId = foreignSettlement.m_sOperationId;

		int reserved = fixture.m_Garrisons.CountExecutableManifestInfantry(
			fixture.m_State,
			fixture.m_Garrison);
		bool exact = reserved == fixture.m_Manifest.m_iAcceptedMemberCount;
		evidence = string.Format(
			"foreign settled claimant reserved/accepted %1/%2 exact=%3",
			reserved,
			fixture.m_Manifest.m_iAcceptedMemberCount,
			exact);
		return exact;
	}

	protected bool ProveTypedQuarantineTerminalization(out string evidence)
	{
		array<int> casualtyCounts = {1, 3};
		bool allExact = true;
		foreach (int casualtyCount : casualtyCounts)
		{
			HST_GarrisonPatrolOperationProofFixture fixture = m_Fixtures.BuildAcceptedFixture(
				string.Format("quarantine_terminal_%1", casualtyCount),
				3);
			bool exact = m_Fixtures.Ready(fixture)
				&& ReleaseHeldProjection(fixture, 20)
				&& CompleteProjection(fixture, 21, string.Format("quarantine_%1", casualtyCount));
			if (exact)
				exact = ConfirmPhysicalQuarantineCasualties(fixture, casualtyCount, 24);
			HST_ForceSpawnQueueCallbackResult terminal;
			if (exact)
			{
				terminal = fixture.m_Queue.CompleteQuarantinedSuccessfulProjectionCancellation(
					fixture.m_State.m_aForceSpawnResults,
					fixture.m_Batch.m_sResultId,
					fixture.m_Batch.m_sProjectionId,
					30,
					"exact garrison patrol proof typed quarantine");
				exact = terminal && terminal.m_bAccepted
					&& fixture.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED
					&& fixture.m_Queue.CountConfirmedCasualtyMemberSlots(fixture.m_Batch) == casualtyCount
					&& QuarantineTerminalSlotsExact(fixture.m_Batch);
			}
			if (exact)
			{
				fixture.m_Group.m_sRuntimeStatus = "exact_garrison_patrol_quarantined";
				fixture.m_Group.m_sSpawnFallbackMode = HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE + "_quarantined";
				exact = !fixture.m_State.IsOperationalActiveGroup(fixture.m_Group)
					&& !fixture.m_State.IsCombatPresentActiveGroup(fixture.m_Group);
			}
			allExact = allExact && exact;
			if (!evidence.IsEmpty())
				evidence = evidence + " | ";
			evidence = evidence + string.Format(
				"dead%1 cancelled/tombstones/nonoperational=%2/%3/%4",
				casualtyCount,
				fixture.m_Batch && fixture.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED,
				fixture.m_Batch && fixture.m_Queue.CountConfirmedCasualtyMemberSlots(fixture.m_Batch) == casualtyCount,
				fixture.m_Group && !fixture.m_State.IsOperationalActiveGroup(fixture.m_Group));
		}
		return allExact;
	}

	protected bool ConfirmPhysicalQuarantineCasualties(
		HST_GarrisonPatrolOperationProofFixture fixture,
		int casualtyCount,
		int nowSecond)
	{
		if (!fixture || !fixture.m_Batch || !fixture.m_Manifest
			|| casualtyCount < 0 || casualtyCount > fixture.m_Manifest.m_aMembers.Count())
			return false;
		for (int index = 0; index < casualtyCount; index++)
		{
			HST_ForceManifestMemberState member = fixture.m_Manifest.m_aMembers[index];
			HST_ForceSpawnSlotResultState slot = fixture.m_Batch.FindSlotResult(member.m_sSlotId);
			if (!slot || slot.m_sEntityId.IsEmpty())
				return false;
			HST_ForceSpawnQueueCallbackResult casualty = fixture.m_Queue.ConfirmQuarantinedRegisteredMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				slot.m_sSlotId,
				slot.m_sEntityId,
				nowSecond + index,
				"exact garrison patrol proof physical quarantine casualty");
			if (!casualty || !casualty.m_bAccepted)
				return false;
		}
		return true;
	}

	protected bool QuarantineTerminalSlotsExact(HST_ForceSpawnResultState batch)
	{
		if (!batch || !batch.m_sNativeGroupId.IsEmpty())
			return false;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || !slot.m_sEntityId.IsEmpty() || !slot.m_sNativeGroupId.IsEmpty())
				return false;
			if (slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				&& slot.m_bCasualtyConfirmed)
			{
				if (slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED)
					return false;
				continue;
			}
			if (slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CANCELLED)
				return false;
		}
		return true;
	}

	protected bool ProveRouteQuarantine(out string evidence)
	{
		HST_GarrisonPatrolOperationProofFixture fixture = m_Fixtures.BuildAcceptedFixture("corrupt_route", 4);
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = m_Fixtures.Failure(fixture);
			return false;
		}
		int moneyBefore = fixture.m_State.m_iFactionMoney;
		int hrBefore = fixture.m_State.m_iHR;
		fixture.m_Operation.m_sRouteContractHash = "garrison_patrol_proof_forged_route_hash";
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		bool exact = QuarantinedGraphPreserved(restored, fixture)
			&& restored.m_iFactionMoney == moneyBefore
			&& restored.m_iHR == hrBefore
			&& AllTransactionsCommittedWithoutRefund(restored);
		evidence = string.Format(
			"quarantined/preserved/no-refund %1/%2/%3",
			ResolveQuarantined(restored, fixture),
			QuarantinedGraphPreserved(restored, fixture),
			restored && AllTransactionsCommittedWithoutRefund(restored));
		return exact;
	}

	protected bool ProveBacklinkQuarantine(out string evidence)
	{
		HST_GarrisonPatrolOperationProofFixture fixture = m_Fixtures.BuildAcceptedFixture("corrupt_backlink", 4);
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = m_Fixtures.Failure(fixture);
			return false;
		}
		int moneyBefore = fixture.m_State.m_iFactionMoney;
		int hrBefore = fixture.m_State.m_iHR;
		fixture.m_Group.m_sOperationId = "garrison_patrol_proof_foreign_operation";
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_ActiveGroupState restoredGroup;
		if (restored)
			restoredGroup = restored.FindActiveGroup(fixture.m_Group.m_sGroupId);
		bool backlinkPreserved = restoredGroup
			&& restoredGroup.m_sOperationId == "garrison_patrol_proof_foreign_operation";
		bool exact = QuarantinedGraphPreserved(restored, fixture)
			&& backlinkPreserved
			&& restored.m_iFactionMoney == moneyBefore
			&& restored.m_iHR == hrBefore
			&& AllTransactionsCommittedWithoutRefund(restored);
		evidence = string.Format(
			"quarantined/preserved/backlink/no-refund %1/%2/%3/%4",
			ResolveQuarantined(restored, fixture),
			QuarantinedGraphPreserved(restored, fixture),
			backlinkPreserved,
			restored && AllTransactionsCommittedWithoutRefund(restored));
		return exact;
	}

	protected void ProveMarkers(HST_GarrisonPatrolOperationProofReport report)
	{
		HST_GarrisonPatrolOperationProofFixture fixture = m_Fixtures.BuildAcceptedFixture("markers", 7);
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sMarkerEvidence = m_Fixtures.Failure(fixture);
			return;
		}
		bool casualty = ConfirmStrategicCasualties(fixture, 1, "marker");
		int living = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		HST_MapMarkerService markers = new HST_MapMarkerService();
		markers.RebuildAllMarkers(fixture.m_State, fixture.m_Preset);
		string markerId = "hst_exact_garrison_patrol_" + fixture.m_Operation.m_sOperationId;
		HST_MapMarkerState marker = fixture.m_State.FindMapMarker(markerId);
		bool markerExists = marker != null;
		bool positionExact = marker && IsExpectedMarkerPresentationPosition(
			marker.m_vPosition,
			fixture.m_Operation.m_vStrategicPosition,
			markerId);
		bool uniqueExact = CountMarkerId(fixture.m_State, markerId) == 1;
		bool openExact = marker && marker.m_bVisible
			&& marker.m_sCategory == "strategic"
			&& marker.m_sOwnerFactionKey == fixture.m_Operation.m_sOwnerFactionKey
			&& marker.m_sLabel.Contains("garrison patrol")
			&& marker.m_sLabel.Contains(string.Format("%1 survivors", living))
			&& positionExact
			&& uniqueExact;
		HST_ZoneState zone = fixture.m_State.FindZone(fixture.m_Quote.m_sTargetZoneId);
		zone.m_sOwnerFactionKey = "US";
		fixture.m_State.m_iElapsedSeconds++;
		fixture.m_Service.Tick(fixture.m_State, fixture.m_Preset);
		markers.RebuildAllMarkers(fixture.m_State, fixture.m_Preset);
		bool terminalCleanup = fixture.m_State.FindMapMarker(markerId) == null
			&& CountMarkerId(fixture.m_State, markerId) == 0;
		report.m_bMarkerExact = casualty && living == 6 && openExact && terminalCleanup;
		report.m_sMarkerEvidence = string.Format(
			"casualty/living/open/position/unique/terminal-cleanup %1/%2/%3/%4/%5/%6",
			casualty,
			living,
			markerExists,
			positionExact,
			uniqueExact,
			terminalCleanup);
	}

	protected bool AdmissionGraphExact(
		HST_GarrisonPatrolOperationProofFixture fixture,
		int expectedMembers)
	{
		if (!m_Fixtures.Ready(fixture) || expectedMembers <= 0
			|| fixture.m_Manifest.m_aGroups.Count() != 1
			|| !fixture.m_Manifest.m_aGroups[0])
			return false;
		HST_ForceManifestGroupState root = fixture.m_Manifest.m_aGroups[0];
		bool rootExact = fixture.m_Manifest.m_sPolicyId == HST_GarrisonPatrolOperationService.EXACT_POLICY_ID
			&& root.m_sPrefab == fixture.m_Manifest.m_sGroupPrefab
			&& root.m_sPrefab.Contains("NotSpawned")
			&& root.m_iExpectedMemberCount == expectedMembers
			&& fixture.m_Manifest.m_aMembers.Count() == expectedMembers;
		bool graphExact = fixture.m_State.m_aOperations.Count() == 1
			&& fixture.m_State.m_aForceSpawnResults.Count() == 1
			&& fixture.m_State.m_aActiveGroups.Count() == 1
			&& fixture.m_State.m_aGeneratedRoutes.Count() == 1
			&& fixture.m_Operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL
			&& fixture.m_Operation.m_iContractVersion == HST_GarrisonPatrolOperationService.EXACT_CONTRACT_VERSION
			&& fixture.m_Operation.m_sQuoteId == fixture.m_Quote.m_sQuoteId
			&& fixture.m_Operation.m_sManifestId == fixture.m_Manifest.m_sManifestId
			&& fixture.m_Operation.m_sSpawnResultId == fixture.m_Batch.m_sResultId
			&& fixture.m_Operation.m_sGroupId == fixture.m_Group.m_sGroupId
			&& fixture.m_Operation.m_sCurrentRouteId == fixture.m_Route.m_sRouteId;
		bool routeExact = fixture.m_Route.m_sSourceCategory == HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE
			&& fixture.m_Route.m_sSourceLayoutId == fixture.m_Operation.m_sOperationId
			&& fixture.m_Route.m_aWaypoints.Count() == 4
			&& !fixture.m_Operation.m_sRouteContractHash.IsEmpty()
			&& fixture.m_Group.m_sRouteId == fixture.m_Route.m_sRouteId;
		bool rosterExact = fixture.m_Batch.m_bStrategicProjectionHeld
			&& fixture.m_Batch.m_iExpectedSlotCount == expectedMembers + 1
			&& fixture.m_Batch.m_aSlotResults.Count() == expectedMembers + 1
			&& fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch) == expectedMembers
			&& fixture.m_Group.m_iInfantryCount == expectedMembers
			&& fixture.m_Group.m_iOriginalInfantryCount == expectedMembers;
		bool purchaseExact = fixture.m_Confirmation && fixture.m_Confirmation.m_bSuccess
			&& !fixture.m_Confirmation.m_bAlreadyApplied
			&& fixture.m_Garrison.m_iInfantryCount == 0
			&& CountString(fixture.m_Garrison.m_aAcceptedManifestIds, fixture.m_Manifest.m_sManifestId) == 1
			&& AllTransactionsCommittedWithoutRefund(fixture.m_State);
		return fixture.m_bPreflightAccepted && fixture.m_bPreflightReadOnly
			&& rootExact && graphExact && routeExact && rosterExact && purchaseExact;
	}

	protected bool ConfirmStrategicCasualties(
		HST_GarrisonPatrolOperationProofFixture fixture,
		int casualtyCount,
		string reasonToken)
	{
		if (!m_Fixtures.Ready(fixture) || casualtyCount < 0
			|| casualtyCount > fixture.m_Manifest.m_aMembers.Count())
			return false;
		for (int index = 0; index < casualtyCount; index++)
		{
			HST_ForceManifestMemberState member = fixture.m_Manifest.m_aMembers[index];
			HST_ForceSpawnQueueCallbackResult casualty = fixture.m_Queue.ConfirmStrategicMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				member.m_sSlotId,
				fixture.m_State.m_iElapsedSeconds + index + 1,
				"exact garrison patrol proof casualty " + reasonToken);
			if (!casualty || !casualty.m_bAccepted)
				return false;
		}
		return fixture.m_Queue.CountConfirmedCasualtyMemberSlots(fixture.m_Batch) == casualtyCount;
	}

	protected bool ReleaseHeldProjection(
		HST_GarrisonPatrolOperationProofFixture fixture,
		int nowSecond)
	{
		if (!m_Fixtures.Ready(fixture))
			return false;
		HST_ForceSpawnQueueCallbackResult released = fixture.m_Queue.ReleaseStrategicProjectionForMaterialization(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			nowSecond,
			nowSecond + HST_GarrisonPatrolOperationService.DEPLOYMENT_GRACE_SECONDS);
		return released && released.m_bAccepted && !fixture.m_Batch.m_bStrategicProjectionHeld;
	}

	protected bool CompleteProjection(
		HST_GarrisonPatrolOperationProofFixture fixture,
		int nowSecond,
		string generationToken)
	{
		if (!m_Fixtures.Ready(fixture) || fixture.m_Batch.m_bStrategicProjectionHeld)
			return false;
		HST_ForceSpawnQueueTickResult rootTick = fixture.m_Queue.AcquireWork(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_State.m_aForceManifests,
			nowSecond);
		if (!rootTick || !rootTick.m_aWorkItems || rootTick.m_aWorkItems.Count() != 1
			|| rootTick.m_aWorkItems[0].m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_GROUP)
			return false;
		CompleteWork(fixture, rootTick.m_aWorkItems[0], nowSecond, generationToken);
		HST_ForceSpawnQueueTickResult memberTick = fixture.m_Queue.AcquireWork(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_State.m_aForceManifests,
			nowSecond + 1);
		if (!memberTick || !memberTick.m_aWorkItems || memberTick.m_aWorkItems.Count() <= 0)
			return false;
		foreach (HST_ForceSpawnQueueWorkItem work : memberTick.m_aWorkItems)
		{
			if (!work || work.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
				return false;
			CompleteWork(fixture, work, nowSecond + 1, generationToken);
		}
		HST_ForceSpawnQueueCallbackResult handoff = fixture.m_Queue.CompleteProjectionHandoff(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			fixture.m_Batch.m_iAttemptGeneration,
			nowSecond + 2);
		return handoff && handoff.m_bAccepted
			&& fixture.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
	}

	protected void CompleteWork(
		HST_GarrisonPatrolOperationProofFixture fixture,
		HST_ForceSpawnQueueWorkItem work,
		int nowSecond,
		string generationToken)
	{
		if (!fixture || !work)
			return;
		HST_ForceSpawnQueueSlotSuccess success = new HST_ForceSpawnQueueSlotSuccess();
		success.m_sResultId = work.m_sResultId;
		success.m_sProjectionId = work.m_sProjectionId;
		success.m_sSlotId = work.m_sSlotId;
		success.m_sEntityId = string.Format("garrison_patrol_proof_%1_%2_entity", generationToken, work.m_sSlotId);
		success.m_sSpawnedPrefab = work.m_sPrefab;
		success.m_sNativeGroupId = "garrison_patrol_proof_native_" + generationToken;
		success.m_iAttemptGeneration = work.m_iAttemptGeneration;
		success.m_bAliveVerified = true;
		success.m_bFactionVerified = true;
		success.m_bGroupVerified = true;
		success.m_bGameMasterVerified = true;
		success.m_bProjectionVerified = true;
		fixture.m_Queue.CompleteSlotSuccess(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			success,
			nowSecond);
	}

	protected int CountRegisteredMemberSlots(HST_ForceSpawnResultState batch)
	{
		int count;
		if (!batch)
			return count;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				&& slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED)
				count++;
		}
		return count;
	}

	protected int CountExactCasualtyTombstones(HST_ForceSpawnResultState batch)
	{
		int count;
		if (!batch)
			return count;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				&& slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				&& slot.m_bEverAlive && slot.m_bCasualtyConfirmed
				&& slot.m_sEntityId.IsEmpty())
				count++;
		}
		return count;
	}

	protected bool AllTransactionsCommittedWithoutRefund(HST_CampaignState state)
	{
		if (!state || state.m_aResourceTransactions.Count() != 2)
			return false;
		foreach (HST_ResourceTransactionState transaction : state.m_aResourceTransactions)
		{
			if (!transaction
				|| transaction.m_eStatus != HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED
				|| transaction.m_iRefundedAmount != 0)
				return false;
		}
		return true;
	}

	protected bool AllReservationsCancelledWithFullRelease(HST_CampaignState state)
	{
		if (!state || state.m_aResourceTransactions.Count() != 2)
			return false;
		foreach (HST_ResourceTransactionState transaction : state.m_aResourceTransactions)
		{
			if (!transaction || transaction.m_iAmount <= 0
				|| transaction.m_iRefundedAmount != transaction.m_iAmount
				|| transaction.m_eStatus != HST_EResourceTransactionStatus.HST_TRANSACTION_CANCELLED
				|| transaction.m_sLastSettlementId.IsEmpty())
				return false;
		}
		return true;
	}

	protected bool QuarantinedGraphPreserved(
		HST_CampaignState restored,
		HST_GarrisonPatrolOperationProofFixture fixture)
	{
		if (!restored || !fixture || !ResolveQuarantined(restored, fixture))
			return false;
		HST_ForceSpawnResultState batch
			= restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
		HST_ActiveGroupState group
			= restored.FindActiveGroup(fixture.m_Group.m_sGroupId);
		return restored.FindForceManifest(fixture.m_Manifest.m_sManifestId)
			&& batch && batch.m_bStrategicProjectionHeld && batch.m_bCancelRequested
			&& group && !group.m_bSpawnedEntity && group.m_sRuntimeEntityId.IsEmpty()
			&& group.m_sSpawnFallbackMode
				== HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE + "_quarantined"
			&& group.m_sRuntimeStatus == "exact_garrison_patrol_quarantined"
			&& restored.FindGeneratedRoute(fixture.m_Route.m_sRouteId);
	}

	protected bool ResolveQuarantined(
		HST_CampaignState restored,
		HST_GarrisonPatrolOperationProofFixture fixture)
	{
		if (!restored || !fixture || !fixture.m_Operation)
			return false;
		HST_OperationRecordState operation = restored.FindOperation(fixture.m_Operation.m_sOperationId);
		return operation
			&& operation.m_iContractVersion == HST_GarrisonPatrolOperationService.QUARANTINED_CONTRACT_VERSION
			&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& !operation.m_sLastProjectionReason.IsEmpty();
	}

	protected bool IsExpectedMarkerPresentationPosition(
		vector actual,
		vector authorityPosition,
		string markerId)
	{
		if (markerId.IsEmpty())
			return false;
		if (Distance2D(actual, authorityPosition) < 0.1)
			return true;

		// Strategic markers intentionally deconflict from the zone marker at the
		// same authority position.  Accept only one of the deterministic offsets
		// produced by HST_MapMarkerService; this still fails if the marker follows
		// a stale or unrelated position.
		for (int attempt = 0; attempt < HST_MapMarkerService.MARKER_DECONFLICT_ATTEMPTS; attempt++)
		{
			int baseSlot = markerId.Length() + attempt;
			int ring = baseSlot / 8;
			int slot = baseSlot - ring * 8;
			float x = 1.0;
			float z = 0.0;
			if (slot == 1)
			{
				x = 0.707;
				z = 0.707;
			}
			else if (slot == 2)
			{
				x = 0.0;
				z = 1.0;
			}
			else if (slot == 3)
			{
				x = -0.707;
				z = 0.707;
			}
			else if (slot == 4)
				x = -1.0;
			else if (slot == 5)
			{
				x = -0.707;
				z = -0.707;
			}
			else if (slot == 6)
			{
				x = 0.0;
				z = -1.0;
			}
			else if (slot == 7)
			{
				x = 0.707;
				z = -0.707;
			}

			float distance = HST_MapMarkerService.MARKER_DECONFLICT_STEP_METERS
				+ ring * 10.0;
			vector expected = authorityPosition;
			expected[0] = expected[0] + x * distance;
			expected[2] = expected[2] + z * distance;
			if (Distance2D(actual, expected) < 0.25)
				return true;
		}
		return false;
	}

	protected int CountRuntimeGraphRows(HST_CampaignState state)
	{
		if (!state)
			return -1;
		return state.m_aOperations.Count() + state.m_aForceSpawnResults.Count()
			+ state.m_aActiveGroups.Count() + state.m_aGeneratedRoutes.Count();
	}

	protected int ResolveOperationCount(HST_GarrisonPatrolOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_State)
			return -1;
		return fixture.m_State.m_aOperations.Count();
	}

	protected int ResolveBatchCount(HST_GarrisonPatrolOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_State)
			return -1;
		return fixture.m_State.m_aForceSpawnResults.Count();
	}

	protected int ResolveGroupCount(HST_GarrisonPatrolOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_State)
			return -1;
		return fixture.m_State.m_aActiveGroups.Count();
	}

	protected int ResolveRouteCount(HST_GarrisonPatrolOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_State)
			return -1;
		return fixture.m_State.m_aGeneratedRoutes.Count();
	}

	protected int ResolveTransactionCount(HST_GarrisonPatrolOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_State)
			return -1;
		return fixture.m_State.m_aResourceTransactions.Count();
	}

	protected int CountString(array<string> values, string expected)
	{
		int count;
		if (!values || expected.IsEmpty())
			return count;
		foreach (string value : values)
		{
			if (value == expected)
				count++;
		}
		return count;
	}

	protected int CountMarkerId(HST_CampaignState state, string markerId)
	{
		int count;
		if (!state || markerId.IsEmpty())
			return count;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (marker && !marker.m_bTombstone && marker.m_bVisible && marker.m_sMarkerId == markerId)
				count++;
		}
		return count;
	}

	protected float Distance2D(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}
}
#endif
