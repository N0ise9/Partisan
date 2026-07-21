#ifdef ENABLE_DIAG
class HST_MissionGuardOperationProofReport
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

// Source-only proof seam. Native entities, adapter handles, and world queries
// are deliberately not substituted by this harness.
class HST_MissionGuardOperationProofHarness : HST_MissionGuardOperationService
{
	bool TickVirtualForProof(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		return TickVirtual(state, operation, mission, manifest, batch, group);
	}

	bool QuarantineForProof(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		string reason)
	{
		return QuarantineOperationAuthority(state, operation, reason);
	}
}

class HST_MissionGuardMarkerProofHarness : HST_MapMarkerService
{
	string BuildHVTMarkerLabelForProof(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState hvt)
	{
		return BuildMissionAssetMarkerLabel(state, mission, hvt);
	}
}

class HST_MissionGuardOperationProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_MissionDefinition m_Definition;
	ref HST_ActiveMissionState m_Mission;
	ref HST_MissionObjectiveState m_Objective;
	ref HST_MissionAssetState m_HVT;
	ref HST_MissionRuntimeService m_MissionRuntime;
	ref HST_ForceSpawnQueueService m_Queue;
	ref HST_ForceSpawnAdapterService m_Adapter;
	ref HST_PhysicalWarService m_PhysicalWar;
	ref HST_MissionGuardOperationProofHarness m_Service;
	ref HST_MissionGuardAdmissionResult m_Preflight;
	ref HST_MissionGuardAdmissionResult m_Admission;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceManifestState m_Manifest;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
	bool m_bPreparedContract;
	bool m_bPreflightReadOnly;
	string m_sFailureReason;
}

class HST_MissionGuardOperationProofFixtureFactory
{
	static const string PROOF_ZONE_PREFIX = "mission_guard_proof_zone_";
	static const string PROOF_MISSION_PREFIX = "mission_guard_proof_mission_";
	static const string PROOF_HVT_PREFAB = "{6985327711303700}Prefabs/Objects/HST/HST_MissionProp_HVT.et";

	HST_MissionGuardOperationProofFixture BuildAdmittedFixture(string suffix)
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
			fixture.m_sFailureReason = "exact mission guard proof admission failed";
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
			fixture.m_sFailureReason = "exact mission guard proof committed graph is incomplete";
		return fixture;
	}

	HST_MissionGuardOperationProofFixture BuildPreparedFixture(string suffix)
	{
		HST_MissionGuardOperationProofFixture fixture = new HST_MissionGuardOperationProofFixture();
		fixture.m_State = BuildState(suffix);
		fixture.m_Preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		fixture.m_Definition = FindDefinition(HST_MissionGuardOperationService.EXACT_MISSION_ID);
		fixture.m_Mission = BuildMission(suffix);
		fixture.m_Objective = BuildObjective(fixture.m_Mission);
		fixture.m_HVT = BuildHVT(fixture.m_Mission);
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
			fixture.m_sFailureReason = "exact mission guard proof preparation failed";
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
		fixture.m_bPreflightReadOnly = fixture.m_Preflight && fixture.m_Preflight.m_bSuccess
			&& fixture.m_State.m_aActiveMissions.Count() == missionsBefore
			&& fixture.m_State.m_aOperations.Count() == operationsBefore
			&& fixture.m_State.m_aForceManifests.Count() == manifestsBefore
			&& fixture.m_State.m_aForceSpawnResults.Count() == batchesBefore
			&& fixture.m_State.m_aActiveGroups.Count() == groupsBefore
			&& fixture.m_State.m_aMissionObjectives.Count() == objectivesBefore
			&& fixture.m_State.m_aMissionAssets.Count() == assetsBefore
			&& fixture.m_Mission.m_sOperationId.IsEmpty()
			&& fixture.m_Mission.m_sManifestId.IsEmpty()
			&& fixture.m_Mission.m_sSpawnResultId.IsEmpty();
		if (!fixture.m_bPreflightReadOnly)
		{
			fixture.m_sFailureReason = "exact mission guard proof preflight was rejected or mutated state";
			if (fixture.m_Preflight && !fixture.m_Preflight.m_sFailureReason.IsEmpty())
				fixture.m_sFailureReason = fixture.m_sFailureReason + ": " + fixture.m_Preflight.m_sFailureReason;
		}
		return fixture;
	}

	bool Prepared(HST_MissionGuardOperationProofFixture fixture)
	{
		return fixture && fixture.m_State && fixture.m_Preset && fixture.m_Definition
			&& fixture.m_Mission && fixture.m_Objective && fixture.m_HVT
			&& fixture.m_MissionRuntime && fixture.m_Queue && fixture.m_Adapter
			&& fixture.m_PhysicalWar && fixture.m_Service
			&& fixture.m_bPreparedContract && fixture.m_bPreflightReadOnly;
	}

	bool Ready(HST_MissionGuardOperationProofFixture fixture)
	{
		return Prepared(fixture) && fixture.m_Admission && fixture.m_Admission.m_bSuccess
			&& fixture.m_Operation && fixture.m_Manifest && fixture.m_Batch && fixture.m_Group;
	}

	string Failure(HST_MissionGuardOperationProofFixture fixture)
	{
		if (!fixture)
			return "exact mission guard proof fixture is unavailable";
		if (!fixture.m_sFailureReason.IsEmpty())
			return fixture.m_sFailureReason;
		return "exact mission guard proof fixture is incomplete";
	}

	protected HST_CampaignState BuildState(string suffix)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_sPresetId = "mission_guard_proof";
		state.m_iCampaignSeed = 550055;
		state.m_iElapsedSeconds = 550;
		state.m_iWarLevel = 4;
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;

		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = BuildZoneId(suffix);
		zone.m_sDisplayName = "Mission Guard Proof " + suffix;
		zone.m_sOwnerFactionKey = "US";
		zone.m_eType = HST_EZoneType.HST_ZONE_TOWN;
		zone.m_vPosition = BuildZonePosition();
		zone.m_iActivationRadiusMeters = 900;
		zone.m_iCaptureRadiusMeters = 140;
		zone.m_iGarrisonSlots = 32;
		state.m_aZones.Insert(zone);
		return state;
	}

	protected HST_ActiveMissionState BuildMission(string suffix)
	{
		HST_ActiveMissionState mission = new HST_ActiveMissionState();
		mission.m_sInstanceId = BuildMissionInstanceId(suffix);
		mission.m_sMissionId = HST_MissionGuardOperationService.EXACT_MISSION_ID;
		mission.m_sDisplayName = "Assassinate Officer";
		mission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		mission.m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_PHYSICAL_MVP;
		mission.m_sTargetZoneId = BuildZoneId(suffix);
		mission.m_vTargetPosition = BuildHVTPosition();
		mission.m_sRuntimePrimitive = "kill_hvt";
		mission.m_sRuntimeType = "kill_hvt";
		mission.m_sRuntimePhase = "active";
		mission.m_iStartedAtSecond = 500;
		mission.m_iRuntimeStartedAtSecond = 500;
		mission.m_iRemainingSeconds = 2400;
		return mission;
	}

	protected HST_MissionObjectiveState BuildObjective(HST_ActiveMissionState mission)
	{
		HST_MissionObjectiveState objective = new HST_MissionObjectiveState();
		objective.m_sObjectiveId = "mission_guard_proof_hvt_objective_" + mission.m_sInstanceId;
		objective.m_sMissionInstanceId = mission.m_sInstanceId;
		objective.m_eType = HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET;
		objective.m_sLabel = "Eliminate the officer";
		objective.m_sRequirementText = "Kill the HVT";
		objective.m_sTargetId = "hvt";
		objective.m_sTargetZoneId = mission.m_sTargetZoneId;
		objective.m_sRuntimePrimitive = "kill_hvt";
		objective.m_vPosition = BuildHVTPosition();
		objective.m_iRequiredProgress = 1;
		return objective;
	}

	protected HST_MissionAssetState BuildHVT(HST_ActiveMissionState mission)
	{
		HST_MissionAssetState hvt = new HST_MissionAssetState();
		hvt.m_sAssetId = "mission_guard_proof_hvt_" + mission.m_sInstanceId;
		hvt.m_sMissionInstanceId = mission.m_sInstanceId;
		hvt.m_sKind = "character";
		hvt.m_sRole = "hvt";
		hvt.m_sPrefab = PROOF_HVT_PREFAB;
		hvt.m_sEntityId = "mission_guard_proof_hvt_runtime_" + mission.m_sInstanceId;
		hvt.m_bSpawned = true;
		hvt.m_bAlive = true;
		hvt.m_vSourcePosition = BuildHVTPosition();
		hvt.m_vTargetPosition = BuildHVTPosition();
		hvt.m_vCurrentPosition = BuildHVTPosition();
		hvt.m_vLastKnownPosition = BuildHVTPosition();
		return hvt;
	}

	protected HST_MissionDefinition FindDefinition(string missionId)
	{
		foreach (HST_MissionDefinition definition : HST_DefaultCatalog.CreateMissionRegistry())
		{
			if (definition && definition.m_sMissionId == missionId)
				return definition;
		}
		return null;
	}

	static string BuildZoneId(string suffix)
	{
		return PROOF_ZONE_PREFIX + suffix;
	}

	static string BuildMissionInstanceId(string suffix)
	{
		return PROOF_MISSION_PREFIX + suffix;
	}

	static vector BuildZonePosition()
	{
		return "5000 20 5000";
	}

	static vector BuildHVTPosition()
	{
		return "5040 20 5040";
	}
}

