#ifdef ENABLE_DIAG
class HST_ForceSettlementArchiveProofReport
{
	bool m_bGarrisonArchiveReplayExact;
	bool m_bSupportArchiveReplayExact;
	bool m_bBacklinkProtectionExact;
	bool m_bRetentionCapacityExact;
	bool m_bPersistenceExact;
	bool m_bSchema47MigrationExact;
	string m_sGarrisonEvidence;
	string m_sSupportEvidence;
	string m_sBacklinkEvidence;
	string m_sCapacityEvidence;
	string m_sPersistenceEvidence;
	string m_sMigrationEvidence;
}

class HST_ForceSettlementArchiveProofService
{
	HST_ForceSettlementArchiveProofReport Run()
	{
		HST_ForceSettlementArchiveProofReport report = new HST_ForceSettlementArchiveProofReport();
		HST_CampaignState archivedGarrisonState = ProveGarrisonArchiveReplay(report);
		ProveSupportArchiveReplay(report);
		ProveBacklinkProtection(report);
		ProveRetentionCapacity(report);
		ProvePersistence(report, archivedGarrisonState);
		ProveSchema47Migration(report);
		return report;
	}

	protected HST_CampaignState ProveGarrisonArchiveReplay(HST_ForceSettlementArchiveProofReport report)
	{
		HST_CampaignState state = BuildBaseState();
		HST_ForceManifestState manifest = BuildManifest("archive_garrison", 4);
		HST_ForceQuoteState quote = BuildAcceptedQuote(manifest, HST_ForcePlanningService.QUOTE_KIND_GARRISON, 4);
		state.m_aForceManifests.Insert(manifest);
		state.m_aForceQuotes.Insert(quote);
		AppendTransactions(state, quote, 0);
		HST_GarrisonState garrison = new HST_GarrisonState();
		garrison.m_sGarrisonId = "garrison_archive_zone_FIA";
		garrison.m_sZoneId = quote.m_sTargetZoneId;
		garrison.m_sFactionKey = quote.m_sFactionKey;
		garrison.m_iInfantryCount = 4;
		garrison.m_aAcceptedManifestIds.Insert(manifest.m_sManifestId);
		state.m_aGarrisons.Insert(garrison);

		HST_ForceSettlementArchiveService archive = new HST_ForceSettlementArchiveService();
		HST_ForceSettlementArchiveResult archiveResult = archive.ArchiveSettledRecords(state);
		HST_ForcePlanningService planning = new HST_ForcePlanningService();
		HST_EconomyService economy = new HST_EconomyService();
		HST_ResourceLedgerService ledger = new HST_ResourceLedgerService();
		int moneyBeforeLedgerReplay = state.m_iFactionMoney;
		HST_ResourceTransactionResult ledgerReplay = ledger.ReserveCost(
			state,
			economy,
			quote.m_sMoneyTransactionId,
			quote.m_sConfirmationRequestId,
			quote.m_sOperationId,
			quote.m_sActorIdentityId,
			HST_ResourceLedgerService.RESOURCE_FACTION_MONEY,
			quote.m_iMoneyCost,
			"archived replay",
			quote.m_sQuoteId,
			quote.m_sManifestId);
		HST_ResourceTransactionResult ledgerConflict = ledger.ReserveCost(
			state,
			economy,
			quote.m_sMoneyTransactionId,
			"conflicting_archive_command",
			quote.m_sOperationId,
			quote.m_sActorIdentityId,
			HST_ResourceLedgerService.RESOURCE_FACTION_MONEY,
			quote.m_iMoneyCost,
			"archived conflict",
			quote.m_sQuoteId,
			quote.m_sManifestId);
		HST_ForceConfirmationResult confirmReplay = planning.ConfirmGarrisonQuote(
			state,
			economy,
			new HST_GarrisonService(),
			ledger,
			quote.m_sActorIdentityId,
			quote.m_sQuoteId,
			"archive_garrison_confirm_replay");
		HST_ForceQuoteResult issueReplay = planning.IssueGarrisonQuote(
			state,
			HST_DefaultCatalog.CreateVanillaEveronPreset(),
			quote.m_sActorIdentityId,
			quote.m_sTargetZoneId,
			quote.m_iRequestedMemberCount,
			quote.m_sCommandRequestId,
			false);
		bool exact = archiveResult && archiveResult.m_iArchivedCount == 1;
		exact = exact && state.m_aForceQuotes.Count() == 0 && state.m_aForceManifests.Count() == 0;
		exact = exact && state.m_aResourceTransactions.Count() == 0 && state.m_aForceSettlementTombstones.Count() == 1;
		exact = exact && garrison.m_iInfantryCount == 4 && !garrison.m_aAcceptedManifestIds.Contains(manifest.m_sManifestId);
		exact = exact && ledgerReplay && ledgerReplay.m_bSuccess && ledgerReplay.m_bAlreadyApplied && ledgerReplay.m_Transaction;
		exact = exact && ledgerReplay.m_Transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED;
		exact = exact && ledgerConflict && !ledgerConflict.m_bSuccess && state.m_iFactionMoney == moneyBeforeLedgerReplay;
		exact = exact && confirmReplay && confirmReplay.m_bSuccess && confirmReplay.m_bAlreadyApplied;
		exact = exact && issueReplay && issueReplay.m_bSuccess && state.m_aForceQuotes.Count() == 0;
		report.m_bGarrisonArchiveReplayExact = exact;
		report.m_sGarrisonEvidence = string.Format("archived %1 | full quote/manifest/tx %2/%3/%4 | tombstones %5 | garrison %6 | confirm replay %7 | issue replay %8", ResolveArchivedCount(archiveResult), state.m_aForceQuotes.Count(), state.m_aForceManifests.Count(), state.m_aResourceTransactions.Count(), state.m_aForceSettlementTombstones.Count(), garrison.m_iInfantryCount, confirmReplay && confirmReplay.m_bAlreadyApplied, issueReplay && issueReplay.m_bSuccess);
		report.m_sGarrisonEvidence = report.m_sGarrisonEvidence + string.Format(" | ledger replay %1 | conflict rejected %2 | money unchanged %3", ledgerReplay && ledgerReplay.m_bAlreadyApplied, ledgerConflict && !ledgerConflict.m_bSuccess, state.m_iFactionMoney == moneyBeforeLedgerReplay);
		return state;
	}

