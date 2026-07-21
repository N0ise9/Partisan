// Schema-66 persistence boundary for exact, town-scoped local security.
// Cyclic waypoints and native entities remain process-local. The frozen
// manifest and SpawnQueue member slots are the durable survivor authority.
class HST_LocalSecuritySaveValidationService
{
	static const int SCHEMA_VERSION = 66;
	static const int CONTRACT_VERSION = 1;
	static const int QUARANTINE_CONTRACT_VERSION = -66;
	static const int MAX_PATROL_ROWS = 512;
	static const int MAX_MUTABLE_REVISION = int.MAX - 16;
	static const int MAX_ID_CHARACTERS = 192;
	static const int MAX_REASON_CHARACTERS = 320;
	static const int MIN_INFANTRY = 2;
	static const int MAX_INFANTRY = 5;
	static const int PROJECTION_CONTRACT_VERSION = 1;
	static const string EXACT_POLICY_ID = "exact_local_security_patrol_v1";
	static const string EXACT_FORCE_KIND = "local_security_patrol";
	static const string EXACT_INTENT_ID = "town_police";
	static const string EXACT_GROUP_MODE = "exact_local_security_patrol";
	static const string SOURCE_TYPE = "town_police_presence";
	static const string ASSIGNMENT_KIND = "local_security_patrol";
	static const string RECALL_POLICY_ID = "retire_without_refund";
	static const string SETTLEMENT_POLICY_ID = "exact_local_security_patrol_roster";
	static const string SETTLEMENT_KIND = "exact_local_security_patrol_terminal";
	static const string LEGACY_GROUP_MODE_TOKEN = "town_security_police";
	static const string STATUS_ACTIVE = "active";
	static const string STATUS_TERMINAL = "terminal";
	static const string STATUS_QUARANTINED = "quarantined";
	static const string LOSS_EVENT_KIND = "local_security_patrol_destroyed";
	static const string MIGRATION_EVENT_ID = "migration_schema66_local_security_patrol_authority";

	protected HST_CampaignSaveData m_SaveData;
	protected bool m_bPrepared;

	void PrepareBeforeGenericNormalization(
		HST_CampaignSaveData saveData,
		int restoredSchemaVersion)
	{
		m_SaveData = saveData;
		if (!m_SaveData)
			return;
		EnsureArrays();
		if (m_bPrepared)
			return;
		m_bPrepared = true;
		if (restoredSchemaVersion < SCHEMA_VERSION)
			MigrateLegacyLocalSecurity();
		HoldOrphanStrongClaimants();
	}

	void Normalize(HST_CampaignSaveData saveData, int restoredSchemaVersion)
	{
		m_SaveData = saveData;
		if (!m_SaveData)
			return;
		EnsureArrays();
		if (!m_bPrepared)
			PrepareBeforeGenericNormalization(saveData, restoredSchemaVersion);
		if (restoredSchemaVersion < SCHEMA_VERSION)
			return;

		for (int nullIndex = m_SaveData.m_aLocalSecurityPatrols.Count() - 1; nullIndex >= 0; nullIndex--)
		{
			if (!m_SaveData.m_aLocalSecurityPatrols[nullIndex])
				m_SaveData.m_aLocalSecurityPatrols.Remove(nullIndex);
		}
		if (m_SaveData.m_aLocalSecurityPatrols.Count() > MAX_PATROL_ROWS)
		{
			foreach (HST_LocalSecurityPatrolState overBound : m_SaveData.m_aLocalSecurityPatrols)
				QuarantineAggregate(overBound, "schema66 local-security patrol bound exceeded");
			HoldOrphanStrongClaimants();
			return;
		}

		foreach (HST_LocalSecurityPatrolState patrol : m_SaveData.m_aLocalSecurityPatrols)
		{
			if (!patrol)
				continue;
			if (patrol.m_iContractVersion == QUARANTINE_CONTRACT_VERSION
				|| patrol.m_sStatus == STATUS_QUARANTINED)
			{
				NormalizeQuarantined(patrol);
				continue;
			}

			HST_OperationRecordState operation = FindUniqueOperation(patrol.m_sOperationId);
			HST_ForceManifestState manifest = FindUniqueManifest(patrol.m_sManifestId);
			HST_ForceSpawnResultState batch = FindUniqueBatch(patrol.m_sSpawnResultId);
			HST_ActiveGroupState group = FindUniqueGroup(patrol.m_sGroupId);
			string failure = ValidateAggregate(patrol, operation, manifest, batch, group);
			if (!failure.IsEmpty())
			{
				QuarantineAggregate(patrol, failure);
				continue;
			}
			if (patrol.m_sStatus == STATUS_ACTIVE)
				NormalizeActiveRuntime(patrol, operation, batch, group);
		}
		HoldOrphanStrongClaimants();
	}

	static string BuildPatrolId(
		string zoneId,
		string factionKey,
		int ownershipRevision,
		int epoch)
	{
		if (zoneId.IsEmpty() || factionKey.IsEmpty()
			|| ownershipRevision <= 0 || epoch <= 0)
			return "";
		return string.Format(
			"local_security_patrol_%1_%2_%3_%4",
			zoneId.Hash(),
			factionKey.Hash(),
			ownershipRevision,
			epoch);
	}

	static string BuildOperationId(string patrolId)
	{
		return HST_StableIdService.BuildOperationId("local_security", patrolId);
	}

	static string BuildManifestId(string operationId)
	{
		if (operationId.IsEmpty())
			return "";
		return "manifest_" + operationId;
	}

	static string BuildSpawnResultId(string patrolId)
	{
		if (patrolId.IsEmpty())
			return "";
		return "spawn_" + patrolId;
	}

	static string BuildForceId(string operationId)
	{
		if (operationId.IsEmpty())
			return "";
		return "force_" + operationId;
	}

	static string BuildProjectionId(string operationId)
	{
		if (operationId.IsEmpty())
			return "";
		return "projection_" + operationId;
	}

	static string BuildLossEventId(string patrolId)
	{
		if (patrolId.IsEmpty())
			return "";
		return LOSS_EVENT_KIND + "_" + patrolId;
	}

	static bool IsSchema66LocalSecurityOperationClaimant(
		HST_CampaignSaveData saveData,
		HST_OperationRecordState operation)
	{
		if (!operation)
			return false;
		if (operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_LOCAL_SECURITY_PATROL
			|| !operation.m_sLocalSecurityPatrolId.IsEmpty())
			return true;
		if (!saveData || !saveData.m_aLocalSecurityPatrols)
			return false;
		foreach (HST_LocalSecurityPatrolState patrol : saveData.m_aLocalSecurityPatrols)
		{
			if (patrol && !patrol.m_sOperationId.IsEmpty()
				&& operation.m_sOperationId == patrol.m_sOperationId)
				return true;
		}
		return false;
	}

	static bool IsSchema66LocalSecurityManifestClaimant(
		HST_CampaignSaveData saveData,
		HST_ForceManifestState manifest)
	{
		if (!manifest)
			return false;
		if (manifest.m_sPolicyId == EXACT_POLICY_ID
			|| manifest.m_sForceKind == EXACT_FORCE_KIND
			|| manifest.m_sIntentId == EXACT_INTENT_ID)
			return true;
		if (!saveData || !saveData.m_aLocalSecurityPatrols)
			return false;
		foreach (HST_LocalSecurityPatrolState patrol : saveData.m_aLocalSecurityPatrols)
		{
			if (!patrol)
				continue;
			if ((!patrol.m_sManifestId.IsEmpty()
					&& manifest.m_sManifestId == patrol.m_sManifestId)
				|| (!patrol.m_sOperationId.IsEmpty()
					&& manifest.m_sOperationId == patrol.m_sOperationId))
				return true;
		}
		return false;
	}

