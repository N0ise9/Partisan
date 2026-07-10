class HST_PaidSupportAuthorityProofReport
{
	string m_sIssueConfirmEvidence;
	string m_sQueueReplayEvidence;
	string m_sRoundtripEvidence;
	string m_sFailureRefundEvidence;
	string m_sCancelRefundEvidence;
	string m_sRecallRefundEvidence;
	string m_sRecallTypedTextEvidence;
	string m_sRecallSettlementFailureEvidence;
	string m_sRecallLostGroupEvidence;
	string m_sTerminalRefundEvidence;
	string m_sMigrationEvidence;
	bool m_bIssueConfirmExact;
	bool m_bQueueReplayExact;
	bool m_bRoundtripExact;
	bool m_bFailureRefundExact;
	bool m_bCancelRefundExact;
	bool m_bRecallRefundExact;
	bool m_bRecallTypedTextExact;
	bool m_bRecallSettlementFailureExact;
	bool m_bRecallLostGroupExact;
	bool m_bTerminalRefundExact;
	bool m_bMigrationExact;
}

class HST_PaidSupportAuthorityProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_EconomyService m_Economy;
	ref HST_ResourceLedgerService m_Ledger;
	ref HST_ForcePlanningService m_Planning;
	ref HST_ForceSpawnQueueService m_Queue;
	ref HST_SupportRequestService m_Support;
	ref HST_ForceQuoteResult m_Issue;
	ref HST_ForceConfirmationResult m_Confirmation;
	ref HST_SupportRequestState m_Request;
	ref HST_ForceSpawnQueueEnqueueResult m_Enqueue;
	int m_iInitialMoney;
	int m_iInitialHR;
}

class HST_PaidSupportAuthorityProofService
{
	HST_PaidSupportAuthorityProofReport BuildReport()
	{
		HST_PaidSupportAuthorityProofReport report = new HST_PaidSupportAuthorityProofReport();
		HST_PaidSupportAuthorityProofFixture fixture = BuildAcceptedFixture(report);
		if (fixture)
		{
			ProveQueueReplay(report, fixture);
			ProveRoundtrip(report, fixture);
			ProveFailureRefund(report, fixture);
		}
		else
		{
			report.m_sQueueReplayEvidence = "accepted fixture missing";
			report.m_sRoundtripEvidence = "accepted fixture missing";
			report.m_sFailureRefundEvidence = "accepted fixture missing";
		}
		ProveCancelRefund(report);
		ProveRecallRefund(report);
		ProveRecallTypedText(report);
		ProveRecallSettlementFailure(report);
		ProveRecallLostGroupSettlement(report);
		ProveTerminalRefund(report);
		ProveLegacyMigration(report);
		return report;
	}

	protected HST_PaidSupportAuthorityProofFixture BuildAcceptedFixture(HST_PaidSupportAuthorityProofReport report)
	{
		HST_PaidSupportAuthorityProofFixture fixture = new HST_PaidSupportAuthorityProofFixture();
		fixture.m_State = CreateProofState();
		fixture.m_Preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		fixture.m_Economy = new HST_EconomyService();
		fixture.m_Ledger = new HST_ResourceLedgerService();
		fixture.m_Planning = new HST_ForcePlanningService();
		fixture.m_Queue = new HST_ForceSpawnQueueService();
		fixture.m_Support = new HST_SupportRequestService();
		fixture.m_Support.SetExactForceAuthorityServices(fixture.m_Queue, fixture.m_Ledger, fixture.m_Economy);
		fixture.m_iInitialMoney = fixture.m_State.m_iFactionMoney;
		fixture.m_iInitialHR = fixture.m_State.m_iHR;
		fixture.m_Issue = fixture.m_Planning.IssuePlayerSupportQuote(
			fixture.m_State,
			fixture.m_Preset,
			"paid_support_proof_actor",
			HST_ESupportRequestType.HST_SUPPORT_QRF,
			"paid_support_target",
			"1600 20 1600",
			"paid_support_proof_issue",
			true);

		bool issueExact = ValidateIssue(fixture);
		fixture.m_Confirmation = null;
		if (issueExact)
		{
			fixture.m_Confirmation = fixture.m_Planning.ConfirmPlayerSupportQuote(
				fixture.m_State,
				fixture.m_Preset,
				fixture.m_Economy,
				fixture.m_Support,
				fixture.m_Ledger,
				"paid_support_proof_actor",
				fixture.m_Issue.m_Quote.m_sQuoteId,
				"paid_support_proof_confirm");
		}
		fixture.m_Request = ResolveOnlySupportRequest(fixture.m_State);
		bool confirmationExact = ValidateConfirmation(fixture);
		report.m_bIssueConfirmExact = issueExact && confirmationExact;
		report.m_sIssueConfirmEvidence = BuildIssueConfirmEvidence(fixture, issueExact, confirmationExact);
		if (!report.m_bIssueConfirmExact)
			return null;
		return fixture;
	}

