class HST_EnemyGarrisonRebuildOperationProofReport
{
	bool m_bAdmissionCapacityExact;
	bool m_bDeliveryHeldExact;
	bool m_bCasualtyContinuityExact;
	bool m_bRestoreExact;
	bool m_bOwnershipTerminalExact;
	bool m_bAdmissionRollbackExact;
	bool m_bPrearrivalRefundExact;
	bool m_bSettlementCrashResumeExact;
	bool m_bHistoricalIsolationExact;
	bool m_bSchema70QuarantineExact;
	bool m_bOrphanRuntimeQuarantineExact;
	bool m_bQuarantineRetentionExact;
	bool m_bSelectedOwnershipABAExact;
	string m_sAdmissionEvidence;
	string m_sDeliveryEvidence;
	string m_sCasualtyEvidence;
	string m_sRestoreEvidence;
	string m_sOwnershipEvidence;
	string m_sAdmissionRollbackEvidence;
	string m_sPrearrivalRefundEvidence;
	string m_sSettlementCrashEvidence;
	string m_sHistoricalEvidence;
	string m_sQuarantineEvidence;
	string m_sOrphanRuntimeEvidence;
	string m_sRetentionEvidence;
	string m_sSelectedOwnershipABAEvidence;

	bool AllExact()
	{
		bool lifecycleExact = m_bAdmissionCapacityExact
			&& m_bDeliveryHeldExact
			&& m_bCasualtyContinuityExact
			&& m_bRestoreExact;
		bool settlementExact = m_bOwnershipTerminalExact
			&& m_bAdmissionRollbackExact
			&& m_bPrearrivalRefundExact
			&& m_bSettlementCrashResumeExact;
		bool safetyExact = m_bSchema70QuarantineExact
			&& m_bOrphanRuntimeQuarantineExact
			&& m_bQuarantineRetentionExact
			&& m_bSelectedOwnershipABAExact
			&& m_bHistoricalIsolationExact;
		return lifecycleExact && settlementExact && safetyExact;
	}

	string BuildReport()
	{
		string report = string.Format(
			"exact enemy garrison rebuild proof | all exact %1 | admission/capacity %2 | delivery/held %3 | casualty continuity %4 | restore %5",
			AllExact(),
			m_bAdmissionCapacityExact,
			m_bDeliveryHeldExact,
			m_bCasualtyContinuityExact,
			m_bRestoreExact);
		return report + string.Format(
			" | ownership terminal %1 | admission rollback %2 | prearrival refund %3 | crash resume %4 | historical isolation %5 | schema-70 quarantine/orphan/retention %6/%7/%8 | selected ownership ABA %9",
			m_bOwnershipTerminalExact,
			m_bAdmissionRollbackExact,
			m_bPrearrivalRefundExact,
			m_bSettlementCrashResumeExact,
			m_bHistoricalIsolationExact,
			m_bSchema70QuarantineExact,
			m_bOrphanRuntimeQuarantineExact,
			m_bQuarantineRetentionExact,
			m_bSelectedOwnershipABAExact);
	}
}

class HST_EnemyGarrisonRebuildOperationProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_BalanceConfig m_Balance;
	ref HST_GarrisonService m_Garrisons;
	ref HST_ForcePlanningService m_Planning;
	ref HST_ForceSpawnQueueService m_Queue;
	ref HST_ForceSpawnAdapterService m_Adapter;
	ref HST_PhysicalWarService m_PhysicalWar;
	ref HST_EnemyDirectorService m_EnemyDirector;
	ref HST_EnemyGarrisonRebuildOperationService m_Service;
	ref HST_EnemyOrderState m_Order;
	ref HST_ForceManifestState m_Manifest;
	ref HST_EnemyGarrisonRebuildAdmissionResult m_Admission;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
	ref HST_GarrisonState m_TargetGarrison;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	string m_sDebitReason;
	int m_iAggregateBefore;
	int m_iSupportBeforeDebit;
	int m_iSupportAfterDebit;
	bool m_bPreflightAccepted;
	bool m_bPreflightReadOnly;
	bool m_bDebitAccepted;
	string m_sFailureReason;
}

// Focused authority proofs must not inherit the current test player's world
// proximity. Production lifecycle transitions still run through the ordinary
// rebuild service; only the proximity input remains deterministically virtual.
class HST_EnemyGarrisonRebuildMaterializationProofHarness
	: HST_MaterializationService
{
	override HST_OperationProjectionDecision EvaluateExactEnemyCounterattack(
		HST_OperationRecordState operation,
		vector position)
	{
		return EvaluateExactEnemyCounterattackForProximity(
			operation,
			false,
			false,
			1800.0,
			2160.0);
	}
}

class HST_EnemyGarrisonRebuildOperationProofHarness
	: HST_EnemyGarrisonRebuildOperationService
{
	void UseDeterministicVirtualProjectionForProof()
	{
		m_Materialization
			= new HST_EnemyGarrisonRebuildMaterializationProofHarness();
	}
}

class HST_EnemyGarrisonRebuildOperationProofFixtureFactory
{
	static const string PROOF_FACTION_KEY = "US";
	static const string PROOF_OTHER_FACTION_KEY = "FIA";
	static const int PROOF_SUPPORT_COST = 10;
	static const int PROOF_AGGREGATE_INFANTRY = 2;

	HST_EnemyGarrisonRebuildOperationProofFixture BuildPreparedFixture(
		string suffix,
		bool debitResources = true)
	{
		HST_EnemyGarrisonRebuildOperationProofFixture fixture
			= new HST_EnemyGarrisonRebuildOperationProofFixture();
		fixture.m_State = BuildState(suffix);
		fixture.m_Preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		fixture.m_Balance = new HST_BalanceConfig();
		fixture.m_Balance.m_bPopulationOutcomeEnabled = false;
		fixture.m_Balance.m_bLegacyControlVictoryEnabled = false;
		fixture.m_Balance.m_bLossConditionEnabled = false;
		HST_DefaultCatalog.AddDefaultFactionPools(
			fixture.m_State,
			fixture.m_Balance,
			fixture.m_Preset);

		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		if (!pool)
		{
			pool = new HST_FactionPoolState();
			pool.m_sFactionKey = PROOF_FACTION_KEY;
			fixture.m_State.m_aFactionPools.Insert(pool);
		}
		pool.m_iStrategicContractVersion
			= HST_EnemyStrategicResourceService.CONTRACT_VERSION;
		pool.m_iStrategicRevision = Math.Max(1, pool.m_iStrategicRevision);
		pool.m_iAttackResources = 500;
		pool.m_iSupportResources = 500;
		pool.m_iAggression = 50;

		fixture.m_sSourceZoneId = BuildSourceZoneId(suffix);
		fixture.m_sTargetZoneId = BuildTargetZoneId(suffix);
		fixture.m_Garrisons = new HST_GarrisonService();
		if (!fixture.m_Garrisons.AddAbstractForces(
			fixture.m_State,
			fixture.m_sTargetZoneId,
			PROOF_FACTION_KEY,
			PROOF_AGGREGATE_INFANTRY,
			0))
		{
			fixture.m_sFailureReason = "exact rebuild proof could not seed aggregate target security";
			return fixture;
		}
		fixture.m_TargetGarrison = fixture.m_State.FindGarrison(
			fixture.m_sTargetZoneId,
			PROOF_FACTION_KEY);
		fixture.m_iAggregateBefore = PROOF_AGGREGATE_INFANTRY;

		fixture.m_Queue = new HST_ForceSpawnQueueService();
		fixture.m_Adapter = new HST_ForceSpawnAdapterService();
		fixture.m_PhysicalWar = new HST_PhysicalWarService();
		fixture.m_EnemyDirector = new HST_EnemyDirectorService();
		fixture.m_EnemyDirector.SetCampaignPreset(fixture.m_Preset);
		fixture.m_Planning = new HST_ForcePlanningService();
		HST_EnemyGarrisonRebuildOperationProofHarness proofService
			= new HST_EnemyGarrisonRebuildOperationProofHarness();
		proofService.UseDeterministicVirtualProjectionForProof();
		fixture.m_Service = proofService;
		fixture.m_Service.SetRuntimeServices(
			fixture.m_Queue,
			fixture.m_Adapter,
			fixture.m_PhysicalWar,
			fixture.m_Garrisons);
		fixture.m_Service.SetEnemyDirectorService(fixture.m_EnemyDirector);

		fixture.m_Order = BuildOrder(fixture, suffix);
		HST_EnemyGarrisonRebuildManifestResult planned
			= fixture.m_Planning.PlanExactEnemyGarrisonRebuild(
				fixture.m_State,
				fixture.m_Preset,
				fixture.m_Order);
		if (!planned || !planned.m_bSuccess || !planned.m_Manifest)
		{
			fixture.m_sFailureReason = "exact rebuild proof planning failed";
			if (planned && !planned.m_sFailureReason.IsEmpty())
				fixture.m_sFailureReason = fixture.m_sFailureReason + ": "
					+ planned.m_sFailureReason;
			return fixture;
		}
		fixture.m_Manifest = planned.m_Manifest;
		fixture.m_Order.m_sManifestId = fixture.m_Manifest.m_sManifestId;
		fixture.m_Order.m_sManifestHash = fixture.m_Manifest.m_sManifestHash;
		fixture.m_Order.m_iCompositionManpower
			= fixture.m_Manifest.m_iAcceptedMemberCount;
		HST_ZoneState target = fixture.m_State.FindZone(fixture.m_sTargetZoneId);
		if (target)
			target.m_iGarrisonSlots = fixture.m_iAggregateBefore
				+ fixture.m_Manifest.m_iAcceptedMemberCount;

		int operationsBefore = fixture.m_State.m_aOperations.Count();
		int manifestsBefore = fixture.m_State.m_aForceManifests.Count();
		int batchesBefore = fixture.m_State.m_aForceSpawnResults.Count();
		int groupsBefore = fixture.m_State.m_aActiveGroups.Count();
		int ordersBefore = fixture.m_State.m_aEnemyOrders.Count();
		int supportBefore = pool.m_iSupportResources;
		HST_EnemyGarrisonRebuildAdmissionResult preflight
			= fixture.m_Service.CanAdmitPreparedOrder(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Manifest,
				fixture.m_EnemyDirector);
		fixture.m_bPreflightAccepted = preflight && preflight.m_bSuccess;
		fixture.m_bPreflightReadOnly
			= fixture.m_State.m_aOperations.Count() == operationsBefore
			&& fixture.m_State.m_aForceManifests.Count() == manifestsBefore
			&& fixture.m_State.m_aForceSpawnResults.Count() == batchesBefore
			&& fixture.m_State.m_aActiveGroups.Count() == groupsBefore
			&& fixture.m_State.m_aEnemyOrders.Count() == ordersBefore
			&& pool.m_iSupportResources == supportBefore;
		if (!fixture.m_bPreflightAccepted)
		{
			fixture.m_sFailureReason = "exact rebuild proof admission preflight failed";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				fixture.m_sFailureReason = fixture.m_sFailureReason + ": "
					+ preflight.m_sFailureReason;
			return fixture;
		}

		fixture.m_iSupportBeforeDebit = pool.m_iSupportResources;
		if (!debitResources)
			return fixture;
		fixture.m_Order.m_sResourceDebitMutationId
			= "enemy_resource_debit_" + fixture.m_Order.m_sOrderId;
		fixture.m_EnemyDirector.RecordZoneDamageSignal(
			fixture.m_State,
			PROOF_FACTION_KEY,
			target,
			100,
			"focused exact garrison rebuild proof pressure");
		fixture.m_bDebitAccepted = fixture.m_EnemyDirector.TrySpendDefense(
			fixture.m_State,
			target,
			PROOF_FACTION_KEY,
			0,
			PROOF_SUPPORT_COST,
			fixture.m_sDebitReason,
			fixture.m_Order.m_sResourceDebitMutationId,
			fixture.m_Order.m_sOrderId,
			fixture.m_Order.m_sOrderId,
			fixture.m_Order.m_sOperationId,
			fixture.m_Manifest.m_sManifestId);
		fixture.m_iSupportAfterDebit = pool.m_iSupportResources;
		if (!fixture.m_bDebitAccepted)
			fixture.m_sFailureReason = "exact rebuild proof debit failed: "
				+ fixture.m_sDebitReason;
		return fixture;
	}

	HST_EnemyGarrisonRebuildOperationProofFixture BuildAdmittedFixture(string suffix)
	{
		HST_EnemyGarrisonRebuildOperationProofFixture fixture
			= BuildPreparedFixture(suffix, true);
		if (!Prepared(fixture))
			return fixture;
		fixture.m_State.m_aEnemyOrders.Insert(fixture.m_Order);
		fixture.m_Admission = fixture.m_Service.AdmitPreparedOrder(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Manifest,
			fixture.m_EnemyDirector);
		if (!fixture.m_Admission || !fixture.m_Admission.m_bSuccess)
		{
			fixture.m_sFailureReason = "exact rebuild proof admission failed";
			if (fixture.m_Admission
				&& !fixture.m_Admission.m_sFailureReason.IsEmpty())
				fixture.m_sFailureReason = fixture.m_sFailureReason + ": "
					+ fixture.m_Admission.m_sFailureReason;
			return fixture;
		}
		fixture.m_Operation = fixture.m_Admission.m_Operation;
		fixture.m_Batch = fixture.m_Admission.m_Batch;
		fixture.m_Group = fixture.m_Admission.m_Group;
		fixture.m_TargetGarrison = fixture.m_State.FindGarrison(
			fixture.m_sTargetZoneId,
			PROOF_FACTION_KEY);
		return fixture;
	}

	bool Prepared(HST_EnemyGarrisonRebuildOperationProofFixture fixture)
	{
		if (!fixture)
			return false;
		bool campaignReady = fixture.m_State && fixture.m_Preset
			&& fixture.m_Garrisons && fixture.m_Planning;
		bool runtimeReady = fixture.m_Queue && fixture.m_Adapter
			&& fixture.m_PhysicalWar && fixture.m_EnemyDirector
			&& fixture.m_Service;
		bool authorityReady = fixture.m_Order && fixture.m_Manifest
			&& fixture.m_bPreflightAccepted && fixture.m_bPreflightReadOnly
			&& fixture.m_bDebitAccepted;
		return campaignReady && runtimeReady && authorityReady;
	}

	bool Ready(HST_EnemyGarrisonRebuildOperationProofFixture fixture)
	{
		return Prepared(fixture) && fixture.m_Admission
			&& fixture.m_Admission.m_bSuccess && fixture.m_Operation
			&& fixture.m_Batch && fixture.m_Group;
	}

	string Failure(HST_EnemyGarrisonRebuildOperationProofFixture fixture)
	{
		if (!fixture)
			return "exact enemy garrison rebuild proof fixture is unavailable";
		if (!fixture.m_sFailureReason.IsEmpty())
			return fixture.m_sFailureReason;
		return "exact enemy garrison rebuild proof fixture is incomplete";
	}

	HST_EnemyOrderState BuildOrder(
		HST_EnemyGarrisonRebuildOperationProofFixture fixture,
		string suffix)
	{
		if (!fixture || !fixture.m_State)
			return null;
		HST_EnemyOrderState order = new HST_EnemyOrderState();
		order.m_sOrderId = "enemy_garrison_rebuild_proof_" + suffix;
		order.m_sOperationId = HST_StableIdService.BuildOperationId(
			"enemy_order",
			order.m_sOrderId);
		order.m_iOperationContractVersion
			= HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_CONTRACT_VERSION;
		order.m_sFactionKey = PROOF_FACTION_KEY;
		order.m_eType
			= HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED;
		order.m_sSourceZoneId = fixture.m_sSourceZoneId;
		order.m_sTargetZoneId = fixture.m_sTargetZoneId;
		HST_ZoneState source = fixture.m_State.FindZone(fixture.m_sSourceZoneId);
		HST_ZoneState target = fixture.m_State.FindZone(fixture.m_sTargetZoneId);
		if (source)
			order.m_vSourcePosition = source.m_vPosition;
		if (target)
		{
			order.m_vTargetPosition = target.m_vPosition;
			order.m_iTargetOwnershipRevision = Math.Max(1, target.m_iOwnershipRevision);
		}
		order.m_iCreatedAtSecond = fixture.m_State.m_iElapsedSeconds;
		order.m_iAttackCost = 0;
		order.m_iSupportCost = PROOF_SUPPORT_COST;
		order.m_sRuntimeStatus = "proof_prepaid_pending";
		return order;
	}

	protected HST_CampaignState BuildState(string suffix)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iCampaignSeed = 707070;
		state.m_iElapsedSeconds = 200;
		state.m_iWarLevel = 3;
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		state.m_bHQDeployed = true;