	static bool IsSchema66LocalSecurityBatchClaimant(
		HST_CampaignSaveData saveData,
		HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return false;
		if (!saveData || !saveData.m_aLocalSecurityPatrols)
			return batch.m_sRequestId.StartsWith("local_security_patrol_");
		foreach (HST_LocalSecurityPatrolState patrol : saveData.m_aLocalSecurityPatrols)
		{
			if (!patrol)
				continue;
			if ((!patrol.m_sSpawnResultId.IsEmpty()
					&& batch.m_sResultId == patrol.m_sSpawnResultId)
				|| (!patrol.m_sPatrolId.IsEmpty()
					&& batch.m_sRequestId == patrol.m_sPatrolId)
				|| (!patrol.m_sOperationId.IsEmpty()
					&& batch.m_sOperationId == patrol.m_sOperationId)
				|| (!patrol.m_sManifestId.IsEmpty()
					&& batch.m_sManifestId == patrol.m_sManifestId)
				|| (!patrol.m_sForceId.IsEmpty()
					&& batch.m_sForceId == patrol.m_sForceId)
				|| (!patrol.m_sProjectionId.IsEmpty()
					&& batch.m_sProjectionId == patrol.m_sProjectionId))
				return true;
		}
		HST_OperationRecordState operation = FindStaticOperation(saveData, batch.m_sOperationId);
		return IsSchema66LocalSecurityOperationClaimant(saveData, operation)
			|| batch.m_sRequestId.StartsWith("local_security_patrol_");
	}

	static bool IsSchema66LocalSecurityGroupClaimant(
		HST_CampaignSaveData saveData,
		HST_ActiveGroupState group)
	{
		if (!group)
			return false;
		if (!group.m_sLocalSecurityPatrolId.IsEmpty()
			|| group.m_sSpawnFallbackMode == EXACT_GROUP_MODE
			|| group.m_sSpawnFallbackMode.StartsWith(EXACT_GROUP_MODE + "_")
			|| group.m_sRuntimeStatus == "exact_local_security_quarantined")
			return true;
		if (!saveData || !saveData.m_aLocalSecurityPatrols)
			return false;
		foreach (HST_LocalSecurityPatrolState patrol : saveData.m_aLocalSecurityPatrols)
		{
			if (!patrol)
				continue;
			if ((!patrol.m_sGroupId.IsEmpty()
					&& group.m_sGroupId == patrol.m_sGroupId)
				|| (!patrol.m_sOperationId.IsEmpty()
					&& group.m_sOperationId == patrol.m_sOperationId)
				|| (!patrol.m_sManifestId.IsEmpty()
					&& group.m_sManifestId == patrol.m_sManifestId)
				|| (!patrol.m_sSpawnResultId.IsEmpty()
					&& group.m_sSpawnResultId == patrol.m_sSpawnResultId)
				|| (!patrol.m_sForceId.IsEmpty()
					&& group.m_sForceId == patrol.m_sForceId)
				|| (!patrol.m_sProjectionId.IsEmpty()
					&& group.m_sProjectionId == patrol.m_sProjectionId))
				return true;
		}
		HST_OperationRecordState operation = FindStaticOperation(saveData, group.m_sOperationId);
		return IsSchema66LocalSecurityOperationClaimant(saveData, operation);
	}

	protected void EnsureArrays()
	{
		if (!m_SaveData.m_aLocalSecurityPatrols)
			m_SaveData.m_aLocalSecurityPatrols = new array<ref HST_LocalSecurityPatrolState>();
		if (!m_SaveData.m_aZones)
			m_SaveData.m_aZones = new array<ref HST_ZoneState>();
		if (!m_SaveData.m_aActiveGroups)
			m_SaveData.m_aActiveGroups = new array<ref HST_ActiveGroupState>();
		if (!m_SaveData.m_aOperations)
			m_SaveData.m_aOperations = new array<ref HST_OperationRecordState>();
		if (!m_SaveData.m_aForceManifests)
			m_SaveData.m_aForceManifests = new array<ref HST_ForceManifestState>();
		if (!m_SaveData.m_aForceSpawnResults)
			m_SaveData.m_aForceSpawnResults = new array<ref HST_ForceSpawnResultState>();
		if (!m_SaveData.m_aTownInfluenceEvents)
			m_SaveData.m_aTownInfluenceEvents = new array<ref HST_TownInfluenceEventState>();
		if (!m_SaveData.m_aCampaignEvents)
			m_SaveData.m_aCampaignEvents = new array<ref HST_CampaignEventState>();
	}

	protected void MigrateLegacyLocalSecurity()
	{
		int removedGroups;
		foreach (HST_ZoneState zone : m_SaveData.m_aZones)
		{
			if (zone)
				zone.m_sLocalSecurityPatrolId = "";
		}
		for (int groupIndex = m_SaveData.m_aActiveGroups.Count() - 1; groupIndex >= 0; groupIndex--)
		{
			HST_ActiveGroupState group = m_SaveData.m_aActiveGroups[groupIndex];
			if (!IsLegacyDisposablePoliceGroup(group))
				continue;
			m_SaveData.m_aActiveGroups.Remove(groupIndex);
			removedGroups++;
		}
		foreach (HST_LocalSecurityPatrolState unexpected : m_SaveData.m_aLocalSecurityPatrols)
		{
			if (unexpected)
				QuarantineAggregate(unexpected, "pre-schema-66 save carried unsupported exact local-security authority");
		}
		if (HasCampaignEvent(MIGRATION_EVENT_ID))
			return;
		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = MIGRATION_EVENT_ID;
		eventState.m_sCategory = "migration";
		eventState.m_sAggregateType = "local_security_patrol";
		eventState.m_sAggregateId = "schema66";
		eventState.m_sTransition = "legacy_security_preserved_without_exact_patrol";
		eventState.m_sReason = string.Format(
			"preserved logical police, roadblock, owner, support, and garrison facts; retired %1 disposable legacy police projection rows without inventing exact rosters, casualties, operations, or refunds",
			removedGroups);
		eventState.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		m_SaveData.m_aCampaignEvents.Insert(eventState);
	}

	protected bool IsLegacyDisposablePoliceGroup(HST_ActiveGroupState group)
	{
		if (!group || !group.m_sSpawnFallbackMode.Contains(LEGACY_GROUP_MODE_TOKEN))
			return false;
		return group.m_sLocalSecurityPatrolId.IsEmpty()
			&& group.m_sManifestId.IsEmpty()
			&& group.m_sSpawnResultId.IsEmpty()
			&& group.m_sForceId.IsEmpty()
			&& group.m_sProjectionId.IsEmpty()
			&& group.m_sMissionInstanceId.IsEmpty()
			&& group.m_sSupportRequestId.IsEmpty()
			&& group.m_sEnemyOrderId.IsEmpty()
			&& group.m_sConvoyElementId.IsEmpty()
			&& group.m_sGarrisonZoneId.IsEmpty()
			&& group.m_sQRFInstanceId.IsEmpty();
	}

