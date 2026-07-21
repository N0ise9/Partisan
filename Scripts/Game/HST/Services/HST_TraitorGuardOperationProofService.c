#ifdef ENABLE_DIAG
class HST_TraitorGuardOperationProofReport
{
	bool m_bAdmissionIsolationExact;
	bool m_bProjectionLifecycleExact;
	bool m_bSettlementExact;
	bool m_bRestoreMigrationExact;
	bool m_bCorruptionQuarantineExact;
	bool m_bMarkerStatusExact;
	string m_sAdmissionIsolationEvidence;
	string m_sProjectionLifecycleEvidence;
	string m_sSettlementEvidence;
	string m_sRestoreMigrationEvidence;
	string m_sCorruptionQuarantineEvidence;
	string m_sMarkerStatusEvidence;
}

// Source-only fixture factory for the Schema-56 traitor branch. It intentionally
// reuses the same queue and operation seams as the officer proof; no world,
// native entity, adapter-handle, or network substitute is installed here.
class HST_TraitorGuardOperationProofFixtureFactory : HST_MissionGuardOperationProofFixtureFactory
{
	static const string TRAITOR_PROOF_ZONE_PREFIX = "traitor_guard_proof_zone_";
	static const string TRAITOR_PROOF_MISSION_PREFIX = "traitor_guard_proof_mission_";
	static const string TRAITOR_PROOF_HVT_PREFAB = "{6985327711303700}Prefabs/Objects/HST/HST_MissionProp_HVT.et";

	override HST_MissionGuardOperationProofFixture BuildAdmittedFixture(string suffix)
	{
		HST_MissionGuardOperationProofFixture fixture = BuildPreparedFixture(suffix);
		if (!Prepared(fixture))
			return fixture;

		fixture.m_Admission = fixture.m_Service.AdmitNewMission(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Definition,
			fixture.m_Mission,
			fixture.m_MissionRuntime);
		if (!fixture.m_Admission || !fixture.m_Admission.m_bSuccess)
		{
			fixture.m_sFailureReason = "exact traitor guard proof admission failed";
			if (fixture.m_Admission && !fixture.m_Admission.m_sFailureReason.IsEmpty())
				fixture.m_sFailureReason = fixture.m_sFailureReason + ": " + fixture.m_Admission.m_sFailureReason;
			return fixture;
		}

		fixture.m_Operation = fixture.m_Admission.m_Operation;
		fixture.m_Manifest = fixture.m_Admission.m_Manifest;
		fixture.m_Batch = fixture.m_Admission.m_Batch;
		fixture.m_Group = fixture.m_Admission.m_Group;
		if (!fixture.m_Operation)
			fixture.m_Operation = fixture.m_State.FindOperation(fixture.m_Mission.m_sOperationId);
		if (!fixture.m_Manifest)
			fixture.m_Manifest = fixture.m_State.FindForceManifest(fixture.m_Mission.m_sManifestId);
		if (!fixture.m_Batch)
			fixture.m_Batch = fixture.m_State.FindForceSpawnResult(fixture.m_Mission.m_sSpawnResultId);
		if (!fixture.m_Group && fixture.m_Operation)
			fixture.m_Group = fixture.m_State.FindActiveGroup(fixture.m_Operation.m_sGroupId);
		if (!Ready(fixture))
			fixture.m_sFailureReason = "exact traitor guard proof committed graph is incomplete";
		return fixture;
	}

	override HST_MissionGuardOperationProofFixture BuildPreparedFixture(string suffix)
	{
		HST_MissionGuardOperationProofFixture fixture = new HST_MissionGuardOperationProofFixture();
		fixture.m_State = BuildTraitorState(suffix);
		fixture.m_Preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		fixture.m_Definition = FindDefinition(HST_MissionGuardOperationService.TRAITOR_MISSION_ID);
		fixture.m_Mission = BuildTraitorMission(suffix);
		fixture.m_Objective = BuildTraitorObjective(fixture.m_Mission);
		fixture.m_HVT = BuildTraitorHVT(fixture.m_Mission);
		fixture.m_MissionRuntime = new HST_MissionRuntimeService();
		fixture.m_Queue = new HST_ForceSpawnQueueService();
		fixture.m_Adapter = new HST_ForceSpawnAdapterService();
		fixture.m_PhysicalWar = new HST_PhysicalWarService();
		fixture.m_Service = new HST_MissionGuardOperationProofHarness();
		fixture.m_Service.SetRuntimeServices(fixture.m_Queue, fixture.m_Adapter, fixture.m_PhysicalWar);
		fixture.m_State.m_aActiveMissions.Insert(fixture.m_Mission);
		fixture.m_State.m_aMissionObjectives.Insert(fixture.m_Objective);
		fixture.m_State.m_aMissionAssets.Insert(fixture.m_HVT);

		fixture.m_bPreparedContract = fixture.m_Service.PrepareNewMissionContract(fixture.m_Mission);
		if (!fixture.m_bPreparedContract || !fixture.m_Definition)
		{
			fixture.m_sFailureReason = "exact traitor guard proof preparation failed";
			return fixture;
		}

		int missionsBefore = fixture.m_State.m_aActiveMissions.Count();
		int operationsBefore = fixture.m_State.m_aOperations.Count();
		int manifestsBefore = fixture.m_State.m_aForceManifests.Count();
		int batchesBefore = fixture.m_State.m_aForceSpawnResults.Count();
		int groupsBefore = fixture.m_State.m_aActiveGroups.Count();
		int objectivesBefore = fixture.m_State.m_aMissionObjectives.Count();
		int assetsBefore = fixture.m_State.m_aMissionAssets.Count();
		fixture.m_Preflight = fixture.m_Service.CanAdmitNewMission(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Definition,
			fixture.m_Mission,
			fixture.m_MissionRuntime);

		bool preflightAccepted = fixture.m_Preflight && fixture.m_Preflight.m_bSuccess;
		bool authorityRowsStable = fixture.m_State.m_aOperations.Count() == operationsBefore;
		authorityRowsStable = authorityRowsStable
			&& fixture.m_State.m_aForceManifests.Count() == manifestsBefore;
		authorityRowsStable = authorityRowsStable
			&& fixture.m_State.m_aForceSpawnResults.Count() == batchesBefore;
		authorityRowsStable = authorityRowsStable
			&& fixture.m_State.m_aActiveGroups.Count() == groupsBefore;
		bool missionRowsStable = fixture.m_State.m_aActiveMissions.Count() == missionsBefore;
		missionRowsStable = missionRowsStable
			&& fixture.m_State.m_aMissionObjectives.Count() == objectivesBefore;
		missionRowsStable = missionRowsStable
			&& fixture.m_State.m_aMissionAssets.Count() == assetsBefore;
		bool backlinksEmpty = fixture.m_Mission.m_sOperationId.IsEmpty();
		backlinksEmpty = backlinksEmpty && fixture.m_Mission.m_sManifestId.IsEmpty();
		backlinksEmpty = backlinksEmpty && fixture.m_Mission.m_sSpawnResultId.IsEmpty();
		fixture.m_bPreflightReadOnly = preflightAccepted && authorityRowsStable;
		fixture.m_bPreflightReadOnly = fixture.m_bPreflightReadOnly && missionRowsStable;
		fixture.m_bPreflightReadOnly = fixture.m_bPreflightReadOnly && backlinksEmpty;
		if (!fixture.m_bPreflightReadOnly)
		{
			fixture.m_sFailureReason = "exact traitor guard proof preflight was rejected or mutated state";
			if (fixture.m_Preflight && !fixture.m_Preflight.m_sFailureReason.IsEmpty())
				fixture.m_sFailureReason = fixture.m_sFailureReason + ": " + fixture.m_Preflight.m_sFailureReason;
		}
		return fixture;
	}

