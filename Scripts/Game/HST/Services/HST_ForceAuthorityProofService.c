#ifdef ENABLE_DIAG
class HST_ForceAuthorityProofReport
{
	string m_sQuantityEvidence;
	string m_sDuplicateEvidence;
	string m_sDeterministicEvidence;
	string m_sCapacityEvidence;
	string m_sStaleEvidence;
	string m_sReservationConflictEvidence;
	string m_sRoundtripEvidence;
	string m_sMigrationEvidence;
	string m_sCatalogEvidence;
	string m_sReconciliationEvidence;
	bool m_bAllQuantitiesExact;
	bool m_bDuplicateIdempotent;
	bool m_bDeterministic;
	bool m_bCapacityRejected;
	bool m_bStaleRejected;
	bool m_bReservationRollback;
	bool m_bRoundtrip;
	bool m_bLegacyMigration;
	bool m_bCatalogExact;
	bool m_bReconciliationExact;
}

class HST_ForceAuthorityProofFixture
{
	ref HST_CampaignState m_AcceptedState;
	ref HST_ForcePlanningService m_AcceptedPlanning;
	ref HST_ForceSpawnQueueService m_AcceptedQueue;
	ref HST_ForceSpawnAdapterService m_AcceptedAdapter;
	ref HST_PhysicalWarService m_AcceptedPhysicalWar;
	ref HST_GarrisonPatrolOperationService m_AcceptedGarrisonPatrolOperations;
	ref HST_EconomyService m_AcceptedEconomy;
	ref HST_GarrisonService m_AcceptedGarrisons;
	ref HST_ResourceLedgerService m_AcceptedLedger;
	ref HST_ForceQuoteResult m_AcceptedQuote;
	string m_sDeterministicHash;
	string m_sDeterministicRoster;
}
class HST_ForceAuthorityProofService
{

	HST_ForceAuthorityProofReport BuildReport()
	{
		HST_ForceAuthorityProofReport report = new HST_ForceAuthorityProofReport();
		HST_CampaignPreset proofPreset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_ForceAuthorityProofFixture fixture = new HST_ForceAuthorityProofFixture();
		ProveExactQuantities(report, proofPreset, fixture);
		ProveDuplicateConfirmation(report, fixture);
		ProveDeterministicManifest(report, proofPreset, fixture);
		ProveCapacityContract(report, proofPreset);
		ProveStaleContext(report, proofPreset);
		ProveReservationRollback(report, proofPreset);
		ProvePersistenceRoundtrip(report, fixture);
		ProveLegacyMigration(report);
		ProveCatalog(report);
		string reconciliationEvidence;
		report.m_bReconciliationExact = CampaignDebugProveInterruptedGarrisonReconciliation(proofPreset, reconciliationEvidence);
		report.m_sReconciliationEvidence = reconciliationEvidence;
		return report;
	}
	protected void ProveExactQuantities(HST_ForceAuthorityProofReport report, HST_CampaignPreset proofPreset, HST_ForceAuthorityProofFixture fixture)
	{
		array<int> quantities = {1, 4, 7, 12};
		bool allQuantitiesExact = true;
		string quantityEvidence;
		foreach (int quantity : quantities)
		{
			HST_CampaignState proofState = CreateCampaignDebugForceAuthorityState(32);
			HST_EconomyService proofEconomy = new HST_EconomyService();
			HST_GarrisonService proofGarrisons = new HST_GarrisonService();
			HST_CampaignEventLogService proofEvents = new HST_CampaignEventLogService();
			HST_ResourceLedgerService proofLedger = new HST_ResourceLedgerService();
			proofLedger.SetEventLogService(proofEvents);
			HST_ForcePlanningService proofPlanning = new HST_ForcePlanningService();
			HST_ForceSpawnQueueService proofQueue = new HST_ForceSpawnQueueService();
			HST_ForceSpawnAdapterService proofAdapter = new HST_ForceSpawnAdapterService();
			HST_PhysicalWarService proofPhysicalWar = new HST_PhysicalWarService();
			HST_GarrisonPatrolOperationService proofGarrisonPatrolOperations = ConfigureCampaignDebugExactGarrisonPlanning(
				proofPlanning,
				proofQueue,
				proofAdapter,
				proofPhysicalWar);
			proofPlanning.SetEventLogService(proofEvents);
			string quoteRequestId = string.Format("force_proof_quote_%1", quantity);
			string confirmRequestId = string.Format("force_proof_confirm_%1", quantity);
			HST_ForceQuoteResult quoteResult = proofPlanning.IssueGarrisonQuote(proofState, proofPreset, "force_proof_actor", "force_proof_zone", quantity, quoteRequestId, false);
			HST_ForceConfirmationResult confirmResult;
			if (quoteResult && quoteResult.m_bSuccess)
				confirmResult = proofPlanning.ConfirmGarrisonQuote(proofState, proofEconomy, proofGarrisons, proofLedger, "force_proof_actor", quoteResult.m_Quote.m_sQuoteId, confirmRequestId);

			HST_GarrisonState garrison = proofState.FindGarrison("force_proof_zone", "FIA");
			HST_ResourceTransactionState moneyTransaction;
			HST_ResourceTransactionState hrTransaction;
			if (quoteResult && quoteResult.m_Quote)
			{
				moneyTransaction = proofState.FindResourceTransaction(quoteResult.m_Quote.m_sMoneyTransactionId);
				hrTransaction = proofState.FindResourceTransaction(quoteResult.m_Quote.m_sHRTransactionId);
			}

			bool exact = quoteResult && quoteResult.m_bSuccess && quoteResult.m_Manifest && quoteResult.m_Quote;
			if (exact)
				exact = quoteResult.m_Manifest.m_iRequestedMemberCount == quantity && quoteResult.m_Manifest.m_iAcceptedMemberCount == quantity && quoteResult.m_Manifest.m_aMembers.Count() == quantity;
			if (exact)
				exact = quoteResult.m_Manifest.m_iMoneyCost == quantity * 50 && quoteResult.m_Manifest.m_iHRCost == quantity && quoteResult.m_Quote.m_iMoneyCost == quantity * 50 && quoteResult.m_Quote.m_iHRCost == quantity;
			if (exact)
				exact = CampaignDebugForceManifestSlotsUnique(quoteResult.m_Manifest) && confirmResult && confirmResult.m_bSuccess && !confirmResult.m_bAlreadyApplied;
			HST_OperationRecordState operation;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			HST_GeneratedRouteState route;
			if (quoteResult && quoteResult.m_Quote)
			{
				operation = proofState.FindOperation(quoteResult.m_Quote.m_sOperationId);
				if (operation)
				{
					batch = proofState.FindForceSpawnResult(operation.m_sSpawnResultId);
					group = proofState.FindActiveGroup(operation.m_sGroupId);
					route = proofState.FindGeneratedRoute(operation.m_sCurrentRouteId);
				}
			}
			if (exact)
				exact = CampaignDebugExactGarrisonAuthorityMatches(
					proofState,
					quoteResult.m_Quote,
					quoteResult.m_Manifest,
					garrison,
					operation,
					batch,
					group,
					route,
					proofQueue,
					quantity);
			if (exact)
				exact = proofState.m_iFactionMoney == 5000 - quantity * 50 && proofState.m_iHR == 100 - quantity && proofState.m_aResourceTransactions.Count() == 2;
			if (exact)
				exact = CampaignDebugForceTransactionMatches(moneyTransaction, quoteResult.m_Quote, quoteResult.m_Manifest, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quantity * 50) && CampaignDebugForceTransactionMatches(hrTransaction, quoteResult.m_Quote, quoteResult.m_Manifest, HST_ResourceLedgerService.RESOURCE_HR, quantity);

			allQuantitiesExact = allQuantitiesExact && exact;
			if (!quantityEvidence.IsEmpty())
				quantityEvidence = quantityEvidence + " | ";
			int evidenceGarrisonCount = -1;
			if (garrison)
				evidenceGarrisonCount = garrison.m_iInfantryCount;
			int evidenceLivingCount = -1;
			if (batch)
				evidenceLivingCount = proofQueue.CountStrategicLivingMemberSlots(batch);
			quantityEvidence = quantityEvidence + string.Format(
				"q%1 exact=%2 money=%3 HR=%4 legacy=%5 living=%6 held=%7 tx=%8",
				quantity,
				exact,
				proofState.m_iFactionMoney,
				proofState.m_iHR,
				evidenceGarrisonCount,
				evidenceLivingCount,
				batch && batch.m_bStrategicProjectionHeld,
				proofState.m_aResourceTransactions.Count());

			if (quantity == 4)
			{
				fixture.m_AcceptedState = proofState;
				fixture.m_AcceptedPlanning = proofPlanning;
				fixture.m_AcceptedQueue = proofQueue;
				fixture.m_AcceptedAdapter = proofAdapter;
				fixture.m_AcceptedPhysicalWar = proofPhysicalWar;
				fixture.m_AcceptedGarrisonPatrolOperations = proofGarrisonPatrolOperations;
				fixture.m_AcceptedEconomy = proofEconomy;
				fixture.m_AcceptedGarrisons = proofGarrisons;
				fixture.m_AcceptedLedger = proofLedger;
				fixture.m_AcceptedQuote = quoteResult;
			}
			if (quantity == 7 && quoteResult && quoteResult.m_bSuccess)
			{
				fixture.m_sDeterministicHash = quoteResult.m_Manifest.m_sManifestHash;
				fixture.m_sDeterministicRoster = BuildCampaignDebugForceRoster(quoteResult.m_Manifest);
			}
		}

		report.m_sQuantityEvidence = quantityEvidence;
		report.m_bAllQuantitiesExact = allQuantitiesExact;
	}

