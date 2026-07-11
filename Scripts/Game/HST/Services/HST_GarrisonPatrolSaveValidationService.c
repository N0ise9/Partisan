// Schema-54 restore boundary for newly purchased exact garrison patrols.
// Policy-v1 garrison purchases remain aggregate-only; no exact roster,
// operation, projection, local route, casualty, or settlement identity is
// inferred for historical saves.
class HST_GarrisonPatrolSaveValidationService
{
	protected HST_CampaignSaveData m_SaveData;

	void Normalize(HST_CampaignSaveData saveData, int restoredSchemaVersion)
	{
		m_SaveData = saveData;
		if (!m_SaveData)
			return;
		if (restoredSchemaVersion < 54)
		{
			PreserveHistoricalGarrisons();
			m_SaveData = null;
			return;
		}

		array<string> validatedOperationIds = {};
		int validatedCount;
		int quarantinedCount;
		foreach (HST_ForceQuoteState quote : m_SaveData.m_aForceQuotes)
		{
			if (!IsAcceptedCurrentPolicyQuote(quote))
				continue;
			HST_ForceManifestState manifest = FindUniqueManifest(quote.m_sManifestId);
			HST_OperationRecordState operation = FindUniqueOperation(quote.m_sOperationId);
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			HST_GeneratedRouteState route;
			if (operation)
			{
				batch = FindUniqueBatch(operation.m_sSpawnResultId);
				group = FindUniqueGroup(operation.m_sGroupId);
				if (!operation.m_sCurrentRouteId.IsEmpty())
					route = FindUniqueRoute(operation.m_sCurrentRouteId);
			}
			HST_GarrisonState garrison = FindUniqueGarrison(quote.m_sTargetZoneId, quote.m_sFactionKey);
			string failure = ValidateAggregate(quote, manifest, operation, batch, group, garrison, route);
			if (!failure.IsEmpty())
			{
				QuarantineAggregate(quote, operation, batch, group, failure);
				quarantinedCount++;
				continue;
			}
			NormalizeValidAggregate(operation, batch, group);
			validatedOperationIds.Insert(operation.m_sOperationId);
			validatedCount++;
		}

		quarantinedCount += PreserveOrphanClaimants(validatedOperationIds);
		if (quarantinedCount > 0 && !HasEvent("normalization_schema54_exact_garrison_patrol_conflict"))
		{
			HST_CampaignEventState eventState = new HST_CampaignEventState();
			eventState.m_sEventId = "normalization_schema54_exact_garrison_patrol_conflict";
			eventState.m_sCategory = "normalization";
			eventState.m_sAggregateType = "garrison_patrol_authority";
			eventState.m_sAggregateId = "schema54";
			eventState.m_sTransition = "corrupt_exact_garrison_patrols_quarantined";
			eventState.m_sReason = string.Format(
				"validated %1 exact purchased-garrison patrol authorities and quarantined %2 incomplete, conflicting, unsupported, or non-unique graphs without converting them to legacy garrison counts",
				validatedCount,
				quarantinedCount);
			eventState.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
			m_SaveData.m_aCampaignEvents.Insert(eventState);
		}
		m_SaveData = null;
	}

	static bool IsSchema54GarrisonPatrolOperationClaimant(
		HST_CampaignSaveData saveData,
		HST_OperationRecordState operation)
	{
		return saveData && operation
			&& operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL;
	}

