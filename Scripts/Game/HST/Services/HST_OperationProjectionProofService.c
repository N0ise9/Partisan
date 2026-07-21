#ifdef ENABLE_DIAG
class HST_OperationProjectionProofReport
{
	bool m_bMovementExact;
	bool m_bHysteresisExact;
	bool m_bRosterTransferExact;
	bool m_bCombatRestoreExact;
	string m_sMovementEvidence;
	string m_sHysteresisEvidence;
	string m_sRosterTransferEvidence;
	string m_sCombatRestoreEvidence;
}

class HST_OperationProjectionProofFixture
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
	ref HST_ForceManifestState m_Manifest;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
	ref HST_OperationRecordState m_Operation;
}

class HST_OperationProjectionProofService
{
	HST_OperationProjectionProofReport Run()
	{
		HST_OperationProjectionProofReport report = new HST_OperationProjectionProofReport();
		ProveMovement(report);
		ProveHysteresis(report);
		ProveRosterTransfer(report);
		ProveCombatRestore(report);
		return report;
	}

	protected void ProveMovement(HST_OperationProjectionProofReport report)
	{
		HST_OperationProjectionProofFixture fixture = BuildFixture();
		if (!Ready(fixture))
		{
			report.m_sMovementEvidence = "strategic movement fixture unavailable";
			return;
		}
		HST_StrategicMovementService movement = new HST_StrategicMovementService();
		float progressBefore = fixture.m_Operation.m_fRouteProgressMeters;
		fixture.m_State.m_iElapsedSeconds += 100;
		bool routeStable = movement.InitializeExactPlayerQRFRoute(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Request,
			fixture.m_Manifest,
			fixture.m_Group);
		HST_StrategicMovementResult first = movement.AdvanceExactPlayerQRF(fixture.m_State, fixture.m_Operation, fixture.m_Group);
		bool firstAccepted = first && first.m_bAccepted;
		bool catchupBounded;
		bool distanceExact;
		if (firstAccepted)
		{
			catchupBounded = first.m_iAdvancedSeconds == HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
			float expectedMeters = HST_StrategicMovementService.EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND * first.m_iAdvancedSeconds;
			distanceExact = Math.AbsFloat(first.m_fAdvancedMeters - expectedMeters) < 0.1;
		}
		int guard;
		while (fixture.m_Operation.m_fRouteProgressMeters + 1.0 < fixture.m_Operation.m_fRouteTotalDistanceMeters && guard < 200)
		{
			fixture.m_State.m_iElapsedSeconds += HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
			if (!movement.InitializeExactPlayerQRFRoute(
				fixture.m_State,
				fixture.m_Operation,
				fixture.m_Request,
				fixture.m_Manifest,
				fixture.m_Group))
				routeStable = false;
			movement.AdvanceExactPlayerQRF(fixture.m_State, fixture.m_Operation, fixture.m_Group);
			guard++;
		}
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult arrived = operations.MarkVirtualOnStation(fixture.m_State, fixture.m_Request, fixture.m_Group);
		bool arrivalAccepted = arrived && arrived.m_bAccepted;
		bool onStation = fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
		bool combatClockExact = fixture.m_Operation.m_iVirtualCombatLastStepSecond == fixture.m_State.m_iElapsedSeconds;
		bool routeComplete = fixture.m_Operation.m_fRouteProgressMeters >= fixture.m_Operation.m_fRouteTotalDistanceMeters - 1.0;
		int expectedETA = HST_ForcePlanningService.ResolvePlayerSupportETASeconds(
			HST_ESupportRequestType.HST_SUPPORT_QRF,
			fixture.m_Issue.m_Quote.m_vSourcePosition,
			fixture.m_Issue.m_Quote.m_vTargetPosition);
		bool etaExact = fixture.m_Issue.m_Quote.m_iETASeconds == expectedETA
			&& fixture.m_Request.m_iETASeconds == expectedETA
			&& expectedETA == HST_ForcePlanningService.SUPPORT_QRF_ETA_SECONDS;
		bool movementStepExact = routeStable && firstAccepted && catchupBounded && distanceExact;
		bool arrivalExact = arrivalAccepted && onStation && combatClockExact && routeComplete;
		report.m_bMovementExact = movementStepExact && arrivalExact && etaExact;
		report.m_sMovementEvidence = string.Format(
			"first %1s/%2m from %3m",
			first && first.m_iAdvancedSeconds,
			first && Math.Round(first.m_fAdvancedMeters),
			Math.Round(progressBefore));
		report.m_sMovementEvidence = report.m_sMovementEvidence + string.Format(
			" | route %1/%2m | frozen QRF quote/request ETA %3/%4s",
			Math.Round(fixture.m_Operation.m_fRouteProgressMeters),
			Math.Round(fixture.m_Operation.m_fRouteTotalDistanceMeters),
			fixture.m_Issue.m_Quote.m_iETASeconds,
			fixture.m_Request.m_iETASeconds);
		report.m_sMovementEvidence = report.m_sMovementEvidence + string.Format(
			" | arrival/clock %1/%2",
			arrivalAccepted,
			combatClockExact);
	}

