#ifdef ENABLE_DIAG
class HST_RadioSiteLifecycleProofReport
{
	bool m_bBindingAdmissionIsolationExact;
	bool m_bLifecycleOutcomesExact;
	bool m_bReceiptRevisionExact;
	bool m_bRestoreMigrationQuarantineExact;
	bool m_bInfluenceSuppressionExact;
	bool m_bOwnershipEquipmentExact;
	string m_sBindingAdmissionIsolationEvidence;
	string m_sLifecycleOutcomesEvidence;
	string m_sReceiptRevisionEvidence;
	string m_sRestoreMigrationQuarantineEvidence;
	string m_sInfluenceSuppressionEvidence;
	string m_sOwnershipEquipmentEvidence;
}

// Source-only seams. Native authored-entity discovery, actual explosive damage,
// damage-state reapplication, generated replacement entities, zone streaming,
// real save/restart, rendered UI, networking, and JIP remain packaged gates.
// These wrappers only expose the production service's protected durable-state
// transition helpers while deliberately skipping IEntity projection work.
class HST_RadioSiteLifecycleProofHarness : HST_RadioSiteLifecycleService
{
	override protected bool PrepareProjectionForAdmission(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission)
	{
		return state && site && mission;
	}

	override protected bool ReconcileAdmissionProjection(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset)
	{
		if (!state || !site || !mission || !asset)
			return false;
		asset.m_bSpawned = true;
		mission.m_bRuntimeSpawned = true;
		HST_MissionRuntimeEntityState runtime = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtime)
			runtime.m_bSpawned = true;
		return true;
	}

	override protected bool ReconcileProjectionAfterOutcome(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission,
		string transitionKind)
	{
		return state && site && mission && !transitionKind.IsEmpty();
	}

	override protected bool ReconcileProjectionForOutcomeReplay(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission)
	{
		return state && site && mission;
	}

	HST_RadioSiteTransitionResult AdmitResolvedMissionForProof(
		HST_CampaignState state,
		HST_ActiveMissionState mission)
	{
		return AdmitNewMission(state, mission);
	}

	bool MarkTargetDestroyedForProof(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset)
	{
		if (!state || !mission || !asset)
			return false;
		HST_RadioSiteState site = state.FindRadioSite(asset.m_sRadioSiteId);
		if (!site)
			return false;
		if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN)
		{
			bool thresholdReached;
			bool alreadyApplied;
			if (!RecordExactExplosiveEvidence(
					state,
					site,
					mission,
					asset,
					asset.m_vCurrentPosition,
					DEFAULT_DEMOLITION_REQUIRED_DAMAGE,
					"source_proof_exact_explosive_score_" + asset.m_sAssetId,
					thresholdReached,
					alreadyApplied)
				|| !thresholdReached || alreadyApplied)
				return false;
		}
		return MarkPhysicalTargetDestroyed(
			state,
			site,
			mission,
			asset,
			asset.m_vCurrentPosition,
			"source proof physical destruction receipt");
	}

	HST_RadioSiteTransitionResult ApplyOutcomeForProof(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		string transitionKind)
	{
		if (!mission)
			return null;
		string expectedTransitionKind;
		if (mission.m_sMissionId == DESTROY_MISSION_ID
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED)
			expectedTransitionKind = TRANSITION_DESTROY_SUCCESS;
		else if (mission.m_sMissionId == DESTROY_MISSION_ID
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED)
			expectedTransitionKind = TRANSITION_DESTROY_FAILURE;
		else if (mission.m_sMissionId == DESTROY_MISSION_ID
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED)
			expectedTransitionKind = TRANSITION_DESTROY_EXPIRY;
		else if (mission.m_sMissionId == REBUILD_MISSION_ID
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED)
			expectedTransitionKind = TRANSITION_REBUILD_SUCCESS;
		else if (mission.m_sMissionId == REBUILD_MISSION_ID
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED)
			expectedTransitionKind = TRANSITION_REBUILD_FAILURE;
		else if (mission.m_sMissionId == REBUILD_MISSION_ID
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED)
			expectedTransitionKind = TRANSITION_REBUILD_EXPIRY;
		if (transitionKind != expectedTransitionKind)
			return Reject(NewResult(mission), "proof transition kind does not match the production entrypoint");
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED)
			return OnMissionSucceeded(state, mission);
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED)
			return OnMissionFailed(state, mission);
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED)
			return OnMissionExpired(state, mission);
		return Reject(NewResult(mission), "proof mission status has no production outcome entrypoint");
	}
}

class HST_RadioSiteTownProofHarness : HST_TownService
{
	HST_ZoneState FindNearestEligibleRadioTowerForProof(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_ZoneState town)
	{
		return FindNearestEligibleRadioTower(state, preset, town);
	}
}

class HST_RadioSiteLifecycleProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_RadioSiteLifecycleProofHarness m_Service;
	ref HST_ZoneState m_Zone;
	ref HST_RadioSiteState m_Site;
}

class HST_RadioSiteLifecycleProofFixtureFactory
{
	static const string PROOF_ZONE_PREFIX = "radio_lifecycle_proof_zone_";
	static const string PROOF_MISSION_PREFIX = "radio_lifecycle_proof_mission_";
	static const string BORROWED_TOWER_PREFAB = "{7E2380494811A5FB}Prefabs/Structures/Infrastructure/Towers/TransmitterTower_01/TransmitterTower_01_medium.et";