	protected void ProveSupportArchiveReplay(HST_ForceSettlementArchiveProofReport report)
	{
		HST_CampaignState state = BuildBaseState();
		HST_ForceManifestState manifest = BuildManifest("archive_support", 4);
		HST_ForceQuoteState quote = BuildAcceptedQuote(manifest, HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_QRF, 4);
		quote.m_eSupportType = HST_ESupportRequestType.HST_SUPPORT_QRF;
		quote.m_sSupportRequestId = "support_" + quote.m_sQuoteId;
		quote.m_sCapabilityId = HST_ForcePlanningService.SUPPORT_QRF_CAPABILITY_ID;
		quote.m_sAssetProfileId = HST_ForcePlanningService.SUPPORT_QRF_ASSET_PROFILE_ID;
		quote.m_iETASeconds = HST_ForcePlanningService.SUPPORT_QRF_ETA_SECONDS;
		quote.m_iCooldownSeconds = HST_ForcePlanningService.SUPPORT_QRF_COOLDOWN_SECONDS;
		state.m_aForceManifests.Insert(manifest);
		state.m_aForceQuotes.Insert(quote);
		AppendTransactions(state, quote, 2);
		HST_SupportRequestState request = new HST_SupportRequestState();
		request.m_sRequestId = quote.m_sSupportRequestId;
		request.m_sOperationId = quote.m_sOperationId;
		request.m_sQuoteId = quote.m_sQuoteId;
		request.m_sManifestId = quote.m_sManifestId;
		request.m_sCommandRequestId = quote.m_sConfirmationRequestId;
		request.m_sMoneyTransactionId = quote.m_sMoneyTransactionId;
		request.m_sHRTransactionId = quote.m_sHRTransactionId;
		request.m_sFactionKey = quote.m_sFactionKey;
		request.m_eType = HST_ESupportRequestType.HST_SUPPORT_QRF;
		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
		request.m_sResolutionKind = "recalled_refund_hr";
		request.m_iMoneyCost = quote.m_iMoneyCost;
		request.m_iHRCost = quote.m_iHRCost;
		request.m_iRefundedHR = 2;
		state.m_aSupportRequests.Insert(request);

		HST_ForceSettlementArchiveService archive = new HST_ForceSettlementArchiveService();
		HST_ForceSettlementArchiveResult archiveResult = archive.ArchiveSettledRecords(state);
		HST_ForcePlanningService planning = new HST_ForcePlanningService();
		HST_ForceConfirmationResult confirmReplay = planning.ConfirmPlayerSupportQuote(
			state,
			null,
			null,
			null,
			null,
			quote.m_sActorIdentityId,
			quote.m_sQuoteId,
			"archive_support_confirm_replay");
		HST_ForceSettlementTombstoneState tombstone = state.FindForceSettlementTombstone(quote.m_sQuoteId);
		HST_ForceSettlementTransactionTombstoneState archivedHR;
		if (tombstone)
			archivedHR = tombstone.FindTransaction(quote.m_sHRTransactionId);
		bool exact = archiveResult && archiveResult.m_iArchivedCount == 1 && tombstone;
		exact = exact && state.m_aForceQuotes.Count() == 0 && state.m_aForceManifests.Count() == 0;
		exact = exact && state.m_aResourceTransactions.Count() == 0;
		exact = exact && request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED && request.m_iRefundedHR == 2;
		exact = exact && archivedHR && archivedHR.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED;
		exact = exact && archivedHR.m_iRefundedAmount == 2;
		exact = exact && confirmReplay && confirmReplay.m_bSuccess && confirmReplay.m_bAlreadyApplied;
		exact = exact && confirmReplay.m_SupportRequest == request;
		report.m_bSupportArchiveReplayExact = exact;
		report.m_sSupportEvidence = string.Format("archived %1 | tombstone %2 | HR refund %3 | terminal request %4 | confirmation replay %5", ResolveArchivedCount(archiveResult), tombstone != null, archivedHR && archivedHR.m_iRefundedAmount == 2, request.m_eStatus, confirmReplay && confirmReplay.m_bAlreadyApplied);
	}