	protected HST_CampaignState BuildTraitorState(string suffix)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_sPresetId = "traitor_guard_proof";
		state.m_iCampaignSeed = 560056;
		state.m_iElapsedSeconds = 560;
		state.m_iWarLevel = 4;
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;

		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = BuildTraitorZoneId(suffix);
		zone.m_sDisplayName = "Traitor Guard Proof " + suffix;
		zone.m_sOwnerFactionKey = "US";
		zone.m_eType = HST_EZoneType.HST_ZONE_TOWN;
		zone.m_vPosition = "5200 20 5200";
		zone.m_iActivationRadiusMeters = 900;
		zone.m_iCaptureRadiusMeters = 140;
		zone.m_iGarrisonSlots = 32;
		state.m_aZones.Insert(zone);
		return state;
	}

	protected HST_ActiveMissionState BuildTraitorMission(string suffix)
	{
		HST_ActiveMissionState mission = new HST_ActiveMissionState();
		mission.m_sInstanceId = BuildTraitorMissionInstanceId(suffix);
		mission.m_sMissionId = HST_MissionGuardOperationService.TRAITOR_MISSION_ID;
		mission.m_sDisplayName = "Assassinate Traitor";
		mission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		mission.m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_PHYSICAL_MVP;
		mission.m_sTargetZoneId = BuildTraitorZoneId(suffix);
		mission.m_vTargetPosition = "5240 20 5240";
		mission.m_sRuntimePrimitive = "kill_hvt";
		mission.m_sRuntimeType = "kill_hvt";
		mission.m_sRuntimePhase = "active";
		mission.m_iStartedAtSecond = 500;
		mission.m_iRuntimeStartedAtSecond = 500;
		mission.m_iRemainingSeconds = 2400;
		return mission;
	}

	protected HST_MissionObjectiveState BuildTraitorObjective(HST_ActiveMissionState mission)
	{
		HST_MissionObjectiveState objective = new HST_MissionObjectiveState();
		objective.m_sObjectiveId = "traitor_guard_proof_hvt_objective_" + mission.m_sInstanceId;
		objective.m_sMissionInstanceId = mission.m_sInstanceId;
		objective.m_eType = HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET;
		objective.m_sLabel = "Eliminate the traitor";
		objective.m_sRequirementText = "Kill the HVT";
		objective.m_sTargetId = "hvt";
		objective.m_sTargetZoneId = mission.m_sTargetZoneId;
		objective.m_sRuntimePrimitive = "kill_hvt";
		objective.m_vPosition = mission.m_vTargetPosition;
		objective.m_iRequiredProgress = 1;
		return objective;
	}

	protected HST_MissionAssetState BuildTraitorHVT(HST_ActiveMissionState mission)
	{
		HST_MissionAssetState hvt = new HST_MissionAssetState();
		hvt.m_sAssetId = "traitor_guard_proof_hvt_" + mission.m_sInstanceId;
		hvt.m_sMissionInstanceId = mission.m_sInstanceId;
		hvt.m_sKind = "character";
		hvt.m_sRole = "hvt";
		hvt.m_sPrefab = TRAITOR_PROOF_HVT_PREFAB;
		hvt.m_sEntityId = "traitor_guard_proof_hvt_runtime_" + mission.m_sInstanceId;
		hvt.m_bSpawned = true;
		hvt.m_bAlive = true;
		hvt.m_vSourcePosition = mission.m_vTargetPosition;
		hvt.m_vTargetPosition = mission.m_vTargetPosition;
		hvt.m_vCurrentPosition = mission.m_vTargetPosition;
		hvt.m_vLastKnownPosition = mission.m_vTargetPosition;
		return hvt;
	}

	static string BuildTraitorZoneId(string suffix)
	{
		return TRAITOR_PROOF_ZONE_PREFIX + suffix;
	}

	static string BuildTraitorMissionInstanceId(string suffix)
	{
		return TRAITOR_PROOF_MISSION_PREFIX + suffix;
	}
}

class HST_TraitorGuardOperationProofService : HST_MissionGuardOperationProofService
{
	static const string UNCLAIMED_GATES = "unclaimed engine/package gates: native entities, adapter handles and native casualties, actual save/restart, rendered UI, owner-change, campaign setup, packaged networking/reconnect/JIP";
	protected ref HST_TraitorGuardOperationProofFixtureFactory m_TraitorFixtures = new HST_TraitorGuardOperationProofFixtureFactory();

	HST_TraitorGuardOperationProofReport RunTraitor()
	{
		HST_TraitorGuardOperationProofReport report = new HST_TraitorGuardOperationProofReport();
		ProveAdmissionIsolation(report);
		ProveProjectionLifecycle(report);
		ProveSettlement(report);
		ProveRestoreMigration(report);
		ProveCorruptionQuarantine(report);
		ProveMarkerStatus(report);
		return report;
	}

