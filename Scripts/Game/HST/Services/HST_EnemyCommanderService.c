class HST_EnemyTargetScoreCandidate
{
	string m_sZoneId;
	string m_sDisplayName;
	string m_sOwnerFactionKey;
	string m_sOwnerRelation;
	HST_EZoneType m_eType;
	int m_iScore;
	int m_iWeight;
	int m_iOwnerScore;
	int m_iPriorityScore;
	int m_iProgressScore;
	int m_iActivityScore;
	int m_iSupportScore;
	int m_iMissionScore;
	int m_iObjectiveScore;
	int m_iStrategicScore;
	int m_iHQScore;
	int m_iGarrisonScore;
	int m_iDamageScore;
	int m_iIncomeScore;
	int m_iCommitmentScore;
	int m_iCompatibleCommitmentCount;
	string m_sLocalityReason;
	string m_sCommitmentReason;
	string m_sReason;
}

class HST_EnemyTargetScoreResult
{
	bool m_bSuccess;
	string m_sFailureReason;
	string m_sFactionKey;
	string m_sSelectedZoneId;
	string m_sBestZoneId;
	string m_sSelectionMode;
	string m_sReason;
	int m_iSelectedScore;
	int m_iBestScore;
	int m_iCandidateCount;
	int m_iEligibleCount;
	int m_iLocalityRejectedCount;
	int m_iCommitmentRejectedCount;
	int m_iTotalWeight;
	int m_iRoll;
	string m_sLocalityRejectedReason;
	string m_sCommitmentRejectedReason;
	ref array<ref HST_EnemyTargetScoreCandidate> m_aCandidates = {};
}

class HST_EnemyPreparedAdmissionResult
{
	bool m_bAccepted;
	bool m_bRetryable;
	bool m_bTerminal;
	bool m_bStateChanged;
	string m_sFailureReason;
	ref HST_EnemyOrderState m_Order;
}

class HST_EnemyCommanderService
{
	static const int ORDER_TICK_SECONDS = 180;
	static const int ORDER_RESOLVE_SECONDS = 420;
	static const int PHYSICAL_ORDER_TIMEOUT_SECONDS = 300;
	static const float HQ_PRESSURE_ZONE_RADIUS_METERS = 1000.0;
	static const int HQ_PRESSURE_MIN_KNOWLEDGE_FOR_OPPORTUNITY_ATTACK = 25;
	static const float LOCAL_OPERATION_FRONT_RADIUS_METERS = 3000.0;
	static const int TOWN_SUPPORT_QRF_RESPONSE_THRESHOLD = 30;
	static const int RECENT_THREAT_WINDOW_SECONDS = 900;
	static const string SPEND_MODE_PROACTIVE_ATTACK = "proactive_attack";
	static const string SPEND_MODE_REACTIVE_DEFENSE = "reactive_defense";
	static const string RUNTIME_OWNER_LEGACY = "legacy";
	static const string RUNTIME_OWNER_EXACT_QRF = "exact_enemy_qrf";
	static const string RUNTIME_OWNER_EXACT_COUNTERATTACK = "exact_enemy_counterattack";
	static const string RUNTIME_OWNER_EXACT_PATROL = "exact_enemy_patrol";
	static const string RUNTIME_OWNER_EXACT_GARRISON_REBUILD = "exact_enemy_garrison_rebuild";
	static const string RUNTIME_OWNER_QUARANTINED = "quarantined";
	static const string RUNTIME_OWNER_UNSUPPORTED = "unsupported_versioned";
	static const int PLANNING_RETRY_SECONDS = 30;
	static const int PLANNING_UNAVAILABLE_REMINDER_SECONDS = 300;
	protected ref HST_ForcePlanningService m_ForcePlanning;
	protected ref HST_EnemyPlanningAuthorityService m_EnemyPlanningAuthority = new HST_EnemyPlanningAuthorityService();
	protected ref HST_EnemyQRFOperationService m_ExactEnemyQRF;
	protected ref HST_EnemyCounterattackOperationService m_ExactEnemyCounterattack;
	protected ref HST_EnemyPatrolOperationService m_ExactEnemyPatrol;
	protected ref HST_EnemyGarrisonRebuildOperationService m_ExactEnemyGarrisonRebuild;
	protected ref HST_GarrisonService m_GarrisonStrength = new HST_GarrisonService();
	protected ref HST_CombatPresenceService m_CombatPresence = new HST_CombatPresenceService();
	protected ref HST_TownInfluenceService m_TownInfluence;
	protected ref map<string, string> m_mPlanningUnavailableFailureByFaction
		= new map<string, string>();
	protected ref map<string, int> m_mPlanningUnavailableLastReportSecondByFaction
		= new map<string, int>();

	void SetCombatPresenceService(HST_CombatPresenceService combatPresence)
	{
		if (combatPresence)
			m_CombatPresence = combatPresence;
	}

	void SetTownInfluenceService(HST_TownInfluenceService townInfluence)
	{
		m_TownInfluence = townInfluence;
	}

	void SetExactEnemyQRFAuthorityServices(
		HST_ForcePlanningService forcePlanning,
		HST_EnemyQRFOperationService exactEnemyQRF)
	{
		m_ForcePlanning = forcePlanning;
		m_ExactEnemyQRF = exactEnemyQRF;
	}

	void SetExactEnemyPatrolAuthorityService(HST_EnemyPatrolOperationService exactEnemyPatrol)
	{
		m_ExactEnemyPatrol = exactEnemyPatrol;
	}

	void SetExactEnemyCounterattackAuthorityService(
		HST_EnemyCounterattackOperationService exactEnemyCounterattack)
	{
		m_ExactEnemyCounterattack = exactEnemyCounterattack;
	}

	void SetExactEnemyGarrisonRebuildAuthorityService(
		HST_EnemyGarrisonRebuildOperationService exactEnemyGarrisonRebuild)
	{
		m_ExactEnemyGarrisonRebuild = exactEnemyGarrisonRebuild;
	}

	// Public so the deterministic Campaign Debug proof can exercise the same
	// transition/reminder gate used by the production planner loop.
	bool ShouldReportPlanningUnavailable(
		string factionKey,
		string failure,
		int elapsedSecond)
	{
		if (factionKey.IsEmpty())
			return false;

		if (!m_mPlanningUnavailableFailureByFaction.Contains(factionKey)
			|| m_mPlanningUnavailableFailureByFaction.Get(factionKey) != failure
			|| !m_mPlanningUnavailableLastReportSecondByFaction.Contains(factionKey))
		{
			m_mPlanningUnavailableFailureByFaction.Set(factionKey, failure);
			m_mPlanningUnavailableLastReportSecondByFaction.Set(
				factionKey,
				Math.Max(0, elapsedSecond));
			return true;
		}

		int previousSecond
			= m_mPlanningUnavailableLastReportSecondByFaction.Get(factionKey);
		if (elapsedSecond >= previousSecond
			&& elapsedSecond - previousSecond
				< PLANNING_UNAVAILABLE_REMINDER_SECONDS)
			return false;

		m_mPlanningUnavailableLastReportSecondByFaction.Set(
			factionKey,
			Math.Max(0, elapsedSecond));
		return true;
	}

	bool ClearPlanningUnavailableReportState(string factionKey)
	{
		if (factionKey.IsEmpty()
			|| !m_mPlanningUnavailableFailureByFaction.Contains(factionKey))
			return false;

		m_mPlanningUnavailableFailureByFaction.Remove(factionKey);
		m_mPlanningUnavailableLastReportSecondByFaction.Remove(factionKey);
		return true;
	}

	string ResolveRuntimeOwner(HST_EnemyOrderState order)
	{
		if (!order || order.m_iOperationContractVersion == 0)
			return RUNTIME_OWNER_LEGACY;
		if (HST_OperationService.RequiresExactEnemyDefensiveQRF(order))
			return RUNTIME_OWNER_EXACT_QRF;
		if (HST_OperationService.RequiresExactEnemyCounterattack(order))
			return RUNTIME_OWNER_EXACT_COUNTERATTACK;
		if (HST_OperationService.RequiresExactEnemyPatrol(order))
			return RUNTIME_OWNER_EXACT_PATROL;
		if (HST_OperationService.RequiresExactEnemyGarrisonRebuild(order))
			return RUNTIME_OWNER_EXACT_GARRISON_REBUILD;
		if (order.m_iOperationContractVersion == HST_EnemyPatrolOperationService.QUARANTINED_CONTRACT_VERSION
			|| order.m_iOperationContractVersion == HST_EnemyCounterattackOperationService.QUARANTINED_CONTRACT_VERSION
			|| order.m_iOperationContractVersion == HST_EnemyGarrisonRebuildOperationService.QUARANTINED_CONTRACT_VERSION)
			return RUNTIME_OWNER_QUARANTINED;
		return RUNTIME_OWNER_UNSUPPORTED;
	}

	protected bool HasVersionedEnemyOperation(HST_EnemyOrderState order)
	{
		return order && order.m_iOperationContractVersion != 0;
	}

	bool Tick(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, HST_SupportRequestService support, HST_GarrisonService garrisons, int elapsedSeconds)
	{
		if (!state || !preset || !enemyDirector || elapsedSeconds <= 0)
			return false;

		bool changed = TickActiveOrderRuntime(state, preset, support, garrisons, enemyDirector);
		changed = ResolveOrders(state, preset, garrisons) || changed;
		return TickPeriodicPlanning(state, preset, enemyDirector, support) || changed;
	}