	HST_RadioSiteLifecycleProofFixture BuildResolvedFixture(
		string suffix,
		HST_ERadioSiteTargetOwnership ownership = HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD)
	{
		HST_RadioSiteLifecycleProofFixture fixture = new HST_RadioSiteLifecycleProofFixture();
		fixture.m_State = new HST_CampaignState();
		fixture.m_State.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		fixture.m_State.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		fixture.m_State.m_sPresetId = "radio_site_lifecycle_proof";
		fixture.m_State.m_iCampaignSeed = 590059;
		fixture.m_State.m_iElapsedSeconds = 590;
		fixture.m_State.m_iWarLevel = 4;
		fixture.m_State.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		fixture.m_Preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		fixture.m_Service = new HST_RadioSiteLifecycleProofHarness();

		fixture.m_Zone = new HST_ZoneState();
		fixture.m_Zone.m_sZoneId = PROOF_ZONE_PREFIX + suffix;
		fixture.m_Zone.m_sDisplayName = "Radio Lifecycle Proof " + suffix;
		fixture.m_Zone.m_sOwnerFactionKey = fixture.m_Preset.m_sOccupierFactionKey;
		fixture.m_Zone.m_eType = HST_EZoneType.HST_ZONE_RADIO_TOWER;
		fixture.m_Zone.m_vPosition = BuildPosition(suffix);
		fixture.m_State.m_aZones.Insert(fixture.m_Zone);

		fixture.m_Site = new HST_RadioSiteState();
		fixture.m_Site.m_iContractVersion = HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION;
		fixture.m_Site.m_sSiteId = HST_RadioSiteLifecycleService.BuildSiteId(fixture.m_Zone.m_sZoneId);
		fixture.m_Site.m_sZoneId = fixture.m_Zone.m_sZoneId;
		fixture.m_Site.m_sTargetId = HST_RadioSiteLifecycleService.BuildTargetId(fixture.m_Zone.m_sZoneId);
		fixture.m_Site.m_sTargetPrefab = HST_RadioSiteLifecycleService.GENERATED_TOWER_PREFAB;
		if (ownership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD)
			fixture.m_Site.m_sTargetPrefab = BORROWED_TOWER_PREFAB;
		fixture.m_Site.m_vTargetPosition = fixture.m_Zone.m_vPosition;
		fixture.m_Site.m_sAuthoredTargetPrefab = BORROWED_TOWER_PREFAB;
		fixture.m_Site.m_vAuthoredTargetPosition = fixture.m_Zone.m_vPosition;
		fixture.m_Site.m_eLifecycleState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE;
		fixture.m_Site.m_eTargetOwnership = ownership;
		fixture.m_Site.m_sLastTransitionReason = "source proof resolved baseline";
		fixture.m_Site.m_iLastTransitionSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Site.m_iRevision = 1;
		fixture.m_State.m_aRadioSites.Insert(fixture.m_Site);
		return fixture;
	}

	HST_RadioSiteTransitionResult AdmitMission(
		HST_RadioSiteLifecycleProofFixture fixture,
		string missionId,
		string suffix)
	{
		if (!fixture || !fixture.m_State || !fixture.m_Service)
			return null;
		HST_ActiveMissionState mission = new HST_ActiveMissionState();
		mission.m_sInstanceId = PROOF_MISSION_PREFIX + suffix;
		mission.m_sMissionId = missionId;
		mission.m_sDisplayName = "Radio Lifecycle Proof Mission";
		mission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		mission.m_sTargetZoneId = fixture.m_Zone.m_sZoneId;
		mission.m_iStartedAtSecond = fixture.m_State.m_iElapsedSeconds;
		mission.m_iActiveUntilSecond = fixture.m_State.m_iElapsedSeconds + 600;
		mission.m_iRemainingSeconds = 600;
		fixture.m_State.m_aActiveMissions.Insert(mission);

		HST_MissionObjectiveState objective = new HST_MissionObjectiveState();
		objective.m_sObjectiveId = "objective_" + mission.m_sInstanceId;
		objective.m_sMissionInstanceId = mission.m_sInstanceId;
		objective.m_eType = HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET;
		objective.m_iRequiredProgress = 1;
		objective.m_iRequiredCount = 1;
		fixture.m_State.m_aMissionObjectives.Insert(objective);

		HST_CampaignTaskState task = new HST_CampaignTaskState();
		task.m_sTaskId = "task_" + mission.m_sInstanceId;
		task.m_sLinkedId = mission.m_sInstanceId;
		task.m_sTitle = "Radio Lifecycle Proof Mission";
		task.m_sDescription = "Source-only exact radio lifecycle proof task";
		task.m_sCategory = "mission";
		task.m_vPosition = fixture.m_Site.m_vTargetPosition;
		task.m_bActive = true;
		fixture.m_State.m_aCampaignTasks.Insert(task);

		if (!fixture.m_Service.PrepareNewMissionContract(fixture.m_State, mission))
			return null;
		return fixture.m_Service.AdmitResolvedMissionForProof(fixture.m_State, mission);
	}

	bool DriveDestroyed(
		HST_RadioSiteLifecycleProofFixture fixture,
		string suffix)
	{
		HST_RadioSiteTransitionResult admitted = AdmitMission(
			fixture,
			HST_RadioSiteLifecycleService.DESTROY_MISSION_ID,
			suffix + "_destroy");
		if (!admitted || !admitted.m_bAccepted || !admitted.m_Mission || !admitted.m_Asset)
			return false;
		if (!fixture.m_Service.MarkTargetDestroyedForProof(
			fixture.m_State,
			admitted.m_Mission,
			admitted.m_Asset))
			return false;
		admitted.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_SUCCEEDED;
		HST_RadioSiteTransitionResult outcome = fixture.m_Service.ApplyOutcomeForProof(
			fixture.m_State,
			admitted.m_Mission,
			HST_RadioSiteLifecycleService.TRANSITION_DESTROY_SUCCESS);
		return outcome && outcome.m_bAccepted
			&& fixture.m_Site.m_eLifecycleState
				== HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED;
	}

	protected vector BuildPosition(string suffix)
	{
		int offset = Math.Max(1, suffix.Length()) * 40;
		vector position = "12000 20 12000";
		position[0] = position[0] + offset;
		position[2] = position[2] + offset;
		return position;
	}
}

class HST_RadioSiteLifecycleProofService
{
	static const string PACKAGED_GATES = "packaged gates not claimed: native authored discovery, natural explosive destruction, damage-state reapplication, generated entity replacement, stream fold/re-entry, real restart, rendered UI, networking, and JIP";
	protected ref HST_RadioSiteLifecycleProofFixtureFactory m_Fixtures = new HST_RadioSiteLifecycleProofFixtureFactory();

	HST_RadioSiteLifecycleProofReport Run()
	{
		HST_RadioSiteLifecycleProofReport report = new HST_RadioSiteLifecycleProofReport();
		ProveBindingAdmissionIsolation(report);
		ProveLifecycleOutcomes(report);
		ProveReceiptRevision(report);
		ProveRestoreMigrationQuarantine(report);
		ProveInfluenceSuppression(report);
		ProveOwnershipEquipment(report);
		return report;
	}

