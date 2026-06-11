class HST_MissionUserActionBase : HST_ContextualUserActionBase
{
	protected string ResolveMissionAssetId(IEntity owner)
	{
		if (!owner)
			return "";

		HST_MissionAssetComponent asset = HST_MissionAssetComponent.Cast(owner.FindComponent(HST_MissionAssetComponent));
		if (!asset)
			return "";

		return asset.GetAssetId();
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
