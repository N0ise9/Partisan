class HST_RescuePOWOperationProofReport
{
	bool m_bAdmissionIsolationExact;
	bool m_bCompositeAuthorityExact;
	bool m_bCaptiveTransitionsExact;
	bool m_bGuardIndependenceExact;
	bool m_bOutcomeGraceExact;
	bool m_bRestoreQuarantineExact;
	string m_sAdmissionIsolationEvidence;
	string m_sCompositeAuthorityEvidence;
	string m_sCaptiveTransitionsEvidence;
	string m_sGuardIndependenceEvidence;
	string m_sOutcomeGraceEvidence;
	string m_sRestoreQuarantineEvidence;
}

// Source-only seams. Native entity creation, actual vehicle compartments,
// adapter handles, save/restart, networking, rendered UI, and JIP are packaged
// runtime gates and are intentionally not substituted here.
class HST_RescuePOWOperationProofHarness : HST_RescuePOWOperationService
{
	bool TickVirtualGuardForProof(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		return TickVirtualGuard(state, operation, manifest, batch, group);
	}

	bool QuarantineForProof(
		HST_CampaignState state,
		HST_OperationRecordState operation,
		string reason)
	{
		return QuarantineOperationAuthority(state, operation, reason);
	}

	bool ReconcileDisconnectedEscortForProof(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState captive)
	{
		return ReconcileDisconnectedEscort(state, mission, captive);
	}
}

class HST_RescuePOWOperationProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_MissionDefinition m_Definition;
	ref HST_ActiveMissionState m_Mission;
	ref HST_MissionObjectiveState m_PrimaryObjective;
	ref HST_MissionObjectiveState m_ExtractionObjective;
	ref HST_MissionRuntimeService m_MissionRuntime;
	ref HST_ForceSpawnQueueService m_Queue;
	ref HST_ForceSpawnAdapterService m_Adapter;
	ref HST_PhysicalWarService m_PhysicalWar;
	ref HST_RescuePOWOperationProofHarness m_Service;
	ref HST_RescuePOWAdmissionResult m_Preflight;
	ref HST_RescuePOWAdmissionResult m_Admission;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceManifestState m_Manifest;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_GuardGroup;
	ref array<ref HST_MissionAssetState> m_aCaptives = {};
	bool m_bPrepared;
	bool m_bPreflightReadOnly;
	string m_sFailureReason;
}

class HST_RescuePOWOperationProofFixtureFactory
{
	static const string PROOF_ZONE_PREFIX = "rescue_pow_proof_zone_";
	static const string PROOF_SITE_PREFIX = "rescue_pow_proof_site_";
	static const string PROOF_MISSION_PREFIX = "rescue_pow_proof_mission_";

