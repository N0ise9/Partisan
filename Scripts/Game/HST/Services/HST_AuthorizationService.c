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
			player.m_bMember = true;
			state.m_aPlayers.Insert(player);
		}

		if (player.m_sFactionKey.IsEmpty())
			player.m_sFactionKey = "FIA";

		if (firstPlayer)
			player.m_bMember = true;

		player.m_bAdmin = player.m_bAdmin || isAdmin;
		if (isAdmin)
			player.m_bMember = true;

		player.m_bGuest = !player.m_bMember;
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

		HST_PlayerState previousCommander = state.FindPlayer(state.m_sCommanderIdentityId);
		if (previousCommander)
		{
			previousCommander.m_bMember = true;
			previousCommander.m_bGuest = false;
		}

		state.m_sCommanderIdentityId = targetIdentityId;
		return true;
	}

	bool TransferCommander(HST_CampaignState state, string actorIdentityId, string targetIdentityId)
	{
		if (!state || actorIdentityId.IsEmpty() || targetIdentityId.IsEmpty() || actorIdentityId == targetIdentityId)
			return false;
		if (state.m_sCommanderIdentityId != actorIdentityId)
			return false;

		HST_PlayerState target = state.FindPlayer(targetIdentityId);
		if (!target || !target.m_bMember)
			return false;

		HST_PlayerState previousCommander = state.FindPlayer(actorIdentityId);
		if (previousCommander)
		{
			previousCommander.m_bMember = true;
			previousCommander.m_bGuest = false;
		}

		state.m_sCommanderIdentityId = targetIdentityId;
		return true;
	}

	bool SetAdminRole(HST_CampaignState state, string actorIdentityId, string targetIdentityId, bool isAdmin)
	{
		HST_PlayerState actor = state.FindPlayer(actorIdentityId);
		HST_PlayerState target = state.FindPlayer(targetIdentityId);
		if (!actor || !actor.m_bAdmin || !target)
			return false;

		target.m_bAdmin = isAdmin;
		if (isAdmin)
		{
			target.m_bMember = true;
			target.m_bGuest = false;
		}

		return true;
	}

	bool CanUseCommanderActions(HST_CampaignState state, string identityId)
	{
		return !identityId.IsEmpty() && state.m_sCommanderIdentityId == identityId;
	}

	bool CanUseAdminActions(HST_CampaignState state, string identityId)
	{
		HST_PlayerState player = state.FindPlayer(identityId);
		return player && player.m_bAdmin;
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
