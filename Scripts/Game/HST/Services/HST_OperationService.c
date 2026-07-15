class HST_OperationTransitionResult
{
	bool m_bAccepted;
	bool m_bStateChanged;
	bool m_bAlreadyApplied;
	string m_sFailureReason;
	ref HST_OperationRecordState m_Operation;
}

class HST_OperationService
{
	static const int EXACT_PLAYER_QRF_CONTRACT_VERSION = 1;
	static const string EXACT_PLAYER_QRF_ASSIGNMENT_KIND = "support_on_station";
	static const string EXACT_PLAYER_QRF_RECALL_POLICY = "exit_then_refund_living_hr";
	static const string EXACT_PLAYER_QRF_SETTLEMENT_POLICY = "exact_paid_qrf_ledger";
	static const int EXACT_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION = 1;
	static const int QUARANTINED_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION = -60;
	static const string EXACT_PLAYER_SEARCH_DESTROY_ASSIGNMENT_KIND = "search_destroy_on_station";
	static const string EXACT_PLAYER_SEARCH_DESTROY_RECALL_POLICY = "exit_then_refund_living_hr";
	static const string EXACT_PLAYER_SEARCH_DESTROY_SETTLEMENT_POLICY = "exact_paid_search_destroy_ledger";
	static const float EXACT_PLAYER_SUPPORT_ASSIGNMENT_RETURN_RADIUS_METERS = 75.0;
	static const int EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION = 1;
	static const string EXACT_ENEMY_DEFENSIVE_QRF_FORCE_KIND = "enemy_defensive_qrf";
	static const string EXACT_ENEMY_DEFENSIVE_QRF_POLICY_ID = "exact_enemy_defensive_qrf_v1";
	static const string EXACT_ENEMY_DEFENSIVE_QRF_MANIFEST_INTENT = "enemy_defensive_qrf";
	static const string EXACT_ENEMY_DEFENSIVE_QRF_ASSIGNMENT_KIND = "defend_zone";
	static const string EXACT_ENEMY_DEFENSIVE_QRF_RECALL_POLICY = "return_to_origin_then_refund_survivors";
	static const string EXACT_ENEMY_DEFENSIVE_QRF_SETTLEMENT_POLICY = "exact_enemy_defensive_qrf_ledger";
	static const float EXACT_ENEMY_DEFENSIVE_QRF_ARRIVAL_RADIUS_METERS = 35.0;
	static const int EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION = 1;
	static const string EXACT_ENEMY_COUNTERATTACK_FORCE_KIND = "enemy_counterattack";
	static const string EXACT_ENEMY_COUNTERATTACK_POLICY_ID = "exact_enemy_counterattack_v1";
	static const string EXACT_ENEMY_COUNTERATTACK_MANIFEST_INTENT = "enemy_counterattack";
	static const string EXACT_ENEMY_COUNTERATTACK_ASSIGNMENT_KIND = "capture_zone";
	static const string EXACT_ENEMY_COUNTERATTACK_RECALL_POLICY = "return_to_origin_then_refund_survivors";
	static const string EXACT_ENEMY_COUNTERATTACK_SETTLEMENT_POLICY = "exact_enemy_counterattack_ledger";
	static const int EXACT_ENEMY_GARRISON_REBUILD_CONTRACT_VERSION = 1;
	static const int QUARANTINED_ENEMY_GARRISON_REBUILD_CONTRACT_VERSION = -70;
	static const string EXACT_ENEMY_GARRISON_REBUILD_FORCE_KIND = "enemy_garrison_rebuild";
	static const string EXACT_ENEMY_GARRISON_REBUILD_POLICY_ID = "exact_enemy_garrison_rebuild_v1";
	static const string EXACT_ENEMY_GARRISON_REBUILD_MANIFEST_INTENT = "hst_rebuild_garrison";
	static const string EXACT_ENEMY_GARRISON_REBUILD_ASSIGNMENT_KIND = "reinforce_garrison";
	static const string EXACT_ENEMY_GARRISON_REBUILD_RECALL_POLICY = "return_to_origin_then_refund_survivors";
	static const string EXACT_ENEMY_GARRISON_REBUILD_SETTLEMENT_POLICY = "exact_enemy_garrison_rebuild_ledger";
	static const string EXACT_ENEMY_GARRISON_REBUILD_DELIVERY_SETTLEMENT_KIND = "delivered_garrison_transfer";
	static const int EXACT_ENEMY_GARRISON_REBUILD_SUPPORT_COST = 10;

	static bool RequiresOperation(HST_SupportRequestState request)
	{
		// Any nonzero player-support contract is owned by typed operation authority.
		// Negative versions are quarantine sentinels and must never fall through to
		// legacy support timers, composition, folding, or guessed refunds.
		return request && request.m_iOperationContractVersion != 0;
	}

	static bool IsExactPlayerSupportType(HST_ESupportRequestType supportType)
	{
		return supportType == HST_ESupportRequestType.HST_SUPPORT_QRF
			|| supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;
	}

	static bool IsExactPlayerSupportOperationType(HST_EOperationType operationType)
	{
		return operationType == HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_QRF
			|| operationType == HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_SEARCH_DESTROY;
	}

	static int ResolveExactPlayerSupportContractVersion(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return EXACT_PLAYER_QRF_CONTRACT_VERSION;
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return EXACT_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION;
		return 0;
	}

	static int ResolveExactPlayerSupportOperationContractVersion(HST_EOperationType operationType)
	{
		if (operationType == HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_QRF)
			return EXACT_PLAYER_QRF_CONTRACT_VERSION;
		if (operationType == HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_SEARCH_DESTROY)
			return EXACT_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION;
		return 0;
	}

	static HST_EOperationType ResolveExactPlayerSupportOperationType(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_QRF;
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_SEARCH_DESTROY;
		return HST_EOperationType.HST_OPERATION_TYPE_UNKNOWN;
	}

	static string ResolveExactPlayerSupportAssignmentKind(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return EXACT_PLAYER_QRF_ASSIGNMENT_KIND;
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return EXACT_PLAYER_SEARCH_DESTROY_ASSIGNMENT_KIND;
		return "";
	}

	static string ResolveExactPlayerSupportRecallPolicy(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return EXACT_PLAYER_QRF_RECALL_POLICY;
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return EXACT_PLAYER_SEARCH_DESTROY_RECALL_POLICY;
		return "";
	}

	static string ResolveExactPlayerSupportSettlementPolicy(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return EXACT_PLAYER_QRF_SETTLEMENT_POLICY;
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return EXACT_PLAYER_SEARCH_DESTROY_SETTLEMENT_POLICY;
		return "";
	}

	static bool RequiresOperation(HST_EnemyOrderState order)
	{
		// Any nonzero enemy-order contract is owned by a typed operation service.
		// This includes negative quarantine versions, which must never fall back to
		// legacy timers, support physicalization, or guessed refunds.
		return order && order.m_iOperationContractVersion != 0;
	}

	static bool RequiresExactEnemyDefensiveQRF(HST_EnemyOrderState order)
	{
		return order && order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF
			&& order.m_iOperationContractVersion == EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION;
	}

	static bool RequiresExactEnemyCounterattack(HST_EnemyOrderState order)
	{
		return order && order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK
			&& order.m_iOperationContractVersion == EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION;
	}

	static bool RequiresExactEnemyGarrisonRebuild(HST_EnemyOrderState order)
	{
		return order && order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON
			&& order.m_iOperationContractVersion == EXACT_ENEMY_GARRISON_REBUILD_CONTRACT_VERSION;
	}

	static bool RequiresExactEnemyDirectedResponse(HST_EnemyOrderState order)
	{
		return RequiresExactEnemyDefensiveQRF(order)
			|| RequiresExactEnemyCounterattack(order)
			|| RequiresExactEnemyGarrisonRebuild(order);
	}

	static HST_EOperationType ResolveExactEnemyDirectedOperationType(HST_EnemyOrderState order)
	{
		if (RequiresExactEnemyGarrisonRebuild(order))
			return HST_EOperationType.HST_OPERATION_TYPE_ENEMY_GARRISON_REBUILD;
		if (RequiresExactEnemyCounterattack(order))
			return HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK;
		if (RequiresExactEnemyDefensiveQRF(order))
			return HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF;
		return HST_EOperationType.HST_OPERATION_TYPE_UNKNOWN;
	}

	static int ResolveExactEnemyDirectedContractVersion(HST_EnemyOrderState order)
	{
		if (RequiresExactEnemyGarrisonRebuild(order))
			return EXACT_ENEMY_GARRISON_REBUILD_CONTRACT_VERSION;
		if (RequiresExactEnemyCounterattack(order))
			return EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION;
		if (RequiresExactEnemyDefensiveQRF(order))
			return EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION;
		return 0;
	}

	static string ResolveExactEnemyDirectedForceKind(HST_EnemyOrderState order)
	{
		if (RequiresExactEnemyGarrisonRebuild(order))
			return EXACT_ENEMY_GARRISON_REBUILD_FORCE_KIND;
		if (RequiresExactEnemyCounterattack(order))
			return EXACT_ENEMY_COUNTERATTACK_FORCE_KIND;
		if (RequiresExactEnemyDefensiveQRF(order))
			return EXACT_ENEMY_DEFENSIVE_QRF_FORCE_KIND;
		return "";
	}

	static string ResolveExactEnemyDirectedPolicyId(HST_EnemyOrderState order)
	{
		if (RequiresExactEnemyGarrisonRebuild(order))
			return EXACT_ENEMY_GARRISON_REBUILD_POLICY_ID;
		if (RequiresExactEnemyCounterattack(order))
			return EXACT_ENEMY_COUNTERATTACK_POLICY_ID;
		if (RequiresExactEnemyDefensiveQRF(order))
			return EXACT_ENEMY_DEFENSIVE_QRF_POLICY_ID;
		return "";
	}

	static string ResolveExactEnemyDirectedManifestIntent(HST_EnemyOrderState order)
	{
		if (RequiresExactEnemyGarrisonRebuild(order))
			return EXACT_ENEMY_GARRISON_REBUILD_MANIFEST_INTENT;
		if (RequiresExactEnemyCounterattack(order))
			return EXACT_ENEMY_COUNTERATTACK_MANIFEST_INTENT;
		if (RequiresExactEnemyDefensiveQRF(order))
			return EXACT_ENEMY_DEFENSIVE_QRF_MANIFEST_INTENT;
		return "";
	}

	static string ResolveExactEnemyDirectedAssignmentKind(HST_EnemyOrderState order)
	{
		if (RequiresExactEnemyGarrisonRebuild(order))
			return EXACT_ENEMY_GARRISON_REBUILD_ASSIGNMENT_KIND;
		if (RequiresExactEnemyCounterattack(order))
			return EXACT_ENEMY_COUNTERATTACK_ASSIGNMENT_KIND;
		if (RequiresExactEnemyDefensiveQRF(order))
			return EXACT_ENEMY_DEFENSIVE_QRF_ASSIGNMENT_KIND;
		return "";
	}

	static string ResolveExactEnemyDirectedRecallPolicy(HST_EnemyOrderState order)
	{
		if (RequiresExactEnemyGarrisonRebuild(order))
			return EXACT_ENEMY_GARRISON_REBUILD_RECALL_POLICY;
		if (RequiresExactEnemyCounterattack(order))
			return EXACT_ENEMY_COUNTERATTACK_RECALL_POLICY;
		if (RequiresExactEnemyDefensiveQRF(order))
			return EXACT_ENEMY_DEFENSIVE_QRF_RECALL_POLICY;
		return "";
	}

	static string ResolveExactEnemyDirectedSettlementPolicy(HST_EnemyOrderState order)
	{
		if (RequiresExactEnemyGarrisonRebuild(order))
			return EXACT_ENEMY_GARRISON_REBUILD_SETTLEMENT_POLICY;
		if (RequiresExactEnemyCounterattack(order))
			return EXACT_ENEMY_COUNTERATTACK_SETTLEMENT_POLICY;
		if (RequiresExactEnemyDefensiveQRF(order))
			return EXACT_ENEMY_DEFENSIVE_QRF_SETTLEMENT_POLICY;
		return "";
	}

	static bool RequiresExactEnemyPatrol(HST_EnemyOrderState order)
	{
		return order && order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL
			&& order.m_iOperationContractVersion == HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION;
	}

	static string BuildSettlementId(string operationId, string settlementKind)
	{
		if (operationId.IsEmpty() || settlementKind.IsEmpty())
			return "";
		return "settlement_" + operationId + "_" + settlementKind;
	}

	HST_OperationTransitionResult RegisterExactPlayerQRF(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		if (!request || request.m_eType != HST_ESupportRequestType.HST_SUPPORT_QRF)
			return BuildRejected("exact player support registration received another support family");
		return RegisterExactPlayerSupport(state, request, quote, manifest);
	}

