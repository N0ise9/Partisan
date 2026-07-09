class HST_CampaignCommandEnvelope
{
	string m_sRequestId;
	string m_sActorIdentityId;
	string m_sSelectedTabId;
	string m_sCommandId;
	string m_sArgument;
	int m_iReceivedAtSecond;
}

class HST_CampaignCommandResult
{
	HST_ECampaignCommandStatus m_eStatus = HST_ECampaignCommandStatus.HST_COMMAND_PENDING;
	string m_sRequestId;
	string m_sMessage;
	string m_sAggregateId;
	bool m_bShouldExecute;
	ref HST_CommandReceiptState m_Receipt;

	bool IsApplied()
	{
		return m_eStatus == HST_ECampaignCommandStatus.HST_COMMAND_APPLIED || m_eStatus == HST_ECampaignCommandStatus.HST_COMMAND_ALREADY_APPLIED;
	}

	string BuildMessage()
	{
		if (!m_sMessage.IsEmpty())
			return m_sMessage;

		return "h-istasi command | no result";
	}
}

class HST_CampaignCommandService
{
	static const int MAX_RECEIPT_ROWS = 256;
	static const int MAX_RECEIPT_RESULT_CHARACTERS = 1200;
	protected ref HST_CampaignEventLogService m_EventLog;

	void SetEventLogService(HST_CampaignEventLogService eventLog)
	{
		m_EventLog = eventLog;
	}

	HST_CampaignCommandEnvelope BuildEnvelope(HST_CampaignState state, string requestId, string actorIdentityId, string selectedTabId, string commandId, string argument)
	{
		HST_CampaignCommandEnvelope envelope = new HST_CampaignCommandEnvelope();
		if (requestId.IsEmpty())
			requestId = HST_StableIdService.NextId(state, "command");
		envelope.m_sRequestId = requestId;
		envelope.m_sActorIdentityId = actorIdentityId;
		envelope.m_sSelectedTabId = selectedTabId;
		envelope.m_sCommandId = commandId;
		envelope.m_sArgument = argument;
		if (state)
			envelope.m_iReceivedAtSecond = state.m_iElapsedSeconds;
		return envelope;
	}

	HST_CampaignCommandResult Begin(HST_CampaignState state, HST_CampaignCommandEnvelope envelope)
	{
		HST_CampaignCommandResult result = new HST_CampaignCommandResult();
		if (!state || !envelope || envelope.m_sRequestId.IsEmpty() || envelope.m_sCommandId.IsEmpty())
		{
			result.m_eStatus = HST_ECampaignCommandStatus.HST_COMMAND_REJECTED;
			result.m_sMessage = "h-istasi command | invalid command envelope";
			return result;
		}

		result.m_sRequestId = envelope.m_sRequestId;
		HST_CommandReceiptState existing = state.FindCommandReceipt(envelope.m_sRequestId);
		if (!existing)
		{
			result.m_bShouldExecute = true;
			return result;
		}

		result.m_Receipt = existing;
		result.m_sAggregateId = existing.m_sAggregateId;
		if (existing.m_sActorIdentityId != envelope.m_sActorIdentityId || existing.m_sCommandId != envelope.m_sCommandId || existing.m_sArgument != envelope.m_sArgument)
		{
			result.m_eStatus = HST_ECampaignCommandStatus.HST_COMMAND_CONFLICT;
			result.m_sMessage = "h-istasi command | request id conflict: " + envelope.m_sRequestId;
			AppendCommandEvent(state, envelope, "conflict", result.m_sMessage);
			return result;
		}

		result.m_sMessage = existing.m_sResult;
		if (existing.m_eStatus == HST_ECampaignCommandStatus.HST_COMMAND_REJECTED)
		{
			result.m_eStatus = HST_ECampaignCommandStatus.HST_COMMAND_REJECTED;
			AppendCommandEvent(state, envelope, "already_rejected", existing.m_sResult);
			return result;
		}

		result.m_eStatus = HST_ECampaignCommandStatus.HST_COMMAND_ALREADY_APPLIED;
		AppendCommandEvent(state, envelope, "already_applied", existing.m_sResult);
		return result;
	}