	protected void ProveAdmissionIsolation(HST_TraitorGuardOperationProofReport report)
	{
		HST_MissionGuardOperationProofFixture traitor = m_TraitorFixtures.BuildAdmittedFixture("admission");
		HST_MissionGuardOperationProofFixture rollback = m_TraitorFixtures.BuildAdmittedFixture("rollback");
		HST_MissionGuardOperationProofFixtureFactory officerFixtures = new HST_MissionGuardOperationProofFixtureFactory();
		HST_MissionGuardOperationProofFixture officer = officerFixtures.BuildAdmittedFixture("schema56_officer_coexistence");
		bool traitorReady = m_TraitorFixtures.Ready(traitor);
		bool rollbackReady = m_TraitorFixtures.Ready(rollback);
		bool officerReady = officerFixtures.Ready(officer);
		if (!traitorReady || !rollbackReady || !officerReady)
		{
			string unavailable = m_TraitorFixtures.Failure(traitor);
			unavailable = unavailable + " | " + m_TraitorFixtures.Failure(rollback);
			unavailable = unavailable + " | " + officerFixtures.Failure(officer);
			report.m_sAdmissionIsolationEvidence = WithUnclaimedGates(unavailable);
			return;
		}

		bool traitorContract = HST_MissionGuardOperationService.IsExactTraitorMission(traitor.m_Mission);
		traitorContract = traitorContract
			&& traitor.m_Mission.m_iOperationContractVersion == HST_MissionGuardOperationService.TRAITOR_CONTRACT_VERSION;
		traitorContract = traitorContract
			&& traitor.m_Operation.m_iContractVersion == HST_MissionGuardOperationService.TRAITOR_CONTRACT_VERSION;
		bool traitorPolicy = traitor.m_Manifest.m_sPolicyId == HST_MissionGuardOperationService.TRAITOR_POLICY_ID;
		traitorPolicy = traitorPolicy
			&& traitor.m_Manifest.m_sIntentId == HST_MissionGuardOperationService.TRAITOR_INTENT_ID;
		traitorPolicy = traitorPolicy && traitor.m_Manifest.m_sForceKind == HST_MissionGuardOperationService.EXACT_FORCE_KIND;
		bool rootShape = traitor.m_Manifest.m_aGroups.Count() == 1;
		HST_ForceManifestGroupState root;
		if (rootShape)
			root = traitor.m_Manifest.m_aGroups[0];
		rootShape = rootShape && root && root.m_sPrefab.Contains("NotSpawned");
		rootShape = rootShape && root.m_iExpectedMemberCount == traitor.m_Manifest.m_aMembers.Count();
		bool memberShape = traitor.m_Manifest.m_aMembers.Count() > 0;
		for (int memberIndex = 0; memberShape && memberIndex < traitor.m_Manifest.m_aMembers.Count(); memberIndex++)
		{
			HST_ForceManifestMemberState member = traitor.m_Manifest.m_aMembers[memberIndex];
			if (!member)
			{
				memberShape = false;
				continue;
			}
			if (member.m_sSlotId != HST_MissionGuardOperationService.BuildMemberSlotId(
				traitor.m_Mission.m_sInstanceId,
				memberIndex))
				memberShape = false;
			if (root && member.m_sGroupElementId != root.m_sElementId)
				memberShape = false;
		}

		bool hvtLinksEmpty = traitor.m_HVT.m_sOperationId.IsEmpty();
		hvtLinksEmpty = hvtLinksEmpty && traitor.m_HVT.m_sManifestId.IsEmpty();
		hvtLinksEmpty = hvtLinksEmpty && traitor.m_HVT.m_sManifestSlotId.IsEmpty();
		bool hvtOutsideManifest = traitor.m_Manifest.m_aAssets.Count() == 0;
		hvtOutsideManifest = hvtOutsideManifest && traitor.m_Group.m_sMissionAssetId.IsEmpty();
		bool hvtIsolation = hvtLinksEmpty && hvtOutsideManifest;
		bool zeroCost = traitor.m_Manifest.m_iMoneyCost == 0;
		zeroCost = zeroCost && traitor.m_Manifest.m_iHRCost == 0;
		zeroCost = zeroCost && traitor.m_Manifest.m_iAttackResourceCost == 0;
		zeroCost = zeroCost && traitor.m_Manifest.m_iSupportResourceCost == 0;

		HST_MissionGuardAdmissionResult replay = traitor.m_Service.AdmitNewMission(
			traitor.m_State,
			traitor.m_Preset,
			traitor.m_Definition,
			traitor.m_Mission,
			traitor.m_MissionRuntime);
		bool replayExact = replay && replay.m_bSuccess && replay.m_bAlreadyApplied;
		replayExact = replayExact && replay.m_Operation == traitor.m_Operation;
		replayExact = replayExact && CountOperation(traitor.m_State, traitor.m_Operation.m_sOperationId) == 1;
		replayExact = replayExact && CountManifest(traitor.m_State, traitor.m_Manifest.m_sManifestId) == 1;
		replayExact = replayExact && CountBatch(traitor.m_State, traitor.m_Batch.m_sResultId) == 1;
		replayExact = replayExact && CountGroup(traitor.m_State, traitor.m_Group.m_sGroupId) == 1;

		string rollbackOperationId = rollback.m_Operation.m_sOperationId;
		string rollbackManifestId = rollback.m_Manifest.m_sManifestId;
		string rollbackBatchId = rollback.m_Batch.m_sResultId;
		string rollbackGroupId = rollback.m_Group.m_sGroupId;
		bool rollbackAccepted = rollback.m_Service.RollbackAdmission(
			rollback.m_State,
			rollback.m_Mission,
			rollback.m_Operation,
			rollback.m_Manifest,
			rollback.m_Batch,
			rollback.m_Group,
			"deterministic Schema-56 traitor rollback");
		bool rollbackRowsRemoved = CountOperation(rollback.m_State, rollbackOperationId) == 0;
		rollbackRowsRemoved = rollbackRowsRemoved && CountManifest(rollback.m_State, rollbackManifestId) == 0;
		rollbackRowsRemoved = rollbackRowsRemoved && CountBatch(rollback.m_State, rollbackBatchId) == 0;
		rollbackRowsRemoved = rollbackRowsRemoved && CountGroup(rollback.m_State, rollbackGroupId) == 0;
		bool rollbackLinksCleared = rollback.m_Mission.m_sOperationId.IsEmpty();
		rollbackLinksCleared = rollbackLinksCleared && rollback.m_Mission.m_sManifestId.IsEmpty();
		rollbackLinksCleared = rollbackLinksCleared && rollback.m_Mission.m_sSpawnResultId.IsEmpty();
		rollbackLinksCleared = rollbackLinksCleared
			&& HST_MissionGuardOperationService.IsQuarantinedTraitorMission(rollback.m_Mission);
		bool rollbackExact = rollbackAccepted && rollbackRowsRemoved && rollbackLinksCleared;

		bool officerContract = HST_MissionGuardOperationService.IsExactOfficerMission(officer.m_Mission);
		officerContract = officerContract
			&& officer.m_Mission.m_iOperationContractVersion == HST_MissionGuardOperationService.OFFICER_CONTRACT_VERSION;
		officerContract = officerContract
			&& officer.m_Manifest.m_sPolicyId == HST_MissionGuardOperationService.OFFICER_POLICY_ID;
		HST_ActiveMissionState historicalTraitor = new HST_ActiveMissionState();
		historicalTraitor.m_sMissionId = HST_MissionGuardOperationService.TRAITOR_MISSION_ID;
		historicalTraitor.m_sInstanceId = "historical_pre56_traitor_contract_zero";
		HST_ActiveMissionState specops = new HST_ActiveMissionState();
		specops.m_sMissionId = "assassinate_specops";
		specops.m_sInstanceId = "historical_pre57_specops_contract_zero";
		bool legacyIsolation = !HST_MissionGuardOperationService.IsExactOrQuarantinedMission(historicalTraitor);
		legacyIsolation = legacyIsolation && historicalTraitor.m_iOperationContractVersion == 0;
		legacyIsolation = legacyIsolation
			&& !HST_MissionGuardOperationService.IsExactOrQuarantinedMission(specops);
		legacyIsolation = legacyIsolation && specops.m_iOperationContractVersion == 0;

		bool admittedExact = traitorContract && traitorPolicy;
		admittedExact = admittedExact && rootShape && memberShape;
		admittedExact = admittedExact && hvtIsolation && zeroCost;
		bool coexistenceExact = officerContract && legacyIsolation;
		report.m_bAdmissionIsolationExact = admittedExact && traitor.m_bPreflightReadOnly;
		report.m_bAdmissionIsolationExact = report.m_bAdmissionIsolationExact && replayExact;
		report.m_bAdmissionIsolationExact = report.m_bAdmissionIsolationExact && rollbackExact;
		report.m_bAdmissionIsolationExact = report.m_bAdmissionIsolationExact && coexistenceExact;

		string evidence = string.Format(
			"traitor contract/policy/root/member/HVT/zero-cost %1/%2/%3/%4/%5/%6",
			traitorContract,
			traitorPolicy,
			rootShape,
			memberShape,
			hvtIsolation,
			zeroCost);
		evidence = evidence + string.Format(
			" | preflight/replay/rollback %1/%2/%3 | officer1/historical-traitor0/historical-specops0 %4/%5/%6",
			traitor.m_bPreflightReadOnly,
			replayExact,
			rollbackExact,
			officerContract,
			historicalTraitor.m_iOperationContractVersion,
			specops.m_iOperationContractVersion);
		report.m_sAdmissionIsolationEvidence = WithUnclaimedGates(evidence);
	}

