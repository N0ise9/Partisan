class HST_OperationRecordProofReport
{
	bool m_bIssueConfirmExact;
	bool m_bMaterializationExact;
	bool m_bEngagementExact;
	bool m_bRecallSettlementExact;
	bool m_bRestoreProjectionExact;
	bool m_bSchema48MigrationExact;
	bool m_bArchiveExact;
	bool m_bLegacyQRFIsolationExact;
	string m_sIssueConfirmEvidence;
	string m_sMaterializationEvidence;
	string m_sEngagementEvidence;
	string m_sRecallSettlementEvidence;
	string m_sRestoreProjectionEvidence;
	string m_sSchema48MigrationEvidence;
	string m_sArchiveEvidence;
	string m_sLegacyQRFIsolationEvidence;
}

class HST_OperationRecordProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_EconomyService m_Economy;
	ref HST_ResourceLedgerService m_Ledger;
	ref HST_ForcePlanningService m_Planning;
	ref HST_ForceSpawnQueueService m_Queue;
	ref HST_SupportRequestService m_Support;
	ref HST_ForceQuoteResult m_Issue;
	ref HST_ForceConfirmationResult m_Confirmation;
	ref HST_SupportRequestState m_Request;
	ref HST_OperationRecordState m_Operation;
	int m_iInitialMoney;
	int m_iInitialHR;
	int m_iOperationCountAfterIssue;
	int m_iMoneyAfterIssue;
	int m_iHRAfterIssue;
	int m_iRequestCountAfterIssue;
	int m_iTransactionCountAfterIssue;
	bool m_bConfirmed;
}

class HST_OperationRecordRuntimeFixture
{
	ref HST_OperationRecordProofFixture m_Base;
	ref HST_ForceSpawnQueueEnqueueResult m_Enqueue;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
	ref HST_OperationRecordState m_Operation;
	string m_sAssignmentZoneId;
	vector m_vAssignmentPosition;
	bool m_bLateOutboundRejected;
	bool m_bExact;
}

class HST_OperationRecordProofService
{
	HST_OperationRecordProofReport Run()
	{
		HST_OperationRecordProofReport report = new HST_OperationRecordProofReport();
		HST_OperationRecordProofFixture confirmed = BuildConfirmedFixture();
		ProveIssueConfirm(report, confirmed);

		HST_OperationRecordRuntimeFixture runtime = BuildRuntimeFixture();
		ProveMaterialization(report, runtime);
		ProveEngagement(report, runtime);
		ProveRestoreProjection(report, runtime);
		ProveRecallSettlement(report, runtime);

		ProveSchema48Migration(report);
		ProveArchive(report);
		return report;
	}

	protected HST_OperationRecordProofFixture BuildConfirmedFixture()
	{
		HST_OperationRecordProofFixture fixture = new HST_OperationRecordProofFixture();
		fixture.m_State = CreateProofState();
		fixture.m_Preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		fixture.m_Economy = new HST_EconomyService();
		fixture.m_Ledger = new HST_ResourceLedgerService();
		fixture.m_Planning = new HST_ForcePlanningService();
		fixture.m_Queue = new HST_ForceSpawnQueueService();
		fixture.m_Support = new HST_SupportRequestService();
		fixture.m_Support.SetExactForceAuthorityServices(fixture.m_Queue, fixture.m_Ledger, fixture.m_Economy);
		fixture.m_iInitialMoney = fixture.m_State.m_iFactionMoney;
		fixture.m_iInitialHR = fixture.m_State.m_iHR;
		fixture.m_Issue = fixture.m_Planning.IssuePlayerSupportQuote(
			fixture.m_State,
			fixture.m_Preset,
			"operation_record_proof_actor",
			HST_ESupportRequestType.HST_SUPPORT_QRF,
			"operation_record_target",
			"1600 20 1600",
			"operation_record_proof_issue",
			true);
		fixture.m_iOperationCountAfterIssue = fixture.m_State.m_aOperations.Count();
		fixture.m_iMoneyAfterIssue = fixture.m_State.m_iFactionMoney;
		fixture.m_iHRAfterIssue = fixture.m_State.m_iHR;
		fixture.m_iRequestCountAfterIssue = fixture.m_State.m_aSupportRequests.Count();
		fixture.m_iTransactionCountAfterIssue = fixture.m_State.m_aResourceTransactions.Count();
		if (!fixture.m_Issue || !fixture.m_Issue.m_bSuccess || !fixture.m_Issue.m_Quote || !fixture.m_Issue.m_Manifest)
			return fixture;

		fixture.m_Confirmation = fixture.m_Planning.ConfirmPlayerSupportQuote(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Economy,
			fixture.m_Support,
			fixture.m_Ledger,
			"operation_record_proof_actor",
			fixture.m_Issue.m_Quote.m_sQuoteId,
			"operation_record_proof_confirm");
		if (fixture.m_State.m_aSupportRequests.Count() == 1)
			fixture.m_Request = fixture.m_State.m_aSupportRequests[0];
		if (fixture.m_Request)
			fixture.m_Operation = fixture.m_State.FindOperation(fixture.m_Request.m_sOperationId);
		fixture.m_bConfirmed = fixture.m_Confirmation && fixture.m_Confirmation.m_bSuccess
			&& !fixture.m_Confirmation.m_bAlreadyApplied && fixture.m_Request && fixture.m_Operation;
		return fixture;
	}

