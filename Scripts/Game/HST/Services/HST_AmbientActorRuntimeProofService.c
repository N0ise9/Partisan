#ifdef ENABLE_DIAG
class HST_AmbientActorRuntimeProofReport
{
	bool m_bPedestrianPathExact;
	bool m_bTrafficPathExact;
	bool m_bIllegalTransitionReadOnlyExact;
	bool m_bBoundedRecoveryExact;
	bool m_bProgressResetExact;
	bool m_bBudgetReadinessExact;
	bool m_bDeterministicEvidenceExact;
	bool m_bPanicRecoveryExact;
	string m_sPedestrianPathEvidence;
	string m_sTrafficPathEvidence;
	string m_sIllegalTransitionEvidence;
	string m_sBoundedRecoveryEvidence;
	string m_sProgressResetEvidence;
	string m_sBudgetReadinessEvidence;
	string m_sDeterministicEvidence;
	string m_sPanicRecoveryEvidence;

	bool AllExact()
	{
		return m_bPedestrianPathExact
			&& m_bTrafficPathExact
			&& m_bIllegalTransitionReadOnlyExact
			&& m_bBoundedRecoveryExact
			&& m_bProgressResetExact
			&& m_bBudgetReadinessExact
			&& m_bDeterministicEvidenceExact
			&& m_bPanicRecoveryExact;
	}

	string BuildReport()
	{
		return string.Format(
			"ambient actor runtime proof | all exact %1 | pedestrian %2 | traffic %3 | illegal read-only %4 | bounded recovery %5 | progress reset %6 | budget/readiness %7 | deterministic %8 | panic/recovery %9",
			AllExact(),
			m_bPedestrianPathExact,
			m_bTrafficPathExact,
			m_bIllegalTransitionReadOnlyExact,
			m_bBoundedRecoveryExact,
			m_bProgressResetExact,
			m_bBudgetReadinessExact,
			m_bDeterministicEvidenceExact,
			m_bPanicRecoveryExact);
	}
}

// Source-level deterministic proof: no world lookup, entity creation, deletion,
// clock access, or persistence is involved.
class HST_AmbientActorRuntimeProofService
{
	protected ref HST_AmbientActorRuntimeService m_Service
		= new HST_AmbientActorRuntimeService();

	HST_AmbientActorRuntimeProofReport Run()
	{
		HST_AmbientActorRuntimeProofReport report
			= new HST_AmbientActorRuntimeProofReport();
		ProvePedestrianPath(report);
		ProveTrafficPath(report);
		ProveIllegalTransitionReadOnly(report);
		ProveBoundedRecovery(report);
		ProveProgressReset(report);
		ProveBudgetAndReadiness(report);
		ProveDeterministicEvidence(report);
		ProvePanicRecovery(report);
		return report;
	}

	protected void ProvePedestrianPath(
		HST_AmbientActorRuntimeProofReport report)
	{
		HST_AmbientActorRuntimeRecord record = m_Service.CreateRecord(
			"pedestrian_runtime",
			"town_alpha",
			HST_AmbientActorRuntimeService.KIND_PEDESTRIAN,
			10,
			1,
			6101);
		bool initializing = m_Service.TryTransition(
			record,
			HST_AmbientActorRuntimeService.STATE_BEHAVIOR_INITIALIZING,
			"group_ready",
			11);
		bool wandering = m_Service.TryTransition(
			record,
			HST_AmbientActorRuntimeService.STATE_WANDERING,
			"waypoint_acknowledged",
			12);
		bool startupProtected = !m_Service.ShouldRecover(record, 16, 5, 10);
		bool stuckExact = !m_Service.ShouldRecover(record, 21, 5, 10)
			&& m_Service.ShouldRecover(record, 22, 5, 10);
		bool recovering = m_Service.BeginRecovery(
			record,
			22,
			2,
			3,
			"pedestrian_stuck");
		bool dueExact = !m_Service.IsRetryDue(record, 24)
			&& m_Service.IsRetryDue(record, 25);
		bool resumed = m_Service.TryTransition(
			record,
			HST_AmbientActorRuntimeService.STATE_WANDERING,
			"waypoint_reissued",
			25);

		report.m_bPedestrianPathExact = record
			&& initializing
			&& wandering
			&& startupProtected
			&& stuckExact
			&& recovering
			&& dueExact
			&& resumed
			&& record.m_sStateId
				== HST_AmbientActorRuntimeService.STATE_WANDERING
			&& record.m_iRecoveryCount == 1
			&& record.m_iStateChangedAtSecond == 25
			&& record.m_iLastProgressAtSecond == 25
			&& m_Service.IsBehaviorReady(record)
			&& m_Service.IsBudgetReservation(record);
		report.m_sPedestrianPathEvidence = string.Format(
			"state %1@%2 | recovery %3 | retry due 24/25 false/true %4 | ready/reserved %5/%6",
			ResolveState(record),
			ResolveStateSecond(record),
			ResolveRecoveryCount(record),
			dueExact,
			m_Service.IsBehaviorReady(record),
			m_Service.IsBudgetReservation(record));
	}

