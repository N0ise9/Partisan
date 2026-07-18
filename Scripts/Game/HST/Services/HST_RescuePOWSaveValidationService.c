// Schema-58 restore boundary for newly started rescue_pows exact authority.
// Historical rescue missions stay contract-zero.  A mission id, generic
// captive role, mission_group_* row, or legacy carrier string never creates
// an exact operation, roster, casualty, extraction, or reward receipt.
class HST_RescuePOWSaveValidationService
{
	static const int SCHEMA_VERSION = 58;
	static const string RUNTIME_PRIMITIVE = "rescue_extract";
	static const string CAPTIVE_KIND = "captive";
	static const string CAPTIVE_ROLE = "captive";
	static const string CAPTIVE_PREFAB = "{6985327711303730}Prefabs/Objects/HST/HST_MissionProp_Captives.et";
	static const string MIGRATION_EVENT_ID = "migration_schema58_exact_rescue_pows";
	static const string CONFLICT_EVENT_ID = "normalization_schema58_exact_rescue_pows_conflict";
	static const string QUARANTINE_PHASE = "rescue_pow_authority_quarantined";
	static const string QUARANTINE_EVENT_KEY = "schema58_rescue_pow_authority_quarantined";

	protected HST_CampaignSaveData m_SaveData;

	void Normalize(HST_CampaignSaveData saveData, int restoredSchemaVersion)
	{
		m_SaveData = saveData;
		if (!m_SaveData)
			return;

		if (restoredSchemaVersion < SCHEMA_VERSION)
		{
			PreservePreSchema58Authority();
			m_SaveData = null;
			return;
		}

		array<string> handledOperationIds = {};
		int validatedCount;
		int quarantinedCount;
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (!IsSchema58RescuePOWMissionClaimant(m_SaveData, mission))
				continue;

			HST_OperationRecordState operation = FindUniqueOperation(mission.m_sOperationId);
			HST_ForceManifestState manifest = FindUniqueManifest(mission.m_sManifestId);
			HST_ForceSpawnResultState batch = FindUniqueBatch(mission.m_sSpawnResultId);
			HST_ActiveGroupState group;
			if (operation)
				group = FindUniqueGroup(operation.m_sGroupId);

			string failure = ValidateAggregate(mission, operation, manifest, batch, group);
			if (!failure.IsEmpty())
			{
				QuarantineAggregate(mission, operation, manifest, batch, group, failure);
				quarantinedCount++;
			}
			else
			{
				NormalizeValidAggregate(mission, operation, batch, group);
				validatedCount++;
			}

			if (operation && !operation.m_sOperationId.IsEmpty()
				&& !handledOperationIds.Contains(operation.m_sOperationId))
				handledOperationIds.Insert(operation.m_sOperationId);
		}

