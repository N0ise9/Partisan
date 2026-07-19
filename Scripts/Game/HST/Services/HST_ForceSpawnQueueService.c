class HST_ForceSpawnQueueRequest
{
	string m_sResultId;
	string m_sRequestId;
	string m_sProjectionId;
	string m_sForceId;
	int m_iPriority;
	int m_iMaxRetries = 3;
	int m_iDeadlineSecond;
	bool m_bExternalAssetAuthority;
}

class HST_ForceSpawnQueueEnqueueResult
{
	bool m_bSuccess;
	bool m_bAlreadyApplied;
	bool m_bStateChanged;
	string m_sFailureReason;
	ref HST_ForceSpawnResultState m_Batch;
}

class HST_ForceSpawnQueueWorkItem
{
	string m_sAction;
	string m_sResultId;
	string m_sRequestId;
	string m_sManifestId;
	string m_sManifestHash;
	string m_sOperationId;
	string m_sForceId;
	string m_sProjectionId;
	string m_sSlotId;
	string m_sSlotKind;
	string m_sPrefab;
	string m_sRole;
	string m_sGroupElementId;
	string m_sAssignedVehicleSlotId;
	string m_sAssignedVehicleEntityId;
	string m_sSeatRole;
	string m_sEntityId;
	string m_sNativeGroupId;
	int m_iAttemptGeneration;
	int m_iAttemptCount;
	int m_iOrdinal = -1;
	int m_iSeatIndex = -1;
	int m_iRequiredCrew;
	bool m_bRequired;
}

class HST_ForceSpawnQueueTickResult
{
	bool m_bStateChanged;
	int m_iBatchesSelected;
	int m_iSlotsSelected;
	int m_iDeferredBatchCount;
	string m_sSummary;
	ref array<ref HST_ForceSpawnQueueWorkItem> m_aWorkItems = {};
}

class HST_ForceSpawnQueueCallbackResult
{
	bool m_bAccepted;
	bool m_bAlreadyApplied;
	bool m_bStale;
	bool m_bCleanupRequired;
	bool m_bStateChanged;
	string m_sFailureReason;
	ref HST_ForceSpawnResultState m_Batch;
}

class HST_ForceSpawnQueueSlotSuccess
{
	string m_sResultId;
	string m_sProjectionId;
	string m_sSlotId;
	string m_sEntityId;
	string m_sSpawnedPrefab;
	string m_sAssignedVehicleEntityId;
	string m_sNativeGroupId;
	int m_iAttemptGeneration;
	bool m_bAliveVerified;
	bool m_bFactionVerified;
	bool m_bGroupVerified;
	bool m_bGameMasterVerified;
	bool m_bProjectionVerified;
	bool m_bSeatVerified;
}

class HST_ForceSpawnQueueSlotFailure
{
	string m_sResultId;
	string m_sProjectionId;
	string m_sSlotId;
	string m_sEntityId;
	string m_sSpawnedPrefab;
	string m_sAssignedVehicleEntityId;
	string m_sNativeGroupId;
	string m_sFailureReason;
	int m_iAttemptGeneration;
	int m_iRetryAtSecond;
	bool m_bRetryable;
}

class HST_ForceSpawnQueueRetentionPins
{
	ref array<string> m_aResultIds = {};
	ref array<string> m_aRequestIds = {};
	ref array<string> m_aManifestIds = {};
	ref array<string> m_aOperationIds = {};
	ref array<string> m_aForceIds = {};
	ref array<string> m_aProjectionIds = {};

	bool Contains(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return true;
		if (!batch.m_sResultId.IsEmpty() && m_aResultIds.Contains(batch.m_sResultId))
			return true;
		if (!batch.m_sRequestId.IsEmpty() && m_aRequestIds.Contains(batch.m_sRequestId))
			return true;
		if (!batch.m_sManifestId.IsEmpty() && m_aManifestIds.Contains(batch.m_sManifestId))
			return true;
		if (!batch.m_sOperationId.IsEmpty() && m_aOperationIds.Contains(batch.m_sOperationId))
			return true;
		if (!batch.m_sForceId.IsEmpty() && m_aForceIds.Contains(batch.m_sForceId))
			return true;
		return !batch.m_sProjectionId.IsEmpty() && m_aProjectionIds.Contains(batch.m_sProjectionId);
	}
}

class HST_ForceSpawnQueueMaintenanceResult
{
	bool m_bStateChanged;
	bool m_bPinsRequired;
	int m_iInspectedBatchCount;
	int m_iReconciledBatchCount;
	int m_iRemovedTerminalCount;
	int m_iPinnedTerminalCount;
	int m_iYoungTerminalCount;
	int m_iEligibleTerminalCount;
	int m_iRemainingTerminalCount;
	ref array<string> m_aEvidence = {};

	string BuildSummary()
	{
		return string.Format(
			"spawn queue maintenance | inspected %1 | reconciled %2 | removed %3 | terminal %4 | pinned %5 | young %6 | eligible %7 | pins required %8",
			m_iInspectedBatchCount,
			m_iReconciledBatchCount,
			m_iRemovedTerminalCount,
			m_iRemainingTerminalCount,
			m_iPinnedTerminalCount,
			m_iYoungTerminalCount,
			m_iEligibleTerminalCount,
			m_bPinsRequired
		);
	}
}

class HST_ForceSpawnQueueManifestSlot
{
	string m_sSlotId;
	string m_sSlotKind;
	string m_sPrefab;
	string m_sRole;
	string m_sGroupElementId;
	string m_sAssignedVehicleSlotId;
	string m_sSeatRole;
	int m_iOrdinal;
	int m_iSeatIndex = -1;
	int m_iRequiredCrew;
	bool m_bRequired;
}

class HST_ForceSpawnQueueManifestValidation
{
	bool m_bValid;
	string m_sFailureReason;
	ref array<ref HST_ForceSpawnQueueManifestSlot> m_aSlots = {};
}

class HST_ForceSpawnQueueService
{
	static const int MAX_NONTERMINAL_BATCHES = 64;
	static const int MAX_TOTAL_SLOT_ROWS = 512;
	static const int MAX_SLOTS_PER_REQUEST = 64;
	static const int MAX_TERMINAL_ROWS = 128;
	static const int MAX_BATCHES_PER_TICK = 2;
	static const int MAX_SLOTS_PER_TICK = 8;
	static const int TERMINAL_RETENTION_SECONDS = 600;
	static const int DEFAULT_DEFER_SECONDS = 2;
	static const int MAX_BACKOFF_SECONDS = 60;

	static const string ACTION_SPAWN = "spawn";
	static const string ACTION_CLEANUP = "cleanup";
	static const string SLOT_KIND_GROUP = "group";
	static const string SLOT_KIND_VEHICLE = "vehicle";
	static const string SLOT_KIND_MEMBER = "member";
	static const string SLOT_KIND_ASSET = "asset";

	protected static const string CLEANUP_DISPOSITION_RETRY = "retryable|";
	protected static const string CLEANUP_DISPOSITION_DEFER = "deferred|";
	protected static const string CLEANUP_DISPOSITION_FINAL = "final|";
	protected static const string CLEANUP_DISPOSITION_CANCEL = "cancelled|";
	protected ref HST_ForcePlanningIntegrityService m_Integrity = new HST_ForcePlanningIntegrityService();

	HST_ForceSpawnQueueEnqueueResult Enqueue(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueRequest request,
		int nowSecond)
	{
		HST_ForceSpawnQueueManifestValidation validation;
		HST_ForceSpawnQueueEnqueueResult result = ValidateEnqueueAdmission(
			batches,
			manifest,
			request,
			nowSecond,
			validation);
		if (!result || !result.m_bSuccess || result.m_bAlreadyApplied)
			return result;

		HST_ForceSpawnResultState batch = BuildBatch(manifest, request, validation.m_aSlots, nowSecond);
		batches.Insert(batch);
		result.m_bStateChanged = true;
		result.m_Batch = batch;
		return result;
	}

	HST_ForceSpawnQueueEnqueueResult CanEnqueue(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueRequest request,
		int nowSecond)
	{
		HST_ForceSpawnQueueManifestValidation validation;
		return ValidateEnqueueAdmission(batches, manifest, request, nowSecond, validation);
	}

	protected HST_ForceSpawnQueueEnqueueResult ValidateEnqueueAdmission(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueRequest request,
		int nowSecond,
		out HST_ForceSpawnQueueManifestValidation validation)
	{
		validation = null;
		HST_ForceSpawnQueueEnqueueResult result = new HST_ForceSpawnQueueEnqueueResult();
		string requestFailure = ValidateEnqueueRequest(batches, manifest, request, nowSecond);
		if (!requestFailure.IsEmpty())
		{
			result.m_sFailureReason = requestFailure;
			return result;
		}
		if (CountByRequest(batches, request.m_sRequestId) > 1
			|| CountByProjection(batches, request.m_sProjectionId) > 1
			|| CountByResult(batches, request.m_sResultId) > 1)
		{
			result.m_sFailureReason = "spawn queue contains duplicate durable identity keys";
			return result;
		}

		HST_ForceSpawnResultState requestMatch = FindByRequest(batches, request.m_sRequestId);
		HST_ForceSpawnResultState projectionMatch = FindByProjection(batches, request.m_sProjectionId);
		HST_ForceSpawnResultState resultMatch = FindByResult(batches, request.m_sResultId);
		HST_ForceSpawnResultState existing = ResolveExistingMatch(requestMatch, projectionMatch, resultMatch);
		if (HasConflictingMatches(requestMatch, projectionMatch, resultMatch))
		{
			result.m_sFailureReason = "request, projection, or result id resolves to different spawn batches";
			return result;
		}
		if (existing)
			return ResolveExistingEnqueue(existing, manifest, request);
		if (request.m_iDeadlineSecond <= nowSecond)
		{
			result.m_sFailureReason = "spawn deadline must be in the future";
			return result;
		}

		validation = ValidateManifest(manifest);
		if (!validation.m_bValid)
		{
			result.m_sFailureReason = validation.m_sFailureReason;
			return result;
		}
		int executableSlotCount = CountExecutableManifestSlots(validation.m_aSlots, request.m_bExternalAssetAuthority);
		string capacityFailure = ValidateAdmissionCapacity(batches, executableSlotCount);
		if (!capacityFailure.IsEmpty())
		{
			result.m_sFailureReason = capacityFailure;
			return result;
		}

		result.m_bSuccess = true;
		return result;
	}

	protected string ValidateEnqueueRequest(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueRequest request,
		int nowSecond)
	{
		if (!batches)
			return "spawn result collection missing";
		if (!manifest)
			return "force manifest missing";
		if (!request)
			return "spawn queue request missing";
		if (request.m_sResultId.IsEmpty())
			return "caller-supplied durable result id missing";
		if (request.m_sRequestId.IsEmpty())
			return "durable request id missing";
		if (request.m_sProjectionId.IsEmpty())
			return "projection id missing";
		if (request.m_sForceId.IsEmpty())
			return "force id missing";
		if (request.m_iMaxRetries < 0)
			return "max retries cannot be negative";
		if (request.m_bExternalAssetAuthority)
		{
			string externalAssetFailure = HST_RescuePOWExternalAssetPolicy.ValidateManifestShape(manifest);
			if (!externalAssetFailure.IsEmpty())
				return "external asset authority is reserved for the exact rescue contract: " + externalAssetFailure;
		}
		return "";
	}

	protected HST_ForceSpawnQueueEnqueueResult ResolveExistingEnqueue(
		HST_ForceSpawnResultState existing,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueRequest request)
	{
		HST_ForceSpawnQueueEnqueueResult result = new HST_ForceSpawnQueueEnqueueResult();
		result.m_Batch = existing;
		if (!BatchIdentityMatches(existing, manifest, request))
		{
			result.m_sFailureReason = "spawn queue idempotency key conflicts with existing payload";
			return result;
		}
		if (request.m_bExternalAssetAuthority)
		{
			string externalAssetFailure = HST_RescuePOWExternalAssetPolicy.ValidateManifestShape(manifest);
			if (!externalAssetFailure.IsEmpty())
			{
				result.m_sFailureReason = "existing external-asset spawn batch conflicts with exact rescue policy: " + externalAssetFailure;
				return result;
			}
		}
		HST_ForceSpawnQueueManifestValidation validation = ValidateManifest(manifest);
		if (!validation.m_bValid || !BatchSlotsMatchManifest(existing, validation.m_aSlots))
		{
			result.m_sFailureReason = "existing spawn batch conflicts with exact manifest slots";
			return result;
		}
		result.m_bSuccess = true;
		result.m_bAlreadyApplied = true;
		return result;
	}

	protected bool BatchIdentityMatches(
		HST_ForceSpawnResultState batch,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueRequest request)
	{
		if (!batch || !manifest || !request)
			return false;
		if (batch.m_sResultId != request.m_sResultId || batch.m_sRequestId != request.m_sRequestId)
			return false;
		if (batch.m_sProjectionId != request.m_sProjectionId || batch.m_sForceId != request.m_sForceId)
			return false;
		if (batch.m_sManifestId != manifest.m_sManifestId || batch.m_sManifestHash != manifest.m_sManifestHash)
			return false;
		if (batch.m_sOperationId != manifest.m_sOperationId || batch.m_iPriority != request.m_iPriority)
			return false;
		return batch.m_iMaxRetries == request.m_iMaxRetries
			&& batch.m_iDeadlineSecond == request.m_iDeadlineSecond
			&& batch.m_bExternalAssetAuthority == request.m_bExternalAssetAuthority;
	}

	protected HST_ForceSpawnResultState ResolveExistingMatch(
		HST_ForceSpawnResultState requestMatch,
		HST_ForceSpawnResultState projectionMatch,
		HST_ForceSpawnResultState resultMatch)
	{
		if (requestMatch)
			return requestMatch;
		if (projectionMatch)
			return projectionMatch;
		return resultMatch;
	}

	protected bool HasConflictingMatches(
		HST_ForceSpawnResultState requestMatch,
		HST_ForceSpawnResultState projectionMatch,
		HST_ForceSpawnResultState resultMatch)
	{
		if (requestMatch && projectionMatch && requestMatch != projectionMatch)
			return true;
		if (requestMatch && resultMatch && requestMatch != resultMatch)
			return true;
		return projectionMatch && resultMatch && projectionMatch != resultMatch;
	}

	protected string ValidateAdmissionCapacity(array<ref HST_ForceSpawnResultState> batches, int requestedSlotCount)
	{
		if (requestedSlotCount <= 0)
			return "manifest has no executable spawn slots";
		if (requestedSlotCount > MAX_SLOTS_PER_REQUEST)
			return string.Format("spawn request has %1 slots; limit is %2", requestedSlotCount, MAX_SLOTS_PER_REQUEST);
		if (CountNonterminalBatches(batches) >= MAX_NONTERMINAL_BATCHES)
			return string.Format("nonterminal spawn batch capacity reached (%1)", MAX_NONTERMINAL_BATCHES);
		if (CountNonterminalSlots(batches) + requestedSlotCount > MAX_TOTAL_SLOT_ROWS)
			return string.Format("nonterminal spawn slot capacity would exceed %1", MAX_TOTAL_SLOT_ROWS);
		if (CountTerminalBatches(batches) >= MAX_TERMINAL_ROWS)
			return string.Format("terminal spawn retention capacity reached (%1); run pinned terminal compaction before enqueue", MAX_TERMINAL_ROWS);
		return "";
	}

	protected HST_ForceSpawnResultState BuildBatch(
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueRequest request,
		array<ref HST_ForceSpawnQueueManifestSlot> slots,
		int nowSecond)
	{
		HST_ForceSpawnResultState batch = new HST_ForceSpawnResultState();
		batch.m_sResultId = request.m_sResultId;
		batch.m_sRequestId = request.m_sRequestId;
		batch.m_sManifestId = manifest.m_sManifestId;
		batch.m_sManifestHash = manifest.m_sManifestHash;
		batch.m_sOperationId = manifest.m_sOperationId;
		batch.m_sForceId = request.m_sForceId;
		batch.m_sProjectionId = request.m_sProjectionId;
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING;
		batch.m_iPriority = request.m_iPriority;
		batch.m_iMaxRetries = request.m_iMaxRetries;
		batch.m_iDeadlineSecond = request.m_iDeadlineSecond;
		batch.m_bExternalAssetAuthority = request.m_bExternalAssetAuthority;
		batch.m_iCreatedAtSecond = nowSecond;
		batch.m_iUpdatedAtSecond = nowSecond;
		batch.m_iExpectedSlotCount = CountExecutableManifestSlots(slots, request.m_bExternalAssetAuthority);
		foreach (HST_ForceSpawnQueueManifestSlot descriptor : slots)
		{
			if (request.m_bExternalAssetAuthority && descriptor
				&& descriptor.m_sSlotKind == SLOT_KIND_ASSET)
				continue;
			HST_ForceSpawnSlotResultState slotResult = new HST_ForceSpawnSlotResultState();
			slotResult.m_sSlotId = descriptor.m_sSlotId;
			slotResult.m_sSlotKind = descriptor.m_sSlotKind;
			slotResult.m_sProjectionId = request.m_sProjectionId;
			slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED;
			slotResult.m_iUpdatedAtSecond = nowSecond;
			batch.m_aSlotResults.Insert(slotResult);
		}
		return batch;
	}

	protected HST_ForceSpawnQueueManifestValidation ValidateManifest(HST_ForceManifestState manifest)
	{
		HST_ForceSpawnQueueManifestValidation result = new HST_ForceSpawnQueueManifestValidation();
		string headerFailure = ValidateManifestHeader(manifest);
		if (!headerFailure.IsEmpty())
		{
			result.m_sFailureReason = headerFailure;
			return result;
		}
		if (!AppendGroupSlots(manifest, result))
			return result;
		if (!AppendVehicleSlots(manifest, result))
			return result;
		if (!AppendMemberSlots(manifest, result))
			return result;
		if (!AppendAssetSlots(manifest, result))
			return result;
		string relationshipFailure = ValidateManifestRelationships(manifest, result.m_aSlots);
		if (!relationshipFailure.IsEmpty())
		{
			result.m_sFailureReason = relationshipFailure;
			return result;
		}
		result.m_bValid = true;
		return result;
	}