	protected void ProveBindingAdmissionIsolation(HST_RadioSiteLifecycleProofReport report)
	{
		HST_RadioSiteLifecycleProofFixture fixture = m_Fixtures.BuildResolvedFixture("binding_admission");
		HST_ActiveMissionState historical = new HST_ActiveMissionState();
		historical.m_sInstanceId = "radio_lifecycle_proof_historical";
		historical.m_sMissionId = HST_RadioSiteLifecycleService.DESTROY_MISSION_ID;
		historical.m_sTargetZoneId = fixture.m_Zone.m_sZoneId;
		historical.m_eStatus = HST_EMissionStatus.HST_MISSION_SUCCEEDED;
		fixture.m_State.m_aActiveMissions.Insert(historical);

		int sitesBefore = fixture.m_State.m_aRadioSites.Count();
		int assetsBefore = fixture.m_State.m_aMissionAssets.Count();
		HST_RadioSiteTransitionResult admitted = m_Fixtures.AdmitMission(
			fixture,
			HST_RadioSiteLifecycleService.DESTROY_MISSION_ID,
			"binding_admission");
		string duplicateFailure;
		bool duplicateBlocked = !fixture.m_Service.CanStartMission(
			fixture.m_State,
			HST_RadioSiteLifecycleService.DESTROY_MISSION_ID,
			fixture.m_Zone.m_sZoneId,
			duplicateFailure);
		string unrelatedFailure;
		bool unrelatedAllowed = fixture.m_Service.CanStartMission(
			fixture.m_State,
			"support_city_supplies",
			fixture.m_Zone.m_sZoneId,
			unrelatedFailure);

		bool identityExact = fixture.m_Site.m_sSiteId
			== HST_RadioSiteLifecycleService.BuildSiteId(fixture.m_Zone.m_sZoneId)
			&& fixture.m_Site.m_sTargetId
				== HST_RadioSiteLifecycleService.BuildTargetId(fixture.m_Zone.m_sZoneId)
			&& HST_RadioSiteLifecycleService.IsBroadcastOperational(
				fixture.m_State,
				fixture.m_Zone.m_sZoneId);
		bool admissionExact = admitted && admitted.m_bAccepted && admitted.m_bStateChanged
			&& admitted.m_Mission && admitted.m_Asset
			&& fixture.m_State.m_aRadioSites.Count() == sitesBefore
			&& fixture.m_State.m_aMissionAssets.Count() == assetsBefore + 1
			&& admitted.m_Mission.m_sRadioSiteId == fixture.m_Site.m_sSiteId
			&& admitted.m_Mission.m_iRadioSiteRevision == fixture.m_Site.m_iRevision
			&& admitted.m_Asset.m_sRadioSiteId == fixture.m_Site.m_sSiteId
			&& admitted.m_Asset.m_sPrefab == fixture.m_Site.m_sTargetPrefab
			&& fixture.m_Site.m_sActiveMissionInstanceId == admitted.m_Mission.m_sInstanceId;
		bool isolationExact = !HST_RadioSiteLifecycleService.IsExactMission(historical)
			&& historical.m_iRadioSiteContractVersion == 0;
		report.m_bBindingAdmissionIsolationExact = identityExact && admissionExact
			&& isolationExact && duplicateBlocked && unrelatedAllowed;
		report.m_sBindingAdmissionIsolationEvidence = string.Format(
			"identity %1 | admission %2 | site rows %3 | target assets %4 | historical contract0 %5 | duplicate blocked %6 (%7) | unrelated allowed %8 | %9",
			identityExact,
			admissionExact,
			fixture.m_State.m_aRadioSites.Count(),
			fixture.m_State.m_aMissionAssets.Count(),
			isolationExact,
			duplicateBlocked,
			duplicateFailure,
			unrelatedAllowed,
			PACKAGED_GATES);
	}

