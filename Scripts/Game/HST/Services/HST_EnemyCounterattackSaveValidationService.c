// Conservative Schema-69 restore boundary for exact enemy counterattacks.
// Historical rows remain on contract zero. Corrupt current exact graphs retain
// their evidence under quarantine and never fall back to the legacy consumer.
class HST_EnemyCounterattackSaveValidationService
{
	static const int SCHEMA_VERSION = 69;
	static const int QUARANTINED_CONTRACT_VERSION = -69;
	static const string EXACT_GROUP_MODE = "exact_enemy_counterattack";

	protected HST_CampaignSaveData m_SaveData;

	void Normalize(HST_CampaignSaveData saveData, int restoredSchemaVersion)
	{
		m_SaveData = saveData;
		if (!m_SaveData)
			return;

		if (restoredSchemaVersion < SCHEMA_VERSION)
		{
			PreserveHistoricalCounterattacks();
			PreserveOrphanClaimants();
			return;
		}

		foreach (HST_EnemyOrderState order : m_SaveData.m_aEnemyOrders)
		{
			if (!order || order.m_eType != HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK
				|| order.m_iOperationContractVersion == 0)
				continue;

			if (order.m_iOperationContractVersion == QUARANTINED_CONTRACT_VERSION)
			{
				NormalizeQuarantinedOrder(order);
				continue;
			}

			HST_OperationRecordState operation = FindUniqueOperation(order.m_sOperationId);
			HST_ForceManifestState manifest = FindUniqueManifest(order.m_sManifestId);
			HST_ForceSpawnResultState batch = FindUniqueBatch(order.m_sSpawnResultId);
			HST_ActiveGroupState group = FindUniqueGroup(order.m_sGroupId);
			bool uncommittedFull = IsUncommittedFullSettlementCandidate(order);
			string failure;
			if (uncommittedFull)
				failure = ValidateUncommittedFullSettlementAggregate(
					order,
					operation,
					manifest);
			else
				failure = ValidateAggregate(order, operation, manifest, batch, group);
			if (!failure.IsEmpty())
			{
				Quarantine(order, operation, batch, group, failure);
				continue;
			}

			if (uncommittedFull)
			{
				if (operation && operation.m_eSettlementState
					== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
					NormalizeSettledRuntime(batch, group);
				continue;
			}
			NormalizeValidAggregate(order, operation, batch, group);
		}

		PreserveOrphanClaimants();
	}

	protected void PreserveHistoricalCounterattacks()
	{
		int preserved;
		int quarantined;
		foreach (HST_EnemyOrderState order : m_SaveData.m_aEnemyOrders)
		{
			if (!order || order.m_eType != HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
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
				"pre-schema-69 counterattack carried an unsupported nonzero operation contract");
			quarantined++;
		}

		if (HasEvent("migration_schema69_enemy_counterattack_authority"))
			return;

		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = "migration_schema69_enemy_counterattack_authority";
		eventState.m_sCategory = "migration";
		eventState.m_sAggregateType = "operation_record";
		eventState.m_sAggregateId = "schema69";
		eventState.m_sTransition = "legacy_enemy_counterattacks_preserved_contract_zero";
		eventState.m_sReason = string.Format(
			"preserved %1 historical counterattack orders at contract zero and quarantined %2 unsupported nonzero rows; created no source, manifest, roster, operation, debit, refund, or outcome",
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
			return "exact enemy counterattack restore authority is incomplete or ambiguous";

		string failure = ValidateIdentity(order, operation, manifest);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateManifest(order, manifest);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateOriginalResourceDebitAuthority(
			m_SaveData.m_aEnemyStrategicMutations,
			order);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateCompletedOwnershipAuthority(
			m_SaveData.m_aZones,
			m_SaveData.m_aOwnershipTransitions,
			order,
			operation);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateRouteAndLifecycle(order, operation);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateRuntime(order, operation, manifest, batch, group);
		if (!failure.IsEmpty())
			return failure;
		return ValidateSettlement(order, operation, manifest, batch, group);
	}

	protected bool IsUncommittedFullSettlementCandidate(HST_EnemyOrderState order)
	{
		return order && !order.m_bStrategicServiceCommitted
			&& IsFullRefundSettlementKind(order.m_sResourceSettlementKind);
	}

	protected string ValidateUncommittedFullSettlementAggregate(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		if (!order || order.m_iOperationContractVersion
			!= HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION
			|| order.m_eType != HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK
			|| order.m_sOrderId.IsEmpty()
			|| order.m_sOperationId
				!= HST_StableIdService.BuildOperationId("enemy_order", order.m_sOrderId)
			|| order.m_sFactionKey.IsEmpty() || order.m_sSourceZoneId.IsEmpty()
			|| order.m_sTargetZoneId.IsEmpty()
			|| order.m_sSourceZoneId == order.m_sTargetZoneId
			|| IsZeroVector(order.m_vSourcePosition)
			|| IsZeroVector(order.m_vTargetPosition))
			return "uncommitted full exact enemy counterattack identity is invalid";
		if (order.m_bStrategicServiceCommitted || !order.m_sSpawnResultId.IsEmpty()
			|| !order.m_sGroupId.IsEmpty() || order.m_bPhysicalized
			|| order.m_bOutcomeApplied
			|| order.m_sResolutionKind == "exact_counterattack_recaptured")
			return "uncommitted full exact enemy counterattack retained committed or ownership authority";
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
			&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
			return "uncommitted full exact enemy counterattack order lifecycle is invalid";
		if (order.m_iCompositionManpower <= 0
			|| order.m_iSettlementAcceptedMemberCount != order.m_iCompositionManpower
			|| order.m_sManifestId != "manifest_" + order.m_sOperationId
			|| order.m_sManifestHash.IsEmpty()
			|| order.m_sCompositionIntentId
				!= HST_OperationService.EXACT_ENEMY_COUNTERATTACK_MANIFEST_INTENT
			|| order.m_sCompositionTier != "exact"
			|| order.m_iCompositionVehicleCount != 0
			|| order.m_iCompositionArmedVehicleCount != 0)
			return "uncommitted full exact enemy counterattack admitted roster is invalid";
		if (CountOrdersByAnyIdentity(order) != 1)
			return "uncommitted full exact enemy counterattack order identity is ambiguous";
		int operationClaimants = CountOperationsByAnyIdentity(order, operation);
		if (operationClaimants > 1 || (!operation && operationClaimants > 0))
			return "uncommitted full exact enemy counterattack operation identity is ambiguous";
		int manifestClaimants = CountManifestsByAnyIdentity(order, manifest);
		if (manifestClaimants > 1 || (!manifest && manifestClaimants > 0))
			return "uncommitted full exact enemy counterattack manifest identity is ambiguous";
		if (CountBatchesByAnyIdentity(order, null) > 1)
			return "uncommitted full exact enemy counterattack spawn-result identity is ambiguous";
		if (CountGroupsByAnyIdentity(order, null) > 1)
			return "uncommitted full exact enemy counterattack active-group identity is ambiguous";
		string tupleFailure = ValidatePreparedResourceSettlementTuple(order);
		if (!tupleFailure.IsEmpty())
			return tupleFailure;
		string debitFailure = ValidateOriginalResourceDebitAuthority(
			m_SaveData.m_aEnemyStrategicMutations,
			order);
		if (!debitFailure.IsEmpty())
			return debitFailure;

		HST_EOperationTerminalResult expectedTerminal;
		bool fullRefund;
		bool requiresRecapture;
		if (!ResolveSettlementPolicy(
			order.m_sResourceSettlementKind,
			expectedTerminal,
			fullRefund,
			requiresRecapture) || !fullRefund)
			return "uncommitted full exact enemy counterattack settlement policy is invalid";
		if (operation)
		{
			if (!manifest)
				return "uncommitted full exact enemy counterattack operation lacks its frozen manifest";
			if (CountOperationsByAnyIdentity(order, operation) != 1
				|| operation.m_eType
					!= HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK
				|| operation.m_iContractVersion
					!= HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION
				|| operation.m_sOperationId != order.m_sOperationId
				|| operation.m_sEnemyOrderId != order.m_sOrderId
				|| operation.m_sManifestId != order.m_sManifestId
				|| !operation.m_sSpawnResultId.IsEmpty()
				|| !operation.m_sGroupId.IsEmpty()
				|| operation.m_eTerminalResult != expectedTerminal
				|| operation.m_sSettlementId != order.m_sResourceSettlementId)
				return "uncommitted full exact enemy counterattack operation intent conflicts";
			if (operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
			{
				if (operation.m_sTerminalReason.IsEmpty()
					|| operation.m_iSettledAtSecond <= 0
					|| operation.m_iSettledAtSecond > m_SaveData.m_iElapsedSeconds
					|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
					|| operation.m_eMaterializationState
						== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
					return "uncommitted full exact enemy counterattack prepared operation is invalid";
			}
			else if (operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			{
				if (!order.m_bResourceSettlementApplied
					|| operation.m_eDutyState
						!= HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
					|| operation.m_eMaterializationState
						!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
					return "uncommitted full exact enemy counterattack terminal operation is invalid";
			}
			else
				return "uncommitted full exact enemy counterattack operation lacks prepared terminal intent";
		}
		if (manifest)
		{
			string manifestFailure = ValidateManifest(order, manifest);
			if (!manifestFailure.IsEmpty())
				return manifestFailure;
		}

		int refundClaimants = CountResourceMutationClaimants(
			m_SaveData.m_aEnemyStrategicMutations,
			order,
			false);
		if (refundClaimants == 0)
		{
			if (order.m_bResourceSettlementApplied)
				return "uncommitted full exact enemy counterattack completed a missing refund";
		}
		else if (refundClaimants == 1)
		{
			string refundFailure;
			if (order.m_bResourceSettlementApplied)
				refundFailure = ValidateSettledResourceRefundAuthority(
					m_SaveData.m_aEnemyStrategicMutations,
					order);
			else
				refundFailure = ValidatePendingResourceRefundAuthority(
					m_SaveData.m_aEnemyStrategicMutations,
					order);
			if (!refundFailure.IsEmpty())
				return refundFailure;
		}
		else
			return "uncommitted full exact enemy counterattack refund authority is ambiguous";

		foreach (HST_ForceSpawnResultState candidateBatch : m_SaveData.m_aForceSpawnResults)
		{
			if (!candidateBatch)
				continue;
			bool claimant = candidateBatch.m_sRequestId == order.m_sOrderId
				|| candidateBatch.m_sOperationId == order.m_sOperationId
				|| candidateBatch.m_sManifestId == order.m_sManifestId
				|| candidateBatch.m_sResultId == "spawn_" + order.m_sOrderId
				|| candidateBatch.m_sProjectionId == "projection_" + order.m_sOperationId
				|| candidateBatch.m_sForceId == "force_" + order.m_sOperationId;
			if (!claimant)
				continue;
			bool reciprocal = candidateBatch.m_sResultId == "spawn_" + order.m_sOrderId
				&& candidateBatch.m_sRequestId == order.m_sOrderId
				&& candidateBatch.m_sOperationId == order.m_sOperationId
				&& candidateBatch.m_sManifestId == order.m_sManifestId
				&& candidateBatch.m_sForceId == "force_" + order.m_sOperationId
				&& candidateBatch.m_sProjectionId == "projection_" + order.m_sOperationId;
			if (!reciprocal)
				return "uncommitted full exact enemy counterattack retained a foreign spawn-result claimant";
			if (candidateBatch.m_iSuccessfulHandoffCount != 0
				|| candidateBatch.m_eStatus
					== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
				return "uncommitted full exact enemy counterattack retained a successful runtime handoff";
		}
		foreach (HST_ActiveGroupState candidateGroup : m_SaveData.m_aActiveGroups)
		{
			if (!candidateGroup)
				continue;
			bool claimant = candidateGroup.m_sEnemyOrderId == order.m_sOrderId
				|| candidateGroup.m_sOperationId == order.m_sOperationId
				|| candidateGroup.m_sManifestId == order.m_sManifestId
				|| candidateGroup.m_sGroupId == "projection_" + order.m_sOperationId
				|| candidateGroup.m_sProjectionId == "projection_" + order.m_sOperationId
				|| candidateGroup.m_sSpawnResultId == "spawn_" + order.m_sOrderId
				|| candidateGroup.m_sForceId == "force_" + order.m_sOperationId;
			if (!claimant)
				continue;
			bool reciprocal = candidateGroup.m_sGroupId == "projection_" + order.m_sOperationId
				&& candidateGroup.m_sProjectionId == candidateGroup.m_sGroupId
				&& candidateGroup.m_sForceId == "force_" + order.m_sOperationId
				&& candidateGroup.m_sEnemyOrderId == order.m_sOrderId
				&& candidateGroup.m_sOperationId == order.m_sOperationId
				&& candidateGroup.m_sManifestId == order.m_sManifestId
				&& candidateGroup.m_sSpawnResultId == "spawn_" + order.m_sOrderId;
			if (!reciprocal)
				return "uncommitted full exact enemy counterattack retained a foreign active-group claimant";
			if (candidateGroup.m_bSpawnedEntity
				|| candidateGroup.m_iSpawnedAgentCount > 0)
				return "uncommitted full exact enemy counterattack retained live group authority";
		}
		return "";
	}

	protected string ValidateIdentity(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		if (order.m_iOperationContractVersion != HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION
			|| operation.m_iContractVersion != HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION
			|| order.m_eType != HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK)
			return "exact enemy counterattack restore contract type or version conflicts";
		if (order.m_sOrderId.IsEmpty() || order.m_sOperationId.IsEmpty()
			|| order.m_sOperationId != HST_StableIdService.BuildOperationId("enemy_order", order.m_sOrderId)
			|| operation.m_sOperationId != order.m_sOperationId
			|| operation.m_sEnemyOrderId != order.m_sOrderId
			|| operation.m_sManifestId != order.m_sManifestId
			|| manifest.m_sOperationId != order.m_sOperationId
			|| manifest.m_sManifestId != order.m_sManifestId)
			return "exact enemy counterattack restore reciprocal identity conflicts";
		if (CountOrdersByAnyIdentity(order) != 1
			|| CountOperationsByAnyIdentity(order, operation) != 1
			|| CountManifestsByAnyIdentity(order, manifest) != 1)
			return "exact enemy counterattack restore durable identity is ambiguous";
		if (order.m_sFactionKey.IsEmpty() || order.m_sSourceZoneId.IsEmpty()
			|| order.m_sTargetZoneId.IsEmpty() || order.m_sSourceZoneId == order.m_sTargetZoneId
			|| IsZeroVector(order.m_vSourcePosition) || IsZeroVector(order.m_vTargetPosition))
			return "exact enemy counterattack restore source, target, or faction conflicts";
		if (operation.m_sOwnerFactionKey != order.m_sFactionKey
			|| operation.m_sOriginZoneId != order.m_sSourceZoneId
			|| operation.m_sAssignmentZoneId != order.m_sTargetZoneId
			|| operation.m_sAssignmentKind != HST_OperationService.EXACT_ENEMY_COUNTERATTACK_ASSIGNMENT_KIND
			|| operation.m_sRecallPolicyId != HST_OperationService.EXACT_ENEMY_COUNTERATTACK_RECALL_POLICY
			|| operation.m_sSettlementPolicyId != HST_OperationService.EXACT_ENEMY_COUNTERATTACK_SETTLEMENT_POLICY)
			return "exact enemy counterattack restore source, target, faction, or policy conflicts";
		if (!operation.m_sSupportRequestId.IsEmpty() || !order.m_sSupportRequestId.IsEmpty())
			return "exact enemy counterattack conflicts with the legacy support consumer";
		if (!PositionsMatch(operation.m_vOriginPosition, order.m_vSourcePosition)
			|| !PositionsMatch(operation.m_vAssignmentPosition, order.m_vTargetPosition))
			return "exact enemy counterattack restore source or target position conflicts";
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			bool returning = operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN;
			string expectedTacticalZoneId = operation.m_sAssignmentZoneId;
			vector expectedTacticalPosition = operation.m_vAssignmentPosition;
			if (returning)
			{
				expectedTacticalZoneId = operation.m_sOriginZoneId;
				expectedTacticalPosition = operation.m_vOriginPosition;
			}
			if (operation.m_sTacticalTargetZoneId != expectedTacticalZoneId
				|| !PositionsMatch(operation.m_vTacticalTargetPosition, expectedTacticalPosition))
				return "exact enemy counterattack restore tactical target conflicts with its open duty phase";
		}
		if (manifest.m_sFactionKey != order.m_sFactionKey
			|| manifest.m_sSourceZoneId != order.m_sSourceZoneId
			|| manifest.m_sTargetZoneId != order.m_sTargetZoneId)
			return "exact enemy counterattack restore manifest source, target, or faction conflicts";
		return "";
	}

	protected string ValidateManifest(HST_EnemyOrderState order, HST_ForceManifestState manifest)
	{
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		HST_StrategicMovementService movement = new HST_StrategicMovementService();
		if (!manifest.m_bFrozen || manifest.m_sManifestHash.IsEmpty()
			|| integrity.BuildManifestHash(manifest) != manifest.m_sManifestHash
			|| order.m_sManifestHash != manifest.m_sManifestHash)
			return "exact enemy counterattack restore frozen manifest hash conflicts";
		if (manifest.m_sForceKind != HST_OperationService.EXACT_ENEMY_COUNTERATTACK_FORCE_KIND
			|| manifest.m_sPolicyId != HST_OperationService.EXACT_ENEMY_COUNTERATTACK_POLICY_ID
			|| manifest.m_sIntentId != HST_OperationService.EXACT_ENEMY_COUNTERATTACK_MANIFEST_INTENT
			|| manifest.m_sManifestId != "manifest_" + order.m_sOperationId
			|| manifest.m_sFactionRole != "enemy" || manifest.m_sCatalogVersion.IsEmpty())
			return "exact enemy counterattack restore manifest policy conflicts";
		if (manifest.m_iRequestedMemberCount <= 0
			|| manifest.m_iRequestedMemberCount != manifest.m_iAcceptedMemberCount
			|| manifest.m_iAcceptedMemberCount != manifest.m_aMembers.Count()
			|| manifest.m_iRequestedVehicleCount != 0 || manifest.m_iAcceptedVehicleCount != 0
			|| manifest.m_aVehicles.Count() != 0 || manifest.m_aAssets.Count() != 0
			|| manifest.m_iMoneyCost != 0 || manifest.m_iHRCost != 0
			|| manifest.m_iEquipmentCost != 0 || manifest.m_iAttackResourceCost < 0
			|| manifest.m_iSupportResourceCost < 0)
			return "exact enemy counterattack restore manifest cost or count shape conflicts";
		if (!movement.IsSupportedExactInfantryManifest(manifest)
			|| manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0]
			|| manifest.m_aGroups[0].m_iExpectedMemberCount != manifest.m_iAcceptedMemberCount)
			return "exact enemy counterattack restore manifest is not one complete infantry root";
		if (manifest.m_iAcceptedMemberCount != order.m_iCompositionManpower
			|| manifest.m_iAttackResourceCost != order.m_iAttackCost
			|| manifest.m_iSupportResourceCost != order.m_iSupportCost)
			return "exact enemy counterattack restore manifest or prepaid ledger conflicts";
		bool attackFunded = order.m_iAttackCost > 0 && order.m_iSupportCost == 0;
		bool supportFunded = order.m_iSupportCost > 0 && order.m_iAttackCost == 0;
		if (!attackFunded && !supportFunded)
			return "exact enemy counterattack restore must retain exactly one prepaid resource pool";
		HST_ForceManifestGroupState root = manifest.m_aGroups[0];
		if (root.m_sElementId != manifest.m_sManifestId + "_group_1"
			|| root.m_sPrefab.IsEmpty() || root.m_sPrefab != manifest.m_sGroupPrefab
			|| root.m_iOrdinal != 0 || !root.m_bRequired)
			return "exact enemy counterattack restore group root is invalid";
		for (int memberIndex = 0; memberIndex < manifest.m_aMembers.Count(); memberIndex++)
		{
			HST_ForceManifestMemberState member = manifest.m_aMembers[memberIndex];
			if (!member || member.m_sSlotId.IsEmpty() || member.m_sPrefab.IsEmpty()
				|| member.m_sGroupElementId != root.m_sElementId || !member.m_bRequired
				|| member.m_sSlotId != string.Format("%1_member_%2", manifest.m_sManifestId, memberIndex + 1)
				|| member.m_iOrdinal != memberIndex || member.m_sCatalogSlotId.IsEmpty()
				|| member.m_iMoneyCost != 0 || member.m_iHRCost != 0 || member.m_iEquipmentCost != 0
				|| CountManifestMembers(manifest, member.m_sSlotId) != 1)
				return "exact enemy counterattack restore member roster conflicts";
		}
		if (order.m_iCompositionVehicleCount != 0 || order.m_iCompositionArmedVehicleCount != 0
			|| order.m_sCompositionIntentId != HST_OperationService.EXACT_ENEMY_COUNTERATTACK_MANIFEST_INTENT
			|| order.m_sCompositionTier != "exact")
			return "exact enemy counterattack restore order composition conflicts";
		return "";
	}

	protected string ValidateRouteAndLifecycle(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		if (operation.m_iProjectionContractVersion
				!= HST_StrategicMovementService.EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION
			|| operation.m_iRouteVersion != HST_StrategicMovementService.DIRECT_ROUTE_VERSION
			|| IsZeroVector(operation.m_vRouteStartPosition)
			|| IsZeroVector(operation.m_vRouteEndPosition)
			|| operation.m_fStrategicSpeedMetersPerSecond <= 0)
			return "exact enemy counterattack restore direct-route contract conflicts";
		bool settled = operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		bool openOrPrepared = operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED;
		bool returning = openOrPrepared
			&& operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN;
		if (!settled)
		{
			vector expectedRouteEnd = operation.m_vAssignmentPosition;
			if (returning)
				expectedRouteEnd = operation.m_vOriginPosition;
			if (!PositionsMatch(operation.m_vRouteEndPosition, expectedRouteEnd))
				return "exact enemy counterattack restore route endpoint conflicts with its open duty phase";
		}
		float expectedDistance = Distance2D(operation.m_vRouteStartPosition, operation.m_vRouteEndPosition);
		if (Math.AbsFloat(operation.m_fRouteTotalDistanceMeters - expectedDistance) > 1.0
			|| operation.m_fRouteProgressMeters < -0.01
			|| operation.m_fRouteProgressMeters > operation.m_fRouteTotalDistanceMeters + 0.01)
			return "exact enemy counterattack restore route geometry conflicts";
		if (operation.m_iStrategicLastUpdateSecond < 0
			|| operation.m_iVirtualCombatLastStepSecond < 0
			|| operation.m_iVirtualCombatStepIndex < 0
			|| operation.m_iVirtualCombatFriendlyDamageCarry < 0
			|| operation.m_iVirtualCombatHostileDamageCarry < 0
			|| operation.m_iLastVirtualFriendlyCount < 0
			|| operation.m_iLastVirtualHostileCount < 0
			|| operation.m_iArrivalConfirmationCount < 0
			|| operation.m_iLastArrivalConfirmationSecond < 0)
			return "exact enemy counterattack restore projection clock or combat authority conflicts";

		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			bool activeDuty = operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING
				|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
				|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
				|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN;
			if (!activeDuty || operation.m_eResumeDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_UNKNOWN
				|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
				|| !operation.m_sSettlementId.IsEmpty())
				return "open exact enemy counterattack restore lifecycle conflicts";
			bool strategicPair = operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
				&& (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
					|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING);
			bool livePair = operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
				&& (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
					|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING);
			if (!strategicPair && !livePair)
				return "open exact enemy counterattack restore projection pair conflicts";
			if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				return "open exact enemy counterattack restore order status conflicts";
		}
		return "";
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
			return "exact enemy counterattack restore contains partial runtime identity";
		if (allOperationRuntimeIds != order.m_bStrategicServiceCommitted)
			return "exact enemy counterattack restore service-commit authority conflicts";
		if (!order.m_bStrategicServiceCommitted)
		{
			if (batch || group || !order.m_sSpawnResultId.IsEmpty() || !order.m_sGroupId.IsEmpty())
				return "uncommitted exact enemy counterattack restore contains runtime authority";
			return "";
		}

		if (order.m_sSpawnResultId != operation.m_sSpawnResultId
			|| order.m_sGroupId != operation.m_sGroupId
			|| operation.m_sSpawnResultId != "spawn_" + order.m_sOrderId
			|| operation.m_sForceId != "force_" + order.m_sOperationId
			|| operation.m_sProjectionId != "projection_" + order.m_sOperationId
			|| operation.m_sGroupId != operation.m_sProjectionId)
			return "exact enemy counterattack restore deterministic runtime identity conflicts";

		bool settled = operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		if (settled && !batch && !group)
			return "";
		if (settled && (!batch || !group))
			return "settled exact enemy counterattack restore contains partial runtime residue";
		if (!batch || !group || CountBatchesByAnyIdentity(order, batch) != 1
			|| CountGroupsByAnyIdentity(order, group) != 1)
			return "exact enemy counterattack restore runtime identity is incomplete or ambiguous";
		if (operation.m_sSpawnResultId != batch.m_sResultId
			|| operation.m_sGroupId != group.m_sGroupId
			|| operation.m_sForceId != batch.m_sForceId
			|| operation.m_sProjectionId != batch.m_sProjectionId
			|| operation.m_sForceId != group.m_sForceId
			|| operation.m_sProjectionId != group.m_sProjectionId
			|| group.m_sGroupId != group.m_sProjectionId)
			return "exact enemy counterattack restore projection backlinks conflict";
		if (batch.m_sOperationId != operation.m_sOperationId
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| batch.m_sRequestId != order.m_sOrderId
			|| batch.m_sManifestHash != manifest.m_sManifestHash)
			return "exact enemy counterattack restore batch backlinks conflict";
		if (group.m_sOperationId != operation.m_sOperationId
			|| group.m_sEnemyOrderId != order.m_sOrderId
			|| group.m_sManifestId != manifest.m_sManifestId
			|| group.m_sSpawnResultId != batch.m_sResultId)
			return "exact enemy counterattack restore group backlinks or role conflicts";
		if (group.m_sFactionKey != order.m_sFactionKey
			|| group.m_sZoneId != order.m_sTargetZoneId
			|| group.m_sPrefab != manifest.m_aGroups[0].m_sPrefab)
			return "exact enemy counterattack restore group backlinks or role conflicts";
		string outboundRouteId = order.m_sOperationId + "_outbound";
		string returnRouteId = order.m_sOperationId + "_return";
		if (settled)
		{
			bool recognizedSettledRoute = operation.m_sCurrentRouteId == outboundRouteId
				|| operation.m_sCurrentRouteId == returnRouteId;
			if (!recognizedSettledRoute || group.m_sRouteId != operation.m_sCurrentRouteId)
				return "settled exact enemy counterattack restore route identity conflicts";
		}
		else
		{
			string expectedRouteId = outboundRouteId;
			if (operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
				expectedRouteId = returnRouteId;
			if (operation.m_sCurrentRouteId != expectedRouteId
				|| group.m_sRouteId != expectedRouteId)
				return "open exact enemy counterattack restore route identity conflicts";
		}
		if (group.m_sSpawnFallbackMode != EXACT_GROUP_MODE
			&& !group.m_sSpawnFallbackMode.StartsWith(EXACT_GROUP_MODE + "_"))
			return "exact enemy counterattack restore group backlinks or role conflicts";
		if (!group.m_bQRF || group.m_iVehicleCount != 0
			|| group.m_iOriginalVehicleCount != 0
			|| group.m_iCompositionVehicleCount != 0)
			return "exact enemy counterattack restore group backlinks or role conflicts";
		if (group.m_iCompositionArmedVehicleCount != 0
			|| group.m_iOriginalInfantryCount != manifest.m_iAcceptedMemberCount)
			return "exact enemy counterattack restore group backlinks or role conflicts";
		if (operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& IsZeroVector(group.m_vPosition))
			return "exact enemy counterattack restore live position authority has no group position";
		if (operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& !PositionsMatch(group.m_vPosition, operation.m_vStrategicPosition, 2.0))
			return "exact enemy counterattack restore strategic group position conflicts";
		return ValidateBatchSlotBijection(operation, manifest, batch);
	}

	protected string ValidateBatchSlotBijection(
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		int expectedSlots = manifest.m_aMembers.Count() + 1;
		if (batch.m_iExpectedSlotCount != expectedSlots || batch.m_aSlotResults.Count() != expectedSlots)
			return "exact enemy counterattack restore batch slot count conflicts";
		string rootSlotId = manifest.m_aGroups[0].m_sElementId;
		if (CountBatchSlots(batch, rootSlotId, HST_ForceSpawnQueueService.SLOT_KIND_GROUP) != 1)
			return "exact enemy counterattack restore group-root slot conflicts";
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member || CountBatchSlots(batch, member.m_sSlotId, HST_ForceSpawnQueueService.SLOT_KIND_MEMBER) != 1)
				return "exact enemy counterattack restore member-slot bijection conflicts";
		}
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_sSlotId.IsEmpty() || slot.m_sProjectionId != batch.m_sProjectionId)
				return "exact enemy counterattack restore batch slot identity conflicts";
			bool rootSlot = slot.m_sSlotId == rootSlotId
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP;
			bool memberSlot = manifest.FindMemberSlot(slot.m_sSlotId) != null
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER;
			if (!rootSlot && !memberSlot)
				return "exact enemy counterattack restore contains a foreign batch slot";
			if (slot.m_bCasualtyConfirmed)
			{
				if (!memberSlot || slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
					|| !slot.m_bEverAlive || slot.m_iCasualtyAtSecond < batch.m_iCreatedAtSecond)
					return "exact enemy counterattack restore casualty tombstone conflicts";
			}
			else if (memberSlot && slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				return "open exact enemy counterattack restore contains an unproven retired member";
		}
		return "";
	}

	protected string ValidateSettlement(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (order.m_bResourceRefundApplied)
			return "exact enemy counterattack restore uses the legacy refund flag";
		if (operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			HST_EOperationTerminalResult expectedTerminal;
			bool fullRefund;
			bool requiresRecapture;
			if (!ResolveSettlementPolicy(
				order.m_sResourceSettlementKind,
				expectedTerminal,
				fullRefund,
				requiresRecapture)
				|| operation.m_eTerminalResult != expectedTerminal
				|| operation.m_sSettlementId != order.m_sResourceSettlementId
				|| operation.m_sTerminalReason.IsEmpty()
				|| operation.m_iSettledAtSecond <= 0
				|| operation.m_iSettledAtSecond > m_SaveData.m_iElapsedSeconds
				|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
				|| operation.m_eMaterializationState
					== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
				return "prepared exact enemy counterattack terminal intent conflicts";
			return ValidatePendingResourceRefundAggregateAuthority(
				m_SaveData.m_aEnemyStrategicMutations,
				order,
				operation,
				manifest,
				batch,
				group);
		}
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			if (order.m_bResourceSettlementApplied || !order.m_sResourceSettlementId.IsEmpty()
				|| !order.m_sResourceSettlementKind.IsEmpty()
				|| !order.m_sResourceRefundMutationId.IsEmpty()
				|| order.m_iSettlementAcceptedMemberCount != 0
				|| order.m_iSettlementSurvivorMemberCount != 0
				|| order.m_iRefundedAttackResources != 0
				|| order.m_iRefundedSupportResources != 0)
				return "open exact enemy counterattack restore contains settlement authority";
			int refundClaimants = CountResourceMutationClaimants(
				m_SaveData.m_aEnemyStrategicMutations,
				order,
				false);
			if (refundClaimants != 0)
				return "open exact enemy counterattack restore contains unlinked refund mutation authority";
			return "";
		}
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			|| !order.m_bResourceSettlementApplied || operation.m_sSettlementId.IsEmpty()
			|| operation.m_sSettlementId != order.m_sResourceSettlementId
			|| order.m_sResourceSettlementKind.IsEmpty()
			|| order.m_sResourceSettlementId != HST_OperationService.BuildSettlementId(
				order.m_sOperationId,
				order.m_sResourceSettlementKind))
			return "settled exact enemy counterattack restore receipt conflicts";
		string settlementPolicyFailure = ValidateSettlementPolicy(order, operation);
		if (!settlementPolicyFailure.IsEmpty())
			return settlementPolicyFailure;
		if (order.m_iSettlementAcceptedMemberCount != manifest.m_iAcceptedMemberCount
			|| order.m_iSettlementAcceptedMemberCount <= 0
			|| order.m_iSettlementSurvivorMemberCount < 0
			|| order.m_iSettlementSurvivorMemberCount > order.m_iSettlementAcceptedMemberCount)
			return "settled exact enemy counterattack restore survivor receipt conflicts";
		int expectedAttackRefund = Math.Max(0, order.m_iAttackCost)
			* order.m_iSettlementSurvivorMemberCount / order.m_iSettlementAcceptedMemberCount;
		int expectedSupportRefund = Math.Max(0, order.m_iSupportCost)
			* order.m_iSettlementSurvivorMemberCount / order.m_iSettlementAcceptedMemberCount;
		if (IsFullRefundSettlementKind(order.m_sResourceSettlementKind))
		{
			expectedAttackRefund = Math.Max(0, order.m_iAttackCost);
			expectedSupportRefund = Math.Max(0, order.m_iSupportCost);
		}
		if (order.m_iRefundedAttackResources != expectedAttackRefund
			|| order.m_iRefundedSupportResources != expectedSupportRefund)
			return "settled exact enemy counterattack restore refund receipt conflicts";
		string refundFailure = ValidateSettledResourceRefundAuthority(
			m_SaveData.m_aEnemyStrategicMutations,
			order);
		if (!refundFailure.IsEmpty())
			return refundFailure;
		if (operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN
			|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return "settled exact enemy counterattack restore terminal lifecycle conflicts";
		bool completed = operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
			|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED;
		if ((completed && order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED)
			|| (!completed && order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED))
			return "settled exact enemy counterattack restore order status conflicts";
		return "";
	}

	protected void NormalizeValidAggregate(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		// PREPARED is an irrevocable terminal intent. Runtime restore consumes it
		// before any ordinary projection tick; virtualizing it here would invent a
		// second reason and could conflict with the already-applied refund.
		if (operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
			return;
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			NormalizeSettledRuntime(batch, group);
			return;
		}

		int restoreSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		bool savedLive = operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING);
		if (savedLive && group && !IsZeroVector(group.m_vPosition))
		{
			operation.m_vStrategicPosition = group.m_vPosition;
			HST_StrategicMovementService movement = new HST_StrategicMovementService();
			movement.SyncRouteProgressFromPosition(operation, operation.m_vStrategicPosition);
		}
		else if (group && !IsZeroVector(operation.m_vStrategicPosition))
		{
			group.m_vPosition = operation.m_vStrategicPosition;
		}

		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_iMaterializationStateEnteredAtSecond = restoreSecond;
		operation.m_iStrategicLastUpdateSecond = restoreSecond;
		operation.m_iVirtualCombatLastStepSecond = restoreSecond;
		operation.m_iLastProgressAtSecond = restoreSecond;
		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		operation.m_sLastProjectionReason = "restored exact counterattack as strategic authority";
		operation.m_iRevision++;
		NormalizeBatchForStrategicHold(batch, restoreSecond);
		NormalizeGroupForStrategicHold(operation, batch, group);
		order.m_bPhysicalized = false;
		order.m_bAbstractResolved = false;
		order.m_sRuntimeStatus = "exact_counterattack_restore_virtual";
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
		group.m_sRuntimeStatus = "enemy_counterattack_virtual";
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
		if (!order)
			return;
		if (reason.IsEmpty())
			reason = "schema-69 exact counterattack authority is invalid";
		bool alreadyQuarantined = order.m_iOperationContractVersion == QUARANTINED_CONTRACT_VERSION;
		if (!alreadyQuarantined || order.m_sFailureReason.IsEmpty())
			order.m_sFailureReason = reason;
		string durableReason = order.m_sFailureReason;
		order.m_iOperationContractVersion = QUARANTINED_CONTRACT_VERSION;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		order.m_bPhysicalized = false;
		order.m_sRuntimeStatus = "exact_counterattack_quarantined";
		HoldOperation(operation, durableReason);
		HoldBatch(batch, durableReason);
		HoldGroup(group, durableReason);
		HoldAllCounterattackClaimants(order, operation, durableReason);
	}

	protected void NormalizeQuarantinedOrder(HST_EnemyOrderState order)
	{
		string reason = order.m_sFailureReason;
		if (reason.IsEmpty())
			reason = "schema-69 exact counterattack remained quarantined after restore";
		Quarantine(
			order,
			FindUniqueOperation(order.m_sOperationId),
			FindUniqueBatch(order.m_sSpawnResultId),
			FindUniqueGroup(order.m_sGroupId),
			reason);
	}

	protected void PreserveOrphanClaimants()
	{
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK)
				continue;
			HST_EnemyOrderState order = FindUniqueOrder(operation.m_sEnemyOrderId);
			if (IsReciprocalCounterattackOrder(order, operation))
				continue;
			HoldOperationClaimants(operation, "exact counterattack operation has no unique reciprocal order");
		}

		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (!IsCounterattackManifestClaimant(manifest))
				continue;
			HST_OperationRecordState operation = FindUniqueOperation(manifest.m_sOperationId);
			HST_EnemyOrderState order;
			if (operation)
				order = FindUniqueOrder(operation.m_sEnemyOrderId);
			if (IsReciprocalCounterattackChain(order, operation, manifest))
				continue;
			if (operation)
				HoldOperationClaimants(operation, "exact counterattack manifest has no unique reciprocal chain");
			HoldManifestClaimants(manifest, "exact counterattack manifest has no unique reciprocal chain");
		}

		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!batch)
				continue;
			HST_ForceManifestState manifest = FindUniqueManifest(batch.m_sManifestId);
			if (!IsCounterattackManifestClaimant(manifest))
				continue;
			HST_OperationRecordState operation = FindUniqueOperation(batch.m_sOperationId);
			HST_EnemyOrderState order;
			if (operation)
				order = FindUniqueOrder(operation.m_sEnemyOrderId);
			HST_ActiveGroupState group;
			if (operation)
				group = FindUniqueGroup(operation.m_sGroupId);
			if (IsReciprocalCounterattackRuntime(order, operation, manifest, batch, group))
				continue;
			HoldBatch(batch, "exact counterattack batch has no unique reciprocal runtime chain");
			if (group)
				HoldGroup(group, "exact counterattack batch has no unique reciprocal runtime chain");
		}

		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!IsCounterattackGroupClaimant(group))
				continue;
			HST_OperationRecordState operation = FindUniqueOperation(group.m_sOperationId);
			HST_ForceManifestState manifest = FindUniqueManifest(group.m_sManifestId);
			HST_EnemyOrderState order;
			if (operation)
				order = FindUniqueOrder(operation.m_sEnemyOrderId);
			HST_ForceSpawnResultState batch = FindUniqueBatch(group.m_sSpawnResultId);
			if (IsReciprocalCounterattackRuntime(order, operation, manifest, batch, group))
				continue;
			HoldGroup(group, "exact counterattack group has no unique reciprocal runtime chain");
			if (batch)
				HoldBatch(batch, "exact counterattack group has no unique reciprocal runtime chain");
		}
	}

	protected bool IsCounterattackManifestClaimant(HST_ForceManifestState manifest)
	{
		return manifest && (manifest.m_sForceKind == HST_OperationService.EXACT_ENEMY_COUNTERATTACK_FORCE_KIND
			|| manifest.m_sPolicyId == HST_OperationService.EXACT_ENEMY_COUNTERATTACK_POLICY_ID
			|| manifest.m_sIntentId == HST_OperationService.EXACT_ENEMY_COUNTERATTACK_MANIFEST_INTENT);
	}

	protected bool IsCounterattackGroupClaimant(HST_ActiveGroupState group)
	{
		if (!group)
			return false;
		if (group.m_sSpawnFallbackMode == EXACT_GROUP_MODE
			|| group.m_sSpawnFallbackMode.StartsWith(EXACT_GROUP_MODE + "_")
			|| group.m_sRuntimeStatus.StartsWith("enemy_counterattack_")
			|| group.m_sRuntimeStatus.StartsWith("exact_counterattack_"))
			return true;
		HST_OperationRecordState operation = FindUniqueOperation(group.m_sOperationId);
		return operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK;
	}

	protected bool IsReciprocalCounterattackOrder(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		if (!order || !operation || order.m_eType != HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
			return false;
		bool supportedContract = order.m_iOperationContractVersion
			== HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION
			|| order.m_iOperationContractVersion == QUARANTINED_CONTRACT_VERSION;
		return supportedContract
			&& order.m_sOperationId == operation.m_sOperationId
			&& order.m_sOrderId == operation.m_sEnemyOrderId
			&& order.m_sManifestId == operation.m_sManifestId;
	}

	protected bool IsReciprocalCounterattackChain(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		return IsReciprocalCounterattackOrder(order, operation)
			&& IsCounterattackManifestClaimant(manifest)
			&& operation.m_sManifestId == manifest.m_sManifestId
			&& manifest.m_sOperationId == operation.m_sOperationId;
	}

	protected bool IsReciprocalCounterattackRuntime(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!IsReciprocalCounterattackChain(order, operation, manifest)
			|| !batch || !group || !order.m_bStrategicServiceCommitted)
			return false;
		if (order.m_sSpawnResultId != batch.m_sResultId
			|| order.m_sGroupId != group.m_sGroupId
			|| operation.m_sSpawnResultId != batch.m_sResultId
			|| operation.m_sGroupId != group.m_sGroupId)
			return false;
		if (batch.m_sOperationId != operation.m_sOperationId
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| group.m_sOperationId != operation.m_sOperationId)
			return false;
		return group.m_sManifestId == manifest.m_sManifestId
			&& group.m_sSpawnResultId == batch.m_sResultId;
	}

	protected void HoldAllCounterattackClaimants(
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
		if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK)
			return;
		HoldOperation(operation, reason);
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

	protected void HoldOperation(HST_OperationRecordState operation, string reason)
	{
		if (!operation || operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
			return;
		bool changed = operation.m_eMaterializationState
				!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| operation.m_ePositionAuthority
				!= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			|| operation.m_sLastProjectionReason != reason;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_sLastProjectionReason = reason;
		if (changed)
			operation.m_iRevision++;
	}

	protected void HoldBatch(HST_ForceSpawnResultState batch, string reason)
	{
		if (!batch)
			return;
		batch.m_bStrategicProjectionHeld = true;
		// Quarantine is a durable evidence hold, not a cancellation request.
		// The queue checks cancellation before strategic hold, so leaving this
		// true would clean and eventually compact the only surviving roster.
		batch.m_bCancelRequested = false;
		if (batch.m_sLastFailureReason.IsEmpty())
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
		group.m_sRuntimeStatus = "exact_counterattack_quarantined";
		if (group.m_sSpawnFailureReason.IsEmpty())
			group.m_sSpawnFailureReason = reason;
	}

	protected bool OperationClaimsOrder(
		HST_OperationRecordState operation,
		HST_EnemyOrderState order,
		HST_OperationRecordState knownOperation)
	{
		if (!operation || !order || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK)
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
		if ((!order.m_sOrderId.IsEmpty() && batch.m_sRequestId == order.m_sOrderId)
			|| (!order.m_sOperationId.IsEmpty() && batch.m_sOperationId == order.m_sOperationId)
			|| (!order.m_sManifestId.IsEmpty() && batch.m_sManifestId == order.m_sManifestId)
			|| (!order.m_sSpawnResultId.IsEmpty() && batch.m_sResultId == order.m_sSpawnResultId))
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
		if ((!order.m_sOrderId.IsEmpty() && group.m_sEnemyOrderId == order.m_sOrderId)
			|| (!order.m_sOperationId.IsEmpty() && group.m_sOperationId == order.m_sOperationId)
			|| (!order.m_sManifestId.IsEmpty() && group.m_sManifestId == order.m_sManifestId)
			|| (!order.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == order.m_sSpawnResultId)
			|| (!order.m_sGroupId.IsEmpty() && group.m_sGroupId == order.m_sGroupId))
			return true;
		if (!operation)
			return false;
		return (!operation.m_sForceId.IsEmpty() && group.m_sForceId == operation.m_sForceId)
			|| (!operation.m_sProjectionId.IsEmpty() && group.m_sProjectionId == operation.m_sProjectionId);
	}

	protected HST_EnemyOrderState FindUniqueOrder(string orderId)
	{
		HST_EnemyOrderState match;
		if (orderId.IsEmpty())
			return null;
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

	protected int CountOrdersByAnyIdentity(HST_EnemyOrderState expected)
	{
		int count;
		if (!expected)
			return count;
		string deterministicResultId;
		if (!expected.m_sOrderId.IsEmpty())
			deterministicResultId = "spawn_" + expected.m_sOrderId;
		string deterministicProjectionId;
		if (!expected.m_sOperationId.IsEmpty())
			deterministicProjectionId = "projection_" + expected.m_sOperationId;
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
			if (!matches && !deterministicResultId.IsEmpty())
				matches = order.m_sSpawnResultId == deterministicResultId;
			if (!matches && !expected.m_sGroupId.IsEmpty())
				matches = order.m_sGroupId == expected.m_sGroupId;
			if (!matches && !deterministicProjectionId.IsEmpty())
				matches = order.m_sGroupId == deterministicProjectionId;
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
		if (!order)
			return count;
		string deterministicResultId;
		if (!order.m_sOrderId.IsEmpty())
			deterministicResultId = "spawn_" + order.m_sOrderId;
		string deterministicProjectionId;
		string deterministicForceId;
		if (!order.m_sOperationId.IsEmpty())
		{
			deterministicProjectionId = "projection_" + order.m_sOperationId;
			deterministicForceId = "force_" + order.m_sOperationId;
		}
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!operation)
				continue;
			bool matches = !order.m_sOperationId.IsEmpty()
				&& operation.m_sOperationId == order.m_sOperationId;
			if (!matches && expected && !expected.m_sOperationId.IsEmpty())
				matches = operation.m_sOperationId == expected.m_sOperationId;
			if (!matches && !order.m_sOrderId.IsEmpty())
				matches = operation.m_sEnemyOrderId == order.m_sOrderId;
			if (!matches && !order.m_sManifestId.IsEmpty())
				matches = operation.m_sManifestId == order.m_sManifestId;
			if (!matches && !order.m_sSpawnResultId.IsEmpty())
				matches = operation.m_sSpawnResultId == order.m_sSpawnResultId;
			if (!matches && !deterministicResultId.IsEmpty())
				matches = operation.m_sSpawnResultId == deterministicResultId;
			if (!matches && !order.m_sGroupId.IsEmpty())
				matches = operation.m_sGroupId == order.m_sGroupId;
			if (!matches && !deterministicProjectionId.IsEmpty())
				matches = operation.m_sGroupId == deterministicProjectionId
					|| operation.m_sProjectionId == deterministicProjectionId;
			if (!matches && !deterministicForceId.IsEmpty())
				matches = operation.m_sForceId == deterministicForceId;
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
		if (!order)
			return count;
		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (!manifest)
				continue;
			bool matches = !order.m_sManifestId.IsEmpty()
				&& manifest.m_sManifestId == order.m_sManifestId;
			if (!matches && expected && !expected.m_sManifestId.IsEmpty())
				matches = manifest.m_sManifestId == expected.m_sManifestId;
			if (!matches && !order.m_sOperationId.IsEmpty())
				matches = manifest.m_sOperationId == order.m_sOperationId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountBatchesByAnyIdentity(
		HST_EnemyOrderState order,
		HST_ForceSpawnResultState expected)
	{
		int count;
		if (!order)
			return count;
		string deterministicResultId;
		if (!order.m_sOrderId.IsEmpty())
			deterministicResultId = "spawn_" + order.m_sOrderId;
		string deterministicProjectionId;
		string deterministicForceId;
		if (!order.m_sOperationId.IsEmpty())
		{
			deterministicProjectionId = "projection_" + order.m_sOperationId;
			deterministicForceId = "force_" + order.m_sOperationId;
		}
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!batch)
				continue;
			bool matches = !order.m_sSpawnResultId.IsEmpty()
				&& batch.m_sResultId == order.m_sSpawnResultId;
			if (!matches && expected && !expected.m_sResultId.IsEmpty())
				matches = batch.m_sResultId == expected.m_sResultId;
			if (!matches && !deterministicResultId.IsEmpty())
				matches = batch.m_sResultId == deterministicResultId;
			if (!matches && !order.m_sOperationId.IsEmpty())
				matches = batch.m_sOperationId == order.m_sOperationId;
			if (!matches && !order.m_sManifestId.IsEmpty())
				matches = batch.m_sManifestId == order.m_sManifestId;
			if (!matches && !order.m_sOrderId.IsEmpty())
				matches = batch.m_sRequestId == order.m_sOrderId;
			if (!matches && expected && !expected.m_sProjectionId.IsEmpty())
				matches = batch.m_sProjectionId == expected.m_sProjectionId;
			if (!matches && !deterministicProjectionId.IsEmpty())
				matches = batch.m_sProjectionId == deterministicProjectionId;
			if (!matches && expected && !expected.m_sForceId.IsEmpty())
				matches = batch.m_sForceId == expected.m_sForceId;
			if (!matches && !deterministicForceId.IsEmpty())
				matches = batch.m_sForceId == deterministicForceId;
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
		if (!order)
			return count;
		string deterministicResultId;
		if (!order.m_sOrderId.IsEmpty())
			deterministicResultId = "spawn_" + order.m_sOrderId;
		string deterministicProjectionId;
		string deterministicForceId;
		if (!order.m_sOperationId.IsEmpty())
		{
			deterministicProjectionId = "projection_" + order.m_sOperationId;
			deterministicForceId = "force_" + order.m_sOperationId;
		}
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!group)
				continue;
			bool matches = !order.m_sGroupId.IsEmpty()
				&& group.m_sGroupId == order.m_sGroupId;
			if (!matches && expected && !expected.m_sGroupId.IsEmpty())
				matches = group.m_sGroupId == expected.m_sGroupId;
			if (!matches && !deterministicProjectionId.IsEmpty())
				matches = group.m_sGroupId == deterministicProjectionId
					|| group.m_sProjectionId == deterministicProjectionId;
			if (!matches && !order.m_sOperationId.IsEmpty())
				matches = group.m_sOperationId == order.m_sOperationId;
			if (!matches && !order.m_sManifestId.IsEmpty())
				matches = group.m_sManifestId == order.m_sManifestId;
			if (!matches && !order.m_sSpawnResultId.IsEmpty())
				matches = group.m_sSpawnResultId == order.m_sSpawnResultId;
			if (!matches && !deterministicResultId.IsEmpty())
				matches = group.m_sSpawnResultId == deterministicResultId;
			if (!matches && expected && !expected.m_sProjectionId.IsEmpty())
				matches = group.m_sProjectionId == expected.m_sProjectionId;
			if (!matches && expected && !expected.m_sForceId.IsEmpty())
				matches = group.m_sForceId == expected.m_sForceId;
			if (!matches && !deterministicForceId.IsEmpty())
				matches = group.m_sForceId == deterministicForceId;
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

	static string ValidateOriginalResourceDebitAuthority(
		array<ref HST_EnemyStrategicMutationState> mutations,
		HST_EnemyOrderState order)
	{
		if (!mutations || !order)
			return "exact enemy counterattack original resource debit context is missing";
		bool attackFunded = order.m_iAttackCost > 0 && order.m_iSupportCost == 0;
		bool supportFunded = order.m_iSupportCost > 0 && order.m_iAttackCost == 0;
		if (!attackFunded && !supportFunded)
			return "exact enemy counterattack original resource debit does not name exactly one charged pool";

		string expectedMutationId = "enemy_resource_debit_" + order.m_sOrderId;
		if (order.m_sOrderId.IsEmpty() || order.m_sResourceDebitMutationId != expectedMutationId)
			return "exact enemy counterattack original resource debit identity conflicts";
		HST_EnemyStrategicMutationState debit;
		int identityCount;
		foreach (HST_EnemyStrategicMutationState mutation : mutations)
		{
			if (!mutation || mutation.m_sMutationId != expectedMutationId)
				continue;
			debit = mutation;
			identityCount++;
		}
		if (identityCount != 1 || !debit
			|| CountResourceMutationClaimants(mutations, order, true) != 1)
			return "exact enemy counterattack original resource debit is missing or ambiguous";

		string expectedKind = "proactive_attack_debit";
		if (supportFunded)
			expectedKind = "defense_support_debit";
		bool debitContractExact = debit.m_iContractVersion
			== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& debit.m_bApplied && debit.m_sKind == expectedKind;
		bool debitSourceExact = debit.m_sFactionKey == order.m_sFactionKey
			&& debit.m_sSourceId == order.m_sOrderId
			&& debit.m_sOrderId == order.m_sOrderId;
		bool debitBacklinksExact = debit.m_sOperationId == order.m_sOperationId
			&& debit.m_sManifestId == order.m_sManifestId
			&& debit.m_sZoneId == order.m_sTargetZoneId;
		bool debitAmountExact = debit.m_iAttackDelta == -order.m_iAttackCost
			&& debit.m_iSupportDelta == -order.m_iSupportCost
			&& debit.m_iAggressionDelta == 0;
		// Frozen planning may be consumed after its decision bucket (including
		// after a retry or restart), so the applied debit may be newer than the
		// durable order. The shared strategic validator separately rejects
		// mutations created after the saved campaign clock.
		bool debitEvidenceExact = debit.m_iCreatedAtSecond >= order.m_iCreatedAtSecond
			&& debit.m_sContributionHash.IsEmpty();
		if (!debitContractExact || !debitSourceExact || !debitBacklinksExact
			|| !debitAmountExact || !debitEvidenceExact)
			return "exact enemy counterattack original resource debit backlink, pool, or cost conflicts";
		string shapeFailure;
		if (!HST_EnemyStrategicResourceSaveValidationService.ValidateMutationShape(
			debit,
			shapeFailure))
			return "exact enemy counterattack original resource debit shape conflicts: " + shapeFailure;
		return "";
	}

	static string ValidateCompletedOwnershipAuthority(
		array<ref HST_ZoneState> zones,
		array<ref HST_OwnershipTransitionState> transitions,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		if (!order || !operation)
			return "exact enemy counterattack ownership context is missing";

		bool returning = operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		returning = returning && operation.m_eDutyState
			== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN;
		bool settledCompleted = operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		settledCompleted = settledCompleted && operation.m_eTerminalResult
			== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED;
		bool recaptureResolution = order.m_sResolutionKind
			== "exact_counterattack_recaptured";
		bool requiresOwnership = returning || settledCompleted
			|| order.m_bOutcomeApplied || recaptureResolution;
		if (!requiresOwnership)
			return "";

		bool reciprocalOrder = operation.m_sOperationId == order.m_sOperationId
			&& operation.m_sEnemyOrderId == order.m_sOrderId;
		bool reciprocalAssignment = operation.m_sAssignmentZoneId == order.m_sTargetZoneId
			&& operation.m_sOwnerFactionKey == order.m_sFactionKey;
		if (!reciprocalOrder || !reciprocalAssignment)
			return "exact enemy counterattack completed ownership context conflicts";
		if (!order.m_bOutcomeApplied || !recaptureResolution)
			return "exact enemy counterattack completed ownership outcome receipt conflicts";
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& !returning)
			return "open exact enemy counterattack completed ownership is not returning";
		if (!zones || !transitions)
			return "exact enemy counterattack completed ownership authority is missing";

		string requestId = "ownership_counterattack_" + order.m_sOperationId;
		HST_ZoneState targetZone;
		int targetCount;
		foreach (HST_ZoneState zone : zones)
		{
			if (!zone)
				continue;
			if (zone.m_sZoneId == order.m_sTargetZoneId)
			{
				targetZone = zone;
				targetCount++;
			}
		}
		if (targetCount != 1 || !targetZone)
			return "exact enemy counterattack completed ownership target is missing or ambiguous";

		HST_OwnershipTransitionState receipt;
		int receiptClaimants;
		foreach (HST_OwnershipTransitionState transition : transitions)
		{
			if (!transition)
				continue;
			bool claimsReceipt = transition.m_sRequestId == requestId;
			if (!claimsReceipt)
				claimsReceipt = transition.m_sSourceId == order.m_sOperationId;
			if (!claimsReceipt)
				continue;
			receipt = transition;
			receiptClaimants++;
		}
		if (receiptClaimants != 1 || !receipt)
			return "exact enemy counterattack completed ownership receipt is missing or ambiguous";

		bool receiptLifecycleExact = receipt.m_iContractVersion
			== HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			&& receipt.m_sStatus == "completed";
		receiptLifecycleExact = receiptLifecycleExact && receipt.m_bValidated
			&& receipt.m_bOwnerApplied;
		receiptLifecycleExact = receiptLifecycleExact && receipt.m_bCompleted
			&& !receipt.m_bQuarantined;
		bool receiptIdentityExact = receipt.m_sRequestId == requestId
			&& receipt.m_sZoneId == order.m_sTargetZoneId;
		receiptIdentityExact = receiptIdentityExact && receipt.m_sCause == "military_capture"
			&& receipt.m_sSourceType == "enemy_counterattack";
		bool receiptOwnerExact = receipt.m_sSourceId == order.m_sOperationId
			&& receipt.m_sNewOwnerFactionKey == order.m_sFactionKey;
		if (!receiptLifecycleExact || !receiptIdentityExact || !receiptOwnerExact)
			return "exact enemy counterattack completed ownership receipt conflicts";
		// The completed receipt is immutable historical authority. The zone may be
		// captured again while this force returns or after the operation settles,
		// so current owner and last/active backlinks must not rewrite that history.
		return "";
	}

	static bool IsFullRefundSettlementKind(string settlementKind)
	{
		HST_EOperationTerminalResult terminalResult;
		bool fullRefund;
		bool requiresRecapture;
		return ResolveSettlementPolicy(
			settlementKind,
			terminalResult,
			fullRefund,
			requiresRecapture) && fullRefund;
	}

	static bool ResolveSettlementPolicy(
		string settlementKind,
		out HST_EOperationTerminalResult terminalResult,
		out bool fullRefund,
		out bool requiresRecapture)
	{
		terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN;
		fullRefund = false;
		requiresRecapture = false;
		if (settlementKind == "returned_survivors")
		{
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED;
			requiresRecapture = true;
		}
		else if (settlementKind == "destroyed")
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED;
		else if (settlementKind == "route_failed_survivors")
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED;
		else if (settlementKind == "virtual_combat_projection_failed_survivors"
			|| settlementKind == "virtual_projection_failed_survivors"
			|| settlementKind == "materialization_failed_survivors")
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED;
		else if (settlementKind == "target_invalidated_survivors"
			|| settlementKind == "ownership_failed_survivors"
			|| settlementKind == "invalidated_survivors"
			|| settlementKind == "restore_invalidated_survivors")
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED;
		else if (settlementKind == "return_transition_failed_survivors")
		{
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED;
			requiresRecapture = true;
		}
		else if (settlementKind == "invalidated_full"
			|| settlementKind == "restore_invalidated_full")
		{
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED;
			fullRefund = true;
		}
		else if (settlementKind == "admission_failed_full")
		{
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED;
			fullRefund = true;
		}
		else
			return false;
		return true;
	}

	static string ValidateSettlementPolicy(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation)
	{
		if (!order || !operation)
			return "settled exact enemy counterattack policy context is missing";

		string kind = order.m_sResourceSettlementKind;
		HST_EOperationTerminalResult expectedTerminal;
		bool fullRefund;
		bool requiresRecapture;
		if (!ResolveSettlementPolicy(
			kind,
			expectedTerminal,
			fullRefund,
			requiresRecapture))
			return "settled exact enemy counterattack settlement kind is unsupported";

		if (operation.m_eTerminalResult != expectedTerminal
			|| order.m_bStrategicServiceCommitted == fullRefund)
			return "settled exact enemy counterattack settlement policy conflicts with terminal authority";
		if (fullRefund && (order.m_iSettlementAcceptedMemberCount <= 0
			|| order.m_iSettlementSurvivorMemberCount
				!= order.m_iSettlementAcceptedMemberCount
			|| order.m_bOutcomeApplied
			|| order.m_sResolutionKind == "exact_counterattack_recaptured"))
			return "full exact enemy counterattack settlement retained contradictory survivor or outcome authority";
		if (requiresRecapture
			&& (!order.m_bOutcomeApplied
				|| order.m_sResolutionKind != "exact_counterattack_recaptured"))
			return "counterattack return settlement lacks the completed ownership outcome";
		if (expectedTerminal == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED
			&& (order.m_iSettlementSurvivorMemberCount != 0
			|| order.m_iRefundedAttackResources != 0
			|| order.m_iRefundedSupportResources != 0))
			return "destroyed exact enemy counterattack settlement retained survivors or refund";
		return "";
	}

	static string ValidateSettledResourceRefundAuthority(
		array<ref HST_EnemyStrategicMutationState> mutations,
		HST_EnemyOrderState order)
	{
		if (!mutations || !order || !order.m_bResourceSettlementApplied
			|| order.m_sResourceSettlementId.IsEmpty()
			|| order.m_sResourceSettlementKind.IsEmpty())
			return "settled exact enemy counterattack resource refund context is incomplete";
		string debitFailure = ValidateOriginalResourceDebitAuthority(mutations, order);
		if (!debitFailure.IsEmpty())
			return debitFailure;

		bool attackFunded = order.m_iAttackCost > 0 && order.m_iSupportCost == 0;
		string expectedMutationId = "enemy_resource_refund_" + order.m_sResourceSettlementId;
		if (order.m_sResourceRefundMutationId != expectedMutationId)
			return "settled exact enemy counterattack refund mutation identity conflicts";
		HST_EnemyStrategicMutationState refund;
		int identityCount;
		foreach (HST_EnemyStrategicMutationState mutation : mutations)
		{
			if (!mutation || mutation.m_sMutationId != expectedMutationId)
				continue;
			refund = mutation;
			identityCount++;
		}
		if (identityCount != 1 || !refund
			|| CountResourceMutationClaimants(mutations, order, false) != 1)
			return "settled exact enemy counterattack refund mutation is missing or ambiguous";
		HST_EnemyStrategicMutationState debit;
		foreach (HST_EnemyStrategicMutationState mutation : mutations)
		{
			if (mutation && mutation.m_sMutationId == order.m_sResourceDebitMutationId)
			{
				debit = mutation;
				break;
			}
		}
		if (!debit)
			return "settled exact enemy counterattack original debit chronology is unavailable";

		string expectedKind = "defense_support_refund";
		if (attackFunded)
			expectedKind = "proactive_attack_refund";
		bool refundContractExact = refund.m_iContractVersion
			== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& refund.m_bApplied && refund.m_sKind == expectedKind;
		bool refundSourceExact = refund.m_sFactionKey == order.m_sFactionKey
			&& refund.m_sSourceId == order.m_sResourceSettlementId
			&& refund.m_sOrderId == order.m_sOrderId;
		bool refundBacklinksExact = refund.m_sOperationId == order.m_sOperationId
			&& refund.m_sManifestId == order.m_sManifestId
			&& refund.m_sZoneId == order.m_sTargetZoneId;
		bool refundAmountExact = refund.m_iAttackDelta == order.m_iRefundedAttackResources
			&& refund.m_iSupportDelta == order.m_iRefundedSupportResources
			&& refund.m_iAggressionDelta == 0;
		bool refundEvidenceExact = refund.m_iCreatedAtSecond >= order.m_iCreatedAtSecond
			&& refund.m_iCreatedAtSecond >= debit.m_iCreatedAtSecond
			&& refund.m_sContributionHash.IsEmpty();
		if (!refundContractExact || !refundSourceExact || !refundBacklinksExact
			|| !refundAmountExact || !refundEvidenceExact)
			return "settled exact enemy counterattack refund mutation backlink, pool, or amount conflicts";
		string shapeFailure;
		if (!HST_EnemyStrategicResourceSaveValidationService.ValidateMutationShape(
			refund,
			shapeFailure))
			return "settled exact enemy counterattack refund mutation shape conflicts: " + shapeFailure;
		return "";
	}

	static string ValidatePreparedResourceSettlementTuple(HST_EnemyOrderState order)
	{
		if (!order || order.m_sOperationId.IsEmpty()
			|| order.m_sResourceSettlementKind.IsEmpty()
			|| order.m_sResourceSettlementId != HST_OperationService.BuildSettlementId(
				order.m_sOperationId,
				order.m_sResourceSettlementKind)
			|| order.m_iSettlementAcceptedMemberCount <= 0
			|| order.m_iSettlementSurvivorMemberCount < 0
			|| order.m_iSettlementSurvivorMemberCount
				> order.m_iSettlementAcceptedMemberCount)
			return "prepared exact enemy counterattack resource settlement tuple is incomplete";

		HST_EOperationTerminalResult expectedTerminal;
		bool fullRefund;
		bool requiresRecapture;
		if (!ResolveSettlementPolicy(
			order.m_sResourceSettlementKind,
			expectedTerminal,
			fullRefund,
			requiresRecapture))
			return "prepared exact enemy counterattack settlement kind is unsupported";
		if (order.m_bStrategicServiceCommitted == fullRefund)
			return "prepared exact enemy counterattack settlement commitment conflicts";
		if (fullRefund && (order.m_iSettlementSurvivorMemberCount
			!= order.m_iSettlementAcceptedMemberCount
			|| order.m_bOutcomeApplied
			|| order.m_sResolutionKind == "exact_counterattack_recaptured"))
			return "prepared full exact enemy counterattack refund conflicts with committed outcome authority";
		if (requiresRecapture && (!order.m_bOutcomeApplied
			|| order.m_sResolutionKind != "exact_counterattack_recaptured"))
			return "prepared exact enemy counterattack return refund lacks completed ownership authority";
		if (expectedTerminal == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED
			&& order.m_iSettlementSurvivorMemberCount != 0)
			return "prepared destroyed exact enemy counterattack retains survivors";

		int expectedAttackRefund = Math.Max(0, order.m_iAttackCost)
			* order.m_iSettlementSurvivorMemberCount
			/ order.m_iSettlementAcceptedMemberCount;
		int expectedSupportRefund = Math.Max(0, order.m_iSupportCost)
			* order.m_iSettlementSurvivorMemberCount
			/ order.m_iSettlementAcceptedMemberCount;
		if (fullRefund)
		{
			expectedAttackRefund = Math.Max(0, order.m_iAttackCost);
			expectedSupportRefund = Math.Max(0, order.m_iSupportCost);
		}
		if (order.m_iRefundedAttackResources != expectedAttackRefund
			|| order.m_iRefundedSupportResources != expectedSupportRefund)
			return "prepared exact enemy counterattack refund amount conflicts with staged survivor authority";
		string expectedRefundMutationId
			= "enemy_resource_refund_" + order.m_sResourceSettlementId;
		if (order.m_sResourceRefundMutationId != expectedRefundMutationId)
			return "prepared exact enemy counterattack refund identity conflicts";
		return "";
	}

	static string ValidatePendingResourceRefundAuthority(
		array<ref HST_EnemyStrategicMutationState> mutations,
		HST_EnemyOrderState order)
	{
		if (!mutations || !order || order.m_bResourceSettlementApplied)
			return "prepared exact enemy counterattack pending refund context is incomplete";
		string tupleFailure = ValidatePreparedResourceSettlementTuple(order);
		if (!tupleFailure.IsEmpty())
			return tupleFailure;
		string debitFailure = ValidateOriginalResourceDebitAuthority(mutations, order);
		if (!debitFailure.IsEmpty())
			return debitFailure;

		HST_EnemyStrategicMutationState refund;
		HST_EnemyStrategicMutationState debit;
		int identityCount;
		foreach (HST_EnemyStrategicMutationState mutation : mutations)
		{
			if (!mutation)
				continue;
			if (mutation.m_sMutationId == order.m_sResourceRefundMutationId)
			{
				refund = mutation;
				identityCount++;
			}
			if (mutation.m_sMutationId == order.m_sResourceDebitMutationId)
				debit = mutation;
		}
		if (identityCount != 1 || !refund
			|| CountResourceMutationClaimants(mutations, order, false) != 1)
			return "prepared exact enemy counterattack pending refund mutation is missing or ambiguous";
		if (!debit)
			return "prepared exact enemy counterattack original debit chronology is unavailable";

		bool attackFunded = order.m_iAttackCost > 0 && order.m_iSupportCost == 0;
		string expectedKind = "defense_support_refund";
		if (attackFunded)
			expectedKind = "proactive_attack_refund";
		bool refundExact = refund.m_iContractVersion
			== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& refund.m_bApplied && refund.m_sKind == expectedKind
			&& refund.m_sSourceId == order.m_sResourceSettlementId
			&& refund.m_sMutationId == order.m_sResourceRefundMutationId
			&& refund.m_sFactionKey == order.m_sFactionKey
			&& refund.m_sOrderId == order.m_sOrderId
			&& refund.m_sOperationId == order.m_sOperationId
			&& refund.m_sManifestId == order.m_sManifestId
			&& refund.m_sZoneId == order.m_sTargetZoneId
			&& refund.m_iAttackDelta == order.m_iRefundedAttackResources
			&& refund.m_iSupportDelta == order.m_iRefundedSupportResources
			&& refund.m_iAggressionDelta == 0
			&& refund.m_iCreatedAtSecond >= order.m_iCreatedAtSecond
			&& refund.m_iCreatedAtSecond >= debit.m_iCreatedAtSecond
			&& refund.m_sContributionHash.IsEmpty();
		if (!refundExact)
			return "prepared exact enemy counterattack pending refund backlink, pool, amount, or chronology conflicts";
		string shapeFailure;
		if (!HST_EnemyStrategicResourceSaveValidationService.ValidateMutationShape(
			refund,
			shapeFailure))
			return "prepared exact enemy counterattack pending refund mutation shape conflicts: " + shapeFailure;
		return "";
	}

	static string ValidatePendingResourceRefundAggregateAuthority(
		array<ref HST_EnemyStrategicMutationState> mutations,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		string tupleFailure = ValidatePreparedResourceSettlementTuple(order);
		if (!tupleFailure.IsEmpty())
			return tupleFailure;
		int refundClaimants = CountResourceMutationClaimants(mutations, order, false);
		if (refundClaimants == 0)
		{
			if (order.m_bResourceSettlementApplied)
				return "prepared exact enemy counterattack marks an unapplied refund as complete";
		}
		else if (refundClaimants == 1)
		{
			string refundFailure;
			if (order.m_bResourceSettlementApplied)
				refundFailure = ValidateSettledResourceRefundAuthority(mutations, order);
			else
				refundFailure = ValidatePendingResourceRefundAuthority(mutations, order);
			if (!refundFailure.IsEmpty())
				return refundFailure;
		}
		else
			return "prepared exact enemy counterattack refund authority is ambiguous";
		HST_EOperationTerminalResult expectedTerminal;
		bool fullRefund;
		bool requiresRecapture;
		if (!ResolveSettlementPolicy(
			order.m_sResourceSettlementKind,
			expectedTerminal,
			fullRefund,
			requiresRecapture))
			return "prepared exact enemy counterattack pending settlement policy is unsupported";

		int accepted = order.m_iSettlementAcceptedMemberCount;
		int stagedSurvivors = order.m_iSettlementSurvivorMemberCount;
		if (fullRefund)
		{
			if (operation && !manifest)
				return "prepared full exact enemy counterattack operation lacks its frozen manifest";
			if (operation && (operation.m_eSettlementState
				!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
				|| operation.m_sOperationId != order.m_sOperationId
				|| operation.m_sEnemyOrderId != order.m_sOrderId
				|| operation.m_sManifestId != order.m_sManifestId
				|| operation.m_eTerminalResult != expectedTerminal
				|| operation.m_sSettlementId != order.m_sResourceSettlementId
				|| !operation.m_sSpawnResultId.IsEmpty()
				|| !operation.m_sGroupId.IsEmpty()))
				return "prepared full exact enemy counterattack operation authority conflicts";
			if (manifest && (manifest.m_sManifestId != order.m_sManifestId
				|| manifest.m_sOperationId != order.m_sOperationId
				|| manifest.m_iAcceptedMemberCount != accepted))
				return "prepared full exact enemy counterattack manifest authority conflicts";
			if (batch)
			{
				bool reciprocalBatch = batch.m_sResultId == "spawn_" + order.m_sOrderId
					&& batch.m_sRequestId == order.m_sOrderId
					&& batch.m_sOperationId == order.m_sOperationId
					&& batch.m_sManifestId == order.m_sManifestId
					&& batch.m_sForceId == "force_" + order.m_sOperationId
					&& batch.m_sProjectionId == "projection_" + order.m_sOperationId;
				if (!reciprocalBatch || batch.m_iSuccessfulHandoffCount != 0
					|| batch.m_eStatus
						== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
					return "prepared full exact enemy counterattack retained foreign or committed runtime authority";
			}
			if (group)
			{
				bool reciprocalGroup = group.m_sGroupId == "projection_" + order.m_sOperationId
					&& group.m_sProjectionId == group.m_sGroupId
					&& group.m_sForceId == "force_" + order.m_sOperationId
					&& group.m_sEnemyOrderId == order.m_sOrderId
					&& group.m_sOperationId == order.m_sOperationId
					&& group.m_sManifestId == order.m_sManifestId
					&& group.m_sSpawnResultId == "spawn_" + order.m_sOrderId;
				if (!reciprocalGroup || group.m_bSpawnedEntity
					|| group.m_iSpawnedAgentCount > 0)
					return "prepared full exact enemy counterattack retained a foreign or live group";
			}
			return "";
		}

		if (!operation || !manifest || !batch || !group)
			return "prepared exact enemy counterattack pending refund lacks reciprocal terminal or runtime authority";
		bool terminalAuthorityExact = operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
			&& operation.m_sOperationId == order.m_sOperationId
			&& operation.m_sEnemyOrderId == order.m_sOrderId
			&& operation.m_sManifestId == order.m_sManifestId
			&& operation.m_eTerminalResult == expectedTerminal
			&& operation.m_sSettlementId == order.m_sResourceSettlementId
			&& operation.m_iSettledAtSecond > 0;
		bool manifestAuthorityExact = manifest.m_sManifestId == order.m_sManifestId
			&& manifest.m_sOperationId == order.m_sOperationId
			&& manifest.m_iAcceptedMemberCount == accepted
			&& manifest.m_sManifestHash == order.m_sManifestHash;
		bool executionAuthorityExact = order.m_sSpawnResultId == operation.m_sSpawnResultId
			&& order.m_sGroupId == operation.m_sGroupId
			&& operation.m_sSpawnResultId == "spawn_" + order.m_sOrderId
			&& operation.m_sForceId == "force_" + order.m_sOperationId
			&& operation.m_sProjectionId == "projection_" + order.m_sOperationId
			&& operation.m_sGroupId == operation.m_sProjectionId;
		bool batchAuthorityExact = batch.m_sResultId == operation.m_sSpawnResultId
			&& batch.m_sRequestId == order.m_sOrderId
			&& batch.m_sOperationId == order.m_sOperationId
			&& batch.m_sManifestId == manifest.m_sManifestId
			&& batch.m_sManifestHash == manifest.m_sManifestHash
			&& batch.m_sForceId == operation.m_sForceId
			&& batch.m_sProjectionId == operation.m_sProjectionId;
		bool groupAuthorityExact = group.m_sGroupId == operation.m_sGroupId
			&& group.m_sGroupId == group.m_sProjectionId
			&& group.m_sForceId == operation.m_sForceId
			&& group.m_sProjectionId == operation.m_sProjectionId
			&& group.m_sEnemyOrderId == order.m_sOrderId
			&& group.m_sOperationId == order.m_sOperationId
			&& group.m_sManifestId == manifest.m_sManifestId
			&& group.m_sSpawnResultId == batch.m_sResultId;
		if (!terminalAuthorityExact || !manifestAuthorityExact
			|| !executionAuthorityExact || !batchAuthorityExact
			|| !groupAuthorityExact)
			return "prepared exact enemy counterattack pending refund lacks reciprocal terminal or runtime authority";

		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		int durableSurvivors;
		bool exactRoster;
		bool upperBoundRoster;
		if (batch.m_bStrategicProjectionHeld)
		{
			durableSurvivors = queue.CountStrategicLivingMemberSlots(batch);
			exactRoster = true;
		}
		else if (batch.m_iSuccessfulHandoffCount > 0)
		{
			durableSurvivors = queue.CountDurableLivingMemberSlots(batch);
			if (operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState
					== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
				upperBoundRoster = true;
			else
				exactRoster = true;
		}
		else
		{
			durableSurvivors = Math.Max(
				group.m_iInfantryCount,
				Math.Max(
					group.m_iDurableLivingInfantryCount,
					group.m_iSurvivorInfantryCount));
			exactRoster = operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				|| operation.m_eMaterializationState
					== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		}
		durableSurvivors = Math.Max(0, Math.Min(accepted, durableSurvivors));
		if (expectedTerminal == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED)
		{
			if (durableSurvivors != 0)
				return "prepared destroyed exact enemy counterattack retained durable survivors";
			return "";
		}
		if ((exactRoster && stagedSurvivors != durableSurvivors)
			|| (upperBoundRoster && stagedSurvivors > durableSurvivors)
			|| (!exactRoster && !upperBoundRoster))
			return "prepared exact enemy counterattack survivor receipt diverges from durable runtime authority";
		return "";
	}

	protected static string ResolveSettlementKindFromId(
		HST_EnemyOrderState order,
		string settlementId)
	{
		if (!order || settlementId.IsEmpty())
			return "";
		if (settlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, "returned_survivors"))
			return "returned_survivors";
		if (settlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, "destroyed"))
			return "destroyed";
		if (settlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, "route_failed_survivors"))
			return "route_failed_survivors";
		if (settlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, "virtual_combat_projection_failed_survivors"))
			return "virtual_combat_projection_failed_survivors";
		if (settlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, "virtual_projection_failed_survivors"))
			return "virtual_projection_failed_survivors";
		if (settlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, "materialization_failed_survivors"))
			return "materialization_failed_survivors";
		if (settlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, "target_invalidated_survivors"))
			return "target_invalidated_survivors";
		if (settlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, "ownership_failed_survivors"))
			return "ownership_failed_survivors";
		if (settlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, "return_transition_failed_survivors"))
			return "return_transition_failed_survivors";
		if (settlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, "invalidated_survivors"))
			return "invalidated_survivors";
		if (settlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, "restore_invalidated_survivors"))
			return "restore_invalidated_survivors";
		if (settlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, "invalidated_full"))
			return "invalidated_full";
		if (settlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, "restore_invalidated_full"))
			return "restore_invalidated_full";
		if (settlementId == HST_OperationService.BuildSettlementId(order.m_sOperationId, "admission_failed_full"))
			return "admission_failed_full";
		return "";
	}

	static int CountResourceMutationClaimants(
		array<ref HST_EnemyStrategicMutationState> mutations,
		HST_EnemyOrderState order,
		bool debit)
	{
		int count;
		if (!mutations || !order || order.m_sOrderId.IsEmpty())
			return count;
		foreach (HST_EnemyStrategicMutationState mutation : mutations)
		{
			if (!mutation)
				continue;
			bool claimsOrder = mutation.m_sOrderId == order.m_sOrderId;
			if (!claimsOrder && !order.m_sOperationId.IsEmpty())
				claimsOrder = mutation.m_sOperationId == order.m_sOperationId;
			if (!claimsOrder && !order.m_sManifestId.IsEmpty())
				claimsOrder = mutation.m_sManifestId == order.m_sManifestId;
			if (!claimsOrder && debit && !order.m_sResourceDebitMutationId.IsEmpty())
				claimsOrder = mutation.m_sMutationId == order.m_sResourceDebitMutationId;
			if (!claimsOrder && !debit && !order.m_sResourceRefundMutationId.IsEmpty())
				claimsOrder = mutation.m_sMutationId == order.m_sResourceRefundMutationId;
			if (!claimsOrder)
				continue;
			bool kindMatches;
			if (debit)
				kindMatches = mutation.m_sKind == "proactive_attack_debit"
					|| mutation.m_sKind == "defense_support_debit";
			else
				kindMatches = mutation.m_sKind == "proactive_attack_refund"
					|| mutation.m_sKind == "defense_support_refund";
			if (kindMatches)
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
