class HST_MissionConvoySaveValidationService
{
	static const string SCHEMA52_CONVOY_PRIMITIVE = "convoy_intercept";
	static const string SCHEMA52_CONVOY_FAILURE_PHASE = "failed";
	static const string SCHEMA52_CONVOY_FAILURE_EVENT = "convoy_failed";

	protected HST_CampaignSaveData m_SaveData;

	void Normalize(HST_CampaignSaveData saveData, int restoredSchemaVersion)
	{
		if (!saveData)
			return;

		m_SaveData = saveData;
		NormalizeSchema52MissionConvoyAuthority(restoredSchemaVersion);
		m_SaveData = null;
	}

	protected void NormalizeSchema52MissionConvoyAuthority(int restoredSchemaVersion)
	{
		if (restoredSchemaVersion < 52)
		{
			int legacyConvoyCount;
			foreach (HST_ActiveMissionState legacyMission : m_SaveData.m_aActiveMissions)
			{
				if (!legacyMission || legacyMission.m_sRuntimePrimitive != SCHEMA52_CONVOY_PRIMITIVE)
					continue;

				// Schema 51 and earlier never admitted mission convoys into the
				// exact operation contract. Keep the historical mission playable,
				// but do not infer any of the new authority rows or receipts.
				legacyMission.m_iOperationContractVersion = 0;
				legacyMission.m_sOperationId = "";
				legacyMission.m_sManifestId = "";
				legacyMission.m_sSpawnResultId = "";
				legacyMission.m_sSettlementId = "";
				legacyConvoyCount++;
			}

			if (HasCampaignEventId("migration_schema52_mission_convoy_authority"))
				return;
			HST_CampaignEventState migrationEvent = new HST_CampaignEventState();
			migrationEvent.m_sEventId = "migration_schema52_mission_convoy_authority";
			migrationEvent.m_sCategory = "migration";
			migrationEvent.m_sAggregateType = "operation_record";
			migrationEvent.m_sAggregateId = "schema52";
			migrationEvent.m_sTransition = "legacy_mission_convoys_preserved_contract_zero";
			migrationEvent.m_sReason = string.Format("preserved %1 pre-schema-52 convoy missions at operation contract version 0; created no exact operations, manifests, rosters, routes, elements, cargo authority, or settlements", legacyConvoyCount);
			migrationEvent.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
			m_SaveData.m_aCampaignEvents.Insert(migrationEvent);
			return;
		}

		int validatedCount;
		int quarantinedCount;
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (!mission || !IsSchema52MissionConvoyMissionClaimant(m_SaveData, mission))
				continue;
			if (mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.QUARANTINED_CONTRACT_VERSION
				&& mission.m_sRuntimePhase == SCHEMA52_CONVOY_FAILURE_PHASE)
				continue;

			HST_OperationRecordState operation = FindSchema52MissionConvoyOperation(mission.m_sOperationId);
			HST_ForceManifestState manifest = FindSchema52MissionConvoyManifest(mission.m_sManifestId);
			HST_ForceSpawnResultState batch = FindSchema52MissionConvoyBatch(mission.m_sSpawnResultId);
			string failure = ValidateSchema52MissionConvoyRestore(mission, operation, manifest, batch);
			if (failure.IsEmpty())
			{
				NormalizeSchema52MissionConvoyDerivedGroupSurvivors(mission);
				validatedCount++;
				continue;
			}

			QuarantineSchema52MissionConvoy(mission, failure);
			quarantinedCount++;
		}