	protected void ProveLifecycleOutcomes(HST_RadioSiteLifecycleProofReport report)
	{
		HST_RadioSiteLifecycleProofFixture destroySuccess = m_Fixtures.BuildResolvedFixture("destroy_success");
		bool destroySuccessExact = m_Fixtures.DriveDestroyed(destroySuccess, "destroy_success")
			&& !destroySuccess.m_Site.m_sLastDestructionReceiptId.IsEmpty()
			&& !HST_RadioSiteLifecycleService.IsBroadcastOperational(
				destroySuccess.m_State,
				destroySuccess.m_Zone.m_sZoneId);

		HST_RadioSiteLifecycleProofFixture destroyFailure = m_Fixtures.BuildResolvedFixture("destroy_failure");
		HST_RadioSiteTransitionResult destroyFailureAdmission = m_Fixtures.AdmitMission(
			destroyFailure,
			HST_RadioSiteLifecycleService.DESTROY_MISSION_ID,
			"destroy_failure");
		HST_RadioSiteTransitionResult destroyFailureOutcome;
		if (destroyFailureAdmission && destroyFailureAdmission.m_Mission)
		{
			destroyFailureAdmission.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
			destroyFailureOutcome = destroyFailure.m_Service.ApplyOutcomeForProof(
				destroyFailure.m_State,
				destroyFailureAdmission.m_Mission,
				HST_RadioSiteLifecycleService.TRANSITION_DESTROY_FAILURE);
		}
		HST_MissionObjectiveService objectiveService = new HST_MissionObjectiveService();
		bool genericObjectiveTickChanged = objectiveService.Tick(destroyFailure.m_State);
		HST_MissionObjectiveState failedRadioObjective;
		foreach (HST_MissionObjectiveState candidateObjective : destroyFailure.m_State.m_aMissionObjectives)
		{
			if (destroyFailureAdmission && candidateObjective
				&& candidateObjective.m_sMissionInstanceId == destroyFailureAdmission.m_Mission.m_sInstanceId)
			{
				failedRadioObjective = candidateObjective;
				break;
			}
		}
		bool genericObjectiveIsolationExact = !genericObjectiveTickChanged
			&& failedRadioObjective && failedRadioObjective.m_bFailed
			&& failedRadioObjective.m_bCleanupComplete
			&& destroyFailureAdmission.m_Mission.m_bRuntimeCleanupComplete;
		bool destroyFailureExact = destroyFailureOutcome && destroyFailureOutcome.m_bAccepted
			&& destroyFailure.m_Site.m_eLifecycleState
				== HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE
			&& genericObjectiveIsolationExact;

		HST_RadioSiteLifecycleProofFixture rebuildSuccess = m_Fixtures.BuildResolvedFixture("rebuild_success");
		bool rebuildSuccessSeeded = m_Fixtures.DriveDestroyed(rebuildSuccess, "rebuild_success_seed");
		string rebuildSuccessDestructionEpoch = rebuildSuccess.m_Site.m_sLastDestructionReceiptId;
		string rebuildStartFailure;
		bool rebuildReachable = rebuildSuccessSeeded && rebuildSuccess.m_Service.CanStartMission(
			rebuildSuccess.m_State,
			HST_RadioSiteLifecycleService.REBUILD_MISSION_ID,
			rebuildSuccess.m_Zone.m_sZoneId,
			rebuildStartFailure);
		HST_RadioSiteTransitionResult rebuildSuccessAdmission = m_Fixtures.AdmitMission(
			rebuildSuccess,
			HST_RadioSiteLifecycleService.REBUILD_MISSION_ID,
			"rebuild_success");
		bool rebuildEquipmentDestroyed = rebuildSuccessAdmission
			&& rebuildSuccessAdmission.m_Asset
			&& rebuildSuccess.m_Service.MarkTargetDestroyedForProof(
				rebuildSuccess.m_State,
				rebuildSuccessAdmission.m_Mission,
				rebuildSuccessAdmission.m_Asset);
		if (rebuildSuccessAdmission && rebuildSuccessAdmission.m_Mission)
			rebuildSuccessAdmission.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_SUCCEEDED;
		HST_RadioSiteTransitionResult rebuildSuccessOutcome;
		if (rebuildSuccessAdmission && rebuildSuccessAdmission.m_Mission)
		{
			rebuildSuccessOutcome = rebuildSuccess.m_Service.ApplyOutcomeForProof(
				rebuildSuccess.m_State,
				rebuildSuccessAdmission.m_Mission,
				HST_RadioSiteLifecycleService.TRANSITION_REBUILD_SUCCESS);
		}
		string repeatedRebuildFailure;
		bool repeatedRebuildBlocked = rebuildSuccessOutcome && rebuildSuccessOutcome.m_bAccepted
			&& !rebuildSuccess.m_Service.CanStartMission(
				rebuildSuccess.m_State,
				HST_RadioSiteLifecycleService.REBUILD_MISSION_ID,
				rebuildSuccess.m_Zone.m_sZoneId,
				repeatedRebuildFailure);
		bool rebuildSuccessExact = rebuildReachable && rebuildEquipmentDestroyed
			&& rebuildSuccessOutcome && rebuildSuccessOutcome.m_bAccepted
			&& rebuildSuccess.m_Site.m_eLifecycleState
				== HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED
			&& rebuildSuccess.m_Site.m_sLastDestructionReceiptId == rebuildSuccessDestructionEpoch
			&& !rebuildSuccess.m_Site.m_sLastRebuildReceiptId.IsEmpty()
			&& repeatedRebuildBlocked;
		HST_RadioSiteTransitionResult directRepeatAdmission = m_Fixtures.AdmitMission(
			rebuildSuccess,
			HST_RadioSiteLifecycleService.REBUILD_MISSION_ID,
			"rebuild_direct_bypass");
		bool directRepeatAdmissionBlocked = directRepeatAdmission
			&& !directRepeatAdmission.m_bAccepted
			&& rebuildSuccess.m_Site.m_iContractVersion
				== HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION;

		HST_RadioSiteLifecycleProofFixture rebuildFailure = m_Fixtures.BuildResolvedFixture("rebuild_failure");
		bool rebuildFailureSeeded = m_Fixtures.DriveDestroyed(rebuildFailure, "rebuild_failure_seed");
		HST_RadioSiteTransitionResult rebuildFailureAdmission = m_Fixtures.AdmitMission(
			rebuildFailure,
			HST_RadioSiteLifecycleService.REBUILD_MISSION_ID,
			"rebuild_failure");
		HST_RadioSiteTransitionResult rebuildFailureOutcome;
		if (rebuildFailureAdmission && rebuildFailureAdmission.m_Mission)
		{
			rebuildFailureAdmission.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
			rebuildFailureOutcome = rebuildFailure.m_Service.ApplyOutcomeForProof(
				rebuildFailure.m_State,
				rebuildFailureAdmission.m_Mission,
				HST_RadioSiteLifecycleService.TRANSITION_REBUILD_FAILURE);
		}
		bool rebuildFailureExact = rebuildFailureSeeded && rebuildFailureOutcome
			&& rebuildFailureOutcome.m_bAccepted
			&& rebuildFailure.m_Site.m_eLifecycleState
				== HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE
			&& rebuildFailure.m_Site.m_eTargetOwnership
				== HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN
			&& !rebuildFailure.m_Site.m_sLastRebuildReceiptId.IsEmpty();

		HST_RadioSiteLifecycleProofFixture rebuildExpiry = m_Fixtures.BuildResolvedFixture("rebuild_expiry");
		bool rebuildExpirySeeded = m_Fixtures.DriveDestroyed(rebuildExpiry, "rebuild_expiry_seed");
		HST_RadioSiteTransitionResult rebuildExpiryAdmission = m_Fixtures.AdmitMission(
			rebuildExpiry,
			HST_RadioSiteLifecycleService.REBUILD_MISSION_ID,
			"rebuild_expiry");
		HST_RadioSiteTransitionResult rebuildExpiryOutcome;
		if (rebuildExpiryAdmission && rebuildExpiryAdmission.m_Mission)
		{
			rebuildExpiryAdmission.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_EXPIRED;
			rebuildExpiryOutcome = rebuildExpiry.m_Service.ApplyOutcomeForProof(
				rebuildExpiry.m_State,
				rebuildExpiryAdmission.m_Mission,
				HST_RadioSiteLifecycleService.TRANSITION_REBUILD_EXPIRY);
		}
		bool rebuildExpiryExact = rebuildExpirySeeded && rebuildExpiryOutcome
			&& rebuildExpiryOutcome.m_bAccepted
			&& rebuildExpiry.m_Site.m_eLifecycleState
				== HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE
			&& rebuildExpiry.m_Site.m_iRebuiltAtSecond
				>= rebuildExpiry.m_Site.m_iRebuildStartedAtSecond;

		report.m_bLifecycleOutcomesExact = destroySuccessExact && destroyFailureExact
			&& rebuildSuccessExact && directRepeatAdmissionBlocked
			&& rebuildFailureExact && rebuildExpiryExact;
		report.m_sLifecycleOutcomesEvidence = string.Format(
			"destroy success/failure %1/%2 | generic objective isolation %3 | rebuild reachable %4 (%5) | rebuild success/failure/expiry %6/%7/%8",
			destroySuccessExact,
			destroyFailureExact,
			genericObjectiveIsolationExact,
			rebuildReachable,
			rebuildStartFailure,
			rebuildSuccessExact,
			rebuildFailureExact,
			rebuildExpiryExact);
		report.m_sLifecycleOutcomesEvidence = report.m_sLifecycleOutcomesEvidence + string.Format(
			" | repeated epoch blocked %1 (%2) | direct admission bypass blocked %3 | %4",
			repeatedRebuildBlocked,
			repeatedRebuildFailure,
			directRepeatAdmissionBlocked,
			PACKAGED_GATES);
	}

