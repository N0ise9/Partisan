class HST_OwnershipTransitionProofReport
{
	bool m_bMilitaryCaptureExact;
	bool m_bPoliticalFlipExact;
	bool m_bEnemyRecaptureExact;
	bool m_bReplayConflictStaleExact;
	bool m_bInterruptedRestoreExact;
	bool m_bProjectionSourceRevisionExact;
	bool m_bColocatedIdentityExact;
	bool m_bLinkedSupportIsolationExact;
	bool m_bPublicationFenceExact;
	bool m_bSerializedIntentRetryExact;
	bool m_bRestoreQueueOrderFailClosed;
	bool m_bPersistenceDeadlineExact;
	bool m_bAllCauseRoutingExact;
	bool m_bPreOwnerAggressionAtomicityExact;
	bool m_bExactSecurityFailClosed;
	bool m_bMigrationRetentionExact;
	string m_sProductionEvidence;
	string m_sReplayEvidence;
	string m_sRestoreProjectionEvidence;
	string m_sAuthorityEvidence;
	string m_sCauseEvidence;

	bool AllExact()
	{
		if (!m_bMilitaryCaptureExact)
			return false;
		if (!m_bPoliticalFlipExact)
			return false;
		if (!m_bEnemyRecaptureExact)
			return false;
		if (!m_bReplayConflictStaleExact)
			return false;
		if (!m_bInterruptedRestoreExact)
			return false;
		if (!m_bProjectionSourceRevisionExact)
			return false;
		if (!m_bColocatedIdentityExact)
			return false;
		if (!m_bLinkedSupportIsolationExact)
			return false;
		if (!m_bPublicationFenceExact)
			return false;
		if (!m_bSerializedIntentRetryExact)
			return false;
		if (!m_bRestoreQueueOrderFailClosed)
			return false;
		if (!m_bPersistenceDeadlineExact)
			return false;
		if (!m_bAllCauseRoutingExact)
			return false;
		if (!m_bPreOwnerAggressionAtomicityExact)
			return false;
		if (!m_bExactSecurityFailClosed)
			return false;
		if (!m_bMigrationRetentionExact)
			return false;
		return true;
	}

	string BuildReport()
	{
		string report = string.Format(
			"ownership transition proof | all exact %1 | military %2 | political %3 | recapture %4 | replay/conflict/stale %5",
			AllExact(),
			m_bMilitaryCaptureExact,
			m_bPoliticalFlipExact,
			m_bEnemyRecaptureExact,
			m_bReplayConflictStaleExact);
		report = report + string.Format(
			" | restore %1 | projection revision %2 | colocated identity %3 | security fail-closed %4 | migration/retention %5",
			m_bInterruptedRestoreExact,
			m_bProjectionSourceRevisionExact,
			m_bColocatedIdentityExact,
			m_bExactSecurityFailClosed,
			m_bMigrationRetentionExact);
		return report + string.Format(
			" | linked-support isolation %1 | publication fence %2 | serialized retry %3 | malformed queue fail-closed %4 | persistence deadline %5 | all-cause routing %6 | pre-owner aggression atomicity %7",
			m_bLinkedSupportIsolationExact,
			m_bPublicationFenceExact,
			m_bSerializedIntentRetryExact,
			m_bRestoreQueueOrderFailClosed,
			m_bPersistenceDeadlineExact,
			m_bAllCauseRoutingExact,
			m_bPreOwnerAggressionAtomicityExact);
	}
}

// Source-only projection seam. It preserves the production marker identity and
// owner-source-revision contract while avoiding native marker-manager access.
// Blocking one rebuild lets the proof persist a genuine production transition
// after domain completion but before projection completion.
class HST_OwnershipTransitionMapProofHarness : HST_MapMarkerService
{
	protected bool m_bProjectionBlocked;
	protected bool m_bObservedPrematureOwnershipProjection;
	protected string m_sStagedMarkerToRemoveForProof;

	void SetProjectionBlocked(bool blocked)
	{
		m_bProjectionBlocked = blocked;
	}

	bool ObservedPrematureOwnershipProjection()
	{
		return m_bObservedPrematureOwnershipProjection;
	}

	void RemoveNextStagedZoneMarkerForProof(string zoneId)
	{
		m_sStagedMarkerToRemoveForProof = "hst_zone_" + zoneId;
	}

	bool CanPublishOwnershipSnapshotForProof(
		HST_CampaignState state,
		string authorizedOwnershipRequestId)
	{
		return CanPublishOwnershipSnapshot(state, authorizedOwnershipRequestId);
	}

	bool ResolveZoneProjectionAuthorityForProof(
		HST_CampaignState state,
		HST_ZoneState zone,
		string authorizedOwnershipRequestId,
		out string ownerFactionKey,
		out int ownershipRevision)
	{
		return ResolveZoneProjectionAuthority(
			state,
			zone,
			authorizedOwnershipRequestId,
			ownerFactionKey,
			ownershipRevision);
	}

	override HST_OwnershipMarkerProjectionTransaction StageOwnershipTransitionMarkers(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string ownershipRequestId)
	{
		if (!state || !preset || m_bProjectionBlocked)
			return null;
		foreach (HST_OwnershipTransitionState transition : state.m_aOwnershipTransitions)
		{
			if (!transition || !transition.m_bOwnerApplied || transition.m_bCompleted)
				continue;
			if (!DomainReadyForProjection(transition))
				m_bObservedPrematureOwnershipProjection = true;
		}
		HST_OwnershipMarkerProjectionTransaction transaction = super.StageOwnershipTransitionMarkers(
			state,
			preset,
			ownershipRequestId);
		if (transaction && !m_sStagedMarkerToRemoveForProof.IsEmpty())
		{
			for (int markerIndex = state.m_aMapMarkers.Count() - 1; markerIndex >= 0; markerIndex--)
			{
				HST_MapMarkerState marker = state.m_aMapMarkers[markerIndex];
				if (marker && marker.m_sMarkerId == m_sStagedMarkerToRemoveForProof)
					state.m_aMapMarkers.Remove(markerIndex);
			}
			m_sStagedMarkerToRemoveForProof = "";
		}
		return transaction;
	}

	override bool RebuildAllMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return false;
		if (!CanPublishOwnershipSnapshot(state, m_sAuthorizedOwnershipPublicationRequestId))
			return false;
		if (m_bProjectionBlocked)
			return false;

		foreach (HST_OwnershipTransitionState transition : state.m_aOwnershipTransitions)
		{
			if (!transition || !transition.m_bOwnerApplied || transition.m_bCompleted)
				continue;
			if (!DomainReadyForProjection(transition))
				m_bObservedPrematureOwnershipProjection = true;
		}

		int previousProjectionSequence;
		bool setupProjection;
		if (!RebuildLogicalMarkerSnapshot(
				state,
				preset,
				m_sAuthorizedOwnershipPublicationRequestId,
				previousProjectionSequence,
				setupProjection))
			return false;
		return setupProjection
			|| state.m_iMarkerProjectionSequence != previousProjectionSequence;
	}

	protected bool DomainReadyForProjection(HST_OwnershipTransitionState transition)
	{
		if (!transition.m_bSupportApplied)
			return false;
		if (!transition.m_bFacilitiesApplied)
			return false;
		if (!transition.m_bLogisticsApplied)
			return false;
		if (!transition.m_bEnemyConsequencesApplied)
			return false;
		if (!transition.m_bEconomyApplied)
			return false;
		if (!transition.m_bStrategicEventCompleted)
			return false;
		if (!transition.m_bEventAppended)
			return false;
		return true;
	}
}

// These wrappers expose only deterministic production helpers. Apply,
// production callers, restore reconciliation, and save validation remain the
// implementation under proof.
class HST_OwnershipTransitionProofHarness : HST_OwnershipTransitionService
{
	HST_OwnershipTransitionRequest BuildReplayRequestForProof(HST_OwnershipTransitionState transition)
	{
		return RequestFromTransition(transition);
	}

	bool CanPruneForProof(HST_CampaignState state, HST_OwnershipTransitionState transition)
	{
		return CanPruneTransition(state, transition);
	}

	void PruneForProof(HST_CampaignState state)
	{
		PruneCompletedHistory(state);
	}

	string ReleaseDeferredChildPublicationsForProof(
		HST_CampaignState state,
		HST_OwnershipTransitionState parent,
		bool liveProjection)
	{
		return ReleaseDeferredChildPublications(state, parent, liveProjection);
	}

	HST_OwnershipTransitionState AdmitPendingForProof(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionRequest request)
	{
		if (!state || !zone || !request)
			return null;
		if (!ValidateNewRequest(state, request).IsEmpty())
			return null;
		HST_OwnershipTransitionState transition = CreateTransition(state, zone, request);
		if (!transition || transition.m_sStrategicEventId.IsEmpty())
			return null;
		state.m_aOwnershipTransitions.Insert(transition);
		zone.m_sActiveOwnershipTransitionRequestId = transition.m_sRequestId;
		return transition;
	}

	string AdmitEnemyAggressionForProof(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition)
	{
		return AdmitEnemyAggression(state, zone, transition);
	}
}

class HST_OwnershipTransitionPersistenceProofHarness : HST_PersistenceService
{
	int m_iMajorChangeRequests;

	override void MarkMajorChange()
	{
		m_iMajorChangeRequests++;
	}

	bool MajorChangePendingForProof()
	{
		return m_bMajorChangePending;
	}
}

class HST_OwnershipTransitionPersistenceClockProofHarness : HST_PersistenceService
{
	int m_iCheckpointAttempts;
	int m_iFailuresRemaining = 1;

	override bool RequestTypedCheckpoint(
		string displayName,
		ESaveGameType saveType,
		HST_CampaignState state = null)
	{
		m_iCheckpointAttempts++;
		if (m_iFailuresRemaining > 0)
		{
			m_iFailuresRemaining--;
			return false;
		}
		return true;
	}

	bool MajorChangePendingForProof()
	{
		return m_bMajorChangePending;
	}
}

class HST_OwnershipTransitionZoneCaptureProofHarness : HST_ZoneCaptureService
{
	int PendingNotificationCountForProof()
	{
		return m_aPendingNotifications.Count();
	}
}

class HST_OwnershipTransitionProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_BalanceConfig m_Balance;
	ref HST_StrategicService m_Strategic;
	ref HST_EconomyService m_Economy;
	ref HST_EnemyStrategicResourceService m_EnemyStrategicResources;
	ref HST_EnemyDirectorService m_EnemyDirector;
	ref HST_GarrisonService m_Garrisons;
	ref HST_CivilianService m_Civilians;
	ref HST_TownInfluenceService m_TownInfluence;
	ref HST_CampaignEventLogService m_CampaignEvents;
	ref HST_OwnershipTransitionZoneCaptureProofHarness m_ZoneCapture;
	ref HST_OwnershipTransitionMapProofHarness m_MapMarkers;
	ref HST_OwnershipTransitionPersistenceProofHarness m_Persistence;
	ref HST_OwnershipTransitionProofHarness m_Service;
}

class HST_OwnershipTransitionProofFixtureFactory
{
	protected HST_TownInfluenceService m_CurrentTownInfluence;

	HST_OwnershipTransitionProofFixture Build(string suffix)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_sPresetId = "ownership_transition_proof_" + suffix;
		state.m_iCampaignSeed = 6200 + suffix.Hash();
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		state.m_iElapsedSeconds = 620;
		state.m_iIncomeAccumulatorSeconds = 1;
		state.m_iWarLevel = 1;
		state.m_iFactionMoney = 1000;
		state.m_iHR = 20;

		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_BalanceConfig balance = new HST_BalanceConfig();
		balance.m_iCaptureCounterattackChancePercent = 0;
		balance.m_bPopulationOutcomeEnabled = false;
		balance.m_bLegacyControlVictoryEnabled = false;
		balance.m_bLossConditionEnabled = false;
		HST_DefaultCatalog.AddDefaultFactionPools(state, balance, preset);
		return Configure(state, preset, balance);
	}

	HST_OwnershipTransitionProofFixture Configure(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance)
	{
		HST_OwnershipTransitionProofFixture fixture = new HST_OwnershipTransitionProofFixture();
		fixture.m_State = state;
		fixture.m_Preset = preset;
		fixture.m_Balance = balance;
		fixture.m_Strategic = new HST_StrategicService();
		fixture.m_Economy = new HST_EconomyService();
		fixture.m_Garrisons = new HST_GarrisonService();
		fixture.m_Civilians = new HST_CivilianService();
		fixture.m_TownInfluence = new HST_TownInfluenceService();
		fixture.m_EnemyStrategicResources = new HST_EnemyStrategicResourceService();
		fixture.m_CampaignEvents = new HST_CampaignEventLogService();
		fixture.m_ZoneCapture = new HST_OwnershipTransitionZoneCaptureProofHarness();
		fixture.m_MapMarkers = new HST_OwnershipTransitionMapProofHarness();
		fixture.m_Service = new HST_OwnershipTransitionProofHarness();

		HST_EnemyCommanderService enemyCommander = new HST_EnemyCommanderService();
		fixture.m_EnemyDirector = new HST_EnemyDirectorService();
		HST_SupportRequestService supportRequests = new HST_SupportRequestService();
		enemyCommander.SetTownInfluenceService(fixture.m_TownInfluence);
		fixture.m_EnemyDirector.SetTownInfluenceService(fixture.m_TownInfluence);
		fixture.m_EnemyDirector.SetCampaignPreset(preset);
		fixture.m_EnemyDirector.SetEnemyStrategicResourceAuthority(
			fixture.m_EnemyStrategicResources);
		supportRequests.SetTownInfluenceService(fixture.m_TownInfluence);
		HST_PhysicalWarService physicalWar = new HST_PhysicalWarService();
		HST_LocalSecurityOperationService localSecurityPatrols = new HST_LocalSecurityOperationService();
		HST_GarrisonPatrolOperationService garrisonPatrols = new HST_GarrisonPatrolOperationService();
		fixture.m_Persistence = new HST_OwnershipTransitionPersistenceProofHarness();

		fixture.m_Service.ConfigureDomainServices(
			preset,
			balance,
			fixture.m_Economy,
			fixture.m_Strategic,
			fixture.m_Garrisons,
			fixture.m_Civilians,
			fixture.m_CampaignEvents);
		fixture.m_Service.ConfigureRuntimeServices(
			enemyCommander,
			fixture.m_EnemyDirector,
			supportRequests,
			physicalWar,
			localSecurityPatrols,
			garrisonPatrols,
			null,
			fixture.m_ZoneCapture);
		fixture.m_Service.ConfigureProjectionServices(fixture.m_MapMarkers, null, fixture.m_Persistence);
		fixture.m_TownInfluence.SetCampaignPreset(preset);
		fixture.m_TownInfluence.SetStrategicService(fixture.m_Strategic);
		fixture.m_TownInfluence.SetEconomyService(fixture.m_Economy);
		fixture.m_EnemyStrategicResources.SetTownInfluenceService(
			fixture.m_TownInfluence);
		fixture.m_Economy.SetCampaignPreset(preset);
		fixture.m_Economy.SetEnemyStrategicResourceAuthority(
			fixture.m_EnemyStrategicResources);
		fixture.m_Strategic.SetTownInfluenceService(fixture.m_TownInfluence);
		fixture.m_TownInfluence.SetOwnershipTransitionService(fixture.m_Service);
		fixture.m_Service.SetTownInfluenceService(fixture.m_TownInfluence);
		fixture.m_ZoneCapture.SetOwnershipTransitionService(fixture.m_Service);
		fixture.m_Civilians.SetStrategicService(fixture.m_Strategic);
		fixture.m_Civilians.SetOwnershipTransitionService(fixture.m_Service);
		fixture.m_Civilians.SetCampaignPreset(preset);
		fixture.m_Civilians.SetTownInfluenceService(fixture.m_TownInfluence);
		m_CurrentTownInfluence = fixture.m_TownInfluence;
		return fixture;
	}

	HST_ZoneState AddZone(
		HST_CampaignState state,
		string zoneId,
		string ownerFactionKey,
		HST_EZoneType zoneType,
		vector position,
		int garrisonSlots = 0)
	{
		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = zoneId;
		zone.m_sDisplayName = "Ownership Proof " + zoneId;
		zone.m_sOwnerFactionKey = ownerFactionKey;
		zone.m_iOwnershipContractVersion = HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION;
		zone.m_iOwnershipRevision = 1;
		zone.m_eType = zoneType;
		zone.m_vPosition = position;
		zone.m_iCaptureRadiusMeters = 250;
		zone.m_iActivationRadiusMeters = 1200;
		zone.m_iGarrisonSlots = garrisonSlots;
		state.m_aZones.Insert(zone);
		return zone;
	}

	HST_CivilianZoneState AddCivilianZone(HST_CampaignState state, string zoneId, int fiaSupport, int occupierSupport)
	{
		HST_CivilianZoneState civilianZone = new HST_CivilianZoneState();
		civilianZone.m_sZoneId = zoneId;
		civilianZone.m_iReputation = 50;
		civilianZone.m_iFIASupport = fiaSupport;
		civilianZone.m_iOccupierSupport = occupierSupport;
		civilianZone.m_iCivilianPresence = 8;
		civilianZone.m_iPolicePresence = 2;
		civilianZone.m_iRoadblockPresence = 1;
		civilianZone.m_iPopulationRemaining = 64;
		state.m_aCivilianZones.Insert(civilianZone);
		if (m_CurrentTownInfluence)
			m_CurrentTownInfluence.EnsureRecords(state);
		return civilianZone;
	}
}

// Deterministic in-memory proof. Native marker entities, rendered UI,
// networking/JIP, profile I/O, and a real save/restart remain packaged gates.
class HST_OwnershipTransitionProofService
{
	protected ref HST_OwnershipTransitionProofFixtureFactory m_Factory = new HST_OwnershipTransitionProofFixtureFactory();

	HST_OwnershipTransitionProofReport Run()
	{
		return BuildProofReport();
	}

	HST_OwnershipTransitionProofReport BuildProofReport()
	{
		HST_OwnershipTransitionProofReport report = new HST_OwnershipTransitionProofReport();
		ProveMilitaryReplayRecaptureAndIdentity(report);
		ProvePoliticalEntryPoint(report);
		ProveAllCauseRouting(report);
		ProvePreOwnerAggressionAdmissionAtomicity(report);
		ProveLinkedSupportIsolation(report);
		ProveInterruptedSaveRestore(report);
		ProvePublicationFence(report);
		ProveSerializedPoliticalIntentRetry(report);
		ProveMalformedOwnerAppliedQueueRestore(report);
		ProvePersistenceCheckpointDeadline(report);
		ProveExactSecurityFailClosed(report);
		ProveMigrationAndRetention(report);
		return report;
	}

	protected void ProvePreOwnerAggressionAdmissionAtomicity(
		HST_OwnershipTransitionProofReport report)
	{
		HST_OwnershipTransitionProofFixture fixture
			= m_Factory.Build("pre_owner_aggression_atomicity");
		HST_ZoneState zone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_pre_owner_aggression",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"7600 20 7600",
			8);
		HST_FactionPoolState pool = fixture.m_State.FindFactionPool(
			fixture.m_Preset.m_sOccupierFactionKey);
		if (!zone || !pool)
			return;
		HST_GarrisonState oldSecurity = fixture.m_Garrisons.FindOrCreate(
			fixture.m_State,
			zone.m_sZoneId,
			fixture.m_Preset.m_sOccupierFactionKey);
		if (!oldSecurity)
			return;
		oldSecurity.m_iInfantryCount = 6;
		oldSecurity.m_iVehicleCount = 1;
		string oldSecurityId = oldSecurity.m_sGarrisonId;
		int garrisonCountBefore = fixture.m_State.m_aGarrisons.Count();

		string requestId = "ownership_proof_pre_owner_aggression_request";
		string mutationId = "enemy_aggression_ownership_" + requestId;
		int ownerRevisionBefore = zone.m_iOwnershipRevision;
		int aggressionBefore = pool.m_iAggression;
		int poolRevisionBefore = pool.m_iStrategicRevision;
		int operationalCountBefore
			= pool.m_iStrategicOperationalMutationCount;
		int mutationCountBefore = fixture.m_State.m_aEnemyStrategicMutations.Count();
		HST_OwnershipTransitionRequest request = fixture.m_Service.BuildRequest(
			fixture.m_State,
			zone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"military_capture",
			"source_proof",
			"pre_owner_aggression_atomicity",
			"resource admission must precede visible owner publication",
			0,
			requestId);
		request.m_bReconcileSecurity = true;
		request.m_bCreateSecurity = true;
		request.m_bNotify = false;

		fixture.m_Economy.SetEnemyStrategicResourceAuthority(null);
		HST_OwnershipTransitionResult blocked = fixture.m_Service.Apply(
			fixture.m_State,
			request);
		HST_OwnershipTransitionState receipt
			= fixture.m_State.FindOwnershipTransition(requestId);
		bool blockedExact = request.m_bReconcileSecurity
			&& request.m_bCreateSecurity && blocked && blocked.m_bAccepted;
		blockedExact = blockedExact && blocked.m_bNeedsRetry;
		blockedExact = blockedExact && !blocked.m_bCompleted
			&& blocked.m_sFailureReason.Contains("before owner publication");
		blockedExact = blockedExact && receipt && !receipt.m_bOwnerApplied
			&& !receipt.m_bEnemyConsequencesApplied
			&& receipt.m_iAggressionApplied == 0;
		blockedExact = blockedExact && !receipt.m_bOldSecurityRetired
			&& !receipt.m_bHostileRuntimeRetired
			&& !receipt.m_bNewSecurityApplied && !receipt.m_bSupportApplied;
		blockedExact = blockedExact
			&& zone.m_sOwnerFactionKey == fixture.m_Preset.m_sOccupierFactionKey
			&& zone.m_iOwnershipRevision == ownerRevisionBefore
			&& zone.m_sActiveOwnershipTransitionRequestId == requestId;
		blockedExact = blockedExact
			&& fixture.m_State.m_aGarrisons.Count() == garrisonCountBefore
			&& oldSecurity.m_iInfantryCount == 6
			&& oldSecurity.m_iVehicleCount == 1
			&& !fixture.m_State.FindGarrison(
				zone.m_sZoneId,
				fixture.m_Preset.m_sResistanceFactionKey);
		blockedExact = blockedExact && pool.m_iAggression == aggressionBefore
			&& pool.m_iStrategicRevision == poolRevisionBefore
			&& pool.m_iStrategicOperationalMutationCount == operationalCountBefore;
		blockedExact = blockedExact
			&& fixture.m_State.m_aEnemyStrategicMutations.Count() == mutationCountBefore
			&& !fixture.m_State.FindEnemyStrategicMutation(mutationId);

