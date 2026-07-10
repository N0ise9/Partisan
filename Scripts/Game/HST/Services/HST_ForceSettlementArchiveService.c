class HST_ForceSettlementArchiveResult
{
	bool m_bStateChanged;
	int m_iArchivedCount;
	int m_iPrunedTombstoneCount;
	int m_iDeferredCount;
	ref array<string> m_aEvidence = {};
}

class HST_ForceSettlementArchiveService
{
	static const int MIN_ACCEPTED_RECORD_RETENTION_SECONDS = 600;
	static const int MIN_TOMBSTONE_RETENTION_SECONDS = 86400;
	static const int MAX_TOMBSTONE_ROWS = 256;
	static const int MAX_TOTAL_PLANNING_AUTHORITY_ROWS = 320;
	protected ref HST_ForcePlanningIntegrityService m_Integrity = new HST_ForcePlanningIntegrityService();
	protected ref HST_OperationService m_Operations = new HST_OperationService();

	HST_ForceSettlementArchiveResult ArchiveSettledRecords(HST_CampaignState state)
	{
		HST_ForceSettlementArchiveResult result = new HST_ForceSettlementArchiveResult();
		if (!state)
		{
			result.m_aEvidence.Insert("force settlement archive unavailable: campaign state missing");
			return result;
		}

		result.m_iPrunedTombstoneCount = PruneExpiredTombstones(state);
		if (result.m_iPrunedTombstoneCount > 0)
			result.m_bStateChanged = true;

		for (int quoteIndex = 0; quoteIndex < state.m_aForceQuotes.Count(); quoteIndex++)
		{
			HST_ForceQuoteState quote = state.m_aForceQuotes[quoteIndex];
			if (!quote || quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED)
				continue;
			int acceptedSecond = quote.m_iAcceptedAtSecond;
			if (acceptedSecond <= 0)
				acceptedSecond = quote.m_iCreatedAtSecond;
			if (state.m_iElapsedSeconds - acceptedSecond < MIN_ACCEPTED_RECORD_RETENTION_SECONDS)
				continue;
			if (state.m_aForceSettlementTombstones.Count() >= MAX_TOMBSTONE_ROWS)
			{
				result.m_iDeferredCount++;
				continue;
			}

			HST_ForceManifestState manifest;
			array<ref HST_ResourceTransactionState> transactions = {};
			HST_SupportRequestState supportRequest;
			HST_GarrisonState garrison;
			HST_OperationRecordState operation;
			string settlementKind;
			string failure = ValidateArchiveCandidate(state, quote, manifest, transactions, supportRequest, garrison, operation, settlementKind);
			if (!failure.IsEmpty())
			{
				result.m_iDeferredCount++;
				continue;
			}

			HST_ForceSettlementTombstoneState tombstone = BuildTombstone(quote, manifest, transactions, operation, settlementKind, state.m_iElapsedSeconds);
			if (!IsReplayTombstoneValid(tombstone))
			{
				result.m_iDeferredCount++;
				continue;
			}

			if (operation && !m_Operations.RemoveArchivedOperation(state, operation))
			{
				result.m_iDeferredCount++;
				continue;
			}
			state.m_aForceSettlementTombstones.Insert(tombstone);
			RemoveAcceptedGarrisonManifestLink(garrison, manifest.m_sManifestId);
			RemoveArchivedTransactions(state, transactions);
			RemoveArchivedManifest(state, manifest);
			state.m_aForceQuotes.Remove(quoteIndex);
			quoteIndex--;
			result.m_iArchivedCount++;
			result.m_bStateChanged = true;
			result.m_aEvidence.Insert(string.Format("archived accepted quote %1 as settlement %2", tombstone.m_sQuoteId, tombstone.m_sSettlementKind));
		}
		return result;
	}