	protected void ProveReceiptRevision(HST_RadioSiteLifecycleProofReport report)
	{
		HST_RadioSiteLifecycleProofFixture fixture = m_Fixtures.BuildResolvedFixture("receipt_revision");
		HST_RadioSiteTransitionResult admitted = m_Fixtures.AdmitMission(
			fixture,
			HST_RadioSiteLifecycleService.DESTROY_MISSION_ID,
			"receipt_revision_destroy");
		if (!admitted || !admitted.m_Mission || !admitted.m_Asset)
		{
			report.m_bReceiptRevisionExact = false;
			report.m_sReceiptRevisionEvidence = "initial exact admission unavailable | " + PACKAGED_GATES;
			return;
		}
		int admissionRevision = fixture.m_Site.m_iRevision;
		int assetsAfterAdmission = fixture.m_State.m_aMissionAssets.Count();
		HST_RadioSiteTransitionResult admissionReplay = fixture.m_Service.AdmitResolvedMissionForProof(
			fixture.m_State,
			admitted.m_Mission);
		bool admissionReplayExact = admissionReplay && admissionReplay.m_bAccepted
			&& admissionReplay.m_bAlreadyApplied
			&& fixture.m_Site.m_iRevision == admissionRevision
			&& fixture.m_State.m_aMissionAssets.Count() == assetsAfterAdmission;

		bool destructionRecorded = fixture.m_Service.MarkTargetDestroyedForProof(
			fixture.m_State,
			admitted.m_Mission,
			admitted.m_Asset);
		admitted.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_SUCCEEDED;
		HST_RadioSiteTransitionResult outcome = fixture.m_Service.ApplyOutcomeForProof(
			fixture.m_State,
			admitted.m_Mission,
			HST_RadioSiteLifecycleService.TRANSITION_DESTROY_SUCCESS);
		int outcomeRevision = fixture.m_Site.m_iRevision;
		HST_RadioSiteTransitionResult outcomeReplay = fixture.m_Service.ApplyOutcomeForProof(
			fixture.m_State,
			admitted.m_Mission,
			HST_RadioSiteLifecycleService.TRANSITION_DESTROY_SUCCESS);
		bool outcomeReplayExact = destructionRecorded && outcome && outcome.m_bAccepted
			&& outcomeReplay && outcomeReplay.m_bAccepted && outcomeReplay.m_bAlreadyApplied
			&& fixture.m_Site.m_iRevision == outcomeRevision;

		HST_RadioSiteTransitionResult rebuildAdmission = m_Fixtures.AdmitMission(
			fixture,
			HST_RadioSiteLifecycleService.REBUILD_MISSION_ID,
			"receipt_revision_rebuild");
		HST_RadioSiteTransitionResult rebuildOutcome;
		if (rebuildAdmission && rebuildAdmission.m_Mission)
		{
			rebuildAdmission.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
			rebuildOutcome = fixture.m_Service.ApplyOutcomeForProof(
				fixture.m_State,
				rebuildAdmission.m_Mission,
				HST_RadioSiteLifecycleService.TRANSITION_REBUILD_FAILURE);
		}

		HST_RadioSiteState snapshot = CopySite(fixture.m_Site);
		int moneyBefore = fixture.m_State.m_iFactionMoney;
		int hrBefore = fixture.m_State.m_iHR;
		int eventsBefore = fixture.m_State.m_aCampaignEvents.Count();
		int assetsBefore = fixture.m_State.m_aMissionAssets.Count();
		HST_RadioSiteTransitionResult staleReplay = fixture.m_Service.ApplyOutcomeForProof(
			fixture.m_State,
			admitted.m_Mission,
			HST_RadioSiteLifecycleService.TRANSITION_DESTROY_SUCCESS);
		string staleReason = "missing stale-replay result";
		if (staleReplay)
			staleReason = staleReplay.m_sReason;
		bool staleExact = rebuildOutcome && rebuildOutcome.m_bAccepted
			&& staleReplay && !staleReplay.m_bAccepted
			&& staleReason.Contains("stale")
			&& SitesEqual(snapshot, fixture.m_Site)
			&& fixture.m_State.m_iFactionMoney == moneyBefore
			&& fixture.m_State.m_iHR == hrBefore
			&& fixture.m_State.m_aCampaignEvents.Count() == eventsBefore
			&& fixture.m_State.m_aMissionAssets.Count() == assetsBefore;

		report.m_bReceiptRevisionExact = admissionReplayExact && outcomeReplayExact && staleExact;
		report.m_sReceiptRevisionEvidence = string.Format(
			"admission replay %1 revision %2 | outcome replay %3 revision %4 | later cycle %5 | stale rejection immutable %6 (%7) | %8",
			admissionReplayExact,
			admissionRevision,
			outcomeReplayExact,
			outcomeRevision,
			rebuildOutcome && rebuildOutcome.m_bAccepted,
			staleExact,
			staleReason,
			PACKAGED_GATES);
	}

