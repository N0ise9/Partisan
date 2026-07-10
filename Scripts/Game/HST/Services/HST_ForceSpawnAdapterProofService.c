class HST_ForceSpawnAdapterProofReport
{
	bool m_bPrerequisiteReady;
	bool m_bRootBeforeMembers;
	bool m_bCancelCleanupExact;
	bool m_bExactPrefabs;
	bool m_bNativeMembershipExact;
	bool m_bGameMasterMembershipExact;
	bool m_bDurableSuccessExact;
	bool m_bActiveGroupHandoffExact;
	bool m_bRetirementExact;
	bool m_bSameWaveFailureCleanupExact;
	bool m_bStateIsolationExact;
	string m_sPrerequisiteEvidence;
	string m_sCancelEvidence;
	string m_sDurableEvidence;
	string m_sRuntimeEvidence;
	string m_sHandoffEvidence;
	string m_sRetirementEvidence;
	string m_sFailureCleanupEvidence;
	string m_sIsolationEvidence;
}

class HST_ForceSpawnAdapterProofService
{
	static const string GROUP_PREFAB = "{6E725D44CA973C24}Prefabs/Groups/INDFOR/Group_FIA_SentryTeam.et";
	static const string MEMBER_PREFAB = "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et";
	static const string UNAVAILABLE_MEMBER_PREFAB = "{0000000000000000}Prefabs/Characters/HST/HST_CampaignDebug_MissingSpawnMember.et";
	static const int FIXTURE_MEMBER_COUNT = 2;
	static const int CLEANUP_TICK_LIMIT = 4;

	protected ref HST_ForcePlanningIntegrityService m_Integrity = new HST_ForcePlanningIntegrityService();
	protected bool m_bStarted;
	protected bool m_bPrerequisiteReady;
	protected bool m_bRootBeforeMembers;
	protected bool m_bCancelCleanupExact;
	protected bool m_bExactPrefabs;
	protected bool m_bNativeMembershipExact;
	protected bool m_bGameMasterMembershipExact;
	protected bool m_bDurableSuccessExact;
	protected bool m_bActiveGroupHandoffExact;
	protected bool m_bRetirementExact;
	protected bool m_bSameWaveFailurePendingExact;
	protected bool m_bSameWaveFailureCleanupExact;
	protected bool m_bStateIsolationExact;
	protected int m_iManifestCountBefore;
	protected int m_iBatchCountBefore;
	protected int m_iActiveGroupCountBefore;
	protected int m_iBindingCountBefore;
	protected string m_sManifestId;
	protected string m_sOperationId;
	protected string m_sGroupElementId;
	protected string m_sFirstMemberSlotId;
	protected string m_sSecondMemberSlotId;
	protected string m_sCancelResultId;
	protected string m_sCancelRequestId;
	protected string m_sCancelForceId;
	protected string m_sCancelProjectionId;
	protected string m_sSuccessResultId;
	protected string m_sSuccessRequestId;
	protected string m_sSuccessForceId;
	protected string m_sSuccessProjectionId;
	protected string m_sFailureManifestId;
	protected string m_sFailureOperationId;
	protected string m_sFailureGroupElementId;
	protected string m_sFailureFirstMemberSlotId;
	protected string m_sFailureSecondMemberSlotId;
	protected string m_sFailureResultId;
	protected string m_sFailureRequestId;
	protected string m_sFailureForceId;
	protected string m_sFailureProjectionId;
	protected string m_sPrerequisiteEvidence;
	protected string m_sCancelEvidence;
	protected string m_sDurableEvidence;
	protected string m_sRuntimeEvidence;
	protected string m_sHandoffEvidence;
	protected string m_sRetirementEvidence;
	protected string m_sFailureCleanupEvidence;
	protected string m_sIsolationEvidence;

	bool IsRuntimeExecutionActive()
	{
		return m_bStarted && m_bPrerequisiteReady;
	}

	string Start(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar,
		string debugPrefix,
		bool isolationActive,
		bool physicalBlocked)
	{
		Cleanup(state, queue, adapter, physicalWar);
		ResetObservations();
		m_bStarted = true;
		CaptureBaseline(state, adapter);
		string failure = ValidateStart(state, queue, adapter, physicalWar, debugPrefix, isolationActive, physicalBlocked);
		if (!failure.IsEmpty())
		{
			m_sPrerequisiteEvidence = failure;
			return "h-istasi campaign debug | exact spawn adapter proof blocked: " + failure;
		}

		BuildFixtureIds(debugPrefix);
		HST_ForceManifestState manifest = BuildManifest(state.m_iElapsedSeconds);
		state.m_aForceManifests.Insert(manifest);
		HST_ActiveGroupState cancelGroup = BuildActiveGroup(m_sCancelForceId, m_sCancelProjectionId, m_sCancelResultId, state);
		state.m_aActiveGroups.Insert(cancelGroup);
		HST_ForceSpawnQueueEnqueueResult enqueue = queue.Enqueue(
			state.m_aForceSpawnResults,
			manifest,
			BuildRequest(m_sCancelResultId, m_sCancelRequestId, m_sCancelForceId, m_sCancelProjectionId, state.m_iElapsedSeconds, 100),
			state.m_iElapsedSeconds);
		if (!enqueue || !enqueue.m_bSuccess)
		{
			m_sPrerequisiteEvidence = "cancel projection enqueue failed: " + ResolveEnqueueFailure(enqueue);
			Cleanup(state, queue, adapter, physicalWar);
			return "h-istasi campaign debug | exact spawn adapter proof failed: " + m_sPrerequisiteEvidence;
		}

		m_bPrerequisiteReady = true;
		m_sPrerequisiteEvidence = string.Format(
			"schema %1 | isolated %2 | manifest %3 | cancel result %4",
			state.m_iSchemaVersion,
			isolationActive,
			m_sManifestId,
			m_sCancelResultId);
		return "h-istasi campaign debug | exact spawn adapter proof queued partial-cancel projection";
	}

	string CancelPartialAndStartSuccess(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar)
	{
		if (!m_bStarted || !m_bPrerequisiteReady)
			return "h-istasi campaign debug | exact spawn adapter proof is not ready";

		CaptureRootBeforeMembers(state, adapter, physicalWar);
		HST_ForceSpawnQueueCallbackResult cancel = queue.RequestCancel(
			state.m_aForceSpawnResults,
			m_sCancelResultId,
			state.m_iElapsedSeconds,
			"campaign debug exact partial cancellation");
		if (!cancel || !cancel.m_bAccepted)
		{
			m_sCancelEvidence = m_sCancelEvidence + " | cancel rejected " + ResolveCallbackFailure(cancel);
			return "h-istasi campaign debug | exact spawn adapter partial cancellation was rejected";
		}

		HST_ForceManifestState manifest = state.FindForceManifest(m_sManifestId);
		HST_ActiveGroupState successGroup = BuildActiveGroup(m_sSuccessForceId, m_sSuccessProjectionId, m_sSuccessResultId, state);
		state.m_aActiveGroups.Insert(successGroup);
		HST_ForceSpawnQueueEnqueueResult enqueue = queue.Enqueue(
			state.m_aForceSpawnResults,
			manifest,
			BuildRequest(m_sSuccessResultId, m_sSuccessRequestId, m_sSuccessForceId, m_sSuccessProjectionId, state.m_iElapsedSeconds, 90),
			state.m_iElapsedSeconds);
		if (!enqueue || !enqueue.m_bSuccess)
		{
			RemoveActiveGroupByProjection(state, m_sSuccessProjectionId);
			m_sCancelEvidence = m_sCancelEvidence + " | success projection enqueue failed " + ResolveEnqueueFailure(enqueue);
			return "h-istasi campaign debug | exact spawn adapter success projection enqueue failed";
		}

		return "h-istasi campaign debug | partial projection cancelled and exact success projection queued";
	}