	protected string ValidateAggregate(
		HST_LocalSecurityPatrolState patrol,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		string failure = ValidateEnvelope(patrol);
		if (!failure.IsEmpty())
			return failure;
		if (CountPatrolsByZone(patrol.m_sZoneId) != 1
			|| CountPatrolsById(patrol.m_sPatrolId) != 1)
			return "schema66 local-security envelope identity is ambiguous";
		HST_ZoneState zone = FindUniqueTown(patrol.m_sZoneId);
		if (!zone)
			return "schema66 local-security zone is missing, duplicated, noncanonical, or not a town";
		if (zone.m_sLocalSecurityPatrolId != patrol.m_sPatrolId)
			return "schema66 local-security zone backlink conflicts";
		if (operation
			&& (!PositionsMatch(operation.m_vOriginPosition, zone.m_vPosition)
				|| !PositionsMatch(operation.m_vAssignmentPosition, zone.m_vPosition)
				|| !PositionsMatch(operation.m_vTacticalTargetPosition, zone.m_vPosition)))
			return "schema66 local-security fixed town positions conflict";

		if (patrol.m_sStatus == STATUS_ACTIVE)
		{
			if (zone.m_sOwnerFactionKey != patrol.m_sFactionKey
				|| zone.m_iOwnershipRevision != patrol.m_iOwnershipRevision)
				return "schema66 active local-security owner revision conflicts";
			if (!operation || !manifest || !batch || !group)
				return "schema66 active local-security graph is incomplete or ambiguous";
			failure = ValidateOperation(patrol, operation, false);
			if (!failure.IsEmpty())
				return failure;
			failure = ValidateManifest(patrol, manifest);
			if (!failure.IsEmpty())
				return failure;
			if (operation.m_iDeterministicSeed <= 0
				|| operation.m_iDeterministicSeed != manifest.m_iDeterministicSeed)
				return "schema66 local-security deterministic seed conflicts";
			failure = ValidateActiveRuntime(patrol, operation, manifest, batch, group);
			if (!failure.IsEmpty())
				return failure;
		}
		else
		{
			if (CountBatchClaimants(patrol) != 0 || CountGroupClaimants(patrol) != 0)
				return "schema66 terminal local-security envelope retains batch or group authority";
			if (!operation || !manifest)
				return "schema66 terminal local-security compact authority is incomplete or ambiguous";
			failure = ValidateOperation(patrol, operation, true);
			if (!failure.IsEmpty())
				return failure;
			failure = ValidateManifest(patrol, manifest);
			if (!failure.IsEmpty())
				return failure;
			if (operation.m_iDeterministicSeed <= 0
				|| operation.m_iDeterministicSeed != manifest.m_iDeterministicSeed)
				return "schema66 local-security deterministic seed conflicts";
			bool destroyed = operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED;
			if (destroyed != patrol.m_bLossEventApplied
				|| (destroyed && patrol.m_iLivingInfantryCount != 0))
				return "schema66 terminal local-security loss authority conflicts";
		}
		return ValidateLossReceipt(patrol);
	}

	protected string ValidateEnvelope(HST_LocalSecurityPatrolState patrol)
	{
		if (!patrol || patrol.m_iContractVersion != CONTRACT_VERSION)
			return "schema66 unsupported local-security patrol contract";
		if (patrol.m_iRevision <= 0 || patrol.m_iRevision >= MAX_MUTABLE_REVISION
			|| patrol.m_iOwnershipRevision <= 0
			|| patrol.m_iOwnershipRevision == int.MAX
			|| patrol.m_iEpoch <= 0 || patrol.m_iEpoch == int.MAX)
			return "schema66 local-security revision or epoch is invalid";
		if (!IsBoundedId(patrol.m_sPatrolId) || !IsBoundedId(patrol.m_sZoneId)
			|| !IsBoundedId(patrol.m_sFactionKey) || !IsBoundedId(patrol.m_sSourceType)
			|| !IsBoundedId(patrol.m_sSourceId))
			return "schema66 local-security source or identity is invalid";
		if (patrol.m_sSourceType != SOURCE_TYPE
			|| patrol.m_sPolicyId != EXACT_POLICY_ID
			|| patrol.m_iPoliceStrength <= 0 || patrol.m_iPoliceStrength > 1000000)
			return "schema66 local-security enemy policy is invalid";
		if (patrol.m_bBaseline != (patrol.m_iEpoch == 1))
			return "schema66 local-security baseline epoch evidence conflicts";
		if (patrol.m_sStatus != STATUS_ACTIVE && patrol.m_sStatus != STATUS_TERMINAL)
			return "schema66 local-security status is invalid";
		if (patrol.m_sPatrolId != BuildPatrolId(
			patrol.m_sZoneId,
			patrol.m_sFactionKey,
			patrol.m_iOwnershipRevision,
			patrol.m_iEpoch))
			return "schema66 local-security patrol identity is not deterministic";
		string operationId = BuildOperationId(patrol.m_sPatrolId);
		string projectionId = BuildProjectionId(operationId);
		if (patrol.m_sOperationId != operationId
			|| patrol.m_sManifestId != BuildManifestId(operationId)
			|| !IsBoundedId(patrol.m_sManifestHash)
			|| patrol.m_sSpawnResultId != BuildSpawnResultId(patrol.m_sPatrolId)
			|| patrol.m_sForceId != BuildForceId(operationId)
			|| patrol.m_sProjectionId != projectionId
			|| patrol.m_sGroupId != projectionId)
			return "schema66 local-security derived identity conflicts";
		int expectedInfantry = Math.Max(MIN_INFANTRY, Math.Min(MAX_INFANTRY, patrol.m_iPoliceStrength + 1));
		if (patrol.m_iOriginalInfantryCount != expectedInfantry
			|| patrol.m_iLivingInfantryCount < 0
			|| patrol.m_iLivingInfantryCount > patrol.m_iOriginalInfantryCount)
			return "schema66 local-security roster aggregate conflicts";
		int nowSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		if (patrol.m_iCreatedAtSecond < 0 || patrol.m_iCreatedAtSecond > nowSecond
			|| patrol.m_iLastChangedAtSecond < patrol.m_iCreatedAtSecond
			|| patrol.m_iLastChangedAtSecond > nowSecond)
			return "schema66 local-security timing authority is invalid";
		if (patrol.m_sStatus == STATUS_ACTIVE)
		{
			if (patrol.m_iLivingInfantryCount <= 0 || patrol.m_iTerminalAtSecond != 0
				|| !patrol.m_sTerminalReason.IsEmpty() || !patrol.m_sAuthorityFailure.IsEmpty())
				return "schema66 active local-security terminal evidence conflicts";
		}
		else if (patrol.m_iTerminalAtSecond < patrol.m_iCreatedAtSecond
			|| patrol.m_iTerminalAtSecond > nowSecond
			|| patrol.m_sTerminalReason.IsEmpty()
			|| patrol.m_sTerminalReason.Length() > MAX_REASON_CHARACTERS
			|| !patrol.m_sAuthorityFailure.IsEmpty())
			return "schema66 terminal local-security evidence is invalid";
		return "";
	}

