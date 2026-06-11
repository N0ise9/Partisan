class HST_MissionCaptiveFreeAction : HST_MissionUserActionBase
{
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

class HST_MissionCaptiveExtractAction : HST_MissionUserActionBase
{
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