	string CaptureAndRetireSuccess(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar)
	{
		if (!m_bStarted || !m_bPrerequisiteReady)
			return "h-istasi campaign debug | exact spawn adapter proof is not ready";

		CaptureCancelledProjection(state, adapter, physicalWar);
		CaptureSuccessfulProjection(state, adapter, physicalWar);
		HST_ForceSpawnAdapterRetireResult retirement = adapter.RetireProjectionRuntime(state, physicalWar, m_sSuccessProjectionId);
		CaptureRetirement(state, adapter, physicalWar, retirement);
		string enqueueFailure = StartSameWaveFailureFixture(state, queue);
		if (!enqueueFailure.IsEmpty())
		{
			m_sFailureCleanupEvidence = enqueueFailure;
			return "h-istasi campaign debug | exact spawn adapter failure-cleanup fixture could not start";
		}
		return "h-istasi campaign debug | exact spawn adapter success retired and failure-cleanup fixture queued";
	}

	string CaptureSameWaveFailure(
		HST_CampaignState state,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar)
	{
		if (!m_bStarted || !m_bPrerequisiteReady)
			return "h-istasi campaign debug | exact spawn adapter proof is not ready";

		CaptureSameWaveFailurePending(state, adapter, physicalWar);
		return "h-istasi campaign debug | exact spawn adapter same-wave member failure captured";
	}

	HST_ForceSpawnAdapterProofReport Finish(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar)
	{
		CaptureSameWaveFailureFinal(state, adapter, physicalWar);
		int cleaned = Cleanup(state, queue, adapter, physicalWar);
		HST_ForceSpawnAdapterProofReport report = BuildReport();
		report.m_sIsolationEvidence = m_sIsolationEvidence + string.Format(" | cleanup count %1", cleaned);
		m_bStarted = false;
		return report;
	}

	int Cleanup(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar)
	{
		if (!state)
			return 0;

		int cleaned;
		cleaned += RetireSucceededProjection(state, adapter, physicalWar, m_sSuccessProjectionId);
		cleaned += RetireSucceededProjection(state, adapter, physicalWar, m_sCancelProjectionId);
		RequestFixtureCancellation(state, queue, m_sCancelResultId);
		RequestFixtureCancellation(state, queue, m_sSuccessResultId);
		RequestFixtureCancellation(state, queue, m_sFailureResultId);
		cleaned += DrainFixtureCleanup(state, queue, adapter, physicalWar);
		cleaned += CleanupPhysicalFallback(state, adapter, physicalWar, m_sCancelProjectionId);
		cleaned += CleanupPhysicalFallback(state, adapter, physicalWar, m_sSuccessProjectionId);
		cleaned += CleanupPhysicalFallback(state, adapter, physicalWar, m_sFailureProjectionId);
		RemoveFixtureRows(state, adapter, physicalWar);
		CaptureIsolationResult(state, adapter);
		m_bStarted = false;
		return cleaned;
	}

	protected string ValidateStart(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar,
		string debugPrefix,
		bool isolationActive,
		bool physicalBlocked)
	{
		if (!state || !queue || !adapter || !physicalWar)
			return "state, queue, adapter, or physical-war bridge missing";
		if (physicalBlocked)
			return "physical runtime was blocked during campaign debug bootstrap";
		if (!isolationActive)
			return "campaign debug state isolation is not active";
		if (adapter.CountBindings() != 0)
			return string.Format("production adapter has %1 pre-existing runtime binding(s)", adapter.CountBindings());
		if (HasNonterminalSpawnBatch(state))
			return "isolated state contains pre-existing nonterminal spawn work";
		if (state.m_iSchemaVersion != HST_CampaignState.SCHEMA_VERSION || state.m_iSchemaVersion != 46)
			return string.Format("schema 46 required, actual %1", state.m_iSchemaVersion);
		if (debugPrefix.IsEmpty() || !debugPrefix.Contains("hst_debug_"))
			return "run-specific campaign debug prefix missing";
		if (IsZeroVector(state.m_vHQPosition))
			return "HQ position is unavailable for isolated physical placement";
		return "";
	}