	bool CanAdmitPlanningRecord(HST_CampaignState state, out string failureReason)
	{
		failureReason = "";
		if (!state)
		{
			failureReason = "planning history authority is unavailable";
			return false;
		}
		int retainedRows = state.m_aForceQuotes.Count() + state.m_aForceSettlementTombstones.Count();
		if (retainedRows < MAX_TOTAL_PLANNING_AUTHORITY_ROWS)
			return true;
		failureReason = string.Format("planning history retention capacity reached (%1); settled authority rows remain inside their replay window or retain live backlinks", MAX_TOTAL_PLANNING_AUTHORITY_ROWS);
		return false;
	}

	HST_ForceQuoteState BuildReplayQuote(HST_ForceSettlementTombstoneState tombstone)
	{
		if (!IsReplayTombstoneValid(tombstone))
			return null;
		HST_ForceQuoteState quote = new HST_ForceQuoteState();
		quote.m_sQuoteId = tombstone.m_sQuoteId;
		quote.m_sManifestId = tombstone.m_sManifestId;
		quote.m_sManifestHash = tombstone.m_sManifestHash;
		quote.m_sOperationId = tombstone.m_sOperationId;
		quote.m_sCommandRequestId = tombstone.m_sCommandRequestId;
		quote.m_sConfirmationRequestId = tombstone.m_sConfirmationRequestId;
		quote.m_sActorIdentityId = tombstone.m_sActorIdentityId;
		quote.m_sQuoteKind = tombstone.m_sQuoteKind;
		quote.m_sSupportRequestId = tombstone.m_sSupportRequestId;
		quote.m_sCapabilityId = tombstone.m_sCapabilityId;
		quote.m_sAssetProfileId = tombstone.m_sAssetProfileId;
		quote.m_sFactionKey = tombstone.m_sFactionKey;
		quote.m_sSourceZoneId = tombstone.m_sSourceZoneId;
		quote.m_sTargetZoneId = tombstone.m_sTargetZoneId;
		quote.m_sCatalogVersion = tombstone.m_sCatalogVersion;
		quote.m_sPolicyId = tombstone.m_sPolicyId;
		quote.m_sMoneyTransactionId = tombstone.m_sMoneyTransactionId;
		quote.m_sHRTransactionId = tombstone.m_sHRTransactionId;
		quote.m_sAttackTransactionId = tombstone.m_sAttackTransactionId;
		quote.m_sSupportTransactionId = tombstone.m_sSupportTransactionId;
		quote.m_vSourcePosition = tombstone.m_vSourcePosition;
		quote.m_vTargetPosition = tombstone.m_vTargetPosition;
		quote.m_eSupportType = tombstone.m_eSupportType;
		quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED;
		quote.m_iRequestedMemberCount = tombstone.m_iRequestedMemberCount;
		quote.m_iAcceptedMemberCount = tombstone.m_iAcceptedMemberCount;
		quote.m_iRequestedVehicleCount = tombstone.m_iRequestedVehicleCount;
		quote.m_iAcceptedVehicleCount = tombstone.m_iAcceptedVehicleCount;
		quote.m_iMoneyCost = tombstone.m_iMoneyCost;
		quote.m_iHRCost = tombstone.m_iHRCost;
		quote.m_iEquipmentCost = tombstone.m_iEquipmentCost;
		quote.m_iAttackResourceCost = tombstone.m_iAttackResourceCost;
		quote.m_iSupportResourceCost = tombstone.m_iSupportResourceCost;
		quote.m_iCreatedAtSecond = tombstone.m_iCreatedAtSecond;
		quote.m_iExpiresAtSecond = tombstone.m_iAcceptedAtSecond;
		quote.m_iAcceptedAtSecond = tombstone.m_iAcceptedAtSecond;
		quote.m_iRevision = 1;
		quote.m_iETASeconds = tombstone.m_iETASeconds;
		quote.m_iCooldownSeconds = tombstone.m_iCooldownSeconds;
		quote.m_bAllOrNothing = tombstone.m_bAllOrNothing;
		return quote;
	}

