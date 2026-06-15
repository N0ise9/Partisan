class HST_MissionUserActionBase : HST_ContextualUserActionBase
{
	bool CanShowForMissionAsset(IEntity owner)
	{
		return true;
	}

	protected string ResolveMissionAssetId(IEntity owner)
	{
		if (!owner)
			return "";

		HST_MissionAssetComponent asset = HST_MissionAssetComponent.Cast(owner.FindComponent(HST_MissionAssetComponent));
		if (!asset)
			return "";

		return asset.GetAssetId();
	}

	protected HST_MissionAssetState ResolveMissionAssetState(IEntity owner)
	{
		string assetId = ResolveMissionAssetId(owner);
		if (assetId.IsEmpty())
			return null;

		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
			return null;

		HST_CampaignState state = coordinator.GetState();
		if (!state)
			return null;

		return state.FindMissionAsset(assetId);
	}

	protected void RunMissionCommand(IEntity owner, IEntity user, string commandId)
	{
		string assetId = ResolveMissionAssetId(owner);
		if (assetId.IsEmpty())
		{
			Print("h-istasi mission action | failed: mission asset component missing", LogLevel.WARNING);
			return;
		}

		RunMenuCommand("missions", commandId, assetId, user);
	}
}

[ComponentEditorProps(category: "h-istasi", description: "Keeps mission-owned context actions visible while suppressing inherited stock actions")]
class HST_MissionActionFilterComponentClass : ScriptComponentClass
{
}

class HST_MissionActionFilterComponent : ScriptComponent
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
		if (actions.Count() <= 0)
			return;

		int missionActions;
		foreach (BaseUserAction action : actions)
		{
			if (!action)
				continue;

			HST_MissionUserActionBase missionAction = HST_MissionUserActionBase.Cast(action);
			if (missionAction)
			{
				action.SetActionEnabled_S(missionAction.CanShowForMissionAsset(owner));
				missionActions++;
				continue;
			}

			action.SetActionEnabled_S(false);
		}

		m_bActionsFiltered = true;
		m_iFilterFrames++;
		int inheritedActions = actions.Count() - missionActions;
		if (inheritedActions < 0)
			inheritedActions = 0;
		if (m_iFilterFrames == 1 && inheritedActions > 0)
			Print(string.Format("h-istasi mission action | filtered %1 inherited action(s); mission actions %2 remain visible", inheritedActions, missionActions));
	}
}

class HST_MissionCargoLoadAction : HST_MissionUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RunMissionCommand(pOwnerEntity, pUserEntity, "mission_asset_load");
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Load mission cargo";
		return true;
	}
}

class HST_MissionCargoUnloadAction : HST_MissionUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RunMissionCommand(pOwnerEntity, pUserEntity, "mission_asset_unload");
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Unload mission cargo";
		return true;
	}
}

class HST_MissionCargoDeliverAction : HST_MissionUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RunMissionCommand(pOwnerEntity, pUserEntity, "mission_asset_deliver");
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Deliver mission cargo";
		return true;
	}
}
