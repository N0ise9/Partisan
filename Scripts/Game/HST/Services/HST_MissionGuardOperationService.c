class HST_MissionGuardAdmissionResult
{
	bool m_bSuccess;
	bool m_bAlreadyApplied;
	bool m_bStateChanged;
	string m_sFailureReason;
	ref HST_ActiveMissionState m_Mission;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceManifestState m_Manifest;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
}

class HST_MissionGuardAdmissionPlan
{
	ref HST_MissionAssetState m_HVT;
	ref HST_ForceCompositionResult m_Composition;
	ref HST_ForceGroupCatalogEntry m_CatalogGroup;
	ref HST_ForceManifestState m_Manifest;
	ref HST_OperationRecordState m_Operation;
	ref HST_ActiveGroupState m_Group;
	vector m_vTargetPosition;
	vector m_vAssignmentPosition;
}

// Exact mission-owned guard authority for newly prepared assassination missions.
// The HVT remains mission-runtime authority and is only referenced as an assignment anchor.
class HST_MissionGuardOperationService
{
	static const int EXACT_CONTRACT_VERSION = 1;
	static const int QUARANTINED_CONTRACT_VERSION = -55;
	static const int EXACT_PROJECTION_CONTRACT_VERSION = 1;
	static const string EXACT_MISSION_ID = "assassinate_officer";
	static const string EXACT_FORCE_KIND = "mission_guard";
	static const string EXACT_POLICY_ID = "exact_assassinate_officer_guard_v1";
	static const string EXACT_INTENT_ID = "assassinate_officer_guard";
	static const int OFFICER_CONTRACT_VERSION = 1;
	static const int OFFICER_QUARANTINED_CONTRACT_VERSION = -55;
	static const string OFFICER_MISSION_ID = "assassinate_officer";
	static const string OFFICER_POLICY_ID = "exact_assassinate_officer_guard_v1";
	static const string OFFICER_INTENT_ID = "assassinate_officer_guard";
	static const int TRAITOR_CONTRACT_VERSION = 2;
	static const int TRAITOR_QUARANTINED_CONTRACT_VERSION = -56;
	static const string TRAITOR_MISSION_ID = "assassinate_traitor";
	static const string TRAITOR_POLICY_ID = "exact_assassinate_traitor_guard_v1";
	static const string TRAITOR_INTENT_ID = "assassinate_traitor_guard";
	static const int SPECOPS_CONTRACT_VERSION = 3;
	static const int SPECOPS_QUARANTINED_CONTRACT_VERSION = -57;
	static const string SPECOPS_MISSION_ID = "assassinate_specops";
	static const string SPECOPS_POLICY_ID = "exact_assassinate_specops_guard_v1";
	static const string SPECOPS_INTENT_ID = "assassinate_specops_guard";
	static const string EXACT_GROUP_MODE = "exact_mission_guard";
	static const string ASSIGNMENT_KIND = "guard_mission_target";
	static const string RECALL_POLICY_ID = "no_recall";
	static const string SETTLEMENT_POLICY_ID = "mission_owned_no_refund";
	static const string SETTLEMENT_KIND = "exact_mission_guard_terminal";
	static const string QUARANTINE_STATUS = "exact_mission_guard_quarantined";
	static const int EXACT_PRIORITY = 70;
	static const int EXACT_MAX_RETRIES = 3;
	static const int DEPLOYMENT_GRACE_SECONDS = 180;
	static const int CONTACT_CLEAR_SECONDS = 30;
	static const float CONTACT_EVIDENCE_RADIUS_METERS = 140.0;

	protected ref HST_ForcePlanningIntegrityService m_Integrity = new HST_ForcePlanningIntegrityService();
	protected ref HST_ForceCatalogService m_Catalog = new HST_ForceCatalogService();
	protected ref HST_MaterializationService m_Materialization = new HST_MaterializationService();
	protected ref HST_ForceSpawnQueueService m_SpawnQueue;
	protected ref HST_ForceSpawnAdapterService m_SpawnAdapter;
	protected ref HST_PhysicalWarService m_PhysicalWar;

	void SetRuntimeServices(
		HST_ForceSpawnQueueService spawnQueue,
		HST_ForceSpawnAdapterService spawnAdapter,
		HST_PhysicalWarService physicalWar)
	{
		m_SpawnQueue = spawnQueue;
		m_SpawnAdapter = spawnAdapter;
		m_PhysicalWar = physicalWar;
	}

	static bool IsExactMission(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;
		int expected = ResolveExpectedContractVersion(mission.m_sMissionId);
		return expected > 0 && mission.m_iOperationContractVersion == expected;
	}

	static bool IsQuarantinedMission(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;
		int expected = ResolveQuarantinedContractVersion(mission.m_sMissionId);
		return expected < 0 && mission.m_iOperationContractVersion == expected;
	}

	static bool IsExactOrQuarantinedMission(HST_ActiveMissionState mission)
	{
		return IsExactMission(mission) || IsQuarantinedMission(mission);
	}

	static bool IsSupportedExactMissionId(string missionId)
	{
		return missionId == OFFICER_MISSION_ID || missionId == TRAITOR_MISSION_ID
			|| missionId == SPECOPS_MISSION_ID;
	}

	static bool IsSupportedExactContractVersion(int contractVersion)
	{
		return contractVersion == OFFICER_CONTRACT_VERSION
			|| contractVersion == TRAITOR_CONTRACT_VERSION
			|| contractVersion == SPECOPS_CONTRACT_VERSION;
	}

	static bool IsQuarantinedOperationContractVersion(int contractVersion)
	{
		return contractVersion == OFFICER_QUARANTINED_CONTRACT_VERSION
			|| contractVersion == TRAITOR_QUARANTINED_CONTRACT_VERSION
			|| contractVersion == SPECOPS_QUARANTINED_CONTRACT_VERSION;
	}

	static int ResolveExpectedContractVersion(string missionId)
	{
		if (missionId == OFFICER_MISSION_ID)
			return OFFICER_CONTRACT_VERSION;
		if (missionId == TRAITOR_MISSION_ID)
			return TRAITOR_CONTRACT_VERSION;
		if (missionId == SPECOPS_MISSION_ID)
			return SPECOPS_CONTRACT_VERSION;
		return 0;
	}

	static string ResolveExpectedPolicyId(string missionId)
	{
		if (missionId == OFFICER_MISSION_ID)
			return OFFICER_POLICY_ID;
		if (missionId == TRAITOR_MISSION_ID)
			return TRAITOR_POLICY_ID;
		if (missionId == SPECOPS_MISSION_ID)
			return SPECOPS_POLICY_ID;
		return "";
	}

	static string ResolveExpectedIntentId(string missionId)
	{
		if (missionId == OFFICER_MISSION_ID)
			return OFFICER_INTENT_ID;
		if (missionId == TRAITOR_MISSION_ID)
			return TRAITOR_INTENT_ID;
		if (missionId == SPECOPS_MISSION_ID)
			return SPECOPS_INTENT_ID;
		return "";
	}

	static int ResolveQuarantinedContractVersion(string missionId)
	{
		if (missionId == OFFICER_MISSION_ID)
			return OFFICER_QUARANTINED_CONTRACT_VERSION;
		if (missionId == TRAITOR_MISSION_ID)
			return TRAITOR_QUARANTINED_CONTRACT_VERSION;
		if (missionId == SPECOPS_MISSION_ID)
			return SPECOPS_QUARANTINED_CONTRACT_VERSION;
		return 0;
	}

	static int ResolveExpectedQuarantineVersion(string missionId)
	{
		return ResolveQuarantinedContractVersion(missionId);
	}

	static int ResolveQuarantinedContractVersionForExactContract(int contractVersion)
	{
		if (contractVersion == OFFICER_CONTRACT_VERSION)
			return OFFICER_QUARANTINED_CONTRACT_VERSION;
		if (contractVersion == TRAITOR_CONTRACT_VERSION)
			return TRAITOR_QUARANTINED_CONTRACT_VERSION;
		if (contractVersion == SPECOPS_CONTRACT_VERSION)
			return SPECOPS_QUARANTINED_CONTRACT_VERSION;
		if (IsQuarantinedOperationContractVersion(contractVersion))
			return contractVersion;
		return 0;
	}

	static bool IsExactOfficerMission(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sMissionId == OFFICER_MISSION_ID
			&& mission.m_iOperationContractVersion == OFFICER_CONTRACT_VERSION;
	}

	static bool IsQuarantinedOfficerMission(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sMissionId == OFFICER_MISSION_ID
			&& mission.m_iOperationContractVersion == OFFICER_QUARANTINED_CONTRACT_VERSION;
	}

	static bool IsExactTraitorMission(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sMissionId == TRAITOR_MISSION_ID
			&& mission.m_iOperationContractVersion == TRAITOR_CONTRACT_VERSION;
	}

	static bool IsQuarantinedTraitorMission(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sMissionId == TRAITOR_MISSION_ID
			&& mission.m_iOperationContractVersion == TRAITOR_QUARANTINED_CONTRACT_VERSION;
	}

	static bool IsExactSpecOpsMission(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sMissionId == SPECOPS_MISSION_ID
			&& mission.m_iOperationContractVersion == SPECOPS_CONTRACT_VERSION;
	}

	static bool IsQuarantinedSpecOpsMission(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sMissionId == SPECOPS_MISSION_ID
			&& mission.m_iOperationContractVersion == SPECOPS_QUARANTINED_CONTRACT_VERSION;
	}

