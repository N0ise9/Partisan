class HST_StableIdService
{
	static string NextId(HST_CampaignState state, string prefix)
	{
		if (!state)
			return "";

		if (prefix.IsEmpty())
			prefix = "id";

		int sequence = Math.Max(1, state.m_iNextAuthoritySequence);
		state.m_iNextAuthoritySequence = sequence + 1;
		return string.Format("%1_%2_%3", prefix, state.m_iCampaignSeed, sequence);
	}

	static string BuildOperationId(string sourceType, string sourceId)
	{
		if (sourceType.IsEmpty() || sourceId.IsEmpty())
			return "";

		return "operation_" + sourceType + "_" + sourceId;
	}

	static string BuildTransactionId(string operationId, string resourceType)
	{
		if (operationId.IsEmpty() || resourceType.IsEmpty())
			return "";

		return "transaction_" + operationId + "_" + resourceType;
	}
}