class HST_MissionGuardOperationProofService
{
	static const string PACKAGED_GATES = "packaged gates not claimed: native entities, real adapter handles, actual save/restart, rendered UI, owner-change, and campaign setup";
	protected ref HST_MissionGuardOperationProofFixtureFactory m_Fixtures = new HST_MissionGuardOperationProofFixtureFactory();

	HST_MissionGuardOperationProofReport Run()
	{
		HST_MissionGuardOperationProofReport report = new HST_MissionGuardOperationProofReport();
		ProveAdmissionIsolation(report);
		ProveProjectionLifecycle(report);
		ProveSettlement(report);
		ProveRestoreMigration(report);
		ProveCorruptionQuarantine(report);
		ProveMarkerStatus(report);
		return report;
	}

	protected void ProveAdmissionIsolation(HST_MissionGuardOperationProofReport report)
	{
		HST_MissionGuardOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("admission");
		HST_MissionGuardOperationProofFixture rollback = m_Fixtures.BuildAdmittedFixture("rollback");
		if (!m_Fixtures.Ready(fixture) || !m_Fixtures.Ready(rollback))
		{
			report.m_sAdmissionIsolationEvidence = WithPackagedGates(
				m_Fixtures.Failure(fixture) + " | " + m_Fixtures.Failure(rollback));
			return;
		}

		HST_MissionGuardAdmissionResult replay = fixture.m_Service.AdmitNewMission(
			fixture.m_State,
			fixture.m_Preset,
			fixture.m_Definition,
			fixture.m_Mission,
			fixture.m_MissionRuntime);
		bool replayIdentity = replay && replay.m_bSuccess && replay.m_bAlreadyApplied
			&& replay.m_Operation == fixture.m_Operation
			&& replay.m_Manifest == fixture.m_Manifest
			&& replay.m_Batch == fixture.m_Batch
			&& replay.m_Group == fixture.m_Group;
		bool replayCardinality = CountOperation(fixture.m_State, fixture.m_Operation.m_sOperationId) == 1
			&& CountManifest(fixture.m_State, fixture.m_Manifest.m_sManifestId) == 1
			&& CountBatch(fixture.m_State, fixture.m_Batch.m_sResultId) == 1
			&& CountGroup(fixture.m_State, fixture.m_Group.m_sGroupId) == 1;
		bool replayExact = replayIdentity && replayCardinality;

		bool admittedStructure = HST_MissionGuardOperationService.IsExactMissionGuardGroup(
			fixture.m_State,
			fixture.m_Group)
			&& fixture.m_Batch.m_bStrategicProjectionHeld
			&& fixture.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			&& fixture.m_Manifest.m_bFrozen;
		bool admittedZeroCost = fixture.m_Manifest.m_iMoneyCost == 0
			&& fixture.m_Manifest.m_iHRCost == 0
			&& fixture.m_Manifest.m_iAttackResourceCost == 0
			&& fixture.m_Manifest.m_iSupportResourceCost == 0;
		bool admittedExact = admittedStructure && admittedZeroCost;

		string rollbackOperationId = rollback.m_Operation.m_sOperationId;
		string rollbackManifestId = rollback.m_Manifest.m_sManifestId;
		string rollbackResultId = rollback.m_Batch.m_sResultId;
		string rollbackGroupId = rollback.m_Group.m_sGroupId;
		bool rolledBack = rollback.m_Service.RollbackAdmission(
			rollback.m_State,
			rollback.m_Mission,
			rollback.m_Operation,
			rollback.m_Manifest,
			rollback.m_Batch,
			rollback.m_Group,
			"deterministic proof rollback");
		bool rollbackRowsRemoved = CountOperation(rollback.m_State, rollbackOperationId) == 0
			&& CountManifest(rollback.m_State, rollbackManifestId) == 0
			&& CountBatch(rollback.m_State, rollbackResultId) == 0
			&& CountGroup(rollback.m_State, rollbackGroupId) == 0;
		bool rollbackLinksCleared = rollback.m_Mission.m_sOperationId.IsEmpty()
			&& rollback.m_Mission.m_sManifestId.IsEmpty()
			&& rollback.m_Mission.m_sSpawnResultId.IsEmpty()
			&& HST_MissionGuardOperationService.IsQuarantinedMission(rollback.m_Mission);
		bool rollbackExact = rolledBack && rollbackRowsRemoved && rollbackLinksCleared;

		HST_ActiveMissionState historical = new HST_ActiveMissionState();
		historical.m_sMissionId = HST_MissionGuardOperationService.EXACT_MISSION_ID;
		historical.m_sInstanceId = "historical_officer_contract_zero";
		HST_ActiveMissionState traitor = new HST_ActiveMissionState();
		traitor.m_sMissionId = "assassinate_traitor";
		traitor.m_sInstanceId = "historical_traitor_contract_zero";
		HST_ActiveMissionState specops = new HST_ActiveMissionState();
		specops.m_sMissionId = "assassinate_specops";
		specops.m_sInstanceId = "historical_pre57_specops_contract_zero";
		bool isolationExact = !HST_MissionGuardOperationService.IsExactOrQuarantinedMission(historical)
			&& !HST_MissionGuardOperationService.IsExactOrQuarantinedMission(traitor)
			&& !HST_MissionGuardOperationService.IsExactOrQuarantinedMission(specops)
			&& historical.m_iOperationContractVersion == 0
			&& traitor.m_iOperationContractVersion == 0
			&& specops.m_iOperationContractVersion == 0;

		report.m_bAdmissionIsolationExact = admittedExact && replayExact && rollbackExact && isolationExact;
		string admissionEvidence = string.Format(
			"new exact admission/preflight/replay/rollback %1/%2/%3/%4 | rows %5/%6/%7/%8",
			admittedExact,
			fixture.m_bPreflightReadOnly,
			replayExact,
			rollbackExact,
			CountOperation(fixture.m_State, fixture.m_Operation.m_sOperationId),
			CountManifest(fixture.m_State, fixture.m_Manifest.m_sManifestId),
			CountBatch(fixture.m_State, fixture.m_Batch.m_sResultId),
			CountGroup(fixture.m_State, fixture.m_Group.m_sGroupId));
		admissionEvidence = admissionEvidence + string.Format(
			" | historical/traitor/specops contracts %1/%2/%3",
			historical.m_iOperationContractVersion,
			traitor.m_iOperationContractVersion,
			specops.m_iOperationContractVersion);
		report.m_sAdmissionIsolationEvidence = WithPackagedGates(admissionEvidence);
	}

