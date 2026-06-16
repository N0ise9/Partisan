class HST_MissionCaptiveFreeAction : HST_MissionUserActionBase
{
	override bool CanShowForMissionAsset(IEntity owner)
	{
		HST_MissionAssetState asset = ResolveMissionAssetState(owner);
		if (!asset)
			return true;

		return asset.m_sKind == "captive" && !asset.m_bPickedUp && !asset.m_bDelivered && !asset.m_bDestroyed;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RunMissionCommand(pOwnerEntity, pUserEntity, "mission_captive_extract");
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Free captive";
		return true;
	}
}

class HST_MissionCaptiveFollowAction : HST_MissionUserActionBase
{
	override bool CanShowForMissionAsset(IEntity owner)
	{
		HST_MissionAssetState asset = ResolveMissionAssetState(owner);
		if (!asset)
			return true;

		return asset.m_sKind == "captive" && asset.m_bPickedUp && !asset.m_bDelivered && !asset.m_bDestroyed;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RunMissionCommand(pOwnerEntity, pUserEntity, "mission_captive_follow");
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Order POWs to follow";
		return true;
	}
}

class HST_MissionCaptiveExtractAction : HST_MissionUserActionBase
{
	override bool CanShowForMissionAsset(IEntity owner)
	{
		HST_MissionAssetState asset = ResolveMissionAssetState(owner);
		if (!asset)
			return true;

		return asset.m_sKind == "captive" && asset.m_bPickedUp && asset.m_bAttachedToCarrier && !asset.m_bDelivered && !asset.m_bDestroyed;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RunMissionCommand(pOwnerEntity, pUserEntity, "mission_captive_extract");
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Extract captive";
		return true;
	}
}
