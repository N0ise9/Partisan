class HST_PlayerSearchDestroyOperationProofReport
{
	bool m_bPlanningExact;
	bool m_bOperationRouteExact;
	bool m_bProjectionContinuityExact;
	bool m_bReturnToAssignmentExact;
	bool m_bRecallSettlementExact;
	bool m_bLegacyIsolationExact;
	bool m_bRestoreQuarantineExact;
	string m_sPlanningEvidence;
	string m_sOperationRouteEvidence;
	string m_sProjectionContinuityEvidence;
	string m_sReturnToAssignmentEvidence;
	string m_sRecallSettlementEvidence;
	string m_sLegacyIsolationEvidence;
	string m_sRestoreQuarantineEvidence;
}

class HST_PlayerSearchDestroyOperationProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_EconomyService m_Economy;
	ref HST_ResourceLedgerService m_Ledger;
	ref HST_ForcePlanningService m_Planning;
	ref HST_ForceSpawnQueueService m_Queue;
	ref HST_SupportRequestService m_Support;
	ref HST_ForceQuoteResult m_Issue;
	ref HST_ForceQuoteResult m_IssueReplay;
	ref HST_ForceConfirmationResult m_Confirmation;
	ref HST_ForceConfirmationResult m_ConfirmationReplay;
	ref HST_SupportRequestState m_Request;
	ref HST_ForceManifestState m_Manifest;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
	ref HST_OperationRecordState m_Operation;
	int m_iInitialMoney;
	int m_iInitialHR;
}

class HST_PlayerSearchDestroyOperationProofService
{
	static const string PROOF_ACTOR_ID = "search_destroy_proof_actor";
	static const string PROOF_ISSUE_REQUEST_ID = "search_destroy_proof_issue";
	static const string PROOF_CONFIRM_REQUEST_ID = "search_destroy_proof_confirm";
	static const string PROOF_SOURCE_ZONE_ID = "search_destroy_proof_source";
	static const string PROOF_TARGET_ZONE_ID = "search_destroy_proof_target";

	HST_PlayerSearchDestroyOperationProofReport Run()
	{
		HST_PlayerSearchDestroyOperationProofReport report = new HST_PlayerSearchDestroyOperationProofReport();
		ProvePlanningAndOperation(report);
		ProveProjectionContinuity(report);
		ProveRecallSettlement(report);
		ProveLegacyIsolation(report);
		ProveMalformedCurrentContractQuarantine(report);
		return report;
	}

	protected void ProvePlanningAndOperation(HST_PlayerSearchDestroyOperationProofReport report)
	{
		HST_PlayerSearchDestroyOperationProofFixture fixture = BuildFixture(true);
		if (!Ready(fixture, true))
		{
			report.m_sPlanningEvidence = BuildFixtureFailureEvidence(fixture);
			report.m_sOperationRouteEvidence = report.m_sPlanningEvidence;
			return;
		}

		HST_ForceQuoteState quote = fixture.m_Issue.m_Quote;
		HST_ForceManifestState manifest = fixture.m_Manifest;
		HST_ForceCatalogService catalog = new HST_ForceCatalogService();
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		HST_ForceGroupCatalogEntry selected = integrity.SelectExactPlayerSupportGroup(
			catalog.BuildGroupCatalog(quote.m_sFactionKey),
			manifest.m_iDeterministicSeed,
			quote.m_iExpectedWarLevel,
			HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY);
		HST_ForceManifestGroupState groupElement;
		if (manifest.m_aGroups.Count() > 0)
			groupElement = manifest.m_aGroups[0];

		bool quoteReplayIdentityExact = fixture.m_IssueReplay && fixture.m_IssueReplay.m_bSuccess
			&& !fixture.m_IssueReplay.m_bStateChanged
			&& fixture.m_IssueReplay.m_Quote == quote
			&& fixture.m_IssueReplay.m_Manifest == manifest;
		bool quoteReplayCountExact = fixture.m_State.m_aForceQuotes.Count() == 1
			&& fixture.m_State.m_aForceManifests.Count() == 1;
		bool quoteReplayExact = quoteReplayIdentityExact && quoteReplayCountExact;
		bool confirmationReplayResultExact = fixture.m_ConfirmationReplay
			&& fixture.m_ConfirmationReplay.m_bSuccess
			&& fixture.m_ConfirmationReplay.m_bAlreadyApplied
			&& !fixture.m_ConfirmationReplay.m_bStateChanged;
		bool confirmationReplayCountExact = fixture.m_State.m_aSupportRequests.Count() == 1
			&& fixture.m_State.m_aResourceTransactions.Count() == 2;
		bool confirmationReplayExact = confirmationReplayResultExact && confirmationReplayCountExact;
		bool quoteFamilyExact = quote.m_eSupportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY
			&& quote.m_sQuoteKind == HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_SEARCH_DESTROY
			&& quote.m_sPolicyId == HST_ForcePlanningService.SUPPORT_SEARCH_DESTROY_POLICY_ID;
		bool quoteCapabilityExact = quote.m_sCapabilityId == HST_ForcePlanningService.SUPPORT_SEARCH_DESTROY_CAPABILITY_ID
			&& quote.m_sAssetProfileId == HST_ForcePlanningService.SUPPORT_SEARCH_DESTROY_ASSET_PROFILE_ID
			&& fixture.m_Request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;
		bool familyExact = quoteFamilyExact && quoteCapabilityExact;
		bool selectedGroupExact = selected && groupElement
			&& selected.m_sEntryId == groupElement.m_sCatalogEntryId;
		bool rosterCountsExact = manifest.m_aGroups.Count() == 1
			&& manifest.m_aMembers.Count() == manifest.m_iAcceptedMemberCount
			&& selected && manifest.m_iAcceptedMemberCount == selected.m_aMemberSlots.Count();
		bool rosterExact = selectedGroupExact && rosterCountsExact && ManifestIsInfantryOnly(manifest);
		bool quotedResourceExact = manifest.m_iMoneyCost == HST_ForcePlanningService.SUPPORT_SEARCH_DESTROY_MONEY_COST
			&& quote.m_iMoneyCost == manifest.m_iMoneyCost
			&& manifest.m_iHRCost == manifest.m_aMembers.Count()
			&& quote.m_iHRCost == manifest.m_iHRCost;
		bool balanceExact = fixture.m_State.m_iFactionMoney == fixture.m_iInitialMoney - manifest.m_iMoneyCost
			&& fixture.m_State.m_iHR == fixture.m_iInitialHR - manifest.m_iHRCost
			&& TransactionsCommittedExact(fixture);
		bool resourceExact = quotedResourceExact && balanceExact;
		report.m_bPlanningExact = quoteReplayExact && confirmationReplayExact && familyExact && rosterExact && resourceExact;
		report.m_sPlanningEvidence = string.Format(
			"quote/confirm replay %1/%2 | family %3 | roster %4 members | cost $%5 HR %6 | infantry-only %7",
			quoteReplayExact,
			confirmationReplayExact,
			familyExact,
			manifest.m_aMembers.Count(),
			manifest.m_iMoneyCost,
			manifest.m_iHRCost,
			ManifestIsInfantryOnly(manifest));

		HST_OperationService operations = new HST_OperationService();
		string operationFailure = operations.ValidateExactPlayerSearchDestroy(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Request,
			quote,
			manifest);
		int expectedETA = HST_ForcePlanningService.ResolvePlayerSupportETASeconds(
			HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY,
			quote.m_vSourcePosition,
			quote.m_vTargetPosition);
		float expectedDistance = Distance2D(quote.m_vSourcePosition, quote.m_vTargetPosition);
		bool operationContractExact = operationFailure.IsEmpty()
			&& fixture.m_Operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_SEARCH_DESTROY
			&& fixture.m_Operation.m_iContractVersion == HST_OperationService.EXACT_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION
			&& fixture.m_Request.m_iOperationContractVersion == HST_OperationService.EXACT_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION;
		bool operationPolicyExact = fixture.m_Operation.m_sAssignmentKind == HST_OperationService.EXACT_PLAYER_SEARCH_DESTROY_ASSIGNMENT_KIND
			&& fixture.m_Operation.m_sRecallPolicyId == HST_OperationService.EXACT_PLAYER_SEARCH_DESTROY_RECALL_POLICY
			&& fixture.m_Operation.m_sSettlementPolicyId == HST_OperationService.EXACT_PLAYER_SEARCH_DESTROY_SETTLEMENT_POLICY;
		int projectionContractVersion = fixture.m_Operation.m_iProjectionContractVersion;
		fixture.m_Operation.m_iProjectionContractVersion = 0;
		string missingProjectionFailure = operations.ValidateExactPlayerSearchDestroy(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Request,
			quote,
			manifest);
		fixture.m_Operation.m_iProjectionContractVersion = projectionContractVersion;
		bool missingProjectionRejected = !missingProjectionFailure.IsEmpty();
		bool operationExact = operationContractExact && operationPolicyExact
			&& missingProjectionRejected;
		bool routeContractExact = fixture.m_Operation.m_iProjectionContractVersion == HST_StrategicMovementService.EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION
			&& fixture.m_Operation.m_iRouteVersion == HST_StrategicMovementService.DIRECT_ROUTE_VERSION
			&& PositionsMatch(fixture.m_Operation.m_vRouteStartPosition, quote.m_vSourcePosition)
			&& PositionsMatch(fixture.m_Operation.m_vRouteEndPosition, quote.m_vTargetPosition);
		bool routeScheduleExact = Math.AbsFloat(fixture.m_Operation.m_fRouteTotalDistanceMeters - expectedDistance) < 0.1
			&& fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& quote.m_iETASeconds == expectedETA;
		bool routeProjectionExact = fixture.m_Batch.m_iExpectedSlotCount == manifest.m_aMembers.Count() + 1
			&& fixture.m_Batch.m_bStrategicProjectionHeld;
		bool routeExact = routeContractExact && routeScheduleExact && routeProjectionExact;
		report.m_bOperationRouteExact = operationExact && routeExact;
		report.m_sOperationRouteEvidence = string.Format(
			"operation/type/contract %1/%2/%3 | route %4m ETA %5s | outbound/held %6/%7 | validation/missing-projection rejection %8/%9",
			operationExact,
			fixture.m_Operation.m_eType,
			fixture.m_Operation.m_iContractVersion,
			Math.Round(fixture.m_Operation.m_fRouteTotalDistanceMeters),
			quote.m_iETASeconds,
			fixture.m_Operation.m_eDutyState,
			fixture.m_Batch.m_bStrategicProjectionHeld,
			operationFailure.IsEmpty(),
			missingProjectionRejected);
	}

