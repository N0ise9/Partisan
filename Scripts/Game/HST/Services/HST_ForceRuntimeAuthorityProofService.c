class HST_ForceRuntimeAuthorityProofReport
{
	bool m_bCasualtyIdempotencyExact;
	bool m_bPersistenceExact;
	bool m_bSurvivorReprojectionExact;
	bool m_bTerminalRosterExact;
	bool m_bSchema46MigrationExact;
	string m_sCasualtyEvidence;
	string m_sPersistenceEvidence;
	string m_sReprojectionEvidence;
	string m_sTerminalEvidence;
	string m_sMigrationEvidence;
}

class HST_ForceRuntimeAuthorityProofService
{
	protected ref HST_ForcePlanningIntegrityService m_Integrity = new HST_ForcePlanningIntegrityService();

	HST_ForceRuntimeAuthorityProofReport Run()
	{
		HST_ForceRuntimeAuthorityProofReport report = new HST_ForceRuntimeAuthorityProofReport();
		HST_CampaignState state = BuildState("runtime");
		HST_ForceManifestState manifest = BuildManifest("runtime", 2);
		state.m_aForceManifests.Insert(manifest);
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		HST_ForceSpawnResultState batch = EnqueueAndCompleteInitialProjection(state, manifest, queue);
		if (!batch)
		{
			report.m_sCasualtyEvidence = "initial exact projection did not reach successful handoff";
			return report;
		}

		HST_ForceSpawnSlotResultState firstMember = batch.FindSlotResult("runtime_member_0");
		HST_ForceSpawnQueueCallbackResult casualty = queue.ConfirmRegisteredMemberCasualty(
			state.m_aForceSpawnResults,
			manifest,
			batch.m_sResultId,
			batch.m_sProjectionId,
			firstMember.m_sSlotId,
			firstMember.m_sEntityId,
			104,
			"proof casualty");
		HST_ForceSpawnQueueCallbackResult casualtyReplay = queue.ConfirmRegisteredMemberCasualty(
			state.m_aForceSpawnResults,
			manifest,
			batch.m_sResultId,
			batch.m_sProjectionId,
			firstMember.m_sSlotId,
			firstMember.m_sEntityId,
			105,
			"proof casualty replay");
		report.m_bCasualtyIdempotencyExact = casualty && casualty.m_bAccepted && casualty.m_bStateChanged
			&& casualtyReplay && casualtyReplay.m_bAccepted && casualtyReplay.m_bAlreadyApplied
			&& firstMember.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
			&& firstMember.m_bEverAlive && firstMember.m_bCasualtyConfirmed
			&& queue.CountDurableLivingMemberSlots(batch) == 1
			&& queue.CountConfirmedCasualtyMemberSlots(batch) == 1;
		report.m_sCasualtyEvidence = string.Format(
			"accepted/replay %1/%2 | slot %3 | living/dead %4/%5 | lifecycle %6",
			casualty && casualty.m_bAccepted,
			casualtyReplay && casualtyReplay.m_bAlreadyApplied,
			firstMember.m_eStatus,
			queue.CountDurableLivingMemberSlots(batch),
			queue.CountConfirmedCasualtyMemberSlots(batch),
			firstMember.m_iLifecycleRevision);

		HST_ActiveGroupState group = BuildActiveGroup(state, manifest, batch);
		state.m_aActiveGroups.Insert(group);
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(state);
		HST_CampaignState restored = saveData.Restore();
		HST_ForceSpawnResultState restoredBatch = restored.FindForceSpawnResult(batch.m_sResultId);
		HST_ActiveGroupState restoredGroup = restored.FindActiveGroup(group.m_sGroupId);
		HST_ForceSpawnSlotResultState restoredDead;
		if (restoredBatch)
			restoredDead = restoredBatch.FindSlotResult(firstMember.m_sSlotId);
		report.m_bPersistenceExact = restored && restoredBatch && restoredGroup && restoredDead
			&& restored.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION
			&& restoredBatch.m_iSuccessfulHandoffCount == 1
			&& restoredDead.m_bCasualtyConfirmed && restoredDead.m_iCasualtyAtSecond == 104
			&& restoredGroup.m_bEverPopulated && restoredGroup.m_bSpawnCompleted
			&& restoredGroup.m_iDurableLivingInfantryCount == 1;
		report.m_sPersistenceEvidence = string.Format(
			"schema %1 | handoffs %2 | casualty %3@%4 | group lifecycle %5/%6 living %7",
			ResolveSchema(restored),
			ResolveHandoffCount(restoredBatch),
			restoredDead && restoredDead.m_bCasualtyConfirmed,
			ResolveCasualtySecond(restoredDead),
			restoredGroup && restoredGroup.m_bEverPopulated,
			restoredGroup && restoredGroup.m_bSpawnCompleted,
			ResolveDurableLiving(restoredGroup));

		if (restored && restoredBatch)
		{
			queue.ReconcileAfterRestore(restored.m_aForceSpawnResults, restored.m_aForceManifests, 106);
			HST_ForceManifestState restoredManifest = restored.FindForceManifest(manifest.m_sManifestId);
			HST_ForceSpawnSlotResultState restoredRetired = restoredBatch.FindSlotResult(firstMember.m_sSlotId);
			int invalidBatchRevision = restoredBatch.m_iLifecycleRevision;
			int invalidSlotRevision = restoredRetired.m_iLifecycleRevision;
			int invalidReprojectionCount = restoredBatch.m_iReprojectionCount;
			HST_EForceSpawnBatchStatus invalidBatchStatus = restoredBatch.m_eStatus;
			HST_EForceSpawnSlotStatus invalidSlotStatus = restoredRetired.m_eStatus;
			bool retiredEverAlive = restoredRetired.m_bEverAlive;
			restoredRetired.m_bEverAlive = false;
			HST_ForceSpawnQueueCallbackResult invalidPreflight = queue.CanRequeueSuccessfulProjectionForStrategicHold(
				restored.m_aForceSpawnResults,
				restoredManifest,
				restoredBatch.m_sResultId,
				restoredBatch.m_sProjectionId,
				106,
				300);
			HST_ForceSpawnQueueCallbackResult invalidRequeue = queue.RequeueSuccessfulProjectionForStrategicHold(
				restored.m_aForceSpawnResults,
				restoredManifest,
				restoredBatch.m_sResultId,
				restoredBatch.m_sProjectionId,
				106,
				300);
			bool invalidPreflightReadOnly = invalidPreflight && !invalidPreflight.m_bAccepted
				&& invalidRequeue && !invalidRequeue.m_bAccepted
				&& !invalidPreflight.m_sFailureReason.IsEmpty()
				&& invalidPreflight.m_sFailureReason == invalidRequeue.m_sFailureReason
				&& restoredBatch.m_eStatus == invalidBatchStatus
				&& restoredBatch.m_iLifecycleRevision == invalidBatchRevision
				&& restoredBatch.m_iReprojectionCount == invalidReprojectionCount
				&& restoredRetired.m_eStatus == invalidSlotStatus
				&& restoredRetired.m_iLifecycleRevision == invalidSlotRevision
				&& !restoredRetired.m_bEverAlive;
			restoredRetired.m_bEverAlive = retiredEverAlive;
			int validBatchRevision = restoredBatch.m_iLifecycleRevision;
			int validSlotRevision = restoredRetired.m_iLifecycleRevision;
			HST_ForceSpawnQueueCallbackResult validPreflight = queue.CanRequeueSuccessfulProjectionForStrategicHold(
				restored.m_aForceSpawnResults,
				restoredManifest,
				restoredBatch.m_sResultId,
				restoredBatch.m_sProjectionId,
				106,
				300);
			bool validPreflightReadOnly = validPreflight && validPreflight.m_bAccepted
				&& restoredBatch.m_eStatus == invalidBatchStatus
				&& restoredBatch.m_iLifecycleRevision == validBatchRevision
				&& restoredBatch.m_iReprojectionCount == invalidReprojectionCount
				&& restoredRetired.m_eStatus == invalidSlotStatus
				&& restoredRetired.m_iLifecycleRevision == validSlotRevision
				&& restoredRetired.m_bEverAlive == retiredEverAlive;
			HST_ForceSpawnQueueCallbackResult requeue = queue.RequeueSuccessfulProjectionAfterRestore(
				restored.m_aForceSpawnResults,
				restoredManifest,
				restoredBatch.m_sResultId,
				restoredBatch.m_sProjectionId,
				106,
				300);
			HST_ForceSpawnQueueTickResult rootTick = queue.AcquireWork(restored.m_aForceSpawnResults, restored.m_aForceManifests, 107);
			bool rootOnly = HasOnlyWorkSlot(rootTick, "runtime_group");
			CompleteWork(queue, restored.m_aForceSpawnResults, restored.FindForceManifest(manifest.m_sManifestId), FirstWork(rootTick), 107, "runtime_group_entity_r2", "runtime_native_r2");
			HST_ForceSpawnQueueTickResult memberTick = queue.AcquireWork(restored.m_aForceSpawnResults, restored.m_aForceManifests, 108);
			bool survivorOnly = HasOnlyWorkSlot(memberTick, "runtime_member_1");
			CompleteWork(queue, restored.m_aForceSpawnResults, restored.FindForceManifest(manifest.m_sManifestId), FirstWork(memberTick), 108, "runtime_member_1_entity_r2", "runtime_native_r2");
			HST_ForceSpawnQueueCallbackResult handoff = queue.CompleteProjectionHandoff(
				restored.m_aForceSpawnResults,
				restored.FindForceManifest(manifest.m_sManifestId),
				restoredBatch.m_sResultId,
				restoredBatch.m_sProjectionId,
				restoredBatch.m_iAttemptGeneration,
				109);
			HST_ForceSpawnSlotResultState retired = restoredBatch.FindSlotResult("runtime_member_0");
			HST_ForceSpawnSlotResultState survivor = restoredBatch.FindSlotResult("runtime_member_1");
			report.m_bSurvivorReprojectionExact = invalidPreflightReadOnly && validPreflightReadOnly
				&& requeue && requeue.m_bAccepted && rootOnly && survivorOnly
				&& handoff && handoff.m_bAccepted
				&& restoredBatch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
				&& restoredBatch.m_iSuccessfulHandoffCount == 2 && restoredBatch.m_iReprojectionCount == 1
				&& retired && retired.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				&& survivor && survivor.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
				&& queue.CountDurableLivingMemberSlots(restoredBatch) == 1;
			report.m_sReprojectionEvidence = string.Format(
				"preflight invalid/valid read-only %1/%2 | requeue/root/survivor/handoff %3/%4/%5/%6 | handoffs/reprojections %7/%8",
				invalidPreflightReadOnly,
				validPreflightReadOnly,
				requeue && requeue.m_bAccepted,
				rootOnly,
				survivorOnly,
				handoff && handoff.m_bAccepted,
				restoredBatch.m_iSuccessfulHandoffCount,
				restoredBatch.m_iReprojectionCount);
			report.m_sReprojectionEvidence = report.m_sReprojectionEvidence + string.Format(
				" | living/dead %1/%2",
				queue.CountDurableLivingMemberSlots(restoredBatch),
				queue.CountConfirmedCasualtyMemberSlots(restoredBatch));

			HST_ForceSpawnQueueCallbackResult lastCasualty = queue.ConfirmRegisteredMemberCasualty(
				restored.m_aForceSpawnResults,
				restored.FindForceManifest(manifest.m_sManifestId),
				restoredBatch.m_sResultId,
				restoredBatch.m_sProjectionId,
				survivor.m_sSlotId,
				survivor.m_sEntityId,
				110,
				"proof final casualty");
			report.m_bTerminalRosterExact = lastCasualty && lastCasualty.m_bAccepted
				&& queue.CountDurableLivingMemberSlots(restoredBatch) == 0
				&& queue.CountConfirmedCasualtyMemberSlots(restoredBatch) == 2;
			report.m_sTerminalEvidence = string.Format(
				"final accepted %1 | living/dead %2/%3 | batch lifecycle %4",
				lastCasualty && lastCasualty.m_bAccepted,
				queue.CountDurableLivingMemberSlots(restoredBatch),
				queue.CountConfirmedCasualtyMemberSlots(restoredBatch),
				restoredBatch.m_iLifecycleRevision);
		}

		RunSchema46MigrationProof(report);
		return report;
	}