	static bool IsSchema54GarrisonPatrolQuoteClaimant(
		HST_CampaignSaveData saveData,
		HST_ForceQuoteState quote)
	{
		if (!saveData || !quote)
			return false;
		if (quote.m_sQuoteKind == HST_ForcePlanningService.QUOTE_KIND_GARRISON
			&& quote.m_sPolicyId == HST_GarrisonPatrolOperationService.EXACT_POLICY_ID)
			return true;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!IsSchema54GarrisonPatrolOperationClaimant(saveData, operation))
				continue;
			if ((!quote.m_sOperationId.IsEmpty() && quote.m_sOperationId == operation.m_sOperationId)
				|| (!quote.m_sQuoteId.IsEmpty() && quote.m_sQuoteId == operation.m_sQuoteId))
				return true;
		}
		return false;
	}

	static bool IsSchema54GarrisonPatrolManifestClaimant(
		HST_CampaignSaveData saveData,
		HST_ForceManifestState manifest)
	{
		if (!saveData || !manifest)
			return false;
		if (manifest.m_sPolicyId == HST_GarrisonPatrolOperationService.EXACT_POLICY_ID)
			return true;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!IsSchema54GarrisonPatrolOperationClaimant(saveData, operation))
				continue;
			if ((!manifest.m_sManifestId.IsEmpty() && manifest.m_sManifestId == operation.m_sManifestId)
				|| (!manifest.m_sOperationId.IsEmpty() && manifest.m_sOperationId == operation.m_sOperationId))
				return true;
		}
		return false;
	}

	static bool IsSchema54GarrisonPatrolBatchClaimant(
		HST_CampaignSaveData saveData,
		HST_ForceSpawnResultState batch)
	{
		if (!saveData || !batch)
			return false;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!IsSchema54GarrisonPatrolOperationClaimant(saveData, operation))
				continue;
			if (!operation.m_sOperationId.IsEmpty()
				&& batch.m_sOperationId == operation.m_sOperationId
				&& ((!batch.m_sResultId.IsEmpty() && batch.m_sResultId == operation.m_sSpawnResultId)
					|| (!batch.m_sManifestId.IsEmpty() && batch.m_sManifestId == operation.m_sManifestId)
					|| (!batch.m_sForceId.IsEmpty() && batch.m_sForceId == operation.m_sForceId)
					|| (!batch.m_sProjectionId.IsEmpty() && batch.m_sProjectionId == operation.m_sProjectionId)))
				return true;
		}
		foreach (HST_ForceManifestState manifest : saveData.m_aForceManifests)
		{
			if (IsSchema54GarrisonPatrolManifestClaimant(saveData, manifest)
				&& !manifest.m_sOperationId.IsEmpty()
				&& batch.m_sOperationId == manifest.m_sOperationId
				&& !manifest.m_sManifestId.IsEmpty()
				&& batch.m_sManifestId == manifest.m_sManifestId)
				return true;
		}
		return false;
	}

	static bool IsSchema54GarrisonPatrolGroupClaimant(
		HST_CampaignSaveData saveData,
		HST_ActiveGroupState group)
	{
		if (!saveData || !group)
			return false;
		if (group.m_sSpawnFallbackMode == HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE
			|| group.m_sSpawnFallbackMode.StartsWith(HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE + "_")
			|| group.m_sRuntimeStatus.StartsWith("garrison_patrol_")
			|| group.m_sRuntimeStatus.StartsWith("exact_garrison_patrol_"))
			return true;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!IsSchema54GarrisonPatrolOperationClaimant(saveData, operation))
				continue;
			if (!operation.m_sOperationId.IsEmpty()
				&& group.m_sOperationId == operation.m_sOperationId
				&& ((!group.m_sGroupId.IsEmpty() && group.m_sGroupId == operation.m_sGroupId)
					|| (!group.m_sManifestId.IsEmpty() && group.m_sManifestId == operation.m_sManifestId)
					|| (!group.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == operation.m_sSpawnResultId)
					|| (!group.m_sForceId.IsEmpty() && group.m_sForceId == operation.m_sForceId)
					|| (!group.m_sProjectionId.IsEmpty() && group.m_sProjectionId == operation.m_sProjectionId)))
				return true;
		}
		return false;
	}

	static bool IsSchema54GarrisonPatrolRouteClaimant(
		HST_CampaignSaveData saveData,
		HST_GeneratedRouteState route)
	{
		if (!saveData || !route || route.m_sRouteId.IsEmpty())
			return false;
		if (route.m_sRouteId.StartsWith("route_garrison_")
			|| route.m_sSourceCategory == HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE)
			return true;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (IsSchema54GarrisonPatrolOperationClaimant(saveData, operation)
				&& operation.m_sCurrentRouteId == route.m_sRouteId)
				return true;
		}
		return false;
	}

	static bool IsSchema54GarrisonPatrolGarrisonClaimant(
		HST_CampaignSaveData saveData,
		HST_GarrisonState garrison)
	{
		if (!saveData || !garrison)
			return false;
		foreach (string manifestId : garrison.m_aAcceptedManifestIds)
		{
			foreach (HST_ForceManifestState manifest : saveData.m_aForceManifests)
			{
				if (manifest && manifest.m_sManifestId == manifestId
					&& IsSchema54GarrisonPatrolManifestClaimant(saveData, manifest))
					return true;
			}
		}
		return false;
	}

	protected void PreserveHistoricalGarrisons()
	{
		int legacyAcceptedCount;
		int unsupportedExactCount;
		foreach (HST_ForceQuoteState quote : m_SaveData.m_aForceQuotes)
		{
			if (!quote || quote.m_sQuoteKind != HST_ForcePlanningService.QUOTE_KIND_GARRISON
				|| quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED)
				continue;
			if (quote.m_sPolicyId == HST_GarrisonPatrolOperationService.EXACT_POLICY_ID)
				continue;
			legacyAcceptedCount++;
		}
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL)
				continue;
			QuarantineAggregate(
				FindUniqueQuote(operation.m_sQuoteId),
				operation,
				FindUniqueBatch(operation.m_sSpawnResultId),
				FindUniqueGroup(operation.m_sGroupId),
				"pre-schema-54 save carried an unsupported exact garrison patrol operation");
			unsupportedExactCount++;
		}
		if (HasEvent("migration_schema54_exact_garrison_patrol"))
			return;
		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = "migration_schema54_exact_garrison_patrol";
		eventState.m_sCategory = "migration";
		eventState.m_sAggregateType = "operation_record";
		eventState.m_sAggregateId = "schema54";
		eventState.m_sTransition = "legacy_garrison_authority_preserved";
		eventState.m_sReason = string.Format(
			"preserved %1 historical accepted garrison purchases on their aggregate-only policy and quarantined %2 unsupported exact-operation rows; created no exact operation, roster, batch, group, route, casualty, or settlement identity",
			legacyAcceptedCount,
			unsupportedExactCount);
		eventState.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		m_SaveData.m_aCampaignEvents.Insert(eventState);
	}

	protected bool IsAcceptedCurrentPolicyQuote(HST_ForceQuoteState quote)
	{
		return quote && quote.m_sQuoteKind == HST_ForcePlanningService.QUOTE_KIND_GARRISON
			&& quote.m_sPolicyId == HST_GarrisonPatrolOperationService.EXACT_POLICY_ID
			&& quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED;
	}

	protected string ValidateAggregate(
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_GarrisonState garrison,
		HST_GeneratedRouteState route)
	{
		if (!quote || !manifest || !operation)
			return "exact garrison patrol restore authority is incomplete or ambiguous";
		string failure = ValidateIdentity(quote, manifest, operation);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateFrozenPurchase(quote, manifest);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateCommittedTransactions(quote);
		if (!failure.IsEmpty())
			return failure;
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return ValidateSettledAggregate(quote, manifest, operation, batch, group, garrison, route);
		if (!batch || !group || !garrison || !route)
			return "open exact garrison patrol restore authority is incomplete or ambiguous";
		failure = ValidateOpenGarrisonBacklink(quote, manifest, garrison);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateOperationLifecycle(quote, operation, group, route);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateRuntime(quote, manifest, operation, batch, group);
		if (!failure.IsEmpty())
			return failure;
		return "";
	}

	protected string ValidateIdentity(
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation)
	{
		if (!IsAcceptedCurrentPolicyQuote(quote)
			|| manifest.m_sPolicyId != HST_GarrisonPatrolOperationService.EXACT_POLICY_ID
			|| manifest.m_sForceKind != HST_GarrisonPatrolOperationService.EXACT_FORCE_KIND)
			return "exact garrison patrol restore contract type, policy, or version conflicts";
		if (manifest.m_sIntentId != HST_GarrisonPatrolOperationService.EXACT_INTENT_ID
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL
			|| operation.m_iContractVersion != HST_GarrisonPatrolOperationService.EXACT_CONTRACT_VERSION)
			return "exact garrison patrol restore contract type, policy, or version conflicts";
		if (quote.m_sQuoteId.IsEmpty() || quote.m_sOperationId.IsEmpty()
			|| quote.m_sManifestId.IsEmpty() || quote.m_sConfirmationRequestId.IsEmpty())
			return "exact garrison patrol restore reciprocal planning identity conflicts";
		if (quote.m_sOperationId != HST_StableIdService.BuildOperationId("garrison_recruitment", quote.m_sQuoteId)
			|| quote.m_sOperationId != manifest.m_sOperationId
			|| quote.m_sOperationId != operation.m_sOperationId)
			return "exact garrison patrol restore reciprocal planning identity conflicts";
		if (quote.m_sManifestId != manifest.m_sManifestId
			|| quote.m_sManifestId != operation.m_sManifestId
			|| quote.m_sQuoteId != manifest.m_sQuoteId
			|| quote.m_sQuoteId != operation.m_sQuoteId)
			return "exact garrison patrol restore reciprocal planning identity conflicts";
		if (CountQuotesByAnyIdentity(quote) != 1
			|| CountManifestsByAnyIdentity(quote, manifest) != 1
			|| CountOperationsByAnyIdentity(quote, operation) != 1)
			return "exact garrison patrol restore planning or operation identity is ambiguous";
		if (quote.m_sFactionKey.IsEmpty() || quote.m_sTargetZoneId.IsEmpty()
			|| manifest.m_sFactionKey != quote.m_sFactionKey
			|| manifest.m_sTargetZoneId != quote.m_sTargetZoneId)
			return "exact garrison patrol restore owner or purchase identity conflicts";
		if (operation.m_sOwnerFactionKey != quote.m_sFactionKey
			|| operation.m_sActorIdentityId != quote.m_sActorIdentityId
			|| operation.m_sIssueRequestId != quote.m_sCommandRequestId
			|| operation.m_sConfirmationRequestId != quote.m_sConfirmationRequestId)
			return "exact garrison patrol restore owner or purchase identity conflicts";

		HST_ZoneState zone = FindUniqueZone(quote.m_sTargetZoneId);
		if (!zone || zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT
			|| zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE
			|| IsZeroVector(zone.m_vPosition))
			return "exact garrison patrol restore assignment zone conflicts";
		if (operation.m_sOriginZoneId != zone.m_sZoneId
			|| operation.m_sAssignmentZoneId != zone.m_sZoneId
			|| operation.m_sTacticalTargetZoneId != zone.m_sZoneId)
			return "exact garrison patrol restore immutable assignment conflicts";
		if (!PositionsMatch(operation.m_vOriginPosition, zone.m_vPosition)
			|| !PositionsMatch(operation.m_vAssignmentPosition, zone.m_vPosition)
			|| !PositionsMatch(operation.m_vTacticalTargetPosition, zone.m_vPosition)
			|| !PositionsMatch(quote.m_vTargetPosition, zone.m_vPosition))
			return "exact garrison patrol restore immutable assignment conflicts";
		return "";
	}

	protected string ValidateOpenGarrisonBacklink(
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_GarrisonState garrison)
	{
		HST_ZoneState zone = FindUniqueZone(quote.m_sTargetZoneId);
		// Ownership can legitimately change after the last operation tick and
		// before a checkpoint. Preserve an otherwise coherent exact graph so the
		// runtime lifecycle can settle it through the typed owner-change path
		// after restore; do not misclassify that transition as corruption.
		if (!zone)
			return "exact garrison patrol restore open assignment zone conflicts";
		if (!garrison || garrison.m_sZoneId != quote.m_sTargetZoneId
			|| garrison.m_sFactionKey != quote.m_sFactionKey)
			return "exact garrison patrol restore garrison backlink conflicts";
		if (garrison.m_sGarrisonId != HST_StableIdService.BuildGarrisonId(
			quote.m_sTargetZoneId,
			quote.m_sFactionKey))
			return "exact garrison patrol restore garrison backlink conflicts";
		if (CountString(garrison.m_aAcceptedManifestIds, manifest.m_sManifestId) != 1
			|| CountGarrisonBacklinks(manifest.m_sManifestId) != 1)
			return "exact garrison patrol restore garrison backlink conflicts";
		return "";
	}

	protected string ValidateFrozenPurchase(
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		string integrityFailure;
		if (!integrity.ValidateFrozenGarrisonQuote(manifest, quote, false, integrityFailure))
			return "exact garrison patrol restore frozen purchase conflicts: " + integrityFailure;
		if (!manifest.m_bFrozen || manifest.m_sManifestHash.IsEmpty()
			|| manifest.m_sManifestHash != quote.m_sManifestHash)
			return "exact garrison patrol restore frozen manifest shape conflicts";
		if (manifest.m_iRequestedMemberCount < 1 || manifest.m_iRequestedMemberCount > 32
			|| manifest.m_iRequestedMemberCount != manifest.m_iAcceptedMemberCount
			|| manifest.m_iAcceptedMemberCount != manifest.m_aMembers.Count())
			return "exact garrison patrol restore frozen manifest shape conflicts";
		if (manifest.m_iRequestedVehicleCount != 0 || manifest.m_iAcceptedVehicleCount != 0
			|| manifest.m_aVehicles.Count() != 0 || manifest.m_aAssets.Count() != 0)
			return "exact garrison patrol restore frozen manifest shape conflicts";
		if (manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
			return "exact garrison patrol restore frozen manifest shape conflicts";
		HST_ForceManifestGroupState root = manifest.m_aGroups[0];
		if (root.m_sElementId != manifest.m_sManifestId + "_group_1"
			|| root.m_sCatalogEntryId.IsEmpty() || root.m_sPrefab.IsEmpty())
			return "exact garrison patrol restore one-root execution contract conflicts";
		if (root.m_sPrefab != manifest.m_sGroupPrefab || !root.m_sPrefab.Contains("NotSpawned")
			|| root.m_sRole.IsEmpty() || root.m_iOrdinal != 0)
			return "exact garrison patrol restore one-root execution contract conflicts";
		if (!root.m_bRequired
			|| root.m_iExpectedMemberCount != manifest.m_iAcceptedMemberCount)
			return "exact garrison patrol restore one-root execution contract conflicts";
		for (int memberIndex = 0; memberIndex < manifest.m_aMembers.Count(); memberIndex++)
		{
			HST_ForceManifestMemberState member = manifest.m_aMembers[memberIndex];
			if (!member || member.m_sSlotId != string.Format("%1_member_%2", manifest.m_sManifestId, memberIndex + 1)
				|| member.m_sCatalogSlotId.IsEmpty() || member.m_sGroupElementId != root.m_sElementId)
				return "exact garrison patrol restore member roster conflicts";
			if (member.m_sPrefab.IsEmpty() || member.m_sRole.IsEmpty()
				|| member.m_iOrdinal != memberIndex || !member.m_bRequired
				|| CountManifestMembers(manifest, member.m_sSlotId) != 1)
				return "exact garrison patrol restore member roster conflicts";
		}
		return "";
	}

	protected string ValidateOperationLifecycle(
		HST_ForceQuoteState quote,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		HST_GeneratedRouteState route)
	{
		if (operation.m_iProjectionContractVersion != HST_GarrisonPatrolOperationService.EXACT_PROJECTION_CONTRACT_VERSION
			|| operation.m_sAssignmentKind != HST_GarrisonPatrolOperationService.ASSIGNMENT_KIND
			|| operation.m_sRecallPolicyId != HST_GarrisonPatrolOperationService.RECALL_POLICY_ID
			|| operation.m_sSettlementPolicyId != HST_GarrisonPatrolOperationService.SETTLEMENT_POLICY_ID)
			return "exact garrison patrol restore projection or assignment policy conflicts";
		if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			|| operation.m_eResumeDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			|| operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_UNKNOWN
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| !operation.m_sSettlementId.IsEmpty())
			return "open exact garrison patrol restore lifecycle conflicts";
		bool strategicPair = operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING);
		bool livePair = operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING);
		if ((!strategicPair && !livePair) || IsZeroVector(operation.m_vStrategicPosition))
			return "open exact garrison patrol restore projection pair conflicts";

		string expectedRouteId = "route_garrison_" + quote.m_sQuoteId;
		if (!route || operation.m_sCurrentRouteId != expectedRouteId
			|| group.m_sRouteId != expectedRouteId)
			return "exact garrison patrol restore local-route backlinks conflict";
		if (route.m_sRouteId != expectedRouteId
			|| route.m_sSourceZoneId != quote.m_sTargetZoneId
			|| route.m_sTargetZoneId != quote.m_sTargetZoneId)
			return "exact garrison patrol restore local-route backlinks conflict";
		if (route.m_sSourceLayerName != "operation_owned"
			|| route.m_sSourceCategory != HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE
			|| route.m_sSourceLayoutId != quote.m_sOperationId
			|| route.m_bRoadRoute || route.m_bValidatedForVehicles)
			return "exact garrison patrol restore local-route ownership conflicts";
		if (CountRoutesByAnyIdentity(route) != 1)
			return "exact garrison patrol restore local-route identity is ambiguous";
		HST_OperationRouteCursorService cursor = new HST_OperationRouteCursorService();
		if (!cursor.IsPatrolRouteContractValid(operation, route))
			return "exact garrison patrol restore frozen local route or cursor conflicts";
		array<vector> positions = HST_OperationRouteCursorService.BuildOrderedRoutePositions(route);
		if (!positions || positions.Count() != 4
			|| route.m_iWaypointCount != 4 || route.m_aWaypoints.Count() != 4)
			return "exact garrison patrol restore local-route geometry conflicts";
		if (operation.m_iRouteWaypointIndex < 0
			|| operation.m_iRouteWaypointIndex >= positions.Count()
			|| operation.m_iRouteLapCount < 0 || operation.m_iRouteLegSequence < 0)
			return "exact garrison patrol restore local-route geometry conflicts";
		if (operation.m_fRouteProgressMeters < -0.01
			|| operation.m_fRouteProgressMeters > operation.m_fRouteTotalDistanceMeters + 0.01
			|| !PositionsMatch(operation.m_vRouteEndPosition, positions[operation.m_iRouteWaypointIndex]))
			return "exact garrison patrol restore local-route geometry conflicts";
		if (!PositionsMatch(route.m_vStartPosition, positions[0])
			|| !PositionsMatch(route.m_vMidPosition, positions[1])
			|| !PositionsMatch(route.m_vEndPosition, positions[3]))
			return "exact garrison patrol restore local-route geometry conflicts";
		foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
		{
			if (!waypoint || waypoint.m_sRouteId != expectedRouteId
				|| waypoint.m_iIndex < 0 || waypoint.m_iIndex >= 4
				|| waypoint.m_iRadiusMeters != 25 || IsZeroVector(waypoint.m_vPosition))
				return "exact garrison patrol restore local-route waypoint conflicts";
		}
		if (operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& !PositionsMatch(operation.m_vStrategicPosition,
				HST_OperationRouteCursorService.ResolvePosition(operation), 2.0))
			return "exact garrison patrol restore strategic route position conflicts";
		return "";
	}

	protected string ValidateRuntime(
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		string expectedResultId = "spawn_garrison_" + quote.m_sQuoteId;
		string expectedForceId = "force_" + quote.m_sOperationId;
		string expectedProjectionId = "projection_" + quote.m_sOperationId;
		if (operation.m_sSpawnResultId != expectedResultId
			|| operation.m_sForceId != expectedForceId
			|| operation.m_sProjectionId != expectedProjectionId
			|| operation.m_sGroupId != expectedProjectionId)
			return "exact garrison patrol restore deterministic batch identity conflicts";
		if (batch.m_sResultId != expectedResultId || batch.m_sRequestId != quote.m_sQuoteId
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| batch.m_sManifestHash != manifest.m_sManifestHash)
			return "exact garrison patrol restore deterministic batch identity conflicts";
		if (batch.m_sOperationId != quote.m_sOperationId
			|| batch.m_sForceId != expectedForceId
			|| batch.m_sProjectionId != expectedProjectionId)
			return "exact garrison patrol restore deterministic batch identity conflicts";
		if (group.m_sGroupId != expectedProjectionId
			|| group.m_sOperationId != quote.m_sOperationId
			|| group.m_sManifestId != manifest.m_sManifestId
			|| group.m_sSpawnResultId != expectedResultId
			|| group.m_sForceId != expectedForceId
			|| group.m_sProjectionId != expectedProjectionId)
			return "exact garrison patrol restore active-group projection backlinks conflict";
		if (CountBatchesByAnyIdentity(quote, batch) != 1
			|| CountGroupsByAnyIdentity(quote, group) != 1)
			return "exact garrison patrol restore runtime identity is ambiguous";

		HST_ForceManifestGroupState root = manifest.m_aGroups[0];
		if (group.m_sZoneId != quote.m_sTargetZoneId
			|| group.m_sGarrisonZoneId != quote.m_sTargetZoneId
			|| group.m_sFactionKey != quote.m_sFactionKey
			|| group.m_sPrefab != root.m_sPrefab)
			return "exact garrison patrol restore active-group role conflicts";
		if (group.m_sCompositionRequestId != manifest.m_sManifestId
			|| group.m_sCompositionIntentId != HST_GarrisonPatrolOperationService.EXACT_INTENT_ID
			|| group.m_sCompositionTier != "exact")
			return "exact garrison patrol restore active-group role conflicts";
		if (group.m_sSpawnFallbackMode != HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE
			&& !group.m_sSpawnFallbackMode.StartsWith(HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE + "_"))
			return "exact garrison patrol restore active-group role conflicts";
		if (group.m_bQRF || !group.m_sSupportRequestId.IsEmpty()
			|| !group.m_sEnemyOrderId.IsEmpty() || !group.m_sMissionInstanceId.IsEmpty())
			return "exact garrison patrol restore active-group role conflicts";
		if (!group.m_sQRFInstanceId.IsEmpty() || !group.m_sConvoyElementId.IsEmpty()
			|| group.m_iVehicleCount != 0 || group.m_iOriginalVehicleCount != 0)
			return "exact garrison patrol restore active-group role conflicts";
		if (group.m_iOriginalInfantryCount != manifest.m_iAcceptedMemberCount
			|| group.m_iCompositionManpower != manifest.m_iAcceptedMemberCount)
			return "exact garrison patrol restore survivor aggregate conflicts";
		if (group.m_iInfantryCount < 0 || group.m_iInfantryCount > manifest.m_iAcceptedMemberCount
			|| group.m_iSurvivorInfantryCount < 0
			|| group.m_iSurvivorInfantryCount > manifest.m_iAcceptedMemberCount)
			return "exact garrison patrol restore survivor aggregate conflicts";
		if (group.m_iDurableLivingInfantryCount < 0
			|| group.m_iDurableLivingInfantryCount > manifest.m_iAcceptedMemberCount)
			return "exact garrison patrol restore survivor aggregate conflicts";
		if (operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& IsZeroVector(group.m_vPosition))
			return "exact garrison patrol restore live position authority has no group position";
		if (operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& !PositionsMatch(group.m_vPosition, operation.m_vStrategicPosition, 2.0))
			return "exact garrison patrol restore strategic group position conflicts";

		bool virtualState = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		bool physicalState = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		bool materializingState = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		bool dematerializingState = operation.m_eMaterializationState
			== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
			return "open exact garrison patrol restore contains a terminal spawn batch";
		if (virtualState && (!batch.m_bStrategicProjectionHeld
			|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			|| !batch.m_sNativeGroupId.IsEmpty() || group.m_bSpawnedEntity
			|| !group.m_sRuntimeEntityId.IsEmpty() || group.m_iSpawnedAgentCount != 0))
			return "virtual exact garrison patrol restore contains process-local authority";
		if (physicalState && (batch.m_bStrategicProjectionHeld
			|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_iSuccessfulHandoffCount <= 0 || !group.m_bSpawnedEntity))
			return "physical exact garrison patrol restore handoff evidence conflicts";
		if (materializingState && batch.m_bStrategicProjectionHeld)
			return "materializing exact garrison patrol restore remains strategically held";
		if (dematerializingState && (batch.m_bStrategicProjectionHeld || !group.m_bSpawnedEntity))
			return "dematerializing exact garrison patrol restore live handoff evidence conflicts";

		string slotFailure = ValidateBatchSlotBijection(manifest, batch);
		if (!slotFailure.IsEmpty())
			return slotFailure;
		if (physicalState)
		{
			slotFailure = ValidatePhysicalSlotEvidence(manifest, batch);
			if (!slotFailure.IsEmpty())
				return slotFailure;
		}
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		int living = manifest.m_iAcceptedMemberCount - queue.CountConfirmedCasualtyMemberSlots(batch);
		if (living <= 0)
			return "open exact garrison patrol restore has no durable living roster slots";
		if (virtualState && queue.CountStrategicLivingMemberSlots(batch) != living)
			return "virtual exact garrison patrol restore survivor slots conflict";
		if ((physicalState || dematerializingState)
			&& queue.CountDurableLivingMemberSlots(batch) != living)
			return "physical exact garrison patrol restore survivor slots conflict";
		return "";
	}

	protected string ValidatePhysicalSlotEvidence(
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		if (batch.m_sNativeGroupId.IsEmpty())
			return "physical exact garrison patrol restore lacks a native group receipt";
		HST_ForceSpawnSlotResultState root = batch.FindSlotResult(manifest.m_aGroups[0].m_sElementId);
		if (!root || root.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
			|| root.m_sEntityId.IsEmpty() || root.m_sNativeGroupId != batch.m_sNativeGroupId)
			return "physical exact garrison patrol restore group-root receipt conflicts";
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member)
				return "physical exact garrison patrol restore member receipt is missing";
			HST_ForceSpawnSlotResultState slot = batch.FindSlotResult(member.m_sSlotId);
			if (!slot)
				return "physical exact garrison patrol restore member receipt is missing";
			if (slot.m_bCasualtyConfirmed)
				continue;
			if (slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
				|| !slot.m_bEverAlive || slot.m_sEntityId.IsEmpty()
				|| slot.m_sNativeGroupId != batch.m_sNativeGroupId)
				return "physical exact garrison patrol restore living-member receipt conflicts";
		}
		return "";
	}

	protected string ValidateBatchSlotBijection(
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		int expectedSlots = manifest.m_aMembers.Count() + 1;
		if (batch.m_iExpectedSlotCount != expectedSlots
			|| batch.m_aSlotResults.Count() != expectedSlots)
			return "exact garrison patrol restore batch slot count conflicts";
		string rootSlotId = manifest.m_aGroups[0].m_sElementId;
		if (CountBatchSlots(batch, rootSlotId, HST_ForceSpawnQueueService.SLOT_KIND_GROUP) != 1)
			return "exact garrison patrol restore group-root slot conflicts";
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member || CountBatchSlots(batch, member.m_sSlotId,
				HST_ForceSpawnQueueService.SLOT_KIND_MEMBER) != 1)
				return "exact garrison patrol restore member-slot bijection conflicts";
		}
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_sSlotId.IsEmpty()
				|| slot.m_sProjectionId != batch.m_sProjectionId)
				return "exact garrison patrol restore batch slot identity conflicts";
			bool rootSlot = slot.m_sSlotId == rootSlotId
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP;
			bool memberSlot = manifest.FindMemberSlot(slot.m_sSlotId) != null
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER;
			if (!rootSlot && !memberSlot)
				return "exact garrison patrol restore contains a foreign batch slot";
			if (slot.m_bCasualtyConfirmed)
			{
				if (!memberSlot
					|| slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
					|| !slot.m_bEverAlive
					|| slot.m_iCasualtyAtSecond < batch.m_iCreatedAtSecond)
					return "exact garrison patrol restore casualty tombstone conflicts";
				continue;
			}
			if (slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				|| slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_FINAL
				|| slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CANCELLED)
				return "open exact garrison patrol restore contains an unproven terminal slot";
			if (memberSlot && slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
				&& !slot.m_bEverAlive)
				return "registered exact garrison patrol member lacks durable living evidence";
		}
		return "";
	}

	protected string ValidateCommittedTransactions(HST_ForceQuoteState quote)
	{
		HST_ResourceTransactionState money = FindUniqueTransaction(quote.m_sMoneyTransactionId);
		HST_ResourceTransactionState hr = FindUniqueTransaction(quote.m_sHRTransactionId);
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		if (!money || !hr
			|| !integrity.TransactionMatchesQuote(money, quote,
				HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quote.m_iMoneyCost)
			|| !integrity.TransactionMatchesQuote(hr, quote,
				HST_ResourceLedgerService.RESOURCE_HR, quote.m_iHRCost))
			return "exact garrison patrol restore committed resource receipt conflicts";
		return "";
	}

	protected string ValidateSettledAggregate(
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_GarrisonState garrison,
		HST_GeneratedRouteState route)
	{
		if (operation.m_iProjectionContractVersion != HST_GarrisonPatrolOperationService.EXACT_PROJECTION_CONTRACT_VERSION
			|| operation.m_sAssignmentKind != HST_GarrisonPatrolOperationService.ASSIGNMENT_KIND
			|| operation.m_sRecallPolicyId != HST_GarrisonPatrolOperationService.RECALL_POLICY_ID
			|| operation.m_sSettlementPolicyId != HST_GarrisonPatrolOperationService.SETTLEMENT_POLICY_ID)
			return "settled exact garrison patrol restore policy receipt conflicts";
		string expectedSettlementId = HST_OperationService.BuildSettlementId(
			operation.m_sOperationId,
			HST_GarrisonPatrolOperationService.SETTLEMENT_KIND);
		if (operation.m_sSettlementId != expectedSettlementId
			|| operation.m_iSettledAtSecond < operation.m_iCreatedAtSecond
			|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN)
			return "settled exact garrison patrol restore receipt conflicts";
		if (operation.m_sSpawnResultId != "spawn_garrison_" + quote.m_sQuoteId
			|| operation.m_sForceId != "force_" + quote.m_sOperationId
			|| operation.m_sProjectionId != "projection_" + quote.m_sOperationId
			|| operation.m_sGroupId != "projection_" + quote.m_sOperationId)
			return "settled exact garrison patrol restore runtime identity receipt conflicts";
		if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			|| operation.m_eResumeDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			|| operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return "settled exact garrison patrol restore terminal lifecycle conflicts";
		if (CountGarrisonBacklinks(manifest.m_sManifestId) != 0)
			return "settled exact garrison patrol restore retains a live garrison backlink";
		if (garrison && CountString(garrison.m_aAcceptedManifestIds, manifest.m_sManifestId) != 0)
			return "settled exact garrison patrol restore retains a live garrison backlink";

		if (route)
		{
			string routeFailure = ValidateSettledRouteResidue(quote, operation, route);
			if (!routeFailure.IsEmpty())
				return routeFailure;
		}
		if (batch)
		{
			string batchFailure = ValidateSettledBatchResidue(quote, manifest, operation, batch);
			if (!batchFailure.IsEmpty())
				return batchFailure;
		}
		if (group)
		{
			string groupFailure = ValidateSettledGroupResidue(quote, manifest, operation, group);
			if (!groupFailure.IsEmpty())
				return groupFailure;
		}
		return "";
	}

	protected string ValidateSettledRouteResidue(
		HST_ForceQuoteState quote,
		HST_OperationRecordState operation,
		HST_GeneratedRouteState route)
	{
		string expectedRouteId = "route_garrison_" + quote.m_sQuoteId;
		if (route.m_sRouteId != expectedRouteId
			|| operation.m_sCurrentRouteId != expectedRouteId
			|| route.m_sSourceZoneId != quote.m_sTargetZoneId
			|| route.m_sTargetZoneId != quote.m_sTargetZoneId)
			return "settled exact garrison patrol restore route residue conflicts";
		if (route.m_sSourceLayerName != "operation_owned"
			|| route.m_sSourceCategory != HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE
			|| route.m_sSourceLayoutId != quote.m_sOperationId)
			return "settled exact garrison patrol restore route residue conflicts";
		array<vector> positions = HST_OperationRouteCursorService.BuildOrderedRoutePositions(route);
		if (!positions || positions.Count() != 4
			|| route.m_iWaypointCount != 4 || route.m_aWaypoints.Count() != 4)
			return "settled exact garrison patrol restore route residue conflicts";
		if (operation.m_sRouteContractHash
			!= HST_OperationRouteCursorService.BuildRouteContractHash(route, positions))
			return "settled exact garrison patrol restore route residue conflicts";
		if (CountRoutesByAnyIdentity(route) != 1)
			return "settled exact garrison patrol restore route residue is ambiguous";
		return "";
	}

	protected string ValidateSettledBatchResidue(
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch)
	{
		string expectedResultId = "spawn_garrison_" + quote.m_sQuoteId;
		string expectedForceId = "force_" + quote.m_sOperationId;
		string expectedProjectionId = "projection_" + quote.m_sOperationId;
		if (batch.m_sResultId != expectedResultId || batch.m_sRequestId != quote.m_sQuoteId
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| batch.m_sManifestHash != manifest.m_sManifestHash)
			return "settled exact garrison patrol restore batch residue conflicts";
		if (batch.m_sOperationId != operation.m_sOperationId
			|| batch.m_sForceId != expectedForceId
			|| batch.m_sProjectionId != expectedProjectionId
			|| CountBatchesByAnyIdentity(quote, batch) != 1)
			return "settled exact garrison patrol restore batch residue conflicts";
		return ValidateSettledBatchSlots(manifest, batch);
	}

	protected string ValidateSettledBatchSlots(
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		if (batch.m_iExpectedSlotCount != manifest.m_aMembers.Count() + 1
			|| batch.m_aSlotResults.Count() != batch.m_iExpectedSlotCount)
			return "settled exact garrison patrol restore batch slot residue conflicts";
		string rootSlotId = manifest.m_aGroups[0].m_sElementId;
		if (CountBatchSlots(batch, rootSlotId, HST_ForceSpawnQueueService.SLOT_KIND_GROUP) != 1)
			return "settled exact garrison patrol restore root-slot residue conflicts";
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member || CountBatchSlots(batch, member.m_sSlotId,
				HST_ForceSpawnQueueService.SLOT_KIND_MEMBER) != 1)
				return "settled exact garrison patrol restore member-slot residue conflicts";
		}
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_sProjectionId != batch.m_sProjectionId)
				return "settled exact garrison patrol restore slot identity residue conflicts";
			bool memberSlot = manifest.FindMemberSlot(slot.m_sSlotId) != null
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER;
			bool rootSlot = slot.m_sSlotId == rootSlotId
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP;
			if (!memberSlot && !rootSlot)
				return "settled exact garrison patrol restore contains a foreign slot residue";
			if (slot.m_bCasualtyConfirmed
				&& (!memberSlot || !slot.m_bEverAlive
					|| slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
					|| slot.m_iCasualtyAtSecond < batch.m_iCreatedAtSecond))
				return "settled exact garrison patrol restore casualty residue conflicts";
		}
		return "";
	}

	protected string ValidateSettledGroupResidue(
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group)
	{
		string expectedProjectionId = "projection_" + quote.m_sOperationId;
		if (group.m_sGroupId != expectedProjectionId
			|| group.m_sOperationId != operation.m_sOperationId
			|| group.m_sManifestId != manifest.m_sManifestId
			|| group.m_sGarrisonZoneId != quote.m_sTargetZoneId)
			return "settled exact garrison patrol restore group residue conflicts";
		if (group.m_sForceId != "force_" + quote.m_sOperationId
			|| group.m_sProjectionId != expectedProjectionId
			|| CountGroupsByAnyIdentity(quote, group) != 1)
			return "settled exact garrison patrol restore group residue conflicts";
		if (group.m_sSpawnFallbackMode != HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE
			&& !group.m_sSpawnFallbackMode.StartsWith(HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE + "_"))
			return "settled exact garrison patrol restore group role residue conflicts";
		return "";
	}

	protected void NormalizeSettledResidue(
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (batch)
		{
			batch.m_sNativeGroupId = "";
			batch.m_bStrategicProjectionHeld = false;
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
		if (!group)
			return;
		group.m_bSpawnedEntity = false;
		group.m_bSpawnAttempted = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
		group.m_sRuntimeStatus = "retired";
	}

	protected void NormalizeValidAggregate(
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			NormalizeSettledResidue(batch, group);
			return;
		}
		int restoreSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		bool savedLive = operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING);
		if (savedLive && group && !IsZeroVector(group.m_vPosition))
		{
			operation.m_vStrategicPosition = group.m_vPosition;
			if (!operation.m_sCurrentRouteId.IsEmpty())
			{
				HST_OperationRouteCursorService cursor = new HST_OperationRouteCursorService();
				cursor.SyncLegFromPositionAtSecond(restoreSecond, operation, group, group.m_vPosition);
			}
		}

		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_iMaterializationStateEnteredAtSecond = restoreSecond;
		operation.m_iStrategicLastUpdateSecond = restoreSecond;
		operation.m_iLastProjectionDecisionSecond = restoreSecond;
		operation.m_iLastProgressAtSecond = restoreSecond;
		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		operation.m_sLastProjectionReason = "restored exact purchased-garrison patrol as held strategic authority";
		operation.m_iRevision++;

		NormalizeBatchForStrategicHold(batch, restoreSecond);
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		int living = queue.CountStrategicLivingMemberSlots(batch);
		operation.m_iLastVirtualFriendlyCount = Math.Max(0, living);
		NormalizeGroupForStrategicHold(operation, group, living);
	}

	protected void NormalizeBatchForStrategicHold(
		HST_ForceSpawnResultState batch,
		int restoreSecond)
	{
		if (!batch)
			return;
		bool wasProcessLocal = batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			|| !batch.m_bStrategicProjectionHeld;
		batch.m_sNativeGroupId = "";
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING;
		batch.m_bStrategicProjectionHeld = true;
		batch.m_bCancelRequested = false;
		if (wasProcessLocal)
			batch.m_iReprojectionCount++;
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
	}

	protected void NormalizeGroupForStrategicHold(
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		int living)
	{
		if (!operation || !group)
			return;
		group.m_bSpawnedEntity = false;
		group.m_bSpawnAttempted = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
		group.m_sRuntimeStatus = "garrison_patrol_virtual";
		group.m_sSpawnFallbackMode = HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE;
		group.m_vPosition = operation.m_vStrategicPosition;
		group.m_vSourcePosition = operation.m_vStrategicPosition;
		if (!operation.m_sCurrentRouteId.IsEmpty())
			group.m_vTargetPosition = operation.m_vRouteEndPosition;
		else
			group.m_vTargetPosition = operation.m_vAssignmentPosition;
		group.m_iInfantryCount = Math.Max(0, living);
		group.m_iLastSeenAliveCount = Math.Max(0, living);
		group.m_iSurvivorInfantryCount = Math.Max(0, living);
		group.m_iDurableLivingInfantryCount = Math.Max(0, living);
		group.m_iLifecycleRevision++;
	}

	protected void QuarantineAggregate(
		HST_ForceQuoteState quote,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		if (reason.IsEmpty())
			reason = "schema-54 exact garrison patrol authority conflict";
		if (operation)
		{
			operation.m_iContractVersion = HST_GarrisonPatrolOperationService.QUARANTINED_CONTRACT_VERSION;
			if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
			{
				operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
				operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
			}
			operation.m_sLastProjectionReason = reason;
			operation.m_iRevision++;
		}
		if (batch)
			HoldBatch(batch, reason);
		if (group)
			HoldGroup(group, reason);
		HoldAllClaimants(quote, operation, reason);
	}

	protected int PreserveOrphanClaimants(array<string> validatedOperationIds)
	{
		int quarantinedCount;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL)
				continue;
			if (validatedOperationIds.Contains(operation.m_sOperationId))
				continue;
			HST_ForceQuoteState quote = FindUniqueQuote(operation.m_sQuoteId);
			string reason = "exact garrison patrol operation has no validated accepted purchase graph";
			if (operation.m_iContractVersion == HST_GarrisonPatrolOperationService.QUARANTINED_CONTRACT_VERSION)
				reason = "schema-54 exact garrison patrol remained quarantined after restore";
			QuarantineAggregate(
				quote,
				operation,
				FindUniqueBatch(operation.m_sSpawnResultId),
				FindUniqueGroup(operation.m_sGroupId),
				reason);
			quarantinedCount++;
		}

		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (!IsSchema54GarrisonPatrolManifestClaimant(m_SaveData, manifest))
				continue;
			HST_ForceQuoteState quote = FindUniqueQuote(manifest.m_sQuoteId);
			if (quote && quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED
				&& quote.m_sPolicyId == HST_GarrisonPatrolOperationService.EXACT_POLICY_ID
				&& quote.m_sManifestId == manifest.m_sManifestId
				&& quote.m_sOperationId == manifest.m_sOperationId)
				continue;
			HST_OperationRecordState operation = FindUniqueOperation(manifest.m_sOperationId);
			if (operation && validatedOperationIds.Contains(operation.m_sOperationId))
				continue;
			if (operation)
				QuarantineAggregate(quote, operation,
					FindUniqueBatch(operation.m_sSpawnResultId),
					FindUniqueGroup(operation.m_sGroupId),
					"exact garrison patrol manifest has no validated reciprocal authority graph");
			else
				HoldManifestClaimants(manifest,
					"exact garrison patrol manifest has no validated reciprocal authority graph");
		}

		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!IsSchema54GarrisonPatrolBatchClaimant(m_SaveData, batch))
				continue;
			HST_OperationRecordState operation = FindUniqueOperation(batch.m_sOperationId);
			if (operation && validatedOperationIds.Contains(operation.m_sOperationId))
				continue;
			HoldBatch(batch, "exact garrison patrol batch has no validated reciprocal authority graph");
		}
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!IsSchema54GarrisonPatrolGroupClaimant(m_SaveData, group))
				continue;
			HST_OperationRecordState operation = FindUniqueOperation(group.m_sOperationId);
			if (operation && validatedOperationIds.Contains(operation.m_sOperationId))
				continue;
			HoldGroup(group, "exact garrison patrol group has no validated reciprocal authority graph");
		}
		return quarantinedCount;
	}

	protected void HoldAllClaimants(
		HST_ForceQuoteState quote,
		HST_OperationRecordState operation,
		string reason)
	{
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (BatchClaimsAuthority(batch, quote, operation))
				HoldBatch(batch, reason);
		}
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (GroupClaimsAuthority(group, quote, operation))
				HoldGroup(group, reason);
		}
	}

	protected void HoldManifestClaimants(HST_ForceManifestState manifest, string reason)
	{
		if (!manifest)
			return;
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (batch && ((!manifest.m_sManifestId.IsEmpty() && batch.m_sManifestId == manifest.m_sManifestId)
				|| (!manifest.m_sOperationId.IsEmpty() && batch.m_sOperationId == manifest.m_sOperationId)))
				HoldBatch(batch, reason);
		}
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (group && ((!manifest.m_sManifestId.IsEmpty() && group.m_sManifestId == manifest.m_sManifestId)
				|| (!manifest.m_sOperationId.IsEmpty() && group.m_sOperationId == manifest.m_sOperationId)))
				HoldGroup(group, reason);
		}
	}

	protected void HoldBatch(HST_ForceSpawnResultState batch, string reason)
	{
		if (!batch)
			return;
		batch.m_sNativeGroupId = "";
		batch.m_bStrategicProjectionHeld = true;
		batch.m_bCancelRequested = true;
		batch.m_sLastFailureReason = reason;
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
		group.m_sSpawnFallbackMode = HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE + "_quarantined";
		group.m_sRuntimeStatus = "exact_garrison_patrol_quarantined";
		group.m_sSpawnFailureReason = reason;
	}

	protected bool BatchClaimsAuthority(
		HST_ForceSpawnResultState batch,
		HST_ForceQuoteState quote,
		HST_OperationRecordState operation)
	{
		if (!batch)
			return false;
		if (quote && !quote.m_sOperationId.IsEmpty()
			&& batch.m_sOperationId == quote.m_sOperationId
			&& ((!quote.m_sQuoteId.IsEmpty() && batch.m_sRequestId == quote.m_sQuoteId)
				|| (!quote.m_sManifestId.IsEmpty() && batch.m_sManifestId == quote.m_sManifestId)))
			return true;
		if (!operation)
			return false;
		if (operation.m_sOperationId.IsEmpty()
			|| batch.m_sOperationId != operation.m_sOperationId)
			return false;
		return (!operation.m_sSpawnResultId.IsEmpty() && batch.m_sResultId == operation.m_sSpawnResultId)
			|| (!operation.m_sManifestId.IsEmpty() && batch.m_sManifestId == operation.m_sManifestId)
			|| (!operation.m_sForceId.IsEmpty() && batch.m_sForceId == operation.m_sForceId)
			|| (!operation.m_sProjectionId.IsEmpty() && batch.m_sProjectionId == operation.m_sProjectionId);
	}

	protected bool GroupClaimsAuthority(
		HST_ActiveGroupState group,
		HST_ForceQuoteState quote,
		HST_OperationRecordState operation)
	{
		if (!group)
			return false;
		if (quote && !quote.m_sOperationId.IsEmpty()
			&& group.m_sOperationId == quote.m_sOperationId
			&& !quote.m_sManifestId.IsEmpty()
			&& group.m_sManifestId == quote.m_sManifestId)
			return true;
		if (!operation)
			return false;
		if (operation.m_sOperationId.IsEmpty()
			|| group.m_sOperationId != operation.m_sOperationId)
			return false;
		return (!operation.m_sGroupId.IsEmpty() && group.m_sGroupId == operation.m_sGroupId)
			|| (!operation.m_sManifestId.IsEmpty() && group.m_sManifestId == operation.m_sManifestId)
			|| (!operation.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == operation.m_sSpawnResultId)
			|| (!operation.m_sForceId.IsEmpty() && group.m_sForceId == operation.m_sForceId)
			|| (!operation.m_sProjectionId.IsEmpty() && group.m_sProjectionId == operation.m_sProjectionId);
	}

	protected HST_ForceQuoteState FindUniqueQuote(string quoteId)
	{
		HST_ForceQuoteState match;
		if (quoteId.IsEmpty())
			return null;
		foreach (HST_ForceQuoteState quote : m_SaveData.m_aForceQuotes)
		{
			if (!quote || quote.m_sQuoteId != quoteId)
				continue;
			if (match)
				return null;
			match = quote;
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

	protected HST_GeneratedRouteState FindUniqueRoute(string routeId)
	{
		HST_GeneratedRouteState match;
		if (routeId.IsEmpty())
			return null;
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

	protected HST_GarrisonState FindUniqueGarrison(string zoneId, string factionKey)
	{
		HST_GarrisonState match;
		foreach (HST_GarrisonState garrison : m_SaveData.m_aGarrisons)
		{
			if (!garrison || garrison.m_sZoneId != zoneId
				|| garrison.m_sFactionKey != factionKey)
				continue;
			if (match)
				return null;
			match = garrison;
		}
		return match;
	}

	protected HST_ZoneState FindUniqueZone(string zoneId)
	{
		HST_ZoneState match;
		foreach (HST_ZoneState zone : m_SaveData.m_aZones)
		{
			if (!zone || zone.m_sZoneId != zoneId)
				continue;
			if (match)
				return null;
			match = zone;
		}
		if (match || !HST_MaidensBayLocationSaveValidationService.IsLegacyZoneId(zoneId))
			return match;

		HST_ZoneState canonicalMatch;
		foreach (HST_ZoneState canonicalZone : m_SaveData.m_aZones)
		{
			if (!canonicalZone || canonicalZone.m_sZoneId != HST_MaidensBayLocationSaveValidationService.CANONICAL_ZONE_ID)
				continue;
			if (canonicalMatch)
				return null;
			canonicalMatch = canonicalZone;
		}
		return HST_MaidensBayLocationSaveValidationService.BuildFrozenHistoricalZoneView(canonicalMatch);
	}

	protected HST_ResourceTransactionState FindUniqueTransaction(string transactionId)
	{
		HST_ResourceTransactionState match;
		if (transactionId.IsEmpty())
			return null;
		foreach (HST_ResourceTransactionState transaction : m_SaveData.m_aResourceTransactions)
		{
			if (!transaction || transaction.m_sTransactionId != transactionId)
				continue;
			if (match)
				return null;
			match = transaction;
		}
		return match;
	}

	protected int CountQuotesByAnyIdentity(HST_ForceQuoteState expected)
	{
		int count;
		foreach (HST_ForceQuoteState quote : m_SaveData.m_aForceQuotes)
		{
			if (!quote)
				continue;
			bool matches = quote.m_sQuoteId == expected.m_sQuoteId;
			if (!matches && !expected.m_sOperationId.IsEmpty())
				matches = quote.m_sOperationId == expected.m_sOperationId;
			if (!matches && !expected.m_sManifestId.IsEmpty())
				matches = quote.m_sManifestId == expected.m_sManifestId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountManifestsByAnyIdentity(
		HST_ForceQuoteState quote,
		HST_ForceManifestState expected)
	{
		int count;
		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (!manifest)
				continue;
			bool matches = manifest.m_sManifestId == expected.m_sManifestId;
			if (!matches && !quote.m_sOperationId.IsEmpty())
				matches = manifest.m_sOperationId == quote.m_sOperationId;
			if (!matches && !quote.m_sQuoteId.IsEmpty())
				matches = manifest.m_sQuoteId == quote.m_sQuoteId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountOperationsByAnyIdentity(
		HST_ForceQuoteState quote,
		HST_OperationRecordState expected)
	{
		int count;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!operation)
				continue;
			bool matches = operation.m_sOperationId == expected.m_sOperationId;
			if (!matches && !quote.m_sQuoteId.IsEmpty())
				matches = operation.m_sQuoteId == quote.m_sQuoteId;
			if (!matches && !quote.m_sManifestId.IsEmpty())
				matches = operation.m_sManifestId == quote.m_sManifestId;
			if (!matches && !expected.m_sSpawnResultId.IsEmpty())
				matches = operation.m_sSpawnResultId == expected.m_sSpawnResultId;
			if (!matches && !expected.m_sProjectionId.IsEmpty())
				matches = operation.m_sProjectionId == expected.m_sProjectionId;
			if (!matches && !expected.m_sGroupId.IsEmpty())
				matches = operation.m_sGroupId == expected.m_sGroupId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountBatchesByAnyIdentity(
		HST_ForceQuoteState quote,
		HST_ForceSpawnResultState expected)
	{
		int count;
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!batch)
				continue;
			bool matches = batch.m_sResultId == expected.m_sResultId;
			if (!matches && !quote.m_sQuoteId.IsEmpty())
				matches = batch.m_sRequestId == quote.m_sQuoteId;
			if (!matches && !quote.m_sOperationId.IsEmpty())
				matches = batch.m_sOperationId == quote.m_sOperationId;
			if (!matches && !quote.m_sManifestId.IsEmpty())
				matches = batch.m_sManifestId == quote.m_sManifestId;
			if (!matches && !expected.m_sForceId.IsEmpty())
				matches = batch.m_sForceId == expected.m_sForceId;
			if (!matches && !expected.m_sProjectionId.IsEmpty())
				matches = batch.m_sProjectionId == expected.m_sProjectionId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountGroupsByAnyIdentity(
		HST_ForceQuoteState quote,
		HST_ActiveGroupState expected)
	{
		int count;
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!group)
				continue;
			bool matches = group.m_sGroupId == expected.m_sGroupId;
			if (!matches && !quote.m_sOperationId.IsEmpty())
				matches = group.m_sOperationId == quote.m_sOperationId;
			if (!matches && !quote.m_sManifestId.IsEmpty())
				matches = group.m_sManifestId == quote.m_sManifestId;
			if (!matches && !expected.m_sSpawnResultId.IsEmpty())
				matches = group.m_sSpawnResultId == expected.m_sSpawnResultId;
			if (!matches && !expected.m_sForceId.IsEmpty())
				matches = group.m_sForceId == expected.m_sForceId;
			if (!matches && !expected.m_sProjectionId.IsEmpty())
				matches = group.m_sProjectionId == expected.m_sProjectionId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountRoutesByAnyIdentity(HST_GeneratedRouteState expected)
	{
		int count;
		foreach (HST_GeneratedRouteState route : m_SaveData.m_aGeneratedRoutes)
		{
			if (!route)
				continue;
			if (route.m_sRouteId == expected.m_sRouteId
				|| route.m_sSourceLayoutId == expected.m_sSourceLayoutId)
				count++;
		}
		return count;
	}

	protected int CountGarrisonBacklinks(string manifestId)
	{
		int count;
		foreach (HST_GarrisonState garrison : m_SaveData.m_aGarrisons)
		{
			if (!garrison)
				continue;
			count += CountString(garrison.m_aAcceptedManifestIds, manifestId);
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

	protected int CountString(array<string> values, string expected)
	{
		int count;
		if (!values || expected.IsEmpty())
			return count;
		foreach (string value : values)
		{
			if (value == expected)
				count++;
		}
		return count;
	}

	protected bool PositionsMatch(vector first, vector second, float toleranceMeters = 0.5)
	{
		return DistanceSq2D(first, second) <= toleranceMeters * toleranceMeters;
	}

	protected float DistanceSq2D(vector first, vector second)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz;
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