	protected bool TickPeriodicPlanning(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_SupportRequestService support)
	{
		if (!state || !preset || !enemyDirector || !m_EnemyPlanningAuthority)
			return false;
		ref array<string> factionKeys = {};
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || !HST_FactionRelationService.IsEnemyFaction(preset, pool.m_sFactionKey)
				|| factionKeys.Contains(pool.m_sFactionKey))
				continue;
			factionKeys.Insert(pool.m_sFactionKey);
		}
		factionKeys.Sort();

		bool changed;
		foreach (string factionKey : factionKeys)
		{
			HST_EnemyPlanningState planning;
			string planningFailure;
			if (!m_EnemyPlanningAuthority.ResolveExactState(
				state,
				factionKey,
				planning,
				planningFailure))
			{
				if (ShouldReportPlanningUnavailable(
					factionKey,
					planningFailure,
					state.m_iElapsedSeconds))
					Print(string.Format("Partisan enemy planner | %1 unavailable | %2", factionKey, planningFailure), LogLevel.WARNING);
				continue;
			}
			if (ClearPlanningUnavailableReportState(factionKey))
				Print(string.Format("Partisan enemy planner | %1 authority recovered", factionKey));

			if (m_EnemyPlanningAuthority.IsPrepared(planning))
			{
				if (state.m_iElapsedSeconds < planning.m_iNextRetrySecond)
					continue;
				HST_EnemyPreparedAdmissionResult retried = ConsumePreparedPeriodicDecision(
					state,
					preset,
					enemyDirector,
					support,
					planning);
				changed = (retried && retried.m_bStateChanged) || changed;
				continue;
			}
			if (!m_EnemyPlanningAuthority.IsDue(planning, state.m_iElapsedSeconds))
				continue;

			HST_EnemyPreparedAdmissionResult prepared = PrepareNextPeriodicDecision(
				state,
				preset,
				enemyDirector,
				planning);
			changed = (prepared && prepared.m_bStateChanged) || changed;
			if (!prepared || !prepared.m_bAccepted
				|| !m_EnemyPlanningAuthority.IsPrepared(planning))
				continue;

			HST_EnemyPreparedAdmissionResult consumed = ConsumePreparedPeriodicDecision(
				state,
				preset,
				enemyDirector,
				support,
				planning);
			changed = (consumed && consumed.m_bStateChanged) || changed;
		}
		return changed;
	}

	// Campaign Debug proof hook: prepares only this faction's next due decision.
	HST_EnemyPreparedAdmissionResult DebugPrepareNextPeriodicDecisionForFaction(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		string factionKey)
	{
		HST_EnemyPlanningState planning;
		string failure;
		if (!m_EnemyPlanningAuthority || !m_EnemyPlanningAuthority.ResolveExactState(
			state,
			factionKey,
			planning,
			failure))
			return BuildPreparedAdmissionFailure(failure, false, true);
		return PrepareNextPeriodicDecision(state, preset, enemyDirector, planning);
	}

	// Campaign Debug proof hook: consumes only an already-prepared faction row.
	HST_EnemyPreparedAdmissionResult DebugConsumePreparedPeriodicDecisionForFaction(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_SupportRequestService support,
		string factionKey)
	{
		HST_EnemyPlanningState planning;
		string failure;
		if (!m_EnemyPlanningAuthority || !m_EnemyPlanningAuthority.ResolveExactState(
			state,
			factionKey,
			planning,
			failure))
			return BuildPreparedAdmissionFailure(failure, false, true);
		return ConsumePreparedPeriodicDecision(state, preset, enemyDirector, support, planning);
	}

	// Campaign Debug proof hook: validates the focal runtime owner and executes
	// only that exact counterattack through its production runtime service.
	bool DebugTickExactCounterattackOrderRuntime(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order)
	{
		if (!state || !preset || !enemyDirector || !order
			|| !m_ExactEnemyCounterattack
			|| ResolveRuntimeOwner(order) != RUNTIME_OWNER_EXACT_COUNTERATTACK)
			return false;
		return m_ExactEnemyCounterattack.TickOrder(
			state,
			preset,
			enemyDirector,
			order);
	}

	// Campaign Debug proof hook: validates the focal runtime owner and executes
	// only that exact garrison rebuild through its production runtime service.
	bool DebugTickExactGarrisonRebuildOrderRuntime(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order)
	{
		if (!state || !preset || !enemyDirector || !order
			|| !m_ExactEnemyGarrisonRebuild
			|| ResolveRuntimeOwner(order)
				!= RUNTIME_OWNER_EXACT_GARRISON_REBUILD)
			return false;
		return m_ExactEnemyGarrisonRebuild.TickOrder(
			state,
			preset,
			enemyDirector,
			order);
	}

	// Campaign Debug proof hook: observes the exact persisted candidate-set hash.
	string DebugBuildTargetCandidateFingerprint(
		HST_EnemyTargetScoreResult candidates,
		out int candidateCount)
	{
		return BuildTargetCandidateFingerprint(candidates, candidateCount);
	}

	// Campaign Debug proof hook: observes the production order-type compatibility
	// rule without mutating any order or operation state.
	bool DebugIsTargetBlockedForOrderType(
		HST_CampaignState state,
		string factionKey,
		string zoneId,
		HST_EEnemyOrderType orderType)
	{
		bool ignoreExactPatrol = orderType
			!= HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL;
		return HasActiveOrderForZone(
			state,
			factionKey,
			zoneId,
			ignoreExactPatrol);
	}

	protected HST_EnemyPreparedAdmissionResult PrepareNextPeriodicDecision(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyPlanningState planning)
	{
		if (!state || !preset || !enemyDirector || !planning || !m_EnemyPlanningAuthority)
			return BuildPreparedAdmissionFailure("enemy planning preparation context is missing", true, false);
		if (m_EnemyPlanningAuthority.IsPrepared(planning))
		{
			HST_EnemyPreparedAdmissionResult replay = new HST_EnemyPreparedAdmissionResult();
			replay.m_bAccepted = true;
			return replay;
		}
		if (!m_EnemyPlanningAuthority.IsDue(planning, state.m_iElapsedSeconds))
			return BuildPreparedAdmissionFailure("enemy planning decision is not due", false, true);

		HST_FactionPoolState pool = state.FindFactionPool(planning.m_sFactionKey);
		if (!pool)
			return RetryUnpreparedDecision(
				planning,
				"enemy planning faction pool is unavailable",
				state.m_iElapsedSeconds);
		int commitmentCount;
		string commitmentFingerprint = HST_EnemyPlanningAuthorityService.BuildCommitmentFingerprint(
			state,
			planning.m_sFactionKey,
			commitmentCount);
		if (commitmentFingerprint.IsEmpty())
			return RetryUnpreparedDecision(
				planning,
				"enemy planning commitment fingerprint is unavailable",
				state.m_iElapsedSeconds);

		HST_EnemyTargetScoreResult candidateSet = BuildTargetScoreResult(
			state,
			preset,
			planning.m_sFactionKey,
			true);
		int targetCandidateCount;
		string targetCandidateFingerprint = BuildTargetCandidateFingerprint(
			candidateSet,
			targetCandidateCount);
		if (targetCandidateFingerprint.IsEmpty())
			return RetryUnpreparedDecision(
				planning,
				"enemy planning target candidate fingerprint is unavailable",
				state.m_iElapsedSeconds);

		int decisionSequence = planning.m_iDecisionSequence + 1;
		int decisionBucketSecond = planning.m_iNextPlanningBucketSecond;
		string targetSalt = BuildStableDecisionSalt(
			state,
			planning.m_sFactionKey,
			decisionSequence,
			decisionBucketSecond,
			commitmentFingerprint,
			targetCandidateFingerprint,
			"");
		HST_EnemyTargetScoreResult selectedTarget;
		HST_ZoneState targetZone;
		ref array<string> sourceZoneIds = {};
		string sourceCandidateFingerprint;
		HST_ZoneState sourceZone;
		string decisionSalt;
		int pressureBefore;
		int pressureDelta;
		int pressureAfter;
		HST_EEnemyOrderType orderType = HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL;
		string spendMode = SPEND_MODE_PROACTIVE_ATTACK;
		int attackCost;
		int supportCost;
		HST_ESupportRequestType plannedSupportType;
		ref array<string> orderTypeRejectedZoneIds = {};
		while (orderTypeRejectedZoneIds.Count() <= targetCandidateCount)
		{
			selectedTarget = BuildTargetScoreResult(
				state,
				preset,
				planning.m_sFactionKey,
				false,
				targetSalt,
				orderTypeRejectedZoneIds);
			targetZone = null;
			sourceZoneIds.Clear();
			sourceZone = null;
			pressureBefore = 0;
			pressureDelta = 0;
			pressureAfter = 0;
			orderType = HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL;
			spendMode = SPEND_MODE_PROACTIVE_ATTACK;
			attackCost = 0;
			supportCost = 0;
			if (selectedTarget && selectedTarget.m_bSuccess)
				targetZone = state.FindZone(selectedTarget.m_sSelectedZoneId);
			if (targetZone)
				BuildCanonicalSourceZoneIds(
					state,
					planning.m_sFactionKey,
					targetZone,
					sourceZoneIds);
			sourceCandidateFingerprint = BuildSourceCandidateFingerprint(
				state,
				targetZone,
				sourceZoneIds);
			if (targetZone)
				sourceZone = ResolveOrderSourceZone(
					state,
					planning.m_sFactionKey,
					targetZone);
			decisionSalt = BuildStableDecisionSalt(
				state,
				planning.m_sFactionKey,
				decisionSequence,
				decisionBucketSecond,
				commitmentFingerprint,
				targetCandidateFingerprint,
				sourceCandidateFingerprint);
			BuildTargetPressureProjection(
				state,
				preset,
				enemyDirector,
				planning.m_sFactionKey,
				targetZone,
				pressureBefore,
				pressureDelta,
				pressureAfter);
			if (targetZone)
				orderType = SelectOrderType(
					state,
					preset,
					targetZone,
					pool,
					decisionSalt,
					pressureAfter);
			if (targetZone)
				spendMode = ResolveOrderSpendMode(
					state,
					preset,
					targetZone,
					orderType,
					"");
			if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL)
				spendMode = SPEND_MODE_PROACTIVE_ATTACK;
			if (targetZone)
				ResolveOrderCostsForSpendMode(
					orderType,
					spendMode,
					attackCost,
					supportCost);
			plannedSupportType = SupportTypeForOrder(
				state,
				preset,
				targetZone,
				orderType,
				decisionSalt);
			if (!targetZone)
				break;

			bool ignoreExactPatrol = orderType
				!= HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL;
			if (!HasActiveOrderForZone(
				state,
				planning.m_sFactionKey,
				targetZone.m_sZoneId,
				ignoreExactPatrol))
				break;
			if (orderTypeRejectedZoneIds.Contains(targetZone.m_sZoneId))
			{
				targetZone = null;
				break;
			}
			orderTypeRejectedZoneIds.Insert(targetZone.m_sZoneId);
		}

		string decisionId = HST_EnemyPlanningAuthorityService.BuildDecisionId(
			planning.m_sFactionKey,
			decisionSequence,
			decisionBucketSecond);
		string orderId = HST_EnemyPlanningAuthorityService.BuildOrderId(decisionId);
		string operationId = HST_EnemyPlanningAuthorityService.BuildOperationId(orderId);
		string debitMutationId = HST_EnemyPlanningAuthorityService.BuildDebitMutationId(orderId);
		HST_EnemyOrderState frozenOrder = BuildOrderFromFrozenPlanningValues(
			state,
			planning.m_sFactionKey,
			targetZone,
			sourceZone,
			orderType,
			spendMode,
			attackCost,
			supportCost,
			plannedSupportType,
			orderId,
			operationId,
			debitMutationId,
			decisionBucketSecond);

		HST_ForceManifestState plannedManifest;
		HST_GeneratedRouteState plannedRoute;
		string routeHash;
		string planningFailure;
		if (targetZone && sourceZone && !BuildFrozenPlanningCapability(
			state,
			preset,
			planning,
			frozenOrder,
			plannedSupportType,
			plannedManifest,
			plannedRoute,
			routeHash,
			planningFailure))
			return RetryUnpreparedDecision(
				planning,
				planningFailure,
				state.m_iElapsedSeconds);

		int operationContractVersion;
		if (frozenOrder)
			operationContractVersion = frozenOrder.m_iOperationContractVersion;
		string manifestId;
		string manifestHash;
		if (plannedManifest)
		{
			manifestId = plannedManifest.m_sManifestId;
			manifestHash = plannedManifest.m_sManifestHash;
			frozenOrder.m_sManifestId = manifestId;
			frozenOrder.m_sManifestHash = manifestHash;
		}
		string capabilityHash = BuildFrozenPlanningCapabilityHash(
			state,
			frozenOrder,
			orderType,
			plannedSupportType,
			operationContractVersion,
			manifestHash,
			routeHash);
		if (frozenOrder)
			frozenOrder.m_sPlanningCapabilityHash = capabilityHash;

		HST_EnemyPlanningDecisionCommand command = new HST_EnemyPlanningDecisionCommand();
		command.m_sFactionKey = planning.m_sFactionKey;
		command.m_iExpectedDecisionSequence = decisionSequence;
		command.m_iExpectedNextPlanningBucketSecond = decisionBucketSecond;
		command.m_iDecisionBucketSecond = decisionBucketSecond;
		command.m_iObservedWarLevel = Math.Max(0, state.m_iWarLevel);
		command.m_iObservedAggression = Math.Max(0, pool.m_iAggression);
		command.m_iObservedPoolRevision = pool.m_iStrategicRevision;
		command.m_iObservedOperationalMutationCount = Math.Max(0, pool.m_iStrategicOperationalMutationCount);
		command.m_iObservedAttackResources = Math.Max(0, pool.m_iAttackResources);
		command.m_iObservedSupportResources = Math.Max(0, pool.m_iSupportResources);
		command.m_iCommitmentCount = commitmentCount;
		command.m_sCommitmentFingerprint = commitmentFingerprint;
		command.m_iTargetCandidateCount = targetCandidateCount;
		command.m_sTargetCandidateFingerprint = targetCandidateFingerprint;
		command.m_iSourceCandidateCount = sourceZoneIds.Count();
		command.m_sSourceCandidateFingerprint = sourceCandidateFingerprint;
		if (targetZone)
			command.m_sSelectedTargetZoneId = targetZone.m_sZoneId;
		if (sourceZone)
			command.m_sSelectedSourceZoneId = sourceZone.m_sZoneId;
		command.m_eSelectedOrderType = orderType;
		command.m_ePlannedSupportType = plannedSupportType;
		command.m_sPlanningCapabilityHash = capabilityHash;
		command.m_sSpendMode = spendMode;
		command.m_iAttackCost = attackCost;
		command.m_iSupportCost = supportCost;
		command.m_iTargetPressureBefore = pressureBefore;
		command.m_iTargetPressureDelta = pressureDelta;
		command.m_iTargetPressureAfter = pressureAfter;
		command.m_bTargetPressureApplied = false;
		command.m_sDecisionId = decisionId;
		command.m_sPlannedOrderId = orderId;
		command.m_sPlannedOperationId = operationId;
		command.m_sPlannedManifestId = manifestId;
		command.m_sPlannedManifestHash = manifestHash;
		command.m_sPlannedDebitMutationId = debitMutationId;

		HST_EnemyPlanningDecisionResult begun = m_EnemyPlanningAuthority.BeginDecision(planning, command);
		if (!begun || !begun.m_bAccepted)
		{
			string beginFailure = "enemy planning decision could not be frozen";
			if (begun && !begun.m_sFailureReason.IsEmpty())
				beginFailure = begun.m_sFailureReason;
			return BuildPreparedAdmissionFailure(beginFailure, false, true, begun && begun.m_bChanged);
		}

		HST_EnemyPreparedAdmissionResult result = new HST_EnemyPreparedAdmissionResult();
		result.m_bAccepted = true;
		result.m_bStateChanged = begun.m_bChanged;
		if (!targetZone)
			return CompletePreparedWithoutOrder(planning, "skipped", "no eligible enemy planning target", result.m_bStateChanged);
		if (!sourceZone)
			return CompletePreparedWithoutOrder(planning, "rejected", "frozen enemy planning source is unavailable", result.m_bStateChanged);
		bool ignoreExactPatrol = orderType != HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL;
		if (HasActiveOrderForZone(
			state,
			planning.m_sFactionKey,
			targetZone.m_sZoneId,
			ignoreExactPatrol))
			return CompletePreparedWithoutOrder(planning, "skipped", "target already has an active enemy order", result.m_bStateChanged);

		// Preparation is intentionally freeze-only. The normal tick consumes this
		// decision immediately, but a restart or intervening mutation must still
		// revalidate commitments and candidates before pressure or resource debit.
		return result;
	}

	protected HST_EnemyPreparedAdmissionResult ConsumePreparedPeriodicDecision(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_SupportRequestService support,
		HST_EnemyPlanningState planning)
	{
		if (!state || !preset || !enemyDirector || !planning || !m_EnemyPlanningAuthority
			|| !m_EnemyPlanningAuthority.IsPrepared(planning))
			return BuildPreparedAdmissionFailure("enemy planning decision is not prepared", false, true);

		HST_EnemyOrderState durableOrder = state.FindEnemyOrder(planning.m_sPlannedOrderId);
		if (durableOrder)
		{
			HST_EnemyPreparedAdmissionResult durablePressure = ApplyFrozenTargetPressure(
				state,
				enemyDirector,
				planning);
			bool durablePressureChanged = durablePressure && durablePressure.m_bStateChanged;
			return CompletePreparedWithOrder(
				state,
				preset,
				enemyDirector,
				planning,
				durableOrder,
				durablePressureChanged);
		}

		HST_FactionPoolState pool = state.FindFactionPool(planning.m_sFactionKey);
		if (!pool)
			return RetryPreparedDecision(planning, "frozen enemy planning faction pool is unavailable", state.m_iElapsedSeconds);
		int commitmentCount;
		string commitmentFingerprint = HST_EnemyPlanningAuthorityService.BuildCommitmentFingerprint(
			state,
			planning.m_sFactionKey,
			commitmentCount);
		if (commitmentCount != planning.m_iCommitmentCount
			|| commitmentFingerprint != planning.m_sCommitmentFingerprint)
			return RejectPreparedDecision(planning, "enemy planning commitment fingerprint changed before admission");
		if (!planning.m_bTargetPressureApplied)
		{
			HST_EnemyTargetScoreResult candidateSet = BuildTargetScoreResult(
				state,
				preset,
				planning.m_sFactionKey,
				true);
			int targetCandidateCount;
			string targetCandidateFingerprint = BuildTargetCandidateFingerprint(
				candidateSet,
				targetCandidateCount);
			if (targetCandidateCount != planning.m_iTargetCandidateCount
				|| targetCandidateFingerprint != planning.m_sTargetCandidateFingerprint)
				return RejectPreparedDecision(planning, "enemy planning target candidate fingerprint changed before admission");
		}

		HST_ZoneState targetZone = state.FindZone(planning.m_sSelectedTargetZoneId);
		if (!targetZone)
			return RejectPreparedDecision(planning, "frozen enemy planning target is unavailable");
		string targetReason;
		if (!IsEligibleTargetZone(targetZone, targetReason))
			return RejectPreparedDecision(planning, "frozen enemy planning target is no longer eligible: " + targetReason);
		string localityReason;
		if (!IsLocalOperationTargetAllowed(
			state,
			preset,
			planning.m_sFactionKey,
			targetZone,
			localityReason))
			return RejectPreparedDecision(planning, "frozen enemy planning locality changed: " + localityReason);
		if (!IsFrozenTargetRelationStructurallyValid(
			preset,
			planning.m_sFactionKey,
			targetZone,
			planning.m_eSelectedOrderType))
			return RejectPreparedDecision(planning, "frozen enemy planning target ownership relation changed");

		ref array<string> sourceZoneIds = {};
		BuildCanonicalSourceZoneIds(state, planning.m_sFactionKey, targetZone, sourceZoneIds);
		string sourceFingerprint = BuildSourceCandidateFingerprint(state, targetZone, sourceZoneIds);
		if (!planning.m_bTargetPressureApplied
			&& (sourceZoneIds.Count() != planning.m_iSourceCandidateCount
				|| sourceFingerprint != planning.m_sSourceCandidateFingerprint))
			return RejectPreparedDecision(planning, "enemy planning source candidate fingerprint changed before admission");
		HST_ZoneState sourceZone = state.FindZone(planning.m_sSelectedSourceZoneId);
		if (!sourceZone || !sourceZoneIds.Contains(sourceZone.m_sZoneId))
			return RejectPreparedDecision(planning, "frozen enemy planning source is unavailable or ineligible");
		if (!planning.m_bTargetPressureApplied)
		{
			HST_ZoneState resolvedSource = ResolveOrderSourceZone(
				state,
				planning.m_sFactionKey,
				targetZone);
			if (!resolvedSource || resolvedSource.m_sZoneId != sourceZone.m_sZoneId)
				return RejectPreparedDecision(planning, "frozen enemy planning source no longer wins the canonical distance tie-break");
		}

		bool ignoreExactPatrol = planning.m_eSelectedOrderType
			!= HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL;
		if (HasActiveOrderForZone(
			state,
			planning.m_sFactionKey,
			targetZone.m_sZoneId,
			ignoreExactPatrol))
			return RejectPreparedDecision(planning, "frozen enemy planning target acquired another active order");
		if (planning.m_eSelectedOrderType
			== HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON)
		{
			HST_EnemyOrderState ownershipProbe = BuildOrderFromFrozenPlanningValues(
				state,
				planning.m_sFactionKey,
				targetZone,
				sourceZone,
				planning.m_eSelectedOrderType,
				planning.m_sSpendMode,
				planning.m_iAttackCost,
				planning.m_iSupportCost,
				planning.m_ePlannedSupportType,
				planning.m_sPlannedOrderId,
				planning.m_sPlannedOperationId,
				planning.m_sPlannedDebitMutationId,
				planning.m_iDecisionBucketSecond);
			string ownershipCapabilityHash = BuildFrozenPlanningCapabilityHash(
				state,
				ownershipProbe,
				planning.m_eSelectedOrderType,
				planning.m_ePlannedSupportType,
				HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_CONTRACT_VERSION,
				planning.m_sPlannedManifestHash,
				"");
			if (!ownershipProbe || ownershipCapabilityHash.IsEmpty()
				|| ownershipCapabilityHash != planning.m_sPlanningCapabilityHash)
			{
				return RejectPreparedDecision(
					planning,
					"frozen enemy garrison rebuild ownership capability changed before pressure");
			}
		}

		HST_EnemyPreparedAdmissionResult pressureApplied = ApplyFrozenTargetPressure(
			state,
			enemyDirector,
			planning);
		if (!pressureApplied || !pressureApplied.m_bAccepted)
		{
			string pressureFailure = "frozen enemy planning target pressure could not be applied";
			if (pressureApplied && !pressureApplied.m_sFailureReason.IsEmpty())
				pressureFailure = pressureApplied.m_sFailureReason;
			return RejectPreparedDecision(planning, pressureFailure);
		}
		bool pressureStateChanged = pressureApplied.m_bStateChanged;

		HST_EnemyOrderState order = BuildOrderFromFrozenPlanningValues(
			state,
			planning.m_sFactionKey,
			targetZone,
			sourceZone,
			planning.m_eSelectedOrderType,
			planning.m_sSpendMode,
			planning.m_iAttackCost,
			planning.m_iSupportCost,
			planning.m_ePlannedSupportType,
			planning.m_sPlannedOrderId,
			planning.m_sPlannedOperationId,
			planning.m_sPlannedDebitMutationId,
			planning.m_iDecisionBucketSecond);
		if (!order)
			return RejectPreparedDecision(planning, "frozen enemy planning order could not be reconstructed");
		CopyPlanningBacklinks(order, planning);

		HST_ForceManifestState manifest;
		HST_GeneratedRouteState route;
		string routeHash;
		string capabilityFailure;
		if (!BuildFrozenPlanningCapability(
			state,
			preset,
			planning,
			order,
			planning.m_ePlannedSupportType,
			manifest,
			route,
			routeHash,
			capabilityFailure))
			return RetryPreparedDecision(planning, capabilityFailure, state.m_iElapsedSeconds);
		string manifestId;
		string manifestHash;
		if (manifest)
		{
			manifestId = manifest.m_sManifestId;
			manifestHash = manifest.m_sManifestHash;
		}
		string capabilityHash = BuildFrozenPlanningCapabilityHash(
			state,
			order,
			order.m_eType,
			planning.m_ePlannedSupportType,
			order.m_iOperationContractVersion,
			manifestHash,
			routeHash);
		if (manifestId != planning.m_sPlannedManifestId
			|| manifestHash != planning.m_sPlannedManifestHash
			|| capabilityHash != planning.m_sPlanningCapabilityHash)
			return RejectPreparedDecision(planning, "frozen enemy planning manifest or route capability fingerprint changed");
		order.m_sManifestId = manifestId;
		order.m_sManifestHash = manifestHash;
		order.m_sPlanningCapabilityHash = capabilityHash;
		if (manifest)
			order.m_iCompositionManpower = manifest.m_iAcceptedMemberCount;

		HST_EnemyPreparedAdmissionResult preflight = PreflightPreparedOrder(
			state,
			enemyDirector,
			order,
			manifest,
			route);
		if (!preflight.m_bAccepted)
			return RetryPreparedDecision(planning, preflight.m_sFailureReason, state.m_iElapsedSeconds);

		string spendReason;
		bool spent;
		if (planning.m_sSpendMode == SPEND_MODE_PROACTIVE_ATTACK)
			spent = enemyDirector.TrySpendProactiveAttack(
				state,
				planning.m_sFactionKey,
				planning.m_iAttackCost,
				spendReason,
				planning.m_sPlannedDebitMutationId,
				planning.m_sPlannedOrderId,
				planning.m_sPlannedOrderId,
				planning.m_sPlannedOperationId,
				planning.m_sPlannedManifestId,
				planning.m_sSelectedTargetZoneId);
		else if (planning.m_sSpendMode == SPEND_MODE_REACTIVE_DEFENSE)
			spent = enemyDirector.TrySpendDefense(
				state,
				targetZone,
				planning.m_sFactionKey,
				planning.m_iAttackCost,
				planning.m_iSupportCost,
				spendReason,
				planning.m_sPlannedDebitMutationId,
				planning.m_sPlannedOrderId,
				planning.m_sPlannedOrderId,
				planning.m_sPlannedOperationId,
				planning.m_sPlannedManifestId);
		else
			return RejectPreparedDecision(planning, "frozen enemy planning spend mode is unsupported");
		if (!spent)
			return RetryPreparedDecision(planning, spendReason, state.m_iElapsedSeconds);

		state.m_aEnemyOrders.Insert(order);
		bool admissionChanged = true;
		if (HST_OperationService.RequiresExactEnemyDefensiveQRF(order))
		{
			HST_EnemyQRFAdmissionResult admitted = m_ExactEnemyQRF.AdmitPreparedOrder(
				state,
				order,
				manifest,
				enemyDirector);
			admissionChanged = !admitted || admitted.m_bStateChanged || admissionChanged;
			if (!admitted || !admitted.m_bSuccess)
			{
				string admissionFailure = "exact enemy QRF admission failed without a durable result";
				if (admitted && !admitted.m_sFailureReason.IsEmpty())
					admissionFailure = admitted.m_sFailureReason;
				if (!admitted || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
					return QuarantinePreparedAdmissionConflict(
						state,
						planning,
						order,
						admissionFailure,
						admissionChanged || pressureStateChanged);
				return CompletePreparedWithOrder(
					state,
					preset,
					enemyDirector,
					planning,
					order,
					admissionChanged || pressureStateChanged);
			}
		}
		else if (HST_OperationService.RequiresExactEnemyCounterattack(order))
		{
			HST_EnemyCounterattackAdmissionResult admittedCounterattack = m_ExactEnemyCounterattack.AdmitPreparedOrder(
				state,
				order,
				manifest,
				enemyDirector);
			admissionChanged = !admittedCounterattack || admittedCounterattack.m_bStateChanged || admissionChanged;
			if (!admittedCounterattack || !admittedCounterattack.m_bSuccess)
			{
				string counterattackFailure = "exact enemy counterattack admission failed without a durable result";
				if (admittedCounterattack && !admittedCounterattack.m_sFailureReason.IsEmpty())
					counterattackFailure = admittedCounterattack.m_sFailureReason;
				if (!admittedCounterattack || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
					return QuarantinePreparedAdmissionConflict(
						state,
						planning,
						order,
						counterattackFailure,
						admissionChanged || pressureStateChanged);
				return CompletePreparedWithOrder(
					state,
					preset,
					enemyDirector,
					planning,
					order,
					admissionChanged || pressureStateChanged);
			}
		}
		else if (HST_OperationService.RequiresExactEnemyPatrol(order))
		{
			HST_EnemyPatrolAdmissionResult admittedPatrol = m_ExactEnemyPatrol.AdmitPreparedOrder(
				state,
				order,
				manifest,
				route,
				enemyDirector);
			admissionChanged = !admittedPatrol || admittedPatrol.m_bStateChanged || admissionChanged;
			if (!admittedPatrol || !admittedPatrol.m_bSuccess)
			{
				string patrolFailure = "exact enemy patrol admission failed without a durable result";
				if (admittedPatrol && !admittedPatrol.m_sFailureReason.IsEmpty())
					patrolFailure = admittedPatrol.m_sFailureReason;
				if (!admittedPatrol || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
					return QuarantinePreparedAdmissionConflict(
						state,
						planning,
						order,
						patrolFailure,
						admissionChanged || pressureStateChanged);
				return CompletePreparedWithOrder(
					state,
					preset,
					enemyDirector,
					planning,
					order,
					admissionChanged || pressureStateChanged);
			}
		}
		else if (HST_OperationService.RequiresExactEnemyGarrisonRebuild(order))
		{
			HST_EnemyGarrisonRebuildAdmissionResult admittedRebuild
				= m_ExactEnemyGarrisonRebuild.AdmitPreparedOrder(
					state,
					order,
					manifest,
					enemyDirector);
			admissionChanged = !admittedRebuild || admittedRebuild.m_bStateChanged || admissionChanged;
			if (!admittedRebuild || !admittedRebuild.m_bSuccess)
			{
				string rebuildFailure = "exact enemy garrison rebuild admission failed without a durable result";
				if (admittedRebuild && !admittedRebuild.m_sFailureReason.IsEmpty())
					rebuildFailure = admittedRebuild.m_sFailureReason;
				if (!admittedRebuild || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
					return QuarantinePreparedAdmissionConflict(
						state,
						planning,
						order,
						rebuildFailure,
						admissionChanged || pressureStateChanged);
				return CompletePreparedWithOrder(
					state,
					preset,
					enemyDirector,
					planning,
					order,
					admissionChanged || pressureStateChanged);
			}
		}
		else
		{
			order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
			if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
				state.m_iLastHQAttackSecond = state.m_iElapsedSeconds;
		}

		return CompletePreparedWithOrder(
			state,
			preset,
			enemyDirector,
			planning,
			order,
			admissionChanged || pressureStateChanged);
	}

	protected bool IsFrozenTargetRelationStructurallyValid(
		HST_CampaignPreset preset,
		string factionKey,
		HST_ZoneState targetZone,
		HST_EEnemyOrderType orderType)
	{
		if (!preset || !targetZone || factionKey.IsEmpty())
			return false;
		string relation = HST_FactionRelationService.ResolveRelation(
			preset,
			factionKey,
			targetZone.m_sOwnerFactionKey);
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF
			|| orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON
			|| orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK)
			return HST_FactionRelationService.IsSameFaction(relation);
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
			return HST_FactionRelationService.IsResistanceEnemy(relation);
		return true;
	}

	protected HST_EnemyPreparedAdmissionResult BuildPreparedAdmissionFailure(
		string failure,
		bool retryable,
		bool terminal,
		bool changed = false)
	{
		HST_EnemyPreparedAdmissionResult result = new HST_EnemyPreparedAdmissionResult();
		result.m_bRetryable = retryable;
		result.m_bTerminal = terminal;
		result.m_bStateChanged = changed;
		result.m_sFailureReason = failure;
		return result;
	}

	protected string BuildTargetCandidateFingerprint(
		HST_EnemyTargetScoreResult candidates,
		out int candidateCount)
	{
		candidateCount = 0;
		string canonical = HST_EnemyPlanningAuthorityService.EXACT_POLICY_ID + "|targets";
		if (candidates)
		{
			foreach (HST_EnemyTargetScoreCandidate candidate : candidates.m_aCandidates)
			{
				if (!candidate)
					continue;
				candidateCount++;
				canonical = canonical + string.Format(
					"|%1:%2:%3:%4:%5:%6:%7",
					candidate.m_sZoneId,
					candidate.m_iScore,
					candidate.m_iWeight,
					candidate.m_sOwnerRelation,
					candidate.m_eType,
					candidate.m_iCompatibleCommitmentCount,
					candidate.m_iCommitmentScore);
			}
		}
		canonical = canonical + string.Format("|count:%1", candidateCount);
		return string.Format(
			"ept2_%1_%2",
			canonical.Hash(),
			(canonical + "|secondary").Hash());
	}

	protected string BuildSourceCandidateFingerprint(
		HST_CampaignState state,
		HST_ZoneState targetZone,
		notnull array<string> sourceZoneIds)
	{
		string canonical = HST_EnemyPlanningAuthorityService.EXACT_POLICY_ID + "|sources";
		foreach (string sourceZoneId : sourceZoneIds)
		{
			HST_ZoneState sourceZone;
			if (state)
				sourceZone = state.FindZone(sourceZoneId);
			int distanceSq;
			if (sourceZone && targetZone)
				distanceSq = Math.Round(DistanceSq2D(sourceZone.m_vPosition, targetZone.m_vPosition));
			canonical = canonical + string.Format("|%1:%2", sourceZoneId, distanceSq);
		}
		canonical = canonical + string.Format("|count:%1", sourceZoneIds.Count());
		return string.Format(
			"eps1_%1_%2",
			canonical.Hash(),
			(canonical + "|secondary").Hash());
	}

	protected string BuildStableDecisionSalt(
		HST_CampaignState state,
		string factionKey,
		int decisionSequence,
		int decisionBucketSecond,
		string commitmentFingerprint,
		string targetCandidateFingerprint,
		string sourceCandidateFingerprint)
	{
		int campaignSeed;
		if (state)
			campaignSeed = state.m_iCampaignSeed;
		return string.Format(
			"%1|salt|%2|%3|%4|%5|%6|%7|%8",
			HST_EnemyPlanningAuthorityService.EXACT_POLICY_ID,
			campaignSeed,
			factionKey,
			decisionSequence,
			decisionBucketSecond,
			commitmentFingerprint,
			targetCandidateFingerprint,
			sourceCandidateFingerprint);
	}

	protected string BuildFrozenPlanningCapabilityHash(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_EEnemyOrderType orderType,
		HST_ESupportRequestType supportType,
		int operationContractVersion,
		string manifestHash,
		string routeHash)
	{
		string baseHash = HST_EnemyPlanningAuthorityService.BuildCapabilityHash(
			orderType,
			supportType,
			operationContractVersion,
			manifestHash,
			routeHash);
		if (!HST_OperationService.RequiresExactEnemyGarrisonRebuild(order))
			return baseHash;
		if (!state || !order)
			return "";
		HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
		HST_ZoneState sourceZone = state.FindZone(order.m_sSourceZoneId);
		if (!targetZone || !sourceZone || order.m_iTargetOwnershipRevision <= 0
			|| targetZone.m_iOwnershipRevision != order.m_iTargetOwnershipRevision)
			return "";
		string canonical = string.Format(
			"%1|rebuild_ownership|%2:%3:%4|%5:%6:%7",
			baseHash,
			targetZone.m_sZoneId,
			targetZone.m_sOwnerFactionKey,
			targetZone.m_iOwnershipRevision,
			sourceZone.m_sZoneId,
			sourceZone.m_sOwnerFactionKey,
			sourceZone.m_iOwnershipRevision);
		return string.Format(
			"epc70_%1_%2",
			canonical.Hash(),
			(canonical + "|secondary").Hash());
	}

	protected HST_EnemyOrderState BuildOrderFromFrozenPlanningValues(
		HST_CampaignState state,
		string factionKey,
		HST_ZoneState targetZone,
		HST_ZoneState sourceZone,
		HST_EEnemyOrderType orderType,
		string spendMode,
		int attackCost,
		int supportCost,
		HST_ESupportRequestType plannedSupportType,
		string orderId,
		string operationId,
		string debitMutationId,
		int decisionBucketSecond)
	{
		if (!state || !targetZone || !sourceZone || factionKey.IsEmpty()
			|| orderId.IsEmpty() || operationId.IsEmpty() || debitMutationId.IsEmpty())
			return null;
		HST_EnemyOrderState order = new HST_EnemyOrderState();
		order.m_sOrderId = orderId;
		order.m_sOperationId = operationId;
		order.m_sFactionKey = factionKey;
		order.m_eType = orderType;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED;
		order.m_sSourceZoneId = sourceZone.m_sZoneId;
		order.m_sTargetZoneId = targetZone.m_sZoneId;
		order.m_vSourcePosition = sourceZone.m_vPosition;
		order.m_vTargetPosition = targetZone.m_vPosition;
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
			order.m_vTargetPosition = ResolvePetrosAttackTargetPosition(state);
		order.m_iCreatedAtSecond = Math.Max(0, decisionBucketSecond);
		order.m_iResolveAtSecond = int.MAX;
		if (order.m_iCreatedAtSecond <= int.MAX - ORDER_RESOLVE_SECONDS)
			order.m_iResolveAtSecond = order.m_iCreatedAtSecond + ORDER_RESOLVE_SECONDS;
		order.m_iAttackCost = Math.Max(0, attackCost);
		order.m_iSupportCost = Math.Max(0, supportCost);
		order.m_ePlannedSupportType = plannedSupportType;
		order.m_sResourceDebitMutationId = debitMutationId;
		order.m_sRuntimeStatus = "active_" + spendMode + "_pending";
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF)
		{
			order.m_iOperationContractVersion = HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION;
			order.m_iResolveAtSecond = 0;
		}
		else if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
		{
			order.m_iOperationContractVersion = HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION;
			order.m_iResolveAtSecond = 0;
		}
		else if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL)
		{
			order.m_iOperationContractVersion = HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION;
			order.m_iResolveAtSecond = 0;
		}
		else if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON)
		{
			order.m_iOperationContractVersion = HST_EnemyGarrisonRebuildOperationService.EXACT_CONTRACT_VERSION;
			order.m_iTargetOwnershipRevision = Math.Max(1, targetZone.m_iOwnershipRevision);
			order.m_iResolveAtSecond = 0;
		}
		return order;
	}

	protected bool BuildFrozenPlanningCapability(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyPlanningState planning,
		HST_EnemyOrderState order,
		HST_ESupportRequestType plannedSupportType,
		out HST_ForceManifestState manifest,
		out HST_GeneratedRouteState route,
		out string routeHash,
		out string failure)
	{
		manifest = null;
		route = null;
		routeHash = "";
		failure = "";
		if (!state || !preset || !order)
		{
			failure = "frozen enemy planning capability context is missing";
			return false;
		}
		int planningWarLevel = state.m_iWarLevel;
		int plannedAtSecond = order.m_iCreatedAtSecond;
		if (planning && m_EnemyPlanningAuthority.IsPrepared(planning))
		{
			planningWarLevel = planning.m_iObservedWarLevel;
			plannedAtSecond = planning.m_iDecisionBucketSecond;
		}
		if (HST_OperationService.RequiresExactEnemyDefensiveQRF(order))
		{
			if (!m_ForcePlanning || !m_ExactEnemyQRF)
			{
				failure = "exact enemy QRF planning services are unavailable";
				return false;
			}
			HST_EnemyDefensiveQRFManifestResult plan = m_ForcePlanning.PlanExactEnemyDefensiveQRF(
				state,
				preset,
				order,
				false,
				planningWarLevel,
				plannedAtSecond);
			if (!plan || !plan.m_bSuccess || !plan.m_Manifest)
			{
				failure = "exact enemy QRF manifest planning failed";
				if (plan && !plan.m_sFailureReason.IsEmpty())
					failure = plan.m_sFailureReason;
				return false;
			}
			manifest = plan.m_Manifest;
			return true;
		}
		if (HST_OperationService.RequiresExactEnemyCounterattack(order))
		{
			if (!m_ForcePlanning || !m_ExactEnemyCounterattack)
			{
				failure = "exact enemy counterattack planning services are unavailable";
				return false;
			}
			HST_EnemyCounterattackManifestResult counterattackPlan = m_ForcePlanning.PlanExactEnemyCounterattack(
				state,
				preset,
				order,
				false,
				planningWarLevel,
				plannedAtSecond);
			if (!counterattackPlan || !counterattackPlan.m_bSuccess || !counterattackPlan.m_Manifest)
			{
				failure = "exact enemy counterattack manifest planning failed";
				if (counterattackPlan && !counterattackPlan.m_sFailureReason.IsEmpty())
					failure = counterattackPlan.m_sFailureReason;
				return false;
			}
			manifest = counterattackPlan.m_Manifest;
			return true;
		}
		if (HST_OperationService.RequiresExactEnemyPatrol(order))
		{
			if (!m_ForcePlanning || !m_ExactEnemyPatrol)
			{
				failure = "exact enemy patrol planning services are unavailable";
				return false;
			}
			HST_EnemyPatrolManifestResult patrolPlan = m_ForcePlanning.PlanExactEnemyPatrol(
				state,
				preset,
				order,
				false,
				planningWarLevel,
				plannedAtSecond);
			if (!patrolPlan || !patrolPlan.m_bSuccess || !patrolPlan.m_Manifest)
			{
				failure = "exact enemy patrol manifest planning failed";
				if (patrolPlan && !patrolPlan.m_sFailureReason.IsEmpty())
					failure = patrolPlan.m_sFailureReason;
				return false;
			}
			manifest = patrolPlan.m_Manifest;
			order.m_sManifestId = manifest.m_sManifestId;
			order.m_sManifestHash = manifest.m_sManifestHash;
			route = m_ExactEnemyPatrol.ResolvePatrolRoute(state, order);
			if (!route)
			{
				failure = "exact enemy patrol route is unavailable";
				return false;
			}
			ref array<vector> positions = HST_OperationRouteCursorService.BuildOrderedRoutePositions(route);
			routeHash = HST_OperationRouteCursorService.BuildRouteContractHash(route, positions);
			if (positions.Count() < 2 || routeHash.IsEmpty())
			{
				failure = "exact enemy patrol route capability hash is unavailable";
				return false;
			}
		}
		else if (HST_OperationService.RequiresExactEnemyGarrisonRebuild(order))
		{
			if (!m_ForcePlanning || !m_ExactEnemyGarrisonRebuild)
			{
				failure = "exact enemy garrison rebuild planning services are unavailable";
				return false;
			}
			HST_EnemyGarrisonRebuildManifestResult rebuildPlan
				= m_ForcePlanning.PlanExactEnemyGarrisonRebuild(
					state,
					preset,
					order,
					false,
					planningWarLevel,
					plannedAtSecond);
			if (!rebuildPlan || !rebuildPlan.m_bSuccess || !rebuildPlan.m_Manifest)
			{
				failure = "exact enemy garrison rebuild manifest planning failed";
				if (rebuildPlan && !rebuildPlan.m_sFailureReason.IsEmpty())
					failure = rebuildPlan.m_sFailureReason;
				return false;
			}
			manifest = rebuildPlan.m_Manifest;
		}
		return true;
	}

	protected void BuildTargetPressureProjection(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		string factionKey,
		HST_ZoneState targetZone,
		out int pressureBefore,
		out int pressureDelta,
		out int pressureAfter)
	{
		pressureBefore = 0;
		pressureDelta = 0;
		pressureAfter = 0;
		if (!state || !preset || !enemyDirector || !targetZone || factionKey.IsEmpty())
			return;
		pressureBefore = Math.Max(0, enemyDirector.GetRecentDamageScore(
			state,
			factionKey,
			targetZone.m_sZoneId));
		string ownerRelation = HST_FactionRelationService.ResolveRelation(
			preset,
			factionKey,
			targetZone.m_sOwnerFactionKey);
		int projectedSignal;
		if (HST_FactionRelationService.IsSameFaction(ownerRelation))
		{
			projectedSignal = Math.Max(0, targetZone.m_iResistanceCaptureProgress / 5);
			if (HasVerifiedHostilePresenceAtZone(state, preset, factionKey, targetZone))
				projectedSignal += 4;
			if (HasActiveMissionNearZone(state, targetZone))
				projectedSignal += 3;
			if (HasActiveObjectiveNearZone(state, targetZone))
				projectedSignal += 3;
			if (IsTownSupportFlipThreat(state, preset, factionKey, targetZone))
				projectedSignal += Math.Max(1, ResolveZoneSupportPercent(state, targetZone) / 10);
		}
		pressureAfter = Math.Min(100, pressureBefore + Math.Max(0, projectedSignal));
		pressureDelta = pressureAfter - pressureBefore;
	}

	protected void CopyPlanningBacklinks(
		HST_EnemyOrderState order,
		HST_EnemyPlanningState planning)
	{
		if (!order || !planning)
			return;
		order.m_iPlanningContractVersion = HST_EnemyPlanningAuthorityService.CONTRACT_VERSION;
		order.m_iPlanningDecisionSequence = planning.m_iDecisionSequence;
		order.m_iPlanningBucketSecond = planning.m_iDecisionBucketSecond;
		order.m_sPlanningDecisionId = planning.m_sDecisionId;
		order.m_sPlanningInputFingerprint = planning.m_sInputFingerprint;
		order.m_sPlanningDecisionFingerprint = planning.m_sDecisionFingerprint;
		order.m_ePlannedSupportType = planning.m_ePlannedSupportType;
		order.m_sPlanningCapabilityHash = planning.m_sPlanningCapabilityHash;
		order.m_sResourceDebitMutationId = planning.m_sPlannedDebitMutationId;
	}

	protected HST_EnemyPreparedAdmissionResult PreflightPreparedOrder(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_ForceManifestState manifest,
		HST_GeneratedRouteState route)
	{
		HST_EnemyPreparedAdmissionResult result = new HST_EnemyPreparedAdmissionResult();
		if (!state || !enemyDirector || !order)
			return BuildPreparedAdmissionFailure("prepared enemy order preflight context is missing", true, false);
		if (HST_OperationService.RequiresExactEnemyDefensiveQRF(order))
		{
			if (!m_ExactEnemyQRF || !manifest)
				return BuildPreparedAdmissionFailure("exact enemy QRF admission service is unavailable", true, false);
			HST_EnemyQRFAdmissionResult qrf = m_ExactEnemyQRF.CanAdmitPreparedOrder(
				state,
				order,
				manifest,
				enemyDirector);
			if (!qrf || !qrf.m_bSuccess)
			{
				string failure = "exact enemy QRF admission preflight failed";
				if (qrf && !qrf.m_sFailureReason.IsEmpty())
					failure = qrf.m_sFailureReason;
				return BuildPreparedAdmissionFailure(failure, true, false);
			}
		}
		else if (HST_OperationService.RequiresExactEnemyCounterattack(order))
		{
			if (!m_ExactEnemyCounterattack || !manifest)
				return BuildPreparedAdmissionFailure("exact enemy counterattack admission service is unavailable", true, false);
			HST_EnemyCounterattackAdmissionResult counterattack = m_ExactEnemyCounterattack.CanAdmitPreparedOrder(
				state,
				order,
				manifest,
				enemyDirector);
			if (!counterattack || !counterattack.m_bSuccess)
			{
				string counterattackFailure = "exact enemy counterattack admission preflight failed";
				if (counterattack && !counterattack.m_sFailureReason.IsEmpty())
					counterattackFailure = counterattack.m_sFailureReason;
				return BuildPreparedAdmissionFailure(counterattackFailure, true, false);
			}
		}
		else if (HST_OperationService.RequiresExactEnemyPatrol(order))
		{
			if (!m_ExactEnemyPatrol || !manifest || !route)
				return BuildPreparedAdmissionFailure("exact enemy patrol admission service is unavailable", true, false);
			HST_EnemyPatrolAdmissionResult patrol = m_ExactEnemyPatrol.CanAdmitPreparedOrder(
				state,
				order,
				manifest,
				route,
				enemyDirector);
			if (!patrol || !patrol.m_bSuccess)
			{
				string failure = "exact enemy patrol admission preflight failed";
				if (patrol && !patrol.m_sFailureReason.IsEmpty())
					failure = patrol.m_sFailureReason;
				return BuildPreparedAdmissionFailure(failure, true, false);
			}
		}
		else if (HST_OperationService.RequiresExactEnemyGarrisonRebuild(order))
		{
			if (!m_ExactEnemyGarrisonRebuild || !manifest)
				return BuildPreparedAdmissionFailure("exact enemy garrison rebuild admission service is unavailable", true, false);
			HST_EnemyGarrisonRebuildAdmissionResult rebuild
				= m_ExactEnemyGarrisonRebuild.CanAdmitPreparedOrder(
					state,
					order,
					manifest,
					enemyDirector);
			if (!rebuild || !rebuild.m_bSuccess)
			{
				string rebuildFailure = "exact enemy garrison rebuild admission preflight failed";
				if (rebuild && !rebuild.m_sFailureReason.IsEmpty())
					rebuildFailure = rebuild.m_sFailureReason;
				return BuildPreparedAdmissionFailure(rebuildFailure, true, false);
			}
		}
		result.m_bAccepted = true;
		return result;
	}

	protected HST_EnemyPreparedAdmissionResult RetryPreparedDecision(
		HST_EnemyPlanningState planning,
		string failure,
		int nowSecond)
	{
		if (failure.IsEmpty())
			failure = "prepared enemy planning admission failed transiently";
		HST_EnemyPlanningDecisionResult retry = m_EnemyPlanningAuthority.RecordRetry(
			planning,
			failure,
			nowSecond);
		HST_EnemyPreparedAdmissionResult result = BuildPreparedAdmissionFailure(
			failure,
			true,
			false,
			retry && retry.m_bChanged);
		result.m_bAccepted = retry && retry.m_bAccepted;
		return result;
	}

	protected HST_EnemyPreparedAdmissionResult RetryUnpreparedDecision(
		HST_EnemyPlanningState planning,
		string failure,
		int nowSecond)
	{
		if (failure.IsEmpty())
			failure = "enemy planning preparation failed transiently";
		HST_EnemyPlanningDecisionResult retry = m_EnemyPlanningAuthority.RecordPreparationRetry(
			planning,
			failure,
			nowSecond);
		HST_EnemyPreparedAdmissionResult result = BuildPreparedAdmissionFailure(
			failure,
			true,
			false,
			retry && retry.m_bChanged);
		result.m_bAccepted = retry && retry.m_bAccepted;
		return result;
	}

	protected HST_EnemyPreparedAdmissionResult RejectPreparedDecision(
		HST_EnemyPlanningState planning,
		string failure)
	{
		return CompletePreparedWithoutOrder(planning, "rejected", failure, false);
	}

	protected HST_EnemyPreparedAdmissionResult CompletePreparedWithoutOrder(
		HST_EnemyPlanningState planning,
		string disposition,
		string failure,
		bool priorChanged)
	{
		HST_EnemyPlanningDecisionResult completed = m_EnemyPlanningAuthority.CompleteDecision(
			planning,
			disposition,
			null,
			failure);
		HST_EnemyPreparedAdmissionResult result = BuildPreparedAdmissionFailure(
			failure,
			false,
			true,
			priorChanged || (completed && completed.m_bChanged));
		result.m_bAccepted = completed && completed.m_bAccepted;
		return result;
	}

	protected HST_EnemyPreparedAdmissionResult ApplyFrozenTargetPressure(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyPlanningState planning)
	{
		HST_EnemyPreparedAdmissionResult result = new HST_EnemyPreparedAdmissionResult();
		if (!state || !enemyDirector || !planning || !m_EnemyPlanningAuthority
			|| !m_EnemyPlanningAuthority.IsPrepared(planning))
			return BuildPreparedAdmissionFailure("frozen enemy planning target-pressure context is missing", false, true);
		if (planning.m_bTargetPressureApplied)
		{
			result.m_bAccepted = true;
			return result;
		}
		string markFailure;
		if (!m_EnemyPlanningAuthority.CanMarkTargetPressureApplied(
			planning,
			markFailure))
			return BuildPreparedAdmissionFailure(markFailure, false, true);
		HST_ZoneState targetZone = state.FindZone(planning.m_sSelectedTargetZoneId);
		if (!targetZone)
			return BuildPreparedAdmissionFailure("frozen enemy planning target is unavailable for pressure", false, true);
		int currentPressure = enemyDirector.GetRecentDamageScore(
			state,
			planning.m_sFactionKey,
			planning.m_sSelectedTargetZoneId);
		if (currentPressure != planning.m_iTargetPressureBefore)
			return BuildPreparedAdmissionFailure("frozen enemy planning target-pressure snapshot changed before application", false, true);
		if (planning.m_iTargetPressureDelta > 0)
			enemyDirector.RecordZoneDamageSignal(
				state,
				planning.m_sFactionKey,
				targetZone,
				planning.m_iTargetPressureDelta,
				"frozen periodic target pressure signal");
		currentPressure = enemyDirector.GetRecentDamageScore(
			state,
			planning.m_sFactionKey,
			planning.m_sSelectedTargetZoneId);
		if (currentPressure != planning.m_iTargetPressureAfter)
			return BuildPreparedAdmissionFailure("frozen enemy planning target-pressure projection did not apply exactly", false, true, planning.m_iTargetPressureDelta > 0);
		HST_EnemyPlanningDecisionResult marked = m_EnemyPlanningAuthority.MarkTargetPressureApplied(planning);
		if (!marked || !marked.m_bAccepted)
		{
			string failure = "enemy planning target-pressure authority could not be marked";
			if (marked && !marked.m_sFailureReason.IsEmpty())
				failure = marked.m_sFailureReason;
			return BuildPreparedAdmissionFailure(failure, false, true, planning.m_iTargetPressureDelta > 0 || (marked && marked.m_bChanged));
		}
		result.m_bAccepted = true;
		result.m_bStateChanged = planning.m_iTargetPressureDelta > 0 || marked.m_bChanged;
		return result;
	}

	protected HST_EnemyPreparedAdmissionResult CompletePreparedWithOrder(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyPlanningState planning,
		HST_EnemyOrderState order,
		bool priorChanged)
	{
		if (!state || !preset || !enemyDirector || !planning || !order)
			return BuildPreparedAdmissionFailure("enemy planning committed order context is missing", false, true, priorChanged);
		CopyPlanningBacklinks(order, planning);
		HST_EnemyPlanningDecisionResult completed = m_EnemyPlanningAuthority.CompleteDecision(
			planning,
			"committed",
			order,
			"");
		HST_EnemyPreparedAdmissionResult result = new HST_EnemyPreparedAdmissionResult();
		result.m_bAccepted = completed && completed.m_bAccepted;
		result.m_bTerminal = true;
		result.m_bStateChanged = priorChanged || (completed && completed.m_bChanged);
		result.m_Order = order;
		if (!result.m_bAccepted)
		{
			result.m_sFailureReason = "enemy planning completion failed after durable order admission";
			if (completed && !completed.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = completed.m_sFailureReason;
		}
		return result;
	}

	protected HST_EnemyPreparedAdmissionResult QuarantinePreparedAdmissionConflict(
		HST_CampaignState state,
		HST_EnemyPlanningState planning,
		HST_EnemyOrderState order,
		string failure,
		bool priorChanged)
	{
		if (failure.IsEmpty())
			failure = "prepared enemy planning admission ended without durable authority";
		bool planningChanged;
		if (m_EnemyPlanningAuthority && planning)
			planningChanged = m_EnemyPlanningAuthority.Quarantine(planning, failure);
		bool orderChanged;
		if (order)
		{
			order.m_iPlanningContractVersion
				= HST_EnemyPlanningAuthorityService.QUARANTINE_CONTRACT_VERSION;
			if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED)
			{
				order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
				order.m_iResolvedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
			}
			order.m_sRuntimeStatus = "planning_admission_conflict";
			order.m_sFailureReason = failure;
			orderChanged = true;
		}
		HST_EnemyPreparedAdmissionResult result = BuildPreparedAdmissionFailure(
			failure,
			false,
			true,
			priorChanged || planningChanged || orderChanged);
		return result;
	}

	string BuildEnemyOrderReport(HST_CampaignState state)
	{
		if (!state)
			return "Partisan enemy commander | state not ready";

		int queued;
		int active;
		int resolved;
		int aborted;
		int physicalized;
		int abstractResolved;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order)
				continue;

			if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED)
				queued++;
			else if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				active++;
			else if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED)
				resolved++;
			else if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
				aborted++;

			if (order.m_bPhysicalized)
				physicalized++;
			if (order.m_bAbstractResolved)
				abstractResolved++;
		}

		string report = string.Format(
			"Partisan enemy commander | queued %1 | active %2 | resolved %3 | aborted %4 | physicalized %5 | abstract %6",
			queued,
			active,
			resolved,
			aborted,
			physicalized,
			abstractResolved
		);

		int emitted;
		for (int i = state.m_aEnemyOrders.Count() - 1; i >= 0; i--)
		{
			HST_EnemyOrderState orderDetail = state.m_aEnemyOrders[i];
			if (!orderDetail)
				continue;

			string targetText = orderDetail.m_sTargetZoneId;
			if (orderDetail.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
				targetText = "resistance_base near " + orderDetail.m_sTargetZoneId;
			string detail = string.Format(
				"\n%1 | %2 | %3 | faction %4 | target %5 | runtime %6 | support %7 | group %8 | resolve %9",
				orderDetail.m_sOrderId,
				orderDetail.m_eType,
				orderDetail.m_eStatus,
				orderDetail.m_sFactionKey,
				targetText,
				orderDetail.m_sRuntimeStatus,
				orderDetail.m_sSupportRequestId,
				orderDetail.m_sGroupId,
				orderDetail.m_iResolveAtSecond
			);
			detail = detail + string.Format(
				" | result %1 | fail %2 | cost %3/%4",
				orderDetail.m_sResolutionKind,
				orderDetail.m_sFailureReason,
				orderDetail.m_iAttackCost,
				orderDetail.m_iSupportCost
			);
			if (!orderDetail.m_sCompositionIntentId.IsEmpty() || orderDetail.m_iCompositionManpower > 0 || orderDetail.m_iCompositionVehicleCount > 0)
				detail = detail + string.Format(" | composition %1 tier %2 compCost %3 manpower %4 vehicles %5 armed %6", orderDetail.m_sCompositionIntentId, orderDetail.m_sCompositionTier, orderDetail.m_iCompositionCost, orderDetail.m_iCompositionManpower, orderDetail.m_iCompositionVehicleCount, orderDetail.m_iCompositionArmedVehicleCount);
			if (!orderDetail.m_sCompositionFailureReason.IsEmpty())
				detail = detail + " | composition failure " + orderDetail.m_sCompositionFailureReason;
			report = report + detail;

			emitted++;
			if (emitted >= 12)
				break;
		}

		return report;
	}

	string BuildPhysicalResponseReport(HST_CampaignState state)
	{
		if (!state)
			return "Partisan enemy physical response | state not ready";

		int linkedSupport;
		int linkedGroups;
		int activePhysicalGroups;
		int abstractPending;
		int physicalPending;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order)
				continue;

			if (!order.m_sSupportRequestId.IsEmpty())
				linkedSupport++;
			if (!order.m_sGroupId.IsEmpty())
				linkedGroups++;
			if (order.m_bPhysicalized && order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				physicalPending++;
			if (!order.m_bPhysicalized && order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				abstractPending++;
		}

		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (!group)
				continue;

			if (group.m_sRuntimeStatus == "support_active" || group.m_sRuntimeStatus == "support_arrived")
				activePhysicalGroups++;
		}

		return string.Format(
			"Partisan enemy physical response | support links %1 | group links %2 | support-active groups %3 | abstract pending %4 | physical pending %5",
			linkedSupport,
			linkedGroups,
			activePhysicalGroups,
			abstractPending,
			physicalPending
		);
	}

	HST_EnemyTargetScoreResult BuildTargetScoreResult(HST_CampaignState state, HST_CampaignPreset preset, string factionKey, bool forceBest, string stableDecisionSalt = "", array<string> excludedZoneIds = null)
	{
		HST_EnemyTargetScoreResult result = new HST_EnemyTargetScoreResult();
		result.m_sFactionKey = factionKey;
		result.m_iBestScore = -9999;
		result.m_sSelectionMode = "weighted_top_band";

		if (!state || !preset || factionKey.IsEmpty())
		{
			result.m_sFailureReason = "state, preset, or faction missing";
			return result;
		}

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;
			if (excludedZoneIds && excludedZoneIds.Contains(zone.m_sZoneId))
				continue;

			result.m_iCandidateCount++;
			string ineligibleReason;
			if (!IsEligibleTargetZone(zone, ineligibleReason))
				continue;

			string localityReason;
			if (!IsLocalOperationTargetAllowed(state, preset, factionKey, zone, localityReason))
			{
				result.m_iLocalityRejectedCount++;
				if (result.m_sLocalityRejectedReason.IsEmpty())
					result.m_sLocalityRejectedReason = string.Format("%1: %2", zone.m_sZoneId, localityReason);
				continue;
			}

			int compatibleCommitmentCount;
			string compatibleCommitmentReason;
			string blockingCommitmentReason;
			if (ResolveTargetCommitmentCompatibility(
				state,
				factionKey,
				zone.m_sZoneId,
				compatibleCommitmentCount,
				compatibleCommitmentReason,
				blockingCommitmentReason))
			{
				result.m_iCommitmentRejectedCount++;
				string commitmentRejectedReason = string.Format(
					"%1: %2",
					zone.m_sZoneId,
					blockingCommitmentReason);
				if (result.m_sCommitmentRejectedReason.IsEmpty()
					|| commitmentRejectedReason.Compare(
						result.m_sCommitmentRejectedReason) < 0)
					result.m_sCommitmentRejectedReason
						= commitmentRejectedReason;
				continue;
			}

			HST_EnemyTargetScoreCandidate candidate = BuildTargetScoreCandidateResolved(
				state,
				preset,
				zone,
				factionKey,
				compatibleCommitmentCount,
				compatibleCommitmentReason,
				localityReason);
			if (!candidate)
				continue;

			result.m_aCandidates.Insert(candidate);
			result.m_iEligibleCount++;
		}
		SortTargetCandidatesByZoneId(result.m_aCandidates);
		HST_EnemyTargetScoreCandidate bestCandidate;
		foreach (HST_EnemyTargetScoreCandidate rankedCandidate : result.m_aCandidates)
		{
			if (!rankedCandidate)
				continue;
			if (!bestCandidate || rankedCandidate.m_iScore > result.m_iBestScore
				|| (rankedCandidate.m_iScore == result.m_iBestScore
					&& rankedCandidate.m_sZoneId.Compare(bestCandidate.m_sZoneId) < 0))
			{
				bestCandidate = rankedCandidate;
				result.m_iBestScore = rankedCandidate.m_iScore;
				result.m_sBestZoneId = rankedCandidate.m_sZoneId;
			}
		}

		if (!bestCandidate)
		{
			result.m_sFailureReason = "no eligible target zones";
			return result;
		}

		int topBandFloor = result.m_iBestScore - 12;
		foreach (HST_EnemyTargetScoreCandidate weightedCandidate : result.m_aCandidates)
		{
			if (!weightedCandidate || weightedCandidate.m_iScore < topBandFloor)
				continue;

			weightedCandidate.m_iWeight = Math.Max(1, weightedCandidate.m_iScore - topBandFloor + 1);
			result.m_iTotalWeight += weightedCandidate.m_iWeight;
		}

		HST_EnemyTargetScoreCandidate selectedCandidate = bestCandidate;
		if (forceBest)
		{
			result.m_sSelectionMode = "forced_best";
		}
		else if (result.m_iTotalWeight > 0)
		{
			int rollSeed;
			if (!stableDecisionSalt.IsEmpty())
				rollSeed = stableDecisionSalt.Hash();
			else
				rollSeed = state.m_iCampaignSeed + state.m_iElapsedSeconds * 13 + factionKey.Length() * 97 + state.m_aEnemyOrders.Count() * 43 + result.m_iEligibleCount * 17 + result.m_iBestScore * 31;
			result.m_iRoll = HST_DefaultCatalog.PositiveMod(rollSeed, result.m_iTotalWeight);
			int cumulative;
			foreach (HST_EnemyTargetScoreCandidate rolledCandidate : result.m_aCandidates)
			{
				if (!rolledCandidate || rolledCandidate.m_iWeight <= 0)
					continue;

				cumulative += rolledCandidate.m_iWeight;
				if (result.m_iRoll < cumulative)
				{
					selectedCandidate = rolledCandidate;
					break;
				}
			}
		}

		result.m_bSuccess = selectedCandidate != null;
		if (selectedCandidate)
		{
			result.m_sSelectedZoneId = selectedCandidate.m_sZoneId;
			result.m_iSelectedScore = selectedCandidate.m_iScore;
			result.m_sReason = selectedCandidate.m_sReason;
		}

		return result;
	}

	protected void SortTargetCandidatesByZoneId(notnull array<ref HST_EnemyTargetScoreCandidate> candidates)
	{
		for (int candidateIndex = 1; candidateIndex < candidates.Count(); candidateIndex++)
		{
			HST_EnemyTargetScoreCandidate selected = candidates[candidateIndex];
			int insertionIndex = candidateIndex - 1;
			while (insertionIndex >= 0 && selected && candidates[insertionIndex]
				&& selected.m_sZoneId.Compare(candidates[insertionIndex].m_sZoneId) < 0)
			{
				candidates[insertionIndex + 1] = candidates[insertionIndex];
				insertionIndex--;
			}
			candidates[insertionIndex + 1] = selected;
		}
	}

	string BuildEnemyTargetScoreReport(HST_CampaignState state, HST_CampaignPreset preset, string factionKey)
	{
		HST_EnemyTargetScoreResult result = BuildTargetScoreResult(state, preset, factionKey, false);
		if (!result)
			return string.Format("Partisan enemy target scoring | failed | faction %1 | reason scorer unavailable", ReportText(factionKey));
		if (!result.m_bSuccess)
		{
			string failureReport = string.Format(
				"Partisan enemy target scoring | failed | faction %1 | reason %2",
				ReportText(factionKey),
				ReportText(result.m_sFailureReason)
			);
			return failureReport + string.Format(
				" | local rejects %1 | first %2 | commitment rejects %3 | first %4",
				result.m_iLocalityRejectedCount,
				ReportText(result.m_sLocalityRejectedReason),
				result.m_iCommitmentRejectedCount,
				ReportText(result.m_sCommitmentRejectedReason)
			);
		}

		string report = string.Format(
			"Partisan enemy target scoring | faction %1 | selected %2 score %3 | best %4 score %5 | eligible %6/%7 | local rejects %8",
			ReportText(factionKey),
			ReportText(result.m_sSelectedZoneId),
			result.m_iSelectedScore,
			ReportText(result.m_sBestZoneId),
			result.m_iBestScore,
			result.m_iEligibleCount,
			result.m_iCandidateCount,
			result.m_iLocalityRejectedCount
		);
		report = report + string.Format(
			" | commitment rejects %1 | mode %2",
			result.m_iCommitmentRejectedCount,
			ReportText(result.m_sSelectionMode)
		);
		report = report + string.Format(" | weight %1 roll %2 | reason %3", result.m_iTotalWeight, result.m_iRoll, ReportText(result.m_sReason));
		if (result.m_iLocalityRejectedCount > 0)
			report = report + string.Format(" | first local reject %1", ReportText(result.m_sLocalityRejectedReason));
		if (result.m_iCommitmentRejectedCount > 0)
			report = report + string.Format(
				" | first commitment reject %1",
				ReportText(result.m_sCommitmentRejectedReason));

		int emitted;
		int topBandFloor = result.m_iBestScore - 12;
		foreach (HST_EnemyTargetScoreCandidate candidate : result.m_aCandidates)
		{
			if (!candidate || candidate.m_iScore < topBandFloor)
				continue;

			report = report + string.Format("\n%1 | score %2 | weight %3 | owner %4 | relation %5", ReportText(candidate.m_sZoneId), candidate.m_iScore, candidate.m_iWeight, ReportText(candidate.m_sOwnerFactionKey), ReportText(candidate.m_sOwnerRelation));
			report = report + string.Format(
				" | type %1 | local %2 | commitments %3 score %4 (%5) | reason %6",
				candidate.m_eType,
				ReportText(candidate.m_sLocalityReason),
				candidate.m_iCompatibleCommitmentCount,
				candidate.m_iCommitmentScore,
				ReportText(candidate.m_sCommitmentReason),
				ReportText(candidate.m_sReason));
			emitted++;
			if (emitted >= 8)
				break;
		}

		return report;
	}

	HST_EEnemyOrderType ResolveOrderTypeForDebug(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState targetZone, HST_FactionPoolState pool)
	{
		return SelectOrderType(state, preset, targetZone, pool);
	}

	bool IsLocalOperationTargetAllowed(HST_CampaignState state, HST_CampaignPreset preset, string factionKey, HST_ZoneState targetZone, out string reason)
	{
		reason = "";
		if (!state)
		{
			reason = "missing locality context";
			return false;
		}
		if (!preset)
		{
			reason = "missing locality context";
			return false;
		}
		if (factionKey.IsEmpty())
		{
			reason = "missing locality context";
			return false;
		}
		if (!targetZone)
		{
			reason = "missing locality context";
			return false;
		}

		string ownerRelation = HST_FactionRelationService.ResolveRelation(preset, factionKey, targetZone.m_sOwnerFactionKey);
		if (HST_FactionRelationService.IsSameFaction(ownerRelation))
		{
			reason = "own holding";
			return true;
		}

		if (HST_FactionRelationService.IsResistanceEnemy(ownerRelation))
		{
			reason = "resistance-held exception";
			return true;
		}

		float nearestDistanceSq;
		HST_ZoneState nearestFoothold = FindNearestLocalOperationFoothold(state, factionKey, targetZone, nearestDistanceSq);
		if (!nearestFoothold)
		{
			reason = "no local faction foothold";
			return false;
		}

		if (AreOperationalZonesLinked(nearestFoothold, targetZone))
		{
			reason = "linked foothold " + nearestFoothold.m_sZoneId;
			return true;
		}

		float frontRadiusSq = LOCAL_OPERATION_FRONT_RADIUS_METERS * LOCAL_OPERATION_FRONT_RADIUS_METERS;
		if (nearestDistanceSq <= frontRadiusSq)
		{
			reason = string.Format("local foothold %1 %2m", nearestFoothold.m_sZoneId, Math.Round(Math.Sqrt(nearestDistanceSq)));
			return true;
		}

		reason = string.Format("disconnected target; nearest foothold %1 is %2m away", nearestFoothold.m_sZoneId, Math.Round(Math.Sqrt(nearestDistanceSq)));
		return false;
	}

	bool TryQueueImmediateCounterattack(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, HST_SupportRequestService support, string factionKey, HST_ZoneState capturedZone, int chancePercent)
	{
		if (!state || !preset || !enemyDirector || !capturedZone || factionKey.IsEmpty())
			return false;

		if (HasActiveOrderForZone(state, factionKey, capturedZone.m_sZoneId, true))
		{
			Print(string.Format("Partisan capture | counterattack skipped for %1 at %2 | active order already exists", factionKey, capturedZone.m_sZoneId));
			return false;
		}

		int attackCost;
		int supportCost;
		ResolveOrderCostsForSpendMode(HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK, SPEND_MODE_REACTIVE_DEFENSE, attackCost, supportCost);
		string spendReason;
		if (!enemyDirector.CanSpendDefense(state, capturedZone, factionKey, attackCost, supportCost, spendReason))
		{
			Print(string.Format("Partisan capture | counterattack skipped for %1 at %2 | %3", factionKey, capturedZone.m_sZoneId, spendReason));
			return false;
		}

		int chance = Math.Max(0, Math.Min(100, chancePercent));
		int rollSeed = state.m_iCampaignSeed + state.m_iElapsedSeconds * 17 + capturedZone.m_sZoneId.Length() * 101 + capturedZone.m_sDisplayName.Length() * 29 + factionKey.Length() * 31 + state.m_aEnemyOrders.Count() * 13 + state.m_iWarLevel * 19 + capturedZone.m_iPriority * 23;
		rollSeed += Math.Round(capturedZone.m_vPosition[0]) + Math.Round(capturedZone.m_vPosition[2]);
		int roll = HST_DefaultCatalog.PositiveMod(rollSeed, 100);
		if (roll >= chance)
		{
			Print(string.Format("Partisan capture | counterattack skipped for %1 at %2 | roll %3 chance %4", factionKey, capturedZone.m_sZoneId, roll, chance));
			return false;
		}

		bool queued = QueueOrder(state, preset, enemyDirector, support, factionKey, capturedZone, HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK, SPEND_MODE_REACTIVE_DEFENSE);
		if (queued)
			Print(string.Format("Partisan capture | counterattack queued for %1 at %2 | roll %3 chance %4", factionKey, capturedZone.m_sZoneId, roll, chance));
		else
			Print(string.Format("Partisan capture | counterattack failed for %1 at %2 after chance pass", factionKey, capturedZone.m_sZoneId));

		return queued;
	}

	HST_EnemyOrderState QueueDebugOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, string factionKey, HST_ZoneState targetZone, HST_EEnemyOrderType orderType, string spendMode = "", bool addDebugResources = true)
	{
		if (!state || !preset || !enemyDirector || !targetZone || factionKey.IsEmpty())
			return null;

		if (!state.FindFactionPool(factionKey))
			return null;

		if (addDebugResources)
			enemyDirector.AddResources(state, factionKey, 100, 100);
		int beforeCount = state.m_aEnemyOrders.Count();
		if (!QueueOrder(state, preset, enemyDirector, null, factionKey, targetZone, orderType, spendMode))
			return null;

		if (state.m_aEnemyOrders.Count() <= beforeCount)
			return null;

		HST_EnemyOrderState queuedOrder = state.m_aEnemyOrders[state.m_aEnemyOrders.Count() - 1];
		if (!queuedOrder || queuedOrder.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return null;

		return queuedOrder;
	}

	// Synthetic debug fixtures may still exercise historical contract-zero QRF,
	// patrol, or garrison-rebuild paths.
	// Production versioned admission never calls this path or falls back to legacy behavior.
	HST_EnemyOrderState QueueDebugLegacyOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, string factionKey, HST_ZoneState targetZone, HST_EEnemyOrderType orderType, string spendMode = "")
	{
		if (!state || !preset || !enemyDirector || !targetZone || factionKey.IsEmpty())
			return null;
		if ((orderType != HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF
			&& orderType != HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL
			&& orderType != HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON)
			|| !state.FindFactionPool(factionKey))
			return null;

		enemyDirector.AddResources(state, factionKey, 100, 100);
		int beforeCount = state.m_aEnemyOrders.Count();
		if (!QueueOrder(state, preset, enemyDirector, null, factionKey, targetZone, orderType, spendMode, true))
			return null;
		if (state.m_aEnemyOrders.Count() <= beforeCount)
			return null;

		HST_EnemyOrderState queuedOrder = state.m_aEnemyOrders[state.m_aEnemyOrders.Count() - 1];
		if (!queuedOrder || queuedOrder.m_iOperationContractVersion != 0
			|| queuedOrder.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return null;

		return queuedOrder;
	}

	HST_EnemyOrderState QueueDebugPetrosAttack(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, string factionKey)
	{
		if (!state || !preset || !enemyDirector || factionKey.IsEmpty())
			return null;

		enemyDirector.AddResources(state, factionKey, 100, 100);
		return QueuePetrosAttack(state, preset, enemyDirector, factionKey);
	}

	HST_EnemyOrderState QueuePetrosAttack(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, string factionKey)
	{
		if (!state || !preset || !enemyDirector || factionKey.IsEmpty())
			return null;

		HST_ZoneState targetZone = ResolvePetrosAttackTargetZone(state, preset);
		if (!targetZone)
			return null;

		if (HasActiveOrderForZone(state, factionKey, targetZone.m_sZoneId, true))
			return null;

		int beforeCount = state.m_aEnemyOrders.Count();
		if (!QueueOrder(state, preset, enemyDirector, null, factionKey, targetZone, HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK, SPEND_MODE_PROACTIVE_ATTACK))
			return null;
		if (state.m_aEnemyOrders.Count() <= beforeCount)
			return null;

		HST_EnemyOrderState order = state.m_aEnemyOrders[state.m_aEnemyOrders.Count() - 1];
		order.m_vTargetPosition = ResolvePetrosAttackTargetPosition(state);
		order.m_sRuntimeStatus = "petros_attack_ordered";
		return order;
	}

	int DebugResolveDueOrdersNow(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons)
	{
		if (!state)
			return 0;

		int resolved;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				continue;
			if (HasVersionedEnemyOperation(order))
				continue;

			order.m_iResolveAtSecond = state.m_iElapsedSeconds;
			resolved++;
		}

		ResolveOrders(state, preset, garrisons);
		return resolved;
	}

	bool DebugResolveOrderNow(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, string orderId)
	{
		HST_EnemyOrderState order = FindOrderForDebug(state, orderId);
		if (!order || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return false;
		if (HasVersionedEnemyOperation(order))
			return false;

		order.m_iResolveAtSecond = state.m_iElapsedSeconds;
		if (order.m_bPhysicalized)
			order.m_iResolveAtSecond = Math.Max(0, state.m_iElapsedSeconds - PHYSICAL_ORDER_TIMEOUT_SECONDS);

		return ResolveOrderNow(state, preset, garrisons, order);
	}

	// Advances only the supplied legacy order for an isolated Campaign Debug fixture.
	// This intentionally bypasses the commander cadence accumulator and every other order.
	bool DebugTickLegacyOrderNow(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_SupportRequestService support,
		HST_GarrisonService garrisons,
		HST_EnemyOrderState order)
	{
		if (!state || !preset || !enemyDirector || !order)
			return false;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			|| HasVersionedEnemyOperation(order))
			return false;

		bool changed;
		if (ShouldPhysicalizeOrder(state, preset, order))
			changed = TryPhysicalizeOrder(state, preset, support, order) || changed;

		changed = SyncPhysicalizedOrder(state, order, enemyDirector) || changed;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return changed;
		if (state.m_iElapsedSeconds < order.m_iResolveAtSecond)
			return changed;
		if (order.m_bPhysicalized && !IsPhysicalizedOrderTimedOut(state, order))
			return changed;

		return ResolveOrderNow(state, preset, garrisons, order) || changed;
	}

	bool DebugApplySurvivorRefund(HST_CampaignState state, HST_EnemyDirectorService enemyDirector, HST_EnemyOrderState order, HST_ActiveGroupState group)
	{
		if (HasVersionedEnemyOperation(order))
			return false;
		return ApplySurvivorRefund(state, enemyDirector, order, group);
	}

	bool SettleTrackedLegacyOrderForAdministrativeStop(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_SupportRequestService support,
		HST_PhysicalWarService physicalWar,
		HST_EnemyOrderState order,
		string reason)
	{
		if (!state || !enemyDirector || !support || !physicalWar || !order
			|| HasVersionedEnemyOperation(order))
			return false;
		if (reason.IsEmpty())
			reason = "campaign debug administrative stop";
		bool attackFunded = order.m_iAttackCost > 0 && order.m_iSupportCost == 0;
		bool supportFunded = order.m_iSupportCost > 0 && order.m_iAttackCost == 0;
		if ((order.m_iAttackCost > 0 || order.m_iSupportCost > 0)
			&& (!attackFunded && !supportFunded))
			return false;
		if (attackFunded || supportFunded)
		{
			string debitFailure = HST_EnemyCounterattackSaveValidationService
				.ValidateOriginalResourceDebitAuthority(
					state.m_aEnemyStrategicMutations,
					order);
			if (!debitFailure.IsEmpty())
			{
				order.m_sFailureReason = debitFailure;
				return false;
			}
		}

		HST_SupportRequestState request;
		if (!order.m_sSupportRequestId.IsEmpty())
		{
			request = state.FindSupportRequest(order.m_sSupportRequestId);
			if (!request || request.m_iOperationContractVersion != 0)
				return false;
			if (request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED
				&& request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
			{
				if (!support.CancelSupportRequest(state, request.m_sRequestId, false))
					return false;
			}
		}

		string runtimeFailure;
		if (!physicalWar.SettleCampaignDebugTrackedLegacyEnemyOrderRuntime(
			state,
			order,
			runtimeFailure))
		{
			order.m_sFailureReason = runtimeFailure;
			return false;
		}

		if (order.m_bResourceRefundApplied)
		{
			if (order.m_iRefundedAttackResources != order.m_iAttackCost
				|| order.m_iRefundedSupportResources != order.m_iSupportCost)
				return false;
		}
		else if (order.m_iAttackCost > 0 || order.m_iSupportCost > 0)
		{
			string refundMutationId = "enemy_resource_refund_"
				+ order.m_sOrderId + "_campaign_debug_admin_stop";
			bool refunded;
			if (attackFunded)
				refunded = enemyDirector.RefundProactiveAttackResources(
					state,
					order.m_sFactionKey,
					order.m_iAttackCost,
					reason,
					refundMutationId,
					order.m_sOrderId,
					order.m_sOrderId,
					order.m_sOperationId,
					order.m_sManifestId,
					order.m_sTargetZoneId);
			else if (supportFunded)
				refunded = enemyDirector.RefundDefenseResources(
					state,
					order.m_sFactionKey,
					order.m_sTargetZoneId,
					0,
					order.m_iSupportCost,
					reason,
					refundMutationId,
					order.m_sOrderId,
					order.m_sOrderId,
					order.m_sOperationId,
					order.m_sManifestId);
			if (!refunded)
				return false;
			order.m_sResourceRefundMutationId = refundMutationId;
			order.m_iRefundedAttackResources = order.m_iAttackCost;
			order.m_iRefundedSupportResources = order.m_iSupportCost;
			order.m_bResourceRefundApplied = true;
		}

		if (!IsTrackedLegacyAdministrativeRefundExact(state, order))
			return false;
		if (request)
		{
			if (request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED
				&& request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
				return false;
			request.m_bPhysicalized = false;
			request.m_sRuntimeEntityId = "";
		}
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		order.m_iResolvedAtSecond = state.m_iElapsedSeconds;
		order.m_sRuntimeStatus = "campaign_debug_administrative_stop";
		order.m_sResolutionKind = "administrative_stop";
		order.m_sFailureReason = reason;
		order.m_bPhysicalized = false;
		order.m_bAbstractResolved = false;
		return true;
	}

	protected bool IsTrackedLegacyAdministrativeRefundExact(
		HST_CampaignState state,
		HST_EnemyOrderState order)
	{
		if (!state || !order)
			return false;
		if (order.m_iAttackCost <= 0 && order.m_iSupportCost <= 0)
			return !order.m_bResourceRefundApplied
				&& order.m_sResourceRefundMutationId.IsEmpty();
		string debitFailure = HST_EnemyCounterattackSaveValidationService
			.ValidateOriginalResourceDebitAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		if (!debitFailure.IsEmpty() || !order.m_bResourceRefundApplied
			|| order.m_sResourceRefundMutationId.IsEmpty()
			|| order.m_bResourceSettlementApplied
			|| !order.m_sResourceSettlementId.IsEmpty()
			|| !order.m_sResourceSettlementKind.IsEmpty())
			return false;
		string expectedRefundMutationId = "enemy_resource_refund_"
			+ order.m_sOrderId + "_campaign_debug_admin_stop";
		if (order.m_sResourceRefundMutationId != expectedRefundMutationId)
			return false;
		HST_EnemyStrategicMutationState refund;
		HST_EnemyStrategicMutationState debit;
		int refundIdentityCount;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (!mutation)
				continue;
			if (mutation.m_sMutationId == order.m_sResourceRefundMutationId)
			{
				refund = mutation;
				refundIdentityCount++;
			}
			if (mutation.m_sMutationId == order.m_sResourceDebitMutationId)
				debit = mutation;
		}
		if (refundIdentityCount != 1 || !refund || !debit
			|| HST_EnemyCounterattackSaveValidationService.CountResourceMutationClaimants(
				state.m_aEnemyStrategicMutations,
				order,
				false) != 1)
			return false;
		bool attackFunded = order.m_iAttackCost > 0 && order.m_iSupportCost == 0;
		string expectedKind = "defense_support_refund";
		if (attackFunded)
			expectedKind = "proactive_attack_refund";
		if (!IsTrackedLegacyAdministrativeRefundIdentityExact(refund, order, expectedKind))
			return false;
		if (!IsTrackedLegacyAdministrativeRefundAmountExact(refund, order))
			return false;
		if (!IsTrackedLegacyAdministrativeRefundEvidenceExact(refund, debit, order))
			return false;
		string shapeFailure;
		return HST_EnemyStrategicResourceSaveValidationService
			.ValidateMutationShape(refund, shapeFailure);
	}

	protected bool IsTrackedLegacyAdministrativeRefundIdentityExact(
		HST_EnemyStrategicMutationState refund,
		HST_EnemyOrderState order,
		string expectedKind)
	{
		if (!refund || !order)
			return false;
		if (refund.m_iContractVersion != HST_EnemyStrategicResourceService.CONTRACT_VERSION)
			return false;
		if (!refund.m_bApplied || refund.m_sKind != expectedKind)
			return false;
		if (refund.m_sSourceId != order.m_sOrderId
			|| refund.m_sOrderId != order.m_sOrderId)
			return false;
		if (refund.m_sOperationId != order.m_sOperationId
			|| refund.m_sManifestId != order.m_sManifestId)
			return false;
		if (refund.m_sFactionKey != order.m_sFactionKey)
			return false;
		return HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
			refund.m_sZoneId,
			order.m_sTargetZoneId);
	}

	protected bool IsTrackedLegacyAdministrativeRefundAmountExact(
		HST_EnemyStrategicMutationState refund,
		HST_EnemyOrderState order)
	{
		if (!refund || !order)
			return false;
		if (refund.m_iAttackDelta != order.m_iAttackCost
			|| refund.m_iSupportDelta != order.m_iSupportCost)
			return false;
		if (refund.m_iAggressionDelta != 0)
			return false;
		return order.m_iRefundedAttackResources == order.m_iAttackCost
			&& order.m_iRefundedSupportResources == order.m_iSupportCost;
	}

	protected bool IsTrackedLegacyAdministrativeRefundEvidenceExact(
		HST_EnemyStrategicMutationState refund,
		HST_EnemyStrategicMutationState debit,
		HST_EnemyOrderState order)
	{
		if (!refund || !debit || !order)
			return false;
		if (refund.m_iCreatedAtSecond < order.m_iCreatedAtSecond
			|| refund.m_iCreatedAtSecond < debit.m_iCreatedAtSecond)
			return false;
		return refund.m_sContributionHash.IsEmpty();
	}

	protected bool QueueOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, HST_SupportRequestService support, string factionKey, HST_ZoneState targetZone, HST_EEnemyOrderType orderType, string spendMode = "", bool forceDebugLegacyOperation = false)
	{
		if (!state || !preset || !enemyDirector || !targetZone || factionKey.IsEmpty())
			return false;

		string localityReason;
		if (!IsLocalOperationTargetAllowed(state, preset, factionKey, targetZone, localityReason))
		{
			Print(string.Format("Partisan enemy commander | order skipped for %1 at %2 type %3 | local front blocked: %4", factionKey, targetZone.m_sZoneId, orderType, localityReason));
			return false;
		}

		string resolvedSpendMode = ResolveOrderSpendMode(state, preset, targetZone, orderType, spendMode);
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL && !forceDebugLegacyOperation)
			resolvedSpendMode = SPEND_MODE_PROACTIVE_ATTACK;
		int attackCost;
		int supportCost;
		ResolveOrderCostsForSpendMode(orderType, resolvedSpendMode, attackCost, supportCost);
		HST_EnemyOrderState order = new HST_EnemyOrderState();
		order.m_sOrderId = string.Format("order_%1_%2_%3", factionKey, state.m_iElapsedSeconds, state.m_aEnemyOrders.Count());
		order.m_sOperationId = HST_StableIdService.BuildOperationId("enemy_order", order.m_sOrderId);
		order.m_sFactionKey = factionKey;
		order.m_eType = orderType;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED;
		order.m_sTargetZoneId = targetZone.m_sZoneId;
		order.m_sRuntimeStatus = "active_" + resolvedSpendMode + "_pending";
		order.m_vTargetPosition = targetZone.m_vPosition;
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
			order.m_vTargetPosition = ResolvePetrosAttackTargetPosition(state);
		HST_ZoneState sourceZone = ResolveOrderSourceZone(state, factionKey, targetZone);
		if (sourceZone)
		{
			order.m_sSourceZoneId = sourceZone.m_sZoneId;
			order.m_vSourcePosition = sourceZone.m_vPosition;
		}
		else
			order.m_vSourcePosition = targetZone.m_vPosition;
		order.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		order.m_iResolveAtSecond = state.m_iElapsedSeconds + ORDER_RESOLVE_SECONDS;
		order.m_iAttackCost = attackCost;
		order.m_iSupportCost = supportCost;

		bool exactEnemyQRF = orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF && !forceDebugLegacyOperation;
		bool exactEnemyCounterattack = orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK && !forceDebugLegacyOperation;
		bool exactEnemyPatrol = orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL && !forceDebugLegacyOperation;
		bool exactEnemyGarrisonRebuild = orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON && !forceDebugLegacyOperation;
		HST_ForceManifestState exactManifest;
		HST_GeneratedRouteState exactPatrolRoute;
		if (exactEnemyQRF)
		{
			if (!m_ForcePlanning || !m_ExactEnemyQRF)
			{
				Print(string.Format("Partisan enemy commander | exact QRF skipped for %1 at %2 | exact authority services unavailable", factionKey, targetZone.m_sZoneId), LogLevel.WARNING);
				return false;
			}
			if (!sourceZone || sourceZone.m_sZoneId == targetZone.m_sZoneId)
			{
				Print(string.Format("Partisan enemy commander | exact QRF skipped for %1 at %2 | distinct faction-owned source unavailable", factionKey, targetZone.m_sZoneId));
				return false;
			}
			if (state.FindActiveQRF(targetZone.m_sZoneId, factionKey) || HasActiveLegacyEnemyQRFSupport(state, factionKey, targetZone.m_sZoneId))
			{
				Print(string.Format("Partisan enemy commander | exact QRF skipped for %1 at %2 | legacy response already owns target", factionKey, targetZone.m_sZoneId));
				return false;
			}
			order.m_iOperationContractVersion = HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION;
			HST_EnemyDefensiveQRFManifestResult planned = m_ForcePlanning.PlanExactEnemyDefensiveQRF(state, preset, order);
			if (!planned || !planned.m_bSuccess || !planned.m_Manifest)
			{
				string planningFailure = "exact manifest planning failed";
				if (planned && !planned.m_sFailureReason.IsEmpty())
					planningFailure = planned.m_sFailureReason;
				Print(string.Format("Partisan enemy commander | exact QRF skipped for %1 at %2 | %3", factionKey, targetZone.m_sZoneId, planningFailure));
				return false;
			}
			exactManifest = planned.m_Manifest;
			order.m_sManifestId = exactManifest.m_sManifestId;
			order.m_sManifestHash = exactManifest.m_sManifestHash;
			order.m_iCompositionManpower = exactManifest.m_iAcceptedMemberCount;
			order.m_iResolveAtSecond = 0;
			if (state.FindOperation(order.m_sOperationId) || state.FindForceManifest(order.m_sManifestId)
				|| state.FindActiveGroup("projection_" + order.m_sOperationId)
				|| state.FindForceSpawnResultByRequest(order.m_sOrderId))
			{
				Print(string.Format("Partisan enemy commander | exact QRF skipped for %1 at %2 | durable identity already exists", factionKey, targetZone.m_sZoneId));
				return false;
			}
			HST_EnemyQRFAdmissionResult admissionPreflight = m_ExactEnemyQRF.CanAdmitPreparedOrder(
				state,
				order,
				exactManifest,
				enemyDirector);
			if (!admissionPreflight || !admissionPreflight.m_bSuccess)
			{
				string preflightFailure = "exact admission preflight failed";
				if (admissionPreflight && !admissionPreflight.m_sFailureReason.IsEmpty())
					preflightFailure = admissionPreflight.m_sFailureReason;
				Print(string.Format("Partisan enemy commander | exact QRF skipped for %1 at %2 | %3", factionKey, targetZone.m_sZoneId, preflightFailure));
				return false;
			}
		}
		else if (exactEnemyCounterattack)
		{
			if (!m_ForcePlanning || !m_ExactEnemyCounterattack)
			{
				Print(string.Format("Partisan enemy commander | exact counterattack skipped for %1 at %2 | exact authority services unavailable", factionKey, targetZone.m_sZoneId), LogLevel.WARNING);
				return false;
			}
			if (!sourceZone || sourceZone.m_sZoneId == targetZone.m_sZoneId)
			{
				Print(string.Format("Partisan enemy commander | exact counterattack skipped for %1 at %2 | distinct faction-owned source unavailable", factionKey, targetZone.m_sZoneId));
				return false;
			}
			order.m_iOperationContractVersion = HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION;
			HST_EnemyCounterattackManifestResult counterattackPlan = m_ForcePlanning.PlanExactEnemyCounterattack(state, preset, order);
			if (!counterattackPlan || !counterattackPlan.m_bSuccess || !counterattackPlan.m_Manifest)
			{
				string counterattackPlanningFailure = "exact counterattack manifest planning failed";
				if (counterattackPlan && !counterattackPlan.m_sFailureReason.IsEmpty())
					counterattackPlanningFailure = counterattackPlan.m_sFailureReason;
				Print(string.Format("Partisan enemy commander | exact counterattack skipped for %1 at %2 | %3", factionKey, targetZone.m_sZoneId, counterattackPlanningFailure));
				return false;
			}
			exactManifest = counterattackPlan.m_Manifest;
			order.m_sManifestId = exactManifest.m_sManifestId;
			order.m_sManifestHash = exactManifest.m_sManifestHash;
			order.m_iCompositionManpower = exactManifest.m_iAcceptedMemberCount;
			order.m_iResolveAtSecond = 0;
			if (state.FindOperation(order.m_sOperationId) || state.FindForceManifest(order.m_sManifestId)
				|| state.FindActiveGroup("projection_" + order.m_sOperationId)
				|| state.FindForceSpawnResultByRequest(order.m_sOrderId))
			{
				Print(string.Format("Partisan enemy commander | exact counterattack skipped for %1 at %2 | durable identity already exists", factionKey, targetZone.m_sZoneId));
				return false;
			}
			HST_EnemyCounterattackAdmissionResult counterattackPreflight = m_ExactEnemyCounterattack.CanAdmitPreparedOrder(
				state,
				order,
				exactManifest,
				enemyDirector);
			if (!counterattackPreflight || !counterattackPreflight.m_bSuccess)
			{
				string counterattackPreflightFailure = "exact counterattack admission preflight failed";
				if (counterattackPreflight && !counterattackPreflight.m_sFailureReason.IsEmpty())
					counterattackPreflightFailure = counterattackPreflight.m_sFailureReason;
				Print(string.Format("Partisan enemy commander | exact counterattack skipped for %1 at %2 | %3", factionKey, targetZone.m_sZoneId, counterattackPreflightFailure));
				return false;
			}
		}
		else if (exactEnemyGarrisonRebuild)
		{
			if (!m_ForcePlanning || !m_ExactEnemyGarrisonRebuild)
			{
				Print(string.Format("Partisan enemy commander | exact garrison rebuild skipped for %1 at %2 | exact authority services unavailable", factionKey, targetZone.m_sZoneId), LogLevel.WARNING);
				return false;
			}
			if (!sourceZone || sourceZone.m_sZoneId == targetZone.m_sZoneId)
			{
				Print(string.Format("Partisan enemy commander | exact garrison rebuild skipped for %1 at %2 | distinct faction-owned source unavailable", factionKey, targetZone.m_sZoneId));
				return false;
			}
			order.m_iOperationContractVersion = HST_EnemyGarrisonRebuildOperationService.EXACT_CONTRACT_VERSION;
			order.m_iTargetOwnershipRevision = Math.Max(1, targetZone.m_iOwnershipRevision);
			HST_EnemyGarrisonRebuildManifestResult rebuildPlan
				= m_ForcePlanning.PlanExactEnemyGarrisonRebuild(state, preset, order);
			if (!rebuildPlan || !rebuildPlan.m_bSuccess || !rebuildPlan.m_Manifest)
			{
				string rebuildPlanningFailure = "exact garrison rebuild manifest planning failed";
				if (rebuildPlan && !rebuildPlan.m_sFailureReason.IsEmpty())
					rebuildPlanningFailure = rebuildPlan.m_sFailureReason;
				Print(string.Format("Partisan enemy commander | exact garrison rebuild skipped for %1 at %2 | %3", factionKey, targetZone.m_sZoneId, rebuildPlanningFailure));
				return false;
			}
			exactManifest = rebuildPlan.m_Manifest;
			order.m_sManifestId = exactManifest.m_sManifestId;
			order.m_sManifestHash = exactManifest.m_sManifestHash;
			order.m_iCompositionManpower = exactManifest.m_iAcceptedMemberCount;
			order.m_iResolveAtSecond = 0;
			if (state.FindOperation(order.m_sOperationId) || state.FindForceManifest(order.m_sManifestId)
				|| state.FindActiveGroup("projection_" + order.m_sOperationId)
				|| state.FindForceSpawnResultByRequest(order.m_sOrderId))
			{
				Print(string.Format("Partisan enemy commander | exact garrison rebuild skipped for %1 at %2 | durable identity already exists", factionKey, targetZone.m_sZoneId));
				return false;
			}
			HST_EnemyGarrisonRebuildAdmissionResult rebuildPreflight
				= m_ExactEnemyGarrisonRebuild.CanAdmitPreparedOrder(
					state,
					order,
					exactManifest,
					enemyDirector);
			if (!rebuildPreflight || !rebuildPreflight.m_bSuccess)
			{
				string rebuildPreflightFailure = "exact garrison rebuild admission preflight failed";
				if (rebuildPreflight && !rebuildPreflight.m_sFailureReason.IsEmpty())
					rebuildPreflightFailure = rebuildPreflight.m_sFailureReason;
				Print(string.Format("Partisan enemy commander | exact garrison rebuild skipped for %1 at %2 | %3", factionKey, targetZone.m_sZoneId, rebuildPreflightFailure));
				return false;
			}
		}
		else if (exactEnemyPatrol)
		{
			if (!PrepareExactEnemyPatrol(
				state,
				preset,
				enemyDirector,
				factionKey,
				targetZone,
				sourceZone,
				order,
				exactManifest,
				exactPatrolRoute))
				return false;
		}
		string spendReason;
		bool spent;
		string debitMutationId = "enemy_resource_debit_" + order.m_sOrderId;
		order.m_sResourceDebitMutationId = debitMutationId;
		if (resolvedSpendMode == SPEND_MODE_PROACTIVE_ATTACK)
			spent = enemyDirector.TrySpendProactiveAttack(
				state,
				factionKey,
				attackCost,
				spendReason,
				debitMutationId,
				order.m_sOrderId,
				order.m_sOrderId,
				order.m_sOperationId,
				order.m_sManifestId,
				order.m_sTargetZoneId);
		else
			spent = enemyDirector.TrySpendDefense(
				state,
				targetZone,
				factionKey,
				attackCost,
				supportCost,
				spendReason,
				debitMutationId,
				order.m_sOrderId,
				order.m_sOrderId,
				order.m_sOperationId,
				order.m_sManifestId);

		if (!spent)
		{
			Print(string.Format("Partisan enemy commander | order skipped for %1 at %2 type %3 mode %4 | %5", factionKey, targetZone.m_sZoneId, orderType, resolvedSpendMode, spendReason));
			return false;
		}

		state.m_aEnemyOrders.Insert(order);
		if (exactEnemyQRF)
		{
			HST_EnemyQRFAdmissionResult admission = m_ExactEnemyQRF.AdmitPreparedOrder(state, order, exactManifest, enemyDirector);
			if (!admission || !admission.m_bSuccess)
			{
				string admissionFailure = order.m_sFailureReason;
				if (admission && !admission.m_sFailureReason.IsEmpty())
					admissionFailure = admission.m_sFailureReason;
				Print(string.Format("Partisan enemy commander | exact QRF admission failed for %1 at %2 | %3", factionKey, targetZone.m_sZoneId, admissionFailure), LogLevel.WARNING);
				// Admission failure is itself a durable aborted/refunded outcome. Report the
				// mutation so the coordinator persists it instead of treating this tick as idle.
				return true;
			}
			Print(string.Format("Partisan | exact enemy defensive QRF %1 active from %2 to %3 | manifest %4", order.m_sOrderId, order.m_sSourceZoneId, order.m_sTargetZoneId, order.m_sManifestId));
			return true;
		}
		if (exactEnemyCounterattack)
		{
			HST_EnemyCounterattackAdmissionResult counterattackAdmission = m_ExactEnemyCounterattack.AdmitPreparedOrder(
				state,
				order,
				exactManifest,
				enemyDirector);
			if (!counterattackAdmission || !counterattackAdmission.m_bSuccess)
			{
				string counterattackAdmissionFailure = order.m_sFailureReason;
				if (counterattackAdmission && !counterattackAdmission.m_sFailureReason.IsEmpty())
					counterattackAdmissionFailure = counterattackAdmission.m_sFailureReason;
				Print(string.Format("Partisan enemy commander | exact counterattack admission failed for %1 at %2 | %3", factionKey, targetZone.m_sZoneId, counterattackAdmissionFailure), LogLevel.WARNING);
				return true;
			}
			Print(string.Format("Partisan | exact enemy counterattack %1 active from %2 to %3 | manifest %4", order.m_sOrderId, order.m_sSourceZoneId, order.m_sTargetZoneId, order.m_sManifestId));
			return true;
		}
		if (exactEnemyGarrisonRebuild)
		{
			HST_EnemyGarrisonRebuildAdmissionResult rebuildAdmission
				= m_ExactEnemyGarrisonRebuild.AdmitPreparedOrder(
					state,
					order,
					exactManifest,
					enemyDirector);
			if (!rebuildAdmission || !rebuildAdmission.m_bSuccess)
			{
				string rebuildAdmissionFailure = order.m_sFailureReason;
				if (rebuildAdmission && !rebuildAdmission.m_sFailureReason.IsEmpty())
					rebuildAdmissionFailure = rebuildAdmission.m_sFailureReason;
				Print(string.Format("Partisan enemy commander | exact garrison rebuild admission failed for %1 at %2 | %3", factionKey, targetZone.m_sZoneId, rebuildAdmissionFailure), LogLevel.WARNING);
				return true;
			}
			Print(string.Format("Partisan | exact enemy garrison rebuild %1 active from %2 to %3 | manifest %4", order.m_sOrderId, order.m_sSourceZoneId, order.m_sTargetZoneId, order.m_sManifestId));
			return true;
		}
		if (exactEnemyPatrol)
		{
			HST_EnemyPatrolAdmissionResult patrolAdmission = m_ExactEnemyPatrol.AdmitPreparedOrder(
				state,
				order,
				exactManifest,
				exactPatrolRoute,
				enemyDirector);
			if (!patrolAdmission || !patrolAdmission.m_bSuccess)
			{
				string admissionFailure = order.m_sFailureReason;
				if (patrolAdmission && !patrolAdmission.m_sFailureReason.IsEmpty())
					admissionFailure = patrolAdmission.m_sFailureReason;
				Print(string.Format("Partisan enemy commander | exact patrol admission failed for %1 at %2 | %3", factionKey, targetZone.m_sZoneId, admissionFailure), LogLevel.WARNING);
				return true;
			}
			Print(string.Format("Partisan | exact enemy patrol %1 active from %2 around %3 | manifest %4 route %5", order.m_sOrderId, order.m_sSourceZoneId, order.m_sTargetZoneId, order.m_sManifestId, exactPatrolRoute.m_sRouteId));
			return true;
		}

		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
			state.m_iLastHQAttackSecond = state.m_iElapsedSeconds;
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
			Print(string.Format("Partisan | enemy order %1 active against Petros/HQ near %2 at %3", order.m_sOrderId, targetZone.m_sZoneId, order.m_vTargetPosition));
		else
			Print(string.Format("Partisan | enemy order %1 active at %2", order.m_sOrderId, targetZone.m_sZoneId));
		return true;
	}

	protected bool PrepareExactEnemyPatrol(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		string factionKey,
		HST_ZoneState targetZone,
		HST_ZoneState sourceZone,
		HST_EnemyOrderState order,
		out HST_ForceManifestState manifest,
		out HST_GeneratedRouteState route)
	{
		manifest = null;
		route = null;
		if (!m_ForcePlanning || !m_ExactEnemyPatrol)
		{
			Print(string.Format("Partisan enemy commander | exact patrol skipped for %1 at %2 | exact authority services unavailable", factionKey, targetZone.m_sZoneId), LogLevel.WARNING);
			return false;
		}
		if (!sourceZone || sourceZone.m_sZoneId == targetZone.m_sZoneId)
		{
			Print(string.Format("Partisan enemy commander | exact patrol skipped for %1 at %2 | distinct faction-owned source unavailable", factionKey, targetZone.m_sZoneId));
			return false;
		}
		order.m_iOperationContractVersion = HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION;
		HST_EnemyPatrolManifestResult plan = m_ForcePlanning.PlanExactEnemyPatrol(state, preset, order);
		if (!plan || !plan.m_bSuccess || !plan.m_Manifest)
		{
			string planningFailure = "exact patrol manifest planning failed";
			if (plan && !plan.m_sFailureReason.IsEmpty())
				planningFailure = plan.m_sFailureReason;
			Print(string.Format("Partisan enemy commander | exact patrol skipped for %1 at %2 | %3", factionKey, targetZone.m_sZoneId, planningFailure));
			return false;
		}
		manifest = plan.m_Manifest;
		order.m_sManifestId = manifest.m_sManifestId;
		order.m_sManifestHash = manifest.m_sManifestHash;
		order.m_iCompositionManpower = manifest.m_iAcceptedMemberCount;
		order.m_iResolveAtSecond = 0;
		route = m_ExactEnemyPatrol.ResolvePatrolRoute(state, order);
		if (!route)
		{
			Print(string.Format("Partisan enemy commander | exact patrol skipped for %1 at %2 | generated patrol route unavailable", factionKey, targetZone.m_sZoneId));
			return false;
		}
		ref array<vector> positions = HST_OperationRouteCursorService.BuildOrderedRoutePositions(route);
		string routeHash = HST_OperationRouteCursorService.BuildRouteContractHash(route, positions);
		if (positions.Count() < 2 || routeHash.IsEmpty())
		{
			Print(string.Format("Partisan enemy commander | exact patrol skipped for %1 at %2 | generated patrol route hash failed", factionKey, targetZone.m_sZoneId));
			return false;
		}
		HST_EnemyPatrolAdmissionResult preflight = m_ExactEnemyPatrol.CanAdmitPreparedOrder(
			state,
			order,
			manifest,
			route,
			enemyDirector);
		if (!preflight || !preflight.m_bSuccess)
		{
			string preflightFailure = "exact patrol admission preflight failed";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				preflightFailure = preflight.m_sFailureReason;
			Print(string.Format("Partisan enemy commander | exact patrol skipped for %1 at %2 | %3", factionKey, targetZone.m_sZoneId, preflightFailure));
			return false;
		}
		return true;
	}

	protected bool TickActiveOrderRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestService support, HST_GarrisonService garrisons, HST_EnemyDirectorService enemyDirector)
	{
		bool changed;
		if (!state)
			return false;

		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order)
				continue;
			string runtimeOwner = ResolveRuntimeOwner(order);
			// A failed admission can leave an ABORTED exact counterattack with an
			// irrevocable PREPARED refund intent. Dispatch that transaction before
			// ordinary open-order filtering so it does not wait for a restart.
			if (runtimeOwner == RUNTIME_OWNER_EXACT_COUNTERATTACK
				&& m_ExactEnemyCounterattack
				&& m_ExactEnemyCounterattack.HasPreparedSettlementResumeCandidate(
					order,
					state.FindOperation(order.m_sOperationId)))
			{
				changed = m_ExactEnemyCounterattack.TickOrder(
					state,
					preset,
					enemyDirector,
					order) || changed;
				continue;
			}
			HST_OperationRecordState rebuildOperation = state.FindOperation(order.m_sOperationId);
			if (runtimeOwner == RUNTIME_OWNER_EXACT_GARRISON_REBUILD
				&& m_ExactEnemyGarrisonRebuild
				&& (m_ExactEnemyGarrisonRebuild.HasPreparedSettlementResumeCandidate(
						order,
						rebuildOperation)
					|| (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED
						&& rebuildOperation
						&& rebuildOperation.m_eSettlementState
							== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)))
			{
				changed = m_ExactEnemyGarrisonRebuild.TickOrder(
					state,
					preset,
					enemyDirector,
					order) || changed;
				continue;
			}
			if ((runtimeOwner == RUNTIME_OWNER_QUARANTINED
					|| runtimeOwner == RUNTIME_OWNER_UNSUPPORTED)
				&& order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON
				&& order.m_iOperationContractVersion != 0)
			{
				string rebuildFailure = string.Format(
					"garrison rebuild-shaped enemy authority has no supported typed runtime owner: version %1 owner %2",
					order.m_iOperationContractVersion,
					runtimeOwner);
				if (m_ExactEnemyGarrisonRebuild)
					changed = m_ExactEnemyGarrisonRebuild.QuarantineUnsupportedGarrisonRebuildAuthority(
						state,
						order,
						rebuildFailure) || changed;
				else
					changed = FailClosedUnsupportedVersionedOrder(state, order, runtimeOwner) || changed;
				continue;
			}
			bool openOrder = order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				|| order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
			if (!openOrder)
				continue;
			if (runtimeOwner != RUNTIME_OWNER_LEGACY
				&& runtimeOwner != RUNTIME_OWNER_EXACT_PATROL
				&& HasLinkedPatrolAuthority(state, order))
			{
				string patrolFailure = BuildUnsupportedPatrolAuthorityReason(order, runtimeOwner);
				if (runtimeOwner == RUNTIME_OWNER_EXACT_GARRISON_REBUILD
					&& m_ExactEnemyGarrisonRebuild)
					changed = m_ExactEnemyGarrisonRebuild.QuarantineUnsupportedGarrisonRebuildAuthority(
						state,
						order,
						patrolFailure) || changed;
				else if (m_ExactEnemyPatrol)
					changed = m_ExactEnemyPatrol.QuarantineUnsupportedPatrolAuthority(state, order, patrolFailure) || changed;
				else
					changed = FailClosedUnsupportedVersionedOrder(state, order, runtimeOwner) || changed;
				continue;
			}
			if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			{
				if (runtimeOwner == RUNTIME_OWNER_QUARANTINED || runtimeOwner == RUNTIME_OWNER_UNSUPPORTED)
					changed = FailClosedUnsupportedVersionedOrder(state, order, runtimeOwner) || changed;
				continue;
			}
			if (runtimeOwner == RUNTIME_OWNER_EXACT_QRF)
			{
				if (m_ExactEnemyQRF)
					changed = m_ExactEnemyQRF.TickOrder(state, preset, enemyDirector, order) || changed;
				else if (order.m_sRuntimeStatus != "exact_qrf_runtime_service_missing")
				{
					order.m_sRuntimeStatus = "exact_qrf_runtime_service_missing";
					order.m_sFailureReason = "exact enemy defensive QRF runtime service is missing";
					changed = true;
				}
				continue;
			}
			if (runtimeOwner == RUNTIME_OWNER_EXACT_COUNTERATTACK)
			{
				if (m_ExactEnemyCounterattack)
					changed = m_ExactEnemyCounterattack.TickOrder(state, preset, enemyDirector, order) || changed;
				else if (order.m_sRuntimeStatus != "exact_counterattack_runtime_service_missing")
				{
					order.m_sRuntimeStatus = "exact_counterattack_runtime_service_missing";
					order.m_sFailureReason = "exact enemy counterattack runtime service is missing";
					changed = true;
				}
				continue;
			}
			if (runtimeOwner == RUNTIME_OWNER_EXACT_PATROL)
			{
				if (m_ExactEnemyPatrol)
					changed = m_ExactEnemyPatrol.TickOrder(state, preset, enemyDirector, order) || changed;
				else if (order.m_sRuntimeStatus != "exact_patrol_runtime_service_missing")
				{
					order.m_sRuntimeStatus = "exact_patrol_runtime_service_missing";
					order.m_sFailureReason = "exact enemy patrol runtime service is missing";
					changed = true;
				}
				continue;
			}
			if (runtimeOwner == RUNTIME_OWNER_EXACT_GARRISON_REBUILD)
			{
				if (m_ExactEnemyGarrisonRebuild)
					changed = m_ExactEnemyGarrisonRebuild.TickOrder(state, preset, enemyDirector, order) || changed;
				else if (order.m_sRuntimeStatus != "exact_garrison_rebuild_runtime_service_missing")
				{
					order.m_sRuntimeStatus = "exact_garrison_rebuild_runtime_service_missing";
					order.m_sFailureReason = "exact enemy garrison rebuild runtime service is missing";
					changed = true;
				}
				continue;
			}
			if (runtimeOwner != RUNTIME_OWNER_LEGACY)
			{
				changed = FailClosedUnsupportedVersionedOrder(state, order, runtimeOwner) || changed;
				continue;
			}

			if (ShouldPhysicalizeOrder(state, preset, order))
				changed = TryPhysicalizeOrder(state, preset, support, order) || changed;

			changed = SyncPhysicalizedOrder(state, order, enemyDirector) || changed;
		}

		return changed;
	}

	protected bool HasLinkedPatrolAuthority(HST_CampaignState state, HST_EnemyOrderState order)
	{
		if (!state || !order)
			return false;
		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL)
			return true;

		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL)
				continue;
			bool linked = false;
			if (!order.m_sOperationId.IsEmpty())
				linked = operation.m_sOperationId == order.m_sOperationId;
			if (!linked && !order.m_sOrderId.IsEmpty())
				linked = operation.m_sEnemyOrderId == order.m_sOrderId;
			if (!linked && !order.m_sManifestId.IsEmpty())
				linked = operation.m_sManifestId == order.m_sManifestId;
			if (!linked && !order.m_sSpawnResultId.IsEmpty())
				linked = operation.m_sSpawnResultId == order.m_sSpawnResultId;
			if (!linked && !order.m_sGroupId.IsEmpty())
				linked = operation.m_sGroupId == order.m_sGroupId;
			if (linked)
				return true;
		}
		return false;
	}

	protected string BuildUnsupportedPatrolAuthorityReason(HST_EnemyOrderState order, string runtimeOwner)
	{
		string reason = string.Format(
			"patrol-shaped enemy authority has no supported typed runtime owner: type %1 version %2 owner %3",
			order.m_eType,
			order.m_iOperationContractVersion,
			runtimeOwner);
		if (!order.m_sFailureReason.IsEmpty())
			reason += " | prior evidence: " + order.m_sFailureReason;
		return reason;
	}

	protected bool FailClosedUnsupportedVersionedOrder(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		string runtimeOwner)
	{
		if (!state || !order)
			return false;
		string reason = string.Format(
			"versioned enemy order has no supported typed runtime owner: type %1 version %2 owner %3",
			order.m_eType,
			order.m_iOperationContractVersion,
			runtimeOwner);
		bool changed = order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			|| order.m_sRuntimeStatus != "versioned_runtime_unsupported"
			|| order.m_sFailureReason != reason;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		order.m_sRuntimeStatus = "versioned_runtime_unsupported";
		order.m_sFailureReason = reason;
		order.m_bPhysicalized = false;
		if (order.m_iResolvedAtSecond <= 0)
			order.m_iResolvedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		return changed;
	}

	protected bool IsPhysicalizableOrderType(HST_EEnemyOrderType orderType)
	{
		return orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF
			|| orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK
			|| orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK
			|| orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL
			|| orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK;
	}

	protected bool ShouldPhysicalizeOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyOrderState order)
	{
		if (!state || !preset || !order || order.m_bPhysicalized || !IsPhysicalizableOrderType(order.m_eType))
			return false;
		if (HasVersionedEnemyOperation(order))
			return false;

		if (order.m_sRuntimeStatus == "physicalize_failed")
			return false;

		HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
		if (!targetZone)
			return false;

		if (targetZone.m_bActive)
			return true;

		if (HasActiveMissionNearZone(state, targetZone))
			return true;

		if (HasActiveObjectiveNearZone(state, targetZone))
			return true;

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK && IsPetrosAttackTargetInsidePlayerEventBubble(state))
			return true;

		vector targetPosition = order.m_vTargetPosition;
		if (targetPosition[0] == 0 && targetPosition[1] == 0 && targetPosition[2] == 0)
			targetPosition = targetZone.m_vPosition;

		return HST_WorldPositionService.IsPositionInsidePlayerEventBubble(targetPosition);
	}

	protected bool TryPhysicalizeOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestService support, HST_EnemyOrderState order)
	{
		if (!state || !preset || !support || !order || order.m_bPhysicalized)
			return false;
		if (HasVersionedEnemyOperation(order))
			return false;

		HST_ESupportRequestType supportType = order.m_ePlannedSupportType;
		if (order.m_iPlanningContractVersion != HST_EnemyPlanningAuthorityService.CONTRACT_VERSION)
			supportType = SupportTypeForOrder(state, preset, state.FindZone(order.m_sTargetZoneId), order.m_eType);
		HST_SupportRequestState request = support.RequestPrepaidEnemySupport(
			state,
			preset,
			order.m_sFactionKey,
			supportType,
			order.m_sTargetZoneId,
			order.m_vSourcePosition,
			order.m_vTargetPosition
		);

		if (!request)
		{
			order.m_sFailureReason = "physical support request could not be created";
			order.m_sRuntimeStatus = "physicalize_failed";
			return true;
		}

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
		{
			request.m_sAssetProfileId = request.m_sAssetProfileId + "_petros_attack";
			request.m_sRuntimeStatus = "petros_attack_support_queued";
		}

		order.m_sSupportRequestId = request.m_sRequestId;
		order.m_bPhysicalized = true;
		order.m_iPhysicalizedAtSecond = state.m_iElapsedSeconds;
		order.m_sRuntimeStatus = "physical_support_queued";
		Print(string.Format("Partisan enemy commander | order %1 physicalized via support %2", order.m_sOrderId, order.m_sSupportRequestId));
		return true;
	}

	protected bool SyncPhysicalizedOrder(HST_CampaignState state, HST_EnemyOrderState order, HST_EnemyDirectorService enemyDirector)
	{
		if (!state || !order || !order.m_bPhysicalized || order.m_sSupportRequestId.IsEmpty())
			return false;
		if (HasVersionedEnemyOperation(order))
			return false;

		HST_SupportRequestState request = FindSupportRequest(state, order.m_sSupportRequestId);
		if (!request)
		{
			if (order.m_sRuntimeStatus == "support_missing")
				return false;

			order.m_sFailureReason = "linked support request missing";
			order.m_sRuntimeStatus = "support_missing";
			return true;
		}

		bool changed;
		changed = SyncOrderCompositionFromSupportRequest(order, request) || changed;

		if (order.m_sGroupId != request.m_sGroupId && !request.m_sGroupId.IsEmpty())
		{
			order.m_sGroupId = request.m_sGroupId;
			order.m_sRuntimeStatus = "physical_group_linked";
			changed = true;
		}

		if (!request.m_sFailureReason.IsEmpty() && order.m_sFailureReason != request.m_sFailureReason)
		{
			order.m_sFailureReason = request.m_sFailureReason;
			changed = true;
		}

		if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
		{
			order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
			order.m_iResolvedAtSecond = state.m_iElapsedSeconds;
			order.m_sRuntimeStatus = "aborted_support_cancelled";
			order.m_sResolutionKind = "aborted";
			changed = true;
		}
		else if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED)
		{
			order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED;
			order.m_iResolvedAtSecond = state.m_iElapsedSeconds;
			order.m_sRuntimeStatus = "resolved_physical_support";
			order.m_sResolutionKind = "physical";
			changed = true;
		}

		if (!order.m_sGroupId.IsEmpty())
		{
			HST_ActiveGroupState group = state.FindActiveGroup(order.m_sGroupId);
			if (group && (group.m_sRuntimeStatus == "eliminated" || group.m_sRuntimeStatus == "folded" || group.m_sRuntimeStatus == "spawn_failed"))
			{
				changed = ApplySurvivorRefund(state, enemyDirector, order, group) || changed;
				order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED;
				order.m_iResolvedAtSecond = state.m_iElapsedSeconds;
				order.m_sRuntimeStatus = "resolved_group_" + group.m_sRuntimeStatus;
				order.m_sResolutionKind = "physical_group_terminal";
				changed = true;
			}
		}

		return changed;
	}

	protected bool ApplySurvivorRefund(HST_CampaignState state, HST_EnemyDirectorService enemyDirector, HST_EnemyOrderState order, HST_ActiveGroupState group)
	{
		if (!state || !enemyDirector || !order || !group || order.m_bResourceRefundApplied)
			return false;
		if (HasVersionedEnemyOperation(order))
			return false;

		int survivorCount = Math.Max(0, group.m_iSurvivorInfantryCount) + Math.Max(0, group.m_iSurvivorVehicleCount);
		if (group.m_sRuntimeStatus != "folded" && survivorCount <= 0)
			return false;

		int attackRefund;
		int supportRefund;
		if (order.m_iAttackCost > 0)
			attackRefund = Math.Max(1, Math.Round(order.m_iAttackCost * 0.35));
		if (order.m_iSupportCost > 0)
			supportRefund = Math.Max(1, Math.Round(order.m_iSupportCost * 0.35));

		attackRefund = Math.Min(order.m_iAttackCost, attackRefund);
		supportRefund = Math.Min(order.m_iSupportCost, supportRefund);
		if (attackRefund <= 0 && supportRefund <= 0)
			return false;

		string refundMutationId
			= "enemy_resource_refund_" + order.m_sOperationId + "_survivor_fold_back";
		if (!enemyDirector.RefundDefenseResources(
			state,
			order.m_sFactionKey,
			order.m_sTargetZoneId,
			attackRefund,
			supportRefund,
			"survivor fold-back",
			refundMutationId,
			order.m_sOrderId,
			order.m_sOrderId,
			order.m_sOperationId,
			order.m_sManifestId))
			return false;
		order.m_sResourceRefundMutationId = refundMutationId;
		order.m_iRefundedAttackResources = attackRefund;
		order.m_iRefundedSupportResources = supportRefund;
		order.m_bResourceRefundApplied = true;
		return true;
	}

	protected bool SyncOrderCompositionFromSupportRequest(HST_EnemyOrderState order, HST_SupportRequestState request)
	{
		if (!order || !request)
			return false;

		bool changed;
		if (!request.m_sCompositionRequestId.IsEmpty() && order.m_sCompositionRequestId != request.m_sCompositionRequestId)
		{
			order.m_sCompositionRequestId = request.m_sCompositionRequestId;
			changed = true;
		}
		if (!request.m_sCompositionIntentId.IsEmpty() && order.m_sCompositionIntentId != request.m_sCompositionIntentId)
		{
			order.m_sCompositionIntentId = request.m_sCompositionIntentId;
			changed = true;
		}
		if (!request.m_sCompositionTier.IsEmpty() && order.m_sCompositionTier != request.m_sCompositionTier)
		{
			order.m_sCompositionTier = request.m_sCompositionTier;
			changed = true;
		}
		if (!request.m_sCompositionSummary.IsEmpty() && order.m_sCompositionSummary != request.m_sCompositionSummary)
		{
			order.m_sCompositionSummary = request.m_sCompositionSummary;
			changed = true;
		}
		if (!request.m_sCompositionFailureReason.IsEmpty() && order.m_sCompositionFailureReason != request.m_sCompositionFailureReason)
		{
			order.m_sCompositionFailureReason = request.m_sCompositionFailureReason;
			changed = true;
		}
		if (order.m_iCompositionCost != request.m_iCompositionCost)
		{
			order.m_iCompositionCost = request.m_iCompositionCost;
			changed = true;
		}
		if (order.m_iCompositionManpower != request.m_iCompositionManpower)
		{
			order.m_iCompositionManpower = request.m_iCompositionManpower;
			changed = true;
		}
		if (order.m_iCompositionVehicleCount != request.m_iCompositionVehicleCount)
		{
			order.m_iCompositionVehicleCount = request.m_iCompositionVehicleCount;
			changed = true;
		}
		if (order.m_iCompositionArmedVehicleCount != request.m_iCompositionArmedVehicleCount)
		{
			order.m_iCompositionArmedVehicleCount = request.m_iCompositionArmedVehicleCount;
			changed = true;
		}

		return changed;
	}

	protected HST_SupportRequestState FindSupportRequest(HST_CampaignState state, string requestId)
	{
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (request && request.m_sRequestId == requestId)
				return request;
		}

		return null;
	}

	protected bool ResolveOrders(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons)
	{
		bool changed;
		if (!state)
			return false;

		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				continue;
			if (HasVersionedEnemyOperation(order))
				continue;

			if (state.m_iElapsedSeconds < order.m_iResolveAtSecond)
				continue;

			if (order.m_bPhysicalized && !IsPhysicalizedOrderTimedOut(state, order))
				continue;

			changed = ResolveOrderNow(state, preset, garrisons, order) || changed;
		}

		return changed;
	}

	protected bool ResolveOrderNow(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_EnemyOrderState order)
	{
		if (!state || !order || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return false;
		if (HasVersionedEnemyOperation(order))
			return false;

		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED;
		order.m_iResolvedAtSecond = state.m_iElapsedSeconds;

		if (order.m_bPhysicalized)
		{
			order.m_sResolutionKind = "physical_timeout";
			order.m_sRuntimeStatus = "resolved_physical_timeout";
		}
		else
		{
			order.m_bAbstractResolved = true;
			order.m_sResolutionKind = "abstract";
			order.m_sRuntimeStatus = "resolved_abstract";
			ApplyResolvedOrder(state, preset, garrisons, order);
		}

		return true;
	}

	protected bool IsPhysicalizedOrderTimedOut(HST_CampaignState state, HST_EnemyOrderState order)
	{
		if (!state || !order)
			return true;

		int timeoutSecond = order.m_iResolveAtSecond + PHYSICAL_ORDER_TIMEOUT_SECONDS;
		return state.m_iElapsedSeconds >= timeoutSecond;
	}

	protected void ApplyResolvedOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_EnemyOrderState order)
	{
		if (!state || !order || order.m_bOutcomeApplied)
			return;
		if (HasVersionedEnemyOperation(order))
			return;

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON)
		{
			if (garrisons)
				garrisons.AddAbstractForces(state, order.m_sTargetZoneId, order.m_sFactionKey, 2 + state.m_iWarLevel, 0);

			order.m_sResolutionKind = "abstract_rebuild_garrison";
			order.m_bOutcomeApplied = true;
			return;
		}

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK)
		{
			HST_CivilianZoneState civilianZone = state.FindCivilianZone(order.m_sTargetZoneId);
			if (!civilianZone)
			{
				civilianZone = new HST_CivilianZoneState();
				civilianZone.m_sZoneId = order.m_sTargetZoneId;
				state.m_aCivilianZones.Insert(civilianZone);
			}

			civilianZone.m_iRoadblockPresence = Math.Min(3, civilianZone.m_iRoadblockPresence + 1);
			order.m_sResolutionKind = "abstract_roadblock";
			order.m_bOutcomeApplied = true;
			return;
		}

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF)
		{
			HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
			if (targetZone)
				targetZone.m_iResistanceCaptureProgress = Math.Max(0, targetZone.m_iResistanceCaptureProgress - 20);

			order.m_sResolutionKind = "abstract_qrf_pressure";
			order.m_bOutcomeApplied = true;
			return;
		}

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
		{
			ApplyAbstractCounterattack(state, preset, garrisons, order);
			order.m_bOutcomeApplied = true;
			return;
		}

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL)
		{
			ApplyAbstractSupportPressure(state, preset, order);
			order.m_bOutcomeApplied = true;
			return;
		}

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
		{
			ApplyAbstractPetrosPressure(state, preset, order);
			order.m_bOutcomeApplied = true;
			return;
		}

		order.m_sResolutionKind = "abstract_patrol_completed";
		order.m_bOutcomeApplied = true;
	}

	protected void ApplyAbstractCounterattack(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_EnemyOrderState order)
	{
		HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
		if (!targetZone)
		{
			order.m_sResolutionKind = "abstract_counterattack_missing_zone";
			return;
		}

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		HST_GarrisonState fiaGarrison = state.FindGarrison(targetZone.m_sZoneId, resistanceFactionKey);
		if (fiaGarrison && fiaGarrison.m_iInfantryCount > 0)
		{
			int casualties = Math.Max(1, state.m_iWarLevel / 2);
			fiaGarrison.m_iInfantryCount = Math.Max(0, fiaGarrison.m_iInfantryCount - casualties);
			order.m_sResolutionKind = string.Format("abstract_counterattack_fia_casualties_%1", casualties);
			return;
		}

		if (targetZone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
		{
			targetZone.m_iResistanceCaptureProgress = Math.Max(0, targetZone.m_iResistanceCaptureProgress - 8);
			order.m_sResolutionKind = "abstract_counterattack_non_town_capture_pressure";
			return;
		}

		if (!RegisterEnemyTownInfluence(
			state,
			preset,
			targetZone,
			order,
			-8,
			"enemy_counterattack",
			"abstract enemy counterattack pressure"))
		{
			order.m_sResolutionKind = "abstract_counterattack_town_influence_authority_unavailable";
			order.m_sFailureReason = "canonical town influence authority unavailable";
			return;
		}

		order.m_sResolutionKind = "abstract_counterattack_town_influence";
	}

	protected void ApplyAbstractSupportPressure(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyOrderState order)
	{
		HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
		if (!targetZone)
		{
			order.m_sResolutionKind = "abstract_support_missing_zone";
			return;
		}

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		if (targetZone.m_sOwnerFactionKey != resistanceFactionKey || targetZone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
		{
			targetZone.m_iResistanceCaptureProgress = Math.Max(0, targetZone.m_iResistanceCaptureProgress - 12);
			order.m_sResolutionKind = "abstract_support_capture_pressure";
			return;
		}

		if (!RegisterEnemyTownInfluence(
			state,
			preset,
			targetZone,
			order,
			-6,
			"enemy_support_pressure",
			"abstract enemy support pressure"))
		{
			order.m_sResolutionKind = "abstract_support_town_influence_authority_unavailable";
			order.m_sFailureReason = "canonical town influence authority unavailable";
			return;
		}

		order.m_sResolutionKind = "abstract_support_town_influence";
	}

	protected bool RegisterEnemyTownInfluence(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_ZoneState targetZone,
		HST_EnemyOrderState order,
		int fiaSupportDelta,
		string eventKind,
		string reason)
	{
		if (!state || !preset || !targetZone || !order || !m_TownInfluence
			|| targetZone.m_eType != HST_EZoneType.HST_ZONE_TOWN
			|| order.m_sOrderId.IsEmpty())
			return false;

		string eventId = string.Format(
			"town_enemy_order_%1_%2_%3",
			targetZone.m_sZoneId.Hash(),
			order.m_sOrderId.Hash(),
			eventKind.Hash());
		HST_TownInfluenceCommand command = new HST_TownInfluenceCommand();
		command.m_sCommandId = eventId;
		command.m_sEventId = eventId;
		command.m_sTownId = targetZone.m_sZoneId;
		command.m_sEventKind = eventKind;
		command.m_sSourceId = order.m_sOrderId;
		command.m_sReason = reason;
		command.m_iRawFIASupportDelta = fiaSupportDelta;
		command.m_bPopulationScaled = true;
		return m_TownInfluence.RegisterInfluenceEventExact(
			state,
			command,
			preset);
	}

	protected void ApplyAbstractPetrosPressure(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyOrderState order)
	{
		state.m_iHQKnowledge = Math.Min(100, state.m_iHQKnowledge + 10);
		state.m_iHQThreatLevel = Math.Max(state.m_iHQThreatLevel, state.m_iHQKnowledge);
		state.m_iHQKnowledgeLastChangedSecond = state.m_iElapsedSeconds;
		state.m_iLastHQActivitySecond = state.m_iElapsedSeconds;
		state.m_sLastHQKnowledgeReason = "abstract Petros pressure";
		state.m_sLastHQThreatReason = "abstract Petros pressure";
		state.m_iLastHQAttackSecond = state.m_iElapsedSeconds;
		order.m_sResolutionKind = "abstract_petros_pressure";
	}

	protected HST_ZoneState ResolvePetrosAttackTargetZone(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return null;

		vector targetPosition = state.m_vPetrosPosition;
		if (IsZeroVector(targetPosition))
			targetPosition = state.m_vHQPosition;

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		HST_ZoneState bestZone;
		float bestDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
				continue;
			if (zone.m_sOwnerFactionKey == resistanceFactionKey)
				continue;

			float distanceSq = DistanceSq2D(targetPosition, zone.m_vPosition);
			if (distanceSq < bestDistanceSq)
			{
				bestZone = zone;
				bestDistanceSq = distanceSq;
			}
		}

		if (bestZone)
			return bestZone;

		foreach (HST_ZoneState fallback : state.m_aZones)
		{
			if (!fallback)
				continue;
			if (fallback.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || fallback.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
				continue;

			float fallbackDistanceSq = DistanceSq2D(targetPosition, fallback.m_vPosition);
			if (!bestZone || fallbackDistanceSq < bestDistanceSq)
			{
				bestZone = fallback;
				bestDistanceSq = fallbackDistanceSq;
			}
		}

		return bestZone;
	}

	protected HST_ZoneState SelectTargetZone(HST_CampaignState state, HST_CampaignPreset preset, string factionKey)
	{
		HST_EnemyTargetScoreResult result = BuildTargetScoreResult(state, preset, factionKey, false);
		if (!result || !result.m_bSuccess || result.m_sSelectedZoneId.IsEmpty())
			return null;

		return state.FindZone(result.m_sSelectedZoneId);
	}

	protected HST_EEnemyOrderType SelectOrderType(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_ZoneState targetZone,
		HST_FactionPoolState pool,
		string stableDecisionSalt = "",
		int projectedThreatScore = -1)
	{
		if (!state || !preset || !targetZone || !pool)
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL;

		string ownerRelation = HST_FactionRelationService.ResolveRelation(preset, pool.m_sFactionKey, targetZone.m_sOwnerFactionKey);
		bool targetOwnedByFaction = HST_FactionRelationService.IsSameFaction(ownerRelation);
		bool targetOwnedByResistance = HST_FactionRelationService.IsResistanceEnemy(ownerRelation);
		bool targetOwnedByRival = HST_FactionRelationService.IsRivalEnemy(ownerRelation);

		bool hqThreatZone = IsHQThreatZone(state, targetZone);
		if (state.m_iHQKnowledge >= 100 && hqThreatZone && pool.m_iAttackResources >= 20 && state.m_iElapsedSeconds > state.m_iLastHQAttackSecond + 1800)
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK;

		if (state.m_iHQKnowledge >= HQ_PRESSURE_MIN_KNOWLEDGE_FOR_OPPORTUNITY_ATTACK && hqThreatZone && pool.m_iAttackResources >= 25 && state.m_iWarLevel >= 4)
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK;

		int recentThreatScore = ResolveRecentThreatScore(state, pool.m_sFactionKey, targetZone);
		if (projectedThreatScore >= 0)
			recentThreatScore = projectedThreatScore;
		string retaliationReason;
		if (targetOwnedByResistance && ShouldRetaliateWithSupport(state, targetZone, pool, HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK, recentThreatScore, retaliationReason, stableDecisionSalt))
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK;
		if (targetOwnedByRival && pool.m_iAttackResources >= 18 && ShouldRetaliateWithSupport(state, targetZone, pool, HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL, recentThreatScore, retaliationReason, stableDecisionSalt))
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL;

		if (targetOwnedByFaction && HasReactiveDefenseSignal(state, preset, pool.m_sFactionKey, targetZone, recentThreatScore) && ShouldRetaliateWithSupport(state, targetZone, pool, HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF, recentThreatScore, retaliationReason, stableDecisionSalt))
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF;

		int authoritativeGarrisonInfantry = m_GarrisonStrength.ResolveAuthoritativeZoneInfantry(
			state,
			targetZone.m_sZoneId,
			pool.m_sFactionKey,
			true);
		int desiredGarrisonInfantry = Math.Max(2, state.m_iWarLevel);
		if (targetZone.m_iGarrisonSlots > 0)
			desiredGarrisonInfantry = Math.Min(
				desiredGarrisonInfantry,
				targetZone.m_iGarrisonSlots);
		bool rebuildCapacityAvailable = targetZone.m_iGarrisonSlots <= 0
			|| authoritativeGarrisonInfantry < targetZone.m_iGarrisonSlots;
		if (targetOwnedByFaction
			&& rebuildCapacityAvailable
			&& authoritativeGarrisonInfantry < desiredGarrisonInfantry
			&& pool.m_iSupportResources >= 10)
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON;

		if (targetOwnedByFaction && targetZone.m_eType == HST_EZoneType.HST_ZONE_TOWN && pool.m_iSupportResources >= 12 && HasTownRoadblockDefenseSignal(state, preset, pool.m_sFactionKey, targetZone) && ShouldRetaliateWithSupport(state, targetZone, pool, HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK, recentThreatScore, retaliationReason, stableDecisionSalt))
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK;

		if (pool.m_iAttackResources >= 20 && state.m_iWarLevel >= 3 && ShouldRetaliateWithSupport(state, targetZone, pool, HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL, recentThreatScore, retaliationReason, stableDecisionSalt))
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL;

		return HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL;
	}

	protected void RecordTargetPressureSignal(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, string factionKey, HST_ZoneState targetZone)
	{
		if (!state || !enemyDirector || !targetZone || factionKey.IsEmpty())
			return;

		string ownerRelation = HST_FactionRelationService.ResolveRelation(preset, factionKey, targetZone.m_sOwnerFactionKey);
		if (!HST_FactionRelationService.IsSameFaction(ownerRelation))
			return;

		int damageScore = Math.Max(0, targetZone.m_iResistanceCaptureProgress / 5);
		if (HasVerifiedHostilePresenceAtZone(state, preset, factionKey, targetZone))
			damageScore += 4;
		if (HasActiveMissionNearZone(state, targetZone))
			damageScore += 3;
		if (HasActiveObjectiveNearZone(state, targetZone))
			damageScore += 3;
		if (IsTownSupportFlipThreat(state, preset, factionKey, targetZone))
			damageScore += Math.Max(1, ResolveZoneSupportPercent(state, targetZone) / 10);

		if (damageScore <= 0)
			return;

		enemyDirector.RecordZoneDamageSignal(state, factionKey, targetZone, damageScore, "target pressure signal");
	}

	protected bool HasReactiveDefenseSignal(HST_CampaignState state, HST_CampaignPreset preset, string factionKey, HST_ZoneState targetZone, int recentDamageScore)
	{
		if (!targetZone)
			return false;

		if (targetZone.m_iResistanceCaptureProgress > 0)
			return true;
		if (HasVerifiedHostilePresenceAtZone(state, preset, factionKey, targetZone))
			return true;
		if (recentDamageScore > 0)
			return true;
		if (HasActiveMissionNearZone(state, targetZone))
			return true;
		if (HasActiveObjectiveNearZone(state, targetZone))
			return true;
		if (IsTownSupportFlipThreat(state, preset, factionKey, targetZone))
			return true;

		return false;
	}

	protected bool HasTownRoadblockDefenseSignal(HST_CampaignState state, HST_CampaignPreset preset, string factionKey, HST_ZoneState targetZone)
	{
		if (!targetZone || targetZone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return false;

		if (targetZone.m_iResistanceCaptureProgress > 0)
			return true;
		if (HasVerifiedHostilePresenceAtZone(state, preset, factionKey, targetZone))
			return true;
		if (HasActiveMissionNearZone(state, targetZone))
			return true;
		if (HasActiveObjectiveNearZone(state, targetZone))
			return true;
		if (IsTownSupportFlipThreat(state, preset, factionKey, targetZone))
			return true;

		return false;
	}

	protected bool HasVerifiedHostilePresenceAtZone(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string observerFactionKey,
		HST_ZoneState zone)
	{
		if (!state || !preset || !zone || observerFactionKey.IsEmpty())
			return false;
		HST_CombatPresenceResult presence = m_CombatPresence.QueryZoneHostilePresence(
			state,
			preset,
			observerFactionKey,
			zone,
			false);
		// Unresolved authority is not verified pressure. The commander can retry on
		// the next strategic tick after the physical sampler publishes a valid
		// result, but must not turn a transient gap into damage, target score, or a
		// high-impact support choice.
		return presence && presence.m_bQueryValid && presence.m_bHasLiveContributors;
	}

	protected bool IsTownSupportFlipThreat(HST_CampaignState state, HST_CampaignPreset preset, string factionKey, HST_ZoneState targetZone)
	{
		if (!state || !preset || !targetZone || targetZone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return false;
		if (factionKey.IsEmpty() || targetZone.m_sOwnerFactionKey != factionKey)
			return false;
		if (targetZone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
			return false;

		return ResolveZoneSupportPercent(state, targetZone) >= TOWN_SUPPORT_QRF_RESPONSE_THRESHOLD;
	}

	protected int ResolveZoneSupportPercent(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return 0;
		if (zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return 0;
		if (!m_TownInfluence)
			return 0;
		return m_TownInfluence.ResolveSignedSupportPercent(state, zone.m_sZoneId);
	}

	protected bool ShouldRetaliateWithSupport(HST_CampaignState state, HST_ZoneState targetZone, HST_FactionPoolState pool, HST_EEnemyOrderType orderType, int baseThreatScore, out string reason, string stableDecisionSalt = "")
	{
		reason = "";
		if (!state || !targetZone || !pool)
		{
			reason = "retaliation context missing";
			return false;
		}

		int aggression = Math.Max(0, pool.m_iAggression);
		if (aggression <= 0)
		{
			reason = "no faction aggression";
			return false;
		}

		int threatScore = ResolveRecentThreatScore(state, pool.m_sFactionKey, targetZone, baseThreatScore);
		if (threatScore <= 0)
		{
			reason = "no recent threat signal";
			return false;
		}

		int chance = ResolveRetaliationChance(state, targetZone, aggression, threatScore, orderType);
		int roll = ResolveRetaliationRoll(state, targetZone, pool.m_sFactionKey, orderType, stableDecisionSalt);
		if (roll >= chance)
		{
			reason = string.Format("retaliation skipped | roll %1 chance %2 aggression %3 threat %4", roll, chance, aggression, threatScore);
			return false;
		}

		reason = string.Format("retaliation accepted | roll %1 chance %2 aggression %3 threat %4", roll, chance, aggression, threatScore);
		return true;
	}

	protected int ResolveRecentThreatScore(HST_CampaignState state, string factionKey, HST_ZoneState targetZone, int baseThreatScore = 0)
	{
		int threatScore = Math.Max(0, baseThreatScore);
		if (!state || factionKey.IsEmpty() || !targetZone)
			return threatScore;

		HST_EnemySupportLedgerState ledger = state.FindEnemySupportLedger(factionKey, targetZone.m_sZoneId);
		if (!ledger || ledger.m_iRecentDamageScore <= 0)
			return threatScore;

		int recentDamage = ledger.m_iRecentDamageScore;
		int age = state.m_iElapsedSeconds - ledger.m_iLastDamageSecond;
		if (age >= RECENT_THREAT_WINDOW_SECONDS)
			recentDamage = 0;
		else if (age > 0)
		{
			int remainingPercent = Math.Max(0, 100 - Math.Round(age * 100.0 / RECENT_THREAT_WINDOW_SECONDS));
			recentDamage = Math.Max(0, Math.Round(ledger.m_iRecentDamageScore * remainingPercent / 100.0));
		}

		return Math.Max(threatScore, recentDamage);
	}

	protected int ResolveRetaliationChance(HST_CampaignState state, HST_ZoneState targetZone, int aggression, int threatScore, HST_EEnemyOrderType orderType)
	{
		int chance = 8;
		chance += Math.Min(35, Math.Max(0, aggression / 2));
		chance += Math.Min(25, Math.Max(0, threatScore * 3));
		if (state)
			chance += Math.Max(0, state.m_iWarLevel - 1) * 3;
		if (targetZone)
			chance += Math.Min(10, Math.Max(0, targetZone.m_iPriority / 4));

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
			chance += 8;
		else if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK)
			chance -= 8;
		else if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL)
			chance -= 4;

		return Math.Max(5, Math.Min(85, chance));
	}

	protected int ResolveRetaliationRoll(HST_CampaignState state, HST_ZoneState targetZone, string factionKey, HST_EEnemyOrderType orderType, string stableDecisionSalt = "")
	{
		if (!state)
			return 99;

		int seed;
		if (!stableDecisionSalt.IsEmpty())
			seed = string.Format("%1|retaliation|%2", stableDecisionSalt, RetaliationOrderTypeScore(orderType)).Hash();
		else
		{
			int orderBucket = Math.Max(0, state.m_iElapsedSeconds / ORDER_TICK_SECONDS);
			seed = state.m_iCampaignSeed + orderBucket * 157 + state.m_iWarLevel * 19 + state.m_aEnemyOrders.Count() * 31;
		}
		if (targetZone)
			seed += targetZone.m_sZoneId.Length() * 101 + targetZone.m_sDisplayName.Length() * 37 + targetZone.m_iPriority * 23 + Math.Round(targetZone.m_vPosition[0]) + Math.Round(targetZone.m_vPosition[2]);
		seed += factionKey.Length() * 43 + RetaliationOrderTypeScore(orderType) * 59;
		return HST_DefaultCatalog.PositiveMod(seed, 100);
	}

	protected int RetaliationOrderTypeScore(HST_EEnemyOrderType orderType)
	{
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF)
			return 3;
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
			return 5;
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK)
			return 7;
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL)
			return 11;

		return 1;
	}

	// Returns true only when an existing commander commitment is incompatible
	// with another response at this target. Exact patrol and non-commander
	// operation roots remain candidates with a deterministic score penalty;
	// the final order-type gate still prevents patrol stacking.
	protected bool ResolveTargetCommitmentCompatibility(
		HST_CampaignState state,
		string factionKey,
		string zoneId,
		out int compatibleCommitmentCount,
		out string compatibleReason,
		out string blockingReason)
	{
		compatibleCommitmentCount = 0;
		compatibleReason = "";
		blockingReason = "";
		if (!state || factionKey.IsEmpty() || zoneId.IsEmpty())
			return false;

		ref array<string> compatibleIdentities = {};
		ref array<string> compatibleRows = {};
		ref array<string> blockingIdentities = {};
		ref array<string> blockingRows = {};

		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order || order.m_sFactionKey != factionKey
				|| !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					order.m_sTargetZoneId,
					zoneId))
				continue;
			if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				continue;

			string identity = BuildTargetCommitmentIdentity(
				"order",
				order.m_sOrderId,
				order.m_sOperationId,
				order.m_sOrderId);
			string row = string.Format(
				"order %1 type %2 status %3",
				ReportText(order.m_sOrderId),
				order.m_eType,
				order.m_eStatus);
			if (HST_OperationService.RequiresExactEnemyPatrol(order))
				InsertTargetCommitmentRow(
					compatibleIdentities,
					compatibleRows,
					identity,
					row);
			else
				InsertTargetCommitmentRow(
					blockingIdentities,
					blockingRows,
					identity,
					row);
		}

		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || request.m_sFactionKey != factionKey
				|| !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					request.m_sTargetZoneId,
					zoneId))
				continue;
			if (request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_QUEUED
				&& request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
				continue;

			HST_EnemyOrderState linkedOrder = ResolveActiveTargetOrderForCommitment(
				state,
				factionKey,
				zoneId,
				request.m_sOperationId,
				"");
			string linkedOrderId;
			if (linkedOrder)
				linkedOrderId = linkedOrder.m_sOrderId;
			string identity = BuildTargetCommitmentIdentity(
				"support",
				linkedOrderId,
				request.m_sOperationId,
				request.m_sRequestId);
			string row = string.Format(
				"support %1 type %2 status %3",
				ReportText(request.m_sRequestId),
				request.m_eType,
				request.m_eStatus);
			bool compatiblePatrol = request.m_eType
				== HST_ESupportRequestType.HST_SUPPORT_PATROL_SWEEP
				&& HST_OperationService.RequiresExactEnemyPatrol(linkedOrder);
			if (compatiblePatrol)
				InsertTargetCommitmentRow(
					compatibleIdentities,
					compatibleRows,
					identity,
					row);
			else
				InsertTargetCommitmentRow(
					blockingIdentities,
					blockingRows,
					identity,
					row);
		}

		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_sOwnerFactionKey != factionKey
				|| operation.m_eSettlementState
					!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				continue;
			if (operation.m_eTerminalResult
					!= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN
				&& operation.m_eTerminalResult
					!= HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
				continue;

			bool assignmentMatches
				= !operation.m_sAssignmentZoneId.IsEmpty()
				&& HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					operation.m_sAssignmentZoneId,
					zoneId);
			bool tacticalMatches
				= !operation.m_sTacticalTargetZoneId.IsEmpty()
				&& HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					operation.m_sTacticalTargetZoneId,
					zoneId);
			if (!assignmentMatches && !tacticalMatches)
				continue;
			HST_EnemyOrderState linkedOrder = ResolveActiveTargetOrderForCommitment(
				state,
				factionKey,
				zoneId,
				operation.m_sOperationId,
				operation.m_sEnemyOrderId);
			string linkedOrderId = operation.m_sEnemyOrderId;
			if (linkedOrder)
				linkedOrderId = linkedOrder.m_sOrderId;
			string identity = BuildTargetCommitmentIdentity(
				"operation",
				linkedOrderId,
				operation.m_sOperationId,
				operation.m_sEnemyOrderId);
			string row = string.Format(
				"operation %1 type %2 contract %3",
				ReportText(operation.m_sOperationId),
				operation.m_eType,
				operation.m_iContractVersion);
			bool compatiblePatrol = linkedOrder
				&& HST_OperationService.RequiresExactEnemyPatrol(linkedOrder)
				&& operation.m_eType
				== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL
				&& operation.m_iContractVersion
					== HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION;
			HST_EnemyOrderState settledLinkedOrder = linkedOrder;
			if (!settledLinkedOrder && !operation.m_sEnemyOrderId.IsEmpty())
				settledLinkedOrder = state.FindEnemyOrder(operation.m_sEnemyOrderId);
			bool compatibleDeliveredGarrisonRebuild = settledLinkedOrder
				&& HST_OperationService.RequiresExactEnemyGarrisonRebuild(settledLinkedOrder)
				&& settledLinkedOrder.m_eStatus
					== HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED
				&& operation.m_eType
					== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_GARRISON_REBUILD
				&& operation.m_eDutyState
					== HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
			bool commanderOperation = operation.m_eType
				== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL
				|| operation.m_eType
					== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF
				|| operation.m_eType
					== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK
				|| operation.m_eType
					== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_GARRISON_REBUILD;
			bool compatibleIndependentOperation = !commanderOperation
				&& operation.m_sEnemyOrderId.IsEmpty();
			if (compatiblePatrol || compatibleDeliveredGarrisonRebuild
				|| compatibleIndependentOperation)
				InsertTargetCommitmentRow(
					compatibleIdentities,
					compatibleRows,
					identity,
					row);
			else
				InsertTargetCommitmentRow(
					blockingIdentities,
					blockingRows,
					identity,
					row);
		}

		// A malformed or partially linked root can expose both a compatible row
		// and a blocking row. Collapse it once with conservative blocking
		// precedence instead of counting the same root in both classifications.
		int compatibleIndex = compatibleIdentities.Count() - 1;
		while (compatibleIndex >= 0)
		{
			if (!blockingIdentities.Contains(
				compatibleIdentities[compatibleIndex]))
			{
				compatibleIndex--;
				continue;
			}
			compatibleIdentities.Remove(compatibleIndex);
			compatibleRows.Remove(compatibleIndex);
			compatibleIndex--;
		}
		compatibleRows.Sort();
		blockingRows.Sort();
		compatibleCommitmentCount = compatibleIdentities.Count();
		if (compatibleCommitmentCount > 0)
			compatibleReason = string.Format(
				"compatible commitments %1 | first %2",
				compatibleCommitmentCount,
				compatibleRows[0]);
		if (blockingRows.IsEmpty())
			return false;

		blockingReason = string.Format(
			"incompatible commitments %1 | first %2",
			blockingIdentities.Count(),
			blockingRows[0]);
		return true;
	}

	protected string BuildTargetCommitmentIdentity(
		string family,
		string linkedOrderId,
		string operationId,
		string fallbackId)
	{
		if (!linkedOrderId.IsEmpty())
			return "order:" + linkedOrderId;
		if (!operationId.IsEmpty())
			return "operation:" + operationId;
		return family + ":" + fallbackId;
	}

	protected HST_EnemyOrderState ResolveActiveTargetOrderForCommitment(
		HST_CampaignState state,
		string factionKey,
		string zoneId,
		string operationId,
		string orderId)
	{
		if (!state)
			return null;

		if (!orderId.IsEmpty())
		{
			HST_EnemyOrderState explicitOrder = state.FindEnemyOrder(orderId);
			if (IsActiveTargetOrderForCommitment(
				explicitOrder,
				factionKey,
				zoneId,
				operationId))
				return explicitOrder;
			return null;
		}

		if (operationId.IsEmpty())
			return null;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (IsActiveTargetOrderForCommitment(
				order,
				factionKey,
				zoneId,
				operationId))
				return order;
		}
		return null;
	}

	protected bool IsActiveTargetOrderForCommitment(
		HST_EnemyOrderState order,
		string factionKey,
		string zoneId,
		string operationId)
	{
		if (!order || order.m_sFactionKey != factionKey
			|| !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
				order.m_sTargetZoneId,
				zoneId))
			return false;
		if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
			&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return false;
		return operationId.IsEmpty() || order.m_sOperationId == operationId;
	}

	protected void InsertTargetCommitmentRow(
		notnull array<string> identities,
		notnull array<string> rows,
		string identity,
		string row)
	{
		int existingIndex = identities.Find(identity);
		if (existingIndex >= 0)
		{
			if (row.Compare(rows[existingIndex]) < 0)
				rows[existingIndex] = row;
			return;
		}

		identities.Insert(identity);
		rows.Insert(row);
	}

	protected int ScoreTargetZone(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState zone, string factionKey)
	{
		HST_EnemyTargetScoreCandidate candidate = BuildTargetScoreCandidate(state, preset, zone, factionKey);
		if (!candidate)
			return -9999;

		return candidate.m_iScore;
	}

	protected HST_EnemyTargetScoreCandidate BuildTargetScoreCandidate(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState zone, string factionKey)
	{
		if (!state || !preset || !zone || factionKey.IsEmpty())
			return null;

		string ineligibleReason;
		if (!IsEligibleTargetZone(zone, ineligibleReason))
			return null;

		string localityReason;
		if (!IsLocalOperationTargetAllowed(state, preset, factionKey, zone, localityReason))
			return null;

		int compatibleCommitmentCount;
		string compatibleCommitmentReason;
		string blockingCommitmentReason;
		if (ResolveTargetCommitmentCompatibility(
			state,
			factionKey,
			zone.m_sZoneId,
			compatibleCommitmentCount,
			compatibleCommitmentReason,
			blockingCommitmentReason))
			return null;

		return BuildTargetScoreCandidateResolved(
			state,
			preset,
			zone,
			factionKey,
			compatibleCommitmentCount,
			compatibleCommitmentReason,
			localityReason);
	}

	protected HST_EnemyTargetScoreCandidate BuildTargetScoreCandidateResolved(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_ZoneState zone,
		string factionKey,
		int compatibleCommitmentCount,
		string compatibleCommitmentReason,
		string localityReason)
	{
		if (!state || !preset || !zone || factionKey.IsEmpty())
			return null;

		HST_EnemyTargetScoreCandidate candidate = new HST_EnemyTargetScoreCandidate();
		candidate.m_sZoneId = zone.m_sZoneId;
		candidate.m_sDisplayName = zone.m_sDisplayName;
		candidate.m_sOwnerFactionKey = zone.m_sOwnerFactionKey;
		candidate.m_sOwnerRelation = HST_FactionRelationService.ResolveRelation(preset, factionKey, zone.m_sOwnerFactionKey);
		candidate.m_eType = zone.m_eType;
		candidate.m_sLocalityReason = localityReason;
		candidate.m_iCompatibleCommitmentCount = Math.Max(
			0,
			compatibleCommitmentCount);
		candidate.m_sCommitmentReason = compatibleCommitmentReason;
		if (candidate.m_iCompatibleCommitmentCount > 0)
		{
			candidate.m_iCommitmentScore = -Math.Min(
				24,
				candidate.m_iCompatibleCommitmentCount * 12);
			AddTargetScorePenalty(
				candidate,
				"existing_commitment",
				candidate.m_iCommitmentScore);
		}

		if (HST_FactionRelationService.IsResistanceEnemy(candidate.m_sOwnerRelation))
		{
			candidate.m_iOwnerScore = 24;
			AddTargetScoreReason(candidate, "resistance_control", candidate.m_iOwnerScore);
		}
		else if (HST_FactionRelationService.IsSameFaction(candidate.m_sOwnerRelation))
		{
			candidate.m_iOwnerScore = 12;
			AddTargetScoreReason(candidate, "owned_zone_pressure", candidate.m_iOwnerScore);
		}
		else if (HST_FactionRelationService.IsRivalEnemy(candidate.m_sOwnerRelation))
		{
			candidate.m_iOwnerScore = 9;
			AddTargetScoreReason(candidate, "rival_enemy_pressure", candidate.m_iOwnerScore);
		}
		else
		{
			candidate.m_iOwnerScore = 2;
			AddTargetScoreReason(candidate, "neutral_pressure", candidate.m_iOwnerScore);
		}

		candidate.m_iPriorityScore = Math.Min(35, Math.Max(0, zone.m_iPriority));
		AddTargetScoreReason(candidate, "priority", candidate.m_iPriorityScore);

		candidate.m_iIncomeScore = Math.Min(12, Math.Max(0, zone.m_iIncomeValue / 8));
		AddTargetScoreReason(candidate, "income", candidate.m_iIncomeScore);

		candidate.m_iProgressScore = Math.Min(24, Math.Max(0, zone.m_iResistanceCaptureProgress / 4));
		AddTargetScoreReason(candidate, "capture_progress", candidate.m_iProgressScore);

		if (HasVerifiedHostilePresenceAtZone(state, preset, factionKey, zone))
		{
			candidate.m_iActivityScore = 12;
			AddTargetScoreReason(candidate, "combat_presence", candidate.m_iActivityScore);
		}

		int localSupport = ResolveZoneSupportPercent(state, zone);
		if (localSupport > 25)
		{
			candidate.m_iSupportScore = Math.Min(10, 4 + localSupport / 20);
			AddTargetScoreReason(candidate, "local_support", candidate.m_iSupportScore);
		}

		if (HasActiveMissionNearZone(state, zone))
		{
			candidate.m_iMissionScore = 10;
			AddTargetScoreReason(candidate, "active_mission", candidate.m_iMissionScore);
		}

		if (HasActiveObjectiveNearZone(state, zone))
		{
			candidate.m_iObjectiveScore = 8;
			AddTargetScoreReason(candidate, "active_objective", candidate.m_iObjectiveScore);
		}

		candidate.m_iStrategicScore = StrategicTargetTypeScore(zone.m_eType);
		AddTargetScoreReason(candidate, "strategic_type", candidate.m_iStrategicScore);

		if (state.m_iHQKnowledge > 0 && IsHQThreatZone(state, zone))
		{
			candidate.m_iHQScore = 10 + state.m_iWarLevel;
			candidate.m_iHQScore += state.m_iHQKnowledge / 10;
			AddTargetScoreReason(candidate, "hq_pressure", candidate.m_iHQScore);
		}

		HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, factionKey);
		if (!garrison)
		{
			candidate.m_iGarrisonScore = 6;
			AddTargetScoreReason(candidate, "missing_garrison", candidate.m_iGarrisonScore);
		}
		else if (garrison.m_iInfantryCount < Math.Max(2, state.m_iWarLevel))
		{
			candidate.m_iGarrisonScore = 5;
			AddTargetScoreReason(candidate, "weak_garrison", candidate.m_iGarrisonScore);
		}

		HST_EnemySupportLedgerState ledger = state.FindEnemySupportLedger(factionKey, zone.m_sZoneId);
		if (ledger && ledger.m_iRecentDamageScore > 0)
		{
			candidate.m_iDamageScore = Math.Min(18, ledger.m_iRecentDamageScore);
			AddTargetScoreReason(candidate, "recent_damage", candidate.m_iDamageScore);
		}

		candidate.m_iScore = candidate.m_iOwnerScore
			+ candidate.m_iPriorityScore
			+ candidate.m_iProgressScore
			+ candidate.m_iActivityScore
			+ candidate.m_iSupportScore
			+ candidate.m_iMissionScore
			+ candidate.m_iObjectiveScore
			+ candidate.m_iStrategicScore
			+ candidate.m_iHQScore
			+ candidate.m_iGarrisonScore
			+ candidate.m_iDamageScore
			+ candidate.m_iIncomeScore
			+ candidate.m_iCommitmentScore;

		return candidate;
	}

	protected bool IsEligibleTargetZone(HST_ZoneState zone, out string reason)
	{
		reason = "";
		if (!zone)
		{
			reason = "missing zone";
			return false;
		}
		if (zone.m_sZoneId.IsEmpty())
		{
			reason = "missing zone id";
			return false;
		}
		if (zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT)
		{
			reason = "hideout bookkeeping anchor";
			return false;
		}
		if (zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
		{
			reason = "mission-site bookkeeping anchor";
			return false;
		}

		return true;
	}

	protected HST_ZoneState FindNearestLocalOperationFoothold(HST_CampaignState state, string factionKey, HST_ZoneState targetZone, out float nearestDistanceSq)
	{
		nearestDistanceSq = 999999999.0;
		if (!state)
			return null;
		if (factionKey.IsEmpty())
			return null;
		if (!targetZone)
			return null;

		HST_ZoneState nearestFoothold;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sZoneId == targetZone.m_sZoneId)
				continue;
			if (zone.m_sOwnerFactionKey != factionKey)
				continue;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
				continue;

			float distanceSq = DistanceSq2D(zone.m_vPosition, targetZone.m_vPosition);
			if (!nearestFoothold || distanceSq < nearestDistanceSq)
			{
				nearestFoothold = zone;
				nearestDistanceSq = distanceSq;
			}
		}

		return nearestFoothold;
	}

	protected bool AreOperationalZonesLinked(HST_ZoneState firstZone, HST_ZoneState secondZone)
	{
		if (!firstZone || !secondZone)
			return false;
		if (firstZone.m_sZoneId.IsEmpty() || secondZone.m_sZoneId.IsEmpty())
			return false;

		if (firstZone.m_aLinkedZoneIds && firstZone.m_aLinkedZoneIds.Contains(secondZone.m_sZoneId))
			return true;
		if (secondZone.m_aLinkedZoneIds && secondZone.m_aLinkedZoneIds.Contains(firstZone.m_sZoneId))
			return true;

		return false;
	}

	protected int StrategicTargetTypeScore(HST_EZoneType zoneType)
	{
		if (zoneType == HST_EZoneType.HST_ZONE_AIRFIELD)
			return 16;
		if (zoneType == HST_EZoneType.HST_ZONE_SEAPORT)
			return 12;
		if (zoneType == HST_EZoneType.HST_ZONE_FACTORY)
			return 10;
		if (zoneType == HST_EZoneType.HST_ZONE_RESOURCE)
			return 8;
		if (zoneType == HST_EZoneType.HST_ZONE_BANK)
			return 7;
		if (zoneType == HST_EZoneType.HST_ZONE_OUTPOST || zoneType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return 6;
		if (zoneType == HST_EZoneType.HST_ZONE_POLICE_STATION)
			return 5;
		if (zoneType == HST_EZoneType.HST_ZONE_TOWN)
			return 4;

		return 0;
	}

	protected void AddTargetScoreReason(HST_EnemyTargetScoreCandidate candidate, string label, int score)
	{
		if (!candidate || score <= 0)
			return;

		string part = string.Format("%1 +%2", label, score);
		if (candidate.m_sReason.IsEmpty())
			candidate.m_sReason = part;
		else
			candidate.m_sReason = candidate.m_sReason + ", " + part;
	}

	protected void AddTargetScorePenalty(HST_EnemyTargetScoreCandidate candidate, string label, int score)
	{
		if (!candidate || score >= 0)
			return;

		string part = string.Format("%1 %2", label, score);
		if (candidate.m_sReason.IsEmpty())
			candidate.m_sReason = part;
		else
			candidate.m_sReason = candidate.m_sReason + ", " + part;
	}

	protected bool HasActiveMissionNearZone(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return false;

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			if (HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
				mission.m_sTargetZoneId, zone.m_sZoneId))
				return true;
			if (DistanceSq2D(mission.m_vTargetPosition, zone.m_vPosition) < 450000)
				return true;
		}

		return false;
	}

	protected bool HasActiveObjectiveNearZone(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return false;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_bComplete || objective.m_bFailed)
				continue;

			HST_ActiveMissionState mission = state.FindActiveMission(objective.m_sMissionInstanceId);
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			if (HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
				objective.m_sTargetZoneId, zone.m_sZoneId))
				return true;
			if (DistanceSq2D(objective.m_vPosition, zone.m_vPosition) < 450000)
				return true;
		}

		return false;
	}

	protected bool IsPetrosAttackTargetInsidePlayerEventBubble(HST_CampaignState state)
	{
		if (!state)
			return false;

		vector target = ResolvePetrosAttackTargetPosition(state);
		if (IsZeroVector(target))
			return false;

		return HST_WorldPositionService.IsPositionInsidePlayerEventBubble(target);
	}

	protected vector ResolvePetrosAttackTargetPosition(HST_CampaignState state)
	{
		if (!state)
			return "0 0 0";

		vector target = state.m_vPetrosPosition;
		if (IsZeroVector(target))
			target = state.m_vHQPosition;

		return target;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected string ReportText(string value)
	{
		if (value.IsEmpty())
			return "none";

		return value;
	}

	protected bool IsHQThreatZone(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone || !state.m_bHQDeployed)
			return false;

		return DistanceSq2D(state.m_vHQPosition, zone.m_vPosition) <= HQ_PRESSURE_ZONE_RADIUS_METERS * HQ_PRESSURE_ZONE_RADIUS_METERS;
	}

	protected vector ResolveOrderSourcePosition(HST_CampaignState state, HST_CampaignPreset preset, string factionKey, HST_ZoneState targetZone)
	{
		HST_ZoneState bestZone = ResolveOrderSourceZone(state, factionKey, targetZone);
		if (bestZone)
			return bestZone.m_vPosition;
		if (!targetZone)
			return "0 0 0";
		return targetZone.m_vPosition;
	}

	protected HST_ZoneState ResolveOrderSourceZone(HST_CampaignState state, string factionKey, HST_ZoneState targetZone)
	{
		if (!state || !targetZone || factionKey.IsEmpty())
			return null;

		ref array<string> sourceZoneIds = {};
		BuildCanonicalSourceZoneIds(state, factionKey, targetZone, sourceZoneIds);
		HST_ZoneState bestZone;
		float bestDistanceSq = 999999999.0;
		foreach (string sourceZoneId : sourceZoneIds)
		{
			HST_ZoneState zone = state.FindZone(sourceZoneId);
			if (!zone)
				continue;
			float distanceSq = DistanceSq2D(zone.m_vPosition, targetZone.m_vPosition);
			if (bestZone && distanceSq > bestDistanceSq)
				continue;
			if (bestZone && Math.AbsFloat(distanceSq - bestDistanceSq) < 0.01
				&& zone.m_sZoneId.Compare(bestZone.m_sZoneId) >= 0)
				continue;
			bestZone = zone;
			bestDistanceSq = distanceSq;
		}
		return bestZone;
	}

	protected void BuildCanonicalSourceZoneIds(
		HST_CampaignState state,
		string factionKey,
		HST_ZoneState targetZone,
		notnull array<string> sourceZoneIds)
	{
		sourceZoneIds.Clear();
		if (!state || !targetZone || factionKey.IsEmpty())
			return;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sZoneId == targetZone.m_sZoneId
				|| zone.m_sOwnerFactionKey != factionKey)
				continue;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT
				|| zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE
				|| IsZeroVector(zone.m_vPosition))
				continue;
			sourceZoneIds.Insert(zone.m_sZoneId);
		}
		sourceZoneIds.Sort();
	}

	protected bool HasActiveLegacyEnemyQRFSupport(HST_CampaignState state, string factionKey, string targetZoneId)
	{
		if (!state || factionKey.IsEmpty() || targetZoneId.IsEmpty())
			return false;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || request.m_bPlayerRequested || request.m_eType != HST_ESupportRequestType.HST_SUPPORT_QRF
				|| request.m_sFactionKey != factionKey
				|| !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					request.m_sTargetZoneId, targetZoneId))
				continue;
			if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED
				|| request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
				return true;
		}
		return false;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}

	protected HST_ESupportRequestType SupportTypeForOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState targetZone, HST_EEnemyOrderType orderType, string stableDecisionSalt = "")
	{
		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
			return HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF || orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
			return HST_ESupportRequestType.HST_SUPPORT_QRF;

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL)
		{
			int roll = ResolveSupportTypeRoll(state, targetZone, orderType, stableDecisionSalt);
			bool verifiedPressure = targetZone
				&& HasVerifiedHostilePresenceAtZone(
					state,
					preset,
					targetZone.m_sOwnerFactionKey,
					targetZone);
			if (state && state.m_iWarLevel >= 6 && targetZone
				&& (targetZone.m_sOwnerFactionKey == resistanceFactionKey || verifiedPressure)
				&& roll < 20)
				return HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55;

			if (state && state.m_iWarLevel >= 3 && roll < 70)
				return HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK;

			return HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;
		}

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK)
			return HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK;

		return HST_ESupportRequestType.HST_SUPPORT_PATROL_SWEEP;
	}

	protected int ResolveSupportTypeRoll(HST_CampaignState state, HST_ZoneState targetZone, HST_EEnemyOrderType orderType, string stableDecisionSalt = "")
	{
		if (!state)
			return 0;

		int zoneHash;
		if (targetZone)
			zoneHash = targetZone.m_sZoneId.Length() * 31 + targetZone.m_iPriority * 17 + targetZone.m_iResistanceCaptureProgress * 7;
		int orderTypeScore = 1;
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL)
			orderTypeScore = 5;
		else if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
			orderTypeScore = 9;
		int seed;
		if (!stableDecisionSalt.IsEmpty())
			seed = string.Format("%1|support_type|%2|%3", stableDecisionSalt, zoneHash, orderTypeScore).Hash();
		else
			seed = state.m_iCampaignSeed + state.m_iElapsedSeconds * 3 + state.m_aEnemyOrders.Count() * 41 + state.m_iWarLevel * 19 + zoneHash + orderTypeScore * 11;
		return HST_DefaultCatalog.PositiveMod(seed, 100);
	}

	protected void ResolveOrderCosts(HST_EEnemyOrderType orderType, out int attackCost, out int supportCost)
	{
		attackCost = 8;
		supportCost = 4;

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF || orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
		{
			attackCost = 15;
			supportCost = 5;
			return;
		}

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL || orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
		{
			attackCost = 20;
			supportCost = 8;
			return;
		}

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON)
		{
			attackCost = 0;
			supportCost = 10;
		}
	}

	protected string ResolveOrderSpendMode(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState targetZone, HST_EEnemyOrderType orderType, string requestedMode)
	{
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON)
			return SPEND_MODE_REACTIVE_DEFENSE;
		if (!requestedMode.IsEmpty())
			return requestedMode;

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF
			|| orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON
			|| orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK)
			return SPEND_MODE_REACTIVE_DEFENSE;

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
		{
			string resistanceFactionKey = "FIA";
			if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
				resistanceFactionKey = preset.m_sResistanceFactionKey;
			if (targetZone && targetZone.m_sOwnerFactionKey == resistanceFactionKey)
				return SPEND_MODE_PROACTIVE_ATTACK;

			return SPEND_MODE_REACTIVE_DEFENSE;
		}

		return SPEND_MODE_PROACTIVE_ATTACK;
	}

	protected void ResolveOrderCostsForSpendMode(HST_EEnemyOrderType orderType, string spendMode, out int attackCost, out int supportCost)
	{
		ResolveOrderCosts(orderType, attackCost, supportCost);
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON)
			return;
		if (spendMode == SPEND_MODE_PROACTIVE_ATTACK)
		{
			supportCost = 0;
			return;
		}

		if (spendMode == SPEND_MODE_REACTIVE_DEFENSE)
			attackCost = 0;
	}

	protected bool HasActiveOrderForZone(
		HST_CampaignState state,
		string factionKey,
		string zoneId,
		bool ignoreExactPatrol = false)
	{
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order || order.m_sFactionKey != factionKey
				|| !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					order.m_sTargetZoneId, zoneId))
				continue;
			if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				continue;
			if (ignoreExactPatrol && HST_OperationService.RequiresExactEnemyPatrol(order))
				continue;
			return true;
		}

		return false;
	}

	protected HST_EnemyOrderState FindOrderForDebug(HST_CampaignState state, string orderId)
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
}