	protected void ProveDuplicateConfirmation(HST_ForceAuthorityProofReport report, HST_ForceAuthorityProofFixture fixture)
	{
		bool duplicateIdempotent;
		string duplicateEvidence = "missing accepted fixture";
		if (fixture.m_AcceptedState && fixture.m_AcceptedPlanning && fixture.m_AcceptedQuote && fixture.m_AcceptedQuote.m_Quote)
		{
			int moneyBeforeDuplicate = fixture.m_AcceptedState.m_iFactionMoney;
			int hrBeforeDuplicate = fixture.m_AcceptedState.m_iHR;
			int transactionsBeforeDuplicate = fixture.m_AcceptedState.m_aResourceTransactions.Count();
			int operationsBeforeDuplicate = fixture.m_AcceptedState.m_aOperations.Count();
			int batchesBeforeDuplicate = fixture.m_AcceptedState.m_aForceSpawnResults.Count();
			int groupsBeforeDuplicate = fixture.m_AcceptedState.m_aActiveGroups.Count();
			int routesBeforeDuplicate = fixture.m_AcceptedState.m_aGeneratedRoutes.Count();
			HST_GarrisonState duplicateGarrison = fixture.m_AcceptedState.FindGarrison("force_proof_zone", "FIA");
			int infantryBeforeDuplicate;
			int manifestLinksBeforeDuplicate;
			if (duplicateGarrison)
			{
				infantryBeforeDuplicate = duplicateGarrison.m_iInfantryCount;
				manifestLinksBeforeDuplicate = duplicateGarrison.m_aAcceptedManifestIds.Count();
			}
			HST_ForceConfirmationResult duplicateResult = fixture.m_AcceptedPlanning.ConfirmGarrisonQuote(fixture.m_AcceptedState, fixture.m_AcceptedEconomy, fixture.m_AcceptedGarrisons, fixture.m_AcceptedLedger, "force_proof_actor", fixture.m_AcceptedQuote.m_Quote.m_sQuoteId, "force_proof_confirm_4_duplicate");
			duplicateGarrison = fixture.m_AcceptedState.FindGarrison("force_proof_zone", "FIA");
			duplicateIdempotent = duplicateResult && duplicateResult.m_bSuccess && duplicateResult.m_bAlreadyApplied;
			duplicateIdempotent = duplicateIdempotent && fixture.m_AcceptedState.m_iFactionMoney == moneyBeforeDuplicate && fixture.m_AcceptedState.m_iHR == hrBeforeDuplicate && fixture.m_AcceptedState.m_aResourceTransactions.Count() == transactionsBeforeDuplicate;
			duplicateIdempotent = duplicateIdempotent && duplicateGarrison && duplicateGarrison.m_iInfantryCount == infantryBeforeDuplicate && duplicateGarrison.m_aAcceptedManifestIds.Count() == manifestLinksBeforeDuplicate;
			duplicateIdempotent = duplicateIdempotent
				&& fixture.m_AcceptedState.m_aOperations.Count() == operationsBeforeDuplicate
				&& fixture.m_AcceptedState.m_aForceSpawnResults.Count() == batchesBeforeDuplicate
				&& fixture.m_AcceptedState.m_aActiveGroups.Count() == groupsBeforeDuplicate
				&& fixture.m_AcceptedState.m_aGeneratedRoutes.Count() == routesBeforeDuplicate;
			int duplicateInfantryEvidence = -1;
			if (duplicateGarrison)
				duplicateInfantryEvidence = duplicateGarrison.m_iInfantryCount;
			duplicateEvidence = string.Format(
				"already %1 | money %2/%3 | HR %4/%5 | tx %6/%7 | legacy infantry %8",
				duplicateResult && duplicateResult.m_bAlreadyApplied,
				moneyBeforeDuplicate,
				fixture.m_AcceptedState.m_iFactionMoney,
				hrBeforeDuplicate,
				fixture.m_AcceptedState.m_iHR,
				transactionsBeforeDuplicate,
				fixture.m_AcceptedState.m_aResourceTransactions.Count(),
				duplicateInfantryEvidence);
			duplicateEvidence = duplicateEvidence + string.Format(
				" | graph %1/%2/%3/%4",
				fixture.m_AcceptedState.m_aOperations.Count(),
				fixture.m_AcceptedState.m_aForceSpawnResults.Count(),
				fixture.m_AcceptedState.m_aActiveGroups.Count(),
				fixture.m_AcceptedState.m_aGeneratedRoutes.Count());
		}

		report.m_sDuplicateEvidence = duplicateEvidence;
		report.m_bDuplicateIdempotent = duplicateIdempotent;
	}