	protected void ProveProjectionLifecycle(HST_MissionGuardOperationProofReport report)
	{
		HST_MissionGuardOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("projection");
		HST_MissionGuardOperationProofFixture eliminated = m_Fixtures.BuildAdmittedFixture("all_dead");
		if (!m_Fixtures.Ready(fixture) || !m_Fixtures.Ready(eliminated))
		{
			report.m_sProjectionLifecycleEvidence = WithPackagedGates(
				m_Fixtures.Failure(fixture) + " | " + m_Fixtures.Failure(eliminated));
			return;
		}

		string hvtBefore = BuildHVTSnapshot(fixture.m_HVT);
		string objectiveBefore = BuildObjectiveSnapshot(fixture.m_Objective);
		HST_ForceSpawnSlotResultState rootSlot = FindRootSlot(fixture.m_Batch);
		bool manifestRosterShape = fixture.m_Manifest.m_aGroups.Count() == 1
			&& fixture.m_Manifest.m_aMembers.Count() > 0
			&& fixture.m_Manifest.m_aVehicles.Count() == 0
			&& fixture.m_Manifest.m_aAssets.Count() == 0;
		bool rootSlotShape = rootSlot
			&& rootSlot.m_sSlotId == fixture.m_Manifest.m_aGroups[0].m_sElementId
			&& fixture.m_Batch.m_iExpectedSlotCount == fixture.m_Manifest.m_aMembers.Count() + 1;
		bool emptyRootRosterExact = manifestRosterShape && rootSlotShape;

		int firstProjectionSecond = fixture.m_State.m_iElapsedSeconds + 10;
		bool released = ReleaseHeldProjection(fixture, firstProjectionSecond);
		bool firstProjection = released
			&& CompleteProjection(fixture, firstProjectionSecond + 1, "first")
			&& CountRegisteredMemberSlots(fixture.m_Batch) == fixture.m_Manifest.m_aMembers.Count();

		HST_ForceManifestMemberState casualtyMember = fixture.m_Manifest.m_aMembers[0];
		HST_ForceSpawnSlotResultState casualtySlot = fixture.m_Batch.FindSlotResult(casualtyMember.m_sSlotId);
		string casualtyEntityId;
		if (casualtySlot)
			casualtyEntityId = casualtySlot.m_sEntityId;
		HST_ForceSpawnQueueCallbackResult casualty = fixture.m_Queue.ConfirmRegisteredMemberCasualty(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			casualtyMember.m_sSlotId,
			casualtyEntityId,
			firstProjectionSecond + 5,
			"deterministic mapped guard casualty");
		bool casualtyCallbackExact = casualty && casualty.m_bAccepted && casualtySlot;
		bool casualtySlotExact = casualtySlot
			&& casualtySlot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
			&& casualtySlot.m_bCasualtyConfirmed && casualtySlot.m_bEverAlive;
		bool casualtyCountsExact = fixture.m_Queue.CountConfirmedCasualtyMemberSlots(fixture.m_Batch) == 1
			&& fixture.m_Queue.CountDurableLivingMemberSlots(fixture.m_Batch)
				== fixture.m_Manifest.m_aMembers.Count() - 1;
		bool casualtyExact = casualtyCallbackExact && casualtySlotExact && casualtyCountsExact;

		int reprojectionSecond = firstProjectionSecond + 20;
		HST_ForceSpawnQueueCallbackResult folded = fixture.m_Queue.RequeueSuccessfulProjectionForStrategicHold(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			reprojectionSecond,
			reprojectionSecond + HST_MissionGuardOperationService.DEPLOYMENT_GRACE_SECONDS);
		bool heldWithCasualty = folded && folded.m_bAccepted
			&& fixture.m_Batch.m_bStrategicProjectionHeld
			&& fixture.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			&& casualtySlot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
			&& casualtySlot.m_bCasualtyConfirmed && casualtySlot.m_sEntityId.IsEmpty();
		bool secondProjection = heldWithCasualty
			&& ReleaseHeldProjection(fixture, reprojectionSecond + 1)
			&& CompleteProjection(fixture, reprojectionSecond + 2, "second")
			&& CountRegisteredMemberSlots(fixture.m_Batch) == fixture.m_Manifest.m_aMembers.Count() - 1
			&& fixture.m_Queue.CountConfirmedCasualtyMemberSlots(fixture.m_Batch) == 1;
		bool missionBoundaryUntouched = hvtBefore == BuildHVTSnapshot(fixture.m_HVT)
			&& objectiveBefore == BuildObjectiveSnapshot(fixture.m_Objective);

		bool allCasualties = ConfirmAllStrategicCasualties(eliminated);
		bool guardSettled = allCasualties && eliminated.m_Service.TickVirtualForProof(
			eliminated.m_State,
			eliminated.m_Operation,
			eliminated.m_Mission,
			eliminated.m_Manifest,
			eliminated.m_Batch,
			eliminated.m_Group);
		bool allDeadSettlementExact = guardSettled
			&& eliminated.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& eliminated.m_Operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED
			&& !eliminated.m_Operation.m_sSettlementId.IsEmpty();
		bool allDeadBoundaryExact = eliminated.m_Mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			&& eliminated.m_Mission.m_sSettlementId == eliminated.m_Operation.m_sSettlementId
			&& eliminated.m_HVT.m_bAlive && !eliminated.m_Objective.m_bComplete;
		bool allDeadExact = allDeadSettlementExact && allDeadBoundaryExact;

		report.m_bProjectionLifecycleExact = emptyRootRosterExact && firstProjection && casualtyExact
			&& secondProjection && missionBoundaryUntouched && allDeadExact;
		string projectionEvidence = string.Format(
			"empty root/member roster %1/%2+%3 | held release/success %4/%5 | casualty/living %6/%7 | fold-held/reprojection %8/%9",
			fixture.m_Manifest.m_aGroups.Count(),
			fixture.m_Manifest.m_aMembers.Count(),
			rootSlot != null,
			released,
			firstProjection,
			fixture.m_Queue.CountConfirmedCasualtyMemberSlots(fixture.m_Batch),
			fixture.m_Queue.CountDurableLivingMemberSlots(fixture.m_Batch),
			heldWithCasualty,
			secondProjection);
		projectionEvidence = projectionEvidence + string.Format(
			" | HVT-objective untouched %1 | all-dead guard settled with mission active %2",
			missionBoundaryUntouched,
			allDeadExact);
		report.m_sProjectionLifecycleEvidence = WithPackagedGates(projectionEvidence);
	}

	protected void ProveSettlement(HST_MissionGuardOperationProofReport report)
	{
		string successEvidence;
		string failedEvidence;
		string expiredEvidence;
		string campaignStopEvidence;
		string spawnFailureEvidence;
		bool successExact = ProveSettlementScenario(
			"settle_success",
			"success",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED,
			successEvidence);
		bool failedExact = ProveSettlementScenario(
			"settle_failed",
			"failed",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED,
			failedEvidence);
		bool expiredExact = ProveSettlementScenario(
			"settle_expired",
			"expired",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED,
			expiredEvidence);
		bool campaignStopExact = ProveSettlementScenario(
			"settle_campaign_stop",
			"campaign_stop",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED,
			campaignStopEvidence);
		bool spawnFailureExact = ProveSettlementScenario(
			"settle_spawn_failed",
			"spawn_failed",
			HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED,
			spawnFailureEvidence);

		report.m_bSettlementExact = successExact && failedExact && expiredExact
			&& campaignStopExact && spawnFailureExact;
		report.m_sSettlementEvidence = WithPackagedGates(
			successEvidence + " | " + failedEvidence + " | " + expiredEvidence
			+ " | " + campaignStopEvidence + " | " + spawnFailureEvidence);
	}

	protected bool ProveSettlementScenario(
		string suffix,
		string mode,
		HST_EOperationTerminalResult expectedTerminal,
		out string evidence)
	{
		evidence = mode + " fixture unavailable";
		HST_MissionGuardOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture(suffix);
		if (!m_Fixtures.Ready(fixture))
		{
			evidence = mode + " " + m_Fixtures.Failure(fixture);
			return false;
		}

		bool firstChanged;
		bool secondChanged;
		bool triggerAccepted = true;
		if (mode == "success")
		{
			fixture.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_SUCCEEDED;
			fixture.m_HVT.m_bAlive = false;
			fixture.m_HVT.m_bDestroyed = true;
			fixture.m_Objective.m_bComplete = true;
			fixture.m_Objective.m_iCurrentProgress = fixture.m_Objective.m_iRequiredProgress;
			firstChanged = fixture.m_Service.ReconcileAfterMissionOutcomes(fixture.m_State);
		}
		else if (mode == "failed")
		{
			fixture.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
			firstChanged = fixture.m_Service.ReconcileAfterMissionOutcomes(fixture.m_State);
		}
		else if (mode == "expired")
		{
			fixture.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_EXPIRED;
			firstChanged = fixture.m_Service.ReconcileAfterMissionOutcomes(fixture.m_State);
		}
		else if (mode == "campaign_stop")
		{
			firstChanged = fixture.m_Service.SettleOpenOperationsForCampaignStop(
				fixture.m_State,
				"deterministic campaign-stop proof");
		}
		else if (mode == "spawn_failed")
		{
			HST_ForceSpawnQueueCallbackResult failed = fixture.m_Queue.FailProjectionFinal(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				fixture.m_State.m_iElapsedSeconds + 1,
				"deterministic coherent spawn failure");
			triggerAccepted = failed && failed.m_bAccepted
				&& fixture.m_Batch.m_eStatus
					== HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
			firstChanged = triggerAccepted
				&& fixture.m_Service.TickBeforePhysical(fixture.m_State, fixture.m_Preset);
		}
		else
			return false;

		string expectedSettlementId = HST_OperationService.BuildSettlementId(
			fixture.m_Operation.m_sOperationId,
			HST_MissionGuardOperationService.SETTLEMENT_KIND);
		int revisionAfterFirst = fixture.m_Operation.m_iRevision;
		string receiptAfterFirst = fixture.m_Operation.m_sSettlementId;
		if (mode == "campaign_stop")
			secondChanged = fixture.m_Service.SettleOpenOperationsForCampaignStop(
				fixture.m_State,
				"deterministic campaign-stop proof");
		else if (mode == "spawn_failed")
			secondChanged = fixture.m_Service.TickBeforePhysical(fixture.m_State, fixture.m_Preset);
		else
			secondChanged = fixture.m_Service.ReconcileAfterMissionOutcomes(fixture.m_State);

		bool fixedReceipt = !expectedSettlementId.IsEmpty()
			&& receiptAfterFirst == expectedSettlementId
			&& fixture.m_Operation.m_sSettlementId == expectedSettlementId
			&& fixture.m_Mission.m_sSettlementId == expectedSettlementId;
		bool idempotentSettlement = firstChanged && !secondChanged
			&& fixture.m_Operation.m_iRevision == revisionAfterFirst
			&& fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& fixture.m_Operation.m_eTerminalResult == expectedTerminal;
		bool oneFixedReceipt = fixedReceipt && idempotentSettlement;
		bool zeroManifestCost = fixture.m_Manifest.m_iMoneyCost == 0
			&& fixture.m_Manifest.m_iHRCost == 0
			&& fixture.m_Manifest.m_iEquipmentCost == 0
			&& fixture.m_Manifest.m_iAttackResourceCost == 0
			&& fixture.m_Manifest.m_iSupportResourceCost == 0;
		bool noRefundHistory = fixture.m_State.m_aResourceTransactions.Count() == 0
			&& fixture.m_State.m_aForceSettlementTombstones.Count() == 0;
		bool zeroRefund = zeroManifestCost && noRefundHistory;
		bool cleanupExact = CountBatch(fixture.m_State, fixture.m_Batch.m_sResultId) == 0
			&& CountGroup(fixture.m_State, fixture.m_Group.m_sGroupId) == 0
			&& CountOperation(fixture.m_State, fixture.m_Operation.m_sOperationId) == 1
			&& CountManifest(fixture.m_State, fixture.m_Manifest.m_sManifestId) == 1;
		evidence = string.Format(
			"%1 receipt fixed/idempotent/zero-refund/cleanup %2/%3/%4/%5",
			mode,
			oneFixedReceipt,
			!secondChanged,
			zeroRefund,
			cleanupExact);
		return triggerAccepted && oneFixedReceipt && zeroRefund && cleanupExact;
	}