	protected void ProveRestoreMigrationQuarantine(HST_RadioSiteLifecycleProofReport report)
	{
		HST_RadioSiteLifecycleProofFixture exact = m_Fixtures.BuildResolvedFixture("restore_exact");
		bool exactDestroyed = m_Fixtures.DriveDestroyed(exact, "restore_exact");
		HST_CampaignSaveData exactSave = new HST_CampaignSaveData();
		exactSave.Capture(exact.m_State);
		HST_CampaignState exactRestored = exactSave.Restore();
		HST_RadioSiteState restoredSite;
		if (exactRestored)
			restoredSite = exactRestored.FindRadioSite(exact.m_Site.m_sSiteId);
		HST_ActiveMissionState restoredMission;
		if (exactRestored && exact.m_Site.m_sLastTransitionMissionInstanceId != "")
			restoredMission = exactRestored.FindActiveMission(exact.m_Site.m_sLastTransitionMissionInstanceId);
		bool roundtripExact = exactDestroyed && restoredSite && restoredMission
			&& restoredSite != exact.m_Site
			&& restoredSite.m_iContractVersion == HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION
			&& restoredSite.m_eLifecycleState
				== HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED
			&& restoredSite.m_iRevision == exact.m_Site.m_iRevision
			&& restoredMission.m_iRadioSiteRevision == restoredSite.m_iRevision;

		HST_RadioSiteLifecycleProofFixture legacyFixture = m_Fixtures.BuildResolvedFixture("migration_legacy");
		legacyFixture.m_State.m_iSchemaVersion = 58;
		legacyFixture.m_State.m_iLastLoadedSchemaVersion = 58;
		legacyFixture.m_State.m_aRadioSites.Clear();
		HST_ActiveMissionState legacyMission = new HST_ActiveMissionState();
		legacyMission.m_sInstanceId = "radio_lifecycle_proof_legacy_active";
		legacyMission.m_sMissionId = HST_RadioSiteLifecycleService.DESTROY_MISSION_ID;
		legacyMission.m_sTargetZoneId = legacyFixture.m_Zone.m_sZoneId;
		legacyMission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		legacyMission.m_iRemainingSeconds = 300;
		legacyFixture.m_State.m_aActiveMissions.Insert(legacyMission);
		int legacyMoney = legacyFixture.m_State.m_iFactionMoney;
		int legacyHR = legacyFixture.m_State.m_iHR;
		HST_CampaignSaveData legacySave = new HST_CampaignSaveData();
		legacySave.Capture(legacyFixture.m_State);
		HST_CampaignState migrated = legacySave.Restore();
		HST_RadioSiteState migratedSite;
		HST_ActiveMissionState migratedMission;
		if (migrated)
		{
			migratedSite = migrated.FindRadioSiteForZone(legacyFixture.m_Zone.m_sZoneId);
			migratedMission = migrated.FindActiveMission(legacyMission.m_sInstanceId);
		}
		bool migrationExact = migratedSite && migratedMission
			&& migratedSite.m_iContractVersion == HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION
			&& migratedSite.m_eLifecycleState
				== HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE
			&& migratedSite.m_eTargetOwnership
				== HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED
			&& migratedSite.m_sLastDestructionReceiptId.IsEmpty()
			&& migratedSite.m_sLastRebuildReceiptId.IsEmpty()
			&& migratedMission.m_iRadioSiteContractVersion == 0
			&& migratedMission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED
			&& migrated.m_iFactionMoney == legacyMoney && migrated.m_iHR == legacyHR
			&& HasEvent(migrated, HST_RadioSiteSaveValidationService.MIGRATION_EVENT_ID);

		HST_RadioSiteLifecycleProofFixture corruptFixture = m_Fixtures.BuildResolvedFixture("restore_corrupt");
		HST_RadioSiteTransitionResult corruptAdmission = m_Fixtures.AdmitMission(
			corruptFixture,
			HST_RadioSiteLifecycleService.DESTROY_MISSION_ID,
			"restore_corrupt_active");
		HST_CampaignSaveData corruptSave = new HST_CampaignSaveData();
		corruptSave.Capture(corruptFixture.m_State);
		if (corruptSave.m_aRadioSites.Count() > 0)
			corruptSave.m_aRadioSites[0].m_vTargetPosition = "50000 20 50000";
		HST_CampaignState quarantined = corruptSave.Restore();
		HST_RadioSiteState quarantinedSite;
		HST_ActiveMissionState quarantinedMission;
		HST_MissionAssetState quarantinedAsset;
		HST_MissionRuntimeEntityState quarantinedRuntime;
		HST_CampaignTaskState quarantinedTask;
		HST_MissionObjectiveState quarantinedObjective;
		if (quarantined)
		{
			quarantinedSite = quarantined.FindRadioSite(corruptFixture.m_Site.m_sSiteId);
			if (corruptAdmission && corruptAdmission.m_Mission)
			{
				quarantinedMission = quarantined.FindActiveMission(
					corruptAdmission.m_Mission.m_sInstanceId);
				quarantinedTask = quarantined.FindCampaignTask(
					"task_" + corruptAdmission.m_Mission.m_sInstanceId);
			}
			if (corruptAdmission && corruptAdmission.m_Asset)
			{
				quarantinedAsset = quarantined.FindMissionAsset(corruptAdmission.m_Asset.m_sAssetId);
				quarantinedRuntime = quarantined.FindMissionRuntimeEntity(
					corruptAdmission.m_Asset.m_sEntityId);
			}
			foreach (HST_MissionObjectiveState objective : quarantined.m_aMissionObjectives)
			{
				if (quarantinedMission
					&& objective.m_sMissionInstanceId == quarantinedMission.m_sInstanceId)
				{
					quarantinedObjective = objective;
					break;
				}
			}
		}
		bool quarantinedSiteExact = quarantinedSite
			&& quarantinedSite.m_iContractVersion
				== HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION
			&& quarantinedSite.m_eLifecycleState
				== HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_QUARANTINED
			&& !HST_RadioSiteLifecycleService.IsBroadcastOperational(
				quarantined,
				corruptFixture.m_Zone.m_sZoneId);
		bool quarantinedMissionExact = quarantinedMission
			&& quarantinedMission.m_iRadioSiteContractVersion
				== HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION
			&& quarantinedMission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED
			&& !quarantinedMission.m_bRuntimeSpawned
			&& quarantinedMission.m_bRuntimeCleanupComplete;
		bool quarantinedProjectionExact = quarantinedAsset && !quarantinedAsset.m_bSpawned
			&& quarantinedRuntime && !quarantinedRuntime.m_bSpawned
			&& quarantinedTask && !quarantinedTask.m_bActive
			&& !quarantinedTask.m_bSucceeded && quarantinedTask.m_bFailed
			&& quarantinedObjective && quarantinedObjective.m_bFailed
			&& quarantinedObjective.m_bCleanupComplete;
		bool quarantineExact = quarantinedSiteExact && quarantinedMissionExact
			&& quarantinedProjectionExact
			&& HasEvent(quarantined, HST_RadioSiteSaveValidationService.CONFLICT_EVENT_ID);

		report.m_bRestoreMigrationQuarantineExact = roundtripExact && migrationExact && quarantineExact;
		report.m_sRestoreMigrationQuarantineEvidence = string.Format(
			"roundtrip %1 | pre59 no-invention/fail-closed %2 | current corruption quarantine %3 | %4",
			roundtripExact,
			migrationExact,
			quarantineExact,
			PACKAGED_GATES);
	}

	protected void ProveInfluenceSuppression(HST_RadioSiteLifecycleProofReport report)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iElapsedSeconds = 590;
		HST_CampaignPreset preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		HST_ZoneState town = BuildZone("influence_town", HST_EZoneType.HST_ZONE_TOWN, preset.m_sResistanceFactionKey, "20000 20 20000");
		HST_ZoneState nearRadio = BuildZone("influence_near", HST_EZoneType.HST_ZONE_RADIO_TOWER, preset.m_sOccupierFactionKey, "20400 20 20000");
		HST_ZoneState farRadio = BuildZone("influence_far", HST_EZoneType.HST_ZONE_RADIO_TOWER, preset.m_sResistanceFactionKey, "20800 20 20000");
		state.m_aZones.Insert(town);
		state.m_aZones.Insert(nearRadio);
		state.m_aZones.Insert(farRadio);
		HST_RadioSiteState nearSite = BuildOperationalSite(nearRadio, HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED);
		HST_RadioSiteState farSite = BuildOperationalSite(farRadio, HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE);
		state.m_aRadioSites.Insert(nearSite);
		state.m_aRadioSites.Insert(farSite);