	protected HST_ForceSpawnResultState EnqueueAndCompleteInitialProjection(
		HST_CampaignState state,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueService queue)
	{
		HST_ForceSpawnQueueRequest request = new HST_ForceSpawnQueueRequest();
		request.m_sResultId = "runtime_result";
		request.m_sRequestId = "runtime_request";
		request.m_sForceId = "runtime_force";
		request.m_sProjectionId = "runtime_projection";
		request.m_iPriority = 100;
		request.m_iMaxRetries = 2;
		request.m_iDeadlineSecond = 300;
		int batchCountBeforePreflight = state.m_aForceSpawnResults.Count();
		HST_ForceSpawnQueueEnqueueResult preflight = queue.CanEnqueue(state.m_aForceSpawnResults, manifest, request, 100);
		if (!preflight || !preflight.m_bSuccess || preflight.m_bStateChanged || preflight.m_Batch
			|| state.m_aForceSpawnResults.Count() != batchCountBeforePreflight)
			return null;
		HST_ForceSpawnQueueEnqueueResult enqueue = queue.Enqueue(state.m_aForceSpawnResults, manifest, request, 100);
		if (!enqueue || !enqueue.m_bSuccess)
			return null;

		HST_ForceSpawnQueueTickResult rootTick = queue.AcquireWork(state.m_aForceSpawnResults, state.m_aForceManifests, 101);
		if (!HasOnlyWorkSlot(rootTick, "runtime_group"))
			return null;
		CompleteWork(queue, state.m_aForceSpawnResults, manifest, FirstWork(rootTick), 101, "runtime_group_entity", "runtime_native");
		HST_ForceSpawnQueueTickResult memberTick = queue.AcquireWork(state.m_aForceSpawnResults, state.m_aForceManifests, 102);
		if (!HasWorkSlot(memberTick, "runtime_member_0") || !HasWorkSlot(memberTick, "runtime_member_1"))
			return null;
		foreach (HST_ForceSpawnQueueWorkItem work : memberTick.m_aWorkItems)
			CompleteWork(queue, state.m_aForceSpawnResults, manifest, work, 102, work.m_sSlotId + "_entity", "runtime_native");
		HST_ForceSpawnQueueCallbackResult handoff = queue.CompleteProjectionHandoff(
			state.m_aForceSpawnResults,
			manifest,
			enqueue.m_Batch.m_sResultId,
			enqueue.m_Batch.m_sProjectionId,
			enqueue.m_Batch.m_iAttemptGeneration,
			103);
		if (!handoff || !handoff.m_bAccepted)
			return null;
		return enqueue.m_Batch;
	}

