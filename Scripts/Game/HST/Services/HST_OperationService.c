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

	static bool RequiresOperation(HST_SupportRequestState request)
	{
		return request && request.m_iOperationContractVersion >= EXACT_PLAYER_QRF_CONTRACT_VERSION;
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
		string failure = ValidateRegistrationIdentity(state, request, quote, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);

		HST_OperationRecordState existing = state.FindOperation(request.m_sOperationId);
		if (existing)
		{
			failure = ValidateExactPlayerQRF(state, existing, request, quote, manifest);
			if (!failure.IsEmpty())
				return BuildRejected(failure);
			return BuildAccepted(existing, false, true);
		}

		if (CountOperationsByAnyIdentity(state, request.m_sOperationId, request.m_sRequestId, request.m_sQuoteId, request.m_sManifestId) > 0)
			return BuildRejected("exact player QRF operation identity is already owned by another record");

		HST_OperationRecordState operation = new HST_OperationRecordState();
		operation.m_sOperationId = request.m_sOperationId;
		operation.m_eType = HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_QRF;
		operation.m_iContractVersion = EXACT_PLAYER_QRF_CONTRACT_VERSION;
		operation.m_sOwnerFactionKey = quote.m_sFactionKey;
		operation.m_sActorIdentityId = quote.m_sActorIdentityId;
		operation.m_sIssueRequestId = quote.m_sCommandRequestId;
		operation.m_sConfirmationRequestId = quote.m_sConfirmationRequestId;
		operation.m_sSupportRequestId = request.m_sRequestId;
		operation.m_sQuoteId = quote.m_sQuoteId;
		operation.m_sManifestId = manifest.m_sManifestId;
		operation.m_sOriginZoneId = quote.m_sSourceZoneId;
		operation.m_vOriginPosition = quote.m_vSourcePosition;
		operation.m_sAssignmentKind = EXACT_PLAYER_QRF_ASSIGNMENT_KIND;
		operation.m_sAssignmentZoneId = quote.m_sTargetZoneId;
		operation.m_vAssignmentPosition = quote.m_vTargetPosition;
		operation.m_sTacticalTargetZoneId = quote.m_sTargetZoneId;
		operation.m_vTacticalTargetPosition = quote.m_vTargetPosition;
		operation.m_vStrategicPosition = quote.m_vSourcePosition;
		operation.m_sRecallPolicyId = EXACT_PLAYER_QRF_RECALL_POLICY;
		operation.m_sSettlementPolicyId = EXACT_PLAYER_QRF_SETTLEMENT_POLICY;
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

	HST_OperationTransitionResult RemoveUncommittedExactPlayerQRF(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		if (!RequiresOperation(request))
			return BuildAccepted(null, false, true);
		if (!state || !request)
			return BuildRejected("exact player QRF operation rollback context is missing");
		HST_OperationRecordState operation = state.FindOperation(request.m_sOperationId);
		if (!operation)
			return BuildAccepted(null, false, true);
		string failure = ValidateExactPlayerQRF(state, operation, request, quote, manifest);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| !operation.m_sSpawnResultId.IsEmpty() || !operation.m_sGroupId.IsEmpty())
			return BuildRejected("exact player QRF operation has execution or settlement authority and cannot be rolled back");
		int index = state.m_aOperations.Find(operation);
		if (index < 0)
			return BuildRejected("exact player QRF operation rollback identity disappeared");
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
			return BuildRejected("exact player QRF materialization links conflict with operation authority");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return BuildRejected("settled exact player QRF operation cannot materialize");
		if ((operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING
			&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
			|| (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return BuildRejected("exact player QRF outbound materialization transition is illegal from the current operation state");
		if (!LinkMatchesOrEmpty(operation.m_sSpawnResultId, batch.m_sResultId)
			|| !LinkMatchesOrEmpty(operation.m_sForceId, batch.m_sForceId)
			|| !LinkMatchesOrEmpty(operation.m_sProjectionId, batch.m_sProjectionId)
			|| !LinkMatchesOrEmpty(operation.m_sGroupId, group.m_sGroupId))
			return BuildRejected("exact player QRF outbound materialization would replace an authoritative projection link");

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
			return BuildRejected("exact player QRF physical handoff conflicts with operation authority");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return BuildRejected("settled exact player QRF operation cannot become physical");
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			return BuildRejected("exact player QRF physical handoff transition is illegal from the current materialization state");
		if (!LinkMatchesOrEmpty(operation.m_sSpawnResultId, batch.m_sResultId)
			|| !LinkMatchesOrEmpty(operation.m_sForceId, batch.m_sForceId)
			|| !LinkMatchesOrEmpty(operation.m_sProjectionId, batch.m_sProjectionId)
			|| !LinkMatchesOrEmpty(operation.m_sGroupId, group.m_sGroupId))
			return BuildRejected("exact player QRF physical handoff would replace an authoritative projection link");

		bool changed;
		changed = AssignString(operation.m_sSpawnResultId, batch.m_sResultId) || changed;
		changed = AssignString(operation.m_sForceId, batch.m_sForceId) || changed;
		changed = AssignString(operation.m_sProjectionId, batch.m_sProjectionId) || changed;
		changed = AssignString(operation.m_sGroupId, group.m_sGroupId) || changed;
		changed = AssignString(operation.m_sCurrentRouteId, group.m_sRouteId) || changed;
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING)
			changed = SetDuty(operation, HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND, state.m_iElapsedSeconds) || changed;
		if (operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
			changed = SetResumeDuty(operation, operation.m_eDutyState) || changed;
		changed = SetMaterialization(operation, HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL, HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE, state.m_iElapsedSeconds) || changed;
		return FinishTransition(operation, changed, state.m_iElapsedSeconds);
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
			return BuildRejected("exact player QRF arrival conflicts with physical operation authority");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return BuildRejected("settled exact player QRF operation cannot arrive on station");
		if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ASSIGNMENT
			&& operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
			return BuildRejected("exact player QRF arrival transition is illegal from the current duty state");

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
			return BuildRejected("exact player QRF restore projection conflicts with operation authority");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return BuildRejected("settled exact player QRF operation cannot reproject");
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return BuildRejected("exact player QRF restore projection is not in persisted materialization authority");
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
			return BuildRejected("settled exact player QRF operation cannot be recalled");
		if (!IsActiveDuty(operation.m_eDutyState))
			return BuildRejected("exact player QRF operation cannot be recalled from the current duty state");
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
			return BuildRejected("settled exact player QRF operation cannot be recalled");
		if (!IsActiveDuty(operation.m_eDutyState))
			return BuildRejected("exact player QRF operation cannot be recalled from the current duty state");
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
			return BuildRejected("exact player QRF recall route conflicts with operation authority");

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
		if (!operation || operation.m_iContractVersion != EXACT_PLAYER_QRF_CONTRACT_VERSION
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_QRF)
			return BuildRejected("exact player QRF operation is missing for engagement transition");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return BuildRejected("settled operation cannot change engagement mode");
		if (!IsActiveDuty(operation.m_eDutyState))
			return BuildRejected("exact player QRF operation duty state cannot accept engagement transitions");
		if (operation.m_eEngagementMode == nextMode)
			return BuildAccepted(operation, false, true);
		if (!IsLegalEngagementTransition(operation.m_eEngagementMode, nextMode))
			return BuildRejected("illegal exact player QRF engagement transition");
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
		HST_OperationRecordState operation;
		string failure = ResolveTransitionOperation(state, request, operation);
		if (!failure.IsEmpty())
			return BuildRejected(failure);
		if (!operation)
			return BuildAccepted(null, false, true);
		if (terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN
			|| terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE || settlementId.IsEmpty())
			return BuildRejected("exact player QRF terminal result or settlement identity is invalid");
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (operation.m_eTerminalResult == terminalResult && operation.m_sSettlementId == settlementId)
				return BuildAccepted(operation, false, true);
			return BuildRejected("exact player QRF operation is already settled with a conflicting result");
		}
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
			return BuildRejected("exact player QRF operation settlement state conflicts");
		return BuildAccepted(operation, false, false);
	}

	HST_OperationTransitionResult SettleExactPlayerQRF(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_EOperationTerminalResult terminalResult,
		string settlementId,
		string reason)
	{
		HST_OperationTransitionResult preflight = CanSettleExactPlayerQRF(state, request, terminalResult, settlementId);
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
		if (!state || !operation || !request || !quote || !manifest)
			return "exact player QRF operation authority is incomplete";
		if (CountOperationId(state, operation.m_sOperationId) != 1)
			return "exact player QRF operation identity is ambiguous";
		if (operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_QRF
			|| operation.m_iContractVersion != EXACT_PLAYER_QRF_CONTRACT_VERSION
			|| request.m_iOperationContractVersion != EXACT_PLAYER_QRF_CONTRACT_VERSION
			|| operation.m_iRevision <= 0)
			return "exact player QRF operation contract version conflicts";
		if (operation.m_sOperationId != request.m_sOperationId || operation.m_sOperationId != quote.m_sOperationId
			|| operation.m_sOperationId != manifest.m_sOperationId || operation.m_sSupportRequestId != request.m_sRequestId
			|| operation.m_sQuoteId != quote.m_sQuoteId || operation.m_sManifestId != manifest.m_sManifestId)
			return "exact player QRF operation aggregate links conflict";
		if (operation.m_sOwnerFactionKey != quote.m_sFactionKey || operation.m_sActorIdentityId != quote.m_sActorIdentityId
			|| operation.m_sIssueRequestId != quote.m_sCommandRequestId
			|| operation.m_sConfirmationRequestId != quote.m_sConfirmationRequestId)
			return "exact player QRF operation owner or command identity conflicts";
		if (operation.m_sOriginZoneId != quote.m_sSourceZoneId || operation.m_vOriginPosition != quote.m_vSourcePosition
			|| operation.m_sAssignmentKind != EXACT_PLAYER_QRF_ASSIGNMENT_KIND
			|| operation.m_sAssignmentZoneId != quote.m_sTargetZoneId || operation.m_vAssignmentPosition != quote.m_vTargetPosition)
			return "exact player QRF immutable origin or assignment conflicts";
		if (operation.m_sRecallPolicyId != EXACT_PLAYER_QRF_RECALL_POLICY
			|| operation.m_sSettlementPolicyId != EXACT_PLAYER_QRF_SETTLEMENT_POLICY
			|| operation.m_iDeterministicSeed != manifest.m_iDeterministicSeed)
			return "exact player QRF operation policy or deterministic seed conflicts";
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_UNKNOWN
			|| operation.m_eResumeDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_UNKNOWN
			|| operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_UNKNOWN
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_UNKNOWN
			|| operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_UNKNOWN
			|| operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_UNKNOWN)
			return "exact player QRF operation contains unknown state authority";
		bool hasExecutionLink = !operation.m_sSpawnResultId.IsEmpty() || !operation.m_sForceId.IsEmpty()
			|| !operation.m_sProjectionId.IsEmpty() || !operation.m_sGroupId.IsEmpty();
		if (hasExecutionLink && (operation.m_sSpawnResultId.IsEmpty() || operation.m_sForceId.IsEmpty()
			|| operation.m_sProjectionId.IsEmpty() || operation.m_sGroupId.IsEmpty()))
			return "exact player QRF operation execution links are incomplete";
		if (hasExecutionLink && (operation.m_sSpawnResultId != request.m_sSpawnResultId
			|| operation.m_sForceId != "force_" + operation.m_sOperationId
			|| operation.m_sProjectionId != "projection_" + operation.m_sOperationId
			|| operation.m_sGroupId != operation.m_sProjectionId))
			return "exact player QRF operation execution links conflict";
		if ((operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			&& !hasExecutionLink)
			return "exact player QRF operation materialization lacks execution authority";
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			&& operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
			return "physical exact player QRF operation does not own live position authority";
		if ((operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
			&& operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
			return "nonphysical exact player QRF operation does not own strategic position authority";
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN
				|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
				|| operation.m_sSettlementId.IsEmpty() || operation.m_iSettledAtSecond < 0
				|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
				|| operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
				|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
				|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
				return "settled exact player QRF operation lacks terminal authority";
		}
		else if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| !operation.m_sSettlementId.IsEmpty())
			return "open exact player QRF operation contains terminal authority";
		return "";
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
			return "exact player QRF operation registration context is missing";
		if (request.m_iOperationContractVersion != EXACT_PLAYER_QRF_CONTRACT_VERSION)
			return "exact player QRF request does not opt into the operation contract";
		if (!request.m_bPlayerRequested || request.m_eType != HST_ESupportRequestType.HST_SUPPORT_QRF
			|| quote.m_sQuoteKind != HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_QRF
			|| quote.m_eSupportType != HST_ESupportRequestType.HST_SUPPORT_QRF)
			return "operation registration is not an exact paid player QRF";
		if (request.m_sOperationId.IsEmpty() || request.m_sOperationId != HST_StableIdService.BuildOperationId("support", request.m_sRequestId)
			|| request.m_sOperationId != quote.m_sOperationId || request.m_sOperationId != manifest.m_sOperationId)
			return "exact player QRF operation identity conflicts";
		if (request.m_sRequestId != quote.m_sSupportRequestId || request.m_sQuoteId != quote.m_sQuoteId
			|| request.m_sManifestId != quote.m_sManifestId || request.m_sManifestId != manifest.m_sManifestId
			|| request.m_sCommandRequestId != quote.m_sConfirmationRequestId)
			return "exact player QRF request, quote, or manifest links conflict";
		if (quote.m_sActorIdentityId.IsEmpty() || quote.m_sCommandRequestId.IsEmpty() || quote.m_sConfirmationRequestId.IsEmpty()
			|| quote.m_sFactionKey.IsEmpty() || quote.m_sTargetZoneId.IsEmpty())
			return "exact player QRF operation immutable identity is incomplete";
		return "";
	}

	protected string ResolveTransitionOperation(
		HST_CampaignState state,
		HST_SupportRequestState request,
		out HST_OperationRecordState operation)
	{
		operation = null;
		if (!request)
			return "exact player QRF operation request is missing";
		if (!RequiresOperation(request))
			return "";
		if (!state)
			return "exact player QRF operation state is missing";
		operation = state.FindOperation(request.m_sOperationId);
		if (!operation)
			return "versioned exact player QRF operation record is missing";
		HST_ForceQuoteState quote = state.FindForceQuote(request.m_sQuoteId);
		HST_ForceManifestState manifest = state.FindForceManifest(request.m_sManifestId);
		return ValidateExactPlayerQRF(state, operation, request, quote, manifest);
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
}