	protected void ProveBacklinkProtection(HST_ForceSettlementArchiveProofReport report)
	{
		HST_CampaignState state = BuildBaseState();
		HST_ForceManifestState manifest = BuildManifest("archive_backlink", 4);
		HST_ForceQuoteState quote = BuildAcceptedQuote(manifest, HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_QRF, 4);
		quote.m_sSupportRequestId = "support_" + quote.m_sQuoteId;
		state.m_aForceManifests.Insert(manifest);
		state.m_aForceQuotes.Insert(quote);
		AppendTransactions(state, quote, 0);
		HST_SupportRequestState request = BuildTerminalSupportRequest(quote);
		state.m_aSupportRequests.Insert(request);
		HST_ForceSpawnResultState batch = new HST_ForceSpawnResultState();
		batch.m_sResultId = "archive_backlink_result";
		batch.m_sManifestId = manifest.m_sManifestId;
		batch.m_sManifestHash = manifest.m_sManifestHash;
		batch.m_sOperationId = manifest.m_sOperationId;
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
		state.m_aForceSpawnResults.Insert(batch);
		HST_ForceSettlementArchiveService archive = new HST_ForceSettlementArchiveService();
		HST_ForceSettlementArchiveResult archiveResult = archive.ArchiveSettledRecords(state);
		report.m_bBacklinkProtectionExact = archiveResult && archiveResult.m_iArchivedCount == 0
			&& state.m_aForceQuotes.Count() == 1 && state.m_aForceManifests.Count() == 1
			&& state.m_aResourceTransactions.Count() == 2 && state.m_aForceSettlementTombstones.Count() == 0;
		report.m_sBacklinkEvidence = string.Format("archived %1 | deferred %2 | retained quote/manifest/tx %3/%4/%5", ResolveArchivedCount(archiveResult), ResolveDeferredCount(archiveResult), state.m_aForceQuotes.Count(), state.m_aForceManifests.Count(), state.m_aResourceTransactions.Count());
	}