		HST_CampaignSaveData pendingSave = new HST_CampaignSaveData();
		pendingSave.Capture(fixture.m_State);
		HST_OwnershipTransitionSaveValidationService ownershipValidator
			= new HST_OwnershipTransitionSaveValidationService();
		ownershipValidator.Normalize(
			pendingSave,
			HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_EnemyStrategicResourceSaveValidationService resourceValidator
			= new HST_EnemyStrategicResourceSaveValidationService();
		resourceValidator.Normalize(
			pendingSave,
			HST_EnemyStrategicResourceSaveValidationService.SCHEMA_VERSION);
		HST_CampaignState resumedState = pendingSave.Restore();
		fixture = m_Factory.Configure(
			resumedState,
			fixture.m_Preset,
			fixture.m_Balance);
		zone = fixture.m_State.FindZone(
			"ownership_proof_pre_owner_aggression");
		pool = fixture.m_State.FindFactionPool(
			fixture.m_Preset.m_sOccupierFactionKey);
		receipt = fixture.m_State.FindOwnershipTransition(requestId);
		oldSecurity = fixture.m_State.FindGarrison(
			"ownership_proof_pre_owner_aggression",
			fixture.m_Preset.m_sOccupierFactionKey);
		bool resumeExact = zone && pool && receipt;
		resumeExact = resumeExact && !receipt.m_bQuarantined
			&& !receipt.m_bOwnerApplied;
		resumeExact = resumeExact && !receipt.m_bEnemyConsequencesApplied
			&& receipt.m_iAggressionApplied == 0;
		resumeExact = resumeExact
			&& zone.m_sOwnerFactionKey == fixture.m_Preset.m_sOccupierFactionKey
			&& zone.m_iOwnershipRevision == ownerRevisionBefore;
		resumeExact = resumeExact
			&& zone.m_sActiveOwnershipTransitionRequestId == requestId;
		resumeExact = resumeExact && pool.m_iAggression == aggressionBefore
			&& pool.m_iStrategicRevision == poolRevisionBefore;
		resumeExact = resumeExact
			&& pool.m_iStrategicOperationalMutationCount == operationalCountBefore;
		resumeExact = resumeExact
			&& fixture.m_State.m_aEnemyStrategicMutations.Count() == mutationCountBefore
			&& !fixture.m_State.FindEnemyStrategicMutation(mutationId);
		resumeExact = resumeExact && oldSecurity
			&& oldSecurity.m_sGarrisonId == oldSecurityId;
		resumeExact = resumeExact && oldSecurity.m_iInfantryCount == 6
			&& oldSecurity.m_iVehicleCount == 1;
		if (!resumeExact)
		{
			report.m_sAuthorityEvidence
				= report.m_sAuthorityEvidence
				+ " | pre-owner aggression pending restore failed";
			return;
		}
		string pendingAdmissionFailure
			= fixture.m_Service.AdmitEnemyAggressionForProof(
				fixture.m_State,
				zone,
				receipt);
		HST_EnemyStrategicMutationState mutation
			= fixture.m_State.FindEnemyStrategicMutation(mutationId);
		int aggressionAfterAdmission = pool.m_iAggression;
		bool pendingAdmissionExact = pendingAdmissionFailure.IsEmpty()
			&& mutation && receipt.m_iAggressionApplied > 0;
		pendingAdmissionExact = pendingAdmissionExact && !receipt.m_bOwnerApplied
			&& !receipt.m_bOldSecurityRetired;
		pendingAdmissionExact = pendingAdmissionExact
			&& !receipt.m_bHostileRuntimeRetired
			&& !receipt.m_bNewSecurityApplied;
		pendingAdmissionExact = pendingAdmissionExact && !receipt.m_bSupportApplied
			&& !receipt.m_bEnemyConsequencesApplied;
		pendingAdmissionExact = pendingAdmissionExact
			&& aggressionAfterAdmission == aggressionBefore + receipt.m_iAggressionApplied;
		pendingAdmissionExact = pendingAdmissionExact
			&& fixture.m_State.m_aEnemyStrategicMutations.Count() == mutationCountBefore + 1;
		HST_CampaignSaveData admittedSave = new HST_CampaignSaveData();
		admittedSave.Capture(fixture.m_State);
		ownershipValidator.Normalize(
			admittedSave,
			HST_OwnershipTransitionService.SCHEMA_VERSION);
		resourceValidator.Normalize(
			admittedSave,
			HST_EnemyStrategicResourceSaveValidationService.SCHEMA_VERSION);
		resumedState = admittedSave.Restore();
		fixture = m_Factory.Configure(
			resumedState,
			fixture.m_Preset,
			fixture.m_Balance);
		zone = fixture.m_State.FindZone(
			"ownership_proof_pre_owner_aggression");
		pool = fixture.m_State.FindFactionPool(
			fixture.m_Preset.m_sOccupierFactionKey);
		receipt = fixture.m_State.FindOwnershipTransition(requestId);
		oldSecurity = fixture.m_State.FindGarrison(
			"ownership_proof_pre_owner_aggression",
			fixture.m_Preset.m_sOccupierFactionKey);
		mutation = fixture.m_State.FindEnemyStrategicMutation(mutationId);
		bool admittedRestoreExact = zone && pool && receipt && oldSecurity;
		admittedRestoreExact = admittedRestoreExact && mutation
			&& !receipt.m_bQuarantined && !receipt.m_bOwnerApplied;
		admittedRestoreExact = admittedRestoreExact
			&& !receipt.m_bOldSecurityRetired
			&& !receipt.m_bHostileRuntimeRetired;
		admittedRestoreExact = admittedRestoreExact
			&& !receipt.m_bNewSecurityApplied && !receipt.m_bSupportApplied;
		admittedRestoreExact = admittedRestoreExact
			&& !receipt.m_bEnemyConsequencesApplied;
		admittedRestoreExact = admittedRestoreExact
			&& receipt.m_iAggressionApplied == mutation.m_iAggressionDelta;
		admittedRestoreExact = admittedRestoreExact
			&& pool.m_iAggression == aggressionAfterAdmission;
		admittedRestoreExact = admittedRestoreExact
			&& pool.m_iStrategicContractVersion
				== HST_EnemyStrategicResourceService.CONTRACT_VERSION;
		admittedRestoreExact = admittedRestoreExact
			&& oldSecurity.m_iInfantryCount == 6
			&& oldSecurity.m_iVehicleCount == 1;
		if (!pendingAdmissionExact || !admittedRestoreExact)
		{
			report.m_sAuthorityEvidence
				= report.m_sAuthorityEvidence
				+ " | admitted pre-owner aggression restore failed";
			return;
		}

		HST_OwnershipTransitionResult retry = fixture.m_Service.Apply(
			fixture.m_State,
			fixture.m_Service.BuildReplayRequestForProof(receipt));
		mutation = fixture.m_State.FindEnemyStrategicMutation(mutationId);
		int mutationCountAfterRetry
			= fixture.m_State.m_aEnemyStrategicMutations.Count();
		int aggressionAfterRetry = pool.m_iAggression;
		HST_GarrisonState newSecurity = fixture.m_State.FindGarrison(
			zone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey);
		bool retryExact = retry && retry.m_bAccepted && retry.m_bCompleted;
		retryExact = retryExact && !retry.m_bNeedsRetry
			&& receipt.m_bOwnerApplied && receipt.m_bEnemyConsequencesApplied
			&& receipt.m_iAggressionApplied > 0;
		retryExact = retryExact
			&& zone.m_sOwnerFactionKey == fixture.m_Preset.m_sResistanceFactionKey
			&& zone.m_iOwnershipRevision == ownerRevisionBefore + 1;
		retryExact = retryExact && receipt.m_bOldSecurityRetired
			&& receipt.m_bHostileRuntimeRetired
			&& receipt.m_bNewSecurityApplied && receipt.m_bSupportApplied;
		retryExact = retryExact && oldSecurity.m_iInfantryCount == 0
			&& oldSecurity.m_iVehicleCount == 0 && newSecurity
			&& newSecurity.m_iInfantryCount == 2
			&& receipt.m_sOldGarrisonId == oldSecurityId
			&& receipt.m_sNewGarrisonId == newSecurity.m_sGarrisonId;
		retryExact = retryExact && mutation
			&& mutation.m_sFactionKey == receipt.m_sPreviousOwnerFactionKey
			&& mutation.m_sKind == "ownership_transition";
		retryExact = retryExact && mutation.m_sSourceId == requestId
			&& mutation.m_sZoneId == zone.m_sZoneId
			&& mutation.m_iAggressionDelta == receipt.m_iAggressionApplied;
		retryExact = retryExact
			&& aggressionAfterRetry == aggressionBefore + receipt.m_iAggressionApplied
			&& pool.m_iStrategicOperationalMutationCount == operationalCountBefore + 1
			&& mutationCountAfterRetry == mutationCountBefore + 1;

		HST_OwnershipTransitionResult replay = fixture.m_Service.Apply(
			fixture.m_State,
			fixture.m_Service.BuildReplayRequestForProof(receipt));
		bool replayExact = replay && replay.m_bAccepted
			&& replay.m_bAlreadyApplied && replay.m_bCompleted
			&& fixture.m_State.m_aEnemyStrategicMutations.Count()
				== mutationCountAfterRetry
			&& pool.m_iAggression == aggressionAfterRetry
			&& pool.m_iStrategicOperationalMutationCount
				== operationalCountBefore + 1;

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		ownershipValidator.Normalize(
			saveData,
			HST_OwnershipTransitionService.SCHEMA_VERSION);
		resourceValidator.Normalize(
			saveData,
			HST_EnemyStrategicResourceSaveValidationService.SCHEMA_VERSION);
		HST_CampaignState restoredState = saveData.Restore();
		HST_OwnershipTransitionState restoredReceipt
			= restoredState.FindOwnershipTransition(requestId);
		HST_EnemyStrategicMutationState restoredMutation
			= restoredState.FindEnemyStrategicMutation(mutationId);
		HST_FactionPoolState restoredPool = restoredState.FindFactionPool(
			fixture.m_Preset.m_sOccupierFactionKey);
		bool backlinkExact = restoredReceipt && restoredMutation && restoredPool
			&& !restoredReceipt.m_bQuarantined
			&& restoredReceipt.m_iContractVersion
				== HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			&& restoredMutation.m_iContractVersion
				== HST_EnemyStrategicResourceService.CONTRACT_VERSION
			&& restoredMutation.m_sSourceId == restoredReceipt.m_sRequestId
			&& restoredMutation.m_sFactionKey
				== restoredReceipt.m_sPreviousOwnerFactionKey
			&& restoredMutation.m_sZoneId == restoredReceipt.m_sZoneId
			&& restoredMutation.m_iAggressionDelta
				== restoredReceipt.m_iAggressionApplied
			&& restoredPool.m_iStrategicOperationalMutationCount
				== operationalCountBefore + 1
			&& restoredPool.m_sStrategicAuthorityFailure.IsEmpty();

		report.m_bPreOwnerAggressionAtomicityExact
			= blockedExact && resumeExact && pendingAdmissionExact
				&& admittedRestoreExact && retryExact && replayExact
				&& backlinkExact;
		report.m_sAuthorityEvidence = report.m_sAuthorityEvidence + string.Format(
			" | pre-owner aggression blocked %1 resume %2 pending/restore %3/%4 retry %5 replay %6 backlink %7",
			blockedExact,
			resumeExact,
			pendingAdmissionExact,
			admittedRestoreExact,
			retryExact,
			replayExact,
			backlinkExact);
	}

	protected void ProveAllCauseRouting(HST_OwnershipTransitionProofReport report)
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("causes");
		HST_ZoneState missionZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_cause_mission",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"10100 20 10100");
		HST_ZoneState adminZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_cause_admin",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"10400 20 10100");
		HST_ZoneState debugZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_cause_debug",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"10700 20 10100");
		HST_ZoneState migrationZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_cause_migration",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"11000 20 10100");

		bool missionCaptured = fixture.m_ZoneCapture.CaptureForResistance(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Strategic,
			fixture.m_Economy,
			fixture.m_Balance,
			missionZone.m_sZoneId,
			0,
			null,
			null,
			null,
			null,
			"ownership_proof_mission_instance");
		HST_OwnershipTransitionState missionReceipt = fixture.m_State.FindOwnershipTransition(
			missionZone.m_sLastOwnershipTransitionRequestId);
		HST_OwnershipTransitionState adminReceipt = ApplyCanonicalCause(
			fixture,
			adminZone,
			"admin",
			"ownership_proof_admin_source");
		HST_OwnershipTransitionState debugReceipt = ApplyCanonicalCause(
			fixture,
			debugZone,
			"debug_seed",
			"ownership_proof_debug_source");
		HST_OwnershipTransitionState migrationReceipt = ApplyCanonicalCause(
			fixture,
			migrationZone,
			"migration_repair",
			"ownership_proof_migration_source");

