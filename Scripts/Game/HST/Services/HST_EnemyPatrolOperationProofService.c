#ifdef ENABLE_DIAG
class HST_EnemyPatrolOperationProofReport
{
	bool m_bAdmissionExact;
	bool m_bReplayRefundExact;
	bool m_bRouteLoopExact;
	bool m_bProjectionRosterExact;
	bool m_bContactHoldExact;
	bool m_bSettlementExact;
	bool m_bRestoreExact;
	bool m_bCorruptionExact;
	bool m_bDispatchIsolationExact;
	bool m_bMarkerExact;
	string m_sAdmissionEvidence;
	string m_sReplayRefundEvidence;
	string m_sRouteLoopEvidence;
	string m_sProjectionRosterEvidence;
	string m_sContactHoldEvidence;
	string m_sSettlementEvidence;
	string m_sRestoreEvidence;
	string m_sCorruptionEvidence;
	string m_sDispatchIsolationEvidence;
	string m_sMarkerEvidence;
}

// Proof-only seams expose protected deterministic transitions without replacing
// any production implementation. Native entities and adapter handles are still
// reserved for packaged runtime verification.
class HST_EnemyPatrolOperationProofHarness : HST_EnemyPatrolOperationService
{
	bool UpdateContactForProof(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveGroupState group,
		out bool contactActive,
		out bool contactCleared)
	{
		return UpdatePhysicalContactState(state, operation, group, contactActive, contactCleared);
	}
}

class HST_EnemyPatrolCommanderProofHarness : HST_EnemyCommanderService
{
	bool QueueOrderForProof(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_SupportRequestService support,
		string factionKey,
		HST_ZoneState targetZone,
		HST_EEnemyOrderType orderType,
		string spendMode = "")
	{
		return QueueOrder(
			state,
			preset,
			enemyDirector,
			support,
			factionKey,
			targetZone,
			orderType,
			spendMode);
	}
}

class HST_EnemyPatrolPhysicalWarProofHarness : HST_PhysicalWarService
{
	bool IsExactPatrolGroupForProof(HST_CampaignState state, HST_ActiveGroupState group)
	{
		return IsExactEnemyPatrolGroup(state, group);
	}
}

class HST_EnemyPatrolOperationProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_EnemyDirectorService m_EnemyDirector;
	ref HST_ForcePlanningService m_Planning;
	ref HST_ForceSpawnQueueService m_Queue;
	ref HST_ForceSpawnAdapterService m_Adapter;
	ref HST_PhysicalWarService m_PhysicalWar;
	ref HST_EnemyPatrolOperationProofHarness m_Service;
	ref HST_EnemyOrderState m_Order;
	ref HST_ForceManifestState m_Manifest;
	ref HST_GeneratedRouteState m_Route;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
	ref HST_OperationRecordState m_Operation;
	ref HST_EnemyPatrolAdmissionResult m_Admission;
	bool m_bAdmissionPreflightAccepted;
	bool m_bAdmissionPreflightSideEffectFree;
	bool m_bDebitAccepted;
	int m_iAttackBeforeDebit;
	int m_iAttackAfterDebit;
	int m_iAttackAfterAdmission;
	string m_sDebitReason;
	string m_sPreparationFailure;
}

class HST_EnemyPatrolProofFixtureFactory
{
	static const string PROOF_FACTION_KEY = "US";
	static const string PROOF_SOURCE_ZONE_PREFIX = "enemy_patrol_proof_source_";
	static const string PROOF_TARGET_ZONE_PREFIX = "enemy_patrol_proof_target_";
	static const int PROOF_ATTACK_COST = 8;

	HST_EnemyPatrolOperationProofFixture BuildPreparedFixture(string suffix)
	{
		HST_EnemyPatrolOperationProofFixture fixture = new HST_EnemyPatrolOperationProofFixture();
		fixture.m_State = BuildState(suffix);
		fixture.m_Preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		fixture.m_EnemyDirector = new HST_EnemyDirectorService();
		fixture.m_Planning = new HST_ForcePlanningService();
		fixture.m_Queue = new HST_ForceSpawnQueueService();
		fixture.m_Adapter = new HST_ForceSpawnAdapterService();
		fixture.m_PhysicalWar = new HST_PhysicalWarService();
		fixture.m_Service = new HST_EnemyPatrolOperationProofHarness();
		fixture.m_Service.SetRuntimeServices(fixture.m_Queue, fixture.m_Adapter, fixture.m_PhysicalWar);
		fixture.m_Route = fixture.m_State.FindGeneratedRoute(BuildRouteId(suffix));
		fixture.m_Order = BuildOrder(fixture.m_State, suffix);

		HST_EnemyPatrolManifestResult planned = fixture.m_Planning.PlanExactEnemyPatrol(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Order);
		if (!planned || !planned.m_bSuccess || !planned.m_Manifest)
		{
			fixture.m_sPreparationFailure = "exact enemy patrol proof planning failed";
			if (planned && !planned.m_sFailureReason.IsEmpty())
				fixture.m_sPreparationFailure = fixture.m_sPreparationFailure + ": " + planned.m_sFailureReason;
			return fixture;
		}
		fixture.m_Manifest = planned.m_Manifest;

		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		int ordersBefore = fixture.m_State.m_aEnemyOrders.Count();
		int operationsBefore = fixture.m_State.m_aOperations.Count();
		int manifestsBefore = fixture.m_State.m_aForceManifests.Count();
		int batchesBefore = fixture.m_State.m_aForceSpawnResults.Count();
		int groupsBefore = fixture.m_State.m_aActiveGroups.Count();
		int attackBefore;
		if (pool)
			attackBefore = pool.m_iAttackResources;
		HST_EnemyPatrolAdmissionResult preflight = fixture.m_Service.CanAdmitPreparedOrder(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Manifest,
			fixture.m_Route,
			fixture.m_EnemyDirector);
		fixture.m_bAdmissionPreflightAccepted = preflight && preflight.m_bSuccess;
		fixture.m_bAdmissionPreflightSideEffectFree = pool && pool.m_iAttackResources == attackBefore;
		fixture.m_bAdmissionPreflightSideEffectFree = fixture.m_bAdmissionPreflightSideEffectFree
			&& fixture.m_State.m_aEnemyOrders.Count() == ordersBefore
			&& fixture.m_State.m_aOperations.Count() == operationsBefore;
		fixture.m_bAdmissionPreflightSideEffectFree = fixture.m_bAdmissionPreflightSideEffectFree
			&& fixture.m_State.m_aForceManifests.Count() == manifestsBefore
			&& fixture.m_State.m_aForceSpawnResults.Count() == batchesBefore
			&& fixture.m_State.m_aActiveGroups.Count() == groupsBefore;
		if (!fixture.m_bAdmissionPreflightAccepted)
		{
			fixture.m_sPreparationFailure = "exact enemy patrol proof admission preflight failed";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				fixture.m_sPreparationFailure = fixture.m_sPreparationFailure + ": " + preflight.m_sFailureReason;
		}
		return fixture;
	}

	HST_EnemyPatrolOperationProofFixture BuildAdmittedFixture(string suffix)
	{
		HST_EnemyPatrolOperationProofFixture fixture = BuildPreparedFixture(suffix);
		if (!Prepared(fixture))
			return fixture;
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		if (pool)
			fixture.m_iAttackBeforeDebit = pool.m_iAttackResources;
		fixture.m_Order.m_sResourceDebitMutationId
			= "enemy_resource_debit_" + fixture.m_Order.m_sOrderId;
		fixture.m_bDebitAccepted = fixture.m_EnemyDirector.TrySpendProactiveAttack(
			fixture.m_State,
			PROOF_FACTION_KEY,
			fixture.m_Order.m_iAttackCost,
			fixture.m_sDebitReason,
			fixture.m_Order.m_sResourceDebitMutationId,
			fixture.m_Order.m_sOrderId,
			fixture.m_Order.m_sOrderId,
			fixture.m_Order.m_sOperationId,
			fixture.m_Manifest.m_sManifestId,
			fixture.m_Order.m_sTargetZoneId);
		if (pool)
			fixture.m_iAttackAfterDebit = pool.m_iAttackResources;
		if (!fixture.m_bDebitAccepted)
			return fixture;
		fixture.m_State.m_aEnemyOrders.Insert(fixture.m_Order);
		fixture.m_Admission = fixture.m_Service.AdmitPreparedOrder(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Manifest,
			fixture.m_Route,
			fixture.m_EnemyDirector);
		if (pool)
			fixture.m_iAttackAfterAdmission = pool.m_iAttackResources;
		if (!fixture.m_Admission || !fixture.m_Admission.m_bSuccess)
			return fixture;
		fixture.m_Operation = fixture.m_Admission.m_Operation;
		fixture.m_Batch = fixture.m_Admission.m_Batch;
		fixture.m_Group = fixture.m_Admission.m_Group;
		return fixture;
	}

	bool Prepared(HST_EnemyPatrolOperationProofFixture fixture)
	{
		if (!fixture)
			return false;
		bool contextReady = fixture.m_State && fixture.m_Preset && fixture.m_EnemyDirector;
		bool planningReady = fixture.m_Planning && fixture.m_Queue && fixture.m_Adapter;
		bool runtimeReady = fixture.m_PhysicalWar && fixture.m_Service;
		bool authorityReady = fixture.m_Order && fixture.m_Manifest && fixture.m_Route;
		return contextReady && planningReady && runtimeReady
			&& authorityReady && fixture.m_bAdmissionPreflightAccepted;
	}

	bool Ready(HST_EnemyPatrolOperationProofFixture fixture)
	{
		return Prepared(fixture) && fixture.m_bDebitAccepted
			&& fixture.m_Admission && fixture.m_Admission.m_bSuccess
			&& fixture.m_Operation && fixture.m_Batch && fixture.m_Group;
	}

	string BuildFailure(HST_EnemyPatrolOperationProofFixture fixture)
	{
		if (!fixture)
			return "exact enemy patrol proof fixture is unavailable";
		if (!fixture.m_sPreparationFailure.IsEmpty())
			return fixture.m_sPreparationFailure;
		if (!fixture.m_bDebitAccepted)
			return "exact enemy patrol proof debit failed: " + fixture.m_sDebitReason;
		if (fixture.m_Admission && !fixture.m_Admission.m_sFailureReason.IsEmpty())
			return "exact enemy patrol proof admission failed: " + fixture.m_Admission.m_sFailureReason;
		return "exact enemy patrol proof fixture is incomplete";
	}