	protected void ProveRestoreMigration(HST_MissionGuardOperationProofReport report)
	{
		string casualtySlotId;
		string hvtBefore;
		string objectiveBefore;
		HST_CampaignSaveData save = BuildPhysicalCasualtySave(
			"restore",
			casualtySlotId,
			hvtBefore,
			objectiveBefore);
		if (!save)
		{
			report.m_sRestoreMigrationEvidence = WithPackagedGates(
				"physical casualty restore fixture could not be built");
			return;
		}

		HST_AssassinationGuardSaveValidationService validator = new HST_AssassinationGuardSaveValidationService();
		validator.Normalize(save, 55);
		HST_ActiveMissionState mission = FindSaveMission(save, HST_MissionGuardOperationProofFixtureFactory.BuildMissionInstanceId("restore"));
		HST_OperationRecordState operation;
		HST_ForceSpawnResultState batch;
		HST_ActiveGroupState group;
		if (mission)
		{
			operation = FindSaveOperation(save, mission.m_sOperationId);
			batch = FindSaveBatch(save, mission.m_sSpawnResultId);
			if (operation)
				group = FindSaveGroup(save, operation.m_sGroupId);
		}
		HST_ForceSpawnSlotResultState casualtySlot;
		if (batch)
			casualtySlot = batch.FindSlotResult(casualtySlotId);
		int expectedLiving;
		HST_ForceSpawnQueueService restoredQueue = new HST_ForceSpawnQueueService();
		if (batch)
			expectedLiving = restoredQueue.CountStrategicLivingMemberSlots(batch);
		bool casualtyPreserved = casualtySlot
			&& casualtySlot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
			&& casualtySlot.m_bCasualtyConfirmed && casualtySlot.m_bEverAlive
			&& casualtySlot.m_sEntityId.IsEmpty() && casualtySlot.m_sNativeGroupId.IsEmpty();
		bool normalizedHeld = mission && operation && batch && group;
		bool operationNormalized;
		bool batchNormalized;
		bool groupNormalized;
		if (normalizedHeld)
		{
			operationNormalized = HST_MissionGuardOperationService.IsExactMission(mission)
				&& operation.m_eMaterializationState
					== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				&& operation.m_ePositionAuthority
					== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
			batchNormalized = batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
				&& batch.m_bStrategicProjectionHeld && batch.m_sNativeGroupId.IsEmpty()
				&& AllSaveSlotProcessIdsCleared(batch);
			groupNormalized = !group.m_bSpawnedEntity && group.m_sRuntimeEntityId.IsEmpty()
				&& group.m_iSpawnedAgentCount == 0 && group.m_iAssignedWaypointCount == 0
				&& group.m_sRuntimeStatus == "mission_guard_virtual"
				&& group.m_iDurableLivingInfantryCount == expectedLiving;
		}
		normalizedHeld = normalizedHeld && operationNormalized && batchNormalized && groupNormalized;
		HST_MissionAssetState restoredHVT;
		HST_MissionObjectiveState restoredObjective;
		if (mission)
		{
			restoredHVT = FindSaveHVT(save, mission.m_sInstanceId);
			restoredObjective = FindSaveHVTObjective(save, mission.m_sInstanceId);
		}
		bool missionBoundaryUntouched = BuildHVTSnapshot(restoredHVT) == hvtBefore
			&& BuildObjectiveSnapshot(restoredObjective) == objectiveBefore;
		float assignmentDistance;
		if (operation && restoredHVT)
			assignmentDistance = Distance2D(operation.m_vAssignmentPosition, restoredHVT.m_vSourcePosition);
		bool assignmentExact = operation && group && restoredHVT
			&& assignmentDistance >= 6.0 && assignmentDistance <= 30.0
			&& Distance2D(group.m_vSourcePosition, operation.m_vAssignmentPosition) <= 0.5
			&& Distance2D(group.m_vTargetPosition, operation.m_vAssignmentPosition) <= 0.5;

		HST_CampaignSaveData legacy = BuildLegacyPre55Save();
		validator.Normalize(legacy, 54);
		HST_ActiveMissionState legacyMission = FindSaveMission(legacy, "mission_guard_proof_legacy");
		HST_ActiveGroupState legacyGroup = FindSaveGroup(legacy, "mission_group_mission_guard_proof_legacy");
		bool legacyExact = legacyMission && legacyGroup
			&& legacyMission.m_iOperationContractVersion == 0
			&& legacyMission.m_sOperationId.IsEmpty()
			&& legacyGroup.m_sRuntimeStatus == "mission_guard"
			&& legacy.m_aForceManifests.Count() == 0
			&& legacy.m_aForceSpawnResults.Count() == 0;

		string destroyedActiveEvidence;
		string compactEvidence;
		bool destroyedActiveExact = ProveDestroyedActiveRestore(destroyedActiveEvidence);
		bool compactExact = ProveCompactSettledRestore(compactEvidence);
		report.m_bRestoreMigrationExact = casualtyPreserved && normalizedHeld
			&& missionBoundaryUntouched && assignmentExact && legacyExact
			&& destroyedActiveExact && compactExact;
		string restoreEvidence = string.Format(
			"casualty/held/process-clear/HVT-objective/anchor/pre55 %1/%2/%3/%4/%5/%6 | living %7 distance %8m",
			casualtyPreserved,
			normalizedHeld,
			AllSaveSlotProcessIdsCleared(batch),
			missionBoundaryUntouched,
			assignmentExact,
			legacyExact,
			expectedLiving,
			Math.Round(assignmentDistance));
		report.m_sRestoreMigrationEvidence = WithPackagedGates(
			restoreEvidence + " | " + destroyedActiveEvidence + " | " + compactEvidence);
	}

	protected void ProveCorruptionQuarantine(HST_MissionGuardOperationProofReport report)
	{
		string duplicateEvidence;
		string crossedEvidence;
		string foreignEvidence;
		string hvtEvidence;
		bool duplicateExact = ProveDuplicateCorruption(duplicateEvidence);
		bool crossedExact = ProveCrossedCorruption(crossedEvidence);
		bool foreignExact = ProveForeignProjectionCorruption(foreignEvidence);
		bool hvtExact = ProveHVTBacklinkCorruption(hvtEvidence);
		report.m_bCorruptionQuarantineExact = duplicateExact && crossedExact && foreignExact && hvtExact;
		report.m_sCorruptionQuarantineEvidence = WithPackagedGates(
			duplicateEvidence + " | " + crossedEvidence + " | " + foreignEvidence + " | " + hvtEvidence);
	}

	protected bool ProveDuplicateCorruption(out string evidence)
	{
		evidence = "duplicate corruption fixture unavailable";
		HST_MissionGuardOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("corrupt_duplicate");
		if (!m_Fixtures.Ready(fixture))
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
		validator.Normalize(save, 55);
		HST_ActiveMissionState mission = FindSaveMission(save, fixture.m_Mission.m_sInstanceId);
		HST_ForceSpawnResultState batch = FindSaveBatch(save, fixture.m_Batch.m_sResultId);
		HST_ActiveGroupState group = FindSaveGroup(save, fixture.m_Group.m_sGroupId);
		bool operationsHeld = save.m_aOperations.Count() == 2;
		foreach (HST_OperationRecordState operation : save.m_aOperations)
		{
			if (!operation || operation.m_iContractVersion
				!= HST_MissionGuardOperationService.QUARANTINED_CONTRACT_VERSION)
				operationsHeld = false;
		}
		bool aggregateHeld = mission && operationsHeld && batch && group;
		bool authorityQuarantined = aggregateHeld
			&& HST_MissionGuardOperationService.IsQuarantinedMission(mission)
			&& batch.m_bStrategicProjectionHeld && batch.m_bCancelRequested
			&& group.m_sRuntimeStatus == HST_MissionGuardOperationService.QUARANTINE_STATUS;
		bool boundaryHeld = aggregateHeld
			&& CountSaveCasualties(batch) == casualtiesBefore
			&& BuildHVTSnapshot(FindSaveHVT(save, mission.m_sInstanceId)) == hvtBefore
			&& HasNoGuessedRefundOrFallback(save, mission.m_sInstanceId, 1);
		bool exact = authorityQuarantined && boundaryHeld;
		evidence = string.Format(
			"duplicate operations/quarantined/unchanged-casualty-HVT/no-refund-fallback %1/%2/%3/%4",
			save.m_aOperations.Count(),
			mission && HST_MissionGuardOperationService.IsQuarantinedMission(mission),
			batch && CountSaveCasualties(batch) == casualtiesBefore,
			exact);
		return exact;
	}

