class HST_SpawnQueueProofReport
{
	string m_sAdmissionEvidence;
	string m_sDuplicateEvidence;
	string m_sIdentityEvidence;
	string m_sPriorityEvidence;
	string m_sFIFOEvidence;
	string m_sRetryEvidence;
	string m_sDeadlineEvidence;
	string m_sCancelEvidence;
	string m_sCapacityEvidence;
	string m_sPruningEvidence;
	string m_sInterruptedRestoreEvidence;
	string m_sTerminalRestoreEvidence;
	string m_sMigrationEvidence;
	bool m_bAdmissionExact;
	bool m_bDuplicateIdempotent;
	bool m_bIdentityConflictRejected;
	bool m_bPriorityExact;
	bool m_bFIFOExact;
	bool m_bRetryBackoffExact;
	bool m_bRetryNoDuplicate;
	bool m_bStaleGenerationRejected;
	bool m_bDeadlineCleanupExact;
	bool m_bCancelIdempotent;
	bool m_bCapacityBounded;
	bool m_bTerminalPruningExact;
	bool m_bInterruptedRestoreExact;
	bool m_bTerminalRestoreExact;
	bool m_bSchema43MigrationExact;
}

class HST_SpawnQueueRetryProofFixture
{
	ref HST_ForceSpawnQueueService m_Queue;
	ref array<ref HST_ForceSpawnResultState> m_aBatches = {};
	ref array<ref HST_ForceManifestState> m_aManifests = {};
	ref HST_ForceManifestState m_Manifest;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ForceSpawnQueueWorkItem m_FirstMemberWork;
	ref HST_ForceSpawnQueueWorkItem m_SecondMemberWork;
	ref HST_ForceSpawnQueueWorkItem m_RetryMemberWork;
	ref HST_ForceSpawnQueueSlotSuccess m_StaleSuccess;
	int m_iFirstGeneration;
	int m_iRetryGeneration;
	int m_iRetryAtSecond;
	bool m_bSetupExact;
	bool m_bBackoffExact;
	bool m_bRetryWorkAtSchedule;
	bool m_bOnlyFailedSlotRetried;
	bool m_bStaleRejected;
	bool m_bCompletedExactly;
	bool m_bReplayIdempotent;
}

class HST_SpawnQueueProofService
{
	protected ref HST_ForcePlanningIntegrityService m_Integrity = new HST_ForcePlanningIntegrityService();

	HST_SpawnQueueProofReport BuildReport()
	{
		HST_SpawnQueueProofReport report = new HST_SpawnQueueProofReport();
		ProveAdmission(report);
		ProveDuplicateRequest(report);
		ProveIdentityConflicts(report);
		ProvePriorityOrder(report);
		ProveFIFOOrder(report);
		ProveRetryLifecycle(report);
		ProveDeadlineCleanup(report);
		ProveCancelIdempotency(report);
		ProveCapacityBounds(report);
		ProveTerminalPruning(report);
		ProveInterruptedRestore(report);
		ProveTerminalRestore(report);
		ProveSchema43Migration(report);
		return report;
	}

	protected void ProveAdmission(HST_SpawnQueueProofReport report)
	{
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		array<ref HST_ForceSpawnResultState> batches = {};
		HST_ForceManifestState manifest = BuildProofManifest("admission", 2, true, true);
		HST_ForceSpawnQueueRequest request = BuildProofRequest("admission", 4, 200);
		HST_ForceSpawnQueueEnqueueResult accepted = queue.Enqueue(batches, manifest, request, 100);
		bool exactSlots = accepted && accepted.m_bSuccess && accepted.m_bStateChanged && accepted.m_Batch;
		if (exactSlots)
			exactSlots = accepted.m_Batch.m_iExpectedSlotCount == 5 && accepted.m_Batch.m_aSlotResults.Count() == 5;
		if (exactSlots)
			exactSlots = CountProofSlotKind(accepted.m_Batch, HST_ForceSpawnQueueService.SLOT_KIND_GROUP) == 1
				&& CountProofSlotKind(accepted.m_Batch, HST_ForceSpawnQueueService.SLOT_KIND_VEHICLE) == 1
				&& CountProofSlotKind(accepted.m_Batch, HST_ForceSpawnQueueService.SLOT_KIND_MEMBER) == 2
				&& CountProofSlotKind(accepted.m_Batch, HST_ForceSpawnQueueService.SLOT_KIND_ASSET) == 1;

		array<ref HST_ForceSpawnResultState> invalidBatches = {};
		HST_ForceManifestState missingRoot = BuildProofManifest("missing_root", 2, false, false);
		missingRoot.m_aGroups.Clear();
		missingRoot.m_sGroupPrefab = "";
		missingRoot.m_sManifestHash = m_Integrity.BuildManifestHash(missingRoot);
		HST_ForceSpawnQueueEnqueueResult rejected = queue.Enqueue(invalidBatches, missingRoot, BuildProofRequest("missing_root", 1, 200), 100);
		bool rejectedWithoutMutation = rejected && !rejected.m_bSuccess && invalidBatches.Count() == 0;
		report.m_bAdmissionExact = exactSlots && rejectedWithoutMutation;
		int actualSlotCount;
		if (accepted && accepted.m_Batch)
			actualSlotCount = accepted.m_Batch.m_aSlotResults.Count();
		report.m_sAdmissionEvidence = string.Format("required group/vehicle/member/asset slots %1 | expected %2 actual %3", exactSlots, 5, actualSlotCount);
		report.m_sAdmissionEvidence = report.m_sAdmissionEvidence + string.Format(" | missing executable group rejected %1 | mutation %2", rejectedWithoutMutation, invalidBatches.Count());
	}

	protected void ProveDuplicateRequest(HST_SpawnQueueProofReport report)
	{
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		array<ref HST_ForceSpawnResultState> batches = {};
		HST_ForceManifestState manifest = BuildProofManifest("duplicate", 0, false, false);
		HST_ForceSpawnQueueRequest request = BuildProofRequest("duplicate", 2, 110);
		HST_ForceSpawnQueueEnqueueResult first = queue.Enqueue(batches, manifest, request, 100);
		HST_ForceSpawnQueueEnqueueResult replay = queue.Enqueue(batches, manifest, request, 120);
		report.m_bDuplicateIdempotent = first && first.m_bSuccess && replay && replay.m_bSuccess
			&& replay.m_bAlreadyApplied && !replay.m_bStateChanged && batches.Count() == 1 && replay.m_Batch == first.m_Batch;
		report.m_sDuplicateEvidence = string.Format("first %1 | replay after deadline %2 already %3 changed %4 | rows %5", first && first.m_bSuccess, replay && replay.m_bSuccess, replay && replay.m_bAlreadyApplied, replay && replay.m_bStateChanged, batches.Count());
	}

