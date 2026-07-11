// Shared fail-closed boundary for the only manifest whose asset rows are not
// executed by HST_ForceSpawnAdapterService. Queue admission/replay validates
// the frozen shape; adapter execution additionally validates the reciprocal
// campaign operation/mission graph before it ignores those asset descriptors.
class HST_RescuePOWExternalAssetPolicy
{
	static const string MANIFEST_ID_PREFIX = "manifest_mission_rescue_";

	static string ValidateManifestShape(HST_ForceManifestState manifest)
	{
		if (!manifest)
			return "exact rescue external-asset manifest missing";
		if (!manifest.m_aGroups || !manifest.m_aMembers || !manifest.m_aVehicles || !manifest.m_aAssets)
			return "exact rescue external-asset slot collections missing";
		if (!manifest.m_bFrozen)
			return "exact rescue external-asset manifest is not frozen";
		if (manifest.m_sForceKind != HST_RescuePOWOperationService.EXACT_FORCE_KIND
			|| manifest.m_sPolicyId != HST_RescuePOWOperationService.EXACT_POLICY_ID
			|| manifest.m_sIntentId != HST_RescuePOWOperationService.EXACT_INTENT_ID)
			return "exact rescue external-asset policy, intent, or force kind conflicts";

		string missionInstanceId = ResolveMissionInstanceId(manifest);
		if (missionInstanceId.IsEmpty())
			return "exact rescue external-asset manifest identity is not mission-derived";
		if (manifest.m_sOperationId != HST_RescuePOWOperationService.BuildOperationId(missionInstanceId))
			return "exact rescue external-asset operation identity is not mission-derived";
		if (manifest.m_sManifestHash.IsEmpty())
			return "exact rescue external-asset manifest hash missing";
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		if (integrity.BuildManifestHash(manifest) != manifest.m_sManifestHash)
			return "exact rescue external-asset manifest hash conflict";

		if (manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
			return "exact rescue external-asset manifest requires one guard root";
		if (manifest.m_aVehicles.Count() != 0 || manifest.m_iRequestedVehicleCount != 0
			|| manifest.m_iAcceptedVehicleCount != 0)
			return "exact rescue external-asset manifest cannot contain vehicles";
		if (manifest.m_iAcceptedMemberCount <= 0
			|| manifest.m_iRequestedMemberCount != manifest.m_iAcceptedMemberCount
			|| manifest.m_aMembers.Count() != manifest.m_iAcceptedMemberCount)
			return "exact rescue external-asset guard roster count conflicts";

		HST_ForceManifestGroupState root = manifest.m_aGroups[0];
		if (root.m_sElementId != HST_RescuePOWOperationService.BuildGroupRootId(missionInstanceId)
			|| root.m_iOrdinal != 0 || !root.m_bRequired
			|| root.m_iExpectedMemberCount != manifest.m_iAcceptedMemberCount)
			return "exact rescue external-asset guard root shape conflicts";
		if (root.m_sCatalogEntryId.IsEmpty() || root.m_sPrefab.IsEmpty() || root.m_sRole.IsEmpty()
			|| manifest.m_sGroupPrefab != root.m_sPrefab)
			return "exact rescue external-asset guard root catalog projection is incomplete";
		for (int memberIndex = 0; memberIndex < manifest.m_aMembers.Count(); memberIndex++)
		{
			HST_ForceManifestMemberState member = manifest.m_aMembers[memberIndex];
			if (!member || member.m_sSlotId != HST_RescuePOWOperationService.BuildMemberSlotId(missionInstanceId, memberIndex)
				|| member.m_sGroupElementId != root.m_sElementId || member.m_iOrdinal != memberIndex
				|| member.m_sPrefab.IsEmpty() || !member.m_sAssignedVehicleSlotId.IsEmpty())
				return "exact rescue external-asset guard member shape conflicts";
		}

		return ValidateCaptiveAssetSlots(manifest, missionInstanceId);
	}

	static string ValidateAdapterAuthorityGraph(
		HST_CampaignState state,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		string shapeFailure = ValidateManifestShape(manifest);
		if (!shapeFailure.IsEmpty())
			return shapeFailure;
		if (!state || !batch || !group)
			return "exact rescue external-asset adapter graph is incomplete";

		string missionInstanceId = ResolveMissionInstanceId(manifest);
		if (!batch.m_bExternalAssetAuthority
			|| batch.m_sResultId != HST_RescuePOWOperationService.BuildSpawnResultId(missionInstanceId)
			|| batch.m_sRequestId != HST_RescuePOWOperationService.BuildSpawnRequestId(missionInstanceId)
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| batch.m_sManifestHash != manifest.m_sManifestHash
			|| batch.m_sOperationId != manifest.m_sOperationId
			|| batch.m_sForceId != HST_RescuePOWOperationService.BuildForceId(missionInstanceId)
			|| batch.m_sProjectionId != HST_RescuePOWOperationService.BuildProjectionId(missionInstanceId))
			return "exact rescue external-asset batch backlinks conflict";

		HST_OperationRecordState operation = state.FindOperation(manifest.m_sOperationId);
		HST_ActiveMissionState mission = state.FindActiveMission(missionInstanceId);
		if (!operation || !mission
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
			|| operation.m_iContractVersion != HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION
			|| operation.m_iProjectionContractVersion != HST_RescuePOWOperationService.EXACT_PROJECTION_CONTRACT_VERSION
			|| !HST_RescuePOWOperationService.IsExactMission(mission)
			|| mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return "exact rescue external-asset operation or mission contract conflicts";
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| !operation.m_sSettlementId.IsEmpty() || !mission.m_sSettlementId.IsEmpty())
			return "exact rescue external-asset operation or mission is not open";

		if (operation.m_sOperationId != HST_RescuePOWOperationService.BuildOperationId(missionInstanceId)
			|| operation.m_sMissionInstanceId != mission.m_sInstanceId
			|| operation.m_sManifestId != manifest.m_sManifestId
			|| operation.m_sSpawnResultId != batch.m_sResultId
			|| operation.m_sForceId != batch.m_sForceId
			|| operation.m_sProjectionId != batch.m_sProjectionId
			|| operation.m_sGroupId != group.m_sGroupId
			|| mission.m_sOperationId != operation.m_sOperationId
			|| mission.m_sManifestId != manifest.m_sManifestId
			|| mission.m_sSpawnResultId != batch.m_sResultId)
			return "exact rescue external-asset operation/mission relationship is not reciprocal";

		if (group.m_sGroupId != HST_RescuePOWOperationService.BuildProjectionId(missionInstanceId)
			|| group.m_sOperationId != operation.m_sOperationId
			|| group.m_sManifestId != manifest.m_sManifestId
			|| group.m_sSpawnResultId != batch.m_sResultId
			|| group.m_sForceId != batch.m_sForceId
			|| group.m_sProjectionId != batch.m_sProjectionId
			|| group.m_sMissionInstanceId != mission.m_sInstanceId)
			return "exact rescue external-asset guard relationship is not reciprocal";
		if (group.m_sSpawnFallbackMode != HST_RescuePOWOperationService.EXACT_GROUP_MODE)
			return "exact rescue external-asset guard execution mode conflicts";

		string captiveAuthorityFailure = ValidateCaptiveAuthorityRows(
			state,
			mission,
			operation,
			manifest);
		if (!captiveAuthorityFailure.IsEmpty())
			return captiveAuthorityFailure;
		if (!HasUniqueAuthorityRows(state, mission, operation, manifest, batch, group))
			return "exact rescue external-asset adapter graph is missing or ambiguous";
		return "";
	}

	static string ResolveMissionInstanceId(HST_ForceManifestState manifest)
	{
		if (!manifest || !manifest.m_sManifestId.StartsWith(MANIFEST_ID_PREFIX)
			|| manifest.m_sManifestId.Length() <= MANIFEST_ID_PREFIX.Length())
			return "";
		string missionInstanceId = manifest.m_sManifestId.Substring(
			MANIFEST_ID_PREFIX.Length(),
			manifest.m_sManifestId.Length() - MANIFEST_ID_PREFIX.Length());
		if (manifest.m_sManifestId != HST_RescuePOWOperationService.BuildManifestId(missionInstanceId))
			return "";
		return missionInstanceId;
	}

	protected static string ValidateCaptiveAssetSlots(
		HST_ForceManifestState manifest,
		string missionInstanceId)
	{
		if (manifest.m_aAssets.Count() != HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT)
			return "exact rescue external-asset manifest requires exactly three captive slots";
		for (int ordinal = 0; ordinal < HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT; ordinal++)
		{
			HST_ForceManifestAssetState asset = manifest.m_aAssets[ordinal];
			if (!asset || asset.m_sSlotId != HST_RescuePOWOperationService.BuildCaptiveSlotId(missionInstanceId, ordinal))
				return "foreign exact rescue asset-slot identity rejected";
			if (asset.m_sKind != HST_RescuePOWOperationService.CAPTIVE_KIND
				|| asset.m_sRole != HST_RescuePOWOperationService.CAPTIVE_ROLE
				|| asset.m_sPrefab != HST_RescuePOWOperationService.CAPTIVE_PREFAB)
				return "foreign exact rescue asset-slot semantics rejected";
			if (asset.m_iOrdinal != ordinal || asset.m_iQuantity != 1 || !asset.m_bRequired
				|| !asset.m_sAssignedVehicleSlotId.IsEmpty())
				return "foreign exact rescue asset-slot cardinality rejected";
		}
		return "";
	}

	protected static string ValidateCaptiveAuthorityRows(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		int linkedCount;
		array<int> ordinals = {};
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset)
				continue;
			bool claimsAuthority = asset.m_sMissionInstanceId == mission.m_sInstanceId
				|| asset.m_sOperationId == operation.m_sOperationId
				|| asset.m_sManifestId == manifest.m_sManifestId;
			for (int expectedOrdinal = 0; expectedOrdinal < HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT; expectedOrdinal++)
			{
				if (asset.m_sAssetId == HST_RescuePOWOperationService.BuildCaptiveAssetId(
					mission.m_sInstanceId, expectedOrdinal)
					|| asset.m_sManifestSlotId == HST_RescuePOWOperationService.BuildCaptiveSlotId(
						mission.m_sInstanceId, expectedOrdinal)
					|| asset.m_sRescueProjectionId == HST_RescuePOWOperationService.BuildCaptiveProjectionId(
						mission.m_sInstanceId, expectedOrdinal))
					claimsAuthority = true;
			}
			if (!claimsAuthority)
				continue;
			linkedCount++;
			if (!HST_RescuePOWOperationService.IsExactRescueCaptiveAsset(state, asset)
				|| !asset.m_sAssignedVehicleSlotId.IsEmpty() || !asset.m_sConvoyElementId.IsEmpty()
				|| ordinals.Contains(asset.m_iRescueOrdinal))
				return "exact rescue external-asset durable captive authority is foreign or ambiguous";
			ordinals.Insert(asset.m_iRescueOrdinal);
		}

