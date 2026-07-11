// Conservative schema-53 restore boundary for newly queued exact enemy patrols.
// Historical patrol orders remain contract zero; current-schema corruption is
// quarantined with its durable graph preserved for diagnosis and settlement.
class HST_EnemyPatrolSaveValidationService
{
	protected HST_CampaignSaveData m_SaveData;

	void Normalize(HST_CampaignSaveData saveData, int restoredSchemaVersion)
	{
		m_SaveData = saveData;
		if (!m_SaveData)
			return;
		if (restoredSchemaVersion < 53)
		{
			PreserveHistoricalPatrols();
			PreserveOrphanClaimants();
			return;
		}

		foreach (HST_EnemyOrderState order : m_SaveData.m_aEnemyOrders)
		{
			if (!order || order.m_eType != HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL
				|| order.m_iOperationContractVersion == 0)
				continue;
			if (order.m_iOperationContractVersion == HST_EnemyPatrolOperationService.QUARANTINED_CONTRACT_VERSION)
			{
				NormalizeQuarantinedOrder(order);
				continue;
			}

			HST_OperationRecordState operation = FindUniqueOperation(order.m_sOperationId);
			HST_ForceManifestState manifest = FindUniqueManifest(order.m_sManifestId);
			HST_ForceSpawnResultState batch = FindUniqueBatch(order.m_sSpawnResultId);
			HST_ActiveGroupState group = FindUniqueGroup(order.m_sGroupId);
			string failure = ValidateAggregate(order, operation, manifest, batch, group);
			if (!failure.IsEmpty())
			{
				Quarantine(order, operation, batch, group, failure);
				continue;
			}
			NormalizeValidAggregate(order, operation, manifest, batch, group);
		}
		PreserveOrphanClaimants();
	}

	protected void PreserveHistoricalPatrols()
	{
		int preserved;
		int quarantined;
		foreach (HST_EnemyOrderState order : m_SaveData.m_aEnemyOrders)
		{
			if (!order || order.m_eType != HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL)
				continue;
			if (order.m_iOperationContractVersion == 0)
			{
				preserved++;
				continue;
			}
			Quarantine(
				order,
				FindUniqueOperation(order.m_sOperationId),
				FindUniqueBatch(order.m_sSpawnResultId),
				FindUniqueGroup(order.m_sGroupId),
				"pre-schema-53 patrol carried an unsupported nonzero operation contract");
			quarantined++;
		}
		if (HasEvent("migration_schema53_enemy_patrol_authority"))
			return;
		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = "migration_schema53_enemy_patrol_authority";
		eventState.m_sCategory = "migration";
		eventState.m_sAggregateType = "operation_record";
		eventState.m_sAggregateId = "schema53";
		eventState.m_sTransition = "legacy_enemy_patrols_preserved_contract_zero";
		eventState.m_sReason = string.Format(
			"preserved %1 historical patrol orders at contract zero and quarantined %2 unsupported nonzero rows; created no route cursor, manifest, roster, operation, or refund",
			preserved,
			quarantined);
		eventState.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		m_SaveData.m_aCampaignEvents.Insert(eventState);
	}