	protected void CompleteWork(
		HST_ForceSpawnQueueService queue,
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueWorkItem work,
		int nowSecond,
		string entityId,
		string nativeGroupId)
	{
		if (!queue || !manifest || !work)
			return;
		HST_ForceSpawnQueueSlotSuccess success = new HST_ForceSpawnQueueSlotSuccess();
		success.m_sResultId = work.m_sResultId;
		success.m_sProjectionId = work.m_sProjectionId;
		success.m_sSlotId = work.m_sSlotId;
		success.m_sEntityId = entityId;
		success.m_sSpawnedPrefab = work.m_sPrefab;
		success.m_sNativeGroupId = nativeGroupId;
		success.m_iAttemptGeneration = work.m_iAttemptGeneration;
		success.m_bAliveVerified = true;
		success.m_bFactionVerified = true;
		success.m_bGroupVerified = true;
		success.m_bGameMasterVerified = true;
		success.m_bProjectionVerified = true;
		queue.CompleteSlotSuccess(batches, manifest, success, nowSecond);
	}

	protected HST_CampaignState BuildState(string token)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		state.m_iElapsedSeconds = 100;
		state.m_sPresetId = "proof_" + token;
		return state;
	}

	protected HST_ForceManifestState BuildManifest(string token, int memberCount)
	{
		HST_ForceManifestState manifest = new HST_ForceManifestState();
		manifest.m_sManifestId = token + "_manifest";
		manifest.m_sOperationId = token + "_operation";
		manifest.m_sQuoteId = token + "_quote";
		manifest.m_sCommandRequestId = token + "_command";
		manifest.m_sForceKind = "proof_force_runtime";
		manifest.m_sFactionRole = "resistance";
		manifest.m_sFactionKey = "FIA";
		manifest.m_sIntentId = token + "_intent";
		manifest.m_sSourceZoneId = token + "_source";
		manifest.m_sTargetZoneId = token + "_target";
		manifest.m_sGroupPrefab = token + "_group_prefab";
		manifest.m_sCatalogVersion = "proof_catalog_1";
		manifest.m_sPolicyId = "proof_exact_lifecycle";
		manifest.m_iRequestedMemberCount = memberCount;
		manifest.m_iAcceptedMemberCount = memberCount;
		manifest.m_iCreatedAtSecond = 100;
		manifest.m_bFrozen = true;
		HST_ForceManifestGroupState group = new HST_ForceManifestGroupState();
		group.m_sElementId = token + "_group";
		group.m_sCatalogEntryId = "proof_group";
		group.m_sPrefab = manifest.m_sGroupPrefab;
		group.m_sRole = "root";
		group.m_iExpectedMemberCount = memberCount;
		manifest.m_aGroups.Insert(group);
		for (int index = 0; index < memberCount; index++)
		{
			HST_ForceManifestMemberState member = new HST_ForceManifestMemberState();
			member.m_sSlotId = string.Format("%1_member_%2", token, index);
			member.m_sCatalogSlotId = string.Format("proof_slot_%1", index);
			member.m_sGroupElementId = group.m_sElementId;
			member.m_sPrefab = string.Format("%1_member_prefab_%2", token, index);
			member.m_sRole = "rifleman";
			member.m_iOrdinal = index;
			manifest.m_aMembers.Insert(member);
		}
		manifest.m_sManifestHash = m_Integrity.BuildManifestHash(manifest);
		return manifest;
	}

	protected HST_ActiveGroupState BuildActiveGroup(
		HST_CampaignState state,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = batch.m_sProjectionId;
		group.m_sOperationId = batch.m_sOperationId;
		group.m_sManifestId = batch.m_sManifestId;
		group.m_sSpawnResultId = batch.m_sResultId;
		group.m_sForceId = batch.m_sForceId;
		group.m_sProjectionId = batch.m_sProjectionId;
		group.m_sFactionKey = manifest.m_sFactionKey;
		group.m_sPrefab = manifest.m_sGroupPrefab;
		group.m_sRuntimeStatus = "support_active";
		group.m_sRuntimeEntityId = group.m_sGroupId;
		group.m_iInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iOriginalInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iSpawnedAgentCount = 1;
		group.m_iLastSeenAliveCount = 1;
		group.m_iSurvivorInfantryCount = 1;
		group.m_iDurableLivingInfantryCount = 1;
		group.m_iLifecycleRevision = 2;
		group.m_bSpawnAttempted = true;
		group.m_bSpawnedEntity = true;
		group.m_bEverPopulated = true;
		group.m_bSpawnCompleted = true;
		return group;
	}

	protected void RunSchema46MigrationProof(HST_ForceRuntimeAuthorityProofReport report)
	{
		HST_CampaignState legacyState = BuildState("legacy");
		legacyState.m_iSchemaVersion = 46;
		legacyState.m_iLastLoadedSchemaVersion = 46;
		HST_ForceManifestState manifest = BuildManifest("legacy", 2);
		legacyState.m_aForceManifests.Insert(manifest);
		HST_ForceSpawnResultState batch = new HST_ForceSpawnResultState();
		batch.m_sResultId = "legacy_result";
		batch.m_sRequestId = "legacy_request";
		batch.m_sManifestId = manifest.m_sManifestId;
		batch.m_sManifestHash = manifest.m_sManifestHash;
		batch.m_sOperationId = manifest.m_sOperationId;
		batch.m_sForceId = "legacy_force";
		batch.m_sProjectionId = "legacy_projection";
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
		batch.m_iExpectedSlotCount = 3;
		batch.m_iCompletedAtSecond = 120;
		AppendLegacyRegisteredSlot(batch, "legacy_group", HST_ForceSpawnQueueService.SLOT_KIND_GROUP, "legacy_group_entity", "legacy_native");
		AppendLegacyRegisteredSlot(batch, "legacy_member_0", HST_ForceSpawnQueueService.SLOT_KIND_MEMBER, "legacy_member_entity_0", "legacy_native");
		AppendLegacyRegisteredSlot(batch, "legacy_member_1", HST_ForceSpawnQueueService.SLOT_KIND_MEMBER, "legacy_member_entity_1", "legacy_native");
		legacyState.m_aForceSpawnResults.Insert(batch);
		HST_ActiveGroupState group = BuildActiveGroup(legacyState, manifest, batch);
		group.m_iDurableLivingInfantryCount = 0;
		group.m_iLifecycleRevision = 0;
		group.m_bEverPopulated = false;
		group.m_bSpawnCompleted = false;
		legacyState.m_aActiveGroups.Insert(group);

		HST_CampaignSaveData legacySave = new HST_CampaignSaveData();
		legacySave.Capture(legacyState);
		HST_CampaignState migrated = legacySave.Restore();
		HST_ForceSpawnResultState migratedBatch = migrated.FindForceSpawnResult(batch.m_sResultId);
		HST_ActiveGroupState migratedGroup = migrated.FindActiveGroup(group.m_sGroupId);
		HST_ForceSpawnSlotResultState migratedMember;
		if (migratedBatch)
			migratedMember = migratedBatch.FindSlotResult("legacy_member_0");
		report.m_bSchema46MigrationExact = migrated && migratedBatch && migratedGroup && migratedMember
			&& migrated.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION
			&& migratedBatch.m_iSuccessfulHandoffCount == 1
			&& migratedMember.m_bEverAlive && !migratedMember.m_bCasualtyConfirmed
			&& migratedGroup.m_bEverPopulated && migratedGroup.m_bSpawnCompleted
			&& migratedGroup.m_iDurableLivingInfantryCount == 2
			&& HasEvent(migrated, "migration_schema47_force_runtime_lifecycle");
		report.m_sMigrationEvidence = string.Format(
			"schema %1 | handoffs %2 | member ever/dead %3/%4 | group lifecycle/living %5/%6/%7 | event %8",
			ResolveSchema(migrated),
			ResolveHandoffCount(migratedBatch),
			migratedMember && migratedMember.m_bEverAlive,
			migratedMember && migratedMember.m_bCasualtyConfirmed,
			migratedGroup && migratedGroup.m_bEverPopulated,
			migratedGroup && migratedGroup.m_bSpawnCompleted,
			ResolveDurableLiving(migratedGroup),
			HasEvent(migrated, "migration_schema47_force_runtime_lifecycle"));
	}

	protected void AppendLegacyRegisteredSlot(HST_ForceSpawnResultState batch, string slotId, string slotKind, string entityId, string nativeGroupId)
	{
		HST_ForceSpawnSlotResultState slot = new HST_ForceSpawnSlotResultState();
		slot.m_sSlotId = slotId;
		slot.m_sSlotKind = slotKind;
		slot.m_sEntityId = entityId;
		slot.m_sNativeGroupId = nativeGroupId;
		slot.m_sProjectionId = batch.m_sProjectionId;
		slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
		slot.m_bAliveVerified = true;
		slot.m_bFactionVerified = true;
		slot.m_bGroupVerified = true;
		slot.m_bGameMasterVerified = true;
		slot.m_bProjectionVerified = true;
		batch.m_aSlotResults.Insert(slot);
	}

	protected HST_ForceSpawnQueueWorkItem FirstWork(HST_ForceSpawnQueueTickResult tick)
	{
		if (!tick || !tick.m_aWorkItems || tick.m_aWorkItems.Count() == 0)
			return null;
		return tick.m_aWorkItems[0];
	}

	protected bool HasOnlyWorkSlot(HST_ForceSpawnQueueTickResult tick, string slotId)
	{
		return tick && tick.m_aWorkItems && tick.m_aWorkItems.Count() == 1
			&& tick.m_aWorkItems[0] && tick.m_aWorkItems[0].m_sSlotId == slotId;
	}

	protected bool HasWorkSlot(HST_ForceSpawnQueueTickResult tick, string slotId)
	{
		if (!tick || !tick.m_aWorkItems)
			return false;
		foreach (HST_ForceSpawnQueueWorkItem work : tick.m_aWorkItems)
		{
			if (work && work.m_sSlotId == slotId)
				return true;
		}
		return false;
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

	protected int ResolveSchema(HST_CampaignState state)
	{
		if (!state)
			return -1;
		return state.m_iSchemaVersion;
	}

	protected int ResolveHandoffCount(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return -1;
		return batch.m_iSuccessfulHandoffCount;
	}

	protected int ResolveCasualtySecond(HST_ForceSpawnSlotResultState slot)
	{
		if (!slot)
			return -1;
		return slot.m_iCasualtyAtSecond;
	}

	protected int ResolveDurableLiving(HST_ActiveGroupState group)
	{
		if (!group)
			return -1;
		return group.m_iDurableLivingInfantryCount;
	}
}