		if (linkedCount != HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT
			|| ordinals.Count() != HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT)
			return "exact rescue external-asset durable captive authority requires exactly three rows";
		for (int ordinal = 0; ordinal < HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT; ordinal++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(
				HST_RescuePOWOperationService.BuildCaptiveAssetId(mission.m_sInstanceId, ordinal));
			if (!asset || asset.m_iRescueOrdinal != ordinal
				|| asset.m_sManifestSlotId != HST_RescuePOWOperationService.BuildCaptiveSlotId(
					mission.m_sInstanceId, ordinal)
				|| !asset.m_sAssignedVehicleSlotId.IsEmpty() || !asset.m_sConvoyElementId.IsEmpty()
				|| !HST_RescuePOWOperationService.IsExactRescueCaptiveAsset(state, asset))
				return "exact rescue external-asset durable captive row is missing or malformed";
		}
		return "";
	}

	protected static bool HasUniqueAuthorityRows(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		int missionCount;
		int operationCount;
		int manifestCount;
		int batchCount;
		int groupCount;
		foreach (HST_ActiveMissionState candidateMission : state.m_aActiveMissions)
		{
			if (candidateMission && (candidateMission.m_sInstanceId == mission.m_sInstanceId
				|| candidateMission.m_sOperationId == operation.m_sOperationId
				|| candidateMission.m_sManifestId == manifest.m_sManifestId
				|| candidateMission.m_sSpawnResultId == batch.m_sResultId))
				missionCount++;
		}
		foreach (HST_OperationRecordState candidateOperation : state.m_aOperations)
		{
			if (candidateOperation && (candidateOperation.m_sOperationId == operation.m_sOperationId
				|| candidateOperation.m_sMissionInstanceId == mission.m_sInstanceId
				|| candidateOperation.m_sManifestId == manifest.m_sManifestId
				|| candidateOperation.m_sSpawnResultId == batch.m_sResultId
				|| candidateOperation.m_sForceId == batch.m_sForceId
				|| candidateOperation.m_sProjectionId == batch.m_sProjectionId
				|| candidateOperation.m_sGroupId == group.m_sGroupId))
				operationCount++;
		}
		foreach (HST_ForceManifestState candidateManifest : state.m_aForceManifests)
		{
			if (candidateManifest && (candidateManifest.m_sManifestId == manifest.m_sManifestId
				|| candidateManifest.m_sOperationId == operation.m_sOperationId))
				manifestCount++;
		}
		foreach (HST_ForceSpawnResultState candidateBatch : state.m_aForceSpawnResults)
		{
			if (candidateBatch && (candidateBatch.m_sResultId == batch.m_sResultId
				|| candidateBatch.m_sManifestId == manifest.m_sManifestId
				|| candidateBatch.m_sOperationId == operation.m_sOperationId
				|| candidateBatch.m_sForceId == batch.m_sForceId
				|| candidateBatch.m_sProjectionId == batch.m_sProjectionId))
				batchCount++;
		}
		foreach (HST_ActiveGroupState candidateGroup : state.m_aActiveGroups)
		{
			if (candidateGroup && (candidateGroup.m_sGroupId == group.m_sGroupId
				|| candidateGroup.m_sOperationId == operation.m_sOperationId
				|| candidateGroup.m_sManifestId == manifest.m_sManifestId
				|| candidateGroup.m_sSpawnResultId == batch.m_sResultId
				|| candidateGroup.m_sForceId == batch.m_sForceId
				|| candidateGroup.m_sProjectionId == batch.m_sProjectionId
				|| candidateGroup.m_sMissionInstanceId == mission.m_sInstanceId))
				groupCount++;
		}
		return missionCount == 1 && operationCount == 1 && manifestCount == 1
			&& batchCount == 1 && groupCount == 1
			&& state.FindActiveMission(mission.m_sInstanceId) == mission
			&& state.FindOperation(operation.m_sOperationId) == operation
			&& state.FindForceManifest(manifest.m_sManifestId) == manifest
			&& state.FindForceSpawnResult(batch.m_sResultId) == batch
			&& state.FindActiveGroup(group.m_sGroupId) == group;
	}
}