		HST_ZoneState source = new HST_ZoneState();
		source.m_sZoneId = BuildSourceZoneId(suffix);
		source.m_sDisplayName = "Exact Rebuild Proof Source";
		source.m_sOwnerFactionKey = PROOF_FACTION_KEY;
		source.m_iOwnershipContractVersion
			= HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION;
		source.m_iOwnershipRevision = 1;
		source.m_eType = HST_EZoneType.HST_ZONE_OUTPOST;
		source.m_iGarrisonSlots = 32;
		source.m_vPosition = "4000 20 4000";
		state.m_aZones.Insert(source);

		HST_ZoneState target = new HST_ZoneState();
		target.m_sZoneId = BuildTargetZoneId(suffix);
		target.m_sDisplayName = "Exact Rebuild Proof Target";
		target.m_sOwnerFactionKey = PROOF_FACTION_KEY;
		target.m_iOwnershipContractVersion
			= HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION;
		target.m_iOwnershipRevision = 3;
		target.m_eType = HST_EZoneType.HST_ZONE_TOWN;
		target.m_iGarrisonSlots = 16;
		target.m_vPosition = "4300 20 4000";
		state.m_aZones.Insert(target);
		return state;
	}

	static string BuildSourceZoneId(string suffix)
	{
		return "enemy_garrison_rebuild_proof_source_" + suffix;
	}

	static string BuildTargetZoneId(string suffix)
	{
		return "enemy_garrison_rebuild_proof_target_" + suffix;
	}
}

// CanEnqueue remains the production read-only preflight. The mutating enqueue
// fails once so the proof exercises the admission rollback after the operation,
// manifest, and projected group have been staged but before authority commits.
class HST_EnemyGarrisonRebuildRejectEnqueueProofHarness
	: HST_ForceSpawnQueueService
{
	override HST_ForceSpawnQueueEnqueueResult Enqueue(
		array<ref HST_ForceSpawnResultState> batches,
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueRequest request,
		int nowSecond)
	{
		HST_ForceSpawnQueueEnqueueResult result
			= new HST_ForceSpawnQueueEnqueueResult();
		result.m_sFailureReason
			= "focused proof injected a post-preflight enqueue rejection";
		return result;
	}
}

class HST_EnemyGarrisonRebuildCrashWindowExpectation
{
	bool m_bSettledBeforeOrderTail;
	string m_sOrderId;
	string m_sOperationId;
	string m_sManifestId;
	string m_sBatchId;
	string m_sGroupId;
	string m_sSettlementId;
	string m_sSettlementKind;
	string m_sTerminalReason;
	string m_sDebitMutationId;
	string m_sRefundMutationId;
	HST_EEnemyOrderStatus m_eStaleOrderStatus;
	string m_sStaleRuntimeStatus;
	string m_sStaleResolutionKind;
	string m_sStaleFailureReason;
	int m_iStaleResolvedAtSecond;
	bool m_bStalePhysicalized;
	bool m_bStaleAbstractResolved;
	bool m_bStaleOutcomeApplied;
	int m_iAccepted;
	int m_iSurvivors;
	int m_iSupportRefund;
	int m_iExpectedSupport;
	int m_iSettledAtSecond;
	int m_iExpectedMutationCount;
}

class HST_EnemyGarrisonRebuildOrphanRuntimeExpectation
{
	static const string DURABLE_REASON
		= "Schema 70 orphan exact enemy garrison rebuild authority quarantined";
	string m_sOperationId;
	string m_sManifestId;
	string m_sBatchId;
	string m_sGroupId;
	string m_sLiveSlotId;
	string m_sCasualtySlotId;
	int m_iRestoreSecond;
	int m_iOperationRevisionBefore;
	int m_iBatchAttemptGenerationBefore;
	int m_iBatchLifecycleRevisionBefore;
	int m_iGroupLifecycleRevisionBefore;
}

class HST_EnemyGarrisonRebuildMalformedQuarantineExpectation
{
	string m_sOrderId;
	string m_sOperationId;
	string m_sManifestId;
	string m_sPrimaryBatchId;
	string m_sPrimaryGroupId;
	string m_sPrimaryProcessSlotId;
	string m_sSecondaryBatchId;
	string m_sSecondaryGroupId;
	string m_sSecondaryProcessSlotId;
	int m_iExpectedSupport;
}

class HST_EnemyGarrisonRebuildQueueEligibilityProofHarness
	: HST_ForceSpawnQueueService
{
	bool IsExecutable(
		HST_ForceSpawnResultState batch,
		HST_ForceManifestState manifest,
		int nowSecond)
	{
		return IsWorkEligible(batch, manifest, nowSecond);
	}
}

// This harness stops only at durable boundaries inside the production
// settlement path. Reconciliation itself always runs through an ordinary
// production service instance after a save copy and restore.
class HST_EnemyGarrisonRebuildCrashWindowProofHarness
	: HST_EnemyGarrisonRebuildOperationService
{
	static const string SETTLEMENT_KIND = "ownership_invalidated_survivors";
	static const string TERMINAL_REASON
		= "focused exact enemy garrison rebuild crash-window settlement";

	bool StageReceiptCrashWindow(
		HST_EnemyGarrisonRebuildOperationProofFixture fixture,
		bool settleOperationBeforeOrderTail)
	{
		if (!fixture || !fixture.m_State || !fixture.m_Order
			|| !fixture.m_Manifest || !fixture.m_Operation)
			return false;
		if (!PrepareRuntimeForTerminal(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Operation,
			fixture.m_Batch,
			fixture.m_Group))
			return false;
		int survivors = ResolveLivingRoster(
			fixture.m_Operation,
			fixture.m_Manifest,
			fixture.m_Batch,
			fixture.m_Group);
		string settlementFailureStage;
		string settlementFailureReason;
		if (!ApplyResourceSettlement(
			fixture.m_State,
			fixture.m_EnemyDirector,
			fixture.m_Order,
			fixture.m_Manifest,
			SETTLEMENT_KIND,
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
			survivors,
			false,
			false,
			TERMINAL_REASON,
			settlementFailureStage,
			settlementFailureReason))
			return false;
		if (!settleOperationBeforeOrderTail)
		{
			return fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
				&& fixture.m_Order.m_bResourceSettlementApplied;
		}
		HST_OperationTransitionResult settled
			= m_Operations.SettleExactEnemyGarrisonRebuild(
				fixture.m_State,
				fixture.m_Order,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED,
				fixture.m_Order.m_sResourceSettlementId,
				TERMINAL_REASON);
		return settled && settled.m_bAccepted && settled.m_Operation
			&& settled.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
	}
}

class HST_EnemyGarrisonRebuildPlanningProofHarness
	: HST_EnemyPlanningProofService
{
	static const string PROOF_SELECTED_TARGET_ZONE_ID = "proof_us_target";
	static const string PROOF_SELECTED_SOURCE_ZONE_ID = "proof_us_source";

	HST_CampaignState BuildSchema70RebuildState()
	{
		HST_CampaignState state = BuildExactState(180, true, 0);
		if (!state)
			return null;
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;
			zone.m_iOwnershipContractVersion
				= HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION;
			zone.m_iOwnershipRevision = 1;
			zone.m_iGarrisonSlots = 16;
			zone.m_iPriority = 0;
			zone.m_iIncomeValue = 0;
			zone.m_iResistanceCaptureProgress = 0;
			if (zone.m_sZoneId == PROOF_SELECTED_TARGET_ZONE_ID)
			{
				// Keep the owned, under-garrisoned target alone in the weighted
				// top band while still using production target/order selection.
				zone.m_eType = HST_EZoneType.HST_ZONE_TOWN;
				zone.m_iPriority = 100;
				zone.m_iIncomeValue = 96;
			}
			else if (zone.m_sZoneId == PROOF_SELECTED_SOURCE_ZONE_ID)
			{
				zone.m_eType = HST_EZoneType.HST_ZONE_TOWN;
			}
			else
			{
				zone.m_eType = HST_EZoneType.HST_ZONE_HIDEOUT;
			}
		}
		return state;
	}

	HST_CampaignPreset BuildSchema70Preset()
	{
		return BuildPreset();
	}

	bool MarkPressureAppliedForProof(HST_EnemyPlanningState planning)
	{
		HST_EnemyPlanningDecisionResult marked
			= m_Authority.MarkTargetPressureApplied(planning);
		return marked && marked.m_bAccepted;
	}
}

// Deterministic source-level proof. Native entity creation, live AI behavior,
// marker rendering, replication, profile I/O, and a real process restart remain
// separate packaged-runtime gates.
class HST_EnemyGarrisonRebuildOperationProofService
{
	protected ref HST_EnemyGarrisonRebuildOperationProofFixtureFactory m_Fixtures
		= new HST_EnemyGarrisonRebuildOperationProofFixtureFactory();

	HST_EnemyGarrisonRebuildOperationProofReport Run()
	{
		HST_EnemyGarrisonRebuildOperationProofReport report
			= new HST_EnemyGarrisonRebuildOperationProofReport();
		ProveAdmissionAndCapacity(report);
		ProveDeliveryAndHeldAuthority(report);
		ProvePhysicalVirtualCasualtyContinuity(report);
		ProveDeliveredRestore(report);
		ProveOwnershipTerminalSettlement(report);
		ProveAdmissionRollback(report);
		ProvePrearrivalSurvivorRefund(report);
		ProveSettlementCrashWindowResume(report);
		ProveHistoricalContractIsolation(report);
		ProveSchema70Quarantine(report);
		ProveOrphanRuntimeQuarantine(report);
		ProveSchema70QuarantineRetention(report);
		ProveSelectedOwnershipABA(report);
		return report;
	}

	protected void ProveAdmissionAndCapacity(
		HST_EnemyGarrisonRebuildOperationProofReport report)
	{
		HST_EnemyGarrisonRebuildOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("admission_capacity");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sAdmissionEvidence = m_Fixtures.Failure(fixture);
			return;
		}

		HST_ForcePlanningIntegrityService integrity
			= new HST_ForcePlanningIntegrityService();
		HST_StrategicMovementService movement = new HST_StrategicMovementService();
		bool manifestShapeExact = fixture.m_Manifest.m_bFrozen
			&& fixture.m_Manifest.m_aGroups.Count() == 1
			&& fixture.m_Manifest.m_aVehicles.Count() == 0
			&& fixture.m_Manifest.m_aAssets.Count() == 0
			&& fixture.m_Manifest.m_iAcceptedMemberCount > 0
			&& fixture.m_Manifest.m_iAcceptedMemberCount
				== fixture.m_Manifest.m_aMembers.Count();
		bool manifestIdentityExact = fixture.m_Manifest.m_sForceKind
				== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_FORCE_KIND
			&& fixture.m_Manifest.m_sPolicyId
				== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_POLICY_ID
			&& fixture.m_Manifest.m_sIntentId
				== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_MANIFEST_INTENT;
		bool manifestResourceExact = fixture.m_Manifest.m_iAttackResourceCost == 0
			&& fixture.m_Manifest.m_iSupportResourceCost
				== HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_SUPPORT_COST
			&& fixture.m_Manifest.m_sManifestHash
				== integrity.BuildManifestHash(fixture.m_Manifest);
		bool manifestExact = manifestShapeExact && manifestIdentityExact
			&& manifestResourceExact
			&& movement.IsSupportedExactInfantryManifest(fixture.m_Manifest);

		bool orderLinks = fixture.m_Order.m_sOperationId
			== fixture.m_Operation.m_sOperationId
			&& fixture.m_Order.m_sManifestId == fixture.m_Manifest.m_sManifestId
			&& fixture.m_Order.m_sSpawnResultId == fixture.m_Batch.m_sResultId
			&& fixture.m_Order.m_sGroupId == fixture.m_Group.m_sGroupId;
		bool operationLinks = fixture.m_Operation.m_sEnemyOrderId
			== fixture.m_Order.m_sOrderId
			&& fixture.m_Operation.m_sManifestId == fixture.m_Manifest.m_sManifestId
			&& fixture.m_Operation.m_sSpawnResultId == fixture.m_Batch.m_sResultId
			&& fixture.m_Operation.m_sGroupId == fixture.m_Group.m_sGroupId;
		bool batchLinks = fixture.m_Batch.m_sOperationId
			== fixture.m_Operation.m_sOperationId
			&& fixture.m_Batch.m_sManifestId == fixture.m_Manifest.m_sManifestId
			&& fixture.m_Batch.m_bStrategicProjectionHeld;
		bool groupLinks = fixture.m_Group.m_sOperationId
			== fixture.m_Operation.m_sOperationId
			&& fixture.m_Group.m_sManifestId == fixture.m_Manifest.m_sManifestId
			&& fixture.m_Group.m_sSpawnResultId == fixture.m_Batch.m_sResultId
			&& fixture.m_Group.m_bQRF;
		bool reciprocalLinks = operationLinks && batchLinks && groupLinks;
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool debitExact = pool
			&& fixture.m_iSupportAfterDebit
				== fixture.m_iSupportBeforeDebit
					- HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_SUPPORT_COST
			&& pool.m_iSupportResources == fixture.m_iSupportAfterDebit
			&& fixture.m_Order.m_iAttackCost == 0;

		int operationsBefore = fixture.m_State.m_aOperations.Count();
		int manifestsBefore = fixture.m_State.m_aForceManifests.Count();
		int batchesBefore = fixture.m_State.m_aForceSpawnResults.Count();
		int groupsBefore = fixture.m_State.m_aActiveGroups.Count();
		HST_EnemyGarrisonRebuildAdmissionResult replay
			= fixture.m_Service.AdmitPreparedOrder(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Manifest,
				fixture.m_EnemyDirector);
		bool replayAuthorityExact = replay && replay.m_bSuccess
			&& !replay.m_bStateChanged
			&& replay.m_Operation == fixture.m_Operation
			&& replay.m_Batch == fixture.m_Batch
			&& replay.m_Group == fixture.m_Group;
		bool replayCountsExact = fixture.m_State.m_aOperations.Count()
			== operationsBefore
			&& fixture.m_State.m_aForceManifests.Count() == manifestsBefore
			&& fixture.m_State.m_aForceSpawnResults.Count() == batchesBefore
			&& fixture.m_State.m_aActiveGroups.Count() == groupsBefore;
		bool replayExact = replayAuthorityExact && replayCountsExact;

		HST_EnemyOrderState capacityOrder = m_Fixtures.BuildOrder(
			fixture,
			"capacity_second");
		HST_EnemyGarrisonRebuildManifestResult capacityPlan
			= fixture.m_Planning.PlanExactEnemyGarrisonRebuild(
				fixture.m_State,
				fixture.m_Preset,
				capacityOrder);
		HST_ZoneState target = fixture.m_State.FindZone(fixture.m_sTargetZoneId);
		int authoritative = fixture.m_Garrisons.ResolveAuthoritativeZoneInfantry(
			fixture.m_State,
			fixture.m_sTargetZoneId,
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool capacityExact = target
			&& authoritative == target.m_iGarrisonSlots
			&& capacityPlan && !capacityPlan.m_bSuccess
			&& capacityPlan.m_sFailureReason.Contains("capacity");
		int targetSlots;
		if (target)
			targetSlots = target.m_iGarrisonSlots;

		report.m_bAdmissionCapacityExact = fixture.m_bPreflightReadOnly
			&& manifestExact && orderLinks && reciprocalLinks && debitExact
			&& replayExact && capacityExact;
		report.m_sAdmissionEvidence = string.Format(
			"preflight/read-only %1/%2 | frozen members %3 | support %4 -> %5 | reciprocal %6 | replay %7 | authoritative/slots %8/%9",
			fixture.m_bPreflightAccepted,
			fixture.m_bPreflightReadOnly,
			fixture.m_Manifest.m_iAcceptedMemberCount,
			fixture.m_iSupportBeforeDebit,
			fixture.m_iSupportAfterDebit,
			reciprocalLinks,
			replayExact,
			authoritative,
			targetSlots);
		report.m_sAdmissionEvidence += string.Format(
			" | second rejected %1",
			capacityPlan && !capacityPlan.m_bSuccess);
	}