	protected bool ProveCrossedCorruption(out string evidence)
	{
		evidence = "crossed corruption fixture unavailable";
		HST_MissionGuardOperationProofFixture leftFixture = m_Fixtures.BuildAdmittedFixture("corrupt_cross_left");
		HST_MissionGuardOperationProofFixture rightFixture = m_Fixtures.BuildAdmittedFixture("corrupt_cross_right");
		if (!m_Fixtures.Ready(leftFixture) || !m_Fixtures.Ready(rightFixture))
			return false;
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		HST_CampaignSaveData right = new HST_CampaignSaveData();
		save.Capture(leftFixture.m_State);
		right.Capture(rightFixture.m_State);
		AppendGuardAggregate(save, right);
		HST_ActiveGroupState leftGroup = FindSaveGroup(save, leftFixture.m_Group.m_sGroupId);
		HST_ActiveGroupState rightGroup = FindSaveGroup(save, rightFixture.m_Group.m_sGroupId);
		if (!leftGroup || !rightGroup)
			return false;
		string leftManifestId = leftGroup.m_sManifestId;
		leftGroup.m_sManifestId = rightGroup.m_sManifestId;
		rightGroup.m_sManifestId = leftManifestId;
		string leftHVTBefore = BuildHVTSnapshot(FindSaveHVT(save, leftFixture.m_Mission.m_sInstanceId));
		string rightHVTBefore = BuildHVTSnapshot(FindSaveHVT(save, rightFixture.m_Mission.m_sInstanceId));
		HST_AssassinationGuardSaveValidationService validator = new HST_AssassinationGuardSaveValidationService();
		validator.Normalize(save, 55);
		HST_ActiveMissionState leftMission = FindSaveMission(save, leftFixture.m_Mission.m_sInstanceId);
		HST_ActiveMissionState rightMission = FindSaveMission(save, rightFixture.m_Mission.m_sInstanceId);
		HST_OperationRecordState leftOperation = FindSaveOperation(save, leftFixture.m_Operation.m_sOperationId);
		HST_OperationRecordState rightOperation = FindSaveOperation(save, rightFixture.m_Operation.m_sOperationId);
		bool rowsHeld = leftMission && rightMission && leftOperation && rightOperation;
		bool authoritiesQuarantined = rowsHeld
			&& HST_MissionGuardOperationService.IsQuarantinedMission(leftMission)
			&& HST_MissionGuardOperationService.IsQuarantinedMission(rightMission)
			&& leftOperation.m_iContractVersion == HST_MissionGuardOperationService.QUARANTINED_CONTRACT_VERSION
			&& rightOperation.m_iContractVersion == HST_MissionGuardOperationService.QUARANTINED_CONTRACT_VERSION;
		bool groupsQuarantined = leftGroup.m_sRuntimeStatus == HST_MissionGuardOperationService.QUARANTINE_STATUS
			&& rightGroup.m_sRuntimeStatus == HST_MissionGuardOperationService.QUARANTINE_STATUS
			&& authoritiesQuarantined;
		bool boundariesHeld = rowsHeld
			&& BuildHVTSnapshot(FindSaveHVT(save, leftMission.m_sInstanceId)) == leftHVTBefore
			&& BuildHVTSnapshot(FindSaveHVT(save, rightMission.m_sInstanceId)) == rightHVTBefore
			&& HasNoGuessedRefundOrFallback(save, leftMission.m_sInstanceId, 2);
		bool exact = groupsQuarantined && boundariesHeld;
		evidence = string.Format(
			"crossed groups left/right quarantined with HVTs unchanged and no guessed death/refund/fallback %1/%2/%3",
			leftMission && HST_MissionGuardOperationService.IsQuarantinedMission(leftMission),
			rightMission && HST_MissionGuardOperationService.IsQuarantinedMission(rightMission),
			exact);
		return exact;
	}

	protected bool ProveForeignProjectionCorruption(out string evidence)
	{
		evidence = "foreign projection corruption fixture unavailable";
		HST_MissionGuardOperationProofFixture exactFixture = m_Fixtures.BuildAdmittedFixture("corrupt_foreign_exact");
		HST_MissionGuardOperationProofFixture foreignFixture = m_Fixtures.BuildAdmittedFixture("corrupt_foreign_row");
		if (!m_Fixtures.Ready(exactFixture) || !m_Fixtures.Ready(foreignFixture))
			return false;
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		HST_CampaignSaveData foreignSource = new HST_CampaignSaveData();
		save.Capture(exactFixture.m_State);
		foreignSource.Capture(foreignFixture.m_State);
		HST_ForceSpawnResultState foreignBatch = foreignSource.m_aForceSpawnResults[0];
		HST_ActiveGroupState foreignGroup = foreignSource.m_aActiveGroups[0];
		foreignBatch.m_sResultId = "foreign_projection_only_result";
		foreignBatch.m_sOperationId = "foreign_projection_only_operation";
		foreignBatch.m_sManifestId = "foreign_projection_only_manifest";
		foreignBatch.m_sForceId = "foreign_projection_only_force";
		foreignBatch.m_sProjectionId = exactFixture.m_Batch.m_sProjectionId;
		foreignBatch.m_bStrategicProjectionHeld = false;
		foreignBatch.m_bCancelRequested = false;
		foreignBatch.m_sLastFailureReason = "foreign_batch_sentinel";
		foreignGroup.m_sGroupId = "foreign_projection_only_group";
		foreignGroup.m_sOperationId = "foreign_projection_only_operation";
		foreignGroup.m_sManifestId = "foreign_projection_only_manifest";
		foreignGroup.m_sSpawnResultId = "foreign_projection_only_result";
		foreignGroup.m_sForceId = "foreign_projection_only_force";
		foreignGroup.m_sProjectionId = exactFixture.m_Group.m_sProjectionId;
		foreignGroup.m_sMissionInstanceId = "foreign_projection_only_mission";
		foreignGroup.m_sSpawnFallbackMode = "foreign_projection_only_mode";
		foreignGroup.m_sRuntimeStatus = "foreign_projection_only_idle";
		foreignGroup.m_sSpawnFailureReason = "foreign_group_sentinel";
		save.m_aForceSpawnResults.Insert(foreignBatch);
		save.m_aActiveGroups.Insert(foreignGroup);
		string foreignBatchBefore = BuildBatchIsolationSnapshot(foreignBatch);
		string foreignGroupBefore = BuildGroupIsolationSnapshot(foreignGroup);
		string hvtBefore = BuildHVTSnapshot(FindSaveHVT(save, exactFixture.m_Mission.m_sInstanceId));
		HST_AssassinationGuardSaveValidationService validator = new HST_AssassinationGuardSaveValidationService();
		validator.Normalize(save, 55);
		HST_ActiveMissionState mission = FindSaveMission(save, exactFixture.m_Mission.m_sInstanceId);
		HST_OperationRecordState operation = FindSaveOperation(save, exactFixture.m_Operation.m_sOperationId);
		HST_ForceSpawnResultState exactBatch = FindSaveBatch(save, exactFixture.m_Batch.m_sResultId);
		HST_ActiveGroupState exactGroup = FindSaveGroup(save, exactFixture.m_Group.m_sGroupId);
		bool foreignUntouched = BuildBatchIsolationSnapshot(foreignBatch) == foreignBatchBefore
			&& BuildGroupIsolationSnapshot(foreignGroup) == foreignGroupBefore;
		bool strongRowsPresent = mission && operation && exactBatch && exactGroup;
		bool strongRowsHeld = strongRowsPresent
			&& HST_MissionGuardOperationService.IsQuarantinedMission(mission)
			&& operation.m_iContractVersion == HST_MissionGuardOperationService.QUARANTINED_CONTRACT_VERSION
			&& exactBatch.m_bStrategicProjectionHeld && exactBatch.m_bCancelRequested
			&& exactGroup.m_sRuntimeStatus == HST_MissionGuardOperationService.QUARANTINE_STATUS;
		bool boundaryHeld = strongRowsPresent
			&& BuildHVTSnapshot(FindSaveHVT(save, mission.m_sInstanceId)) == hvtBefore
			&& HasNoGuessedRefundOrFallback(save, mission.m_sInstanceId, 2);
		bool exact = foreignUntouched && strongRowsHeld && boundaryHeld;
		evidence = string.Format(
			"projection-only foreign batch/group untouched %1 | strong op-linked rows held %2 | no guessed death/refund/fallback %3",
			foreignUntouched,
			strongRowsHeld,
			exact);
		return exact;
	}