	static bool IsMissionGuardGroupClaimant(HST_CampaignState state, HST_ActiveGroupState group)
	{
		if (!state || !group)
			return false;
		HST_OperationRecordState operation = state.FindOperation(group.m_sOperationId);
		if (operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
			&& operation.m_sOperationId == group.m_sOperationId)
		{
			if ((!operation.m_sGroupId.IsEmpty() && operation.m_sGroupId == group.m_sGroupId)
				|| (!operation.m_sProjectionId.IsEmpty() && operation.m_sProjectionId == group.m_sProjectionId)
				|| (!operation.m_sManifestId.IsEmpty() && operation.m_sManifestId == group.m_sManifestId)
				|| (!operation.m_sSpawnResultId.IsEmpty() && operation.m_sSpawnResultId == group.m_sSpawnResultId))
				return true;
		}

		HST_ActiveMissionState mission = state.FindActiveMission(group.m_sMissionInstanceId);
		if (IsExactOrQuarantinedMission(mission) && !group.m_sOperationId.IsEmpty()
			&& mission.m_sOperationId == group.m_sOperationId)
		{
			if ((!mission.m_sManifestId.IsEmpty() && mission.m_sManifestId == group.m_sManifestId)
				|| (!mission.m_sSpawnResultId.IsEmpty() && mission.m_sSpawnResultId == group.m_sSpawnResultId))
				return true;
		}

		bool exactMode = group.m_sSpawnFallbackMode == EXACT_GROUP_MODE
			|| group.m_sSpawnFallbackMode == QUARANTINE_STATUS;
		bool exactStatus = group.m_sRuntimeStatus.StartsWith("mission_guard_")
			|| group.m_sRuntimeStatus == QUARANTINE_STATUS;
		if (!exactMode && !exactStatus)
			return false;
		HST_ForceManifestState manifest = state.FindForceManifest(group.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(group.m_sSpawnResultId);
		return manifest && batch && !group.m_sOperationId.IsEmpty()
			&& manifest.m_sOperationId == group.m_sOperationId
			&& batch.m_sOperationId == group.m_sOperationId
			&& batch.m_sManifestId == manifest.m_sManifestId
			&& batch.m_sProjectionId == group.m_sProjectionId;
	}

	static bool IsExactMissionGuardGroup(HST_CampaignState state, HST_ActiveGroupState group)
	{
		if (!IsMissionGuardGroupClaimant(state, group))
			return false;
		HST_ActiveMissionState mission = state.FindActiveMission(group.m_sMissionInstanceId);
		HST_OperationRecordState operation = state.FindOperation(group.m_sOperationId);
		int expectedContract;
		string expectedPolicy;
		string expectedIntent;
		if (mission)
		{
			expectedContract = ResolveExpectedContractVersion(mission.m_sMissionId);
			expectedPolicy = ResolveExpectedPolicyId(mission.m_sMissionId);
			expectedIntent = ResolveExpectedIntentId(mission.m_sMissionId);
		}
		if (!IsExactMission(mission) || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE
			|| !operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
			|| operation.m_iContractVersion != expectedContract
			|| operation.m_iProjectionContractVersion != EXACT_PROJECTION_CONTRACT_VERSION
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
			return false;
		HST_ForceManifestState manifest = state.FindForceManifest(group.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(group.m_sSpawnResultId);
		if (!manifest || !batch || !manifest.m_bFrozen
			|| manifest.m_sPolicyId != expectedPolicy || manifest.m_sForceKind != EXACT_FORCE_KIND
			|| manifest.m_sIntentId != expectedIntent || manifest.m_aGroups.Count() != 1
			|| !manifest.m_aGroups[0] || manifest.m_aGroups[0].m_sPrefab != group.m_sPrefab)
			return false;
		if (mission.m_sOperationId != operation.m_sOperationId
			|| mission.m_sManifestId != manifest.m_sManifestId
			|| mission.m_sSpawnResultId != batch.m_sResultId)
			return false;
		if (operation.m_sMissionInstanceId != mission.m_sInstanceId
			|| operation.m_sManifestId != manifest.m_sManifestId
			|| operation.m_sSpawnResultId != batch.m_sResultId
			|| operation.m_sForceId != group.m_sForceId
			|| operation.m_sProjectionId != group.m_sProjectionId
			|| operation.m_sGroupId != group.m_sGroupId
			|| operation.m_sOwnerFactionKey != group.m_sFactionKey
			|| operation.m_sAssignmentZoneId != group.m_sZoneId)
			return false;
		if (manifest.m_sOperationId != operation.m_sOperationId
			|| manifest.m_sFactionKey != group.m_sFactionKey
			|| manifest.m_sTargetZoneId != group.m_sZoneId)
			return false;
		if (batch.m_sResultId != operation.m_sSpawnResultId
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| batch.m_sManifestHash != manifest.m_sManifestHash
			|| batch.m_sOperationId != operation.m_sOperationId
			|| batch.m_sForceId != operation.m_sForceId
			|| batch.m_sProjectionId != operation.m_sProjectionId)
			return false;
		return !group.m_sGroupId.IsEmpty() && group.m_sGroupId == group.m_sProjectionId
			&& group.m_sOperationId == operation.m_sOperationId
			&& group.m_sManifestId == manifest.m_sManifestId
			&& group.m_sSpawnResultId == batch.m_sResultId
			&& group.m_sMissionAssetId.IsEmpty()
			&& group.m_sSpawnFallbackMode == EXACT_GROUP_MODE;
	}

	static string BuildGuardStatusText(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || !IsSupportedExactMissionId(mission.m_sMissionId)
			|| mission.m_iOperationContractVersion == 0)
			return "";
		if (IsQuarantinedMission(mission))
			return "guard authority unavailable";
		if (!IsExactMission(mission))
			return "guard authority unavailable";
		HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
		int expectedContract = ResolveExpectedContractVersion(mission.m_sMissionId);
		if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
			|| operation.m_iContractVersion != expectedContract)
			return "guard authority unavailable";
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED)
				return "guards neutralized";
			if (operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
				|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED)
				return "";
			return "guard authority unavailable";
		}
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
		HST_ActiveGroupState group = state.FindActiveGroup(operation.m_sGroupId);
		if (!batch || !group || !IsExactMissionGuardGroup(state, group))
			return "guard authority unavailable";
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		int living;
		if (batch.m_bStrategicProjectionHeld)
			living = queue.CountStrategicLivingMemberSlots(batch);
		else
			living = queue.CountDurableLivingMemberSlots(batch);
		return string.Format("guards %1", Math.Max(0, living));
	}

	static string BuildOperationId(string missionInstanceId)
	{
		return HST_StableIdService.BuildOperationId("mission_guard", missionInstanceId);
	}

	static string BuildManifestId(string missionInstanceId)
	{
		if (missionInstanceId.IsEmpty())
			return "";
		return "manifest_mission_guard_" + missionInstanceId;
	}

	static string BuildSpawnResultId(string missionInstanceId)
	{
		if (missionInstanceId.IsEmpty())
			return "";
		return "spawn_mission_guard_" + missionInstanceId;
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
		return "mission_guard_group_" + missionInstanceId;
	}

	static string BuildMemberSlotId(string missionInstanceId, int ordinal)
	{
		if (missionInstanceId.IsEmpty() || ordinal < 0)
			return "";
		return string.Format("mission_guard_member_%1_%2", missionInstanceId, ordinal + 1);
	}

	bool PrepareNewMissionContract(HST_ActiveMissionState mission)
	{
		if (!mission || !IsSupportedExactMissionId(mission.m_sMissionId))
			return false;
		if (IsExactMission(mission))
			return true;
		if (mission.m_iOperationContractVersion != 0 || !mission.m_sOperationId.IsEmpty()
			|| !mission.m_sManifestId.IsEmpty() || !mission.m_sSpawnResultId.IsEmpty()
			|| !mission.m_sSettlementId.IsEmpty())
			return false;
		mission.m_iOperationContractVersion = ResolveExpectedContractVersion(mission.m_sMissionId);
		return true;
	}

	HST_MissionGuardAdmissionResult CanAdmitNewMission(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_MissionDefinition definition,
		HST_ActiveMissionState mission,
		HST_MissionRuntimeService missionRuntime)
	{
		HST_MissionGuardAdmissionResult result = new HST_MissionGuardAdmissionResult();
		result.m_Mission = mission;
		if (!state || !preset || !definition || !mission || !missionRuntime || !m_SpawnQueue
			|| !m_SpawnAdapter || !m_PhysicalWar || !m_Integrity || !m_Catalog)
		{
			result.m_sFailureReason = "exact mission guard admission services are unavailable";
			return result;
		}
		if (HasAnyAdmissionAuthority(state, mission))
			return ResolveCommittedAdmission(state, mission);
		HST_MissionGuardAdmissionPlan plan;
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
			result.m_sFailureReason = "exact mission guard spawn admission preflight failed";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = result.m_sFailureReason + ": " + preflight.m_sFailureReason;
			return result;
		}
		result.m_bSuccess = true;
		result.m_Operation = plan.m_Operation;
		result.m_Manifest = plan.m_Manifest;
		result.m_Group = plan.m_Group;
		return result;
	}

	HST_MissionGuardAdmissionResult AdmitNewMission(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_MissionDefinition definition,
		HST_ActiveMissionState mission,
		HST_MissionRuntimeService missionRuntime)
	{
		if (HasAnyAdmissionAuthority(state, mission))
			return ResolveCommittedAdmission(state, mission);

		HST_MissionGuardAdmissionResult result = new HST_MissionGuardAdmissionResult();
		result.m_Mission = mission;
		HST_MissionGuardAdmissionResult preflight = CanAdmitNewMission(
			state,
			preset,
			definition,
			mission,
			missionRuntime);
		if (!preflight || !preflight.m_bSuccess)
		{
			result.m_sFailureReason = "exact mission guard admission preflight failed";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = preflight.m_sFailureReason;
			QuarantineUncommittedMission(state, mission, result.m_sFailureReason);
			result.m_bStateChanged = true;
			return result;
		}

		HST_MissionGuardAdmissionPlan plan;
		string failure = BuildAdmissionPlan(state, preset, definition, mission, missionRuntime, plan);
		if (!failure.IsEmpty() || !plan || !plan.m_Manifest || !plan.m_Operation || !plan.m_Group)
		{
			result.m_sFailureReason = failure;
			if (result.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = "exact mission guard detached authority could not be built";
			QuarantineUncommittedMission(state, mission, result.m_sFailureReason);
			result.m_bStateChanged = true;
			return result;
		}

		state.m_aForceManifests.Insert(plan.m_Manifest);
		state.m_aOperations.Insert(plan.m_Operation);
		state.m_aActiveGroups.Insert(plan.m_Group);
		result.m_Manifest = plan.m_Manifest;
		result.m_Operation = plan.m_Operation;
		result.m_Group = plan.m_Group;
		result.m_bStateChanged = true;

		HST_ForceSpawnQueueEnqueueResult enqueue = m_SpawnQueue.Enqueue(
			state.m_aForceSpawnResults,
			plan.m_Manifest,
			BuildSpawnRequest(state, mission),
			Math.Max(0, state.m_iElapsedSeconds));
		if (!enqueue || !enqueue.m_bSuccess || !enqueue.m_Batch || enqueue.m_bAlreadyApplied)
		{
			failure = "exact mission guard spawn admission failed";
			if (enqueue && !enqueue.m_sFailureReason.IsEmpty())
				failure = failure + ": " + enqueue.m_sFailureReason;
			HST_ForceSpawnResultState failedBatch;
			if (enqueue)
				failedBatch = enqueue.m_Batch;
			bool rolledBack = RollbackAdmission(state, mission, plan.m_Operation, plan.m_Manifest,
				failedBatch, plan.m_Group, failure);
			if (!rolledBack)
			{
				QuarantineOperationAuthority(state, plan.m_Operation,
					"exact mission guard admission rollback could not be proven: " + failure);
				QuarantineUncommittedMission(state, mission, failure);
			}
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
			failure = "exact mission guard strategic hold failed";
			if (held && !held.m_sFailureReason.IsEmpty())
				failure = failure + ": " + held.m_sFailureReason;
			bool rolledBack = RollbackAdmission(state, mission, plan.m_Operation, plan.m_Manifest,
				enqueue.m_Batch, plan.m_Group, failure);
			if (!rolledBack)
			{
				QuarantineOperationAuthority(state, plan.m_Operation,
					"exact mission guard admission rollback could not be proven: " + failure);
				QuarantineUncommittedMission(state, mission, failure);
			}
			result.m_sFailureReason = failure;
			return result;
		}

		mission.m_sOperationId = plan.m_Operation.m_sOperationId;
		mission.m_sManifestId = plan.m_Manifest.m_sManifestId;
		mission.m_sSpawnResultId = enqueue.m_Batch.m_sResultId;
		failure = ValidateCommittedGraph(state, mission, plan.m_Operation, plan.m_Manifest, enqueue.m_Batch, plan.m_Group, true);
		if (!failure.IsEmpty())
		{
			string graphFailure = "exact mission guard committed graph failed: " + failure;
			bool rolledBack = RollbackAdmission(state, mission, plan.m_Operation, plan.m_Manifest,
				enqueue.m_Batch, plan.m_Group, graphFailure);
			if (!rolledBack)
			{
				QuarantineOperationAuthority(state, plan.m_Operation,
					"exact mission guard admission rollback could not be proven: " + graphFailure);
				QuarantineUncommittedMission(state, mission, graphFailure);
			}
			result.m_sFailureReason = "exact mission guard committed graph failed: " + failure;
			return result;
		}

		result.m_bSuccess = true;
		return result;
	}

	HST_MissionGuardAdmissionResult ResolveCommittedAdmission(
		HST_CampaignState state,
		HST_ActiveMissionState mission)
	{
		HST_MissionGuardAdmissionResult result = new HST_MissionGuardAdmissionResult();
		result.m_Mission = mission;
		if (!state || !mission || !IsExactMission(mission))
		{
			result.m_sFailureReason = "exact mission guard committed replay context is incomplete";
			return result;
		}
		HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
		if (!operation && mission.m_sOperationId.IsEmpty())
			operation = state.FindOperation(BuildOperationId(mission.m_sInstanceId));
		HST_ForceManifestState manifest = state.FindForceManifest(mission.m_sManifestId);
		if (!manifest && mission.m_sManifestId.IsEmpty())
			manifest = state.FindForceManifest(BuildManifestId(mission.m_sInstanceId));
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		if (!batch && mission.m_sSpawnResultId.IsEmpty())
			batch = state.FindForceSpawnResult(BuildSpawnResultId(mission.m_sInstanceId));
		HST_ActiveGroupState group;
		if (operation)
			group = state.FindActiveGroup(operation.m_sGroupId);
		string failure = ValidateCommittedGraph(state, mission, operation, manifest, batch, group, false);
		if (!failure.IsEmpty())
		{
			result.m_sFailureReason = failure;
			bool interruptedAdmission = mission.m_sOperationId.IsEmpty()
				|| mission.m_sManifestId.IsEmpty() || mission.m_sSpawnResultId.IsEmpty();
			if (operation)
				QuarantineOperationAuthority(state, operation, failure);
			if (!operation || interruptedAdmission)
				QuarantineUncommittedMission(state, mission, failure);
			result.m_bStateChanged = true;
			return result;
		}
		result.m_bSuccess = true;
		result.m_bAlreadyApplied = true;
		result.m_Operation = operation;
		result.m_Manifest = manifest;
		result.m_Batch = batch;
		result.m_Group = group;
		return result;
	}

	bool RollbackAdmission(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		if (!state || !mission || !manifest)
			return false;
		if (operation && (state.m_aOperations.Find(operation) < 0
			|| operation.m_sMissionInstanceId != mission.m_sInstanceId
			|| operation.m_sManifestId != manifest.m_sManifestId
			|| CountOperationIdentity(state, operation) != 1))
			return false;
		if (group && (state.m_aActiveGroups.Find(group) < 0 || group.m_bSpawnedEntity
			|| !group.m_sRuntimeEntityId.IsEmpty() || group.m_iSpawnedAgentCount > 0
			|| group.m_sMissionInstanceId != mission.m_sInstanceId
			|| group.m_sManifestId != manifest.m_sManifestId
			|| CountGroupIdentity(state, group) != 1))
			return false;
		if (batch && state.m_aForceSpawnResults.Find(batch) < 0)
			batch = null;
		if (batch && (batch.m_sManifestId != manifest.m_sManifestId
			|| CountBatchIdentity(state, batch) != 1
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING))
			return false;
		if (CountManifestIdentity(state, manifest) != 1)
			return false;

		if (batch)
		{
			if (m_SpawnQueue)
			{
				HST_ForceSpawnQueueCallbackResult cancelled = m_SpawnQueue.RequestCancel(
					state.m_aForceSpawnResults,
					batch.m_sResultId,
					Math.Max(0, state.m_iElapsedSeconds),
					reason);
				if (!cancelled || !cancelled.m_bAccepted || !IsTerminalSpawnBatch(batch))
					return false;
			}
			int batchIndex = state.m_aForceSpawnResults.Find(batch);
			if (batchIndex >= 0)
				state.m_aForceSpawnResults.Remove(batchIndex);
		}
		if (group)
		{
			int groupIndex = state.m_aActiveGroups.Find(group);
			if (groupIndex >= 0)
				state.m_aActiveGroups.Remove(groupIndex);
		}
		if (operation)
		{
			int operationIndex = state.m_aOperations.Find(operation);
			if (operationIndex >= 0)
				state.m_aOperations.Remove(operationIndex);
		}
		int manifestIndex = state.m_aForceManifests.Find(manifest);
		if (manifestIndex >= 0)
			state.m_aForceManifests.Remove(manifestIndex);

		if (mission.m_sOperationId == BuildOperationId(mission.m_sInstanceId))
			mission.m_sOperationId = "";
		if (mission.m_sManifestId == BuildManifestId(mission.m_sInstanceId))
			mission.m_sManifestId = "";
		if (mission.m_sSpawnResultId == BuildSpawnResultId(mission.m_sInstanceId))
			mission.m_sSpawnResultId = "";
		mission.m_sSettlementId = "";
		QuarantineUncommittedMission(state, mission, reason);
		return true;
	}

	protected string BuildAdmissionPlan(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_MissionDefinition definition,
		HST_ActiveMissionState mission,
		HST_MissionRuntimeService missionRuntime,
		out HST_MissionGuardAdmissionPlan plan)
	{
		plan = null;
		if (!state || !preset || !definition || !mission || !missionRuntime)
			return "exact mission guard detached planning context is missing";
		if (!IsExactMission(mission) || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE
			|| definition.m_sMissionId != mission.m_sMissionId || mission.m_sInstanceId.IsEmpty()
			|| mission.m_sTargetZoneId.IsEmpty())
			return "mission did not opt into a new active exact guard contract";
		if (state.FindActiveMission(mission.m_sInstanceId) != mission
			|| CountMissionIdentity(state, mission) != 1)
			return "exact mission guard mission identity is ambiguous";

		HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
		if (!zone || zone.m_sOwnerFactionKey.IsEmpty())
			return "exact mission guard target zone is unavailable";
		if (!HST_FactionRelationService.IsEnemyFaction(preset, zone.m_sOwnerFactionKey))
			return "exact mission guard target zone is not enemy-owned";

		HST_MissionAssetState hvt;
		int hvtCount;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != "hvt")
				continue;
			hvt = asset;
			hvtCount++;
		}
		if (hvtCount != 1 || !hvt || hvt.m_sAssetId.IsEmpty())
			return "exact mission guard requires one unique HVT assignment anchor";
		vector hvtPosition = ResolveHVTPosition(hvt);
		if (IsZeroVector(hvtPosition))
			return "exact mission guard HVT assignment position is unavailable";
		vector assignmentPosition = ResolveGuardAnchor(mission.m_sInstanceId, hvtPosition);
		if (IsZeroVector(assignmentPosition))
			return "exact mission guard safe offset assignment anchor is unavailable";

		HST_ForceCompositionResult composition = missionRuntime.ComposeMissionGuardForce(
			state,
			preset,
			definition,
			mission);
		HST_GroupSpawnPlan groupPlan = SelectExactExecutionGroupPlan(composition);
		if (!composition || !composition.m_bSuccess || !groupPlan || groupPlan.m_sPrefab.IsEmpty()
			|| composition.m_iVehicleCount != 0 || composition.m_aVehicles.Count() != 0
			|| composition.m_aStatics.Count() != 0)
			return "exact mission guard composition did not resolve a route-less infantry group";
		if (composition.m_sFactionKey != zone.m_sOwnerFactionKey)
			return "exact mission guard composition faction conflicts with target ownership";

		HST_ForceCatalogValidationResult catalogValidation = m_Catalog.ValidateFactionCatalog(
			composition.m_sFactionKey,
			true);
		if (!catalogValidation || !catalogValidation.m_bValid)
		{
			string catalogFailure = "exact mission guard faction catalog is invalid";
			if (catalogValidation && !catalogValidation.m_sFailureReason.IsEmpty())
				catalogFailure = catalogFailure + ": " + catalogValidation.m_sFailureReason;
			return catalogFailure;
		}
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
			return "exact mission guard composition has no unique empty execution root";

		plan = new HST_MissionGuardAdmissionPlan();
		plan.m_HVT = hvt;
		plan.m_Composition = composition;
		plan.m_CatalogGroup = catalogGroup;
		plan.m_vTargetPosition = hvtPosition;
		plan.m_vAssignmentPosition = assignmentPosition;
		plan.m_Manifest = BuildManifest(state, mission, zone, catalogGroup);
		plan.m_Operation = BuildOperation(state, mission, zone, hvt, plan.m_Manifest,
			assignmentPosition, hvtPosition);
		plan.m_Group = BuildActiveGroup(state, mission, zone, hvt, composition, groupPlan,
			catalogGroup, plan.m_Manifest, assignmentPosition);
		if (!plan.m_Manifest || !plan.m_Operation || !plan.m_Group)
			return "exact mission guard detached authority construction failed";
		return ValidateDetachedPlan(mission, zone, hvt, plan);
	}

	// The broad mission composer may satisfy a large guard request with several
	// groups. Exact mission-guard projection is intentionally one-root, so freeze
	// the strongest executable proposal and use source order as the stable tie
	// breaker. Unselected proposals never become manifest or runtime authority.
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
		manifest.m_sIntentId = ResolveExpectedIntentId(mission.m_sMissionId);
		manifest.m_sTargetZoneId = zone.m_sZoneId;
		manifest.m_sGroupPrefab = catalogGroup.m_sExecutionPrefab;
		manifest.m_sCatalogVersion = HST_ForceCatalogService.CATALOG_VERSION;
		manifest.m_sPolicyId = ResolveExpectedPolicyId(mission.m_sMissionId);
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
			state,
			mission.m_sInstanceId + "|mission_guard",
			zone.m_sZoneId);
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
		manifest.m_sManifestHash = m_Integrity.BuildManifestHash(manifest);
		if (manifest.m_sManifestHash.IsEmpty())
			return null;
		return manifest;
	}

	protected HST_OperationRecordState BuildOperation(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ZoneState zone,
		HST_MissionAssetState hvt,
		HST_ForceManifestState manifest,
		vector assignmentPosition,
		vector hvtPosition)
	{
		if (!state || !mission || !zone || !hvt || !manifest
			|| IsZeroVector(assignmentPosition) || IsZeroVector(hvtPosition))
			return null;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		HST_OperationRecordState operation = new HST_OperationRecordState();
		operation.m_sOperationId = BuildOperationId(mission.m_sInstanceId);
		operation.m_eType = HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD;
		operation.m_iContractVersion = ResolveExpectedContractVersion(mission.m_sMissionId);
		operation.m_iProjectionContractVersion = EXACT_PROJECTION_CONTRACT_VERSION;
		operation.m_sOwnerFactionKey = zone.m_sOwnerFactionKey;
		operation.m_sMissionInstanceId = mission.m_sInstanceId;
		operation.m_sManifestId = manifest.m_sManifestId;
		operation.m_sSpawnResultId = BuildSpawnResultId(mission.m_sInstanceId);
		operation.m_sForceId = BuildForceId(mission.m_sInstanceId);
		operation.m_sProjectionId = BuildProjectionId(mission.m_sInstanceId);
		operation.m_sGroupId = BuildProjectionId(mission.m_sInstanceId);
		operation.m_sOriginZoneId = zone.m_sZoneId;
		operation.m_vOriginPosition = assignmentPosition;
		operation.m_sAssignmentKind = ASSIGNMENT_KIND;
		operation.m_sAssignmentZoneId = zone.m_sZoneId;
		operation.m_vAssignmentPosition = assignmentPosition;
		operation.m_sTacticalTargetZoneId = zone.m_sZoneId;
		operation.m_vTacticalTargetPosition = hvtPosition;
		operation.m_vStrategicPosition = assignmentPosition;
		operation.m_sCurrentRouteId = "";
		operation.m_sRouteContractHash = "";
		operation.m_iRouteVersion = 0;
		operation.m_iRouteWaypointIndex = -1;
		operation.m_vRouteStartPosition = "0 0 0";
		operation.m_vRouteEndPosition = "0 0 0";
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
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ZoneState zone,
		HST_MissionAssetState hvt,
		HST_ForceCompositionResult composition,
		HST_GroupSpawnPlan groupPlan,
		HST_ForceGroupCatalogEntry catalogGroup,
		HST_ForceManifestState manifest,
		vector assignmentPosition)
	{
		if (!state || !mission || !zone || !hvt || !composition || !groupPlan
			|| !catalogGroup || !manifest || IsZeroVector(assignmentPosition))
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
		group.m_sCompositionIntentId = ResolveExpectedIntentId(mission.m_sMissionId);
		group.m_sCompositionTier = groupPlan.m_sTier;
		group.m_sCompositionSummary = composition.m_sDebugSummary;
		group.m_sSpawnFallbackMode = EXACT_GROUP_MODE;
		group.m_vPosition = assignmentPosition;
		group.m_vSourcePosition = assignmentPosition;
		group.m_vTargetPosition = assignmentPosition;
		group.m_sRouteId = "";
		group.m_sRuntimeStatus = "mission_guard_virtual";
		group.m_iInfantryCount = living;
		group.m_iVehicleCount = 0;
		group.m_iOriginalInfantryCount = living;
		group.m_iOriginalVehicleCount = 0;
		group.m_iCompositionCost = 0;
		group.m_iCompositionManpower = living;
		group.m_iCompositionVehicleCount = 0;
		group.m_iCompositionArmedVehicleCount = 0;
		group.m_iLastSeenAliveCount = living;
		group.m_iSurvivorInfantryCount = living;
		group.m_iSurvivorVehicleCount = 0;
		group.m_iDurableLivingInfantryCount = living;
		group.m_bQRF = false;
		return group;
	}

	protected string ValidateDetachedPlan(
		HST_ActiveMissionState mission,
		HST_ZoneState zone,
		HST_MissionAssetState hvt,
		HST_MissionGuardAdmissionPlan plan)
	{
		if (!mission || !zone || !hvt || !plan || !plan.m_Manifest || !plan.m_Operation || !plan.m_Group)
			return "exact mission guard detached plan is incomplete";
		HST_ForceManifestState manifest = plan.m_Manifest;
		HST_OperationRecordState operation = plan.m_Operation;
		HST_ActiveGroupState group = plan.m_Group;
		int expectedContract = ResolveExpectedContractVersion(mission.m_sMissionId);
		string expectedPolicy = ResolveExpectedPolicyId(mission.m_sMissionId);
		string expectedIntent = ResolveExpectedIntentId(mission.m_sMissionId);
		if (expectedContract <= 0 || expectedPolicy.IsEmpty() || expectedIntent.IsEmpty()
			|| operation.m_iContractVersion != expectedContract
			|| manifest.m_sPolicyId != expectedPolicy)
			return "exact mission guard detached mission contract conflicts";
		if (manifest.m_sIntentId != expectedIntent || group.m_sCompositionIntentId != expectedIntent)
			return "exact mission guard detached mission contract conflicts";
		if (operation.m_sOperationId != BuildOperationId(mission.m_sInstanceId)
			|| manifest.m_sManifestId != BuildManifestId(mission.m_sInstanceId)
			|| operation.m_sManifestId != manifest.m_sManifestId
			|| operation.m_sSpawnResultId != BuildSpawnResultId(mission.m_sInstanceId)
			|| operation.m_sForceId != BuildForceId(mission.m_sInstanceId)
			|| operation.m_sProjectionId != BuildProjectionId(mission.m_sInstanceId)
			|| operation.m_sGroupId != BuildProjectionId(mission.m_sInstanceId))
			return "exact mission guard detached deterministic identity conflicts";
		if (manifest.m_sOperationId != operation.m_sOperationId
			|| group.m_sOperationId != operation.m_sOperationId
			|| group.m_sManifestId != manifest.m_sManifestId
			|| group.m_sSpawnResultId != operation.m_sSpawnResultId
			|| group.m_sForceId != operation.m_sForceId
			|| group.m_sProjectionId != operation.m_sProjectionId
			|| group.m_sGroupId != operation.m_sGroupId)
			return "exact mission guard detached backlinks conflict";
		if (operation.m_sMissionInstanceId != mission.m_sInstanceId
			|| group.m_sMissionInstanceId != mission.m_sInstanceId
			|| !group.m_sMissionAssetId.IsEmpty()
			|| operation.m_sAssignmentZoneId != zone.m_sZoneId
			|| group.m_sZoneId != zone.m_sZoneId
			|| operation.m_sOwnerFactionKey != zone.m_sOwnerFactionKey
			|| group.m_sFactionKey != zone.m_sOwnerFactionKey)
			return "exact mission guard detached mission assignment conflicts";
		if (!operation.m_sCurrentRouteId.IsEmpty() || !operation.m_sRouteContractHash.IsEmpty()
			|| operation.m_iRouteVersion != 0 || operation.m_iRouteWaypointIndex != -1
			|| !group.m_sRouteId.IsEmpty())
			return "exact mission guard detached plan unexpectedly owns a route";
		if (!manifest.m_bFrozen || manifest.m_aGroups.Count() != 1
			|| manifest.m_aMembers.Count() <= 0 || manifest.m_aVehicles.Count() != 0
			|| manifest.m_aAssets.Count() != 0 || manifest.m_iMoneyCost != 0
			|| manifest.m_iHRCost != 0 || manifest.m_iEquipmentCost != 0
			|| manifest.m_iAttackResourceCost != 0 || manifest.m_iSupportResourceCost != 0
			|| manifest.m_sManifestHash != m_Integrity.BuildManifestHash(manifest))
			return "exact mission guard detached frozen manifest conflicts";
		return "";
	}

	protected HST_ForceSpawnQueueRequest BuildSpawnRequest(
		HST_CampaignState state,
		HST_ActiveMissionState mission)
	{
		HST_ForceSpawnQueueRequest request = new HST_ForceSpawnQueueRequest();
		if (!state || !mission)
			return request;
		request.m_sResultId = BuildSpawnResultId(mission.m_sInstanceId);
		request.m_sRequestId = "mission_guard_" + mission.m_sInstanceId;
		request.m_sForceId = BuildForceId(mission.m_sInstanceId);
		request.m_sProjectionId = BuildProjectionId(mission.m_sInstanceId);
		request.m_iPriority = EXACT_PRIORITY;
		request.m_iMaxRetries = EXACT_MAX_RETRIES;
		request.m_iDeadlineSecond = Math.Max(0, state.m_iElapsedSeconds) + DEPLOYMENT_GRACE_SECONDS;
		return request;
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
		string projectionId = BuildProjectionId(mission.m_sInstanceId);
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (operation && (operation.m_sOperationId == operationId
				|| operation.m_sMissionInstanceId == mission.m_sInstanceId
				|| operation.m_sManifestId == manifestId || operation.m_sSpawnResultId == resultId
				|| operation.m_sProjectionId == projectionId || operation.m_sGroupId == projectionId))
				return true;
		}
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
		{
			if (manifest && (manifest.m_sManifestId == manifestId || manifest.m_sOperationId == operationId))
				return true;
		}
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && (batch.m_sResultId == resultId || batch.m_sOperationId == operationId
				|| batch.m_sManifestId == manifestId || batch.m_sProjectionId == projectionId))
				return true;
		}
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && (group.m_sGroupId == projectionId || group.m_sOperationId == operationId
				|| group.m_sMissionInstanceId == mission.m_sInstanceId || group.m_sManifestId == manifestId
				|| group.m_sSpawnResultId == resultId || group.m_sProjectionId == projectionId))
				return true;
		}
		return false;
	}

	protected string FindAdmissionIdentityCollision(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionGuardAdmissionPlan plan)
	{
		if (!state || !mission || !plan || !plan.m_Operation || !plan.m_Manifest || !plan.m_Group)
			return "exact mission guard collision context is missing";
		string operationId = plan.m_Operation.m_sOperationId;
		string manifestId = plan.m_Manifest.m_sManifestId;
		string resultId = plan.m_Operation.m_sSpawnResultId;
		string forceId = plan.m_Operation.m_sForceId;
		string projectionId = plan.m_Operation.m_sProjectionId;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (operation && (operation.m_sOperationId == operationId
				|| operation.m_sMissionInstanceId == mission.m_sInstanceId
				|| operation.m_sManifestId == manifestId || operation.m_sSpawnResultId == resultId
				|| operation.m_sForceId == forceId || operation.m_sProjectionId == projectionId
				|| operation.m_sGroupId == projectionId))
				return "exact mission guard identity is already claimed by an operation";
		}
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
		{
			if (manifest && (manifest.m_sManifestId == manifestId || manifest.m_sOperationId == operationId))
				return "exact mission guard identity is already claimed by a manifest";
		}
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && (batch.m_sResultId == resultId || batch.m_sOperationId == operationId
				|| batch.m_sManifestId == manifestId || batch.m_sForceId == forceId
				|| batch.m_sProjectionId == projectionId))
				return "exact mission guard identity is already claimed by a spawn batch";
		}
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && (group.m_sGroupId == projectionId || group.m_sOperationId == operationId
				|| group.m_sMissionInstanceId == mission.m_sInstanceId || group.m_sManifestId == manifestId
				|| group.m_sSpawnResultId == resultId || group.m_sForceId == forceId
				|| group.m_sProjectionId == projectionId))
				return "exact mission guard identity is already claimed by an active group";
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
		bool requireVirtualHold)
	{
		if (!state || !mission || !operation || !manifest || !batch || !group)
			return "exact mission guard reciprocal graph is incomplete";
		int expectedContract = ResolveExpectedContractVersion(mission.m_sMissionId);
		string expectedPolicy = ResolveExpectedPolicyId(mission.m_sMissionId);
		string expectedIntent = ResolveExpectedIntentId(mission.m_sMissionId);
		if (expectedContract <= 0 || expectedPolicy.IsEmpty() || expectedIntent.IsEmpty())
			return "exact mission guard mission contract is unsupported";
		if (CountMissionIdentity(state, mission) != 1 || CountOperationIdentity(state, operation) != 1
			|| CountManifestIdentity(state, manifest) != 1 || CountBatchIdentity(state, batch) != 1
			|| CountGroupIdentity(state, group) != 1)
			return "exact mission guard reciprocal graph is ambiguous";
		if (!IsExactMission(mission) || mission.m_sInstanceId.IsEmpty()
			|| mission.m_sOperationId != operation.m_sOperationId
			|| mission.m_sManifestId != manifest.m_sManifestId
			|| mission.m_sSpawnResultId != batch.m_sResultId)
			return "exact mission guard mission backlinks conflict";
		if (!mission.m_sSettlementId.IsEmpty())
			return "open exact mission guard unexpectedly has a settlement receipt";

		if (operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
			|| operation.m_iContractVersion != expectedContract
			|| operation.m_iProjectionContractVersion != EXACT_PROJECTION_CONTRACT_VERSION
			|| operation.m_sMissionInstanceId != mission.m_sInstanceId)
			return "exact mission guard operation identity conflicts";
		if (operation.m_sOperationId != BuildOperationId(mission.m_sInstanceId)
			|| operation.m_sManifestId != BuildManifestId(mission.m_sInstanceId)
			|| operation.m_sSpawnResultId != BuildSpawnResultId(mission.m_sInstanceId))
			return "exact mission guard operation identity conflicts";
		if (operation.m_sForceId != BuildForceId(mission.m_sInstanceId)
			|| operation.m_sProjectionId != BuildProjectionId(mission.m_sInstanceId)
			|| operation.m_sGroupId != BuildProjectionId(mission.m_sInstanceId))
			return "exact mission guard operation identity conflicts";
		if (operation.m_sAssignmentKind != ASSIGNMENT_KIND
			|| operation.m_sRecallPolicyId != RECALL_POLICY_ID
			|| operation.m_sSettlementPolicyId != SETTLEMENT_POLICY_ID
			|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
			return "exact mission guard open lifecycle conflicts";
		if (operation.m_eResumeDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| !operation.m_sSettlementId.IsEmpty())
			return "exact mission guard open lifecycle conflicts";
		if (!operation.m_sCurrentRouteId.IsEmpty() || !operation.m_sRouteContractHash.IsEmpty()
			|| operation.m_iRouteVersion != 0 || operation.m_iRouteWaypointIndex != -1)
			return "exact mission guard operation unexpectedly owns route authority";
		if (operation.m_iRouteLapCount != 0 || operation.m_iRouteLegSequence != 0
			|| operation.m_fRouteTotalDistanceMeters != 0.0 || operation.m_fRouteProgressMeters != 0.0)
			return "exact mission guard operation unexpectedly owns route authority";
		if (operation.m_fStrategicSpeedMetersPerSecond != 0.0)
			return "exact mission guard operation unexpectedly owns route authority";

		HST_ZoneState zone = state.FindZone(operation.m_sAssignmentZoneId);
		if (!zone || operation.m_sOriginZoneId != zone.m_sZoneId
			|| operation.m_sTacticalTargetZoneId != zone.m_sZoneId
			|| operation.m_sOwnerFactionKey != manifest.m_sFactionKey
			|| !PositionsMatch(operation.m_vOriginPosition, operation.m_vAssignmentPosition))
			return "exact mission guard immutable assignment conflicts";
		if (IsZeroVector(operation.m_vAssignmentPosition)
			|| IsZeroVector(operation.m_vTacticalTargetPosition)
			|| Distance2D(operation.m_vAssignmentPosition, operation.m_vTacticalTargetPosition) < 6.0
			|| Distance2D(operation.m_vAssignmentPosition, operation.m_vTacticalTargetPosition) > 30.0)
			return "exact mission guard immutable assignment position is missing";

		if (!manifest.m_bFrozen || manifest.m_sManifestId != operation.m_sManifestId
			|| manifest.m_sOperationId != operation.m_sOperationId
			|| manifest.m_sForceKind != EXACT_FORCE_KIND || manifest.m_sPolicyId != expectedPolicy)
			return "exact mission guard frozen manifest conflicts";
		if (manifest.m_sIntentId != expectedIntent || manifest.m_sFactionRole != "enemy"
			|| manifest.m_sFactionKey != operation.m_sOwnerFactionKey
			|| !manifest.m_sSourceZoneId.IsEmpty())
			return "exact mission guard frozen manifest conflicts";
		if (manifest.m_sTargetZoneId != operation.m_sAssignmentZoneId
			|| manifest.m_sCatalogVersion != HST_ForceCatalogService.CATALOG_VERSION)
			return "exact mission guard frozen manifest conflicts";
		if (manifest.m_iMoneyCost != 0 || manifest.m_iHRCost != 0
			|| manifest.m_iEquipmentCost != 0 || manifest.m_iAttackResourceCost != 0)
			return "exact mission guard frozen manifest conflicts";
		if (manifest.m_iSupportResourceCost != 0 || manifest.m_iRequestedVehicleCount != 0
			|| manifest.m_iAcceptedVehicleCount != 0)
			return "exact mission guard frozen manifest conflicts";
		if (manifest.m_aGroups.Count() != 1 || manifest.m_aMembers.Count() <= 0
			|| manifest.m_aVehicles.Count() != 0 || manifest.m_aAssets.Count() != 0)
			return "exact mission guard frozen manifest conflicts";
		if (manifest.m_sManifestHash.IsEmpty()
			|| manifest.m_sManifestHash != m_Integrity.BuildManifestHash(manifest))
			return "exact mission guard frozen manifest conflicts";
		string catalogFailure = ValidateExactCatalogRoster(manifest, mission.m_sInstanceId);
		if (!catalogFailure.IsEmpty())
			return catalogFailure;

		if (batch.m_sResultId != operation.m_sSpawnResultId
			|| batch.m_sRequestId != "mission_guard_" + mission.m_sInstanceId
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| batch.m_sManifestHash != manifest.m_sManifestHash
			|| batch.m_sOperationId != operation.m_sOperationId
			|| batch.m_sForceId != operation.m_sForceId
			|| batch.m_sProjectionId != operation.m_sProjectionId
			|| batch.m_iPriority != EXACT_PRIORITY || batch.m_iMaxRetries != EXACT_MAX_RETRIES)
			return "exact mission guard spawn batch backlinks conflict";
		string rosterFailure = ValidateBatchRosterBijection(manifest, batch);
		if (!rosterFailure.IsEmpty())
			return rosterFailure;

		if (group.m_sGroupId != operation.m_sGroupId || group.m_sGroupId != group.m_sProjectionId
			|| group.m_sOperationId != operation.m_sOperationId
			|| group.m_sManifestId != manifest.m_sManifestId)
			return "exact mission guard active-group backlinks conflict";
		if (group.m_sSpawnResultId != batch.m_sResultId || group.m_sForceId != batch.m_sForceId
			|| group.m_sProjectionId != batch.m_sProjectionId
			|| group.m_sMissionInstanceId != mission.m_sInstanceId)
			return "exact mission guard active-group backlinks conflict";
		if (!group.m_sMissionAssetId.IsEmpty() || group.m_sZoneId != operation.m_sAssignmentZoneId
			|| group.m_sFactionKey != operation.m_sOwnerFactionKey
			|| group.m_sCompositionRequestId != manifest.m_sManifestId)
			return "exact mission guard active-group backlinks conflict";
		if (group.m_sCompositionIntentId != expectedIntent
			|| group.m_sSpawnFallbackMode != EXACT_GROUP_MODE
			|| group.m_sPrefab != manifest.m_sGroupPrefab || !group.m_sRouteId.IsEmpty())
			return "exact mission guard active-group backlinks conflict";
		if (!group.m_sSupportRequestId.IsEmpty() || !group.m_sEnemyOrderId.IsEmpty()
			|| !group.m_sConvoyElementId.IsEmpty() || !group.m_sGarrisonZoneId.IsEmpty())
			return "exact mission guard active-group backlinks conflict";
		if (!group.m_sQRFInstanceId.IsEmpty() || group.m_bQRF
			|| group.m_iOriginalInfantryCount != manifest.m_iAcceptedMemberCount
			|| group.m_iOriginalVehicleCount != 0)
			return "exact mission guard active-group backlinks conflict";
		if (group.m_iVehicleCount != 0 || group.m_iCompositionCost != 0
			|| group.m_iCompositionManpower != manifest.m_iAcceptedMemberCount
			|| group.m_iCompositionVehicleCount != 0)
			return "exact mission guard active-group backlinks conflict";
		if (group.m_iCompositionArmedVehicleCount != 0)
			return "exact mission guard active-group backlinks conflict";

		bool strategicPair = operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING);
		bool livePair = operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING);
		if (!strategicPair && !livePair)
			return "exact mission guard materialization and position authority conflict";
		if (requireVirtualHold && (operation.m_eMaterializationState
			!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| !batch.m_bStrategicProjectionHeld
			|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING))
			return "new exact mission guard projection is not an idle strategic hold";
		bool terminalBatch = batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED;
		if (!terminalBatch)
		{
			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				&& (!batch.m_bStrategicProjectionHeld
					|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING))
				return "virtual exact mission guard batch is not strategically held";
			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
				&& batch.m_bStrategicProjectionHeld)
				return "materializing exact mission guard batch remains strategically held";
			if ((operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
				&& (batch.m_bStrategicProjectionHeld
					|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED))
				return "physical exact mission guard batch authority conflicts";
		}
		return "";
	}

	protected string ValidateExactCatalogRoster(HST_ForceManifestState manifest, string missionInstanceId)
	{
		if (!manifest || manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
			return "exact mission guard catalog root is missing";
		HST_ForceManifestGroupState root = manifest.m_aGroups[0];
		HST_ForceGroupCatalogEntry catalogGroup;
		int matches;
		foreach (HST_ForceGroupCatalogEntry candidate : m_Catalog.BuildGroupCatalog(manifest.m_sFactionKey))
		{
			if (!candidate || candidate.m_sEntryId != root.m_sCatalogEntryId)
				continue;
			catalogGroup = candidate;
			matches++;
		}
		if (matches != 1 || !catalogGroup)
			return "exact mission guard catalog execution root conflicts";
		int catalogMemberCount = catalogGroup.m_aMemberSlots.Count();
		bool rootIdentityMatches = root.m_sElementId == BuildGroupRootId(missionInstanceId)
			&& root.m_iOrdinal == 0 && root.m_bRequired;
		bool rootCatalogMatches = root.m_sPrefab == catalogGroup.m_sExecutionPrefab
			&& root.m_sRole == catalogGroup.m_sRole;
		bool manifestCatalogMatches = manifest.m_sGroupPrefab == catalogGroup.m_sExecutionPrefab
			&& root.m_iExpectedMemberCount == catalogMemberCount;
		bool manifestRosterCountsMatch = manifest.m_iRequestedMemberCount == catalogMemberCount
			&& manifest.m_iAcceptedMemberCount == catalogMemberCount
			&& manifest.m_aMembers.Count() == catalogMemberCount;
		if (!rootIdentityMatches || !rootCatalogMatches)
			return "exact mission guard catalog execution root conflicts";
		if (!manifestCatalogMatches || !manifestRosterCountsMatch)
			return "exact mission guard catalog execution root conflicts";
		for (int memberIndex = 0; memberIndex < catalogGroup.m_aMemberSlots.Count(); memberIndex++)
		{
			HST_ForceGroupCatalogSlot catalogSlot = catalogGroup.m_aMemberSlots[memberIndex];
			HST_ForceManifestMemberState member = manifest.m_aMembers[memberIndex];
			if (!catalogSlot || !member)
				return "exact mission guard ordered catalog member roster conflicts";
			bool memberIdentityMatches = member.m_sSlotId == BuildMemberSlotId(missionInstanceId, memberIndex)
				&& member.m_sCatalogSlotId == catalogGroup.m_sEntryId + "/" + catalogSlot.m_sSlotId
				&& member.m_sGroupElementId == root.m_sElementId;
			bool memberCatalogMatches = member.m_sPrefab == catalogSlot.m_sPrefab
				&& member.m_sRole == catalogSlot.m_sRole
				&& member.m_bRequired == catalogSlot.m_bRequired;
			bool memberPlacementMatches = member.m_sAssignedVehicleSlotId.IsEmpty()
				&& member.m_sSeatRole.IsEmpty() && member.m_iSeatIndex == -1;
			bool memberOrdinalMatches = member.m_iOrdinal == memberIndex;
			bool memberCostsMatch = member.m_iMoneyCost == 0
				&& member.m_iHRCost == 0 && member.m_iEquipmentCost == 0;
			if (!memberIdentityMatches || !memberCatalogMatches)
				return "exact mission guard ordered catalog member roster conflicts";
			if (!memberPlacementMatches || !memberOrdinalMatches || !memberCostsMatch)
				return "exact mission guard ordered catalog member roster conflicts";
		}
		return "";
	}

	protected string ValidateBatchRosterBijection(
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		if (!manifest || !batch || manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
			return "exact mission guard durable roster is incomplete";
		int expectedSlots = manifest.m_aMembers.Count() + 1;
		if (batch.m_iExpectedSlotCount != expectedSlots || batch.m_aSlotResults.Count() != expectedSlots)
			return "exact mission guard durable roster slot count conflicts";
		string rootId = manifest.m_aGroups[0].m_sElementId;
		if (CountBatchSlots(batch, rootId, HST_ForceSpawnQueueService.SLOT_KIND_GROUP) != 1)
			return "exact mission guard group-root slot is not unique";
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member || CountManifestMembers(manifest, member.m_sSlotId) != 1
				|| CountBatchSlots(batch, member.m_sSlotId, HST_ForceSpawnQueueService.SLOT_KIND_MEMBER) != 1)
				return "exact mission guard member-slot bijection conflicts";
		}
		bool terminalBatch = batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_sSlotId.IsEmpty() || slot.m_sProjectionId != batch.m_sProjectionId)
				return "exact mission guard roster contains an invalid slot identity";
			bool rootSlot = slot.m_sSlotId == rootId
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP;
			bool memberSlot = manifest.FindMemberSlot(slot.m_sSlotId) != null
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER;
			if (!rootSlot && !memberSlot)
				return "exact mission guard roster contains a foreign slot";
			if (slot.m_bCasualtyConfirmed)
			{
				if (!memberSlot || slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
					|| !slot.m_bEverAlive || slot.m_iCasualtyAtSecond < batch.m_iCreatedAtSecond)
					return "exact mission guard casualty tombstone conflicts";
				continue;
			}
			if (!terminalBatch && (slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				|| slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_FINAL
				|| slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CANCELLED))
				return "open exact mission guard roster contains an unproven terminal slot";
			if (memberSlot && slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
				&& !slot.m_bEverAlive)
				return "registered exact mission guard member lacks living evidence";
		}
		return "";
	}

	bool TickBeforePhysical(HST_CampaignState state, HST_CampaignPreset preset)
	{
		return Tick(state, preset);
	}

	bool Tick(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return false;
		bool changed;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD)
				continue;
			if (IsQuarantinedOperationContractVersion(operation.m_iContractVersion))
			{
				changed = ApplyQuarantineStatus(state, operation, operation.m_sLastProjectionReason) || changed;
				continue;
			}
			if (!IsSupportedExactContractVersion(operation.m_iContractVersion))
			{
				changed = QuarantineOperationAuthority(state, operation,
					"unsupported exact mission guard contract version") || changed;
				continue;
			}
			changed = TickOperation(state, preset, operation) || changed;
		}
		return changed;
	}

	protected bool NormalizeRestoredQuarantinedAuthority(
		HST_CampaignState state,
		HST_OperationRecordState operation)
	{
		if (!state || !operation || !IsQuarantinedOperationContractVersion(operation.m_iContractVersion)
			|| !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar)
			return false;
		bool changed;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (!BatchClaimsOperationAuthority(batch, operation)
				|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
				continue;
			HST_ActiveGroupState group;
			if (!ResolveQuarantinedSuccessfulProjectionContext(state, operation, batch, group))
				continue;
			bool runtimeExists = m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0
				|| m_SpawnAdapter.CountHandlesForResultId(batch.m_sResultId) > 0
				|| m_PhysicalWar.GetForceSpawnGroupRoot(group) != null
				|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0;
			if (runtimeExists)
				continue;
			ClearGroupProcessAuthority(group);
			group.m_iLifecycleRevision++;
			HST_ForceSpawnQueueCallbackResult terminal = m_SpawnQueue.CompleteQuarantinedSuccessfulProjectionCancellation(
				state.m_aForceSpawnResults,
				batch.m_sResultId,
				batch.m_sProjectionId,
				Math.Max(0, state.m_iElapsedSeconds),
				"restored quarantined exact mission guard had no process runtime authority");
			if (terminal && terminal.m_bAccepted)
				changed = true;
		}
		return changed;
	}

	bool ReconcileAfterMissionOutcomes(HST_CampaignState state)
	{
		if (!state)
			return false;
		bool changed;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD)
				continue;
			if (IsQuarantinedOperationContractVersion(operation.m_iContractVersion))
			{
				changed = ApplyQuarantineStatus(state, operation, operation.m_sLastProjectionReason) || changed;
				continue;
			}
			if (!IsSupportedExactContractVersion(operation.m_iContractVersion))
				continue;
			HST_ActiveMissionState mission = state.FindActiveMission(operation.m_sMissionInstanceId);
			if (!mission)
			{
				changed = QuarantineOperationAuthority(state, operation,
					"exact mission guard lost its mission outcome authority") || changed;
				continue;
			}
			if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			{
				if (mission.m_sSettlementId.IsEmpty())
				{
					mission.m_sSettlementId = operation.m_sSettlementId;
					changed = true;
				}
				continue;
			}
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			string failure = ResolveRuntimeContext(state, operation, mission, manifest, batch, group);
			if (!failure.IsEmpty())
			{
				changed = QuarantineOperationAuthority(state, operation, failure) || changed;
				continue;
			}
			HST_EOperationTerminalResult terminal = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED;
			string detail = "mission_failed";
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED)
			{
				terminal = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED;
				detail = "mission_completed";
			}
			else if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED)
				detail = "mission_expired";
			changed = RetireAndSettle(state, operation, mission, manifest, batch, group,
				terminal, detail, "mission outcome ended exact guard authority") || changed;
		}
		return changed;
	}

	bool ReconcileAfterRestore(HST_CampaignState state)
	{
		if (!state || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar)
			return false;
		bool changed;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD)
				continue;
			if (IsQuarantinedOperationContractVersion(operation.m_iContractVersion))
			{
				changed = NormalizeRestoredQuarantinedAuthority(state, operation) || changed;
				changed = ApplyQuarantineStatus(state, operation, operation.m_sLastProjectionReason) || changed;
				continue;
			}
			if (!IsSupportedExactContractVersion(operation.m_iContractVersion))
			{
				changed = QuarantineOperationAuthority(state, operation,
					"restore found an unsupported exact mission guard contract version") || changed;
				continue;
			}
			if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			{
				changed = RetireSettledRuntimeIfPresent(state, operation) || changed;
				changed = FinalizeSettledRuntime(state, operation) || changed;
				continue;
			}
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
			{
				HST_EOperationTerminalResult outcome = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED;
				string detail = "restore_mission_terminal";
				if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED)
				{
					outcome = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED;
					detail = "restore_mission_completed";
				}
				changed = RetireAndSettle(state, operation, mission, manifest, batch, group,
					outcome, detail, "restored mission outcome ended exact guard authority") || changed;
				continue;
			}
			HST_ZoneState zone = state.FindZone(operation.m_sAssignmentZoneId);
			if (!zone || zone.m_sOwnerFactionKey != operation.m_sOwnerFactionKey)
			{
				MarkMissionRuntimeFailed(state, mission, "mission target ownership changed during restore");
				changed = RetireAndSettle(state, operation, mission, manifest, batch, group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
					"owner_changed", "mission target ownership changed during restore") || changed;
				continue;
			}
			if (IsTerminalSpawnBatch(batch))
			{
				MarkMissionRuntimeFailed(state, mission, "restored exact mission guard has terminal spawn failure");
				changed = RetireAndSettle(state, operation, mission, manifest, batch, group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
					"restore_spawn_failed", "restored exact mission guard has terminal spawn failure") || changed;
				continue;
			}
			changed = NormalizeRestoredOpenRuntime(state, operation, mission, manifest, batch, group) || changed;
		}
		return changed;
	}

	bool PrepareOpenPhysicalAuthorityForSettlement(HST_CampaignState state, out string failure)
	{
		failure = "";
		if (!state || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar)
		{
			failure = "exact mission guard settlement reconciliation services are unavailable";
			return false;
		}
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
				|| !IsSupportedExactContractVersion(operation.m_iContractVersion)
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				continue;
			bool live = operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
			if (!live)
				continue;
			HST_ActiveMissionState mission;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			failure = ResolveRuntimeContext(state, operation, mission, manifest, batch, group);
			if (!failure.IsEmpty())
			{
				QuarantineOperationAuthority(state, operation, failure);
				failure = "exact mission guard settlement graph conflicts: " + failure;
				return false;
			}
			HST_ForceSpawnAdapterTickResult reconciled = m_SpawnAdapter.ReconcileExactInfantryProjectionAuthority(
				state, m_SpawnQueue, m_PhysicalWar, Math.Max(0, state.m_iElapsedSeconds), operation.m_sProjectionId);
			if (!reconciled || reconciled.m_iFailedCount > 0)
			{
				failure = "exact mission guard settlement casualty reconciliation failed";
				return false;
			}
			int living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
			SyncGroupRoster(group, living);
			operation.m_iLastVirtualFriendlyCount = living;
			if (living <= 0)
				continue;
			string bindingFailure;
			if (!m_SpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
				state, batch, m_SpawnQueue, m_PhysicalWar, bindingFailure))
			{
				failure = "exact mission guard settlement live bindings are incomplete: " + bindingFailure;
				return false;
			}
		}
		return true;
	}

	bool PrepareOpenPhysicalAuthorityForPersistence(HST_CampaignState state, out string failure)
	{
		return PrepareOpenPhysicalAuthorityForSettlement(state, failure);
	}

	bool PrepareQuarantinedAuthorityForPersistence(HST_CampaignState state, out string failure)
	{
		failure = "";
		if (!state || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar)
		{
			failure = "exact mission guard quarantine cleanup services are unavailable";
			return false;
		}
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
				|| !IsQuarantinedOperationContractVersion(operation.m_iContractVersion))
				continue;
			bool operationNormalizationChanged;
			ApplyQuarantineStatus(state, operation, operation.m_sLastProjectionReason);
			foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
			{
				if (!BatchClaimsOperationAuthority(batch, operation))
					continue;
				if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
				{
					bool cleanupChanged;
					if (!TryRetireQuarantinedSuccessfulProjection(state, operation, batch,
						operation.m_sLastProjectionReason, cleanupChanged))
					{
						failure = "strong exact mission guard quarantine runtime could not be retired";
						return false;
					}
				}
				if (!IsTerminalSpawnBatch(batch) || BatchHasProcessResidue(batch))
				{
					failure = "strong exact mission guard quarantine batch is not terminal and process-clean";
					return false;
				}
			}
			foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
			{
				if (!GroupClaimsOperationAuthority(group, operation))
					continue;
				if (m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0
					|| m_PhysicalWar.GetForceSpawnGroupRoot(group)
					|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0)
				{
					failure = "strong exact mission guard quarantine still owns physical runtime";
					return false;
				}
				ClearGroupProcessAuthority(group);
				if (!IsZeroVector(group.m_vPosition)
					&& !PositionsMatch(operation.m_vStrategicPosition, group.m_vPosition))
				{
					operation.m_vStrategicPosition = group.m_vPosition;
					operationNormalizationChanged = true;
				}
				if (GroupHasProcessResidue(group))
				{
					failure = "strong exact mission guard quarantine group is not process-clean";
					return false;
				}
			}
			if (m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0
				|| (!operation.m_sSpawnResultId.IsEmpty()
					&& m_SpawnAdapter.CountHandlesForResultId(operation.m_sSpawnResultId) > 0))
			{
				failure = "strong exact mission guard quarantine still owns adapter handles";
				return false;
			}
			if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
				|| operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
				operationNormalizationChanged = true;
			if (operationNormalizationChanged)
			{
				int normalizedSecond = Math.Max(0, state.m_iElapsedSeconds);
				operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
				operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
				operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
				operation.m_iMaterializationStateEnteredAtSecond = normalizedSecond;
				operation.m_iEngagementStateEnteredAtSecond = normalizedSecond;
				operation.m_iStrategicLastUpdateSecond = normalizedSecond;
				operation.m_iLastProgressAtSecond = normalizedSecond;
				operation.m_iRevision++;
			}
		}
		return true;
	}

	bool SettleOpenOperationsForCampaignStop(HST_CampaignState state, string reason)
	{
		if (!state)
			return false;
		if (reason.IsEmpty())
			reason = "campaign phase no longer permits an active mission guard";
		bool changed;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
				|| !IsSupportedExactContractVersion(operation.m_iContractVersion)
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				continue;
			HST_ActiveMissionState mission;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			string failure = ResolveRuntimeContext(state, operation, mission, manifest, batch, group);
			if (!failure.IsEmpty())
			{
				changed = QuarantineOperationAuthority(state, operation, reason + ": " + failure) || changed;
				continue;
			}
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
			{
				mission.m_eStatus = HST_EMissionStatus.HST_MISSION_EXPIRED;
				mission.m_iRemainingSeconds = 0;
				mission.m_sRuntimePhase = "expired";
				mission.m_sRuntimeFailureReason = reason;
				changed = true;
			}
			changed = RetireAndSettle(state, operation, mission, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED,
				"campaign_stop", reason) || changed;
		}
		return changed;
	}

	bool ReconcileSettledRuntimeCleanup(HST_CampaignState state)
	{
		if (!state)
			return false;
		bool changed;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
				|| !IsSupportedExactContractVersion(operation.m_iContractVersion)
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				continue;
			changed = RetireSettledRuntimeIfPresent(state, operation) || changed;
			changed = FinalizeSettledRuntime(state, operation) || changed;
		}
		return changed;
	}

	protected bool TickOperation(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_OperationRecordState operation)
	{
		if (!state || !preset || !operation || !m_SpawnQueue || !m_SpawnAdapter
			|| !m_PhysicalWar || !m_Materialization || !m_Integrity || !m_Catalog)
			return QuarantineOperationAuthority(state, operation,
				"exact mission guard runtime services are unavailable");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return FinalizeSettledRuntime(state, operation);
		HST_ActiveMissionState mission;
		HST_ForceManifestState manifest;
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		string failure = ResolveRuntimeContext(state, operation, mission, manifest, batch, group);
		if (!failure.IsEmpty())
			return QuarantineOperationAuthority(state, operation, failure);
		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
		{
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
			{
				mission.m_eStatus = HST_EMissionStatus.HST_MISSION_EXPIRED;
				mission.m_iRemainingSeconds = 0;
				mission.m_sRuntimePhase = "expired";
				mission.m_sRuntimeFailureReason = "campaign phase ended exact mission guard authority";
			}
			return RetireAndSettle(state, operation, mission, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED,
				"campaign_stop", "campaign phase ended exact mission guard authority");
		}
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
		{
			HST_EOperationTerminalResult terminal = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED;
			string detail = "mission_failed";
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED)
			{
				terminal = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED;
				detail = "mission_completed";
			}
			else if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED)
				detail = "mission_expired";
			return RetireAndSettle(state, operation, mission, manifest, batch, group,
				terminal, detail, "mission outcome ended exact guard authority");
		}
		HST_ZoneState zone = state.FindZone(operation.m_sAssignmentZoneId);
		if (!zone || zone.m_sOwnerFactionKey != operation.m_sOwnerFactionKey)
		{
			MarkMissionRuntimeFailed(state, mission, "mission target ownership changed");
			return RetireAndSettle(state, operation, mission, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				"owner_changed", "mission target ownership changed");
		}
		if (IsTerminalSpawnBatch(batch))
		{
			MarkMissionRuntimeFailed(state, mission, "exact mission guard spawn authority failed");
			return RetireAndSettle(state, operation, mission, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"spawn_failed", "exact mission guard spawn authority failed");
		}
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL)
			return TickVirtual(state, operation, mission, manifest, batch, group);
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
			return TickMaterializing(state, operation, mission, manifest, batch, group);
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			return TickPhysical(state, operation, mission, manifest, batch, group);
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
			return ContinueDematerialization(state, operation, mission, manifest, batch, group,
				operation.m_sLastProjectionReason);
		return QuarantineOperationAuthority(state, operation,
			"exact mission guard materialization authority is invalid");
	}

	protected bool TickVirtual(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!batch.m_bStrategicProjectionHeld)
			return QuarantineOperationAuthority(state, operation,
				"virtual exact mission guard batch is not strategically held");
		int living = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
		SyncGroupRoster(group, living);
		operation.m_iLastVirtualFriendlyCount = living;
		if (living <= 0)
		{
			HST_ForceSpawnQueueCallbackResult eliminated = m_SpawnQueue.CompleteStrategicProjectionElimination(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				Math.Max(0, state.m_iElapsedSeconds),
				"exact mission guard virtual roster eliminated");
			if (!eliminated || !eliminated.m_bAccepted)
				return QuarantineOperationAuthority(state, operation,
					"exact mission guard virtual elimination could not be recorded");
			return SettleOperation(state, operation, mission, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
				"destroyed", "all exact mission guards were eliminated");
		}

		HST_OperationProjectionDecision decision = m_Materialization.EvaluateExactPlayerQRF(
			operation,
			operation.m_vStrategicPosition);
		bool changed = RecordProjectionDecision(state, operation, decision);
		if (decision.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE)
			return BeginMaterialization(state, operation, mission, manifest, batch, group, decision.m_sReason) || changed;
		ApplyVirtualRuntimeStatus(group);
		return changed;
	}

	protected bool BeginMaterialization(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		HST_ForceSpawnQueueCallbackResult released = m_SpawnQueue.ReleaseStrategicProjectionForMaterialization(
			state.m_aForceSpawnResults,
			manifest,
			batch.m_sResultId,
			batch.m_sProjectionId,
			nowSecond,
			nowSecond + DEPLOYMENT_GRACE_SECONDS);
		if (!released || !released.m_bAccepted)
		{
			MarkMissionRuntimeFailed(state, mission, "exact mission guard materialization release failed");
			return SettleOperation(state, operation, mission, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"spawn_failed", "exact mission guard materialization release failed");
		}
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iLastProjectionDecisionSecond = nowSecond;
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		group.m_vPosition = operation.m_vStrategicPosition;
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		group.m_vTargetPosition = operation.m_vAssignmentPosition;
		group.m_sRuntimeStatus = "mission_guard_materializing";
		group.m_iLifecycleRevision++;
		return true;
	}

	protected bool TickMaterializing(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (IsTerminalSpawnBatch(batch) || group.m_sRuntimeStatus == "spawn_failed")
		{
			MarkMissionRuntimeFailed(state, mission, "exact mission guard materialization failed");
			return RetireAndSettle(state, operation, mission, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"spawn_failed", "exact mission guard materialization failed");
		}
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| !group.m_bSpawnedEntity)
			return false;

		int living;
		string rosterFailure;
		if (!ReconcileProjectionRoster(state, operation, batch, group, living, rosterFailure))
			return QuarantineOperationAuthority(state, operation,
				"exact mission guard materialization reconciliation failed: " + rosterFailure);
		if (living <= 0)
			return RetireAndSettle(state, operation, mission, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
				"destroyed", "exact mission guards were eliminated during materialization");

		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_iRevision++;
		group.m_sRuntimeStatus = "mission_guard_physical";
		group.m_iLifecycleRevision++;
		if (!m_PhysicalWar.RestartExactMissionGuardInfantryAssignment(
			state,
			group,
			operation.m_vAssignmentPosition,
			"Exact mission guards materialized at their persisted assignment."))
		{
			MarkMissionRuntimeFailed(state, mission, "exact mission guard assignment restart failed");
			return RetireAndSettle(state, operation, mission, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"assignment_failed", "exact mission guard assignment restart failed");
		}
		return true;
	}

	protected bool TickPhysical(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			MarkMissionRuntimeFailed(state, mission, "physical exact mission guard lost successful batch authority");
			return RetireAndSettle(state, operation, mission, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				"physical_batch_failed", "physical exact mission guard lost successful batch authority");
		}
		if (group.m_sRuntimeStatus.Contains("runtime_binding_missing"))
			return QuarantineOperationAuthority(state, operation,
				"exact mission guard runtime binding disappeared without casualty proof");

		int living;
		string rosterFailure;
		if (!ReconcileProjectionRoster(state, operation, batch, group, living, rosterFailure))
			return QuarantineOperationAuthority(state, operation,
				"exact mission guard physical reconciliation failed: " + rosterFailure);
		if (living <= 0 || group.m_sRuntimeStatus == "eliminated")
		{
			string eliminationFailure;
			if (!m_PhysicalWar.FinalizeEliminatedForceSpawnProjection(
				state,
				group,
				Math.Max(0, state.m_iElapsedSeconds),
				eliminationFailure))
				return QuarantineOperationAuthority(state, operation,
					"exact mission guard elimination cleanup is unresolved: " + eliminationFailure);
			return SettleOperation(state, operation, mission, manifest, batch, group,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
				"destroyed", "all exact mission guards were eliminated");
		}

		vector livePosition;
		string liveEvidence;
		if (!m_PhysicalWar.TryResolveExactMissionGuardLivePosition(
			state,
			group,
			livePosition,
			liveEvidence))
			return QuarantineOperationAuthority(state, operation,
				"exact mission guard live position is unavailable: " + liveEvidence);
		operation.m_vStrategicPosition = livePosition;
		operation.m_iLastProgressAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iRevision++;
		group.m_vPosition = livePosition;
		bool changed = true;
		changed = UpdatePhysicalEngagement(state, operation, group) || changed;

		HST_OperationProjectionDecision decision = m_Materialization.EvaluateExactPlayerQRF(operation, livePosition);
		changed = RecordProjectionDecision(state, operation, decision) || changed;
		if (decision.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_DEMATERIALIZE)
			return TryDematerialize(state, operation, mission, manifest, batch, group, decision.m_sReason) || changed;
		return changed;
	}

	protected bool ReconcileProjectionRoster(
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
			failure = "projection roster services are unavailable";
			return false;
		}
		HST_ForceSpawnAdapterTickResult reconciled = m_SpawnAdapter.ReconcileExactInfantryProjectionAuthority(
			state,
			m_SpawnQueue,
			m_PhysicalWar,
			Math.Max(0, state.m_iElapsedSeconds),
			operation.m_sProjectionId);
		if (!reconciled || reconciled.m_iFailedCount > 0)
		{
			failure = "adapter reconciliation rejected exact projection";
			if (reconciled && !reconciled.m_sSummary.IsEmpty())
				failure = failure + ": " + reconciled.m_sSummary;
			return false;
		}
		living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
		SyncGroupRoster(group, living);
		operation.m_iLastVirtualFriendlyCount = living;
		if (living <= 0)
			return true;
		if (!group.m_bSpawnedEntity || group.m_sRuntimeStatus == "spawn_failed")
		{
			failure = "durable living roster has no physical group root";
			return false;
		}
		string bindingFailure;
		if (!m_SpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
			state,
			batch,
			m_SpawnQueue,
			m_PhysicalWar,
			bindingFailure))
		{
			failure = "living projection bindings are incomplete: " + bindingFailure;
			return false;
		}
		return true;
	}

	protected bool UpdatePhysicalEngagement(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group)
	{
		if (!state || !operation || !group)
			return false;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		string evidence;
		bool contact = m_PhysicalWar.HasExactMissionGuardLiveContactEvidence(
			state,
			group,
			CONTACT_EVIDENCE_RADIUS_METERS,
			evidence);
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
		if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
			return false;
		if (nowSecond - operation.m_iLastContactAtSecond <= CONTACT_CLEAR_SECONDS)
			return false;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_iRevision++;
		return true;
	}

	protected bool TryDematerialize(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		HST_ForceSpawnQueueCallbackResult preflight = m_SpawnQueue.CanRequeueSuccessfulProjectionForStrategicHold(
			state.m_aForceSpawnResults,
			manifest,
			batch.m_sResultId,
			batch.m_sProjectionId,
			nowSecond,
			nowSecond + DEPLOYMENT_GRACE_SECONDS);
		if (!preflight || !preflight.m_bAccepted)
			return false;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		group.m_sRuntimeStatus = "mission_guard_dematerializing";
		group.m_iLifecycleRevision++;
		return ContinueDematerialization(state, operation, mission, manifest, batch, group, reason);
	}

	protected bool ContinueDematerialization(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		if (!batch.m_bStrategicProjectionHeld)
		{
			int living;
			string rosterFailure;
			if (!ReconcileProjectionRoster(state, operation, batch, group, living, rosterFailure))
				return QuarantineOperationAuthority(state, operation,
					"exact mission guard fold reconciliation failed: " + rosterFailure);
			if (living <= 0)
			{
				string eliminationFailure;
				if (!m_PhysicalWar.FinalizeEliminatedForceSpawnProjection(
					state, group, Math.Max(0, state.m_iElapsedSeconds), eliminationFailure))
					return QuarantineOperationAuthority(state, operation,
						"exact mission guard fold elimination cleanup failed: " + eliminationFailure);
				return SettleOperation(state, operation, mission, manifest, batch, group,
					HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
					"destroyed", "all exact mission guards were eliminated during fold");
			}
			UpdatePhysicalEngagement(state, operation, group);
			if (operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
				return false;

			vector foldedPosition;
			string liveEvidence;
			if (!m_PhysicalWar.TryResolveExactMissionGuardLivePosition(
				state, group, foldedPosition, liveEvidence))
				return QuarantineOperationAuthority(state, operation,
					"exact mission guard fold live position is unavailable: " + liveEvidence);
			HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
				state,
				m_PhysicalWar,
				operation.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return false;
			int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
			HST_ForceSpawnQueueCallbackResult held = m_SpawnQueue.RequeueSuccessfulProjectionForStrategicHold(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				nowSecond,
				nowSecond + DEPLOYMENT_GRACE_SECONDS);
			if (!held || !held.m_bAccepted)
				return QuarantineOperationAuthority(state, operation,
					"exact mission guard survivors could not enter strategic hold");
			operation.m_vStrategicPosition = foldedPosition;
			group.m_vPosition = foldedPosition;
		}

		ClearGroupProcessAuthority(group);
		int virtualLiving = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
		SyncGroupRoster(group, virtualLiving);
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		group.m_vTargetPosition = operation.m_vAssignmentPosition;
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
		ApplyVirtualRuntimeStatus(group);
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

	protected bool RetireAndSettle(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_EOperationTerminalResult terminalResult,
		string outcomeDetail,
		string reason)
	{
		if (!state || !operation || !mission || !manifest || !batch || !group)
			return QuarantineOperationAuthority(state, operation,
				"exact mission guard settlement graph is incomplete: " + reason);
		int handles = m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId);
		bool runtimeExists = handles > 0 || group.m_bSpawnedEntity;
		if (!runtimeExists)
			runtimeExists = m_PhysicalWar.GetForceSpawnGroupRoot(group) != null
				|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0;
		bool liveState = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
		if (liveState && !runtimeExists && m_SpawnQueue.CountDurableLivingMemberSlots(batch) > 0)
			return QuarantineOperationAuthority(state, operation,
				"exact mission guard terminal transition lost live survivors: " + reason);
		if (runtimeExists)
		{
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			{
				int living;
				string rosterFailure;
				if (!ReconcileProjectionRoster(state, operation, batch, group, living, rosterFailure))
					return QuarantineOperationAuthority(state, operation,
						"exact mission guard terminal reconciliation failed: " + rosterFailure);
			}
			HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
				state,
				m_PhysicalWar,
				operation.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return false;
			ClearGroupProcessAuthority(group);
			group.m_iLifecycleRevision++;
		}
		else if (handles > 0)
			return QuarantineOperationAuthority(state, operation,
				"exact mission guard terminal found orphan adapter handles: " + reason);
		return SettleOperation(state, operation, mission, manifest, batch, group,
			terminalResult, outcomeDetail, reason);
	}

	protected bool SettleOperation(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_EOperationTerminalResult terminalResult,
		string outcomeDetail,
		string reason)
	{
		if (!state || !operation || !mission || !manifest || outcomeDetail.IsEmpty())
			return false;
		int expectedContract = ResolveExpectedContractVersion(mission.m_sMissionId);
		string settlementId = HST_OperationService.BuildSettlementId(operation.m_sOperationId, SETTLEMENT_KIND);
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (operation.m_sSettlementId != settlementId || operation.m_eTerminalResult != terminalResult
				|| mission.m_sSettlementId != settlementId)
				return QuarantineOperationAuthority(state, operation,
					"exact mission guard terminal receipt conflicts");
			return FinalizeSettledRuntime(state, operation);
		}
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
			|| expectedContract <= 0 || operation.m_iContractVersion != expectedContract
			|| terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN)
			return QuarantineOperationAuthority(state, operation,
				"exact mission guard settlement authority is invalid");
		int living = ResolveLivingRoster(manifest, batch, group, operation);
		if (!EnsureBatchTerminalForSettlement(state, operation, manifest, batch, group, reason))
			return false;
		if (terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED)
		{
			if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_SUCCEEDED)
				return QuarantineOperationAuthority(state, operation,
					"completed exact mission guard lacks a succeeded mission outcome");
		}
		else if (terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED)
		{
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
			{
				mission.m_eStatus = HST_EMissionStatus.HST_MISSION_EXPIRED;
				mission.m_iRemainingSeconds = 0;
			}
		}
		else if (terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED
			|| terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED)
		{
			mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
			mission.m_iRemainingSeconds = 0;
		}
		else if (terminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
			return QuarantineOperationAuthority(state, operation,
				"active exact HVT mission cannot receive this guard terminal result");

		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iLastVirtualFriendlyCount = living;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		operation.m_eTerminalResult = terminalResult;
		operation.m_sSettlementId = settlementId;
		operation.m_sTerminalReason = reason;
		operation.m_sLastProjectionReason = string.Format("%1: %2", SETTLEMENT_KIND, outcomeDetail);
		operation.m_iDutyStateEnteredAtSecond = nowSecond;
		operation.m_iEngagementStateEnteredAtSecond = nowSecond;
		operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
		operation.m_iLastProgressAtSecond = nowSecond;
		operation.m_iSettledAtSecond = nowSecond;
		operation.m_iRevision++;
		mission.m_sSettlementId = settlementId;
		if (group)
		{
			ClearGroupProcessAuthority(group);
			SyncGroupRoster(group, living);
			group.m_sRuntimeStatus = "mission_guard_terminal_" + outcomeDetail;
			group.m_iLifecycleRevision++;
		}
		FinalizeSettledRuntime(state, operation);
		return true;
	}

	protected bool RetireSettledRuntimeIfPresent(
		HST_CampaignState state,
		HST_OperationRecordState operation)
	{
		if (!state || !operation || !m_SpawnAdapter || !m_PhysicalWar)
			return false;
		HST_ActiveGroupState group = state.FindActiveGroup(operation.m_sGroupId);
		if (!group)
			return false;
		bool runtimeExists = m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0
			|| group.m_bSpawnedEntity || m_PhysicalWar.GetForceSpawnGroupRoot(group) != null
			|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0;
		if (!runtimeExists)
			return false;
		HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
			state,
			m_PhysicalWar,
			operation.m_sProjectionId);
		if (!retired || !retired.m_bSuccess)
			return false;
		ClearGroupProcessAuthority(group);
		group.m_iLifecycleRevision++;
		return true;
	}

	protected bool FinalizeSettledRuntime(
		HST_CampaignState state,
		HST_OperationRecordState operation)
	{
		if (!state || !operation || !m_SpawnAdapter || !m_PhysicalWar
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			|| operation.m_sSettlementId != HST_OperationService.BuildSettlementId(
				operation.m_sOperationId, SETTLEMENT_KIND))
			return false;
		HST_ActiveMissionState mission = state.FindActiveMission(operation.m_sMissionInstanceId);
		if (!mission || mission.m_sSettlementId != operation.m_sSettlementId)
			return false;
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
		HST_ActiveGroupState group = state.FindActiveGroup(operation.m_sGroupId);
		if (m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0)
			return false;
		if (group && (m_PhysicalWar.GetForceSpawnGroupRoot(group)
			|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0))
			return false;
		if (batch && (!BatchClaimsOperationAuthority(batch, operation)
			|| CountBatchIdentity(state, batch) != 1))
			return false;
		if (batch && !IsTerminalSpawnBatch(batch))
			return false;
		if (group && (!GroupClaimsOperationAuthority(group, operation)
			|| CountGroupIdentity(state, group) != 1))
			return false;
		bool changed;
		if (batch)
		{
			int batchIndex = state.m_aForceSpawnResults.Find(batch);
			if (batchIndex >= 0)
			{
				state.m_aForceSpawnResults.Remove(batchIndex);
				changed = true;
			}
		}
		if (group)
		{
			int groupIndex = state.m_aActiveGroups.Find(group);
			if (groupIndex >= 0)
			{
				state.m_aActiveGroups.Remove(groupIndex);
				changed = true;
			}
		}
		return changed;
	}

	protected bool EnsureBatchTerminalForSettlement(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		if (!state || !operation || !manifest || !batch || !group || !m_SpawnQueue
			|| !m_SpawnAdapter || !m_PhysicalWar || !BatchClaimsOperationAuthority(batch, operation))
			return false;
		if (IsTerminalSpawnBatch(batch))
			return true;
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			if (m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0
				|| m_SpawnAdapter.CountHandlesForResultId(batch.m_sResultId) > 0
				|| m_PhysicalWar.GetForceSpawnGroupRoot(group)
				|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0)
				return false;
			HST_ForceSpawnQueueCallbackResult completed = m_SpawnQueue.CompleteQuarantinedSuccessfulProjectionCancellation(
				state.m_aForceSpawnResults,
				batch.m_sResultId,
				batch.m_sProjectionId,
				Math.Max(0, state.m_iElapsedSeconds),
				reason);
			return completed && completed.m_bAccepted && IsTerminalSpawnBatch(batch);
		}
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
			return false;
		HST_ForceSpawnQueueCallbackResult cancelled = m_SpawnQueue.RequestCancel(
			state.m_aForceSpawnResults,
			batch.m_sResultId,
			Math.Max(0, state.m_iElapsedSeconds),
			reason);
		if (!cancelled || !cancelled.m_bAccepted)
			return false;
		return IsTerminalSpawnBatch(batch);
	}

	protected bool NormalizeRestoredOpenRuntime(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!state || !operation || !mission || !manifest || !batch || !group)
			return false;
		bool changed;
		if (operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& !IsZeroVector(group.m_vPosition))
		{
			operation.m_vStrategicPosition = group.m_vPosition;
			changed = true;
		}
		if (!batch.m_bStrategicProjectionHeld
			&& batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			bool runtimeExists = group.m_bSpawnedEntity
				|| m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0
				|| m_PhysicalWar.GetForceSpawnGroupRoot(group) != null
				|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0;
			if (runtimeExists)
			{
				HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
					state, m_PhysicalWar, operation.m_sProjectionId);
				if (!retired || !retired.m_bSuccess)
					return QuarantineOperationAuthority(state, operation,
						"restored exact mission guard runtime could not retire safely");
			}
			int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
			HST_ForceSpawnQueueCallbackResult requeued = m_SpawnQueue.RequeueSuccessfulProjectionAfterRestore(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				nowSecond,
				nowSecond + DEPLOYMENT_GRACE_SECONDS);
			if (!requeued || !requeued.m_bAccepted)
			{
				string reason = "restored exact mission guard survivor roster could not be requeued";
				m_SpawnQueue.CompleteQuarantinedSuccessfulProjectionCancellation(
					state.m_aForceSpawnResults, batch.m_sResultId, batch.m_sProjectionId, nowSecond, reason);
				return QuarantineOperationAuthority(state, operation, reason);
			}
			HST_ForceSpawnQueueCallbackResult heldRequeue = m_SpawnQueue.HoldPendingProjectionForStrategicSimulation(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				nowSecond);
			if (!heldRequeue || !heldRequeue.m_bAccepted)
				return QuarantineOperationAuthority(state, operation,
					"restored exact mission guard survivor roster could not enter strategic hold");
			changed = true;
		}
		else if (!batch.m_bStrategicProjectionHeld)
		{
			HST_ForceSpawnQueueCallbackResult heldPending = m_SpawnQueue.HoldPendingProjectionForStrategicSimulation(
				state.m_aForceSpawnResults,
				manifest,
				batch.m_sResultId,
				batch.m_sProjectionId,
				Math.Max(0, state.m_iElapsedSeconds));
			if (!heldPending || !heldPending.m_bAccepted)
				return QuarantineOperationAuthority(state, operation,
					"restored exact mission guard pending roster could not enter strategic hold");
			changed = true;
		}
		NormalizeHeldBatchProcessAuthority(batch, Math.Max(0, state.m_iElapsedSeconds));
		ClearGroupProcessAuthority(group);
		int now = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iMaterializationStateEnteredAtSecond = now;
		operation.m_iEngagementStateEnteredAtSecond = now;
		operation.m_iStrategicLastUpdateSecond = now;
		operation.m_iLastProjectionDecisionSecond = now;
		operation.m_iLastProgressAtSecond = now;
		operation.m_sLastProjectionReason = "restored exact mission guard as held strategic authority";
		operation.m_iRevision++;
		int living = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
		operation.m_iLastVirtualFriendlyCount = living;
		SyncGroupRoster(group, living);
		group.m_vPosition = operation.m_vStrategicPosition;
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		group.m_vTargetPosition = operation.m_vAssignmentPosition;
		ApplyVirtualRuntimeStatus(group);
		group.m_iLifecycleRevision++;
		return true;
	}

	protected int ResolveQuarantineVersionForOperation(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission)
	{
		if (mission && IsSupportedExactMissionId(mission.m_sMissionId))
			return ResolveQuarantinedContractVersion(mission.m_sMissionId);
		if (operation)
		{
			int contractVersion = ResolveQuarantinedContractVersionForExactContract(
				operation.m_iContractVersion);
			if (contractVersion != 0)
				return contractVersion;
		}
		if (!state || !operation)
			return 0;
		HST_ForceManifestState manifest = state.FindForceManifest(operation.m_sManifestId);
		if (manifest)
		{
			if (manifest.m_sPolicyId == OFFICER_POLICY_ID)
				return OFFICER_QUARANTINED_CONTRACT_VERSION;
			if (manifest.m_sPolicyId == TRAITOR_POLICY_ID)
				return TRAITOR_QUARANTINED_CONTRACT_VERSION;
			if (manifest.m_sPolicyId == SPECOPS_POLICY_ID)
				return SPECOPS_QUARANTINED_CONTRACT_VERSION;
		}
		// Typed mission-guard authority originated with the Schema-55 officer
		// contract. If no later-family evidence survives, retain that quarantine
		// default instead of leaving an unsupported version live forever.
		return OFFICER_QUARANTINED_CONTRACT_VERSION;
	}

	protected bool QuarantineOperationAuthority(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		string reason)
	{
		if (!state || !operation)
			return false;
		string prefix = "exact mission guard authority quarantined without refund or legacy conversion: ";
		string failure = reason;
		if (failure.IsEmpty())
			failure = "unspecified authority conflict";
		if (!failure.StartsWith(prefix))
			failure = prefix + failure;
		HST_ActiveMissionState mission = state.FindActiveMission(operation.m_sMissionInstanceId);
		int quarantineVersion = ResolveQuarantineVersionForOperation(state, operation, mission);
		bool changed = operation.m_sLastProjectionReason != failure;
		if (quarantineVersion != 0 && operation.m_iContractVersion != quarantineVersion)
		{
			operation.m_iContractVersion = quarantineVersion;
			changed = true;
		}
		operation.m_sLastProjectionReason = failure;
		operation.m_iRevision++;
		if (mission && mission.m_sOperationId == operation.m_sOperationId
			&& ((!operation.m_sManifestId.IsEmpty() && mission.m_sManifestId == operation.m_sManifestId)
				|| (!operation.m_sSpawnResultId.IsEmpty() && mission.m_sSpawnResultId == operation.m_sSpawnResultId)))
		{
			int missionQuarantineVersion = ResolveQuarantinedContractVersion(mission.m_sMissionId);
			if (missionQuarantineVersion != 0
				&& operation.m_iContractVersion != missionQuarantineVersion)
			{
				operation.m_iContractVersion = missionQuarantineVersion;
				operation.m_iRevision++;
				changed = true;
			}
			if (missionQuarantineVersion != 0
				&& mission.m_iOperationContractVersion != missionQuarantineVersion)
			{
				mission.m_iOperationContractVersion = missionQuarantineVersion;
				changed = true;
			}
			mission.m_sRuntimeFailureReason = failure;
		}
		return ApplyQuarantineStatus(state, operation, failure) || changed;
	}

	protected bool QuarantineUncommittedMission(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		string reason)
	{
		if (!mission || !IsSupportedExactMissionId(mission.m_sMissionId))
			return false;
		mission.m_iOperationContractVersion = ResolveQuarantinedContractVersion(mission.m_sMissionId);
		mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
		mission.m_iRemainingSeconds = 0;
		MarkMissionRuntimeFailed(state, mission, reason);
		return true;
	}

	protected bool ApplyQuarantineStatus(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		string reason)
	{
		if (!state || !operation)
			return false;
		if (reason.IsEmpty())
			reason = "exact mission guard authority remains quarantined";
		bool changed;
		HST_ActiveMissionState mission = state.FindActiveMission(operation.m_sMissionInstanceId);
		if (mission && mission.m_sOperationId == operation.m_sOperationId
			&& ((!operation.m_sManifestId.IsEmpty() && mission.m_sManifestId == operation.m_sManifestId)
				|| (!operation.m_sSpawnResultId.IsEmpty() && mission.m_sSpawnResultId == operation.m_sSpawnResultId)))
		{
			int missionQuarantineVersion = ResolveQuarantinedContractVersion(mission.m_sMissionId);
			if (missionQuarantineVersion != 0
				&& operation.m_iContractVersion != missionQuarantineVersion)
			{
				operation.m_iContractVersion = missionQuarantineVersion;
				operation.m_iRevision++;
				changed = true;
			}
			if (missionQuarantineVersion != 0
				&& mission.m_iOperationContractVersion != missionQuarantineVersion)
			{
				mission.m_iOperationContractVersion = missionQuarantineVersion;
				changed = true;
			}
			mission.m_sRuntimeFailureReason = reason;
		}
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (!BatchClaimsOperationAuthority(batch, operation))
				continue;
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			{
				bool retiredChanged;
				TryRetireQuarantinedSuccessfulProjection(state, operation, batch, reason, retiredChanged);
				changed = retiredChanged || changed;
			}
			if (m_SpawnQueue)
			{
				HST_ForceSpawnQueueCallbackResult cancelled = m_SpawnQueue.RequestCancel(
					state.m_aForceSpawnResults,
					batch.m_sResultId,
					Math.Max(0, state.m_iElapsedSeconds),
					reason);
				if (cancelled && cancelled.m_bStateChanged)
					changed = true;
			}
			else if (!batch.m_bCancelRequested || batch.m_sLastFailureReason != reason)
			{
				batch.m_bCancelRequested = true;
				batch.m_sLastFailureReason = reason;
				changed = true;
			}
		}
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (!GroupClaimsOperationAuthority(group, operation))
				continue;
			if (m_SpawnAdapter && m_PhysicalWar
				&& m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) == 0
				&& m_PhysicalWar.GetForceSpawnGroupRoot(group) == null
				&& m_PhysicalWar.CountForceSpawnRuntimeMembers(group) == 0)
			{
				bool hadProcess = GroupHasProcessResidue(group);
				ClearGroupProcessAuthority(group);
				if (hadProcess)
					changed = true;
			}
			if (group.m_sRuntimeStatus != QUARANTINE_STATUS
				|| group.m_sSpawnFailureReason != reason
				|| group.m_sSpawnFallbackMode != QUARANTINE_STATUS)
			{
				group.m_sRuntimeStatus = QUARANTINE_STATUS;
				group.m_sSpawnFailureReason = reason;
				group.m_sSpawnFallbackMode = QUARANTINE_STATUS;
				group.m_iLifecycleRevision++;
				changed = true;
			}
		}
		return changed;
	}

	protected bool BatchHasProcessResidue(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return false;
		if (!batch.m_sNativeGroupId.IsEmpty())
			return true;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot)
				continue;
			if (!slot.m_sEntityId.IsEmpty() || !slot.m_sAssignedVehicleEntityId.IsEmpty()
				|| !slot.m_sNativeGroupId.IsEmpty() || slot.m_bAliveVerified
				|| slot.m_bFactionVerified || slot.m_bGroupVerified
				|| slot.m_bGameMasterVerified || slot.m_bProjectionVerified || slot.m_bSeatVerified)
				return true;
		}
		return false;
	}

	protected bool GroupHasProcessResidue(HST_ActiveGroupState group)
	{
		return group && (group.m_bSpawnedEntity || group.m_bSpawnAttempted
			|| !group.m_sRuntimeEntityId.IsEmpty() || group.m_iSpawnedAgentCount > 0
			|| group.m_iAssignedWaypointCount > 0);
	}

	protected bool TryRetireQuarantinedSuccessfulProjection(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		string reason,
		out bool changed)
	{
		changed = false;
		if (!state || !operation || !batch || !m_SpawnQueue || !m_SpawnAdapter || !m_PhysicalWar
			|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			return false;
		HST_ActiveGroupState group;
		if (!ResolveQuarantinedSuccessfulProjectionContext(state, operation, batch, group))
			return false;

		string originalBatchOperation = batch.m_sOperationId;
		string originalBatchManifest = batch.m_sManifestId;
		string originalBatchForce = batch.m_sForceId;
		string originalGroupOperation = group.m_sOperationId;
		string originalGroupManifest = group.m_sManifestId;
		string originalGroupResult = group.m_sSpawnResultId;
		string originalGroupForce = group.m_sForceId;
		string runtimeOperation = batch.m_sOperationId;
		if (runtimeOperation.IsEmpty())
			runtimeOperation = "quarantine_runtime_operation_" + operation.m_sProjectionId;
		string runtimeManifest = batch.m_sManifestId;
		if (runtimeManifest.IsEmpty())
			runtimeManifest = "quarantine_runtime_manifest_" + operation.m_sProjectionId;
		string runtimeForce = batch.m_sForceId;
		if (runtimeForce.IsEmpty())
			runtimeForce = "quarantine_runtime_force_" + operation.m_sProjectionId;
		batch.m_sOperationId = runtimeOperation;
		batch.m_sManifestId = runtimeManifest;
		batch.m_sForceId = runtimeForce;
		group.m_sOperationId = runtimeOperation;
		group.m_sManifestId = runtimeManifest;
		group.m_sSpawnResultId = batch.m_sResultId;
		group.m_sForceId = runtimeForce;

		bool retired = RetireQuarantinedSuccessfulProjectionWithRuntimeIdentity(
			state, operation, batch, group, reason, changed);
		group.m_sOperationId = originalGroupOperation;
		group.m_sManifestId = originalGroupManifest;
		group.m_sSpawnResultId = originalGroupResult;
		group.m_sForceId = originalGroupForce;
		batch.m_sOperationId = originalBatchOperation;
		batch.m_sManifestId = originalBatchManifest;
		batch.m_sForceId = originalBatchForce;
		return retired;
	}

	protected bool RetireQuarantinedSuccessfulProjectionWithRuntimeIdentity(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason,
		out bool changed)
	{
		changed = false;
		string runtimeKeyFailure;
		if (!m_SpawnAdapter.ValidateExactProjectionRuntimeKeys(batch, runtimeKeyFailure))
			return false;
		HST_ForceSpawnAdapterTickResult reconciled = m_SpawnAdapter.ReconcileQuarantinedExactInfantryProjectionAuthority(
			state,
			m_SpawnQueue,
			m_PhysicalWar,
			Math.Max(0, state.m_iElapsedSeconds),
			operation.m_sProjectionId);
		if (reconciled)
			changed = reconciled.m_bStateChanged || reconciled.m_bRuntimeChanged;
		if (!reconciled || reconciled.m_iFailedCount > 0)
			return false;
		int handles = m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId);
		bool runtimeExists = handles > 0 || m_PhysicalWar.GetForceSpawnGroupRoot(group) != null
			|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0;
		int living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
		if (!runtimeExists && living > 0)
			return false;
		if (runtimeExists)
		{
			string bindingFailure;
			if (!m_SpawnAdapter.ValidateExactLivingProjectionBindingsForPersistence(
				state, batch, m_SpawnQueue, m_PhysicalWar, bindingFailure))
				return false;
			HST_ForceSpawnAdapterRetireResult retired = m_SpawnAdapter.RetireProjectionRuntime(
				state, m_PhysicalWar, operation.m_sProjectionId);
			if (!retired || !retired.m_bSuccess)
				return false;
			changed = retired.m_bRuntimeChanged || changed;
		}
		if (m_SpawnAdapter.CountHandlesForProjection(operation.m_sProjectionId) > 0
			|| m_SpawnAdapter.CountHandlesForResultId(batch.m_sResultId) > 0
			|| m_PhysicalWar.GetForceSpawnGroupRoot(group)
			|| m_PhysicalWar.CountForceSpawnRuntimeMembers(group) > 0)
			return false;
		ClearGroupProcessAuthority(group);
		group.m_iLifecycleRevision++;
		changed = true;
		HST_ForceSpawnQueueCallbackResult terminal = m_SpawnQueue.CompleteQuarantinedSuccessfulProjectionCancellation(
			state.m_aForceSpawnResults,
			batch.m_sResultId,
			batch.m_sProjectionId,
			Math.Max(0, state.m_iElapsedSeconds),
			reason);
		if (!terminal || !terminal.m_bAccepted)
			return false;
		changed = terminal.m_bStateChanged || changed;
		return true;
	}

	protected bool ResolveQuarantinedSuccessfulProjectionContext(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		out HST_ActiveGroupState group)
	{
		group = null;
		if (!state || !operation || !batch || operation.m_sProjectionId.IsEmpty()
			|| batch.m_sResultId.IsEmpty() || !BatchClaimsOperationAuthority(batch, operation))
			return false;
		int projectionMatches;
		int resultMatches;
		foreach (HST_ForceSpawnResultState candidateBatch : state.m_aForceSpawnResults)
		{
			if (!candidateBatch)
				continue;
			if (candidateBatch.m_sProjectionId == operation.m_sProjectionId)
				projectionMatches++;
			if (candidateBatch.m_sResultId == batch.m_sResultId)
				resultMatches++;
		}
		if (projectionMatches != 1 || resultMatches != 1
			|| batch.m_sProjectionId != operation.m_sProjectionId)
			return false;
		int groupMatches;
		foreach (HST_ActiveGroupState candidateGroup : state.m_aActiveGroups)
		{
			if (!candidateGroup || candidateGroup.m_sProjectionId != operation.m_sProjectionId)
				continue;
			group = candidateGroup;
			groupMatches++;
		}
		if (groupMatches != 1 || !group || !GroupClaimsOperationAuthority(group, operation)
			|| CountGroupIdentity(state, group) != 1)
			return false;
		foreach (HST_OperationRecordState competing : state.m_aOperations)
		{
			if (!competing || competing == operation)
				continue;
			if (competing.m_sProjectionId == operation.m_sProjectionId
				|| (!batch.m_sResultId.IsEmpty() && competing.m_sSpawnResultId == batch.m_sResultId)
				|| (!group.m_sGroupId.IsEmpty() && competing.m_sGroupId == group.m_sGroupId)
				|| (!batch.m_sManifestId.IsEmpty() && competing.m_sManifestId == batch.m_sManifestId)
				|| (!batch.m_sForceId.IsEmpty() && competing.m_sForceId == batch.m_sForceId))
				return false;
		}
		return true;
	}

	protected bool BatchClaimsOperationAuthority(
		HST_ForceSpawnResultState batch,
		HST_OperationRecordState operation)
	{
		if (!batch || !operation || operation.m_sOperationId.IsEmpty()
			|| batch.m_sOperationId != operation.m_sOperationId)
			return false;
		return (!operation.m_sSpawnResultId.IsEmpty() && batch.m_sResultId == operation.m_sSpawnResultId)
			|| (!operation.m_sManifestId.IsEmpty() && batch.m_sManifestId == operation.m_sManifestId)
			|| (!operation.m_sForceId.IsEmpty() && batch.m_sForceId == operation.m_sForceId)
			|| (!operation.m_sProjectionId.IsEmpty() && batch.m_sProjectionId == operation.m_sProjectionId);
	}

	protected bool GroupClaimsOperationAuthority(
		HST_ActiveGroupState group,
		HST_OperationRecordState operation)
	{
		if (!group || !operation || operation.m_sOperationId.IsEmpty()
			|| group.m_sOperationId != operation.m_sOperationId)
			return false;
		return (!operation.m_sGroupId.IsEmpty() && group.m_sGroupId == operation.m_sGroupId)
			|| (!operation.m_sManifestId.IsEmpty() && group.m_sManifestId == operation.m_sManifestId)
			|| (!operation.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == operation.m_sSpawnResultId)
			|| (!operation.m_sForceId.IsEmpty() && group.m_sForceId == operation.m_sForceId)
			|| (!operation.m_sProjectionId.IsEmpty() && group.m_sProjectionId == operation.m_sProjectionId);
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
			return "exact mission guard runtime context is missing";
		mission = state.FindActiveMission(operation.m_sMissionInstanceId);
		manifest = state.FindForceManifest(operation.m_sManifestId);
		batch = state.FindForceSpawnResult(operation.m_sSpawnResultId);
		group = state.FindActiveGroup(operation.m_sGroupId);
		return ValidateCommittedGraph(state, mission, operation, manifest, batch, group, false);
	}

	protected void MarkMissionRuntimeFailed(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		string reason)
	{
		if (!mission)
			return;
		mission.m_sRuntimePhase = "failed";
		mission.m_sRuntimeFailureReason = reason;
		if (!state)
			return;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId
				|| objective.m_bComplete)
				continue;
			objective.m_bFailed = true;
		}
	}

	protected vector ResolveHVTPosition(HST_MissionAssetState hvt)
	{
		if (!hvt)
			return "0 0 0";
		if (!IsZeroVector(hvt.m_vCurrentPosition))
			return hvt.m_vCurrentPosition;
		if (!IsZeroVector(hvt.m_vSourcePosition))
			return hvt.m_vSourcePosition;
		if (!IsZeroVector(hvt.m_vTargetPosition))
			return hvt.m_vTargetPosition;
		if (!IsZeroVector(hvt.m_vLastKnownPosition))
			return hvt.m_vLastKnownPosition;
		return "0 0 0";
	}

	// Pure deterministic offset. The adapter ground-resolves the root at materialization;
	// keeping this calculation world-free also makes detached admission replay stable.
	protected vector ResolveGuardAnchor(string missionInstanceId, vector hvtPosition)
	{
		if (missionInstanceId.IsEmpty() || IsZeroVector(hvtPosition))
			return "0 0 0";
		int direction = PositiveModulo(missionInstanceId.Hash(), 8);
		if (direction == 0)
			return hvtPosition + "10 0 0";
		if (direction == 1)
			return hvtPosition + "7 0 7";
		if (direction == 2)
			return hvtPosition + "0 0 10";
		if (direction == 3)
			return hvtPosition + "-7 0 7";
		if (direction == 4)
			return hvtPosition + "-10 0 0";
		if (direction == 5)
			return hvtPosition + "-7 0 -7";
		if (direction == 6)
			return hvtPosition + "0 0 -10";
		return hvtPosition + "7 0 -7";
	}

	protected bool IsTerminalSpawnBatch(HST_ForceSpawnResultState batch)
	{
		return batch && (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED);
	}

	protected void ApplyVirtualRuntimeStatus(HST_ActiveGroupState group)
	{
		if (group)
			group.m_sRuntimeStatus = "mission_guard_virtual";
	}

	protected void SyncGroupRoster(HST_ActiveGroupState group, int living)
	{
		if (!group)
			return;
		int bounded = Math.Max(0, Math.Min(group.m_iOriginalInfantryCount, living));
		group.m_iInfantryCount = bounded;
		group.m_iDurableLivingInfantryCount = bounded;
		group.m_iLastSeenAliveCount = bounded;
		group.m_iSurvivorInfantryCount = bounded;
		group.m_iVehicleCount = 0;
		group.m_iSurvivorVehicleCount = 0;
	}

	protected int ResolveLivingRoster(
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_OperationRecordState operation)
	{
		int accepted;
		if (manifest)
			accepted = Math.Max(0, manifest.m_iAcceptedMemberCount);
		else if (group)
			accepted = Math.Max(0, group.m_iOriginalInfantryCount);
		int living;
		bool authoritative;
		if (batch && m_SpawnQueue)
		{
			if (batch.m_bStrategicProjectionHeld)
			{
				living = m_SpawnQueue.CountStrategicLivingMemberSlots(batch);
				authoritative = true;
			}
			else if (batch.m_iSuccessfulHandoffCount > 0
				|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			{
				living = m_SpawnQueue.CountDurableLivingMemberSlots(batch);
				authoritative = true;
			}
		}
		if (!authoritative && operation)
			living = operation.m_iLastVirtualFriendlyCount;
		if (!authoritative && living <= 0 && group && group.m_sRuntimeStatus != "eliminated")
			living = Math.Max(group.m_iDurableLivingInfantryCount, group.m_iSurvivorInfantryCount);
		return Math.Max(0, Math.Min(accepted, living));
	}

	protected void ClearGroupProcessAuthority(HST_ActiveGroupState group)
	{
		if (!group)
			return;
		group.m_bSpawnedEntity = false;
		group.m_bSpawnAttempted = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
	}

	protected void NormalizeHeldBatchProcessAuthority(
		HST_ForceSpawnResultState batch,
		int nowSecond)
	{
		if (!batch)
			return;
		batch.m_sNativeGroupId = "";
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING;
		batch.m_bStrategicProjectionHeld = true;
		batch.m_bCancelRequested = false;
		batch.m_iStrategicHoldSinceSecond = nowSecond;
		batch.m_iNextAttemptSecond = 0;
		batch.m_iUpdatedAtSecond = nowSecond;
		batch.m_iCompletedAtSecond = 0;
		batch.m_iLastLifecycleSecond = nowSecond;
		batch.m_iLifecycleRevision++;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot)
				continue;
			slot.m_sSpawnedPrefab = "";
			slot.m_sEntityId = "";
			slot.m_sAssignedVehicleEntityId = "";
			slot.m_sNativeGroupId = "";
			slot.m_bAliveVerified = false;
			slot.m_bFactionVerified = false;
			slot.m_bGroupVerified = false;
			slot.m_bGameMasterVerified = false;
			slot.m_bProjectionVerified = false;
			slot.m_bSeatVerified = false;
			slot.m_iUpdatedAtSecond = nowSecond;
			if (slot.m_bCasualtyConfirmed)
			{
				slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
				continue;
			}
			slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED;
		}
	}

	protected int CountMissionIdentity(HST_CampaignState state, HST_ActiveMissionState expected)
	{
		int count;
		if (!state || !expected)
			return count;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (mission && mission.m_sInstanceId == expected.m_sInstanceId)
				count++;
		}
		return count;
	}

	protected int CountManifestIdentity(HST_CampaignState state, HST_ForceManifestState expected)
	{
		int count;
		if (!state || !expected)
			return count;
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
		{
			if (manifest && (manifest.m_sManifestId == expected.m_sManifestId
				|| (!expected.m_sOperationId.IsEmpty() && manifest.m_sOperationId == expected.m_sOperationId)))
				count++;
		}
		return count;
	}

	protected int CountOperationIdentity(HST_CampaignState state, HST_OperationRecordState expected)
	{
		int count;
		if (!state || !expected)
			return count;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (operation && (operation.m_sOperationId == expected.m_sOperationId
				|| (!expected.m_sMissionInstanceId.IsEmpty() && operation.m_sMissionInstanceId == expected.m_sMissionInstanceId)
				|| (!expected.m_sManifestId.IsEmpty() && operation.m_sManifestId == expected.m_sManifestId)
				|| (!expected.m_sSpawnResultId.IsEmpty() && operation.m_sSpawnResultId == expected.m_sSpawnResultId)
				|| (!expected.m_sProjectionId.IsEmpty() && operation.m_sProjectionId == expected.m_sProjectionId)
				|| (!expected.m_sGroupId.IsEmpty() && operation.m_sGroupId == expected.m_sGroupId)))
				count++;
		}
		return count;
	}

	protected int CountBatchIdentity(HST_CampaignState state, HST_ForceSpawnResultState expected)
	{
		int count;
		if (!state || !expected)
			return count;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && (batch.m_sResultId == expected.m_sResultId
				|| (!expected.m_sRequestId.IsEmpty() && batch.m_sRequestId == expected.m_sRequestId)
				|| (!expected.m_sOperationId.IsEmpty() && batch.m_sOperationId == expected.m_sOperationId)
				|| (!expected.m_sManifestId.IsEmpty() && batch.m_sManifestId == expected.m_sManifestId)
				|| (!expected.m_sForceId.IsEmpty() && batch.m_sForceId == expected.m_sForceId)
				|| (!expected.m_sProjectionId.IsEmpty() && batch.m_sProjectionId == expected.m_sProjectionId)))
				count++;
		}
		return count;
	}

	protected int CountGroupIdentity(HST_CampaignState state, HST_ActiveGroupState expected)
	{
		int count;
		if (!state || !expected)
			return count;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && (group.m_sGroupId == expected.m_sGroupId
				|| (!expected.m_sOperationId.IsEmpty() && group.m_sOperationId == expected.m_sOperationId)
				|| (!expected.m_sManifestId.IsEmpty() && group.m_sManifestId == expected.m_sManifestId)
				|| (!expected.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == expected.m_sSpawnResultId)
				|| (!expected.m_sForceId.IsEmpty() && group.m_sForceId == expected.m_sForceId)
				|| (!expected.m_sProjectionId.IsEmpty() && group.m_sProjectionId == expected.m_sProjectionId)))
				count++;
		}
		return count;
	}

	protected int CountManifestMembers(HST_ForceManifestState manifest, string slotId)
	{
		int count;
		if (!manifest || slotId.IsEmpty())
			return count;
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (member && member.m_sSlotId == slotId)
				count++;
		}
		return count;
	}

	protected int CountBatchSlots(HST_ForceSpawnResultState batch, string slotId, string slotKind)
	{
		int count;
		if (!batch || slotId.IsEmpty() || slotKind.IsEmpty())
			return count;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotId == slotId && slot.m_sSlotKind == slotKind)
				count++;
		}
		return count;
	}

	protected bool PositionsMatch(vector left, vector right, float toleranceMeters = 0.5)
	{
		return Distance2D(left, right) <= toleranceMeters;
	}

	protected int PositiveModulo(int value, int divisor)
	{
		if (divisor <= 0)
			return 0;
		int result = value % divisor;
		if (result < 0)
			result += divisor;
		return result;
	}

	protected float Distance2D(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}

	protected bool IsZeroVector(vector value)
	{
		return Math.AbsFloat(value[0]) < 0.01
			&& Math.AbsFloat(value[1]) < 0.01
			&& Math.AbsFloat(value[2]) < 0.01;
	}
}