	HST_ForceManifestState BuildReplayManifest(HST_ForceSettlementTombstoneState tombstone)
	{
		if (!IsReplayTombstoneValid(tombstone))
			return null;
		HST_ForceManifestState manifest = new HST_ForceManifestState();
		manifest.m_sManifestId = tombstone.m_sManifestId;
		manifest.m_sManifestHash = tombstone.m_sManifestHash;
		manifest.m_sOperationId = tombstone.m_sOperationId;
		manifest.m_sQuoteId = tombstone.m_sQuoteId;
		manifest.m_sCommandRequestId = tombstone.m_sCommandRequestId;
		manifest.m_sForceKind = tombstone.m_sQuoteKind;
		manifest.m_sFactionRole = "resistance";
		manifest.m_sFactionKey = tombstone.m_sFactionKey;
		manifest.m_sIntentId = tombstone.m_sQuoteKind;
		manifest.m_sSourceZoneId = tombstone.m_sSourceZoneId;
		manifest.m_sTargetZoneId = tombstone.m_sTargetZoneId;
		manifest.m_sCatalogVersion = tombstone.m_sCatalogVersion;
		manifest.m_sPolicyId = tombstone.m_sPolicyId;
		manifest.m_iRequestedMemberCount = tombstone.m_iRequestedMemberCount;
		manifest.m_iAcceptedMemberCount = tombstone.m_iAcceptedMemberCount;
		manifest.m_iRequestedVehicleCount = tombstone.m_iRequestedVehicleCount;
		manifest.m_iAcceptedVehicleCount = tombstone.m_iAcceptedVehicleCount;
		manifest.m_iMoneyCost = tombstone.m_iMoneyCost;
		manifest.m_iHRCost = tombstone.m_iHRCost;
		manifest.m_iEquipmentCost = tombstone.m_iEquipmentCost;
		manifest.m_iAttackResourceCost = tombstone.m_iAttackResourceCost;
		manifest.m_iSupportResourceCost = tombstone.m_iSupportResourceCost;
		manifest.m_iCreatedAtSecond = tombstone.m_iCreatedAtSecond;
		manifest.m_bFrozen = true;
		return manifest;
	}

	bool IsReplayTombstoneValid(HST_ForceSettlementTombstoneState tombstone)
	{
		if (!tombstone || tombstone.m_sQuoteId.IsEmpty() || tombstone.m_sManifestId.IsEmpty()
			|| tombstone.m_sManifestHash.IsEmpty() || tombstone.m_sOperationId.IsEmpty()
			|| tombstone.m_sCommandRequestId.IsEmpty() || tombstone.m_sConfirmationRequestId.IsEmpty()
			|| tombstone.m_sActorIdentityId.IsEmpty() || tombstone.m_sQuoteKind.IsEmpty())
			return false;
		if (tombstone.m_sQuoteKind != HST_ForcePlanningService.QUOTE_KIND_GARRISON
			&& tombstone.m_sQuoteKind != HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_QRF)
			return false;
		if (tombstone.m_sTargetZoneId.IsEmpty() || tombstone.m_sFactionKey.IsEmpty()
			|| tombstone.m_iAcceptedAtSecond <= 0 || tombstone.m_iArchivedAtSecond < tombstone.m_iAcceptedAtSecond)
			return false;
		if (tombstone.m_sQuoteKind == HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_QRF
			&& (tombstone.m_sSupportRequestId.IsEmpty() || tombstone.m_eSupportType != HST_ESupportRequestType.HST_SUPPORT_QRF))
			return false;
		if (tombstone.m_iAcceptedMemberCount < 0 || tombstone.m_iAcceptedVehicleCount < 0)
			return false;
		if (tombstone.m_iOperationContractVersion > 0)
		{
			if (tombstone.m_iOperationContractVersion != HST_OperationService.EXACT_PLAYER_QRF_CONTRACT_VERSION
				|| tombstone.m_sQuoteKind != HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_QRF
				|| tombstone.m_eSupportType != HST_ESupportRequestType.HST_SUPPORT_QRF
				|| tombstone.m_sOperationSettlementId.IsEmpty() || tombstone.m_iOperationRevision <= 0
				|| tombstone.m_eOperationTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN
				|| tombstone.m_eOperationTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
				|| tombstone.m_sOperationSettlementId != HST_OperationService.BuildSettlementId(tombstone.m_sOperationId, tombstone.m_sSettlementKind))
				return false;
		}
		else if (!tombstone.m_sOperationSettlementId.IsEmpty() || tombstone.m_iOperationRevision != 0
			|| tombstone.m_eOperationTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN)
			return false;
		array<string> transactionIds = {};
		foreach (HST_ForceSettlementTransactionTombstoneState transaction : tombstone.m_aTransactions)
		{
			if (!transaction || transaction.m_sTransactionId.IsEmpty() || transactionIds.Contains(transaction.m_sTransactionId))
				return false;
			transactionIds.Insert(transaction.m_sTransactionId);
		}
		if (!ValidateTombstoneTransaction(tombstone, tombstone.m_sMoneyTransactionId, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, tombstone.m_iMoneyCost))
			return false;
		if (!ValidateTombstoneTransaction(tombstone, tombstone.m_sHRTransactionId, HST_ResourceLedgerService.RESOURCE_HR, tombstone.m_iHRCost))
			return false;
		return true;
	}