		if (quarantinedCount <= 0 || HasCampaignEventId("normalization_schema52_mission_convoy_authority_conflict"))
			return;
		HST_CampaignEventState conflictEvent = new HST_CampaignEventState();
		conflictEvent.m_sEventId = "normalization_schema52_mission_convoy_authority_conflict";
		conflictEvent.m_sCategory = "normalization";
		conflictEvent.m_sAggregateType = "mission_convoy_authority";
		conflictEvent.m_sAggregateId = "schema52";
		conflictEvent.m_sTransition = "corrupt_exact_convoys_quarantined";
		conflictEvent.m_sReason = string.Format("validated %1 exact mission convoy authorities and quarantined %2 incomplete, conflicting, or non-unique mission roots without mutating their claimed operation, manifest, batch, group, asset, element, route, or settlement rows", validatedCount, quarantinedCount);
		conflictEvent.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		m_SaveData.m_aCampaignEvents.Insert(conflictEvent);
	}

	static bool IsSchema52MissionConvoyMissionClaimant(
		HST_CampaignSaveData saveData,
		HST_ActiveMissionState mission)
	{
		if (!saveData || !mission)
			return false;
		bool hasExactIdentityReceipt = !mission.m_sOperationId.IsEmpty()
			|| !mission.m_sManifestId.IsEmpty()
			|| !mission.m_sSpawnResultId.IsEmpty()
			|| !mission.m_sSettlementId.IsEmpty();
		if (mission.m_sRuntimePrimitive == SCHEMA52_CONVOY_PRIMITIVE
			&& (mission.m_iOperationContractVersion != 0 || hasExactIdentityReceipt))
			return true;

		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY)
				continue;
			if ((!mission.m_sOperationId.IsEmpty() && operation.m_sOperationId == mission.m_sOperationId)
				|| (!mission.m_sInstanceId.IsEmpty() && operation.m_sMissionInstanceId == mission.m_sInstanceId))
				return true;
		}
		foreach (HST_ConvoyElementState element : saveData.m_aConvoyElements)
		{
			if (element && !mission.m_sInstanceId.IsEmpty() && element.m_sMissionInstanceId == mission.m_sInstanceId)
				return true;
		}
		return false;
	}

	static bool HasSchema52MissionConvoyMissionClaimant(HST_CampaignSaveData saveData)
	{
		if (!saveData)
			return false;
		foreach (HST_ActiveMissionState mission : saveData.m_aActiveMissions)
		{
			if (IsSchema52MissionConvoyMissionClaimant(saveData, mission))
				return true;
		}
		return false;
	}

	static bool IsSchema52MissionConvoyRouteClaimant(
		HST_CampaignSaveData saveData,
		HST_GeneratedRouteState route)
	{
		if (!saveData || !route || route.m_sRouteId.IsEmpty())
			return false;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY
				&& operation.m_sCurrentRouteId == route.m_sRouteId)
				return true;
		}
		foreach (HST_ActiveMissionState mission : saveData.m_aActiveMissions)
		{
			if (!IsSchema52MissionConvoyMissionClaimant(saveData, mission) || mission.m_sSiteId.IsEmpty())
				continue;
			foreach (HST_GeneratedSiteState site : saveData.m_aGeneratedSites)
			{
				if (site && site.m_sSiteId == mission.m_sSiteId && site.m_sRouteId == route.m_sRouteId)
					return true;
			}
		}
		return false;
	}

	static bool IsSchema52MissionConvoyAssetClaimant(
		HST_CampaignSaveData saveData,
		HST_MissionAssetState asset)
	{
		if (!saveData || !asset)
			return false;
		if (!asset.m_sConvoyElementId.IsEmpty())
			return true;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY)
				continue;
			if ((!asset.m_sOperationId.IsEmpty() && asset.m_sOperationId == operation.m_sOperationId)
				|| (!asset.m_sManifestId.IsEmpty() && asset.m_sManifestId == operation.m_sManifestId))
				return true;
		}
		foreach (HST_ActiveMissionState mission : saveData.m_aActiveMissions)
		{
			if (!IsSchema52MissionConvoyMissionClaimant(saveData, mission) || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (asset.m_sRole == HST_MissionConvoyOperationService.VEHICLE_ROLE
				|| asset.m_sRole == HST_MissionConvoyOperationService.PAYLOAD_ROLE
				|| asset.m_sRole == HST_MissionConvoyOperationService.CAPTIVE_ROLE)
				return true;
		}
		return false;
	}

	static bool IsSchema52MissionConvoyGroupClaimant(
		HST_CampaignSaveData saveData,
		HST_ActiveGroupState group)
	{
		if (!saveData || !group)
			return false;
		if (!group.m_sConvoyElementId.IsEmpty() || group.m_sGroupId.StartsWith("mission_convoy_"))
			return true;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY)
				continue;
			bool operationMatches = !group.m_sOperationId.IsEmpty() && group.m_sOperationId == operation.m_sOperationId;
			bool manifestMatches = !group.m_sManifestId.IsEmpty() && group.m_sManifestId == operation.m_sManifestId;
			bool projectionMatches = !group.m_sProjectionId.IsEmpty() && group.m_sProjectionId == operation.m_sProjectionId;
			bool forceMatches = !group.m_sForceId.IsEmpty() && group.m_sForceId == operation.m_sForceId;
			if (operationMatches || manifestMatches || projectionMatches || forceMatches)
				return true;
		}
		return false;
	}

	static bool IsSchema52MissionConvoyManifestClaimant(
		HST_CampaignSaveData saveData,
		HST_ForceManifestState manifest)
	{
		if (!saveData || !manifest)
			return false;
		if (manifest.m_sForceKind == HST_MissionConvoyOperationService.EXACT_FORCE_KIND)
			return true;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY
				&& (operation.m_sManifestId == manifest.m_sManifestId || operation.m_sOperationId == manifest.m_sOperationId))
				return true;
		}
		foreach (HST_ActiveMissionState mission : saveData.m_aActiveMissions)
		{
			if (IsSchema52MissionConvoyMissionClaimant(saveData, mission) && !mission.m_sManifestId.IsEmpty()
				&& mission.m_sManifestId == manifest.m_sManifestId)
				return true;
		}
		return false;
	}

	static bool IsSchema52MissionConvoyBatchClaimant(
		HST_CampaignSaveData saveData,
		HST_ForceSpawnResultState batch)
	{
		if (!saveData || !batch)
			return false;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY)
				continue;
			if (operation.m_sSpawnResultId == batch.m_sResultId || operation.m_sOperationId == batch.m_sOperationId
				|| operation.m_sProjectionId == batch.m_sProjectionId || operation.m_sForceId == batch.m_sForceId)
				return true;
		}
		foreach (HST_ActiveMissionState mission : saveData.m_aActiveMissions)
		{
			if (!IsSchema52MissionConvoyMissionClaimant(saveData, mission))
				continue;
			if (mission.m_sSpawnResultId == batch.m_sResultId || mission.m_sInstanceId == batch.m_sRequestId)
				return true;
		}
		return false;
	}

	protected void QuarantineSchema52MissionConvoy(HST_ActiveMissionState mission, string failure)
	{
		if (!mission)
			return;
		mission.m_iOperationContractVersion = HST_MissionConvoyOperationService.QUARANTINED_CONTRACT_VERSION;
		mission.m_sRuntimePhase = SCHEMA52_CONVOY_FAILURE_PHASE;
		mission.m_sRuntimeFailureReason = "Exact mission convoy restore authority invalidated: " + failure;
		mission.m_sLastRuntimeEventKey = SCHEMA52_CONVOY_FAILURE_EVENT;
		mission.m_sConvoyOutcomeSummary = "authority quarantined without mutating ambiguous operation, manifest, batch, group, asset, element, route, or settlement rows: " + failure;
		mission.m_bRuntimeSpawned = false;
		mission.m_bRuntimeFallback = false;
	}

	protected void NormalizeSchema52MissionConvoyDerivedGroupSurvivors(HST_ActiveMissionState mission)
	{
		if (!mission)
			return;
		for (int ordinal = 0; ordinal < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; ordinal++)
		{
			HST_ConvoyElementState element = FindSchema52MissionConvoyElement(
				HST_MissionConvoyOperationService.BuildElementId(mission, ordinal));
			HST_ActiveGroupState group = FindSchema52MissionConvoyGroup(
				HST_MissionConvoyOperationService.BuildGroupId(mission, ordinal));
			if (!element || !group)
				continue;

			int survivors = Math.Max(0, Math.Min(element.m_iOriginalCrewCount, element.m_iSurvivingCrewCount));
			bool changed = group.m_iInfantryCount != survivors
				|| group.m_iLastSeenAliveCount != survivors
				|| group.m_iSurvivorInfantryCount != survivors
				|| group.m_iDurableLivingInfantryCount != survivors;
			group.m_iInfantryCount = survivors;
			group.m_iLastSeenAliveCount = survivors;
			group.m_iSurvivorInfantryCount = survivors;
			group.m_iDurableLivingInfantryCount = survivors;
			if (changed)
				group.m_iLifecycleRevision++;
		}
	}

	protected HST_OperationRecordState FindSchema52MissionConvoyOperation(string operationId)
	{
		HST_OperationRecordState match;
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

	protected HST_ForceManifestState FindSchema52MissionConvoyManifest(string manifestId)
	{
		HST_ForceManifestState match;
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

	protected HST_ForceSpawnResultState FindSchema52MissionConvoyBatch(string resultId)
	{
		HST_ForceSpawnResultState match;
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

	protected string ValidateSchema52MissionConvoyRestore(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		if (!mission || !operation || !manifest || !batch)
			return "exact mission convoy restore authority is incomplete or ambiguous";
		if (CountSchema52MissionConvoyMissionsByAnyIdentity(mission) != 1)
			return "exact mission convoy restore mission identity is ambiguous";
		if (CountSchema52MissionConvoyOperationsByAnyIdentity(operation, mission, manifest, batch) != 1)
			return "exact mission convoy restore operation identity is ambiguous";
		if (CountSchema52MissionConvoyManifestsByAnyIdentity(manifest, mission, operation) != 1)
			return "exact mission convoy restore manifest identity is ambiguous";
		if (CountSchema52MissionConvoyBatchesByAnyIdentity(batch, mission, operation, manifest) != 1)
			return "exact mission convoy restore roster-batch identity is ambiguous";

		if (mission.m_sRuntimePrimitive != SCHEMA52_CONVOY_PRIMITIVE
			|| mission.m_iOperationContractVersion != HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY
			|| operation.m_iContractVersion != HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION)
			return "exact mission convoy restore contract conflicts";
		if (mission.m_sInstanceId.IsEmpty()
			|| mission.m_sOperationId != HST_MissionConvoyOperationService.BuildOperationId(mission)
			|| mission.m_sManifestId != HST_MissionConvoyOperationService.BuildManifestId(mission)
			|| mission.m_sSpawnResultId != HST_MissionConvoyOperationService.BuildSpawnResultId(mission))
			return "exact mission convoy restore deterministic mission identity conflicts";
		if (operation.m_sOperationId != mission.m_sOperationId
			|| operation.m_sMissionInstanceId != mission.m_sInstanceId
			|| operation.m_sIssueRequestId != mission.m_sInstanceId)
			return "exact mission convoy restore operation backlinks conflict";
		if (operation.m_sManifestId != mission.m_sManifestId
			|| operation.m_sSpawnResultId != mission.m_sSpawnResultId
			|| !operation.m_sGroupId.IsEmpty())
			return "exact mission convoy restore operation backlinks conflict";
		if (operation.m_sForceId != HST_MissionConvoyOperationService.BuildForceId(mission)
			|| operation.m_sProjectionId != HST_MissionConvoyOperationService.BuildProjectionId(mission))
			return "exact mission convoy restore operation backlinks conflict";
		if (manifest.m_sManifestId != mission.m_sManifestId
			|| manifest.m_sOperationId != operation.m_sOperationId)
			return "exact mission convoy restore manifest or roster backlinks conflict";
		if (batch.m_sResultId != mission.m_sSpawnResultId || batch.m_sRequestId != mission.m_sInstanceId
			|| batch.m_sOperationId != operation.m_sOperationId)
			return "exact mission convoy restore manifest or roster backlinks conflict";
		if (batch.m_sManifestId != manifest.m_sManifestId || batch.m_sForceId != operation.m_sForceId
			|| batch.m_sProjectionId != operation.m_sProjectionId)
			return "exact mission convoy restore manifest or roster backlinks conflict";
		if (batch.m_sManifestHash != manifest.m_sManifestHash)
			return "exact mission convoy restore manifest or roster backlinks conflict";

		string failure = ValidateSchema52MissionConvoyManifest(mission, operation, manifest);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateSchema52MissionConvoyRoute(mission, operation, manifest);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateSchema52MissionConvoyProjection(mission, operation, batch);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateSchema52MissionConvoyBatch(mission, operation, manifest, batch);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateSchema52MissionConvoyElements(mission, operation, manifest, batch);
		if (!failure.IsEmpty())
			return failure;
		return ValidateSchema52MissionConvoySettlement(mission, operation, batch);
	}

	protected int CountSchema52MissionConvoyMissionsByAnyIdentity(HST_ActiveMissionState expected)
	{
		int count;
		if (!expected)
			return count;
		foreach (HST_ActiveMissionState candidate : m_SaveData.m_aActiveMissions)
		{
			if (!candidate)
				continue;
			bool matches = !expected.m_sInstanceId.IsEmpty() && candidate.m_sInstanceId == expected.m_sInstanceId;
			if (!matches && !expected.m_sOperationId.IsEmpty())
				matches = candidate.m_sOperationId == expected.m_sOperationId;
			if (!matches && !expected.m_sManifestId.IsEmpty())
				matches = candidate.m_sManifestId == expected.m_sManifestId;
			if (!matches && !expected.m_sSpawnResultId.IsEmpty())
				matches = candidate.m_sSpawnResultId == expected.m_sSpawnResultId;
			if (!matches && !expected.m_sSettlementId.IsEmpty())
				matches = candidate.m_sSettlementId == expected.m_sSettlementId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountSchema52MissionConvoyOperationsByAnyIdentity(
		HST_OperationRecordState expected,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		int count;
		if (!expected || !mission || !manifest || !batch)
			return count;
		foreach (HST_OperationRecordState candidate : m_SaveData.m_aOperations)
		{
			if (!candidate)
				continue;
			bool matches = candidate.m_sOperationId == expected.m_sOperationId;
			if (!matches && !mission.m_sInstanceId.IsEmpty())
				matches = candidate.m_sMissionInstanceId == mission.m_sInstanceId;
			if (!matches && !manifest.m_sManifestId.IsEmpty())
				matches = candidate.m_sManifestId == manifest.m_sManifestId;
			if (!matches && !batch.m_sResultId.IsEmpty())
				matches = candidate.m_sSpawnResultId == batch.m_sResultId;
			if (!matches && !expected.m_sForceId.IsEmpty())
				matches = candidate.m_sForceId == expected.m_sForceId;
			if (!matches && !expected.m_sProjectionId.IsEmpty())
				matches = candidate.m_sProjectionId == expected.m_sProjectionId;
			if (!matches && !expected.m_sSettlementId.IsEmpty())
				matches = candidate.m_sSettlementId == expected.m_sSettlementId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountSchema52MissionConvoyManifestsByAnyIdentity(
		HST_ForceManifestState expected,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation)
	{
		int count;
		if (!expected || !mission || !operation)
			return count;
		foreach (HST_ForceManifestState candidate : m_SaveData.m_aForceManifests)
		{
			if (!candidate)
				continue;
			bool matches = candidate.m_sManifestId == expected.m_sManifestId;
			if (!matches && !operation.m_sOperationId.IsEmpty())
				matches = candidate.m_sOperationId == operation.m_sOperationId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountSchema52MissionConvoyBatchesByAnyIdentity(
		HST_ForceSpawnResultState expected,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		int count;
		if (!expected || !mission || !operation || !manifest)
			return count;
		foreach (HST_ForceSpawnResultState candidate : m_SaveData.m_aForceSpawnResults)
		{
			if (!candidate)
				continue;
			bool matches = candidate.m_sResultId == expected.m_sResultId;
			if (!matches && !mission.m_sInstanceId.IsEmpty())
				matches = candidate.m_sRequestId == mission.m_sInstanceId;
			if (!matches && !operation.m_sOperationId.IsEmpty())
				matches = candidate.m_sOperationId == operation.m_sOperationId;
			if (!matches && !manifest.m_sManifestId.IsEmpty())
				matches = candidate.m_sManifestId == manifest.m_sManifestId;
			if (!matches && !expected.m_sForceId.IsEmpty())
				matches = candidate.m_sForceId == expected.m_sForceId;
			if (!matches && !expected.m_sProjectionId.IsEmpty())
				matches = candidate.m_sProjectionId == expected.m_sProjectionId;
			if (matches)
				count++;
		}
		return count;
	}

	protected string ValidateSchema52MissionConvoyManifest(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		if (!manifest.m_bFrozen || manifest.m_sManifestHash.IsEmpty()
			|| integrity.BuildManifestHash(manifest) != manifest.m_sManifestHash)
			return "exact mission convoy restore frozen manifest hash conflicts";
		if (manifest.m_sForceKind != HST_MissionConvoyOperationService.EXACT_FORCE_KIND
			|| manifest.m_sPolicyId != HST_MissionConvoyOperationService.EXACT_POLICY_ID)
			return "exact mission convoy restore manifest policy, faction, or seed conflicts";
		if (manifest.m_sCommandRequestId != mission.m_sInstanceId
			|| manifest.m_sIntentId != mission.m_sMissionId)
			return "exact mission convoy restore manifest mission receipt conflicts";
		if (manifest.m_sFactionKey.IsEmpty() || manifest.m_sFactionKey != operation.m_sOwnerFactionKey
			|| manifest.m_iDeterministicSeed != operation.m_iDeterministicSeed)
			return "exact mission convoy restore manifest policy, faction, or seed conflicts";
		if (manifest.m_iRequestedVehicleCount != HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT
			|| manifest.m_iAcceptedVehicleCount != HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT)
			return "exact mission convoy restore manifest is not exactly three vehicle/crew roots with at most one payload";
		if (manifest.m_aGroups.Count() != HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT
			|| manifest.m_aVehicles.Count() != HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT
			|| manifest.m_aAssets.Count() > 1)
			return "exact mission convoy restore manifest is not exactly three vehicle/crew roots with at most one payload";
		if (manifest.m_iAcceptedMemberCount <= 0
			|| manifest.m_iRequestedMemberCount != manifest.m_iAcceptedMemberCount
			|| manifest.m_iAcceptedMemberCount != manifest.m_aMembers.Count())
			return "exact mission convoy restore member manifest counts conflict";
		if (manifest.m_iMoneyCost != 0 || manifest.m_iHRCost != 0 || manifest.m_iEquipmentCost != 0)
			return "exact mission convoy restore contains unsupported prepaid resource authority";
		if (manifest.m_iAttackResourceCost != 0 || manifest.m_iSupportResourceCost != 0)
			return "exact mission convoy restore contains unsupported prepaid resource authority";

		int expectedMembers;
		int expectedMemberOrdinal;
		for (int ordinal = 0; ordinal < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; ordinal++)
		{
			string groupElementId = HST_MissionConvoyOperationService.BuildCrewGroupElementId(mission, ordinal);
			string vehicleSlotId = HST_MissionConvoyOperationService.BuildVehicleSlotId(mission, ordinal);
			HST_ForceManifestGroupState groupSlot = FindSchema52ManifestGroup(manifest, groupElementId);
			HST_ForceManifestVehicleState vehicleSlot = FindSchema52ManifestVehicle(manifest, vehicleSlotId);
			if (!groupSlot || !vehicleSlot
				|| CountSchema52ManifestGroups(manifest, groupElementId) != 1
				|| CountSchema52ManifestVehicles(manifest, vehicleSlotId) != 1)
				return string.Format("exact mission convoy restore manifest root %1 is missing or ambiguous", ordinal);
			if (groupSlot.m_iOrdinal != ordinal || vehicleSlot.m_iOrdinal != ordinal
				|| groupSlot.m_sRole != "convoy_crew")
				return string.Format("exact mission convoy restore manifest root %1 contract conflicts", ordinal);
			if (vehicleSlot.m_sRole != HST_MissionConvoyOperationService.VEHICLE_ROLE
				|| groupSlot.m_sPrefab.IsEmpty() || vehicleSlot.m_sPrefab.IsEmpty())
				return string.Format("exact mission convoy restore manifest root %1 contract conflicts", ordinal);
			if (vehicleSlot.m_sGroupElementId != groupSlot.m_sElementId
				|| groupSlot.m_iExpectedMemberCount <= 0
				|| vehicleSlot.m_iRequiredCrew != groupSlot.m_iExpectedMemberCount)
				return string.Format("exact mission convoy restore manifest root %1 contract conflicts", ordinal);
			if (!groupSlot.m_bRequired || !vehicleSlot.m_bRequired)
				return string.Format("exact mission convoy restore manifest root %1 contract conflicts", ordinal);
			int groupMemberCount = CountSchema52ManifestMembersForGroup(manifest, groupElementId);
			if (groupMemberCount != groupSlot.m_iExpectedMemberCount)
				return string.Format("exact mission convoy restore crew root %1 member count conflicts", ordinal);
			for (int seatIndex = 0; seatIndex < groupSlot.m_iExpectedMemberCount; seatIndex++)
			{
				string memberSlotId = HST_MissionConvoyOperationService.BuildMemberSlotId(mission, ordinal, seatIndex);
				HST_ForceManifestMemberState memberSlot = manifest.FindMemberSlot(memberSlotId);
				bool memberIdentityValid = memberSlot
					&& CountSchema52ManifestMembers(manifest, memberSlotId) == 1
					&& memberSlot.m_bRequired
					&& !memberSlot.m_sCatalogSlotId.IsEmpty();
				bool memberPrefabRoleValid = memberSlot
					&& !memberSlot.m_sPrefab.IsEmpty()
					&& !memberSlot.m_sRole.IsEmpty();
				bool memberAssignmentValid = memberSlot
					&& memberSlot.m_sGroupElementId == groupElementId
					&& memberSlot.m_sAssignedVehicleSlotId == vehicleSlotId;
				bool memberOrdinalValid = memberSlot
					&& memberSlot.m_iOrdinal == expectedMemberOrdinal
					&& memberSlot.m_iSeatIndex == seatIndex;
				if (!memberIdentityValid || !memberPrefabRoleValid || !memberAssignmentValid || !memberOrdinalValid)
					return string.Format("exact mission convoy restore crew root %1 seat %2 identity conflicts", ordinal, seatIndex);
				if ((seatIndex == 0 && memberSlot.m_sSeatRole != "driver")
					|| (seatIndex > 0 && memberSlot.m_sSeatRole != "passenger"))
					return string.Format("exact mission convoy restore crew root %1 seat %2 role conflicts", ordinal, seatIndex);
				expectedMemberOrdinal++;
			}
			expectedMembers += groupMemberCount;
		}
		if (expectedMembers != manifest.m_iAcceptedMemberCount || expectedMemberOrdinal != manifest.m_aMembers.Count())
			return "exact mission convoy restore manifest member roots do not balance";

		for (int memberIndex = 0; memberIndex < manifest.m_aMembers.Count(); memberIndex++)
		{
			HST_ForceManifestMemberState member = manifest.m_aMembers[memberIndex];
			if (!member || member.m_sSlotId.IsEmpty() || member.m_sCatalogSlotId.IsEmpty())
				return "exact mission convoy restore member slot identity conflicts";
			// Member roles remain the concrete authored catalog roles (driver may
			// still be a squad leader, rifleman, and so on).  "convoy_crew" is the
			// manifest group role, never a replacement for per-member catalog truth.
			if (member.m_sPrefab.IsEmpty() || member.m_sRole.IsEmpty()
				|| member.m_sGroupElementId.IsEmpty())
				return "exact mission convoy restore member slot identity conflicts";
			if (member.m_sAssignedVehicleSlotId.IsEmpty() || member.m_iOrdinal < 0
				|| member.m_iOrdinal >= manifest.m_aMembers.Count())
				return "exact mission convoy restore member slot identity conflicts";
			if (CountSchema52ManifestMembers(manifest, member.m_sSlotId) != 1)
				return "exact mission convoy restore member slot identity conflicts";
			HST_ForceManifestGroupState memberGroup = FindSchema52ManifestGroup(manifest, member.m_sGroupElementId);
			HST_ForceManifestVehicleState memberVehicle = FindSchema52ManifestVehicle(manifest, member.m_sAssignedVehicleSlotId);
			if (!memberGroup || !memberVehicle || memberVehicle.m_sGroupElementId != memberGroup.m_sElementId)
				return "exact mission convoy restore member assignment backlinks conflict";
			if ((member.m_sSeatRole != "driver" && member.m_sSeatRole != "passenger") || member.m_iSeatIndex < 0)
				return "exact mission convoy restore member seat contract conflicts";
		}

		if (manifest.m_aAssets.Count() == 1)
		{
			HST_ForceManifestAssetState assetSlot = manifest.m_aAssets[0];
			if (!assetSlot || assetSlot.m_sSlotId.IsEmpty() || assetSlot.m_sPrefab.IsEmpty())
				return "exact mission convoy restore payload manifest slot conflicts";
			bool payloadSlot = assetSlot.m_sRole == HST_MissionConvoyOperationService.PAYLOAD_ROLE
				&& assetSlot.m_sKind == HST_MissionConvoyOperationService.CARGO_KIND;
			bool captiveSlot = assetSlot.m_sRole == HST_MissionConvoyOperationService.CAPTIVE_ROLE
				&& assetSlot.m_sKind == HST_MissionConvoyOperationService.CAPTIVE_KIND;
			if (!payloadSlot && !captiveSlot)
				return "exact mission convoy restore payload manifest slot conflicts";
			if (assetSlot.m_sAssignedVehicleSlotId != HST_MissionConvoyOperationService.BuildVehicleSlotId(mission, 0)
				|| assetSlot.m_iQuantity != 1 || assetSlot.m_iOrdinal != 0)
				return "exact mission convoy restore payload manifest slot conflicts";
			if (!assetSlot.m_bRequired || CountSchema52ManifestAssets(manifest, assetSlot.m_sSlotId) != 1)
				return "exact mission convoy restore payload manifest slot conflicts";
		}
		return "";
	}

	protected HST_ForceManifestGroupState FindSchema52ManifestGroup(HST_ForceManifestState manifest, string elementId)
	{
		if (!manifest || elementId.IsEmpty())
			return null;
		foreach (HST_ForceManifestGroupState group : manifest.m_aGroups)
		{
			if (group && group.m_sElementId == elementId)
				return group;
		}
		return null;
	}

	protected HST_ForceManifestVehicleState FindSchema52ManifestVehicle(HST_ForceManifestState manifest, string slotId)
	{
		if (!manifest || slotId.IsEmpty())
			return null;
		foreach (HST_ForceManifestVehicleState vehicle : manifest.m_aVehicles)
		{
			if (vehicle && vehicle.m_sSlotId == slotId)
				return vehicle;
		}
		return null;
	}

	protected HST_ForceManifestAssetState FindSchema52ManifestAsset(HST_ForceManifestState manifest, string slotId)
	{
		if (!manifest || slotId.IsEmpty())
			return null;
		foreach (HST_ForceManifestAssetState asset : manifest.m_aAssets)
		{
			if (asset && asset.m_sSlotId == slotId)
				return asset;
		}
		return null;
	}

	protected int CountSchema52ManifestGroups(HST_ForceManifestState manifest, string elementId)
	{
		int count;
		if (!manifest || elementId.IsEmpty())
			return count;
		foreach (HST_ForceManifestGroupState group : manifest.m_aGroups)
		{
			if (group && group.m_sElementId == elementId)
				count++;
		}
		return count;
	}

	protected int CountSchema52ManifestVehicles(HST_ForceManifestState manifest, string slotId)
	{
		int count;
		if (!manifest || slotId.IsEmpty())
			return count;
		foreach (HST_ForceManifestVehicleState vehicle : manifest.m_aVehicles)
		{
			if (vehicle && vehicle.m_sSlotId == slotId)
				count++;
		}
		return count;
	}

	protected int CountSchema52ManifestMembers(HST_ForceManifestState manifest, string slotId)
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

	protected int CountSchema52ManifestMembersForGroup(HST_ForceManifestState manifest, string groupElementId)
	{
		int count;
		if (!manifest || groupElementId.IsEmpty())
			return count;
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (member && member.m_sGroupElementId == groupElementId)
				count++;
		}
		return count;
	}

	protected int CountSchema52ManifestAssets(HST_ForceManifestState manifest, string slotId)
	{
		int count;
		if (!manifest || slotId.IsEmpty())
			return count;
		foreach (HST_ForceManifestAssetState asset : manifest.m_aAssets)
		{
			if (asset && asset.m_sSlotId == slotId)
				count++;
		}
		return count;
	}

	protected string ValidateSchema52MissionConvoyRoute(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		if (mission.m_sSiteId.IsEmpty() || operation.m_sCurrentRouteId.IsEmpty()
			|| operation.m_sRouteContractHash.IsEmpty())
			return "exact mission convoy restore site or route identity is missing";
		HST_GeneratedSiteState site = FindSchema52MissionConvoySite(mission.m_sSiteId);
		HST_GeneratedRouteState route = FindSchema52MissionConvoyRoute(operation.m_sCurrentRouteId);
		if (!site || !route || CountSchema52MissionConvoySites(mission.m_sSiteId) != 1
			|| CountSchema52MissionConvoyRoutes(operation.m_sCurrentRouteId) != 1)
			return "exact mission convoy restore site or route identity is ambiguous";
		if (site.m_sRouteId != route.m_sRouteId || site.m_sZoneId != mission.m_sTargetZoneId
			|| route.m_sSourceZoneId.IsEmpty())
			return "exact mission convoy restore route source, target, site, or manifest backlinks conflict";
		if (route.m_sTargetZoneId != mission.m_sTargetZoneId
			|| manifest.m_sSourceZoneId != route.m_sSourceZoneId
			|| manifest.m_sTargetZoneId != route.m_sTargetZoneId)
			return "exact mission convoy restore route source, target, site, or manifest backlinks conflict";
		if (operation.m_sOriginZoneId != route.m_sSourceZoneId
			|| operation.m_sAssignmentZoneId != route.m_sTargetZoneId
			|| operation.m_sTacticalTargetZoneId != route.m_sTargetZoneId)
			return "exact mission convoy restore route source, target, site, or manifest backlinks conflict";
		if (!route.m_bRoadRoute || !route.m_bValidatedForVehicles || !route.m_sValidationFailureReason.IsEmpty())
			return "exact mission convoy restore route is not a validated vehicle road route";

		array<int> waypointIndexes = {};
		foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
		{
			if (!waypoint || waypoint.m_sRouteId != route.m_sRouteId || waypoint.m_iIndex < 0
				|| waypointIndexes.Contains(waypoint.m_iIndex))
				return "exact mission convoy restore route waypoint identity conflicts";
			waypointIndexes.Insert(waypoint.m_iIndex);
		}

		HST_MissionAssetState leadAsset = FindSchema52MissionAssetById(BuildSchema52MissionConvoyVehicleAssetId(mission, 0));
		ref array<vector> routePositions = BuildSchema52MissionConvoyRoutePositions(route, leadAsset);
		float routeDistance = CalculateSchema52MissionConvoyRouteDistance(routePositions);
		if (!leadAsset || routePositions.Count() < 2 || routeDistance <= HST_MissionConvoyOperationService.EXACT_ARRIVAL_RADIUS_METERS)
			return "exact mission convoy restore route geometry is incomplete or too short";
		string expectedRouteHash = HST_MissionConvoyOperationService.BuildRouteContractHash(route, routePositions);
		if (expectedRouteHash.IsEmpty() || operation.m_sRouteContractHash != expectedRouteHash)
			return "exact mission convoy restore frozen route contract hash conflicts";
		vector routeStart = routePositions[0];
		vector routeEnd = routePositions[routePositions.Count() - 1];
		if (DistanceSq2D(operation.m_vRouteStartPosition, routeStart) > 1.0
			|| DistanceSq2D(operation.m_vRouteEndPosition, routeEnd) > 1.0)
			return "exact mission convoy restore frozen route geometry conflicts";
		if (DistanceSq2D(operation.m_vOriginPosition, routeStart) > 1.0
			|| DistanceSq2D(operation.m_vAssignmentPosition, routeEnd) > 1.0)
			return "exact mission convoy restore frozen route geometry conflicts";
		if (DistanceSq2D(operation.m_vTacticalTargetPosition, routeEnd) > 1.0
			|| Math.AbsFloat(operation.m_fRouteTotalDistanceMeters - routeDistance) > 1.0)
			return "exact mission convoy restore frozen route geometry conflicts";
		if (route.m_iDistanceMeters > 0 && Math.AbsFloat(route.m_iDistanceMeters - routeDistance) > 2.0)
			return "exact mission convoy restore generated-route distance conflicts";
		if (route.m_iWaypointCount > 0 && route.m_iWaypointCount != route.m_aWaypoints.Count())
			return "exact mission convoy restore generated-route waypoint count conflicts";
		return "";
	}

	protected HST_GeneratedSiteState FindSchema52MissionConvoySite(string siteId)
	{
		HST_GeneratedSiteState match;
		foreach (HST_GeneratedSiteState site : m_SaveData.m_aGeneratedSites)
		{
			if (!site || site.m_sSiteId != siteId)
				continue;
			if (match)
				return null;
			match = site;
		}
		return match;
	}

	protected HST_GeneratedRouteState FindSchema52MissionConvoyRoute(string routeId)
	{
		HST_GeneratedRouteState match;
		foreach (HST_GeneratedRouteState route : m_SaveData.m_aGeneratedRoutes)
		{
			if (!route || route.m_sRouteId != routeId)
				continue;
			if (match)
				return null;
			match = route;
		}
		return match;
	}

	protected int CountSchema52MissionConvoySites(string siteId)
	{
		int count;
		foreach (HST_GeneratedSiteState site : m_SaveData.m_aGeneratedSites)
		{
			if (site && site.m_sSiteId == siteId)
				count++;
		}
		return count;
	}

	protected int CountSchema52MissionConvoyRoutes(string routeId)
	{
		int count;
		foreach (HST_GeneratedRouteState route : m_SaveData.m_aGeneratedRoutes)
		{
			if (route && route.m_sRouteId == routeId)
				count++;
		}
		return count;
	}

	protected ref array<vector> BuildSchema52MissionConvoyRoutePositions(
		HST_GeneratedRouteState route,
		HST_MissionAssetState leadAsset)
	{
		ref array<vector> positions = {};
		if (leadAsset && !IsZeroVector(leadAsset.m_vSourcePosition))
			AppendSchema52MissionConvoyRoutePosition(positions, leadAsset.m_vSourcePosition);
		if (route)
		{
			int lastIndex = -2147483647;
			while (true)
			{
				HST_RouteWaypointState selected;
				int selectedIndex = 2147483647;
				foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
				{
					if (!waypoint || waypoint.m_iIndex <= lastIndex || waypoint.m_iIndex >= selectedIndex)
						continue;
					selected = waypoint;
					selectedIndex = waypoint.m_iIndex;
				}
				if (!selected)
					break;
				AppendSchema52MissionConvoyRoutePosition(positions, selected.m_vPosition);
				lastIndex = selectedIndex;
			}
			AppendSchema52MissionConvoyRoutePosition(positions, route.m_vEndPosition);
		}
		if (leadAsset)
			AppendSchema52MissionConvoyRoutePosition(positions, leadAsset.m_vTargetPosition);
		return positions;
	}

	protected void AppendSchema52MissionConvoyRoutePosition(array<vector> positions, vector position)
	{
		if (!positions || IsZeroVector(position))
			return;
		if (positions.Count() > 0 && DistanceSq2D(positions[positions.Count() - 1], position) <= 1.0)
			return;
		positions.Insert(position);
	}

	protected float CalculateSchema52MissionConvoyRouteDistance(array<vector> positions)
	{
		float distance;
		if (!positions)
			return distance;
		for (int i = 1; i < positions.Count(); i++)
			distance += Math.Sqrt(DistanceSq2D(positions[i - 1], positions[i]));
		return distance;
	}

	protected string ValidateSchema52MissionConvoyProjection(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch)
	{
		if (operation.m_sAssignmentKind != HST_MissionConvoyOperationService.EXACT_ASSIGNMENT_KIND
			|| operation.m_sRecallPolicyId != HST_MissionConvoyOperationService.EXACT_RECALL_POLICY
			|| operation.m_sSettlementPolicyId != HST_MissionConvoyOperationService.EXACT_SETTLEMENT_POLICY)
			return "exact mission convoy restore operation policy conflicts";
		if (operation.m_iProjectionContractVersion != HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION
			|| operation.m_iRouteVersion != HST_MissionConvoyOperationService.EXACT_ROUTE_VERSION)
			return "exact mission convoy restore route cursor or speed conflicts";
		if (Math.AbsFloat(operation.m_fStrategicSpeedMetersPerSecond - HST_MissionConvoyOperationService.EXACT_SPEED_METERS_PER_SECOND) > 0.01
			|| operation.m_fRouteTotalDistanceMeters <= HST_MissionConvoyOperationService.EXACT_ARRIVAL_RADIUS_METERS)
			return "exact mission convoy restore route cursor or speed conflicts";
		if (operation.m_fRouteProgressMeters < 0.0
			|| operation.m_fRouteProgressMeters > operation.m_fRouteTotalDistanceMeters + 0.1)
			return "exact mission convoy restore route cursor or speed conflicts";
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_UNKNOWN
			|| operation.m_eResumeDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_UNKNOWN
			|| operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_UNKNOWN)
			return "exact mission convoy restore operation state is incomplete";
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_UNKNOWN
			|| operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_UNKNOWN
			|| operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_UNKNOWN)
			return "exact mission convoy restore operation state is incomplete";
		if (operation.m_iRevision <= 0)
			return "exact mission convoy restore operation state is incomplete";
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& !HST_MissionConvoyP1Policy.IsOpenDutyPair(operation.m_eDutyState, operation.m_eResumeDutyState))
			return "open exact mission convoy restore duty and resume states are illegal";
		if (!HST_MissionConvoyP1Policy.IsProjectionPair(
			operation.m_eSettlementState,
			operation.m_eMaterializationState,
			operation.m_ePositionAuthority))
			return "exact mission convoy restore materialization and position authority cross-product is illegal";
		if (batch.m_iExpectedSlotCount != batch.m_aSlotResults.Count() || batch.m_iLifecycleRevision < 0)
			return "exact mission convoy restore roster projection counters conflict";
		if (batch.m_iCreatedAtSecond < 0 || batch.m_iUpdatedAtSecond < batch.m_iCreatedAtSecond)
			return "exact mission convoy restore roster lifecycle timestamps conflict";
		return "";
	}

	protected string ValidateSchema52MissionConvoyBatch(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		bool openAuthority = operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		if (openAuthority && (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			|| !batch.m_bStrategicProjectionHeld || batch.m_bCancelRequested))
			return "open exact mission convoy restore roster is not pending and strategically held";
		int expectedSlots = manifest.m_aGroups.Count() + manifest.m_aVehicles.Count()
			+ manifest.m_aMembers.Count() + manifest.m_aAssets.Count();
		if (batch.m_iExpectedSlotCount != expectedSlots || batch.m_aSlotResults.Count() != expectedSlots)
			return "exact mission convoy restore held roster slot count conflicts";

		foreach (HST_ForceManifestGroupState group : manifest.m_aGroups)
		{
			string failure = ValidateSchema52MissionConvoyBatchSlot(batch, group.m_sElementId, "group", group.m_sPrefab);
			if (!failure.IsEmpty())
				return failure;
		}
		foreach (HST_ForceManifestVehicleState vehicle : manifest.m_aVehicles)
		{
			string failure = ValidateSchema52MissionConvoyBatchSlot(batch, vehicle.m_sSlotId, "vehicle", vehicle.m_sPrefab);
			if (!failure.IsEmpty())
				return failure;
		}
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			string failure = ValidateSchema52MissionConvoyBatchSlot(batch, member.m_sSlotId, "member", member.m_sPrefab);
			if (!failure.IsEmpty())
				return failure;
		}
		foreach (HST_ForceManifestAssetState asset : manifest.m_aAssets)
		{
			string failure = ValidateSchema52MissionConvoyBatchSlot(batch, asset.m_sSlotId, "asset", asset.m_sPrefab);
			if (!failure.IsEmpty())
				return failure;
		}

		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_sSlotId.IsEmpty() || CountSchema52BatchSlots(batch, slot.m_sSlotId) != 1)
				return "exact mission convoy restore roster slot identity is missing or ambiguous";
			bool known = FindSchema52ManifestGroup(manifest, slot.m_sSlotId)
				|| FindSchema52ManifestVehicle(manifest, slot.m_sSlotId)
				|| manifest.FindMemberSlot(slot.m_sSlotId)
				|| FindSchema52ManifestAsset(manifest, slot.m_sSlotId);
			if (!known)
				return "exact mission convoy restore roster contains a slot absent from the frozen manifest";
			if (slot.m_sSlotKind == "member")
			{
				if (!slot.m_bEverAlive)
					return "exact mission convoy restore member roster contains never-living authority";
				if (slot.m_bCasualtyConfirmed
					&& (slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED || slot.m_bAliveVerified))
					return "exact mission convoy restore casualty slot conflicts";
				if (openAuthority && !slot.m_bCasualtyConfirmed
					&& (slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED || !slot.m_bAliveVerified))
					return "open exact mission convoy restore living member is not registered and alive-authorized";
				if (!openAuthority && slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED)
					return "settled exact mission convoy restore roster contains a non-retired slot";
				continue;
			}

			// Casualty tombstones belong only to concrete member slots. Group,
			// vehicle, and cargo roots remain non-casualty lifecycle rows even when
			// their convoy element or mission asset becomes terminal.
			if (slot.m_bCasualtyConfirmed)
				return "exact mission convoy restore non-member root contains casualty authority";
			if (openAuthority && slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED)
				return "open exact mission convoy restore root is retired or unregistered";
			if (openAuthority && slot.m_sSlotKind == "group"
				&& (slot.m_bAliveVerified || slot.m_bEverAlive))
				return "open exact mission convoy restore group root contains character-alive authority";
			if (openAuthority && (slot.m_sSlotKind == "vehicle" || slot.m_sSlotKind == "asset")
				&& (!slot.m_bAliveVerified || !slot.m_bEverAlive))
				return "open exact mission convoy restore vehicle or asset root lacks alive lifecycle authority";
			if (!openAuthority && slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED)
				return "settled exact mission convoy restore roster contains a non-retired slot";
		}
		return "";
	}

	protected string ValidateSchema52MissionConvoyBatchSlot(
		HST_ForceSpawnResultState batch,
		string slotId,
		string slotKind,
		string prefab)
	{
		HST_ForceSpawnSlotResultState slot = FindSchema52BatchSlot(batch, slotId);
		if (!slot || CountSchema52BatchSlots(batch, slotId) != 1)
			return "exact mission convoy restore roster slot is missing or ambiguous";
		if (slot.m_sSlotKind != slotKind || slot.m_sSpawnedPrefab != prefab)
			return "exact mission convoy restore roster slot kind or frozen prefab conflicts";
		return "";
	}

	protected HST_ForceSpawnSlotResultState FindSchema52BatchSlot(HST_ForceSpawnResultState batch, string slotId)
	{
		if (!batch || slotId.IsEmpty())
			return null;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotId == slotId)
				return slot;
		}
		return null;
	}

	protected int CountSchema52BatchSlots(HST_ForceSpawnResultState batch, string slotId)
	{
		int count;
		if (!batch || slotId.IsEmpty())
			return count;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotId == slotId)
				count++;
		}
		return count;
	}

	protected string ValidateSchema52MissionConvoyElements(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		if (CountSchema52MissionConvoyElementClaimants(mission, operation, manifest)
			!= HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT)
			return "exact mission convoy restore element ownership is incomplete or ambiguous";
		if (CountSchema52MissionConvoyGroupClaimants(mission, operation, manifest, batch)
			!= HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT)
			return "exact mission convoy restore group ownership is incomplete or ambiguous";
		int expectedAssetCount = HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT + manifest.m_aAssets.Count();
		if (CountSchema52MissionConvoyAssetClaimants(mission, operation, manifest) != expectedAssetCount)
			return "exact mission convoy restore vehicle or payload asset ownership is incomplete or ambiguous";

		int totalSurvivors;
		for (int ordinal = 0; ordinal < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; ordinal++)
		{
			string elementId = HST_MissionConvoyOperationService.BuildElementId(mission, ordinal);
			string groupId = HST_MissionConvoyOperationService.BuildGroupId(mission, ordinal);
			string vehicleSlotId = HST_MissionConvoyOperationService.BuildVehicleSlotId(mission, ordinal);
			string groupElementId = HST_MissionConvoyOperationService.BuildCrewGroupElementId(mission, ordinal);
			string vehicleAssetId = BuildSchema52MissionConvoyVehicleAssetId(mission, ordinal);
			HST_ConvoyElementState element = FindSchema52MissionConvoyElement(elementId);
			HST_ActiveGroupState group = FindSchema52MissionConvoyGroup(groupId);
			HST_MissionAssetState vehicleAsset = FindSchema52MissionAssetById(vehicleAssetId);
			HST_ForceManifestGroupState groupSlot = FindSchema52ManifestGroup(manifest, groupElementId);
			HST_ForceManifestVehicleState vehicleSlot = FindSchema52ManifestVehicle(manifest, vehicleSlotId);
			if (!element || !group || !vehicleAsset)
				return string.Format("exact mission convoy restore rooted element %1 is missing or ambiguous", ordinal);
			if (!groupSlot || !vehicleSlot || CountSchema52MissionConvoyElements(elementId) != 1)
				return string.Format("exact mission convoy restore rooted element %1 is missing or ambiguous", ordinal);
			if (CountSchema52MissionConvoyGroups(groupId, elementId) != 1
				|| CountSchema52MissionConvoyVehicleAssets(vehicleAssetId, elementId) != 1)
				return string.Format("exact mission convoy restore rooted element %1 is missing or ambiguous", ordinal);

			if (element.m_iOrdinal != ordinal || element.m_sOperationId != operation.m_sOperationId
				|| element.m_sMissionInstanceId != mission.m_sInstanceId)
				return string.Format("exact mission convoy restore rooted element %1 backlinks conflict", ordinal);
			if (element.m_sManifestId != manifest.m_sManifestId
				|| element.m_sVehicleSlotId != vehicleSlot.m_sSlotId
				|| element.m_sCrewGroupElementId != groupSlot.m_sElementId)
				return string.Format("exact mission convoy restore rooted element %1 backlinks conflict", ordinal);
			if (element.m_sVehicleAssetId != vehicleAsset.m_sAssetId
				|| element.m_sGroupId != group.m_sGroupId)
				return string.Format("exact mission convoy restore rooted element %1 backlinks conflict", ordinal);
			if (element.m_sVehiclePrefab != vehicleSlot.m_sPrefab || element.m_sVehiclePrefab != vehicleAsset.m_sPrefab
				|| element.m_sCrewGroupPrefab != groupSlot.m_sPrefab || element.m_sCrewGroupPrefab != group.m_sPrefab)
				return string.Format("exact mission convoy restore rooted element %1 frozen prefabs conflict", ordinal);
			if (group.m_sOperationId != operation.m_sOperationId
				|| group.m_sManifestId != manifest.m_sManifestId
				|| group.m_sSpawnResultId != batch.m_sResultId)
				return string.Format("exact mission convoy restore crew group %1 backlinks conflict", ordinal);
			if (group.m_sForceId != batch.m_sForceId || group.m_sProjectionId != batch.m_sProjectionId
				|| group.m_sMissionInstanceId != mission.m_sInstanceId)
				return string.Format("exact mission convoy restore crew group %1 backlinks conflict", ordinal);
			if (group.m_sConvoyElementId != element.m_sElementId
				|| group.m_sMissionAssetId != vehicleAsset.m_sAssetId
				|| group.m_sRouteId != operation.m_sCurrentRouteId)
				return string.Format("exact mission convoy restore crew group %1 backlinks conflict", ordinal);
			if (group.m_sFactionKey != manifest.m_sFactionKey)
				return string.Format("exact mission convoy restore crew group %1 backlinks conflict", ordinal);
			if (vehicleAsset.m_sMissionInstanceId != mission.m_sInstanceId
				|| vehicleAsset.m_sOperationId != operation.m_sOperationId
				|| vehicleAsset.m_sManifestId != manifest.m_sManifestId)
				return string.Format("exact mission convoy restore vehicle asset %1 backlinks conflict", ordinal);
			if (vehicleAsset.m_sManifestSlotId != vehicleSlot.m_sSlotId
				|| vehicleAsset.m_sAssignedVehicleSlotId != vehicleSlot.m_sSlotId
				|| vehicleAsset.m_sConvoyElementId != element.m_sElementId)
				return string.Format("exact mission convoy restore vehicle asset %1 backlinks conflict", ordinal);
			if (vehicleAsset.m_sRole != HST_MissionConvoyOperationService.VEHICLE_ROLE)
				return string.Format("exact mission convoy restore vehicle asset %1 backlinks conflict", ordinal);

			if (element.m_iOriginalCrewCount != groupSlot.m_iExpectedMemberCount
				|| element.m_iSurvivingCrewCount < 0
				|| element.m_iSurvivingCrewCount > element.m_iOriginalCrewCount)
				return string.Format("exact mission convoy restore crew survivor authority %1 conflicts", ordinal);
			// Exact member slots plus the convoy element own durable casualties. The
			// active-group counters are a physical projection snapshot and can lag at
			// a save boundary; they are rebound only after every reciprocal identity
			// and roster invariant above has validated uniquely.
			if (group.m_iOriginalInfantryCount != element.m_iOriginalCrewCount)
				return string.Format("exact mission convoy restore crew survivor authority %1 conflicts", ordinal);
			if (CountSchema52LivingMemberSlotsForGroup(manifest, batch, groupElementId)
				!= element.m_iSurvivingCrewCount)
				return string.Format("exact mission convoy restore durable member slots for element %1 conflict", ordinal);
			if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_UNKNOWN
				|| element.m_iRevision <= 0)
				return string.Format("exact mission convoy restore element %1 disposition or vehicle state conflicts", ordinal);
			if (element.m_fVehicleDamageFraction < 0.0 || element.m_fVehicleDamageFraction > 1.0
				|| element.m_fFuelFraction < 0.0 || element.m_fFuelFraction > 1.0)
				return string.Format("exact mission convoy restore element %1 disposition or vehicle state conflicts", ordinal);
			if (element.m_fAmmoFraction < 0.0 || element.m_fAmmoFraction > 1.0)
				return string.Format("exact mission convoy restore element %1 disposition or vehicle state conflicts", ordinal);
			string dispositionFailure = HST_MissionConvoyOperationService.ValidateElementDispositionAssetState(
				element,
				vehicleAsset,
				operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN);
			if (!dispositionFailure.IsEmpty())
				return string.Format("exact mission convoy restore element %1 %2", ordinal, dispositionFailure);
			if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
				&& (!element.m_bMobile || element.m_iSurvivingCrewCount <= 0))
				return string.Format("exact mission convoy restore element %1 is an active ghost", ordinal);
			if (element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
				&& element.m_bMobile)
				return string.Format("exact mission convoy restore terminal element %1 remains mobile", ordinal);
			bool processStateMayOwnPhysicalRoot = operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRING;
			if (element.m_bPhysicalized && !processStateMayOwnPhysicalRoot)
				return string.Format("exact mission convoy restore element %1 physical flag conflicts", ordinal);
			if (Math.AbsFloat(element.m_vFormationOffset[0]) > 0.1
				|| Math.AbsFloat(element.m_vFormationOffset[2] + HST_MissionConvoyOperationService.EXACT_FORMATION_SPACING_METERS * ordinal) > 0.1)
				return string.Format("exact mission convoy restore element %1 formation offset conflicts", ordinal);
			totalSurvivors += element.m_iSurvivingCrewCount;
		}

		if (CountSchema52LivingMemberSlots(batch) != totalSurvivors)
			return "exact mission convoy restore aggregate survivor count conflicts with durable roster";
		return ValidateSchema52MissionConvoyCargo(mission, operation, manifest);
	}

	protected string ValidateSchema52MissionConvoyCargo(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		HST_MissionAssetState contractCargo;
		int cargoRowCount;
		foreach (HST_MissionAssetState candidate : m_SaveData.m_aMissionAssets)
		{
			if (!candidate || candidate.m_sMissionInstanceId != mission.m_sInstanceId
				|| candidate.m_sRole == HST_MissionConvoyOperationService.VEHICLE_ROLE)
				continue;
			contractCargo = candidate;
			cargoRowCount++;
		}
		string cargoContractFailure = HST_MissionConvoyP1Policy.ValidateCargoContract(
			mission,
			contractCargo,
			cargoRowCount);
		if (!cargoContractFailure.IsEmpty())
			return "exact mission convoy restore cargo contract conflicts: " + cargoContractFailure;

		HST_ConvoyElementState lead = FindSchema52MissionConvoyElement(
			HST_MissionConvoyOperationService.BuildElementId(mission, 0));
		if (!lead)
			return "exact mission convoy restore lead element is missing";
		for (int ordinal = 1; ordinal < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; ordinal++)
		{
			HST_ConvoyElementState element = FindSchema52MissionConvoyElement(
				HST_MissionConvoyOperationService.BuildElementId(mission, ordinal));
			if (element && !element.m_sCargoAssetId.IsEmpty())
				return "exact mission convoy restore payload is assigned to more than the lead element";
		}
		if (manifest.m_aAssets.Count() == 0)
		{
			if (!lead.m_sCargoAssetId.IsEmpty())
				return "exact mission convoy restore lead element claims cargo absent from the manifest";
			return "";
		}

		HST_ForceManifestAssetState assetSlot = manifest.m_aAssets[0];
		HST_MissionAssetState cargoAsset = FindSchema52MissionAssetByManifestSlot(assetSlot.m_sSlotId);
		if (!cargoAsset || cargoAsset != contractCargo
			|| CountSchema52MissionAssetsByManifestSlot(assetSlot.m_sSlotId) != 1)
			return "exact mission convoy restore payload asset identity is missing or ambiguous";
		if (lead.m_sCargoAssetId != cargoAsset.m_sAssetId
			|| cargoAsset.m_sMissionInstanceId != mission.m_sInstanceId
			|| cargoAsset.m_sOperationId != operation.m_sOperationId)
			return "exact mission convoy restore payload backlinks conflict";
		if (cargoAsset.m_sManifestId != manifest.m_sManifestId
			|| cargoAsset.m_sManifestSlotId != assetSlot.m_sSlotId
			|| cargoAsset.m_sAssignedVehicleSlotId != assetSlot.m_sAssignedVehicleSlotId)
			return "exact mission convoy restore payload backlinks conflict";
		if (cargoAsset.m_sConvoyElementId != lead.m_sElementId
			|| cargoAsset.m_sRole != assetSlot.m_sRole || cargoAsset.m_sKind != assetSlot.m_sKind
			|| cargoAsset.m_sPrefab != assetSlot.m_sPrefab)
			return "exact mission convoy restore payload backlinks conflict";
		return "";
	}

	protected HST_ConvoyElementState FindSchema52MissionConvoyElement(string elementId)
	{
		HST_ConvoyElementState match;
		foreach (HST_ConvoyElementState element : m_SaveData.m_aConvoyElements)
		{
			if (!element || element.m_sElementId != elementId)
				continue;
			if (match)
				return null;
			match = element;
		}
		return match;
	}

	protected HST_ActiveGroupState FindSchema52MissionConvoyGroup(string groupId)
	{
		HST_ActiveGroupState match;
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

	protected HST_MissionAssetState FindSchema52MissionAssetById(string assetId)
	{
		HST_MissionAssetState match;
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

	protected HST_MissionAssetState FindSchema52MissionAssetByManifestSlot(string slotId)
	{
		HST_MissionAssetState match;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!asset || asset.m_sManifestSlotId != slotId)
				continue;
			if (match)
				return null;
			match = asset;
		}
		return match;
	}

	protected string BuildSchema52MissionConvoyVehicleAssetId(HST_ActiveMissionState mission, int ordinal)
	{
		if (!mission)
			return "";
		return string.Format("asset_%1_%2_%3", mission.m_sInstanceId, HST_MissionConvoyOperationService.VEHICLE_ROLE, ordinal);
	}

	protected int CountSchema52MissionConvoyElementClaimants(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		int count;
		foreach (HST_ConvoyElementState element : m_SaveData.m_aConvoyElements)
		{
			if (!element)
				continue;
			if (element.m_sMissionInstanceId == mission.m_sInstanceId
				|| element.m_sOperationId == operation.m_sOperationId
				|| element.m_sManifestId == manifest.m_sManifestId)
				count++;
		}
		return count;
	}

	protected int CountSchema52MissionConvoyGroupClaimants(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		int count;
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!group)
				continue;
			bool matches = group.m_sMissionInstanceId == mission.m_sInstanceId
				|| group.m_sOperationId == operation.m_sOperationId;
			if (!matches)
				matches = group.m_sManifestId == manifest.m_sManifestId
					|| group.m_sSpawnResultId == batch.m_sResultId;
			if (!matches)
				matches = group.m_sForceId == batch.m_sForceId
					|| group.m_sProjectionId == batch.m_sProjectionId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountSchema52MissionConvoyAssetClaimants(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		int count;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!asset)
				continue;
			bool convoyRole = asset.m_sRole == HST_MissionConvoyOperationService.VEHICLE_ROLE
				|| asset.m_sRole == HST_MissionConvoyOperationService.PAYLOAD_ROLE
				|| asset.m_sRole == HST_MissionConvoyOperationService.CAPTIVE_ROLE;
			bool matches = convoyRole && asset.m_sMissionInstanceId == mission.m_sInstanceId;
			if (!matches)
				matches = asset.m_sOperationId == operation.m_sOperationId
					|| asset.m_sManifestId == manifest.m_sManifestId;
			if (!matches && !asset.m_sConvoyElementId.IsEmpty())
				matches = asset.m_sConvoyElementId.StartsWith("convoy_element_" + mission.m_sInstanceId + "_");
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountSchema52MissionConvoyElements(string elementId)
	{
		int count;
		foreach (HST_ConvoyElementState element : m_SaveData.m_aConvoyElements)
		{
			if (element && element.m_sElementId == elementId)
				count++;
		}
		return count;
	}

	protected int CountSchema52MissionConvoyGroups(string groupId, string elementId)
	{
		int count;
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (group && (group.m_sGroupId == groupId || group.m_sConvoyElementId == elementId))
				count++;
		}
		return count;
	}

	protected int CountSchema52MissionConvoyVehicleAssets(string assetId, string elementId)
	{
		int count;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!asset || asset.m_sRole != HST_MissionConvoyOperationService.VEHICLE_ROLE)
				continue;
			if (asset.m_sAssetId == assetId || asset.m_sConvoyElementId == elementId)
				count++;
		}
		return count;
	}

	protected int CountSchema52MissionAssetsByManifestSlot(string slotId)
	{
		int count;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (asset && asset.m_sManifestSlotId == slotId)
				count++;
		}
		return count;
	}

	protected int CountSchema52LivingMemberSlotsForGroup(
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		string groupElementId)
	{
		int count;
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member || member.m_sGroupElementId != groupElementId)
				continue;
			HST_ForceSpawnSlotResultState slot = FindSchema52BatchSlot(batch, member.m_sSlotId);
			if (slot && slot.m_bEverAlive && !slot.m_bCasualtyConfirmed)
				count++;
		}
		return count;
	}

	protected int CountSchema52LivingMemberSlots(HST_ForceSpawnResultState batch)
	{
		int count;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotKind == "member" && slot.m_bEverAlive && !slot.m_bCasualtyConfirmed)
				count++;
		}
		return count;
	}

	protected string ValidateSchema52MissionConvoySettlement(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch)
	{
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			if (!HST_MissionConvoyP1Policy.IsOpenDutyPair(operation.m_eDutyState, operation.m_eResumeDutyState))
				return "open exact mission convoy restore duty and resume states are illegal";
			if (!HST_MissionConvoyP1Policy.IsProjectionPair(
				operation.m_eSettlementState,
				operation.m_eMaterializationState,
				operation.m_ePositionAuthority))
				return "open exact mission convoy restore projection state is illegal";
			if (operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
				|| !operation.m_sSettlementId.IsEmpty() || !mission.m_sSettlementId.IsEmpty())
				return "open exact mission convoy restore contains terminal or cancelled authority";
			if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
				|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
				return "open exact mission convoy restore contains terminal or cancelled authority";
			return "";
		}
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return "exact mission convoy restore settlement state conflicts";
		string settlementPrefix = "settlement_" + operation.m_sOperationId + "_";
		if (operation.m_sSettlementId.IsEmpty() || !operation.m_sSettlementId.StartsWith(settlementPrefix)
			|| mission.m_sSettlementId != operation.m_sSettlementId)
			return "settled exact mission convoy restore receipt conflicts";
		if (operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN
			|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
			return "settled exact mission convoy restore receipt conflicts";
		string arrivedSettlementId = HST_OperationService.BuildSettlementId(operation.m_sOperationId, "arrived");
		string crewEliminatedSettlementId = HST_OperationService.BuildSettlementId(operation.m_sOperationId, "crew_eliminated");
		string missionSucceededSettlementId = HST_OperationService.BuildSettlementId(operation.m_sOperationId, "mission_succeeded");
		string expiredSettlementId = HST_OperationService.BuildSettlementId(operation.m_sOperationId, "expired");
		string campaignStoppedSettlementId = HST_OperationService.BuildSettlementId(operation.m_sOperationId, "campaign_stopped");
		string failedSettlementId = HST_OperationService.BuildSettlementId(operation.m_sOperationId, "failed");
		if (operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED)
		{
			if (operation.m_sSettlementId != arrivedSettlementId || !mission.m_bConvoyArrivalOutcomeApplied
				|| !HasSchema52SettledArrivalEvidence(mission, operation))
				return "settled exact mission convoy restore arrival receipt or outcome evidence conflicts";
		}
		else if (operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED)
		{
			bool crewEliminatedReceipt = operation.m_sSettlementId == crewEliminatedSettlementId
				&& mission.m_bConvoyCrewEliminatedOutcomeApplied;
			bool missionSucceededReceipt = operation.m_sSettlementId == missionSucceededSettlementId
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED;
			if (!crewEliminatedReceipt && !missionSucceededReceipt)
				return "settled exact mission convoy restore destroyed receipt or outcome evidence conflicts";
		}
		else if (operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED)
		{
			if (operation.m_sSettlementId != expiredSettlementId && operation.m_sSettlementId != campaignStoppedSettlementId)
				return "settled exact mission convoy restore cancellation receipt conflicts";
			if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_EXPIRED && !mission.m_bConvoyExpiredOutcomeApplied)
				return "settled exact mission convoy restore cancellation outcome evidence conflicts";
		}
		else if (operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED)
		{
			if (operation.m_sSettlementId != failedSettlementId
				|| (mission.m_sRuntimePhase != SCHEMA52_CONVOY_FAILURE_PHASE && mission.m_eStatus != HST_EMissionStatus.HST_MISSION_FAILED)
				|| mission.m_bConvoyArrivalOutcomeApplied)
				return "settled exact mission convoy restore route-failure receipt or outcome evidence conflicts";
		}
		else
			return "settled exact mission convoy restore terminal result is unsupported by the exact contract";
		if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			|| operation.m_eResumeDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			|| operation.m_iSettledAtSecond < operation.m_iCreatedAtSecond)
			return "settled exact mission convoy restore receipt conflicts";
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRING
			&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
			return "settled exact mission convoy restore materialization state conflicts";
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
			&& operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return "settled exact mission convoy restore retired position authority conflicts";
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED
			|| batch.m_bStrategicProjectionHeld)
			return "settled exact mission convoy restore roster receipt conflicts";
		if (batch.m_iCompletedAtSecond < batch.m_iCreatedAtSecond
			|| batch.m_iUpdatedAtSecond < batch.m_iCreatedAtSecond)
			return "settled exact mission convoy restore roster receipt conflicts";
		foreach (HST_ConvoyElementState element : m_SaveData.m_aConvoyElements)
		{
			if (!element || element.m_sOperationId != operation.m_sOperationId)
				continue;
			if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
				|| element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_UNKNOWN
				|| element.m_bMobile)
				return "settled exact mission convoy restore contains a nonterminal element";
		}
		return "";
	}

	protected bool HasSchema52SettledArrivalEvidence(
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation)
	{
		if (!mission || !operation)
			return false;
		bool missionReceiptValid = mission.m_sRuntimePhase == SCHEMA52_CONVOY_FAILURE_PHASE
			&& mission.m_sLastRuntimeEventKey == SCHEMA52_CONVOY_FAILURE_EVENT
			&& mission.m_sRuntimeFailureReason.Contains("Convoy reached its destination:");
		bool operationReceiptValid = operation.m_sTerminalReason == "convoy reached destination with living crew"
			&& !IsZeroVector(operation.m_vRouteEndPosition)
			&& operation.m_fRouteTotalDistanceMeters > 0.0;
		bool routeProgressValid = operation.m_fRouteProgressMeters
			>= operation.m_fRouteTotalDistanceMeters - HST_MissionConvoyOperationService.EXACT_ARRIVAL_RADIUS_METERS;
		if (!missionReceiptValid || !operationReceiptValid || !routeProgressValid)
			return false;

		int arrivedElements;
		foreach (HST_ConvoyElementState element : m_SaveData.m_aConvoyElements)
		{
			if (!element || element.m_sOperationId != operation.m_sOperationId)
				continue;
			if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ARRIVED)
				arrivedElements++;
		}
		return arrivedElements > 0;
	}

	protected bool HasCampaignEventId(string eventId)
	{
		foreach (HST_CampaignEventState eventState : m_SaveData.m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				return true;
		}
		return false;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}
}