	protected string ValidateManifestHeader(HST_ForceManifestState manifest)
	{
		if (!manifest)
			return "force manifest missing";
		if (!manifest.m_bFrozen)
			return "force manifest is not frozen";
		if (manifest.m_sManifestId.IsEmpty() || manifest.m_sManifestHash.IsEmpty())
			return "force manifest identity or hash missing";
		if (manifest.m_sOperationId.IsEmpty() || manifest.m_sForceKind.IsEmpty() || manifest.m_sFactionKey.IsEmpty())
			return "force manifest operation identity incomplete";
		if (!m_Integrity || m_Integrity.BuildManifestHash(manifest) != manifest.m_sManifestHash)
			return "force manifest hash conflict";
		if (!manifest.m_aGroups || !manifest.m_aMembers || !manifest.m_aVehicles || !manifest.m_aAssets)
			return "force manifest slot collections missing";
		if (manifest.m_iAcceptedMemberCount != manifest.m_aMembers.Count())
			return "force manifest accepted member count conflict";
		if (manifest.m_iAcceptedVehicleCount != manifest.m_aVehicles.Count())
			return "force manifest accepted vehicle count conflict";
		if (manifest.m_iRequestedMemberCount < manifest.m_iAcceptedMemberCount || manifest.m_iRequestedVehicleCount < manifest.m_iAcceptedVehicleCount)
			return "force manifest requested count is below accepted count";
		return "";
	}

	protected bool AppendGroupSlots(HST_ForceManifestState manifest, HST_ForceSpawnQueueManifestValidation result)
	{
		if (manifest.m_aGroups.Count() == 0)
		{
			result.m_sFailureReason = "force manifest has no executable group root";
			return false;
		}
		array<int> ordinals = {};
		foreach (HST_ForceManifestGroupState group : manifest.m_aGroups)
		{
			if (!group || group.m_sElementId.IsEmpty())
			{
				result.m_sFailureReason = "force manifest group identity missing";
				return false;
			}
			if (!group.m_bRequired)
			{
				result.m_sFailureReason = "optional force manifest groups require a deployed-manifest policy";
				return false;
			}
			string prefab = group.m_sPrefab;
			if (prefab.IsEmpty())
				prefab = manifest.m_sGroupPrefab;
			if (prefab.IsEmpty())
			{
				result.m_sFailureReason = "force manifest group prefab missing";
				return false;
			}
			if (group.m_iOrdinal < 0 || ordinals.Contains(group.m_iOrdinal))
			{
				result.m_sFailureReason = "force manifest group ordinal conflict";
				return false;
			}
			if (group.m_iExpectedMemberCount < 0 || HasManifestSlot(result.m_aSlots, group.m_sElementId))
			{
				result.m_sFailureReason = "force manifest group slot conflict";
				return false;
			}
			ordinals.Insert(group.m_iOrdinal);
			HST_ForceSpawnQueueManifestSlot descriptor = new HST_ForceSpawnQueueManifestSlot();
			descriptor.m_sSlotId = group.m_sElementId;
			descriptor.m_sSlotKind = SLOT_KIND_GROUP;
			descriptor.m_sPrefab = prefab;
			descriptor.m_sRole = group.m_sRole;
			descriptor.m_sGroupElementId = group.m_sElementId;
			descriptor.m_iOrdinal = group.m_iOrdinal;
			descriptor.m_bRequired = group.m_bRequired;
			result.m_aSlots.Insert(descriptor);
		}
		return true;
	}

	protected bool AppendVehicleSlots(HST_ForceManifestState manifest, HST_ForceSpawnQueueManifestValidation result)
	{
		array<int> ordinals = {};
		foreach (HST_ForceManifestVehicleState vehicle : manifest.m_aVehicles)
		{
			if (!vehicle || vehicle.m_sSlotId.IsEmpty() || vehicle.m_sPrefab.IsEmpty())
			{
				result.m_sFailureReason = "force manifest vehicle slot identity or prefab missing";
				return false;
			}
			if (!vehicle.m_bRequired)
			{
				result.m_sFailureReason = "optional force manifest vehicles require a deployed-manifest policy";
				return false;
			}
			if (vehicle.m_iOrdinal < 0 || ordinals.Contains(vehicle.m_iOrdinal))
			{
				result.m_sFailureReason = "force manifest vehicle ordinal conflict";
				return false;
			}
			if (vehicle.m_sGroupElementId.IsEmpty() || HasManifestSlot(result.m_aSlots, vehicle.m_sSlotId))
			{
				result.m_sFailureReason = "force manifest vehicle group or slot conflict";
				return false;
			}
			ordinals.Insert(vehicle.m_iOrdinal);
			HST_ForceSpawnQueueManifestSlot descriptor = new HST_ForceSpawnQueueManifestSlot();
			descriptor.m_sSlotId = vehicle.m_sSlotId;
			descriptor.m_sSlotKind = SLOT_KIND_VEHICLE;
			descriptor.m_sPrefab = vehicle.m_sPrefab;
			descriptor.m_sRole = vehicle.m_sRole;
			descriptor.m_sGroupElementId = vehicle.m_sGroupElementId;
			descriptor.m_iOrdinal = vehicle.m_iOrdinal;
			descriptor.m_iRequiredCrew = vehicle.m_iRequiredCrew;
			descriptor.m_bRequired = vehicle.m_bRequired;
			result.m_aSlots.Insert(descriptor);
		}
		return true;
	}