	protected void ProveRetentionCapacity(HST_ForceSettlementArchiveProofReport report)
	{
		HST_CampaignState state = BuildBaseState();
		state.m_iElapsedSeconds = 100000;
		for (int index = 0; index < HST_ForceSettlementArchiveService.MAX_TOMBSTONE_ROWS; index++)
		{
			HST_ForceSettlementTombstoneState tombstone = new HST_ForceSettlementTombstoneState();
			tombstone.m_sQuoteId = "capacity_quote_" + index;
			tombstone.m_sManifestId = "capacity_manifest_" + index;
			tombstone.m_iArchivedAtSecond = 1;
			state.m_aForceSettlementTombstones.Insert(tombstone);
		}
		for (int quoteIndex = 0; quoteIndex < HST_ForceSettlementArchiveService.MAX_TOTAL_PLANNING_AUTHORITY_ROWS - HST_ForceSettlementArchiveService.MAX_TOMBSTONE_ROWS; quoteIndex++)
			state.m_aForceQuotes.Insert(new HST_ForceQuoteState());
		HST_ForceSettlementArchiveService archive = new HST_ForceSettlementArchiveService();
		string beforeFailure;
		bool beforeCapacity = archive.CanAdmitPlanningRecord(state, beforeFailure);
		HST_ForceSettlementArchiveResult maintenance = archive.ArchiveSettledRecords(state);
		string afterFailure;
		bool afterCapacity = archive.CanAdmitPlanningRecord(state, afterFailure);
		report.m_bRetentionCapacityExact = !beforeCapacity && maintenance && maintenance.m_iPrunedTombstoneCount == 1
			&& state.m_aForceSettlementTombstones.Count() == HST_ForceSettlementArchiveService.MAX_TOMBSTONE_ROWS - 1 && afterCapacity;
		report.m_sCapacityEvidence = string.Format("before admitted %1 | pruned %2 | tombstones %3 | after admitted %4", beforeCapacity, maintenance && maintenance.m_iPrunedTombstoneCount, state.m_aForceSettlementTombstones.Count(), afterCapacity);
	}

	protected void ProvePersistence(HST_ForceSettlementArchiveProofReport report, HST_CampaignState archivedState)
	{
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(archivedState);
		HST_CampaignState restored = saveData.Restore();
		HST_ForceSettlementTombstoneState source;
		HST_ForceSettlementTombstoneState restoredTombstone;
		if (archivedState && archivedState.m_aForceSettlementTombstones.Count() == 1)
		{
			source = archivedState.m_aForceSettlementTombstones[0];
			if (restored)
				restoredTombstone = restored.FindForceSettlementTombstone(source.m_sQuoteId);
		}
		HST_ForceConfirmationResult replay;
		if (restored && restoredTombstone)
		{
			HST_ForcePlanningService planning = new HST_ForcePlanningService();
			replay = planning.ConfirmGarrisonQuote(
				restored,
				new HST_EconomyService(),
				new HST_GarrisonService(),
				new HST_ResourceLedgerService(),
				restoredTombstone.m_sActorIdentityId,
				restoredTombstone.m_sQuoteId,
				"archive_roundtrip_confirm");
		}
		report.m_bPersistenceExact = restoredTombstone && source && restoredTombstone != source
			&& restoredTombstone.m_sManifestHash == source.m_sManifestHash
			&& restoredTombstone.m_aTransactions.Count() == source.m_aTransactions.Count()
			&& replay && replay.m_bAlreadyApplied;
		report.m_sPersistenceEvidence = string.Format("restored tombstone %1 | deep copy %2 | tx %3 | replay %4", restoredTombstone != null, restoredTombstone && source && restoredTombstone != source, restoredTombstone && restoredTombstone.m_aTransactions.Count(), replay && replay.m_bAlreadyApplied);
	}

