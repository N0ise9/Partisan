class HST_ResourceTransactionResult
{
	bool m_bSuccess;
	bool m_bAlreadyApplied;
	string m_sFailureReason;
	ref HST_ResourceTransactionState m_Transaction;
}

class HST_ResourceLedgerService
{
	static const string RESOURCE_FACTION_MONEY = "faction_money";
	static const string RESOURCE_HR = "hr";
	protected ref HST_CampaignEventLogService m_EventLog;

	void SetEventLogService(HST_CampaignEventLogService eventLog)
	{
		m_EventLog = eventLog;
	}

	HST_ResourceTransactionResult ReserveCost(
		HST_CampaignState state,
		HST_EconomyService economy,
		string transactionId,
		string commandRequestId,
		string operationId,
		string actorIdentityId,
		string resourceType,
		int amount,
		string reason)
	{
		HST_ResourceTransactionResult result = new HST_ResourceTransactionResult();
		if (!state || !economy)
		{
			result.m_sFailureReason = "campaign state or economy service not ready";
			return result;
		}
		if (transactionId.IsEmpty() || operationId.IsEmpty() || resourceType.IsEmpty())
		{
			result.m_sFailureReason = "transaction identity incomplete";
			return result;
		}
		if (amount < 0)
		{
			result.m_sFailureReason = "negative reservation amount";
			return result;
		}
		if (!IsSupportedResourceType(resourceType))
		{
			result.m_sFailureReason = "unsupported resource type";
			return result;
		}

		HST_ResourceTransactionState existing = state.FindResourceTransaction(transactionId);
		if (existing)
		{
			result.m_Transaction = existing;
			if (existing.m_sCommandRequestId != commandRequestId || existing.m_sOperationId != operationId || existing.m_sActorIdentityId != actorIdentityId || existing.m_sResourceType != resourceType || existing.m_iAmount != amount)
			{
				result.m_sFailureReason = "transaction id conflict";
				return result;
			}

			result.m_bSuccess = existing.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_RESERVED || existing.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED || existing.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED || existing.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED;
			result.m_bAlreadyApplied = result.m_bSuccess;
			if (!result.m_bSuccess)
				result.m_sFailureReason = "transaction already cancelled";
			return result;
		}

		if (!SpendResource(state, economy, resourceType, amount))
		{
			result.m_sFailureReason = "resource reservation spend failed";
			return result;
		}

		HST_ResourceTransactionState transaction = new HST_ResourceTransactionState();
		transaction.m_sTransactionId = transactionId;
		transaction.m_sCommandRequestId = commandRequestId;
		transaction.m_sOperationId = operationId;
		transaction.m_sActorIdentityId = actorIdentityId;
		transaction.m_sResourceType = resourceType;
		transaction.m_sReason = reason;
		transaction.m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_RESERVED;
		transaction.m_iAmount = amount;
		transaction.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		state.m_aResourceTransactions.Insert(transaction);
		result.m_bSuccess = true;
		result.m_Transaction = transaction;
		AppendTransactionEvent(state, transaction, "reserved", reason);
		return result;
	}

	bool CommitReserved(HST_CampaignState state, string transactionId)
	{
		if (!state || transactionId.IsEmpty())
			return false;

		HST_ResourceTransactionState transaction = state.FindResourceTransaction(transactionId);
		if (!transaction)
			return false;
		if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED || transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED || transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED)
			return true;
		if (transaction.m_eStatus != HST_EResourceTransactionStatus.HST_TRANSACTION_RESERVED)
			return false;