	protected string ValidateArchiveCandidate(
		HST_CampaignState state,
		HST_ForceQuoteState quote,
		out HST_ForceManifestState manifest,
		array<ref HST_ResourceTransactionState> transactions,
		out HST_SupportRequestState supportRequest,
		out HST_GarrisonState garrison,
		out HST_OperationRecordState operation,
		out string settlementKind)
	{
		manifest = null;
		supportRequest = null;
		garrison = null;
		operation = null;
		settlementKind = "";
		if (!state || !quote || !transactions)
			return "archive candidate authority is incomplete";
		if (state.FindForceSettlementTombstone(quote.m_sQuoteId))
			return "archive candidate already has a tombstone";
		HST_ForceSettlementTombstoneState commandTombstone = state.FindForceSettlementTombstoneByCommandRequest(quote.m_sCommandRequestId);
		if (commandTombstone)
			return "archive candidate command identity already has a tombstone";
		if (CountQuotes(state, quote.m_sQuoteId) != 1 || CountManifests(state, quote.m_sManifestId) != 1)
			return "archive candidate quote or manifest identity is ambiguous";
		manifest = state.FindForceManifest(quote.m_sManifestId);
		if (!manifest || !m_Integrity || manifest.m_sQuoteId != quote.m_sQuoteId || manifest.m_sManifestHash != quote.m_sManifestHash
			|| manifest.m_sOperationId != quote.m_sOperationId || !manifest.m_bFrozen
			|| m_Integrity.BuildManifestHash(manifest) != quote.m_sManifestHash)
			return "archive candidate manifest identity conflicts";
		if (!ManifestArchiveHeaderMatchesQuote(manifest, quote))
			return "archive candidate manifest header conflicts with the accepted quote";
		string transactionFailure = CollectArchiveTransactions(state, quote, transactions);
		if (!transactionFailure.IsEmpty())
			return transactionFailure;
		if (HasProjectionBacklink(state, quote.m_sManifestId, quote.m_sQuoteId))
			return "archive candidate retains a projection backlink";

		if (quote.m_sQuoteKind == HST_ForcePlanningService.QUOTE_KIND_GARRISON)
		{
			int acceptedLinkCount;
			foreach (HST_GarrisonState candidate : state.m_aGarrisons)
			{
				if (!candidate)
					continue;
				foreach (string acceptedManifestId : candidate.m_aAcceptedManifestIds)
				{
					if (acceptedManifestId != quote.m_sManifestId)
						continue;
					acceptedLinkCount++;
					garrison = candidate;
				}
			}
			if (acceptedLinkCount != 1 || !garrison || garrison.m_sZoneId != quote.m_sTargetZoneId || garrison.m_sFactionKey != quote.m_sFactionKey)
				return "accepted garrison provenance is missing or ambiguous";
			settlementKind = "strategic_garrison_registered";
			return "";
		}

		if (quote.m_sQuoteKind != HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_QRF)
			return "accepted quote kind has no archive settlement policy";
		int requestCount;
		foreach (HST_SupportRequestState candidateRequest : state.m_aSupportRequests)
		{
			if (!candidateRequest || candidateRequest.m_sQuoteId != quote.m_sQuoteId)
				continue;
			requestCount++;
			supportRequest = candidateRequest;
		}
		if (requestCount != 1 || !supportRequest || supportRequest.m_sRequestId != quote.m_sSupportRequestId
			|| supportRequest.m_sManifestId != quote.m_sManifestId || supportRequest.m_sMoneyTransactionId != quote.m_sMoneyTransactionId
			|| supportRequest.m_sHRTransactionId != quote.m_sHRTransactionId)
			return "accepted support settlement provenance is missing or ambiguous";
		if (supportRequest.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED
			&& supportRequest.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
			return "accepted support operation is not terminal";
		settlementKind = supportRequest.m_sResolutionKind;
		if (settlementKind.IsEmpty())
			settlementKind = "terminal_support_settlement";
		if (HST_OperationService.RequiresOperation(supportRequest))
		{
			operation = state.FindOperation(supportRequest.m_sOperationId);
			string operationFailure = m_Operations.ValidateExactPlayerQRF(state, operation, supportRequest, quote, manifest);
			if (!operationFailure.IsEmpty())
				return "accepted support operation conflicts with archive identity: " + operationFailure;
			if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
				|| operation.m_sSettlementId != HST_OperationService.BuildSettlementId(operation.m_sOperationId, settlementKind))
				return "accepted support operation is not settled under the archived settlement identity";
		}
		else if (state.FindOperation(supportRequest.m_sOperationId))
			return "pre-contract support unexpectedly owns an operation record";
		return "";
	}