	HST_RescuePOWOperationProofFixture BuildAdmittedFixture(string suffix)
	{
		HST_RescuePOWOperationProofFixture fixture = BuildPreparedFixture(suffix);
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
			fixture.m_sFailureReason = "exact rescue proof admission failed";
			if (fixture.m_Admission && !fixture.m_Admission.m_sFailureReason.IsEmpty())
				fixture.m_sFailureReason = fixture.m_sFailureReason + ": "
					+ fixture.m_Admission.m_sFailureReason;
			return fixture;
		}
		fixture.m_Operation = fixture.m_Admission.m_Operation;
		fixture.m_Manifest = fixture.m_Admission.m_Manifest;
		fixture.m_Batch = fixture.m_Admission.m_Batch;
		fixture.m_GuardGroup = fixture.m_Admission.m_GuardGroup;
		fixture.m_aCaptives = fixture.m_Admission.m_aCaptives;
		if (!Ready(fixture))
			fixture.m_sFailureReason = "exact rescue proof committed graph is incomplete";
		return fixture;
	}

	HST_RescuePOWOperationProofFixture BuildPreparedFixture(string suffix)
	{
		HST_RescuePOWOperationProofFixture fixture = new HST_RescuePOWOperationProofFixture();
		fixture.m_State = BuildState(suffix);
		fixture.m_Preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		fixture.m_Definition = FindDefinition(HST_RescuePOWOperationService.EXACT_MISSION_ID);
		fixture.m_Mission = BuildMission(suffix);
		fixture.m_PrimaryObjective = BuildObjective(fixture.m_Mission, false);
		fixture.m_ExtractionObjective = BuildObjective(fixture.m_Mission, true);
		fixture.m_MissionRuntime = new HST_MissionRuntimeService();
		fixture.m_Queue = new HST_ForceSpawnQueueService();
		fixture.m_Adapter = new HST_ForceSpawnAdapterService();
		fixture.m_PhysicalWar = new HST_PhysicalWarService();
		fixture.m_Service = new HST_RescuePOWOperationProofHarness();
		fixture.m_Service.SetRuntimeServices(
			fixture.m_Queue,
			fixture.m_Adapter,
			fixture.m_PhysicalWar,
			fixture.m_MissionRuntime);
		fixture.m_State.m_aActiveMissions.Insert(fixture.m_Mission);
		fixture.m_State.m_aMissionObjectives.Insert(fixture.m_PrimaryObjective);
		fixture.m_State.m_aMissionObjectives.Insert(fixture.m_ExtractionObjective);
		fixture.m_bPrepared = fixture.m_Service.PrepareNewMissionContract(fixture.m_Mission);
		if (!fixture.m_bPrepared || !fixture.m_Definition)
		{
			fixture.m_sFailureReason = "exact rescue proof preparation failed";
			return fixture;
		}

		int missionsBefore = fixture.m_State.m_aActiveMissions.Count();
		int operationsBefore = fixture.m_State.m_aOperations.Count();
		int manifestsBefore = fixture.m_State.m_aForceManifests.Count();
		int batchesBefore = fixture.m_State.m_aForceSpawnResults.Count();
		int groupsBefore = fixture.m_State.m_aActiveGroups.Count();
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
			&& fixture.m_State.m_aMissionAssets.Count() == assetsBefore
			&& fixture.m_Mission.m_sOperationId.IsEmpty()
			&& fixture.m_Mission.m_sManifestId.IsEmpty()
			&& fixture.m_Mission.m_sSpawnResultId.IsEmpty();
		if (!fixture.m_bPreflightReadOnly)
		{
			fixture.m_sFailureReason = "exact rescue proof preflight rejected or mutated state";
			if (fixture.m_Preflight && !fixture.m_Preflight.m_sFailureReason.IsEmpty())
				fixture.m_sFailureReason = fixture.m_sFailureReason + ": "
					+ fixture.m_Preflight.m_sFailureReason;
		}
		return fixture;
	}

	bool Prepared(HST_RescuePOWOperationProofFixture fixture)
	{
		return fixture && fixture.m_State && fixture.m_Preset && fixture.m_Definition
			&& fixture.m_Mission && fixture.m_PrimaryObjective && fixture.m_ExtractionObjective
			&& fixture.m_MissionRuntime && fixture.m_Queue && fixture.m_Adapter
			&& fixture.m_PhysicalWar && fixture.m_Service
			&& fixture.m_bPrepared && fixture.m_bPreflightReadOnly;
	}

	bool Ready(HST_RescuePOWOperationProofFixture fixture)
	{
		return Prepared(fixture) && fixture.m_Admission && fixture.m_Admission.m_bSuccess
			&& fixture.m_Operation && fixture.m_Manifest && fixture.m_Batch
			&& fixture.m_GuardGroup
			&& fixture.m_aCaptives.Count() == HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT;
	}

	string Failure(HST_RescuePOWOperationProofFixture fixture)
	{
		if (!fixture)
			return "exact rescue proof fixture is unavailable";
		if (!fixture.m_sFailureReason.IsEmpty())
			return fixture.m_sFailureReason;
		return "exact rescue proof fixture is incomplete";
	}

	protected HST_CampaignState BuildState(string suffix)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_sPresetId = "rescue_pow_proof";
		state.m_iCampaignSeed = 580058;
		state.m_iElapsedSeconds = 580;
		state.m_iWarLevel = 4;
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		state.m_vHQPosition = "4600 20 4600";

		HST_PlayerState escort = new HST_PlayerState();
		escort.m_sIdentityId = "escort_alpha";
		escort.m_sDisplayName = "Proof Escort";
		escort.m_sFactionKey = "FIA";
		escort.m_iLastSeenPlayerId = 1;
		escort.m_bMember = true;
		state.m_aPlayers.Insert(escort);

		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = BuildZoneId(suffix);
		zone.m_sDisplayName = "Rescue POW Proof " + suffix;
		zone.m_sOwnerFactionKey = "US";
		zone.m_eType = HST_EZoneType.HST_ZONE_TOWN;
		zone.m_vPosition = BuildSitePosition();
		zone.m_iActivationRadiusMeters = 900;
		zone.m_iCaptureRadiusMeters = 140;
		zone.m_iGarrisonSlots = 32;
		state.m_aZones.Insert(zone);

		HST_GeneratedSiteState site = new HST_GeneratedSiteState();
		site.m_sSiteId = BuildSiteId(suffix);
		site.m_sZoneId = zone.m_sZoneId;
		site.m_sSourceCategory = "primary";
		site.m_vPosition = BuildSitePosition();
		site.m_iRadiusMeters = 30;
		site.m_bValid = true;
		state.m_aGeneratedSites.Insert(site);
		return state;
	}

	protected HST_ActiveMissionState BuildMission(string suffix)
	{
		HST_ActiveMissionState mission = new HST_ActiveMissionState();
		mission.m_sInstanceId = BuildMissionInstanceId(suffix);
		mission.m_sMissionId = HST_RescuePOWOperationService.EXACT_MISSION_ID;
		mission.m_sDisplayName = "Rescue POWs";
		mission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		mission.m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_PHYSICAL_MVP;
		mission.m_sTargetZoneId = BuildZoneId(suffix);
		mission.m_sSiteId = BuildSiteId(suffix);
		mission.m_vTargetPosition = BuildSitePosition();
		mission.m_sRuntimePrimitive = "rescue_extract";
		mission.m_sRuntimeType = "rescue_extract";
		mission.m_sRuntimePhase = "active";
		mission.m_iStartedAtSecond = 500;
		mission.m_iRuntimeStartedAtSecond = 500;
		mission.m_iActiveUntilSecond = 4100;
		mission.m_iRemainingSeconds = 3520;
		mission.m_iRequiredCaptiveCount = HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT;
		return mission;
	}

	protected HST_MissionObjectiveState BuildObjective(
		HST_ActiveMissionState mission,
		bool extraction)
	{
		HST_MissionObjectiveState objective = new HST_MissionObjectiveState();
		objective.m_sObjectiveId = string.Format("rescue_pow_proof_objective_%1_%2",
			mission.m_sInstanceId, extraction);
		objective.m_sMissionInstanceId = mission.m_sInstanceId;
		objective.m_eType = HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES;
		objective.m_sLabel = "Rescue and extract captives";
		objective.m_sTargetId = "captive";
		if (extraction)
			objective.m_sTargetId = "extract_captives";
		objective.m_sTargetZoneId = mission.m_sTargetZoneId;
		objective.m_sRuntimePrimitive = "rescue_extract";
		objective.m_vPosition = BuildSitePosition();
		if (extraction)
			objective.m_vPosition = "4600 20 4600";
		objective.m_iRequiredProgress = HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT;
		objective.m_iRequiredCount = HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT;
		return objective;
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

	static string BuildSiteId(string suffix)
	{
		return PROOF_SITE_PREFIX + suffix;
	}

	static string BuildMissionInstanceId(string suffix)
	{
		return PROOF_MISSION_PREFIX + suffix;
	}

	static vector BuildSitePosition()
	{
		return "5040 20 5040";
	}
}

