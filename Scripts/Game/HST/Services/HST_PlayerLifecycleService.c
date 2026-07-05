class HST_PlayerLifecycleService
{
	static const string DEFAULT_PLAYER_FACTION = "FIA";

	string ResolveIdentityId(int playerId, string identityId)
	{
		if (!identityId.IsEmpty())
			return identityId;

		string backendIdentityId = ResolveBackendIdentityId(playerId);
		if (!backendIdentityId.IsEmpty())
			return backendIdentityId;

		return BuildWorkbenchIdentityId(playerId);
	}

	HST_PlayerState RegisterConnectedPlayer(HST_CampaignState state, HST_AuthorizationService authorization, int playerId, string identityId, bool isAdmin = false)
	{
		if (!state || !authorization)
			return null;

		string resolvedIdentityId = ResolveIdentityId(playerId, identityId);
		MigrateWorkbenchIdentity(state, BuildWorkbenchIdentityId(playerId), resolvedIdentityId, playerId);

		HST_PlayerState player = authorization.RegisterPlayer(state, resolvedIdentityId, isAdmin);
		if (!player)
			return null;

		player.m_sFactionKey = DEFAULT_PLAYER_FACTION;
		player.m_iLastSeenPlayerId = playerId;
		RefreshPlayerDisplayName(player, playerId, resolvedIdentityId);
		return player;
	}

	bool RefreshPlayerDisplayName(HST_PlayerState player, int playerId, string identityId = "")
	{
		if (!player)
			return false;

		string displayName = ResolvePlayerDisplayName(playerId, identityId);
		if (displayName.IsEmpty())
			return false;
		if (player.m_sDisplayName == displayName)
			return false;

		player.m_sDisplayName = displayName;
		return true;
	}

	string ResolvePlayerDisplayName(int playerId, string identityId = "")
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return "";

		string displayName;
		if (playerId > 0)
			displayName = playerManager.GetPlayerName(playerId);
		if (displayName.IsEmpty() && !identityId.IsEmpty())
			displayName = playerManager.GetPlayerNameByIdentity(identityId);

		if (displayName == identityId)
			return "";

		return displayName;
	}

	protected string BuildWorkbenchIdentityId(int playerId)
	{
		return string.Format("workbench_player_%1", playerId);
	}

	protected string ResolveBackendIdentityId(int playerId)
	{
		if (playerId <= 0 || !Replication.IsServer())
			return "";

		BackendApi backendApi = GetGame().GetBackendApi();
		if (!backendApi)
			return "";

		return backendApi.GetPlayerIdentityId(playerId);
	}

	protected void MigrateWorkbenchIdentity(HST_CampaignState state, string placeholderIdentityId, string resolvedIdentityId, int playerId)
	{
		if (!state || placeholderIdentityId.IsEmpty() || resolvedIdentityId.IsEmpty() || placeholderIdentityId == resolvedIdentityId)
			return;

		HST_PlayerState placeholder = state.FindPlayer(placeholderIdentityId);
		HST_PlayerState resolved = state.FindPlayer(resolvedIdentityId);
		if (!placeholder)
		{
			RewriteIdentityReferences(state, placeholderIdentityId, resolvedIdentityId);
			return;
		}

		if (!resolved)
		{
			placeholder.m_sIdentityId = resolvedIdentityId;
			if (placeholder.m_iLastSeenPlayerId <= 0)
				placeholder.m_iLastSeenPlayerId = playerId;
			RewriteIdentityReferences(state, placeholderIdentityId, resolvedIdentityId);
			Print(string.Format("h-istasi player lifecycle | migrated player %1 identity %2 -> %3", playerId, placeholderIdentityId, resolvedIdentityId));
			return;
		}

		MergePlayerState(resolved, placeholder);
		if (resolved.m_iLastSeenPlayerId <= 0)
			resolved.m_iLastSeenPlayerId = playerId;
		RewriteIdentityReferences(state, placeholderIdentityId, resolvedIdentityId);
		RemovePlayerState(state, placeholderIdentityId);
		Print(string.Format("h-istasi player lifecycle | merged player %1 placeholder identity %2 into %3", playerId, placeholderIdentityId, resolvedIdentityId));
	}

	protected void MergePlayerState(HST_PlayerState target, HST_PlayerState source)
	{
		if (!target || !source || target == source)
			return;

		if (target.m_sFactionKey.IsEmpty() && !source.m_sFactionKey.IsEmpty())
			target.m_sFactionKey = source.m_sFactionKey;
		if (target.m_sDisplayName.IsEmpty() && !source.m_sDisplayName.IsEmpty())
			target.m_sDisplayName = source.m_sDisplayName;

		bool preserveTargetGuest = !target.m_bMember && target.m_bGuest && !source.m_bAdmin;
		if (!preserveTargetGuest)
			target.m_bMember = target.m_bMember || source.m_bMember || source.m_bAdmin;

		target.m_bAdmin = target.m_bAdmin || source.m_bAdmin;
		if (target.m_bAdmin)
			target.m_bMember = true;
		target.m_bGuest = !target.m_bMember;
		target.m_iMoney = Math.Max(target.m_iMoney, source.m_iMoney);
		target.m_iRank = Math.Max(target.m_iRank, source.m_iRank);
		target.m_iSpawnCount = Math.Max(target.m_iSpawnCount, source.m_iSpawnCount);
		if (source.m_iLastSeenPlayerId > 0)
			target.m_iLastSeenPlayerId = source.m_iLastSeenPlayerId;

		if (source.m_bHasSpawnRecord && !target.m_bHasSpawnRecord)
		{
			target.m_bHasSpawnRecord = true;
			target.m_sLastSpawnPrefab = source.m_sLastSpawnPrefab;
			target.m_vLastSpawnPosition = source.m_vLastSpawnPosition;
		}
	}

	protected void RewriteIdentityReferences(HST_CampaignState state, string oldIdentityId, string newIdentityId)
	{
		if (!state || oldIdentityId.IsEmpty() || newIdentityId.IsEmpty() || oldIdentityId == newIdentityId)
			return;

		if (state.m_sCommanderIdentityId == oldIdentityId)
			state.m_sCommanderIdentityId = newIdentityId;

		foreach (HST_PlayerUndercoverState undercover : state.m_aUndercoverPlayers)
		{
			if (undercover && undercover.m_sIdentityId == oldIdentityId)
				undercover.m_sIdentityId = newIdentityId;
		}

		foreach (HST_SavedLoadoutState loadout : state.m_aSavedLoadouts)
		{
			if (loadout && loadout.m_sOwnerIdentityId == oldIdentityId)
				loadout.m_sOwnerIdentityId = newIdentityId;
		}

		foreach (HST_IssuedLoadoutItemState issuedItem : state.m_aIssuedLoadoutItems)
		{
			if (issuedItem && issuedItem.m_sOwnerIdentityId == oldIdentityId)
				issuedItem.m_sOwnerIdentityId = newIdentityId;
		}

		foreach (HST_LoadoutEditorSessionState session : state.m_aLoadoutEditorSessions)
		{
			if (session && session.m_sOwnerIdentityId == oldIdentityId)
				session.m_sOwnerIdentityId = newIdentityId;
		}
	}

	protected void RemovePlayerState(HST_CampaignState state, string identityId)
	{
		if (!state || identityId.IsEmpty())
			return;

		for (int i = state.m_aPlayers.Count() - 1; i >= 0; i--)
		{
			HST_PlayerState player = state.m_aPlayers[i];
			if (player && player.m_sIdentityId == identityId)
				state.m_aPlayers.Remove(i);
		}
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