	protected void ProveDeliveryAndHeldAuthority(
		HST_EnemyGarrisonRebuildOperationProofReport report)
	{
		HST_EnemyGarrisonRebuildOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("delivery_held");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sDeliveryEvidence = m_Fixtures.Failure(fixture);
			return;
		}
		int supportAfterDebit = fixture.m_iSupportAfterDebit;
		bool delivered = DriveUntilDelivered(fixture, 40);
		string deliveryFailureEvidence = BuildDeliveryFailureEvidence(fixture);
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		HST_GarrisonState garrison = fixture.m_State.FindGarrison(
			fixture.m_sTargetZoneId,
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		int living = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		int authoritative = fixture.m_Garrisons.ResolveAuthoritativeZoneInfantry(
			fixture.m_State,
			fixture.m_sTargetZoneId,
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool receiptExact = fixture.m_Order.m_bResourceSettlementApplied
			&& fixture.m_Order.m_sResourceSettlementKind
				== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_DELIVERY_SETTLEMENT_KIND
			&& fixture.m_Order.m_iSettlementAcceptedMemberCount
				== fixture.m_Manifest.m_iAcceptedMemberCount
			&& fixture.m_Order.m_iSettlementSurvivorMemberCount == living
			&& fixture.m_Order.m_iRefundedAttackResources == 0
			&& fixture.m_Order.m_iRefundedSupportResources == 0;
		bool heldLifecycleExact = delivered && fixture.m_Order.m_eStatus
			== HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED
			&& fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& fixture.m_Operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			&& fixture.m_Batch.m_bStrategicProjectionHeld;
		bool heldRosterExact = garrison
			&& garrison.m_iInfantryCount == fixture.m_iAggregateBefore
			&& garrison.m_aAcceptedManifestIds.Contains(
				fixture.m_Manifest.m_sManifestId)
			&& authoritative == fixture.m_iAggregateBefore + living;
		bool heldResourceExact = pool
			&& pool.m_iSupportResources == supportAfterDebit;
		bool heldExact = heldLifecycleExact && heldRosterExact
			&& heldResourceExact;
		string settlementId = fixture.m_Order.m_sResourceSettlementId;
		bool replayChanged = fixture.m_Service.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		bool replayExact = !replayChanged
			&& fixture.m_Order.m_sResourceSettlementId == settlementId
			&& fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& pool.m_iSupportResources == supportAfterDebit
			&& CountAcceptedManifest(garrison, fixture.m_Manifest.m_sManifestId) == 1;
		int aggregateInfantry;
		if (garrison)
			aggregateInfantry = garrison.m_iInfantryCount;
		report.m_bDeliveryHeldExact = receiptExact && heldExact && replayExact;
		report.m_sDeliveryEvidence = string.Format(
			"delivered %1 | receipt %2 accepted/survivors %3/%4 | open/on-station %5/%6 | aggregate/exact/authoritative %7/%8/%9",
			delivered,
			fixture.m_Order.m_sResourceSettlementKind,
			fixture.m_Order.m_iSettlementAcceptedMemberCount,
			fixture.m_Order.m_iSettlementSurvivorMemberCount,
			fixture.m_Operation.m_eSettlementState,
			fixture.m_Operation.m_eDutyState,
			aggregateInfantry,
			living,
			authoritative);
		report.m_sDeliveryEvidence += string.Format(
			" | zero-refund replay %1",
			replayExact);
		report.m_sDeliveryEvidence = report.m_sDeliveryEvidence
			+ " | " + deliveryFailureEvidence;
	}

	protected void ProvePhysicalVirtualCasualtyContinuity(
		HST_EnemyGarrisonRebuildOperationProofReport report)
	{
		HST_EnemyGarrisonRebuildOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("physical_virtual_casualties");
		if (!m_Fixtures.Ready(fixture)
			|| fixture.m_Manifest.m_iAcceptedMemberCount < 2)
		{
			report.m_sCasualtyEvidence = m_Fixtures.Failure(fixture);
			if (m_Fixtures.Ready(fixture))
				report.m_sCasualtyEvidence
					= "exact rebuild proof manifest has fewer than two members";
			return;
		}

		int accepted = fixture.m_Manifest.m_iAcceptedMemberCount;
		string virtualCasualtyId = fixture.m_Queue.SelectStrategicLivingMemberSlotId(
			fixture.m_Batch,
			fixture.m_Operation.m_iDeterministicSeed + 17);
		HST_ForceSpawnQueueCallbackResult virtualCasualty
			= fixture.m_Queue.ConfirmStrategicMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				virtualCasualtyId,
				fixture.m_State.m_iElapsedSeconds + 1,
				"focused rebuild proof virtual casualty");
		ApplyGroupRoster(fixture.m_Group, accepted - 1);

		HST_ForceSpawnQueueCallbackResult released
			= fixture.m_Queue.ReleaseStrategicProjectionForMaterialization(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				fixture.m_State.m_iElapsedSeconds + 2,
				fixture.m_State.m_iElapsedSeconds + 182);
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult materializing
			= operations.MarkExactEnemyGarrisonRebuildMaterializingFromVirtual(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				fixture.m_Batch,
				"focused rebuild proof entered the physical bubble");
		PrepareSyntheticSuccessfulProjection(fixture);
		fixture.m_Group.m_bSpawnedEntity = true;
		fixture.m_Group.m_sRuntimeEntityId = fixture.m_Batch.m_sNativeGroupId;
		fixture.m_Group.m_iSpawnedAgentCount = accepted - 1;
		HST_OperationTransitionResult physical
			= operations.MarkExactEnemyGarrisonRebuildPhysical(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				fixture.m_Batch,
				"focused rebuild proof established physical roster authority");

		HST_ForceSpawnSlotResultState liveSlot
			= FindFirstRegisteredMemberSlot(fixture.m_Batch);
		string physicalCasualtyId;
		HST_ForceSpawnQueueCallbackResult physicalCasualty;
		if (liveSlot)
		{
			physicalCasualtyId = liveSlot.m_sSlotId;
			physicalCasualty = fixture.m_Queue.ConfirmRegisteredMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				liveSlot.m_sSlotId,
				liveSlot.m_sEntityId,
				fixture.m_State.m_iElapsedSeconds + 3,
				"focused rebuild proof physical casualty");
		}
		ApplyGroupRoster(fixture.m_Group, accepted - 2);
		fixture.m_Group.m_iSpawnedAgentCount = Math.Max(0, accepted - 2);
		HST_OperationTransitionResult folding
			= operations.BeginExactEnemyGarrisonRebuildDematerialization(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				"focused rebuild proof left the physical bubble");
		HST_ForceSpawnQueueCallbackResult held
			= fixture.m_Queue.RequeueSuccessfulProjectionForStrategicHold(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				fixture.m_State.m_iElapsedSeconds + 4,
				fixture.m_State.m_iElapsedSeconds + 184);
		fixture.m_Group.m_bSpawnedEntity = false;
		fixture.m_Group.m_sRuntimeEntityId = "";
		fixture.m_Group.m_iSpawnedAgentCount = 0;
		fixture.m_Group.m_iAssignedWaypointCount = 0;
		HST_OperationTransitionResult virtualized
			= operations.CompleteExactEnemyGarrisonRebuildDematerialization(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				fixture.m_Batch,
				"focused rebuild proof resumed the exact strategic roster");

