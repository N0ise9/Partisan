class HST_ForceSpawnAdapterHandle
{
	string m_sResultId;
	string m_sProjectionId;
	string m_sForceId;
	string m_sSlotId;
	string m_sSlotKind;
	string m_sPrefab;
	string m_sEntityId;
	string m_sNativeGroupId;
	int m_iAttemptGeneration;
	bool m_bHandedOff;
	IEntity m_Entity;
}

class HST_ForceSpawnAdapterTickResult
{
	bool m_bStateChanged;
	bool m_bRuntimeChanged;
	int m_iAcquiredBatchCount;
	int m_iAcquiredActionCount;
	int m_iSpawnedCount;
	int m_iCleanedCount;
	int m_iDeferredCount;
	int m_iFailedCount;
	int m_iHandedOffCount;
	int m_iHandoffRefusedCount;
	int m_iCasualtyCount;
	int m_iEliminatedCount;
	string m_sSummary;
	ref array<string> m_aEvidence = {};
}

class HST_ForceSpawnAdapterRetireResult
{
	bool m_bSuccess;
	bool m_bRuntimeChanged;
	int m_iMemberCount;
	int m_iRootCount;
	string m_sFailureReason;
}

class HST_ForceSpawnAdapterService
{
	static const string ADAPTER_MODE = "exact_spawn_queue";
	static const string UNSUPPORTED_MANIFEST_REASON = "physical adapter supports one exact infantry group without vehicle or asset slots";
	static const int MAX_RECONCILE_BATCHES_PER_TICK = 4;
	static const int MAX_HANDLE_RECONCILE_PER_TICK = 8;
	static const int MAX_LIFECYCLE_HANDLES_PER_TICK = 16;

	protected ref array<ref HST_ForceSpawnAdapterHandle> m_aHandles = {};
	protected int m_iTickCount;
	protected int m_iSpawnedTotal;
	protected int m_iCleanedTotal;
	protected int m_iFailedTotal;
	protected int m_iDeferredTotal;
	protected int m_iHandoffRefusedTotal;
	protected int m_iReconcileCursor;
	protected int m_iHandleReconcileCursor;
	protected int m_iLifecycleReconcileCursor;
	protected string m_sLastSummary;
	protected string m_sLastFailureEvidence;

	HST_ForceSpawnAdapterTickResult Tick(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_PhysicalWarService physicalWar,
		int nowSecond)
	{
		HST_ForceSpawnAdapterTickResult result = new HST_ForceSpawnAdapterTickResult();
		if (!state || !queue || !physicalWar)
		{
			result.m_sSummary = "force spawn adapter unavailable: state, queue, or physical-war bridge missing";
			return result;
		}

		ReconcileHandedOffMemberLifecycle(state, queue, physicalWar, nowSecond, result);
		ReconcileZeroLivingProjectionCleanup(state, queue, physicalWar, nowSecond, result);
		ReconcileDeletedStagedHandles(state, queue, nowSecond, result);
		HST_ForceSpawnQueueTickResult acquired = queue.AcquireWork(state.m_aForceSpawnResults, state.m_aForceManifests, nowSecond);
		if (!acquired)
		{
			result.m_sSummary = "force spawn adapter unavailable: queue acquisition failed";
			return result;
		}

		m_iTickCount++;
		result.m_bStateChanged = result.m_bStateChanged || acquired.m_bStateChanged;
		result.m_iAcquiredBatchCount = acquired.m_iBatchesSelected;
		result.m_iAcquiredActionCount = acquired.m_iSlotsSelected;
		array<string> touchedResultIds = {};
		ExecuteCleanupPass(state, queue, physicalWar, acquired.m_aWorkItems, nowSecond, touchedResultIds, result);
		ExecuteSpawnPass(state, queue, physicalWar, acquired.m_aWorkItems, nowSecond, touchedResultIds, result);
		AppendBoundedReconcileResultIds(state, touchedResultIds);
		FinalizeTouchedBatches(state, queue, physicalWar, nowSecond, touchedResultIds, result);
		UpdateTotals(result);
		result.m_sSummary = BuildTickSummary(result);
		m_sLastSummary = result.m_sSummary;
		return result;
	}

	HST_ForceSpawnAdapterTickResult ReconcileExactInfantryAuthorityForPersistence(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_PhysicalWarService physicalWar,
		int nowSecond)
	{
		HST_ForceSpawnAdapterTickResult result = new HST_ForceSpawnAdapterTickResult();
		if (!state || !queue || !physicalWar)
		{
			result.m_iFailedCount = 1;
			result.m_sSummary = "exact infantry persistence reconciliation is unavailable";
			return result;
		}

		ReconcileHandedOffMemberLifecycle(state, queue, physicalWar, nowSecond, result, true);
		ReconcileZeroLivingProjectionCleanup(state, queue, physicalWar, nowSecond, result);
		result.m_sSummary = string.Format(
			"exact infantry persistence reconciliation | casualties %1 | eliminated %2 | failures %3",
			result.m_iCasualtyCount,
			result.m_iEliminatedCount,
			result.m_iFailedCount);
		return result;
	}

	HST_ForceSpawnAdapterTickResult ReconcileExactInfantryProjectionAuthority(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_PhysicalWarService physicalWar,
		int nowSecond,
		string projectionId)
	{
		HST_ForceSpawnAdapterTickResult result = new HST_ForceSpawnAdapterTickResult();
		if (!state || !queue || !physicalWar || projectionId.IsEmpty())
		{
			result.m_iFailedCount = 1;
			result.m_sSummary = "exact infantry projection reconciliation is unavailable";
			return result;
		}
		ReconcileHandedOffMemberLifecycle(state, queue, physicalWar, nowSecond, result, true, projectionId);
		ReconcileZeroLivingProjectionCleanup(state, queue, physicalWar, nowSecond, result, projectionId);
		result.m_sSummary = string.Format(
			"exact infantry projection reconciliation | projection %1 | casualties %2 | eliminated %3 | failures %4",
			projectionId,
			result.m_iCasualtyCount,
			result.m_iEliminatedCount,
			result.m_iFailedCount);
		return result;
	}