	HST_CampaignCommandResult Complete(HST_CampaignState state, HST_CampaignCommandEnvelope envelope, string message, string aggregateId = "")
	{
		HST_CampaignCommandResult result = new HST_CampaignCommandResult();
		if (!state || !envelope || envelope.m_sRequestId.IsEmpty())
		{
			result.m_eStatus = HST_ECampaignCommandStatus.HST_COMMAND_REJECTED;
			result.m_sMessage = "h-istasi command | could not record command result";
			return result;
		}

		HST_CommandReceiptState existing = state.FindCommandReceipt(envelope.m_sRequestId);
		if (existing)
			return Begin(state, envelope);

		HST_CommandReceiptState receipt = new HST_CommandReceiptState();
		receipt.m_sRequestId = envelope.m_sRequestId;
		receipt.m_sActorIdentityId = envelope.m_sActorIdentityId;
		receipt.m_sCommandId = envelope.m_sCommandId;
		receipt.m_sArgument = envelope.m_sArgument;
		receipt.m_sResult = Shorten(message, MAX_RECEIPT_RESULT_CHARACTERS);
		receipt.m_sAggregateId = aggregateId;
		receipt.m_iReceivedAtSecond = envelope.m_iReceivedAtSecond;
		receipt.m_iCompletedAtSecond = state.m_iElapsedSeconds;
		if (IsFailureMessage(message))
			receipt.m_eStatus = HST_ECampaignCommandStatus.HST_COMMAND_REJECTED;
		else
			receipt.m_eStatus = HST_ECampaignCommandStatus.HST_COMMAND_APPLIED;

		state.m_aCommandReceipts.Insert(receipt);
		while (state.m_aCommandReceipts.Count() > MAX_RECEIPT_ROWS)
			state.m_aCommandReceipts.Remove(0);

		result.m_eStatus = receipt.m_eStatus;
		result.m_sRequestId = receipt.m_sRequestId;
		result.m_sMessage = receipt.m_sResult;
		result.m_sAggregateId = receipt.m_sAggregateId;
		result.m_Receipt = receipt;
		AppendCommandEvent(state, envelope, CommandStatusLabel(receipt.m_eStatus), message);
		return result;
	}

	string BuildReport(HST_CampaignState state)
	{
		if (!state)
			return "command receipts unavailable";

		int applied;
		int rejected;
		foreach (HST_CommandReceiptState receipt : state.m_aCommandReceipts)
		{
			if (!receipt)
				continue;
			if (receipt.m_eStatus == HST_ECampaignCommandStatus.HST_COMMAND_APPLIED)
				applied++;
			else if (receipt.m_eStatus == HST_ECampaignCommandStatus.HST_COMMAND_REJECTED)
				rejected++;
		}

		return string.Format("command receipts | retained %1/%2 | applied %3 | rejected %4", state.m_aCommandReceipts.Count(), MAX_RECEIPT_ROWS, applied, rejected);
	}

	protected void AppendCommandEvent(HST_CampaignState state, HST_CampaignCommandEnvelope envelope, string transition, string reason)
	{
		if (!m_EventLog || !state || !envelope)
			return;

		m_EventLog.Append(state, "command", "command", envelope.m_sCommandId, envelope.m_sRequestId, transition, reason);
	}

	protected bool IsFailureMessage(string message)
	{
		if (message.IsEmpty())
			return true;

		string firstLine = message;
		int lineEnd = message.IndexOf("\n");
		if (lineEnd >= 0)
			firstLine = message.Substring(0, lineEnd);
		return firstLine.Contains("failed") || firstLine.Contains("not ready") || firstLine.Contains("invalid") || firstLine.Contains("unavailable") || firstLine.Contains("denied");
	}

	protected string CommandStatusLabel(HST_ECampaignCommandStatus status)
	{
		if (status == HST_ECampaignCommandStatus.HST_COMMAND_APPLIED)
			return "applied";
		if (status == HST_ECampaignCommandStatus.HST_COMMAND_REJECTED)
			return "rejected";
		if (status == HST_ECampaignCommandStatus.HST_COMMAND_ALREADY_APPLIED)
			return "already_applied";
		if (status == HST_ECampaignCommandStatus.HST_COMMAND_CONFLICT)
			return "conflict";
		return "pending";
	}

	protected string Shorten(string value, int maxCharacters)
	{
		if (value.Length() <= maxCharacters)
			return value;
		return value.Substring(0, maxCharacters - 3) + "...";
	}
}