	protected void ProveIssueConfirm(HST_OperationRecordProofReport report, HST_OperationRecordProofFixture fixture)
	{
		if (!report || !fixture || !fixture.m_Issue || !fixture.m_Issue.m_Quote || !fixture.m_Issue.m_Manifest)
		{
			if (report)
				report.m_sIssueConfirmEvidence = "exact paid QRF issue fixture was not created";
			return;
		}

		HST_ForceQuoteState quote = fixture.m_Issue.m_Quote;
		HST_ForceManifestState manifest = fixture.m_Issue.m_Manifest;
		bool issueBoundary = fixture.m_iOperationCountAfterIssue == 0
			&& fixture.m_iRequestCountAfterIssue == 0 && fixture.m_iTransactionCountAfterIssue == 0
			&& fixture.m_iMoneyAfterIssue == fixture.m_iInitialMoney
			&& fixture.m_iHRAfterIssue == fixture.m_iInitialHR;
		bool registered = fixture.m_bConfirmed && fixture.m_State.m_aOperations.Count() == 1;
		if (registered)
			registered = fixture.m_Request.m_iOperationContractVersion == HST_OperationService.EXACT_PLAYER_QRF_CONTRACT_VERSION
				&& fixture.m_Operation.m_iContractVersion == HST_OperationService.EXACT_PLAYER_QRF_CONTRACT_VERSION
				&& fixture.m_Operation.m_sOperationId == quote.m_sOperationId
				&& fixture.m_Operation.m_sSupportRequestId == fixture.m_Request.m_sRequestId
				&& fixture.m_Operation.m_sQuoteId == quote.m_sQuoteId
				&& fixture.m_Operation.m_sManifestId == manifest.m_sManifestId;
		if (registered)
			registered = fixture.m_Operation.m_sOriginZoneId == quote.m_sSourceZoneId
				&& fixture.m_Operation.m_sAssignmentZoneId == quote.m_sTargetZoneId
				&& PositionsMatch(fixture.m_Operation.m_vAssignmentPosition, quote.m_vTargetPosition)
				&& fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING
				&& fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		if (registered)
			registered = fixture.m_State.m_iFactionMoney == fixture.m_iInitialMoney - manifest.m_iMoneyCost
				&& fixture.m_State.m_iHR == fixture.m_iInitialHR - manifest.m_iHRCost
				&& fixture.m_State.m_aResourceTransactions.Count() == 2 && fixture.m_State.m_aQRFs.Count() == 0;

		int moneyBeforeReplay = fixture.m_State.m_iFactionMoney;
		int hrBeforeReplay = fixture.m_State.m_iHR;
		int transactionCountBeforeReplay = fixture.m_State.m_aResourceTransactions.Count();
		int operationRevisionBeforeReplay = -1;
		if (fixture.m_Operation)
			operationRevisionBeforeReplay = fixture.m_Operation.m_iRevision;
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult registrationReplay = operations.RegisterExactPlayerQRF(fixture.m_State, fixture.m_Request, quote, manifest);
		HST_ForceConfirmationResult confirmationReplay = fixture.m_Planning.ConfirmPlayerSupportQuote(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Economy,
			fixture.m_Support,
			fixture.m_Ledger,
			"operation_record_proof_actor",
			quote.m_sQuoteId,
			"operation_record_proof_confirm_replay");
		bool replayExact = registrationReplay && registrationReplay.m_bAccepted && registrationReplay.m_bAlreadyApplied
			&& !registrationReplay.m_bStateChanged && confirmationReplay && confirmationReplay.m_bSuccess
			&& confirmationReplay.m_bAlreadyApplied && fixture.m_State.m_aOperations.Count() == 1
			&& fixture.m_Operation.m_iRevision == operationRevisionBeforeReplay
			&& fixture.m_State.m_iFactionMoney == moneyBeforeReplay && fixture.m_State.m_iHR == hrBeforeReplay
			&& fixture.m_State.m_aResourceTransactions.Count() == transactionCountBeforeReplay;
		report.m_bIssueConfirmExact = issueBoundary && registered && replayExact;
		report.m_sIssueConfirmEvidence = string.Format(
			"issue operations/requests/transactions %1/%2/%3 balances %4/%5 | confirm %6 contract %7 operations %8",
			fixture.m_iOperationCountAfterIssue,
			fixture.m_iRequestCountAfterIssue,
			fixture.m_iTransactionCountAfterIssue,
			fixture.m_iMoneyAfterIssue,
			fixture.m_iHRAfterIssue,
			fixture.m_bConfirmed,
			fixture.m_Request && fixture.m_Request.m_iOperationContractVersion,
			fixture.m_State.m_aOperations.Count());
		report.m_sIssueConfirmEvidence = report.m_sIssueConfirmEvidence + string.Format(
			" | registration replay %1 | confirmation replay %2 | final balances %3/%4 tx %5 revision %6",
			registrationReplay && registrationReplay.m_bAlreadyApplied,
			confirmationReplay && confirmationReplay.m_bAlreadyApplied,
			fixture.m_State.m_iFactionMoney,
			fixture.m_State.m_iHR,
			fixture.m_State.m_aResourceTransactions.Count(),
			fixture.m_Operation && fixture.m_Operation.m_iRevision);
	}