	protected void ProveProjectionLifecycle(HST_TraitorGuardOperationProofReport report)
	{
		HST_MissionGuardOperationProofFixture survivors = m_TraitorFixtures.BuildAdmittedFixture("projection_survivors");
		HST_MissionGuardOperationProofFixture eliminated = m_TraitorFixtures.BuildAdmittedFixture("projection_all_dead");
		bool survivorsReady = m_TraitorFixtures.Ready(survivors);
		bool eliminatedReady = m_TraitorFixtures.Ready(eliminated);
		if (!survivorsReady || !eliminatedReady)
		{
			string unavailable = m_TraitorFixtures.Failure(survivors);
			unavailable = unavailable + " | " + m_TraitorFixtures.Failure(eliminated);
			report.m_sProjectionLifecycleEvidence = WithUnclaimedGates(unavailable);
			return;
		}

		string hvtBefore = BuildHVTSnapshot(survivors.m_HVT);
		string objectiveBefore = BuildObjectiveSnapshot(survivors.m_Objective);
		int virtualCombatBefore = survivors.m_Operation.m_iVirtualCombatLastStepSecond;
		int projectionSecond = survivors.m_State.m_iElapsedSeconds + 10;
		bool released = ReleaseHeldProjection(survivors, projectionSecond);
		bool firstProjection = released
			&& CompleteProjection(survivors, projectionSecond + 1, "traitor_first");
		int expectedInitialLiving = survivors.m_Manifest.m_aMembers.Count();
		bool firstRosterExact = CountRegisteredMemberSlots(survivors.m_Batch) == expectedInitialLiving;
		firstProjection = firstProjection && firstRosterExact;

		HST_ForceManifestMemberState casualtyMember = survivors.m_Manifest.m_aMembers[0];
		HST_ForceSpawnSlotResultState casualtySlot;
		if (casualtyMember)
			casualtySlot = survivors.m_Batch.FindSlotResult(casualtyMember.m_sSlotId);
		string casualtyEntityId;
		if (casualtySlot)
			casualtyEntityId = casualtySlot.m_sEntityId;
		HST_ForceSpawnQueueCallbackResult casualty;
		if (casualtyMember && casualtySlot)
		{
			casualty = survivors.m_Queue.ConfirmRegisteredMemberCasualty(
				survivors.m_State.m_aForceSpawnResults,
				survivors.m_Manifest,
				survivors.m_Batch.m_sResultId,
				survivors.m_Batch.m_sProjectionId,
				casualtyMember.m_sSlotId,
				casualtyEntityId,
				projectionSecond + 5,
				"deterministic traitor-guard casualty");
		}
		bool casualtyAccepted = casualty && casualty.m_bAccepted;
		bool casualtyTombstone = casualtySlot
			&& casualtySlot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
		casualtyTombstone = casualtyTombstone && casualtySlot.m_bCasualtyConfirmed;
		casualtyTombstone = casualtyTombstone && casualtySlot.m_bEverAlive;
		int expectedSurvivors = Math.Max(0, expectedInitialLiving - 1);
		bool casualtyCountExact = survivors.m_Queue.CountConfirmedCasualtyMemberSlots(survivors.m_Batch) == 1;
		casualtyCountExact = casualtyCountExact
			&& survivors.m_Queue.CountDurableLivingMemberSlots(survivors.m_Batch) == expectedSurvivors;
		bool casualtyExact = casualtyAccepted && casualtyTombstone && casualtyCountExact;

		int foldSecond = projectionSecond + 20;
		HST_ForceSpawnQueueCallbackResult folded = survivors.m_Queue.RequeueSuccessfulProjectionForStrategicHold(
			survivors.m_State.m_aForceSpawnResults,
			survivors.m_Manifest,
			survivors.m_Batch.m_sResultId,
			survivors.m_Batch.m_sProjectionId,
			foldSecond,
			foldSecond + HST_MissionGuardOperationService.DEPLOYMENT_GRACE_SECONDS);
		bool foldAccepted = folded && folded.m_bAccepted;
		bool heldExact = foldAccepted && survivors.m_Batch.m_bStrategicProjectionHeld;
		heldExact = heldExact
			&& survivors.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING;
		heldExact = heldExact && casualtySlot.m_bCasualtyConfirmed;
		heldExact = heldExact && casualtySlot.m_sEntityId.IsEmpty();
		bool secondReleased = heldExact && ReleaseHeldProjection(survivors, foldSecond + 1);
		bool secondProjection = secondReleased
			&& CompleteProjection(survivors, foldSecond + 2, "traitor_second");
		bool survivorProjectionExact = CountRegisteredMemberSlots(survivors.m_Batch) == expectedSurvivors;
		survivorProjectionExact = survivorProjectionExact
			&& survivors.m_Queue.CountConfirmedCasualtyMemberSlots(survivors.m_Batch) == 1;
		secondProjection = secondProjection && survivorProjectionExact;

		bool hvtUntouched = BuildHVTSnapshot(survivors.m_HVT) == hvtBefore;
		hvtUntouched = hvtUntouched
			&& BuildObjectiveSnapshot(survivors.m_Objective) == objectiveBefore;
		bool noVirtualCombat = survivors.m_Operation.m_iVirtualCombatLastStepSecond == virtualCombatBefore;

		bool allCasualties = ConfirmAllStrategicCasualties(eliminated);
		bool allDeadTick = false;
		if (allCasualties)
		{
			allDeadTick = eliminated.m_Service.TickVirtualForProof(
				eliminated.m_State,
				eliminated.m_Operation,
				eliminated.m_Mission,
				eliminated.m_Manifest,
				eliminated.m_Batch,
				eliminated.m_Group);
		}
		bool destroyedReceipt = allDeadTick
			&& eliminated.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		destroyedReceipt = destroyedReceipt
			&& eliminated.m_Operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED;
		destroyedReceipt = destroyedReceipt
			&& !eliminated.m_Operation.m_sSettlementId.IsEmpty();
		bool hvtStillActive = eliminated.m_Mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE;
		hvtStillActive = hvtStillActive && eliminated.m_HVT.m_bAlive;
		hvtStillActive = hvtStillActive && !eliminated.m_HVT.m_bDestroyed;
		hvtStillActive = hvtStillActive && !eliminated.m_Objective.m_bComplete;
		hvtStillActive = hvtStillActive && !eliminated.m_Objective.m_bFailed;
		bool allDeadExact = destroyedReceipt && hvtStillActive;

		report.m_bProjectionLifecycleExact = firstProjection && casualtyExact;
		report.m_bProjectionLifecycleExact = report.m_bProjectionLifecycleExact && heldExact;
		report.m_bProjectionLifecycleExact = report.m_bProjectionLifecycleExact && secondProjection;
		report.m_bProjectionLifecycleExact = report.m_bProjectionLifecycleExact && hvtUntouched;
		report.m_bProjectionLifecycleExact = report.m_bProjectionLifecycleExact && noVirtualCombat;
		report.m_bProjectionLifecycleExact = report.m_bProjectionLifecycleExact && allDeadExact;

		string evidence = string.Format(
			"traitor held-release/first/casualty/fold/reprojection %1/%2/%3/%4/%5",
			released,
			firstProjection,
			casualtyExact,
			heldExact,
			secondProjection);
		evidence = evidence + string.Format(
			" | living %1/%2 | no virtual combat %3 | HVT untouched %4 | all-dead guard/HVT-active %5",
			survivors.m_Queue.CountDurableLivingMemberSlots(survivors.m_Batch),
			expectedSurvivors,
			noVirtualCombat,
			hvtUntouched,
			allDeadExact);
		report.m_sProjectionLifecycleEvidence = WithUnclaimedGates(evidence);
	}

	protected void ProveSettlement(HST_TraitorGuardOperationProofReport report)
	{
		string successEvidence;
		string failureEvidence;
		string expiryEvidence;
		string campaignStopEvidence;
		string spawnFailureEvidence;
		bool successExact = ProveTraitorSettlementScenario(
			"settlement_success",
			"success",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED,
			successEvidence);
		bool failureExact = ProveTraitorSettlementScenario(
			"settlement_failure",
			"failure",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED,
			failureEvidence);
		bool expiryExact = ProveTraitorSettlementScenario(
			"settlement_expiry",
			"expiry",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED,
			expiryEvidence);
		bool campaignStopExact = ProveTraitorSettlementScenario(
			"settlement_campaign_stop",
			"campaign_stop",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED,
			campaignStopEvidence);
		bool spawnFailureExact = ProveTraitorSettlementScenario(
			"settlement_spawn_failure",
			"spawn_failure",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
			spawnFailureEvidence);

		report.m_bSettlementExact = successExact && failureExact;
		report.m_bSettlementExact = report.m_bSettlementExact && expiryExact;
		report.m_bSettlementExact = report.m_bSettlementExact && campaignStopExact;
		report.m_bSettlementExact = report.m_bSettlementExact && spawnFailureExact;
		string evidence = successEvidence + " | " + failureEvidence;
		evidence = evidence + " | " + expiryEvidence;
		evidence = evidence + " | " + campaignStopEvidence;
		evidence = evidence + " | " + spawnFailureEvidence;
		report.m_sSettlementEvidence = WithUnclaimedGates(evidence);
	}

