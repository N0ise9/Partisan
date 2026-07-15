// Schema-neutral persistence authority for exact defensive-QRF settlement.
// The appended PREPARED operation state and the existing order/mutation fields
// are sufficient to resume a deterministic refund without inventing authority.
class HST_EnemyQRFSaveValidationService
{
	static bool IsExactEnemyDefensiveQRF(HST_EnemyOrderState order)
	{
		return order
			&& order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF
			&& order.m_iOperationContractVersion
				== HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION;
	}

	static bool HasPreparedSettlementIntent(
		HST_CampaignSaveData saveData,
		HST_EnemyOrderState order)
	{
		if (!saveData || !IsExactEnemyDefensiveQRF(order))
			return false;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!operation)
				continue;
			bool claimant = operation.m_sOperationId == order.m_sOperationId
				|| operation.m_sEnemyOrderId == order.m_sOrderId
				|| operation.m_sManifestId == order.m_sManifestId;
			if (claimant && operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
				return true;
		}
		return false;
	}

	// Container schema alone cannot distinguish a historical exact-QRF row that
	// predates strategic mutation receipts from a row produced by the current
	// resource contract. Use only durable row-level evidence for that decision.
	static bool HasStrategicReceiptProvenance(
		HST_CampaignSaveData saveData,
		HST_EnemyOrderState order)
	{
		if (!saveData || !IsExactEnemyDefensiveQRF(order))
			return false;
		string expectedDebitId = "enemy_resource_debit_" + order.m_sOrderId;
		string expectedRefundId
			= "enemy_resource_refund_" + order.m_sResourceSettlementId;
		// These fields did not exist on the historical mutationless boundary.
		// Any value, including a malformed one, therefore claims current receipt
		// provenance and must pass the strict validator rather than fall back.
		if (!order.m_sResourceDebitMutationId.IsEmpty()
			|| !order.m_sResourceRefundMutationId.IsEmpty())
			return true;
		if (!saveData.m_aEnemyStrategicMutations)
			return false;
		foreach (HST_EnemyStrategicMutationState mutation : saveData.m_aEnemyStrategicMutations)
		{
			if (!mutation)
				continue;
			bool identityClaim = mutation.m_sMutationId == expectedDebitId
				|| mutation.m_sMutationId == expectedRefundId;
			bool aggregateClaim = mutation.m_sOrderId == order.m_sOrderId
				|| mutation.m_sOperationId == order.m_sOperationId
				|| mutation.m_sManifestId == order.m_sManifestId;
			if (identityClaim || aggregateClaim)
				return true;
		}
		return false;
	}

	static string ValidatePreparedSaveAuthority(
		HST_CampaignSaveData saveData,
		HST_EnemyOrderState order)
	{
		if (!saveData || !IsExactEnemyDefensiveQRF(order))
			return "prepared exact enemy defensive QRF save authority is incomplete";
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string aggregateFailure = ResolveUniqueAggregate(
			saveData,
			order,
			operation,
			manifest);
		if (!aggregateFailure.IsEmpty())
			return aggregateFailure;

		HST_EOperationTerminalResult expectedTerminal;
		bool fullRefund;
		string tupleFailure = ValidateSettlementTuple(
			order,
			manifest,
			expectedTerminal,
			fullRefund);
		if (!tupleFailure.IsEmpty())
			return tupleFailure;
		if (operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
			|| operation.m_eTerminalResult != expectedTerminal
			|| operation.m_sSettlementId != order.m_sResourceSettlementId
			|| operation.m_sTerminalReason.IsEmpty()
			|| operation.m_iSettledAtSecond <= 0
			|| operation.m_iSettledAtSecond > saveData.m_iElapsedSeconds
			|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			|| operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
			return "prepared exact enemy defensive QRF terminal intent conflicts";
		if ((order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
				&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED)
			|| order.m_iResolvedAtSecond > 0)
			return "prepared exact enemy defensive QRF order status is not a reachable predecessor";

		string runtimeFailure = ValidatePreparedRuntimeClaimants(
			saveData,
			order,
			operation,
			manifest,
			fullRefund);
		if (!runtimeFailure.IsEmpty())
			return runtimeFailure;
		string debitFailure = ValidateOriginalResourceDebitAuthority(
			saveData.m_aEnemyStrategicMutations,
			order);
		if (!debitFailure.IsEmpty())
			return debitFailure;
		string refundFailure = ValidatePreparedRefundAuthority(
			saveData.m_aEnemyStrategicMutations,
			order);
		if (!refundFailure.IsEmpty())
			return refundFailure;
		return ValidateSettlementMutationChronology(
			saveData,
			order,
			operation,
			false);
	}

	static string ValidateSettledSaveAuthority(
		HST_CampaignSaveData saveData,
		HST_EnemyOrderState order)
	{
		if (!saveData || !IsExactEnemyDefensiveQRF(order))
			return "settled exact enemy defensive QRF save authority is incomplete";
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string aggregateFailure = ResolveUniqueAggregate(
			saveData,
			order,
			operation,
			manifest);
		if (!aggregateFailure.IsEmpty())
			return aggregateFailure;

		HST_EOperationTerminalResult expectedTerminal;
		bool fullRefund;
		string tupleFailure = ValidateSettlementTuple(
			order,
			manifest,
			expectedTerminal,
			fullRefund);
		if (!tupleFailure.IsEmpty())
			return tupleFailure;
		if (!order.m_bResourceSettlementApplied
			|| operation.m_eSettlementState
				!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			|| operation.m_eTerminalResult != expectedTerminal
			|| operation.m_sSettlementId != order.m_sResourceSettlementId
			|| operation.m_sTerminalReason.IsEmpty()
			// A synchronous admission failure can settle at campaign second zero.
			// SETTLED is the authority marker; PREPARED remains strictly positive.
			|| operation.m_iSettledAtSecond < 0
			|| operation.m_iSettledAtSecond > saveData.m_iElapsedSeconds
			|| operation.m_eDutyState
				!= HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			|| operation.m_eEngagementMode
				!= HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
			|| operation.m_eMaterializationState
				!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
			|| operation.m_ePositionAuthority
				!= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return "settled exact enemy defensive QRF terminal receipt conflicts";

		string debitFailure = ValidateOriginalResourceDebitAuthority(
			saveData.m_aEnemyStrategicMutations,
			order);
		if (!debitFailure.IsEmpty())
			return debitFailure;
		string refundFailure = ValidateSettledResourceRefundAuthority(
			saveData.m_aEnemyStrategicMutations,
			order);
		if (!refundFailure.IsEmpty())
			return refundFailure;
		return ValidateSettlementMutationChronology(
			saveData,
			order,
			operation,
			true);
	}

	static string ValidateOriginalResourceDebitAuthority(
		array<ref HST_EnemyStrategicMutationState> mutations,
		HST_EnemyOrderState order)
	{
		if (!mutations || !IsExactEnemyDefensiveQRF(order)
			|| order.m_sOrderId.IsEmpty() || order.m_sOperationId.IsEmpty()
			|| order.m_sManifestId.IsEmpty() || order.m_sFactionKey.IsEmpty()
			|| order.m_sTargetZoneId.IsEmpty() || order.m_iAttackCost < 0
			|| order.m_iSupportCost <= 0)
			return "exact enemy defensive QRF original debit authority is incomplete";
		string expectedMutationId = "enemy_resource_debit_" + order.m_sOrderId;
		if (order.m_sResourceDebitMutationId != expectedMutationId)
			return "exact enemy defensive QRF original debit identity conflicts";

		HST_EnemyStrategicMutationState debit;
		int identityCount;
		foreach (HST_EnemyStrategicMutationState mutation : mutations)
		{
			if (!mutation)
				continue;
			if (mutation.m_sMutationId == expectedMutationId)
			{
				debit = mutation;
				identityCount++;
			}
		}
		if (identityCount != 1 || !debit
			|| CountResourceMutationClaimants(mutations, order, true) != 1)
			return "exact enemy defensive QRF original debit is missing or ambiguous";

		bool exact = debit.m_iContractVersion
			== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& debit.m_bApplied
			&& debit.m_sKind == "defense_support_debit"
			&& debit.m_sMutationId == expectedMutationId
			&& debit.m_sSourceId == order.m_sOrderId
			&& debit.m_sFactionKey == order.m_sFactionKey
			&& debit.m_sOrderId == order.m_sOrderId
			&& debit.m_sOperationId == order.m_sOperationId
			&& debit.m_sManifestId == order.m_sManifestId
			&& debit.m_sZoneId == order.m_sTargetZoneId
			&& debit.m_iAttackDelta == -order.m_iAttackCost
			&& debit.m_iSupportDelta == -order.m_iSupportCost
			&& debit.m_iAggressionDelta == 0
			&& debit.m_iCreatedAtSecond >= order.m_iCreatedAtSecond
			&& debit.m_sContributionHash.IsEmpty();
		if (!exact)
			return "exact enemy defensive QRF original debit backlink, amount, or chronology conflicts";
		string shapeFailure;
		if (!HST_EnemyStrategicResourceSaveValidationService.ValidateMutationShape(
			debit,
			shapeFailure))
			return "exact enemy defensive QRF original debit shape conflicts: " + shapeFailure;
		return "";
	}

	static string ValidatePreparedRefundAuthority(
		array<ref HST_EnemyStrategicMutationState> mutations,
		HST_EnemyOrderState order)
	{
		if (!mutations || !order)
			return "prepared exact enemy defensive QRF refund context is incomplete";
		int refundClaimants = CountResourceMutationClaimants(mutations, order, false);
		int refundIdentityCount;
		foreach (HST_EnemyStrategicMutationState mutation : mutations)
		{
			if (mutation && mutation.m_sMutationId == order.m_sResourceRefundMutationId)
				refundIdentityCount++;
		}
		if (refundClaimants == 0)
		{
			if (refundIdentityCount != 0)
				return "prepared exact enemy defensive QRF refund identity has no valid refund role";
			if (order.m_bResourceSettlementApplied)
				return "prepared exact enemy defensive QRF marks a missing refund as applied";
			return "";
		}
		if (refundClaimants != 1)
			return "prepared exact enemy defensive QRF refund authority is ambiguous";
		return ValidateRefundMutationAuthority(mutations, order);
	}

	static string ValidateSettledResourceRefundAuthority(
		array<ref HST_EnemyStrategicMutationState> mutations,
		HST_EnemyOrderState order)
	{
		if (!mutations || !order || !order.m_bResourceSettlementApplied)
			return "settled exact enemy defensive QRF refund context is incomplete";
		if (CountResourceMutationClaimants(mutations, order, false) != 1)
			return "settled exact enemy defensive QRF refund authority is missing or ambiguous";
		return ValidateRefundMutationAuthority(mutations, order);
	}

	protected static string ValidateRefundMutationAuthority(
		array<ref HST_EnemyStrategicMutationState> mutations,
		HST_EnemyOrderState order)
	{
		string expectedMutationId
			= "enemy_resource_refund_" + order.m_sResourceSettlementId;
		if (order.m_sResourceRefundMutationId != expectedMutationId)
			return "exact enemy defensive QRF refund identity conflicts";
		HST_EnemyStrategicMutationState refund;
		HST_EnemyStrategicMutationState debit;
		int identityCount;
		foreach (HST_EnemyStrategicMutationState mutation : mutations)
		{
			if (!mutation)
				continue;
			if (mutation.m_sMutationId == expectedMutationId)
			{
				refund = mutation;
				identityCount++;
			}
			if (mutation.m_sMutationId == order.m_sResourceDebitMutationId)
				debit = mutation;
		}
		if (identityCount != 1 || !refund || !debit)
			return "exact enemy defensive QRF refund mutation is missing or ambiguous";

		bool exact = refund.m_iContractVersion
			== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& refund.m_bApplied
			&& refund.m_sKind == "defense_support_refund"
			&& refund.m_sMutationId == expectedMutationId
			&& refund.m_sSourceId == order.m_sResourceSettlementId
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
		if (!exact)
			return "exact enemy defensive QRF refund backlink, amount, or chronology conflicts";
		string shapeFailure;
		if (!HST_EnemyStrategicResourceSaveValidationService.ValidateMutationShape(
			refund,
			shapeFailure))
			return "exact enemy defensive QRF refund shape conflicts: " + shapeFailure;
		return "";
	}

	protected static string ValidateSettlementMutationChronology(
		HST_CampaignSaveData saveData,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		bool requireRefund)
	{
		if (!saveData || !order || !operation
			|| (operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
				&& operation.m_iSettledAtSecond <= 0)
			|| (operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
				&& operation.m_iSettledAtSecond < 0)
			|| operation.m_iSettledAtSecond > saveData.m_iElapsedSeconds
			|| order.m_iCreatedAtSecond < 0
			|| order.m_iCreatedAtSecond > saveData.m_iElapsedSeconds)
			return "exact enemy defensive QRF settlement chronology is incomplete";

		HST_EnemyStrategicMutationState debit;
		HST_EnemyStrategicMutationState refund;
		int debitCount;
		int refundCount;
		foreach (HST_EnemyStrategicMutationState mutation : saveData.m_aEnemyStrategicMutations)
		{
			if (!mutation)
				continue;
			if (mutation.m_sMutationId == order.m_sResourceDebitMutationId)
			{
				debit = mutation;
				debitCount++;
			}
			if (mutation.m_sMutationId == order.m_sResourceRefundMutationId)
			{
				refund = mutation;
				refundCount++;
			}
		}
		if (debitCount != 1 || !debit
			|| debit.m_iCreatedAtSecond > operation.m_iSettledAtSecond
			|| debit.m_iCreatedAtSecond > saveData.m_iElapsedSeconds)
			return "exact enemy defensive QRF debit chronology conflicts with prepared terminal intent";
		if (refundCount == 0)
		{
			if (requireRefund)
				return "settled exact enemy defensive QRF refund chronology is missing";
			return "";
		}
		if (refundCount != 1 || !refund
			|| refund.m_iCreatedAtSecond < operation.m_iSettledAtSecond
			|| refund.m_iCreatedAtSecond > saveData.m_iElapsedSeconds)
			return "exact enemy defensive QRF refund chronology conflicts with prepared terminal intent";
		return "";
	}

	protected static string ResolveUniqueAggregate(
		HST_CampaignSaveData saveData,
		HST_EnemyOrderState order,
		out HST_OperationRecordState operation,
		out HST_ForceManifestState manifest)
	{
		operation = null;
		manifest = null;
		int orderCount;
		foreach (HST_EnemyOrderState candidateOrder : saveData.m_aEnemyOrders)
		{
			if (!candidateOrder)
				continue;
			bool claimant = candidateOrder.m_sOrderId == order.m_sOrderId
				|| candidateOrder.m_sOperationId == order.m_sOperationId
				|| candidateOrder.m_sManifestId == order.m_sManifestId;
			if (!claimant && !order.m_sSpawnResultId.IsEmpty())
				claimant = candidateOrder.m_sSpawnResultId == order.m_sSpawnResultId;
			if (!claimant && !order.m_sGroupId.IsEmpty())
				claimant = candidateOrder.m_sGroupId == order.m_sGroupId;
			if (claimant)
				orderCount++;
		}
		if (orderCount != 1)
			return "exact enemy defensive QRF order settlement claimant is ambiguous";

		int operationCount;
		foreach (HST_OperationRecordState candidateOperation : saveData.m_aOperations)
		{
			if (!candidateOperation)
				continue;
			bool claimant = candidateOperation.m_sOperationId == order.m_sOperationId
				|| candidateOperation.m_sEnemyOrderId == order.m_sOrderId
				|| candidateOperation.m_sManifestId == order.m_sManifestId;
			if (!claimant && !order.m_sSpawnResultId.IsEmpty())
				claimant = candidateOperation.m_sSpawnResultId == order.m_sSpawnResultId;
			if (!claimant && !order.m_sGroupId.IsEmpty())
				claimant = candidateOperation.m_sGroupId == order.m_sGroupId;
			if (!claimant)
				continue;
			operation = candidateOperation;
			operationCount++;
		}
		if (operationCount != 1 || !operation)
			return "exact enemy defensive QRF operation settlement claimant is missing or ambiguous";

		int manifestCount;
		foreach (HST_ForceManifestState candidateManifest : saveData.m_aForceManifests)
		{
			if (!candidateManifest || (candidateManifest.m_sManifestId != order.m_sManifestId
				&& candidateManifest.m_sOperationId != order.m_sOperationId))
				continue;
			manifest = candidateManifest;
			manifestCount++;
		}
		if (manifestCount != 1 || !manifest)
			return "exact enemy defensive QRF manifest settlement claimant is missing or ambiguous";
		if (operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF
			|| operation.m_iContractVersion
				!= HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION
			|| operation.m_sOperationId != order.m_sOperationId
			|| operation.m_sEnemyOrderId != order.m_sOrderId
			|| operation.m_sManifestId != order.m_sManifestId
			|| manifest.m_sManifestId != order.m_sManifestId
			|| manifest.m_sOperationId != order.m_sOperationId)
			return "exact enemy defensive QRF settlement aggregate backlinks conflict";
		if (order.m_sOrderId.IsEmpty() || order.m_sOperationId.IsEmpty()
			|| order.m_sManifestId.IsEmpty() || order.m_sFactionKey.IsEmpty()
			|| order.m_sSourceZoneId.IsEmpty() || order.m_sTargetZoneId.IsEmpty()
			|| order.m_sOperationId
				!= HST_StableIdService.BuildOperationId("enemy_order", order.m_sOrderId))
			return "exact enemy defensive QRF stable aggregate identity conflicts";
		if (order.m_bResourceRefundApplied)
			return "exact enemy defensive QRF settlement uses the legacy refund flag";
		if (order.m_sManifestHash.IsEmpty()
			|| order.m_sManifestHash != manifest.m_sManifestHash
			|| operation.m_sOriginZoneId != order.m_sSourceZoneId
			|| operation.m_sAssignmentZoneId != order.m_sTargetZoneId
			|| operation.m_sOwnerFactionKey != order.m_sFactionKey
			|| manifest.m_sSourceZoneId != order.m_sSourceZoneId
			|| manifest.m_sTargetZoneId != order.m_sTargetZoneId
			|| manifest.m_sFactionKey != order.m_sFactionKey)
			return "exact enemy defensive QRF settlement source, target, faction, or hash backlink conflicts";

		HST_StrategicMovementService movement = new HST_StrategicMovementService();
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		if (!manifest.m_bFrozen
			|| manifest.m_sForceKind
				!= HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_FORCE_KIND
			|| manifest.m_sPolicyId
				!= HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_POLICY_ID
			|| manifest.m_sIntentId
				!= HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_MANIFEST_INTENT
			|| !movement.IsSupportedExactInfantryManifest(manifest)
			|| manifest.m_sManifestHash.IsEmpty()
			|| integrity.BuildManifestHash(manifest) != manifest.m_sManifestHash
			|| manifest.m_iRequestedMemberCount != manifest.m_iAcceptedMemberCount
			|| manifest.m_iAcceptedMemberCount != order.m_iCompositionManpower
			|| manifest.m_iAttackResourceCost != order.m_iAttackCost
			|| manifest.m_iSupportResourceCost != order.m_iSupportCost
			|| order.m_iCompositionVehicleCount != 0
			|| order.m_iCompositionArmedVehicleCount != 0)
			return "exact enemy defensive QRF frozen manifest or prepaid roster conflicts";
		if (operation.m_sAssignmentKind
				!= HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_ASSIGNMENT_KIND
			|| operation.m_sRecallPolicyId
				!= HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_RECALL_POLICY
			|| operation.m_sSettlementPolicyId
				!= HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_SETTLEMENT_POLICY)
			return "exact enemy defensive QRF operation policy authority conflicts";
		if (operation.m_iProjectionContractVersion
				!= HST_StrategicMovementService.EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION
			|| operation.m_iRouteVersion != HST_StrategicMovementService.DIRECT_ROUTE_VERSION
			|| operation.m_fStrategicSpeedMetersPerSecond <= 0
			|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_UNKNOWN
			|| operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_UNKNOWN
			|| operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_UNKNOWN
			|| operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_UNKNOWN)
			return "exact enemy defensive QRF strategic projection authority conflicts";

		string expectedResultId = "spawn_" + order.m_sOrderId;
		string expectedProjectionId = "projection_" + order.m_sOperationId;
		string expectedForceId = "force_" + order.m_sOperationId;
		if (order.m_bStrategicServiceCommitted)
		{
			if (order.m_sSpawnResultId != expectedResultId
				|| order.m_sGroupId != expectedProjectionId
				|| operation.m_sSpawnResultId != expectedResultId
				|| operation.m_sForceId != expectedForceId
				|| operation.m_sProjectionId != expectedProjectionId
				|| operation.m_sGroupId != expectedProjectionId)
				return "exact enemy defensive QRF committed execution backlinks conflict";
		}
		else if (!order.m_sSpawnResultId.IsEmpty() || !order.m_sGroupId.IsEmpty()
			|| !operation.m_sSpawnResultId.IsEmpty() || !operation.m_sForceId.IsEmpty()
			|| !operation.m_sProjectionId.IsEmpty() || !operation.m_sGroupId.IsEmpty())
			return "uncommitted exact enemy defensive QRF retained execution authority";
		return "";
	}

	protected static string ValidateSettlementTuple(
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		out HST_EOperationTerminalResult expectedTerminal,
		out bool fullRefund)
	{
		expectedTerminal = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN;
		fullRefund = false;
		if (!order || !manifest || order.m_sOperationId.IsEmpty()
			|| order.m_sResourceSettlementKind.IsEmpty()
			|| order.m_sResourceSettlementId != HST_OperationService.BuildSettlementId(
				order.m_sOperationId,
				order.m_sResourceSettlementKind)
			|| order.m_sResourceRefundMutationId
				!= "enemy_resource_refund_" + order.m_sResourceSettlementId
			|| order.m_iSettlementAcceptedMemberCount <= 0
			|| order.m_iSettlementAcceptedMemberCount != manifest.m_iAcceptedMemberCount
			|| order.m_iSettlementSurvivorMemberCount < 0
			|| order.m_iSettlementSurvivorMemberCount
				> order.m_iSettlementAcceptedMemberCount)
			return "exact enemy defensive QRF staged settlement tuple is incomplete";
		if (!ResolveSettlementPolicy(
			order.m_sResourceSettlementKind,
			expectedTerminal,
			fullRefund))
			return "exact enemy defensive QRF staged settlement policy is unsupported";
		if (order.m_iAttackCost < 0 || order.m_iSupportCost <= 0)
			return "exact enemy defensive QRF staged defense-resource cost is invalid";
		if (order.m_bStrategicServiceCommitted == fullRefund)
			return "exact enemy defensive QRF staged settlement commitment conflicts";
		if (fullRefund && order.m_iSettlementSurvivorMemberCount
			!= order.m_iSettlementAcceptedMemberCount)
			return "exact enemy defensive QRF full refund lacks the complete admitted roster";
		if (expectedTerminal == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED
			&& order.m_iSettlementSurvivorMemberCount != 0)
			return "destroyed exact enemy defensive QRF retains staged survivors";

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
			return "exact enemy defensive QRF staged refund amount conflicts";
		return "";
	}

	protected static string ValidatePreparedRuntimeClaimants(
		HST_CampaignSaveData saveData,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		bool fullRefund)
	{
		string expectedResultId = "spawn_" + order.m_sOrderId;
		string expectedProjectionId = "projection_" + order.m_sOperationId;
		string expectedForceId = "force_" + order.m_sOperationId;
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		int batchCount;
		int groupCount;
		foreach (HST_ForceSpawnResultState candidateBatch : saveData.m_aForceSpawnResults)
		{
			if (!candidateBatch)
				continue;
			bool claimant = candidateBatch.m_sResultId == expectedResultId
				|| candidateBatch.m_sRequestId == order.m_sOrderId
				|| candidateBatch.m_sOperationId == order.m_sOperationId
				|| candidateBatch.m_sManifestId == order.m_sManifestId
				|| candidateBatch.m_sProjectionId == expectedProjectionId
				|| candidateBatch.m_sForceId == expectedForceId;
			if (!claimant)
				continue;
			batch = candidateBatch;
			batchCount++;
		}
		foreach (HST_ActiveGroupState candidateGroup : saveData.m_aActiveGroups)
		{
			if (!candidateGroup)
				continue;
			bool claimant = candidateGroup.m_sGroupId == expectedProjectionId
				|| candidateGroup.m_sProjectionId == expectedProjectionId
				|| candidateGroup.m_sForceId == expectedForceId
				|| candidateGroup.m_sEnemyOrderId == order.m_sOrderId
				|| candidateGroup.m_sOperationId == order.m_sOperationId
				|| candidateGroup.m_sManifestId == order.m_sManifestId
				|| candidateGroup.m_sSpawnResultId == expectedResultId;
			if (!claimant)
				continue;
			group = candidateGroup;
			groupCount++;
		}

		if (batchCount > 1 || groupCount > 1)
			return "prepared exact enemy defensive QRF runtime claimant is ambiguous";
		if (fullRefund)
		{
			if (order.m_bStrategicServiceCommitted
				|| !order.m_sSpawnResultId.IsEmpty() || !order.m_sGroupId.IsEmpty()
				|| !operation.m_sSpawnResultId.IsEmpty() || !operation.m_sForceId.IsEmpty()
				|| !operation.m_sProjectionId.IsEmpty() || !operation.m_sGroupId.IsEmpty())
				return "prepared full exact enemy defensive QRF retained committed runtime authority";
			if (batch)
			{
				bool reciprocalBatch = batch.m_sResultId == expectedResultId
					&& batch.m_sRequestId == order.m_sOrderId
					&& batch.m_sOperationId == order.m_sOperationId
					&& batch.m_sManifestId == manifest.m_sManifestId
					&& batch.m_sManifestHash == manifest.m_sManifestHash
					&& batch.m_sProjectionId == expectedProjectionId
					&& batch.m_sForceId == expectedForceId;
				if (!reciprocalBatch || batch.m_iSuccessfulHandoffCount != 0
					|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
					return "prepared full exact enemy defensive QRF retained foreign or committed batch authority";
			}
			if (group)
			{
				bool reciprocalGroup = group.m_sGroupId == expectedProjectionId
					&& group.m_sProjectionId == expectedProjectionId
					&& group.m_sForceId == expectedForceId
					&& group.m_sEnemyOrderId == order.m_sOrderId
					&& group.m_sOperationId == order.m_sOperationId
					&& group.m_sManifestId == manifest.m_sManifestId
					&& group.m_sSpawnResultId == expectedResultId
					&& group.m_sFactionKey == order.m_sFactionKey;
				if (!reciprocalGroup || group.m_bSpawnedEntity
					|| group.m_iSpawnedAgentCount > 0
					|| !group.m_sRuntimeEntityId.IsEmpty())
					return "prepared full exact enemy defensive QRF retained foreign or live group authority";
			}
			return "";
		}

		if (!order.m_bStrategicServiceCommitted || batchCount != 1 || groupCount != 1
			|| !batch || !group)
			return "prepared exact enemy defensive QRF lacks committed runtime claimants";
		bool executionExact = order.m_sSpawnResultId == expectedResultId
			&& order.m_sGroupId == expectedProjectionId
			&& operation.m_sSpawnResultId == expectedResultId
			&& operation.m_sForceId == expectedForceId
			&& operation.m_sProjectionId == expectedProjectionId
			&& operation.m_sGroupId == expectedProjectionId;
		bool batchExact = batch.m_sResultId == expectedResultId
			&& batch.m_sRequestId == order.m_sOrderId
			&& batch.m_sOperationId == order.m_sOperationId
			&& batch.m_sManifestId == manifest.m_sManifestId
			&& batch.m_sManifestHash == manifest.m_sManifestHash
			&& batch.m_sProjectionId == expectedProjectionId
			&& batch.m_sForceId == expectedForceId;
		bool groupExact = group.m_sGroupId == expectedProjectionId
			&& group.m_sProjectionId == expectedProjectionId
			&& group.m_sForceId == expectedForceId
			&& group.m_sEnemyOrderId == order.m_sOrderId
			&& group.m_sOperationId == order.m_sOperationId
			&& group.m_sManifestId == manifest.m_sManifestId
			&& group.m_sSpawnResultId == expectedResultId
			&& group.m_sFactionKey == order.m_sFactionKey;
		if (!executionExact || !batchExact || !groupExact)
			return "prepared exact enemy defensive QRF runtime backlinks conflict";
		return ValidatePreparedDurableSurvivorAuthority(
			order,
			operation,
			manifest,
			batch,
			group);
	}

	// A PREPARED refund is restart authority, so its survivor count cannot be
	// trusted merely because the arithmetic is internally consistent. Resolve
	// the roster from the reciprocal runtime graph first. Strategic slot state
	// is exact; a live physical roster is a conservative durable upper bound,
	// matching the established exact-counterattack persistence boundary.
	protected static string ValidatePreparedDurableSurvivorAuthority(
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!order || !operation || !manifest || !batch || !group
			|| manifest.m_iAcceptedMemberCount <= 0)
			return "prepared exact enemy defensive QRF durable survivor authority is incomplete";

		int accepted = manifest.m_iAcceptedMemberCount;
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
			if (group.m_sRuntimeStatus == "eliminated")
				durableSurvivors = 0;
			else
				durableSurvivors = Math.Max(
					group.m_iDurableLivingInfantryCount,
					group.m_iSurvivorInfantryCount);
			exactRoster = operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				|| operation.m_eMaterializationState
					== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		}
		durableSurvivors = Math.Max(0, Math.Min(accepted, durableSurvivors));
		int stagedSurvivors = order.m_iSettlementSurvivorMemberCount;
		if (operation.m_eTerminalResult
			== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED
			&& durableSurvivors != 0)
			return "prepared destroyed exact enemy defensive QRF retained durable survivors";
		if ((exactRoster && stagedSurvivors != durableSurvivors)
			|| (upperBoundRoster && stagedSurvivors > durableSurvivors)
			|| (!exactRoster && !upperBoundRoster))
			return "prepared exact enemy defensive QRF survivor receipt diverges from durable runtime authority";

		int expectedAttackRefund = Math.Max(0, order.m_iAttackCost)
			* stagedSurvivors / accepted;
		int expectedSupportRefund = Math.Max(0, order.m_iSupportCost)
			* stagedSurvivors / accepted;
		if (order.m_iRefundedAttackResources != expectedAttackRefund
			|| order.m_iRefundedSupportResources != expectedSupportRefund)
			return "prepared exact enemy defensive QRF refund tuple diverges from durable survivor authority";
		return "";
	}

	protected static int CountResourceMutationClaimants(
		array<ref HST_EnemyStrategicMutationState> mutations,
		HST_EnemyOrderState order,
		bool debit)
	{
		int count;
		if (!mutations || !order)
			return count;
		foreach (HST_EnemyStrategicMutationState mutation : mutations)
		{
			if (!mutation)
				continue;
			bool claimant = mutation.m_sOrderId == order.m_sOrderId
				|| mutation.m_sOperationId == order.m_sOperationId
				|| mutation.m_sManifestId == order.m_sManifestId;
			if (!claimant && debit)
				claimant = mutation.m_sMutationId == order.m_sResourceDebitMutationId;
			if (!claimant && !debit)
				claimant = mutation.m_sMutationId == order.m_sResourceRefundMutationId;
			if (!claimant)
				continue;
			bool role;
			if (debit)
				role = mutation.m_sKind == "defense_support_debit"
					|| mutation.m_sKind == "proactive_attack_debit";
			else
				role = mutation.m_sKind == "defense_support_refund"
					|| mutation.m_sKind == "proactive_attack_refund";
			if (role)
				count++;
		}
		return count;
	}

	protected static bool ResolveSettlementPolicy(
		string settlementKind,
		out HST_EOperationTerminalResult terminalResult,
		out bool fullRefund)
	{
		terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN;
		fullRefund = false;
		if (settlementKind == "returned_survivors")
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED;
		else if (settlementKind == "destroyed")
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED;
		else if (settlementKind == "virtual_projection_failed_survivors"
			|| settlementKind == "materialization_failed_survivors")
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED;
		else if (settlementKind == "route_failed_survivors")
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED;
		else if (settlementKind == "invalidated_survivors"
			|| settlementKind == "restore_invalidated_survivors")
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED;
		else if (settlementKind == "admission_failed_full")
		{
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED;
			fullRefund = true;
		}
		else if (settlementKind == "invalidated_full"
			|| settlementKind == "restore_invalidated_full")
		{
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED;
			fullRefund = true;
		}
		return terminalResult
			!= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN;
	}
}