	protected void ProveDeterministicManifest(HST_ForceAuthorityProofReport report, HST_CampaignPreset proofPreset, HST_ForceAuthorityProofFixture fixture)
	{
		HST_CampaignState deterministicState = CreateCampaignDebugForceAuthorityState(32);
		HST_ForcePlanningService deterministicPlanning = new HST_ForcePlanningService();
		ConfigureCampaignDebugExactGarrisonPlanning(deterministicPlanning);
		HST_ForceQuoteResult deterministicQuote = deterministicPlanning.IssueGarrisonQuote(deterministicState, proofPreset, "force_proof_actor", "force_proof_zone", 7, "force_proof_quote_7", false);
		bool deterministic = deterministicQuote && deterministicQuote.m_bSuccess && deterministicQuote.m_Manifest.m_sManifestHash == fixture.m_sDeterministicHash && BuildCampaignDebugForceRoster(deterministicQuote.m_Manifest) == fixture.m_sDeterministicRoster;
		string deterministicActualHash = "missing";
		bool deterministicRosterEqual;
		if (deterministicQuote && deterministicQuote.m_Manifest)
		{
			deterministicActualHash = deterministicQuote.m_Manifest.m_sManifestHash;
			deterministicRosterEqual = BuildCampaignDebugForceRoster(deterministicQuote.m_Manifest) == fixture.m_sDeterministicRoster;
		}
		string deterministicEvidence = string.Format("hash %1/%2 | roster equal %3", fixture.m_sDeterministicHash, deterministicActualHash, deterministicRosterEqual);

		report.m_sDeterministicEvidence = deterministicEvidence;
		report.m_bDeterministic = deterministic;
	}

	protected void ProveCapacityContract(HST_ForceAuthorityProofReport report, HST_CampaignPreset proofPreset)
	{
		HST_CampaignState capacityState = CreateCampaignDebugForceAuthorityState(12);
		HST_GarrisonState capacityGarrison = new HST_GarrisonState();
		capacityGarrison.m_sGarrisonId = HST_StableIdService.BuildGarrisonId("force_proof_zone", "FIA");
		capacityGarrison.m_sZoneId = "force_proof_zone";
		capacityGarrison.m_sFactionKey = "FIA";
		capacityGarrison.m_iInfantryCount = 10;
		capacityState.m_aGarrisons.Insert(capacityGarrison);
		HST_ForcePlanningService capacityPlanning = new HST_ForcePlanningService();
		ConfigureCampaignDebugExactGarrisonPlanning(capacityPlanning);
		HST_ForceQuoteResult capacityQuote = capacityPlanning.IssueGarrisonQuote(capacityState, proofPreset, "force_proof_actor", "force_proof_zone", 4, "force_proof_capacity", false);
		bool capacityRejected = capacityQuote && !capacityQuote.m_bSuccess && capacityQuote.m_sFailureReason.Contains("all-or-nothing capacity conflict") && capacityGarrison.m_iInfantryCount == 10 && capacityState.m_iFactionMoney == 5000 && capacityState.m_iHR == 100 && capacityState.m_aForceQuotes.Count() == 0 && capacityState.m_aForceManifests.Count() == 0 && capacityState.m_aResourceTransactions.Count() == 0;
		string capacityReason = "missing";
		if (capacityQuote)
			capacityReason = capacityQuote.m_sFailureReason;
		string capacityEvidence = string.Format("success %1 | reason %2 | infantry %3 | money %4 | HR %5 | manifests %6 quotes %7 tx %8", capacityQuote && capacityQuote.m_bSuccess, capacityReason, capacityGarrison.m_iInfantryCount, capacityState.m_iFactionMoney, capacityState.m_iHR, capacityState.m_aForceManifests.Count(), capacityState.m_aForceQuotes.Count(), capacityState.m_aResourceTransactions.Count());

		report.m_sCapacityEvidence = capacityEvidence;
		report.m_bCapacityRejected = capacityRejected;
	}