	protected bool ProveHVTBacklinkCorruption(out string evidence)
	{
		evidence = "HVT backlink corruption fixture unavailable";
		HST_MissionGuardOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("corrupt_hvt_backlink");
		if (!m_Fixtures.Ready(fixture))
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
		validator.Normalize(save, 55);
		HST_ActiveMissionState mission = FindSaveMission(save, fixture.m_Mission.m_sInstanceId);
		HST_OperationRecordState operation = FindSaveOperation(save, fixture.m_Operation.m_sOperationId);
		HST_ForceSpawnResultState batch = FindSaveBatch(save, fixture.m_Batch.m_sResultId);
		HST_ActiveGroupState group = FindSaveGroup(save, fixture.m_Group.m_sGroupId);
		bool rowsHeld = mission && operation && batch && group;
		bool authorityQuarantined = rowsHeld
			&& HST_MissionGuardOperationService.IsQuarantinedMission(mission)
			&& operation.m_iContractVersion == HST_MissionGuardOperationService.QUARANTINED_CONTRACT_VERSION
			&& group.m_sRuntimeStatus == HST_MissionGuardOperationService.QUARANTINE_STATUS;
		bool boundaryHeld = rowsHeld
			&& BuildHVTSnapshot(hvt) == hvtBefore && hvt.m_bAlive && !hvt.m_bDestroyed
			&& CountSaveCasualties(batch) == 0
			&& HasNoGuessedRefundOrFallback(save, mission.m_sInstanceId, 1);
		bool exact = authorityQuarantined && boundaryHeld;
		evidence = string.Format(
			"surviving HVT illegal guard backlinks quarantine only guard authority while HVT row remains unchanged %1/%2",
			BuildHVTSnapshot(hvt) == hvtBefore,
			exact);
		return exact;
	}

	protected void ProveMarkerStatus(HST_MissionGuardOperationProofReport report)
	{
		HST_MissionGuardOperationProofFixture active = m_Fixtures.BuildAdmittedFixture("marker_active");
		HST_MissionGuardOperationProofFixture casualty = m_Fixtures.BuildAdmittedFixture("marker_casualty");
		HST_MissionGuardOperationProofFixture neutralized = m_Fixtures.BuildAdmittedFixture("marker_neutralized");
		HST_MissionGuardOperationProofFixture quarantined = m_Fixtures.BuildAdmittedFixture("marker_quarantine");
		if (!m_Fixtures.Ready(active) || !m_Fixtures.Ready(casualty)
			|| !m_Fixtures.Ready(neutralized) || !m_Fixtures.Ready(quarantined))
		{
			report.m_sMarkerStatusEvidence = WithPackagedGates(
				"marker status fixtures could not be admitted");
			return;
		}

		HST_MissionGuardMarkerProofHarness markers = new HST_MissionGuardMarkerProofHarness();
		string activeStatus = HST_MissionGuardOperationService.BuildGuardStatusText(
			active.m_State,
			active.m_Mission);
		string activeLabel = markers.BuildHVTMarkerLabelForProof(active.m_State, active.m_Mission, active.m_HVT);
		bool activeExact = activeStatus == string.Format(
			"guards %1",
			active.m_Manifest.m_iAcceptedMemberCount)
			&& CountToken(activeLabel, " | " + activeStatus) == 1;

		HST_ForceManifestMemberState casualtyMember = casualty.m_Manifest.m_aMembers[0];
		HST_ForceSpawnQueueCallbackResult casualtyResult = casualty.m_Queue.ConfirmStrategicMemberCasualty(
			casualty.m_State.m_aForceSpawnResults,
			casualty.m_Manifest,
			casualty.m_Batch.m_sResultId,
			casualty.m_Batch.m_sProjectionId,
			casualtyMember.m_sSlotId,
			casualty.m_State.m_iElapsedSeconds + 1,
			"marker casualty proof");
		string casualtyStatus = HST_MissionGuardOperationService.BuildGuardStatusText(
			casualty.m_State,
			casualty.m_Mission);
		string casualtyLabel = markers.BuildHVTMarkerLabelForProof(
			casualty.m_State,
			casualty.m_Mission,
			casualty.m_HVT);
		bool casualtyExact = casualtyResult && casualtyResult.m_bAccepted
			&& casualtyStatus == string.Format(
				"guards %1",
				casualty.m_Manifest.m_iAcceptedMemberCount - 1)
			&& CountToken(casualtyLabel, " | " + casualtyStatus) == 1;

		bool neutralizedTransition = ConfirmAllStrategicCasualties(neutralized)
			&& neutralized.m_Service.TickVirtualForProof(
				neutralized.m_State,
				neutralized.m_Operation,
				neutralized.m_Mission,
				neutralized.m_Manifest,
				neutralized.m_Batch,
				neutralized.m_Group);
		string neutralizedStatus = HST_MissionGuardOperationService.BuildGuardStatusText(
			neutralized.m_State,
			neutralized.m_Mission);
		string neutralizedLabel = markers.BuildHVTMarkerLabelForProof(
			neutralized.m_State,
			neutralized.m_Mission,
			neutralized.m_HVT);
		bool neutralizedExact = neutralizedTransition && neutralizedStatus == "guards neutralized"
			&& CountToken(neutralizedLabel, " | " + neutralizedStatus) == 1;

		bool quarantineTransition = quarantined.m_Service.QuarantineForProof(
			quarantined.m_State,
			quarantined.m_Operation,
			"marker quarantine proof");
		string quarantineStatus = HST_MissionGuardOperationService.BuildGuardStatusText(
			quarantined.m_State,
			quarantined.m_Mission);
		string quarantineLabel = markers.BuildHVTMarkerLabelForProof(
			quarantined.m_State,
			quarantined.m_Mission,
			quarantined.m_HVT);
		bool quarantineExact = quarantineTransition
			&& quarantineStatus == "guard authority unavailable"
			&& CountToken(quarantineLabel, " | " + quarantineStatus) == 1;

		HST_CampaignState legacyState = new HST_CampaignState();
		HST_ActiveMissionState legacyMission = new HST_ActiveMissionState();
		legacyMission.m_sInstanceId = "mission_guard_marker_legacy";
		legacyMission.m_sMissionId = HST_MissionGuardOperationService.EXACT_MISSION_ID;
		legacyMission.m_sDisplayName = "Assassinate Officer";
		legacyMission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		legacyMission.m_iRemainingSeconds = 1200;
		legacyMission.m_iOperationContractVersion = 0;
		HST_MissionAssetState legacyHVT = new HST_MissionAssetState();
		legacyHVT.m_sMissionInstanceId = legacyMission.m_sInstanceId;
		legacyHVT.m_sRole = "hvt";
		string legacyStatus = HST_MissionGuardOperationService.BuildGuardStatusText(legacyState, legacyMission);
		string legacyLabel = markers.BuildHVTMarkerLabelForProof(legacyState, legacyMission, legacyHVT);
		bool legacyExact = legacyStatus.IsEmpty()
			&& !legacyLabel.Contains(" | guards")
			&& !legacyLabel.Contains("guard authority unavailable");

		report.m_bMarkerStatusExact = activeExact && casualtyExact && neutralizedExact
			&& quarantineExact && legacyExact;
		report.m_sMarkerStatusEvidence = WithPackagedGates(string.Format(
			"one existing HVT-marker suffix active/casualty/neutralized/quarantine/legacy-empty %1/%2/%3/%4/%5",
			activeExact,
			casualtyExact,
			neutralizedExact,
			quarantineExact,
			legacyExact));
	}

	protected string WithPackagedGates(string evidence)
	{
		return evidence + " | " + PACKAGED_GATES;
	}

	protected int CountToken(string value, string token)
	{
		if (value.IsEmpty() || token.IsEmpty())
			return 0;
		string stripped = value;
		stripped.Replace(token, "");
		return (value.Length() - stripped.Length()) / token.Length();
	}

	protected float Distance2D(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}

	protected HST_CampaignSaveData BuildPhysicalCasualtySave(
		string suffix,
		out string casualtySlotId,
		out string hvtSnapshot,
		out string objectiveSnapshot)
	{
		casualtySlotId = "";
		hvtSnapshot = "";
		objectiveSnapshot = "";
		HST_MissionGuardOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture(suffix);
		if (!m_Fixtures.Ready(fixture))
			return null;
		int nowSecond = fixture.m_State.m_iElapsedSeconds + 10;
		if (!ReleaseHeldProjection(fixture, nowSecond)
			|| !CompleteProjection(fixture, nowSecond + 1, "restore"))
			return null;

		HST_ForceManifestMemberState member = fixture.m_Manifest.m_aMembers[0];
		HST_ForceSpawnSlotResultState slot = fixture.m_Batch.FindSlotResult(member.m_sSlotId);
		if (!slot)
			return null;
		HST_ForceSpawnQueueCallbackResult casualty = fixture.m_Queue.ConfirmRegisteredMemberCasualty(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			member.m_sSlotId,
			slot.m_sEntityId,
			nowSecond + 5,
			"deterministic restore casualty");
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
		fixture.m_Group.m_sRuntimeEntityId = "mission_guard_proof_process_group";
		fixture.m_Group.m_sRuntimeStatus = "mission_guard_physical";
		fixture.m_Group.m_iSpawnedAgentCount = living;
		fixture.m_Group.m_iAssignedWaypointCount = 1;
		fixture.m_Group.m_iInfantryCount = living;
		fixture.m_Group.m_iLastSeenAliveCount = living;
		fixture.m_Group.m_iSurvivorInfantryCount = living;
		fixture.m_Group.m_iDurableLivingInfantryCount = living;
		fixture.m_Group.m_vPosition = fixture.m_Operation.m_vAssignmentPosition;
		casualtySlotId = member.m_sSlotId;
		hvtSnapshot = BuildHVTSnapshot(fixture.m_HVT);
		objectiveSnapshot = BuildObjectiveSnapshot(fixture.m_Objective);

		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(fixture.m_State);
		return save;
	}