		quarantinedCount += PreserveOrphanClaimants(handledOperationIds);
		RecordNormalizationConflict(validatedCount, quarantinedCount);
		m_SaveData = null;
	}

	// Runtime and proof code can use the same validator before capture without
	// mutating the aggregate.  Open authority requires the guard batch/group;
	// settled compact authority may retain either as terminal evidence or omit it.
	string ValidateCurrentAggregate(
		HST_CampaignSaveData saveData,
		HST_ActiveMissionState mission)
	{
		m_SaveData = saveData;
		if (!m_SaveData || !mission)
		{
			m_SaveData = null;
			return "exact rescue POW current authority is unavailable";
		}

		HST_OperationRecordState operation = FindUniqueOperation(mission.m_sOperationId);
		HST_ForceManifestState manifest = FindUniqueManifest(mission.m_sManifestId);
		HST_ForceSpawnResultState batch = FindUniqueBatch(mission.m_sSpawnResultId);
		HST_ActiveGroupState group;
		if (operation)
			group = FindUniqueGroup(operation.m_sGroupId);
		string failure = ValidateAggregate(mission, operation, manifest, batch, group);
		m_SaveData = null;
		return failure;
	}

	string ValidateCompactAggregate(
		HST_CampaignSaveData saveData,
		HST_ActiveMissionState mission)
	{
		m_SaveData = saveData;
		if (!m_SaveData || !mission)
		{
			m_SaveData = null;
			return "exact rescue POW compact authority is unavailable";
		}

		HST_OperationRecordState operation = FindUniqueOperation(mission.m_sOperationId);
		if (!operation || operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			m_SaveData = null;
			return "exact rescue POW compact authority is not settled";
		}
		HST_ForceManifestState manifest = FindUniqueManifest(mission.m_sManifestId);
		HST_ForceSpawnResultState batch = FindUniqueBatch(mission.m_sSpawnResultId);
		HST_ActiveGroupState group = FindUniqueGroup(operation.m_sGroupId);
		string failure = ValidateAggregate(mission, operation, manifest, batch, group);
		m_SaveData = null;
		return failure;
	}

	static bool IsSchema58RescuePOWMissionClaimant(
		HST_CampaignSaveData saveData,
		HST_ActiveMissionState mission)
	{
		if (!saveData || !mission || mission.m_sMissionId
			!= HST_RescuePOWOperationService.EXACT_MISSION_ID)
			return false;
		if (mission.m_iOperationContractVersion == HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION
			|| mission.m_iOperationContractVersion == HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION)
			return true;

		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!IsSchema58RescuePOWOperationClaimant(saveData, operation)
				|| operation.m_sOperationId.IsEmpty()
				|| operation.m_sMissionInstanceId.IsEmpty())
				continue;
			if (mission.m_sOperationId == operation.m_sOperationId
				&& mission.m_sInstanceId == operation.m_sMissionInstanceId
				&& ((!mission.m_sManifestId.IsEmpty() && mission.m_sManifestId == operation.m_sManifestId)
					|| (!mission.m_sSpawnResultId.IsEmpty() && mission.m_sSpawnResultId == operation.m_sSpawnResultId)))
				return true;
		}
		return false;
	}

	static bool HasSchema58RescuePOWMissionClaimant(HST_CampaignSaveData saveData)
	{
		if (!saveData)
			return false;
		foreach (HST_ActiveMissionState mission : saveData.m_aActiveMissions)
		{
			if (IsSchema58RescuePOWMissionClaimant(saveData, mission))
				return true;
		}
		return false;
	}

	static bool IsSchema58RescuePOWOperationClaimant(
		HST_CampaignSaveData saveData,
		HST_OperationRecordState operation)
	{
		return saveData && operation
			&& operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE;
	}

	static bool IsSchema58RescuePOWManifestClaimant(
		HST_CampaignSaveData saveData,
		HST_ForceManifestState manifest)
	{
		if (!saveData || !manifest)
			return false;
		if (manifest.m_sPolicyId == HST_RescuePOWOperationService.EXACT_POLICY_ID)
			return true;
		if (manifest.m_sForceKind == HST_RescuePOWOperationService.EXACT_FORCE_KIND
			&& manifest.m_sIntentId == HST_RescuePOWOperationService.EXACT_INTENT_ID)
			return true;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!IsSchema58RescuePOWOperationClaimant(saveData, operation)
				|| operation.m_sOperationId.IsEmpty())
				continue;
			if (manifest.m_sOperationId == operation.m_sOperationId
				&& !manifest.m_sManifestId.IsEmpty()
				&& manifest.m_sManifestId == operation.m_sManifestId)
				return true;
		}
		return false;
	}

	static bool IsSchema58RescuePOWBatchClaimant(
		HST_CampaignSaveData saveData,
		HST_ForceSpawnResultState batch)
	{
		if (!saveData || !batch)
			return false;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!IsSchema58RescuePOWOperationClaimant(saveData, operation)
				|| operation.m_sOperationId.IsEmpty()
				|| batch.m_sOperationId != operation.m_sOperationId)
				continue;
			if ((!batch.m_sResultId.IsEmpty() && batch.m_sResultId == operation.m_sSpawnResultId)
				|| (!batch.m_sManifestId.IsEmpty() && batch.m_sManifestId == operation.m_sManifestId)
				|| (!batch.m_sForceId.IsEmpty() && batch.m_sForceId == operation.m_sForceId)
				|| (!batch.m_sProjectionId.IsEmpty() && batch.m_sProjectionId == operation.m_sProjectionId))
				return true;
		}
		foreach (HST_ForceManifestState manifest : saveData.m_aForceManifests)
		{
			if (!IsSchema58RescuePOWManifestClaimant(saveData, manifest)
				|| manifest.m_sOperationId.IsEmpty()
				|| manifest.m_sManifestId.IsEmpty())
				continue;
			if (batch.m_bExternalAssetAuthority
				&& batch.m_sOperationId == manifest.m_sOperationId
				&& batch.m_sManifestId == manifest.m_sManifestId)
				return true;
		}
		return false;
	}

	static bool IsSchema58RescuePOWGroupClaimant(
		HST_CampaignSaveData saveData,
		HST_ActiveGroupState group)
	{
		if (!saveData || !group)
			return false;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!IsSchema58RescuePOWOperationClaimant(saveData, operation)
				|| operation.m_sOperationId.IsEmpty()
				|| group.m_sOperationId != operation.m_sOperationId)
				continue;
			if ((!group.m_sGroupId.IsEmpty() && group.m_sGroupId == operation.m_sGroupId)
				|| (!group.m_sManifestId.IsEmpty() && group.m_sManifestId == operation.m_sManifestId)
				|| (!group.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == operation.m_sSpawnResultId)
				|| (!group.m_sForceId.IsEmpty() && group.m_sForceId == operation.m_sForceId)
				|| (!group.m_sProjectionId.IsEmpty() && group.m_sProjectionId == operation.m_sProjectionId))
				return true;
		}
		bool exactMode = group.m_sSpawnFallbackMode == HST_RescuePOWOperationService.EXACT_GROUP_MODE
			|| group.m_sSpawnFallbackMode == HST_RescuePOWOperationService.QUARANTINE_STATUS;
		bool exactStatus = group.m_sRuntimeStatus.StartsWith("rescue_pow_guard_")
			|| group.m_sRuntimeStatus == HST_RescuePOWOperationService.QUARANTINE_STATUS;
		if (!exactMode && !exactStatus)
			return false;
		foreach (HST_ForceManifestState manifest : saveData.m_aForceManifests)
		{
			if (IsSchema58RescuePOWManifestClaimant(saveData, manifest)
				&& !manifest.m_sOperationId.IsEmpty()
				&& group.m_sOperationId == manifest.m_sOperationId
				&& group.m_sManifestId == manifest.m_sManifestId)
				return true;
		}
		return false;
	}

	static bool IsSchema58RescuePOWAssetClaimant(
		HST_CampaignSaveData saveData,
		HST_MissionAssetState asset)
	{
		if (!saveData || !asset)
			return false;
		if (asset.m_iRescueContractVersion == HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION
			|| asset.m_iRescueContractVersion == HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION)
			return true;
		if (asset.m_sOperationId.IsEmpty() || asset.m_sManifestId.IsEmpty()
			|| asset.m_sManifestSlotId.IsEmpty())
			return false;
		foreach (HST_ForceManifestState manifest : saveData.m_aForceManifests)
		{
			if (!IsSchema58RescuePOWManifestClaimant(saveData, manifest)
				|| manifest.m_sManifestId != asset.m_sManifestId
				|| manifest.m_sOperationId != asset.m_sOperationId)
				continue;
			foreach (HST_ForceManifestAssetState slot : manifest.m_aAssets)
			{
				if (slot && slot.m_sSlotId == asset.m_sManifestSlotId)
					return true;
			}
		}
		return false;
	}

	protected void PreservePreSchema58Authority()
	{
		int historicalMissionCount;
		int historicalGroupCount;
		int historicalAssetCount;
		int quarantinedCount;
		array<string> quarantinedOperationIds = {};

		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (!mission || mission.m_sMissionId != HST_RescuePOWOperationService.EXACT_MISSION_ID)
				continue;
			bool strongClaim = mission.m_iOperationContractVersion != 0
				|| HasTypedOperationForMission(mission);
			if (!strongClaim)
			{
				mission.m_iOperationContractVersion = 0;
				mission.m_bRescueExtractionGrace = false;
				mission.m_iRescueGraceUntilSecond = 0;
				historicalMissionCount++;
				foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
				{
					if (group && group.m_sGroupId == "mission_group_" + mission.m_sInstanceId)
						historicalGroupCount++;
				}
				foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
				{
					if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId
						&& asset.m_sRole == CAPTIVE_ROLE
						&& asset.m_iRescueContractVersion == 0)
						historicalAssetCount++;
				}
				continue;
			}

			HST_OperationRecordState operation = FindUniqueOperation(mission.m_sOperationId);
			HST_ForceManifestState manifest = FindUniqueManifest(mission.m_sManifestId);
			HST_ForceSpawnResultState batch = FindUniqueBatch(mission.m_sSpawnResultId);
			HST_ActiveGroupState group;
			if (operation)
				group = FindUniqueGroup(operation.m_sGroupId);
			QuarantineAggregate(
				mission,
				operation,
				manifest,
				batch,
				group,
				"pre-schema-58 save carried unsupported exact rescue POW authority");
			if (operation && !operation.m_sOperationId.IsEmpty())
				quarantinedOperationIds.Insert(operation.m_sOperationId);
			quarantinedCount++;
		}

		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!IsSchema58RescuePOWOperationClaimant(m_SaveData, operation)
				|| quarantinedOperationIds.Contains(operation.m_sOperationId))
				continue;
			HST_ActiveMissionState mission = FindUniqueMission(operation.m_sMissionInstanceId);
			QuarantineAggregate(
				mission,
				operation,
				FindUniqueManifest(operation.m_sManifestId),
				FindUniqueBatch(operation.m_sSpawnResultId),
				FindUniqueGroup(operation.m_sGroupId),
				"pre-schema-58 save carried orphan exact rescue POW operation authority");
			quarantinedOperationIds.Insert(operation.m_sOperationId);
			quarantinedCount++;
		}

		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!asset || asset.m_iRescueContractVersion == 0)
				continue;
			if (asset.m_iRescueContractVersion != HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION
				&& asset.m_iRescueContractVersion != HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION)
				continue;
			QuarantineAsset(asset, "pre-schema-58 save carried unsupported exact captive authority");
		}
		quarantinedCount += PreserveOrphanClaimants(quarantinedOperationIds);

		if (HasEvent(MIGRATION_EVENT_ID))
			return;
		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = MIGRATION_EVENT_ID;
		eventState.m_sCategory = "migration";
		eventState.m_sAggregateType = "mission_rescue_authority";
		eventState.m_sAggregateId = "schema58";
		eventState.m_sTransition = "legacy_rescue_pows_preserved";
		eventState.m_sReason = string.Format(
			"preserved %1 historical rescue_pows missions, %2 legacy mission_group rows, and %3 legacy captive rows on contract zero; quarantined %4 unsupported exact claimants and inferred no operation, manifest, guard roster, captive identity, death, extraction, carrier, projection, settlement, or reward authority",
			historicalMissionCount,
			historicalGroupCount,
			historicalAssetCount,
			quarantinedCount);
		eventState.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		m_SaveData.m_aCampaignEvents.Insert(eventState);
	}

	protected string ValidateAggregate(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!mission || !operation || !manifest)
			return "exact rescue POW restore authority is incomplete or ambiguous";
		if (mission.m_iOperationContractVersion == HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION
			|| operation.m_iContractVersion == HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION)
			return "exact rescue POW authority was already quarantined";

		string failure = ValidateIdentity(mission, operation, manifest, batch, group);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateAdmissionAnchor(mission);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateFrozenManifest(mission, operation, manifest, group);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateCaptiveAuthority(mission, operation, manifest);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateLifecycle(mission, operation, group);
		if (!failure.IsEmpty())
			return failure;
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return ValidateSettlement(mission, operation, batch, group);
		if (!batch || !group)
			return "open exact rescue POW guard authority is incomplete or ambiguous";
		failure = ValidateBatchSlotBijection(manifest, batch);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateRuntime(mission, operation, manifest, batch, group);
		if (!failure.IsEmpty())
			return failure;
		return ValidateSettlement(mission, operation, batch, group);
	}

	protected string ValidateIdentity(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!HST_RescuePOWOperationService.IsExactMission(mission)
			|| mission.m_sRuntimePrimitive != RUNTIME_PRIMITIVE
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
			|| operation.m_iContractVersion != HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION)
			return "exact rescue POW mission or operation contract conflicts";
		if (mission.m_sInstanceId.IsEmpty() || mission.m_sOperationId.IsEmpty()
			|| mission.m_sManifestId.IsEmpty() || mission.m_sSpawnResultId.IsEmpty()
			|| operation.m_sGroupId.IsEmpty() || operation.m_sForceId.IsEmpty()
			|| operation.m_sProjectionId.IsEmpty())
			return "exact rescue POW reciprocal identity is incomplete";
		if (operation.m_sOperationId != mission.m_sOperationId
			|| operation.m_sMissionInstanceId != mission.m_sInstanceId
			|| operation.m_sManifestId != mission.m_sManifestId
			|| operation.m_sSpawnResultId != mission.m_sSpawnResultId)
			return "exact rescue POW mission and operation backlinks conflict";
		if (!operation.m_sActorIdentityId.IsEmpty() || !operation.m_sIssueRequestId.IsEmpty()
			|| !operation.m_sConfirmationRequestId.IsEmpty()
			|| !operation.m_sSupportRequestId.IsEmpty() || !operation.m_sEnemyOrderId.IsEmpty()
			|| !operation.m_sQuoteId.IsEmpty())
			return "exact rescue POW operation contains foreign authority backlinks";
		if (operation.m_sOperationId != HST_RescuePOWOperationService.BuildOperationId(mission.m_sInstanceId)
			|| manifest.m_sManifestId != HST_RescuePOWOperationService.BuildManifestId(mission.m_sInstanceId)
			|| operation.m_sSpawnResultId != HST_RescuePOWOperationService.BuildSpawnResultId(mission.m_sInstanceId)
			|| operation.m_sForceId != HST_RescuePOWOperationService.BuildForceId(mission.m_sInstanceId)
			|| operation.m_sProjectionId != HST_RescuePOWOperationService.BuildProjectionId(mission.m_sInstanceId)
			|| operation.m_sGroupId != operation.m_sProjectionId)
			return "exact rescue POW deterministic identity conflicts";
		if (manifest.m_sManifestId != operation.m_sManifestId
			|| manifest.m_sOperationId != operation.m_sOperationId)
			return "exact rescue POW manifest backlinks conflict";
		if (batch)
		{
			if (batch.m_sResultId != operation.m_sSpawnResultId
				|| batch.m_sRequestId != HST_RescuePOWOperationService.BuildSpawnRequestId(mission.m_sInstanceId)
				|| batch.m_sOperationId != operation.m_sOperationId
				|| batch.m_sManifestId != operation.m_sManifestId
				|| batch.m_sForceId != operation.m_sForceId
				|| batch.m_sProjectionId != operation.m_sProjectionId
				|| batch.m_iPriority != HST_RescuePOWOperationService.EXACT_PRIORITY
				|| batch.m_iMaxRetries != HST_RescuePOWOperationService.EXACT_MAX_RETRIES
				|| !batch.m_bExternalAssetAuthority)
				return "exact rescue POW guard batch backlinks conflict";
			if (batch.m_sManifestHash.IsEmpty() || batch.m_sManifestHash != manifest.m_sManifestHash)
				return "exact rescue POW immutable manifest receipt conflicts";
		}
		if (group)
		{
			if (group.m_sGroupId != operation.m_sProjectionId
				|| group.m_sGroupId != operation.m_sGroupId
				|| group.m_sOperationId != operation.m_sOperationId
				|| group.m_sMissionInstanceId != operation.m_sMissionInstanceId
				|| group.m_sManifestId != operation.m_sManifestId
				|| group.m_sSpawnResultId != operation.m_sSpawnResultId
				|| group.m_sForceId != operation.m_sForceId
				|| group.m_sProjectionId != operation.m_sProjectionId)
				return "exact rescue POW guard group backlinks conflict";
		}
		if (CountMissionsByAnyIdentity(mission, operation) != 1
			|| CountOperationsByAnyIdentity(mission, operation) != 1
			|| CountManifestsByAnyIdentity(operation, manifest) != 1)
			return "exact rescue POW authority identity is ambiguous";
		if ((batch && CountBatchesByAnyIdentity(operation, batch) != 1)
			|| (!batch && CountBatchesClaimingOperation(operation) != 0)
			|| (group && CountGroupsByAnyIdentity(operation, group) != 1)
			|| (!group && CountGroupsClaimingOperation(operation) != 0))
			return "exact rescue POW runtime authority identity is ambiguous";
		if (mission.m_sTargetZoneId.IsEmpty() || IsZeroVector(mission.m_vTargetPosition)
			|| operation.m_sOwnerFactionKey.IsEmpty()
			|| operation.m_sAssignmentZoneId != mission.m_sTargetZoneId
			|| operation.m_sTacticalTargetZoneId != mission.m_sTargetZoneId
			|| (group && group.m_sZoneId != mission.m_sTargetZoneId)
			|| (group && group.m_sFactionKey != operation.m_sOwnerFactionKey))
			return "exact rescue POW owner or assignment conflicts";
		if (manifest.m_sFactionKey != operation.m_sOwnerFactionKey
			|| manifest.m_sTargetZoneId != operation.m_sAssignmentZoneId)
			return "exact rescue POW manifest owner or assignment conflicts";
		return "";
	}

	protected string ValidateAdmissionAnchor(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_sSiteId.IsEmpty() || mission.m_sTargetZoneId.IsEmpty()
			|| IsZeroVector(mission.m_vTargetPosition)
			|| IsZeroVector(mission.m_vRescueExtractionPosition))
			return "exact rescue POW admission anchor is incomplete";
		HST_GeneratedSiteState site;
		int siteCount;
		foreach (HST_GeneratedSiteState candidate : m_SaveData.m_aGeneratedSites)
		{
			if (!candidate || candidate.m_sSiteId != mission.m_sSiteId)
				continue;
			site = candidate;
			siteCount++;
		}
		if (siteCount != 1 || !site || !site.m_bValid
			|| site.m_sZoneId != mission.m_sTargetZoneId
			|| IsZeroVector(site.m_vPosition))
			return "exact rescue POW admission site is missing, ambiguous, invalid, or outside its target zone";
		if (!PositionsMatch(mission.m_vTargetPosition, site.m_vPosition, 1.0))
			return "exact rescue POW target position conflicts with its frozen generated site";
		if (!IsZeroVector(m_SaveData.m_vHQPosition)
			&& PositionsMatch(mission.m_vTargetPosition, m_SaveData.m_vHQPosition, 1.0))
			return "exact rescue POW target retained an HQ fallback instead of its admission site";
		return "";
	}

	protected string ValidateFrozenManifest(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ActiveGroupState group)
	{
		if (!mission || !operation || !manifest || !manifest.m_bFrozen
			|| manifest.m_sManifestHash.IsEmpty())
			return "exact rescue POW frozen manifest is incomplete";
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		if (manifest.m_sManifestHash != integrity.BuildManifestHash(manifest))
			return "exact rescue POW frozen manifest hash conflicts";
		if (manifest.m_sPolicyId != HST_RescuePOWOperationService.EXACT_POLICY_ID
			|| manifest.m_sForceKind != HST_RescuePOWOperationService.EXACT_FORCE_KIND
			|| manifest.m_sIntentId != HST_RescuePOWOperationService.EXACT_INTENT_ID
			|| manifest.m_sFactionRole != "enemy" || !manifest.m_sSourceZoneId.IsEmpty()
			|| !manifest.m_sQuoteId.IsEmpty() || !manifest.m_sCommandRequestId.IsEmpty()
			|| manifest.m_sCatalogVersion != HST_ForceCatalogService.CATALOG_VERSION)
			return "exact rescue POW manifest policy or force kind conflicts";
		if (manifest.m_iDeterministicSeed != operation.m_iDeterministicSeed)
			return "exact rescue POW manifest deterministic seed conflicts";
		if (manifest.m_iRequestedMemberCount < 1 || manifest.m_iRequestedMemberCount > 32
			|| manifest.m_iRequestedMemberCount != manifest.m_iAcceptedMemberCount
			|| manifest.m_iAcceptedMemberCount != manifest.m_aMembers.Count())
			return "exact rescue POW guard member roster shape conflicts";
		if (manifest.m_iRequestedVehicleCount != 0 || manifest.m_iAcceptedVehicleCount != 0
			|| manifest.m_aVehicles.Count() != 0)
			return "exact rescue POW manifest must contain no vehicle authority";
		if (manifest.m_iMoneyCost != 0 || manifest.m_iHRCost != 0
			|| manifest.m_iEquipmentCost != 0 || manifest.m_iAttackResourceCost != 0
			|| manifest.m_iSupportResourceCost != 0)
			return "exact rescue POW manifest must contain no resource cost authority";
		if (manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0]
			|| manifest.m_aAssets.Count() != HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT)
			return "exact rescue POW manifest requires one guard root and exactly three captive slots";

		HST_ForceManifestGroupState root = manifest.m_aGroups[0];
		if (root.m_sElementId != HST_RescuePOWOperationService.BuildGroupRootId(mission.m_sInstanceId)
			|| root.m_sCatalogEntryId.IsEmpty() || root.m_sPrefab.IsEmpty()
			|| root.m_sRole.IsEmpty() || root.m_iOrdinal != 0 || !root.m_bRequired
			|| root.m_iExpectedMemberCount != manifest.m_iAcceptedMemberCount
			|| manifest.m_sGroupPrefab != root.m_sPrefab
			|| (group && group.m_sPrefab != root.m_sPrefab))
			return "exact rescue POW one-root guard execution contract conflicts";
		for (int memberIndex = 0; memberIndex < manifest.m_aMembers.Count(); memberIndex++)
		{
			HST_ForceManifestMemberState member = manifest.m_aMembers[memberIndex];
			if (!member || member.m_sSlotId.IsEmpty() || member.m_sCatalogSlotId.IsEmpty()
				|| member.m_sSlotId != HST_RescuePOWOperationService.BuildMemberSlotId(mission.m_sInstanceId, memberIndex)
				|| member.m_sGroupElementId != root.m_sElementId
				|| member.m_sPrefab.IsEmpty() || member.m_sRole.IsEmpty()
				|| member.m_iOrdinal != memberIndex
				|| CountManifestMembers(manifest, member.m_sSlotId) != 1)
				return "exact rescue POW ordered guard member roster conflicts";
			if (member.m_iMoneyCost != 0 || member.m_iHRCost != 0
				|| member.m_iEquipmentCost != 0 || !member.m_sAssignedVehicleSlotId.IsEmpty()
				|| !member.m_sSeatRole.IsEmpty() || member.m_iSeatIndex != -1)
				return "exact rescue POW guard member cost or vehicle assignment conflicts";
		}

		for (int ordinal = 0; ordinal < HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT; ordinal++)
		{
			string slotId = HST_RescuePOWOperationService.BuildCaptiveSlotId(mission.m_sInstanceId, ordinal);
			HST_ForceManifestAssetState assetSlot = FindUniqueManifestAsset(manifest, slotId);
			if (!assetSlot || CountManifestAssets(manifest, slotId) != 1
				|| assetSlot.m_sKind != CAPTIVE_KIND || assetSlot.m_sRole != CAPTIVE_ROLE
				|| assetSlot.m_sPrefab != CAPTIVE_PREFAB
				|| !assetSlot.m_sAssignedVehicleSlotId.IsEmpty()
				|| assetSlot.m_iQuantity != 1 || assetSlot.m_iOrdinal != ordinal
				|| !assetSlot.m_bRequired)
				return "exact rescue POW captive manifest slot conflicts";
		}
		return ValidateCurrentCatalogRoster(mission, manifest);
	}

	protected string ValidateCurrentCatalogRoster(
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest)
	{
		if (!mission || !manifest || manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
			return "exact rescue POW guard catalog root is missing";
		HST_ForceManifestGroupState root = manifest.m_aGroups[0];
		HST_ForceCatalogService catalog = new HST_ForceCatalogService();
		HST_ForceGroupCatalogEntry catalogGroup;
		int matches;
		foreach (HST_ForceGroupCatalogEntry candidate : catalog.BuildGroupCatalog(manifest.m_sFactionKey))
		{
			if (!candidate || candidate.m_sEntryId != root.m_sCatalogEntryId)
				continue;
			catalogGroup = candidate;
			matches++;
		}
		if (matches != 1 || !catalogGroup)
			return "exact rescue POW guard catalog execution root conflicts";
		int catalogMemberCount = catalogGroup.m_aMemberSlots.Count();
		if (root.m_sPrefab != catalogGroup.m_sExecutionPrefab
			|| manifest.m_sGroupPrefab != catalogGroup.m_sExecutionPrefab
			|| root.m_sRole != catalogGroup.m_sRole
			|| root.m_iExpectedMemberCount != catalogMemberCount
			|| manifest.m_iAcceptedMemberCount != catalogMemberCount
			|| manifest.m_aMembers.Count() != catalogMemberCount)
			return "exact rescue POW guard catalog execution root conflicts";
		for (int memberIndex = 0; memberIndex < catalogMemberCount; memberIndex++)
		{
			HST_ForceGroupCatalogSlot catalogSlot = catalogGroup.m_aMemberSlots[memberIndex];
			HST_ForceManifestMemberState member = manifest.m_aMembers[memberIndex];
			if (!catalogSlot || !member
				|| member.m_sCatalogSlotId != catalogGroup.m_sEntryId + "/" + catalogSlot.m_sSlotId
				|| member.m_sPrefab != catalogSlot.m_sPrefab
				|| member.m_sRole != catalogSlot.m_sRole
				|| member.m_bRequired != catalogSlot.m_bRequired)
				return "exact rescue POW ordered guard catalog roster conflicts";
		}
		return "";
	}

	protected string ValidateCaptiveAuthority(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		if (mission.m_iRequiredCaptiveCount != HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT)
			return "exact rescue POW required captive count conflicts";
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& (IsZeroVector(m_SaveData.m_vHQPosition)
				|| !PositionsMatch(mission.m_vRescueExtractionPosition,
					m_SaveData.m_vHQPosition, 1.0)))
			return "open exact rescue POW frozen extraction anchor no longer matches HQ";
		int extractedCount;
		int killedCount;
		int frozenCaptiveDeadlineSecond = -1;
		array<string> casualtyReceiptIds = {};
		array<string> extractionReceiptIds = {};
		for (int ordinal = 0; ordinal < HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT; ordinal++)
		{
			string slotId = HST_RescuePOWOperationService.BuildCaptiveSlotId(mission.m_sInstanceId, ordinal);
			string assetId = HST_RescuePOWOperationService.BuildCaptiveAssetId(mission.m_sInstanceId, ordinal);
			HST_ForceManifestAssetState manifestAsset = FindUniqueManifestAsset(manifest, slotId);
			HST_MissionAssetState asset = FindUniqueMissionAsset(assetId);
			if (!manifestAsset || !asset || CountMissionAssetsByAnyIdentity(mission, operation, slotId, assetId) != 1)
				return "exact rescue POW captive authority is missing or ambiguous";
			if (asset.m_sAssetId != assetId || asset.m_sMissionInstanceId != mission.m_sInstanceId
				|| asset.m_sOperationId != operation.m_sOperationId
				|| asset.m_sManifestId != manifest.m_sManifestId
				|| asset.m_sManifestSlotId != slotId
				|| asset.m_iRescueContractVersion != HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION
				|| asset.m_iRescueOrdinal != ordinal
				|| asset.m_sRescueProjectionId
					!= HST_RescuePOWOperationService.BuildCaptiveProjectionId(mission.m_sInstanceId, ordinal))
				return "exact rescue POW captive reciprocal identity conflicts";
			if (asset.m_sKind != manifestAsset.m_sKind || asset.m_sRole != manifestAsset.m_sRole
				|| asset.m_sPrefab != manifestAsset.m_sPrefab
				|| !asset.m_sAssignedVehicleSlotId.IsEmpty() || !asset.m_sConvoyElementId.IsEmpty())
				return "exact rescue POW captive manifest projection conflicts";
			vector expectedSource = ResolveExpectedCaptiveSourcePosition(mission.m_vTargetPosition, ordinal);
			if (IsZeroVector(asset.m_vSourcePosition) || IsZeroVector(asset.m_vCurrentPosition)
				|| IsZeroVector(asset.m_vLastKnownPosition) || IsZeroVector(asset.m_vTargetPosition)
				|| !PositionsMatch(asset.m_vSourcePosition, expectedSource, 1.0)
				|| !PositionsMatch(asset.m_vTargetPosition,
					mission.m_vRescueExtractionPosition, 1.0)
				|| asset.m_iDeadlineSecond <= 0
				|| asset.m_iInteractionRadiusMeters != 5)
				return "exact rescue POW captive admission or extraction anchor conflicts";
			if (frozenCaptiveDeadlineSecond < 0)
				frozenCaptiveDeadlineSecond = asset.m_iDeadlineSecond;
			else if (asset.m_iDeadlineSecond != frozenCaptiveDeadlineSecond)
				return "exact rescue POW captive frozen deadline identities conflict";
			string failure = ValidateCaptiveState(asset);
			if (!failure.IsEmpty())
				return string.Format("exact rescue POW captive %1 conflicts: %2", ordinal, failure);
			if (asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED)
			{
				if (asset.m_sRescueCasualtyReceiptId
					!= HST_RescuePOWOperationService.BuildCaptiveCasualtyReceiptId(
						mission.m_sInstanceId, asset.m_sAssetId))
					return "exact rescue POW captive casualty receipt conflicts";
				if (casualtyReceiptIds.Contains(asset.m_sRescueCasualtyReceiptId))
					return "exact rescue POW captive casualty receipt identity is ambiguous";
				casualtyReceiptIds.Insert(asset.m_sRescueCasualtyReceiptId);
				killedCount++;
			}
			else if (asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED)
			{
				if (asset.m_sRescueExtractionReceiptId
					!= HST_RescuePOWOperationService.BuildCaptiveExtractionReceiptId(
						mission.m_sInstanceId, asset.m_sAssetId)
					|| !PositionsMatch(asset.m_vCurrentPosition, asset.m_vTargetPosition,
						Math.Max(5.0, asset.m_iInteractionRadiusMeters)))
					return "exact rescue POW captive extraction receipt conflicts";
				if (extractionReceiptIds.Contains(asset.m_sRescueExtractionReceiptId))
					return "exact rescue POW captive extraction receipt identity is ambiguous";
				extractionReceiptIds.Insert(asset.m_sRescueExtractionReceiptId);
				extractedCount++;
			}
		}

		if (CountMissionCaptiveClaimants(mission.m_sInstanceId) != HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT)
			return "exact rescue POW mission contains foreign or duplicate typed captive authority";
		if (CountMissionOwnedAssets(mission, operation, manifest)
			!= HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT)
			return "exact rescue POW mission contains broad or foreign mission-asset authority";
		if (mission.m_iExtractedCaptiveCount != extractedCount
			|| mission.m_iRuntimeDeliveryCount != extractedCount
			|| mission.m_iRuntimeDestroyedCount != killedCount)
			return "exact rescue POW mission counters conflict with captive receipts";
		if (mission.m_bRescueExtractionGrace)
		{
			int durableGraceRemaining = mission.m_iRescueGraceUntilSecond
				- m_SaveData.m_iElapsedSeconds;
			if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE
				|| operation.m_eSettlementState
					!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				|| mission.m_sRuntimePhase != HST_RescuePOWOperationService.RESCUE_GRACE_PHASE
				|| mission.m_iRescueGraceUntilSecond <= 0
				|| mission.m_iRescueGraceUntilSecond != mission.m_iActiveUntilSecond
				|| mission.m_iRescueGraceUntilSecond <= m_SaveData.m_iElapsedSeconds
				|| mission.m_iRescueGraceUntilSecond
					!= frozenCaptiveDeadlineSecond + HST_RescuePOWOperationService.EXTRACTION_GRACE_SECONDS
				|| durableGraceRemaining <= 0
				|| durableGraceRemaining > HST_RescuePOWOperationService.EXTRACTION_GRACE_SECONDS
				|| mission.m_iRemainingSeconds != durableGraceRemaining)
				return "exact rescue POW extraction grace receipt conflicts";
			int custodyCount;
			foreach (HST_MissionAssetState custodyAsset : m_SaveData.m_aMissionAssets)
			{
				if (!custodyAsset || custodyAsset.m_sMissionInstanceId != mission.m_sInstanceId
					|| custodyAsset.m_iRescueContractVersion
						!= HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION)
					continue;
				bool inCustody = custodyAsset.m_eRescueDisposition
					== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
					|| custodyAsset.m_eRescueDisposition
						== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
					|| custodyAsset.m_eRescueDisposition
						== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
				if (inCustody && HasOneConnectedEscortIdentity(
					custodyAsset.m_sRescueEscortIdentityId))
					custodyCount++;
			}
			if (custodyCount + extractedCount
					!= HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT
				|| killedCount != 0)
				return "exact rescue POW extraction grace lost a living escort-bound or extracted captive";
		}
		else if (mission.m_iRescueGraceUntilSecond != 0
			|| mission.m_sRuntimePhase == HST_RescuePOWOperationService.RESCUE_GRACE_PHASE)
			return "exact rescue POW inactive extraction grace retains phase or deadline authority";
		else if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& (mission.m_iActiveUntilSecond != frozenCaptiveDeadlineSecond
				|| mission.m_iRemainingSeconds != Math.Max(0,
					frozenCaptiveDeadlineSecond - m_SaveData.m_iElapsedSeconds)))
			return "open exact rescue POW mission deadline drifted from its frozen captive deadline";
		else if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& mission.m_iActiveUntilSecond != frozenCaptiveDeadlineSecond
			&& mission.m_iActiveUntilSecond
				!= frozenCaptiveDeadlineSecond + HST_RescuePOWOperationService.EXTRACTION_GRACE_SECONDS)
			return "settled exact rescue POW mission deadline is neither the base nor one grace window";
		return "";
	}

	protected string ValidateCaptiveState(HST_MissionAssetState asset)
	{
		if (!asset)
			return "state is missing";
		if (asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_UNKNOWN
			|| asset.m_iRescueOrdinal < 0 || asset.m_iRescueRevision < 1
			|| asset.m_iRescueTransitionSecond < 0 || asset.m_iRescueProjectionGeneration < 0)
			return "typed disposition, ordinal, timestamp, revision, or generation is invalid";
		if (asset.m_sRescueLastCommandResult.IsEmpty()
			!= asset.m_sRescueLastCommandRequestId.IsEmpty())
			return "command request and result receipt pairing conflicts";
		if (!asset.m_aRescueCommandReceipts
			|| asset.m_aRescueCommandReceipts.Count()
				> HST_RescuePOWOperationService.MAX_CAPTIVE_COMMAND_RECEIPTS)
			return "durable command receipt ledger cardinality conflicts";
		array<string> commandRequestIds = {};
		int previousRecordedRevision;
		int rejectedCommandReceiptCount;
		foreach (HST_RescueCommandReceiptState commandReceipt : asset.m_aRescueCommandReceipts)
		{
			if (!commandReceipt || commandReceipt.m_sRequestId.IsEmpty()
				|| commandReceipt.m_sActorIdentityId.IsEmpty()
				|| CountPlayerIdentity(commandReceipt.m_sActorIdentityId) != 1
				|| commandReceipt.m_sCommand.IsEmpty()
				|| commandReceipt.m_sResult.IsEmpty()
				|| commandRequestIds.Contains(commandReceipt.m_sRequestId)
				|| CountExactCommandReceiptClaimants(commandReceipt.m_sRequestId) != 1
				|| !IsSupportedCaptiveCommandReceipt(
					commandReceipt.m_sCommand, commandReceipt.m_sResult)
				|| commandReceipt.m_iRecordedRevision < 1
				|| commandReceipt.m_iRecordedRevision > asset.m_iRescueRevision
				|| commandReceipt.m_iRecordedRevision <= previousRecordedRevision)
				return "durable command receipt ledger identity or result conflicts";
			if (asset.m_sRescueLastCommandRequestId == commandReceipt.m_sRequestId
				&& asset.m_sRescueLastCommandResult != commandReceipt.m_sResult)
				return "last command receipt conflicts with its durable accepted row";
			commandRequestIds.Insert(commandReceipt.m_sRequestId);
			previousRecordedRevision = commandReceipt.m_iRecordedRevision;
			if (commandReceipt.m_sResult.StartsWith("rejected:"))
				rejectedCommandReceiptCount++;
		}
		if (rejectedCommandReceiptCount
			> HST_RescuePOWOperationService.MAX_REJECTED_CAPTIVE_COMMAND_RECEIPTS)
			return "durable rejected command receipt retention bound conflicts";
		if (!asset.m_sRescueLastCommandResult.IsEmpty()
			&& !asset.m_sRescueLastCommandResult.StartsWith("accepted:")
			&& !asset.m_sRescueLastCommandResult.StartsWith("rejected:"))
			return "command result receipt is unsupported";
		if (asset.m_sRescueLastCommandResult == "rejected:")
			return "rejected command result receipt lacks a failure reason";
		if (asset.m_sRescueLastCommandResult.StartsWith("accepted:")
			&& asset.m_sRescueLastCommandResult != string.Format(
				"accepted:%1", asset.m_eRescueDisposition))
			return "accepted command receipt conflicts with the durable disposition";
		if (asset.m_bRescueDeathObserved && asset.m_bRescueExtractionObserved)
			return "death and extraction were both observed";

		bool killed = asset.m_eRescueDisposition
			== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED;
		bool extracted = asset.m_eRescueDisposition
			== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED;
		bool following = asset.m_eRescueDisposition
			== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING;
		bool boarding = asset.m_eRescueDisposition
			== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING;
		bool boarded = asset.m_eRescueDisposition
			== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
		if (killed)
		{
			if (!asset.m_bRescueDeathObserved
				|| asset.m_sRescueCasualtyReceiptId.IsEmpty()
				|| !asset.m_sRescueExtractionReceiptId.IsEmpty()
				|| !asset.m_bDestroyed || asset.m_bAlive || asset.m_bDelivered)
				return "killed disposition lacks one exact casualty receipt";
		}
		else if (extracted)
		{
			if (!asset.m_bRescueExtractionObserved || asset.m_bRescueDeathObserved
				|| !asset.m_sRescueCasualtyReceiptId.IsEmpty()
				|| asset.m_sRescueExtractionReceiptId.IsEmpty()
				|| asset.m_bDestroyed || !asset.m_bAlive || !asset.m_bDelivered)
				return "extracted disposition lacks one exact extraction receipt";
		}
		else
		{
			if (asset.m_bRescueDeathObserved || asset.m_bRescueExtractionObserved
				|| !asset.m_sRescueCasualtyReceiptId.IsEmpty()
				|| !asset.m_sRescueExtractionReceiptId.IsEmpty()
				|| asset.m_bDestroyed || !asset.m_bAlive || asset.m_bDelivered)
				return "nonterminal disposition contains terminal authority";
		}

		if ((following || boarding || boarded) && asset.m_sRescueEscortIdentityId.IsEmpty())
			return "escort-bound disposition lacks a stable identity";
		if ((asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD
			|| asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED)
			&& !asset.m_sRescueEscortIdentityId.IsEmpty())
			return "unclaimed disposition retains stable escort authority";
		if (extracted && asset.m_sRescueEscortIdentityId.IsEmpty())
			return "extracted disposition lacks its stable escort receipt";
		if (boarded)
		{
			if (asset.m_sRescueCarrierVehicleId.IsEmpty())
				return "vehicle-bound disposition lacks a stable carrier identity";
		}
		else if (!boarding && (!asset.m_sRescueCarrierVehicleId.IsEmpty()
			|| !asset.m_sRescueCarrierSeatToken.IsEmpty()))
			return "non-vehicle disposition retains stable carrier authority";
		if (boarded && asset.m_sRescueCarrierSeatToken.IsEmpty())
			return "boarded disposition lacks a stable seat token";
		if (!boarded && !asset.m_sRescueCarrierSeatToken.IsEmpty())
			return "non-boarded disposition retains a stable seat token";
		if (killed && (!asset.m_sRescueEscortIdentityId.IsEmpty()
			|| !asset.m_sRescueCarrierVehicleId.IsEmpty()
			|| !asset.m_sRescueCarrierSeatToken.IsEmpty()))
			return "killed disposition retains escort or carrier authority";
		if (extracted && (!asset.m_sRescueCarrierVehicleId.IsEmpty()
			|| !asset.m_sRescueCarrierSeatToken.IsEmpty()))
			return "extracted disposition retains carrier authority";
		if (!killed && !extracted && !asset.m_bPickedUp
			&& asset.m_eRescueDisposition != HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD)
			return "released disposition conflicts with legacy pickup receipt";
		if (asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD
			&& asset.m_bPickedUp)
			return "held disposition conflicts with legacy pickup receipt";
		return ValidateStableCustodyLinks(asset);
	}

	protected string ValidateStableCustodyLinks(HST_MissionAssetState asset)
	{
		if (!asset)
			return "stable custody state is missing";
		if (!asset.m_sRescueEscortIdentityId.IsEmpty())
		{
			int escortCount;
			foreach (HST_PlayerState player : m_SaveData.m_aPlayers)
			{
				if (player && player.m_sIdentityId == asset.m_sRescueEscortIdentityId)
					escortCount++;
			}
			if (escortCount != 1)
				return "stable escort identity is missing or ambiguous";
		}
		if (!asset.m_sRescueCarrierVehicleId.IsEmpty())
		{
			HST_RuntimeVehicleState carrier;
			int carrierCount;
			foreach (HST_RuntimeVehicleState vehicle : m_SaveData.m_aRuntimeVehicles)
			{
				if (!vehicle || vehicle.m_sVehicleRuntimeId != asset.m_sRescueCarrierVehicleId)
					continue;
				carrier = vehicle;
				carrierCount++;
			}
			if (carrierCount != 1 || !carrier || carrier.m_bDeleted
				|| carrier.m_sPrefab.IsEmpty() || carrier.m_sRuntimeKind != "mission_carrier")
				return "stable carrier identity is missing, ambiguous, deleted, or not restorable";
		}
		return "";
	}

	protected bool HasOneConnectedEscortIdentity(string identityId)
	{
		if (!m_SaveData || identityId.IsEmpty())
			return false;
		int count;
		bool connected;
		foreach (HST_PlayerState player : m_SaveData.m_aPlayers)
		{
			if (!player || player.m_sIdentityId != identityId)
				continue;
			count++;
			if (player.m_iLastSeenPlayerId > 0)
				connected = true;
		}
		return count == 1 && connected;
	}

	protected int CountPlayerIdentity(string identityId)
	{
		if (!m_SaveData || identityId.IsEmpty())
			return 0;
		int count;
		foreach (HST_PlayerState player : m_SaveData.m_aPlayers)
		{
			if (player && player.m_sIdentityId == identityId)
				count++;
		}
		return count;
	}

	protected bool IsSupportedCaptiveCommandReceipt(string command, string result)
	{
		if (!IsSupportedCaptiveCommand(command))
			return false;
		if (result.StartsWith("rejected:") && result != "rejected:")
			return true;
		string freed = string.Format("accepted:%1",
			HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED);
		string following = string.Format("accepted:%1",
			HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING);
		string boarding = string.Format("accepted:%1",
			HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING);
		string boarded = string.Format("accepted:%1",
			HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED);
		string extracted = string.Format("accepted:%1",
			HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED);
		if (command == "free")
			return result == freed;
		if (command == "follow" || command == "mission_captive_follow")
			return result == following;
		if (command == "board" || command == "mission_captive_board")
			return result == boarding || result == boarded;
		if (command == "extract")
			return result == extracted;
		if (command == "mission_captive_extract")
			return result == freed || result == extracted;
		return false;
	}

	protected bool IsSupportedCaptiveCommand(string command)
	{
		return command == "free" || command == "follow" || command == "board"
			|| command == "extract" || command == "mission_captive_extract"
			|| command == "mission_captive_follow" || command == "mission_captive_board";
	}

	protected int CountExactCommandReceiptClaimants(string requestId)
	{
		if (!m_SaveData || requestId.IsEmpty())
			return 0;
		int count;
		foreach (HST_MissionAssetState candidateAsset : m_SaveData.m_aMissionAssets)
		{
			if (!candidateAsset
				|| candidateAsset.m_iRescueContractVersion
					!= HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION
				|| !candidateAsset.m_aRescueCommandReceipts)
				continue;
			foreach (HST_RescueCommandReceiptState candidateReceipt : candidateAsset.m_aRescueCommandReceipts)
			{
				if (candidateReceipt && candidateReceipt.m_sRequestId == requestId)
					count++;
			}
		}
		return count;
	}

	protected string ValidateLifecycle(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group)
	{
		if (operation.m_iProjectionContractVersion != HST_RescuePOWOperationService.EXACT_PROJECTION_CONTRACT_VERSION
			|| operation.m_sAssignmentKind != HST_RescuePOWOperationService.ASSIGNMENT_KIND
			|| operation.m_sRecallPolicyId != HST_RescuePOWOperationService.RECALL_POLICY_ID
			|| operation.m_sSettlementPolicyId != HST_RescuePOWOperationService.SETTLEMENT_POLICY_ID)
			return "exact rescue POW projection or assignment policy conflicts";
		if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED)
			return "exact rescue POW stationary guard duty conflicts";
		bool settledDuty = operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		if ((!settledDuty && operation.m_eResumeDutyState
				!= HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
			|| (settledDuty && operation.m_eResumeDutyState
				!= HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED))
			return "exact rescue POW resume duty conflicts";
		if (operation.m_eEngagementMode
			== HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_UNKNOWN)
			return "exact rescue POW engagement authority is unknown";
		if (!operation.m_sCurrentRouteId.IsEmpty() || !operation.m_sRouteContractHash.IsEmpty()
			|| operation.m_iRouteVersion != 0 || operation.m_iRouteWaypointIndex != -1
			|| operation.m_iRouteLapCount != 0 || operation.m_iRouteLegSequence != 0
			|| operation.m_iRouteLoopStartedAtSecond != 0
			|| operation.m_iRouteLoopCompletedAtSecond != 0
			|| !IsZeroVector(operation.m_vRouteStartPosition)
			|| !IsZeroVector(operation.m_vRouteEndPosition)
			|| operation.m_fRouteTotalDistanceMeters != 0
			|| operation.m_fRouteProgressMeters != 0
			|| operation.m_fStrategicSpeedMetersPerSecond != 0
			|| (group && !group.m_sRouteId.IsEmpty()))
			return "exact rescue POW guard must remain stationary and route-free";
		if (IsZeroVector(operation.m_vAssignmentPosition)
			|| IsZeroVector(operation.m_vTacticalTargetPosition))
			return "exact rescue POW guard assignment position conflicts";
		if (operation.m_sOriginZoneId != operation.m_sAssignmentZoneId
			|| !PositionsMatch(operation.m_vOriginPosition, operation.m_vAssignmentPosition, 1.0))
			return "exact rescue POW immutable guard origin anchor conflicts";
		if (group && !PositionsMatch(
			group.m_vTargetPosition,
			operation.m_vAssignmentPosition,
			1.0))
			return "exact rescue POW guard group anchor semantics conflict";
		bool physicalAnchor = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
		if (group && physicalAnchor
			&& (IsZeroVector(group.m_vPosition)
				|| !PositionsMatch(group.m_vPosition, operation.m_vStrategicPosition, 1.0)))
			return "physical exact rescue POW guard live anchor conflicts";
		if (group && !physicalAnchor
			&& !PositionsMatch(group.m_vSourcePosition, operation.m_vStrategicPosition, 1.0))
			return "strategic exact rescue POW guard source anchor conflicts";
		if (group && operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& !PositionsMatch(group.m_vPosition, operation.m_vStrategicPosition, 1.0))
			return "virtual exact rescue POW guard position conflicts with its strategic anchor";
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return "";
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| !operation.m_sSettlementId.IsEmpty())
			return "open exact rescue POW authority contains terminal state";
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return "open exact rescue POW mission is not active";
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_UNKNOWN
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRING
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
			return "open exact rescue POW materialization state conflicts";
		bool liveState = operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
		if (liveState && operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
			return "physical exact rescue POW guard does not own live position authority";
		if (!liveState && operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return "virtual exact rescue POW guard does not own strategic position authority";
		return "";
	}

	protected string ValidateBatchSlotBijection(
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		if (!manifest || !batch || manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0]
			|| !batch.m_bExternalAssetAuthority)
			return "exact rescue POW guard batch or manifest is incomplete";
		int expectedSlotCount = 1 + manifest.m_aMembers.Count();
		if (batch.m_iExpectedSlotCount != expectedSlotCount
			|| batch.m_aSlotResults.Count() != expectedSlotCount)
			return "exact rescue POW guard batch slot cardinality conflicts";
		HST_ForceManifestGroupState root = manifest.m_aGroups[0];
		if (CountBatchSlots(batch, root.m_sElementId, HST_ForceSpawnQueueService.SLOT_KIND_GROUP) != 1)
			return "exact rescue POW guard root slot conflicts";
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member || CountBatchSlots(batch, member.m_sSlotId, HST_ForceSpawnQueueService.SLOT_KIND_MEMBER) != 1)
				return "exact rescue POW guard member slot bijection conflicts";
		}
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_sSlotId.IsEmpty()
				|| (slot.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_GROUP
					&& slot.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
				|| slot.m_sProjectionId != batch.m_sProjectionId)
				return "exact rescue POW guard batch contains a foreign or malformed slot";
			if (slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP
				&& slot.m_sSlotId != root.m_sElementId)
				return "exact rescue POW guard batch contains a foreign group root";
			if (slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				&& !manifest.FindMemberSlot(slot.m_sSlotId))
				return "exact rescue POW guard batch contains a foreign member slot";
			bool memberSlot = slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER;
			if (slot.m_bCasualtyConfirmed)
			{
				if (!memberSlot || slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
					|| !slot.m_bEverAlive || slot.m_iCasualtyAtSecond < batch.m_iCreatedAtSecond)
					return "exact rescue POW guard casualty tombstone conflicts";
				continue;
			}
			if (memberSlot && slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
				&& !slot.m_bEverAlive)
				return "registered exact rescue POW guard member lacks living evidence";
		}
		return "";
	}

	protected string ValidateRuntime(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (group.m_sSpawnFallbackMode != HST_RescuePOWOperationService.EXACT_GROUP_MODE
			|| group.m_sCompositionRequestId != manifest.m_sManifestId
			|| group.m_sCompositionIntentId != HST_RescuePOWOperationService.EXACT_INTENT_ID
			|| !group.m_sMissionAssetId.IsEmpty() || group.m_bQRF
			|| !group.m_sSupportRequestId.IsEmpty() || !group.m_sEnemyOrderId.IsEmpty()
			|| !group.m_sConvoyElementId.IsEmpty() || !group.m_sGarrisonZoneId.IsEmpty()
			|| !group.m_sQRFInstanceId.IsEmpty())
			return "exact rescue POW guard group ownership conflicts";
		if (group.m_iVehicleCount != 0 || group.m_iOriginalVehicleCount != 0
			|| group.m_iSurvivorVehicleCount != 0 || group.m_iCompositionVehicleCount != 0
			|| group.m_iCompositionArmedVehicleCount != 0 || !group.m_sVehiclePrefab.IsEmpty())
			return "exact rescue POW guard group must contain no vehicles";
		if (group.m_iCompositionCost != 0
			|| group.m_iCompositionManpower != manifest.m_iAcceptedMemberCount
			|| group.m_iOriginalInfantryCount != manifest.m_iAcceptedMemberCount)
			return "exact rescue POW guard frozen composition conflicts";

		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		int living;
		bool processLocal = operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
		if (processLocal)
		{
			if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
				|| batch.m_bStrategicProjectionHeld)
				return "physical exact rescue POW guard batch lifecycle conflicts";
			living = queue.CountDurableLivingMemberSlots(batch);
		}
		else
		{
			if (batch.m_bStrategicProjectionHeld)
				living = queue.CountStrategicLivingMemberSlots(batch);
			else
				living = queue.CountDurableLivingMemberSlots(batch);
			bool zeroRosterTerminal = living == 0
				&& (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED
					|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL);
			if (!zeroRosterTerminal
				&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
				&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
				return "virtual exact rescue POW guard batch lifecycle conflicts";
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
				&& !batch.m_bStrategicProjectionHeld)
				return "virtual exact rescue POW guard lacks strategic roster hold";
		}
		if (living < 0 || living > manifest.m_iAcceptedMemberCount)
			return "exact rescue POW guard survivor roster conflicts";
		if (group.m_iInfantryCount != living || group.m_iLastSeenAliveCount != living
			|| group.m_iSurvivorInfantryCount != living
			|| group.m_iDurableLivingInfantryCount != living
			|| operation.m_iLastVirtualFriendlyCount != living)
			return "exact rescue POW guard survivor counts conflict with durable slots";
		return "";
	}

	protected string ValidateSettlement(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			if (!mission.m_sSettlementId.IsEmpty())
				return "open exact rescue POW authority contains a mission settlement receipt";
			return "";
		}
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN)
			return "exact rescue POW settlement state conflicts";
		if (operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
			&& operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED
			&& operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED)
			return "exact rescue POW settlement uses an unsupported terminal result";
		string expectedSettlementId = HST_OperationService.BuildSettlementId(
			operation.m_sOperationId,
			HST_RescuePOWOperationService.SETTLEMENT_KIND);
		if (operation.m_sSettlementId.IsEmpty()
			|| operation.m_sSettlementId != expectedSettlementId
			|| mission.m_sSettlementId != operation.m_sSettlementId)
			return "exact rescue POW terminal receipt conflicts";
		if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return "settled exact rescue POW lifecycle residue conflicts";
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_AVAILABLE
			|| mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			|| mission.m_bRescueExtractionGrace || mission.m_iRescueGraceUntilSecond != 0)
			return "settled exact rescue POW authority retains an active mission or grace window";
		int killedCount = mission.m_iRuntimeDestroyedCount;
		int extractedCount = mission.m_iExtractedCaptiveCount;
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED)
		{
			if (operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
				|| extractedCount != HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT
				|| killedCount != 0)
				return "successful exact rescue POW settlement lacks three extraction receipts";
		}
		else if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED)
		{
			if (extractedCount >= HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT)
				return "failed exact rescue POW settlement already has complete extraction authority";
			if (killedCount > 0
				&& operation.m_eTerminalResult
					!= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED)
				return "failed exact rescue POW casualty settlement is not destroyed";
			if (killedCount == 0
				&& operation.m_eTerminalResult
					!= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED)
				return "failed exact rescue POW non-casualty settlement is not cancelled";
		}
		else if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED)
		{
			if (killedCount != 0
				|| extractedCount >= HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT
				|| operation.m_eTerminalResult
					!= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED)
				return "expired exact rescue POW settlement conflicts with cancellation authority";
		}
		else
			return "settled exact rescue POW mission status is unsupported";
		if (batch && batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED
			&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL)
			return "settled exact rescue POW guard batch is not terminal";
		if (group && GroupHasProcessResidue(group))
			return "settled exact rescue POW guard retains process-local authority";
		return "";
	}

	protected void NormalizeValidAggregate(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!mission || !operation)
			return;
		int restoreSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			NormalizeSettledProcessResidue(batch, group);
			NormalizeCaptiveProcessResidue(mission, restoreSecond);
			return;
		}
		if (!batch || !group)
			return;

		bool processLocal = operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		if (processLocal && !IsZeroVector(group.m_vPosition))
			operation.m_vStrategicPosition = group.m_vPosition;
		if (!processLocal
			&& operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& batch.m_bStrategicProjectionHeld
			&& !BatchHasProcessResidue(batch) && !GroupHasProcessResidue(group)
			&& !MissionCaptivesHaveProcessResidue(mission))
			return;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_iMaterializationStateEnteredAtSecond = restoreSecond;
		operation.m_iStrategicLastUpdateSecond = restoreSecond;
		operation.m_iVirtualCombatLastStepSecond = restoreSecond;
		operation.m_iLastProgressAtSecond = restoreSecond;
		operation.m_sLastProjectionReason = "schema-58 restore folded exact rescue POW guard into held strategic survivor authority";
		NormalizeBatchForStrategicHold(batch, restoreSecond);
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		int living = queue.CountStrategicLivingMemberSlots(batch);
		operation.m_iLastVirtualFriendlyCount = living;
		NormalizeGroupForStrategicHold(operation, group, living);
		NormalizeCaptiveProcessResidue(mission, restoreSecond);
		operation.m_iRevision++;
	}

	protected void NormalizeBatchForStrategicHold(
		HST_ForceSpawnResultState batch,
		int restoreSecond)
	{
		if (!batch)
			return;
		bool terminalZeroRoster = batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
		batch.m_sNativeGroupId = "";
		batch.m_bStrategicProjectionHeld = true;
		batch.m_bCancelRequested = false;
		batch.m_iStrategicHoldSinceSecond = restoreSecond;
		batch.m_iNextAttemptSecond = 0;
		batch.m_iUpdatedAtSecond = restoreSecond;
		batch.m_iLastLifecycleSecond = restoreSecond;
		if (!terminalZeroRoster)
		{
			batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING;
			batch.m_sTerminalReason = "";
			batch.m_sLastFailureReason = "";
			batch.m_iCompletedAtSecond = 0;
			batch.m_iReprojectionCount++;
			batch.m_iAttemptGeneration++;
		}
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
			slot.m_iUpdatedAtSecond = restoreSecond;
			if (!terminalZeroRoster && (slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				|| !slot.m_bCasualtyConfirmed))
				slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED;
		}
	}

	protected void NormalizeGroupForStrategicHold(
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		int living)
	{
		if (!operation || !group)
			return;
		ClearGroupProcessResidue(group);
		if (living > 0)
			group.m_sRuntimeStatus = HST_RescuePOWOperationService.GUARD_VIRTUAL_STATUS;
		else
			group.m_sRuntimeStatus = HST_RescuePOWOperationService.GUARD_ELIMINATED_STATUS;
		group.m_sSpawnFallbackMode = HST_RescuePOWOperationService.EXACT_GROUP_MODE;
		group.m_vPosition = operation.m_vStrategicPosition;
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		group.m_vTargetPosition = operation.m_vAssignmentPosition;
		group.m_iInfantryCount = Math.Max(0, living);
		group.m_iLastSeenAliveCount = Math.Max(0, living);
		group.m_iSurvivorInfantryCount = Math.Max(0, living);
		group.m_iDurableLivingInfantryCount = Math.Max(0, living);
		group.m_iLifecycleRevision++;
	}

	protected void NormalizeCaptiveProcessResidue(
		HST_ActiveMissionState mission,
		int restoreSecond)
	{
		if (!mission)
			return;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId
				|| asset.m_iRescueContractVersion != HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION)
				continue;
			bool runtimeResidue = NormalizeRuntimeProjectionRecord(asset, true);
			bool residue = asset.m_bSpawned || asset.m_bAttachedToCarrier
				|| !asset.m_sEntityId.IsEmpty() || !asset.m_sCarriedByVehicleId.IsEmpty()
				|| runtimeResidue;
			asset.m_bSpawned = false;
			asset.m_bAttachedToCarrier = false;
			asset.m_sEntityId = "";
			asset.m_sCarriedByVehicleId = "";
			if (residue)
			{
				asset.m_iRescueProjectionGeneration++;
				asset.m_iRescueRevision++;
				asset.m_iRescueTransitionSecond = restoreSecond;
			}
		}
	}

	protected void NormalizeSettledProcessResidue(
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (batch)
		{
			batch.m_sNativeGroupId = "";
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
		ClearGroupProcessResidue(group);
	}

	protected void QuarantineAggregate(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		if (reason.IsEmpty())
			reason = "exact rescue POW authority conflict";
		if (mission && mission.m_sMissionId == HST_RescuePOWOperationService.EXACT_MISSION_ID
			&& (mission.m_iOperationContractVersion != 0
				|| MissionStronglyClaimsOperation(mission, operation)))
		{
			mission.m_iOperationContractVersion = HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION;
			mission.m_sRuntimePhase = QUARANTINE_PHASE;
			mission.m_sRuntimeFailureReason = reason;
			mission.m_sLastRuntimeEventKey = QUARANTINE_EVENT_KEY;
			mission.m_bRuntimeSpawned = false;
			mission.m_bRuntimeFallback = false;
			mission.m_bRescueExtractionGrace = false;
			mission.m_iRescueGraceUntilSecond = 0;
		}
		if (operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE)
		{
			bool operationChanged = operation.m_iContractVersion
				!= HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION
				|| operation.m_sLastProjectionReason != reason;
			operation.m_iContractVersion = HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION;
			if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
			{
				operationChanged = operationChanged
					|| operation.m_eMaterializationState
						!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
					|| operation.m_ePositionAuthority
						!= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
				operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
				operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
			}
			operation.m_sLastProjectionReason = reason;
			if (operationChanged)
				operation.m_iRevision++;
		}
		if (BatchClaimsAuthority(batch, mission, operation))
			HoldBatch(batch, reason);
		if (GroupClaimsAuthority(group, mission, operation))
			HoldGroup(group, reason);
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (AssetClaimsAuthority(asset, mission, operation, manifest))
				QuarantineAsset(asset, reason);
		}
		HoldStrongClaimants(mission, operation, manifest, reason);
	}

	protected int PreserveOrphanClaimants(array<string> handledOperationIds)
	{
		int quarantinedCount;
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (!IsSchema58RescuePOWMissionClaimant(m_SaveData, mission)
				|| handledOperationIds.Contains(mission.m_sOperationId))
				continue;
			HST_OperationRecordState operation = FindUniqueOperation(mission.m_sOperationId);
			HST_ForceManifestState manifest = FindUniqueManifest(mission.m_sManifestId);
			HST_ForceSpawnResultState batch = FindUniqueBatch(mission.m_sSpawnResultId);
			HST_ActiveGroupState group;
			if (operation)
				group = FindUniqueGroup(operation.m_sGroupId);
			QuarantineAggregate(mission, operation, manifest, batch, group,
				"exact rescue POW mission has no validated reciprocal authority graph");
			quarantinedCount++;
		}
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!IsSchema58RescuePOWOperationClaimant(m_SaveData, operation)
				|| handledOperationIds.Contains(operation.m_sOperationId))
				continue;
			HST_ActiveMissionState mission = FindUniqueMission(operation.m_sMissionInstanceId);
			QuarantineAggregate(
				mission,
				operation,
				FindUniqueManifest(operation.m_sManifestId),
				FindUniqueBatch(operation.m_sSpawnResultId),
				FindUniqueGroup(operation.m_sGroupId),
				"exact rescue POW operation has no validated reciprocal mission graph");
			quarantinedCount++;
		}
		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (!IsSchema58RescuePOWManifestClaimant(m_SaveData, manifest)
				|| handledOperationIds.Contains(manifest.m_sOperationId))
				continue;
			HoldStrongClaimants(null, null, manifest,
				"exact rescue POW manifest has no validated reciprocal operation graph");
			quarantinedCount++;
		}
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!IsSchema58RescuePOWAssetClaimant(m_SaveData, asset)
				|| handledOperationIds.Contains(asset.m_sOperationId))
				continue;
			QuarantineAsset(asset,
				"exact rescue POW captive has no validated reciprocal operation graph");
		}
		return quarantinedCount;
	}

	protected void HoldStrongClaimants(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		string reason)
	{
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (BatchClaimsAuthority(batch, mission, operation)
				|| (manifest && IsSchema58RescuePOWBatchClaimant(m_SaveData, batch)
					&& batch.m_sManifestId == manifest.m_sManifestId))
				HoldBatch(batch, reason);
		}
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (GroupClaimsAuthority(group, mission, operation)
				|| (manifest && IsSchema58RescuePOWGroupClaimant(m_SaveData, group)
					&& group.m_sManifestId == manifest.m_sManifestId
					&& group.m_sOperationId == manifest.m_sOperationId))
				HoldGroup(group, reason);
		}
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (AssetClaimsAuthority(asset, mission, operation, manifest))
				QuarantineAsset(asset, reason);
		}
	}

	protected void HoldBatch(HST_ForceSpawnResultState batch, string reason)
	{
		if (!batch)
			return;
		bool changed = !batch.m_bStrategicProjectionHeld || !batch.m_bCancelRequested
			|| batch.m_sLastFailureReason != reason || BatchHasProcessResidue(batch);
		batch.m_bStrategicProjectionHeld = true;
		batch.m_bCancelRequested = true;
		batch.m_sLastFailureReason = reason;
		batch.m_sNativeGroupId = "";
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
		if (changed)
		{
			batch.m_iLifecycleRevision++;
			batch.m_iLastLifecycleSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		}
	}

	protected void HoldGroup(HST_ActiveGroupState group, string reason)
	{
		if (!group)
			return;
		bool changed = group.m_sSpawnFallbackMode != HST_RescuePOWOperationService.QUARANTINE_STATUS
			|| group.m_sRuntimeStatus != HST_RescuePOWOperationService.QUARANTINE_STATUS
			|| group.m_sSpawnFailureReason != reason || GroupHasProcessResidue(group);
		ClearGroupProcessResidue(group);
		group.m_sSpawnFallbackMode = HST_RescuePOWOperationService.QUARANTINE_STATUS;
		group.m_sRuntimeStatus = HST_RescuePOWOperationService.QUARANTINE_STATUS;
		group.m_sSpawnFailureReason = reason;
		if (changed)
			group.m_iLifecycleRevision++;
	}

	protected void QuarantineAsset(HST_MissionAssetState asset, string reason)
	{
		if (!asset)
			return;
		bool runtimeResidue = NormalizeRuntimeProjectionRecord(asset, false);
		bool changed = asset.m_iRescueContractVersion
			!= HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION
			|| asset.m_bSpawned || asset.m_bAttachedToCarrier
			|| !asset.m_sEntityId.IsEmpty() || !asset.m_sCarriedByVehicleId.IsEmpty()
			|| runtimeResidue;
		asset.m_iRescueContractVersion = HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION;
		asset.m_bSpawned = false;
		asset.m_bAttachedToCarrier = false;
		asset.m_sEntityId = "";
		asset.m_sCarriedByVehicleId = "";
		if (changed)
		{
			asset.m_iRescueProjectionGeneration++;
			asset.m_iRescueRevision++;
			asset.m_iRescueTransitionSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		}
	}

	protected bool NormalizeRuntimeProjectionRecord(
		HST_MissionAssetState asset,
		bool applyTypedOutcome)
	{
		if (!asset || asset.m_sRescueProjectionId.IsEmpty())
			return false;
		bool changed;
		foreach (HST_MissionRuntimeEntityState runtimeEntity : m_SaveData.m_aMissionRuntimeEntities)
		{
			if (!runtimeEntity || runtimeEntity.m_sRuntimeEntityId != asset.m_sRescueProjectionId)
				continue;
			if (runtimeEntity.m_bSpawned)
				changed = true;
			runtimeEntity.m_bSpawned = false;
			if (!applyTypedOutcome)
				continue;
			bool destroyed = asset.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED;
			bool recovered = asset.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED;
			if (runtimeEntity.m_bDestroyed != destroyed || runtimeEntity.m_bRecovered != recovered)
				changed = true;
			runtimeEntity.m_bDestroyed = destroyed;
			runtimeEntity.m_bRecovered = recovered;
			if (!IsZeroVector(asset.m_vCurrentPosition))
				runtimeEntity.m_vPosition = asset.m_vCurrentPosition;
		}
		return changed;
	}

	protected void ClearGroupProcessResidue(HST_ActiveGroupState group)
	{
		if (!group)
			return;
		group.m_bSpawnedEntity = false;
		group.m_bSpawnAttempted = false;
		group.m_bSpawnCompleted = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
	}

	protected bool HasTypedOperationForMission(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (IsSchema58RescuePOWOperationClaimant(m_SaveData, operation)
				&& operation.m_sMissionInstanceId == mission.m_sInstanceId)
				return true;
		}
		return false;
	}

	protected bool MissionStronglyClaimsOperation(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation)
	{
		if (!mission || !operation
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
			|| operation.m_sOperationId.IsEmpty()
			|| mission.m_sOperationId != operation.m_sOperationId
			|| mission.m_sInstanceId != operation.m_sMissionInstanceId)
			return false;
		return (!mission.m_sManifestId.IsEmpty() && mission.m_sManifestId == operation.m_sManifestId)
			|| (!mission.m_sSpawnResultId.IsEmpty() && mission.m_sSpawnResultId == operation.m_sSpawnResultId);
	}

	protected bool BatchClaimsAuthority(
		HST_ForceSpawnResultState batch,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation)
	{
		if (!batch)
			return false;
		if (operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
			&& !operation.m_sOperationId.IsEmpty()
			&& batch.m_sOperationId == operation.m_sOperationId)
		{
			return (!operation.m_sSpawnResultId.IsEmpty() && batch.m_sResultId == operation.m_sSpawnResultId)
				|| (!operation.m_sManifestId.IsEmpty() && batch.m_sManifestId == operation.m_sManifestId)
				|| (!operation.m_sForceId.IsEmpty() && batch.m_sForceId == operation.m_sForceId)
				|| (!operation.m_sProjectionId.IsEmpty() && batch.m_sProjectionId == operation.m_sProjectionId);
		}
		if (!mission || mission.m_sOperationId.IsEmpty()
			|| batch.m_sOperationId != mission.m_sOperationId)
			return false;
		return (!mission.m_sSpawnResultId.IsEmpty() && batch.m_sResultId == mission.m_sSpawnResultId)
			|| (!mission.m_sManifestId.IsEmpty() && batch.m_sManifestId == mission.m_sManifestId);
	}

	protected bool GroupClaimsAuthority(
		HST_ActiveGroupState group,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation)
	{
		if (!group)
			return false;
		if (operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
			&& !operation.m_sOperationId.IsEmpty()
			&& group.m_sOperationId == operation.m_sOperationId)
		{
			return (!operation.m_sGroupId.IsEmpty() && group.m_sGroupId == operation.m_sGroupId)
				|| (!operation.m_sManifestId.IsEmpty() && group.m_sManifestId == operation.m_sManifestId)
				|| (!operation.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == operation.m_sSpawnResultId)
				|| (!operation.m_sForceId.IsEmpty() && group.m_sForceId == operation.m_sForceId)
				|| (!operation.m_sProjectionId.IsEmpty() && group.m_sProjectionId == operation.m_sProjectionId);
		}
		if (!mission || mission.m_sOperationId.IsEmpty()
			|| group.m_sOperationId != mission.m_sOperationId
			|| group.m_sMissionInstanceId != mission.m_sInstanceId)
			return false;
		return (!mission.m_sManifestId.IsEmpty() && group.m_sManifestId == mission.m_sManifestId)
			|| (!mission.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == mission.m_sSpawnResultId);
	}

	protected bool AssetClaimsAuthority(
		HST_MissionAssetState asset,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		if (!asset)
			return false;
		if (asset.m_iRescueContractVersion == HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION
			|| asset.m_iRescueContractVersion == HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION)
			return true;
		if (operation && !operation.m_sOperationId.IsEmpty()
			&& asset.m_sOperationId == operation.m_sOperationId)
			return !operation.m_sManifestId.IsEmpty()
				&& asset.m_sManifestId == operation.m_sManifestId
				&& !asset.m_sManifestSlotId.IsEmpty();
		if (manifest && !manifest.m_sManifestId.IsEmpty()
			&& asset.m_sManifestId == manifest.m_sManifestId
			&& asset.m_sOperationId == manifest.m_sOperationId
			&& !asset.m_sManifestSlotId.IsEmpty())
			return true;
		return mission && !mission.m_sOperationId.IsEmpty()
			&& asset.m_sMissionInstanceId == mission.m_sInstanceId
			&& asset.m_sOperationId == mission.m_sOperationId
			&& asset.m_sManifestId == mission.m_sManifestId
			&& !asset.m_sManifestSlotId.IsEmpty();
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
				|| slot.m_bGameMasterVerified || slot.m_bProjectionVerified
				|| slot.m_bSeatVerified)
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

	protected bool MissionCaptivesHaveProcessResidue(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId
				|| asset.m_iRescueContractVersion != HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION)
				continue;
			if (asset.m_bSpawned || asset.m_bAttachedToCarrier
				|| !asset.m_sEntityId.IsEmpty() || !asset.m_sCarriedByVehicleId.IsEmpty())
				return true;
			foreach (HST_MissionRuntimeEntityState runtimeEntity : m_SaveData.m_aMissionRuntimeEntities)
			{
				if (runtimeEntity && runtimeEntity.m_sRuntimeEntityId == asset.m_sRescueProjectionId
					&& runtimeEntity.m_bSpawned)
					return true;
			}
		}
		return false;
	}

	protected void RecordNormalizationConflict(int validatedCount, int quarantinedCount)
	{
		if (quarantinedCount <= 0 || HasEvent(CONFLICT_EVENT_ID))
			return;
		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = CONFLICT_EVENT_ID;
		eventState.m_sCategory = "normalization";
		eventState.m_sAggregateType = "mission_rescue_authority";
		eventState.m_sAggregateId = "schema58";
		eventState.m_sTransition = "corrupt_exact_rescue_pows_quarantined";
		eventState.m_sReason = string.Format(
			"validated %1 exact rescue POW authorities and quarantined %2 incomplete, conflicting, unsupported, or non-unique graphs without inventing captive death, extraction, carrier, settlement, or reward authority",
			validatedCount,
			quarantinedCount);
		eventState.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		m_SaveData.m_aCampaignEvents.Insert(eventState);
	}

	protected HST_ActiveMissionState FindUniqueMission(string instanceId)
	{
		HST_ActiveMissionState match;
		if (instanceId.IsEmpty())
			return null;
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (!mission || mission.m_sInstanceId != instanceId)
				continue;
			if (match)
				return null;
			match = mission;
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

	protected HST_MissionAssetState FindUniqueMissionAsset(string assetId)
	{
		HST_MissionAssetState match;
		if (assetId.IsEmpty())
			return null;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!asset || asset.m_sAssetId != assetId)
				continue;
			if (match)
				return null;
			match = asset;
		}
		return match;
	}

	protected HST_ForceManifestAssetState FindUniqueManifestAsset(
		HST_ForceManifestState manifest,
		string slotId)
	{
		HST_ForceManifestAssetState match;
		if (!manifest || slotId.IsEmpty())
			return null;
		foreach (HST_ForceManifestAssetState asset : manifest.m_aAssets)
		{
			if (!asset || asset.m_sSlotId != slotId)
				continue;
			if (match)
				return null;
			match = asset;
		}
		return match;
	}

	protected int CountMissionsByAnyIdentity(
		HST_ActiveMissionState expected,
		HST_OperationRecordState operation)
	{
		int count;
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (!mission)
				continue;
			bool matches = mission.m_sInstanceId == expected.m_sInstanceId;
			if (!matches && !operation.m_sOperationId.IsEmpty())
				matches = mission.m_sOperationId == operation.m_sOperationId;
			if (!matches && !operation.m_sManifestId.IsEmpty())
				matches = mission.m_sManifestId == operation.m_sManifestId;
			if (!matches && !operation.m_sSpawnResultId.IsEmpty())
				matches = mission.m_sSpawnResultId == operation.m_sSpawnResultId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountOperationsByAnyIdentity(
		HST_ActiveMissionState mission,
		HST_OperationRecordState expected)
	{
		int count;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!operation)
				continue;
			bool matches = operation.m_sOperationId == expected.m_sOperationId;
			if (!matches && !mission.m_sInstanceId.IsEmpty())
				matches = operation.m_sMissionInstanceId == mission.m_sInstanceId;
			if (!matches && !mission.m_sManifestId.IsEmpty())
				matches = operation.m_sManifestId == mission.m_sManifestId;
			if (!matches && !mission.m_sSpawnResultId.IsEmpty())
				matches = operation.m_sSpawnResultId == mission.m_sSpawnResultId;
			if (!matches && !expected.m_sGroupId.IsEmpty())
				matches = operation.m_sGroupId == expected.m_sGroupId;
			if (!matches && !expected.m_sProjectionId.IsEmpty())
				matches = operation.m_sProjectionId == expected.m_sProjectionId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountManifestsByAnyIdentity(
		HST_OperationRecordState operation,
		HST_ForceManifestState expected)
	{
		int count;
		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (!manifest)
				continue;
			bool matches = manifest.m_sManifestId == expected.m_sManifestId;
			if (!matches && !operation.m_sOperationId.IsEmpty())
				matches = manifest.m_sOperationId == operation.m_sOperationId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountBatchesByAnyIdentity(
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState expected)
	{
		int count;
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!batch)
				continue;
			bool matches = batch.m_sResultId == expected.m_sResultId;
			if (!matches && !operation.m_sOperationId.IsEmpty())
				matches = batch.m_sOperationId == operation.m_sOperationId;
			if (!matches && !operation.m_sManifestId.IsEmpty())
				matches = batch.m_sManifestId == operation.m_sManifestId;
			if (!matches && !operation.m_sForceId.IsEmpty())
				matches = batch.m_sForceId == operation.m_sForceId;
			if (!matches && !operation.m_sProjectionId.IsEmpty())
				matches = batch.m_sProjectionId == operation.m_sProjectionId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountBatchesClaimingOperation(HST_OperationRecordState operation)
	{
		int count;
		if (!operation)
			return count;
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!batch)
				continue;
			if ((!operation.m_sSpawnResultId.IsEmpty() && batch.m_sResultId == operation.m_sSpawnResultId)
				|| (!operation.m_sOperationId.IsEmpty() && batch.m_sOperationId == operation.m_sOperationId)
				|| (!operation.m_sManifestId.IsEmpty() && batch.m_sManifestId == operation.m_sManifestId)
				|| (!operation.m_sForceId.IsEmpty() && batch.m_sForceId == operation.m_sForceId)
				|| (!operation.m_sProjectionId.IsEmpty() && batch.m_sProjectionId == operation.m_sProjectionId))
				count++;
		}
		return count;
	}

	protected int CountGroupsByAnyIdentity(
		HST_OperationRecordState operation,
		HST_ActiveGroupState expected)
	{
		int count;
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!group)
				continue;
			bool matches = group.m_sGroupId == expected.m_sGroupId;
			if (!matches && !operation.m_sOperationId.IsEmpty())
				matches = group.m_sOperationId == operation.m_sOperationId;
			if (!matches && !operation.m_sManifestId.IsEmpty())
				matches = group.m_sManifestId == operation.m_sManifestId;
			if (!matches && !operation.m_sSpawnResultId.IsEmpty())
				matches = group.m_sSpawnResultId == operation.m_sSpawnResultId;
			if (!matches && !operation.m_sForceId.IsEmpty())
				matches = group.m_sForceId == operation.m_sForceId;
			if (!matches && !operation.m_sProjectionId.IsEmpty())
				matches = group.m_sProjectionId == operation.m_sProjectionId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountGroupsClaimingOperation(HST_OperationRecordState operation)
	{
		int count;
		if (!operation)
			return count;
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!group)
				continue;
			if ((!operation.m_sGroupId.IsEmpty() && group.m_sGroupId == operation.m_sGroupId)
				|| (!operation.m_sOperationId.IsEmpty() && group.m_sOperationId == operation.m_sOperationId)
				|| (!operation.m_sManifestId.IsEmpty() && group.m_sManifestId == operation.m_sManifestId)
				|| (!operation.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == operation.m_sSpawnResultId)
				|| (!operation.m_sForceId.IsEmpty() && group.m_sForceId == operation.m_sForceId)
				|| (!operation.m_sProjectionId.IsEmpty() && group.m_sProjectionId == operation.m_sProjectionId))
				count++;
		}
		return count;
	}

	protected int CountMissionAssetsByAnyIdentity(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		string slotId,
		string assetId)
	{
		int count;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!asset)
				continue;
			bool matches = asset.m_sAssetId == assetId;
			if (!matches && !slotId.IsEmpty())
				matches = asset.m_sManifestSlotId == slotId;
			if (!matches && !operation.m_sOperationId.IsEmpty()
				&& asset.m_sOperationId == operation.m_sOperationId
				&& asset.m_iRescueOrdinal >= 0)
				matches = asset.m_iRescueOrdinal == ResolveOrdinalFromSlot(mission, slotId);
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountMissionCaptiveClaimants(string missionInstanceId)
	{
		int count;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == missionInstanceId
				&& (asset.m_iRescueContractVersion == HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION
					|| asset.m_iRescueContractVersion == HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION))
				count++;
		}
		return count;
	}

	protected int CountMissionOwnedAssets(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		int count;
		if (!mission || !operation || !manifest)
			return count;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!asset)
				continue;
			bool matchesAuthority = asset.m_sMissionInstanceId == mission.m_sInstanceId;
			if (!matchesAuthority && !operation.m_sOperationId.IsEmpty())
				matchesAuthority = asset.m_sOperationId == operation.m_sOperationId;
			if (!matchesAuthority && !manifest.m_sManifestId.IsEmpty())
				matchesAuthority = asset.m_sManifestId == manifest.m_sManifestId;
			if (matchesAuthority)
				count++;
		}
		return count;
	}

	protected int ResolveOrdinalFromSlot(HST_ActiveMissionState mission, string slotId)
	{
		if (!mission)
			return -1;
		for (int ordinal = 0; ordinal < HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT; ordinal++)
		{
			if (slotId == HST_RescuePOWOperationService.BuildCaptiveSlotId(mission.m_sInstanceId, ordinal))
				return ordinal;
		}
		return -1;
	}

	protected vector ResolveExpectedCaptiveSourcePosition(vector targetPosition, int ordinal)
	{
		if (ordinal == 0)
			return targetPosition + Vector(-2.0, 0.0, 1.5);
		if (ordinal == 1)
			return targetPosition + Vector(0.0, 0.0, -1.5);
		return targetPosition + Vector(2.0, 0.0, 1.5);
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

	protected int CountManifestAssets(HST_ForceManifestState manifest, string slotId)
	{
		int count;
		foreach (HST_ForceManifestAssetState asset : manifest.m_aAssets)
		{
			if (asset && asset.m_sSlotId == slotId)
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

	protected bool PositionsMatch(vector first, vector second, float toleranceMeters = 0.5)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz <= toleranceMeters * toleranceMeters;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected bool HasEvent(string eventId)
	{
		foreach (HST_CampaignEventState eventState : m_SaveData.m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				return true;
		}
		return false;
	}
}