	protected void ProveStaleContext(HST_ForceAuthorityProofReport report, HST_CampaignPreset proofPreset)
	{
		HST_CampaignState staleState = CreateCampaignDebugForceAuthorityState(32);
		HST_EconomyService staleEconomy = new HST_EconomyService();
		HST_GarrisonService staleGarrisons = new HST_GarrisonService();
		HST_ResourceLedgerService staleLedger = new HST_ResourceLedgerService();
		HST_ForcePlanningService stalePlanning = new HST_ForcePlanningService();
		ConfigureCampaignDebugExactGarrisonPlanning(stalePlanning);
		HST_ForceQuoteResult staleQuote = stalePlanning.IssueGarrisonQuote(staleState, proofPreset, "force_proof_actor", "force_proof_zone", 4, "force_proof_stale", false);
		staleState.FindZone("force_proof_zone").m_iActiveInfantryCount = 1;
		HST_ForceConfirmationResult staleConfirm;
		if (staleQuote && staleQuote.m_bSuccess)
			staleConfirm = stalePlanning.ConfirmGarrisonQuote(staleState, staleEconomy, staleGarrisons, staleLedger, "force_proof_actor", staleQuote.m_Quote.m_sQuoteId, "force_proof_stale_confirm");
		bool staleRejected = staleConfirm && !staleConfirm.m_bSuccess && staleConfirm.m_sFailureReason.Contains("context changed") && staleState.m_iFactionMoney == 5000 && staleState.m_iHR == 100 && staleState.m_aGarrisons.Count() == 0 && staleState.m_aResourceTransactions.Count() == 0;
		string staleReason = "missing";
		if (staleConfirm)
			staleReason = staleConfirm.m_sFailureReason;
		string staleEvidence = string.Format("success %1 | reason %2 | money %3 HR %4 | garrisons %5 tx %6", staleConfirm && staleConfirm.m_bSuccess, staleReason, staleState.m_iFactionMoney, staleState.m_iHR, staleState.m_aGarrisons.Count(), staleState.m_aResourceTransactions.Count());

		report.m_sStaleEvidence = staleEvidence;
		report.m_bStaleRejected = staleRejected;
	}

	protected void ProveReservationRollback(HST_ForceAuthorityProofReport report, HST_CampaignPreset proofPreset)
	{
		HST_CampaignState reservationConflictState = CreateCampaignDebugForceAuthorityState(32);
		HST_EconomyService reservationConflictEconomy = new HST_EconomyService();
		HST_GarrisonService reservationConflictGarrisons = new HST_GarrisonService();
		HST_ResourceLedgerService reservationConflictLedger = new HST_ResourceLedgerService();
		HST_ForcePlanningService reservationConflictPlanning = new HST_ForcePlanningService();
		ConfigureCampaignDebugExactGarrisonPlanning(reservationConflictPlanning);
		HST_ForceQuoteResult reservationConflictQuote = reservationConflictPlanning.IssueGarrisonQuote(reservationConflictState, proofPreset, "force_proof_actor", "force_proof_zone", 4, "force_proof_reservation_conflict", false);
		if (reservationConflictQuote && reservationConflictQuote.m_bSuccess)
		{
			HST_ResourceTransactionState conflictingHR = new HST_ResourceTransactionState();
			conflictingHR.m_sTransactionId = reservationConflictQuote.m_Quote.m_sHRTransactionId;
			conflictingHR.m_sCommandRequestId = "conflicting_request";
			conflictingHR.m_sOperationId = "conflicting_operation";
			conflictingHR.m_sResourceType = HST_ResourceLedgerService.RESOURCE_HR;
			conflictingHR.m_iAmount = 999;
			conflictingHR.m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_CANCELLED;
			reservationConflictState.m_aResourceTransactions.Insert(conflictingHR);
		}
		HST_ForceConfirmationResult reservationConflictConfirm;
		if (reservationConflictQuote && reservationConflictQuote.m_bSuccess)
			reservationConflictConfirm = reservationConflictPlanning.ConfirmGarrisonQuote(reservationConflictState, reservationConflictEconomy, reservationConflictGarrisons, reservationConflictLedger, "force_proof_actor", reservationConflictQuote.m_Quote.m_sQuoteId, "force_proof_reservation_conflict_confirm");
		HST_ResourceTransactionState rolledBackMoney;
		if (reservationConflictQuote && reservationConflictQuote.m_Quote)
			rolledBackMoney = reservationConflictState.FindResourceTransaction(reservationConflictQuote.m_Quote.m_sMoneyTransactionId);
		bool reservationRollback = reservationConflictConfirm && !reservationConflictConfirm.m_bSuccess && reservationConflictConfirm.m_sFailureReason.Contains("HR reservation failed") && reservationConflictState.m_iFactionMoney == 5000 && reservationConflictState.m_iHR == 100 && reservationConflictState.m_aGarrisons.Count() == 0 && rolledBackMoney && rolledBackMoney.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_CANCELLED;
		string reservationConflictEvidence = string.Format("success %1 | reason %2 | money %3 HR %4 | garrisons %5 | money terminal %6", reservationConflictConfirm && reservationConflictConfirm.m_bSuccess, reservationConflictConfirm != null, reservationConflictState.m_iFactionMoney, reservationConflictState.m_iHR, reservationConflictState.m_aGarrisons.Count(), rolledBackMoney && rolledBackMoney.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_CANCELLED);

		report.m_sReservationConflictEvidence = reservationConflictEvidence;
		report.m_bReservationRollback = reservationRollback;
	}

