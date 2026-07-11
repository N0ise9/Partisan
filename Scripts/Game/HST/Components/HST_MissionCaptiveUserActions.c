class HST_MissionCaptiveActionPolicy
{
	static HST_MissionAssetComponent ResolveProjection(IEntity owner)
	{
		if (!owner)
			return null;
		return HST_MissionAssetComponent.Cast(owner.FindComponent(HST_MissionAssetComponent));
	}

	static bool IsExactRescueAsset(
		HST_MissionAssetState asset,
		HST_MissionAssetComponent projection)
	{
		if (projection && projection.HasRescueActionProjection())
		{
			return projection.GetRescueActionContractVersion()
					== HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION
				&& projection.IsRescueActionMissionActive()
				&& !projection.IsRescueActionAuthorityQuarantined();
		}
		if (!asset || asset.m_iRescueContractVersion != HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION)
			return false;

		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
			return false;
		HST_CampaignState state = coordinator.GetState();
		if (!state)
			return false;
		HST_ActiveMissionState mission = state.FindActiveMission(asset.m_sMissionInstanceId);
		return HST_RescuePOWOperationService.IsExactMission(mission)
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE;
	}

	static bool IsProjectionPending(HST_MissionAssetComponent projection)
	{
		return projection && !projection.IsRescueActionProjectionEvaluated();
	}

	static bool IsRescueClaimant(
		HST_MissionAssetState asset,
		HST_MissionAssetComponent projection)
	{
		if (projection && projection.HasRescueActionProjection())
			return true;
		return asset && (asset.m_iRescueContractVersion != 0
			|| asset.m_eRescueDisposition != HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_UNKNOWN);
	}

	static HST_ERescueCaptiveDisposition ResolveDisposition(
		HST_MissionAssetState asset,
		HST_MissionAssetComponent projection)
	{
		if (projection && projection.HasRescueActionProjection())
			return projection.GetRescueActionDisposition();
		if (asset)
			return asset.m_eRescueDisposition;
		return HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_UNKNOWN;
	}

	static bool CanFree(
		HST_MissionAssetState asset,
		HST_MissionAssetComponent projection)
	{
		return ResolveDisposition(asset, projection)
			== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD;
	}

	static bool CanFollow(
		HST_MissionAssetState asset,
		HST_MissionAssetComponent projection)
	{
		return ResolveDisposition(asset, projection)
			== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED;
	}

	static bool CanExtract(
		HST_MissionAssetState asset,
		HST_MissionAssetComponent projection)
	{
		HST_ERescueCaptiveDisposition disposition = ResolveDisposition(asset, projection);
		return disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
			|| disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
	}
}

class HST_MissionCaptiveFreeAction : HST_MissionUserActionBase
{
	override bool CanShowForMissionAsset(IEntity owner)
	{
		HST_MissionAssetState asset = ResolveMissionAssetState(owner);
		HST_MissionAssetComponent projection = HST_MissionCaptiveActionPolicy.ResolveProjection(owner);
		if (HST_MissionCaptiveActionPolicy.IsProjectionPending(projection))
			return false;
		if (HST_MissionCaptiveActionPolicy.IsRescueClaimant(asset, projection))
			return HST_MissionCaptiveActionPolicy.IsExactRescueAsset(asset, projection)
				&& (!asset || asset.m_sKind == "captive")
				&& HST_MissionCaptiveActionPolicy.CanFree(asset, projection);
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
		HST_MissionAssetComponent projection = HST_MissionCaptiveActionPolicy.ResolveProjection(owner);
		if (HST_MissionCaptiveActionPolicy.IsProjectionPending(projection))
			return false;
		if (HST_MissionCaptiveActionPolicy.IsRescueClaimant(asset, projection))
			return HST_MissionCaptiveActionPolicy.IsExactRescueAsset(asset, projection)
				&& (!asset || asset.m_sKind == "captive")
				&& HST_MissionCaptiveActionPolicy.CanFollow(asset, projection);
		if (!asset)
			return true;

		return asset.m_sKind == "captive" && asset.m_bPickedUp && !asset.m_bAttachedToCarrier && !asset.m_bDelivered && !asset.m_bDestroyed;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RunMissionCommand(pOwnerEntity, pUserEntity, "mission_captive_follow");
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Order captive to follow";
		return true;
	}
}

class HST_MissionCaptiveExtractAction : HST_MissionUserActionBase
{
	override bool CanShowForMissionAsset(IEntity owner)
	{
		HST_MissionAssetState asset = ResolveMissionAssetState(owner);
		HST_MissionAssetComponent projection = HST_MissionCaptiveActionPolicy.ResolveProjection(owner);
		if (HST_MissionCaptiveActionPolicy.IsProjectionPending(projection))
			return false;
		if (HST_MissionCaptiveActionPolicy.IsRescueClaimant(asset, projection))
			return HST_MissionCaptiveActionPolicy.IsExactRescueAsset(asset, projection)
				&& (!asset || asset.m_sKind == "captive")
				&& HST_MissionCaptiveActionPolicy.CanExtract(asset, projection);
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