class HST_RescuePOWOperationProofService
{
	static const string PACKAGED_GATES = "packaged gates not claimed: native entities, natural guard combat, vehicle seats, save/restart, reconnect/JIP, rendered UI, owner-change, and setup";
	protected ref HST_RescuePOWOperationProofFixtureFactory m_Fixtures = new HST_RescuePOWOperationProofFixtureFactory();

	HST_RescuePOWOperationProofReport Run()
	{
		HST_RescuePOWOperationProofReport report = new HST_RescuePOWOperationProofReport();
		ProveAdmissionIsolation(report);
		ProveCompositeAuthority(report);
		ProveCaptiveTransitions(report);
		ProveGuardIndependence(report);
		ProveOutcomeGrace(report);
		ProveRestoreQuarantine(report);
		return report;
	}

	protected void ProveAdmissionIsolation(HST_RescuePOWOperationProofReport report)
	{
		HST_RescuePOWOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("admission");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sAdmissionIsolationEvidence = m_Fixtures.Failure(fixture) + " | " + PACKAGED_GATES;
			return;
		}
		int operationsBefore = fixture.m_State.m_aOperations.Count();
		int manifestsBefore = fixture.m_State.m_aForceManifests.Count();
		int batchesBefore = fixture.m_State.m_aForceSpawnResults.Count();
		int groupsBefore = fixture.m_State.m_aActiveGroups.Count();
		int assetsBefore = fixture.m_State.m_aMissionAssets.Count();
		HST_RescuePOWAdmissionResult replay = fixture.m_Service.AdmitNewMission(
			fixture.m_State, fixture.m_Preset, fixture.m_Definition, fixture.m_Mission,
			fixture.m_MissionRuntime);