	protected void ProveHysteresis(HST_OperationProjectionProofReport report)
	{
		HST_OperationProjectionProofFixture fixture = BuildFixture();
		if (!Ready(fixture))
		{
			report.m_sHysteresisEvidence = "materialization fixture unavailable";
			return;
		}
		HST_MaterializationService service = new HST_MaterializationService();
		HST_OperationProjectionDecision enter = service.EvaluateExactPlayerQRFForProximity(fixture.m_Operation, true, true, 1800.0, 2160.0);
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		fixture.m_Operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		HST_OperationProjectionDecision band = service.EvaluateExactPlayerQRFForProximity(fixture.m_Operation, false, true, 1800.0, 2160.0);
		HST_OperationProjectionDecision leave = service.EvaluateExactPlayerQRFForProximity(fixture.m_Operation, false, false, 1800.0, 2160.0);
		bool entered = enter.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE;
		bool retained = band.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_RETAIN;
		bool left = leave.m_eDecision == HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_DEMATERIALIZE;
		bool separated = enter.m_fMaterializeOutDistanceMeters > enter.m_fMaterializeInDistanceMeters;
		report.m_bHysteresisExact = entered && retained && left && separated;
		report.m_sHysteresisEvidence = string.Format(
			"in/out %1/%2m",
			enter.m_fMaterializeInDistanceMeters,
			enter.m_fMaterializeOutDistanceMeters);
		report.m_sHysteresisEvidence = report.m_sHysteresisEvidence + string.Format(
			" | decisions %1/%2/%3",
			enter.m_eDecision,
			band.m_eDecision,
			leave.m_eDecision);
	}