		transaction.m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED;
		transaction.m_iSettledAtSecond = state.m_iElapsedSeconds;
		AppendTransactionEvent(state, transaction, "committed", transaction.m_sReason);
		return true;
	}

	int CancelReservation(HST_CampaignState state, HST_EconomyService economy, string transactionId, string settlementId, string reason)
	{
		if (!state || !economy || transactionId.IsEmpty())
			return 0;

		HST_ResourceTransactionState transaction = state.FindResourceTransaction(transactionId);
		if (!transaction || transaction.m_eStatus != HST_EResourceTransactionStatus.HST_TRANSACTION_RESERVED)
			return 0;
		if (!settlementId.IsEmpty() && transaction.m_sLastSettlementId == settlementId)
			return 0;

		int refund = Math.Max(0, transaction.m_iAmount - transaction.m_iRefundedAmount);
		AddResource(state, economy, transaction.m_sResourceType, refund);
		transaction.m_iRefundedAmount += refund;
		transaction.m_sLastSettlementId = settlementId;
		transaction.m_sReason = reason;
		transaction.m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_CANCELLED;
		transaction.m_iSettledAtSecond = state.m_iElapsedSeconds;
		AppendTransactionEvent(state, transaction, "cancelled", reason);
		return refund;
	}

	int RefundCommitted(HST_CampaignState state, HST_EconomyService economy, string transactionId, string settlementId, int requestedAmount, string reason)
	{
		if (!state || !economy || transactionId.IsEmpty())
			return 0;

		HST_ResourceTransactionState transaction = state.FindResourceTransaction(transactionId);
		if (!transaction)
			return 0;
		if (transaction.m_eStatus != HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED && transaction.m_eStatus != HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED)
			return 0;
		if (!settlementId.IsEmpty() && transaction.m_sLastSettlementId == settlementId)
			return 0;

		int remaining = Math.Max(0, transaction.m_iAmount - transaction.m_iRefundedAmount);
		int refund = Math.Min(remaining, Math.Max(0, requestedAmount));
		if (refund <= 0)
			return 0;

		AddResource(state, economy, transaction.m_sResourceType, refund);
		transaction.m_iRefundedAmount += refund;
		transaction.m_sLastSettlementId = settlementId;
		transaction.m_sReason = reason;
		transaction.m_iSettledAtSecond = state.m_iElapsedSeconds;
		if (transaction.m_iRefundedAmount >= transaction.m_iAmount)
			transaction.m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED;
		else
			transaction.m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED;
		AppendTransactionEvent(state, transaction, "refunded", reason);
		return refund;
	}

	int ReconcileOpenReservations(HST_CampaignState state, HST_EconomyService economy)
	{
		if (!state || !economy)
			return 0;

		int reconciled;
		foreach (HST_ResourceTransactionState transaction : state.m_aResourceTransactions)
		{
			if (!transaction || transaction.m_eStatus != HST_EResourceTransactionStatus.HST_TRANSACTION_RESERVED)
				continue;

			string settlementId = "restore_reconcile_" + transaction.m_sTransactionId;
			CancelReservation(state, economy, transaction.m_sTransactionId, settlementId, "open reservation cancelled during restore reconciliation");
			reconciled++;
		}

		return reconciled;
	}

	string BuildReport(HST_CampaignState state)
	{
		if (!state)
			return "resource ledger unavailable";

		int reserved;
		int committed;
		int refunded;
		int cancelled;
		foreach (HST_ResourceTransactionState transaction : state.m_aResourceTransactions)
		{
			if (!transaction)
				continue;
			if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_RESERVED)
				reserved++;
			else if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED)
				committed++;
			else if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED || transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED)
				refunded++;
			else if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_CANCELLED)
				cancelled++;
		}

		return string.Format("resource ledger | total %1 | reserved %2 | committed %3 | refunded %4 | cancelled %5", state.m_aResourceTransactions.Count(), reserved, committed, refunded, cancelled);
	}

	protected bool SpendResource(HST_CampaignState state, HST_EconomyService economy, string resourceType, int amount)
	{
		if (amount <= 0)
			return true;
		if (resourceType == RESOURCE_FACTION_MONEY)
			return economy.SpendFactionMoney(state, amount);
		if (resourceType == RESOURCE_HR)
			return economy.SpendHR(state, amount);
		return false;
	}

	protected bool IsSupportedResourceType(string resourceType)
	{
		return resourceType == RESOURCE_FACTION_MONEY || resourceType == RESOURCE_HR;
	}

	protected void AddResource(HST_CampaignState state, HST_EconomyService economy, string resourceType, int amount)
	{
		if (amount <= 0)
			return;
		if (resourceType == RESOURCE_FACTION_MONEY)
			economy.AddFactionMoney(state, amount);
		else if (resourceType == RESOURCE_HR)
			economy.AddHR(state, amount);
	}

	protected void AppendTransactionEvent(HST_CampaignState state, HST_ResourceTransactionState transaction, string transition, string reason)
	{
		if (!m_EventLog || !state || !transaction)
			return;

		m_EventLog.Append(state, "resource", "transaction", transaction.m_sTransactionId, transaction.m_sCommandRequestId, transition, reason);
	}
}