	protected bool ProveTraitorSettlementScenario(
		string suffix,
		string mode,
		HST_EOperationTerminalResult expectedTerminal,
		out string evidence)
	{
		evidence = mode + " traitor fixture unavailable";
		HST_MissionGuardOperationProofFixture fixture = m_TraitorFixtures.BuildAdmittedFixture(suffix);
		if (!m_TraitorFixtures.Ready(fixture))
		{
			evidence = mode + " " + m_TraitorFixtures.Failure(fixture);
			return false;
		}

		bool triggerAccepted = true;
		bool firstChanged;
		if (mode == "success")
		{
			fixture.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_SUCCEEDED;
			fixture.m_HVT.m_bAlive = false;
			fixture.m_HVT.m_bDestroyed = true;
			fixture.m_Objective.m_bComplete = true;
			fixture.m_Objective.m_iCurrentProgress = fixture.m_Objective.m_iRequiredProgress;
			firstChanged = fixture.m_Service.ReconcileAfterMissionOutcomes(fixture.m_State);
		}
		else if (mode == "failure")
		{
			fixture.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
			firstChanged = fixture.m_Service.ReconcileAfterMissionOutcomes(fixture.m_State);
		}
		else if (mode == "expiry")
		{
			fixture.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_EXPIRED;
			firstChanged = fixture.m_Service.ReconcileAfterMissionOutcomes(fixture.m_State);
		}
		else if (mode == "campaign_stop")
		{
			firstChanged = fixture.m_Service.SettleOpenOperationsForCampaignStop(
				fixture.m_State,
				"deterministic Schema-56 campaign stop");
		}
		else if (mode == "spawn_failure")
		{
			HST_ForceSpawnQueueCallbackResult failed = fixture.m_Queue.FailProjectionFinal(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				fixture.m_State.m_iElapsedSeconds + 1,
				"deterministic Schema-56 traitor spawn failure");
			triggerAccepted = failed && failed.m_bAccepted;
			triggerAccepted = triggerAccepted
				&& fixture.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
			firstChanged = triggerAccepted
				&& fixture.m_Service.TickBeforePhysical(fixture.m_State, fixture.m_Preset);
		}
		else
		{
			return false;
		}

		string expectedReceipt = HST_OperationService.BuildSettlementId(
			fixture.m_Operation.m_sOperationId,
			HST_MissionGuardOperationService.SETTLEMENT_KIND);
		int revisionAfterFirst = fixture.m_Operation.m_iRevision;
		string receiptAfterFirst = fixture.m_Operation.m_sSettlementId;
		bool secondChanged;
		if (mode == "campaign_stop")
		{
			secondChanged = fixture.m_Service.SettleOpenOperationsForCampaignStop(
				fixture.m_State,
				"deterministic Schema-56 campaign stop");
		}
		else if (mode == "spawn_failure")
		{
			secondChanged = fixture.m_Service.TickBeforePhysical(fixture.m_State, fixture.m_Preset);
		}
		else
		{
			secondChanged = fixture.m_Service.ReconcileAfterMissionOutcomes(fixture.m_State);
		}

		bool receiptExact = !expectedReceipt.IsEmpty();
		receiptExact = receiptExact && receiptAfterFirst == expectedReceipt;
		receiptExact = receiptExact && fixture.m_Mission.m_sSettlementId == expectedReceipt;
		bool terminalExact = fixture.m_Operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		terminalExact = terminalExact && fixture.m_Operation.m_eTerminalResult == expectedTerminal;
		terminalExact = terminalExact
			&& fixture.m_Operation.m_iContractVersion == HST_MissionGuardOperationService.TRAITOR_CONTRACT_VERSION;
		bool idempotent = !secondChanged && fixture.m_Operation.m_iRevision == revisionAfterFirst;
		idempotent = idempotent && fixture.m_Operation.m_sSettlementId == receiptAfterFirst;
		bool noRefund = fixture.m_State.m_aResourceTransactions.Count() == 0;
		noRefund = noRefund && fixture.m_State.m_aForceSettlementTombstones.Count() == 0;
		noRefund = noRefund && fixture.m_Manifest.m_iMoneyCost == 0;
		noRefund = noRefund && fixture.m_Manifest.m_iHRCost == 0;
		bool runtimeCleaned = CountBatch(fixture.m_State, fixture.m_Operation.m_sSpawnResultId) == 0;
		runtimeCleaned = runtimeCleaned && CountGroup(fixture.m_State, fixture.m_Operation.m_sGroupId) == 0;
		bool missionOutcomeExact = true;
		if (mode == "campaign_stop")
			missionOutcomeExact = fixture.m_Mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED;
		else if (mode == "spawn_failure")
			missionOutcomeExact = fixture.m_Mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED;
		bool exact = triggerAccepted && firstChanged;
		exact = exact && receiptExact && terminalExact;
		exact = exact && idempotent && noRefund;
		exact = exact && runtimeCleaned && missionOutcomeExact;
		evidence = string.Format(
			"%1 changed/terminal/receipt/idempotent/zero-refund/clean %2/%3/%4/%5/%6/%7",
			mode,
			firstChanged,
			terminalExact,
			receiptExact,
			idempotent,
			noRefund,
			runtimeCleaned);
		return exact;
	}

