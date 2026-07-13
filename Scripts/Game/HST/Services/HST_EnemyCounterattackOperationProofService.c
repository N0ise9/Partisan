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
		bool quarantineExact = m_bSchema69QuarantineExact
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
			" | restore lifecycle %1 | resource authority %2 | ambiguity hold %3 | quarantine retention %4",
			m_bRestoreLifecycleExact,
			m_bResourceAuthorityQuarantineExact,
			m_bAmbiguityHoldExact,
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
			&& fixture.m_PhysicalWar && fixture.m_Service && fixture.m_Ownership;
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

	protected float Distance2D(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}
}