	protected void ProvePersistenceRoundtrip(HST_ForceAuthorityProofReport report, HST_ForceAuthorityProofFixture fixture)
	{
		bool roundtrip;
		string roundtripEvidence = "missing accepted fixture";
		if (fixture.m_AcceptedState && fixture.m_AcceptedQueue
			&& fixture.m_AcceptedQuote && fixture.m_AcceptedQuote.m_Manifest
			&& fixture.m_AcceptedQuote.m_Quote)
		{
			HST_OperationRecordState acceptedOperation = fixture.m_AcceptedState.FindOperation(
				fixture.m_AcceptedQuote.m_Quote.m_sOperationId);
			HST_ForceSpawnResultState acceptedBatch;
			if (acceptedOperation)
				acceptedBatch = fixture.m_AcceptedState.FindForceSpawnResult(acceptedOperation.m_sSpawnResultId);
			HST_CampaignSaveData saveData = new HST_CampaignSaveData();
			saveData.Capture(fixture.m_AcceptedState);
			HST_CampaignState restoredState = saveData.Restore();
			HST_ForceManifestState restoredManifest;
			HST_ForceQuoteState restoredQuote;
			HST_OperationRecordState restoredOperation;
			HST_ForceSpawnResultState restoredBatch;
			HST_ActiveGroupState restoredGroup;
			HST_GeneratedRouteState restoredRoute;
			HST_GarrisonState restoredGarrison;
			HST_ResourceTransactionState restoredMoneyTransaction;
			HST_ResourceTransactionState restoredHRTransaction;
			if (restoredState)
			{
				restoredManifest = restoredState.FindForceManifest(fixture.m_AcceptedQuote.m_Manifest.m_sManifestId);
				restoredQuote = restoredState.FindForceQuote(fixture.m_AcceptedQuote.m_Quote.m_sQuoteId);
				restoredOperation = restoredState.FindOperation(fixture.m_AcceptedQuote.m_Quote.m_sOperationId);
				if (restoredOperation)
				{
					restoredBatch = restoredState.FindForceSpawnResult(restoredOperation.m_sSpawnResultId);
					restoredGroup = restoredState.FindActiveGroup(restoredOperation.m_sGroupId);
					restoredRoute = restoredState.FindGeneratedRoute(restoredOperation.m_sCurrentRouteId);
				}
				restoredGarrison = restoredState.FindGarrison("force_proof_zone", "FIA");
				if (restoredQuote)
				{
					restoredMoneyTransaction = restoredState.FindResourceTransaction(restoredQuote.m_sMoneyTransactionId);
					restoredHRTransaction = restoredState.FindResourceTransaction(restoredQuote.m_sHRTransactionId);
				}
			}
			roundtrip = acceptedBatch && restoredState
				&& restoredState.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION
				&& restoredManifest && restoredQuote && restoredOperation && restoredBatch
				&& restoredGroup && restoredRoute && restoredGarrison;
			if (roundtrip)
				roundtrip = restoredManifest != fixture.m_AcceptedQuote.m_Manifest && restoredManifest.m_sManifestHash == fixture.m_AcceptedQuote.m_Manifest.m_sManifestHash && restoredManifest.m_aMembers.Count() == 4 && restoredManifest.m_aMembers[0] != fixture.m_AcceptedQuote.m_Manifest.m_aMembers[0];
			if (roundtrip)
				roundtrip = restoredQuote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED
					&& restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
					&& restoredOperation.m_iContractVersion == HST_GarrisonPatrolOperationService.EXACT_CONTRACT_VERSION
					&& restoredBatch.m_bStrategicProjectionHeld
					&& restoredBatch.m_aSlotResults.Count() == 5
					&& fixture.m_AcceptedQueue.CountStrategicLivingMemberSlots(restoredBatch) == 4;
			if (roundtrip)
				roundtrip = restoredGarrison.m_iInfantryCount == 0
					&& CountCampaignDebugString(restoredGarrison.m_aAcceptedManifestIds, restoredManifest.m_sManifestId) == 1
					&& restoredOperation.m_sCurrentRouteId == restoredRoute.m_sRouteId
					&& restoredGroup.m_sRouteId == restoredRoute.m_sRouteId
					&& restoredMoneyTransaction && restoredHRTransaction
					&& restoredMoneyTransaction.m_sManifestId == restoredManifest.m_sManifestId
					&& restoredHRTransaction.m_sQuoteId == restoredQuote.m_sQuoteId;
			int restoredSchemaEvidence = -1;
			int restoredMemberEvidence = -1;
			int restoredSlotEvidence = -1;
			if (restoredState)
				restoredSchemaEvidence = restoredState.m_iSchemaVersion;
			if (restoredManifest)
				restoredMemberEvidence = restoredManifest.m_aMembers.Count();
			if (restoredBatch)
				restoredSlotEvidence = restoredBatch.m_aSlotResults.Count();
			roundtripEvidence = string.Format(
				"schema %1 | manifest %2 members %3 | quote/operation %4/%5 | held batch %6 slots/living %7/%8",
				restoredSchemaEvidence,
				restoredManifest != null,
				restoredMemberEvidence,
				restoredQuote != null,
				restoredOperation != null,
				restoredBatch && restoredBatch.m_bStrategicProjectionHeld,
				restoredSlotEvidence,
				fixture.m_AcceptedQueue.CountStrategicLivingMemberSlots(restoredBatch));
			roundtripEvidence = roundtripEvidence + string.Format(
				" | route/group %1/%2 | backlink %3",
				restoredRoute != null,
				restoredGroup != null,
				restoredGarrison && restoredManifest
					&& restoredGarrison.m_aAcceptedManifestIds.Contains(restoredManifest.m_sManifestId));
		}

		report.m_sRoundtripEvidence = roundtripEvidence;
		report.m_bRoundtrip = roundtrip;
	}