	protected void ProveIdentityConflicts(HST_SpawnQueueProofReport report)
	{
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		array<ref HST_ForceSpawnResultState> batches = {};
		HST_ForceManifestState manifest = BuildProofManifest("identity", 0, false, false);
		HST_ForceSpawnQueueRequest original = BuildProofRequest("identity_original", 3, 300);
		HST_ForceSpawnQueueEnqueueResult first = queue.Enqueue(batches, manifest, original, 100);

		HST_ForceSpawnQueueRequest resultConflict = BuildProofRequest("identity_result", 3, 300);
		resultConflict.m_sResultId = original.m_sResultId;
		HST_ForceSpawnQueueEnqueueResult resultRejected = queue.Enqueue(batches, manifest, resultConflict, 100);
		HST_ForceSpawnQueueRequest requestConflict = BuildProofRequest("identity_request", 3, 300);
		requestConflict.m_sRequestId = original.m_sRequestId;
		HST_ForceSpawnQueueEnqueueResult requestRejected = queue.Enqueue(batches, manifest, requestConflict, 100);
		HST_ForceSpawnQueueRequest projectionConflict = BuildProofRequest("identity_projection", 3, 300);
		projectionConflict.m_sProjectionId = original.m_sProjectionId;
		HST_ForceSpawnQueueEnqueueResult projectionRejected = queue.Enqueue(batches, manifest, projectionConflict, 100);
		HST_ForceSpawnQueueEnqueueResult secondProjection = queue.Enqueue(batches, manifest, BuildProofRequest("identity_second_projection", 3, 300), 100);

		bool conflictsRejected = first && first.m_bSuccess && resultRejected && !resultRejected.m_bSuccess;
		conflictsRejected = conflictsRejected && requestRejected && !requestRejected.m_bSuccess && projectionRejected && !projectionRejected.m_bSuccess;
		bool manifestCanProjectAgain = secondProjection && secondProjection.m_bSuccess && batches.Count() == 2;
		report.m_bIdentityConflictRejected = conflictsRejected && manifestCanProjectAgain;
		report.m_sIdentityEvidence = string.Format("result conflict %1 | request conflict %2 | projection conflict %3", resultRejected && !resultRejected.m_bSuccess, requestRejected && !requestRejected.m_bSuccess, projectionRejected && !projectionRejected.m_bSuccess);
		report.m_sIdentityEvidence = report.m_sIdentityEvidence + string.Format(" | same manifest new durable projection %1 | rows %2", manifestCanProjectAgain, batches.Count());
	}

	protected void ProvePriorityOrder(HST_SpawnQueueProofReport report)
	{
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		array<ref HST_ForceSpawnResultState> batches = {};
		array<ref HST_ForceManifestState> manifests = {};
		HST_ForceManifestState manifest = BuildProofManifest("priority", 0, false, false);
		manifests.Insert(manifest);
		HST_ForceSpawnQueueRequest low = BuildProofRequest("priority_low", 1, 300);
		HST_ForceSpawnQueueRequest high = BuildProofRequest("priority_high", 9, 300);
		queue.Enqueue(batches, manifest, low, 100);
		queue.Enqueue(batches, manifest, high, 101);
		HST_ForceSpawnQueueTickResult tick = queue.AcquireWork(batches, manifests, 102);
		bool priorityExact = tick && tick.m_aWorkItems.Count() == 2 && tick.m_aWorkItems[0].m_sResultId == high.m_sResultId;

		string cleanupEvidence;
		bool cleanupFirst = ProveCleanupPrecedence(cleanupEvidence);
		report.m_bPriorityExact = priorityExact && cleanupFirst;
		report.m_sPriorityEvidence = string.Format("high priority first %1 | first %2 | actions %3", priorityExact, ResolveFirstResultId(tick), ResolveWorkCount(tick));
		report.m_sPriorityEvidence = report.m_sPriorityEvidence + " | " + cleanupEvidence;
	}

	protected bool ProveCleanupPrecedence(out string evidence)
	{
		evidence = "cleanup precedence fixture unavailable";
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		array<ref HST_ForceSpawnResultState> batches = {};
		array<ref HST_ForceManifestState> manifests = {};
		HST_ForceManifestState manifest = BuildProofManifest("cleanup_priority", 1, false, false);
		manifests.Insert(manifest);
		HST_ForceSpawnQueueRequest cleanupRequest = BuildProofRequest("cleanup_priority_cancel", 1, 300);
		queue.Enqueue(batches, manifest, cleanupRequest, 100);
		HST_ForceSpawnQueueTickResult groupTick = queue.AcquireWork(batches, manifests, 100);
		HST_ForceSpawnQueueWorkItem groupWork = ResolveFirstWork(groupTick);
		if (!groupWork)
			return false;
		CompleteProofSuccess(queue, batches, manifest, groupWork, 100, "cleanup_priority_group");
		queue.RequestCancel(batches, cleanupRequest.m_sResultId, 101, "priority cleanup proof");
		HST_ForceSpawnQueueRequest pendingRequest = BuildProofRequest("cleanup_priority_pending", 99, 300);
		queue.Enqueue(batches, manifest, pendingRequest, 101);
		HST_ForceSpawnQueueTickResult cleanupTick = queue.AcquireWork(batches, manifests, 102);
		HST_ForceSpawnQueueWorkItem firstWork = ResolveFirstWork(cleanupTick);
		bool exact = firstWork && firstWork.m_sAction == HST_ForceSpawnQueueService.ACTION_CLEANUP
			&& firstWork.m_sResultId == cleanupRequest.m_sResultId;
		evidence = string.Format("cleanup before priority %1 | first action %2", exact, ResolveFirstAction(cleanupTick));
		return exact;
	}

	protected void ProveFIFOOrder(HST_SpawnQueueProofReport report)
	{
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		array<ref HST_ForceSpawnResultState> batches = {};
		array<ref HST_ForceManifestState> manifests = {};
		HST_ForceManifestState manifest = BuildProofManifest("fifo", 0, false, false);
		manifests.Insert(manifest);
		HST_ForceSpawnQueueRequest older = BuildProofRequest("fifo_older", 5, 300);
		HST_ForceSpawnQueueRequest newer = BuildProofRequest("fifo_newer", 5, 300);
		queue.Enqueue(batches, manifest, older, 100);
		queue.Enqueue(batches, manifest, newer, 101);
		HST_ForceSpawnQueueTickResult tick = queue.AcquireWork(batches, manifests, 102);
		report.m_bFIFOExact = tick && tick.m_aWorkItems.Count() == 2
			&& tick.m_aWorkItems[0].m_sResultId == older.m_sResultId && tick.m_aWorkItems[1].m_sResultId == newer.m_sResultId;
		report.m_sFIFOEvidence = string.Format("same priority FIFO %1 | first %2 | second %3", report.m_bFIFOExact, ResolveWorkResultId(tick, 0), ResolveWorkResultId(tick, 1));
	}

	protected void ProveRetryLifecycle(HST_SpawnQueueProofReport report)
	{
		HST_SpawnQueueRetryProofFixture fixture = BuildRetryFixture();
		ResolveFirstRetryAttempt(fixture);
		ResolveSecondRetryAttempt(fixture);
		bool baseExact = fixture && fixture.m_bSetupExact;
		report.m_bRetryBackoffExact = baseExact && fixture.m_bBackoffExact && fixture.m_bRetryWorkAtSchedule;
		report.m_bRetryNoDuplicate = baseExact && fixture.m_bOnlyFailedSlotRetried
			&& fixture.m_bCompletedExactly && fixture.m_bReplayIdempotent;
		report.m_bStaleGenerationRejected = baseExact && fixture.m_bStaleRejected;
		if (!fixture || !fixture.m_Batch)
		{
			report.m_sRetryEvidence = "retry fixture unavailable";
			return;
		}
		report.m_sRetryEvidence = string.Format("setup %1 | retry count %2 | retry at %3 | generations %4 -> %5", fixture.m_bSetupExact, fixture.m_Batch.m_iRetryCount, fixture.m_iRetryAtSecond, fixture.m_iFirstGeneration, fixture.m_iRetryGeneration);
		report.m_sRetryEvidence = report.m_sRetryEvidence + string.Format(" | backoff %1 | work at schedule %2 | only failed slot %3 | stale rejected %4", fixture.m_bBackoffExact, fixture.m_bRetryWorkAtSchedule, fixture.m_bOnlyFailedSlotRetried, fixture.m_bStaleRejected);
		report.m_sRetryEvidence = report.m_sRetryEvidence + string.Format(" | completed %1 | replay %2 | registered %3", fixture.m_bCompletedExactly, fixture.m_bReplayIdempotent, CountRegisteredProofSlots(fixture.m_Batch));
	}