	protected void ProveRestoreMigration(HST_TraitorGuardOperationProofReport report)
	{
		string casualtySlotId;
		string missionInstanceId;
		string hvtBefore;
		string objectiveBefore;
		HST_CampaignSaveData current = BuildTraitorPhysicalCasualtySave(
			"restore_current",
			missionInstanceId,
			casualtySlotId,
			hvtBefore,
			objectiveBefore);
		if (!current)
		{
			report.m_sRestoreMigrationEvidence = WithUnclaimedGates(
				"current Schema-56 traitor casualty fixture unavailable");
			return;
		}

		HST_AssassinationGuardSaveValidationService validator = new HST_AssassinationGuardSaveValidationService();
		validator.Normalize(current, 56);
		HST_ActiveMissionState restoredMission = FindSaveMission(current, missionInstanceId);
		HST_OperationRecordState restoredOperation;
		HST_ForceManifestState restoredManifest;
		HST_ForceSpawnResultState restoredBatch;
		HST_ActiveGroupState restoredGroup;
		if (restoredMission)
		{
			restoredOperation = FindSaveOperation(current, restoredMission.m_sOperationId);
			restoredManifest = FindSaveManifest(current, restoredMission.m_sManifestId);
			restoredBatch = FindSaveBatch(current, restoredMission.m_sSpawnResultId);
		}
		if (restoredOperation)
			restoredGroup = FindSaveGroup(current, restoredOperation.m_sGroupId);
		int revisionAfterFirst;
		if (restoredOperation)
			revisionAfterFirst = restoredOperation.m_iRevision;
		validator.Normalize(current, 56);

		bool rowsPresent = restoredMission && restoredOperation;
		rowsPresent = rowsPresent && restoredManifest && restoredBatch;
		rowsPresent = rowsPresent && restoredGroup;
		bool contractExact = rowsPresent
			&& HST_MissionGuardOperationService.IsExactTraitorMission(restoredMission);
		contractExact = contractExact
			&& restoredOperation.m_iContractVersion == HST_MissionGuardOperationService.TRAITOR_CONTRACT_VERSION;
		contractExact = contractExact
			&& restoredManifest.m_sPolicyId == HST_MissionGuardOperationService.TRAITOR_POLICY_ID;
		bool heldExact = rowsPresent
			&& restoredOperation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		heldExact = heldExact
			&& restoredOperation.m_ePositionAuthority
				== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		heldExact = heldExact && restoredBatch.m_bStrategicProjectionHeld;
		heldExact = heldExact
			&& restoredBatch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING;
		bool casualtyPreserved = rowsPresent;
		HST_ForceSpawnSlotResultState restoredCasualty;
		if (restoredBatch)
			restoredCasualty = restoredBatch.FindSlotResult(casualtySlotId);
		casualtyPreserved = casualtyPreserved && restoredCasualty;
		casualtyPreserved = casualtyPreserved && restoredCasualty.m_bCasualtyConfirmed;
		casualtyPreserved = casualtyPreserved
			&& restoredCasualty.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
		int restoredLiving;
		if (restoredBatch)
		{
			HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
			restoredLiving = queue.CountStrategicLivingMemberSlots(restoredBatch);
		}
		bool survivorCountsExact = rowsPresent && restoredGroup.m_iInfantryCount == restoredLiving;
		survivorCountsExact = survivorCountsExact
			&& restoredGroup.m_iDurableLivingInfantryCount == restoredLiving;
		survivorCountsExact = survivorCountsExact
			&& restoredOperation.m_iLastVirtualFriendlyCount == restoredLiving;
		bool processIdsCleared = rowsPresent && AllSaveSlotProcessIdsCleared(restoredBatch);
		processIdsCleared = processIdsCleared && !restoredGroup.m_bSpawnedEntity;
		processIdsCleared = processIdsCleared && restoredGroup.m_sRuntimeEntityId.IsEmpty();
		processIdsCleared = processIdsCleared && restoredGroup.m_iSpawnedAgentCount == 0;
		bool hvtBoundaryExact = rowsPresent
			&& BuildHVTSnapshot(FindSaveHVT(current, missionInstanceId)) == hvtBefore;
		hvtBoundaryExact = hvtBoundaryExact
			&& BuildObjectiveSnapshot(FindSaveHVTObjective(current, missionInstanceId)) == objectiveBefore;
		bool currentIdempotent = rowsPresent && restoredOperation.m_iRevision == revisionAfterFirst;
		bool currentExact = contractExact && heldExact;
		currentExact = currentExact && casualtyPreserved && survivorCountsExact;
		currentExact = currentExact && processIdsCleared && hvtBoundaryExact;
		currentExact = currentExact && currentIdempotent;

		string destroyedEvidence;
		bool destroyedActiveExact = ProveTraitorDestroyedActiveRestore(destroyedEvidence);
		HST_CampaignSaveData legacy = BuildLegacyPre56TraitorSave();
		validator.Normalize(legacy, 55);
		validator.Normalize(legacy, 55);
		HST_ActiveMissionState legacyMission = FindSaveMission(
			legacy,
			"traitor_guard_proof_legacy_pre56");
		bool legacyContractZero = legacyMission && legacyMission.m_iOperationContractVersion == 0;
		legacyContractZero = legacyContractZero
			&& !HST_MissionGuardOperationService.IsExactOrQuarantinedMission(legacyMission);
		bool noInventedAuthority = legacy.m_aOperations.Count() == 0;
		noInventedAuthority = noInventedAuthority && legacy.m_aForceManifests.Count() == 0;
		noInventedAuthority = noInventedAuthority && legacy.m_aForceSpawnResults.Count() == 0;
		noInventedAuthority = noInventedAuthority && legacy.m_aActiveGroups.Count() == 1;
		bool migrationEventExact = CountEvent(
			legacy,
			"migration_schema56_exact_traitor_guard") == 1;
		bool legacyExact = legacyContractZero && noInventedAuthority && migrationEventExact;

		report.m_bRestoreMigrationExact = currentExact && destroyedActiveExact;
		report.m_bRestoreMigrationExact = report.m_bRestoreMigrationExact && legacyExact;
		string evidence = string.Format(
			"current traitor contract/held/casualty/counts/process-clean/HVT/idempotent %1/%2/%3/%4/%5/%6/%7",
			contractExact,
			heldExact,
			casualtyPreserved,
			survivorCountsExact,
			processIdsCleared,
			hvtBoundaryExact,
			currentIdempotent);
		evidence = evidence + " | " + destroyedEvidence;
		evidence = evidence + string.Format(
			" | pre56 traitor contract0/no-invention/migration-once %1/%2/%3",
			legacyContractZero,
			noInventedAuthority,
			migrationEventExact);
		report.m_sRestoreMigrationEvidence = WithUnclaimedGates(evidence);
	}

	protected HST_CampaignSaveData BuildTraitorPhysicalCasualtySave(
		string suffix,
		out string missionInstanceId,
		out string casualtySlotId,
		out string hvtSnapshot,
		out string objectiveSnapshot)
	{
		missionInstanceId = "";
		casualtySlotId = "";
		hvtSnapshot = "";
		objectiveSnapshot = "";
		HST_MissionGuardOperationProofFixture fixture = m_TraitorFixtures.BuildAdmittedFixture(suffix);
		if (!m_TraitorFixtures.Ready(fixture))
			return null;
		int nowSecond = fixture.m_State.m_iElapsedSeconds + 10;
		if (!ReleaseHeldProjection(fixture, nowSecond))
			return null;
		if (!CompleteProjection(fixture, nowSecond + 1, "traitor_restore"))
			return null;

		HST_ForceManifestMemberState member = fixture.m_Manifest.m_aMembers[0];
		HST_ForceSpawnSlotResultState slot;
		if (member)
			slot = fixture.m_Batch.FindSlotResult(member.m_sSlotId);
		if (!member || !slot)
			return null;
		HST_ForceSpawnQueueCallbackResult casualty = fixture.m_Queue.ConfirmRegisteredMemberCasualty(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			member.m_sSlotId,
			slot.m_sEntityId,
			nowSecond + 5,
			"deterministic Schema-56 restore casualty");
		if (!casualty || !casualty.m_bAccepted)
			return null;

		int living = fixture.m_Queue.CountDurableLivingMemberSlots(fixture.m_Batch);
		fixture.m_Operation.m_eMaterializationState
			= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		fixture.m_Operation.m_ePositionAuthority
			= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		fixture.m_Operation.m_vStrategicPosition = fixture.m_Operation.m_vAssignmentPosition;
		fixture.m_Operation.m_iLastVirtualFriendlyCount = living;
		fixture.m_Group.m_bSpawnAttempted = true;
		fixture.m_Group.m_bSpawnedEntity = true;
		fixture.m_Group.m_bSpawnCompleted = true;
		fixture.m_Group.m_sRuntimeEntityId = "traitor_guard_proof_process_group";
		fixture.m_Group.m_sRuntimeStatus = "mission_guard_physical";
		fixture.m_Group.m_iSpawnedAgentCount = living;
		fixture.m_Group.m_iAssignedWaypointCount = 1;
		fixture.m_Group.m_iInfantryCount = living;
		fixture.m_Group.m_iLastSeenAliveCount = living;
		fixture.m_Group.m_iSurvivorInfantryCount = living;
		fixture.m_Group.m_iDurableLivingInfantryCount = living;
		fixture.m_Group.m_vPosition = fixture.m_Operation.m_vAssignmentPosition;
		missionInstanceId = fixture.m_Mission.m_sInstanceId;
		casualtySlotId = member.m_sSlotId;
		hvtSnapshot = BuildHVTSnapshot(fixture.m_HVT);
		objectiveSnapshot = BuildObjectiveSnapshot(fixture.m_Objective);
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(fixture.m_State);
		return save;
	}

	protected HST_CampaignSaveData BuildLegacyPre56TraitorSave()
	{
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.m_iSchemaVersion = 55;
		save.m_iLastLoadedSchemaVersion = 55;
		save.m_iElapsedSeconds = 100;
		HST_ActiveMissionState mission = new HST_ActiveMissionState();
		mission.m_sInstanceId = "traitor_guard_proof_legacy_pre56";
		mission.m_sMissionId = HST_MissionGuardOperationService.TRAITOR_MISSION_ID;
		mission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		mission.m_sTargetZoneId = "traitor_guard_proof_legacy_pre56_zone";
		mission.m_sRuntimePrimitive = "kill_hvt";
		mission.m_iOperationContractVersion = 0;
		save.m_aActiveMissions.Insert(mission);
		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = "mission_group_" + mission.m_sInstanceId;
		group.m_sMissionInstanceId = mission.m_sInstanceId;
		group.m_sZoneId = mission.m_sTargetZoneId;
		group.m_sFactionKey = "US";
		group.m_sRuntimeStatus = "mission_guard";
		group.m_iInfantryCount = 4;
		group.m_iSurvivorInfantryCount = 4;
		save.m_aActiveGroups.Insert(group);
		return save;
	}

