[ComponentEditorProps(category: "h-istasi", description: "Reports definitive exact-rescue captive death evidence to server authority")]
class HST_RescueCaptiveLifecycleComponentClass : ScriptComponentClass
{
}

class HST_RescueCaptiveLifecycleComponent : ScriptComponent
{
	protected bool m_bDeathReported;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_bDeathReported || !Replication.IsServer() || !owner)
			return;

		HST_MissionAssetComponent identity = HST_MissionAssetComponent.Cast(
			owner.FindComponent(HST_MissionAssetComponent));
		if (!identity || identity.GetAssetId().IsEmpty())
			return;

		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
			return;
		HST_CampaignState state = coordinator.GetState();
		HST_MissionAssetState asset;
		if (state)
			asset = state.FindMissionAsset(identity.GetAssetId());
		if (!asset || asset.m_iRescueContractVersion != HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION)
			return;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(
			owner.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager || damageManager.GetState() != EDamageState.DESTROYED)
			return;

		m_bDeathReported = true;
		Print(coordinator.RequestServerMissionAssetDestroyed(identity.GetAssetId(), owner.GetOrigin()));
	}
}