	protected HST_SpawnQueueRetryProofFixture BuildRetryFixture()
	{
		HST_SpawnQueueRetryProofFixture fixture = new HST_SpawnQueueRetryProofFixture();
		fixture.m_Queue = new HST_ForceSpawnQueueService();
		fixture.m_Manifest = BuildProofManifest("retry", 3, false, false);
		fixture.m_aManifests.Insert(fixture.m_Manifest);
		HST_ForceSpawnQueueRequest request = BuildProofRequest("retry", 5, 500);
		HST_ForceSpawnQueueEnqueueResult enqueue = fixture.m_Queue.Enqueue(fixture.m_aBatches, fixture.m_Manifest, request, 100);
		HST_ForceSpawnQueueTickResult groupTick = fixture.m_Queue.AcquireWork(fixture.m_aBatches, fixture.m_aManifests, 100);
		HST_ForceSpawnQueueWorkItem groupWork = ResolveFirstWork(groupTick);
		if (!enqueue || !enqueue.m_bSuccess || !groupWork)
			return fixture;
		fixture.m_Batch = enqueue.m_Batch;
		fixture.m_iFirstGeneration = groupWork.m_iAttemptGeneration;
		HST_ForceSpawnQueueCallbackResult groupSuccess = CompleteProofSuccess(fixture.m_Queue, fixture.m_aBatches, fixture.m_Manifest, groupWork, 100, "retry_group_entity");
		HST_ForceSpawnQueueTickResult memberTick = fixture.m_Queue.AcquireWork(fixture.m_aBatches, fixture.m_aManifests, 101);
		fixture.m_FirstMemberWork = ResolveWork(memberTick, "retry_member_0");
		fixture.m_SecondMemberWork = ResolveWork(memberTick, "retry_member_1");
		fixture.m_RetryMemberWork = ResolveWork(memberTick, "retry_member_2");
		fixture.m_bSetupExact = groupSuccess && groupSuccess.m_bAccepted && memberTick
			&& memberTick.m_aWorkItems.Count() == 3 && fixture.m_FirstMemberWork && fixture.m_SecondMemberWork && fixture.m_RetryMemberWork;
		return fixture;
	}

	protected void ResolveFirstRetryAttempt(HST_SpawnQueueRetryProofFixture fixture)
	{
		if (!fixture || !fixture.m_bSetupExact)
			return;
		CompleteProofSuccess(fixture.m_Queue, fixture.m_aBatches, fixture.m_Manifest, fixture.m_FirstMemberWork, 101, "retry_member_entity_0");
		CompleteProofSuccess(fixture.m_Queue, fixture.m_aBatches, fixture.m_Manifest, fixture.m_SecondMemberWork, 101, "retry_member_entity_1");
		HST_ForceSpawnQueueSlotFailure failure = BuildProofFailure(fixture.m_RetryMemberWork, "transient proof failure", true);
		HST_ForceSpawnQueueCallbackResult failed = fixture.m_Queue.FailSlot(fixture.m_aBatches, fixture.m_Manifest, failure, 101);
		fixture.m_iRetryAtSecond = fixture.m_Batch.m_iNextAttemptSecond;
		HST_ForceSpawnQueueTickResult earlyTick = fixture.m_Queue.AcquireWork(fixture.m_aBatches, fixture.m_aManifests, 102);
		fixture.m_bBackoffExact = failed && failed.m_bAccepted
			&& fixture.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_RETRYABLE
			&& fixture.m_Batch.m_iRetryCount == 1 && fixture.m_Batch.m_iNextAttemptSecond == 103
			&& earlyTick && earlyTick.m_aWorkItems.Count() == 0;
	}

	protected void ResolveSecondRetryAttempt(HST_SpawnQueueRetryProofFixture fixture)
	{
		if (!fixture || !fixture.m_bBackoffExact)
			return;
		HST_ForceSpawnQueueTickResult retryTick = fixture.m_Queue.AcquireWork(fixture.m_aBatches, fixture.m_aManifests, 103);
		HST_ForceSpawnQueueWorkItem retryWork = ResolveFirstWork(retryTick);
		if (!retryWork)
			return;
		fixture.m_iRetryGeneration = retryWork.m_iAttemptGeneration;
		fixture.m_bRetryWorkAtSchedule = retryTick.m_aWorkItems.Count() == 1
			&& fixture.m_iRetryGeneration == fixture.m_iFirstGeneration + 1;
		fixture.m_bOnlyFailedSlotRetried = retryTick.m_aWorkItems.Count() == 1
			&& retryWork.m_sSlotId == fixture.m_RetryMemberWork.m_sSlotId
			&& CountRegisteredProofSlots(fixture.m_Batch) == 3;

		fixture.m_StaleSuccess = BuildProofSuccess(fixture.m_aBatches, retryWork, "retry_stale_entity");
		fixture.m_StaleSuccess.m_iAttemptGeneration = fixture.m_iFirstGeneration;
		HST_ForceSpawnQueueCallbackResult stale = fixture.m_Queue.CompleteSlotSuccess(fixture.m_aBatches, fixture.m_Manifest, fixture.m_StaleSuccess, 103);
		fixture.m_bStaleRejected = stale && stale.m_bStale && !stale.m_bAccepted && stale.m_bCleanupRequired
			&& fixture.m_Batch.FindSlotResult(retryWork.m_sSlotId).m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING;

		HST_ForceSpawnQueueSlotSuccess properSuccess = BuildProofSuccess(fixture.m_aBatches, retryWork, "retry_member_entity_2");
		HST_ForceSpawnQueueCallbackResult completed = fixture.m_Queue.CompleteSlotSuccess(fixture.m_aBatches, fixture.m_Manifest, properSuccess, 103);
		HST_ForceSpawnQueueCallbackResult replay = fixture.m_Queue.CompleteSlotSuccess(fixture.m_aBatches, fixture.m_Manifest, properSuccess, 104);
		fixture.m_bCompletedExactly = completed && completed.m_bAccepted
			&& fixture.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			&& CountRegisteredProofSlots(fixture.m_Batch) == 4 && ProofRegisteredEntityIdsUnique(fixture.m_Batch);
		fixture.m_bReplayIdempotent = replay && replay.m_bAccepted && replay.m_bAlreadyApplied && !replay.m_bStateChanged;
	}