	protected void ProveTrafficPath(
		HST_AmbientActorRuntimeProofReport report)
	{
		HST_AmbientActorRuntimeRecord record = BuildReadyTraffic(
			"traffic_runtime",
			"town_bravo",
			20);
		bool ready = record
			&& record.m_sStateId
				== HST_AmbientActorRuntimeService.STATE_ROUTE_FOLLOWING
			&& record.m_iCreatedAtSecond == 20
			&& record.m_iStateChangedAtSecond == 27
			&& record.m_iAdmittedAtSecond == 27
			&& m_Service.IsBehaviorReady(record);
		bool recovering = m_Service.BeginRecovery(
			record,
			40,
			1,
			2,
			"traffic_stuck");
		bool terminal = recovering
			&& m_Service.IsRetryDue(record, 42)
			&& m_Service.TryTransition(
				record,
				HST_AmbientActorRuntimeService.STATE_DESPAWN_QUEUED,
				"traffic_recycle",
				42);

		report.m_bTrafficPathExact = ready
			&& terminal
			&& record.m_sStateId
				== HST_AmbientActorRuntimeService.STATE_DESPAWN_QUEUED
			&& record.m_iDespawnQueuedAtSecond == 42
			&& !m_Service.IsBehaviorReady(record)
			&& !m_Service.IsBudgetReservation(record);
		report.m_sTrafficPathEvidence = string.Format(
			"ready at 27 %1 | recovery/despawn %2/%3 | terminal %4@%5 | reserved %6",
			ready,
			recovering,
			terminal,
			ResolveState(record),
			ResolveDespawnSecond(record),
			m_Service.IsBudgetReservation(record));
	}

	protected void ProveIllegalTransitionReadOnly(
		HST_AmbientActorRuntimeProofReport report)
	{
		HST_AmbientActorRuntimeRecord record = m_Service.CreateRecord(
			"illegal_runtime",
			"town_charlie",
			HST_AmbientActorRuntimeService.KIND_PEDESTRIAN,
			30,
			2,
			6102);
		HST_AmbientActorRuntimeRecord invalidIdentity = m_Service.CreateRecord(
			"invalid_identity",
			"town_charlie",
			HST_AmbientActorRuntimeService.KIND_PEDESTRIAN,
			30,
			-1,
			6103);
		string stateBefore = record.m_sStateId;
		string reasonBefore = record.m_sReasonId;
		int stateSecondBefore = record.m_iStateChangedAtSecond;
		int recoveryBefore = record.m_iRecoveryCount;
		bool admittedBefore = record.m_bAdmitted;
		bool accepted = m_Service.TryTransition(
			record,
			HST_AmbientActorRuntimeService.STATE_DRIVER_SPAWNED,
			"illegal_skip",
			31);

		report.m_bIllegalTransitionReadOnlyExact = !invalidIdentity
			&& !accepted
			&& record.m_sStateId == stateBefore
			&& record.m_sReasonId == reasonBefore
			&& record.m_iStateChangedAtSecond == stateSecondBefore
			&& record.m_iRecoveryCount == recoveryBefore
			&& record.m_bAdmitted == admittedBefore;
		report.m_sIllegalTransitionEvidence = string.Format(
			"accepted %1 | state/reason/time %2/%3/%4 | recovery/admitted %5/%6",
			accepted,
			record.m_sStateId,
			record.m_sReasonId,
			record.m_iStateChangedAtSecond,
			record.m_iRecoveryCount,
			record.m_bAdmitted);
	}

