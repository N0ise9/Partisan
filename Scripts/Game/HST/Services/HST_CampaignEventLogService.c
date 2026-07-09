class HST_CampaignEventLogService
{
	static const int MAX_EVENT_ROWS = 256;
	static const int MAX_EVENT_REASON_CHARACTERS = 320;

	HST_CampaignEventState Append(
		HST_CampaignState state,
		string category,
		string aggregateType,
		string aggregateId,
		string commandRequestId,
		string transition,
		string reason)
	{
		if (!state)
			return null;

		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = HST_StableIdService.NextId(state, "event");
		eventState.m_sCategory = category;
		eventState.m_sAggregateType = aggregateType;
		eventState.m_sAggregateId = aggregateId;
		eventState.m_sCommandRequestId = commandRequestId;
		eventState.m_sTransition = transition;
		eventState.m_sReason = Shorten(reason, MAX_EVENT_REASON_CHARACTERS);
		eventState.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		state.m_aCampaignEvents.Insert(eventState);

		while (state.m_aCampaignEvents.Count() > MAX_EVENT_ROWS)
			state.m_aCampaignEvents.Remove(0);

		return eventState;
	}

	string BuildReport(HST_CampaignState state)
	{
		if (!state)
			return "campaign events unavailable";

		string report = string.Format("campaign events | retained %1/%2", state.m_aCampaignEvents.Count(), MAX_EVENT_ROWS);
		if (state.m_aCampaignEvents.Count() <= 0)
			return report;

		HST_CampaignEventState latest = state.m_aCampaignEvents[state.m_aCampaignEvents.Count() - 1];
		if (latest)
			report = report + string.Format(" | latest %1 %2 %3", latest.m_sCategory, latest.m_sAggregateId, latest.m_sTransition);
		return report;
	}

	protected string Shorten(string value, int maxCharacters)
	{
		if (value.Length() <= maxCharacters)
			return value;
		return value.Substring(0, maxCharacters - 3) + "...";
	}
}