	protected HST_OperationRecordRuntimeFixture BuildRuntimeFixture()
	{
		HST_OperationRecordRuntimeFixture runtime = new HST_OperationRecordRuntimeFixture();
		runtime.m_Base = BuildConfirmedFixture();
		if (!runtime.m_Base || !runtime.m_Base.m_bConfirmed)
			return runtime;
		runtime.m_Operation = runtime.m_Base.m_Operation;
		runtime.m_sAssignmentZoneId = runtime.m_Operation.m_sAssignmentZoneId;
		runtime.m_vAssignmentPosition = runtime.m_Operation.m_vAssignmentPosition;
		runtime.m_Base.m_State.m_iElapsedSeconds++;
		runtime.m_Enqueue = runtime.m_Base.m_Support.EnqueueAcceptedExactPlayerSupportProjection(
			runtime.m_Base.m_State,
			runtime.m_Base.m_Preset,
			runtime.m_Base.m_Request,
			"1200 20 1200",
			"1600 20 1600");
		if (!runtime.m_Enqueue || !runtime.m_Enqueue.m_bSuccess || !runtime.m_Enqueue.m_Batch)
			return runtime;
		runtime.m_Batch = runtime.m_Enqueue.m_Batch;
		runtime.m_Group = runtime.m_Base.m_State.FindActiveGroup(runtime.m_Batch.m_sProjectionId);
		bool outbound = runtime.m_Group
			&& runtime.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& runtime.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& runtime.m_Operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& runtime.m_Batch.m_bStrategicProjectionHeld
			&& runtime.m_Operation.m_iProjectionContractVersion == HST_StrategicMovementService.EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION
			&& runtime.m_Operation.m_sSpawnResultId == runtime.m_Batch.m_sResultId
			&& runtime.m_Operation.m_sGroupId == runtime.m_Group.m_sGroupId
			&& runtime.m_Operation.m_sProjectionId == runtime.m_Group.m_sProjectionId;

		HST_ForceSpawnQueueCallbackResult release = runtime.m_Base.m_Queue.ReleaseStrategicProjectionForMaterialization(
			runtime.m_Base.m_State.m_aForceSpawnResults,
			runtime.m_Base.m_Issue.m_Manifest,
			runtime.m_Batch.m_sResultId,
			runtime.m_Batch.m_sProjectionId,
			runtime.m_Base.m_State.m_iElapsedSeconds,
			runtime.m_Base.m_State.m_iElapsedSeconds + 120);
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult materializing = operations.MarkMaterializingFromVirtual(
			runtime.m_Base.m_State,
			runtime.m_Base.m_Request,
			runtime.m_Group,
			runtime.m_Batch,
			"operation proof entered materialize-in distance");
		runtime.m_Batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
		runtime.m_Base.m_State.m_iElapsedSeconds++;
		HST_OperationTransitionResult physical = operations.MarkPhysical(runtime.m_Base.m_State, runtime.m_Base.m_Request, runtime.m_Group, runtime.m_Batch);
		bool physicalExact = physical && physical.m_bAccepted && physical.m_bStateChanged
			&& release && release.m_bAccepted && materializing && materializing.m_bAccepted
			&& runtime.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			&& runtime.m_Operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		runtime.m_Base.m_State.m_iElapsedSeconds++;
		HST_OperationTransitionResult onStation = operations.MarkOnStation(runtime.m_Base.m_State, runtime.m_Base.m_Request, runtime.m_Group);
		bool arrivalExact = onStation && onStation.m_bAccepted && onStation.m_bStateChanged
			&& runtime.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			&& runtime.m_Operation.m_eResumeDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			&& runtime.m_Operation.m_sAssignmentZoneId == runtime.m_sAssignmentZoneId
			&& PositionsMatch(runtime.m_Operation.m_vAssignmentPosition, runtime.m_vAssignmentPosition);
		int revisionBeforeReplay = runtime.m_Operation.m_iRevision;
		HST_OperationTransitionResult arrivalReplay = operations.MarkOnStation(runtime.m_Base.m_State, runtime.m_Base.m_Request, runtime.m_Group);
		bool replayExact = arrivalReplay && arrivalReplay.m_bAccepted && arrivalReplay.m_bAlreadyApplied
			&& !arrivalReplay.m_bStateChanged && runtime.m_Operation.m_iRevision == revisionBeforeReplay;
		HST_OperationTransitionResult lateOutbound = operations.MarkOutboundMaterializing(
			runtime.m_Base.m_State,
			runtime.m_Base.m_Request,
			runtime.m_Group,
			runtime.m_Batch);
		bool lateReplayRejected = lateOutbound && !lateOutbound.m_bAccepted
			&& runtime.m_Operation.m_iRevision == revisionBeforeReplay
			&& runtime.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			&& runtime.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			&& runtime.m_Operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		runtime.m_bLateOutboundRejected = lateReplayRejected;
		runtime.m_bExact = outbound && physicalExact && arrivalExact && replayExact && lateReplayRejected;
		return runtime;
	}

	protected void ProveMaterialization(HST_OperationRecordProofReport report, HST_OperationRecordRuntimeFixture runtime)
	{
		if (!report)
			return;
		report.m_bMaterializationExact = runtime && runtime.m_bExact;
		report.m_sMaterializationEvidence = string.Format(
			"enqueue %1 | batch %2 group %3 | duty %4 | materialization %5 position %6 | immutable assignment %7 | exact %8",
			runtime && runtime.m_Enqueue && runtime.m_Enqueue.m_bSuccess,
			runtime && runtime.m_Batch != null,
			runtime && runtime.m_Group != null,
			runtime && runtime.m_Operation && runtime.m_Operation.m_eDutyState,
			runtime && runtime.m_Operation && runtime.m_Operation.m_eMaterializationState,
			runtime && runtime.m_Operation && runtime.m_Operation.m_ePositionAuthority,
			runtime && runtime.m_Operation && runtime.m_Operation.m_sAssignmentZoneId == runtime.m_sAssignmentZoneId,
			runtime && runtime.m_bExact);
		report.m_sMaterializationEvidence = report.m_sMaterializationEvidence
			+ string.Format(" | late outbound replay rejected without regression %1", runtime.m_bLateOutboundRejected);
	}

	protected void ProveEngagement(HST_OperationRecordProofReport report, HST_OperationRecordRuntimeFixture runtime)
	{
		if (!report || !runtime || !runtime.m_bExact || !runtime.m_Operation)
		{
			if (report)
				report.m_sEngagementEvidence = "physical on-station operation fixture was not created";
			return;
		}

		HST_OperationService operations = new HST_OperationService();
		HST_EOperationDutyState duty = runtime.m_Operation.m_eDutyState;
		int revisionBeforeIllegal = runtime.m_Operation.m_iRevision;
		HST_OperationTransitionResult illegal = operations.RecordEngagement(
			runtime.m_Base.m_State,
			runtime.m_Operation.m_sOperationId,
			HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED);
		bool illegalRejected = illegal && !illegal.m_bAccepted
			&& runtime.m_Operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
			&& runtime.m_Operation.m_eDutyState == duty && runtime.m_Operation.m_iRevision == revisionBeforeIllegal;

		runtime.m_Base.m_State.m_iElapsedSeconds++;
		HST_OperationTransitionResult contact = operations.RecordEngagement(runtime.m_Base.m_State, runtime.m_Operation.m_sOperationId, HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT);
		runtime.m_Base.m_State.m_iElapsedSeconds++;
		HST_OperationTransitionResult engaged = operations.RecordEngagement(runtime.m_Base.m_State, runtime.m_Operation.m_sOperationId, HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED);
		runtime.m_Base.m_State.m_iElapsedSeconds++;
		HST_OperationTransitionResult disengaging = operations.RecordEngagement(runtime.m_Base.m_State, runtime.m_Operation.m_sOperationId, HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_DISENGAGING);
		runtime.m_Base.m_State.m_iElapsedSeconds++;
		HST_OperationTransitionResult clear = operations.RecordEngagement(runtime.m_Base.m_State, runtime.m_Operation.m_sOperationId, HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR);
		bool legalSequence = TransitionChanged(contact) && TransitionChanged(engaged)
			&& TransitionChanged(disengaging) && TransitionChanged(clear)
			&& runtime.m_Operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
			&& runtime.m_Operation.m_eDutyState == duty && runtime.m_Operation.m_eResumeDutyState == duty
			&& runtime.m_Operation.m_iLastContactAtSecond > 0;
		report.m_bEngagementExact = illegalRejected && legalSequence;
		report.m_sEngagementEvidence = string.Format(
			"illegal rejected %1 | sequence contact/engaged/disengaging/clear %2/%3/%4/%5 | duty %6 resume %7 | final mode %8",
			illegalRejected,
			TransitionChanged(contact),
			TransitionChanged(engaged),
			TransitionChanged(disengaging),
			TransitionChanged(clear),
			runtime.m_Operation.m_eDutyState,
			runtime.m_Operation.m_eResumeDutyState,
			runtime.m_Operation.m_eEngagementMode);
	}