	protected void ProveDeadlineCleanup(HST_SpawnQueueProofReport report)
	{
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		array<ref HST_ForceSpawnResultState> batches = {};
		array<ref HST_ForceManifestState> manifests = {};
		HST_ForceManifestState manifest = BuildProofManifest("deadline", 1, false, false);
		manifests.Insert(manifest);
		HST_ForceSpawnQueueRequest request = BuildProofRequest("deadline", 4, 105);
		HST_ForceSpawnQueueEnqueueResult enqueue = queue.Enqueue(batches, manifest, request, 100);
		HST_ForceSpawnQueueTickResult groupTick = queue.AcquireWork(batches, manifests, 100);
		HST_ForceSpawnQueueWorkItem groupWork = ResolveFirstWork(groupTick);
		if (!enqueue || !enqueue.m_bSuccess || !groupWork)
		{
			report.m_sDeadlineEvidence = "deadline fixture unavailable";
			return;
		}
		CompleteProofSuccess(queue, batches, manifest, groupWork, 100, "deadline_group_entity");
		HST_ForceSpawnQueueTickResult deadlineTick = queue.AcquireWork(batches, manifests, 105);
		HST_ForceSpawnQueueWorkItem cleanupWork = ResolveFirstWork(deadlineTick);
		bool cleanupBeforeTerminal = cleanupWork && cleanupWork.m_sAction == HST_ForceSpawnQueueService.ACTION_CLEANUP
			&& enqueue.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING;
		HST_ForceSpawnQueueCallbackResult cleanup;
		if (cleanupWork)
			cleanup = queue.CompleteCleanup(batches, request.m_sResultId, request.m_sProjectionId, cleanupWork.m_sSlotId, cleanupWork.m_iAttemptGeneration, 105, true);
		bool terminalAfterCleanup = cleanup && cleanup.m_bAccepted
			&& enqueue.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			&& enqueue.m_Batch.m_sNativeGroupId.IsEmpty() && cleanupWork.m_sEntityId == "deadline_group_entity";
		report.m_bDeadlineCleanupExact = cleanupBeforeTerminal && terminalAfterCleanup;
		report.m_sDeadlineEvidence = string.Format("deadline cleanup pending %1 | action %2 | terminal after callback %3 | status %4", cleanupBeforeTerminal, ResolveFirstAction(deadlineTick), terminalAfterCleanup, enqueue.m_Batch.m_eStatus);
	}

	protected void ProveCancelIdempotency(HST_SpawnQueueProofReport report)
	{
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		array<ref HST_ForceSpawnResultState> batches = {};
		array<ref HST_ForceManifestState> manifests = {};
		HST_ForceManifestState manifest = BuildProofManifest("cancel", 1, false, false);
		manifests.Insert(manifest);
		HST_ForceSpawnQueueRequest request = BuildProofRequest("cancel", 2, 105);
		HST_ForceSpawnQueueEnqueueResult enqueue = queue.Enqueue(batches, manifest, request, 100);
		HST_ForceSpawnQueueWorkItem groupWork = ResolveFirstWork(queue.AcquireWork(batches, manifests, 100));
		if (!enqueue || !enqueue.m_bSuccess || !groupWork)
		{
			report.m_sCancelEvidence = "cancel fixture unavailable";
			return;
		}
		CompleteProofSuccess(queue, batches, manifest, groupWork, 100, "cancel_group_entity");
		HST_ForceSpawnQueueCallbackResult firstCancel = queue.RequestCancel(batches, request.m_sResultId, 104, "proof cancellation");
		HST_ForceSpawnQueueCallbackResult duplicatePending = queue.RequestCancel(batches, request.m_sResultId, 106, "duplicate proof cancellation");
		HST_ForceSpawnQueueTickResult cleanupTick = queue.AcquireWork(batches, manifests, 106);
		HST_ForceSpawnQueueWorkItem cleanupWork = ResolveFirstWork(cleanupTick);
		HST_ForceSpawnQueueCallbackResult cleanup;
		if (cleanupWork)
			cleanup = queue.CompleteCleanup(batches, request.m_sResultId, request.m_sProjectionId, cleanupWork.m_sSlotId, cleanupWork.m_iAttemptGeneration, 106, true);
		HST_ForceSpawnQueueCallbackResult duplicateTerminal = queue.RequestCancel(batches, request.m_sResultId, 107, "terminal replay");
		HST_ForceSpawnQueueCallbackResult cleanupReplay;
		if (cleanupWork)
			cleanupReplay = queue.CompleteCleanup(batches, request.m_sResultId, request.m_sProjectionId, cleanupWork.m_sSlotId, cleanupWork.m_iAttemptGeneration, 107, true);

		bool acceptedAcrossDeadline = firstCancel && firstCancel.m_bAccepted && duplicatePending && duplicatePending.m_bAccepted
			&& duplicatePending.m_bAlreadyApplied && cleanupWork && cleanupWork.m_sAction == HST_ForceSpawnQueueService.ACTION_CLEANUP;
		bool exactlyTerminal = cleanup && cleanup.m_bAccepted
			&& enqueue.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED;
		bool replayExact = duplicateTerminal && duplicateTerminal.m_bAccepted && duplicateTerminal.m_bAlreadyApplied
			&& cleanupReplay && cleanupReplay.m_bAccepted && cleanupReplay.m_bAlreadyApplied;
		report.m_bCancelIdempotent = acceptedAcrossDeadline && exactlyTerminal && replayExact;
		report.m_sCancelEvidence = string.Format("accepted before deadline %1 | duplicate after deadline %2 | cleanup action %3", firstCancel && firstCancel.m_bAccepted, duplicatePending && duplicatePending.m_bAlreadyApplied, ResolveFirstAction(cleanupTick));
		report.m_sCancelEvidence = report.m_sCancelEvidence + string.Format(" | cancelled once %1 | terminal cancel replay %2 | cleanup replay %3", exactlyTerminal, duplicateTerminal && duplicateTerminal.m_bAlreadyApplied, cleanupReplay && cleanupReplay.m_bAlreadyApplied);
	}

	protected void ProveCapacityBounds(HST_SpawnQueueProofReport report)
	{
		string requestEvidence;
		string slotEvidence;
		string batchEvidence;
		bool requestBound = ProvePerRequestCapacity(requestEvidence);
		bool slotBound = ProveTotalSlotCapacity(slotEvidence);
		bool batchBound = ProveBatchCapacity(batchEvidence);
		report.m_bCapacityBounded = requestBound && slotBound && batchBound;
		report.m_sCapacityEvidence = requestEvidence + " | " + slotEvidence + " | " + batchEvidence;
	}

	protected bool ProvePerRequestCapacity(out string evidence)
	{
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		array<ref HST_ForceSpawnResultState> acceptedRows = {};
		array<ref HST_ForceSpawnResultState> rejectedRows = {};
		HST_ForceManifestState atLimit = BuildProofManifest("capacity_request_64", 63, false, false);
		HST_ForceManifestState overLimit = BuildProofManifest("capacity_request_65", 64, false, false);
		HST_ForceSpawnQueueEnqueueResult accepted = queue.Enqueue(acceptedRows, atLimit, BuildProofRequest("capacity_request_64", 1, 1000), 100);
		HST_ForceSpawnQueueEnqueueResult rejected = queue.Enqueue(rejectedRows, overLimit, BuildProofRequest("capacity_request_65", 1, 1000), 100);
		bool exact = accepted && accepted.m_bSuccess && accepted.m_Batch.m_iExpectedSlotCount == 64
			&& rejected && !rejected.m_bSuccess && rejectedRows.Count() == 0;
		evidence = string.Format("per-request 64 accepted %1 | 65 rejected %2", accepted && accepted.m_bSuccess, rejected && !rejected.m_bSuccess);
		return exact;
	}

