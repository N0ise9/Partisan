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
			if (exact)
				exact = garrison && garrison.m_iInfantryCount == quantity && CountCampaignDebugString(garrison.m_aAcceptedManifestIds, quoteResult.m_Manifest.m_sManifestId) == 1;
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
			quantityEvidence = quantityEvidence + string.Format("q%1 exact=%2 money=%3 HR=%4 roster=%5 tx=%6", quantity, exact, proofState.m_iFactionMoney, proofState.m_iHR, evidenceGarrisonCount, proofState.m_aResourceTransactions.Count());

			if (quantity == 4)
			{
				fixture.m_AcceptedState = proofState;
				fixture.m_AcceptedPlanning = proofPlanning;
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
			int duplicateInfantryEvidence = -1;
			if (duplicateGarrison)
				duplicateInfantryEvidence = duplicateGarrison.m_iInfantryCount;
			duplicateEvidence = string.Format("already %1 | money %2/%3 | HR %4/%5 | tx %6/%7 | infantry %8", duplicateResult && duplicateResult.m_bAlreadyApplied, moneyBeforeDuplicate, fixture.m_AcceptedState.m_iFactionMoney, hrBeforeDuplicate, fixture.m_AcceptedState.m_iHR, transactionsBeforeDuplicate, fixture.m_AcceptedState.m_aResourceTransactions.Count(), duplicateInfantryEvidence);
		}

		report.m_sDuplicateEvidence = duplicateEvidence;
		report.m_bDuplicateIdempotent = duplicateIdempotent;
	}

	protected void ProveDeterministicManifest(HST_ForceAuthorityProofReport report, HST_CampaignPreset proofPreset, HST_ForceAuthorityProofFixture fixture)
	{
		HST_CampaignState deterministicState = CreateCampaignDebugForceAuthorityState(32);
		HST_ForcePlanningService deterministicPlanning = new HST_ForcePlanningService();
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
		if (fixture.m_AcceptedState && fixture.m_AcceptedQuote && fixture.m_AcceptedQuote.m_Manifest && fixture.m_AcceptedQuote.m_Quote)
		{
			HST_ForceSpawnResultState spawnResult = new HST_ForceSpawnResultState();
			spawnResult.m_sResultId = "force_proof_spawn_result";
			spawnResult.m_sManifestId = fixture.m_AcceptedQuote.m_Manifest.m_sManifestId;
			spawnResult.m_sOperationId = fixture.m_AcceptedQuote.m_Manifest.m_sOperationId;
			spawnResult.m_sForceId = "force_proof_force";
			spawnResult.m_sNativeGroupId = "force_proof_native_group";
			spawnResult.m_sProjectionId = "force_proof_projection";
			spawnResult.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
			spawnResult.m_iExpectedSlotCount = fixture.m_AcceptedQuote.m_Manifest.m_aMembers.Count();
			foreach (HST_ForceManifestMemberState member : fixture.m_AcceptedQuote.m_Manifest.m_aMembers)
			{
				HST_ForceSpawnSlotResultState slotResult = new HST_ForceSpawnSlotResultState();
				slotResult.m_sSlotId = member.m_sSlotId;
				slotResult.m_sSlotKind = "member";
				slotResult.m_sEntityId = "force_proof_entity_" + member.m_sSlotId;
				slotResult.m_sNativeGroupId = spawnResult.m_sNativeGroupId;
				slotResult.m_sProjectionId = spawnResult.m_sProjectionId;
				slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
				slotResult.m_bFactionVerified = true;
				slotResult.m_bGroupVerified = true;
				slotResult.m_bProjectionVerified = true;
				spawnResult.m_aSlotResults.Insert(slotResult);
			}
			fixture.m_AcceptedState.m_aForceSpawnResults.Insert(spawnResult);
			HST_CampaignSaveData saveData = new HST_CampaignSaveData();
			saveData.Capture(fixture.m_AcceptedState);
			HST_CampaignState restoredState = saveData.Restore();
			HST_ForceManifestState restoredManifest;
			HST_ForceQuoteState restoredQuote;
			HST_ForceSpawnResultState restoredSpawn;
			HST_GarrisonState restoredGarrison;
			HST_ResourceTransactionState restoredMoneyTransaction;
			HST_ResourceTransactionState restoredHRTransaction;
			if (restoredState)
			{
				restoredManifest = restoredState.FindForceManifest(fixture.m_AcceptedQuote.m_Manifest.m_sManifestId);
				restoredQuote = restoredState.FindForceQuote(fixture.m_AcceptedQuote.m_Quote.m_sQuoteId);
				restoredSpawn = restoredState.FindForceSpawnResult("force_proof_spawn_result");
				restoredGarrison = restoredState.FindGarrison("force_proof_zone", "FIA");
				if (restoredQuote)
				{
					restoredMoneyTransaction = restoredState.FindResourceTransaction(restoredQuote.m_sMoneyTransactionId);
					restoredHRTransaction = restoredState.FindResourceTransaction(restoredQuote.m_sHRTransactionId);
				}
			}
			roundtrip = restoredState && restoredState.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION && restoredManifest && restoredQuote && restoredSpawn && restoredGarrison;
			if (roundtrip)
				roundtrip = restoredManifest != fixture.m_AcceptedQuote.m_Manifest && restoredManifest.m_sManifestHash == fixture.m_AcceptedQuote.m_Manifest.m_sManifestHash && restoredManifest.m_aMembers.Count() == 4 && restoredManifest.m_aMembers[0] != fixture.m_AcceptedQuote.m_Manifest.m_aMembers[0];
			if (roundtrip)
				roundtrip = restoredQuote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED && restoredSpawn.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED && restoredSpawn.m_aSlotResults.Count() == 4 && CampaignDebugAllSpawnSlotsRegistered(restoredSpawn);
			if (roundtrip)
				roundtrip = CountCampaignDebugString(restoredGarrison.m_aAcceptedManifestIds, restoredManifest.m_sManifestId) == 1 && restoredMoneyTransaction && restoredHRTransaction && restoredMoneyTransaction.m_sManifestId == restoredManifest.m_sManifestId && restoredHRTransaction.m_sQuoteId == restoredQuote.m_sQuoteId;
			int restoredSchemaEvidence = -1;
			int restoredMemberEvidence = -1;
			int restoredSlotEvidence = -1;
			if (restoredState)
				restoredSchemaEvidence = restoredState.m_iSchemaVersion;
			if (restoredManifest)
				restoredMemberEvidence = restoredManifest.m_aMembers.Count();
			if (restoredSpawn)
				restoredSlotEvidence = restoredSpawn.m_aSlotResults.Count();
			roundtripEvidence = string.Format("schema %1 | manifest %2 members %3 | quote %4 | spawn %5 slots %6 | accepted garrison provenance %7", restoredSchemaEvidence, restoredManifest != null, restoredMemberEvidence, restoredQuote != null, restoredSpawn != null, restoredSlotEvidence, restoredGarrison && restoredManifest && restoredGarrison.m_aAcceptedManifestIds.Contains(restoredManifest.m_sManifestId));
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