		bool missionExact = missionCaptured
			&& missionReceipt
			&& missionReceipt.m_bCompleted
			&& missionReceipt.m_sCause == "mission_capture"
			&& missionReceipt.m_iAppliedRevision == 2;
		bool directCausesExact = ReceiptMatchesCause(adminReceipt, "admin")
			&& ReceiptMatchesCause(debugReceipt, "debug_seed")
			&& ReceiptMatchesCause(migrationReceipt, "migration_repair");
		report.m_bAllCauseRoutingExact = report.m_bMilitaryCaptureExact
			&& report.m_bPoliticalFlipExact
			&& missionExact
			&& directCausesExact
			&& fixture.m_State.m_aOwnershipTransitions.Count() == 4;
		report.m_sCauseEvidence = string.Format(
			"military/political/mission/admin/debug/migration %1/%2/%3/%4/%5/%6",
			report.m_bMilitaryCaptureExact,
			report.m_bPoliticalFlipExact,
			missionExact,
			ReceiptMatchesCause(adminReceipt, "admin"),
			ReceiptMatchesCause(debugReceipt, "debug_seed"),
			ReceiptMatchesCause(migrationReceipt, "migration_repair"));
	}

	protected HST_OwnershipTransitionState ApplyCanonicalCause(
		HST_OwnershipTransitionProofFixture fixture,
		HST_ZoneState zone,
		string cause,
		string sourceId)
	{
		if (!fixture || !zone)
			return null;
		HST_OwnershipTransitionRequest request = fixture.m_Service.BuildRequest(
			fixture.m_State,
			zone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			cause,
			"source_proof",
			sourceId,
			"source proof cause routing " + cause,
			0,
			"ownership_proof_cause_" + cause);
		request.m_bApplyEnemyConsequences = false;
		request.m_bReconcileSecurity = false;
		request.m_bCreateSecurity = false;
		request.m_bNotify = false;
		HST_OwnershipTransitionResult result = fixture.m_Service.Apply(fixture.m_State, request);
		if (!result || !result.m_bAccepted || !result.m_bCompleted)
			return null;
		return result.m_Transition;
	}

	protected bool ReceiptMatchesCause(HST_OwnershipTransitionState transition, string cause)
	{
		return transition
			&& transition.m_bCompleted
			&& transition.m_sCause == cause
			&& transition.m_iExpectedRevision == 1
			&& transition.m_iAppliedRevision == 2;
	}

	protected void ProveLinkedSupportIsolation(HST_OwnershipTransitionProofReport report)
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("linked_support");
		HST_ZoneState capturedZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_linked_site",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"7600 20 7600");
		HST_ZoneState linkedTown = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_linked_town",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"7900 20 7600");
		capturedZone.m_aLinkedZoneIds.Insert(linkedTown.m_sZoneId);
		HST_CivilianZoneState civilianZone = m_Factory.AddCivilianZone(
			fixture.m_State,
			linkedTown.m_sZoneId,
			60,
			40);

		bool captured = fixture.m_ZoneCapture.CaptureForResistance(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Strategic,
			fixture.m_Economy,
			fixture.m_Balance,
			capturedZone.m_sZoneId,
			10);
		HST_OwnershipTransitionState parentReceipt = fixture.m_State.FindOwnershipTransition(capturedZone.m_sLastOwnershipTransitionRequestId);
		HST_OwnershipTransitionState townReceipt = fixture.m_State.FindOwnershipTransition(linkedTown.m_sLastOwnershipTransitionRequestId);
		HST_MapMarkerState parentMarker = fixture.m_State.FindMapMarker("hst_zone_" + capturedZone.m_sZoneId);
		HST_MapMarkerState townMarker = fixture.m_State.FindMapMarker("hst_zone_" + linkedTown.m_sZoneId);
		int receiptCountBeforeReplay = fixture.m_State.m_aOwnershipTransitions.Count();
		int influenceCountBeforeReplay = fixture.m_State.m_aTownInfluenceEvents.Count();
		int townSupportBeforeReplay = civilianZone.m_iFIASupport;
		int persistenceRequestsBeforeReplay = fixture.m_Persistence.m_iMajorChangeRequests;
		HST_OwnershipTransitionResult replay;
		if (parentReceipt)
			replay = fixture.m_Service.Apply(fixture.m_State, fixture.m_Service.BuildReplayRequestForProof(parentReceipt));

		bool parentExact = LinkedParentReceiptIsExact(captured, parentReceipt, linkedTown.m_sZoneId);
		bool nestedExact = LinkedTownReceiptIsExact(
			fixture,
			capturedZone,
			linkedTown,
			civilianZone,
			parentReceipt,
			townReceipt);
		bool projectionExact = LinkedMarkersAreExact(
			capturedZone,
			linkedTown,
			parentReceipt,
			townReceipt,
			parentMarker,
			townMarker);
		bool replayExact = LinkedReplayIsExact(
			fixture,
			civilianZone,
			replay,
			receiptCountBeforeReplay,
			influenceCountBeforeReplay,
			townSupportBeforeReplay,
			persistenceRequestsBeforeReplay);
		bool queuedCollisionExact = ProveLinkedSupportQueuedTopLevelCollision();
		report.m_bLinkedSupportIsolationExact = parentExact;
		if (!nestedExact || !projectionExact || !replayExact || !queuedCollisionExact)
			report.m_bLinkedSupportIsolationExact = false;
		if (fixture.m_MapMarkers.ObservedPrematureOwnershipProjection())
			report.m_bLinkedSupportIsolationExact = false;
		report.m_sProductionEvidence = report.m_sProductionEvidence + string.Format(
			" | linked parent/town revisions %1/%2 | support events %3 | premature projection %4 | queued-town collision %5",
			capturedZone.m_iOwnershipRevision,
			linkedTown.m_iOwnershipRevision,
			fixture.m_State.m_aTownInfluenceEvents.Count(),
			fixture.m_MapMarkers.ObservedPrematureOwnershipProjection(),
			queuedCollisionExact);
	}

	protected bool ProveLinkedSupportQueuedTopLevelCollision()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("linked_support_queue_collision");
		HST_ZoneState capturedZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_linked_queue_site",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"16600 20 14200");
		HST_ZoneState linkedTown = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_linked_queue_town",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"16900 20 14200");
		capturedZone.m_aLinkedZoneIds.Insert(linkedTown.m_sZoneId);
		HST_CivilianZoneState civilianZone = m_Factory.AddCivilianZone(
			fixture.m_State,
			linkedTown.m_sZoneId,
			60,
			40);

		HST_OwnershipTransitionRequest parentRequest = fixture.m_Service.BuildRequest(
			fixture.m_State,
			capturedZone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"military_capture",
			"source_proof",
			"linked_support_queue_parent",
			"linked support queue collision parent",
			10,
			"ownership_proof_linked_queue_parent_request");
		parentRequest.m_bApplyEnemyConsequences = false;
		parentRequest.m_bReconcileSecurity = false;
		parentRequest.m_bCreateSecurity = false;
		parentRequest.m_bNotify = false;
		HST_OwnershipTransitionState parentReceipt = fixture.m_Service.AdmitPendingForProof(
			fixture.m_State,
			capturedZone,
			parentRequest);

		HST_OwnershipTransitionRequest queuedRequest = fixture.m_Service.BuildRequest(
			fixture.m_State,
			linkedTown.m_sZoneId,
			fixture.m_Preset.m_sInvaderFactionKey,
			"admin",
			"source_proof",
			"linked_support_queue_later",
			"later pristine town authority waits behind linked support parent",
			0,
			"ownership_proof_linked_queue_later_request");
		queuedRequest.m_bApplyEnemyConsequences = false;
		queuedRequest.m_bReconcileSecurity = false;
		queuedRequest.m_bCreateSecurity = false;
		queuedRequest.m_bNotify = false;
		HST_OwnershipTransitionResult queuedResult = fixture.m_Service.Apply(
			fixture.m_State,
			queuedRequest);
		HST_OwnershipTransitionState queuedReceipt = fixture.m_State.FindOwnershipTransition(
			queuedRequest.m_sRequestId);
		HST_OwnershipTransitionResult parentResult;
		if (parentReceipt)
			parentResult = fixture.m_Service.Apply(fixture.m_State, parentRequest);

		bool parentDeferredExact = parentResult
			&& parentResult.m_bAccepted
			&& parentResult.m_bCompleted
			&& parentReceipt.m_bSupportApplied
			&& parentReceipt.m_aAppliedSupportZoneIds.Contains(linkedTown.m_sZoneId)
			&& queuedResult
			&& queuedResult.m_bAccepted
			&& queuedResult.m_bNeedsRetry
			&& queuedReceipt
			&& !queuedReceipt.m_bOwnerApplied
			&& !queuedReceipt.m_bCompleted
			&& linkedTown.m_sOwnerFactionKey == fixture.m_Preset.m_sOccupierFactionKey
			&& linkedTown.m_sActiveOwnershipTransitionRequestId == queuedReceipt.m_sRequestId
			&& civilianZone.m_iFIASupport == 70
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 1
			&& fixture.m_State.m_aOwnershipTransitions.Count() == 2;

		HST_CampaignSaveData pendingSave = new HST_CampaignSaveData();
		pendingSave.Capture(fixture.m_State);
		HST_OwnershipTransitionSaveValidationService validator = new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(pendingSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = pendingSave.Restore();
		restoredState.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_WON;
		int frozenElapsedSecond = restoredState.m_iElapsedSeconds;
		HST_OwnershipTransitionProofFixture restored = m_Factory.Configure(
			restoredState,
			fixture.m_Preset,
			fixture.m_Balance);
		bool queuedResumeChanged = restored.m_Service.ReconcileAfterRestore(restoredState);
		HST_ZoneState restoredTown = restoredState.FindZone(linkedTown.m_sZoneId);
		HST_OwnershipTransitionState restoredQueued = restoredState.FindOwnershipTransition(
			queuedRequest.m_sRequestId);
		bool queuedCompletedExact = queuedResumeChanged
			&& restoredTown
			&& restoredQueued
			&& restoredQueued.m_bCompleted
			&& !restoredQueued.m_bQuarantined
			&& restoredTown.m_sOwnerFactionKey == fixture.m_Preset.m_sInvaderFactionKey
			&& restoredTown.m_iOwnershipRevision == 2
			&& restoredState.m_aTownInfluenceEvents.Count() == 1
			&& restoredState.m_aOwnershipTransitions.Count() == 2;

		bool politicalChanged = restored.m_Civilians.ReconcileTownOwnershipPolicies(
			restoredState,
			restored.m_Preset,
			true);
		HST_OwnershipTransitionState politicalReceipt;
		if (restoredTown)
			politicalReceipt = restoredState.FindOwnershipTransition(
				restoredTown.m_sLastOwnershipTransitionRequestId);
		bool politicalCompletedExact = politicalChanged
			&& politicalReceipt
			&& politicalReceipt.m_bCompleted
			&& !politicalReceipt.m_bQuarantined
			&& politicalReceipt.m_sCause == "political_support"
			&& restoredTown.m_sOwnerFactionKey == fixture.m_Preset.m_sResistanceFactionKey
			&& restoredTown.m_iOwnershipRevision == 3
			&& restoredState.m_iElapsedSeconds == frozenElapsedSecond
			&& restoredState.m_aTownInfluenceEvents.Count() == 1
			&& restoredState.m_aOwnershipTransitions.Count() == 3;

		HST_CampaignSaveData completedSave = new HST_CampaignSaveData();
		completedSave.Capture(restoredState);
		validator.Normalize(completedSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState secondRestoredState = completedSave.Restore();
		HST_OwnershipTransitionProofFixture secondRestored = m_Factory.Configure(
			secondRestoredState,
			fixture.m_Preset,
			fixture.m_Balance);
		secondRestored.m_Service.ReconcileAfterRestore(secondRestoredState);
		secondRestoredState.m_iElapsedSeconds += 5;
		bool postRestoreChanged = secondRestored.m_Civilians.ReconcileTownOwnershipPolicies(
			secondRestoredState,
			secondRestored.m_Preset,
			true);
		HST_ZoneState secondRestoredTown = secondRestoredState.FindZone(linkedTown.m_sZoneId);
		bool postRestoreInert = !postRestoreChanged
			&& secondRestoredTown
			&& secondRestoredTown.m_sOwnerFactionKey == fixture.m_Preset.m_sResistanceFactionKey
			&& secondRestoredTown.m_iOwnershipRevision == 3
			&& secondRestoredState.m_aTownInfluenceEvents.Count() == 1
			&& secondRestoredState.m_aOwnershipTransitions.Count() == 3;

		bool setupFrozenClockExact = ProveSetupFrozenClockPoliticalReconcile();
		return parentDeferredExact
			&& queuedCompletedExact
			&& politicalCompletedExact
			&& postRestoreInert
			&& setupFrozenClockExact;
	}

	protected bool ProveSetupFrozenClockPoliticalReconcile()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("setup_frozen_policy");
		HST_ZoneState town = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_setup_frozen_policy_town",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"17100 20 14500");
		HST_CivilianZoneState civilianZone = m_Factory.AddCivilianZone(
			fixture.m_State,
			town.m_sZoneId,
			60,
			40);
		fixture.m_State.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_SETUP;
		int frozenSecond = fixture.m_State.m_iElapsedSeconds;
		bool initialChanged = fixture.m_Civilians.ReconcileTownOwnershipPolicies(
			fixture.m_State,
			fixture.m_Preset);
		civilianZone.m_iFIASupport = 70;
		civilianZone.m_iOccupierSupport = 30;
		bool bypassChanged = fixture.m_Civilians.ReconcileTownOwnershipPolicies(
			fixture.m_State,
			fixture.m_Preset,
			true);
		HST_OwnershipTransitionState receipt = fixture.m_State.FindOwnershipTransition(
			town.m_sLastOwnershipTransitionRequestId);
		return !initialChanged
			&& bypassChanged
			&& fixture.m_State.m_iElapsedSeconds == frozenSecond
			&& town.m_sOwnerFactionKey == fixture.m_Preset.m_sResistanceFactionKey
			&& receipt
			&& receipt.m_bCompleted
			&& !receipt.m_bApplyEnemyConsequences
			&& !receipt.m_bNotify;
	}

	protected bool LinkedParentReceiptIsExact(
		bool captured,
		HST_OwnershipTransitionState receipt,
		string linkedTownId)
	{
		if (!captured || !receipt || !receipt.m_bCompleted || !receipt.m_bSupportApplied)
			return false;
		if (receipt.m_aSupportZoneIds.Count() != 1)
			return false;
		if (receipt.m_aAppliedSupportZoneIds.Count() != 1)
			return false;
		return receipt.m_aAppliedSupportZoneIds[0] == linkedTownId;
	}

	protected bool LinkedTownReceiptIsExact(
		HST_OwnershipTransitionProofFixture fixture,
		HST_ZoneState parentZone,
		HST_ZoneState townZone,
		HST_CivilianZoneState civilianZone,
		HST_OwnershipTransitionState parentReceipt,
		HST_OwnershipTransitionState townReceipt)
	{
		if (!fixture || !parentReceipt || !townReceipt || !townReceipt.m_bCompleted)
			return false;
		if (townReceipt.m_sCause != "political_support")
			return false;
		if (townReceipt.m_sProjectionParentRequestId != parentReceipt.m_sRequestId)
			return false;
		if (!townReceipt.m_bDeferredPublicationReleased)
			return false;
		if (parentZone.m_iOwnershipRevision != 2 || townZone.m_iOwnershipRevision != 2)
			return false;
		if (civilianZone.m_iFIASupport != 70)
			return false;
		return fixture.m_State.m_aTownInfluenceEvents.Count() == 1;
	}

	protected bool LinkedMarkersAreExact(
		HST_ZoneState parentZone,
		HST_ZoneState townZone,
		HST_OwnershipTransitionState parentReceipt,
		HST_OwnershipTransitionState townReceipt,
		HST_MapMarkerState parentMarker,
		HST_MapMarkerState townMarker)
	{
		if (!parentZone || !townZone || !parentReceipt || !townReceipt)
			return false;
		if (!parentMarker || !townMarker)
			return false;
		if (parentMarker.m_iSourceRevision != parentZone.m_iOwnershipRevision)
			return false;
		if (townMarker.m_iSourceRevision != townZone.m_iOwnershipRevision)
			return false;
		if (parentReceipt.m_sMarkerId != parentMarker.m_sMarkerId)
			return false;
		return townReceipt.m_sMarkerId == townMarker.m_sMarkerId;
	}

	protected bool LinkedReplayIsExact(
		HST_OwnershipTransitionProofFixture fixture,
		HST_CivilianZoneState civilianZone,
		HST_OwnershipTransitionResult replay,
		int receiptCount,
		int influenceCount,
		int support,
		int persistenceRequests)
	{
		if (!fixture || !civilianZone || !replay || !replay.m_bAlreadyApplied)
			return false;
		if (fixture.m_State.m_aOwnershipTransitions.Count() != receiptCount)
			return false;
		if (fixture.m_State.m_aTownInfluenceEvents.Count() != influenceCount)
			return false;
		if (civilianZone.m_iFIASupport != support)
			return false;
		if (persistenceRequests != 2)
			return false;
		return fixture.m_Persistence.m_iMajorChangeRequests == persistenceRequests;
	}

	protected void ProveMilitaryReplayRecaptureAndIdentity(HST_OwnershipTransitionProofReport report)
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("military");
		vector colocatedPosition = "6200 20 6200";
		HST_ZoneState alpha = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_alpha",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			colocatedPosition);
		HST_ZoneState beta = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_beta",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			colocatedPosition);

		bool captureProjectionExact;
		HST_OwnershipTransitionState captureReceipt = ProveInitialMilitaryCapture(
			fixture,
			alpha,
			report,
			captureProjectionExact);
		ProveReplayConflictAndStaleRequest(fixture, alpha, captureReceipt, report);
		bool aliasReplayExact = ProveAliasRequestReplay();
		bool aliasRestoreExact = ProveLegacyAliasReceiptRestore();
		if (!aliasReplayExact || !aliasRestoreExact)
			report.m_bReplayConflictStaleExact = false;
		report.m_sReplayEvidence = report.m_sReplayEvidence
			+ string.Format(
				" | retired alias rebuilt replay/receipt restore %1/%2",
				aliasReplayExact,
				aliasRestoreExact);
		ProveEnemyRecaptureAndLocationIdentity(fixture, alpha, beta, captureProjectionExact, report);
	}

	protected bool ProveAliasRequestReplay()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("alias_replay");
		HST_ZoneState canonicalZone = m_Factory.AddZone(
			fixture.m_State,
			HST_MaidensBayLocationSaveValidationService.CANONICAL_ZONE_ID,
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"17200 20 14200");
		string requestId = "ownership_proof_retired_alias_replay_request";
		HST_OwnershipTransitionRequest firstRequest = fixture.m_Service.BuildRequest(
			fixture.m_State,
			HST_MaidensBayLocationSaveValidationService.LEGACY_ZONE_ID,
			fixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"retired_alias_replay",
			"retired alias replay canonicalizes before fingerprinting",
			0,
			requestId);
		firstRequest.m_bApplyEnemyConsequences = false;
		HST_OwnershipTransitionResult firstResult = fixture.m_Service.Apply(
			fixture.m_State,
			firstRequest);
		HST_OwnershipTransitionState receipt = fixture.m_State.FindOwnershipTransition(requestId);
		int receiptCount = fixture.m_State.m_aOwnershipTransitions.Count();
		int strategicEventCount = fixture.m_State.m_aStrategicEvents.Count();
		int campaignEventCount = fixture.m_State.m_aCampaignEvents.Count();
		int markerSequence = fixture.m_State.m_iMarkerProjectionSequence;

		HST_OwnershipTransitionRequest replayRequest = fixture.m_Service.BuildRequest(
			fixture.m_State,
			HST_MaidensBayLocationSaveValidationService.LEGACY_ZONE_ID,
			fixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"retired_alias_replay",
			"retired alias replay canonicalizes before fingerprinting",
			0,
			requestId);
		replayRequest.m_bApplyEnemyConsequences = false;
		HST_OwnershipTransitionResult replayResult = fixture.m_Service.Apply(
			fixture.m_State,
			replayRequest);

		if (!firstResult || !firstResult.m_bAccepted || !firstResult.m_bCompleted)
			return false;
		if (firstRequest.m_sZoneId
			!= HST_MaidensBayLocationSaveValidationService.CANONICAL_ZONE_ID)
			return false;
		if (!receipt || receipt.m_sZoneId
			!= HST_MaidensBayLocationSaveValidationService.CANONICAL_ZONE_ID)
			return false;
		bool replayExpectedOwnerExact = replayRequest.m_sExpectedOwnerFactionKey == receipt.m_sExpectedOwnerFactionKey;
		if (replayRequest.m_sZoneId != receipt.m_sZoneId
			|| !replayExpectedOwnerExact
			|| replayRequest.m_iExpectedRevision != receipt.m_iExpectedRevision)
			return false;
		if (!replayResult || !replayResult.m_bAccepted
			|| !replayResult.m_bAlreadyApplied || !replayResult.m_bCompleted
			|| replayResult.m_bStateChanged)
			return false;
		if (fixture.m_State.FindZone(
				HST_MaidensBayLocationSaveValidationService.LEGACY_ZONE_ID) != canonicalZone)
			return false;
		if (fixture.m_State.m_aOwnershipTransitions.Count() != receiptCount
			|| fixture.m_State.m_aStrategicEvents.Count() != strategicEventCount
			|| fixture.m_State.m_aCampaignEvents.Count() != campaignEventCount
			|| fixture.m_State.m_iMarkerProjectionSequence != markerSequence)
			return false;
		return canonicalZone.m_sOwnerFactionKey == fixture.m_Preset.m_sResistanceFactionKey
			&& canonicalZone.m_iOwnershipRevision == 2;
	}

	protected bool ProveLegacyAliasReceiptRestore()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("alias_receipt_restore");
		HST_ZoneState canonicalZone = m_Factory.AddZone(
			fixture.m_State,
			HST_MaidensBayLocationSaveValidationService.CANONICAL_ZONE_ID,
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"17300 20 14200");
		string requestId = "ownership_proof_retired_alias_receipt_restore_request";
		HST_OwnershipTransitionRequest request = fixture.m_Service.BuildRequest(
			fixture.m_State,
			HST_MaidensBayLocationSaveValidationService.LEGACY_ZONE_ID,
			fixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"retired_alias_receipt_restore",
			"current-schema retired alias receipt restores canonically",
			0,
			requestId);
		request.m_bApplyEnemyConsequences = false;
		HST_OwnershipTransitionState receipt = fixture.m_Service.AdmitPendingForProof(
			fixture.m_State,
			canonicalZone,
			request);
		if (!receipt)
			return false;
		receipt.m_sStatus = "applying";
		receipt.m_iAttemptCount = 1;
		receipt.m_iLastAttemptAtSecond = receipt.m_iCreatedAtSecond;

		HST_CampaignSaveData aliasSave = new HST_CampaignSaveData();
		aliasSave.Capture(fixture.m_State);
		HST_OwnershipTransitionState savedReceipt;
		foreach (HST_OwnershipTransitionState candidateReceipt : aliasSave.m_aOwnershipTransitions)
		{
			if (candidateReceipt && candidateReceipt.m_sRequestId == requestId)
				savedReceipt = candidateReceipt;
		}
		HST_StrategicEventState savedStrategicEvent;
		if (savedReceipt)
		{
			foreach (HST_StrategicEventState candidateEvent : aliasSave.m_aStrategicEvents)
			{
				if (candidateEvent
					&& candidateEvent.m_sEventId == savedReceipt.m_sStrategicEventId)
					savedStrategicEvent = candidateEvent;
			}
		}
		if (!savedReceipt || !savedStrategicEvent)
			return false;
		savedReceipt.m_sZoneId = HST_MaidensBayLocationSaveValidationService.LEGACY_ZONE_ID;
		savedStrategicEvent.m_sTargetZoneId
			= HST_MaidensBayLocationSaveValidationService.LEGACY_ZONE_ID;

		HST_OwnershipTransitionSaveValidationService validator
			= new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(aliasSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = aliasSave.Restore();
		HST_OwnershipTransitionState restoredReceipt;
		HST_StrategicEventState restoredStrategicEvent;
		HST_ZoneState restoredZone;
		if (restoredState)
		{
			restoredReceipt = restoredState.FindOwnershipTransition(requestId);
			restoredStrategicEvent = restoredState.FindStrategicEvent(
				savedReceipt.m_sStrategicEventId);
			restoredZone = restoredState.FindZone(
				HST_MaidensBayLocationSaveValidationService.CANONICAL_ZONE_ID);
		}
		return restoredReceipt
			&& !restoredReceipt.m_bQuarantined
			&& restoredReceipt.m_sRequestId == requestId
			&& restoredReceipt.m_sZoneId
				== HST_MaidensBayLocationSaveValidationService.CANONICAL_ZONE_ID
			&& restoredStrategicEvent
			&& restoredStrategicEvent.m_sTargetZoneId
				== HST_MaidensBayLocationSaveValidationService.CANONICAL_ZONE_ID
			&& restoredZone
			&& restoredZone.m_sActiveOwnershipTransitionRequestId == requestId
			&& CountCampaignEvents(
				aliasSave,
				HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID) == 0;
	}

	protected HST_OwnershipTransitionState ProveInitialMilitaryCapture(
		HST_OwnershipTransitionProofFixture fixture,
		HST_ZoneState zone,
		HST_OwnershipTransitionProofReport report,
		out bool projectionExact)
	{
		bool captured = fixture.m_ZoneCapture.CaptureForResistance(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Strategic,
			fixture.m_Economy,
			fixture.m_Balance,
			zone.m_sZoneId,
			0);
		HST_OwnershipTransitionState receipt = fixture.m_State.FindOwnershipTransition(zone.m_sLastOwnershipTransitionRequestId);
		HST_MapMarkerState marker = fixture.m_State.FindMapMarker("hst_zone_" + zone.m_sZoneId);
		projectionExact = MarkerMatchesZoneRevision(marker, zone);
		report.m_bMilitaryCaptureExact = captured;
		if (zone.m_sOwnerFactionKey != fixture.m_Preset.m_sResistanceFactionKey)
			report.m_bMilitaryCaptureExact = false;
		if (zone.m_iOwnershipRevision != 2)
			report.m_bMilitaryCaptureExact = false;
		if (!ReceiptMatchesCause(receipt, "military_capture"))
			report.m_bMilitaryCaptureExact = false;
		return receipt;
	}

	protected void ProveReplayConflictAndStaleRequest(
		HST_OwnershipTransitionProofFixture fixture,
		HST_ZoneState zone,
		HST_OwnershipTransitionState captureReceipt,
		HST_OwnershipTransitionProofReport report)
	{
		if (!captureReceipt)
		{
			report.m_bReplayConflictStaleExact = false;
			report.m_sReplayEvidence = "initial capture receipt missing";
			return;
		}

		int revisionBeforeReplay = zone.m_iOwnershipRevision;
		int receiptCountBeforeReplay = fixture.m_State.m_aOwnershipTransitions.Count();
		int campaignEventCountBeforeReplay = fixture.m_State.m_aCampaignEvents.Count();
		HST_OwnershipTransitionRequest replayRequest = fixture.m_Service.BuildReplayRequestForProof(captureReceipt);
		HST_OwnershipTransitionResult replay = fixture.m_Service.Apply(fixture.m_State, replayRequest);
		HST_OwnershipTransitionRequest conflictRequest = fixture.m_Service.BuildReplayRequestForProof(captureReceipt);
		conflictRequest.m_sReason = conflictRequest.m_sReason + " changed fingerprint";
		HST_OwnershipTransitionResult conflict = fixture.m_Service.Apply(fixture.m_State, conflictRequest);
		HST_OwnershipTransitionRequest stale = fixture.m_Service.BuildRequest(
			fixture.m_State,
			zone.m_sZoneId,
			fixture.m_Preset.m_sOccupierFactionKey,
			"admin",
			"source_proof",
			"stale_revision",
			"stale ownership request",
			0,
			"ownership_proof_stale_request");
		stale.m_sExpectedOwnerFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		stale.m_iExpectedRevision = 1;
		stale.m_bApplyEnemyConsequences = false;
		HST_OwnershipTransitionResult staleResult = fixture.m_Service.Apply(fixture.m_State, stale);
		report.m_bReplayConflictStaleExact = ReplayResultIsExact(replay);
		if (!RejectedWithReason(conflict, "different fingerprint"))
			report.m_bReplayConflictStaleExact = false;
		if (!RejectedWithReason(staleResult, "stale"))
			report.m_bReplayConflictStaleExact = false;
		if (zone.m_iOwnershipRevision != revisionBeforeReplay)
			report.m_bReplayConflictStaleExact = false;
		if (fixture.m_State.m_aOwnershipTransitions.Count() != receiptCountBeforeReplay)
			report.m_bReplayConflictStaleExact = false;
		if (fixture.m_State.m_aCampaignEvents.Count() != campaignEventCountBeforeReplay)
			report.m_bReplayConflictStaleExact = false;

		bool replayAccepted;
		bool replayAlreadyApplied;
		bool replayChanged;
		if (replay)
		{
			replayAccepted = replay.m_bAccepted;
			replayAlreadyApplied = replay.m_bAlreadyApplied;
			replayChanged = replay.m_bStateChanged;
		}
		report.m_sReplayEvidence = string.Format(
			"replay accepted/already/changed %1/%2/%3 | conflict/stale rejected %4/%5",
			replayAccepted,
			replayAlreadyApplied,
			replayChanged,
			RejectedWithReason(conflict, "different fingerprint"),
			RejectedWithReason(staleResult, "stale"));
		report.m_sReplayEvidence = report.m_sReplayEvidence + string.Format(
			" | revision %1",
			zone.m_iOwnershipRevision);
	}

	protected void ProveEnemyRecaptureAndLocationIdentity(
		HST_OwnershipTransitionProofFixture fixture,
		HST_ZoneState zone,
		HST_ZoneState colocatedZone,
		bool captureProjectionExact,
		HST_OwnershipTransitionProofReport report)
	{

		bool enemyRecaptured = fixture.m_ZoneCapture.CaptureZone(
			fixture.m_State,
			fixture.m_Strategic,
			fixture.m_Economy,
			fixture.m_Balance,
			zone.m_sZoneId,
			fixture.m_Preset.m_sOccupierFactionKey,
			fixture.m_Preset.m_sResistanceFactionKey);
		HST_OwnershipTransitionState recaptureReceipt = fixture.m_State.FindOwnershipTransition(zone.m_sLastOwnershipTransitionRequestId);
		HST_MapMarkerState recaptureMarker = fixture.m_State.FindMapMarker("hst_zone_" + zone.m_sZoneId);
		HST_MapMarkerState colocatedMarker = fixture.m_State.FindMapMarker("hst_zone_" + colocatedZone.m_sZoneId);
		report.m_bEnemyRecaptureExact = enemyRecaptured;
		if (zone.m_sOwnerFactionKey != fixture.m_Preset.m_sOccupierFactionKey)
			report.m_bEnemyRecaptureExact = false;
		if (zone.m_iOwnershipRevision != 3)
			report.m_bEnemyRecaptureExact = false;
		if (!ReceiptHasRevision(recaptureReceipt, 2, 3))
			report.m_bEnemyRecaptureExact = false;

		report.m_bProjectionSourceRevisionExact = captureProjectionExact;
		if (!MarkerMatchesReceipt(recaptureMarker, recaptureReceipt, 3))
			report.m_bProjectionSourceRevisionExact = false;

		report.m_bColocatedIdentityExact = ColocatedZoneRemainedIndependent(
			fixture,
			colocatedZone,
			recaptureMarker,
			colocatedMarker);

		string receiptOwner;
		int receiptRevision;
		if (recaptureReceipt)
		{
			receiptOwner = recaptureReceipt.m_sNewOwnerFactionKey;
			receiptRevision = recaptureReceipt.m_iAppliedRevision;
		}
		report.m_sProductionEvidence = string.Format(
			"military owner/revision %1/%2 | recapture owner/revision %3/%4 | receipts %5",
			zone.m_sOwnerFactionKey,
			zone.m_iOwnershipRevision,
			receiptOwner,
			receiptRevision,
			fixture.m_State.m_aOwnershipTransitions.Count());
	}

	protected bool MarkerMatchesZoneRevision(HST_MapMarkerState marker, HST_ZoneState zone)
	{
		if (!marker || !zone)
			return false;
		if (marker.m_sOwnerFactionKey != zone.m_sOwnerFactionKey)
			return false;
		return marker.m_iSourceRevision == zone.m_iOwnershipRevision;
	}

	protected bool ReplayResultIsExact(HST_OwnershipTransitionResult result)
	{
		if (!result || !result.m_bAccepted)
			return false;
		if (!result.m_bAlreadyApplied || !result.m_bCompleted)
			return false;
		return !result.m_bStateChanged;
	}

	protected bool RejectedWithReason(HST_OwnershipTransitionResult result, string reasonFragment)
	{
		if (!result || result.m_bAccepted)
			return false;
		return result.m_sFailureReason.Contains(reasonFragment);
	}

	protected bool ReceiptHasRevision(
		HST_OwnershipTransitionState receipt,
		int expectedRevision,
		int appliedRevision)
	{
		if (!receipt || !receipt.m_bCompleted)
			return false;
		if (receipt.m_iExpectedRevision != expectedRevision)
			return false;
		return receipt.m_iAppliedRevision == appliedRevision;
	}

	protected bool MarkerMatchesReceipt(
		HST_MapMarkerState marker,
		HST_OwnershipTransitionState receipt,
		int sourceRevision)
	{
		if (!marker || !receipt)
			return false;
		if (marker.m_iSourceRevision != sourceRevision)
			return false;
		if (receipt.m_sMarkerId != marker.m_sMarkerId)
			return false;
		return receipt.m_iMarkerRevision == marker.m_iRevision;
	}

	protected bool ColocatedZoneRemainedIndependent(
		HST_OwnershipTransitionProofFixture fixture,
		HST_ZoneState zone,
		HST_MapMarkerState changedMarker,
		HST_MapMarkerState unchangedMarker)
	{
		if (!fixture || !zone || !changedMarker || !unchangedMarker)
			return false;
		if (zone.m_sOwnerFactionKey != fixture.m_Preset.m_sOccupierFactionKey)
			return false;
		if (zone.m_iOwnershipRevision != 1 || !zone.m_sLastOwnershipTransitionRequestId.IsEmpty())
			return false;
		if (changedMarker.m_sMarkerId == unchangedMarker.m_sMarkerId)
			return false;
		if (changedMarker.m_vPosition != unchangedMarker.m_vPosition)
			return false;
		return unchangedMarker.m_iSourceRevision == 1;
	}

	protected void ProvePoliticalEntryPoint(HST_OwnershipTransitionProofReport report)
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("political");
		HST_ZoneState town = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_town",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"7100 18 7100");
		HST_CivilianZoneState civilianZone = m_Factory.AddCivilianZone(fixture.m_State, town.m_sZoneId, 60, 40);

		HST_OwnershipTransitionState upReceipt;
		bool aidExact = ProvePoliticalAidAndReplay(fixture, town, upReceipt);
		HST_OwnershipTransitionState downReceipt;
		bool pressureExact = ProvePoliticalPressure(fixture, town, downReceipt);
		report.m_bPoliticalFlipExact = aidExact;
		if (!pressureExact)
			report.m_bPoliticalFlipExact = false;
		if (town.m_sOwnerFactionKey != fixture.m_Preset.m_sOccupierFactionKey)
			report.m_bPoliticalFlipExact = false;
		if (town.m_iOwnershipRevision != 3)
			report.m_bPoliticalFlipExact = false;
		if (civilianZone.m_iFIASupport != 70 || civilianZone.m_iOccupierSupport != 100)
			report.m_bPoliticalFlipExact = false;

		if (!ReceiptHasRevision(downReceipt, 2, 3))
			report.m_bEnemyRecaptureExact = false;

		int upRevision;
		int downRevision;
		if (upReceipt)
			upRevision = upReceipt.m_iAppliedRevision;
		if (downReceipt)
			downRevision = downReceipt.m_iAppliedRevision;
		report.m_sProductionEvidence = report.m_sProductionEvidence + string.Format(
			" | political revisions %1/%2 | influence %3/%4",
			upRevision,
			downRevision,
			civilianZone.m_iFIASupport,
			civilianZone.m_iOccupierSupport);
	}

	protected bool ProvePoliticalAidAndReplay(
		HST_OwnershipTransitionProofFixture fixture,
		HST_ZoneState town,
		out HST_OwnershipTransitionState receipt)
	{
		bool aidApplied = fixture.m_Civilians.RegisterInfluenceEventExact(
			fixture.m_State,
			town.m_sZoneId,
			"source_proof_aid",
			10,
			0,
			0,
			0,
			0,
			0,
			0,
			"source proof political aid",
			fixture.m_Preset,
			0,
			"ownership_proof_aid_source",
			"ownership_proof_aid_event");
		receipt = fixture.m_State.FindOwnershipTransition(town.m_sLastOwnershipTransitionRequestId);
		int transitionCountAfterAid = fixture.m_State.m_aOwnershipTransitions.Count();
		bool exactAidReplay = fixture.m_Civilians.RegisterInfluenceEventExact(
			fixture.m_State,
			town.m_sZoneId,
			"source_proof_aid",
			10,
			0,
			0,
			0,
			0,
			0,
			0,
			"source proof political aid",
			fixture.m_Preset,
			0,
			"ownership_proof_aid_source",
			"ownership_proof_aid_event");
		if (!aidApplied || !exactAidReplay)
			return false;
		if (fixture.m_State.m_aOwnershipTransitions.Count() != transitionCountAfterAid)
			return false;
		if (!receipt || !receipt.m_bCompleted)
			return false;
		if (receipt.m_sCause != "political_support")
			return false;
		return receipt.m_sNewOwnerFactionKey == fixture.m_Preset.m_sResistanceFactionKey;
	}

	protected bool ProvePoliticalPressure(
		HST_OwnershipTransitionProofFixture fixture,
		HST_ZoneState town,
		out HST_OwnershipTransitionState receipt)
	{
		bool pressureApplied = fixture.m_Civilians.RegisterInfluenceEventExact(
			fixture.m_State,
			town.m_sZoneId,
			"source_proof_pressure",
			0,
			70,
			0,
			20,
			0,
			0,
			0,
			"source proof political pressure",
			fixture.m_Preset,
			0,
			"ownership_proof_pressure_source",
			"ownership_proof_pressure_event");
		receipt = fixture.m_State.FindOwnershipTransition(town.m_sLastOwnershipTransitionRequestId);
		if (!pressureApplied || !receipt || !receipt.m_bCompleted)
			return false;
		if (receipt.m_sCause != "political_support")
			return false;
		return receipt.m_sNewOwnerFactionKey == fixture.m_Preset.m_sOccupierFactionKey;
	}

	protected void ProvePublicationFence(HST_OwnershipTransitionProofReport report)
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("publication_fence");
		HST_ZoneState parentZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_fence_parent",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"14200 20 14200");
		HST_ZoneState childZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_fence_child",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"14500 20 14200");

		HST_OwnershipTransitionState parent = new HST_OwnershipTransitionState();
		parent.m_iContractVersion = HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION;
		parent.m_sRequestId = "ownership_proof_fence_parent_request";
		parent.m_sZoneId = parentZone.m_sZoneId;
		parent.m_sExpectedOwnerFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		parent.m_sPreviousOwnerFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		parent.m_sNewOwnerFactionKey = fixture.m_Preset.m_sResistanceFactionKey;
		parent.m_iExpectedRevision = 1;
		parent.m_iAppliedRevision = 2;
		parent.m_bOwnerApplied = true;
		parent.m_sStatus = "projecting";
		parentZone.m_sOwnerFactionKey = fixture.m_Preset.m_sResistanceFactionKey;
		parentZone.m_iOwnershipRevision = 2;
		parentZone.m_sActiveOwnershipTransitionRequestId = parent.m_sRequestId;
		fixture.m_State.m_aOwnershipTransitions.Insert(parent);

		HST_OwnershipTransitionState child = new HST_OwnershipTransitionState();
		child.m_iContractVersion = HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION;
		child.m_sRequestId = "ownership_proof_fence_child_request";
		child.m_sZoneId = childZone.m_sZoneId;
		child.m_sExpectedOwnerFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		child.m_sPreviousOwnerFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		child.m_sNewOwnerFactionKey = fixture.m_Preset.m_sResistanceFactionKey;
		child.m_iExpectedRevision = 1;
		child.m_iAppliedRevision = 2;
		child.m_bOwnerApplied = true;
		child.m_bCompleted = true;
		child.m_sStatus = "completed";
		child.m_sProjectionParentRequestId = parent.m_sRequestId;
		childZone.m_sOwnerFactionKey = fixture.m_Preset.m_sResistanceFactionKey;
		childZone.m_iOwnershipRevision = 2;
		childZone.m_sLastOwnershipTransitionRequestId = child.m_sRequestId;
		fixture.m_State.m_aOwnershipTransitions.Insert(child);
		InsertProjectionProofMarker(
			fixture.m_State,
			parentZone,
			fixture.m_Preset.m_sOccupierFactionKey,
			1);
		InsertProjectionProofMarker(
			fixture.m_State,
			childZone,
			fixture.m_Preset.m_sOccupierFactionKey,
			1);

		bool ordinarySnapshotBlocked = !fixture.m_MapMarkers.CanPublishOwnershipSnapshotForProof(
			fixture.m_State,
			"");
		bool ordinaryRebuildBlocked = !fixture.m_MapMarkers.RebuildAllMarkers(
			fixture.m_State,
			fixture.m_Preset);
		bool priorSnapshotRetained = ProjectionProofMarkerMatches(
			fixture.m_State,
			parentZone.m_sZoneId,
			fixture.m_Preset.m_sOccupierFactionKey,
			1)
			&& ProjectionProofMarkerMatches(
				fixture.m_State,
				childZone.m_sZoneId,
				fixture.m_Preset.m_sOccupierFactionKey,
				1);
		bool commandOldSnapshotExact = CommandUIPublicationMatches(
			fixture,
			fixture.m_Preset.m_sOccupierFactionKey,
			0,
			2,
			"Hostile");
		string projectedOwner;
		int projectedRevision;
		bool parentOldResolved = fixture.m_MapMarkers.ResolveZoneProjectionAuthorityForProof(
			fixture.m_State,
			parentZone,
			"",
			projectedOwner,
			projectedRevision);
		bool parentOldExact = parentOldResolved
			&& projectedOwner == fixture.m_Preset.m_sOccupierFactionKey
			&& projectedRevision == 1;
		bool childOldResolved = fixture.m_MapMarkers.ResolveZoneProjectionAuthorityForProof(
			fixture.m_State,
			childZone,
			"",
			projectedOwner,
			projectedRevision);
		bool childOldExact = childOldResolved
			&& projectedOwner == fixture.m_Preset.m_sOccupierFactionKey
			&& projectedRevision == 1;

		bool parentSnapshotAuthorized = fixture.m_MapMarkers.CanPublishOwnershipSnapshotForProof(
			fixture.m_State,
			parent.m_sRequestId);
		bool parentNewResolved = fixture.m_MapMarkers.ResolveZoneProjectionAuthorityForProof(
			fixture.m_State,
			parentZone,
			parent.m_sRequestId,
			projectedOwner,
			projectedRevision);
		bool parentNewExact = parentNewResolved
			&& projectedOwner == fixture.m_Preset.m_sResistanceFactionKey
			&& projectedRevision == 2;
		bool childNewResolved = fixture.m_MapMarkers.ResolveZoneProjectionAuthorityForProof(
			fixture.m_State,
			childZone,
			parent.m_sRequestId,
			projectedOwner,
			projectedRevision);
		bool childNewExact = childNewResolved
			&& projectedOwner == fixture.m_Preset.m_sResistanceFactionKey
			&& projectedRevision == 2;
		HST_OwnershipMarkerProjectionTransaction authorizedTransaction
			= fixture.m_MapMarkers.StageOwnershipTransitionMarkers(
				fixture.m_State,
				fixture.m_Preset,
				parent.m_sRequestId);
		bool authorizedStaged = authorizedTransaction
			&& ProjectionProofMarkerMatches(
				fixture.m_State,
				parentZone.m_sZoneId,
				fixture.m_Preset.m_sResistanceFactionKey,
				2)
			&& ProjectionProofMarkerMatches(
				fixture.m_State,
				childZone.m_sZoneId,
				fixture.m_Preset.m_sResistanceFactionKey,
				2);
		if (authorizedTransaction)
			fixture.m_MapMarkers.RollbackOwnershipTransitionMarkers(
				fixture.m_State,
				authorizedTransaction);
		bool authorizedSnapshotExact = authorizedStaged
			&& ProjectionProofMarkerMatches(
				fixture.m_State,
				parentZone.m_sZoneId,
				fixture.m_Preset.m_sOccupierFactionKey,
				1)
			&& ProjectionProofMarkerMatches(
				fixture.m_State,
				childZone.m_sZoneId,
				fixture.m_Preset.m_sOccupierFactionKey,
				1);
		bool quarantinedFenceExact = ProveQuarantinedPublicationFence();
		bool nestedRestoreQuarantineExact = ProveNestedRestoreAndQuarantine();
		bool deferredChildAtomicExact = ProveDeferredChildPublicationAtomicity();
		bool stagedRollbackExact = ProveStagedMarkerRollback();
		bool preProjectionLeakExact = ProvePreProjectionMarkerLeakFailClosed();
		bool setupHistoryExact = ProveSetupProjectionHistoryAcrossActivation();

		report.m_bPublicationFenceExact = ordinarySnapshotBlocked
			&& ordinaryRebuildBlocked
			&& priorSnapshotRetained
			&& commandOldSnapshotExact
			&& parentOldExact
			&& childOldExact
			&& parentSnapshotAuthorized
			&& parentNewExact
			&& childNewExact
			&& authorizedSnapshotExact
			&& quarantinedFenceExact
			&& nestedRestoreQuarantineExact
			&& deferredChildAtomicExact
			&& stagedRollbackExact
			&& preProjectionLeakExact
			&& setupHistoryExact;
		string publicationEvidence = string.Format(
				" | publication fence ordinary %1/%2 retained %3 | parent old/new %4/%5 | child old/new %6/%7 | authorized %8/%9",
				ordinarySnapshotBlocked,
				ordinaryRebuildBlocked,
				priorSnapshotRetained,
				parentOldExact,
				parentNewExact,
				childOldExact,
				childNewExact,
				parentSnapshotAuthorized,
				authorizedSnapshotExact);
		report.m_sRestoreProjectionEvidence = report.m_sRestoreProjectionEvidence
			+ publicationEvidence
			+ string.Format(
				" | quarantine %1 | nested restore/quarantine %2 | deferred-child atomic %3 | staged rollback %4 | preprojection leak %5 | setup history %6 | command prior %7",
				quarantinedFenceExact,
				nestedRestoreQuarantineExact,
				deferredChildAtomicExact,
				stagedRollbackExact,
				preProjectionLeakExact,
				setupHistoryExact,
				commandOldSnapshotExact);
	}

	protected bool ProveQuarantinedPublicationFence()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("publication_quarantine");
		HST_ZoneState zone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_fence_quarantine",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"14800 20 14200");
		InsertProjectionProofMarker(
			fixture.m_State,
			zone,
			fixture.m_Preset.m_sOccupierFactionKey,
			1);
		HST_OwnershipTransitionState transition = new HST_OwnershipTransitionState();
		transition.m_iContractVersion = HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION;
		transition.m_sRequestId = "ownership_proof_fence_quarantine_request";
		transition.m_sZoneId = zone.m_sZoneId;
		transition.m_sPreviousOwnerFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		transition.m_sNewOwnerFactionKey = fixture.m_Preset.m_sResistanceFactionKey;
		transition.m_iExpectedRevision = 1;
		transition.m_iAppliedRevision = 2;
		transition.m_bOwnerApplied = true;
		transition.m_bQuarantined = true;
		transition.m_sStatus = "quarantined";
		zone.m_sOwnerFactionKey = fixture.m_Preset.m_sResistanceFactionKey;
		zone.m_iOwnershipRevision = 2;
		zone.m_iOwnershipContractVersion = HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION;
		zone.m_sActiveOwnershipTransitionRequestId = transition.m_sRequestId;
		fixture.m_State.m_aOwnershipTransitions.Insert(transition);

		bool blocked = !fixture.m_MapMarkers.RebuildAllMarkers(fixture.m_State, fixture.m_Preset);
		bool retainedExact = ProjectionProofMarkerMatches(
			fixture.m_State,
			zone.m_sZoneId,
			fixture.m_Preset.m_sOccupierFactionKey,
			1);
		fixture.m_State.m_aMapMarkers.Clear();
		string projectedOwnerFactionKey;
		int projectedOwnershipRevision;
		bool sourceRejected = !fixture.m_MapMarkers.ResolveZoneProjectionAuthorityForProof(
			fixture.m_State,
			zone,
			"",
			projectedOwnerFactionKey,
			projectedOwnershipRevision)
			&& projectedOwnerFactionKey.IsEmpty()
			&& projectedOwnershipRevision == 0;
		bool commandUnavailable = CommandUIPublicationUnavailable(fixture);
		return blocked && retainedExact && sourceRejected && commandUnavailable;
	}

	protected bool ProveNestedRestoreAndQuarantine()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("nested_restore");
		HST_ZoneState parentZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_nested_restore_parent",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"15700 20 14200");
		HST_ZoneState childZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_nested_restore_child",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"16000 20 14200");
		parentZone.m_aLinkedZoneIds.Insert(childZone.m_sZoneId);
		m_Factory.AddCivilianZone(fixture.m_State, childZone.m_sZoneId, 60, 40);
		fixture.m_MapMarkers.SetProjectionBlocked(true);
		bool captureCompleted = fixture.m_ZoneCapture.CaptureForResistance(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Strategic,
			fixture.m_Economy,
			fixture.m_Balance,
			parentZone.m_sZoneId,
			10);
		HST_OwnershipTransitionState parent = fixture.m_State.FindOwnershipTransition(
			parentZone.m_sActiveOwnershipTransitionRequestId);
		HST_OwnershipTransitionState child = fixture.m_State.FindOwnershipTransition(
			childZone.m_sLastOwnershipTransitionRequestId);
		if (captureCompleted || !parent || !child || parent.m_bCompleted
			|| !child.m_bCompleted || child.m_bDeferredPublicationReleased)
			return false;

		HST_CampaignSaveData restartSave = new HST_CampaignSaveData();
		restartSave.Capture(fixture.m_State);
		HST_CampaignState restoredState = restartSave.Restore();
		HST_OwnershipTransitionProofFixture restored = m_Factory.Configure(
			restoredState,
			fixture.m_Preset,
			fixture.m_Balance);
		bool reconciled = restored.m_Service.ReconcileAfterRestore(restoredState);
		HST_OwnershipTransitionState restoredParent = restoredState.FindOwnershipTransition(parent.m_sRequestId);
		HST_OwnershipTransitionState restoredChild = restoredState.FindOwnershipTransition(child.m_sRequestId);
		HST_ZoneState restoredChildZone = restoredState.FindZone(childZone.m_sZoneId);
		bool restoreExact = reconciled
			&& restoredParent
			&& restoredParent.m_bCompleted
			&& restoredChild
			&& restoredChild.m_bDeferredPublicationReleased
			&& restoredChildZone
			&& ProjectionProofMarkerMatches(
				restoredState,
				restoredChildZone.m_sZoneId,
				restoredChildZone.m_sOwnerFactionKey,
				restoredChildZone.m_iOwnershipRevision);

		HST_CampaignSaveData corruptSave = new HST_CampaignSaveData();
		corruptSave.Capture(fixture.m_State);
		HST_OwnershipTransitionState corruptParent;
		HST_OwnershipTransitionState corruptChild;
		foreach (HST_OwnershipTransitionState candidate : corruptSave.m_aOwnershipTransitions)
		{
			if (!candidate)
				continue;
			if (candidate.m_sRequestId == parent.m_sRequestId)
				corruptParent = candidate;
			else if (candidate.m_sRequestId == child.m_sRequestId)
				corruptChild = candidate;
		}
		HST_ZoneState corruptParentZone = null;
		HST_ZoneState corruptChildZone = null;
		foreach (HST_ZoneState savedZone : corruptSave.m_aZones)
		{
			if (!savedZone)
				continue;
			if (savedZone.m_sZoneId == parentZone.m_sZoneId)
				corruptParentZone = savedZone;
			else if (savedZone.m_sZoneId == childZone.m_sZoneId)
				corruptChildZone = savedZone;
		}
		if (!corruptParent || !corruptChild || !corruptParentZone || !corruptChildZone)
			return false;
		corruptSave.m_aOwnershipTransitions.Clear();
		corruptSave.m_aOwnershipTransitions.Insert(corruptChild);
		corruptSave.m_aOwnershipTransitions.Insert(corruptParent);
		corruptParentZone.m_sActiveOwnershipTransitionRequestId
			= "ownership_proof_missing_parent_active_backlink";
		HST_OwnershipTransitionSaveValidationService validator = new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(corruptSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		bool quarantineExact = corruptParent.m_bQuarantined
			&& corruptChild.m_bQuarantined
			&& corruptParentZone
			&& corruptChildZone
			&& corruptParentZone.m_iOwnershipContractVersion
				== HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION
			&& corruptChildZone.m_iOwnershipContractVersion
				== HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION
			&& corruptParent.m_sFailureReason.Contains("unique active authority")
			&& corruptChild.m_sFailureReason.Contains("live parent");
		return restoreExact && quarantineExact;
	}

	protected bool ProveDeferredChildPublicationAtomicity()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("deferred_atomic");
		HST_ZoneState parentZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_atomic_parent",
			fixture.m_Preset.m_sResistanceFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"16300 20 14200");
		HST_ZoneState firstZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_atomic_child_first",
			fixture.m_Preset.m_sResistanceFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"16600 20 14200");
		HST_ZoneState secondZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_atomic_child_second",
			fixture.m_Preset.m_sResistanceFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"16900 20 14200");

		HST_OwnershipTransitionState parent = new HST_OwnershipTransitionState();
		parent.m_iContractVersion = HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION;
		parent.m_sRequestId = "ownership_proof_atomic_parent_request";
		parent.m_sZoneId = parentZone.m_sZoneId;
		parent.m_sExpectedOwnerFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		parent.m_sPreviousOwnerFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		parent.m_sNewOwnerFactionKey = fixture.m_Preset.m_sResistanceFactionKey;
		parent.m_iExpectedRevision = 1;
		parent.m_iAppliedRevision = 2;
		parent.m_bOwnerApplied = true;
		parent.m_sStatus = "projecting";
		parentZone.m_iOwnershipRevision = 2;
		parentZone.m_sActiveOwnershipTransitionRequestId = parent.m_sRequestId;
		fixture.m_State.m_aOwnershipTransitions.Insert(parent);

		HST_OwnershipTransitionState first = AddDeferredChildForProof(
			fixture,
			firstZone,
			parent,
			"ownership_proof_atomic_child_first_request");
		HST_OwnershipTransitionState second = AddDeferredChildForProof(
			fixture,
			secondZone,
			parent,
			"ownership_proof_atomic_child_second_request");
		InsertProjectionProofMarker(
			fixture.m_State,
			firstZone,
			fixture.m_Preset.m_sResistanceFactionKey,
			2);

		int notificationsBefore = fixture.m_ZoneCapture.PendingNotificationCountForProof();
		string blockedFailure = fixture.m_Service.ReleaseDeferredChildPublicationsForProof(
			fixture.m_State,
			parent,
			true);
		bool blockedAtomic = !blockedFailure.IsEmpty()
			&& !first.m_bDeferredPublicationReleased
			&& !second.m_bDeferredPublicationReleased
			&& first.m_sProjectionDecision.IndexOf("publication released") < 0
			&& second.m_sProjectionDecision.IndexOf("publication released") < 0
			&& fixture.m_ZoneCapture.PendingNotificationCountForProof() == notificationsBefore;

		InsertProjectionProofMarker(
			fixture.m_State,
			secondZone,
			fixture.m_Preset.m_sResistanceFactionKey,
			2);
		string releasedFailure = fixture.m_Service.ReleaseDeferredChildPublicationsForProof(
			fixture.m_State,
			parent,
			true);
		bool releasedTogether = releasedFailure.IsEmpty()
			&& first.m_bDeferredPublicationReleased
			&& second.m_bDeferredPublicationReleased
			&& fixture.m_ZoneCapture.PendingNotificationCountForProof()
				== notificationsBefore + 2;
		return blockedAtomic && releasedTogether;
	}

	protected bool ProveStagedMarkerRollback()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("staged_marker_rollback");
		HST_ZoneState parentZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_staged_parent_zone",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"17300 20 14800");
		HST_ZoneState childZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_staged_child_zone",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"17600 20 14800");
		parentZone.m_aLinkedZoneIds.Insert(childZone.m_sZoneId);
		m_Factory.AddCivilianZone(fixture.m_State, childZone.m_sZoneId, 60, 40);
		InsertProjectionProofMarker(
			fixture.m_State,
			parentZone,
			fixture.m_Preset.m_sOccupierFactionKey,
			1);
		InsertProjectionProofMarker(
			fixture.m_State,
			childZone,
			fixture.m_Preset.m_sOccupierFactionKey,
			1);
		HST_MapMarkerState unrelatedTombstone = new HST_MapMarkerState();
		unrelatedTombstone.m_sMarkerId = "ownership_proof_unrelated_tombstone";
		unrelatedTombstone.m_sLinkedId = "ownership_proof_unrelated";
		unrelatedTombstone.m_sLabel = "unrelated retained tombstone";
		unrelatedTombstone.m_sCallsign = "ROLLBACK";
		unrelatedTombstone.m_sCategory = "proof";
		unrelatedTombstone.m_sOwnerFactionKey = fixture.m_Preset.m_sInvaderFactionKey;
		unrelatedTombstone.m_sIconHint = "POINT";
		unrelatedTombstone.m_sColorHint = "red";
		unrelatedTombstone.m_sTextColorHint = "white";
		unrelatedTombstone.m_sStyleHint = "proof_tombstone";
		unrelatedTombstone.m_vPosition = "18000 20 14800";
		unrelatedTombstone.m_bVisible = false;
		unrelatedTombstone.m_bRuntimeNative = true;
		unrelatedTombstone.m_iRevision = 7;
		unrelatedTombstone.m_iSourceRevision = 4;
		unrelatedTombstone.m_iStreamSequence = 19;
		unrelatedTombstone.m_bTombstone = true;
		unrelatedTombstone.m_iTombstonedAtSecond = 617;
		fixture.m_State.m_aMapMarkers.Insert(unrelatedTombstone);
		fixture.m_State.m_iMarkerProjectionEpoch = 7;
		fixture.m_State.m_iMarkerProjectionSequence = 19;
		array<ref HST_MapMarkerState> previousMarkerSnapshot = {};
		foreach (HST_MapMarkerState previousMarker : fixture.m_State.m_aMapMarkers)
		{
			if (previousMarker)
				previousMarkerSnapshot.Insert(HST_MarkerProjectionCodec.CopyMarker(previousMarker));
		}
		int previousMarkerRecordCount = fixture.m_State.m_aMapMarkers.Count();
		int previousProjectionEpoch = fixture.m_State.m_iMarkerProjectionEpoch;
		int previousProjectionSequence = fixture.m_State.m_iMarkerProjectionSequence;
		fixture.m_MapMarkers.RemoveNextStagedZoneMarkerForProof(childZone.m_sZoneId);
		bool firstCaptureCompleted = fixture.m_ZoneCapture.CaptureForResistance(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Strategic,
			fixture.m_Economy,
			fixture.m_Balance,
			parentZone.m_sZoneId,
			10);
		HST_OwnershipTransitionState parent = fixture.m_State.FindOwnershipTransition(
			parentZone.m_sActiveOwnershipTransitionRequestId);
		HST_OwnershipTransitionState child = fixture.m_State.FindOwnershipTransition(
			childZone.m_sLastOwnershipTransitionRequestId);
		bool rollbackExact = !firstCaptureCompleted
			&& parent
			&& parent.m_bOwnerApplied
			&& !parent.m_bProjectionRequested
			&& parent.m_sMarkerId.IsEmpty()
			&& child
			&& child.m_bCompleted
			&& !child.m_bDeferredPublicationReleased
			&& child.m_sMarkerId.IsEmpty()
			&& ProjectionProofMarkerMatches(
				fixture.m_State,
				parentZone.m_sZoneId,
				fixture.m_Preset.m_sOccupierFactionKey,
				1)
			&& ProjectionProofMarkerMatches(
				fixture.m_State,
				childZone.m_sZoneId,
				fixture.m_Preset.m_sOccupierFactionKey,
				1)
			&& fixture.m_State.m_iMarkerProjectionEpoch == previousProjectionEpoch
			&& fixture.m_State.m_iMarkerProjectionSequence == previousProjectionSequence
			&& fixture.m_State.m_aMapMarkers.Count() == previousMarkerRecordCount
			&& MarkerSnapshotsTransportEqual(
				previousMarkerSnapshot,
				fixture.m_State.m_aMapMarkers)
			&& fixture.m_ZoneCapture.PendingNotificationCountForProof() == 0;

		HST_OwnershipTransitionResult retry;
		if (parent)
			retry = fixture.m_Service.Apply(
				fixture.m_State,
				fixture.m_Service.BuildReplayRequestForProof(parent));
		bool committedExact = retry
			&& retry.m_bCompleted
			&& parent.m_bProjectionRequested
			&& child.m_bDeferredPublicationReleased
			&& ProjectionProofMarkerMatches(
				fixture.m_State,
				parentZone.m_sZoneId,
				fixture.m_Preset.m_sResistanceFactionKey,
				2)
			&& ProjectionProofMarkerMatches(
				fixture.m_State,
				childZone.m_sZoneId,
				fixture.m_Preset.m_sResistanceFactionKey,
				2)
			&& fixture.m_ZoneCapture.PendingNotificationCountForProof() == 2;
		return rollbackExact && committedExact;
	}

	protected bool MarkerSnapshotsTransportEqual(
		array<ref HST_MapMarkerState> expected,
		array<ref HST_MapMarkerState> actual)
	{
		if (!expected || !actual || expected.Count() != actual.Count())
			return false;
		for (int markerIndex; markerIndex < expected.Count(); markerIndex++)
		{
			if (!HST_MarkerProjectionCodec.TransportEquals(
					expected[markerIndex],
					actual[markerIndex]))
				return false;
		}
		return true;
	}

	protected bool ProvePreProjectionMarkerLeakFailClosed()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build(
			"preprojection_marker_leak");
		HST_ZoneState zone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_preprojection_marker_zone",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"18100 20 14800");
		InsertProjectionProofMarker(
			fixture.m_State,
			zone,
			fixture.m_Preset.m_sOccupierFactionKey,
			1);
		fixture.m_MapMarkers.SetProjectionBlocked(true);
		HST_OwnershipTransitionRequest request = fixture.m_Service.BuildRequest(
			fixture.m_State,
			zone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"preprojection_marker_leak",
			"preprojection marker leak proof",
			0,
			"ownership_proof_preprojection_marker_request");
		request.m_bApplyEnemyConsequences = false;
		HST_OwnershipTransitionResult interrupted = fixture.m_Service.Apply(
			fixture.m_State,
			request);
		HST_OwnershipTransitionState receipt = fixture.m_State.FindOwnershipTransition(
			request.m_sRequestId);
		HST_MapMarkerState leakedMarker = fixture.m_State.FindMapMarker(
			"hst_zone_" + zone.m_sZoneId);
		if (!interrupted || interrupted.m_bCompleted || !receipt || !receipt.m_bOwnerApplied
			|| receipt.m_bProjectionRequested || !leakedMarker)
			return false;
		leakedMarker.m_sOwnerFactionKey = receipt.m_sNewOwnerFactionKey;
		leakedMarker.m_iSourceRevision = receipt.m_iAppliedRevision;

		string resolvedOwnerFactionKey;
		int resolvedOwnershipRevision;
		bool priorAuthorityExact = fixture.m_MapMarkers.ResolvePublishedZoneOwnership(
			fixture.m_State,
			zone,
			resolvedOwnerFactionKey,
			resolvedOwnershipRevision)
			&& resolvedOwnerFactionKey == receipt.m_sPreviousOwnerFactionKey
			&& resolvedOwnershipRevision == receipt.m_iExpectedRevision;
		bool commandUnavailableBeforeRestore = CommandUIPublicationUnavailable(fixture);
		string captureBeforeRestore = fixture.m_ZoneCapture.BuildCaptureReport(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Balance,
			fixture.m_MapMarkers);
		bool captureUnavailableBeforeRestore = captureBeforeRestore.Contains(
			"owner publication unavailable");

		HST_CampaignSaveData leakedSave = new HST_CampaignSaveData();
		leakedSave.Capture(fixture.m_State);
		HST_OwnershipTransitionSaveValidationService validator
			= new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(leakedSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = leakedSave.Restore();
		HST_OwnershipTransitionProofFixture restored = m_Factory.Configure(
			restoredState,
			fixture.m_Preset,
			fixture.m_Balance);
		HST_OwnershipTransitionState restoredReceipt;
		HST_ZoneState restoredZone;
		if (restoredState)
		{
			restoredReceipt = restoredState.FindOwnershipTransition(receipt.m_sRequestId);
			restoredZone = restoredState.FindZone(zone.m_sZoneId);
		}
		bool markerPurged = restoredState
			&& !restoredState.FindMapMarkerProjectionRecord(
				"hst_zone_" + zone.m_sZoneId);
		bool restoredSourceRejected;
		if (restored && restoredZone)
		{
			restoredSourceRejected = !restored.m_MapMarkers.ResolvePublishedZoneOwnership(
				restoredState,
				restoredZone,
				resolvedOwnerFactionKey,
				resolvedOwnershipRevision)
				&& resolvedOwnerFactionKey.IsEmpty()
				&& resolvedOwnershipRevision == 0;
		}
		string captureAfterRestore;
		if (restored)
		{
			captureAfterRestore = restored.m_ZoneCapture.BuildCaptureReport(
				restoredState,
				restored.m_Preset,
				restored.m_Balance,
				restored.m_MapMarkers);
		}
		return priorAuthorityExact
			&& commandUnavailableBeforeRestore
			&& captureUnavailableBeforeRestore
			&& restoredReceipt
			&& restoredReceipt.m_bQuarantined
			&& restoredReceipt.m_sFailureReason.Contains("non-prior marker snapshot")
			&& restoredZone
			&& restoredZone.m_iOwnershipContractVersion
				== HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION
			&& markerPurged
			&& restoredSourceRejected
			&& CommandUIPublicationUnavailable(restored)
			&& captureAfterRestore.Contains("owner publication unavailable")
			&& CountCampaignEvents(
				leakedSave,
				HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID) == 1;
	}

	protected bool ProveSetupProjectionHistoryAcrossActivation()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build(
			"setup_projection_history");
		fixture.m_State.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_SETUP;
		HST_ZoneState parentZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_setup_projection_parent",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"18400 20 14800");
		HST_ZoneState childZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_setup_projection_child",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"18700 20 14800");
		parentZone.m_aLinkedZoneIds.Insert(childZone.m_sZoneId);
		m_Factory.AddCivilianZone(fixture.m_State, childZone.m_sZoneId, 60, 40);
		bool captured = fixture.m_ZoneCapture.CaptureForResistance(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Strategic,
			fixture.m_Economy,
			fixture.m_Balance,
			parentZone.m_sZoneId,
			10);
		HST_OwnershipTransitionState parent = fixture.m_State.FindOwnershipTransition(
			parentZone.m_sLastOwnershipTransitionRequestId);
		HST_OwnershipTransitionState child = fixture.m_State.FindOwnershipTransition(
			childZone.m_sLastOwnershipTransitionRequestId);
		bool setupExact = captured
			&& parent && parent.m_bCompleted && parent.m_bProjectionRequested
			&& parent.m_bSetupProjectionWithoutMarkers
			&& parent.m_sMarkerId.IsEmpty() && parent.m_iMarkerRevision == 0
			&& child && child.m_bCompleted && child.m_bDeferredPublicationReleased
			&& child.m_bSetupProjectionWithoutMarkers
			&& child.m_sMarkerId.IsEmpty() && child.m_iMarkerRevision == 0
			&& !fixture.m_State.FindMapMarker("hst_zone_" + parentZone.m_sZoneId)
			&& !fixture.m_State.FindMapMarker("hst_zone_" + childZone.m_sZoneId);

		HST_CampaignSaveData setupSave = new HST_CampaignSaveData();
		setupSave.Capture(fixture.m_State);
		HST_OwnershipTransitionSaveValidationService validator
			= new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(setupSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = setupSave.Restore();
		HST_OwnershipTransitionProofFixture restored = m_Factory.Configure(
			restoredState,
			fixture.m_Preset,
			fixture.m_Balance);
		if (!setupExact || !restored || !restoredState)
			return false;
		restoredState.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		bool rebuilt = restored.m_MapMarkers.RebuildAllMarkers(
			restoredState,
			restored.m_Preset);
		HST_OwnershipTransitionState restoredParent
			= restoredState.FindOwnershipTransition(parent.m_sRequestId);
		HST_OwnershipTransitionState restoredChild
			= restoredState.FindOwnershipTransition(child.m_sRequestId);
		bool activatedExact = rebuilt
			&& restoredParent && restoredParent.m_bSetupProjectionWithoutMarkers
			&& restoredParent.m_sMarkerId.IsEmpty()
			&& restoredChild && restoredChild.m_bSetupProjectionWithoutMarkers
			&& restoredChild.m_sMarkerId.IsEmpty()
			&& ProjectionProofMarkerMatches(
				restoredState,
				parentZone.m_sZoneId,
				fixture.m_Preset.m_sResistanceFactionKey,
				2)
			&& ProjectionProofMarkerMatches(
				restoredState,
				childZone.m_sZoneId,
				fixture.m_Preset.m_sResistanceFactionKey,
				2);
		HST_CampaignSaveData activeSave = new HST_CampaignSaveData();
		activeSave.Capture(restoredState);
		validator.Normalize(activeSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState activeRestored = activeSave.Restore();
		HST_OwnershipTransitionState activeParent;
		HST_OwnershipTransitionState activeChild;
		if (activeRestored)
		{
			activeParent = activeRestored.FindOwnershipTransition(parent.m_sRequestId);
			activeChild = activeRestored.FindOwnershipTransition(child.m_sRequestId);
		}
		return activatedExact
			&& activeParent && !activeParent.m_bQuarantined
			&& activeParent.m_bSetupProjectionWithoutMarkers
			&& activeChild && !activeChild.m_bQuarantined
			&& activeChild.m_bSetupProjectionWithoutMarkers
			&& CountCampaignEvents(
				activeSave,
				HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID) == 0;
	}

	protected HST_OwnershipTransitionState AddDeferredChildForProof(
		HST_OwnershipTransitionProofFixture fixture,
		HST_ZoneState zone,
		HST_OwnershipTransitionState parent,
		string requestId)
	{
		HST_OwnershipTransitionState child = new HST_OwnershipTransitionState();
		child.m_iContractVersion = HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION;
		child.m_sRequestId = requestId;
		child.m_sZoneId = zone.m_sZoneId;
		child.m_sExpectedOwnerFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		child.m_sPreviousOwnerFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		child.m_sNewOwnerFactionKey = fixture.m_Preset.m_sResistanceFactionKey;
		child.m_iExpectedRevision = 1;
		child.m_iAppliedRevision = 2;
		child.m_bOwnerApplied = true;
		child.m_bCompleted = true;
		child.m_bNotify = true;
		child.m_sStatus = "completed";
		child.m_sProjectionParentRequestId = parent.m_sRequestId;
		child.m_sProjectionDecision = "publication delegated to parent " + parent.m_sRequestId;
		zone.m_iOwnershipRevision = 2;
		zone.m_sLastOwnershipTransitionRequestId = child.m_sRequestId;
		fixture.m_State.m_aOwnershipTransitions.Insert(child);
		return child;
	}

	protected void InsertProjectionProofMarker(
		HST_CampaignState state,
		HST_ZoneState zone,
		string ownerFactionKey,
		int sourceRevision)
	{
		if (!state || !zone)
			return;
		HST_MapMarkerState marker = new HST_MapMarkerState();
		marker.m_sMarkerId = "hst_zone_" + zone.m_sZoneId;
		marker.m_sLinkedId = zone.m_sZoneId;
		marker.m_sLabel = zone.m_sDisplayName + " | " + ownerFactionKey;
		marker.m_sCategory = "location";
		marker.m_sOwnerFactionKey = ownerFactionKey;
		marker.m_iRevision = Math.Max(1, sourceRevision);
		marker.m_iSourceRevision = Math.Max(1, sourceRevision);
		marker.m_iStreamSequence = Math.Max(1, sourceRevision);
		marker.m_bVisible = true;
		state.m_aMapMarkers.Insert(marker);
	}

	protected bool ProjectionProofMarkerMatches(
		HST_CampaignState state,
		string zoneId,
		string ownerFactionKey,
		int sourceRevision)
	{
		if (!state)
			return false;
		HST_MapMarkerState marker = state.FindMapMarker("hst_zone_" + zoneId);
		return marker
			&& marker.m_sOwnerFactionKey == ownerFactionKey
			&& marker.m_iSourceRevision == sourceRevision;
	}

	protected bool CommandUIPublicationMatches(
		HST_OwnershipTransitionProofFixture fixture,
		string ownerFactionKey,
		int expectedResistanceZones,
		int expectedEnemyZones,
		string expectedPlayerOwnerLabel)
	{
		if (!fixture || !fixture.m_State || !fixture.m_Preset || !fixture.m_MapMarkers)
			return false;
		HST_CommandUIService commandUI = new HST_CommandUIService();
		string zoneReport = commandUI.BuildZoneListReport(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_MapMarkers);
		string payload = commandUI.BuildVisibleMenuPayload(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_MapMarkers,
			null,
			null,
			null,
			fixture.m_Balance,
			0,
			"map",
			"",
			HST_CommandMenuAccess.Create(true, true, true, true),
			null,
			fixture.m_ZoneCapture);
		return zoneReport.Contains("owner " + ownerFactionKey)
			&& zoneReport.Contains(string.Format(
				"resistance %1 | enemy %2",
				expectedResistanceZones,
				expectedEnemyZones))
			&& payload.Contains(string.Format(
				"ROW|war_map|Resistance|%1 controlled zones|good",
				expectedResistanceZones))
			&& payload.Contains(string.Format(
				"ROW|war_map|Enemy|%1 hostile zones|bad",
				expectedEnemyZones))
			&& payload.Contains(expectedPlayerOwnerLabel + " %7C");
	}

	protected bool CommandUIPublicationUnavailable(HST_OwnershipTransitionProofFixture fixture)
	{
		if (!fixture || !fixture.m_State || !fixture.m_Preset || !fixture.m_MapMarkers)
			return false;
		HST_CommandUIService commandUI = new HST_CommandUIService();
		string zoneReport = commandUI.BuildZoneListReport(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_MapMarkers);
		string payload = commandUI.BuildVisibleMenuPayload(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_MapMarkers,
			null,
			null,
			null,
			fixture.m_Balance,
			0,
			"map",
			"",
			HST_CommandMenuAccess.Create(true, true, true, true),
			null,
			fixture.m_ZoneCapture);
		return zoneReport.Contains("resistance 0 | enemy 0")
			&& zoneReport.Contains("owner publication unavailable")
			&& payload.Contains("ROW|war_map|Resistance|0 controlled zones|good")
			&& payload.Contains("ROW|war_map|Enemy|0 hostile zones|bad")
			&& payload.Contains("Publication unavailable %7C");
	}

	protected void ProveSerializedPoliticalIntentRetry(HST_OwnershipTransitionProofReport report)
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("serialized_intent");
		HST_ZoneState blockingZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_serialized_blocker",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"15100 20 14200");
		HST_ZoneState politicalZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_serialized_town",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"15400 20 14200");
		HST_ZoneState missionZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_serialized_mission",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"15700 20 14200");
		m_Factory.AddCivilianZone(fixture.m_State, politicalZone.m_sZoneId, 60, 40);

		fixture.m_MapMarkers.SetProjectionBlocked(true);
		HST_OwnershipTransitionRequest blockingRequest = fixture.m_Service.BuildRequest(
			fixture.m_State,
			blockingZone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"serialized_blocker",
			"source proof serialized publication blocker",
			0,
			"ownership_proof_serialized_blocker_request");
		blockingRequest.m_bApplyEnemyConsequences = false;
		blockingRequest.m_bReconcileSecurity = false;
		blockingRequest.m_bCreateSecurity = false;
		blockingRequest.m_bNotify = false;
		HST_OwnershipTransitionResult blocked = fixture.m_Service.Apply(
			fixture.m_State,
			blockingRequest);
		HST_OwnershipTransitionState blockingReceipt = fixture.m_State.FindOwnershipTransition(
			blockingRequest.m_sRequestId);

		bool influenceApplied = fixture.m_Civilians.RegisterInfluenceEventExact(
			fixture.m_State,
			politicalZone.m_sZoneId,
			"serialized_retry_proof",
			10,
			0,
			0,
			0,
			0,
			0,
			0,
			"ownership intent waits for the publication fence",
			fixture.m_Preset,
			0,
			"ownership_proof_serialized_intent",
			"town_influence_ownership_proof_serialized_intent");
		HST_OwnershipTransitionState queuedPoliticalReceipt = fixture.m_State.FindOwnershipTransition(
			politicalZone.m_sActiveOwnershipTransitionRequestId);
		bool missionProgressApplied = fixture.m_ZoneCapture.AddResistanceCaptureProgress(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Strategic,
			fixture.m_Economy,
			fixture.m_Balance,
			missionZone.m_sZoneId,
			HST_ZoneCaptureService.CAPTURE_PROGRESS_REQUIRED,
			15,
			null,
			null,
			null,
			null,
			"ownership_proof_serialized_mission_instance");
		HST_OwnershipTransitionState queuedMissionReceipt = fixture.m_State.FindOwnershipTransition(
			missionZone.m_sActiveOwnershipTransitionRequestId);
		bool intentDeferred = influenceApplied
			&& politicalZone.m_sOwnerFactionKey == fixture.m_Preset.m_sOccupierFactionKey
			&& politicalZone.m_sLastOwnershipTransitionRequestId.IsEmpty()
			&& queuedPoliticalReceipt
			&& !queuedPoliticalReceipt.m_bOwnerApplied
			&& !queuedPoliticalReceipt.m_bCompleted
			&& queuedPoliticalReceipt.m_sFailureReason.Contains("durably queued")
			&& missionProgressApplied
			&& queuedMissionReceipt
			&& queuedMissionReceipt.m_sCause == "mission_capture"
			&& !queuedMissionReceipt.m_bOwnerApplied
			&& !queuedMissionReceipt.m_bCompleted
			&& queuedMissionReceipt.m_sFailureReason.Contains("durably queued")
			&& fixture.m_State.m_aOwnershipTransitions.Count() == 3
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 1;

		HST_CampaignSaveData pendingSave = new HST_CampaignSaveData();
		pendingSave.Capture(fixture.m_State);
		HST_OwnershipTransitionSaveValidationService validator = new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(pendingSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = pendingSave.Restore();
		HST_OwnershipTransitionProofFixture restored = m_Factory.Configure(
			restoredState,
			fixture.m_Preset,
			fixture.m_Balance);
		bool blockerResumeChanged = restored.m_Service.ReconcileAfterRestore(restoredState);
		HST_OwnershipTransitionState restoredBlockingReceipt = restoredState.FindOwnershipTransition(
			blockingRequest.m_sRequestId);
		HST_ZoneState restoredPoliticalZone = restoredState.FindZone(politicalZone.m_sZoneId);
		HST_ZoneState restoredMissionZone = restoredState.FindZone(missionZone.m_sZoneId);
		HST_OwnershipTransitionState restoredPoliticalReceipt;
		if (restoredPoliticalZone)
			restoredPoliticalReceipt = restoredState.FindOwnershipTransition(
				restoredPoliticalZone.m_sLastOwnershipTransitionRequestId);
		HST_OwnershipTransitionState restoredMissionReceipt;
		if (restoredMissionZone)
			restoredMissionReceipt = restoredState.FindOwnershipTransition(
				restoredMissionZone.m_sLastOwnershipTransitionRequestId);
		int influenceCountBeforePolicy = restoredState.m_aTownInfluenceEvents.Count();
		int transitionCountBeforePolicy = restoredState.m_aOwnershipTransitions.Count();
		int strategicCountBeforePolicy = restoredState.m_aStrategicEvents.Count();
		int campaignEventCountBeforePolicy = restoredState.m_aCampaignEvents.Count();
		bool politicalChanged = restored.m_Civilians.ReconcileTownOwnershipPolicies(
			restoredState,
			restored.m_Preset);
		bool firstPolicyExact = !politicalChanged
			&& restoredPoliticalReceipt
			&& restoredPoliticalReceipt.m_bCompleted
			&& restoredPoliticalReceipt.m_sCause == "political_support"
			&& restoredMissionReceipt
			&& restoredMissionReceipt.m_bCompleted
			&& restoredMissionReceipt.m_sCause == "mission_capture"
			&& restoredMissionZone.m_sOwnerFactionKey
				== fixture.m_Preset.m_sResistanceFactionKey
			&& restoredState.m_aTownInfluenceEvents.Count() == influenceCountBeforePolicy
			&& restoredState.m_aOwnershipTransitions.Count() == transitionCountBeforePolicy
			&& restoredState.m_aStrategicEvents.Count() == strategicCountBeforePolicy
			&& restoredState.m_aCampaignEvents.Count() == campaignEventCountBeforePolicy;

		int transitionCountAfterPolicy = restoredState.m_aOwnershipTransitions.Count();
		int strategicCountAfterPolicy = restoredState.m_aStrategicEvents.Count();
		int campaignEventCountAfterPolicy = restoredState.m_aCampaignEvents.Count();
		restoredState.m_iElapsedSeconds += 5;
		bool secondPolicyChanged = restored.m_Civilians.ReconcileTownOwnershipPolicies(
			restoredState,
			restored.m_Preset);
		bool repeatedPolicyInert = !secondPolicyChanged
			&& restoredState.m_aTownInfluenceEvents.Count() == influenceCountBeforePolicy
			&& restoredState.m_aOwnershipTransitions.Count() == transitionCountAfterPolicy
			&& restoredState.m_aStrategicEvents.Count() == strategicCountAfterPolicy
			&& restoredState.m_aCampaignEvents.Count() == campaignEventCountAfterPolicy;

		HST_CampaignSaveData completedSave = new HST_CampaignSaveData();
		completedSave.Capture(restoredState);
		validator.Normalize(completedSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState secondRestoredState = completedSave.Restore();
		HST_OwnershipTransitionProofFixture secondRestored = m_Factory.Configure(
			secondRestoredState,
			fixture.m_Preset,
			fixture.m_Balance);
		secondRestored.m_Service.ReconcileAfterRestore(secondRestoredState);
		secondRestoredState.m_iElapsedSeconds += 5;
		bool postRestoreChanged = secondRestored.m_Civilians.ReconcileTownOwnershipPolicies(
			secondRestoredState,
			secondRestored.m_Preset);
		bool postRestoreInert = !postRestoreChanged
			&& secondRestoredState.m_aTownInfluenceEvents.Count() == influenceCountBeforePolicy
			&& secondRestoredState.m_aOwnershipTransitions.Count() == transitionCountAfterPolicy
			&& secondRestoredState.m_aStrategicEvents.Count() == strategicCountAfterPolicy
			&& secondRestoredState.m_aCampaignEvents.Count() == campaignEventCountAfterPolicy;

		report.m_bSerializedIntentRetryExact = blocked
			&& blocked.m_bNeedsRetry
			&& blockingReceipt
			&& intentDeferred
			&& blockerResumeChanged
			&& restoredBlockingReceipt
			&& restoredBlockingReceipt.m_bCompleted
			&& restoredPoliticalZone
			&& restoredPoliticalZone.m_sOwnerFactionKey
				== fixture.m_Preset.m_sResistanceFactionKey
			&& firstPolicyExact
			&& repeatedPolicyInert
			&& postRestoreInert;
		report.m_sCauseEvidence = report.m_sCauseEvidence + string.Format(
			" | serialized intent queued %1 restore %2 political exact-once %3 repeat/restart %4/%5",
			intentDeferred,
			blockerResumeChanged && restoredBlockingReceipt && restoredBlockingReceipt.m_bCompleted,
			firstPolicyExact,
			repeatedPolicyInert,
			postRestoreInert);
	}

	protected void ProveMalformedOwnerAppliedQueueRestore(HST_OwnershipTransitionProofReport report)
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("malformed_queue_order");
		HST_ZoneState earlierZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_queue_order_earlier_zone",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"16000 20 14200");
		HST_ZoneState ownerAppliedZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_queue_order_owner_applied_zone",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"16300 20 14200");

		HST_OwnershipTransitionRequest earlierRequest = fixture.m_Service.BuildRequest(
			fixture.m_State,
			earlierZone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"queue_order_earlier",
			"malformed restore proof earlier queue authority",
			0,
			"ownership_proof_queue_order_earlier_request");
		earlierRequest.m_bApplyEnemyConsequences = false;
		earlierRequest.m_bReconcileSecurity = false;
		earlierRequest.m_bCreateSecurity = false;
		earlierRequest.m_bNotify = false;
		HST_OwnershipTransitionState earlierReceipt = fixture.m_Service.AdmitPendingForProof(
			fixture.m_State,
			earlierZone,
			earlierRequest);

		HST_OwnershipTransitionRequest ownerAppliedRequest = fixture.m_Service.BuildRequest(
			fixture.m_State,
			ownerAppliedZone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"queue_order_owner_applied",
			"malformed restore proof later owner-applied authority",
			0,
			"ownership_proof_queue_order_owner_applied_request");
		ownerAppliedRequest.m_bApplyEnemyConsequences = false;
		ownerAppliedRequest.m_bReconcileSecurity = false;
		ownerAppliedRequest.m_bCreateSecurity = false;
		ownerAppliedRequest.m_bNotify = false;
		HST_OwnershipTransitionState ownerAppliedReceipt = fixture.m_Service.AdmitPendingForProof(
			fixture.m_State,
			ownerAppliedZone,
			ownerAppliedRequest);

		if (earlierReceipt)
		{
			earlierReceipt.m_iAttemptCount = 1;
			earlierReceipt.m_iLastAttemptAtSecond = earlierReceipt.m_iCreatedAtSecond;
			earlierReceipt.m_sStatus = "applying";
			earlierReceipt.m_sFailureReason = "ownership transition is durably queued for malformed restore proof";
		}
		if (ownerAppliedReceipt)
		{
			ownerAppliedReceipt.m_iAttemptCount = 1;
			ownerAppliedReceipt.m_iLastAttemptAtSecond = ownerAppliedReceipt.m_iCreatedAtSecond;
			ownerAppliedReceipt.m_sStatus = "applying";
			ownerAppliedReceipt.m_bOldSecurityRetired = true;
			ownerAppliedReceipt.m_bHostileRuntimeRetired = true;
			ownerAppliedReceipt.m_bNewSecurityApplied = true;
			ownerAppliedReceipt.m_bSupportApplied = true;
			ownerAppliedReceipt.m_iAppliedRevision = ownerAppliedReceipt.m_iExpectedRevision + 1;
			ownerAppliedReceipt.m_bOwnerApplied = true;
			ownerAppliedZone.m_sOwnerFactionKey = ownerAppliedReceipt.m_sNewOwnerFactionKey;
			ownerAppliedZone.m_iOwnershipRevision = ownerAppliedReceipt.m_iAppliedRevision;
		}

		HST_CampaignSaveData malformedSave = new HST_CampaignSaveData();
		malformedSave.Capture(fixture.m_State);
		HST_OwnershipTransitionSaveValidationService validator = new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(malformedSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = malformedSave.Restore();
		HST_OwnershipTransitionState restoredEarlier;
		HST_OwnershipTransitionState restoredOwnerApplied;
		HST_ZoneState restoredEarlierZone;
		HST_ZoneState restoredOwnerAppliedZone;
		if (restoredState)
		{
			restoredEarlier = restoredState.FindOwnershipTransition(earlierRequest.m_sRequestId);
			restoredOwnerApplied = restoredState.FindOwnershipTransition(ownerAppliedRequest.m_sRequestId);
			restoredEarlierZone = restoredState.FindZone(earlierZone.m_sZoneId);
			restoredOwnerAppliedZone = restoredState.FindZone(ownerAppliedZone.m_sZoneId);
		}

		bool queueOrderExact = earlierReceipt && ownerAppliedReceipt && restoredState;
		if (queueOrderExact)
			queueOrderExact = restoredState.m_aOwnershipTransitions.Count() == 2
				&& restoredState.m_aOwnershipTransitions[0].m_sRequestId
					== earlierRequest.m_sRequestId;
		if (queueOrderExact)
			queueOrderExact = restoredEarlier && !restoredEarlier.m_bOwnerApplied
				&& !restoredEarlier.m_bQuarantined
				&& restoredEarlier.m_iContractVersion
					== HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION;
		if (queueOrderExact)
			queueOrderExact = restoredEarlierZone
				&& restoredEarlierZone.m_sActiveOwnershipTransitionRequestId
					== restoredEarlier.m_sRequestId;
		if (queueOrderExact)
			queueOrderExact = restoredOwnerApplied
				&& restoredOwnerApplied.m_bOwnerApplied
				&& restoredOwnerApplied.m_bQuarantined
				&& restoredOwnerApplied.m_iContractVersion
					== HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION;
		bool ownerAppliedFailureExact = restoredOwnerApplied
			&& restoredOwnerApplied.m_sFailureReason.Contains("earliest unresolved publication authority");
		if (queueOrderExact)
			queueOrderExact = ownerAppliedFailureExact;
		if (queueOrderExact)
			queueOrderExact = restoredOwnerAppliedZone
				&& restoredOwnerAppliedZone.m_iOwnershipContractVersion
					== HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION
				&& restoredOwnerAppliedZone.m_sActiveOwnershipTransitionRequestId
					== restoredOwnerApplied.m_sRequestId;
		if (queueOrderExact)
			queueOrderExact = CountCampaignEvents(
				malformedSave,
				HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID) == 1;
		bool pristineQueueExact = ProveMalformedPristineQueueRestore();
		report.m_bRestoreQueueOrderFailClosed = queueOrderExact && pristineQueueExact;
		report.m_sRestoreProjectionEvidence = report.m_sRestoreProjectionEvidence + string.Format(
			" | malformed queue earlier retained %1 | later owner-applied quarantined %2 | conflict event %3 | later progressed pre-owner quarantined %4",
			restoredEarlier && !restoredEarlier.m_bQuarantined,
			restoredOwnerApplied && restoredOwnerApplied.m_bQuarantined,
			CountCampaignEvents(malformedSave, HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID),
			pristineQueueExact);
	}

	protected bool ProveMalformedPristineQueueRestore()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("malformed_pristine_queue");
		HST_ZoneState earlierZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_pristine_queue_earlier_zone",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"17500 20 14200");
		HST_ZoneState laterZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_pristine_queue_later_zone",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"17800 20 14200");
		HST_OwnershipTransitionRequest earlierRequest = fixture.m_Service.BuildRequest(
			fixture.m_State,
			earlierZone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"pristine_queue_earlier",
			"malformed pristine queue earlier authority",
			0,
			"ownership_proof_pristine_queue_earlier_request");
		earlierRequest.m_bApplyEnemyConsequences = false;
		earlierRequest.m_bReconcileSecurity = false;
		earlierRequest.m_bCreateSecurity = false;
		earlierRequest.m_bNotify = false;
		HST_OwnershipTransitionState earlierReceipt = fixture.m_Service.AdmitPendingForProof(
			fixture.m_State,
			earlierZone,
			earlierRequest);
		HST_OwnershipTransitionRequest laterRequest = fixture.m_Service.BuildRequest(
			fixture.m_State,
			laterZone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"pristine_queue_later",
			"malformed later queue authority has pre-owner security progress",
			0,
			"ownership_proof_pristine_queue_later_request");
		laterRequest.m_bApplyEnemyConsequences = false;
		laterRequest.m_bReconcileSecurity = false;
		laterRequest.m_bCreateSecurity = false;
		laterRequest.m_bNotify = false;
		HST_OwnershipTransitionState laterReceipt = fixture.m_Service.AdmitPendingForProof(
			fixture.m_State,
			laterZone,
			laterRequest);
		if (earlierReceipt)
		{
			earlierReceipt.m_iAttemptCount = 1;
			earlierReceipt.m_iLastAttemptAtSecond = earlierReceipt.m_iCreatedAtSecond;
			earlierReceipt.m_sStatus = "applying";
		}
		if (laterReceipt)
		{
			laterReceipt.m_iAttemptCount = 1;
			laterReceipt.m_iLastAttemptAtSecond = laterReceipt.m_iCreatedAtSecond;
			laterReceipt.m_sStatus = "applying";
			laterReceipt.m_bOldSecurityRetired = true;
		}

		HST_CampaignSaveData malformedSave = new HST_CampaignSaveData();
		malformedSave.Capture(fixture.m_State);
		HST_OwnershipTransitionSaveValidationService validator = new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(malformedSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = malformedSave.Restore();
		HST_OwnershipTransitionState restoredEarlier;
		HST_OwnershipTransitionState restoredLater;
		HST_ZoneState restoredLaterZone;
		if (restoredState)
		{
			restoredEarlier = restoredState.FindOwnershipTransition(earlierRequest.m_sRequestId);
			restoredLater = restoredState.FindOwnershipTransition(laterRequest.m_sRequestId);
			restoredLaterZone = restoredState.FindZone(laterZone.m_sZoneId);
		}
		return earlierReceipt
			&& laterReceipt
			&& restoredEarlier
			&& !restoredEarlier.m_bQuarantined
			&& restoredLater
			&& restoredLater.m_bQuarantined
			&& restoredLater.m_sFailureReason.Contains("not pristine queued authority")
			&& restoredLaterZone
			&& restoredLaterZone.m_iOwnershipContractVersion == HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION
			&& CountCampaignEvents(malformedSave, HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID) == 1;
	}

	protected void ProvePersistenceCheckpointDeadline(HST_OwnershipTransitionProofReport report)
	{
		HST_OwnershipTransitionPersistenceClockProofHarness persistence = new HST_OwnershipTransitionPersistenceClockProofHarness();
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_WON;
		state.m_iElapsedSeconds = 620;
		int frozenElapsedSecond = state.m_iElapsedSeconds;
		persistence.MarkMajorChange();
		for (int firstWindowTick; firstWindowTick < 5; firstWindowTick++)
		{
			persistence.MarkMajorChange();
			persistence.Tick(state, 1.0, 1000, 5);
		}
		bool firstDeadlineExact = persistence.m_iCheckpointAttempts == 1
			&& persistence.MajorChangePendingForProof();
		for (int retryWindowTick; retryWindowTick < 5; retryWindowTick++)
		{
			persistence.MarkMajorChange();
			persistence.Tick(state, 1.0, 1000, 5);
		}
		bool retryDeadlineExact = persistence.m_iCheckpointAttempts == 2
			&& !persistence.MajorChangePendingForProof();

		persistence.MarkMajorChange();
		for (int freshWindowTick; freshWindowTick < 4; freshWindowTick++)
		{
			persistence.MarkMajorChange();
			persistence.Tick(state, 1.0, 1000, 5);
		}
		bool freshIntervalHeld = persistence.m_iCheckpointAttempts == 2
			&& persistence.MajorChangePendingForProof();
		persistence.MarkMajorChange();
		persistence.Tick(state, 1.0, 1000, 5);
		bool freshDeadlineExact = persistence.m_iCheckpointAttempts == 3
			&& !persistence.MajorChangePendingForProof();
		bool completionRearmExact = ProveCompletionPersistenceRearm();
		report.m_bPersistenceDeadlineExact = firstDeadlineExact
			&& retryDeadlineExact
			&& freshIntervalHeld
			&& freshDeadlineExact
			&& completionRearmExact
			&& state.m_iElapsedSeconds == frozenElapsedSecond;
		report.m_sRestoreProjectionEvidence = report.m_sRestoreProjectionEvidence + string.Format(
			" | persistence debounce first/retry/fresh %1/%2/%3 attempts %4 frozen %5 completion rearm %6",
			firstDeadlineExact,
			retryDeadlineExact,
			freshIntervalHeld && freshDeadlineExact,
			persistence.m_iCheckpointAttempts,
			state.m_iElapsedSeconds == frozenElapsedSecond,
			completionRearmExact);
	}

	protected bool ProveCompletionPersistenceRearm()
	{
		HST_OwnershipTransitionProofFixture fixture;
		HST_OwnershipTransitionState receipt;
		BuildCompletedAdminCorrelationFixture(
			"completion_persistence_rearm",
			0,
			fixture,
			receipt);
		if (!fixture || !receipt || !receipt.m_bPersistenceRequested)
			return false;
		HST_ZoneState zone = fixture.m_State.FindZone(receipt.m_sZoneId);
		if (!zone)
			return false;
		receipt.m_sStatus = "projecting";
		receipt.m_bCompleted = false;
		receipt.m_iCompletedAtSecond = 0;
		zone.m_sActiveOwnershipTransitionRequestId = receipt.m_sRequestId;
		zone.m_sLastOwnershipTransitionRequestId = "";

		HST_CampaignSaveData interruptedSave = new HST_CampaignSaveData();
		interruptedSave.Capture(fixture.m_State);
		HST_OwnershipTransitionSaveValidationService validator
			= new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(interruptedSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = interruptedSave.Restore();
		HST_OwnershipTransitionProofFixture restored = m_Factory.Configure(
			restoredState,
			fixture.m_Preset,
			fixture.m_Balance);
		if (!restored || !restoredState || restored.m_Persistence.MajorChangePendingForProof())
			return false;
		bool reconciled = restored.m_Service.ReconcileAfterRestore(restoredState);
		HST_OwnershipTransitionState restoredReceipt = restoredState.FindOwnershipTransition(
			receipt.m_sRequestId);
		HST_ZoneState restoredZone = restoredState.FindZone(receipt.m_sZoneId);
		return reconciled
			&& restoredReceipt
			&& restoredReceipt.m_bCompleted
			&& restoredReceipt.m_bPersistenceRequested
			&& restoredZone
			&& restoredZone.m_sActiveOwnershipTransitionRequestId.IsEmpty()
			&& restoredZone.m_sLastOwnershipTransitionRequestId == receipt.m_sRequestId
			&& restored.m_Persistence.MajorChangePendingForProof();
	}

	protected void ProveInterruptedSaveRestore(HST_OwnershipTransitionProofReport report)
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("restore");
		HST_ZoneState zone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_restore",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_FACTORY,
			"8200 25 8200");
		fixture.m_MapMarkers.SetProjectionBlocked(true);
		HST_OwnershipTransitionRequest request = fixture.m_Service.BuildRequest(
			fixture.m_State,
			zone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"interrupted_restore",
			"source proof interrupted transition",
			0,
			"ownership_proof_interrupted_restore");
		request.m_bApplyEnemyConsequences = false;
		request.m_bReconcileSecurity = false;
		request.m_bCreateSecurity = false;
		request.m_bNotify = false;
		HST_OwnershipTransitionResult interrupted = fixture.m_Service.Apply(fixture.m_State, request);
		HST_OwnershipTransitionState interruptedReceipt = fixture.m_State.FindOwnershipTransition(request.m_sRequestId);
		int partialPersistenceRequests = fixture.m_Persistence.m_iMajorChangeRequests;
		int strategicEventsBeforeSave = fixture.m_State.m_aStrategicEvents.Count();
		int campaignEventsBeforeSave = fixture.m_State.m_aCampaignEvents.Count();

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restoredState = saveData.Restore();
		HST_OwnershipTransitionProofFixture restored = m_Factory.Configure(restoredState, fixture.m_Preset, fixture.m_Balance);
		bool reconciled = restored.m_Service.ReconcileAfterRestore(restoredState);
		HST_ZoneState restoredZone = restoredState.FindZone(zone.m_sZoneId);
		HST_OwnershipTransitionState restoredReceipt = restoredState.FindOwnershipTransition(request.m_sRequestId);
		HST_MapMarkerState restoredMarker = restoredState.FindMapMarker("hst_zone_" + zone.m_sZoneId);

		bool interruptionExact = InterruptedReceiptIsExact(interrupted, interruptedReceipt, partialPersistenceRequests);
		bool restoredAuthorityExact = RestoredAuthorityIsExact(
			reconciled,
			restoredState,
			restored,
			restoredZone,
			restoredReceipt);
		bool restoredEffectsExact = restoredState.m_aStrategicEvents.Count() == strategicEventsBeforeSave;
		if (restoredState.m_aCampaignEvents.Count() != campaignEventsBeforeSave)
			restoredEffectsExact = false;
		report.m_bInterruptedRestoreExact = interruptionExact;
		if (!restoredAuthorityExact || !restoredEffectsExact)
			report.m_bInterruptedRestoreExact = false;

		bool restoreProjectionExact = false;
		if (restoredZone)
			restoreProjectionExact = MarkerMatchesReceipt(
				restoredMarker,
				restoredReceipt,
				restoredZone.m_iOwnershipRevision);
		if (!restoreProjectionExact)
			report.m_bProjectionSourceRevisionExact = false;

		bool interruptedRetry;
		bool restoredCompleted;
		int restoredAttempts;
		int restoredRevision;
		int markerSourceRevision;
		if (interrupted)
			interruptedRetry = interrupted.m_bNeedsRetry;
		if (restoredReceipt)
		{
			restoredCompleted = restoredReceipt.m_bCompleted;
			restoredAttempts = restoredReceipt.m_iAttemptCount;
		}
		if (restoredZone)
			restoredRevision = restoredZone.m_iOwnershipRevision;
		if (restoredMarker)
			markerSourceRevision = restoredMarker.m_iSourceRevision;
		report.m_sRestoreProjectionEvidence = string.Format(
			"interrupted retry/completed %1/%2 | attempts %3 | owner revision %4 | marker source %5",
			interruptedRetry,
			restoredCompleted,
			restoredAttempts,
			restoredRevision,
			markerSourceRevision);
		report.m_sRestoreProjectionEvidence = report.m_sRestoreProjectionEvidence + string.Format(
			" | events %1/%2",
			restoredState.m_aStrategicEvents.Count(),
			restoredState.m_aCampaignEvents.Count());
	}

	protected bool InterruptedReceiptIsExact(
		HST_OwnershipTransitionResult result,
		HST_OwnershipTransitionState receipt,
		int persistenceRequests)
	{
		if (!result || !result.m_bAccepted || !result.m_bNeedsRetry || result.m_bCompleted)
			return false;
		if (!receipt || !receipt.m_bOwnerApplied)
			return false;
		if (receipt.m_bProjectionRequested || receipt.m_bPersistenceRequested)
			return false;
		return persistenceRequests == 1;
	}

	protected bool RestoredAuthorityIsExact(
		bool reconciled,
		HST_CampaignState state,
		HST_OwnershipTransitionProofFixture fixture,
		HST_ZoneState zone,
		HST_OwnershipTransitionState receipt)
	{
		if (!reconciled || !state || !fixture || !zone || !receipt)
			return false;
		if (!receipt.m_bCompleted || receipt.m_iAttemptCount != 2)
			return false;
		if (zone.m_iOwnershipRevision != 2)
			return false;
		if (state.m_aOwnershipTransitions.Count() != 1)
			return false;
		return fixture.m_Persistence.m_iMajorChangeRequests == 1;
	}

	protected void ProveExactSecurityFailClosed(HST_OwnershipTransitionProofReport report)
	{
		string nonPatrolEvidence;
		string orphanEvidence;
		string resumeEvidence;
		bool nonPatrolExact = ProveNonPatrolSecurityFailClosed(nonPatrolEvidence);
		bool orphanExact = ProveOrphanPatrolSecurityFailClosed(orphanEvidence);
		bool resumeExact = ProveLateSecurityResumeFailClosed(resumeEvidence);
		report.m_bExactSecurityFailClosed = nonPatrolExact && orphanExact && resumeExact;
		report.m_sAuthorityEvidence = nonPatrolEvidence
			+ " | " + orphanEvidence
			+ " | " + resumeEvidence;
	}

	protected bool ProveNonPatrolSecurityFailClosed(out string evidence)
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("security");
		HST_ZoneState zone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_security",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"9300 30 9300",
			12);
		HST_ForceManifestState manifest = new HST_ForceManifestState();
		manifest.m_sManifestId = "ownership_proof_non_patrol_manifest";
		manifest.m_sPolicyId = "source_proof_non_patrol_exact_authority";
		manifest.m_sFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		manifest.m_sTargetZoneId = zone.m_sZoneId;
		fixture.m_State.m_aForceManifests.Insert(manifest);
		HST_GarrisonState garrison = new HST_GarrisonState();
		garrison.m_sGarrisonId = "ownership_proof_security_garrison";
		garrison.m_sZoneId = zone.m_sZoneId;
		garrison.m_sFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		garrison.m_iInfantryCount = 4;
		garrison.m_aAcceptedManifestIds.Insert(manifest.m_sManifestId);
		fixture.m_State.m_aGarrisons.Insert(garrison);

		bool productionCapture = fixture.m_ZoneCapture.CaptureForResistance(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Strategic,
			fixture.m_Economy,
			fixture.m_Balance,
			zone.m_sZoneId,
			0);
		HST_OwnershipTransitionRequest diagnosticRequest = fixture.m_Service.BuildRequest(
			fixture.m_State,
			zone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"military_capture",
			"source_proof",
			"security_fail_closed",
			"source proof exact security preflight",
			0,
			"ownership_proof_security_fail_closed");
		HST_OwnershipTransitionResult diagnostic = fixture.m_Service.Apply(fixture.m_State, diagnosticRequest);
		bool exact = !productionCapture;
		if (!RejectedWithReason(diagnostic, "non-patrol exact garrison authority"))
			exact = false;
		if (zone.m_sOwnerFactionKey != fixture.m_Preset.m_sOccupierFactionKey)
			exact = false;
		if (zone.m_iOwnershipRevision != 1)
			exact = false;
		if (!fixture.m_State.m_aOwnershipTransitions.IsEmpty())
			exact = false;
		if (garrison.m_iInfantryCount != 4)
			exact = false;
		if (!garrison.m_aAcceptedManifestIds.Contains(manifest.m_sManifestId))
			exact = false;
		string diagnosticReason;
		if (diagnostic)
			diagnosticReason = diagnostic.m_sFailureReason;
		evidence = string.Format(
			"non-patrol security capture accepted %1 | owner/revision %2/%3 | receipts %4 | reason %5",
			productionCapture,
			zone.m_sOwnerFactionKey,
			zone.m_iOwnershipRevision,
			fixture.m_State.m_aOwnershipTransitions.Count(),
			diagnosticReason);
		return exact;
	}

	protected bool ProveOrphanPatrolSecurityFailClosed(out string evidence)
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("security_orphan");
		HST_ZoneState zone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_security_orphan",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"9350 30 9350",
			12);
		HST_ForceManifestState manifest = new HST_ForceManifestState();
		manifest.m_sManifestId = "ownership_proof_orphan_patrol_manifest";
		manifest.m_sOperationId = "ownership_proof_missing_patrol_operation";
		manifest.m_sPolicyId = HST_GarrisonPatrolOperationService.EXACT_POLICY_ID;
		manifest.m_sFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		manifest.m_sTargetZoneId = zone.m_sZoneId;
		fixture.m_State.m_aForceManifests.Insert(manifest);
		HST_GarrisonState garrison = new HST_GarrisonState();
		garrison.m_sGarrisonId = "ownership_proof_orphan_security_garrison";
		garrison.m_sZoneId = zone.m_sZoneId;
		garrison.m_sFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		garrison.m_iInfantryCount = 5;
		garrison.m_aAcceptedManifestIds.Insert(manifest.m_sManifestId);
		fixture.m_State.m_aGarrisons.Insert(garrison);

		HST_OwnershipTransitionRequest request = fixture.m_Service.BuildRequest(
			fixture.m_State,
			zone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"military_capture",
			"source_proof",
			"orphan_patrol_fail_closed",
			"source proof orphan patrol authority preflight",
			0,
			"ownership_proof_orphan_patrol_fail_closed");
		HST_OwnershipTransitionResult result = fixture.m_Service.Apply(fixture.m_State, request);
		bool exact = RejectedWithReason(result, "reciprocal operation authority")
			&& zone.m_sOwnerFactionKey == fixture.m_Preset.m_sOccupierFactionKey
			&& zone.m_iOwnershipRevision == 1
			&& fixture.m_State.m_aOwnershipTransitions.IsEmpty()
			&& garrison.m_iInfantryCount == 5
			&& garrison.m_aAcceptedManifestIds.Contains(manifest.m_sManifestId);
		string failureReason;
		if (result)
			failureReason = result.m_sFailureReason;
		evidence = string.Format(
			"orphan patrol rejected %1 | owner/revision %2/%3 | garrison %4 | reason %5",
			exact,
			zone.m_sOwnerFactionKey,
			zone.m_iOwnershipRevision,
			garrison.m_iInfantryCount,
			failureReason);
		return exact;
	}

	protected bool ProveLateSecurityResumeFailClosed(out string evidence)
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("security_resume");
		HST_ZoneState zone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_security_resume",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"9400 30 9400",
			12);
		HST_OwnershipTransitionRequest request = fixture.m_Service.BuildRequest(
			fixture.m_State,
			zone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"military_capture",
			"source_proof",
			"late_security_resume",
			"source proof late security resume",
			0,
			"ownership_proof_late_security_resume");
		HST_OwnershipTransitionState receipt = fixture.m_Service.AdmitPendingForProof(
			fixture.m_State,
			zone,
			request);
		if (!receipt)
		{
			evidence = "late security resume fixture admission failed";
			return false;
		}
		receipt.m_bOldSecurityRetired = true;
		receipt.m_bHostileRuntimeRetired = true;
		receipt.m_bNewSecurityApplied = true;
		receipt.m_bSupportApplied = true;

		HST_ForceManifestState manifest = new HST_ForceManifestState();
		manifest.m_sManifestId = "ownership_proof_late_patrol_manifest";
		manifest.m_sOperationId = "ownership_proof_late_missing_operation";
		manifest.m_sPolicyId = HST_GarrisonPatrolOperationService.EXACT_POLICY_ID;
		manifest.m_sFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		manifest.m_sTargetZoneId = zone.m_sZoneId;
		fixture.m_State.m_aForceManifests.Insert(manifest);
		HST_GarrisonState garrison = new HST_GarrisonState();
		garrison.m_sGarrisonId = "ownership_proof_late_security_garrison";
		garrison.m_sZoneId = zone.m_sZoneId;
		garrison.m_sFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		garrison.m_iInfantryCount = 6;
		garrison.m_aAcceptedManifestIds.Insert(manifest.m_sManifestId);
		fixture.m_State.m_aGarrisons.Insert(garrison);

		HST_OwnershipTransitionResult result = fixture.m_Service.Apply(fixture.m_State, request);
		bool exact = result && result.m_bAccepted && result.m_bNeedsRetry && !result.m_bCompleted
			&& result.m_sFailureReason.Contains("reciprocal operation authority")
			&& !receipt.m_bOwnerApplied
			&& zone.m_sOwnerFactionKey == fixture.m_Preset.m_sOccupierFactionKey
			&& zone.m_iOwnershipRevision == 1
			&& zone.m_sActiveOwnershipTransitionRequestId == receipt.m_sRequestId
			&& garrison.m_iInfantryCount == 6
			&& garrison.m_aAcceptedManifestIds.Contains(manifest.m_sManifestId);
		string failureReason;
		if (result)
			failureReason = result.m_sFailureReason;
		evidence = string.Format(
			"late resume held %1 | owner/revision %2/%3 | active %4 | garrison %5 | reason %6",
			exact,
			zone.m_sOwnerFactionKey,
			zone.m_iOwnershipRevision,
			zone.m_sActiveOwnershipTransitionRequestId,
			garrison.m_iInfantryCount,
			failureReason);
		return exact;
	}

	protected void ProveMigrationAndRetention(HST_OwnershipTransitionProofReport report)
	{
		HST_OwnershipTransitionSaveValidationService validator = new HST_OwnershipTransitionSaveValidationService();
		HST_CampaignSaveData legacy = new HST_CampaignSaveData();
		legacy.m_iElapsedSeconds = 6200;
		HST_FactionPoolState legacyPool = new HST_FactionPoolState();
		legacyPool.m_sFactionKey = "USSR";
		legacy.m_aFactionPools.Insert(legacyPool);
		HST_ZoneState legacyZone = new HST_ZoneState();
		legacyZone.m_sZoneId = "ownership_proof_legacy";
		legacyZone.m_sOwnerFactionKey = "USSR";
		legacyZone.m_iOwnershipContractVersion = 0;
		legacyZone.m_iOwnershipRevision = 9;
		legacy.m_aZones.Insert(legacyZone);
		HST_OwnershipTransitionState discardedLegacyReceipt = new HST_OwnershipTransitionState();
		discardedLegacyReceipt.m_sRequestId = "ownership_proof_legacy_receipt";
		legacy.m_aOwnershipTransitions.Insert(discardedLegacyReceipt);
		HST_MapMarkerState legacyMarker = new HST_MapMarkerState();
		legacyMarker.m_sMarkerId = "hst_zone_" + legacyZone.m_sZoneId;
		legacyMarker.m_sLinkedId = legacyZone.m_sZoneId;
		legacyMarker.m_iSourceRevision = 9;
		legacy.m_aMapMarkers.Insert(legacyMarker);
		validator.Normalize(legacy, 61);
		int migrationEventCountAfterFirst = CountCampaignEvents(legacy, HST_OwnershipTransitionSaveValidationService.MIGRATION_EVENT_ID);
		validator.Normalize(legacy, 62);
		bool migrationExact = legacyZone.m_sOwnerFactionKey == "USSR"
			&& legacyZone.m_iOwnershipContractVersion == HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			&& legacyZone.m_iOwnershipRevision == 1
			&& legacyZone.m_sActiveOwnershipTransitionRequestId.IsEmpty()
			&& legacyZone.m_sLastOwnershipTransitionRequestId.IsEmpty()
			&& legacy.m_aOwnershipTransitions.IsEmpty()
			&& legacyMarker.m_iSourceRevision == 1
			&& migrationEventCountAfterFirst == 1
			&& CountCampaignEvents(legacy, HST_OwnershipTransitionSaveValidationService.MIGRATION_EVENT_ID) == 1;

		HST_CampaignSaveData corrupt = new HST_CampaignSaveData();
		corrupt.m_iElapsedSeconds = 6300;
		HST_FactionPoolState corruptPool = new HST_FactionPoolState();
		corruptPool.m_sFactionKey = "FIA";
		corrupt.m_aFactionPools.Insert(corruptPool);
		HST_ZoneState corruptZone = new HST_ZoneState();
		corruptZone.m_sZoneId = "ownership_proof_corrupt";
		corruptZone.m_sOwnerFactionKey = "FIA";
		corruptZone.m_iOwnershipContractVersion = HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION;
		corruptZone.m_iOwnershipRevision = 2;
		corruptZone.m_sLastOwnershipTransitionRequestId = "ownership_proof_corrupt_receipt";
		corrupt.m_aZones.Insert(corruptZone);
		HST_OwnershipTransitionState corruptReceipt = new HST_OwnershipTransitionState();
		corruptReceipt.m_sRequestId = corruptZone.m_sLastOwnershipTransitionRequestId;
		corruptReceipt.m_sZoneId = corruptZone.m_sZoneId;
		corruptReceipt.m_iContractVersion = HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION;
		corruptReceipt.m_sStatus = "completed";
		corruptReceipt.m_bCompleted = true;
		corrupt.m_aOwnershipTransitions.Insert(corruptReceipt);
		validator.Normalize(corrupt, 62);
		bool quarantineExact = corruptReceipt.m_bQuarantined
			&& corruptReceipt.m_iContractVersion == HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION
			&& corruptZone.m_iOwnershipContractVersion == HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION
			&& !corruptZone.m_sOwnershipAuthorityFailure.IsEmpty()
			&& CountCampaignEvents(corrupt, HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID) == 1;

		HST_CampaignState retentionState = new HST_CampaignState();
		retentionState.m_iElapsedSeconds = 200000;
		HST_ZoneState retentionZone = new HST_ZoneState();
		retentionZone.m_sZoneId = "ownership_proof_retention_zone";
		retentionZone.m_sLastOwnershipTransitionRequestId = "ownership_proof_retention_latest";
		retentionState.m_aZones.Insert(retentionZone);
		HST_OwnershipTransitionState oldEligible = BuildRetentionReceipt("ownership_proof_retention_old", 1, true, false);
		HST_OwnershipTransitionState recent = BuildRetentionReceipt("ownership_proof_retention_recent", 199999, true, false);
		HST_OwnershipTransitionState latest = BuildRetentionReceipt(retentionZone.m_sLastOwnershipTransitionRequestId, 2, true, false);
		HST_OwnershipTransitionState incomplete = BuildRetentionReceipt("ownership_proof_retention_incomplete", 0, false, false);
		HST_OwnershipTransitionState quarantined = BuildRetentionReceipt("ownership_proof_retention_quarantined", 3, true, true);
		retentionState.m_aOwnershipTransitions.Insert(oldEligible);
		retentionState.m_aOwnershipTransitions.Insert(recent);
		retentionState.m_aOwnershipTransitions.Insert(latest);
		retentionState.m_aOwnershipTransitions.Insert(incomplete);
		retentionState.m_aOwnershipTransitions.Insert(quarantined);
		for (int i = retentionState.m_aOwnershipTransitions.Count(); i < HST_OwnershipTransitionService.MAX_TRANSITION_ROWS; i++)
			retentionState.m_aOwnershipTransitions.Insert(BuildRetentionReceipt("ownership_proof_retention_filler_" + i, 199999, true, false));

		HST_OwnershipTransitionProofHarness retentionHarness = new HST_OwnershipTransitionProofHarness();
		bool eligibilityExact = retentionHarness.CanPruneForProof(retentionState, oldEligible)
			&& !retentionHarness.CanPruneForProof(retentionState, recent)
			&& !retentionHarness.CanPruneForProof(retentionState, latest)
			&& !retentionHarness.CanPruneForProof(retentionState, incomplete)
			&& !retentionHarness.CanPruneForProof(retentionState, quarantined);
		retentionHarness.PruneForProof(retentionState);
		bool retentionExact = eligibilityExact
			&& retentionState.m_aOwnershipTransitions.Count() == HST_OwnershipTransitionService.MAX_TRANSITION_ROWS - 1
			&& !retentionState.FindOwnershipTransition(oldEligible.m_sRequestId)
			&& retentionState.FindOwnershipTransition(recent.m_sRequestId)
			&& retentionState.FindOwnershipTransition(latest.m_sRequestId)
			&& retentionState.FindOwnershipTransition(incomplete.m_sRequestId)
			&& retentionState.FindOwnershipTransition(quarantined.m_sRequestId);

		bool forgedChildExact = ProveForgedUnreleasedChildQuarantine();
		bool historyExact = ProveConflictingCompletedHistoryQuarantine();
		bool causePolicyExact = ProveCausePolicyFailClosed();
		bool revisionCeilingExact = ProveRevisionCeilingFailClosed();
		bool supportEventExact = ProveAppliedSupportEventCorrelationFailClosed();
		bool supportTargetExact = ProveSupportTargetAuthorityFailClosed();
		bool derivedCorrelationExact = ProveReceiptDerivedCorrelationFailClosed();
		bool pre64SupportReceiptExact = ProvePre64AppliedSupportReceiptMigration();
		report.m_bMigrationRetentionExact = migrationExact
			&& quarantineExact
			&& retentionExact
			&& forgedChildExact
			&& historyExact
			&& causePolicyExact
			&& revisionCeilingExact
			&& supportEventExact
			&& supportTargetExact
			&& derivedCorrelationExact
			&& pre64SupportReceiptExact;
		report.m_sAuthorityEvidence = report.m_sAuthorityEvidence + string.Format(
			" | migration/quarantine/retention %1/%2/%3 | retained %4 | forged child/history %5/%6 | cause/revision/support-event %7/%8/%9",
			migrationExact,
			quarantineExact,
			retentionExact,
			retentionState.m_aOwnershipTransitions.Count(),
			forgedChildExact,
			historyExact,
			causePolicyExact,
			revisionCeilingExact,
			supportEventExact);
		report.m_sAuthorityEvidence = report.m_sAuthorityEvidence
			+ string.Format(
				" | deterministic support targets/prefix %1 | derived correlations %2 | pre64 support receipt %3",
				supportTargetExact,
				derivedCorrelationExact,
				pre64SupportReceiptExact);
	}

	protected bool ProvePre64AppliedSupportReceiptMigration()
	{
		HST_OwnershipTransitionProofFixture fixture
			= m_Factory.Build("pre64_support_receipt_migration");
		HST_ZoneState capturedZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_pre64_support_site",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"11800 20 14800");
		HST_ZoneState linkedTown = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_pre64_support_town",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"12100 20 14800");
		capturedZone.m_aLinkedZoneIds.Insert(linkedTown.m_sZoneId);
		m_Factory.AddCivilianZone(
			fixture.m_State,
			linkedTown.m_sZoneId,
			60,
			40);
		bool captured = fixture.m_ZoneCapture.CaptureForResistance(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Strategic,
			fixture.m_Economy,
			fixture.m_Balance,
			capturedZone.m_sZoneId,
			10);
		string parentRequestId = capturedZone.m_sLastOwnershipTransitionRequestId;
		HST_OwnershipTransitionState parentBefore
			= fixture.m_State.FindOwnershipTransition(parentRequestId);
		if (!captured || !parentBefore || !parentBefore.m_bCompleted
			|| !parentBefore.m_bSupportApplied)
			return false;

		HST_CampaignSaveData restored = new HST_CampaignSaveData();
		restored.Capture(fixture.m_State);
		// Schema 63 had no canonical record array and no serialized influence
		// contract field. In the current class layout those absent event fields
		// deserialize to one until the schema-64 prepass relabels them.
		restored.m_aTownInfluenceRecords.Clear();
		HST_TownInfluenceSaveValidationService townValidator
			= new HST_TownInfluenceSaveValidationService();
		townValidator.ValidateCurrentAuthorityBeforeOwnership(restored, 63);
		HST_OwnershipTransitionSaveValidationService ownershipValidator
			= new HST_OwnershipTransitionSaveValidationService();
		ownershipValidator.Normalize(restored, 63);
		townValidator.ValidateAfterOwnership(restored);

		HST_OwnershipTransitionState parentAfter;
		foreach (HST_OwnershipTransitionState candidate : restored.m_aOwnershipTransitions)
		{
			if (candidate && candidate.m_sRequestId == parentRequestId)
			{
				parentAfter = candidate;
				break;
			}
		}
		HST_TownInfluenceRecord townRecord;
		foreach (HST_TownInfluenceRecord candidateRecord : restored.m_aTownInfluenceRecords)
		{
			if (candidateRecord && candidateRecord.m_sTownId == linkedTown.m_sZoneId)
			{
				townRecord = candidateRecord;
				break;
			}
		}
		HST_TownInfluenceEventState supportEvent;
		foreach (HST_TownInfluenceEventState candidateEvent : restored.m_aTownInfluenceEvents)
		{
			if (candidateEvent
				&& candidateEvent.m_sKind == "ownership_support"
				&& candidateEvent.m_sSourceId == parentRequestId
				&& candidateEvent.m_sZoneId == linkedTown.m_sZoneId)
			{
				supportEvent = candidateEvent;
				break;
			}
		}
		return parentAfter && parentAfter.m_bCompleted
			&& parentAfter.m_bSupportApplied
			&& !parentAfter.m_bQuarantined
			&& parentAfter.m_iContractVersion
				== HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			&& supportEvent && supportEvent.m_bApplied
			&& supportEvent.m_iContractVersion == 0
			&& townRecord
			&& townRecord.m_iContractVersion
				== HST_TownInfluenceService.EXACT_CONTRACT_VERSION
			&& townRecord.m_sAuthorityFailure.IsEmpty();
	}

	protected bool ProveForgedUnreleasedChildQuarantine()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("forged_unreleased_child");
		HST_ZoneState parentZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_forged_child_parent_zone",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"18100 20 14200");
		HST_ZoneState childZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_forged_child_zone",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"18400 20 14200");
		parentZone.m_aLinkedZoneIds.Insert(childZone.m_sZoneId);
		m_Factory.AddCivilianZone(fixture.m_State, childZone.m_sZoneId, 60, 40);
		fixture.m_MapMarkers.SetProjectionBlocked(true);
		fixture.m_ZoneCapture.CaptureForResistance(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Strategic,
			fixture.m_Economy,
			fixture.m_Balance,
			parentZone.m_sZoneId,
			10);
		HST_OwnershipTransitionState parentReceipt = fixture.m_State.FindOwnershipTransition(
			parentZone.m_sActiveOwnershipTransitionRequestId);
		HST_OwnershipTransitionState childReceipt = fixture.m_State.FindOwnershipTransition(
			childZone.m_sLastOwnershipTransitionRequestId);
		if (childReceipt)
		{
			childReceipt.m_sCause = "admin";
			childReceipt.m_bApplyEnemyConsequences = false;
		}

		HST_CampaignSaveData forgedSave = new HST_CampaignSaveData();
		forgedSave.Capture(fixture.m_State);
		HST_OwnershipTransitionSaveValidationService validator = new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(forgedSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = forgedSave.Restore();
		HST_OwnershipTransitionState restoredParent;
		HST_OwnershipTransitionState restoredChild;
		HST_ZoneState restoredParentZone;
		HST_ZoneState restoredChildZone;
		if (restoredState && parentReceipt && childReceipt)
		{
			restoredParent = restoredState.FindOwnershipTransition(parentReceipt.m_sRequestId);
			restoredChild = restoredState.FindOwnershipTransition(childReceipt.m_sRequestId);
			restoredParentZone = restoredState.FindZone(parentZone.m_sZoneId);
			restoredChildZone = restoredState.FindZone(childZone.m_sZoneId);
		}
		return parentReceipt
			&& childReceipt
			&& !childReceipt.m_bDeferredPublicationReleased
			&& restoredParent
			&& restoredParent.m_bQuarantined
			&& restoredParent.m_sFailureReason.Contains("noncausal unreleased child")
			&& restoredChild
			&& restoredChild.m_bQuarantined
			&& restoredParentZone
			&& restoredParentZone.m_iOwnershipContractVersion == HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION
			&& restoredChildZone
			&& restoredChildZone.m_iOwnershipContractVersion == HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION
			&& CountCampaignEvents(forgedSave, HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID) == 1;
	}

	protected bool ProveConflictingCompletedHistoryQuarantine()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("conflicting_completed_history");
		HST_ZoneState zone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_history_zone",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"18700 20 14200");
		HST_OwnershipTransitionRequest firstRequest = fixture.m_Service.BuildRequest(
			fixture.m_State,
			zone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"history_first",
			"completed history first owner",
			0,
			"ownership_proof_history_first_request");
		firstRequest.m_bApplyEnemyConsequences = false;
		HST_OwnershipTransitionResult firstResult = fixture.m_Service.Apply(
			fixture.m_State,
			firstRequest);
		HST_OwnershipTransitionRequest secondRequest = fixture.m_Service.BuildRequest(
			fixture.m_State,
			zone.m_sZoneId,
			fixture.m_Preset.m_sInvaderFactionKey,
			"admin",
			"source_proof",
			"history_second",
			"completed history conflicting owner",
			0,
			"ownership_proof_history_second_request");
		secondRequest.m_bApplyEnemyConsequences = false;
		HST_OwnershipTransitionResult secondResult = fixture.m_Service.Apply(
			fixture.m_State,
			secondRequest);
		HST_OwnershipTransitionState firstReceipt = fixture.m_State.FindOwnershipTransition(
			firstRequest.m_sRequestId);
		HST_OwnershipTransitionState secondReceipt = fixture.m_State.FindOwnershipTransition(
			secondRequest.m_sRequestId);
		if (secondReceipt)
		{
			secondReceipt.m_iExpectedRevision = 1;
			secondReceipt.m_iAppliedRevision = 2;
			zone.m_iOwnershipRevision = 2;
			HST_MapMarkerState marker = fixture.m_State.FindMapMarker("hst_zone_" + zone.m_sZoneId);
			if (marker)
				marker.m_iSourceRevision = 2;
		}

		HST_CampaignSaveData conflictingSave = new HST_CampaignSaveData();
		conflictingSave.Capture(fixture.m_State);
		HST_OwnershipTransitionSaveValidationService validator = new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(conflictingSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = conflictingSave.Restore();
		HST_OwnershipTransitionState restoredFirst;
		HST_OwnershipTransitionState restoredSecond;
		HST_ZoneState restoredZone;
		if (restoredState)
		{
			restoredFirst = restoredState.FindOwnershipTransition(firstRequest.m_sRequestId);
			restoredSecond = restoredState.FindOwnershipTransition(secondRequest.m_sRequestId);
			restoredZone = restoredState.FindZone(zone.m_sZoneId);
		}
		return firstResult
			&& firstResult.m_bCompleted
			&& secondResult
			&& secondResult.m_bCompleted
			&& firstReceipt
			&& secondReceipt
			&& firstReceipt.m_iAppliedRevision == secondReceipt.m_iAppliedRevision
			&& firstReceipt.m_sNewOwnerFactionKey != secondReceipt.m_sNewOwnerFactionKey
			&& restoredFirst
			&& restoredFirst.m_bQuarantined
			&& restoredSecond
			&& restoredSecond.m_bQuarantined
			&& restoredSecond.m_sFailureReason.Contains("duplicate or non-increasing")
			&& restoredZone
			&& restoredZone.m_iOwnershipContractVersion == HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION
			&& CountCampaignEvents(conflictingSave, HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID) == 1;
	}

	protected bool ProveCausePolicyFailClosed()
	{
		HST_OwnershipTransitionProofFixture reasonFixture = m_Factory.Build("canonical_reason");
		HST_ZoneState reasonZone = m_Factory.AddZone(
			reasonFixture.m_State,
			"ownership_proof_canonical_reason_zone",
			reasonFixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"18750 20 14200");
		HST_OwnershipTransitionRequest blankReasonRequest = reasonFixture.m_Service.BuildRequest(
			reasonFixture.m_State,
			reasonZone.m_sZoneId,
			reasonFixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"canonical_reason",
			"   ",
			0,
			"ownership_proof_canonical_reason_request");
		blankReasonRequest.m_bApplyEnemyConsequences = false;
		HST_OwnershipTransitionResult blankReasonResult = reasonFixture.m_Service.Apply(
			reasonFixture.m_State,
			blankReasonRequest);
		HST_OwnershipTransitionState blankReasonReceipt = reasonFixture.m_State.FindOwnershipTransition(
			blankReasonRequest.m_sRequestId);
		bool strategicReasonExact;
		bool campaignReasonExact;
		if (blankReasonReceipt)
		{
			foreach (HST_StrategicEventState strategicEvent : reasonFixture.m_State.m_aStrategicEvents)
			{
				if (strategicEvent
					&& strategicEvent.m_sEventId == blankReasonReceipt.m_sStrategicEventId
					&& strategicEvent.m_sReason == blankReasonReceipt.m_sReason)
					strategicReasonExact = true;
			}
			foreach (HST_CampaignEventState campaignEvent : reasonFixture.m_State.m_aCampaignEvents)
			{
				if (campaignEvent
					&& campaignEvent.m_sEventId == blankReasonReceipt.m_sCampaignEventId
					&& campaignEvent.m_sReason == blankReasonReceipt.m_sReason)
					campaignReasonExact = true;
			}
		}
		bool canonicalReasonExact = blankReasonResult
			&& blankReasonResult.m_bCompleted
			&& blankReasonReceipt
			&& !blankReasonReceipt.m_sReason.Trim().IsEmpty()
			&& blankReasonReceipt.m_sReason == "ownership changed to "
				+ reasonFixture.m_Preset.m_sResistanceFactionKey
			&& strategicReasonExact
			&& campaignReasonExact;

		HST_OwnershipTransitionProofFixture runtime = m_Factory.Build("cause_policy_runtime");
		array<string> causes = {"admin", "debug_seed", "migration_repair", "political_support"};
		array<ref HST_OwnershipTransitionResult> rejectedResults = {};
		for (int runtimeIndex; runtimeIndex < causes.Count(); runtimeIndex++)
		{
			string cause = causes[runtimeIndex];
			HST_ZoneState zone = m_Factory.AddZone(
				runtime.m_State,
				"ownership_proof_policy_runtime_" + cause,
				runtime.m_Preset.m_sOccupierFactionKey,
				HST_EZoneType.HST_ZONE_RESOURCE,
				"19000 20 14200");
			int supportReward;
			if (cause == "political_support")
				supportReward = 1;
			HST_OwnershipTransitionRequest request = runtime.m_Service.BuildRequest(
				runtime.m_State,
				zone.m_sZoneId,
				runtime.m_Preset.m_sResistanceFactionKey,
				cause,
				"source_proof",
				"policy_runtime_" + cause,
				"malformed runtime cause policy " + cause,
				supportReward,
				"ownership_proof_policy_runtime_request_" + cause);
			if (cause == "debug_seed")
				request.m_bApplyEnemyConsequences = false;
			if (cause == "migration_repair")
			{
				request.m_bApplyEnemyConsequences = false;
				request.m_bNotify = false;
			}
			rejectedResults.Insert(runtime.m_Service.Apply(runtime.m_State, request));
		}
		bool runtimeRejected = runtime.m_State.m_aOwnershipTransitions.IsEmpty()
			&& runtime.m_State.m_aStrategicEvents.IsEmpty();
		foreach (HST_OwnershipTransitionResult rejectedResult : rejectedResults)
		{
			if (!rejectedResult || rejectedResult.m_bAccepted
				|| !rejectedResult.m_sFailureReason.Contains("ownership policy"))
				runtimeRejected = false;
		}

		HST_OwnershipTransitionProofFixture saved = m_Factory.Build("cause_policy_save");
		array<ref HST_OwnershipTransitionState> receipts = {};
		for (int saveIndex; saveIndex < causes.Count(); saveIndex++)
		{
			string savedCause = causes[saveIndex];
			HST_ZoneState savedZone = m_Factory.AddZone(
				saved.m_State,
				"ownership_proof_policy_save_" + savedCause,
				saved.m_Preset.m_sOccupierFactionKey,
				HST_EZoneType.HST_ZONE_RESOURCE,
				"19000 20 14500");
			HST_OwnershipTransitionRequest validRequest = saved.m_Service.BuildRequest(
				saved.m_State,
				savedZone.m_sZoneId,
				saved.m_Preset.m_sResistanceFactionKey,
				savedCause,
				"source_proof",
				"policy_save_" + savedCause,
				"valid cause policy before save corruption " + savedCause,
				0,
				"ownership_proof_policy_save_request_" + savedCause);
			if (savedCause == "admin" || savedCause == "debug_seed" || savedCause == "migration_repair")
				validRequest.m_bApplyEnemyConsequences = false;
			if (savedCause == "debug_seed" || savedCause == "migration_repair")
				validRequest.m_bNotify = false;
			if (savedCause == "migration_repair")
			{
				validRequest.m_bReconcileSecurity = false;
				validRequest.m_bCreateSecurity = false;
			}
			HST_OwnershipTransitionResult validResult = saved.m_Service.Apply(
				saved.m_State,
				validRequest);
			if (validResult && validResult.m_bCompleted)
				receipts.Insert(validResult.m_Transition);
		}
		if (receipts.Count() == 4)
		{
			receipts[0].m_iSupportReward = 1;
			receipts[1].m_bNotify = true;
			receipts[2].m_bReconcileSecurity = true;
			receipts[3].m_iSupportReward = 1;
		}

		HST_CampaignSaveData malformedSave = new HST_CampaignSaveData();
		malformedSave.Capture(saved.m_State);
		HST_OwnershipTransitionSaveValidationService validator = new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(malformedSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = malformedSave.Restore();
		bool saveRejected = receipts.Count() == 4 && restoredState
			&& CountCampaignEvents(malformedSave, HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID) == 1;
		if (saveRejected)
		{
			foreach (HST_OwnershipTransitionState sourceReceipt : receipts)
			{
				HST_OwnershipTransitionState restoredReceipt = restoredState.FindOwnershipTransition(
					sourceReceipt.m_sRequestId);
				HST_ZoneState restoredZone = restoredState.FindZone(sourceReceipt.m_sZoneId);
				if (!restoredReceipt || !restoredReceipt.m_bQuarantined
					|| !restoredReceipt.m_sFailureReason.Contains("ownership policy")
					|| !restoredZone
					|| restoredZone.m_iOwnershipContractVersion != HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION)
					saveRejected = false;
			}
		}
		return canonicalReasonExact && runtimeRejected && saveRejected;
	}

	protected bool ProveRevisionCeilingFailClosed()
	{
		HST_OwnershipTransitionProofFixture runtime = m_Factory.Build("revision_ceiling_runtime");
		HST_ZoneState runtimeZone = m_Factory.AddZone(
			runtime.m_State,
			"ownership_proof_revision_ceiling_runtime_zone",
			runtime.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"19800 20 14200");
		runtimeZone.m_iOwnershipRevision = int.MAX - 1;
		HST_OwnershipTransitionRequest runtimeRequest = runtime.m_Service.BuildRequest(
			runtime.m_State,
			runtimeZone.m_sZoneId,
			runtime.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"revision_ceiling_runtime",
			"revision ceiling must reject before admission",
			0,
			"ownership_proof_revision_ceiling_runtime_request");
		runtimeRequest.m_bApplyEnemyConsequences = false;
		HST_OwnershipTransitionResult runtimeResult = runtime.m_Service.Apply(
			runtime.m_State,
			runtimeRequest);
		bool runtimeExact = runtimeResult
			&& !runtimeResult.m_bAccepted
			&& runtimeResult.m_sFailureReason.Contains("expected revision")
			&& runtime.m_State.m_aOwnershipTransitions.IsEmpty()
			&& runtimeZone.m_iOwnershipRevision == int.MAX - 1;

		HST_OwnershipTransitionProofFixture saved = m_Factory.Build("revision_ceiling_save");
		HST_ZoneState savedZone = m_Factory.AddZone(
			saved.m_State,
			"ownership_proof_revision_ceiling_save_zone",
			saved.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"20000 20 14200");
		HST_OwnershipTransitionRequest savedRequest = saved.m_Service.BuildRequest(
			saved.m_State,
			savedZone.m_sZoneId,
			saved.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"revision_ceiling_save",
			"malformed saved revision ceiling",
			0,
			"ownership_proof_revision_ceiling_save_request");
		savedRequest.m_bApplyEnemyConsequences = false;
		savedRequest.m_bReconcileSecurity = false;
		savedRequest.m_bCreateSecurity = false;
		savedRequest.m_bNotify = false;
		HST_OwnershipTransitionState savedReceipt = saved.m_Service.AdmitPendingForProof(
			saved.m_State,
			savedZone,
			savedRequest);
		if (savedReceipt)
		{
			savedReceipt.m_iExpectedRevision = int.MAX - 1;
			savedReceipt.m_iAttemptCount = 1;
			savedReceipt.m_iLastAttemptAtSecond = savedReceipt.m_iCreatedAtSecond;
			savedReceipt.m_sStatus = "applying";
			savedZone.m_iOwnershipRevision = int.MAX - 1;
		}
		HST_CampaignSaveData malformedSave = new HST_CampaignSaveData();
		malformedSave.Capture(saved.m_State);
		HST_OwnershipTransitionSaveValidationService validator = new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(malformedSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = malformedSave.Restore();
		HST_OwnershipTransitionState restoredReceipt;
		if (restoredState)
			restoredReceipt = restoredState.FindOwnershipTransition(savedRequest.m_sRequestId);
		bool saveExact = savedReceipt
			&& restoredReceipt
			&& restoredReceipt.m_bQuarantined
			&& restoredReceipt.m_sFailureReason.Contains("cannot advance safely")
			&& CountCampaignEvents(malformedSave, HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID) == 1;
		return runtimeExact && saveExact;
	}

	protected bool ProveAppliedSupportEventCorrelationFailClosed()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("support_event_correlation");
		HST_ZoneState capturedZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_support_event_parent_zone",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"20200 20 14200");
		HST_ZoneState townZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_support_event_town_zone",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"20500 20 14200");
		capturedZone.m_aLinkedZoneIds.Insert(townZone.m_sZoneId);
		m_Factory.AddCivilianZone(fixture.m_State, townZone.m_sZoneId, 20, 80);
		bool captured = fixture.m_ZoneCapture.CaptureForResistance(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Strategic,
			fixture.m_Economy,
			fixture.m_Balance,
			capturedZone.m_sZoneId,
			10);
		HST_OwnershipTransitionState receipt = fixture.m_State.FindOwnershipTransition(
			capturedZone.m_sLastOwnershipTransitionRequestId);
		HST_TownInfluenceEventState originalEvent;
		if (!fixture.m_State.m_aTownInfluenceEvents.IsEmpty())
			originalEvent = fixture.m_State.m_aTownInfluenceEvents[0];
		if (originalEvent)
		{
			HST_TownInfluenceEventState duplicateEvent = new HST_TownInfluenceEventState();
			duplicateEvent.m_sEventId = originalEvent.m_sEventId;
			duplicateEvent.m_sZoneId = originalEvent.m_sZoneId;
			duplicateEvent.m_sKind = originalEvent.m_sKind;
			duplicateEvent.m_sSourceId = originalEvent.m_sSourceId;
			duplicateEvent.m_sReason = originalEvent.m_sReason;
			duplicateEvent.m_iCreatedAtSecond = originalEvent.m_iCreatedAtSecond;
			duplicateEvent.m_iFIASupportDelta = originalEvent.m_iFIASupportDelta + 1;
			duplicateEvent.m_iReputationDelta = originalEvent.m_iReputationDelta;
			duplicateEvent.m_bApplied = true;
			fixture.m_State.m_aTownInfluenceEvents.Insert(duplicateEvent);
		}
		HST_CampaignSaveData malformedSave = new HST_CampaignSaveData();
		malformedSave.Capture(fixture.m_State);
		HST_OwnershipTransitionSaveValidationService validator = new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(malformedSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = malformedSave.Restore();
		HST_OwnershipTransitionState restoredReceipt;
		HST_ZoneState restoredZone;
		if (restoredState && receipt)
		{
			restoredReceipt = restoredState.FindOwnershipTransition(receipt.m_sRequestId);
			restoredZone = restoredState.FindZone(capturedZone.m_sZoneId);
		}
		return captured
			&& receipt
			&& originalEvent
			&& restoredReceipt
			&& restoredReceipt.m_bQuarantined
			&& restoredReceipt.m_sFailureReason.Contains("one deterministic influence event")
			&& restoredZone
			&& restoredZone.m_iOwnershipContractVersion == HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION
			&& CountCampaignEvents(malformedSave, HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID) == 1;
	}

	protected bool ProveSupportTargetAuthorityFailClosed()
	{
		HST_OwnershipTransitionProofFixture scopeFixture;
		HST_OwnershipTransitionState scopeReceipt;
		string scopeFarZoneId;
		BuildPendingSupportTargetFixture(
			"forged_support_scope",
			scopeFixture,
			scopeReceipt,
			scopeFarZoneId);
		if (!scopeFixture || !scopeReceipt || scopeFarZoneId.IsEmpty()
			|| scopeReceipt.m_aSupportZoneIds.Count() != 2
			|| scopeReceipt.m_aAppliedSupportZoneIds.Count() != 2)
			return false;
		scopeReceipt.m_aSupportZoneIds.Insert(scopeFarZoneId);
		bool scopeExact = CorruptedReceiptQuarantines(
			scopeFixture,
			scopeReceipt,
			"deterministic admission set");

		HST_OwnershipTransitionProofFixture prefixFixture;
		HST_OwnershipTransitionState prefixReceipt;
		string prefixFarZoneId;
		BuildPendingSupportTargetFixture(
			"forged_support_prefix",
			prefixFixture,
			prefixReceipt,
			prefixFarZoneId);
		if (!prefixFixture || !prefixReceipt
			|| prefixReceipt.m_aSupportZoneIds.Count() != 2
			|| prefixReceipt.m_aAppliedSupportZoneIds.Count() != 2)
			return false;
		string firstAppliedZoneId = prefixReceipt.m_aAppliedSupportZoneIds[0];
		prefixReceipt.m_aAppliedSupportZoneIds[0]
			= prefixReceipt.m_aAppliedSupportZoneIds[1];
		prefixReceipt.m_aAppliedSupportZoneIds[1] = firstAppliedZoneId;
		bool prefixExact = CorruptedReceiptQuarantines(
			prefixFixture,
			prefixReceipt,
			"ordered retry prefix");
		return scopeExact && prefixExact;
	}

	protected void BuildPendingSupportTargetFixture(
		string suffix,
		out HST_OwnershipTransitionProofFixture fixture,
		out HST_OwnershipTransitionState receipt,
		out string farZoneId)
	{
		fixture = m_Factory.Build(suffix);
		HST_ZoneState sourceZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_support_source_" + suffix,
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"22000 20 14200");
		HST_ZoneState firstTown = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_support_a_" + suffix,
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"22300 20 14200");
		HST_ZoneState secondTown = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_support_b_" + suffix,
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"22600 20 14200");
		HST_ZoneState farTown = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_support_far_" + suffix,
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_TOWN,
			"25000 20 14200");
		farZoneId = farTown.m_sZoneId;
		sourceZone.m_aLinkedZoneIds.Insert(firstTown.m_sZoneId);
		m_Factory.AddCivilianZone(fixture.m_State, firstTown.m_sZoneId, 40, 40);
		m_Factory.AddCivilianZone(fixture.m_State, secondTown.m_sZoneId, 40, 40);
		m_Factory.AddCivilianZone(fixture.m_State, farTown.m_sZoneId, 40, 40);
		fixture.m_MapMarkers.SetProjectionBlocked(true);
		fixture.m_ZoneCapture.CaptureForResistance(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Strategic,
			fixture.m_Economy,
			fixture.m_Balance,
			sourceZone.m_sZoneId,
			10);
		receipt = fixture.m_State.FindOwnershipTransition(
			sourceZone.m_sActiveOwnershipTransitionRequestId);
	}

	protected bool ProveReceiptDerivedCorrelationFailClosed()
	{
		return ProveSharedStrategicEventQuarantine()
			&& ProveCampaignEventMismatchQuarantine()
			&& ProveGarrisonIdentityMismatchQuarantine()
			&& ProveCounterattackMismatchQuarantine()
			&& ProveMarkerEvidenceMismatchQuarantine()
			&& ProveQueuedStrategicEventIsolation();
	}

	protected bool ProveQueuedStrategicEventIsolation()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build(
			"queued_strategic_event_isolation");
		HST_ZoneState blockingZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_event_isolation_blocker",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"21600 20 14500");
		HST_ZoneState queuedZone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_event_isolation_queued",
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"21900 20 14500");
		queuedZone.m_iResistanceCaptureProgress = 73;
		fixture.m_MapMarkers.SetProjectionBlocked(true);
		HST_OwnershipTransitionRequest blockingRequest = fixture.m_Service.BuildRequest(
			fixture.m_State,
			blockingZone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"event_isolation_blocker",
			"strategic event isolation blocker",
			0,
			"ownership_proof_event_isolation_blocker_request");
		blockingRequest.m_bApplyEnemyConsequences = false;
		HST_OwnershipTransitionResult blockingResult = fixture.m_Service.Apply(
			fixture.m_State,
			blockingRequest);
		HST_OwnershipTransitionState blockingReceipt = fixture.m_State.FindOwnershipTransition(
			blockingRequest.m_sRequestId);

		HST_OwnershipTransitionRequest queuedRequest = fixture.m_Service.BuildRequest(
			fixture.m_State,
			queuedZone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"military_capture",
			"source_proof",
			"event_isolation_queued",
			"queued strategic event isolation proof",
			0,
			"ownership_proof_event_isolation_queued_request");
		HST_OwnershipTransitionResult queuedResult = fixture.m_Service.Apply(
			fixture.m_State,
			queuedRequest);
		HST_OwnershipTransitionState queuedReceipt = fixture.m_State.FindOwnershipTransition(
			queuedRequest.m_sRequestId);
		HST_StrategicEventState queuedEvent;
		if (queuedReceipt)
			queuedEvent = fixture.m_State.FindStrategicEvent(queuedReceipt.m_sStrategicEventId);
		bool queuedExact = blockingResult && !blockingResult.m_bCompleted
			&& blockingReceipt && blockingReceipt.m_bOwnerApplied
			&& queuedResult && queuedResult.m_bNeedsRetry
			&& queuedReceipt && !queuedReceipt.m_bOwnerApplied
			&& queuedEvent
			&& queuedEvent.m_iCaptureProgressBefore == 73
			&& queuedEvent.m_iCaptureProgressAfter == 73
			&& queuedEvent.m_iCaptureProgressDelta == 0;
		if (!queuedExact)
			return false;

		fixture.m_State.m_iFactionMoney += 137;
		fixture.m_State.m_iHR += 9;
		fixture.m_State.m_iHQKnowledge += 11;
		HST_FactionPoolState targetPool = fixture.m_State.FindFactionPool(
			fixture.m_Preset.m_sOccupierFactionKey);
		if (!targetPool)
			return false;
		targetPool.m_iAttackResources += 17;
		targetPool.m_iSupportResources += 13;
		fixture.m_MapMarkers.SetProjectionBlocked(false);
		HST_OwnershipTransitionResult blockerDrain;
		if (blockingReceipt)
			blockerDrain = fixture.m_Service.Apply(
				fixture.m_State,
				fixture.m_Service.BuildReplayRequestForProof(blockingReceipt));
		HST_OwnershipTransitionResult queuedDrain;
		if (queuedReceipt)
			queuedDrain = fixture.m_Service.Apply(
				fixture.m_State,
				fixture.m_Service.BuildReplayRequestForProof(queuedReceipt));
		queuedEvent = fixture.m_State.FindStrategicEvent(queuedReceipt.m_sStrategicEventId);
		bool eventExact = queuedExact && targetPool
			&& blockerDrain && blockerDrain.m_bCompleted
			&& queuedDrain && queuedDrain.m_bCompleted;
		if (eventExact)
			eventExact = queuedReceipt.m_bStrategicEventCompleted
				&& queuedEvent && queuedEvent.m_bApplied;
		if (eventExact)
			eventExact = queuedEvent.m_sOwnerBefore
					== queuedReceipt.m_sPreviousOwnerFactionKey
				&& queuedEvent.m_sOwnerAfter == queuedReceipt.m_sNewOwnerFactionKey;
		if (eventExact)
			eventExact = queuedEvent.m_iFactionMoneyDelta == 0
				&& queuedEvent.m_iHRDelta == 0
				&& queuedEvent.m_iHQKnowledgeDelta == 0
				&& queuedEvent.m_iHQKnowledgeBefore == 0
				&& queuedEvent.m_iHQKnowledgeAfter == 0;
		if (eventExact)
			eventExact = queuedEvent.m_iTownSupportDelta == 0
				&& queuedEvent.m_iSupportBefore == 0
				&& queuedEvent.m_iSupportAfter == 0
				&& queuedEvent.m_iAttackResourceDelta == 0
				&& queuedEvent.m_iSupportResourceDelta == 0;
		if (eventExact)
			eventExact = queuedEvent.m_iAggressionDelta
					== queuedReceipt.m_iAggressionApplied
				&& queuedEvent.m_iCaptureProgressBefore == 73
				&& queuedEvent.m_iCaptureProgressAfter == 0
				&& queuedEvent.m_iCaptureProgressDelta == -73;

		HST_CampaignSaveData completedSave = new HST_CampaignSaveData();
		completedSave.Capture(fixture.m_State);
		HST_OwnershipTransitionSaveValidationService validator
			= new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(completedSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = completedSave.Restore();
		HST_OwnershipTransitionState restoredReceipt;
		if (restoredState)
			restoredReceipt = restoredState.FindOwnershipTransition(queuedReceipt.m_sRequestId);
		return eventExact
			&& restoredReceipt
			&& !restoredReceipt.m_bQuarantined
			&& CountCampaignEvents(
				completedSave,
				HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID) == 0;
	}

	protected bool ProveSharedStrategicEventQuarantine()
	{
		HST_OwnershipTransitionProofFixture fixture = m_Factory.Build("shared_strategic_event");
		array<ref HST_OwnershipTransitionState> receipts = {};
		for (int zoneIndex; zoneIndex < 2; zoneIndex++)
		{
			HST_ZoneState zone = m_Factory.AddZone(
				fixture.m_State,
				"ownership_proof_shared_event_zone_" + zoneIndex,
				fixture.m_Preset.m_sOccupierFactionKey,
				HST_EZoneType.HST_ZONE_RESOURCE,
				"20800 20 14200");
			HST_OwnershipTransitionRequest request = fixture.m_Service.BuildRequest(
				fixture.m_State,
				zone.m_sZoneId,
				fixture.m_Preset.m_sResistanceFactionKey,
				"admin",
				"source_proof",
				"shared_strategic_event_" + zoneIndex,
				"shared strategic event proof",
				0,
				"ownership_proof_shared_event_request_" + zoneIndex);
			request.m_bApplyEnemyConsequences = false;
			HST_OwnershipTransitionResult result = fixture.m_Service.Apply(fixture.m_State, request);
			if (result && result.m_bCompleted)
				receipts.Insert(result.m_Transition);
		}
		if (receipts.Count() == 2)
			receipts[1].m_sStrategicEventId = receipts[0].m_sStrategicEventId;
		HST_CampaignSaveData malformedSave = new HST_CampaignSaveData();
		malformedSave.Capture(fixture.m_State);
		HST_OwnershipTransitionSaveValidationService validator = new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(malformedSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = malformedSave.Restore();
		if (receipts.Count() != 2 || !restoredState)
			return false;
		HST_OwnershipTransitionState first = restoredState.FindOwnershipTransition(receipts[0].m_sRequestId);
		HST_OwnershipTransitionState second = restoredState.FindOwnershipTransition(receipts[1].m_sRequestId);
		return first && first.m_bQuarantined
			&& second && second.m_bQuarantined
			&& (first.m_sFailureReason.Contains("shared") || second.m_sFailureReason.Contains("shared"))
			&& CountCampaignEvents(malformedSave, HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID) == 1;
	}

	protected bool ProveCampaignEventMismatchQuarantine()
	{
		HST_OwnershipTransitionProofFixture fixture;
		HST_OwnershipTransitionState receipt;
		BuildCompletedAdminCorrelationFixture("campaign_event_mismatch", 0, fixture, receipt);
		if (!fixture || !receipt)
			return false;
		foreach (HST_CampaignEventState eventRow : fixture.m_State.m_aCampaignEvents)
		{
			if (eventRow && eventRow.m_sEventId == receipt.m_sCampaignEventId)
				eventRow.m_sAggregateId = "ownership_proof_unrelated_zone";
		}
		return CorruptedReceiptQuarantines(fixture, receipt, "campaign event retained row");
	}

	protected bool ProveGarrisonIdentityMismatchQuarantine()
	{
		HST_OwnershipTransitionProofFixture fixture;
		HST_OwnershipTransitionState receipt;
		BuildCompletedAdminCorrelationFixture("garrison_identity_mismatch", 4, fixture, receipt);
		if (!fixture || !receipt || receipt.m_sNewGarrisonId.IsEmpty())
			return false;
		receipt.m_sNewGarrisonId = "ownership_proof_wrong_garrison_id";
		bool identityExact = CorruptedReceiptQuarantines(
			fixture,
			receipt,
			"garrison identity");

		HST_OwnershipTransitionProofFixture missingFixture = m_Factory.Build(
			"garrison_missing_preowner");
		HST_ZoneState missingZone = m_Factory.AddZone(
			missingFixture.m_State,
			"ownership_proof_garrison_missing_preowner_zone",
			missingFixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_OUTPOST,
			"21100 20 14500",
			4);
		HST_OwnershipTransitionRequest missingRequest = missingFixture.m_Service.BuildRequest(
			missingFixture.m_State,
			missingZone.m_sZoneId,
			missingFixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"garrison_missing_preowner",
			"missing retained preowner garrison proof",
			0,
			"ownership_proof_garrison_missing_preowner_request");
		missingRequest.m_bApplyEnemyConsequences = false;
		HST_OwnershipTransitionState missingReceipt
			= missingFixture.m_Service.AdmitPendingForProof(
				missingFixture.m_State,
				missingZone,
				missingRequest);
		if (!missingReceipt)
			return false;
		missingReceipt.m_iAttemptCount = 1;
		missingReceipt.m_iLastAttemptAtSecond = missingReceipt.m_iCreatedAtSecond;
		missingReceipt.m_sStatus = "applying";
		missingReceipt.m_bOldSecurityRetired = true;
		missingReceipt.m_bHostileRuntimeRetired = true;
		missingReceipt.m_bNewSecurityApplied = true;
		missingReceipt.m_sNewGarrisonId = HST_StableIdService.BuildGarrisonId(
			missingZone.m_sZoneId,
			missingFixture.m_Preset.m_sResistanceFactionKey);
		bool missingExact = CorruptedReceiptQuarantines(
			missingFixture,
			missingReceipt,
			"lacks required new-garrison authority");
		return identityExact && missingExact;
	}

	protected bool ProveCounterattackMismatchQuarantine()
	{
		HST_OwnershipTransitionProofFixture orderFixture;
		HST_OwnershipTransitionState orderReceipt;
		BuildCompletedMilitaryCorrelationFixture(
			"counterattack_order_mismatch",
			orderFixture,
			orderReceipt);
		if (!orderFixture || !orderReceipt)
			return false;
		orderReceipt.m_iCounterattackChance = 1;
		orderReceipt.m_iCounterattackRoll = 0;
		orderReceipt.m_bCounterattackSelected = true;
		orderReceipt.m_bCounterattackQueued = true;
		orderReceipt.m_sEnemyOrderId = "ownership_proof_missing_counterattack_order";
		orderReceipt.m_sEnemyConsequenceDecision = string.Format(
			"aggression %1 | counterattack chance %2 roll %3 selected %4 queued %5 order %6",
			orderReceipt.m_iAggressionApplied,
			orderReceipt.m_iCounterattackChance,
			orderReceipt.m_iCounterattackRoll,
			orderReceipt.m_bCounterattackSelected,
			orderReceipt.m_bCounterattackQueued,
			orderReceipt.m_sEnemyOrderId);
		bool orderExact = CorruptedReceiptQuarantines(
			orderFixture,
			orderReceipt,
			"counterattack order");

		HST_OwnershipTransitionProofFixture decisionFixture;
		HST_OwnershipTransitionState decisionReceipt;
		BuildCompletedMilitaryCorrelationFixture(
			"counterattack_decision_mismatch",
			decisionFixture,
			decisionReceipt);
		if (!decisionFixture || !decisionReceipt)
			return false;
		decisionReceipt.m_iCounterattackChance = 1;
		decisionReceipt.m_iCounterattackRoll = 1;
		decisionReceipt.m_bCounterattackSelected = true;
		decisionReceipt.m_bCounterattackQueued = false;
		decisionReceipt.m_sEnemyOrderId = "";
		bool decisionExact = CorruptedReceiptQuarantines(
			decisionFixture,
			decisionReceipt,
			"frozen counterattack selection");

		HST_OwnershipTransitionProofFixture nonApplicableFixture;
		HST_OwnershipTransitionState nonApplicableReceipt;
		BuildCompletedAdminCorrelationFixture(
			"nonapplicable_enemy_residue",
			0,
			nonApplicableFixture,
			nonApplicableReceipt);
		if (!nonApplicableFixture || !nonApplicableReceipt)
			return false;
		nonApplicableReceipt.m_iAggressionApplied = 1;
		HST_StrategicEventState nonApplicableEvent
			= nonApplicableFixture.m_State.FindStrategicEvent(
				nonApplicableReceipt.m_sStrategicEventId);
		if (!nonApplicableEvent)
			return false;
		nonApplicableEvent.m_iAggressionDelta = 1;
		bool nonApplicableExact = CorruptedReceiptQuarantines(
			nonApplicableFixture,
			nonApplicableReceipt,
			"non-applicable ownership enemy policy");
		return orderExact && decisionExact && nonApplicableExact;
	}

	protected bool ProveMarkerEvidenceMismatchQuarantine()
	{
		HST_OwnershipTransitionProofFixture fixture;
		HST_OwnershipTransitionState receipt;
		BuildCompletedAdminCorrelationFixture("marker_evidence_mismatch", 0, fixture, receipt);
		if (!fixture || !receipt)
			return false;
		receipt.m_iMarkerStreamSequence = fixture.m_State.m_iMarkerProjectionSequence + 1;
		return CorruptedReceiptQuarantines(fixture, receipt, "outside campaign bounds");
	}

	protected void BuildCompletedAdminCorrelationFixture(
		string suffix,
		int garrisonSlots,
		out HST_OwnershipTransitionProofFixture fixture,
		out HST_OwnershipTransitionState receipt)
	{
		fixture = m_Factory.Build(suffix);
		HST_ZoneState zone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_correlation_zone_" + suffix,
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"21000 20 14200",
			garrisonSlots);
		HST_OwnershipTransitionRequest request = fixture.m_Service.BuildRequest(
			fixture.m_State,
			zone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"admin",
			"source_proof",
			"correlation_" + suffix,
			"receipt correlation proof " + suffix,
			0,
			"ownership_proof_correlation_request_" + suffix);
		request.m_bApplyEnemyConsequences = false;
		HST_OwnershipTransitionResult result = fixture.m_Service.Apply(fixture.m_State, request);
		if (result && result.m_bCompleted)
			receipt = result.m_Transition;
	}

	protected void BuildCompletedMilitaryCorrelationFixture(
		string suffix,
		out HST_OwnershipTransitionProofFixture fixture,
		out HST_OwnershipTransitionState receipt)
	{
		fixture = m_Factory.Build(suffix);
		HST_ZoneState zone = m_Factory.AddZone(
			fixture.m_State,
			"ownership_proof_correlation_zone_" + suffix,
			fixture.m_Preset.m_sOccupierFactionKey,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"21300 20 14200");
		HST_OwnershipTransitionRequest request = fixture.m_Service.BuildRequest(
			fixture.m_State,
			zone.m_sZoneId,
			fixture.m_Preset.m_sResistanceFactionKey,
			"military_capture",
			"source_proof",
			"correlation_" + suffix,
			"receipt correlation proof " + suffix,
			0,
			"ownership_proof_correlation_request_" + suffix);
		HST_OwnershipTransitionResult result = fixture.m_Service.Apply(
			fixture.m_State,
			request);
		if (result && result.m_bCompleted)
			receipt = result.m_Transition;
	}

	protected bool CorruptedReceiptQuarantines(
		HST_OwnershipTransitionProofFixture fixture,
		HST_OwnershipTransitionState receipt,
		string expectedFailure)
	{
		if (!fixture || !receipt)
			return false;
		HST_CampaignSaveData malformedSave = new HST_CampaignSaveData();
		malformedSave.Capture(fixture.m_State);
		HST_OwnershipTransitionSaveValidationService validator = new HST_OwnershipTransitionSaveValidationService();
		validator.Normalize(malformedSave, HST_OwnershipTransitionService.SCHEMA_VERSION);
		HST_CampaignState restoredState = malformedSave.Restore();
		if (!restoredState)
			return false;
		HST_OwnershipTransitionState restoredReceipt = restoredState.FindOwnershipTransition(receipt.m_sRequestId);
		HST_ZoneState restoredZone = restoredState.FindZone(receipt.m_sZoneId);
		return restoredReceipt
			&& restoredReceipt.m_bQuarantined
			&& restoredReceipt.m_sFailureReason.Contains(expectedFailure)
			&& restoredZone
			&& restoredZone.m_iOwnershipContractVersion == HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION
			&& CountCampaignEvents(malformedSave, HST_OwnershipTransitionSaveValidationService.CONFLICT_EVENT_ID) == 1;
	}

	protected HST_OwnershipTransitionState BuildRetentionReceipt(
		string requestId,
		int completedAtSecond,
		bool completed,
		bool quarantined)
	{
		HST_OwnershipTransitionState transition = new HST_OwnershipTransitionState();
		transition.m_sRequestId = requestId;
		transition.m_sZoneId = "ownership_proof_retention_other";
		transition.m_bCompleted = completed;
		transition.m_iCompletedAtSecond = completedAtSecond;
		transition.m_bQuarantined = quarantined;
		return transition;
	}

	protected int CountCampaignEvents(HST_CampaignSaveData saveData, string eventId)
	{
		if (!saveData || eventId.IsEmpty())
			return 0;
		int count;
		foreach (HST_CampaignEventState eventState : saveData.m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				count++;
		}
		return count;
	}
}