	protected bool ProveTotalSlotCapacity(out string evidence)
	{
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		array<ref HST_ForceSpawnResultState> batches = {};
		HST_ForceManifestState eightSlots = BuildProofManifest("capacity_slots_8", 7, false, false);
		bool fillsExactly = true;
		for (int index = 0; index < 63; index++)
		{
			HST_ForceSpawnQueueRequest request = BuildProofRequest(string.Format("capacity_slots_fill_%1", index), 1, 1000);
			HST_ForceSpawnQueueEnqueueResult accepted = queue.Enqueue(batches, eightSlots, request, 100);
			fillsExactly = fillsExactly && accepted && accepted.m_bSuccess;
		}
		HST_ForceManifestState nineSlots = BuildProofManifest("capacity_slots_9", 8, false, false);
		HST_ForceSpawnQueueEnqueueResult rejected = queue.Enqueue(batches, nineSlots, BuildProofRequest("capacity_slots_over", 1, 1000), 100);
		bool exact = fillsExactly && batches.Count() == 63 && rejected && !rejected.m_bSuccess;
		evidence = string.Format("slot rows 504 across %1 batches | adding 9 rejected %2", batches.Count(), rejected && !rejected.m_bSuccess);
		return exact;
	}

	protected bool ProveBatchCapacity(out string evidence)
	{
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		array<ref HST_ForceSpawnResultState> batches = {};
		HST_ForceManifestState oneSlot = BuildProofManifest("capacity_batches", 0, false, false);
		bool fillsExactly = true;
		for (int index = 0; index < 64; index++)
		{
			HST_ForceSpawnQueueRequest request = BuildProofRequest(string.Format("capacity_batch_%1", index), 1, 1000);
			HST_ForceSpawnQueueEnqueueResult accepted = queue.Enqueue(batches, oneSlot, request, 100);
			fillsExactly = fillsExactly && accepted && accepted.m_bSuccess;
		}
		HST_ForceSpawnQueueEnqueueResult rejected = queue.Enqueue(batches, oneSlot, BuildProofRequest("capacity_batch_over", 1, 1000), 100);
		bool exact = fillsExactly && batches.Count() == 64 && rejected && !rejected.m_bSuccess;
		evidence = string.Format("nonterminal batches %1 | 65th rejected %2", batches.Count(), rejected && !rejected.m_bSuccess);
		return exact;
	}

	protected void ProveTerminalPruning(HST_SpawnQueueProofReport report)
	{
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		array<ref HST_ForceSpawnResultState> compactableRows = BuildProofTerminalRows("compact", 128, 100);
		HST_ForceSpawnQueueMaintenanceResult refused = queue.CompactTerminalRows(compactableRows, null, 2000);
		int refusedCount = compactableRows.Count();
		HST_ForceSpawnQueueRetentionPins emptyPins = new HST_ForceSpawnQueueRetentionPins();
		HST_ForceSpawnQueueMaintenanceResult compacted = queue.CompactTerminalRows(compactableRows, emptyPins, 2000);
		HST_ForceManifestState manifest = BuildProofManifest("post_compaction", 0, false, false);
		HST_ForceSpawnQueueEnqueueResult admitted = queue.Enqueue(compactableRows, manifest, BuildProofRequest("post_compaction", 1, 3000), 2000);

		array<ref HST_ForceSpawnResultState> protectedRows = BuildProofTerminalRows("protected", 128, 100);
		protectedRows[127].m_iCompletedAtSecond = 1900;
		HST_ForceSpawnQueueRetentionPins protectedPins = new HST_ForceSpawnQueueRetentionPins();
		protectedPins.m_aResultIds.Insert("result_terminal_protected_0");
		HST_ForceSpawnQueueMaintenanceResult protectedCompaction = queue.CompactTerminalRows(protectedRows, protectedPins, 2000);
		bool refusalExact = refused && refused.m_bPinsRequired && !refused.m_bStateChanged && refusedCount == 128;
		bool admissionRoom = compacted && compacted.m_iRemovedTerminalCount == 1 && admitted && admitted.m_bSuccess;
		bool protectionExact = protectedCompaction && protectedCompaction.m_iRemovedTerminalCount == 1
			&& FindProofBatch(protectedRows, "result_terminal_protected_0")
			&& FindProofBatch(protectedRows, "result_terminal_protected_127")
			&& !FindProofBatch(protectedRows, "result_terminal_protected_1");
		report.m_bTerminalPruningExact = refusalExact && admissionRoom && protectionExact;
		report.m_sPruningEvidence = string.Format("missing pins refused %1 | explicit pins removed %2 | admission %3", refusalExact, ResolveRemovedCount(compacted), admitted && admitted.m_bSuccess);
		report.m_sPruningEvidence = report.m_sPruningEvidence + string.Format(" | pinned kept %1 | young kept %2 | oldest eligible removed %3", FindProofBatch(protectedRows, "result_terminal_protected_0") != null, FindProofBatch(protectedRows, "result_terminal_protected_127") != null, FindProofBatch(protectedRows, "result_terminal_protected_1") == null);
	}

	protected void ProveInterruptedRestore(HST_SpawnQueueProofReport report)
	{
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		HST_CampaignState state = new HST_CampaignState();
		state.m_iElapsedSeconds = 100;
		state.m_bRestoredFromPersistence = true;
		state.m_iPersistenceRestoreSequence = 9;
		state.m_iForceSpawnQueueReconciledRestoreSequence = 8;
		HST_ForceManifestState manifest = BuildProofManifest("interrupted_restore", 0, false, false);
		state.m_aForceManifests.Insert(manifest);
		HST_ForceSpawnQueueRequest request = BuildProofRequest("interrupted_restore", 3, 400);
		HST_ForceSpawnQueueEnqueueResult enqueue = queue.Enqueue(state.m_aForceSpawnResults, manifest, request, 100);
		HST_ForceSpawnQueueTickResult beforeSaveTick = queue.AcquireWork(state.m_aForceSpawnResults, state.m_aForceManifests, 100);
		int interruptedGeneration = ResolveFirstGeneration(beforeSaveTick);

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(state);
		HST_CampaignState restored = saveData.Restore();
		HST_ForceSpawnQueueMaintenanceResult firstReconcile = queue.ReconcileCampaignAfterRestore(restored);
		HST_ForceSpawnResultState restoredBatch;
		if (restored)
			restoredBatch = restored.FindForceSpawnResult(request.m_sResultId);
		int reconciledGeneration;
		if (restoredBatch)
			reconciledGeneration = restoredBatch.m_iAttemptGeneration;
		HST_ForceSpawnQueueMaintenanceResult repeatReconcile = queue.ReconcileCampaignAfterRestore(restored);

		bool reconciledOnce = enqueue && enqueue.m_bSuccess && interruptedGeneration == 1 && firstReconcile && firstReconcile.m_bStateChanged;
		reconciledOnce = reconciledOnce && restoredBatch && restoredBatch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_RETRYABLE;
		reconciledOnce = reconciledOnce && reconciledGeneration == 2 && restored.m_iForceSpawnQueueReconciledRestoreSequence == 9;
		bool repeatNoOp = repeatReconcile && !repeatReconcile.m_bStateChanged && restoredBatch.m_iAttemptGeneration == reconciledGeneration;
		bool completedOnce = CompleteRestoredProofBatch(queue, restored, manifest, request.m_sResultId, 102);
		report.m_bInterruptedRestoreExact = reconciledOnce && repeatNoOp && completedOnce;
		report.m_sInterruptedRestoreEvidence = string.Format("interrupted generation %1 | reconciled generation %2 sequence %3", interruptedGeneration, reconciledGeneration, restored.m_iForceSpawnQueueReconciledRestoreSequence);
		report.m_sInterruptedRestoreEvidence = report.m_sInterruptedRestoreEvidence + string.Format(" | first changed %1 | repeat no-op %2 | completed once %3", firstReconcile && firstReconcile.m_bStateChanged, repeatNoOp, completedOnce);
	}

