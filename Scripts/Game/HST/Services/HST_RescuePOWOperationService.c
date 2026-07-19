class HST_RescuePOWAdmissionResult
{
	bool m_bSuccess;
	bool m_bAlreadyApplied;
	bool m_bStateChanged;
	string m_sFailureReason;
	ref HST_ActiveMissionState m_Mission;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceManifestState m_Manifest;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_GuardGroup;
	ref array<ref HST_MissionAssetState> m_aCaptives = {};
}

class HST_RescuePOWTransitionResult
{
	bool m_bSuccess;
	bool m_bAlreadyApplied;
	bool m_bStateChanged;
	string m_sFailureReason;
	string m_sResult;
	ref HST_MissionAssetState m_Captive;
}

class HST_RescuePOWAdmissionPlan
{
	ref HST_ForceCompositionResult m_Composition;
	ref HST_ForceGroupCatalogEntry m_CatalogGroup;
	ref HST_ForceManifestState m_Manifest;
	ref HST_OperationRecordState m_Operation;
	ref HST_ActiveGroupState m_GuardGroup;
	ref array<ref HST_MissionAssetState> m_aCaptives = {};
	vector m_vGuardPosition;
	vector m_vExtractionPosition;
}

// A controlled-shutdown rescue fence pins both the prepared durable DTO graph
// and the exact native captive/carrier topology that produced it. The normal
// persistence preparation remains the sole live sampler; after these pins are
// published, retries maintain the same stopped follower, frozen mission carrier,
// seat identity, state objects, scope, and values without resampling authority.
class HST_ControlledShutdownRescueCaptiveSamplePin
{
	ref HST_MissionAssetState m_Captive;
	ref HST_MissionRuntimeEntityState m_RuntimeProjection;
	ref HST_RuntimeVehicleState m_CarrierRecord;
	IEntity m_CaptiveEntity;
	IEntity m_CarrierEntity;
	BaseCompartmentSlot m_CaptiveSlot;
	HST_MissionCaptiveFollowComponent m_FollowComponent;
	string m_sAssetId;
	string m_sProjectionId;
	string m_sCarrierId;
	string m_sObservedSeatToken;
	string m_sCaptiveSample;
	string m_sRuntimeProjectionSample;
	string m_sCarrierSample;
	vector m_vCaptivePosition;
	vector m_vCaptiveAngles;
}

class HST_ControlledShutdownRescueCarrierPin
{
	IEntity m_Entity;
	ref HST_RuntimeVehicleState m_Record;
	string m_sRuntimeId;
	vector m_aTransform[4];
	ref array<BaseCompartmentSlot> m_aCompartmentSlots = {};
	ref array<IEntity> m_aCompartmentOccupants = {};
}

class HST_ControlledShutdownRescueOperationSamplePin
{
	ref HST_OperationRecordState m_Operation;
	ref HST_ActiveMissionState m_Mission;
	ref HST_ActiveGroupState m_GuardGroup;
	string m_sOperationId;
	string m_sMissionId;
	string m_sOperationSample;
	string m_sMissionSample;
	string m_sGuardSample;
	ref array<ref HST_ControlledShutdownRescueCaptiveSamplePin> m_aCaptives = {};
	ref array<ref HST_MissionObjectiveState> m_aObjectives = {};
	ref array<string> m_aObjectiveSamples = {};
}

// Schema-58 authority for newly started rescue_pows missions only. One
// operation owns one frozen composite manifest: a catalog-backed exact guard
// roster plus three externally projected captive slots. The generic force
// adapter owns only the guard projection; the mission runtime is an actuator
// for captive IEntity state and never owns durable captive transitions.
class HST_RescuePOWOperationService
{
	static const int EXACT_CONTRACT_VERSION = 1;
	static const int QUARANTINED_CONTRACT_VERSION = -58;
	static const int EXACT_PROJECTION_CONTRACT_VERSION = 1;
	static const int EXACT_CAPTIVE_COUNT = 3;
	static const int REQUIRED_CAPTIVE_COUNT = 3;
	static const int EXTRACTION_GRACE_SECONDS = 300;
	static const string RESCUE_GRACE_PHASE = "rescue_extraction_grace";
	static const string EXACT_MISSION_ID = "rescue_pows";
	static const string EXACT_FORCE_KIND = "mission_rescue";
	static const string EXACT_POLICY_ID = "exact_rescue_pows_v1";
	static const string EXACT_INTENT_ID = "rescue_pows_guard";
	static const string EXACT_GROUP_MODE = "exact_rescue_pow_guard";
	static const string ASSIGNMENT_KIND = "guard_rescue_pows";
	static const string RECALL_POLICY_ID = "no_recall";
	static const string SETTLEMENT_POLICY_ID = "mission_owned_no_refund";
	static const string SETTLEMENT_KIND = "exact_rescue_pow_terminal";
	static const string QUARANTINE_STATUS = "exact_rescue_pow_quarantined";
	static const string GUARD_VIRTUAL_STATUS = "rescue_pow_guard_virtual";
	static const string GUARD_PHYSICAL_STATUS = "rescue_pow_guard_physical";
	static const string GUARD_FOLD_PENDING_STATUS = "rescue_pow_guard_fold_pending";
	static const string GUARD_ELIMINATED_STATUS = "rescue_pow_guard_eliminated";
	static const string CAPTIVE_KIND = "captive";
	static const string CAPTIVE_ROLE = "captive";
	static const string CAPTIVE_PREFAB = "{6985327711303730}Prefabs/Objects/HST/HST_MissionProp_Captives.et";
	static const int EXACT_PRIORITY = 70;
	static const int EXACT_MAX_RETRIES = 3;
	static const int DEPLOYMENT_GRACE_SECONDS = 180;
	static const int CONTACT_CLEAR_SECONDS = 30;
	static const int CARRIER_EVIDENCE_GRACE_SECONDS = 10;
	static const int MAX_CAPTIVE_COMMAND_RECEIPTS = 32;
	static const int MAX_REJECTED_CAPTIVE_COMMAND_RECEIPTS = 8;
	static const float CONTACT_EVIDENCE_RADIUS_METERS = 140.0;

	protected ref HST_ForcePlanningIntegrityService m_Integrity = new HST_ForcePlanningIntegrityService();
	protected ref HST_ForceCatalogService m_Catalog = new HST_ForceCatalogService();
	protected ref HST_MaterializationService m_Materialization = new HST_MaterializationService();
	protected ref HST_ForceSpawnQueueService m_SpawnQueue;
	protected ref HST_ForceSpawnAdapterService m_SpawnAdapter;
	protected ref HST_PhysicalWarService m_PhysicalWar;
	protected ref HST_MissionRuntimeService m_MissionRuntime;
	protected ref HST_CampaignState m_ControlledShutdownPersistenceState;
	protected ref array<ref HST_ControlledShutdownRescueOperationSamplePin>
		m_aControlledShutdownPersistencePins = {};
	protected ref array<ref HST_ControlledShutdownRescueCarrierPin>
		m_aControlledShutdownCarrierPins = {};
	protected bool m_bControlledShutdownPersistenceSampleApplied;
	protected bool m_bControlledShutdownPersistenceSampleExact;

	void SetRuntimeServices(
		HST_ForceSpawnQueueService spawnQueue,
		HST_ForceSpawnAdapterService spawnAdapter,
		HST_PhysicalWarService physicalWar,
		HST_MissionRuntimeService missionRuntime)
	{
		m_SpawnQueue = spawnQueue;
		m_SpawnAdapter = spawnAdapter;
		m_PhysicalWar = physicalWar;
		m_MissionRuntime = missionRuntime;
	}

	void SetMissionRuntimeService(HST_MissionRuntimeService missionRuntime)
	{
		m_MissionRuntime = missionRuntime;
	}

	static bool IsExactMission(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sMissionId == EXACT_MISSION_ID
			&& mission.m_iOperationContractVersion == EXACT_CONTRACT_VERSION;
	}

	static bool IsQuarantinedMission(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sMissionId == EXACT_MISSION_ID
			&& mission.m_iOperationContractVersion == QUARANTINED_CONTRACT_VERSION;
	}

	static bool IsExactOrQuarantinedMission(HST_ActiveMissionState mission)
	{
		return IsExactMission(mission) || IsQuarantinedMission(mission);
	}