	protected void ProveRestoreProjection(HST_OperationRecordProofReport report, HST_OperationRecordRuntimeFixture runtime)
	{
		if (!report || !runtime || !runtime.m_bExact || !runtime.m_Operation)
		{
			if (report)
				report.m_sRestoreProjectionEvidence = "physical operation fixture was not created";
			return;
		}

		runtime.m_Group.m_vPosition = "1425 20 1510";
		runtime.m_Base.m_Request.m_bPhysicalized = true;
		runtime.m_Base.m_Request.m_bAbstractResolved = true;
		runtime.m_Base.m_Request.m_sRuntimeStatus = "physical_arrived";
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult sampled = operations.UpdatePhysicalPosition(
			runtime.m_Base.m_State,
			runtime.m_Base.m_Request,
			runtime.m_Group);
		int sourceRevision = runtime.m_Operation.m_iRevision;
		int sourceReprojectionCount = runtime.m_Batch.m_iReprojectionCount;
		vector savedGroupPosition = runtime.m_Group.m_vPosition;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(runtime.m_Base.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_OperationRecordState restoredOperation;
		HST_SupportRequestState restoredRequest;
		HST_ForceSpawnResultState restoredBatch;
		HST_ActiveGroupState restoredGroup;
		HST_ForceManifestState restoredManifest;
		if (restored)
		{
			restoredOperation = restored.FindOperation(runtime.m_Operation.m_sOperationId);
			restoredRequest = restored.FindSupportRequest(runtime.m_Base.m_Request.m_sRequestId);
			restoredBatch = restored.FindForceSpawnResult(runtime.m_Batch.m_sResultId);
			restoredGroup = restored.FindActiveGroup(runtime.m_Group.m_sGroupId);
			restoredManifest = restored.FindForceManifest(runtime.m_Base.m_Issue.m_Manifest.m_sManifestId);
		}
		bool restoreRecordsPresent = restored && restoredOperation && restoredRequest;
		restoreRecordsPresent = restoreRecordsPresent && restoredBatch && restoredGroup && restoredManifest;
		bool routeInitialized;
		if (restoreRecordsPresent)
		{
			HST_StrategicMovementService movement = new HST_StrategicMovementService();
			routeInitialized = movement.InitializeExactPlayerQRFRoute(
				restored,
				restoredOperation,
				restoredRequest,
				restoredManifest,
				restoredGroup);
		}
		bool exact = sampled && sampled.m_bAccepted && restoreRecordsPresent && routeInitialized;
		if (exact)
			exact = restored.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION
				&& restoredOperation != runtime.m_Operation && restored.m_aOperations.Count() == 1
				&& restoredRequest.m_iOperationContractVersion == HST_OperationService.EXACT_PLAYER_QRF_CONTRACT_VERSION;
		if (exact)
			exact = restoredOperation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				&& restoredOperation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
				&& restoredOperation.m_iProjectionContractVersion == HST_StrategicMovementService.EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION
				&& restoredOperation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
				&& restoredOperation.m_eResumeDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
				&& restoredOperation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		if (exact)
			exact = restoredOperation.m_sAssignmentZoneId == runtime.m_sAssignmentZoneId
				&& PositionsMatch(restoredOperation.m_vAssignmentPosition, runtime.m_vAssignmentPosition)
				&& PositionsMatch(restoredOperation.m_vStrategicPosition, savedGroupPosition)
				&& PositionsMatch(restoredOperation.m_vRouteStartPosition, savedGroupPosition)
				&& PositionsMatch(restoredGroup.m_vPosition, savedGroupPosition)
				&& restoredOperation.m_sGroupId == runtime.m_Group.m_sGroupId
				&& restoredOperation.m_iRevision == sourceRevision + 1;
		if (exact)
			exact = restoredBatch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
				&& restoredBatch.m_bStrategicProjectionHeld
				&& restoredBatch.m_iReprojectionCount == sourceReprojectionCount + 1;
		if (exact)
			exact = !restoredRequest.m_bPhysicalized
				&& restoredRequest.m_bAbstractResolved
				&& restoredRequest.m_sRuntimeStatus == "exact_virtual_on_station";
		report.m_bRestoreProjectionExact = exact;
		bool requestVirtualized;
		string restoredRequestStatus;
		if (restoredRequest)
		{
			requestVirtualized = !restoredRequest.m_bPhysicalized;
			restoredRequestStatus = restoredRequest.m_sRuntimeStatus;
		}
		report.m_sRestoreProjectionEvidence = string.Format(
			"schema %1 | restored operation %2 deep copy %3 | materialization %4 position %5 | strategic group position %6 | duty/resume %7/%8",
			restored && restored.m_iSchemaVersion,
			restoredOperation != null,
			restoredOperation && restoredOperation != runtime.m_Operation,
			restoredOperation && restoredOperation.m_eMaterializationState,
			restoredOperation && restoredOperation.m_ePositionAuthority,
			restoredOperation && PositionsMatch(restoredOperation.m_vStrategicPosition, savedGroupPosition),
			restoredOperation && restoredOperation.m_eDutyState,
			restoredOperation && restoredOperation.m_eResumeDutyState);
		report.m_sRestoreProjectionEvidence = report.m_sRestoreProjectionEvidence + string.Format(
			" | cursor/init %1/%2 | reprojection %3",
			restoredOperation && PositionsMatch(restoredOperation.m_vRouteStartPosition, savedGroupPosition),
			routeInitialized,
			restoredBatch && restoredBatch.m_iReprojectionCount);
		report.m_sRestoreProjectionEvidence = report.m_sRestoreProjectionEvidence + string.Format(
			" | revision %1/%2 | request virtualized %3 status %4",
			restoredOperation && restoredOperation.m_iRevision,
			sourceRevision + 1,
			requestVirtualized,
			restoredRequestStatus);
	}

	protected void ProveRecallSettlement(HST_OperationRecordProofReport report, HST_OperationRecordRuntimeFixture runtime)
	{
		if (!report || !runtime || !runtime.m_bExact || !runtime.m_Operation || !runtime.m_Group)
		{
			if (report)
				report.m_sRecallSettlementEvidence = "physical operation fixture was not created";
			return;
		}

		HST_OperationService operations = new HST_OperationService();
		string assignmentZone = runtime.m_Operation.m_sAssignmentZoneId;
		vector assignmentPosition = runtime.m_Operation.m_vAssignmentPosition;
		vector exitPosition = runtime.m_Operation.m_vOriginPosition;
		runtime.m_Base.m_State.m_iElapsedSeconds++;
		HST_OperationTransitionResult begin = operations.BeginRecall(runtime.m_Base.m_State, runtime.m_Base.m_Request, exitPosition);
		int revisionAfterBegin = runtime.m_Operation.m_iRevision;
		HST_OperationTransitionResult beginReplay = operations.BeginRecall(runtime.m_Base.m_State, runtime.m_Base.m_Request, exitPosition);
		runtime.m_Base.m_State.m_iElapsedSeconds++;
		HST_OperationTransitionResult exiting = operations.MarkRecallExiting(runtime.m_Base.m_State, runtime.m_Base.m_Request, runtime.m_Group, exitPosition);
		bool recallExact = TransitionChanged(begin) && beginReplay && beginReplay.m_bAccepted
			&& beginReplay.m_bAlreadyApplied && !beginReplay.m_bStateChanged
			&& revisionAfterBegin > 0 && exiting && exiting.m_bAccepted && exiting.m_bStateChanged
			&& runtime.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_EXITING
			&& runtime.m_Operation.m_sTacticalTargetZoneId == runtime.m_Operation.m_sOriginZoneId
			&& PositionsMatch(runtime.m_Operation.m_vTacticalTargetPosition, exitPosition)
			&& runtime.m_Operation.m_sAssignmentZoneId == assignmentZone
			&& PositionsMatch(runtime.m_Operation.m_vAssignmentPosition, assignmentPosition);

		string settlementKind = "operation_proof_recalled";
		string settlementId = HST_OperationService.BuildSettlementId(runtime.m_Operation.m_sOperationId, settlementKind);
		runtime.m_Base.m_State.m_iElapsedSeconds++;
		HST_OperationTransitionResult settlement = operations.SettleExactPlayerQRF(
			runtime.m_Base.m_State,
			runtime.m_Base.m_Request,
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_RECALLED,
			settlementId,
			"operation record deterministic recall proof");
		int revisionAfterSettlement = runtime.m_Operation.m_iRevision;
		HST_OperationTransitionResult settlementReplay = operations.SettleExactPlayerQRF(
			runtime.m_Base.m_State,
			runtime.m_Base.m_Request,
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_RECALLED,
			settlementId,
			"operation record deterministic recall replay");
		HST_OperationTransitionResult conflict = operations.SettleExactPlayerQRF(
			runtime.m_Base.m_State,
			runtime.m_Base.m_Request,
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
			HST_OperationService.BuildSettlementId(runtime.m_Operation.m_sOperationId, "operation_proof_destroyed"),
			"conflicting deterministic terminal result");
		bool settlementExact = TransitionChanged(settlement)
			&& settlementReplay && settlementReplay.m_bAccepted && settlementReplay.m_bAlreadyApplied
			&& !settlementReplay.m_bStateChanged && runtime.m_Operation.m_iRevision == revisionAfterSettlement
			&& conflict && !conflict.m_bAccepted
			&& runtime.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& runtime.m_Operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_RECALLED
			&& runtime.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			&& runtime.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
			&& runtime.m_Operation.m_sSettlementId == settlementId
			&& runtime.m_Operation.m_sAssignmentZoneId == assignmentZone
			&& PositionsMatch(runtime.m_Operation.m_vAssignmentPosition, assignmentPosition);
		report.m_bRecallSettlementExact = recallExact && settlementExact;
		report.m_sRecallSettlementEvidence = string.Format(
			"recall begin/replay/exiting %1/%2/%3 | immutable assignment %4 | settled %5 result %6 | settlement replay %7 revision %8 | conflict rejected %9",
			TransitionChanged(begin),
			beginReplay && beginReplay.m_bAlreadyApplied,
			exiting && exiting.m_bStateChanged,
			runtime.m_Operation.m_sAssignmentZoneId == assignmentZone,
			settlement && settlement.m_bStateChanged,
			runtime.m_Operation.m_eTerminalResult,
			settlementReplay && settlementReplay.m_bAlreadyApplied,
			runtime.m_Operation.m_iRevision,
			conflict && !conflict.m_bAccepted);
	}

	protected void ProveSchema48Migration(HST_OperationRecordProofReport report)
	{
		if (!report)
			return;
		HST_OperationRecordProofFixture fixture = BuildConfirmedFixture();
		if (!fixture || !fixture.m_bConfirmed)
		{
			report.m_sSchema48MigrationEvidence = "confirmed schema 48 migration fixture was not created";
			report.m_sLegacyQRFIsolationEvidence = report.m_sSchema48MigrationEvidence;
			return;
		}

		fixture.m_State.m_iSchemaVersion = 48;
		fixture.m_State.m_iLastLoadedSchemaVersion = 48;
		fixture.m_State.m_aOperations.Clear();
		fixture.m_Request.m_iOperationContractVersion = 0;
		HST_SupportRequestState preExact = new HST_SupportRequestState();
		preExact.m_sRequestId = "operation_record_pre_exact_request";
		preExact.m_sOperationId = HST_StableIdService.BuildOperationId("support", preExact.m_sRequestId);
		preExact.m_sFactionKey = fixture.m_Request.m_sFactionKey;
		preExact.m_eType = HST_ESupportRequestType.HST_SUPPORT_QRF;
		preExact.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_QUEUED;
		preExact.m_bPlayerRequested = true;
		fixture.m_State.m_aSupportRequests.Insert(preExact);
		HST_QRFState legacyQRF = new HST_QRFState();
		legacyQRF.m_sInstanceId = "operation_record_legacy_enemy_qrf";
		legacyQRF.m_sFactionKey = "US";
		legacyQRF.m_sSourceZoneId = "operation_record_target";
		legacyQRF.m_sTargetZoneId = "operation_record_source";
		legacyQRF.m_sGroupId = "operation_record_legacy_enemy_group";
		legacyQRF.m_iStartedAtSecond = 20;
		legacyQRF.m_iETASeconds = 90;
		fixture.m_State.m_aQRFs.Insert(legacyQRF);
		int expectedMoney = fixture.m_State.m_iFactionMoney;
		int expectedHR = fixture.m_State.m_iHR;
		int expectedTransactions = fixture.m_State.m_aResourceTransactions.Count();

		HST_CampaignSaveData coherentSave = new HST_CampaignSaveData();
		coherentSave.Capture(fixture.m_State);
		fixture.m_State.m_aForceQuotes.Insert(fixture.m_Issue.m_Quote);
		HST_CampaignSaveData ambiguousSave = new HST_CampaignSaveData();
		ambiguousSave.Capture(fixture.m_State);
		fixture.m_State.m_aForceQuotes.Remove(fixture.m_State.m_aForceQuotes.Count() - 1);
		HST_ResourceTransactionState removedHR = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sHRTransactionId);
		int removedHRIndex = fixture.m_State.m_aResourceTransactions.Find(removedHR);
		if (removedHRIndex >= 0)
			fixture.m_State.m_aResourceTransactions.Remove(removedHRIndex);
		HST_CampaignSaveData incompleteLedgerSave = new HST_CampaignSaveData();
		incompleteLedgerSave.Capture(fixture.m_State);
		if (removedHR)
			fixture.m_State.m_aResourceTransactions.Insert(removedHR);
		fixture.m_Request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
		fixture.m_Request.m_sResolutionKind = "operation_proof_terminal_legacy";
		HST_CampaignSaveData terminalSave = new HST_CampaignSaveData();
		terminalSave.Capture(fixture.m_State);

		HST_CampaignState coherent = coherentSave.Restore();
		HST_CampaignState ambiguous = ambiguousSave.Restore();
		HST_CampaignState incompleteLedger = incompleteLedgerSave.Restore();
		HST_CampaignState terminal = terminalSave.Restore();
		HST_SupportRequestState coherentRequest;
		HST_SupportRequestState coherentPreExact;
		HST_OperationRecordState coherentOperation;
		if (coherent)
		{
			coherentRequest = coherent.FindSupportRequest(fixture.m_Request.m_sRequestId);
			coherentPreExact = coherent.FindSupportRequest(preExact.m_sRequestId);
			coherentOperation = coherent.FindOperation(fixture.m_Request.m_sOperationId);
		}
		HST_SupportRequestState ambiguousRequest;
		if (ambiguous)
			ambiguousRequest = ambiguous.FindSupportRequest(fixture.m_Request.m_sRequestId);
		HST_SupportRequestState terminalRequest;
		if (terminal)
			terminalRequest = terminal.FindSupportRequest(fixture.m_Request.m_sRequestId);
		HST_SupportRequestState incompleteLedgerRequest;
		if (incompleteLedger)
			incompleteLedgerRequest = incompleteLedger.FindSupportRequest(fixture.m_Request.m_sRequestId);

		HST_CampaignSaveData idempotentSave = new HST_CampaignSaveData();
		if (coherent)
			idempotentSave.Capture(coherent);
		HST_CampaignState idempotent;
		if (coherent)
			idempotent = idempotentSave.Restore();
		HST_OperationRecordState idempotentOperation;
		HST_SupportRequestState idempotentRequest;
		if (idempotent)
		{
			idempotentOperation = idempotent.FindOperation(fixture.m_Request.m_sOperationId);
			idempotentRequest = idempotent.FindSupportRequest(fixture.m_Request.m_sRequestId);
		}

		bool coherentExact = coherent && coherent.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION
			&& coherent.m_aOperations.Count() == 1 && coherentOperation && coherentRequest && coherentPreExact
			&& coherentRequest.m_iOperationContractVersion == HST_OperationService.EXACT_PLAYER_QRF_CONTRACT_VERSION
			&& coherentPreExact.m_iOperationContractVersion == 0
			&& coherentOperation.m_sSupportRequestId == coherentRequest.m_sRequestId
			&& coherentOperation.m_sAssignmentZoneId == fixture.m_Issue.m_Quote.m_sTargetZoneId
			&& coherentOperation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING
			&& coherent.m_iFactionMoney == expectedMoney && coherent.m_iHR == expectedHR
			&& coherent.m_aResourceTransactions.Count() == expectedTransactions
			&& coherentRequest.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED
			&& HasEvent(coherent, "migration_schema49_operation_record");
		bool idempotentExact = idempotent && idempotent.m_aOperations.Count() == 1
			&& idempotentOperation && idempotentRequest
			&& idempotentRequest.m_iOperationContractVersion == HST_OperationService.EXACT_PLAYER_QRF_CONTRACT_VERSION;
		bool ambiguousPreserved = ambiguous && ambiguous.m_aOperations.Count() == 0 && ambiguousRequest
			&& ambiguousRequest.m_iOperationContractVersion == 0
			&& ambiguousRequest.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED
			&& ambiguous.m_iFactionMoney == expectedMoney && ambiguous.m_iHR == expectedHR
			&& HasEvent(ambiguous, "migration_schema49_operation_record_conflict");
		bool terminalPreserved = terminal && terminal.m_aOperations.Count() == 0 && terminalRequest
			&& terminalRequest.m_iOperationContractVersion == 0
			&& terminalRequest.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED
			&& terminal.m_iFactionMoney == expectedMoney && terminal.m_iHR == expectedHR;
		bool incompleteLedgerPreserved = incompleteLedger && incompleteLedger.m_aOperations.Count() == 0 && incompleteLedgerRequest
			&& incompleteLedgerRequest.m_iOperationContractVersion == 0
			&& incompleteLedgerRequest.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED
			&& incompleteLedger.m_iFactionMoney == expectedMoney && incompleteLedger.m_iHR == expectedHR
			&& incompleteLedger.m_aResourceTransactions.Count() == expectedTransactions - 1
			&& HasEvent(incompleteLedger, "migration_schema49_operation_record_conflict");
		report.m_bSchema48MigrationExact = coherentExact && idempotentExact && ambiguousPreserved
			&& incompleteLedgerPreserved && terminalPreserved;
		report.m_sSchema48MigrationEvidence = string.Format(
			"coherent operations/contract %1/%2 | idempotent operations %3 | pre-exact contract %4 | ambiguous operations/contract/conflict %5/%6/%7",
			coherent && coherent.m_aOperations.Count(),
			coherentRequest && coherentRequest.m_iOperationContractVersion,
			idempotent && idempotent.m_aOperations.Count(),
			coherentPreExact && coherentPreExact.m_iOperationContractVersion,
			ambiguous && ambiguous.m_aOperations.Count(),
			ambiguousRequest && ambiguousRequest.m_iOperationContractVersion,
			ambiguous && HasEvent(ambiguous, "migration_schema49_operation_record_conflict"));
		report.m_sSchema48MigrationEvidence = report.m_sSchema48MigrationEvidence + string.Format(
			" | incomplete ledger operations/contract/conflict %1/%2/%3 | terminal operations/contract %4/%5 | balances %6/%7 tx %8",
			incompleteLedger && incompleteLedger.m_aOperations.Count(),
			incompleteLedgerRequest && incompleteLedgerRequest.m_iOperationContractVersion,
			incompleteLedger && HasEvent(incompleteLedger, "migration_schema49_operation_record_conflict"),
			terminal && terminal.m_aOperations.Count(),
			terminalRequest && terminalRequest.m_iOperationContractVersion,
			coherent && coherent.m_iFactionMoney,
			coherent && coherent.m_iHR,
			coherent && coherent.m_aResourceTransactions.Count());

		string legacyOperationId = HST_StableIdService.BuildOperationId("qrf", legacyQRF.m_sInstanceId);
		bool isolationExact = coherent && ambiguous && incompleteLedger && terminal
			&& coherent.m_aQRFs.Count() == 1 && ambiguous.m_aQRFs.Count() == 1
			&& incompleteLedger.m_aQRFs.Count() == 1 && terminal.m_aQRFs.Count() == 1
			&& coherent.m_aQRFs[0].m_sInstanceId == legacyQRF.m_sInstanceId
			&& !coherent.FindOperation(legacyOperationId) && !ambiguous.FindOperation(legacyOperationId)
			&& !incompleteLedger.FindOperation(legacyOperationId) && !terminal.FindOperation(legacyOperationId);
		report.m_bLegacyQRFIsolationExact = isolationExact;
		report.m_sLegacyQRFIsolationEvidence = string.Format(
			"legacy QRF rows coherent/ambiguous/incomplete/terminal %1/%2/%3/%4 | legacy operation invented %5 | exact support operations %6",
			coherent && coherent.m_aQRFs.Count(),
			ambiguous && ambiguous.m_aQRFs.Count(),
			incompleteLedger && incompleteLedger.m_aQRFs.Count(),
			terminal && terminal.m_aQRFs.Count(),
			coherent && coherent.FindOperation(legacyOperationId) != null,
			coherent && coherent.m_aOperations.Count());
	}

