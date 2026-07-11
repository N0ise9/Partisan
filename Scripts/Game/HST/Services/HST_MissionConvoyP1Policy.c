// Small fail-closed policy surface kept separate from the large convoy runtime
// and save-data classes so Enforce can compile those classes reliably.
class HST_MissionConvoyP1Policy
{
	static string ValidateAdmissionCargo(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "exact mission convoy cargo admission context is missing";

		HST_MissionAssetState cargo;
		int cargoRowCount;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId
				|| asset.m_sRole == HST_MissionConvoyOperationService.VEHICLE_ROLE)
				continue;
			cargo = asset;
			cargoRowCount++;
		}
		return ValidateCargoContract(mission, cargo, cargoRowCount);
	}

	static string ValidateCargoContract(
		HST_ActiveMissionState mission,
		HST_MissionAssetState cargo,
		int cargoRowCount)
	{
		if (!mission)
			return "exact mission convoy cargo contract mission is missing";
		if (mission.m_sRuntimeType != HST_MissionConvoyOperationService.EXACT_RUNTIME_TYPE)
			return "exact mission convoy runtime type is incompatible with frozen convoy authority";
		if (cargoRowCount > 1)
			return "exact mission convoy admission contains more than one optional cargo row";
		if (cargoRowCount < 0)
			return "exact mission convoy cargo row count is invalid";

		string expectedRole;
		string expectedKind;
		int expectedCount = ResolveExpectedCargoContract(mission.m_sMissionId, expectedRole, expectedKind);
		if (expectedCount < 0)
			return "exact mission convoy mission kind is unsupported by frozen convoy authority";
		if (expectedCount == 0)
		{
			if (cargoRowCount != 0)
				return "exact mission convoy mission kind does not permit a cargo row";
			return "";
		}
		if (cargoRowCount != 1 || !cargo)
			return "exact mission convoy mission kind requires exactly one compatible cargo row";
		if (cargo.m_sAssetId.IsEmpty() || cargo.m_sPrefab.IsEmpty())
			return "exact mission convoy cargo identity or prefab is empty";
		if (cargo.m_sRole != expectedRole || cargo.m_sKind != expectedKind)
			return "exact mission convoy cargo role or kind is incompatible with the mission";
		return ValidateCargoPrefabContract(cargo, expectedRole);
	}

	protected static int ResolveExpectedCargoContract(
		string missionId,
		out string expectedRole,
		out string expectedKind)
	{
		expectedRole = "";
		expectedKind = "";
		if (missionId == "convoy_money" || missionId == "convoy_supplies")
		{
			expectedRole = HST_MissionConvoyOperationService.PAYLOAD_ROLE;
			expectedKind = HST_MissionConvoyOperationService.CARGO_KIND;
			return 1;
		}
		if (missionId == "convoy_prisoners")
		{
			expectedRole = HST_MissionConvoyOperationService.CAPTIVE_ROLE;
			expectedKind = HST_MissionConvoyOperationService.CAPTIVE_KIND;
			return 1;
		}
		if (missionId == "convoy_ammo" || missionId == "convoy_armored"
			|| missionId == "convoy_reinforcements")
			return 0;
		return -1;
	}

	protected static string ValidateCargoPrefabContract(HST_MissionAssetState cargo, string expectedRole)
	{
		ResourceName cargoResourceName = cargo.m_sPrefab;
		Resource cargoResource = Resource.Load(cargoResourceName);
		IEntitySource cargoSource;
		if (cargoResource && cargoResource.IsValid())
			cargoSource = SCR_BaseContainerTools.FindEntitySource(cargoResource);
		if (!cargoSource)
			return "exact mission convoy cargo prefab is missing, invalid, or not an entity prefab";
		typename cargoType = cargoSource.GetClassName().ToType();
		if (!cargoType)
			return "exact mission convoy cargo prefab entity type is unavailable";
		if (!SCR_BaseContainerTools.FindComponentSource(cargoSource, HST_MissionAssetComponent))
			return "exact mission convoy cargo prefab lacks mission-asset runtime capability";

		bool characterPrefab = cargoType.IsInherited(SCR_ChimeraCharacter);
		if (expectedRole == HST_MissionConvoyOperationService.CAPTIVE_ROLE)
		{
			if (!characterPrefab
				|| !SCR_BaseContainerTools.FindComponentSource(cargoSource, SCR_CompartmentAccessComponent))
				return "exact mission convoy captive prefab is not a boardable character with compartment access";
			return "";
		}
		if (characterPrefab)
			return "exact mission convoy payload prefab must be a non-character mission-asset entity";
		return "";
	}

	static bool IsOpenDutyPair(
		HST_EOperationDutyState dutyState,
		HST_EOperationDutyState resumeDutyState)
	{
		if (dutyState != resumeDutyState)
			return false;
		return dutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING
			|| dutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			|| dutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
	}

	static bool IsProjectionPair(
		HST_EOperationSettlementState settlementState,
		HST_EOperationMaterializationState materializationState,
		HST_EOperationPositionAuthority positionAuthority)
	{
		if (settlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			if (positionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
				return materializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
					|| materializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
			if (positionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
				return materializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
					|| materializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
			return false;
		}
		if (settlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (materializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRING)
				return positionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
			if (materializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
				return positionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		}
		return false;
	}
}