	protected bool ProveTraitorDestroyedActiveRestore(out string evidence)
	{
		evidence = "destroyed-active traitor restore fixture unavailable";
		HST_MissionGuardOperationProofFixture fixture = m_TraitorFixtures.BuildAdmittedFixture(
			"restore_destroyed_active");
		if (!m_TraitorFixtures.Ready(fixture))
			return false;
		if (!ConfirmAllStrategicCasualties(fixture))
			return false;
		if (!fixture.m_Service.TickVirtualForProof(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Mission,
			fixture.m_Manifest,
			fixture.m_Batch,
			fixture.m_Group))
			return false;
		string hvtBefore = BuildHVTSnapshot(fixture.m_HVT);
		string objectiveBefore = BuildObjectiveSnapshot(fixture.m_Objective);
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(fixture.m_State);
		HST_AssassinationGuardSaveValidationService validator = new HST_AssassinationGuardSaveValidationService();
		validator.Normalize(save, 56);
		validator.Normalize(save, 56);
		HST_ActiveMissionState mission = FindSaveMission(save, fixture.m_Mission.m_sInstanceId);
		HST_OperationRecordState operation;
		if (mission)
			operation = FindSaveOperation(save, mission.m_sOperationId);
		bool rowsPresent = mission && operation;
		bool activeDestroyed = rowsPresent
			&& HST_MissionGuardOperationService.IsExactTraitorMission(mission);
		activeDestroyed = activeDestroyed && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE;
		activeDestroyed = activeDestroyed
			&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		activeDestroyed = activeDestroyed
			&& operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED;
		bool hvtBoundary = rowsPresent
			&& BuildHVTSnapshot(FindSaveHVT(save, mission.m_sInstanceId)) == hvtBefore;
		hvtBoundary = hvtBoundary
			&& BuildObjectiveSnapshot(FindSaveHVTObjective(save, mission.m_sInstanceId)) == objectiveBefore;
		bool compactProjection = rowsPresent
			&& FindSaveBatch(save, operation.m_sSpawnResultId) == null;
		compactProjection = compactProjection
			&& FindSaveGroup(save, operation.m_sGroupId) == null;
		bool exact = activeDestroyed && hvtBoundary && compactProjection;
		evidence = string.Format(
			"destroyed guard with active HVT compact restore active/boundary/clean %1/%2/%3",
			activeDestroyed,
			hvtBoundary,
			compactProjection);
		return exact;
	}

	protected void ProveCorruptionQuarantine(HST_TraitorGuardOperationProofReport report)
	{
		string duplicateEvidence;
		string hvtEvidence;
		bool duplicateExact = ProveDuplicateTraitorCorruption(duplicateEvidence);
		bool hvtExact = ProveTraitorHVTBacklinkCorruption(hvtEvidence);
		report.m_bCorruptionQuarantineExact = duplicateExact && hvtExact;
		report.m_sCorruptionQuarantineEvidence = WithUnclaimedGates(
			duplicateEvidence + " | " + hvtEvidence);
	}

	protected bool ProveDuplicateTraitorCorruption(out string evidence)
	{
		evidence = "duplicate traitor corruption fixture unavailable";
		HST_MissionGuardOperationProofFixture fixture = m_TraitorFixtures.BuildAdmittedFixture(
			"corruption_duplicate");
		if (!m_TraitorFixtures.Ready(fixture))
			return false;
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		HST_CampaignSaveData duplicateSource = new HST_CampaignSaveData();
		save.Capture(fixture.m_State);
		duplicateSource.Capture(fixture.m_State);
		if (duplicateSource.m_aOperations.Count() != 1)
			return false;
		save.m_aOperations.Insert(duplicateSource.m_aOperations[0]);
		string hvtBefore = BuildHVTSnapshot(FindSaveHVT(save, fixture.m_Mission.m_sInstanceId));
		int casualtiesBefore = CountSaveCasualties(FindSaveBatch(save, fixture.m_Batch.m_sResultId));
		HST_AssassinationGuardSaveValidationService validator = new HST_AssassinationGuardSaveValidationService();
		validator.Normalize(save, 56);
		HST_ActiveMissionState mission = FindSaveMission(save, fixture.m_Mission.m_sInstanceId);
		HST_ForceSpawnResultState batch = FindSaveBatch(save, fixture.m_Batch.m_sResultId);
		HST_ActiveGroupState group = FindSaveGroup(save, fixture.m_Group.m_sGroupId);
		bool operationQuarantine = save.m_aOperations.Count() == 2;
		foreach (HST_OperationRecordState operation : save.m_aOperations)
		{
			if (!operation || operation.m_iContractVersion
				!= HST_MissionGuardOperationService.TRAITOR_QUARANTINED_CONTRACT_VERSION)
				operationQuarantine = false;
		}
		bool missionQuarantine = mission
			&& HST_MissionGuardOperationService.IsQuarantinedTraitorMission(mission);
		bool runtimeHeld = batch && group;
		runtimeHeld = runtimeHeld && batch.m_bStrategicProjectionHeld;
		runtimeHeld = runtimeHeld && batch.m_bCancelRequested;
		runtimeHeld = runtimeHeld
			&& group.m_sRuntimeStatus == HST_MissionGuardOperationService.QUARANTINE_STATUS;
		bool boundaryHeld = mission && batch;
		boundaryHeld = boundaryHeld && CountSaveCasualties(batch) == casualtiesBefore;
		boundaryHeld = boundaryHeld
			&& BuildHVTSnapshot(FindSaveHVT(save, mission.m_sInstanceId)) == hvtBefore;
		boundaryHeld = boundaryHeld
			&& HasNoGuessedRefundOrFallback(save, mission.m_sInstanceId, 1);
		bool eventExact = CountEvent(
			save,
			"normalization_schema56_exact_traitor_guard_conflict") == 1;
		bool exact = operationQuarantine && missionQuarantine;
		exact = exact && runtimeHeld && boundaryHeld;
		exact = exact && eventExact;
		evidence = string.Format(
			"duplicate traitor ops -> mission/op -56, runtime held, HVT/casualty/no-refund, event %1/%2/%3/%4",
			missionQuarantine && operationQuarantine,
			runtimeHeld,
			boundaryHeld,
			eventExact);
		return exact;
	}

	protected bool ProveTraitorHVTBacklinkCorruption(out string evidence)
	{
		evidence = "traitor HVT-backlink corruption fixture unavailable";
		HST_MissionGuardOperationProofFixture fixture = m_TraitorFixtures.BuildAdmittedFixture(
			"corruption_hvt_backlink");
		if (!m_TraitorFixtures.Ready(fixture))
			return false;
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(fixture.m_State);
		HST_MissionAssetState hvt = FindSaveHVT(save, fixture.m_Mission.m_sInstanceId);
		if (!hvt)
			return false;
		hvt.m_sOperationId = fixture.m_Operation.m_sOperationId;
		hvt.m_sManifestId = fixture.m_Manifest.m_sManifestId;
		hvt.m_sManifestSlotId = fixture.m_Manifest.m_aMembers[0].m_sSlotId;
		string hvtBefore = BuildHVTSnapshot(hvt);
		HST_AssassinationGuardSaveValidationService validator = new HST_AssassinationGuardSaveValidationService();
		validator.Normalize(save, 56);
		HST_ActiveMissionState mission = FindSaveMission(save, fixture.m_Mission.m_sInstanceId);
		HST_OperationRecordState operation = FindSaveOperation(save, fixture.m_Operation.m_sOperationId);
		HST_ForceSpawnResultState batch = FindSaveBatch(save, fixture.m_Batch.m_sResultId);
		HST_ActiveGroupState group = FindSaveGroup(save, fixture.m_Group.m_sGroupId);
		bool rowsPresent = mission && operation;
		rowsPresent = rowsPresent && batch && group;
		bool quarantineExact = rowsPresent
			&& HST_MissionGuardOperationService.IsQuarantinedTraitorMission(mission);
		quarantineExact = quarantineExact
			&& operation.m_iContractVersion
				== HST_MissionGuardOperationService.TRAITOR_QUARANTINED_CONTRACT_VERSION;
		quarantineExact = quarantineExact
			&& group.m_sRuntimeStatus == HST_MissionGuardOperationService.QUARANTINE_STATUS;
		bool hvtUntouched = rowsPresent && BuildHVTSnapshot(hvt) == hvtBefore;
		hvtUntouched = hvtUntouched && hvt.m_bAlive && !hvt.m_bDestroyed;
		bool noInventedOutcome = rowsPresent && CountSaveCasualties(batch) == 0;
		noInventedOutcome = noInventedOutcome
			&& HasNoGuessedRefundOrFallback(save, mission.m_sInstanceId, 1);
		bool exact = quarantineExact && hvtUntouched && noInventedOutcome;
		evidence = string.Format(
			"illegal traitor HVT backlinks quarantine guard at -56 while HVT/no-refund remain unchanged %1/%2/%3",
			quarantineExact,
			hvtUntouched,
			noInventedOutcome);
		return exact;
	}