	protected string CollectArchiveTransactions(HST_CampaignState state, HST_ForceQuoteState quote, array<ref HST_ResourceTransactionState> transactions)
	{
		array<string> expectedIds = {};
		AppendUniqueId(expectedIds, quote.m_sMoneyTransactionId);
		AppendUniqueId(expectedIds, quote.m_sHRTransactionId);
		AppendUniqueId(expectedIds, quote.m_sAttackTransactionId);
		AppendUniqueId(expectedIds, quote.m_sSupportTransactionId);
		foreach (string transactionId : expectedIds)
		{
			if (state.CountForceSettlementTombstonesByTransaction(transactionId) > 0)
				return "accepted quote transaction identity already exists in the settlement archive";
			if (CountTransactions(state, transactionId) != 1)
				return "accepted quote transaction identity is missing or ambiguous";
			HST_ResourceTransactionState transaction = state.FindResourceTransaction(transactionId);
			if (!transaction || transaction.m_sQuoteId != quote.m_sQuoteId || transaction.m_sManifestId != quote.m_sManifestId
				|| transaction.m_sOperationId != quote.m_sOperationId || transaction.m_sActorIdentityId != quote.m_sActorIdentityId
				|| !LiveTransactionSettlementIsConsistent(transaction))
				return "accepted quote transaction is not terminal or conflicts with archive identity";
			transactions.Insert(transaction);
		}
		foreach (HST_ResourceTransactionState linkedTransaction : state.m_aResourceTransactions)
		{
			if (!linkedTransaction)
				continue;
			if (linkedTransaction.m_sQuoteId != quote.m_sQuoteId && linkedTransaction.m_sManifestId != quote.m_sManifestId)
				continue;
			if (!ContainsTransaction(transactions, linkedTransaction.m_sTransactionId))
				return "accepted quote has an unexpected linked transaction";
		}
		if (quote.m_iMoneyCost > 0 && !quote.m_sMoneyTransactionId.IsEmpty() && !ContainsTransaction(transactions, quote.m_sMoneyTransactionId))
			return "accepted quote money transaction is missing";
		if (quote.m_iHRCost > 0 && !quote.m_sHRTransactionId.IsEmpty() && !ContainsTransaction(transactions, quote.m_sHRTransactionId))
			return "accepted quote HR transaction is missing";
		return "";
	}

