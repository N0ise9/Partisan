#ifdef ENABLE_DIAG
class HST_EnemyCounterattackOperationProofReport
{
	bool m_bFrozenPlanningExact;
	bool m_bAdmissionExact;
	bool m_bVirtualTravelExact;
	bool m_bVirtualCombatExact;
	bool m_bPhysicalHandoffExact;
	bool m_bOwnershipRetryExact;
	bool m_bSettlementReplayExact;
	bool m_bSupportSettlementExact;
	bool m_bRestoreLifecycleExact;
	bool m_bResourceAuthorityQuarantineExact;
	bool m_bAmbiguityHoldExact;
	bool m_bOwnershipCorrelationQuarantineExact;
	bool m_bSchema69QuarantineExact;
	bool m_bQuarantineRetentionExact;
	string m_sPlanningEvidence;
	string m_sAdmissionEvidence;
	string m_sTravelEvidence;
	string m_sCombatEvidence;
	string m_sPhysicalHandoffEvidence;
	string m_sOwnershipEvidence;
	string m_sSettlementEvidence;
	string m_sSupportSettlementEvidence;
	string m_sRestoreEvidence;
	string m_sResourceAuthorityEvidence;
	string m_sAmbiguityEvidence;
	string m_sOwnershipCorrelationEvidence;
	string m_sQuarantineEvidence;
	string m_sRetentionEvidence;

	bool AllExact()
	{
		bool planningExact = m_bFrozenPlanningExact
			&& m_bAdmissionExact && m_bVirtualTravelExact;
		bool combatExact = m_bVirtualCombatExact
			&& m_bPhysicalHandoffExact && m_bOwnershipRetryExact;
		bool settlementExact = m_bSettlementReplayExact
			&& m_bSupportSettlementExact && m_bRestoreLifecycleExact;
		bool quarantineExact = m_bOwnershipCorrelationQuarantineExact
			&& m_bSchema69QuarantineExact
			&& m_bQuarantineRetentionExact;
		bool authorityExact = m_bResourceAuthorityQuarantineExact
			&& m_bAmbiguityHoldExact && quarantineExact;
		return planningExact && combatExact && settlementExact && authorityExact;
	}

	string BuildReport()
	{
		string report = string.Format(
			"exact enemy counterattack proof | all exact %1 | planning %2 | admission %3 | travel %4 | combat %5",
			AllExact(),
			m_bFrozenPlanningExact,
			m_bAdmissionExact,
			m_bVirtualTravelExact,
			m_bVirtualCombatExact);
		report = report + string.Format(
			" | physical handoff %1 | ownership retry %2 | attack settlement %3 | support settlement %4 | schema-69 quarantine %5",
			m_bPhysicalHandoffExact,
			m_bOwnershipRetryExact,
			m_bSettlementReplayExact,
			m_bSupportSettlementExact,
			m_bSchema69QuarantineExact);
		return report + string.Format(
			" | restore lifecycle %1 | resource authority %2 | ambiguity hold %3 | ownership correlation %4 | quarantine retention %5",
			m_bRestoreLifecycleExact,
			m_bResourceAuthorityQuarantineExact,
			m_bAmbiguityHoldExact,
			m_bOwnershipCorrelationQuarantineExact,
			m_bQuarantineRetentionExact);
	}
}

// Source-only proximity seam. All lifecycle decisions still pass through the
// production counterattack service, while the focused proof remains independent
// of a native player entity or a particular autotest world location.
class HST_EnemyCounterattackMaterializationProofHarness : HST_MaterializationService
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

class HST_EnemyCounterattackOperationProofHarness : HST_EnemyCounterattackOperationService
{
	void UseDeterministicVirtualProjectionForProof()
	{
		m_Materialization = new HST_EnemyCounterattackMaterializationProofHarness();
	}

	bool BeginMaterializationForProof(
		HST_CampaignState state,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyOrderState order,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string reason)
	{
		return BeginMaterialization(
			state,
			enemyDirector,
			order,
			operation,
			manifest,
			batch,
			group,
			reason);
	}
}

// One fail-before-mutation response proves that the operation remains on
// station and retries the same canonical command. Every later attempt delegates
// to the production ownership transaction, including its replay fingerprint.
class HST_EnemyCounterattackOwnershipRetryProofHarness : HST_OwnershipTransitionService
{
	protected int m_iInjectedRetriesRemaining;
	protected int m_iInjectedRetryCount;
	protected int m_iOwnerAppliedPendingRemaining;
	protected int m_iOwnerAppliedPendingCount;
	protected int m_iProductionApplyCount;

	void InjectRetryCount(int retryCount)
	{
		m_iInjectedRetriesRemaining = Math.Max(0, retryCount);
	}

	int InjectedRetryCount()
	{
		return m_iInjectedRetryCount;
	}

	void InjectOwnerAppliedPendingCount(int pendingCount)
	{
		m_iOwnerAppliedPendingRemaining = Math.Max(0, pendingCount);
	}

	int OwnerAppliedPendingCount()
	{
		return m_iOwnerAppliedPendingCount;
	}

	int ProductionApplyCount()
	{
		return m_iProductionApplyCount;
	}

	HST_OwnershipTransitionState AdmitPreOwnerPendingForProof(
		HST_CampaignState state,
		HST_OwnershipTransitionRequest request)
	{
		if (!state || !request || !ValidateNewRequest(state, request).IsEmpty()
			|| !EnsureAdmissionCapacity(state))
			return null;
		HST_ZoneState zone = state.FindZone(request.m_sZoneId);
		HST_OwnershipTransitionState transition = CreateTransition(
			state,
			zone,
			request);
		if (!transition || transition.m_sStrategicEventId.IsEmpty())
			return null;
		state.m_aOwnershipTransitions.Insert(transition);
		zone.m_sActiveOwnershipTransitionRequestId = transition.m_sRequestId;
		transition.m_iAttemptCount = 1;
		transition.m_iLastAttemptAtSecond = state.m_iElapsedSeconds;
		transition.m_sStatus = "applying";
		return transition;
	}

	string MissingDependenciesForProof()
	{
		string missing;
		if (!m_Preset) missing = missing + " preset";
		if (!m_Balance) missing = missing + " balance";
		if (!m_Economy) missing = missing + " economy";
		if (!m_Strategic) missing = missing + " strategic";
		if (!m_Garrisons) missing = missing + " garrisons";
		if (!m_Civilians) missing = missing + " civilians";
		if (!m_TownInfluence) missing = missing + " town_influence";
		if (!m_CampaignEvents) missing = missing + " campaign_events";
		if (!m_PhysicalWar) missing = missing + " physical_war";
		if (!m_LocalSecurityPatrols) missing = missing + " local_security";
		if (!m_GarrisonPatrols) missing = missing + " garrison_patrols";
		if (!m_EnemyGarrisonRebuilds) missing = missing + " enemy_garrison_rebuilds";
		if (!m_ZoneCapture) missing = missing + " zone_capture";
		if (!m_MapMarkers) missing = missing + " map_markers";
		if (!m_Persistence) missing = missing + " persistence";
		return missing.Trim();
	}

	override HST_OwnershipTransitionResult Apply(
		HST_CampaignState state,
		HST_OwnershipTransitionRequest request)
	{
		if (m_iInjectedRetriesRemaining > 0)
		{
			m_iInjectedRetriesRemaining--;
			m_iInjectedRetryCount++;
			HST_OwnershipTransitionResult retry = new HST_OwnershipTransitionResult();
			retry.m_bAccepted = true;
			retry.m_bNeedsRetry = true;
			retry.m_sFailureReason = "focused proof injected a pre-mutation ownership retry";
			return retry;
		}

		m_iProductionApplyCount++;
		HST_OwnershipTransitionResult production = super.Apply(state, request);
		if (m_iOwnerAppliedPendingRemaining <= 0 || !production
			|| !production.m_bCompleted || !production.m_Transition || !production.m_Zone)
			return production;

		m_iOwnerAppliedPendingRemaining--;
		m_iOwnerAppliedPendingCount++;
		production.m_Transition.m_bCompleted = false;
		production.m_Transition.m_sStatus = "projecting";
		production.m_Transition.m_iCompletedAtSecond = 0;
		production.m_Zone.m_sActiveOwnershipTransitionRequestId
			= production.m_Transition.m_sRequestId;
		production.m_Zone.m_sLastOwnershipTransitionRequestId = "";
		production.m_bCompleted = false;
		production.m_bAlreadyApplied = false;
		production.m_bNeedsRetry = true;
		production.m_sFailureReason
			= "focused proof paused the exact receipt after its visible owner effect";
		return production;
	}
}

class HST_EnemyCounterattackOperationProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_BalanceConfig m_Balance;
	ref HST_ForcePlanningService m_Planning;
	ref HST_ForceSpawnQueueService m_Queue;
	ref HST_ForceSpawnAdapterService m_Adapter;
	ref HST_PhysicalWarService m_PhysicalWar;
	ref HST_CombatPresenceService m_CombatPresence;
	ref HST_EnemyDirectorService m_EnemyDirector;
	ref HST_EnemyCounterattackOperationProofHarness m_Service;
	ref HST_EnemyCounterattackOwnershipRetryProofHarness m_Ownership;
	ref HST_OwnershipTransitionProofFixture m_OwnershipFixture;
	ref HST_GarrisonPatrolOperationProofFixture m_DefenderFixture;
	ref HST_GarrisonPatrolOperationService m_GarrisonPatrols;
	ref HST_LocalSecurityOperationService m_LocalSecurity;
	ref HST_EnemyGarrisonRebuildOperationService m_EnemyGarrisonRebuilds;
	ref HST_EnemyOrderState m_Order;
	ref HST_ForceManifestState m_Manifest;
	ref HST_EnemyCounterattackAdmissionResult m_Admission;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	string m_sDebitReason;
	int m_iAttackBeforeDebit;
	int m_iSupportBeforeDebit;
	int m_iAttackAfterDebit;
	int m_iSupportAfterDebit;
	bool m_bPreflightAccepted;
	bool m_bDebitAccepted;
	string m_sFailureReason;
}

class HST_EnemyCounterattackExternalVirtualCutPolicy
{
	bool m_bDematerializingCut;
	bool m_bAllowZeroRouteProgress;
	bool m_bRequireBatchTerminalReason;
	bool m_bRequirePriorBatchAttempt;
	bool m_bExpectedRootEverAlive;
	int m_iExpectedSuccessfulHandoffCount;
	vector m_vExpectedRouteStart;
}

class HST_EnemyCounterattackExternalVirtualProjectionProof
{
	bool m_bManifestExact;
	bool m_bBatchExact;
	bool m_bGroupIdentityExact;
	bool m_bGroupRuntimeExact;
	bool m_bProjectionExact;
	int m_iLivingMemberCount;
	string m_sBatchEvidence;
	string m_sGroupEvidence;
	string m_sCasualtyEvidence;
}

class HST_EnemyCounterattackOperationProofFixtureFactory
{
	static const string PROOF_FACTION_KEY = "US";
	static const int PROOF_ATTACK_COST = 24;
	static const int PROOF_SUPPORT_COST = 18;

	HST_EnemyCounterattackOperationProofFixture BuildAdmittedFixture(
		string suffix,
		bool supportFunded = false)
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= new HST_EnemyCounterattackOperationProofFixture();
		HST_GarrisonPatrolOperationProofFixtureFactory defenderFactory
			= new HST_GarrisonPatrolOperationProofFixtureFactory();
		fixture.m_DefenderFixture = defenderFactory.BuildAcceptedFixture(
			"enemy_counterattack_" + suffix,
			1);
		if (!defenderFactory.Ready(fixture.m_DefenderFixture))
		{
			fixture.m_sFailureReason = "exact held defender fixture failed: "
				+ defenderFactory.Failure(fixture.m_DefenderFixture);
			return fixture;
		}

		fixture.m_State = fixture.m_DefenderFixture.m_State;
		fixture.m_Preset = fixture.m_DefenderFixture.m_Preset;
		fixture.m_sTargetZoneId = fixture.m_DefenderFixture.m_Quote.m_sTargetZoneId;
		fixture.m_sSourceZoneId = "enemy_counterattack_proof_source_" + suffix;
		fixture.m_State.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		fixture.m_State.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		fixture.m_State.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		fixture.m_State.m_iWarLevel = 3;

		fixture.m_Balance = new HST_BalanceConfig();
		fixture.m_Balance.m_iCaptureCounterattackChancePercent = 0;
		fixture.m_Balance.m_bPopulationOutcomeEnabled = false;
		fixture.m_Balance.m_bLegacyControlVictoryEnabled = false;
		fixture.m_Balance.m_bLossConditionEnabled = false;
		HST_DefaultCatalog.AddDefaultFactionPools(
			fixture.m_State,
			fixture.m_Balance,
			fixture.m_Preset);

		HST_FactionPoolState enemyPool = fixture.m_State.FindFactionPool(PROOF_FACTION_KEY);
		if (!enemyPool)
		{
			enemyPool = new HST_FactionPoolState();
			enemyPool.m_sFactionKey = PROOF_FACTION_KEY;
			fixture.m_State.m_aFactionPools.Insert(enemyPool);
		}
		enemyPool.m_iStrategicContractVersion = HST_EnemyStrategicResourceService.CONTRACT_VERSION;
		enemyPool.m_iStrategicRevision = Math.Max(1, enemyPool.m_iStrategicRevision);
		enemyPool.m_iAttackResources = 500;
		enemyPool.m_iSupportResources = 500;
		enemyPool.m_iAggression = 50;

		HST_ZoneState target = fixture.m_State.FindZone(fixture.m_sTargetZoneId);
		if (!target)
		{
			fixture.m_sFailureReason = "exact counterattack proof target is unavailable";
			return fixture;
		}
		target.m_sOwnerFactionKey = fixture.m_Preset.m_sResistanceFactionKey;
		target.m_iOwnershipContractVersion = HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION;
		target.m_iOwnershipRevision = Math.Max(1, target.m_iOwnershipRevision);
		target.m_sOwnershipAuthorityFailure = "";

		HST_ZoneState source = new HST_ZoneState();
		source.m_sZoneId = fixture.m_sSourceZoneId;
		source.m_sDisplayName = "Enemy Counterattack Proof Source";
		source.m_sOwnerFactionKey = PROOF_FACTION_KEY;
		source.m_iOwnershipContractVersion = HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION;
		source.m_iOwnershipRevision = 1;
		source.m_eType = HST_EZoneType.HST_ZONE_OUTPOST;
		source.m_iGarrisonSlots = 32;
		vector sourcePosition = target.m_vPosition;
		sourcePosition[0] = sourcePosition[0] - 420.0;
		source.m_vPosition = sourcePosition;
		fixture.m_State.m_aZones.Insert(source);

		HST_OwnershipTransitionProofFixtureFactory ownershipFactory
			= new HST_OwnershipTransitionProofFixtureFactory();
		fixture.m_OwnershipFixture = ownershipFactory.Configure(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Balance);
		if (!fixture.m_OwnershipFixture)
		{
			fixture.m_sFailureReason = "ownership proof domain fixture is unavailable";
			return fixture;
		}

		fixture.m_Queue = new HST_ForceSpawnQueueService();
		fixture.m_Adapter = new HST_ForceSpawnAdapterService();
		fixture.m_PhysicalWar = new HST_PhysicalWarService();
		fixture.m_CombatPresence = new HST_CombatPresenceService();
		fixture.m_GarrisonPatrols = new HST_GarrisonPatrolOperationService();
		fixture.m_GarrisonPatrols.SetRuntimeServices(
			fixture.m_Queue,
			fixture.m_Adapter,
			fixture.m_PhysicalWar);
		fixture.m_LocalSecurity = new HST_LocalSecurityOperationService();
		fixture.m_LocalSecurity.SetRuntimeServices(
			fixture.m_Queue,
			fixture.m_Adapter,
			fixture.m_PhysicalWar,
			fixture.m_OwnershipFixture.m_TownInfluence);
		fixture.m_EnemyGarrisonRebuilds
			= new HST_EnemyGarrisonRebuildOperationService();
		fixture.m_EnemyGarrisonRebuilds.SetRuntimeServices(
			fixture.m_Queue,
			fixture.m_Adapter,
			fixture.m_PhysicalWar,
			fixture.m_OwnershipFixture.m_Garrisons);
		fixture.m_EnemyGarrisonRebuilds.SetEnemyDirectorService(
			fixture.m_OwnershipFixture.m_EnemyDirector);

		fixture.m_Ownership = new HST_EnemyCounterattackOwnershipRetryProofHarness();
		fixture.m_Ownership.ConfigureDomainServices(
			fixture.m_Preset,
			fixture.m_Balance,
			fixture.m_OwnershipFixture.m_Economy,
			fixture.m_OwnershipFixture.m_Strategic,
			fixture.m_OwnershipFixture.m_Garrisons,
			fixture.m_OwnershipFixture.m_Civilians,
			fixture.m_OwnershipFixture.m_CampaignEvents);
		fixture.m_Ownership.ConfigureRuntimeServices(
			new HST_EnemyCommanderService(),
			fixture.m_OwnershipFixture.m_EnemyDirector,
			new HST_SupportRequestService(),
			fixture.m_PhysicalWar,
			fixture.m_LocalSecurity,
			fixture.m_GarrisonPatrols,
			fixture.m_EnemyGarrisonRebuilds,
			fixture.m_OwnershipFixture.m_ZoneCapture);
		fixture.m_Ownership.ConfigureProjectionServices(
			fixture.m_OwnershipFixture.m_MapMarkers,
			null,
			fixture.m_OwnershipFixture.m_Persistence);
		fixture.m_Ownership.SetTownInfluenceService(
			fixture.m_OwnershipFixture.m_TownInfluence);
		fixture.m_OwnershipFixture.m_TownInfluence.SetOwnershipTransitionService(
			fixture.m_Ownership);
		fixture.m_OwnershipFixture.m_ZoneCapture.SetOwnershipTransitionService(
			fixture.m_Ownership);
		fixture.m_OwnershipFixture.m_Civilians.SetOwnershipTransitionService(
			fixture.m_Ownership);

		fixture.m_EnemyDirector = fixture.m_OwnershipFixture.m_EnemyDirector;
		fixture.m_Planning = new HST_ForcePlanningService();
		fixture.m_Service = new HST_EnemyCounterattackOperationProofHarness();
		fixture.m_Service.UseDeterministicVirtualProjectionForProof();
		fixture.m_Service.SetRuntimeServices(
			fixture.m_Queue,
			fixture.m_Adapter,
			fixture.m_PhysicalWar,
			fixture.m_CombatPresence,
			fixture.m_Ownership);

		int attackCost = PROOF_ATTACK_COST;
		int supportCost;
		if (supportFunded)
		{
			attackCost = 0;
			supportCost = PROOF_SUPPORT_COST;
		}
		fixture.m_Order = BuildOrder(
			fixture,
			suffix,
			attackCost,
			supportCost);
		HST_EnemyCounterattackManifestResult planned
			= fixture.m_Planning.PlanExactEnemyCounterattack(
				fixture.m_State,
				fixture.m_Preset,
				fixture.m_Order);
		if (!planned || !planned.m_bSuccess || !planned.m_Manifest)
		{
			fixture.m_sFailureReason = "exact counterattack planning failed";
			if (planned && !planned.m_sFailureReason.IsEmpty())
				fixture.m_sFailureReason = fixture.m_sFailureReason + ": " + planned.m_sFailureReason;
			return fixture;
		}
		fixture.m_Manifest = planned.m_Manifest;
		fixture.m_Order.m_sManifestId = fixture.m_Manifest.m_sManifestId;
		fixture.m_Order.m_sManifestHash = fixture.m_Manifest.m_sManifestHash;
		fixture.m_Order.m_iCompositionManpower = fixture.m_Manifest.m_iAcceptedMemberCount;

		HST_EnemyCounterattackAdmissionResult preflight
			= fixture.m_Service.CanAdmitPreparedOrder(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Manifest,
				fixture.m_EnemyDirector);
		fixture.m_bPreflightAccepted = preflight && preflight.m_bSuccess;
		if (!fixture.m_bPreflightAccepted)
		{
			fixture.m_sFailureReason = "exact counterattack admission preflight failed";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				fixture.m_sFailureReason = fixture.m_sFailureReason + ": " + preflight.m_sFailureReason;
			return fixture;
		}

		fixture.m_iAttackBeforeDebit = enemyPool.m_iAttackResources;
		fixture.m_iSupportBeforeDebit = enemyPool.m_iSupportResources;
		fixture.m_Order.m_sResourceDebitMutationId
			= "enemy_resource_debit_" + fixture.m_Order.m_sOrderId;
		if (supportFunded)
		{
			HST_ZoneState debitTarget = fixture.m_State.FindZone(
				fixture.m_Order.m_sTargetZoneId);
			fixture.m_EnemyDirector.RecordZoneDamageSignal(
				fixture.m_State,
				PROOF_FACTION_KEY,
				debitTarget,
				100,
				"focused support-funded counterattack proof pressure");
			fixture.m_bDebitAccepted = fixture.m_EnemyDirector.TrySpendDefense(
				fixture.m_State,
				debitTarget,
				PROOF_FACTION_KEY,
				0,
				fixture.m_Order.m_iSupportCost,
				fixture.m_sDebitReason,
				fixture.m_Order.m_sResourceDebitMutationId,
				fixture.m_Order.m_sOrderId,
				fixture.m_Order.m_sOrderId,
				fixture.m_Order.m_sOperationId,
				fixture.m_Manifest.m_sManifestId);
		}
		else
		{
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
		}
		fixture.m_iAttackAfterDebit = enemyPool.m_iAttackResources;
		fixture.m_iSupportAfterDebit = enemyPool.m_iSupportResources;
		if (!fixture.m_bDebitAccepted)
		{
			fixture.m_sFailureReason = "exact counterattack debit failed: " + fixture.m_sDebitReason;
			return fixture;
		}

		fixture.m_State.m_aEnemyOrders.Insert(fixture.m_Order);
		fixture.m_Admission = fixture.m_Service.AdmitPreparedOrder(
			fixture.m_State,
			fixture.m_Order,
			fixture.m_Manifest,
			fixture.m_EnemyDirector);
		if (!fixture.m_Admission || !fixture.m_Admission.m_bSuccess)
		{
			fixture.m_sFailureReason = "exact counterattack admission failed";
			if (fixture.m_Admission && !fixture.m_Admission.m_sFailureReason.IsEmpty())
				fixture.m_sFailureReason = fixture.m_sFailureReason + ": "
					+ fixture.m_Admission.m_sFailureReason;
			return fixture;
		}
		fixture.m_Operation = fixture.m_Admission.m_Operation;
		fixture.m_Batch = fixture.m_Admission.m_Batch;
		fixture.m_Group = fixture.m_Admission.m_Group;
		return fixture;
	}

	HST_EnemyOrderState BuildOrder(
		HST_EnemyCounterattackOperationProofFixture fixture,
		string suffix,
		int attackCost,
		int supportCost)
	{
		if (!fixture || !fixture.m_State)
			return null;
		HST_EnemyOrderState order = new HST_EnemyOrderState();
		order.m_sOrderId = string.Format(
			"enemy_counterattack_proof_%1_%2_%3",
			suffix,
			attackCost,
			supportCost);
		order.m_sOperationId = HST_StableIdService.BuildOperationId(
			"enemy_order",
			order.m_sOrderId);
		order.m_iOperationContractVersion
			= HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION;
		order.m_sFactionKey = PROOF_FACTION_KEY;
		order.m_eType = HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED;
		order.m_sSourceZoneId = fixture.m_sSourceZoneId;
		order.m_sTargetZoneId = fixture.m_sTargetZoneId;
		order.m_vSourcePosition = fixture.m_State.FindZone(fixture.m_sSourceZoneId).m_vPosition;
		order.m_vTargetPosition = fixture.m_State.FindZone(fixture.m_sTargetZoneId).m_vPosition;
		order.m_iCreatedAtSecond = fixture.m_State.m_iElapsedSeconds;
		order.m_iAttackCost = attackCost;
		order.m_iSupportCost = supportCost;
		order.m_sRuntimeStatus = "proof_prepaid_pending";
		return order;
	}

	bool Ready(HST_EnemyCounterattackOperationProofFixture fixture)
	{
		if (!fixture)
			return false;
		bool campaignReady = fixture.m_State && fixture.m_Preset
			&& fixture.m_Planning && fixture.m_EnemyDirector;
		bool runtimeReady = fixture.m_Queue && fixture.m_Adapter
			&& fixture.m_PhysicalWar && fixture.m_Service && fixture.m_Ownership
			&& fixture.m_EnemyGarrisonRebuilds;
		bool planningReady = fixture.m_Order && fixture.m_Manifest
			&& fixture.m_bPreflightAccepted && fixture.m_bDebitAccepted;
		bool admissionReady = fixture.m_Admission
			&& fixture.m_Admission.m_bSuccess && fixture.m_Operation;
		bool projectionReady = fixture.m_Batch && fixture.m_Group;
		return campaignReady && runtimeReady && planningReady
			&& admissionReady && projectionReady;
	}

	string Failure(HST_EnemyCounterattackOperationProofFixture fixture)
	{
		if (!fixture)
			return "exact enemy counterattack proof fixture is unavailable";
		if (!fixture.m_sFailureReason.IsEmpty())
			return fixture.m_sFailureReason;
		return "exact enemy counterattack proof fixture is incomplete";
	}
}

// Deterministic source-level proof. Native spawning, live AI combat, marker
// entities, network replication, profile I/O, and a real restart remain
// packaged-runtime gates.
class HST_EnemyCounterattackOperationProofService
{
	protected ref HST_EnemyCounterattackOperationProofFixtureFactory m_Fixtures
		= new HST_EnemyCounterattackOperationProofFixtureFactory();

	HST_EnemyCounterattackOperationProofReport Run()
	{
		HST_EnemyCounterattackOperationProofReport report
			= new HST_EnemyCounterattackOperationProofReport();
		HST_EnemyCounterattackOperationProofFixture lifecycle
			= m_Fixtures.BuildAdmittedFixture("lifecycle");
		ProveFrozenPlanningAndAdmission(report, lifecycle);
		ProveVirtualLifecycle(report, lifecycle);
		ProvePhysicalToVirtualHandoff(report);
		ProveSupportFundedSettlement(report);
		ProveRestoreLifecycle(report);
		ProveResourceAuthorityQuarantine(report);
		ProveAmbiguousRuntimeHold(report);
		ProveOwnershipTransitionCorrelationQuarantine(report);
		ProveSchema69Quarantine(report);
		ProveSchema69QuarantineRetention(report);
		return report;
	}

	protected void ProveFrozenPlanningAndAdmission(
		HST_EnemyCounterattackOperationProofReport report,
		HST_EnemyCounterattackOperationProofFixture fixture)
	{
		if (!m_Fixtures.Ready(fixture))
		{
			string failure = m_Fixtures.Failure(fixture);
			report.m_sPlanningEvidence = failure;
			report.m_sAdmissionEvidence = failure;
			return;
		}

		HST_ForcePlanningIntegrityService integrity
			= new HST_ForcePlanningIntegrityService();
		HST_StrategicMovementService movement = new HST_StrategicMovementService();
		bool frozen = fixture.m_Manifest.m_bFrozen
			&& fixture.m_Manifest.m_aGroups.Count() == 1
			&& fixture.m_Manifest.m_aVehicles.Count() == 0
			&& fixture.m_Manifest.m_aAssets.Count() == 0
			&& fixture.m_Manifest.m_iAcceptedMemberCount
				== fixture.m_Manifest.m_aMembers.Count()
			&& fixture.m_Manifest.m_sForceKind
				== HST_OperationService.EXACT_ENEMY_COUNTERATTACK_FORCE_KIND
			&& fixture.m_Manifest.m_sPolicyId
				== HST_OperationService.EXACT_ENEMY_COUNTERATTACK_POLICY_ID
			&& fixture.m_Manifest.m_sIntentId
				== HST_OperationService.EXACT_ENEMY_COUNTERATTACK_MANIFEST_INTENT
			&& fixture.m_Manifest.m_sManifestHash
				== integrity.BuildManifestHash(fixture.m_Manifest)
			&& movement.IsSupportedExactInfantryManifest(fixture.m_Manifest);

		HST_EnemyOrderState supportOrder = m_Fixtures.BuildOrder(
			fixture,
			"planning_support",
			0,
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_SUPPORT_COST);
		HST_EnemyCounterattackManifestResult supportPlan
			= fixture.m_Planning.PlanExactEnemyCounterattack(
				fixture.m_State,
				fixture.m_Preset,
				supportOrder);
		HST_EnemyOrderState bothOrder = m_Fixtures.BuildOrder(
			fixture,
			"planning_both_rejected",
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_ATTACK_COST,
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_SUPPORT_COST);
		HST_EnemyCounterattackManifestResult bothPlan
			= fixture.m_Planning.PlanExactEnemyCounterattack(
				fixture.m_State,
				fixture.m_Preset,
				bothOrder);
		HST_EnemyOrderState zeroOrder = m_Fixtures.BuildOrder(
			fixture,
			"planning_zero_rejected",
			0,
			0);
		HST_EnemyCounterattackManifestResult zeroPlan
			= fixture.m_Planning.PlanExactEnemyCounterattack(
				fixture.m_State,
				fixture.m_Preset,
				zeroOrder);
		bool onePoolExact = fixture.m_Order.m_iAttackCost > 0
			&& fixture.m_Order.m_iSupportCost == 0
			&& fixture.m_Manifest.m_iAttackResourceCost == fixture.m_Order.m_iAttackCost
			&& fixture.m_Manifest.m_iSupportResourceCost == 0
			&& supportPlan && supportPlan.m_bSuccess && supportPlan.m_Manifest
			&& supportPlan.m_Manifest.m_iAttackResourceCost == 0
			&& supportPlan.m_Manifest.m_iSupportResourceCost == supportOrder.m_iSupportCost
			&& bothPlan && !bothPlan.m_bSuccess
			&& zeroPlan && !zeroPlan.m_bSuccess;

		HST_EnemyCounterattackManifestResult persistedReplay
			= fixture.m_Planning.PlanExactEnemyCounterattack(
				fixture.m_State,
				fixture.m_Preset,
				fixture.m_Order);
		bool planningReplay = persistedReplay && persistedReplay.m_bSuccess
			&& persistedReplay.m_Manifest == fixture.m_Manifest
			&& persistedReplay.m_Manifest.m_sManifestHash
				== fixture.m_Manifest.m_sManifestHash;
		report.m_bFrozenPlanningExact = frozen && onePoolExact && planningReplay;
		report.m_sPlanningEvidence = string.Format(
			"manifest %1 hash %2 members %3 | attack/support shapes %4/%5",
			fixture.m_Manifest.m_sManifestId,
			fixture.m_Manifest.m_sManifestHash,
			fixture.m_Manifest.m_iAcceptedMemberCount,
			fixture.m_Manifest.m_iAttackResourceCost,
			supportPlan && supportPlan.m_Manifest
				&& supportPlan.m_Manifest.m_iSupportResourceCost);
		report.m_sPlanningEvidence = report.m_sPlanningEvidence + string.Format(
			" | both/zero rejected %1/%2 | replay %3",
			bothPlan && !bothPlan.m_bSuccess,
			zeroPlan && !zeroPlan.m_bSuccess,
			planningReplay);

		int operationsBefore = CountOperationId(
			fixture.m_State,
			fixture.m_Operation.m_sOperationId);
		int manifestsBefore = CountManifestId(
			fixture.m_State,
			fixture.m_Manifest.m_sManifestId);
		int batchesBefore = CountBatchId(
			fixture.m_State,
			fixture.m_Batch.m_sResultId);
		int groupsBefore = CountGroupId(
			fixture.m_State,
			fixture.m_Group.m_sGroupId);
		HST_EnemyCounterattackAdmissionResult replay
			= fixture.m_Service.AdmitPreparedOrder(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Manifest,
				fixture.m_EnemyDirector);
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool debitExact = fixture.m_iAttackAfterDebit
			== fixture.m_iAttackBeforeDebit - fixture.m_Order.m_iAttackCost
			&& fixture.m_iSupportAfterDebit == fixture.m_iSupportBeforeDebit
			&& pool.m_iAttackResources == fixture.m_iAttackAfterDebit
			&& pool.m_iSupportResources == fixture.m_iSupportAfterDebit;
		bool orderLinksExact = fixture.m_Order.m_sOperationId
			== fixture.m_Operation.m_sOperationId
			&& fixture.m_Order.m_sManifestId == fixture.m_Manifest.m_sManifestId
			&& fixture.m_Order.m_sSpawnResultId == fixture.m_Batch.m_sResultId
			&& fixture.m_Order.m_sGroupId == fixture.m_Group.m_sGroupId;
		bool operationLinksExact = fixture.m_Operation.m_sEnemyOrderId
			== fixture.m_Order.m_sOrderId
			&& fixture.m_Operation.m_sSpawnResultId == fixture.m_Batch.m_sResultId
			&& fixture.m_Operation.m_sGroupId == fixture.m_Group.m_sGroupId;
		bool batchLinksExact = fixture.m_Batch.m_sOperationId
			== fixture.m_Operation.m_sOperationId
			&& fixture.m_Batch.m_sManifestId == fixture.m_Manifest.m_sManifestId;
		bool groupLinksExact = fixture.m_Group.m_sOperationId
			== fixture.m_Operation.m_sOperationId
			&& fixture.m_Group.m_sManifestId == fixture.m_Manifest.m_sManifestId
			&& fixture.m_Group.m_sSpawnResultId == fixture.m_Batch.m_sResultId;
		bool projectionLinksExact = fixture.m_Group.m_bQRF
			&& fixture.m_Batch.m_bStrategicProjectionHeld;
		bool linksExact = orderLinksExact && operationLinksExact
			&& batchLinksExact && groupLinksExact && projectionLinksExact;
		bool replayExact = replay && replay.m_bSuccess && !replay.m_bStateChanged
			&& replay.m_Operation == fixture.m_Operation
			&& replay.m_Batch == fixture.m_Batch
			&& replay.m_Group == fixture.m_Group
			&& CountOperationId(fixture.m_State, fixture.m_Operation.m_sOperationId)
				== operationsBefore
			&& CountManifestId(fixture.m_State, fixture.m_Manifest.m_sManifestId)
				== manifestsBefore
			&& CountBatchId(fixture.m_State, fixture.m_Batch.m_sResultId)
				== batchesBefore
			&& CountGroupId(fixture.m_State, fixture.m_Group.m_sGroupId)
				== groupsBefore;
		report.m_bAdmissionExact = fixture.m_bPreflightAccepted
			&& debitExact && linksExact && replayExact;
		report.m_sAdmissionEvidence = string.Format(
			"pool attack/support %1/%2 -> %3/%4 | links %5 | held %6",
			fixture.m_iAttackBeforeDebit,
			fixture.m_iSupportBeforeDebit,
			pool.m_iAttackResources,
			pool.m_iSupportResources,
			linksExact,
			fixture.m_Batch.m_bStrategicProjectionHeld);
		report.m_sAdmissionEvidence = report.m_sAdmissionEvidence + string.Format(
			" | replay rows %1/%2/%3/%4",
			CountOperationId(fixture.m_State, fixture.m_Operation.m_sOperationId),
			CountManifestId(fixture.m_State, fixture.m_Manifest.m_sManifestId),
			CountBatchId(fixture.m_State, fixture.m_Batch.m_sResultId),
			CountGroupId(fixture.m_State, fixture.m_Group.m_sGroupId));
	}

	protected void ProveVirtualLifecycle(
		HST_EnemyCounterattackOperationProofReport report,
		HST_EnemyCounterattackOperationProofFixture fixture)
	{
		if (!m_Fixtures.Ready(fixture))
		{
			string failure = m_Fixtures.Failure(fixture);
			report.m_sTravelEvidence = failure;
			report.m_sCombatEvidence = failure;
			report.m_sOwnershipEvidence = failure;
			report.m_sSettlementEvidence = failure;
			return;
		}

		int accepted = fixture.m_Manifest.m_iAcceptedMemberCount;
		string retiredSlotId;
		bool seededCasualty;
		if (accepted > 1)
		{
			retiredSlotId = fixture.m_Queue.SelectStrategicLivingMemberSlotId(
				fixture.m_Batch,
				fixture.m_Operation.m_iDeterministicSeed + 17);
			HST_ForceSpawnQueueCallbackResult casualty
				= fixture.m_Queue.ConfirmStrategicMemberCasualty(
					fixture.m_State.m_aForceSpawnResults,
					fixture.m_Manifest,
					fixture.m_Batch.m_sResultId,
					fixture.m_Batch.m_sProjectionId,
					retiredSlotId,
					fixture.m_State.m_iElapsedSeconds + 1,
					"exact enemy counterattack proof pre-travel casualty");
			seededCasualty = casualty && casualty.m_bAccepted;
		}
		int expectedTravelLiving = accepted - 1;
		int livingAfterSeed = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		float progressBefore = fixture.m_Operation.m_fRouteProgressMeters;
		int travelTicks;
		for (; travelTicks < 20; travelTicks++)
		{
			if (fixture.m_Operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
				break;
			fixture.m_State.m_iElapsedSeconds
				+= HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
			fixture.m_Service.TickOrder(
				fixture.m_State,
				fixture.m_Preset,
				fixture.m_EnemyDirector,
				fixture.m_Order);
		}
		int livingAtTarget = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		bool arrived = fixture.m_Operation.m_eDutyState
			== HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
		bool travelExact = seededCasualty && accepted > 1
			&& livingAfterSeed == expectedTravelLiving
			&& livingAtTarget == expectedTravelLiving
			&& fixture.m_Operation.m_fRouteProgressMeters > progressBefore
			&& arrived
			&& Distance2D(
				fixture.m_Operation.m_vStrategicPosition,
				fixture.m_Order.m_vTargetPosition)
				<= HST_StrategicMovementService.ARRIVAL_EPSILON_METERS;

		HST_CampaignSaveData openSave = new HST_CampaignSaveData();
		openSave.Capture(fixture.m_State);
		HST_CampaignState openRestored = openSave.Restore();
		HST_EnemyOrderState restoredOrder;
		HST_ForceSpawnResultState restoredBatch;
		if (openRestored)
		{
			restoredOrder = openRestored.FindEnemyOrder(fixture.m_Order.m_sOrderId);
			if (restoredOrder)
				restoredBatch = openRestored.FindForceSpawnResult(restoredOrder.m_sSpawnResultId);
		}
		HST_ForceSpawnQueueService restoredQueue = new HST_ForceSpawnQueueService();
		bool openRosterPersisted = restoredOrder && restoredBatch
			&& restoredQueue.CountStrategicLivingMemberSlots(restoredBatch)
				== expectedTravelLiving
			&& IsConfirmedRetiredMemberSlot(restoredBatch, retiredSlotId);
		report.m_bVirtualTravelExact = travelExact && openRosterPersisted;
		report.m_sTravelEvidence = string.Format(
			"ticks %1 | route %2/%3m | living accepted/after-loss/arrival %4/%5/%6",
			travelTicks,
			Math.Round(fixture.m_Operation.m_fRouteProgressMeters),
			Math.Round(fixture.m_Operation.m_fRouteTotalDistanceMeters),
			accepted,
			livingAfterSeed,
			livingAtTarget);
		report.m_sTravelEvidence = report.m_sTravelEvidence + string.Format(
			" | restored retired slot %1",
			openRosterPersisted);

		HST_GarrisonService garrisons = new HST_GarrisonService();
		HST_GarrisonVirtualCombatRosterResult defenderBefore
			= garrisons.ResolveExactVirtualCombatRoster(
				fixture.m_State,
				fixture.m_sTargetZoneId,
				fixture.m_Preset.m_sResistanceFactionKey);
		fixture.m_Ownership.InjectRetryCount(1);
		fixture.m_Ownership.InjectOwnerAppliedPendingCount(1);
		for (int combatTick = 0; combatTick < 16; combatTick++)
		{
			if (fixture.m_Order.m_sRuntimeStatus == "exact_virtual_ownership_retry")
				break;
			fixture.m_State.m_iElapsedSeconds += HST_VirtualCombatService.COMBAT_STEP_SECONDS;
			fixture.m_Service.TickOrder(
				fixture.m_State,
				fixture.m_Preset,
				fixture.m_EnemyDirector,
				fixture.m_Order);
		}
		HST_GarrisonVirtualCombatRosterResult defenderAfterRetry
			= garrisons.ResolveExactVirtualCombatRoster(
				fixture.m_State,
				fixture.m_sTargetZoneId,
				fixture.m_Preset.m_sResistanceFactionKey);
		int attackerAfterCombat = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		int heldDefenderLiving = fixture.m_DefenderFixture.m_Queue
			.CountStrategicLivingMemberSlots(fixture.m_DefenderFixture.m_Batch);
		bool combatExact = defenderBefore && defenderBefore.m_bAccepted
			&& defenderBefore.m_iExactHeldInfantryCount == 1
			&& defenderBefore.m_iAggregateInfantryCount == 0
			&& defenderAfterRetry && defenderAfterRetry.m_bAccepted
			&& defenderAfterRetry.m_iTotalDefenderCount == 0
			&& heldDefenderLiving == 0
			&& HasConfirmedCasualty(fixture.m_DefenderFixture.m_Batch)
			&& attackerAfterCombat == expectedTravelLiving
			&& fixture.m_Operation.m_iVirtualCombatStepIndex > 0
			&& fixture.m_Operation.m_iLastVirtualHostileCount == 0;
		report.m_bVirtualCombatExact = combatExact;
		report.m_sCombatEvidence = string.Format(
			"defenders aggregate/exact/total %1/%2/%3 -> %4 | attacker living %5 | combat steps %6",
			defenderBefore && defenderBefore.m_iAggregateInfantryCount,
			defenderBefore && defenderBefore.m_iExactHeldInfantryCount,
			defenderBefore && defenderBefore.m_iTotalDefenderCount,
			defenderAfterRetry && defenderAfterRetry.m_iTotalDefenderCount,
			attackerAfterCombat,
			fixture.m_Operation.m_iVirtualCombatStepIndex);
		report.m_sCombatEvidence = report.m_sCombatEvidence + string.Format(
			" | held casualty %1",
			HasConfirmedCasualty(fixture.m_DefenderFixture.m_Batch));

		HST_ZoneState target = fixture.m_State.FindZone(fixture.m_sTargetZoneId);
		bool retryHeld = fixture.m_Ownership.InjectedRetryCount() == 1
			&& target.m_sOwnerFactionKey == fixture.m_Preset.m_sResistanceFactionKey
			&& fixture.m_Order.m_sRuntimeStatus == "exact_virtual_ownership_retry"
			&& fixture.m_Operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			&& !fixture.m_Order.m_bOutcomeApplied
			&& fixture.m_State.FindOwnershipTransition(
				"ownership_counterattack_" + fixture.m_Order.m_sOperationId) == null;

		string ownershipRequestId
			= "ownership_counterattack_" + fixture.m_Order.m_sOperationId;
		fixture.m_State.m_iElapsedSeconds += HST_VirtualCombatService.COMBAT_STEP_SECONDS;
		fixture.m_Service.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		HST_OwnershipTransitionState pendingReceipt
			= fixture.m_State.FindOwnershipTransition(ownershipRequestId);
		bool pendingReceiptExact = pendingReceipt && pendingReceipt.m_bOwnerApplied
			&& !pendingReceipt.m_bCompleted
			&& pendingReceipt.m_sRequestId == ownershipRequestId
			&& pendingReceipt.m_sSourceId == fixture.m_Order.m_sOperationId;
		bool pendingInjectionExact = fixture.m_Ownership.OwnerAppliedPendingCount() == 1;
		bool pendingZoneExact = target.m_sOwnerFactionKey == fixture.m_Order.m_sFactionKey
			&& target.m_sActiveOwnershipTransitionRequestId == ownershipRequestId
			&& target.m_sLastOwnershipTransitionRequestId.IsEmpty();
		bool pendingRuntimeExact = fixture.m_Order.m_sRuntimeStatus
			== "exact_virtual_ownership_retry";
		bool pendingDutyExact = fixture.m_Operation.m_eDutyState
			== HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
		bool pendingOutcomeExact = !fixture.m_Order.m_bOutcomeApplied;
		bool ownerAppliedPendingHeld = pendingInjectionExact && pendingZoneExact
			&& pendingReceiptExact && pendingRuntimeExact && pendingDutyExact
			&& pendingOutcomeExact;
		string pendingRuntimeStatus = fixture.m_Order.m_sRuntimeStatus;
		string pendingOrderFailure = fixture.m_Order.m_sFailureReason;
		string pendingReceiptStatus;
		string pendingReceiptFailure;
		if (pendingReceipt)
		{
			pendingReceiptStatus = pendingReceipt.m_sStatus;
			pendingReceiptFailure = pendingReceipt.m_sFailureReason;
		}

		fixture.m_State.m_iElapsedSeconds += HST_VirtualCombatService.COMBAT_STEP_SECONDS;
		fixture.m_Service.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		HST_OwnershipTransitionState ownershipReceipt
			= fixture.m_State.FindOwnershipTransition(ownershipRequestId);
		bool canonicalOutcome = target.m_sOwnerFactionKey == fixture.m_Order.m_sFactionKey
			&& fixture.m_Order.m_bOutcomeApplied
			&& fixture.m_Order.m_sResolutionKind == "exact_counterattack_recaptured"
			&& fixture.m_Operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN
			&& ownershipReceipt && ownershipReceipt.m_bCompleted
			&& ownershipReceipt.m_sCause == "military_capture"
			&& ownershipReceipt.m_sSourceType == "enemy_counterattack"
			&& ownershipReceipt.m_sSourceId == fixture.m_Order.m_sOperationId
			&& !ownershipReceipt.m_bApplyEnemyConsequences
			&& ownershipReceipt.m_bReconcileSecurity
			&& !ownershipReceipt.m_bCreateSecurity;

		HST_OwnershipTransitionRequest replayRequest
			= fixture.m_Ownership.BuildRequest(
				fixture.m_State,
				fixture.m_sTargetZoneId,
				fixture.m_Order.m_sFactionKey,
				"military_capture",
				"enemy_counterattack",
				fixture.m_Order.m_sOperationId,
				"exact enemy counterattack recaptured the location",
				0,
				ownershipRequestId);
		replayRequest.m_bApplyEnemyConsequences = false;
		replayRequest.m_bReconcileSecurity = true;
		replayRequest.m_bCreateSecurity = false;
		replayRequest.m_bNotify = true;
		HST_OwnershipTransitionResult ownershipReplay
			= fixture.m_Ownership.Apply(fixture.m_State, replayRequest);
		bool ownershipIdempotent = ownershipReplay && ownershipReplay.m_bAccepted
			&& ownershipReplay.m_bCompleted && ownershipReplay.m_bAlreadyApplied
			&& CountOwnershipTransitionId(fixture.m_State, ownershipRequestId) == 1;
		report.m_bOwnershipRetryExact = retryHeld && ownerAppliedPendingHeld
			&& canonicalOutcome
			&& ownershipIdempotent;
		report.m_sOwnershipEvidence = string.Format(
			"pre-owner retry held %1 | owner-applied partial held %2 | owner/outcome/return %3/%4/%5",
			retryHeld,
			ownerAppliedPendingHeld,
			target.m_sOwnerFactionKey,
			fixture.m_Order.m_bOutcomeApplied,
			fixture.m_Operation.m_eDutyState);
		report.m_sOwnershipEvidence = report.m_sOwnershipEvidence + string.Format(
			" | receipt completed %1 | replay %2",
			ownershipReceipt && ownershipReceipt.m_bCompleted,
			ownershipIdempotent);
		report.m_sOwnershipEvidence = report.m_sOwnershipEvidence + string.Format(
			" | production applies %1 | stable request %2",
			fixture.m_Ownership.ProductionApplyCount(),
			ownershipRequestId);
		report.m_sOwnershipEvidence = report.m_sOwnershipEvidence + string.Format(
			" | pending injection/zone/receipt/runtime/duty/outcome %1/%2/%3/%4/%5/%6",
			pendingInjectionExact,
			pendingZoneExact,
			pendingReceiptExact,
			pendingRuntimeExact,
			pendingDutyExact,
			pendingOutcomeExact);
		report.m_sOwnershipEvidence = report.m_sOwnershipEvidence + string.Format(
			" | pending status '%1' receipt '%2' | order failure '%3' | receipt failure '%4'",
			pendingRuntimeStatus,
			pendingReceiptStatus,
			pendingOrderFailure,
			pendingReceiptFailure);
		report.m_sOwnershipEvidence = report.m_sOwnershipEvidence + string.Format(
			" | missing dependencies '%1'",
			fixture.m_Ownership.MissingDependenciesForProof());

		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		int attackBeforeReturn = pool.m_iAttackResources;
		int supportBeforeReturn = pool.m_iSupportResources;
		for (int returnTick = 0; returnTick < 20; returnTick++)
		{
			if (fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				break;
			fixture.m_State.m_iElapsedSeconds
				+= HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
			fixture.m_Service.TickOrder(
				fixture.m_State,
				fixture.m_Preset,
				fixture.m_EnemyDirector,
				fixture.m_Order);
		}
		int expectedRefund = fixture.m_Order.m_iAttackCost
			* fixture.m_Order.m_iSettlementSurvivorMemberCount
			/ fixture.m_Order.m_iSettlementAcceptedMemberCount;
		int attackAfterReturn = pool.m_iAttackResources;
		int supportAfterReturn = pool.m_iSupportResources;
		string settlementId = fixture.m_Order.m_sResourceSettlementId;
		string refundMutationId = fixture.m_Order.m_sResourceRefundMutationId;
		bool refundExact = fixture.m_Order.m_bResourceSettlementApplied
			&& fixture.m_Order.m_iSettlementAcceptedMemberCount == accepted
			&& fixture.m_Order.m_iSettlementSurvivorMemberCount == expectedTravelLiving
			&& fixture.m_Order.m_iRefundedAttackResources == expectedRefund
			&& fixture.m_Order.m_iRefundedSupportResources == 0
			&& attackAfterReturn - attackBeforeReturn == expectedRefund
			&& supportAfterReturn == supportBeforeReturn;
		bool terminalExact = fixture.m_Order.m_eStatus
			== HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED
			&& fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& fixture.m_Operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
			&& fixture.m_Operation.m_sSettlementId == settlementId
			&& CountMutationId(fixture.m_State, refundMutationId) == 1;

		bool replayTick = fixture.m_Service.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		bool settlementReplay = !replayTick
			&& pool.m_iAttackResources == attackAfterReturn
			&& pool.m_iSupportResources == supportAfterReturn
			&& fixture.m_Order.m_sResourceSettlementId == settlementId
			&& fixture.m_Order.m_sResourceRefundMutationId == refundMutationId
			&& CountMutationId(fixture.m_State, refundMutationId) == 1
			&& CountOperationId(fixture.m_State, fixture.m_Operation.m_sOperationId) == 1;
		report.m_bSettlementReplayExact = refundExact && terminalExact
			&& settlementReplay;
		report.m_sSettlementEvidence = string.Format(
			"accepted/survivors %1/%2 | attack refund expected/applied %3/%4 | pool delta %5 | support delta %6",
			fixture.m_Order.m_iSettlementAcceptedMemberCount,
			fixture.m_Order.m_iSettlementSurvivorMemberCount,
			expectedRefund,
			fixture.m_Order.m_iRefundedAttackResources,
			attackAfterReturn - attackBeforeReturn,
			supportAfterReturn - supportBeforeReturn);
		report.m_sSettlementEvidence = report.m_sSettlementEvidence + string.Format(
			" | terminal/replay/mutations %1/%2/%3",
			terminalExact,
			settlementReplay,
			CountMutationId(fixture.m_State, refundMutationId));
	}

	protected void ProvePhysicalToVirtualHandoff(
		HST_EnemyCounterattackOperationProofReport report)
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("physical_handoff");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sPhysicalHandoffEvidence = m_Fixtures.Failure(fixture);
			return;
		}

		HST_MaterializationService materialization = new HST_MaterializationService();
		HST_OperationProjectionDecision enter
			= materialization.EvaluateExactEnemyCounterattackForProximity(
				fixture.m_Operation,
				true,
				true,
				1800.0,
				2160.0);
		HST_ForceSpawnQueueCallbackResult released
			= fixture.m_Queue.ReleaseStrategicProjectionForMaterialization(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				fixture.m_State.m_iElapsedSeconds,
				fixture.m_State.m_iElapsedSeconds + 180);
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult materializing
			= operations.MarkExactEnemyCounterattackMaterializingFromVirtual(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				fixture.m_Batch,
				"focused proof entered the physical counterattack bubble");
		PrepareSyntheticSuccessfulProjection(fixture);
		fixture.m_Group.m_bSpawnedEntity = true;
		fixture.m_Group.m_sRuntimeEntityId = fixture.m_Batch.m_sNativeGroupId;
		fixture.m_Group.m_iSpawnedAgentCount = fixture.m_Manifest.m_iAcceptedMemberCount;
		HST_OperationTransitionResult physical
			= operations.MarkExactEnemyCounterattackPhysical(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				fixture.m_Batch,
				"focused proof established exact live roster authority");

		HST_ForceSpawnSlotResultState casualtySlot
			= FindFirstRegisteredMemberSlot(fixture.m_Batch);
		string casualtySlotId;
		string casualtyEntityId;
		if (casualtySlot)
		{
			casualtySlotId = casualtySlot.m_sSlotId;
			casualtyEntityId = casualtySlot.m_sEntityId;
		}
		HST_ForceSpawnQueueCallbackResult casualty;
		if (casualtySlot)
		{
			casualty = fixture.m_Queue.ConfirmRegisteredMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				casualtySlotId,
				casualtyEntityId,
				fixture.m_State.m_iElapsedSeconds + 1,
				"focused proof confirmed a live counterattack casualty");
		}
		int expectedSurvivors = fixture.m_Manifest.m_iAcceptedMemberCount - 1;
		ApplyGroupRoster(fixture.m_Group, expectedSurvivors);
		fixture.m_Group.m_iSpawnedAgentCount = expectedSurvivors;
		HST_OperationProjectionDecision leave
			= materialization.EvaluateExactEnemyCounterattackForProximity(
				fixture.m_Operation,
				false,
				false,
				1800.0,
				2160.0);
		HST_OperationTransitionResult folding
			= operations.BeginExactEnemyCounterattackDematerialization(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				"focused proof left the physical counterattack bubble");
		HST_ForceSpawnQueueCallbackResult held
			= fixture.m_Queue.RequeueSuccessfulProjectionForStrategicHold(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				fixture.m_State.m_iElapsedSeconds + 2,
				fixture.m_State.m_iElapsedSeconds + 182);
		vector handoffPosition = fixture.m_Order.m_vSourcePosition
			+ (fixture.m_Order.m_vTargetPosition - fixture.m_Order.m_vSourcePosition) * 0.35;
		fixture.m_Group.m_vPosition = handoffPosition;
		HST_StrategicMovementService strategicMovement
			= new HST_StrategicMovementService();
		bool routeSynced = strategicMovement.SyncRouteProgressFromPosition(
			fixture.m_Operation,
			handoffPosition);
		fixture.m_Group.m_bSpawnedEntity = false;
		fixture.m_Group.m_sRuntimeEntityId = "";
		fixture.m_Group.m_iSpawnedAgentCount = 0;
		fixture.m_Group.m_iAssignedWaypointCount = 0;
		HST_OperationTransitionResult virtualized
			= operations.CompleteExactEnemyCounterattackDematerialization(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				fixture.m_Batch,
				"focused proof resumed the same strategic roster");

		float progressBefore = fixture.m_Operation.m_fRouteProgressMeters;
		fixture.m_State.m_iElapsedSeconds
			+= HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
		HST_StrategicMovementResult advanced
			= strategicMovement.AdvanceExactInfantryDirectRoute(
				fixture.m_State,
				fixture.m_Operation,
				fixture.m_Group);
		int livingAfterFold = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		bool decisionsExact = enter.m_eDecision
			== HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE
			&& leave.m_eDecision
				== HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_DEMATERIALIZE;
		bool transitionsExact = released && released.m_bAccepted
			&& materializing && materializing.m_bAccepted
			&& physical && physical.m_bAccepted
			&& casualty && casualty.m_bAccepted
			&& folding && folding.m_bAccepted
			&& held && held.m_bAccepted && routeSynced
			&& virtualized && virtualized.m_bAccepted;
		HST_ForcePlanningIntegrityService integrity
			= new HST_ForcePlanningIntegrityService();
		bool rosterExact = fixture.m_Batch.m_bStrategicProjectionHeld
			&& IsConfirmedRetiredMemberSlot(fixture.m_Batch, casualtySlotId)
			&& livingAfterFold == expectedSurvivors
			&& fixture.m_Group.m_iDurableLivingInfantryCount == expectedSurvivors
			&& fixture.m_Manifest.m_sManifestHash
				== integrity.BuildManifestHash(fixture.m_Manifest);
		bool continued = advanced && advanced.m_bAccepted
			&& advanced.m_bStateChanged
			&& fixture.m_Operation.m_fRouteProgressMeters > progressBefore
			&& fixture.m_Operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& fixture.m_Operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;

		HST_CampaignSaveData handoffSave = new HST_CampaignSaveData();
		handoffSave.Capture(fixture.m_State);
		HST_CampaignState handoffRestored = handoffSave.Restore();
		HST_EnemyOrderState restoredOrder;
		HST_OperationRecordState restoredOperation;
		HST_ForceSpawnResultState restoredBatch;
		if (handoffRestored)
		{
			restoredOrder = handoffRestored.FindEnemyOrder(fixture.m_Order.m_sOrderId);
			if (restoredOrder)
			{
				restoredOperation = handoffRestored.FindOperation(restoredOrder.m_sOperationId);
				restoredBatch = handoffRestored.FindForceSpawnResult(restoredOrder.m_sSpawnResultId);
			}
		}
		bool restoredIdentityExact = restoredOrder && restoredOperation && restoredBatch
			&& restoredOrder.m_iOperationContractVersion
				== HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION
			&& restoredOperation.m_sOperationId == fixture.m_Operation.m_sOperationId;
		bool restoredRouteExact = restoredOperation
			&& Distance2D(restoredOperation.m_vRouteStartPosition, handoffPosition) <= 0.01
			&& Distance2D(restoredOperation.m_vRouteEndPosition, fixture.m_Order.m_vTargetPosition) <= 0.01
			&& Math.AbsFloat(restoredOperation.m_fRouteProgressMeters
				- fixture.m_Operation.m_fRouteProgressMeters) <= 0.01;
		HST_ForceSpawnQueueService restoredQueue = new HST_ForceSpawnQueueService();
		bool restoredRosterExact = restoredBatch && restoredBatch.m_bStrategicProjectionHeld
			&& !restoredBatch.m_bCancelRequested
			&& restoredQueue.CountStrategicLivingMemberSlots(restoredBatch) == expectedSurvivors
			&& IsConfirmedRetiredMemberSlot(restoredBatch, casualtySlotId);

		HST_CampaignSaveData replaySave = new HST_CampaignSaveData();
		if (handoffRestored)
			replaySave.Capture(handoffRestored);
		HST_CampaignState replayRestored;
		if (handoffRestored)
			replayRestored = replaySave.Restore();
		HST_EnemyOrderState replayOrder;
		HST_OperationRecordState replayOperation;
		if (replayRestored)
		{
			replayOrder = replayRestored.FindEnemyOrder(fixture.m_Order.m_sOrderId);
			if (replayOrder)
				replayOperation = replayRestored.FindOperation(replayOrder.m_sOperationId);
		}
		bool replayRestoreExact = replayOrder && replayOperation
			&& replayOrder.m_iOperationContractVersion
				== HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION
			&& Distance2D(replayOperation.m_vRouteStartPosition, handoffPosition) <= 0.01
			&& Math.AbsFloat(replayOperation.m_fRouteProgressMeters
				- fixture.m_Operation.m_fRouteProgressMeters) <= 0.01;
		bool handoffRestoreExact = restoredIdentityExact && restoredRouteExact
			&& restoredRosterExact && replayRestoreExact;
		report.m_bPhysicalHandoffExact = decisionsExact && transitionsExact
			&& rosterExact && continued && handoffRestoreExact;
		report.m_sPhysicalHandoffEvidence = string.Format(
			"decisions enter/leave %1/%2 | transitions release/materialize/physical/casualty %3/%4/%5/%6",
			enter.m_eDecision,
			leave.m_eDecision,
			released && released.m_bAccepted,
			materializing && materializing.m_bAccepted,
			physical && physical.m_bAccepted,
			casualty && casualty.m_bAccepted);
		report.m_sPhysicalHandoffEvidence
			= report.m_sPhysicalHandoffEvidence + string.Format(
			" | transitions fold/hold/virtual %1/%2/%3",
			folding && folding.m_bAccepted,
			held && held.m_bAccepted,
			virtualized && virtualized.m_bAccepted);
		report.m_sPhysicalHandoffEvidence
			= report.m_sPhysicalHandoffEvidence + string.Format(
			" | slot %1 retired | living %2 | resumed %3m | handoff restore/replay %4/%5",
			casualtySlotId,
			livingAfterFold,
			advanced && Math.Round(advanced.m_fAdvancedMeters),
			handoffRestoreExact,
			replayRestoreExact);
	}

	protected void ProveSupportFundedSettlement(
		HST_EnemyCounterattackOperationProofReport report)
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("support_settlement", true);
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sSupportSettlementEvidence = m_Fixtures.Failure(fixture);
			return;
		}

		int accepted = fixture.m_Manifest.m_iAcceptedMemberCount;
		string casualtySlotId = fixture.m_Queue.SelectStrategicLivingMemberSlotId(
			fixture.m_Batch,
			fixture.m_Operation.m_iDeterministicSeed + 29);
		HST_ForceSpawnQueueCallbackResult casualty
			= fixture.m_Queue.ConfirmStrategicMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				casualtySlotId,
				fixture.m_State.m_iElapsedSeconds + 1,
				"focused support-funded counterattack casualty");
		bool settled = DriveUntilSettled(fixture, 80);
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		int survivors = fixture.m_Order.m_iSettlementSurvivorMemberCount;
		int expectedSupportRefund = fixture.m_Order.m_iSupportCost
			* survivors / accepted;
		string refundMutationId = fixture.m_Order.m_sResourceRefundMutationId;
		int attackAfterSettlement = pool.m_iAttackResources;
		int supportAfterSettlement = pool.m_iSupportResources;
		bool debitExact = fixture.m_Order.m_iAttackCost == 0
			&& fixture.m_Order.m_iSupportCost > 0
			&& fixture.m_iAttackAfterDebit == fixture.m_iAttackBeforeDebit
			&& fixture.m_iSupportAfterDebit
				== fixture.m_iSupportBeforeDebit - fixture.m_Order.m_iSupportCost;
		bool refundExact = casualty && casualty.m_bAccepted
			&& settled && survivors > 0 && survivors < accepted
			&& fixture.m_Order.m_iRefundedAttackResources == 0
			&& fixture.m_Order.m_iRefundedSupportResources == expectedSupportRefund
			&& attackAfterSettlement == fixture.m_iAttackAfterDebit
			&& supportAfterSettlement - fixture.m_iSupportAfterDebit
				== expectedSupportRefund
			&& CountMutationId(fixture.m_State, refundMutationId) == 1;
		bool replay = fixture.m_Service.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		bool replayExact = !replay
			&& pool.m_iAttackResources == attackAfterSettlement
			&& pool.m_iSupportResources == supportAfterSettlement
			&& CountMutationId(fixture.m_State, refundMutationId) == 1
			&& fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		report.m_bSupportSettlementExact = debitExact && refundExact && replayExact;
		report.m_sSupportSettlementEvidence = string.Format(
			"pool attack/support %1/%2 -> debit %3/%4 -> settle %5/%6",
			fixture.m_iAttackBeforeDebit,
			fixture.m_iSupportBeforeDebit,
			fixture.m_iAttackAfterDebit,
			fixture.m_iSupportAfterDebit,
			attackAfterSettlement,
			supportAfterSettlement);
		report.m_sSupportSettlementEvidence
			= report.m_sSupportSettlementEvidence + string.Format(
			" | accepted/survivors %1/%2 | support refund expected/applied %3/%4 | replay/mutations %5/%6",
			accepted,
			survivors,
			expectedSupportRefund,
			fixture.m_Order.m_iRefundedSupportResources,
			replayExact,
			CountMutationId(fixture.m_State, refundMutationId));
	}

	protected void ProveRestoreLifecycle(
		HST_EnemyCounterattackOperationProofReport report)
	{
		HST_EnemyCounterattackOperationProofFixture returning
			= m_Fixtures.BuildAdmittedFixture("returning_restore");
		if (!m_Fixtures.Ready(returning))
		{
			report.m_sRestoreEvidence = m_Fixtures.Failure(returning);
			return;
		}
		bool reachedReturning = DriveUntilReturning(returning, 80);
		HST_CampaignSaveData returningSave = new HST_CampaignSaveData();
		returningSave.Capture(returning.m_State);
		HST_CampaignState firstReturning = returningSave.Restore();
		bool firstReturningExact = IsReturningRestoreExact(
			firstReturning,
			returning.m_Order.m_sOrderId);
		HST_CampaignSaveData returningReplay = new HST_CampaignSaveData();
		if (firstReturning)
			returningReplay.Capture(firstReturning);
		HST_CampaignState secondReturning;
		if (firstReturning)
			secondReturning = returningReplay.Restore();
		bool secondReturningExact = IsReturningRestoreExact(
			secondReturning,
			returning.m_Order.m_sOrderId);

		HST_EnemyCounterattackOperationProofFixture settled
			= m_Fixtures.BuildAdmittedFixture("settled_restore");
		if (!m_Fixtures.Ready(settled))
		{
			report.m_sRestoreEvidence = m_Fixtures.Failure(settled);
			return;
		}
		bool reachedSettled = DriveUntilSettled(settled, 80);
		HST_CampaignSaveData settledSave = new HST_CampaignSaveData();
		settledSave.Capture(settled.m_State);
		HST_CampaignState firstSettled = settledSave.Restore();
		bool firstSettledExact = IsSettledRestoreExact(
			firstSettled,
			settled.m_Order.m_sOrderId);
		HST_CampaignSaveData settledReplay = new HST_CampaignSaveData();
		if (firstSettled)
			settledReplay.Capture(firstSettled);
		HST_CampaignState secondSettled;
		if (firstSettled)
			secondSettled = settledReplay.Restore();
		bool secondSettledExact = IsSettledRestoreExact(
			secondSettled,
			settled.m_Order.m_sOrderId);
		bool preparedBeforeRefundExact = ProvePreparedBeforeRefundRestore();
		bool preparedAfterRefundExact = ProvePreparedAfterRefundRestore();
		bool preparedAfterRecordExact = ProvePreparedAfterRecordRestore();
		bool uncommittedFullExact = ProveUncommittedFullPreparedRestore();
		bool physicalBindingLossExact = ProvePreparedPhysicalBindingLossRestore();
		bool sameSessionPreparedExact = ProveAbortedPreparedSameSessionConsumption();
		report.m_bRestoreLifecycleExact = reachedReturning
			&& firstReturningExact && secondReturningExact
			&& reachedSettled && firstSettledExact && secondSettledExact
			&& preparedBeforeRefundExact && preparedAfterRefundExact
			&& preparedAfterRecordExact && uncommittedFullExact
			&& physicalBindingLossExact && sameSessionPreparedExact;
		report.m_sRestoreEvidence = string.Format(
			"return reached/restore/replay %1/%2/%3 | settled reached/restore/replay %4/%5/%6",
			reachedReturning,
			firstReturningExact,
			secondReturningExact,
			reachedSettled,
			firstSettledExact,
			secondSettledExact);
		report.m_sRestoreEvidence = report.m_sRestoreEvidence + string.Format(
			" | prepared pre-refund/post-refund/post-record/full/physical-zero/same-session %1/%2/%3/%4/%5/%6",
			preparedBeforeRefundExact,
			preparedAfterRefundExact,
			preparedAfterRecordExact,
			uncommittedFullExact,
			physicalBindingLossExact,
			sameSessionPreparedExact);
	}

	protected void ProveResourceAuthorityQuarantine(
		HST_EnemyCounterattackOperationProofReport report)
	{
		bool missingDebit = ProveOpenResourceQuarantineCase("missing_debit", 0);
		bool duplicateDebit = ProveOpenResourceQuarantineCase("duplicate_debit", 1);
		bool mismatchedDebit = ProveOpenResourceQuarantineCase("mismatched_debit", 2);
		bool missingRefund = ProveSettledResourceQuarantineCase("missing_refund", 0);
		bool duplicateRefund = ProveSettledResourceQuarantineCase("duplicate_refund", 1);
		bool mismatchedRefund = ProveSettledResourceQuarantineCase("mismatched_refund", 2);
		bool forgedPending = ProveForgedOpenPreparedRefundQuarantine();
		bool forgedDestroyed = ProvePreparedDestroyedLivingQuarantine();
		bool foreignExecution = ProvePreparedForeignExecutionQuarantine();
		bool settledWithoutReceipt = ProveSettledWithoutResourceReceiptHold();
		bool debitsExact = missingDebit && duplicateDebit && mismatchedDebit;
		bool refundsExact = missingRefund && duplicateRefund && mismatchedRefund;
		report.m_bResourceAuthorityQuarantineExact = debitsExact && refundsExact
			&& forgedPending && forgedDestroyed && foreignExecution
			&& settledWithoutReceipt;
		report.m_sResourceAuthorityEvidence = string.Format(
			"debit missing/duplicate/mismatch %1/%2/%3 | settled refund missing/duplicate/mismatch %4/%5/%6 | forged open/destroyed/foreign %7/%8/%9",
			missingDebit,
			duplicateDebit,
			mismatchedDebit,
			missingRefund,
			duplicateRefund,
			mismatchedRefund,
			forgedPending,
			forgedDestroyed,
			foreignExecution);
		report.m_sResourceAuthorityEvidence
			= report.m_sResourceAuthorityEvidence + string.Format(
				" | settled without receipt held %1",
				settledWithoutReceipt);
	}

	protected bool ProveSettledWithoutResourceReceiptHold()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("settled_without_receipt");
		string reason = "focused proof settled operation without resource receipt";
		if (!StagePreparedSettlement(
			fixture,
			"route_failed_survivors",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
			reason,
			true))
			return false;

		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!pool || fixture.m_Operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
			|| fixture.m_Order.m_bResourceSettlementApplied)
			return false;
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		int mutationsBefore = fixture.m_State.m_aEnemyStrategicMutations.Count();
		string refundMutationId = fixture.m_Order.m_sResourceRefundMutationId;
		if (refundMutationId.IsEmpty()
			|| CountMutationId(fixture.m_State, refundMutationId) != 0)
			return false;

		// This is the only forged field: every prepared tuple and reciprocal
		// authority row remains otherwise identical to the accepted transaction.
		fixture.m_Operation.m_eSettlementState
			= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		bool occupiedBefore = fixture.m_Service.HasOpenExactEnemyCounterattack(
			fixture.m_State,
			fixture.m_Order.m_sFactionKey,
			fixture.m_Order.m_sTargetZoneId);
		bool firstTick = fixture.m_Service.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		string firstStatus = fixture.m_Order.m_sRuntimeStatus;
		string firstFailure = fixture.m_Order.m_sFailureReason;
		bool occupiedAfterFirst = fixture.m_Service.HasOpenExactEnemyCounterattack(
			fixture.m_State,
			fixture.m_Order.m_sFactionKey,
			fixture.m_Order.m_sTargetZoneId);
		bool secondTick = fixture.m_Service.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		bool occupiedAfterSecond = fixture.m_Service.HasOpenExactEnemyCounterattack(
			fixture.m_State,
			fixture.m_Order.m_sFactionKey,
			fixture.m_Order.m_sTargetZoneId);

		bool conflictExact = firstTick && !secondTick
			&& firstStatus == "exact_runtime_conflict"
			&& firstFailure
				== "settled exact enemy counterattack precedes its durable resource receipt"
			&& fixture.m_Order.m_sRuntimeStatus == firstStatus
			&& fixture.m_Order.m_sFailureReason == firstFailure
			&& fixture.m_Order.m_eStatus
				== HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& fixture.m_Order.m_iResolvedAtSecond == 0
			&& !fixture.m_Order.m_bOutcomeApplied;
		bool settlementHeld = fixture.m_Operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& !fixture.m_Order.m_bResourceSettlementApplied
			&& CountMutationId(fixture.m_State, refundMutationId) == 0;
		bool resourcesHeld = pool.m_iAttackResources == attackBefore
			&& pool.m_iSupportResources == supportBefore
			&& fixture.m_State.m_aEnemyStrategicMutations.Count() == mutationsBefore;
		bool rowsHeld = CountEnemyOrderId(
			fixture.m_State,
			fixture.m_Order.m_sOrderId) == 1
			&& CountOperationId(
				fixture.m_State,
				fixture.m_Operation.m_sOperationId) == 1
			&& CountManifestId(
				fixture.m_State,
				fixture.m_Manifest.m_sManifestId) == 1
			&& CountBatchId(
				fixture.m_State,
				fixture.m_Batch.m_sResultId) == 1
			&& CountGroupId(
				fixture.m_State,
				fixture.m_Group.m_sGroupId) == 1;
		bool targetHeld = occupiedBefore && occupiedAfterFirst
			&& occupiedAfterSecond;
		string evidence = string.Format(
			"Partisan settled-without-receipt proof | changed/idempotent/conflict %1/%2/%3 | occupied %4/%5/%6 | pool %7/%8",
			firstTick,
			!secondTick,
			conflictExact,
			occupiedBefore,
			occupiedAfterFirst,
			occupiedAfterSecond,
			pool.m_iAttackResources,
			pool.m_iSupportResources);
		evidence = evidence + string.Format(
			" | mutations/refund %1/%2 | rows %3",
			fixture.m_State.m_aEnemyStrategicMutations.Count(),
			CountMutationId(fixture.m_State, refundMutationId),
			rowsHeld);
		Print(string.Format("%1", evidence));
		return conflictExact && settlementHeld && resourcesHeld
			&& rowsHeld && targetHeld;
	}

	protected void ProveAmbiguousRuntimeHold(
		HST_EnemyCounterattackOperationProofReport report)
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("ambiguous_hold");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sAmbiguityEvidence = m_Fixtures.Failure(fixture);
			return;
		}
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		int mutationsBefore = fixture.m_State.m_aEnemyStrategicMutations.Count();
		fixture.m_State.m_aForceSpawnResults.Insert(fixture.m_Batch);
		bool firstTick = fixture.m_Service.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		bool secondTick = fixture.m_Service.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		bool orderHeld = fixture.m_Order.m_eStatus
			== HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& fixture.m_Order.m_sRuntimeStatus == "exact_authority_ambiguous_held"
			&& fixture.m_Order.m_iResolvedAtSecond == 0
			&& !fixture.m_Order.m_bOutcomeApplied;
		bool settlementHeld = fixture.m_Operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& !fixture.m_Order.m_bResourceSettlementApplied
			&& fixture.m_Order.m_sResourceRefundMutationId.IsEmpty();
		bool resourcesHeld = pool.m_iAttackResources == attackBefore
			&& pool.m_iSupportResources == supportBefore
			&& fixture.m_State.m_aEnemyStrategicMutations.Count() == mutationsBefore;
		bool rowsHeld = CountBatchId(fixture.m_State, fixture.m_Batch.m_sResultId) == 2
			&& CountOperationId(fixture.m_State, fixture.m_Operation.m_sOperationId) == 1
			&& CountManifestId(fixture.m_State, fixture.m_Manifest.m_sManifestId) == 1
			&& CountGroupId(fixture.m_State, fixture.m_Group.m_sGroupId) == 1;
		bool foreignDeterministicCollisionHeld = ProveForeignDeterministicCollisionHold();
		report.m_bAmbiguityHoldExact = firstTick && !secondTick
			&& orderHeld && settlementHeld && resourcesHeld && rowsHeld
			&& foreignDeterministicCollisionHeld;
		report.m_sAmbiguityEvidence = string.Format(
			"ticks changed/idempotent %1/%2 | active/open %3/%4 | pool %5/%6",
			firstTick,
			!secondTick,
			orderHeld,
			settlementHeld,
			pool.m_iAttackResources,
			pool.m_iSupportResources);
		report.m_sAmbiguityEvidence = report.m_sAmbiguityEvidence + string.Format(
			" | duplicate batch retained %1 | foreign deterministic collision held %2",
			CountBatchId(fixture.m_State, fixture.m_Batch.m_sResultId),
			foreignDeterministicCollisionHeld);
	}

	protected bool ProveForeignDeterministicCollisionHold()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("foreign_deterministic_collision");
		if (!m_Fixtures.Ready(fixture))
			return false;
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!pool)
			return false;
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		int mutationsBefore = fixture.m_State.m_aEnemyStrategicMutations.Count();
		HST_ForceSpawnResultState foreign = new HST_ForceSpawnResultState();
		foreign.m_sResultId = "foreign_spawn_collision_" + fixture.m_Order.m_sOrderId;
		foreign.m_sProjectionId = "projection_" + fixture.m_Order.m_sOperationId;
		foreign.m_sForceId = "foreign_force_collision_" + fixture.m_Order.m_sOperationId;
		fixture.m_State.m_aForceSpawnResults.Insert(foreign);
		bool firstTick = fixture.m_Service.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		bool secondTick = fixture.m_Service.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		bool held = firstTick && !secondTick
			&& fixture.m_Order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& fixture.m_Order.m_sRuntimeStatus == "exact_authority_ambiguous_held"
			&& fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& !fixture.m_Order.m_bResourceSettlementApplied
			&& fixture.m_Order.m_sResourceRefundMutationId.IsEmpty()
			&& pool.m_iAttackResources == attackBefore
			&& pool.m_iSupportResources == supportBefore
			&& fixture.m_State.m_aEnemyStrategicMutations.Count() == mutationsBefore
			&& fixture.m_State.m_aForceSpawnResults.Find(foreign) >= 0;
		Print(string.Format(
			"Partisan foreign deterministic collision proof | changed/idempotent/held %1/%2/%3 | projection %4",
			firstTick,
			!secondTick,
			held,
			foreign.m_sProjectionId));
		return held;
	}

	protected void ProveOwnershipTransitionCorrelationQuarantine(
		HST_EnemyCounterattackOperationProofReport report)
	{
		bool onStationPendingExact = ProveOnStationPendingOwnershipCorrelation();
		bool materializingPendingExact
			= ProveMaterializingPendingOwnershipCorrelation();
		bool dematerializingPendingExact
			= ProveDematerializingPendingOwnershipCorrelation();
		bool returningCompletedExact = ProveReturningCompletedOwnershipCorrelation();
		bool prematureExact = ProvePrematureOwnershipTransitionQuarantine();
		bool engagedExact = ProveEngagedOwnershipTransitionQuarantine();
		bool foreignExact = ProveForeignOwnershipTransitionQuarantine();
		bool duplicateExact = ProveDuplicateOwnershipTransitionQuarantine();
		bool orphanExact = ProveOrphanOwnershipTransitionQuarantine();
		report.m_bOwnershipCorrelationQuarantineExact = onStationPendingExact
			&& materializingPendingExact && dematerializingPendingExact
			&& returningCompletedExact && prematureExact && engagedExact
			&& foreignExact && duplicateExact && orphanExact;
		report.m_sOwnershipCorrelationEvidence = string.Format(
			"lifecycle ownership authority | stable pending/returning completed %1/%2 | handoff pending materializing/dematerializing %3/%4",
			onStationPendingExact,
			returningCompletedExact,
			materializingPendingExact,
			dematerializingPendingExact);
		report.m_sOwnershipCorrelationEvidence
			= report.m_sOwnershipCorrelationEvidence + string.Format(
			" | premature/engaged/foreign/duplicate/orphan quarantined %1/%2/%3/%4/%5",
			prematureExact,
			engagedExact,
			foreignExact,
			duplicateExact,
			orphanExact);
	}

	protected bool ProveOnStationPendingOwnershipCorrelation()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("ownership_correlation_pending");
		HST_OwnershipTransitionState pending
			= DriveUntilOwnerAppliedPending(fixture, 80);
		if (!pending)
			return false;

		string orderId = fixture.m_Order.m_sOrderId;
		string operationId = fixture.m_Operation.m_sOperationId;
		string requestId = pending.m_sRequestId;
		string zoneId = pending.m_sZoneId;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		NormalizeCounterattackAuthority(saveData);

		HST_EnemyOrderState order = FindOrder(saveData.m_aEnemyOrders, orderId);
		HST_OperationRecordState operation = FindOperation(
			saveData.m_aOperations,
			operationId);
		HST_OwnershipTransitionState transition = FindSavedOwnershipTransition(
			saveData.m_aOwnershipTransitions,
			requestId);
		HST_ZoneState zone = FindZone(saveData.m_aZones, zoneId);
		return order && operation && transition && zone
			&& order.m_iOperationContractVersion
				== HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			&& transition.m_iContractVersion
				== HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			&& transition.m_bOwnerApplied && !transition.m_bCompleted
			&& !transition.m_bQuarantined && transition.m_sFailureReason.IsEmpty()
			&& zone.m_iOwnershipContractVersion
				== HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			&& zone.m_sActiveOwnershipTransitionRequestId == requestId
			&& CountSavedOwnershipTransitionClaimants(saveData, operationId) == 1;
	}

	protected bool ProveReturningCompletedOwnershipCorrelation()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("ownership_correlation_returning");
		if (!m_Fixtures.Ready(fixture) || !DriveUntilReturning(fixture, 80))
			return false;

		string orderId = fixture.m_Order.m_sOrderId;
		string operationId = fixture.m_Operation.m_sOperationId;
		string requestId = "ownership_counterattack_" + operationId;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		NormalizeCounterattackAuthority(saveData);

		HST_EnemyOrderState order = FindOrder(saveData.m_aEnemyOrders, orderId);
		HST_OperationRecordState operation = FindOperation(
			saveData.m_aOperations,
			operationId);
		HST_OwnershipTransitionState transition = FindSavedOwnershipTransition(
			saveData.m_aOwnershipTransitions,
			requestId);
		return order && operation && transition
			&& order.m_iOperationContractVersion
				== HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& order.m_bOutcomeApplied
			&& order.m_sResolutionKind == "exact_counterattack_recaptured"
			&& operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN
			&& transition.m_iContractVersion
				== HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			&& transition.m_bCompleted && !transition.m_bQuarantined
			&& transition.m_sStatus == "completed"
			&& transition.m_sFailureReason.IsEmpty()
			&& CountSavedOwnershipTransitionClaimants(saveData, operationId) == 1;
	}

	protected bool ProveMaterializingPendingOwnershipCorrelation()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("ownership_correlation_materializing");
		if (!DriveUntilOnStation(fixture, 80))
			return false;
		HST_OwnershipTransitionRequest request
			= BuildCanonicalCounterattackOwnershipRequest(fixture);
		HST_OwnershipTransitionState pending
			= fixture.m_Ownership.AdmitPreOwnerPendingForProof(
				fixture.m_State,
				request);
		HST_ForceSpawnQueueCallbackResult released
			= fixture.m_Queue.ReleaseStrategicProjectionForMaterialization(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				fixture.m_State.m_iElapsedSeconds,
				fixture.m_State.m_iElapsedSeconds + 180);
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult materializing
			= operations.MarkExactEnemyCounterattackMaterializingFromVirtual(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				fixture.m_Batch,
				"focused ownership correlation crossed materialization");
		if (!pending || !released || !released.m_bAccepted
			|| !materializing || !materializing.m_bAccepted
			|| fixture.m_Operation.m_eMaterializationState
				!= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
			return false;

		string orderId = fixture.m_Order.m_sOrderId;
		string operationId = fixture.m_Operation.m_sOperationId;
		string requestId = pending.m_sRequestId;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		NormalizeCounterattackAuthority(saveData);
		return IsValidPendingCounterattackOwnershipAfterNormalize(
			saveData,
			orderId,
			operationId,
			requestId,
			false);
	}

	protected bool ProveDematerializingPendingOwnershipCorrelation()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("ownership_correlation_dematerializing");
		HST_OwnershipTransitionState pending
			= DriveUntilOwnerAppliedPending(fixture, 80);
		if (!pending)
			return false;
		HST_ForceSpawnQueueCallbackResult released
			= fixture.m_Queue.ReleaseStrategicProjectionForMaterialization(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				fixture.m_State.m_iElapsedSeconds,
				fixture.m_State.m_iElapsedSeconds + 180);
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult materializing
			= operations.MarkExactEnemyCounterattackMaterializingFromVirtual(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				fixture.m_Batch,
				"focused ownership correlation entered physical authority");
		PrepareSyntheticSuccessfulProjection(fixture);
		fixture.m_Group.m_bSpawnedEntity = true;
		fixture.m_Group.m_sRuntimeEntityId = fixture.m_Batch.m_sNativeGroupId;
		fixture.m_Group.m_iSpawnedAgentCount
			= fixture.m_Manifest.m_iAcceptedMemberCount;
		HST_OperationTransitionResult physical
			= operations.MarkExactEnemyCounterattackPhysical(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				fixture.m_Batch,
				"focused ownership correlation established live authority");
		HST_OperationTransitionResult dematerializing
			= operations.BeginExactEnemyCounterattackDematerialization(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				"focused ownership correlation crossed dematerialization");
		if (!released || !released.m_bAccepted
			|| !materializing || !materializing.m_bAccepted
			|| !physical || !physical.m_bAccepted
			|| !dematerializing || !dematerializing.m_bAccepted)
			return false;

		string orderId = fixture.m_Order.m_sOrderId;
		string operationId = fixture.m_Operation.m_sOperationId;
		string requestId = pending.m_sRequestId;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		NormalizeCounterattackAuthority(saveData);
		return IsValidPendingCounterattackOwnershipAfterNormalize(
			saveData,
			orderId,
			operationId,
			requestId,
			true);
	}

	protected bool ProvePrematureOwnershipTransitionQuarantine()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("ownership_correlation_premature");
		HST_OwnershipTransitionState pending = ApplyPrematureOwnershipPending(fixture);
		if (!pending || fixture.m_Operation.m_eDutyState
			== HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
			return false;

		string orderId = fixture.m_Order.m_sOrderId;
		string requestId = pending.m_sRequestId;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		NormalizeCounterattackAuthority(saveData);
		return IsLinkedCounterattackOwnershipQuarantineExact(
			saveData,
			orderId,
			requestId);
	}

	protected bool ProveEngagedOwnershipTransitionQuarantine()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("ownership_correlation_engaged");
		HST_OwnershipTransitionState pending
			= DriveUntilOwnerAppliedPending(fixture, 80);
		if (!pending)
			return false;
		fixture.m_Operation.m_eEngagementMode
			= HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED;

		string orderId = fixture.m_Order.m_sOrderId;
		string requestId = pending.m_sRequestId;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		NormalizeCounterattackAuthority(saveData);
		return IsLinkedCounterattackOwnershipQuarantineExact(
			saveData,
			orderId,
			requestId);
	}

	protected bool ProveForeignOwnershipTransitionQuarantine()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("ownership_correlation_foreign");
		HST_OwnershipTransitionState pending
			= DriveUntilOwnerAppliedPending(fixture, 80);
		if (!pending)
			return false;

		string orderId = fixture.m_Order.m_sOrderId;
		string canonicalRequestId = pending.m_sRequestId;
		string foreignRequestId = "ownership_counterattack_foreign_"
			+ fixture.m_Operation.m_sOperationId;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_OwnershipTransitionState saved = FindSavedOwnershipTransition(
			saveData.m_aOwnershipTransitions,
			canonicalRequestId);
		if (!saved)
			return false;
		HST_ZoneState zone = FindZone(saveData.m_aZones, saved.m_sZoneId);
		if (!zone)
			return false;
		saved.m_sRequestId = foreignRequestId;
		zone.m_sActiveOwnershipTransitionRequestId = foreignRequestId;
		NormalizeCounterattackAuthority(saveData);
		return IsLinkedCounterattackOwnershipQuarantineExact(
			saveData,
			orderId,
			foreignRequestId);
	}

	protected bool ProveDuplicateOwnershipTransitionQuarantine()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("ownership_correlation_duplicate");
		HST_OwnershipTransitionState pending
			= DriveUntilOwnerAppliedPending(fixture, 80);
		if (!pending)
			return false;

		string orderId = fixture.m_Order.m_sOrderId;
		string requestId = pending.m_sRequestId;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_OwnershipTransitionState saved = FindSavedOwnershipTransition(
			saveData.m_aOwnershipTransitions,
			requestId);
		HST_CampaignSaveData collisionSource = new HST_CampaignSaveData();
		collisionSource.Capture(fixture.m_State);
		HST_OwnershipTransitionState collision = FindSavedOwnershipTransition(
			collisionSource.m_aOwnershipTransitions,
			requestId);
		if (!saved || !collision || saved == collision)
			return false;
		saveData.m_aOwnershipTransitions.Insert(collision);
		NormalizeCounterattackAuthority(saveData);
		return CountSavedOwnershipTransitionClaimants(
			saveData,
			fixture.m_Operation.m_sOperationId) == 2
			&& IsLinkedCounterattackOwnershipQuarantineExact(
				saveData,
				orderId,
				requestId);
	}

	protected bool ProveOrphanOwnershipTransitionQuarantine()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("ownership_correlation_orphan");
		HST_OwnershipTransitionState pending
			= DriveUntilOwnerAppliedPending(fixture, 80);
		if (!pending)
			return false;

		string requestId = "foreign_ownership_request_"
			+ fixture.m_Operation.m_sOperationId;
		string zoneId = pending.m_sZoneId;
		string operationId = fixture.m_Operation.m_sOperationId;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_OwnershipTransitionState saved = FindSavedOwnershipTransition(
			saveData.m_aOwnershipTransitions,
			pending.m_sRequestId);
		HST_ZoneState capturedZone = FindZone(saveData.m_aZones, zoneId);
		if (!saved || !capturedZone)
			return false;
		saved.m_sRequestId = requestId;
		saved.m_sSourceType = "foreign_ownership_source";
		capturedZone.m_sActiveOwnershipTransitionRequestId = requestId;
		saveData.m_aEnemyOrders.Clear();
		NormalizeCounterattackAuthority(saveData);

		HST_OwnershipTransitionState transition = FindSavedOwnershipTransition(
			saveData.m_aOwnershipTransitions,
			requestId);
		HST_ZoneState zone = FindZone(saveData.m_aZones, zoneId);
		HST_OperationRecordState operation = FindOperation(
			saveData.m_aOperations,
			operationId);
		return IsOwnershipAuthorityQuarantined(transition, zone)
			&& saveData.m_aEnemyOrders.IsEmpty()
			&& operation
			&& operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& !operation.m_sLastProjectionReason.IsEmpty();
	}

	protected bool DriveUntilOnStation(
		HST_EnemyCounterattackOperationProofFixture fixture,
		int maxTicks)
	{
		if (!m_Fixtures.Ready(fixture))
			return false;
		for (int tick = 0; tick < maxTicks; tick++)
		{
			if (fixture.m_Operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
				return fixture.m_Operation.m_eEngagementMode
					== HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
			fixture.m_State.m_iElapsedSeconds
				+= HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
			fixture.m_Service.TickOrder(
				fixture.m_State,
				fixture.m_Preset,
				fixture.m_EnemyDirector,
				fixture.m_Order);
		}
		return false;
	}

	protected HST_OwnershipTransitionState DriveUntilOwnerAppliedPending(
		HST_EnemyCounterattackOperationProofFixture fixture,
		int maxTicks)
	{
		if (!m_Fixtures.Ready(fixture))
			return null;
		string requestId = "ownership_counterattack_"
			+ fixture.m_Operation.m_sOperationId;
		fixture.m_Ownership.InjectOwnerAppliedPendingCount(1);
		for (int tick = 0; tick < maxTicks; tick++)
		{
			HST_OwnershipTransitionState pending
				= fixture.m_State.FindOwnershipTransition(requestId);
			if (pending)
			{
				if (pending.m_bOwnerApplied && !pending.m_bCompleted
					&& pending.m_sSourceType == "enemy_counterattack"
					&& pending.m_sSourceId == fixture.m_Operation.m_sOperationId)
					return pending;
				return null;
			}
			fixture.m_State.m_iElapsedSeconds
				+= HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
			fixture.m_Service.TickOrder(
				fixture.m_State,
				fixture.m_Preset,
				fixture.m_EnemyDirector,
				fixture.m_Order);
		}
		return null;
	}

	protected bool IsValidPendingCounterattackOwnershipAfterNormalize(
		HST_CampaignSaveData saveData,
		string orderId,
		string operationId,
		string requestId,
		bool ownerApplied)
	{
		HST_EnemyOrderState order = FindOrder(saveData.m_aEnemyOrders, orderId);
		HST_OperationRecordState operation = FindOperation(
			saveData.m_aOperations,
			operationId);
		HST_OwnershipTransitionState transition = FindSavedOwnershipTransition(
			saveData.m_aOwnershipTransitions,
			requestId);
		HST_ZoneState zone;
		if (transition)
			zone = FindZone(saveData.m_aZones, transition.m_sZoneId);
		return order && operation && transition && zone
			&& order.m_iOperationContractVersion
				== HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			&& operation.m_eEngagementMode
				== HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
			&& transition.m_iContractVersion
				== HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			&& transition.m_bOwnerApplied == ownerApplied
			&& !transition.m_bCompleted && !transition.m_bQuarantined
			&& transition.m_sFailureReason.IsEmpty()
			&& zone.m_iOwnershipContractVersion
				== HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			&& zone.m_sActiveOwnershipTransitionRequestId == requestId
			&& CountSavedOwnershipTransitionClaimants(saveData, operationId) == 1;
	}

	protected HST_OwnershipTransitionState ApplyPrematureOwnershipPending(
		HST_EnemyCounterattackOperationProofFixture fixture)
	{
		if (!m_Fixtures.Ready(fixture))
			return null;
		fixture.m_Ownership.InjectOwnerAppliedPendingCount(1);
		HST_OwnershipTransitionRequest request
			= BuildCanonicalCounterattackOwnershipRequest(fixture);
		if (!request)
			return null;
		HST_OwnershipTransitionResult result
			= fixture.m_Ownership.Apply(fixture.m_State, request);
		if (!result || !result.m_bAccepted || !result.m_bNeedsRetry
			|| !result.m_Transition)
			return null;
		return result.m_Transition;
	}

	protected HST_OwnershipTransitionRequest BuildCanonicalCounterattackOwnershipRequest(
		HST_EnemyCounterattackOperationProofFixture fixture)
	{
		if (!m_Fixtures.Ready(fixture))
			return null;
		string requestId = "ownership_counterattack_"
			+ fixture.m_Operation.m_sOperationId;
		HST_OwnershipTransitionRequest request = fixture.m_Ownership.BuildRequest(
			fixture.m_State,
			fixture.m_sTargetZoneId,
			fixture.m_Order.m_sFactionKey,
			"military_capture",
			"enemy_counterattack",
			fixture.m_Operation.m_sOperationId,
			"exact enemy counterattack recaptured the location",
			0,
			requestId);
		if (!request)
			return null;
		request.m_bApplyEnemyConsequences = false;
		request.m_bReconcileSecurity = true;
		request.m_bCreateSecurity = false;
		request.m_bNotify = true;
		return request;
	}

	protected void NormalizeCounterattackAuthority(HST_CampaignSaveData saveData)
	{
		HST_OwnershipTransitionSaveValidationService ownershipValidation
			= new HST_OwnershipTransitionSaveValidationService();
		ownershipValidation.Normalize(saveData, HST_CampaignState.SCHEMA_VERSION);
		HST_EnemyCounterattackSaveValidationService validation
			= new HST_EnemyCounterattackSaveValidationService();
		validation.Normalize(saveData, HST_CampaignState.SCHEMA_VERSION);
	}

	protected bool IsLinkedCounterattackOwnershipQuarantineExact(
		HST_CampaignSaveData saveData,
		string orderId,
		string requestId)
	{
		if (!saveData)
			return false;
		HST_EnemyOrderState order = FindOrder(saveData.m_aEnemyOrders, orderId);
		HST_OwnershipTransitionState transition = FindSavedOwnershipTransition(
			saveData.m_aOwnershipTransitions,
			requestId);
		HST_ZoneState zone;
		if (transition)
			zone = FindZone(saveData.m_aZones, transition.m_sZoneId);
		HST_OperationRecordState operation;
		HST_ForceSpawnResultState batch;
		if (order)
		{
			operation = FindOperation(saveData.m_aOperations, order.m_sOperationId);
			batch = FindBatch(saveData.m_aForceSpawnResults, order.m_sSpawnResultId);
		}
		bool aggregateHeld = order
			&& order.m_iOperationContractVersion
				== HST_EnemyCounterattackSaveValidationService.QUARANTINED_CONTRACT_VERSION
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& order.m_sRuntimeStatus == "exact_counterattack_quarantined"
			&& !order.m_sFailureReason.IsEmpty()
			&& operation
			&& operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& !operation.m_sLastProjectionReason.IsEmpty()
			&& batch && batch.m_bStrategicProjectionHeld
			&& !batch.m_bCancelRequested;
		return aggregateHeld && IsOwnershipAuthorityQuarantined(transition, zone);
	}

	protected bool IsOwnershipAuthorityQuarantined(
		HST_OwnershipTransitionState transition,
		HST_ZoneState zone)
	{
		return transition && zone
			&& transition.m_iContractVersion
				== HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION
			&& transition.m_bQuarantined
			&& transition.m_sStatus == "quarantined"
			&& !transition.m_sFailureReason.IsEmpty()
			&& zone.m_iOwnershipContractVersion
				== HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION
			&& !zone.m_sOwnershipAuthorityFailure.IsEmpty();
	}

	protected HST_OwnershipTransitionState FindSavedOwnershipTransition(
		array<ref HST_OwnershipTransitionState> transitions,
		string requestId)
	{
		if (!transitions || requestId.IsEmpty())
			return null;
		foreach (HST_OwnershipTransitionState transition : transitions)
		{
			if (transition && transition.m_sRequestId == requestId)
				return transition;
		}
		return null;
	}

	protected int CountSavedOwnershipTransitionClaimants(
		HST_CampaignSaveData saveData,
		string operationId)
	{
		int count;
		if (!saveData || operationId.IsEmpty())
			return count;
		string requestId = "ownership_counterattack_" + operationId;
		foreach (HST_OwnershipTransitionState transition : saveData.m_aOwnershipTransitions)
		{
			if (transition && (transition.m_sRequestId == requestId
				|| transition.m_sSourceId == operationId))
				count++;
		}
		return count;
	}

	protected void ProveSchema69Quarantine(
		HST_EnemyCounterattackOperationProofReport report)
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("schema69_quarantine");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sQuarantineEvidence = m_Fixtures.Failure(fixture);
			return;
		}

		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		string operationId = fixture.m_Operation.m_sOperationId;
		string manifestId = fixture.m_Manifest.m_sManifestId;
		string batchId = fixture.m_Batch.m_sResultId;
		string groupId = fixture.m_Group.m_sGroupId;
		string targetZoneId = fixture.m_sTargetZoneId;

		HST_CampaignSaveData malformed = new HST_CampaignSaveData();
		malformed.Capture(fixture.m_State);
		HST_EnemyOrderState malformedOrder = FindOrder(
			malformed.m_aEnemyOrders,
			fixture.m_Order.m_sOrderId);
		if (!malformedOrder)
		{
			report.m_sQuarantineEvidence = "schema-69 malformed proof order was not captured";
			return;
		}
		malformedOrder.m_sGroupId = "missing_schema69_counterattack_group";
		HST_CampaignState restored = malformed.Restore();
		HST_EnemyOrderState quarantined;
		if (restored)
			quarantined = restored.FindEnemyOrder(fixture.m_Order.m_sOrderId);
		HST_FactionPoolState restoredPool;
		HST_ZoneState restoredTarget;
		HST_ForceSpawnResultState restoredBatch;
		if (restored)
		{
			restoredPool = restored.FindFactionPool(
				HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
			restoredTarget = restored.FindZone(targetZoneId);
			restoredBatch = restored.FindForceSpawnResult(batchId);
		}
		bool firstContextExact = restored && quarantined && restoredPool
			&& restoredTarget;
		bool firstStatusExact;
		bool firstOutcomeExact;
		bool firstPoolExact;
		bool firstRowsExact;
		if (firstContextExact)
		{
			firstStatusExact = quarantined.m_iOperationContractVersion
				== HST_EnemyCounterattackSaveValidationService.QUARANTINED_CONTRACT_VERSION
				&& quarantined.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
				&& quarantined.m_sRuntimeStatus == "exact_counterattack_quarantined"
				&& !quarantined.m_sFailureReason.IsEmpty();
			firstOutcomeExact = !quarantined.m_bOutcomeApplied
				&& !quarantined.m_bResourceSettlementApplied
				&& restoredTarget.m_sOwnerFactionKey
					== fixture.m_Preset.m_sResistanceFactionKey;
			firstPoolExact = restoredPool.m_iAttackResources == attackBefore
				&& restoredPool.m_iSupportResources == supportBefore;
			firstRowsExact = CountOperationId(restored, operationId) == 1
				&& CountManifestId(restored, manifestId) == 1
				&& CountBatchId(restored, batchId) == 1
				&& CountGroupId(restored, groupId) == 1;
			firstRowsExact = firstRowsExact
				&& CountOwnershipTransitionForSource(restored, operationId) == 0
				&& restoredBatch && restoredBatch.m_bStrategicProjectionHeld
				&& !restoredBatch.m_bCancelRequested;
		}
		bool firstExact = firstContextExact && firstStatusExact
			&& firstOutcomeExact && firstPoolExact && firstRowsExact;
		string firstReason;
		if (quarantined)
			firstReason = quarantined.m_sFailureReason;

		HST_CampaignSaveData replaySave = new HST_CampaignSaveData();
		if (restored)
			replaySave.Capture(restored);
		HST_CampaignState replayRestored;
		if (restored)
			replayRestored = replaySave.Restore();
		HST_EnemyOrderState replayOrder;
		HST_FactionPoolState replayPool;
		HST_ZoneState replayTarget;
		HST_ForceSpawnResultState replayBatch;
		if (replayRestored)
		{
			replayOrder = replayRestored.FindEnemyOrder(fixture.m_Order.m_sOrderId);
			replayPool = replayRestored.FindFactionPool(
				HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
			replayTarget = replayRestored.FindZone(targetZoneId);
			replayBatch = replayRestored.FindForceSpawnResult(batchId);
		}
		bool replayContextExact = replayRestored && replayOrder && replayPool
			&& replayTarget;
		bool replayStatusExact;
		bool replayOutcomeExact;
		bool replayPoolExact;
		bool replayRowsExact;
		if (replayContextExact)
		{
			replayStatusExact = replayOrder.m_iOperationContractVersion
				== HST_EnemyCounterattackSaveValidationService.QUARANTINED_CONTRACT_VERSION
				&& replayOrder.m_sFailureReason == firstReason;
			replayOutcomeExact = !replayOrder.m_bOutcomeApplied
				&& !replayOrder.m_bResourceSettlementApplied
				&& replayTarget.m_sOwnerFactionKey
					== fixture.m_Preset.m_sResistanceFactionKey;
			replayPoolExact = replayPool.m_iAttackResources == attackBefore
				&& replayPool.m_iSupportResources == supportBefore;
			replayRowsExact = CountOperationId(replayRestored, operationId) == 1
				&& CountManifestId(replayRestored, manifestId) == 1
				&& CountBatchId(replayRestored, batchId) == 1
				&& CountGroupId(replayRestored, groupId) == 1;
			replayRowsExact = replayRowsExact
				&& CountOwnershipTransitionForSource(replayRestored, operationId) == 0
				&& replayBatch && replayBatch.m_bStrategicProjectionHeld
				&& !replayBatch.m_bCancelRequested;
		}
		bool replayExact = replayContextExact && replayStatusExact
			&& replayOutcomeExact && replayPoolExact && replayRowsExact;
		bool missingOwnershipExact = ProveReturningOwnershipQuarantine();
		bool inventedSettlementExact = ProveInventedFullSettlementQuarantine();
		report.m_bSchema69QuarantineExact = firstExact && replayExact
			&& missingOwnershipExact && inventedSettlementExact;
		int quarantinedContract;
		HST_EEnemyOrderStatus quarantinedStatus;
		int restoredAttack;
		int restoredSupport;
		string restoredOwner;
		if (quarantined)
		{
			quarantinedContract = quarantined.m_iOperationContractVersion;
			quarantinedStatus = quarantined.m_eStatus;
		}
		if (restoredPool)
		{
			restoredAttack = restoredPool.m_iAttackResources;
			restoredSupport = restoredPool.m_iSupportResources;
		}
		if (restoredTarget)
			restoredOwner = restoredTarget.m_sOwnerFactionKey;
		report.m_sQuarantineEvidence = string.Format(
			"contract/status %1/%2 | pool %3/%4 unchanged | rows operation/manifest %5/%6",
			quarantinedContract,
			quarantinedStatus,
			restoredAttack,
			restoredSupport,
			CountOperationId(restored, operationId),
			CountManifestId(restored, manifestId));
		report.m_sQuarantineEvidence
			= report.m_sQuarantineEvidence + string.Format(
			" | rows batch/group %1/%2 | cancel first/replay %3/%4",
			CountBatchId(restored, batchId),
			CountGroupId(restored, groupId),
			restoredBatch && restoredBatch.m_bCancelRequested,
			replayBatch && replayBatch.m_bCancelRequested);
		report.m_sQuarantineEvidence
			= report.m_sQuarantineEvidence + string.Format(
			" | owner %1 | replay reason/rows exact %2 | reason '%3'",
			restoredOwner,
			replayExact,
			firstReason);
		report.m_sQuarantineEvidence
			= report.m_sQuarantineEvidence + string.Format(
			" | missing ownership/bogus full quarantined %1/%2",
			missingOwnershipExact,
			inventedSettlementExact);
	}

	protected bool ProveReturningOwnershipQuarantine()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("missing_ownership");
		if (!m_Fixtures.Ready(fixture) || !DriveUntilReturning(fixture, 80))
			return false;

		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!pool)
			return false;
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		int mutationsBefore = fixture.m_State.m_aEnemyStrategicMutations.Count();
		HST_CampaignSaveData malformed = new HST_CampaignSaveData();
		malformed.Capture(fixture.m_State);
		HST_EnemyOrderState malformedOrder = FindOrder(
			malformed.m_aEnemyOrders,
			fixture.m_Order.m_sOrderId);
		HST_OperationRecordState malformedOperation = FindOperation(
			malformed.m_aOperations,
			fixture.m_Operation.m_sOperationId);
		if (!malformedOrder || !malformedOperation)
			return false;
		HST_ZoneState laterRecapturedTarget = FindZone(
			malformed.m_aZones,
			fixture.m_Order.m_sTargetZoneId);
		if (!laterRecapturedTarget)
			return false;
		// A later legitimate capture may replace the zone's mutable owner and
		// backlinks while this operation returns. The completed counterattack
		// receipt must remain valid historical authority in that state.
		laterRecapturedTarget.m_sOwnerFactionKey
			= fixture.m_Preset.m_sResistanceFactionKey;
		laterRecapturedTarget.m_sLastOwnershipTransitionRequestId
			= "ownership_later_capture_" + fixture.m_Operation.m_sOperationId;
		laterRecapturedTarget.m_sActiveOwnershipTransitionRequestId = "";
		string validFailure
			= HST_EnemyCounterattackSaveValidationService.ValidateCompletedOwnershipAuthority(
				malformed.m_aZones,
				malformed.m_aOwnershipTransitions,
				malformedOrder,
				malformedOperation);
		int removedReceipts = RemoveOwnershipTransitionsForSource(
			malformed.m_aOwnershipTransitions,
			fixture.m_Operation.m_sOperationId);
		string corruptFailure
			= HST_EnemyCounterattackSaveValidationService.ValidateCompletedOwnershipAuthority(
				malformed.m_aZones,
				malformed.m_aOwnershipTransitions,
				malformedOrder,
				malformedOperation);
		HST_CampaignState restored = malformed.Restore();
		if (!restored)
			return false;
		HST_EnemyOrderState quarantined = restored.FindEnemyOrder(
			fixture.m_Order.m_sOrderId);
		HST_FactionPoolState restoredPool = restored.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		HST_ForceSpawnResultState batch = restored.FindForceSpawnResult(
			fixture.m_Batch.m_sResultId);
		bool statusExact = quarantined
			&& quarantined.m_iOperationContractVersion
				== HST_EnemyCounterattackSaveValidationService.QUARANTINED_CONTRACT_VERSION
			&& quarantined.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& quarantined.m_sFailureReason.Contains("ownership");
		bool resourcesExact = restoredPool
			&& restoredPool.m_iAttackResources == attackBefore
			&& restoredPool.m_iSupportResources == supportBefore
			&& restored.m_aEnemyStrategicMutations.Count() == mutationsBefore;
		bool holdExact = batch && batch.m_bStrategicProjectionHeld
			&& !batch.m_bCancelRequested;
		return validFailure.IsEmpty() && removedReceipts == 1
			&& corruptFailure.Contains("ownership")
			&& statusExact && resourcesExact && holdExact;
	}

	protected bool ProveInventedFullSettlementQuarantine()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("invented_full_settlement");
		if (!m_Fixtures.Ready(fixture) || !DriveUntilSettled(fixture, 80))
			return false;

		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!pool)
			return false;
		int attackBefore = pool.m_iAttackResources;
		int supportBefore = pool.m_iSupportResources;
		int mutationsBefore = fixture.m_State.m_aEnemyStrategicMutations.Count();
		HST_CampaignSaveData malformed = new HST_CampaignSaveData();
		malformed.Capture(fixture.m_State);
		HST_EnemyOrderState malformedOrder = FindOrder(
			malformed.m_aEnemyOrders,
			fixture.m_Order.m_sOrderId);
		HST_OperationRecordState malformedOperation = FindOperation(
			malformed.m_aOperations,
			fixture.m_Operation.m_sOperationId);
		if (!malformedOrder || !malformedOperation)
			return false;
		string validFailure
			= HST_EnemyCounterattackSaveValidationService.ValidateSettlementPolicy(
				malformedOrder,
				malformedOperation);
		malformedOrder.m_sResourceSettlementKind = "bogus_full";
		malformedOrder.m_sResourceSettlementId = HST_OperationService.BuildSettlementId(
			malformedOrder.m_sOperationId,
			malformedOrder.m_sResourceSettlementKind);
		malformedOperation.m_sSettlementId
			= malformedOrder.m_sResourceSettlementId;
		string inventedRefundMutationId
			= "enemy_resource_refund_" + malformedOrder.m_sResourceSettlementId;
		bool fullKindRejected
			= !HST_EnemyCounterattackSaveValidationService.IsFullRefundSettlementKind(
				malformedOrder.m_sResourceSettlementKind);
		string corruptFailure
			= HST_EnemyCounterattackSaveValidationService.ValidateSettlementPolicy(
				malformedOrder,
				malformedOperation);
		HST_CampaignState restored = malformed.Restore();
		if (!restored)
			return false;
		HST_EnemyOrderState quarantined = restored.FindEnemyOrder(
			fixture.m_Order.m_sOrderId);
		HST_FactionPoolState restoredPool = restored.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool statusExact = quarantined
			&& quarantined.m_iOperationContractVersion
				== HST_EnemyCounterattackSaveValidationService.QUARANTINED_CONTRACT_VERSION
			&& quarantined.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& !quarantined.m_sFailureReason.IsEmpty();
		bool resourcesExact = restoredPool
			&& restoredPool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceSaveValidationService.QUARANTINE_CONTRACT_VERSION
			&& !restoredPool.m_sStrategicAuthorityFailure.IsEmpty()
			&& restoredPool.m_iAttackResources == attackBefore
			&& restoredPool.m_iSupportResources == supportBefore
			&& restored.m_aEnemyStrategicMutations.Count() <= mutationsBefore
			&& CountMutationId(restored, inventedRefundMutationId) == 0;
		return validFailure.IsEmpty() && fullKindRejected
			&& corruptFailure.Contains("unsupported")
			&& statusExact && resourcesExact;
	}

	protected void ProveSchema69QuarantineRetention(
		HST_EnemyCounterattackOperationProofReport report)
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("quarantine_retention");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sRetentionEvidence = m_Fixtures.Failure(fixture);
			return;
		}

		string orderId = fixture.m_Order.m_sOrderId;
		string operationId = fixture.m_Operation.m_sOperationId;
		string manifestId = fixture.m_Manifest.m_sManifestId;
		string batchId = fixture.m_Batch.m_sResultId;
		string groupId = fixture.m_Group.m_sGroupId;
		HST_CampaignSaveData malformed = new HST_CampaignSaveData();
		malformed.Capture(fixture.m_State);
		HST_EnemyOrderState malformedOrder = FindOrder(
			malformed.m_aEnemyOrders,
			orderId);
		HST_ForceSpawnResultState malformedBatch = FindBatch(
			malformed.m_aForceSpawnResults,
			batchId);
		if (!malformedOrder || !malformedBatch
			|| !RemoveGroup(malformed.m_aActiveGroups, groupId))
		{
			report.m_sRetentionEvidence
				= "quarantine-retention corruption fixture was incomplete";
			return;
		}

		// Break every direct order-to-batch identity as well as removing its group.
		// The surviving terminal batch can therefore be pinned only by finding the
		// reciprocal operation through operation.m_sEnemyOrderId and following that
		// operation's result/manifest identities.
		malformedOrder.m_sSpawnResultId
			= "missing_quarantine_retention_result_" + orderId;
		malformedOrder.m_sOperationId
			= "missing_quarantine_retention_operation_" + orderId;
		malformedOrder.m_sManifestId
			= "missing_quarantine_retention_manifest_" + orderId;
		malformedOrder.m_sGroupId
			= "missing_quarantine_retention_group_" + orderId;
		malformedBatch.m_sRequestId
			= "unrelated_quarantine_retention_request_" + orderId;
		malformedBatch.m_eStatus
			= HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
		malformedBatch.m_sTerminalReason
			= "focused proof terminal evidence awaiting quarantine retention";
		malformedBatch.m_iCompletedAtSecond = 1;
		malformedBatch.m_iUpdatedAtSecond = 1;

		HST_CampaignState retained = malformed.Restore();
		bool initialExact = IsQuarantineRetentionGraphExact(
			retained,
			orderId,
			operationId,
			manifestId,
			batchId,
			groupId);
		bool cyclesExact = initialExact;
		int pinnedPasses;
		int compactedRows;
		int persistencePasses;
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		for (int cycle = 0; cycle < 3 && retained; cycle++)
		{
			int nowSecond = HST_ForceSpawnQueueService.TERMINAL_RETENTION_SECONDS
				+ 1000 + cycle;
			retained.m_iElapsedSeconds = nowSecond;
			AddExpiredTerminalFillers(
				retained,
				string.Format("quarantine_retention_%1", cycle),
				HST_ForceSpawnQueueService.MAX_TERMINAL_ROWS + 4);
			HST_ForceSpawnQueueRetentionPins pins
				= new HST_ForceSpawnQueueRetentionPins();
			HST_EnemyCounterattackRetentionService.AddQuarantinedSpawnPins(
				retained,
				pins);
			HST_ForceSpawnResultState retainedBatch
				= retained.FindForceSpawnResult(batchId);
			HST_EnemyOrderState retainedOrder = retained.FindEnemyOrder(orderId);
			HST_OperationRecordState retainedOperation = retained.FindOperation(operationId);
			bool directLinksBroken = retainedBatch && retainedOrder
				&& retainedBatch.m_sRequestId != retainedOrder.m_sOrderId
				&& retainedBatch.m_sResultId != retainedOrder.m_sSpawnResultId
				&& retainedBatch.m_sManifestId != retainedOrder.m_sManifestId
				&& retainedBatch.m_sOperationId != retainedOrder.m_sOperationId;
			bool reciprocalRootExact = retainedOperation
				&& retainedOperation.m_sEnemyOrderId == orderId
				&& retainedOperation.m_sSpawnResultId == batchId
				&& retainedOperation.m_sManifestId == manifestId;
			bool reciprocalPinExact = retainedBatch
				&& pins.Contains(retainedBatch)
				&& pins.m_aResultIds.Contains(batchId)
				&& pins.m_aManifestIds.Contains(manifestId)
				&& pins.m_aOperationIds.Contains(operationId)
				&& directLinksBroken && reciprocalRootExact;
			HST_ForceSpawnQueueMaintenanceResult maintenance
				= queue.CompactTerminalRows(
					retained.m_aForceSpawnResults,
					pins,
					nowSecond);
			bool compactionExact = maintenance
				&& maintenance.m_iPinnedTerminalCount >= 1
				&& maintenance.m_iRemovedTerminalCount > 0
				&& retained.FindForceSpawnResult(batchId) == retainedBatch;
			if (maintenance)
				compactedRows += maintenance.m_iRemovedTerminalCount;
			if (reciprocalPinExact && compactionExact)
				pinnedPasses++;
			else
				cyclesExact = false;

			fixture.m_Planning.PrunePlanningRecords(retained);
			HST_CampaignSaveData replay = new HST_CampaignSaveData();
			replay.Capture(retained);
			retained = replay.Restore();
			bool replayExact = IsQuarantineRetentionGraphExact(
				retained,
				orderId,
				operationId,
				manifestId,
				batchId,
				groupId);
			if (replayExact)
				persistencePasses++;
			else
				cyclesExact = false;
		}

		report.m_bQuarantineRetentionExact = cyclesExact
			&& pinnedPasses == 3 && persistencePasses == 3
			&& compactedRows > HST_ForceSpawnQueueService.MAX_TERMINAL_ROWS;
		int finalOrderCount;
		if (retained)
			finalOrderCount = CountEnemyOrderId(retained, orderId);
		report.m_sRetentionEvidence = string.Format(
			"initial %1 | reciprocal pin passes %2/3 | save/restore passes %3/3 | compacted fillers %4",
			initialExact,
			pinnedPasses,
			persistencePasses,
			compactedRows);
		report.m_sRetentionEvidence = report.m_sRetentionEvidence + string.Format(
			" | retained order/operation/manifest/batch/group %1/%2/%3/%4/%5",
			finalOrderCount,
			CountOperationId(retained, operationId),
			CountManifestId(retained, manifestId),
			CountBatchId(retained, batchId),
			CountGroupId(retained, groupId));
	}

	protected bool IsQuarantineRetentionGraphExact(
		HST_CampaignState state,
		string orderId,
		string operationId,
		string manifestId,
		string batchId,
		string missingGroupId)
	{
		if (!state)
			return false;
		HST_EnemyOrderState order = state.FindEnemyOrder(orderId);
		HST_OperationRecordState operation = state.FindOperation(operationId);
		HST_ForceManifestState manifest = state.FindForceManifest(manifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(batchId);
		bool orderExact = order
			&& order.m_iOperationContractVersion
				== HST_EnemyCounterattackSaveValidationService.QUARANTINED_CONTRACT_VERSION
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& order.m_sRuntimeStatus == "exact_counterattack_quarantined"
			&& order.m_sSpawnResultId
				== "missing_quarantine_retention_result_" + orderId
			&& order.m_sOperationId
				== "missing_quarantine_retention_operation_" + orderId
			&& order.m_sManifestId
				== "missing_quarantine_retention_manifest_" + orderId
			&& order.m_sGroupId
				== "missing_quarantine_retention_group_" + orderId;
		bool batchExact = batch
			&& batch.m_eStatus
				== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			&& batch.m_bStrategicProjectionHeld
			&& !batch.m_bCancelRequested
			&& batch.m_sRequestId
				== "unrelated_quarantine_retention_request_" + orderId
			&& batch.m_sOperationId == operationId
			&& batch.m_sManifestId == manifestId;
		bool reciprocalRootExact = operation
			&& operation.m_sEnemyOrderId == orderId
			&& operation.m_sSpawnResultId == batchId
			&& operation.m_sManifestId == manifestId;
		bool rootsExact = CountEnemyOrderId(state, orderId) == 1
			&& CountOperationId(state, operationId) == 1
			&& CountManifestId(state, manifestId) == 1
			&& CountBatchId(state, batchId) == 1
			&& CountGroupId(state, missingGroupId) == 0
			&& state.FindActiveGroup(order.m_sGroupId) == null;
		return orderExact && manifest && batchExact
			&& reciprocalRootExact && rootsExact;
	}

	protected void AddExpiredTerminalFillers(
		HST_CampaignState state,
		string prefix,
		int count)
	{
		if (!state)
			return;
		for (int index = 0; index < count; index++)
		{
			string identity = string.Format("%1_%2", prefix, index);
			HST_ForceSpawnResultState filler = new HST_ForceSpawnResultState();
			filler.m_sResultId = "spawn_filler_" + identity;
			filler.m_sRequestId = "request_filler_" + identity;
			filler.m_sManifestId = "manifest_filler_" + identity;
			filler.m_sOperationId = "operation_filler_" + identity;
			filler.m_sForceId = "force_filler_" + identity;
			filler.m_sProjectionId = "projection_filler_" + identity;
			filler.m_eStatus
				= HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
			filler.m_sTerminalReason = "focused proof expired terminal filler";
			filler.m_iCreatedAtSecond = 1;
			filler.m_iUpdatedAtSecond = 1;
			filler.m_iCompletedAtSecond = 1;
			state.m_aForceSpawnResults.Insert(filler);
		}
	}

	protected bool RemoveGroup(
		array<ref HST_ActiveGroupState> groups,
		string groupId)
	{
		if (!groups || groupId.IsEmpty())
			return false;
		for (int index = groups.Count() - 1; index >= 0; index--)
		{
			HST_ActiveGroupState group = groups[index];
			if (!group || group.m_sGroupId != groupId)
				continue;
			groups.Remove(index);
			return true;
		}
		return false;
	}

	protected void PrepareSyntheticSuccessfulProjection(
		HST_EnemyCounterattackOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_Batch || !fixture.m_Manifest)
			return;
		fixture.m_Batch.m_eStatus
			= HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
		fixture.m_Batch.m_bStrategicProjectionHeld = false;
		fixture.m_Batch.m_sNativeGroupId
			= "enemy_counterattack_proof_native_" + fixture.m_Order.m_sOrderId;
		fixture.m_Batch.m_sTerminalReason
			= "all exact manifest slots registered and verified";
		fixture.m_Batch.m_sLastFailureReason = "";
		fixture.m_Batch.m_iSuccessfulHandoffCount = 1;
		fixture.m_Batch.m_iCompletedAtSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Batch.m_iUpdatedAtSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Batch.m_iLifecycleRevision++;
		fixture.m_Batch.m_iLastLifecycleSecond = fixture.m_State.m_iElapsedSeconds;
		foreach (HST_ForceSpawnSlotResultState slot : fixture.m_Batch.m_aSlotResults)
		{
			if (!slot)
				continue;
			slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
			slot.m_sSpawnedPrefab = ResolveFixtureSlotPrefab(
				fixture.m_Manifest,
				slot);
			slot.m_sEntityId = "enemy_counterattack_proof_entity_" + slot.m_sSlotId;
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

	protected bool DriveUntilReturning(
		HST_EnemyCounterattackOperationProofFixture fixture,
		int maxTicks)
	{
		if (!m_Fixtures.Ready(fixture))
			return false;
		for (int tick = 0; tick < maxTicks; tick++)
		{
			if (fixture.m_Operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN)
				return true;
			fixture.m_State.m_iElapsedSeconds
				+= HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
			fixture.m_Service.TickOrder(
				fixture.m_State,
				fixture.m_Preset,
				fixture.m_EnemyDirector,
				fixture.m_Order);
		}
		return fixture.m_Operation.m_eDutyState
			== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN;
	}

	protected bool IsReturningRestoreExact(
		HST_CampaignState state,
		string orderId)
	{
		if (!state)
			return false;
		HST_EnemyOrderState order = state.FindEnemyOrder(orderId);
		if (!order || order.m_iOperationContractVersion
			!= HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION)
			return false;
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(order.m_sSpawnResultId);
		HST_ActiveGroupState group = state.FindActiveGroup(order.m_sGroupId);
		if (!operation || !manifest || !batch || !group)
			return false;
		bool lifecycleExact = order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN;
		bool routeExact = Distance2D(operation.m_vRouteEndPosition, operation.m_vOriginPosition) <= 0.01
			&& operation.m_sTacticalTargetZoneId == operation.m_sOriginZoneId
			&& Distance2D(operation.m_vTacticalTargetPosition, operation.m_vOriginPosition) <= 0.01;
		bool rosterExact = batch.m_bStrategicProjectionHeld && !batch.m_bCancelRequested
			&& !group.m_bSpawnedEntity && group.m_sRuntimeEntityId.IsEmpty();
		string debitFailure
			= HST_EnemyCounterattackSaveValidationService.ValidateOriginalResourceDebitAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		bool ledgerExact = debitFailure.IsEmpty()
			&& !order.m_bResourceSettlementApplied
			&& order.m_sResourceRefundMutationId.IsEmpty();
		return lifecycleExact && routeExact && rosterExact && ledgerExact;
	}

	protected bool IsSettledRestoreExact(
		HST_CampaignState state,
		string orderId)
	{
		if (!state)
			return false;
		HST_EnemyOrderState order = state.FindEnemyOrder(orderId);
		if (!order || order.m_iOperationContractVersion
			!= HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION)
			return false;
		HST_OperationRecordState operation = state.FindOperation(order.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(order.m_sManifestId);
		if (!operation || !manifest)
			return false;
		bool lifecycleExact = operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			&& operation.m_sSettlementId == order.m_sResourceSettlementId
			&& order.m_bResourceSettlementApplied;
		string debitFailure
			= HST_EnemyCounterattackSaveValidationService.ValidateOriginalResourceDebitAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		string refundFailure
			= HST_EnemyCounterattackSaveValidationService.ValidateSettledResourceRefundAuthority(
				state.m_aEnemyStrategicMutations,
				order);
		return lifecycleExact && debitFailure.IsEmpty() && refundFailure.IsEmpty()
			&& CountMutationId(state, order.m_sResourceRefundMutationId) == 1;
	}

	protected bool ProvePreparedBeforeRefundRestore()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("prepared_pre_refund");
		string reason = "focused proof prepared before refund";
		if (!StagePreparedSettlement(
			fixture,
			"route_failed_survivors",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
			reason,
			true))
			return false;

		string orderId = fixture.m_Order.m_sOrderId;
		string refundId = fixture.m_Order.m_sResourceRefundMutationId;
		int preparedAt = fixture.m_Operation.m_iSettledAtSecond;
		HST_FactionPoolState sourcePool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!sourcePool)
			return false;
		int attackBeforeResume = sourcePool.m_iAttackResources;
		int expectedAttackAfter = attackBeforeResume
			+ fixture.m_Order.m_iRefundedAttackResources;

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		if (!restored)
			return false;
		HST_EnemyOrderState restoredOrder = restored.FindEnemyOrder(orderId);
		HST_OperationRecordState restoredOperation;
		if (restoredOrder)
			restoredOperation = restored.FindOperation(restoredOrder.m_sOperationId);
		bool preparedExact = restoredOrder && restoredOperation
			&& !restoredOrder.m_bResourceSettlementApplied
			&& CountMutationId(restored, refundId) == 0
			&& restoredOperation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
			&& restoredOperation.m_iSettledAtSecond == preparedAt;
		bool firstChanged = fixture.m_Service.ReconcileAfterRestore(
			restored,
			fixture.m_EnemyDirector);
		restoredOrder = restored.FindEnemyOrder(orderId);
		if (restoredOrder)
			restoredOperation = restored.FindOperation(restoredOrder.m_sOperationId);
		HST_FactionPoolState restoredPool = restored.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool firstExact = firstChanged && restoredPool && restoredOperation
			&& IsSettledRestoreExact(restored, orderId)
			&& restoredPool.m_iAttackResources == expectedAttackAfter
			&& restoredOperation.m_iSettledAtSecond == preparedAt;
		int firstRevision;
		if (restoredOperation)
			firstRevision = restoredOperation.m_iRevision;
		bool sameStateReplayChanged = fixture.m_Service.ReconcileAfterRestore(
			restored,
			fixture.m_EnemyDirector);
		bool sameStateReplayExact = !sameStateReplayChanged && restoredOperation
			&& restoredPool
			&& restoredOperation.m_iRevision == firstRevision
			&& restoredPool.m_iAttackResources == expectedAttackAfter
			&& CountMutationId(restored, refundId) == 1;

		HST_CampaignSaveData replaySave = new HST_CampaignSaveData();
		replaySave.Capture(restored);
		HST_CampaignState replayRestored = replaySave.Restore();
		bool replayChanged;
		int replayAttack;
		if (replayRestored)
		{
			HST_FactionPoolState replayPool = replayRestored.FindFactionPool(
				HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
			if (replayPool)
				replayAttack = replayPool.m_iAttackResources;
			replayChanged = fixture.m_Service.ReconcileAfterRestore(
				replayRestored,
				fixture.m_EnemyDirector);
		}
		bool replayExact = replayRestored && !replayChanged
			&& IsSettledRestoreExact(replayRestored, orderId)
			&& replayAttack == expectedAttackAfter
			&& CountMutationId(replayRestored, refundId) == 1;
		Print(string.Format(
			"Partisan prepared pre-refund restore proof | prepared/first/replay %1/%2/%3 | pool %4 -> %5 | mutations %6",
			preparedExact,
			firstExact,
			sameStateReplayExact && replayExact,
			attackBeforeResume,
			expectedAttackAfter,
			CountMutationId(restored, refundId)));
		return preparedExact && firstExact && sameStateReplayExact && replayExact;
	}

	protected bool ProvePreparedAfterRefundRestore()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("prepared_post_refund");
		string reason = "focused proof prepared after refund";
		if (!StagePreparedSettlement(
			fixture,
			"route_failed_survivors",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
			reason,
			true)
			|| !ApplyPreparedRefund(fixture, reason))
			return false;

		string orderId = fixture.m_Order.m_sOrderId;
		string refundId = fixture.m_Order.m_sResourceRefundMutationId;
		HST_FactionPoolState sourcePool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!sourcePool)
			return false;
		int attackAfterRefund = sourcePool.m_iAttackResources;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		if (!restored)
			return false;
		HST_EnemyOrderState restoredOrder = restored.FindEnemyOrder(orderId);
		HST_OperationRecordState restoredOperation;
		if (restoredOrder)
			restoredOperation = restored.FindOperation(restoredOrder.m_sOperationId);
		bool preparedExact = restoredOrder && restoredOperation
			&& !restoredOrder.m_bResourceSettlementApplied
			&& CountMutationId(restored, refundId) == 1
			&& restoredOperation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED;
		bool firstChanged = fixture.m_Service.ReconcileAfterRestore(
			restored,
			fixture.m_EnemyDirector);
		restoredOrder = restored.FindEnemyOrder(orderId);
		if (restoredOrder)
			restoredOperation = restored.FindOperation(restoredOrder.m_sOperationId);
		HST_FactionPoolState restoredPool = restored.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool firstExact = firstChanged && restoredPool && restoredOperation
			&& IsSettledRestoreExact(restored, orderId)
			&& restoredPool.m_iAttackResources == attackAfterRefund
			&& CountMutationId(restored, refundId) == 1;
		int settledRevision;
		if (restoredOperation)
			settledRevision = restoredOperation.m_iRevision;
		bool replayChanged = fixture.m_Service.ReconcileAfterRestore(
			restored,
			fixture.m_EnemyDirector);
		bool replayExact = !replayChanged && restoredOperation && restoredPool
			&& restoredOperation.m_iRevision == settledRevision
			&& restoredPool.m_iAttackResources == attackAfterRefund
			&& CountMutationId(restored, refundId) == 1;
		Print(string.Format(
			"Partisan prepared post-refund restore proof | prepared/first/replay %1/%2/%3 | pool %4 | mutations %5",
			preparedExact,
			firstExact,
			replayExact,
			attackAfterRefund,
			CountMutationId(restored, refundId)));
		return preparedExact && firstExact && replayExact;
	}

	protected bool ProvePreparedAfterRecordRestore()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("prepared_post_record");
		string reason = "focused proof prepared after resource record";
		if (!StagePreparedSettlement(
			fixture,
			"route_failed_survivors",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
			reason,
			true)
			|| !ApplyPreparedRefund(fixture, reason))
			return false;
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult recorded
			= operations.RecordExactEnemyCounterattackResourceSettlement(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Order.m_sResourceSettlementKind,
				fixture.m_Order.m_iSettlementAcceptedMemberCount,
				fixture.m_Order.m_iSettlementSurvivorMemberCount);
		if (!recorded || !recorded.m_bAccepted
			|| !fixture.m_Order.m_bResourceSettlementApplied
			|| fixture.m_Operation.m_eSettlementState
				!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
			return false;

		string orderId = fixture.m_Order.m_sOrderId;
		string refundId = fixture.m_Order.m_sResourceRefundMutationId;
		HST_FactionPoolState sourcePool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!sourcePool)
			return false;
		int attackAfterRecord = sourcePool.m_iAttackResources;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		if (!restored)
			return false;
		HST_EnemyOrderState restoredOrder = restored.FindEnemyOrder(orderId);
		HST_OperationRecordState restoredOperation;
		if (restoredOrder)
			restoredOperation = restored.FindOperation(restoredOrder.m_sOperationId);
		int preparedRevision;
		if (restoredOperation)
			preparedRevision = restoredOperation.m_iRevision;
		bool preparedExact = restoredOrder && restoredOperation
			&& restoredOrder.m_bResourceSettlementApplied
			&& restoredOperation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
			&& CountMutationId(restored, refundId) == 1;
		bool firstChanged = fixture.m_Service.ReconcileAfterRestore(
			restored,
			fixture.m_EnemyDirector);
		restoredOrder = restored.FindEnemyOrder(orderId);
		if (restoredOrder)
			restoredOperation = restored.FindOperation(restoredOrder.m_sOperationId);
		HST_FactionPoolState restoredPool = restored.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool firstExact = firstChanged && restoredPool && restoredOperation
			&& IsSettledRestoreExact(restored, orderId)
			&& restoredOperation.m_iRevision == preparedRevision + 1
			&& restoredPool.m_iAttackResources == attackAfterRecord
			&& CountMutationId(restored, refundId) == 1;
		int settledRevision;
		if (restoredOperation)
			settledRevision = restoredOperation.m_iRevision;
		bool replayChanged = fixture.m_Service.ReconcileAfterRestore(
			restored,
			fixture.m_EnemyDirector);
		bool replayExact = !replayChanged && restoredOperation && restoredPool
			&& restoredOperation.m_iRevision == settledRevision
			&& restoredPool.m_iAttackResources == attackAfterRecord
			&& CountMutationId(restored, refundId) == 1;
		Print(string.Format(
			"Partisan prepared post-record restore proof | prepared/first/replay %1/%2/%3 | revision %4 -> %5 | mutations %6",
			preparedExact,
			firstExact,
			replayExact,
			preparedRevision,
			settledRevision,
			CountMutationId(restored, refundId)));
		return preparedExact && firstExact && replayExact;
	}

	protected bool ProveUncommittedFullPreparedRestore()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("prepared_uncommitted_full");
		if (!m_Fixtures.Ready(fixture))
			return false;
		string operationId = fixture.m_Operation.m_sOperationId;
		string manifestId = fixture.m_Manifest.m_sManifestId;
		string batchId = fixture.m_Batch.m_sResultId;
		string groupId = fixture.m_Group.m_sGroupId;
		fixture.m_Order.m_bStrategicServiceCommitted = false;
		fixture.m_Order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		fixture.m_Order.m_sSpawnResultId = "";
		fixture.m_Order.m_sGroupId = "";
		fixture.m_Order.m_bPhysicalized = false;
		fixture.m_Order.m_bAbstractResolved = false;
		int operationIndex = fixture.m_State.m_aOperations.Find(fixture.m_Operation);
		if (operationIndex >= 0)
			fixture.m_State.m_aOperations.Remove(operationIndex);
		int manifestIndex = fixture.m_State.m_aForceManifests.Find(fixture.m_Manifest);
		if (manifestIndex >= 0)
			fixture.m_State.m_aForceManifests.Remove(manifestIndex);
		int batchIndex = fixture.m_State.m_aForceSpawnResults.Find(fixture.m_Batch);
		if (batchIndex >= 0)
			fixture.m_State.m_aForceSpawnResults.Remove(batchIndex);
		int groupIndex = fixture.m_State.m_aActiveGroups.Find(fixture.m_Group);
		if (groupIndex >= 0)
			fixture.m_State.m_aActiveGroups.Remove(groupIndex);
		string reason = "focused proof uncommitted full prepared rollback";
		if (!StagePreparedSettlement(
			fixture,
			"admission_failed_full",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
			reason,
			false))
			return false;

		string orderId = fixture.m_Order.m_sOrderId;
		string refundId = fixture.m_Order.m_sResourceRefundMutationId;
		HST_FactionPoolState sourcePool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!sourcePool)
			return false;
		int attackBeforeResume = sourcePool.m_iAttackResources;
		int expectedAttackAfter = fixture.m_iAttackBeforeDebit;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		if (!restored)
			return false;
		HST_EnemyOrderState restoredOrder = restored.FindEnemyOrder(orderId);
		bool preparedExact = restoredOrder
			&& restoredOrder.m_iOperationContractVersion
				== HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION
			&& !restoredOrder.m_bResourceSettlementApplied
			&& !restored.FindOperation(operationId)
			&& !restored.FindForceManifest(manifestId)
			&& !restored.FindForceSpawnResult(batchId)
			&& !restored.FindActiveGroup(groupId)
			&& CountMutationId(restored, refundId) == 0;
		bool firstChanged = fixture.m_Service.ReconcileAfterRestore(
			restored,
			fixture.m_EnemyDirector);
		restoredOrder = restored.FindEnemyOrder(orderId);
		HST_FactionPoolState restoredPool = restored.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool firstExact = firstChanged && restoredOrder && restoredPool
			&& restoredOrder.m_iOperationContractVersion
				== HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION
			&& restoredOrder.m_bResourceSettlementApplied
			&& restoredOrder.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& restoredOrder.m_sRuntimeStatus == "resolved_exact_terminal"
			&& restoredPool.m_iAttackResources == expectedAttackAfter
			&& CountMutationId(restored, refundId) == 1
			&& !restored.FindOperation(operationId)
			&& !restored.FindForceManifest(manifestId);
		bool sameStateReplayChanged = fixture.m_Service.ReconcileAfterRestore(
			restored,
			fixture.m_EnemyDirector);
		bool sameStateReplayExact = !sameStateReplayChanged && restoredPool
			&& restoredPool.m_iAttackResources == expectedAttackAfter
			&& CountMutationId(restored, refundId) == 1;

		HST_CampaignSaveData replaySave = new HST_CampaignSaveData();
		replaySave.Capture(restored);
		HST_CampaignState replayRestored = replaySave.Restore();
		bool replayChanged;
		HST_EnemyOrderState replayOrder;
		HST_FactionPoolState replayPool;
		if (replayRestored)
		{
			replayOrder = replayRestored.FindEnemyOrder(orderId);
			replayPool = replayRestored.FindFactionPool(
				HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
			replayChanged = fixture.m_Service.ReconcileAfterRestore(
				replayRestored,
				fixture.m_EnemyDirector);
		}
		bool replayExact = replayRestored && replayOrder && replayPool
			&& !replayChanged
			&& replayOrder.m_iOperationContractVersion
				== HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION
			&& replayOrder.m_bResourceSettlementApplied
			&& replayPool.m_iAttackResources == expectedAttackAfter
			&& CountMutationId(replayRestored, refundId) == 1
			&& !replayRestored.FindOperation(operationId)
			&& !replayRestored.FindForceManifest(manifestId);
		Print(string.Format(
			"Partisan uncommitted full prepared restore proof | prepared/first/replay %1/%2/%3 | pool %4 -> %5 | mutations %6",
			preparedExact,
			firstExact,
			sameStateReplayExact && replayExact,
			attackBeforeResume,
			expectedAttackAfter,
			CountMutationId(restored, refundId)));
		return preparedExact && firstExact && sameStateReplayExact && replayExact;
	}

	protected bool ProveAbortedPreparedSameSessionConsumption()
	{
		HST_EnemyCounterattackOperationProofFixture operationBacked
			= m_Fixtures.BuildAdmittedFixture("prepared_same_session_operation");
		string operationReason = "focused proof aborted prepared operation same-session retry";
		if (!StagePreparedSettlement(
			operationBacked,
			"route_failed_survivors",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
			operationReason,
			true))
			return false;
		operationBacked.m_Order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		HST_FactionPoolState operationPool = operationBacked.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!operationPool)
			return false;
		int operationExpectedAttack = operationPool.m_iAttackResources
			+ operationBacked.m_Order.m_iRefundedAttackResources;
		string operationRefundId = operationBacked.m_Order.m_sResourceRefundMutationId;
		bool operationBlockedBefore
			= operationBacked.m_Service.HasOpenExactEnemyCounterattack(
				operationBacked.m_State,
				operationBacked.m_Order.m_sFactionKey,
				operationBacked.m_Order.m_sTargetZoneId);
		bool operationFirst = operationBacked.m_Service.TickOrder(
			operationBacked.m_State,
			operationBacked.m_Preset,
			operationBacked.m_EnemyDirector,
			operationBacked.m_Order);
		bool operationReplay = operationBacked.m_Service.TickOrder(
			operationBacked.m_State,
			operationBacked.m_Preset,
			operationBacked.m_EnemyDirector,
			operationBacked.m_Order);
		bool operationBlockedAfter
			= operationBacked.m_Service.HasOpenExactEnemyCounterattack(
				operationBacked.m_State,
				operationBacked.m_Order.m_sFactionKey,
				operationBacked.m_Order.m_sTargetZoneId);
		bool operationExact = operationFirst && !operationReplay
			&& operationBlockedBefore && !operationBlockedAfter
			&& operationPool.m_iAttackResources == operationExpectedAttack
			&& CountMutationId(operationBacked.m_State, operationRefundId) == 1
			&& IsSettledRestoreExact(
				operationBacked.m_State,
				operationBacked.m_Order.m_sOrderId);

		HST_EnemyCounterattackOperationProofFixture operationless
			= m_Fixtures.BuildAdmittedFixture("prepared_same_session_full");
		if (!m_Fixtures.Ready(operationless))
			return false;
		operationless.m_Order.m_bStrategicServiceCommitted = false;
		operationless.m_Order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
		operationless.m_Order.m_sSpawnResultId = "";
		operationless.m_Order.m_sGroupId = "";
		operationless.m_Order.m_bPhysicalized = false;
		operationless.m_Order.m_bAbstractResolved = false;
		int operationIndex = operationless.m_State.m_aOperations.Find(operationless.m_Operation);
		if (operationIndex >= 0)
			operationless.m_State.m_aOperations.Remove(operationIndex);
		int manifestIndex = operationless.m_State.m_aForceManifests.Find(operationless.m_Manifest);
		if (manifestIndex >= 0)
			operationless.m_State.m_aForceManifests.Remove(manifestIndex);
		int batchIndex = operationless.m_State.m_aForceSpawnResults.Find(operationless.m_Batch);
		if (batchIndex >= 0)
			operationless.m_State.m_aForceSpawnResults.Remove(batchIndex);
		int groupIndex = operationless.m_State.m_aActiveGroups.Find(operationless.m_Group);
		if (groupIndex >= 0)
			operationless.m_State.m_aActiveGroups.Remove(groupIndex);
		string fullReason = "focused proof aborted operationless full same-session retry";
		if (!StagePreparedSettlement(
			operationless,
			"admission_failed_full",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
			fullReason,
			false))
			return false;
		HST_FactionPoolState fullPool = operationless.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!fullPool)
			return false;
		string fullRefundId = operationless.m_Order.m_sResourceRefundMutationId;
		bool fullBlockedBefore = operationless.m_Service.HasOpenExactEnemyCounterattack(
			operationless.m_State,
			operationless.m_Order.m_sFactionKey,
			operationless.m_Order.m_sTargetZoneId);
		bool fullFirst = operationless.m_Service.SettleOpenOrdersForCampaignStop(
			operationless.m_State,
			operationless.m_EnemyDirector,
			"focused proof campaign stop consumes aborted prepared settlement");
		bool fullReplay = operationless.m_Service.SettleOpenOrdersForCampaignStop(
			operationless.m_State,
			operationless.m_EnemyDirector,
			"focused proof campaign stop prepared settlement replay");
		bool fullTickReplay = operationless.m_Service.TickOrder(
			operationless.m_State,
			operationless.m_Preset,
			operationless.m_EnemyDirector,
			operationless.m_Order);
		bool fullBlockedAfter = operationless.m_Service.HasOpenExactEnemyCounterattack(
			operationless.m_State,
			operationless.m_Order.m_sFactionKey,
			operationless.m_Order.m_sTargetZoneId);
		bool fullExact = fullFirst && !fullReplay && !fullTickReplay
			&& fullBlockedBefore && !fullBlockedAfter
			&& operationless.m_Order.m_bResourceSettlementApplied
			&& operationless.m_Order.m_eStatus
				== HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& operationless.m_Order.m_sRuntimeStatus == "resolved_exact_terminal"
			&& fullPool.m_iAttackResources == operationless.m_iAttackBeforeDebit
			&& CountMutationId(operationless.m_State, fullRefundId) == 1;
		Print(string.Format(
			"Partisan aborted prepared same-session proof | operation/full %1/%2 | blocked before %3/%4 after %5/%6 | mutations %7/%8",
			operationExact,
			fullExact,
			operationBlockedBefore,
			fullBlockedBefore,
			operationBlockedAfter,
			fullBlockedAfter,
			CountMutationId(operationBacked.m_State, operationRefundId),
			CountMutationId(operationless.m_State, fullRefundId)));
		return operationExact && fullExact;
	}

	protected bool ProveForgedOpenPreparedRefundQuarantine()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("forged_open_pending");
		string reason = "focused proof forged open pending refund";
		if (!StagePreparedSettlement(
			fixture,
			"route_failed_survivors",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
			reason,
			false)
			|| !ApplyPreparedRefund(fixture, reason))
			return false;
		if (fixture.m_Operation.m_eSettlementState
			!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
			return false;
		string aggregateFailure
			= HST_EnemyCounterattackSaveValidationService.ValidatePendingResourceRefundAggregateAuthority(
				fixture.m_State.m_aEnemyStrategicMutations,
				fixture.m_Order,
				fixture.m_Operation,
				fixture.m_Manifest,
				fixture.m_Batch,
				fixture.m_Group);
		HST_FactionPoolState sourcePool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!sourcePool)
			return false;
		int attackAfterForgery = sourcePool.m_iAttackResources;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		if (!restored)
			return false;
		HST_EnemyOrderState order = restored.FindEnemyOrder(fixture.m_Order.m_sOrderId);
		HST_OperationRecordState operation;
		if (order)
			operation = restored.FindOperation(order.m_sOperationId);
		HST_FactionPoolState pool = restored.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool quarantined = !aggregateFailure.IsEmpty() && order && pool
			&& order.m_iOperationContractVersion
				== HST_EnemyCounterattackSaveValidationService.QUARANTINED_CONTRACT_VERSION
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& order.m_sRuntimeStatus == "exact_counterattack_quarantined"
			&& !order.m_bResourceSettlementApplied
			&& pool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceSaveValidationService.QUARANTINE_CONTRACT_VERSION
			&& pool.m_iAttackResources == attackAfterForgery
			&& operation && operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		Print(string.Format(
			"Partisan forged open pending refund proof | rejected/quarantined %1/%2 | pool contract %3 | reason '%4'",
			!aggregateFailure.IsEmpty(),
			quarantined,
			pool && pool.m_iStrategicContractVersion,
			aggregateFailure));
		return quarantined;
	}

	protected bool ProvePreparedPhysicalBindingLossRestore()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("prepared_physical_binding_loss");
		if (!m_Fixtures.Ready(fixture))
			return false;
		HST_ForceSpawnQueueCallbackResult released
			= fixture.m_Queue.ReleaseStrategicProjectionForMaterialization(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				fixture.m_State.m_iElapsedSeconds,
				fixture.m_State.m_iElapsedSeconds + 180);
		HST_OperationService operations = new HST_OperationService();
		HST_OperationTransitionResult materializing
			= operations.MarkExactEnemyCounterattackMaterializingFromVirtual(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				fixture.m_Batch,
				"focused proof entered physical authority before binding loss");
		PrepareSyntheticSuccessfulProjection(fixture);
		fixture.m_Group.m_bSpawnedEntity = true;
		fixture.m_Group.m_sRuntimeEntityId = fixture.m_Batch.m_sNativeGroupId;
		fixture.m_Group.m_iSpawnedAgentCount
			= fixture.m_Manifest.m_iAcceptedMemberCount;
		HST_OperationTransitionResult physical
			= operations.MarkExactEnemyCounterattackPhysical(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				fixture.m_Batch,
				"focused proof established physical authority before binding loss");
		HST_OperationTransitionResult dematerializing
			= operations.BeginExactEnemyCounterattackDematerialization(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				"focused proof began dematerialization before binding loss");
		if (!released || !released.m_bAccepted
			|| !materializing || !materializing.m_bAccepted
			|| !physical || !physical.m_bAccepted
			|| !dematerializing || !dematerializing.m_bAccepted)
			return false;

		// Process-local bindings are gone, while the successful batch remains the
		// durable upper bound. Zero is therefore conservative, not invented.
		fixture.m_Group.m_bSpawnedEntity = false;
		fixture.m_Group.m_sRuntimeEntityId = "";
		fixture.m_Group.m_iSpawnedAgentCount = 0;
		int durableLiving = fixture.m_Queue.CountDurableLivingMemberSlots(
			fixture.m_Batch);
		string reason = "focused proof physical binding loss settled conservatively";
		if (durableLiving <= 0
			|| !StagePreparedSettlementWithSurvivors(
				fixture,
				"materialization_failed_survivors",
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
				reason,
				true,
				0))
			return false;
		string aggregateFailure
			= HST_EnemyCounterattackSaveValidationService.ValidatePendingResourceRefundAggregateAuthority(
				fixture.m_State.m_aEnemyStrategicMutations,
				fixture.m_Order,
				fixture.m_Operation,
				fixture.m_Manifest,
				fixture.m_Batch,
				fixture.m_Group);
		if (!aggregateFailure.IsEmpty())
			return false;

		string orderId = fixture.m_Order.m_sOrderId;
		string refundId = fixture.m_Order.m_sResourceRefundMutationId;
		int preparedAt = fixture.m_Operation.m_iSettledAtSecond;
		HST_FactionPoolState sourcePool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!sourcePool)
			return false;
		int attackBeforeResume = sourcePool.m_iAttackResources;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		if (!restored)
			return false;
		HST_EnemyOrderState restoredOrder = restored.FindEnemyOrder(orderId);
		HST_OperationRecordState restoredOperation;
		HST_ForceSpawnResultState restoredBatch;
		if (restoredOrder)
		{
			restoredOperation = restored.FindOperation(restoredOrder.m_sOperationId);
			restoredBatch = restored.FindForceSpawnResult(restoredOrder.m_sSpawnResultId);
		}
		HST_ForceSpawnQueueService restoredQueue = new HST_ForceSpawnQueueService();
		bool preparedExact = restoredOrder && restoredOperation && restoredBatch
			&& restoredOrder.m_iSettlementSurvivorMemberCount == 0
			&& restoredOperation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
			&& restoredOperation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING
			&& restoredQueue.CountDurableLivingMemberSlots(restoredBatch)
				== durableLiving
			&& CountMutationId(restored, refundId) == 0;
		bool firstChanged = fixture.m_Service.ReconcileAfterRestore(
			restored,
			fixture.m_EnemyDirector);
		restoredOrder = restored.FindEnemyOrder(orderId);
		if (restoredOrder)
			restoredOperation = restored.FindOperation(restoredOrder.m_sOperationId);
		HST_FactionPoolState restoredPool = restored.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		HST_EnemyStrategicMutationState refund = FindMutation(
			restored.m_aEnemyStrategicMutations,
			refundId);
		bool firstExact = firstChanged && restoredOrder && restoredOperation
			&& restoredPool && refund
			&& IsSettledRestoreExact(restored, orderId)
			&& restoredOrder.m_iRefundedAttackResources == 0
			&& restoredOrder.m_iRefundedSupportResources == 0
			&& restoredOperation.m_iSettledAtSecond == preparedAt
			&& restoredPool.m_iAttackResources == attackBeforeResume
			&& refund.m_iAttackDelta == 0 && refund.m_iSupportDelta == 0
			&& CountMutationId(restored, refundId) == 1;
		int settledRevision;
		if (restoredOperation)
			settledRevision = restoredOperation.m_iRevision;
		bool replayChanged = fixture.m_Service.ReconcileAfterRestore(
			restored,
			fixture.m_EnemyDirector);
		bool replayExact = !replayChanged && restoredOperation && restoredPool
			&& restoredOperation.m_iRevision == settledRevision
			&& restoredPool.m_iAttackResources == attackBeforeResume
			&& CountMutationId(restored, refundId) == 1;
		Print(string.Format(
			"Partisan prepared physical binding-loss proof | durable/staged %1/%2 | prepared/settled/replay %3/%4/%5 | pool %6 | mutations %7",
			durableLiving,
			0,
			preparedExact,
			firstExact,
			replayExact,
			attackBeforeResume,
			CountMutationId(restored, refundId)));
		return preparedExact && firstExact && replayExact;
	}

	protected bool ProvePreparedDestroyedLivingQuarantine()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("forged_destroyed_living");
		string reason = "focused proof forged destroyed intent with living roster";
		if (!StagePreparedSettlementWithSurvivors(
			fixture,
			"destroyed",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED,
			reason,
			true,
			0))
			return false;
		int durableLiving = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		string aggregateFailure
			= HST_EnemyCounterattackSaveValidationService.ValidatePendingResourceRefundAggregateAuthority(
				fixture.m_State.m_aEnemyStrategicMutations,
				fixture.m_Order,
				fixture.m_Operation,
				fixture.m_Manifest,
				fixture.m_Batch,
				fixture.m_Group);
		HST_FactionPoolState sourcePool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!sourcePool)
			return false;
		int attackBeforeRestore = sourcePool.m_iAttackResources;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		if (!restored)
			return false;
		HST_EnemyOrderState order = restored.FindEnemyOrder(fixture.m_Order.m_sOrderId);
		HST_FactionPoolState pool = restored.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool quarantined = durableLiving > 0
			&& aggregateFailure.Contains("durable survivors")
			&& order && pool
			&& order.m_iOperationContractVersion
				== HST_EnemyCounterattackSaveValidationService.QUARANTINED_CONTRACT_VERSION
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& order.m_sRuntimeStatus == "exact_counterattack_quarantined"
			&& order.m_sFailureReason.Contains("durable survivors")
			&& !order.m_bResourceSettlementApplied
			&& pool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& pool.m_iAttackResources == attackBeforeRestore
			&& CountMutationId(restored, fixture.m_Order.m_sResourceRefundMutationId) == 0;
		Print(string.Format(
			"Partisan forged destroyed-living proof | durable %1 | rejected/quarantined %2/%3 | reason '%4'",
			durableLiving,
			!aggregateFailure.IsEmpty(),
			quarantined,
			aggregateFailure));
		return quarantined;
	}

	protected bool ProvePreparedForeignExecutionQuarantine()
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("forged_foreign_execution");
		string reason = "focused proof forged foreign execution backlink";
		if (!StagePreparedSettlement(
			fixture,
			"route_failed_survivors",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
			reason,
			true))
			return false;
		fixture.m_Batch.m_sForceId
			= "force_foreign_execution_" + fixture.m_Order.m_sOrderId;
		bool claimantLinksRetained = fixture.m_Batch.m_sOperationId
			== fixture.m_Operation.m_sOperationId
			&& fixture.m_Batch.m_sManifestId == fixture.m_Manifest.m_sManifestId;
		string aggregateFailure
			= HST_EnemyCounterattackSaveValidationService.ValidatePendingResourceRefundAggregateAuthority(
				fixture.m_State.m_aEnemyStrategicMutations,
				fixture.m_Order,
				fixture.m_Operation,
				fixture.m_Manifest,
				fixture.m_Batch,
				fixture.m_Group);
		HST_FactionPoolState sourcePool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		if (!sourcePool)
			return false;
		int attackBeforeRestore = sourcePool.m_iAttackResources;
		string foreignForceId = fixture.m_Batch.m_sForceId;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		if (!restored)
			return false;
		HST_EnemyOrderState order = restored.FindEnemyOrder(fixture.m_Order.m_sOrderId);
		HST_ForceSpawnResultState batch = restored.FindForceSpawnResult(
			fixture.m_Order.m_sSpawnResultId);
		HST_FactionPoolState pool = restored.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool quarantined = claimantLinksRetained && !aggregateFailure.IsEmpty()
			&& order && batch && pool
			&& order.m_iOperationContractVersion
				== HST_EnemyCounterattackSaveValidationService.QUARANTINED_CONTRACT_VERSION
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& order.m_sRuntimeStatus == "exact_counterattack_quarantined"
			&& !order.m_bResourceSettlementApplied
			&& batch.m_sOperationId == fixture.m_Operation.m_sOperationId
			&& batch.m_sManifestId == fixture.m_Manifest.m_sManifestId
			&& batch.m_sForceId == foreignForceId
			&& pool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& pool.m_iAttackResources == attackBeforeRestore
			&& CountMutationId(restored, fixture.m_Order.m_sResourceRefundMutationId) == 0;
		Print(string.Format(
			"Partisan forged foreign-execution proof | claimant/rejected/quarantined %1/%2/%3 | force '%4' | reason '%5'",
			claimantLinksRetained,
			!aggregateFailure.IsEmpty(),
			quarantined,
			foreignForceId,
			aggregateFailure));
		return quarantined;
	}

	protected bool StagePreparedSettlement(
		HST_EnemyCounterattackOperationProofFixture fixture,
		string settlementKind,
		HST_EOperationTerminalResult terminalResult,
		string reason,
		bool prepareOperation)
	{
		return StagePreparedSettlementWithSurvivors(
			fixture,
			settlementKind,
			terminalResult,
			reason,
			prepareOperation,
			-1);
	}

	protected bool StagePreparedSettlementWithSurvivors(
		HST_EnemyCounterattackOperationProofFixture fixture,
		string settlementKind,
		HST_EOperationTerminalResult terminalResult,
		string reason,
		bool prepareOperation,
		int stagedSurvivors)
	{
		if (!m_Fixtures.Ready(fixture) || settlementKind.IsEmpty())
			return false;
		fixture.m_State.m_iElapsedSeconds = Math.Max(
			1,
			fixture.m_State.m_iElapsedSeconds + 1);
		int accepted = fixture.m_Order.m_iCompositionManpower;
		if (fixture.m_Manifest)
			accepted = fixture.m_Manifest.m_iAcceptedMemberCount;
		if (accepted <= 0)
			return false;
		int survivors = accepted;
		if (stagedSurvivors >= 0)
			survivors = Math.Max(0, Math.Min(accepted, stagedSurvivors));
		string settlementId = HST_OperationService.BuildSettlementId(
			fixture.m_Order.m_sOperationId,
			settlementKind);
		if (prepareOperation)
		{
			HST_OperationService operations = new HST_OperationService();
			HST_OperationTransitionResult prepared
				= operations.PrepareExactEnemyCounterattackSettlement(
					fixture.m_State,
					fixture.m_Order,
					terminalResult,
					settlementId,
					reason);
			if (!prepared || !prepared.m_bAccepted || !prepared.m_Operation
				|| prepared.m_Operation.m_eSettlementState
					!= HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
				return false;
		}

		int attackRefund = Math.Max(0, fixture.m_Order.m_iAttackCost)
			* survivors / accepted;
		int supportRefund = Math.Max(0, fixture.m_Order.m_iSupportCost)
			* survivors / accepted;
		if (HST_EnemyCounterattackSaveValidationService.IsFullRefundSettlementKind(
			settlementKind))
		{
			attackRefund = Math.Max(0, fixture.m_Order.m_iAttackCost);
			supportRefund = Math.Max(0, fixture.m_Order.m_iSupportCost);
		}
		fixture.m_Order.m_sResourceSettlementId = settlementId;
		fixture.m_Order.m_sResourceSettlementKind = settlementKind;
		fixture.m_Order.m_iSettlementAcceptedMemberCount = accepted;
		fixture.m_Order.m_iSettlementSurvivorMemberCount = survivors;
		fixture.m_Order.m_iRefundedAttackResources = attackRefund;
		fixture.m_Order.m_iRefundedSupportResources = supportRefund;
		fixture.m_Order.m_sResourceRefundMutationId
			= "enemy_resource_refund_" + settlementId;
		fixture.m_Order.m_bResourceSettlementApplied = false;
		return HST_EnemyCounterattackSaveValidationService
			.ValidatePreparedResourceSettlementTuple(fixture.m_Order).IsEmpty();
	}

	protected bool ApplyPreparedRefund(
		HST_EnemyCounterattackOperationProofFixture fixture,
		string reason)
	{
		if (!fixture || !fixture.m_State || !fixture.m_EnemyDirector
			|| !fixture.m_Order)
			return false;
		HST_EnemyOrderState order = fixture.m_Order;
		if (order.m_iAttackCost > 0 && order.m_iSupportCost == 0)
		{
			return fixture.m_EnemyDirector.RefundProactiveAttackResources(
				fixture.m_State,
				order.m_sFactionKey,
				order.m_iRefundedAttackResources,
				reason,
				order.m_sResourceRefundMutationId,
				order.m_sResourceSettlementId,
				order.m_sOrderId,
				order.m_sOperationId,
				order.m_sManifestId,
				order.m_sTargetZoneId);
		}
		if (order.m_iSupportCost <= 0 || order.m_iAttackCost != 0)
			return false;
		return fixture.m_EnemyDirector.RefundDefenseResources(
			fixture.m_State,
			order.m_sFactionKey,
			order.m_sTargetZoneId,
			order.m_iRefundedAttackResources,
			order.m_iRefundedSupportResources,
			reason,
			order.m_sResourceRefundMutationId,
			order.m_sResourceSettlementId,
			order.m_sOrderId,
			order.m_sOperationId,
			order.m_sManifestId);
	}

	protected bool ProveOpenResourceQuarantineCase(string suffix, int mutationMode)
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("resource_" + suffix);
		if (!m_Fixtures.Ready(fixture))
			return false;
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		int expectedAttack = pool.m_iAttackResources;
		int expectedSupport = pool.m_iSupportResources;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_EnemyStrategicMutationState debit = FindMutation(
			saveData.m_aEnemyStrategicMutations,
			fixture.m_Order.m_sResourceDebitMutationId);
		if (!debit)
			return false;
		if (mutationMode == 0)
			saveData.m_aEnemyStrategicMutations.Remove(
				saveData.m_aEnemyStrategicMutations.Find(debit));
		else if (mutationMode == 1)
			saveData.m_aEnemyStrategicMutations.Insert(debit);
		else
		{
			debit.m_sOperationId = debit.m_sOperationId + "_mismatch";
			debit.m_sFingerprint
				= HST_EnemyStrategicResourceService.BuildMutationFingerprint(debit);
		}
		HST_CampaignState restored = saveData.Restore();
		return IsOpenResourceQuarantineExact(
			restored,
			fixture.m_Order.m_sOrderId,
			expectedAttack,
			expectedSupport);
	}

	protected bool ProveSettledResourceQuarantineCase(string suffix, int mutationMode)
	{
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("resource_" + suffix);
		if (!m_Fixtures.Ready(fixture) || !DriveUntilSettled(fixture, 80))
			return false;
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		int expectedAttack = pool.m_iAttackResources;
		int expectedSupport = pool.m_iSupportResources;
		string refundMutationId = fixture.m_Order.m_sResourceRefundMutationId;
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_EnemyOrderState savedOrder = FindOrder(
			saveData.m_aEnemyOrders,
			fixture.m_Order.m_sOrderId);
		if (!savedOrder)
			return false;
		string validFailure
			= HST_EnemyCounterattackSaveValidationService.ValidateSettledResourceRefundAuthority(
				saveData.m_aEnemyStrategicMutations,
				savedOrder);
		HST_EnemyStrategicMutationState refund = FindMutation(
			saveData.m_aEnemyStrategicMutations,
			refundMutationId);
		if (!refund)
			return false;
		if (mutationMode == 0)
			saveData.m_aEnemyStrategicMutations.Remove(
				saveData.m_aEnemyStrategicMutations.Find(refund));
		else if (mutationMode == 1)
			saveData.m_aEnemyStrategicMutations.Insert(refund);
		else
		{
			refund.m_sOperationId = refund.m_sOperationId + "_mismatch";
			refund.m_sFingerprint
				= HST_EnemyStrategicResourceService.BuildMutationFingerprint(refund);
		}
		string corruptFailure
			= HST_EnemyCounterattackSaveValidationService.ValidateSettledResourceRefundAuthority(
				saveData.m_aEnemyStrategicMutations,
				savedOrder);
		int maximumMutationRows = saveData.m_aEnemyStrategicMutations.Count();
		HST_CampaignState restored = saveData.Restore();
		if (!restored)
			return false;
		HST_EnemyOrderState order = restored.FindEnemyOrder(fixture.m_Order.m_sOrderId);
		HST_FactionPoolState restoredPool = restored.FindFactionPool(
			HST_EnemyCounterattackOperationProofFixtureFactory.PROOF_FACTION_KEY);
		bool statusExact = order && order.m_iOperationContractVersion
			== HST_EnemyCounterattackSaveValidationService.QUARANTINED_CONTRACT_VERSION
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& !order.m_sFailureReason.IsEmpty();
		bool resourcesExact = restoredPool
			&& restoredPool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceSaveValidationService.QUARANTINE_CONTRACT_VERSION
			&& !restoredPool.m_sStrategicAuthorityFailure.IsEmpty()
			&& restoredPool.m_iAttackResources == expectedAttack
			&& restoredPool.m_iSupportResources == expectedSupport
			&& restored.m_aEnemyStrategicMutations.Count() <= maximumMutationRows;
		bool rowsExact = CountOperationId(restored, fixture.m_Operation.m_sOperationId) == 1
			&& CountManifestId(restored, fixture.m_Manifest.m_sManifestId) == 1;
		int restoredContract;
		int restoredStatus;
		string restoredFailure;
		int restoredPoolContract;
		if (order)
		{
			restoredContract = order.m_iOperationContractVersion;
			restoredStatus = order.m_eStatus;
			restoredFailure = order.m_sFailureReason;
		}
		if (restoredPool)
			restoredPoolContract = restoredPool.m_iStrategicContractVersion;
		Print(string.Format(
			"Partisan counterattack settled-refund quarantine proof | case %1 | contract/status %2/%3 | reason '%4' | status/resources/rows %5/%6/%7 | pool contract %8",
			suffix,
			restoredContract,
			restoredStatus,
			restoredFailure,
			statusExact,
			resourcesExact,
			rowsExact,
			restoredPoolContract));
		return validFailure.IsEmpty() && corruptFailure.Contains("refund")
			&& statusExact && resourcesExact && rowsExact;
	}

	protected bool IsOpenResourceQuarantineExact(
		HST_CampaignState restored,
		string orderId,
		int expectedAttack,
		int expectedSupport)
	{
		if (!restored)
			return false;
		HST_EnemyOrderState order = restored.FindEnemyOrder(orderId);
		if (!order)
			return false;
		HST_OperationRecordState operation = restored.FindOperation(order.m_sOperationId);
		HST_ForceManifestState manifest = restored.FindForceManifest(order.m_sManifestId);
		HST_ForceSpawnResultState batch = restored.FindForceSpawnResult(order.m_sSpawnResultId);
		HST_ActiveGroupState group = restored.FindActiveGroup(order.m_sGroupId);
		HST_FactionPoolState pool = restored.FindFactionPool(order.m_sFactionKey);
		bool statusExact = order.m_iOperationContractVersion
			== HST_EnemyCounterattackSaveValidationService.QUARANTINED_CONTRACT_VERSION
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& order.m_sFailureReason.Contains("original resource debit");
		bool holdExact = batch && batch.m_bStrategicProjectionHeld
			&& !batch.m_bCancelRequested && group
			&& group.m_sRuntimeStatus == "exact_counterattack_quarantined";
		bool outcomeExact = !order.m_bOutcomeApplied
			&& !order.m_bResourceSettlementApplied
			&& order.m_sResourceRefundMutationId.IsEmpty();
		bool resourcesExact = pool && pool.m_iAttackResources == expectedAttack
			&& pool.m_iSupportResources == expectedSupport;
		return statusExact && holdExact && outcomeExact && resourcesExact
			&& operation && manifest;
	}

	protected HST_EnemyStrategicMutationState FindMutation(
		array<ref HST_EnemyStrategicMutationState> mutations,
		string mutationId)
	{
		if (!mutations || mutationId.IsEmpty())
			return null;
		foreach (HST_EnemyStrategicMutationState mutation : mutations)
		{
			if (mutation && mutation.m_sMutationId == mutationId)
				return mutation;
		}
		return null;
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
		int bounded = Math.Max(0, Math.Min(group.m_iOriginalInfantryCount, living));
		group.m_iInfantryCount = bounded;
		group.m_iDurableLivingInfantryCount = bounded;
		group.m_iLastSeenAliveCount = bounded;
		group.m_iSurvivorInfantryCount = bounded;
	}

	protected bool DriveUntilSettled(
		HST_EnemyCounterattackOperationProofFixture fixture,
		int maxTicks)
	{
		if (!m_Fixtures.Ready(fixture))
			return false;
		for (int tick = 0; tick < maxTicks; tick++)
		{
			if (fixture.m_Operation.m_eSettlementState
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
		return fixture.m_Operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
	}

	protected HST_EnemyOrderState FindOrder(
		array<ref HST_EnemyOrderState> orders,
		string orderId)
	{
		if (!orders || orderId.IsEmpty())
			return null;
		foreach (HST_EnemyOrderState order : orders)
		{
			if (order && order.m_sOrderId == orderId)
				return order;
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

	protected HST_ZoneState FindZone(
		array<ref HST_ZoneState> zones,
		string zoneId)
	{
		if (!zones || zoneId.IsEmpty())
			return null;
		foreach (HST_ZoneState zone : zones)
		{
			if (zone && zone.m_sZoneId == zoneId)
				return zone;
		}
		return null;
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

	protected int RemoveOwnershipTransitionsForSource(
		array<ref HST_OwnershipTransitionState> transitions,
		string sourceId)
	{
		int removed;
		if (!transitions || sourceId.IsEmpty())
			return removed;
		for (int index = transitions.Count() - 1; index >= 0; index--)
		{
			HST_OwnershipTransitionState transition = transitions[index];
			if (!transition || transition.m_sSourceId != sourceId)
				continue;
			transitions.Remove(index);
			removed++;
		}
		return removed;
	}

	protected bool IsConfirmedRetiredMemberSlot(
		HST_ForceSpawnResultState batch,
		string slotId)
	{
		if (!batch || slotId.IsEmpty())
			return false;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotId == slotId
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
			{
				return slot.m_eStatus
					== HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
					&& slot.m_bCasualtyConfirmed;
			}
		}
		return false;
	}

	protected bool HasConfirmedCasualty(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return false;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				&& slot.m_bCasualtyConfirmed
				&& slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED)
				return true;
		}
		return false;
	}

	protected int CountEnemyOrderId(HST_CampaignState state, string orderId)
	{
		int count;
		if (!state || orderId.IsEmpty())
			return count;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (order && order.m_sOrderId == orderId)
				count++;
		}
		return count;
	}

	protected int CountOperationId(HST_CampaignState state, string operationId)
	{
		int count;
		if (!state || operationId.IsEmpty())
			return count;
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
		if (!state || manifestId.IsEmpty())
			return count;
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
		{
			if (manifest && manifest.m_sManifestId == manifestId)
				count++;
		}
		return count;
	}

	protected int CountBatchId(HST_CampaignState state, string batchId)
	{
		int count;
		if (!state || batchId.IsEmpty())
			return count;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && batch.m_sResultId == batchId)
				count++;
		}
		return count;
	}

	protected int CountGroupId(HST_CampaignState state, string groupId)
	{
		int count;
		if (!state || groupId.IsEmpty())
			return count;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && group.m_sGroupId == groupId)
				count++;
		}
		return count;
	}

	protected int CountMutationId(HST_CampaignState state, string mutationId)
	{
		int count;
		if (!state || mutationId.IsEmpty())
			return count;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_sMutationId == mutationId)
				count++;
		}
		return count;
	}

	protected int CountOwnershipTransitionId(HST_CampaignState state, string requestId)
	{
		int count;
		if (!state || requestId.IsEmpty())
			return count;
		foreach (HST_OwnershipTransitionState transition : state.m_aOwnershipTransitions)
		{
			if (transition && transition.m_sRequestId == requestId)
				count++;
		}
		return count;
	}

	protected int CountOwnershipTransitionForSource(
		HST_CampaignState state,
		string sourceId)
	{
		int count;
		if (!state || sourceId.IsEmpty())
			return count;
		foreach (HST_OwnershipTransitionState transition : state.m_aOwnershipTransitions)
		{
			if (transition && transition.m_sSourceId == sourceId)
				count++;
		}
		return count;
	}

	protected int CountExternalZoneId(HST_CampaignState state, string zoneId)
	{
		int count;
		if (!state || zoneId.IsEmpty())
			return count;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_sZoneId == zoneId)
				count++;
		}
		return count;
	}

	protected int CountExternalOwnershipTransitionClaimants(
		HST_CampaignState state,
		string operationId)
	{
		int count;
		if (!state || operationId.IsEmpty())
			return count;
		string requestId = "ownership_counterattack_" + operationId;
		foreach (HST_OwnershipTransitionState transition : state.m_aOwnershipTransitions)
		{
			if (transition && (transition.m_sRequestId == requestId
				|| transition.m_sSourceId == operationId))
				count++;
		}
		return count;
	}

	protected string BuildExternalEndpointOwnershipSemanticRow(HST_ZoneState zone)
	{
		if (!zone)
			return "null";
		return string.Format(
			"%1|%2|%3|%4|%5|%6|%7",
			zone.m_sZoneId,
			zone.m_sOwnerFactionKey,
			zone.m_iOwnershipContractVersion,
			zone.m_iOwnershipRevision,
			zone.m_sActiveOwnershipTransitionRequestId,
			zone.m_sLastOwnershipTransitionRequestId,
			BuildExternalCanonicalOptionalText(zone.m_sOwnershipAuthorityFailure));
	}

	protected bool ValidateExternalEndpointOwnershipAuthority(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string evidence)
	{
		evidence = "external counterattack endpoint ownership authority rejected";
		if (!state || !carrier)
			return false;
		string sourceZoneId;
		string targetZoneId;
		string sourceOwner;
		string targetOwner;
		string operationId;
		int sourceRevision;
		int targetRevision;
		if (carrier.m_Expectation && !carrier.m_SettlementExpectation)
		{
			sourceZoneId = carrier.m_Expectation.m_sSourceZoneId;
			targetZoneId = carrier.m_Expectation.m_sTargetZoneId;
			sourceOwner
				= carrier.m_Expectation.m_sExpectedSourceOwnerFactionKey;
			targetOwner
				= carrier.m_Expectation.m_sExpectedTargetOwnerFactionKey;
			sourceRevision
				= carrier.m_Expectation.m_iExpectedSourceOwnershipRevision;
			targetRevision
				= carrier.m_Expectation.m_iExpectedTargetOwnershipRevision;
			operationId = carrier.m_Expectation.m_sOperationId;
		}
		else if (!carrier.m_Expectation && carrier.m_SettlementExpectation)
		{
			sourceZoneId = carrier.m_SettlementExpectation.m_sSourceZoneId;
			targetZoneId = carrier.m_SettlementExpectation.m_sTargetZoneId;
			sourceOwner
				= carrier.m_SettlementExpectation.m_sExpectedSourceOwnerFactionKey;
			targetOwner
				= carrier.m_SettlementExpectation.m_sExpectedTargetOwnerFactionKey;
			sourceRevision
				= carrier.m_SettlementExpectation.m_iExpectedSourceOwnershipRevision;
			targetRevision
				= carrier.m_SettlementExpectation.m_iExpectedTargetOwnershipRevision;
			operationId = carrier.m_SettlementExpectation.m_sOperationId;
		}
		else
			return false;
		if (sourceZoneId.IsEmpty() || targetZoneId.IsEmpty()
			|| sourceZoneId == targetZoneId || sourceOwner.IsEmpty()
			|| targetOwner.IsEmpty() || operationId.IsEmpty()
			|| sourceRevision <= 0 || targetRevision <= 0)
			return false;
		HST_ZoneState source = state.FindZone(sourceZoneId);
		HST_ZoneState target = state.FindZone(targetZoneId);
		bool exact = source && target
			&& CountExternalZoneId(state, sourceZoneId) == 1
			&& CountExternalZoneId(state, targetZoneId) == 1;
		if (exact)
		{
			exact = source.m_sOwnerFactionKey == sourceOwner
				&& source.m_iOwnershipRevision == sourceRevision
				&& target.m_sOwnerFactionKey == targetOwner
				&& target.m_iOwnershipRevision == targetRevision;
		}
		if (!exact)
		{
			evidence = string.Format(
				"endpoint ownership mismatch | source rows/revision %1/%2 | target rows/revision %3/%4",
				CountExternalZoneId(state, sourceZoneId),
				sourceRevision,
				CountExternalZoneId(state, targetZoneId),
				targetRevision);
			return false;
		}
		evidence = string.Format(
			"endpoint ownership exact | source/target revisions %1/%2 | operation %3",
			sourceRevision,
			targetRevision,
			operationId);
		return true;
	}

	protected bool ValidateExternalEndpointAuthorityTamperRejection(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string evidence)
	{
		evidence = "external counterattack endpoint/claimant tamper rejection failed";
		string endpointEvidence;
		if (!ValidateExternalEndpointOwnershipAuthority(
			state,
			carrier,
			endpointEvidence))
			return false;
		string validationFingerprint;
		string validationEvidence;
		bool settlementCut = !carrier.m_Expectation
			&& carrier.m_SettlementExpectation;
		bool baselineExact;
		if (settlementCut)
		{
			baselineExact = ValidateExternalPreparedSettlementState(
				state,
				carrier,
				validationFingerprint,
				validationEvidence);
		}
		else
		{
			baselineExact = ValidateExternalPreparedCutState(
				state,
				carrier,
				validationFingerprint,
				validationEvidence);
		}
		if (!baselineExact)
			return false;
		string sourceZoneId;
		string operationId;
		if (carrier.m_Expectation && !carrier.m_SettlementExpectation)
		{
			sourceZoneId = carrier.m_Expectation.m_sSourceZoneId;
			operationId = carrier.m_Expectation.m_sOperationId;
		}
		else if (!carrier.m_Expectation && carrier.m_SettlementExpectation)
		{
			sourceZoneId = carrier.m_SettlementExpectation.m_sSourceZoneId;
			operationId = carrier.m_SettlementExpectation.m_sOperationId;
		}
		else
			return false;
		HST_ZoneState source = state.FindZone(sourceZoneId);
		if (!source || CountExternalOwnershipTransitionClaimants(
			state,
			operationId) != 0)
			return false;
		int sourceRevision = source.m_iOwnershipRevision;
		source.m_iOwnershipRevision = sourceRevision + 1;
		bool revisionRejected = !ValidateExternalEndpointOwnershipAuthority(
			state,
			carrier,
			endpointEvidence);
		if (settlementCut)
		{
			revisionRejected = revisionRejected
				&& !ValidateExternalPreparedSettlementState(
					state,
					carrier,
					validationFingerprint,
					validationEvidence);
		}
		else
		{
			revisionRejected = revisionRejected
				&& !ValidateExternalPreparedCutState(
					state,
					carrier,
					validationFingerprint,
					validationEvidence);
		}
		source.m_iOwnershipRevision = sourceRevision;

		int insertionIndex = state.m_aOwnershipTransitions.Count();
		HST_OwnershipTransitionState requestClaimant
			= new HST_OwnershipTransitionState();
		requestClaimant.m_sRequestId = "ownership_counterattack_" + operationId;
		requestClaimant.m_sSourceId = "foreign_source";
		state.m_aOwnershipTransitions.Insert(requestClaimant);
		bool requestClaimantRejected
			= CountExternalOwnershipTransitionClaimants(state, operationId) == 1;
		if (settlementCut)
		{
			requestClaimantRejected = requestClaimantRejected
				&& !ValidateExternalPreparedSettlementState(
					state,
					carrier,
					validationFingerprint,
					validationEvidence);
		}
		else
		{
			requestClaimantRejected = requestClaimantRejected
				&& !ValidateExternalPreparedCutState(
					state,
					carrier,
					validationFingerprint,
					validationEvidence);
		}
		state.m_aOwnershipTransitions.Remove(insertionIndex);

		insertionIndex = state.m_aOwnershipTransitions.Count();
		HST_OwnershipTransitionState sourceClaimant
			= new HST_OwnershipTransitionState();
		sourceClaimant.m_sRequestId = "foreign_request";
		sourceClaimant.m_sSourceId = operationId;
		state.m_aOwnershipTransitions.Insert(sourceClaimant);
		bool sourceClaimantRejected
			= CountExternalOwnershipTransitionClaimants(state, operationId) == 1;
		if (settlementCut)
		{
			sourceClaimantRejected = sourceClaimantRejected
				&& !ValidateExternalPreparedSettlementState(
					state,
					carrier,
					validationFingerprint,
					validationEvidence);
		}
		else
		{
			sourceClaimantRejected = sourceClaimantRejected
				&& !ValidateExternalPreparedCutState(
					state,
					carrier,
					validationFingerprint,
					validationEvidence);
		}
		state.m_aOwnershipTransitions.Remove(insertionIndex);

		bool restoredExact = ValidateExternalEndpointOwnershipAuthority(
			state,
			carrier,
			endpointEvidence)
			&& CountExternalOwnershipTransitionClaimants(state, operationId) == 0;
		if (settlementCut)
		{
			restoredExact = restoredExact
				&& ValidateExternalPreparedSettlementState(
					state,
					carrier,
					validationFingerprint,
					validationEvidence);
		}
		else
		{
			restoredExact = restoredExact
				&& ValidateExternalPreparedCutState(
					state,
					carrier,
					validationFingerprint,
					validationEvidence);
		}
		bool exact = revisionRejected && requestClaimantRejected
			&& sourceClaimantRejected && restoredExact;
		if (exact)
			evidence = "endpoint revision and both operation claimant identities rejected";
		return exact;
	}

	protected bool PopulateExternalMovementEndpointOwnershipExpectation(
		HST_CampaignState state,
		HST_EnemyCounterattackOutboundVirtualExpectation expectation)
	{
		if (!state || !expectation
			|| CountExternalZoneId(state, expectation.m_sSourceZoneId) != 1
			|| CountExternalZoneId(state, expectation.m_sTargetZoneId) != 1)
			return false;
		HST_ZoneState source = state.FindZone(expectation.m_sSourceZoneId);
		HST_ZoneState target = state.FindZone(expectation.m_sTargetZoneId);
		if (!source || !target || source.m_sOwnerFactionKey.IsEmpty()
			|| target.m_sOwnerFactionKey.IsEmpty()
			|| source.m_iOwnershipRevision <= 0
			|| target.m_iOwnershipRevision <= 0)
			return false;
		expectation.m_sExpectedSourceOwnerFactionKey
			= source.m_sOwnerFactionKey;
		expectation.m_iExpectedSourceOwnershipRevision
			= source.m_iOwnershipRevision;
		expectation.m_sExpectedTargetOwnerFactionKey
			= target.m_sOwnerFactionKey;
		expectation.m_iExpectedTargetOwnershipRevision
			= target.m_iOwnershipRevision;
		return true;
	}

	protected bool PopulateExternalSettlementEndpointOwnershipExpectation(
		HST_CampaignState state,
		HST_EnemyCounterattackPreparedSettlementExpectation expectation)
	{
		if (!state || !expectation
			|| CountExternalZoneId(state, expectation.m_sSourceZoneId) != 1
			|| CountExternalZoneId(state, expectation.m_sTargetZoneId) != 1)
			return false;
		HST_ZoneState source = state.FindZone(expectation.m_sSourceZoneId);
		HST_ZoneState target = state.FindZone(expectation.m_sTargetZoneId);
		if (!source || !target || source.m_sOwnerFactionKey.IsEmpty()
			|| target.m_sOwnerFactionKey.IsEmpty()
			|| source.m_iOwnershipRevision <= 0
			|| target.m_iOwnershipRevision <= 0)
			return false;
		expectation.m_sExpectedSourceOwnerFactionKey
			= source.m_sOwnerFactionKey;
		expectation.m_iExpectedSourceOwnershipRevision
			= source.m_iOwnershipRevision;
		expectation.m_sExpectedTargetOwnerFactionKey
			= target.m_sOwnerFactionKey;
		expectation.m_iExpectedTargetOwnershipRevision
			= target.m_iOwnershipRevision;
		return true;
	}

	bool PrepareExternalOwnerAppliedPendingRestartCarrier(
		string sessionNonce,
		string runId,
		string world,
		string cutName,
		out HST_CampaignState stagedState,
		out HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string evidence)
	{
		stagedState = null;
		carrier = null;
		evidence = "external exact counterattack owner-applied pending cut rejected";
		if (!HST_EnemyCounterattackExternalRestartProofService.ValidateNonce(
				sessionNonce)
			|| !HST_EnemyCounterattackExternalRestartProofService.ValidateRunId(runId)
			|| world.IsEmpty()
			|| cutName != HST_EnemyCounterattackExternalRestartProofService
				.CUT_OWNER_APPLIED_PENDING)
			return false;

		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("external_owner_applied_pending");
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = m_Fixtures.Failure(fixture);
			return false;
		}
		HST_OwnershipTransitionState pending
			= DriveUntilOwnerAppliedPending(fixture, 80);
		if (!pending || fixture.m_Ownership.OwnerAppliedPendingCount() != 1)
		{
			evidence = "production counterattack did not reach one owner-applied incomplete receipt";
			return false;
		}

		int living = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		HST_EnemyCounterattackOutboundVirtualExpectation expectation
			= BuildExternalNoCasualtyExpectation(fixture, living);
		if (!expectation)
		{
			evidence = "owner-applied pending aggregate expectation was unavailable";
			return false;
		}

		carrier = new HST_EnemyCounterattackExternalRestartCarrier();
		carrier.m_sMagic
			= HST_EnemyCounterattackExternalRestartProofService.CARRIER_MAGIC;
		carrier.m_sSessionNonce = sessionNonce;
		carrier.m_sRunId = runId;
		carrier.m_sBuildSha = HST_BuildInfo.BUILD_SHA;
		carrier.m_sBuildUtc = HST_BuildInfo.BUILD_UTC;
		carrier.m_sBuildLabel = HST_BuildInfo.BUILD_LABEL;
		carrier.m_iCampaignSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		carrier.m_sWorld = world;
		carrier.m_sCutName = cutName;
		carrier.m_iCut
			= HST_EnemyCounterattackExternalRestartProofService.ResolveCut(cutName);
		carrier.m_Expectation = expectation;
		carrier.m_iPreparedElapsedSecond = fixture.m_State.m_iElapsedSeconds;
		carrier.m_fPreparedRouteProgressMeters
			= fixture.m_Operation.m_fRouteProgressMeters;
		carrier.m_fPreparedRouteTotalDistanceMeters
			= fixture.m_Operation.m_fRouteTotalDistanceMeters;
		carrier.m_vPreparedStrategicPosition
			= fixture.m_Operation.m_vStrategicPosition;
		carrier.m_iExpectedPhysicalAdapterHandleCount = 0;
		carrier.m_iExpectedPhysicalRuntimeMemberCount = 0;
		carrier.m_sRawPreparedCutSemanticFingerprint
			= BuildExternalOwnerAppliedPendingFingerprint(fixture.m_State, carrier);
		HST_CampaignSaveData normalizedSave = new HST_CampaignSaveData();
		normalizedSave.Capture(fixture.m_State);
		HST_CampaignState normalizedState = normalizedSave.Restore();
		if (!normalizedState)
		{
			carrier = null;
			evidence = "owner-applied pending state did not survive one canonical restore";
			return false;
		}
		carrier.m_sPreparedSemanticFingerprint
			= BuildExternalOwnerAppliedPendingFingerprint(normalizedState, carrier);

		string rawFingerprint;
		string rawEvidence;
		string normalizedFingerprint;
		string normalizedEvidence;
		string rawCasualtyEvidence;
		string normalizedCasualtyEvidence;
		string rawRuntimeEvidence;
		string normalizedRuntimeEvidence;
		bool exact = ValidateExternalOwnerAppliedRawPendingState(
			fixture.m_State,
			carrier,
			rawFingerprint,
			rawEvidence)
			&& rawFingerprint == carrier.m_sRawPreparedCutSemanticFingerprint
			&& ValidateExternalOwnerAppliedPendingState(
				normalizedState,
				carrier,
				normalizedFingerprint,
				normalizedEvidence)
			&& normalizedFingerprint == carrier.m_sPreparedSemanticFingerprint
			&& rawFingerprint != normalizedFingerprint
			&& ValidateExternalCasualtyContinuity(
				fixture.m_State,
				carrier,
				rawCasualtyEvidence)
			&& ValidateExternalCasualtyContinuity(
				normalizedState,
				carrier,
				normalizedCasualtyEvidence)
			&& ValidateExternalRuntimeClaimantsZero(
				fixture.m_State,
				carrier,
				fixture.m_Adapter,
				fixture.m_PhysicalWar,
				rawRuntimeEvidence)
			&& ValidateExternalRuntimeClaimantsZero(
				normalizedState,
				carrier,
				fixture.m_Adapter,
				fixture.m_PhysicalWar,
				normalizedRuntimeEvidence);
		if (!exact)
		{
			carrier = null;
			evidence = "raw " + rawEvidence + " | normalized "
				+ normalizedEvidence + " | raw casualty " + rawCasualtyEvidence
				+ " | normalized casualty " + normalizedCasualtyEvidence;
			evidence += " | raw runtime " + rawRuntimeEvidence
				+ " | normalized runtime " + normalizedRuntimeEvidence;
			return false;
		}
		stagedState = fixture.m_State;
		evidence = string.Format(
			"external counterattack raw/normalized owner-applied pending receipt exact | route %1/%2 | owner revision %3",
			Math.Round(fixture.m_Operation.m_fRouteProgressMeters),
			Math.Round(fixture.m_Operation.m_fRouteTotalDistanceMeters),
			expectation.m_iExpectedTargetOwnershipRevision);
		return true;
	}

	bool ValidateExternalOwnerAppliedPendingState(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		return ValidateExternalOwnerAppliedLifecycleState(
			state,
			carrier,
			HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION,
			false,
			false,
			true,
			fingerprint,
			evidence);
	}

	bool ValidateExternalOwnerAppliedRawPendingState(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		return ValidateExternalOwnerAppliedLifecycleState(
			state,
			carrier,
			HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION,
			false,
			false,
			false,
			fingerprint,
			evidence);
	}

	bool ValidateExternalOwnerAppliedCompletedAwaitingOutcomeState(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		return ValidateExternalOwnerAppliedLifecycleState(
			state,
			carrier,
			HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION,
			true,
			false,
			true,
			fingerprint,
			evidence);
	}

	bool ValidateExternalOwnerAppliedReturningState(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		return ValidateExternalOwnerAppliedLifecycleState(
			state,
			carrier,
			HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN,
			true,
			true,
			true,
			fingerprint,
			evidence);
	}

	bool ValidateExternalOwnerAppliedRawReturningState(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		return ValidateExternalOwnerAppliedLifecycleState(
			state,
			carrier,
			HST_EOperationDutyState.HST_OPERATION_DUTY_RETURNING_TO_ORIGIN,
			true,
			true,
			false,
			fingerprint,
			evidence);
	}

	bool AdvanceExternalOwnerAppliedPendingRecovery(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyDirectorService enemyDirector,
		HST_EnemyCounterattackOperationService counterattacks,
		HST_OwnershipTransitionService ownership,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "external-counterattack-owner-applied-recovery-unavailable";
		evidence = "external counterattack owner-applied recovery rejected";
		if (!state || !preset || !enemyDirector || !counterattacks || !ownership
			|| !carrier || !carrier.m_Expectation)
			return false;

		string completedFingerprint;
		string completedEvidence;
		if (!ValidateExternalOwnerAppliedCompletedAwaitingOutcomeState(
			state,
			carrier,
			completedFingerprint,
			completedEvidence))
		{
			evidence = completedEvidence;
			return false;
		}
		HST_EnemyCounterattackOutboundVirtualExpectation expected
			= carrier.m_Expectation;
		HST_OwnershipTransitionState receipt = state.FindOwnershipTransition(
			"ownership_counterattack_" + expected.m_sOperationId);
		HST_ZoneState target = state.FindZone(expected.m_sTargetZoneId);
		HST_StrategicEventState strategicEvent;
		if (receipt)
			strategicEvent = state.FindStrategicEvent(receipt.m_sStrategicEventId);
		if (!receipt || !target || !strategicEvent)
			return false;
		string receiptBefore = BuildExternalOwnershipTransitionSemanticRow(receipt);
		string ownerEffectBefore = BuildExternalStrategicEventSemanticRow(strategicEvent);
		string ownerBefore = target.m_sOwnerFactionKey;
		int ownerRevisionBefore = target.m_iOwnershipRevision;

		bool secondOwnershipChanged = ownership.ReconcileAfterRestore(state);
		string secondFingerprint;
		string secondEvidence;
		bool secondExact = !secondOwnershipChanged
			&& ValidateExternalOwnerAppliedCompletedAwaitingOutcomeState(
				state,
				carrier,
				secondFingerprint,
				secondEvidence)
			&& secondFingerprint == completedFingerprint
			&& receiptBefore
				== BuildExternalOwnershipTransitionSemanticRow(receipt)
			&& ownerEffectBefore
				== BuildExternalStrategicEventSemanticRow(strategicEvent)
			&& target.m_sOwnerFactionKey == ownerBefore
			&& target.m_iOwnershipRevision == ownerRevisionBefore;
		if (!secondExact)
		{
			evidence = "second ownership reconcile changed completed authority | "
				+ secondEvidence;
			return false;
		}

		HST_EnemyOrderState order = state.FindEnemyOrder(expected.m_sOrderId);
		if (!order)
			return false;
		bool tickChanged = counterattacks.TickOrder(
			state,
			preset,
			enemyDirector,
			order);
		string returningEvidence;
		bool returningExact = tickChanged
			&& ValidateExternalOwnerAppliedRawReturningState(
				state,
				carrier,
				fingerprint,
				returningEvidence)
			&& receiptBefore
				== BuildExternalOwnershipTransitionSemanticRow(receipt)
			&& ownerEffectBefore
				== BuildExternalStrategicEventSemanticRow(strategicEvent)
			&& target.m_sOwnerFactionKey == ownerBefore
			&& target.m_iOwnershipRevision == ownerRevisionBefore;
		evidence = string.Format(
			"second ownership changed/exact %1/%2 | tick/return %3/%4",
			secondOwnershipChanged,
			secondExact,
			tickChanged,
			returningExact);
		evidence += " | return " + returningEvidence
			+ " | second ownership " + secondEvidence
			+ " | completed " + completedEvidence;
		return returningExact;
	}

	bool ValidateExternalOwnerAppliedPendingReplay(
		HST_CampaignState state,
		HST_OwnershipTransitionService ownership,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "external-counterattack-owner-applied-replay-unavailable";
		evidence = "external counterattack owner-applied replay rejected";
		if (!state || !ownership || !carrier || !carrier.m_Expectation)
			return false;

		string beforeFingerprint;
		string beforeEvidence;
		if (!ValidateExternalOwnerAppliedReturningState(
			state,
			carrier,
			beforeFingerprint,
			beforeEvidence))
		{
			evidence = beforeEvidence;
			return false;
		}
		HST_EnemyCounterattackOutboundVirtualExpectation expected
			= carrier.m_Expectation;
		HST_OwnershipTransitionState receipt = state.FindOwnershipTransition(
			"ownership_counterattack_" + expected.m_sOperationId);
		HST_ZoneState target = state.FindZone(expected.m_sTargetZoneId);
		HST_StrategicEventState strategicEvent;
		if (receipt)
			strategicEvent = state.FindStrategicEvent(receipt.m_sStrategicEventId);
		if (!receipt || !target || !strategicEvent)
			return false;
		string receiptBefore = BuildExternalOwnershipTransitionSemanticRow(receipt);
		string ownerEffectBefore = BuildExternalStrategicEventSemanticRow(strategicEvent);
		string ownerBefore = target.m_sOwnerFactionKey;
		int ownerRevisionBefore = target.m_iOwnershipRevision;

		bool ownershipChanged = ownership.ReconcileAfterRestore(state);
		string afterEvidence;
		bool exact = !ownershipChanged
			&& ValidateExternalOwnerAppliedReturningState(
				state,
				carrier,
				fingerprint,
				afterEvidence)
			&& fingerprint == beforeFingerprint
			&& receiptBefore
				== BuildExternalOwnershipTransitionSemanticRow(receipt)
			&& ownerEffectBefore
				== BuildExternalStrategicEventSemanticRow(strategicEvent)
			&& target.m_sOwnerFactionKey == ownerBefore
			&& target.m_iOwnershipRevision == ownerRevisionBefore;
		evidence = "returning " + beforeEvidence
			+ " | ownership replay no-op " + afterEvidence;
		return exact;
	}

	protected bool ValidateExternalOwnerAppliedLifecycleState(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		HST_EOperationDutyState expectedDuty,
		bool receiptCompleted,
		bool outcomeApplied,
		bool restoredNormalized,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "external-counterattack-owner-applied-unavailable";
		evidence = "external counterattack owner-applied lifecycle rejected";
		string carrierEvidence;
		if (!state || !carrier
			|| !HST_EnemyCounterattackExternalRestartProofService.ValidateCarrier(
				carrier,
				carrier.m_sSessionNonce,
				carrier.m_sRunId,
				carrier.m_sCutName,
				carrier.m_sWorld,
				carrierEvidence)
			|| !HST_EnemyCounterattackExternalRestartProofService
				.IsOwnerAppliedPendingCut(carrier.m_sCutName)
			|| state.m_iSchemaVersion != carrier.m_iCampaignSchemaVersion)
			return false;

		HST_EnemyCounterattackOutboundVirtualExpectation expected
			= carrier.m_Expectation;
		HST_EnemyOrderState order = state.FindEnemyOrder(expected.m_sOrderId);
		HST_OperationRecordState operation
			= state.FindOperation(expected.m_sOperationId);
		HST_ForceManifestState manifest
			= state.FindForceManifest(expected.m_sManifestId);
		HST_ForceSpawnResultState batch
			= state.FindForceSpawnResult(expected.m_sBatchId);
		HST_ActiveGroupState group = state.FindActiveGroup(expected.m_sGroupId);
		HST_FactionPoolState pool = state.FindFactionPool(expected.m_sFactionKey);
		if (!order || !operation || !manifest || !batch || !group || !pool)
			return false;

		bool rowsExact = CountEnemyOrderId(state, expected.m_sOrderId) == 1
			&& CountOperationId(state, expected.m_sOperationId) == 1
			&& CountManifestId(state, expected.m_sManifestId) == 1
			&& CountBatchId(state, expected.m_sBatchId) == 1
			&& CountGroupId(state, expected.m_sGroupId) == 1
			&& CountMutationId(state, expected.m_sDebitMutationId) == 1
			&& CountExternalOwnershipTransitionClaimants(
				state,
				expected.m_sOperationId) == 1;
		string endpointEvidence;
		bool endpointExact = ValidateExternalEndpointOwnershipAuthority(
			state,
			carrier,
			endpointEvidence);

		bool orderIdentityExact = order.m_sOperationId == expected.m_sOperationId
			&& order.m_eType
				== HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK
			&& order.m_iOperationContractVersion
				== HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION
			&& order.m_sManifestId == expected.m_sManifestId
			&& order.m_sManifestHash == expected.m_sManifestHash
			&& order.m_sSpawnResultId == expected.m_sBatchId
			&& order.m_sGroupId == expected.m_sGroupId
			&& order.m_sFactionKey == expected.m_sFactionKey
			&& order.m_sSourceZoneId == expected.m_sSourceZoneId
			&& order.m_sTargetZoneId == expected.m_sTargetZoneId;
		bool orderResourceExact
			= order.m_sResourceDebitMutationId == expected.m_sDebitMutationId
			&& order.m_iAttackCost == expected.m_iAttackCost
			&& order.m_iSupportCost == expected.m_iSupportCost
			&& !order.m_bResourceRefundApplied
			&& !order.m_bResourceSettlementApplied
			&& order.m_sResourceRefundMutationId.IsEmpty()
			&& order.m_sResourceSettlementId.IsEmpty()
			&& order.m_sResourceSettlementKind.IsEmpty();
		bool orderLifecycleExact
			= order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& order.m_bStrategicServiceCommitted
			&& !order.m_bPhysicalized
			&& order.m_bOutcomeApplied == outcomeApplied;
		bool orderExact = orderIdentityExact && orderResourceExact
			&& orderLifecycleExact;
		if (outcomeApplied)
		{
			orderExact = orderExact
				&& order.m_sResolutionKind == "exact_counterattack_recaptured"
				&& order.m_sFailureReason.IsEmpty();
		}
		else
		{
			orderExact = orderExact && order.m_sResolutionKind.IsEmpty()
				&& !order.m_sFailureReason.IsEmpty();
		}

		bool operationIdentityExact
			= operation.m_sEnemyOrderId == expected.m_sOrderId
			&& operation.m_eType
				== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK
			&& operation.m_iContractVersion
				== HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION
			&& operation.m_sManifestId == expected.m_sManifestId
			&& operation.m_sSpawnResultId == expected.m_sBatchId
			&& operation.m_sGroupId == expected.m_sGroupId
			&& operation.m_sProjectionId == expected.m_sProjectionId
			&& operation.m_sForceId == expected.m_sForceId
			&& operation.m_sOwnerFactionKey == expected.m_sFactionKey
			&& operation.m_sOriginZoneId == expected.m_sSourceZoneId
			&& operation.m_sAssignmentZoneId == expected.m_sTargetZoneId;
		bool operationLifecycleExact = operation.m_eDutyState == expectedDuty
			&& operation.m_eResumeDutyState == expectedDuty
			&& operation.m_eEngagementMode
				== HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
			&& operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			&& operation.m_sSettlementId.IsEmpty()
			&& operation.m_sTerminalReason.IsEmpty();
		bool operationExact = operationIdentityExact && operationLifecycleExact;
		if (expectedDuty
			== HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
		{
			operationExact = operationExact
				&& operation.m_sTacticalTargetZoneId
					== expected.m_sTargetZoneId
				&& operation.m_sCurrentRouteId
					== expected.m_sOperationId + "_outbound"
				&& Math.AbsFloat(operation.m_fRouteProgressMeters
					- operation.m_fRouteTotalDistanceMeters) <= 0.1
				&& VectorExact(
					operation.m_vStrategicPosition,
					operation.m_vAssignmentPosition,
					0.1);
		}
		else
		{
			operationExact = operationExact
				&& operation.m_sTacticalTargetZoneId
					== expected.m_sSourceZoneId
				&& operation.m_sCurrentRouteId
					== expected.m_sOperationId + "_return"
				&& VectorExact(
					operation.m_vRouteEndPosition,
					operation.m_vOriginPosition,
					0.1);
		}

		bool batchExact = batch.m_sManifestId == expected.m_sManifestId
			&& batch.m_sOperationId == expected.m_sOperationId
			&& batch.m_sResultId == expected.m_sBatchId
			&& batch.m_sProjectionId == expected.m_sProjectionId
			&& batch.m_sForceId == expected.m_sForceId
			&& batch.m_eStatus
				== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			&& batch.m_bStrategicProjectionHeld
			&& !batch.m_bExternalAssetAuthority
			&& batch.m_sNativeGroupId.IsEmpty();
		bool groupIdentityExact = group.m_sOperationId == expected.m_sOperationId
			&& group.m_sManifestId == expected.m_sManifestId
			&& group.m_sSpawnResultId == expected.m_sBatchId
			&& group.m_sGroupId == expected.m_sGroupId
			&& group.m_sProjectionId == expected.m_sProjectionId
			&& group.m_sForceId == expected.m_sForceId
			&& group.m_sEnemyOrderId == expected.m_sOrderId;
		bool groupRuntimeExact = !group.m_bSpawnedEntity
			&& group.m_sRuntimeEntityId.IsEmpty()
			&& group.m_iSpawnedAgentCount == 0;
		bool groupRosterExact
			= group.m_iInfantryCount == expected.m_iLivingMemberCount
			&& group.m_iDurableLivingInfantryCount
				== expected.m_iLivingMemberCount;
		bool aggregateExact = ValidateExternalVirtualManifest(manifest, expected)
			&& batchExact && groupIdentityExact && groupRuntimeExact
			&& groupRosterExact;
		if (restoredNormalized)
		{
			aggregateExact = aggregateExact
				&& group.m_sRuntimeStatus == "enemy_counterattack_virtual";
		}
		else if (outcomeApplied)
		{
			aggregateExact = aggregateExact
				&& group.m_sRuntimeStatus
					== "enemy_counterattack_virtual_returning";
		}
		else
		{
			aggregateExact = aggregateExact
				&& group.m_sRuntimeStatus
					== "enemy_counterattack_virtual_ownership_retry";
		}
		bool normalizationExact;
		if (restoredNormalized)
		{
			bool normalizedOrderExact = !order.m_bAbstractResolved
				&& order.m_sRuntimeStatus
					== "exact_counterattack_restore_virtual";
			bool normalizedOperationExact
				= operation.m_iMaterializationStateEnteredAtSecond
					== state.m_iElapsedSeconds
				&& operation.m_iStrategicLastUpdateSecond
					== state.m_iElapsedSeconds
				&& operation.m_iVirtualCombatLastStepSecond
					== state.m_iElapsedSeconds
				&& operation.m_iLastProgressAtSecond
					== state.m_iElapsedSeconds
				&& operation.m_iArrivalConfirmationCount == 0
				&& operation.m_iLastArrivalConfirmationSecond == 0
				&& operation.m_sLastProjectionReason
					== "restored exact counterattack as strategic authority";
			bool normalizedBatchExact
				= batch.m_iStrategicHoldSinceSecond == state.m_iElapsedSeconds
				&& batch.m_iUpdatedAtSecond == state.m_iElapsedSeconds
				&& batch.m_iLastLifecycleSecond == state.m_iElapsedSeconds
				&& batch.m_iAttemptGeneration > 0
				&& batch.m_iLifecycleRevision > 0;
			bool normalizedGroupExact = VectorExact(
					group.m_vPosition,
					operation.m_vStrategicPosition,
					0.1)
				&& VectorExact(
					group.m_vSourcePosition,
					operation.m_vStrategicPosition,
					0.1);
			normalizationExact = normalizedOrderExact
				&& normalizedOperationExact && normalizedBatchExact
				&& normalizedGroupExact;
		}
		else if (outcomeApplied)
		{
			normalizationExact = !order.m_bAbstractResolved
				&& order.m_sRuntimeStatus == "exact_virtual_returning";
		}
		else
		{
			normalizationExact = order.m_bAbstractResolved
				&& order.m_sRuntimeStatus == "exact_virtual_ownership_retry";
		}

		bool poolExact = pool.m_iAttackResources
				== expected.m_iExpectedAttackPool
			&& pool.m_iSupportResources == expected.m_iExpectedSupportPool
			&& pool.m_iStrategicOperationalMutationCount
				== expected.m_iExpectedPoolOperationalMutationCount
			&& pool.m_sLastStrategicMutationId == expected.m_sDebitMutationId;
		bool resourceExact = poolExact
			&& ValidateExternalDebit(state, order, pool, expected);
		resourceExact = resourceExact
			&& CountExternalSupportClaimants(state, expected) == 0;
		resourceExact = resourceExact
			&& CountExternalLegacyQRFClaimants(state, expected) == 0;

		string receiptEvidence;
		bool receiptExact = IsExternalCanonicalCounterattackOwnershipReceiptExact(
			state,
			carrier,
			receiptCompleted,
			receiptEvidence);
		string casualtyEvidence;
		bool casualtyExact = ValidateExternalCasualtyContinuity(
			state,
			carrier,
			casualtyEvidence);
		fingerprint = BuildExternalOwnerAppliedPendingFingerprint(state, carrier);
		bool fingerprintExact = fingerprint
			!= "external-counterattack-owner-applied-unavailable";
		if (!receiptCompleted && !outcomeApplied)
		{
			if (restoredNormalized)
			{
				fingerprintExact = fingerprintExact
					&& fingerprint == carrier.m_sPreparedSemanticFingerprint;
			}
			else
			{
				fingerprintExact = fingerprintExact
					&& fingerprint == carrier.m_sRawPreparedCutSemanticFingerprint;
			}
		}
		bool exact = rowsExact && endpointExact && orderExact && operationExact
			&& aggregateExact && resourceExact && receiptExact && casualtyExact
			&& normalizationExact && fingerprintExact;
		evidence = string.Format(
			"owner-applied lifecycle rows/endpoint/order/operation/aggregate %1/%2/%3/%4/%5",
			rowsExact,
			endpointExact,
			orderExact,
			operationExact,
			aggregateExact);
		evidence += string.Format(
			" | resources/receipt/casualty/normalization/fingerprint %1/%2/%3/%4/%5 | %6",
			resourceExact,
			receiptExact,
			casualtyExact,
			normalizationExact,
			fingerprintExact,
			receiptEvidence);
		return exact;
	}

	protected bool IsExternalCanonicalCounterattackOwnershipReceiptExact(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		bool completed,
		out string evidence)
	{
		evidence = "canonical counterattack ownership receipt rejected";
		if (!state || !carrier || !carrier.m_Expectation)
			return false;
		HST_EnemyCounterattackOutboundVirtualExpectation expected
			= carrier.m_Expectation;
		string requestId = "ownership_counterattack_" + expected.m_sOperationId;
		HST_OwnershipTransitionState transition
			= state.FindOwnershipTransition(requestId);
		HST_ZoneState zone = state.FindZone(expected.m_sTargetZoneId);
		if (!transition || !zone
			|| CountOwnershipTransitionId(state, requestId) != 1)
			return false;

		bool identityExact = transition.m_iContractVersion
				== HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			&& transition.m_sRequestId == requestId
			&& transition.m_sZoneId == expected.m_sTargetZoneId
			&& transition.m_sCause == "military_capture"
			&& transition.m_sSourceType == "enemy_counterattack"
			&& transition.m_sSourceId == expected.m_sOperationId
			&& transition.m_sActorIdentityId.IsEmpty()
			&& transition.m_sReason
				== "exact enemy counterattack recaptured the location";
		bool ownerIdentityExact = transition.m_sExpectedOwnerFactionKey
				== transition.m_sPreviousOwnerFactionKey
			&& !transition.m_sPreviousOwnerFactionKey.IsEmpty()
			&& transition.m_sPreviousOwnerFactionKey
				!= transition.m_sNewOwnerFactionKey
			&& transition.m_sNewOwnerFactionKey == expected.m_sFactionKey
			&& transition.m_iExpectedRevision + 1
				== transition.m_iAppliedRevision
			&& transition.m_iAppliedRevision
				== expected.m_iExpectedTargetOwnershipRevision;
		identityExact = identityExact && ownerIdentityExact;
		bool requestExact = transition.m_iSupportReward == 0
			&& !transition.m_bApplyEnemyConsequences
			&& transition.m_bReconcileSecurity
			&& !transition.m_bCreateSecurity
			&& transition.m_bNotify
			&& transition.m_sProjectionParentRequestId.IsEmpty()
			&& !transition.m_bQuarantined
			&& transition.m_sFailureReason.IsEmpty();
		bool checklistOwnerExact = transition.m_bValidated
			&& transition.m_bOwnerApplied
			&& transition.m_bTownPolicyApplied
			&& transition.m_bOldSecurityRetired
			&& transition.m_bHostileRuntimeRetired
			&& transition.m_bNewSecurityApplied;
		bool checklistResourceExact = transition.m_bSupportApplied
			&& transition.m_bFacilitiesApplied
			&& transition.m_bLogisticsApplied
			&& transition.m_bEconomyApplied
			&& transition.m_bEnemyConsequencesApplied;
		bool checklistPublicationExact = transition.m_bStrategicEventCompleted
			&& transition.m_bEventAppended
			&& transition.m_bNotificationApplied
			&& transition.m_bProjectionRequested
			&& !transition.m_bDeferredPublicationReleased
			&& !transition.m_bSetupProjectionWithoutMarkers
			&& transition.m_bPersistenceRequested;
		bool checklistExact = checklistOwnerExact && checklistResourceExact
			&& checklistPublicationExact;
		bool clockExact = transition.m_iCreatedAtSecond > 0
			&& transition.m_iLastAttemptAtSecond >= transition.m_iCreatedAtSecond
			&& transition.m_iLastAttemptAtSecond <= state.m_iElapsedSeconds
			&& transition.m_iAttemptCount > 0;
		bool completionExact;
		bool backlinkExact;
		if (completed)
		{
			completionExact = transition.m_sStatus == "completed"
				&& transition.m_bCompleted
				&& transition.m_iCompletedAtSecond
					== transition.m_iLastAttemptAtSecond
				&& transition.m_iCompletedAtSecond > 0;
			backlinkExact = zone.m_sActiveOwnershipTransitionRequestId.IsEmpty()
				&& zone.m_sLastOwnershipTransitionRequestId == requestId;
		}
		else
		{
			completionExact = transition.m_sStatus == "projecting"
				&& !transition.m_bCompleted
				&& transition.m_iCompletedAtSecond == 0;
			backlinkExact
				= zone.m_sActiveOwnershipTransitionRequestId == requestId
				&& zone.m_sLastOwnershipTransitionRequestId.IsEmpty();
		}
		bool zoneExact = zone.m_sOwnerFactionKey == expected.m_sFactionKey
			&& zone.m_iOwnershipContractVersion
				== HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			&& zone.m_iOwnershipRevision
				== expected.m_iExpectedTargetOwnershipRevision
			&& zone.m_sOwnershipAuthorityFailure.IsEmpty()
			&& backlinkExact;

		HST_StrategicEventState strategicEvent
			= state.FindStrategicEvent(transition.m_sStrategicEventId);
		bool eventIdentityExact = strategicEvent
			&& strategicEvent.m_sKind == "zone_captured"
			&& strategicEvent.m_sSourceType == "enemy_counterattack"
			&& strategicEvent.m_sSourceId == expected.m_sOperationId
			&& strategicEvent.m_sTargetZoneId == expected.m_sTargetZoneId;
		bool eventCountExact = strategicEvent
			&& CountExternalStrategicEventId(
				state,
				transition.m_sStrategicEventId) == 1
			&& CountExternalCounterattackOwnershipStrategicEvents(
				state,
				expected.m_sOperationId,
				expected.m_sTargetZoneId,
				expected.m_sFactionKey) == 1;
		bool eventOwnerExact = strategicEvent
			&& strategicEvent.m_sTargetFactionKey
				== transition.m_sPreviousOwnerFactionKey
			&& strategicEvent.m_sOwnerBefore
				== transition.m_sPreviousOwnerFactionKey
			&& strategicEvent.m_sOwnerAfter == expected.m_sFactionKey
			&& strategicEvent.m_bApplied;
		bool eventExact = eventIdentityExact && eventCountExact && eventOwnerExact;
		bool exact = identityExact && requestExact && checklistExact;
		exact = exact && clockExact && completionExact && zoneExact && eventExact;
		evidence = string.Format(
			"ownership receipt identity/request/checklist/clock/completion/zone/event %1/%2/%3/%4/%5/%6/%7",
			identityExact,
			requestExact,
			checklistExact,
			clockExact,
			completionExact,
			zoneExact,
			eventExact);
		return exact;
	}

	protected string BuildExternalOwnerAppliedPendingFingerprint(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier)
	{
		if (!state || !carrier || !carrier.m_Expectation)
			return "external-counterattack-owner-applied-unavailable";
		HST_EnemyCounterattackOutboundVirtualExpectation expected
			= carrier.m_Expectation;
		HST_OwnershipTransitionState transition = state.FindOwnershipTransition(
			"ownership_counterattack_" + expected.m_sOperationId);
		if (!transition)
			return "external-counterattack-owner-applied-unavailable";
		HST_StrategicEventState strategicEvent
			= state.FindStrategicEvent(transition.m_sStrategicEventId);
		if (!strategicEvent)
			return "external-counterattack-owner-applied-unavailable";
		string aggregate = BuildExternalCanonicalFingerprint(state, carrier, true);
		if (aggregate == "external-counterattack-semantic-unavailable")
			return "external-counterattack-owner-applied-unavailable";
		return aggregate + " | ownership transition "
			+ BuildExternalOwnershipTransitionSemanticRow(transition)
			+ " | ownership strategic event "
			+ BuildExternalStrategicEventSemanticRow(strategicEvent)
			+ " | ownership strategic source events "
			+ CountExternalCounterattackOwnershipStrategicEvents(
				state,
				expected.m_sOperationId,
				expected.m_sTargetZoneId,
				expected.m_sFactionKey).ToString();
	}

	protected string BuildExternalOwnershipTransitionSemanticRow(
		HST_OwnershipTransitionState transition)
	{
		if (!transition)
			return "null";
		string row = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			transition.m_iContractVersion,
			transition.m_sStatus,
			transition.m_sRequestId,
			transition.m_sZoneId,
			transition.m_sCause,
			transition.m_sSourceType,
			transition.m_sSourceId,
			BuildExternalCanonicalOptionalText(transition.m_sActorIdentityId),
			transition.m_sReason);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			transition.m_sExpectedOwnerFactionKey,
			transition.m_iExpectedRevision,
			transition.m_sPreviousOwnerFactionKey,
			transition.m_sNewOwnerFactionKey,
			transition.m_iAppliedRevision,
			transition.m_iSupportReward,
			transition.m_bApplyEnemyConsequences,
			transition.m_bReconcileSecurity,
			transition.m_bCreateSecurity);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			transition.m_bNotify,
			transition.m_iCreatedAtSecond,
			transition.m_iLastAttemptAtSecond,
			transition.m_iCompletedAtSecond,
			transition.m_iAttemptCount,
			transition.m_sStrategicEventId,
			BuildExternalCanonicalOptionalText(transition.m_sCampaignEventId),
			BuildExternalCanonicalOptionalText(transition.m_sOldGarrisonId),
			BuildExternalCanonicalOptionalText(transition.m_sNewGarrisonId));
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			BuildExternalCanonicalOptionalText(transition.m_sSecurityDecision),
			BuildExternalCanonicalOptionalText(
				transition.m_sFacilityLogisticsDecision),
			BuildExternalCanonicalOptionalText(
				transition.m_sEnemyConsequenceDecision),
			BuildExternalCanonicalOptionalText(transition.m_sEnemyOrderId),
			BuildExternalCanonicalOptionalText(transition.m_sProjectionDecision),
			BuildExternalCanonicalOptionalText(
				transition.m_sProjectionParentRequestId),
			BuildExternalCanonicalOptionalText(transition.m_sMarkerId),
			transition.m_iMarkerProjectionEpoch,
			transition.m_iMarkerRevision);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			transition.m_iMarkerStreamSequence,
			transition.m_iAggressionApplied,
			transition.m_iCounterattackChance,
			transition.m_iCounterattackRoll,
			transition.m_bCounterattackSelected,
			transition.m_bCounterattackQueued,
			BuildExternalStringArraySemanticRow(transition.m_aSupportZoneIds),
			BuildExternalStringArraySemanticRow(
				transition.m_aAppliedSupportZoneIds),
			transition.m_bValidated);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			transition.m_bOwnerApplied,
			transition.m_bTownPolicyApplied,
			transition.m_bOldSecurityRetired,
			transition.m_bHostileRuntimeRetired,
			transition.m_bNewSecurityApplied,
			transition.m_bSupportApplied,
			transition.m_bFacilitiesApplied,
			transition.m_bLogisticsApplied,
			transition.m_bEconomyApplied);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			transition.m_bEnemyConsequencesApplied,
			transition.m_bStrategicEventCompleted,
			transition.m_bEventAppended,
			transition.m_bNotificationApplied,
			transition.m_bProjectionRequested,
			transition.m_bDeferredPublicationReleased,
			transition.m_bSetupProjectionWithoutMarkers,
			transition.m_bPersistenceRequested,
			transition.m_bCompleted);
		return row + string.Format(
			"|%1|%2",
			transition.m_bQuarantined,
			BuildExternalCanonicalOptionalText(transition.m_sFailureReason));
	}

	protected string BuildExternalStrategicEventSemanticRow(
		HST_StrategicEventState strategicEvent)
	{
		if (!strategicEvent)
			return "null";
		string row = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			strategicEvent.m_sEventId,
			strategicEvent.m_sKind,
			strategicEvent.m_sSourceType,
			strategicEvent.m_sSourceId,
			BuildExternalCanonicalOptionalText(strategicEvent.m_sMissionId),
			BuildExternalCanonicalOptionalText(
				strategicEvent.m_sMissionInstanceId),
			strategicEvent.m_sTargetZoneId,
			strategicEvent.m_sTargetFactionKey,
			strategicEvent.m_sReason);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			BuildExternalCanonicalOptionalText(strategicEvent.m_sSummary),
			strategicEvent.m_iCreatedAtSecond,
			strategicEvent.m_iFactionMoneyDelta,
			strategicEvent.m_iHRDelta,
			strategicEvent.m_iAggressionDelta,
			strategicEvent.m_iAttackResourceDelta,
			strategicEvent.m_iSupportResourceDelta,
			strategicEvent.m_iTownSupportDelta,
			strategicEvent.m_iCaptureProgressDelta);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			strategicEvent.m_iHQKnowledgeDelta,
			strategicEvent.m_sOwnerBefore,
			strategicEvent.m_sOwnerAfter,
			strategicEvent.m_iSupportBefore,
			strategicEvent.m_iSupportAfter,
			strategicEvent.m_iCaptureProgressBefore,
			strategicEvent.m_iCaptureProgressAfter,
			strategicEvent.m_iHQKnowledgeBefore,
			strategicEvent.m_iHQKnowledgeAfter);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			BuildExternalCanonicalOptionalText(strategicEvent.m_sVehicleRuntimeId),
			strategicEvent.m_iVehicleHeatBefore,
			strategicEvent.m_iVehicleHeatAfter,
			strategicEvent.m_iVehicleHeatDelta,
			strategicEvent.m_bVehicleReportedBefore,
			strategicEvent.m_bVehicleReportedAfter,
			strategicEvent.m_iVehicleReportedUntilBefore,
			strategicEvent.m_iVehicleReportedUntilAfter,
			strategicEvent.m_iVehicleReportedUntilDelta);
		return row + string.Format("|%1", strategicEvent.m_bApplied);
	}

	protected string BuildExternalStringArraySemanticRow(array<string> values)
	{
		if (!values)
			return "null";
		string row = values.Count().ToString() + ":";
		bool first = true;
		foreach (string value : values)
		{
			if (!first)
				row += ",";
			row += value;
			first = false;
		}
		return row;
	}

	protected int CountExternalStrategicEventId(
		HST_CampaignState state,
		string eventId)
	{
		int count;
		if (!state || eventId.IsEmpty())
			return count;
		foreach (HST_StrategicEventState strategicEvent : state.m_aStrategicEvents)
		{
			if (strategicEvent && strategicEvent.m_sEventId == eventId)
				count++;
		}
		return count;
	}

	protected int CountExternalCounterattackOwnershipStrategicEvents(
		HST_CampaignState state,
		string operationId,
		string targetZoneId,
		string ownerFactionKey)
	{
		int count;
		if (!state || operationId.IsEmpty() || targetZoneId.IsEmpty()
			|| ownerFactionKey.IsEmpty())
			return count;
		foreach (HST_StrategicEventState strategicEvent : state.m_aStrategicEvents)
		{
			if (strategicEvent
				&& strategicEvent.m_sKind == "zone_captured"
				&& strategicEvent.m_sSourceType == "enemy_counterattack"
				&& strategicEvent.m_sSourceId == operationId
				&& strategicEvent.m_sTargetZoneId == targetZoneId
				&& strategicEvent.m_sOwnerAfter == ownerFactionKey
				&& strategicEvent.m_bApplied)
				count++;
		}
		return count;
	}

	bool PrepareExternalPreparedSettlementRestartCarrier(
		string sessionNonce,
		string runId,
		string world,
		string cutName,
		out HST_CampaignState stagedState,
		out HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string evidence)
	{
		stagedState = null;
		carrier = null;
		evidence = "external exact counterattack prepared settlement rejected";
		if (!HST_EnemyCounterattackExternalRestartProofService.ValidateNonce(
				sessionNonce)
			|| !HST_EnemyCounterattackExternalRestartProofService.ValidateRunId(runId)
			|| world.IsEmpty())
			return false;
		bool beforeRefund = cutName
			== HST_EnemyCounterattackExternalRestartProofService
				.CUT_PREPARED_BEFORE_REFUND;
		bool afterRefund = cutName
			== HST_EnemyCounterattackExternalRestartProofService
				.CUT_PREPARED_AFTER_REFUND;
		bool afterReceipt = cutName
			== HST_EnemyCounterattackExternalRestartProofService
				.CUT_PREPARED_AFTER_RECEIPT;
		if (!beforeRefund && !afterRefund && !afterReceipt)
			return false;

		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture(
				"external_prepared_settlement_" + cutName);
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = m_Fixtures.Failure(fixture);
			return false;
		}
		int accepted = fixture.m_Manifest.m_iAcceptedMemberCount;
		int initialLiving = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		if (accepted <= 1 || initialLiving != accepted)
		{
			evidence = string.Format(
				"nondegenerate admitted counterattack roster unavailable: %1/%2",
				accepted,
				initialLiving);
			return false;
		}

		string casualtySlotId = fixture.m_Queue.SelectStrategicLivingMemberSlotId(
			fixture.m_Batch,
			fixture.m_Operation.m_iDeterministicSeed + 73);
		HST_ForceSpawnQueueCallbackResult casualty
			= fixture.m_Queue.ConfirmStrategicMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				casualtySlotId,
				fixture.m_State.m_iElapsedSeconds + 1,
				"external prepared counterattack settlement confirmed casualty");
		int survivors = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		if (!casualty || !casualty.m_bAccepted || survivors <= 0
			|| survivors >= accepted || survivors != accepted - 1)
		{
			evidence = string.Format(
				"nondegenerate counterattack casualty roster failed: %1/%2",
				accepted,
				survivors);
			return false;
		}

		string settlementKind = "route_failed_survivors";
		string reason
			= "external exact counterattack durable prepared settlement restart proof";
		if (!StagePreparedSettlementWithSurvivors(
			fixture,
			settlementKind,
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED,
			reason,
			true,
			survivors))
		{
			evidence = "counterattack PREPARED settlement staging failed";
			return false;
		}

		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			fixture.m_Order.m_sFactionKey);
		if (!pool)
			return false;
		int attackBeforeRefund = pool.m_iAttackResources;
		int supportBeforeRefund = pool.m_iSupportResources;
		int poolRevisionBeforeRefund = pool.m_iStrategicRevision;
		int operationalMutationsBeforeRefund
			= pool.m_iStrategicOperationalMutationCount;
		if ((afterRefund || afterReceipt)
			&& !ApplyPreparedRefund(fixture, reason))
		{
			evidence = "counterattack deterministic prepared refund staging failed";
			return false;
		}
		if (afterReceipt)
		{
			HST_OperationService operations = new HST_OperationService();
			HST_OperationTransitionResult recorded
				= operations.RecordExactEnemyCounterattackResourceSettlement(
						fixture.m_State,
						fixture.m_Order,
						fixture.m_Order.m_sResourceSettlementKind,
						fixture.m_Order.m_iSettlementAcceptedMemberCount,
						fixture.m_Order.m_iSettlementSurvivorMemberCount);
			if (!recorded || !recorded.m_bAccepted
				|| !fixture.m_Order.m_bResourceSettlementApplied
				|| fixture.m_Operation.m_eSettlementState
					!= HST_EOperationSettlementState
						.HST_OPERATION_SETTLEMENT_PREPARED)
			{
				evidence = "counterattack prepared resource receipt staging failed";
				return false;
			}
		}

		HST_EnemyCounterattackPreparedSettlementExpectation expectation
			= BuildExternalPreparedSettlementExpectation(
				fixture,
				attackBeforeRefund,
				supportBeforeRefund,
				poolRevisionBeforeRefund,
				operationalMutationsBeforeRefund);
		if (!expectation)
			return false;

		carrier = new HST_EnemyCounterattackExternalRestartCarrier();
		carrier.m_sMagic
			= HST_EnemyCounterattackExternalRestartProofService.CARRIER_MAGIC;
		carrier.m_sSessionNonce = sessionNonce;
		carrier.m_sRunId = runId;
		carrier.m_sBuildSha = HST_BuildInfo.BUILD_SHA;
		carrier.m_sBuildUtc = HST_BuildInfo.BUILD_UTC;
		carrier.m_sBuildLabel = HST_BuildInfo.BUILD_LABEL;
		carrier.m_iCampaignSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		carrier.m_sWorld = world;
		carrier.m_sCutName = cutName;
		carrier.m_iCut
			= HST_EnemyCounterattackExternalRestartProofService.ResolveCut(cutName);
		carrier.m_iAccepted = accepted;
		carrier.m_iCasualties = accepted - survivors;
		carrier.m_iSurvivors = survivors;
		carrier.m_iAttackRefund = fixture.m_Order.m_iRefundedAttackResources;
		carrier.m_iSupportRefund = fixture.m_Order.m_iRefundedSupportResources;
		carrier.m_iAttackBeforeRefund = attackBeforeRefund;
		carrier.m_iSupportBeforeRefund = supportBeforeRefund;
		carrier.m_iPreparedAtSecond = fixture.m_Operation.m_iSettledAtSecond;
		carrier.m_iPrefixRevision = fixture.m_Operation.m_iRevision;
		carrier.m_iExpectedPrefixMutationCount = 0;
		carrier.m_iExpectedPrefixAttackDelta = 0;
		carrier.m_iExpectedPrefixSupportDelta = 0;
		carrier.m_bExpectedPrefixReceiptApplied = afterReceipt;
		if (afterRefund || afterReceipt)
		{
			carrier.m_iExpectedPrefixMutationCount = 1;
			carrier.m_iExpectedPrefixAttackDelta = carrier.m_iAttackRefund;
			carrier.m_iExpectedPrefixSupportDelta = carrier.m_iSupportRefund;
		}
		carrier.m_sSettlementKind = settlementKind;
		carrier.m_sSettlementId = fixture.m_Order.m_sResourceSettlementId;
		carrier.m_sRefundMutationId
			= fixture.m_Order.m_sResourceRefundMutationId;
		carrier.m_sReason = reason;
		carrier.m_iExpectedPhysicalAdapterHandleCount = 0;
		carrier.m_iExpectedPhysicalRuntimeMemberCount = 0;
		carrier.m_SettlementExpectation = expectation;
		carrier.m_sPreparedSettlementFingerprint
			= BuildExternalPreparedSettlementFingerprint(
				fixture.m_State,
				carrier);
		carrier.m_sPreparedSemanticFingerprint
			= carrier.m_sPreparedSettlementFingerprint;
		carrier.m_sRawPreparedCutSemanticFingerprint
			= carrier.m_sPreparedSettlementFingerprint;

		string preparedFingerprint;
		string preparedEvidence;
		string runtimeEvidence;
		string endpointTamperEvidence;
		bool exact = ValidateExternalPreparedSettlementState(
			fixture.m_State,
			carrier,
			preparedFingerprint,
			preparedEvidence)
			&& ValidateExternalRuntimeClaimantsZero(
				fixture.m_State,
				carrier,
				fixture.m_Adapter,
				fixture.m_PhysicalWar,
				runtimeEvidence)
			&& ValidateExternalEndpointAuthorityTamperRejection(
				fixture.m_State,
				carrier,
				endpointTamperEvidence);
		if (!exact)
		{
			carrier = null;
			evidence = preparedEvidence + " | " + runtimeEvidence
				+ " | " + endpointTamperEvidence;
			return false;
		}
		stagedState = fixture.m_State;
		evidence = string.Format(
			"external counterattack PREPARED cut %1 | roster %2/%3 | refund %4/%5 | receipt %6",
			cutName,
			survivors,
			accepted,
			carrier.m_iAttackRefund,
			carrier.m_iSupportRefund,
			carrier.m_bExpectedPrefixReceiptApplied);
		return true;
	}

	bool ValidateExternalPreparedSettlementState(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "external-counterattack-prepared-settlement-unavailable";
		evidence = "external counterattack PREPARED settlement state rejected";
		string carrierEvidence;
		if (!state || !carrier || !carrier.m_SettlementExpectation
			|| !HST_EnemyCounterattackExternalRestartProofService.ValidateCarrier(
				carrier,
				carrier.m_sSessionNonce,
				carrier.m_sRunId,
				carrier.m_sCutName,
				carrier.m_sWorld,
				carrierEvidence))
			return false;
		if (state.m_iSchemaVersion != carrier.m_iCampaignSchemaVersion)
			return false;
		fingerprint = BuildExternalPreparedSettlementFingerprint(state, carrier);
		if (!IsExternalPreparedSettlementExact(state, carrier, evidence))
			return false;
		if (fingerprint != carrier.m_sPreparedSettlementFingerprint)
		{
			evidence = "external counterattack PREPARED settlement fingerprint mismatch";
			return false;
		}
		evidence = "external counterattack PREPARED settlement state exact";
		return true;
	}

	bool ValidateExternalTerminalSettlementState(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "external-counterattack-terminal-settlement-unavailable";
		evidence = "external counterattack terminal settlement state rejected";
		string carrierEvidence;
		if (!state || !carrier || !carrier.m_SettlementExpectation
			|| !HST_EnemyCounterattackExternalRestartProofService.ValidateCarrier(
				carrier,
				carrier.m_sSessionNonce,
				carrier.m_sRunId,
				carrier.m_sCutName,
				carrier.m_sWorld,
				carrierEvidence))
			return false;
		if (state.m_iSchemaVersion != carrier.m_iCampaignSchemaVersion)
			return false;
		fingerprint = BuildExternalTerminalSettlementFingerprint(state, carrier);
		if (!IsExternalTerminalSettlementExact(state, carrier, evidence))
			return false;
		evidence = "external counterattack terminal settlement state exact";
		return true;
	}

	bool PrepareExternalRestartCarrier(
		string sessionNonce,
		string runId,
		string world,
		string cutName,
		out HST_CampaignState stagedState,
		out HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string evidence)
	{
		stagedState = null;
		carrier = null;
		evidence = "external exact counterattack preparation rejected";
		if (!HST_EnemyCounterattackExternalRestartProofService.ValidateNonce(
				sessionNonce)
			|| !HST_EnemyCounterattackExternalRestartProofService.ValidateRunId(runId)
			|| world.IsEmpty()
			|| HST_EnemyCounterattackExternalRestartProofService.ResolveCut(cutName) < 0)
			return false;
		if (cutName == HST_EnemyCounterattackExternalRestartProofService
			.CUT_MATERIALIZING_CHECKPOINT_DEFERRED
			|| cutName == HST_EnemyCounterattackExternalRestartProofService
				.CUT_DEMATERIALIZING_BEFORE_HOLD)
		{
			evidence = "checkpoint-deferred cut requires a baseline-aware preparation API";
			return false;
		}

		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("external_restart");
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = m_Fixtures.Failure(fixture);
			return false;
		}

		// Admission used the deterministic fixture seam. The persisted moving cut
		// still reaches inherited production lifecycle logic, with only the
		// player-proximity decision replaced by the guarded virtual proof seam.
		HST_EnemyCounterattackOperationProofHarness production
			= new HST_EnemyCounterattackOperationProofHarness();
		production.UseDeterministicVirtualProjectionForProof();
		production.SetRuntimeServices(
			fixture.m_Queue,
			fixture.m_Adapter,
			fixture.m_PhysicalWar,
			fixture.m_CombatPresence,
			fixture.m_Ownership);
		int elapsedBefore = fixture.m_State.m_iElapsedSeconds;
		int lastUpdateBefore = fixture.m_Operation.m_iStrategicLastUpdateSecond;
		float progressBefore = fixture.m_Operation.m_fRouteProgressMeters;
		float expectedPreparedProgress = progressBefore
			+ HST_StrategicMovementService.EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND
				* HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
		fixture.m_State.m_iElapsedSeconds
			+= HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
		bool advanced = production.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		float preparedFraction = expectedPreparedProgress
			/ fixture.m_Operation.m_fRouteTotalDistanceMeters;
		vector expectedPreparedPosition = fixture.m_Operation.m_vRouteStartPosition
			+ (fixture.m_Operation.m_vRouteEndPosition
				- fixture.m_Operation.m_vRouteStartPosition) * preparedFraction;
		bool preparedClockExact = fixture.m_State.m_iElapsedSeconds == elapsedBefore
				+ HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK
			&& fixture.m_Operation.m_iStrategicLastUpdateSecond == lastUpdateBefore
				+ HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK
			&& Math.AbsFloat(fixture.m_Operation.m_fRouteProgressMeters
				- expectedPreparedProgress) <= 0.01;
		bool preparedPositionExact
			= VectorExact(fixture.m_Operation.m_vStrategicPosition,
				expectedPreparedPosition, 0.01)
			&& VectorExact(fixture.m_Group.m_vPosition,
				expectedPreparedPosition, 0.01)
			&& fixture.m_Operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		bool preparedStepExact = advanced && preparedClockExact
			&& preparedPositionExact;
		if (!preparedStepExact)
		{
			evidence = "production counterattack owner did not advance the exact outbound route tuple";
			return false;
		}

		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			fixture.m_Order.m_sFactionKey);
		int living = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		if (!pool || living <= 0)
			return false;

		HST_EnemyCounterattackOutboundVirtualExpectation expectation
			= new HST_EnemyCounterattackOutboundVirtualExpectation();
		expectation.m_sOrderId = fixture.m_Order.m_sOrderId;
		expectation.m_sOperationId = fixture.m_Operation.m_sOperationId;
		expectation.m_sManifestId = fixture.m_Manifest.m_sManifestId;
		expectation.m_sManifestHash = fixture.m_Manifest.m_sManifestHash;
		expectation.m_sBatchId = fixture.m_Batch.m_sResultId;
		expectation.m_sGroupId = fixture.m_Group.m_sGroupId;
		expectation.m_sProjectionId = fixture.m_Batch.m_sProjectionId;
		expectation.m_sForceId = fixture.m_Batch.m_sForceId;
		expectation.m_sFactionKey = fixture.m_Order.m_sFactionKey;
		expectation.m_sSourceZoneId = fixture.m_Order.m_sSourceZoneId;
		expectation.m_sTargetZoneId = fixture.m_Order.m_sTargetZoneId;
		if (!PopulateExternalMovementEndpointOwnershipExpectation(
			fixture.m_State,
			expectation))
			return false;
		expectation.m_sDebitMutationId
			= fixture.m_Order.m_sResourceDebitMutationId;
		expectation.m_iAttackCost = fixture.m_Order.m_iAttackCost;
		expectation.m_iSupportCost = fixture.m_Order.m_iSupportCost;
		expectation.m_iExpectedAttackPool = pool.m_iAttackResources;
		expectation.m_iExpectedSupportPool = pool.m_iSupportResources;
		expectation.m_iExpectedPoolOperationalMutationCount
			= pool.m_iStrategicOperationalMutationCount;
		expectation.m_iAcceptedMemberCount
			= fixture.m_Manifest.m_iAcceptedMemberCount;
		expectation.m_iLivingMemberCount = living;
		expectation.m_sLivingSlotFingerprint
			= BuildExternalLivingSlotFingerprint(fixture.m_Batch);
		expectation.m_iExpectedNormalizedReprojectionCount = 0;

		carrier = new HST_EnemyCounterattackExternalRestartCarrier();
		carrier.m_sMagic
			= HST_EnemyCounterattackExternalRestartProofService.CARRIER_MAGIC;
		carrier.m_sSessionNonce = sessionNonce;
		carrier.m_sRunId = runId;
		carrier.m_sBuildSha = HST_BuildInfo.BUILD_SHA;
		carrier.m_sBuildUtc = HST_BuildInfo.BUILD_UTC;
		carrier.m_sBuildLabel = HST_BuildInfo.BUILD_LABEL;
		carrier.m_iCampaignSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		carrier.m_sWorld = world;
		carrier.m_sCutName = cutName;
		carrier.m_iCut
			= HST_EnemyCounterattackExternalRestartProofService.ResolveCut(cutName);
		carrier.m_Expectation = expectation;
		carrier.m_iPreparedElapsedSecond = fixture.m_State.m_iElapsedSeconds;
		carrier.m_fPreparedRouteProgressMeters
			= fixture.m_Operation.m_fRouteProgressMeters;
		carrier.m_fPreparedRouteTotalDistanceMeters
			= fixture.m_Operation.m_fRouteTotalDistanceMeters;
		carrier.m_vPreparedStrategicPosition
			= fixture.m_Operation.m_vStrategicPosition;
		carrier.m_sPreparedSemanticFingerprint
			= BuildExternalSemanticFingerprint(fixture.m_State, carrier);
		carrier.m_sRawPreparedCutSemanticFingerprint
			= carrier.m_sPreparedSemanticFingerprint;

		string fingerprint;
		string stateEvidence;
		string runtimeEvidence;
		string endpointTamperEvidence;
		bool exact = ValidateExternalVirtualState(
			fixture.m_State,
			carrier,
			fingerprint,
			stateEvidence)
			&& fingerprint == carrier.m_sPreparedSemanticFingerprint
			&& ValidateExternalRuntimeClaimantsZero(
				fixture.m_State,
				carrier,
				fixture.m_Adapter,
				fixture.m_PhysicalWar,
				runtimeEvidence)
			&& ValidateExternalEndpointAuthorityTamperRejection(
				fixture.m_State,
				carrier,
				endpointTamperEvidence);
		if (!exact)
		{
			carrier = null;
			evidence = stateEvidence + " | " + runtimeEvidence
				+ " | " + endpointTamperEvidence;
			return false;
		}
		stagedState = fixture.m_State;
		evidence = string.Format(
			"external exact counterattack prepared virtual route %1/%2 with %3 living slots",
			Math.Round(carrier.m_fPreparedRouteProgressMeters),
			Math.Round(carrier.m_fPreparedRouteTotalDistanceMeters),
			living);
		return true;
	}

	bool PrepareExternalPhysicalRestartBaseline(
		string sessionNonce,
		string runId,
		string world,
		string cutName,
		out HST_CampaignState stagedState,
		out HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string evidence)
	{
		stagedState = null;
		carrier = null;
		evidence = "external exact counterattack physical baseline rejected";
		if (cutName != HST_EnemyCounterattackExternalRestartProofService
			.CUT_PHYSICAL_LIVE_POSITION)
			return false;

		// Build the same outbound fixture already proven by the virtual cut, then
		// relabel only its portable expectation before the coordinator adopts it.
		// Native materialization is deliberately left to the real-frame owner.
		HST_EnemyCounterattackExternalRestartCarrier outboundCarrier;
		string outboundEvidence;
		if (!PrepareExternalRestartCarrier(
			sessionNonce,
			runId,
			world,
			HST_EnemyCounterattackExternalRestartProofService.CUT_OUTBOUND_VIRTUAL,
			stagedState,
			outboundCarrier,
			outboundEvidence)
			|| !stagedState || !outboundCarrier || !outboundCarrier.m_Expectation)
		{
			evidence = "outbound fixture baseline failed: " + outboundEvidence;
			return false;
		}

		carrier = outboundCarrier;
		carrier.m_sCutName = cutName;
		carrier.m_iCut
			= HST_EnemyCounterattackExternalRestartProofService.ResolveCut(cutName);
		carrier.m_Expectation.m_bExpectedLivingSlotsEverAlive = true;
		carrier.m_Expectation.m_iExpectedNormalizedSlotAttemptCount = 1;
		carrier.m_Expectation.m_iExpectedNormalizedReprojectionCount = 1;
		carrier.m_iExpectedPhysicalAdapterHandleCount
			= carrier.m_Expectation.m_iLivingMemberCount + 1;
		carrier.m_iExpectedPhysicalRuntimeMemberCount
			= carrier.m_Expectation.m_iLivingMemberCount;
		carrier.m_sPreparedSemanticFingerprint = "";
		carrier.m_sRawPreparedCutSemanticFingerprint = "";
		evidence = "outbound fixture baseline built and ready for coordinator adoption | "
			+ outboundEvidence;
		return true;
	}

	bool PrepareExternalMaterializingRestartCarrier(
		string sessionNonce,
		string runId,
		string world,
		string cutName,
		out HST_CampaignState stagedState,
		out HST_CampaignState persistenceSourceState,
		out HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string evidence)
	{
		stagedState = null;
		persistenceSourceState = null;
		carrier = null;
		evidence = "external exact counterattack materializing checkpoint cut rejected";
		if (!HST_EnemyCounterattackExternalRestartProofService.ValidateNonce(
				sessionNonce)
			|| !HST_EnemyCounterattackExternalRestartProofService.ValidateRunId(
				runId)
			|| world.IsEmpty()
			|| cutName != HST_EnemyCounterattackExternalRestartProofService
				.CUT_MATERIALIZING_CHECKPOINT_DEFERRED)
			return false;

		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("external_restart_materializing");
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = m_Fixtures.Failure(fixture);
			return false;
		}
		if (!AdvanceExternalRestartOutboundCursor(fixture, evidence))
			return false;

		int living = fixture.m_Queue.CountStrategicLivingMemberSlots(
			fixture.m_Batch);
		HST_EnemyCounterattackOutboundVirtualExpectation expectation
			= BuildExternalNoCasualtyExpectation(fixture, living);
		if (!expectation)
			return false;

		// The canonical source remains the last safe VIRTUAL checkpoint. Build the
		// raw interruption from a migration-free copy, while a separate one-restore
		// clone defines the fingerprint the next process must observe.
		HST_CampaignSaveData baselineSave = new HST_CampaignSaveData();
		baselineSave.Capture(fixture.m_State);
		HST_CampaignState rawState = new HST_CampaignState();
		baselineSave.ApplyTo(rawState, false);
		HST_CampaignState normalizedBaseline = baselineSave.Restore();
		if (!rawState || !normalizedBaseline)
			return false;

		HST_EnemyOrderState rawOrder
			= rawState.FindEnemyOrder(expectation.m_sOrderId);
		HST_OperationRecordState rawOperation
			= rawState.FindOperation(expectation.m_sOperationId);
		HST_ForceManifestState rawManifest
			= rawState.FindForceManifest(expectation.m_sManifestId);
		HST_ForceSpawnResultState rawBatch
			= rawState.FindForceSpawnResult(expectation.m_sBatchId);
		HST_ActiveGroupState rawGroup
			= rawState.FindActiveGroup(expectation.m_sGroupId);
		HST_OperationRecordState normalizedOperation
			= normalizedBaseline.FindOperation(expectation.m_sOperationId);
		if (!rawOrder || !rawOperation || !rawManifest || !rawBatch || !rawGroup
			|| !normalizedOperation)
			return false;

		carrier = new HST_EnemyCounterattackExternalRestartCarrier();
		carrier.m_sMagic
			= HST_EnemyCounterattackExternalRestartProofService.CARRIER_MAGIC;
		carrier.m_sSessionNonce = sessionNonce;
		carrier.m_sRunId = runId;
		carrier.m_sBuildSha = HST_BuildInfo.BUILD_SHA;
		carrier.m_sBuildUtc = HST_BuildInfo.BUILD_UTC;
		carrier.m_sBuildLabel = HST_BuildInfo.BUILD_LABEL;
		carrier.m_iCampaignSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		carrier.m_sWorld = world;
		carrier.m_sCutName = cutName;
		carrier.m_iCut
			= HST_EnemyCounterattackExternalRestartProofService.ResolveCut(cutName);
		carrier.m_Expectation = expectation;
		carrier.m_iPreparedElapsedSecond = normalizedBaseline.m_iElapsedSeconds;
		carrier.m_fPreparedRouteProgressMeters
			= normalizedOperation.m_fRouteProgressMeters;
		carrier.m_fPreparedRouteTotalDistanceMeters
			= normalizedOperation.m_fRouteTotalDistanceMeters;
		carrier.m_vPreparedStrategicPosition
			= normalizedOperation.m_vStrategicPosition;
		carrier.m_sPreparedSemanticFingerprint
			= BuildExternalSemanticFingerprint(normalizedBaseline, carrier);

		bool materializing = fixture.m_Service.BeginMaterializationForProof(
			rawState,
			fixture.m_EnemyDirector,
			rawOrder,
			rawOperation,
			rawManifest,
			rawBatch,
			rawGroup,
			"external restart proof paused after materialization release");
		if (!materializing)
		{
			carrier = null;
			return false;
		}
		carrier.m_sRawPreparedCutSemanticFingerprint
			= BuildExternalSemanticFingerprint(rawState, carrier);

		string normalizedFingerprint;
		string normalizedEvidence;
		string rawFingerprint;
		string rawEvidence;
		string normalizedRuntimeEvidence;
		string rawRuntimeEvidence;
		string endpointTamperEvidence;
		bool exact = ValidateExternalVirtualState(
			normalizedBaseline,
			carrier,
			normalizedFingerprint,
			normalizedEvidence)
			&& normalizedFingerprint == carrier.m_sPreparedSemanticFingerprint
			&& ValidateExternalMaterializingCutState(
				rawState,
				carrier,
				rawFingerprint,
				rawEvidence)
			&& rawFingerprint == carrier.m_sRawPreparedCutSemanticFingerprint
			&& ValidateExternalRuntimeClaimantsZero(
				normalizedBaseline,
				carrier,
				fixture.m_Adapter,
				fixture.m_PhysicalWar,
				normalizedRuntimeEvidence)
			&& ValidateExternalRuntimeClaimantsZero(
				rawState,
				carrier,
				fixture.m_Adapter,
				fixture.m_PhysicalWar,
				rawRuntimeEvidence)
			&& ValidateExternalEndpointAuthorityTamperRejection(
				rawState,
				carrier,
				endpointTamperEvidence);
		if (!exact)
		{
			carrier = null;
			evidence = "normalized " + normalizedEvidence
				+ " | raw " + rawEvidence
				+ " | normalized runtime " + normalizedRuntimeEvidence
				+ " | raw runtime " + rawRuntimeEvidence
				+ " | endpoint tamper " + endpointTamperEvidence;
			return false;
		}

		stagedState = rawState;
		persistenceSourceState = fixture.m_State;
		evidence = string.Format(
			"external exact counterattack paused MATERIALIZING/STRATEGIC after retained virtual route %1/%2 with %3 living slots",
			Math.Round(carrier.m_fPreparedRouteProgressMeters),
			Math.Round(carrier.m_fPreparedRouteTotalDistanceMeters),
			living);
		return true;
	}

	protected HST_EnemyCounterattackOutboundVirtualExpectation BuildExternalNoCasualtyExpectation(
		HST_EnemyCounterattackOperationProofFixture fixture,
		int living)
	{
		if (!fixture || !fixture.m_State || !fixture.m_Order
			|| !fixture.m_Operation || !fixture.m_Manifest || !fixture.m_Batch
			|| !fixture.m_Group || living <= 0
			|| living != fixture.m_Manifest.m_iAcceptedMemberCount)
			return null;
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			fixture.m_Order.m_sFactionKey);
		if (!pool)
			return null;
		HST_EnemyCounterattackOutboundVirtualExpectation expectation
			= new HST_EnemyCounterattackOutboundVirtualExpectation();
		expectation.m_sOrderId = fixture.m_Order.m_sOrderId;
		expectation.m_sOperationId = fixture.m_Operation.m_sOperationId;
		expectation.m_sManifestId = fixture.m_Manifest.m_sManifestId;
		expectation.m_sManifestHash = fixture.m_Manifest.m_sManifestHash;
		expectation.m_sBatchId = fixture.m_Batch.m_sResultId;
		expectation.m_sGroupId = fixture.m_Group.m_sGroupId;
		expectation.m_sProjectionId = fixture.m_Batch.m_sProjectionId;
		expectation.m_sForceId = fixture.m_Batch.m_sForceId;
		expectation.m_sFactionKey = fixture.m_Order.m_sFactionKey;
		expectation.m_sSourceZoneId = fixture.m_Order.m_sSourceZoneId;
		expectation.m_sTargetZoneId = fixture.m_Order.m_sTargetZoneId;
		if (!PopulateExternalMovementEndpointOwnershipExpectation(
			fixture.m_State,
			expectation))
			return null;
		expectation.m_sDebitMutationId
			= fixture.m_Order.m_sResourceDebitMutationId;
		expectation.m_iAttackCost = fixture.m_Order.m_iAttackCost;
		expectation.m_iSupportCost = fixture.m_Order.m_iSupportCost;
		expectation.m_iExpectedAttackPool = pool.m_iAttackResources;
		expectation.m_iExpectedSupportPool = pool.m_iSupportResources;
		expectation.m_iExpectedPoolOperationalMutationCount
			= pool.m_iStrategicOperationalMutationCount;
		expectation.m_iAcceptedMemberCount
			= fixture.m_Manifest.m_iAcceptedMemberCount;
		expectation.m_iLivingMemberCount = living;
		expectation.m_sLivingSlotFingerprint
			= BuildExternalLivingSlotFingerprint(fixture.m_Batch);
		expectation.m_iExpectedNormalizedReprojectionCount = 0;
		return expectation;
	}

	bool PrepareExternalDematerializingRestartCarrier(
		string sessionNonce,
		string runId,
		string world,
		string cutName,
		out HST_CampaignState stagedState,
		out HST_CampaignState persistenceSourceState,
		out HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string evidence)
	{
		stagedState = null;
		persistenceSourceState = null;
		carrier = null;
		evidence = "external exact counterattack dematerializing cut rejected";
		if (!HST_EnemyCounterattackExternalRestartProofService.ValidateNonce(
				sessionNonce)
			|| !HST_EnemyCounterattackExternalRestartProofService.ValidateRunId(
				runId)
			|| world.IsEmpty()
			|| cutName != HST_EnemyCounterattackExternalRestartProofService
				.CUT_DEMATERIALIZING_BEFORE_HOLD)
			return false;
		HST_EnemyCounterattackOperationProofFixture fixture
			= m_Fixtures.BuildAdmittedFixture("external_restart_dematerializing");
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = m_Fixtures.Failure(fixture);
			return false;
		}

		HST_ForceSpawnSlotResultState casualtySlot;
		int expectedSurvivors;
		if (!StageExternalDematerializingRawCut(
				fixture,
				casualtySlot,
				expectedSurvivors,
				evidence))
			return false;
		evidence = "external exact counterattack dematerializing cut rejected";

		HST_EnemyCounterattackOutboundVirtualExpectation expectation
			= BuildExternalDematerializingExpectation(
				fixture,
				casualtySlot,
				expectedSurvivors);
		if (!expectation)
			return false;

		HST_CampaignState normalizedState;
		carrier = BuildExternalDematerializingCarrier(
			sessionNonce,
			runId,
			world,
			cutName,
			fixture,
			expectation,
			normalizedState,
			evidence);
		if (!carrier)
			return false;
		if (!ValidateExternalDematerializingPreparedStates(
				fixture,
				normalizedState,
				carrier,
				evidence))
		{
			carrier = null;
			return false;
		}
		stagedState = fixture.m_State;
		persistenceSourceState = normalizedState;
		evidence = string.Format(
			"external exact counterattack staged DEMATERIALIZING before hold while retaining the canonical VIRTUAL baseline | accepted/living/casualties %1/%2/1 | restored route %3/%4",
			expectation.m_iAcceptedMemberCount,
			expectation.m_iLivingMemberCount,
			Math.Round(carrier.m_fPreparedRouteProgressMeters),
			Math.Round(carrier.m_fPreparedRouteTotalDistanceMeters));
		return true;
	}

	protected bool StageExternalDematerializingRawCut(
		HST_EnemyCounterattackOperationProofFixture fixture,
		out HST_ForceSpawnSlotResultState casualtySlot,
		out int expectedSurvivors,
		out string evidence)
	{
		casualtySlot = null;
		expectedSurvivors = 0;
		evidence = "production transitions did not establish the exact dematerializing N-1 cut";
		if (!AdvanceExternalRestartOutboundCursor(fixture, evidence))
			return false;

		HST_OperationService operations = new HST_OperationService();
		bool physicalExact = EstablishExternalDematerializingPhysicalCut(
			fixture,
			operations);
		bool foldingExact = FoldExternalDematerializingCut(
			fixture,
			operations,
			casualtySlot,
			expectedSurvivors);
		bool rawLifecycleExact = physicalExact && foldingExact
			&& fixture.m_Operation.m_eMaterializationState
				== HST_EOperationMaterializationState
					.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING
			&& fixture.m_Operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& fixture.m_Batch.m_eStatus
				== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			&& !fixture.m_Batch.m_bStrategicProjectionHeld
			&& fixture.m_Queue.CountConfirmedCasualtyMemberSlots(
				fixture.m_Batch) == 1
			&& fixture.m_Queue.CountDurableLivingMemberSlots(
				fixture.m_Batch) == expectedSurvivors;
		return rawLifecycleExact && expectedSurvivors > 0 && casualtySlot;
	}

	protected bool AdvanceExternalRestartOutboundCursor(
		HST_EnemyCounterattackOperationProofFixture fixture,
		out string evidence)
	{
		evidence = "";
		// Establish a nonzero outbound cursor through the production owner before
		// crossing the same queue and operation transitions used by live runtime.
		HST_EnemyCounterattackOperationProofHarness production
			= new HST_EnemyCounterattackOperationProofHarness();
		production.UseDeterministicVirtualProjectionForProof();
		production.SetRuntimeServices(
			fixture.m_Queue,
			fixture.m_Adapter,
			fixture.m_PhysicalWar,
			fixture.m_CombatPresence,
			fixture.m_Ownership);
		fixture.m_State.m_iElapsedSeconds
			+= HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
		bool advanced = production.TickOrder(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_EnemyDirector,
			fixture.m_Order);
		bool bounded = fixture.m_Operation.m_fRouteProgressMeters > 0
			&& fixture.m_Operation.m_fRouteProgressMeters
				< fixture.m_Operation.m_fRouteTotalDistanceMeters;
		if (advanced && bounded)
			return true;
		evidence = "production counterattack owner did not establish a bounded outbound cursor";
		return false;
	}

	protected bool EstablishExternalDematerializingPhysicalCut(
		HST_EnemyCounterattackOperationProofFixture fixture,
		HST_OperationService operations)
	{
		HST_ForceSpawnQueueCallbackResult released
			= fixture.m_Queue.ReleaseStrategicProjectionForMaterialization(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				fixture.m_State.m_iElapsedSeconds,
				fixture.m_State.m_iElapsedSeconds + 180);
		HST_OperationTransitionResult materializing
			= operations.MarkExactEnemyCounterattackMaterializingFromVirtual(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				fixture.m_Batch,
				"external restart proof entered the physical counterattack bubble");
		PrepareSyntheticSuccessfulProjection(fixture);
		fixture.m_Group.m_bSpawnAttempted = true;
		fixture.m_Group.m_bSpawnedEntity = true;
		fixture.m_Group.m_sRuntimeEntityId = fixture.m_Batch.m_sNativeGroupId;
		fixture.m_Group.m_iSpawnedAgentCount
			= fixture.m_Manifest.m_iAcceptedMemberCount;
		fixture.m_Group.m_sRuntimeStatus = "enemy_counterattack_physical";
		HST_OperationTransitionResult physical
			= operations.MarkExactEnemyCounterattackPhysical(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				fixture.m_Batch,
				"external restart proof established exact live roster authority");
		return released && released.m_bAccepted
			&& materializing && materializing.m_bAccepted
			&& physical && physical.m_bAccepted;
	}

	protected bool FoldExternalDematerializingCut(
		HST_EnemyCounterattackOperationProofFixture fixture,
		HST_OperationService operations,
		out HST_ForceSpawnSlotResultState casualtySlot,
		out int expectedSurvivors)
	{
		casualtySlot = FindFirstRegisteredMemberSlot(fixture.m_Batch);
		HST_ForceSpawnQueueCallbackResult casualty;
		if (casualtySlot)
		{
			casualty = fixture.m_Queue.ConfirmRegisteredMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				casualtySlot.m_sSlotId,
				casualtySlot.m_sEntityId,
				fixture.m_State.m_iElapsedSeconds + 1,
				"external restart proof confirmed one live counterattack casualty");
		}
		expectedSurvivors = fixture.m_Manifest.m_iAcceptedMemberCount - 1;
		ApplyGroupRoster(fixture.m_Group, expectedSurvivors);
		fixture.m_Group.m_iSpawnedAgentCount = expectedSurvivors;
		vector handoffPosition = fixture.m_Order.m_vSourcePosition
			+ (fixture.m_Order.m_vTargetPosition
				- fixture.m_Order.m_vSourcePosition) * 0.35;
		fixture.m_Group.m_vPosition = handoffPosition;
		HST_OperationTransitionResult physicalPosition
			= operations.UpdateExactEnemyCounterattackPhysicalPosition(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group);
		HST_OperationTransitionResult folding
			= operations.BeginExactEnemyCounterattackDematerialization(
				fixture.m_State,
				fixture.m_Order,
				fixture.m_Group,
				"external restart proof stopped before strategic hold");
		return casualty && casualty.m_bAccepted && casualty.m_bStateChanged
			&& physicalPosition && physicalPosition.m_bAccepted
			&& folding && folding.m_bAccepted;
	}

	protected HST_EnemyCounterattackOutboundVirtualExpectation BuildExternalDematerializingExpectation(
		HST_EnemyCounterattackOperationProofFixture fixture,
		HST_ForceSpawnSlotResultState casualtySlot,
		int expectedSurvivors)
	{
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			fixture.m_Order.m_sFactionKey);
		if (!pool)
			return null;
		HST_EnemyCounterattackOutboundVirtualExpectation expectation
			= new HST_EnemyCounterattackOutboundVirtualExpectation();
		expectation.m_sOrderId = fixture.m_Order.m_sOrderId;
		expectation.m_sOperationId = fixture.m_Operation.m_sOperationId;
		expectation.m_sManifestId = fixture.m_Manifest.m_sManifestId;
		expectation.m_sManifestHash = fixture.m_Manifest.m_sManifestHash;
		expectation.m_sBatchId = fixture.m_Batch.m_sResultId;
		expectation.m_sGroupId = fixture.m_Group.m_sGroupId;
		expectation.m_sProjectionId = fixture.m_Batch.m_sProjectionId;
		expectation.m_sForceId = fixture.m_Batch.m_sForceId;
		expectation.m_sFactionKey = fixture.m_Order.m_sFactionKey;
		expectation.m_sSourceZoneId = fixture.m_Order.m_sSourceZoneId;
		expectation.m_sTargetZoneId = fixture.m_Order.m_sTargetZoneId;
		if (!PopulateExternalMovementEndpointOwnershipExpectation(
			fixture.m_State,
			expectation))
			return null;
		expectation.m_sDebitMutationId
			= fixture.m_Order.m_sResourceDebitMutationId;
		expectation.m_iAttackCost = fixture.m_Order.m_iAttackCost;
		expectation.m_iSupportCost = fixture.m_Order.m_iSupportCost;
		expectation.m_iExpectedAttackPool = pool.m_iAttackResources;
		expectation.m_iExpectedSupportPool = pool.m_iSupportResources;
		expectation.m_iExpectedPoolOperationalMutationCount
			= pool.m_iStrategicOperationalMutationCount;
		expectation.m_iAcceptedMemberCount
			= fixture.m_Manifest.m_iAcceptedMemberCount;
		expectation.m_iLivingMemberCount = expectedSurvivors;
		expectation.m_sLivingSlotFingerprint
			= BuildExternalLivingSlotFingerprint(fixture.m_Batch);
		expectation.m_sConfirmedCasualtySlotId = casualtySlot.m_sSlotId;
		expectation.m_sCasualtyTombstoneFingerprint
			= BuildExternalCasualtyTombstoneFingerprint(
				fixture.m_Batch,
				casualtySlot.m_sSlotId);
		expectation.m_iExpectedNormalizedReprojectionCount = 1;
		return expectation;
	}

	protected HST_EnemyCounterattackExternalRestartCarrier BuildExternalDematerializingCarrier(
		string sessionNonce,
		string runId,
		string world,
		string cutName,
		HST_EnemyCounterattackOperationProofFixture fixture,
		HST_EnemyCounterattackOutboundVirtualExpectation expectation,
		out HST_CampaignState normalizedState,
		out string evidence)
	{
		normalizedState = null;
		evidence = "dematerializing cut did not produce a normalized strategic operation";
		HST_EnemyCounterattackExternalRestartCarrier carrier
			= new HST_EnemyCounterattackExternalRestartCarrier();
		carrier.m_sMagic
			= HST_EnemyCounterattackExternalRestartProofService.CARRIER_MAGIC;
		carrier.m_sSessionNonce = sessionNonce;
		carrier.m_sRunId = runId;
		carrier.m_sBuildSha = HST_BuildInfo.BUILD_SHA;
		carrier.m_sBuildUtc = HST_BuildInfo.BUILD_UTC;
		carrier.m_sBuildLabel = HST_BuildInfo.BUILD_LABEL;
		carrier.m_iCampaignSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		carrier.m_sWorld = world;
		carrier.m_sCutName = cutName;
		carrier.m_iCut
			= HST_EnemyCounterattackExternalRestartProofService.ResolveCut(cutName);
		carrier.m_Expectation = expectation;
		carrier.m_iPreparedElapsedSecond = fixture.m_State.m_iElapsedSeconds;

		// Clone before normalization so the persisted staged state remains the raw
		// DEMATERIALIZING/LIVE cut that the next process must consume.
		HST_CampaignSaveData normalizedSave = new HST_CampaignSaveData();
		normalizedSave.Capture(fixture.m_State);
		normalizedState = normalizedSave.Restore();
		HST_OperationRecordState normalizedOperation;
		if (normalizedState)
			normalizedOperation = normalizedState.FindOperation(
				expectation.m_sOperationId);
		if (!normalizedOperation)
			return null;

		carrier.m_fPreparedRouteProgressMeters
			= normalizedOperation.m_fRouteProgressMeters;
		carrier.m_fPreparedRouteTotalDistanceMeters
			= normalizedOperation.m_fRouteTotalDistanceMeters;
		carrier.m_vPreparedStrategicPosition
			= normalizedOperation.m_vStrategicPosition;
		carrier.m_sRawPreparedCutSemanticFingerprint
			= BuildExternalSemanticFingerprint(fixture.m_State, carrier);
		carrier.m_sPreparedSemanticFingerprint
			= BuildExternalSemanticFingerprint(normalizedState, carrier);
		return carrier;
	}

	protected bool ValidateExternalDematerializingPreparedStates(
		HST_EnemyCounterattackOperationProofFixture fixture,
		HST_CampaignState normalizedState,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string evidence)
	{
		string rawFingerprint;
		string rawEvidence;
		string normalizedFingerprint;
		string normalizedEvidence;
		string casualtyEvidence;
		string rawRuntimeEvidence;
		string normalizedRuntimeEvidence;
		string endpointTamperEvidence;
		bool exact = ValidateExternalDematerializingCutState(
			fixture.m_State,
			carrier,
			rawFingerprint,
			rawEvidence);
		if (exact)
			exact = rawFingerprint == carrier.m_sRawPreparedCutSemanticFingerprint;
		if (exact)
		{
			exact = ValidateExternalVirtualState(
				normalizedState,
				carrier,
				normalizedFingerprint,
				normalizedEvidence);
		}
		if (exact)
			exact = normalizedFingerprint == carrier.m_sPreparedSemanticFingerprint;
		if (exact)
		{
			exact = ValidateExternalCasualtyContinuity(
				normalizedState,
				carrier,
				casualtyEvidence);
		}
		if (exact)
		{
			exact = ValidateExternalRuntimeClaimantsZero(
				fixture.m_State,
				carrier,
				fixture.m_Adapter,
				fixture.m_PhysicalWar,
				rawRuntimeEvidence);
		}
		if (exact)
		{
				exact = ValidateExternalRuntimeClaimantsZero(
				normalizedState,
				carrier,
				fixture.m_Adapter,
				fixture.m_PhysicalWar,
				normalizedRuntimeEvidence);
		}
		if (exact)
		{
			exact = ValidateExternalEndpointAuthorityTamperRejection(
				fixture.m_State,
				carrier,
				endpointTamperEvidence);
		}
		if (exact)
		{
			evidence = "";
			return true;
		}
		evidence = rawEvidence + " | normalized " + normalizedEvidence
			+ " | casualty " + casualtyEvidence
			+ " | raw runtime " + rawRuntimeEvidence
			+ " | normalized runtime " + normalizedRuntimeEvidence
			+ " | endpoint tamper " + endpointTamperEvidence;
		return false;
	}

	bool ValidateExternalPreparedCutState(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		if (carrier && carrier.m_sCutName
			== HST_EnemyCounterattackExternalRestartProofService
				.CUT_MATERIALIZING_CHECKPOINT_DEFERRED)
		{
			return ValidateExternalMaterializingCutState(
				state,
				carrier,
				fingerprint,
				evidence);
		}
		if (carrier && carrier.m_sCutName
			== HST_EnemyCounterattackExternalRestartProofService
				.CUT_DEMATERIALIZING_BEFORE_HOLD)
		{
			return ValidateExternalDematerializingCutState(
				state,
				carrier,
				fingerprint,
				evidence);
		}
		return ValidateExternalVirtualState(
			state,
			carrier,
			fingerprint,
			evidence);
	}

	bool ValidateExternalMaterializingCutState(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "external-counterattack-semantic-unavailable";
		evidence = "external exact counterattack materializing cut rejected";
		string carrierEvidence;
		if (!state || !carrier
			|| carrier.m_sCutName
				!= HST_EnemyCounterattackExternalRestartProofService
					.CUT_MATERIALIZING_CHECKPOINT_DEFERRED
			|| !HST_EnemyCounterattackExternalRestartProofService.ValidateCarrier(
				carrier,
				carrier.m_sSessionNonce,
				carrier.m_sRunId,
				carrier.m_sCutName,
				carrier.m_sWorld,
				carrierEvidence)
			|| state.m_iSchemaVersion != carrier.m_iCampaignSchemaVersion)
			return false;

		HST_EnemyCounterattackOutboundVirtualExpectation expected
			= carrier.m_Expectation;
		HST_EnemyOrderState order = state.FindEnemyOrder(expected.m_sOrderId);
		HST_OperationRecordState operation
			= state.FindOperation(expected.m_sOperationId);
		HST_ForceManifestState manifest
			= state.FindForceManifest(expected.m_sManifestId);
		HST_ForceSpawnResultState batch
			= state.FindForceSpawnResult(expected.m_sBatchId);
		HST_ActiveGroupState group = state.FindActiveGroup(expected.m_sGroupId);
		HST_FactionPoolState pool = state.FindFactionPool(expected.m_sFactionKey);
		if (!order || !operation || !manifest || !batch || !group || !pool)
			return false;
		HST_EnemyCounterattackOperationService owner
			= new HST_EnemyCounterattackOperationService();
		string authorityFailure = owner.DebugValidateOpenRuntimeAuthority(state, order);
		if (!authorityFailure.IsEmpty())
		{
			evidence = authorityFailure;
			return false;
		}
		string endpointEvidence;
		bool endpointExact = ValidateExternalEndpointOwnershipAuthority(
			state,
			carrier,
			endpointEvidence);
		bool ownershipClaimantsExact
			= CountExternalOwnershipTransitionClaimants(
				state,
				expected.m_sOperationId) == 0;
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		bool rowsExact = CountEnemyOrderId(state, expected.m_sOrderId) == 1
			&& CountOperationId(state, expected.m_sOperationId) == 1
			&& CountManifestId(state, expected.m_sManifestId) == 1
			&& CountBatchId(state, expected.m_sBatchId) == 1
			&& CountGroupId(state, expected.m_sGroupId) == 1
			&& CountMutationId(state, expected.m_sDebitMutationId) == 1;
		bool orderExact = order.m_sOperationId == expected.m_sOperationId
			&& order.m_sManifestId == expected.m_sManifestId
			&& order.m_sManifestHash == expected.m_sManifestHash
			&& order.m_sSpawnResultId == expected.m_sBatchId
			&& order.m_sGroupId == expected.m_sGroupId
			&& order.m_sFactionKey == expected.m_sFactionKey
			&& order.m_sSourceZoneId == expected.m_sSourceZoneId
			&& order.m_sTargetZoneId == expected.m_sTargetZoneId
			&& order.m_eStatus
				== HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& order.m_bStrategicServiceCommitted
			&& !order.m_bPhysicalized && !order.m_bAbstractResolved
			&& !order.m_bOutcomeApplied && !order.m_bResourceRefundApplied
			&& !order.m_bResourceSettlementApplied
			&& order.m_sRuntimeStatus == "exact_materializing";
		bool operationIdentityExact
			= operation.m_sEnemyOrderId == expected.m_sOrderId
			&& operation.m_sManifestId == expected.m_sManifestId
			&& operation.m_sSpawnResultId == expected.m_sBatchId
			&& operation.m_sGroupId == expected.m_sGroupId
			&& operation.m_sProjectionId == expected.m_sProjectionId
			&& operation.m_sForceId == expected.m_sForceId;
		bool operationLifecycleExact = operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& operation.m_eResumeDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& operation.m_eMaterializationState
				== HST_EOperationMaterializationState
					.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			&& operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE;
		bool operationCursorExact
			= operation.m_iLastProjectionDecisionSecond == state.m_iElapsedSeconds
			&& !operation.m_sLastProjectionReason.IsEmpty()
			&& Math.AbsFloat(operation.m_fRouteProgressMeters
				- carrier.m_fPreparedRouteProgressMeters) <= 0.1
			&& Math.AbsFloat(operation.m_fRouteTotalDistanceMeters
				- carrier.m_fPreparedRouteTotalDistanceMeters) <= 0.1
			&& VectorExact(operation.m_vStrategicPosition,
				carrier.m_vPreparedStrategicPosition, 0.1)
			&& VectorExact(operation.m_vStrategicPosition, group.m_vPosition, 0.1)
			&& VectorExact(operation.m_vStrategicPosition,
				group.m_vSourcePosition, 0.1)
			&& VectorExact(group.m_vTargetPosition,
				operation.m_vRouteEndPosition, 0.1);
		bool operationExact = operationIdentityExact && operationLifecycleExact;
		operationExact = operationExact && operationCursorExact;
		bool batchIdentityExact = batch.m_sManifestId == expected.m_sManifestId
			&& batch.m_sManifestHash == expected.m_sManifestHash
			&& batch.m_sOperationId == expected.m_sOperationId
			&& batch.m_sResultId == expected.m_sBatchId
			&& batch.m_sRequestId == expected.m_sOrderId
			&& batch.m_sProjectionId == expected.m_sProjectionId
			&& batch.m_sForceId == expected.m_sForceId;
		bool batchLifecycleExact = batch.m_eStatus
				== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			&& !batch.m_bStrategicProjectionHeld
			&& batch.m_iStrategicHoldSinceSecond == 0
			&& batch.m_iNextAttemptSecond == state.m_iElapsedSeconds
			&& batch.m_iDeadlineSecond > state.m_iElapsedSeconds
			&& batch.m_iSuccessfulHandoffCount == 0
			&& batch.m_iReprojectionCount == 0;
		bool batchRuntimeExact = !batch.m_bExternalAssetAuthority
			&& batch.m_sNativeGroupId.IsEmpty()
			&& batch.m_sTerminalReason.IsEmpty();
		bool batchRosterExact = queue.CountStrategicLivingMemberSlots(batch)
				== expected.m_iLivingMemberCount
			&& queue.CountConfirmedCasualtyMemberSlots(batch) == 0
			&& BuildExternalLivingSlotFingerprint(batch)
				== expected.m_sLivingSlotFingerprint
			&& ValidateExternalQueuedSlots(batch, manifest, expected);
		bool batchExact = batchIdentityExact && batchLifecycleExact;
		batchExact = batchExact && batchRuntimeExact && batchRosterExact;
		bool groupIdentityExact = group.m_sOperationId == expected.m_sOperationId
			&& group.m_sManifestId == expected.m_sManifestId
			&& group.m_sSpawnResultId == expected.m_sBatchId
			&& group.m_sGroupId == expected.m_sGroupId
			&& group.m_sProjectionId == expected.m_sProjectionId
			&& group.m_sForceId == expected.m_sForceId
			&& group.m_sEnemyOrderId == expected.m_sOrderId;
		bool groupRuntimeExact
			= group.m_sRuntimeStatus == "enemy_counterattack_materializing"
			&& !group.m_bSpawnedEntity && !group.m_bSpawnAttempted
			&& group.m_sRuntimeEntityId.IsEmpty()
			&& group.m_iSpawnedAgentCount == 0;
		bool groupRosterExact
			= group.m_iInfantryCount == expected.m_iLivingMemberCount
			&& group.m_iOriginalInfantryCount == expected.m_iAcceptedMemberCount
			&& group.m_iLastSeenAliveCount == expected.m_iLivingMemberCount
			&& group.m_iSurvivorInfantryCount == expected.m_iLivingMemberCount
			&& group.m_iDurableLivingInfantryCount
				== expected.m_iLivingMemberCount;
		bool groupExact = groupIdentityExact && groupRuntimeExact;
		groupExact = groupExact && groupRosterExact;
		bool resourcesExact = pool.m_iAttackResources
				== expected.m_iExpectedAttackPool
			&& pool.m_iSupportResources == expected.m_iExpectedSupportPool
			&& pool.m_iStrategicOperationalMutationCount
				== expected.m_iExpectedPoolOperationalMutationCount
			&& ValidateExternalDebit(state, order, pool, expected)
			&& CountExternalSupportClaimants(state, expected) == 0
			&& CountExternalLegacyQRFClaimants(state, expected) == 0;
		bool manifestExact = ValidateExternalVirtualManifest(manifest, expected);
		fingerprint = BuildExternalSemanticFingerprint(state, carrier);
		bool exact = rowsExact && endpointExact && ownershipClaimantsExact;
		exact = exact && orderExact && operationExact && manifestExact
			&& batchExact && groupExact && resourcesExact
			&& fingerprint == carrier.m_sRawPreparedCutSemanticFingerprint;
		if (!exact)
		{
			evidence = string.Format(
				"materializing cut rejected | rows %1 | order %2 | operation %3 | manifest %4 | batch %5 | group %6 | resources %7 | fingerprint %8",
				rowsExact,
				orderExact,
				operationExact,
				manifestExact,
				batchExact,
				groupExact,
				resourcesExact,
				fingerprint == carrier.m_sRawPreparedCutSemanticFingerprint);
			evidence += string.Format(
				" | endpoints/ownership claimants %1/%2 | %3",
				endpointExact,
				ownershipClaimantsExact,
				endpointEvidence);
			return false;
		}
		evidence = string.Format(
			"materializing cut exact | route %1/%2 | living %3 | checkpoint remains virtual",
			Math.Round(operation.m_fRouteProgressMeters),
			Math.Round(operation.m_fRouteTotalDistanceMeters),
			expected.m_iLivingMemberCount);
		return true;
	}

	bool ValidateExternalDematerializingCutState(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "external-counterattack-semantic-unavailable";
		evidence = "external exact counterattack dematerializing cut rejected";
		string carrierEvidence;
		if (!state || !carrier
			|| carrier.m_sCutName
				!= HST_EnemyCounterattackExternalRestartProofService
					.CUT_DEMATERIALIZING_BEFORE_HOLD
			|| !HST_EnemyCounterattackExternalRestartProofService.ValidateCarrier(
				carrier,
				carrier.m_sSessionNonce,
				carrier.m_sRunId,
				carrier.m_sCutName,
				carrier.m_sWorld,
				carrierEvidence))
			return false;
		HST_EnemyCounterattackOutboundVirtualExpectation expected
			= carrier.m_Expectation;
		HST_EnemyOrderState order = state.FindEnemyOrder(expected.m_sOrderId);
		HST_OperationRecordState operation
			= state.FindOperation(expected.m_sOperationId);
		HST_ForceManifestState manifest
			= state.FindForceManifest(expected.m_sManifestId);
		HST_ForceSpawnResultState batch
			= state.FindForceSpawnResult(expected.m_sBatchId);
		HST_ActiveGroupState group = state.FindActiveGroup(expected.m_sGroupId);
		HST_FactionPoolState pool = state.FindFactionPool(expected.m_sFactionKey);
		if (!order || !operation || !manifest || !batch || !group || !pool)
			return false;
		HST_EnemyCounterattackOperationService owner
			= new HST_EnemyCounterattackOperationService();
		string authorityFailure = owner.DebugValidateOpenRuntimeAuthority(state, order);
		if (!authorityFailure.IsEmpty())
		{
			evidence = authorityFailure;
			return false;
		}
		string endpointEvidence;
		bool endpointExact = ValidateExternalEndpointOwnershipAuthority(
			state,
			carrier,
			endpointEvidence);
		bool ownershipClaimantsExact
			= CountExternalOwnershipTransitionClaimants(
				state,
				expected.m_sOperationId) == 0;
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		bool rowsExact = CountEnemyOrderId(state, expected.m_sOrderId) == 1
			&& CountOperationId(state, expected.m_sOperationId) == 1
			&& CountManifestId(state, expected.m_sManifestId) == 1
			&& CountBatchId(state, expected.m_sBatchId) == 1
			&& CountGroupId(state, expected.m_sGroupId) == 1
			&& CountMutationId(state, expected.m_sDebitMutationId) == 1;
		bool cutExact = order.m_eStatus
				== HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& order.m_bStrategicServiceCommitted && order.m_bPhysicalized
			&& order.m_sRuntimeStatus == "exact_dematerializing"
			&& operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& operation.m_eMaterializationState
				== HST_EOperationMaterializationState
					.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING
			&& operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& VectorExact(operation.m_vStrategicPosition, group.m_vPosition, 0.01);
		bool projectionExact = batch.m_eStatus
				== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			&& batch.m_iSuccessfulHandoffCount == 1
			&& batch.m_iReprojectionCount == 0
			&& !batch.m_bStrategicProjectionHeld
			&& !batch.m_sNativeGroupId.IsEmpty()
			&& group.m_bSpawnedEntity && group.m_bSpawnAttempted
			&& group.m_sRuntimeEntityId == batch.m_sNativeGroupId
			&& group.m_iSpawnedAgentCount == expected.m_iLivingMemberCount
			&& group.m_iDurableLivingInfantryCount
				== expected.m_iLivingMemberCount
			&& queue.CountDurableLivingMemberSlots(batch)
				== expected.m_iLivingMemberCount
			&& queue.CountConfirmedCasualtyMemberSlots(batch) == 1
			&& BuildExternalLivingSlotFingerprint(batch)
				== expected.m_sLivingSlotFingerprint
			&& BuildExternalCasualtyTombstoneFingerprint(
				batch,
				expected.m_sConfirmedCasualtySlotId)
				== expected.m_sCasualtyTombstoneFingerprint;
		bool resourcesExact = pool.m_iAttackResources
				== expected.m_iExpectedAttackPool
			&& pool.m_iSupportResources == expected.m_iExpectedSupportPool
			&& ValidateExternalDebit(state, order, pool, expected)
			&& CountExternalSupportClaimants(state, expected) == 0
			&& CountExternalLegacyQRFClaimants(state, expected) == 0;
		fingerprint = BuildExternalSemanticFingerprint(state, carrier);
		bool exact = rowsExact && endpointExact && ownershipClaimantsExact;
		exact = exact && cutExact && projectionExact && resourcesExact
			&& fingerprint == carrier.m_sRawPreparedCutSemanticFingerprint;
		if (!exact)
		{
			evidence = string.Format(
				"dematerializing cut rejected | rows %1 | lifecycle %2 | projection %3 | resources %4 | fingerprint %5",
				rowsExact,
				cutExact,
				projectionExact,
				resourcesExact,
				fingerprint == carrier.m_sRawPreparedCutSemanticFingerprint);
			evidence += string.Format(
				" | endpoints/ownership claimants %1/%2 | %3",
				endpointExact,
				ownershipClaimantsExact,
				endpointEvidence);
			return false;
		}
		evidence = string.Format(
			"dematerializing cut exact | living/casualties %1/1 | live position %2",
			expected.m_iLivingMemberCount,
			group.m_vPosition);
		return true;
	}

	bool ValidateExternalPhysicalCutState(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		HST_ForceSpawnQueueService queue,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "external-counterattack-semantic-unavailable";
		evidence = "external exact counterattack physical cut rejected";
		if (!state || !carrier || !queue || !adapter || !physicalWar
			|| carrier.m_sCutName
				!= HST_EnemyCounterattackExternalRestartProofService
					.CUT_PHYSICAL_LIVE_POSITION
			|| state.m_iSchemaVersion != carrier.m_iCampaignSchemaVersion)
		{
			evidence = "physical cut prerequisite or schema identity rejected";
			return false;
		}
		string carrierEvidence;
		if (!HST_EnemyCounterattackExternalRestartProofService.ValidateCarrier(
				carrier,
				carrier.m_sSessionNonce,
				carrier.m_sRunId,
				carrier.m_sCutName,
				carrier.m_sWorld,
				carrierEvidence))
		{
			evidence = "physical cut carrier rejected | " + carrierEvidence;
			return false;
		}

		HST_EnemyCounterattackOutboundVirtualExpectation expected
			= carrier.m_Expectation;
		HST_EnemyOrderState order = state.FindEnemyOrder(expected.m_sOrderId);
		HST_OperationRecordState operation
			= state.FindOperation(expected.m_sOperationId);
		HST_ForceManifestState manifest
			= state.FindForceManifest(expected.m_sManifestId);
		HST_ForceSpawnResultState batch
			= state.FindForceSpawnResult(expected.m_sBatchId);
		HST_ActiveGroupState group = state.FindActiveGroup(expected.m_sGroupId);
		HST_FactionPoolState pool = state.FindFactionPool(expected.m_sFactionKey);
		if (!order || !operation || !manifest || !batch || !group || !pool)
		{
			evidence = "physical cut aggregate lookup rejected";
			return false;
		}

		HST_EnemyCounterattackOperationService owner
			= new HST_EnemyCounterattackOperationService();
		string authorityFailure = owner.DebugValidateOpenRuntimeAuthority(
			state,
			order);
		if (!authorityFailure.IsEmpty())
		{
			evidence = authorityFailure;
			return false;
		}
		string endpointEvidence;
		bool endpointExact = ValidateExternalEndpointOwnershipAuthority(
			state,
			carrier,
			endpointEvidence);
		bool ownershipClaimantsExact
			= CountExternalOwnershipTransitionClaimants(
				state,
				expected.m_sOperationId) == 0;

		int living = queue.CountDurableLivingMemberSlots(batch);
		int casualties = queue.CountConfirmedCasualtyMemberSlots(batch);
		int projectionHandles
			= adapter.CountHandlesForProjection(expected.m_sProjectionId);
		int resultHandles = adapter.CountHandlesForResultId(expected.m_sBatchId);
		int runtimeMembers = physicalWar.CountForceSpawnRuntimeMembers(group);
		string bindingFailure;
		bool bindingsExact
			= adapter.ValidateExactLivingProjectionBindingsForPersistence(
				state,
				batch,
				queue,
				physicalWar,
				bindingFailure);
		bool rowsExact = CountEnemyOrderId(state, expected.m_sOrderId) == 1
			&& CountOperationId(state, expected.m_sOperationId) == 1
			&& CountManifestId(state, expected.m_sManifestId) == 1
			&& CountBatchId(state, expected.m_sBatchId) == 1
			&& CountGroupId(state, expected.m_sGroupId) == 1
			&& CountMutationId(state, expected.m_sDebitMutationId) == 1;
		bool lifecycleExact = order.m_eStatus
				== HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& order.m_bStrategicServiceCommitted && order.m_bPhysicalized
			&& order.m_sRuntimeStatus.StartsWith("exact_physical")
			&& operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& operation.m_eMaterializationState
				== HST_EOperationMaterializationState
					.HST_OPERATION_MATERIALIZATION_PHYSICAL
			&& operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& VectorExact(
				operation.m_vStrategicPosition,
				group.m_vPosition,
				0.1)
			&& VectorExact(
				group.m_vPosition,
				carrier.m_vPreparedLivePosition,
				0.1);
		bool projectionBatchExact = batch.m_eStatus
			== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			&& !batch.m_bStrategicProjectionHeld
			&& batch.m_iSuccessfulHandoffCount == 1
			&& batch.m_iReprojectionCount == 0
			&& !batch.m_sNativeGroupId.IsEmpty();
		bool projectionGroupExact = group.m_bSpawnedEntity
			&& group.m_bSpawnAttempted
			&& group.m_sRuntimeEntityId == group.m_sGroupId
			&& group.m_sRuntimeStatus == "routing";
		bool projectionRosterExact = living == expected.m_iLivingMemberCount
			&& casualties == 0
			&& BuildExternalLivingSlotFingerprint(batch)
				== expected.m_sLivingSlotFingerprint;
		bool projectionRuntimeExact = projectionHandles
				== carrier.m_iExpectedPhysicalAdapterHandleCount
			&& resultHandles
				== carrier.m_iExpectedPhysicalAdapterHandleCount
			&& runtimeMembers
				== carrier.m_iExpectedPhysicalRuntimeMemberCount
			&& physicalWar.GetForceSpawnGroupRoot(group)
			&& bindingsExact;
		bool projectionExact = projectionBatchExact && projectionGroupExact
			&& projectionRosterExact && projectionRuntimeExact;
		bool rosterExact = group.m_iInfantryCount
				== expected.m_iLivingMemberCount
			&& group.m_iOriginalInfantryCount
				== expected.m_iAcceptedMemberCount
			&& group.m_iLastSeenAliveCount
				== expected.m_iLivingMemberCount
			&& group.m_iSurvivorInfantryCount
				== expected.m_iLivingMemberCount
			&& group.m_iDurableLivingInfantryCount
				== expected.m_iLivingMemberCount;
		bool resourcesExact = pool.m_iAttackResources
				== expected.m_iExpectedAttackPool
			&& pool.m_iSupportResources == expected.m_iExpectedSupportPool
			&& ValidateExternalDebit(state, order, pool, expected)
			&& CountExternalSupportClaimants(state, expected) == 0
			&& CountExternalLegacyQRFClaimants(state, expected) == 0;
		fingerprint = BuildExternalSemanticFingerprint(state, carrier);
		bool exact = rowsExact && endpointExact && ownershipClaimantsExact;
		exact = exact && lifecycleExact && projectionExact
			&& rosterExact && resourcesExact
			&& fingerprint == carrier.m_sRawPreparedCutSemanticFingerprint;
		if (!exact)
		{
			evidence = string.Format(
				"physical cut rejected | rows/lifecycle/projection(batch/group/roster/runtime)/roster/resources %1/%2/%3(%4/%5/%6/%7)/%8/%9",
				rowsExact,
				lifecycleExact,
				projectionExact,
				projectionBatchExact,
				projectionGroupExact,
				projectionRosterExact,
				projectionRuntimeExact,
				rosterExact,
				resourcesExact);
			evidence += string.Format(
				" | living/runtime/handles %1/%2/%3 | group spawned/attempted/id/status %4/%5/%6/%7",
				living,
				runtimeMembers,
				projectionHandles,
				group.m_bSpawnedEntity,
				group.m_bSpawnAttempted,
				group.m_sRuntimeEntityId == group.m_sGroupId,
				group.m_sRuntimeStatus);
			evidence += string.Format(
				" | handoff/reprojection %1/%2 | binding %3",
				batch.m_iSuccessfulHandoffCount,
				batch.m_iReprojectionCount,
				bindingFailure);
			evidence += string.Format(
				" | endpoints/ownership claimants %1/%2 | %3",
				endpointExact,
				ownershipClaimantsExact,
				endpointEvidence);
			return false;
		}
		evidence = string.Format(
			"physical cut exact | root plus member handles %1 | runtime members %2 | live position %3",
			projectionHandles,
			runtimeMembers,
			group.m_vPosition);
		return true;
	}

	protected HST_EnemyCounterattackExternalVirtualCutPolicy BuildExternalVirtualCutPolicy(
		HST_EnemyOrderState order,
		HST_EnemyCounterattackExternalRestartCarrier carrier)
	{
		if (!order || !carrier)
			return null;
		HST_EnemyCounterattackExternalVirtualCutPolicy policy
			= new HST_EnemyCounterattackExternalVirtualCutPolicy();
		policy.m_vExpectedRouteStart = order.m_vSourcePosition;
		policy.m_bDematerializingCut = carrier.m_sCutName
			== HST_EnemyCounterattackExternalRestartProofService
				.CUT_DEMATERIALIZING_BEFORE_HOLD;
		bool physicalLivePositionCut = carrier.m_sCutName
			== HST_EnemyCounterattackExternalRestartProofService
				.CUT_PHYSICAL_LIVE_POSITION;
		if (policy.m_bDematerializingCut)
		{
			policy.m_bAllowZeroRouteProgress = true;
			policy.m_bRequireBatchTerminalReason = true;
			policy.m_iExpectedSuccessfulHandoffCount = 1;
			policy.m_vExpectedRouteStart = carrier.m_vPreparedStrategicPosition;
		}
		else if (physicalLivePositionCut)
		{
			// Restore replans the strategic leg from the sampled live centroid.
			// The successful native projection also retains its completed-attempt
			// audit evidence while process-local entity identities are cleared.
			policy.m_bAllowZeroRouteProgress = true;
			policy.m_bRequireBatchTerminalReason = true;
			policy.m_bRequirePriorBatchAttempt = true;
			policy.m_bExpectedRootEverAlive = true;
			policy.m_iExpectedSuccessfulHandoffCount = 1;
			policy.m_vExpectedRouteStart = carrier.m_vPreparedLivePosition;
		}
		return policy;
	}

	protected bool ValidateExternalVirtualManifest(
		HST_ForceManifestState manifest,
		HST_EnemyCounterattackOutboundVirtualExpectation expected)
	{
		if (!manifest || !expected)
			return false;
		bool exact = manifest.m_bFrozen;
		exact = exact && manifest.m_sManifestHash == expected.m_sManifestHash;
		exact = exact && manifest.m_sOperationId == expected.m_sOperationId;
		exact = exact
			&& manifest.m_iAcceptedMemberCount == expected.m_iAcceptedMemberCount;
		exact = exact && manifest.m_sFactionKey == expected.m_sFactionKey;
		exact = exact && manifest.m_sSourceZoneId == expected.m_sSourceZoneId;
		exact = exact && manifest.m_sTargetZoneId == expected.m_sTargetZoneId;
		if (!exact)
			return false;
		HST_ForcePlanningIntegrityService integrity
			= new HST_ForcePlanningIntegrityService();
		return integrity.BuildManifestHash(manifest) == expected.m_sManifestHash;
	}

	protected bool ValidateExternalVirtualBatch(
		HST_ForceSpawnResultState batch,
		HST_ForceManifestState manifest,
		HST_EnemyCounterattackOutboundVirtualExpectation expected,
		HST_EnemyCounterattackExternalVirtualCutPolicy policy,
		out string evidence)
	{
		evidence = "virtual batch rejected";
		if (!batch || !manifest || !expected || !policy)
			return false;
		bool identityExact = batch.m_sManifestId == expected.m_sManifestId;
		identityExact = identityExact
			&& batch.m_sManifestHash == expected.m_sManifestHash;
		identityExact = identityExact
			&& batch.m_sOperationId == expected.m_sOperationId;
		identityExact = identityExact && batch.m_sResultId == expected.m_sBatchId;
		identityExact = identityExact && batch.m_sRequestId == expected.m_sOrderId;
		identityExact = identityExact
			&& batch.m_sProjectionId == expected.m_sProjectionId;
		identityExact = identityExact && batch.m_sForceId == expected.m_sForceId;

		bool queueExact = batch.m_eStatus
			== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING;
		queueExact = queueExact && batch.m_iPriority
			== HST_EnemyCounterattackOperationService.EXACT_COUNTERATTACK_PRIORITY;
		queueExact = queueExact && batch.m_iRetryCount == 0;
		queueExact = queueExact && batch.m_iMaxRetries
			== HST_EnemyCounterattackOperationService.EXACT_COUNTERATTACK_MAX_RETRIES;
		bool attemptClockExact = batch.m_iLastAttemptSecond == 0;
		if (policy.m_bRequirePriorBatchAttempt)
		{
			attemptClockExact = batch.m_iLastAttemptSecond > 0
				&& batch.m_iLastAttemptSecond <= batch.m_iUpdatedAtSecond;
		}
		queueExact = queueExact && attemptClockExact;
		queueExact = queueExact && batch.m_iNextAttemptSecond == 0;
		queueExact = queueExact && batch.m_iCompletedAtSecond == 0;

		bool rosterExact = batch.m_iExpectedSlotCount
			== manifest.m_iAcceptedMemberCount + 1;
		rosterExact = rosterExact
			&& batch.m_aSlotResults.Count() == batch.m_iExpectedSlotCount;
		rosterExact = rosterExact && batch.m_iSuccessfulHandoffCount
			== policy.m_iExpectedSuccessfulHandoffCount;
		rosterExact = rosterExact && batch.m_iReprojectionCount
			== expected.m_iExpectedNormalizedReprojectionCount;

		bool terminalExact = batch.m_sTerminalReason.IsEmpty();
		if (policy.m_bRequireBatchTerminalReason)
			terminalExact = !batch.m_sTerminalReason.IsEmpty();
		bool lifecycleExact = !batch.m_bCancelRequested;
		lifecycleExact = lifecycleExact && !batch.m_bExternalAssetAuthority;
		lifecycleExact = lifecycleExact && terminalExact;
		lifecycleExact = lifecycleExact && batch.m_sLastFailureReason.IsEmpty();
		lifecycleExact = lifecycleExact && batch.m_bStrategicProjectionHeld;
		lifecycleExact = lifecycleExact && batch.m_sNativeGroupId.IsEmpty();

		bool slotsExact = ValidateExternalQueuedSlots(
			batch,
			manifest,
			expected,
			policy);
		bool exact = identityExact && queueExact;
		exact = exact && rosterExact && lifecycleExact && slotsExact;
		if (!exact)
		{
			evidence = string.Format(
				"batch identity/queue/roster/lifecycle/slots %1/%2/%3/%4/%5",
				identityExact,
				queueExact,
				rosterExact,
				lifecycleExact,
				slotsExact);
			evidence += string.Format(
				" | status/attempt/updated/terminal %1/%2/%3/%4",
				batch.m_eStatus,
				batch.m_iLastAttemptSecond,
				batch.m_iUpdatedAtSecond,
				!batch.m_sTerminalReason.IsEmpty());
			return false;
		}
		evidence = "virtual batch exact";
		return true;
	}

	protected void PopulateExternalVirtualGroupProjectionProof(
		HST_EnemyCounterattackExternalVirtualProjectionProof proof,
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_EnemyCounterattackOutboundVirtualExpectation expected)
	{
		if (!proof || !state || !carrier || !batch || !group || !expected)
			return;
		bool identityExact = group.m_sOperationId == expected.m_sOperationId;
		identityExact = identityExact && group.m_sManifestId == expected.m_sManifestId;
		identityExact = identityExact && group.m_sSpawnResultId == expected.m_sBatchId;
		identityExact = identityExact && group.m_sGroupId == expected.m_sGroupId;
		identityExact = identityExact
			&& group.m_sProjectionId == expected.m_sProjectionId;
		identityExact = identityExact && group.m_sForceId == expected.m_sForceId;
		identityExact = identityExact && group.m_sEnemyOrderId == expected.m_sOrderId;
		identityExact = identityExact && group.m_sSupportRequestId.IsEmpty();
		proof.m_bGroupIdentityExact = identityExact;

		bool processExact = !group.m_bSpawnedEntity;
		processExact = processExact && group.m_sRuntimeEntityId.IsEmpty();
		processExact = processExact && group.m_iSpawnedAgentCount == 0;
		processExact = processExact && !group.m_bSpawnAttempted;
		processExact = processExact && group.m_iAssignedWaypointCount == 0;
		bool vehicleExact = group.m_iVehicleCount == 0;
		vehicleExact = vehicleExact && group.m_iOriginalVehicleCount == 0;
		vehicleExact = vehicleExact && group.m_iSurvivorVehicleCount == 0;
		vehicleExact = vehicleExact && group.m_iCompositionVehicleCount == 0;
		vehicleExact = vehicleExact && group.m_iCompositionArmedVehicleCount == 0;
		bool combatExact = group.m_iCombatEffectiveInfantryCount == 0;
		combatExact = combatExact && group.m_iOperationalMannedVehicleCount == 0;
		combatExact = combatExact
			&& group.m_iCombatEffectiveStaticOperatorCount == 0;
		combatExact = combatExact && group.m_iLastCasualtySecond == 0;
		combatExact = combatExact && group.m_iEliminatedAtSecond == 0;
		bool modeExact = group.m_sSpawnFailureReason.IsEmpty();
		modeExact = modeExact
			&& group.m_sRuntimeStatus == "enemy_counterattack_virtual";
		modeExact = modeExact && group.m_bQRF;

		int living = proof.m_iLivingMemberCount;
		bool rosterExact = living == expected.m_iLivingMemberCount;
		rosterExact = rosterExact && group.m_iInfantryCount == living;
		rosterExact = rosterExact
			&& group.m_iOriginalInfantryCount == expected.m_iAcceptedMemberCount;
		rosterExact = rosterExact && group.m_iLastSeenAliveCount == living;
		rosterExact = rosterExact && group.m_iSurvivorInfantryCount == living;
		rosterExact = rosterExact && group.m_iDurableLivingInfantryCount == living;
		bool slotsExact = BuildExternalLivingSlotFingerprint(batch)
			== expected.m_sLivingSlotFingerprint;
		bool casualtyExact = ValidateExternalCasualtyContinuity(
			state,
			carrier,
			proof.m_sCasualtyEvidence);

		bool runtimeExact = processExact && vehicleExact;
		runtimeExact = runtimeExact && combatExact && modeExact;
		runtimeExact = runtimeExact && rosterExact;
		proof.m_bGroupRuntimeExact = runtimeExact && slotsExact && casualtyExact;
		proof.m_sGroupEvidence = string.Format(
			"group p%1 v%2 c%3 m%4 r%5 s%6 k%7",
			processExact,
			vehicleExact,
			combatExact,
			modeExact,
			rosterExact,
			slotsExact,
			casualtyExact);
	}

	protected HST_EnemyCounterattackExternalVirtualProjectionProof BuildExternalVirtualProjectionProof(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_EnemyCounterattackExternalVirtualCutPolicy policy)
	{
		HST_EnemyCounterattackExternalVirtualProjectionProof proof
			= new HST_EnemyCounterattackExternalVirtualProjectionProof();
		if (!state || !carrier || !carrier.m_Expectation
			|| !manifest || !batch || !group || !policy)
			return proof;
		HST_EnemyCounterattackOutboundVirtualExpectation expected
			= carrier.m_Expectation;
		proof.m_bManifestExact = ValidateExternalVirtualManifest(manifest, expected);
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		proof.m_iLivingMemberCount = queue.CountStrategicLivingMemberSlots(batch);
		proof.m_bBatchExact = ValidateExternalVirtualBatch(
			batch,
			manifest,
			expected,
			policy,
			proof.m_sBatchEvidence);
		PopulateExternalVirtualGroupProjectionProof(
			proof,
			state,
			carrier,
			batch,
			group,
			expected);
		proof.m_bProjectionExact = proof.m_bBatchExact
			&& proof.m_bGroupIdentityExact;
		proof.m_bProjectionExact = proof.m_bProjectionExact
			&& proof.m_bGroupRuntimeExact;
		return proof;
	}

	bool ValidateExternalVirtualState(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "external-counterattack-semantic-unavailable";
		evidence = "external exact counterattack virtual state rejected";
		string carrierEvidence;
		if (!state || !carrier
			|| !HST_EnemyCounterattackExternalRestartProofService.ValidateCarrier(
				carrier,
				carrier.m_sSessionNonce,
				carrier.m_sRunId,
				carrier.m_sCutName,
				carrier.m_sWorld,
				carrierEvidence)
			|| state.m_iSchemaVersion != carrier.m_iCampaignSchemaVersion)
			return false;

		HST_EnemyCounterattackOutboundVirtualExpectation expected
			= carrier.m_Expectation;
		HST_EnemyOrderState order = state.FindEnemyOrder(expected.m_sOrderId);
		HST_OperationRecordState operation
			= state.FindOperation(expected.m_sOperationId);
		HST_ForceManifestState manifest
			= state.FindForceManifest(expected.m_sManifestId);
		HST_ForceSpawnResultState batch
			= state.FindForceSpawnResult(expected.m_sBatchId);
		HST_ActiveGroupState group = state.FindActiveGroup(expected.m_sGroupId);
		HST_FactionPoolState pool = state.FindFactionPool(expected.m_sFactionKey);
		if (!order || !operation || !manifest || !batch || !group || !pool)
			return false;
		HST_EnemyCounterattackExternalVirtualCutPolicy cutPolicy
			= BuildExternalVirtualCutPolicy(order, carrier);
		if (!cutPolicy)
			return false;

		HST_EnemyCounterattackOperationService owner
			= new HST_EnemyCounterattackOperationService();
		string authorityFailure = owner.DebugValidateOpenRuntimeAuthority(state, order);
		if (!authorityFailure.IsEmpty())
		{
			evidence = authorityFailure;
			return false;
		}
		string endpointEvidence;
		bool endpointExact = ValidateExternalEndpointOwnershipAuthority(
			state,
			carrier,
			endpointEvidence);
		bool ownershipClaimantsExact
			= CountExternalOwnershipTransitionClaimants(
				state,
				expected.m_sOperationId) == 0;
		bool rowsExact = CountEnemyOrderId(state, expected.m_sOrderId) == 1
			&& CountOperationId(state, expected.m_sOperationId) == 1
			&& CountManifestId(state, expected.m_sManifestId) == 1
			&& CountBatchId(state, expected.m_sBatchId) == 1
			&& CountGroupId(state, expected.m_sGroupId) == 1
			&& CountMutationId(state, expected.m_sDebitMutationId) == 1;
		bool orderContractExact = order.m_sOperationId == expected.m_sOperationId
			&& order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK
			&& order.m_iOperationContractVersion
				== HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION;
		bool orderLinkExact = order.m_sManifestId == expected.m_sManifestId
			&& order.m_sManifestHash == expected.m_sManifestHash
			&& order.m_sSpawnResultId == expected.m_sBatchId
			&& order.m_sGroupId == expected.m_sGroupId;
		bool orderZoneExact = order.m_sFactionKey == expected.m_sFactionKey
			&& order.m_sSourceZoneId == expected.m_sSourceZoneId
			&& order.m_sTargetZoneId == expected.m_sTargetZoneId;
		bool orderDebitExact = order.m_iAttackCost == expected.m_iAttackCost
			&& order.m_iSupportCost == expected.m_iSupportCost
			&& order.m_iRefundedAttackResources == 0
			&& order.m_iRefundedSupportResources == 0
			&& order.m_sResourceDebitMutationId == expected.m_sDebitMutationId;
		bool orderSettlementExact = !order.m_bResourceSettlementApplied
			&& order.m_sResourceRefundMutationId.IsEmpty()
			&& order.m_sResourceSettlementId.IsEmpty()
			&& order.m_sResourceSettlementKind.IsEmpty()
			&& order.m_iSettlementAcceptedMemberCount == 0
			&& order.m_iSettlementSurvivorMemberCount == 0;
		bool orderStatusExact
			= order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& order.m_bStrategicServiceCommitted
			&& !order.m_bPhysicalized && !order.m_bAbstractResolved
			&& !order.m_bOutcomeApplied && !order.m_bResourceRefundApplied;
		bool orderResultExact = order.m_sSupportRequestId.IsEmpty()
			&& order.m_sResolutionKind.IsEmpty()
			&& order.m_sFailureReason.IsEmpty()
			&& (order.m_sRuntimeStatus == "exact_virtual_outbound"
				|| order.m_sRuntimeStatus == "exact_counterattack_restore_virtual");
		bool orderExact = orderContractExact && orderLinkExact && orderZoneExact;
		orderExact = orderExact && orderDebitExact && orderSettlementExact;
		orderExact = orderExact && orderStatusExact && orderResultExact;

		bool routeContractExact = operation.m_sEnemyOrderId == expected.m_sOrderId
			&& operation.m_eType
				== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK
			&& operation.m_iContractVersion
				== HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION;
		bool routeLinkExact = operation.m_sManifestId == expected.m_sManifestId
			&& operation.m_sSpawnResultId == expected.m_sBatchId
			&& operation.m_sGroupId == expected.m_sGroupId
			&& operation.m_sProjectionId == expected.m_sProjectionId
			&& operation.m_sForceId == expected.m_sForceId;
		bool routeAssignmentExact = operation.m_sOwnerFactionKey == expected.m_sFactionKey
			&& operation.m_sOriginZoneId == expected.m_sSourceZoneId
			&& operation.m_sAssignmentKind
				== HST_EnemyCounterattackOperationService.ASSIGNMENT_KIND
			&& operation.m_sAssignmentZoneId == expected.m_sTargetZoneId
			&& operation.m_sTacticalTargetZoneId == expected.m_sTargetZoneId;
		bool routeNameExact = operation.m_sCurrentRouteId == group.m_sRouteId
			&& operation.m_sCurrentRouteId == expected.m_sOperationId + "_outbound"
			&& operation.m_sRouteContractHash.IsEmpty();
		bool routeDutyExact = operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& operation.m_eResumeDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& operation.m_eEngagementMode
				== HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		bool routeProjectionExact = operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE;
		bool routePolicyExact = operation.m_sRecallPolicyId
				== HST_EnemyCounterattackOperationService.RECALL_POLICY_ID
			&& operation.m_sSettlementPolicyId
				== HST_EnemyCounterattackOperationService.SETTLEMENT_POLICY_ID
			&& operation.m_sSettlementId.IsEmpty()
			&& operation.m_sTerminalReason.IsEmpty();
		bool routeCursorContractExact = operation.m_iRouteVersion
				== HST_StrategicMovementService.DIRECT_ROUTE_VERSION
			&& operation.m_iProjectionContractVersion
				== HST_StrategicMovementService.EXACT_PLAYER_QRF_PROJECTION_CONTRACT_VERSION
			&& operation.m_iRouteWaypointIndex == -1
			&& operation.m_iRouteLapCount == 0
			&& operation.m_iRouteLegSequence == 0;
		bool routeLoopExact = operation.m_iRouteLoopStartedAtSecond == 0
			&& operation.m_iRouteLoopCompletedAtSecond == 0;
		bool routeEndpointsExact
			= VectorExact(
				operation.m_vRouteStartPosition,
				cutPolicy.m_vExpectedRouteStart,
				0.1)
			&& VectorExact(operation.m_vRouteEndPosition, order.m_vTargetPosition, 0.1)
			&& VectorExact(operation.m_vOriginPosition, order.m_vSourcePosition, 0.1)
			&& VectorExact(operation.m_vAssignmentPosition, order.m_vTargetPosition, 0.1)
			&& VectorExact(operation.m_vTacticalTargetPosition, order.m_vTargetPosition, 0.1);
		bool routeProgressExact = operation.m_fRouteProgressMeters > 0;
		if (cutPolicy.m_bAllowZeroRouteProgress)
			routeProgressExact = operation.m_fRouteProgressMeters >= 0;
		bool routeClockExact = Math.AbsFloat(operation.m_fStrategicSpeedMetersPerSecond
				- HST_StrategicMovementService.EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND) <= 0.01
			&& operation.m_iStrategicLastUpdateSecond == state.m_iElapsedSeconds
			&& operation.m_iLastProgressAtSecond
				== operation.m_iStrategicLastUpdateSecond
			&& routeProgressExact
			&& operation.m_fRouteProgressMeters < operation.m_fRouteTotalDistanceMeters
			&& Math.AbsFloat(operation.m_fRouteTotalDistanceMeters
				- carrier.m_fPreparedRouteTotalDistanceMeters) <= 0.1;
		bool routePositionExact
			= VectorExact(operation.m_vStrategicPosition, group.m_vPosition, 0.1)
			&& VectorExact(operation.m_vStrategicPosition, group.m_vSourcePosition, 0.1)
			&& VectorExact(group.m_vTargetPosition, order.m_vTargetPosition, 0.1);
		bool routeArrivalExact = operation.m_iArrivalConfirmationCount == 0
			&& operation.m_iLastArrivalConfirmationSecond == 0;
		bool routeCombatExact = operation.m_iVirtualCombatStepIndex == 0
			&& operation.m_iVirtualCombatFriendlyDamageCarry == 0
			&& operation.m_iVirtualCombatHostileDamageCarry == 0
			&& operation.m_iLastVirtualFriendlyCount == expected.m_iLivingMemberCount
			&& operation.m_iLastVirtualHostileCount == 0
			&& operation.m_sLastVirtualCombatReason.IsEmpty();
		bool routeExact = routeContractExact && routeLinkExact;
		routeExact = routeExact && routeAssignmentExact && routeNameExact;
		routeExact = routeExact && routeDutyExact && routeProjectionExact;
		routeExact = routeExact && routePolicyExact && routeCursorContractExact;
		routeExact = routeExact && routeLoopExact && routeEndpointsExact;
		routeExact = routeExact && routeClockExact && routePositionExact;
		routeExact = routeExact && routeArrivalExact && routeCombatExact
			&& operation.m_sSupportRequestId.IsEmpty();
		HST_EnemyCounterattackExternalVirtualProjectionProof projectionProof
			= BuildExternalVirtualProjectionProof(
				state,
				carrier,
				manifest,
				batch,
				group,
				cutPolicy);
		if (!projectionProof)
			return false;
		bool poolExact = pool.m_iAttackResources
				== expected.m_iExpectedAttackPool
			&& pool.m_iSupportResources == expected.m_iExpectedSupportPool
			&& expected.m_iExpectedPoolOperationalMutationCount == 1
			&& pool.m_iStrategicOperationalMutationCount
				== expected.m_iExpectedPoolOperationalMutationCount
			&& pool.m_sLastStrategicMutationId == expected.m_sDebitMutationId;
		bool receiptExact = ValidateExternalDebit(state, order, pool, expected)
			&& CountExternalSupportClaimants(state, expected) == 0
			&& CountExternalLegacyQRFClaimants(state, expected) == 0;
		bool resourcesExact = poolExact && receiptExact;

		fingerprint = BuildExternalSemanticFingerprint(state, carrier);
		if (!rowsExact || !endpointExact || !ownershipClaimantsExact
			|| !orderExact || !routeExact
			|| !projectionProof.m_bManifestExact
			|| !projectionProof.m_bProjectionExact || !resourcesExact
			|| fingerprint == "external-counterattack-semantic-unavailable")
		{
			evidence = string.Format(
				"external exact counterattack virtual state rejected | rows %1 | order %2 | route %3 | manifest %4",
				rowsExact,
				orderExact,
				routeExact,
				projectionProof.m_bManifestExact);
			evidence += string.Format(
				" | projection %1 | resources %2 | fingerprint %3",
				projectionProof.m_bProjectionExact,
				resourcesExact,
				fingerprint != "external-counterattack-semantic-unavailable");
			evidence += " | " + projectionProof.m_sGroupEvidence;
			evidence += " | " + projectionProof.m_sBatchEvidence;
			evidence += string.Format(
				" | b%1 g%2",
				projectionProof.m_bBatchExact,
				projectionProof.m_bGroupRuntimeExact);
			if (!routeExact)
			{
				evidence += string.Format(
					" | route c%1 l%2 a%3 n%4 d%5 p%6 y%7 u%8",
					routeContractExact,
					routeLinkExact,
					routeAssignmentExact,
					routeNameExact,
					routeDutyExact,
					routeProjectionExact,
					routePolicyExact,
					routeCursorContractExact);
				evidence += string.Format(
					" o%1 e%2 k%3 x%4 v%5 b%6",
					routeLoopExact,
					routeEndpointsExact,
					routeClockExact,
					routePositionExact,
					routeArrivalExact,
					routeCombatExact);
			}
			evidence += string.Format(
				" | pool %1 | receipt %2",
				poolExact,
				receiptExact);
			evidence += string.Format(
				" | endpoints/ownership claimants %1/%2 | %3",
				endpointExact,
				ownershipClaimantsExact,
				endpointEvidence);
			evidence += " | casualty "
				+ projectionProof.m_sCasualtyEvidence;
			return false;
		}
		evidence = string.Format(
			"external exact counterattack virtual authority exact | route %1/%2 | living %3",
			Math.Round(operation.m_fRouteProgressMeters),
			Math.Round(operation.m_fRouteTotalDistanceMeters),
			projectionProof.m_iLivingMemberCount);
		return true;
	}

	protected HST_EnemyCounterattackPreparedSettlementExpectation BuildExternalPreparedSettlementExpectation(
			HST_EnemyCounterattackOperationProofFixture fixture,
			int attackBeforeRefund,
			int supportBeforeRefund,
			int poolRevisionBeforeRefund,
			int operationalMutationsBeforeRefund)
	{
		if (!m_Fixtures.Ready(fixture) || !fixture.m_Order
			|| !fixture.m_Operation || !fixture.m_Manifest
			|| !fixture.m_Batch || !fixture.m_Group)
			return null;
		HST_EnemyCounterattackPreparedSettlementExpectation expectation
			= new HST_EnemyCounterattackPreparedSettlementExpectation();
		expectation.m_sOrderId = fixture.m_Order.m_sOrderId;
		expectation.m_sOperationId = fixture.m_Operation.m_sOperationId;
		expectation.m_sManifestId = fixture.m_Manifest.m_sManifestId;
		expectation.m_sManifestHash = fixture.m_Manifest.m_sManifestHash;
		expectation.m_sBatchId = fixture.m_Batch.m_sResultId;
		expectation.m_sGroupId = fixture.m_Group.m_sGroupId;
		expectation.m_sProjectionId = fixture.m_Batch.m_sProjectionId;
		expectation.m_sForceId = fixture.m_Batch.m_sForceId;
		expectation.m_sFactionKey = fixture.m_Order.m_sFactionKey;
		expectation.m_sSourceZoneId = fixture.m_Order.m_sSourceZoneId;
		expectation.m_sTargetZoneId = fixture.m_Order.m_sTargetZoneId;
		if (!PopulateExternalSettlementEndpointOwnershipExpectation(
			fixture.m_State,
			expectation))
			return null;
		expectation.m_sDebitMutationId
			= fixture.m_Order.m_sResourceDebitMutationId;
		expectation.m_sSettlementKind
			= fixture.m_Order.m_sResourceSettlementKind;
		expectation.m_sSettlementId
			= fixture.m_Order.m_sResourceSettlementId;
		expectation.m_sRefundMutationId
			= fixture.m_Order.m_sResourceRefundMutationId;
		expectation.m_sReason = fixture.m_Operation.m_sTerminalReason;
		expectation.m_iAttackCost = fixture.m_Order.m_iAttackCost;
		expectation.m_iSupportCost = fixture.m_Order.m_iSupportCost;
		expectation.m_iAccepted
			= fixture.m_Order.m_iSettlementAcceptedMemberCount;
		expectation.m_iSurvivors
			= fixture.m_Order.m_iSettlementSurvivorMemberCount;
		expectation.m_iAttackRefund
			= fixture.m_Order.m_iRefundedAttackResources;
		expectation.m_iSupportRefund
			= fixture.m_Order.m_iRefundedSupportResources;
		expectation.m_iExpectedAttackPool
			= attackBeforeRefund + expectation.m_iAttackRefund;
		expectation.m_iExpectedSupportPool
			= supportBeforeRefund + expectation.m_iSupportRefund;
		expectation.m_iExpectedPoolRevision = poolRevisionBeforeRefund + 1;
		expectation.m_iExpectedPoolOperationalMutationCount
			= operationalMutationsBeforeRefund + 1;
		expectation.m_sExpectedLastStrategicMutationId
			= expectation.m_sRefundMutationId;
		expectation.m_iPreparedAtSecond
			= fixture.m_Operation.m_iSettledAtSecond;
		expectation.m_iExpectedTerminalRevision
			= fixture.m_Operation.m_iRevision + 2;
		if (fixture.m_Order.m_bResourceSettlementApplied)
			expectation.m_iExpectedTerminalRevision
				= fixture.m_Operation.m_iRevision + 1;
		return expectation;
	}

	protected bool IsExternalPreparedSettlementExact(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string evidence)
	{
		evidence = "counterattack PREPARED settlement aggregate is incomplete";
		if (!state || !carrier || !carrier.m_SettlementExpectation)
			return false;
		HST_EnemyCounterattackPreparedSettlementExpectation expected
			= carrier.m_SettlementExpectation;
		HST_EnemyOrderState order = state.FindEnemyOrder(expected.m_sOrderId);
		HST_OperationRecordState operation
			= state.FindOperation(expected.m_sOperationId);
		HST_ForceManifestState manifest
			= state.FindForceManifest(expected.m_sManifestId);
		HST_ForceSpawnResultState batch
			= state.FindForceSpawnResult(expected.m_sBatchId);
		HST_ActiveGroupState group = state.FindActiveGroup(expected.m_sGroupId);
		HST_FactionPoolState pool = state.FindFactionPool(expected.m_sFactionKey);
		if (!order || !operation || !manifest || !batch || !group || !pool)
			return false;
		string endpointEvidence;
		bool endpointExact = ValidateExternalEndpointOwnershipAuthority(
			state,
			carrier,
			endpointEvidence);
		bool ownershipClaimantsExact
			= CountExternalOwnershipTransitionClaimants(
				state,
				expected.m_sOperationId) == 0;

		bool carrierExact = carrier.m_iAccepted == expected.m_iAccepted
			&& carrier.m_iCasualties
				== expected.m_iAccepted - expected.m_iSurvivors
			&& carrier.m_iSurvivors == expected.m_iSurvivors
			&& carrier.m_iAttackRefund == expected.m_iAttackRefund
			&& carrier.m_iSupportRefund == expected.m_iSupportRefund
			&& carrier.m_iPreparedAtSecond == expected.m_iPreparedAtSecond
			&& carrier.m_sSettlementKind == expected.m_sSettlementKind
			&& carrier.m_sSettlementId == expected.m_sSettlementId
			&& carrier.m_sRefundMutationId == expected.m_sRefundMutationId
			&& carrier.m_sReason == expected.m_sReason
			&& carrier.m_iAccepted > 1 && carrier.m_iCasualties > 0
			&& carrier.m_iSurvivors > 0
			&& carrier.m_iSurvivors < carrier.m_iAccepted;
		bool identityExact = order.m_sOperationId == expected.m_sOperationId
			&& order.m_sManifestId == expected.m_sManifestId
			&& order.m_sManifestHash == expected.m_sManifestHash
			&& order.m_sSpawnResultId == expected.m_sBatchId
			&& order.m_sGroupId == expected.m_sGroupId
			&& order.m_sFactionKey == expected.m_sFactionKey
			&& order.m_sSourceZoneId == expected.m_sSourceZoneId
			&& order.m_sTargetZoneId == expected.m_sTargetZoneId
			&& order.m_sResourceDebitMutationId == expected.m_sDebitMutationId;
		bool orderExact = order.m_iOperationContractVersion
				== HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION
			&& order.m_eType
				== HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE
			&& order.m_sRuntimeStatus == "exact_virtual_outbound";
		orderExact = orderExact
			&& order.m_iAttackCost == expected.m_iAttackCost
			&& order.m_iSupportCost == expected.m_iSupportCost
			&& order.m_iAttackCost > 0 && order.m_iSupportCost == 0
			&& order.m_bStrategicServiceCommitted;
		orderExact = orderExact && !order.m_bPhysicalized
			&& !order.m_bAbstractResolved && !order.m_bOutcomeApplied
			&& !order.m_bResourceRefundApplied
			&& order.m_sResourceSettlementId == expected.m_sSettlementId
			&& order.m_sResourceSettlementKind == expected.m_sSettlementKind;
		orderExact = orderExact && order.m_sResourceRefundMutationId
				== expected.m_sRefundMutationId
			&& order.m_iSettlementAcceptedMemberCount == expected.m_iAccepted
			&& order.m_iSettlementSurvivorMemberCount == expected.m_iSurvivors
			&& order.m_iRefundedAttackResources == expected.m_iAttackRefund
			&& order.m_iRefundedSupportResources == expected.m_iSupportRefund
			&& order.m_bResourceSettlementApplied
				== carrier.m_bExpectedPrefixReceiptApplied;
		bool operationExact = operation.m_eType
				== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK
			&& operation.m_iContractVersion
				== HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION
			&& operation.m_sEnemyOrderId == expected.m_sOrderId
			&& operation.m_sManifestId == expected.m_sManifestId
			&& operation.m_sSpawnResultId == expected.m_sBatchId;
		operationExact = operationExact
			&& operation.m_sGroupId == expected.m_sGroupId
			&& operation.m_sProjectionId == expected.m_sProjectionId
			&& operation.m_sForceId == expected.m_sForceId
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED
			&& operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED;
		operationExact = operationExact
			&& operation.m_sSettlementId == expected.m_sSettlementId
			&& operation.m_sTerminalReason == expected.m_sReason
			&& operation.m_iSettledAtSecond == expected.m_iPreparedAtSecond
			&& operation.m_iRevision == carrier.m_iPrefixRevision
			&& operation.m_eDutyState
				!= HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			&& operation.m_eMaterializationState
				!= HST_EOperationMaterializationState
					.HST_OPERATION_MATERIALIZATION_RETIRED;
		bool manifestExact = manifest.m_bFrozen
			&& manifest.m_sManifestId == expected.m_sManifestId
			&& manifest.m_sOperationId == expected.m_sOperationId
			&& manifest.m_sManifestHash == expected.m_sManifestHash
			&& manifest.m_iAcceptedMemberCount == expected.m_iAccepted;
		bool executionExact = batch.m_sResultId == expected.m_sBatchId
			&& batch.m_sRequestId == expected.m_sOrderId
			&& batch.m_sOperationId == expected.m_sOperationId
			&& batch.m_sManifestId == expected.m_sManifestId
			&& batch.m_sProjectionId == expected.m_sProjectionId
			&& batch.m_sForceId == expected.m_sForceId;
		executionExact = executionExact
			&& group.m_sGroupId == expected.m_sGroupId
			&& group.m_sEnemyOrderId == expected.m_sOrderId
			&& group.m_sOperationId == expected.m_sOperationId
			&& group.m_sManifestId == expected.m_sManifestId
			&& group.m_sSpawnResultId == expected.m_sBatchId
			&& group.m_sProjectionId == expected.m_sProjectionId
			&& group.m_sForceId == expected.m_sForceId;

		int missingRefundRevision = 0;
		if (carrier.m_iExpectedPrefixMutationCount == 0)
			missingRefundRevision = 1;
		bool poolExact = pool.m_iAttackResources
				== carrier.m_iAttackBeforeRefund
					+ carrier.m_iExpectedPrefixAttackDelta
			&& pool.m_iSupportResources
				== carrier.m_iSupportBeforeRefund
					+ carrier.m_iExpectedPrefixSupportDelta
			&& pool.m_iStrategicRevision
				== expected.m_iExpectedPoolRevision - missingRefundRevision
			&& pool.m_iStrategicOperationalMutationCount
				== expected.m_iExpectedPoolOperationalMutationCount
					- missingRefundRevision;
		string expectedLastMutationId = expected.m_sRefundMutationId;
		if (carrier.m_iExpectedPrefixMutationCount == 0)
			expectedLastMutationId = expected.m_sDebitMutationId;
		poolExact = poolExact
			&& pool.m_sLastStrategicMutationId == expectedLastMutationId;
		bool mutationExact = CountMutationId(state, expected.m_sDebitMutationId) == 1
			&& CountMutationId(state, expected.m_sRefundMutationId)
				== carrier.m_iExpectedPrefixMutationCount
			&& state.m_aEnemyStrategicMutations.Count()
				== 1 + carrier.m_iExpectedPrefixMutationCount;
		string debitFailure
			= HST_EnemyCounterattackSaveValidationService
				.ValidateOriginalResourceDebitAuthority(
					state.m_aEnemyStrategicMutations,
					order);
		string aggregateFailure
			= HST_EnemyCounterattackSaveValidationService
				.ValidatePendingResourceRefundAggregateAuthority(
					state.m_aEnemyStrategicMutations,
					order,
					operation,
					manifest,
					batch,
					group);
		bool claimantExact = CountExternalSettlementBatchClaimants(state, expected) == 1
			&& CountExternalSettlementGroupClaimants(state, expected) == 1
			&& CountExternalSettlementForeignClaimants(state, expected) == 0;
		bool exact = endpointExact && ownershipClaimantsExact;
		exact = exact && carrierExact && identityExact && orderExact
			&& operationExact && manifestExact && executionExact && poolExact
			&& mutationExact && debitFailure.IsEmpty()
			&& aggregateFailure.IsEmpty() && claimantExact;
		if (!exact)
		{
			evidence = string.Format(
				"PREPARED exact carrier/id/order/op/manifest/execution/pool/mutation/claimants %1/%2/%3/%4/%5/%6/%7/%8/%9",
				carrierExact,
				identityExact,
				orderExact,
				operationExact,
				manifestExact,
				executionExact,
				poolExact,
				mutationExact,
				claimantExact);
			evidence += string.Format(
				" | authority %1",
				debitFailure.IsEmpty() && aggregateFailure.IsEmpty());
			evidence += string.Format(
				" | endpoints/ownership claimants %1/%2 | %3",
				endpointExact,
				ownershipClaimantsExact,
				endpointEvidence);
			if (!debitFailure.IsEmpty())
				evidence += " | debit " + debitFailure;
			if (!aggregateFailure.IsEmpty())
				evidence += " | aggregate " + aggregateFailure;
		}
		return exact;
	}

	protected bool IsExternalTerminalSettlementExact(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string evidence)
	{
		evidence = "counterattack terminal settlement aggregate is incomplete";
		if (!state || !carrier || !carrier.m_SettlementExpectation)
			return false;
		HST_EnemyCounterattackPreparedSettlementExpectation expected
			= carrier.m_SettlementExpectation;
		HST_EnemyOrderState order = state.FindEnemyOrder(expected.m_sOrderId);
		HST_OperationRecordState operation
			= state.FindOperation(expected.m_sOperationId);
		HST_ForceManifestState manifest
			= state.FindForceManifest(expected.m_sManifestId);
		HST_FactionPoolState pool = state.FindFactionPool(expected.m_sFactionKey);
		if (!order || !operation || !manifest || !pool)
			return false;
		string endpointEvidence;
		bool endpointExact = ValidateExternalEndpointOwnershipAuthority(
			state,
			carrier,
			endpointEvidence);
		bool ownershipClaimantsExact
			= CountExternalOwnershipTransitionClaimants(
				state,
				expected.m_sOperationId) == 0;

		bool orderExact = order.m_sOperationId == expected.m_sOperationId
			&& order.m_sManifestId == expected.m_sManifestId
			&& order.m_sManifestHash == expected.m_sManifestHash
			&& order.m_sSpawnResultId == expected.m_sBatchId
			&& order.m_sGroupId == expected.m_sGroupId;
		orderExact = orderExact
			&& order.m_sFactionKey == expected.m_sFactionKey
			&& order.m_sSourceZoneId == expected.m_sSourceZoneId
			&& order.m_sTargetZoneId == expected.m_sTargetZoneId
			&& order.m_iOperationContractVersion
				== HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION
			&& order.m_eType
				== HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK;
		orderExact = orderExact
			&& order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED
			&& order.m_sRuntimeStatus == "resolved_exact_terminal"
			&& order.m_sResolutionKind == expected.m_sSettlementKind
			&& order.m_sFailureReason == expected.m_sReason
			&& order.m_iResolvedAtSecond == expected.m_iPreparedAtSecond;
		orderExact = orderExact
			&& order.m_iAttackCost == expected.m_iAttackCost
			&& order.m_iSupportCost == expected.m_iSupportCost
			&& order.m_bStrategicServiceCommitted
			&& !order.m_bPhysicalized && !order.m_bAbstractResolved
			&& !order.m_bOutcomeApplied && !order.m_bResourceRefundApplied
			&& order.m_bResourceSettlementApplied;
		orderExact = orderExact
			&& order.m_sResourceDebitMutationId == expected.m_sDebitMutationId
			&& order.m_sResourceSettlementId == expected.m_sSettlementId
			&& order.m_sResourceSettlementKind == expected.m_sSettlementKind
			&& order.m_sResourceRefundMutationId
				== expected.m_sRefundMutationId
			&& order.m_iSettlementAcceptedMemberCount == expected.m_iAccepted
			&& order.m_iSettlementSurvivorMemberCount == expected.m_iSurvivors
			&& order.m_iRefundedAttackResources == expected.m_iAttackRefund
			&& order.m_iRefundedSupportResources == expected.m_iSupportRefund;
		bool operationExact = operation.m_eType
				== HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK
			&& operation.m_iContractVersion
				== HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION
			&& operation.m_sEnemyOrderId == expected.m_sOrderId
			&& operation.m_sManifestId == expected.m_sManifestId
			&& operation.m_sSpawnResultId == expected.m_sBatchId;
		operationExact = operationExact
			&& operation.m_sGroupId == expected.m_sGroupId
			&& operation.m_sProjectionId == expected.m_sProjectionId
			&& operation.m_sForceId == expected.m_sForceId
			&& operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED;
		operationExact = operationExact
			&& operation.m_sSettlementId == expected.m_sSettlementId
			&& operation.m_sTerminalReason == expected.m_sReason
			&& operation.m_iSettledAtSecond == expected.m_iPreparedAtSecond
			&& operation.m_iRevision == expected.m_iExpectedTerminalRevision
			&& operation.m_eDutyState
				== HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			&& operation.m_eEngagementMode
				== HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
			&& operation.m_eMaterializationState
				== HST_EOperationMaterializationState
					.HST_OPERATION_MATERIALIZATION_RETIRED;
		operationExact = operationExact
			&& operation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& operation.m_iDutyStateEnteredAtSecond == expected.m_iPreparedAtSecond
			&& operation.m_iEngagementStateEnteredAtSecond
				== expected.m_iPreparedAtSecond
			&& operation.m_iMaterializationStateEnteredAtSecond
				== expected.m_iPreparedAtSecond
			&& operation.m_iLastProgressAtSecond == expected.m_iPreparedAtSecond
			&& operation.m_iArrivalConfirmationCount == 0
			&& operation.m_iLastArrivalConfirmationSecond == 0;
		bool manifestExact = manifest.m_bFrozen
			&& manifest.m_sManifestId == expected.m_sManifestId
			&& manifest.m_sOperationId == expected.m_sOperationId
			&& manifest.m_sManifestHash == expected.m_sManifestHash
			&& manifest.m_iAcceptedMemberCount == expected.m_iAccepted;
		bool poolExact = pool.m_iAttackResources == expected.m_iExpectedAttackPool
			&& pool.m_iSupportResources == expected.m_iExpectedSupportPool
			&& pool.m_iStrategicRevision == expected.m_iExpectedPoolRevision
			&& pool.m_iStrategicOperationalMutationCount
				== expected.m_iExpectedPoolOperationalMutationCount
			&& pool.m_sLastStrategicMutationId
				== expected.m_sExpectedLastStrategicMutationId;
		string debitFailure
			= HST_EnemyCounterattackSaveValidationService
				.ValidateOriginalResourceDebitAuthority(
					state.m_aEnemyStrategicMutations,
					order);
		string refundFailure
			= HST_EnemyCounterattackSaveValidationService
				.ValidateSettledResourceRefundAuthority(
					state.m_aEnemyStrategicMutations,
					order);
		bool mutationExact = state.m_aEnemyStrategicMutations.Count() == 2
			&& CountMutationId(state, expected.m_sDebitMutationId) == 1
			&& CountMutationId(state, expected.m_sRefundMutationId) == 1;
		bool rowsExact = CountEnemyOrderId(state, expected.m_sOrderId) == 1
			&& CountOperationId(state, expected.m_sOperationId) == 1
			&& CountManifestId(state, expected.m_sManifestId) == 1
			&& CountExternalSettlementBatchClaimants(state, expected) == 0
			&& CountExternalSettlementGroupClaimants(state, expected) == 0
			&& CountExternalSettlementForeignClaimants(state, expected) == 0
			&& !state.FindForceSpawnResult(expected.m_sBatchId)
			&& !state.FindActiveGroup(expected.m_sGroupId);
		bool exact = endpointExact && ownershipClaimantsExact;
		exact = exact && orderExact && operationExact && manifestExact && poolExact
			&& mutationExact && rowsExact && debitFailure.IsEmpty()
			&& refundFailure.IsEmpty();
		if (!exact)
		{
			evidence = string.Format(
				"terminal exact order/op/manifest/pool/mutation/rows/authority %1/%2/%3/%4/%5/%6/%7",
				orderExact,
				operationExact,
				manifestExact,
				poolExact,
				mutationExact,
				rowsExact,
				debitFailure.IsEmpty() && refundFailure.IsEmpty());
			if (!debitFailure.IsEmpty())
				evidence += " | debit " + debitFailure;
			if (!refundFailure.IsEmpty())
				evidence += " | refund " + refundFailure;
			evidence += string.Format(
				" | endpoints/ownership claimants %1/%2 | %3",
				endpointExact,
				ownershipClaimantsExact,
				endpointEvidence);
		}
		return exact;
	}

	protected string BuildExternalPreparedSettlementFingerprint(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier)
	{
		if (!state || !carrier || !carrier.m_SettlementExpectation)
			return "external-counterattack-prepared-settlement-unavailable";
		HST_EnemyCounterattackPreparedSettlementExpectation expected
			= carrier.m_SettlementExpectation;
		HST_EnemyOrderState order = state.FindEnemyOrder(expected.m_sOrderId);
		HST_OperationRecordState operation
			= state.FindOperation(expected.m_sOperationId);
		HST_ForceManifestState manifest
			= state.FindForceManifest(expected.m_sManifestId);
		HST_ForceSpawnResultState batch
			= state.FindForceSpawnResult(expected.m_sBatchId);
		HST_ActiveGroupState group = state.FindActiveGroup(expected.m_sGroupId);
		HST_FactionPoolState pool = state.FindFactionPool(expected.m_sFactionKey);
		HST_EnemyStrategicMutationState debit
			= FindExternalMutation(state, expected.m_sDebitMutationId);
		HST_EnemyStrategicMutationState refund
			= FindExternalMutation(state, expected.m_sRefundMutationId);
		HST_ZoneState sourceZone = state.FindZone(expected.m_sSourceZoneId);
		HST_ZoneState targetZone = state.FindZone(expected.m_sTargetZoneId);
		if (!order || !operation || !manifest || !batch || !group || !pool
			|| !debit || !sourceZone || !targetZone)
			return "external-counterattack-prepared-settlement-incomplete";
		string fingerprint = string.Format(
			"schema/cut/elapsed %1/%2/%3 | prefix %4/%5/%6/%7/%8",
			state.m_iSchemaVersion,
			carrier.m_iCut,
			state.m_iElapsedSeconds,
			carrier.m_iPrefixRevision,
			carrier.m_iExpectedPrefixMutationCount,
			carrier.m_iExpectedPrefixAttackDelta,
			carrier.m_iExpectedPrefixSupportDelta,
			carrier.m_bExpectedPrefixReceiptApplied);
		fingerprint += " | order " + BuildExternalOrderSemanticRow(order);
		fingerprint += " | operation "
			+ BuildExternalOperationSemanticRow(operation, false);
		fingerprint += " | operation revision " + operation.m_iRevision.ToString();
		fingerprint += " | manifest " + BuildExternalManifestSemanticRow(manifest);
		fingerprint += " | batch " + BuildExternalBatchSemanticRow(batch);
		fingerprint += " | slots " + BuildExternalSlotSemanticRows(batch);
		fingerprint += " | group " + BuildExternalGroupSemanticRow(group, false);
		fingerprint += " | pool " + BuildExternalPoolSemanticRow(pool);
		fingerprint += " | debit " + BuildExternalMutationSemanticRow(debit);
		fingerprint += " | refund " + BuildExternalMutationSemanticRow(refund);
		fingerprint += " | source zone "
			+ BuildExternalEndpointOwnershipSemanticRow(sourceZone);
		fingerprint += " | target zone "
			+ BuildExternalEndpointOwnershipSemanticRow(targetZone);
		fingerprint += " | ownership claimants "
			+ CountExternalOwnershipTransitionClaimants(
				state,
				expected.m_sOperationId).ToString();
		fingerprint += string.Format(
			" | rows %1/%2/%3/%4/%5/%6/%7 | execution %8/%9",
			CountEnemyOrderId(state, expected.m_sOrderId),
			CountOperationId(state, expected.m_sOperationId),
			CountManifestId(state, expected.m_sManifestId),
			CountBatchId(state, expected.m_sBatchId),
			CountGroupId(state, expected.m_sGroupId),
			CountMutationId(state, expected.m_sDebitMutationId),
			CountMutationId(state, expected.m_sRefundMutationId),
			CountExternalSettlementBatchClaimants(state, expected),
			CountExternalSettlementGroupClaimants(state, expected));
		fingerprint += " | foreign "
			+ CountExternalSettlementForeignClaimants(state, expected).ToString();
		fingerprint += " | mutation total "
			+ state.m_aEnemyStrategicMutations.Count().ToString();
		return fingerprint;
	}

	protected string BuildExternalTerminalSettlementFingerprint(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier)
	{
		if (!state || !carrier || !carrier.m_SettlementExpectation)
			return "external-counterattack-terminal-settlement-unavailable";
		HST_EnemyCounterattackPreparedSettlementExpectation expected
			= carrier.m_SettlementExpectation;
		HST_EnemyOrderState order = state.FindEnemyOrder(expected.m_sOrderId);
		HST_OperationRecordState operation
			= state.FindOperation(expected.m_sOperationId);
		HST_ForceManifestState manifest
			= state.FindForceManifest(expected.m_sManifestId);
		HST_FactionPoolState pool = state.FindFactionPool(expected.m_sFactionKey);
		HST_EnemyStrategicMutationState debit
			= FindExternalMutation(state, expected.m_sDebitMutationId);
		HST_EnemyStrategicMutationState refund
			= FindExternalMutation(state, expected.m_sRefundMutationId);
		HST_ZoneState sourceZone = state.FindZone(expected.m_sSourceZoneId);
		HST_ZoneState targetZone = state.FindZone(expected.m_sTargetZoneId);
		if (!order || !operation || !manifest || !pool || !debit || !refund
			|| !sourceZone || !targetZone)
			return "external-counterattack-terminal-settlement-incomplete";
		string fingerprint = string.Format(
			"schema/cut %1/%2 | order %3",
			state.m_iSchemaVersion,
			carrier.m_iCut,
			BuildExternalOrderSemanticRow(order));
		fingerprint += " | operation "
			+ BuildExternalOperationSemanticRow(operation, false);
		fingerprint += " | operation revision " + operation.m_iRevision.ToString();
		fingerprint += " | manifest " + BuildExternalManifestSemanticRow(manifest);
		fingerprint += " | pool " + BuildExternalPoolSemanticRow(pool);
		fingerprint += " | debit " + BuildExternalMutationSemanticRow(debit);
		fingerprint += " | refund " + BuildExternalMutationSemanticRow(refund);
		fingerprint += " | source zone "
			+ BuildExternalEndpointOwnershipSemanticRow(sourceZone);
		fingerprint += " | target zone "
			+ BuildExternalEndpointOwnershipSemanticRow(targetZone);
		fingerprint += " | ownership claimants "
			+ CountExternalOwnershipTransitionClaimants(
				state,
				expected.m_sOperationId).ToString();
		fingerprint += string.Format(
			" | rows %1/%2/%3/%4/%5/%6/%7 | execution %8/%9",
			CountEnemyOrderId(state, expected.m_sOrderId),
			CountOperationId(state, expected.m_sOperationId),
			CountManifestId(state, expected.m_sManifestId),
			CountBatchId(state, expected.m_sBatchId),
			CountGroupId(state, expected.m_sGroupId),
			CountMutationId(state, expected.m_sDebitMutationId),
			CountMutationId(state, expected.m_sRefundMutationId),
			CountExternalSettlementBatchClaimants(state, expected),
			CountExternalSettlementGroupClaimants(state, expected));
		fingerprint += " | foreign "
			+ CountExternalSettlementForeignClaimants(state, expected).ToString();
		fingerprint += " | mutation total "
			+ state.m_aEnemyStrategicMutations.Count().ToString();
		return fingerprint;
	}

	protected int CountExternalSettlementBatchClaimants(
		HST_CampaignState state,
		HST_EnemyCounterattackPreparedSettlementExpectation expected)
	{
		int count;
		if (!state || !expected)
			return count;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && (batch.m_sResultId == expected.m_sBatchId
				|| batch.m_sRequestId == expected.m_sOrderId
				|| batch.m_sOperationId == expected.m_sOperationId
				|| batch.m_sManifestId == expected.m_sManifestId
				|| batch.m_sProjectionId == expected.m_sProjectionId
				|| batch.m_sForceId == expected.m_sForceId))
				count++;
		}
		return count;
	}

	protected int CountExternalSettlementGroupClaimants(
		HST_CampaignState state,
		HST_EnemyCounterattackPreparedSettlementExpectation expected)
	{
		int count;
		if (!state || !expected)
			return count;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && (group.m_sGroupId == expected.m_sGroupId
				|| group.m_sEnemyOrderId == expected.m_sOrderId
				|| group.m_sOperationId == expected.m_sOperationId
				|| group.m_sManifestId == expected.m_sManifestId
				|| group.m_sSpawnResultId == expected.m_sBatchId
				|| group.m_sProjectionId == expected.m_sProjectionId
				|| group.m_sForceId == expected.m_sForceId))
				count++;
		}
		return count;
	}

	protected int CountExternalSettlementForeignClaimants(
		HST_CampaignState state,
		HST_EnemyCounterattackPreparedSettlementExpectation expected)
	{
		int count;
		if (!state || !expected)
			return count;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (request && (request.m_sRequestId == expected.m_sOrderId
				|| request.m_sOperationId == expected.m_sOperationId
				|| request.m_sManifestId == expected.m_sManifestId
				|| request.m_sSpawnResultId == expected.m_sBatchId
				|| request.m_sGroupId == expected.m_sGroupId))
				count++;
		}
		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (qrf && qrf.m_sGroupId == expected.m_sGroupId)
				count++;
		}
		return count;
	}

	bool ValidateExternalRuntimeClaimantsZero(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		HST_ForceSpawnAdapterService adapter,
		HST_PhysicalWarService physicalWar,
		out string evidence)
	{
		evidence = "external exact counterattack runtime claimant check rejected";
		if (!state || !carrier || !adapter || !physicalWar)
			return false;
		bool settlementCut
			= HST_EnemyCounterattackExternalRestartProofService
				.IsPreparedSettlementCut(carrier.m_sCutName);
		if (!settlementCut)
		{
			if (!carrier.m_Expectation || carrier.m_SettlementExpectation)
				return false;
			HST_EnemyCounterattackOutboundVirtualExpectation expected
				= carrier.m_Expectation;
			HST_ActiveGroupState group = state.FindActiveGroup(expected.m_sGroupId);
			if (!group)
				return false;
			int resultHandles
				= adapter.CountHandlesForResultId(expected.m_sBatchId);
			int projectionHandles
				= adapter.CountHandlesForProjection(expected.m_sProjectionId);
			int runtimeMembers = physicalWar.CountForceSpawnRuntimeMembers(group);
			bool exact = resultHandles == 0 && projectionHandles == 0
				&& runtimeMembers == 0
				&& !physicalWar.HasActiveGroupRuntimeHandle(group)
				&& !physicalWar.GetForceSpawnGroupRoot(group);
			if (!exact)
			{
				evidence = string.Format(
					"runtime claimant conflict | result %1 | projection %2 | members %3",
					resultHandles,
					projectionHandles,
					runtimeMembers);
				return false;
			}
			evidence = "external exact counterattack runtime claimants zero";
			return true;
		}
		if (carrier.m_Expectation || !carrier.m_SettlementExpectation)
			return false;

		HST_EnemyCounterattackPreparedSettlementExpectation settlement
			= carrier.m_SettlementExpectation;
		HST_ActiveGroupState durableGroup
			= state.FindActiveGroup(settlement.m_sGroupId);
		HST_ActiveGroupState runtimeProbe = durableGroup;
		if (!runtimeProbe)
		{
			runtimeProbe = new HST_ActiveGroupState();
			runtimeProbe.m_sGroupId = settlement.m_sGroupId;
		}
		int settlementResultHandles
			= adapter.CountHandlesForResultId(settlement.m_sBatchId);
		int settlementProjectionHandles
			= adapter.CountHandlesForProjection(settlement.m_sProjectionId);
		int settlementRuntimeMembers
			= physicalWar.CountForceSpawnRuntimeMembers(runtimeProbe);
		bool settlementExact = settlementResultHandles == 0
			&& settlementProjectionHandles == 0
			&& settlementRuntimeMembers == 0
			&& !physicalWar.HasActiveGroupRuntimeHandle(runtimeProbe)
			&& !physicalWar.GetForceSpawnGroupRoot(runtimeProbe);
		HST_OperationRecordState settlementOperation
			= state.FindOperation(settlement.m_sOperationId);
		if (!settlementOperation)
			return false;
		if (settlementOperation && settlementOperation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_PREPARED)
			settlementExact = settlementExact && durableGroup != null;
		else if (settlementOperation && settlementOperation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			settlementExact = settlementExact && durableGroup == null;
		else
			settlementExact = false;
		if (!settlementExact)
		{
			evidence = string.Format(
				"runtime claimant conflict | result %1 | projection %2 | members %3 | durable group %4",
				settlementResultHandles,
				settlementProjectionHandles,
				settlementRuntimeMembers,
				durableGroup != null);
			return false;
		}
		evidence = "external exact counterattack runtime claimants zero";
		return true;
	}

	string BuildExternalSemanticFingerprint(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier)
	{
		return BuildExternalCanonicalFingerprint(state, carrier, true);
	}

	string BuildExternalContinuationInvariantFingerprint(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier)
	{
		return BuildExternalCanonicalFingerprint(state, carrier, false);
	}

	string BuildExternalSemanticDifferenceEvidence(
		HST_CampaignState before,
		HST_CampaignState after,
		HST_EnemyCounterattackExternalRestartCarrier carrier)
	{
		if (!before || !after || !carrier || !carrier.m_Expectation)
			return "semantic row comparison unavailable";
		HST_EnemyCounterattackOutboundVirtualExpectation expected
			= carrier.m_Expectation;
		HST_EnemyOrderState beforeOrder
			= before.FindEnemyOrder(expected.m_sOrderId);
		HST_EnemyOrderState afterOrder
			= after.FindEnemyOrder(expected.m_sOrderId);
		HST_OperationRecordState beforeOperation
			= before.FindOperation(expected.m_sOperationId);
		HST_OperationRecordState afterOperation
			= after.FindOperation(expected.m_sOperationId);
		HST_ForceManifestState beforeManifest
			= before.FindForceManifest(expected.m_sManifestId);
		HST_ForceManifestState afterManifest
			= after.FindForceManifest(expected.m_sManifestId);
		HST_ForceSpawnResultState beforeBatch
			= before.FindForceSpawnResult(expected.m_sBatchId);
		HST_ForceSpawnResultState afterBatch
			= after.FindForceSpawnResult(expected.m_sBatchId);
		HST_ActiveGroupState beforeGroup
			= before.FindActiveGroup(expected.m_sGroupId);
		HST_ActiveGroupState afterGroup
			= after.FindActiveGroup(expected.m_sGroupId);
		HST_FactionPoolState beforePool
			= before.FindFactionPool(expected.m_sFactionKey);
		HST_FactionPoolState afterPool
			= after.FindFactionPool(expected.m_sFactionKey);
		HST_EnemyStrategicMutationState beforeMutation
			= FindExternalMutation(before, expected.m_sDebitMutationId);
		HST_EnemyStrategicMutationState afterMutation
			= FindExternalMutation(after, expected.m_sDebitMutationId);
		HST_ZoneState beforeSourceZone
			= before.FindZone(expected.m_sSourceZoneId);
		HST_ZoneState afterSourceZone
			= after.FindZone(expected.m_sSourceZoneId);
		HST_ZoneState beforeTargetZone
			= before.FindZone(expected.m_sTargetZoneId);
		HST_ZoneState afterTargetZone
			= after.FindZone(expected.m_sTargetZoneId);

		string evidence;
		if (BuildExternalGroupSemanticRow(beforeGroup, false)
			!= BuildExternalGroupSemanticRow(afterGroup, false))
		{
			evidence = BuildExternalGroupStaticDifferenceEvidence(
				beforeGroup,
				afterGroup) + " | ";
		}
		evidence += string.Format(
			"semantic row matches | elapsed %1 | order %2 | operation %3 | manifest %4",
			before.m_iElapsedSeconds == after.m_iElapsedSeconds,
			BuildExternalOrderSemanticRow(beforeOrder)
				== BuildExternalOrderSemanticRow(afterOrder),
			BuildExternalOperationSemanticRow(beforeOperation, true)
				== BuildExternalOperationSemanticRow(afterOperation, true),
			BuildExternalManifestSemanticRow(beforeManifest)
				== BuildExternalManifestSemanticRow(afterManifest));
		evidence += string.Format(
			" | batch %1 | slots %2 | group %3 | pool %4",
			BuildExternalBatchSemanticRow(beforeBatch)
				== BuildExternalBatchSemanticRow(afterBatch),
			BuildExternalSlotSemanticRows(beforeBatch)
				== BuildExternalSlotSemanticRows(afterBatch),
			BuildExternalGroupSemanticRow(beforeGroup, true)
				== BuildExternalGroupSemanticRow(afterGroup, true),
			BuildExternalPoolSemanticRow(beforePool)
				== BuildExternalPoolSemanticRow(afterPool));
		evidence += string.Format(
			" | mutation %1",
			BuildExternalMutationSemanticRow(beforeMutation)
				== BuildExternalMutationSemanticRow(afterMutation));
		evidence += string.Format(
			" | source zone %1 | target zone %2 | ownership claimants %3",
			BuildExternalEndpointOwnershipSemanticRow(beforeSourceZone)
				== BuildExternalEndpointOwnershipSemanticRow(afterSourceZone),
			BuildExternalEndpointOwnershipSemanticRow(beforeTargetZone)
				== BuildExternalEndpointOwnershipSemanticRow(afterTargetZone),
			CountExternalOwnershipTransitionClaimants(
				before,
				expected.m_sOperationId)
				== CountExternalOwnershipTransitionClaimants(
					after,
					expected.m_sOperationId));
		bool groupStaticExact = BuildExternalGroupSemanticRow(beforeGroup, false)
			== BuildExternalGroupSemanticRow(afterGroup, false);
		bool positionExact = false;
		bool sourceExact = false;
		bool lifecycleExact = false;
		if (beforeGroup && afterGroup)
		{
			positionExact = BuildExternalVectorFingerprint(beforeGroup.m_vPosition)
				== BuildExternalVectorFingerprint(afterGroup.m_vPosition);
			sourceExact = BuildExternalVectorFingerprint(beforeGroup.m_vSourcePosition)
				== BuildExternalVectorFingerprint(afterGroup.m_vSourcePosition);
			lifecycleExact = beforeGroup.m_iLifecycleRevision
				== afterGroup.m_iLifecycleRevision;
		}
		evidence += string.Format(
			" | group static %1 | position %2 | source %3 | lifecycle %4",
			groupStaticExact,
			positionExact,
			sourceExact,
			lifecycleExact);
		return evidence;
	}

	protected string BuildExternalGroupStaticDifferenceEvidence(
		HST_ActiveGroupState before,
		HST_ActiveGroupState after)
	{
		if (!before || !after)
			return "group static comparison unavailable";
		bool identityExact = true;
		identityExact = identityExact && before.m_sGroupId == after.m_sGroupId;
		identityExact = identityExact
			&& before.m_sOperationId == after.m_sOperationId;
		identityExact = identityExact
			&& before.m_sManifestId == after.m_sManifestId;
		identityExact = identityExact
			&& before.m_sSpawnResultId == after.m_sSpawnResultId;
		identityExact = identityExact && before.m_sForceId == after.m_sForceId;
		identityExact = identityExact
			&& before.m_sProjectionId == after.m_sProjectionId;
		identityExact = identityExact && before.m_sZoneId == after.m_sZoneId;
		identityExact = identityExact
			&& before.m_sFactionKey == after.m_sFactionKey;
		identityExact = identityExact
			&& before.m_sMissionInstanceId == after.m_sMissionInstanceId;

		bool claimantExact = true;
		claimantExact = claimantExact
			&& before.m_sSupportRequestId == after.m_sSupportRequestId;
		claimantExact = claimantExact
			&& before.m_sEnemyOrderId == after.m_sEnemyOrderId;
		claimantExact = claimantExact
			&& before.m_sConvoyElementId == after.m_sConvoyElementId;
		claimantExact = claimantExact
			&& before.m_sMissionAssetId == after.m_sMissionAssetId;
		claimantExact = claimantExact
			&& before.m_sGarrisonZoneId == after.m_sGarrisonZoneId;
		claimantExact = claimantExact
			&& before.m_sQRFInstanceId == after.m_sQRFInstanceId;
		claimantExact = claimantExact && before.m_sLocalSecurityPatrolId
			== after.m_sLocalSecurityPatrolId;
		claimantExact = claimantExact && before.m_sPrefab == after.m_sPrefab;
		claimantExact = claimantExact
			&& before.m_sVehiclePrefab == after.m_sVehiclePrefab;

		bool compositionExact = true;
		compositionExact = compositionExact && before.m_sCompositionRequestId
			== after.m_sCompositionRequestId;
		compositionExact = compositionExact && before.m_sCompositionIntentId
			== after.m_sCompositionIntentId;
		compositionExact = compositionExact
			&& before.m_sCompositionTier == after.m_sCompositionTier;
		compositionExact = compositionExact
			&& before.m_sCompositionSummary == after.m_sCompositionSummary;
		compositionExact = compositionExact
			&& before.m_sSpawnFallbackMode == after.m_sSpawnFallbackMode;
		compositionExact = compositionExact
			&& before.m_sSpawnFailureReason == after.m_sSpawnFailureReason;
		compositionExact = compositionExact
			&& before.m_sRouteId == after.m_sRouteId;
		compositionExact = compositionExact
			&& BuildExternalVectorFingerprint(before.m_vTargetPosition)
				== BuildExternalVectorFingerprint(after.m_vTargetPosition);
		compositionExact = compositionExact
			&& before.m_iInfantryCount == after.m_iInfantryCount;

		bool rosterExact = true;
		rosterExact = rosterExact
			&& before.m_iVehicleCount == after.m_iVehicleCount;
		rosterExact = rosterExact && before.m_iOriginalInfantryCount
			== after.m_iOriginalInfantryCount;
		rosterExact = rosterExact && before.m_iOriginalVehicleCount
			== after.m_iOriginalVehicleCount;
		rosterExact = rosterExact
			&& before.m_iCompositionCost == after.m_iCompositionCost;
		rosterExact = rosterExact && before.m_iCompositionManpower
			== after.m_iCompositionManpower;
		rosterExact = rosterExact && before.m_iCompositionVehicleCount
			== after.m_iCompositionVehicleCount;
		rosterExact = rosterExact && before.m_iCompositionArmedVehicleCount
			== after.m_iCompositionArmedVehicleCount;
		rosterExact = rosterExact
			&& before.m_iSpawnedAtSecond == after.m_iSpawnedAtSecond;
		rosterExact = rosterExact
			&& before.m_iLastSeenAliveCount == after.m_iLastSeenAliveCount;
		rosterExact = rosterExact && before.m_iSurvivorInfantryCount
			== after.m_iSurvivorInfantryCount;
		rosterExact = rosterExact && before.m_iSurvivorVehicleCount
			== after.m_iSurvivorVehicleCount;
		rosterExact = rosterExact && before.m_iAssignedWaypointCount
			== after.m_iAssignedWaypointCount;
		rosterExact = rosterExact && before.m_iMaxObservedCrewAlive
			== after.m_iMaxObservedCrewAlive;
		rosterExact = rosterExact && before.m_iDurableLivingInfantryCount
			== after.m_iDurableLivingInfantryCount;

		bool combatExact = true;
		combatExact = combatExact && before.m_iCombatEffectiveInfantryCount
			== after.m_iCombatEffectiveInfantryCount;
		combatExact = combatExact && before.m_iOperationalMannedVehicleCount
			== after.m_iOperationalMannedVehicleCount;
		combatExact = combatExact
			&& before.m_iCombatEffectiveStaticOperatorCount
				== after.m_iCombatEffectiveStaticOperatorCount;
		combatExact = combatExact && before.m_iCombatPresenceSampleSecond
			== after.m_iCombatPresenceSampleSecond;
		combatExact = combatExact
			&& before.m_iLastCasualtySecond == after.m_iLastCasualtySecond;
		combatExact = combatExact
			&& before.m_iEliminatedAtSecond == after.m_iEliminatedAtSecond;
		combatExact = combatExact
			&& before.m_bEverHadLivingCrew == after.m_bEverHadLivingCrew;
		combatExact = combatExact && before.m_bCombatPresenceSampleAuthoritative
			== after.m_bCombatPresenceSampleAuthoritative;
		bool combatReasonExact;
		if (!before.m_bCombatPresenceSampleAuthoritative
			&& !after.m_bCombatPresenceSampleAuthoritative)
		{
			combatReasonExact = true;
		}
		else
		{
			combatReasonExact = BuildExternalCanonicalOptionalText(
				before.m_sCombatPresenceSampleReason)
				== BuildExternalCanonicalOptionalText(
					after.m_sCombatPresenceSampleReason);
		}
		combatExact = combatExact && combatReasonExact;
		combatExact = combatExact
			&& before.m_bEverPopulated == after.m_bEverPopulated;
		combatExact = combatExact
			&& before.m_bSpawnCompleted == after.m_bSpawnCompleted;
		combatExact = combatExact && before.m_bCrewPopulationTerminallyFailed
			== after.m_bCrewPopulationTerminallyFailed;
		combatExact = combatExact
			&& BuildExternalCanonicalOptionalText(
				before.m_sCrewPopulationFailureReason)
				== BuildExternalCanonicalOptionalText(
					after.m_sCrewPopulationFailureReason);

		bool runtimeExact = true;
		runtimeExact = runtimeExact
			&& before.m_sConvoyRuntimeStage == after.m_sConvoyRuntimeStage;
		runtimeExact = runtimeExact && before.m_bQRF == after.m_bQRF;
		runtimeExact = runtimeExact
			&& before.m_bSpawnAttempted == after.m_bSpawnAttempted;
		runtimeExact = runtimeExact
			&& before.m_bSpawnedEntity == after.m_bSpawnedEntity;
		runtimeExact = runtimeExact && before.m_sRuntimeEntityId.IsEmpty()
			== after.m_sRuntimeEntityId.IsEmpty();
		runtimeExact = runtimeExact && before.m_iSpawnedAgentCount
			== after.m_iSpawnedAgentCount;
		string evidence;
		if (!combatExact)
		{
			evidence = string.Format(
				"combat fields i%1 v%2 s%3 t%4",
				before.m_iCombatEffectiveInfantryCount
					== after.m_iCombatEffectiveInfantryCount,
				before.m_iOperationalMannedVehicleCount
					== after.m_iOperationalMannedVehicleCount,
				before.m_iCombatEffectiveStaticOperatorCount
					== after.m_iCombatEffectiveStaticOperatorCount,
				before.m_iCombatPresenceSampleSecond
					== after.m_iCombatPresenceSampleSecond);
			evidence += string.Format(
				" c%1 e%2 h%3 a%4",
				before.m_iLastCasualtySecond == after.m_iLastCasualtySecond,
				before.m_iEliminatedAtSecond == after.m_iEliminatedAtSecond,
				before.m_bEverHadLivingCrew == after.m_bEverHadLivingCrew,
				before.m_bCombatPresenceSampleAuthoritative
					== after.m_bCombatPresenceSampleAuthoritative);
			evidence += string.Format(
				" r%1 p%2 d%3 f%4",
				combatReasonExact,
				before.m_bEverPopulated == after.m_bEverPopulated,
				before.m_bSpawnCompleted == after.m_bSpawnCompleted,
				before.m_bCrewPopulationTerminallyFailed
					== after.m_bCrewPopulationTerminallyFailed);
			evidence += string.Format(
				" x%1 | ",
				BuildExternalCanonicalOptionalText(
					before.m_sCrewPopulationFailureReason)
					== BuildExternalCanonicalOptionalText(
						after.m_sCrewPopulationFailureReason));
		}
		evidence += string.Format(
			"group fields | identity %1 | claimant %2 | composition %3 | roster %4",
			identityExact,
			claimantExact,
			compositionExact,
			rosterExact)
			+ string.Format(
				" | combat %1 | runtime %2",
				combatExact,
				runtimeExact);
		return evidence;
	}

	protected string BuildExternalCanonicalFingerprint(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		bool includeMovementCursor)
	{
		if (!state || !carrier || !carrier.m_Expectation)
			return "external-counterattack-semantic-unavailable";
		HST_EnemyCounterattackOutboundVirtualExpectation expected
			= carrier.m_Expectation;
		HST_EnemyOrderState order = state.FindEnemyOrder(expected.m_sOrderId);
		HST_OperationRecordState operation
			= state.FindOperation(expected.m_sOperationId);
		HST_ForceManifestState manifest
			= state.FindForceManifest(expected.m_sManifestId);
		HST_ForceSpawnResultState batch
			= state.FindForceSpawnResult(expected.m_sBatchId);
		HST_ActiveGroupState group = state.FindActiveGroup(expected.m_sGroupId);
		HST_FactionPoolState pool = state.FindFactionPool(expected.m_sFactionKey);
		HST_EnemyStrategicMutationState mutation
			= FindExternalMutation(state, expected.m_sDebitMutationId);
		HST_ZoneState sourceZone = state.FindZone(expected.m_sSourceZoneId);
		HST_ZoneState targetZone = state.FindZone(expected.m_sTargetZoneId);
		if (!order || !operation || !manifest || !batch || !group || !pool
			|| !mutation || !sourceZone || !targetZone)
			return "external-counterattack-semantic-unavailable";

		string fingerprint = string.Format(
			"schema %1 | movement %2",
			state.m_iSchemaVersion,
			includeMovementCursor);
		if (includeMovementCursor)
			fingerprint += " | elapsed " + state.m_iElapsedSeconds.ToString();
		fingerprint += " | order " + BuildExternalOrderSemanticRow(order);
		fingerprint += " | operation "
			+ BuildExternalOperationSemanticRow(operation, includeMovementCursor);
		fingerprint += " | manifest " + BuildExternalManifestSemanticRow(manifest);
		fingerprint += " | batch " + BuildExternalBatchSemanticRow(batch);
		fingerprint += " | slots " + BuildExternalSlotSemanticRows(batch);
		fingerprint += " | group "
			+ BuildExternalGroupSemanticRow(group, includeMovementCursor);
		fingerprint += " | pool " + BuildExternalPoolSemanticRow(pool);
		fingerprint += " | mutation " + BuildExternalMutationSemanticRow(mutation);
		fingerprint += " | source zone "
			+ BuildExternalEndpointOwnershipSemanticRow(sourceZone);
		fingerprint += " | target zone "
			+ BuildExternalEndpointOwnershipSemanticRow(targetZone);
		fingerprint += " | ownership claimants "
			+ CountExternalOwnershipTransitionClaimants(
				state,
				expected.m_sOperationId).ToString();
		fingerprint += string.Format(
			" | rows %1/%2/%3/%4/%5/%6 | foreign %7/%8",
			CountEnemyOrderId(state, expected.m_sOrderId),
			CountOperationId(state, expected.m_sOperationId),
			CountManifestId(state, expected.m_sManifestId),
			CountBatchId(state, expected.m_sBatchId),
			CountGroupId(state, expected.m_sGroupId),
			CountMutationId(state, expected.m_sDebitMutationId),
			CountExternalSupportClaimants(state, expected),
			CountExternalLegacyQRFClaimants(state, expected));
		return fingerprint;
	}

	protected string BuildExternalOrderSemanticRow(HST_EnemyOrderState order)
	{
		if (!order)
			return "null";
		string row = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			order.m_sOrderId,
			order.m_sOperationId,
			order.m_iOperationContractVersion,
			order.m_sManifestId,
			order.m_sManifestHash,
			order.m_sSpawnResultId,
			order.m_iPlanningContractVersion,
			order.m_iPlanningDecisionSequence,
			order.m_iPlanningBucketSecond);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			order.m_sPlanningDecisionId,
			order.m_sPlanningInputFingerprint,
			order.m_sPlanningDecisionFingerprint,
			order.m_ePlannedSupportType,
			order.m_sPlanningCapabilityHash,
			order.m_sFactionKey,
			order.m_eType,
			order.m_eStatus,
			order.m_sSourceZoneId);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			order.m_sTargetZoneId,
			order.m_iTargetOwnershipRevision,
			order.m_sCompositionRequestId,
			order.m_sCompositionIntentId,
			order.m_sCompositionTier,
			order.m_sCompositionSummary,
			order.m_sCompositionFailureReason,
			order.m_sSupportRequestId,
			order.m_sGroupId);
		// Runtime status is deliberately canonicalized: restore is allowed to
		// relabel the same virtual authority, while resolution/failure are not.
		row += string.Format(
			"|virtual_runtime|%1|%2|%3|%4|%5|%6|%7|%8",
			order.m_sResolutionKind,
			order.m_sFailureReason,
			BuildExternalVectorFingerprint(order.m_vSourcePosition),
			BuildExternalVectorFingerprint(order.m_vTargetPosition),
			order.m_iCreatedAtSecond,
			order.m_iResolveAtSecond,
			order.m_iPhysicalizedAtSecond,
			order.m_iResolvedAtSecond);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			order.m_iAttackCost,
			order.m_iSupportCost,
			order.m_iRefundedAttackResources,
			order.m_iRefundedSupportResources,
			order.m_iCompositionCost,
			order.m_iCompositionManpower,
			order.m_iCompositionVehicleCount,
			order.m_iCompositionArmedVehicleCount,
			order.m_sResourceDebitMutationId);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			order.m_sResourceRefundMutationId,
			order.m_sResourceSettlementId,
			order.m_sResourceSettlementKind,
			order.m_iSettlementAcceptedMemberCount,
			order.m_iSettlementSurvivorMemberCount,
			order.m_bStrategicServiceCommitted,
			order.m_bResourceSettlementApplied,
			order.m_bPhysicalized,
			order.m_bAbstractResolved);
		return row + string.Format(
			"|%1|%2",
			order.m_bOutcomeApplied,
			order.m_bResourceRefundApplied);
	}

	protected string BuildExternalOperationSemanticRow(
		HST_OperationRecordState operation,
		bool includeMovementCursor)
	{
		if (!operation)
			return "null";
		string row = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			operation.m_sOperationId,
			operation.m_eType,
			operation.m_iContractVersion,
			operation.m_sOwnerFactionKey,
			operation.m_sActorIdentityId,
			operation.m_sIssueRequestId,
			operation.m_sConfirmationRequestId,
			operation.m_sSupportRequestId,
			operation.m_sEnemyOrderId);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			operation.m_sMissionInstanceId,
			operation.m_sLocalSecurityPatrolId,
			operation.m_sQuoteId,
			operation.m_sManifestId,
			operation.m_sSpawnResultId,
			operation.m_sForceId,
			operation.m_sProjectionId,
			operation.m_sGroupId,
			operation.m_sOriginZoneId);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			BuildExternalVectorFingerprint(operation.m_vOriginPosition),
			operation.m_sAssignmentKind,
			operation.m_sAssignmentZoneId,
			BuildExternalVectorFingerprint(operation.m_vAssignmentPosition),
			operation.m_sTacticalTargetZoneId,
			BuildExternalVectorFingerprint(operation.m_vTacticalTargetPosition),
			operation.m_sCurrentRouteId,
			operation.m_sRouteContractHash,
			operation.m_iProjectionContractVersion);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			operation.m_iRouteVersion,
			operation.m_iRouteWaypointIndex,
			operation.m_iRouteLapCount,
			operation.m_iRouteLegSequence,
			operation.m_iRouteLoopStartedAtSecond,
			operation.m_iRouteLoopCompletedAtSecond,
			BuildExternalVectorFingerprint(operation.m_vRouteStartPosition),
			BuildExternalVectorFingerprint(operation.m_vRouteEndPosition),
			Math.Round(operation.m_fRouteTotalDistanceMeters * 100.0));
		row += "|speed="
			+ Math.Round(operation.m_fStrategicSpeedMetersPerSecond * 100.0).ToString();
		row += string.Format(
			"|projection=%1|%2",
			operation.m_iLastProjectionDecisionSecond,
			operation.m_iLastNormalizedRestoreSequence);
		if (includeMovementCursor)
		{
			row += string.Format(
				"|cursor=%1|%2|%3|%4",
				BuildExternalVectorFingerprint(operation.m_vStrategicPosition),
				Math.Round(operation.m_fRouteProgressMeters * 100.0),
				operation.m_iStrategicLastUpdateSecond,
				operation.m_iLastProgressAtSecond);
		}
		row += string.Format(
			"|combat=%1|%2|%3|%4|%5|%6|%7|%8",
			operation.m_iVirtualCombatStepIndex,
			operation.m_iVirtualCombatFriendlyDamageCarry,
			operation.m_iVirtualCombatHostileDamageCarry,
			operation.m_iLastVirtualFriendlyCount,
			operation.m_iLastVirtualHostileCount,
			operation.m_iArrivalConfirmationCount,
			operation.m_iLastArrivalConfirmationSecond,
			operation.m_sLastVirtualCombatReason);
		row += string.Format(
			"|policy=%1|%2|%3|%4|%5|%6|%7|%8|%9",
			operation.m_sRecallPolicyId,
			operation.m_sSettlementPolicyId,
			operation.m_eDutyState,
			operation.m_eResumeDutyState,
			operation.m_eEngagementMode,
			operation.m_eMaterializationState,
			operation.m_ePositionAuthority,
			operation.m_eSettlementState,
			operation.m_eTerminalResult);
		// Restore-only normalization clocks/reasons/revisions are intentionally
		// excluded; the strategic movement cursor above is never excluded.
		return row + string.Format(
			"|terminal=%1|%2|%3|%4|%5|%6|%7|%8",
			operation.m_sSettlementId,
			operation.m_sTerminalReason,
			operation.m_iDeterministicSeed,
			operation.m_iCreatedAtSecond,
			operation.m_iDutyStateEnteredAtSecond,
			operation.m_iEngagementStateEnteredAtSecond,
			operation.m_iLastContactAtSecond,
			operation.m_iSettledAtSecond);
	}

	protected string BuildExternalManifestSemanticRow(HST_ForceManifestState manifest)
	{
		if (!manifest)
			return "null";
		string row = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			manifest.m_sManifestId,
			manifest.m_sManifestHash,
			manifest.m_sOperationId,
			manifest.m_sQuoteId,
			manifest.m_sCommandRequestId,
			manifest.m_sForceKind,
			manifest.m_sFactionRole,
			manifest.m_sFactionKey,
			manifest.m_sIntentId);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			manifest.m_sSourceZoneId,
			manifest.m_sTargetZoneId,
			manifest.m_sGroupPrefab,
			manifest.m_sCatalogVersion,
			manifest.m_sPolicyId,
			manifest.m_iRequestedMemberCount,
			manifest.m_iAcceptedMemberCount,
			manifest.m_iRequestedVehicleCount,
			manifest.m_iAcceptedVehicleCount);
		return row + string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			manifest.m_iMoneyCost,
			manifest.m_iHRCost,
			manifest.m_iEquipmentCost,
			manifest.m_iAttackResourceCost,
			manifest.m_iSupportResourceCost,
			manifest.m_iDeterministicSeed,
			manifest.m_iCreatedAtSecond,
			manifest.m_bFrozen,
			manifest.m_aGroups.Count() + manifest.m_aMembers.Count()
				+ manifest.m_aVehicles.Count() + manifest.m_aAssets.Count());
	}

	protected string BuildExternalBatchSemanticRow(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return "null";
		string row = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			batch.m_sResultId,
			batch.m_sRequestId,
			batch.m_sManifestId,
			batch.m_sManifestHash,
			batch.m_sOperationId,
			batch.m_sForceId,
			batch.m_sProjectionId,
			batch.m_sTerminalReason,
			batch.m_sLastFailureReason);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			batch.m_eStatus,
			batch.m_iPriority,
			batch.m_iRetryCount,
			batch.m_iMaxRetries,
			batch.m_iDeadlineSecond,
			batch.m_iCreatedAtSecond,
			batch.m_iLastAttemptSecond,
			batch.m_iNextAttemptSecond,
			batch.m_iCompletedAtSecond);
		// Attempt generation, hold/update/lifecycle clocks are restore-normalized.
		return row + string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8",
			batch.m_iExpectedSlotCount,
			batch.m_iSuccessfulHandoffCount,
			batch.m_iReprojectionCount,
			batch.m_bCancelRequested,
			batch.m_bStrategicProjectionHeld,
			batch.m_bExternalAssetAuthority,
			batch.m_aSlotResults.Count(),
			batch.m_sNativeGroupId.IsEmpty());
	}

	protected string BuildExternalSlotSemanticRows(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return "null";
		array<string> rows = {};
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
			rows.Insert(BuildExternalSlotSemanticRow(slot));
		rows.Sort();
		string fingerprint;
		foreach (string row : rows)
		{
			if (!fingerprint.IsEmpty())
				fingerprint += ";";
			fingerprint += row;
		}
		return fingerprint;
	}

	protected string BuildExternalSlotSemanticRow(HST_ForceSpawnSlotResultState slot)
	{
		if (!slot)
			return "null";
		string row = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			slot.m_sSlotId,
			slot.m_sSlotKind,
			slot.m_sSpawnedPrefab,
			slot.m_sEntityId,
			slot.m_sAssignedVehicleEntityId,
			slot.m_sNativeGroupId,
			slot.m_sProjectionId,
			slot.m_sFailureReason,
			slot.m_eStatus);
		// Updated-at is restore-normalized. Lifecycle revision is durable for this
		// counterattack path and remains part of the canonical slot tuple.
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			slot.m_iAttemptCount,
			slot.m_iCasualtyAtSecond,
			slot.m_sRetirementReason,
			slot.m_bFactionVerified,
			slot.m_bGroupVerified,
			slot.m_bGameMasterVerified,
			slot.m_bProjectionVerified,
			slot.m_bSeatVerified,
			slot.m_bAliveVerified);
		return row + string.Format(
			"|%1|%2|%3",
			slot.m_bEverAlive,
			slot.m_bCasualtyConfirmed,
			slot.m_iLifecycleRevision);
	}

	protected string BuildExternalGroupSemanticRow(
		HST_ActiveGroupState group,
		bool includeMovementCursor)
	{
		if (!group)
			return "null";
		string row = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			group.m_sGroupId,
			group.m_sOperationId,
			group.m_sManifestId,
			group.m_sSpawnResultId,
			group.m_sForceId,
			group.m_sProjectionId,
			group.m_sZoneId,
			group.m_sFactionKey,
			group.m_sMissionInstanceId);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			group.m_sSupportRequestId,
			group.m_sEnemyOrderId,
			group.m_sConvoyElementId,
			group.m_sMissionAssetId,
			group.m_sGarrisonZoneId,
			group.m_sQRFInstanceId,
			group.m_sLocalSecurityPatrolId,
			group.m_sPrefab,
			group.m_sVehiclePrefab);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			group.m_sCompositionRequestId,
			group.m_sCompositionIntentId,
			group.m_sCompositionTier,
			group.m_sCompositionSummary,
			group.m_sSpawnFallbackMode,
			group.m_sSpawnFailureReason,
			group.m_sRouteId,
			BuildExternalVectorFingerprint(group.m_vTargetPosition),
			group.m_iInfantryCount);
		if (includeMovementCursor)
		{
			row += "|cursor=" + BuildExternalVectorFingerprint(group.m_vPosition)
				+ "/" + BuildExternalVectorFingerprint(group.m_vSourcePosition);
		}
		row += string.Format(
			"|roster=%1|%2|%3|%4|%5|%6|%7|%8|%9",
			group.m_iVehicleCount,
			group.m_iOriginalInfantryCount,
			group.m_iOriginalVehicleCount,
			group.m_iCompositionCost,
			group.m_iCompositionManpower,
			group.m_iCompositionVehicleCount,
			group.m_iCompositionArmedVehicleCount,
			group.m_iSpawnedAtSecond,
			group.m_iLastSeenAliveCount);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			group.m_iSurvivorInfantryCount,
			group.m_iSurvivorVehicleCount,
			group.m_iAssignedWaypointCount,
			group.m_iMaxObservedCrewAlive,
			group.m_iDurableLivingInfantryCount,
			group.m_iCombatEffectiveInfantryCount,
			group.m_iOperationalMannedVehicleCount,
			group.m_iCombatEffectiveStaticOperatorCount,
			group.m_iCombatPresenceSampleSecond);
		string combatSampleReason = "non_authoritative_combat_sample";
		if (group.m_bCombatPresenceSampleAuthoritative)
		{
			combatSampleReason = BuildExternalCanonicalOptionalText(
				group.m_sCombatPresenceSampleReason);
		}
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			group.m_iLastCasualtySecond,
			group.m_iEliminatedAtSecond,
			group.m_bEverHadLivingCrew,
			group.m_bCombatPresenceSampleAuthoritative,
			combatSampleReason,
			group.m_bEverPopulated,
			group.m_bSpawnCompleted,
			group.m_bCrewPopulationTerminallyFailed,
			BuildExternalCanonicalOptionalText(
				group.m_sCrewPopulationFailureReason));
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|virtual_runtime",
			group.m_sConvoyRuntimeStage,
			group.m_bQRF,
			group.m_bSpawnAttempted,
			group.m_bSpawnedEntity,
			group.m_sRuntimeEntityId.IsEmpty(),
			group.m_iSpawnedAgentCount);
		if (includeMovementCursor)
			row += "|lifecycle=" + group.m_iLifecycleRevision.ToString();
		return row;
	}

	protected string BuildExternalPoolSemanticRow(HST_FactionPoolState pool)
	{
		if (!pool)
			return "null";
		return string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			pool.m_sFactionKey,
			pool.m_iAttackResources,
			pool.m_iSupportResources,
			pool.m_iMoney,
			pool.m_iHR,
			pool.m_iAggression,
			pool.m_iStrategicContractVersion,
			pool.m_iStrategicOperationalMutationCount,
			pool.m_sLastStrategicMutationId)
			+ string.Format(
				"|%1|%2|%3|%4|%5|%6",
				pool.m_iResourceAccumulatorSeconds,
				pool.m_iAggressionAccumulatorSeconds,
				pool.m_iLastResourceBucketSecond,
				pool.m_iLastAggressionBucketSecond,
				pool.m_iStrategicRevision,
				pool.m_sStrategicAuthorityFailure);
	}

	protected string BuildExternalMutationSemanticRow(
		HST_EnemyStrategicMutationState mutation)
	{
		if (!mutation)
			return "null";
		string row = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			mutation.m_iContractVersion,
			mutation.m_sMutationId,
			mutation.m_sFactionKey,
			mutation.m_sKind,
			mutation.m_sSourceId,
			mutation.m_sOrderId,
			mutation.m_sOperationId,
			mutation.m_sManifestId,
			mutation.m_sZoneId);
		row += string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			mutation.m_iCreatedAtSecond,
			mutation.m_iPoolRevisionBefore,
			mutation.m_iPoolRevisionAfter,
			mutation.m_iOperationalSequence,
			mutation.m_iAttackBefore,
			mutation.m_iAttackDelta,
			mutation.m_iAttackAfter,
			mutation.m_iSupportBefore,
			mutation.m_iSupportDelta);
		return row + string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8",
			mutation.m_iSupportAfter,
			mutation.m_iAggressionBefore,
			mutation.m_iAggressionDelta,
			mutation.m_iAggressionAfter,
			mutation.m_sContributionHash,
			mutation.m_sFingerprint,
			mutation.m_bApplied,
			mutation.m_sSourceId.IsEmpty());
	}

	protected HST_EnemyStrategicMutationState FindExternalMutation(
		HST_CampaignState state,
		string mutationId)
	{
		if (!state || mutationId.IsEmpty())
			return null;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_sMutationId == mutationId)
				return mutation;
		}
		return null;
	}

	protected string BuildExternalCanonicalOptionalText(string value)
	{
		if (value.IsEmpty())
			return "<empty>";
		return value;
	}

	protected string BuildExternalVectorFingerprint(vector value)
	{
		return string.Format(
			"%1,%2,%3",
			Math.Round(value[0] * 100.0),
			Math.Round(value[1] * 100.0),
			Math.Round(value[2] * 100.0));
	}

	bool ValidateExternalDeterministicContinuation(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		int elapsedBefore,
		int strategicLastUpdateBefore,
		int operationRevisionBefore,
		int groupLifecycleRevisionBefore,
		float progressBefore,
		string invariantBefore,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "external-counterattack-semantic-unavailable";
		evidence = "external exact counterattack continuation rejected";
		if (!state || !carrier || !carrier.m_Expectation
			|| invariantBefore.IsEmpty()
			|| invariantBefore == "external-counterattack-semantic-unavailable")
			return false;
		HST_EnemyCounterattackOutboundVirtualExpectation expected
			= carrier.m_Expectation;
		HST_OperationRecordState operation
			= state.FindOperation(expected.m_sOperationId);
		HST_ActiveGroupState group = state.FindActiveGroup(expected.m_sGroupId);
		if (!operation || !group)
			return false;

		int expectedElapsed = elapsedBefore
			+ HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
		int elapsedAvailable = Math.Max(
			0,
			expectedElapsed - strategicLastUpdateBefore);
		int expectedAdvancedSeconds = Math.Min(
			HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK,
			elapsedAvailable);
		if (expectedAdvancedSeconds
			!= HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK)
			return false;
		float expectedAdvancedMeters
			= HST_StrategicMovementService.EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND
				* expectedAdvancedSeconds;
		float expectedProgress = progressBefore + expectedAdvancedMeters;
		if (expectedProgress >= operation.m_fRouteTotalDistanceMeters
			- HST_StrategicMovementService.ARRIVAL_EPSILON_METERS)
			return false;
		float fraction = expectedProgress / operation.m_fRouteTotalDistanceMeters;
		vector expectedPosition = operation.m_vRouteStartPosition
			+ (operation.m_vRouteEndPosition - operation.m_vRouteStartPosition)
				* fraction;
		string invariantAfter
			= BuildExternalContinuationInvariantFingerprint(state, carrier);
		bool clockExact = state.m_iElapsedSeconds == expectedElapsed
			&& operation.m_iStrategicLastUpdateSecond
				== strategicLastUpdateBefore + expectedAdvancedSeconds
			&& operation.m_iLastProgressAtSecond
				== operation.m_iStrategicLastUpdateSecond;
		bool positionExact
			= Math.AbsFloat(operation.m_fRouteProgressMeters - expectedProgress) <= 0.01
			&& VectorExact(operation.m_vStrategicPosition, expectedPosition, 0.01)
			&& VectorExact(group.m_vPosition, expectedPosition, 0.01)
			&& VectorExact(group.m_vSourcePosition, expectedPosition, 0.01);
		bool revisionExact = operation.m_iRevision == operationRevisionBefore + 1
			&& group.m_iLifecycleRevision == groupLifecycleRevisionBefore + 1
			&& invariantAfter == invariantBefore;
		bool exact = clockExact && positionExact && revisionExact;
		fingerprint = BuildExternalSemanticFingerprint(state, carrier);
		if (!exact
			|| fingerprint == "external-counterattack-semantic-unavailable")
			return false;
		evidence = string.Format(
			"external exact counterattack advanced one deterministic %1-second/%2-meter step to %3 meters",
			expectedAdvancedSeconds,
			Math.Round(expectedAdvancedMeters),
			Math.Round(expectedProgress));
		return true;
	}

	protected bool ValidateExternalQueuedSlots(
		HST_ForceSpawnResultState batch,
		HST_ForceManifestState manifest,
		HST_EnemyCounterattackOutboundVirtualExpectation expected,
		HST_EnemyCounterattackExternalVirtualCutPolicy policy = null)
	{
		if (!batch || !manifest || !expected
			|| manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
			return false;
		array<string> seen = {};
		int rootCount;
		int memberCount;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot)
				return false;
			bool confirmedCasualty = !expected.m_sConfirmedCasualtySlotId.IsEmpty()
				&& slot.m_sSlotId == expected.m_sConfirmedCasualtySlotId;
			HST_EForceSpawnSlotStatus expectedStatus
				= HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED;
			if (confirmedCasualty)
				expectedStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
			bool identityExact = !slot.m_sSlotId.IsEmpty()
				&& !seen.Contains(slot.m_sSlotId)
				&& slot.m_sProjectionId == expected.m_sProjectionId
				&& slot.m_eStatus == expectedStatus;
			bool lifecycleExact = slot.m_iAttemptCount
					== expected.m_iExpectedNormalizedSlotAttemptCount
				&& slot.m_sFailureReason.IsEmpty();
			if (confirmedCasualty)
			{
				lifecycleExact = lifecycleExact
					&& slot.m_iCasualtyAtSecond > 0
					&& !slot.m_sRetirementReason.IsEmpty();
			}
			else
			{
				lifecycleExact = lifecycleExact
					&& slot.m_iCasualtyAtSecond == 0
					&& slot.m_sRetirementReason.IsEmpty();
			}
			bool physicalExact = slot.m_sSpawnedPrefab.IsEmpty()
				&& slot.m_sEntityId.IsEmpty()
				&& slot.m_sAssignedVehicleEntityId.IsEmpty()
				&& slot.m_sNativeGroupId.IsEmpty();
			bool verificationExact = !slot.m_bFactionVerified
				&& !slot.m_bGroupVerified
				&& !slot.m_bGameMasterVerified
				&& !slot.m_bProjectionVerified
				&& !slot.m_bSeatVerified;
			bool expectedEverAlive = expected.m_bExpectedLivingSlotsEverAlive
				&& slot.m_sSlotKind
					== HST_ForceSpawnQueueService.SLOT_KIND_MEMBER;
			if (policy && policy.m_bExpectedRootEverAlive
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP)
				expectedEverAlive = true;
			if (!expected.m_sConfirmedCasualtySlotId.IsEmpty()
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
				expectedEverAlive = true;
			bool casualtyExact = !slot.m_bAliveVerified
				&& slot.m_bEverAlive == expectedEverAlive
				&& slot.m_bCasualtyConfirmed == confirmedCasualty;
			if (!identityExact || !lifecycleExact || !physicalExact
				|| !verificationExact || !casualtyExact)
				return false;
			seen.Insert(slot.m_sSlotId);
			if (slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP)
			{
				if (slot.m_sSlotId != manifest.m_aGroups[0].m_sElementId)
					return false;
				rootCount++;
			}
			else if (slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
			{
				if (!manifest.FindMemberSlot(slot.m_sSlotId))
					return false;
				memberCount++;
			}
			else
				return false;
		}
		return rootCount == 1
			&& memberCount == manifest.m_iAcceptedMemberCount
			&& seen.Count() == batch.m_iExpectedSlotCount;
	}

	bool ValidateExternalCasualtyContinuity(
		HST_CampaignState state,
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string evidence)
	{
		evidence = "external exact counterattack casualty continuity rejected";
		string carrierEvidence;
		if (!state || !carrier
			|| !HST_EnemyCounterattackExternalRestartProofService.ValidateCarrier(
				carrier,
				carrier.m_sSessionNonce,
				carrier.m_sRunId,
				carrier.m_sCutName,
				carrier.m_sWorld,
				carrierEvidence))
			return false;
		HST_EnemyCounterattackOutboundVirtualExpectation expected
			= carrier.m_Expectation;
		HST_ForceSpawnResultState batch
			= state.FindForceSpawnResult(expected.m_sBatchId);
		HST_ActiveGroupState group = state.FindActiveGroup(expected.m_sGroupId);
		if (!batch || !group)
			return false;
		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		int living = queue.CountStrategicLivingMemberSlots(batch);
		int casualties = queue.CountConfirmedCasualtyMemberSlots(batch);
		bool rosterExact = living == expected.m_iLivingMemberCount
			&& BuildExternalLivingSlotFingerprint(batch)
				== expected.m_sLivingSlotFingerprint
			&& batch.m_iReprojectionCount
				== expected.m_iExpectedNormalizedReprojectionCount
			&& group.m_iInfantryCount == living
			&& group.m_iDurableLivingInfantryCount == living
			&& group.m_iLastSeenAliveCount == living
			&& group.m_iSurvivorInfantryCount == living;
		bool casualtyExact;
		bool expectsNoCasualty
			= expected.m_sConfirmedCasualtySlotId.IsEmpty()
			&& expected.m_sCasualtyTombstoneFingerprint.IsEmpty();
		if (expectsNoCasualty)
		{
			casualtyExact = casualties == 0
				&& expected.m_iLivingMemberCount
					== expected.m_iAcceptedMemberCount;
		}
		else
		{
			HST_ForceSpawnSlotResultState casualtySlot
				= batch.FindSlotResult(expected.m_sConfirmedCasualtySlotId);
			casualtyExact = casualties == 1 && casualtySlot
				&& casualtySlot.m_eStatus
					== HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				&& casualtySlot.m_bEverAlive
				&& casualtySlot.m_bCasualtyConfirmed
				&& casualtySlot.m_sSpawnedPrefab.IsEmpty()
				&& casualtySlot.m_sEntityId.IsEmpty()
				&& casualtySlot.m_sAssignedVehicleEntityId.IsEmpty()
				&& casualtySlot.m_sNativeGroupId.IsEmpty()
				&& BuildExternalCasualtyTombstoneFingerprint(
					batch,
					expected.m_sConfirmedCasualtySlotId)
					== expected.m_sCasualtyTombstoneFingerprint;
		}
		bool exact = rosterExact && casualtyExact;
		evidence = string.Format(
			"external exact counterattack living/casualties/reprojections %1/%2/%3 | roster %4 | tombstone %5",
			living,
			casualties,
			batch.m_iReprojectionCount,
			rosterExact,
			casualtyExact);
		return exact;
	}

	protected string BuildExternalCasualtyTombstoneFingerprint(
		HST_ForceSpawnResultState batch,
		string slotId)
	{
		if (!batch || slotId.IsEmpty())
			return "";
		HST_ForceSpawnSlotResultState slot = batch.FindSlotResult(slotId);
		if (!slot || slot.m_sSlotKind
			!= HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
			return "";
		return string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8",
			slot.m_sSlotId,
			slot.m_sSlotKind,
			slot.m_eStatus,
			slot.m_iCasualtyAtSecond,
			slot.m_sRetirementReason,
			slot.m_bEverAlive,
			slot.m_bCasualtyConfirmed,
			slot.m_iLifecycleRevision);
	}

	protected string BuildExternalLivingSlotFingerprint(
		HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return "";
		array<string> livingSlotIds = {};
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				|| slot.m_bCasualtyConfirmed || slot.m_sSlotId.IsEmpty())
				continue;
			livingSlotIds.Insert(slot.m_sSlotId);
		}
		livingSlotIds.Sort();
		string fingerprint;
		foreach (string slotId : livingSlotIds)
		{
			if (!fingerprint.IsEmpty())
				fingerprint = fingerprint + ",";
			fingerprint = fingerprint + slotId;
		}
		return fingerprint;
	}

	protected bool ValidateExternalDebit(
		HST_CampaignState state,
		HST_EnemyOrderState order,
		HST_FactionPoolState pool,
		HST_EnemyCounterattackOutboundVirtualExpectation expected)
	{
		if (!state || !order || !pool || !expected
			|| state.m_aEnemyStrategicMutations.Count() != 1)
			return false;
		HST_EnemyStrategicMutationState mutation
			= state.m_aEnemyStrategicMutations[0];
		if (!mutation || !mutation.m_bApplied)
			return false;
		bool identityExact
			= mutation.m_sMutationId == expected.m_sDebitMutationId
			&& mutation.m_sKind == "proactive_attack_debit"
			&& mutation.m_sFactionKey == expected.m_sFactionKey
			&& mutation.m_sOrderId == expected.m_sOrderId;
		bool linksExact = mutation.m_sOperationId == expected.m_sOperationId
			&& mutation.m_sManifestId == expected.m_sManifestId;
		bool deltaExact = mutation.m_iAttackDelta == -expected.m_iAttackCost
			&& mutation.m_iSupportDelta == -expected.m_iSupportCost;
		bool balanceExact = mutation.m_iAttackAfter == pool.m_iAttackResources
			&& mutation.m_iSupportAfter == pool.m_iSupportResources;
		return identityExact && linksExact && deltaExact && balanceExact;
	}

	protected int CountExternalSupportClaimants(
		HST_CampaignState state,
		HST_EnemyCounterattackOutboundVirtualExpectation expected)
	{
		int count;
		if (!state || !expected)
			return count;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (request && (request.m_sRequestId == expected.m_sOrderId
				|| request.m_sOperationId == expected.m_sOperationId
				|| request.m_sManifestId == expected.m_sManifestId
				|| request.m_sSpawnResultId == expected.m_sBatchId
				|| request.m_sGroupId == expected.m_sGroupId))
				count++;
		}
		return count;
	}

	protected int CountExternalLegacyQRFClaimants(
		HST_CampaignState state,
		HST_EnemyCounterattackOutboundVirtualExpectation expected)
	{
		int count;
		if (!state || !expected)
			return count;
		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (qrf && qrf.m_sGroupId == expected.m_sGroupId)
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

	protected bool VectorExact(vector left, vector right, float epsilon)
	{
		return Math.AbsFloat(left[0] - right[0]) <= epsilon
			&& Math.AbsFloat(left[1] - right[1]) <= epsilon
			&& Math.AbsFloat(left[2] - right[2]) <= epsilon;
	}
}
#endif