		HST_RadioSiteTownProofHarness towns = new HST_RadioSiteTownProofHarness();
		HST_ZoneState selected = towns.FindNearestEligibleRadioTowerForProof(state, preset, town);
		bool fartherOnlineSelected = selected == farRadio
			&& !HST_RadioSiteLifecycleService.IsBroadcastOperational(state, nearRadio.m_sZoneId)
			&& HST_RadioSiteLifecycleService.IsBroadcastOperational(state, farRadio.m_sZoneId);
		farSite.m_eLifecycleState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING;
		bool rebuildingSuppressed = towns.FindNearestEligibleRadioTowerForProof(state, preset, town) == null;
		farSite.m_eLifecycleState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_QUARANTINED;
		farSite.m_iContractVersion = HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION;
		bool quarantinedSuppressed = !HST_RadioSiteLifecycleService.IsBroadcastOperational(state, farRadio.m_sZoneId);
		farSite.m_iContractVersion = HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION;
		farSite.m_eLifecycleState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE;
		farSite.m_eTargetOwnership = HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED;
		farSite.m_sTargetPrefab = "";
		farSite.m_vTargetPosition = "0 0 0";
		bool unresolvedSuppressed = !HST_RadioSiteLifecycleService.IsBroadcastOperational(state, farRadio.m_sZoneId);