		int expectedLiving = accepted - 2;
		int living = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		int casualties = fixture.m_Queue.CountConfirmedCasualtyMemberSlots(
			fixture.m_Batch);
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_EnemyOrderState restoredOrder;
		HST_OperationRecordState restoredOperation;
		HST_ForceSpawnResultState restoredBatch;
		if (restored)
		{
			restoredOrder = restored.FindEnemyOrder(fixture.m_Order.m_sOrderId);
			if (restoredOrder)
			{
				restoredOperation = restored.FindOperation(restoredOrder.m_sOperationId);
				restoredBatch = restored.FindForceSpawnResult(
					restoredOrder.m_sSpawnResultId);
			}
		}
		HST_ForceSpawnQueueService restoredQueue = new HST_ForceSpawnQueueService();
		bool restoredRowsExact = restoredOrder && restoredOperation && restoredBatch;
		bool restoredOrderExact;
		bool restoredProjectionExact;
		if (restoredRowsExact)
		{
			restoredOrderExact = restoredOrder.m_iOperationContractVersion
				== HST_EnemyGarrisonRebuildOperationService.EXACT_CONTRACT_VERSION
				&& restoredOperation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				&& restoredBatch.m_bStrategicProjectionHeld;
			restoredProjectionExact
				= restoredQueue.CountStrategicLivingMemberSlots(restoredBatch)
				== expectedLiving
				&& restoredQueue.CountConfirmedCasualtyMemberSlots(restoredBatch) == 2;
		}
		bool restoredExact = restoredRowsExact && restoredOrderExact
			&& restoredProjectionExact;
		bool virtualTransitionsExact = virtualCasualty && virtualCasualty.m_bAccepted
			&& released && released.m_bAccepted
			&& materializing && materializing.m_bAccepted;
		bool physicalTransitionsExact = physical && physical.m_bAccepted
			&& physicalCasualty && physicalCasualty.m_bAccepted
			&& folding && folding.m_bAccepted;
		bool returnTransitionsExact = held && held.m_bAccepted
			&& virtualized && virtualized.m_bAccepted;
		bool transitionsExact = virtualTransitionsExact
			&& physicalTransitionsExact
			&& returnTransitionsExact;
		report.m_bCasualtyContinuityExact = transitionsExact
			&& living == expectedLiving && casualties == 2 && restoredExact
			&& !fixture.m_Order.m_bResourceSettlementApplied
			&& fixture.m_Order.m_iRefundedSupportResources == 0;
		report.m_sCasualtyEvidence = string.Format(
			"accepted/living/casualties %1/%2/%3 | virtual slot %4 | physical slot %5 | transitions %6 | restored %7",
			accepted,
			living,
			casualties,
			virtualCasualtyId,
			physicalCasualtyId,
			transitionsExact,
			restoredExact);
	}

	protected void ProveDeliveredRestore(
		HST_EnemyGarrisonRebuildOperationProofReport report)
	{
		HST_EnemyGarrisonRebuildOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("delivered_restore");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sRestoreEvidence = m_Fixtures.Failure(fixture);
			return;
		}
		string casualtyId;
		bool casualtyExact = ConfirmOneStrategicCasualty(fixture, casualtyId);
		if (!casualtyExact || !DriveUntilDelivered(fixture, 40))
		{
			report.m_sRestoreEvidence
				= "exact rebuild proof could not reach delivered casualty state | "
					+ BuildDeliveryFailureEvidence(fixture);
			return;
		}
		int expectedLiving = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		string settlementId = fixture.m_Order.m_sResourceSettlementId;
		HST_CampaignSaveData firstSave = new HST_CampaignSaveData();
		firstSave.Capture(fixture.m_State);
		HST_CampaignState first = firstSave.Restore();
		bool firstExact = IsDeliveredRestoreExact(
			first,
			fixture.m_Order.m_sOrderId,
			fixture.m_Manifest.m_sManifestId,
			settlementId,
			expectedLiving,
			casualtyId);

		HST_EnemyGarrisonRebuildOperationService restoredService
			= new HST_EnemyGarrisonRebuildOperationService();
		HST_ForceSpawnQueueService restoredQueue = new HST_ForceSpawnQueueService();
		restoredService.SetRuntimeServices(
			restoredQueue,
			new HST_ForceSpawnAdapterService(),
			new HST_PhysicalWarService(),
			new HST_GarrisonService());
		HST_EnemyDirectorService restoredDirector = new HST_EnemyDirectorService();
		restoredDirector.SetCampaignPreset(fixture.m_Preset);
		restoredService.SetEnemyDirectorService(restoredDirector);
		bool reconcileChanged;
		if (first)
			reconcileChanged = restoredService.ReconcileAfterRestore(
				first,
				restoredDirector);
		bool reconciledExact = firstExact && IsDeliveredRestoreExact(
			first,
			fixture.m_Order.m_sOrderId,
			fixture.m_Manifest.m_sManifestId,
			settlementId,
			expectedLiving,
			casualtyId);

		HST_CampaignSaveData replaySave = new HST_CampaignSaveData();
		if (first)
			replaySave.Capture(first);
		HST_CampaignState replay;
		if (first)
			replay = replaySave.Restore();
		bool replayExact = IsDeliveredRestoreExact(
			replay,
			fixture.m_Order.m_sOrderId,
			fixture.m_Manifest.m_sManifestId,
			settlementId,
			expectedLiving,
			casualtyId);
		report.m_bRestoreExact = firstExact && reconciledExact && replayExact;
		report.m_sRestoreEvidence = string.Format(
			"delivered living %1 casualty %2 | first/reconciled/replay %3/%4/%5 | reconcile changed %6",
			expectedLiving,
			casualtyId,
			firstExact,
			reconciledExact,
			replayExact,
			reconcileChanged);
	}

	protected void ProveOwnershipTerminalSettlement(
		HST_EnemyGarrisonRebuildOperationProofReport report)
	{
		HST_EnemyGarrisonRebuildOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("ownership_terminal");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sOwnershipEvidence = m_Fixtures.Failure(fixture);
			return;
		}
		if (!DriveUntilDelivered(fixture, 40))
		{
			report.m_sOwnershipEvidence
				= "exact rebuild proof could not reach delivered ownership state | "
					+ BuildDeliveryFailureEvidence(fixture);
			return;
		}
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		int supportBefore = pool.m_iSupportResources;
		string deliverySettlementId = fixture.m_Order.m_sResourceSettlementId;
		string preflightFailure;
		bool preflight = fixture.m_Service.CanReconcileZoneOwnershipChange(
			fixture.m_State,
			fixture.m_sTargetZoneId,
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_OTHER_FACTION_KEY,
			preflightFailure);
		bool changed;
		string failure;
		bool reconciled = fixture.m_Service.ReconcileZoneOwnershipChange(
			fixture.m_State,
			fixture.m_sTargetZoneId,
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_OTHER_FACTION_KEY,
			changed,
			failure);
		HST_GarrisonState garrison = fixture.m_State.FindGarrison(
			fixture.m_sTargetZoneId,
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool terminalLifecycleExact = preflight && reconciled && changed
			&& fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& fixture.m_Operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		bool terminalReceiptExact = fixture.m_Order.m_sResourceSettlementId
			== deliverySettlementId
			&& fixture.m_Order.m_sResourceSettlementKind
				== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_DELIVERY_SETTLEMENT_KIND
			&& fixture.m_Order.m_iRefundedAttackResources == 0
			&& fixture.m_Order.m_iRefundedSupportResources == 0;
		bool terminalResourceExact = pool.m_iSupportResources == supportBefore;
		bool terminalRosterExact = !garrison
			|| !garrison.m_aAcceptedManifestIds.Contains(
				fixture.m_Manifest.m_sManifestId);
		bool terminalExact = terminalLifecycleExact && terminalReceiptExact
			&& terminalResourceExact && terminalRosterExact;

		bool replayChanged;
		string replayFailure;
		bool replay = fixture.m_Service.ReconcileZoneOwnershipChange(
			fixture.m_State,
			fixture.m_sTargetZoneId,
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_OTHER_FACTION_KEY,
			replayChanged,
			replayFailure);
		bool replayExact = replay && !replayChanged
			&& fixture.m_Order.m_sResourceSettlementId == deliverySettlementId
			&& pool.m_iSupportResources == supportBefore
			&& fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		report.m_bOwnershipTerminalExact = terminalExact && replayExact;
		report.m_sOwnershipEvidence = string.Format(
			"preflight/reconciled/changed %1/%2/%3 | operation settlement/duty %4/%5 | delivery receipt retained %6 | support %7 | replay %8 | failure '%9'",
			preflight,
			reconciled,
			changed,
			fixture.m_Operation.m_eSettlementState,
			fixture.m_Operation.m_eDutyState,
			fixture.m_Order.m_sResourceSettlementId == deliverySettlementId,
			pool.m_iSupportResources,
			replayExact,
			failure);
	}

	protected void ProveAdmissionRollback(
		HST_EnemyGarrisonRebuildOperationProofReport report)
	{
		HST_EnemyGarrisonRebuildOperationProofFixture fixture
			= m_Fixtures.BuildPreparedFixture("admission_rollback", true);
		if (!m_Fixtures.Prepared(fixture))
		{
			report.m_sAdmissionRollbackEvidence = m_Fixtures.Failure(fixture);
			return;
		}
		HST_EnemyGarrisonRebuildRejectEnqueueProofHarness rejectingQueue
			= new HST_EnemyGarrisonRebuildRejectEnqueueProofHarness();
		fixture.m_Queue = rejectingQueue;
		fixture.m_Service = new HST_EnemyGarrisonRebuildOperationService();
		fixture.m_Service.SetRuntimeServices(
			fixture.m_Queue,
			fixture.m_Adapter,
			fixture.m_PhysicalWar,
			fixture.m_Garrisons);
		fixture.m_Service.SetEnemyDirectorService(fixture.m_EnemyDirector);
		fixture.m_State.m_aEnemyOrders.Insert(fixture.m_Order);
		HST_EnemyGarrisonRebuildAdmissionResult admission
			= fixture.m_Service.AdmitPreparedOrder(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Manifest,
				fixture.m_EnemyDirector);
		for (int tick = 0; tick < 4
			&& !fixture.m_Order.m_bResourceSettlementApplied; tick++)
		{
			fixture.m_State.m_iElapsedSeconds++;
			fixture.m_Service.TickOrder(
				fixture.m_State,
				fixture.m_Preset,
				fixture.m_EnemyDirector,
				fixture.m_Order);
		}
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool resourcesExact = pool
			&& pool.m_iSupportResources == fixture.m_iSupportBeforeDebit
			&& fixture.m_Order.m_bResourceSettlementApplied
			&& fixture.m_Order.m_iRefundedSupportResources
				== HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_SUPPORT_COST
			&& fixture.m_Order.m_iRefundedAttackResources == 0
			&& CountMutationId(
				fixture.m_State,
				fixture.m_Order.m_sResourceRefundMutationId) == 1;
		bool rollbackOrderExact = admission && !admission.m_bSuccess
			&& fixture.m_Order.m_eStatus
				== HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& !fixture.m_Order.m_bStrategicServiceCommitted;
		bool rollbackGraphExact
			= !fixture.m_State.FindOperation(fixture.m_Order.m_sOperationId)
			&& !fixture.m_State.FindForceManifest(fixture.m_Manifest.m_sManifestId)
			&& fixture.m_State.m_aForceSpawnResults.Count() == 0
			&& fixture.m_State.m_aActiveGroups.Count() == 0;
		bool rollbackExact = rollbackOrderExact && rollbackGraphExact;
		int supportAfterRollback;
		if (pool)
			supportAfterRollback = pool.m_iSupportResources;
		report.m_bAdmissionRollbackExact = resourcesExact && rollbackExact;
		report.m_sAdmissionRollbackEvidence = string.Format(
			"admission success %1 | status %2 committed %3 | support %4 -> %5 -> %6 refunded %7 | graph operation/manifest %8/%9",
			admission && admission.m_bSuccess,
			fixture.m_Order.m_eStatus,
			fixture.m_Order.m_bStrategicServiceCommitted,
			fixture.m_iSupportBeforeDebit,
			fixture.m_iSupportAfterDebit,
			supportAfterRollback,
			fixture.m_Order.m_iRefundedSupportResources,
			fixture.m_State.FindOperation(fixture.m_Order.m_sOperationId) != null,
			fixture.m_State.FindForceManifest(fixture.m_Manifest.m_sManifestId) != null);
		report.m_sAdmissionRollbackEvidence += string.Format(
			" | graph batch/group %1/%2",
			fixture.m_State.m_aForceSpawnResults.Count(),
			fixture.m_State.m_aActiveGroups.Count());
	}

	protected void ProvePrearrivalSurvivorRefund(
		HST_EnemyGarrisonRebuildOperationProofReport report)
	{
		HST_EnemyGarrisonRebuildOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("prearrival_refund");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sPrearrivalRefundEvidence = m_Fixtures.Failure(fixture);
			return;
		}
		string casualtyId;
		bool casualty = ConfirmOneStrategicCasualty(fixture, casualtyId);
		int survivors = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		HST_ZoneState target = fixture.m_State.FindZone(fixture.m_sTargetZoneId);
		target.m_sOwnerFactionKey
			= HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_OTHER_FACTION_KEY;
		target.m_iOwnershipRevision++;
		int supportBeforeSettlement = fixture.m_iSupportAfterDebit;
		bool settled = DriveUntilSettled(fixture, 80);
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		int expectedRefund
			= HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_SUPPORT_COST
				* survivors / fixture.m_Manifest.m_iAcceptedMemberCount;
		HST_GarrisonState garrison = fixture.m_State.FindGarrison(
			fixture.m_sTargetZoneId,
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool refundReceiptExact = casualty && settled
			&& fixture.m_Order.m_bResourceSettlementApplied
			&& fixture.m_Order.m_sResourceSettlementKind
				!= HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_DELIVERY_SETTLEMENT_KIND
			&& fixture.m_Order.m_iSettlementAcceptedMemberCount
				== fixture.m_Manifest.m_iAcceptedMemberCount
			&& fixture.m_Order.m_iSettlementSurvivorMemberCount == survivors;
		bool refundResourceExact
			= fixture.m_Order.m_iRefundedSupportResources == expectedRefund
			&& pool.m_iSupportResources == supportBeforeSettlement + expectedRefund
			&& CountMutationId(
				fixture.m_State,
				fixture.m_Order.m_sResourceRefundMutationId) == 1;
		bool refundRosterExact = !garrison
			|| !garrison.m_aAcceptedManifestIds.Contains(
				fixture.m_Manifest.m_sManifestId);
		bool refundExact = refundReceiptExact && refundResourceExact
			&& refundRosterExact;
		int supportAfterSettlement = pool.m_iSupportResources;
		string settlementId = fixture.m_Order.m_sResourceSettlementId;
		bool replayChanged = fixture.m_Service.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		bool replayExact = !replayChanged
			&& pool.m_iSupportResources == supportAfterSettlement
			&& fixture.m_Order.m_sResourceSettlementId == settlementId
			&& CountMutationId(
				fixture.m_State,
				fixture.m_Order.m_sResourceRefundMutationId) == 1;
		report.m_bPrearrivalRefundExact = refundExact && replayExact;
		report.m_sPrearrivalRefundEvidence = string.Format(
			"casualty %1 | accepted/survivors %2/%3 | expected/applied support refund %4/%5 | pool %6 -> %7 | settled/replay %8/%9",
			casualtyId,
			fixture.m_Manifest.m_iAcceptedMemberCount,
			survivors,
			expectedRefund,
			fixture.m_Order.m_iRefundedSupportResources,
			supportBeforeSettlement,
			supportAfterSettlement,
			settled,
			replayExact);
		report.m_sPrearrivalRefundEvidence += string.Format(
			" | kind %1",
			fixture.m_Order.m_sResourceSettlementKind);
	}

	protected void ProveSettlementCrashWindowResume(
		HST_EnemyGarrisonRebuildOperationProofReport report)
	{
		string preparedEvidence;
		string settledEvidence;
		bool preparedExact = ProveSettlementCrashWindowCase(
			false,
			preparedEvidence);
		bool settledExact = ProveSettlementCrashWindowCase(
			true,
			settledEvidence);
		report.m_bSettlementCrashResumeExact = preparedExact && settledExact;
		report.m_sSettlementCrashEvidence = preparedEvidence
			+ " | " + settledEvidence;
	}

	protected bool ProveSettlementCrashWindowCase(
		bool settleOperationBeforeOrderTail,
		out string evidence)
	{
		string windowLabel = "prepared-with-receipt";
		string suffix = "prepared_receipt_crash";
		if (settleOperationBeforeOrderTail)
		{
			windowLabel = "settled-before-order-tail";
			suffix = "settled_order_tail_crash";
		}
		HST_EnemyGarrisonRebuildOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture(suffix);
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = windowLabel + " fixture failed: "
				+ m_Fixtures.Failure(fixture);
			return false;
		}

		string casualtyId;
		bool casualty = ConfirmOneStrategicCasualty(fixture, casualtyId);
		if (!casualty)
		{
			evidence = windowLabel
				+ " could not establish survivor-proportional settlement";
			return false;
		}
		HST_EnemyGarrisonRebuildCrashWindowExpectation expectation
			= new HST_EnemyGarrisonRebuildCrashWindowExpectation();
		expectation.m_bSettledBeforeOrderTail
			= settleOperationBeforeOrderTail;
		expectation.m_eStaleOrderStatus = fixture.m_Order.m_eStatus;
		expectation.m_sStaleRuntimeStatus = fixture.m_Order.m_sRuntimeStatus;
		expectation.m_sStaleResolutionKind = fixture.m_Order.m_sResolutionKind;
		expectation.m_sStaleFailureReason = fixture.m_Order.m_sFailureReason;
		expectation.m_iStaleResolvedAtSecond
			= fixture.m_Order.m_iResolvedAtSecond;
		expectation.m_bStalePhysicalized = fixture.m_Order.m_bPhysicalized;
		expectation.m_bStaleAbstractResolved
			= fixture.m_Order.m_bAbstractResolved;
		expectation.m_bStaleOutcomeApplied = fixture.m_Order.m_bOutcomeApplied;

		HST_EnemyGarrisonRebuildCrashWindowProofHarness staging
			= new HST_EnemyGarrisonRebuildCrashWindowProofHarness();
		staging.SetRuntimeServices(
			fixture.m_Queue,
			fixture.m_Adapter,
			fixture.m_PhysicalWar,
			fixture.m_Garrisons);
		staging.SetEnemyDirectorService(fixture.m_EnemyDirector);
		bool staged = staging.StageReceiptCrashWindow(
			fixture,
			settleOperationBeforeOrderTail);

		expectation.m_sOrderId = fixture.m_Order.m_sOrderId;
		expectation.m_sOperationId = fixture.m_Operation.m_sOperationId;
		expectation.m_sManifestId = fixture.m_Manifest.m_sManifestId;
		expectation.m_sBatchId = fixture.m_Batch.m_sResultId;
		expectation.m_sGroupId = fixture.m_Group.m_sGroupId;
		expectation.m_sSettlementId = fixture.m_Order.m_sResourceSettlementId;
		expectation.m_sSettlementKind
			= HST_EnemyGarrisonRebuildCrashWindowProofHarness.SETTLEMENT_KIND;
		expectation.m_sTerminalReason
			= HST_EnemyGarrisonRebuildCrashWindowProofHarness.TERMINAL_REASON;
		expectation.m_sDebitMutationId
			= fixture.m_Order.m_sResourceDebitMutationId;
		expectation.m_sRefundMutationId
			= fixture.m_Order.m_sResourceRefundMutationId;
		expectation.m_iAccepted = fixture.m_Manifest.m_iAcceptedMemberCount;
		expectation.m_iSurvivors = fixture.m_Queue
			.CountStrategicLivingMemberSlots(fixture.m_Batch);
		expectation.m_iSupportRefund
			= HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_SUPPORT_COST
				* expectation.m_iSurvivors / expectation.m_iAccepted;
		expectation.m_iExpectedSupport = fixture.m_iSupportAfterDebit
			+ expectation.m_iSupportRefund;
		expectation.m_iSettledAtSecond = fixture.m_Operation.m_iSettledAtSecond;
		expectation.m_iExpectedMutationCount = 2;

		bool stagedExact = staged
			&& IsSettlementCrashWindowExact(fixture.m_State, expectation);
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		bool persistedExact = IsSettlementCrashWindowExact(
			restored,
			expectation);

		HST_EnemyGarrisonRebuildOperationService resumed
			= new HST_EnemyGarrisonRebuildOperationService();
		HST_EnemyDirectorService restoredDirector
			= new HST_EnemyDirectorService();
		restoredDirector.SetCampaignPreset(fixture.m_Preset);
		resumed.SetRuntimeServices(
			new HST_ForceSpawnQueueService(),
			new HST_ForceSpawnAdapterService(),
			new HST_PhysicalWarService(),
			new HST_GarrisonService());
		resumed.SetEnemyDirectorService(restoredDirector);
		bool reconcileChanged;
		if (restored)
			reconcileChanged = resumed.ReconcileAfterRestore(
				restored,
				restoredDirector);
		bool resumedExact = IsSettlementCrashTerminalExact(
			restored,
			expectation);

		HST_OperationRecordState terminalOperation;
		HST_FactionPoolState terminalPool;
		if (restored)
		{
			terminalOperation = restored.FindOperation(expectation.m_sOperationId);
			terminalPool = restored.FindFactionPool(
				HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		}
		int terminalRevision;
		int supportBeforeReplay;
		int mutationsBeforeReplay;
		if (terminalOperation)
			terminalRevision = terminalOperation.m_iRevision;
		if (terminalPool)
			supportBeforeReplay = terminalPool.m_iSupportResources;
		if (restored)
			mutationsBeforeReplay = restored.m_aEnemyStrategicMutations.Count();
		bool replayChanged;
		if (restored)
			replayChanged = resumed.ReconcileAfterRestore(
				restored,
				restoredDirector);
		bool replayExact = !replayChanged
			&& IsSettlementCrashTerminalExact(restored, expectation)
			&& terminalOperation
			&& terminalOperation.m_iRevision == terminalRevision
			&& terminalPool
			&& terminalPool.m_iSupportResources == supportBeforeReplay
			&& restored.m_aEnemyStrategicMutations.Count()
				== mutationsBeforeReplay;

		evidence = string.Format(
			"window %1 | casualty %2 | staged/persisted/resumed %3/%4/%5 | changed/replay %6/%7 | support/mutations %8/%9",
			windowLabel,
			casualtyId,
			stagedExact,
			persistedExact,
			resumedExact,
			reconcileChanged,
			replayExact,
			supportBeforeReplay,
			mutationsBeforeReplay);
		evidence += string.Format(
			" | accepted/survivors/refund %1/%2/%3",
			expectation.m_iAccepted,
			expectation.m_iSurvivors,
			expectation.m_iSupportRefund);
		return stagedExact && persistedExact && reconcileChanged
			&& resumedExact && replayExact;
	}

	protected void ProveHistoricalContractIsolation(
		HST_EnemyGarrisonRebuildOperationProofReport report)
	{
		HST_EnemyGarrisonRebuildOperationProofFixture fixture
			= m_Fixtures.BuildPreparedFixture("historical_contract_zero", false);
		if (!fixture || !fixture.m_State)
		{
			report.m_sHistoricalEvidence
				= "historical exact rebuild proof fixture is unavailable";
			return;
		}
		HST_EnemyOrderState legacy = new HST_EnemyOrderState();
		legacy.m_sOrderId = "historical_contract_zero_rebuild";
		legacy.m_sOperationId = HST_StableIdService.BuildOperationId(
			"enemy_order",
			legacy.m_sOrderId);
		legacy.m_iOperationContractVersion = 0;
		legacy.m_sFactionKey
			= HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY;
		legacy.m_eType
			= HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON;
		legacy.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
		legacy.m_sSourceZoneId = fixture.m_sSourceZoneId;
		legacy.m_sTargetZoneId = fixture.m_sTargetZoneId;
		legacy.m_vSourcePosition
			= fixture.m_State.FindZone(fixture.m_sSourceZoneId).m_vPosition;
		legacy.m_vTargetPosition
			= fixture.m_State.FindZone(fixture.m_sTargetZoneId).m_vPosition;
		legacy.m_iCreatedAtSecond = fixture.m_State.m_iElapsedSeconds - 10;
		legacy.m_iResolveAtSecond = fixture.m_State.m_iElapsedSeconds + 410;
		legacy.m_iSupportCost
			= HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_SUPPORT_COST;
		legacy.m_sRuntimeStatus = "active_rebuild";
		fixture.m_State.m_aEnemyOrders.Insert(legacy);

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		saveData.m_iSchemaVersion = 69;
		saveData.m_iLastLoadedSchemaVersion = 69;
		HST_CampaignState restored = saveData.Restore();
		HST_EnemyOrderState restoredLegacy;
		if (restored)
			restoredLegacy = restored.FindEnemyOrder(legacy.m_sOrderId);
		bool historicalOrderExact = restoredLegacy
			&& restoredLegacy.m_iOperationContractVersion == 0
			&& restoredLegacy.m_eType
				== HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON
			&& restoredLegacy.m_sOperationId == legacy.m_sOperationId;
		bool historicalLinksEmpty = restoredLegacy
			&& restoredLegacy.m_sManifestId.IsEmpty()
			&& restoredLegacy.m_sSpawnResultId.IsEmpty()
			&& restoredLegacy.m_sGroupId.IsEmpty();
		bool historicalGraphEmpty = restored
			&& !restored.FindOperation(legacy.m_sOperationId)
			&& restored.m_aForceManifests.IsEmpty()
			&& restored.m_aForceSpawnResults.IsEmpty()
			&& restored.m_aActiveGroups.IsEmpty();
		bool historicalSettlementEmpty = restoredLegacy && restored
			&& restored.m_aEnemyStrategicMutations.IsEmpty()
			&& !restoredLegacy.m_bResourceSettlementApplied
			&& !restoredLegacy.m_bResourceRefundApplied;
		bool exact = historicalOrderExact && historicalLinksEmpty
			&& historicalGraphEmpty && historicalSettlementEmpty;
		int restoredContractVersion;
		HST_EEnemyOrderStatus restoredStatus;
		if (restoredLegacy)
		{
			restoredContractVersion = restoredLegacy.m_iOperationContractVersion;
			restoredStatus = restoredLegacy.m_eStatus;
		}
		int restoredMutationCount;
		if (restored)
			restoredMutationCount = restored.m_aEnemyStrategicMutations.Count();
		report.m_bHistoricalIsolationExact = exact;
		report.m_sHistoricalEvidence = string.Format(
			"restored %1 | contract/status %2/%3 | exact graph operation/manifest/batch/group %4/%5/%6/%7 | mutations %8",
			restored != null,
			restoredContractVersion,
			restoredStatus,
			restored && restored.FindOperation(legacy.m_sOperationId) != null,
			restored && !restored.m_aForceManifests.IsEmpty(),
			restored && !restored.m_aForceSpawnResults.IsEmpty(),
			restored && !restored.m_aActiveGroups.IsEmpty(),
			restoredMutationCount);
	}

	protected void ProveSchema70Quarantine(
		HST_EnemyGarrisonRebuildOperationProofReport report)
	{
		HST_EnemyGarrisonRebuildOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("schema70_quarantine");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sQuarantineEvidence = m_Fixtures.Failure(fixture);
			return;
		}
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		int supportBefore = pool.m_iSupportResources;
		HST_CampaignSaveData malformed = new HST_CampaignSaveData();
		malformed.Capture(fixture.m_State);
		HST_EnemyGarrisonRebuildMalformedQuarantineExpectation expectation
			= new HST_EnemyGarrisonRebuildMalformedQuarantineExpectation();
		expectation.m_sOrderId = fixture.m_Order.m_sOrderId;
		expectation.m_sOperationId = fixture.m_Operation.m_sOperationId;
		expectation.m_sManifestId = fixture.m_Manifest.m_sManifestId;
		expectation.m_sPrimaryBatchId = fixture.m_Batch.m_sResultId;
		expectation.m_sPrimaryGroupId = fixture.m_Group.m_sGroupId;
		expectation.m_iExpectedSupport = supportBefore;
		HST_OperationRecordState malformedOperation = FindOperation(
			malformed.m_aOperations,
			fixture.m_Operation.m_sOperationId);
		HST_ForceSpawnResultState primaryBatch = FindBatch(
			malformed.m_aForceSpawnResults,
			expectation.m_sPrimaryBatchId);
		HST_ActiveGroupState primaryGroup = FindGroup(
			malformed.m_aActiveGroups,
			expectation.m_sPrimaryGroupId);
		if (!malformedOperation || !primaryBatch || !primaryGroup)
		{
			report.m_sQuarantineEvidence
				= "schema-70 quarantine proof primary save graph is unavailable";
			return;
		}
		HST_ForceSpawnResultState secondaryBatch
			= BuildSecondaryQuarantineBatch(primaryBatch);
		HST_ActiveGroupState secondaryGroup = BuildSecondaryQuarantineGroup(
			primaryGroup,
			secondaryBatch);
		if (!secondaryBatch || !secondaryGroup)
		{
			report.m_sQuarantineEvidence
				= "schema-70 quarantine proof secondary claimant build failed";
			return;
		}
		malformed.m_aForceSpawnResults.Insert(secondaryBatch);
		malformed.m_aActiveGroups.Insert(secondaryGroup);
		expectation.m_sSecondaryBatchId = secondaryBatch.m_sResultId;
		expectation.m_sSecondaryGroupId = secondaryGroup.m_sGroupId;
		bool primaryStaged = StageQuarantineProcessEvidence(
			primaryBatch,
			primaryGroup,
			"primary",
			malformed.m_iElapsedSeconds,
			expectation.m_sPrimaryProcessSlotId);
		bool secondaryStaged = StageQuarantineProcessEvidence(
			secondaryBatch,
			secondaryGroup,
			"secondary",
			malformed.m_iElapsedSeconds,
			expectation.m_sSecondaryProcessSlotId);
		malformedOperation.m_sManifestId
			= malformedOperation.m_sManifestId + "_corrupt";
		HST_CampaignState restored = malformed.Restore();
		bool firstExact = IsSchema70QuarantineExact(
			restored,
			expectation);
		HST_OperationRecordState firstOperation;
		HST_ForceSpawnResultState firstPrimaryBatch;
		HST_ForceSpawnResultState firstSecondaryBatch;
		HST_ActiveGroupState firstPrimaryGroup;
		HST_ActiveGroupState firstSecondaryGroup;
		if (restored)
		{
			firstOperation = restored.FindOperation(expectation.m_sOperationId);
			firstPrimaryBatch = restored.FindForceSpawnResult(
				expectation.m_sPrimaryBatchId);
			firstSecondaryBatch = restored.FindForceSpawnResult(
				expectation.m_sSecondaryBatchId);
			firstPrimaryGroup = restored.FindActiveGroup(
				expectation.m_sPrimaryGroupId);
			firstSecondaryGroup = restored.FindActiveGroup(
				expectation.m_sSecondaryGroupId);
		}

		HST_CampaignSaveData replaySave = new HST_CampaignSaveData();
		if (restored)
			replaySave.Capture(restored);
		HST_CampaignState replay;
		if (restored)
			replay = replaySave.Restore();
		bool replayExact = IsSchema70QuarantineExact(
			replay,
			expectation);
		bool operationReplayExact = firstOperation && replay
			&& replay.FindOperation(expectation.m_sOperationId)
			&& replay.FindOperation(expectation.m_sOperationId).m_iRevision
				== firstOperation.m_iRevision;
		bool primaryReplayExact = IsQuarantineRuntimeRevisionReplayExact(
			firstPrimaryBatch,
			firstPrimaryGroup,
			replay,
			expectation.m_sPrimaryBatchId,
			expectation.m_sPrimaryGroupId);
		bool secondaryReplayExact = IsQuarantineRuntimeRevisionReplayExact(
			firstSecondaryBatch,
			firstSecondaryGroup,
			replay,
			expectation.m_sSecondaryBatchId,
			expectation.m_sSecondaryGroupId);
		bool replayIdempotent = operationReplayExact && primaryReplayExact
			&& secondaryReplayExact;
		HST_EnemyOrderState quarantined;
		if (restored)
			quarantined = restored.FindEnemyOrder(fixture.m_Order.m_sOrderId);
		int quarantinedContractVersion;
		HST_EEnemyOrderStatus quarantinedStatus;
		string quarantineFailure;
		if (quarantined)
		{
			quarantinedContractVersion = quarantined.m_iOperationContractVersion;
			quarantinedStatus = quarantined.m_eStatus;
			quarantineFailure = quarantined.m_sFailureReason;
		}
		int retainedBatchCount;
		int retainedGroupCount;
		int retainedMutationCount;
		if (replay)
		{
			retainedBatchCount = replay.m_aForceSpawnResults.Count();
			retainedGroupCount = replay.m_aActiveGroups.Count();
			retainedMutationCount = replay.m_aEnemyStrategicMutations.Count();
		}
		report.m_bSchema70QuarantineExact = primaryStaged && secondaryStaged
			&& firstExact && replayExact && replayIdempotent;
		report.m_sQuarantineEvidence = string.Format(
			"claimants staged %1/%2 | first/replay/idempotent %3/%4/%5 | contract/status %6/%7 | retained batch/group %8/%9",
			primaryStaged,
			secondaryStaged,
			firstExact,
			replayExact,
			replayIdempotent,
			quarantinedContractVersion,
			quarantinedStatus,
			retainedBatchCount,
			retainedGroupCount);
		report.m_sQuarantineEvidence += string.Format(
			" | settlement/refund/mutations %1/%2/%3 | reason '%4'",
			quarantined && quarantined.m_bResourceSettlementApplied,
			quarantined && quarantined.m_bResourceRefundApplied,
			retainedMutationCount,
			quarantineFailure);
	}

	protected HST_ForceSpawnResultState BuildSecondaryQuarantineBatch(
		HST_ForceSpawnResultState primary)
	{
		if (!primary)
			return null;
		HST_ForceSpawnResultState batch = new HST_ForceSpawnResultState();
		batch.m_sResultId = primary.m_sResultId + "_secondary_claimant";
		batch.m_sRequestId = "schema70_secondary_quarantine_request";
		batch.m_sManifestId = primary.m_sManifestId;
		batch.m_sManifestHash = primary.m_sManifestHash;
		batch.m_sOperationId = primary.m_sOperationId;
		batch.m_sForceId = primary.m_sForceId;
		batch.m_sProjectionId = primary.m_sProjectionId;
		batch.m_iPriority = primary.m_iPriority;
		batch.m_iMaxRetries = primary.m_iMaxRetries;
		batch.m_iDeadlineSecond = primary.m_iDeadlineSecond;
		batch.m_iCreatedAtSecond = primary.m_iCreatedAtSecond;
		batch.m_iExpectedSlotCount = primary.m_iExpectedSlotCount;
		batch.m_iAttemptGeneration = primary.m_iAttemptGeneration;
		batch.m_iLifecycleRevision = primary.m_iLifecycleRevision;
		foreach (HST_ForceSpawnSlotResultState source : primary.m_aSlotResults)
		{
			if (!source)
				continue;
			HST_ForceSpawnSlotResultState slot
				= new HST_ForceSpawnSlotResultState();
			slot.m_sSlotId = source.m_sSlotId;
			slot.m_sSlotKind = source.m_sSlotKind;
			slot.m_sProjectionId = primary.m_sProjectionId;
			slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED;
			slot.m_iLifecycleRevision = source.m_iLifecycleRevision;
			slot.m_bEverAlive = source.m_bEverAlive;
			batch.m_aSlotResults.Insert(slot);
		}
		return batch;
	}

	protected HST_ActiveGroupState BuildSecondaryQuarantineGroup(
		HST_ActiveGroupState primary,
		HST_ForceSpawnResultState secondaryBatch)
	{
		if (!primary || !secondaryBatch)
			return null;
		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = primary.m_sGroupId + "_secondary_claimant";
		group.m_sOperationId = primary.m_sOperationId;
		group.m_sManifestId = primary.m_sManifestId;
		group.m_sSpawnResultId = secondaryBatch.m_sResultId;
		group.m_sForceId = primary.m_sForceId;
		group.m_sProjectionId = primary.m_sProjectionId;
		group.m_sZoneId = primary.m_sZoneId;
		group.m_sFactionKey = primary.m_sFactionKey;
		group.m_sEnemyOrderId = "";
		group.m_sPrefab = primary.m_sPrefab;
		group.m_vPosition = primary.m_vPosition;
		group.m_vSourcePosition = primary.m_vSourcePosition;
		group.m_vTargetPosition = primary.m_vTargetPosition;
		group.m_sRouteId = primary.m_sRouteId;
		group.m_iInfantryCount = primary.m_iInfantryCount;
		group.m_iOriginalInfantryCount = primary.m_iOriginalInfantryCount;
		group.m_iLastSeenAliveCount = primary.m_iLastSeenAliveCount;
		group.m_iSurvivorInfantryCount = primary.m_iSurvivorInfantryCount;
		group.m_iDurableLivingInfantryCount
			= primary.m_iDurableLivingInfantryCount;
		group.m_iLifecycleRevision = primary.m_iLifecycleRevision;
		return group;
	}

	protected bool StageQuarantineProcessEvidence(
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string label,
		int nowSecond,
		out string processSlotId)
	{
		processSlotId = "";
		if (!batch || !group || label.IsEmpty())
			return false;
		HST_ForceSpawnSlotResultState processSlot;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && !(slot.m_eStatus
				== HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				&& slot.m_bCasualtyConfirmed))
			{
				processSlot = slot;
				break;
			}
		}
		if (!processSlot)
			return false;
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS;
		batch.m_bStrategicProjectionHeld = false;
		batch.m_bCancelRequested = false;
		batch.m_sNativeGroupId = "schema70_" + label + "_native_group";
		batch.m_iNextAttemptSecond = nowSecond + 19;
		batch.m_iCompletedAtSecond = Math.Max(1, nowSecond - 1);
		batch.m_iUpdatedAtSecond = Math.Max(0, nowSecond - 2);
		batch.m_iStrategicHoldSinceSecond = 0;
		processSlotId = processSlot.m_sSlotId;
		processSlot.m_eStatus
			= HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
		processSlot.m_sSpawnedPrefab = "schema70_" + label + "_prefab";
		processSlot.m_sEntityId = "schema70_" + label + "_entity";
		processSlot.m_sAssignedVehicleEntityId
			= "schema70_" + label + "_vehicle";
		processSlot.m_sNativeGroupId = batch.m_sNativeGroupId;
		processSlot.m_bAliveVerified = true;
		processSlot.m_bFactionVerified = true;
		processSlot.m_bGroupVerified = true;
		processSlot.m_bGameMasterVerified = true;
		processSlot.m_bProjectionVerified = true;
		processSlot.m_bSeatVerified = true;
		processSlot.m_iUpdatedAtSecond = Math.Max(0, nowSecond - 2);
		group.m_bSpawnedEntity = true;
		group.m_bSpawnAttempted = true;
		group.m_sRuntimeEntityId = "schema70_" + label + "_group_entity";
		group.m_iSpawnedAgentCount = 2;
		group.m_iAssignedWaypointCount = 3;
		group.m_sRuntimeStatus = "schema70_" + label + "_process_live";
		group.m_sSpawnFailureReason = "";
		return true;
	}

	protected bool IsQuarantineRuntimeRevisionReplayExact(
		HST_ForceSpawnResultState firstBatch,
		HST_ActiveGroupState firstGroup,
		HST_CampaignState replay,
		string batchId,
		string groupId)
	{
		if (!firstBatch || !firstGroup || !replay)
			return false;
		HST_ForceSpawnResultState replayBatch = replay.FindForceSpawnResult(batchId);
		HST_ActiveGroupState replayGroup = replay.FindActiveGroup(groupId);
		return replayBatch && replayGroup
			&& replayBatch.m_iAttemptGeneration
				== firstBatch.m_iAttemptGeneration
			&& replayBatch.m_iLifecycleRevision
				== firstBatch.m_iLifecycleRevision
			&& replayGroup.m_iLifecycleRevision
				== firstGroup.m_iLifecycleRevision;
	}

	protected void ProveOrphanRuntimeQuarantine(
		HST_EnemyGarrisonRebuildOperationProofReport report)
	{
		HST_EnemyGarrisonRebuildOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("orphan_runtime_quarantine");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sOrphanRuntimeEvidence = m_Fixtures.Failure(fixture);
			return;
		}
		string casualtySlotId;
		if (!ConfirmOneStrategicCasualty(fixture, casualtySlotId))
		{
			report.m_sOrphanRuntimeEvidence
				= "orphan runtime proof could not establish a retired casualty";
			return;
		}
		HST_ForceSpawnSlotResultState liveSlot;
		foreach (HST_ForceSpawnSlotResultState candidate : fixture.m_Batch.m_aSlotResults)
		{
			if (candidate && candidate.m_sSlotKind
				== HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				&& candidate.m_sSlotId != casualtySlotId)
			{
				liveSlot = candidate;
				break;
			}
		}
		if (!liveSlot)
		{
			report.m_sOrphanRuntimeEvidence
				= "orphan runtime proof lacks a second exact member slot";
			return;
		}

		fixture.m_Operation.m_eMaterializationState
			= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		fixture.m_Operation.m_ePositionAuthority
			= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		fixture.m_Batch.m_eStatus
			= HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS;
		fixture.m_Batch.m_bStrategicProjectionHeld = false;
		fixture.m_Batch.m_sNativeGroupId = "orphan_runtime_native_group";
		fixture.m_Batch.m_iNextAttemptSecond
			= fixture.m_State.m_iElapsedSeconds + 17;
		fixture.m_Batch.m_iCompletedAtSecond
			= fixture.m_State.m_iElapsedSeconds - 3;
		fixture.m_Batch.m_iStrategicHoldSinceSecond = 0;
		fixture.m_Batch.m_iUpdatedAtSecond
			= fixture.m_State.m_iElapsedSeconds - 2;
		fixture.m_Batch.m_iLastLifecycleSecond
			= fixture.m_State.m_iElapsedSeconds - 2;

		liveSlot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
		liveSlot.m_sSpawnedPrefab = "orphan_runtime_member_prefab";
		liveSlot.m_sEntityId = "orphan_runtime_member_entity";
		liveSlot.m_sAssignedVehicleEntityId = "orphan_runtime_vehicle_entity";
		liveSlot.m_sNativeGroupId = "orphan_runtime_native_group";
		liveSlot.m_bAliveVerified = true;
		liveSlot.m_bFactionVerified = true;
		liveSlot.m_bGroupVerified = true;
		liveSlot.m_bGameMasterVerified = true;
		liveSlot.m_bProjectionVerified = true;
		liveSlot.m_bSeatVerified = true;
		liveSlot.m_iUpdatedAtSecond = fixture.m_State.m_iElapsedSeconds - 2;

		fixture.m_Group.m_bSpawnedEntity = true;
		fixture.m_Group.m_bSpawnAttempted = true;
		fixture.m_Group.m_sRuntimeEntityId = "orphan_runtime_group_entity";
		fixture.m_Group.m_iSpawnedAgentCount = 2;
		fixture.m_Group.m_iAssignedWaypointCount = 3;
		fixture.m_Group.m_sRuntimeStatus = "orphan_runtime_process_live";
		fixture.m_Group.m_sSpawnFailureReason = "";

		int orderIndex = fixture.m_State.m_aEnemyOrders.Find(fixture.m_Order);
		if (orderIndex < 0)
		{
			report.m_sOrphanRuntimeEvidence
				= "orphan runtime proof could not remove its reciprocal order";
			return;
		}
		fixture.m_State.m_aEnemyOrders.Remove(orderIndex);

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_EnemyGarrisonRebuildOrphanRuntimeExpectation expectation
			= new HST_EnemyGarrisonRebuildOrphanRuntimeExpectation();
		expectation.m_sOperationId = fixture.m_Operation.m_sOperationId;
		expectation.m_sManifestId = fixture.m_Manifest.m_sManifestId;
		expectation.m_sBatchId = fixture.m_Batch.m_sResultId;
		expectation.m_sGroupId = fixture.m_Group.m_sGroupId;
		expectation.m_sLiveSlotId = liveSlot.m_sSlotId;
		expectation.m_sCasualtySlotId = casualtySlotId;
		expectation.m_iRestoreSecond = saveData.m_iElapsedSeconds;
		HST_OperationRecordState savedOperation = FindOperation(
			saveData.m_aOperations,
			expectation.m_sOperationId);
		HST_ForceSpawnResultState savedBatch = FindBatch(
			saveData.m_aForceSpawnResults,
			expectation.m_sBatchId);
		HST_ActiveGroupState savedGroup = FindGroup(
			saveData.m_aActiveGroups,
			expectation.m_sGroupId);
		if (!savedOperation || !savedBatch || !savedGroup)
		{
			report.m_sOrphanRuntimeEvidence
				= "orphan runtime proof save copy is incomplete";
			return;
		}
		expectation.m_iOperationRevisionBefore = savedOperation.m_iRevision;
		expectation.m_iBatchAttemptGenerationBefore
			= savedBatch.m_iAttemptGeneration;
		expectation.m_iBatchLifecycleRevisionBefore
			= savedBatch.m_iLifecycleRevision;
		expectation.m_iGroupLifecycleRevisionBefore
			= savedGroup.m_iLifecycleRevision;

		HST_EnemyGarrisonRebuildSaveValidationService validator
			= new HST_EnemyGarrisonRebuildSaveValidationService();
		validator.PrepareBeforeGenericNormalization(
			saveData,
			HST_CampaignState.SCHEMA_VERSION);
		bool firstExact = IsOrphanRuntimeQuarantineExact(
			saveData,
			expectation);
		int operationRevisionAfterFirst = savedOperation.m_iRevision;
		int batchAttemptAfterFirst = savedBatch.m_iAttemptGeneration;
		int batchRevisionAfterFirst = savedBatch.m_iLifecycleRevision;
		int groupRevisionAfterFirst = savedGroup.m_iLifecycleRevision;
		validator.Normalize(saveData, HST_CampaignState.SCHEMA_VERSION);
		bool secondExact = IsOrphanRuntimeQuarantineExact(
			saveData,
			expectation)
			&& savedOperation.m_iRevision == operationRevisionAfterFirst
			&& savedBatch.m_iAttemptGeneration == batchAttemptAfterFirst
			&& savedBatch.m_iLifecycleRevision == batchRevisionAfterFirst
			&& savedGroup.m_iLifecycleRevision == groupRevisionAfterFirst;

		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		HST_ForceSpawnQueueTickResult tick = queue.AcquireWork(
			saveData.m_aForceSpawnResults,
			saveData.m_aForceManifests,
			saveData.m_iElapsedSeconds);
		bool queueHeldExact = tick && !tick.m_bStateChanged
			&& tick.m_iBatchesSelected == 0
			&& tick.m_iSlotsSelected == 0
			&& tick.m_aWorkItems.IsEmpty()
			&& savedBatch.m_bStrategicProjectionHeld;
		report.m_bOrphanRuntimeQuarantineExact = firstExact && secondExact
			&& queueHeldExact;
		report.m_sOrphanRuntimeEvidence = string.Format(
			"orphan process runtime | casualty/live %1/%2 | first/second %3/%4 | operation/batch/group revisions %5/%6/%7 | held queue %8",
			casualtySlotId,
			liveSlot.m_sSlotId,
			firstExact,
			secondExact,
			operationRevisionAfterFirst,
			batchRevisionAfterFirst,
			groupRevisionAfterFirst,
			queueHeldExact);
		report.m_sOrphanRuntimeEvidence += string.Format(
			" | batch attempt generation %1",
			batchAttemptAfterFirst);
	}

	protected void ProveSchema70QuarantineRetention(
		HST_EnemyGarrisonRebuildOperationProofReport report)
	{
		// The production retention helper pins only save-quarantined Schema-70
		// aggregates. A malformed current graph must survive queue compaction
		// without granting the helper authority over healthy or historical rows.
		HST_EnemyGarrisonRebuildOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("quarantine_retention");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sRetentionEvidence = m_Fixtures.Failure(fixture);
			return;
		}
		fixture.m_Order.m_iOperationContractVersion
			= HST_EnemyGarrisonRebuildSaveValidationService.QUARANTINED_CONTRACT_VERSION;
		fixture.m_Order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		fixture.m_Order.m_sRuntimeStatus = "exact_garrison_rebuild_quarantined";
		fixture.m_Batch.m_eStatus
			= HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
		fixture.m_Batch.m_iCompletedAtSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Batch.m_iUpdatedAtSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Batch.m_bStrategicProjectionHeld = true;
		fixture.m_Batch.m_bCancelRequested = false;
		HST_ForceSpawnQueueRetentionPins pins
			= new HST_ForceSpawnQueueRetentionPins();
		HST_EnemyGarrisonRebuildRetentionService.AddQuarantinedSpawnPins(
			fixture.m_State,
			pins);
		bool pinsExact = pins.m_aResultIds.Contains(fixture.m_Batch.m_sResultId)
			&& pins.m_aManifestIds.Contains(fixture.m_Manifest.m_sManifestId)
			&& pins.m_aOperationIds.Contains(fixture.m_Operation.m_sOperationId)
			&& pins.m_aForceIds.Contains(fixture.m_Batch.m_sForceId)
			&& pins.m_aProjectionIds.Contains(fixture.m_Batch.m_sProjectionId);
		HST_ForceSpawnQueueMaintenanceResult maintenance
			= fixture.m_Queue.CompactTerminalRows(
				fixture.m_State.m_aForceSpawnResults,
				pins,
				fixture.m_State.m_iElapsedSeconds + 10000);
		bool retained = fixture.m_State.FindForceSpawnResult(
			fixture.m_Batch.m_sResultId) == fixture.m_Batch;
		int pinnedTerminalCount;
		int removedTerminalCount;
		if (maintenance)
		{
			pinnedTerminalCount = maintenance.m_iPinnedTerminalCount;
			removedTerminalCount = maintenance.m_iRemovedTerminalCount;
		}
		report.m_bQuarantineRetentionExact = pinsExact && retained
			&& maintenance && maintenance.m_iPinnedTerminalCount >= 1;
		report.m_sRetentionEvidence = string.Format(
			"pins exact %1 | retained %2 | pinned/removed %3/%4",
			pinsExact,
			retained,
			pinnedTerminalCount,
			removedTerminalCount);
	}

	protected void ProveSelectedOwnershipABA(
		HST_EnemyGarrisonRebuildOperationProofReport report)
	{
		string targetEvidence;
		string sourceEvidence;
		bool targetExact = ProveSelectedOwnershipABACase(
			true,
			targetEvidence);
		bool sourceExact = ProveSelectedOwnershipABACase(
			false,
			sourceEvidence);
		report.m_bSelectedOwnershipABAExact = targetExact && sourceExact;
		report.m_sSelectedOwnershipABAEvidence
			= targetEvidence + " | " + sourceEvidence;
	}

	protected bool ProveSelectedOwnershipABACase(
		bool mutateTarget,
		out string evidence)
	{
		string selectionLabel = "source";
		if (mutateTarget)
			selectionLabel = "target";
		HST_EnemyGarrisonRebuildPlanningProofHarness planningHarness
			= new HST_EnemyGarrisonRebuildPlanningProofHarness();
		HST_CampaignState state = planningHarness.BuildSchema70RebuildState();
		HST_CampaignPreset preset = planningHarness.BuildSchema70Preset();
		HST_ForcePlanningService forcePlanning = new HST_ForcePlanningService();
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		HST_EnemyGarrisonRebuildOperationService rebuild
			= new HST_EnemyGarrisonRebuildOperationService();
		rebuild.SetRuntimeServices(
			queue,
			new HST_ForceSpawnAdapterService(),
			new HST_PhysicalWarService(),
			new HST_GarrisonService());
		HST_EnemyCommanderService commander = new HST_EnemyCommanderService();
		commander.SetExactEnemyQRFAuthorityServices(
			forcePlanning,
			new HST_EnemyQRFOperationService());
		commander.SetExactEnemyGarrisonRebuildAuthorityService(rebuild);
		HST_EnemyDirectorService enemyDirector = new HST_EnemyDirectorService();
		enemyDirector.SetCampaignPreset(preset);
		rebuild.SetEnemyDirectorService(enemyDirector);
		HST_EnemyPreparedAdmissionResult prepared
			= commander.DebugPrepareNextPeriodicDecisionForFaction(
				state,
				preset,
				enemyDirector,
				HST_EnemyPlanningProofService.OCCUPIER_FACTION);
		HST_EnemyPlanningState planning = state.FindEnemyPlanningState(
			HST_EnemyPlanningProofService.OCCUPIER_FACTION);
		bool preparationAccepted = prepared && prepared.m_bAccepted
			&& !prepared.m_bTerminal;
		bool preparedSelectionExact = planning
			&& planning.m_sDisposition == "prepared"
			&& planning.m_eSelectedOrderType
				== HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON;
		bool preparedCapabilityExact = planning
			&& planning.m_sPlanningCapabilityHash.StartsWith("epc70_")
			&& planning.m_sSelectedTargetZoneId
				== HST_EnemyGarrisonRebuildPlanningProofHarness.PROOF_SELECTED_TARGET_ZONE_ID
			&& planning.m_sSelectedSourceZoneId
				== HST_EnemyGarrisonRebuildPlanningProofHarness.PROOF_SELECTED_SOURCE_ZONE_ID;
		bool preparedExact = preparationAccepted && preparedSelectionExact
			&& preparedCapabilityExact;
		if (!preparedExact)
		{
			HST_EEnemyOrderType selectedType;
			string selectedCapabilityHash;
			string preparationFailure;
			if (planning)
			{
				selectedType = planning.m_eSelectedOrderType;
				selectedCapabilityHash = planning.m_sPlanningCapabilityHash;
			}
			if (prepared)
				preparationFailure = prepared.m_sFailureReason;
			evidence = string.Format(
				"selected %1 ABA preparation failed | accepted/terminal/type/hash %2/%3/%4/%5 | reason '%6'",
				selectionLabel,
				prepared && prepared.m_bAccepted,
				prepared && prepared.m_bTerminal,
				selectedType,
				selectedCapabilityHash,
				preparationFailure);
			return false;
		}

		bool pressureMarked = planningHarness.MarkPressureAppliedForProof(planning);
		string originalCapabilityHash = planning.m_sPlanningCapabilityHash;
		string selectedZoneId = planning.m_sSelectedSourceZoneId;
		if (mutateTarget)
			selectedZoneId = planning.m_sSelectedTargetZoneId;
		HST_ZoneState selected = state.FindZone(selectedZoneId);
		string originalOwner;
		int originalRevision;
		if (selected)
		{
			originalOwner = selected.m_sOwnerFactionKey;
			originalRevision = selected.m_iOwnershipRevision;
			selected.m_sOwnerFactionKey
				= HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_OTHER_FACTION_KEY;
			selected.m_iOwnershipRevision++;
			selected.m_sOwnerFactionKey = originalOwner;
			selected.m_iOwnershipRevision++;
		}
		HST_FactionPoolState pool = state.FindFactionPool(
			HST_EnemyPlanningProofService.OCCUPIER_FACTION);
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		HST_EnemyPreparedAdmissionResult consumed
			= commander.DebugConsumePreparedPeriodicDecisionForFaction(
				state,
				preset,
				enemyDirector,
				new HST_SupportRequestService(),
				HST_EnemyPlanningProofService.OCCUPIER_FACTION);
		bool ownershipABAExact = pressureMarked && selected
			&& selected.m_sOwnerFactionKey == originalOwner
			&& selected.m_iOwnershipRevision == originalRevision + 2;
		bool consumeRejectedExact = consumed && consumed.m_bAccepted
			&& consumed.m_bTerminal
			&& consumed.m_sFailureReason
				== "frozen enemy garrison rebuild ownership capability changed before pressure";
		bool planningRejectedExact = planning.m_sDisposition == "rejected"
			&& planning.m_sPlanningCapabilityHash == originalCapabilityHash
			&& state.m_aEnemyOrders.IsEmpty();
		bool noDebitExact = state.m_aEnemyStrategicMutations.IsEmpty()
			&& pool
			&& pool.m_iAttackResources == attackBefore
			&& pool.m_iSupportResources == supportBefore;
		bool rejected = ownershipABAExact && consumeRejectedExact
			&& planningRejectedExact && noDebitExact;
		int selectedRevision;
		string consumeFailure;
		if (selected)
			selectedRevision = selected.m_iOwnershipRevision;
		if (consumed)
			consumeFailure = consumed.m_sFailureReason;
		evidence = string.Format(
			"selected %1 ABA | epc70 %2 | owner restored %3 | revision %4 -> %5 | rejected/no-order/no-debit %6/%7/%8 | reason '%9'",
			selectionLabel,
			originalCapabilityHash.StartsWith("epc70_"),
			selected && selected.m_sOwnerFactionKey == originalOwner,
			originalRevision,
			selectedRevision,
			consumed && consumed.m_bTerminal
				&& planning.m_sDisposition == "rejected",
			state.m_aEnemyOrders.IsEmpty(),
			state.m_aEnemyStrategicMutations.IsEmpty(),
			consumeFailure);
		evidence += string.Format(
			" | pressure marked %1",
			pressureMarked);
		return rejected;
	}

	protected string BuildDeliveryFailureEvidence(
		HST_EnemyGarrisonRebuildOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_Service || !fixture.m_Order)
			return "delivery diagnostic fixture is incomplete";
		string firstFailure = fixture.m_Service.GetFirstDeliveryFailure(
			fixture.m_Order);
		string lastFailure = fixture.m_Service.GetLastDeliveryFailure(
			fixture.m_Order);
		if (firstFailure.IsEmpty())
			firstFailure = "none";
		if (lastFailure.IsEmpty())
			lastFailure = "none";
		return "delivery rejection first " + firstFailure
			+ " | last " + lastFailure
			+ " | " + BuildDeliveryCapacityEvidence(fixture)
			+ " | " + BuildDeliveryTupleEvidence(fixture)
			+ " | " + BuildDeliveryOperationEvidence(fixture);
	}

	protected string BuildDeliveryCapacityEvidence(
		HST_EnemyGarrisonRebuildOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_State || !fixture.m_Order
			|| !fixture.m_Manifest || !fixture.m_Garrisons || !fixture.m_Queue)
			return "link capacity context unavailable";
		HST_ZoneState target = fixture.m_State.FindZone(
			fixture.m_Order.m_sTargetZoneId);
		HST_GarrisonState garrison = fixture.m_State.FindGarrison(
			fixture.m_Order.m_sTargetZoneId,
			fixture.m_Order.m_sFactionKey);
		int living = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		int aggregateInfantry;
		int exactInfantry;
		int activeInfantry;
		int slots;
		int acceptedLinks;
		int inboundLiving = living;
		if (garrison)
		{
			aggregateInfantry = Math.Max(0, garrison.m_iInfantryCount);
			exactInfantry = fixture.m_Garrisons.CountExecutableManifestInfantry(
				fixture.m_State,
				garrison);
			acceptedLinks = CountAcceptedManifest(
				garrison,
				fixture.m_Manifest.m_sManifestId);
			if (acceptedLinks > 0)
				inboundLiving = 0;
		}
		if (target)
		{
			activeInfantry = Math.Max(0, target.m_iActiveInfantryCount);
			slots = target.m_iGarrisonSlots;
		}
		return string.Format(
			"link aggregate/exact/active/living/projected/slots %1/%2/%3/%4/%5/%6 | accepted links %7",
			aggregateInfantry,
			exactInfantry,
			activeInfantry,
			living,
			aggregateInfantry + exactInfantry + activeInfantry + inboundLiving,
			slots,
			acceptedLinks);
	}

	protected string BuildDeliveryTupleEvidence(
		HST_EnemyGarrisonRebuildOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_Order)
			return "delivery tuple unavailable";
		string settlementId = fixture.m_Order.m_sResourceSettlementId;
		string settlementKind = fixture.m_Order.m_sResourceSettlementKind;
		string refundMutationId = fixture.m_Order.m_sResourceRefundMutationId;
		if (settlementId.IsEmpty())
			settlementId = "none";
		if (settlementKind.IsEmpty())
			settlementKind = "none";
		if (refundMutationId.IsEmpty())
			refundMutationId = "none";
		return string.Format(
			"tuple id/kind/refund/applied %1/%2/%3/%4 | accepted/survivors/refund %5/%6/%7",
			settlementId,
			settlementKind,
			refundMutationId,
			fixture.m_Order.m_bResourceSettlementApplied,
			fixture.m_Order.m_iSettlementAcceptedMemberCount,
			fixture.m_Order.m_iSettlementSurvivorMemberCount,
			fixture.m_Order.m_iRefundedSupportResources);
	}

	protected string BuildDeliveryOperationEvidence(
		HST_EnemyGarrisonRebuildOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_State || !fixture.m_Operation || !fixture.m_Order)
			return "delivery operation unavailable";
		HST_ZoneState target = fixture.m_State.FindZone(
			fixture.m_Order.m_sTargetZoneId);
		string owner = "missing";
		int ownershipRevision;
		if (target)
		{
			owner = target.m_sOwnerFactionKey;
			ownershipRevision = target.m_iOwnershipRevision;
		}
		string lifecycle = string.Format(
			"operation settlement/duty/terminal/materialization %1/%2/%3/%4",
			fixture.m_Operation.m_eSettlementState,
			fixture.m_Operation.m_eDutyState,
			fixture.m_Operation.m_eTerminalResult,
			fixture.m_Operation.m_eMaterializationState);
		string route = string.Format(
			" | route %1/%2 | order status/outcome %3/%4",
			fixture.m_Operation.m_fRouteProgressMeters,
			fixture.m_Operation.m_fRouteTotalDistanceMeters,
			fixture.m_Order.m_eStatus,
			fixture.m_Order.m_bOutcomeApplied);
		string ownership = string.Format(
			" | owner/revision current %1/%2 frozen %3/%4",
			owner,
			ownershipRevision,
			fixture.m_Order.m_sFactionKey,
			fixture.m_Order.m_iTargetOwnershipRevision);
		return lifecycle + route + ownership
			+ " | projection reason "
			+ fixture.m_Operation.m_sLastProjectionReason;
	}

	protected bool DriveUntilDelivered(
		HST_EnemyGarrisonRebuildOperationProofFixture fixture,
		int maxTicks)
	{
		if (!m_Fixtures.Ready(fixture))
			return false;
		for (int tick = 0; tick < maxTicks; tick++)
		{
			if (IsDelivered(fixture))
				return true;
			fixture.m_State.m_iElapsedSeconds
				+= HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
			fixture.m_Service.TickOrder(
				fixture.m_State,
				fixture.m_Preset,
				fixture.m_EnemyDirector,
				fixture.m_Order);
		}
		return IsDelivered(fixture);
	}

	protected bool IsDelivered(
		HST_EnemyGarrisonRebuildOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_Order || !fixture.m_Operation)
			return false;
		bool orderDelivered = fixture.m_Order.m_eStatus
				== HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED
			&& fixture.m_Order.m_bResourceSettlementApplied
			&& fixture.m_Order.m_sResourceSettlementKind
				== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_DELIVERY_SETTLEMENT_KIND;
		bool operationHeld = fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& fixture.m_Operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
		return orderDelivered && operationHeld;
	}

	protected bool DriveUntilSettled(
		HST_EnemyGarrisonRebuildOperationProofFixture fixture,
		int maxTicks)
	{
		if (!m_Fixtures.Ready(fixture))
			return false;
		for (int tick = 0; tick < maxTicks; tick++)
		{
			HST_OperationRecordState operation = fixture.m_State.FindOperation(
				fixture.m_Order.m_sOperationId);
			if (operation && operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				return true;
			fixture.m_State.m_iElapsedSeconds
				+= HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
			fixture.m_Service.TickOrder(
				fixture.m_State,
				fixture.m_Preset,
				fixture.m_EnemyDirector,
				fixture.m_Order);
		}
		HST_OperationRecordState finalOperation = fixture.m_State.FindOperation(
			fixture.m_Order.m_sOperationId);
		return finalOperation && finalOperation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
	}

	protected bool ConfirmOneStrategicCasualty(
		HST_EnemyGarrisonRebuildOperationProofFixture fixture,
		out string casualtyId)
	{
		casualtyId = "";
		if (!m_Fixtures.Ready(fixture)
			|| fixture.m_Manifest.m_iAcceptedMemberCount < 2)
			return false;
		casualtyId = fixture.m_Queue.SelectStrategicLivingMemberSlotId(
			fixture.m_Batch,
			fixture.m_Operation.m_iDeterministicSeed + 29);
		HST_ForceSpawnQueueCallbackResult casualty
			= fixture.m_Queue.ConfirmStrategicMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				casualtyId,
				fixture.m_State.m_iElapsedSeconds + 1,
				"focused exact garrison rebuild casualty");
		int living = fixture.m_Manifest.m_iAcceptedMemberCount - 1;
		ApplyGroupRoster(fixture.m_Group, living);
		return casualty && casualty.m_bAccepted
			&& fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch)
				== living;
	}

	protected void PrepareSyntheticSuccessfulProjection(
		HST_EnemyGarrisonRebuildOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_Batch || !fixture.m_Manifest)
			return;
		fixture.m_Batch.m_eStatus
			= HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
		fixture.m_Batch.m_bStrategicProjectionHeld = false;
		fixture.m_Batch.m_sNativeGroupId
			= "enemy_garrison_rebuild_proof_native_" + fixture.m_Order.m_sOrderId;
		fixture.m_Batch.m_sTerminalReason
			= "all surviving exact manifest slots registered and verified";
		fixture.m_Batch.m_sLastFailureReason = "";
		fixture.m_Batch.m_iSuccessfulHandoffCount = 1;
		fixture.m_Batch.m_iCompletedAtSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Batch.m_iUpdatedAtSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Batch.m_iLifecycleRevision++;
		fixture.m_Batch.m_iLastLifecycleSecond = fixture.m_State.m_iElapsedSeconds;
		foreach (HST_ForceSpawnSlotResultState slot : fixture.m_Batch.m_aSlotResults)
		{
			if (!slot || slot.m_bCasualtyConfirmed)
				continue;
			slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
			slot.m_sSpawnedPrefab = ResolveFixtureSlotPrefab(
				fixture.m_Manifest,
				slot);
			slot.m_sEntityId
				= "enemy_garrison_rebuild_proof_entity_" + slot.m_sSlotId;
			slot.m_sNativeGroupId = fixture.m_Batch.m_sNativeGroupId;
			slot.m_bAliveVerified = true;
			slot.m_bFactionVerified = true;
			slot.m_bGroupVerified = true;
			slot.m_bGameMasterVerified = true;
			slot.m_bProjectionVerified = true;
			slot.m_iLifecycleRevision = Math.Max(1, slot.m_iLifecycleRevision);
			if (slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
				slot.m_bEverAlive = true;
		}
	}

	protected string ResolveFixtureSlotPrefab(
		HST_ForceManifestState manifest,
		HST_ForceSpawnSlotResultState slot)
	{
		if (!manifest || !slot)
			return "";
		if (slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP
			&& manifest.m_aGroups.Count() == 1 && manifest.m_aGroups[0])
			return manifest.m_aGroups[0].m_sPrefab;
		HST_ForceManifestMemberState member = manifest.FindMemberSlot(slot.m_sSlotId);
		if (member)
			return member.m_sPrefab;
		return "";
	}

	protected HST_ForceSpawnSlotResultState FindFirstRegisteredMemberSlot(
		HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return null;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotKind
				== HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				&& slot.m_eStatus
					== HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
				&& !slot.m_sEntityId.IsEmpty() && slot.m_bEverAlive
				&& !slot.m_bCasualtyConfirmed)
				return slot;
		}
		return null;
	}

	protected void ApplyGroupRoster(HST_ActiveGroupState group, int living)
	{
		if (!group)
			return;
		int bounded = Math.Max(0, Math.Min(group.m_iOriginalInfantryCount, living));
		group.m_iInfantryCount = bounded;
		group.m_iDurableLivingInfantryCount = bounded;
		group.m_iLastSeenAliveCount = bounded;
		group.m_iSurvivorInfantryCount = bounded;
	}

	protected bool IsOrphanRuntimeQuarantineExact(
		HST_CampaignSaveData saveData,
		HST_EnemyGarrisonRebuildOrphanRuntimeExpectation expectation)
	{
		if (!saveData || !expectation)
			return false;
		HST_OperationRecordState operation = FindOperation(
			saveData.m_aOperations,
			expectation.m_sOperationId);
		HST_ForceManifestState manifest = FindManifest(
			saveData.m_aForceManifests,
			expectation.m_sManifestId);
		HST_ForceSpawnResultState batch = FindBatch(
			saveData.m_aForceSpawnResults,
			expectation.m_sBatchId);
		HST_ActiveGroupState group = FindGroup(
			saveData.m_aActiveGroups,
			expectation.m_sGroupId);
		if (!operation || !manifest || !batch || !group)
			return false;
		HST_ForceSpawnSlotResultState liveSlot = batch.FindSlotResult(
			expectation.m_sLiveSlotId);
		HST_ForceSpawnSlotResultState casualtySlot = batch.FindSlotResult(
			expectation.m_sCasualtySlotId);
		if (!liveSlot || !casualtySlot)
			return false;

		bool operationExact = operation.m_eType
				== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_GARRISON_REBUILD
			&& operation.m_iContractVersion
				== HST_EnemyGarrisonRebuildSaveValidationService.QUARANTINED_CONTRACT_VERSION
			&& operation.m_sLastProjectionReason
				== HST_EnemyGarrisonRebuildOrphanRuntimeExpectation.DURABLE_REASON
			&& operation.m_iRevision
				== expectation.m_iOperationRevisionBefore + 1;
		bool batchLifecycleExact = batch.m_eStatus
				== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			&& batch.m_bStrategicProjectionHeld
			&& batch.m_sNativeGroupId.IsEmpty()
			&& batch.m_iNextAttemptSecond == 0
			&& batch.m_iCompletedAtSecond == 0;
		bool batchClockExact = batch.m_iStrategicHoldSinceSecond
				== expectation.m_iRestoreSecond
			&& batch.m_iUpdatedAtSecond == expectation.m_iRestoreSecond
			&& batch.m_iLastLifecycleSecond == expectation.m_iRestoreSecond
			&& batch.m_iAttemptGeneration
				== expectation.m_iBatchAttemptGenerationBefore + 1
			&& batch.m_iLifecycleRevision
				== expectation.m_iBatchLifecycleRevisionBefore + 1;

		bool liveSlotStateExact = liveSlot.m_eStatus
				== HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED
			&& liveSlot.m_iUpdatedAtSecond == expectation.m_iRestoreSecond
			&& liveSlot.m_sSpawnedPrefab.IsEmpty()
			&& liveSlot.m_sEntityId.IsEmpty()
			&& liveSlot.m_sAssignedVehicleEntityId.IsEmpty()
			&& liveSlot.m_sNativeGroupId.IsEmpty();
		bool liveSlotVerificationCleared = !liveSlot.m_bAliveVerified
			&& !liveSlot.m_bFactionVerified
			&& !liveSlot.m_bGroupVerified
			&& !liveSlot.m_bGameMasterVerified
			&& !liveSlot.m_bProjectionVerified
			&& !liveSlot.m_bSeatVerified;
		bool casualtyStateExact = casualtySlot.m_eStatus
				== HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
			&& casualtySlot.m_bEverAlive
			&& casualtySlot.m_bCasualtyConfirmed
			&& casualtySlot.m_iUpdatedAtSecond == expectation.m_iRestoreSecond;
		bool casualtyProcessCleared = casualtySlot.m_sSpawnedPrefab.IsEmpty()
			&& casualtySlot.m_sEntityId.IsEmpty()
			&& casualtySlot.m_sAssignedVehicleEntityId.IsEmpty()
			&& casualtySlot.m_sNativeGroupId.IsEmpty()
			&& !casualtySlot.m_bAliveVerified
			&& !casualtySlot.m_bFactionVerified;

		bool groupProcessCleared = !group.m_bSpawnedEntity
			&& !group.m_bSpawnAttempted
			&& group.m_sRuntimeEntityId.IsEmpty()
			&& group.m_iSpawnedAgentCount == 0
			&& group.m_iAssignedWaypointCount == 0;
		bool groupQuarantineExact = group.m_sRuntimeStatus
				== "exact_garrison_rebuild_quarantined"
			&& group.m_sSpawnFailureReason
				== HST_EnemyGarrisonRebuildOrphanRuntimeExpectation.DURABLE_REASON
			&& group.m_iLifecycleRevision
				== expectation.m_iGroupLifecycleRevisionBefore + 1;
		bool graphExact = saveData.m_aEnemyOrders.IsEmpty()
			&& operation.m_sManifestId == manifest.m_sManifestId
			&& operation.m_sSpawnResultId == batch.m_sResultId
			&& operation.m_sGroupId == group.m_sGroupId;
		bool batchExact = batchLifecycleExact && batchClockExact;
		bool slotExact = liveSlotStateExact && liveSlotVerificationCleared
			&& casualtyStateExact && casualtyProcessCleared;
		bool groupExact = groupProcessCleared && groupQuarantineExact;
		return operationExact && batchExact && slotExact
			&& groupExact && graphExact;
	}

	protected bool IsSettlementCrashWindowExact(
		HST_CampaignState state,
		HST_EnemyGarrisonRebuildCrashWindowExpectation expectation)
	{
		if (!state || !expectation)
			return false;
		HST_EnemyOrderState order = state.FindEnemyOrder(expectation.m_sOrderId);
		HST_OperationRecordState operation = state.FindOperation(
			expectation.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(
			expectation.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(
			expectation.m_sBatchId);
		HST_ActiveGroupState group = state.FindActiveGroup(expectation.m_sGroupId);
		HST_FactionPoolState pool = state.FindFactionPool(
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool rowsExact = order && operation && manifest && batch && group && pool;
		if (!rowsExact)
			return false;

		bool graphExact = CountEnemyOrderId(state, expectation.m_sOrderId) == 1
			&& CountOperationId(state, expectation.m_sOperationId) == 1
			&& CountManifestId(state, expectation.m_sManifestId) == 1
			&& CountBatchId(state, expectation.m_sBatchId) == 1
			&& CountGroupId(state, expectation.m_sGroupId) == 1;
		bool runtimeLinksExact = order.m_sOperationId == operation.m_sOperationId
			&& order.m_sManifestId == manifest.m_sManifestId
			&& order.m_sSpawnResultId == batch.m_sResultId
			&& order.m_sGroupId == group.m_sGroupId
			&& batch.m_sOperationId == operation.m_sOperationId
			&& group.m_sOperationId == operation.m_sOperationId;
		bool staleTailExact = order.m_eStatus == expectation.m_eStaleOrderStatus
			&& order.m_sRuntimeStatus == expectation.m_sStaleRuntimeStatus
			&& order.m_sResolutionKind == expectation.m_sStaleResolutionKind
			&& order.m_sFailureReason == expectation.m_sStaleFailureReason
			&& order.m_iResolvedAtSecond == expectation.m_iStaleResolvedAtSecond;
		bool staleFlagsExact = order.m_bPhysicalized
				== expectation.m_bStalePhysicalized
			&& order.m_bAbstractResolved
				== expectation.m_bStaleAbstractResolved
			&& order.m_bOutcomeApplied == expectation.m_bStaleOutcomeApplied;

		bool receiptIdentityExact = order.m_bResourceSettlementApplied
			&& order.m_sResourceSettlementId == expectation.m_sSettlementId
			&& order.m_sResourceSettlementKind == expectation.m_sSettlementKind
			&& order.m_sResourceDebitMutationId == expectation.m_sDebitMutationId
			&& order.m_sResourceRefundMutationId
				== expectation.m_sRefundMutationId;
		bool receiptRosterExact = order.m_iSettlementAcceptedMemberCount
				== expectation.m_iAccepted
			&& order.m_iSettlementSurvivorMemberCount
				== expectation.m_iSurvivors
			&& order.m_iRefundedAttackResources == 0
			&& order.m_iRefundedSupportResources
				== expectation.m_iSupportRefund;
		bool receiptValid = HST_EnemyGarrisonRebuildSaveValidationService
			.ValidateSettledResourceRefundAuthority(
				state.m_aEnemyStrategicMutations,
				order).IsEmpty();
		bool resourceExact = pool.m_iSupportResources
				== expectation.m_iExpectedSupport
			&& state.m_aEnemyStrategicMutations.Count()
				== expectation.m_iExpectedMutationCount
			&& CountMutationId(state, expectation.m_sDebitMutationId) == 1
			&& CountMutationId(state, expectation.m_sRefundMutationId) == 1;

		bool operationIntentExact = operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED
			&& operation.m_sSettlementId == expectation.m_sSettlementId
			&& operation.m_sTerminalReason == expectation.m_sTerminalReason
			&& operation.m_iSettledAtSecond == expectation.m_iSettledAtSecond
			&& operation.m_iSettledAtSecond > 0;
		bool operationWindowExact = operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
			&& operation.m_eDutyState
				!= HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		if (expectation.m_bSettledBeforeOrderTail)
		{
			operationWindowExact = operation.m_eSettlementState
					== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
				&& operation.m_eDutyState
					== HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
				&& operation.m_eResumeDutyState
					== HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
				&& operation.m_eMaterializationState
					== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
				&& operation.m_ePositionAuthority
					== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		}
		bool graphAndTailExact = graphExact && runtimeLinksExact
			&& staleTailExact && staleFlagsExact;
		bool receiptExact = receiptIdentityExact && receiptRosterExact
			&& receiptValid && resourceExact;
		return graphAndTailExact && receiptExact
			&& operationIntentExact && operationWindowExact;
	}

	protected bool IsSettlementCrashTerminalExact(
		HST_CampaignState state,
		HST_EnemyGarrisonRebuildCrashWindowExpectation expectation)
	{
		if (!state || !expectation)
			return false;
		HST_EnemyOrderState order = state.FindEnemyOrder(expectation.m_sOrderId);
		HST_OperationRecordState operation = state.FindOperation(
			expectation.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(
			expectation.m_sManifestId);
		HST_FactionPoolState pool = state.FindFactionPool(
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!order || !operation || !manifest || !pool)
			return false;
		bool orderLifecycleExact = order.m_eStatus
				== HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& order.m_sRuntimeStatus == "resolved_exact_rebuild_terminal"
			&& order.m_sResolutionKind == expectation.m_sSettlementKind
			&& order.m_sFailureReason == expectation.m_sTerminalReason
			&& order.m_iResolvedAtSecond == expectation.m_iSettledAtSecond;
		bool orderFlagsExact = !order.m_bPhysicalized
			&& !order.m_bAbstractResolved
			&& !order.m_bOutcomeApplied
			&& order.m_bResourceSettlementApplied;
		bool operationLifecycleExact = operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			&& operation.m_eResumeDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			&& operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
			&& operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		bool operationIntentExact = operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED
			&& operation.m_sSettlementId == expectation.m_sSettlementId
			&& operation.m_sTerminalReason == expectation.m_sTerminalReason
			&& operation.m_iSettledAtSecond == expectation.m_iSettledAtSecond;
		bool receiptExact = order.m_sResourceSettlementId
				== expectation.m_sSettlementId
			&& order.m_sResourceSettlementKind == expectation.m_sSettlementKind
			&& order.m_sResourceDebitMutationId == expectation.m_sDebitMutationId
			&& order.m_sResourceRefundMutationId
				== expectation.m_sRefundMutationId
			&& order.m_iSettlementAcceptedMemberCount
				== expectation.m_iAccepted
			&& order.m_iSettlementSurvivorMemberCount
				== expectation.m_iSurvivors;
		bool refundExact = order.m_iRefundedAttackResources == 0
			&& order.m_iRefundedSupportResources == expectation.m_iSupportRefund
			&& pool.m_iSupportResources == expectation.m_iExpectedSupport
			&& state.m_aEnemyStrategicMutations.Count()
				== expectation.m_iExpectedMutationCount
			&& CountMutationId(state, expectation.m_sDebitMutationId) == 1
			&& CountMutationId(state, expectation.m_sRefundMutationId) == 1;
		bool receiptValid = HST_EnemyGarrisonRebuildSaveValidationService
			.ValidateSettledResourceRefundAuthority(
				state.m_aEnemyStrategicMutations,
				order).IsEmpty();
		bool terminalGraphExact = CountEnemyOrderId(state, expectation.m_sOrderId) == 1
			&& CountOperationId(state, expectation.m_sOperationId) == 1
			&& CountManifestId(state, expectation.m_sManifestId) == 1
			&& CountBatchId(state, expectation.m_sBatchId) == 0
			&& CountGroupId(state, expectation.m_sGroupId) == 0;
		HST_GarrisonState garrison = state.FindGarrison(
			order.m_sTargetZoneId,
			order.m_sFactionKey);
		bool noHeldRoster = !garrison
			|| CountAcceptedManifest(garrison, expectation.m_sManifestId) == 0;
		bool lifecycleExact = orderLifecycleExact && orderFlagsExact
			&& operationLifecycleExact && operationIntentExact;
		bool authorityExact = receiptExact && refundExact && receiptValid
			&& terminalGraphExact && noHeldRoster;
		return lifecycleExact && authorityExact;
	}

	protected bool IsDeliveredRestoreExact(
		HST_CampaignState state,
		string orderId,
		string manifestId,
		string settlementId,
		int expectedLiving,
		string casualtyId)
	{
		if (!state)
			return false;
		HST_EnemyOrderState order = state.FindEnemyOrder(orderId);
		if (!order)
			return false;
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(
			order.m_sSpawnResultId);
		HST_ActiveGroupState group = state.FindActiveGroup(order.m_sGroupId);
		HST_GarrisonState garrison = state.FindGarrison(
			order.m_sTargetZoneId,
			order.m_sFactionKey);
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		bool orderExact = order.m_iOperationContractVersion
				== HST_EnemyGarrisonRebuildOperationService.EXACT_CONTRACT_VERSION
			&& order.m_eStatus
				== HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED
			&& order.m_bResourceSettlementApplied
			&& order.m_sResourceSettlementId == settlementId
			&& order.m_sResourceSettlementKind
				== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_DELIVERY_SETTLEMENT_KIND
			&& order.m_iRefundedSupportResources == 0;
		bool rowsExact = operation && manifest && batch && group && garrison;
		if (!rowsExact)
			return false;
		bool operationExact = operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
		bool projectionExact = manifest.m_sManifestId == manifestId
			&& batch.m_bStrategicProjectionHeld
			&& !batch.m_bCancelRequested
			&& queue.CountStrategicLivingMemberSlots(batch) == expectedLiving
			&& IsConfirmedRetiredMemberSlot(batch, casualtyId);
		bool garrisonExact = garrison.m_iInfantryCount
				== HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_AGGREGATE_INFANTRY
			&& CountAcceptedManifest(garrison, manifestId) == 1;
		return orderExact && operationExact && projectionExact && garrisonExact;
	}

	protected bool IsConfirmedRetiredMemberSlot(
		HST_ForceSpawnResultState batch,
		string slotId)
	{
		if (!batch || slotId.IsEmpty())
			return false;
		HST_ForceSpawnSlotResultState slot = batch.FindSlotResult(slotId);
		return slot && slot.m_sSlotKind
			== HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
			&& slot.m_eStatus
				== HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
			&& slot.m_bEverAlive && slot.m_bCasualtyConfirmed;
	}

	protected bool IsSchema70QuarantineExact(
		HST_CampaignState state,
		HST_EnemyGarrisonRebuildMalformedQuarantineExpectation expectation)
	{
		if (!state || !expectation)
			return false;
		HST_EnemyOrderState order = state.FindEnemyOrder(expectation.m_sOrderId);
		HST_OperationRecordState operation = state.FindOperation(
			expectation.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(
			expectation.m_sManifestId);
		HST_FactionPoolState pool = state.FindFactionPool(
			HST_EnemyGarrisonRebuildOperationProofFixtureFactory.PROOF_FACTION_KEY);
		HST_GarrisonState garrison;
		if (order)
			garrison = state.FindGarrison(order.m_sTargetZoneId, order.m_sFactionKey);
		bool rowsExact = order && operation && manifest && pool;
		if (!rowsExact)
			return false;
		bool orderExact = order.m_iOperationContractVersion
				== HST_EnemyGarrisonRebuildSaveValidationService.QUARANTINED_CONTRACT_VERSION
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& order.m_sRuntimeStatus == "exact_garrison_rebuild_quarantined"
			&& !order.m_bResourceSettlementApplied
			&& !order.m_bResourceRefundApplied
			&& order.m_sResourceRefundMutationId.IsEmpty();
		bool operationExact = operation.m_iContractVersion
				== HST_EnemyGarrisonRebuildSaveValidationService.QUARANTINED_CONTRACT_VERSION
			&& operation.m_sLastProjectionReason == order.m_sFailureReason;
		bool primaryRuntimeExact = IsQuarantinedRuntimeClaimantExact(
			state,
			expectation.m_sPrimaryBatchId,
			expectation.m_sPrimaryGroupId,
			expectation.m_sPrimaryProcessSlotId,
			expectation.m_sManifestId,
			order.m_sFailureReason);
		bool secondaryRuntimeExact = IsQuarantinedRuntimeClaimantExact(
			state,
			expectation.m_sSecondaryBatchId,
			expectation.m_sSecondaryGroupId,
			expectation.m_sSecondaryProcessSlotId,
			expectation.m_sManifestId,
			order.m_sFailureReason);
		bool runtimeExact = primaryRuntimeExact && secondaryRuntimeExact;
		bool resourceExact = state.m_aEnemyStrategicMutations.Count() == 1
			&& CountMutationId(state, order.m_sResourceDebitMutationId) == 1
			&& pool.m_iSupportResources == expectation.m_iExpectedSupport;
		bool garrisonExact = !garrison
			|| !garrison.m_aAcceptedManifestIds.Contains(expectation.m_sManifestId);
		bool authorityGraphExact = CountEnemyOrderId(
				state,
				expectation.m_sOrderId) == 1
			&& CountOperationId(state, expectation.m_sOperationId) == 1
			&& CountManifestId(state, expectation.m_sManifestId) == 1;
		bool claimantGraphExact = CountBatchId(
				state,
				expectation.m_sPrimaryBatchId) == 1
			&& CountBatchId(state, expectation.m_sSecondaryBatchId) == 1
			&& CountGroupId(state, expectation.m_sPrimaryGroupId) == 1
			&& CountGroupId(state, expectation.m_sSecondaryGroupId) == 1
			&& state.m_aForceSpawnResults.Count() == 2
			&& state.m_aActiveGroups.Count() == 2;
		bool graphExact = authorityGraphExact && claimantGraphExact;
		return orderExact && operationExact && runtimeExact
			&& resourceExact && garrisonExact && graphExact;
	}

	protected bool IsQuarantinedRuntimeClaimantExact(
		HST_CampaignState state,
		string batchId,
		string groupId,
		string processSlotId,
		string manifestId,
		string durableReason)
	{
		if (!state || batchId.IsEmpty() || groupId.IsEmpty()
			|| processSlotId.IsEmpty() || durableReason.IsEmpty())
			return false;
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(batchId);
		HST_ActiveGroupState group = state.FindActiveGroup(groupId);
		HST_ForceManifestState manifest = state.FindForceManifest(manifestId);
		if (!batch || !group || !manifest)
			return false;
		HST_ForceSpawnSlotResultState slot = batch.FindSlotResult(processSlotId);
		if (!slot)
			return false;
		bool batchExact = batch.m_eStatus
				== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			&& batch.m_bStrategicProjectionHeld
			&& !batch.m_bCancelRequested
			&& batch.m_sNativeGroupId.IsEmpty()
			&& batch.m_iNextAttemptSecond == 0
			&& batch.m_iCompletedAtSecond == 0;
		bool slotProcessCleared = slot.m_eStatus
				== HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED
			&& slot.m_sSpawnedPrefab.IsEmpty()
			&& slot.m_sEntityId.IsEmpty()
			&& slot.m_sAssignedVehicleEntityId.IsEmpty()
			&& slot.m_sNativeGroupId.IsEmpty();
		bool slotVerificationCleared = !slot.m_bAliveVerified
			&& !slot.m_bFactionVerified
			&& !slot.m_bGroupVerified
			&& !slot.m_bGameMasterVerified
			&& !slot.m_bProjectionVerified
			&& !slot.m_bSeatVerified;
		bool groupProcessCleared = !group.m_bSpawnedEntity
			&& !group.m_bSpawnAttempted
			&& group.m_sRuntimeEntityId.IsEmpty()
			&& group.m_iSpawnedAgentCount == 0
			&& group.m_iAssignedWaypointCount == 0;
		bool groupQuarantineExact = group.m_sRuntimeStatus
				== "exact_garrison_rebuild_quarantined"
			&& group.m_sSpawnFailureReason == durableReason;
		HST_EnemyGarrisonRebuildQueueEligibilityProofHarness queue
			= new HST_EnemyGarrisonRebuildQueueEligibilityProofHarness();
		bool nonExecutable = !queue.IsExecutable(
			batch,
			manifest,
			state.m_iElapsedSeconds);
		bool processExact = slotProcessCleared && slotVerificationCleared
			&& groupProcessCleared && groupQuarantineExact;
		return batchExact && processExact && nonExecutable;
	}

	protected int CountAcceptedManifest(
		HST_GarrisonState garrison,
		string manifestId)
	{
		if (!garrison || manifestId.IsEmpty())
			return 0;
		int count;
		foreach (string candidate : garrison.m_aAcceptedManifestIds)
		{
			if (candidate == manifestId)
				count++;
		}
		return count;
	}

	protected HST_OperationRecordState FindOperation(
		array<ref HST_OperationRecordState> operations,
		string operationId)
	{
		if (!operations || operationId.IsEmpty())
			return null;
		foreach (HST_OperationRecordState operation : operations)
		{
			if (operation && operation.m_sOperationId == operationId)
				return operation;
		}
		return null;
	}

	protected HST_ForceManifestState FindManifest(
		array<ref HST_ForceManifestState> manifests,
		string manifestId)
	{
		if (!manifests || manifestId.IsEmpty())
			return null;
		foreach (HST_ForceManifestState manifest : manifests)
		{
			if (manifest && manifest.m_sManifestId == manifestId)
				return manifest;
		}
		return null;
	}

	protected HST_ForceSpawnResultState FindBatch(
		array<ref HST_ForceSpawnResultState> batches,
		string batchId)
	{
		if (!batches || batchId.IsEmpty())
			return null;
		foreach (HST_ForceSpawnResultState batch : batches)
		{
			if (batch && batch.m_sResultId == batchId)
				return batch;
		}
		return null;
	}

	protected HST_ActiveGroupState FindGroup(
		array<ref HST_ActiveGroupState> groups,
		string groupId)
	{
		if (!groups || groupId.IsEmpty())
			return null;
		foreach (HST_ActiveGroupState group : groups)
		{
			if (group && group.m_sGroupId == groupId)
				return group;
		}
		return null;
	}

	protected int CountMutationId(HST_CampaignState state, string mutationId)
	{
		if (!state || mutationId.IsEmpty())
			return 0;
		int count;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_sMutationId == mutationId)
				count++;
		}
		return count;
	}

	protected int CountEnemyOrderId(HST_CampaignState state, string orderId)
	{
		if (!state || orderId.IsEmpty())
			return 0;
		int count;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (order && order.m_sOrderId == orderId)
				count++;
		}
		return count;
	}

	protected int CountOperationId(HST_CampaignState state, string operationId)
	{
		if (!state || operationId.IsEmpty())
			return 0;
		int count;
		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (operation && operation.m_sOperationId == operationId)
				count++;
		}
		return count;
	}

	protected int CountManifestId(HST_CampaignState state, string manifestId)
	{
		if (!state || manifestId.IsEmpty())
			return 0;
		int count;
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
		{
			if (manifest && manifest.m_sManifestId == manifestId)
				count++;
		}
		return count;
	}

	protected int CountBatchId(HST_CampaignState state, string batchId)
	{
		if (!state || batchId.IsEmpty())
			return 0;
		int count;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && batch.m_sResultId == batchId)
				count++;
		}
		return count;
	}

	protected int CountGroupId(HST_CampaignState state, string groupId)
	{
		if (!state || groupId.IsEmpty())
			return 0;
		int count;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && group.m_sGroupId == groupId)
				count++;
		}
		return count;
	}
}