	protected bool CompleteRestoredProofBatch(
		HST_ForceSpawnQueueService queue,
		HST_CampaignState restored,
		HST_ForceManifestState sourceManifest,
		string resultId,
		int nowSecond)
	{
		if (!queue || !restored)
			return false;
		HST_ForceManifestState restoredManifest = restored.FindForceManifest(sourceManifest.m_sManifestId);
		HST_ForceSpawnQueueTickResult tick = queue.AcquireWork(restored.m_aForceSpawnResults, restored.m_aForceManifests, nowSecond);
		HST_ForceSpawnQueueWorkItem work = ResolveFirstWork(tick);
		if (!restoredManifest || !work || work.m_sResultId != resultId)
			return false;
		HST_ForceSpawnQueueSlotSuccess success = BuildProofSuccess(restored.m_aForceSpawnResults, work, "restored_group_entity");
		HST_ForceSpawnQueueCallbackResult completed = queue.CompleteSlotSuccess(restored.m_aForceSpawnResults, restoredManifest, success, nowSecond);
		HST_ForceSpawnQueueCallbackResult replay = queue.CompleteSlotSuccess(restored.m_aForceSpawnResults, restoredManifest, success, nowSecond + 1);
		HST_ForceSpawnResultState batch = restored.FindForceSpawnResult(resultId);
		return completed && completed.m_bAccepted && batch
			&& batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			&& replay && replay.m_bAccepted && replay.m_bAlreadyApplied && !replay.m_bStateChanged;
	}

	protected void ProveTerminalRestore(HST_SpawnQueueProofReport report)
	{
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		HST_CampaignState state = BuildTerminalRestoreState(queue);
		if (!state)
		{
			report.m_sTerminalRestoreEvidence = "terminal restore fixture unavailable";
			return;
		}
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(state);
		HST_CampaignState restored = saveData.Restore();
		HST_ForceSpawnQueueMaintenanceResult firstReconcile = queue.ReconcileCampaignAfterRestore(restored);
		HST_ForceSpawnResultState batch;
		if (restored)
			batch = restored.FindForceSpawnResult("result_terminal_restore");
		HST_ForceSpawnSlotResultState slot;
		if (batch)
			slot = batch.FindSlotResult("terminal_restore_group");
		HST_ForceSpawnQueueMaintenanceResult repeatReconcile = queue.ReconcileCampaignAfterRestore(restored);
		HST_ForceSpawnQueueTickResult tick;
		if (restored)
			tick = queue.AcquireWork(restored.m_aForceSpawnResults, restored.m_aForceManifests, 101);

		bool terminalPreserved = batch && slot && batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
		terminalPreserved = terminalPreserved && slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
		terminalPreserved = terminalPreserved && slot.m_sSpawnedPrefab == "proof_group_prefab" && slot.m_bAliveVerified;
		bool processIdsCleared = terminalPreserved && batch.m_sNativeGroupId.IsEmpty()
			&& slot.m_sEntityId.IsEmpty() && slot.m_sNativeGroupId.IsEmpty();
		bool neverReopened = tick && tick.m_aWorkItems.Count() == 0 && batch.m_iAttemptGeneration == 1;
		bool epochNoOp = firstReconcile && firstReconcile.m_bStateChanged && repeatReconcile && !repeatReconcile.m_bStateChanged;
		report.m_bTerminalRestoreExact = terminalPreserved && processIdsCleared && neverReopened && epochNoOp;
		report.m_sTerminalRestoreEvidence = string.Format("success preserved %1 | prefab/verification preserved %2 | process ids cleared %3", terminalPreserved, slot && slot.m_sSpawnedPrefab == "proof_group_prefab" && slot.m_bAliveVerified, processIdsCleared);
		report.m_sTerminalRestoreEvidence = report.m_sTerminalRestoreEvidence + string.Format(" | reacquired actions %1 | repeat no-op %2", ResolveWorkCount(tick), repeatReconcile && !repeatReconcile.m_bStateChanged);
	}

