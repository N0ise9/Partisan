class HST_PetrosUserActionBase : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		return true;
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return true;
	}

	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	protected int ResolvePlayerId(IEntity userEntity)
	{
		if (!userEntity)
			return 0;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		int playerId = playerManager.GetPlayerIdFromControlledEntity(userEntity);
		if (playerId > 0)
			return playerId;

		BaseRplComponent rpl = BaseRplComponent.Cast(userEntity.FindComponent(BaseRplComponent));
		if (rpl)
			return playerManager.GetPlayerIdFromEntityRplId(rpl.Id());

		return 0;
	}

	protected void OpenMenuWithResult(string tabId, int playerId, string result)
	{
		HST_CommandMenuComponent menu = HST_CommandMenuComponent.GetLocalInstance();
		if (menu)
			menu.OpenMenuToTab(tabId);

		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (menu && coordinator && Replication.IsServer())
			menu.OnServerSnapshot(coordinator.BuildVisibleMenuPayload(playerId, tabId, result), result);

		if (!result.IsEmpty())
			Print(result);
	}
}

class HST_PetrosCommandMenuAction : HST_PetrosUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		OpenMenuWithResult("petros", ResolvePlayerId(pUserEntity), "");
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "h-istasi HQ Menu";
		return true;
	}
}

class HST_PetrosMoveBaseHereAction : HST_PetrosUserActionBase
{
	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}

	override bool CanBroadcastScript()
	{
		return false;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		int playerId = ResolvePlayerId(pUserEntity);
		string result = "h-istasi Petros | server coordinator not ready";
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (coordinator && Replication.IsServer())
		{
			if (coordinator.RequestCommanderMoveHQToPlayer(playerId))
				result = "h-istasi Petros | base moved to your position";
			else
				result = "h-istasi Petros | move base denied or failed";
		}

		OpenMenuWithResult("petros", playerId, result);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Move h-istasi base here";
		return true;
	}
}

class HST_PetrosArsenalMenuAction : HST_PetrosUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		OpenMenuWithResult("arsenal", ResolvePlayerId(pUserEntity), "");
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "h-istasi Arsenal";
		return true;
	}
}