	protected void ProveLegacyMigration(HST_ForceAuthorityProofReport report)
	{
		HST_CampaignSaveData legacySave = new HST_CampaignSaveData();
		legacySave.m_iSchemaVersion = 42;
		legacySave.m_iCampaignSeed = 4343;
		HST_GarrisonState legacyGarrison = new HST_GarrisonState();
		legacyGarrison.m_sZoneId = "force_legacy_zone";
		legacyGarrison.m_sFactionKey = "FIA";
		legacyGarrison.m_iInfantryCount = 6;
		legacySave.m_aGarrisons.Insert(legacyGarrison);
		HST_CampaignState migratedState = legacySave.Restore();
		HST_GarrisonState migratedGarrison;
		bool migrationEventFound;
		if (migratedState)
		{
			migratedGarrison = migratedState.FindGarrison("force_legacy_zone", "FIA");
			foreach (HST_CampaignEventState eventState : migratedState.m_aCampaignEvents)
			{
				if (eventState && eventState.m_sEventId == "migration_schema43_force_authority")
					migrationEventFound = true;
			}
		}
		bool legacyMigration = migratedState && migratedState.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION && migratedGarrison && migratedGarrison.m_iInfantryCount == 6 && migratedGarrison.m_sGarrisonId == HST_StableIdService.BuildGarrisonId("force_legacy_zone", "FIA") && migratedState.m_aForceManifests.Count() == 0 && migrationEventFound;
		int migratedSchemaEvidence = -1;
		int migratedInfantryEvidence = -1;
		int migratedManifestEvidence = -1;
		string migratedIdEvidence = "missing";
		if (migratedState)
		{
			migratedSchemaEvidence = migratedState.m_iSchemaVersion;
			migratedManifestEvidence = migratedState.m_aForceManifests.Count();
		}
		if (migratedGarrison)
		{
			migratedInfantryEvidence = migratedGarrison.m_iInfantryCount;
			migratedIdEvidence = migratedGarrison.m_sGarrisonId;
		}
		string migrationEvidence = string.Format("schema %1 | infantry %2 | id %3 | manifests %4 | warning %5", migratedSchemaEvidence, migratedInfantryEvidence, migratedIdEvidence, migratedManifestEvidence, migrationEventFound);

		report.m_sMigrationEvidence = migrationEvidence;
		report.m_bLegacyMigration = legacyMigration;
	}

	protected void ProveCatalog(HST_ForceAuthorityProofReport report)
	{
		HST_ForceCatalogService proofCatalog = new HST_ForceCatalogService();
		HST_ForceCatalogValidationResult fiaCatalog = proofCatalog.ValidateFactionCatalog("FIA", true);
		HST_ForceCatalogValidationResult usCatalog = proofCatalog.ValidateFactionCatalog("US", true);
		HST_ForceCatalogValidationResult ussrCatalog = proofCatalog.ValidateFactionCatalog("USSR", true);
		bool catalogExact = fiaCatalog && fiaCatalog.m_bValid && fiaCatalog.m_iGroupEntryCount == 4 && fiaCatalog.m_iGroupSlotCount == 16;
		catalogExact = catalogExact && usCatalog && usCatalog.m_bValid && usCatalog.m_iGroupEntryCount == 4 && usCatalog.m_iGroupSlotCount == 17;
		catalogExact = catalogExact && ussrCatalog && ussrCatalog.m_bValid && ussrCatalog.m_iGroupEntryCount == 4 && ussrCatalog.m_iGroupSlotCount == 14;
		string fiaCatalogEvidence = "missing";
		string usCatalogEvidence = "missing";
		string ussrCatalogEvidence = "missing";
		if (fiaCatalog)
			fiaCatalogEvidence = fiaCatalog.BuildSummary();
		if (usCatalog)
			usCatalogEvidence = usCatalog.BuildSummary();
		if (ussrCatalog)
			ussrCatalogEvidence = ussrCatalog.BuildSummary();
		string catalogEvidence = "FIA " + fiaCatalogEvidence + " | US " + usCatalogEvidence + " | USSR " + ussrCatalogEvidence;
		report.m_sCatalogEvidence = catalogEvidence;
		report.m_bCatalogExact = catalogExact;
	}