	protected bool HasNonterminalSpawnBatch(HST_CampaignState state)
	{
		if (!state)
			return true;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && !IsTerminalBatch(batch))
				return true;
		}
		return false;
	}

	protected void CaptureBaseline(HST_CampaignState state, HST_ForceSpawnAdapterService adapter)
	{
		if (state)
		{
			m_iManifestCountBefore = state.m_aForceManifests.Count();
			m_iBatchCountBefore = state.m_aForceSpawnResults.Count();
			m_iActiveGroupCountBefore = state.m_aActiveGroups.Count();
		}
		if (adapter)
			m_iBindingCountBefore = adapter.CountBindings();
	}

	protected void BuildFixtureIds(string prefix)
	{
		m_sManifestId = prefix + "_spawn_adapter_manifest";
		m_sOperationId = prefix + "_spawn_adapter_operation";
		m_sGroupElementId = prefix + "_spawn_adapter_root";
		m_sFirstMemberSlotId = prefix + "_spawn_adapter_member_1";
		m_sSecondMemberSlotId = prefix + "_spawn_adapter_member_2";
		m_sCancelResultId = prefix + "_spawn_adapter_cancel_result";
		m_sCancelRequestId = prefix + "_spawn_adapter_cancel_request";
		m_sCancelForceId = prefix + "_spawn_adapter_cancel_force";
		m_sCancelProjectionId = prefix + "_spawn_adapter_cancel_projection";
		m_sSuccessResultId = prefix + "_spawn_adapter_success_result";
		m_sSuccessRequestId = prefix + "_spawn_adapter_success_request";
		m_sSuccessForceId = prefix + "_spawn_adapter_success_force";
		m_sSuccessProjectionId = prefix + "_spawn_adapter_success_projection";
		m_sFailureManifestId = prefix + "_spawn_adapter_failure_manifest";
		m_sFailureOperationId = prefix + "_spawn_adapter_failure_operation";
		m_sFailureGroupElementId = prefix + "_spawn_adapter_failure_root";
		m_sFailureFirstMemberSlotId = prefix + "_spawn_adapter_failure_member_1";
		m_sFailureSecondMemberSlotId = prefix + "_spawn_adapter_failure_member_2";
		m_sFailureResultId = prefix + "_spawn_adapter_failure_result";
		m_sFailureRequestId = prefix + "_spawn_adapter_failure_request";
		m_sFailureForceId = prefix + "_spawn_adapter_failure_force";
		m_sFailureProjectionId = prefix + "_spawn_adapter_failure_projection";
	}

	protected HST_ForceManifestState BuildManifest(int nowSecond)
	{
		HST_ForceManifestState manifest = new HST_ForceManifestState();
		manifest.m_sManifestId = m_sManifestId;
		manifest.m_sOperationId = m_sOperationId;
		manifest.m_sForceKind = "campaign_debug_exact_infantry";
		manifest.m_sFactionRole = "sentry_team";
		manifest.m_sFactionKey = "FIA";
		manifest.m_sIntentId = "campaign_debug_spawn_adapter";
		manifest.m_sGroupPrefab = GROUP_PREFAB;
		manifest.m_sCatalogVersion = HST_ForceCatalogService.CATALOG_VERSION;
		manifest.m_sPolicyId = "campaign_debug_exact_adapter_schema45";
		manifest.m_iRequestedMemberCount = FIXTURE_MEMBER_COUNT;
		manifest.m_iAcceptedMemberCount = FIXTURE_MEMBER_COUNT;
		manifest.m_iCreatedAtSecond = nowSecond;
		manifest.m_bFrozen = true;
		manifest.m_aGroups.Insert(BuildGroupSlot());
		manifest.m_aMembers.Insert(BuildMemberSlot(m_sFirstMemberSlotId, "rifleman_1", 0));
		manifest.m_aMembers.Insert(BuildMemberSlot(m_sSecondMemberSlotId, "rifleman_2", 1));
		manifest.m_sManifestHash = m_Integrity.BuildManifestHash(manifest);
		return manifest;
	}

	protected HST_ForceManifestGroupState BuildGroupSlot()
	{
		HST_ForceManifestGroupState group = new HST_ForceManifestGroupState();
		group.m_sElementId = m_sGroupElementId;
		group.m_sCatalogEntryId = "fia_sentry_team";
		group.m_sPrefab = GROUP_PREFAB;
		group.m_sRole = "sentry_team";
		group.m_iOrdinal = 0;
		group.m_iExpectedMemberCount = FIXTURE_MEMBER_COUNT;
		group.m_bRequired = true;
		return group;
	}

	protected HST_ForceManifestMemberState BuildMemberSlot(string slotId, string catalogSlotId, int ordinal)
	{
		HST_ForceManifestMemberState member = new HST_ForceManifestMemberState();
		member.m_sSlotId = slotId;
		member.m_sCatalogSlotId = catalogSlotId;
		member.m_sGroupElementId = m_sGroupElementId;
		member.m_sPrefab = MEMBER_PREFAB;
		member.m_sRole = "rifleman";
		member.m_iOrdinal = ordinal;
		member.m_iHRCost = 0;
		member.m_bRequired = true;
		return member;
	}

	protected HST_ForceManifestState BuildFailureManifest(int nowSecond)
	{
		HST_ForceManifestState manifest = new HST_ForceManifestState();
		manifest.m_sManifestId = m_sFailureManifestId;
		manifest.m_sOperationId = m_sFailureOperationId;
		manifest.m_sForceKind = "campaign_debug_failure_infantry";
		manifest.m_sFactionRole = "sentry_team";
		manifest.m_sFactionKey = "FIA";
		manifest.m_sIntentId = "campaign_debug_spawn_adapter_failure_cleanup";
		manifest.m_sGroupPrefab = GROUP_PREFAB;
		manifest.m_sCatalogVersion = HST_ForceCatalogService.CATALOG_VERSION;
		manifest.m_sPolicyId = "campaign_debug_exact_adapter_failure_schema45";
		manifest.m_iRequestedMemberCount = FIXTURE_MEMBER_COUNT;
		manifest.m_iAcceptedMemberCount = FIXTURE_MEMBER_COUNT;
		manifest.m_iCreatedAtSecond = nowSecond;
		manifest.m_bFrozen = true;
		manifest.m_aGroups.Insert(BuildFailureGroupSlot());
		manifest.m_aMembers.Insert(BuildFailureMemberSlot(m_sFailureFirstMemberSlotId, "unavailable_member_1", UNAVAILABLE_MEMBER_PREFAB, 0));
		manifest.m_aMembers.Insert(BuildFailureMemberSlot(m_sFailureSecondMemberSlotId, "rifleman_2", MEMBER_PREFAB, 1));
		manifest.m_sManifestHash = m_Integrity.BuildManifestHash(manifest);
		return manifest;
	}

	protected HST_ForceManifestGroupState BuildFailureGroupSlot()
	{
		HST_ForceManifestGroupState group = new HST_ForceManifestGroupState();
		group.m_sElementId = m_sFailureGroupElementId;
		group.m_sCatalogEntryId = "fia_sentry_team";
		group.m_sPrefab = GROUP_PREFAB;
		group.m_sRole = "sentry_team";
		group.m_iOrdinal = 0;
		group.m_iExpectedMemberCount = FIXTURE_MEMBER_COUNT;
		group.m_bRequired = true;
		return group;
	}

	protected HST_ForceManifestMemberState BuildFailureMemberSlot(string slotId, string catalogSlotId, string prefab, int ordinal)
	{
		HST_ForceManifestMemberState member = new HST_ForceManifestMemberState();
		member.m_sSlotId = slotId;
		member.m_sCatalogSlotId = catalogSlotId;
		member.m_sGroupElementId = m_sFailureGroupElementId;
		member.m_sPrefab = prefab;
		member.m_sRole = "rifleman";
		member.m_iOrdinal = ordinal;
		member.m_iHRCost = 0;
		member.m_bRequired = true;
		return member;
	}

	protected HST_ActiveGroupState BuildActiveGroup(string forceId, string projectionId, string resultId, HST_CampaignState state)
	{
		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = projectionId;
		group.m_sOperationId = m_sOperationId;
		group.m_sManifestId = m_sManifestId;
		group.m_sSpawnResultId = resultId;
		group.m_sForceId = forceId;
		group.m_sProjectionId = projectionId;
		group.m_sFactionKey = "FIA";
		group.m_sPrefab = GROUP_PREFAB;
		group.m_sCompositionIntentId = "campaign_debug_spawn_adapter";
		group.m_sRuntimeStatus = "campaign_debug_exact_spawn_queued";
		group.m_vPosition = BuildFixturePosition(state, projectionId == m_sSuccessProjectionId);
		group.m_vSourcePosition = group.m_vPosition;
		group.m_vTargetPosition = group.m_vPosition;
		group.m_iInfantryCount = FIXTURE_MEMBER_COUNT;
		group.m_iOriginalInfantryCount = FIXTURE_MEMBER_COUNT;
		group.m_iLastSeenAliveCount = FIXTURE_MEMBER_COUNT;
		group.m_iSurvivorInfantryCount = FIXTURE_MEMBER_COUNT;
		return group;
	}

	protected HST_ActiveGroupState BuildFailureActiveGroup(HST_CampaignState state)
	{
		HST_ActiveGroupState group = BuildActiveGroup(m_sFailureForceId, m_sFailureProjectionId, m_sFailureResultId, state);
		group.m_sOperationId = m_sFailureOperationId;
		group.m_sManifestId = m_sFailureManifestId;
		vector position = BuildFixturePosition(state, true);
		position[0] = position[0] + 8.0;
		group.m_vPosition = position;
		group.m_vSourcePosition = group.m_vPosition;
		group.m_vTargetPosition = group.m_vPosition;
		return group;
	}

	protected vector BuildFixturePosition(HST_CampaignState state, bool successProjection)
	{
		vector position;
		if (state)
			position = state.m_vHQPosition;
		position[0] = position[0] + 28.0;
		position[2] = position[2] + 12.0;
		if (successProjection)
			position[2] = position[2] + 8.0;
		return position;
	}

	protected HST_ForceSpawnQueueRequest BuildRequest(
		string resultId,
		string requestId,
		string forceId,
		string projectionId,
		int nowSecond,
		int priority)
	{
		HST_ForceSpawnQueueRequest request = new HST_ForceSpawnQueueRequest();
		request.m_sResultId = resultId;
		request.m_sRequestId = requestId;
		request.m_sForceId = forceId;
		request.m_sProjectionId = projectionId;
		request.m_iPriority = priority;
		request.m_iMaxRetries = 0;
		request.m_iDeadlineSecond = nowSecond + 120;
		return request;
	}

	protected void CaptureRootBeforeMembers(
		HST_CampaignState state,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar)
	{
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(m_sCancelResultId);
		HST_ActiveGroupState group = FindActiveGroupByProjection(state, m_sCancelProjectionId);
		HST_ForceSpawnSlotResultState rootSlot;
		HST_ForceSpawnSlotResultState firstMember;
		HST_ForceSpawnSlotResultState secondMember;
		if (batch)
		{
			rootSlot = batch.FindSlotResult(m_sGroupElementId);
			firstMember = batch.FindSlotResult(m_sFirstMemberSlotId);
			secondMember = batch.FindSlotResult(m_sSecondMemberSlotId);
		}
		SCR_AIGroup root;
		int runtimeMembers;
		if (group && physicalWar)
		{
			root = physicalWar.GetForceSpawnGroupRoot(group);
			runtimeMembers = physicalWar.CountForceSpawnRuntimeMembers(group);
		}
		m_bRootBeforeMembers = rootSlot && rootSlot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
		m_bRootBeforeMembers = m_bRootBeforeMembers && IsQueuedSlot(firstMember) && IsQueuedSlot(secondMember);
		m_bRootBeforeMembers = m_bRootBeforeMembers && root && runtimeMembers == 0;
		m_bRootBeforeMembers = m_bRootBeforeMembers && adapter.CountHandlesForProjection(m_sCancelProjectionId) == 1;
		m_sCancelEvidence = string.Format(
			"root-before-members %1 | batch %2 | root %3 | members %4 | bindings %5",
			m_bRootBeforeMembers,
			ResolveBatchStatus(batch),
			root != null,
			runtimeMembers,
			adapter.CountHandlesForProjection(m_sCancelProjectionId));
	}

	protected void CaptureCancelledProjection(
		HST_CampaignState state,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar)
	{
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(m_sCancelResultId);
		HST_ActiveGroupState group = FindActiveGroupByProjection(state, m_sCancelProjectionId);
		SCR_AIGroup root;
		int runtimeMembers;
		if (group)
		{
			root = physicalWar.GetForceSpawnGroupRoot(group);
			runtimeMembers = physicalWar.CountForceSpawnRuntimeMembers(group);
		}
		bool terminal = batch && batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED;
		bool cleared = adapter.CountHandlesForProjection(m_sCancelProjectionId) == 0 && !root && runtimeMembers == 0;
		m_bCancelCleanupExact = m_bRootBeforeMembers && terminal && cleared && CancelledSlotsCleared(batch);
		m_sCancelEvidence = m_sCancelEvidence + string.Format(
			" | terminal-cancelled %1 | cleared %2 | members %3 | bindings %4",
			terminal,
			cleared,
			runtimeMembers,
			adapter.CountHandlesForProjection(m_sCancelProjectionId));
	}

	protected void CaptureSuccessfulProjection(
		HST_CampaignState state,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar)
	{
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(m_sSuccessResultId);
		HST_ActiveGroupState group = FindActiveGroupByProjection(state, m_sSuccessProjectionId);
		HST_ForceSpawnSlotResultState rootSlot;
		HST_ForceSpawnSlotResultState firstMember;
		HST_ForceSpawnSlotResultState secondMember;
		if (batch)
		{
			rootSlot = batch.FindSlotResult(m_sGroupElementId);
			firstMember = batch.FindSlotResult(m_sFirstMemberSlotId);
			secondMember = batch.FindSlotResult(m_sSecondMemberSlotId);
		}
		CaptureDurableSuccess(batch, rootSlot, firstMember, secondMember);
		CaptureRuntimeSuccess(group, rootSlot, firstMember, secondMember, adapter, physicalWar);
		CaptureActiveGroupHandoff(group, adapter, physicalWar);
	}

	protected void CaptureDurableSuccess(
		HST_ForceSpawnResultState batch,
		HST_ForceSpawnSlotResultState rootSlot,
		HST_ForceSpawnSlotResultState firstMember,
		HST_ForceSpawnSlotResultState secondMember)
	{
		bool terminal = batch && batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
		bool exactCount = batch && batch.m_iExpectedSlotCount == 3 && batch.m_aSlotResults.Count() == 3;
		m_bExactPrefabs = IsExactRegisteredSlot(rootSlot, GROUP_PREFAB);
		m_bExactPrefabs = m_bExactPrefabs && IsExactRegisteredSlot(firstMember, MEMBER_PREFAB);
		m_bExactPrefabs = m_bExactPrefabs && IsExactRegisteredSlot(secondMember, MEMBER_PREFAB);
		bool uniqueEntities = SlotsHaveUniqueEntities(rootSlot, firstMember, secondMember);
		bool groupAuthority = SlotsShareNativeGroup(rootSlot, firstMember, secondMember);
		m_bDurableSuccessExact = terminal && exactCount && m_bExactPrefabs && uniqueEntities && groupAuthority;
		int slotCount;
		if (batch)
			slotCount = batch.m_aSlotResults.Count();
		m_sDurableEvidence = string.Format(
			"status %1 | slots %2/3 | exact prefabs %3 | unique entities %4 | native group %5",
			ResolveBatchStatus(batch),
			slotCount,
			m_bExactPrefabs,
			uniqueEntities,
			groupAuthority);
	}

	protected void CaptureRuntimeSuccess(
		HST_ActiveGroupState group,
		HST_ForceSpawnSlotResultState rootSlot,
		HST_ForceSpawnSlotResultState firstMember,
		HST_ForceSpawnSlotResultState secondMember,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar)
	{
		SCR_AIGroup root;
		if (group)
			root = physicalWar.GetForceSpawnGroupRoot(group);
		array<AIAgent> agents = {};
		if (root)
			root.GetAgents(agents);
		bool entityIdsExact;
		bool prefabsExact;
		bool nativeExact;
		bool editableExact;
		InspectRuntimeAgents(group, root, agents, rootSlot, firstMember, secondMember, physicalWar, entityIdsExact, prefabsExact, nativeExact, editableExact);
		SCR_EditableGroupComponent editableGroup;
		if (root)
			editableGroup = SCR_EditableGroupComponent.Cast(root.FindComponent(SCR_EditableGroupComponent));
		int editableSize;
		if (editableGroup)
			editableSize = editableGroup.GetSize();
		m_bNativeMembershipExact = root && agents.Count() == FIXTURE_MEMBER_COUNT && nativeExact && entityIdsExact;
		m_bGameMasterMembershipExact = editableGroup && editableSize == FIXTURE_MEMBER_COUNT && editableExact;
		m_bExactPrefabs = m_bExactPrefabs && prefabsExact;
		m_sRuntimeEvidence = string.Format(
			"root %1 | agents %2 | native exact %3 | entity ids %4 | editable size %5 | parents %6 | bindings %7",
			root != null,
			agents.Count(),
			nativeExact,
			entityIdsExact,
			editableSize,
			editableExact,
			adapter.CountHandlesForProjection(m_sSuccessProjectionId));
	}

	protected void InspectRuntimeAgents(
		HST_ActiveGroupState group,
		SCR_AIGroup root,
		array<AIAgent> agents,
		HST_ForceSpawnSlotResultState rootSlot,
		HST_ForceSpawnSlotResultState firstMember,
		HST_ForceSpawnSlotResultState secondMember,
		HST_PhysicalWarService physicalWar,
		out bool entityIdsExact,
		out bool prefabsExact,
		out bool nativeExact,
		out bool editableExact)
	{
		entityIdsExact = root && rootSlot && rootSlot.m_sEntityId == ResolveEntityId(root);
		prefabsExact = root && ResolveEntityPrefab(root) == GROUP_PREFAB;
		nativeExact = root != null;
		editableExact = root != null;
		array<string> expectedMemberIds = {};
		if (firstMember)
			expectedMemberIds.Insert(firstMember.m_sEntityId);
		if (secondMember)
			expectedMemberIds.Insert(secondMember.m_sEntityId);
		SCR_EditableGroupComponent editableGroup;
		if (root)
			editableGroup = SCR_EditableGroupComponent.Cast(root.FindComponent(SCR_EditableGroupComponent));
		foreach (AIAgent agent : agents)
		{
			IEntity member;
			if (agent)
				member = agent.GetControlledEntity();
			string memberId = ResolveEntityId(member);
			entityIdsExact = entityIdsExact && !memberId.IsEmpty() && expectedMemberIds.Contains(memberId);
			prefabsExact = prefabsExact && ResolveEntityPrefab(member) == MEMBER_PREFAB;
			nativeExact = nativeExact && agent && agent.GetParentGroup() == root;
			nativeExact = nativeExact && member && SCR_AIDamageHandling.IsAlive(member);
			nativeExact = nativeExact && group && physicalWar.IsForceSpawnRuntimeHandleRegistered(group, member);
			SCR_EditableEntityComponent editableMember = SCR_EditableEntityComponent.GetEditableEntity(member);
			editableExact = editableExact && editableGroup && editableMember;
			if (editableMember)
				editableExact = editableExact && editableMember.GetParentEntity() == editableGroup;
			else
				editableExact = false;
		}
	}

	protected void CaptureActiveGroupHandoff(
		HST_ActiveGroupState group,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar)
	{
		int runtimeMembers;
		bool rootReady;
		if (group)
		{
			runtimeMembers = physicalWar.CountForceSpawnRuntimeMembers(group);
			rootReady = physicalWar.GetForceSpawnGroupRoot(group) != null;
		}
		m_bActiveGroupHandoffExact = group && group.m_sForceId == m_sSuccessForceId;
		m_bActiveGroupHandoffExact = m_bActiveGroupHandoffExact && group.m_sProjectionId == m_sSuccessProjectionId;
		m_bActiveGroupHandoffExact = m_bActiveGroupHandoffExact && group.m_sSpawnResultId == m_sSuccessResultId;
		m_bActiveGroupHandoffExact = m_bActiveGroupHandoffExact && group.m_bSpawnedEntity;
		m_bActiveGroupHandoffExact = m_bActiveGroupHandoffExact && group.m_iSpawnedAgentCount == FIXTURE_MEMBER_COUNT;
		m_bActiveGroupHandoffExact = m_bActiveGroupHandoffExact && runtimeMembers == FIXTURE_MEMBER_COUNT && rootReady;
		m_bActiveGroupHandoffExact = m_bActiveGroupHandoffExact && adapter.CountHandlesForProjection(m_sSuccessProjectionId) == 3;
		string groupId = "missing";
		string runtimeStatus = "missing";
		int spawnedAgentCount;
		if (group)
		{
			groupId = group.m_sGroupId;
			runtimeStatus = group.m_sRuntimeStatus;
			spawnedAgentCount = group.m_iSpawnedAgentCount;
		}
		m_sHandoffEvidence = string.Format(
			"group %1 | spawned %2 | agents %3/%4 | status %5 | root %6 | bindings %7",
			groupId,
			group && group.m_bSpawnedEntity,
			spawnedAgentCount,
			runtimeMembers,
			runtimeStatus,
			rootReady,
			adapter.CountHandlesForProjection(m_sSuccessProjectionId));
	}

	protected void CaptureRetirement(
		HST_CampaignState state,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar,
		HST_ForceSpawnAdapterRetireResult retirement)
	{
		HST_ActiveGroupState group = FindActiveGroupByProjection(state, m_sSuccessProjectionId);
		SCR_AIGroup root;
		int runtimeMembers;
		if (group)
		{
			root = physicalWar.GetForceSpawnGroupRoot(group);
			runtimeMembers = physicalWar.CountForceSpawnRuntimeMembers(group);
		}
		m_bRetirementExact = retirement && retirement.m_bSuccess;
		m_bRetirementExact = m_bRetirementExact && retirement.m_iRootCount == 1;
		m_bRetirementExact = m_bRetirementExact && retirement.m_iMemberCount == FIXTURE_MEMBER_COUNT;
		m_bRetirementExact = m_bRetirementExact && adapter.CountHandlesForProjection(m_sSuccessProjectionId) == 0;
		m_bRetirementExact = m_bRetirementExact && !root && runtimeMembers == 0;
		bool retirementSucceeded;
		int retiredMembers;
		int retiredRoots;
		string retirementFailure = "retirement result missing";
		if (retirement)
		{
			retirementSucceeded = retirement.m_bSuccess;
			retiredMembers = retirement.m_iMemberCount;
			retiredRoots = retirement.m_iRootCount;
			retirementFailure = retirement.m_sFailureReason;
		}
		m_sRetirementEvidence = string.Format(
			"success %1 | members %2/%3 | roots %4/1 | bindings %5 | runtime %6/%7 | failure %8",
			retirementSucceeded,
			retiredMembers,
			FIXTURE_MEMBER_COUNT,
			retiredRoots,
			adapter.CountHandlesForProjection(m_sSuccessProjectionId),
			root != null,
			runtimeMembers,
			retirementFailure);
	}

	protected string StartSameWaveFailureFixture(HST_CampaignState state, HST_ForceSpawnQueueService queue)
	{
		if (!state || !queue)
			return "failure-cleanup state or queue missing";
		HST_ForceManifestState manifest = BuildFailureManifest(state.m_iElapsedSeconds);
		state.m_aForceManifests.Insert(manifest);
		state.m_aActiveGroups.Insert(BuildFailureActiveGroup(state));
		HST_ForceSpawnQueueEnqueueResult enqueue = queue.Enqueue(
			state.m_aForceSpawnResults,
			manifest,
			BuildRequest(m_sFailureResultId, m_sFailureRequestId, m_sFailureForceId, m_sFailureProjectionId, state.m_iElapsedSeconds, 110),
			state.m_iElapsedSeconds);
		if (enqueue && enqueue.m_bSuccess)
			return "";

		RemoveActiveGroupByProjection(state, m_sFailureProjectionId);
		RemoveManifestById(state, m_sFailureManifestId);
		return "failure-cleanup enqueue failed: " + ResolveEnqueueFailure(enqueue);
	}

	protected void CaptureSameWaveFailurePending(
		HST_CampaignState state,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar)
	{
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(m_sFailureResultId);
		HST_ActiveGroupState group = FindActiveGroupByProjection(state, m_sFailureProjectionId);
		HST_ForceSpawnSlotResultState rootSlot;
		HST_ForceSpawnSlotResultState firstMember;
		HST_ForceSpawnSlotResultState secondMember;
		if (batch)
		{
			rootSlot = batch.FindSlotResult(m_sFailureGroupElementId);
			firstMember = batch.FindSlotResult(m_sFailureFirstMemberSlotId);
			secondMember = batch.FindSlotResult(m_sFailureSecondMemberSlotId);
		}
		SCR_AIGroup root;
		int runtimeMembers;
		if (group && physicalWar)
		{
			root = physicalWar.GetForceSpawnGroupRoot(group);
			runtimeMembers = physicalWar.CountForceSpawnRuntimeMembers(group);
		}
		bool cleanupPending = batch && batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING;
		bool bothMembersAcquired = firstMember && secondMember;
		bothMembersAcquired = bothMembersAcquired && firstMember.m_iAttemptCount == 1 && secondMember.m_iAttemptCount == 1;
		bool memberEvidenceAbsent = SlotCleanupPendingWithoutPhysicalEvidence(firstMember);
		memberEvidenceAbsent = memberEvidenceAbsent && SlotCleanupPendingWithoutPhysicalEvidence(secondMember);
		bool realRootHeld = rootSlot && rootSlot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING;
		realRootHeld = realRootHeld && root && rootSlot.m_sEntityId == ResolveEntityId(root);
		realRootHeld = realRootHeld && rootSlot.m_sSpawnedPrefab == GROUP_PREFAB;
		m_bSameWaveFailurePendingExact = cleanupPending && bothMembersAcquired && memberEvidenceAbsent && realRootHeld;
		m_bSameWaveFailurePendingExact = m_bSameWaveFailurePendingExact && runtimeMembers == 0;
		m_bSameWaveFailurePendingExact = m_bSameWaveFailurePendingExact && adapter && adapter.CountHandlesForProjection(m_sFailureProjectionId) == 1;
		int firstAttemptCount;
		int secondAttemptCount;
		int bindingCount;
		if (firstMember)
			firstAttemptCount = firstMember.m_iAttemptCount;
		if (secondMember)
			secondAttemptCount = secondMember.m_iAttemptCount;
		if (adapter)
			bindingCount = adapter.CountHandlesForProjection(m_sFailureProjectionId);
		m_sFailureCleanupEvidence = string.Format(
			"same-wave pending %1 | status %2 | acquired %3/%4 | member evidence absent %5 | real root %6 | runtime members %7 | bindings %8",
			m_bSameWaveFailurePendingExact,
			ResolveBatchStatus(batch),
			firstAttemptCount,
			secondAttemptCount,
			memberEvidenceAbsent,
			realRootHeld,
			runtimeMembers,
			bindingCount);
	}

	protected void CaptureSameWaveFailureFinal(
		HST_CampaignState state,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar)
	{
		if (!state || m_sFailureResultId.IsEmpty())
			return;
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(m_sFailureResultId);
		HST_ActiveGroupState group = FindActiveGroupByProjection(state, m_sFailureProjectionId);
		SCR_AIGroup root;
		int runtimeMembers;
		if (group && physicalWar)
		{
			root = physicalWar.GetForceSpawnGroupRoot(group);
			runtimeMembers = physicalWar.CountForceSpawnRuntimeMembers(group);
		}
		bool terminal = batch && batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
		bool slotsCleared = FailedSlotsCleared(batch);
		bool runtimeCleared = !root && runtimeMembers == 0;
		runtimeCleared = runtimeCleared && adapter && adapter.CountHandlesForProjection(m_sFailureProjectionId) == 0;
		bool handoffFailed = group && !group.m_bSpawnedEntity && group.m_sRuntimeStatus == "spawn_failed";
		handoffFailed = handoffFailed && group.m_iSpawnedAgentCount == 0;
		m_bSameWaveFailureCleanupExact = m_bSameWaveFailurePendingExact && terminal && slotsCleared && runtimeCleared && handoffFailed;
		int bindingCount;
		if (adapter)
			bindingCount = adapter.CountHandlesForProjection(m_sFailureProjectionId);
		m_sFailureCleanupEvidence = m_sFailureCleanupEvidence + string.Format(
			" | next-tick final %1 | slots cleared %2 | runtime %3/%4 | bindings %5 | group failed %6",
			terminal,
			slotsCleared,
			root != null,
			runtimeMembers,
			bindingCount,
			handoffFailed);
	}

	protected bool SlotCleanupPendingWithoutPhysicalEvidence(HST_ForceSpawnSlotResultState slot)
	{
		return slot
			&& slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CLEANUP_PENDING
			&& slot.m_sSpawnedPrefab.IsEmpty()
			&& slot.m_sEntityId.IsEmpty()
			&& slot.m_sNativeGroupId.IsEmpty();
	}

	protected bool FailedSlotsCleared(HST_ForceSpawnResultState batch)
	{
		if (!batch || batch.m_aSlotResults.Count() != 3)
			return false;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_FINAL)
				return false;
			if (!slot.m_sSpawnedPrefab.IsEmpty() || !slot.m_sEntityId.IsEmpty() || !slot.m_sNativeGroupId.IsEmpty())
				return false;
		}
		return true;
	}

	protected bool IsExactRegisteredSlot(HST_ForceSpawnSlotResultState slot, string prefab)
	{
		return slot
			&& slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
			&& slot.m_sSpawnedPrefab == prefab
			&& !slot.m_sEntityId.IsEmpty()
			&& slot.m_bAliveVerified
			&& slot.m_bFactionVerified
			&& slot.m_bGroupVerified
			&& slot.m_bProjectionVerified
			&& slot.m_bGameMasterVerified;
	}

	protected bool SlotsHaveUniqueEntities(
		HST_ForceSpawnSlotResultState rootSlot,
		HST_ForceSpawnSlotResultState firstMember,
		HST_ForceSpawnSlotResultState secondMember)
	{
		if (!rootSlot || !firstMember || !secondMember)
			return false;
		if (rootSlot.m_sEntityId.IsEmpty() || firstMember.m_sEntityId.IsEmpty() || secondMember.m_sEntityId.IsEmpty())
			return false;
		return rootSlot.m_sEntityId != firstMember.m_sEntityId
			&& rootSlot.m_sEntityId != secondMember.m_sEntityId
			&& firstMember.m_sEntityId != secondMember.m_sEntityId;
	}

	protected bool SlotsShareNativeGroup(
		HST_ForceSpawnSlotResultState rootSlot,
		HST_ForceSpawnSlotResultState firstMember,
		HST_ForceSpawnSlotResultState secondMember)
	{
		if (!rootSlot || !firstMember || !secondMember || rootSlot.m_sNativeGroupId.IsEmpty())
			return false;
		return firstMember.m_sNativeGroupId == rootSlot.m_sNativeGroupId
			&& secondMember.m_sNativeGroupId == rootSlot.m_sNativeGroupId;
	}

	protected bool CancelledSlotsCleared(HST_ForceSpawnResultState batch)
	{
		if (!batch || batch.m_aSlotResults.Count() != 3)
			return false;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CANCELLED)
				return false;
			if (!slot.m_sEntityId.IsEmpty() || !slot.m_sNativeGroupId.IsEmpty())
				return false;
		}
		return true;
	}

	protected bool IsQueuedSlot(HST_ForceSpawnSlotResultState slot)
	{
		return slot && slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED;
	}

	protected int RetireSucceededProjection(
		HST_CampaignState state,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar,
		string projectionId)
	{
		if (!state || !adapter || !physicalWar || projectionId.IsEmpty())
			return 0;
		HST_ActiveGroupState group = FindActiveGroupByProjection(state, projectionId);
		if (!group)
			return 0;
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(group.m_sSpawnResultId);
		if (!batch || batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			return 0;
		HST_ForceSpawnAdapterRetireResult retirement = adapter.RetireProjectionRuntime(state, physicalWar, projectionId);
		if (!retirement || !retirement.m_bSuccess)
			return 0;
		return retirement.m_iMemberCount + retirement.m_iRootCount;
	}

	protected void RequestFixtureCancellation(HST_CampaignState state, HST_ForceSpawnQueueService queue, string resultId)
	{
		if (!state || !queue || resultId.IsEmpty())
			return;
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(resultId);
		if (!batch || IsTerminalBatch(batch))
			return;
		queue.RequestCancel(state.m_aForceSpawnResults, resultId, state.m_iElapsedSeconds, "campaign debug exact adapter cleanup");
	}

	protected int DrainFixtureCleanup(
		HST_CampaignState state,
		HST_ForceSpawnQueueService queue,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar)
	{
		if (!state || !queue || !adapter || !physicalWar)
			return 0;
		int cleaned;
		for (int attempt = 0; attempt < CLEANUP_TICK_LIMIT; attempt++)
		{
			if (!FixtureNeedsQueueDrain(state, adapter))
				break;
			HST_ForceSpawnAdapterTickResult tick = adapter.Tick(state, queue, physicalWar, state.m_iElapsedSeconds);
			if (tick)
				cleaned += tick.m_iCleanedCount;
		}
		return cleaned;
	}

	protected bool FixtureNeedsQueueDrain(HST_CampaignState state, HST_ForceSpawnAdapterService adapter)
	{
		if (!state || !adapter)
			return false;
		HST_ForceSpawnResultState cancelBatch = state.FindForceSpawnResult(m_sCancelResultId);
		HST_ForceSpawnResultState successBatch = state.FindForceSpawnResult(m_sSuccessResultId);
		HST_ForceSpawnResultState failureBatch = state.FindForceSpawnResult(m_sFailureResultId);
		if (cancelBatch && !IsTerminalBatch(cancelBatch))
			return true;
		if (successBatch && !IsTerminalBatch(successBatch))
			return true;
		if (failureBatch && !IsTerminalBatch(failureBatch))
			return true;
		return adapter.CountHandlesForProjection(m_sCancelProjectionId) > 0
			|| adapter.CountHandlesForProjection(m_sSuccessProjectionId) > 0
			|| adapter.CountHandlesForProjection(m_sFailureProjectionId) > 0;
	}

	protected int CleanupPhysicalFallback(
		HST_CampaignState state,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar,
		string projectionId)
	{
		if (!physicalWar || projectionId.IsEmpty())
			return 0;
		HST_ActiveGroupState group = FindActiveGroupByProjection(state, projectionId);
		if (group)
			physicalWar.ReleaseForceSpawnRuntimeOwnership(group);
		bool cleaned = physicalWar.CleanupRuntimeGroupEntityForDebug(projectionId);
		int pruned;
		if (adapter)
			pruned = adapter.PruneDeletedProjectionBindings(projectionId);
		if (cleaned)
			return 1 + pruned;
		return pruned;
	}

	protected void RemoveFixtureRows(
		HST_CampaignState state,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar)
	{
		if (!state || !adapter)
			return;
		bool cancelCleared = ProjectionBindingsCleared(adapter, m_sCancelProjectionId);
		bool successCleared = ProjectionBindingsCleared(adapter, m_sSuccessProjectionId);
		bool failureCleared = ProjectionBindingsCleared(adapter, m_sFailureProjectionId);
		if (cancelCleared)
		{
			ReleaseFixtureOwnership(state, physicalWar, m_sCancelProjectionId);
			RemoveActiveGroupByProjection(state, m_sCancelProjectionId);
			RemoveBatchById(state, m_sCancelResultId);
		}
		if (successCleared)
		{
			ReleaseFixtureOwnership(state, physicalWar, m_sSuccessProjectionId);
			RemoveActiveGroupByProjection(state, m_sSuccessProjectionId);
			RemoveBatchById(state, m_sSuccessResultId);
		}
		if (failureCleared)
		{
			ReleaseFixtureOwnership(state, physicalWar, m_sFailureProjectionId);
			RemoveActiveGroupByProjection(state, m_sFailureProjectionId);
			RemoveBatchById(state, m_sFailureResultId);
			RemoveManifestById(state, m_sFailureManifestId);
		}
		if (cancelCleared && successCleared)
			RemoveManifestById(state, m_sManifestId);
	}

	protected bool ProjectionBindingsCleared(HST_ForceSpawnAdapterService adapter, string projectionId)
	{
		if (projectionId.IsEmpty())
			return true;
		return adapter && adapter.CountHandlesForProjection(projectionId) == 0;
	}

	protected void ReleaseFixtureOwnership(HST_CampaignState state, HST_PhysicalWarService physicalWar, string projectionId)
	{
		if (!physicalWar)
			return;
		HST_ActiveGroupState group = FindActiveGroupByProjection(state, projectionId);
		if (group)
			physicalWar.ReleaseForceSpawnRuntimeOwnership(group);
	}

	protected void RemoveActiveGroupByProjection(HST_CampaignState state, string projectionId)
	{
		if (!state || projectionId.IsEmpty())
			return;
		for (int index = state.m_aActiveGroups.Count() - 1; index >= 0; index--)
		{
			HST_ActiveGroupState group = state.m_aActiveGroups[index];
			if (group && group.m_sProjectionId == projectionId)
				state.m_aActiveGroups.Remove(index);
		}
	}

	protected void RemoveBatchById(HST_CampaignState state, string resultId)
	{
		if (!state || resultId.IsEmpty())
			return;
		for (int index = state.m_aForceSpawnResults.Count() - 1; index >= 0; index--)
		{
			HST_ForceSpawnResultState batch = state.m_aForceSpawnResults[index];
			if (batch && batch.m_sResultId == resultId)
				state.m_aForceSpawnResults.Remove(index);
		}
	}

	protected void RemoveManifestById(HST_CampaignState state, string manifestId)
	{
		if (!state || manifestId.IsEmpty())
			return;
		for (int index = state.m_aForceManifests.Count() - 1; index >= 0; index--)
		{
			HST_ForceManifestState manifest = state.m_aForceManifests[index];
			if (manifest && manifest.m_sManifestId == manifestId)
				state.m_aForceManifests.Remove(index);
		}
	}

	protected void CaptureIsolationResult(HST_CampaignState state, HST_ForceSpawnAdapterService adapter)
	{
		int manifests;
		int batches;
		int groups;
		int bindings;
		if (state)
		{
			manifests = state.m_aForceManifests.Count();
			batches = state.m_aForceSpawnResults.Count();
			groups = state.m_aActiveGroups.Count();
		}
		if (adapter)
			bindings = adapter.CountBindings();
		bool fixtureAbsent = !FindActiveGroupByProjection(state, m_sCancelProjectionId);
		fixtureAbsent = fixtureAbsent && !FindActiveGroupByProjection(state, m_sSuccessProjectionId);
		fixtureAbsent = fixtureAbsent && !FindActiveGroupByProjection(state, m_sFailureProjectionId);
		fixtureAbsent = fixtureAbsent && (!state || !state.FindForceSpawnResult(m_sCancelResultId));
		fixtureAbsent = fixtureAbsent && (!state || !state.FindForceSpawnResult(m_sSuccessResultId));
		fixtureAbsent = fixtureAbsent && (!state || !state.FindForceSpawnResult(m_sFailureResultId));
		fixtureAbsent = fixtureAbsent && (!state || !state.FindForceManifest(m_sManifestId));
		fixtureAbsent = fixtureAbsent && (!state || !state.FindForceManifest(m_sFailureManifestId));
		m_bStateIsolationExact = fixtureAbsent;
		m_bStateIsolationExact = m_bStateIsolationExact && manifests == m_iManifestCountBefore;
		m_bStateIsolationExact = m_bStateIsolationExact && batches == m_iBatchCountBefore;
		m_bStateIsolationExact = m_bStateIsolationExact && groups == m_iActiveGroupCountBefore;
		m_bStateIsolationExact = m_bStateIsolationExact && bindings == m_iBindingCountBefore;
		m_sIsolationEvidence = string.Format(
			"fixture absent %1 | manifests %2/%3 | batches %4/%5 | groups %6/%7 | bindings %8/%9",
			fixtureAbsent,
			manifests,
			m_iManifestCountBefore,
			batches,
			m_iBatchCountBefore,
			groups,
			m_iActiveGroupCountBefore,
			bindings,
			m_iBindingCountBefore);
	}

	protected HST_ForceSpawnAdapterProofReport BuildReport()
	{
		HST_ForceSpawnAdapterProofReport report = new HST_ForceSpawnAdapterProofReport();
		report.m_bPrerequisiteReady = m_bPrerequisiteReady;
		report.m_bRootBeforeMembers = m_bRootBeforeMembers;
		report.m_bCancelCleanupExact = m_bCancelCleanupExact;
		report.m_bExactPrefabs = m_bExactPrefabs;
		report.m_bNativeMembershipExact = m_bNativeMembershipExact;
		report.m_bGameMasterMembershipExact = m_bGameMasterMembershipExact;
		report.m_bDurableSuccessExact = m_bDurableSuccessExact;
		report.m_bActiveGroupHandoffExact = m_bActiveGroupHandoffExact;
		report.m_bRetirementExact = m_bRetirementExact;
		report.m_bSameWaveFailureCleanupExact = m_bSameWaveFailureCleanupExact;
		report.m_bStateIsolationExact = m_bStateIsolationExact;
		report.m_sPrerequisiteEvidence = m_sPrerequisiteEvidence;
		report.m_sCancelEvidence = m_sCancelEvidence;
		report.m_sDurableEvidence = m_sDurableEvidence;
		report.m_sRuntimeEvidence = m_sRuntimeEvidence;
		report.m_sHandoffEvidence = m_sHandoffEvidence;
		report.m_sRetirementEvidence = m_sRetirementEvidence;
		report.m_sFailureCleanupEvidence = m_sFailureCleanupEvidence;
		report.m_sIsolationEvidence = m_sIsolationEvidence;
		return report;
	}

	protected HST_ActiveGroupState FindActiveGroupByProjection(HST_CampaignState state, string projectionId)
	{
		if (!state || projectionId.IsEmpty())
			return null;
		HST_ActiveGroupState found;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (!group || group.m_sProjectionId != projectionId)
				continue;
			if (found)
				return null;
			found = group;
		}
		return found;
	}

	protected bool IsTerminalBatch(HST_ForceSpawnResultState batch)
	{
		return batch && (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED);
	}

	protected string ResolveBatchStatus(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return "missing";
		return string.Format("%1", batch.m_eStatus);
	}

	protected string ResolveEnqueueFailure(HST_ForceSpawnQueueEnqueueResult result)
	{
		if (!result)
			return "result missing";
		if (result.m_sFailureReason.IsEmpty())
			return "unspecified enqueue failure";
		return result.m_sFailureReason;
	}

	protected string ResolveCallbackFailure(HST_ForceSpawnQueueCallbackResult result)
	{
		if (!result)
			return "result missing";
		if (result.m_sFailureReason.IsEmpty())
			return "unspecified callback failure";
		return result.m_sFailureReason;
	}

	protected string ResolveEntityPrefab(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return "";
		return entity.GetPrefabData().GetPrefabName();
	}

	protected string ResolveEntityId(IEntity entity)
	{
		if (!entity)
			return "";
		BaseRplComponent rpl = BaseRplComponent.Cast(entity.FindComponent(BaseRplComponent));
		if (!rpl || !rpl.Id().IsValid())
			return "";
		return string.Format("rpl_%1", rpl.Id());
	}

	protected bool IsZeroVector(vector value)
	{
		return Math.AbsFloat(value[0]) < 0.01 && Math.AbsFloat(value[1]) < 0.01 && Math.AbsFloat(value[2]) < 0.01;
	}

	protected void ResetObservations()
	{
		m_bPrerequisiteReady = false;
		m_bRootBeforeMembers = false;
		m_bCancelCleanupExact = false;
		m_bExactPrefabs = false;
		m_bNativeMembershipExact = false;
		m_bGameMasterMembershipExact = false;
		m_bDurableSuccessExact = false;
		m_bActiveGroupHandoffExact = false;
		m_bRetirementExact = false;
		m_bSameWaveFailurePendingExact = false;
		m_bSameWaveFailureCleanupExact = false;
		m_bStateIsolationExact = false;
		m_sPrerequisiteEvidence = "not started";
		m_sCancelEvidence = "not observed";
		m_sDurableEvidence = "not observed";
		m_sRuntimeEvidence = "not observed";
		m_sHandoffEvidence = "not observed";
		m_sRetirementEvidence = "not observed";
		m_sFailureCleanupEvidence = "not observed";
		m_sIsolationEvidence = "not cleaned";
	}
}