		HST_ActiveMissionState historical = new HST_ActiveMissionState();
		historical.m_sMissionId = HST_RescuePOWOperationService.EXACT_MISSION_ID;
		historical.m_iOperationContractVersion = 0;
		HST_ActiveMissionState refugees = new HST_ActiveMissionState();
		refugees.m_sMissionId = "rescue_refugees";
		refugees.m_iOperationContractVersion = 0;
		bool replayExact = replay && replay.m_bSuccess && replay.m_bAlreadyApplied
			&& fixture.m_State.m_aOperations.Count() == operationsBefore
			&& fixture.m_State.m_aForceManifests.Count() == manifestsBefore
			&& fixture.m_State.m_aForceSpawnResults.Count() == batchesBefore
			&& fixture.m_State.m_aActiveGroups.Count() == groupsBefore
			&& fixture.m_State.m_aMissionAssets.Count() == assetsBefore;
		bool isolationExact = !HST_RescuePOWOperationService.IsExactOrQuarantinedMission(historical)
			&& !HST_RescuePOWOperationService.IsExactOrQuarantinedMission(refugees);
		report.m_bAdmissionIsolationExact = fixture.m_bPreflightReadOnly && replayExact && isolationExact;
		report.m_sAdmissionIsolationEvidence = string.Format(
			"prepared %1 | preflight_read_only %2 | replay %3 | operations %4 | manifests %5 | batches %6 | groups %7 | assets %8",
			fixture.m_bPrepared,
			fixture.m_bPreflightReadOnly,
			replayExact,
			operationsBefore,
			manifestsBefore,
			batchesBefore,
			groupsBefore,
			assetsBefore);
		report.m_sAdmissionIsolationEvidence = report.m_sAdmissionIsolationEvidence
			+ string.Format(" | historical/refugee contract0 %1 | %2", isolationExact, PACKAGED_GATES);
	}

	protected void ProveCompositeAuthority(HST_RescuePOWOperationProofReport report)
	{
		HST_RescuePOWOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("composite");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sCompositeAuthorityEvidence = m_Fixtures.Failure(fixture) + " | " + PACKAGED_GATES;
			return;
		}
		bool manifestExact = fixture.m_Manifest.m_bFrozen
			&& fixture.m_Manifest.m_sPolicyId == HST_RescuePOWOperationService.EXACT_POLICY_ID
			&& fixture.m_Manifest.m_sIntentId == HST_RescuePOWOperationService.EXACT_INTENT_ID
			&& fixture.m_Manifest.m_sForceKind == HST_RescuePOWOperationService.EXACT_FORCE_KIND
			&& fixture.m_Manifest.m_aGroups.Count() == 1
			&& fixture.m_Manifest.m_aMembers.Count() > 0
			&& fixture.m_Manifest.m_aVehicles.Count() == 0
			&& fixture.m_Manifest.m_aAssets.Count() == HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT;
		bool batchExact = fixture.m_Batch.m_bExternalAssetAuthority
			&& fixture.m_Batch.m_iExpectedSlotCount
				== fixture.m_Manifest.m_aGroups.Count() + fixture.m_Manifest.m_aMembers.Count()
			&& fixture.m_Batch.m_aSlotResults.Count() == fixture.m_Batch.m_iExpectedSlotCount;
		bool captivesExact = true;
		bool frozenExtractionExact = fixture.m_Mission.m_vRescueExtractionPosition
			== fixture.m_State.m_vHQPosition
			&& HST_RescuePOWOperationService.HasOpenFrozenHQExtractionAuthority(fixture.m_State);
		array<int> ordinals = {};
		foreach (HST_MissionAssetState captive : fixture.m_aCaptives)
		{
			captivesExact = captivesExact && captive
				&& HST_RescuePOWOperationService.IsExactRescueCaptiveAsset(fixture.m_State, captive)
				&& !ordinals.Contains(captive.m_iRescueOrdinal)
				&& captive.m_vTargetPosition == fixture.m_Mission.m_vRescueExtractionPosition
				&& captive.m_eRescueDisposition
					== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD;
			if (captive)
				ordinals.Insert(captive.m_iRescueOrdinal);
		}
		captivesExact = captivesExact
			&& ordinals.Count() == HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT;
		bool reciprocal = fixture.m_Operation.m_eType
				== HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE
			&& fixture.m_Mission.m_sOperationId == fixture.m_Operation.m_sOperationId
			&& fixture.m_Mission.m_sManifestId == fixture.m_Manifest.m_sManifestId
			&& fixture.m_Mission.m_sSpawnResultId == fixture.m_Batch.m_sResultId
			&& HST_RescuePOWOperationService.IsExactMissionRescueGroup(
				fixture.m_State, fixture.m_GuardGroup);
		report.m_bCompositeAuthorityExact = manifestExact && batchExact && captivesExact
			&& reciprocal && frozenExtractionExact;
		report.m_sCompositeAuthorityEvidence = string.Format(
			"manifest %1 | guard members %2 | external batch %3/%4 | captives %5/%6 | reciprocal %7 | frozen HQ %8 | %9",
			manifestExact,
			fixture.m_Manifest.m_aMembers.Count(),
			batchExact,
			fixture.m_Batch.m_iExpectedSlotCount,
			captivesExact,
			fixture.m_aCaptives.Count(),
			reciprocal,
			frozenExtractionExact,
			PACKAGED_GATES);
	}

	protected void ProveCaptiveTransitions(HST_RescuePOWOperationProofReport report)
	{
		HST_RescuePOWOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("transitions");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sCaptiveTransitionsEvidence = m_Fixtures.Failure(fixture) + " | " + PACKAGED_GATES;
			return;
		}
		HST_MissionAssetState first = fixture.m_aCaptives[0];
		HST_MissionAssetState second = fixture.m_aCaptives[1];
		HST_MissionAssetState third = fixture.m_aCaptives[2];
		HST_RescuePOWTransitionResult freed = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, first, "mission_captive_extract",
			"escort_alpha", "request_free_first");
		HST_RescuePOWTransitionResult immediateReplay = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, first, "mission_captive_extract",
			"escort_alpha", "request_free_first");
		HST_RescuePOWTransitionResult following = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, first, "follow",
			"escort_alpha", "request_follow_first");
		HST_RescuePOWTransitionResult foreign = fixture.m_Service.ApplyCaptiveTransition(
			fixture.m_State, fixture.m_Mission, first,
			HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING,
			"escort_bravo", "request_foreign_board", "vehicle_foreign");
		HST_RescuePOWTransitionResult boarding = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, first, "board",
			"escort_alpha", "request_board_first", "vehicle_alpha");
		HST_RescuePOWTransitionResult boarded = fixture.m_Service.ApplyCaptiveTransition(
			fixture.m_State, fixture.m_Mission, first,
			HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED,
			"escort_alpha", "request_seat_first", "vehicle_alpha", "seat_2");
		HST_ERescueCaptiveDisposition dispositionBeforeDelayedReplay = first.m_eRescueDisposition;
		string escortBeforeDelayedReplay = first.m_sRescueEscortIdentityId;
		string carrierBeforeDelayedReplay = first.m_sRescueCarrierVehicleId;
		string seatBeforeDelayedReplay = first.m_sRescueCarrierSeatToken;
		int revisionBeforeDelayedReplay = first.m_iRescueRevision;
		HST_RescuePOWTransitionResult delayedFreeReplay = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, first, "mission_captive_extract",
			"escort_alpha", "request_free_first");
		HST_RescuePOWTransitionResult delayedFollowReplay = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, first, "follow",
			"escort_alpha", "request_follow_first");
		HST_RescuePOWTransitionResult fingerprintConflict = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, first, "follow",
			"escort_bravo", "request_free_first");
		int secondRevisionBeforeCollision = second.m_iRescueRevision;
		HST_RescuePOWTransitionResult foreignReceiptCollision = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, second, "mission_captive_extract",
			"escort_alpha", "request_free_first");
		bool delayedReplayImmutable = delayedFreeReplay && delayedFreeReplay.m_bSuccess
			&& delayedFreeReplay.m_bAlreadyApplied && delayedFreeReplay.m_sResult == freed.m_sResult
			&& delayedFollowReplay && delayedFollowReplay.m_bSuccess
			&& delayedFollowReplay.m_bAlreadyApplied
			&& delayedFollowReplay.m_sResult == following.m_sResult
			&& first.m_eRescueDisposition == dispositionBeforeDelayedReplay
			&& first.m_sRescueEscortIdentityId == escortBeforeDelayedReplay
			&& first.m_sRescueCarrierVehicleId == carrierBeforeDelayedReplay
			&& first.m_sRescueCarrierSeatToken == seatBeforeDelayedReplay
			&& first.m_iRescueRevision == revisionBeforeDelayedReplay;
		bool collisionExact = fingerprintConflict && !fingerprintConflict.m_bSuccess
			&& fingerprintConflict.m_bAlreadyApplied
			&& first.m_iRescueRevision == revisionBeforeDelayedReplay
			&& foreignReceiptCollision && !foreignReceiptCollision.m_bSuccess
			&& foreignReceiptCollision.m_bAlreadyApplied
			&& second.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD
			&& second.m_iRescueRevision == secondRevisionBeforeCollision;
		first.m_vCurrentPosition = first.m_vTargetPosition;
		HST_RescuePOWTransitionResult extracted = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, first, "extract",
			"escort_alpha", "request_extract_first");
		HST_RescuePOWTransitionResult rejectedFollow = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, second, "follow",
			"escort_alpha", "request_rejected_follow_second");
		HST_RescuePOWTransitionResult secondFreed = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, second, "free",
			"escort_alpha", "request_free_second");
		int secondRevisionBeforeRejectedReplay = second.m_iRescueRevision;
		HST_RescuePOWTransitionResult rejectedFollowReplay = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, second, "follow",
			"escort_alpha", "request_rejected_follow_second");
		bool rejectedReplayExact = rejectedFollow && !rejectedFollow.m_bSuccess
			&& secondFreed && secondFreed.m_bSuccess
			&& rejectedFollowReplay && !rejectedFollowReplay.m_bSuccess
			&& rejectedFollowReplay.m_bAlreadyApplied
			&& rejectedFollowReplay.m_sResult == rejectedFollow.m_sResult
			&& second.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED
			&& second.m_iRescueRevision == secondRevisionBeforeRejectedReplay;
		bool killed = fixture.m_Service.MarkCaptiveDeathObserved(
			fixture.m_State, fixture.m_Mission, second, "proof observed destroyed state");
		HST_RescuePOWTransitionResult missingDeath = fixture.m_Service.ApplyCaptiveTransition(
			fixture.m_State, fixture.m_Mission, third,
			HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED,
			"", "request_missing_death_evidence");
		bool missingDeathExact = missingDeath && !missingDeath.m_bSuccess
			&& third.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD;
		bool rejectedRetentionExact = true;
		int rejectionIndex;
		for (rejectionIndex = 0; rejectionIndex < 12; rejectionIndex++)
		{
			HST_RescuePOWTransitionResult retainedRejection = fixture.m_Service.HandleCaptiveCommand(
				fixture.m_State, fixture.m_Mission, third, "follow",
				"escort_alpha", string.Format("capacity_rejected_%1", rejectionIndex));
			rejectedRetentionExact = rejectedRetentionExact
				&& retainedRejection && !retainedRejection.m_bSuccess;
		}
		rejectedRetentionExact = rejectedRetentionExact
			&& third.m_aRescueCommandReceipts.Count()
				== HST_RescuePOWOperationService.MAX_REJECTED_CAPTIVE_COMMAND_RECEIPTS;
		HST_RescuePOWTransitionResult capacityFree = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, third, "free",
			"escort_alpha", "capacity_free");
		HST_RescuePOWTransitionResult capacityFollow = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, third, "follow",
			"escort_alpha", "capacity_follow");
		bool capacityFillExact = rejectedRetentionExact
			&& capacityFree && capacityFree.m_bSuccess
			&& capacityFollow && capacityFollow.m_bSuccess;
		int capacitySequence;
		while (capacityFillExact && third.m_aRescueCommandReceipts.Count()
			< HST_RescuePOWOperationService.MAX_CAPTIVE_COMMAND_RECEIPTS - 1)
		{
			if (third.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING)
			{
				HST_RescuePOWTransitionResult returnToFollow = fixture.m_Service.ApplyCaptiveTransition(
					fixture.m_State, fixture.m_Mission, third,
					HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING,
					"escort_alpha", string.Format("capacity_follow_cycle_%1", capacitySequence));
				capacityFillExact = returnToFollow && returnToFollow.m_bSuccess;
			}
			if (!capacityFillExact)
				break;
			HST_RescuePOWTransitionResult capacityBoard = fixture.m_Service.HandleCaptiveCommand(
				fixture.m_State, fixture.m_Mission, third, "board",
				"escort_alpha", string.Format("capacity_board_%1", capacitySequence),
				"vehicle_capacity");
			capacityFillExact = capacityBoard && capacityBoard.m_bSuccess;
			capacitySequence++;
		}
		HST_RescuePOWTransitionResult capacityBoarded = fixture.m_Service.ApplyCaptiveTransition(
			fixture.m_State, fixture.m_Mission, third,
			HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED,
			"escort_alpha", "capacity_seat", "vehicle_capacity", "seat_capacity");
		third.m_vCurrentPosition = third.m_vTargetPosition;
		HST_RescuePOWTransitionResult capacityExtract = fixture.m_Service.HandleCaptiveCommand(
			fixture.m_State, fixture.m_Mission, third, "extract",
			"escort_alpha", "capacity_terminal_extract");
		bool capacityExact = capacityFillExact && capacityBoarded && capacityBoarded.m_bSuccess
			&& capacityExtract && capacityExtract.m_bSuccess
			&& third.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED
			&& third.m_aRescueCommandReceipts.Count()
				== HST_RescuePOWOperationService.MAX_CAPTIVE_COMMAND_RECEIPTS
			&& third.m_aRescueCommandReceipts[
				third.m_aRescueCommandReceipts.Count() - 1].m_sRequestId
					== "capacity_terminal_extract";
		HST_CampaignSaveData captured = new HST_CampaignSaveData();
		captured.Capture(fixture.m_State);
		HST_CampaignState copiedState = captured.Restore();
		HST_MissionAssetState copiedFirst;
		if (copiedState)
			copiedFirst = copiedState.FindMissionAsset(first.m_sAssetId);
		bool ledgerCopyExact = copiedFirst && copiedFirst.m_aRescueCommandReceipts
			&& first.m_aRescueCommandReceipts
			&& copiedFirst.m_aRescueCommandReceipts.Count()
				== first.m_aRescueCommandReceipts.Count();
		if (ledgerCopyExact)
		{
			int receiptIndex;
			for (receiptIndex = 0; receiptIndex < first.m_aRescueCommandReceipts.Count(); receiptIndex++)
			{
				HST_RescueCommandReceiptState sourceReceipt = first.m_aRescueCommandReceipts[receiptIndex];
				HST_RescueCommandReceiptState copiedReceipt = copiedFirst.m_aRescueCommandReceipts[receiptIndex];
				ledgerCopyExact = ledgerCopyExact && sourceReceipt && copiedReceipt
					&& copiedReceipt != sourceReceipt
					&& copiedReceipt.m_sRequestId == sourceReceipt.m_sRequestId
					&& copiedReceipt.m_sActorIdentityId == sourceReceipt.m_sActorIdentityId
					&& copiedReceipt.m_sCommand == sourceReceipt.m_sCommand
					&& copiedReceipt.m_sResult == sourceReceipt.m_sResult
					&& copiedReceipt.m_iRecordedRevision == sourceReceipt.m_iRecordedRevision;
			}
		}

		bool sequenceExact = freed && freed.m_bSuccess;
		sequenceExact = sequenceExact && immediateReplay && immediateReplay.m_bSuccess
			&& immediateReplay.m_bAlreadyApplied;
		sequenceExact = sequenceExact && following && following.m_bSuccess;
		sequenceExact = sequenceExact && foreign && !foreign.m_bSuccess;
		sequenceExact = sequenceExact && boarding && boarding.m_bSuccess;
		sequenceExact = sequenceExact && boarded && boarded.m_bSuccess;
		sequenceExact = sequenceExact && extracted && extracted.m_bSuccess;
		sequenceExact = sequenceExact && first.m_bRescueExtractionObserved
			&& first.m_bDelivered && first.m_bAlive;
		sequenceExact = sequenceExact && killed && second.m_bRescueDeathObserved
			&& second.m_bDestroyed && !second.m_bAlive;
		sequenceExact = sequenceExact && rejectedReplayExact
			&& missingDeathExact && capacityExact;
		report.m_bCaptiveTransitionsExact = sequenceExact && delayedReplayImmutable
			&& collisionExact && ledgerCopyExact;
		report.m_sCaptiveTransitionsEvidence = string.Format(
			"free %1 | immediate replay %2 | follow %3 | foreign escort rejected %4 | boarding/seat %5/%6 | delayed replay immutable %7 | request collisions %8",
			freed && freed.m_bSuccess,
			immediateReplay && immediateReplay.m_bAlreadyApplied,
			following && following.m_bSuccess,
			foreign && !foreign.m_bSuccess,
			boarding && boarding.m_bSuccess,
			boarded && boarded.m_bSuccess,
			delayedReplayImmutable,
			collisionExact);
		report.m_sCaptiveTransitionsEvidence = report.m_sCaptiveTransitionsEvidence
			+ string.Format(" | extraction %1 | rejected replay stable %2 | death %3 | missing-death rejected %4 | bounded rejection/no-DoS + terminal capacity %5 | ledger copy %6",
				extracted && extracted.m_bSuccess, rejectedReplayExact, killed,
				missingDeathExact, capacityExact, ledgerCopyExact);
		report.m_sCaptiveTransitionsEvidence = report.m_sCaptiveTransitionsEvidence
			+ " | " + PACKAGED_GATES;
	}

	protected void ProveGuardIndependence(HST_RescuePOWOperationProofReport report)
	{
		HST_RescuePOWOperationProofFixture fixture = m_Fixtures.BuildAdmittedFixture("guard_independence");
		if (!m_Fixtures.Ready(fixture))
		{
			report.m_sGuardIndependenceEvidence = m_Fixtures.Failure(fixture) + " | " + PACKAGED_GATES;
			return;
		}
		bool casualtiesAccepted = true;
		foreach (HST_ForceManifestMemberState member : fixture.m_Manifest.m_aMembers)
		{
			HST_ForceSpawnQueueCallbackResult casualty = fixture.m_Queue.ConfirmStrategicMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				member.m_sSlotId,
				fixture.m_State.m_iElapsedSeconds,
				"source proof guard casualty");
			casualtiesAccepted = casualtiesAccepted && casualty && casualty.m_bAccepted;
		}
		bool ticked = fixture.m_Service.TickVirtualGuardForProof(
			fixture.m_State,
			fixture.m_Operation,
			fixture.m_Manifest,
			fixture.m_Batch,
			fixture.m_GuardGroup);
		bool independent = casualtiesAccepted && ticked
			&& fixture.m_GuardGroup.m_sRuntimeStatus
				== HST_RescuePOWOperationService.GUARD_ELIMINATED_STATUS
			&& fixture.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& fixture.m_Operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			&& fixture.m_Mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			&& !HST_RescuePOWOperationService.AreAllRequiredCaptivesExtracted(
				fixture.m_State, fixture.m_Mission)
			&& !HST_RescuePOWOperationService.HasAnyRequiredCaptiveDeath(
				fixture.m_State, fixture.m_Mission);
		report.m_bGuardIndependenceExact = independent;
		report.m_sGuardIndependenceEvidence = string.Format(
			"casualties %1/%2 | ticked %3 | guard status %4 | operation settlement %5 | mission status %6 | captives untouched %7 | %8",
			casualtiesAccepted,
			fixture.m_Manifest.m_aMembers.Count(),
			ticked,
			fixture.m_GuardGroup.m_sRuntimeStatus,
			fixture.m_Operation.m_eSettlementState,
			fixture.m_Mission.m_eStatus,
			independent,
			PACKAGED_GATES);
	}

	protected void ProveOutcomeGrace(HST_RescuePOWOperationProofReport report)
	{
		HST_RescuePOWOperationProofFixture success = m_Fixtures.BuildAdmittedFixture("outcome_success");
		HST_RescuePOWOperationProofFixture death = m_Fixtures.BuildAdmittedFixture("outcome_death");
		HST_RescuePOWOperationProofFixture handoff = m_Fixtures.BuildAdmittedFixture("escort_handoff");
		HST_RescuePOWOperationProofFixture graceDisconnect = m_Fixtures.BuildAdmittedFixture("grace_disconnect");
		if (!m_Fixtures.Ready(success) || !m_Fixtures.Ready(death)
			|| !m_Fixtures.Ready(handoff) || !m_Fixtures.Ready(graceDisconnect))
		{
			report.m_sOutcomeGraceEvidence = "success [" + m_Fixtures.Failure(success)
				+ "] | death [" + m_Fixtures.Failure(death)
				+ "] | handoff [" + m_Fixtures.Failure(handoff)
				+ "] | grace disconnect [" + m_Fixtures.Failure(graceDisconnect)
				+ "] | " + PACKAGED_GATES;
			return;
		}
		bool custodyExact = true;
		foreach (HST_MissionAssetState captive : success.m_aCaptives)
		{
			string suffix = string.Format("_%1", captive.m_iRescueOrdinal);
			HST_RescuePOWTransitionResult freed = success.m_Service.HandleCaptiveCommand(
				success.m_State, success.m_Mission, captive, "free", "escort_alpha",
				"grace_free" + suffix);
			HST_RescuePOWTransitionResult following = success.m_Service.HandleCaptiveCommand(
				success.m_State, success.m_Mission, captive, "follow", "escort_alpha",
				"grace_follow" + suffix);
			custodyExact = custodyExact && freed && freed.m_bSuccess
				&& following && following.m_bSuccess;
		}
		bool graceEligible = HST_RescuePOWOperationService.CanEnterExtractionGrace(
			success.m_State, success.m_Mission);
		int frozenBaseDeadline = success.m_Mission.m_iActiveUntilSecond;
		success.m_State.m_iElapsedSeconds = frozenBaseDeadline + 37;
		bool graceBegan = success.m_Service.BeginExtractionGrace(success.m_State, success.m_Mission);
		bool graceExact = graceBegan && success.m_Mission.m_bRescueExtractionGrace
			&& success.m_Mission.m_iRescueGraceUntilSecond
				== frozenBaseDeadline + HST_RescuePOWOperationService.EXTRACTION_GRACE_SECONDS
			&& success.m_Mission.m_iRemainingSeconds
				== HST_RescuePOWOperationService.EXTRACTION_GRACE_SECONDS - 37
			&& success.m_Mission.m_sRuntimePhase == HST_RescuePOWOperationService.RESCUE_GRACE_PHASE;
		bool extractedExact = true;
		foreach (HST_MissionAssetState captive : success.m_aCaptives)
		{
			captive.m_vCurrentPosition = captive.m_vTargetPosition;
			HST_RescuePOWTransitionResult extracted = success.m_Service.HandleCaptiveCommand(
				success.m_State, success.m_Mission, captive, "extract", "escort_alpha",
				string.Format("grace_extract_%1", captive.m_iRescueOrdinal));
			extractedExact = extractedExact && extracted && extracted.m_bSuccess;
		}
		bool completionCandidate = success.m_Service.FindCompletedActiveMissionId(success.m_State)
			== success.m_Mission.m_sInstanceId;
		success.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_SUCCEEDED;
		bool settled = success.m_Service.ReconcileAfterMissionOutcomes(success.m_State);
		bool settlementExact = settled
			&& success.m_Operation.m_eSettlementState
				== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& success.m_Operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
			&& !success.m_Mission.m_sSettlementId.IsEmpty()
			&& !success.m_Mission.m_bRescueExtractionGrace
			&& success.m_Mission.m_iRescueGraceUntilSecond == 0;

		bool deathObserved = death.m_Service.MarkCaptiveDeathObserved(
			death.m_State, death.m_Mission, death.m_aCaptives[0], "proof casualty");
		bool failureCandidate = death.m_Service.FindFailedActiveMissionId(death.m_State)
			== death.m_Mission.m_sInstanceId;

		HST_MissionAssetState handoffCaptive = handoff.m_aCaptives[0];
		HST_RescuePOWTransitionResult handoffFree = handoff.m_Service.HandleCaptiveCommand(
			handoff.m_State, handoff.m_Mission, handoffCaptive, "free",
			"escort_alpha", "handoff_free");
		HST_RescuePOWTransitionResult handoffFollow = handoff.m_Service.HandleCaptiveCommand(
			handoff.m_State, handoff.m_Mission, handoffCaptive, "follow",
			"escort_alpha", "handoff_follow");
		handoff.m_State.FindPlayer("escort_alpha").m_iLastSeenPlayerId = -1;
		bool handoffReleased = handoff.m_Service.ReconcileDisconnectedEscortForProof(
			handoff.m_State, handoff.m_Mission, handoffCaptive);
		bool handoffExact = handoffFree && handoffFree.m_bSuccess
			&& handoffFollow && handoffFollow.m_bSuccess && handoffReleased
			&& handoffCaptive.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED
			&& handoffCaptive.m_sRescueEscortIdentityId.IsEmpty()
			&& handoffCaptive.m_sRescueCarrierVehicleId.IsEmpty()
			&& handoffCaptive.m_sRescueCarrierSeatToken.IsEmpty();

		bool graceDisconnectCustody = true;
		foreach (HST_MissionAssetState graceCaptive : graceDisconnect.m_aCaptives)
		{
			HST_RescuePOWTransitionResult graceFree = graceDisconnect.m_Service.HandleCaptiveCommand(
					graceDisconnect.m_State, graceDisconnect.m_Mission, graceCaptive,
					"free", "escort_alpha",
					"disconnect_free_" + graceCaptive.m_sAssetId);
			HST_RescuePOWTransitionResult graceFollow = graceDisconnect.m_Service.HandleCaptiveCommand(
					graceDisconnect.m_State, graceDisconnect.m_Mission, graceCaptive,
					"follow", "escort_alpha",
					"disconnect_follow_" + graceCaptive.m_sAssetId);
			graceDisconnectCustody = graceDisconnectCustody
				&& graceFree && graceFree.m_bSuccess && graceFollow && graceFollow.m_bSuccess;
		}
		graceDisconnect.m_State.m_iElapsedSeconds = graceDisconnect.m_Mission.m_iActiveUntilSecond;
		bool disconnectGraceBegan = graceDisconnect.m_Service.BeginExtractionGrace(
			graceDisconnect.m_State, graceDisconnect.m_Mission);
		graceDisconnect.m_State.FindPlayer("escort_alpha").m_iLastSeenPlayerId = -1;
		bool graceDisconnectFails = graceDisconnect.m_Service.FindFailedActiveMissionId(
			graceDisconnect.m_State) == graceDisconnect.m_Mission.m_sInstanceId;
		report.m_bOutcomeGraceExact = custodyExact && graceEligible && graceExact
			&& extractedExact && completionCandidate && settlementExact
			&& deathObserved && failureCandidate && handoffExact
			&& graceDisconnectCustody && disconnectGraceBegan && graceDisconnectFails;
		report.m_sOutcomeGraceEvidence = string.Format(
			"custody %1 | hitch-stable grace eligible/began %2/%3 | all extracted %4 | completion candidate %5 | settlement %6 | death/failure candidate %7/%8",
			custodyExact,
			graceEligible,
			graceExact,
			extractedExact,
			completionCandidate,
			settlementExact,
			deathObserved,
			failureCandidate);
		report.m_sOutcomeGraceEvidence = report.m_sOutcomeGraceEvidence
			+ string.Format(" | disconnect handoff %1 | grace disconnect failure %2/%3/%4",
				handoffExact, graceDisconnectCustody, disconnectGraceBegan,
				graceDisconnectFails);
		report.m_sOutcomeGraceEvidence = report.m_sOutcomeGraceEvidence + " | " + PACKAGED_GATES;
	}

	protected void ProveRestoreQuarantine(HST_RescuePOWOperationProofReport report)
	{
		HST_RescuePOWOperationProofFixture restored = m_Fixtures.BuildAdmittedFixture("restore");
		HST_RescuePOWOperationProofFixture corrupt = m_Fixtures.BuildAdmittedFixture("corrupt");
		HST_RescuePOWOperationProofFixture ledger = m_Fixtures.BuildAdmittedFixture("ledger_corrupt");
		if (!m_Fixtures.Ready(restored) || !m_Fixtures.Ready(corrupt)
			|| !m_Fixtures.Ready(ledger))
		{
			report.m_sRestoreQuarantineEvidence = "restore [" + m_Fixtures.Failure(restored)
				+ "] | corrupt [" + m_Fixtures.Failure(corrupt)
				+ "] | ledger [" + m_Fixtures.Failure(ledger)
				+ "] | " + PACKAGED_GATES;
			return;
		}
		bool restoreChanged = restored.m_Service.ReconcileAfterRestore(restored.m_State);
		bool restoreExact = restoreChanged
			&& HST_RescuePOWOperationService.IsExactMission(restored.m_Mission)
			&& restored.m_Batch.m_bStrategicProjectionHeld
			&& restored.m_Operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& restored.m_State.m_aMissionAssets.Count()
				== HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT;

		corrupt.m_State.m_aMissionAssets.Insert(corrupt.m_aCaptives[0]);
		HST_RescuePOWAdmissionResult replay = corrupt.m_Service.ResolveCommittedAdmission(
			corrupt.m_State, corrupt.m_Mission);
		bool quarantineExact = replay && !replay.m_bSuccess
			&& HST_RescuePOWOperationService.IsQuarantinedMission(corrupt.m_Mission)
			&& corrupt.m_Operation.m_iContractVersion
				== HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION
			&& corrupt.m_GuardGroup.m_sRuntimeStatus
				== HST_RescuePOWOperationService.QUARANTINE_STATUS;
		bool captiveQuarantine = true;
		foreach (HST_MissionAssetState captive : corrupt.m_State.m_aMissionAssets)
		{
			if (captive && captive.m_sMissionInstanceId == corrupt.m_Mission.m_sInstanceId)
				captiveQuarantine = captiveQuarantine
					&& captive.m_iRescueContractVersion
						== HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION;
		}

		HST_ActiveMissionState historical = new HST_ActiveMissionState();
		historical.m_sInstanceId = "rescue_pow_proof_historical";
		historical.m_sMissionId = HST_RescuePOWOperationService.EXACT_MISSION_ID;
		historical.m_iOperationContractVersion = 0;
		bool noInvention = !HST_RescuePOWOperationService.IsExactOrQuarantinedMission(historical)
			&& historical.m_sOperationId.IsEmpty() && historical.m_sManifestId.IsEmpty();

		HST_CampaignSaveData baselineSave = new HST_CampaignSaveData();
		baselineSave.Capture(ledger.m_State);
		HST_RescuePOWSaveValidationService saveValidator = new HST_RescuePOWSaveValidationService();
		string baselineFailure = saveValidator.ValidateCurrentAggregate(
			baselineSave, baselineSave.m_aActiveMissions[0]);
		HST_RescuePOWTransitionResult firstLedgerFree = ledger.m_Service.HandleCaptiveCommand(
			ledger.m_State, ledger.m_Mission, ledger.m_aCaptives[0], "free",
			"escort_alpha", "ledger_first_request");
		HST_RescuePOWTransitionResult secondLedgerFree = ledger.m_Service.HandleCaptiveCommand(
			ledger.m_State, ledger.m_Mission, ledger.m_aCaptives[1], "free",
			"escort_alpha", "ledger_second_request");
		bool collisionPrepared = firstLedgerFree && firstLedgerFree.m_bSuccess
			&& secondLedgerFree && secondLedgerFree.m_bSuccess
			&& ledger.m_aCaptives[0].m_aRescueCommandReceipts.Count() == 1
			&& ledger.m_aCaptives[1].m_aRescueCommandReceipts.Count() == 1;
		if (collisionPrepared)
			ledger.m_aCaptives[1].m_aRescueCommandReceipts[0].m_sRequestId = ledger.m_aCaptives[0].m_aRescueCommandReceipts[0].m_sRequestId;
		HST_CampaignSaveData collisionSave = new HST_CampaignSaveData();
		collisionSave.Capture(ledger.m_State);
		string collisionFailure = saveValidator.ValidateCurrentAggregate(
			collisionSave, collisionSave.m_aActiveMissions[0]);
		bool ledgerCollisionExact = baselineFailure.IsEmpty() && collisionPrepared
			&& collisionFailure.Contains("command receipt ledger");
		report.m_bRestoreQuarantineExact = restoreExact && quarantineExact
			&& captiveQuarantine && noInvention && ledgerCollisionExact;
		report.m_sRestoreQuarantineEvidence = string.Format(
			"restore %1 | quarantine %2 | captive quarantine %3 | historical no-invention %4 | ledger collision %5 | %6",
			restoreExact,
			quarantineExact,
			captiveQuarantine,
			noInvention,
			ledgerCollisionExact,
			PACKAGED_GATES);
	}
}