	protected bool CampaignDebugProveInterruptedGarrisonReconciliation(HST_CampaignPreset proofPreset, out string evidence)
	{
		evidence = "";
		bool allExact = true;
		for (int stage = 1; stage <= 5; stage++)
		{
			HST_CampaignState state = CreateCampaignDebugForceAuthorityState(32);
			HST_EconomyService economy = new HST_EconomyService();
			HST_GarrisonService garrisons = new HST_GarrisonService();
			HST_ResourceLedgerService ledger = new HST_ResourceLedgerService();
			HST_ForcePlanningService planning = new HST_ForcePlanningService();
			string quoteRequestId = string.Format("force_reconcile_quote_%1", stage);
			string confirmRequestId = string.Format("force_reconcile_confirm_%1", stage);
			HST_ForceQuoteResult issued = planning.IssueGarrisonQuote(state, proofPreset, "force_reconcile_actor", "force_proof_zone", 4, quoteRequestId, false);
			if (!issued || !issued.m_bSuccess || !issued.m_Quote || !issued.m_Manifest)
			{
				allExact = false;
				evidence = evidence + string.Format("stage%1 issue_failed | ", stage);
				continue;
			}
			// This fixture intentionally proves the policy-v1 aggregate reconciliation
			// contract. Newly issued policy-v2 purchases are operation-owned and are
			// covered by HST_GarrisonPatrolOperationProofService instead.
			ConvertCampaignDebugGarrisonQuoteToLegacyPolicy(state, issued.m_Quote, issued.m_Manifest);
			HST_ForceQuoteState quote = issued.m_Quote;
			ledger.ReserveCost(state, economy, quote.m_sMoneyTransactionId, confirmRequestId, quote.m_sOperationId, quote.m_sActorIdentityId, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quote.m_iMoneyCost, "reconcile proof", quote.m_sQuoteId, quote.m_sManifestId);
			if (stage >= 2)
				ledger.ReserveCost(state, economy, quote.m_sHRTransactionId, confirmRequestId, quote.m_sOperationId, quote.m_sActorIdentityId, HST_ResourceLedgerService.RESOURCE_HR, quote.m_iHRCost, "reconcile proof", quote.m_sQuoteId, quote.m_sManifestId);
			if (stage >= 3)
				garrisons.AddManifestForcesExact(state, quote.m_sTargetZoneId, quote.m_sFactionKey, issued.m_Manifest);
			if (stage >= 4)
				ledger.CommitReserved(state, quote.m_sMoneyTransactionId);
			if (stage >= 5)
				ledger.CommitReserved(state, quote.m_sHRTransactionId);

			HST_CampaignSaveData saveData = new HST_CampaignSaveData();
			saveData.Capture(state);
			HST_CampaignState restored = saveData.Restore();
			if (!restored)
			{
				allExact = false;
				evidence = evidence + string.Format("stage%1 restore_failed | ", stage);
				continue;
			}
			HST_EconomyService restoredEconomy = new HST_EconomyService();
			HST_GarrisonService restoredGarrisons = new HST_GarrisonService();
			HST_ResourceLedgerService restoredLedger = new HST_ResourceLedgerService();
			HST_ForcePlanningService restoredPlanning = new HST_ForcePlanningService();
			int reconciled = restoredPlanning.ReconcileInterruptedGarrisonConfirmations(restored, restoredEconomy, restoredGarrisons, restoredLedger);
			HST_ForceQuoteState restoredQuote = restored.FindForceQuote(quote.m_sQuoteId);
			HST_GarrisonState restoredGarrison = restored.FindGarrison(quote.m_sTargetZoneId, quote.m_sFactionKey);
			HST_ResourceTransactionState restoredMoney = restored.FindResourceTransaction(quote.m_sMoneyTransactionId);
			HST_ResourceTransactionState restoredHR = restored.FindResourceTransaction(quote.m_sHRTransactionId);
			bool noLink = !restoredGarrison || !restoredGarrison.m_aAcceptedManifestIds.Contains(quote.m_sManifestId);
			bool moneyTerminal = restoredMoney && restoredMoney.m_eStatus != HST_EResourceTransactionStatus.HST_TRANSACTION_RESERVED && restoredMoney.m_eStatus != HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED;
			bool hrTerminal = stage < 2 || (restoredHR && restoredHR.m_eStatus != HST_EResourceTransactionStatus.HST_TRANSACTION_RESERVED && restoredHR.m_eStatus != HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED);
			bool exact = reconciled == 1 && restoredQuote && restoredQuote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_REJECTED && restored.m_iFactionMoney == 5000 && restored.m_iHR == 100 && noLink && moneyTerminal && hrTerminal;
			allExact = allExact && exact;
			if (!evidence.IsEmpty())
				evidence = evidence + " | ";
			evidence = evidence + string.Format("stage%1 exact=%2 money=%3 HR=%4 link=%5 reconciled=%6", stage, exact, restored.m_iFactionMoney, restored.m_iHR, !noLink, reconciled);
		}
		return allExact;
	}

	protected HST_GarrisonPatrolOperationService ConfigureCampaignDebugExactGarrisonPlanning(
		HST_ForcePlanningService planning,
		HST_ForceSpawnQueueService queue = null,
		HST_ForceSpawnAdapterService adapter = null,
		HST_PhysicalWarService physicalWar = null)
	{
		if (!planning)
			return null;
		ref HST_ForceSpawnQueueService resolvedQueue = queue;
		ref HST_ForceSpawnAdapterService resolvedAdapter = adapter;
		ref HST_PhysicalWarService resolvedPhysicalWar = physicalWar;
		if (!resolvedQueue)
			resolvedQueue = new HST_ForceSpawnQueueService();
		if (!resolvedAdapter)
			resolvedAdapter = new HST_ForceSpawnAdapterService();
		if (!resolvedPhysicalWar)
			resolvedPhysicalWar = new HST_PhysicalWarService();
		HST_GarrisonPatrolOperationService operations = new HST_GarrisonPatrolOperationService();
		operations.SetRuntimeServices(resolvedQueue, resolvedAdapter, resolvedPhysicalWar);
		planning.SetExactGarrisonPatrolAuthorityService(operations);
		return operations;
	}

	protected bool CampaignDebugExactGarrisonAuthorityMatches(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_GarrisonState garrison,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_GeneratedRouteState route,
		HST_ForceSpawnQueueService queue,
		int expectedMembers)
	{
		if (!state || !quote || !manifest || !garrison || !operation || !batch
			|| !group || !route || !queue || expectedMembers <= 0)
			return false;
		bool manifestExact = manifest.m_aGroups.Count() == 1 && manifest.m_aGroups[0]
			&& manifest.m_sPolicyId == HST_GarrisonPatrolOperationService.EXACT_POLICY_ID
			&& manifest.m_sGroupPrefab == manifest.m_aGroups[0].m_sPrefab
			&& manifest.m_aGroups[0].m_iExpectedMemberCount == expectedMembers
			&& manifest.m_aGroups[0].m_sPrefab.Contains("NotSpawned")
			&& manifest.m_aMembers.Count() == expectedMembers;
		bool graphExact = state.m_aOperations.Count() == 1
			&& state.m_aForceSpawnResults.Count() == 1
			&& state.m_aActiveGroups.Count() == 1
			&& state.m_aGeneratedRoutes.Count() == 1
			&& operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL
			&& operation.m_iContractVersion == HST_GarrisonPatrolOperationService.EXACT_CONTRACT_VERSION
			&& operation.m_sQuoteId == quote.m_sQuoteId
			&& operation.m_sManifestId == manifest.m_sManifestId
			&& operation.m_sSpawnResultId == batch.m_sResultId
			&& operation.m_sGroupId == group.m_sGroupId
			&& operation.m_sCurrentRouteId == route.m_sRouteId;
		bool routeExact = route.m_sSourceCategory == HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE
			&& route.m_sSourceLayoutId == operation.m_sOperationId
			&& route.m_sSourceZoneId == quote.m_sTargetZoneId
			&& route.m_sTargetZoneId == quote.m_sTargetZoneId
			&& route.m_aWaypoints.Count() == 4
			&& operation.m_sRouteContractHash != ""
			&& group.m_sRouteId == route.m_sRouteId;
		bool rosterExact = batch.m_bStrategicProjectionHeld
			&& batch.m_iExpectedSlotCount == expectedMembers + 1
			&& batch.m_aSlotResults.Count() == expectedMembers + 1
			&& queue.CountStrategicLivingMemberSlots(batch) == expectedMembers
			&& CampaignDebugHeldBatchMatchesManifest(batch, manifest)
			&& group.m_iInfantryCount == expectedMembers
			&& group.m_iOriginalInfantryCount == expectedMembers
			&& group.m_iDurableLivingInfantryCount == expectedMembers;
		bool garrisonExact = garrison.m_iInfantryCount == 0
			&& CountCampaignDebugString(garrison.m_aAcceptedManifestIds, manifest.m_sManifestId) == 1;
		return manifestExact && graphExact && routeExact && rosterExact && garrisonExact;
	}