	protected void ProveRosterTransfer(HST_OperationProjectionProofReport report)
	{
		HST_OperationProjectionProofFixture fixture = BuildFixture();
		if (!Ready(fixture))
		{
			report.m_sRosterTransferEvidence = "roster transfer fixture unavailable";
			return;
		}
		int initialLiving = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		string casualtySlot = fixture.m_Queue.SelectStrategicLivingMemberSlotId(fixture.m_Batch, fixture.m_Operation.m_iDeterministicSeed);
		HST_ForceSpawnQueueCallbackResult casualty = fixture.m_Queue.ConfirmStrategicMemberCasualty(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			casualtySlot,
			fixture.m_State.m_iElapsedSeconds + 30,
			"projection proof casualty");
		int livingAfterCasualty = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
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
			"projection proof materialize");
		HST_OperationTransitionResult contactBeforeHandoff = operations.RecordEngagement(
			fixture.m_State,
			fixture.m_Operation.m_sOperationId,
			HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT);
		HST_OperationTransitionResult engagedBeforeHandoff = operations.RecordEngagement(
			fixture.m_State,
			fixture.m_Operation.m_sOperationId,
			HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED);
		ApplySyntheticSuccessfulProjection(fixture);
		HST_OperationTransitionResult physical = operations.MarkPhysical(fixture.m_State, fixture.m_Request, fixture.m_Group, fixture.m_Batch);
		bool physicalEngagementClear = fixture.m_Operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		fixture.m_Group.m_vPosition = fixture.m_Group.m_vPosition + "45 0 30";
		fixture.m_State.m_iElapsedSeconds += 300;
		int expectedFoldCombatClock = fixture.m_State.m_iElapsedSeconds;
		HST_OperationTransitionResult dematerializing = operations.BeginDematerialization(
			fixture.m_State,
			fixture.m_Request,
			fixture.m_Group,
			"projection proof fold");
		HST_ForceSpawnQueueCallbackResult held = fixture.m_Queue.RequeueSuccessfulProjectionForStrategicHold(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			fixture.m_State.m_iElapsedSeconds + 1,
			fixture.m_State.m_iElapsedSeconds + 121);
		HST_StrategicMovementService movement = new HST_StrategicMovementService();
		bool routeSynced = movement.SyncRouteProgressFromPosition(fixture.m_Operation, fixture.m_Group.m_vPosition);
		HST_OperationTransitionResult virtualized = operations.CompleteDematerialization(
			fixture.m_State,
			fixture.m_Request,
			fixture.m_Group,
			fixture.m_Batch,
			"projection proof fold complete");
		int finalLiving = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		bool casualtyAccepted = casualty && casualty.m_bAccepted;
		bool casualtyCountExact = initialLiving > 1 && livingAfterCasualty == initialLiving - 1;
		bool releaseAccepted = release && release.m_bAccepted;
		bool materializingAccepted = materializing && materializing.m_bAccepted;
		bool abstractContactEntered = contactBeforeHandoff && contactBeforeHandoff.m_bAccepted;
		bool abstractEngagementEntered = engagedBeforeHandoff && engagedBeforeHandoff.m_bAccepted;
		bool physicalAccepted = physical && physical.m_bAccepted;
		bool dematerializingAccepted = dematerializing && dematerializing.m_bAccepted;
		bool holdAccepted = held && held.m_bAccepted;
		bool virtualAccepted = virtualized && virtualized.m_bAccepted;
		bool casualtyExact = casualtyAccepted && casualtyCountExact;
		bool materializationExact = releaseAccepted && materializingAccepted && abstractContactEntered;
		materializationExact = materializationExact && abstractEngagementEntered && physicalAccepted && physicalEngagementClear;
		bool foldCombatClockExact = fixture.m_Operation.m_iVirtualCombatLastStepSecond == expectedFoldCombatClock;
		bool foldExact = dematerializingAccepted && holdAccepted && routeSynced;
		foldExact = foldExact && virtualAccepted && foldCombatClockExact;
		bool rosterHeld = fixture.m_Batch.m_bStrategicProjectionHeld;
		bool livingExact = finalLiving == livingAfterCasualty;
		bool oneCasualty = fixture.m_Queue.CountConfirmedCasualtyMemberSlots(fixture.m_Batch) == 1;
		bool rosterExact = rosterHeld && livingExact && oneCasualty;
		bool virtualState = fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		bool strategicAuthority = fixture.m_Operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		bool authorityExact = virtualState && strategicAuthority;
		report.m_bRosterTransferExact = casualtyExact && materializationExact && foldExact && rosterExact && authorityExact;
		report.m_sRosterTransferEvidence = string.Format(
			"living %1 -> %2 -> %3",
			initialLiving,
			livingAfterCasualty,
			finalLiving);
		report.m_sRosterTransferEvidence = report.m_sRosterTransferEvidence + string.Format(
			" | casualty %1 | physical/fold %2/%3",
			casualtySlot,
			physicalAccepted,
			dematerializingAccepted);
		report.m_sRosterTransferEvidence = report.m_sRosterTransferEvidence + string.Format(
			" | handoff clear %1 | fold clock %2 | reprojections %3",
			physicalEngagementClear,
			foldCombatClockExact,
			fixture.m_Batch.m_iReprojectionCount);
	}

	protected void ProveCombatRestore(HST_OperationProjectionProofReport report)
	{
		HST_OperationProjectionProofFixture fixture = BuildFixture();
		if (!Ready(fixture))
		{
			report.m_sCombatRestoreEvidence = "virtual combat fixture unavailable";
			return;
		}
		fixture.m_Operation.m_fRouteProgressMeters = fixture.m_Operation.m_fRouteTotalDistanceMeters;
		fixture.m_Operation.m_vStrategicPosition = fixture.m_Operation.m_vRouteEndPosition;
		fixture.m_Group.m_vPosition = fixture.m_Operation.m_vStrategicPosition;
		HST_OperationService operations = new HST_OperationService();
		operations.MarkVirtualOnStation(fixture.m_State, fixture.m_Request, fixture.m_Group);
		int contactStartSecond = fixture.m_State.m_iElapsedSeconds;
		HST_GarrisonState hostile = new HST_GarrisonState();
		hostile.m_sGarrisonId = "projection_proof_hostile";
		hostile.m_sZoneId = fixture.m_Request.m_sTargetZoneId;
		hostile.m_sFactionKey = "US";
		hostile.m_iInfantryCount = 8;
		fixture.m_State.m_aGarrisons.Insert(hostile);
		int livingBefore = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		fixture.m_State.m_iElapsedSeconds += 600;
		HST_VirtualCombatService virtualCombat = new HST_VirtualCombatService();
		HST_VirtualCombatResult combat = virtualCombat.TickExactPlayerQRF(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Request,
			fixture.m_Manifest,
			fixture.m_Batch,
			fixture.m_Group,
			fixture.m_Queue);
		int livingAfter = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_OperationRecordState restoredOperation;
		HST_ForceSpawnResultState restoredBatch;
		if (restored)
		{
			restoredOperation = restored.FindOperation(fixture.m_Operation.m_sOperationId);
			restoredBatch = restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
		}
		int restoredLiving;
		if (restoredBatch)
			restoredLiving = fixture.m_Queue.CountStrategicLivingMemberSlots(restoredBatch);
		bool combatAccepted = combat && combat.m_bAccepted;
		bool boundedSteps;
		int expectedCombatClock;
		if (combatAccepted)
		{
			boundedSteps = combat.m_iProcessedSteps == HST_VirtualCombatService.MAX_COMBAT_STEPS_PER_TICK;
			expectedCombatClock = contactStartSecond + HST_VirtualCombatService.COMBAT_STEP_SECONDS * combat.m_iProcessedSteps;
		}
		bool combatClockExact = fixture.m_Operation.m_iVirtualCombatLastStepSecond == expectedCombatClock;
		bool survivorCountPlausible;
		if (combatAccepted)
			survivorCountPlausible = livingAfter >= livingBefore - combat.m_iProcessedSteps && livingAfter > 0;
		bool cadence = combatAccepted && boundedSteps && combatClockExact && survivorCountPlausible;
		bool schemaExact = restored && restored.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION;
		bool restoreRecordsPresent = restoredOperation && restoredBatch;
		bool restoredVirtual;
		bool restoredStrategic;
		bool restoredHeld;
		bool restoredRoster;
		bool restoredStepIndex;
		if (restoreRecordsPresent)
		{
			restoredVirtual = restoredOperation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
			restoredStrategic = restoredOperation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
			restoredHeld = restoredBatch.m_bStrategicProjectionHeld;
			restoredRoster = restoredLiving == livingAfter;
			restoredStepIndex = restoredOperation.m_iVirtualCombatStepIndex == fixture.m_Operation.m_iVirtualCombatStepIndex;
		}
		bool restoreAuthority = restoredVirtual && restoredStrategic && restoredHeld;
		bool restoreExact = schemaExact && restoreRecordsPresent && restoreAuthority && restoredRoster && restoredStepIndex;
		report.m_bCombatRestoreExact = cadence && restoreExact;
		report.m_sCombatRestoreEvidence = string.Format(
			"steps %1/%2 | clock %3",
			combat && combat.m_iProcessedSteps,
			HST_VirtualCombatService.MAX_COMBAT_STEPS_PER_TICK,
			fixture.m_Operation.m_iVirtualCombatLastStepSecond);
		report.m_sCombatRestoreEvidence = report.m_sCombatRestoreEvidence + string.Format(
			" | living %1 -> %2 -> %3",
			livingBefore,
			livingAfter,
			restoredLiving);
		report.m_sCombatRestoreEvidence = report.m_sCombatRestoreEvidence + string.Format(
			" | hostile/schema %1/%2 | virtual/held %3/%4",
			hostile.m_iInfantryCount,
			restored && restored.m_iSchemaVersion,
			restoredOperation && restoredOperation.m_eMaterializationState,
			restoredBatch && restoredBatch.m_bStrategicProjectionHeld);
	}

	protected HST_OperationProjectionProofFixture BuildFixture()
	{
		HST_OperationProjectionProofFixture fixture = new HST_OperationProjectionProofFixture();
		fixture.m_State = new HST_CampaignState();
		fixture.m_State.m_iCampaignSeed = 5050;
		fixture.m_State.m_iElapsedSeconds = 100;
		fixture.m_State.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		fixture.m_State.m_bHQDeployed = true;
		fixture.m_State.m_vHQPosition = "1000 20 1000";
		fixture.m_State.m_iWarLevel = 3;
		fixture.m_State.m_iFactionMoney = 1000;
		fixture.m_State.m_iHR = 50;
		HST_FactionPoolState pool = new HST_FactionPoolState();
		pool.m_sFactionKey = "FIA";
		fixture.m_State.m_aFactionPools.Insert(pool);
		HST_ZoneState source = new HST_ZoneState();
		source.m_sZoneId = "projection_proof_source";
		source.m_sOwnerFactionKey = "FIA";
		source.m_vPosition = "1000 20 1000";
		fixture.m_State.m_aZones.Insert(source);
		HST_ZoneState target = new HST_ZoneState();
		target.m_sZoneId = "projection_proof_target";
		target.m_sOwnerFactionKey = "US";
		target.m_vPosition = "2200 20 1800";
		fixture.m_State.m_aZones.Insert(target);
		fixture.m_Preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		fixture.m_Economy = new HST_EconomyService();
		fixture.m_Ledger = new HST_ResourceLedgerService();
		fixture.m_Planning = new HST_ForcePlanningService();
		fixture.m_Queue = new HST_ForceSpawnQueueService();
		fixture.m_Support = new HST_SupportRequestService();
		fixture.m_Support.SetExactForceAuthorityServices(fixture.m_Queue, fixture.m_Ledger, fixture.m_Economy);
		fixture.m_Issue = fixture.m_Planning.IssuePlayerSupportQuote(
			fixture.m_State,
			fixture.m_Preset,
			"projection_proof_actor",
			HST_ESupportRequestType.HST_SUPPORT_QRF,
			target.m_sZoneId,
			target.m_vPosition,
			"projection_proof_issue",
			true);
		if (!fixture.m_Issue || !fixture.m_Issue.m_bSuccess)
			return fixture;
		fixture.m_Manifest = fixture.m_Issue.m_Manifest;
		fixture.m_Confirmation = fixture.m_Planning.ConfirmPlayerSupportQuote(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Economy,
			fixture.m_Support,
			fixture.m_Ledger,
			"projection_proof_actor",
			fixture.m_Issue.m_Quote.m_sQuoteId,
			"projection_proof_confirm");
		fixture.m_Request = fixture.m_State.FindSupportRequest(fixture.m_Issue.m_Quote.m_sSupportRequestId);
		if (!fixture.m_Request)
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

	protected void ApplySyntheticSuccessfulProjection(HST_OperationProjectionProofFixture fixture)
	{
		if (!fixture || !fixture.m_Batch)
			return;
		fixture.m_Batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
		fixture.m_Batch.m_iSuccessfulHandoffCount = 1;
		foreach (HST_ForceSpawnSlotResultState slot : fixture.m_Batch.m_aSlotResults)
		{
			if (!slot || (slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED && slot.m_bCasualtyConfirmed))
				continue;
			slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
			slot.m_sEntityId = "projection_proof_entity_" + slot.m_sSlotId;
			slot.m_sNativeGroupId = "projection_proof_native_group";
			slot.m_bAliveVerified = true;
			if (slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
				slot.m_bEverAlive = true;
		}
	}

	protected bool Ready(HST_OperationProjectionProofFixture fixture)
	{
		if (!fixture || !fixture.m_Confirmation || !fixture.m_Confirmation.m_bSuccess)
			return false;
		if (!fixture.m_Request || !fixture.m_Manifest || !fixture.m_Batch)
			return false;
		if (!fixture.m_Batch.m_bStrategicProjectionHeld)
			return false;
		return fixture.m_Group && fixture.m_Operation;
	}
}
#endif
