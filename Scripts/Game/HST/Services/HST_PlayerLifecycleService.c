class HST_PlayerLifecycleService
{
	string ResolveIdentityId(int playerId, string identityId)
	{
		if (!identityId.IsEmpty())
			return identityId;

		return string.Format("workbench_player_%1", playerId);
	}

	HST_PlayerState RegisterConnectedPlayer(HST_CampaignState state, HST_AuthorizationService authorization, int playerId, string identityId, bool isAdmin = false)
	{
		if (!state || !authorization)
			return null;

		string resolvedIdentityId = ResolveIdentityId(playerId, identityId);
		HST_PlayerState player = authorization.RegisterPlayer(state, resolvedIdentityId, isAdmin);
		if (!player)
			return null;

		player.m_iLastSeenPlayerId = playerId;
		return player;
	}

	bool AddPersonalMoney(HST_CampaignState state, string identityId, int amount)
	{
		HST_PlayerState player = state.FindPlayer(identityId);
		if (!player)
			return false;

		player.m_iMoney = Math.Max(0, player.m_iMoney + amount);
		return true;
	}

	bool AddRank(HST_CampaignState state, string identityId, int amount)
	{
		HST_PlayerState player = state.FindPlayer(identityId);
		if (!player)
			return false;

		player.m_iRank = Math.Max(0, player.m_iRank + amount);
		return true;
	}
}
