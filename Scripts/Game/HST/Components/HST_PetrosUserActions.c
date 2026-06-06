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

	override bool CanBroadcastScript()
	{
		return false;
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

	protected void OpenLoadoutEditor(IEntity userEntity)
	{
		HST_CommandMenuComponent menu = HST_CommandMenuComponent.GetLocalInstance();
		if (menu)
			menu.CloseMenuFromExternal();

		HST_LoadoutEditorComponent editor = HST_LoadoutEditorComponent.GetLocalInstance();
		if (editor)
		{
			editor.OpenFromArsenal(userEntity);
			return;
		}

		RunMenuCommand("arsenal", "loadout_editor_open", "", userEntity);
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

	protected string ResolveRuntimeEntityId(IEntity entity)
	{
		if (!entity)
			return "";

		BaseRplComponent rpl = BaseRplComponent.Cast(entity.FindComponent(BaseRplComponent));
		if (rpl)
			return string.Format("rpl_%1", rpl.Id());

		string prefab = "";
		if (entity.GetPrefabData())
			prefab = entity.GetPrefabData().GetPrefabName();

		return string.Format("local_%1_%2", prefab, entity.GetOrigin());
	}
}

[ComponentEditorProps(category: "h-istasi", description: "Disables inherited stock arsenal actions on the h-istasi HQ arsenal")]
class HST_HQArsenalActionFilterComponentClass : ScriptComponentClass
{
}

class HST_HQArsenalActionFilterComponent : ScriptComponent
{
	protected bool m_bActionsFiltered;
	protected int m_iFilterFrames;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}

	override void EOnInit(IEntity owner)
	{
		FilterActions(owner);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_bActionsFiltered && m_iFilterFrames > 90)
			return;

		FilterActions(owner);
	}

	protected void FilterActions(IEntity owner)
	{
		if (!owner)
			return;

		ActionsManagerComponent actionsManager = ActionsManagerComponent.Cast(owner.FindComponent(ActionsManagerComponent));
		if (!actionsManager)
			return;

		array<BaseUserAction> actions = {};
		actionsManager.GetActionsList(actions);
		int actionCount = actions.Count();
		if (actionCount <= 0)
			return;

		foreach (BaseUserAction action : actions)
		{
			if (!action)
				continue;

			if (HST_HQArsenalLoadoutEditorAction.Cast(action))
				continue;

			action.SetActionEnabled_S(false);
		}

		m_bActionsFiltered = true;
		m_iFilterFrames++;
		if (m_iFilterFrames == 1 || m_iFilterFrames == 90)
			Print(string.Format("h-istasi arsenal | filtered %1 inherited action(s); custom loadout editor action remains authoritative", Math.Max(0, actionCount - 1)));
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

class HST_HQArsenalLoadoutEditorAction : HST_ContextualUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		OpenLoadoutEditor(pUserEntity);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Open Loadout Editor";
		return true;
	}
}

class HST_VehicleCollectLootAction : HST_ContextualUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RunMenuCommand("arsenal", "vehicle_collect_loot", ResolveRuntimeEntityId(pOwnerEntity), pUserEntity);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Load loot to vehicle";
		return true;
	}
}

class HST_VehicleUnloadLootAction : HST_ContextualUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RunMenuCommand("arsenal", "vehicle_unload_loot", ResolveRuntimeEntityId(pOwnerEntity), pUserEntity);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Unload vehicle loot to arsenal";
		return true;
	}
}