	protected HST_CampaignSaveData BuildLegacyPre55Save()
	{
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.m_iSchemaVersion = 54;
		save.m_iLastLoadedSchemaVersion = 54;
		save.m_iElapsedSeconds = 100;
		HST_ActiveMissionState mission = new HST_ActiveMissionState();
		mission.m_sInstanceId = "mission_guard_proof_legacy";
		mission.m_sMissionId = HST_MissionGuardOperationService.EXACT_MISSION_ID;
		mission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		mission.m_sTargetZoneId = "mission_guard_proof_legacy_zone";
		mission.m_sRuntimePrimitive = "kill_hvt";
		mission.m_iOperationContractVersion = 0;
		save.m_aActiveMissions.Insert(mission);
		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = "mission_group_mission_guard_proof_legacy";
		group.m_sMissionInstanceId = mission.m_sInstanceId;
		group.m_sZoneId = mission.m_sTargetZoneId;
		group.m_sFactionKey = "US";
		group.m_sRuntimeStatus = "mission_guard";
		group.m_iInfantryCount = 4;
		group.m_iSurvivorInfantryCount = 4;
		save.m_aActiveGroups.Insert(group);
		return save;
	}

	protected bool ProveDestroyedActiveRestore(out string evidence)
	{
		evidence = "settled destroyed active fixture unavailable";
		HST_MissionGuardOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("restore_destroyed_active");
		if (!m_Fixtures.Ready(fixture) || !ConfirmAllStrategicCasualties(fixture)
			|| !fixture.m_Service.TickVirtualForProof(
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
		validator.Normalize(save, 55);
		validator.Normalize(save, 55);
		HST_ActiveMissionState mission = FindSaveMission(save, fixture.m_Mission.m_sInstanceId);
		HST_OperationRecordState operation;
		if (mission)
			operation = FindSaveOperation(save, mission.m_sOperationId);
		HST_MissionAssetState hvt = FindSaveHVT(save, fixture.m_Mission.m_sInstanceId);
		HST_MissionObjectiveState objective = FindSaveHVTObjective(save, fixture.m_Mission.m_sInstanceId);
		bool rowsHeld = mission && operation && hvt && objective;
		bool activeDestroyed = rowsHeld && HST_MissionGuardOperationService.IsExactMission(mission)
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED;
		bool missionBoundaryHeld = rowsHeld
			&& BuildHVTSnapshot(hvt) == hvtBefore
			&& BuildObjectiveSnapshot(objective) == objectiveBefore
			&& hvt.m_bAlive && !objective.m_bComplete;
		bool projectionCompacted = rowsHeld
			&& FindSaveBatch(save, operation.m_sSpawnResultId) == null
			&& FindSaveGroup(save, operation.m_sGroupId) == null;
		bool exact = activeDestroyed && missionBoundaryHeld && projectionCompacted;
		evidence = string.Format(
			"settled-destroyed active mission/HVT/objective compact restore %1/%2/%3",
			exact,
			hvt != null,
			objective != null);
		return exact;
	}

	protected bool ProveCompactSettledRestore(out string evidence)
	{
		evidence = "compact settled non-active fixture unavailable";
		HST_MissionGuardOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("restore_compact");
		if (!m_Fixtures.Ready(fixture))
			return false;
		fixture.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_SUCCEEDED;
		fixture.m_HVT.m_bAlive = false;
		fixture.m_HVT.m_bDestroyed = true;
		fixture.m_Objective.m_bComplete = true;
		if (!fixture.m_Service.ReconcileAfterMissionOutcomes(fixture.m_State))
			return false;
		int hvtIndex = fixture.m_State.m_aMissionAssets.Find(fixture.m_HVT);
		int objectiveIndex = fixture.m_State.m_aMissionObjectives.Find(fixture.m_Objective);
		if (hvtIndex < 0 || objectiveIndex < 0)
			return false;
		fixture.m_State.m_aMissionAssets.Remove(hvtIndex);
		fixture.m_State.m_aMissionObjectives.Remove(objectiveIndex);
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(fixture.m_State);
		HST_OperationRecordState before = FindSaveOperation(save, fixture.m_Operation.m_sOperationId);
		int revisionBefore;
		if (before)
			revisionBefore = before.m_iRevision;
		HST_AssassinationGuardSaveValidationService validator = new HST_AssassinationGuardSaveValidationService();
		validator.Normalize(save, 55);
		validator.Normalize(save, 55);
		HST_ActiveMissionState mission = FindSaveMission(save, fixture.m_Mission.m_sInstanceId);
		HST_OperationRecordState operation = FindSaveOperation(save, fixture.m_Operation.m_sOperationId);
		bool rowsHeld = mission && operation;
		bool settledSucceeded = rowsHeld && HST_MissionGuardOperationService.IsExactMission(mission)
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED
			&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
			&& operation.m_iRevision == revisionBefore;
		bool missionBoundaryCompacted = rowsHeld
			&& FindSaveHVT(save, mission.m_sInstanceId) == null
			&& FindSaveHVTObjective(save, mission.m_sInstanceId) == null;
		bool projectionCompacted = rowsHeld
			&& FindSaveBatch(save, operation.m_sSpawnResultId) == null
			&& FindSaveGroup(save, operation.m_sGroupId) == null;
		bool exact = settledSucceeded && missionBoundaryCompacted && projectionCompacted;
		evidence = string.Format(
			"settled non-active compact/idempotent/cleaned-HVT-absence %1/%2/%3",
			exact,
			operation && operation.m_iRevision == revisionBefore,
			mission && FindSaveHVT(save, mission.m_sInstanceId) == null);
		return exact;
	}

	protected bool AllSaveSlotProcessIdsCleared(HST_ForceSpawnResultState batch)
	{
		if (!batch || !batch.m_sNativeGroupId.IsEmpty())
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

	protected void AppendGuardAggregate(HST_CampaignSaveData target, HST_CampaignSaveData source)
	{
		if (!target || !source)
			return;
		foreach (HST_ZoneState zone : source.m_aZones)
			target.m_aZones.Insert(zone);
		foreach (HST_ActiveMissionState mission : source.m_aActiveMissions)
			target.m_aActiveMissions.Insert(mission);
		foreach (HST_OperationRecordState operation : source.m_aOperations)
			target.m_aOperations.Insert(operation);
		foreach (HST_ForceManifestState manifest : source.m_aForceManifests)
			target.m_aForceManifests.Insert(manifest);
		foreach (HST_ForceSpawnResultState batch : source.m_aForceSpawnResults)
			target.m_aForceSpawnResults.Insert(batch);
		foreach (HST_ActiveGroupState group : source.m_aActiveGroups)
			target.m_aActiveGroups.Insert(group);
		foreach (HST_MissionObjectiveState objective : source.m_aMissionObjectives)
			target.m_aMissionObjectives.Insert(objective);
		foreach (HST_MissionAssetState asset : source.m_aMissionAssets)
			target.m_aMissionAssets.Insert(asset);
	}

	protected bool HasNoGuessedRefundOrFallback(
		HST_CampaignSaveData save,
		string missionInstanceId,
		int expectedGroupCount)
	{
		if (!save || missionInstanceId.IsEmpty()
			|| save.m_aResourceTransactions.Count() != 0
			|| save.m_aForceSettlementTombstones.Count() != 0
			|| save.m_aActiveGroups.Count() != expectedGroupCount)
			return false;
		string legacyFallbackId = "mission_group_" + missionInstanceId;
		foreach (HST_ActiveGroupState group : save.m_aActiveGroups)
		{
			if (group && group.m_sGroupId == legacyFallbackId)
				return false;
		}
		return true;
	}

	protected int CountSaveCasualties(HST_ForceSpawnResultState batch)
	{
		int count;
		if (!batch)
			return count;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				&& slot.m_bCasualtyConfirmed)
				count++;
		}
		return count;
	}

	protected string BuildBatchIsolationSnapshot(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return "missing";
		return string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			batch.m_sResultId,
			batch.m_sOperationId,
			batch.m_sManifestId,
			batch.m_sForceId,
			batch.m_sProjectionId,
			batch.m_eStatus,
			batch.m_bStrategicProjectionHeld,
			batch.m_bCancelRequested,
			batch.m_sLastFailureReason);
	}