		report.m_bInfluenceSuppressionExact = fartherOnlineSelected && rebuildingSuppressed
			&& quarantinedSuppressed && unresolvedSuppressed;
		report.m_sInfluenceSuppressionEvidence = string.Format(
			"nearer destroyed skipped/farther online selected %1 | rebuilding %2 | quarantine %3 | unresolved %4 | %5",
			fartherOnlineSelected,
			rebuildingSuppressed,
			quarantinedSuppressed,
			unresolvedSuppressed,
			PACKAGED_GATES);
	}

	protected void ProveOwnershipEquipment(HST_RadioSiteLifecycleProofReport report)
	{
		HST_RadioSiteLifecycleProofFixture fixture = m_Fixtures.BuildResolvedFixture(
			"ownership_equipment",
			HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD);
		string borrowedTargetId = fixture.m_Site.m_sTargetId;
		string borrowedPrefab = fixture.m_Site.m_sTargetPrefab;
		vector borrowedPosition = fixture.m_Site.m_vTargetPosition;
		string frozenAuthoredPrefab = fixture.m_Site.m_sAuthoredTargetPrefab;
		vector frozenAuthoredPosition = fixture.m_Site.m_vAuthoredTargetPosition;
		HST_RadioSiteTransitionResult firstDestroy = m_Fixtures.AdmitMission(
			fixture,
			HST_RadioSiteLifecycleService.DESTROY_MISSION_ID,
			"ownership_destroy_failure");
		bool borrowedAssetExact = firstDestroy && firstDestroy.m_Asset
			&& firstDestroy.m_Asset.m_sPrefab == borrowedPrefab;
		HST_RadioSiteTransitionResult failed;
		if (firstDestroy && firstDestroy.m_Mission)
		{
			firstDestroy.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
			failed = fixture.m_Service.ApplyOutcomeForProof(
				fixture.m_State,
				firstDestroy.m_Mission,
				HST_RadioSiteLifecycleService.TRANSITION_DESTROY_FAILURE);
		}
		bool borrowedFailurePreserved = failed && failed.m_bAccepted
			&& fixture.m_Site.m_eTargetOwnership
				== HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD
			&& fixture.m_Site.m_sTargetId == borrowedTargetId
			&& fixture.m_Site.m_sTargetPrefab == borrowedPrefab
			&& fixture.m_Site.m_vTargetPosition == borrowedPosition;

		bool borrowedDestroyed = m_Fixtures.DriveDestroyed(fixture, "ownership_destroy_success")
			&& fixture.m_Site.m_eTargetOwnership
				== HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD
			&& fixture.m_Site.m_sTargetId == borrowedTargetId;
		HST_RadioSiteTransitionResult rebuild = m_Fixtures.AdmitMission(
			fixture,
			HST_RadioSiteLifecycleService.REBUILD_MISSION_ID,
			"ownership_rebuild");
		bool handoffExact = rebuild && rebuild.m_bAccepted && rebuild.m_Asset
			&& fixture.m_Site.m_eTargetOwnership
				== HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN
			&& fixture.m_Site.m_sTargetPrefab == HST_RadioSiteLifecycleService.GENERATED_TOWER_PREFAB
			&& fixture.m_Site.m_sAuthoredTargetPrefab == frozenAuthoredPrefab
			&& fixture.m_Site.m_vAuthoredTargetPosition == frozenAuthoredPosition
			&& rebuild.m_Asset.m_sPrefab == HST_RadioSiteLifecycleService.REBUILD_EQUIPMENT_PREFAB
			&& !HST_RadioSiteLifecycleService.IsSupportedTransmitterPrefab(rebuild.m_Asset.m_sPrefab)
			&& fixture.m_Site.m_sTargetId == borrowedTargetId
			&& fixture.m_Site.m_vTargetPosition == borrowedPosition;

		report.m_bOwnershipEquipmentExact = borrowedAssetExact && borrowedFailurePreserved
			&& borrowedDestroyed && handoffExact;
		report.m_sOwnershipEquipmentEvidence = string.Format(
			"borrowed mission asset %1 | failure preserved borrowed descriptor %2 | destroyed tombstone retained without source deletion claim %3 | explicit rebuild handoff/equipment %4 | %5",
			borrowedAssetExact,
			borrowedFailurePreserved,
			borrowedDestroyed,
			handoffExact,
			PACKAGED_GATES);
	}

	protected HST_RadioSiteState CopySite(HST_RadioSiteState source)
	{
		if (!source)
			return null;
		HST_RadioSiteState target = new HST_RadioSiteState();
		target.m_iContractVersion = source.m_iContractVersion;
		target.m_sSiteId = source.m_sSiteId;
		target.m_sZoneId = source.m_sZoneId;
		target.m_sTargetId = source.m_sTargetId;
		target.m_sTargetPrefab = source.m_sTargetPrefab;
		target.m_vTargetPosition = source.m_vTargetPosition;
		target.m_sAuthoredTargetPrefab = source.m_sAuthoredTargetPrefab;
		target.m_vAuthoredTargetPosition = source.m_vAuthoredTargetPosition;
		target.m_eLifecycleState = source.m_eLifecycleState;
		target.m_eTargetOwnership = source.m_eTargetOwnership;
		target.m_sActiveMissionInstanceId = source.m_sActiveMissionInstanceId;
		target.m_sActiveMissionId = source.m_sActiveMissionId;
		target.m_sActiveTransitionRequestId = source.m_sActiveTransitionRequestId;
		target.m_sLastDestructionReceiptId = source.m_sLastDestructionReceiptId;
		target.m_sLastDestructionMissionInstanceId = source.m_sLastDestructionMissionInstanceId;
		target.m_iDestroyedAtSecond = source.m_iDestroyedAtSecond;
		target.m_sLastRebuildReceiptId = source.m_sLastRebuildReceiptId;
		target.m_sLastRebuildMissionInstanceId = source.m_sLastRebuildMissionInstanceId;
		target.m_iRebuildStartedAtSecond = source.m_iRebuildStartedAtSecond;
		target.m_iRebuiltAtSecond = source.m_iRebuiltAtSecond;
		target.m_sLastTransitionRequestId = source.m_sLastTransitionRequestId;
		target.m_sLastTransitionMissionInstanceId = source.m_sLastTransitionMissionInstanceId;
		target.m_sLastTransitionKind = source.m_sLastTransitionKind;
		target.m_eLastTransitionFromState = source.m_eLastTransitionFromState;
		target.m_eLastTransitionToState = source.m_eLastTransitionToState;
		target.m_iLastTransitionRecordedRevision = source.m_iLastTransitionRecordedRevision;
		target.m_sLastTransitionReason = source.m_sLastTransitionReason;
		target.m_iLastTransitionSecond = source.m_iLastTransitionSecond;
		target.m_iRevision = source.m_iRevision;
		return target;
	}

	protected bool SitesEqual(HST_RadioSiteState first, HST_RadioSiteState second)
	{
		if (!first || !second)
			return false;
		if (first.m_iContractVersion != second.m_iContractVersion
			|| first.m_sSiteId != second.m_sSiteId
			|| first.m_sZoneId != second.m_sZoneId
			|| first.m_sTargetId != second.m_sTargetId)
			return false;
		if (first.m_sTargetPrefab != second.m_sTargetPrefab
			|| first.m_vTargetPosition != second.m_vTargetPosition
			|| first.m_sAuthoredTargetPrefab != second.m_sAuthoredTargetPrefab
			|| first.m_vAuthoredTargetPosition != second.m_vAuthoredTargetPosition
			|| first.m_eLifecycleState != second.m_eLifecycleState
			|| first.m_eTargetOwnership != second.m_eTargetOwnership)
			return false;
		if (first.m_sActiveMissionInstanceId != second.m_sActiveMissionInstanceId
			|| first.m_sActiveMissionId != second.m_sActiveMissionId
			|| first.m_sActiveTransitionRequestId != second.m_sActiveTransitionRequestId)
			return false;
		if (first.m_sLastDestructionReceiptId != second.m_sLastDestructionReceiptId
			|| first.m_sLastDestructionMissionInstanceId != second.m_sLastDestructionMissionInstanceId
			|| first.m_iDestroyedAtSecond != second.m_iDestroyedAtSecond)
			return false;
		if (first.m_sLastRebuildReceiptId != second.m_sLastRebuildReceiptId
			|| first.m_sLastRebuildMissionInstanceId != second.m_sLastRebuildMissionInstanceId
			|| first.m_iRebuildStartedAtSecond != second.m_iRebuildStartedAtSecond
			|| first.m_iRebuiltAtSecond != second.m_iRebuiltAtSecond)
			return false;
		if (first.m_sLastTransitionRequestId != second.m_sLastTransitionRequestId
			|| first.m_sLastTransitionMissionInstanceId != second.m_sLastTransitionMissionInstanceId
			|| first.m_sLastTransitionKind != second.m_sLastTransitionKind)
			return false;
		if (first.m_eLastTransitionFromState != second.m_eLastTransitionFromState
			|| first.m_eLastTransitionToState != second.m_eLastTransitionToState
			|| first.m_iLastTransitionRecordedRevision != second.m_iLastTransitionRecordedRevision)
			return false;
		return first.m_sLastTransitionReason == second.m_sLastTransitionReason
			&& first.m_iLastTransitionSecond == second.m_iLastTransitionSecond
			&& first.m_iRevision == second.m_iRevision;
	}

	protected HST_ZoneState BuildZone(
		string suffix,
		HST_EZoneType zoneType,
		string ownerFactionKey,
		vector position)
	{
		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = "radio_lifecycle_proof_" + suffix;
		zone.m_sDisplayName = "Radio Lifecycle Proof " + suffix;
		zone.m_eType = zoneType;
		zone.m_sOwnerFactionKey = ownerFactionKey;
		zone.m_vPosition = position;
		return zone;
	}

	protected HST_RadioSiteState BuildOperationalSite(
		HST_ZoneState zone,
		HST_ERadioSiteLifecycleState lifecycle)
	{
		HST_RadioSiteState site = new HST_RadioSiteState();
		site.m_iContractVersion = HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION;
		site.m_sSiteId = HST_RadioSiteLifecycleService.BuildSiteId(zone.m_sZoneId);
		site.m_sZoneId = zone.m_sZoneId;
		site.m_sTargetId = HST_RadioSiteLifecycleService.BuildTargetId(zone.m_sZoneId);
		site.m_sTargetPrefab = HST_RadioSiteLifecycleProofFixtureFactory.BORROWED_TOWER_PREFAB;
		site.m_vTargetPosition = zone.m_vPosition;
		site.m_sAuthoredTargetPrefab = HST_RadioSiteLifecycleProofFixtureFactory.BORROWED_TOWER_PREFAB;
		site.m_vAuthoredTargetPosition = zone.m_vPosition;
		site.m_eLifecycleState = lifecycle;
		site.m_eTargetOwnership = HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD;
		if (lifecycle == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED
			|| lifecycle == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING)
		{
			site.m_sLastDestructionReceiptId = "radio_lifecycle_proof_destruction_" + zone.m_sZoneId;
			site.m_sLastDestructionMissionInstanceId = "radio_lifecycle_proof_destroy_mission_" + zone.m_sZoneId;
			site.m_iDestroyedAtSecond = 1;
		}
		if (lifecycle == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING)
		{
			site.m_sTargetPrefab = HST_RadioSiteLifecycleService.GENERATED_TOWER_PREFAB;
			site.m_eTargetOwnership = HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN;
			site.m_iRebuildStartedAtSecond = 1;
		}
		site.m_iRevision = 1;
		return site;
	}

	protected bool HasEvent(HST_CampaignState state, string eventId)
	{
		if (!state || eventId.IsEmpty())
			return false;
		foreach (HST_CampaignEventState eventState : state.m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				return true;
		}
		return false;
	}
}
#endif