	protected HST_CampaignState BuildTerminalRestoreState(HST_ForceSpawnQueueService queue)
	{
		if (!queue)
			return null;
		HST_CampaignState state = new HST_CampaignState();
		state.m_iElapsedSeconds = 100;
		state.m_bRestoredFromPersistence = true;
		state.m_iPersistenceRestoreSequence = 4;
		state.m_iForceSpawnQueueReconciledRestoreSequence = 3;
		HST_ForceManifestState manifest = BuildProofManifest("terminal_restore", 0, false, false);
		state.m_aForceManifests.Insert(manifest);
		HST_ForceSpawnQueueRequest request = BuildProofRequest("terminal_restore", 3, 400);
		HST_ForceSpawnQueueEnqueueResult enqueue = queue.Enqueue(state.m_aForceSpawnResults, manifest, request, 100);
		HST_ForceSpawnQueueTickResult tick = queue.AcquireWork(state.m_aForceSpawnResults, state.m_aForceManifests, 100);
		HST_ForceSpawnQueueWorkItem work = ResolveFirstWork(tick);
		if (!enqueue || !enqueue.m_bSuccess || !work)
			return null;
		HST_ForceSpawnQueueCallbackResult completed = CompleteProofSuccess(queue, state.m_aForceSpawnResults, manifest, work, 100, "terminal_restore_entity");
		if (!completed || !completed.m_bAccepted || enqueue.m_Batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			return null;
		return state;
	}

	protected void ProveSchema43Migration(HST_SpawnQueueProofReport report)
	{
		HST_CampaignSaveData legacySave = new HST_CampaignSaveData();
		legacySave.m_iSchemaVersion = 43;
		legacySave.m_iElapsedSeconds = 700;
		legacySave.m_aForceSpawnResults.Insert(BuildLegacyProofBatch("legacy_active", false));
		legacySave.m_aForceSpawnResults.Insert(BuildLegacyProofBatch("legacy_terminal", true));
		HST_CampaignState restored = legacySave.Restore();
		HST_ForceSpawnResultState active;
		HST_ForceSpawnResultState terminal;
		if (restored)
		{
			active = restored.FindForceSpawnResult("result_legacy_active");
			terminal = restored.FindForceSpawnResult("result_legacy_terminal");
		}
		bool eventFound = HasProofEvent(restored, "migration_schema44_spawn_queue");
		bool activeFailedClosed = active && active.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
		if (activeFailedClosed)
			activeFailedClosed = ProofBatchPhysicalIdsClear(active) && active.m_aSlotResults[0].m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_FINAL;
		bool terminalPreserved = terminal && terminal.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
		if (terminalPreserved)
			terminalPreserved = terminal.m_aSlotResults[0].m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
				&& terminal.m_aSlotResults[0].m_sEntityId == "legacy_terminal_entity";
		report.m_bSchema43MigrationExact = restored && restored.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION
			&& restored.m_iLastLoadedSchemaVersion == 43 && activeFailedClosed && terminalPreserved && eventFound;
		report.m_sMigrationEvidence = string.Format("schema 43 -> %1 | active failed closed %2 | terminal preserved %3 | event %4", ResolveSchema(restored), activeFailedClosed, terminalPreserved, eventFound);
	}

	protected HST_ForceSpawnResultState BuildLegacyProofBatch(string token, bool terminal)
	{
		HST_ForceSpawnResultState batch = new HST_ForceSpawnResultState();
		batch.m_sResultId = "result_" + token;
		batch.m_sRequestId = "request_" + token;
		batch.m_sManifestId = "manifest_" + token;
		batch.m_sManifestHash = "legacy_hash_" + token;
		batch.m_sOperationId = "operation_" + token;
		batch.m_sForceId = "force_" + token;
		batch.m_sProjectionId = "projection_" + token;
		batch.m_sNativeGroupId = "legacy_native_" + token;
		batch.m_iExpectedSlotCount = 1;
		batch.m_iAttemptGeneration = 3;
		batch.m_iCreatedAtSecond = 100;
		batch.m_iUpdatedAtSecond = 200;
		batch.m_iDeadlineSecond = 900;
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS;
		if (terminal)
		{
			batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
			batch.m_iCompletedAtSecond = 300;
		}
		HST_ForceSpawnSlotResultState slot = new HST_ForceSpawnSlotResultState();
		slot.m_sSlotId = token + "_group";
		slot.m_sSlotKind = HST_ForceSpawnQueueService.SLOT_KIND_GROUP;
		slot.m_sSpawnedPrefab = "proof_group_prefab";
		slot.m_sEntityId = token + "_entity";
		slot.m_sNativeGroupId = "legacy_native_" + token;
		slot.m_sProjectionId = batch.m_sProjectionId;
		slot.m_iAttemptCount = 1;
		slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING;
		if (terminal)
			slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
		slot.m_bAliveVerified = true;
		slot.m_bFactionVerified = true;
		slot.m_bGroupVerified = true;
		slot.m_bProjectionVerified = true;
		batch.m_aSlotResults.Insert(slot);
		return batch;
	}

	protected HST_ForceManifestState BuildProofManifest(
		string token,
		int memberCount,
		bool includeVehicle,
		bool includeAsset)
	{
		HST_ForceManifestState manifest = new HST_ForceManifestState();
		manifest.m_sManifestId = "manifest_" + token;
		manifest.m_sOperationId = "operation_" + token;
		manifest.m_sQuoteId = "quote_" + token;
		manifest.m_sCommandRequestId = "command_" + token;
		manifest.m_sForceKind = "proof_force";
		manifest.m_sFactionRole = "proof_role";
		manifest.m_sFactionKey = "FIA";
		manifest.m_sIntentId = "intent_" + token;
		manifest.m_sSourceZoneId = "source_proof";
		manifest.m_sTargetZoneId = "target_proof";
		manifest.m_sGroupPrefab = "proof_group_prefab";
		manifest.m_sCatalogVersion = "proof_catalog_1";
		manifest.m_sPolicyId = "proof_all_required";
		manifest.m_iRequestedMemberCount = memberCount;
		manifest.m_iAcceptedMemberCount = memberCount;
		manifest.m_iCreatedAtSecond = 100;
		manifest.m_bFrozen = true;
		AppendProofGroup(manifest, token, memberCount);
		if (includeVehicle)
			AppendProofVehicle(manifest, token);
		AppendProofMembers(manifest, token, memberCount, includeVehicle);
		if (includeAsset)
			AppendProofAsset(manifest, token, includeVehicle);
		manifest.m_iRequestedVehicleCount = manifest.m_aVehicles.Count();
		manifest.m_iAcceptedVehicleCount = manifest.m_aVehicles.Count();
		manifest.m_sManifestHash = m_Integrity.BuildManifestHash(manifest);
		return manifest;
	}

	protected void AppendProofGroup(HST_ForceManifestState manifest, string token, int memberCount)
	{
		HST_ForceManifestGroupState group = new HST_ForceManifestGroupState();
		group.m_sElementId = token + "_group";
		group.m_sCatalogEntryId = "proof_group_catalog";
		group.m_sPrefab = "proof_group_prefab";
		group.m_sRole = "root";
		group.m_iOrdinal = 0;
		group.m_iExpectedMemberCount = memberCount;
		group.m_bRequired = true;
		manifest.m_aGroups.Insert(group);
	}

	protected void AppendProofVehicle(HST_ForceManifestState manifest, string token)
	{
		HST_ForceManifestVehicleState vehicle = new HST_ForceManifestVehicleState();
		vehicle.m_sSlotId = token + "_vehicle";
		vehicle.m_sCatalogEntryId = "proof_vehicle_catalog";
		vehicle.m_sGroupElementId = token + "_group";
		vehicle.m_sPrefab = "proof_vehicle_prefab";
		vehicle.m_sRole = "transport";
		vehicle.m_iOrdinal = 0;
		vehicle.m_iRequiredCrew = 1;
		vehicle.m_bRequired = true;
		manifest.m_aVehicles.Insert(vehicle);
	}

	protected void AppendProofMembers(HST_ForceManifestState manifest, string token, int memberCount, bool includeVehicle)
	{
		for (int index = 0; index < memberCount; index++)
		{
			HST_ForceManifestMemberState member = new HST_ForceManifestMemberState();
			member.m_sSlotId = string.Format("%1_member_%2", token, index);
			member.m_sCatalogSlotId = string.Format("proof_member_catalog_%1", index);
			member.m_sGroupElementId = token + "_group";
			member.m_sPrefab = "proof_member_prefab";
			member.m_sRole = "rifleman";
			member.m_iOrdinal = index;
			member.m_bRequired = true;
			if (includeVehicle && index == 0)
			{
				member.m_sAssignedVehicleSlotId = token + "_vehicle";
				member.m_sSeatRole = "driver";
				member.m_iSeatIndex = 0;
			}
			manifest.m_aMembers.Insert(member);
		}
	}

	protected void AppendProofAsset(HST_ForceManifestState manifest, string token, bool includeVehicle)
	{
		HST_ForceManifestAssetState asset = new HST_ForceManifestAssetState();
		asset.m_sSlotId = token + "_asset";
		asset.m_sKind = "supply";
		asset.m_sPrefab = "proof_asset_prefab";
		asset.m_sRole = "cargo";
		if (includeVehicle)
			asset.m_sAssignedVehicleSlotId = token + "_vehicle";
		asset.m_iQuantity = 1;
		asset.m_iOrdinal = 0;
		asset.m_bRequired = true;
		manifest.m_aAssets.Insert(asset);
	}

	protected HST_ForceSpawnQueueRequest BuildProofRequest(string token, int priority, int deadlineSecond)
	{
		HST_ForceSpawnQueueRequest request = new HST_ForceSpawnQueueRequest();
		request.m_sResultId = "result_" + token;
		request.m_sRequestId = "request_" + token;
		request.m_sProjectionId = "projection_" + token;
		request.m_sForceId = "force_" + token;
		request.m_iPriority = priority;
		request.m_iMaxRetries = 3;
		request.m_iDeadlineSecond = deadlineSecond;
		return request;
	}

	protected HST_ForceSpawnQueueSlotSuccess BuildProofSuccess(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceSpawnQueueWorkItem work,
		string entityId)
	{
		HST_ForceSpawnQueueSlotSuccess success = new HST_ForceSpawnQueueSlotSuccess();
		if (!work)
			return success;
		success.m_sResultId = work.m_sResultId;
		success.m_sProjectionId = work.m_sProjectionId;
		success.m_sSlotId = work.m_sSlotId;
		success.m_sEntityId = entityId;
		success.m_sSpawnedPrefab = work.m_sPrefab;
		success.m_sAssignedVehicleEntityId = work.m_sAssignedVehicleEntityId;
		success.m_iAttemptGeneration = work.m_iAttemptGeneration;
		success.m_bAliveVerified = true;
		success.m_bFactionVerified = true;
		success.m_bGroupVerified = true;
		success.m_bProjectionVerified = true;
		success.m_bSeatVerified = !work.m_sAssignedVehicleSlotId.IsEmpty();
		HST_ForceSpawnResultState batch = FindProofBatch(batches, work.m_sResultId);
		if (work.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP)
			success.m_sNativeGroupId = "native_" + work.m_sResultId;
		else if (batch)
			success.m_sNativeGroupId = ResolveProofNativeGroup(batch, work.m_sGroupElementId);
		return success;
	}

	protected HST_ForceSpawnQueueCallbackResult CompleteProofSuccess(
		HST_ForceSpawnQueueService queue,
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueWorkItem work,
		int nowSecond,
		string entityId)
	{
		if (!queue || !work)
			return null;
		HST_ForceSpawnQueueSlotSuccess success = BuildProofSuccess(batches, work, entityId);
		return queue.CompleteSlotSuccess(batches, manifest, success, nowSecond);
	}

	protected HST_ForceSpawnQueueSlotFailure BuildProofFailure(
		HST_ForceSpawnQueueWorkItem work,
		string reason,
		bool retryable)
	{
		HST_ForceSpawnQueueSlotFailure failure = new HST_ForceSpawnQueueSlotFailure();
		if (!work)
			return failure;
		failure.m_sResultId = work.m_sResultId;
		failure.m_sProjectionId = work.m_sProjectionId;
		failure.m_sSlotId = work.m_sSlotId;
		failure.m_sFailureReason = reason;
		failure.m_iAttemptGeneration = work.m_iAttemptGeneration;
		failure.m_bRetryable = retryable;
		return failure;
	}

	protected string ResolveProofNativeGroup(HST_ForceSpawnResultState batch, string groupSlotId)
	{
		if (!batch)
			return "";
		HST_ForceSpawnSlotResultState groupSlot = batch.FindSlotResult(groupSlotId);
		if (groupSlot)
			return groupSlot.m_sNativeGroupId;
		return batch.m_sNativeGroupId;
	}

	protected array<ref HST_ForceSpawnResultState> BuildProofTerminalRows(string prefix, int count, int firstCompletedSecond)
	{
		array<ref HST_ForceSpawnResultState> rows = {};
		for (int index = 0; index < count; index++)
		{
			string token = string.Format("terminal_%1_%2", prefix, index);
			HST_ForceSpawnResultState batch = new HST_ForceSpawnResultState();
			batch.m_sResultId = "result_" + token;
			batch.m_sRequestId = "request_" + token;
			batch.m_sManifestId = "manifest_" + token;
			batch.m_sManifestHash = "hash_" + token;
			batch.m_sOperationId = "operation_" + token;
			batch.m_sForceId = "force_" + token;
			batch.m_sProjectionId = "projection_" + token;
			batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
			batch.m_iCreatedAtSecond = firstCompletedSecond;
			batch.m_iCompletedAtSecond = firstCompletedSecond + index;
			batch.m_iUpdatedAtSecond = batch.m_iCompletedAtSecond;
			rows.Insert(batch);
		}
		return rows;
	}

	protected HST_ForceSpawnResultState FindProofBatch(array<ref HST_ForceSpawnResultState> batches, string resultId)
	{
		if (!batches)
			return null;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (batch && batch.m_sResultId == resultId)
				return batch;
		}
		return null;
	}