	protected void ProveSchema47Migration(HST_ForceSettlementArchiveProofReport report)
	{
		HST_CampaignState legacy = BuildBaseState();
		legacy.m_iSchemaVersion = 47;
		legacy.m_iLastLoadedSchemaVersion = 47;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(legacy);
		HST_CampaignState restored = saveData.Restore();
		report.m_bSchema47MigrationExact = restored && restored.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION
			&& restored.m_aForceSettlementTombstones.Count() == 0 && HasEvent(restored, "migration_schema48_force_settlement_archive");
		report.m_sMigrationEvidence = string.Format("schema %1 | tombstones %2 | event %3", restored && restored.m_iSchemaVersion, restored && restored.m_aForceSettlementTombstones.Count(), restored && HasEvent(restored, "migration_schema48_force_settlement_archive"));
	}

	protected HST_CampaignState BuildBaseState()
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iElapsedSeconds = 2000;
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		state.m_bHQDeployed = true;
		state.m_iFactionMoney = 1000;
		state.m_iHR = 100;
		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = "archive_zone";
		zone.m_sOwnerFactionKey = "FIA";
		zone.m_eType = HST_EZoneType.HST_ZONE_OUTPOST;
		zone.m_iGarrisonSlots = 32;
		zone.m_vPosition = "1000 0 1000";
		state.m_aZones.Insert(zone);
		return state;
	}

	protected HST_ForceManifestState BuildManifest(string token, int members)
	{
		HST_ForceManifestState manifest = new HST_ForceManifestState();
		manifest.m_sManifestId = token + "_manifest";
		manifest.m_sOperationId = token + "_operation";
		manifest.m_sQuoteId = token + "_quote";
		manifest.m_sCommandRequestId = token + "_issue";
		manifest.m_sForceKind = token;
		manifest.m_sFactionRole = "resistance";
		manifest.m_sFactionKey = "FIA";
		manifest.m_sTargetZoneId = "archive_zone";
		manifest.m_sCatalogVersion = "archive_catalog";
		manifest.m_sPolicyId = "archive_policy";
		manifest.m_iRequestedMemberCount = members;
		manifest.m_iAcceptedMemberCount = members;
		manifest.m_iMoneyCost = 250;
		manifest.m_iHRCost = members;
		manifest.m_iCreatedAtSecond = 100;
		manifest.m_bFrozen = true;
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		manifest.m_sManifestHash = integrity.BuildManifestHash(manifest);
		return manifest;
	}

	protected HST_ForceQuoteState BuildAcceptedQuote(HST_ForceManifestState manifest, string quoteKind, int members)
	{
		HST_ForceQuoteState quote = new HST_ForceQuoteState();
		quote.m_sQuoteId = manifest.m_sQuoteId;
		quote.m_sManifestId = manifest.m_sManifestId;
		quote.m_sManifestHash = manifest.m_sManifestHash;
		quote.m_sOperationId = manifest.m_sOperationId;
		quote.m_sCommandRequestId = manifest.m_sCommandRequestId;
		quote.m_sConfirmationRequestId = manifest.m_sManifestId + "_confirm";
		quote.m_sActorIdentityId = "archive_actor";
		quote.m_sQuoteKind = quoteKind;
		quote.m_sFactionKey = manifest.m_sFactionKey;
		quote.m_sTargetZoneId = manifest.m_sTargetZoneId;
		quote.m_sCatalogVersion = manifest.m_sCatalogVersion;
		quote.m_sPolicyId = manifest.m_sPolicyId;
		quote.m_sMoneyTransactionId = manifest.m_sOperationId + "_money";
		quote.m_sHRTransactionId = manifest.m_sOperationId + "_hr";
		quote.m_vTargetPosition = "1000 0 1000";
		quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED;
		quote.m_iRequestedMemberCount = members;
		quote.m_iAcceptedMemberCount = members;
		quote.m_iMoneyCost = manifest.m_iMoneyCost;
		quote.m_iHRCost = manifest.m_iHRCost;
		quote.m_iCreatedAtSecond = 100;
		quote.m_iAcceptedAtSecond = 200;
		quote.m_bAllOrNothing = true;
		return quote;
	}

	protected void AppendTransactions(HST_CampaignState state, HST_ForceQuoteState quote, int hrRefund)
	{
		state.m_aResourceTransactions.Insert(BuildTransaction(quote, quote.m_sMoneyTransactionId, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quote.m_iMoneyCost, 0));
		state.m_aResourceTransactions.Insert(BuildTransaction(quote, quote.m_sHRTransactionId, HST_ResourceLedgerService.RESOURCE_HR, quote.m_iHRCost, hrRefund));
	}

	protected HST_ResourceTransactionState BuildTransaction(HST_ForceQuoteState quote, string transactionId, string resourceType, int amount, int refunded)
	{
		HST_ResourceTransactionState transaction = new HST_ResourceTransactionState();
		transaction.m_sTransactionId = transactionId;
		transaction.m_sCommandRequestId = quote.m_sConfirmationRequestId;
		transaction.m_sOperationId = quote.m_sOperationId;
		transaction.m_sQuoteId = quote.m_sQuoteId;
		transaction.m_sManifestId = quote.m_sManifestId;
		transaction.m_sActorIdentityId = quote.m_sActorIdentityId;
		transaction.m_sResourceType = resourceType;
		transaction.m_sLastSettlementId = "archive_settlement_" + transactionId;
		transaction.m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED;
		if (refunded > 0 && refunded < amount)
			transaction.m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED;
		else if (refunded >= amount && amount > 0)
			transaction.m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED;
		transaction.m_iAmount = amount;
		transaction.m_iRefundedAmount = refunded;
		transaction.m_iCreatedAtSecond = 200;
		transaction.m_iSettledAtSecond = 300;
		return transaction;
	}

	protected HST_SupportRequestState BuildTerminalSupportRequest(HST_ForceQuoteState quote)
	{
		HST_SupportRequestState request = new HST_SupportRequestState();
		request.m_sRequestId = quote.m_sSupportRequestId;
		request.m_sOperationId = quote.m_sOperationId;
		request.m_sQuoteId = quote.m_sQuoteId;
		request.m_sManifestId = quote.m_sManifestId;
		request.m_sMoneyTransactionId = quote.m_sMoneyTransactionId;
		request.m_sHRTransactionId = quote.m_sHRTransactionId;
		request.m_eType = HST_ESupportRequestType.HST_SUPPORT_QRF;
		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
		request.m_sResolutionKind = "force_eliminated";
		return request;
	}

	protected bool HasEvent(HST_CampaignState state, string eventId)
	{
		foreach (HST_CampaignEventState eventState : state.m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				return true;
		}
		return false;
	}

	protected int ResolveArchivedCount(HST_ForceSettlementArchiveResult result)
	{
		if (!result)
			return -1;
		return result.m_iArchivedCount;
	}

	protected int ResolveDeferredCount(HST_ForceSettlementArchiveResult result)
	{
		if (!result)
			return -1;
		return result.m_iDeferredCount;
	}
}
#endif