	protected bool ValidateExactLivingMemberBindingsForPersistence(
		HST_ForceSpawnResultState batch,
		HST_ForceSpawnQueueService queue,
		HST_PhysicalWarService physicalWar,
		out string failure)
	{
		failure = "";
		if (!batch || !queue || !physicalWar || batch.m_sResultId.IsEmpty()
			|| batch.m_sProjectionId.IsEmpty()
			|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_bStrategicProjectionHeld)
		{
			failure = "exact physical infantry batch is unavailable or not live-authoritative";
			return false;
		}

		int expectedLiving = queue.CountDurableLivingMemberSlots(batch);
		int matchedLiving;
		HST_ForceSpawnSlotResultState rootSlot;
		int rootSlotCount;
		foreach (HST_ForceSpawnSlotResultState candidateRootSlot : batch.m_aSlotResults)
		{
			if (!candidateRootSlot || candidateRootSlot.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_GROUP)
				continue;
			rootSlotCount++;
			rootSlot = candidateRootSlot;
		}
		HST_ForceSpawnAdapterHandle rootHandle;
		int rootHandleCount;
		foreach (HST_ForceSpawnAdapterHandle candidateRootHandle : m_aHandles)
		{
			if (!candidateRootHandle || candidateRootHandle.m_sResultId != batch.m_sResultId
				|| candidateRootHandle.m_sProjectionId != batch.m_sProjectionId
				|| candidateRootHandle.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_GROUP)
				continue;
			rootHandleCount++;
			rootHandle = candidateRootHandle;
		}
		if (rootSlotCount != 1 || !rootSlot
			|| rootSlot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
			|| rootSlot.m_sEntityId.IsEmpty() || rootHandleCount != 1 || !rootHandle
			|| !rootHandle.m_bHandedOff || rootHandle.m_sSlotId != rootSlot.m_sSlotId
			|| rootHandle.m_sEntityId != rootSlot.m_sEntityId || !rootHandle.m_Entity
			|| rootHandle.m_Entity.IsDeleted() || !SCR_AIGroup.Cast(rootHandle.m_Entity))
		{
			failure = "exact physical infantry projection lacks one live handed-off group root";
			return false;
		}
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				|| !slot.m_bEverAlive || slot.m_bCasualtyConfirmed)
				continue;
			if (slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
				|| slot.m_sEntityId.IsEmpty())
			{
				failure = "durable living member has no registered runtime identity " + slot.m_sSlotId;
				return false;
			}
			HST_ForceSpawnAdapterHandle matchedHandle;
			int handleMatches;
			foreach (HST_ForceSpawnAdapterHandle handle : m_aHandles)
			{
				if (!handle || handle.m_sResultId != batch.m_sResultId
					|| handle.m_sProjectionId != batch.m_sProjectionId
					|| handle.m_sSlotId != slot.m_sSlotId)
					continue;
				handleMatches++;
				matchedHandle = handle;
			}
			if (handleMatches != 1 || !matchedHandle || !matchedHandle.m_bHandedOff
				|| matchedHandle.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				|| matchedHandle.m_sEntityId != slot.m_sEntityId
				|| !matchedHandle.m_Entity || matchedHandle.m_Entity.IsDeleted()
				|| !physicalWar.IsForceSpawnRuntimeMemberAlive(matchedHandle.m_Entity))
			{
				failure = "durable living member lacks one live handed-off adapter binding " + slot.m_sSlotId;
				return false;
			}
			matchedLiving++;
		}

		foreach (HST_ForceSpawnAdapterHandle candidate : m_aHandles)
		{
			if (!candidate || candidate.m_sResultId != batch.m_sResultId
				|| candidate.m_sProjectionId != batch.m_sProjectionId
				|| candidate.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
				continue;
			if (!candidate.m_bHandedOff)
			{
				failure = "physical infantry projection retains an unhanded member binding " + candidate.m_sSlotId;
				return false;
			}
			bool durableOwner;
			foreach (HST_ForceSpawnSlotResultState ownerSlot : batch.m_aSlotResults)
			{
				if (ownerSlot && ownerSlot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
					&& ownerSlot.m_sSlotId == candidate.m_sSlotId && ownerSlot.m_bEverAlive
					&& !ownerSlot.m_bCasualtyConfirmed)
				{
					durableOwner = true;
					break;
				}
			}
			if (!durableOwner)
			{
				failure = "handed-off adapter member has no durable living roster owner " + candidate.m_sSlotId;
				return false;
			}
		}
		if (matchedLiving != expectedLiving)
		{
			failure = string.Format("live binding count %1 does not match durable living roster %2", matchedLiving, expectedLiving);
			return false;
		}
		return true;
	}

	bool ValidateExactLivingProjectionBindingsForPersistence(
		HST_CampaignState state,
		HST_ForceSpawnResultState batch,
		HST_ForceSpawnQueueService queue,
		HST_PhysicalWarService physicalWar,
		out string failure)
	{
		if (!ValidateExactLivingMemberBindingsForPersistence(batch, queue, physicalWar, failure))
			return false;
		HST_ActiveGroupState activeGroup = FindActiveGroupForBatch(state, batch);
		if (!activeGroup)
		{
			failure = "exact physical infantry batch has no unique active-group owner";
			return false;
		}
		SCR_AIGroup registeredRoot = physicalWar.GetForceSpawnGroupRoot(activeGroup);
		HST_ForceSpawnAdapterHandle rootHandle;
		array<string> entityIds = {};
		array<IEntity> entities = {};
		int relevantHandles;
		foreach (HST_ForceSpawnAdapterHandle handle : m_aHandles)
		{
			if (!handle)
				continue;
			bool resultMatches = handle.m_sResultId == batch.m_sResultId;
			bool projectionMatches = handle.m_sProjectionId == batch.m_sProjectionId;
			if (!resultMatches && !projectionMatches)
				continue;
			if (resultMatches != projectionMatches)
			{
				failure = "exact physical infantry projection contains a cross-key adapter claimant " + handle.m_sSlotId;
				return false;
			}
			relevantHandles++;
			if (!handle.m_bHandedOff || handle.m_sEntityId.IsEmpty() || !handle.m_Entity
				|| handle.m_Entity.IsDeleted() || entityIds.Contains(handle.m_sEntityId)
				|| entities.Contains(handle.m_Entity))
			{
				failure = "exact physical infantry projection contains an ambiguous or aliased adapter binding " + handle.m_sSlotId;
				return false;
			}
			if (handle.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_GROUP
				&& handle.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
			{
				failure = "exact physical infantry projection contains a foreign adapter slot " + handle.m_sSlotId;
				return false;
			}
			entityIds.Insert(handle.m_sEntityId);
			entities.Insert(handle.m_Entity);
			if (handle.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP)
				rootHandle = handle;
		}
		int expectedLiving = queue.CountDurableLivingMemberSlots(batch);
		if (relevantHandles != expectedLiving + 1 || !rootHandle
			|| rootHandle.m_Entity != registeredRoot
			|| physicalWar.CountForceSpawnRuntimeMembers(activeGroup) != expectedLiving)
		{
			failure = string.Format(
				"exact physical infantry runtime cardinality conflicts: bindings %1 expected %2 registered %3",
				relevantHandles,
				expectedLiving + 1,
				physicalWar.CountForceSpawnRuntimeMembers(activeGroup));
			return false;
		}
		return true;
	}

	protected void ReconcileHandedOffMemberLifecycle(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_PhysicalWarService physicalWar,
		int nowSecond,
		HST_ForceSpawnAdapterTickResult result,
		bool exhaustive = false,
		string projectionFilterId = "")
	{
		if (!state || !queue || !physicalWar || !result)
			return;
		int inspected;
		int scanned;
		int handleCount = m_aHandles.Count();
		if (handleCount <= 0)
		{
			m_iLifecycleReconcileCursor = 0;
			return;
		}
		m_iLifecycleReconcileCursor = HST_DefaultCatalog.PositiveMod(m_iLifecycleReconcileCursor, handleCount);
		array<string> eliminatedProjectionIds = {};
		array<ref HST_ForceSpawnAdapterHandle> detachedHandles = {};
		for (scanned = 0; scanned < handleCount; scanned++)
		{
			if (!exhaustive && inspected >= MAX_LIFECYCLE_HANDLES_PER_TICK)
				break;
			int handleIndex = HST_DefaultCatalog.PositiveMod(m_iLifecycleReconcileCursor + scanned, handleCount);
			HST_ForceSpawnAdapterHandle handle = m_aHandles[handleIndex];
			if (!handle || !handle.m_bHandedOff || handle.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				|| (!projectionFilterId.IsEmpty() && handle.m_sProjectionId != projectionFilterId))
				continue;
			inspected++;
			if (!handle.m_Entity || handle.m_Entity.IsDeleted())
			{
				if (exhaustive)
				{
					result.m_iFailedCount++;
					result.m_aEvidence.Insert("exact handed-off member binding disappeared without casualty proof " + handle.m_sSlotId);
				}
				continue;
			}
			if (physicalWar.IsForceSpawnRuntimeMemberAlive(handle.m_Entity))
				continue;

			HST_ForceSpawnResultState batch = state.FindForceSpawnResult(handle.m_sResultId);
			if (!batch || batch.m_sProjectionId != handle.m_sProjectionId
				|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
				continue;
			HST_ForceSpawnQueueCallbackResult casualty = queue.ConfirmRegisteredMemberCasualty(
				state.m_aForceSpawnResults,
				state.FindForceManifest(batch.m_sManifestId),
				handle.m_sResultId,
				handle.m_sProjectionId,
				handle.m_sSlotId,
				handle.m_sEntityId,
				nowSecond,
				"authoritative runtime life state confirmed exact member death");
			if (!casualty || !casualty.m_bAccepted)
			{
				result.m_iFailedCount++;
				result.m_aEvidence.Insert("exact member casualty callback rejected " + handle.m_sSlotId + ": " + ResolveCallbackFailure(casualty, "unknown casualty callback failure"));
				continue;
			}
			HST_ActiveGroupState activeGroup = FindActiveGroupForBatch(state, batch);
			if (!activeGroup)
			{
				result.m_iFailedCount++;
				result.m_aEvidence.Insert("exact casualty has no canonical active-group projection " + handle.m_sProjectionId);
				continue;
			}
			int livingMembers = queue.CountDurableLivingMemberSlots(batch);
			activeGroup.m_iDurableLivingInfantryCount = livingMembers;
			activeGroup.m_iSpawnedAgentCount = livingMembers;
			activeGroup.m_iLastSeenAliveCount = livingMembers;
			activeGroup.m_iSurvivorInfantryCount = livingMembers;
			if (casualty.m_bStateChanged)
			{
				activeGroup.m_iLastCasualtySecond = nowSecond;
				activeGroup.m_iLifecycleRevision++;
				result.m_bStateChanged = true;
				result.m_iCasualtyCount++;
				result.m_aEvidence.Insert(string.Format("exact member casualty %1 | projection %2 | living %3", handle.m_sSlotId, handle.m_sProjectionId, livingMembers));
			}
			string detachmentFailure;
			if (!physicalWar.DetachConfirmedDeadForceSpawnMember(state, activeGroup, handle.m_Entity, detachmentFailure))
			{
				result.m_iFailedCount++;
				result.m_aEvidence.Insert("exact casualty corpse detachment refused " + handle.m_sSlotId + ": " + detachmentFailure);
				continue;
			}
			if (livingMembers > 0 || eliminatedProjectionIds.Contains(handle.m_sProjectionId))
			{
				detachedHandles.Insert(handle);
				continue;
			}
			string bindingConflict = FindProjectionBindingConflict(batch);
			if (!bindingConflict.IsEmpty())
			{
				result.m_iFailedCount++;
				result.m_aEvidence.Insert("exact force terminal cleanup blocked: " + bindingConflict);
				continue;
			}

			string eliminationFailure;
			if (!physicalWar.FinalizeEliminatedForceSpawnProjection(state, activeGroup, nowSecond, eliminationFailure))
			{
				result.m_iFailedCount++;
				result.m_aEvidence.Insert("exact force terminal cleanup refused " + handle.m_sProjectionId + ": " + eliminationFailure);
				continue;
			}
			eliminatedProjectionIds.Insert(handle.m_sProjectionId);
			result.m_bRuntimeChanged = true;
			result.m_iEliminatedCount++;
			result.m_aEvidence.Insert("exact force eliminated and runtime root retired " + handle.m_sProjectionId);
		}

		foreach (string projectionId : eliminatedProjectionIds)
			RemoveProjectionBindings(projectionId);
		foreach (HST_ForceSpawnAdapterHandle detachedHandle : detachedHandles)
			RemoveHandle(detachedHandle);
		if (exhaustive || m_aHandles.Count() <= 0)
			m_iLifecycleReconcileCursor = 0;
		else
			m_iLifecycleReconcileCursor = HST_DefaultCatalog.PositiveMod(
				m_iLifecycleReconcileCursor + Math.Max(1, scanned),
				m_aHandles.Count());
	}

	protected void ReconcileZeroLivingProjectionCleanup(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_PhysicalWarService physicalWar,
		int nowSecond,
		HST_ForceSpawnAdapterTickResult result,
		string projectionFilterId = "")
	{
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_sSpawnResultId.IsEmpty()
				|| (!projectionFilterId.IsEmpty() && activeGroup.m_sProjectionId != projectionFilterId)
				|| activeGroup.m_sRuntimeStatus == "eliminated"
				|| !activeGroup.m_bEverPopulated || !activeGroup.m_bSpawnCompleted)
				continue;
			HST_ForceSpawnResultState batch = state.FindForceSpawnResult(activeGroup.m_sSpawnResultId);
			if (!batch || batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
				|| queue.CountDurableLivingMemberSlots(batch) > 0)
				continue;
			string bindingConflict = FindProjectionBindingConflict(batch);
			if (!bindingConflict.IsEmpty())
			{
				result.m_iFailedCount++;
				result.m_aEvidence.Insert("exact force zero-living cleanup blocked: " + bindingConflict);
				continue;
			}
			string failure;
			if (!physicalWar.FinalizeEliminatedForceSpawnProjection(state, activeGroup, nowSecond, failure))
			{
				result.m_iFailedCount++;
				result.m_aEvidence.Insert("exact force zero-living cleanup pending " + activeGroup.m_sProjectionId + ": " + failure);
				continue;
			}
			RemoveProjectionBindings(activeGroup.m_sProjectionId);
			result.m_bStateChanged = true;
			result.m_bRuntimeChanged = true;
			result.m_iEliminatedCount++;
			result.m_aEvidence.Insert("exact force zero-living cleanup completed " + activeGroup.m_sProjectionId);
		}
	}

	protected void ExecuteCleanupPass(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_PhysicalWarService physicalWar,
		array<ref HST_ForceSpawnQueueWorkItem> workItems,
		int nowSecond,
		array<string> touchedResultIds,
		HST_ForceSpawnAdapterTickResult result)
	{
		foreach (HST_ForceSpawnQueueWorkItem work : workItems)
		{
			if (!work || work.m_sAction != HST_ForceSpawnQueueService.ACTION_CLEANUP)
				continue;
			TouchResultId(touchedResultIds, work.m_sResultId);
			ExecuteCleanupWork(state, queue, physicalWar, work, nowSecond, result);
		}
	}

	protected void ExecuteSpawnPass(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_PhysicalWarService physicalWar,
		array<ref HST_ForceSpawnQueueWorkItem> workItems,
		int nowSecond,
		array<string> touchedResultIds,
		HST_ForceSpawnAdapterTickResult result)
	{
		foreach (HST_ForceSpawnQueueWorkItem work : workItems)
		{
			if (!work || work.m_sAction != HST_ForceSpawnQueueService.ACTION_SPAWN)
				continue;
			TouchResultId(touchedResultIds, work.m_sResultId);
			HST_ForceSpawnResultState batch = state.FindForceSpawnResult(work.m_sResultId);
			if (!batch || batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS)
				continue;
			ExecuteSpawnWork(state, queue, physicalWar, work, nowSecond, result);
		}
	}

	protected void ExecuteSpawnWork(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_PhysicalWarService physicalWar,
		HST_ForceSpawnQueueWorkItem work,
		int nowSecond,
		HST_ForceSpawnAdapterTickResult result)
	{
		HST_ActiveGroupState activeGroup;
		HST_ForceManifestState manifest;
		string contextFailure = ResolveExecutionContext(state, work, activeGroup, manifest);
		if (!contextFailure.IsEmpty())
		{
			FailWork(state, queue, manifest, work, nowSecond, contextFailure, false, result);
			return;
		}

		string supportFailure = ValidateSupportedManifest(manifest);
		if (!supportFailure.IsEmpty())
		{
			FailWork(state, queue, manifest, work, nowSecond, supportFailure, false, result);
			return;
		}

		if (!GetGame() || !GetGame().GetWorld())
		{
			DeferWork(state, queue, manifest, work, nowSecond, "world unavailable for exact force spawn", result);
			return;
		}
		if (work.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP)
		{
			SpawnGroupRoot(state, queue, physicalWar, activeGroup, manifest, work, nowSecond, result);
			return;
		}
		if (work.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
		{
			SpawnGroupMember(state, queue, physicalWar, activeGroup, manifest, work, nowSecond, result);
			return;
		}
		FailWork(state, queue, manifest, work, nowSecond, "unsupported exact force slot kind: " + work.m_sSlotKind, false, result);
	}

	protected void SpawnGroupRoot(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_PhysicalWarService physicalWar,
		HST_ActiveGroupState activeGroup,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueWorkItem work,
		int nowSecond,
		HST_ForceSpawnAdapterTickResult result)
	{
		ResourceName resourceName = work.m_sPrefab;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded || !loaded.IsValid())
		{
			FailWork(state, queue, manifest, work, nowSecond, "exact group-root prefab resource is unavailable", false, result);
			return;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		vector spawnPosition;
		if (!TryResolveGroupSpawnPosition(activeGroup, spawnPosition))
		{
			DeferWork(state, queue, manifest, work, nowSecond, "no safe dry-ground position resolved for exact group root", result);
			return;
		}
		params.Transform[3] = spawnPosition;
		SCR_AIGroup.IgnoreSpawning(true);
		IEntity entity = GetGame().SpawnEntityPrefabEx(resourceName, false, GetGame().GetWorld(), params);
		SCR_AIGroup.IgnoreSpawning(false);
		SCR_AIGroup root = SCR_AIGroup.Cast(entity);
		if (!root)
		{
			DeleteUnregisteredEntity(entity);
			FailWork(state, queue, manifest, work, nowSecond, "exact group-root prefab did not create SCR_AIGroup", false, result);
			return;
		}

		root.SetOrigin(params.Transform[3]);
		root.SetAngles(HST_WorldPositionService.BuildEntitySetAnglesFromYawVector(HST_WorldPositionService.BuildUprightAngles(0)));
		string actualPrefab = ResolveEntityPrefabName(root);
		if (actualPrefab != work.m_sPrefab)
		{
			DeleteUnregisteredEntity(root);
			FailWork(state, queue, manifest, work, nowSecond, "spawned group-root prefab differs from frozen manifest", false, result);
			return;
		}

		string entityId = ResolveReplicatedEntityId(root);
		if (entityId.IsEmpty())
		{
			DeleteUnregisteredEntity(root);
			FailWork(state, queue, manifest, work, nowSecond, "spawned group root has no valid replication id", false, result);
			return;
		}

		HST_ForceSpawnAdapterHandle handle = BuildHandle(work, root, actualPrefab, entityId, "native_group_" + entityId);
		if (!RegisterHandle(handle))
		{
			DeleteUnregisteredEntity(root);
			FailWork(state, queue, manifest, work, nowSecond, "group-root runtime binding conflicts with another slot", false, result);
			return;
		}
		string bridgeFailure;
		if (!physicalWar.TryRegisterForceSpawnGroupRoot(state, activeGroup, root, bridgeFailure))
		{
			RemoveHandle(handle);
			DeleteUnregisteredEntity(root);
			FailWork(state, queue, manifest, work, nowSecond, "group-root registration failed: " + bridgeFailure, false, result);
			return;
		}

		HST_ForceSpawnQueueSlotSuccess success = BuildSuccess(work, handle, true, true);
		HST_ForceSpawnQueueCallbackResult callback = queue.CompleteSlotSuccess(state.m_aForceSpawnResults, manifest, success, nowSecond);
		if (!callback || !callback.m_bAccepted)
		{
			string callbackReason = ResolveCallbackFailure(callback, "group-root success callback rejected");
			if (RollbackPhysicalRegistration(physicalWar, activeGroup, handle))
				FailWork(state, queue, manifest, work, nowSecond, callbackReason, false, result);
			else
				FailWorkWithHandle(state, queue, manifest, work, handle, nowSecond, callbackReason, result);
			return;
		}

		activeGroup.m_bSpawnAttempted = true;
		activeGroup.m_sSpawnFallbackMode = AppendModeToken(activeGroup.m_sSpawnFallbackMode, ADAPTER_MODE);
		activeGroup.m_sSpawnFailureReason = "exact group root registered; awaiting required manifest members";
		result.m_bStateChanged = true;
		result.m_bRuntimeChanged = true;
		result.m_iSpawnedCount++;
		result.m_aEvidence.Insert(string.Format("group root %1 registered as %2", work.m_sSlotId, entityId));
	}

	protected void SpawnGroupMember(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_PhysicalWarService physicalWar,
		HST_ActiveGroupState activeGroup,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueWorkItem work,
		int nowSecond,
		HST_ForceSpawnAdapterTickResult result)
	{
		HST_ForceSpawnAdapterHandle rootHandle = FindDurableHandle(work.m_sResultId, work.m_sProjectionId, work.m_sGroupElementId);
		SCR_AIGroup root;
		if (rootHandle)
			root = SCR_AIGroup.Cast(rootHandle.m_Entity);
		if (!root || root.IsDeleted())
		{
			DeferWork(state, queue, manifest, work, nowSecond, "exact member is waiting for its registered group root", result);
			return;
		}

		string budgetFailure;
		if (!physicalWar.CanSpawnForceSpawnGroupMember(activeGroup, budgetFailure))
		{
			DeferWork(state, queue, manifest, work, nowSecond, "AIWorld capacity deferred exact member: " + budgetFailure, result);
			return;
		}

		ResourceName resourceName = work.m_sPrefab;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded || !loaded.IsValid())
		{
			FailWork(state, queue, manifest, work, nowSecond, "exact member prefab resource is unavailable", false, result);
			return;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		vector spawnPosition;
		if (!TryResolveMemberSpawnPosition(activeGroup, root, work.m_iOrdinal, spawnPosition))
		{
			DeferWork(state, queue, manifest, work, nowSecond, "no safe dry-ground position resolved for exact member", result);
			return;
		}
		params.Transform[3] = spawnPosition;
		IEntity member = GetGame().SpawnEntityPrefabEx(resourceName, false, GetGame().GetWorld(), params);
		if (!member)
		{
			FailWork(state, queue, manifest, work, nowSecond, "engine did not create exact member prefab", true, result);
			return;
		}
		member.SetOrigin(params.Transform[3]);

		string actualPrefab = ResolveEntityPrefabName(member);
		if (actualPrefab != work.m_sPrefab)
		{
			DeleteUnregisteredEntity(member);
			FailWork(state, queue, manifest, work, nowSecond, "spawned member prefab differs from frozen manifest", false, result);
			return;
		}

		string entityId = ResolveReplicatedEntityId(member);
		if (entityId.IsEmpty())
		{
			DeleteUnregisteredEntity(member);
			FailWork(state, queue, manifest, work, nowSecond, "spawned member has no valid replication id", false, result);
			return;
		}

		HST_ForceSpawnAdapterHandle handle = BuildHandle(work, member, actualPrefab, entityId, rootHandle.m_sNativeGroupId);
		if (!RegisterHandle(handle))
		{
			DeleteUnregisteredEntity(member);
			FailWork(state, queue, manifest, work, nowSecond, "member runtime binding conflicts with another slot", false, result);
			return;
		}
		string bridgeFailure;
		if (!physicalWar.TryRegisterForceSpawnGroupMember(state, activeGroup, root, member, work.m_iOrdinal, bridgeFailure))
		{
			RemoveHandle(handle);
			DeleteUnregisteredEntity(member);
			FailWork(state, queue, manifest, work, nowSecond, "member registration failed: " + bridgeFailure, false, result);
			return;
		}

		HST_ForceSpawnQueueSlotSuccess success = BuildSuccess(work, handle, true, true);
		HST_ForceSpawnQueueCallbackResult callback = queue.CompleteSlotSuccess(state.m_aForceSpawnResults, manifest, success, nowSecond);
		if (!callback || !callback.m_bAccepted)
		{
			string callbackReason = ResolveCallbackFailure(callback, "member success callback rejected");
			if (RollbackPhysicalRegistration(physicalWar, activeGroup, handle))
				FailWork(state, queue, manifest, work, nowSecond, callbackReason, false, result);
			else
				FailWorkWithHandle(state, queue, manifest, work, handle, nowSecond, callbackReason, result);
			return;
		}

		result.m_bStateChanged = true;
		result.m_bRuntimeChanged = true;
		result.m_iSpawnedCount++;
		result.m_aEvidence.Insert(string.Format("member %1 registered as %2", work.m_sSlotId, entityId));
	}

	protected void ExecuteCleanupWork(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_PhysicalWarService physicalWar,
		HST_ForceSpawnQueueWorkItem work,
		int nowSecond,
		HST_ForceSpawnAdapterTickResult result)
	{
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(work.m_sResultId);
		HST_ActiveGroupState activeGroup = FindActiveGroupForBatch(state, batch);
		HST_ForceSpawnAdapterHandle handle = FindDurableHandle(work.m_sResultId, work.m_sProjectionId, work.m_sSlotId);
		if (!handle && !HasCleanupPhysicalEvidence(work))
		{
			HST_ForceSpawnQueueCallbackResult unmaterialized = queue.CompleteCleanup(
				state.m_aForceSpawnResults,
				work.m_sResultId,
				work.m_sProjectionId,
				work.m_sSlotId,
				work.m_iAttemptGeneration,
				nowSecond,
				true,
				"no runtime entity materialized");
			MergeCallback(unmaterialized, result);
			if (unmaterialized && unmaterialized.m_bAccepted)
			{
				result.m_iCleanedCount++;
				result.m_aEvidence.Insert("acknowledged unmaterialized cleanup slot " + work.m_sSlotId);
			}
			else
			{
				result.m_iFailedCount++;
			}
			return;
		}
		if (!activeGroup || !handle)
		{
			HST_ForceSpawnQueueCallbackResult missing = queue.CompleteCleanup(
				state.m_aForceSpawnResults,
				work.m_sResultId,
				work.m_sProjectionId,
				work.m_sSlotId,
				work.m_iAttemptGeneration,
				nowSecond,
				false,
				"exact runtime cleanup binding is missing");
			MergeCallback(missing, result);
			result.m_iFailedCount++;
			return;
		}
		if (handle.m_sEntityId != work.m_sEntityId || handle.m_sNativeGroupId != work.m_sNativeGroupId)
		{
			HST_ForceSpawnQueueCallbackResult conflict = queue.CompleteCleanup(
				state.m_aForceSpawnResults,
				work.m_sResultId,
				work.m_sProjectionId,
				work.m_sSlotId,
				work.m_iAttemptGeneration,
				nowSecond,
				false,
				"exact runtime cleanup identity conflicts with acquired work");
			MergeCallback(conflict, result);
			result.m_iFailedCount++;
			return;
		}

		string bridgeFailure;
		bool cleaned;
		if (handle.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
			cleaned = physicalWar.TryUnregisterForceSpawnGroupMember(activeGroup, handle.m_Entity, true, bridgeFailure);
		else if (handle.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP)
			cleaned = physicalWar.TryUnregisterForceSpawnGroupRoot(activeGroup, SCR_AIGroup.Cast(handle.m_Entity), true, bridgeFailure);
		else
			bridgeFailure = "adapter has no exact cleanup bridge for slot kind " + handle.m_sSlotKind;

		if (cleaned)
			RemoveHandle(handle);
		HST_ForceSpawnQueueCallbackResult callback = queue.CompleteCleanup(
			state.m_aForceSpawnResults,
			work.m_sResultId,
			work.m_sProjectionId,
			work.m_sSlotId,
			work.m_iAttemptGeneration,
			nowSecond,
			cleaned,
			bridgeFailure);
		MergeCallback(callback, result);
		if (!cleaned)
		{
			result.m_iFailedCount++;
			return;
		}

		result.m_bRuntimeChanged = true;
		result.m_iCleanedCount++;
		result.m_aEvidence.Insert("cleaned exact slot " + work.m_sSlotId);
	}

	protected void FailWork(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueWorkItem work,
		int nowSecond,
		string reason,
		bool retryable,
		HST_ForceSpawnAdapterTickResult result)
	{
		if (!manifest)
			manifest = state.FindForceManifest(work.m_sManifestId);
		HST_ForceSpawnQueueSlotFailure failure = new HST_ForceSpawnQueueSlotFailure();
		failure.m_sResultId = work.m_sResultId;
		failure.m_sProjectionId = work.m_sProjectionId;
		failure.m_sSlotId = work.m_sSlotId;
		failure.m_sFailureReason = reason;
		failure.m_iAttemptGeneration = work.m_iAttemptGeneration;
		failure.m_bRetryable = retryable;
		HST_ForceSpawnQueueCallbackResult callback = queue.FailSlot(state.m_aForceSpawnResults, manifest, failure, nowSecond);
		MergeCallback(callback, result);
		result.m_iFailedCount++;
		result.m_aEvidence.Insert("spawn failed " + work.m_sSlotId + ": " + reason);
	}

	protected void FailWorkWithHandle(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueWorkItem work,
		HST_ForceSpawnAdapterHandle handle,
		int nowSecond,
		string reason,
		HST_ForceSpawnAdapterTickResult result)
	{
		HST_ForceSpawnQueueSlotFailure failure = new HST_ForceSpawnQueueSlotFailure();
		failure.m_sResultId = work.m_sResultId;
		failure.m_sProjectionId = work.m_sProjectionId;
		failure.m_sSlotId = work.m_sSlotId;
		failure.m_sFailureReason = reason;
		failure.m_iAttemptGeneration = work.m_iAttemptGeneration;
		failure.m_bRetryable = false;
		if (handle)
		{
			failure.m_sEntityId = handle.m_sEntityId;
			failure.m_sSpawnedPrefab = handle.m_sPrefab;
			failure.m_sNativeGroupId = handle.m_sNativeGroupId;
		}
		HST_ForceSpawnQueueCallbackResult callback = queue.FailSlot(state.m_aForceSpawnResults, manifest, failure, nowSecond);
		MergeCallback(callback, result);
		result.m_iFailedCount++;
		result.m_aEvidence.Insert("spawn failed with retained cleanup ownership " + work.m_sSlotId + ": " + reason);
	}

	protected void DeferWork(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueWorkItem work,
		int nowSecond,
		string reason,
		HST_ForceSpawnAdapterTickResult result)
	{
		if (!manifest)
			manifest = state.FindForceManifest(work.m_sManifestId);
		HST_ForceSpawnQueueCallbackResult callback = queue.DeferSlot(
			state.m_aForceSpawnResults,
			manifest,
			work.m_sResultId,
			work.m_sProjectionId,
			work.m_sSlotId,
			work.m_iAttemptGeneration,
			nowSecond,
			nowSecond + HST_ForceSpawnQueueService.DEFAULT_DEFER_SECONDS,
			reason);
		MergeCallback(callback, result);
		result.m_iDeferredCount++;
		result.m_aEvidence.Insert("spawn deferred " + work.m_sSlotId + ": " + reason);
	}

	protected void ReconcileDeletedStagedHandles(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		int nowSecond,
		HST_ForceSpawnAdapterTickResult result)
	{
		array<string> failedResultIds = {};
		array<ref HST_ForceSpawnAdapterHandle> removableHandles = {};
		int handleCount = m_aHandles.Count();
		if (handleCount <= 0)
		{
			m_iHandleReconcileCursor = 0;
			return;
		}
		m_iHandleReconcileCursor = m_iHandleReconcileCursor % handleCount;
		int inspected;
		while (inspected < handleCount && inspected < MAX_HANDLE_RECONCILE_PER_TICK)
		{
			int index = (m_iHandleReconcileCursor + inspected) % handleCount;
			HST_ForceSpawnAdapterHandle handle = m_aHandles[index];
			inspected++;
			if (!handle)
			{
				removableHandles.Insert(handle);
				continue;
			}
			if (handle.m_Entity && !handle.m_Entity.IsDeleted())
				continue;
			if (handle.m_bHandedOff)
			{
				HST_ForceSpawnResultState handedOffBatch = state.FindForceSpawnResult(handle.m_sResultId);
				HST_ActiveGroupState handedOffGroup = FindActiveGroupForBatch(state, handedOffBatch);
				if (handedOffGroup && handedOffGroup.m_sRuntimeStatus != "eliminated"
					&& handedOffGroup.m_sRuntimeStatus != "folded"
					&& handedOffGroup.m_sRuntimeStatus != "exact_runtime_binding_missing_unresolved")
				{
					handedOffGroup.m_sRuntimeStatus = "exact_runtime_binding_missing_unresolved";
					handedOffGroup.m_sSpawnFailureReason = "handed-off exact runtime entity disappeared without confirmed casualty evidence";
					handedOffGroup.m_iLifecycleRevision++;
					result.m_bStateChanged = true;
					result.m_iFailedCount++;
					result.m_aEvidence.Insert("handed-off exact binding disappeared without casualty proof " + handle.m_sSlotId);
				}
				continue;
			}
			if (failedResultIds.Contains(handle.m_sResultId))
				continue;
			HST_ForceSpawnResultState batch = state.FindForceSpawnResult(handle.m_sResultId);
			if (!batch || batch.m_sProjectionId != handle.m_sProjectionId)
				continue;
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING
				|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
				|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
				|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
				continue;
			string reason = "registered runtime entity disappeared before exact projection handoff: " + handle.m_sSlotId;
			HST_ForceSpawnQueueCallbackResult failed = queue.FailProjectionFinal(
				state.m_aForceSpawnResults,
				handle.m_sResultId,
				handle.m_sProjectionId,
				nowSecond,
				reason);
			MergeCallback(failed, result);
			failedResultIds.Insert(handle.m_sResultId);
			if (failed && failed.m_bAccepted)
				result.m_aEvidence.Insert(reason);
			else
				result.m_iFailedCount++;
		}
		foreach (HST_ForceSpawnAdapterHandle removable : removableHandles)
		{
			int removeIndex = m_aHandles.Find(removable);
			if (removeIndex >= 0)
				m_aHandles.Remove(removeIndex);
		}
		if (m_aHandles.Count() <= 0)
			m_iHandleReconcileCursor = 0;
		else
			m_iHandleReconcileCursor = HST_DefaultCatalog.PositiveMod(m_iHandleReconcileCursor + Math.Max(1, inspected), m_aHandles.Count());
	}

	protected void AppendBoundedReconcileResultIds(HST_CampaignState state, array<string> resultIds)
	{
		if (!state || !resultIds)
			return;
		int batchCount = state.m_aForceSpawnResults.Count();
		if (batchCount <= 0)
		{
			m_iReconcileCursor = 0;
			return;
		}
		m_iReconcileCursor = HST_DefaultCatalog.PositiveMod(m_iReconcileCursor, batchCount);
		int inspected;
		while (inspected < batchCount && resultIds.Count() < MAX_RECONCILE_BATCHES_PER_TICK)
		{
			int index = HST_DefaultCatalog.PositiveMod(m_iReconcileCursor + inspected, batchCount);
			HST_ForceSpawnResultState batch = state.m_aForceSpawnResults[index];
			if (batch && !batch.m_sResultId.IsEmpty() && !resultIds.Contains(batch.m_sResultId))
				resultIds.Insert(batch.m_sResultId);
			inspected++;
		}
		m_iReconcileCursor = HST_DefaultCatalog.PositiveMod(m_iReconcileCursor + Math.Max(1, inspected), batchCount);
	}

	protected bool HasCleanupPhysicalEvidence(HST_ForceSpawnQueueWorkItem work)
	{
		return work && (!work.m_sEntityId.IsEmpty()
			|| !work.m_sNativeGroupId.IsEmpty()
			|| !work.m_sAssignedVehicleEntityId.IsEmpty()
			|| !work.m_sPrefab.IsEmpty());
	}

	protected void FinalizeTouchedBatches(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_PhysicalWarService physicalWar,
		int nowSecond,
		array<string> touchedResultIds,
		HST_ForceSpawnAdapterTickResult result)
	{
		foreach (string resultId : touchedResultIds)
		{
			HST_ForceSpawnResultState batch = state.FindForceSpawnResult(resultId);
			if (!batch)
				continue;
			HST_ActiveGroupState activeGroup = FindActiveGroupForBatch(state, batch);
			if (!activeGroup)
				continue;
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			{
				if (HasHandedOffHandle(resultId) || CountHandlesForResult(resultId) <= 0)
					continue;
				if (activeGroup.m_bSpawnedEntity && physicalWar.GetForceSpawnGroupRoot(activeGroup))
					MarkHandlesHandedOff(resultId);
				continue;
			}
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_READY_FOR_HANDOFF)
			{
				if (CountHandlesForResult(resultId) <= 0)
					continue;
				string handoffFailure;
				if (!physicalWar.FinalizeForceSpawnProjection(state, activeGroup, handoffFailure))
				{
					result.m_aEvidence.Insert("spawn handoff failed " + resultId + ": " + handoffFailure);
					result.m_iFailedCount++;
					result.m_iHandoffRefusedCount++;
					continue;
				}
				HST_ForceManifestState manifest = state.FindForceManifest(batch.m_sManifestId);
				HST_ForceSpawnQueueCallbackResult handoff = queue.CompleteProjectionHandoff(
					state.m_aForceSpawnResults,
					manifest,
					batch.m_sResultId,
					batch.m_sProjectionId,
					batch.m_iAttemptGeneration,
					nowSecond);
				if (!handoff || !handoff.m_bAccepted)
				{
					result.m_aEvidence.Insert("durable spawn handoff callback rejected " + resultId + ": " + ResolveCallbackFailure(handoff, "unknown handoff callback failure"));
					result.m_iFailedCount++;
					result.m_iHandoffRefusedCount++;
					continue;
				}
				MarkHandlesHandedOff(resultId);
				result.m_bStateChanged = result.m_bStateChanged || handoff.m_bStateChanged;
				result.m_bRuntimeChanged = true;
				result.m_iHandedOffCount++;
				result.m_aEvidence.Insert("exact projection handed off " + batch.m_sProjectionId);
				continue;
			}
			if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
				&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
				continue;
			if (CountHandlesForResult(resultId) > 0)
				continue;
			bool alreadyApplied = IsProjectionFailureApplied(activeGroup);
			if (alreadyApplied)
				continue;
			if (!physicalWar.MarkForceSpawnProjectionFailed(state, activeGroup, batch.m_sTerminalReason))
			{
				result.m_aEvidence.Insert("spawn projection failure handoff refused " + resultId);
				result.m_iFailedCount++;
				result.m_iHandoffRefusedCount++;
				continue;
			}
			result.m_bStateChanged = true;
			result.m_bRuntimeChanged = true;
			result.m_aEvidence.Insert("spawn projection failure applied " + resultId);
		}
	}

	protected bool IsProjectionFailureApplied(HST_ActiveGroupState activeGroup)
	{
		return activeGroup
			&& !activeGroup.m_bSpawnedEntity
			&& activeGroup.m_sRuntimeEntityId.IsEmpty()
			&& activeGroup.m_sRuntimeStatus == "spawn_failed"
			&& activeGroup.m_iSpawnedAgentCount == 0
			&& activeGroup.m_iLastSeenAliveCount == 0
			&& activeGroup.m_iSurvivorInfantryCount == 0
			&& activeGroup.m_iSurvivorVehicleCount == 0;
	}

	HST_ForceSpawnAdapterRetireResult RetireProjectionRuntime(
		HST_CampaignState state,
		HST_PhysicalWarService physicalWar,
		string projectionId)
	{
		HST_ForceSpawnAdapterRetireResult result = new HST_ForceSpawnAdapterRetireResult();
		if (!state || !physicalWar || projectionId.IsEmpty())
		{
			result.m_sFailureReason = "retirement state, bridge, or projection identity missing";
			return result;
		}

		HST_ActiveGroupState activeGroup = FindActiveGroupByProjectionId(state, projectionId);
		if (!activeGroup)
		{
			result.m_sFailureReason = "retirement projection has no exact active-group owner";
			return result;
		}
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(activeGroup.m_sSpawnResultId);
		string bindingConflict = FindProjectionBindingConflict(batch);
		if (!bindingConflict.IsEmpty())
		{
			result.m_sFailureReason = "retirement refused for ambiguous adapter authority: " + bindingConflict;
			return result;
		}
		string ownershipFailure;
		if (!physicalWar.PrepareForceSpawnProjectionCleanup(state, activeGroup, ownershipFailure))
		{
			result.m_sFailureReason = ownershipFailure;
			return result;
		}

		string failure;
		for (int memberIndex = m_aHandles.Count() - 1; memberIndex >= 0; memberIndex--)
		{
			HST_ForceSpawnAdapterHandle handle = m_aHandles[memberIndex];
			if (!handle || handle.m_sProjectionId != projectionId || handle.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
				continue;
			if (!physicalWar.TryUnregisterForceSpawnGroupMember(activeGroup, handle.m_Entity, true, failure))
			{
				result.m_sFailureReason = failure;
				return result;
			}
			m_aHandles.Remove(memberIndex);
			result.m_iMemberCount++;
			result.m_bRuntimeChanged = true;
		}

		for (int rootIndex = m_aHandles.Count() - 1; rootIndex >= 0; rootIndex--)
		{
			HST_ForceSpawnAdapterHandle rootHandle = m_aHandles[rootIndex];
			if (!rootHandle || rootHandle.m_sProjectionId != projectionId || rootHandle.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_GROUP)
				continue;
			if (!physicalWar.TryUnregisterForceSpawnGroupRoot(activeGroup, SCR_AIGroup.Cast(rootHandle.m_Entity), true, failure))
			{
				result.m_sFailureReason = failure;
				return result;
			}
			m_aHandles.Remove(rootIndex);
			result.m_iRootCount++;
			result.m_bRuntimeChanged = true;
		}

		SCR_AIGroup remainingRoot = physicalWar.GetForceSpawnGroupRoot(activeGroup);
		int remainingRuntimeMembers = physicalWar.CountForceSpawnRuntimeMembers(activeGroup);
		if (remainingRoot && remainingRuntimeMembers <= 0)
		{
			if (!physicalWar.TryUnregisterForceSpawnGroupRoot(activeGroup, remainingRoot, true, failure))
			{
				result.m_sFailureReason = failure;
				return result;
			}
			result.m_iRootCount++;
			result.m_bRuntimeChanged = true;
			remainingRoot = physicalWar.GetForceSpawnGroupRoot(activeGroup);
		}

		int remainingBindings = CountHandlesForProjection(projectionId);
		remainingRuntimeMembers = physicalWar.CountForceSpawnRuntimeMembers(activeGroup);
		result.m_bSuccess = remainingBindings == 0 && !remainingRoot && remainingRuntimeMembers == 0;
		if (!result.m_bSuccess)
		{
			result.m_sFailureReason = string.Format(
				"retirement left exact runtime authority: bindings %1 | root %2 | members %3",
				remainingBindings,
				remainingRoot != null,
				remainingRuntimeMembers);
			return result;
		}

		physicalWar.ReleaseForceSpawnRuntimeOwnership(activeGroup);
		return result;
	}

	protected string ResolveExecutionContext(
		HST_CampaignState state,
		HST_ForceSpawnQueueWorkItem work,
		out HST_ActiveGroupState activeGroup,
		out HST_ForceManifestState manifest)
	{
		activeGroup = null;
		manifest = null;
		if (!state || !work)
			return "spawn execution context missing";
		manifest = state.FindForceManifest(work.m_sManifestId);
		if (!manifest)
			return "spawn execution manifest missing";
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(work.m_sResultId);
		if (!batch)
			return "durable spawn batch missing";
		if (batch.m_sResultId != work.m_sResultId || batch.m_sRequestId != work.m_sRequestId)
			return "durable spawn result or request identity conflict";
		if (batch.m_sManifestId != work.m_sManifestId || batch.m_sManifestHash != work.m_sManifestHash)
			return "durable spawn manifest identity conflict";
		if (batch.m_sOperationId != work.m_sOperationId || batch.m_sForceId != work.m_sForceId
			|| batch.m_sProjectionId != work.m_sProjectionId)
			return "durable spawn operation, force, or projection identity conflict";
		if (manifest.m_sManifestId != batch.m_sManifestId || manifest.m_sManifestHash != batch.m_sManifestHash
			|| manifest.m_sOperationId != batch.m_sOperationId)
			return "frozen manifest does not own the durable spawn batch";
		activeGroup = FindActiveGroupForBatch(state, batch);
		if (!activeGroup)
			return "spawn execution force has no active-group projection";
		if (activeGroup.m_sFactionKey != manifest.m_sFactionKey)
			return "active-group faction conflicts with the frozen manifest";
		if (manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
			return "frozen manifest does not contain exactly one group projection";
		HST_ForceManifestGroupState manifestGroup = manifest.m_aGroups[0];
		string expectedGroupPrefab = manifestGroup.m_sPrefab;
		if (expectedGroupPrefab.IsEmpty())
			expectedGroupPrefab = manifest.m_sGroupPrefab;
		if (expectedGroupPrefab.IsEmpty() || activeGroup.m_sPrefab != expectedGroupPrefab)
			return "active-group prefab conflicts with the frozen group projection";
		if (activeGroup.m_iInfantryCount != manifest.m_iAcceptedMemberCount
			|| manifestGroup.m_iExpectedMemberCount != manifest.m_iAcceptedMemberCount)
			return "active-group infantry count conflicts with the frozen manifest";
		if (activeGroup.m_iVehicleCount != manifest.m_iAcceptedVehicleCount)
			return "active-group vehicle count conflicts with the frozen manifest";
		return "";
	}

	protected string ValidateSupportedManifest(HST_ForceManifestState manifest)
	{
		if (!manifest)
			return "physical adapter manifest missing";
		if (manifest.m_aGroups.Count() != 1)
			return UNSUPPORTED_MANIFEST_REASON + ": group roots must equal one";
		if (manifest.m_aVehicles.Count() > 0 || manifest.m_aAssets.Count() > 0)
			return UNSUPPORTED_MANIFEST_REASON + ": vehicle or asset slots present";
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (member && !member.m_sAssignedVehicleSlotId.IsEmpty())
				return UNSUPPORTED_MANIFEST_REASON + ": assigned vehicle member present";
		}
		return "";
	}

	protected HST_ForceSpawnAdapterHandle BuildHandle(
		HST_ForceSpawnQueueWorkItem work,
		IEntity entity,
		string actualPrefab,
		string entityId,
		string nativeGroupId)
	{
		HST_ForceSpawnAdapterHandle handle = new HST_ForceSpawnAdapterHandle();
		handle.m_sResultId = work.m_sResultId;
		handle.m_sProjectionId = work.m_sProjectionId;
		handle.m_sForceId = work.m_sForceId;
		handle.m_sSlotId = work.m_sSlotId;
		handle.m_sSlotKind = work.m_sSlotKind;
		handle.m_sPrefab = actualPrefab;
		handle.m_sEntityId = entityId;
		handle.m_sNativeGroupId = nativeGroupId;
		handle.m_iAttemptGeneration = work.m_iAttemptGeneration;
		handle.m_Entity = entity;
		return handle;
	}

	protected HST_ForceSpawnQueueSlotSuccess BuildSuccess(
		HST_ForceSpawnQueueWorkItem work,
		HST_ForceSpawnAdapterHandle handle,
		bool groupVerified,
		bool gameMasterVerified)
	{
		HST_ForceSpawnQueueSlotSuccess success = new HST_ForceSpawnQueueSlotSuccess();
		success.m_sResultId = work.m_sResultId;
		success.m_sProjectionId = work.m_sProjectionId;
		success.m_sSlotId = work.m_sSlotId;
		success.m_sEntityId = handle.m_sEntityId;
		success.m_sSpawnedPrefab = handle.m_sPrefab;
		success.m_sNativeGroupId = handle.m_sNativeGroupId;
		success.m_iAttemptGeneration = work.m_iAttemptGeneration;
		success.m_bAliveVerified = handle.m_Entity && !handle.m_Entity.IsDeleted();
		success.m_bFactionVerified = true;
		success.m_bGroupVerified = groupVerified;
		success.m_bProjectionVerified = HandleMatchesWork(handle, work);
		success.m_bSeatVerified = false;
		success.m_bGameMasterVerified = gameMasterVerified;
		return success;
	}

	protected bool RegisterHandle(HST_ForceSpawnAdapterHandle handle)
	{
		if (!handle || !handle.m_Entity || handle.m_sResultId.IsEmpty() || handle.m_sSlotId.IsEmpty() || handle.m_sEntityId.IsEmpty())
			return false;
		foreach (HST_ForceSpawnAdapterHandle existing : m_aHandles)
		{
			if (!existing)
				continue;
			if (existing.m_sEntityId == handle.m_sEntityId || existing.m_Entity == handle.m_Entity)
				return existing.m_sResultId == handle.m_sResultId && existing.m_sSlotId == handle.m_sSlotId;
			if (existing.m_sResultId == handle.m_sResultId && existing.m_sSlotId == handle.m_sSlotId)
				return false;
		}
		m_aHandles.Insert(handle);
		return true;
	}

	protected HST_ForceSpawnAdapterHandle FindDurableHandle(string resultId, string projectionId, string slotId)
	{
		HST_ForceSpawnAdapterHandle found;
		foreach (HST_ForceSpawnAdapterHandle handle : m_aHandles)
		{
			if (!handle || handle.m_sResultId != resultId || handle.m_sProjectionId != projectionId || handle.m_sSlotId != slotId)
				continue;
			if (found)
				return null;
			found = handle;
		}
		return found;
	}

	protected string FindProjectionBindingConflict(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return "spawn batch is unavailable";
		foreach (HST_ForceSpawnAdapterHandle handle : m_aHandles)
		{
			if (!handle)
				continue;
			bool resultMatches = handle.m_sResultId == batch.m_sResultId;
			bool projectionMatches = handle.m_sProjectionId == batch.m_sProjectionId;
			if (!resultMatches && !projectionMatches)
				continue;
			if (resultMatches != projectionMatches)
				return "cross-key adapter claimant " + handle.m_sSlotId;
			int slotMatches;
			foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
			{
				if (slot && slot.m_sSlotId == handle.m_sSlotId
					&& slot.m_sSlotKind == handle.m_sSlotKind)
					slotMatches++;
			}
			if (slotMatches != 1)
				return "adapter claimant has no unique durable slot " + handle.m_sSlotId;
		}
		return "";
	}

	protected bool HandleMatchesWork(HST_ForceSpawnAdapterHandle handle, HST_ForceSpawnQueueWorkItem work)
	{
		return handle && work
			&& handle.m_sResultId == work.m_sResultId
			&& handle.m_sProjectionId == work.m_sProjectionId
			&& handle.m_sForceId == work.m_sForceId
			&& handle.m_sSlotId == work.m_sSlotId
			&& handle.m_iAttemptGeneration == work.m_iAttemptGeneration;
	}

	protected bool RollbackPhysicalRegistration(
		HST_PhysicalWarService physicalWar,
		HST_ActiveGroupState activeGroup,
		HST_ForceSpawnAdapterHandle handle)
	{
		if (!handle)
			return true;
		string failure;
		bool cleaned;
		if (physicalWar && activeGroup && handle.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
			cleaned = physicalWar.TryUnregisterForceSpawnGroupMember(activeGroup, handle.m_Entity, true, failure);
		else if (physicalWar && activeGroup && handle.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP)
			cleaned = physicalWar.TryUnregisterForceSpawnGroupRoot(activeGroup, SCR_AIGroup.Cast(handle.m_Entity), true, failure);
		else
		{
			DeleteUnregisteredEntity(handle.m_Entity);
			cleaned = true;
		}
		if (cleaned)
			RemoveHandle(handle);
		return cleaned;
	}

	protected void RemoveHandle(HST_ForceSpawnAdapterHandle handle)
	{
		if (!handle)
			return;
		int index = m_aHandles.Find(handle);
		if (index >= 0)
			m_aHandles.Remove(index);
	}

	protected int RemoveProjectionBindings(string projectionId)
	{
		int removed;
		if (projectionId.IsEmpty())
			return removed;
		for (int index = m_aHandles.Count() - 1; index >= 0; index--)
		{
			HST_ForceSpawnAdapterHandle handle = m_aHandles[index];
			if (!handle || handle.m_sProjectionId != projectionId)
				continue;
			m_aHandles.Remove(index);
			removed++;
		}
		return removed;
	}

	protected HST_ActiveGroupState FindActiveGroupForBatch(HST_CampaignState state, HST_ForceSpawnResultState batch)
	{
		if (!state || !batch)
			return null;
		HST_ActiveGroupState found;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_sForceId != batch.m_sForceId || activeGroup.m_sProjectionId != batch.m_sProjectionId)
				continue;
			if (activeGroup.m_sSpawnResultId != batch.m_sResultId || activeGroup.m_sManifestId != batch.m_sManifestId)
				continue;
			if (activeGroup.m_sOperationId != batch.m_sOperationId)
				continue;
			if (found)
				return null;
			found = activeGroup;
		}
		return found;
	}

	protected HST_ActiveGroupState FindActiveGroupByProjectionId(HST_CampaignState state, string projectionId)
	{
		if (!state || projectionId.IsEmpty())
			return null;
		HST_ActiveGroupState found;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_sProjectionId != projectionId)
				continue;
			if (found)
				return null;
			found = activeGroup;
		}
		return found;
	}

	protected bool TryResolveGroupSpawnPosition(HST_ActiveGroupState activeGroup, out vector position)
	{
		vector preferred;
		if (activeGroup)
			preferred = activeGroup.m_vPosition;
		return HST_WorldPositionService.TryResolveSafeGroundPosition(preferred, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, position, true, 2.0);
	}

	protected bool TryResolveMemberSpawnPosition(HST_ActiveGroupState activeGroup, SCR_AIGroup root, int ordinal, out vector position)
	{
		vector preferred;
		if (root)
			preferred = root.GetOrigin();
		else if (activeGroup)
			preferred = activeGroup.m_vPosition;
		int normalizedOrdinal = Math.Max(0, ordinal);
		float distance = 1.5 + (normalizedOrdinal / 8) * 1.25;
		int slot = normalizedOrdinal % 8;
		if (slot == 0)
			preferred[0] = preferred[0] + distance;
		else if (slot == 1)
			preferred[2] = preferred[2] + distance;
		else if (slot == 2)
			preferred[0] = preferred[0] - distance;
		else if (slot == 3)
			preferred[2] = preferred[2] - distance;
		else if (slot == 4)
		{
			preferred[0] = preferred[0] + distance;
			preferred[2] = preferred[2] + distance;
		}
		else if (slot == 5)
		{
			preferred[0] = preferred[0] - distance;
			preferred[2] = preferred[2] + distance;
		}
		else if (slot == 6)
		{
			preferred[0] = preferred[0] + distance;
			preferred[2] = preferred[2] - distance;
		}
		else
		{
			preferred[0] = preferred[0] - distance;
			preferred[2] = preferred[2] - distance;
		}
		return HST_WorldPositionService.TryResolveSafeGroundPosition(preferred, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, position, true, 1.0);
	}

	protected string ResolveEntityPrefabName(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return "";
		return entity.GetPrefabData().GetPrefabName();
	}

	protected string ResolveReplicatedEntityId(IEntity entity)
	{
		if (!entity)
			return "";
		BaseRplComponent rpl = BaseRplComponent.Cast(entity.FindComponent(BaseRplComponent));
		if (!rpl || !rpl.Id().IsValid())
			return "";
		return string.Format("rpl_%1", rpl.Id());
	}

	protected void DeleteUnregisteredEntity(IEntity entity)
	{
		if (entity && !entity.IsDeleted())
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}

	protected string AppendModeToken(string mode, string token)
	{
		if (token.IsEmpty() || mode.Contains(token))
			return mode;
		if (mode.IsEmpty())
			return token;
		return mode + "_" + token;
	}

	protected void MergeCallback(HST_ForceSpawnQueueCallbackResult callback, HST_ForceSpawnAdapterTickResult result)
	{
		if (!callback || !result)
			return;
		result.m_bStateChanged = callback.m_bStateChanged || result.m_bStateChanged;
		if (!callback.m_sFailureReason.IsEmpty())
			result.m_aEvidence.Insert(callback.m_sFailureReason);
	}

	protected string ResolveCallbackFailure(
		HST_ForceSpawnQueueCallbackResult callback,
		string fallback)
	{
		if (callback && !callback.m_sFailureReason.IsEmpty())
			return callback.m_sFailureReason;
		return fallback;
	}

	protected void TouchResultId(array<string> resultIds, string resultId)
	{
		if (resultIds && !resultId.IsEmpty() && !resultIds.Contains(resultId))
			resultIds.Insert(resultId);
	}

	protected int CountHandlesForResult(string resultId)
	{
		int count;
		foreach (HST_ForceSpawnAdapterHandle handle : m_aHandles)
		{
			if (handle && handle.m_sResultId == resultId)
				count++;
		}
		return count;
	}

	int CountHandlesForProjection(string projectionId)
	{
		int count;
		foreach (HST_ForceSpawnAdapterHandle handle : m_aHandles)
		{
			if (handle && handle.m_sProjectionId == projectionId)
				count++;
		}
		return count;
	}

	int CountHandlesForResultId(string resultId)
	{
		if (resultId.IsEmpty())
			return 0;
		return CountHandlesForResult(resultId);
	}

	int PruneDeletedProjectionBindings(string projectionId)
	{
		if (projectionId.IsEmpty())
			return 0;
		int removed;
		for (int index = m_aHandles.Count() - 1; index >= 0; index--)
		{
			HST_ForceSpawnAdapterHandle handle = m_aHandles[index];
			if (!handle || handle.m_sProjectionId != projectionId)
				continue;
			if (handle.m_Entity && !handle.m_Entity.IsDeleted())
				continue;
			m_aHandles.Remove(index);
			removed++;
		}
		return removed;
	}

	int CountBindings()
	{
		return m_aHandles.Count();
	}

	protected bool HasHandedOffHandle(string resultId)
	{
		foreach (HST_ForceSpawnAdapterHandle handle : m_aHandles)
		{
			if (handle && handle.m_sResultId == resultId && handle.m_bHandedOff)
				return true;
		}
		return false;
	}

	protected void MarkHandlesHandedOff(string resultId)
	{
		foreach (HST_ForceSpawnAdapterHandle handle : m_aHandles)
		{
			if (handle && handle.m_sResultId == resultId)
				handle.m_bHandedOff = true;
		}
	}

	protected void UpdateTotals(HST_ForceSpawnAdapterTickResult result)
	{
		m_iSpawnedTotal += result.m_iSpawnedCount;
		m_iCleanedTotal += result.m_iCleanedCount;
		m_iFailedTotal += result.m_iFailedCount;
		m_iDeferredTotal += result.m_iDeferredCount;
		m_iHandoffRefusedTotal += result.m_iHandoffRefusedCount;
		if ((result.m_iFailedCount > 0 || result.m_iHandoffRefusedCount > 0) && result.m_aEvidence.Count() > 0)
			m_sLastFailureEvidence = result.m_aEvidence[result.m_aEvidence.Count() - 1];
	}

	protected string BuildTickSummary(HST_ForceSpawnAdapterTickResult result)
	{
		string summary = string.Format(
			"force spawn adapter tick | batches %1 | actions %2 | spawned %3 | cleaned %4 | deferred %5 | failed %6 | handed off %7 | refused %8",
			result.m_iAcquiredBatchCount,
			result.m_iAcquiredActionCount,
			result.m_iSpawnedCount,
			result.m_iCleanedCount,
			result.m_iDeferredCount,
			result.m_iFailedCount,
			result.m_iHandedOffCount,
			result.m_iHandoffRefusedCount);
		return summary + string.Format(
			" | casualties %1 | eliminated %2 | bindings %3",
			result.m_iCasualtyCount,
			result.m_iEliminatedCount,
			m_aHandles.Count());
	}

	string BuildReport()
	{
		string report = string.Format(
			"force spawn adapter | ticks %1 | bindings %2 | spawned %3 | cleaned %4 | deferred %5 | failed %6 | handoff refused %7",
			m_iTickCount,
			m_aHandles.Count(),
			m_iSpawnedTotal,
			m_iCleanedTotal,
			m_iDeferredTotal,
			m_iFailedTotal,
			m_iHandoffRefusedTotal);
		if (!m_sLastFailureEvidence.IsEmpty())
			report = report + "\nlast failure | " + m_sLastFailureEvidence;
		if (!m_sLastSummary.IsEmpty())
			return report + "\n" + m_sLastSummary;
		return report;
	}
}