	protected void ProveArchive(HST_OperationRecordProofReport report)
	{
		if (!report)
			return;
		HST_OperationRecordProofFixture fixture = BuildConfirmedFixture();
		if (!fixture || !fixture.m_bConfirmed)
		{
			report.m_sArchiveEvidence = "confirmed operation archive fixture was not created";
			return;
		}

		string settlementKind = "operation_proof_archived_recall";
		string settlementId = HST_OperationService.BuildSettlementId(fixture.m_Operation.m_sOperationId, settlementKind);
		fixture.m_State.m_iElapsedSeconds = Math.Max(1000, fixture.m_Issue.m_Quote.m_iAcceptedAtSecond + HST_ForceSettlementArchiveService.MIN_ACCEPTED_RECORD_RETENTION_SECONDS + 1);
		fixture.m_Request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
		fixture.m_Request.m_sResolutionKind = settlementKind;
		fixture.m_Request.m_iResolvedAtSecond = fixture.m_State.m_iElapsedSeconds;
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult settled = operations.SettleExactPlayerQRF(
			fixture.m_State,
			fixture.m_Request,
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_RECALLED,
			settlementId,
			"operation record deterministic archive proof");
		int settledRevision = fixture.m_Operation.m_iRevision;
		HST_ForceSettlementArchiveService archive = new HST_ForceSettlementArchiveService();
		HST_ForceSettlementArchiveResult archived = archive.ArchiveSettledRecords(fixture.m_State);
		HST_ForceSettlementTombstoneState tombstone = fixture.m_State.FindForceSettlementTombstone(fixture.m_Issue.m_Quote.m_sQuoteId);
		bool compacted = settled && settled.m_bAccepted && settled.m_bStateChanged
			&& archived && archived.m_iArchivedCount == 1 && tombstone
			&& fixture.m_State.m_aOperations.Count() == 0
			&& fixture.m_State.m_aForceQuotes.Count() == 0
			&& fixture.m_State.m_aForceManifests.Count() == 0
			&& fixture.m_State.m_aResourceTransactions.Count() == 0
			&& tombstone.m_iOperationContractVersion == HST_OperationService.EXACT_PLAYER_QRF_CONTRACT_VERSION
			&& tombstone.m_sOperationSettlementId == settlementId
			&& tombstone.m_iOperationRevision == settledRevision
			&& tombstone.m_eOperationTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_RECALLED;

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_ForceSettlementTombstoneState restoredTombstone;
		if (restored && tombstone)
			restoredTombstone = restored.FindForceSettlementTombstone(tombstone.m_sQuoteId);
		HST_ForceConfirmationResult confirmationReplay;
		if (restored && restoredTombstone)
		{
			HST_ForcePlanningService planning = new HST_ForcePlanningService();
			confirmationReplay = planning.ConfirmPlayerSupportQuote(
				restored,
				HST_DefaultCatalog.CreateVanillaEveronPreset(),
				new HST_EconomyService(),
				new HST_SupportRequestService(),
				new HST_ResourceLedgerService(),
				restoredTombstone.m_sActorIdentityId,
				restoredTombstone.m_sQuoteId,
				"operation_record_archive_replay");
		}
		bool persisted = restoredTombstone && restoredTombstone != tombstone
			&& restored.m_aOperations.Count() == 0
			&& restoredTombstone.m_iOperationContractVersion == HST_OperationService.EXACT_PLAYER_QRF_CONTRACT_VERSION
			&& restoredTombstone.m_sOperationSettlementId == settlementId
			&& restoredTombstone.m_iOperationRevision == settledRevision
			&& restoredTombstone.m_eOperationTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_RECALLED
			&& confirmationReplay && confirmationReplay.m_bSuccess && confirmationReplay.m_bAlreadyApplied;
		bool mismatchRetained = ProveTypedQRFArchiveMismatchRetention(
			restored,
			restoredTombstone);
		report.m_bArchiveExact = compacted && persisted && mismatchRetained;
		report.m_sArchiveEvidence = string.Format(
			"settled %1 | archived %2 | operations/full rows %3/%4/%5/%6 | tombstone contract/revision %7/%8",
			settled && settled.m_bStateChanged,
			archived && archived.m_iArchivedCount,
			fixture.m_State.m_aOperations.Count(),
			fixture.m_State.m_aForceQuotes.Count(),
			fixture.m_State.m_aForceManifests.Count(),
			fixture.m_State.m_aResourceTransactions.Count(),
			tombstone && tombstone.m_iOperationContractVersion,
			tombstone && tombstone.m_iOperationRevision);
		report.m_sArchiveEvidence = report.m_sArchiveEvidence + string.Format(
			" result %1 | restored %2 replay %3 | mismatch retained %4",
			tombstone && tombstone.m_eOperationTerminalResult,
			restoredTombstone != null,
			confirmationReplay && confirmationReplay.m_bAlreadyApplied,
			mismatchRetained);
	}