	protected HST_ForceSpawnQueueWorkItem ResolveFirstWork(HST_ForceSpawnQueueTickResult tick)
	{
		return ResolveWorkAt(tick, 0);
	}

	protected HST_ForceSpawnQueueWorkItem ResolveWorkAt(HST_ForceSpawnQueueTickResult tick, int index)
	{
		if (!tick || !tick.m_aWorkItems || index < 0 || index >= tick.m_aWorkItems.Count())
			return null;
		return tick.m_aWorkItems[index];
	}

	protected HST_ForceSpawnQueueWorkItem ResolveWork(HST_ForceSpawnQueueTickResult tick, string slotId)
	{
		if (!tick || !tick.m_aWorkItems)
			return null;
		foreach (HST_ForceSpawnQueueWorkItem work : tick.m_aWorkItems)
		{
			if (work && work.m_sSlotId == slotId)
				return work;
		}
		return null;
	}

	protected int CountProofSlotKind(HST_ForceSpawnResultState batch, string slotKind)
	{
		int count;
		if (!batch || !batch.m_aSlotResults)
			return count;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotKind == slotKind)
				count++;
		}
		return count;
	}

	protected int CountRegisteredProofSlots(HST_ForceSpawnResultState batch)
	{
		int count;
		if (!batch || !batch.m_aSlotResults)
			return count;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED)
				count++;
		}
		return count;
	}

	protected bool ProofRegisteredEntityIdsUnique(HST_ForceSpawnResultState batch)
	{
		if (!batch || !batch.m_aSlotResults)
			return false;
		array<string> entityIds = {};
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED || slot.m_sEntityId.IsEmpty())
				return false;
			if (entityIds.Contains(slot.m_sEntityId))
				return false;
			entityIds.Insert(slot.m_sEntityId);
		}
		return entityIds.Count() == batch.m_iExpectedSlotCount;
	}

	protected bool ProofBatchPhysicalIdsClear(HST_ForceSpawnResultState batch)
	{
		if (!batch || !batch.m_sNativeGroupId.IsEmpty())
			return false;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot)
				continue;
			if (!slot.m_sEntityId.IsEmpty() || !slot.m_sAssignedVehicleEntityId.IsEmpty() || !slot.m_sNativeGroupId.IsEmpty())
				return false;
		}
		return true;
	}

	protected bool HasProofEvent(HST_CampaignState state, string eventId)
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

	protected string ResolveFirstResultId(HST_ForceSpawnQueueTickResult tick)
	{
		return ResolveWorkResultId(tick, 0);
	}

	protected string ResolveWorkResultId(HST_ForceSpawnQueueTickResult tick, int index)
	{
		HST_ForceSpawnQueueWorkItem work = ResolveWorkAt(tick, index);
		if (work)
			return work.m_sResultId;
		return "missing";
	}

	protected string ResolveFirstAction(HST_ForceSpawnQueueTickResult tick)
	{
		HST_ForceSpawnQueueWorkItem work = ResolveFirstWork(tick);
		if (work)
			return work.m_sAction;
		return "missing";
	}

	protected int ResolveFirstGeneration(HST_ForceSpawnQueueTickResult tick)
	{
		HST_ForceSpawnQueueWorkItem work = ResolveFirstWork(tick);
		if (work)
			return work.m_iAttemptGeneration;
		return -1;
	}

	protected int ResolveWorkCount(HST_ForceSpawnQueueTickResult tick)
	{
		if (!tick || !tick.m_aWorkItems)
			return 0;
		return tick.m_aWorkItems.Count();
	}

	protected int ResolveRemovedCount(HST_ForceSpawnQueueMaintenanceResult result)
	{
		if (!result)
			return -1;
		return result.m_iRemovedTerminalCount;
	}

	protected int ResolveSchema(HST_CampaignState state)
	{
		if (!state)
			return -1;
		return state.m_iSchemaVersion;
	}
}