	protected HST_CampaignState BuildState(string suffix)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iCampaignSeed = 535353;
		state.m_iElapsedSeconds = 100;
		state.m_iWarLevel = 3;
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		AddFactionPool(state);
		AddZones(state, suffix);
		AddRoute(state, suffix);
		return state;
	}

	protected void AddFactionPool(HST_CampaignState state)
	{
		HST_FactionPoolState pool = new HST_FactionPoolState();
		pool.m_sFactionKey = PROOF_FACTION_KEY;
		pool.m_iStrategicContractVersion = HST_EnemyStrategicResourceService.CONTRACT_VERSION;
		pool.m_iStrategicRevision = 1;
		pool.m_iAttackResources = 200;
		pool.m_iSupportResources = 200;
		pool.m_iAggression = 50;
		state.m_aFactionPools.Insert(pool);
	}

	protected void AddZones(HST_CampaignState state, string suffix)
	{
		HST_ZoneState source = new HST_ZoneState();
		source.m_sZoneId = BuildSourceZoneId(suffix);
		source.m_sDisplayName = "Enemy Patrol Proof Source";
		source.m_sOwnerFactionKey = PROOF_FACTION_KEY;
		source.m_eType = HST_EZoneType.HST_ZONE_OUTPOST;
		source.m_vPosition = "1000 20 1000";
		state.m_aZones.Insert(source);

		HST_ZoneState target = new HST_ZoneState();
		target.m_sZoneId = BuildTargetZoneId(suffix);
		target.m_sDisplayName = "Enemy Patrol Proof Target";
		target.m_sOwnerFactionKey = PROOF_FACTION_KEY;
		target.m_eType = HST_EZoneType.HST_ZONE_TOWN;
		target.m_vPosition = "1400 20 1400";
		target.m_sPatrolRouteId = BuildRouteId(suffix);
		state.m_aZones.Insert(target);
	}

	protected void AddRoute(HST_CampaignState state, string suffix)
	{
		HST_GeneratedRouteState route = new HST_GeneratedRouteState();
		route.m_sRouteId = BuildRouteId(suffix);
		route.m_sSourceZoneId = BuildSourceZoneId(suffix);
		route.m_sTargetZoneId = BuildTargetZoneId(suffix);
		route.m_sSourceCategory = "enemy_patrol_proof";
		route.m_vStartPosition = "1400 20 1400";
		route.m_vMidPosition = "1460 20 1400";
		route.m_vEndPosition = "1460 20 1460";
		route.m_iWaypointCount = 3;
		AddWaypoint(route, 0, route.m_vStartPosition);
		AddWaypoint(route, 1, route.m_vMidPosition);
		AddWaypoint(route, 2, route.m_vEndPosition);
		state.m_aGeneratedRoutes.Insert(route);
	}

	protected void AddWaypoint(HST_GeneratedRouteState route, int index, vector position)
	{
		HST_RouteWaypointState waypoint = new HST_RouteWaypointState();
		waypoint.m_sRouteId = route.m_sRouteId;
		waypoint.m_iIndex = index;
		waypoint.m_vPosition = position;
		waypoint.m_iRadiusMeters = 20;
		waypoint.m_sHint = "enemy patrol proof waypoint";
		route.m_aWaypoints.Insert(waypoint);
	}

	protected HST_EnemyOrderState BuildOrder(HST_CampaignState state, string suffix)
	{
		HST_EnemyOrderState order = new HST_EnemyOrderState();
		order.m_sOrderId = "enemy_patrol_proof_" + suffix;
		order.m_sOperationId = HST_StableIdService.BuildOperationId("enemy_order", order.m_sOrderId);
		order.m_iOperationContractVersion = HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION;
		order.m_sFactionKey = PROOF_FACTION_KEY;
		order.m_eType = HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED;
		order.m_sSourceZoneId = BuildSourceZoneId(suffix);
		order.m_sTargetZoneId = BuildTargetZoneId(suffix);
		order.m_vSourcePosition = state.FindZone(order.m_sSourceZoneId).m_vPosition;
		order.m_vTargetPosition = state.FindZone(order.m_sTargetZoneId).m_vPosition;
		order.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		order.m_iAttackCost = PROOF_ATTACK_COST;
		order.m_iSupportCost = 0;
		order.m_sRuntimeStatus = "proof_prepaid_pending";
		return order;
	}

	protected string BuildSourceZoneId(string suffix)
	{
		return PROOF_SOURCE_ZONE_PREFIX + suffix;
	}

	protected string BuildTargetZoneId(string suffix)
	{
		return PROOF_TARGET_ZONE_PREFIX + suffix;
	}

	protected string BuildRouteId(string suffix)
	{
		return "route_" + BuildTargetZoneId(suffix) + "_alpha";
	}
}

class HST_EnemyPatrolOperationProofService
{
	protected ref HST_EnemyPatrolProofFixtureFactory m_Fixtures = new HST_EnemyPatrolProofFixtureFactory();

	HST_EnemyPatrolOperationProofReport Run()
	{
		HST_EnemyPatrolOperationProofReport report = new HST_EnemyPatrolOperationProofReport();
		ProveAdmission(report);
		ProveReplayAndRefund(report);
		ProveRouteLoop(report);
		ProveProjectionAndRoster(report);
		ProveContactHold(report);
		ProveReturnSettlement(report);
		ProveRestore(report);
		ProveCorruption(report);
		ProveDispatchIsolation(report);
		ProveMarkers(report);
		return report;
	}