	protected bool ValidateIssue(HST_PaidSupportAuthorityProofFixture fixture)
	{
		if (!fixture || !fixture.m_Issue || !fixture.m_Issue.m_bSuccess || !fixture.m_Issue.m_Quote || !fixture.m_Issue.m_Manifest)
			return false;
		HST_ForceQuoteState quote = fixture.m_Issue.m_Quote;
		HST_ForceManifestState manifest = fixture.m_Issue.m_Manifest;
		if (fixture.m_State.m_iFactionMoney != fixture.m_iInitialMoney || fixture.m_State.m_iHR != fixture.m_iInitialHR)
			return false;
		if (quote.m_sQuoteKind != HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_QRF || quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
			return false;
		if (manifest.m_iMoneyCost != HST_ForcePlanningService.SUPPORT_QRF_MONEY_COST || manifest.m_iHRCost != manifest.m_aMembers.Count())
			return false;
		return manifest.m_aGroups.Count() == 1 && manifest.m_iAcceptedMemberCount == manifest.m_aMembers.Count() && manifest.m_aMembers.Count() > 0;
	}

	protected bool ValidateConfirmation(HST_PaidSupportAuthorityProofFixture fixture)
	{
		if (!fixture || !fixture.m_Confirmation || !fixture.m_Confirmation.m_bSuccess || fixture.m_Confirmation.m_bAlreadyApplied || !fixture.m_Request)
			return false;
		HST_ForceQuoteState quote = fixture.m_Issue.m_Quote;
		HST_ForceManifestState manifest = fixture.m_Issue.m_Manifest;
		HST_ResourceTransactionState money = fixture.m_State.FindResourceTransaction(quote.m_sMoneyTransactionId);
		HST_ResourceTransactionState hr = fixture.m_State.FindResourceTransaction(quote.m_sHRTransactionId);
		if (quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED || fixture.m_State.m_aSupportRequests.Count() != 1)
			return false;
		if (!TransactionCommittedExact(money, quote, manifest, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, manifest.m_iMoneyCost))
			return false;
		if (!TransactionCommittedExact(hr, quote, manifest, HST_ResourceLedgerService.RESOURCE_HR, manifest.m_iHRCost))
			return false;
		if (fixture.m_State.m_iFactionMoney != fixture.m_iInitialMoney - manifest.m_iMoneyCost || fixture.m_State.m_iHR != fixture.m_iInitialHR - manifest.m_iHRCost)
			return false;
		return fixture.m_Request.m_sQuoteId == quote.m_sQuoteId && fixture.m_Request.m_sManifestId == manifest.m_sManifestId
			&& fixture.m_Request.m_sSpawnResultId == "spawn_" + fixture.m_Request.m_sRequestId;
	}

	protected void ProveQueueReplay(HST_PaidSupportAuthorityProofReport report, HST_PaidSupportAuthorityProofFixture fixture)
	{
		HST_ForceQuoteState quote = fixture.m_Issue.m_Quote;
		HST_ForceManifestState manifest = fixture.m_Issue.m_Manifest;
		vector canonicalSource = fixture.m_Request.m_vSourcePosition;
		vector canonicalTarget = fixture.m_Request.m_vTargetPosition;
		fixture.m_Enqueue = fixture.m_Support.EnqueueAcceptedExactPlayerSupportProjection(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Request,
			"1200 20 1200",
			"1600 20 1600");
		int moneyBeforeReplay = fixture.m_State.m_iFactionMoney;
		int hrBeforeReplay = fixture.m_State.m_iHR;
		int transactionsBeforeReplay = fixture.m_State.m_aResourceTransactions.Count();
		HST_ForceConfirmationResult replay = fixture.m_Planning.ConfirmPlayerSupportQuote(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Economy,
			fixture.m_Support,
			fixture.m_Ledger,
			"paid_support_proof_actor",
			quote.m_sQuoteId,
			"paid_support_proof_confirm_replay");
		HST_ActiveGroupState group;
		HST_ForceSpawnResultState batch;
		if (fixture.m_Enqueue)
			batch = fixture.m_Enqueue.m_Batch;
		if (batch)
			group = fixture.m_State.FindActiveGroup(batch.m_sProjectionId);
		bool exact = fixture.m_Enqueue && fixture.m_Enqueue.m_bSuccess && batch && group;
		if (exact)
			exact = batch.m_iExpectedSlotCount == manifest.m_aMembers.Count() + 1 && group.m_sManifestId == manifest.m_sManifestId && group.m_sSupportRequestId == fixture.m_Request.m_sRequestId;
		if (exact)
			exact = PositionsMatch(fixture.m_Request.m_vSourcePosition, canonicalSource) && PositionsMatch(fixture.m_Request.m_vTargetPosition, canonicalTarget);
		if (exact)
			exact = replay && replay.m_bSuccess && replay.m_bAlreadyApplied && fixture.m_State.m_iFactionMoney == moneyBeforeReplay && fixture.m_State.m_iHR == hrBeforeReplay && fixture.m_State.m_aResourceTransactions.Count() == transactionsBeforeReplay;
		int batchSlotCount = -1;
		if (batch)
			batchSlotCount = batch.m_iExpectedSlotCount;
		report.m_bQueueReplayExact = exact;
		report.m_sQueueReplayEvidence = string.Format("enqueue %1 | slots %2/%3 | group %4 | canonical positions %5 | replay %6 | money %7 HR %8 tx %9", fixture.m_Enqueue && fixture.m_Enqueue.m_bSuccess, batchSlotCount, manifest.m_aMembers.Count() + 1, group != null, PositionsMatch(fixture.m_Request.m_vSourcePosition, canonicalSource) && PositionsMatch(fixture.m_Request.m_vTargetPosition, canonicalTarget), replay && replay.m_bAlreadyApplied, fixture.m_State.m_iFactionMoney, fixture.m_State.m_iHR, fixture.m_State.m_aResourceTransactions.Count());
	}

	protected void ProveRoundtrip(HST_PaidSupportAuthorityProofReport report, HST_PaidSupportAuthorityProofFixture fixture)
	{
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_ForceQuoteState restoredQuote;
		HST_ForceManifestState restoredManifest;
		HST_SupportRequestState restoredRequest;
		HST_ForceSpawnResultState restoredBatch;
		if (restored)
		{
			restoredQuote = restored.FindForceQuote(fixture.m_Issue.m_Quote.m_sQuoteId);
			restoredManifest = restored.FindForceManifest(fixture.m_Issue.m_Manifest.m_sManifestId);
			restoredRequest = restored.FindSupportRequest(fixture.m_Request.m_sRequestId);
			restoredBatch = restored.FindForceSpawnResult(fixture.m_Request.m_sSpawnResultId);
		}
		bool exact = restored && restored.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION && restoredQuote && restoredManifest && restoredRequest && restoredBatch;
		if (exact)
			exact = restoredQuote.m_sSupportRequestId == restoredRequest.m_sRequestId && restoredQuote.m_iETASeconds == HST_ForcePlanningService.SUPPORT_QRF_ETA_SECONDS && restoredQuote.m_iCooldownSeconds == HST_ForcePlanningService.SUPPORT_QRF_COOLDOWN_SECONDS;
		if (exact)
			exact = restoredQuote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED && restoredManifest.m_aGroups.Count() == 1 && restoredManifest.m_aMembers.Count() == fixture.m_Issue.m_Manifest.m_aMembers.Count();
		if (exact)
			exact = restoredRequest.m_sMoneyTransactionId == restoredQuote.m_sMoneyTransactionId && restoredRequest.m_sHRTransactionId == restoredQuote.m_sHRTransactionId && !restoredRequest.m_bPhysicalized;
		int restoredSchema = -1;
		if (restored)
			restoredSchema = restored.m_iSchemaVersion;
		report.m_bRoundtripExact = exact;
		report.m_sRoundtripEvidence = string.Format("schema %1 | quote %2 | manifest %3 | support %4 | batch %5 | exact %6", restoredSchema, restoredQuote != null, restoredManifest != null, restoredRequest != null, restoredBatch != null, exact);
	}

	protected void ProveFailureRefund(HST_PaidSupportAuthorityProofReport report, HST_PaidSupportAuthorityProofFixture fixture)
	{
		HST_ForceSpawnResultState batch;
		if (fixture.m_Enqueue)
			batch = fixture.m_Enqueue.m_Batch;
		if (batch)
		{
			batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
			batch.m_sTerminalReason = "paid support proof terminal failure";
		}
		bool settled = fixture.m_Support.TickExactPlayerSupportSettlements(fixture.m_State);
		HST_ResourceTransactionState money = fixture.m_State.FindResourceTransaction(fixture.m_Issue.m_Quote.m_sMoneyTransactionId);
		HST_ResourceTransactionState hr = fixture.m_State.FindResourceTransaction(fixture.m_Issue.m_Quote.m_sHRTransactionId);
		int moneyAfterSettlement = fixture.m_State.m_iFactionMoney;
		int hrAfterSettlement = fixture.m_State.m_iHR;
		HST_ForceConfirmationResult refundedReplay = fixture.m_Planning.ConfirmPlayerSupportQuote(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Economy,
			fixture.m_Support,
			fixture.m_Ledger,
			"paid_support_proof_actor",
			fixture.m_Issue.m_Quote.m_sQuoteId,
			"paid_support_proof_refunded_replay");
		HST_ForceQuoteResult replacementQuote = fixture.m_Planning.IssuePlayerSupportQuote(
			fixture.m_State,
			fixture.m_Preset,
			"paid_support_proof_actor",
			HST_ESupportRequestType.HST_SUPPORT_QRF,
			"paid_support_target",
			"1600 20 1600",
			"paid_support_proof_replacement_issue",
			true);
		bool exact = settled && fixture.m_Request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
		if (exact)
			exact = money && money.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED && money.m_iRefundedAmount == money.m_iAmount;
		if (exact)
			exact = hr && hr.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED && hr.m_iRefundedAmount == hr.m_iAmount;
		if (exact)
			exact = moneyAfterSettlement == fixture.m_iInitialMoney && hrAfterSettlement == fixture.m_iInitialHR;
		if (exact)
			exact = fixture.m_Request.m_sGroupId.IsEmpty() && refundedReplay && refundedReplay.m_bSuccess && refundedReplay.m_bAlreadyApplied && replacementQuote && replacementQuote.m_bSuccess && fixture.m_State.m_iFactionMoney == moneyAfterSettlement && fixture.m_State.m_iHR == hrAfterSettlement;
		int moneyRefund = -1;
		int hrRefund = -1;
		if (money)
			moneyRefund = money.m_iRefundedAmount;
		if (hr)
			hrRefund = hr.m_iRefundedAmount;
		report.m_bFailureRefundExact = exact;
		report.m_sFailureRefundEvidence = string.Format("settled %1 | status %2 | money %3/%4 | HR %5/%6 | group removed %7 | replacement quote %8 | replay %9", settled, fixture.m_Request.m_eStatus, moneyRefund, fixture.m_Request.m_iMoneyCost, hrRefund, fixture.m_Request.m_iHRCost, fixture.m_Request.m_sGroupId.IsEmpty(), replacementQuote && replacementQuote.m_bSuccess, refundedReplay && refundedReplay.m_bAlreadyApplied);
	}

	protected void ProveCancelRefund(HST_PaidSupportAuthorityProofReport report)
	{
		HST_PaidSupportAuthorityProofReport fixtureReport = new HST_PaidSupportAuthorityProofReport();
		HST_PaidSupportAuthorityProofFixture fixture = BuildAcceptedFixture(fixtureReport);
		if (!fixture)
		{
			report.m_sCancelRefundEvidence = "accepted cancellation fixture missing: " + fixtureReport.m_sIssueConfirmEvidence;
			return;
		}
		bool cancelled = fixture.m_Support.CancelSupportRequest(fixture.m_State, fixture.m_Request.m_sRequestId, true);
		HST_ResourceTransactionState money = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sMoneyTransactionId);
		HST_ResourceTransactionState hr = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sHRTransactionId);
		bool exact = cancelled && fixture.m_Request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED;
		if (exact)
			exact = fixture.m_State.m_iFactionMoney == fixture.m_iInitialMoney && fixture.m_State.m_iHR == fixture.m_iInitialHR;
		if (exact)
			exact = money && hr && money.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED && hr.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED;
		HST_ForceQuoteResult replacementQuote = fixture.m_Planning.IssuePlayerSupportQuote(
			fixture.m_State,
			fixture.m_Preset,
			"paid_support_proof_actor",
			HST_ESupportRequestType.HST_SUPPORT_QRF,
			"paid_support_target",
			"1600 20 1600",
			"paid_support_proof_cancel_replacement_issue",
			true);
		if (exact)
			exact = replacementQuote && replacementQuote.m_bSuccess;
		report.m_bCancelRefundExact = exact;
		report.m_sCancelRefundEvidence = string.Format("cancelled %1 | status %2 | money %3 | HR %4 | money tx %5 | HR tx %6 | replacement quote %7", cancelled, fixture.m_Request.m_eStatus, fixture.m_State.m_iFactionMoney, fixture.m_State.m_iHR, money && money.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED, hr && hr.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED, replacementQuote && replacementQuote.m_bSuccess);
	}

	protected void ProveRecallRefund(HST_PaidSupportAuthorityProofReport report)
	{
		HST_PaidSupportAuthorityProofReport fixtureReport = new HST_PaidSupportAuthorityProofReport();
		HST_PaidSupportAuthorityProofFixture fixture = BuildAcceptedFixture(fixtureReport);
		if (!fixture)
		{
			report.m_sRecallRefundEvidence = "accepted recall fixture missing: " + fixtureReport.m_sIssueConfirmEvidence;
			return;
		}
		fixture.m_Enqueue = fixture.m_Support.EnqueueAcceptedExactPlayerSupportProjection(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Request,
			"1200 20 1200",
			"1600 20 1600");
		HST_SupportRecallResult recallResult = fixture.m_Support.RecallSupportRequestDetailed(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Economy,
			null,
			fixture.m_Request.m_sRequestId,
			true);
		fixture.m_Support.TickExactPlayerSupportSettlements(fixture.m_State);
		HST_ResourceTransactionState money = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sMoneyTransactionId);
		HST_ResourceTransactionState hr = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sHRTransactionId);
		bool exact = fixture.m_Enqueue && fixture.m_Enqueue.m_bSuccess && recallResult && recallResult.m_bAccepted && recallResult.m_bStateChanged;
		if (exact)
			exact = recallResult.m_sDisposition == "predeployment_cleanup_pending" && recallResult.m_sOperationId == fixture.m_Request.m_sOperationId;
		if (exact)
			exact = fixture.m_Request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED && fixture.m_Request.m_sResolutionKind == "recalled_before_deploy";
		if (exact)
			exact = fixture.m_State.m_iFactionMoney == fixture.m_iInitialMoney - fixture.m_Request.m_iMoneyCost && fixture.m_State.m_iHR == fixture.m_iInitialHR;
		if (exact)
			exact = money && money.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED && money.m_iRefundedAmount == 0;
		if (exact)
			exact = hr && hr.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED && hr.m_iRefundedAmount == hr.m_iAmount && fixture.m_Request.m_sGroupId.IsEmpty();
		report.m_bRecallRefundExact = exact;
		report.m_sRecallRefundEvidence = string.Format("enqueue %1 | recall accepted %2 changed %3 disposition %4 | status %5 | money retained %6 | HR restored %7 | money tx committed %8 | HR tx refunded %9", fixture.m_Enqueue && fixture.m_Enqueue.m_bSuccess, recallResult && recallResult.m_bAccepted, recallResult && recallResult.m_bStateChanged, recallResult && recallResult.m_sDisposition, fixture.m_Request.m_eStatus, fixture.m_State.m_iFactionMoney, fixture.m_State.m_iHR, money && money.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED, hr && hr.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED);
		report.m_sRecallRefundEvidence = report.m_sRecallRefundEvidence + string.Format(" | group removed %1", fixture.m_Request.m_sGroupId.IsEmpty());
	}

	protected void ProveRecallTypedText(HST_PaidSupportAuthorityProofReport report)
	{
		HST_PaidSupportAuthorityProofReport fixtureReport = new HST_PaidSupportAuthorityProofReport();
		HST_PaidSupportAuthorityProofFixture fixture = BuildAcceptedFixture(fixtureReport);
		if (!fixture)
		{
			report.m_sRecallTypedTextEvidence = "accepted typed-text fixture missing: " + fixtureReport.m_sIssueConfirmEvidence;
			return;
		}
		fixture.m_Enqueue = fixture.m_Support.EnqueueAcceptedExactPlayerSupportProjection(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Request,
			"1200 20 1200",
			"1600 20 1600");
		HST_ForceSpawnResultState batch;
		if (fixture.m_Enqueue)
			batch = fixture.m_Enqueue.m_Batch;
		if (batch)
		{
			batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
			batch.m_sTerminalReason = "typed recall proof failed before deployment";
		}
		HST_SupportRecallResult recallResult = fixture.m_Support.RecallSupportRequestDetailed(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Economy,
			null,
			fixture.m_Request.m_sRequestId,
			true);
		HST_ResourceTransactionState money = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sMoneyTransactionId);
		HST_ResourceTransactionState hr = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sHRTransactionId);
		HST_CampaignEventLogService commandEvents = new HST_CampaignEventLogService();
		HST_CampaignCommandService commands = new HST_CampaignCommandService();
		commands.SetEventLogService(commandEvents);
		HST_CampaignCommandEnvelope envelope = commands.BuildEnvelope(fixture.m_State, "paid_support_proof_typed_recall_command", "paid_support_proof_actor", "forces", "support_recall", fixture.m_Request.m_sRequestId);
		HST_CampaignCommandResult commandBegin = commands.Begin(fixture.m_State, envelope);
		HST_CampaignCommandResult commandComplete;
		if (recallResult)
			commandComplete = commands.CompleteExplicit(fixture.m_State, envelope, recallResult.ResolveCommandStatus(), recallResult.BuildSummary(), recallResult.m_sOperationId);
		int moneyRefundBeforeReplay;
		int hrRefundBeforeReplay;
		if (money)
			moneyRefundBeforeReplay = money.m_iRefundedAmount;
		if (hr)
			hrRefundBeforeReplay = hr.m_iRefundedAmount;
		HST_CampaignCommandResult commandReplay = commands.Begin(fixture.m_State, envelope);
		HST_CommandReceiptState recallReceipt = fixture.m_State.FindCommandReceipt(envelope.m_sRequestId);
		bool exact = fixture.m_Enqueue && fixture.m_Enqueue.m_bSuccess && batch && recallResult;
		if (exact)
			exact = recallResult.m_bAccepted && recallResult.m_bStateChanged && recallResult.m_bTerminal && recallResult.m_sDisplayMessage.Contains("failed");
		if (exact)
			exact = recallResult.m_sFailureReason.IsEmpty() && recallResult.m_sDisposition == "deployment_failed_settled" && recallResult.m_sOperationId == fixture.m_Request.m_sOperationId && recallResult.m_Request == fixture.m_Request;
		if (exact)
			exact = fixture.m_Request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED && fixture.m_Request.m_sResolutionKind == "exact_deployment_failed_refunded";
		if (exact)
			exact = fixture.m_State.m_iFactionMoney == fixture.m_iInitialMoney && fixture.m_State.m_iHR == fixture.m_iInitialHR;
		if (exact)
			exact = money && hr && money.m_iRefundedAmount == money.m_iAmount && hr.m_iRefundedAmount == hr.m_iAmount && fixture.m_Request.m_sGroupId.IsEmpty();
		if (exact)
			exact = commandBegin && commandBegin.m_bShouldExecute && commandComplete && commandComplete.m_eStatus == HST_ECampaignCommandStatus.HST_COMMAND_APPLIED && recallReceipt && recallReceipt.m_eStatus == HST_ECampaignCommandStatus.HST_COMMAND_APPLIED && recallReceipt.m_sAggregateId == recallResult.m_sOperationId;
		if (exact)
			exact = commandReplay && !commandReplay.m_bShouldExecute && commandReplay.m_eStatus == HST_ECampaignCommandStatus.HST_COMMAND_ALREADY_APPLIED && money.m_iRefundedAmount == moneyRefundBeforeReplay && hr.m_iRefundedAmount == hrRefundBeforeReplay;
		report.m_bRecallTypedTextExact = exact;
		report.m_sRecallTypedTextEvidence = string.Format("accepted %1 | changed %2 | terminal %3 | disposition %4 | display contains failed %5 | status %6 | money refund %7/%8 | HR refund %9", recallResult && recallResult.m_bAccepted, recallResult && recallResult.m_bStateChanged, recallResult && recallResult.m_bTerminal, recallResult && recallResult.m_sDisposition, recallResult && recallResult.m_sDisplayMessage.Contains("failed"), fixture.m_Request.m_eStatus, money && money.m_iRefundedAmount, fixture.m_Request.m_iMoneyCost, hr && hr.m_iRefundedAmount);
		report.m_sRecallTypedTextEvidence = report.m_sRecallTypedTextEvidence + string.Format("/%1", fixture.m_Request.m_iHRCost);
		report.m_sRecallTypedTextEvidence = report.m_sRecallTypedTextEvidence + string.Format(" | receipt %1 aggregate %2 | replay %3", recallReceipt && recallReceipt.m_eStatus, recallReceipt && recallReceipt.m_sAggregateId, commandReplay && commandReplay.m_eStatus);
	}

	protected void ProveRecallSettlementFailure(HST_PaidSupportAuthorityProofReport report)
	{
		HST_PaidSupportAuthorityProofReport fixtureReport = new HST_PaidSupportAuthorityProofReport();
		HST_PaidSupportAuthorityProofFixture fixture = BuildAcceptedFixture(fixtureReport);
		if (!fixture)
		{
			report.m_sRecallSettlementFailureEvidence = "accepted settlement-failure fixture missing: " + fixtureReport.m_sIssueConfirmEvidence;
			return;
		}
		fixture.m_Enqueue = fixture.m_Support.EnqueueAcceptedExactPlayerSupportProjection(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Request,
			"1200 20 1200",
			"1600 20 1600");
		HST_ForceSpawnResultState batch;
		if (fixture.m_Enqueue)
			batch = fixture.m_Enqueue.m_Batch;
		if (batch)
		{
			batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
			batch.m_sTerminalReason = "typed settlement failure proof";
		}
		HST_ResourceTransactionState money = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sMoneyTransactionId);
		HST_ResourceTransactionState hr = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sHRTransactionId);
		if (hr)
			hr.m_sOperationId = "corrupt_recall_settlement_operation";
		int moneyBefore = fixture.m_State.m_iFactionMoney;
		int hrBefore = fixture.m_State.m_iHR;
		HST_SupportRecallResult recallResult = fixture.m_Support.RecallSupportRequestDetailed(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Economy,
			null,
			fixture.m_Request.m_sRequestId,
			true);
		bool exact = fixture.m_Enqueue && fixture.m_Enqueue.m_bSuccess && batch && money && hr && recallResult;
		if (exact)
			exact = !recallResult.m_bAccepted && !recallResult.m_bStateChanged && !recallResult.m_sFailureReason.IsEmpty() && recallResult.m_sDisposition == "deployment_failure_settlement_failed";
		if (exact)
			exact = money.m_iRefundedAmount == 0 && hr.m_iRefundedAmount == 0 && money.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED && hr.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED;
		if (exact)
			exact = fixture.m_State.m_iFactionMoney == moneyBefore && fixture.m_State.m_iHR == hrBefore;
		if (exact)
			exact = fixture.m_Request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED && fixture.m_Request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED && fixture.m_Request.m_sResolutionKind.IsEmpty() && fixture.m_Request.m_iRefundedHR == 0 && !fixture.m_Request.m_sGroupId.IsEmpty();

		HST_PaidSupportAuthorityProofReport eligibilityFixtureReport = new HST_PaidSupportAuthorityProofReport();
		HST_PaidSupportAuthorityProofFixture eligibilityFixture = BuildAcceptedFixture(eligibilityFixtureReport);
		HST_ForceSpawnResultState eligibilityBatch;
		HST_ResourceTransactionState eligibilityMoney;
		HST_ResourceTransactionState eligibilityHR;
		HST_SupportRecallResult eligibilityRecall;
		int eligibilityMoneyBefore;
		int eligibilityHRBefore;
		if (eligibilityFixture)
		{
			eligibilityFixture.m_Enqueue = eligibilityFixture.m_Support.EnqueueAcceptedExactPlayerSupportProjection(
				eligibilityFixture.m_State,
				eligibilityFixture.m_Preset,
				eligibilityFixture.m_Request,
				"1200 20 1200",
				"1600 20 1600");
			if (eligibilityFixture.m_Enqueue)
				eligibilityBatch = eligibilityFixture.m_Enqueue.m_Batch;
			if (eligibilityBatch)
			{
				eligibilityBatch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
				eligibilityBatch.m_sTerminalReason = "typed settlement eligibility proof";
			}
			eligibilityMoney = eligibilityFixture.m_State.FindResourceTransaction(eligibilityFixture.m_Request.m_sMoneyTransactionId);
			eligibilityHR = eligibilityFixture.m_State.FindResourceTransaction(eligibilityFixture.m_Request.m_sHRTransactionId);
			if (eligibilityHR)
				eligibilityHR.m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_CANCELLED;
			eligibilityMoneyBefore = eligibilityFixture.m_State.m_iFactionMoney;
			eligibilityHRBefore = eligibilityFixture.m_State.m_iHR;
			eligibilityRecall = eligibilityFixture.m_Support.RecallSupportRequestDetailed(
				eligibilityFixture.m_State,
				eligibilityFixture.m_Preset,
				eligibilityFixture.m_Economy,
				null,
				eligibilityFixture.m_Request.m_sRequestId,
				true);
		}
		bool eligibilityExact = eligibilityFixture && eligibilityFixture.m_Enqueue && eligibilityFixture.m_Enqueue.m_bSuccess && eligibilityBatch && eligibilityMoney && eligibilityHR && eligibilityRecall;
		if (eligibilityExact)
			eligibilityExact = !eligibilityRecall.m_bAccepted && !eligibilityRecall.m_bStateChanged && eligibilityRecall.m_sDisposition == "deployment_failure_settlement_failed";
		if (eligibilityExact)
			eligibilityExact = eligibilityMoney.m_iRefundedAmount == 0 && eligibilityHR.m_iRefundedAmount == 0 && eligibilityFixture.m_State.m_iFactionMoney == eligibilityMoneyBefore && eligibilityFixture.m_State.m_iHR == eligibilityHRBefore;
		if (eligibilityExact)
			eligibilityExact = eligibilityFixture.m_Request.m_sResolutionKind.IsEmpty() && !eligibilityFixture.m_Request.m_sGroupId.IsEmpty();
		exact = exact && eligibilityExact;
		report.m_bRecallSettlementFailureExact = exact;
		report.m_sRecallSettlementFailureEvidence = string.Format("accepted %1 | changed %2 | disposition %3 | failure %4 | money refund %5 | HR refund %6 | balances %7/%8 | status %9", recallResult && recallResult.m_bAccepted, recallResult && recallResult.m_bStateChanged, recallResult && recallResult.m_sDisposition, recallResult && recallResult.m_sFailureReason, money && money.m_iRefundedAmount, hr && hr.m_iRefundedAmount, fixture.m_State.m_iFactionMoney, fixture.m_State.m_iHR, fixture.m_Request.m_eStatus);
		report.m_sRecallSettlementFailureEvidence = report.m_sRecallSettlementFailureEvidence + string.Format(" | group linked %1", !fixture.m_Request.m_sGroupId.IsEmpty());
		report.m_sRecallSettlementFailureEvidence = report.m_sRecallSettlementFailureEvidence + string.Format(" | eligibility rejected %1 | refunds %2/%3 | balances %4/%5", eligibilityRecall && !eligibilityRecall.m_bAccepted, eligibilityMoney && eligibilityMoney.m_iRefundedAmount, eligibilityHR && eligibilityHR.m_iRefundedAmount, eligibilityFixture && eligibilityFixture.m_State.m_iFactionMoney, eligibilityFixture && eligibilityFixture.m_State.m_iHR);
	}

	protected void ProveRecallLostGroupSettlement(HST_PaidSupportAuthorityProofReport report)
	{
		HST_PaidSupportAuthorityProofReport validFixtureReport = new HST_PaidSupportAuthorityProofReport();
		HST_PaidSupportAuthorityProofFixture validFixture = BuildAcceptedFixture(validFixtureReport);
		HST_PaidSupportAuthorityProofReport invalidFixtureReport = new HST_PaidSupportAuthorityProofReport();
		HST_PaidSupportAuthorityProofFixture invalidFixture = BuildAcceptedFixture(invalidFixtureReport);
		if (!validFixture || !invalidFixture)
		{
			report.m_sRecallLostGroupEvidence = "accepted lost-group fixture missing";
			return;
		}

		HST_ActiveGroupState validGroup = PrepareLostGroupRecallFixture(validFixture);
		HST_ResourceTransactionState validMoney = validFixture.m_State.FindResourceTransaction(validFixture.m_Request.m_sMoneyTransactionId);
		HST_ResourceTransactionState validHR = validFixture.m_State.FindResourceTransaction(validFixture.m_Request.m_sHRTransactionId);
		HST_SupportRecallResult validRecall = validFixture.m_Support.RecallSupportRequestDetailed(
			validFixture.m_State,
			validFixture.m_Preset,
			validFixture.m_Economy,
			null,
			validFixture.m_Request.m_sRequestId,
			true);

		HST_ActiveGroupState invalidGroup = PrepareLostGroupRecallFixture(invalidFixture);
		HST_ResourceTransactionState invalidMoney = invalidFixture.m_State.FindResourceTransaction(invalidFixture.m_Request.m_sMoneyTransactionId);
		HST_ResourceTransactionState invalidHR = invalidFixture.m_State.FindResourceTransaction(invalidFixture.m_Request.m_sHRTransactionId);
		if (invalidHR)
			invalidHR.m_sOperationId = "corrupt_lost_group_recall_operation";
		int invalidMoneyBefore = invalidFixture.m_State.m_iFactionMoney;
		int invalidHRBefore = invalidFixture.m_State.m_iHR;
		HST_SupportRecallResult invalidRecall = invalidFixture.m_Support.RecallSupportRequestDetailed(
			invalidFixture.m_State,
			invalidFixture.m_Preset,
			invalidFixture.m_Economy,
			null,
			invalidFixture.m_Request.m_sRequestId,
			true);

		bool validExact = validGroup && validMoney && validHR && validRecall && validRecall.m_bAccepted && validRecall.m_bStateChanged && validRecall.m_bTerminal;
		if (validExact)
			validExact = validRecall.m_sDisposition == "recalled_group_lost" && validRecall.m_sDisplayMessage.Contains("spawn_failed") && validFixture.m_Request.m_sResolutionKind == "recalled_group_lost";
		if (validExact)
			validExact = validMoney.m_iRefundedAmount == 0 && validMoney.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED && validHR.m_iRefundedAmount > 0 && validFixture.m_Request.m_sGroupId.IsEmpty();

		bool invalidExact = invalidGroup && invalidMoney && invalidHR && invalidRecall && !invalidRecall.m_bAccepted && !invalidRecall.m_bStateChanged;
		if (invalidExact)
			invalidExact = invalidRecall.m_sDisposition == "lost_group_settlement_failed" && !invalidRecall.m_sFailureReason.IsEmpty() && invalidMoney.m_iRefundedAmount == 0 && invalidHR.m_iRefundedAmount == 0;
		if (invalidExact)
			invalidExact = invalidFixture.m_State.m_iFactionMoney == invalidMoneyBefore && invalidFixture.m_State.m_iHR == invalidHRBefore && invalidFixture.m_Request.m_sResolutionKind.IsEmpty() && !invalidFixture.m_Request.m_sGroupId.IsEmpty();

		report.m_bRecallLostGroupExact = validExact && invalidExact;
		report.m_sRecallLostGroupEvidence = string.Format("valid accepted %1 changed %2 terminal %3 disposition %4 | money refund %5 | HR refund %6 | invalid accepted %7 changed %8 disposition %9", validRecall && validRecall.m_bAccepted, validRecall && validRecall.m_bStateChanged, validRecall && validRecall.m_bTerminal, validRecall && validRecall.m_sDisposition, validMoney && validMoney.m_iRefundedAmount, validHR && validHR.m_iRefundedAmount, invalidRecall && invalidRecall.m_bAccepted, invalidRecall && invalidRecall.m_bStateChanged, invalidRecall && invalidRecall.m_sDisposition);
		report.m_sRecallLostGroupEvidence = report.m_sRecallLostGroupEvidence + string.Format(" | invalid money/HR refunds %1/%2 | group linked %3", invalidMoney && invalidMoney.m_iRefundedAmount, invalidHR && invalidHR.m_iRefundedAmount, !invalidFixture.m_Request.m_sGroupId.IsEmpty());
	}

	protected HST_ActiveGroupState PrepareLostGroupRecallFixture(HST_PaidSupportAuthorityProofFixture fixture)
	{
		if (!fixture)
			return null;
		fixture.m_Enqueue = fixture.m_Support.EnqueueAcceptedExactPlayerSupportProjection(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Request,
			"1200 20 1200",
			"1600 20 1600");
		HST_ForceSpawnResultState batch;
		if (fixture.m_Enqueue)
			batch = fixture.m_Enqueue.m_Batch;
		if (!batch)
			return null;
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
		batch.m_iSuccessfulHandoffCount = 1;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (!slotResult || slotResult.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
				continue;
			slotResult.m_bEverAlive = true;
			slotResult.m_bAliveVerified = true;
			slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
		}
		HST_ActiveGroupState group = fixture.m_State.FindActiveGroup(fixture.m_Request.m_sGroupId);
		if (group)
			group.m_sRuntimeStatus = "spawn_failed";
		return group;
	}

	protected void ProveTerminalRefund(HST_PaidSupportAuthorityProofReport report)
	{
		HST_PaidSupportAuthorityProofReport fixtureReport = new HST_PaidSupportAuthorityProofReport();
		HST_PaidSupportAuthorityProofFixture fixture = BuildAcceptedFixture(fixtureReport);
		if (!fixture)
		{
			report.m_sTerminalRefundEvidence = "accepted terminal fixture missing: " + fixtureReport.m_sIssueConfirmEvidence;
			return;
		}
		fixture.m_State.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_WON;
		bool settled = fixture.m_Support.TickExactPlayerSupportSettlements(fixture.m_State);
		HST_ResourceTransactionState money = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sMoneyTransactionId);
		HST_ResourceTransactionState hr = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sHRTransactionId);
		bool exact = settled && fixture.m_Request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED;
		if (exact)
			exact = fixture.m_State.m_iFactionMoney == fixture.m_iInitialMoney && fixture.m_State.m_iHR == fixture.m_iInitialHR;
		if (exact)
			exact = money && hr && money.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED && hr.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED;
		report.m_bTerminalRefundExact = exact;
		report.m_sTerminalRefundEvidence = string.Format("settled %1 | phase %2 | status %3 | money %4 | HR %5 | money tx %6 | HR tx %7", settled, fixture.m_State.m_ePhase, fixture.m_Request.m_eStatus, fixture.m_State.m_iFactionMoney, fixture.m_State.m_iHR, money && money.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED, hr && hr.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED);
	}

	protected void ProveLegacyMigration(HST_PaidSupportAuthorityProofReport report)
	{
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.m_iSchemaVersion = 45;
		saveData.m_iCampaignSeed = 4646;
		saveData.m_iFactionMoney = 750;
		saveData.m_iHR = 47;
		HST_SupportRequestState legacy = new HST_SupportRequestState();
		legacy.m_sRequestId = "paid_support_legacy_qrf";
		legacy.m_sOperationId = HST_StableIdService.BuildOperationId("support", legacy.m_sRequestId);
		legacy.m_sFactionKey = "FIA";
		legacy.m_eType = HST_ESupportRequestType.HST_SUPPORT_QRF;
		legacy.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
		legacy.m_iMoneyCost = 250;
		legacy.m_iHRCost = 5;
		legacy.m_iRefundedHR = 2;
		legacy.m_iRequestedAtSecond = 10;
		legacy.m_iResolvedAtSecond = 20;
		legacy.m_bPlayerRequested = true;
		saveData.m_aSupportRequests.Insert(legacy);
		HST_CampaignState restored = saveData.Restore();
		HST_SupportRequestState restoredRequest;
		HST_ResourceTransactionState money;
		HST_ResourceTransactionState hr;
		if (restored)
		{
			restoredRequest = restored.FindSupportRequest(legacy.m_sRequestId);
			if (restoredRequest)
			{
				money = restored.FindResourceTransaction(restoredRequest.m_sMoneyTransactionId);
				hr = restored.FindResourceTransaction(restoredRequest.m_sHRTransactionId);
			}
		}
		bool exact = restored && restoredRequest && money && hr && restored.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION;
		if (exact)
			exact = restored.m_iFactionMoney == 750 && restored.m_iHR == 47 && restored.m_aForceQuotes.Count() == 0 && restored.m_aForceManifests.Count() == 0;
		if (exact)
			exact = money.m_iAmount == 250 && money.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED && money.m_iRefundedAmount == 0;
		if (exact)
			exact = hr.m_iAmount == 5 && hr.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED && hr.m_iRefundedAmount == 2;
		if (exact)
			exact = HasEvent(restored, "migration_schema46_player_qrf_ledger_imported");
		int restoredSchema = -1;
		int restoredMoney = -1;
		int restoredHR = -1;
		int restoredHRRefund = -1;
		if (restored)
		{
			restoredSchema = restored.m_iSchemaVersion;
			restoredMoney = restored.m_iFactionMoney;
			restoredHR = restored.m_iHR;
		}
		if (hr)
			restoredHRRefund = hr.m_iRefundedAmount;
		report.m_bMigrationExact = exact;
		report.m_sMigrationEvidence = string.Format("schema %1 | balances %2/%3 | money tx %4 | HR tx %5 refund %6 | no invented quote/manifest %7 | event %8", restoredSchema, restoredMoney, restoredHR, money && money.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED, hr && hr.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED, restoredHRRefund, restored && restored.m_aForceQuotes.Count() == 0 && restored.m_aForceManifests.Count() == 0, HasEvent(restored, "migration_schema46_player_qrf_ledger_imported"));
	}

	protected HST_CampaignState CreateProofState()
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iCampaignSeed = 4646;
		state.m_iElapsedSeconds = 100;
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		state.m_bHQDeployed = true;
		state.m_vHQPosition = "1000 20 1000";
		state.m_iWarLevel = 3;
		state.m_iFactionMoney = 1000;
		state.m_iHR = 50;
		HST_FactionPoolState pool = new HST_FactionPoolState();
		pool.m_sFactionKey = "FIA";
		state.m_aFactionPools.Insert(pool);
		HST_ZoneState source = new HST_ZoneState();
		source.m_sZoneId = "paid_support_source";
		source.m_sDisplayName = "Paid Support Source";
		source.m_sOwnerFactionKey = "FIA";
		source.m_vPosition = "1000 20 1000";
		state.m_aZones.Insert(source);
		HST_ZoneState target = new HST_ZoneState();
		target.m_sZoneId = "paid_support_target";
		target.m_sDisplayName = "Paid Support Target";
		target.m_sOwnerFactionKey = "US";
		target.m_vPosition = "1600 20 1600";
		state.m_aZones.Insert(target);
		return state;
	}

	protected HST_SupportRequestState ResolveOnlySupportRequest(HST_CampaignState state)
	{
		if (!state || state.m_aSupportRequests.Count() != 1)
			return null;
		return state.m_aSupportRequests[0];
	}

	protected bool TransactionCommittedExact(HST_ResourceTransactionState transaction, HST_ForceQuoteState quote, HST_ForceManifestState manifest, string resourceType, int amount)
	{
		return transaction && quote && manifest && transaction.m_sQuoteId == quote.m_sQuoteId
			&& transaction.m_sManifestId == manifest.m_sManifestId && transaction.m_sOperationId == quote.m_sOperationId
			&& transaction.m_sCommandRequestId == quote.m_sConfirmationRequestId && transaction.m_sResourceType == resourceType
			&& transaction.m_iAmount == amount && transaction.m_iRefundedAmount == 0
			&& transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED;
	}

	protected string BuildIssueConfirmEvidence(HST_PaidSupportAuthorityProofFixture fixture, bool issueExact, bool confirmationExact)
	{
		int members = -1;
		int moneyCost = -1;
		int hrCost = -1;
		if (fixture && fixture.m_Issue && fixture.m_Issue.m_Manifest)
		{
			members = fixture.m_Issue.m_Manifest.m_aMembers.Count();
			moneyCost = fixture.m_Issue.m_Manifest.m_iMoneyCost;
			hrCost = fixture.m_Issue.m_Manifest.m_iHRCost;
		}
		int stateMoney = -1;
		int stateHR = -1;
		int requestCount = -1;
		int transactionCount = -1;
		if (fixture && fixture.m_State)
		{
			stateMoney = fixture.m_State.m_iFactionMoney;
			stateHR = fixture.m_State.m_iHR;
			requestCount = fixture.m_State.m_aSupportRequests.Count();
			transactionCount = fixture.m_State.m_aResourceTransactions.Count();
		}
		return string.Format("issue %1 | confirm %2 | members %3 | cost $%4 HR %5 | balances %6/%7 | requests %8 | tx %9", issueExact, confirmationExact, members, moneyCost, hrCost, stateMoney, stateHR, requestCount, transactionCount);
	}

	protected bool PositionsMatch(vector left, vector right)
	{
		return Math.AbsFloat(left[0] - right[0]) < 0.01 && Math.AbsFloat(left[1] - right[1]) < 0.01 && Math.AbsFloat(left[2] - right[2]) < 0.01;
	}

	protected bool HasEvent(HST_CampaignState state, string eventId)
	{
		if (!state)
			return false;
		foreach (HST_CampaignEventState eventState : state.m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				return true;
		}
		return false;
	}
}