	protected bool ProveTypedQRFArchiveMismatchRetention(
		HST_CampaignState state,
		HST_ForceSettlementTombstoneState tombstone)
	{
		if (!state || !tombstone)
			return false;
		HST_SupportRequestState request = state.FindSupportRequest(
			tombstone.m_sSupportRequestId);
		if (!request || request.m_iOperationContractVersion
			!= HST_OperationService.EXACT_PLAYER_QRF_CONTRACT_VERSION)
			return false;
		request.m_iMoneyCost++;
		int archiveSecond = tombstone.m_iArchivedAtSecond;
		int index = state.m_aForceSettlementTombstones.Count();
		while (index < HST_ForceSettlementArchiveService.MAX_TOMBSTONE_ROWS)
		{
			HST_ForceSettlementTombstoneState retained = new HST_ForceSettlementTombstoneState();
			retained.m_sQuoteId = string.Format("typed_qrf_retained_archive_%1", index);
			retained.m_iArchivedAtSecond = archiveSecond + index + 1;
			state.m_aForceSettlementTombstones.Insert(retained);
			index++;
		}
		state.m_iElapsedSeconds = archiveSecond
			+ HST_ForceSettlementArchiveService.MIN_TOMBSTONE_RETENTION_SECONDS
			+ HST_ForceSettlementArchiveService.MAX_TOMBSTONE_ROWS + 1;
		HST_ForceSettlementArchiveService archive = new HST_ForceSettlementArchiveService();
		HST_ForceSettlementArchiveResult prune = archive.ArchiveSettledRecords(state);
		return prune && prune.m_iPrunedTombstoneCount == 1
			&& state.FindForceSettlementTombstone(tombstone.m_sQuoteId) == tombstone
			&& state.FindSupportRequest(request.m_sRequestId) == request;
	}