	protected void ProveAdmission(HST_EnemyPatrolOperationProofReport report)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("admission");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sAdmissionEvidence = m_Fixtures.BuildFailure(fixture);
			return;
		}
		bool manifestExact = IsFrozenInfantryManifestExact(fixture);
		bool rowsExact = CountOperationId(fixture.m_State, fixture.m_Operation.m_sOperationId) == 1;
		rowsExact = rowsExact && CountManifestId(fixture.m_State, fixture.m_Manifest.m_sManifestId) == 1;
		rowsExact = rowsExact && CountBatchId(fixture.m_State, fixture.m_Batch.m_sResultId) == 1;
		rowsExact = rowsExact && CountGroupId(fixture.m_State, fixture.m_Group.m_sGroupId) == 1;
		bool linksExact = AdmissionLinksExact(fixture);
		bool routeExact = AdmissionRouteExact(fixture);
		bool debitExact = fixture.m_bAdmissionPreflightSideEffectFree && fixture.m_bDebitAccepted;
		debitExact = debitExact && fixture.m_iAttackAfterDebit == fixture.m_iAttackBeforeDebit - fixture.m_Order.m_iAttackCost;
		debitExact = debitExact && fixture.m_iAttackAfterAdmission == fixture.m_iAttackAfterDebit;
		report.m_bAdmissionExact = manifestExact && rowsExact && linksExact;
		report.m_bAdmissionExact = report.m_bAdmissionExact && routeExact && debitExact;
		report.m_sAdmissionEvidence = string.Format(
			"preflight pure/debit %1/%2 | attack %3 -> %4 -> %5 | members %6",
			fixture.m_bAdmissionPreflightSideEffectFree,
			fixture.m_bDebitAccepted,
			fixture.m_iAttackBeforeDebit,
			fixture.m_iAttackAfterDebit,
			fixture.m_iAttackAfterAdmission,
			fixture.m_Manifest.m_iAcceptedMemberCount);
		report.m_sAdmissionEvidence = report.m_sAdmissionEvidence + string.Format(
			" | rows/links/route %1/%2/%3 | held %4",
			rowsExact,
			linksExact,
			routeExact,
			fixture.m_Batch.m_bStrategicProjectionHeld);
	}

	protected void ProveReplayAndRefund(HST_EnemyPatrolOperationProofReport report)
	{
		string replayEvidence;
		string corruptReplayEvidence;
		string refundEvidence;
		string collisionEvidence;
		bool replayExact = ProveCommittedReplay(replayEvidence);
		bool corruptReplayRejected = ProveCorruptCommittedReplayRejection(corruptReplayEvidence);
		bool refundExact = ProveAdmissionFailureRefund(refundEvidence);
		bool collisionExact = ProveAdmissionCollisionSafety(collisionEvidence);
		report.m_bReplayRefundExact = replayExact && corruptReplayRejected && refundExact && collisionExact;
		report.m_sReplayRefundEvidence = "replay " + replayEvidence;
		report.m_sReplayRefundEvidence = report.m_sReplayRefundEvidence + " | corrupt replay " + corruptReplayEvidence;
		report.m_sReplayRefundEvidence = report.m_sReplayRefundEvidence + " | refund " + refundEvidence;
		report.m_sReplayRefundEvidence = report.m_sReplayRefundEvidence + " | collision " + collisionEvidence;
	}

	protected bool ProveCommittedReplay(out string evidence)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("replay");
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = m_Fixtures.BuildFailure(fixture);
			return false;
		}
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(HST_EnemyPatrolProofFixtureFactory.PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		int operationsBefore = fixture.m_State.m_aOperations.Count();
		int manifestsBefore = fixture.m_State.m_aForceManifests.Count();
		int batchesBefore = fixture.m_State.m_aForceSpawnResults.Count();
		int groupsBefore = fixture.m_State.m_aActiveGroups.Count();
		fixture.m_State.m_iElapsedSeconds += 73;
		fixture.m_State.m_iWarLevel = 5;
		HST_EnemyPatrolManifestResult replayPlan = fixture.m_Planning.PlanExactEnemyPatrol(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Order);
		HST_EnemyPatrolAdmissionResult replay;
		if (replayPlan && replayPlan.m_bSuccess && replayPlan.m_Manifest)
		{
			replay = fixture.m_Service.AdmitPreparedOrder(
				fixture.m_State,
				fixture.m_Order,
				replayPlan.m_Manifest,
				fixture.m_Route,
				fixture.m_EnemyDirector);
		}
		bool manifestExact = replayPlan && replayPlan.m_bSuccess && replayPlan.m_Manifest;
		manifestExact = manifestExact && replayPlan.m_Manifest.m_sManifestHash == fixture.m_Manifest.m_sManifestHash;
		bool noMutation = pool.m_iAttackResources == attackBefore;
		noMutation = noMutation && fixture.m_State.m_aOperations.Count() == operationsBefore;
		noMutation = noMutation && fixture.m_State.m_aForceManifests.Count() == manifestsBefore;
		noMutation = noMutation && fixture.m_State.m_aForceSpawnResults.Count() == batchesBefore;
		noMutation = noMutation && fixture.m_State.m_aActiveGroups.Count() == groupsBefore;
		bool accepted = replay && replay.m_bSuccess && !replay.m_bStateChanged;
		evidence = string.Format("accepted/no-mutation/manifest %1/%2/%3", accepted, noMutation, manifestExact);
		return accepted && noMutation && manifestExact;
	}

	protected bool ProveCorruptCommittedReplayRejection(out string evidence)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("replay_corrupt_route");
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = m_Fixtures.BuildFailure(fixture);
			return false;
		}
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(HST_EnemyPatrolProofFixtureFactory.PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		int operationsBefore = fixture.m_State.m_aOperations.Count();
		int manifestsBefore = fixture.m_State.m_aForceManifests.Count();
		int batchesBefore = fixture.m_State.m_aForceSpawnResults.Count();
		int groupsBefore = fixture.m_State.m_aActiveGroups.Count();
		fixture.m_Operation.m_sRouteContractHash = "enemy_patrol_proof_corrupt_committed_route";
		HST_EnemyPatrolManifestResult replayPlan = fixture.m_Planning.PlanExactEnemyPatrol(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Order);
		HST_EnemyPatrolAdmissionResult replay;
		if (replayPlan && replayPlan.m_bSuccess && replayPlan.m_Manifest)
		{
			replay = fixture.m_Service.AdmitPreparedOrder(
				fixture.m_State,
				fixture.m_Order,
				replayPlan.m_Manifest,
				fixture.m_Route,
				fixture.m_EnemyDirector);
		}
		bool rejected = replayPlan && replayPlan.m_bSuccess && replay && !replay.m_bSuccess;
		bool noMutation = pool.m_iAttackResources == attackBefore;
		noMutation = noMutation && fixture.m_State.m_aOperations.Count() == operationsBefore;
		noMutation = noMutation && fixture.m_State.m_aForceManifests.Count() == manifestsBefore;
		noMutation = noMutation && fixture.m_State.m_aForceSpawnResults.Count() == batchesBefore;
		noMutation = noMutation && fixture.m_State.m_aActiveGroups.Count() == groupsBefore;
		noMutation = noMutation && !fixture.m_Order.m_bResourceSettlementApplied;
		evidence = string.Format("rejected/no-mutation %1/%2", rejected, noMutation);
		return rejected && noMutation;
	}

	protected bool ProveAdmissionFailureRefund(out string evidence)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildPreparedFixture("refund");
		if (!m_Fixtures.Prepared(fixture))
		{
			evidence = m_Fixtures.BuildFailure(fixture);
			return false;
		}
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(HST_EnemyPatrolProofFixtureFactory.PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		string debitReason;
		fixture.m_Order.m_sResourceDebitMutationId
			= "enemy_resource_debit_" + fixture.m_Order.m_sOrderId;
		bool spent = fixture.m_EnemyDirector.TrySpendProactiveAttack(
			fixture.m_State,
			fixture.m_Order.m_sFactionKey,
			fixture.m_Order.m_iAttackCost,
			debitReason,
			fixture.m_Order.m_sResourceDebitMutationId,
			fixture.m_Order.m_sOrderId,
			fixture.m_Order.m_sOrderId,
			fixture.m_Order.m_sOperationId,
			fixture.m_Manifest.m_sManifestId,
			fixture.m_Order.m_sTargetZoneId);
		fixture.m_State.m_aEnemyOrders.Insert(fixture.m_Order);
		fixture.m_Route.m_sTargetZoneId = "enemy_patrol_proof_invalid_route_target";
		HST_EnemyPatrolAdmissionResult failed = fixture.m_Service.AdmitPreparedOrder(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Manifest,
			fixture.m_Route,
			fixture.m_EnemyDirector);
		int attackAfterFailure = pool.m_iAttackResources;
		HST_EnemyPatrolAdmissionResult replay = fixture.m_Service.AdmitPreparedOrder(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Manifest,
			fixture.m_Route,
			fixture.m_EnemyDirector);
		int attackAfterReplay = pool.m_iAttackResources;
		bool receiptExact = fixture.m_Order.m_bResourceSettlementApplied;
		receiptExact = receiptExact && fixture.m_Order.m_sResourceSettlementKind == "admission_failed_full";
		receiptExact = receiptExact && fixture.m_Order.m_iRefundedAttackResources == fixture.m_Order.m_iAttackCost;
		bool terminalExact = fixture.m_Order.m_iOperationContractVersion == HST_EnemyPatrolOperationService.QUARANTINED_CONTRACT_VERSION;
		terminalExact = terminalExact && fixture.m_Order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		bool rowsExact = fixture.m_State.m_aOperations.Count() == 0 && fixture.m_State.m_aForceManifests.Count() == 0;
		rowsExact = rowsExact && fixture.m_State.m_aForceSpawnResults.Count() == 0 && fixture.m_State.m_aActiveGroups.Count() == 0;
		bool refundExact = spent && failed && !failed.m_bSuccess && receiptExact;
		refundExact = refundExact && attackAfterFailure == attackBefore && attackAfterReplay == attackAfterFailure;
		evidence = string.Format(
			"spent/rejected/receipt %1/%2/%3 | attack %4/%5/%6",
			spent,
			failed && !failed.m_bSuccess,
			receiptExact,
			attackBefore,
			attackAfterFailure,
			attackAfterReplay);
		evidence = evidence + string.Format(" | terminal/rows/replay-rejected %1/%2/%3", terminalExact, rowsExact, replay && !replay.m_bSuccess);
		return refundExact && terminalExact && rowsExact && replay && !replay.m_bSuccess;
	}

	protected bool ProveAdmissionCollisionSafety(out string evidence)
	{
		string preflightEvidence;
		string postDebitEvidence;
		bool preflightExact = ProvePreflightClaimantCollision(preflightEvidence);
		bool postDebitExact = ProvePostDebitClaimantCollision(postDebitEvidence);
		evidence = "preflight " + preflightEvidence + " | post-debit " + postDebitEvidence;
		return preflightExact && postDebitExact;
	}

	protected bool ProvePreflightClaimantCollision(out string evidence)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildPreparedFixture("collision_preflight");
		if (!m_Fixtures.Prepared(fixture))
		{
			evidence = m_Fixtures.BuildFailure(fixture);
			return false;
		}
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(HST_EnemyPatrolProofFixtureFactory.PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		HST_ForceSpawnResultState foreignBatch = BuildForeignCollisionBatch(fixture.m_Order, "preflight");
		fixture.m_State.m_aForceSpawnResults.Insert(foreignBatch);
		HST_EnemyPatrolAdmissionResult rejected = fixture.m_Service.CanAdmitPreparedOrder(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Manifest,
			fixture.m_Route,
			fixture.m_EnemyDirector);
		bool foreignPreserved = ForeignCollisionBatchExact(fixture.m_State, foreignBatch, "preflight");
		bool pure = pool.m_iAttackResources == attackBefore && fixture.m_State.m_aEnemyOrders.Count() == 0;
		pure = pure && fixture.m_State.m_aOperations.Count() == 0 && fixture.m_State.m_aForceManifests.Count() == 0;
		evidence = string.Format("rejected/preserved/pure %1/%2/%3", rejected && !rejected.m_bSuccess, foreignPreserved, pure);
		return rejected && !rejected.m_bSuccess && foreignPreserved && pure;
	}

	protected bool ProvePostDebitClaimantCollision(out string evidence)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildPreparedFixture("collision_post_debit");
		if (!m_Fixtures.Prepared(fixture))
		{
			evidence = m_Fixtures.BuildFailure(fixture);
			return false;
		}
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(HST_EnemyPatrolProofFixtureFactory.PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		string spendReason;
		fixture.m_Order.m_sResourceDebitMutationId
			= "enemy_resource_debit_" + fixture.m_Order.m_sOrderId;
		bool spent = fixture.m_EnemyDirector.TrySpendProactiveAttack(
			fixture.m_State,
			fixture.m_Order.m_sFactionKey,
			fixture.m_Order.m_iAttackCost,
			spendReason,
			fixture.m_Order.m_sResourceDebitMutationId,
			fixture.m_Order.m_sOrderId,
			fixture.m_Order.m_sOrderId,
			fixture.m_Order.m_sOperationId,
			fixture.m_Manifest.m_sManifestId,
			fixture.m_Order.m_sTargetZoneId);
		fixture.m_State.m_aEnemyOrders.Insert(fixture.m_Order);
		HST_ForceSpawnResultState foreignBatch = BuildForeignCollisionBatch(fixture.m_Order, "post_debit");
		fixture.m_State.m_aForceSpawnResults.Insert(foreignBatch);
		HST_EnemyPatrolAdmissionResult rejected = fixture.m_Service.AdmitPreparedOrder(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Manifest,
			fixture.m_Route,
			fixture.m_EnemyDirector);
		int attackAfterFailure = pool.m_iAttackResources;
		HST_EnemyPatrolAdmissionResult replay = fixture.m_Service.AdmitPreparedOrder(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Manifest,
			fixture.m_Route,
			fixture.m_EnemyDirector);
		int attackAfterReplay = pool.m_iAttackResources;
		bool foreignPreserved = ForeignCollisionBatchExact(fixture.m_State, foreignBatch, "post_debit");
		bool refundedOnce = spent && attackAfterFailure == attackBefore && attackAfterReplay == attackAfterFailure;
		refundedOnce = refundedOnce && fixture.m_Order.m_bResourceSettlementApplied;
		refundedOnce = refundedOnce && fixture.m_Order.m_iRefundedAttackResources == fixture.m_Order.m_iAttackCost;
		bool rejectedExact = rejected && !rejected.m_bSuccess && replay && !replay.m_bSuccess;
		evidence = string.Format(
			"spent/rejected/preserved %1/%2/%3 | attack %4/%5/%6 | once %7",
			spent,
			rejectedExact,
			foreignPreserved,
			attackBefore,
			attackAfterFailure,
			attackAfterReplay,
			refundedOnce);
		return rejectedExact && foreignPreserved && refundedOnce;
	}

	protected void ProveRouteLoop(HST_EnemyPatrolOperationProofReport report)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("route_loop");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sRouteLoopEvidence = m_Fixtures.BuildFailure(fixture);
			return;
		}
		bool onStation = AdvanceUntilDuty(fixture, HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION, 24);
		bool loopStarted = onStation && fixture.m_Operation.m_iRouteWaypointIndex == 1;
		loopStarted = loopStarted && fixture.m_Operation.m_iRouteLoopStartedAtSecond > 0;
		bool lapReached = AdvanceUntilLap(fixture, 1, 16);
		ref array<vector> positions = HST_OperationRouteCursorService.BuildOrderedRoutePositions(fixture.m_Route);
		bool closedLoop = lapReached && fixture.m_Operation.m_iRouteWaypointIndex == 0;
		closedLoop = closedLoop && positions.Count() >= 3 && Distance2D(fixture.m_Operation.m_vRouteEndPosition, positions[0]) < 0.1;
		bool returning = AdvanceUntilDuty(fixture, HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN, 16);
		bool returnExact = returning && fixture.m_Operation.m_iRouteWaypointIndex == -1;
		returnExact = returnExact && Distance2D(fixture.m_Operation.m_vRouteEndPosition, fixture.m_Operation.m_vOriginPosition) < 0.1;
		bool stillOpen = fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		stillOpen = stillOpen && !fixture.m_Order.m_bResourceSettlementApplied;
		report.m_bRouteLoopExact = loopStarted && closedLoop && returnExact && stillOpen;
		report.m_sRouteLoopEvidence = string.Format(
			"loop started/lap/closed/return %1/%2/%3/%4",
			loopStarted,
			fixture.m_Operation.m_iRouteLapCount,
			closedLoop,
			returnExact);
		report.m_sRouteLoopEvidence = report.m_sRouteLoopEvidence + string.Format(
			" | waypoint/leg/open %1/%2/%3",
			fixture.m_Operation.m_iRouteWaypointIndex,
			fixture.m_Operation.m_iRouteLegSequence,
			stillOpen);
	}

	protected void ProveProjectionAndRoster(HST_EnemyPatrolOperationProofReport report)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("projection");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sProjectionRosterEvidence = m_Fixtures.BuildFailure(fixture);
			return;
		}
		fixture.m_State.m_iElapsedSeconds += 30;
		fixture.m_Service.TickOrder(fixture.m_State, fixture.m_Preset, fixture.m_EnemyDirector, fixture.m_Order);
		int waypointBefore = fixture.m_Operation.m_iRouteWaypointIndex;
		vector endpointBefore = fixture.m_Operation.m_vRouteEndPosition;
		HST_ForceSpawnQueueCallbackResult released = fixture.m_Queue.ReleaseStrategicProjectionForMaterialization(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			fixture.m_State.m_iElapsedSeconds,
			fixture.m_State.m_iElapsedSeconds + 180);
		PrepareSuccessfulQueueCallbackFixture(fixture, "projection");
		HST_ForceSpawnSlotResultState casualtySlot = FindFirstMappedMemberSlot(fixture.m_Batch);
		string mappedEntityId;
		if (casualtySlot)
			mappedEntityId = casualtySlot.m_sEntityId;
		HST_ForceSpawnQueueCallbackResult casualty;
		if (casualtySlot)
		{
			casualty = fixture.m_Queue.ConfirmRegisteredMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				casualtySlot.m_sSlotId,
				mappedEntityId,
				fixture.m_State.m_iElapsedSeconds + 1,
				"exact enemy patrol proof mapped casualty");
		}
		int durableLiving = fixture.m_Queue.CountDurableLivingMemberSlots(fixture.m_Batch);
		vector foldedPosition = fixture.m_Group.m_vPosition + "8 0 8";
		HST_ForceSpawnQueueCallbackResult foldPreflight = fixture.m_Queue.CanRequeueSuccessfulProjectionForStrategicHold(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			fixture.m_State.m_iElapsedSeconds + 2,
			fixture.m_State.m_iElapsedSeconds + 182);
		HST_ForceSpawnQueueCallbackResult held = fixture.m_Queue.RequeueSuccessfulProjectionForStrategicHold(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			fixture.m_State.m_iElapsedSeconds + 2,
			fixture.m_State.m_iElapsedSeconds + 182);
		HST_OperationRouteCursorService cursor = new HST_OperationRouteCursorService();
		bool cursorFolded = cursor.SyncLegFromPositionAtSecond(
			fixture.m_State.m_iElapsedSeconds + 2,
			fixture.m_Operation,
			fixture.m_Group,
			foldedPosition);
		bool tombstoneExact = casualty && casualty.m_bAccepted && casualtySlot;
		tombstoneExact = tombstoneExact && casualtySlot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
		tombstoneExact = tombstoneExact && casualtySlot.m_bEverAlive && casualtySlot.m_bCasualtyConfirmed;
		tombstoneExact = tombstoneExact && casualtySlot.m_sEntityId.IsEmpty();
		bool rosterExact = durableLiving == fixture.m_Manifest.m_iAcceptedMemberCount - 1;
		rosterExact = rosterExact && fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch) == durableLiving;
		bool routeExact = cursorFolded && fixture.m_Operation.m_iRouteWaypointIndex == waypointBefore;
		routeExact = routeExact && fixture.m_Operation.m_vRouteEndPosition == endpointBefore;
		routeExact = routeExact && Distance2D(fixture.m_Operation.m_vStrategicPosition, foldedPosition) < 0.1;
		bool projectionExact = released && released.m_bAccepted && foldPreflight && foldPreflight.m_bAccepted;
		projectionExact = projectionExact && held && held.m_bAccepted && fixture.m_Batch.m_bStrategicProjectionHeld;
		projectionExact = projectionExact && fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		bool noSettlement = !fixture.m_Order.m_bResourceSettlementApplied;
		report.m_bProjectionRosterExact = tombstoneExact && rosterExact && routeExact;
		report.m_bProjectionRosterExact = report.m_bProjectionRosterExact && projectionExact && noSettlement;
		report.m_sProjectionRosterEvidence = string.Format(
			"queue release/casualty/requeue %1/%2/%3 | living %4/%5",
			released && released.m_bAccepted,
			casualty && casualty.m_bAccepted,
			held && held.m_bAccepted,
			durableLiving,
			fixture.m_Manifest.m_iAcceptedMemberCount);
		report.m_sProjectionRosterEvidence = report.m_sProjectionRosterEvidence + string.Format(
			" | tombstone/route/no-settlement %1/%2/%3",
			tombstoneExact,
			routeExact,
			noSettlement);
	}

	protected void ProveContactHold(HST_EnemyPatrolOperationProofReport report)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("contact_hold");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sContactHoldEvidence = m_Fixtures.BuildFailure(fixture);
			return;
		}
		fixture.m_State.m_iElapsedSeconds += 30;
		fixture.m_Service.TickOrder(fixture.m_State, fixture.m_Preset, fixture.m_EnemyDirector, fixture.m_Order);
		float progressBefore = fixture.m_Operation.m_fRouteProgressMeters;
		vector positionBefore = fixture.m_Operation.m_vStrategicPosition;
		int waypointBefore = fixture.m_Operation.m_iRouteWaypointIndex;
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		fixture.m_Operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		fixture.m_Group.m_iLastCasualtySecond = fixture.m_State.m_iElapsedSeconds;
		bool contactActive;
		bool contactCleared;
		bool entered = fixture.m_Service.UpdateContactForProof(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Group,
			contactActive,
			contactCleared);
		bool enteredExact = entered && contactActive && !contactCleared;
		enteredExact = enteredExact && fixture.m_Operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT;
		enteredExact = enteredExact && fixture.m_Group.m_sRuntimeStatus == "enemy_patrol_contact_hold";
		fixture.m_Group.m_iLastCasualtySecond = 0;
		fixture.m_State.m_iElapsedSeconds += HST_EnemyPatrolOperationService.CONTACT_CLEAR_SECONDS - 1;
		bool retainedActive;
		bool retainedCleared;
		fixture.m_Service.UpdateContactForProof(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Group,
			retainedActive,
			retainedCleared);
		fixture.m_State.m_iElapsedSeconds++;
		bool clearedActive;
		bool clearedFlag;
		bool cleared = fixture.m_Service.UpdateContactForProof(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Group,
			clearedActive,
			clearedFlag);
		bool heldStill = fixture.m_Operation.m_fRouteProgressMeters == progressBefore;
		heldStill = heldStill && fixture.m_Operation.m_vStrategicPosition == positionBefore;
		heldStill = heldStill && fixture.m_Operation.m_iRouteWaypointIndex == waypointBefore;
		bool transitionExact = retainedActive && !retainedCleared;
		transitionExact = transitionExact && cleared && !clearedActive && clearedFlag;
		transitionExact = transitionExact && fixture.m_Operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		fixture.m_Operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		fixture.m_Operation.m_iStrategicLastUpdateSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_State.m_iElapsedSeconds++;
		HST_OperationRouteCursorService cursor = new HST_OperationRouteCursorService();
		HST_OperationRouteCursorResult resumed = cursor.AdvanceVirtualLeg(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Group);
		float resumedMeters = fixture.m_Operation.m_fRouteProgressMeters - progressBefore;
		bool boundedResume = resumed && resumed.m_bAccepted && resumedMeters > 0;
		boundedResume = boundedResume && resumedMeters <= HST_OperationRouteCursorService.DEFAULT_INFANTRY_SPEED_METERS_PER_SECOND + 0.1;
		bool noSettlement = !fixture.m_Order.m_bResourceSettlementApplied;
		report.m_bContactHoldExact = enteredExact && heldStill && transitionExact && boundedResume && noSettlement;
		report.m_sContactHoldEvidence = string.Format(
			"casualty-contact/held/cleared/bounded-resume %1/%2/%3/%4 | resumed %5m",
			enteredExact,
			heldStill,
			transitionExact,
			boundedResume,
			resumedMeters);
		report.m_sContactHoldEvidence = report.m_sContactHoldEvidence + string.Format(
			" | waypoint/no-settlement %1/%2",
			fixture.m_Operation.m_iRouteWaypointIndex,
			noSettlement);
	}

	protected void ProveReturnSettlement(HST_EnemyPatrolOperationProofReport report)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("settlement");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sSettlementEvidence = m_Fixtures.BuildFailure(fixture);
			return;
		}
		bool casualtyAccepted = ConfirmStrategicCasualty(fixture, "settlement");
		int survivors = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(HST_EnemyPatrolProofFixtureFactory.PROOF_FACTION_KEY);
		int attackBeforeSettlement = pool.m_iAttackResources;
		bool settled = DriveUntilSettled(fixture, 80);
		int attackAfterSettlement = pool.m_iAttackResources;
		string settlementId = fixture.m_Order.m_sResourceSettlementId;
		fixture.m_Service.ReconcileSettledRuntimeCleanup(fixture.m_State);
		int attackAfterReplay = pool.m_iAttackResources;
		int expectedRefund = fixture.m_Order.m_iAttackCost * survivors / fixture.m_Manifest.m_iAcceptedMemberCount;
		bool refundExact = attackAfterSettlement - attackBeforeSettlement == expectedRefund;
		refundExact = refundExact && attackAfterReplay == attackAfterSettlement;
		bool terminalExact = settled && fixture.m_Operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED;
		terminalExact = terminalExact && fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		terminalExact = terminalExact && fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
		terminalExact = terminalExact && fixture.m_Order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED;
		bool receiptExact = fixture.m_Order.m_bResourceSettlementApplied && !settlementId.IsEmpty();
		receiptExact = receiptExact && fixture.m_Order.m_sResourceSettlementKind == "returned_survivors";
		receiptExact = receiptExact && fixture.m_Order.m_iSettlementSurvivorMemberCount == survivors;
		bool cleanupExact = fixture.m_State.FindForceSpawnResult(fixture.m_Batch.m_sResultId) == null;
		cleanupExact = cleanupExact && fixture.m_State.FindActiveGroup(fixture.m_Group.m_sGroupId) == null;
		report.m_bSettlementExact = casualtyAccepted && refundExact && terminalExact;
		report.m_bSettlementExact = report.m_bSettlementExact && receiptExact && cleanupExact;
		report.m_sSettlementEvidence = string.Format(
			"casualty/survivors/refund %1/%2/%3 | attack %4 -> %5",
			casualtyAccepted,
			survivors,
			expectedRefund,
			attackBeforeSettlement,
			attackAfterSettlement);
		report.m_sSettlementEvidence = report.m_sSettlementEvidence + string.Format(
			" | terminal/receipt/cleanup/replay %1/%2/%3/%4",
			terminalExact,
			receiptExact,
			cleanupExact,
			attackAfterReplay == attackAfterSettlement);
	}

	protected void ProveRestore(HST_EnemyPatrolOperationProofReport report)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("restore");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sRestoreEvidence = m_Fixtures.BuildFailure(fixture);
			return;
		}
		bool onStation = AdvanceUntilDuty(fixture, HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION, 24);
		fixture.m_State.m_iElapsedSeconds += 10;
		fixture.m_Service.TickOrder(fixture.m_State, fixture.m_Preset, fixture.m_EnemyDirector, fixture.m_Order);
		HST_ForceSpawnQueueCallbackResult released = fixture.m_Queue.ReleaseStrategicProjectionForMaterialization(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			fixture.m_State.m_iElapsedSeconds,
			fixture.m_State.m_iElapsedSeconds + 180);
		PrepareSuccessfulQueueCallbackFixture(fixture, "restore");
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		fixture.m_Operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		fixture.m_Operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		fixture.m_Group.m_bSpawnedEntity = true;
		fixture.m_Group.m_bSpawnAttempted = true;
		fixture.m_Group.m_sRuntimeEntityId = "enemy_patrol_proof_runtime_group_restore";
		fixture.m_Group.m_iSpawnedAgentCount = fixture.m_Manifest.m_iAcceptedMemberCount;
		fixture.m_Group.m_vPosition = fixture.m_Operation.m_vStrategicPosition;
		fixture.m_Order.m_bPhysicalized = true;
		fixture.m_Order.m_sRuntimeStatus = "exact_patrol_physical";
		HST_ForceSpawnSlotResultState casualtySlot = FindFirstMappedMemberSlot(fixture.m_Batch);
		string casualtyEntityId;
		if (casualtySlot)
			casualtyEntityId = casualtySlot.m_sEntityId;
		HST_ForceSpawnQueueCallbackResult casualty;
		if (casualtySlot)
		{
			casualty = fixture.m_Queue.ConfirmRegisteredMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				casualtySlot.m_sSlotId,
				casualtyEntityId,
				fixture.m_State.m_iElapsedSeconds + 1,
				"exact enemy patrol proof restore casualty");
		}
		bool casualtyAccepted = casualty && casualty.m_bAccepted;
		vector livePositionBefore = fixture.m_Group.m_vPosition;
		vector endpointBefore = fixture.m_Operation.m_vRouteEndPosition;
		int waypointBefore = fixture.m_Operation.m_iRouteWaypointIndex;
		int lapBefore = fixture.m_Operation.m_iRouteLapCount;
		int livingBefore = fixture.m_Queue.CountDurableLivingMemberSlots(fixture.m_Batch);
		fixture.m_Operation.m_iLastVirtualFriendlyCount = livingBefore;
		ApplyGroupRoster(fixture.m_Group, livingBefore);
		fixture.m_State.m_bRestoredFromPersistence = true;
		fixture.m_State.m_iPersistenceRestoreSequence = 53;
		fixture.m_State.m_iForceSpawnQueueReconciledRestoreSequence = 52;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_ForceSpawnQueueService restoredQueue = new HST_ForceSpawnQueueService();
		HST_EnemyPatrolOperationService restoredService = new HST_EnemyPatrolOperationService();
		if (restored)
		{
			restoredQueue.ReconcileCampaignAfterRestore(restored);
			restoredService.SetRuntimeServices(
				restoredQueue,
				new HST_ForceSpawnAdapterService(),
				new HST_PhysicalWarService());
			restoredService.ReconcileAfterRestore(restored, new HST_EnemyDirectorService());
		}
		HST_EnemyOrderState restoredOrder;
		HST_OperationRecordState restoredOperation;
		HST_ForceManifestState restoredManifest;
		HST_ForceSpawnResultState restoredBatch;
		HST_ActiveGroupState restoredGroup;
		if (restored)
		{
			restoredOrder = FindOrder(restored, fixture.m_Order.m_sOrderId);
			restoredOperation = restored.FindOperation(fixture.m_Operation.m_sOperationId);
			restoredManifest = restored.FindForceManifest(fixture.m_Manifest.m_sManifestId);
			restoredBatch = restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
			restoredGroup = restored.FindActiveGroup(fixture.m_Group.m_sGroupId);
		}
		bool recordsExact = restoredOrder && restoredOperation && restoredManifest && restoredBatch && restoredGroup;
		bool routeExact = recordsExact && restoredOperation.m_iRouteWaypointIndex == waypointBefore;
		routeExact = routeExact && restoredOperation.m_iRouteLapCount == lapBefore;
		routeExact = routeExact && Distance2D(restoredOperation.m_vRouteStartPosition, livePositionBefore) < 0.1;
		routeExact = routeExact && Distance2D(restoredOperation.m_vRouteEndPosition, endpointBefore) < 0.1;
		routeExact = routeExact && Distance2D(restoredOperation.m_vStrategicPosition, livePositionBefore) < 0.1;
		routeExact = routeExact && Math.AbsFloat(restoredOperation.m_fRouteProgressMeters) < 0.1;
		int livingAfter;
		if (restoredBatch)
			livingAfter = restoredQueue.CountStrategicLivingMemberSlots(restoredBatch);
		bool rosterExact = livingAfter == livingBefore && restoredBatch;
		rosterExact = rosterExact && restoredQueue.CountConfirmedCasualtyMemberSlots(restoredBatch) == 1;
		bool authorityExact = recordsExact && restoredBatch.m_bStrategicProjectionHeld;
		authorityExact = authorityExact && restoredBatch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING;
		authorityExact = authorityExact && restoredOperation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		authorityExact = authorityExact && restoredOperation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		bool processIdsCleared = recordsExact && RuntimeIdsCleared(restoredBatch, restoredGroup);
		int attackBeforeRepeat;
		HST_FactionPoolState restoredPool;
		if (restored)
			restoredPool = restored.FindFactionPool(HST_EnemyPatrolProofFixtureFactory.PROOF_FACTION_KEY);
		if (restoredPool)
			attackBeforeRepeat = restoredPool.m_iAttackResources;
		float progressBeforeRepeat;
		int waypointBeforeRepeat;
		int lapBeforeRepeat;
		if (restoredOperation)
		{
			progressBeforeRepeat = restoredOperation.m_fRouteProgressMeters;
			waypointBeforeRepeat = restoredOperation.m_iRouteWaypointIndex;
			lapBeforeRepeat = restoredOperation.m_iRouteLapCount;
		}
		HST_ForceSpawnQueueMaintenanceResult repeatedQueue;
		if (restored)
		{
			repeatedQueue = restoredQueue.ReconcileCampaignAfterRestore(restored);
			restoredService.ReconcileAfterRestore(restored, new HST_EnemyDirectorService());
		}
		bool idempotent = recordsExact && repeatedQueue && !repeatedQueue.m_bStateChanged;
		idempotent = idempotent && RuntimeIdsCleared(restoredBatch, restoredGroup);
		idempotent = idempotent && restoredQueue.CountStrategicLivingMemberSlots(restoredBatch) == livingAfter;
		idempotent = idempotent && restoredQueue.CountConfirmedCasualtyMemberSlots(restoredBatch) == 1;
		idempotent = idempotent && Math.AbsFloat(restoredOperation.m_fRouteProgressMeters - progressBeforeRepeat) < 0.1;
		idempotent = idempotent && restoredOperation.m_iRouteWaypointIndex == waypointBeforeRepeat;
		idempotent = idempotent && restoredOperation.m_iRouteLapCount == lapBeforeRepeat;
		idempotent = idempotent && restoredPool && restoredPool.m_iAttackResources == attackBeforeRepeat;
		idempotent = idempotent && !restoredOrder.m_bResourceSettlementApplied;
		bool continued = false;
		if (recordsExact)
		{
			float restoredProgress = restoredOperation.m_fRouteProgressMeters;
			restored.m_iElapsedSeconds++;
			restoredService.TickOrder(restored, fixture.m_Preset, new HST_EnemyDirectorService(), restoredOrder);
			continued = restoredOperation.m_fRouteProgressMeters > restoredProgress;
		}
		bool schemaExact = restored && restored.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION;
		schemaExact = schemaExact && restored.m_iLastLoadedSchemaVersion == HST_CampaignState.SCHEMA_VERSION;
		report.m_bRestoreExact = onStation && released && released.m_bAccepted && casualtyAccepted;
		report.m_bRestoreExact = report.m_bRestoreExact && schemaExact && recordsExact && routeExact && rosterExact;
		report.m_bRestoreExact = report.m_bRestoreExact && authorityExact && processIdsCleared && idempotent && continued;
		report.m_sRestoreEvidence = string.Format(
			"physical fixture release/casualty %1/%2 | schema/records/route %3/%4/%5",
			released && released.m_bAccepted,
			casualtyAccepted,
			schemaExact,
			recordsExact,
			routeExact);
		report.m_sRestoreEvidence = report.m_sRestoreEvidence + string.Format(
			" | living %1/%2 | authority/ids/idempotent/continued %3/%4/%5/%6",
			livingBefore,
			livingAfter,
			authorityExact,
			processIdsCleared,
			idempotent,
			continued);
	}

	protected void ProveCorruption(HST_EnemyPatrolOperationProofReport report)
	{
		string routeEvidence;
		string backlinkEvidence;
		string receiptEvidence;
		bool routeQuarantined = ProveRouteHashQuarantine(routeEvidence);
		bool backlinkQuarantined = ProveBacklinkQuarantine(backlinkEvidence);
		bool receiptQuarantined = ProvePartialReceiptQuarantine(receiptEvidence);
		report.m_bCorruptionExact = routeQuarantined && backlinkQuarantined && receiptQuarantined;
		report.m_sCorruptionEvidence = "route " + routeEvidence;
		report.m_sCorruptionEvidence = report.m_sCorruptionEvidence + " | backlink " + backlinkEvidence;
		report.m_sCorruptionEvidence = report.m_sCorruptionEvidence + " | receipt " + receiptEvidence;
	}

	protected bool ProveRouteHashQuarantine(out string evidence)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("corrupt_route");
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = m_Fixtures.BuildFailure(fixture);
			return false;
		}
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(HST_EnemyPatrolProofFixtureFactory.PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		fixture.m_Operation.m_sRouteContractHash = "enemy_patrol_proof_forged_route_hash";
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_EnemyOrderState order;
		if (restored)
			order = FindOrder(restored, fixture.m_Order.m_sOrderId);
		bool quarantined = IsQuarantined(order);
		bool evidencePreserved = restored && restored.FindOperation(fixture.m_Operation.m_sOperationId);
		evidencePreserved = evidencePreserved && restored.FindForceManifest(fixture.m_Manifest.m_sManifestId);
		evidencePreserved = evidencePreserved && restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
		evidencePreserved = evidencePreserved && restored.FindActiveGroup(fixture.m_Group.m_sGroupId);
		HST_FactionPoolState restoredPool;
		if (restored)
			restoredPool = restored.FindFactionPool(HST_EnemyPatrolProofFixtureFactory.PROOF_FACTION_KEY);
		bool noRefund = restoredPool && restoredPool.m_iAttackResources == attackBefore;
		noRefund = noRefund && order && !order.m_bResourceSettlementApplied;
		bool cleanupHeld = PreserveQuarantinedEvidenceAtCleanup(restored, order, fixture);
		evidence = string.Format("quarantined/preserved/no-refund/cleanup-held %1/%2/%3/%4", quarantined, evidencePreserved, noRefund, cleanupHeld);
		return quarantined && evidencePreserved && noRefund && cleanupHeld;
	}

	protected bool ProveBacklinkQuarantine(out string evidence)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("corrupt_backlink");
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = m_Fixtures.BuildFailure(fixture);
			return false;
		}
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(HST_EnemyPatrolProofFixtureFactory.PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		string forgedBacklink = "enemy_patrol_proof_foreign_operation_backlink";
		fixture.m_Group.m_sOperationId = forgedBacklink;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_EnemyOrderState order;
		HST_ActiveGroupState group;
		if (restored)
		{
			order = FindOrder(restored, fixture.m_Order.m_sOrderId);
			group = restored.FindActiveGroup(fixture.m_Group.m_sGroupId);
		}
		bool quarantined = IsQuarantined(order);
		bool backlinkPreserved = group && group.m_sOperationId == forgedBacklink;
		// The reciprocal-link conflict is first quarantined through the owning
		// order, then the orphan-claimant pass independently holds the forged
		// group.  Both statuses are fail-closed; require the final orphan hold as
		// well as the untouched foreign backlink instead of depending on the
		// earlier intermediate status string.
		backlinkPreserved = backlinkPreserved
			&& group.m_sRuntimeStatus == "exact_patrol_orphan_quarantined"
			&& group.m_sSpawnFallbackMode
				== HST_EnemyPatrolOperationService.EXACT_GROUP_MODE + "_quarantined";
		bool evidencePreserved = restored && restored.FindOperation(fixture.m_Operation.m_sOperationId);
		evidencePreserved = evidencePreserved && restored.FindForceManifest(fixture.m_Manifest.m_sManifestId);
		evidencePreserved = evidencePreserved && restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId) && group;
		HST_FactionPoolState restoredPool;
		if (restored)
			restoredPool = restored.FindFactionPool(HST_EnemyPatrolProofFixtureFactory.PROOF_FACTION_KEY);
		bool noRefund = restoredPool && restoredPool.m_iAttackResources == attackBefore;
		noRefund = noRefund && order && !order.m_bResourceSettlementApplied;
		bool cleanupHeld = PreserveQuarantinedEvidenceAtCleanup(restored, order, fixture);
		evidence = string.Format(
			"quarantined/backlink/preserved/no-refund/cleanup-held %1/%2/%3/%4/%5",
			quarantined,
			backlinkPreserved,
			evidencePreserved,
			noRefund,
			cleanupHeld);
		return quarantined && backlinkPreserved && evidencePreserved && noRefund && cleanupHeld;
	}

	protected bool ProvePartialReceiptQuarantine(out string evidence)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("corrupt_receipt");
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = m_Fixtures.BuildFailure(fixture);
			return false;
		}
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(HST_EnemyPatrolProofFixtureFactory.PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		fixture.m_Order.m_iRefundedAttackResources = 1;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_EnemyOrderState order;
		if (restored)
			order = FindOrder(restored, fixture.m_Order.m_sOrderId);
		HST_FactionPoolState restoredPool;
		if (restored)
			restoredPool = restored.FindFactionPool(HST_EnemyPatrolProofFixtureFactory.PROOF_FACTION_KEY);
		bool quarantined = IsQuarantined(order);
		bool receiptPreserved = order && order.m_iRefundedAttackResources == 1;
		receiptPreserved = receiptPreserved && !order.m_bResourceSettlementApplied;
		bool noRefund = restoredPool && restoredPool.m_iAttackResources == attackBefore;
		bool cleanupHeld = PreserveQuarantinedEvidenceAtCleanup(restored, order, fixture);
		evidence = string.Format("quarantined/partial-preserved/no-refund/cleanup-held %1/%2/%3/%4", quarantined, receiptPreserved, noRefund, cleanupHeld);
		return quarantined && receiptPreserved && noRefund && cleanupHeld;
	}

	protected void ProveDispatchIsolation(HST_EnemyPatrolOperationProofReport report)
	{
		HST_EnemyCommanderService commander = new HST_EnemyCommanderService();
		HST_EnemyPatrolOperationService patrol = new HST_EnemyPatrolOperationService();
		HST_EnemyQRFOperationService qrf = new HST_EnemyQRFOperationService();
		HST_EnemyOrderState exactPatrol = BuildDispatchOrder(
			HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL,
			HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION);
		HST_EnemyOrderState exactQRF = BuildDispatchOrder(
			HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF,
			HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION);
		HST_EnemyOrderState legacyPatrol = BuildDispatchOrder(HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL, 0);
		HST_EnemyOrderState quarantined = BuildDispatchOrder(
			HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL,
			HST_EnemyPatrolOperationService.QUARANTINED_CONTRACT_VERSION);
		HST_EnemyOrderState unsupported = BuildDispatchOrder(HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL, 99);
		bool ownersExact = commander.ResolveRuntimeOwner(exactPatrol) == HST_EnemyCommanderService.RUNTIME_OWNER_EXACT_PATROL;
		ownersExact = ownersExact && commander.ResolveRuntimeOwner(exactQRF) == HST_EnemyCommanderService.RUNTIME_OWNER_EXACT_QRF;
		ownersExact = ownersExact && commander.ResolveRuntimeOwner(legacyPatrol) == HST_EnemyCommanderService.RUNTIME_OWNER_LEGACY;
		ownersExact = ownersExact && commander.ResolveRuntimeOwner(quarantined) == HST_EnemyCommanderService.RUNTIME_OWNER_QUARANTINED;
		ownersExact = ownersExact && commander.ResolveRuntimeOwner(unsupported) == HST_EnemyCommanderService.RUNTIME_OWNER_UNSUPPORTED;
		bool predicatesExact = patrol.IsExactEnemyPatrol(exactPatrol) && !patrol.IsExactEnemyPatrol(exactQRF);
		predicatesExact = predicatesExact && qrf.IsExactEnemyDefensiveQRF(exactQRF) && !qrf.IsExactEnemyDefensiveQRF(exactPatrol);
		HST_EnemyPatrolPhysicalWarProofHarness physical = new HST_EnemyPatrolPhysicalWarProofHarness();
		HST_CampaignState orphanState = new HST_CampaignState();
		HST_ActiveGroupState exactModeOrphan = new HST_ActiveGroupState();
		exactModeOrphan.m_sGroupId = "enemy_patrol_proof_exact_mode_orphan";
		exactModeOrphan.m_sSpawnFallbackMode = HST_EnemyPatrolOperationService.EXACT_GROUP_MODE;
		HST_ActiveGroupState ordinaryOrphan = new HST_ActiveGroupState();
		ordinaryOrphan.m_sGroupId = "enemy_patrol_proof_ordinary_orphan";
		ordinaryOrphan.m_sSpawnFallbackMode = "ordinary_proof_group";
		bool orphanClassificationExact = physical.IsExactPatrolGroupForProof(orphanState, exactModeOrphan);
		orphanClassificationExact = orphanClassificationExact && !physical.IsExactPatrolGroupForProof(orphanState, ordinaryOrphan);
		string priorityEvidence;
		bool targetPriorityExact = ProveTargetPriority(priorityEvidence);
		report.m_bDispatchIsolationExact = ownersExact && predicatesExact && orphanClassificationExact && targetPriorityExact;
		report.m_sDispatchIsolationEvidence = string.Format(
			"owners patrol/qrf/legacy/quarantine/unsupported %1/%2/%3/%4/%5",
			commander.ResolveRuntimeOwner(exactPatrol),
			commander.ResolveRuntimeOwner(exactQRF),
			commander.ResolveRuntimeOwner(legacyPatrol),
			commander.ResolveRuntimeOwner(quarantined),
			commander.ResolveRuntimeOwner(unsupported));
		report.m_sDispatchIsolationEvidence = report.m_sDispatchIsolationEvidence + string.Format(
			" | owner/predicate/orphan-class exact %1/%2/%3",
			ownersExact,
			predicatesExact,
			orphanClassificationExact);
		report.m_sDispatchIsolationEvidence = report.m_sDispatchIsolationEvidence + " | priority " + priorityEvidence;
	}

	protected bool ProveTargetPriority(out string evidence)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("target_priority");
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = m_Fixtures.BuildFailure(fixture);
			return false;
		}
		HST_EnemyPatrolCommanderProofHarness commander = new HST_EnemyPatrolCommanderProofHarness();
		commander.SetExactEnemyQRFAuthorityServices(fixture.m_Planning, new HST_EnemyQRFOperationService());
		commander.SetExactEnemyPatrolAuthorityService(fixture.m_Service);
		HST_EnemyCounterattackOperationService counterattack
			= new HST_EnemyCounterattackOperationService();
		counterattack.SetRuntimeServices(
			fixture.m_Queue,
			fixture.m_Adapter,
			fixture.m_PhysicalWar,
			new HST_CombatPresenceService(),
			new HST_OwnershipTransitionService());
		commander.SetExactEnemyCounterattackAuthorityService(counterattack);
		HST_ZoneState target = fixture.m_State.FindZone(fixture.m_Order.m_sTargetZoneId);
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(HST_EnemyPatrolProofFixtureFactory.PROOF_FACTION_KEY);
		int ordersBefore = fixture.m_State.m_aEnemyOrders.Count();
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		bool duplicateQueued = commander.QueueOrderForProof(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			null,
			fixture.m_Order.m_sFactionKey,
			target,
			HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL);
		bool duplicateSuppressed = !duplicateQueued && fixture.m_State.m_aEnemyOrders.Count() == ordersBefore;
		duplicateSuppressed = duplicateSuppressed && pool.m_iAttackResources == attackBefore;
		// Immediate counterattacks are raised after the target changes hands.  The
		// admitted patrol deliberately remains on that target so this fixture can
		// prove that target-level duplicate suppression ignores only its exact
		// patrol authority while the counterattack uses its own typed admission.
		string resistanceFactionKey = fixture.m_Preset.m_sResistanceFactionKey;
		if (resistanceFactionKey.IsEmpty())
			resistanceFactionKey = "FIA";
		target.m_sOwnerFactionKey = resistanceFactionKey;
		bool counterattackQueued = commander.TryQueueImmediateCounterattack(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			new HST_SupportRequestService(),
			fixture.m_Order.m_sFactionKey,
			target,
			100);
		HST_EnemyOrderState response;
		if (fixture.m_State.m_aEnemyOrders.Count() > ordersBefore)
			response = fixture.m_State.m_aEnemyOrders[fixture.m_State.m_aEnemyOrders.Count() - 1];
		bool responseExact = counterattackQueued && response && response != fixture.m_Order;
		responseExact = responseExact && response.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK;
		responseExact = responseExact && response.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
		responseExact = responseExact && response.m_sTargetZoneId == fixture.m_Order.m_sTargetZoneId;
		bool patrolUntouched = fixture.m_State.FindEnemyOrder(fixture.m_Order.m_sOrderId) == fixture.m_Order;
		patrolUntouched = patrolUntouched && fixture.m_Order.m_bStrategicServiceCommitted;
		patrolUntouched = patrolUntouched && !fixture.m_Order.m_bResourceSettlementApplied;
		patrolUntouched = patrolUntouched && pool.m_iAttackResources == attackBefore;
		bool defenseSpent = response && pool.m_iSupportResources == supportBefore - response.m_iSupportCost;
		evidence = string.Format(
			"duplicate/counterattack/patrol-untouched/defense-spent %1/%2/%3/%4",
			duplicateSuppressed,
			responseExact,
			patrolUntouched,
			defenseSpent);
		return duplicateSuppressed && responseExact && patrolUntouched && defenseSpent;
	}

	protected void ProveMarkers(HST_EnemyPatrolOperationProofReport report)
	{
		HST_EnemyPatrolOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("markers");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sMarkerEvidence = m_Fixtures.BuildFailure(fixture);
			return;
		}
		fixture.m_State.m_iElapsedSeconds += 30;
		fixture.m_Service.TickOrder(fixture.m_State, fixture.m_Preset, fixture.m_EnemyDirector, fixture.m_Order);
		bool casualtyAccepted = ConfirmStrategicCasualty(fixture, "marker");
		int living = fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch);
		HST_MapMarkerService markers = new HST_MapMarkerService();
		markers.RebuildAllMarkers(fixture.m_State, fixture.m_Preset);
		string markerId = "hst_exact_enemy_patrol_" + fixture.m_Operation.m_sOperationId;
		HST_MapMarkerState marker = fixture.m_State.FindMapMarker(markerId);
		bool openExact = marker && marker.m_bVisible;
		openExact = openExact && Distance2D(marker.m_vPosition, fixture.m_Operation.m_vStrategicPosition) < 0.1;
		openExact = openExact && marker.m_sLabel.Contains("patrol") && marker.m_sLabel.Contains("leg");
		openExact = openExact && marker.m_sLabel.Contains(string.Format("%1 alive", living));
		openExact = openExact && CountMarkerId(fixture.m_State, markerId) == 1;
		bool settled = DriveUntilSettled(fixture, 80);
		markers.RebuildAllMarkers(fixture.m_State, fixture.m_Preset);
		bool cleanupExact = settled && fixture.m_State.FindMapMarker(markerId) == null;
		cleanupExact = cleanupExact && CountMarkerId(fixture.m_State, markerId) == 0;
		report.m_bMarkerExact = casualtyAccepted && living == fixture.m_Manifest.m_iAcceptedMemberCount - 1;
		report.m_bMarkerExact = report.m_bMarkerExact && openExact && cleanupExact;
		report.m_sMarkerEvidence = string.Format(
			"casualty/living/open/cursor/settled/cleanup %1/%2/%3/%4/%5/%6",
			casualtyAccepted,
			living,
			marker != null,
			marker && Distance2D(marker.m_vPosition, fixture.m_Operation.m_vStrategicPosition) < 0.1,
			settled,
			cleanupExact);
	}

	protected bool IsFrozenInfantryManifestExact(HST_EnemyPatrolOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_Manifest)
			return false;
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		HST_StrategicMovementService movement = new HST_StrategicMovementService();
		bool exact = fixture.m_Manifest.m_bFrozen && movement.IsSupportedExactInfantryManifest(fixture.m_Manifest);
		exact = exact && fixture.m_Manifest.m_sForceKind == HST_EnemyPatrolOperationService.EXACT_FORCE_KIND;
		exact = exact && fixture.m_Manifest.m_sPolicyId == HST_EnemyPatrolOperationService.EXACT_POLICY_ID;
		exact = exact && fixture.m_Manifest.m_sManifestHash == integrity.BuildManifestHash(fixture.m_Manifest);
		return exact;
	}

	protected bool AdmissionLinksExact(HST_EnemyPatrolOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_Order || !fixture.m_Operation || !fixture.m_Batch || !fixture.m_Group)
			return false;
		bool exact = fixture.m_Order.m_sOperationId == fixture.m_Operation.m_sOperationId;
		exact = exact && fixture.m_Order.m_sManifestId == fixture.m_Manifest.m_sManifestId;
		exact = exact && fixture.m_Order.m_sSpawnResultId == fixture.m_Batch.m_sResultId;
		exact = exact && fixture.m_Order.m_sGroupId == fixture.m_Group.m_sGroupId;
		exact = exact && fixture.m_Operation.m_sGroupId == fixture.m_Group.m_sGroupId;
		exact = exact && fixture.m_Batch.m_sProjectionId == fixture.m_Group.m_sProjectionId;
		exact = exact && fixture.m_Group.m_sEnemyOrderId == fixture.m_Order.m_sOrderId;
		return exact && fixture.m_Order.m_bStrategicServiceCommitted && !fixture.m_Group.m_bQRF;
	}

	protected bool AdmissionRouteExact(HST_EnemyPatrolOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_Operation || !fixture.m_Route)
			return false;
		HST_OperationRouteCursorService cursor = new HST_OperationRouteCursorService();
		bool exact = cursor.IsPatrolRouteContractValid(fixture.m_Operation, fixture.m_Route);
		exact = exact && fixture.m_Operation.m_iRouteWaypointIndex == 0;
		exact = exact && fixture.m_Operation.m_iRouteLapCount == 0;
		exact = exact && fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		exact = exact && fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		return exact && fixture.m_Batch.m_bStrategicProjectionHeld;
	}

	protected bool AdvanceUntilDuty(
		HST_EnemyPatrolOperationProofFixture fixture,
		HST_EOperationDutyState duty,
		int maxTicks)
	{
		if (!m_Fixtures.Ready(fixture))
			return false;
		for (int tick = 0; tick < maxTicks; tick++)
		{
			if (fixture.m_Operation.m_eDutyState == duty)
				return true;
			if (fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				return false;
			fixture.m_State.m_iElapsedSeconds += HST_OperationRouteCursorService.MAX_CATCHUP_SECONDS_PER_TICK;
			fixture.m_Service.TickOrder(fixture.m_State, fixture.m_Preset, fixture.m_EnemyDirector, fixture.m_Order);
		}
		return fixture.m_Operation.m_eDutyState == duty;
	}

	protected bool AdvanceUntilLap(HST_EnemyPatrolOperationProofFixture fixture, int lapCount, int maxTicks)
	{
		if (!m_Fixtures.Ready(fixture))
			return false;
		for (int tick = 0; tick < maxTicks; tick++)
		{
			if (fixture.m_Operation.m_iRouteLapCount >= lapCount)
				return true;
			fixture.m_State.m_iElapsedSeconds += HST_OperationRouteCursorService.MAX_CATCHUP_SECONDS_PER_TICK;
			fixture.m_Service.TickOrder(fixture.m_State, fixture.m_Preset, fixture.m_EnemyDirector, fixture.m_Order);
		}
		return fixture.m_Operation.m_iRouteLapCount >= lapCount;
	}

	protected bool DriveUntilSettled(HST_EnemyPatrolOperationProofFixture fixture, int maxTicks)
	{
		if (!m_Fixtures.Ready(fixture))
			return false;
		for (int tick = 0; tick < maxTicks; tick++)
		{
			if (fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				return true;
			fixture.m_State.m_iElapsedSeconds += HST_OperationRouteCursorService.MAX_CATCHUP_SECONDS_PER_TICK;
			fixture.m_Service.TickOrder(fixture.m_State, fixture.m_Preset, fixture.m_EnemyDirector, fixture.m_Order);
		}
		return fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
	}

	protected bool ConfirmStrategicCasualty(HST_EnemyPatrolOperationProofFixture fixture, string reasonToken)
	{
		if (!m_Fixtures.Ready(fixture))
			return false;
		string slotId = fixture.m_Queue.SelectStrategicLivingMemberSlotId(
			fixture.m_Batch,
			fixture.m_Operation.m_iDeterministicSeed);
		if (slotId.IsEmpty())
			return false;
		HST_ForceSpawnQueueCallbackResult casualty = fixture.m_Queue.ConfirmStrategicMemberCasualty(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			slotId,
			fixture.m_State.m_iElapsedSeconds + 1,
			"exact enemy patrol proof casualty " + reasonToken);
		return casualty && casualty.m_bAccepted;
	}

	protected void PrepareSuccessfulQueueCallbackFixture(
		HST_EnemyPatrolOperationProofFixture fixture,
		string reasonToken)
	{
		if (!fixture || !fixture.m_Batch)
			return;
		fixture.m_Batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
		fixture.m_Batch.m_bStrategicProjectionHeld = false;
		fixture.m_Batch.m_sNativeGroupId = "enemy_patrol_proof_native_group_" + reasonToken;
		fixture.m_Batch.m_sTerminalReason = "all exact manifest slots registered and verified";
		fixture.m_Batch.m_sLastFailureReason = "";
		fixture.m_Batch.m_iSuccessfulHandoffCount = 1;
		fixture.m_Batch.m_iCompletedAtSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Batch.m_iUpdatedAtSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Batch.m_iLifecycleRevision++;
		fixture.m_Batch.m_iLastLifecycleSecond = fixture.m_State.m_iElapsedSeconds;
		foreach (HST_ForceSpawnSlotResultState slot : fixture.m_Batch.m_aSlotResults)
		{
			if (!slot || (slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				&& slot.m_bCasualtyConfirmed))
				continue;
			slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
			slot.m_sSpawnedPrefab = ResolveFixtureSlotPrefab(fixture.m_Manifest, slot);
			slot.m_sEntityId = "enemy_patrol_proof_entity_" + slot.m_sSlotId;
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

	protected HST_ForceSpawnSlotResultState FindFirstMappedMemberSlot(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return null;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				&& slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
				&& !slot.m_sEntityId.IsEmpty() && slot.m_bEverAlive)
				return slot;
		}
		return null;
	}

	protected void ApplyGroupRoster(HST_ActiveGroupState group, int living)
	{
		if (!group)
			return;
		group.m_iInfantryCount = living;
		group.m_iDurableLivingInfantryCount = living;
		group.m_iLastSeenAliveCount = living;
		group.m_iSurvivorInfantryCount = living;
	}

	protected bool RuntimeIdsCleared(
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!batch || !group || !batch.m_sNativeGroupId.IsEmpty()
			|| group.m_bSpawnedEntity || !group.m_sRuntimeEntityId.IsEmpty())
			return false;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || !slot.m_sEntityId.IsEmpty()
				|| !slot.m_sAssignedVehicleEntityId.IsEmpty()
				|| !slot.m_sNativeGroupId.IsEmpty())
				return false;
		}
		return true;
	}

	protected HST_ForceSpawnResultState BuildForeignCollisionBatch(
		HST_EnemyOrderState order,
		string suffix)
	{
		HST_ForceSpawnResultState batch = new HST_ForceSpawnResultState();
		batch.m_sResultId = "spawn_" + order.m_sOrderId;
		batch.m_sRequestId = "enemy_patrol_proof_foreign_request_" + suffix;
		batch.m_sManifestId = "enemy_patrol_proof_foreign_manifest_" + suffix;
		batch.m_sManifestHash = "enemy_patrol_proof_foreign_hash_" + suffix;
		batch.m_sOperationId = "enemy_patrol_proof_foreign_operation_" + suffix;
		batch.m_sForceId = "enemy_patrol_proof_foreign_force_" + suffix;
		batch.m_sProjectionId = "enemy_patrol_proof_foreign_projection_" + suffix;
		batch.m_sLastFailureReason = "enemy_patrol_proof_foreign_evidence_" + suffix;
		batch.m_iLifecycleRevision = 71;
		return batch;
	}

	protected bool ForeignCollisionBatchExact(
		HST_CampaignState state,
		HST_ForceSpawnResultState batch,
		string suffix)
	{
		if (!state || !batch || state.FindForceSpawnResult(batch.m_sResultId) != batch)
			return false;
		return batch.m_sRequestId == "enemy_patrol_proof_foreign_request_" + suffix
			&& batch.m_sOperationId == "enemy_patrol_proof_foreign_operation_" + suffix
			&& batch.m_sProjectionId == "enemy_patrol_proof_foreign_projection_" + suffix
			&& batch.m_sLastFailureReason == "enemy_patrol_proof_foreign_evidence_" + suffix
			&& batch.m_iLifecycleRevision == 71 && !batch.m_bCancelRequested;
	}

	protected bool PreserveQuarantinedEvidenceAtCleanup(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EnemyPatrolOperationProofFixture fixture)
	{
		if (!state || !order || !fixture)
			return false;
		int operationCount = CountOperationId(state, fixture.m_Operation.m_sOperationId);
		int manifestCount = CountManifestId(state, fixture.m_Manifest.m_sManifestId);
		int batchCount = CountBatchId(state, fixture.m_Batch.m_sResultId);
		int groupCount = CountGroupId(state, fixture.m_Group.m_sGroupId);
		HST_EnemyPatrolOperationService service = new HST_EnemyPatrolOperationService();
		service.ReconcileSettledRuntimeCleanup(state);
		return IsQuarantined(order)
			&& CountOperationId(state, fixture.m_Operation.m_sOperationId) == operationCount
			&& CountManifestId(state, fixture.m_Manifest.m_sManifestId) == manifestCount
			&& CountBatchId(state, fixture.m_Batch.m_sResultId) == batchCount
			&& CountGroupId(state, fixture.m_Group.m_sGroupId) == groupCount;
	}

	protected HST_EnemyOrderState BuildDispatchOrder(HST_EEnemyOrderType type, int contractVersion)
	{
		HST_EnemyOrderState order = new HST_EnemyOrderState();
		order.m_eType = type;
		order.m_iOperationContractVersion = contractVersion;
		return order;
	}

	protected bool IsQuarantined(HST_EnemyOrderState order)
	{
		return order
			&& order.m_iOperationContractVersion == HST_EnemyPatrolOperationService.QUARANTINED_CONTRACT_VERSION
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& order.m_sRuntimeStatus == "exact_patrol_quarantined";
	}

	protected HST_EnemyOrderState FindOrder(HST_CampaignState state, string orderId)
	{
		if (!state || orderId.IsEmpty())
			return null;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (order && order.m_sOrderId == orderId)
				return order;
		}
		return null;
	}

	protected int CountOperationId(HST_CampaignState state, string operationId)
	{
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
		int count;
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
		{
			if (manifest && manifest.m_sManifestId == manifestId)
				count++;
		}
		return count;
	}

	protected int CountBatchId(HST_CampaignState state, string resultId)
	{
		int count;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && batch.m_sResultId == resultId)
				count++;
		}
		return count;
	}

	protected int CountGroupId(HST_CampaignState state, string groupId)
	{
		int count;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && group.m_sGroupId == groupId)
				count++;
		}
		return count;
	}

	protected int CountMarkerId(HST_CampaignState state, string markerId)
	{
		int count;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (marker && !marker.m_bTombstone && marker.m_bVisible && marker.m_sMarkerId == markerId)
				count++;
		}
		return count;
	}

	protected float Distance2D(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}
}
#endif