	static bool HasOpenFrozenHQExtractionAuthority(HST_CampaignState state)
	{
		if (!state)
			return false;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!IsExactMission(mission) || !mission.m_sSettlementId.IsEmpty())
				continue;
			HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
			if (operation
				&& operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
				&& operation.m_iContractVersion == EXACT_CONTRACT_VERSION
				&& operation.m_sMissionInstanceId == mission.m_sInstanceId
				&& operation.m_eSettlementState
					== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				&& operation.m_sSettlementId.IsEmpty())
				return true;
		}
		return false;
	}

	static bool CanCompleteExactMission(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !IsExactMission(mission)
			|| mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE
			|| !mission.m_sSettlementId.IsEmpty())
			return false;
		HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
		if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| operation.m_sMissionInstanceId != mission.m_sInstanceId
			|| operation.m_sManifestId != mission.m_sManifestId
			|| operation.m_sSpawnResultId != mission.m_sSpawnResultId
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| !operation.m_sSettlementId.IsEmpty())
			return false;
		HST_ActiveGroupState group = state.FindActiveGroup(operation.m_sGroupId);
		return IsExactRescueGroup(state, group)
			&& AreAllRequiredCaptivesExtracted(state, mission);
	}

	static bool IsSupportedExactMissionId(string missionId)
	{
		return missionId == EXACT_MISSION_ID;
	}

	static bool IsSupportedExactContractVersion(int contractVersion)
	{
		return contractVersion == EXACT_CONTRACT_VERSION;
	}

	static bool IsQuarantinedOperationContractVersion(int contractVersion)
	{
		return contractVersion == QUARANTINED_CONTRACT_VERSION;
	}

	static int ResolveExpectedContractVersion(string missionId)
	{
		if (missionId == EXACT_MISSION_ID)
			return EXACT_CONTRACT_VERSION;
		return 0;
	}

	static int ResolveQuarantinedContractVersion(string missionId)
	{
		if (missionId == EXACT_MISSION_ID)
			return QUARANTINED_CONTRACT_VERSION;
		return 0;
	}

	static string ResolveExpectedPolicyId(string missionId)
	{
		if (missionId == EXACT_MISSION_ID)
			return EXACT_POLICY_ID;
		return "";
	}

	static string ResolveExpectedIntentId(string missionId)
	{
		if (missionId == EXACT_MISSION_ID)
			return EXACT_INTENT_ID;
		return "";
	}

	static string BuildOperationId(string missionInstanceId)
	{
		return HST_StableIdService.BuildOperationId("mission_rescue", missionInstanceId);
	}

	static string BuildManifestId(string missionInstanceId)
	{
		if (missionInstanceId.IsEmpty())
			return "";
		return "manifest_mission_rescue_" + missionInstanceId;
	}

	static string BuildSpawnResultId(string missionInstanceId)
	{
		if (missionInstanceId.IsEmpty())
			return "";
		return "spawn_mission_rescue_" + missionInstanceId;
	}

	static string BuildSpawnRequestId(string missionInstanceId)
	{
		if (missionInstanceId.IsEmpty())
			return "";
		return "mission_rescue_" + missionInstanceId;
	}

	static string BuildForceId(string missionInstanceId)
	{
		string operationId = BuildOperationId(missionInstanceId);
		if (operationId.IsEmpty())
			return "";
		return "force_" + operationId;
	}

	static string BuildProjectionId(string missionInstanceId)
	{
		string operationId = BuildOperationId(missionInstanceId);
		if (operationId.IsEmpty())
			return "";
		return "projection_" + operationId;
	}

	static string BuildGroupRootId(string missionInstanceId)
	{
		if (missionInstanceId.IsEmpty())
			return "";
		return "mission_rescue_guard_group_" + missionInstanceId;
	}

	static string BuildMemberSlotId(string missionInstanceId, int ordinal)
	{
		if (missionInstanceId.IsEmpty() || ordinal < 0)
			return "";
		return string.Format("mission_rescue_guard_member_%1_%2", missionInstanceId, ordinal + 1);
	}

	static string BuildCaptiveSlotId(string missionInstanceId, int ordinal)
	{
		if (missionInstanceId.IsEmpty() || ordinal < 0 || ordinal >= EXACT_CAPTIVE_COUNT)
			return "";
		return string.Format("mission_rescue_captive_slot_%1_%2", missionInstanceId, ordinal + 1);
	}

	static string BuildCaptiveAssetId(string missionInstanceId, int ordinal)
	{
		if (missionInstanceId.IsEmpty() || ordinal < 0 || ordinal >= EXACT_CAPTIVE_COUNT)
			return "";
		return string.Format("mission_rescue_captive_%1_%2", missionInstanceId, ordinal + 1);
	}

	static string BuildCaptiveProjectionId(string missionInstanceId, int ordinal)
	{
		if (missionInstanceId.IsEmpty() || ordinal < 0 || ordinal >= EXACT_CAPTIVE_COUNT)
			return "";
		return string.Format("projection_mission_rescue_captive_%1_%2", missionInstanceId, ordinal + 1);
	}

	static string BuildCaptiveCasualtyReceiptId(string missionInstanceId, string assetId)
	{
		if (missionInstanceId.IsEmpty() || assetId.IsEmpty())
			return "";
		return "rescue_casualty_" + missionInstanceId + "_" + assetId;
	}

	static string BuildCaptiveExtractionReceiptId(string missionInstanceId, string assetId)
	{
		if (missionInstanceId.IsEmpty() || assetId.IsEmpty())
			return "";
		return "rescue_extraction_" + missionInstanceId + "_" + assetId;
	}

	static bool IsRescueGroupClaimant(HST_CampaignState state, HST_ActiveGroupState group)
	{
		if (!state || !group)
			return false;
		HST_OperationRecordState operation = state.FindOperation(group.m_sOperationId);
		if (operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE)
		{
			if (operation.m_sGroupId == group.m_sGroupId
				|| operation.m_sProjectionId == group.m_sProjectionId
				|| operation.m_sManifestId == group.m_sManifestId)
				return true;
		}
		HST_ActiveMissionState mission = state.FindActiveMission(group.m_sMissionInstanceId);
		if (IsExactOrQuarantinedMission(mission) && mission.m_sOperationId == group.m_sOperationId)
			return true;
		return group.m_sSpawnFallbackMode == EXACT_GROUP_MODE
			|| group.m_sSpawnFallbackMode == QUARANTINE_STATUS
			|| group.m_sRuntimeStatus.StartsWith("rescue_pow_guard_");
	}

	static bool IsExactRescueCaptiveAsset(HST_CampaignState state, HST_MissionAssetState asset)
	{
		if (!state || !asset || asset.m_iRescueContractVersion != EXACT_CONTRACT_VERSION
			|| asset.m_iRescueOrdinal < 0 || asset.m_iRescueOrdinal >= EXACT_CAPTIVE_COUNT)
			return false;
		HST_ActiveMissionState mission = state.FindActiveMission(asset.m_sMissionInstanceId);
		if (!IsExactMission(mission))
			return false;
		return asset.m_sAssetId == BuildCaptiveAssetId(mission.m_sInstanceId, asset.m_iRescueOrdinal)
			&& asset.m_sOperationId == mission.m_sOperationId
			&& asset.m_sManifestId == mission.m_sManifestId
			&& asset.m_sManifestSlotId == BuildCaptiveSlotId(mission.m_sInstanceId, asset.m_iRescueOrdinal)
			&& asset.m_sRescueProjectionId == BuildCaptiveProjectionId(mission.m_sInstanceId, asset.m_iRescueOrdinal)
			&& asset.m_sKind == CAPTIVE_KIND && asset.m_sRole == CAPTIVE_ROLE
			&& asset.m_sPrefab == CAPTIVE_PREFAB;
	}

	static bool IsExactRescueGroup(HST_CampaignState state, HST_ActiveGroupState group)
	{
		if (!IsRescueGroupClaimant(state, group))
			return false;
		HST_ActiveMissionState mission = state.FindActiveMission(group.m_sMissionInstanceId);
		HST_OperationRecordState operation = state.FindOperation(group.m_sOperationId);
		if (!IsExactMission(mission) || !operation
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| operation.m_iProjectionContractVersion != EXACT_PROJECTION_CONTRACT_VERSION)
			return false;
		HST_ForceManifestState manifest = state.FindForceManifest(group.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(group.m_sSpawnResultId);
		if (!manifest || !batch || !manifest.m_bFrozen || !batch.m_bExternalAssetAuthority
			|| manifest.m_sPolicyId != EXACT_POLICY_ID || manifest.m_sForceKind != EXACT_FORCE_KIND
			|| manifest.m_sIntentId != EXACT_INTENT_ID || manifest.m_aGroups.Count() != 1
			|| manifest.m_aAssets.Count() != EXACT_CAPTIVE_COUNT)
			return false;
		return mission.m_sOperationId == operation.m_sOperationId
			&& mission.m_sManifestId == manifest.m_sManifestId
			&& mission.m_sSpawnResultId == batch.m_sResultId
			&& operation.m_sManifestId == manifest.m_sManifestId
			&& operation.m_sSpawnResultId == batch.m_sResultId
			&& operation.m_sGroupId == group.m_sGroupId
			&& operation.m_sProjectionId == group.m_sProjectionId
			&& group.m_sGroupId == BuildProjectionId(mission.m_sInstanceId)
			&& group.m_sSpawnFallbackMode == EXACT_GROUP_MODE;
	}

	static bool IsMissionRescueGroupClaimant(HST_CampaignState state, HST_ActiveGroupState group)
	{
		return IsRescueGroupClaimant(state, group);
	}

	static bool IsExactMissionRescueGroup(HST_CampaignState state, HST_ActiveGroupState group)
	{
		return IsExactRescueGroup(state, group);
	}

	bool PrepareNewMissionContract(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_sMissionId != EXACT_MISSION_ID)
			return false;
		if (IsExactMission(mission))
			return true;
		if (mission.m_iOperationContractVersion != 0 || !mission.m_sOperationId.IsEmpty()
			|| !mission.m_sManifestId.IsEmpty() || !mission.m_sSpawnResultId.IsEmpty()
			|| !mission.m_sSettlementId.IsEmpty())
			return false;
		mission.m_iOperationContractVersion = EXACT_CONTRACT_VERSION;
		mission.m_iRequiredCaptiveCount = EXACT_CAPTIVE_COUNT;
		mission.m_iExtractedCaptiveCount = 0;
		mission.m_bRescueExtractionGrace = false;
		mission.m_iRescueGraceUntilSecond = 0;
		return true;
	}

	HST_RescuePOWAdmissionResult CanAdmitNewMission(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_MissionDefinition definition,
		HST_ActiveMissionState mission,
		HST_MissionRuntimeService missionRuntime)
	{
		HST_RescuePOWAdmissionResult result = new HST_RescuePOWAdmissionResult();
		result.m_Mission = mission;
		if (!state || !preset || !definition || !mission || !missionRuntime || !m_SpawnQueue
			|| !m_SpawnAdapter || !m_PhysicalWar || !m_Integrity || !m_Catalog)
		{
			result.m_sFailureReason = "exact rescue admission services are unavailable";
			return result;
		}
		if (HasAnyAdmissionAuthority(state, mission))
			return ResolveCommittedAdmission(state, mission);
		HST_RescuePOWAdmissionPlan plan;
		string failure = BuildAdmissionPlan(state, preset, definition, mission, missionRuntime, plan);
		if (!failure.IsEmpty())
		{
			result.m_sFailureReason = failure;
			return result;
		}
		failure = FindAdmissionIdentityCollision(state, mission, plan);
		if (!failure.IsEmpty())
		{
			result.m_sFailureReason = failure;
			return result;
		}
		HST_ForceSpawnQueueEnqueueResult preflight = m_SpawnQueue.CanEnqueue(
			state.m_aForceSpawnResults,
			plan.m_Manifest,
			BuildSpawnRequest(state, mission),
			Math.Max(0, state.m_iElapsedSeconds));
		if (!preflight || !preflight.m_bSuccess || preflight.m_bAlreadyApplied)
		{
			result.m_sFailureReason = "exact rescue spawn admission preflight failed";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = result.m_sFailureReason + ": " + preflight.m_sFailureReason;
			return result;
		}
		result.m_bSuccess = true;
		result.m_Operation = plan.m_Operation;
		result.m_Manifest = plan.m_Manifest;
		result.m_GuardGroup = plan.m_GuardGroup;
		result.m_aCaptives = plan.m_aCaptives;
		return result;
	}

	HST_RescuePOWAdmissionResult AdmitNewMission(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_MissionDefinition definition,
		HST_ActiveMissionState mission,
		HST_MissionRuntimeService missionRuntime)
	{
		if (HasAnyAdmissionAuthority(state, mission))
			return ResolveCommittedAdmission(state, mission);

		HST_RescuePOWAdmissionResult result = new HST_RescuePOWAdmissionResult();
		result.m_Mission = mission;
		HST_RescuePOWAdmissionResult preflight = CanAdmitNewMission(
			state, preset, definition, mission, missionRuntime);
		if (!preflight || !preflight.m_bSuccess)
		{
			result.m_sFailureReason = "exact rescue admission preflight failed";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = preflight.m_sFailureReason;
			QuarantineUncommittedMission(mission, result.m_sFailureReason);
			result.m_bStateChanged = true;
			return result;
		}

		HST_RescuePOWAdmissionPlan plan;
		string failure = BuildAdmissionPlan(state, preset, definition, mission, missionRuntime, plan);
		if (!failure.IsEmpty() || !plan || !plan.m_Manifest || !plan.m_Operation
			|| !plan.m_GuardGroup || plan.m_aCaptives.Count() != EXACT_CAPTIVE_COUNT)
		{
			result.m_sFailureReason = failure;
			if (result.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = "exact rescue detached authority could not be built";
			QuarantineUncommittedMission(mission, result.m_sFailureReason);
			result.m_bStateChanged = true;
			return result;
		}

		mission.m_vRescueExtractionPosition = plan.m_vExtractionPosition;
		state.m_aForceManifests.Insert(plan.m_Manifest);
		state.m_aOperations.Insert(plan.m_Operation);
		state.m_aActiveGroups.Insert(plan.m_GuardGroup);
		foreach (HST_MissionAssetState captive : plan.m_aCaptives)
			state.m_aMissionAssets.Insert(captive);
		result.m_Manifest = plan.m_Manifest;
		result.m_Operation = plan.m_Operation;
		result.m_GuardGroup = plan.m_GuardGroup;
		result.m_aCaptives = plan.m_aCaptives;
		result.m_bStateChanged = true;

		HST_ForceSpawnQueueEnqueueResult enqueue = m_SpawnQueue.Enqueue(
			state.m_aForceSpawnResults,
			plan.m_Manifest,
			BuildSpawnRequest(state, mission),
			Math.Max(0, state.m_iElapsedSeconds));
		if (!enqueue || !enqueue.m_bSuccess || !enqueue.m_Batch || enqueue.m_bAlreadyApplied)
		{
			failure = "exact rescue spawn admission failed";
			if (enqueue && !enqueue.m_sFailureReason.IsEmpty())
				failure = failure + ": " + enqueue.m_sFailureReason;
			HST_ForceSpawnResultState failedBatch;
			if (enqueue)
				failedBatch = enqueue.m_Batch;
			RollbackAdmission(state, mission, plan, failedBatch, failure);
			result.m_sFailureReason = failure;
			return result;
		}
		result.m_Batch = enqueue.m_Batch;

		HST_ForceSpawnQueueCallbackResult held = m_SpawnQueue.HoldPendingProjectionForStrategicSimulation(
			state.m_aForceSpawnResults,
			plan.m_Manifest,
			enqueue.m_Batch.m_sResultId,
			enqueue.m_Batch.m_sProjectionId,
			Math.Max(0, state.m_iElapsedSeconds));
		if (!held || !held.m_bAccepted || !enqueue.m_Batch.m_bStrategicProjectionHeld)
		{
			failure = "exact rescue strategic hold failed";
			if (held && !held.m_sFailureReason.IsEmpty())
				failure = failure + ": " + held.m_sFailureReason;
			RollbackAdmission(state, mission, plan, enqueue.m_Batch, failure);
			result.m_sFailureReason = failure;
			return result;
		}

		mission.m_sOperationId = plan.m_Operation.m_sOperationId;
		mission.m_sManifestId = plan.m_Manifest.m_sManifestId;
		mission.m_sSpawnResultId = enqueue.m_Batch.m_sResultId;
		failure = ValidateCommittedGraph(state, mission, plan.m_Operation, plan.m_Manifest,
			enqueue.m_Batch, plan.m_GuardGroup, true);
		if (!failure.IsEmpty())
		{
			RollbackAdmission(state, mission, plan, enqueue.m_Batch,
				"exact rescue committed graph failed: " + failure);
			result.m_sFailureReason = "exact rescue committed graph failed: " + failure;
			return result;
		}
		result.m_bSuccess = true;
		return result;
	}

	HST_RescuePOWAdmissionResult ResolveCommittedAdmission(
		HST_CampaignState state,
		HST_ActiveMissionState mission)
	{
		HST_RescuePOWAdmissionResult result = new HST_RescuePOWAdmissionResult();
		result.m_Mission = mission;
		if (!state || !IsExactMission(mission))
		{
			result.m_sFailureReason = "exact rescue committed replay context is incomplete";
			return result;
		}
		HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(mission.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		HST_ActiveGroupState group;
		if (operation)
			group = state.FindActiveGroup(operation.m_sGroupId);
		string failure = ValidateCommittedGraph(state, mission, operation, manifest, batch, group, false);
		if (!failure.IsEmpty())
		{
			result.m_sFailureReason = failure;
			if (operation)
				QuarantineOperationAuthority(state, operation, failure);
			else
				QuarantineUncommittedMission(mission, failure);
			result.m_bStateChanged = true;
			return result;
		}
		result.m_bSuccess = true;
		result.m_bAlreadyApplied = true;
		result.m_Operation = operation;
		result.m_Manifest = manifest;
		result.m_Batch = batch;
		result.m_GuardGroup = group;
		CollectExactCaptives(state, mission, result.m_aCaptives);
		return result;
	}

	// Campaign Debug and other read-only diagnostics must recognize the exact
	// rescue aggregate without replaying admission or quarantining live authority.
	// Keep this wrapper on the production graph validator so diagnostic health
	// cannot drift from the committed operation/manifest/batch/group contract.
	bool ValidateCommittedGraphReadOnly(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		out string evidence,
		bool requireActiveMission = true)
	{
		evidence = "exact rescue committed graph is unavailable";
		if (!state || !mission)
			return false;

		HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(mission.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		HST_ActiveGroupState group;
		if (operation)
			group = state.FindActiveGroup(operation.m_sGroupId);
		string failure = ValidateCommittedGraph(
			state,
			mission,
			operation,
			manifest,
			batch,
			group,
			requireActiveMission);
		if (!failure.IsEmpty())
		{
			evidence = failure;
			return false;
		}

		evidence = "exact rescue committed graph is read-only exact";
		return true;
	}

	// Exact rescue intentionally leaves mission.m_bRuntimeSpawned false because
	// the aggregate, not generic MissionRuntime, owns the captive projections.
	// Runtime readiness therefore means three exact durable captive rows, three
	// matching live runtime DTO rows, and three unique native handles.
	bool InspectLiveCaptiveRuntimeReadOnly(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		out int captiveRows,
		out int liveRuntimeRows,
		out int liveHandles,
		out string evidence)
	{
		captiveRows = 0;
		liveRuntimeRows = 0;
		liveHandles = 0;
		evidence = "exact rescue live captive runtime is unavailable";
		if (!state || !mission || !m_MissionRuntime || !IsExactMission(mission))
			return false;

		bool captiveIdentitiesExact = true;
		array<string> projectionIds = {};
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (asset.m_sKind != CAPTIVE_KIND && asset.m_sRole != CAPTIVE_ROLE
				&& asset.m_iRescueContractVersion == 0)
				continue;

			captiveRows++;
			if (!IsExactRescueCaptiveAsset(state, asset)
				|| asset.m_sEntityId.IsEmpty()
				|| asset.m_sEntityId != asset.m_sRescueProjectionId
				|| projectionIds.Contains(asset.m_sEntityId))
			{
				captiveIdentitiesExact = false;
				continue;
			}
			projectionIds.Insert(asset.m_sEntityId);
			IEntity captiveEntity;
			string handleEvidence;
			if (asset.m_bAlive && !asset.m_bDestroyed && asset.m_bSpawned
				&& m_MissionRuntime.ResolveExactRescueCaptiveProjectionReadOnly(
					state,
					mission,
					asset,
					captiveEntity,
					handleEvidence))
				liveHandles++;
		}

		bool runtimeIdentitiesExact = true;
		array<string> runtimeProjectionIds = {};
		foreach (HST_MissionRuntimeEntityState runtimeEntity : state.m_aMissionRuntimeEntities)
		{
			if (!runtimeEntity
				|| runtimeEntity.m_sMissionInstanceId != mission.m_sInstanceId
				|| runtimeEntity.m_sKind != CAPTIVE_ROLE)
				continue;

			if (runtimeEntity.m_sRuntimeEntityId.IsEmpty()
				|| !projectionIds.Contains(runtimeEntity.m_sRuntimeEntityId)
				|| runtimeProjectionIds.Contains(runtimeEntity.m_sRuntimeEntityId))
			{
				runtimeIdentitiesExact = false;
				continue;
			}
			runtimeProjectionIds.Insert(runtimeEntity.m_sRuntimeEntityId);
			if (runtimeEntity.m_bSpawned && !runtimeEntity.m_bDestroyed)
				liveRuntimeRows++;
		}

		bool exact = captiveIdentitiesExact && runtimeIdentitiesExact
			&& captiveRows == EXACT_CAPTIVE_COUNT
			&& projectionIds.Count() == EXACT_CAPTIVE_COUNT
			&& runtimeProjectionIds.Count() == EXACT_CAPTIVE_COUNT
			&& liveRuntimeRows == EXACT_CAPTIVE_COUNT
			&& liveHandles == EXACT_CAPTIVE_COUNT;
		evidence = string.Format(
			"captive DTO rows %1/%2 | live runtime rows %3/%2 | live handles %4/%2 | identities exact %5/%6",
			captiveRows,
			EXACT_CAPTIVE_COUNT,
			liveRuntimeRows,
			liveHandles,
			captiveIdentitiesExact,
			runtimeIdentitiesExact);
		evidence = evidence + string.Format(
			" | unique captive/runtime identities %1/%2 of %3",
			projectionIds.Count(),
			runtimeProjectionIds.Count(),
			EXACT_CAPTIVE_COUNT);
		return exact;
	}

	bool RollbackAdmission(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_RescuePOWAdmissionPlan plan,
		HST_ForceSpawnResultState batch,
		string reason)
	{
		if (!state || !mission || !plan || !plan.m_Manifest)
			return false;
		if (batch && state.m_aForceSpawnResults.Find(batch) >= 0)
		{
			if (!IsTerminalSpawnBatch(batch) && m_SpawnQueue)
			{
				HST_ForceSpawnQueueCallbackResult cancelled = m_SpawnQueue.RequestCancel(
					state.m_aForceSpawnResults, batch.m_sResultId,
					Math.Max(0, state.m_iElapsedSeconds), reason);
				if (!cancelled || !cancelled.m_bAccepted || !IsTerminalSpawnBatch(batch))
				{
					if (plan.m_Operation)
						QuarantineOperationAuthority(state, plan.m_Operation, reason);
					QuarantineUncommittedMission(mission, reason);
					return false;
				}
			}
			state.m_aForceSpawnResults.Remove(state.m_aForceSpawnResults.Find(batch));
		}
		foreach (HST_MissionAssetState captive : plan.m_aCaptives)
		{
			int captiveIndex = state.m_aMissionAssets.Find(captive);
			if (captiveIndex >= 0)
				state.m_aMissionAssets.Remove(captiveIndex);
		}
		int groupIndex = state.m_aActiveGroups.Find(plan.m_GuardGroup);
		if (groupIndex >= 0)
			state.m_aActiveGroups.Remove(groupIndex);
		int operationIndex = state.m_aOperations.Find(plan.m_Operation);
		if (operationIndex >= 0)
			state.m_aOperations.Remove(operationIndex);
		int manifestIndex = state.m_aForceManifests.Find(plan.m_Manifest);
		if (manifestIndex >= 0)
			state.m_aForceManifests.Remove(manifestIndex);
		mission.m_sOperationId = "";
		mission.m_sManifestId = "";
		mission.m_sSpawnResultId = "";
		mission.m_sSettlementId = "";
		QuarantineUncommittedMission(mission, reason);
		return true;
	}

	protected string BuildAdmissionPlan(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_MissionDefinition definition,
		HST_ActiveMissionState mission,
		HST_MissionRuntimeService missionRuntime,
		out HST_RescuePOWAdmissionPlan plan)
	{
		plan = null;
		if (!state || !preset || !definition || !mission || !missionRuntime)
			return "exact rescue detached planning context is missing";
		if (!IsExactMission(mission) || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE
			|| definition.m_sMissionId != EXACT_MISSION_ID || mission.m_sInstanceId.IsEmpty()
			|| mission.m_sTargetZoneId.IsEmpty() || mission.m_iRequiredCaptiveCount != EXACT_CAPTIVE_COUNT)
			return "mission did not opt into a new active exact rescue contract";
		if (state.FindActiveMission(mission.m_sInstanceId) != mission || CountMissionIdentity(state, mission) != 1)
			return "exact rescue mission identity is ambiguous";
		HST_ZoneState zone = state.FindFrozenHistoricalZoneView(mission.m_sTargetZoneId);
		if (!zone || zone.m_sOwnerFactionKey.IsEmpty()
			|| !HST_FactionRelationService.IsEnemyFaction(preset, zone.m_sOwnerFactionKey))
			return "exact rescue target zone is unavailable or not enemy-owned";
		HST_GeneratedSiteState site = state.FindGeneratedSite(mission.m_sSiteId);
		if (!site || !site.m_bValid || site.m_sZoneId != mission.m_sTargetZoneId)
			return "exact rescue requires one valid generated site in its target zone";
		if (IsZeroVector(mission.m_vTargetPosition)
			|| Distance2D(mission.m_vTargetPosition, site.m_vPosition) > 1.0)
			return "exact rescue target position does not match its generated site";

		HST_ForceCompositionResult composition = missionRuntime.ComposeMissionGuardForce(
			state, preset, definition, mission);
		HST_GroupSpawnPlan groupPlan = SelectExactExecutionGroupPlan(composition);
		if (!composition || !composition.m_bSuccess || !groupPlan || groupPlan.m_sPrefab.IsEmpty()
			|| composition.m_iVehicleCount != 0 || composition.m_aVehicles.Count() != 0
			|| composition.m_aStatics.Count() != 0 || composition.m_sFactionKey != zone.m_sOwnerFactionKey)
			return "exact rescue guard composition did not resolve one hostile infantry root";
		HST_ForceCatalogValidationResult catalogValidation = m_Catalog.ValidateFactionCatalog(
			composition.m_sFactionKey, true);
		if (!catalogValidation || !catalogValidation.m_bValid)
			return "exact rescue guard faction catalog is invalid";
		HST_ForceGroupCatalogEntry catalogGroup;
		int catalogMatches;
		foreach (HST_ForceGroupCatalogEntry candidate : m_Catalog.BuildGroupCatalog(composition.m_sFactionKey))
		{
			if (!candidate || (candidate.m_sAuthoredPrefab != groupPlan.m_sPrefab
				&& candidate.m_sExecutionPrefab != groupPlan.m_sPrefab))
				continue;
			catalogGroup = candidate;
			catalogMatches++;
		}
		if (catalogMatches != 1 || !catalogGroup || catalogGroup.m_sExecutionPrefab.IsEmpty()
			|| catalogGroup.m_aMemberSlots.Count() <= 0)
			return "exact rescue guard composition has no unique catalog execution root";

		plan = new HST_RescuePOWAdmissionPlan();
		plan.m_Composition = composition;
		plan.m_CatalogGroup = catalogGroup;
		plan.m_vExtractionPosition = state.m_vHQPosition;
		if (IsZeroVector(plan.m_vExtractionPosition))
			return "exact rescue requires one deployed frozen HQ extraction point";
		plan.m_vGuardPosition = ResolveGuardAnchor(mission.m_sInstanceId, mission.m_vTargetPosition);
		plan.m_Manifest = BuildManifest(state, mission, zone, catalogGroup);
		plan.m_Operation = BuildOperation(state, mission, zone, plan.m_Manifest, plan.m_vGuardPosition);
		plan.m_GuardGroup = BuildActiveGroup(mission, zone, composition, groupPlan,
			catalogGroup, plan.m_Manifest, plan.m_vGuardPosition);
		BuildCaptiveRows(state, mission, plan.m_Manifest,
			plan.m_vExtractionPosition, plan.m_aCaptives);
		if (!plan.m_Manifest || !plan.m_Operation || !plan.m_GuardGroup
			|| plan.m_aCaptives.Count() != EXACT_CAPTIVE_COUNT)
			return "exact rescue detached authority construction failed";
		return ValidateDetachedPlan(mission, zone, plan);
	}

	protected HST_GroupSpawnPlan SelectExactExecutionGroupPlan(HST_ForceCompositionResult composition)
	{
		if (!composition)
			return null;
		HST_GroupSpawnPlan selected;
		foreach (HST_GroupSpawnPlan candidate : composition.m_aGroups)
		{
			if (!candidate || candidate.m_bSkipped || candidate.m_sPrefab.IsEmpty())
				continue;
			if (!selected || candidate.m_iManpower > selected.m_iManpower)
				selected = candidate;
		}
		return selected;
	}

	protected HST_ForceManifestState BuildManifest(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ZoneState zone,
		HST_ForceGroupCatalogEntry catalogGroup)
	{
		if (!state || !mission || !zone || !catalogGroup || catalogGroup.m_aMemberSlots.Count() <= 0)
			return null;
		HST_ForceManifestState manifest = new HST_ForceManifestState();
		manifest.m_sManifestId = BuildManifestId(mission.m_sInstanceId);
		manifest.m_sOperationId = BuildOperationId(mission.m_sInstanceId);
		manifest.m_sForceKind = EXACT_FORCE_KIND;
		manifest.m_sFactionRole = "enemy";
		manifest.m_sFactionKey = zone.m_sOwnerFactionKey;
		manifest.m_sIntentId = EXACT_INTENT_ID;
		manifest.m_sTargetZoneId = zone.m_sZoneId;
		manifest.m_sGroupPrefab = catalogGroup.m_sExecutionPrefab;
		manifest.m_sCatalogVersion = HST_ForceCatalogService.CATALOG_VERSION;
		manifest.m_sPolicyId = EXACT_POLICY_ID;
		manifest.m_iRequestedMemberCount = catalogGroup.m_aMemberSlots.Count();
		manifest.m_iAcceptedMemberCount = catalogGroup.m_aMemberSlots.Count();
		manifest.m_iRequestedVehicleCount = 0;
		manifest.m_iAcceptedVehicleCount = 0;
		manifest.m_iMoneyCost = 0;
		manifest.m_iHRCost = 0;
		manifest.m_iEquipmentCost = 0;
		manifest.m_iAttackResourceCost = 0;
		manifest.m_iSupportResourceCost = 0;
		manifest.m_iDeterministicSeed = m_Integrity.BuildDeterministicSeed(
			state, mission.m_sInstanceId + "|mission_rescue", zone.m_sZoneId);
		manifest.m_iCreatedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		manifest.m_bFrozen = true;

		HST_ForceManifestGroupState root = new HST_ForceManifestGroupState();
		root.m_sElementId = BuildGroupRootId(mission.m_sInstanceId);
		root.m_sCatalogEntryId = catalogGroup.m_sEntryId;
		root.m_sPrefab = catalogGroup.m_sExecutionPrefab;
		root.m_sRole = catalogGroup.m_sRole;
		root.m_iOrdinal = 0;
		root.m_iExpectedMemberCount = catalogGroup.m_aMemberSlots.Count();
		root.m_bRequired = true;
		manifest.m_aGroups.Insert(root);

		for (int memberIndex = 0; memberIndex < catalogGroup.m_aMemberSlots.Count(); memberIndex++)
		{
			HST_ForceGroupCatalogSlot catalogSlot = catalogGroup.m_aMemberSlots[memberIndex];
			if (!catalogSlot || catalogSlot.m_sSlotId.IsEmpty() || catalogSlot.m_sPrefab.IsEmpty())
				return null;
			HST_ForceManifestMemberState member = new HST_ForceManifestMemberState();
			member.m_sSlotId = BuildMemberSlotId(mission.m_sInstanceId, memberIndex);
			member.m_sCatalogSlotId = catalogGroup.m_sEntryId + "/" + catalogSlot.m_sSlotId;
			member.m_sGroupElementId = root.m_sElementId;
			member.m_sPrefab = catalogSlot.m_sPrefab;
			member.m_sRole = catalogSlot.m_sRole;
			member.m_iSeatIndex = -1;
			member.m_iOrdinal = memberIndex;
			member.m_iMoneyCost = 0;
			member.m_iHRCost = 0;
			member.m_iEquipmentCost = 0;
			member.m_bRequired = catalogSlot.m_bRequired;
			manifest.m_aMembers.Insert(member);
		}

		for (int captiveOrdinal = 0; captiveOrdinal < EXACT_CAPTIVE_COUNT; captiveOrdinal++)
		{
			HST_ForceManifestAssetState captiveSlot = new HST_ForceManifestAssetState();
			captiveSlot.m_sSlotId = BuildCaptiveSlotId(mission.m_sInstanceId, captiveOrdinal);
			captiveSlot.m_sKind = CAPTIVE_KIND;
			captiveSlot.m_sPrefab = CAPTIVE_PREFAB;
			captiveSlot.m_sRole = CAPTIVE_ROLE;
			captiveSlot.m_iQuantity = 1;
			captiveSlot.m_iOrdinal = captiveOrdinal;
			captiveSlot.m_bRequired = true;
			manifest.m_aAssets.Insert(captiveSlot);
		}
		manifest.m_sManifestHash = m_Integrity.BuildManifestHash(manifest);
		if (manifest.m_sManifestHash.IsEmpty())
			return null;
		return manifest;
	}

	protected HST_OperationRecordState BuildOperation(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ZoneState zone,
		HST_ForceManifestState manifest,
		vector guardPosition)
	{
		if (!state || !mission || !zone || !manifest)
			return null;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		HST_OperationRecordState operation = new HST_OperationRecordState();
		operation.m_sOperationId = BuildOperationId(mission.m_sInstanceId);
		operation.m_eType = HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE;
		operation.m_iContractVersion = EXACT_CONTRACT_VERSION;
		operation.m_iProjectionContractVersion = EXACT_PROJECTION_CONTRACT_VERSION;
		operation.m_sOwnerFactionKey = zone.m_sOwnerFactionKey;
		operation.m_sMissionInstanceId = mission.m_sInstanceId;
		operation.m_sManifestId = manifest.m_sManifestId;
		operation.m_sSpawnResultId = BuildSpawnResultId(mission.m_sInstanceId);
		operation.m_sForceId = BuildForceId(mission.m_sInstanceId);
		operation.m_sProjectionId = BuildProjectionId(mission.m_sInstanceId);
		operation.m_sGroupId = BuildProjectionId(mission.m_sInstanceId);
		operation.m_sOriginZoneId = zone.m_sZoneId;
		operation.m_vOriginPosition = guardPosition;
		operation.m_sAssignmentKind = ASSIGNMENT_KIND;
		operation.m_sAssignmentZoneId = zone.m_sZoneId;
		operation.m_vAssignmentPosition = guardPosition;
		operation.m_sTacticalTargetZoneId = zone.m_sZoneId;
		operation.m_vTacticalTargetPosition = mission.m_vTargetPosition;
		operation.m_vStrategicPosition = guardPosition;
		operation.m_sCurrentRouteId = "";
		operation.m_sRouteContractHash = "";
		operation.m_iRouteVersion = 0;
		operation.m_iRouteWaypointIndex = -1;
		operation.m_sRecallPolicyId = RECALL_POLICY_ID;
		operation.m_sSettlementPolicyId = SETTLEMENT_POLICY_ID;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
		operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		operation.m_eTerminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE;
		operation.m_iDeterministicSeed = manifest.m_iDeterministicSeed;
		operation.m_iCreatedAtSecond = nowSecond;
		operation.m_iDutyStateEnteredAtSecond = nowSecond;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iStrategicLastUpdateSecond = nowSecond;
		operation.m_iLastProjectionDecisionSecond = nowSecond;
		operation.m_iLastProgressAtSecond = nowSecond;
		operation.m_iLastVirtualFriendlyCount = manifest.m_iAcceptedMemberCount;
		operation.m_iRevision = 1;
		return operation;
	}

	protected HST_ActiveGroupState BuildActiveGroup(
		HST_ActiveMissionState mission,
		HST_ZoneState zone,
		HST_ForceCompositionResult composition,
		HST_GroupSpawnPlan groupPlan,
		HST_ForceGroupCatalogEntry catalogGroup,
		HST_ForceManifestState manifest,
		vector guardPosition)
	{
		if (!mission || !zone || !composition || !groupPlan || !catalogGroup || !manifest)
			return null;
		int living = manifest.m_iAcceptedMemberCount;
		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = BuildProjectionId(mission.m_sInstanceId);
		group.m_sOperationId = BuildOperationId(mission.m_sInstanceId);
		group.m_sManifestId = manifest.m_sManifestId;
		group.m_sSpawnResultId = BuildSpawnResultId(mission.m_sInstanceId);
		group.m_sForceId = BuildForceId(mission.m_sInstanceId);
		group.m_sProjectionId = BuildProjectionId(mission.m_sInstanceId);
		group.m_sZoneId = zone.m_sZoneId;
		group.m_sFactionKey = zone.m_sOwnerFactionKey;
		group.m_sMissionInstanceId = mission.m_sInstanceId;
		group.m_sMissionAssetId = "";
		group.m_sPrefab = catalogGroup.m_sExecutionPrefab;
		group.m_sCompositionRequestId = manifest.m_sManifestId;
		group.m_sCompositionIntentId = EXACT_INTENT_ID;
		group.m_sCompositionTier = groupPlan.m_sTier;
		group.m_sCompositionSummary = composition.m_sDebugSummary;
		group.m_sSpawnFallbackMode = EXACT_GROUP_MODE;
		group.m_vPosition = guardPosition;
		group.m_vSourcePosition = guardPosition;
		group.m_vTargetPosition = guardPosition;
		group.m_sRouteId = "";
		group.m_sRuntimeStatus = GUARD_VIRTUAL_STATUS;
		group.m_iInfantryCount = living;
		group.m_iOriginalInfantryCount = living;
		group.m_iCompositionManpower = living;
		group.m_iLastSeenAliveCount = living;
		group.m_iSurvivorInfantryCount = living;
		group.m_iDurableLivingInfantryCount = living;
		group.m_bQRF = false;
		return group;
	}

	protected void BuildCaptiveRows(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		vector extractionPosition,
		array<ref HST_MissionAssetState> captives)
	{
		if (!state || !mission || !manifest || !captives)
			return;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		for (int ordinal = 0; ordinal < EXACT_CAPTIVE_COUNT; ordinal++)
		{
			HST_MissionAssetState captive = new HST_MissionAssetState();
			captive.m_sAssetId = BuildCaptiveAssetId(mission.m_sInstanceId, ordinal);
			captive.m_sMissionInstanceId = mission.m_sInstanceId;
			captive.m_sOperationId = BuildOperationId(mission.m_sInstanceId);
			captive.m_sManifestId = manifest.m_sManifestId;
			captive.m_sManifestSlotId = BuildCaptiveSlotId(mission.m_sInstanceId, ordinal);
			captive.m_sKind = CAPTIVE_KIND;
			captive.m_sRole = CAPTIVE_ROLE;
			captive.m_sPrefab = CAPTIVE_PREFAB;
			captive.m_vSourcePosition = ResolveCaptiveStartPosition(mission.m_vTargetPosition, ordinal);
			captive.m_vCurrentPosition = captive.m_vSourcePosition;
			captive.m_vLastKnownPosition = captive.m_vSourcePosition;
			captive.m_vTargetPosition = extractionPosition;
			captive.m_iDeadlineSecond = mission.m_iActiveUntilSecond;
			captive.m_iInteractionRadiusMeters = 5;
			captive.m_iRescueContractVersion = EXACT_CONTRACT_VERSION;
			captive.m_iRescueOrdinal = ordinal;
			captive.m_eRescueDisposition = HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD;
			captive.m_sRescueProjectionId = BuildCaptiveProjectionId(mission.m_sInstanceId, ordinal);
			captive.m_iRescueTransitionSecond = nowSecond;
			captive.m_iRescueRevision = 1;
			ApplyCaptiveCompatibilityProjection(captive);
			captives.Insert(captive);
		}
	}

	protected HST_ForceSpawnQueueRequest BuildSpawnRequest(
		HST_CampaignState state,
		HST_ActiveMissionState mission)
	{
		HST_ForceSpawnQueueRequest request = new HST_ForceSpawnQueueRequest();
		if (!state || !mission)
			return request;
		request.m_sResultId = BuildSpawnResultId(mission.m_sInstanceId);
		request.m_sRequestId = BuildSpawnRequestId(mission.m_sInstanceId);
		request.m_sForceId = BuildForceId(mission.m_sInstanceId);
		request.m_sProjectionId = BuildProjectionId(mission.m_sInstanceId);
		request.m_iPriority = EXACT_PRIORITY;
		request.m_iMaxRetries = EXACT_MAX_RETRIES;
		request.m_iDeadlineSecond = Math.Max(0, state.m_iElapsedSeconds) + DEPLOYMENT_GRACE_SECONDS;
		request.m_bExternalAssetAuthority = true;
		return request;
	}

	protected string ValidateDetachedPlan(
		HST_ActiveMissionState mission,
		HST_ZoneState zone,
		HST_RescuePOWAdmissionPlan plan)
	{
		if (!mission || !zone || !plan || !plan.m_Manifest || !plan.m_Operation
			|| !plan.m_GuardGroup || plan.m_aCaptives.Count() != EXACT_CAPTIVE_COUNT)
			return "exact rescue detached plan is incomplete";
		if (IsZeroVector(plan.m_vExtractionPosition))
			return "exact rescue detached extraction anchor is incomplete";
		HST_ForceManifestState manifest = plan.m_Manifest;
		HST_OperationRecordState operation = plan.m_Operation;
		HST_ActiveGroupState group = plan.m_GuardGroup;
		if (operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| manifest.m_sPolicyId != EXACT_POLICY_ID || manifest.m_sIntentId != EXACT_INTENT_ID
			|| manifest.m_sForceKind != EXACT_FORCE_KIND)
			return "exact rescue detached contract conflicts";
		if (operation.m_sOperationId != BuildOperationId(mission.m_sInstanceId)
			|| manifest.m_sManifestId != BuildManifestId(mission.m_sInstanceId)
			|| operation.m_sManifestId != manifest.m_sManifestId
			|| operation.m_sSpawnResultId != BuildSpawnResultId(mission.m_sInstanceId)
			|| operation.m_sForceId != BuildForceId(mission.m_sInstanceId)
			|| operation.m_sProjectionId != BuildProjectionId(mission.m_sInstanceId)
			|| operation.m_sGroupId != group.m_sGroupId)
			return "exact rescue detached deterministic identity conflicts";
		if (manifest.m_sOperationId != operation.m_sOperationId
			|| group.m_sOperationId != operation.m_sOperationId
			|| group.m_sManifestId != manifest.m_sManifestId
			|| group.m_sSpawnResultId != operation.m_sSpawnResultId
			|| group.m_sForceId != operation.m_sForceId
			|| group.m_sProjectionId != operation.m_sProjectionId)
			return "exact rescue detached backlinks conflict";
		if (operation.m_sMissionInstanceId != mission.m_sInstanceId
			|| group.m_sMissionInstanceId != mission.m_sInstanceId
			|| operation.m_sAssignmentZoneId != zone.m_sZoneId
			|| group.m_sZoneId != zone.m_sZoneId
			|| operation.m_sOwnerFactionKey != zone.m_sOwnerFactionKey
			|| group.m_sFactionKey != zone.m_sOwnerFactionKey)
			return "exact rescue detached assignment conflicts";
		if (!manifest.m_bFrozen || manifest.m_aGroups.Count() != 1
			|| manifest.m_aMembers.Count() <= 0 || manifest.m_aVehicles.Count() != 0
			|| manifest.m_aAssets.Count() != EXACT_CAPTIVE_COUNT
			|| manifest.m_sManifestHash != m_Integrity.BuildManifestHash(manifest))
			return "exact rescue detached frozen manifest conflicts";
		array<int> ordinals = {};
		foreach (HST_MissionAssetState captive : plan.m_aCaptives)
		{
			if (!captive || ordinals.Contains(captive.m_iRescueOrdinal)
				|| captive.m_sOperationId != operation.m_sOperationId
				|| captive.m_sManifestId != manifest.m_sManifestId
				|| captive.m_sManifestSlotId != BuildCaptiveSlotId(mission.m_sInstanceId, captive.m_iRescueOrdinal)
				|| captive.m_sRescueProjectionId != BuildCaptiveProjectionId(mission.m_sInstanceId, captive.m_iRescueOrdinal)
				|| !PositionsWithin2D(captive.m_vTargetPosition, plan.m_vExtractionPosition, 1.0)
				|| captive.m_eRescueDisposition != HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD)
				return "exact rescue detached captive authority conflicts";
			ordinals.Insert(captive.m_iRescueOrdinal);
		}
		return "";
	}

	protected string ValidateCommittedGraph(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		bool requireActiveMission)
	{
		if (!state || !mission || !operation || !manifest || !batch || !group)
			return "exact rescue committed graph is incomplete";
		if (!IsExactMission(mission) || IsZeroVector(mission.m_vRescueExtractionPosition)
			|| (requireActiveMission
			&& mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE))
			return "exact rescue committed mission contract conflicts";
		if (CountMissionIdentity(state, mission) != 1 || CountOperationIdentity(state, operation) != 1
			|| CountManifestIdentity(state, manifest) != 1 || CountBatchIdentity(state, batch) != 1
			|| CountGroupIdentity(state, group) != 1)
			return "exact rescue committed identity is ambiguous";
		if (operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| operation.m_iProjectionContractVersion != EXACT_PROJECTION_CONTRACT_VERSION)
			return "exact rescue operation contract conflicts";
		if (mission.m_sOperationId != operation.m_sOperationId
			|| mission.m_sManifestId != manifest.m_sManifestId
			|| mission.m_sSpawnResultId != batch.m_sResultId
			|| operation.m_sMissionInstanceId != mission.m_sInstanceId
			|| operation.m_sManifestId != manifest.m_sManifestId
			|| operation.m_sSpawnResultId != batch.m_sResultId
			|| operation.m_sGroupId != group.m_sGroupId)
			return "exact rescue reciprocal backlinks conflict";
		if (manifest.m_sOperationId != operation.m_sOperationId
			|| manifest.m_sPolicyId != EXACT_POLICY_ID || manifest.m_sIntentId != EXACT_INTENT_ID
			|| manifest.m_sForceKind != EXACT_FORCE_KIND || !manifest.m_bFrozen
			|| manifest.m_aGroups.Count() != 1 || manifest.m_aMembers.Count() <= 0
			|| manifest.m_aVehicles.Count() != 0 || manifest.m_aAssets.Count() != EXACT_CAPTIVE_COUNT
			|| manifest.m_sManifestHash != m_Integrity.BuildManifestHash(manifest))
			return "exact rescue frozen manifest conflicts";
		if (batch.m_sManifestId != manifest.m_sManifestId
			|| batch.m_sManifestHash != manifest.m_sManifestHash
			|| batch.m_sOperationId != operation.m_sOperationId
			|| batch.m_sForceId != operation.m_sForceId
			|| batch.m_sProjectionId != operation.m_sProjectionId
			|| batch.m_sRequestId != BuildSpawnRequestId(mission.m_sInstanceId)
			|| !batch.m_bExternalAssetAuthority)
			return "exact rescue guard batch conflicts";
		if (group.m_sOperationId != operation.m_sOperationId
			|| group.m_sManifestId != manifest.m_sManifestId
			|| group.m_sSpawnResultId != batch.m_sResultId
			|| group.m_sForceId != operation.m_sForceId
			|| group.m_sProjectionId != operation.m_sProjectionId
			|| group.m_sSpawnFallbackMode != EXACT_GROUP_MODE)
			return "exact rescue guard projection conflicts";
		array<ref HST_MissionAssetState> captives = {};
		CollectExactCaptives(state, mission, captives);
		if (captives.Count() != EXACT_CAPTIVE_COUNT)
			return "exact rescue requires exactly three durable captive rows";
		array<int> ordinals = {};
		foreach (HST_MissionAssetState captive : captives)
		{
			if (!IsExactRescueCaptiveAsset(state, captive) || ordinals.Contains(captive.m_iRescueOrdinal)
				|| !PositionsWithin2D(captive.m_vTargetPosition,
					mission.m_vRescueExtractionPosition, 1.0))
				return "exact rescue captive authority conflicts";
			ordinals.Insert(captive.m_iRescueOrdinal);
			HST_ForceManifestAssetState slot = FindManifestAssetSlot(
				manifest, captive.m_sManifestSlotId);
			if (!slot || slot.m_iOrdinal != captive.m_iRescueOrdinal || slot.m_sKind != CAPTIVE_KIND
				|| slot.m_sRole != CAPTIVE_ROLE || slot.m_sPrefab != CAPTIVE_PREFAB
				|| !slot.m_bRequired || slot.m_iQuantity != 1)
				return "exact rescue captive manifest slot conflicts";
		}
		return "";
	}

	protected bool HasAnyAdmissionAuthority(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;
		if (!mission.m_sOperationId.IsEmpty() || !mission.m_sManifestId.IsEmpty()
			|| !mission.m_sSpawnResultId.IsEmpty() || !mission.m_sSettlementId.IsEmpty())
			return true;
		string operationId = BuildOperationId(mission.m_sInstanceId);
		string manifestId = BuildManifestId(mission.m_sInstanceId);
		string resultId = BuildSpawnResultId(mission.m_sInstanceId);
		if (state.FindOperation(operationId) || state.FindForceManifest(manifestId)
			|| state.FindForceSpawnResult(resultId) || state.FindActiveGroup(BuildProjectionId(mission.m_sInstanceId)))
			return true;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && (asset.m_sMissionInstanceId == mission.m_sInstanceId
				|| asset.m_sOperationId == operationId || asset.m_sManifestId == manifestId))
				return true;
		}
		return false;
	}

	protected string FindAdmissionIdentityCollision(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_RescuePOWAdmissionPlan plan)
	{
		if (!state || !mission || !plan)
			return "exact rescue collision context is incomplete";
		if (state.FindOperation(plan.m_Operation.m_sOperationId)
			|| state.FindForceManifest(plan.m_Manifest.m_sManifestId)
			|| state.FindForceSpawnResult(BuildSpawnResultId(mission.m_sInstanceId))
			|| state.FindActiveGroup(plan.m_GuardGroup.m_sGroupId))
			return "exact rescue deterministic authority identity already exists";
		foreach (HST_MissionAssetState captive : plan.m_aCaptives)
		{
			if (state.FindMissionAsset(captive.m_sAssetId))
				return "exact rescue deterministic captive identity already exists";
		}
		foreach (HST_MissionAssetState existing : state.m_aMissionAssets)
		{
			if (existing && existing.m_sMissionInstanceId == mission.m_sInstanceId)
				return "exact rescue mission already has broad or foreign asset authority";
		}
		return "";
	}

	protected void CollectExactCaptives(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		array<ref HST_MissionAssetState> output)
	{
		if (!state || !mission || !output)
			return;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId
				&& asset.m_iRescueContractVersion == EXACT_CONTRACT_VERSION)
				output.Insert(asset);
		}
	}

	protected static bool CollectValidatedRequiredCaptives(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		array<ref HST_MissionAssetState> output)
	{
		if (!state || !IsExactMission(mission) || !output)
			return false;
		output.Clear();
		array<int> ordinals = {};
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (!IsExactRescueCaptiveAsset(state, asset)
				|| ordinals.Contains(asset.m_iRescueOrdinal))
				return false;
			ordinals.Insert(asset.m_iRescueOrdinal);
			output.Insert(asset);
		}
		return output.Count() == EXACT_CAPTIVE_COUNT
			&& ordinals.Count() == EXACT_CAPTIVE_COUNT;
	}

	protected HST_ForceManifestAssetState FindManifestAssetSlot(
		HST_ForceManifestState manifest,
		string slotId)
	{
		if (!manifest || slotId.IsEmpty())
			return null;
		foreach (HST_ForceManifestAssetState slot : manifest.m_aAssets)
		{
			if (slot && slot.m_sSlotId == slotId)
				return slot;
		}
		return null;
	}

	protected int CountMissionIdentity(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		int count;
		foreach (HST_ActiveMissionState candidate : state.m_aActiveMissions)
		{
			if (candidate && candidate.m_sInstanceId == mission.m_sInstanceId)
				count++;
		}
		return count;
	}

	protected int CountOperationIdentity(HST_CampaignState state, HST_OperationRecordState operation)
	{
		int count;
		foreach (HST_OperationRecordState candidate : state.m_aOperations)
		{
			if (candidate && candidate.m_sOperationId == operation.m_sOperationId)
				count++;
		}
		return count;
	}

	protected bool ResolveExactInstanceOperation(
		HST_CampaignState state,
		string instanceId,
		out HST_ActiveMissionState mission,
		out HST_OperationRecordState operation)
	{
		mission = null;
		operation = null;
		if (!state || instanceId.IsEmpty())
			return false;
		mission = state.FindActiveMission(instanceId);
		if (!mission || mission.m_sInstanceId != instanceId || !IsExactMission(mission)
			|| CountMissionIdentity(state, mission) != 1 || mission.m_sOperationId.IsEmpty())
		{
			mission = null;
			return false;
		}
		// The exact-instance seam is containment-scoped. Resolve every operation
		// backlink before mutation so a conflicting claimant cannot be quarantined
		// or settled as a side effect of an ambiguous lookup.
		int claimantCount;
		foreach (HST_OperationRecordState candidate : state.m_aOperations)
		{
			if (!candidate)
				continue;
			bool claimsInstance = candidate.m_sMissionInstanceId == instanceId;
			bool claimsOperationId = candidate.m_sOperationId == mission.m_sOperationId;
			if (!claimsInstance && !claimsOperationId)
				continue;
			claimantCount++;
			if (claimsInstance && claimsOperationId
				&& candidate.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
				&& candidate.m_iContractVersion == EXACT_CONTRACT_VERSION)
				operation = candidate;
		}
		if (claimantCount == 1 && operation
			&& state.FindOperation(mission.m_sOperationId) == operation)
			return true;
		mission = null;
		operation = null;
		return false;
	}

	protected int CountManifestIdentity(HST_CampaignState state, HST_ForceManifestState manifest)
	{
		int count;
		foreach (HST_ForceManifestState candidate : state.m_aForceManifests)
		{
			if (candidate && candidate.m_sManifestId == manifest.m_sManifestId)
				count++;
		}
		return count;
	}

	protected int CountBatchIdentity(HST_CampaignState state, HST_ForceSpawnResultState batch)
	{
		int count;
		foreach (HST_ForceSpawnResultState candidate : state.m_aForceSpawnResults)
		{
			if (candidate && candidate.m_sResultId == batch.m_sResultId)
				count++;
		}
		return count;
	}

	protected int CountGroupIdentity(HST_CampaignState state, HST_ActiveGroupState group)
	{
		int count;
		foreach (HST_ActiveGroupState candidate : state.m_aActiveGroups)
		{
			if (candidate && candidate.m_sGroupId == group.m_sGroupId)
				count++;
		}
		return count;
	}

	protected vector ResolveGuardAnchor(string missionInstanceId, vector targetPosition)
	{
		float side = 1.0;
		if ((missionInstanceId.Length() % 2) == 0)
			side = -1.0;
		return targetPosition + Vector(8.0 * side, 0.0, 6.0);
	}

	protected vector ResolveCaptiveStartPosition(vector targetPosition, int ordinal)
	{
		if (ordinal == 0)
			return targetPosition + Vector(-2.0, 0.0, 1.5);
		if (ordinal == 1)
			return targetPosition + Vector(0.0, 0.0, -1.5);
		return targetPosition + Vector(2.0, 0.0, 1.5);
	}

	HST_RescuePOWTransitionResult ApplyCaptiveTransition(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState captive,
		HST_ERescueCaptiveDisposition nextDisposition,
		string actorIdentityId,
		string commandRequestId,
		string carrierVehicleId = "",
		string carrierSeatToken = "",
		string evidenceReceiptId = "")
	{
		HST_RescuePOWTransitionResult result = new HST_RescuePOWTransitionResult();
		result.m_Captive = captive;
		if (!state || !mission || !captive || !IsExactMission(mission)
			|| !IsExactRescueCaptiveAsset(state, captive))
		{
			result.m_sFailureReason = "exact rescue captive transition authority is unavailable";
			return result;
		}
		string expectedResult = string.Format("accepted:%1", nextDisposition);
		if (!commandRequestId.IsEmpty() && captive.m_sRescueLastCommandRequestId == commandRequestId)
		{
			result.m_bAlreadyApplied = true;
			result.m_sResult = captive.m_sRescueLastCommandResult;
			result.m_bSuccess = captive.m_sRescueLastCommandResult == expectedResult;
			if (!result.m_bSuccess)
				result.m_sFailureReason = captive.m_sRescueLastCommandResult;
			return result;
		}
		if (IsTerminalCaptiveDisposition(captive.m_eRescueDisposition))
			return RejectCaptiveTransition(captive, commandRequestId,
				"terminal exact rescue captive cannot transition", result);
		if (!IsAllowedCaptiveTransition(captive.m_eRescueDisposition, nextDisposition))
			return RejectCaptiveTransition(captive, commandRequestId,
				"exact rescue captive transition is not allowed", result);

		bool actorRequired = nextDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED
			|| nextDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
			|| nextDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
			|| nextDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED
			|| nextDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED;
		if (actorRequired && actorIdentityId.IsEmpty())
			return RejectCaptiveTransition(captive, commandRequestId,
				"exact rescue captive transition requires a stable escort identity", result);
		if (!captive.m_sRescueEscortIdentityId.IsEmpty() && !actorIdentityId.IsEmpty()
			&& captive.m_sRescueEscortIdentityId != actorIdentityId
			&& nextDisposition != HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED
			&& IsEscortAuthorityValid(state, captive.m_sRescueEscortIdentityId))
			return RejectCaptiveTransition(captive, commandRequestId,
				"exact rescue captive is owned by another valid escort", result);
		if (nextDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED
			&& (carrierVehicleId.IsEmpty() || carrierSeatToken.IsEmpty()))
			return RejectCaptiveTransition(captive, commandRequestId,
				"boarded exact rescue captive requires observed vehicle and seat evidence", result);
		if (nextDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED
			&& evidenceReceiptId.IsEmpty())
			return RejectCaptiveTransition(captive, commandRequestId,
				"killed exact rescue captive requires an observed casualty receipt", result);
		if (nextDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED
			&& evidenceReceiptId.IsEmpty())
			return RejectCaptiveTransition(captive, commandRequestId,
				"extracted exact rescue captive requires an extraction receipt", result);

		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		captive.m_eRescueDisposition = nextDisposition;
		captive.m_iRescueTransitionSecond = nowSecond;
		captive.m_iRescueRevision++;
		captive.m_sRescueLastCommandRequestId = commandRequestId;
		captive.m_sRescueLastCommandResult = expectedResult;
		if (actorRequired)
			captive.m_sRescueEscortIdentityId = actorIdentityId;
		if (nextDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED)
		{
			captive.m_sRescueEscortIdentityId = "";
			captive.m_sRescueCarrierVehicleId = "";
			captive.m_sRescueCarrierSeatToken = "";
		}
		else if (nextDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING)
		{
			captive.m_sRescueCarrierVehicleId = "";
			captive.m_sRescueCarrierSeatToken = "";
		}
		else if (nextDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING)
		{
			captive.m_sRescueCarrierVehicleId = carrierVehicleId;
			captive.m_sRescueCarrierSeatToken = "";
		}
		else if (nextDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED)
		{
			captive.m_sRescueCarrierVehicleId = carrierVehicleId;
			captive.m_sRescueCarrierSeatToken = carrierSeatToken;
		}
		else if (nextDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED)
		{
			captive.m_bRescueExtractionObserved = true;
			captive.m_sRescueExtractionReceiptId = evidenceReceiptId;
			captive.m_sRescueCarrierVehicleId = "";
			captive.m_sRescueCarrierSeatToken = "";
		}
		else if (nextDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED)
		{
			captive.m_bRescueDeathObserved = true;
			captive.m_sRescueCasualtyReceiptId = evidenceReceiptId;
			captive.m_sRescueEscortIdentityId = "";
			captive.m_sRescueCarrierVehicleId = "";
			captive.m_sRescueCarrierSeatToken = "";
		}
		ApplyCaptiveCompatibilityProjection(captive);
		SyncMissionCaptiveCounters(state, mission);
		result.m_bSuccess = true;
		result.m_bStateChanged = true;
		result.m_sResult = expectedResult;
		return result;
	}

	HST_RescuePOWTransitionResult HandleCaptiveCommand(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState captive,
		string command,
		string actorIdentityId,
		string requestId,
		string carrierVehicleId = "",
		string carrierSeatToken = "")
	{
		HST_RescuePOWTransitionResult rejected = new HST_RescuePOWTransitionResult();
		rejected.m_Captive = captive;
		if (!state || !mission || !captive || actorIdentityId.IsEmpty() || requestId.IsEmpty())
		{
			rejected.m_sFailureReason = "exact rescue command identity is incomplete";
			return rejected;
		}
		if (!IsExactMission(mission) || !IsExactRescueCaptiveAsset(state, captive))
		{
			rejected.m_sFailureReason = "exact rescue command authority is unavailable";
			return rejected;
		}
		HST_RescuePOWTransitionResult durableReplay;
		if (TryReplayCaptiveCommandReceipt(
			state, captive, actorIdentityId, command, requestId, durableReplay))
			return durableReplay;
		if (captive.m_sRescueLastCommandRequestId == requestId)
		{
			rejected.m_bAlreadyApplied = true;
			rejected.m_sResult = captive.m_sRescueLastCommandResult;
			rejected.m_bSuccess = captive.m_sRescueLastCommandResult.StartsWith("accepted:");
			if (!rejected.m_bSuccess)
				rejected.m_sFailureReason = captive.m_sRescueLastCommandResult;
			return rejected;
		}
		EnsureCaptiveCommandReceiptArray(captive);
		bool freeCommand = command == "free" || (command == "mission_captive_extract"
			&& captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD);
		bool followCommand = command == "follow" || command == "mission_captive_follow";
		bool boardCommand = command == "board" || command == "mission_captive_board";
		bool extractCommand = command == "extract" || (command == "mission_captive_extract" && !freeCommand);
		bool terminalExtractCommand = extractCommand
			&& (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
				|| captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED);
		if (!freeCommand && !followCommand && !boardCommand && !extractCommand)
		{
			rejected.m_sFailureReason = "unsupported exact rescue captive command";
			rejected.m_sResult = "rejected:" + rejected.m_sFailureReason;
			return rejected;
		}
		int receiptLimit = MAX_CAPTIVE_COMMAND_RECEIPTS - 1;
		if (terminalExtractCommand)
			receiptLimit = MAX_CAPTIVE_COMMAND_RECEIPTS;
		if (captive.m_aRescueCommandReceipts.Count() >= receiptLimit
			&& !DiscardOldestRejectedCommandReceipt(captive))
		{
			rejected.m_sFailureReason = "exact rescue captive command receipt ledger is saturated";
			rejected.m_sResult = "rejected:" + rejected.m_sFailureReason;
			return rejected;
		}
		if (mission.m_bRescueExtractionGrace
			&& !captive.m_sRescueEscortIdentityId.IsEmpty()
			&& captive.m_sRescueEscortIdentityId != actorIdentityId)
			return RememberCaptiveCommandReceipt(captive, actorIdentityId, command, requestId,
				RejectCaptiveTransition(captive, requestId,
					"extraction grace freezes the existing captive escort identity", rejected));
		if (mission.m_bRescueExtractionGrace && (freeCommand || (followCommand
			&& captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED)))
			return RememberCaptiveCommandReceipt(captive, actorIdentityId, command, requestId,
				RejectCaptiveTransition(captive, requestId,
					"extraction grace does not permit new captive release or claim", rejected));
		if (freeCommand)
			return RememberCaptiveCommandReceipt(captive, actorIdentityId, command, requestId,
				ApplyCaptiveTransition(state, mission, captive,
					HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED,
					actorIdentityId, requestId));
		if (followCommand)
		{
			if (captive.m_eRescueDisposition
				!= HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED)
				return RememberCaptiveCommandReceipt(captive, actorIdentityId, command, requestId,
					RejectCaptiveTransition(captive, requestId,
						"exact rescue follow command requires one freed unclaimed captive", rejected));
			return RememberCaptiveCommandReceipt(captive, actorIdentityId, command, requestId,
				ApplyCaptiveTransition(state, mission, captive,
					HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING,
					actorIdentityId, requestId));
		}
		if (boardCommand)
		{
			if (captive.m_eRescueDisposition
				!= HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING)
				return RememberCaptiveCommandReceipt(captive, actorIdentityId, command, requestId,
					RejectCaptiveTransition(captive, requestId,
						"exact rescue board command requires one following captive", rejected));
			HST_ERescueCaptiveDisposition boarding = HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING;
			if (!carrierVehicleId.IsEmpty() && !carrierSeatToken.IsEmpty())
				boarding = HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
			return RememberCaptiveCommandReceipt(captive, actorIdentityId, command, requestId,
				ApplyCaptiveTransition(state, mission, captive, boarding,
					actorIdentityId, requestId, carrierVehicleId, carrierSeatToken));
		}
		if (extractCommand)
		{
			if (!terminalExtractCommand)
				return RememberCaptiveCommandReceipt(captive, actorIdentityId, command, requestId,
					RejectCaptiveTransition(captive, requestId,
						"exact rescue extraction requires a following or boarded captive", rejected));
			float extractionRadius = Math.Max(5.0, captive.m_iInteractionRadiusMeters);
			if (Distance2D(captive.m_vCurrentPosition, captive.m_vTargetPosition) > extractionRadius)
				return RememberCaptiveCommandReceipt(captive, actorIdentityId, command, requestId,
					RejectCaptiveTransition(captive, requestId,
						"exact rescue captive is outside the frozen extraction radius", rejected));
			string receipt = BuildCaptiveExtractionReceiptId(mission.m_sInstanceId, captive.m_sAssetId);
			return RememberCaptiveCommandReceipt(captive, actorIdentityId, command, requestId,
				ApplyCaptiveTransition(state, mission, captive,
					HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED,
					actorIdentityId, requestId, "", "", receipt));
		}
		return rejected;
	}

	bool MarkCaptiveDeathObserved(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState captive,
		string evidence = "runtime_destroyed")
	{
		if (!state || !mission || !captive || !IsExactRescueCaptiveAsset(state, captive))
			return false;
		if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED)
			return captive.m_bRescueDeathObserved
				&& captive.m_sRescueCasualtyReceiptId
					== BuildCaptiveCasualtyReceiptId(mission.m_sInstanceId, captive.m_sAssetId);
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;
		HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
		if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| operation.m_sMissionInstanceId != mission.m_sInstanceId
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
			return false;
		string receipt = BuildCaptiveCasualtyReceiptId(mission.m_sInstanceId, captive.m_sAssetId);
		HST_RescuePOWTransitionResult transitioned = ApplyCaptiveTransition(
			state, mission, captive,
			HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED,
			"", "system_death_" + receipt, "", "", receipt);
		if (transitioned && transitioned.m_bSuccess && !evidence.IsEmpty())
			captive.m_sOutcomeKind = evidence;
		return transitioned && transitioned.m_bSuccess;
	}

	bool MarkCaptiveExtractionObserved(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState captive,
		string actorIdentityId,
		string commandRequestId)
	{
		if (!state || !mission || !captive)
			return false;
		string receipt = BuildCaptiveExtractionReceiptId(mission.m_sInstanceId, captive.m_sAssetId);
		HST_RescuePOWTransitionResult transitioned = ApplyCaptiveTransition(
			state, mission, captive,
			HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED,
			actorIdentityId, commandRequestId, "", "", receipt);
		return transitioned && transitioned.m_bSuccess;
	}

	static bool ApplyCaptiveCompatibilityProjection(HST_MissionAssetState captive)
	{
		if (!captive || captive.m_iRescueContractVersion != EXACT_CONTRACT_VERSION)
			return false;
		bool beforeSpawned = captive.m_bSpawned;
		bool beforePicked = captive.m_bPickedUp;
		bool beforeDelivered = captive.m_bDelivered;
		bool beforeDestroyed = captive.m_bDestroyed;
		bool beforeAlive = captive.m_bAlive;
		bool beforeAttached = captive.m_bAttachedToCarrier;
		string beforeCarrier = captive.m_sCarriedByVehicleId;
		string beforeInteraction = captive.m_sLastInteraction;

		captive.m_bPickedUp = captive.m_eRescueDisposition != HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD
			&& captive.m_eRescueDisposition != HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_UNKNOWN;
		captive.m_bDelivered = captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED;
		captive.m_bDestroyed = captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED;
		captive.m_bAlive = !captive.m_bDestroyed;
		captive.m_bAttachedToCarrier = captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
			|| captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
			|| captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
		captive.m_sCarriedByVehicleId = "";
		if (captive.m_bAttachedToCarrier)
		{
			captive.m_sCarriedByVehicleId = captive.m_sRescueCarrierVehicleId;
			if (captive.m_sCarriedByVehicleId.IsEmpty())
				captive.m_sCarriedByVehicleId = captive.m_sRescueEscortIdentityId;
		}
		if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD)
			captive.m_sLastInteraction = "held";
		else if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED)
			captive.m_sLastInteraction = "freed";
		else if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING)
			captive.m_sLastInteraction = "following";
		else if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING)
			captive.m_sLastInteraction = "boarding";
		else if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED)
			captive.m_sLastInteraction = "loaded";
		else if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED)
			captive.m_sLastInteraction = "extracted";
		else if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED)
			captive.m_sLastInteraction = "killed";
		if (captive.m_bDelivered || captive.m_bDestroyed)
			captive.m_bSpawned = false;
		return beforeSpawned != captive.m_bSpawned || beforePicked != captive.m_bPickedUp
			|| beforeDelivered != captive.m_bDelivered || beforeDestroyed != captive.m_bDestroyed
			|| beforeAlive != captive.m_bAlive || beforeAttached != captive.m_bAttachedToCarrier
			|| beforeCarrier != captive.m_sCarriedByVehicleId || beforeInteraction != captive.m_sLastInteraction;
	}

	static bool HasAnyRequiredCaptiveDeath(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		array<ref HST_MissionAssetState> captives = {};
		if (!CollectValidatedRequiredCaptives(state, mission, captives))
			return false;
		foreach (HST_MissionAssetState captive : captives)
		{
			if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED
				&& captive.m_bRescueDeathObserved
				&& captive.m_sRescueCasualtyReceiptId
					== BuildCaptiveCasualtyReceiptId(mission.m_sInstanceId, captive.m_sAssetId))
				return true;
		}
		return false;
	}

	static bool AreAllRequiredCaptivesExtracted(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		array<ref HST_MissionAssetState> captives = {};
		if (!CollectValidatedRequiredCaptives(state, mission, captives)
			|| IsZeroVector(mission.m_vRescueExtractionPosition))
			return false;
		int extracted;
		foreach (HST_MissionAssetState captive : captives)
		{
			if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED
				&& captive.m_bRescueExtractionObserved
				&& captive.m_sRescueExtractionReceiptId
					== BuildCaptiveExtractionReceiptId(mission.m_sInstanceId, captive.m_sAssetId)
				&& PositionsWithin2D(captive.m_vTargetPosition,
					mission.m_vRescueExtractionPosition, 1.0)
				&& PositionsWithin2D(captive.m_vCurrentPosition, captive.m_vTargetPosition,
					Math.Max(5.0, captive.m_iInteractionRadiusMeters)))
				extracted++;
		}
		return extracted == EXACT_CAPTIVE_COUNT;
	}

	protected static bool PositionsWithin2D(vector left, vector right, float radius)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return dx * dx + dz * dz <= radius * radius;
	}

	static bool CanEnterExtractionGrace(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !IsExactMission(mission) || HasAnyRequiredCaptiveDeath(state, mission)
			|| AreAllRequiredCaptivesExtracted(state, mission))
			return false;
		array<ref HST_MissionAssetState> captives = {};
		if (!CollectValidatedRequiredCaptives(state, mission, captives))
			return false;
		int inCustody;
		foreach (HST_MissionAssetState captive : captives)
		{
			bool custody = captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
				|| captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
				|| captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
			if (custody && IsEscortAuthorityValid(state, captive.m_sRescueEscortIdentityId))
				inCustody++;
		}
		return inCustody == EXACT_CAPTIVE_COUNT;
	}

	static bool IsExtractionGraceCustodyValid(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !IsExactMission(mission) || HasAnyRequiredCaptiveDeath(state, mission))
			return false;
		array<ref HST_MissionAssetState> captives = {};
		if (!CollectValidatedRequiredCaptives(state, mission, captives))
			return false;
		int valid;
		foreach (HST_MissionAssetState captive : captives)
		{
			bool extracted = captive.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED
				&& captive.m_bRescueExtractionObserved
				&& captive.m_sRescueExtractionReceiptId
					== BuildCaptiveExtractionReceiptId(mission.m_sInstanceId, captive.m_sAssetId);
			bool custody = (captive.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
				|| captive.m_eRescueDisposition
					== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
				|| captive.m_eRescueDisposition
					== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED)
				&& IsEscortAuthorityValid(state, captive.m_sRescueEscortIdentityId);
			if (extracted || custody)
				valid++;
		}
		return valid == EXACT_CAPTIVE_COUNT;
	}

	bool BeginExtractionGrace(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_bRescueExtractionGrace || !CanEnterExtractionGrace(state, mission))
			return false;
		int baseDeadlineSecond = mission.m_iActiveUntilSecond;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		int graceUntilSecond = baseDeadlineSecond + EXTRACTION_GRACE_SECONDS;
		if (baseDeadlineSecond <= 0 || nowSecond < baseDeadlineSecond
			|| nowSecond >= graceUntilSecond)
			return false;
		mission.m_bRescueExtractionGrace = true;
		mission.m_iRescueGraceUntilSecond = graceUntilSecond;
		mission.m_iActiveUntilSecond = mission.m_iRescueGraceUntilSecond;
		mission.m_sRuntimePhase = RESCUE_GRACE_PHASE;
		mission.m_iRemainingSeconds = graceUntilSecond - nowSecond;
		return true;
	}

	static bool IsExtractionGraceOpen(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		return state && mission && IsExactMission(mission) && mission.m_bRescueExtractionGrace
			&& mission.m_sRuntimePhase == RESCUE_GRACE_PHASE
			&& mission.m_iRescueGraceUntilSecond > state.m_iElapsedSeconds
			&& IsExtractionGraceCustodyValid(state, mission);
	}

	protected bool SyncMissionCaptiveCounters(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;
		int extracted;
		int dead;
		foreach (HST_MissionAssetState captive : state.m_aMissionAssets)
		{
			if (!captive || captive.m_sMissionInstanceId != mission.m_sInstanceId
				|| captive.m_iRescueContractVersion != EXACT_CONTRACT_VERSION)
				continue;
			ApplyCaptiveCompatibilityProjection(captive);
			if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED)
				extracted++;
			else if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED)
				dead++;
		}
		bool changed = mission.m_iRequiredCaptiveCount != EXACT_CAPTIVE_COUNT
			|| mission.m_iExtractedCaptiveCount != extracted
			|| mission.m_iRuntimeDeliveryCount != extracted
			|| mission.m_iRuntimeDestroyedCount != dead;
		mission.m_iRequiredCaptiveCount = EXACT_CAPTIVE_COUNT;
		mission.m_iExtractedCaptiveCount = extracted;
		mission.m_iRuntimeDeliveryCount = extracted;
		mission.m_iRuntimeDestroyedCount = dead;
		return changed;
	}

	protected HST_RescuePOWTransitionResult RejectCaptiveTransition(
		HST_MissionAssetState captive,
		string commandRequestId,
		string reason,
		HST_RescuePOWTransitionResult result)
	{
		result.m_Captive = captive;
		result.m_sFailureReason = reason;
		result.m_sResult = "rejected:" + reason;
		if (captive && !commandRequestId.IsEmpty())
		{
			captive.m_sRescueLastCommandRequestId = commandRequestId;
			captive.m_sRescueLastCommandResult = result.m_sResult;
			captive.m_iRescueRevision++;
			result.m_bStateChanged = true;
		}
		return result;
	}

	protected void EnsureCaptiveCommandReceiptArray(HST_MissionAssetState captive)
	{
		if (!captive)
			return;
		if (!captive.m_aRescueCommandReceipts)
			captive.m_aRescueCommandReceipts = {};
	}

	bool TryReplayCaptiveCommandReceipt(
		HST_CampaignState state,
		HST_MissionAssetState captive,
		string actorIdentityId,
		string command,
		string requestId,
		out HST_RescuePOWTransitionResult result)
	{
		result = null;
		if (!state || !captive || requestId.IsEmpty())
			return false;
		EnsureCaptiveCommandReceiptArray(captive);
		HST_RescueCommandReceiptState receipt;
		foreach (HST_RescueCommandReceiptState candidate : captive.m_aRescueCommandReceipts)
		{
			if (candidate && candidate.m_sRequestId == requestId)
			{
				receipt = candidate;
				break;
			}
		}
		if (!receipt)
		{
			foreach (HST_MissionAssetState foreignCaptive : state.m_aMissionAssets)
			{
				if (!foreignCaptive || foreignCaptive == captive
					|| foreignCaptive.m_iRescueContractVersion != EXACT_CONTRACT_VERSION
					|| !foreignCaptive.m_aRescueCommandReceipts)
					continue;
				foreach (HST_RescueCommandReceiptState foreignReceipt : foreignCaptive.m_aRescueCommandReceipts)
				{
					if (!foreignReceipt || foreignReceipt.m_sRequestId != requestId)
						continue;
					result = new HST_RescuePOWTransitionResult();
					result.m_Captive = captive;
					result.m_bAlreadyApplied = true;
					result.m_sFailureReason = "exact rescue request identity already belongs to another captive";
					result.m_sResult = "rejected:" + result.m_sFailureReason;
					return true;
				}
			}
			return false;
		}
		result = new HST_RescuePOWTransitionResult();
		result.m_Captive = captive;
		result.m_bAlreadyApplied = true;
		if (receipt.m_sActorIdentityId != actorIdentityId || receipt.m_sCommand != command)
		{
			result.m_sFailureReason = "exact rescue request identity was reused with a different actor or command";
			result.m_sResult = "rejected:" + result.m_sFailureReason;
			return true;
		}
		result.m_sResult = receipt.m_sResult;
		result.m_bSuccess = receipt.m_sResult.StartsWith("accepted:");
		if (!result.m_bSuccess)
			result.m_sFailureReason = receipt.m_sResult;
		return true;
	}

	protected HST_RescuePOWTransitionResult RememberCaptiveCommandReceipt(
		HST_MissionAssetState captive,
		string actorIdentityId,
		string command,
		string requestId,
		HST_RescuePOWTransitionResult result)
	{
		if (!captive || !result || actorIdentityId.IsEmpty()
			|| command.IsEmpty() || requestId.IsEmpty())
			return result;
		EnsureCaptiveCommandReceiptArray(captive);
		if (result.m_sResult.IsEmpty())
		{
			if (result.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = "exact rescue command was rejected without a result receipt";
			result.m_sResult = "rejected:" + result.m_sFailureReason;
		}
		foreach (HST_RescueCommandReceiptState existing : captive.m_aRescueCommandReceipts)
		{
			if (existing && existing.m_sRequestId == requestId)
				return result;
		}
		if (!result.m_bSuccess)
		{
			while (CountRejectedCommandReceipts(captive)
					>= MAX_REJECTED_CAPTIVE_COMMAND_RECEIPTS
				|| captive.m_aRescueCommandReceipts.Count()
					>= MAX_CAPTIVE_COMMAND_RECEIPTS - 1)
			{
				if (!DiscardOldestRejectedCommandReceipt(captive))
					break;
			}
		}
		if (captive.m_aRescueCommandReceipts.Count() >= MAX_CAPTIVE_COMMAND_RECEIPTS)
			return result;
		HST_RescueCommandReceiptState receipt = new HST_RescueCommandReceiptState();
		receipt.m_sRequestId = requestId;
		receipt.m_sActorIdentityId = actorIdentityId;
		receipt.m_sCommand = command;
		receipt.m_sResult = result.m_sResult;
		receipt.m_iRecordedRevision = captive.m_iRescueRevision;
		captive.m_aRescueCommandReceipts.Insert(receipt);
		if (!result.m_bStateChanged)
		{
			captive.m_iRescueRevision++;
			result.m_bStateChanged = true;
		}
		return result;
	}

	protected int CountRejectedCommandReceipts(HST_MissionAssetState captive)
	{
		if (!captive || !captive.m_aRescueCommandReceipts)
			return 0;
		int count;
		foreach (HST_RescueCommandReceiptState receipt : captive.m_aRescueCommandReceipts)
		{
			if (receipt && receipt.m_sResult.StartsWith("rejected:"))
				count++;
		}
		return count;
	}

	protected bool DiscardOldestRejectedCommandReceipt(HST_MissionAssetState captive)
	{
		if (!captive || !captive.m_aRescueCommandReceipts)
			return false;
		for (int receiptIndex = 0; receiptIndex < captive.m_aRescueCommandReceipts.Count(); receiptIndex++)
		{
			HST_RescueCommandReceiptState receipt = captive.m_aRescueCommandReceipts[receiptIndex];
			if (!receipt || !receipt.m_sResult.StartsWith("rejected:"))
				continue;
			captive.m_aRescueCommandReceipts.Remove(receiptIndex);
			return true;
		}
		return false;
	}

	protected bool IsTerminalCaptiveDisposition(HST_ERescueCaptiveDisposition disposition)
	{
		return disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED
			|| disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED;
	}

	protected bool IsAllowedCaptiveTransition(
		HST_ERescueCaptiveDisposition current,
		HST_ERescueCaptiveDisposition next)
	{
		if (next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED)
			return !IsTerminalCaptiveDisposition(current);
		if (current == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD)
			return next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED;
		if (current == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED)
			return next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING;
		if (current == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING)
			return next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
				|| next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED
				|| next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
				|| next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED
				|| next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED;
		if (current == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING)
			return next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
				|| next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED
				|| next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
		if (current == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED)
			return next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
				|| next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
				|| next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED
				|| next == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED;
		return false;
	}

	protected static bool IsEscortAuthorityValid(HST_CampaignState state, string identityId)
	{
		if (!state || identityId.IsEmpty())
			return false;
		HST_PlayerState escort = state.FindPlayer(identityId);
		if (!escort)
			return false;
		return escort.m_iLastSeenPlayerId > 0;
	}

	bool TickBeforeMissionRuntime(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar)
			return false;
		bool changed;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE)
				continue;
			if (operation.m_iContractVersion == QUARANTINED_CONTRACT_VERSION)
			{
				changed = ApplyQuarantineStatus(state, operation, operation.m_sLastProjectionReason) || changed;
				continue;
			}
			if (operation.m_iContractVersion != EXACT_CONTRACT_VERSION)
			{
				changed = QuarantineOperationAuthority(state, operation,
					"unsupported exact rescue contract version") || changed;
				continue;
			}
			if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				continue;
			HST_ActiveMissionState mission;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			string failure = ResolveRuntimeContext(state, operation, mission, manifest, batch, group);
			if (!failure.IsEmpty())
			{
				changed = QuarantineOperationAuthority(state, operation, failure) || changed;
				continue;
			}
			if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			HST_ZoneState zone = state.FindZone(operation.m_sAssignmentZoneId);
			if (!zone || zone.m_sOwnerFactionKey != operation.m_sOwnerFactionKey)
			{
				changed = RetireGuardSubgraph(state, operation, manifest, batch, group,
					"target ownership changed; captives remain under rescue authority") || changed;
				continue;
			}
			changed = TickGuardProjection(state, operation, mission, manifest, batch, group) || changed;
		}
		return changed;
	}

	bool TickAfterMissionRuntime(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return false;
		bool changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!IsExactMission(mission) || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			changed = TickAfterMissionRuntimeForMission(state, mission) || changed;
		}
		return changed;
	}

	// Phase 13 starts and probes a mission inside one runner callback, after the
	// normal after-runtime stage for that frame has already passed. Reuse the exact
	// per-mission production stage once, without advancing shared time or touching
	// another mission, then require read-only graph and native readiness evidence.
	bool ReconcileCampaignDebugMissionAfterRuntime(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string missionInstanceId,
		out string evidence)
	{
		evidence = "exact rescue Campaign Debug after-runtime reconciliation unavailable";
		if (!state || !preset || missionInstanceId.IsEmpty())
			return false;

		HST_ActiveMissionState mission = state.FindActiveMission(missionInstanceId);
		string graphBeforeEvidence;
		if (!ValidateCommittedGraphReadOnly(state, mission, graphBeforeEvidence, true))
		{
			evidence = "before | " + graphBeforeEvidence;
			return false;
		}

		bool changed = TickAfterMissionRuntimeForMission(state, mission);
		string graphAfterEvidence;
		bool graphExact = ValidateCommittedGraphReadOnly(
			state,
			mission,
			graphAfterEvidence,
			true);
		int captiveRows;
		int liveRuntimeRows;
		int liveHandles;
		string runtimeEvidence;
		bool runtimeExact = InspectLiveCaptiveRuntimeReadOnly(
			state,
			mission,
			captiveRows,
			liveRuntimeRows,
			liveHandles,
			runtimeEvidence);
		evidence = string.Format(
			"production after-runtime changed %1 | graph %2 | runtime %3 | %4 | %5",
			changed,
			graphExact,
			runtimeExact,
			graphAfterEvidence,
			runtimeEvidence);
		return graphExact && runtimeExact;
	}

	protected bool TickAfterMissionRuntimeForMission(
		HST_CampaignState state,
		HST_ActiveMissionState mission)
	{
		if (!state || !IsExactMission(mission)
			|| mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;

		bool changed;
		if (m_MissionRuntime)
			changed = m_MissionRuntime.TickExactRescueCaptiveActuators(state, mission) || changed;
		array<ref HST_MissionAssetState> captives = {};
		CollectExactCaptives(state, mission, captives);
		HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
		foreach (HST_MissionAssetState captive : captives)
		{
			if (!captive)
				continue;
			if (m_MissionRuntime && !IsTerminalCaptiveDisposition(captive.m_eRescueDisposition))
			{
				vector deathPosition;
				string deathEvidence;
				if (m_MissionRuntime.TryResolveExactRescueCaptiveDeathEvidence(
					state, mission, captive, deathPosition, deathEvidence))
				{
					captive.m_vCurrentPosition = deathPosition;
					captive.m_vLastKnownPosition = deathPosition;
					changed = MarkCaptiveDeathObserved(state, mission, captive, deathEvidence) || changed;
				}
			}
			changed = ReconcileDisconnectedEscort(state, mission, captive) || changed;
			changed = ReconcileObservedCarrierState(state, mission, captive) || changed;
			changed = ReconcileCaptiveProjectionChoice(state, mission, operation, captive) || changed;
			changed = ApplyCaptiveCompatibilityProjection(captive) || changed;
		}
		changed = SyncMissionCaptiveCounters(state, mission) || changed;
		changed = SyncMissionObjectiveProjection(state, mission) || changed;
		return changed;
	}

	protected bool ReconcileDisconnectedEscort(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState captive)
	{
		if (!state || !mission || !captive || mission.m_bRescueExtractionGrace)
			return false;
		bool custody = captive.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
			|| captive.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
			|| captive.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
		if (!custody || IsEscortAuthorityValid(state, captive.m_sRescueEscortIdentityId))
			return false;
		string previousEscortIdentityId = captive.m_sRescueEscortIdentityId;
		if (previousEscortIdentityId.IsEmpty())
			return false;
		HST_RescuePOWTransitionResult released = ApplyCaptiveTransition(
			state,
			mission,
			captive,
			HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED,
			previousEscortIdentityId,
			"system_disconnected_escort_release_" + captive.m_sAssetId);
		return released && released.m_bSuccess && released.m_bStateChanged;
	}

	protected bool ReconcileCaptiveProjectionChoice(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_MissionAssetState captive)
	{
		if (!state || !mission || !operation || !captive || !m_MissionRuntime)
			return false;
		bool wasSpawned = captive.m_bSpawned;
		int previousGeneration = captive.m_iRescueProjectionGeneration;
		if (IsTerminalCaptiveDisposition(captive.m_eRescueDisposition))
		{
			m_MissionRuntime.FoldExactRescueCaptiveProjection(
				state, mission, captive, "terminal exact rescue captive");
			return wasSpawned != captive.m_bSpawned
				|| previousGeneration != captive.m_iRescueProjectionGeneration;
		}
		bool custody = captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
			|| captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
			|| captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
		if (custody)
		{
			m_MissionRuntime.EnsureExactRescueCaptiveProjection(state, mission, captive);
			return wasSpawned != captive.m_bSpawned
				|| previousGeneration != captive.m_iRescueProjectionGeneration;
		}
		float bubble = Math.Max(100.0, HST_WorldPositionService.GetPlayerEventBubbleRadiusMeters());
		bool guardPhysical = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		bool nearPlayer = HST_WorldPositionService.IsPositionNearLivingPlayer(
			captive.m_vCurrentPosition, bubble);
		if (guardPhysical || nearPlayer)
			m_MissionRuntime.EnsureExactRescueCaptiveProjection(state, mission, captive);
		else
			m_MissionRuntime.FoldExactRescueCaptiveProjection(
				state, mission, captive, "exact rescue captive outside materialization bubble");
		return wasSpawned != captive.m_bSpawned
			|| previousGeneration != captive.m_iRescueProjectionGeneration;
	}

	string FindCompletedActiveMissionId(HST_CampaignState state)
	{
		if (!state)
			return "";
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (IsExactMission(mission) && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
				&& AreAllRequiredCaptivesExtracted(state, mission))
				return mission.m_sInstanceId;
		}
		return "";
	}

	string FindFailedActiveMissionId(HST_CampaignState state)
	{
		if (!state)
			return "";
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!IsExactMission(mission) || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			if (HasAnyRequiredCaptiveDeath(state, mission))
				return mission.m_sInstanceId;
			if (!mission.m_sRuntimeFailureReason.IsEmpty())
				return mission.m_sInstanceId;
			if (mission.m_bRescueExtractionGrace
				&& (!IsExtractionGraceCustodyValid(state, mission)
					|| state.m_iElapsedSeconds >= mission.m_iRescueGraceUntilSecond))
				return mission.m_sInstanceId;
		}
		return "";
	}

	protected bool TickGuardProjection(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (group.m_sRuntimeStatus == GUARD_ELIMINATED_STATUS
			|| group.m_sRuntimeStatus == "rescue_pow_guard_retired")
			return false;
		if (IsTerminalSpawnBatch(batch))
		{
			mission.m_sRuntimeFailureReason = "Exact rescue guard projection failed before guard retirement.";
			return true;
		}
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL)
			return TickVirtualGuard(state, operation, manifest, batch, group);
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
			return TickMaterializingGuard(state, operation, mission, manifest, batch, group);
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			return TickPhysicalGuard(state, operation, mission, manifest, batch, group);
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
			return ContinueGuardFold(state, operation, manifest, batch, group,
				operation.m_sLastProjectionReason);
		return QuarantineOperationAuthority(state, operation,
			"exact rescue guard materialization authority is invalid");
	}

	protected bool TickVirtualGuard(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!batch.m_bStrategicProjectionHeld)
			return QuarantineOperationAuthority(state, operation,
				"virtual exact rescue guard batch is not strategically held");
		int living = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
		SyncGroupRoster(group, living);
		operation.m_iLastVirtualFriendlyCount = living;
		if (living <= 0)
		{
			HST_ForceSpawnQueueCallbackResult eliminated = m_SpawnQueue.CompleteStrategicProjectionElimination(
				state.m_aForceSpawnResults, manifest, batch.m_sResultId, batch.m_sProjectionId,
				Math.Max(0, state.m_iElapsedSeconds), "exact rescue guard roster eliminated");
			if (!eliminated || !eliminated.m_bAccepted)
				return QuarantineOperationAuthority(state, operation,
					"exact rescue guard elimination could not be recorded");
			return MarkGuardEliminated(state, operation, group,
				"all exact rescue guards were eliminated");
		}
		HST_OperationProjectionDecision decision = m_Materialization.EvaluateExactPlayerQRF(
			operation, operation.m_vStrategicPosition);
		bool changed = RecordProjectionDecision(state, operation, decision);
		if (decision.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE)
			return BeginGuardMaterialization(state, operation, manifest, batch, group,
				decision.m_sReason) || changed;
		group.m_sRuntimeStatus = GUARD_VIRTUAL_STATUS;
		return changed;
	}

	// Restart verification has no player bubble by design. This narrow seam
	// validates the already-restored exact rescue graph, then delegates to the
	// same production release transition used by TickVirtualGuard. It cannot
	// create, repair, reset, or otherwise bypass durable operation authority.
	bool PrepareOrdinaryCampaignMixedNativeRestoredGuardProof(
		HST_CampaignState state,
		string missionInstanceId,
		out string evidence)
	{
		evidence
			= "ordinary mixed-native restored rescue guard is not ready";
		if (!state || !state.m_bRestoredFromPersistence
			|| missionInstanceId.IsEmpty())
			return false;
		HST_ActiveMissionState mission
			= state.FindActiveMission(missionInstanceId);
		if (!mission || !IsExactMission(mission)
			|| mission.m_eStatus
				!= HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;
		HST_OperationRecordState operation
			= state.FindOperation(mission.m_sOperationId);
		HST_ForceManifestState manifest;
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		string failure = ResolveRuntimeContext(
			state,
			operation,
			mission,
			manifest,
			batch,
			group);
		if (!failure.IsEmpty()
			|| operation.m_eSettlementState
				!= HST_EOperationSettlementState
					.HST_OPERATION_SETTLEMENT_OPEN)
		{
			evidence = "ordinary mixed-native restored rescue graph rejected: "
				+ failure;
			return false;
		}
		if (operation.m_eMaterializationState
				== HST_EOperationMaterializationState
					.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			|| operation.m_eMaterializationState
				== HST_EOperationMaterializationState
					.HST_OPERATION_MATERIALIZATION_PHYSICAL)
		{
			evidence
				= "ordinary mixed-native restored rescue guard release is already in progress";
			return true;
		}
		if (operation.m_eMaterializationState
				!= HST_EOperationMaterializationState
					.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| !batch.m_bStrategicProjectionHeld
			|| group.m_bSpawnedEntity
			|| m_PhysicalWar.GetForceSpawnGroupRoot(group)
			|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) != 0
			|| m_SpawnAdapter.CountHandlesForProjection(
				operation.m_sProjectionId) != 0
			|| m_SpawnQueue.CountStrategicLivingMemberSlots(batch) <= 0)
			return false;
		bool transitionChanged = BeginGuardMaterialization(
			state,
			operation,
			manifest,
			batch,
			group,
			"ordinary mixed-native restart verification");
		if (!transitionChanged
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| operation.m_eMaterializationState
				!= HST_EOperationMaterializationState
					.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			|| batch.m_bStrategicProjectionHeld)
		{
			evidence
				= "ordinary mixed-native restored rescue guard release postcondition rejected";
			return false;
		}
		evidence
			= "ordinary mixed-native restored rescue guard entered the production materialization path";
		return true;
	}

	protected bool BeginGuardMaterialization(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		HST_ForceSpawnQueueCallbackResult released = m_SpawnQueue.ReleaseStrategicProjectionForMaterialization(
			state.m_aForceSpawnResults, manifest, batch.m_sResultId, batch.m_sProjectionId,
			nowSecond, nowSecond + DEPLOYMENT_GRACE_SECONDS);
		if (!released || !released.m_bAccepted)
			return QuarantineOperationAuthority(state, operation,
				"exact rescue guard materialization release failed");
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iLastProjectionDecisionSecond = nowSecond;
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		group.m_vPosition = operation.m_vStrategicPosition;
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		group.m_vTargetPosition = operation.m_vAssignmentPosition;
		group.m_sRuntimeStatus = "rescue_pow_guard_materializing";
		group.m_iLifecycleRevision++;
		return true;
	}

	protected bool TickMaterializingGuard(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (IsTerminalSpawnBatch(batch) || group.m_sRuntimeStatus == "spawn_failed")
		{
			mission.m_sRuntimeFailureReason = "Exact rescue guard materialization failed.";
			return true;
		}
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| !group.m_bSpawnedEntity)
			return false;
		int living;
		string rosterFailure;
		if (!ReconcileGuardRoster(state, operation, batch, group, living, rosterFailure))
			return QuarantineOperationAuthority(state, operation,
				"exact rescue guard materialization reconciliation failed: " + rosterFailure);
		if (living <= 0)
			return FinalizePhysicalGuardElimination(state, operation, group);
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_iRevision++;
		group.m_sRuntimeStatus = GUARD_PHYSICAL_STATUS;
		group.m_iLifecycleRevision++;
		if (!m_PhysicalWar.RestartExactMissionGuardInfantryAssignment(
			state, group, operation.m_vAssignmentPosition,
			"Exact rescue guards materialized at their persisted assignment."))
		{
			mission.m_sRuntimeFailureReason = "Exact rescue guard assignment restart failed.";
			return true;
		}
		return true;
	}

	protected bool TickPhysicalGuard(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			mission.m_sRuntimeFailureReason = "Physical exact rescue guard lost successful batch authority.";
			return true;
		}
		if (group.m_sRuntimeStatus.Contains("runtime_binding_missing"))
			return QuarantineOperationAuthority(state, operation,
				"exact rescue guard binding disappeared without casualty proof");
		int living;
		string rosterFailure;
		if (!ReconcileGuardRoster(state, operation, batch, group, living, rosterFailure))
			return QuarantineOperationAuthority(state, operation,
				"exact rescue guard physical reconciliation failed: " + rosterFailure);
		if (living <= 0 || group.m_sRuntimeStatus == "eliminated")
			return FinalizePhysicalGuardElimination(state, operation, group);
		vector livePosition;
		string liveEvidence;
		if (!m_PhysicalWar.TryResolveExactMissionGuardLivePosition(
			state, group, livePosition, liveEvidence))
			return QuarantineOperationAuthority(state, operation,
				"exact rescue guard live position is unavailable: " + liveEvidence);
		operation.m_vStrategicPosition = livePosition;
		operation.m_iLastProgressAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iRevision++;
		group.m_vPosition = livePosition;
		group.m_vSourcePosition = livePosition;
		group.m_vTargetPosition = operation.m_vAssignmentPosition;
		bool changed = UpdateGuardEngagement(state, operation, group);
		HST_OperationProjectionDecision decision = m_Materialization.EvaluateExactPlayerQRF(operation, livePosition);
		changed = RecordProjectionDecision(state, operation, decision) || changed;
		if (decision.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_DEMATERIALIZE)
			return TryFoldGuard(state, operation, manifest, batch, group, decision.m_sReason) || changed;
		return true;
	}

	protected bool ReconcileGuardRoster(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		out int living,
		out string failure)
	{
		living = 0;
		failure = "";
		if (!state || !operation || !batch || !group || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar)
		{
			failure = "exact rescue guard roster services are unavailable";
			return false;
		}
		HST_ForceSpawnAdapterTickResult reconciled = m_SpawnAdapter.ReconcileExactInfantryProjectionAuthority(
			state, m_SpawnQueue, m_PhysicalWar, Math.Max(0, state.m_iElapsedSeconds),
			operation.m_sProjectionId);
		if (!reconciled || reconciled.m_iFailedCount > 0)
		{
			failure = "adapter reconciliation rejected exact rescue guard projection";
			return false;
		}
		living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
		SyncGroupRoster(group, living);
		operation.m_iLastVirtualFriendlyCount = living;
		if (living <= 0)
			return true;
		if (!group.m_bSpawnedEntity || group.m_sRuntimeStatus == "spawn_failed")
		{
			failure = "durable rescue guard roster has no physical root";
			return false;
		}
		string bindingFailure;
		if (!m_SpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
			state, batch, m_SpawnQueue, m_PhysicalWar, bindingFailure))
		{
			failure = "living rescue guard bindings are incomplete: " + bindingFailure;
			return false;
		}
		return true;
	}

	protected bool UpdateGuardEngagement(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group)
	{
		if (!state || !operation || !group)
			return false;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		string evidence;
		bool contact = m_PhysicalWar.HasExactMissionGuardLiveContactEvidence(
			state, group, CONTACT_EVIDENCE_RADIUS_METERS, evidence);
		bool recentCasualty = group.m_iLastCasualtySecond > 0
			&& nowSecond - group.m_iLastCasualtySecond <= CONTACT_CLEAR_SECONDS;
		if (contact || recentCasualty)
		{
			bool changed = operation.m_eEngagementMode
				!= HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT;
			operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT;
			operation.m_iEngagementStateEnteredAtSecond = nowSecond;
			operation.m_iLastContactAtSecond = nowSecond;
			if (changed)
				operation.m_iRevision++;
			return changed;
		}
		if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
			|| nowSecond - operation.m_iLastContactAtSecond <= CONTACT_CLEAR_SECONDS)
			return false;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_iRevision++;
		return true;
	}

	protected bool TryFoldGuard(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		HST_ForceSpawnQueueCallbackResult preflight = m_SpawnQueue.CanRequeueSuccessfulProjectionForStrategicHold(
			state.m_aForceSpawnResults, manifest, batch.m_sResultId, batch.m_sProjectionId,
			nowSecond, nowSecond + DEPLOYMENT_GRACE_SECONDS);
		if (!preflight || !preflight.m_bAccepted)
			return false;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		group.m_sRuntimeStatus = GUARD_FOLD_PENDING_STATUS;
		group.m_iLifecycleRevision++;
		return ContinueGuardFold(state, operation, manifest, batch, group, reason);
	}

	protected bool ContinueGuardFold(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		if (!batch.m_bStrategicProjectionHeld)
		{
			int living;
			string rosterFailure;
			if (!ReconcileGuardRoster(state, operation, batch, group, living, rosterFailure))
				return QuarantineOperationAuthority(state, operation,
					"exact rescue guard fold reconciliation failed: " + rosterFailure);
			if (living <= 0)
				return FinalizePhysicalGuardElimination(state, operation, group);
			UpdateGuardEngagement(state, operation, group);
			if (operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
				return false;
			vector foldedPosition;
			string liveEvidence;
			if (!m_PhysicalWar.TryResolveExactMissionGuardLivePosition(
				state, group, foldedPosition, liveEvidence))
				return QuarantineOperationAuthority(state, operation,
					"exact rescue guard fold position is unavailable: " + liveEvidence);
			HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
				state, m_PhysicalWar, operation.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return false;
			int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
			HST_ForceSpawnQueueCallbackResult held = m_SpawnQueue.RequeueSuccessfulProjectionForStrategicHold(
				state.m_aForceSpawnResults, manifest, batch.m_sResultId, batch.m_sProjectionId,
				nowSecond, nowSecond + DEPLOYMENT_GRACE_SECONDS);
			if (!held || !held.m_bAccepted)
				return QuarantineOperationAuthority(state, operation,
					"exact rescue guard survivors could not enter strategic hold");
			operation.m_vStrategicPosition = foldedPosition;
			group.m_vPosition = foldedPosition;
		}
		ClearGroupProcessAuthority(group);
		int virtualLiving = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
		SyncGroupRoster(group, virtualLiving);
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		group.m_vTargetPosition = operation.m_vAssignmentPosition;
		group.m_sRuntimeStatus = GUARD_VIRTUAL_STATUS;
		group.m_iLifecycleRevision++;
		int transitionSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iMaterializationStateEnteredAtSecond = transitionSecond;
		operation.m_iEngagementStateEnteredAtSecond = transitionSecond;
		operation.m_iStrategicLastUpdateSecond = transitionSecond;
		operation.m_iLastVirtualFriendlyCount = virtualLiving;
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		return true;
	}

	protected bool FinalizePhysicalGuardElimination(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group)
	{
		string eliminationFailure;
		if (!m_PhysicalWar.FinalizeEliminatedForceSpawnProjection(
			state, group, Math.Max(0, state.m_iElapsedSeconds), eliminationFailure))
			return QuarantineOperationAuthority(state, operation,
				"exact rescue guard elimination cleanup is unresolved: " + eliminationFailure);
		return MarkGuardEliminated(state, operation, group,
			"all exact rescue guards were eliminated physically");
	}

	protected bool MarkGuardEliminated(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		string reason)
	{
		if (!state || !operation || !group)
			return false;
		ClearGroupProcessAuthority(group);
		SyncGroupRoster(group, 0);
		group.m_sRuntimeStatus = GUARD_ELIMINATED_STATUS;
		group.m_iEliminatedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		group.m_iLifecycleRevision++;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iLastVirtualFriendlyCount = 0;
		operation.m_sLastProjectionReason = reason + "; rescue remains open";
		operation.m_iMaterializationStateEnteredAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iRevision++;
		return true;
	}

	protected bool RetireGuardSubgraph(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		if (!state || !operation || !manifest || !batch || !group)
			return false;
		if (group.m_sRuntimeStatus == "rescue_pow_guard_retired")
			return false;
		if (m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0
			|| group.m_bSpawnedEntity || m_PhysicalWar.GetForceSpawnGroupRoot(group)
			|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0)
		{
			HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
				state, m_PhysicalWar, operation.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return false;
		}
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		HST_ForceSpawnQueueCallbackResult terminal;
		bool requiresTerminalReceipt = batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| !IsTerminalSpawnBatch(batch);
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			terminal = m_SpawnQueue.CompleteQuarantinedSuccessfulProjectionCancellation(
				state.m_aForceSpawnResults, batch.m_sResultId, batch.m_sProjectionId, nowSecond, reason);
		else if (!IsTerminalSpawnBatch(batch))
			terminal = m_SpawnQueue.RequestCancel(state.m_aForceSpawnResults,
				batch.m_sResultId, nowSecond, reason);
		if (requiresTerminalReceipt && (!terminal || !terminal.m_bAccepted))
			return false;
		ClearGroupProcessAuthority(group);
		SyncGroupRoster(group, 0);
		group.m_sRuntimeStatus = "rescue_pow_guard_retired";
		group.m_iLifecycleRevision++;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iLastVirtualFriendlyCount = 0;
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		return true;
	}

	protected bool RecordProjectionDecision(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_OperationProjectionDecision decision)
	{
		if (!state || !operation || !decision)
			return false;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		if (operation.m_sLastProjectionReason == decision.m_sReason
			&& operation.m_iLastProjectionDecisionSecond == nowSecond)
			return false;
		operation.m_sLastProjectionReason = decision.m_sReason;
		operation.m_iLastProjectionDecisionSecond = nowSecond;
		operation.m_iRevision++;
		return true;
	}

	protected void SyncGroupRoster(HST_ActiveGroupState group, int living)
	{
		if (!group)
			return;
		living = Math.Max(0, living);
		group.m_iInfantryCount = living;
		group.m_iLastSeenAliveCount = living;
		group.m_iSurvivorInfantryCount = living;
		group.m_iDurableLivingInfantryCount = living;
		group.m_iVehicleCount = 0;
		group.m_iSurvivorVehicleCount = 0;
	}

	protected void ClearGroupProcessAuthority(HST_ActiveGroupState group)
	{
		if (!group)
			return;
		group.m_sRuntimeEntityId = "";
		group.m_bSpawnedEntity = false;
		group.m_bSpawnAttempted = false;
		group.m_bSpawnCompleted = false;
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
	}

	protected bool IsTerminalSpawnBatch(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return true;
		return batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED;
	}

	protected bool ReconcileObservedCarrierState(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState captive)
	{
		if (!state || !mission || !captive || !m_MissionRuntime
			|| IsTerminalCaptiveDisposition(captive.m_eRescueDisposition))
			return false;
		if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING)
		{
			IEntity carrierEntity;
			string carrierVehicleId;
			vector carrierPosition;
			string evidence;
			if (!m_MissionRuntime.ResolveExactRescueEscortCarrierEvidence(
				state, mission, captive, carrierEntity, carrierVehicleId, carrierPosition, evidence))
				return false;
			HST_RescuePOWTransitionResult boarding = ApplyCaptiveTransition(
				state, mission, captive,
				HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING,
				captive.m_sRescueEscortIdentityId,
				string.Format("rescue_carrier_observed_%1_%2", captive.m_sAssetId, captive.m_iRescueRevision + 1),
				carrierVehicleId);
			if (boarding && boarding.m_bSuccess)
			{
				captive.m_vCurrentPosition = carrierPosition;
				captive.m_vLastKnownPosition = carrierPosition;
			}
			return boarding && boarding.m_bStateChanged;
		}
		if (captive.m_eRescueDisposition != HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
			&& captive.m_eRescueDisposition != HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED)
			return false;
		IEntity verifiedCarrier;
		string seatToken;
		vector verifiedPosition;
		string carrierEvidence;
		if (!m_MissionRuntime.ResolveExactRescueCarrierEvidence(
			state, mission, captive, verifiedCarrier, seatToken, verifiedPosition, carrierEvidence))
		{
			if (Math.Max(0, state.m_iElapsedSeconds) - captive.m_iRescueTransitionSecond
				<= CARRIER_EVIDENCE_GRACE_SECONDS)
				return false;
			HST_RescuePOWTransitionResult fallback = ApplyCaptiveTransition(
				state, mission, captive,
				HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING,
				captive.m_sRescueEscortIdentityId,
				string.Format("rescue_carrier_unavailable_%1_%2", captive.m_sAssetId,
					captive.m_iRescueRevision + 1));
			return fallback && fallback.m_bStateChanged;
		}
		captive.m_vCurrentPosition = verifiedPosition;
		captive.m_vLastKnownPosition = verifiedPosition;
		if (seatToken.IsEmpty())
		{
			if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING)
			{
				if (Math.Max(0, state.m_iElapsedSeconds) - captive.m_iRescueTransitionSecond
					<= CARRIER_EVIDENCE_GRACE_SECONDS)
					return false;
				HST_RescuePOWTransitionResult followFallback = ApplyCaptiveTransition(
					state, mission, captive,
					HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING,
					captive.m_sRescueEscortIdentityId,
					string.Format("rescue_seat_unavailable_%1_%2", captive.m_sAssetId,
						captive.m_iRescueRevision + 1));
				return followFallback && followFallback.m_bStateChanged;
			}
			HST_RescuePOWTransitionResult pending = ApplyCaptiveTransition(
				state, mission, captive,
				HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING,
				captive.m_sRescueEscortIdentityId,
				string.Format("rescue_seat_lost_%1_%2", captive.m_sAssetId, captive.m_iRescueRevision + 1),
				captive.m_sRescueCarrierVehicleId);
			return pending && pending.m_bStateChanged;
		}
		if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED
			&& captive.m_sRescueCarrierSeatToken == seatToken)
			return false;
		if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED)
		{
			captive.m_sRescueCarrierSeatToken = seatToken;
			captive.m_iRescueTransitionSecond = Math.Max(0, state.m_iElapsedSeconds);
			captive.m_iRescueRevision++;
			ApplyCaptiveCompatibilityProjection(captive);
			return true;
		}
		HST_RescuePOWTransitionResult boarded = ApplyCaptiveTransition(
			state, mission, captive,
			HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED,
			captive.m_sRescueEscortIdentityId,
			string.Format("rescue_seat_observed_%1_%2", captive.m_sAssetId, captive.m_iRescueRevision + 1),
			captive.m_sRescueCarrierVehicleId, seatToken);
		return boarded && boarded.m_bStateChanged;
	}

	protected bool SyncMissionObjectiveProjection(
		HST_CampaignState state,
		HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;
		int extracted = Math.Max(0, mission.m_iExtractedCaptiveCount);
		bool complete = extracted == EXACT_CAPTIVE_COUNT && AreAllRequiredCaptivesExtracted(state, mission);
		bool changed;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (objective.m_iRequiredCount != EXACT_CAPTIVE_COUNT
				|| objective.m_iCurrentCount != extracted
				|| objective.m_iRequiredProgress != EXACT_CAPTIVE_COUNT
				|| objective.m_iCurrentProgress != extracted
				|| objective.m_bComplete != complete)
				changed = true;
			objective.m_iRequiredCount = EXACT_CAPTIVE_COUNT;
			objective.m_iCurrentCount = extracted;
			objective.m_iRequiredProgress = EXACT_CAPTIVE_COUNT;
			objective.m_iCurrentProgress = extracted;
			objective.m_bComplete = complete;
		}
		return changed;
	}

	protected float Distance2D(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}

	protected static bool IsZeroVector(vector value)
	{
		return Math.AbsFloat(value[0]) < 0.01
			&& Math.AbsFloat(value[1]) < 0.01
			&& Math.AbsFloat(value[2]) < 0.01;
	}

	bool ReconcileAfterMissionOutcomes(HST_CampaignState state)
	{
		if (!state)
			return false;
		bool changed;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE)
				continue;
			HST_ActiveMissionState mission = state.FindActiveMission(operation.m_sMissionInstanceId);
			changed = ReconcileMissionOutcomeForOperation(state, operation, mission) || changed;
		}
		return changed;
	}

	bool ReconcileMissionOutcomeForInstance(HST_CampaignState state, string instanceId)
	{
		HST_ActiveMissionState mission;
		HST_OperationRecordState operation;
		if (!ResolveExactInstanceOperation(state, instanceId, mission, operation))
			return false;
		return ReconcileMissionOutcomeForOperation(state, operation, mission);
	}

	protected bool ReconcileMissionOutcomeForOperation(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission)
	{
		if (!state || !operation
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE)
			return false;
		if (operation.m_iContractVersion == QUARANTINED_CONTRACT_VERSION)
			return ApplyQuarantineStatus(state, operation, operation.m_sLastProjectionReason);
		if (operation.m_iContractVersion != EXACT_CONTRACT_VERSION)
			return false;
		if (!mission)
			return QuarantineOperationAuthority(state, operation,
				"exact rescue lost its mission outcome authority");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (!mission.m_sSettlementId.IsEmpty())
				return false;
			mission.m_sSettlementId = operation.m_sSettlementId;
			return true;
		}
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;
		HST_ForceManifestState manifest;
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		string failure = ResolveRuntimeContext(state, operation, mission, manifest, batch, group);
		if (!failure.IsEmpty())
			return QuarantineOperationAuthority(state, operation, failure);
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_AVAILABLE)
			return QuarantineOperationAuthority(state, operation,
				"exact rescue outcome retained an unavailable mission status");
		bool changed;
		bool allExtracted = AreAllRequiredCaptivesExtracted(state, mission);
		bool captiveDeath = HasAnyRequiredCaptiveDeath(state, mission);
		if (allExtracted && mission.m_eStatus != HST_EMissionStatus.HST_MISSION_SUCCEEDED)
			return QuarantineOperationAuthority(state, operation,
				"exact rescue outcome has complete extraction authority without success");
		if (captiveDeath && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED)
		{
			mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
			mission.m_sRuntimePhase = "failed";
			changed = true;
		}
		HST_EOperationTerminalResult terminal = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED;
		string detail = "mission_failed";
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED)
		{
			if (!allExtracted || captiveDeath)
				return QuarantineOperationAuthority(state, operation,
					"succeeded exact rescue lacks three extraction receipts") || changed;
			terminal = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED;
			detail = "mission_completed";
		}
		else if (captiveDeath)
		{
			terminal = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED;
			detail = "required_captive_killed";
		}
		else if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED)
			detail = "mission_expired";
		return RetireAndSettle(state, operation, mission, manifest, batch, group,
			terminal, detail, "mission outcome ended exact rescue authority") || changed;
	}

	bool SettleOpenOperationsForCampaignStop(HST_CampaignState state, string reason)
	{
		if (!state)
			return false;
		bool changed;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
				|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				continue;
			HST_ActiveMissionState mission;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			string failure = ResolveRuntimeContext(state, operation, mission, manifest, batch, group);
			if (!failure.IsEmpty())
			{
				changed = QuarantineOperationAuthority(state, operation, failure) || changed;
				continue;
			}
			bool captiveDeath = HasAnyRequiredCaptiveDeath(state, mission);
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
			{
				mission.m_iRemainingSeconds = 0;
				mission.m_sRuntimeFailureReason = reason;
				if (captiveDeath)
				{
					mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
					mission.m_sRuntimePhase = "failed";
				}
				else
				{
					mission.m_eStatus = HST_EMissionStatus.HST_MISSION_EXPIRED;
					mission.m_sRuntimePhase = "expired";
				}
				changed = true;
			}
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_AVAILABLE)
			{
				changed = QuarantineOperationAuthority(state, operation,
					"campaign stop found unavailable exact rescue outcome authority") || changed;
				continue;
			}
			bool allExtracted = AreAllRequiredCaptivesExtracted(state, mission);
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED
				&& (!allExtracted || captiveDeath))
			{
				changed = QuarantineOperationAuthority(state, operation,
					"campaign stop found contradictory exact rescue success authority") || changed;
				continue;
			}
			if (allExtracted && mission.m_eStatus != HST_EMissionStatus.HST_MISSION_SUCCEEDED)
			{
				changed = QuarantineOperationAuthority(state, operation,
					"campaign stop found complete extraction without exact rescue success") || changed;
				continue;
			}
			if (captiveDeath && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED)
			{
				mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
				mission.m_sRuntimePhase = "failed";
				changed = true;
			}
			HST_EOperationTerminalResult terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED;
			string terminalDetail = "campaign_stopped";
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED
				&& allExtracted)
			{
				terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED;
				terminalDetail = "mission_completed";
			}
			else if (captiveDeath)
			{
				terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED;
				terminalDetail = "required_captive_killed";
			}
			changed = RetireAndSettle(state, operation, mission, manifest, batch, group,
				terminalResult, terminalDetail, reason) || changed;
		}
		return changed;
	}

	protected bool RetireAndSettle(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_EOperationTerminalResult terminalResult,
		string detail,
		string reason)
	{
		if (!state || !operation || !mission || !manifest || !batch || !group)
			return false;
		array<ref HST_MissionAssetState> captives = {};
		CollectExactCaptives(state, mission, captives);
		if (captives.Count() != EXACT_CAPTIVE_COUNT)
			return QuarantineOperationAuthority(state, operation,
				"exact rescue settlement lost captive authority");
		foreach (HST_MissionAssetState captive : captives)
		{
			bool custody = captive.m_eRescueDisposition
					== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
				|| captive.m_eRescueDisposition
					== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
				|| captive.m_eRescueDisposition
					== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
			if (!custody)
				continue;
			HST_RescuePOWTransitionResult released = ApplyCaptiveTransition(
				state,
				mission,
				captive,
				HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED,
				captive.m_sRescueEscortIdentityId,
				"system_terminal_custody_release_" + captive.m_sAssetId);
			if (!released || !released.m_bSuccess)
				return QuarantineOperationAuthority(state, operation,
					"exact rescue settlement could not release nonterminal custody authority");
		}
		if (m_MissionRuntime)
		{
			foreach (HST_MissionAssetState captive : captives)
			{
				m_MissionRuntime.FoldExactRescueCaptiveProjection(state, mission, captive, reason);
				if (!m_MissionRuntime.IsExactRescueCaptiveProjectionAbsent(captive)
					|| captive.m_bSpawned)
					return false;
			}
		}
		if (group.m_sRuntimeStatus != GUARD_ELIMINATED_STATUS
			&& group.m_sRuntimeStatus != "rescue_pow_guard_retired")
		{
			if (!RetireGuardSubgraph(state, operation, manifest, batch, group, reason))
				return false;
		}
		string settlementId = HST_OperationService.BuildSettlementId(operation.m_sOperationId, SETTLEMENT_KIND);
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return operation.m_sSettlementId == settlementId
				&& operation.m_eTerminalResult == terminalResult
				&& mission.m_sSettlementId == settlementId;
		if (terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN)
			return false;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		operation.m_eTerminalResult = terminalResult;
		operation.m_sSettlementId = settlementId;
		operation.m_sTerminalReason = reason;
		operation.m_sLastProjectionReason = SETTLEMENT_KIND + ": " + detail;
		operation.m_iDutyStateEnteredAtSecond = nowSecond;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iSettledAtSecond = nowSecond;
		operation.m_iRevision++;
		mission.m_sSettlementId = settlementId;
		mission.m_bRescueExtractionGrace = false;
		mission.m_iRescueGraceUntilSecond = 0;
		group.m_sRuntimeStatus = "rescue_pow_terminal_" + detail;
		group.m_iLifecycleRevision++;
		return true;
	}

	bool ReconcileSettledRuntimeCleanup(HST_CampaignState state)
	{
		if (!state || !m_SpawnAdapter || !m_PhysicalWar)
			return false;
		bool changed;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				continue;
			HST_ActiveMissionState mission = state.FindActiveMission(operation.m_sMissionInstanceId);
			changed = ReconcileSettledRuntimeCleanupForOperation(state, operation, mission) || changed;
		}
		return changed;
	}

	bool ReconcileSettledRuntimeCleanupForInstance(HST_CampaignState state, string instanceId)
	{
		HST_ActiveMissionState mission;
		HST_OperationRecordState operation;
		if (!ResolveExactInstanceOperation(state, instanceId, mission, operation))
			return false;
		return ReconcileSettledRuntimeCleanupForOperation(state, operation, mission);
	}

	protected bool ReconcileSettledRuntimeCleanupForOperation(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission)
	{
		if (!state || !operation || !mission || !m_SpawnAdapter || !m_PhysicalWar
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			|| mission.m_sSettlementId != operation.m_sSettlementId)
			return false;
		bool changed;
		bool captiveProcessAuthorityAbsent = true;
		foreach (HST_MissionAssetState captive : state.m_aMissionAssets)
		{
			if (!captive || captive.m_sMissionInstanceId != mission.m_sInstanceId
				|| captive.m_iRescueContractVersion != EXACT_CONTRACT_VERSION)
				continue;
			if (m_MissionRuntime)
				m_MissionRuntime.FoldExactRescueCaptiveProjection(
					state, mission, captive, "settled exact rescue cleanup");
			changed = ApplyCaptiveCompatibilityProjection(captive) || changed;
			if (captive.m_bSpawned)
				captiveProcessAuthorityAbsent = false;
			if (m_MissionRuntime && !m_MissionRuntime.IsExactRescueCaptiveProjectionAbsent(captive))
				captiveProcessAuthorityAbsent = false;
		}
		if (!captiveProcessAuthorityAbsent)
			return changed;
		foreach (HST_MissionRuntimeEntityState runtimeEntity : state.m_aMissionRuntimeEntities)
		{
			if (runtimeEntity && runtimeEntity.m_sMissionInstanceId == mission.m_sInstanceId
				&& runtimeEntity.m_bSpawned)
				return changed;
		}
		if (m_MissionRuntime
			&& m_MissionRuntime.CountRuntimeEntityHandlesForMission(
				state, mission.m_sInstanceId) > 0)
			return changed;
		if (m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0)
			return changed;
		if (!operation.m_sSpawnResultId.IsEmpty()
			&& m_SpawnAdapter.CountHandlesForResultId(operation.m_sSpawnResultId) > 0)
			return changed;
		HST_ActiveGroupState group = state.FindActiveGroup(operation.m_sGroupId);
		if (group)
		{
			if (group.m_bSpawnedEntity || group.m_iSpawnedAgentCount > 0
				|| group.m_iAssignedWaypointCount > 0 || !group.m_sRuntimeEntityId.IsEmpty())
				return changed;
			if (m_PhysicalWar.GetForceSpawnGroupRoot(group)
				|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0)
				return changed;
			if (!group.m_sProjectionId.IsEmpty()
				&& group.m_sProjectionId != operation.m_sProjectionId
				&& m_SpawnAdapter.CountHandlesForProjection(group.m_sProjectionId) > 0)
				return changed;
			if (!group.m_sSpawnResultId.IsEmpty()
				&& group.m_sSpawnResultId != operation.m_sSpawnResultId
				&& m_SpawnAdapter.CountHandlesForResultId(group.m_sSpawnResultId) > 0)
				return changed;
		}
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
		if (batch && !IsTerminalSpawnBatch(batch))
			return changed;
		if (batch)
		{
			state.m_aForceSpawnResults.Remove(state.m_aForceSpawnResults.Find(batch));
			changed = true;
		}
		if (group)
		{
			state.m_aActiveGroups.Remove(state.m_aActiveGroups.Find(group));
			changed = true;
		}
		if (mission.m_bRuntimeSpawned)
		{
			mission.m_bRuntimeSpawned = false;
			changed = true;
		}
		if (!mission.m_bRuntimeCleanupComplete)
		{
			mission.m_bRuntimeCleanupComplete = true;
			changed = true;
		}
		return changed;
	}

	protected string ResolveRuntimeContext(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		out HST_ActiveMissionState mission,
		out HST_ForceManifestState manifest,
		out HST_ForceSpawnResultState batch,
		out HST_ActiveGroupState group)
	{
		mission = null;
		manifest = null;
		batch = null;
		group = null;
		if (!state || !operation)
			return "exact rescue runtime context is missing";
		mission = state.FindActiveMission(operation.m_sMissionInstanceId);
		manifest = state.FindForceManifest(operation.m_sManifestId);
		batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
		group = state.FindActiveGroup(operation.m_sGroupId);
		return ValidateCommittedGraph(state, mission, operation, manifest, batch, group, false);
	}

	bool PreflightControlledShutdownPersistenceSample(
		HST_CampaignState state,
		out string evidence,
		bool requirePreparedAuthority = false)
	{
		if (m_bControlledShutdownPersistenceSampleApplied)
		{
			// A partially applied native fence must remain retryable. Preflight is
			// read-only and proves only the immutable DTO/object pins; the ordered
			// Prepare/Maintain step reapplies native pins before strict validation.
			return ValidateControlledShutdownPersistenceFence(
				state, evidence, false);
		}
		return ValidateControlledShutdownPersistenceCandidate(
			state, requirePreparedAuthority, evidence);
	}

	bool PrepareControlledShutdownPersistenceSample(
		HST_CampaignState state,
		out string evidence)
	{
		evidence = "exact rescue controlled-shutdown durable sample was rejected";
		if (m_bControlledShutdownPersistenceSampleApplied)
			return MaintainControlledShutdownPersistenceSample(state, evidence);
		if (!ValidateControlledShutdownNativeDomainFences(state, evidence)
			|| !ValidateControlledShutdownPersistenceCandidate(state, true, evidence)
			|| !BuildControlledShutdownPersistencePins(state, evidence))
			return false;

		// Publish the irreversible process-local latch only after every prepared
		// durable row and identity has been captured successfully. No reset path is
		// permitted after this point, including for an empty rescue scope.
		m_bControlledShutdownPersistenceSampleApplied = true;
		m_bControlledShutdownPersistenceSampleExact = false;
		return MaintainControlledShutdownPersistenceSample(state, evidence);
	}

	bool MaintainControlledShutdownPersistenceSample(
		HST_CampaignState state,
		out string evidence)
	{
		evidence = "exact rescue controlled-shutdown durable sample maintenance was rejected";
		if (!m_bControlledShutdownPersistenceSampleApplied)
			return false;
		m_bControlledShutdownPersistenceSampleExact = false;
		// First prove the immutable DTO/object pins without requiring native
		// quiescence, then reapply the pinned native fence. This makes the first
		// post-latch call and every partial retry converge through the same path.
		if (!ValidateControlledShutdownPersistenceFence(state, evidence, false)
			|| !ApplyControlledShutdownNativePins(state, evidence)
			|| !ValidateControlledShutdownPersistenceFence(state, evidence, true))
			return false;
		m_bControlledShutdownPersistenceSampleExact = true;
		evidence = string.Format(
			"exact rescue controlled-shutdown durable sample maintained for %1 operation(s)",
			m_aControlledShutdownPersistencePins.Count());
		return true;
	}

	bool HasControlledShutdownPersistenceSampleApplied()
	{
		return m_bControlledShutdownPersistenceSampleApplied;
	}

	bool IsControlledShutdownPersistenceSampleExact(HST_CampaignState state)
	{
		if (!m_bControlledShutdownPersistenceSampleApplied
			|| !m_bControlledShutdownPersistenceSampleExact)
			return false;
		string evidence;
		return ValidateControlledShutdownPersistenceFence(state, evidence);
	}

	protected bool IsOpenExactRescueOperation(HST_OperationRecordState operation)
	{
		return operation
			&& operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
			&& operation.m_iContractVersion == EXACT_CONTRACT_VERSION
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
	}

	protected bool ValidateNoOpenMaterializingExactRescue(
		HST_CampaignState state,
		out string evidence)
	{
		if (!state)
		{
			evidence = "exact rescue controlled-shutdown state is unavailable";
			return false;
		}
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!IsOpenExactRescueOperation(operation))
				continue;
			if (operation.m_eMaterializationState
				!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
				continue;
			evidence = "exact rescue persistence cannot sample MATERIALIZING authority";
			if (!operation.m_sOperationId.IsEmpty())
				evidence += ": " + operation.m_sOperationId;
			return false;
		}
		return true;
	}

	protected bool ValidateControlledShutdownPersistenceCandidate(
		HST_CampaignState state,
		bool requirePreparedAuthority,
		out string evidence)
	{
		evidence = "exact rescue controlled-shutdown durable sample preflight was rejected";
		if (!ValidateNoOpenMaterializingExactRescue(state, evidence))
			return false;

		int openOperationCount;
		array<IEntity> candidateCarriers = {};
		array<string> candidateCarrierIds = {};
		array<IEntity> candidateCaptives = {};
		array<IEntity> candidateCaptiveCarriers = {};
		array<BaseCompartmentSlot> candidateCaptiveSlots = {};
		array<bool> candidateCaptiveRequiresSeat = {};
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!IsOpenExactRescueOperation(operation))
				continue;
			openOperationCount++;
			if (!m_MissionRuntime)
			{
				evidence = "exact rescue controlled-shutdown mission runtime inspector is unavailable";
				return false;
			}

			HST_ActiveMissionState mission;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			string graphFailure = ResolveRuntimeContext(
				state, operation, mission, manifest, batch, group);
			if (!graphFailure.IsEmpty())
			{
				evidence = "exact rescue controlled-shutdown durable graph conflicts: " + graphFailure;
				return false;
			}
			if (mission.m_aGunShopItems
				&& mission.m_aGunShopItems.Count() != 0)
			{
				evidence
					= "exact rescue controlled-shutdown mission contains foreign gun-shop item rows";
				return false;
			}

			array<ref HST_MissionAssetState> captives = {};
			CollectExactCaptives(state, mission, captives);
			if (captives.Count() != EXACT_CAPTIVE_COUNT)
			{
				evidence = "exact rescue controlled-shutdown requires exactly three captive rows";
				return false;
			}
			array<int> ordinals = {};
			int extracted;
			int dead;
			foreach (HST_MissionAssetState captive : captives)
			{
				if (!IsExactRescueCaptiveAsset(state, captive)
					|| ordinals.Contains(captive.m_iRescueOrdinal))
				{
					evidence = "exact rescue controlled-shutdown captive identity or ordinal conflicts";
					return false;
				}
				ordinals.Insert(captive.m_iRescueOrdinal);

				IEntity captiveEntity;
				IEntity carrierEntity;
				BaseCompartmentSlot captiveSlot;
				string observedSeatToken;
				string topologyEvidence;
				if (!m_MissionRuntime.InspectExactRescueRuntimeTopologyReadOnly(
					state,
					mission,
					captive,
					captiveEntity,
					carrierEntity,
					captiveSlot,
					observedSeatToken,
					topologyEvidence,
					requirePreparedAuthority))
				{
					evidence = topologyEvidence;
					return false;
				}
				if (carrierEntity
					&& IsControlledShutdownRescuePlayerUsingCarrier(carrierEntity))
				{
					evidence
						= HST_PhysicalWarService.CONTROLLED_SHUTDOWN_PLAYER_RELEASE_EVIDENCE
						+ " | exact rescue mission carrier is player-occupied or a player is changing compartments";
					return false;
				}
				HST_MissionCaptiveFollowComponent followComponent;
				if (captiveEntity)
				{
					followComponent = HST_MissionCaptiveFollowComponent.Cast(
						captiveEntity.FindComponent(
							HST_MissionCaptiveFollowComponent));
					if (!followComponent
						|| !followComponent.PreflightControlledShutdownQuiescence(
							evidence))
					{
						if (evidence.IsEmpty())
						{
							evidence
								= "exact rescue controlled-shutdown live captive follower is not ready";
						}
						return false;
					}
					if (candidateCaptives.Find(captiveEntity) >= 0)
					{
						evidence
							= "exact rescue controlled-shutdown live captive identity is duplicated";
						return false;
					}
					candidateCaptives.Insert(captiveEntity);
					candidateCaptiveCarriers.Insert(carrierEntity);
					candidateCaptiveSlots.Insert(captiveSlot);
					candidateCaptiveRequiresSeat.Insert(captiveSlot != null);
				}
				if (carrierEntity)
				{
					int candidateCarrierIndex
						= candidateCarriers.Find(carrierEntity);
					if (candidateCarrierIndex < 0)
					{
						if (candidateCarrierIds.Contains(
							captive.m_sRescueCarrierVehicleId))
						{
							evidence
								= "exact rescue controlled-shutdown carrier runtime identity resolves to multiple roots";
							return false;
						}
						candidateCarriers.Insert(carrierEntity);
						candidateCarrierIds.Insert(
							captive.m_sRescueCarrierVehicleId);
					}
					else if (candidateCarrierIndex >= candidateCarrierIds.Count()
						|| candidateCarrierIds[candidateCarrierIndex]
							!= captive.m_sRescueCarrierVehicleId)
					{
						evidence
							= "exact rescue controlled-shutdown shared carrier identity conflicts";
						return false;
					}
				}

				HST_MissionRuntimeEntityState runtimeProjection;
				if (!TryResolveUniqueMissionRuntimeProjectionReadOnly(
					state, captive.m_sRescueProjectionId, runtimeProjection, evidence))
					return false;
				HST_RuntimeVehicleState carrierRecord;
				if (!TryResolveUniqueMissionCarrierReadOnly(
					state, captive.m_sRescueCarrierVehicleId, carrierRecord, evidence))
					return false;
				if (requirePreparedAuthority
					&& (!IsPreparedCaptiveCompatibilityExact(captive)
					|| !IsPreparedCaptiveRuntimeProjectionExact(
						mission, captive, runtimeProjection)))
				{
					evidence = "exact rescue controlled-shutdown captive durable preparation is not exact";
					return false;
				}
				if (captive.m_eRescueDisposition
					== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED)
					extracted++;
				else if (captive.m_eRescueDisposition
					== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED)
					dead++;
			}

			array<ref HST_MissionObjectiveState> objectives = {};
			if (!CollectControlledShutdownMissionObjectivesReadOnly(
				state, mission, objectives, evidence))
				return false;
			if (requirePreparedAuthority)
			{
				if (mission.m_iRequiredCaptiveCount != EXACT_CAPTIVE_COUNT
					|| mission.m_iExtractedCaptiveCount != extracted
					|| mission.m_iRuntimeDeliveryCount != extracted
					|| mission.m_iRuntimeDestroyedCount != dead)
				{
					evidence = "exact rescue controlled-shutdown prepared mission counters are not exact";
					return false;
				}
				bool objectiveComplete = extracted == EXACT_CAPTIVE_COUNT
					&& AreAllRequiredCaptivesExtracted(state, mission);
				foreach (HST_MissionObjectiveState objective : objectives)
				{
					if (objective.m_iRequiredCount != EXACT_CAPTIVE_COUNT
						|| objective.m_iCurrentCount != extracted
						|| objective.m_iRequiredProgress != EXACT_CAPTIVE_COUNT
						|| objective.m_iCurrentProgress != extracted
						|| objective.m_bComplete != objectiveComplete)
					{
						evidence = "exact rescue controlled-shutdown prepared objective projection is not exact";
						return false;
					}
				}
			}
		}
		if (!PreflightControlledShutdownCarrierScopesReadOnly(
			candidateCarriers,
			candidateCarrierIds,
			candidateCaptives,
			candidateCaptiveCarriers,
			candidateCaptiveSlots,
			candidateCaptiveRequiresSeat,
			evidence))
			return false;
		evidence = string.Format(
			"exact rescue controlled-shutdown durable sample preflight exact | prepared/scope %1/%2",
			requirePreparedAuthority,
			openOperationCount);
		return true;
	}

	protected bool BuildControlledShutdownPersistencePins(
		HST_CampaignState state,
		out string evidence)
	{
		evidence = "exact rescue controlled-shutdown durable sample pinning was rejected";
		if (!state || m_bControlledShutdownPersistenceSampleApplied)
			return false;
		m_ControlledShutdownPersistenceState = null;
		m_aControlledShutdownPersistencePins.Clear();
		m_aControlledShutdownCarrierPins.Clear();

		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!IsOpenExactRescueOperation(operation))
				continue;
			HST_ActiveMissionState mission;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			string graphFailure = ResolveRuntimeContext(
				state, operation, mission, manifest, batch, group);
			if (!graphFailure.IsEmpty())
			{
				m_aControlledShutdownPersistencePins.Clear();
				return false;
			}

			HST_ControlledShutdownRescueOperationSamplePin operationPin
				= new HST_ControlledShutdownRescueOperationSamplePin();
			operationPin.m_Operation = operation;
			operationPin.m_Mission = mission;
			operationPin.m_GuardGroup = group;
			operationPin.m_sOperationId = operation.m_sOperationId;
			operationPin.m_sMissionId = mission.m_sInstanceId;
			operationPin.m_sOperationSample = BuildControlledShutdownOperationSample(operation);
			operationPin.m_sMissionSample = BuildControlledShutdownMissionSample(mission);
			operationPin.m_sGuardSample = BuildControlledShutdownGroupSample(group);

			array<ref HST_MissionAssetState> captives = {};
			CollectExactCaptives(state, mission, captives);
			foreach (HST_MissionAssetState captive : captives)
			{
				HST_MissionRuntimeEntityState runtimeProjection;
				HST_RuntimeVehicleState carrierRecord;
				if (!TryResolveUniqueMissionRuntimeProjectionReadOnly(
					state, captive.m_sRescueProjectionId, runtimeProjection, evidence)
					|| !TryResolveUniqueMissionCarrierReadOnly(
						state, captive.m_sRescueCarrierVehicleId, carrierRecord, evidence))
				{
					m_aControlledShutdownPersistencePins.Clear();
					return false;
				}
				HST_ControlledShutdownRescueCaptiveSamplePin captivePin
					= new HST_ControlledShutdownRescueCaptiveSamplePin();
				captivePin.m_Captive = captive;
				captivePin.m_RuntimeProjection = runtimeProjection;
				captivePin.m_CarrierRecord = carrierRecord;
				captivePin.m_sAssetId = captive.m_sAssetId;
				captivePin.m_sProjectionId = captive.m_sRescueProjectionId;
				captivePin.m_sCarrierId = captive.m_sRescueCarrierVehicleId;
				captivePin.m_sCaptiveSample = BuildControlledShutdownCaptiveSample(captive);
				captivePin.m_sRuntimeProjectionSample
					= BuildControlledShutdownRuntimeProjectionSample(runtimeProjection);
				captivePin.m_sCarrierSample
					= BuildControlledShutdownCarrierSample(carrierRecord);

				IEntity captiveEntity;
				IEntity carrierEntity;
				BaseCompartmentSlot captiveSlot;
				string observedSeatToken;
				if (!m_MissionRuntime.InspectExactRescueRuntimeTopologyReadOnly(
					state,
					mission,
					captive,
					captiveEntity,
					carrierEntity,
					captiveSlot,
					observedSeatToken,
					evidence,
					true))
				{
					m_aControlledShutdownPersistencePins.Clear();
					return false;
				}
				captivePin.m_CaptiveEntity = captiveEntity;
				captivePin.m_CarrierEntity = carrierEntity;
				captivePin.m_CaptiveSlot = captiveSlot;
				captivePin.m_sObservedSeatToken = observedSeatToken;
				if (captiveEntity)
				{
					captivePin.m_FollowComponent
						= HST_MissionCaptiveFollowComponent.Cast(
							captiveEntity.FindComponent(
								HST_MissionCaptiveFollowComponent));
					if (!captivePin.m_FollowComponent)
					{
						evidence
							= "exact rescue controlled-shutdown live captive lacks its follower quiescence owner";
						m_aControlledShutdownPersistencePins.Clear();
						return false;
					}
					if (!captivePin.m_FollowComponent.PreflightControlledShutdownQuiescence(
						evidence))
					{
						m_aControlledShutdownPersistencePins.Clear();
						m_aControlledShutdownCarrierPins.Clear();
						return false;
					}
					captivePin.m_vCaptivePosition = captiveEntity.GetOrigin();
					captivePin.m_vCaptiveAngles
						= HST_WorldPositionService.BuildUprightAnglesFromVector(
							captiveEntity.GetYawPitchRoll());
				}
				operationPin.m_aCaptives.Insert(captivePin);
			}

			array<ref HST_MissionObjectiveState> objectives = {};
			if (!CollectControlledShutdownMissionObjectivesReadOnly(
				state, mission, objectives, evidence))
			{
				m_aControlledShutdownPersistencePins.Clear();
				return false;
			}
			foreach (HST_MissionObjectiveState objective : objectives)
			{
				operationPin.m_aObjectives.Insert(objective);
				operationPin.m_aObjectiveSamples.Insert(
					BuildControlledShutdownObjectiveSample(objective));
			}
			m_aControlledShutdownPersistencePins.Insert(operationPin);
		}

		foreach (HST_ControlledShutdownRescueOperationSamplePin pinnedOperation : m_aControlledShutdownPersistencePins)
		{
			foreach (HST_ControlledShutdownRescueCaptiveSamplePin pinnedCaptive : pinnedOperation.m_aCaptives)
			{
				if (!pinnedCaptive || !pinnedCaptive.m_CarrierEntity)
					continue;
				HST_ControlledShutdownRescueCarrierPin existingCarrierPin
					= FindControlledShutdownCarrierPin(pinnedCaptive.m_CarrierEntity);
				if (existingCarrierPin)
				{
					if (existingCarrierPin.m_Record != pinnedCaptive.m_CarrierRecord
						|| existingCarrierPin.m_sRuntimeId != pinnedCaptive.m_sCarrierId)
					{
						evidence
							= "exact rescue controlled-shutdown shared carrier identity conflicts";
						m_aControlledShutdownPersistencePins.Clear();
						m_aControlledShutdownCarrierPins.Clear();
						return false;
					}
					continue;
				}

				HST_ControlledShutdownRescueCarrierPin carrierPin
					= new HST_ControlledShutdownRescueCarrierPin();
				carrierPin.m_Entity = pinnedCaptive.m_CarrierEntity;
				carrierPin.m_Record = pinnedCaptive.m_CarrierRecord;
				carrierPin.m_sRuntimeId = pinnedCaptive.m_sCarrierId;
				carrierPin.m_Entity.GetTransform(carrierPin.m_aTransform);
				if (!CollectControlledShutdownCarrierCompartmentPins(
					carrierPin.m_Entity,
					carrierPin.m_aCompartmentSlots,
					carrierPin.m_aCompartmentOccupants,
					evidence))
				{
					m_aControlledShutdownPersistencePins.Clear();
					m_aControlledShutdownCarrierPins.Clear();
					return false;
				}
				m_aControlledShutdownCarrierPins.Insert(carrierPin);
			}
		}
		if (!ValidateControlledShutdownCarrierOccupantScope(evidence))
		{
			m_aControlledShutdownPersistencePins.Clear();
			m_aControlledShutdownCarrierPins.Clear();
			return false;
		}
		m_ControlledShutdownPersistenceState = state;
		evidence = string.Format(
			"exact rescue controlled-shutdown pinned %1 prepared operation(s) and %2 carrier root(s)",
			m_aControlledShutdownPersistencePins.Count(),
			m_aControlledShutdownCarrierPins.Count());
		return true;
	}

	protected bool ValidateControlledShutdownPersistenceFence(
		HST_CampaignState state,
		out string evidence,
		bool requireNativeQuiescence = true)
	{
		evidence = "exact rescue controlled-shutdown pinned durable authority changed";
		if (!m_bControlledShutdownPersistenceSampleApplied || !state
			|| state != m_ControlledShutdownPersistenceState
			|| !ValidateNoOpenMaterializingExactRescue(state, evidence))
			return false;

		int openOperationCount;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (IsOpenExactRescueOperation(operation))
				openOperationCount++;
		}
		if (openOperationCount != m_aControlledShutdownPersistencePins.Count())
		{
			evidence = "exact rescue controlled-shutdown open operation scope changed after pinning";
			return false;
		}

		array<string> operationIds = {};
		foreach (HST_ControlledShutdownRescueOperationSamplePin operationPin : m_aControlledShutdownPersistencePins)
		{
			if (!operationPin || operationPin.m_sOperationId.IsEmpty()
				|| operationIds.Contains(operationPin.m_sOperationId)
				|| !ValidateControlledShutdownOperationPin(
					state, operationPin, evidence, requireNativeQuiescence))
				return false;
			operationIds.Insert(operationPin.m_sOperationId);
		}
		if (requireNativeQuiescence
			&& (!ValidateControlledShutdownNativeDomainFences(state, evidence)
				|| !ValidateControlledShutdownCarrierPins(state, evidence)))
			return false;
		return true;
	}

	protected bool ValidateControlledShutdownOperationPin(
		HST_CampaignState state,
		HST_ControlledShutdownRescueOperationSamplePin pin,
		out string evidence,
		bool requireNativeQuiescence)
	{
		if (!pin.m_Operation || !pin.m_Mission || !pin.m_GuardGroup
			|| state.m_aOperations.Find(pin.m_Operation) < 0
			|| state.FindOperation(pin.m_sOperationId) != pin.m_Operation
			|| !IsOpenExactRescueOperation(pin.m_Operation)
			|| pin.m_sOperationSample
				!= BuildControlledShutdownOperationSample(pin.m_Operation))
			return false;

		HST_ActiveMissionState mission;
		HST_ForceManifestState manifest;
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		string graphFailure = ResolveRuntimeContext(
			state, pin.m_Operation, mission, manifest, batch, group);
		if (!graphFailure.IsEmpty() || mission != pin.m_Mission
			|| group != pin.m_GuardGroup || mission.m_sInstanceId != pin.m_sMissionId
			|| pin.m_sMissionSample != BuildControlledShutdownMissionSample(mission)
			|| pin.m_sGuardSample != BuildControlledShutdownGroupSample(group))
		{
			evidence = "exact rescue controlled-shutdown pinned operation graph or values changed";
			return false;
		}

		array<ref HST_MissionAssetState> captives = {};
		CollectExactCaptives(state, mission, captives);
		if (captives.Count() != pin.m_aCaptives.Count()
			|| pin.m_aCaptives.Count() != EXACT_CAPTIVE_COUNT)
		{
			evidence = "exact rescue controlled-shutdown pinned captive scope changed";
			return false;
		}
		array<string> assetIds = {};
		foreach (HST_ControlledShutdownRescueCaptiveSamplePin captivePin : pin.m_aCaptives)
		{
			if (!captivePin || !captivePin.m_Captive
				|| captivePin.m_sAssetId.IsEmpty()
				|| assetIds.Contains(captivePin.m_sAssetId)
				|| captives.Find(captivePin.m_Captive) < 0
				|| state.FindMissionAsset(captivePin.m_sAssetId) != captivePin.m_Captive
				|| captivePin.m_sProjectionId
					!= captivePin.m_Captive.m_sRescueProjectionId
				|| captivePin.m_sCarrierId
					!= captivePin.m_Captive.m_sRescueCarrierVehicleId
				|| captivePin.m_sCaptiveSample
					!= BuildControlledShutdownCaptiveSample(captivePin.m_Captive))
			{
				evidence = "exact rescue controlled-shutdown pinned captive identity or values changed";
				return false;
			}
			HST_MissionRuntimeEntityState runtimeProjection;
			HST_RuntimeVehicleState carrierRecord;
			if (!TryResolveUniqueMissionRuntimeProjectionReadOnly(
				state, captivePin.m_sProjectionId, runtimeProjection, evidence)
				|| runtimeProjection != captivePin.m_RuntimeProjection
				|| captivePin.m_sRuntimeProjectionSample
					!= BuildControlledShutdownRuntimeProjectionSample(runtimeProjection)
				|| !TryResolveUniqueMissionCarrierReadOnly(
					state, captivePin.m_sCarrierId, carrierRecord, evidence)
				|| carrierRecord != captivePin.m_CarrierRecord
				|| captivePin.m_sCarrierSample
					!= BuildControlledShutdownCarrierSample(carrierRecord))
			{
				evidence = "exact rescue controlled-shutdown pinned projection or carrier DTO changed";
				return false;
			}
			if (requireNativeQuiescence
				&& !ValidateControlledShutdownCaptiveNativePin(
					state,
					mission,
					captivePin,
					evidence))
				return false;
			assetIds.Insert(captivePin.m_sAssetId);
		}

		array<ref HST_MissionObjectiveState> objectives = {};
		if (!CollectControlledShutdownMissionObjectivesReadOnly(
			state, mission, objectives, evidence)
			|| objectives.Count() != pin.m_aObjectives.Count()
			|| pin.m_aObjectives.Count() != pin.m_aObjectiveSamples.Count())
		{
			evidence = "exact rescue controlled-shutdown pinned objective scope changed";
			return false;
		}
		for (int objectiveIndex = 0; objectiveIndex < pin.m_aObjectives.Count(); objectiveIndex++)
		{
			HST_MissionObjectiveState objective = pin.m_aObjectives[objectiveIndex];
			if (!objective || objectives.Find(objective) < 0
				|| pin.m_aObjectiveSamples[objectiveIndex]
					!= BuildControlledShutdownObjectiveSample(objective))
			{
				evidence = "exact rescue controlled-shutdown pinned objective identity or values changed";
				return false;
			}
		}
		return true;
	}

	protected bool ValidateControlledShutdownNativeDomainFences(
		HST_CampaignState state,
		out string evidence)
	{
		if (!state || !m_PhysicalWar
			|| !m_PhysicalWar.HasControlledShutdownActiveGroupQuiescenceApplied()
			|| !m_PhysicalWar.IsControlledShutdownActiveGroupQuiescenceExact(state))
		{
			evidence
				= "exact rescue controlled-shutdown guard native fence is not exact";
			return false;
		}
		return true;
	}

	protected HST_ControlledShutdownRescueCarrierPin FindControlledShutdownCarrierPin(
		IEntity carrier)
	{
		if (!carrier)
			return null;
		foreach (HST_ControlledShutdownRescueCarrierPin pin : m_aControlledShutdownCarrierPins)
		{
			if (pin && pin.m_Entity == carrier)
				return pin;
		}
		return null;
	}

	protected bool PreflightControlledShutdownCarrierScopesReadOnly(
		array<IEntity> carriers,
		array<string> carrierIds,
		array<IEntity> captives,
		array<IEntity> captiveCarriers,
		array<BaseCompartmentSlot> captiveSlots,
		array<bool> captiveRequiresSeat,
		out string evidence)
	{
		evidence
			= "exact rescue controlled-shutdown carrier scope preflight was rejected";
		if (!carriers || !carrierIds || !captives || !captiveCarriers
			|| !captiveSlots || !captiveRequiresSeat
			|| carriers.Count() != carrierIds.Count()
			|| captives.Count() != captiveCarriers.Count()
			|| captives.Count() != captiveSlots.Count()
			|| captives.Count() != captiveRequiresSeat.Count())
			return false;
		foreach (IEntity carrier : carriers)
		{
			int carrierIndex = carriers.Find(carrier);
			if (!carrier || !carrier.GetWorld() || carrierIndex < 0
				|| carrierIndex >= carrierIds.Count()
				|| carrierIds[carrierIndex].IsEmpty()
				|| IsControlledShutdownRescueRuntimeProxy(carrier))
				return false;
			if (IsControlledShutdownRescuePlayerUsingCarrier(carrier))
			{
				evidence
					= HST_PhysicalWarService.CONTROLLED_SHUTDOWN_PLAYER_RELEASE_EVIDENCE
					+ " | exact rescue mission carrier is player-occupied or a player is changing compartments";
				return false;
			}

			array<BaseCompartmentSlot> slots = {};
			array<IEntity> occupants = {};
			if (!CollectControlledShutdownCarrierCompartmentPins(
				carrier, slots, occupants, evidence))
				return false;
			array<IEntity> observedCaptives = {};
			for (int slotIndex; slotIndex < slots.Count(); slotIndex++)
			{
				BaseCompartmentSlot slot = slots[slotIndex];
				IEntity occupant = occupants[slotIndex];
				BaseCompartmentManagerComponent manager;
				if (slot)
					manager = slot.GetManager();
				if (!slot || slot.GetVehicle() != carrier || !manager
					|| !manager.GetOwner()
					|| !IsEntityWithinControlledShutdownCarrierRoot(
						manager.GetOwner(), carrier)
					|| slot.GetOccupant() != occupant)
				{
					evidence
						= "exact rescue controlled-shutdown carrier compartment topology is not read-only exact";
					return false;
				}
				if (!occupant)
					continue;
				int captiveIndex = captives.Find(occupant);
				if (captiveIndex < 0
					|| captiveIndex >= captiveCarriers.Count()
					|| captiveIndex >= captiveSlots.Count()
					|| captiveCarriers[captiveIndex] != carrier
					|| captiveSlots[captiveIndex] != slot
					|| observedCaptives.Find(occupant) >= 0)
				{
					evidence
						= "exact rescue controlled-shutdown carrier contains foreign or ambiguous live occupancy";
					return false;
				}
				observedCaptives.Insert(occupant);
			}
			for (int captiveIndex; captiveIndex < captives.Count(); captiveIndex++)
			{
				if (captiveCarriers[captiveIndex] != carrier)
					continue;
				// Prepared BOARDING authority deliberately has a carrier but no
				// occupied seat yet. Only topology that actually observed a seat
				// may require the captive to be present in the compartment scope.
				if (!captiveRequiresSeat[captiveIndex])
					continue;
				if (!captives[captiveIndex] || !captiveSlots[captiveIndex]
					|| observedCaptives.Find(captives[captiveIndex]) < 0)
				{
					evidence
						= "exact rescue controlled-shutdown captive is absent from its carrier seat scope";
					return false;
				}
			}
		}
		return true;
	}

	protected bool CollectControlledShutdownCarrierCompartmentPins(
		IEntity entity,
		array<BaseCompartmentSlot> slots,
		array<IEntity> occupants,
		out string evidence)
	{
		if (!entity || !entity.GetWorld() || !slots || !occupants
			|| slots.Count() != occupants.Count()
			|| IsControlledShutdownRescueRuntimeProxy(entity))
		{
			evidence
				= "exact rescue controlled-shutdown carrier compartment root is unavailable or proxy-local";
			return false;
		}

		array<Managed> managedComponents = {};
		int managerCount
			= entity.FindComponents(BaseCompartmentManagerComponent, managedComponents);
		if (managerCount != managedComponents.Count())
			return false;
		foreach (Managed managedComponent : managedComponents)
		{
			BaseCompartmentManagerComponent manager
				= BaseCompartmentManagerComponent.Cast(managedComponent);
			if (!manager || manager.GetOwner() != entity)
				return false;
			array<BaseCompartmentSlot> managedSlots = {};
			int managedSlotCount = manager.GetCompartments(managedSlots);
			if (managedSlotCount != managedSlots.Count())
				return false;
			foreach (BaseCompartmentSlot slot : managedSlots)
			{
				if (!slot || slot.GetManager() != manager || slots.Find(slot) >= 0)
					return false;
				slots.Insert(slot);
				occupants.Insert(slot.GetOccupant());
			}
		}

		IEntity child = entity.GetChildren();
		while (child)
		{
			if (!CollectControlledShutdownCarrierCompartmentPins(
				child,
				slots,
				occupants,
				evidence))
				return false;
			child = child.GetSibling();
		}
		return slots.Count() == occupants.Count();
	}

	protected bool ValidateControlledShutdownCarrierOccupantScope(
		out string evidence)
	{
		array<IEntity> uniqueCarriers = {};
		array<string> uniqueCarrierIds = {};
		foreach (HST_ControlledShutdownRescueCarrierPin carrierPin : m_aControlledShutdownCarrierPins)
		{
			if (!carrierPin || !carrierPin.m_Entity || !carrierPin.m_Entity.GetWorld()
				|| carrierPin.m_sRuntimeId.IsEmpty() || !carrierPin.m_Record
				|| uniqueCarriers.Find(carrierPin.m_Entity) >= 0
				|| uniqueCarrierIds.Contains(carrierPin.m_sRuntimeId)
				|| carrierPin.m_aCompartmentSlots.Count()
					!= carrierPin.m_aCompartmentOccupants.Count()
				|| IsControlledShutdownRescueRuntimeProxy(carrierPin.m_Entity)
				|| IsControlledShutdownRescuePlayerUsingCarrier(carrierPin.m_Entity))
			{
				evidence
					= "exact rescue controlled-shutdown carrier identity, locality, or player safety conflicts";
				return false;
			}
			uniqueCarriers.Insert(carrierPin.m_Entity);
			uniqueCarrierIds.Insert(carrierPin.m_sRuntimeId);

			array<IEntity> expectedCaptives = {};
			foreach (HST_ControlledShutdownRescueOperationSamplePin operationPin : m_aControlledShutdownPersistencePins)
			{
				foreach (HST_ControlledShutdownRescueCaptiveSamplePin captivePin : operationPin.m_aCaptives)
				{
					if (!captivePin || captivePin.m_CarrierEntity != carrierPin.m_Entity
						|| !captivePin.m_CaptiveEntity)
						continue;
					if (expectedCaptives.Find(captivePin.m_CaptiveEntity) >= 0)
					{
						evidence
							= "exact rescue controlled-shutdown carrier captive identity is duplicated";
						return false;
					}
					expectedCaptives.Insert(captivePin.m_CaptiveEntity);
					if (captivePin.m_CaptiveSlot
						&& (carrierPin.m_aCompartmentSlots.Find(captivePin.m_CaptiveSlot) < 0
							|| captivePin.m_CaptiveSlot.GetOccupant()
								!= captivePin.m_CaptiveEntity))
					{
						evidence
							= "exact rescue controlled-shutdown captive seat is outside its pinned carrier";
						return false;
					}
				}
			}

			for (int slotIndex; slotIndex < carrierPin.m_aCompartmentSlots.Count(); slotIndex++)
			{
				BaseCompartmentSlot slot = carrierPin.m_aCompartmentSlots[slotIndex];
				IEntity expectedOccupant = carrierPin.m_aCompartmentOccupants[slotIndex];
				BaseCompartmentManagerComponent manager;
				if (slot)
					manager = slot.GetManager();
				if (!slot || slot.GetVehicle() != carrierPin.m_Entity
					|| !manager || !manager.GetOwner()
					|| !IsEntityWithinControlledShutdownCarrierRoot(
						manager.GetOwner(), carrierPin.m_Entity)
					|| slot.GetOccupant() != expectedOccupant)
				{
					evidence
						= "exact rescue controlled-shutdown carrier compartment topology changed";
					return false;
				}
				if (expectedOccupant && expectedCaptives.Find(expectedOccupant) < 0)
				{
					evidence
						= "exact rescue controlled-shutdown carrier contains foreign live occupancy";
					return false;
				}
			}
		}
		return true;
	}

	protected bool ApplyControlledShutdownNativePins(
		HST_CampaignState state,
		out string evidence)
	{
		if (!ValidateControlledShutdownNativeDomainFences(state, evidence)
			|| !ValidateControlledShutdownCarrierOccupantScope(evidence))
			return false;

		// Re-run identity, seat, damage, and player topology for every native
		// captive before touching any carrier. Transform drift is intentionally
		// repairable from the pins; destruction or topology drift is not.
		foreach (HST_ControlledShutdownRescueOperationSamplePin operationPin : m_aControlledShutdownPersistencePins)
		{
			foreach (HST_ControlledShutdownRescueCaptiveSamplePin captivePin : operationPin.m_aCaptives)
			{
				if (!ValidateControlledShutdownCaptiveNativeIdentity(
					state,
					operationPin.m_Mission,
					captivePin,
					evidence,
					false))
					return false;
			}
		}

		foreach (HST_ControlledShutdownRescueCarrierPin carrierPin : m_aControlledShutdownCarrierPins)
		{
			if (!carrierPin || !carrierPin.m_Entity || !carrierPin.m_Entity.GetWorld()
				|| IsControlledShutdownRescueRuntimeProxy(carrierPin.m_Entity)
				|| IsControlledShutdownRescuePlayerUsingCarrier(carrierPin.m_Entity))
			{
				evidence
					= "exact rescue controlled-shutdown carrier became unsafe before quiescence";
				return false;
			}
			carrierPin.m_Entity.SetTransform(carrierPin.m_aTransform);
			if (!QuiesceControlledShutdownRescueCarrier(carrierPin.m_Entity, evidence))
				return false;
		}

		foreach (HST_ControlledShutdownRescueOperationSamplePin operationPin : m_aControlledShutdownPersistencePins)
		{
			foreach (HST_ControlledShutdownRescueCaptiveSamplePin captivePin : operationPin.m_aCaptives)
			{
				if (!captivePin || !captivePin.m_CaptiveEntity)
					continue;
				if (!ValidateControlledShutdownCaptiveNativeIdentity(
					state,
					operationPin.m_Mission,
					captivePin,
					evidence,
					false))
					return false;
				if (!captivePin.m_FollowComponent.PrepareControlledShutdownQuiescence(evidence))
					return false;
				if (!captivePin.m_CaptiveSlot)
				{
					HST_WorldPositionService.ApplyUprightEntityTransform(
						captivePin.m_CaptiveEntity,
						captivePin.m_vCaptivePosition,
						captivePin.m_vCaptiveAngles);
				}
				if (!captivePin.m_FollowComponent.MaintainControlledShutdownQuiescence(evidence))
					return false;
				if (!ValidateControlledShutdownCaptiveNativeIdentity(
					state,
					operationPin.m_Mission,
					captivePin,
					evidence,
					true))
					return false;
			}
		}
		return true;
	}

	protected bool ValidateControlledShutdownCaptiveNativeIdentity(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ControlledShutdownRescueCaptiveSamplePin pin,
		out string evidence,
		bool requirePinnedPose)
	{
		IEntity captiveEntity;
		IEntity carrierEntity;
		BaseCompartmentSlot captiveSlot;
		string observedSeatToken;
		if (!pin || !m_MissionRuntime
			|| !m_MissionRuntime.InspectExactRescueRuntimeTopologyReadOnly(
				state,
				mission,
				pin.m_Captive,
				captiveEntity,
				carrierEntity,
				captiveSlot,
				observedSeatToken,
				evidence,
				requirePinnedPose))
			return false;
		SCR_DamageManagerComponent captiveDamage;
		if (captiveEntity)
		{
			captiveDamage = SCR_DamageManagerComponent.Cast(
				captiveEntity.FindComponent(SCR_DamageManagerComponent));
		}
		SCR_DamageManagerComponent carrierDamage;
		if (carrierEntity)
		{
			carrierDamage = SCR_DamageManagerComponent.Cast(
				carrierEntity.FindComponent(SCR_DamageManagerComponent));
		}
		if ((captiveDamage
				&& captiveDamage.GetState() == EDamageState.DESTROYED)
			|| (carrierDamage
				&& carrierDamage.GetState() == EDamageState.DESTROYED))
		{
			evidence
				= "exact rescue controlled-shutdown captive or carrier was destroyed after pinning";
			return false;
		}
		if (captiveEntity != pin.m_CaptiveEntity
			|| carrierEntity != pin.m_CarrierEntity
			|| captiveSlot != pin.m_CaptiveSlot
			|| observedSeatToken != pin.m_sObservedSeatToken
			|| (captiveEntity && (!pin.m_FollowComponent
				|| pin.m_FollowComponent.GetOwner() != captiveEntity)))
		{
			evidence
				= "exact rescue controlled-shutdown captive native identity or seat topology changed";
			return false;
		}
		return true;
	}

	protected bool ValidateControlledShutdownCaptiveNativePin(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ControlledShutdownRescueCaptiveSamplePin pin,
		out string evidence)
	{
		if (!ValidateControlledShutdownCaptiveNativeIdentity(
			state,
			mission,
			pin,
			evidence,
			true))
			return false;
		if (!pin.m_CaptiveEntity)
			return true;
		if (!pin.m_FollowComponent
			|| !pin.m_FollowComponent.IsControlledShutdownQuiescenceExact())
		{
			evidence
				= "exact rescue controlled-shutdown captive follower is not quiescent";
			return false;
		}
		if (pin.m_CarrierEntity
			&& !FindControlledShutdownCarrierPin(pin.m_CarrierEntity))
			return false;
		// A carrier association does not imply that the captive is inside its
		// compartment hierarchy. Keep a seatless BOARDING captive pinned to its
		// own on-foot transform; a seated captive inherits the carrier pin.
		if (pin.m_CaptiveSlot)
			return pin.m_CarrierEntity != null;
		return IsExactRescueControlledShutdownPositionExact(
				pin.m_CaptiveEntity.GetOrigin(),
				pin.m_vCaptivePosition)
			&& AreExactRescueControlledShutdownAnglesExact(
				pin.m_CaptiveEntity.GetYawPitchRoll(),
				pin.m_vCaptiveAngles);
	}

	protected bool IsExactRescueControlledShutdownPositionExact(
		vector first,
		vector second)
	{
		float deltaX = first[0] - second[0];
		float deltaY = first[1] - second[1];
		float deltaZ = first[2] - second[2];
		return deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ
			<= 0.0001;
	}

	protected bool AreExactRescueControlledShutdownAnglesExact(
		vector first,
		vector second)
	{
		return IsExactRescueControlledShutdownAngleExact(first[0], second[0])
			&& IsExactRescueControlledShutdownAngleExact(first[1], second[1])
			&& IsExactRescueControlledShutdownAngleExact(first[2], second[2]);
	}

	protected bool IsExactRescueControlledShutdownAngleExact(
		float first,
		float second)
	{
		float difference = Math.AbsFloat(first - second);
		while (difference >= 360.0)
			difference -= 360.0;
		if (difference > 180.0)
			difference = 360.0 - difference;
		return difference <= 0.01;
	}

	protected bool ValidateControlledShutdownCarrierPins(
		HST_CampaignState state,
		out string evidence)
	{
		if (!ValidateControlledShutdownCarrierOccupantScope(evidence))
			return false;
		foreach (HST_ControlledShutdownRescueCarrierPin pin : m_aControlledShutdownCarrierPins)
		{
			if (!pin || !pin.m_Entity || !pin.m_Record
				|| state.FindRuntimeVehicle(pin.m_sRuntimeId) != pin.m_Record
				|| pin.m_Record.m_sRuntimeKind != "mission_carrier"
				|| IsControlledShutdownRescuePlayerUsingCarrier(pin.m_Entity)
				|| !IsControlledShutdownRescueTransformPinned(pin.m_Entity, pin.m_aTransform)
				|| !IsControlledShutdownRescueVehicleQuiescent(pin.m_Entity))
			{
				evidence
					= "exact rescue controlled-shutdown carrier pin is no longer exact";
				return false;
			}
		}
		return true;
	}

	protected bool IsControlledShutdownRescuePlayerUsingCarrier(IEntity carrier)
	{
		if (!carrier || !carrier.GetWorld())
			return true;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return true;
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			array<IEntity> playerEntities = {};
			IEntity controlled = playerManager.GetPlayerControlledEntity(playerId);
			IEntity main = SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
			if (controlled)
				playerEntities.Insert(controlled);
			if (main && main != controlled)
				playerEntities.Insert(main);
			foreach (IEntity playerEntity : playerEntities)
			{
				if (!playerEntity || !playerEntity.GetWorld())
					continue;
				if (playerEntity == carrier
					|| IsEntityWithinControlledShutdownCarrierRoot(playerEntity, carrier))
					return true;
				SCR_CompartmentAccessComponent access
					= SCR_CompartmentAccessComponent.Cast(
						playerEntity.FindComponent(SCR_CompartmentAccessComponent));
				if (access && ((access.IsInCompartment()
						&& access.GetVehicle() == carrier)
					|| access.IsGettingIn() || access.IsGettingOut()
					|| access.IsSwitchingSeatsAnim()))
					return true;
			}
		}
		return false;
	}

	protected bool IsEntityWithinControlledShutdownCarrierRoot(
		IEntity entity,
		IEntity root)
	{
		IEntity cursor = entity;
		for (int depth; depth < 64 && cursor; depth++)
		{
			if (cursor == root)
				return true;
			IEntity parent = cursor.GetParent();
			if (parent == cursor)
				return false;
			cursor = parent;
		}
		return false;
	}

	protected bool QuiesceControlledShutdownRescueCarrier(
		IEntity carrier,
		out string evidence)
	{
		if (!carrier || !carrier.GetWorld()
			|| IsControlledShutdownRescueRuntimeProxy(carrier)
			|| IsControlledShutdownRescuePlayerUsingCarrier(carrier))
			return false;
		if (!QuiesceControlledShutdownRescuePhysicsHierarchy(carrier))
		{
			evidence
				= "exact rescue controlled-shutdown carrier physics quiescence failed";
			return false;
		}
		return true;
	}

	protected bool QuiesceControlledShutdownRescuePhysicsHierarchy(IEntity entity)
	{
		if (!entity || !entity.GetWorld()
			|| IsControlledShutdownRescueRuntimeProxy(entity))
			return false;
		BaseVehicleControllerComponent controller
			= BaseVehicleControllerComponent.Cast(
				entity.FindComponent(BaseVehicleControllerComponent));
		if (controller)
		{
			controller.Shutdown();
			controller.StopEngine(false);
		}
		CarControllerComponent carController
			= CarControllerComponent.Cast(
				entity.FindComponent(CarControllerComponent));
		if (carController)
			carController.SetPersistentHandBrake(true);
		HelicopterControllerComponent helicopterController
			= HelicopterControllerComponent.Cast(
				entity.FindComponent(HelicopterControllerComponent));
		if (helicopterController)
		{
			helicopterController.SetPersistentWheelBrake(true);
			helicopterController.SetAutohoverEnabled(true);
		}
		IEntity child = entity.GetChildren();
		while (child)
		{
			if (!QuiesceControlledShutdownRescuePhysicsHierarchy(child))
				return false;
			child = child.GetSibling();
		}
		Physics physics = entity.GetPhysics();
		if (physics && physics.IsDynamic())
		{
			physics.ClearForces();
			physics.SetVelocity(vector.Zero);
			physics.SetAngularVelocity(vector.Zero);
			physics.SetActive(ActiveState.INACTIVE);
		}
		return true;
	}

	protected bool IsControlledShutdownRescueVehicleQuiescent(IEntity entity)
	{
		if (!entity || !entity.GetWorld()
			|| IsControlledShutdownRescueRuntimeProxy(entity))
			return false;
		BaseVehicleControllerComponent controller
			= BaseVehicleControllerComponent.Cast(
				entity.FindComponent(BaseVehicleControllerComponent));
		if (controller && controller.IsEngineOn())
			return false;
		CarControllerComponent carController
			= CarControllerComponent.Cast(
				entity.FindComponent(CarControllerComponent));
		if (carController && !carController.GetPersistentHandBrake())
			return false;
		HelicopterControllerComponent helicopterController
			= HelicopterControllerComponent.Cast(
				entity.FindComponent(HelicopterControllerComponent));
		if (helicopterController
			&& (!helicopterController.GetPersistentWheelBrake()
				|| !helicopterController.GetAutohoverEnabled()))
			return false;
		Physics physics = entity.GetPhysics();
		if (physics && physics.IsDynamic()
			&& (physics.IsActive()
				|| !IsControlledShutdownRescueZeroVector(physics.GetVelocity())
				|| !IsControlledShutdownRescueZeroVector(physics.GetAngularVelocity())))
			return false;
		IEntity child = entity.GetChildren();
		while (child)
		{
			if (!IsControlledShutdownRescueVehicleQuiescent(child))
				return false;
			child = child.GetSibling();
		}
		return true;
	}

	protected bool IsControlledShutdownRescueTransformPinned(
		IEntity entity,
		vector expectedTransform[4])
	{
		if (!entity || !entity.GetWorld())
			return false;
		vector currentTransform[4];
		entity.GetTransform(currentTransform);
		for (int index; index < 4; index++)
		{
			if (!IsControlledShutdownRescueVectorNear(
				currentTransform[index],
				expectedTransform[index]))
				return false;
		}
		return true;
	}

	protected bool IsControlledShutdownRescueVectorNear(vector first, vector second)
	{
		return Math.AbsFloat(first[0] - second[0]) <= 0.001
			&& Math.AbsFloat(first[1] - second[1]) <= 0.001
			&& Math.AbsFloat(first[2] - second[2]) <= 0.001;
	}

	protected bool IsControlledShutdownRescueZeroVector(vector value)
	{
		return Math.AbsFloat(value[0]) <= 0.001
			&& Math.AbsFloat(value[1]) <= 0.001
			&& Math.AbsFloat(value[2]) <= 0.001;
	}

	protected bool IsControlledShutdownRescueRuntimeProxy(IEntity entity)
	{
		if (!entity)
			return true;
		BaseRplComponent replication
			= BaseRplComponent.Cast(entity.FindComponent(BaseRplComponent));
		return replication && replication.IsProxy();
	}

	protected bool TryResolveUniqueMissionRuntimeProjectionReadOnly(
		HST_CampaignState state,
		string projectionId,
		out HST_MissionRuntimeEntityState projection,
		out string evidence)
	{
		projection = null;
		if (!state || projectionId.IsEmpty())
		{
			evidence = "exact rescue controlled-shutdown durable projection identity is unavailable";
			return false;
		}
		int matches;
		foreach (HST_MissionRuntimeEntityState candidate : state.m_aMissionRuntimeEntities)
		{
			if (!candidate || candidate.m_sRuntimeEntityId != projectionId)
				continue;
			matches++;
			projection = candidate;
		}
		if (matches > 1)
		{
			evidence = "exact rescue controlled-shutdown durable projection identity is duplicated";
			return false;
		}
		return true;
	}

	protected bool TryResolveUniqueMissionCarrierReadOnly(
		HST_CampaignState state,
		string carrierId,
		out HST_RuntimeVehicleState carrier,
		out string evidence)
	{
		carrier = null;
		if (!state)
			return false;
		if (carrierId.IsEmpty())
			return true;
		int matches;
		foreach (HST_RuntimeVehicleState candidate : state.m_aRuntimeVehicles)
		{
			if (!candidate || candidate.m_sVehicleRuntimeId != carrierId)
				continue;
			matches++;
			carrier = candidate;
		}
		if (matches != 1 || !carrier || carrier.m_sRuntimeKind != "mission_carrier")
		{
			evidence = "exact rescue controlled-shutdown durable carrier row is absent, duplicate, or foreign";
			return false;
		}
		return true;
	}

	protected bool CollectControlledShutdownMissionObjectivesReadOnly(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		array<ref HST_MissionObjectiveState> output,
		out string evidence)
	{
		if (!state || !mission || !output)
			return false;
		output.Clear();
		array<string> objectiveIds = {};
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (objective.m_sObjectiveId.IsEmpty()
				|| objectiveIds.Contains(objective.m_sObjectiveId))
			{
				evidence = "exact rescue controlled-shutdown mission objective identity is empty or duplicated";
				return false;
			}
			objectiveIds.Insert(objective.m_sObjectiveId);
			output.Insert(objective);
		}
		return true;
	}

	protected bool IsPreparedCaptiveCompatibilityExact(HST_MissionAssetState captive)
	{
		if (!captive || captive.m_iRescueContractVersion != EXACT_CONTRACT_VERSION)
			return false;
		HST_ERescueCaptiveDisposition disposition = captive.m_eRescueDisposition;
		if (disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_UNKNOWN)
			return false;
		bool picked = disposition != HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD;
		bool delivered = disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED;
		bool destroyed = disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED;
		bool attached = disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
			|| disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
			|| disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
		string carriedBy;
		if (attached)
		{
			carriedBy = captive.m_sRescueCarrierVehicleId;
			if (carriedBy.IsEmpty())
				carriedBy = captive.m_sRescueEscortIdentityId;
		}
		string interaction;
		if (disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD)
			interaction = "held";
		else if (disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED)
			interaction = "freed";
		else if (disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING)
			interaction = "following";
		else if (disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING)
			interaction = "boarding";
		else if (disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED)
			interaction = "loaded";
		else if (delivered)
			interaction = "extracted";
		else if (destroyed)
			interaction = "killed";
		return captive.m_bPickedUp == picked
			&& captive.m_bDelivered == delivered
			&& captive.m_bDestroyed == destroyed
			&& captive.m_bAlive == !destroyed
			&& captive.m_bAttachedToCarrier == attached
			&& captive.m_sCarriedByVehicleId == carriedBy
			&& captive.m_sLastInteraction == interaction
			&& ((!delivered && !destroyed) || !captive.m_bSpawned);
	}

	protected bool IsPreparedCaptiveRuntimeProjectionExact(
		HST_ActiveMissionState mission,
		HST_MissionAssetState captive,
		HST_MissionRuntimeEntityState projection)
	{
		if (!mission || !captive)
			return false;
		if (!projection)
			return !captive.m_bSpawned;
		bool killed = captive.m_eRescueDisposition
			== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED;
		bool extracted = captive.m_eRescueDisposition
			== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED;
		return projection.m_sRuntimeEntityId == captive.m_sRescueProjectionId
			&& projection.m_sMissionInstanceId == mission.m_sInstanceId
			&& projection.m_sKind == captive.m_sKind
			&& projection.m_sPrefab == captive.m_sPrefab
			&& projection.m_bSpawned == captive.m_bSpawned
			&& projection.m_bDestroyed == killed
			&& projection.m_bRecovered == extracted;
	}

	protected string BuildControlledShutdownOperationSample(
		HST_OperationRecordState operation)
	{
		if (!operation)
			return "<absent-operation>";
		string sample = string.Format(
			"operation|id=%1|type=%2|contract=%3|owner=%4|actor=%5|issue=%6|confirmation=%7|support=%8|enemy=%9",
			operation.m_sOperationId,
			operation.m_eType,
			operation.m_iContractVersion,
			operation.m_sOwnerFactionKey,
			operation.m_sActorIdentityId,
			operation.m_sIssueRequestId,
			operation.m_sConfirmationRequestId,
			operation.m_sSupportRequestId,
			operation.m_sEnemyOrderId);
		sample += string.Format(
			"|mission=%1|patrol=%2|quote=%3|manifest=%4|spawn=%5|force=%6|projection=%7|group=%8|originZone=%9",
			operation.m_sMissionInstanceId,
			operation.m_sLocalSecurityPatrolId,
			operation.m_sQuoteId,
			operation.m_sManifestId,
			operation.m_sSpawnResultId,
			operation.m_sForceId,
			operation.m_sProjectionId,
			operation.m_sGroupId,
			operation.m_sOriginZoneId);
		sample += string.Format(
			"|originPosition=%1|assignmentKind=%2|assignmentZone=%3|assignmentPosition=%4|tacticalZone=%5|tacticalPosition=%6|strategicPosition=%7|route=%8|routeHash=%9",
			operation.m_vOriginPosition,
			operation.m_sAssignmentKind,
			operation.m_sAssignmentZoneId,
			operation.m_vAssignmentPosition,
			operation.m_sTacticalTargetZoneId,
			operation.m_vTacticalTargetPosition,
			operation.m_vStrategicPosition,
			operation.m_sCurrentRouteId,
			operation.m_sRouteContractHash);
		sample += string.Format(
			"|projectionContract=%1|routeVersion=%2|waypoint=%3|lap=%4|leg=%5|loopStarted=%6|loopCompleted=%7|routeStart=%8|routeEnd=%9",
			operation.m_iProjectionContractVersion,
			operation.m_iRouteVersion,
			operation.m_iRouteWaypointIndex,
			operation.m_iRouteLapCount,
			operation.m_iRouteLegSequence,
			operation.m_iRouteLoopStartedAtSecond,
			operation.m_iRouteLoopCompletedAtSecond,
			operation.m_vRouteStartPosition,
			operation.m_vRouteEndPosition);
		sample += string.Format(
			"|routeDistance=%1|routeProgress=%2|speed=%3|strategicUpdated=%4|projectionDecision=%5|restoreSequence=%6|virtualLast=%7|virtualStep=%8|friendlyCarry=%9",
			operation.m_fRouteTotalDistanceMeters,
			operation.m_fRouteProgressMeters,
			operation.m_fStrategicSpeedMetersPerSecond,
			operation.m_iStrategicLastUpdateSecond,
			operation.m_iLastProjectionDecisionSecond,
			operation.m_iLastNormalizedRestoreSequence,
			operation.m_iVirtualCombatLastStepSecond,
			operation.m_iVirtualCombatStepIndex,
			operation.m_iVirtualCombatFriendlyDamageCarry);
		sample += string.Format(
			"|hostileCarry=%1|lastFriendly=%2|lastHostile=%3|arrivalCount=%4|arrivalSecond=%5|projectionReason=%6|virtualReason=%7|recall=%8|settlementPolicy=%9",
			operation.m_iVirtualCombatHostileDamageCarry,
			operation.m_iLastVirtualFriendlyCount,
			operation.m_iLastVirtualHostileCount,
			operation.m_iArrivalConfirmationCount,
			operation.m_iLastArrivalConfirmationSecond,
			operation.m_sLastProjectionReason,
			operation.m_sLastVirtualCombatReason,
			operation.m_sRecallPolicyId,
			operation.m_sSettlementPolicyId);
		sample += string.Format(
			"|duty=%1|resumeDuty=%2|engagement=%3|materialization=%4|positionAuthority=%5|settlement=%6|terminal=%7|settlementId=%8|terminalReason=%9",
			operation.m_eDutyState,
			operation.m_eResumeDutyState,
			operation.m_eEngagementMode,
			operation.m_eMaterializationState,
			operation.m_ePositionAuthority,
			operation.m_eSettlementState,
			operation.m_eTerminalResult,
			operation.m_sSettlementId,
			operation.m_sTerminalReason);
		sample += string.Format(
			"|seed=%1|created=%2|dutyEntered=%3|engagementEntered=%4|materializationEntered=%5|lastContact=%6|lastProgress=%7|settled=%8|revision=%9",
			operation.m_iDeterministicSeed,
			operation.m_iCreatedAtSecond,
			operation.m_iDutyStateEnteredAtSecond,
			operation.m_iEngagementStateEnteredAtSecond,
			operation.m_iMaterializationStateEnteredAtSecond,
			operation.m_iLastContactAtSecond,
			operation.m_iLastProgressAtSecond,
			operation.m_iSettledAtSecond,
			operation.m_iRevision);
		return sample;
	}

	protected string BuildControlledShutdownMissionSample(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "<absent-mission>";
		int gunShopItemCount = -1;
		if (mission.m_aGunShopItems)
			gunShopItemCount = mission.m_aGunShopItems.Count();
		string sample = string.Format(
			"mission|instance=%1|mission=%2|display=%3|operation=%4|manifest=%5|spawn=%6|settlement=%7|operationContract=%8|radioContract=%9",
			mission.m_sInstanceId,
			mission.m_sMissionId,
			mission.m_sDisplayName,
			mission.m_sOperationId,
			mission.m_sManifestId,
			mission.m_sSpawnResultId,
			mission.m_sSettlementId,
			mission.m_iOperationContractVersion,
			mission.m_iRadioSiteContractVersion);
		sample += string.Format(
			"|radioSite=%1|radioRequest=%2|radioRevision=%3|status=%4|runtimeMode=%5|remaining=%6|targetZone=%7|site=%8|targetPosition=%9",
			mission.m_sRadioSiteId,
			mission.m_sRadioSiteTransitionRequestId,
			mission.m_iRadioSiteRevision,
			mission.m_eStatus,
			mission.m_eRuntimeMode,
			mission.m_iRemainingSeconds,
			mission.m_sTargetZoneId,
			mission.m_sSiteId,
			mission.m_vTargetPosition);
		sample += string.Format(
			"|extractionPosition=%1|marker=%2|primitive=%3|runtimeType=%4|phase=%5|failure=%6|runtimeEntity=%7|lastEvent=%8|started=%9",
			mission.m_vRescueExtractionPosition,
			mission.m_sMarkerId,
			mission.m_sRuntimePrimitive,
			mission.m_sRuntimeType,
			mission.m_sRuntimePhase,
			mission.m_sRuntimeFailureReason,
			mission.m_sRuntimeEntityId,
			mission.m_sLastRuntimeEventKey,
			mission.m_iStartedAtSecond);
		sample += string.Format(
			"|activeUntil=%1|runtimeStarted=%2|hold=%3|eta=%4|counterA=%5|counterB=%6|counterC=%7|requiredCargo=%8|recoveredCargo=%9",
			mission.m_iActiveUntilSecond,
			mission.m_iRuntimeStartedAtSecond,
			mission.m_iRuntimeHoldSeconds,
			mission.m_iRuntimeETASeconds,
			mission.m_iRuntimeCounterA,
			mission.m_iRuntimeCounterB,
			mission.m_iRuntimeCounterC,
			mission.m_iRequiredCargoCount,
			mission.m_iRecoveredCargoCount);
		sample += string.Format(
			"|requiredCaptives=%1|extractedCaptives=%2|rescueGraceUntil=%3|requiredVehicles=%4|capturedVehicles=%5|runtimePickups=%6|runtimeDeliveries=%7|runtimeDestroyed=%8|dynamic=%9",
			mission.m_iRequiredCaptiveCount,
			mission.m_iExtractedCaptiveCount,
			mission.m_iRescueGraceUntilSecond,
			mission.m_iRequiredVehicleCount,
			mission.m_iCapturedVehicleCount,
			mission.m_iRuntimePickupCount,
			mission.m_iRuntimeDeliveryCount,
			mission.m_iRuntimeDestroyedCount,
			mission.m_bDynamic);
		sample += string.Format(
			"|requested=%1|static=%2|runtimeSpawned=%3|runtimeFallback=%4|cleanup=%5|rescueGrace=%6|createdNotice=%7|completedNotice=%8|failedNotice=%9",
			mission.m_bRequested,
			mission.m_bStatic,
			mission.m_bRuntimeSpawned,
			mission.m_bRuntimeFallback,
			mission.m_bRuntimeCleanupComplete,
			mission.m_bRescueExtractionGrace,
			mission.m_bCreatedNotificationSent,
			mission.m_bCompletedNotificationSent,
			mission.m_bFailedNotificationSent);
		sample += string.Format(
			"|expiredNotice=%1|convoyArrival=%2|convoyCrew=%3|convoyVehicle=%4|convoyCargo=%5|convoyExpired=%6|convoySummary=%7|gunShopItems=%8|gunShopSeller=%9",
			mission.m_bExpiredNotificationSent,
			mission.m_bConvoyArrivalOutcomeApplied,
			mission.m_bConvoyCrewEliminatedOutcomeApplied,
			mission.m_bConvoyVehicleCapturedOutcomeApplied,
			mission.m_bConvoyCargoDeliveredOutcomeApplied,
			mission.m_bConvoyExpiredOutcomeApplied,
			mission.m_sConvoyOutcomeSummary,
			gunShopItemCount,
			mission.m_sGunShopSellerAssetId);
		sample += string.Format(
			"|gunShopDriver=%1|gunShopVehicle=%2|gunShopMarker=%3|sellerPosition=%4|deliveryPosition=%5|stock=%6|purchase=%7|purchaseNotice=%8|expiryNotice=%9",
			mission.m_sGunShopDeliveryDriverAssetId,
			mission.m_sGunShopDeliveryVehicleAssetId,
			mission.m_sGunShopDeliveryMarkerId,
			mission.m_vGunShopSellerPosition,
			mission.m_vGunShopDeliveryPosition,
			mission.m_bGunShopStockGenerated,
			mission.m_bGunShopPurchaseMade,
			mission.m_bGunShopPurchaseNoticeSent,
			mission.m_bGunShopExpiryNoticeSent);
		sample += string.Format(
			"|deliverySpawned=%1|deliveryNotice=%2|deliveryArrived=%3|purchasedTotal=%4|deliveryStarted=%5",
			mission.m_bGunShopDeliverySpawned,
			mission.m_bGunShopDeliveryNoticeSent,
			mission.m_bGunShopDeliveryArrived,
			mission.m_iGunShopPurchasedTotal,
			mission.m_iGunShopDeliveryStartedAtSecond);
		return sample;
	}

	protected string BuildControlledShutdownGroupSample(HST_ActiveGroupState group)
	{
		if (!group)
			return "<absent-group>";
		string sample = string.Format(
			"group|id=%1|operation=%2|manifest=%3|spawn=%4|force=%5|projection=%6|zone=%7|faction=%8|mission=%9",
			group.m_sGroupId,
			group.m_sOperationId,
			group.m_sManifestId,
			group.m_sSpawnResultId,
			group.m_sForceId,
			group.m_sProjectionId,
			group.m_sZoneId,
			group.m_sFactionKey,
			group.m_sMissionInstanceId);
		sample += string.Format(
			"|support=%1|enemy=%2|convoy=%3|asset=%4|garrison=%5|qrf=%6|patrol=%7|prefab=%8|vehiclePrefab=%9",
			group.m_sSupportRequestId,
			group.m_sEnemyOrderId,
			group.m_sConvoyElementId,
			group.m_sMissionAssetId,
			group.m_sGarrisonZoneId,
			group.m_sQRFInstanceId,
			group.m_sLocalSecurityPatrolId,
			group.m_sPrefab,
			group.m_sVehiclePrefab);
		sample += string.Format(
			"|compositionRequest=%1|compositionIntent=%2|compositionTier=%3|compositionSummary=%4|fallback=%5|spawnFailure=%6|position=%7|route=%8|sourcePosition=%9",
			group.m_sCompositionRequestId,
			group.m_sCompositionIntentId,
			group.m_sCompositionTier,
			group.m_sCompositionSummary,
			group.m_sSpawnFallbackMode,
			group.m_sSpawnFailureReason,
			group.m_vPosition,
			group.m_sRouteId,
			group.m_vSourcePosition);
		sample += string.Format(
			"|targetPosition=%1|runtimeEntity=%2|runtimeStatus=%3|infantry=%4|vehicles=%5|originalInfantry=%6|originalVehicles=%7|compositionCost=%8|compositionManpower=%9",
			group.m_vTargetPosition,
			group.m_sRuntimeEntityId,
			group.m_sRuntimeStatus,
			group.m_iInfantryCount,
			group.m_iVehicleCount,
			group.m_iOriginalInfantryCount,
			group.m_iOriginalVehicleCount,
			group.m_iCompositionCost,
			group.m_iCompositionManpower);
		sample += string.Format(
			"|compositionVehicles=%1|compositionArmed=%2|spawnedAt=%3|lastSeen=%4|survivorInfantry=%5|survivorVehicles=%6|spawnedAgents=%7|waypoints=%8|maxCrew=%9",
			group.m_iCompositionVehicleCount,
			group.m_iCompositionArmedVehicleCount,
			group.m_iSpawnedAtSecond,
			group.m_iLastSeenAliveCount,
			group.m_iSurvivorInfantryCount,
			group.m_iSurvivorVehicleCount,
			group.m_iSpawnedAgentCount,
			group.m_iAssignedWaypointCount,
			group.m_iMaxObservedCrewAlive);
		sample += string.Format(
			"|durableLiving=%1|combatInfantry=%2|mannedVehicles=%3|staticOperators=%4|sampleSecond=%5|lastCasualty=%6|eliminated=%7|revision=%8|everCrewed=%9",
			group.m_iDurableLivingInfantryCount,
			group.m_iCombatEffectiveInfantryCount,
			group.m_iOperationalMannedVehicleCount,
			group.m_iCombatEffectiveStaticOperatorCount,
			group.m_iCombatPresenceSampleSecond,
			group.m_iLastCasualtySecond,
			group.m_iEliminatedAtSecond,
			group.m_iLifecycleRevision,
			group.m_bEverHadLivingCrew);
		sample += string.Format(
			"|sampleAuthoritative=%1|sampleReason=%2|everPopulated=%3|spawnCompleted=%4|crewFailed=%5|crewFailure=%6|convoyStage=%7|qrfFlag=%8|spawnAttempted=%9",
			group.m_bCombatPresenceSampleAuthoritative,
			group.m_sCombatPresenceSampleReason,
			group.m_bEverPopulated,
			group.m_bSpawnCompleted,
			group.m_bCrewPopulationTerminallyFailed,
			group.m_sCrewPopulationFailureReason,
			group.m_sConvoyRuntimeStage,
			group.m_bQRF,
			group.m_bSpawnAttempted);
		sample += string.Format("|spawnedEntity=%1", group.m_bSpawnedEntity);
		return sample;
	}

	protected string BuildControlledShutdownCaptiveSample(HST_MissionAssetState captive)
	{
		if (!captive)
			return "<absent-captive>";
		string sample = string.Format(
			"captive|id=%1|mission=%2|operation=%3|manifest=%4|slot=%5|vehicleSlot=%6|convoy=%7|kind=%8|role=%9",
			captive.m_sAssetId,
			captive.m_sMissionInstanceId,
			captive.m_sOperationId,
			captive.m_sManifestId,
			captive.m_sManifestSlotId,
			captive.m_sAssignedVehicleSlotId,
			captive.m_sConvoyElementId,
			captive.m_sKind,
			captive.m_sRole);
		sample += string.Format(
			"|prefab=%1|entity=%2|carriedBy=%3|interaction=%4|spawned=%5|picked=%6|delivered=%7|destroyed=%8|alive=%9",
			captive.m_sPrefab,
			captive.m_sEntityId,
			captive.m_sCarriedByVehicleId,
			captive.m_sLastInteraction,
			captive.m_bSpawned,
			captive.m_bPickedUp,
			captive.m_bDelivered,
			captive.m_bDestroyed,
			captive.m_bAlive);
		sample += string.Format(
			"|attached=%1|outcomeApplied=%2|outcome=%3|demolitionDamage=%4|demolitionRequired=%5|demolitionHits=%6|demolitionSource=%7|demolitionSecond=%8|sourcePosition=%9",
			captive.m_bAttachedToCarrier,
			captive.m_bOutcomeApplied,
			captive.m_sOutcomeKind,
			captive.m_fDemolitionDamage,
			captive.m_fDemolitionRequiredDamage,
			captive.m_iDemolitionHits,
			captive.m_sLastDemolitionSource,
			captive.m_iLastDemolitionSecond,
			captive.m_vSourcePosition);
		sample += string.Format(
			"|targetPosition=%1|currentPosition=%2|lastPosition=%3|deadline=%4|cargoCost=%5|interactionRadius=%6|radioContract=%7|radioSite=%8|radioOwnership=%9",
			captive.m_vTargetPosition,
			captive.m_vCurrentPosition,
			captive.m_vLastKnownPosition,
			captive.m_iDeadlineSecond,
			captive.m_iCargoCapacityCost,
			captive.m_iInteractionRadiusMeters,
			captive.m_iRadioSiteContractVersion,
			captive.m_sRadioSiteId,
			captive.m_eRadioSiteTargetOwnership);
		sample += string.Format(
			"|radioPrefab=%1|radioPosition=%2|rescueContract=%3|ordinal=%4|disposition=%5|escort=%6|carrier=%7|seat=%8|lastRequest=%9",
			captive.m_sRadioSiteAuthoredTargetPrefab,
			captive.m_vRadioSiteAuthoredTargetPosition,
			captive.m_iRescueContractVersion,
			captive.m_iRescueOrdinal,
			captive.m_eRescueDisposition,
			captive.m_sRescueEscortIdentityId,
			captive.m_sRescueCarrierVehicleId,
			captive.m_sRescueCarrierSeatToken,
			captive.m_sRescueLastCommandRequestId);
		sample += string.Format(
			"|lastResult=%1|casualtyReceipt=%2|extractionReceipt=%3|projection=%4|transitionSecond=%5|rescueRevision=%6|projectionGeneration=%7|deathObserved=%8|extractionObserved=%9",
			captive.m_sRescueLastCommandResult,
			captive.m_sRescueCasualtyReceiptId,
			captive.m_sRescueExtractionReceiptId,
			captive.m_sRescueProjectionId,
			captive.m_iRescueTransitionSecond,
			captive.m_iRescueRevision,
			captive.m_iRescueProjectionGeneration,
			captive.m_bRescueDeathObserved,
			captive.m_bRescueExtractionObserved);

		if (!captive.m_aDemolitionEvidenceKeys)
			sample += "|demolitionEvidence=<null>";
		else
		{
			sample += string.Format(
				"|demolitionEvidenceCount=%1",
				captive.m_aDemolitionEvidenceKeys.Count());
			for (int demolitionIndex = 0; demolitionIndex < captive.m_aDemolitionEvidenceKeys.Count(); demolitionIndex++)
			{
				sample += string.Format(
					"|demolitionEvidence[%1]=%2",
					demolitionIndex,
					captive.m_aDemolitionEvidenceKeys[demolitionIndex]);
			}
		}
		if (!captive.m_aRescueCommandReceipts)
			sample += "|receipts=<null>";
		else
		{
			sample += string.Format(
				"|receiptCount=%1",
				captive.m_aRescueCommandReceipts.Count());
			for (int receiptIndex = 0; receiptIndex < captive.m_aRescueCommandReceipts.Count(); receiptIndex++)
			{
				HST_RescueCommandReceiptState receipt
					= captive.m_aRescueCommandReceipts[receiptIndex];
				if (!receipt)
				{
					sample += string.Format("|receipt[%1]=<null>", receiptIndex);
					continue;
				}
				sample += string.Format(
					"|receipt[%1].request=%2|actor=%3|command=%4|result=%5|revision=%6",
					receiptIndex,
					receipt.m_sRequestId,
					receipt.m_sActorIdentityId,
					receipt.m_sCommand,
					receipt.m_sResult,
					receipt.m_iRecordedRevision);
			}
		}
		return sample;
	}

	protected string BuildControlledShutdownObjectiveSample(HST_MissionObjectiveState objective)
	{
		if (!objective)
			return "<absent-objective>";
		string sample = string.Format(
			"objective|id=%1|mission=%2|type=%3|label=%4|requirement=%5|target=%6|targetZone=%7|physical=%8|runtime=%9",
			objective.m_sObjectiveId,
			objective.m_sMissionInstanceId,
			objective.m_eType,
			objective.m_sLabel,
			objective.m_sRequirementText,
			objective.m_sTargetId,
			objective.m_sTargetZoneId,
			objective.m_sPhysicalEntityId,
			objective.m_sLinkedRuntimeEntityId);
		sample += string.Format(
			"|primitive=%1|position=%2|requiredProgress=%3|currentProgress=%4|hold=%5|requiredHold=%6|currentCount=%7|requiredCount=%8|extractionStarted=%9",
			objective.m_sRuntimePrimitive,
			objective.m_vPosition,
			objective.m_iRequiredProgress,
			objective.m_iCurrentProgress,
			objective.m_iHoldSeconds,
			objective.m_iRequiredHoldSeconds,
			objective.m_iCurrentCount,
			objective.m_iRequiredCount,
			objective.m_bExtractionStarted);
		sample += string.Format(
			"|deliveryStarted=%1|complete=%2|failed=%3|cleanup=%4|worldDetected=%5|abstractFallback=%6",
			objective.m_bDeliveryStarted,
			objective.m_bComplete,
			objective.m_bFailed,
			objective.m_bCleanupComplete,
			objective.m_bWorldDetected,
			objective.m_bAbstractFallback);
		return sample;
	}

	protected string BuildControlledShutdownRuntimeProjectionSample(
		HST_MissionRuntimeEntityState projection)
	{
		if (!projection)
			return "<absent-runtime-projection>";
		return string.Format(
			"runtimeProjection|id=%1|mission=%2|kind=%3|prefab=%4|position=%5|angles=%6|spawned=%7|destroyed=%8|recovered=%9",
			projection.m_sRuntimeEntityId,
			projection.m_sMissionInstanceId,
			projection.m_sKind,
			projection.m_sPrefab,
			projection.m_vPosition,
			projection.m_vAngles,
			projection.m_bSpawned,
			projection.m_bDestroyed,
			projection.m_bRecovered);
	}

	protected string BuildControlledShutdownCarrierSample(HST_RuntimeVehicleState carrier)
	{
		if (!carrier)
			return "<absent-carrier>";
		string sample = string.Format(
			"carrier|id=%1|prefab=%2|display=%3|faction=%4|zone=%5|kind=%6|sourceKind=%7|position=%8|angles=%9",
			carrier.m_sVehicleRuntimeId,
			carrier.m_sPrefab,
			carrier.m_sDisplayName,
			carrier.m_sFactionKey,
			carrier.m_sZoneId,
			carrier.m_sRuntimeKind,
			carrier.m_sSourceVehicleKind,
			carrier.m_vPosition,
			carrier.m_vAngles);
		sample += string.Format(
			"|spawned=%1|detached=%2|deleted=%3|ammo=%4|repair=%5|fuel=%6|reported=%7|undercover=%8|heat=%9",
			carrier.m_iSpawnedAtSecond,
			carrier.m_bDetached,
			carrier.m_bDeleted,
			carrier.m_bAmmoSource,
			carrier.m_bRepairSource,
			carrier.m_bFuelSource,
			carrier.m_bReported,
			carrier.m_bCanProvideUndercover,
			carrier.m_iVehicleHeat);
		sample += string.Format(
			"|lastReported=%1|reportedUntil=%2|heatChanged=%3|passengerCompromise=%4|reportedReason=%5|reporterZone=%6",
			carrier.m_iLastReportedSecond,
			carrier.m_iReportedUntilSecond,
			carrier.m_iLastVehicleHeatChangedSecond,
			carrier.m_iPassengerCompromiseCount,
			carrier.m_sLastReportedReason,
			carrier.m_sLastReporterZoneId);
		return sample;
	}

	protected bool SamplePhysicalGuardPositionForPersistence(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		out string failure)
	{
		failure = "";
		vector previousGroupPosition = group.m_vPosition;
		vector livePosition;
		string liveEvidence;
		if (!m_PhysicalWar.TryResolveExactMissionGuardLivePosition(
			state,
			group,
			livePosition,
			liveEvidence))
		{
			failure = "exact rescue guard persistence live position is unavailable: "
				+ liveEvidence;
			return false;
		}

		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		bool operationChanged = operation.m_vStrategicPosition != livePosition
			|| operation.m_iLastProgressAtSecond != nowSecond
			|| previousGroupPosition != livePosition
			|| group.m_vSourcePosition != livePosition
			|| group.m_vTargetPosition != operation.m_vAssignmentPosition;
		operation.m_vStrategicPosition = livePosition;
		operation.m_iLastProgressAtSecond = nowSecond;
		if (operationChanged)
			operation.m_iRevision++;

		// PHYSICAL and DEMATERIALIZING rescue guards own one live durable anchor.
		// Source remains a canonical copy of that anchor while target remains the
		// immutable assignment, matching the normal TickPhysicalGuard projection.
		// Pose sampling is not a lifecycle transition, so it must not advance the
		// active-group lifecycle revision.
		group.m_vPosition = livePosition;
		group.m_vSourcePosition = livePosition;
		group.m_vTargetPosition = operation.m_vAssignmentPosition;
		return true;
	}

	bool PrepareOpenPhysicalAuthorityForPersistence(HST_CampaignState state, out string failure)
	{
		failure = "";
		if (!state)
		{
			failure = "exact rescue persistence reconciliation state is unavailable";
			return false;
		}
		// Scan the complete exact-rescue scope before any operation can be
		// reconciled. A later MATERIALIZING row must not be discovered only after an
		// earlier row has already mutated, and guard spawn state is intentionally
		// irrelevant to this rejection.
		if (!ValidateNoOpenMaterializingExactRescue(state, failure))
			return false;
		if (m_bControlledShutdownPersistenceSampleApplied)
			return MaintainControlledShutdownPersistenceSample(state, failure);
		if (!m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar || !m_MissionRuntime)
		{
			failure = "exact rescue persistence reconciliation services are unavailable";
			return false;
		}
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
				|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				continue;
			if (operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
			{
				failure = "exact rescue persistence cannot sample MATERIALIZING authority";
				if (!operation.m_sOperationId.IsEmpty())
					failure += ": " + operation.m_sOperationId;
				return false;
			}
			HST_ActiveMissionState mission;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			failure = ResolveRuntimeContext(state, operation, mission, manifest, batch, group);
			if (!failure.IsEmpty())
			{
				QuarantineOperationAuthority(state, operation, failure);
				return false;
			}
			bool physical = operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
			if (physical && group.m_sRuntimeStatus != GUARD_ELIMINATED_STATUS
				&& group.m_sRuntimeStatus != "rescue_pow_guard_retired")
			{
				int living;
				if (!ReconcileGuardRoster(state, operation, batch, group, living, failure))
				{
					failure = "exact rescue guard persistence reconciliation failed: " + failure;
					return false;
				}
				if (living <= 0)
					FinalizePhysicalGuardElimination(state, operation, group);
				else if (!SamplePhysicalGuardPositionForPersistence(
					state,
					operation,
					group,
					failure))
				{
					return false;
				}
			}
			array<ref HST_MissionAssetState> captives = {};
			CollectExactCaptives(state, mission, captives);
			if (captives.Count() != EXACT_CAPTIVE_COUNT)
			{
				failure = "exact rescue persistence requires exactly three captive rows";
				QuarantineOperationAuthority(state, operation, failure);
				return false;
			}
			foreach (HST_MissionAssetState captive : captives)
			{
				if (!IsTerminalCaptiveDisposition(captive.m_eRescueDisposition))
				{
					vector deathPosition;
					string deathEvidence;
					if (m_MissionRuntime.TryResolveExactRescueCaptiveDeathEvidence(
						state, mission, captive, deathPosition, deathEvidence))
					{
						captive.m_vCurrentPosition = deathPosition;
						captive.m_vLastKnownPosition = deathPosition;
						MarkCaptiveDeathObserved(state, mission, captive, deathEvidence);
					}
				}
				bool restoredFoldedProjection = state.m_bRestoredFromPersistence
					&& !captive.m_bSpawned
					&& m_MissionRuntime.IsExactRescueCaptiveProjectionAbsent(captive);
				// Startup recapture must preserve the durable BOARDING on-foot pose and
				// BOARDED seat token after process-local projections are deliberately
				// folded. Runtime carrier reconciliation resumes after materialization.
				if (!restoredFoldedProjection)
					ReconcileObservedCarrierState(state, mission, captive);
				if (!IsTerminalCaptiveDisposition(captive.m_eRescueDisposition))
				{
					string poseEvidence;
					if (!m_MissionRuntime.SampleExactRescueRuntimePoseForPersistence(
						state,
						mission,
						captive,
						poseEvidence))
					{
						failure
							= "exact rescue captive persistence pose sample is unresolved: "
								+ poseEvidence;
						return false;
					}
				}
				if (IsTerminalCaptiveDisposition(captive.m_eRescueDisposition)
					&& !m_MissionRuntime.FoldExactRescueCaptiveProjection(
						state, mission, captive, "persistence terminal projection fold"))
				{
					failure = "exact rescue terminal captive projection could not fold";
					return false;
				}
				ApplyCaptiveCompatibilityProjection(captive);
			}
			SyncMissionCaptiveCounters(state, mission);
			SyncMissionObjectiveProjection(state, mission);
		}
		return true;
	}

	protected bool QuarantinedBatchClaimsOperationAuthority(
		HST_ForceSpawnResultState batch,
		HST_OperationRecordState operation)
	{
		if (!batch || !operation || operation.m_sOperationId.IsEmpty()
			|| operation.m_sManifestId.IsEmpty() || operation.m_sSpawnResultId.IsEmpty()
			|| operation.m_sForceId.IsEmpty() || operation.m_sProjectionId.IsEmpty())
			return false;
		return batch.m_sOperationId == operation.m_sOperationId
			&& batch.m_sManifestId == operation.m_sManifestId
			&& batch.m_sResultId == operation.m_sSpawnResultId
			&& batch.m_sForceId == operation.m_sForceId
			&& batch.m_sProjectionId == operation.m_sProjectionId;
	}

	protected bool QuarantinedGroupClaimsOperationAuthority(
		HST_ActiveGroupState group,
		HST_OperationRecordState operation)
	{
		if (!group || !operation || operation.m_sOperationId.IsEmpty()
			|| operation.m_sManifestId.IsEmpty() || operation.m_sSpawnResultId.IsEmpty()
			|| operation.m_sForceId.IsEmpty() || operation.m_sProjectionId.IsEmpty()
			|| operation.m_sGroupId.IsEmpty())
			return false;
		return group.m_sOperationId == operation.m_sOperationId
			&& group.m_sManifestId == operation.m_sManifestId
			&& group.m_sSpawnResultId == operation.m_sSpawnResultId
			&& group.m_sForceId == operation.m_sForceId
			&& group.m_sProjectionId == operation.m_sProjectionId
			&& group.m_sGroupId == operation.m_sGroupId;
	}

	protected bool ResolveQuarantinedSuccessfulProjectionContext(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		out HST_ActiveGroupState group)
	{
		group = null;
		if (!state || !operation || !batch
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
			|| operation.m_iContractVersion != QUARANTINED_CONTRACT_VERSION
			|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| !QuarantinedBatchClaimsOperationAuthority(batch, operation))
			return false;

		int resultMatches;
		int projectionMatches;
		foreach (HST_ForceSpawnResultState candidateBatch : state.m_aForceSpawnResults)
		{
			if (!candidateBatch)
				continue;
			if (candidateBatch.m_sResultId == batch.m_sResultId)
				resultMatches++;
			if (candidateBatch.m_sProjectionId == operation.m_sProjectionId)
				projectionMatches++;
		}
		if (resultMatches != 1 || projectionMatches != 1)
			return false;

		int groupMatches;
		foreach (HST_ActiveGroupState candidateGroup : state.m_aActiveGroups)
		{
			if (!candidateGroup
				|| candidateGroup.m_sProjectionId != operation.m_sProjectionId)
				continue;
			group = candidateGroup;
			groupMatches++;
		}
		if (groupMatches != 1 || !group
			|| CountGroupIdentity(state, group) != 1
			|| !QuarantinedGroupClaimsOperationAuthority(group, operation))
			return false;

		foreach (HST_OperationRecordState competing : state.m_aOperations)
		{
			if (!competing || competing == operation)
				continue;
			if (competing.m_sProjectionId == operation.m_sProjectionId
				|| competing.m_sSpawnResultId == operation.m_sSpawnResultId
				|| competing.m_sGroupId == operation.m_sGroupId
				|| competing.m_sManifestId == operation.m_sManifestId
				|| competing.m_sForceId == operation.m_sForceId)
				return false;
		}
		return true;
	}

	// Schema-58 quarantine deliberately preserves a successful guard batch as
	// held/cancel-requested evidence. After a process restart there is no native
	// projection left to retire, so complete the queue-owned typed cancellation
	// before startup capture asks Rescue authority for its narrower terminal bar.
	protected bool NormalizeRestoredQuarantinedAuthority(
		HST_CampaignState state,
		HST_OperationRecordState operation)
	{
		if (!state || !operation
			|| operation.m_iContractVersion != QUARANTINED_CONTRACT_VERSION
			|| !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar)
			return false;
		bool changed;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (!batch
				|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
				|| !QuarantinedBatchClaimsOperationAuthority(batch, operation))
				continue;
			HST_ActiveGroupState group;
			if (!ResolveQuarantinedSuccessfulProjectionContext(
				state, operation, batch, group))
				continue;
			if (m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0
				|| m_SpawnAdapter.CountHandlesForResultId(batch.m_sResultId) > 0
				|| m_PhysicalWar.GetForceSpawnGroupRoot(group)
				|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0)
				continue;

			ClearGroupProcessAuthority(group);
			group.m_iLifecycleRevision++;
			HST_ForceSpawnQueueCallbackResult terminal
				= m_SpawnQueue.CompleteQuarantinedSuccessfulProjectionCancellation(
					state.m_aForceSpawnResults,
					batch.m_sResultId,
					batch.m_sProjectionId,
					Math.Max(0, state.m_iElapsedSeconds),
					operation.m_sLastProjectionReason);
			if (terminal && terminal.m_bAccepted && IsTerminalSpawnBatch(batch))
				changed = true;
		}
		return changed;
	}

	bool ReconcileAfterRestore(HST_CampaignState state)
	{
		if (!state || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar || !m_MissionRuntime)
			return false;
		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
		{
			bool settled = SettleOpenOperationsForCampaignStop(
				state, "restored campaign phase does not permit open exact rescue authority");
			return ReconcileSettledRuntimeCleanup(state) || settled;
		}
		bool changed;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE)
				continue;
			if (operation.m_iContractVersion == QUARANTINED_CONTRACT_VERSION)
			{
				changed = NormalizeRestoredQuarantinedAuthority(state, operation) || changed;
				changed = ApplyQuarantineStatus(state, operation, operation.m_sLastProjectionReason) || changed;
				continue;
			}
			if (operation.m_iContractVersion != EXACT_CONTRACT_VERSION)
			{
				changed = QuarantineOperationAuthority(state, operation,
					"restore found unsupported exact rescue contract") || changed;
				continue;
			}
			if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				continue;
			HST_ActiveMissionState mission;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			string failure = ResolveRuntimeContext(state, operation, mission, manifest, batch, group);
			if (!failure.IsEmpty())
			{
				changed = QuarantineOperationAuthority(state, operation, failure) || changed;
				continue;
			}
			if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			changed = NormalizeRestoredGuardAuthority(state, operation, manifest, batch, group) || changed;
			array<ref HST_MissionAssetState> captives = {};
			CollectExactCaptives(state, mission, captives);
			foreach (HST_MissionAssetState captive : captives)
			{
				changed = ReconcileDisconnectedEscort(state, mission, captive) || changed;
				if (IsTerminalCaptiveDisposition(captive.m_eRescueDisposition))
				{
					changed = m_MissionRuntime.FoldExactRescueCaptiveProjection(
						state, mission, captive, "restored terminal captive") || changed;
					continue;
				}
				bool wasSpawned = captive.m_bSpawned;
				if (!m_MissionRuntime.FoldRestoredExactRescueCaptiveProjection(
					state,
					mission,
					captive,
					"restored exact rescue process-local projection"))
				{
					Print(
						"Partisan rescue | restored captive projection could not fold before startup recapture",
						LogLevel.WARNING);
				}
				else if (wasSpawned)
					changed = true;
				if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING)
				{
					string escortEvidence;
					m_MissionRuntime.RebindExactRescueEscortProjection(state, mission, captive, escortEvidence);
				}
				else if (captive.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED)
				{
					IEntity carrier;
					string seatToken;
					vector carrierPosition;
					string carrierEvidence;
					bool verified = m_MissionRuntime.ResolveExactRescueCarrierEvidence(
						state, mission, captive, carrier, seatToken, carrierPosition, carrierEvidence);
					if (verified && seatToken.IsEmpty())
					{
						HST_RescuePOWTransitionResult boarding = ApplyCaptiveTransition(
							state, mission, captive,
							HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING,
							captive.m_sRescueEscortIdentityId,
							"restore_seat_unresolved_" + captive.m_sAssetId,
							captive.m_sRescueCarrierVehicleId);
						changed = boarding && boarding.m_bStateChanged || changed;
					}
				}
				changed = ApplyCaptiveCompatibilityProjection(captive) || changed;
			}
			changed = SyncMissionCaptiveCounters(state, mission) || changed;
			changed = SyncMissionObjectiveProjection(state, mission) || changed;
		}
		changed = ReconcileSettledRuntimeCleanup(state) || changed;
		return changed;
	}

	protected bool NormalizeRestoredGuardAuthority(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (group.m_sRuntimeStatus == GUARD_ELIMINATED_STATUS
			|| group.m_sRuntimeStatus == "rescue_pow_guard_retired")
		{
			ClearGroupProcessAuthority(group);
			operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
			operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
			return true;
		}
		if (IsTerminalSpawnBatch(batch))
			return QuarantineOperationAuthority(state, operation,
				"restored exact rescue guard batch is terminal without retirement evidence");
		if (m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0
			|| group.m_bSpawnedEntity || m_PhysicalWar.GetForceSpawnGroupRoot(group)
			|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0)
		{
			HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
				state, m_PhysicalWar, operation.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return QuarantineOperationAuthority(state, operation,
					"restored exact rescue guard runtime could not retire");
		}
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			HST_ForceSpawnQueueCallbackResult requeued = m_SpawnQueue.RequeueSuccessfulProjectionAfterRestore(
				state.m_aForceSpawnResults, manifest, batch.m_sResultId, batch.m_sProjectionId,
				nowSecond, nowSecond + DEPLOYMENT_GRACE_SECONDS);
			if (!requeued || !requeued.m_bAccepted)
				return QuarantineOperationAuthority(state, operation,
					"restored exact rescue guard roster could not requeue");
		}
		if (!batch.m_bStrategicProjectionHeld)
		{
			HST_ForceSpawnQueueCallbackResult held = m_SpawnQueue.HoldPendingProjectionForStrategicSimulation(
				state.m_aForceSpawnResults, manifest, batch.m_sResultId, batch.m_sProjectionId, nowSecond);
			if (!held || !held.m_bAccepted)
				return QuarantineOperationAuthority(state, operation,
					"restored exact rescue guard roster could not enter strategic hold");
		}
		ClearGroupProcessAuthority(group);
		int living = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
		SyncGroupRoster(group, living);
		group.m_sRuntimeStatus = GUARD_VIRTUAL_STATUS;
		group.m_iLifecycleRevision++;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iLastVirtualFriendlyCount = living;
		operation.m_sLastProjectionReason = "restored exact rescue guard as strategic authority";
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iRevision++;
		return true;
	}

	bool PrepareQuarantinedAuthorityForPersistence(HST_CampaignState state, out string failure)
	{
		failure = "";
		if (!state || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar)
		{
			failure = "exact rescue quarantine cleanup services are unavailable";
			return false;
		}
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
				|| operation.m_iContractVersion != QUARANTINED_CONTRACT_VERSION)
				continue;
			NormalizeRestoredQuarantinedAuthority(state, operation);
			ApplyQuarantineStatus(state, operation, operation.m_sLastProjectionReason);
			if (m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0
				|| (!operation.m_sSpawnResultId.IsEmpty()
					&& m_SpawnAdapter.CountHandlesForResultId(operation.m_sSpawnResultId) > 0))
			{
				failure = "quarantined exact rescue retains adapter handles";
				return false;
			}
			HST_ActiveGroupState group = state.FindActiveGroup(operation.m_sGroupId);
			if (group && (m_PhysicalWar.GetForceSpawnGroupRoot(group)
				|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0))
			{
				failure = "quarantined exact rescue retains physical guard runtime";
				return false;
			}
			HST_ForceSpawnResultState batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
			if (batch && !IsTerminalSpawnBatch(batch))
			{
				failure = string.Format(
					"quarantined exact rescue guard batch is not terminal | operation %1 | reason %2 | batch status %3 | held %4 | cancel %5 | terminal reason %6 | last failure %7",
					operation.m_sOperationId,
					operation.m_sLastProjectionReason,
					batch.m_eStatus,
					batch.m_bStrategicProjectionHeld,
					batch.m_bCancelRequested,
					batch.m_sTerminalReason,
					batch.m_sLastFailureReason);
				return false;
			}
		}
		return true;
	}

	protected bool QuarantineOperationAuthority(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		string reason)
	{
		if (!state || !operation)
			return false;
		if (reason.IsEmpty())
			reason = "exact rescue authority conflict";
		HST_ActiveMissionState mission = state.FindActiveMission(operation.m_sMissionInstanceId);
		HST_ActiveGroupState group = state.FindActiveGroup(operation.m_sGroupId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
		if (mission && m_MissionRuntime)
		{
			foreach (HST_MissionAssetState captive : state.m_aMissionAssets)
			{
				if (captive && captive.m_sMissionInstanceId == mission.m_sInstanceId)
					m_MissionRuntime.FoldExactRescueCaptiveProjection(
						state, mission, captive, "exact rescue quarantine");
			}
		}
		if (group && (m_SpawnAdapter && m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0
			|| m_PhysicalWar && (m_PhysicalWar.GetForceSpawnGroupRoot(group)
				|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0)))
		{
			HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
				state, m_PhysicalWar, operation.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return false;
		}
		if (batch && !IsTerminalSpawnBatch(batch) && m_SpawnQueue)
		{
			HST_ForceSpawnQueueCallbackResult terminal;
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
				terminal = m_SpawnQueue.CompleteQuarantinedSuccessfulProjectionCancellation(
					state.m_aForceSpawnResults, batch.m_sResultId, batch.m_sProjectionId,
					Math.Max(0, state.m_iElapsedSeconds), reason);
			else
				terminal = m_SpawnQueue.RequestCancel(state.m_aForceSpawnResults,
					batch.m_sResultId, Math.Max(0, state.m_iElapsedSeconds), reason);
			if (!terminal || !terminal.m_bAccepted)
				return false;
		}
		operation.m_iContractVersion = QUARANTINED_CONTRACT_VERSION;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		if (mission)
		{
			mission.m_iOperationContractVersion = QUARANTINED_CONTRACT_VERSION;
			mission.m_bRescueExtractionGrace = false;
			mission.m_iRescueGraceUntilSecond = 0;
			mission.m_sRuntimePhase = QUARANTINE_STATUS;
			mission.m_sRuntimeFailureReason = reason;
		}
		if (group)
		{
			ClearGroupProcessAuthority(group);
			group.m_sSpawnFallbackMode = QUARANTINE_STATUS;
			group.m_sRuntimeStatus = QUARANTINE_STATUS;
			group.m_sSpawnFailureReason = reason;
			group.m_iLifecycleRevision++;
		}
		foreach (HST_MissionAssetState captive : state.m_aMissionAssets)
		{
			if (!captive || captive.m_sOperationId != operation.m_sOperationId)
				continue;
			captive.m_iRescueContractVersion = QUARANTINED_CONTRACT_VERSION;
			captive.m_sRescueLastCommandResult = "quarantined:" + reason;
			captive.m_iRescueRevision++;
		}
		return true;
	}

	protected bool ApplyQuarantineStatus(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		string reason)
	{
		if (!state || !operation)
			return false;
		bool changed;
		if (operation.m_iContractVersion != QUARANTINED_CONTRACT_VERSION)
		{
			operation.m_iContractVersion = QUARANTINED_CONTRACT_VERSION;
			changed = true;
		}
		HST_ActiveMissionState mission = state.FindActiveMission(operation.m_sMissionInstanceId);
		if (mission && (mission.m_iOperationContractVersion != QUARANTINED_CONTRACT_VERSION
			|| mission.m_bRescueExtractionGrace || mission.m_iRescueGraceUntilSecond != 0))
		{
			mission.m_iOperationContractVersion = QUARANTINED_CONTRACT_VERSION;
			mission.m_bRescueExtractionGrace = false;
			mission.m_iRescueGraceUntilSecond = 0;
			mission.m_sRuntimePhase = QUARANTINE_STATUS;
			mission.m_sRuntimeFailureReason = reason;
			changed = true;
		}
		HST_ActiveGroupState group = state.FindActiveGroup(operation.m_sGroupId);
		if (group && (group.m_sSpawnFallbackMode != QUARANTINE_STATUS
			|| group.m_sRuntimeStatus != QUARANTINE_STATUS))
		{
			ClearGroupProcessAuthority(group);
			group.m_sSpawnFallbackMode = QUARANTINE_STATUS;
			group.m_sRuntimeStatus = QUARANTINE_STATUS;
			group.m_sSpawnFailureReason = reason;
			group.m_iLifecycleRevision++;
			changed = true;
		}
		foreach (HST_MissionAssetState captive : state.m_aMissionAssets)
		{
			if (!captive || captive.m_sOperationId != operation.m_sOperationId
				|| captive.m_iRescueContractVersion == QUARANTINED_CONTRACT_VERSION)
				continue;
			captive.m_iRescueContractVersion = QUARANTINED_CONTRACT_VERSION;
			captive.m_sRescueLastCommandResult = "quarantined:" + reason;
			captive.m_iRescueRevision++;
			changed = true;
		}
		return changed;
	}

	protected void QuarantineUncommittedMission(HST_ActiveMissionState mission, string reason)
	{
		if (!mission)
			return;
		mission.m_iOperationContractVersion = QUARANTINED_CONTRACT_VERSION;
		mission.m_bRescueExtractionGrace = false;
		mission.m_iRescueGraceUntilSecond = 0;
		mission.m_sRuntimePhase = QUARANTINE_STATUS;
		mission.m_sRuntimeFailureReason = reason;
	}
}
