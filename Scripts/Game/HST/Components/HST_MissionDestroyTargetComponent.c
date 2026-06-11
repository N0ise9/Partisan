[ComponentEditorProps(category: "h-istasi", description: "Reports destroyed mission target state to the server coordinator")]
class HST_MissionDestroyTargetComponentClass : ScriptComponentClass
{
}

class HST_MissionDestroyTargetComponent : ScriptComponent
{
	protected bool m_bReportedDestroyed;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_bReportedDestroyed || !Replication.IsServer() || !owner)
			return;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(owner.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager || damageManager.GetState() != EDamageState.DESTROYED)
			return;

		HST_MissionAssetComponent asset = HST_MissionAssetComponent.Cast(owner.FindComponent(HST_MissionAssetComponent));
		if (!asset)
			return;

		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
			return;

		m_bReportedDestroyed = true;
		Print(coordinator.RequestServerMissionAssetDestroyed(asset.GetAssetId(), owner.GetOrigin()));
	}
}

class HST_MissionDestroyTargetSabotageAction : HST_MissionUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RunMissionCommand(pOwnerEntity, pUserEntity, "mission_asset_sabotage");
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Sabotage target";
		return true;
	}
}