	protected string ValidateOperation(
		HST_LocalSecurityPatrolState patrol,
		HST_OperationRecordState operation,
		bool terminal)
	{
		if (!operation || CountOperationsByAnyIdentity(patrol, operation) != 1)
			return "schema66 local-security operation identity is ambiguous";
		if (operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_LOCAL_SECURITY_PATROL
			|| operation.m_iContractVersion != CONTRACT_VERSION
			|| operation.m_iProjectionContractVersion != PROJECTION_CONTRACT_VERSION)
			return "schema66 local-security operation backlinks conflict";
		if (operation.m_sOperationId != patrol.m_sOperationId
			|| operation.m_sLocalSecurityPatrolId != patrol.m_sPatrolId
			|| operation.m_sOwnerFactionKey != patrol.m_sFactionKey
			|| operation.m_sManifestId != patrol.m_sManifestId)
			return "schema66 local-security operation backlinks conflict";
		if (operation.m_sOriginZoneId != patrol.m_sZoneId
			|| operation.m_sAssignmentZoneId != patrol.m_sZoneId
			|| operation.m_sTacticalTargetZoneId != patrol.m_sZoneId)
			return "schema66 local-security operation backlinks conflict";
		if (operation.m_iRevision <= 0
			|| operation.m_iCreatedAtSecond != patrol.m_iCreatedAtSecond
			|| operation.m_iLastVirtualFriendlyCount != patrol.m_iLivingInfantryCount
			|| IsZeroVector(operation.m_vStrategicPosition))
			return "schema66 local-security operation roster or timing conflicts";
		if (operation.m_sAssignmentKind != ASSIGNMENT_KIND
			|| operation.m_sRecallPolicyId != RECALL_POLICY_ID
			|| operation.m_sSettlementPolicyId != SETTLEMENT_POLICY_ID)
			return "schema66 local-security operation policy conflicts";
		if (!operation.m_sCurrentRouteId.IsEmpty()
			|| !operation.m_sRouteContractHash.IsEmpty()
			|| operation.m_iRouteVersion != 0
			|| operation.m_iRouteWaypointIndex != -1)
			return "schema66 local-security operation owns a persisted route";
		if (operation.m_iRouteLapCount != 0 || operation.m_iRouteLegSequence != 0
			|| operation.m_iRouteLoopStartedAtSecond != 0
			|| operation.m_iRouteLoopCompletedAtSecond != 0)
			return "schema66 local-security operation owns a persisted route";
		if (!IsZeroVector(operation.m_vRouteStartPosition)
			|| !IsZeroVector(operation.m_vRouteEndPosition)
			|| Math.AbsFloat(operation.m_fRouteTotalDistanceMeters) > 0.01
			|| Math.AbsFloat(operation.m_fRouteProgressMeters) > 0.01
			|| Math.AbsFloat(operation.m_fStrategicSpeedMetersPerSecond) > 0.01)
			return "schema66 local-security operation owns a persisted route";
		if (!terminal)
		{
			if (operation.m_sSpawnResultId != patrol.m_sSpawnResultId
				|| operation.m_sForceId != patrol.m_sForceId
				|| operation.m_sProjectionId != patrol.m_sProjectionId
				|| operation.m_sGroupId != patrol.m_sGroupId)
				return "schema66 local-security operation runtime backlinks conflict";
			if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
				|| !operation.m_sSettlementId.IsEmpty()
				|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
				|| operation.m_eResumeDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
				|| operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
				|| operation.m_iSettledAtSecond != 0
				|| !operation.m_sTerminalReason.IsEmpty())
				return "schema66 active local-security operation lifecycle conflicts";
			bool strategicPair = operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
				&& (operation.m_eMaterializationState
					== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
					|| operation.m_eMaterializationState
						== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING);
			bool livePair = operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
				&& (operation.m_eMaterializationState
					== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
					|| operation.m_eMaterializationState
						== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING);
			if (!strategicPair && !livePair)
				return "schema66 active local-security projection pair conflicts";
		}
		else if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			|| operation.m_eResumeDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			|| operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN
			|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| operation.m_iSettledAtSecond != patrol.m_iTerminalAtSecond
			|| operation.m_sTerminalReason != patrol.m_sTerminalReason
			|| operation.m_sSettlementId != HST_OperationService.BuildSettlementId(
				operation.m_sOperationId,
				SETTLEMENT_KIND))
			return "schema66 terminal local-security operation lifecycle conflicts";
		if (terminal
			&& operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED
			&& operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED
			&& operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED
			&& operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED)
			return "schema66 terminal local-security result is unsupported";
		return "";
	}

	protected string ValidateManifest(
		HST_LocalSecurityPatrolState patrol,
		HST_ForceManifestState manifest)
	{
		if (!manifest || CountManifestsByAnyIdentity(patrol, manifest) != 1)
			return "schema66 local-security manifest identity is ambiguous";
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		if (!manifest.m_bFrozen || manifest.m_sManifestId != patrol.m_sManifestId
			|| manifest.m_sOperationId != patrol.m_sOperationId
			|| manifest.m_sManifestHash != patrol.m_sManifestHash
			|| integrity.BuildManifestHash(manifest) != patrol.m_sManifestHash)
			return "schema66 local-security frozen manifest hash conflicts";
		if (manifest.m_sPolicyId != EXACT_POLICY_ID
			|| manifest.m_sForceKind != EXACT_FORCE_KIND
			|| manifest.m_sIntentId != EXACT_INTENT_ID
			|| manifest.m_sFactionRole != "enemy"
			|| manifest.m_sFactionKey != patrol.m_sFactionKey
			|| manifest.m_sSourceZoneId != patrol.m_sZoneId
			|| manifest.m_sTargetZoneId != patrol.m_sZoneId
			|| manifest.m_sCatalogVersion != HST_LocalSecurityCatalogService.CATALOG_VERSION
			|| manifest.m_iCreatedAtSecond != patrol.m_iCreatedAtSecond)
			return "schema66 local-security manifest policy conflicts";
		if (!manifest.m_aGroups || !manifest.m_aMembers
			|| !manifest.m_aVehicles || !manifest.m_aAssets)
			return "schema66 local-security manifest collections are missing";
		if (manifest.m_iRequestedMemberCount != patrol.m_iOriginalInfantryCount
			|| manifest.m_iAcceptedMemberCount != patrol.m_iOriginalInfantryCount
			|| manifest.m_aMembers.Count() != patrol.m_iOriginalInfantryCount
			|| manifest.m_iRequestedVehicleCount != 0 || manifest.m_iAcceptedVehicleCount != 0
			|| manifest.m_aVehicles.Count() != 0 || manifest.m_aAssets.Count() != 0
			|| manifest.m_iMoneyCost != 0 || manifest.m_iHRCost != 0
			|| manifest.m_iEquipmentCost != 0 || manifest.m_iAttackResourceCost != 0
			|| manifest.m_iSupportResourceCost != 0 || manifest.m_aGroups.Count() != 1)
			return "schema66 local-security manifest cost or shape conflicts";
		HST_ForceManifestGroupState root = manifest.m_aGroups[0];
		string expectedCatalogEntryId = string.Format(
			"local_security_%1_%2",
			patrol.m_sFactionKey,
			patrol.m_iOriginalInfantryCount);
		if (!root || root.m_sElementId != manifest.m_sManifestId + "_group_1"
			|| root.m_sCatalogEntryId != expectedCatalogEntryId
			|| root.m_sRole != HST_LocalSecurityCatalogService.ROLE_TOWN_POLICE
			|| root.m_sPrefab.IsEmpty() || root.m_sPrefab != manifest.m_sGroupPrefab
			|| root.m_iOrdinal != 0 || !root.m_bRequired
			|| root.m_iExpectedMemberCount != patrol.m_iOriginalInfantryCount)
			return "schema66 local-security manifest root conflicts";
		for (int memberIndex = 0; memberIndex < manifest.m_aMembers.Count(); memberIndex++)
		{
			HST_ForceManifestMemberState member = manifest.m_aMembers[memberIndex];
			if (!member || member.m_sSlotId != string.Format("%1_member_%2", manifest.m_sManifestId, memberIndex + 1)
				|| member.m_sCatalogSlotId != string.Format(
					"%1/town_police_%2", expectedCatalogEntryId, memberIndex + 1)
				|| member.m_sRole != HST_LocalSecurityCatalogService.ROLE_TOWN_POLICE
				|| member.m_sPrefab.IsEmpty()
				|| member.m_sGroupElementId != root.m_sElementId
				|| member.m_iOrdinal != memberIndex || !member.m_bRequired
				|| member.m_iMoneyCost != 0 || member.m_iHRCost != 0
				|| member.m_iEquipmentCost != 0
				|| CountManifestMembers(manifest, member.m_sSlotId) != 1)
				return "schema66 local-security ordered member roster conflicts";
		}
		return "";
	}

	protected string ValidateActiveRuntime(
		HST_LocalSecurityPatrolState patrol,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (CountBatchesByAnyIdentity(patrol, batch) != 1
			|| CountGroupsByAnyIdentity(patrol, group) != 1)
			return "schema66 local-security runtime identity is ambiguous";
		if (batch.m_sResultId != patrol.m_sSpawnResultId
			|| batch.m_sRequestId != patrol.m_sPatrolId
			|| batch.m_sOperationId != patrol.m_sOperationId
			|| batch.m_sManifestId != patrol.m_sManifestId
			|| batch.m_sManifestHash != patrol.m_sManifestHash
			|| batch.m_sForceId != patrol.m_sForceId
			|| batch.m_sProjectionId != patrol.m_sProjectionId)
			return "schema66 local-security batch backlinks conflict";
		if (group.m_sGroupId != patrol.m_sGroupId
			|| group.m_sLocalSecurityPatrolId != patrol.m_sPatrolId
			|| group.m_sOperationId != patrol.m_sOperationId
			|| group.m_sManifestId != patrol.m_sManifestId
			|| group.m_sSpawnResultId != patrol.m_sSpawnResultId
			|| group.m_sForceId != patrol.m_sForceId
			|| group.m_sProjectionId != patrol.m_sProjectionId
			|| group.m_sGroupId != group.m_sProjectionId)
			return "schema66 local-security group backlinks conflict";
		if (group.m_sZoneId != patrol.m_sZoneId
			|| group.m_sFactionKey != patrol.m_sFactionKey
			|| group.m_sPrefab != manifest.m_sGroupPrefab
			|| !group.m_sVehiclePrefab.IsEmpty()
			|| !group.m_sRouteId.IsEmpty())
			return "schema66 local-security group role conflicts";
		if (group.m_sSpawnFallbackMode != EXACT_GROUP_MODE
			&& !group.m_sSpawnFallbackMode.StartsWith(EXACT_GROUP_MODE + "_"))
			return "schema66 local-security group role conflicts";
		if (group.m_bQRF || group.m_iVehicleCount != 0
			|| group.m_iOriginalVehicleCount != 0
			|| group.m_iSurvivorVehicleCount != 0
			|| group.m_iCompositionVehicleCount != 0
			|| group.m_iCompositionArmedVehicleCount != 0)
			return "schema66 local-security group role conflicts";
		if (!group.m_sGarrisonZoneId.IsEmpty()
			|| !group.m_sMissionInstanceId.IsEmpty()
			|| !group.m_sSupportRequestId.IsEmpty()
			|| !group.m_sEnemyOrderId.IsEmpty()
			|| !group.m_sQRFInstanceId.IsEmpty())
			return "schema66 local-security group role conflicts";
		if (!group.m_sConvoyElementId.IsEmpty()
			|| !group.m_sMissionAssetId.IsEmpty())
			return "schema66 local-security group role conflicts";
		if (group.m_iOriginalInfantryCount != patrol.m_iOriginalInfantryCount
			|| group.m_iCompositionManpower != patrol.m_iOriginalInfantryCount
			|| group.m_iInfantryCount != patrol.m_iLivingInfantryCount
			|| group.m_iLastSeenAliveCount != patrol.m_iLivingInfantryCount
			|| group.m_iSurvivorInfantryCount != patrol.m_iLivingInfantryCount
			|| group.m_iDurableLivingInfantryCount != patrol.m_iLivingInfantryCount)
			return "schema66 local-security survivor projection conflicts";
		if (group.m_sCompositionRequestId != patrol.m_sPatrolId
			|| group.m_sCompositionIntentId != EXACT_INTENT_ID
			|| group.m_sCompositionTier != "exact")
			return "schema66 local-security group composition conflicts";
		bool virtualState = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		bool materializingState = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		bool physicalState = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		bool dematerializingState = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
			return "schema66 open local-security graph contains a terminal batch";
		if (virtualState && (!batch.m_bStrategicProjectionHeld
			|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			|| !batch.m_sNativeGroupId.IsEmpty() || group.m_bSpawnedEntity
			|| !group.m_sRuntimeEntityId.IsEmpty() || group.m_iSpawnedAgentCount != 0))
			return "schema66 virtual local-security graph contains process-local authority";
		if (materializingState && batch.m_bStrategicProjectionHeld)
			return "schema66 materializing local-security graph remains strategically held";
		if (physicalState && (batch.m_bStrategicProjectionHeld
			|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_iSuccessfulHandoffCount <= 0 || !group.m_bSpawnedEntity))
			return "schema66 physical local-security handoff evidence conflicts";
		if (dematerializingState && (batch.m_bStrategicProjectionHeld || !group.m_bSpawnedEntity))
			return "schema66 dematerializing local-security handoff evidence conflicts";
		if ((physicalState || dematerializingState) && IsZeroVector(group.m_vPosition))
			return "schema66 live local-security position authority is empty";
		if (!PositionsMatch(group.m_vPosition, operation.m_vStrategicPosition, 2.0))
			return "schema66 local-security group position conflicts with operation authority";
		string slotFailure = ValidateBatchSlotBijection(operation, manifest, batch);
		if (!slotFailure.IsEmpty())
			return slotFailure;
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		if (queue.CountStrategicLivingMemberSlots(batch) != patrol.m_iLivingInfantryCount)
			return "schema66 local-security durable survivor count conflicts";
		return "";
	}

	protected string ValidateBatchSlotBijection(
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		if (!batch || !batch.m_aSlotResults)
			return "schema66 local-security batch slot authority is missing";
		int expectedSlots = manifest.m_aMembers.Count() + 1;
		if (batch.m_iExpectedSlotCount != expectedSlots
			|| batch.m_aSlotResults.Count() != expectedSlots)
			return "schema66 local-security batch slot count conflicts";
		string rootSlotId = manifest.m_aGroups[0].m_sElementId;
		if (CountBatchSlots(batch, rootSlotId, HST_ForceSpawnQueueService.SLOT_KIND_GROUP) != 1)
			return "schema66 local-security root slot conflicts";
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member || CountBatchSlots(batch, member.m_sSlotId, HST_ForceSpawnQueueService.SLOT_KIND_MEMBER) != 1)
				return "schema66 local-security member slot bijection conflicts";
		}
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_sSlotId.IsEmpty() || slot.m_sProjectionId != batch.m_sProjectionId)
				return "schema66 local-security batch slot identity conflicts";
			bool rootSlot = slot.m_sSlotId == rootSlotId
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP;
			bool memberSlot = manifest.FindMemberSlot(slot.m_sSlotId) != null
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER;
			if (!rootSlot && !memberSlot)
				return "schema66 local-security batch contains a foreign slot";
			if (slot.m_bCasualtyConfirmed)
			{
				if (!memberSlot || slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
					|| !slot.m_bEverAlive || slot.m_iCasualtyAtSecond < batch.m_iCreatedAtSecond)
					return "schema66 local-security casualty tombstone conflicts";
			}
			else if (memberSlot
				&& (slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
					|| slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_FINAL
					|| slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CANCELLED)
				&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				return "schema66 open local-security batch contains an unproven terminal member";
		}
		return "";
	}

	protected string ValidateLossReceipt(HST_LocalSecurityPatrolState patrol)
	{
		if (!patrol.m_bLossEventApplied)
		{
			if (!patrol.m_sLossEventId.IsEmpty() || patrol.m_iLossEventAppliedAtSecond != 0)
				return "schema66 unapplied local-security loss carries receipt evidence";
			return "";
		}
		if (patrol.m_sStatus != STATUS_TERMINAL || patrol.m_iLivingInfantryCount != 0
			|| patrol.m_sLossEventId != BuildLossEventId(patrol.m_sPatrolId)
			|| patrol.m_iLossEventAppliedAtSecond < 0
			|| patrol.m_iLossEventAppliedAtSecond != patrol.m_iTerminalAtSecond
			|| patrol.m_iLossEventAppliedAtSecond > Math.Max(0, m_SaveData.m_iElapsedSeconds))
			return "schema66 local-security loss envelope conflicts";
		HST_TownInfluenceEventState match;
		int count;
		foreach (HST_TownInfluenceEventState eventState : m_SaveData.m_aTownInfluenceEvents)
		{
			if (!eventState || eventState.m_sEventId != patrol.m_sLossEventId)
				continue;
			match = eventState;
			count++;
		}
		if (count != 1 || !match || !IsExactLossEventFingerprint(match, patrol))
			return "schema66 local-security loss event fingerprint conflicts";
		return "";
	}

	protected bool IsExactLossEventFingerprint(
		HST_TownInfluenceEventState match,
		HST_LocalSecurityPatrolState patrol)
	{
		if (!match || !patrol || match.m_iContractVersion != 1 || !match.m_bApplied)
			return false;
		if (match.m_sZoneId != patrol.m_sZoneId || match.m_sKind != LOSS_EVENT_KIND
			|| match.m_sSourceId != patrol.m_sOperationId
			|| match.m_iCreatedAtSecond != patrol.m_iLossEventAppliedAtSecond)
			return false;
		if (match.m_iExpiresAtSecond != 0 || match.m_iPoliceDelta != -1
			|| match.m_iFIASupportDelta != 0 || match.m_iOccupierSupportDelta != 0
			|| match.m_iReputationDelta != 0 || match.m_iHeatDelta != 0)
			return false;
		if (match.m_iPopulationDelta != 0 || match.m_iRoadblockDelta != 0
			|| !match.m_sAggressionFactionKey.IsEmpty()
			|| match.m_iAggressionDelta != 0
			|| match.m_iAggressionBefore != 0 || match.m_iAggressionAfter != 0)
			return false;
		if (match.m_iRequestedFIABasisPointDelta != 0
			|| match.m_iRequestedOccupierBasisPointDelta != 0
			|| match.m_iRequestedInvaderBasisPointDelta != 0)
			return false;
		if (match.m_iEffectiveFIABasisPointDelta != 0
			|| match.m_iEffectiveOccupierBasisPointDelta != 0
			|| match.m_iEffectiveInvaderBasisPointDelta != 0
			|| match.m_bPopulationScaled
#ifdef ENABLE_DIAG
			|| match.m_bAbsoluteDebugSeed
#endif
			)
			return false;
		if (match.m_iFIABasisPointsBefore != match.m_iFIABasisPointsAfter
			|| match.m_iOccupierBasisPointsBefore != match.m_iOccupierBasisPointsAfter
			|| match.m_iInvaderBasisPointsBefore != match.m_iInvaderBasisPointsAfter)
			return false;
		return match.m_iInitialPopulationBefore == match.m_iInitialPopulationAfter
			&& match.m_iRemainingPopulationBefore == match.m_iRemainingPopulationAfter
			&& match.m_iDestroyedPopulationBefore == match.m_iDestroyedPopulationAfter;
	}

	protected void NormalizeActiveRuntime(
		HST_LocalSecurityPatrolState patrol,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		int restoreSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		if (operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& group && !IsZeroVector(group.m_vPosition))
			operation.m_vStrategicPosition = group.m_vPosition;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_iMaterializationStateEnteredAtSecond = restoreSecond;
		operation.m_iStrategicLastUpdateSecond = restoreSecond;
		operation.m_iLastProgressAtSecond = restoreSecond;
		operation.m_sLastProjectionReason = "restored exact local-security patrol as strategic authority";
		operation.m_iRevision++;
		batch.m_sNativeGroupId = "";
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING;
		batch.m_bStrategicProjectionHeld = true;
		batch.m_iStrategicHoldSinceSecond = restoreSecond;
		batch.m_iNextAttemptSecond = 0;
		batch.m_iUpdatedAtSecond = restoreSecond;
		batch.m_iCompletedAtSecond = 0;
		batch.m_iAttemptGeneration++;
		batch.m_iLifecycleRevision++;
		batch.m_iLastLifecycleSecond = restoreSecond;
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
			slot.m_iUpdatedAtSecond = restoreSecond;
			if (slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				|| !slot.m_bCasualtyConfirmed)
				slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED;
		}
		group.m_bSpawnedEntity = false;
		group.m_bSpawnAttempted = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
		group.m_sRuntimeStatus = "local_security_virtual";
		group.m_vPosition = operation.m_vStrategicPosition;
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		group.m_vTargetPosition = operation.m_vAssignmentPosition;
		group.m_iInfantryCount = patrol.m_iLivingInfantryCount;
		group.m_iLastSeenAliveCount = patrol.m_iLivingInfantryCount;
		group.m_iSurvivorInfantryCount = patrol.m_iLivingInfantryCount;
		group.m_iDurableLivingInfantryCount = patrol.m_iLivingInfantryCount;
	}

	protected void QuarantineAggregate(HST_LocalSecurityPatrolState patrol, string reason)
	{
		if (!patrol)
			return;
		if (reason.IsEmpty())
			reason = "schema66 local-security authority conflict";
		if (reason.Length() > MAX_REASON_CHARACTERS)
			reason = reason.Substring(0, MAX_REASON_CHARACTERS);
		patrol.m_iContractVersion = QUARANTINE_CONTRACT_VERSION;
		patrol.m_sStatus = STATUS_QUARANTINED;
		patrol.m_sAuthorityFailure = reason;
		patrol.m_iRevision = Math.Max(1, patrol.m_iRevision);
		foreach (HST_ZoneState zone : m_SaveData.m_aZones)
		{
			if (zone && zone.m_sLocalSecurityPatrolId == patrol.m_sPatrolId)
				zone.m_sLocalSecurityPatrolId = "";
		}
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (OperationClaimsPatrol(operation, patrol))
				HoldOperation(operation, reason);
		}
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (BatchClaimsPatrol(batch, patrol))
				HoldBatch(batch, reason);
		}
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (GroupClaimsPatrol(group, patrol))
				HoldGroup(group, reason);
		}
	}

	protected void NormalizeQuarantined(HST_LocalSecurityPatrolState patrol)
	{
		string reason = patrol.m_sAuthorityFailure;
		if (reason.IsEmpty())
			reason = "schema66 local-security patrol remained quarantined after restore";
		QuarantineAggregate(patrol, reason);
	}

	protected void HoldOrphanStrongClaimants()
	{
		foreach (HST_ZoneState zone : m_SaveData.m_aZones)
		{
			if (!zone || zone.m_sLocalSecurityPatrolId.IsEmpty())
				continue;
			HST_LocalSecurityPatrolState linkedPatrol = FindUniquePatrolById(
				zone.m_sLocalSecurityPatrolId);
			if (!linkedPatrol || linkedPatrol.m_sZoneId != zone.m_sZoneId
				|| linkedPatrol.m_iContractVersion != CONTRACT_VERSION
				|| linkedPatrol.m_sStatus == STATUS_QUARANTINED)
				zone.m_sLocalSecurityPatrolId = "";
		}
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!IsSchema66LocalSecurityOperationClaimant(m_SaveData, operation))
				continue;
			HST_LocalSecurityPatrolState patrol = FindUniquePatrolById(operation.m_sLocalSecurityPatrolId);
			if (patrol && patrol.m_sOperationId == operation.m_sOperationId)
				continue;
			HoldOperation(operation, "schema66 orphan local-security operation claimant");
		}
		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (!IsSchema66LocalSecurityManifestClaimant(m_SaveData, manifest))
				continue;
			HST_LocalSecurityPatrolState patrol = FindUniquePatrolByManifestId(
				manifest.m_sManifestId);
			if (patrol && patrol.m_sOperationId == manifest.m_sOperationId)
				continue;
			HoldManifestClaimants(
				manifest,
				"schema66 orphan local-security manifest claimant");
		}
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!IsSchema66LocalSecurityBatchClaimant(m_SaveData, batch))
				continue;
			HST_LocalSecurityPatrolState patrol = FindUniquePatrolById(batch.m_sRequestId);
			if (patrol && patrol.m_sSpawnResultId == batch.m_sResultId
				&& patrol.m_sStatus == STATUS_ACTIVE)
				continue;
			HoldBatch(batch, "schema66 orphan local-security batch claimant");
		}
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!IsSchema66LocalSecurityGroupClaimant(m_SaveData, group))
				continue;
			HST_LocalSecurityPatrolState patrol = FindUniquePatrolById(group.m_sLocalSecurityPatrolId);
			if (patrol && patrol.m_sGroupId == group.m_sGroupId
				&& patrol.m_sStatus == STATUS_ACTIVE)
				continue;
			HoldGroup(group, "schema66 orphan local-security group claimant");
		}
	}

	protected void HoldOperation(HST_OperationRecordState operation, string reason)
	{
		if (!operation)
			return;
		bool changed = operation.m_iContractVersion != QUARANTINE_CONTRACT_VERSION
			|| operation.m_eMaterializationState
				!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| operation.m_ePositionAuthority
				!= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			|| operation.m_sLastProjectionReason != reason;
		operation.m_iContractVersion = QUARANTINE_CONTRACT_VERSION;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_sLastProjectionReason = reason;
		if (operation.m_iRevision <= 0)
			operation.m_iRevision = 1;
		else if (changed && operation.m_iRevision < MAX_MUTABLE_REVISION)
			operation.m_iRevision++;
	}

	protected void HoldBatch(HST_ForceSpawnResultState batch, string reason)
	{
		if (!batch)
			return;
		batch.m_sNativeGroupId = "";
		batch.m_bStrategicProjectionHeld = true;
		batch.m_bCancelRequested = true;
		batch.m_sLastFailureReason = reason;
		if (!batch.m_aSlotResults)
			return;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot)
				continue;
			slot.m_sEntityId = "";
			slot.m_sAssignedVehicleEntityId = "";
			slot.m_sNativeGroupId = "";
			slot.m_bAliveVerified = false;
			slot.m_bFactionVerified = false;
			slot.m_bGroupVerified = false;
			slot.m_bGameMasterVerified = false;
			slot.m_bProjectionVerified = false;
			slot.m_bSeatVerified = false;
		}
	}

	protected void HoldManifestClaimants(HST_ForceManifestState manifest, string reason)
	{
		if (!manifest)
			return;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (operation
				&& ((!manifest.m_sManifestId.IsEmpty()
					&& operation.m_sManifestId == manifest.m_sManifestId)
					|| (!manifest.m_sOperationId.IsEmpty()
						&& operation.m_sOperationId == manifest.m_sOperationId)))
				HoldOperation(operation, reason);
		}
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (batch
				&& ((!manifest.m_sManifestId.IsEmpty()
					&& batch.m_sManifestId == manifest.m_sManifestId)
					|| (!manifest.m_sOperationId.IsEmpty()
						&& batch.m_sOperationId == manifest.m_sOperationId)))
				HoldBatch(batch, reason);
		}
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (group
				&& ((!manifest.m_sManifestId.IsEmpty()
					&& group.m_sManifestId == manifest.m_sManifestId)
					|| (!manifest.m_sOperationId.IsEmpty()
						&& group.m_sOperationId == manifest.m_sOperationId)))
				HoldGroup(group, reason);
		}
	}

	protected void HoldGroup(HST_ActiveGroupState group, string reason)
	{
		if (!group)
			return;
		group.m_bSpawnedEntity = false;
		group.m_bSpawnAttempted = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
		group.m_sRuntimeStatus = "exact_local_security_quarantined";
		group.m_sSpawnFallbackMode = EXACT_GROUP_MODE + "_quarantined";
		group.m_sSpawnFailureReason = reason;
	}

	protected bool OperationClaimsPatrol(
		HST_OperationRecordState operation,
		HST_LocalSecurityPatrolState patrol)
	{
		if (!operation || !patrol)
			return false;
		if (!patrol.m_sPatrolId.IsEmpty()
			&& operation.m_sLocalSecurityPatrolId == patrol.m_sPatrolId)
			return true;
		if (!patrol.m_sOperationId.IsEmpty()
			&& operation.m_sOperationId == patrol.m_sOperationId)
			return true;
		if (!patrol.m_sManifestId.IsEmpty()
			&& operation.m_sManifestId == patrol.m_sManifestId)
			return true;
		if (!patrol.m_sSpawnResultId.IsEmpty()
			&& operation.m_sSpawnResultId == patrol.m_sSpawnResultId)
			return true;
		if (!patrol.m_sForceId.IsEmpty()
			&& operation.m_sForceId == patrol.m_sForceId)
			return true;
		if (!patrol.m_sProjectionId.IsEmpty()
			&& operation.m_sProjectionId == patrol.m_sProjectionId)
			return true;
		return !patrol.m_sGroupId.IsEmpty()
			&& operation.m_sGroupId == patrol.m_sGroupId;
	}

	protected bool BatchClaimsPatrol(
		HST_ForceSpawnResultState batch,
		HST_LocalSecurityPatrolState patrol)
	{
		if (!batch || !patrol)
			return false;
		if (!patrol.m_sPatrolId.IsEmpty()
			&& batch.m_sRequestId == patrol.m_sPatrolId)
			return true;
		if (!patrol.m_sSpawnResultId.IsEmpty()
			&& batch.m_sResultId == patrol.m_sSpawnResultId)
			return true;
		if (!patrol.m_sOperationId.IsEmpty()
			&& batch.m_sOperationId == patrol.m_sOperationId)
			return true;
		if (!patrol.m_sManifestId.IsEmpty()
			&& batch.m_sManifestId == patrol.m_sManifestId)
			return true;
		if (!patrol.m_sForceId.IsEmpty()
			&& batch.m_sForceId == patrol.m_sForceId)
			return true;
		return !patrol.m_sProjectionId.IsEmpty()
			&& batch.m_sProjectionId == patrol.m_sProjectionId;
	}

	protected bool GroupClaimsPatrol(
		HST_ActiveGroupState group,
		HST_LocalSecurityPatrolState patrol)
	{
		if (!group || !patrol)
			return false;
		if (!patrol.m_sPatrolId.IsEmpty()
			&& group.m_sLocalSecurityPatrolId == patrol.m_sPatrolId)
			return true;
		if (!patrol.m_sGroupId.IsEmpty()
			&& group.m_sGroupId == patrol.m_sGroupId)
			return true;
		if (!patrol.m_sOperationId.IsEmpty()
			&& group.m_sOperationId == patrol.m_sOperationId)
			return true;
		if (!patrol.m_sManifestId.IsEmpty()
			&& group.m_sManifestId == patrol.m_sManifestId)
			return true;
		if (!patrol.m_sSpawnResultId.IsEmpty()
			&& group.m_sSpawnResultId == patrol.m_sSpawnResultId)
			return true;
		if (!patrol.m_sForceId.IsEmpty()
			&& group.m_sForceId == patrol.m_sForceId)
			return true;
		return !patrol.m_sProjectionId.IsEmpty()
			&& group.m_sProjectionId == patrol.m_sProjectionId;
	}

	protected HST_ZoneState FindUniqueTown(string zoneId)
	{
		HST_ZoneState match;
		if (zoneId.IsEmpty()
			|| HST_MaidensBayLocationSaveValidationService.ResolveCanonicalZoneId(zoneId) != zoneId)
			return null;
		foreach (HST_ZoneState zone : m_SaveData.m_aZones)
		{
			if (!zone || zone.m_sZoneId != zoneId)
				continue;
			if (zone.m_eType != HST_EZoneType.HST_ZONE_TOWN || match)
				return null;
			match = zone;
		}
		return match;
	}

	protected HST_LocalSecurityPatrolState FindUniquePatrolById(string patrolId)
	{
		HST_LocalSecurityPatrolState match;
		if (patrolId.IsEmpty())
			return null;
		foreach (HST_LocalSecurityPatrolState patrol : m_SaveData.m_aLocalSecurityPatrols)
		{
			if (!patrol || patrol.m_sPatrolId != patrolId)
				continue;
			if (match)
				return null;
			match = patrol;
		}
		return match;
	}

	protected HST_LocalSecurityPatrolState FindUniquePatrolByManifestId(string manifestId)
	{
		HST_LocalSecurityPatrolState match;
		if (manifestId.IsEmpty())
			return null;
		foreach (HST_LocalSecurityPatrolState patrol : m_SaveData.m_aLocalSecurityPatrols)
		{
			if (!patrol || patrol.m_sManifestId != manifestId)
				continue;
			if (match)
				return null;
			match = patrol;
		}
		return match;
	}

	protected HST_OperationRecordState FindUniqueOperation(string operationId)
	{
		HST_OperationRecordState match;
		if (operationId.IsEmpty())
			return null;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!operation || operation.m_sOperationId != operationId)
				continue;
			if (match)
				return null;
			match = operation;
		}
		return match;
	}

	protected HST_ForceManifestState FindUniqueManifest(string manifestId)
	{
		HST_ForceManifestState match;
		if (manifestId.IsEmpty())
			return null;
		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (!manifest || manifest.m_sManifestId != manifestId)
				continue;
			if (match)
				return null;
			match = manifest;
		}
		return match;
	}

	protected HST_ForceSpawnResultState FindUniqueBatch(string resultId)
	{
		HST_ForceSpawnResultState match;
		if (resultId.IsEmpty())
			return null;
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!batch || batch.m_sResultId != resultId)
				continue;
			if (match)
				return null;
			match = batch;
		}
		return match;
	}

	protected HST_ActiveGroupState FindUniqueGroup(string groupId)
	{
		HST_ActiveGroupState match;
		if (groupId.IsEmpty())
			return null;
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!group || group.m_sGroupId != groupId)
				continue;
			if (match)
				return null;
			match = group;
		}
		return match;
	}

	protected int CountPatrolsByZone(string zoneId)
	{
		int count;
		foreach (HST_LocalSecurityPatrolState patrol : m_SaveData.m_aLocalSecurityPatrols)
		{
			if (patrol && patrol.m_sZoneId == zoneId)
				count++;
		}
		return count;
	}

	protected int CountPatrolsById(string patrolId)
	{
		int count;
		foreach (HST_LocalSecurityPatrolState patrol : m_SaveData.m_aLocalSecurityPatrols)
		{
			if (patrol && patrol.m_sPatrolId == patrolId)
				count++;
		}
		return count;
	}

	protected int CountOperationsByAnyIdentity(
		HST_LocalSecurityPatrolState patrol,
		HST_OperationRecordState expected)
	{
		int count;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!operation)
				continue;
			if (operation.m_sOperationId == expected.m_sOperationId
				|| operation.m_sLocalSecurityPatrolId == patrol.m_sPatrolId
				|| operation.m_sManifestId == patrol.m_sManifestId
				|| operation.m_sSpawnResultId == patrol.m_sSpawnResultId
				|| operation.m_sProjectionId == patrol.m_sProjectionId
				|| operation.m_sGroupId == patrol.m_sGroupId)
				count++;
		}
		return count;
	}

	protected int CountManifestsByAnyIdentity(
		HST_LocalSecurityPatrolState patrol,
		HST_ForceManifestState expected)
	{
		int count;
		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (manifest && (manifest.m_sManifestId == expected.m_sManifestId
				|| manifest.m_sOperationId == patrol.m_sOperationId))
				count++;
		}
		return count;
	}

	protected int CountBatchesByAnyIdentity(
		HST_LocalSecurityPatrolState patrol,
		HST_ForceSpawnResultState expected)
	{
		int count;
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!batch)
				continue;
			if (batch.m_sResultId == expected.m_sResultId
				|| batch.m_sRequestId == patrol.m_sPatrolId
				|| batch.m_sOperationId == patrol.m_sOperationId
				|| batch.m_sManifestId == patrol.m_sManifestId
				|| batch.m_sForceId == patrol.m_sForceId
				|| batch.m_sProjectionId == patrol.m_sProjectionId)
				count++;
		}
		return count;
	}

	protected int CountBatchClaimants(HST_LocalSecurityPatrolState patrol)
	{
		int count;
		if (!patrol)
			return count;
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (BatchClaimsPatrol(batch, patrol))
				count++;
		}
		return count;
	}

	protected int CountGroupsByAnyIdentity(
		HST_LocalSecurityPatrolState patrol,
		HST_ActiveGroupState expected)
	{
		int count;
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!group)
				continue;
			if (group.m_sGroupId == expected.m_sGroupId
				|| group.m_sLocalSecurityPatrolId == patrol.m_sPatrolId
				|| group.m_sOperationId == patrol.m_sOperationId
				|| group.m_sManifestId == patrol.m_sManifestId
				|| group.m_sSpawnResultId == patrol.m_sSpawnResultId
				|| group.m_sForceId == patrol.m_sForceId
				|| group.m_sProjectionId == patrol.m_sProjectionId)
				count++;
		}
		return count;
	}

	protected int CountGroupClaimants(HST_LocalSecurityPatrolState patrol)
	{
		int count;
		if (!patrol)
			return count;
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (GroupClaimsPatrol(group, patrol))
				count++;
		}
		return count;
	}

	protected int CountManifestMembers(HST_ForceManifestState manifest, string slotId)
	{
		int count;
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (member && member.m_sSlotId == slotId)
				count++;
		}
		return count;
	}

	protected int CountBatchSlots(
		HST_ForceSpawnResultState batch,
		string slotId,
		string slotKind)
	{
		int count;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotId == slotId && slot.m_sSlotKind == slotKind)
				count++;
		}
		return count;
	}

	protected bool HasCampaignEvent(string eventId)
	{
		foreach (HST_CampaignEventState eventState : m_SaveData.m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				return true;
		}
		return false;
	}

	protected bool IsBoundedId(string value)
	{
		return !value.IsEmpty() && value == value.Trim()
			&& value.Length() <= MAX_ID_CHARACTERS;
	}

	protected bool IsZeroVector(vector value)
	{
		return Math.AbsFloat(value[0]) < 0.01
			&& Math.AbsFloat(value[1]) < 0.01
			&& Math.AbsFloat(value[2]) < 0.01;
	}

	protected bool PositionsMatch(vector left, vector right, float tolerance = 0.1)
	{
		float dx = left[0] - right[0];
		float dy = left[1] - right[1];
		float dz = left[2] - right[2];
		return dx * dx + dy * dy + dz * dz <= tolerance * tolerance;
	}

	protected static HST_OperationRecordState FindStaticOperation(
		HST_CampaignSaveData saveData,
		string operationId)
	{
		if (!saveData || !saveData.m_aOperations || operationId.IsEmpty())
			return null;
		HST_OperationRecordState match;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!operation || operation.m_sOperationId != operationId)
				continue;
			if (match)
				return null;
			match = operation;
		}
		return match;
	}
}