	protected void ProveBoundedRecovery(
		HST_AmbientActorRuntimeProofReport report)
	{
		HST_AmbientActorRuntimeRecord record = BuildReadyTraffic(
			"bounded_runtime",
			"town_delta",
			40);
		bool first = m_Service.BeginRecovery(record, 50, 2, 4, "stuck_1")
			&& !m_Service.IsRetryDue(record, 53)
			&& m_Service.IsRetryDue(record, 54)
			&& m_Service.TryTransition(
				record,
				HST_AmbientActorRuntimeService.STATE_ROUTE_FOLLOWING,
				"replan_1",
				54);
		bool second = m_Service.BeginRecovery(record, 70, 2, 4, "stuck_2")
			&& m_Service.IsRetryDue(record, 74)
			&& m_Service.TryTransition(
				record,
				HST_AmbientActorRuntimeService.STATE_ROUTE_FOLLOWING,
				"replan_2",
				74);
		bool exhausted = m_Service.BeginRecovery(
			record,
			90,
			2,
			4,
			"stuck_3");

		report.m_bBoundedRecoveryExact = first
			&& second
			&& exhausted
			&& record.m_iRecoveryCount == 2
			&& record.m_sStateId
				== HST_AmbientActorRuntimeService.STATE_DESPAWN_QUEUED
			&& record.m_sReasonId
				== HST_AmbientActorRuntimeService.REASON_RECOVERY_EXHAUSTED
			&& record.m_iDespawnQueuedAtSecond == 90
			&& !m_Service.IsBudgetReservation(record);
		report.m_sBoundedRecoveryEvidence = string.Format(
			"attempts %1/2 | exhausted mutation %2 | state/reason/time %3/%4/%5",
			ResolveRecoveryCount(record),
			exhausted,
			ResolveState(record),
			ResolveReason(record),
			ResolveDespawnSecond(record));
	}

	protected void ProveProgressReset(
		HST_AmbientActorRuntimeProofReport report)
	{
		HST_AmbientActorRuntimeRecord record = BuildReadyPedestrian(
			"progress_runtime",
			"town_echo",
			100);
		vector firstPosition = "10 0 10";
		vector movedPosition = "13 0 14";
		bool firstSampleProgress = m_Service.RecordMovementProgress(
			record,
			firstPosition,
			103,
			2.0);
		bool movementProgress = m_Service.RecordMovementProgress(
			record,
			movedPosition,
			104,
			2.0);
		bool resetExact = !m_Service.ShouldRecover(record, 113, 0, 10)
			&& m_Service.ShouldRecover(record, 114, 0, 10);

		report.m_bProgressResetExact = !firstSampleProgress
			&& movementProgress
			&& resetExact
			&& record.m_bMovementObserved
			&& record.m_bHasMovementSample
			&& record.m_iLastSampleAtSecond == 104
			&& record.m_iLastProgressAtSecond == 104
			&& record.m_vLastSamplePosition == movedPosition;
		report.m_sProgressResetEvidence = string.Format(
			"first/progress %1/%2 | sample/progress second %3/%4 | observed %5 | stuck 113/114 false/true %6",
			firstSampleProgress,
			movementProgress,
			ResolveSampleSecond(record),
			ResolveProgressSecond(record),
			record && record.m_bMovementObserved,
			resetExact);
	}