	protected void ProveMarkerStatus(HST_TraitorGuardOperationProofReport report)
	{
		HST_MissionGuardOperationProofFixture active = m_TraitorFixtures.BuildAdmittedFixture("marker_active");
		HST_MissionGuardOperationProofFixture casualty = m_TraitorFixtures.BuildAdmittedFixture("marker_casualty");
		HST_MissionGuardOperationProofFixture neutralized = m_TraitorFixtures.BuildAdmittedFixture("marker_neutralized");
		HST_MissionGuardOperationProofFixture quarantined = m_TraitorFixtures.BuildAdmittedFixture("marker_quarantined");
		bool fixturesReady = m_TraitorFixtures.Ready(active);
		fixturesReady = fixturesReady && m_TraitorFixtures.Ready(casualty);
		fixturesReady = fixturesReady && m_TraitorFixtures.Ready(neutralized);
		fixturesReady = fixturesReady && m_TraitorFixtures.Ready(quarantined);
		if (!fixturesReady)
		{
			report.m_sMarkerStatusEvidence = WithUnclaimedGates(
				"Schema-56 traitor marker fixtures unavailable");
			return;
		}

		HST_MissionGuardMarkerProofHarness markers = new HST_MissionGuardMarkerProofHarness();
		string activeStatus = HST_MissionGuardOperationService.BuildGuardStatusText(
			active.m_State,
			active.m_Mission);
		string activeLabel = markers.BuildHVTMarkerLabelForProof(
			active.m_State,
			active.m_Mission,
			active.m_HVT);
		string expectedActive = string.Format(
			"guards %1",
			active.m_Manifest.m_iAcceptedMemberCount);
		bool activeExact = activeStatus == expectedActive;
		activeExact = activeExact && CountToken(activeLabel, " | " + activeStatus) == 1;

		HST_ForceManifestMemberState casualtyMember = casualty.m_Manifest.m_aMembers[0];
		HST_ForceSpawnQueueCallbackResult casualtyResult;
		if (casualtyMember)
		{
			casualtyResult = casualty.m_Queue.ConfirmStrategicMemberCasualty(
				casualty.m_State.m_aForceSpawnResults,
				casualty.m_Manifest,
				casualty.m_Batch.m_sResultId,
				casualty.m_Batch.m_sProjectionId,
				casualtyMember.m_sSlotId,
				casualty.m_State.m_iElapsedSeconds + 1,
				"Schema-56 marker casualty proof");
		}
		string casualtyStatus = HST_MissionGuardOperationService.BuildGuardStatusText(
			casualty.m_State,
			casualty.m_Mission);
		string casualtyLabel = markers.BuildHVTMarkerLabelForProof(
			casualty.m_State,
			casualty.m_Mission,
			casualty.m_HVT);
		string expectedCasualty = string.Format(
			"guards %1",
			casualty.m_Manifest.m_iAcceptedMemberCount - 1);
		bool casualtyExact = casualtyResult && casualtyResult.m_bAccepted;
		casualtyExact = casualtyExact && casualtyStatus == expectedCasualty;
		casualtyExact = casualtyExact
			&& CountToken(casualtyLabel, " | " + casualtyStatus) == 1;

		bool neutralizedTransition = ConfirmAllStrategicCasualties(neutralized);
		if (neutralizedTransition)
		{
			neutralizedTransition = neutralized.m_Service.TickVirtualForProof(
				neutralized.m_State,
				neutralized.m_Operation,
				neutralized.m_Mission,
				neutralized.m_Manifest,
				neutralized.m_Batch,
				neutralized.m_Group);
		}
		string neutralizedStatus = HST_MissionGuardOperationService.BuildGuardStatusText(
			neutralized.m_State,
			neutralized.m_Mission);
		string neutralizedLabel = markers.BuildHVTMarkerLabelForProof(
			neutralized.m_State,
			neutralized.m_Mission,
			neutralized.m_HVT);
		bool neutralizedExact = neutralizedTransition && neutralizedStatus == "guards neutralized";
		neutralizedExact = neutralizedExact
			&& CountToken(neutralizedLabel, " | " + neutralizedStatus) == 1;

		bool quarantineTransition = quarantined.m_Service.QuarantineForProof(
			quarantined.m_State,
			quarantined.m_Operation,
			"Schema-56 marker quarantine proof");
		string quarantineStatus = HST_MissionGuardOperationService.BuildGuardStatusText(
			quarantined.m_State,
			quarantined.m_Mission);
		string quarantineLabel = markers.BuildHVTMarkerLabelForProof(
			quarantined.m_State,
			quarantined.m_Mission,
			quarantined.m_HVT);
		bool quarantineExact = quarantineTransition
			&& HST_MissionGuardOperationService.IsQuarantinedTraitorMission(quarantined.m_Mission);
		quarantineExact = quarantineExact && quarantineStatus == "guard authority unavailable";
		quarantineExact = quarantineExact
			&& CountToken(quarantineLabel, " | " + quarantineStatus) == 1;

		HST_CampaignState historicalState = new HST_CampaignState();
		HST_ActiveMissionState historical = new HST_ActiveMissionState();
		historical.m_sInstanceId = "traitor_guard_marker_historical_pre56";
		historical.m_sMissionId = HST_MissionGuardOperationService.TRAITOR_MISSION_ID;
		historical.m_sDisplayName = "Assassinate Traitor";
		historical.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		historical.m_iRemainingSeconds = 1200;
		historical.m_iOperationContractVersion = 0;
		HST_MissionAssetState historicalHVT = new HST_MissionAssetState();
		historicalHVT.m_sMissionInstanceId = historical.m_sInstanceId;
		historicalHVT.m_sRole = "hvt";
		string historicalStatus = HST_MissionGuardOperationService.BuildGuardStatusText(
			historicalState,
			historical);
		string historicalLabel = markers.BuildHVTMarkerLabelForProof(
			historicalState,
			historical,
			historicalHVT);
		bool historicalExact = historicalStatus.IsEmpty();
		historicalExact = historicalExact && !historicalLabel.Contains(" | guards");
		historicalExact = historicalExact
			&& !historicalLabel.Contains("guard authority unavailable");

		report.m_bMarkerStatusExact = activeExact && casualtyExact;
		report.m_bMarkerStatusExact = report.m_bMarkerStatusExact && neutralizedExact;
		report.m_bMarkerStatusExact = report.m_bMarkerStatusExact && quarantineExact;
		report.m_bMarkerStatusExact = report.m_bMarkerStatusExact && historicalExact;
		string evidence = string.Format(
			"existing traitor HVT-label suffix active/casualty/neutralized/-56/historical-empty %1/%2/%3/%4/%5",
			activeExact,
			casualtyExact,
			neutralizedExact,
			quarantineExact,
			historicalExact);
		report.m_sMarkerStatusEvidence = WithUnclaimedGates(evidence);
	}

	protected int CountEvent(HST_CampaignSaveData save, string eventId)
	{
		int count;
		if (!save || eventId.IsEmpty())
			return count;
		foreach (HST_CampaignEventState eventState : save.m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				count++;
		}
		return count;
	}

	protected string WithUnclaimedGates(string evidence)
	{
		return evidence + " | " + UNCLAIMED_GATES;
	}
}
#endif