	protected bool ManifestArchiveHeaderMatchesQuote(HST_ForceManifestState manifest, HST_ForceQuoteState quote)
	{
		if (!manifest || !quote)
			return false;
		bool matches = manifest.m_sCommandRequestId == quote.m_sCommandRequestId;
		matches = matches && manifest.m_sFactionKey == quote.m_sFactionKey;
		matches = matches && manifest.m_sSourceZoneId == quote.m_sSourceZoneId && manifest.m_sTargetZoneId == quote.m_sTargetZoneId;
		matches = matches && manifest.m_sCatalogVersion == quote.m_sCatalogVersion && manifest.m_sPolicyId == quote.m_sPolicyId;
		matches = matches && manifest.m_iRequestedMemberCount == quote.m_iRequestedMemberCount;
		matches = matches && manifest.m_iAcceptedMemberCount == quote.m_iAcceptedMemberCount;
		matches = matches && manifest.m_iRequestedVehicleCount == quote.m_iRequestedVehicleCount;
		matches = matches && manifest.m_iAcceptedVehicleCount == quote.m_iAcceptedVehicleCount;
		matches = matches && manifest.m_iMoneyCost == quote.m_iMoneyCost && manifest.m_iHRCost == quote.m_iHRCost;
		matches = matches && manifest.m_iEquipmentCost == quote.m_iEquipmentCost;
		matches = matches && manifest.m_iAttackResourceCost == quote.m_iAttackResourceCost;
		matches = matches && manifest.m_iSupportResourceCost == quote.m_iSupportResourceCost;
		return matches;
	}

	protected HST_ForceSettlementTombstoneState BuildTombstone(
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		array<ref HST_ResourceTransactionState> transactions,
		HST_OperationRecordState operation,
		string settlementKind,
		int archivedAtSecond)
	{
		HST_ForceSettlementTombstoneState tombstone = new HST_ForceSettlementTombstoneState();
		tombstone.m_sQuoteId = quote.m_sQuoteId;
		tombstone.m_sManifestId = quote.m_sManifestId;
		tombstone.m_sManifestHash = quote.m_sManifestHash;
		tombstone.m_sOperationId = quote.m_sOperationId;
		tombstone.m_sCommandRequestId = quote.m_sCommandRequestId;
		tombstone.m_sConfirmationRequestId = quote.m_sConfirmationRequestId;
		tombstone.m_sActorIdentityId = quote.m_sActorIdentityId;
		tombstone.m_sQuoteKind = quote.m_sQuoteKind;
		tombstone.m_sSupportRequestId = quote.m_sSupportRequestId;
		tombstone.m_sCapabilityId = quote.m_sCapabilityId;
		tombstone.m_sAssetProfileId = quote.m_sAssetProfileId;
		tombstone.m_sFactionKey = quote.m_sFactionKey;
		tombstone.m_sSourceZoneId = quote.m_sSourceZoneId;
		tombstone.m_sTargetZoneId = quote.m_sTargetZoneId;
		tombstone.m_sCatalogVersion = quote.m_sCatalogVersion;
		tombstone.m_sPolicyId = quote.m_sPolicyId;
		tombstone.m_sMoneyTransactionId = quote.m_sMoneyTransactionId;
		tombstone.m_sHRTransactionId = quote.m_sHRTransactionId;
		tombstone.m_sAttackTransactionId = quote.m_sAttackTransactionId;
		tombstone.m_sSupportTransactionId = quote.m_sSupportTransactionId;
		tombstone.m_sSettlementKind = settlementKind;
		if (operation)
		{
			tombstone.m_sOperationSettlementId = operation.m_sSettlementId;
			tombstone.m_iOperationContractVersion = operation.m_iContractVersion;
			tombstone.m_iOperationRevision = operation.m_iRevision;
			tombstone.m_eOperationTerminalResult = operation.m_eTerminalResult;
		}
		tombstone.m_vSourcePosition = quote.m_vSourcePosition;
		tombstone.m_vTargetPosition = quote.m_vTargetPosition;
		tombstone.m_eSupportType = quote.m_eSupportType;
		tombstone.m_iRequestedMemberCount = quote.m_iRequestedMemberCount;
		tombstone.m_iAcceptedMemberCount = quote.m_iAcceptedMemberCount;
		tombstone.m_iRequestedVehicleCount = quote.m_iRequestedVehicleCount;
		tombstone.m_iAcceptedVehicleCount = quote.m_iAcceptedVehicleCount;
		tombstone.m_iMoneyCost = quote.m_iMoneyCost;
		tombstone.m_iHRCost = quote.m_iHRCost;
		tombstone.m_iEquipmentCost = quote.m_iEquipmentCost;
		tombstone.m_iAttackResourceCost = quote.m_iAttackResourceCost;
		tombstone.m_iSupportResourceCost = quote.m_iSupportResourceCost;
		tombstone.m_iCreatedAtSecond = quote.m_iCreatedAtSecond;
		tombstone.m_iAcceptedAtSecond = quote.m_iAcceptedAtSecond;
		tombstone.m_iArchivedAtSecond = archivedAtSecond;
		tombstone.m_iETASeconds = quote.m_iETASeconds;
		tombstone.m_iCooldownSeconds = quote.m_iCooldownSeconds;
		tombstone.m_bAllOrNothing = quote.m_bAllOrNothing;
		foreach (HST_ResourceTransactionState transaction : transactions)
		{
			HST_ForceSettlementTransactionTombstoneState transactionTombstone = new HST_ForceSettlementTransactionTombstoneState();
			transactionTombstone.m_sTransactionId = transaction.m_sTransactionId;
			transactionTombstone.m_sResourceType = transaction.m_sResourceType;
			transactionTombstone.m_sLastSettlementId = transaction.m_sLastSettlementId;
			transactionTombstone.m_eStatus = transaction.m_eStatus;
			transactionTombstone.m_iAmount = transaction.m_iAmount;
			transactionTombstone.m_iRefundedAmount = transaction.m_iRefundedAmount;
			transactionTombstone.m_iSettledAtSecond = transaction.m_iSettledAtSecond;
			tombstone.m_aTransactions.Insert(transactionTombstone);
		}
		return tombstone;
	}