	protected void ProveBudgetAndReadiness(
		HST_AmbientActorRuntimeProofReport report)
	{
		HST_AmbientActorRuntimeRecord queued = m_Service.CreateRecord(
			"budget_queued",
			"town_foxtrot",
			HST_AmbientActorRuntimeService.KIND_PEDESTRIAN,
			200,
			4,
			6104);
		bool queuedExact = m_Service.IsBudgetReservation(queued)
			&& !m_Service.IsBehaviorReady(queued);
		HST_AmbientActorRuntimeRecord ready = BuildReadyPedestrian(
			"budget_ready",
			"town_foxtrot",
			200);
		bool readyExact = m_Service.IsBudgetReservation(ready)
			&& m_Service.IsBehaviorReady(ready);
		bool recoveringExact = m_Service.BeginRecovery(
			ready,
			210,
			1,
			5,
			"budget_recovery")
			&& m_Service.IsBudgetReservation(ready)
			&& !m_Service.IsBehaviorReady(ready);
		bool terminalExact = m_Service.QueueDespawn(
			ready,
			"budget_release",
			211)
			&& !m_Service.IsBudgetReservation(ready)
			&& !m_Service.IsBehaviorReady(ready);

		report.m_bBudgetReadinessExact = queuedExact
			&& readyExact
			&& recoveringExact
			&& terminalExact;
		report.m_sBudgetReadinessEvidence = string.Format(
			"queued reserved/not-ready %1 | admitted reserved/ready %2 | recovery reserved/not-ready %3 | despawn released %4",
			queuedExact,
			readyExact,
			recoveringExact,
			terminalExact);
	}

	protected void ProveDeterministicEvidence(
		HST_AmbientActorRuntimeProofReport report)
	{
		HST_AmbientActorRuntimeRecord first = BuildDeterministicFixture();
		HST_AmbientActorRuntimeRecord second = BuildDeterministicFixture();
		string firstEvidence = BuildRecordEvidence(first);
		string secondEvidence = BuildRecordEvidence(second);
		report.m_bDeterministicEvidenceExact = first
			&& second
			&& firstEvidence == secondEvidence
			&& first.m_iProjectionSlot == 3
			&& first.m_iProjectionSeed == 7303
			&& first.m_iCreatedAtSecond == 300
			&& first.m_iStateChangedAtSecond == 306
			&& first.m_iAdmittedAtSecond == 302
			&& first.m_iLastProgressAtSecond == 306
			&& first.m_iRecoveryCount == 1
			&& first.m_sReasonId == "deterministic_resume";
		report.m_sDeterministicEvidence = string.Format(
			"equal %1 | %2",
			firstEvidence == secondEvidence,
			firstEvidence);
	}