	protected string BuildGroupIsolationSnapshot(HST_ActiveGroupState group)
	{
		if (!group)
			return "missing";
		return string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			group.m_sGroupId,
			group.m_sOperationId,
			group.m_sManifestId,
			group.m_sSpawnResultId,
			group.m_sForceId,
			group.m_sProjectionId,
			group.m_sSpawnFallbackMode,
			group.m_sRuntimeStatus,
			group.m_sSpawnFailureReason);
	}

	protected bool ReleaseHeldProjection(
		HST_MissionGuardOperationProofFixture fixture,
		int nowSecond)
	{
		if (!m_Fixtures.Ready(fixture))
			return false;
		HST_ForceSpawnQueueCallbackResult released = fixture.m_Queue.ReleaseStrategicProjectionForMaterialization(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			nowSecond,
			nowSecond + HST_MissionGuardOperationService.DEPLOYMENT_GRACE_SECONDS);
		return released && released.m_bAccepted && !fixture.m_Batch.m_bStrategicProjectionHeld;
	}

	protected bool CompleteProjection(
		HST_MissionGuardOperationProofFixture fixture,
		int nowSecond,
		string generationToken)
	{
		if (!m_Fixtures.Ready(fixture) || fixture.m_Batch.m_bStrategicProjectionHeld)
			return false;
		HST_ForceSpawnQueueTickResult rootTick = fixture.m_Queue.AcquireWork(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_State.m_aForceManifests,
			nowSecond);
		if (!rootTick || !rootTick.m_aWorkItems || rootTick.m_aWorkItems.Count() != 1
			|| rootTick.m_aWorkItems[0].m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_GROUP)
			return false;
		if (!CompleteWork(fixture, rootTick.m_aWorkItems[0], nowSecond, generationToken))
			return false;

		HST_ForceSpawnQueueTickResult memberTick = fixture.m_Queue.AcquireWork(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_State.m_aForceManifests,
			nowSecond + 1);
		if (!memberTick || !memberTick.m_aWorkItems || memberTick.m_aWorkItems.Count() <= 0)
			return false;
		foreach (HST_ForceSpawnQueueWorkItem work : memberTick.m_aWorkItems)
		{
			if (!work || work.m_sSlotKind != HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				|| !CompleteWork(fixture, work, nowSecond + 1, generationToken))
				return false;
		}

		HST_ForceSpawnQueueCallbackResult handoff = fixture.m_Queue.CompleteProjectionHandoff(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			fixture.m_Batch.m_sResultId,
			fixture.m_Batch.m_sProjectionId,
			fixture.m_Batch.m_iAttemptGeneration,
			nowSecond + 2);
		return handoff && handoff.m_bAccepted
			&& fixture.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED;
	}

	protected bool CompleteWork(
		HST_MissionGuardOperationProofFixture fixture,
		HST_ForceSpawnQueueWorkItem work,
		int nowSecond,
		string generationToken)
	{
		if (!fixture || !work)
			return false;
		HST_ForceSpawnQueueSlotSuccess success = new HST_ForceSpawnQueueSlotSuccess();
		success.m_sResultId = work.m_sResultId;
		success.m_sProjectionId = work.m_sProjectionId;
		success.m_sSlotId = work.m_sSlotId;
		success.m_sEntityId = string.Format(
			"mission_guard_proof_%1_%2_entity",
			generationToken,
			work.m_sSlotId);
		success.m_sSpawnedPrefab = work.m_sPrefab;
		success.m_sNativeGroupId = "mission_guard_proof_native_" + generationToken;
		success.m_iAttemptGeneration = work.m_iAttemptGeneration;
		success.m_bAliveVerified = true;
		success.m_bFactionVerified = true;
		success.m_bGroupVerified = true;
		success.m_bGameMasterVerified = true;
		success.m_bProjectionVerified = true;
		HST_ForceSpawnQueueCallbackResult completed = fixture.m_Queue.CompleteSlotSuccess(
			fixture.m_State.m_aForceSpawnResults,
			fixture.m_Manifest,
			success,
			nowSecond);
		return completed && completed.m_bAccepted && !completed.m_bCleanupRequired;
	}

	protected bool ConfirmAllStrategicCasualties(HST_MissionGuardOperationProofFixture fixture)
	{
		if (!m_Fixtures.Ready(fixture) || !fixture.m_Batch.m_bStrategicProjectionHeld)
			return false;
		for (int index = 0; index < fixture.m_Manifest.m_aMembers.Count(); index++)
		{
			HST_ForceManifestMemberState member = fixture.m_Manifest.m_aMembers[index];
			if (!member)
				return false;
			HST_ForceSpawnQueueCallbackResult casualty = fixture.m_Queue.ConfirmStrategicMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				member.m_sSlotId,
				fixture.m_State.m_iElapsedSeconds + index + 1,
				"deterministic all-guards-dead proof casualty");
			if (!casualty || !casualty.m_bAccepted)
				return false;
		}
		return fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch) == 0
			&& fixture.m_Queue.CountConfirmedCasualtyMemberSlots(fixture.m_Batch)
				== fixture.m_Manifest.m_aMembers.Count();
	}

	protected HST_ForceSpawnSlotResultState FindRootSlot(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return null;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP)
				return slot;
		}
		return null;
	}

	protected int CountRegisteredMemberSlots(HST_ForceSpawnResultState batch)
	{
		int count;
		if (!batch)
			return count;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
				&& slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED)
				count++;
		}
		return count;
	}

	protected string BuildHVTSnapshot(HST_MissionAssetState hvt)
	{
		if (!hvt)
			return "missing";
		string snapshot = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			hvt.m_sAssetId,
			hvt.m_sMissionInstanceId,
			hvt.m_sOperationId,
			hvt.m_sManifestId,
			hvt.m_sManifestSlotId,
			hvt.m_sKind,
			hvt.m_sRole,
			hvt.m_sEntityId,
			hvt.m_bSpawned);
		return snapshot + string.Format(
			"|%1|%2|%3|%4|%5|%6",
			hvt.m_bAlive,
			hvt.m_bDestroyed,
			hvt.m_vSourcePosition,
			hvt.m_vCurrentPosition,
			hvt.m_vLastKnownPosition,
			hvt.m_sLastInteraction);
	}

	protected string BuildObjectiveSnapshot(HST_MissionObjectiveState objective)
	{
		if (!objective)
			return "missing";
		string snapshot = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			objective.m_sObjectiveId,
			objective.m_sMissionInstanceId,
			objective.m_eType,
			objective.m_sTargetId,
			objective.m_sRuntimePrimitive,
			objective.m_vPosition,
			objective.m_iCurrentProgress,
			objective.m_iRequiredProgress,
			objective.m_bComplete);
		return snapshot + string.Format(
			"|%1|%2",
			objective.m_bFailed,
			objective.m_bCleanupComplete);
	}

	protected int CountOperation(HST_CampaignState state, string operationId)
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

	protected int CountManifest(HST_CampaignState state, string manifestId)
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

	protected int CountBatch(HST_CampaignState state, string resultId)
	{
		int count;
		if (!state || resultId.IsEmpty())
			return count;
		foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
		{
			if (batch && batch.m_sResultId == resultId)
				count++;
		}
		return count;
	}

	protected int CountGroup(HST_CampaignState state, string groupId)
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

	protected HST_ActiveMissionState FindSaveMission(HST_CampaignSaveData save, string instanceId)
	{
		if (!save || instanceId.IsEmpty())
			return null;
		foreach (HST_ActiveMissionState mission : save.m_aActiveMissions)
		{
			if (mission && mission.m_sInstanceId == instanceId)
				return mission;
		}
		return null;
	}

	protected HST_OperationRecordState FindSaveOperation(HST_CampaignSaveData save, string operationId)
	{
		if (!save || operationId.IsEmpty())
			return null;
		foreach (HST_OperationRecordState operation : save.m_aOperations)
		{
			if (operation && operation.m_sOperationId == operationId)
				return operation;
		}
		return null;
	}

	protected HST_ForceManifestState FindSaveManifest(HST_CampaignSaveData save, string manifestId)
	{
		if (!save || manifestId.IsEmpty())
			return null;
		foreach (HST_ForceManifestState manifest : save.m_aForceManifests)
		{
			if (manifest && manifest.m_sManifestId == manifestId)
				return manifest;
		}
		return null;
	}

	protected HST_ForceSpawnResultState FindSaveBatch(HST_CampaignSaveData save, string resultId)
	{
		if (!save || resultId.IsEmpty())
			return null;
		foreach (HST_ForceSpawnResultState batch : save.m_aForceSpawnResults)
		{
			if (batch && batch.m_sResultId == resultId)
				return batch;
		}
		return null;
	}

	protected HST_ActiveGroupState FindSaveGroup(HST_CampaignSaveData save, string groupId)
	{
		if (!save || groupId.IsEmpty())
			return null;
		foreach (HST_ActiveGroupState group : save.m_aActiveGroups)
		{
			if (group && group.m_sGroupId == groupId)
				return group;
		}
		return null;
	}

	protected HST_MissionAssetState FindSaveHVT(HST_CampaignSaveData save, string missionInstanceId)
	{
		if (!save || missionInstanceId.IsEmpty())
			return null;
		foreach (HST_MissionAssetState asset : save.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == missionInstanceId && asset.m_sRole == "hvt")
				return asset;
		}
		return null;
	}

	protected HST_MissionObjectiveState FindSaveHVTObjective(HST_CampaignSaveData save, string missionInstanceId)
	{
		if (!save || missionInstanceId.IsEmpty())
			return null;
		foreach (HST_MissionObjectiveState objective : save.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == missionInstanceId
				&& objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET
				&& objective.m_sTargetId == "hvt")
				return objective;
		}
		return null;
	}
}
#endif