	protected string ValidateAggregate(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!order || !operation || !manifest)
			return "exact enemy patrol restore authority is incomplete or ambiguous";
		string identityFailure = ValidateIdentity(order, operation, manifest);
		if (!identityFailure.IsEmpty())
			return identityFailure;
		string manifestFailure = ValidateManifest(order, manifest);
		if (!manifestFailure.IsEmpty())
			return manifestFailure;
		string routeFailure = ValidateRouteAndLifecycle(order, operation);
		if (!routeFailure.IsEmpty())
			return routeFailure;
		string runtimeFailure = ValidateRuntime(order, operation, manifest, batch, group);
		if (!runtimeFailure.IsEmpty())
			return runtimeFailure;
		return ValidateSettlement(order, operation, manifest, batch);
	}

	protected string ValidateIdentity(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		if (order.m_iOperationContractVersion != HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION
			|| operation.m_iContractVersion != HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION
			|| order.m_eType != HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL)
			return "exact enemy patrol restore contract type or version conflicts";
		if (order.m_sOrderId.IsEmpty() || order.m_sOperationId.IsEmpty()
			|| order.m_sOperationId != HST_StableIdService.BuildOperationId("enemy_order", order.m_sOrderId)
			|| operation.m_sOperationId != order.m_sOperationId
			|| operation.m_sEnemyOrderId != order.m_sOrderId
			|| operation.m_sManifestId != order.m_sManifestId
			|| manifest.m_sOperationId != order.m_sOperationId
			|| manifest.m_sManifestId != order.m_sManifestId)
			return "exact enemy patrol restore reciprocal identity conflicts";
		if (CountOrdersByAnyIdentity(order) != 1
			|| CountOperationsByAnyIdentity(order, operation) != 1
			|| CountManifestsByAnyIdentity(order, manifest) != 1)
			return "exact enemy patrol restore durable identity is ambiguous";
		if (order.m_sFactionKey.IsEmpty() || order.m_sSourceZoneId.IsEmpty()
			|| order.m_sTargetZoneId.IsEmpty() || order.m_sSourceZoneId == order.m_sTargetZoneId
			|| IsZeroVector(order.m_vSourcePosition) || IsZeroVector(order.m_vTargetPosition))
			return "exact enemy patrol restore source, target, or faction conflicts";
		if (operation.m_sOwnerFactionKey != order.m_sFactionKey
			|| operation.m_sOriginZoneId != order.m_sSourceZoneId
			|| operation.m_sAssignmentZoneId != order.m_sTargetZoneId
			|| operation.m_sTacticalTargetZoneId != order.m_sTargetZoneId)
			return "exact enemy patrol restore source, target, or faction conflicts";
		if (!PositionsMatch(operation.m_vOriginPosition, order.m_vSourcePosition)
			|| !PositionsMatch(operation.m_vAssignmentPosition, order.m_vTargetPosition)
			|| !PositionsMatch(operation.m_vTacticalTargetPosition, order.m_vTargetPosition))
			return "exact enemy patrol restore source, target, or faction conflicts";
		if (manifest.m_sFactionKey != order.m_sFactionKey
			|| manifest.m_sSourceZoneId != order.m_sSourceZoneId
			|| manifest.m_sTargetZoneId != order.m_sTargetZoneId)
			return "exact enemy patrol restore source, target, or faction conflicts";
		return "";
	}

	protected string ValidateManifest(HST_EnemyOrderState order, HST_ForceManifestState manifest)
	{
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		HST_StrategicMovementService movement = new HST_StrategicMovementService();
		if (!manifest.m_bFrozen || manifest.m_sManifestHash.IsEmpty()
			|| integrity.BuildManifestHash(manifest) != manifest.m_sManifestHash
			|| order.m_sManifestHash != manifest.m_sManifestHash)
			return "exact enemy patrol restore frozen manifest hash conflicts";
		if (manifest.m_sForceKind != HST_EnemyPatrolOperationService.EXACT_FORCE_KIND
			|| manifest.m_sPolicyId != HST_EnemyPatrolOperationService.EXACT_POLICY_ID
			|| manifest.m_sIntentId != HST_EnemyPatrolOperationService.EXACT_MANIFEST_INTENT
			|| manifest.m_sManifestId != "manifest_" + order.m_sOperationId
			|| manifest.m_sFactionRole != "enemy" || manifest.m_sCatalogVersion.IsEmpty())
			return "exact enemy patrol restore manifest policy conflicts";
		if (manifest.m_iRequestedMemberCount <= 0
			|| manifest.m_iRequestedMemberCount != manifest.m_iAcceptedMemberCount
			|| manifest.m_iAcceptedMemberCount != manifest.m_aMembers.Count()
			|| manifest.m_iRequestedVehicleCount != 0 || manifest.m_iAcceptedVehicleCount != 0
			|| manifest.m_aVehicles.Count() != 0 || manifest.m_aAssets.Count() != 0
			|| manifest.m_iMoneyCost != 0 || manifest.m_iHRCost != 0
			|| manifest.m_iEquipmentCost != 0 || manifest.m_iAttackResourceCost < 0
			|| manifest.m_iSupportResourceCost != 0)
			return "exact enemy patrol restore manifest cost or count shape conflicts";
		if (!movement.IsSupportedExactInfantryManifest(manifest)
			|| manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0]
			|| manifest.m_aGroups[0].m_iExpectedMemberCount != manifest.m_iAcceptedMemberCount)
			return "exact enemy patrol restore manifest is not one complete infantry root";
		if (manifest.m_iAcceptedMemberCount != order.m_iCompositionManpower
			|| manifest.m_iAttackResourceCost != order.m_iAttackCost
			|| manifest.m_iSupportResourceCost != order.m_iSupportCost
			|| order.m_iSupportCost != 0)
			return "exact enemy patrol restore manifest or proactive ledger conflicts";
		HST_ForceManifestGroupState root = manifest.m_aGroups[0];
		if (root.m_sElementId != manifest.m_sManifestId + "_group_1"
			|| root.m_sPrefab.IsEmpty() || root.m_sPrefab != manifest.m_sGroupPrefab
			|| root.m_iOrdinal != 0 || !root.m_bRequired)
			return "exact enemy patrol restore group root is invalid";
		for (int memberIndex = 0; memberIndex < manifest.m_aMembers.Count(); memberIndex++)
		{
			HST_ForceManifestMemberState member = manifest.m_aMembers[memberIndex];
			if (!member || member.m_sSlotId.IsEmpty() || member.m_sPrefab.IsEmpty()
				|| member.m_sGroupElementId != root.m_sElementId || !member.m_bRequired
				|| member.m_sSlotId != string.Format("%1_member_%2", manifest.m_sManifestId, memberIndex + 1)
				|| member.m_iOrdinal != memberIndex || member.m_sCatalogSlotId.IsEmpty()
				|| member.m_iMoneyCost != 0 || member.m_iHRCost != 0 || member.m_iEquipmentCost != 0
				|| CountManifestMembers(manifest, member.m_sSlotId) != 1)
				return "exact enemy patrol restore member roster conflicts";
		}
		if (order.m_iCompositionManpower != manifest.m_iAcceptedMemberCount
			|| order.m_iCompositionVehicleCount != 0 || order.m_iCompositionArmedVehicleCount != 0
			|| order.m_sCompositionIntentId != HST_EnemyPatrolOperationService.EXACT_MANIFEST_INTENT
			|| order.m_sCompositionTier != "exact")
			return "exact enemy patrol restore order composition conflicts";
		return "";
	}

	protected string ValidateRouteAndLifecycle(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		HST_GeneratedRouteState route = FindUniqueRoute(operation.m_sCurrentRouteId);
		HST_OperationRouteCursorService cursor = new HST_OperationRouteCursorService();
		if (!route || !cursor.IsPatrolRouteContractValid(operation, route))
			return "exact enemy patrol restore frozen route or cursor conflicts";
		if (route.m_sTargetZoneId != order.m_sTargetZoneId)
			return "exact enemy patrol restore route target conflicts";
		if (operation.m_sAssignmentKind != HST_EnemyPatrolOperationService.ASSIGNMENT_KIND
			|| operation.m_sRecallPolicyId != HST_EnemyPatrolOperationService.RECALL_POLICY_ID
			|| operation.m_sSettlementPolicyId != HST_EnemyPatrolOperationService.SETTLEMENT_POLICY_ID)
			return "exact enemy patrol restore assignment or settlement policy conflicts";
		ref array<vector> positions = HST_OperationRouteCursorService.BuildOrderedRoutePositions(route);
		string geometryFailure = ValidateRouteGeometry(operation, positions);
		if (!geometryFailure.IsEmpty())
			return geometryFailure;
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			bool activeDuty = operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
				|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
				|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN;
			if (!activeDuty || operation.m_eResumeDutyState != operation.m_eDutyState
				|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
				|| !operation.m_sSettlementId.IsEmpty())
				return "open exact enemy patrol restore lifecycle conflicts";
			bool strategicPair = operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
				&& (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
					|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING);
			bool livePair = operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
				&& (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
					|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING);
			if (!strategicPair && !livePair)
				return "open exact enemy patrol restore projection pair conflicts";
			string cursorFailure = ValidateOpenRouteCursor(operation, positions);
			if (!cursorFailure.IsEmpty())
				return cursorFailure;
			if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				return "open exact enemy patrol restore order status conflicts";
		}
		return "";
	}

	protected string ValidateRouteGeometry(
		HST_OperationRecordState operation,
		array<vector> positions)
	{
		if (!operation || !positions || positions.Count() < 2
			|| operation.m_iRouteLegSequence < 1
			|| operation.m_iRouteLapCount < 0
			|| operation.m_iRouteLoopStartedAtSecond < 0
			|| operation.m_iRouteLoopCompletedAtSecond < 0)
			return "exact enemy patrol restore route cursor metadata conflicts";
		if (Math.AbsFloat(operation.m_fStrategicSpeedMetersPerSecond
			- HST_OperationRouteCursorService.DEFAULT_INFANTRY_SPEED_METERS_PER_SECOND) > 0.01)
			return "exact enemy patrol restore strategic speed conflicts";
		float expectedDistance = Distance2D(operation.m_vRouteStartPosition, operation.m_vRouteEndPosition);
		if (Math.AbsFloat(operation.m_fRouteTotalDistanceMeters - expectedDistance) > 1.0
			|| operation.m_fRouteProgressMeters < -0.01
			|| operation.m_fRouteProgressMeters > operation.m_fRouteTotalDistanceMeters + 0.01)
			return "exact enemy patrol restore route leg geometry conflicts";
		if (operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
		{
			vector expectedPosition = HST_OperationRouteCursorService.ResolvePosition(operation);
			if (!PositionsMatch(operation.m_vStrategicPosition, expectedPosition, 2.0))
				return "exact enemy patrol restore strategic cursor position conflicts";
		}
		return "";
	}

	protected string ValidateOpenRouteCursor(
		HST_OperationRecordState operation,
		array<vector> positions)
	{
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
		{
			if (operation.m_iRouteWaypointIndex != 0 || operation.m_iRouteLapCount != 0
				|| !PositionsMatch(operation.m_vRouteEndPosition, positions[0])
				|| operation.m_iRouteLoopCompletedAtSecond != 0)
				return "exact enemy patrol restore outbound cursor conflicts";
			return "";
		}
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
		{
			int waypointIndex = operation.m_iRouteWaypointIndex;
			if (waypointIndex < 0 || waypointIndex >= positions.Count()
				|| !PositionsMatch(operation.m_vRouteEndPosition, positions[waypointIndex])
				|| operation.m_iRouteLapCount > HST_EnemyPatrolOperationService.REQUIRED_PATROL_LAPS)
				return "exact enemy patrol restore loop cursor conflicts";
			return "";
		}
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
		{
			if (operation.m_iRouteWaypointIndex != -1
				|| operation.m_iRouteLapCount < HST_EnemyPatrolOperationService.REQUIRED_PATROL_LAPS
				|| !PositionsMatch(operation.m_vRouteEndPosition, operation.m_vOriginPosition))
				return "exact enemy patrol restore return cursor conflicts";
			return "";
		}
		return "exact enemy patrol restore duty has no route cursor owner";
	}

	protected string ValidateRuntime(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		bool anyOperationRuntimeId = !operation.m_sSpawnResultId.IsEmpty()
			|| !operation.m_sForceId.IsEmpty() || !operation.m_sProjectionId.IsEmpty()
			|| !operation.m_sGroupId.IsEmpty();
		bool allOperationRuntimeIds = !operation.m_sSpawnResultId.IsEmpty()
			&& !operation.m_sForceId.IsEmpty() && !operation.m_sProjectionId.IsEmpty()
			&& !operation.m_sGroupId.IsEmpty();
		if (anyOperationRuntimeId != allOperationRuntimeIds)
			return "exact enemy patrol restore contains partial runtime identity";
		bool executionLinked = allOperationRuntimeIds;
		if (executionLinked != order.m_bStrategicServiceCommitted)
			return "exact enemy patrol restore service-commit authority conflicts";
		if (!order.m_bStrategicServiceCommitted)
		{
			if (batch || group || !order.m_sSpawnResultId.IsEmpty() || !order.m_sGroupId.IsEmpty())
				return "uncommitted exact enemy patrol restore contains runtime authority";
			return "";
		}
		if (order.m_sSpawnResultId != operation.m_sSpawnResultId
			|| order.m_sGroupId != operation.m_sGroupId
			|| operation.m_sSpawnResultId != "spawn_" + order.m_sOrderId
			|| operation.m_sForceId != "force_" + order.m_sOperationId
			|| operation.m_sProjectionId != "projection_" + order.m_sOperationId
			|| operation.m_sGroupId != operation.m_sProjectionId)
			return "exact enemy patrol restore deterministic runtime identity conflicts";
		bool settled = operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		if (settled && !batch && !group)
			return "";
		if (settled && (!batch || !group))
			return "settled exact enemy patrol restore contains partial runtime residue";
		if (!batch || !group || CountBatchesByAnyIdentity(order, batch) != 1
			|| CountGroupsByAnyIdentity(order, group) != 1)
			return "exact enemy patrol restore runtime identity is incomplete or ambiguous";
		if (operation.m_sSpawnResultId != batch.m_sResultId
			|| operation.m_sGroupId != group.m_sGroupId
			|| operation.m_sForceId != batch.m_sForceId
			|| operation.m_sProjectionId != batch.m_sProjectionId
			|| operation.m_sForceId != group.m_sForceId
			|| operation.m_sProjectionId != group.m_sProjectionId
			|| group.m_sGroupId != group.m_sProjectionId)
			return "exact enemy patrol restore projection backlinks conflict";
		if (batch.m_sOperationId != operation.m_sOperationId
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| batch.m_sRequestId != order.m_sOrderId
			|| batch.m_sManifestHash != manifest.m_sManifestHash)
			return "exact enemy patrol restore batch backlinks conflict";
		if (group.m_sOperationId != operation.m_sOperationId
			|| group.m_sEnemyOrderId != order.m_sOrderId
			|| group.m_sManifestId != manifest.m_sManifestId
			|| group.m_sSpawnResultId != batch.m_sResultId
			|| group.m_sRouteId != operation.m_sCurrentRouteId)
			return "exact enemy patrol restore group backlinks or role conflicts";
		if (group.m_sFactionKey != order.m_sFactionKey
			|| group.m_sZoneId != order.m_sTargetZoneId
			|| group.m_sPrefab != manifest.m_aGroups[0].m_sPrefab
			|| (group.m_sSpawnFallbackMode != HST_EnemyPatrolOperationService.EXACT_GROUP_MODE
				&& !group.m_sSpawnFallbackMode.StartsWith(HST_EnemyPatrolOperationService.EXACT_GROUP_MODE + "_"))
			|| group.m_bQRF)
			return "exact enemy patrol restore group backlinks or role conflicts";
		if (operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& IsZeroVector(group.m_vPosition))
			return "exact enemy patrol restore live position authority has no group position";
		if (operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& !PositionsMatch(group.m_vPosition, operation.m_vStrategicPosition, 2.0))
			return "exact enemy patrol restore strategic group position conflicts";
		return ValidateBatchSlotBijection(operation, manifest, batch);
	}

	protected string ValidateBatchSlotBijection(
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		int expectedSlots = manifest.m_aMembers.Count() + 1;
		if (batch.m_iExpectedSlotCount != expectedSlots || batch.m_aSlotResults.Count() != expectedSlots)
			return "exact enemy patrol restore batch slot count conflicts";
		string rootSlotId = manifest.m_aGroups[0].m_sElementId;
		if (CountBatchSlots(batch, rootSlotId, HST_ForceSpawnQueueService.SLOT_KIND_GROUP) != 1)
			return "exact enemy patrol restore group-root slot conflicts";
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member || CountBatchSlots(batch, member.m_sSlotId, HST_ForceSpawnQueueService.SLOT_KIND_MEMBER) != 1)
				return "exact enemy patrol restore member-slot bijection conflicts";
		}
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_sSlotId.IsEmpty() || slot.m_sProjectionId != batch.m_sProjectionId)
				return "exact enemy patrol restore batch slot identity conflicts";
			bool rootSlot = slot.m_sSlotId == rootSlotId
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP;
			bool memberSlot = manifest.FindMemberSlot(slot.m_sSlotId) != null
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER;
			if (!rootSlot && !memberSlot)
				return "exact enemy patrol restore contains a foreign batch slot";
			if (slot.m_bCasualtyConfirmed)
			{
				if (!memberSlot || slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
					|| !slot.m_bEverAlive || slot.m_iCasualtyAtSecond < batch.m_iCreatedAtSecond)
					return "exact enemy patrol restore casualty tombstone conflicts";
			}
			else if (memberSlot && slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				return "open exact enemy patrol restore contains an unproven retired member";
		}
		return "";
	}

	protected string ValidateSettlement(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		if (order.m_bResourceRefundApplied)
			return "exact enemy patrol restore uses the legacy refund flag";
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			if (order.m_bResourceSettlementApplied || !order.m_sResourceSettlementId.IsEmpty()
				|| !order.m_sResourceSettlementKind.IsEmpty() || order.m_iRefundedAttackResources != 0
				|| order.m_iRefundedSupportResources != 0)
				return "open exact enemy patrol restore contains settlement authority";
			return "";
		}
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			|| !order.m_bResourceSettlementApplied || operation.m_sSettlementId.IsEmpty()
			|| operation.m_sSettlementId != order.m_sResourceSettlementId
			|| order.m_sResourceSettlementId != HST_OperationService.BuildSettlementId(
				order.m_sOperationId,
				order.m_sResourceSettlementKind))
			return "settled exact enemy patrol restore receipt conflicts";
		if (order.m_iSettlementAcceptedMemberCount != manifest.m_iAcceptedMemberCount
			|| order.m_iSettlementSurvivorMemberCount < 0
			|| order.m_iSettlementSurvivorMemberCount > order.m_iSettlementAcceptedMemberCount
			|| order.m_iRefundedSupportResources != 0)
			return "settled exact enemy patrol restore survivor receipt conflicts";
		HST_EOperationTerminalResult expectedTerminal;
		bool fullRefund;
		if (!ResolveSettlementPolicy(order.m_sResourceSettlementKind, expectedTerminal, fullRefund)
			|| operation.m_eTerminalResult != expectedTerminal)
			return "settled exact enemy patrol restore settlement kind or terminal result conflicts";
		if (fullRefund && order.m_iSettlementSurvivorMemberCount != order.m_iSettlementAcceptedMemberCount)
			return "full exact enemy patrol refund does not retain the complete admitted roster";
		if (expectedTerminal == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED
			&& order.m_iSettlementSurvivorMemberCount != 0)
			return "destroyed exact enemy patrol restore retains survivors";
		bool rosterAuthoritative;
		int durableSurvivors = ResolveBatchSettlementSurvivors(manifest, batch, rosterAuthoritative);
		if (rosterAuthoritative && durableSurvivors != order.m_iSettlementSurvivorMemberCount)
			return "settled exact enemy patrol survivor receipt conflicts with durable slots";
		int expectedAttackRefund = Math.Max(0, order.m_iAttackCost)
			* order.m_iSettlementSurvivorMemberCount / order.m_iSettlementAcceptedMemberCount;
		if (fullRefund)
			expectedAttackRefund = Math.Max(0, order.m_iAttackCost);
		if (order.m_iRefundedAttackResources != expectedAttackRefund)
			return "settled exact enemy patrol restore proactive refund conflicts";
		if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return "settled exact enemy patrol restore terminal lifecycle conflicts";
		bool completed = operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
			|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED;
		if ((completed && order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED)
			|| (!completed && order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED))
			return "settled exact enemy patrol restore order status conflicts";
		return "";
	}

	protected bool ResolveSettlementPolicy(
		string settlementKind,
		out HST_EOperationTerminalResult terminalResult,
		out bool fullRefund)
	{
		terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN;
		fullRefund = false;
		if (settlementKind == "admission_failed_full"
			|| settlementKind == "restore_admission_interrupted_full")
		{
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED;
			fullRefund = true;
			return true;
		}
		if (settlementKind == "virtual_projection_failed_survivors"
			|| settlementKind == "materialization_failed_survivors")
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED;
		else if (settlementKind == "invalidated_survivors")
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED;
		else if (settlementKind == "route_failed_survivors")
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED;
		else if (settlementKind == "returned_survivors")
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED;
		else if (settlementKind == "destroyed")
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED;
		else
			return false;
		return true;
	}

	protected int ResolveBatchSettlementSurvivors(
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		out bool authoritative)
	{
		authoritative = false;
		if (!manifest || !batch)
			return 0;
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		int survivors;
		if (batch.m_bStrategicProjectionHeld)
		{
			survivors = queue.CountStrategicLivingMemberSlots(batch);
			authoritative = true;
		}
		else if (batch.m_iSuccessfulHandoffCount > 0)
		{
			survivors = queue.CountDurableLivingMemberSlots(batch);
			authoritative = true;
		}
		return Math.Max(0, Math.Min(manifest.m_iAcceptedMemberCount, survivors));
	}

	protected void NormalizeValidAggregate(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			NormalizeSettledRuntime(operation, batch, group);
			return;
		}
		int restoreSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		bool savedLive = operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING);
		if (savedLive && group && !IsZeroVector(group.m_vPosition))
		{
			HST_OperationRouteCursorService cursor = new HST_OperationRouteCursorService();
			cursor.SyncLegFromPositionAtSecond(restoreSecond, operation, group, group.m_vPosition);
		}
		else if (group && !IsZeroVector(operation.m_vStrategicPosition))
		{
			group.m_vPosition = operation.m_vStrategicPosition;
			group.m_vSourcePosition = operation.m_vStrategicPosition;
			group.m_vTargetPosition = operation.m_vRouteEndPosition;
		}
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_iMaterializationStateEnteredAtSecond = restoreSecond;
		operation.m_iStrategicLastUpdateSecond = restoreSecond;
		operation.m_iLastProgressAtSecond = restoreSecond;
		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		operation.m_sLastProjectionReason = "restored exact patrol as strategic authority";
		operation.m_iRevision++;
		NormalizeBatchForStrategicHold(batch, restoreSecond);
		NormalizeGroupForStrategicHold(order, operation, batch, group);
		order.m_bPhysicalized = false;
		order.m_bAbstractResolved = false;
		order.m_sRuntimeStatus = "exact_patrol_restore_virtual";
	}

	protected void NormalizeBatchForStrategicHold(HST_ForceSpawnResultState batch, int restoreSecond)
	{
		if (!batch || batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
			return;
		bool wasPhysical = batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			&& !batch.m_bStrategicProjectionHeld;
		batch.m_sNativeGroupId = "";
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING;
		batch.m_bStrategicProjectionHeld = true;
		if (wasPhysical)
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
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!group)
			return;
		group.m_bSpawnedEntity = false;
		group.m_bSpawnAttempted = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
			group.m_sRuntimeStatus = "enemy_patrol_virtual_returning";
		else
			group.m_sRuntimeStatus = "enemy_patrol_virtual";
		if (!batch)
			return;
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		int living = queue.CountStrategicLivingMemberSlots(batch);
		operation.m_iLastVirtualFriendlyCount = Math.Max(0, living);
		group.m_iDurableLivingInfantryCount = Math.Max(0, living);
		group.m_iLastSeenAliveCount = Math.Max(0, living);
		group.m_iSurvivorInfantryCount = Math.Max(0, living);
		group.m_iInfantryCount = Math.Max(0, living);
	}

	protected void NormalizeSettledRuntime(
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (batch)
		{
			batch.m_sNativeGroupId = "";
			batch.m_bStrategicProjectionHeld = false;
		}
		if (!group)
			return;
		group.m_bSpawnedEntity = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
		group.m_sRuntimeStatus = "retired";
	}

	protected void Quarantine(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		order.m_iOperationContractVersion = HST_EnemyPatrolOperationService.QUARANTINED_CONTRACT_VERSION;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		order.m_bPhysicalized = false;
		order.m_sRuntimeStatus = "exact_patrol_quarantined";
		order.m_sFailureReason = reason;
		if (operation && operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
			operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
			operation.m_sLastProjectionReason = reason;
			operation.m_iRevision++;
		}
		if (batch)
		{
			batch.m_bStrategicProjectionHeld = true;
			batch.m_bCancelRequested = true;
			batch.m_sLastFailureReason = reason;
		}
		if (group)
		{
			group.m_bSpawnedEntity = false;
			group.m_sRuntimeEntityId = "";
			group.m_iSpawnedAgentCount = 0;
			group.m_iAssignedWaypointCount = 0;
			group.m_sRuntimeStatus = "exact_patrol_quarantined";
			group.m_sSpawnFailureReason = reason;
		}
		HoldAllPatrolClaimants(order, operation, reason);
	}

	protected void NormalizeQuarantinedOrder(HST_EnemyOrderState order)
	{
		HST_OperationRecordState operation = FindUniqueOperation(order.m_sOperationId);
		HST_ForceSpawnResultState batch = FindUniqueBatch(order.m_sSpawnResultId);
		HST_ActiveGroupState group = FindUniqueGroup(order.m_sGroupId);
		string reason = order.m_sFailureReason;
		if (reason.IsEmpty())
			reason = "schema-53 exact patrol remained quarantined after restore";
		Quarantine(order, operation, batch, group, reason);
	}

	protected void PreserveOrphanClaimants()
	{
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL)
				continue;
			HST_EnemyOrderState order = FindUniqueOrder(operation.m_sEnemyOrderId);
			if (IsReciprocalOpenOrSettledPatrolOrder(order, operation))
				continue;
			HoldOperationClaimants(operation, "exact patrol operation has no unique reciprocal order");
		}
		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (!IsPatrolManifestClaimant(manifest))
				continue;
			HST_OperationRecordState operation = FindUniqueOperation(manifest.m_sOperationId);
			HST_EnemyOrderState order;
			if (operation)
				order = FindUniqueOrder(operation.m_sEnemyOrderId);
			if (IsReciprocalPatrolChain(order, operation, manifest))
				continue;
			if (operation)
				HoldOperationClaimants(operation, "exact patrol manifest has no unique reciprocal chain");
			HoldManifestClaimants(manifest, "exact patrol manifest has no unique reciprocal chain");
		}
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!batch)
				continue;
			HST_ForceManifestState manifest = FindUniqueManifest(batch.m_sManifestId);
			if (!IsPatrolManifestClaimant(manifest))
				continue;
			HST_OperationRecordState operation = FindUniqueOperation(batch.m_sOperationId);
			HST_EnemyOrderState order;
			if (operation)
				order = FindUniqueOrder(operation.m_sEnemyOrderId);
			HST_ActiveGroupState group;
			if (operation)
				group = FindUniqueGroup(operation.m_sGroupId);
			if (IsReciprocalPatrolRuntime(order, operation, manifest, batch, group))
				continue;
			HoldBatch(batch, "exact patrol batch has no unique reciprocal runtime chain");
			if (group)
				HoldGroup(group, "exact patrol batch has no unique reciprocal runtime chain");
		}
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!IsExactPatrolGroupMode(group))
				continue;
			HST_OperationRecordState operation = FindUniqueOperation(group.m_sOperationId);
			HST_ForceManifestState manifest = FindUniqueManifest(group.m_sManifestId);
			HST_EnemyOrderState order;
			if (operation)
				order = FindUniqueOrder(operation.m_sEnemyOrderId);
			HST_ForceSpawnResultState batch = FindUniqueBatch(group.m_sSpawnResultId);
			if (IsReciprocalPatrolRuntime(order, operation, manifest, batch, group))
				continue;
			HoldGroup(group, "exact patrol group has no unique reciprocal runtime chain");
			if (batch)
				HoldBatch(batch, "exact patrol group has no unique reciprocal runtime chain");
		}
	}

	protected bool IsExactPatrolManifest(HST_ForceManifestState manifest)
	{
		return manifest && manifest.m_sForceKind == HST_EnemyPatrolOperationService.EXACT_FORCE_KIND
			&& manifest.m_sPolicyId == HST_EnemyPatrolOperationService.EXACT_POLICY_ID
			&& manifest.m_sIntentId == HST_EnemyPatrolOperationService.EXACT_MANIFEST_INTENT;
	}

	protected bool IsPatrolManifestClaimant(HST_ForceManifestState manifest)
	{
		return manifest && (manifest.m_sForceKind == HST_EnemyPatrolOperationService.EXACT_FORCE_KIND
			|| manifest.m_sPolicyId == HST_EnemyPatrolOperationService.EXACT_POLICY_ID
			|| manifest.m_sIntentId == HST_EnemyPatrolOperationService.EXACT_MANIFEST_INTENT);
	}

	protected bool IsExactPatrolGroupMode(HST_ActiveGroupState group)
	{
		return group && (group.m_sSpawnFallbackMode == HST_EnemyPatrolOperationService.EXACT_GROUP_MODE
			|| group.m_sSpawnFallbackMode.StartsWith(HST_EnemyPatrolOperationService.EXACT_GROUP_MODE + "_")
			|| group.m_sRuntimeStatus.StartsWith("enemy_patrol_")
			|| group.m_sRuntimeStatus == "exact_patrol_quarantined"
			|| group.m_sRuntimeStatus == "exact_patrol_orphan_quarantined");
	}

	protected bool IsReciprocalPatrolChain(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		return IsReciprocalOpenOrSettledPatrolOrder(order, operation)
			&& IsExactPatrolManifest(manifest)
			&& operation.m_sManifestId == manifest.m_sManifestId
			&& manifest.m_sOperationId == operation.m_sOperationId;
	}

	protected bool IsReciprocalPatrolRuntime(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!IsReciprocalPatrolChain(order, operation, manifest) || !batch || !group
			|| !order.m_bStrategicServiceCommitted)
			return false;
		if (order.m_sSpawnResultId != batch.m_sResultId || order.m_sGroupId != group.m_sGroupId
			|| operation.m_sSpawnResultId != batch.m_sResultId
			|| operation.m_sGroupId != group.m_sGroupId)
			return false;
		if (batch.m_sOperationId != operation.m_sOperationId
			|| batch.m_sManifestId != manifest.m_sManifestId)
			return false;
		return group.m_sOperationId == operation.m_sOperationId
			&& group.m_sManifestId == manifest.m_sManifestId
			&& group.m_sSpawnResultId == batch.m_sResultId;
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

	protected bool IsReciprocalOpenOrSettledPatrolOrder(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		return order && operation
			&& order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL
			&& order.m_iOperationContractVersion == HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION
			&& order.m_sOperationId == operation.m_sOperationId
			&& order.m_sOrderId == operation.m_sEnemyOrderId
			&& order.m_sManifestId == operation.m_sManifestId;
	}

	protected void HoldAllPatrolClaimants(
		HST_EnemyOrderState order,
		HST_OperationRecordState knownOperation,
		string reason)
	{
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!OperationClaimsOrder(operation, order, knownOperation))
				continue;
			HoldOperationClaimants(operation, reason);
		}
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (BatchClaimsOrder(batch, order, knownOperation))
				HoldBatch(batch, reason);
		}
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (GroupClaimsOrder(group, order, knownOperation))
				HoldGroup(group, reason);
		}
	}

	protected void HoldOperationClaimants(HST_OperationRecordState operation, string reason)
	{
		if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL)
			return;
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
			operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
			operation.m_sLastProjectionReason = reason;
			operation.m_iRevision++;
		}
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (batch && ((!operation.m_sOperationId.IsEmpty() && batch.m_sOperationId == operation.m_sOperationId)
				|| (!operation.m_sSpawnResultId.IsEmpty() && batch.m_sResultId == operation.m_sSpawnResultId)
				|| (!operation.m_sManifestId.IsEmpty() && batch.m_sManifestId == operation.m_sManifestId)
				|| (!operation.m_sForceId.IsEmpty() && batch.m_sForceId == operation.m_sForceId)
				|| (!operation.m_sProjectionId.IsEmpty() && batch.m_sProjectionId == operation.m_sProjectionId)))
				HoldBatch(batch, reason);
		}
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (group && ((!operation.m_sOperationId.IsEmpty() && group.m_sOperationId == operation.m_sOperationId)
				|| (!operation.m_sManifestId.IsEmpty() && group.m_sManifestId == operation.m_sManifestId)
				|| (!operation.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == operation.m_sSpawnResultId)
				|| (!operation.m_sForceId.IsEmpty() && group.m_sForceId == operation.m_sForceId)
				|| (!operation.m_sProjectionId.IsEmpty() && group.m_sProjectionId == operation.m_sProjectionId)
				|| (!operation.m_sGroupId.IsEmpty() && group.m_sGroupId == operation.m_sGroupId)))
				HoldGroup(group, reason);
		}
	}

	protected bool OperationClaimsOrder(
		HST_OperationRecordState operation,
		HST_EnemyOrderState order,
		HST_OperationRecordState knownOperation)
	{
		if (!operation || !order || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL)
			return false;
		return operation == knownOperation
			|| (!order.m_sOrderId.IsEmpty() && operation.m_sEnemyOrderId == order.m_sOrderId)
			|| (!order.m_sOperationId.IsEmpty() && operation.m_sOperationId == order.m_sOperationId)
			|| (!order.m_sManifestId.IsEmpty() && operation.m_sManifestId == order.m_sManifestId)
			|| (!order.m_sSpawnResultId.IsEmpty() && operation.m_sSpawnResultId == order.m_sSpawnResultId)
			|| (!order.m_sGroupId.IsEmpty() && operation.m_sGroupId == order.m_sGroupId);
	}

	protected bool BatchClaimsOrder(
		HST_ForceSpawnResultState batch,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		if (!batch || !order)
			return false;
		if (!order.m_sOrderId.IsEmpty() && batch.m_sRequestId == order.m_sOrderId)
			return true;
		if (!order.m_sOperationId.IsEmpty() && batch.m_sOperationId == order.m_sOperationId)
			return true;
		if (!order.m_sManifestId.IsEmpty() && batch.m_sManifestId == order.m_sManifestId)
			return true;
		if (!order.m_sSpawnResultId.IsEmpty() && batch.m_sResultId == order.m_sSpawnResultId)
			return true;
		if (!operation)
			return false;
		return (!operation.m_sForceId.IsEmpty() && batch.m_sForceId == operation.m_sForceId)
			|| (!operation.m_sProjectionId.IsEmpty() && batch.m_sProjectionId == operation.m_sProjectionId);
	}

	protected bool GroupClaimsOrder(
		HST_ActiveGroupState group,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		if (!group || !order)
			return false;
		if (!order.m_sOrderId.IsEmpty() && group.m_sEnemyOrderId == order.m_sOrderId)
			return true;
		if (!order.m_sOperationId.IsEmpty() && group.m_sOperationId == order.m_sOperationId)
			return true;
		if (!order.m_sManifestId.IsEmpty() && group.m_sManifestId == order.m_sManifestId)
			return true;
		if (!order.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == order.m_sSpawnResultId)
			return true;
		if (!order.m_sGroupId.IsEmpty() && group.m_sGroupId == order.m_sGroupId)
			return true;
		if (!operation)
			return false;
		return (!operation.m_sForceId.IsEmpty() && group.m_sForceId == operation.m_sForceId)
			|| (!operation.m_sProjectionId.IsEmpty() && group.m_sProjectionId == operation.m_sProjectionId);
	}

	protected void HoldBatch(HST_ForceSpawnResultState batch, string reason)
	{
		if (!batch)
			return;
		batch.m_bStrategicProjectionHeld = true;
		batch.m_bCancelRequested = true;
		batch.m_sLastFailureReason = reason;
	}

	protected void HoldGroup(HST_ActiveGroupState group, string reason)
	{
		if (!group)
			return;
		group.m_bSpawnedEntity = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
		group.m_sSpawnFallbackMode = HST_EnemyPatrolOperationService.EXACT_GROUP_MODE + "_quarantined";
		group.m_sRuntimeStatus = "exact_patrol_orphan_quarantined";
		group.m_sSpawnFailureReason = reason;
	}

	protected HST_EnemyOrderState FindUniqueOrder(string orderId)
	{
		HST_EnemyOrderState match;
		foreach (HST_EnemyOrderState order : m_SaveData.m_aEnemyOrders)
		{
			if (!order || order.m_sOrderId != orderId)
				continue;
			if (match)
				return null;
			match = order;
		}
		return match;
	}

	protected HST_OperationRecordState FindUniqueOperation(string operationId)
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

	protected HST_ForceManifestState FindUniqueManifest(string manifestId)
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

	protected int CountOrdersByAnyIdentity(HST_EnemyOrderState expected)
	{
		int count;
		foreach (HST_EnemyOrderState order : m_SaveData.m_aEnemyOrders)
		{
			if (!order)
				continue;
			bool matches = order.m_sOrderId == expected.m_sOrderId;
			if (!matches && !expected.m_sOperationId.IsEmpty())
				matches = order.m_sOperationId == expected.m_sOperationId;
			if (!matches && !expected.m_sManifestId.IsEmpty())
				matches = order.m_sManifestId == expected.m_sManifestId;
			if (!matches && !expected.m_sSpawnResultId.IsEmpty())
				matches = order.m_sSpawnResultId == expected.m_sSpawnResultId;
			if (!matches && !expected.m_sGroupId.IsEmpty())
				matches = order.m_sGroupId == expected.m_sGroupId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountOperationsByAnyIdentity(
		HST_EnemyOrderState order,
		HST_OperationRecordState expected)
	{
		int count;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!operation)
				continue;
			bool matches = operation.m_sOperationId == expected.m_sOperationId;
			if (!matches && !order.m_sOrderId.IsEmpty())
				matches = operation.m_sEnemyOrderId == order.m_sOrderId;
			if (!matches && !order.m_sManifestId.IsEmpty())
				matches = operation.m_sManifestId == order.m_sManifestId;
			if (!matches && !order.m_sSpawnResultId.IsEmpty())
				matches = operation.m_sSpawnResultId == order.m_sSpawnResultId;
			if (!matches && !order.m_sGroupId.IsEmpty())
				matches = operation.m_sGroupId == order.m_sGroupId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountManifestsByAnyIdentity(
		HST_EnemyOrderState order,
		HST_ForceManifestState expected)
	{
		int count;
		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (!manifest)
				continue;
			if (manifest.m_sManifestId == expected.m_sManifestId
				|| manifest.m_sOperationId == order.m_sOperationId)
				count++;
		}
		return count;
	}

	protected int CountBatchesByAnyIdentity(
		HST_EnemyOrderState order,
		HST_ForceSpawnResultState expected)
	{
		int count;
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!batch)
				continue;
			bool matches = batch.m_sResultId == expected.m_sResultId;
			if (!matches)
				matches = batch.m_sOperationId == order.m_sOperationId;
			if (!matches)
				matches = batch.m_sManifestId == order.m_sManifestId;
			if (!matches)
				matches = batch.m_sRequestId == order.m_sOrderId;
			if (!matches && !expected.m_sProjectionId.IsEmpty())
				matches = batch.m_sProjectionId == expected.m_sProjectionId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountGroupsByAnyIdentity(
		HST_EnemyOrderState order,
		HST_ActiveGroupState expected)
	{
		int count;
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!group)
				continue;
			bool matches = group.m_sGroupId == expected.m_sGroupId;
			if (!matches)
				matches = group.m_sOperationId == order.m_sOperationId;
			if (!matches)
				matches = group.m_sManifestId == order.m_sManifestId;
			if (!matches && !order.m_sSpawnResultId.IsEmpty())
				matches = group.m_sSpawnResultId == order.m_sSpawnResultId;
			if (!matches && !expected.m_sProjectionId.IsEmpty())
				matches = group.m_sProjectionId == expected.m_sProjectionId;
			if (matches)
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

	protected int CountBatchSlots(HST_ForceSpawnResultState batch, string slotId, string slotKind)
	{
		int count;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotId == slotId && slot.m_sSlotKind == slotKind)
				count++;
		}
		return count;
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

	protected bool IsZeroVector(vector value)
	{
		return Math.AbsFloat(value[0]) < 0.01
			&& Math.AbsFloat(value[1]) < 0.01
			&& Math.AbsFloat(value[2]) < 0.01;
	}

	protected bool PositionsMatch(vector left, vector right, float toleranceMeters = 1.0)
	{
		return Distance2D(left, right) <= Math.Max(0.01, toleranceMeters);
	}

	protected float Distance2D(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}
}