	HST_OperationTransitionResult RegisterExactPlayerSearchDestroy(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		if (!request || request.m_eType != HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return BuildRejected("exact player search-and-destroy registration received another support family");
		return RegisterExactPlayerSupport(state, request, quote, manifest);
	}

	HST_OperationTransitionResult RegisterExactPlayerSupport(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		string failure = ValidateRegistrationIdentity(state, request, quote, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);

		HST_OperationRecordState existing = state.FindOperation(request.m_sOperationId);
		if (existing)
		{
			failure = ValidateExactPlayerSupport(state, existing, request, quote, manifest);
			if (!failure.IsEmpty())
				return BuildRejected(failure);
			return BuildAccepted(existing, false, true);
		}

		if (CountOperationsByAnyIdentity(state, request.m_sOperationId, request.m_sRequestId, request.m_sQuoteId, request.m_sManifestId) > 0)
			return BuildRejected("exact player support operation identity is already owned by another record");

		HST_OperationRecordState operation = new HST_OperationRecordState();
		operation.m_sOperationId = request.m_sOperationId;
		operation.m_eType = ResolveExactPlayerSupportOperationType(request.m_eType);
		operation.m_iContractVersion = ResolveExactPlayerSupportContractVersion(request.m_eType);
		operation.m_sOwnerFactionKey = quote.m_sFactionKey;
		operation.m_sActorIdentityId = quote.m_sActorIdentityId;
		operation.m_sIssueRequestId = quote.m_sCommandRequestId;
		operation.m_sConfirmationRequestId = quote.m_sConfirmationRequestId;
		operation.m_sSupportRequestId = request.m_sRequestId;
		operation.m_sQuoteId = quote.m_sQuoteId;
		operation.m_sManifestId = manifest.m_sManifestId;
		operation.m_sOriginZoneId = quote.m_sSourceZoneId;
		operation.m_vOriginPosition = quote.m_vSourcePosition;
		operation.m_sAssignmentKind = ResolveExactPlayerSupportAssignmentKind(request.m_eType);
		operation.m_sAssignmentZoneId = quote.m_sTargetZoneId;
		operation.m_vAssignmentPosition = quote.m_vTargetPosition;
		operation.m_sTacticalTargetZoneId = quote.m_sTargetZoneId;
		operation.m_vTacticalTargetPosition = quote.m_vTargetPosition;
		operation.m_vStrategicPosition = quote.m_vSourcePosition;
		operation.m_iProjectionContractVersion = HST_StrategicMovementService.EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION;
		operation.m_iRouteVersion = HST_StrategicMovementService.DIRECT_ROUTE_VERSION;
		operation.m_vRouteStartPosition = quote.m_vSourcePosition;
		operation.m_vRouteEndPosition = quote.m_vTargetPosition;
		operation.m_fRouteTotalDistanceMeters = Distance2D(quote.m_vSourcePosition, quote.m_vTargetPosition);
		operation.m_fStrategicSpeedMetersPerSecond = HST_StrategicMovementService.EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND;
		operation.m_iStrategicLastUpdateSecond = state.m_iElapsedSeconds;
		operation.m_iVirtualCombatLastStepSecond = state.m_iElapsedSeconds;
		operation.m_iLastVirtualFriendlyCount = manifest.m_iAcceptedMemberCount;
		operation.m_sRecallPolicyId = ResolveExactPlayerSupportRecallPolicy(request.m_eType);
		operation.m_sSettlementPolicyId = ResolveExactPlayerSupportSettlementPolicy(request.m_eType);
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING;
		operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		operation.m_eTerminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE;
		operation.m_iDeterministicSeed = manifest.m_iDeterministicSeed;
		operation.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		operation.m_iDutyStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iEngagementStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		operation.m_iRevision = 1;
		state.m_aOperations.Insert(operation);
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult RegisterExactEnemyDefensiveQRF(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		string failure = ValidateEnemyDefensiveQRFRegistrationIdentity(state, order, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);

		HST_OperationRecordState existing = state.FindOperation(order.m_sOperationId);
		if (existing)
		{
			failure = ValidateExactEnemyDefensiveQRF(state, existing, order, manifest);
			if (!failure.IsEmpty())
				return BuildRejected(failure);
			return BuildAccepted(existing, false, true);
		}
		if (CountEnemyOperationsByAnyIdentity(state, order.m_sOperationId, order.m_sOrderId, order.m_sManifestId) > 0)
			return BuildRejected("exact enemy directed-response identity is already owned by another operation");

		HST_OperationRecordState operation = new HST_OperationRecordState();
		operation.m_sOperationId = order.m_sOperationId;
		operation.m_eType = ResolveExactEnemyDirectedOperationType(order);
		operation.m_iContractVersion = ResolveExactEnemyDirectedContractVersion(order);
		operation.m_sOwnerFactionKey = order.m_sFactionKey;
		operation.m_sEnemyOrderId = order.m_sOrderId;
		operation.m_sManifestId = manifest.m_sManifestId;
		operation.m_sOriginZoneId = order.m_sSourceZoneId;
		operation.m_vOriginPosition = order.m_vSourcePosition;
		operation.m_sAssignmentKind = ResolveExactEnemyDirectedAssignmentKind(order);
		operation.m_sAssignmentZoneId = order.m_sTargetZoneId;
		operation.m_vAssignmentPosition = order.m_vTargetPosition;
		operation.m_sTacticalTargetZoneId = order.m_sTargetZoneId;
		operation.m_vTacticalTargetPosition = order.m_vTargetPosition;
		operation.m_vStrategicPosition = order.m_vSourcePosition;
		operation.m_iProjectionContractVersion = HST_StrategicMovementService.EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION;
		operation.m_iRouteVersion = HST_StrategicMovementService.DIRECT_ROUTE_VERSION;
		operation.m_vRouteStartPosition = order.m_vSourcePosition;
		operation.m_vRouteEndPosition = order.m_vTargetPosition;
		operation.m_fRouteTotalDistanceMeters = Distance2D(order.m_vSourcePosition, order.m_vTargetPosition);
		operation.m_fStrategicSpeedMetersPerSecond = HST_StrategicMovementService.EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND;
		operation.m_iStrategicLastUpdateSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iVirtualCombatLastStepSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iLastVirtualFriendlyCount = manifest.m_iAcceptedMemberCount;
		operation.m_sRecallPolicyId = ResolveExactEnemyDirectedRecallPolicy(order);
		operation.m_sSettlementPolicyId = ResolveExactEnemyDirectedSettlementPolicy(order);
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING;
		operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		operation.m_eTerminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE;
		operation.m_iDeterministicSeed = manifest.m_iDeterministicSeed;
		operation.m_iCreatedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iDutyStateEnteredAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iEngagementStateEnteredAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iMaterializationStateEnteredAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iLastProgressAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		operation.m_iRevision = 1;
		state.m_aOperations.Insert(operation);
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult RemoveUncommittedExactEnemyDefensiveQRF(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!RequiresOperation(order))
			return BuildAccepted(null, false, true);
		if (!state || !order)
			return BuildRejected("exact enemy defensive QRF rollback context is missing");
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		if (!operation)
			return BuildAccepted(null, false, true);
		string failure = ValidateExactEnemyDefensiveQRF(state, operation, order, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| !operation.m_sSpawnResultId.IsEmpty() || !operation.m_sGroupId.IsEmpty())
			return BuildRejected("exact enemy defensive QRF has execution or settlement authority and cannot be rolled back");
		int index = state.m_aOperations.Find(operation);
		if (index < 0)
			return BuildRejected("exact enemy defensive QRF rollback identity disappeared");
		state.m_aOperations.Remove(index);
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult LinkExactEnemyDefensiveQRFOutboundVirtual(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch)
	{
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		failure = ValidateEnemyDefensiveQRFProjectionLinks(operation, manifest, group, batch);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!batch.m_bStrategicProjectionHeld)
			return BuildRejected("exact enemy defensive QRF outbound batch is not held for strategic projection");
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING
				&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return BuildRejected("exact enemy defensive QRF cannot become virtual outbound from the current state");
		if (!LinkMatchesOrEmpty(operation.m_sSpawnResultId, batch.m_sResultId)
			|| !LinkMatchesOrEmpty(operation.m_sForceId, batch.m_sForceId)
			|| !LinkMatchesOrEmpty(operation.m_sProjectionId, batch.m_sProjectionId)
			|| !LinkMatchesOrEmpty(operation.m_sGroupId, group.m_sGroupId))
			return BuildRejected("exact enemy defensive QRF outbound would replace authoritative projection links");

		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		int living = queue.CountStrategicLivingMemberSlots(batch);
		if (living <= 0 || living > manifest.m_iAcceptedMemberCount)
			return BuildRejected("exact enemy defensive QRF outbound roster does not match the frozen manifest");

		bool changed;
		changed = AssignString(operation.m_sSpawnResultId, batch.m_sResultId) || changed;
		changed = AssignString(operation.m_sForceId, batch.m_sForceId) || changed;
		changed = AssignString(operation.m_sProjectionId, batch.m_sProjectionId) || changed;
		changed = AssignString(operation.m_sGroupId, group.m_sGroupId) || changed;
		changed = AssignString(operation.m_sCurrentRouteId, group.m_sRouteId) || changed;
		changed = AssignString(order.m_sSpawnResultId, batch.m_sResultId) || changed;
		changed = AssignString(order.m_sGroupId, group.m_sGroupId) || changed;
		changed = AssignVector(operation.m_vStrategicPosition, group.m_vPosition) || changed;
		if (operation.m_iLastVirtualFriendlyCount != living)
		{
			operation.m_iLastVirtualFriendlyCount = living;
			changed = true;
		}
		changed = SetDuty(operation, HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND, state.m_iElapsedSeconds) || changed;
		changed = SetResumeDuty(operation, operation.m_eDutyState) || changed;
		changed = ResetArrivalConfirmation(operation) || changed;
		order.m_bPhysicalized = false;
		order.m_bAbstractResolved = false;
		order.m_bStrategicServiceCommitted = true;
		order.m_sRuntimeStatus = "exact_virtual_outbound";
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult MarkExactEnemyDefensiveQRFMaterializingFromVirtual(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch,
		string reason)
	{
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		failure = ValidateEnemyDefensiveQRFProjectionLinks(operation, manifest, group, batch);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (batch.m_bStrategicProjectionHeld)
			return BuildRejected("exact enemy defensive QRF materialization batch remains strategically held");
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return BuildRejected("exact enemy defensive QRF cannot materialize from the current projection state");
		bool changed = SetMaterialization(operation, HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING, HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC, state.m_iElapsedSeconds);
		changed = AssignVector(operation.m_vStrategicPosition, group.m_vPosition) || changed;
		changed = AssignString(operation.m_sLastProjectionReason, reason) || changed;
		operation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
		order.m_sRuntimeStatus = "exact_materializing";
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult MarkExactEnemyDefensiveQRFPhysical(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch,
		string reason)
	{
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		failure = ValidateEnemyDefensiveQRFProjectionLinks(operation, manifest, group, batch);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			return BuildRejected("exact enemy defensive QRF physical batch has no successful handoff");
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
				&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL))
			return BuildRejected("exact enemy defensive QRF cannot become physical from the current projection state");

		bool changed = SetMaterialization(operation, HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL, HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE, state.m_iElapsedSeconds);
		changed = AssignVector(operation.m_vStrategicPosition, group.m_vPosition) || changed;
		changed = AssignString(operation.m_sLastProjectionReason, reason) || changed;
		operation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
		order.m_bPhysicalized = true;
		order.m_iPhysicalizedAtSecond = Math.Max(order.m_iPhysicalizedAtSecond, state.m_iElapsedSeconds);
		order.m_sRuntimeStatus = "exact_physical";
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult UpdateExactEnemyDefensiveQRFPhysicalPosition(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group)
	{
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!EnemyDefensiveQRFGroupLinksMatch(operation, manifest, group)
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
			return BuildRejected("exact enemy defensive QRF physical position does not own live authority");
		bool changed = AssignVector(operation.m_vStrategicPosition, group.m_vPosition);
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult BeginExactEnemyDefensiveQRFDematerialization(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		string reason)
	{
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!EnemyDefensiveQRFGroupLinksMatch(operation, manifest, group)
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
			return BuildRejected("exact enemy defensive QRF cannot dematerialize from the current projection state");
		bool changed = AssignVector(operation.m_vStrategicPosition, group.m_vPosition);
		changed = SetMaterialization(operation, HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING, HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE, state.m_iElapsedSeconds) || changed;
		changed = AssignString(operation.m_sLastProjectionReason, reason) || changed;
		operation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
		order.m_sRuntimeStatus = "exact_dematerializing";
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult CompleteExactEnemyDefensiveQRFDematerialization(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch,
		string reason)
	{
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		failure = ValidateEnemyDefensiveQRFProjectionLinks(operation, manifest, group, batch);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!batch.m_bStrategicProjectionHeld)
			return BuildRejected("exact enemy defensive QRF dematerialization batch is not held");
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
			return BuildRejected("exact enemy defensive QRF dematerialization completion is out of order");
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		int living = queue.CountStrategicLivingMemberSlots(batch);
		bool changed = AssignVector(operation.m_vStrategicPosition, group.m_vPosition);
		changed = SetMaterialization(operation, HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL, HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC, state.m_iElapsedSeconds) || changed;
		changed = AssignString(operation.m_sLastProjectionReason, reason) || changed;
		if (operation.m_iLastVirtualFriendlyCount != living)
		{
			operation.m_iLastVirtualFriendlyCount = Math.Max(0, living);
			changed = true;
		}
		operation.m_iStrategicLastUpdateSecond = state.m_iElapsedSeconds;
		operation.m_iVirtualCombatLastStepSecond = state.m_iElapsedSeconds;
		operation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
		operation.m_sLastVirtualCombatReason = "physical interval excluded from virtual combat catch-up";
		order.m_bPhysicalized = false;
		order.m_sRuntimeStatus = "exact_virtual";
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult ConfirmExactEnemyDefensiveQRFArrivalSample(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group)
	{
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!EnemyDefensiveQRFGroupLinksMatch(operation, manifest, group)
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			|| (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
				&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN))
			return BuildRejected("exact enemy defensive QRF arrival sample lacks live route authority");

		bool changed = AssignVector(operation.m_vStrategicPosition, group.m_vPosition);
		float arrivalDistance = Distance2D(group.m_vPosition, operation.m_vRouteEndPosition);
		if (arrivalDistance > EXACT_ENEMY_DEFENSIVE_QRF_ARRIVAL_RADIUS_METERS)
		{
			changed = ResetArrivalConfirmation(operation) || changed;
			return FinishTransition(operation, changed, state.m_iElapsedSeconds);
		}
		if (operation.m_iLastArrivalConfirmationSecond == state.m_iElapsedSeconds)
			return FinishTransition(operation, changed, state.m_iElapsedSeconds);
		if (operation.m_iArrivalConfirmationCount < 2)
		{
			operation.m_iArrivalConfirmationCount++;
			changed = true;
		}
		operation.m_iLastArrivalConfirmationSecond = state.m_iElapsedSeconds;
		return FinishTransition(operation, true, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult MarkExactEnemyDefensiveQRFOnStation(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group)
	{
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!EnemyDefensiveQRFGroupLinksMatch(operation, manifest, group)
			|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
			return BuildRejected("exact enemy defensive QRF arrival conflicts with outbound authority");
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
		{
			if (operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
				|| operation.m_iArrivalConfirmationCount < 2)
				return BuildRejected("exact enemy defensive QRF physical arrival requires two distinct-second confirmations");
		}
		else if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			|| Distance2D(operation.m_vStrategicPosition, operation.m_vRouteEndPosition) > HST_StrategicMovementService.ARRIVAL_EPSILON_METERS)
			return BuildRejected("exact enemy defensive QRF virtual arrival has not reached the target");

		bool changed = SetDuty(operation, HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION, state.m_iElapsedSeconds);
		changed = SetResumeDuty(operation, operation.m_eDutyState) || changed;
		changed = ResetArrivalConfirmation(operation) || changed;
		operation.m_iVirtualCombatLastStepSecond = state.m_iElapsedSeconds;
		order.m_bAbstractResolved = operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		if (order.m_bAbstractResolved)
			order.m_sRuntimeStatus = "exact_virtual_on_station";
		else
			order.m_sRuntimeStatus = "exact_physical_on_station";
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult BeginExactEnemyDefensiveQRFReturnToOrigin(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group)
	{
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		bool rebuildPrearrivalReturn = RequiresExactEnemyGarrisonRebuild(order)
			&& operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		if (!EnemyDefensiveQRFGroupLinksMatch(operation, manifest, group)
			|| (!rebuildPrearrivalReturn
				&& operation.m_eDutyState
					!= HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
			return BuildRejected("exact enemy defensive QRF cannot begin return from the current state");

		vector returnStart = operation.m_vStrategicPosition;
		if (!IsZeroVector(group.m_vPosition))
			returnStart = group.m_vPosition;
		bool changed = AssignVector(operation.m_vStrategicPosition, returnStart);
		changed = AssignVector(operation.m_vRouteStartPosition, returnStart) || changed;
		changed = AssignVector(operation.m_vRouteEndPosition, operation.m_vOriginPosition) || changed;
		float returnDistance = Distance2D(returnStart, operation.m_vOriginPosition);
		if (operation.m_fRouteTotalDistanceMeters != returnDistance)
		{
			operation.m_fRouteTotalDistanceMeters = returnDistance;
			changed = true;
		}
		if (operation.m_fRouteProgressMeters != 0)
		{
			operation.m_fRouteProgressMeters = 0;
			changed = true;
		}
		changed = AssignString(operation.m_sCurrentRouteId, operation.m_sOperationId + "_return") || changed;
		changed = AssignString(group.m_sRouteId, operation.m_sCurrentRouteId) || changed;
		changed = AssignVector(group.m_vSourcePosition, returnStart) || changed;
		changed = AssignVector(group.m_vTargetPosition, operation.m_vOriginPosition) || changed;
		changed = AssignString(operation.m_sTacticalTargetZoneId, operation.m_sOriginZoneId) || changed;
		changed = AssignVector(operation.m_vTacticalTargetPosition, operation.m_vOriginPosition) || changed;
		changed = SetDuty(operation, HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN, state.m_iElapsedSeconds) || changed;
		changed = SetResumeDuty(operation, operation.m_eDutyState) || changed;
		changed = ResetArrivalConfirmation(operation) || changed;
		operation.m_iStrategicLastUpdateSecond = state.m_iElapsedSeconds;
		operation.m_iVirtualCombatLastStepSecond = state.m_iElapsedSeconds;
		operation.m_sLastVirtualCombatReason = "return leg excludes completed defensive engagement";
		order.m_bAbstractResolved = false;
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			order.m_sRuntimeStatus = "exact_physical_returning";
		else
			order.m_sRuntimeStatus = "exact_virtual_returning";
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult RecordExactEnemyDefensiveQRFEngagement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationEngagementMode nextMode)
	{
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| !IsActiveDuty(operation.m_eDutyState))
			return BuildRejected("exact enemy defensive QRF cannot change engagement in the current state");
		if (operation.m_eEngagementMode == nextMode)
			return BuildAccepted(operation, false, true);
		if (!IsLegalEngagementTransition(operation.m_eEngagementMode, nextMode))
			return BuildRejected("illegal exact enemy defensive QRF engagement transition");
		if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
			operation.m_eResumeDutyState = operation.m_eDutyState;
		operation.m_eEngagementMode = nextMode;
		operation.m_iEngagementStateEnteredAtSecond = state.m_iElapsedSeconds;
		if (nextMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT
			|| nextMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED)
			operation.m_iLastContactAtSecond = state.m_iElapsedSeconds;
		operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		operation.m_iRevision++;
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult CanPrepareExactEnemyDefensiveQRFSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId)
	{
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!IsEnemyDefensiveQRFTerminalResult(terminalResult) || settlementId.IsEmpty())
			return BuildRejected("exact enemy defensive QRF terminal result or settlement identity is invalid");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (operation.m_eTerminalResult == terminalResult && operation.m_sSettlementId == settlementId)
				return BuildAccepted(operation, false, true);
			return BuildRejected("exact enemy defensive QRF is already settled with a conflicting result");
		}
		if (RequiresExactEnemyDefensiveQRF(order)
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			if (operation.m_eTerminalResult == terminalResult
				&& operation.m_sSettlementId == settlementId
				&& operation.m_iSettledAtSecond > 0)
				return BuildAccepted(operation, false, true);
			return BuildRejected("prepared exact enemy defensive QRF settlement intent conflicts");
		}
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
			return BuildRejected("exact enemy defensive QRF settlement state conflicts");
		if (terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED)
		{
			if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
				return BuildRejected("completed exact enemy defensive QRF has not begun its return leg");
			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			{
				if (operation.m_iArrivalConfirmationCount < 2)
					return BuildRejected("completed exact enemy defensive QRF requires two physical return confirmations");
			}
			else if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				|| Distance2D(operation.m_vStrategicPosition, operation.m_vOriginPosition) > HST_StrategicMovementService.ARRIVAL_EPSILON_METERS)
				return BuildRejected("completed exact enemy defensive QRF has not reached its origin");
		}
		return BuildAccepted(operation, false, false);
	}

	HST_OperationTransitionResult PrepareExactEnemyDefensiveQRFSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		if (!RequiresExactEnemyDefensiveQRF(order))
			return BuildRejected("exact enemy defensive QRF settlement received another order family");
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			bool preparedExact = operation.m_eTerminalResult == terminalResult
				&& operation.m_sSettlementId == settlementId
				&& operation.m_sTerminalReason == reason
				&& operation.m_iSettledAtSecond > 0;
			if (preparedExact)
				return BuildAccepted(operation, false, true);
			return BuildRejected("prepared exact enemy defensive QRF settlement intent conflicts");
		}

		HST_OperationTransitionResult preflight = CanPrepareExactEnemyDefensiveQRFSettlement(
			state,
			order,
			terminalResult,
			settlementId);
		if (!preflight || !preflight.m_bAccepted || !preflight.m_Operation
			|| preflight.m_bAlreadyApplied)
			return preflight;
		if (reason.IsEmpty() || state.m_iElapsedSeconds <= 0)
			return BuildRejected("exact enemy defensive QRF prepared settlement reason or timestamp is invalid");

		operation = preflight.m_Operation;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED;
		operation.m_eTerminalResult = terminalResult;
		operation.m_sSettlementId = settlementId;
		operation.m_sTerminalReason = reason;
		// The prepared-at timestamp is durable transaction chronology and remains
		// unchanged when the terminal operation is finalized.
		operation.m_iSettledAtSecond = state.m_iElapsedSeconds;
		operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		operation.m_iRevision++;
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult CanFinalizePreparedExactEnemyDefensiveQRFSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		if (!RequiresExactEnemyDefensiveQRF(order))
			return BuildRejected("exact enemy defensive QRF settlement received another order family");
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (operation.m_eTerminalResult == terminalResult
				&& operation.m_sSettlementId == settlementId
				&& operation.m_sTerminalReason == reason)
				return BuildAccepted(operation, false, true);
			return BuildRejected("exact enemy defensive QRF is already settled with a conflicting result");
		}
		if (operation.m_eSettlementState
				!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
			|| operation.m_eTerminalResult != terminalResult
			|| operation.m_sSettlementId != settlementId
			|| operation.m_sTerminalReason != reason
			|| operation.m_iSettledAtSecond <= 0
			|| operation.m_iSettledAtSecond > state.m_iElapsedSeconds)
			return BuildRejected("exact enemy defensive QRF prepared settlement intent conflicts");
		if (!order.m_bResourceSettlementApplied || order.m_sResourceSettlementId.IsEmpty()
			|| order.m_sResourceSettlementKind.IsEmpty()
			|| order.m_sResourceSettlementId
				!= BuildSettlementId(order.m_sOperationId, order.m_sResourceSettlementKind)
			|| order.m_sResourceSettlementId != settlementId
			|| order.m_sResourceRefundMutationId != "enemy_resource_refund_" + settlementId)
			return BuildRejected("exact enemy defensive QRF prepared resource settlement is incomplete");
		return BuildAccepted(operation, false, false);
	}

	HST_OperationTransitionResult CanSettleExactEnemyDefensiveQRF(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId)
	{
		HST_OperationRecordState operation;
		if (state && order)
			operation = state.FindOperation(order.m_sOperationId);
		if (RequiresExactEnemyDefensiveQRF(order) && operation
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			return CanFinalizePreparedExactEnemyDefensiveQRFSettlement(
				state,
				order,
				terminalResult,
				settlementId,
				operation.m_sTerminalReason);
		}
		HST_OperationTransitionResult prepared = CanPrepareExactEnemyDefensiveQRFSettlement(
			state,
			order,
			terminalResult,
			settlementId);
		if (!prepared || !prepared.m_bAccepted)
			return prepared;
		if (!order.m_bResourceSettlementApplied || order.m_sResourceSettlementId.IsEmpty()
			|| order.m_sResourceSettlementKind.IsEmpty()
			|| order.m_sResourceSettlementId != BuildSettlementId(order.m_sOperationId, order.m_sResourceSettlementKind)
			|| order.m_sResourceSettlementId != settlementId)
			return BuildRejected("exact enemy defensive QRF resource settlement has not been applied for this terminal receipt");
		return prepared;
	}

	HST_OperationTransitionResult CanRecordExactEnemyDefensiveQRFResourceSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		string settlementKind,
		int acceptedMemberCount,
		int survivorMemberCount,
		string refundMutationId = "",
		int refundedAttackResources = 0,
		int refundedSupportResources = 0)
	{
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (settlementKind.IsEmpty() || acceptedMemberCount != manifest.m_iAcceptedMemberCount
			|| survivorMemberCount < 0 || survivorMemberCount > acceptedMemberCount)
			return BuildRejected("exact enemy defensive QRF resource settlement roster is invalid");
		string settlementId = BuildSettlementId(operation.m_sOperationId, settlementKind);
		if (settlementId.IsEmpty())
			return BuildRejected("exact enemy defensive QRF resource settlement identity is invalid");
		bool requireFullReceipt = !refundMutationId.IsEmpty();
		if (requireFullReceipt)
		{
			int expectedAttackRefund;
			int expectedSupportRefund;
			if (settlementKind.Contains("_full"))
			{
				expectedAttackRefund = Math.Max(0, order.m_iAttackCost);
				expectedSupportRefund = Math.Max(0, order.m_iSupportCost);
			}
			else if (acceptedMemberCount > 0)
			{
				expectedAttackRefund = Math.Max(0, order.m_iAttackCost)
					* survivorMemberCount / acceptedMemberCount;
				expectedSupportRefund = Math.Max(0, order.m_iSupportCost)
					* survivorMemberCount / acceptedMemberCount;
			}
			if (refundMutationId != "enemy_resource_refund_" + settlementId
				|| refundedAttackResources != expectedAttackRefund
				|| refundedSupportResources != expectedSupportRefund)
				return BuildRejected("exact enemy defensive QRF resource refund receipt is invalid");
		}
		if (RequiresExactEnemyDefensiveQRF(order)
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			bool stagedExact = requireFullReceipt
				&& operation.m_sSettlementId == settlementId
				&& order.m_sResourceSettlementId == settlementId
				&& order.m_sResourceSettlementKind == settlementKind
				&& order.m_sResourceRefundMutationId == refundMutationId
				&& order.m_iSettlementAcceptedMemberCount == acceptedMemberCount
				&& order.m_iSettlementSurvivorMemberCount == survivorMemberCount
				&& order.m_iRefundedAttackResources == refundedAttackResources
				&& order.m_iRefundedSupportResources == refundedSupportResources;
			if (!stagedExact)
				return BuildRejected("prepared exact enemy defensive QRF resource settlement intent conflicts");
			if (order.m_bResourceSettlementApplied)
				return BuildAccepted(operation, false, true);
			return BuildAccepted(operation, false, false);
		}
		if (order.m_bResourceSettlementApplied)
		{
			bool receiptExact = order.m_sResourceSettlementId == settlementId
				&& order.m_sResourceSettlementKind == settlementKind
				&& order.m_iSettlementAcceptedMemberCount == acceptedMemberCount
				&& order.m_iSettlementSurvivorMemberCount == survivorMemberCount;
			if (requireFullReceipt)
			{
				receiptExact = receiptExact
					&& order.m_sResourceRefundMutationId == refundMutationId
					&& order.m_iRefundedAttackResources == refundedAttackResources
					&& order.m_iRefundedSupportResources == refundedSupportResources;
			}
			if (receiptExact)
				return BuildAccepted(operation, false, true);
			return BuildRejected("exact enemy defensive QRF resource settlement already conflicts");
		}
		bool hasPartialAuthority = !order.m_sResourceSettlementId.IsEmpty()
			|| !order.m_sResourceSettlementKind.IsEmpty()
			|| order.m_iSettlementAcceptedMemberCount != 0
			|| order.m_iSettlementSurvivorMemberCount != 0;
		if (requireFullReceipt)
		{
			hasPartialAuthority = hasPartialAuthority
				|| !order.m_sResourceRefundMutationId.IsEmpty()
				|| order.m_iRefundedAttackResources != 0
				|| order.m_iRefundedSupportResources != 0;
		}
		if (hasPartialAuthority)
			return BuildRejected("exact enemy defensive QRF contains partial resource settlement authority");
		return BuildAccepted(operation, false, false);
	}