	protected void ProvePanicRecovery(
		HST_AmbientActorRuntimeProofReport report)
	{
		HST_AmbientActorRuntimeRecord record = BuildReadyPedestrian(
			"panic_runtime",
			"town_hotel",
			400);
		bool began = m_Service.BeginOrExtendPanic(
			record,
			403,
			5,
			"nearby_combat");
		bool extended = m_Service.BeginOrExtendPanic(
			record,
			405,
			6,
			"nearby_combat_repeat");
		bool noOpExtensionReadOnly = !m_Service.BeginOrExtendPanic(
			record,
			406,
			1,
			"shorter_repeat");
		bool firstDeadline = !m_Service.ShouldBeginPanicRecovery(record, 410)
			&& m_Service.ShouldBeginPanicRecovery(record, 411);
		bool firstRecovery = m_Service.BeginPanicRecovery(record, 411, 2)
			&& !m_Service.IsRetryDue(record, 412)
			&& m_Service.IsRetryDue(record, 413);
		bool rebound = m_Service.BeginOrExtendPanic(
			record,
			412,
			5,
			"combat_rebound");
		bool secondDeadline = !m_Service.ShouldBeginPanicRecovery(record, 416)
			&& m_Service.ShouldBeginPanicRecovery(record, 417);
		bool secondRecovery = m_Service.BeginPanicRecovery(record, 417, 2)
			&& m_Service.IsRetryDue(record, 419);
		bool resumed = m_Service.TryTransition(
			record,
			HST_AmbientActorRuntimeService.STATE_WANDERING,
			"panic_recovery_acknowledged",
			419);
		HST_AmbientActorRuntimeRecord blocked = BuildReadyPedestrian(
			"panic_route_bound_runtime",
			"town_india",
			500);
		bool blockedBegan = m_Service.BeginOrExtendPanic(
			blocked,
			503,
			30,
			"blocked_route");
		bool blockedAttemptOne = m_Service.RecordPanicRouteRecoveryAttempt(
			blocked,
			504,
			2,
			"blocked_route_1");
		bool blockedAttemptTwo = m_Service.RecordPanicRouteRecoveryAttempt(
			blocked,
			505,
			2,
			"blocked_route_2");
		bool blockedExhausted = m_Service.RecordPanicRouteRecoveryAttempt(
			blocked,
			506,
			2,
			"blocked_route_3");
		bool boundedRouteExact = blockedBegan
			&& blockedAttemptOne && blockedAttemptTwo && blockedExhausted
			&& blocked.m_iPanicRouteRecoveryCount == 2
			&& blocked.m_iRecoveryCount == 0
			&& blocked.m_sStateId
				== HST_AmbientActorRuntimeService.STATE_DESPAWN_QUEUED;

		bool panicTransitionsExact = began && extended
			&& noOpExtensionReadOnly && firstDeadline && firstRecovery
			&& rebound && secondDeadline && secondRecovery && resumed;
		bool panicFinalStateExact = record
			&& record.m_sStateId
				== HST_AmbientActorRuntimeService.STATE_WANDERING
			&& record.m_iPanicCount == 2
			&& record.m_iPanicUntilSecond == -1
			&& record.m_iRecoveryCount == 0
			&& m_Service.IsBehaviorReady(record)
			&& m_Service.IsBudgetReservation(record);
		report.m_bPanicRecoveryExact = panicTransitionsExact
			&& boundedRouteExact && panicFinalStateExact;
		report.m_sPanicRecoveryEvidence = string.Format(
			"begin/extend/no-op %1/%2/%3 | first deadline/recovery %4/%5 | rebound %6 | second deadline/recovery %7/%8 | resume %9",
			began,
			extended,
			noOpExtensionReadOnly,
			firstDeadline,
			firstRecovery,
			rebound,
			secondDeadline,
			secondRecovery,
			resumed);
		report.m_sPanicRecoveryEvidence
			= report.m_sPanicRecoveryEvidence + string.Format(
				" | panic episodes %1 | route bound %2",
				ResolvePanicCount(record),
				boundedRouteExact);
	}

	protected HST_AmbientActorRuntimeRecord BuildReadyPedestrian(
		string runtimeId,
		string zoneId,
		int createdAtSecond)
	{
		HST_AmbientActorRuntimeRecord record = m_Service.CreateRecord(
			runtimeId,
			zoneId,
			HST_AmbientActorRuntimeService.KIND_PEDESTRIAN,
			createdAtSecond,
			3,
			7303);
		if (!record
			|| !m_Service.TryTransition(
				record,
				HST_AmbientActorRuntimeService.STATE_BEHAVIOR_INITIALIZING,
				"pedestrian_group_ready",
				createdAtSecond + 1)
			|| !m_Service.TryTransition(
				record,
				HST_AmbientActorRuntimeService.STATE_WANDERING,
				"pedestrian_waypoint_ready",
				createdAtSecond + 2))
			return null;
		return record;
	}