	protected int PruneExpiredTombstones(HST_CampaignState state)
	{
		int removed;
		while (state.m_aForceSettlementTombstones.Count() >= MAX_TOMBSTONE_ROWS
			|| state.m_aForceSettlementTombstones.Count() + state.m_aForceQuotes.Count() >= MAX_TOTAL_PLANNING_AUTHORITY_ROWS)
		{
			int oldestIndex = FindOldestExpiredTombstoneIndex(state);
			if (oldestIndex < 0)
				break;
			state.m_aForceSettlementTombstones.Remove(oldestIndex);
			removed++;
		}
		return removed;
	}

	protected int FindOldestExpiredTombstoneIndex(HST_CampaignState state)
	{
		int selectedIndex = -1;
		int selectedSecond = int.MAX;
		for (int index = 0; index < state.m_aForceSettlementTombstones.Count(); index++)
		{
			HST_ForceSettlementTombstoneState tombstone = state.m_aForceSettlementTombstones[index];
			if (!tombstone)
				return index;
			if (state.m_iElapsedSeconds - tombstone.m_iArchivedAtSecond < MIN_TOMBSTONE_RETENTION_SECONDS)
				continue;
			if (tombstone.m_iArchivedAtSecond < selectedSecond)
			{
				selectedSecond = tombstone.m_iArchivedAtSecond;
				selectedIndex = index;
			}
		}
		return selectedIndex;
	}

