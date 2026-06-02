class HST_AuthorizationService
{
	HST_PlayerState RegisterPlayer(HST_CampaignState state, string identityId, bool isAdmin = false)
	{
		if (identityId.IsEmpty())
			return null;

		bool firstPlayer = state.m_aPlayers.Count() == 0;
		HST_PlayerState player = state.FindPlayer(identityId);
		if (!player)
		{
			player = new HST_PlayerState();
			player.m_sIdentityId = identityId;
			state.m_aPlayers.Insert(player);
		}

		if (firstPlayer)
			player.m_bMember = true;

		player.m_bGuest = !player.m_bMember;
		player.m_bAdmin = player.m_bAdmin || isAdmin;
		AssignCommanderOnVacancy(state);
		return player;
	}

	bool SetMembership(HST_CampaignState state, string actorIdentityId, string targetIdentityId, bool isMember)
	{
		HST_PlayerState actor = state.FindPlayer(actorIdentityId);
		HST_PlayerState target = state.FindPlayer(targetIdentityId);
		if (!actor || !actor.m_bAdmin || !target)
			return false;

		target.m_bMember = isMember;
		target.m_bGuest = !isMember;
		if (!isMember && state.m_sCommanderIdentityId == targetIdentityId)
			state.m_sCommanderIdentityId = "";

		AssignCommanderOnVacancy(state);
		return true;
	}

	bool OverrideCommander(HST_CampaignState state, string actorIdentityId, string targetIdentityId)
	{
		HST_PlayerState actor = state.FindPlayer(actorIdentityId);
		HST_PlayerState target = state.FindPlayer(targetIdentityId);
		if (!actor || !actor.m_bAdmin || !target || !target.m_bMember)
			return false;

		state.m_sCommanderIdentityId = targetIdentityId;
		return true;
	}

	bool CanUseCommanderActions(HST_CampaignState state, string identityId)
	{
		return !identityId.IsEmpty() && state.m_sCommanderIdentityId == identityId;
	}

	void AssignCommanderOnVacancy(HST_CampaignState state)
	{
		if (!state.m_sCommanderIdentityId.IsEmpty())
			return;

		foreach (HST_PlayerState player : state.m_aPlayers)
		{
			if (!player.m_bMember)
				continue;

			state.m_sCommanderIdentityId = player.m_sIdentityId;
			return;
		}
	}
}