	HST_OperationTransitionResult RecordExactEnemyDefensiveQRFResourceSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		string settlementKind,
		int acceptedMemberCount,
		int survivorMemberCount,
		string refundMutationId = "",
		int refundedAttackResources = 0,
		int refundedSupportResources = 0)
	{
		HST_OperationTransitionResult preflight
			= CanRecordExactEnemyDefensiveQRFResourceSettlement(
				state,
				order,
				settlementKind,
				acceptedMemberCount,
				survivorMemberCount,
				refundMutationId,
				refundedAttackResources,
				refundedSupportResources);
		if (!preflight || !preflight.m_bAccepted || !preflight.m_Operation
			|| preflight.m_bAlreadyApplied)
			return preflight;
		HST_OperationRecordState operation = preflight.m_Operation;
		string settlementId = BuildSettlementId(operation.m_sOperationId, settlementKind);
		order.m_sResourceSettlementId = settlementId;
		order.m_sResourceSettlementKind = settlementKind;
		if (!refundMutationId.IsEmpty())
		{
			order.m_sResourceRefundMutationId = refundMutationId;
			order.m_iRefundedAttackResources = refundedAttackResources;
			order.m_iRefundedSupportResources = refundedSupportResources;
		}
		order.m_iSettlementAcceptedMemberCount = acceptedMemberCount;
		order.m_iSettlementSurvivorMemberCount = survivorMemberCount;
		// This flag is the commit marker for the complete receipt above.
		order.m_bResourceSettlementApplied = true;
		operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		operation.m_iRevision++;
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult FinalizePreparedExactEnemyDefensiveQRFSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		HST_OperationTransitionResult preflight
			= CanFinalizePreparedExactEnemyDefensiveQRFSettlement(
				state,
				order,
				terminalResult,
				settlementId,
				reason);
		if (!preflight || !preflight.m_bAccepted || !preflight.m_Operation
			|| preflight.m_bAlreadyApplied)
			return preflight;
		HST_OperationRecordState operation = preflight.m_Operation;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		// Preserve terminal result, settlement identity, reason, and the prepared-at
		// timestamp as the durable transaction chronology.
		operation.m_iDutyStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iEngagementStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		ResetArrivalConfirmation(operation);
		operation.m_iRevision++;
		order.m_bPhysicalized = false;
		order.m_bAbstractResolved = false;
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult SettleExactEnemyDefensiveQRF(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		HST_OperationRecordState operation;
		if (state && order)
			operation = state.FindOperation(order.m_sOperationId);
		if (RequiresExactEnemyDefensiveQRF(order) && operation
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			return FinalizePreparedExactEnemyDefensiveQRFSettlement(
				state,
				order,
				terminalResult,
				settlementId,
				reason);
		}
		HST_OperationTransitionResult preflight = CanSettleExactEnemyDefensiveQRF(state, order, terminalResult, settlementId);
		if (!preflight || !preflight.m_bAccepted || !preflight.m_Operation
			|| preflight.m_bAlreadyApplied)
			return preflight;
		operation = preflight.m_Operation;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		operation.m_eTerminalResult = terminalResult;
		operation.m_sSettlementId = settlementId;
		operation.m_sTerminalReason = reason;
		operation.m_iDutyStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iEngagementStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		operation.m_iSettledAtSecond = state.m_iElapsedSeconds;
		ResetArrivalConfirmation(operation);
		operation.m_iRevision++;
		order.m_bPhysicalized = false;
		order.m_bAbstractResolved = false;
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult RegisterExactEnemyCounterattack(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack registration received another order family");
		return RegisterExactEnemyDefensiveQRF(state, order, manifest);
	}

	HST_OperationTransitionResult RemoveUncommittedExactEnemyCounterattack(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack rollback received another order family");
		return RemoveUncommittedExactEnemyDefensiveQRF(state, order, manifest);
	}

	HST_OperationTransitionResult LinkExactEnemyCounterattackOutboundVirtual(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack transition received another order family");
		return LinkExactEnemyDefensiveQRFOutboundVirtual(state, order, group, batch);
	}

	HST_OperationTransitionResult MarkExactEnemyCounterattackMaterializingFromVirtual(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch,
		string reason)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack transition received another order family");
		return MarkExactEnemyDefensiveQRFMaterializingFromVirtual(state, order, group, batch, reason);
	}

	HST_OperationTransitionResult MarkExactEnemyCounterattackPhysical(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch,
		string reason)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack transition received another order family");
		return MarkExactEnemyDefensiveQRFPhysical(state, order, group, batch, reason);
	}

	HST_OperationTransitionResult UpdateExactEnemyCounterattackPhysicalPosition(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack transition received another order family");
		return UpdateExactEnemyDefensiveQRFPhysicalPosition(state, order, group);
	}

	HST_OperationTransitionResult BeginExactEnemyCounterattackDematerialization(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		string reason)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack transition received another order family");
		return BeginExactEnemyDefensiveQRFDematerialization(state, order, group, reason);
	}

	HST_OperationTransitionResult CompleteExactEnemyCounterattackDematerialization(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch,
		string reason)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack transition received another order family");
		return CompleteExactEnemyDefensiveQRFDematerialization(state, order, group, batch, reason);
	}

	HST_OperationTransitionResult ConfirmExactEnemyCounterattackArrivalSample(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack transition received another order family");
		return ConfirmExactEnemyDefensiveQRFArrivalSample(state, order, group);
	}

	HST_OperationTransitionResult MarkExactEnemyCounterattackOnStation(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack transition received another order family");
		return MarkExactEnemyDefensiveQRFOnStation(state, order, group);
	}

	HST_OperationTransitionResult BeginExactEnemyCounterattackReturnToOrigin(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack transition received another order family");
		return BeginExactEnemyDefensiveQRFReturnToOrigin(state, order, group);
	}

	HST_OperationTransitionResult RecordExactEnemyCounterattackEngagement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationEngagementMode nextMode)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack transition received another order family");
		return RecordExactEnemyDefensiveQRFEngagement(state, order, nextMode);
	}

	HST_OperationTransitionResult CanPrepareExactEnemyCounterattackSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack settlement received another order family");
		HST_OperationRecordState operation;
		if (state && order)
			operation = state.FindOperation(order.m_sOperationId);
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			HST_ForceManifestState manifest;
			string failure = ResolveEnemyDefensiveQRFTransition(
				state,
				order,
				operation,
				manifest);
			if (!failure.IsEmpty())
				return BuildRejected(failure);
			if (operation.m_eTerminalResult == terminalResult
				&& operation.m_sSettlementId == settlementId
				&& operation.m_iSettledAtSecond > 0)
				return BuildAccepted(operation, false, true);
			return BuildRejected("prepared exact enemy counterattack settlement intent conflicts");
		}
		return CanPrepareExactEnemyDefensiveQRFSettlement(state, order, terminalResult, settlementId);
	}

	HST_OperationTransitionResult PrepareExactEnemyCounterattackSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack settlement received another order family");
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			bool preparedExact = operation.m_eTerminalResult == terminalResult
				&& operation.m_sSettlementId == settlementId
				&& operation.m_sTerminalReason == reason
				&& operation.m_iSettledAtSecond > 0;
			if (preparedExact)
				return BuildAccepted(operation, false, true);
			return BuildRejected("prepared exact enemy counterattack settlement intent conflicts");
		}

		HST_OperationTransitionResult preflight = CanPrepareExactEnemyDefensiveQRFSettlement(
			state,
			order,
			terminalResult,
			settlementId);
		if (!preflight || !preflight.m_bAccepted || !preflight.m_Operation
			|| preflight.m_bAlreadyApplied)
			return preflight;
		if (reason.IsEmpty() || state.m_iElapsedSeconds <= 0)
			return BuildRejected("exact enemy counterattack prepared settlement reason or timestamp is invalid");

		operation = preflight.m_Operation;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED;
		operation.m_eTerminalResult = terminalResult;
		operation.m_sSettlementId = settlementId;
		operation.m_sTerminalReason = reason;
		// The prepared-at timestamp intentionally occupies the existing settled
		// field and is retained unchanged when the terminal receipt is finalized.
		operation.m_iSettledAtSecond = state.m_iElapsedSeconds;
		operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		operation.m_iRevision++;
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult CanFinalizePreparedExactEnemyCounterattackSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack settlement received another order family");
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (operation.m_eTerminalResult == terminalResult
				&& operation.m_sSettlementId == settlementId
				&& operation.m_sTerminalReason == reason)
				return BuildAccepted(operation, false, true);
			return BuildRejected("exact enemy counterattack is already settled with a conflicting result");
		}
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
			|| operation.m_eTerminalResult != terminalResult
			|| operation.m_sSettlementId != settlementId
			|| operation.m_sTerminalReason != reason
			|| operation.m_iSettledAtSecond <= 0
			|| operation.m_iSettledAtSecond > state.m_iElapsedSeconds)
			return BuildRejected("exact enemy counterattack prepared settlement intent conflicts");
		if (!order.m_bResourceSettlementApplied || order.m_sResourceSettlementId.IsEmpty()
			|| order.m_sResourceSettlementKind.IsEmpty()
			|| order.m_sResourceSettlementId
				!= BuildSettlementId(order.m_sOperationId, order.m_sResourceSettlementKind)
			|| order.m_sResourceSettlementId != settlementId)
			return BuildRejected("exact enemy counterattack prepared resource settlement is incomplete");
		return BuildAccepted(operation, false, false);
	}

	HST_OperationTransitionResult CanSettleExactEnemyCounterattack(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack settlement received another order family");
		HST_OperationRecordState operation;
		if (state && order)
			operation = state.FindOperation(order.m_sOperationId);
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			return CanFinalizePreparedExactEnemyCounterattackSettlement(
				state,
				order,
				terminalResult,
				settlementId,
				operation.m_sTerminalReason);
		}
		return CanSettleExactEnemyDefensiveQRF(state, order, terminalResult, settlementId);
	}

	HST_OperationTransitionResult CanRecordExactEnemyCounterattackResourceSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		string settlementKind,
		int acceptedMemberCount,
		int survivorMemberCount)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack settlement received another order family");
		HST_OperationRecordState operation;
		if (state && order)
			operation = state.FindOperation(order.m_sOperationId);
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			HST_ForceManifestState manifest;
			string failure = ResolveEnemyDefensiveQRFTransition(
				state,
				order,
				operation,
				manifest);
			if (!failure.IsEmpty())
				return BuildRejected(failure);
			if (settlementKind.IsEmpty()
				|| acceptedMemberCount != manifest.m_iAcceptedMemberCount
				|| survivorMemberCount < 0
				|| survivorMemberCount > acceptedMemberCount)
				return BuildRejected("prepared exact enemy counterattack resource settlement roster is invalid");
			string expectedSettlementId = BuildSettlementId(order.m_sOperationId, settlementKind);
			bool stagedExact = order.m_sResourceSettlementId == expectedSettlementId
				&& order.m_sResourceSettlementKind == settlementKind
				&& order.m_iSettlementAcceptedMemberCount == acceptedMemberCount
				&& order.m_iSettlementSurvivorMemberCount == survivorMemberCount;
			if (!stagedExact || operation.m_sSettlementId != expectedSettlementId)
				return BuildRejected("prepared exact enemy counterattack resource settlement intent conflicts");
			if (order.m_bResourceSettlementApplied)
				return BuildAccepted(operation, false, true);
			return BuildAccepted(operation, false, false);
		}
		return CanRecordExactEnemyDefensiveQRFResourceSettlement(
			state,
			order,
			settlementKind,
			acceptedMemberCount,
			survivorMemberCount);
	}

	HST_OperationTransitionResult RecordExactEnemyCounterattackResourceSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		string settlementKind,
		int acceptedMemberCount,
		int survivorMemberCount)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack settlement received another order family");
		HST_OperationTransitionResult preflight
			= CanRecordExactEnemyCounterattackResourceSettlement(
				state,
				order,
				settlementKind,
				acceptedMemberCount,
				survivorMemberCount);
		if (!preflight || !preflight.m_bAccepted || !preflight.m_Operation
			|| preflight.m_bAlreadyApplied)
			return preflight;
		HST_OperationRecordState operation = preflight.m_Operation;
		if (operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			return RecordExactEnemyDefensiveQRFResourceSettlement(
				state,
				order,
				settlementKind,
				acceptedMemberCount,
				survivorMemberCount);
		}
		order.m_sResourceSettlementId = BuildSettlementId(order.m_sOperationId, settlementKind);
		order.m_sResourceSettlementKind = settlementKind;
		order.m_iSettlementAcceptedMemberCount = acceptedMemberCount;
		order.m_iSettlementSurvivorMemberCount = survivorMemberCount;
		order.m_bResourceSettlementApplied = true;
		operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		operation.m_iRevision++;
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult FinalizePreparedExactEnemyCounterattackSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		HST_OperationTransitionResult preflight
			= CanFinalizePreparedExactEnemyCounterattackSettlement(
				state,
				order,
				terminalResult,
				settlementId,
				reason);
		if (!preflight || !preflight.m_bAccepted || !preflight.m_Operation
			|| preflight.m_bAlreadyApplied)
			return preflight;

		HST_OperationRecordState operation = preflight.m_Operation;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		// Preserve terminal result, settlement identity, reason, and the original
		// prepared-at timestamp as the durable transaction chronology.
		operation.m_iDutyStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iEngagementStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		ResetArrivalConfirmation(operation);
		operation.m_iRevision++;
		order.m_bPhysicalized = false;
		order.m_bAbstractResolved = false;
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult SettleExactEnemyCounterattack(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return BuildRejected("exact enemy counterattack settlement received another order family");
		HST_OperationRecordState operation;
		if (state && order)
			operation = state.FindOperation(order.m_sOperationId);
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			return FinalizePreparedExactEnemyCounterattackSettlement(
				state,
				order,
				terminalResult,
				settlementId,
				reason);
		}
		return SettleExactEnemyDefensiveQRF(state, order, terminalResult, settlementId, reason);
	}

	HST_OperationTransitionResult RegisterExactEnemyGarrisonRebuild(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild registration received another order family");
		return RegisterExactEnemyDefensiveQRF(state, order, manifest);
	}

	HST_OperationTransitionResult RemoveUncommittedExactEnemyGarrisonRebuild(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild rollback received another order family");
		return RemoveUncommittedExactEnemyDefensiveQRF(state, order, manifest);
	}

	HST_OperationTransitionResult LinkExactEnemyGarrisonRebuildOutboundVirtual(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild transition received another order family");
		return LinkExactEnemyDefensiveQRFOutboundVirtual(state, order, group, batch);
	}

	HST_OperationTransitionResult MarkExactEnemyGarrisonRebuildMaterializingFromVirtual(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch,
		string reason)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild transition received another order family");
		return MarkExactEnemyDefensiveQRFMaterializingFromVirtual(state, order, group, batch, reason);
	}

	HST_OperationTransitionResult MarkExactEnemyGarrisonRebuildPhysical(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch,
		string reason)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild transition received another order family");
		return MarkExactEnemyDefensiveQRFPhysical(state, order, group, batch, reason);
	}

	HST_OperationTransitionResult UpdateExactEnemyGarrisonRebuildPhysicalPosition(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild transition received another order family");
		return UpdateExactEnemyDefensiveQRFPhysicalPosition(state, order, group);
	}

	HST_OperationTransitionResult BeginExactEnemyGarrisonRebuildDematerialization(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		string reason)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild transition received another order family");
		return BeginExactEnemyDefensiveQRFDematerialization(state, order, group, reason);
	}

	HST_OperationTransitionResult CompleteExactEnemyGarrisonRebuildDematerialization(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch,
		string reason)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild transition received another order family");
		return CompleteExactEnemyDefensiveQRFDematerialization(state, order, group, batch, reason);
	}

	HST_OperationTransitionResult ConfirmExactEnemyGarrisonRebuildArrivalSample(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild transition received another order family");
		return ConfirmExactEnemyDefensiveQRFArrivalSample(state, order, group);
	}

	HST_OperationTransitionResult MarkExactEnemyGarrisonRebuildOnStation(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild transition received another order family");
		return MarkExactEnemyDefensiveQRFOnStation(state, order, group);
	}

	HST_OperationTransitionResult BeginExactEnemyGarrisonRebuildReturnToOrigin(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ActiveGroupState group)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild transition received another order family");
		return BeginExactEnemyDefensiveQRFReturnToOrigin(state, order, group);
	}

	HST_OperationTransitionResult RecordExactEnemyGarrisonRebuildEngagement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationEngagementMode nextMode)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild transition received another order family");
		return RecordExactEnemyDefensiveQRFEngagement(state, order, nextMode);
	}

	HST_OperationTransitionResult CanPrepareExactEnemyGarrisonRebuildSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild settlement received another order family");
		HST_OperationRecordState operation;
		if (state && order)
			operation = state.FindOperation(order.m_sOperationId);
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			HST_ForceManifestState manifest;
			string failure = ResolveEnemyDefensiveQRFTransition(
				state,
				order,
				operation,
				manifest);
			if (!failure.IsEmpty())
				return BuildRejected(failure);
			if (operation.m_eTerminalResult == terminalResult
				&& operation.m_sSettlementId == settlementId
				&& operation.m_iSettledAtSecond > 0)
				return BuildAccepted(operation, false, true);
			return BuildRejected("prepared exact enemy garrison rebuild settlement intent conflicts");
		}
		return CanPrepareExactEnemyDefensiveQRFSettlement(state, order, terminalResult, settlementId);
	}

	HST_OperationTransitionResult PrepareExactEnemyGarrisonRebuildSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild settlement received another order family");
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			bool preparedExact = operation.m_eTerminalResult == terminalResult
				&& operation.m_sSettlementId == settlementId
				&& operation.m_sTerminalReason == reason
				&& operation.m_iSettledAtSecond > 0;
			if (preparedExact)
				return BuildAccepted(operation, false, true);
			return BuildRejected("prepared exact enemy garrison rebuild settlement intent conflicts");
		}

		HST_OperationTransitionResult preflight = CanPrepareExactEnemyDefensiveQRFSettlement(
			state,
			order,
			terminalResult,
			settlementId);
		if (!preflight || !preflight.m_bAccepted || !preflight.m_Operation
			|| preflight.m_bAlreadyApplied)
			return preflight;
		if (reason.IsEmpty() || state.m_iElapsedSeconds <= 0)
			return BuildRejected("exact enemy garrison rebuild prepared settlement reason or timestamp is invalid");

		operation = preflight.m_Operation;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED;
		operation.m_eTerminalResult = terminalResult;
		operation.m_sSettlementId = settlementId;
		operation.m_sTerminalReason = reason;
		operation.m_iSettledAtSecond = state.m_iElapsedSeconds;
		operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		operation.m_iRevision++;
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult CanFinalizePreparedExactEnemyGarrisonRebuildSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild settlement received another order family");
		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(state, order, operation, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (operation.m_eTerminalResult == terminalResult
				&& operation.m_sSettlementId == settlementId
				&& operation.m_sTerminalReason == reason)
				return BuildAccepted(operation, false, true);
			return BuildRejected("exact enemy garrison rebuild is already settled with a conflicting result");
		}
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
			|| operation.m_eTerminalResult != terminalResult
			|| operation.m_sSettlementId != settlementId
			|| operation.m_sTerminalReason != reason
			|| operation.m_iSettledAtSecond <= 0
			|| operation.m_iSettledAtSecond > state.m_iElapsedSeconds)
			return BuildRejected("exact enemy garrison rebuild prepared settlement intent conflicts");
		if (!order.m_bResourceSettlementApplied || order.m_sResourceSettlementId.IsEmpty()
			|| order.m_sResourceSettlementKind.IsEmpty()
			|| order.m_sResourceSettlementId
				!= BuildSettlementId(order.m_sOperationId, order.m_sResourceSettlementKind)
			|| order.m_sResourceSettlementId != settlementId)
			return BuildRejected("exact enemy garrison rebuild prepared resource settlement is incomplete");
		return BuildAccepted(operation, false, false);
	}

	HST_OperationTransitionResult CanRecordExactEnemyGarrisonRebuildResourceSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		string settlementKind,
		int acceptedMemberCount,
		int survivorMemberCount)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild settlement received another order family");
		HST_OperationRecordState operation;
		if (state && order)
			operation = state.FindOperation(order.m_sOperationId);
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			HST_ForceManifestState manifest;
			string failure = ResolveEnemyDefensiveQRFTransition(
				state,
				order,
				operation,
				manifest);
			if (!failure.IsEmpty())
				return BuildRejected(failure);
			if (settlementKind.IsEmpty()
				|| acceptedMemberCount != manifest.m_iAcceptedMemberCount
				|| survivorMemberCount < 0
				|| survivorMemberCount > acceptedMemberCount)
				return BuildRejected("prepared exact enemy garrison rebuild resource settlement roster is invalid");
			string expectedSettlementId = BuildSettlementId(order.m_sOperationId, settlementKind);
			bool stagedExact = order.m_sResourceSettlementId == expectedSettlementId
				&& order.m_sResourceSettlementKind == settlementKind
				&& order.m_iSettlementAcceptedMemberCount == acceptedMemberCount
				&& order.m_iSettlementSurvivorMemberCount == survivorMemberCount;
			if (!stagedExact || operation.m_sSettlementId != expectedSettlementId)
				return BuildRejected("prepared exact enemy garrison rebuild resource settlement intent conflicts");
			if (order.m_bResourceSettlementApplied)
				return BuildAccepted(operation, false, true);
			return BuildAccepted(operation, false, false);
		}
		HST_ForceManifestState manifest;
		string failure = ResolveEnemyDefensiveQRFTransition(
			state,
			order,
			operation,
			manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (settlementKind != EXACT_ENEMY_GARRISON_REBUILD_DELIVERY_SETTLEMENT_KIND)
			return BuildRejected("open exact enemy garrison rebuild may record only its delivery receipt");
		if (acceptedMemberCount != manifest.m_iAcceptedMemberCount
			|| survivorMemberCount <= 0
			|| survivorMemberCount > acceptedMemberCount)
			return BuildRejected("exact enemy garrison rebuild delivery roster is invalid");
		HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
		if (!targetZone || targetZone.m_sOwnerFactionKey != order.m_sFactionKey
			|| targetZone.m_iOwnershipRevision != order.m_iTargetOwnershipRevision)
			return BuildRejected("exact enemy garrison rebuild delivery target ownership changed");
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
			return BuildRejected("exact enemy garrison rebuild delivery has not reached an open on-station roster");

		string settlementId = BuildSettlementId(order.m_sOperationId, settlementKind);
		string refundMutationId = "enemy_resource_refund_" + settlementId;
		bool tupleEmpty = order.m_sResourceSettlementId.IsEmpty()
			&& order.m_sResourceSettlementKind.IsEmpty()
			&& order.m_sResourceRefundMutationId.IsEmpty()
			&& order.m_iSettlementAcceptedMemberCount == 0
			&& order.m_iSettlementSurvivorMemberCount == 0
			&& order.m_iRefundedAttackResources == 0
			&& order.m_iRefundedSupportResources == 0;
		bool tupleExact = order.m_sResourceSettlementId == settlementId
			&& order.m_sResourceSettlementKind == settlementKind
			&& order.m_sResourceRefundMutationId == refundMutationId
			&& order.m_iSettlementAcceptedMemberCount == acceptedMemberCount
			&& order.m_iSettlementSurvivorMemberCount == survivorMemberCount
			&& order.m_iRefundedAttackResources == 0
			&& order.m_iRefundedSupportResources == 0;
		if (order.m_bResourceSettlementApplied)
		{
			if (tupleExact)
				return BuildAccepted(operation, false, true);
			return BuildRejected("exact enemy garrison rebuild delivery receipt already conflicts");
		}
		if (!tupleEmpty && !tupleExact)
			return BuildRejected("exact enemy garrison rebuild contains a partial delivery receipt");
		return BuildAccepted(operation, false, false);
	}

	HST_OperationTransitionResult RecordExactEnemyGarrisonRebuildResourceSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		string settlementKind,
		int acceptedMemberCount,
		int survivorMemberCount)
	{
		HST_OperationTransitionResult preflight = CanRecordExactEnemyGarrisonRebuildResourceSettlement(
			state,
			order,
			settlementKind,
			acceptedMemberCount,
			survivorMemberCount);
		if (!preflight || !preflight.m_bAccepted || !preflight.m_Operation || preflight.m_bAlreadyApplied)
			return preflight;
		if (preflight.m_Operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			order.m_sResourceSettlementId = BuildSettlementId(order.m_sOperationId, settlementKind);
			order.m_sResourceSettlementKind = settlementKind;
			order.m_iSettlementAcceptedMemberCount = acceptedMemberCount;
			order.m_iSettlementSurvivorMemberCount = survivorMemberCount;
			order.m_bResourceSettlementApplied = true;
			preflight.m_Operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
			preflight.m_Operation.m_iRevision++;
			return BuildAccepted(preflight.m_Operation, true, false);
		}
		string settlementId = BuildSettlementId(order.m_sOperationId, settlementKind);
		bool stagedDeliveryExact = settlementKind
				== EXACT_ENEMY_GARRISON_REBUILD_DELIVERY_SETTLEMENT_KIND
			&& order.m_sResourceSettlementId == settlementId
			&& order.m_sResourceSettlementKind == settlementKind
			&& order.m_sResourceRefundMutationId
				== "enemy_resource_refund_" + settlementId
			&& order.m_iSettlementAcceptedMemberCount == acceptedMemberCount
			&& order.m_iSettlementSurvivorMemberCount == survivorMemberCount
			&& order.m_iRefundedAttackResources == 0
			&& order.m_iRefundedSupportResources == 0;
		if (!stagedDeliveryExact)
			return BuildRejected("exact enemy garrison rebuild delivery receipt was not durably staged");
		order.m_bResourceSettlementApplied = true;
		preflight.m_Operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		preflight.m_Operation.m_iRevision++;
		return BuildAccepted(preflight.m_Operation, true, false);
	}

	HST_OperationTransitionResult CanSettleExactEnemyGarrisonRebuild(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild settlement received another order family");
		HST_OperationRecordState operation;
		if (state && order)
			operation = state.FindOperation(order.m_sOperationId);
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			return CanFinalizePreparedExactEnemyGarrisonRebuildSettlement(
				state,
				order,
				terminalResult,
				settlementId,
				operation.m_sTerminalReason);
		}
		return CanSettleExactEnemyDefensiveQRF(state, order, terminalResult, settlementId);
	}

	HST_OperationTransitionResult FinalizePreparedExactEnemyGarrisonRebuildSettlement(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		HST_OperationTransitionResult preflight
			= CanFinalizePreparedExactEnemyGarrisonRebuildSettlement(
				state,
				order,
				terminalResult,
				settlementId,
				reason);
		if (!preflight || !preflight.m_bAccepted || !preflight.m_Operation
			|| preflight.m_bAlreadyApplied)
			return preflight;
		HST_OperationRecordState operation = preflight.m_Operation;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		operation.m_iDutyStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iEngagementStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		ResetArrivalConfirmation(operation);
		operation.m_iRevision++;
		order.m_bPhysicalized = false;
		order.m_bAbstractResolved = false;
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult SettleExactEnemyGarrisonRebuild(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return BuildRejected("exact enemy garrison rebuild settlement received another order family");
		HST_OperationRecordState operation;
		if (state && order)
			operation = state.FindOperation(order.m_sOperationId);
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			return FinalizePreparedExactEnemyGarrisonRebuildSettlement(
				state,
				order,
				terminalResult,
				settlementId,
				reason);
		}
		return SettleExactEnemyDefensiveQRF(state, order, terminalResult, settlementId, reason);
	}

	HST_OperationTransitionResult LinkOutboundVirtual(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch)
	{
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (!group || !batch || batch.m_sOperationId != operation.m_sOperationId
			|| group.m_sOperationId != operation.m_sOperationId || group.m_sManifestId != operation.m_sManifestId
			|| batch.m_sManifestId != operation.m_sManifestId || group.m_sSpawnResultId != batch.m_sResultId
			|| group.m_sProjectionId != batch.m_sProjectionId || group.m_sForceId != batch.m_sForceId
			|| !batch.m_bStrategicProjectionHeld)
			return BuildRejected("exact player support virtual projection links conflict with operation authority");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return BuildRejected("settled exact player support operation cannot become virtual outbound");
		if ((operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING
			&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return BuildRejected("exact player support virtual outbound transition is illegal from the current operation state");
		if (!LinkMatchesOrEmpty(operation.m_sSpawnResultId, batch.m_sResultId)
			|| !LinkMatchesOrEmpty(operation.m_sForceId, batch.m_sForceId)
			|| !LinkMatchesOrEmpty(operation.m_sProjectionId, batch.m_sProjectionId)
			|| !LinkMatchesOrEmpty(operation.m_sGroupId, group.m_sGroupId))
			return BuildRejected("exact player support virtual outbound would replace an authoritative projection link");

		bool changed;
		changed = AssignString(operation.m_sSpawnResultId, batch.m_sResultId) || changed;
		changed = AssignString(operation.m_sForceId, batch.m_sForceId) || changed;
		changed = AssignString(operation.m_sProjectionId, batch.m_sProjectionId) || changed;
		changed = AssignString(operation.m_sGroupId, group.m_sGroupId) || changed;
		changed = AssignString(operation.m_sCurrentRouteId, group.m_sRouteId) || changed;
		changed = AssignVector(operation.m_vStrategicPosition, group.m_vPosition) || changed;
		changed = SetDuty(operation, HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND, state.m_iElapsedSeconds) || changed;
		changed = SetResumeDuty(operation, operation.m_eDutyState) || changed;
		changed = ResetArrivalConfirmation(operation) || changed;
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult MarkMaterializingFromVirtual(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch,
		string reason)
	{
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (!group || !batch || batch.m_bStrategicProjectionHeld
			|| operation.m_sSpawnResultId != batch.m_sResultId || operation.m_sGroupId != group.m_sGroupId
			|| group.m_sOperationId != operation.m_sOperationId || group.m_sProjectionId != operation.m_sProjectionId)
			return BuildRejected("exact player support materialization release conflicts with virtual authority");
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
			return BuildRejected("exact player support cannot materialize from the current projection state");
		bool changed = SetMaterialization(operation, HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING, HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC, state.m_iElapsedSeconds);
		changed = AssignVector(operation.m_vStrategicPosition, group.m_vPosition) || changed;
		changed = AssignString(operation.m_sLastProjectionReason, reason) || changed;
		operation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult UpdatePhysicalPosition(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ActiveGroupState group)
	{
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (!group || group.m_sGroupId != operation.m_sGroupId || group.m_sOperationId != operation.m_sOperationId)
			return BuildRejected("physical position sample conflicts with operation projection identity");
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
			return BuildRejected("physical position sample does not own live authority");
		bool changed = AssignVector(operation.m_vStrategicPosition, group.m_vPosition);
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult BeginDematerialization(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ActiveGroupState group,
		string reason)
	{
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (!group || group.m_sGroupId != operation.m_sGroupId || group.m_sOperationId != operation.m_sOperationId)
			return BuildRejected("dematerialization projection identity conflicts");
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
			return BuildRejected("exact player support cannot dematerialize from the current projection state");
		bool changed = AssignVector(operation.m_vStrategicPosition, group.m_vPosition);
		changed = SetMaterialization(operation, HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING, HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE, state.m_iElapsedSeconds) || changed;
		changed = AssignString(operation.m_sLastProjectionReason, reason) || changed;
		operation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult CompleteDematerialization(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch,
		string reason)
	{
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (!group || !batch || !batch.m_bStrategicProjectionHeld
			|| group.m_sGroupId != operation.m_sGroupId || batch.m_sResultId != operation.m_sSpawnResultId)
			return BuildRejected("dematerialization completion lacks held exact projection authority");
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
			return BuildRejected("exact player support dematerialization completion is out of order");
		bool changed = AssignVector(operation.m_vStrategicPosition, group.m_vPosition);
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		int strategicLiving = queue.CountStrategicLivingMemberSlots(batch);
		if (operation.m_iLastVirtualFriendlyCount != strategicLiving)
		{
			operation.m_iLastVirtualFriendlyCount = strategicLiving;
			changed = true;
		}
		changed = ApplyExactPlayerSupportReturnToAssignment(
			operation,
			group,
			state.m_iElapsedSeconds) || changed;
		changed = SetMaterialization(operation, HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL, HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC, state.m_iElapsedSeconds) || changed;
		changed = AssignString(operation.m_sLastProjectionReason, reason) || changed;
		operation.m_iStrategicLastUpdateSecond = state.m_iElapsedSeconds;
		operation.m_iVirtualCombatLastStepSecond = state.m_iElapsedSeconds;
		operation.m_sLastVirtualCombatReason = "physical interval excluded from virtual combat catch-up";
		operation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	bool ApplyExactPlayerSupportReturnToAssignment(
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		int nowSecond)
	{
		if (!operation || !group || !IsExactPlayerSupportOperationType(operation.m_eType)
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			|| Distance2D(group.m_vPosition, operation.m_vAssignmentPosition)
				<= EXACT_PLAYER_SUPPORT_ASSIGNMENT_RETURN_RADIUS_METERS)
			return false;

		bool changed = SetDuty(
			operation,
			HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ASSIGNMENT,
			nowSecond);
		changed = SetResumeDuty(
			operation,
			HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ASSIGNMENT) || changed;
		changed = AssignString(operation.m_sTacticalTargetZoneId, operation.m_sAssignmentZoneId) || changed;
		changed = AssignVector(operation.m_vTacticalTargetPosition, operation.m_vAssignmentPosition) || changed;
		changed = AssignVector(operation.m_vRouteEndPosition, operation.m_vAssignmentPosition) || changed;
		HST_StrategicMovementService movement = new HST_StrategicMovementService();
		changed = movement.SyncRouteProgressFromPosition(operation, group.m_vPosition) || changed;
		changed = AssignString(operation.m_sCurrentRouteId, operation.m_sOperationId + "_assignment_return") || changed;
		changed = AssignString(group.m_sRouteId, operation.m_sCurrentRouteId) || changed;
		changed = AssignVector(group.m_vSourcePosition, group.m_vPosition) || changed;
		changed = AssignVector(group.m_vTargetPosition, operation.m_vAssignmentPosition) || changed;
		return changed;
	}

	HST_OperationTransitionResult MarkVirtualOnStation(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ActiveGroupState group)
	{
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (!group || group.m_sGroupId != operation.m_sGroupId || group.m_sOperationId != operation.m_sOperationId
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return BuildRejected("exact player support virtual arrival conflicts with strategic authority");
		if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ASSIGNMENT
			&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
			return BuildRejected("exact player support virtual arrival transition is illegal from the current duty state");
		bool changed = SetDuty(operation, HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION, state.m_iElapsedSeconds);
		changed = AssignVector(operation.m_vStrategicPosition, operation.m_vRouteEndPosition) || changed;
		changed = AssignVector(group.m_vPosition, operation.m_vStrategicPosition) || changed;
		changed = SetResumeDuty(operation, operation.m_eDutyState) || changed;
		operation.m_iVirtualCombatLastStepSecond = state.m_iElapsedSeconds;
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult RemoveUncommittedExactPlayerQRF(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		if (!request || request.m_eType != HST_ESupportRequestType.HST_SUPPORT_QRF)
			return BuildRejected("exact player support rollback received another support family");
		return RemoveUncommittedExactPlayerSupport(state, request, quote, manifest);
	}

	HST_OperationTransitionResult RemoveUncommittedExactPlayerSupport(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		if (!RequiresOperation(request))
			return BuildAccepted(null, false, true);
		if (!state || !request)
			return BuildRejected("exact player support operation rollback context is missing");
		HST_OperationRecordState operation = state.FindOperation(request.m_sOperationId);
		if (!operation)
			return BuildAccepted(null, false, true);
		string failure = ValidateExactPlayerSupport(state, operation, request, quote, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| !operation.m_sSpawnResultId.IsEmpty() || !operation.m_sGroupId.IsEmpty())
			return BuildRejected("exact player support operation has execution or settlement authority and cannot be rolled back");
		int index = state.m_aOperations.Find(operation);
		if (index < 0)
			return BuildRejected("exact player support operation rollback identity disappeared");
		state.m_aOperations.Remove(index);
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult MarkOutboundMaterializing(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch)
	{
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (!group || !batch || batch.m_sOperationId != operation.m_sOperationId
			|| group.m_sOperationId != operation.m_sOperationId || group.m_sManifestId != operation.m_sManifestId
			|| batch.m_sManifestId != operation.m_sManifestId || group.m_sSpawnResultId != batch.m_sResultId
			|| group.m_sProjectionId != batch.m_sProjectionId || group.m_sForceId != batch.m_sForceId)
			return BuildRejected("exact player support materialization links conflict with operation authority");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return BuildRejected("settled exact player support operation cannot materialize");
		if ((operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING
			&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
			|| (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return BuildRejected("exact player support outbound materialization transition is illegal from the current operation state");
		if (!LinkMatchesOrEmpty(operation.m_sSpawnResultId, batch.m_sResultId)
			|| !LinkMatchesOrEmpty(operation.m_sForceId, batch.m_sForceId)
			|| !LinkMatchesOrEmpty(operation.m_sProjectionId, batch.m_sProjectionId)
			|| !LinkMatchesOrEmpty(operation.m_sGroupId, group.m_sGroupId))
			return BuildRejected("exact player support outbound materialization would replace an authoritative projection link");

		bool changed;
		changed = AssignString(operation.m_sSpawnResultId, batch.m_sResultId) || changed;
		changed = AssignString(operation.m_sForceId, batch.m_sForceId) || changed;
		changed = AssignString(operation.m_sProjectionId, batch.m_sProjectionId) || changed;
		changed = AssignString(operation.m_sGroupId, group.m_sGroupId) || changed;
		changed = AssignString(operation.m_sCurrentRouteId, group.m_sRouteId) || changed;
		changed = AssignVector(operation.m_vStrategicPosition, group.m_vPosition) || changed;
		changed = SetDuty(operation, HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND, state.m_iElapsedSeconds) || changed;
		changed = SetMaterialization(operation, HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING, HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC, state.m_iElapsedSeconds) || changed;
		changed = SetResumeDuty(operation, operation.m_eDutyState) || changed;
		changed = ResetArrivalConfirmation(operation) || changed;
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult MarkPhysical(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch)
	{
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (!group || !batch || batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_sOperationId != operation.m_sOperationId || group.m_sOperationId != operation.m_sOperationId
			|| batch.m_sManifestId != operation.m_sManifestId || group.m_sManifestId != operation.m_sManifestId
			|| group.m_sSpawnResultId != batch.m_sResultId || group.m_sProjectionId != batch.m_sProjectionId
			|| group.m_sForceId != batch.m_sForceId)
			return BuildRejected("exact player support physical handoff conflicts with operation authority");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return BuildRejected("settled exact player support operation cannot become physical");
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			return BuildRejected("exact player support physical handoff transition is illegal from the current materialization state");
		if (!LinkMatchesOrEmpty(operation.m_sSpawnResultId, batch.m_sResultId)
			|| !LinkMatchesOrEmpty(operation.m_sForceId, batch.m_sForceId)
			|| !LinkMatchesOrEmpty(operation.m_sProjectionId, batch.m_sProjectionId)
			|| !LinkMatchesOrEmpty(operation.m_sGroupId, group.m_sGroupId))
			return BuildRejected("exact player support physical handoff would replace an authoritative projection link");

		bool completingPhysicalHandoff = operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		bool changed;
		changed = AssignString(operation.m_sSpawnResultId, batch.m_sResultId) || changed;
		changed = AssignString(operation.m_sForceId, batch.m_sForceId) || changed;
		changed = AssignString(operation.m_sProjectionId, batch.m_sProjectionId) || changed;
		changed = AssignString(operation.m_sGroupId, group.m_sGroupId) || changed;
		changed = AssignString(operation.m_sCurrentRouteId, group.m_sRouteId) || changed;
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING)
			changed = SetDuty(operation, HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND, state.m_iElapsedSeconds) || changed;
		if (completingPhysicalHandoff)
		{
			HST_OperationTransitionResult engagementHandoff = NormalizeAbstractEngagementForPhysicalHandoff(state, operation);
			if (!engagementHandoff || !engagementHandoff.m_bAccepted)
				return BuildRejected("exact player support abstract engagement could not hand off to physical authority");
			changed = engagementHandoff.m_bStateChanged || changed;
			operation.m_sLastVirtualCombatReason = "abstract engagement handed off clear to physical projection";
		}
		if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
			changed = SetResumeDuty(operation, operation.m_eDutyState) || changed;
		changed = SetMaterialization(operation, HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL, HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE, state.m_iElapsedSeconds) || changed;
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	protected HST_OperationTransitionResult NormalizeAbstractEngagementForPhysicalHandoff(
		HST_CampaignState state,
		HST_OperationRecordState operation)
	{
		if (!state || !operation)
			return BuildRejected("physical engagement handoff context is missing");
		bool changed;
		HST_OperationTransitionResult transition;
		if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT)
		{
			transition = RecordEngagement(state, operation.m_sOperationId, HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED);
			if (!transition || !transition.m_bAccepted)
				return BuildRejected("contact could not advance during physical engagement handoff");
			changed = transition.m_bStateChanged || changed;
		}
		if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED)
		{
			transition = RecordEngagement(state, operation.m_sOperationId, HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_DISENGAGING);
			if (!transition || !transition.m_bAccepted)
				return BuildRejected("engaged contact could not disengage during physical handoff");
			changed = transition.m_bStateChanged || changed;
		}
		if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_DISENGAGING)
		{
			transition = RecordEngagement(state, operation.m_sOperationId, HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR);
			if (!transition || !transition.m_bAccepted)
				return BuildRejected("disengaging contact could not clear during physical handoff");
			changed = transition.m_bStateChanged || changed;
		}
		if (operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
			return BuildRejected("physical handoff does not support the current abstract engagement mode");
		return BuildAccepted(operation, changed, !changed);
	}

	HST_OperationTransitionResult MarkOnStation(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ActiveGroupState group)
	{
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (!group || group.m_sOperationId != operation.m_sOperationId || group.m_sGroupId != operation.m_sGroupId
			|| group.m_sManifestId != operation.m_sManifestId || group.m_sSpawnResultId != operation.m_sSpawnResultId
			|| group.m_sForceId != operation.m_sForceId || group.m_sProjectionId != operation.m_sProjectionId
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			return BuildRejected("exact player support arrival conflicts with physical operation authority");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return BuildRejected("settled exact player support operation cannot arrive on station");
		if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ASSIGNMENT
			&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
			return BuildRejected("exact player support arrival transition is illegal from the current duty state");

		bool changed = SetDuty(operation, HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION, state.m_iElapsedSeconds);
		changed = AssignString(operation.m_sTacticalTargetZoneId, operation.m_sAssignmentZoneId) || changed;
		changed = AssignVector(operation.m_vTacticalTargetPosition, operation.m_vAssignmentPosition) || changed;
		changed = SetResumeDuty(operation, operation.m_eDutyState) || changed;
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult MarkRestoreMaterializing(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ActiveGroupState group)
	{
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (!group || group.m_sOperationId != operation.m_sOperationId || group.m_sGroupId != operation.m_sGroupId
			|| group.m_sManifestId != operation.m_sManifestId || group.m_sSpawnResultId != operation.m_sSpawnResultId
			|| group.m_sForceId != operation.m_sForceId || group.m_sProjectionId != operation.m_sProjectionId)
			return BuildRejected("exact player support restore projection conflicts with operation authority");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return BuildRejected("settled exact player support operation cannot reproject");
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return BuildRejected("exact player support restore projection is not in persisted materialization authority");
		bool changed = SetMaterialization(operation, HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING, HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC, state.m_iElapsedSeconds);
		changed = AssignVector(operation.m_vStrategicPosition, group.m_vPosition) || changed;
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult BeginRecall(
		HST_CampaignState state,
		HST_SupportRequestState request,
		vector recallTarget)
	{
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return BuildRejected("settled exact player support operation cannot be recalled");
		if (!IsActiveDuty(operation.m_eDutyState))
			return BuildRejected("exact player support operation cannot be recalled from the current duty state");
		bool alreadyRecall = operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RECALL_REQUESTED
			|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_EXITING
			|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN;
		bool changed;
		if (!alreadyRecall)
		{
			operation.m_eResumeDutyState = operation.m_eDutyState;
			changed = SetDuty(operation, HST_EOperationDutyState.HST_OPERATION_DUTY_RECALL_REQUESTED, state.m_iElapsedSeconds);
		}
		if (IsZeroVector(recallTarget))
			recallTarget = operation.m_vOriginPosition;
		changed = AssignString(operation.m_sTacticalTargetZoneId, operation.m_sOriginZoneId) || changed;
		changed = AssignVector(operation.m_vTacticalTargetPosition, recallTarget) || changed;
		changed = ResetArrivalConfirmation(operation) || changed;
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
	}

	HST_OperationTransitionResult CanBeginRecall(HST_CampaignState state, HST_SupportRequestState request)
	{
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED)
			return BuildRejected("settled exact player support operation cannot be recalled");
		if (!IsActiveDuty(operation.m_eDutyState))
			return BuildRejected("exact player support operation cannot be recalled from the current duty state");
		return BuildAccepted(operation, false, operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RECALL_REQUESTED
			|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_EXITING
			|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN);
	}

	HST_OperationTransitionResult MarkRecallExiting(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ActiveGroupState group,
		vector exitPosition)
	{
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (!group || group.m_sOperationId != operation.m_sOperationId || group.m_sGroupId != operation.m_sGroupId
			|| group.m_sManifestId != operation.m_sManifestId || group.m_sSpawnResultId != operation.m_sSpawnResultId
			|| group.m_sForceId != operation.m_sForceId || group.m_sProjectionId != operation.m_sProjectionId)
			return BuildRejected("exact player support recall route conflicts with operation authority");

		HST_OperationTransitionResult begun = BeginRecall(state, request, exitPosition);
		if (!begun.m_bAccepted || !begun.m_Operation)
			return begun;
		operation = begun.m_Operation;
		bool changed = begun.m_bStateChanged;
		changed = SetDuty(operation, HST_EOperationDutyState.HST_OPERATION_DUTY_EXITING, state.m_iElapsedSeconds) || changed;
		changed = AssignString(operation.m_sCurrentRouteId, group.m_sRouteId) || changed;
		return FinishTransition(operation, changed, state.m_iElapsedSeconds, begun.m_bStateChanged);
	}

	HST_OperationTransitionResult RecordEngagement(
		HST_CampaignState state,
		string operationId,
		HST_EOperationEngagementMode nextMode)
	{
		if (!state || operationId.IsEmpty())
			return BuildRejected("operation engagement context is missing");
		HST_OperationRecordState operation = state.FindOperation(operationId);
		int expectedContractVersion;
		if (operation)
			expectedContractVersion = ResolveExactPlayerSupportOperationContractVersion(operation.m_eType);
		if (!operation || expectedContractVersion == 0
			|| operation.m_iContractVersion != expectedContractVersion)
			return BuildRejected("exact player support operation is missing for engagement transition");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return BuildRejected("settled operation cannot change engagement mode");
		if (!IsActiveDuty(operation.m_eDutyState))
			return BuildRejected("exact player support operation duty state cannot accept engagement transitions");
		if (operation.m_eEngagementMode == nextMode)
			return BuildAccepted(operation, false, true);
		if (!IsLegalEngagementTransition(operation.m_eEngagementMode, nextMode))
			return BuildRejected("illegal exact player support engagement transition");
		if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
			operation.m_eResumeDutyState = operation.m_eDutyState;
		operation.m_eEngagementMode = nextMode;
		operation.m_iEngagementStateEnteredAtSecond = state.m_iElapsedSeconds;
		if (nextMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT
			|| nextMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED)
			operation.m_iLastContactAtSecond = state.m_iElapsedSeconds;
		operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		operation.m_iRevision++;
		return BuildAccepted(operation, true, false);
	}

	HST_OperationTransitionResult CanSettleExactPlayerQRF(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_EOperationTerminalResult terminalResult,
		string settlementId)
	{
		if (!request || request.m_eType != HST_ESupportRequestType.HST_SUPPORT_QRF)
			return BuildRejected("exact player support settlement received another support family");
		return CanSettleExactPlayerSupport(state, request, terminalResult, settlementId);
	}

	HST_OperationTransitionResult CanSettleExactPlayerSupport(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_EOperationTerminalResult terminalResult,
		string settlementId)
	{
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN
			|| terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED || settlementId.IsEmpty())
			return BuildRejected("exact player support terminal result or settlement identity is invalid");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (operation.m_eTerminalResult == terminalResult && operation.m_sSettlementId == settlementId)
				return BuildAccepted(operation, false, true);
			return BuildRejected("exact player support operation is already settled with a conflicting result");
		}
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
			return BuildRejected("exact player support operation settlement state conflicts");
		return BuildAccepted(operation, false, false);
	}

	HST_OperationTransitionResult SettleExactPlayerQRF(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		if (!request || request.m_eType != HST_ESupportRequestType.HST_SUPPORT_QRF)
			return BuildRejected("exact player support settlement received another support family");
		return SettleExactPlayerSupport(state, request, terminalResult, settlementId, reason);
	}

	HST_OperationTransitionResult SettleExactPlayerSupport(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		HST_OperationTransitionResult preflight = CanSettleExactPlayerSupport(state, request, terminalResult, settlementId);
		if (!preflight.m_bAccepted || !preflight.m_Operation || preflight.m_bAlreadyApplied)
			return preflight;
		HST_OperationRecordState operation = preflight.m_Operation;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		operation.m_eTerminalResult = terminalResult;
		operation.m_sSettlementId = settlementId;
		operation.m_sTerminalReason = reason;
		operation.m_iDutyStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iEngagementStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		operation.m_iSettledAtSecond = state.m_iElapsedSeconds;
		operation.m_iRevision++;
		return BuildAccepted(operation, true, false);
	}

	string ValidateExactPlayerQRF(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		if (!request || request.m_eType != HST_ESupportRequestType.HST_SUPPORT_QRF)
			return "exact player support validation received another support family";
		return ValidateExactPlayerSupport(state, operation, request, quote, manifest);
	}

	string ValidateExactPlayerSearchDestroy(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		if (!request || request.m_eType != HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return "exact player search-and-destroy validation received another support family";
		return ValidateExactPlayerSupport(state, operation, request, quote, manifest);
	}

	string ValidateExactPlayerSupport(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		if (!state || !operation || !request || !quote || !manifest)
			return "exact player support operation authority is incomplete";
		if (!IsExactPlayerSupportType(request.m_eType) || quote.m_eSupportType != request.m_eType)
			return "exact player support family identity conflicts";
		string expectedQuoteKind = HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_QRF;
		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			expectedQuoteKind = HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_SEARCH_DESTROY;
		if (quote.m_sQuoteKind != expectedQuoteKind)
			return "exact player support quote family identity conflicts";
		int expectedContractVersion = ResolveExactPlayerSupportContractVersion(request.m_eType);
		HST_EOperationType expectedOperationType = ResolveExactPlayerSupportOperationType(request.m_eType);
		string expectedAssignmentKind = ResolveExactPlayerSupportAssignmentKind(request.m_eType);
		string expectedRecallPolicy = ResolveExactPlayerSupportRecallPolicy(request.m_eType);
		string expectedSettlementPolicy = ResolveExactPlayerSupportSettlementPolicy(request.m_eType);
		if (CountOperationId(state, operation.m_sOperationId) != 1)
			return "exact player support operation identity is ambiguous";
		if (operation.m_eType != expectedOperationType
			|| operation.m_iContractVersion != expectedContractVersion
			|| request.m_iOperationContractVersion != expectedContractVersion
			|| operation.m_iRevision <= 0)
			return "exact player support operation contract version conflicts";
		if (operation.m_sOperationId != request.m_sOperationId || operation.m_sOperationId != quote.m_sOperationId
			|| operation.m_sOperationId != manifest.m_sOperationId || operation.m_sSupportRequestId != request.m_sRequestId
			|| operation.m_sQuoteId != quote.m_sQuoteId || operation.m_sManifestId != manifest.m_sManifestId)
			return "exact player support operation aggregate links conflict";
		if (operation.m_sOwnerFactionKey != quote.m_sFactionKey || operation.m_sActorIdentityId != quote.m_sActorIdentityId
			|| operation.m_sIssueRequestId != quote.m_sCommandRequestId
			|| operation.m_sConfirmationRequestId != quote.m_sConfirmationRequestId)
			return "exact player support operation owner or command identity conflicts";
		if (operation.m_sOriginZoneId != quote.m_sSourceZoneId || operation.m_vOriginPosition != quote.m_vSourcePosition
			|| operation.m_sAssignmentKind != expectedAssignmentKind
			|| operation.m_sAssignmentZoneId != quote.m_sTargetZoneId || operation.m_vAssignmentPosition != quote.m_vTargetPosition)
			return "exact player support immutable origin or assignment conflicts";
		if (operation.m_sRecallPolicyId != expectedRecallPolicy
			|| operation.m_sSettlementPolicyId != expectedSettlementPolicy
			|| operation.m_iDeterministicSeed != manifest.m_iDeterministicSeed)
			return "exact player support operation policy or deterministic seed conflicts";
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_UNKNOWN
			|| operation.m_eResumeDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_UNKNOWN
			|| operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_UNKNOWN
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_UNKNOWN
			|| operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_UNKNOWN
			|| operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_UNKNOWN)
			return "exact player support operation contains unknown state authority";
		bool hasExecutionLink = !operation.m_sSpawnResultId.IsEmpty() || !operation.m_sForceId.IsEmpty()
			|| !operation.m_sProjectionId.IsEmpty() || !operation.m_sGroupId.IsEmpty();
		if (hasExecutionLink && (operation.m_sSpawnResultId.IsEmpty() || operation.m_sForceId.IsEmpty()
			|| operation.m_sProjectionId.IsEmpty() || operation.m_sGroupId.IsEmpty()))
			return "exact player support operation execution links are incomplete";
		if (hasExecutionLink && (operation.m_sSpawnResultId != request.m_sSpawnResultId
			|| operation.m_sForceId != "force_" + operation.m_sOperationId
			|| operation.m_sProjectionId != "projection_" + operation.m_sOperationId
			|| operation.m_sGroupId != operation.m_sProjectionId))
			return "exact player support operation execution links conflict";
		if ((operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			&& !hasExecutionLink)
			return "exact player support operation materialization lacks execution authority";
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			&& operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
			return "physical exact player support operation does not own live position authority";
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING
			&& operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
			return "dematerializing exact player support operation does not retain live position authority";
		if ((operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
			&& operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return "nonphysical exact player support operation does not own strategic position authority";
		if (operation.m_iProjectionContractVersion
				!= HST_StrategicMovementService.EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION
			|| operation.m_iRouteVersion != HST_StrategicMovementService.DIRECT_ROUTE_VERSION
			|| operation.m_fRouteTotalDistanceMeters < 0 || operation.m_fRouteProgressMeters < 0
			|| operation.m_fRouteProgressMeters > operation.m_fRouteTotalDistanceMeters + 1.0
			|| operation.m_fStrategicSpeedMetersPerSecond <= 0)
			return "exact player support strategic projection contract conflicts";
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN
				|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
				|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
				|| operation.m_sSettlementId.IsEmpty() || operation.m_iSettledAtSecond < 0
				|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
				|| operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
				|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
				|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
				return "settled exact player support operation lacks terminal authority";
		}
		else if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| !operation.m_sSettlementId.IsEmpty())
			return "open exact player support operation contains terminal authority";
		return "";
	}

	string ValidateExactEnemyDefensiveQRF(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!state || !operation || !order || !manifest)
			return "exact enemy defensive QRF operation authority is incomplete";
		if (!RequiresExactEnemyDirectedResponse(order))
			return "enemy order does not opt into an exact directed-response contract";
		if (CountOperationId(state, operation.m_sOperationId) != 1)
			return "exact enemy defensive QRF operation identity is ambiguous";
		if (CountEnemyOperationsByAnyIdentity(state, operation.m_sOperationId, order.m_sOrderId, manifest.m_sManifestId) != 1)
			return "exact enemy defensive QRF operation ownership backlinks are ambiguous";
		if (CountEnemyOrdersByAnyIdentity(state, order) != 1)
			return "exact enemy defensive QRF enemy-order identity is ambiguous";
		if (CountEnemyForceManifestsByAnyIdentity(state, manifest.m_sManifestId, operation.m_sOperationId) != 1)
			return "exact enemy defensive QRF manifest identity is ambiguous";
		if (operation.m_eType != ResolveExactEnemyDirectedOperationType(order)
			|| operation.m_iContractVersion != ResolveExactEnemyDirectedContractVersion(order)
			|| order.m_iOperationContractVersion != ResolveExactEnemyDirectedContractVersion(order)
			|| operation.m_iRevision <= 0)
			return "exact enemy defensive QRF operation contract conflicts";
		if (operation.m_sOperationId != order.m_sOperationId
			|| operation.m_sOperationId != HST_StableIdService.BuildOperationId("enemy_order", order.m_sOrderId)
			|| operation.m_sEnemyOrderId != order.m_sOrderId
			|| operation.m_sManifestId != order.m_sManifestId || operation.m_sManifestId != manifest.m_sManifestId
			|| manifest.m_sOperationId != operation.m_sOperationId)
			return "exact enemy defensive QRF aggregate backlinks conflict";
		if (!operation.m_sSupportRequestId.IsEmpty() || !operation.m_sQuoteId.IsEmpty()
			|| !order.m_sSupportRequestId.IsEmpty() || !manifest.m_sQuoteId.IsEmpty())
			return "exact enemy defensive QRF conflicts with the legacy support consumer";
		if (operation.m_sOwnerFactionKey != order.m_sFactionKey || manifest.m_sFactionKey != order.m_sFactionKey)
			return "exact enemy defensive QRF immutable source, target, or owner conflicts";
		if (RequiresExactEnemyGarrisonRebuild(order) && order.m_iTargetOwnershipRevision <= 0)
			return "exact enemy garrison rebuild frozen target ownership revision is invalid";
		if (operation.m_sOriginZoneId != order.m_sSourceZoneId || manifest.m_sSourceZoneId != order.m_sSourceZoneId)
			return "exact enemy defensive QRF immutable source, target, or owner conflicts";
		if (operation.m_sAssignmentZoneId != order.m_sTargetZoneId || manifest.m_sTargetZoneId != order.m_sTargetZoneId)
			return "exact enemy defensive QRF immutable source, target, or owner conflicts";
		if (operation.m_vOriginPosition != order.m_vSourcePosition
			|| operation.m_vAssignmentPosition != order.m_vTargetPosition)
			return "exact enemy defensive QRF immutable source, target, or owner conflicts";
		if (operation.m_sAssignmentKind != ResolveExactEnemyDirectedAssignmentKind(order))
			return "exact enemy defensive QRF immutable source, target, or owner conflicts";
		if (operation.m_sRecallPolicyId != ResolveExactEnemyDirectedRecallPolicy(order)
			|| operation.m_sSettlementPolicyId != ResolveExactEnemyDirectedSettlementPolicy(order)
			|| operation.m_iDeterministicSeed != manifest.m_iDeterministicSeed)
			return "exact enemy defensive QRF policy or deterministic seed conflicts";
		string manifestFailure = ValidateExactEnemyDefensiveQRFManifest(order, manifest);
		if (!manifestFailure.IsEmpty())
			return manifestFailure;
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_UNKNOWN
			|| operation.m_eResumeDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_UNKNOWN
			|| operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_UNKNOWN
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_UNKNOWN
			|| operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_UNKNOWN
			|| operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_UNKNOWN)
			return "exact enemy defensive QRF contains unknown state authority";
		if (operation.m_iProjectionContractVersion != HST_StrategicMovementService.EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION
			|| operation.m_iRouteVersion != HST_StrategicMovementService.DIRECT_ROUTE_VERSION
			|| operation.m_fRouteTotalDistanceMeters < 0 || operation.m_fRouteProgressMeters < 0
			|| operation.m_fRouteProgressMeters > operation.m_fRouteTotalDistanceMeters + 1.0
			|| operation.m_fStrategicSpeedMetersPerSecond != HST_StrategicMovementService.EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND)
			return "exact enemy defensive QRF strategic route contract conflicts";
		if (operation.m_iLastVirtualFriendlyCount < 0
			|| operation.m_iLastVirtualFriendlyCount > manifest.m_iAcceptedMemberCount
			|| operation.m_iArrivalConfirmationCount < 0 || operation.m_iArrivalConfirmationCount > 2
			|| (operation.m_iArrivalConfirmationCount > 0 && operation.m_iLastArrivalConfirmationSecond < 0))
			return "exact enemy defensive QRF roster or arrival confirmation authority conflicts";

		bool hasExecutionLink = !operation.m_sSpawnResultId.IsEmpty() || !operation.m_sForceId.IsEmpty()
			|| !operation.m_sProjectionId.IsEmpty() || !operation.m_sGroupId.IsEmpty();
		if (order.m_bResourceRefundApplied)
			return "exact enemy defensive QRF cannot use the legacy refund flag";
		if (hasExecutionLink != order.m_bStrategicServiceCommitted)
			return "exact enemy defensive QRF strategic service commit authority conflicts";
		if (hasExecutionLink && (operation.m_sSpawnResultId.IsEmpty() || operation.m_sForceId.IsEmpty()
			|| operation.m_sProjectionId.IsEmpty() || operation.m_sGroupId.IsEmpty()))
			return "exact enemy defensive QRF execution links are incomplete";
		if (hasExecutionLink && (operation.m_sSpawnResultId != order.m_sSpawnResultId
			|| operation.m_sGroupId != order.m_sGroupId
			|| operation.m_sForceId != "force_" + operation.m_sOperationId
			|| operation.m_sProjectionId != "projection_" + operation.m_sOperationId
			|| operation.m_sGroupId != operation.m_sProjectionId))
			return "exact enemy defensive QRF execution backlinks conflict";
		if (!hasExecutionLink && (!order.m_sSpawnResultId.IsEmpty() || !order.m_sGroupId.IsEmpty()))
			return "exact enemy defensive QRF order contains unowned execution backlinks";
		bool hasSettlementIntent = !order.m_sResourceSettlementId.IsEmpty()
			|| !order.m_sResourceSettlementKind.IsEmpty()
			|| !order.m_sResourceRefundMutationId.IsEmpty()
			|| order.m_iSettlementAcceptedMemberCount != 0
			|| order.m_iSettlementSurvivorMemberCount != 0
			|| order.m_iRefundedAttackResources != 0
			|| order.m_iRefundedSupportResources != 0;
		if (order.m_bResourceSettlementApplied || hasSettlementIntent)
		{
			if (order.m_sResourceSettlementId.IsEmpty() || order.m_sResourceSettlementKind.IsEmpty()
				|| order.m_sResourceSettlementId != BuildSettlementId(operation.m_sOperationId, order.m_sResourceSettlementKind)
				|| order.m_iSettlementAcceptedMemberCount != manifest.m_iAcceptedMemberCount
				|| order.m_iSettlementSurvivorMemberCount < 0
				|| order.m_iSettlementSurvivorMemberCount > order.m_iSettlementAcceptedMemberCount)
				return "exact enemy defensive QRF resource settlement authority conflicts";
			int expectedAttackRefund = Math.Max(0, order.m_iAttackCost) * order.m_iSettlementSurvivorMemberCount / order.m_iSettlementAcceptedMemberCount;
			int expectedSupportRefund = Math.Max(0, order.m_iSupportCost) * order.m_iSettlementSurvivorMemberCount / order.m_iSettlementAcceptedMemberCount;
			if (RequiresExactEnemyGarrisonRebuild(order)
				&& order.m_sResourceSettlementKind == EXACT_ENEMY_GARRISON_REBUILD_DELIVERY_SETTLEMENT_KIND)
			{
				expectedAttackRefund = 0;
				expectedSupportRefund = 0;
			}
			if (order.m_sResourceSettlementKind.Contains("_full"))
			{
				expectedAttackRefund = Math.Max(0, order.m_iAttackCost);
				expectedSupportRefund = Math.Max(0, order.m_iSupportCost);
			}
			if (order.m_iRefundedAttackResources != expectedAttackRefund
				|| order.m_iRefundedSupportResources != expectedSupportRefund)
				return "exact enemy defensive QRF resource refund amounts conflict with its survivor receipt";
			if (!order.m_bResourceSettlementApplied)
			{
				bool exactDirectedPrepared = (RequiresExactEnemyDefensiveQRF(order)
					|| RequiresExactEnemyCounterattack(order)
					|| RequiresExactEnemyGarrisonRebuild(order))
					&& operation.m_eSettlementState
						== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
					&& operation.m_sSettlementId == order.m_sResourceSettlementId
					&& order.m_sResourceRefundMutationId
						== "enemy_resource_refund_" + order.m_sResourceSettlementId;
				// Delivery is a durable zero-refund receipt while the operation remains
				// OPEN and on station. Its staged tuple is the crash-resume token between
				// intent, zero-delta mutation, and applied receipt.
				bool exactRebuildDeliveryPrepared = RequiresExactEnemyGarrisonRebuild(order)
					&& operation.m_eSettlementState
						== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
					&& operation.m_eTerminalResult
						== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
					&& operation.m_eDutyState
						== HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
					&& order.m_sResourceSettlementKind
						== EXACT_ENEMY_GARRISON_REBUILD_DELIVERY_SETTLEMENT_KIND
					&& order.m_sResourceRefundMutationId
						== "enemy_resource_refund_" + order.m_sResourceSettlementId;
				if (!exactDirectedPrepared && !exactRebuildDeliveryPrepared)
					return "unsettled exact enemy defensive QRF contains partial resource settlement authority";
			}
		}
		if ((operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
			&& !hasExecutionLink)
			return "exact enemy defensive QRF materialization lacks execution authority";
		if ((operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
			&& operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
			return "exact enemy defensive QRF live projection lacks live position authority";
		if ((operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
			&& operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return "exact enemy defensive QRF nonphysical projection lacks strategic position authority";
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (!IsEnemyDefensiveQRFTerminalResult(operation.m_eTerminalResult)
				|| !order.m_bResourceSettlementApplied
				|| operation.m_sSettlementId.IsEmpty() || operation.m_sSettlementId != order.m_sResourceSettlementId
				|| operation.m_iSettledAtSecond < 0
				|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
				|| operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
				|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
				|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
				return "settled exact enemy defensive QRF lacks terminal authority";
		}
		else if (operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
		{
			if ((!RequiresExactEnemyDefensiveQRF(order)
					&& !RequiresExactEnemyCounterattack(order)
					&& !RequiresExactEnemyGarrisonRebuild(order))
				|| !IsEnemyDefensiveQRFTerminalResult(operation.m_eTerminalResult)
				|| operation.m_sSettlementId.IsEmpty()
				|| operation.m_sTerminalReason.IsEmpty()
				|| operation.m_iSettledAtSecond <= 0
				|| operation.m_iSettledAtSecond > state.m_iElapsedSeconds
				|| !hasSettlementIntent
				|| operation.m_sSettlementId != order.m_sResourceSettlementId
				|| (RequiresExactEnemyDefensiveQRF(order)
					&& order.m_sResourceRefundMutationId
						!= "enemy_resource_refund_" + order.m_sResourceSettlementId)
				|| operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
				|| operation.m_eMaterializationState
					== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
				return "prepared exact enemy directed response lacks terminal intent authority";
		}
		else if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| !operation.m_sSettlementId.IsEmpty())
			return "open exact enemy defensive QRF contains terminal authority";
		return "";
	}

	string ValidateExactEnemyCounterattack(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!RequiresExactEnemyCounterattack(order))
			return "enemy order does not opt into the exact counterattack contract";
		return ValidateExactEnemyDefensiveQRF(state, operation, order, manifest);
	}

	string ValidateExactEnemyGarrisonRebuild(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!RequiresExactEnemyGarrisonRebuild(order))
			return "enemy order does not opt into the exact garrison rebuild contract";
		return ValidateExactEnemyDefensiveQRF(state, operation, order, manifest);
	}

	bool RemoveArchivedOperation(HST_CampaignState state, HST_OperationRecordState operation)
	{
		if (!state || !operation || operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return false;
		int index = state.m_aOperations.Find(operation);
		if (index < 0)
			return false;
		state.m_aOperations.Remove(index);
		return true;
	}

	protected string ValidateRegistrationIdentity(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		if (!state || !request || !quote || !manifest)
			return "exact player support operation registration context is missing";
		if (!IsExactPlayerSupportType(request.m_eType)
			|| request.m_iOperationContractVersion != ResolveExactPlayerSupportContractVersion(request.m_eType))
			return "exact player support request does not opt into its typed operation contract";
		string expectedQuoteKind = HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_QRF;
		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			expectedQuoteKind = HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_SEARCH_DESTROY;
		if (!request.m_bPlayerRequested || quote.m_sQuoteKind != expectedQuoteKind
			|| quote.m_eSupportType != request.m_eType)
			return "operation registration is not an exact paid player support family";
		if (request.m_sOperationId.IsEmpty() || request.m_sOperationId != HST_StableIdService.BuildOperationId("support", request.m_sRequestId)
			|| request.m_sOperationId != quote.m_sOperationId || request.m_sOperationId != manifest.m_sOperationId)
			return "exact player support operation identity conflicts";
		if (request.m_sRequestId != quote.m_sSupportRequestId || request.m_sQuoteId != quote.m_sQuoteId
			|| request.m_sManifestId != quote.m_sManifestId || request.m_sManifestId != manifest.m_sManifestId
			|| request.m_sCommandRequestId != quote.m_sConfirmationRequestId)
			return "exact player support request, quote, or manifest links conflict";
		if (quote.m_sActorIdentityId.IsEmpty() || quote.m_sCommandRequestId.IsEmpty() || quote.m_sConfirmationRequestId.IsEmpty()
			|| quote.m_sFactionKey.IsEmpty() || quote.m_sTargetZoneId.IsEmpty())
			return "exact player support operation immutable identity is incomplete";
		return "";
	}

	protected string ValidateEnemyDefensiveQRFRegistrationIdentity(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!state || !order || !manifest)
			return "exact enemy defensive QRF registration context is missing";
		if (!RequiresExactEnemyDirectedResponse(order))
			return "enemy order does not opt into an exact directed-response contract";
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
			&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return "exact enemy defensive QRF registration requires an open order";
		if (order.m_sOrderId.IsEmpty() || order.m_sOperationId.IsEmpty()
			|| order.m_sOperationId != HST_StableIdService.BuildOperationId("enemy_order", order.m_sOrderId)
			|| order.m_sOperationId != manifest.m_sOperationId
			|| order.m_sManifestId.IsEmpty() || order.m_sManifestId != manifest.m_sManifestId
			|| order.m_sManifestHash.IsEmpty() || order.m_sManifestHash != manifest.m_sManifestHash)
			return "exact enemy defensive QRF order or manifest identity conflicts";
		if (order.m_sFactionKey.IsEmpty() || order.m_sSourceZoneId.IsEmpty() || order.m_sTargetZoneId.IsEmpty()
			|| order.m_sSourceZoneId == order.m_sTargetZoneId || IsZeroVector(order.m_vSourcePosition)
			|| IsZeroVector(order.m_vTargetPosition))
			return "exact enemy defensive QRF source, target, or faction identity is incomplete";
		if (RequiresExactEnemyGarrisonRebuild(order))
		{
			HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
			if (!targetZone || order.m_iTargetOwnershipRevision <= 0
				|| targetZone.m_sOwnerFactionKey != order.m_sFactionKey
				|| targetZone.m_iOwnershipRevision != order.m_iTargetOwnershipRevision)
				return "exact enemy garrison rebuild target ownership revision is not frozen";
		}
		if (!order.m_sSpawnResultId.IsEmpty() || !order.m_sGroupId.IsEmpty() || !order.m_sSupportRequestId.IsEmpty()
			|| order.m_bPhysicalized || order.m_bAbstractResolved || order.m_bStrategicServiceCommitted
			|| order.m_bResourceSettlementApplied || order.m_bResourceRefundApplied
			|| !order.m_sResourceSettlementId.IsEmpty() || !order.m_sResourceSettlementKind.IsEmpty()
			|| order.m_iSettlementAcceptedMemberCount != 0 || order.m_iSettlementSurvivorMemberCount != 0
			|| order.m_iRefundedAttackResources != 0 || order.m_iRefundedSupportResources != 0)
			return "exact enemy defensive QRF registration contains execution or settlement authority";
		return ValidateExactEnemyDefensiveQRFManifest(order, manifest);
	}

	protected string ValidateExactEnemyDefensiveQRFManifest(
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest)
	{
		if (!order || !manifest)
			return "exact enemy defensive QRF manifest authority is missing";
		HST_StrategicMovementService movement = new HST_StrategicMovementService();
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		if (!manifest.m_bFrozen || !movement.IsSupportedExactInfantryManifest(manifest)
			|| manifest.m_sManifestHash.IsEmpty() || integrity.BuildManifestHash(manifest) != manifest.m_sManifestHash)
			return "exact enemy defensive QRF manifest is not a valid frozen infantry-only roster";
		if (manifest.m_sForceKind != ResolveExactEnemyDirectedForceKind(order)
			|| manifest.m_sPolicyId != ResolveExactEnemyDirectedPolicyId(order)
			|| manifest.m_sIntentId != ResolveExactEnemyDirectedManifestIntent(order))
			return "exact enemy defensive QRF manifest policy conflicts";
		if (manifest.m_sOperationId != order.m_sOperationId || manifest.m_sManifestId != order.m_sManifestId
			|| manifest.m_sManifestHash != order.m_sManifestHash || manifest.m_sFactionKey != order.m_sFactionKey
			|| manifest.m_sSourceZoneId != order.m_sSourceZoneId || manifest.m_sTargetZoneId != order.m_sTargetZoneId)
			return "exact enemy defensive QRF manifest backlinks conflict";
		if (manifest.m_iRequestedMemberCount != manifest.m_iAcceptedMemberCount
			|| manifest.m_iAcceptedMemberCount != order.m_iCompositionManpower
			|| manifest.m_iAttackResourceCost != order.m_iAttackCost
			|| manifest.m_iSupportResourceCost != order.m_iSupportCost
			|| order.m_iCompositionVehicleCount != 0 || order.m_iCompositionArmedVehicleCount != 0)
			return "exact enemy defensive QRF frozen roster or prepaid resource ledger conflicts";
		if (RequiresExactEnemyCounterattack(order))
		{
			bool attackFunded = order.m_iAttackCost > 0 && order.m_iSupportCost == 0;
			bool supportFunded = order.m_iSupportCost > 0 && order.m_iAttackCost == 0;
			if (!attackFunded && !supportFunded)
				return "exact enemy counterattack must freeze exactly one prepaid resource pool";
		}
		if (RequiresExactEnemyGarrisonRebuild(order)
			&& (order.m_iAttackCost != 0
				|| order.m_iSupportCost != EXACT_ENEMY_GARRISON_REBUILD_SUPPORT_COST))
			return "exact enemy garrison rebuild must freeze its support-only cost";
		return "";
	}

	protected string ResolveEnemyDefensiveQRFTransition(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		out HST_OperationRecordState operation,
		out HST_ForceManifestState manifest)
	{
		operation = null;
		manifest = null;
		if (!state || !order)
			return "exact enemy defensive QRF transition context is missing";
		if (!RequiresExactEnemyDirectedResponse(order))
			return "enemy order is not owned by an exact directed-response contract";
		operation = state.FindOperation(order.m_sOperationId);
		manifest = state.FindForceManifest(order.m_sManifestId);
		return ValidateExactEnemyDefensiveQRF(state, operation, order, manifest);
	}

	protected string ValidateEnemyDefensiveQRFProjectionLinks(
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ActiveGroupState group,
		HST_ForceSpawnResultState batch)
	{
		if (!operation || !manifest || !group || !batch)
			return "exact enemy defensive QRF projection authority is incomplete";
		if (batch.m_sOperationId != operation.m_sOperationId || batch.m_sManifestId != manifest.m_sManifestId)
			return "exact enemy defensive QRF projection backlinks conflict";
		if (group.m_sOperationId != operation.m_sOperationId || group.m_sEnemyOrderId != operation.m_sEnemyOrderId)
			return "exact enemy defensive QRF projection backlinks conflict";
		if (group.m_sManifestId != manifest.m_sManifestId || group.m_sSpawnResultId != batch.m_sResultId)
			return "exact enemy defensive QRF projection backlinks conflict";
		if (group.m_sProjectionId != batch.m_sProjectionId || group.m_sForceId != batch.m_sForceId)
			return "exact enemy defensive QRF projection backlinks conflict";
		if (group.m_sGroupId != batch.m_sProjectionId)
			return "exact enemy defensive QRF projection backlinks conflict";
		if (!group.m_bQRF || group.m_iVehicleCount != 0 || group.m_iOriginalVehicleCount != 0
			|| group.m_iCompositionVehicleCount != 0 || group.m_iCompositionArmedVehicleCount != 0
			|| group.m_iOriginalInfantryCount != manifest.m_iAcceptedMemberCount)
			return "exact enemy defensive QRF active group is not the frozen infantry-only roster";
		if (batch.m_sForceId != "force_" + operation.m_sOperationId
			|| batch.m_sProjectionId != "projection_" + operation.m_sOperationId)
			return "exact enemy defensive QRF force or projection identity conflicts";
		return "";
	}

	protected bool EnemyDefensiveQRFGroupLinksMatch(
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ActiveGroupState group)
	{
		if (!operation || !manifest || !group || operation.m_sGroupId.IsEmpty())
			return false;
		if (group.m_sGroupId != operation.m_sGroupId || group.m_sOperationId != operation.m_sOperationId)
			return false;
		if (group.m_sEnemyOrderId != operation.m_sEnemyOrderId || group.m_sManifestId != manifest.m_sManifestId)
			return false;
		if (group.m_sSpawnResultId != operation.m_sSpawnResultId || group.m_sForceId != operation.m_sForceId)
			return false;
		return group.m_sProjectionId == operation.m_sProjectionId;
	}

	protected string ResolveTransitionOperation(
		HST_CampaignState state,
		HST_SupportRequestState request,
		out HST_OperationRecordState operation)
	{
		operation = null;
		if (!request)
			return "exact player support operation request is missing";
		if (!RequiresOperation(request))
			return "";
		if (!state)
			return "exact player support operation state is missing";
		operation = state.FindOperation(request.m_sOperationId);
		if (!operation)
			return "versioned exact player support operation record is missing";
		HST_ForceQuoteState quote = state.FindForceQuote(request.m_sQuoteId);
		HST_ForceManifestState manifest = state.FindForceManifest(request.m_sManifestId);
		return ValidateExactPlayerSupport(state, operation, request, quote, manifest);
	}

	protected bool IsLegalEngagementTransition(HST_EOperationEngagementMode currentMode, HST_EOperationEngagementMode nextMode)
	{
		if (currentMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
			return nextMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT;
		if (currentMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT)
			return nextMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED;
		if (currentMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED)
			return nextMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_DISENGAGING;
		if (currentMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_DISENGAGING)
			return nextMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		return false;
	}

	protected bool IsActiveDuty(HST_EOperationDutyState dutyState)
	{
		return dutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING
			|| dutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			|| dutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			|| dutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ASSIGNMENT
			|| dutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RECALL_REQUESTED
			|| dutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN
			|| dutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_EXITING;
	}

	protected bool LinkMatchesOrEmpty(string currentValue, string expectedValue)
	{
		return currentValue.IsEmpty() || currentValue == expectedValue;
	}

	protected bool SetDuty(HST_OperationRecordState operation, HST_EOperationDutyState dutyState, int nowSecond)
	{
		if (!operation || operation.m_eDutyState == dutyState)
			return false;
		operation.m_eDutyState = dutyState;
		operation.m_iDutyStateEnteredAtSecond = nowSecond;
		return true;
	}

	protected bool SetResumeDuty(HST_OperationRecordState operation, HST_EOperationDutyState dutyState)
	{
		if (!operation || operation.m_eResumeDutyState == dutyState)
			return false;
		operation.m_eResumeDutyState = dutyState;
		return true;
	}

	protected bool ResetArrivalConfirmation(HST_OperationRecordState operation)
	{
		if (!operation || (operation.m_iArrivalConfirmationCount == 0 && operation.m_iLastArrivalConfirmationSecond == 0))
			return false;
		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		return true;
	}

	protected bool IsEnemyDefensiveQRFTerminalResult(HST_EOperationTerminalResult terminalResult)
	{
		return terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
			|| terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED
			|| terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED
			|| terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED
			|| terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED
			|| terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED;
	}

	protected bool SetMaterialization(
		HST_OperationRecordState operation,
		HST_EOperationMaterializationState materializationState,
		HST_EOperationPositionAuthority positionAuthority,
		int nowSecond)
	{
		if (!operation)
			return false;
		bool changed;
		if (operation.m_eMaterializationState != materializationState)
		{
			operation.m_eMaterializationState = materializationState;
			operation.m_iMaterializationStateEnteredAtSecond = nowSecond;
			changed = true;
		}
		if (operation.m_ePositionAuthority != positionAuthority)
		{
			operation.m_ePositionAuthority = positionAuthority;
			changed = true;
		}
		return changed;
	}

	protected HST_OperationTransitionResult FinishTransition(HST_OperationRecordState operation, bool changed, int nowSecond, bool revisionAlreadyApplied = false)
	{
		if (changed)
		{
			operation.m_iLastProgressAtSecond = nowSecond;
			if (!revisionAlreadyApplied)
				operation.m_iRevision++;
		}
		return BuildAccepted(operation, changed, !changed);
	}

	protected bool AssignString(out string target, string value)
	{
		if (target == value)
			return false;
		target = value;
		return true;
	}

	protected bool AssignVector(out vector target, vector value)
	{
		if (target == value)
			return false;
		target = value;
		return true;
	}

	protected int CountOperationId(HST_CampaignState state, string operationId)
	{
		int count;
		if (!state || operationId.IsEmpty())
			return count;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (operation && operation.m_sOperationId == operationId)
				count++;
		}
		return count;
	}

	protected int CountEnemyOrderId(HST_CampaignState state, string orderId)
	{
		int count;
		if (!state || orderId.IsEmpty())
			return count;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (order && order.m_sOrderId == orderId)
				count++;
		}
		return count;
	}

	protected int CountForceManifestId(HST_CampaignState state, string manifestId)
	{
		int count;
		if (!state || manifestId.IsEmpty())
			return count;
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
		{
			if (manifest && manifest.m_sManifestId == manifestId)
				count++;
		}
		return count;
	}

	protected int CountEnemyOrdersByAnyIdentity(HST_CampaignState state, HST_EnemyOrderState expected)
	{
		int count;
		if (!state || !expected)
			return count;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
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

	protected int CountEnemyForceManifestsByAnyIdentity(
		HST_CampaignState state,
		string manifestId,
		string operationId)
	{
		int count;
		if (!state)
			return count;
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
		{
			if (!manifest)
				continue;
			bool matches = !manifestId.IsEmpty() && manifest.m_sManifestId == manifestId;
			if (!matches && !operationId.IsEmpty())
				matches = manifest.m_sOperationId == operationId;
			if (matches)
				count++;
		}
		return count;
	}

	protected int CountOperationsByAnyIdentity(HST_CampaignState state, string operationId, string supportRequestId, string quoteId, string manifestId)
	{
		int count;
		if (!state)
			return count;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation)
				continue;
			if (operation.m_sOperationId == operationId || operation.m_sSupportRequestId == supportRequestId
				|| operation.m_sQuoteId == quoteId || operation.m_sManifestId == manifestId)
				count++;
		}
		return count;
	}

	protected int CountEnemyOperationsByAnyIdentity(HST_CampaignState state, string operationId, string enemyOrderId, string manifestId)
	{
		int count;
		if (!state)
			return count;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation)
				continue;
			if ((!operationId.IsEmpty() && operation.m_sOperationId == operationId)
				|| (!enemyOrderId.IsEmpty() && operation.m_sEnemyOrderId == enemyOrderId)
				|| (!manifestId.IsEmpty() && operation.m_sManifestId == manifestId))
				count++;
		}
		return count;
	}

	protected HST_OperationTransitionResult BuildAccepted(HST_OperationRecordState operation, bool changed, bool alreadyApplied)
	{
		HST_OperationTransitionResult result = new HST_OperationTransitionResult();
		result.m_bAccepted = true;
		result.m_bStateChanged = changed;
		result.m_bAlreadyApplied = alreadyApplied;
		result.m_Operation = operation;
		return result;
	}

	protected HST_OperationTransitionResult BuildRejected(string failure)
	{
		HST_OperationTransitionResult result = new HST_OperationTransitionResult();
		result.m_sFailureReason = failure;
		return result;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected float Distance2D(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}
}