	protected HST_AmbientActorRuntimeRecord BuildReadyTraffic(
		string runtimeId,
		string zoneId,
		int createdAtSecond)
	{
		HST_AmbientActorRuntimeRecord record = m_Service.CreateRecord(
			runtimeId,
			zoneId,
			HST_AmbientActorRuntimeService.KIND_TRAFFIC,
			createdAtSecond,
			5,
			8305);
		if (!record)
			return null;

		array<string> states = {
			HST_AmbientActorRuntimeService.STATE_VEHICLE_SPAWNED,
			HST_AmbientActorRuntimeService.STATE_DRIVER_SPAWNED,
			HST_AmbientActorRuntimeService.STATE_SEATING,
			HST_AmbientActorRuntimeService.STATE_DRIVER_CONFIRMED,
			HST_AmbientActorRuntimeService.STATE_ENGINE_STARTING,
			HST_AmbientActorRuntimeService.STATE_ENGINE_STARTED,
			HST_AmbientActorRuntimeService.STATE_ROUTE_FOLLOWING
		};
		array<string> reasons = {
			"vehicle_acknowledged",
			"driver_acknowledged",
			"seat_requested",
			"driver_seat_confirmed",
			"engine_start_requested",
			"engine_started",
			"route_acknowledged"
		};
		for (int index; index < states.Count(); index++)
		{
			if (!m_Service.TryTransition(
				record,
				states[index],
				reasons[index],
				createdAtSecond + index + 1))
				return null;
		}
		return record;
	}

	protected HST_AmbientActorRuntimeRecord BuildDeterministicFixture()
	{
		HST_AmbientActorRuntimeRecord record = BuildReadyPedestrian(
			"deterministic_runtime",
			"town_golf",
			300);
		if (!record
			|| !m_Service.BeginRecovery(
				record,
				304,
				2,
				2,
				"deterministic_recovery")
			|| !m_Service.TryTransition(
				record,
				HST_AmbientActorRuntimeService.STATE_WANDERING,
				"deterministic_resume",
				306))
			return null;
		return record;
	}

	protected string BuildRecordEvidence(HST_AmbientActorRuntimeRecord record)
	{
		if (!record)
			return "missing";
		string evidence = string.Format(
			"%1|%2|%3|%4|%5|created=%6|state=%7|admitted=%8|progress=%9",
			record.m_sRuntimeId,
			record.m_sZoneId,
			record.m_sKindId,
			record.m_sStateId,
			record.m_sReasonId,
			record.m_iCreatedAtSecond,
			record.m_iStateChangedAtSecond,
			record.m_iAdmittedAtSecond,
			record.m_iLastProgressAtSecond);
		return evidence + string.Format(
			"|slot=%1|seed=%2|retry=%3|recoveries=%4|ready=%5|reserved=%6",
			record.m_iProjectionSlot,
			record.m_iProjectionSeed,
			record.m_iRetryAtSecond,
			record.m_iRecoveryCount,
			m_Service.IsBehaviorReady(record),
			m_Service.IsBudgetReservation(record));
	}

	protected string ResolveState(HST_AmbientActorRuntimeRecord record)
	{
		if (!record)
			return "missing";
		return record.m_sStateId;
	}

	protected string ResolveReason(HST_AmbientActorRuntimeRecord record)
	{
		if (!record)
			return "missing";
		return record.m_sReasonId;
	}

	protected int ResolveStateSecond(HST_AmbientActorRuntimeRecord record)
	{
		if (!record)
			return -1;
		return record.m_iStateChangedAtSecond;
	}

	protected int ResolveDespawnSecond(HST_AmbientActorRuntimeRecord record)
	{
		if (!record)
			return -1;
		return record.m_iDespawnQueuedAtSecond;
	}

	protected int ResolveRecoveryCount(HST_AmbientActorRuntimeRecord record)
	{
		if (!record)
			return -1;
		return record.m_iRecoveryCount;
	}

	protected int ResolvePanicCount(HST_AmbientActorRuntimeRecord record)
	{
		if (!record)
			return -1;
		return record.m_iPanicCount;
	}

	protected int ResolveSampleSecond(HST_AmbientActorRuntimeRecord record)
	{
		if (!record)
			return -1;
		return record.m_iLastSampleAtSecond;
	}

	protected int ResolveProgressSecond(HST_AmbientActorRuntimeRecord record)
	{
		if (!record)
			return -1;
		return record.m_iLastProgressAtSecond;
	}
}
#endif