	protected void ProveProjectionContinuity(HST_PlayerSearchDestroyOperationProofReport report)
	{
		HST_PlayerSearchDestroyOperationProofFixture fixture = BuildFixture(true);
		if (!Ready(fixture, true))
		{
			report.m_sProjectionContinuityEvidence = BuildFixtureFailureEvidence(fixture);
			report.m_sReturnToAssignmentEvidence = report.m_sProjectionContinuityEvidence;
			return;
		}

		HST_OperationService operations = new HST_OperationService();
		fixture.m_Operation.m_fRouteProgressMeters = fixture.m_Operation.m_fRouteTotalDistanceMeters;
		fixture.m_Operation.m_vStrategicPosition = fixture.m_Operation.m_vRouteEndPosition;
		fixture.m_Group.m_vPosition = fixture.m_Operation.m_vStrategicPosition;
		HST_OperationTransitionResult arrived = operations.MarkVirtualOnStation(fixture.m_State, fixture.m_Request, fixture.m_Group);
		int livingBeforeCombat = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		fixture.m_State.m_iElapsedSeconds += HST_VirtualCombatService.COMBAT_STEP_SECONDS
			* HST_VirtualCombatService.MAX_COMBAT_STEPS_PER_TICK;
		HST_VirtualCombatService virtualCombat = new HST_VirtualCombatService();
		HST_VirtualCombatResult combat = virtualCombat.TickExactPlayerQRF(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Request,
			fixture.m_Manifest,
			fixture.m_Batch,
			fixture.m_Group,
			fixture.m_Queue);
		int livingAfterCombat = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		bool combatTransitionExact = arrived && arrived.m_bAccepted
			&& combat && combat.m_bAccepted;
		bool combatCadenceExact = combat
			&& combat.m_iProcessedSteps == HST_VirtualCombatService.MAX_COMBAT_STEPS_PER_TICK
			&& combat.m_iFriendlyCasualties > 0;
		bool combatRosterExact = combat
			&& livingAfterCombat == livingBeforeCombat - combat.m_iFriendlyCasualties
			&& livingAfterCombat > 1;
		bool combatExact = combatTransitionExact && combatCadenceExact && combatRosterExact;

		bool firstPhysical = MaterializeFixture(fixture, "search-destroy proof first materialization");
		int livingAtFirstPhysical = fixture.m_Queue.CountDurableLivingMemberSlots(fixture.m_Batch);
		fixture.m_State.m_iElapsedSeconds += 60;
		HST_OperationTransitionResult folding = operations.BeginDematerialization(
			fixture.m_State,
			fixture.m_Request,
			fixture.m_Group,
			"search-destroy proof physical fold");
		HST_ForceSpawnQueueCallbackResult held = HoldSuccessfulProjection(fixture);
		string physicalCasualtySlot = fixture.m_Queue.SelectStrategicLivingMemberSlotId(
			fixture.m_Batch,
			fixture.m_Operation.m_iDeterministicSeed + 611);
		HST_ForceSpawnQueueCallbackResult physicalCasualty = fixture.m_Queue.ConfirmStrategicMemberCasualty(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			physicalCasualtySlot,
			fixture.m_State.m_iElapsedSeconds,
			"confirmed casualty at physical-to-virtual handoff");
		HST_StrategicMovementService movement = new HST_StrategicMovementService();
		bool routeSynced = movement.SyncRouteProgressFromPosition(fixture.m_Operation, fixture.m_Group.m_vPosition);
		HST_OperationTransitionResult virtualized = operations.CompleteDematerialization(
			fixture.m_State,
			fixture.m_Request,
			fixture.m_Group,
			fixture.m_Batch,
			"search-destroy proof physical fold complete");
		SetVirtualFixtureBookkeeping(fixture);
		int livingAfterPhysicalFold = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		int casualtiesAfterPhysicalFold = fixture.m_Queue.CountConfirmedCasualtyMemberSlots(fixture.m_Batch);
		bool foldBegan = folding && folding.m_bAccepted;
		bool holdAccepted = held && held.m_bAccepted;
		bool virtualizationAccepted = virtualized && virtualized.m_bAccepted;
		bool physicalFoldTransitionsExact = firstPhysical && foldBegan
			&& holdAccepted && virtualizationAccepted;
		bool physicalFoldCasualtyExact = livingAtFirstPhysical == livingAfterCombat
			&& physicalCasualty && physicalCasualty.m_bAccepted
			&& livingAfterPhysicalFold == livingAtFirstPhysical - 1
			&& casualtiesAfterPhysicalFold == combat.m_iFriendlyCasualties + 1;
		bool physicalFoldAuthorityExact = routeSynced
			&& fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
		bool physicalFoldExact = physicalFoldTransitionsExact
			&& physicalFoldCasualtyExact
			&& physicalFoldAuthorityExact;

		bool restored = RestoreFixture(fixture);
		int livingAfterRestore = -1;
		if (restored)
			livingAfterRestore = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		bool secondPhysical = restored && MaterializeFixture(fixture, "search-destroy proof survivor re-entry");
		int livingAfterReentry = -1;
		if (secondPhysical)
			livingAfterReentry = fixture.m_Queue.CountDurableLivingMemberSlots(fixture.m_Batch);
		bool restoredRosterExact = restored
			&& livingAfterRestore == livingAfterPhysicalFold
			&& secondPhysical
			&& livingAfterReentry == livingAfterPhysicalFold;
		bool restoredAuthorityExact = fixture.m_Queue.CountConfirmedCasualtyMemberSlots(fixture.m_Batch) == casualtiesAfterPhysicalFold
			&& fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			&& fixture.m_Operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		bool restoreReentryExact = restoredRosterExact && restoredAuthorityExact;

		report.m_bProjectionContinuityExact = combatExact && physicalFoldExact && restoreReentryExact;
		report.m_sProjectionContinuityEvidence = string.Format(
			"virtual combat steps/losses %1/%2 | living %3 -> %4 -> %5 | physical fold/re-entry %6/%7 | restored %8 | casualties %9",
			combat && combat.m_iProcessedSteps,
			combat && combat.m_iFriendlyCasualties,
			livingBeforeCombat,
			livingAfterCombat,
			livingAfterPhysicalFold,
			firstPhysical,
			secondPhysical,
			restored,
			casualtiesAfterPhysicalFold);

		string assignmentZoneId = fixture.m_Operation.m_sAssignmentZoneId;
		vector assignmentPosition = fixture.m_Operation.m_vAssignmentPosition;
		fixture.m_Group.m_vPosition = assignmentPosition + "260 0 120";
		fixture.m_State.m_iElapsedSeconds += 60;
		HST_OperationTransitionResult returnFolding = operations.BeginDematerialization(
			fixture.m_State,
			fixture.m_Request,
			fixture.m_Group,
			"search-destroy proof folded away from assignment");
		HST_ForceSpawnQueueCallbackResult returnHeld = HoldSuccessfulProjection(fixture);
		bool returnRouteSynced = movement.SyncRouteProgressFromPosition(fixture.m_Operation, fixture.m_Group.m_vPosition);
		HST_OperationTransitionResult returning = operations.CompleteDematerialization(
			fixture.m_State,
			fixture.m_Request,
			fixture.m_Group,
			fixture.m_Batch,
			"search-destroy proof return-to-assignment handoff");
		SetVirtualFixtureBookkeeping(fixture);
		bool returnFoldBegan = returnFolding && returnFolding.m_bAccepted;
		bool returnHoldAccepted = returnHeld && returnHeld.m_bAccepted;
		bool returnTransitionAccepted = returning && returning.m_bAccepted;
		bool returnFoldExact = returnFoldBegan && returnHoldAccepted
			&& returnRouteSynced && returnTransitionAccepted;
		bool returnDutyExact = fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ASSIGNMENT
			&& fixture.m_Operation.m_eResumeDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ASSIGNMENT
			&& fixture.m_Operation.m_sAssignmentZoneId == assignmentZoneId
			&& PositionsMatch(fixture.m_Operation.m_vAssignmentPosition, assignmentPosition);
		bool returnTargetExact = fixture.m_Operation.m_sTacticalTargetZoneId == assignmentZoneId
			&& PositionsMatch(fixture.m_Operation.m_vTacticalTargetPosition, assignmentPosition)
			&& PositionsMatch(fixture.m_Operation.m_vRouteEndPosition, assignmentPosition);
		bool returnTransitionExact = returnFoldExact && returnDutyExact && returnTargetExact;

		bool movementAccepted = true;
		int movementGuard;
		while (fixture.m_Operation.m_fRouteProgressMeters + HST_StrategicMovementService.ARRIVAL_EPSILON_METERS
			< fixture.m_Operation.m_fRouteTotalDistanceMeters && movementGuard < 20)
		{
			fixture.m_State.m_iElapsedSeconds += HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
			HST_StrategicMovementResult advance = movement.AdvanceExactPlayerQRF(
				fixture.m_State,
				fixture.m_Operation,
				fixture.m_Group);
			if (!advance || !advance.m_bAccepted)
				movementAccepted = false;
			movementGuard++;
		}
		HST_OperationTransitionResult returned = operations.MarkVirtualOnStation(
			fixture.m_State,
			fixture.m_Request,
			fixture.m_Group);
		bool returnArrivalAccepted = movementAccepted && movementGuard < 20
			&& returned && returned.m_bAccepted;
		bool returnPositionExact = fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			&& PositionsMatch(fixture.m_Operation.m_vStrategicPosition, assignmentPosition)
			&& PositionsMatch(fixture.m_Group.m_vPosition, assignmentPosition);
		bool returnRosterExact = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch) == livingAfterPhysicalFold;
		bool returnArrivalExact = returnArrivalAccepted && returnPositionExact
			&& returnRosterExact;
		string physicalRestoreReturnEvidence;
		bool physicalRestoreReturnExact = ProvePhysicalSaveAwayReturn(
			physicalRestoreReturnEvidence);
		report.m_bReturnToAssignmentExact = returnTransitionExact && returnArrivalExact
			&& physicalRestoreReturnExact;
		report.m_sReturnToAssignmentEvidence = string.Format(
			"fold/held/return %1/%2/%3 | immutable target %4 | duty %5 -> %6 | steps %7 | survivors %8 | physical-save return %9",
			returnFolding && returnFolding.m_bAccepted,
			returnHeld && returnHeld.m_bAccepted,
			returning && returning.m_bAccepted,
			fixture.m_Operation.m_sAssignmentZoneId == assignmentZoneId && PositionsMatch(fixture.m_Operation.m_vAssignmentPosition, assignmentPosition),
			HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ASSIGNMENT,
			fixture.m_Operation.m_eDutyState,
			movementGuard,
			fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch),
			physicalRestoreReturnEvidence);
	}

	protected bool ProvePhysicalSaveAwayReturn(out string evidence)
	{
		evidence = "fixture unavailable";
		HST_PlayerSearchDestroyOperationProofFixture fixture = BuildFixture(true);
		if (!Ready(fixture, true))
		{
			evidence = BuildFixtureFailureEvidence(fixture);
			return false;
		}

		HST_OperationService operations = new HST_OperationService();
		fixture.m_Operation.m_fRouteProgressMeters = fixture.m_Operation.m_fRouteTotalDistanceMeters;
		fixture.m_Operation.m_vStrategicPosition = fixture.m_Operation.m_vRouteEndPosition;
		fixture.m_Group.m_vPosition = fixture.m_Operation.m_vStrategicPosition;
		HST_OperationTransitionResult arrived = operations.MarkVirtualOnStation(
			fixture.m_State,
			fixture.m_Request,
			fixture.m_Group);
		bool physical = arrived && arrived.m_bAccepted
			&& MaterializeFixture(fixture, "search-destroy physical save-away proof");
		vector assignment = fixture.m_Operation.m_vAssignmentPosition;
		vector awayPosition = assignment + "260 0 120";
		if (physical)
		{
			fixture.m_Group.m_vPosition = awayPosition;
			fixture.m_Group.m_vSourcePosition = awayPosition;
		}
		bool restored = physical && RestoreFixture(fixture);
		bool dutyExact = restored
			&& fixture.m_Operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ASSIGNMENT
			&& fixture.m_Operation.m_eResumeDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ASSIGNMENT;
		bool routeExact = restored
			&& fixture.m_Operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& fixture.m_Operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& PositionsMatch(fixture.m_Operation.m_vRouteStartPosition, awayPosition)
			&& PositionsMatch(fixture.m_Operation.m_vRouteEndPosition, assignment);
		bool groupExact = restored
			&& PositionsMatch(fixture.m_Group.m_vPosition, awayPosition)
			&& PositionsMatch(fixture.m_Group.m_vSourcePosition, awayPosition)
			&& PositionsMatch(fixture.m_Group.m_vTargetPosition, assignment)
			&& fixture.m_Batch.m_bStrategicProjectionHeld;
		evidence = string.Format(
			"physical/restored %1/%2 | duty %3 | route %4 | group/hold %5/%6",
			physical,
			restored,
			fixture.m_Operation && fixture.m_Operation.m_eDutyState,
			routeExact,
			groupExact,
			fixture.m_Batch && fixture.m_Batch.m_bStrategicProjectionHeld);
		return dutyExact && routeExact && groupExact;
	}

	protected void ProveRecallSettlement(HST_PlayerSearchDestroyOperationProofReport report)
	{
		HST_PlayerSearchDestroyOperationProofFixture fixture = BuildFixture(false);
		if (!Ready(fixture, false))
		{
			report.m_sRecallSettlementEvidence = BuildFixtureFailureEvidence(fixture);
			return;
		}

		int moneyBeforeRecall = fixture.m_State.m_iFactionMoney;
		HST_SupportRecallResult recall = fixture.m_Support.RecallSupportRequestDetailed(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Economy,
			null,
			fixture.m_Request.m_sRequestId,
			true);
		HST_ResourceTransactionState money = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sMoneyTransactionId);
		HST_ResourceTransactionState hr = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sHRTransactionId);
		HST_SupportRecallResult replay = fixture.m_Support.RecallSupportRequestDetailed(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Economy,
			null,
			fixture.m_Request.m_sRequestId,
			true);
		bool recallResultExact = recall && recall.m_bAccepted
			&& recall.m_bStateChanged && recall.m_bTerminal
			&& recall.m_sDisposition == "recalled_before_deploy";
		bool replayExact = replay && replay.m_bAccepted
			&& replay.m_bAlreadyApplied && !replay.m_bStateChanged;
		bool requestSettlementExact = fixture.m_Request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED
			&& fixture.m_Request.m_sResolutionKind == "recalled_before_deploy"
			&& fixture.m_Request.m_iRefundedHR == fixture.m_Request.m_iHRCost;
		bool balanceSettlementExact = fixture.m_State.m_iFactionMoney == moneyBeforeRecall
			&& fixture.m_State.m_iHR == fixture.m_iInitialHR;
		bool moneySettlementExact = money
			&& money.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED
			&& money.m_iRefundedAmount == 0;
		bool hrSettlementExact = hr
			&& hr.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED
			&& hr.m_iRefundedAmount == fixture.m_Request.m_iHRCost;
		bool operationSettlementExact = fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& fixture.m_Operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_RECALLED
			&& fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		string quoteId = fixture.m_Issue.m_Quote.m_sQuoteId;
		fixture.m_State.m_iElapsedSeconds = Math.Max(
			fixture.m_State.m_iElapsedSeconds,
			fixture.m_Issue.m_Quote.m_iAcceptedAtSecond
				+ HST_ForceSettlementArchiveService.MIN_ACCEPTED_RECORD_RETENTION_SECONDS);
		HST_ForceSettlementArchiveService archive = new HST_ForceSettlementArchiveService();
		HST_ForceSettlementArchiveResult archiveResult = archive.ArchiveSettledRecords(
			fixture.m_State);
		HST_ForceSettlementTombstoneState tombstone
			= fixture.m_State.FindForceSettlementTombstone(quoteId);
		HST_ForceConfirmationResult archiveConfirmationReplay
			= fixture.m_Planning.ConfirmPlayerSupportQuote(
				fixture.m_State,
				fixture.m_Preset,
				fixture.m_Economy,
				fixture.m_Support,
				fixture.m_Ledger,
				PROOF_ACTOR_ID,
				quoteId,
				PROOF_CONFIRM_REQUEST_ID);
		HST_ForceQuoteResult archiveIssueReplay = fixture.m_Planning.IssuePlayerSupportQuote(
			fixture.m_State,
			fixture.m_Preset,
			PROOF_ACTOR_ID,
			HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY,
			PROOF_TARGET_ZONE_ID,
			"1800 20 1600",
			PROOF_ISSUE_REQUEST_ID,
			true);
		bool archiveFamilyExact = tombstone
			&& tombstone.m_sQuoteKind
				== HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_SEARCH_DESTROY
			&& tombstone.m_eSupportType
				== HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY
			&& tombstone.m_sPolicyId
				== HST_ForcePlanningService.SUPPORT_SEARCH_DESTROY_POLICY_ID
			&& tombstone.m_iOperationContractVersion
				== HST_OperationService.EXACT_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION;
		bool archiveReceiptExact = tombstone
			&& tombstone.m_sSettlementKind == fixture.m_Request.m_sResolutionKind
			&& tombstone.m_sOperationSettlementId
				== HST_OperationService.BuildSettlementId(
					fixture.m_Request.m_sOperationId,
					fixture.m_Request.m_sResolutionKind)
			&& tombstone.m_aTransactions.Count() == 2;
		bool archiveCompactedExact = archiveResult && archiveResult.m_iArchivedCount == 1
			&& fixture.m_State.m_aForceQuotes.Count() == 0
			&& fixture.m_State.m_aForceManifests.Count() == 0
			&& fixture.m_State.m_aResourceTransactions.Count() == 0
			&& fixture.m_State.FindOperation(fixture.m_Request.m_sOperationId) == null;
		bool archiveReplayExact = archiveConfirmationReplay
			&& archiveConfirmationReplay.m_bSuccess
			&& archiveConfirmationReplay.m_bAlreadyApplied
			&& archiveConfirmationReplay.m_SupportRequest == fixture.m_Request
			&& archiveIssueReplay && archiveIssueReplay.m_bSuccess
			&& !archiveIssueReplay.m_bStateChanged
			&& archiveIssueReplay.m_Quote
			&& archiveIssueReplay.m_Quote.m_sQuoteKind
				== HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_SEARCH_DESTROY;
		string expiredArchiveEvidence;
		bool expiredArchivePruneExact = ProveExpiredArchivePairPruning(
			fixture,
			tombstone,
			expiredArchiveEvidence);
		string foldedRecallEvidence;
		bool foldedRecallExact = ProveSyntheticFoldImmediateRecall(foldedRecallEvidence);
		bool exact = recallResultExact && replayExact && requestSettlementExact;
		exact = exact && balanceSettlementExact && moneySettlementExact;
		exact = exact && hrSettlementExact && operationSettlementExact;
		exact = exact && archiveFamilyExact && archiveReceiptExact;
		exact = exact && archiveCompactedExact && archiveReplayExact;
		exact = exact && expiredArchivePruneExact;
		exact = exact && foldedRecallExact;
		report.m_bRecallSettlementExact = exact;
		report.m_sRecallSettlementEvidence = string.Format(
			"accepted/terminal/replay %1/%2/%3 | disposition %4 | money retained %5 | HR refund %6/%7 | operation result %8",
			recall && recall.m_bAccepted,
			recall && recall.m_bTerminal,
			replay && replay.m_bAlreadyApplied,
			recall && recall.m_sDisposition,
			money && money.m_iRefundedAmount == 0,
			hr && hr.m_iRefundedAmount,
			fixture.m_Request.m_iHRCost,
			fixture.m_Operation.m_eTerminalResult);
		report.m_sRecallSettlementEvidence += string.Format(
			" | archive family/receipt/compact/replay %1/%2/%3/%4 | expiry %5 | synthetic fold-immediate-recall %6",
			archiveFamilyExact,
			archiveReceiptExact,
			archiveCompactedExact,
			archiveReplayExact,
			expiredArchiveEvidence,
			foldedRecallEvidence);
	}

	protected bool ProveExpiredArchivePairPruning(
		HST_PlayerSearchDestroyOperationProofFixture fixture,
		HST_ForceSettlementTombstoneState tombstone,
		out string evidence)
	{
		evidence = "fixture unavailable";
		if (!fixture || !fixture.m_State || !fixture.m_Request || !tombstone)
			return false;

		string quarantineEvidence;
		bool quarantineRetained = ProveQuarantinedArchiveRetention(
			fixture,
			tombstone,
			quarantineEvidence);
		string requestId = fixture.m_Request.m_sRequestId;
		string quoteId = tombstone.m_sQuoteId;
		int archiveSecond = tombstone.m_iArchivedAtSecond;
		FillArchiveCapacity(fixture.m_State, archiveSecond, "valid");
		fixture.m_State.m_iElapsedSeconds = archiveSecond
			+ HST_ForceSettlementArchiveService.MIN_TOMBSTONE_RETENTION_SECONDS
			+ HST_ForceSettlementArchiveService.MAX_TOMBSTONE_ROWS + 1;
		HST_ForceSettlementArchiveService archive = new HST_ForceSettlementArchiveService();
		HST_ForceSettlementArchiveResult prune = archive.ArchiveSettledRecords(fixture.m_State);
		bool pairPruned = prune && prune.m_iPrunedTombstoneCount == 1
			&& fixture.m_State.FindForceSettlementTombstone(quoteId) == null
			&& fixture.m_State.FindSupportRequest(requestId) == null;

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		bool restoreExact = restored && restored.FindSupportRequest(requestId) == null
			&& restored.FindForceSettlementTombstone(quoteId) == null;
		evidence = string.Format(
			"pair/pruned/restore %1/%2/%3 | quarantine %4",
			pairPruned,
			prune && prune.m_iPrunedTombstoneCount,
			restoreExact,
			quarantineEvidence);
		return pairPruned && restoreExact && quarantineRetained;
	}

	protected bool ProveQuarantinedArchiveRetention(
		HST_PlayerSearchDestroyOperationProofFixture fixture,
		HST_ForceSettlementTombstoneState tombstone,
		out string evidence)
	{
		evidence = "fixture unavailable";
		HST_CampaignSaveData copy = new HST_CampaignSaveData();
		copy.Capture(fixture.m_State);
		HST_CampaignState working = copy.Restore();
		if (!working)
			return false;
		HST_SupportRequestState request = working.FindSupportRequest(
			fixture.m_Request.m_sRequestId);
		HST_ForceSettlementTombstoneState workingTombstone
			= working.FindForceSettlementTombstone(tombstone.m_sQuoteId);
		if (!request || !workingTombstone)
			return false;
		request.m_iMoneyCost++;
		HST_CampaignSaveData corrupt = new HST_CampaignSaveData();
		corrupt.Capture(working);
		HST_CampaignState quarantined = corrupt.Restore();
		if (!quarantined)
			return false;
		request = quarantined.FindSupportRequest(fixture.m_Request.m_sRequestId);
		workingTombstone = quarantined.FindForceSettlementTombstone(tombstone.m_sQuoteId);
		bool quarantineEstablished = request && workingTombstone
			&& request.m_iOperationContractVersion
				== HST_OperationService.QUARANTINED_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION
			&& workingTombstone.m_iOperationContractVersion
				== HST_OperationService.QUARANTINED_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION;
		int archiveSecond;
		if (workingTombstone)
			archiveSecond = workingTombstone.m_iArchivedAtSecond;
		FillArchiveCapacity(quarantined, archiveSecond, "quarantine");
		quarantined.m_iElapsedSeconds = archiveSecond
			+ HST_ForceSettlementArchiveService.MIN_TOMBSTONE_RETENTION_SECONDS
			+ HST_ForceSettlementArchiveService.MAX_TOMBSTONE_ROWS + 1;
		HST_ForceSettlementArchiveService archive = new HST_ForceSettlementArchiveService();
		HST_ForceSettlementArchiveResult prune = archive.ArchiveSettledRecords(quarantined);
		request = quarantined.FindSupportRequest(fixture.m_Request.m_sRequestId);
		workingTombstone = quarantined.FindForceSettlementTombstone(tombstone.m_sQuoteId);
		bool evidenceRetained = request && workingTombstone
			&& request.m_iOperationContractVersion
				== HST_OperationService.QUARANTINED_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION
			&& workingTombstone.m_iOperationContractVersion
				== HST_OperationService.QUARANTINED_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION;
		evidence = string.Format(
			"established/retained/pruned-other %1/%2/%3",
			quarantineEstablished,
			evidenceRetained,
			prune && prune.m_iPrunedTombstoneCount);
		return quarantineEstablished && evidenceRetained
			&& prune && prune.m_iPrunedTombstoneCount == 1;
	}

	protected void FillArchiveCapacity(
		HST_CampaignState state,
		int archiveSecond,
		string label)
	{
		if (!state)
			return;
		int index = state.m_aForceSettlementTombstones.Count();
		while (index < HST_ForceSettlementArchiveService.MAX_TOMBSTONE_ROWS)
		{
			HST_ForceSettlementTombstoneState retained = new HST_ForceSettlementTombstoneState();
			retained.m_sQuoteId = string.Format(
				"search_destroy_%1_retained_archive_%2",
				label,
				index);
			retained.m_iArchivedAtSecond = archiveSecond + index + 1;
			state.m_aForceSettlementTombstones.Insert(retained);
			index++;
		}
	}

	protected bool ProveSyntheticFoldImmediateRecall(out string evidence)
	{
		evidence = "fixture unavailable";
		HST_PlayerSearchDestroyOperationProofFixture fixture = BuildFixture(true);
		if (!Ready(fixture, true))
		{
			evidence = BuildFixtureFailureEvidence(fixture);
			return false;
		}

		HST_OperationService operations = new HST_OperationService();
		bool physical = MaterializeFixture(fixture, "synthetic fold-recall proof materialization");
		HST_OperationTransitionResult folding;
		HST_ForceSpawnQueueCallbackResult held;
		HST_ForceSpawnQueueCallbackResult casualty;
		HST_OperationTransitionResult virtualized;
		if (physical)
		{
			fixture.m_State.m_iElapsedSeconds += 60;
			folding = operations.BeginDematerialization(
				fixture.m_State,
				fixture.m_Request,
				fixture.m_Group,
				"synthetic fold-recall proof handoff");
			held = HoldSuccessfulProjection(fixture);
			string casualtySlot = fixture.m_Queue.SelectStrategicLivingMemberSlotId(
				fixture.m_Batch,
				fixture.m_Operation.m_iDeterministicSeed + 911);
			casualty = fixture.m_Queue.ConfirmStrategicMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				casualtySlot,
				fixture.m_State.m_iElapsedSeconds,
				"synthetic physical-fold casualty before immediate recall");
			virtualized = operations.CompleteDematerialization(
				fixture.m_State,
				fixture.m_Request,
				fixture.m_Group,
				fixture.m_Batch,
				"synthetic fold-recall proof virtual handoff");
			SetVirtualFixtureBookkeeping(fixture);
		}

		int livingAfterFold = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		bool foldExact = physical && folding && folding.m_bAccepted
			&& held && held.m_bAccepted && casualty && casualty.m_bAccepted
			&& virtualized && virtualized.m_bAccepted
			&& livingAfterFold == fixture.m_Request.m_iHRCost - 1;
		bool operationSnapshotExact = fixture.m_Operation.m_iLastVirtualFriendlyCount
			== livingAfterFold;
		int moneyAfterPurchase = fixture.m_State.m_iFactionMoney;
		HST_SupportRecallResult recall = fixture.m_Support.RecallSupportRequestDetailed(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Economy,
			null,
			fixture.m_Request.m_sRequestId,
			true);
		HST_GarrisonService garrisons = new HST_GarrisonService();
		for (int tick = 0; tick < 3; tick++)
		{
			if (fixture.m_Request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED
				|| fixture.m_Request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
				break;
			fixture.m_State.m_iElapsedSeconds++;
			fixture.m_Support.Tick(
				fixture.m_State,
				fixture.m_Preset,
				garrisons,
				null,
				null,
				null,
				fixture.m_Economy,
				null);
		}

		HST_ResourceTransactionState hr = fixture.m_State.FindResourceTransaction(
			fixture.m_Request.m_sHRTransactionId);
		bool recallExact = recall && recall.m_bAccepted
			&& fixture.m_Request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED
			&& fixture.m_Request.m_iRefundedHR == livingAfterFold
			&& fixture.m_State.m_iFactionMoney == moneyAfterPurchase
			&& fixture.m_State.m_iHR == fixture.m_iInitialHR
				- fixture.m_Request.m_iHRCost + livingAfterFold
			&& hr && hr.m_iRefundedAmount == livingAfterFold
			&& fixture.m_Operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_RECALLED;
		evidence = string.Format(
			"fold/casualty/snapshot %1/%2/%3 | living/refund %4/%5 | terminal %6",
			foldExact,
			casualty && casualty.m_bAccepted,
			operationSnapshotExact,
			livingAfterFold,
			fixture.m_Request.m_iRefundedHR,
			fixture.m_Operation.m_eTerminalResult);
		return foldExact && operationSnapshotExact && recallExact;
	}

	protected void ProveLegacyIsolation(HST_PlayerSearchDestroyOperationProofReport report)
	{
		HST_CampaignState state = CreateProofState();
		HST_SupportRequestState legacy = new HST_SupportRequestState();
		legacy.m_sRequestId = "legacy_search_destroy_contract_zero";
		legacy.m_sOperationId = HST_StableIdService.BuildOperationId("support", legacy.m_sRequestId);
		legacy.m_sQuoteId = "legacy_incidental_search_quote";
		legacy.m_sManifestId = "legacy_incidental_search_manifest";
		legacy.m_sFactionKey = "FIA";
		legacy.m_eType = HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;
		legacy.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE;
		legacy.m_sSourceZoneId = PROOF_SOURCE_ZONE_ID;
		legacy.m_sTargetZoneId = PROOF_TARGET_ZONE_ID;
		legacy.m_vSourcePosition = "1000 20 1000";
		legacy.m_vTargetPosition = "1800 20 1600";
		legacy.m_iRequestedAtSecond = state.m_iElapsedSeconds;
		legacy.m_iETASeconds = 3600;
		legacy.m_iOperationContractVersion = 0;
		legacy.m_bPlayerRequested = true;
		legacy.m_sPhysicalizationMode = HST_SupportRequestService.EXACT_PLAYER_SUPPORT_MODE;
		state.m_aSupportRequests.Insert(legacy);
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(state);
		HST_CampaignState restored = saveData.Restore();
		HST_SupportRequestState restoredLegacy;
		if (restored)
			restoredLegacy = restored.FindSupportRequest(legacy.m_sRequestId);
		HST_SupportRequestService support = new HST_SupportRequestService();
		bool legacyTickChanged;
		if (restored && restoredLegacy)
			legacyTickChanged = support.Tick(
				restored,
				HST_DefaultCatalog.CreateVanillaEveronPreset(),
				new HST_GarrisonService());
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult ignored;
		if (restoredLegacy)
			ignored = operations.BeginRecall(restored, restoredLegacy, restoredLegacy.m_vSourcePosition);
		bool legacyIdentityExact = restored && restoredLegacy
			&& restoredLegacy.m_iOperationContractVersion == 0
			&& restoredLegacy.m_sQuoteId == legacy.m_sQuoteId
			&& restoredLegacy.m_sManifestId == legacy.m_sManifestId
			&& restoredLegacy.m_sPhysicalizationMode == HST_SupportRequestService.EXACT_PLAYER_SUPPORT_MODE
			&& !HST_OperationService.RequiresOperation(restoredLegacy);
		bool legacyRuntimeStayedLegacy = restoredLegacy
			&& restoredLegacy.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE
			&& restoredLegacy.m_sFailureReason.IsEmpty()
			&& !legacyTickChanged;
		bool legacyRowsExact;
		if (restored)
		{
			legacyRowsExact = restored.m_aOperations.Count() == 0
				&& restored.m_aForceQuotes.Count() == 0
				&& restored.m_aForceManifests.Count() == 0
				&& restored.m_aResourceTransactions.Count() == 0;
		}
		bool legacyTransitionIgnored = ignored && ignored.m_bAccepted && ignored.m_bAlreadyApplied
			&& !ignored.m_bStateChanged && !ignored.m_Operation;
		bool exact = legacyIdentityExact && legacyRowsExact
			&& legacyRuntimeStayedLegacy && legacyTransitionIgnored;
		report.m_bLegacyIsolationExact = exact;
		report.m_sLegacyIsolationEvidence = string.Format(
			"contract %1 | requires typed operation %2 | incidental links/mode preserved %3 | operations/quotes/manifests/tx %4/%5/%6/%7 | legacy tick/transition %8/%9",
			restoredLegacy && restoredLegacy.m_iOperationContractVersion,
			restoredLegacy && HST_OperationService.RequiresOperation(restoredLegacy),
			legacyIdentityExact,
			restored && restored.m_aOperations.Count(),
			restored && restored.m_aForceQuotes.Count(),
			restored && restored.m_aForceManifests.Count(),
			restored && restored.m_aResourceTransactions.Count(),
			legacyRuntimeStayedLegacy,
			ignored && ignored.m_bAlreadyApplied);
	}

	protected void ProveMalformedCurrentContractQuarantine(
		HST_PlayerSearchDestroyOperationProofReport report)
	{
		HST_PlayerSearchDestroyOperationProofFixture fixture = BuildFixture(true);
		if (!Ready(fixture, true))
		{
			report.m_sRestoreQuarantineEvidence = BuildFixtureFailureEvidence(fixture);
			return;
		}

		int expectedMoney = fixture.m_State.m_iFactionMoney;
		int expectedHR = fixture.m_State.m_iHR;
		string requestId = fixture.m_Request.m_sRequestId;
		string originalManifestHash = fixture.m_Manifest.m_sManifestHash;
		fixture.m_Manifest.m_sManifestHash = originalManifestHash + "_corrupt";
		bool corruptionApplied = !originalManifestHash.IsEmpty()
			&& fixture.m_Manifest.m_sManifestHash != originalManifestHash;

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		string quarantineEvidence;
		bool quarantined = AssertMalformedCurrentContractQuarantined(
			restored,
			requestId,
			expectedMoney,
			expectedHR,
			quarantineEvidence);
		report.m_bRestoreQuarantineExact = corruptionApplied && quarantined;
		report.m_sRestoreQuarantineEvidence = string.Format(
			"manifest hash corrupted %1 | %2",
			corruptionApplied,
			quarantineEvidence);
	}

	bool AssertMalformedCurrentContractQuarantined(
		HST_CampaignState restored,
		string requestId,
		int expectedMoney,
		int expectedHR,
		out string evidence)
	{
		evidence = "malformed current-contract quarantine state is unavailable";
		if (!restored || requestId.IsEmpty())
			return false;
		HST_SupportRequestState request = restored.FindSupportRequest(requestId);
		if (!request)
			return false;
		HST_OperationRecordState operation = restored.FindOperation(request.m_sOperationId);
		HST_ForceQuoteState quote = restored.FindForceQuote(request.m_sQuoteId);
		HST_ForceManifestState manifest = restored.FindForceManifest(request.m_sManifestId);
		HST_ResourceTransactionState money = restored.FindResourceTransaction(request.m_sMoneyTransactionId);
		HST_ResourceTransactionState hr = restored.FindResourceTransaction(request.m_sHRTransactionId);
		HST_ForceSpawnResultState batch = restored.FindForceSpawnResult(request.m_sSpawnResultId);
		HST_ActiveGroupState group = restored.FindActiveGroup(request.m_sGroupId);
		bool requestQuarantined = request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY
			&& request.m_iOperationContractVersion == HST_OperationService.QUARANTINED_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION
			&& request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED
			&& !request.m_sFailureReason.IsEmpty()
			&& HST_OperationService.RequiresOperation(request);
		bool planningQuarantined = quote && manifest
			&& quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_CANCELLED;
		bool operationQuarantined = operation
			&& operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_SEARCH_DESTROY
			&& operation.m_iContractVersion == HST_OperationService.QUARANTINED_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION;
		bool batchTerminal = batch
			&& batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED;
		bool groupQuarantined = group && group.m_sRuntimeStatus.Contains("quarant")
			&& restored.IsQuarantinedActiveGroup(group)
			&& !restored.IsOperationalActiveGroup(group)
			&& !restored.IsCombatPresentActiveGroup(group);
		bool ledgerPreserved = money && hr
			&& money.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED
			&& hr.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED
			&& money.m_iRefundedAmount == 0 && hr.m_iRefundedAmount == 0;
		bool balancesPreserved = restored.m_iFactionMoney == expectedMoney && restored.m_iHR == expectedHR;
		bool exact = requestQuarantined && planningQuarantined && operationQuarantined;
		exact = exact && batchTerminal && groupQuarantined && ledgerPreserved && balancesPreserved;
		evidence = string.Format(
			"request contract/status %1/%2 | quote %3 | operation contract %4 | batch %5 | group %6 | ledger retained %7 | balances %8/%9",
			request.m_iOperationContractVersion,
			request.m_eStatus,
			quote && quote.m_eStatus,
			operation && operation.m_iContractVersion,
			batch && batch.m_eStatus,
			group && group.m_sRuntimeStatus,
			ledgerPreserved,
			restored.m_iFactionMoney,
			restored.m_iHR);
		return exact;
	}

	protected HST_PlayerSearchDestroyOperationProofFixture BuildFixture(bool enqueueProjection)
	{
		HST_PlayerSearchDestroyOperationProofFixture fixture = new HST_PlayerSearchDestroyOperationProofFixture();
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
			PROOF_ACTOR_ID,
			HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY,
			PROOF_TARGET_ZONE_ID,
			"1800 20 1600",
			PROOF_ISSUE_REQUEST_ID,
			true);
		if (!fixture.m_Issue || !fixture.m_Issue.m_bSuccess || !fixture.m_Issue.m_Quote || !fixture.m_Issue.m_Manifest)
			return fixture;
		fixture.m_Manifest = fixture.m_Issue.m_Manifest;
		fixture.m_IssueReplay = fixture.m_Planning.IssuePlayerSupportQuote(
			fixture.m_State,
			fixture.m_Preset,
			PROOF_ACTOR_ID,
			HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY,
			PROOF_TARGET_ZONE_ID,
			"1800 20 1600",
			PROOF_ISSUE_REQUEST_ID,
			true);
		fixture.m_Confirmation = fixture.m_Planning.ConfirmPlayerSupportQuote(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Economy,
			fixture.m_Support,
			fixture.m_Ledger,
			PROOF_ACTOR_ID,
			fixture.m_Issue.m_Quote.m_sQuoteId,
			PROOF_CONFIRM_REQUEST_ID);
		fixture.m_ConfirmationReplay = fixture.m_Planning.ConfirmPlayerSupportQuote(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Economy,
			fixture.m_Support,
			fixture.m_Ledger,
			PROOF_ACTOR_ID,
			fixture.m_Issue.m_Quote.m_sQuoteId,
			PROOF_CONFIRM_REQUEST_ID);
		fixture.m_Request = fixture.m_State.FindSupportRequest(fixture.m_Issue.m_Quote.m_sSupportRequestId);
		if (!fixture.m_Request)
			return fixture;
		fixture.m_Operation = fixture.m_State.FindOperation(fixture.m_Request.m_sOperationId);
		if (!enqueueProjection)
			return fixture;

		HST_ForceSpawnQueueEnqueueResult enqueue = fixture.m_Support.EnqueueAcceptedExactPlayerSupportProjection(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Request,
			fixture.m_Issue.m_Quote.m_vSourcePosition,
			fixture.m_Issue.m_Quote.m_vTargetPosition);
		if (enqueue)
			fixture.m_Batch = enqueue.m_Batch;
		if (fixture.m_Batch)
			fixture.m_Group = fixture.m_State.FindActiveGroup(fixture.m_Batch.m_sProjectionId);
		fixture.m_Operation = fixture.m_State.FindOperation(fixture.m_Request.m_sOperationId);
		return fixture;
	}

	protected HST_CampaignState CreateProofState()
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iCampaignSeed = 6060;
		state.m_iElapsedSeconds = 100;
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		state.m_bHQDeployed = true;
		state.m_vHQPosition = "1000 20 1000";
		state.m_iWarLevel = 3;
		state.m_iFactionMoney = 1200;
		state.m_iHR = 60;
		HST_FactionPoolState pool = new HST_FactionPoolState();
		pool.m_sFactionKey = "FIA";
		state.m_aFactionPools.Insert(pool);
		HST_ZoneState source = new HST_ZoneState();
		source.m_sZoneId = PROOF_SOURCE_ZONE_ID;
		source.m_sDisplayName = "Search Destroy Proof Source";
		source.m_sOwnerFactionKey = "FIA";
		source.m_vPosition = "1000 20 1000";
		state.m_aZones.Insert(source);
		HST_ZoneState target = new HST_ZoneState();
		target.m_sZoneId = PROOF_TARGET_ZONE_ID;
		target.m_sDisplayName = "Search Destroy Proof Target";
		target.m_sOwnerFactionKey = "US";
		target.m_vPosition = "1800 20 1600";
		state.m_aZones.Insert(target);
		HST_GarrisonState hostile = new HST_GarrisonState();
		hostile.m_sGarrisonId = "search_destroy_proof_hostile";
		hostile.m_sZoneId = PROOF_TARGET_ZONE_ID;
		hostile.m_sFactionKey = "US";
		hostile.m_iInfantryCount = 8;
		state.m_aGarrisons.Insert(hostile);
		return state;
	}

	protected bool MaterializeFixture(HST_PlayerSearchDestroyOperationProofFixture fixture, string reason)
	{
		if (!Ready(fixture, true) || !fixture.m_Batch.m_bStrategicProjectionHeld)
			return false;
		HST_ForceSpawnQueueCallbackResult release = fixture.m_Queue.ReleaseStrategicProjectionForMaterialization(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			fixture.m_State.m_iElapsedSeconds,
			fixture.m_State.m_iElapsedSeconds + 120);
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult materializing = operations.MarkMaterializingFromVirtual(
			fixture.m_State,
			fixture.m_Request,
			fixture.m_Group,
			fixture.m_Batch,
			reason);
		ApplySyntheticSuccessfulProjection(fixture);
		HST_OperationTransitionResult physical = operations.MarkPhysical(
			fixture.m_State,
			fixture.m_Request,
			fixture.m_Group,
			fixture.m_Batch);
		bool releaseExact = release && release.m_bAccepted;
		bool materializingExact = materializing && materializing.m_bAccepted;
		bool physicalExact = physical && physical.m_bAccepted;
		return releaseExact && materializingExact && physicalExact;
	}

	protected HST_ForceSpawnQueueCallbackResult HoldSuccessfulProjection(HST_PlayerSearchDestroyOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_State || !fixture.m_Manifest || !fixture.m_Batch)
			return null;
		return fixture.m_Queue.RequeueSuccessfulProjectionForStrategicHold(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			fixture.m_State.m_iElapsedSeconds,
			fixture.m_State.m_iElapsedSeconds + 120);
	}

	protected void ApplySyntheticSuccessfulProjection(HST_PlayerSearchDestroyOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_Batch || !fixture.m_Group || !fixture.m_Request)
			return;
		fixture.m_Batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
		fixture.m_Batch.m_iSuccessfulHandoffCount = 1;
		foreach (HST_ForceSpawnSlotResultState slot : fixture.m_Batch.m_aSlotResults)
		{
			if (!slot || (slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED && slot.m_bCasualtyConfirmed))
				continue;
			slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
			slot.m_sEntityId = "search_destroy_proof_entity_" + slot.m_sSlotId;
			slot.m_sNativeGroupId = "search_destroy_proof_native_group";
			slot.m_bAliveVerified = true;
			if (slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
				slot.m_bEverAlive = true;
		}
		int living = fixture.m_Queue.CountDurableLivingMemberSlots(fixture.m_Batch);
		fixture.m_Group.m_bSpawnAttempted = true;
		fixture.m_Group.m_bSpawnedEntity = true;
		fixture.m_Group.m_bSpawnCompleted = true;
		fixture.m_Group.m_bEverPopulated = true;
		fixture.m_Group.m_sRuntimeEntityId = "search_destroy_proof_native_group";
		fixture.m_Group.m_sRuntimeStatus = "support_active";
		fixture.m_Group.m_iDurableLivingInfantryCount = living;
		fixture.m_Group.m_iLastSeenAliveCount = living;
		fixture.m_Group.m_iSurvivorInfantryCount = living;
		fixture.m_Group.m_iSpawnedAgentCount = living;
		fixture.m_Request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE;
		fixture.m_Request.m_bPhysicalized = true;
		fixture.m_Request.m_sRuntimeStatus = "physical_active";
	}

	protected void SetVirtualFixtureBookkeeping(HST_PlayerSearchDestroyOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_Group || !fixture.m_Request || !fixture.m_Batch || !fixture.m_Operation)
			return;
		int living = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		fixture.m_Group.m_bSpawnedEntity = false;
		fixture.m_Group.m_sRuntimeEntityId = "";
		fixture.m_Group.m_sRuntimeStatus = "support_virtual";
		fixture.m_Group.m_iSpawnedAgentCount = 0;
		fixture.m_Group.m_iDurableLivingInfantryCount = living;
		fixture.m_Group.m_iLastSeenAliveCount = living;
		fixture.m_Group.m_iSurvivorInfantryCount = living;
		fixture.m_Request.m_bPhysicalized = false;
		fixture.m_Request.m_bAbstractResolved = fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
		if (fixture.m_Request.m_bAbstractResolved)
			fixture.m_Request.m_sRuntimeStatus = "exact_virtual_on_station";
		else
			fixture.m_Request.m_sRuntimeStatus = "exact_virtual_outbound";
	}

	protected bool RestoreFixture(HST_PlayerSearchDestroyOperationProofFixture fixture)
	{
		if (!Ready(fixture, true))
			return false;
		string requestId = fixture.m_Request.m_sRequestId;
		string manifestId = fixture.m_Manifest.m_sManifestId;
		string resultId = fixture.m_Batch.m_sResultId;
		string groupId = fixture.m_Group.m_sGroupId;
		string operationId = fixture.m_Operation.m_sOperationId;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		if (!restored)
			return false;
		fixture.m_State = restored;
		fixture.m_Request = restored.FindSupportRequest(requestId);
		fixture.m_Manifest = restored.FindForceManifest(manifestId);
		fixture.m_Batch = restored.FindForceSpawnResult(resultId);
		fixture.m_Group = restored.FindActiveGroup(groupId);
		fixture.m_Operation = restored.FindOperation(operationId);
		bool recordsPresent = fixture.m_Request && fixture.m_Manifest
			&& fixture.m_Batch && fixture.m_Group && fixture.m_Operation;
		if (!recordsPresent)
			return false;
		bool contractExact = fixture.m_Request.m_iOperationContractVersion == HST_OperationService.EXACT_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION
			&& fixture.m_Operation.m_iContractVersion == HST_OperationService.EXACT_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION;
		return contractExact && fixture.m_Batch.m_bStrategicProjectionHeld;
	}

	protected bool Ready(HST_PlayerSearchDestroyOperationProofFixture fixture, bool requireProjection)
	{
		if (!fixture || !fixture.m_State || !fixture.m_Preset || !fixture.m_Economy)
			return false;
		if (!fixture.m_Ledger || !fixture.m_Planning || !fixture.m_Queue || !fixture.m_Support)
			return false;
		bool issueReady = fixture.m_Issue && fixture.m_Issue.m_bSuccess
			&& fixture.m_IssueReplay && fixture.m_IssueReplay.m_bSuccess;
		if (!issueReady)
			return false;
		bool confirmationReady = fixture.m_Confirmation && fixture.m_Confirmation.m_bSuccess
			&& fixture.m_ConfirmationReplay && fixture.m_ConfirmationReplay.m_bSuccess;
		if (!confirmationReady)
			return false;
		if (!fixture.m_Request || !fixture.m_Manifest || !fixture.m_Operation)
			return false;
		if (!requireProjection)
			return true;
		return fixture.m_Batch && fixture.m_Group;
	}

	protected bool ManifestIsInfantryOnly(HST_ForceManifestState manifest)
	{
		if (!manifest || manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
			return false;
		bool memberCountsExact = manifest.m_aMembers.Count() > 0
			&& manifest.m_aMembers.Count() == manifest.m_iAcceptedMemberCount;
		bool noVehicleRows = manifest.m_iRequestedVehicleCount == 0
			&& manifest.m_iAcceptedVehicleCount == 0
			&& manifest.m_aVehicles.Count() == 0;
		bool noAssetCosts = manifest.m_aAssets.Count() == 0
			&& manifest.m_iEquipmentCost == 0
			&& manifest.m_iAttackResourceCost == 0
			&& manifest.m_iSupportResourceCost == 0;
		if (!memberCountsExact || !noVehicleRows || !noAssetCosts)
			return false;
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member)
				return false;
			bool noSeatAssignment = member.m_sAssignedVehicleSlotId.IsEmpty()
				&& member.m_sSeatRole.IsEmpty() && member.m_iSeatIndex == -1;
			bool memberCostExact = member.m_iHRCost == 1
				&& member.m_iMoneyCost == 0 && member.m_iEquipmentCost == 0;
			if (!noSeatAssignment || !memberCostExact)
				return false;
		}
		return true;
	}

	protected bool TransactionsCommittedExact(HST_PlayerSearchDestroyOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_Request || !fixture.m_Issue || !fixture.m_Issue.m_Quote || !fixture.m_Manifest)
			return false;
		HST_ResourceTransactionState money = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sMoneyTransactionId);
		HST_ResourceTransactionState hr = fixture.m_State.FindResourceTransaction(fixture.m_Request.m_sHRTransactionId);
		bool moneyExact = TransactionCommittedExact(
			money,
			fixture.m_Issue.m_Quote,
			fixture.m_Manifest,
			HST_ResourceLedgerService.RESOURCE_FACTION_MONEY,
			fixture.m_Manifest.m_iMoneyCost);
		bool hrExact = TransactionCommittedExact(
			hr,
			fixture.m_Issue.m_Quote,
			fixture.m_Manifest,
			HST_ResourceLedgerService.RESOURCE_HR,
			fixture.m_Manifest.m_iHRCost);
		return moneyExact && hrExact;
	}

	protected bool TransactionCommittedExact(
		HST_ResourceTransactionState transaction,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		string resourceType,
		int amount)
	{
		if (!transaction || !quote || !manifest)
			return false;
		bool identityExact = transaction.m_sQuoteId == quote.m_sQuoteId
			&& transaction.m_sManifestId == manifest.m_sManifestId
			&& transaction.m_sOperationId == quote.m_sOperationId;
		if (!identityExact)
			return false;
		bool commandResourceExact = transaction.m_sCommandRequestId == quote.m_sConfirmationRequestId
			&& transaction.m_sResourceType == resourceType
			&& transaction.m_iAmount == amount;
		bool settlementExact = transaction.m_iRefundedAmount == 0
			&& transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED;
		return commandResourceExact && settlementExact;
	}

	protected string BuildFixtureFailureEvidence(HST_PlayerSearchDestroyOperationProofFixture fixture)
	{
		string evidence = "exact search-and-destroy fixture unavailable";
		if (fixture && fixture.m_Issue && !fixture.m_Issue.m_sFailureReason.IsEmpty())
			evidence = evidence + ": issue " + fixture.m_Issue.m_sFailureReason;
		else if (fixture && fixture.m_Confirmation && !fixture.m_Confirmation.m_sFailureReason.IsEmpty())
			evidence = evidence + ": confirmation " + fixture.m_Confirmation.m_sFailureReason;
		return evidence;
	}

	protected bool PositionsMatch(vector left, vector right)
	{
		return Math.AbsFloat(left[0] - right[0]) < 0.01
			&& Math.AbsFloat(left[1] - right[1]) < 0.01
			&& Math.AbsFloat(left[2] - right[2]) < 0.01;
	}

	protected float Distance2D(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}
}