	protected bool HasProjectionBacklink(HST_CampaignState state, string manifestId, string quoteId)
	{
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && group.m_sManifestId == manifestId)
				return true;
		}
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (order && order.m_sManifestId == manifestId)
				return true;
		}
		foreach (HST_ForceSpawnResultState spawnResult : state.m_aForceSpawnResults)
		{
			if (spawnResult && spawnResult.m_sManifestId == manifestId)
				return true;
		}
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (request && request.m_sManifestId == manifestId && request.m_sQuoteId != quoteId)
				return true;
		}
		return false;
	}

	protected bool ValidateTombstoneTransaction(HST_ForceSettlementTombstoneState tombstone, string transactionId, string resourceType, int amount)
	{
		if (amount <= 0 && transactionId.IsEmpty())
			return true;
		if (transactionId.IsEmpty())
			return false;
		HST_ForceSettlementTransactionTombstoneState transaction = tombstone.FindTransaction(transactionId);
		return transaction && transaction.m_sResourceType == resourceType && transaction.m_iAmount == amount
			&& TombstoneTransactionSettlementIsConsistent(transaction);
	}

	protected bool LiveTransactionSettlementIsConsistent(HST_ResourceTransactionState transaction)
	{
		if (!transaction || transaction.m_iAmount < 0 || transaction.m_iRefundedAmount < 0 || transaction.m_iRefundedAmount > transaction.m_iAmount)
			return false;
		if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED)
			return transaction.m_iRefundedAmount == 0;
		if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED)
			return transaction.m_iRefundedAmount > 0 && transaction.m_iRefundedAmount < transaction.m_iAmount;
		if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED
			|| transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_CANCELLED)
			return transaction.m_iRefundedAmount == transaction.m_iAmount;
		return false;
	}

	protected bool TombstoneTransactionSettlementIsConsistent(HST_ForceSettlementTransactionTombstoneState transaction)
	{
		if (!transaction || transaction.m_iAmount < 0 || transaction.m_iRefundedAmount < 0 || transaction.m_iRefundedAmount > transaction.m_iAmount)
			return false;
		if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED)
			return transaction.m_iRefundedAmount == 0;
		if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED)
			return transaction.m_iRefundedAmount > 0 && transaction.m_iRefundedAmount < transaction.m_iAmount;
		if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED
			|| transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_CANCELLED)
			return transaction.m_iRefundedAmount == transaction.m_iAmount;
		return false;
	}

	protected void AppendUniqueId(array<string> values, string value)
	{
		if (!value.IsEmpty() && !values.Contains(value))
			values.Insert(value);
	}

	protected bool ContainsTransaction(array<ref HST_ResourceTransactionState> transactions, string transactionId)
	{
		foreach (HST_ResourceTransactionState transaction : transactions)
		{
			if (transaction && transaction.m_sTransactionId == transactionId)
				return true;
		}
		return false;
	}

	protected int CountQuotes(HST_CampaignState state, string quoteId)
	{
		int count;
		foreach (HST_ForceQuoteState quote : state.m_aForceQuotes)
		{
			if (quote && quote.m_sQuoteId == quoteId)
				count++;
		}
		return count;
	}

	protected int CountManifests(HST_CampaignState state, string manifestId)
	{
		int count;
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
		{
			if (manifest && manifest.m_sManifestId == manifestId)
				count++;
		}
		return count;
	}

	protected int CountTransactions(HST_CampaignState state, string transactionId)
	{
		int count;
		foreach (HST_ResourceTransactionState transaction : state.m_aResourceTransactions)
		{
			if (transaction && transaction.m_sTransactionId == transactionId)
				count++;
		}
		return count;
	}

	protected void RemoveAcceptedGarrisonManifestLink(HST_GarrisonState garrison, string manifestId)
	{
		if (!garrison)
			return;
		int index = garrison.m_aAcceptedManifestIds.Find(manifestId);
		if (index >= 0)
			garrison.m_aAcceptedManifestIds.Remove(index);
	}

	protected void RemoveArchivedTransactions(HST_CampaignState state, array<ref HST_ResourceTransactionState> transactions)
	{
		for (int index = state.m_aResourceTransactions.Count() - 1; index >= 0; index--)
		{
			HST_ResourceTransactionState transaction = state.m_aResourceTransactions[index];
			if (transactions.Contains(transaction))
				state.m_aResourceTransactions.Remove(index);
		}
	}

	protected void RemoveArchivedManifest(HST_CampaignState state, HST_ForceManifestState manifest)
	{
		int index = state.m_aForceManifests.Find(manifest);
		if (index >= 0)
			state.m_aForceManifests.Remove(index);
	}
}