	protected bool CampaignDebugHeldBatchMatchesManifest(
		HST_ForceSpawnResultState batch,
		HST_ForceManifestState manifest)
	{
		if (!batch || !manifest || manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
			return false;
		HST_ForceSpawnSlotResultState root = batch.FindSlotResult(manifest.m_aGroups[0].m_sElementId);
		if (!root || root.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_GROUP)
			return false;
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			HST_ForceSpawnSlotResultState slot = batch.FindSlotResult(member.m_sSlotId);
			if (!slot || slot.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				|| slot.m_bCasualtyConfirmed)
				return false;
		}
		return true;
	}

	protected void ConvertCampaignDebugGarrisonQuoteToLegacyPolicy(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		if (!state || !quote || !manifest)
			return;
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		manifest.m_sPolicyId = HST_ForcePlanningService.LEGACY_GARRISON_POLICY_ID;
		quote.m_sPolicyId = HST_ForcePlanningService.LEGACY_GARRISON_POLICY_ID;
		manifest.m_sManifestHash = integrity.BuildManifestHash(manifest);
		quote.m_sManifestHash = manifest.m_sManifestHash;
		HST_ZoneState zone = state.FindZone(quote.m_sTargetZoneId);
		quote.m_sContextHash = integrity.BuildGarrisonContextHash(
			state,
			zone,
			quote.m_sFactionKey,
			HST_ForcePlanningService.LEGACY_GARRISON_POLICY_ID);
	}

	protected HST_CampaignState CreateCampaignDebugForceAuthorityState(int garrisonSlots)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iCampaignSeed = 4343;
		state.m_iElapsedSeconds = 200;
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		state.m_bHQDeployed = true;
		state.m_iFactionMoney = 5000;
		state.m_iHR = 100;
		HST_FactionPoolState factionPool = new HST_FactionPoolState();
		factionPool.m_sFactionKey = "FIA";
		state.m_aFactionPools.Insert(factionPool);
		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = "force_proof_zone";
		zone.m_sDisplayName = "Force Proof Zone";
		zone.m_sOwnerFactionKey = "FIA";
		zone.m_iGarrisonSlots = garrisonSlots;
		zone.m_vPosition = "4000 20 4000";
		state.m_aZones.Insert(zone);
		return state;
	}

	protected bool CampaignDebugForceManifestSlotsUnique(HST_ForceManifestState manifest)
	{
		if (!manifest || manifest.m_aMembers.Count() != manifest.m_iAcceptedMemberCount)
			return false;
		array<string> slotIds = {};
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member || member.m_sSlotId.IsEmpty() || member.m_sPrefab.IsEmpty() || member.m_sRole.IsEmpty() || slotIds.Contains(member.m_sSlotId))
				return false;
			slotIds.Insert(member.m_sSlotId);
		}
		return slotIds.Count() == manifest.m_iAcceptedMemberCount;
	}

	protected bool CampaignDebugForceTransactionMatches(HST_ResourceTransactionState transaction, HST_ForceQuoteState quote, HST_ForceManifestState manifest, string resourceType, int amount)
	{
		return transaction && quote && manifest && transaction.m_sQuoteId == quote.m_sQuoteId && transaction.m_sManifestId == manifest.m_sManifestId && transaction.m_sOperationId == manifest.m_sOperationId && transaction.m_sActorIdentityId == quote.m_sActorIdentityId && transaction.m_sCommandRequestId == quote.m_sConfirmationRequestId && transaction.m_sResourceType == resourceType && transaction.m_iAmount == amount && transaction.m_iRefundedAmount == 0 && transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED;
	}

	protected int CountCampaignDebugString(array<string> values, string expected)
	{
		int count;
		foreach (string value : values)
		{
			if (value == expected)
				count++;
		}
		return count;
	}

	protected string BuildCampaignDebugForceRoster(HST_ForceManifestState manifest)
	{
		if (!manifest)
			return "";
		string roster;
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member)
				continue;
			if (!roster.IsEmpty())
				roster = roster + "|";
			roster = roster + member.m_sCatalogSlotId + ":" + member.m_sPrefab + ":" + member.m_sRole;
		}
		return roster;
	}

	protected bool CampaignDebugAllSpawnSlotsRegistered(HST_ForceSpawnResultState spawnResult)
	{
		if (!spawnResult || spawnResult.m_aSlotResults.Count() != spawnResult.m_iExpectedSlotCount)
			return false;
		foreach (HST_ForceSpawnSlotResultState slotResult : spawnResult.m_aSlotResults)
		{
			if (!slotResult || slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED || slotResult.m_sEntityId.IsEmpty() || !slotResult.m_bFactionVerified || !slotResult.m_bGroupVerified || !slotResult.m_bProjectionVerified)
				return false;
		}
		return true;
	}
}
#endif