	protected HST_CampaignState CreateProofState()
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iCampaignSeed = 4949;
		state.m_iElapsedSeconds = 100;
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		state.m_bHQDeployed = true;
		state.m_vHQPosition = "1000 20 1000";
		state.m_iWarLevel = 3;
		state.m_iFactionMoney = 1000;
		state.m_iHR = 50;
		HST_FactionPoolState pool = new HST_FactionPoolState();
		pool.m_sFactionKey = "FIA";
		state.m_aFactionPools.Insert(pool);
		HST_ZoneState source = new HST_ZoneState();
		source.m_sZoneId = "operation_record_source";
		source.m_sDisplayName = "Operation Record Source";
		source.m_sOwnerFactionKey = "FIA";
		source.m_vPosition = "1000 20 1000";
		state.m_aZones.Insert(source);
		HST_ZoneState target = new HST_ZoneState();
		target.m_sZoneId = "operation_record_target";
		target.m_sDisplayName = "Operation Record Target";
		target.m_sOwnerFactionKey = "US";
		target.m_vPosition = "1600 20 1600";
		state.m_aZones.Insert(target);
		return state;
	}

	protected bool TransitionChanged(HST_OperationTransitionResult result)
	{
		return result && result.m_bAccepted && result.m_bStateChanged && !result.m_bAlreadyApplied;
	}

	protected bool PositionsMatch(vector left, vector right)
	{
		return Math.AbsFloat(left[0] - right[0]) < 0.01
			&& Math.AbsFloat(left[1] - right[1]) < 0.01
			&& Math.AbsFloat(left[2] - right[2]) < 0.01;
	}

	protected bool HasEvent(HST_CampaignState state, string eventId)
	{
		if (!state)
			return false;
		foreach (HST_CampaignEventState eventState : state.m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				return true;
		}
		return false;
	}
}