	protected bool AppendMemberSlots(HST_ForceManifestState manifest, HST_ForceSpawnQueueManifestValidation result)
	{
		array<int> ordinals = {};
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member || member.m_sSlotId.IsEmpty() || member.m_sPrefab.IsEmpty())
			{
				result.m_sFailureReason = "force manifest member slot identity or prefab missing";
				return false;
			}
			if (!member.m_bRequired)
			{
				result.m_sFailureReason = "optional force manifest members require a deployed-manifest policy";
				return false;
			}
			if (member.m_iOrdinal < 0 || ordinals.Contains(member.m_iOrdinal))
			{
				result.m_sFailureReason = "force manifest member ordinal conflict";
				return false;
			}
			if (member.m_sGroupElementId.IsEmpty() || HasManifestSlot(result.m_aSlots, member.m_sSlotId))
			{
				result.m_sFailureReason = "force manifest member group or slot conflict";
				return false;
			}
			ordinals.Insert(member.m_iOrdinal);
			HST_ForceSpawnQueueManifestSlot descriptor = new HST_ForceSpawnQueueManifestSlot();
			descriptor.m_sSlotId = member.m_sSlotId;
			descriptor.m_sSlotKind = SLOT_KIND_MEMBER;
			descriptor.m_sPrefab = member.m_sPrefab;
			descriptor.m_sRole = member.m_sRole;
			descriptor.m_sGroupElementId = member.m_sGroupElementId;
			descriptor.m_sAssignedVehicleSlotId = member.m_sAssignedVehicleSlotId;
			descriptor.m_sSeatRole = member.m_sSeatRole;
			descriptor.m_iOrdinal = member.m_iOrdinal;
			descriptor.m_iSeatIndex = member.m_iSeatIndex;
			descriptor.m_bRequired = member.m_bRequired;
			result.m_aSlots.Insert(descriptor);
		}
		return true;
	}

	protected bool AppendAssetSlots(HST_ForceManifestState manifest, HST_ForceSpawnQueueManifestValidation result)
	{
		array<int> ordinals = {};
		foreach (HST_ForceManifestAssetState asset : manifest.m_aAssets)
		{
			if (!asset || asset.m_sSlotId.IsEmpty() || asset.m_sPrefab.IsEmpty())
			{
				result.m_sFailureReason = "force manifest asset slot identity or prefab missing";
				return false;
			}
			if (!asset.m_bRequired)
			{
				result.m_sFailureReason = "optional force manifest assets require a deployed-manifest policy";
				return false;
			}
			if (asset.m_iQuantity != 1)
			{
				result.m_sFailureReason = "force manifest asset quantity cannot map exactly to one durable slot";
				return false;
			}
			if (asset.m_iOrdinal < 0 || ordinals.Contains(asset.m_iOrdinal) || HasManifestSlot(result.m_aSlots, asset.m_sSlotId))
			{
				result.m_sFailureReason = "force manifest asset ordinal or slot conflict";
				return false;
			}
			ordinals.Insert(asset.m_iOrdinal);
			HST_ForceSpawnQueueManifestSlot descriptor = new HST_ForceSpawnQueueManifestSlot();
			descriptor.m_sSlotId = asset.m_sSlotId;
			descriptor.m_sSlotKind = SLOT_KIND_ASSET;
			descriptor.m_sPrefab = asset.m_sPrefab;
			descriptor.m_sRole = asset.m_sRole;
			descriptor.m_sAssignedVehicleSlotId = asset.m_sAssignedVehicleSlotId;
			descriptor.m_iOrdinal = asset.m_iOrdinal;
			descriptor.m_bRequired = asset.m_bRequired;
			result.m_aSlots.Insert(descriptor);
		}
		return true;
	}

	protected string ValidateManifestRelationships(
		HST_ForceManifestState manifest,
		array<ref HST_ForceSpawnQueueManifestSlot> slots)
	{
		foreach (HST_ForceManifestGroupState group : manifest.m_aGroups)
		{
			if (!group)
				return "force manifest group missing";
			if (CountMembersForGroup(manifest, group.m_sElementId) != group.m_iExpectedMemberCount)
				return "force manifest group member count conflict";
		}
		foreach (HST_ForceManifestVehicleState vehicle : manifest.m_aVehicles)
		{
			if (!vehicle || !FindManifestSlot(slots, vehicle.m_sGroupElementId, SLOT_KIND_GROUP))
				return "force manifest vehicle references an unknown group";
		}
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member || !FindManifestSlot(slots, member.m_sGroupElementId, SLOT_KIND_GROUP))
				return "force manifest member references an unknown group";
			if (!member.m_sAssignedVehicleSlotId.IsEmpty() && !FindManifestSlot(slots, member.m_sAssignedVehicleSlotId, SLOT_KIND_VEHICLE))
				return "force manifest member references an unknown vehicle";
		}
		foreach (HST_ForceManifestAssetState asset : manifest.m_aAssets)
		{
			if (!asset)
				return "force manifest asset missing";
			if (!asset.m_sAssignedVehicleSlotId.IsEmpty() && !FindManifestSlot(slots, asset.m_sAssignedVehicleSlotId, SLOT_KIND_VEHICLE))
				return "force manifest asset references an unknown vehicle";
		}
		return ValidateManifestSeatAssignments(manifest);
	}

	protected string ValidateManifestSeatAssignments(HST_ForceManifestState manifest)
	{
		array<string> seatKeys = {};
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member)
				return "force manifest member missing";
			if (member.m_sAssignedVehicleSlotId.IsEmpty())
			{
				if (member.m_iSeatIndex != -1 || !member.m_sSeatRole.IsEmpty())
					return "unassigned manifest member contains seat authority";
				continue;
			}
			if (member.m_iSeatIndex < 0 || member.m_sSeatRole.IsEmpty())
				return "assigned manifest member seat authority incomplete";
			string seatKey = string.Format("%1|%2", member.m_sAssignedVehicleSlotId, member.m_iSeatIndex);
			if (seatKeys.Contains(seatKey))
				return "force manifest assigns multiple members to one vehicle seat";
			seatKeys.Insert(seatKey);
		}
		foreach (HST_ForceManifestVehicleState vehicle : manifest.m_aVehicles)
		{
			if (!vehicle || vehicle.m_iRequiredCrew < 0)
				return "force manifest vehicle required crew is invalid";
			if (CountAssignedMembers(manifest, vehicle.m_sSlotId) < vehicle.m_iRequiredCrew)
				return "force manifest vehicle required crew is not assigned";
		}
		return "";
	}

	protected int CountAssignedMembers(HST_ForceManifestState manifest, string vehicleSlotId)
	{
		int count;
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (member && member.m_sAssignedVehicleSlotId == vehicleSlotId)
				count++;
		}
		return count;
	}

	protected int CountMembersForGroup(HST_ForceManifestState manifest, string groupElementId)
	{
		int count;
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (member && member.m_sGroupElementId == groupElementId)
				count++;
		}
		return count;
	}

	protected bool HasManifestSlot(array<ref HST_ForceSpawnQueueManifestSlot> slots, string slotId)
	{
		return FindManifestSlot(slots, slotId, "") != null;
	}

	protected HST_ForceSpawnQueueManifestSlot FindManifestSlot(
		array<ref HST_ForceSpawnQueueManifestSlot> slots,
		string slotId,
		string slotKind)
	{
		if (!slots || slotId.IsEmpty())
			return null;
		foreach (HST_ForceSpawnQueueManifestSlot descriptor : slots)
		{
			if (!descriptor || descriptor.m_sSlotId != slotId)
				continue;
			if (slotKind.IsEmpty() || descriptor.m_sSlotKind == slotKind)
				return descriptor;
		}
		return null;
	}

	protected bool BatchSlotsMatchManifest(
		HST_ForceSpawnResultState batch,
		array<ref HST_ForceSpawnQueueManifestSlot> descriptors)
	{
		if (!batch || !descriptors)
			return false;
		int expectedCount = CountExecutableManifestSlots(descriptors, batch.m_bExternalAssetAuthority);
		if (batch.m_iExpectedSlotCount != expectedCount)
			return false;
		if (!batch.m_aSlotResults || batch.m_aSlotResults.Count() != expectedCount)
			return false;
		foreach (HST_ForceSpawnQueueManifestSlot descriptor : descriptors)
		{
			if (IsExternallyManagedAssetDescriptor(batch, descriptor))
				continue;
			HST_ForceSpawnSlotResultState slotResult = batch.FindSlotResult(descriptor.m_sSlotId);
			if (!slotResult || slotResult.m_sSlotKind != descriptor.m_sSlotKind)
				return false;
		}
		return true;
	}

	protected int CountExecutableManifestSlots(
		array<ref HST_ForceSpawnQueueManifestSlot> descriptors,
		bool externalAssetAuthority)
	{
		int count;
		if (!descriptors)
			return count;
		foreach (HST_ForceSpawnQueueManifestSlot descriptor : descriptors)
		{
			if (!descriptor)
				continue;
			if (externalAssetAuthority && descriptor.m_sSlotKind == SLOT_KIND_ASSET)
				continue;
			count++;
		}
		return count;
	}

	protected bool IsExternallyManagedAssetDescriptor(
		HST_ForceSpawnResultState batch,
		HST_ForceSpawnQueueManifestSlot descriptor)
	{
		return batch && batch.m_bExternalAssetAuthority && descriptor
			&& descriptor.m_sSlotKind == SLOT_KIND_ASSET;
	}

	HST_ForceSpawnQueueTickResult AcquireWork(
		array<ref HST_ForceSpawnResultState> batches,
		array<ref HST_ForceManifestState> manifests,
		int nowSecond)
	{
		HST_ForceSpawnQueueTickResult tick = new HST_ForceSpawnQueueTickResult();
		if (!batches || !manifests)
		{
			tick.m_sSummary = "spawn queue tick unavailable: durable batches or manifests missing";
			return tick;
		}

		if (FailClosedDuplicateNonterminalKeys(batches, manifests, nowSecond))
			tick.m_bStateChanged = true;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (IsExternallyManagedMissionConvoyBatch(batch, manifests))
				continue;
			if (CollidesWithExternallyManagedMissionConvoyBatch(batch, batches, manifests))
				continue;
			if (PrepareBatchForTick(batch, manifests, nowSecond))
				tick.m_bStateChanged = true;
			if (batch && (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_DEFERRED
				|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_RETRYABLE))
				tick.m_iDeferredBatchCount++;
		}

		array<string> selectedResultIds = {};
		while (tick.m_iBatchesSelected < MAX_BATCHES_PER_TICK && tick.m_iSlotsSelected < MAX_SLOTS_PER_TICK)
		{
			HST_ForceSpawnResultState batch = SelectNextWorkBatch(batches, manifests, selectedResultIds, nowSecond);
			if (!batch)
				break;
			selectedResultIds.Insert(batch.m_sResultId);
			HST_ForceManifestState manifest = FindManifest(manifests, batch.m_sManifestId);
			if (StartAttemptIfReady(batch, nowSecond))
				tick.m_bStateChanged = true;
			int remainingBudget = MAX_SLOTS_PER_TICK - tick.m_iSlotsSelected;
			int added = AppendWorkItems(tick.m_aWorkItems, batch, manifest, remainingBudget, nowSecond);
			if (added <= 0)
				continue;
			tick.m_iBatchesSelected++;
			tick.m_iSlotsSelected += added;
			tick.m_bStateChanged = true;
		}
		tick.m_sSummary = string.Format(
			"spawn queue tick | batches %1/%2 | actions %3/%4 | deferred %5",
			tick.m_iBatchesSelected,
			MAX_BATCHES_PER_TICK,
			tick.m_iSlotsSelected,
			MAX_SLOTS_PER_TICK,
			tick.m_iDeferredBatchCount
		);
		return tick;
	}

	protected bool PrepareBatchForTick(
		HST_ForceSpawnResultState batch,
		array<ref HST_ForceManifestState> manifests,
		int nowSecond)
	{
		if (!batch || IsTerminalBatch(batch))
			return false;
		HST_ForceManifestState manifest = FindManifest(manifests, batch.m_sManifestId);
		string integrityFailure = ValidateBatchManifest(batch, manifest);
		if (!integrityFailure.IsEmpty())
		{
			BeginCleanup(batch, CLEANUP_DISPOSITION_FINAL, "spawn batch integrity conflict: " + integrityFailure, nowSecond);
			AdvanceCleanupIfReady(batch, nowSecond);
			return true;
		}
		if (IsAcceptedCancellationCleanup(batch))
			return AdvanceCleanupIfReady(batch, nowSecond);
		if (batch.m_bCancelRequested)
		{
			BeginCleanup(batch, CLEANUP_DISPOSITION_CANCEL, "spawn cancellation requested", nowSecond);
			AdvanceCleanupIfReady(batch, nowSecond);
			return true;
		}
		if (batch.m_bStrategicProjectionHeld)
			return false;
		if (batch.m_iDeadlineSecond > 0 && nowSecond >= batch.m_iDeadlineSecond)
		{
			BeginCleanup(batch, CLEANUP_DISPOSITION_FINAL, "spawn deadline expired", nowSecond);
			AdvanceCleanupIfReady(batch, nowSecond);
			return true;
		}
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
			return AdvanceCleanupIfReady(batch, nowSecond);
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS)
			return false;
		if (AdvanceInProgressState(batch, nowSecond))
			return true;
		if (!AllSlotsRegisteredAndVerified(batch, manifest))
			return false;
		MarkBatchReadyForHandoff(batch, nowSecond);
		return true;
	}

	protected bool StartAttemptIfReady(HST_ForceSpawnResultState batch, int nowSecond)
	{
		if (!batch || batch.m_iNextAttemptSecond > nowSecond)
			return false;
		if (!CanStartSpawnAttempt(batch))
			return false;
		batch.m_iAttemptGeneration++;
		batch.m_iLastAttemptSecond = nowSecond;
		batch.m_iUpdatedAtSecond = nowSecond;
		batch.m_iNextAttemptSecond = 0;
		batch.m_sTerminalReason = "";
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (!slotResult)
				continue;
			if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED)
				continue;
			if (slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED
				&& slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_DEFERRED
				&& slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_RETRYABLE)
				continue;
			ResetSlotForAttempt(slotResult, batch.m_sProjectionId, nowSecond);
		}
		return true;
	}

	protected bool CanStartSpawnAttempt(HST_ForceSpawnResultState batch)
	{
		return batch && (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_DEFERRED
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_RETRYABLE);
	}

	protected void ResetSlotForAttempt(HST_ForceSpawnSlotResultState slotResult, string projectionId, int nowSecond)
	{
		slotResult.m_sSpawnedPrefab = "";
		slotResult.m_sEntityId = "";
		slotResult.m_sAssignedVehicleEntityId = "";
		slotResult.m_sNativeGroupId = "";
		slotResult.m_sProjectionId = projectionId;
		slotResult.m_sFailureReason = "";
		slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED;
		slotResult.m_iUpdatedAtSecond = nowSecond;
		slotResult.m_bAliveVerified = false;
		slotResult.m_bFactionVerified = false;
		slotResult.m_bGroupVerified = false;
		slotResult.m_bGameMasterVerified = false;
		slotResult.m_bProjectionVerified = false;
		slotResult.m_bSeatVerified = false;
	}

	protected HST_ForceSpawnResultState SelectNextWorkBatch(
		array<ref HST_ForceSpawnResultState> batches,
		array<ref HST_ForceManifestState> manifests,
		array<string> excludedResultIds,
		int nowSecond)
	{
		HST_ForceSpawnResultState selected;
		foreach (HST_ForceSpawnResultState candidate : batches)
		{
			if (!candidate || excludedResultIds.Contains(candidate.m_sResultId))
				continue;
			if (IsExternallyManagedMissionConvoyBatch(candidate, manifests))
				continue;
			if (CollidesWithExternallyManagedMissionConvoyBatch(candidate, batches, manifests))
				continue;
			HST_ForceManifestState manifest = FindManifest(manifests, candidate.m_sManifestId);
			if (!IsWorkEligible(candidate, manifest, nowSecond))
				continue;
			if (!selected || IsHigherQueuePriority(candidate, selected))
				selected = candidate;
		}
		return selected;
	}

	protected bool IsWorkEligible(HST_ForceSpawnResultState batch, HST_ForceManifestState manifest, int nowSecond)
	{
		if (!batch || batch.m_iNextAttemptSecond > nowSecond)
			return false;
		if (batch.m_bStrategicProjectionHeld)
			return false;
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
			return HasCleanupWork(batch);
		if (!manifest)
			return false;
		if (!CanStartSpawnAttempt(batch)
			&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS)
			return false;
		return HasSpawnWork(batch, manifest);
	}

	protected bool IsHigherQueuePriority(HST_ForceSpawnResultState candidate, HST_ForceSpawnResultState selected)
	{
		bool candidateCleanup = candidate.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING;
		bool selectedCleanup = selected.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING;
		if (candidateCleanup != selectedCleanup)
			return candidateCleanup;
		if (candidate.m_iPriority != selected.m_iPriority)
			return candidate.m_iPriority > selected.m_iPriority;
		return candidate.m_iCreatedAtSecond < selected.m_iCreatedAtSecond;
	}

	protected int AppendWorkItems(
		array<ref HST_ForceSpawnQueueWorkItem> workItems,
		HST_ForceSpawnResultState batch,
		HST_ForceManifestState manifest,
		int budget,
		int nowSecond)
	{
		if (!workItems || !batch || budget <= 0)
			return 0;
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
			return AppendCleanupWorkItems(workItems, batch, budget, nowSecond);
		if (!manifest)
			return 0;
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS)
			return 0;
		HST_ForceSpawnQueueManifestValidation validation = ValidateManifest(manifest);
		if (!validation.m_bValid)
			return 0;
		int added;
		foreach (HST_ForceSpawnQueueManifestSlot descriptor : validation.m_aSlots)
		{
			if (IsExternallyManagedAssetDescriptor(batch, descriptor))
				continue;
			if (added >= budget)
				break;
			HST_ForceSpawnSlotResultState slotResult = batch.FindSlotResult(descriptor.m_sSlotId);
			if (!slotResult || slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED)
				continue;
			if (!DependenciesRegistered(batch, descriptor))
				continue;
			HST_ForceSpawnQueueWorkItem item = BuildWorkItem(ACTION_SPAWN, batch, descriptor, slotResult);
			workItems.Insert(item);
			slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING;
			slotResult.m_iAttemptCount++;
			slotResult.m_iUpdatedAtSecond = nowSecond;
			added++;
		}
		return added;
	}

	protected int AppendCleanupWorkItems(
		array<ref HST_ForceSpawnQueueWorkItem> workItems,
		HST_ForceSpawnResultState batch,
		int budget,
		int nowSecond)
	{
		int added = AppendCleanupWorkItemsForKind(workItems, batch, SLOT_KIND_ASSET, budget, nowSecond);
		if (added < budget)
			added += AppendCleanupWorkItemsForKind(workItems, batch, SLOT_KIND_MEMBER, budget - added, nowSecond);
		if (added < budget)
			added += AppendCleanupWorkItemsForKind(workItems, batch, SLOT_KIND_VEHICLE, budget - added, nowSecond);
		if (added < budget)
			added += AppendCleanupWorkItemsForKind(workItems, batch, SLOT_KIND_GROUP, budget - added, nowSecond);
		return added;
	}

	protected int AppendCleanupWorkItemsForKind(
		array<ref HST_ForceSpawnQueueWorkItem> workItems,
		HST_ForceSpawnResultState batch,
		string slotKind,
		int budget,
		int nowSecond)
	{
		int added;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (added >= budget)
				break;
			if (!slotResult || slotResult.m_sSlotKind != slotKind
				|| slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING)
				continue;
			HST_ForceSpawnQueueWorkItem item = BuildCleanupWorkItem(batch, slotResult);
			workItems.Insert(item);
			slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING;
			slotResult.m_iUpdatedAtSecond = nowSecond;
			added++;
		}
		return added;
	}

	protected HST_ForceSpawnQueueWorkItem BuildCleanupWorkItem(
		HST_ForceSpawnResultState batch,
		HST_ForceSpawnSlotResultState slotResult)
	{
		HST_ForceSpawnQueueWorkItem item = new HST_ForceSpawnQueueWorkItem();
		item.m_sAction = ACTION_CLEANUP;
		item.m_sResultId = batch.m_sResultId;
		item.m_sRequestId = batch.m_sRequestId;
		item.m_sManifestId = batch.m_sManifestId;
		item.m_sManifestHash = batch.m_sManifestHash;
		item.m_sOperationId = batch.m_sOperationId;
		item.m_sForceId = batch.m_sForceId;
		item.m_sProjectionId = batch.m_sProjectionId;
		item.m_sSlotId = slotResult.m_sSlotId;
		item.m_sSlotKind = slotResult.m_sSlotKind;
		item.m_sPrefab = slotResult.m_sSpawnedPrefab;
		item.m_sAssignedVehicleEntityId = slotResult.m_sAssignedVehicleEntityId;
		item.m_sEntityId = slotResult.m_sEntityId;
		item.m_sNativeGroupId = slotResult.m_sNativeGroupId;
		item.m_iAttemptGeneration = batch.m_iAttemptGeneration;
		item.m_iAttemptCount = slotResult.m_iAttemptCount;
		item.m_bRequired = true;
		return item;
	}

	protected HST_ForceSpawnQueueWorkItem BuildWorkItem(
		string action,
		HST_ForceSpawnResultState batch,
		HST_ForceSpawnQueueManifestSlot descriptor,
		HST_ForceSpawnSlotResultState slotResult)
	{
		HST_ForceSpawnQueueWorkItem item = new HST_ForceSpawnQueueWorkItem();
		item.m_sAction = action;
		item.m_sResultId = batch.m_sResultId;
		item.m_sRequestId = batch.m_sRequestId;
		item.m_sManifestId = batch.m_sManifestId;
		item.m_sManifestHash = batch.m_sManifestHash;
		item.m_sOperationId = batch.m_sOperationId;
		item.m_sForceId = batch.m_sForceId;
		item.m_sProjectionId = batch.m_sProjectionId;
		item.m_sSlotId = descriptor.m_sSlotId;
		item.m_sSlotKind = descriptor.m_sSlotKind;
		item.m_sPrefab = descriptor.m_sPrefab;
		item.m_sRole = descriptor.m_sRole;
		item.m_sGroupElementId = descriptor.m_sGroupElementId;
		item.m_sAssignedVehicleSlotId = descriptor.m_sAssignedVehicleSlotId;
		item.m_sAssignedVehicleEntityId = ResolveAssignedVehicleEntity(batch, descriptor.m_sAssignedVehicleSlotId);
		item.m_sSeatRole = descriptor.m_sSeatRole;
		item.m_sEntityId = slotResult.m_sEntityId;
		item.m_sNativeGroupId = slotResult.m_sNativeGroupId;
		item.m_iAttemptGeneration = batch.m_iAttemptGeneration;
		item.m_iAttemptCount = slotResult.m_iAttemptCount;
		item.m_iOrdinal = descriptor.m_iOrdinal;
		item.m_iSeatIndex = descriptor.m_iSeatIndex;
		item.m_iRequiredCrew = descriptor.m_iRequiredCrew;
		item.m_bRequired = descriptor.m_bRequired;
		return item;
	}

	protected bool HasSpawnWork(HST_ForceSpawnResultState batch, HST_ForceManifestState manifest)
	{
		HST_ForceSpawnQueueManifestValidation validation = ValidateManifest(manifest);
		if (!validation.m_bValid || !BatchSlotsMatchManifest(batch, validation.m_aSlots))
			return false;
		foreach (HST_ForceSpawnQueueManifestSlot descriptor : validation.m_aSlots)
		{
			if (IsExternallyManagedAssetDescriptor(batch, descriptor))
				continue;
			HST_ForceSpawnSlotResultState slotResult = batch.FindSlotResult(descriptor.m_sSlotId);
			if (IsSlotSelectableForSpawnAttempt(batch, slotResult)
				&& DependenciesRegistered(batch, descriptor))
				return true;
		}
		return false;
	}

	protected bool IsSlotSelectableForSpawnAttempt(
		HST_ForceSpawnResultState batch,
		HST_ForceSpawnSlotResultState slotResult)
	{
		if (!slotResult)
			return false;
		if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED)
			return true;
		if (!CanStartSpawnAttempt(batch))
			return false;
		return slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_DEFERRED
			|| slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_RETRYABLE;
	}

	protected bool HasCleanupWork(HST_ForceSpawnResultState batch)
	{
		if (!batch || !batch.m_aSlotResults)
			return false;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (slotResult && slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING)
				return true;
		}
		return false;
	}

	protected bool HasCleanupOutstanding(HST_ForceSpawnResultState batch)
	{
		if (!batch || !batch.m_aSlotResults)
			return false;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (slotResult && (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING
				|| slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING))
				return true;
		}
		return false;
	}

	protected bool DependenciesRegistered(HST_ForceSpawnResultState batch, HST_ForceSpawnQueueManifestSlot descriptor)
	{
		if (!batch || !descriptor)
			return false;
		if (descriptor.m_sSlotKind == SLOT_KIND_GROUP)
			return true;
		if (!descriptor.m_sGroupElementId.IsEmpty())
		{
			HST_ForceSpawnSlotResultState groupResult = batch.FindSlotResult(descriptor.m_sGroupElementId);
			if (!IsRegisteredSlot(groupResult) || groupResult.m_sNativeGroupId.IsEmpty())
				return false;
		}
		else if (descriptor.m_sSlotKind == SLOT_KIND_ASSET && !AllGroupSlotsRegistered(batch))
		{
			return false;
		}
		if (descriptor.m_sAssignedVehicleSlotId.IsEmpty())
			return true;
		HST_ForceSpawnSlotResultState vehicleResult = batch.FindSlotResult(descriptor.m_sAssignedVehicleSlotId);
		return IsRegisteredSlot(vehicleResult) && !vehicleResult.m_sEntityId.IsEmpty();
	}

	protected bool AllGroupSlotsRegistered(HST_ForceSpawnResultState batch)
	{
		bool foundGroup;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (!slotResult || slotResult.m_sSlotKind != SLOT_KIND_GROUP)
				continue;
			foundGroup = true;
			if (!IsRegisteredSlot(slotResult) || slotResult.m_sNativeGroupId.IsEmpty())
				return false;
		}
		return foundGroup;
	}

	protected bool IsRegisteredSlot(HST_ForceSpawnSlotResultState slotResult)
	{
		return slotResult && slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
	}

	protected string ResolveAssignedVehicleEntity(HST_ForceSpawnResultState batch, string vehicleSlotId)
	{
		if (!batch || vehicleSlotId.IsEmpty())
			return "";
		HST_ForceSpawnSlotResultState vehicleResult = batch.FindSlotResult(vehicleSlotId);
		if (!IsRegisteredSlot(vehicleResult))
			return "";
		return vehicleResult.m_sEntityId;
	}

	protected string ValidateBatchManifest(HST_ForceSpawnResultState batch, HST_ForceManifestState manifest)
	{
		if (!batch)
			return "batch missing";
		if (!manifest)
			return "manifest missing";
		if (batch.m_sManifestId != manifest.m_sManifestId || batch.m_sManifestHash != manifest.m_sManifestHash)
			return "manifest identity mismatch";
		if (batch.m_sOperationId != manifest.m_sOperationId)
			return "operation identity mismatch";
		if (batch.m_bExternalAssetAuthority)
		{
			string externalAssetFailure = HST_RescuePOWExternalAssetPolicy.ValidateManifestShape(manifest);
			if (!externalAssetFailure.IsEmpty())
				return "external asset authority conflicts with exact rescue manifest: " + externalAssetFailure;
		}
		if (batch.m_sResultId.IsEmpty() || batch.m_sRequestId.IsEmpty() || batch.m_sProjectionId.IsEmpty() || batch.m_sForceId.IsEmpty())
			return "durable batch identity incomplete";
		HST_ForceSpawnQueueManifestValidation validation = ValidateManifest(manifest);
		if (!validation.m_bValid)
			return validation.m_sFailureReason;
		if (!BatchSlotsMatchManifest(batch, validation.m_aSlots))
			return "durable batch slots do not exactly match manifest";
		return "";
	}

	protected HST_ForceManifestState FindManifest(array<ref HST_ForceManifestState> manifests, string manifestId)
	{
		if (!manifests || manifestId.IsEmpty())
			return null;
		HST_ForceManifestState found;
		foreach (HST_ForceManifestState manifest : manifests)
		{
			if (!manifest || manifest.m_sManifestId != manifestId)
				continue;
			if (found)
				return null;
			found = manifest;
		}
		return found;
	}

	HST_ForceSpawnQueueCallbackResult CompleteSlotSuccess(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueSlotSuccess success,
		int nowSecond)
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		if (!batches || !manifest || !success || success.m_sResultId.IsEmpty() || success.m_sSlotId.IsEmpty())
		{
			result.m_sFailureReason = "spawn success callback incomplete";
			return result;
		}
		if (CountByResult(batches, success.m_sResultId) != 1)
		{
			result.m_sFailureReason = "spawn success callback result identity is missing or ambiguous";
			return result;
		}
		HST_ForceSpawnResultState batch = FindByResult(batches, success.m_sResultId);
		result.m_Batch = batch;
		if (!BatchHasUniqueKeys(batches, batch))
		{
			result.m_sFailureReason = "spawn success callback durable identity is ambiguous";
			result.m_bCleanupRequired = ShouldCleanupReturnedEntity(batches, batch, success.m_sEntityId, success.m_sNativeGroupId);
			return result;
		}
		if (success.m_sProjectionId != batch.m_sProjectionId)
		{
			result.m_sFailureReason = "spawn success callback projection identity conflict";
			result.m_bCleanupRequired = ShouldCleanupReturnedEntity(batches, batch, success.m_sEntityId, success.m_sNativeGroupId);
			return result;
		}
		if (success.m_iAttemptGeneration != batch.m_iAttemptGeneration)
			return MarkStaleCallback(result, "spawn success callback generation is stale", ShouldCleanupReturnedEntity(batches, batch, success.m_sEntityId, success.m_sNativeGroupId));

		HST_ForceSpawnQueueManifestValidation validation = ValidateManifest(manifest);
		HST_ForceSpawnQueueManifestSlot descriptor = FindManifestSlot(validation.m_aSlots, success.m_sSlotId, "");
		string batchFailure = ValidateBatchManifest(batch, manifest);
		if (!validation.m_bValid || !descriptor || !batchFailure.IsEmpty())
		{
			result.m_sFailureReason = "spawn success callback manifest integrity conflict";
			result.m_bCleanupRequired = ShouldCleanupReturnedEntity(batches, batch, success.m_sEntityId, success.m_sNativeGroupId);
			return result;
		}
		HST_ForceSpawnSlotResultState slotResult = batch.FindSlotResult(success.m_sSlotId);
		if (slotResult && slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED)
		{
			if (EntityRegisteredToAnotherSlot(batches, batch, success.m_sSlotId, success.m_sEntityId)
				|| (slotResult.m_sSlotKind == SLOT_KIND_GROUP && GroupRootNativeIdRegisteredElsewhere(batches, batch, success.m_sSlotId, success.m_sNativeGroupId)))
			{
				result.m_sFailureReason = "registered spawn slot entity is duplicated across manifest slots";
				return result;
			}
			return ResolveRegisteredSuccessReplay(result, batches, batch, slotResult, success);
		}
		if (!CanAcceptSpawnSuccess(batch, slotResult))
			return MarkStaleCallback(result, "spawn success callback is no longer expected", ShouldCleanupReturnedEntity(batches, batch, success.m_sEntityId, success.m_sNativeGroupId));
		bool entityOwnershipConflict = EntityRegisteredToAnotherSlot(batches, batch, success.m_sSlotId, success.m_sEntityId);
		bool groupOwnershipConflict;
		if (descriptor.m_sSlotKind == SLOT_KIND_GROUP)
			groupOwnershipConflict = GroupRootNativeIdRegisteredElsewhere(batches, batch, success.m_sSlotId, success.m_sNativeGroupId);
		if (entityOwnershipConflict || groupOwnershipConflict)
		{
			string ownershipFailure = "spawn success callback conflicts with registered entity or native group authority";
			BeginCleanup(batch, CLEANUP_DISPOSITION_FINAL, ownershipFailure, nowSecond);
			result.m_bAccepted = true;
			result.m_bStateChanged = true;
			result.m_bCleanupRequired = !entityOwnershipConflict && ShouldCleanupReturnedEntity(batches, batch, success.m_sEntityId, success.m_sNativeGroupId);
			result.m_sFailureReason = ownershipFailure;
			return result;
		}

		bool scopedSiblingSpawn = batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING
			&& IsScopedRetryCleanup(batch)
			&& slotResult.m_sFailureReason.IsEmpty();
		string evidenceFailure = ValidateSuccessEvidence(batch, descriptor, success);
		ApplySuccessEvidence(slotResult, success, batch.m_sProjectionId, nowSecond);
		if (!evidenceFailure.IsEmpty())
		{
			BeginCleanup(batch, CLEANUP_DISPOSITION_FINAL, evidenceFailure, nowSecond);
			result.m_bAccepted = true;
			result.m_bStateChanged = true;
			result.m_bCleanupRequired = true;
			result.m_sFailureReason = evidenceFailure;
			return result;
		}

		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
		{
			if (scopedSiblingSpawn)
			{
				slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
				if (descriptor.m_sSlotKind == SLOT_KIND_GROUP && batch.m_sNativeGroupId.IsEmpty())
					batch.m_sNativeGroupId = success.m_sNativeGroupId;
				batch.m_iUpdatedAtSecond = nowSecond;
				AdvanceCleanupIfReady(batch, nowSecond);
				return result;
			}
			slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING;
			result.m_bCleanupRequired = true;
			return result;
		}
		slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
		if (descriptor.m_sSlotKind == SLOT_KIND_GROUP && batch.m_sNativeGroupId.IsEmpty())
			batch.m_sNativeGroupId = success.m_sNativeGroupId;
		batch.m_iUpdatedAtSecond = nowSecond;
		if (AllSlotsRegisteredAndVerified(batch, manifest))
			MarkBatchReadyForHandoff(batch, nowSecond);
		return result;
	}

	HST_ForceSpawnQueueCallbackResult CompleteProjectionHandoff(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		string resultId,
		string projectionId,
		int attemptGeneration,
		int nowSecond)
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		if (!batches || !manifest || resultId.IsEmpty() || projectionId.IsEmpty() || CountByResult(batches, resultId) != 1)
		{
			result.m_sFailureReason = "spawn handoff callback identity is missing or ambiguous";
			return result;
		}
		HST_ForceSpawnResultState batch = FindByResult(batches, resultId);
		result.m_Batch = batch;
		if (!BatchHasUniqueKeys(batches, batch) || batch.m_sProjectionId != projectionId)
		{
			result.m_sFailureReason = "spawn handoff callback durable identity conflicts with the queue";
			return result;
		}
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			result.m_bAccepted = true;
			result.m_bAlreadyApplied = true;
			return result;
		}
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_READY_FOR_HANDOFF)
		{
			result.m_sFailureReason = "spawn projection is not ready for physical handoff";
			return result;
		}
		if (attemptGeneration != batch.m_iAttemptGeneration)
			return MarkStaleCallback(result, "spawn handoff callback generation is stale", false);
		if (!ValidateBatchManifest(batch, manifest).IsEmpty() || !AllSlotsRegisteredAndVerified(batch, manifest))
		{
			result.m_sFailureReason = "spawn handoff callback manifest or registered-slot evidence conflicts";
			return result;
		}
		MarkBatchSucceeded(batch, nowSecond);
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		return result;
	}

	HST_ForceSpawnQueueCallbackResult ConfirmRegisteredMemberCasualty(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		string resultId,
		string projectionId,
		string slotId,
		string entityId,
		int nowSecond,
		string reason)
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		if (!batches || !manifest || resultId.IsEmpty() || projectionId.IsEmpty() || slotId.IsEmpty() || entityId.IsEmpty())
		{
			result.m_sFailureReason = "force casualty callback identity is incomplete";
			return result;
		}
		if (CountByResult(batches, resultId) != 1)
		{
			result.m_sFailureReason = "force casualty callback result identity is missing or ambiguous";
			return result;
		}

		HST_ForceSpawnResultState batch = FindByResult(batches, resultId);
		result.m_Batch = batch;
		if (!BatchHasUniqueKeys(batches, batch) || batch.m_sProjectionId != projectionId)
		{
			result.m_sFailureReason = "force casualty callback projection identity conflicts with the queue";
			return result;
		}
		HST_ForceSpawnQueueManifestValidation validation = ValidateManifest(manifest);
		HST_ForceSpawnQueueManifestSlot descriptor = FindManifestSlot(validation.m_aSlots, slotId, SLOT_KIND_MEMBER);
		if (!validation.m_bValid || !descriptor || !ValidateBatchManifest(batch, manifest).IsEmpty())
		{
			result.m_sFailureReason = "force casualty callback manifest integrity conflict";
			return result;
		}
		HST_ForceSpawnSlotResultState slotResult = batch.FindSlotResult(slotId);
		if (!slotResult || slotResult.m_sSlotKind != SLOT_KIND_MEMBER)
		{
			result.m_sFailureReason = "force casualty callback does not resolve to an exact member slot";
			return result;
		}
		if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
			&& slotResult.m_bCasualtyConfirmed)
		{
			if (slotResult.m_sEntityId != entityId)
			{
				result.m_sFailureReason = "force casualty replay conflicts with the retired slot entity";
				return result;
			}
			result.m_bAccepted = true;
			result.m_bAlreadyApplied = true;
			return result;
		}
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_iSuccessfulHandoffCount <= 0)
		{
			result.m_sFailureReason = "force casualty callback requires a successfully handed-off projection";
			return result;
		}
		if (slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
			|| slotResult.m_sEntityId != entityId || !slotResult.m_bEverAlive)
		{
			result.m_sFailureReason = "force casualty callback conflicts with registered living-slot evidence";
			return result;
		}

		if (reason.IsEmpty())
			reason = "authoritative life state confirmed member death";
		slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
		slotResult.m_bAliveVerified = false;
		slotResult.m_bCasualtyConfirmed = true;
		slotResult.m_iCasualtyAtSecond = Math.Max(0, nowSecond);
		slotResult.m_iUpdatedAtSecond = Math.Max(0, nowSecond);
		slotResult.m_iLifecycleRevision++;
		slotResult.m_sRetirementReason = reason;
		batch.m_iLifecycleRevision++;
		batch.m_iLastLifecycleSecond = Math.Max(0, nowSecond);
		batch.m_iUpdatedAtSecond = Math.Max(batch.m_iUpdatedAtSecond, nowSecond);
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		return result;
	}

	HST_ForceSpawnQueueCallbackResult ConfirmQuarantinedRegisteredMemberCasualty(
		array<ref HST_ForceSpawnResultState> batches,
		string resultId,
		string projectionId,
		string slotId,
		string entityId,
		int nowSecond,
		string reason)
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		if (!batches || resultId.IsEmpty() || projectionId.IsEmpty()
			|| slotId.IsEmpty() || entityId.IsEmpty())
		{
			result.m_sFailureReason = "quarantined casualty identity is incomplete";
			return result;
		}
		int resultMatches;
		int projectionMatches;
		HST_ForceSpawnResultState batch;
		foreach (HST_ForceSpawnResultState candidate : batches)
		{
			if (!candidate)
				continue;
			if (candidate.m_sResultId == resultId)
			{
				resultMatches++;
				batch = candidate;
			}
			if (candidate.m_sProjectionId == projectionId)
				projectionMatches++;
		}
		result.m_Batch = batch;
		if (resultMatches != 1 || projectionMatches != 1 || !batch
			|| batch.m_sProjectionId != projectionId)
		{
			result.m_sFailureReason = "quarantined casualty runtime keys are missing or ambiguous";
			return result;
		}
		int slotMatches;
		HST_ForceSpawnSlotResultState slotResult;
		foreach (HST_ForceSpawnSlotResultState candidateSlot : batch.m_aSlotResults)
		{
			if (!candidateSlot || candidateSlot.m_sSlotId != slotId)
				continue;
			slotMatches++;
			slotResult = candidateSlot;
		}
		if (slotMatches != 1 || !slotResult || slotResult.m_sSlotKind != SLOT_KIND_MEMBER)
		{
			result.m_sFailureReason = "quarantined casualty does not resolve to one exact member slot";
			return result;
		}
		if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
			&& slotResult.m_bCasualtyConfirmed)
		{
			if (slotResult.m_sEntityId != entityId)
			{
				result.m_sFailureReason = "quarantined casualty replay conflicts with the retired slot entity";
				return result;
			}
			result.m_bAccepted = true;
			result.m_bAlreadyApplied = true;
			return result;
		}
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_iSuccessfulHandoffCount <= 0
			|| slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
			|| slotResult.m_sEntityId != entityId || !slotResult.m_bEverAlive)
		{
			result.m_sFailureReason = "quarantined casualty conflicts with handed-off member evidence";
			return result;
		}
		if (reason.IsEmpty())
			reason = "authoritative life state confirmed member death during typed quarantine";
		slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
		slotResult.m_bAliveVerified = false;
		slotResult.m_bCasualtyConfirmed = true;
		slotResult.m_iCasualtyAtSecond = Math.Max(0, nowSecond);
		slotResult.m_iUpdatedAtSecond = Math.Max(0, nowSecond);
		slotResult.m_iLifecycleRevision++;
		slotResult.m_sRetirementReason = reason;
		batch.m_iLifecycleRevision++;
		batch.m_iLastLifecycleSecond = Math.Max(0, nowSecond);
		batch.m_iUpdatedAtSecond = Math.Max(batch.m_iUpdatedAtSecond, nowSecond);
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		return result;
	}

	HST_ForceSpawnQueueCallbackResult CompleteQuarantinedSuccessfulProjectionCancellation(
		array<ref HST_ForceSpawnResultState> batches,
		string resultId,
		string projectionId,
		int nowSecond,
		string reason)
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		if (!batches || resultId.IsEmpty() || projectionId.IsEmpty())
		{
			result.m_sFailureReason = "quarantined successful-projection cancellation identity is incomplete";
			return result;
		}
		int resultMatches;
		int projectionMatches;
		HST_ForceSpawnResultState batch;
		foreach (HST_ForceSpawnResultState candidate : batches)
		{
			if (!candidate)
				continue;
			if (candidate.m_sResultId == resultId)
			{
				resultMatches++;
				batch = candidate;
			}
			if (candidate.m_sProjectionId == projectionId)
				projectionMatches++;
		}
		result.m_Batch = batch;
		if (resultMatches != 1 || projectionMatches != 1 || !batch
			|| batch.m_sProjectionId != projectionId)
		{
			result.m_sFailureReason = "quarantined successful projection is missing or runtime-key ambiguous";
			return result;
		}
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
		{
			result.m_bAccepted = true;
			result.m_bAlreadyApplied = true;
			return result;
		}
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_iSuccessfulHandoffCount <= 0)
		{
			result.m_sFailureReason = "quarantine terminalization requires one retired successful projection";
			return result;
		}
		int rootSlots;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot)
			{
				result.m_sFailureReason = "quarantined successful projection contains an empty slot row";
				return result;
			}
			if (slot.m_sSlotKind == SLOT_KIND_GROUP)
				rootSlots++;
			else if (slot.m_sSlotKind != SLOT_KIND_MEMBER)
			{
				result.m_sFailureReason = "quarantined successful projection contains a foreign slot kind";
				return result;
			}
			bool registered = slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
			bool casualty = slot.m_sSlotKind == SLOT_KIND_MEMBER
				&& slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				&& slot.m_bEverAlive && slot.m_bCasualtyConfirmed;
			if (!registered && !casualty)
			{
				result.m_sFailureReason = "quarantined successful projection slot lacks registered or casualty authority " + slot.m_sSlotId;
				return result;
			}
		}
		if (rootSlots != 1)
		{
			result.m_sFailureReason = "quarantined successful projection does not contain one exact group root";
			return result;
		}

		if (reason.IsEmpty())
			reason = "exact successful projection retired into typed quarantine";
		foreach (HST_ForceSpawnSlotResultState retiredSlot : batch.m_aSlotResults)
		{
			bool preserveCasualty = retiredSlot.m_sSlotKind == SLOT_KIND_MEMBER
				&& retiredSlot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				&& retiredSlot.m_bCasualtyConfirmed;
			ClearSlotPhysicalEvidence(retiredSlot, nowSecond);
			retiredSlot.m_sProjectionId = projectionId;
			if (!preserveCasualty)
			{
				retiredSlot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CANCELLED;
				retiredSlot.m_sFailureReason = reason;
				retiredSlot.m_sRetirementReason = reason;
			}
			retiredSlot.m_iLifecycleRevision++;
		}
		batch.m_sNativeGroupId = "";
		MarkBatchTerminal(
			batch,
			HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED,
			reason,
			Math.Max(0, nowSecond));
		batch.m_iLifecycleRevision++;
		batch.m_iLastLifecycleSecond = Math.Max(0, nowSecond);
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		return result;
	}

	HST_ForceSpawnQueueCallbackResult RequeueSuccessfulProjectionAfterRestore(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		string resultId,
		string projectionId,
		int nowSecond,
		int deadlineSecond)
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		HST_ForceSpawnQueueManifestValidation validation = ValidateSuccessfulProjectionRequeue(
			batches,
			manifest,
			resultId,
			projectionId,
			nowSecond,
			deadlineSecond,
			result);
		if (!validation)
			return result;
		HST_ForceSpawnResultState batch = result.m_Batch;

		foreach (HST_ForceSpawnQueueManifestSlot descriptor : validation.m_aSlots)
		{
			if (IsExternallyManagedAssetDescriptor(batch, descriptor))
				continue;
			HST_ForceSpawnSlotResultState slotResult = batch.FindSlotResult(descriptor.m_sSlotId);
			if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				|| slotResult.m_bCasualtyConfirmed)
			{
				ClearSlotPhysicalEvidence(slotResult, nowSecond);
				slotResult.m_sProjectionId = batch.m_sProjectionId;
				slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
				continue;
			}
			ResetSlotForAttempt(slotResult, batch.m_sProjectionId, nowSecond);
		}

		batch.m_sNativeGroupId = "";
		batch.m_sTerminalReason = "successful exact projection queued for survivor reprojection after restore";
		batch.m_sLastFailureReason = "";
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING;
		batch.m_iRetryCount = 0;
		batch.m_iDeadlineSecond = deadlineSecond;
		batch.m_iLastAttemptSecond = 0;
		batch.m_iNextAttemptSecond = nowSecond;
		batch.m_iUpdatedAtSecond = nowSecond;
		batch.m_iCompletedAtSecond = 0;
		batch.m_iReprojectionCount++;
		batch.m_iLifecycleRevision++;
		batch.m_iLastLifecycleSecond = nowSecond;
		batch.m_bCancelRequested = false;
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		return result;
	}

	HST_ForceSpawnQueueCallbackResult HoldPendingProjectionForStrategicSimulation(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		string resultId,
		string projectionId,
		int nowSecond)
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		HST_ForceSpawnResultState batch = ResolveStrategicProjectionBatch(batches, manifest, resultId, projectionId, result);
		if (!batch)
			return result;
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_DEFERRED
			&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_RETRYABLE)
		{
			result.m_sFailureReason = "only an idle projection can enter strategic hold";
			return result;
		}
		if (batch.m_bStrategicProjectionHeld)
		{
			result.m_bAccepted = true;
			result.m_bAlreadyApplied = true;
			return result;
		}
		batch.m_bStrategicProjectionHeld = true;
		batch.m_iStrategicHoldSinceSecond = Math.Max(0, nowSecond);
		batch.m_iNextAttemptSecond = 0;
		batch.m_iUpdatedAtSecond = Math.Max(batch.m_iUpdatedAtSecond, nowSecond);
		batch.m_iLifecycleRevision++;
		batch.m_iLastLifecycleSecond = Math.Max(0, nowSecond);
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		return result;
	}

	HST_ForceSpawnQueueCallbackResult ReleaseStrategicProjectionForMaterialization(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		string resultId,
		string projectionId,
		int nowSecond,
		int deadlineSecond)
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		HST_ForceSpawnResultState batch = ResolveStrategicProjectionBatch(batches, manifest, resultId, projectionId, result);
		if (!batch)
			return result;
		if (!batch.m_bStrategicProjectionHeld)
		{
			result.m_bAccepted = true;
			result.m_bAlreadyApplied = true;
			return result;
		}
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_DEFERRED
			&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_RETRYABLE)
		{
			result.m_sFailureReason = "strategically held projection is not idle for materialization";
			return result;
		}
		if (deadlineSecond <= nowSecond)
		{
			result.m_sFailureReason = "strategic projection materialization deadline must be in the future";
			return result;
		}
		batch.m_bStrategicProjectionHeld = false;
		batch.m_iStrategicHoldSinceSecond = 0;
		batch.m_iDeadlineSecond = deadlineSecond;
		batch.m_iNextAttemptSecond = nowSecond;
		batch.m_iUpdatedAtSecond = nowSecond;
		batch.m_iLifecycleRevision++;
		batch.m_iLastLifecycleSecond = nowSecond;
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		return result;
	}

	HST_ForceSpawnQueueCallbackResult RequeueSuccessfulProjectionForStrategicHold(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		string resultId,
		string projectionId,
		int nowSecond,
		int deadlineSecond)
	{
		HST_ForceSpawnQueueCallbackResult result = RequeueSuccessfulProjectionAfterRestore(
			batches,
			manifest,
			resultId,
			projectionId,
			nowSecond,
			deadlineSecond);
		if (!result || !result.m_bAccepted || !result.m_Batch)
			return result;
		result.m_Batch.m_bStrategicProjectionHeld = true;
		result.m_Batch.m_iStrategicHoldSinceSecond = Math.Max(0, nowSecond);
		result.m_Batch.m_iNextAttemptSecond = 0;
		return result;
	}

	HST_ForceSpawnQueueCallbackResult CanRequeueSuccessfulProjectionForStrategicHold(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		string resultId,
		string projectionId,
		int nowSecond,
		int deadlineSecond)
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		HST_ForceSpawnQueueManifestValidation validation = ValidateSuccessfulProjectionRequeue(
			batches,
			manifest,
			resultId,
			projectionId,
			nowSecond,
			deadlineSecond,
			result);
		if (!validation)
			return result;
		result.m_bAccepted = true;
		return result;
	}

	HST_ForceSpawnQueueCallbackResult ConfirmStrategicMemberCasualty(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		string resultId,
		string projectionId,
		string slotId,
		int nowSecond,
		string reason)
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		HST_ForceSpawnResultState batch = ResolveStrategicProjectionBatch(batches, manifest, resultId, projectionId, result);
		if (!batch)
			return result;
		if (!batch.m_bStrategicProjectionHeld)
		{
			result.m_sFailureReason = "strategic casualty requires virtual projection authority";
			return result;
		}
		HST_ForceSpawnQueueManifestValidation validation = ValidateManifest(manifest);
		HST_ForceSpawnQueueManifestSlot descriptor = FindManifestSlot(validation.m_aSlots, slotId, SLOT_KIND_MEMBER);
		HST_ForceSpawnSlotResultState slotResult = batch.FindSlotResult(slotId);
		if (!descriptor || !slotResult || slotResult.m_sSlotKind != SLOT_KIND_MEMBER)
		{
			result.m_sFailureReason = "strategic casualty does not resolve to an exact member slot";
			return result;
		}
		if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED && slotResult.m_bCasualtyConfirmed)
		{
			result.m_bAccepted = true;
			result.m_bAlreadyApplied = true;
			return result;
		}
		if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
			|| slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING
			|| slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING)
		{
			result.m_sFailureReason = "strategic casualty conflicts with physical slot evidence";
			return result;
		}
		if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_FINAL
			|| slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CANCELLED)
		{
			result.m_sFailureReason = "strategic casualty slot is not part of the living roster";
			return result;
		}
		if (reason.IsEmpty())
			reason = "deterministic virtual combat casualty";
		ClearSlotPhysicalEvidence(slotResult, nowSecond);
		slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
		slotResult.m_bEverAlive = true;
		slotResult.m_bCasualtyConfirmed = true;
		slotResult.m_iCasualtyAtSecond = Math.Max(0, nowSecond);
		slotResult.m_iUpdatedAtSecond = Math.Max(0, nowSecond);
		slotResult.m_iLifecycleRevision++;
		slotResult.m_sRetirementReason = reason;
		batch.m_iLifecycleRevision++;
		batch.m_iLastLifecycleSecond = Math.Max(0, nowSecond);
		batch.m_iUpdatedAtSecond = Math.Max(batch.m_iUpdatedAtSecond, nowSecond);
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		return result;
	}

	HST_ForceSpawnQueueCallbackResult CompleteStrategicProjectionElimination(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		string resultId,
		string projectionId,
		int nowSecond,
		string reason)
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		HST_ForceSpawnResultState batch = ResolveStrategicProjectionBatch(batches, manifest, resultId, projectionId, result);
		if (!batch)
			return result;
		if (!batch.m_bStrategicProjectionHeld || CountStrategicLivingMemberSlots(batch) > 0)
		{
			result.m_sFailureReason = "strategic elimination requires a held projection with zero living member slots";
			return result;
		}
		if (reason.IsEmpty())
			reason = "exact strategic roster eliminated";
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (!slotResult || (slotResult.m_sSlotKind == SLOT_KIND_MEMBER && slotResult.m_bCasualtyConfirmed))
				continue;
			ClearSlotPhysicalEvidence(slotResult, nowSecond);
			slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_FINAL;
			slotResult.m_sFailureReason = reason;
			slotResult.m_iLifecycleRevision++;
		}
		MarkBatchTerminal(batch, HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL, reason, nowSecond);
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		return result;
	}

	int CountStrategicLivingMemberSlots(HST_ForceSpawnResultState batch)
	{
		int count;
		if (!batch)
			return count;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (!slotResult || slotResult.m_sSlotKind != SLOT_KIND_MEMBER || slotResult.m_bCasualtyConfirmed)
				continue;
			if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				|| slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_FINAL
				|| slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CANCELLED)
				continue;
			count++;
		}
		return count;
	}

	string SelectStrategicLivingMemberSlotId(HST_ForceSpawnResultState batch, int deterministicIndex)
	{
		int livingCount = CountStrategicLivingMemberSlots(batch);
		if (livingCount <= 0)
			return "";
		int normalizedIndex = deterministicIndex;
		if (normalizedIndex < 0)
			normalizedIndex = -normalizedIndex;
		int selectedIndex = normalizedIndex % livingCount;
		int currentIndex;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (!slotResult || slotResult.m_sSlotKind != SLOT_KIND_MEMBER || slotResult.m_bCasualtyConfirmed)
				continue;
			if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				|| slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_FINAL
				|| slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CANCELLED)
				continue;
			if (currentIndex == selectedIndex)
				return slotResult.m_sSlotId;
			currentIndex++;
		}
		return "";
	}

	protected HST_ForceSpawnQueueManifestValidation ValidateSuccessfulProjectionRequeue(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		string resultId,
		string projectionId,
		int nowSecond,
		int deadlineSecond,
		HST_ForceSpawnQueueCallbackResult result)
	{
		if (!result)
			return null;
		if (!batches || !manifest || resultId.IsEmpty() || projectionId.IsEmpty())
		{
			result.m_sFailureReason = "force reprojection identity or manifest is missing";
			return null;
		}
		if (CountByResult(batches, resultId) != 1)
		{
			result.m_sFailureReason = "force reprojection result identity is missing or ambiguous";
			return null;
		}
		HST_ForceSpawnResultState batch = FindByResult(batches, resultId);
		result.m_Batch = batch;
		if (!BatchHasUniqueKeys(batches, batch) || batch.m_sProjectionId != projectionId)
		{
			result.m_sFailureReason = "force reprojection identity conflicts with the queue";
			return null;
		}
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_iSuccessfulHandoffCount <= 0)
		{
			result.m_sFailureReason = "force reprojection requires a previously successful projection";
			return null;
		}
		HST_ForceSpawnQueueManifestValidation validation = ValidateManifest(manifest);
		if (!validation.m_bValid || !ValidateBatchManifest(batch, manifest).IsEmpty())
		{
			result.m_sFailureReason = "force reprojection manifest integrity conflict";
			return null;
		}
		if (deadlineSecond <= nowSecond)
		{
			result.m_sFailureReason = "force reprojection deadline must be in the future";
			return null;
		}
		if (CountNonterminalBatches(batches) >= MAX_NONTERMINAL_BATCHES
			|| CountNonterminalSlots(batches) + batch.m_aSlotResults.Count() > MAX_TOTAL_SLOT_ROWS)
		{
			result.m_sFailureReason = "force reprojection would exceed nonterminal queue capacity";
			return null;
		}
		if (CountDurableLivingMemberSlots(batch) <= 0)
		{
			result.m_sFailureReason = "force reprojection has no durable living member slots";
			return null;
		}
		foreach (HST_ForceSpawnQueueManifestSlot descriptor : validation.m_aSlots)
		{
			if (IsExternallyManagedAssetDescriptor(batch, descriptor))
				continue;
			HST_ForceSpawnSlotResultState slotResult = batch.FindSlotResult(descriptor.m_sSlotId);
			if (slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				&& !slotResult.m_bCasualtyConfirmed)
				continue;
			if (descriptor.m_sSlotKind != SLOT_KIND_MEMBER || !slotResult.m_bEverAlive
				|| !slotResult.m_bCasualtyConfirmed
				|| slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED)
			{
				result.m_sFailureReason = "force reprojection contains invalid retired-slot evidence";
				return null;
			}
		}
		return validation;
	}

	protected HST_ForceSpawnResultState ResolveStrategicProjectionBatch(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		string resultId,
		string projectionId,
		HST_ForceSpawnQueueCallbackResult result)
	{
		if (!batches || !manifest || resultId.IsEmpty() || projectionId.IsEmpty() || CountByResult(batches, resultId) != 1)
		{
			result.m_sFailureReason = "strategic projection identity is missing or ambiguous";
			return null;
		}
		HST_ForceSpawnResultState batch = FindByResult(batches, resultId);
		result.m_Batch = batch;
		if (!BatchHasUniqueKeys(batches, batch) || batch.m_sProjectionId != projectionId
			|| !ValidateBatchManifest(batch, manifest).IsEmpty())
		{
			result.m_sFailureReason = "strategic projection identity or manifest conflicts with the queue";
			return null;
		}
		return batch;
	}

	int CountDurableLivingMemberSlots(HST_ForceSpawnResultState batch)
	{
		int count;
		if (!batch)
			return count;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (!slotResult || slotResult.m_sSlotKind != SLOT_KIND_MEMBER)
				continue;
			if (slotResult.m_bEverAlive && !slotResult.m_bCasualtyConfirmed)
				count++;
		}
		return count;
	}

	int CountConfirmedCasualtyMemberSlots(HST_ForceSpawnResultState batch)
	{
		int count;
		if (!batch)
			return count;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (slotResult && slotResult.m_sSlotKind == SLOT_KIND_MEMBER
				&& slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				&& slotResult.m_bEverAlive && slotResult.m_bCasualtyConfirmed)
				count++;
		}
		return count;
	}

	protected HST_ForceSpawnQueueCallbackResult ResolveRegisteredSuccessReplay(
		HST_ForceSpawnQueueCallbackResult result,
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceSpawnResultState batch,
		HST_ForceSpawnSlotResultState slotResult,
		HST_ForceSpawnQueueSlotSuccess success)
	{
		if (!SlotSuccessMatches(slotResult, success))
		{
			result.m_sFailureReason = "registered spawn slot callback conflicts with durable result";
			result.m_bCleanupRequired = ShouldCleanupReturnedEntity(batches, batch, success.m_sEntityId, success.m_sNativeGroupId);
			return result;
		}
		result.m_bAccepted = true;
		result.m_bAlreadyApplied = true;
		result.m_Batch = batch;
		return result;
	}

	protected bool CanAcceptSpawnSuccess(HST_ForceSpawnResultState batch, HST_ForceSpawnSlotResultState slotResult)
	{
		if (!batch || !slotResult)
			return false;
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS)
			return slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING;
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
			return false;
		return slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING
			|| slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING;
	}

	protected bool IsScopedRetryCleanup(HST_ForceSpawnResultState batch)
	{
		if (!batch || batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
			return false;
		string disposition = CleanupDispositionPrefix(batch.m_sTerminalReason);
		return disposition == CLEANUP_DISPOSITION_RETRY || disposition == CLEANUP_DISPOSITION_DEFER;
	}

	protected string ValidateSuccessEvidence(
		HST_ForceSpawnResultState batch,
		HST_ForceSpawnQueueManifestSlot descriptor,
		HST_ForceSpawnQueueSlotSuccess success)
	{
		if (success.m_sEntityId.IsEmpty())
			return "spawn success returned no entity id";
		if (success.m_sSpawnedPrefab != descriptor.m_sPrefab)
			return "spawned prefab does not match exact manifest slot";
		if (!success.m_bAliveVerified || !success.m_bFactionVerified || !success.m_bGroupVerified
			|| !success.m_bGameMasterVerified || !success.m_bProjectionVerified)
			return "spawned entity failed alive, faction, group, Game Master, or projection verification";
		string expectedNativeGroupId = ResolveExpectedNativeGroup(batch, descriptor);
		if (descriptor.m_sSlotKind == SLOT_KIND_GROUP && success.m_sNativeGroupId.IsEmpty())
			return "spawned group root returned no native group id";
		if (descriptor.m_sSlotKind != SLOT_KIND_GROUP && (expectedNativeGroupId.IsEmpty() || success.m_sNativeGroupId != expectedNativeGroupId))
			return "spawned entity native group does not match manifest dependency";
		string expectedVehicleEntityId = ResolveAssignedVehicleEntity(batch, descriptor.m_sAssignedVehicleSlotId);
		if (!descriptor.m_sAssignedVehicleSlotId.IsEmpty())
		{
			if (expectedVehicleEntityId.IsEmpty() || success.m_sAssignedVehicleEntityId != expectedVehicleEntityId || !success.m_bSeatVerified)
				return "spawned entity vehicle assignment or seat verification failed";
		}
		else if (!success.m_sAssignedVehicleEntityId.IsEmpty())
		{
			return "spawned entity reported an unexpected vehicle assignment";
		}
		return "";
	}

	protected string ResolveExpectedNativeGroup(HST_ForceSpawnResultState batch, HST_ForceSpawnQueueManifestSlot descriptor)
	{
		if (!batch || !descriptor)
			return "";
		if (descriptor.m_sSlotKind == SLOT_KIND_GROUP)
			return descriptor.m_sSlotId;
		if (!descriptor.m_sGroupElementId.IsEmpty())
		{
			HST_ForceSpawnSlotResultState groupResult = batch.FindSlotResult(descriptor.m_sGroupElementId);
			if (IsRegisteredSlot(groupResult))
				return groupResult.m_sNativeGroupId;
		}
		if (!descriptor.m_sAssignedVehicleSlotId.IsEmpty())
		{
			HST_ForceSpawnSlotResultState vehicleResult = batch.FindSlotResult(descriptor.m_sAssignedVehicleSlotId);
			if (IsRegisteredSlot(vehicleResult))
				return vehicleResult.m_sNativeGroupId;
		}
		return batch.m_sNativeGroupId;
	}

	protected void ApplySuccessEvidence(
		HST_ForceSpawnSlotResultState slotResult,
		HST_ForceSpawnQueueSlotSuccess success,
		string projectionId,
		int nowSecond)
	{
		slotResult.m_sSpawnedPrefab = success.m_sSpawnedPrefab;
		slotResult.m_sEntityId = success.m_sEntityId;
		slotResult.m_sAssignedVehicleEntityId = success.m_sAssignedVehicleEntityId;
		slotResult.m_sNativeGroupId = success.m_sNativeGroupId;
		slotResult.m_sProjectionId = projectionId;
		slotResult.m_sFailureReason = "";
		slotResult.m_iUpdatedAtSecond = nowSecond;
		slotResult.m_iLifecycleRevision++;
		slotResult.m_bAliveVerified = success.m_bAliveVerified;
		slotResult.m_bEverAlive = slotResult.m_bEverAlive || success.m_bAliveVerified;
		slotResult.m_bCasualtyConfirmed = false;
		slotResult.m_iCasualtyAtSecond = 0;
		slotResult.m_sRetirementReason = "";
		slotResult.m_bFactionVerified = success.m_bFactionVerified;
		slotResult.m_bGroupVerified = success.m_bGroupVerified;
		slotResult.m_bGameMasterVerified = success.m_bGameMasterVerified;
		slotResult.m_bProjectionVerified = success.m_bProjectionVerified;
		slotResult.m_bSeatVerified = success.m_bSeatVerified;
	}

	protected bool SlotSuccessMatches(HST_ForceSpawnSlotResultState slotResult, HST_ForceSpawnQueueSlotSuccess success)
	{
		if (!slotResult || !success)
			return false;
		if (slotResult.m_sEntityId != success.m_sEntityId || slotResult.m_sSpawnedPrefab != success.m_sSpawnedPrefab)
			return false;
		if (slotResult.m_sAssignedVehicleEntityId != success.m_sAssignedVehicleEntityId || slotResult.m_sNativeGroupId != success.m_sNativeGroupId)
			return false;
		if (slotResult.m_bAliveVerified != success.m_bAliveVerified || slotResult.m_bFactionVerified != success.m_bFactionVerified)
			return false;
		if (slotResult.m_bGroupVerified != success.m_bGroupVerified || slotResult.m_bGameMasterVerified != success.m_bGameMasterVerified
			|| slotResult.m_bProjectionVerified != success.m_bProjectionVerified)
			return false;
		return slotResult.m_bSeatVerified == success.m_bSeatVerified;
	}

	protected bool EntityRegisteredToAnotherSlot(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceSpawnResultState currentBatch,
		string slotId,
		string entityId)
	{
		if (!batches || !currentBatch || entityId.IsEmpty())
			return false;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (!batch)
				continue;
			foreach (HST_ForceSpawnSlotResultState other : batch.m_aSlotResults)
			{
				if (!other || (batch == currentBatch && other.m_sSlotId == slotId))
					continue;
				if (other.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED && other.m_sEntityId == entityId)
					return true;
			}
		}
		return false;
	}

	protected bool ShouldCleanupReturnedEntity(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceSpawnResultState currentBatch,
		string entityId,
		string nativeGroupId)
	{
		if (!batches || !currentBatch || (entityId.IsEmpty() && nativeGroupId.IsEmpty()))
			return false;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (!batch)
				continue;
			foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
			{
				if (!slotResult || slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED)
					continue;
				if (!entityId.IsEmpty() && slotResult.m_sEntityId == entityId)
					return false;
				if (entityId.IsEmpty() && !nativeGroupId.IsEmpty() && slotResult.m_sNativeGroupId == nativeGroupId)
					return false;
			}
		}
		return true;
	}

	protected bool GroupRootNativeIdRegisteredElsewhere(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceSpawnResultState currentBatch,
		string currentSlotId,
		string nativeGroupId)
	{
		if (!batches || !currentBatch || currentSlotId.IsEmpty() || nativeGroupId.IsEmpty())
			return false;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (!batch)
				continue;
			foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
			{
				if (!slotResult || slotResult.m_sSlotKind != SLOT_KIND_GROUP)
					continue;
				if (batch == currentBatch && slotResult.m_sSlotId == currentSlotId)
					continue;
				if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
					&& slotResult.m_sNativeGroupId == nativeGroupId)
					return true;
			}
		}
		return false;
	}

	protected HST_ForceSpawnQueueCallbackResult MarkStaleCallback(
		HST_ForceSpawnQueueCallbackResult result,
		string reason,
		bool cleanupRequired)
	{
		result.m_bStale = true;
		result.m_bCleanupRequired = cleanupRequired;
		result.m_sFailureReason = reason;
		return result;
	}

	HST_ForceSpawnQueueCallbackResult FailSlot(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueSlotFailure failure,
		int nowSecond)
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		if (!batches || !manifest || !failure || failure.m_sResultId.IsEmpty() || failure.m_sSlotId.IsEmpty())
		{
			result.m_sFailureReason = "spawn failure callback incomplete";
			return result;
		}
		if (CountByResult(batches, failure.m_sResultId) != 1)
		{
			result.m_sFailureReason = "spawn failure callback result identity is missing or ambiguous";
			return result;
		}
		HST_ForceSpawnResultState batch = FindByResult(batches, failure.m_sResultId);
		result.m_Batch = batch;
		if (!BatchHasUniqueKeys(batches, batch))
		{
			result.m_sFailureReason = "spawn failure callback durable identity is ambiguous";
			result.m_bCleanupRequired = ShouldCleanupReturnedEntity(batches, batch, failure.m_sEntityId, failure.m_sNativeGroupId);
			return result;
		}
		if (failure.m_sProjectionId != batch.m_sProjectionId)
		{
			result.m_sFailureReason = "spawn failure callback projection identity conflict";
			result.m_bCleanupRequired = ShouldCleanupReturnedEntity(batches, batch, failure.m_sEntityId, failure.m_sNativeGroupId);
			return result;
		}
		if (failure.m_iAttemptGeneration != batch.m_iAttemptGeneration)
			return MarkStaleCallback(result, "spawn failure callback generation is stale", ShouldCleanupReturnedEntity(batches, batch, failure.m_sEntityId, failure.m_sNativeGroupId));
		bool scopedSiblingSpawn = batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING
			&& IsScopedRetryCleanup(batch);
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS && !scopedSiblingSpawn)
			return MarkStaleCallback(result, "spawn failure callback is no longer expected", ShouldCleanupReturnedEntity(batches, batch, failure.m_sEntityId, failure.m_sNativeGroupId));
		HST_ForceSpawnSlotResultState slotResult = batch.FindSlotResult(failure.m_sSlotId);
		if (!slotResult || slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING)
			return MarkStaleCallback(result, "spawn failure callback slot is no longer in flight", ShouldCleanupReturnedEntity(batches, batch, failure.m_sEntityId, failure.m_sNativeGroupId));
		if (scopedSiblingSpawn && !slotResult.m_sFailureReason.IsEmpty())
			return MarkStaleCallback(result, "spawn failure callback collided with cleanup work", ShouldCleanupReturnedEntity(batches, batch, failure.m_sEntityId, failure.m_sNativeGroupId));

		HST_ForceSpawnQueueManifestValidation validation = ValidateManifest(manifest);
		HST_ForceSpawnQueueManifestSlot descriptor = FindManifestSlot(validation.m_aSlots, failure.m_sSlotId, "");
		if (!validation.m_bValid || !descriptor || !ValidateBatchManifest(batch, manifest).IsEmpty())
		{
			result.m_sFailureReason = "spawn failure callback manifest integrity conflict";
			result.m_bCleanupRequired = ShouldCleanupReturnedEntity(batches, batch, failure.m_sEntityId, failure.m_sNativeGroupId);
			return result;
		}
		bool groupOwnershipConflict;
		if (descriptor.m_sSlotKind == SLOT_KIND_GROUP)
			groupOwnershipConflict = GroupRootNativeIdRegisteredElsewhere(batches, batch, failure.m_sSlotId, failure.m_sNativeGroupId);
		if (EntityRegisteredToAnotherSlot(batches, batch, failure.m_sSlotId, failure.m_sEntityId) || groupOwnershipConflict)
		{
			result.m_sFailureReason = "spawn failure callback claims registered entity or native group authority";
			result.m_bCleanupRequired = ShouldCleanupReturnedEntity(batches, batch, failure.m_sEntityId, failure.m_sNativeGroupId);
			return result;
		}

		ApplyFailureEvidence(slotResult, failure, batch.m_sProjectionId, nowSecond);
		string reason = failure.m_sFailureReason;
		if (reason.IsEmpty())
			reason = "spawn executor reported failure";
		batch.m_sLastFailureReason = reason;
		batch.m_iUpdatedAtSecond = nowSecond;
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		if (!failure.m_bRetryable || batch.m_iRetryCount >= batch.m_iMaxRetries)
		{
			BeginCleanup(batch, CLEANUP_DISPOSITION_FINAL, reason, nowSecond);
			result.m_bCleanupRequired = true;
			return result;
		}

		int retryAtSecond = ResolveRetryAt(batch, failure.m_iRetryAtSecond, nowSecond);
		if (batch.m_iDeadlineSecond > 0 && retryAtSecond >= batch.m_iDeadlineSecond)
		{
			BeginCleanup(batch, CLEANUP_DISPOSITION_FINAL, "retry would exceed spawn deadline: " + reason, nowSecond);
			result.m_bCleanupRequired = true;
			return result;
		}
		batch.m_iNextAttemptSecond = Math.Max(batch.m_iNextAttemptSecond, retryAtSecond);
		if (HasCleanupOwnedPartialEntity(failure, descriptor))
		{
			BeginScopedRetryCleanup(batch, slotResult, reason, nowSecond);
			result.m_bCleanupRequired = true;
			return result;
		}
		slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_RETRYABLE;
		slotResult.m_sFailureReason = reason;
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
			AdvanceCleanupIfReady(batch, nowSecond);
		else
			AdvanceInProgressState(batch, nowSecond);
		return result;
	}

	protected void ApplyFailureEvidence(
		HST_ForceSpawnSlotResultState slotResult,
		HST_ForceSpawnQueueSlotFailure failure,
		string projectionId,
		int nowSecond)
	{
		slotResult.m_sSpawnedPrefab = failure.m_sSpawnedPrefab;
		slotResult.m_sEntityId = failure.m_sEntityId;
		slotResult.m_sAssignedVehicleEntityId = failure.m_sAssignedVehicleEntityId;
		slotResult.m_sNativeGroupId = failure.m_sNativeGroupId;
		slotResult.m_sProjectionId = projectionId;
		slotResult.m_sFailureReason = failure.m_sFailureReason;
		slotResult.m_iUpdatedAtSecond = nowSecond;
	}

	protected bool HasCleanupOwnedPartialEntity(
		HST_ForceSpawnQueueSlotFailure failure,
		HST_ForceSpawnQueueManifestSlot descriptor)
	{
		if (!failure || !descriptor)
			return false;
		if (!failure.m_sEntityId.IsEmpty())
			return true;
		return descriptor.m_sSlotKind == SLOT_KIND_GROUP && !failure.m_sNativeGroupId.IsEmpty();
	}

	HST_ForceSpawnQueueCallbackResult DeferSlot(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		string resultId,
		string projectionId,
		string slotId,
		int attemptGeneration,
		int nowSecond,
		int retryAtSecond,
		string reason)
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		if (!batches || !manifest || resultId.IsEmpty() || slotId.IsEmpty() || CountByResult(batches, resultId) != 1)
		{
			result.m_sFailureReason = "spawn defer callback identity is missing or ambiguous";
			return result;
		}
		HST_ForceSpawnResultState batch = FindByResult(batches, resultId);
		result.m_Batch = batch;
		if (!BatchHasUniqueKeys(batches, batch))
		{
			result.m_sFailureReason = "spawn defer callback durable identity is ambiguous";
			return result;
		}
		if (projectionId != batch.m_sProjectionId)
		{
			result.m_sFailureReason = "spawn defer callback projection identity conflict";
			return result;
		}
		if (attemptGeneration != batch.m_iAttemptGeneration)
			return MarkStaleCallback(result, "spawn defer callback generation is stale", false);
		bool scopedSiblingSpawn = batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING
			&& IsScopedRetryCleanup(batch);
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS && !scopedSiblingSpawn)
			return MarkStaleCallback(result, "spawn defer callback is no longer expected", false);
		HST_ForceSpawnSlotResultState slotResult = batch.FindSlotResult(slotId);
		if (!slotResult || slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING)
			return MarkStaleCallback(result, "spawn defer callback slot is no longer in flight", false);
		if (scopedSiblingSpawn && !slotResult.m_sFailureReason.IsEmpty())
			return MarkStaleCallback(result, "spawn defer callback collided with cleanup work", false);
		if (!ValidateBatchManifest(batch, manifest).IsEmpty())
		{
			result.m_sFailureReason = "spawn defer callback manifest integrity conflict";
			return result;
		}
		if (reason.IsEmpty())
			reason = "spawn executor deferred slot";
		if (retryAtSecond <= nowSecond)
			retryAtSecond = nowSecond + DEFAULT_DEFER_SECONDS;
		if (batch.m_iDeadlineSecond > 0 && retryAtSecond >= batch.m_iDeadlineSecond)
		{
			BeginCleanup(batch, CLEANUP_DISPOSITION_FINAL, "defer would exceed spawn deadline: " + reason, nowSecond);
			result.m_bAccepted = true;
			result.m_bStateChanged = true;
			result.m_bCleanupRequired = true;
			return result;
		}
		slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_DEFERRED;
		slotResult.m_sFailureReason = reason;
		slotResult.m_iUpdatedAtSecond = nowSecond;
		batch.m_sLastFailureReason = reason;
		batch.m_iNextAttemptSecond = Math.Max(batch.m_iNextAttemptSecond, retryAtSecond);
		batch.m_iUpdatedAtSecond = nowSecond;
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
			AdvanceCleanupIfReady(batch, nowSecond);
		else
			AdvanceInProgressState(batch, nowSecond);
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		return result;
	}

	HST_ForceSpawnQueueCallbackResult RequestCancel(
		array<ref HST_ForceSpawnResultState> batches,
		string resultId,
		int nowSecond,
		string reason = "")
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		if (!batches || resultId.IsEmpty() || CountByResult(batches, resultId) != 1)
		{
			result.m_sFailureReason = "spawn cancellation result identity is missing or ambiguous";
			return result;
		}
		HST_ForceSpawnResultState batch = FindByResult(batches, resultId);
		result.m_Batch = batch;
		if (!BatchHasUniqueKeys(batches, batch))
		{
			result.m_sFailureReason = "spawn cancellation durable identity is ambiguous";
			return result;
		}
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
		{
			result.m_bAccepted = true;
			result.m_bAlreadyApplied = true;
			return result;
		}
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING
			&& CleanupDispositionPrefix(batch.m_sTerminalReason) == CLEANUP_DISPOSITION_FINAL)
		{
			result.m_sFailureReason = "spawn batch is already settling a final failure";
			return result;
		}
		if (IsTerminalBatch(batch))
		{
			result.m_sFailureReason = "terminal spawn batch cannot be cancelled";
			return result;
		}
		if (IsAcceptedCancellationCleanup(batch))
		{
			result.m_bAccepted = true;
			result.m_bAlreadyApplied = true;
			result.m_bCleanupRequired = true;
			return result;
		}
		if (batch.m_iDeadlineSecond > 0 && nowSecond >= batch.m_iDeadlineSecond
			&& !batch.m_bStrategicProjectionHeld)
		{
			string deadlineReason = "spawn deadline expired before cancellation";
			BeginCleanup(batch, CLEANUP_DISPOSITION_FINAL, deadlineReason, nowSecond);
			AdvanceCleanupIfReady(batch, nowSecond);
			result.m_bStateChanged = true;
			result.m_bCleanupRequired = batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING;
			result.m_sFailureReason = deadlineReason;
			return result;
		}
		if (reason.IsEmpty())
			reason = "spawn cancellation requested";
		batch.m_bCancelRequested = true;
		BeginCleanup(batch, CLEANUP_DISPOSITION_CANCEL, reason, nowSecond);
		AdvanceCleanupIfReady(batch, nowSecond);
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		result.m_bCleanupRequired = batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING;
		return result;
	}

	HST_ForceSpawnQueueCallbackResult FailProjectionFinal(
		array<ref HST_ForceSpawnResultState> batches,
		string resultId,
		string projectionId,
		int nowSecond,
		string reason)
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		if (!batches || resultId.IsEmpty() || projectionId.IsEmpty() || CountByResult(batches, resultId) != 1)
		{
			result.m_sFailureReason = "spawn projection failure identity is missing or ambiguous";
			return result;
		}
		HST_ForceSpawnResultState batch = FindByResult(batches, resultId);
		result.m_Batch = batch;
		if (!BatchHasUniqueKeys(batches, batch) || batch.m_sProjectionId != projectionId)
		{
			result.m_sFailureReason = "spawn projection failure durable identity conflicts with the queue";
			return result;
		}
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL)
		{
			result.m_bAccepted = true;
			result.m_bAlreadyApplied = true;
			return result;
		}
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING
			&& CleanupDispositionPrefix(batch.m_sTerminalReason) == CLEANUP_DISPOSITION_FINAL)
		{
			result.m_bAccepted = true;
			result.m_bAlreadyApplied = true;
			result.m_bCleanupRequired = true;
			return result;
		}
		if (IsTerminalBatch(batch))
		{
			result.m_sFailureReason = "terminal spawn projection cannot be failed again";
			return result;
		}
		if (reason.IsEmpty())
			reason = "spawn projection failed before physical handoff";
		BeginCleanup(batch, CLEANUP_DISPOSITION_FINAL, reason, nowSecond);
		AdvanceCleanupIfReady(batch, nowSecond);
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		result.m_bCleanupRequired = batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING;
		return result;
	}

	HST_ForceSpawnQueueCallbackResult CompleteCleanup(
		array<ref HST_ForceSpawnResultState> batches,
		string resultId,
		string projectionId,
		string slotId,
		int attemptGeneration,
		int nowSecond,
		bool cleanupSucceeded,
		string failureReason = "")
	{
		HST_ForceSpawnQueueCallbackResult result = new HST_ForceSpawnQueueCallbackResult();
		if (!batches || resultId.IsEmpty() || projectionId.IsEmpty() || slotId.IsEmpty() || CountByResult(batches, resultId) != 1)
		{
			result.m_sFailureReason = "spawn cleanup callback identity is missing or ambiguous";
			return result;
		}
		HST_ForceSpawnResultState batch = FindByResult(batches, resultId);
		result.m_Batch = batch;
		if (!BatchHasUniqueKeys(batches, batch))
		{
			result.m_sFailureReason = "spawn cleanup callback durable identity is ambiguous";
			return result;
		}
		if (projectionId != batch.m_sProjectionId)
		{
			result.m_sFailureReason = "spawn cleanup callback projection identity conflict";
			return result;
		}
		if (attemptGeneration != batch.m_iAttemptGeneration)
			return MarkStaleCallback(result, "spawn cleanup callback generation is stale", false);
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
			return ResolveCleanupReplay(result, batch, slotId);
		HST_ForceSpawnSlotResultState slotResult = batch.FindSlotResult(slotId);
		if (!slotResult || slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING)
			return MarkStaleCallback(result, "spawn cleanup callback slot is not in flight", false);

		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		if (!cleanupSucceeded)
		{
			if (failureReason.IsEmpty())
				failureReason = "spawn cleanup executor reported failure";
			slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING;
			slotResult.m_sFailureReason = failureReason;
			slotResult.m_iUpdatedAtSecond = nowSecond;
			batch.m_sLastFailureReason = failureReason;
			batch.m_iNextAttemptSecond = nowSecond + DEFAULT_DEFER_SECONDS;
			batch.m_iUpdatedAtSecond = nowSecond;
			result.m_sFailureReason = failureReason;
			result.m_bCleanupRequired = true;
			return result;
		}

		string cleanedNativeGroupId = slotResult.m_sNativeGroupId;
		ClearSlotPhysicalEvidence(slotResult, nowSecond);
		slotResult.m_eStatus = CleanupDispositionSlotStatus(batch.m_sTerminalReason);
		if (slotResult.m_sSlotKind == SLOT_KIND_GROUP && !cleanedNativeGroupId.IsEmpty() && batch.m_sNativeGroupId == cleanedNativeGroupId)
			batch.m_sNativeGroupId = "";
		batch.m_iUpdatedAtSecond = nowSecond;
		if (!HasCleanupOutstanding(batch))
			batch.m_iNextAttemptSecond = 0;
		AdvanceCleanupIfReady(batch, nowSecond);
		result.m_bCleanupRequired = batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING;
		return result;
	}

	protected HST_ForceSpawnQueueCallbackResult ResolveCleanupReplay(
		HST_ForceSpawnQueueCallbackResult result,
		HST_ForceSpawnResultState batch,
		string slotId)
	{
		HST_ForceSpawnSlotResultState slotResult = batch.FindSlotResult(slotId);
		if (slotResult && slotResult.m_sEntityId.IsEmpty() && slotResult.m_sNativeGroupId.IsEmpty()
			&& slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING
			&& slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING)
		{
			result.m_bAccepted = true;
			result.m_bAlreadyApplied = true;
			return result;
		}
		return MarkStaleCallback(result, "spawn cleanup callback is no longer expected", false);
	}

	protected void ClearSlotPhysicalEvidence(HST_ForceSpawnSlotResultState slotResult, int nowSecond)
	{
		slotResult.m_sSpawnedPrefab = "";
		slotResult.m_sEntityId = "";
		slotResult.m_sAssignedVehicleEntityId = "";
		slotResult.m_sNativeGroupId = "";
		slotResult.m_iUpdatedAtSecond = nowSecond;
		slotResult.m_bAliveVerified = false;
		slotResult.m_bFactionVerified = false;
		slotResult.m_bGroupVerified = false;
		slotResult.m_bGameMasterVerified = false;
		slotResult.m_bProjectionVerified = false;
		slotResult.m_bSeatVerified = false;
	}

	protected void BeginScopedRetryCleanup(
		HST_ForceSpawnResultState batch,
		HST_ForceSpawnSlotResultState failedSlot,
		string reason,
		int nowSecond)
	{
		batch.m_sTerminalReason = CLEANUP_DISPOSITION_RETRY + reason;
		batch.m_sLastFailureReason = reason;
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING;
		batch.m_iUpdatedAtSecond = nowSecond;
		failedSlot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING;
		failedSlot.m_sFailureReason = reason;
		failedSlot.m_iUpdatedAtSecond = nowSecond;
	}

	protected void BeginCleanup(HST_ForceSpawnResultState batch, string disposition, string reason, int nowSecond)
	{
		if (!batch || IsTerminalBatch(batch))
			return;
		if (reason.IsEmpty())
			reason = "spawn cleanup required";
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING
			&& CleanupDispositionStrength(batch.m_sTerminalReason) > CleanupDispositionStrength(disposition))
			disposition = CleanupDispositionPrefix(batch.m_sTerminalReason);
		batch.m_sTerminalReason = disposition + reason;
		batch.m_sLastFailureReason = reason;
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING;
		batch.m_iUpdatedAtSecond = nowSecond;
		HST_EForceSpawnSlotStatus targetStatus = CleanupDispositionSlotStatus(disposition);
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (!slotResult)
				continue;
			if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				&& slotResult.m_bCasualtyConfirmed)
				continue;
			if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
				|| slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING
				|| slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING)
				slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING;
			else
				slotResult.m_eStatus = targetStatus;
			slotResult.m_sFailureReason = reason;
			slotResult.m_iUpdatedAtSecond = nowSecond;
		}
	}

	protected bool AdvanceCleanupIfReady(HST_ForceSpawnResultState batch, int nowSecond)
	{
		if (!batch || batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
			return false;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (slotResult && (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING
				|| slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING))
				return false;
		}

		string disposition = CleanupDispositionPrefix(batch.m_sTerminalReason);
		if (disposition == CLEANUP_DISPOSITION_RETRY || disposition == CLEANUP_DISPOSITION_DEFER)
		{
			batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS;
			batch.m_iUpdatedAtSecond = nowSecond;
			AdvanceInProgressState(batch, nowSecond);
			return true;
		}
		if (disposition == CLEANUP_DISPOSITION_CANCEL)
			MarkBatchTerminal(batch, HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED, StripCleanupDisposition(batch.m_sTerminalReason), nowSecond);
		else
			MarkBatchTerminal(batch, HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL, StripCleanupDisposition(batch.m_sTerminalReason), nowSecond);
		return true;
	}

	protected bool AdvanceInProgressState(HST_ForceSpawnResultState batch, int nowSecond)
	{
		if (!batch || batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS)
			return false;
		if (HasSlotStatus(batch, HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING))
			return false;
		if (HasSlotStatus(batch, HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_FINAL))
		{
			BeginCleanup(batch, CLEANUP_DISPOSITION_FINAL, batch.m_sLastFailureReason, nowSecond);
			AdvanceCleanupIfReady(batch, nowSecond);
			return true;
		}
		if (HasSlotStatus(batch, HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_RETRYABLE))
		{
			if (batch.m_iRetryCount >= batch.m_iMaxRetries)
			{
				BeginCleanup(batch, CLEANUP_DISPOSITION_FINAL, "spawn retry limit exhausted: " + batch.m_sLastFailureReason, nowSecond);
				AdvanceCleanupIfReady(batch, nowSecond);
				return true;
			}
			batch.m_iRetryCount++;
			batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_RETRYABLE;
			if (batch.m_iNextAttemptSecond <= nowSecond)
				batch.m_iNextAttemptSecond = ResolveRetryAt(batch, 0, nowSecond);
			batch.m_iUpdatedAtSecond = nowSecond;
			return true;
		}
		if (HasSlotStatus(batch, HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_DEFERRED))
		{
			batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_DEFERRED;
			if (batch.m_iNextAttemptSecond <= nowSecond)
				batch.m_iNextAttemptSecond = nowSecond + DEFAULT_DEFER_SECONDS;
			batch.m_iUpdatedAtSecond = nowSecond;
			return true;
		}
		return false;
	}

	protected int ResolveRetryAt(HST_ForceSpawnResultState batch, int requestedRetryAtSecond, int nowSecond)
	{
		int retryNumber = Math.Max(1, batch.m_iRetryCount + 1);
		int backoffSeconds = 2;
		for (int index = 1; index < retryNumber; index++)
			backoffSeconds = Math.Min(MAX_BACKOFF_SECONDS, backoffSeconds * 2);
		int retryAtSecond = nowSecond + backoffSeconds;
		if (requestedRetryAtSecond > retryAtSecond)
			retryAtSecond = requestedRetryAtSecond;
		return retryAtSecond;
	}

	protected HST_EForceSpawnSlotStatus CleanupDispositionSlotStatus(string disposition)
	{
		string prefix = CleanupDispositionPrefix(disposition);
		if (prefix == CLEANUP_DISPOSITION_RETRY)
			return HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_RETRYABLE;
		if (prefix == CLEANUP_DISPOSITION_DEFER)
			return HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_DEFERRED;
		if (prefix == CLEANUP_DISPOSITION_CANCEL)
			return HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CANCELLED;
		return HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_FINAL;
	}

	protected string CleanupDispositionPrefix(string value)
	{
		if (value.IndexOf(CLEANUP_DISPOSITION_RETRY) == 0)
			return CLEANUP_DISPOSITION_RETRY;
		if (value.IndexOf(CLEANUP_DISPOSITION_DEFER) == 0)
			return CLEANUP_DISPOSITION_DEFER;
		if (value.IndexOf(CLEANUP_DISPOSITION_CANCEL) == 0)
			return CLEANUP_DISPOSITION_CANCEL;
		return CLEANUP_DISPOSITION_FINAL;
	}

	protected bool IsAcceptedCancellationCleanup(HST_ForceSpawnResultState batch)
	{
		return batch
			&& batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING
			&& CleanupDispositionPrefix(batch.m_sTerminalReason) == CLEANUP_DISPOSITION_CANCEL;
	}

	protected int CleanupDispositionStrength(string value)
	{
		string prefix = CleanupDispositionPrefix(value);
		if (prefix == CLEANUP_DISPOSITION_FINAL)
			return 4;
		if (prefix == CLEANUP_DISPOSITION_CANCEL)
			return 3;
		if (prefix == CLEANUP_DISPOSITION_RETRY)
			return 2;
		return 1;
	}

	protected string StripCleanupDisposition(string value)
	{
		string prefix = CleanupDispositionPrefix(value);
		if (value.Length() <= prefix.Length())
			return "spawn cleanup completed";
		return value.Substring(prefix.Length(), value.Length() - prefix.Length());
	}

	protected bool HasSlotStatus(HST_ForceSpawnResultState batch, HST_EForceSpawnSlotStatus status)
	{
		if (!batch || !batch.m_aSlotResults)
			return false;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (slotResult && slotResult.m_eStatus == status)
				return true;
		}
		return false;
	}

	protected void MarkBatchReadyForHandoff(HST_ForceSpawnResultState batch, int nowSecond)
	{
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_READY_FOR_HANDOFF;
		batch.m_sTerminalReason = "all exact manifest slots registered; awaiting physical handoff";
		batch.m_sLastFailureReason = "";
		batch.m_iNextAttemptSecond = 0;
		batch.m_iUpdatedAtSecond = nowSecond;
		batch.m_iCompletedAtSecond = 0;
		batch.m_bCancelRequested = false;
	}

	protected void MarkBatchSucceeded(HST_ForceSpawnResultState batch, int nowSecond)
	{
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
		batch.m_sTerminalReason = "all exact manifest slots registered and verified";
		batch.m_sLastFailureReason = "";
		batch.m_iNextAttemptSecond = 0;
		batch.m_iUpdatedAtSecond = nowSecond;
		batch.m_iCompletedAtSecond = nowSecond;
		batch.m_iSuccessfulHandoffCount++;
		batch.m_iLifecycleRevision++;
		batch.m_iLastLifecycleSecond = nowSecond;
		batch.m_bCancelRequested = false;
		batch.m_bStrategicProjectionHeld = false;
		batch.m_iStrategicHoldSinceSecond = 0;
	}

	protected void MarkBatchTerminal(
		HST_ForceSpawnResultState batch,
		HST_EForceSpawnBatchStatus status,
		string reason,
		int nowSecond)
	{
		if (reason.IsEmpty())
			reason = "spawn batch terminal";
		batch.m_eStatus = status;
		batch.m_sTerminalReason = reason;
		batch.m_sLastFailureReason = reason;
		batch.m_iNextAttemptSecond = 0;
		batch.m_iUpdatedAtSecond = nowSecond;
		batch.m_iCompletedAtSecond = nowSecond;
		if (status == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
			batch.m_bCancelRequested = true;
		batch.m_bStrategicProjectionHeld = false;
		batch.m_iStrategicHoldSinceSecond = 0;
	}

	protected bool AllSlotsRegisteredAndVerified(HST_ForceSpawnResultState batch, HST_ForceManifestState manifest)
	{
		HST_ForceSpawnQueueManifestValidation validation = ValidateManifest(manifest);
		if (!validation.m_bValid || !BatchSlotsMatchManifest(batch, validation.m_aSlots))
			return false;
		array<string> entityIds = {};
		array<string> groupNativeIds = {};
		int resolvedSlotCount;
		foreach (HST_ForceSpawnQueueManifestSlot descriptor : validation.m_aSlots)
		{
			if (IsExternallyManagedAssetDescriptor(batch, descriptor))
				continue;
			HST_ForceSpawnSlotResultState slotResult = batch.FindSlotResult(descriptor.m_sSlotId);
			if (slotResult && slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED)
			{
				if (!ValidateRetiredMemberSlot(batch, descriptor, slotResult))
					return false;
				resolvedSlotCount++;
				continue;
			}
			if (!ValidateRegisteredSlot(batch, descriptor, slotResult, entityIds))
				return false;
			if (descriptor.m_sSlotKind == SLOT_KIND_GROUP)
			{
				if (groupNativeIds.Contains(slotResult.m_sNativeGroupId))
					return false;
				groupNativeIds.Insert(slotResult.m_sNativeGroupId);
			}
			entityIds.Insert(slotResult.m_sEntityId);
			resolvedSlotCount++;
		}
		return resolvedSlotCount == batch.m_iExpectedSlotCount;
	}

	protected bool ValidateRetiredMemberSlot(
		HST_ForceSpawnResultState batch,
		HST_ForceSpawnQueueManifestSlot descriptor,
		HST_ForceSpawnSlotResultState slotResult)
	{
		if (!batch || !descriptor || !slotResult || descriptor.m_sSlotKind != SLOT_KIND_MEMBER)
			return false;
		if (!slotResult.m_bEverAlive || !slotResult.m_bCasualtyConfirmed || slotResult.m_bAliveVerified)
			return false;
		if (slotResult.m_sProjectionId != batch.m_sProjectionId || slotResult.m_iLifecycleRevision <= 0)
			return false;
		return slotResult.m_sEntityId.IsEmpty()
			&& slotResult.m_sAssignedVehicleEntityId.IsEmpty()
			&& slotResult.m_sNativeGroupId.IsEmpty();
	}

	protected bool ValidateRegisteredSlot(
		HST_ForceSpawnResultState batch,
		HST_ForceSpawnQueueManifestSlot descriptor,
		HST_ForceSpawnSlotResultState slotResult,
		array<string> entityIds)
	{
		if (!slotResult || slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED)
			return false;
		if (slotResult.m_sSpawnedPrefab != descriptor.m_sPrefab || slotResult.m_sEntityId.IsEmpty() || entityIds.Contains(slotResult.m_sEntityId))
			return false;
		if (slotResult.m_sProjectionId != batch.m_sProjectionId)
			return false;
		if (!slotResult.m_bAliveVerified || !slotResult.m_bFactionVerified || !slotResult.m_bGroupVerified
			|| !slotResult.m_bGameMasterVerified || !slotResult.m_bProjectionVerified)
			return false;
		if (descriptor.m_sSlotKind == SLOT_KIND_GROUP)
			return !slotResult.m_sNativeGroupId.IsEmpty();
		string expectedNativeGroupId = ResolveExpectedNativeGroup(batch, descriptor);
		if (expectedNativeGroupId.IsEmpty() || slotResult.m_sNativeGroupId != expectedNativeGroupId)
			return false;
		if (descriptor.m_sAssignedVehicleSlotId.IsEmpty())
			return slotResult.m_sAssignedVehicleEntityId.IsEmpty();
		string expectedVehicleEntityId = ResolveAssignedVehicleEntity(batch, descriptor.m_sAssignedVehicleSlotId);
		return !expectedVehicleEntityId.IsEmpty()
			&& slotResult.m_sAssignedVehicleEntityId == expectedVehicleEntityId
			&& slotResult.m_bSeatVerified;
	}

	HST_ForceSpawnQueueMaintenanceResult ReconcileAfterRestore(
		array<ref HST_ForceSpawnResultState> batches,
		array<ref HST_ForceManifestState> manifests,
		int nowSecond)
	{
		HST_ForceSpawnQueueMaintenanceResult result = new HST_ForceSpawnQueueMaintenanceResult();
		if (!batches || !manifests)
		{
			result.m_aEvidence.Insert("restore reconciliation unavailable: batches or manifests missing");
			return result;
		}
		if (FailClosedDuplicateNonterminalKeys(batches, manifests, nowSecond))
			result.m_bStateChanged = true;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (!batch)
				continue;
			result.m_iInspectedBatchCount++;
			if (IsExternallyManagedMissionConvoyBatch(batch, manifests))
				continue;
			if (CollidesWithExternallyManagedMissionConvoyBatch(batch, batches, manifests))
				continue;
			if (IsTerminalBatch(batch))
			{
				if (ClearTerminalProcessIds(batch))
				{
					result.m_bStateChanged = true;
					result.m_iReconciledBatchCount++;
					result.m_aEvidence.Insert(string.Format("%1 terminal process-local ids cleared", batch.m_sResultId));
				}
				continue;
			}
			bool changed = ReconcileRestoredBatch(batch, manifests, nowSecond);
			if (!changed)
				continue;
			result.m_bStateChanged = true;
			result.m_iReconciledBatchCount++;
			result.m_aEvidence.Insert(string.Format("%1 -> %2 generation %3", batch.m_sResultId, BatchStatusLabel(batch.m_eStatus), batch.m_iAttemptGeneration));
		}
		result.m_iRemainingTerminalCount = CountTerminalBatches(batches);
		return result;
	}

	HST_ForceSpawnQueueMaintenanceResult ReconcileCampaignAfterRestore(HST_CampaignState state)
	{
		HST_ForceSpawnQueueMaintenanceResult result = new HST_ForceSpawnQueueMaintenanceResult();
		if (!state)
		{
			result.m_aEvidence.Insert("campaign restore reconciliation skipped: state missing");
			return result;
		}
		if (!state.m_bRestoredFromPersistence)
		{
			result.m_aEvidence.Insert("campaign restore reconciliation skipped: state was not restored");
			return result;
		}
		int restoreSequence = Math.Max(0, state.m_iPersistenceRestoreSequence);
		if (state.m_iForceSpawnQueueReconciledRestoreSequence == restoreSequence)
		{
			result.m_aEvidence.Insert(string.Format("campaign restore sequence %1 already reconciled", restoreSequence));
			return result;
		}

		result = ReconcileAfterRestore(state.m_aForceSpawnResults, state.m_aForceManifests, state.m_iElapsedSeconds);
		state.m_iForceSpawnQueueReconciledRestoreSequence = restoreSequence;
		result.m_bStateChanged = true;
		result.m_aEvidence.Insert(string.Format("campaign restore sequence %1 reconciled", restoreSequence));
		return result;
	}

	protected bool ClearTerminalProcessIds(HST_ForceSpawnResultState batch)
	{
		if (!batch || !IsTerminalBatch(batch))
			return false;
		bool changed;
		if (!batch.m_sNativeGroupId.IsEmpty())
		{
			batch.m_sNativeGroupId = "";
			changed = true;
		}
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (!slotResult)
				continue;
			if (slotResult.m_sEntityId.IsEmpty() && slotResult.m_sAssignedVehicleEntityId.IsEmpty() && slotResult.m_sNativeGroupId.IsEmpty())
				continue;
			slotResult.m_sEntityId = "";
			slotResult.m_sAssignedVehicleEntityId = "";
			slotResult.m_sNativeGroupId = "";
			changed = true;
		}
		return changed;
	}

	protected bool ReconcileRestoredBatch(
		HST_ForceSpawnResultState batch,
		array<ref HST_ForceManifestState> manifests,
		int nowSecond)
	{
		NormalizeRestoredCounters(batch, nowSecond);
		HST_EForceSpawnBatchStatus restoredStatus = batch.m_eStatus;
		string restoredDisposition = CleanupDispositionPrefix(batch.m_sTerminalReason);
		ResetRuntimeEvidenceAfterRestore(batch, restoredStatus, restoredDisposition, nowSecond);
		HST_ForceManifestState manifest = FindManifest(manifests, batch.m_sManifestId);
		string integrityFailure = ValidateBatchManifest(batch, manifest);
		if (!integrityFailure.IsEmpty())
		{
			BeginCleanup(batch, CLEANUP_DISPOSITION_FINAL, "restore integrity conflict: " + integrityFailure, nowSecond);
			AdvanceCleanupIfReady(batch, nowSecond);
			return true;
		}
		if (restoredStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING
			&& restoredDisposition == CLEANUP_DISPOSITION_CANCEL)
		{
			AdvanceRestoredDisposition(batch, restoredStatus, restoredDisposition, nowSecond);
			return true;
		}
		if (!batch.m_bStrategicProjectionHeld && batch.m_iDeadlineSecond > 0 && nowSecond >= batch.m_iDeadlineSecond)
		{
			BeginCleanup(batch, CLEANUP_DISPOSITION_FINAL, "restored spawn deadline expired", nowSecond);
			AdvanceCleanupIfReady(batch, nowSecond);
			return true;
		}
		if (batch.m_bCancelRequested)
		{
			BeginCleanup(batch, CLEANUP_DISPOSITION_CANCEL, "restored cancellation request", nowSecond);
			AdvanceCleanupIfReady(batch, nowSecond);
			return true;
		}
		AdvanceRestoredDisposition(batch, restoredStatus, restoredDisposition, nowSecond);
		return true;
	}

	protected bool NormalizeRestoredCounters(HST_ForceSpawnResultState batch, int nowSecond)
	{
		bool changed;
		int retryCount = Math.Max(0, batch.m_iRetryCount);
		int maxRetries = Math.Max(0, batch.m_iMaxRetries);
		int generation = Math.Max(0, batch.m_iAttemptGeneration);
		if (retryCount != batch.m_iRetryCount || maxRetries != batch.m_iMaxRetries || generation != batch.m_iAttemptGeneration)
			changed = true;
		batch.m_iRetryCount = retryCount;
		batch.m_iMaxRetries = maxRetries;
		batch.m_iAttemptGeneration = generation;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (!slotResult)
				continue;
			int attempts = Math.Max(0, slotResult.m_iAttemptCount);
			if (attempts != slotResult.m_iAttemptCount)
			{
				slotResult.m_iAttemptCount = attempts;
				slotResult.m_iUpdatedAtSecond = nowSecond;
				changed = true;
			}
		}
		if (changed)
			batch.m_iUpdatedAtSecond = nowSecond;
		return changed;
	}

	protected void ResetRuntimeEvidenceAfterRestore(
		HST_ForceSpawnResultState batch,
		HST_EForceSpawnBatchStatus restoredStatus,
		string restoredDisposition,
		int nowSecond)
	{
		batch.m_iAttemptGeneration++;
		batch.m_sNativeGroupId = "";
		batch.m_iUpdatedAtSecond = nowSecond;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (!slotResult)
				continue;
			ClearSlotPhysicalEvidence(slotResult, nowSecond);
			slotResult.m_sProjectionId = batch.m_sProjectionId;
			if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				&& slotResult.m_bCasualtyConfirmed)
				continue;
			slotResult.m_eStatus = RestoredSlotStatus(restoredStatus, restoredDisposition);
		}
	}

	protected HST_EForceSpawnSlotStatus RestoredSlotStatus(
		HST_EForceSpawnBatchStatus restoredStatus,
		string restoredDisposition)
	{
		if (restoredStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING)
			return HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED;
		if (restoredStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_DEFERRED)
			return HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_DEFERRED;
		if (restoredStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
			return CleanupDispositionSlotStatus(restoredDisposition);
		return HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_RETRYABLE;
	}

	protected void AdvanceRestoredDisposition(
		HST_ForceSpawnResultState batch,
		HST_EForceSpawnBatchStatus restoredStatus,
		string restoredDisposition,
		int nowSecond)
	{
		if (restoredStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
		{
			batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING;
			batch.m_sTerminalReason = restoredDisposition + StripCleanupDisposition(batch.m_sTerminalReason);
			AdvanceCleanupIfReady(batch, nowSecond);
			return;
		}
		if (restoredStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS)
		{
			batch.m_sLastFailureReason = "interrupted spawn attempt restored";
			batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS;
			if (batch.m_iNextAttemptSecond <= nowSecond)
				batch.m_iNextAttemptSecond = ResolveRetryAt(batch, 0, nowSecond);
			AdvanceInProgressState(batch, nowSecond);
			return;
		}
		if (restoredStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_READY_FOR_HANDOFF)
		{
			batch.m_sLastFailureReason = "ready physical handoff was interrupted by restore";
			batch.m_sTerminalReason = "";
			batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_RETRYABLE;
			batch.m_iNextAttemptSecond = nowSecond;
			batch.m_iUpdatedAtSecond = nowSecond;
			return;
		}
		batch.m_eStatus = restoredStatus;
		batch.m_iUpdatedAtSecond = nowSecond;
	}

	HST_ForceSpawnQueueMaintenanceResult CompactTerminalRows(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceSpawnQueueRetentionPins pins,
		int nowSecond)
	{
		HST_ForceSpawnQueueMaintenanceResult result = new HST_ForceSpawnQueueMaintenanceResult();
		if (!batches)
		{
			result.m_aEvidence.Insert("terminal compaction unavailable: batch collection missing");
			return result;
		}
		result.m_iInspectedBatchCount = batches.Count();
		if (!pins)
		{
			result.m_bPinsRequired = true;
			result.m_iRemainingTerminalCount = CountTerminalBatches(batches);
			result.m_aEvidence.Insert("terminal compaction refused without explicit backlink retention pins");
			return result;
		}

		ClassifyTerminalRetention(batches, pins, nowSecond, result);
		while (CountTerminalBatches(batches) >= MAX_TERMINAL_ROWS)
		{
			int removeIndex = FindOldestEligibleTerminalIndex(batches, pins, nowSecond);
			if (removeIndex < 0)
				break;
			HST_ForceSpawnResultState removed = batches[removeIndex];
			result.m_aEvidence.Insert(string.Format("compacted terminal %1 projection %2", removed.m_sResultId, removed.m_sProjectionId));
			batches.Remove(removeIndex);
			result.m_iRemovedTerminalCount++;
			result.m_bStateChanged = true;
		}
		result.m_iRemainingTerminalCount = CountTerminalBatches(batches);
		if (result.m_iRemainingTerminalCount > MAX_TERMINAL_ROWS)
			result.m_aEvidence.Insert(string.Format("terminal retention remains over capacity: %1 protected rows", result.m_iRemainingTerminalCount));
		return result;
	}

	protected void ClassifyTerminalRetention(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceSpawnQueueRetentionPins pins,
		int nowSecond,
		HST_ForceSpawnQueueMaintenanceResult result)
	{
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (!IsTerminalBatch(batch))
				continue;
			if (pins.Contains(batch))
			{
				result.m_iPinnedTerminalCount++;
				continue;
			}
			if (!IsTerminalRetentionExpired(batch, nowSecond))
			{
				result.m_iYoungTerminalCount++;
				continue;
			}
			result.m_iEligibleTerminalCount++;
		}
	}

	protected int FindOldestEligibleTerminalIndex(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceSpawnQueueRetentionPins pins,
		int nowSecond)
	{
		int selectedIndex = -1;
		int selectedCompletedSecond = int.MAX;
		for (int index = 0; index < batches.Count(); index++)
		{
			HST_ForceSpawnResultState batch = batches[index];
			if (!IsTerminalBatch(batch) || pins.Contains(batch) || !IsTerminalRetentionExpired(batch, nowSecond))
				continue;
			if (selectedIndex < 0 || batch.m_iCompletedAtSecond < selectedCompletedSecond)
			{
				selectedIndex = index;
				selectedCompletedSecond = batch.m_iCompletedAtSecond;
			}
		}
		return selectedIndex;
	}

	protected bool IsTerminalRetentionExpired(HST_ForceSpawnResultState batch, int nowSecond)
	{
		if (!batch || batch.m_iCompletedAtSecond < 0)
			return false;
		return nowSecond - batch.m_iCompletedAtSecond >= TERMINAL_RETENTION_SECONDS;
	}

	string BuildReport(array<ref HST_ForceSpawnResultState> batches, int nowSecond, int maxRows = 16)
	{
		if (!batches)
			return "spawn queue unavailable";
		int nonterminalCount = CountNonterminalBatches(batches);
		int terminalCount = CountTerminalBatches(batches);
		int nonterminalSlots = CountNonterminalSlots(batches);
		int pendingCount;
		int inProgressCount;
		int deferredCount;
		int retryableCount;
		int cleanupCount;
		int readyCount;
		int oldestAge;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (!batch || IsTerminalBatch(batch))
				continue;
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING)
				pendingCount++;
			else if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS)
				inProgressCount++;
			else if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_DEFERRED)
				deferredCount++;
			else if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_RETRYABLE)
				retryableCount++;
			else if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
				cleanupCount++;
			else if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_READY_FOR_HANDOFF)
				readyCount++;
			oldestAge = Math.Max(oldestAge, Math.Max(0, nowSecond - batch.m_iCreatedAtSecond));
		}
		string report = string.Format(
			"spawn queue | active %1/%2 | active slots %3/%4 | terminal %5/%6 | pending %7 | in progress %8 | deferred %9",
			nonterminalCount,
			MAX_NONTERMINAL_BATCHES,
			nonterminalSlots,
			MAX_TOTAL_SLOT_ROWS,
			terminalCount,
			MAX_TERMINAL_ROWS,
			pendingCount,
			inProgressCount,
			deferredCount
		);
		report = report + string.Format(
			" | retryable %1 | cleanup %2 | ready %3 | oldest %4s",
			retryableCount,
			cleanupCount,
			readyCount,
			oldestAge
		);
		if (maxRows <= 0)
			return report;
		int rows;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (!batch || IsTerminalBatch(batch) || rows >= maxRows)
				continue;
			report = report + "\n" + BuildBatchReportRow(batch, nowSecond);
			rows++;
		}
		return report;
	}

	protected string BuildBatchReportRow(HST_ForceSpawnResultState batch, int nowSecond)
	{
		int registered;
		int spawning;
		int cleanup;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (!slotResult)
				continue;
			if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED)
				registered++;
			else if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_SPAWNING)
				spawning++;
			else if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING)
				cleanup++;
		}
		string row = string.Format(
			"- %1 | %2 | projection %3 | priority %4 | age %5s | generation %6 | retry %7/%8 | expected slots %9",
			batch.m_sResultId,
			BatchStatusLabel(batch.m_eStatus),
			batch.m_sProjectionId,
			batch.m_iPriority,
			Math.Max(0, nowSecond - batch.m_iCreatedAtSecond),
			batch.m_iAttemptGeneration,
			batch.m_iRetryCount,
			batch.m_iMaxRetries,
			batch.m_iExpectedSlotCount
		);
		return row + string.Format(
			" | registered %1 | in flight %2 | cleanup %3 | next %4 | deadline %5 | last %6",
			registered,
			spawning,
			cleanup,
			batch.m_iNextAttemptSecond,
			batch.m_iDeadlineSecond,
			EmptyLabel(batch.m_sLastFailureReason)
		);
	}

	protected bool FailClosedDuplicateNonterminalKeys(
		array<ref HST_ForceSpawnResultState> batches,
		array<ref HST_ForceManifestState> manifests,
		int nowSecond)
	{
		bool changed;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (!batch || IsTerminalBatch(batch))
				continue;
			if (IsExternallyManagedMissionConvoyBatch(batch, manifests))
				continue;
			if (CollidesWithExternallyManagedMissionConvoyBatch(batch, batches, manifests))
				continue;
			bool duplicate = batch.m_sResultId.IsEmpty() || batch.m_sRequestId.IsEmpty() || batch.m_sProjectionId.IsEmpty();
			if (!duplicate && CountByResult(batches, batch.m_sResultId) > 1)
				duplicate = true;
			if (!duplicate && CountByRequest(batches, batch.m_sRequestId) > 1)
				duplicate = true;
			if (!duplicate && CountByProjection(batches, batch.m_sProjectionId) > 1)
				duplicate = true;
			if (!duplicate)
				continue;
			BeginCleanup(batch, CLEANUP_DISPOSITION_FINAL, "duplicate or missing durable spawn identity", nowSecond);
			changed = true;
		}
		return changed;
	}

	protected bool CollidesWithExternallyManagedMissionConvoyBatch(
		HST_ForceSpawnResultState batch,
		array<ref HST_ForceSpawnResultState> batches,
		array<ref HST_ForceManifestState> manifests)
	{
		if (!batch || !batches || IsTerminalBatch(batch)
			|| IsExternallyManagedMissionConvoyBatch(batch, manifests))
			return false;
		foreach (HST_ForceSpawnResultState externalBatch : batches)
		{
			if (!externalBatch || externalBatch == batch || IsTerminalBatch(externalBatch)
				|| !IsExternallyManagedMissionConvoyBatch(externalBatch, manifests))
				continue;
			if ((!batch.m_sResultId.IsEmpty() && batch.m_sResultId == externalBatch.m_sResultId)
				|| (!batch.m_sRequestId.IsEmpty() && batch.m_sRequestId == externalBatch.m_sRequestId)
				|| (!batch.m_sProjectionId.IsEmpty() && batch.m_sProjectionId == externalBatch.m_sProjectionId))
				return true;
		}
		return false;
	}

	protected bool IsExternallyManagedMissionConvoyBatch(
		HST_ForceSpawnResultState batch,
		array<ref HST_ForceManifestState> manifests)
	{
		if (!batch)
			return false;
		HST_ForceManifestState manifest = FindManifest(manifests, batch.m_sManifestId);
		if (manifest && (manifest.m_sForceKind == HST_MissionConvoyOperationService.EXACT_FORCE_KIND
			|| manifest.m_sPolicyId == HST_MissionConvoyOperationService.EXACT_POLICY_ID))
			return true;

		// Missing or conflicting manifests belong to the mission-convoy authority
		// validator when their deterministic IDs claim that namespace.  The generic
		// queue must not mutate those rows before mission-only quarantine runs.
		return batch.m_sResultId.StartsWith("spawn_mission_convoy_")
			|| batch.m_sManifestId.StartsWith("manifest_mission_convoy_")
			|| batch.m_sOperationId.StartsWith("operation_mission_convoy_")
			|| batch.m_sForceId.StartsWith("force_mission_convoy_")
			|| batch.m_sProjectionId.StartsWith("projection_mission_convoy_");
	}

	protected HST_ForceSpawnResultState FindByResult(array<ref HST_ForceSpawnResultState> batches, string resultId)
	{
		if (!batches || resultId.IsEmpty())
			return null;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (batch && batch.m_sResultId == resultId)
				return batch;
		}
		return null;
	}

	protected HST_ForceSpawnResultState FindByRequest(array<ref HST_ForceSpawnResultState> batches, string requestId)
	{
		if (!batches || requestId.IsEmpty())
			return null;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (batch && batch.m_sRequestId == requestId)
				return batch;
		}
		return null;
	}

	protected HST_ForceSpawnResultState FindByProjection(array<ref HST_ForceSpawnResultState> batches, string projectionId)
	{
		if (!batches || projectionId.IsEmpty())
			return null;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (batch && batch.m_sProjectionId == projectionId)
				return batch;
		}
		return null;
	}

	protected int CountByResult(array<ref HST_ForceSpawnResultState> batches, string resultId)
	{
		int count;
		if (!batches || resultId.IsEmpty())
			return count;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (batch && batch.m_sResultId == resultId)
				count++;
		}
		return count;
	}

	protected int CountByRequest(array<ref HST_ForceSpawnResultState> batches, string requestId)
	{
		int count;
		if (!batches || requestId.IsEmpty())
			return count;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (batch && batch.m_sRequestId == requestId)
				count++;
		}
		return count;
	}

	protected int CountByProjection(array<ref HST_ForceSpawnResultState> batches, string projectionId)
	{
		int count;
		if (!batches || projectionId.IsEmpty())
			return count;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (batch && batch.m_sProjectionId == projectionId)
				count++;
		}
		return count;
	}

	protected bool BatchHasUniqueKeys(array<ref HST_ForceSpawnResultState> batches, HST_ForceSpawnResultState batch)
	{
		if (!batches || !batch)
			return false;
		if (batch.m_sResultId.IsEmpty() || batch.m_sRequestId.IsEmpty() || batch.m_sProjectionId.IsEmpty())
			return false;
		if (CountByResult(batches, batch.m_sResultId) != 1)
			return false;
		if (CountByRequest(batches, batch.m_sRequestId) != 1)
			return false;
		return CountByProjection(batches, batch.m_sProjectionId) == 1;
	}

	protected int CountNonterminalByResult(array<ref HST_ForceSpawnResultState> batches, string resultId)
	{
		int count;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (batch && !IsTerminalBatch(batch) && batch.m_sResultId == resultId)
				count++;
		}
		return count;
	}

	protected int CountNonterminalByRequest(array<ref HST_ForceSpawnResultState> batches, string requestId)
	{
		int count;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (batch && !IsTerminalBatch(batch) && batch.m_sRequestId == requestId)
				count++;
		}
		return count;
	}

	protected int CountNonterminalByProjection(array<ref HST_ForceSpawnResultState> batches, string projectionId)
	{
		int count;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (batch && !IsTerminalBatch(batch) && batch.m_sProjectionId == projectionId)
				count++;
		}
		return count;
	}

	protected int CountNonterminalBatches(array<ref HST_ForceSpawnResultState> batches)
	{
		int count;
		if (!batches)
			return count;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (batch && !IsTerminalBatch(batch))
				count++;
		}
		return count;
	}

	protected int CountTerminalBatches(array<ref HST_ForceSpawnResultState> batches)
	{
		int count;
		if (!batches)
			return count;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (IsTerminalBatch(batch))
				count++;
		}
		return count;
	}

	protected int CountNonterminalSlots(array<ref HST_ForceSpawnResultState> batches)
	{
		int count;
		if (!batches)
			return count;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (batch && !IsTerminalBatch(batch) && batch.m_aSlotResults)
				count += batch.m_aSlotResults.Count();
		}
		return count;
	}

	protected bool IsTerminalBatch(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return false;
		return batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED;
	}

	protected string BatchStatusLabel(HST_EForceSpawnBatchStatus status)
	{
		if (status == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			return "succeeded";
		if (status == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_DEFERRED)
			return "deferred";
		if (status == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_RETRYABLE)
			return "failed_retryable";
		if (status == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL)
			return "failed_final";
		if (status == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
			return "cancelled";
		if (status == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS)
			return "in_progress";
		if (status == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
			return "cleanup_pending";
		if (status == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_READY_FOR_HANDOFF)
			return "ready_for_handoff";
		return "pending";
	}

	protected string EmptyLabel(string value)
	{
		if (value.IsEmpty())
			return "none";
		return value;
	}
}
