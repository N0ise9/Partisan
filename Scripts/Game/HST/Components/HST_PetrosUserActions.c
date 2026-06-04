class HST_ContextualUserActionBase : ScriptedUserAction
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

	protected void OpenMenuTab(string tabId, IEntity userEntity)
	{
		HST_CommandMenuComponent menu = HST_CommandMenuComponent.GetLocalInstance();
		if (menu)
		{
			menu.OpenMenuToTab(tabId);
			return;
		}

		Print("h-istasi menu | local command menu component not ready", LogLevel.WARNING);
	}

	protected void RunMenuCommand(string tabId, string commandId, string argument, IEntity userEntity)
	{
		HST_CommandMenuComponent menu = HST_CommandMenuComponent.GetLocalInstance();
		if (menu)
		{
			menu.RunCommandFromContext(tabId, commandId, argument);
			return;
		}

		int playerId = ResolvePlayerId(userEntity);
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (coordinator && Replication.IsServer())
		{
			string result = coordinator.RequestVisibleMenuCommand(playerId, tabId, commandId, argument);
			Print(result);
			return;
		}

		Print("h-istasi command | player request bridge not ready", LogLevel.WARNING);
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
}

class HST_PetrosUserActionBase : HST_ContextualUserActionBase
{
}

class HST_PetrosCommandMenuAction : HST_PetrosUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		OpenMenuTab("petros", pUserEntity);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "h-istasi HQ Menu";
		return true;
	}
}

class HST_PetrosMoveBaseHereAction : HST_PetrosUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RunMenuCommand("petros", "move_hq_here", "", pUserEntity);
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
		OpenMenuTab("arsenal", pUserEntity);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "h-istasi Arsenal";
		return true;
	}
}

class HST_HQArsenalOpenAction : HST_ContextualUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		OpenMenuTab("arsenal", pUserEntity);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Open h-istasi Arsenal";
		return true;
	}
}

class HST_HQArsenalLootNearbyAction : HST_ContextualUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RunMenuCommand("arsenal", "loot_nearby", "", pUserEntity);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Loot nearby to h-istasi Arsenal";
		return true;
	}
}

class HST_VehicleCollectLootAction : HST_ContextualUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RunMenuCommand("arsenal", "vehicle_collect_loot", "", pUserEntity);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Collect nearby loot";
		return true;
	}
}

class HST_VehicleUnloadLootAction : HST_ContextualUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RunMenuCommand("arsenal", "vehicle_unload_loot", "", pUserEntity);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Unload h-istasi cargo";
		return true;
	}
}
