#ifdef ENABLE_DIAG
// The campaign coordinator's full-debug runner calls Run() and publishes the
// integrated proof evidence. This source proof deliberately does not spawn
// process-local world entities; live vehicle/crew creation, successful physical
// fold, and rematerialization remain separate packaged-runtime assertions.
class HST_MissionConvoyOperationProofReport
{
	bool m_bAdmissionExact;
	bool m_bAdmissionRollbackExact;
	bool m_bProjectionExact;
	bool m_bCasualtyRestoreExact;
	bool m_bSettlementExact;
	bool m_bRestoreExact;
	bool m_bCorruptionRejected;
	bool m_bMarkerExact;
	bool m_bWatchdogExact;
	string m_sAdmissionEvidence;
	string m_sAdmissionRollbackEvidence;
	string m_sProjectionEvidence;
	string m_sCasualtyRestoreEvidence;
	string m_sSettlementEvidence;
	string m_sRestoreEvidence;
	string m_sCorruptionEvidence;
	string m_sMarkerEvidence;
	string m_sWatchdogEvidence;
}

class HST_MissionConvoyOperationProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_PhysicalWarService m_PhysicalWar;
	ref HST_MissionConvoyOperationService m_Service;
	ref HST_ActiveMissionState m_Mission;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceManifestState m_Manifest;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_MissionAssetState m_Cargo;
	ref HST_MissionConvoyAdmissionResult m_Admission;
	string m_sFailureReason;
}

class HST_MissionConvoyOperationProofService
{
	static const string PROOF_SOURCE_ZONE_ID = "mission_convoy_proof_source";
	static const string PROOF_TARGET_ZONE_ID = "mission_convoy_proof_target";
	static const string PROOF_FACTION_KEY = "US";
	static const string PROOF_GROUP_CATALOG_ENTRY_ID = "us_sentry_team";
	static const string PROOF_CREW_PREFAB = "{3BF36BDEEB33AEC9}Prefabs/Groups/BLUFOR/Group_US_SentryTeam.et";
	static const string PROOF_CREW_MEMBER_PREFAB = "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et";
	static const string PROOF_VEHICLE_PREFAB = "{B55C6990A6A9411B}Prefabs/Vehicles/Wheeled/M998/M998_covered.et";
	static const string PROOF_CARGO_PREFAB = "{6985327711303750}Prefabs/Objects/HST/HST_MissionProp_CitySupplies.et";
	static const string PROOF_CAPTIVE_PREFAB = "{6985327711303730}Prefabs/Objects/HST/HST_MissionProp_Captives.et";
	static const int PROOF_CREW_PER_VEHICLE = 2;
	static const float PROOF_ROUTE_START_X = 200000.0;
	static const float PROOF_ROUTE_END_X = 210000.0;
	static const float PROOF_ROUTE_Z = 200000.0;
	static const int PROOF_VIRTUAL_ADVANCE_SECONDS = 600;

	HST_MissionConvoyOperationProofReport Run()
	{
		HST_MissionConvoyOperationProofReport report = new HST_MissionConvoyOperationProofReport();
		ProveAdmission(report);
		ProveAdmissionRollback(report);
		ProveProjection(report);
		ProveCasualtyRestore(report);
		ProveSettlement(report);
		ProveRestore(report);
		ProveCorruptionRejection(report);
		ProveMarkers(report);
		ProveWatchdog(report);
		return report;
	}

	protected void ProveAdmission(HST_MissionConvoyOperationProofReport report)
	{
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("admission", true, "convoy_money");
		HST_MissionConvoyOperationProofFixture replay = BuildAdmittedFixture("admission", true, "convoy_money");
		HST_MissionConvoyOperationProofFixture captive = BuildAdmittedFixture("admission_captive", true, "convoy_prisoners");
		HST_MissionConvoyOperationProofFixture noCargo = BuildAdmittedFixture("admission_no_cargo", true, "convoy_reinforcements");
		if (!Ready(fixture) || !Ready(replay) || !Ready(captive) || !Ready(noCargo))
		{
			report.m_sAdmissionEvidence = BuildPairFailure(fixture, replay)
				+ " | " + BuildPairFailure(captive, noCargo);
			return;
		}

		bool identityExact = fixture.m_Mission.m_sOperationId == HST_MissionConvoyOperationService.BuildOperationId(fixture.m_Mission)
			&& fixture.m_Mission.m_sManifestId == HST_MissionConvoyOperationService.BuildManifestId(fixture.m_Mission)
			&& fixture.m_Mission.m_sSpawnResultId == HST_MissionConvoyOperationService.BuildSpawnResultId(fixture.m_Mission)
			&& CountOperations(fixture.m_State, fixture.m_Operation.m_sOperationId) == 1
			&& CountManifests(fixture.m_State, fixture.m_Manifest.m_sManifestId) == 1
			&& CountBatches(fixture.m_State, fixture.m_Batch.m_sResultId) == 1
			&& CountElements(fixture.m_State, fixture.m_Operation.m_sOperationId) == HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT;
		bool deterministic = replay.m_Operation.m_sOperationId == fixture.m_Operation.m_sOperationId
			&& replay.m_Manifest.m_sManifestId == fixture.m_Manifest.m_sManifestId
			&& replay.m_Manifest.m_sManifestHash == fixture.m_Manifest.m_sManifestHash
			&& replay.m_Batch.m_sResultId == fixture.m_Batch.m_sResultId;
		bool manifestExact = ValidateFrozenManifest(fixture);
		bool backlinksExact = ValidateBacklinks(fixture);
		bool operationalExact = AreExactMissionGroupsOperational(fixture.m_State, fixture.m_Mission);
		bool optionalCargoShapesExact = captive.m_Manifest.m_aAssets.Count() == 1
			&& captive.m_Manifest.m_aAssets[0].m_sRole == HST_MissionConvoyOperationService.CAPTIVE_ROLE
			&& captive.m_Manifest.m_aAssets[0].m_sKind == HST_MissionConvoyOperationService.CAPTIVE_KIND
			&& noCargo.m_Manifest.m_aAssets.Count() == 0;
		HST_ConvoyElementState cargoCarrier = fixture.m_State.FindConvoyElement(fixture.m_Cargo.m_sConvoyElementId);
		bool cargoExact = fixture.m_Manifest.m_aAssets.Count() == 1
			&& fixture.m_Manifest.m_aAssets[0].m_sAssignedVehicleSlotId == HST_MissionConvoyOperationService.BuildVehicleSlotId(fixture.m_Mission, 0)
			&& fixture.m_Cargo.m_sAssignedVehicleSlotId == fixture.m_Manifest.m_aAssets[0].m_sAssignedVehicleSlotId
			&& fixture.m_Cargo.m_sConvoyElementId == HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, 0)
			&& cargoCarrier && cargoCarrier.m_sCargoAssetId == fixture.m_Cargo.m_sAssetId;

		report.m_bAdmissionExact = identityExact && deterministic && manifestExact && backlinksExact
			&& cargoExact && operationalExact && optionalCargoShapesExact;
		report.m_sAdmissionEvidence = string.Format(
			"rows operation/manifest/batch/elements %1/%2/%3/%4 | frozen groups/vehicles/members/assets %5/%6/%7/%8",
			CountOperations(fixture.m_State, fixture.m_Operation.m_sOperationId),
			CountManifests(fixture.m_State, fixture.m_Manifest.m_sManifestId),
			CountBatches(fixture.m_State, fixture.m_Batch.m_sResultId),
			CountElements(fixture.m_State, fixture.m_Operation.m_sOperationId),
			fixture.m_Manifest.m_aGroups.Count(),
			fixture.m_Manifest.m_aVehicles.Count(),
			fixture.m_Manifest.m_aMembers.Count(),
			fixture.m_Manifest.m_aAssets.Count());
		report.m_sAdmissionEvidence = report.m_sAdmissionEvidence + string.Format(
			" | held slots %1/%2",
			fixture.m_Batch.m_aSlotResults.Count(),
			fixture.m_Batch.m_iExpectedSlotCount);
		report.m_sAdmissionEvidence = report.m_sAdmissionEvidence + string.Format(
			" | deterministic hash %1 | links/cargo/operational %2/%3/%4",
			fixture.m_Manifest.m_sManifestHash == replay.m_Manifest.m_sManifestHash,
			backlinksExact,
			cargoExact,
			operationalExact);
		report.m_sAdmissionEvidence = report.m_sAdmissionEvidence + string.Format(
			" | valid captive/no-cargo manifest shapes %1/%2",
			captive.m_Manifest.m_aAssets.Count(),
			noCargo.m_Manifest.m_aAssets.Count());
	}

	protected void ProveAdmissionRollback(HST_MissionConvoyOperationProofReport report)
	{
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("admission_rollback", false, "convoy_money");
		if (!Prepared(fixture))
		{
			report.m_sAdmissionRollbackEvidence = BuildFixtureFailure(fixture);
			return;
		}

		HST_GeneratedRouteState route = fixture.m_State.FindGeneratedRoute("mission_convoy_proof_route_admission_rollback");
		if (!route)
		{
			report.m_sAdmissionRollbackEvidence = "rollback fixture route is missing";
			return;
		}
		route.m_aWaypoints.Clear();
		route.m_vStartPosition = BuildProofPosition(PROOF_ROUTE_START_X, PROOF_ROUTE_Z);
		route.m_vEndPosition = BuildProofPosition(PROOF_ROUTE_START_X + 10.0, PROOF_ROUTE_Z);
		fixture.m_Mission.m_vTargetPosition = route.m_vEndPosition;
		for (int i = 0; i < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; i++)
		{
			HST_MissionAssetState asset = fixture.m_State.FindMissionAsset(BuildVehicleAssetId(fixture.m_Mission, i));
			if (asset)
				asset.m_vTargetPosition = route.m_vEndPosition;
		}
		fixture.m_Cargo.m_vTargetPosition = route.m_vEndPosition;

		int operationsBefore = fixture.m_State.m_aOperations.Count();
		int manifestsBefore = fixture.m_State.m_aForceManifests.Count();
		int batchesBefore = fixture.m_State.m_aForceSpawnResults.Count();
		int elementsBefore = fixture.m_State.m_aConvoyElements.Count();
		int groupsBefore = fixture.m_State.m_aActiveGroups.Count();
		fixture.m_Admission = fixture.m_Service.AdmitNewMission(fixture.m_State, fixture.m_Preset, fixture.m_Mission);
		bool authorityRolledBack = fixture.m_Admission && !fixture.m_Admission.m_bSuccess
			&& fixture.m_State.m_aOperations.Count() == operationsBefore
			&& fixture.m_State.m_aForceManifests.Count() == manifestsBefore
			&& fixture.m_State.m_aForceSpawnResults.Count() == batchesBefore
			&& fixture.m_State.m_aConvoyElements.Count() == elementsBefore
			&& fixture.m_State.m_aActiveGroups.Count() == 0
			&& groupsBefore == HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT;
		bool linksRolledBack = fixture.m_Mission.m_sOperationId.IsEmpty()
			&& fixture.m_Mission.m_sManifestId.IsEmpty()
			&& fixture.m_Mission.m_sSpawnResultId.IsEmpty()
			&& fixture.m_Mission.m_sSettlementId.IsEmpty()
			&& ValidateRolledBackAssetLinks(fixture);
		bool terminalExact = fixture.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
			&& fixture.m_Mission.m_sLastRuntimeEventKey == HST_MissionConvoyOperationService.CONVOY_FAIL_EVENT
			&& !fixture.m_Mission.m_sRuntimeFailureReason.IsEmpty();
		int durableAuthorityArtifacts = CountFailedAdmissionAuthorityArtifacts(fixture);

		string cargoAdmissionEvidence;
		bool cargoAdmissionRejected = ProveInvalidCargoAdmission(cargoAdmissionEvidence);
		report.m_bAdmissionRollbackExact = authorityRolledBack && linksRolledBack && terminalExact
			&& durableAuthorityArtifacts == 0 && cargoAdmissionRejected;
		report.m_sAdmissionRollbackEvidence = string.Format(
			"accepted/changed %1/%2 | rows before operation/manifest/batch/elements/groups %3/%4/%5/%6/%7",
			fixture.m_Admission && fixture.m_Admission.m_bSuccess,
			fixture.m_Admission && fixture.m_Admission.m_bStateChanged,
			operationsBefore,
			manifestsBefore,
			batchesBefore,
			elementsBefore,
			groupsBefore);
		report.m_sAdmissionRollbackEvidence = report.m_sAdmissionRollbackEvidence + string.Format(
			" -> %1/%2/%3/%4/%5 | links '%6'/'%7'/'%8'",
			fixture.m_State.m_aOperations.Count(),
			fixture.m_State.m_aForceManifests.Count(),
			fixture.m_State.m_aForceSpawnResults.Count(),
			fixture.m_State.m_aConvoyElements.Count(),
			fixture.m_State.m_aActiveGroups.Count(),
			fixture.m_Mission.m_sOperationId,
			fixture.m_Mission.m_sManifestId,
			fixture.m_Mission.m_sSpawnResultId);
		report.m_sAdmissionRollbackEvidence = report.m_sAdmissionRollbackEvidence + string.Format(
			" | terminal %1 reason '%2' | durable exact authority artifacts %3",
			terminalExact,
			fixture.m_Admission && fixture.m_Admission.m_sFailureReason,
			durableAuthorityArtifacts);
		report.m_sAdmissionRollbackEvidence = report.m_sAdmissionRollbackEvidence + " | " + cargoAdmissionEvidence;
	}

	protected bool ProveInvalidCargoAdmission(out string evidence)
	{
		evidence = "invalid cargo admission fixtures did not run";
		HST_MissionConvoyOperationProofFixture duplicate = BuildAdmittedFixture("cargo_duplicate", false, "convoy_money");
		HST_MissionConvoyOperationProofFixture invalidPrefab = BuildAdmittedFixture("cargo_invalid_prefab", false, "convoy_money");
		HST_MissionConvoyOperationProofFixture wrongKind = BuildAdmittedFixture("cargo_wrong_kind", false, "convoy_money");
		HST_MissionConvoyOperationProofFixture disallowed = BuildAdmittedFixture("cargo_disallowed", false, "convoy_ammo");
		HST_MissionConvoyOperationProofFixture missing = BuildAdmittedFixture("cargo_missing", false, "convoy_money");
		HST_MissionConvoyOperationProofFixture wrongRuntime = BuildAdmittedFixture("cargo_wrong_runtime", false, "convoy_money");
		bool fixturesReady = Prepared(duplicate) && Prepared(invalidPrefab) && Prepared(wrongKind)
			&& Prepared(disallowed) && Prepared(missing) && Prepared(wrongRuntime);
		if (!fixturesReady)
		{
			evidence = "invalid cargo fixtures were not prepared";
			return false;
		}

		HST_MissionAssetState duplicateCargo = BuildProofCargoAsset(
			duplicate.m_Mission,
			"convoy_payload_duplicate",
			HST_MissionConvoyOperationService.CARGO_KIND,
			HST_MissionConvoyOperationService.PAYLOAD_ROLE,
			PROOF_CARGO_PREFAB);
		duplicate.m_State.m_aMissionAssets.Insert(duplicateCargo);
		invalidPrefab.m_Cargo.m_sPrefab = "{0000000000000000}Prefabs/Invalid/HST_MissingConvoyCargo.et";
		wrongKind.m_Cargo.m_sKind = HST_MissionConvoyOperationService.CAPTIVE_KIND;
		HST_MissionAssetState disallowedCargo = BuildProofCargoAsset(
			disallowed.m_Mission,
			"convoy_payload_disallowed",
			HST_MissionConvoyOperationService.CARGO_KIND,
			HST_MissionConvoyOperationService.PAYLOAD_ROLE,
			PROOF_CARGO_PREFAB);
		disallowed.m_State.m_aMissionAssets.Insert(disallowedCargo);
		int missingCargoIndex = missing.m_State.m_aMissionAssets.Find(missing.m_Cargo);
		if (missingCargoIndex >= 0)
			missing.m_State.m_aMissionAssets.Remove(missingCargoIndex);
		missing.m_Cargo = null;
		wrongRuntime.m_Mission.m_sRuntimeType = "unsupported_convoy_runtime";

		bool duplicateRejected = RejectsCargoAdmissionBeforeFreeze(duplicate, "more than one optional cargo row");
		bool invalidPrefabRejected = RejectsCargoAdmissionBeforeFreeze(invalidPrefab, "prefab is missing, invalid, or not an entity prefab");
		bool wrongKindRejected = RejectsCargoAdmissionBeforeFreeze(wrongKind, "role or kind is incompatible");
		bool disallowedRejected = RejectsCargoAdmissionBeforeFreeze(disallowed, "does not permit a cargo row");
		bool missingRejected = RejectsCargoAdmissionBeforeFreeze(missing, "requires exactly one compatible cargo row");
		bool wrongRuntimeRejected = RejectsCargoAdmissionBeforeFreeze(wrongRuntime, "runtime type is incompatible");
		string sourceTypeEvidence;
		bool sourceTypesRejected = ProveInvalidCargoSourceAdmission(sourceTypeEvidence);
		evidence = string.Format(
			"cargo admission rejected duplicate/invalid-prefab/wrong-kind/disallowed/missing/runtime %1/%2/%3/%4/%5/%6 before frozen authority",
			duplicateRejected,
			invalidPrefabRejected,
			wrongKindRejected,
			disallowedRejected,
			missingRejected,
			wrongRuntimeRejected);
		evidence = evidence + " | " + sourceTypeEvidence;
		return duplicateRejected && invalidPrefabRejected && wrongKindRejected
			&& disallowedRejected && missingRejected && wrongRuntimeRejected && sourceTypesRejected;
	}

	protected bool ProveInvalidCargoSourceAdmission(out string evidence)
	{
		evidence = "cargo source-type admission fixtures did not run";
		HST_MissionConvoyOperationProofFixture captiveObject = BuildAdmittedFixture(
			"cargo_captive_non_character", false, "convoy_prisoners");
		HST_MissionConvoyOperationProofFixture payloadCharacter = BuildAdmittedFixture(
			"cargo_payload_character", false, "convoy_money");
		if (!Prepared(captiveObject) || !Prepared(payloadCharacter))
			return false;

		captiveObject.m_Cargo.m_sPrefab = PROOF_CARGO_PREFAB;
		payloadCharacter.m_Cargo.m_sPrefab = PROOF_CAPTIVE_PREFAB;
		bool captiveRejected = RejectsCargoAdmissionBeforeFreeze(
			captiveObject,
			"not a boardable character with compartment access");
		bool payloadRejected = RejectsCargoAdmissionBeforeFreeze(
			payloadCharacter,
			"must be a non-character mission-asset entity");
		evidence = string.Format(
			"cargo admission rejected captive-object/payload-character source types %1/%2",
			captiveRejected,
			payloadRejected);
		return captiveRejected && payloadRejected;
	}

	protected bool RejectsCargoAdmissionBeforeFreeze(
		HST_MissionConvoyOperationProofFixture fixture,
		string expectedReason)
	{
		if (!fixture || !fixture.m_State || !fixture.m_Service || !fixture.m_Mission)
			return false;
		fixture.m_Admission = fixture.m_Service.AdmitNewMission(fixture.m_State, fixture.m_Preset, fixture.m_Mission);
		bool rejected = fixture.m_Admission && !fixture.m_Admission.m_bSuccess
			&& fixture.m_Admission.m_sFailureReason.Contains(expectedReason);
		bool noFrozenRows = fixture.m_State.m_aOperations.Count() == 0
			&& fixture.m_State.m_aForceManifests.Count() == 0
			&& fixture.m_State.m_aForceSpawnResults.Count() == 0
			&& fixture.m_State.m_aConvoyElements.Count() == 0;
		bool missionFailedClosed = fixture.m_Mission.m_iOperationContractVersion == 0
			&& fixture.m_Mission.m_sOperationId.IsEmpty() && fixture.m_Mission.m_sManifestId.IsEmpty()
			&& fixture.m_Mission.m_sSpawnResultId.IsEmpty()
			&& fixture.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
			&& fixture.m_Mission.m_sRuntimeFailureReason.Contains(expectedReason);
		return rejected && noFrozenRows && missionFailedClosed && ValidateRolledBackAssetLinks(fixture);
	}

	protected void ProveProjection(HST_MissionConvoyOperationProofReport report)
	{
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("projection", true, "convoy_money");
		if (!Ready(fixture))
		{
			report.m_sProjectionEvidence = BuildFixtureFailure(fixture);
			return;
		}

		float progressBefore = fixture.m_Operation.m_fRouteProgressMeters;
		fixture.m_State.m_iElapsedSeconds += PROOF_VIRTUAL_ADVANCE_SECONDS;
		bool advanced = fixture.m_Service.TickBeforePhysical(fixture.m_State, fixture.m_Preset, PROOF_VIRTUAL_ADVANCE_SECONDS);
		float expectedProgress = progressBefore + HST_MissionConvoyOperationService.EXACT_SPEED_METERS_PER_SECOND * PROOF_VIRTUAL_ADVANCE_SECONDS;
		int expectedETA = Math.Round((fixture.m_Operation.m_fRouteTotalDistanceMeters - expectedProgress) / HST_MissionConvoyOperationService.EXACT_SPEED_METERS_PER_SECOND);
		HST_ConvoyElementState lead = fixture.m_State.FindConvoyElement(HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, 0));
		HST_ConvoyElementState second = fixture.m_State.FindConvoyElement(HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, 1));
		HST_ConvoyElementState third = fixture.m_State.FindConvoyElement(HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, 2));
		float leadSpacing = -1.0;
		float rearSpacing = -1.0;
		float cargoLeadDistance = -1.0;
		if (lead && second)
			leadSpacing = Distance2D(lead.m_vCurrentPosition, second.m_vCurrentPosition);
		if (second && third)
			rearSpacing = Distance2D(second.m_vCurrentPosition, third.m_vCurrentPosition);
		if (lead)
			cargoLeadDistance = Distance2D(fixture.m_Cargo.m_vCurrentPosition, lead.m_vCurrentPosition);
		bool movementExact = advanced
			&& fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& fixture.m_Operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& Math.AbsFloat(fixture.m_Operation.m_fRouteProgressMeters - expectedProgress) < 0.1
			&& fixture.m_Operation.m_iLastProgressAtSecond == fixture.m_State.m_iElapsedSeconds
			&& fixture.m_Operation.m_sCurrentRouteId == "mission_convoy_proof_route_projection"
			&& Distance2D(fixture.m_Operation.m_vStrategicPosition, lead.m_vCurrentPosition) < 0.1
			&& fixture.m_Mission.m_iRuntimeETASeconds == expectedETA
			&& lead && second && third
			&& Math.AbsFloat(leadSpacing - HST_MissionConvoyOperationService.EXACT_FORMATION_SPACING_METERS) < 0.1
			&& Math.AbsFloat(rearSpacing - HST_MissionConvoyOperationService.EXACT_FORMATION_SPACING_METERS) < 0.1
			&& cargoLeadDistance >= 0.0 && cargoLeadDistance < 0.1;

		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		fixture.m_Operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		fixture.m_Operation.m_iMaterializationStateEnteredAtSecond = fixture.m_State.m_iElapsedSeconds;
		for (int i = 0; i < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; i++)
		{
			HST_MissionAssetState asset = fixture.m_State.FindMissionAsset(BuildVehicleAssetId(fixture.m_Mission, i));
			HST_ActiveGroupState group = fixture.m_State.FindActiveGroup(HST_MissionConvoyOperationService.BuildGroupId(fixture.m_Mission, i));
			asset.m_bSpawned = true;
			group.m_bSpawnedEntity = true;
		}
		bool materializationTick = fixture.m_Service.TickAfterPhysical(fixture.m_State);
		bool allOrNothingGate = !materializationTick
			&& fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			&& fixture.m_Operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& !fixture.m_PhysicalWar.HasExactMissionConvoyRuntime(fixture.m_Mission)
			&& CountPhysicalizedElements(fixture) == 0;

		int survivorsBeforeFold = CountElementSurvivors(fixture);
		float progressBeforeFold = fixture.m_Operation.m_fRouteProgressMeters;
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		fixture.m_Operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		fixture.m_Mission.m_sRuntimePhase = HST_MissionConvoyOperationService.CONVOY_MOVING;
		string foldReason;
		bool folded = fixture.m_PhysicalWar.FoldExactMissionConvoyRuntime(fixture.m_State, fixture.m_Mission, foldReason);
		bool foldRejectedWithoutMutation = !folded
			&& !foldReason.IsEmpty()
			&& fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			&& fixture.m_Operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE
			&& CountElementSurvivors(fixture) == survivorsBeforeFold
			&& Math.AbsFloat(fixture.m_Operation.m_fRouteProgressMeters - progressBeforeFold) < 0.1;

		HST_MissionConvoyOperationProofFixture partial = BuildAdmittedFixture("projection_partial_abandoned", true, "convoy_money");
		bool partialPrepared = Ready(partial);
		if (partialPrepared)
		{
			partial.m_State.m_iElapsedSeconds += PROOF_VIRTUAL_ADVANCE_SECONDS;
			partial.m_Service.TickBeforePhysical(partial.m_State, partial.m_Preset, PROOF_VIRTUAL_ADVANCE_SECONDS);
			partialPrepared = EliminateExactCrewAuthorityForElement(partial, 0);
		}
		HST_MissionAssetState partialAsset;
		HST_ConvoyElementState partialElement;
		HST_EOperationDutyState partialDuty;
		if (partialPrepared)
		{
			partialAsset = partial.m_State.FindMissionAsset(BuildVehicleAssetId(partial.m_Mission, 0));
			partialElement = partial.m_State.FindConvoyElement(HST_MissionConvoyOperationService.BuildElementId(partial.m_Mission, 0));
			partialDuty = partial.m_Operation.m_eDutyState;
		}
		bool partialAbandonedExact = partialPrepared && partialAsset && partialElement
			&& partialDuty == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& CountElementSurvivors(partial) > 0
			&& HST_MissionConvoyOperationService.IsRecoverableAbandonedVehicleRoot(partialAsset, partialElement);

		report.m_bProjectionExact = movementExact && allOrNothingGate && foldRejectedWithoutMutation && partialAbandonedExact;
		report.m_sProjectionEvidence = string.Format(
			"10-minute progress %1/%2m ETA %3/%4s | formation %5/%6m | cargo lead %7m | route cursor %8",
			Math.Round(fixture.m_Operation.m_fRouteProgressMeters),
			Math.Round(expectedProgress),
			fixture.m_Mission.m_iRuntimeETASeconds,
			expectedETA,
			Math.Round(leadSpacing),
			Math.Round(rearSpacing),
			Math.Round(cargoLeadDistance),
			fixture.m_Operation.m_sCurrentRouteId);
		report.m_sProjectionEvidence = report.m_sProjectionEvidence + string.Format(
			" | materialization state/physicalized %1/%2 | fold accepted %3 reason '%4' | survivors %5/%6",
			fixture.m_Operation.m_eMaterializationState,
			CountPhysicalizedElements(fixture),
			folded,
			foldReason,
			CountElementSurvivors(fixture),
			survivorsBeforeFold);
		report.m_sProjectionEvidence = report.m_sProjectionEvidence + string.Format(
			" | partial abandoned prepared/recoverable/remaining crew %1/%2/%3 while duty %4",
			partialPrepared,
			partialAbandonedExact,
			CountElementSurvivors(partial),
			partialDuty);
	}

	protected void ProveCasualtyRestore(HST_MissionConvoyOperationProofReport report)
	{
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("casualty_restore");
		if (!Ready(fixture))
		{
			report.m_sCasualtyRestoreEvidence = BuildFixtureFailure(fixture);
			return;
		}

		HST_ConvoyElementState element = fixture.m_State.FindConvoyElement(HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, 0));
		HST_ActiveGroupState group;
		if (element)
			group = fixture.m_State.FindActiveGroup(element.m_sGroupId);
		HST_ForceSpawnSlotResultState casualtySlot = fixture.m_Batch.FindSlotResult(
			HST_MissionConvoyOperationService.BuildMemberSlotId(fixture.m_Mission, 0, 0));
		HST_ForceSpawnSlotResultState survivingLaterSlot = fixture.m_Batch.FindSlotResult(
			HST_MissionConvoyOperationService.BuildMemberSlotId(fixture.m_Mission, 0, PROOF_CREW_PER_VEHICLE - 1));
		if (!element || !group || !casualtySlot || !survivingLaterSlot)
		{
			report.m_sCasualtyRestoreEvidence = "casualty fixture is missing its lead element, group, or durable member slot";
			return;
		}
		int originalLiving = CountLivingMemberSlots(fixture.m_Batch);
		casualtySlot.m_bCasualtyConfirmed = true;
		casualtySlot.m_bAliveVerified = false;
		casualtySlot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
		casualtySlot.m_iCasualtyAtSecond = fixture.m_State.m_iElapsedSeconds;
		casualtySlot.m_sRetirementReason = "mission convoy proof casualty";
		element.m_iSurvivingCrewCount--;
		// The element plus exact member slots are durable casualty authority. Leave
		// the physical group snapshot intentionally stale so restore must rebind it
		// instead of relying on the proof to synchronize four derivative counters.
		bool staleGroupSnapshotPrepared = group.m_iInfantryCount != element.m_iSurvivingCrewCount
			&& group.m_iLastSeenAliveCount != element.m_iSurvivingCrewCount
			&& group.m_iSurvivorInfantryCount != element.m_iSurvivingCrewCount
			&& group.m_iDurableLivingInfantryCount != element.m_iSurvivingCrewCount;
		int expectedLiving = originalLiving - 1;
		float progressBeforeRestore = fixture.m_Operation.m_fRouteProgressMeters;
		float expectedProgressAfterRestore = Math.Min(
			fixture.m_Operation.m_fRouteTotalDistanceMeters,
			progressBeforeRestore + fixture.m_Operation.m_fStrategicSpeedMetersPerSecond);
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		fixture.m_Operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		for (int physicalIndex = 0; physicalIndex < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; physicalIndex++)
		{
			HST_ConvoyElementState physicalElement = fixture.m_State.FindConvoyElement(HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, physicalIndex));
			HST_MissionAssetState physicalAsset = fixture.m_State.FindMissionAsset(BuildVehicleAssetId(fixture.m_Mission, physicalIndex));
			HST_ActiveGroupState physicalGroup = fixture.m_State.FindActiveGroup(HST_MissionConvoyOperationService.BuildGroupId(fixture.m_Mission, physicalIndex));
			if (physicalElement)
				physicalElement.m_bPhysicalized = true;
			if (physicalAsset)
				physicalAsset.m_bSpawned = true;
			if (physicalGroup)
				physicalGroup.m_bSpawnedEntity = true;
		}

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_ActiveMissionState restoredMission;
		HST_OperationRecordState restoredOperation;
		HST_ConvoyElementState restoredElement;
		HST_ActiveGroupState restoredGroup;
		HST_ForceSpawnResultState restoredBatch;
		HST_ForceSpawnSlotResultState restoredCasualty;
		HST_ForceSpawnSlotResultState restoredLaterSurvivor;
		bool normalized;
		if (restored)
		{
			restored.m_bRestoredFromPersistence = true;
			restored.m_iPersistenceRestoreSequence++;
			restoredMission = restored.FindActiveMission(fixture.m_Mission.m_sInstanceId);
			if (restoredMission)
				restoredOperation = restored.FindOperation(restoredMission.m_sOperationId);
			restoredElement = restored.FindConvoyElement(element.m_sElementId);
			restoredGroup = restored.FindActiveGroup(group.m_sGroupId);
			restoredBatch = restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
			if (restoredBatch)
			{
				restoredCasualty = restoredBatch.FindSlotResult(casualtySlot.m_sSlotId);
				restoredLaterSurvivor = restoredBatch.FindSlotResult(survivingLaterSlot.m_sSlotId);
			}
			HST_PhysicalWarService restoredPhysicalWar = new HST_PhysicalWarService();
			HST_MissionConvoyOperationService restoredService = new HST_MissionConvoyOperationService();
			restoredService.SetRuntimeServices(restoredPhysicalWar);
			restored.m_iElapsedSeconds++;
			normalized = restoredService.TickBeforePhysical(restored, fixture.m_Preset, 1);
		}

		bool restoredAuthorityRowsPresent = restored && restoredMission && restoredElement && restoredGroup && restoredBatch;
		bool casualtySlotExact = restoredCasualty
			&& restoredCasualty.m_bCasualtyConfirmed
			&& !restoredCasualty.m_bAliveVerified;
		bool laterSurvivorSlotExact = restoredLaterSurvivor
			&& restoredLaterSurvivor.m_bEverAlive
			&& !restoredLaterSurvivor.m_bCasualtyConfirmed
			&& restoredLaterSurvivor.m_bAliveVerified;
		bool durableSurvivorCountsExact = restoredElement && restoredGroup && restoredBatch
			&& restoredElement.m_iSurvivingCrewCount == element.m_iSurvivingCrewCount
			&& restoredGroup.m_iDurableLivingInfantryCount == element.m_iSurvivingCrewCount
			&& restoredGroup.m_iSurvivorInfantryCount == element.m_iSurvivingCrewCount
			&& CountLivingMemberSlots(restoredBatch) == expectedLiving;
		bool processFlagsCleared = restoredGroup && restoredElement
			&& !restoredGroup.m_bSpawnedEntity
			&& !restoredElement.m_bPhysicalized;
		bool survivorExact = restoredAuthorityRowsPresent
			&& casualtySlotExact
			&& laterSurvivorSlotExact
			&& durableSurvivorCountsExact
			&& processFlagsCleared;
		bool operationProjectionExact = restoredOperation
			&& restoredOperation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& restoredOperation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		bool operationRouteExact = restoredOperation && restored
			&& Math.AbsFloat(restoredOperation.m_fRouteProgressMeters - expectedProgressAfterRestore) < 0.1
			&& restoredOperation.m_fRouteProgressMeters >= progressBeforeRestore
			&& restoredOperation.m_iLastProgressAtSecond == restored.m_iElapsedSeconds
			&& restoredOperation.m_sLastProjectionReason.Contains("restore sequence");
		bool restoredMissionOpen = restoredMission
			&& restoredMission.m_sRuntimePhase != HST_MissionConvoyOperationService.CONVOY_FAILED;
		bool authorityExact = survivorExact && operationProjectionExact && operationRouteExact && restoredMissionOpen;
		report.m_bCasualtyRestoreExact = staleGroupSnapshotPrepared && normalized && authorityExact;
		int restoredLiving = -1;
		int restoredElementSurvivors = -1;
		int restoredGroupSurvivors = -1;
		int restoredProgress = -1;
		bool casualtyRetained;
		if (restoredBatch)
			restoredLiving = CountLivingMemberSlots(restoredBatch);
		if (restoredElement)
			restoredElementSurvivors = restoredElement.m_iSurvivingCrewCount;
		if (restoredGroup)
			restoredGroupSurvivors = restoredGroup.m_iSurvivorInfantryCount;
		if (restoredCasualty)
			casualtyRetained = restoredCasualty.m_bCasualtyConfirmed;
		if (restoredOperation)
			restoredProgress = Math.Round(restoredOperation.m_fRouteProgressMeters);
		report.m_sCasualtyRestoreEvidence = string.Format(
			"living slots %1 -> %2 -> %3 | element survivors %4/%5 | group survivors %6 | casualty retained %7 | physical restore virtualized %8",
			originalLiving,
			expectedLiving,
			restoredLiving,
			element.m_iSurvivingCrewCount,
			restoredElementSurvivors,
			restoredGroupSurvivors,
			casualtyRetained,
			authorityExact);
		report.m_sCasualtyRestoreEvidence = report.m_sCasualtyRestoreEvidence + string.Format(
			" | non-suffix seat0 casualty/later survivor %1/%2 | stale group snapshot prepared %3 | progress before/expected/actual %4/%5/%6 | one-tick progress timestamp exact %7",
			restoredCasualty && restoredCasualty.m_bCasualtyConfirmed,
			restoredLaterSurvivor && !restoredLaterSurvivor.m_bCasualtyConfirmed,
			staleGroupSnapshotPrepared,
			Math.Round(progressBeforeRestore),
			Math.Round(expectedProgressAfterRestore),
			restoredProgress,
			restoredOperation && restored && restoredOperation.m_iLastProgressAtSecond == restored.m_iElapsedSeconds);
	}

	protected void ProveSettlement(HST_MissionConvoyOperationProofReport report)
	{
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("settlement");
		if (!Ready(fixture))
		{
			report.m_sSettlementEvidence = BuildFixtureFailure(fixture);
			return;
		}

		fixture.m_Operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		fixture.m_Operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		fixture.m_Operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		fixture.m_Operation.m_iMaterializationStateEnteredAtSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Operation.m_fRouteProgressMeters = fixture.m_Operation.m_fRouteTotalDistanceMeters;
		fixture.m_Operation.m_iStrategicLastUpdateSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Mission.m_sRuntimePhase = HST_MissionConvoyOperationService.CONVOY_MOVING;

		HST_ConvoyElementState lead = fixture.m_State.FindConvoyElement(HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, 0));
		HST_MissionAssetState leadAsset = fixture.m_State.FindMissionAsset(BuildVehicleAssetId(fixture.m_Mission, 0));
		HST_ActiveGroupState leadGroup = fixture.m_State.FindActiveGroup(HST_MissionConvoyOperationService.BuildGroupId(fixture.m_Mission, 0));
		if (!lead || !leadAsset || !leadGroup)
		{
			report.m_sSettlementEvidence = "settlement fixture is missing its exact lead carrier roots";
			return;
		}

		// A completed route cursor is not arrival authority. The first physical
		// sample deliberately leaves the assigned carrier at the source.
		fixture.m_State.m_iElapsedSeconds++;
		bool farSampleChanged = fixture.m_Service.TickAfterPhysical(fixture.m_State);
		float farDistance = Distance2D(lead.m_vCurrentPosition, fixture.m_Operation.m_vRouteEndPosition);
		bool cursorOnlyRejected = fixture.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_MOVING
			&& fixture.m_Operation.m_iArrivalConfirmationCount == 0
			&& Math.AbsFloat(fixture.m_Operation.m_fRouteProgressMeters - fixture.m_Operation.m_fRouteTotalDistanceMeters) < 0.1
			&& farDistance > HST_MissionConvoyOperationService.EXACT_ARRIVAL_RADIUS_METERS;

		vector endpoint = fixture.m_Operation.m_vRouteEndPosition;
		lead.m_vCurrentPosition = endpoint;
		leadAsset.m_vCurrentPosition = endpoint;
		leadAsset.m_vLastKnownPosition = endpoint;
		leadGroup.m_vPosition = endpoint;
		fixture.m_State.m_iElapsedSeconds++;
		bool firstEndpointSample = fixture.m_Service.TickAfterPhysical(fixture.m_State);
		bool firstConfirmationExact = firstEndpointSample
			&& fixture.m_Operation.m_iArrivalConfirmationCount == 1
			&& fixture.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_MOVING;

		fixture.m_State.m_iElapsedSeconds++;
		bool arrived = fixture.m_Service.TickAfterPhysical(fixture.m_State);
		bool arrivalExact = cursorOnlyRejected && firstConfirmationExact && arrived
			&& fixture.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
			&& fixture.m_Mission.m_sLastRuntimeEventKey == HST_MissionConvoyOperationService.CONVOY_FAIL_EVENT
			&& CountElementsWithDisposition(fixture, HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ARRIVED) == HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT
			&& Math.AbsFloat(fixture.m_Operation.m_fRouteProgressMeters - fixture.m_Operation.m_fRouteTotalDistanceMeters) < 0.1;
		int arrivalRevision = fixture.m_Operation.m_iRevision;
		string arrivalEvent = fixture.m_Mission.m_sLastRuntimeEventKey;
		fixture.m_State.m_iElapsedSeconds++;
		bool arrivalReplayChanged = fixture.m_Service.TickBeforePhysical(fixture.m_State, fixture.m_Preset, 1);
		int arrivalReplayRevisionDelta = fixture.m_Operation.m_iRevision - arrivalRevision;
		bool arrivalOnce = !arrivalReplayChanged
			&& fixture.m_Operation.m_iRevision == arrivalRevision
			&& fixture.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
			&& fixture.m_Mission.m_sLastRuntimeEventKey == arrivalEvent
			&& CountElementsWithDisposition(fixture, HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ARRIVED) == HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT;
		fixture.m_Mission.m_bConvoyArrivalOutcomeApplied = true;
		bool settled = fixture.m_Service.TickAfterOutcomes(fixture.m_State);
		string settlementId = fixture.m_Operation.m_sSettlementId;
		int revisionAfterSettlement = fixture.m_Operation.m_iRevision;
		bool replay = fixture.m_Service.TickAfterOutcomes(fixture.m_State);
		bool settlementExact = settled
			&& !replay
			&& !settlementId.IsEmpty()
			&& fixture.m_Mission.m_sSettlementId == settlementId
			&& fixture.m_Operation.m_iRevision == revisionAfterSettlement
			&& fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& fixture.m_Operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
			&& fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			&& fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
			&& fixture.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED
			&& !fixture.m_Batch.m_bStrategicProjectionHeld
			&& AllNonCasualtySlotsRetired(fixture.m_Batch);

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_OperationRecordState restoredOperation;
		HST_ActiveMissionState restoredMission;
		if (restored)
		{
			restoredOperation = restored.FindOperation(fixture.m_Operation.m_sOperationId);
			restoredMission = restored.FindActiveMission(fixture.m_Mission.m_sInstanceId);
		}
		bool persistenceExact = restoredOperation && restoredMission
			&& restoredOperation.m_sSettlementId == settlementId
			&& restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& restoredOperation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
			&& restoredMission.m_sSettlementId == settlementId;

		string arrivalBoundaryEvidence;
		bool arrivalBoundaryExact = ProveArrivalOutcomeRestoreBoundary(arrivalBoundaryEvidence);
		string virtualArrivalEvidence;
		bool virtualArrivalExact = ProveVirtualRouteArrival(virtualArrivalEvidence);
		report.m_bSettlementExact = arrivalExact && arrivalOnce && settlementExact && persistenceExact
			&& arrivalBoundaryExact && virtualArrivalExact;
		report.m_sSettlementEvidence = string.Format(
			"cursor-only far sample changed/rejected/distance %1/%2/%3m | endpoint samples %4/%5 | route %6/%7m",
			farSampleChanged,
			cursorOnlyRejected,
			Math.Round(farDistance),
			firstEndpointSample,
			arrived,
			Math.Round(fixture.m_Operation.m_fRouteProgressMeters),
			Math.Round(fixture.m_Operation.m_fRouteTotalDistanceMeters));
		report.m_sSettlementEvidence = report.m_sSettlementEvidence + string.Format(
			" | arrived elements %1 | arrival replay changed/revision %2/%3 | settlement %4",
			CountElementsWithDisposition(fixture, HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ARRIVED),
			arrivalReplayChanged,
			arrivalReplayRevisionDelta,
			settlementId);
		report.m_sSettlementEvidence = report.m_sSettlementEvidence + string.Format(
			" | terminal/duty/materialization %1/%2/%3 | settlement replay %4 | restore %5",
			fixture.m_Operation.m_eTerminalResult,
			fixture.m_Operation.m_eDutyState,
			fixture.m_Operation.m_eMaterializationState,
			replay,
			persistenceExact);
		report.m_sSettlementEvidence = report.m_sSettlementEvidence + " | " + arrivalBoundaryEvidence;
		report.m_sSettlementEvidence = report.m_sSettlementEvidence + " | " + virtualArrivalEvidence;
	}

	protected bool ProveVirtualRouteArrival(out string evidence)
	{
		evidence = "virtual arrival fixture did not run";
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("virtual_arrival");
		if (!Ready(fixture))
		{
			evidence = "virtual arrival: " + BuildFixtureFailure(fixture);
			return false;
		}

		fixture.m_Operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		fixture.m_Operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		fixture.m_Operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		fixture.m_Operation.m_fRouteProgressMeters = Math.Max(0.0,
			fixture.m_Operation.m_fRouteTotalDistanceMeters - fixture.m_Operation.m_fStrategicSpeedMetersPerSecond);
		fixture.m_Operation.m_iStrategicLastUpdateSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Mission.m_sRuntimePhase = HST_MissionConvoyOperationService.CONVOY_MOVING;
		fixture.m_State.m_iElapsedSeconds++;
		bool arrivalChanged = fixture.m_Service.TickBeforePhysical(fixture.m_State, fixture.m_Preset, 1);
		bool exact = arrivalChanged
			&& fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& fixture.m_Operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& fixture.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
			&& fixture.m_Mission.m_sLastRuntimeEventKey == HST_MissionConvoyOperationService.CONVOY_FAIL_EVENT
			&& fixture.m_Mission.m_sRuntimeFailureReason.Contains("virtual route cursor reached destination")
			&& Math.AbsFloat(fixture.m_Operation.m_fRouteProgressMeters - fixture.m_Operation.m_fRouteTotalDistanceMeters) < 0.1
			&& CountElementsWithDisposition(fixture, HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ARRIVED)
				== HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT;
		evidence = string.Format(
			"virtual arrival changed/exact/progress/elements %1/%2/%3-%4/%5",
			arrivalChanged,
			exact,
			Math.Round(fixture.m_Operation.m_fRouteProgressMeters),
			Math.Round(fixture.m_Operation.m_fRouteTotalDistanceMeters),
			CountElementsWithDisposition(fixture, HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ARRIVED));
		return exact;
	}

	protected void ProveRestore(HST_MissionConvoyOperationProofReport report)
	{
		HST_MissionConvoyOperationProofFixture openFixture = BuildAdmittedFixture("restore_open");
		HST_MissionConvoyOperationProofFixture settledFixture = BuildAdmittedFixture("restore_settled");
		if (!Ready(openFixture) || !Ready(settledFixture))
		{
			report.m_sRestoreEvidence = BuildPairFailure(openFixture, settledFixture);
			return;
		}

		openFixture.m_State.m_iElapsedSeconds += PROOF_VIRTUAL_ADVANCE_SECONDS;
		openFixture.m_Service.TickBeforePhysical(openFixture.m_State, openFixture.m_Preset, PROOF_VIRTUAL_ADVANCE_SECONDS);
		float openProgress = openFixture.m_Operation.m_fRouteProgressMeters;
		int openLiving = CountLivingMemberSlots(openFixture.m_Batch);
		HST_CampaignSaveData openSave = new HST_CampaignSaveData();
		openSave.Capture(openFixture.m_State);
		HST_CampaignState restoredOpen = openSave.Restore();
		HST_ActiveMissionState restoredOpenMission;
		HST_OperationRecordState restoredOpenOperation;
		HST_ForceManifestState restoredOpenManifest;
		HST_ForceSpawnResultState restoredOpenBatch;
		bool openNormalized;
		if (restoredOpen)
		{
			restoredOpen.m_bRestoredFromPersistence = true;
			restoredOpen.m_iPersistenceRestoreSequence++;
			restoredOpenMission = restoredOpen.FindActiveMission(openFixture.m_Mission.m_sInstanceId);
			if (restoredOpenMission)
			{
				restoredOpenOperation = restoredOpen.FindOperation(restoredOpenMission.m_sOperationId);
				restoredOpenManifest = restoredOpen.FindForceManifest(restoredOpenMission.m_sManifestId);
				restoredOpenBatch = restoredOpen.FindForceSpawnResult(restoredOpenMission.m_sSpawnResultId);
			}
			HST_MissionConvoyOperationService restoredOpenService = new HST_MissionConvoyOperationService();
			restoredOpenService.SetRuntimeServices(new HST_PhysicalWarService());
			openNormalized = restoredOpenService.TickBeforePhysical(restoredOpen, openFixture.m_Preset, 0);
		}
		bool openRecordsPresent = restoredOpen && restoredOpenMission && restoredOpenOperation && restoredOpenManifest && restoredOpenBatch;
		bool openRowsExact;
		bool openAuthorityExact;
		bool openRosterExact;
		if (openRecordsPresent)
		{
			openRowsExact = restoredOpen.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION
				&& CountOperations(restoredOpen, restoredOpenOperation.m_sOperationId) == 1
				&& CountManifests(restoredOpen, restoredOpenManifest.m_sManifestId) == 1
				&& CountBatches(restoredOpen, restoredOpenBatch.m_sResultId) == 1
				&& CountElements(restoredOpen, restoredOpenOperation.m_sOperationId) == HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT;
			openAuthorityExact = restoredOpenOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				&& restoredOpenOperation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				&& restoredOpenOperation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
				&& restoredOpenOperation.m_sCurrentRouteId == "mission_convoy_proof_route_restore_open"
				&& Math.AbsFloat(restoredOpenOperation.m_fRouteProgressMeters - openProgress) < 0.1;
			openRosterExact = restoredOpenManifest.m_sManifestHash == openFixture.m_Manifest.m_sManifestHash
				&& CountLivingMemberSlots(restoredOpenBatch) == openLiving
				&& ValidateConcreteMemberSlots(restoredOpenManifest);
		}
		bool openExact = openRecordsPresent && openRowsExact && openAuthorityExact && openRosterExact;

		settledFixture.m_Operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		settledFixture.m_Operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		settledFixture.m_Operation.m_fRouteProgressMeters = Math.Max(0.0, settledFixture.m_Operation.m_fRouteTotalDistanceMeters - 45.0);
		settledFixture.m_Operation.m_iStrategicLastUpdateSecond = settledFixture.m_State.m_iElapsedSeconds;
		settledFixture.m_Mission.m_sRuntimePhase = HST_MissionConvoyOperationService.CONVOY_MOVING;
		settledFixture.m_State.m_iElapsedSeconds += 5;
		settledFixture.m_Service.TickBeforePhysical(settledFixture.m_State, settledFixture.m_Preset, 5);
		settledFixture.m_Mission.m_bConvoyArrivalOutcomeApplied = true;
		bool settlementApplied = settledFixture.m_Service.TickAfterOutcomes(settledFixture.m_State);
		string settlementId = settledFixture.m_Operation.m_sSettlementId;
		HST_CampaignSaveData settledSave = new HST_CampaignSaveData();
		settledSave.Capture(settledFixture.m_State);
		HST_CampaignState restoredSettled = settledSave.Restore();
		HST_ActiveMissionState restoredSettledMission;
		HST_OperationRecordState restoredSettledOperation;
		HST_ForceSpawnResultState restoredSettledBatch;
		bool settledBeforeChanged;
		bool settledPhysicalChanged;
		bool settledOutcomeChanged;
		int settledRevisionBefore;
		if (restoredSettled)
		{
			restoredSettled.m_bRestoredFromPersistence = true;
			restoredSettled.m_iPersistenceRestoreSequence++;
			restoredSettledMission = restoredSettled.FindActiveMission(settledFixture.m_Mission.m_sInstanceId);
			if (restoredSettledMission)
			{
				restoredSettledOperation = restoredSettled.FindOperation(restoredSettledMission.m_sOperationId);
				restoredSettledBatch = restoredSettled.FindForceSpawnResult(restoredSettledMission.m_sSpawnResultId);
			}
			if (restoredSettledOperation)
				settledRevisionBefore = restoredSettledOperation.m_iRevision;
			HST_MissionConvoyOperationService restoredSettledService = new HST_MissionConvoyOperationService();
			restoredSettledService.SetRuntimeServices(new HST_PhysicalWarService());
			settledBeforeChanged = restoredSettledService.TickBeforePhysical(restoredSettled, settledFixture.m_Preset, 0);
			settledPhysicalChanged = restoredSettledService.TickAfterPhysical(restoredSettled);
			settledOutcomeChanged = restoredSettledService.TickAfterOutcomes(restoredSettled);
		}
		bool settledRecordsPresent = restoredSettledMission && restoredSettledOperation && restoredSettledBatch;
		bool settledReceiptExact;
		bool settledBatchExact;
		if (settledRecordsPresent)
		{
			settledReceiptExact = restoredSettledOperation.m_sSettlementId == settlementId
				&& restoredSettledMission.m_sSettlementId == settlementId
				&& restoredSettledOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
				&& restoredSettledOperation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
				&& restoredSettledOperation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
			settledBatchExact = restoredSettledBatch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED
				&& !restoredSettledBatch.m_bStrategicProjectionHeld;
		}
		bool settledQuiescent = settledRecordsPresent
			&& restoredSettledOperation.m_iRevision == settledRevisionBefore
			&& !settledBeforeChanged && !settledPhysicalChanged && !settledOutcomeChanged;
		bool settledExact = settlementApplied && settledRecordsPresent && settledReceiptExact && settledBatchExact && settledQuiescent;

		string recoveryEvidence;
		bool recoveryExact = ProveRecoveryRestore(recoveryEvidence);
		string restoreOrderingEvidence;
		bool restoreOrderingExact = ProveRestoreOrderingBoundary(restoreOrderingEvidence);
		string terminalCleanupEvidence;
		bool terminalCleanupExact = ProveTerminalCleanupRestoreBoundary(terminalCleanupEvidence);
		report.m_bRestoreExact = openNormalized && openExact && settledExact && recoveryExact
			&& restoreOrderingExact && terminalCleanupExact;
		int restoredOpenSchema = -1;
		int restoredOpenOperations = -1;
		int restoredOpenManifests = -1;
		int restoredOpenBatches = -1;
		int restoredOpenElements = -1;
		int restoredOpenProgress = -1;
		int restoredOpenLiving = -1;
		int restoredSettledRevision = -1;
		if (restoredOpen)
			restoredOpenSchema = restoredOpen.m_iSchemaVersion;
		if (restoredOpenOperation)
		{
			restoredOpenOperations = CountOperations(restoredOpen, restoredOpenOperation.m_sOperationId);
			restoredOpenElements = CountElements(restoredOpen, restoredOpenOperation.m_sOperationId);
			restoredOpenProgress = Math.Round(restoredOpenOperation.m_fRouteProgressMeters);
		}
		if (restoredOpenManifest)
			restoredOpenManifests = CountManifests(restoredOpen, restoredOpenManifest.m_sManifestId);
		if (restoredOpenBatch)
		{
			restoredOpenBatches = CountBatches(restoredOpen, restoredOpenBatch.m_sResultId);
			restoredOpenLiving = CountLivingMemberSlots(restoredOpenBatch);
		}
		if (restoredSettledOperation)
			restoredSettledRevision = restoredSettledOperation.m_iRevision;
		report.m_sRestoreEvidence = string.Format(
			"open normalized/schema/rows %1/%2/%3-%4-%5-%6 | progress/living %7/%8",
			openNormalized,
			restoredOpenSchema,
			restoredOpenOperations,
			restoredOpenManifests,
			restoredOpenBatches,
			restoredOpenElements,
			restoredOpenProgress,
			restoredOpenLiving);
		report.m_sRestoreEvidence = report.m_sRestoreEvidence + string.Format(
			" | settled applied/id %1/%2 | replay before/physical/outcome %3/%4/%5 | archive revision %6/%7",
			settlementApplied,
			settlementId,
			settledBeforeChanged,
			settledPhysicalChanged,
			settledOutcomeChanged,
			settledRevisionBefore,
			restoredSettledRevision);
		report.m_sRestoreEvidence = report.m_sRestoreEvidence + " | " + recoveryEvidence;
		report.m_sRestoreEvidence = report.m_sRestoreEvidence + " | " + restoreOrderingEvidence;
		report.m_sRestoreEvidence = report.m_sRestoreEvidence + " | " + terminalCleanupEvidence;
	}

	protected bool ProveArrivalOutcomeRestoreBoundary(out string evidence)
	{
		evidence = "arrival/outcome save boundary fixture did not run";
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("arrival_outcome_boundary");
		if (!Ready(fixture))
		{
			evidence = "arrival/outcome save boundary: " + BuildFixtureFailure(fixture);
			return false;
		}

		fixture.m_Operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		fixture.m_Operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		fixture.m_Operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		fixture.m_Operation.m_iMaterializationStateEnteredAtSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Operation.m_fRouteProgressMeters = fixture.m_Operation.m_fRouteTotalDistanceMeters;
		fixture.m_Operation.m_iStrategicLastUpdateSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Mission.m_sRuntimePhase = HST_MissionConvoyOperationService.CONVOY_MOVING;

		HST_ConvoyElementState carrier = fixture.m_State.FindConvoyElement(HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, 0));
		HST_MissionAssetState carrierAsset = fixture.m_State.FindMissionAsset(BuildVehicleAssetId(fixture.m_Mission, 0));
		HST_ActiveGroupState carrierGroup = fixture.m_State.FindActiveGroup(HST_MissionConvoyOperationService.BuildGroupId(fixture.m_Mission, 0));
		if (!carrier || !carrierAsset || !carrierGroup)
		{
			evidence = "arrival/outcome save boundary is missing its assigned carrier roots";
			return false;
		}

		vector endpoint = fixture.m_Operation.m_vRouteEndPosition;
		carrier.m_vCurrentPosition = endpoint;
		carrierAsset.m_vCurrentPosition = endpoint;
		carrierAsset.m_vLastKnownPosition = endpoint;
		carrierGroup.m_vPosition = endpoint;
		fixture.m_State.m_iElapsedSeconds++;
		bool firstArrivalSample = fixture.m_Service.TickAfterPhysical(fixture.m_State);
		fixture.m_State.m_iElapsedSeconds++;
		bool secondArrivalSample = fixture.m_Service.TickAfterPhysical(fixture.m_State);
		bool boundaryPrepared = firstArrivalSample && secondArrivalSample
			&& !fixture.m_Mission.m_bConvoyArrivalOutcomeApplied
			&& fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& fixture.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
			&& fixture.m_Mission.m_sLastRuntimeEventKey == HST_MissionConvoyOperationService.CONVOY_FAIL_EVENT
			&& CountElementsWithDisposition(fixture, HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ARRIVED) == HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT;

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_ActiveMissionState restoredMission;
		HST_OperationRecordState restoredOperation;
		HST_ForceSpawnResultState restoredBatch;
		bool restoreReconciled;
		bool pendingOutcomeChanged;
		bool pendingBoundaryOpen;
		bool settledAfterOutcome;
		int revisionBeforePending;
		if (restored)
		{
			restored.m_bRestoredFromPersistence = true;
			restored.m_iPersistenceRestoreSequence++;
			restoredMission = restored.FindActiveMission(fixture.m_Mission.m_sInstanceId);
			if (restoredMission)
			{
				restoredOperation = restored.FindOperation(restoredMission.m_sOperationId);
				restoredBatch = restored.FindForceSpawnResult(restoredMission.m_sSpawnResultId);
			}
			HST_MissionConvoyOperationService restoredService = new HST_MissionConvoyOperationService();
			restoredService.SetRuntimeServices(new HST_PhysicalWarService());
			if (restoredOperation)
				revisionBeforePending = restoredOperation.m_iRevision;
			restoreReconciled = restoredService.ReconcileAfterRestore(restored);
			pendingOutcomeChanged = restoredService.TickAfterOutcomes(restored);
			pendingBoundaryOpen = restoredMission && restoredOperation && restoredBatch
				&& !restoredMission.m_bConvoyArrivalOutcomeApplied
				&& restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				&& restoreReconciled
				&& restoredOperation.m_iRevision > revisionBeforePending
				&& restoredOperation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
				&& restoredOperation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
				&& restoredOperation.m_iLastNormalizedRestoreSequence == restored.m_iPersistenceRestoreSequence
				&& AreExactMissionProjectionFlagsNormalized(restored, restoredMission)
				&& restoredBatch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
				&& restoredBatch.m_bStrategicProjectionHeld
				&& !pendingOutcomeChanged;
			if (pendingBoundaryOpen)
			{
				restoredMission.m_bConvoyArrivalOutcomeApplied = true;
				settledAfterOutcome = restoredService.TickAfterOutcomes(restored)
					&& restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
					&& restoredOperation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
					&& restoredOperation.m_sSettlementId == HST_OperationService.BuildSettlementId(restoredOperation.m_sOperationId, "arrived")
					&& restoredMission.m_sSettlementId == restoredOperation.m_sSettlementId;
			}
		}

		evidence = string.Format(
			"arrival/outcome boundary prepared %1 | restore normalized virtual %2 and remained open %3 | pending replay changed %4 | settled after outcome %5",
			boundaryPrepared,
			restoreReconciled,
			pendingBoundaryOpen,
			pendingOutcomeChanged,
			settledAfterOutcome);
		return boundaryPrepared && restored && pendingBoundaryOpen && settledAfterOutcome;
	}

	protected bool ProveRestoreOrderingBoundary(out string evidence)
	{
		evidence = "restore ordering fixture did not run";
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("restore_ordering");
		if (!Ready(fixture))
		{
			evidence = "restore ordering: " + BuildFixtureFailure(fixture);
			return false;
		}

		fixture.m_State.m_iElapsedSeconds += 30;
		fixture.m_Service.TickBeforePhysical(fixture.m_State, fixture.m_Preset, 30);
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_ActiveMissionState restoredMission;
		HST_OperationRecordState restoredOperation;
		HST_ForceSpawnResultState restoredBatch;
		string batchBeforeQueue;
		string batchAfterQueue;
		string batchAfterConvoy;
		HST_ForceSpawnQueueMaintenanceResult queueResult;
		bool convoyReconciled;
		if (restored)
		{
			restored.m_bRestoredFromPersistence = true;
			restored.m_iPersistenceRestoreSequence++;
			restoredMission = restored.FindActiveMission(fixture.m_Mission.m_sInstanceId);
			if (restoredMission)
			{
				restoredOperation = restored.FindOperation(restoredMission.m_sOperationId);
				restoredBatch = restored.FindForceSpawnResult(restoredMission.m_sSpawnResultId);
			}
			batchBeforeQueue = BuildBatchAuthoritySnapshot(restoredBatch);
			HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
			queueResult = queue.ReconcileCampaignAfterRestore(restored);
			batchAfterQueue = BuildBatchAuthoritySnapshot(restoredBatch);
			HST_MissionConvoyOperationService restoredService = new HST_MissionConvoyOperationService();
			restoredService.SetRuntimeServices(new HST_PhysicalWarService());
			convoyReconciled = restoredService.ReconcileAfterRestore(restored);
			batchAfterConvoy = BuildBatchAuthoritySnapshot(restoredBatch);
		}

		bool queueSkippedExactBatch = restoredBatch && batchBeforeQueue == batchAfterQueue;
		bool convoyPreservedBatch = restoredBatch && batchAfterQueue == batchAfterConvoy;
		bool authorityOpen = restoredMission && restoredOperation && restoredBatch
			&& restoredMission.m_iOperationContractVersion == HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION
			&& restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& restoredBatch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
			&& restoredBatch.m_bStrategicProjectionHeld && !restoredBatch.m_bCancelRequested
			&& CountLivingMemberSlots(restoredBatch) == CountLivingMemberSlots(fixture.m_Batch);
		int queueReconciledCount = -1;
		if (queueResult)
			queueReconciledCount = queueResult.m_iReconciledBatchCount;
		evidence = string.Format(
			"restore order queue->convoy exact batch unchanged %1/%2 | queue reconciled rows %3 | convoy normalized %4 | open authority %5",
			queueSkippedExactBatch,
			convoyPreservedBatch,
			queueReconciledCount,
			convoyReconciled,
			authorityOpen);
		return restored && queueResult && queueSkippedExactBatch && convoyPreservedBatch && authorityOpen;
	}

	protected bool ProveTerminalCleanupRestoreBoundary(out string evidence)
	{
		evidence = "terminal cleanup/restore fixture did not run";
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("terminal_cleanup_restore");
		if (!Ready(fixture))
		{
			evidence = "terminal cleanup/restore: " + BuildFixtureFailure(fixture);
			return false;
		}

		fixture.m_Mission.m_sRuntimePhase = HST_MissionConvoyOperationService.CONVOY_FAILED;
		fixture.m_Mission.m_sRuntimeFailureReason = "mission convoy proof terminal cleanup";
		fixture.m_Mission.m_sLastRuntimeEventKey = HST_MissionConvoyOperationService.CONVOY_FAIL_EVENT;
		bool settled = fixture.m_Service.TickAfterOutcomes(fixture.m_State);
		fixture.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
		bool settledGroupsArchived = AreExactMissionGroupsNonOperational(fixture.m_State, fixture.m_Mission);
		string membersBefore = BuildBatchAuthoritySnapshot(fixture.m_Batch);
		string receiptBefore = BuildSettlementReceiptSnapshot(fixture.m_Mission, fixture.m_Operation);
		int groupsBeforeCount = CountMissionGroups(fixture.m_State, fixture.m_Mission.m_sInstanceId);
		int assetsBeforeCount = CountMissionAssets(fixture.m_State, fixture.m_Mission.m_sInstanceId);
		bool staleProjectionSeeded = SeedSettledStaleProjectionFlags(fixture);

		HST_MissionRuntimeService missionRuntime = new HST_MissionRuntimeService();
		bool missionCleanupChanged = missionRuntime.Tick(fixture.m_State, fixture.m_Preset, new HST_MissionObjectiveService(), 1);
		bool physicalCleanupChanged = fixture.m_PhysicalWar.ReconcileInactiveMissionConvoyRuntime(fixture.m_State);
		string groupsAfterCleanup = BuildGroupAuthoritySnapshot(fixture.m_State, fixture.m_Mission);
		string assetsAfterCleanup = BuildAssetAuthoritySnapshot(fixture.m_State, fixture.m_Mission);
		bool secondPhysicalCleanupChanged = fixture.m_PhysicalWar.ReconcileInactiveMissionConvoyRuntime(fixture.m_State);
		bool cleanupPreserved = settledGroupsArchived && staleProjectionSeeded && physicalCleanupChanged && !secondPhysicalCleanupChanged
			&& AreExactMissionProjectionFlagsNormalized(fixture.m_State, fixture.m_Mission)
			&& groupsAfterCleanup == BuildGroupAuthoritySnapshot(fixture.m_State, fixture.m_Mission)
			&& membersBefore == BuildBatchAuthoritySnapshot(fixture.m_Batch)
			&& assetsAfterCleanup == BuildAssetAuthoritySnapshot(fixture.m_State, fixture.m_Mission)
			&& receiptBefore == BuildSettlementReceiptSnapshot(fixture.m_Mission, fixture.m_Operation)
			&& groupsBeforeCount == CountMissionGroups(fixture.m_State, fixture.m_Mission.m_sInstanceId)
			&& assetsBeforeCount == CountMissionAssets(fixture.m_State, fixture.m_Mission.m_sInstanceId);

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_ActiveMissionState restoredMission;
		HST_OperationRecordState restoredOperation;
		HST_ForceSpawnResultState restoredBatch;
		if (restored)
		{
			restoredMission = restored.FindActiveMission(fixture.m_Mission.m_sInstanceId);
			if (restoredMission)
			{
				restoredOperation = restored.FindOperation(restoredMission.m_sOperationId);
				restoredBatch = restored.FindForceSpawnResult(restoredMission.m_sSpawnResultId);
			}
		}
		bool restorePreserved = restoredMission && restoredOperation && restoredBatch
			&& AreExactMissionGroupsNonOperational(restored, restoredMission)
			&& groupsAfterCleanup == BuildGroupAuthoritySnapshot(restored, restoredMission)
			&& membersBefore == BuildBatchAuthoritySnapshot(restoredBatch)
			&& assetsAfterCleanup == BuildAssetAuthoritySnapshot(restored, restoredMission)
			&& receiptBefore == BuildSettlementReceiptSnapshot(restoredMission, restoredOperation)
			&& groupsBeforeCount == CountMissionGroups(restored, restoredMission.m_sInstanceId)
			&& assetsBeforeCount == CountMissionAssets(restored, restoredMission.m_sInstanceId);

		evidence = string.Format(
			"terminal cleanup settled/runtime/stale-first/idempotent-second %1/%2/%3/%4 | normalized durable authority unchanged %5 | restore unchanged %6 | rows groups/assets %7/%8",
			settled,
			missionCleanupChanged,
			physicalCleanupChanged,
			!secondPhysicalCleanupChanged,
			cleanupPreserved,
			restorePreserved,
			groupsBeforeCount,
			assetsBeforeCount);
		return settled && missionCleanupChanged && cleanupPreserved && restorePreserved;
	}

	protected bool ProveRecoveryRestore(out string evidence)
	{
		evidence = "recovery fixture did not run";
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("recovery_restore", true, "convoy_money");
		if (!Ready(fixture))
		{
			evidence = "recovery restore: " + BuildFixtureFailure(fixture);
			return false;
		}

		bool zeroRosterPrepared = EliminateExactCrewAuthority(fixture);
		fixture.m_Operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		fixture.m_Operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		fixture.m_Operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		fixture.m_Operation.m_iMaterializationStateEnteredAtSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Mission.m_sRuntimePhase = HST_MissionConvoyOperationService.CONVOY_MOVING;
		for (int physicalIndex = 0; physicalIndex < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; physicalIndex++)
		{
			HST_ConvoyElementState physicalElement = fixture.m_State.FindConvoyElement(HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, physicalIndex));
			HST_MissionAssetState physicalAsset = fixture.m_State.FindMissionAsset(BuildVehicleAssetId(fixture.m_Mission, physicalIndex));
			HST_ActiveGroupState physicalGroup = fixture.m_State.FindActiveGroup(HST_MissionConvoyOperationService.BuildGroupId(fixture.m_Mission, physicalIndex));
			if (physicalElement)
				physicalElement.m_bPhysicalized = true;
			if (physicalAsset)
				physicalAsset.m_bSpawned = true;
			if (physicalGroup)
				physicalGroup.m_bSpawnedEntity = true;
		}

		fixture.m_State.m_iElapsedSeconds++;
		bool recoveryEntered = fixture.m_Service.TickBeforePhysical(fixture.m_State, fixture.m_Preset, 1);
		fixture.m_Mission.m_bConvoyCrewEliminatedOutcomeApplied = true;
		fixture.m_Service.TickAfterOutcomes(fixture.m_State);
		bool heldBeforeSave = recoveryEntered
			&& fixture.m_Mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			&& fixture.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_ELIMINATED
			&& fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			&& fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			&& CountElementsWithDisposition(fixture, HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED) == HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT;

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_CampaignState restored = saveData.Restore();
		HST_ActiveMissionState restoredMission;
		HST_OperationRecordState restoredOperation;
		HST_ForceSpawnResultState restoredBatch;
		HST_MissionAssetState restoredCargo;
		bool normalized;
		if (restored)
		{
			restored.m_bRestoredFromPersistence = true;
			restored.m_iPersistenceRestoreSequence++;
			restored.m_iElapsedSeconds++;
			restoredMission = restored.FindActiveMission(fixture.m_Mission.m_sInstanceId);
			if (restoredMission)
			{
				restoredOperation = restored.FindOperation(restoredMission.m_sOperationId);
				restoredBatch = restored.FindForceSpawnResult(restoredMission.m_sSpawnResultId);
			}
			restoredCargo = restored.FindMissionAsset(fixture.m_Cargo.m_sAssetId);
			HST_MissionConvoyOperationService restoredService = new HST_MissionConvoyOperationService();
			restoredService.SetRuntimeServices(new HST_PhysicalWarService());
			normalized = restoredService.ReconcileAfterRestore(restored);
		}

		bool cargoRetained = restoredCargo && restoredMission
			&& restoredCargo.m_sOperationId == restoredMission.m_sOperationId
			&& !restoredCargo.m_sAssignedVehicleSlotId.IsEmpty()
			&& !restoredCargo.m_bDelivered && !restoredCargo.m_bDestroyed && !restoredCargo.m_bOutcomeApplied;
		bool virtualRecoveryExact = normalized && restoredMission && restoredOperation && restoredBatch
			&& restoredMission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			&& restoredMission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_ELIMINATED
			&& restoredMission.m_bConvoyCrewEliminatedOutcomeApplied
			&& restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& restoredOperation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			&& restoredOperation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& restoredOperation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& restoredOperation.m_iLastNormalizedRestoreSequence == restored.m_iPersistenceRestoreSequence
			&& restoredBatch.m_bStrategicProjectionHeld
			&& ValidateRestoredRecoveryElements(restored, restoredMission)
			&& cargoRetained;

		bool remainedOpen;
		bool recoverySettled;
		bool completionDeferred;
		bool completionReleased;
		if (virtualRecoveryExact)
		{
			HST_MissionConvoyOperationService outcomeService = new HST_MissionConvoyOperationService();
			outcomeService.SetRuntimeServices(new HST_PhysicalWarService());
			completionDeferred = outcomeService.ShouldDeferGenericMissionCompletion(restored, restoredMission);
			int revisionBeforePendingReplay = restoredOperation.m_iRevision;
			bool pendingReplayChanged = outcomeService.TickAfterOutcomes(restored);
			bool secondPendingReplayChanged = outcomeService.TickAfterOutcomes(restored);
			remainedOpen = !pendingReplayChanged && !secondPendingReplayChanged
				&& restoredOperation.m_iRevision == revisionBeforePendingReplay
				&& restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				&& restoredMission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
				&& outcomeService.ShouldDeferGenericMissionCompletion(restored, restoredMission);
			restoredCargo.m_bDelivered = true;
			restoredCargo.m_bOutcomeApplied = true;
			completionReleased = !outcomeService.ShouldDeferGenericMissionCompletion(restored, restoredMission);
			recoverySettled = outcomeService.TickAfterOutcomes(restored)
				&& restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
				&& restoredOperation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED
				&& !restoredOperation.m_sSettlementId.IsEmpty();
		}

		evidence = string.Format(
			"crewless recovery prepared/held %1/%2 | restore normalized/open cargo+vehicles %3/%4 | two coordinator-order replays open/deferred %5/%6 | completion released/settled %7/%8",
			zeroRosterPrepared,
			heldBeforeSave,
			normalized,
			virtualRecoveryExact,
			remainedOpen,
			completionDeferred,
			completionReleased,
			recoverySettled);
		return zeroRosterPrepared && heldBeforeSave && virtualRecoveryExact && remainedOpen
			&& completionDeferred && completionReleased && recoverySettled;
	}

	protected void ProveCorruptionRejection(HST_MissionConvoyOperationProofReport report)
	{
		string liveEvidence;
		string restoreHashEvidence;
		string restoreSeatEvidence;
		string restoreArrivalEvidence;
		string restoreForeignEvidence;
		string restoreMissingCargoEvidence;
		string restoreCaptiveSourceEvidence;
		string missionlessEvidence;
		string lifecycleEvidence;
		bool liveRejected = ProveLiveAuthorityCorruption(liveEvidence);
		bool restoreHashRejected = ProveRestoreManifestHashCorruption(restoreHashEvidence);
		bool restoreSeatRejected = ProveRestoreSeatTopologyCorruption(restoreSeatEvidence);
		bool restoreArrivalRejected = ProveRestoreArrivalReceiptCorruption(restoreArrivalEvidence);
		bool restoreForeignRejected = ProveRestoreForeignAuthorityCorruption(restoreForeignEvidence);
		bool restoreMissingCargoRejected = ProveRestoreMissingRequiredCargo(restoreMissingCargoEvidence);
		bool restoreCaptiveSourceRejected = ProveRestoreCaptiveSourceTypeCorruption(restoreCaptiveSourceEvidence);
		bool missionlessPreserved = ProveMissionlessDurableClaimantPreservation(missionlessEvidence);
		bool lifecycleRejected = ProveRestoreLifecycleGuards(lifecycleEvidence);
		report.m_bCorruptionRejected = liveRejected && restoreHashRejected && restoreSeatRejected
			&& restoreArrivalRejected && restoreForeignRejected && restoreMissingCargoRejected
			&& restoreCaptiveSourceRejected && missionlessPreserved && lifecycleRejected;
		report.m_sCorruptionEvidence = liveEvidence + " | " + restoreHashEvidence
			+ " | " + restoreSeatEvidence + " | " + restoreArrivalEvidence
			+ " | " + restoreForeignEvidence + " | " + restoreMissingCargoEvidence
			+ " | " + restoreCaptiveSourceEvidence + " | " + missionlessEvidence
			+ " | " + lifecycleEvidence;
	}

	protected bool ProveLiveAuthorityCorruption(out string evidence)
	{
		evidence = "live authority corruption fixtures did not run";
		HST_MissionConvoyOperationProofFixture duplicate = BuildAdmittedFixture("duplicate_corruption");
		HST_MissionConvoyOperationProofFixture hash = BuildAdmittedFixture("hash_corruption");
		HST_MissionConvoyOperationProofFixture missingBacklink = BuildAdmittedFixture("missing_backlink_corruption");
		if (!Ready(duplicate) || !Ready(hash) || !Ready(missingBacklink))
		{
			evidence = BuildPairFailure(duplicate, hash) + " | " + BuildFixtureFailure(missingBacklink);
			return false;
		}

		int duplicateOperationRevision = duplicate.m_Operation.m_iRevision;
		int duplicateBatchRevision = duplicate.m_Batch.m_iLifecycleRevision;
		HST_EForceSpawnBatchStatus duplicateBatchStatus = duplicate.m_Batch.m_eStatus;
		bool duplicateBatchHeld = duplicate.m_Batch.m_bStrategicProjectionHeld;
		int duplicateElementRevisionSum = SumElementRevisions(duplicate);
		int duplicateLiving = CountLivingMemberSlots(duplicate.m_Batch);
		string duplicateManifestHash = duplicate.m_Manifest.m_sManifestHash;
		HST_OperationRecordState duplicateClaimant = new HST_OperationRecordState();
		duplicateClaimant.m_sOperationId = duplicate.m_Operation.m_sOperationId;
		duplicateClaimant.m_sMissionInstanceId = duplicate.m_Mission.m_sInstanceId;
		duplicateClaimant.m_sTerminalReason = "distinct duplicate proof claimant";
		duplicateClaimant.m_iRevision = 520052;
		duplicate.m_State.m_aOperations.Insert(duplicateClaimant);
		duplicate.m_State.m_iElapsedSeconds++;
		bool duplicateChanged = duplicate.m_Service.TickBeforePhysical(duplicate.m_State, duplicate.m_Preset, 1);
		bool duplicateReplayChanged = duplicate.m_Service.TickBeforePhysical(duplicate.m_State, duplicate.m_Preset, 1);
		bool duplicateMissionRejected = duplicateChanged && !duplicateReplayChanged
			&& duplicate.m_Mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.QUARANTINED_CONTRACT_VERSION
			&& duplicate.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
			&& duplicate.m_Mission.m_sRuntimeFailureReason.Contains("ambiguous")
			&& duplicate.m_Mission.m_sConvoyOutcomeSummary.Contains("without mutating ambiguous")
			&& AreExactMissionGroupsNonOperational(duplicate.m_State, duplicate.m_Mission);
		bool duplicateEvidencePreserved = duplicate.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& duplicate.m_Operation.m_iRevision == duplicateOperationRevision
			&& duplicate.m_Batch.m_iLifecycleRevision == duplicateBatchRevision
			&& duplicate.m_Batch.m_eStatus == duplicateBatchStatus
			&& duplicate.m_Batch.m_bStrategicProjectionHeld == duplicateBatchHeld
			&& SumElementRevisions(duplicate) == duplicateElementRevisionSum
			&& CountLivingMemberSlots(duplicate.m_Batch) == duplicateLiving
			&& duplicate.m_Manifest.m_sManifestHash == duplicateManifestHash
			&& duplicateClaimant != duplicate.m_Operation
			&& duplicateClaimant.m_iRevision == 520052
			&& duplicateClaimant.m_sTerminalReason == "distinct duplicate proof claimant";
		bool duplicateRejected = duplicateMissionRejected && duplicateEvidencePreserved;

		hash.m_Manifest.m_sPolicyId = hash.m_Manifest.m_sPolicyId + "_tampered";
		hash.m_State.m_iElapsedSeconds++;
		bool hashChanged = hash.m_Service.TickBeforePhysical(hash.m_State, hash.m_Preset, 1);
		bool hashRejected = hashChanged
			&& hash.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
			&& hash.m_Mission.m_sRuntimeFailureReason.Contains("manifest hash changed")
			&& hash.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;

		int missingOperationRevision = missingBacklink.m_Operation.m_iRevision;
		int missingBatchRevision = missingBacklink.m_Batch.m_iLifecycleRevision;
		int missingElementRevisionSum = SumElementRevisions(missingBacklink);
		string removedOperationBacklink = missingBacklink.m_Mission.m_sOperationId;
		missingBacklink.m_Mission.m_sOperationId = "";
		missingBacklink.m_State.m_iElapsedSeconds++;
		bool missingChanged = missingBacklink.m_Service.TickBeforePhysical(missingBacklink.m_State, missingBacklink.m_Preset, 1);
		bool missingReplayChanged = missingBacklink.m_Service.TickBeforePhysical(missingBacklink.m_State, missingBacklink.m_Preset, 1);
		bool missingRejected = missingChanged && !missingReplayChanged
			&& missingBacklink.m_Mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.QUARANTINED_CONTRACT_VERSION
			&& missingBacklink.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
			&& missingBacklink.m_Mission.m_sRuntimeFailureReason.Contains("canonical mission authority links conflict")
			&& missingBacklink.m_Mission.m_sConvoyOutcomeSummary.Contains("without mutating ambiguous")
			&& missingBacklink.m_Mission.m_sOperationId.IsEmpty()
			&& missingBacklink.m_State.FindOperation(removedOperationBacklink) == missingBacklink.m_Operation
			&& missingBacklink.m_Operation.m_iRevision == missingOperationRevision
			&& missingBacklink.m_Batch.m_iLifecycleRevision == missingBatchRevision
			&& missingBacklink.m_Batch.m_bStrategicProjectionHeld
			&& SumElementRevisions(missingBacklink) == missingElementRevisionSum
			&& AreExactMissionGroupsNonOperational(missingBacklink.m_State, missingBacklink.m_Mission);

		evidence = string.Format(
			"live duplicate/hash/missing-backlink rejected %1/%2/%3 | duplicate claimant preserved %4 | replay %5",
			duplicateRejected,
			hashRejected,
			missingRejected,
			duplicateEvidencePreserved,
			missingReplayChanged);
		return duplicateRejected && hashRejected && missingRejected;
	}

	protected bool ProveRestoreManifestHashCorruption(out string evidence)
	{
		evidence = "restore manifest-hash corruption fixture did not run";
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("restore_hash_corruption");
		if (!Ready(fixture))
		{
			evidence = BuildFixtureFailure(fixture);
			return false;
		}
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_ForceManifestState savedManifest = FindSavedManifest(saveData, fixture.m_Manifest.m_sManifestId);
		if (!savedManifest)
			return false;
		savedManifest.m_sPolicyId = savedManifest.m_sPolicyId + "_tampered";

		HST_CampaignState restored = saveData.Restore();
		HST_ActiveMissionState restoredMission;
		HST_OperationRecordState restoredOperation;
		HST_ForceManifestState restoredManifest;
		HST_ForceSpawnResultState restoredBatch;
		if (restored)
		{
			restoredMission = restored.FindActiveMission(fixture.m_Mission.m_sInstanceId);
			restoredOperation = restored.FindOperation(fixture.m_Operation.m_sOperationId);
			restoredManifest = restored.FindForceManifest(fixture.m_Manifest.m_sManifestId);
			restoredBatch = restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
		}
		bool exact;
		if (restoredMission && restoredOperation && restoredManifest && restoredBatch)
		{
			bool missionQuarantined = restoredMission.m_iOperationContractVersion == HST_MissionConvoyOperationService.QUARANTINED_CONTRACT_VERSION
				&& restoredMission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
				&& restoredMission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
				&& restoredMission.m_sRuntimeFailureReason.Contains("frozen manifest hash conflicts")
				&& AreExactMissionGroupsNonOperational(restored, restoredMission);
			bool evidencePreserved = restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				&& restoredOperation.m_iRevision == fixture.m_Operation.m_iRevision
				&& restoredManifest.m_sPolicyId == savedManifest.m_sPolicyId
				&& restoredManifest.m_sManifestHash == fixture.m_Manifest.m_sManifestHash
				&& restoredBatch.m_bStrategicProjectionHeld && !restoredBatch.m_bCancelRequested
				&& restoredBatch.m_iLifecycleRevision == fixture.m_Batch.m_iLifecycleRevision
				&& SumElementRevisionsForOperation(restored, restoredOperation.m_sOperationId) == SumElementRevisions(fixture);
			exact = missionQuarantined && evidencePreserved;
		}
		string reason;
		if (restoredMission)
			reason = restoredMission.m_sRuntimeFailureReason;
		evidence = string.Format("restore manifest hash rejected %1 reason '%2'", exact, reason);
		return exact;
	}

	protected bool ProveRestoreSeatTopologyCorruption(out string evidence)
	{
		evidence = "restore seat-topology corruption fixture did not run";
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("restore_seat_corruption");
		if (!Ready(fixture))
		{
			evidence = BuildFixtureFailure(fixture);
			return false;
		}
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_ForceManifestState savedManifest = FindSavedManifest(saveData, fixture.m_Manifest.m_sManifestId);
		HST_ForceSpawnResultState savedBatch = FindSavedBatch(saveData, fixture.m_Batch.m_sResultId);
		if (!savedManifest || !savedBatch || savedManifest.m_aMembers.Count() < 2)
			return false;

		HST_ForceManifestMemberState firstSeat = savedManifest.m_aMembers[0];
		HST_ForceManifestMemberState duplicateSeat = savedManifest.m_aMembers[1];
		if (!firstSeat || !duplicateSeat)
			return false;
		duplicateSeat.m_iSeatIndex = firstSeat.m_iSeatIndex;
		duplicateSeat.m_sSeatRole = firstSeat.m_sSeatRole;
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		savedManifest.m_sManifestHash = integrity.BuildManifestHash(savedManifest);
		savedBatch.m_sManifestHash = savedManifest.m_sManifestHash;

		HST_CampaignState restored = saveData.Restore();
		HST_ActiveMissionState restoredMission;
		HST_OperationRecordState restoredOperation;
		HST_ForceManifestState restoredManifest;
		HST_ForceSpawnResultState restoredBatch;
		if (restored)
		{
			restoredMission = restored.FindActiveMission(fixture.m_Mission.m_sInstanceId);
			restoredOperation = restored.FindOperation(fixture.m_Operation.m_sOperationId);
			restoredManifest = restored.FindForceManifest(fixture.m_Manifest.m_sManifestId);
			restoredBatch = restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
		}
		bool exact;
		if (restoredMission && restoredOperation && restoredManifest && restoredBatch)
		{
			bool missionQuarantined = restoredMission.m_iOperationContractVersion == HST_MissionConvoyOperationService.QUARANTINED_CONTRACT_VERSION
				&& restoredMission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
				&& restoredMission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
				&& restoredMission.m_sRuntimeFailureReason.Contains("seat")
				&& AreExactMissionGroupsNonOperational(restored, restoredMission);
			bool evidencePreserved = restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				&& restoredOperation.m_iRevision == fixture.m_Operation.m_iRevision
				&& restoredManifest.m_sManifestHash == savedManifest.m_sManifestHash
				&& restoredBatch.m_sManifestHash == savedManifest.m_sManifestHash
				&& restoredBatch.m_bStrategicProjectionHeld && !restoredBatch.m_bCancelRequested
				&& restoredBatch.m_iLifecycleRevision == fixture.m_Batch.m_iLifecycleRevision
				&& SumElementRevisionsForOperation(restored, restoredOperation.m_sOperationId) == SumElementRevisions(fixture);
			exact = missionQuarantined && evidencePreserved;
		}
		string reason;
		if (restoredMission)
			reason = restoredMission.m_sRuntimeFailureReason;
		evidence = string.Format("restore duplicate seat rejected %1 reason '%2'", exact, reason);
		return exact;
	}

	protected bool ProveRestoreArrivalReceiptCorruption(out string evidence)
	{
		evidence = "restore arrival-receipt corruption fixture did not run";
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("restore_arrival_corruption");
		if (!Ready(fixture))
		{
			evidence = BuildFixtureFailure(fixture);
			return false;
		}
		fixture.m_Operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		fixture.m_Operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		fixture.m_Operation.m_fRouteProgressMeters = Math.Max(
			0.0,
			fixture.m_Operation.m_fRouteTotalDistanceMeters - HST_MissionConvoyOperationService.EXACT_ARRIVAL_RADIUS_METERS + 5.0);
		fixture.m_Operation.m_iStrategicLastUpdateSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_Mission.m_sRuntimePhase = HST_MissionConvoyOperationService.CONVOY_MOVING;
		fixture.m_State.m_iElapsedSeconds += 5;
		fixture.m_Service.TickBeforePhysical(fixture.m_State, fixture.m_Preset, 5);
		fixture.m_Mission.m_bConvoyArrivalOutcomeApplied = true;
		bool settled = fixture.m_Service.TickAfterOutcomes(fixture.m_State)
			&& fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& fixture.m_Operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED;

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_ActiveMissionState savedMission = FindSavedMission(saveData, fixture.m_Mission.m_sInstanceId);
		if (!savedMission)
			return false;
		savedMission.m_sLastRuntimeEventKey = HST_MissionConvoyOperationService.CONVOY_COMPLETE_EVENT;

		HST_CampaignState restored = saveData.Restore();
		HST_ActiveMissionState restoredMission;
		HST_OperationRecordState restoredOperation;
		HST_ForceSpawnResultState restoredBatch;
		if (restored)
		{
			restoredMission = restored.FindActiveMission(fixture.m_Mission.m_sInstanceId);
			restoredOperation = restored.FindOperation(fixture.m_Operation.m_sOperationId);
			restoredBatch = restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
		}
		bool exact;
		if (settled && restoredMission && restoredOperation && restoredBatch)
		{
			bool missionQuarantined = restoredMission.m_iOperationContractVersion == HST_MissionConvoyOperationService.QUARANTINED_CONTRACT_VERSION
				&& restoredMission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
				&& restoredMission.m_sRuntimeFailureReason.Contains("arrival receipt or outcome evidence conflicts")
				&& AreExactMissionGroupsNonOperational(restored, restoredMission);
			bool evidencePreserved = restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
				&& restoredOperation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED
				&& restoredOperation.m_iRevision == fixture.m_Operation.m_iRevision
				&& restoredBatch.m_iLifecycleRevision == fixture.m_Batch.m_iLifecycleRevision
				&& SumElementRevisionsForOperation(restored, restoredOperation.m_sOperationId) == SumElementRevisions(fixture);
			exact = missionQuarantined && evidencePreserved;
		}
		string reason;
		if (restoredMission)
			reason = restoredMission.m_sRuntimeFailureReason;
		evidence = string.Format("forged arrival receipt rejected %1 reason '%2'", exact, reason);
		return exact;
	}

	protected bool ProveRestoreForeignAuthorityCorruption(out string evidence)
	{
		evidence = "restore foreign-authority corruption fixture did not run";
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("restore_foreign_corruption");
		if (!Ready(fixture))
		{
			evidence = BuildFixtureFailure(fixture);
			return false;
		}
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_OperationRecordState foreignOperation = new HST_OperationRecordState();
		foreignOperation.m_sOperationId = fixture.m_Operation.m_sOperationId + "_foreign";
		foreignOperation.m_sMissionInstanceId = fixture.m_Mission.m_sInstanceId;
		foreignOperation.m_sManifestId = fixture.m_Manifest.m_sManifestId;
		foreignOperation.m_sSpawnResultId = fixture.m_Batch.m_sResultId;
		saveData.m_aOperations.Insert(foreignOperation);

		HST_CampaignState restored = saveData.Restore();
		HST_ActiveMissionState restoredMission;
		HST_OperationRecordState restoredCanonical;
		HST_ForceSpawnResultState restoredBatch;
		if (restored)
		{
			restoredMission = restored.FindActiveMission(fixture.m_Mission.m_sInstanceId);
			restoredCanonical = restored.FindOperation(fixture.m_Operation.m_sOperationId);
			restoredBatch = restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
		}
		bool exact;
		if (restoredMission && restoredCanonical && restoredBatch)
		{
			bool missionQuarantined = restoredMission.m_iOperationContractVersion == HST_MissionConvoyOperationService.QUARANTINED_CONTRACT_VERSION
				&& restoredMission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
				&& restoredMission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
				&& restoredMission.m_sRuntimeFailureReason.Contains("operation identity is ambiguous")
				&& AreExactMissionGroupsNonOperational(restored, restoredMission);
			bool evidencePreserved = restoredCanonical.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				&& restoredCanonical.m_iRevision == fixture.m_Operation.m_iRevision
				&& restoredBatch.m_bStrategicProjectionHeld && !restoredBatch.m_bCancelRequested
				&& restoredBatch.m_iLifecycleRevision == fixture.m_Batch.m_iLifecycleRevision
				&& SumElementRevisionsForOperation(restored, restoredCanonical.m_sOperationId) == SumElementRevisions(fixture)
				&& restored.FindOperation(foreignOperation.m_sOperationId) != null;
			exact = missionQuarantined && evidencePreserved;
		}
		string reason;
		int operationRows = -1;
		if (restoredMission)
			reason = restoredMission.m_sRuntimeFailureReason;
		if (restored)
			operationRows = restored.m_aOperations.Count();
		evidence = string.Format("restore foreign authority rejected %1 reason '%2' rows %3", exact, reason, operationRows);
		return exact;
	}

	protected bool ProveRestoreMissingRequiredCargo(out string evidence)
	{
		evidence = "restore missing-required-cargo fixture did not run";
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture(
			"restore_missing_required_cargo", true, "convoy_money");
		if (!Ready(fixture))
		{
			evidence = BuildFixtureFailure(fixture);
			return false;
		}
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_ForceManifestState manifest = FindSavedManifest(saveData, fixture.m_Manifest.m_sManifestId);
		HST_ForceSpawnResultState batch = FindSavedBatch(saveData, fixture.m_Batch.m_sResultId);
		HST_MissionAssetState cargo = FindSavedMissionAsset(saveData, fixture.m_Cargo.m_sAssetId);
		HST_ConvoyElementState lead = FindSavedConvoyElement(
			saveData,
			HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, 0));
		if (!manifest || !batch || !cargo || !lead || manifest.m_aAssets.Count() != 1)
			return false;
		HST_ForceManifestAssetState assetSlot = manifest.m_aAssets[0];
		if (!assetSlot)
			return false;
		HST_ForceSpawnSlotResultState batchSlot = batch.FindSlotResult(assetSlot.m_sSlotId);
		int assetSlotIndex = manifest.m_aAssets.Find(assetSlot);
		int batchSlotIndex = batch.m_aSlotResults.Find(batchSlot);
		int cargoIndex = saveData.m_aMissionAssets.Find(cargo);
		if (!batchSlot || assetSlotIndex < 0 || batchSlotIndex < 0 || cargoIndex < 0)
			return false;

		manifest.m_aAssets.Remove(assetSlotIndex);
		batch.m_aSlotResults.Remove(batchSlotIndex);
		saveData.m_aMissionAssets.Remove(cargoIndex);
		lead.m_sCargoAssetId = "";
		batch.m_iExpectedSlotCount = batch.m_aSlotResults.Count();
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		manifest.m_sManifestHash = integrity.BuildManifestHash(manifest);
		batch.m_sManifestHash = manifest.m_sManifestHash;

		HST_CampaignState restored = saveData.Restore();
		HST_ActiveMissionState restoredMission;
		HST_OperationRecordState restoredOperation;
		HST_ForceManifestState restoredManifest;
		HST_ForceSpawnResultState restoredBatch;
		if (restored)
		{
			restoredMission = restored.FindActiveMission(fixture.m_Mission.m_sInstanceId);
			restoredOperation = restored.FindOperation(fixture.m_Operation.m_sOperationId);
			restoredManifest = restored.FindForceManifest(fixture.m_Manifest.m_sManifestId);
			restoredBatch = restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
		}
		bool exact = restoredMission && restoredOperation && restoredManifest && restoredBatch
			&& restoredMission.m_iOperationContractVersion == HST_MissionConvoyOperationService.QUARANTINED_CONTRACT_VERSION
			&& restoredMission.m_sRuntimeFailureReason.Contains("requires exactly one compatible cargo row")
			&& restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& restoredManifest.m_aAssets.Count() == 0
			&& integrity.BuildManifestHash(restoredManifest) == restoredManifest.m_sManifestHash
			&& restoredBatch.m_iExpectedSlotCount == restoredBatch.m_aSlotResults.Count()
			&& restoredBatch.m_sManifestHash == restoredManifest.m_sManifestHash
			&& !restored.FindMissionAsset(fixture.m_Cargo.m_sAssetId)
			&& AreExactMissionGroupsNonOperational(restored, restoredMission);
		string reason;
		if (restoredMission)
			reason = restoredMission.m_sRuntimeFailureReason;
		evidence = string.Format(
			"coherent restore without required payload rejected %1 reason '%2' hash/slots %3/%4",
			exact,
			reason,
			restoredManifest && integrity.BuildManifestHash(restoredManifest) == restoredManifest.m_sManifestHash,
			restoredBatch && restoredBatch.m_iExpectedSlotCount == restoredBatch.m_aSlotResults.Count());
		return exact;
	}

	protected bool ProveRestoreCaptiveSourceTypeCorruption(out string evidence)
	{
		evidence = "restore captive-source-type fixture did not run";
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture(
			"restore_captive_non_character", true, "convoy_prisoners");
		if (!Ready(fixture))
		{
			evidence = BuildFixtureFailure(fixture);
			return false;
		}
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_ForceManifestState manifest = FindSavedManifest(saveData, fixture.m_Manifest.m_sManifestId);
		HST_ForceSpawnResultState batch = FindSavedBatch(saveData, fixture.m_Batch.m_sResultId);
		HST_MissionAssetState cargo = FindSavedMissionAsset(saveData, fixture.m_Cargo.m_sAssetId);
		if (!manifest || !batch || !cargo || manifest.m_aAssets.Count() != 1)
			return false;
		HST_ForceManifestAssetState assetSlot = manifest.m_aAssets[0];
		if (!assetSlot)
			return false;
		HST_ForceSpawnSlotResultState batchSlot = batch.FindSlotResult(assetSlot.m_sSlotId);
		if (!batchSlot)
			return false;

		cargo.m_sPrefab = PROOF_CARGO_PREFAB;
		assetSlot.m_sPrefab = PROOF_CARGO_PREFAB;
		batchSlot.m_sSpawnedPrefab = PROOF_CARGO_PREFAB;
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		manifest.m_sManifestHash = integrity.BuildManifestHash(manifest);
		batch.m_sManifestHash = manifest.m_sManifestHash;

		HST_CampaignState restored = saveData.Restore();
		HST_ActiveMissionState restoredMission;
		HST_OperationRecordState restoredOperation;
		HST_ForceManifestState restoredManifest;
		HST_ForceSpawnResultState restoredBatch;
		HST_MissionAssetState restoredCargo;
		if (restored)
		{
			restoredMission = restored.FindActiveMission(fixture.m_Mission.m_sInstanceId);
			restoredOperation = restored.FindOperation(fixture.m_Operation.m_sOperationId);
			restoredManifest = restored.FindForceManifest(fixture.m_Manifest.m_sManifestId);
			restoredBatch = restored.FindForceSpawnResult(fixture.m_Batch.m_sResultId);
			restoredCargo = restored.FindMissionAsset(fixture.m_Cargo.m_sAssetId);
		}
		bool exact = restoredMission && restoredOperation && restoredManifest && restoredBatch && restoredCargo
			&& restoredMission.m_iOperationContractVersion == HST_MissionConvoyOperationService.QUARANTINED_CONTRACT_VERSION
			&& restoredMission.m_sRuntimeFailureReason.Contains("not a boardable character with compartment access")
			&& restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& integrity.BuildManifestHash(restoredManifest) == restoredManifest.m_sManifestHash
			&& restoredBatch.m_sManifestHash == restoredManifest.m_sManifestHash
			&& restoredCargo.m_sPrefab == PROOF_CARGO_PREFAB
			&& restoredManifest.m_aAssets.Count() == 1
			&& restoredManifest.m_aAssets[0].m_sPrefab == PROOF_CARGO_PREFAB
			&& AreExactMissionGroupsNonOperational(restored, restoredMission);
		string reason;
		if (restoredMission)
			reason = restoredMission.m_sRuntimeFailureReason;
		evidence = string.Format(
			"coherent restore with non-character captive rejected %1 reason '%2' prefab/hash preserved %3/%4",
			exact,
			reason,
			restoredCargo && restoredCargo.m_sPrefab == PROOF_CARGO_PREFAB,
			restoredManifest && integrity.BuildManifestHash(restoredManifest) == restoredManifest.m_sManifestHash);
		return exact;
	}

	protected bool ProveMissionlessDurableClaimantPreservation(out string evidence)
	{
		evidence = "missionless durable-claimant fixture did not run";
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("missionless_claimant");
		if (!Ready(fixture))
		{
			evidence = BuildFixtureFailure(fixture);
			return false;
		}
		string groupId = HST_MissionConvoyOperationService.BuildGroupId(fixture.m_Mission, 0);
		string elementId = HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, 0);
		HST_ActiveGroupState originalGroup = fixture.m_State.FindActiveGroup(groupId);
		HST_ConvoyElementState originalElement = fixture.m_State.FindConvoyElement(elementId);
		if (!originalGroup || !originalElement)
			return false;
		string operationId = originalGroup.m_sOperationId;
		string manifestId = originalGroup.m_sManifestId;
		int groupInfantry = originalGroup.m_iInfantryCount;
		int elementRevision = originalElement.m_iRevision;

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(fixture.m_State);
		HST_ActiveMissionState savedMission = FindSavedMission(saveData, fixture.m_Mission.m_sInstanceId);
		int missionIndex = saveData.m_aActiveMissions.Find(savedMission);
		if (!savedMission || missionIndex < 0)
			return false;
		saveData.m_aActiveMissions.Remove(missionIndex);

		HST_CampaignState restored = saveData.Restore();
		HST_ActiveGroupState restoredGroup;
		HST_ConvoyElementState restoredElement;
		if (restored)
		{
			restoredGroup = restored.FindActiveGroup(groupId);
			restoredElement = restored.FindConvoyElement(elementId);
		}
		bool survivedRestore = restored && !restored.FindActiveMission(fixture.m_Mission.m_sInstanceId)
			&& restoredGroup && restoredElement
			&& restoredGroup.m_sOperationId == operationId
			&& restoredGroup.m_sManifestId == manifestId
			&& restoredGroup.m_sConvoyElementId == elementId
			&& restoredGroup.m_iInfantryCount == groupInfantry
			&& restoredElement.m_iRevision == elementRevision;
		if (survivedRestore)
		{
			HST_PhysicalWarService physicalWar = new HST_PhysicalWarService();
			physicalWar.ReconcileInactiveMissionConvoyRuntime(restored);
			restoredGroup = restored.FindActiveGroup(groupId);
			restoredElement = restored.FindConvoyElement(elementId);
		}
		bool survivedCleanup = survivedRestore && restoredGroup && restoredElement
			&& restoredGroup.m_sOperationId == operationId
			&& restoredGroup.m_sManifestId == manifestId
			&& restoredGroup.m_sConvoyElementId == elementId
			&& restoredGroup.m_iInfantryCount == groupInfantry
			&& restoredElement.m_iRevision == elementRevision;
		evidence = string.Format(
			"missionless prefix/element claimant survived restore/cleanup %1/%2 with operation/manifest/element evidence %3/%4/%5",
			survivedRestore,
			survivedCleanup,
			operationId,
			manifestId,
			elementId);
		return survivedCleanup;
	}


	protected bool ProveRestoreLifecycleGuards(out string evidence)
	{
		evidence = "restore lifecycle guard fixture did not run";
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("restore_lifecycle_guards", true, "convoy_money");
		if (!Ready(fixture) || fixture.m_Manifest.m_aGroups.Count() == 0
			|| fixture.m_Manifest.m_aVehicles.Count() == 0 || fixture.m_Manifest.m_aAssets.Count() == 0)
			return false;

		HST_ForceSpawnSlotResultState groupRoot = fixture.m_Batch.FindSlotResult(fixture.m_Manifest.m_aGroups[0].m_sElementId);
		HST_ForceSpawnSlotResultState vehicleRoot = fixture.m_Batch.FindSlotResult(fixture.m_Manifest.m_aVehicles[0].m_sSlotId);
		HST_ForceSpawnSlotResultState assetRoot = fixture.m_Batch.FindSlotResult(fixture.m_Manifest.m_aAssets[0].m_sSlotId);
		if (!groupRoot || !vehicleRoot || !assetRoot)
			return false;

		HST_CampaignSaveData groupCasualtySave = new HST_CampaignSaveData();
		groupRoot.m_bCasualtyConfirmed = true;
		groupRoot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
		groupCasualtySave.Capture(fixture.m_State);
		groupRoot.m_bCasualtyConfirmed = false;
		groupRoot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;

		HST_CampaignSaveData vehicleCasualtySave = new HST_CampaignSaveData();
		vehicleRoot.m_bCasualtyConfirmed = true;
		vehicleRoot.m_bAliveVerified = false;
		vehicleRoot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
		vehicleCasualtySave.Capture(fixture.m_State);
		vehicleRoot.m_bCasualtyConfirmed = false;
		vehicleRoot.m_bAliveVerified = true;
		vehicleRoot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;

		HST_CampaignSaveData assetCasualtySave = new HST_CampaignSaveData();
		assetRoot.m_bCasualtyConfirmed = true;
		assetRoot.m_bAliveVerified = false;
		assetRoot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
		assetCasualtySave.Capture(fixture.m_State);
		assetRoot.m_bCasualtyConfirmed = false;
		assetRoot.m_bAliveVerified = true;
		assetRoot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;

		HST_CampaignSaveData retiredRootSave = new HST_CampaignSaveData();
		vehicleRoot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
		retiredRootSave.Capture(fixture.m_State);
		vehicleRoot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;

		HST_CampaignSaveData retiredDutySave = new HST_CampaignSaveData();
		fixture.m_Operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_RETIRED;
		fixture.m_Operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_RETIRED;
		retiredDutySave.Capture(fixture.m_State);
		fixture.m_Operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING;
		fixture.m_Operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING;

		HST_CampaignSaveData mismatchedDutySave = new HST_CampaignSaveData();
		fixture.m_Operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		mismatchedDutySave.Capture(fixture.m_State);
		fixture.m_Operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING;

		HST_CampaignSaveData retiredProjectionSave = new HST_CampaignSaveData();
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
		retiredProjectionSave.Capture(fixture.m_State);
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;

		HST_CampaignSaveData crossedProjectionSave = new HST_CampaignSaveData();
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
		crossedProjectionSave.Capture(fixture.m_State);
		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;

		bool groupCasualtyRejected = RestoreSaveQuarantinesLifecycle(
			groupCasualtySave, fixture, "non-member root contains casualty authority");
		bool vehicleCasualtyRejected = RestoreSaveQuarantinesLifecycle(
			vehicleCasualtySave, fixture, "non-member root contains casualty authority");
		bool assetCasualtyRejected = RestoreSaveQuarantinesLifecycle(
			assetCasualtySave, fixture, "non-member root contains casualty authority");
		bool retiredRootRejected = RestoreSaveQuarantinesLifecycle(
			retiredRootSave, fixture, "root is retired or unregistered");
		bool retiredDutyRejected = RestoreSaveQuarantinesLifecycle(
			retiredDutySave, fixture, "duty and resume states are illegal");
		bool mismatchedDutyRejected = RestoreSaveQuarantinesLifecycle(
			mismatchedDutySave, fixture, "duty and resume states are illegal");
		bool retiredProjectionRejected = RestoreSaveQuarantinesLifecycle(
			retiredProjectionSave, fixture, "cross-product is illegal");
		bool crossedProjectionRejected = RestoreSaveQuarantinesLifecycle(
			crossedProjectionSave, fixture, "cross-product is illegal");

		evidence = string.Format(
			"restore rejected casualty group/vehicle/asset %1/%2/%3 | retired root %4 | retired/mismatched duty %5/%6 | retired/crossed projection %7/%8",
			groupCasualtyRejected,
			vehicleCasualtyRejected,
			assetCasualtyRejected,
			retiredRootRejected,
			retiredDutyRejected,
			mismatchedDutyRejected,
			retiredProjectionRejected,
			crossedProjectionRejected);
		bool rootGuardsExact = groupCasualtyRejected && vehicleCasualtyRejected
			&& assetCasualtyRejected && retiredRootRejected;
		bool operationGuardsExact = retiredDutyRejected && mismatchedDutyRejected
			&& retiredProjectionRejected && crossedProjectionRejected;
		return rootGuardsExact && operationGuardsExact;
	}

	protected bool RestoreSaveQuarantinesLifecycle(
		HST_CampaignSaveData saveData,
		HST_MissionConvoyOperationProofFixture fixture,
		string expectedReason)
	{
		if (!saveData || !fixture || !fixture.m_Mission || !fixture.m_Operation)
			return false;
		HST_CampaignState restored = saveData.Restore();
		HST_ActiveMissionState restoredMission;
		HST_OperationRecordState restoredOperation;
		if (restored)
		{
			restoredMission = restored.FindActiveMission(fixture.m_Mission.m_sInstanceId);
			restoredOperation = restored.FindOperation(fixture.m_Operation.m_sOperationId);
		}
		return restoredMission && restoredOperation
			&& restoredMission.m_iOperationContractVersion == HST_MissionConvoyOperationService.QUARANTINED_CONTRACT_VERSION
			&& restoredMission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
			&& restoredMission.m_sRuntimeFailureReason.Contains(expectedReason)
			&& restoredOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
	}

	protected void ProveMarkers(HST_MissionConvoyOperationProofReport report)
	{
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("markers");
		if (!Ready(fixture))
		{
			report.m_sMarkerEvidence = BuildFixtureFailure(fixture);
			return;
		}

		fixture.m_State.m_iElapsedSeconds += 120;
		fixture.m_Service.TickBeforePhysical(fixture.m_State, fixture.m_Preset, 120);
		HST_MapMarkerService markers = new HST_MapMarkerService();
		markers.RebuildAllMarkers(fixture.m_State, fixture.m_Preset);
		string currentId = "hst_mission_convoy_current_" + fixture.m_Mission.m_sInstanceId;
		string destinationId = "hst_mission_convoy_dest_" + fixture.m_Mission.m_sInstanceId;
		HST_MapMarkerState current = fixture.m_State.FindMapMarker(currentId);
		HST_MapMarkerState destination = fixture.m_State.FindMapMarker(destinationId);
		int linkedBeforeCleanup = CountMissionMarkers(fixture.m_State, fixture.m_Mission.m_sInstanceId);
		int aggregateCount = CountMarkerId(fixture.m_State, currentId);
		int destinationCount = CountMarkerId(fixture.m_State, destinationId);
		int individualCount = CountMarkerIdPrefix(fixture.m_State, "hst_mission_convoy_vehicle_asset_" + fixture.m_Mission.m_sInstanceId);
		bool aggregateExact = current && destination
			&& linkedBeforeCleanup == 2
			&& aggregateCount == 1
			&& destinationCount == 1
			&& individualCount == 0
			&& current.m_bVisible && destination.m_bVisible
			&& Distance2D(current.m_vPosition, fixture.m_Operation.m_vStrategicPosition) < 0.1;
		float aggregateCursorDelta = -1.0;
		if (current)
			aggregateCursorDelta = Distance2D(current.m_vPosition, fixture.m_Operation.m_vStrategicPosition);
		fixture.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
		fixture.m_Mission.m_sRuntimePhase = HST_MissionConvoyOperationService.CONVOY_FAILED;
		fixture.m_Mission.m_sRuntimeFailureReason = "mission convoy marker proof terminal settlement";
		bool markerSettlementChanged = fixture.m_Service.TickAfterOutcomes(fixture.m_State);
		markers.RebuildAllMarkers(fixture.m_State, fixture.m_Preset);
		int linkedAfterSettlement = CountMissionMarkers(fixture.m_State, fixture.m_Mission.m_sInstanceId);
		bool cleanupExact = markerSettlementChanged
			&& fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& CountMarkerId(fixture.m_State, currentId) == 0
			&& CountMarkerId(fixture.m_State, destinationId) == 0
			&& linkedAfterSettlement == 0;

		HST_MissionConvoyOperationProofFixture recovery = BuildAdmittedFixture("marker_recovery", true, "convoy_money");
		bool recoveryPrepared = Ready(recovery) && EliminateExactCrewAuthority(recovery);
		if (recoveryPrepared)
		{
			recovery.m_State.m_vHQPosition = BuildProofPosition(PROOF_ROUTE_START_X - 400.0, PROOF_ROUTE_Z + 300.0);
			recovery.m_Cargo.m_vTargetPosition = recovery.m_State.m_vHQPosition;
			recovery.m_Service.TickBeforePhysical(recovery.m_State, recovery.m_Preset, 1);
		}
		markers.RebuildAllMarkers(recovery.m_State, recovery.m_Preset);
		string recoveryCurrentId = "hst_mission_convoy_current_" + recovery.m_Mission.m_sInstanceId;
		string recoveryDestinationId = "hst_mission_convoy_dest_" + recovery.m_Mission.m_sInstanceId;
		HST_MapMarkerState recoveryCurrent = recovery.m_State.FindMapMarker(recoveryCurrentId);
		HST_MapMarkerState recoveryDestination = recovery.m_State.FindMapMarker(recoveryDestinationId);
		int recoveryCurrentCount = CountMarkerId(recovery.m_State, recoveryCurrentId);
		int recoveryDestinationCount = CountMarkerId(recovery.m_State, recoveryDestinationId);
		int recoveryIndividualCount = CountMarkerIdPrefix(recovery.m_State, "hst_mission_convoy_vehicle_");
		int recoveryOutcomeCount = CountMarkerIdPrefix(recovery.m_State, "hst_mission_convoy_outcome_");
		bool recoveryAggregateExact = recoveryPrepared
			&& recovery.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_ELIMINATED
			&& recoveryCurrent && recoveryDestination && recoveryCurrentCount == 1 && recoveryDestinationCount == 1
			&& recoveryIndividualCount == 0 && recoveryOutcomeCount == 0
			&& Distance2D(recoveryCurrent.m_vPosition, recovery.m_Operation.m_vStrategicPosition) < 0.1
			&& Distance2D(recoveryDestination.m_vPosition, recovery.m_State.m_vHQPosition) < 0.1
			&& recoveryDestination.m_sLabel.Contains("HQ");
		markers.CleanupMarkers(recovery.m_State);
		bool recoveryCleanupExact = CountLiveMarkers(recovery.m_State) == 0;

		HST_MissionConvoyOperationProofFixture supplyRecovery = BuildAdmittedFixture("marker_supply_recovery", true, "convoy_supplies");
		bool supplyRecoveryPrepared = Ready(supplyRecovery) && EliminateExactCrewAuthority(supplyRecovery);
		if (supplyRecoveryPrepared)
		{
			HST_ZoneState supplyDestinationZone = supplyRecovery.m_State.FindZone(PROOF_SOURCE_ZONE_ID);
			if (supplyDestinationZone)
			{
				supplyDestinationZone.m_sDisplayName = "Proof Support Town";
				supplyRecovery.m_Cargo.m_vTargetPosition = supplyDestinationZone.m_vPosition;
			}
			supplyRecovery.m_Mission.m_bConvoyCrewEliminatedOutcomeApplied = true;
			foreach (HST_MissionObjectiveState supplyObjective : supplyRecovery.m_State.m_aMissionObjectives)
			{
				if (supplyObjective && supplyObjective.m_sMissionInstanceId == supplyRecovery.m_Mission.m_sInstanceId)
					supplyObjective.m_bComplete = true;
			}
			supplyRecovery.m_Service.TickBeforePhysical(supplyRecovery.m_State, supplyRecovery.m_Preset, 1);
		}
		HST_MissionObjectiveService objectives = new HST_MissionObjectiveService();
		bool supplyOutcomeHeld = supplyRecoveryPrepared
			&& supplyRecovery.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_ELIMINATED
			&& !objectives.AreMissionObjectivesComplete(supplyRecovery.m_State, supplyRecovery.m_Mission.m_sInstanceId);
		markers.RebuildAllMarkers(supplyRecovery.m_State, supplyRecovery.m_Preset);
		string supplyCurrentId = "hst_mission_convoy_current_" + supplyRecovery.m_Mission.m_sInstanceId;
		string supplyDestinationId = "hst_mission_convoy_dest_" + supplyRecovery.m_Mission.m_sInstanceId;
		HST_MapMarkerState supplyDestination = supplyRecovery.m_State.FindMapMarker(supplyDestinationId);
		int supplyCurrentCount = CountMarkerId(supplyRecovery.m_State, supplyCurrentId);
		int supplyDestinationCount = CountMarkerId(supplyRecovery.m_State, supplyDestinationId);
		int supplyIndividualCount = CountMarkerIdPrefix(supplyRecovery.m_State, "hst_mission_convoy_vehicle_");
		int supplyOutcomeCount = CountMarkerIdPrefix(supplyRecovery.m_State, "hst_mission_convoy_outcome_");
		bool supplyAggregateExact = supplyOutcomeHeld
			&& supplyDestination && supplyCurrentCount == 1 && supplyDestinationCount == 1
			&& supplyIndividualCount == 0 && supplyOutcomeCount == 0;
		HST_ZoneState expectedSupplyDestination = supplyRecovery.m_State.FindZone(PROOF_SOURCE_ZONE_ID);
		if (supplyAggregateExact)
			supplyAggregateExact = expectedSupplyDestination
				&& Distance2D(supplyDestination.m_vPosition, expectedSupplyDestination.m_vPosition) < 0.1
				&& supplyDestination.m_sLabel.Contains("Proof Support Town");
		markers.CleanupMarkers(supplyRecovery.m_State);
		bool supplyCleanupExact = CountLiveMarkers(supplyRecovery.m_State) == 0;

		HST_MissionConvoyOperationProofFixture vehicleRecovery = BuildAdmittedFixture("marker_vehicle_recovery", true, "convoy_ammo");
		bool vehicleRecoveryPrepared = Ready(vehicleRecovery) && EliminateExactCrewAuthority(vehicleRecovery);
		if (vehicleRecoveryPrepared)
			vehicleRecovery.m_Service.TickBeforePhysical(vehicleRecovery.m_State, vehicleRecovery.m_Preset, 1);
		markers.RebuildAllMarkers(vehicleRecovery.m_State, vehicleRecovery.m_Preset);
		string vehicleCurrentId = "hst_mission_convoy_current_" + vehicleRecovery.m_Mission.m_sInstanceId;
		string vehicleDestinationId = "hst_mission_convoy_dest_" + vehicleRecovery.m_Mission.m_sInstanceId;
		HST_MapMarkerState vehicleDestination = vehicleRecovery.m_State.FindMapMarker(vehicleDestinationId);
		HST_MissionAssetState expectedVehicleDestination = vehicleRecovery.m_State.FindMissionAsset(
			BuildVehicleAssetId(vehicleRecovery.m_Mission, 0));
		int vehicleCurrentCount = CountMarkerId(vehicleRecovery.m_State, vehicleCurrentId);
		int vehicleDestinationCount = CountMarkerId(vehicleRecovery.m_State, vehicleDestinationId);
		int vehicleIndividualCount = CountMarkerIdPrefix(vehicleRecovery.m_State, "hst_mission_convoy_vehicle_");
		int vehicleOutcomeCount = CountMarkerIdPrefix(vehicleRecovery.m_State, "hst_mission_convoy_outcome_");
		bool vehicleAggregateExact = vehicleRecoveryPrepared
			&& vehicleRecovery.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_ELIMINATED
			&& vehicleDestination && expectedVehicleDestination
			&& vehicleCurrentCount == 1 && vehicleDestinationCount == 1
			&& vehicleIndividualCount == 0 && vehicleOutcomeCount == 0
			&& Distance2D(vehicleDestination.m_vPosition, expectedVehicleDestination.m_vCurrentPosition) < 0.1
			&& vehicleDestination.m_sLabel.Contains("vehicle recovery");
		markers.CleanupMarkers(vehicleRecovery.m_State);
		bool vehicleCleanupExact = CountLiveMarkers(vehicleRecovery.m_State) == 0;

		report.m_bMarkerExact = aggregateExact && cleanupExact
			&& recoveryAggregateExact && recoveryCleanupExact
			&& supplyAggregateExact && supplyCleanupExact
			&& vehicleAggregateExact && vehicleCleanupExact;
		report.m_sMarkerEvidence = string.Format(
			"linked/current/destination/individual %1/%2/%3/%4 | aggregate cursor delta %5m | post-settlement linked %6",
			linkedBeforeCleanup,
			aggregateCount,
			destinationCount,
			individualCount,
			Math.Round(aggregateCursorDelta),
			linkedAfterSettlement);
		report.m_sMarkerEvidence = report.m_sMarkerEvidence + string.Format(
			" | recovery prepared/aggregate/cleanup %1/%2/%3 current/destination/individual/outcome %4/%5/%6/%7",
			recoveryPrepared,
			recoveryAggregateExact,
			recoveryCleanupExact,
			recoveryCurrentCount,
			recoveryDestinationCount,
			recoveryIndividualCount,
			recoveryOutcomeCount);
		report.m_sMarkerEvidence = report.m_sMarkerEvidence + string.Format(
			" | supply recovery held/aggregate/cleanup %1/%2/%3 current/destination/individual/outcome %4/%5/%6/%7",
			supplyOutcomeHeld,
			supplyAggregateExact,
			supplyCleanupExact,
			supplyCurrentCount,
			supplyDestinationCount,
			supplyIndividualCount,
			supplyOutcomeCount);
		report.m_sMarkerEvidence = report.m_sMarkerEvidence + string.Format(
			" | vehicle recovery prepared/aggregate/cleanup %1/%2/%3 current/destination/individual/outcome %4/%5/%6/%7",
			vehicleRecoveryPrepared,
			vehicleAggregateExact,
			vehicleCleanupExact,
			vehicleCurrentCount,
			vehicleDestinationCount,
			vehicleIndividualCount,
			vehicleOutcomeCount);
	}

	protected void ProveWatchdog(HST_MissionConvoyOperationProofReport report)
	{
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("watchdog");
		if (!Ready(fixture))
		{
			report.m_sWatchdogEvidence = BuildFixtureFailure(fixture);
			return;
		}

		fixture.m_Operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		fixture.m_Operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		fixture.m_Operation.m_iMaterializationStateEnteredAtSecond = fixture.m_State.m_iElapsedSeconds;
		fixture.m_State.m_iElapsedSeconds += HST_MissionConvoyOperationService.EXACT_MATERIALIZATION_TIMEOUT_SECONDS + 1;
		bool timeoutChanged = fixture.m_Service.TickAfterPhysical(fixture.m_State);
		bool timeoutFailedClosed = timeoutChanged
			&& fixture.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_FAILED
			&& fixture.m_Mission.m_sRuntimeFailureReason.Contains("materialization")
			&& fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		bool settlementChanged = fixture.m_Service.TickAfterOutcomes(fixture.m_State);
		int settledRevision = fixture.m_Operation.m_iRevision;
		int batchRevision = fixture.m_Batch.m_iLifecycleRevision;
		int elementRevisionSum = SumElementRevisions(fixture);
		bool beforeReplay = fixture.m_Service.TickBeforePhysical(fixture.m_State, fixture.m_Preset, 1);
		bool physicalReplay = fixture.m_Service.TickAfterPhysical(fixture.m_State);
		bool outcomeReplay = fixture.m_Service.TickAfterOutcomes(fixture.m_State);
		bool terminalExact = settlementChanged
			&& fixture.m_Operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			&& fixture.m_Operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED
			&& fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			&& fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
			&& CountElementsWithDisposition(fixture, HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_RETIRED) == HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT
			&& CountMobileElements(fixture) == 0
			&& fixture.m_Batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED
			&& !fixture.m_Batch.m_bStrategicProjectionHeld
			&& AllNonCasualtySlotsRetired(fixture.m_Batch);
		bool replayExact = !beforeReplay && !physicalReplay && !outcomeReplay
			&& fixture.m_Operation.m_iRevision == settledRevision
			&& fixture.m_Batch.m_iLifecycleRevision == batchRevision
			&& SumElementRevisions(fixture) == elementRevisionSum;
		string cargoOnlyEvidence;
		bool cargoOnlyExact = ProveCargoOnlyRecoveryRoot(cargoOnlyEvidence);

		report.m_bWatchdogExact = timeoutFailedClosed && terminalExact && replayExact && cargoOnlyExact;
		report.m_sWatchdogEvidence = string.Format(
			"timeout changed/failed %1/%2 reason '%3' | settled/result/elements/mobile %4/%5/%6/%7",
			timeoutChanged,
			timeoutFailedClosed,
			fixture.m_Mission.m_sRuntimeFailureReason,
			settlementChanged,
			fixture.m_Operation.m_eTerminalResult,
			CountElementsWithDisposition(fixture, HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_RETIRED),
			CountMobileElements(fixture));
		report.m_sWatchdogEvidence = report.m_sWatchdogEvidence + string.Format(
			" | replay before/physical/outcome %1/%2/%3 | revisions %4/%5/%6",
			beforeReplay,
			physicalReplay,
			outcomeReplay,
			fixture.m_Operation.m_iRevision - settledRevision,
			fixture.m_Batch.m_iLifecycleRevision - batchRevision,
			SumElementRevisions(fixture) - elementRevisionSum);
		report.m_sWatchdogEvidence = report.m_sWatchdogEvidence + " | " + cargoOnlyEvidence;
	}

	protected bool ProveCargoOnlyRecoveryRoot(out string evidence)
	{
		evidence = "cargo-only recovery fixture did not run";
		HST_MissionConvoyOperationProofFixture fixture = BuildAdmittedFixture("cargo_only_recovery", true, "convoy_money");
		if (!Ready(fixture) || !EliminateExactCrewAuthority(fixture))
		{
			evidence = "cargo-only recovery: " + BuildFixtureFailure(fixture);
			return false;
		}
		for (int index = 0; index < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = fixture.m_State.FindMissionAsset(BuildVehicleAssetId(fixture.m_Mission, index));
			HST_ConvoyElementState element = fixture.m_State.FindConvoyElement(
				HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, index));
			HST_ActiveGroupState group = fixture.m_State.FindActiveGroup(
				HST_MissionConvoyOperationService.BuildGroupId(fixture.m_Mission, index));
			if (!asset || !element || !group)
				return false;
			asset.m_bSpawned = false;
			asset.m_bDestroyed = true;
			asset.m_bAlive = false;
			asset.m_sLastInteraction = "destroyed";
			element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED;
			element.m_fVehicleDamageFraction = 1.0;
			element.m_bPhysicalized = false;
			element.m_bMobile = false;
			element.m_sTerminalReason = "mission convoy cargo-only recovery proof destroyed vehicle";
			group.m_iSurvivorVehicleCount = 0;
		}
		fixture.m_Cargo.m_bSpawned = false;
		fixture.m_Cargo.m_bPickedUp = false;
		fixture.m_Cargo.m_bDelivered = false;
		fixture.m_Cargo.m_bDestroyed = false;
		fixture.m_Cargo.m_bOutcomeApplied = false;
		fixture.m_Cargo.m_bAttachedToCarrier = false;
		fixture.m_Cargo.m_sCarriedByVehicleId = "";
		HST_ConvoyElementState carrier = fixture.m_State.FindConvoyElement(
			HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, 0));
		if (carrier)
		{
			fixture.m_Cargo.m_vCurrentPosition = carrier.m_vCurrentPosition;
			fixture.m_Cargo.m_vLastKnownPosition = carrier.m_vCurrentPosition;
		}
		fixture.m_State.m_iElapsedSeconds++;
		bool recoveryChanged = fixture.m_Service.TickBeforePhysical(fixture.m_State, fixture.m_Preset, 1);
		bool exact = recoveryChanged
			&& fixture.m_Mission.m_sRuntimePhase == HST_MissionConvoyOperationService.CONVOY_ELIMINATED
			&& fixture.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			&& fixture.m_Operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& CountElementsWithDisposition(fixture, HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED)
				== HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT
			&& CountElementsWithDisposition(fixture, HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED) == 0
			&& fixture.m_Service.HasMaterializableRecoveryRoot(fixture.m_State, fixture.m_Mission);
		evidence = string.Format(
			"cargo-only recovery changed/exact/duty/materialization/destroyed %1/%2/%3/%4/%5",
			recoveryChanged,
			exact,
			fixture.m_Operation.m_eDutyState,
			fixture.m_Operation.m_eMaterializationState,
			CountElementsWithDisposition(fixture, HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED));
		return exact;
	}

	protected HST_MissionConvoyOperationProofFixture BuildAdmittedFixture(
		string suffix,
		bool admit = true,
		string missionId = "convoy_reinforcements")
	{
		HST_MissionConvoyOperationProofFixture fixture = new HST_MissionConvoyOperationProofFixture();
		fixture.m_State = new HST_CampaignState();
		fixture.m_State.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		fixture.m_State.m_iCampaignSeed = 520052;
		fixture.m_State.m_iElapsedSeconds = 100;
		fixture.m_Preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		fixture.m_PhysicalWar = new HST_PhysicalWarService();
		fixture.m_Service = new HST_MissionConvoyOperationService();
		fixture.m_Service.SetRuntimeServices(fixture.m_PhysicalWar);

		fixture.m_Mission = new HST_ActiveMissionState();
		fixture.m_Mission.m_sInstanceId = "mission_convoy_proof_" + suffix;
		fixture.m_Mission.m_sMissionId = missionId;
		fixture.m_Mission.m_sDisplayName = "Mission Convoy Proof";
		fixture.m_Mission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		fixture.m_Mission.m_sRuntimePrimitive = HST_MissionConvoyOperationService.CONVOY_PRIMITIVE;
		fixture.m_Mission.m_sRuntimeType = HST_MissionConvoyOperationService.EXACT_RUNTIME_TYPE;
		fixture.m_Mission.m_sRuntimePhase = "created";
		fixture.m_Mission.m_sTargetZoneId = PROOF_TARGET_ZONE_ID;
		fixture.m_Mission.m_sSiteId = "mission_convoy_proof_site_" + suffix;
		fixture.m_Mission.m_vTargetPosition = BuildProofPosition(PROOF_ROUTE_END_X, PROOF_ROUTE_Z);
		fixture.m_Mission.m_iRuntimeCounterA = 0;
		fixture.m_Mission.m_iRuntimeCounterB = 0;
		if (!fixture.m_Service.PrepareNewMissionContract(fixture.m_Mission))
		{
			fixture.m_sFailureReason = "exact contract preparation failed";
			return fixture;
		}
		fixture.m_State.m_aActiveMissions.Insert(fixture.m_Mission);

		HST_ZoneState source = new HST_ZoneState();
		source.m_sZoneId = PROOF_SOURCE_ZONE_ID;
		source.m_sOwnerFactionKey = PROOF_FACTION_KEY;
		source.m_vPosition = BuildProofPosition(PROOF_ROUTE_START_X, PROOF_ROUTE_Z);
		fixture.m_State.m_aZones.Insert(source);
		HST_ZoneState target = new HST_ZoneState();
		target.m_sZoneId = PROOF_TARGET_ZONE_ID;
		target.m_sOwnerFactionKey = PROOF_FACTION_KEY;
		target.m_vPosition = BuildProofPosition(PROOF_ROUTE_END_X, PROOF_ROUTE_Z);
		fixture.m_State.m_aZones.Insert(target);

		HST_GeneratedRouteState route = new HST_GeneratedRouteState();
		route.m_sRouteId = "mission_convoy_proof_route_" + suffix;
		route.m_sSourceZoneId = PROOF_SOURCE_ZONE_ID;
		route.m_sTargetZoneId = PROOF_TARGET_ZONE_ID;
		route.m_vStartPosition = BuildProofPosition(PROOF_ROUTE_START_X, PROOF_ROUTE_Z);
		route.m_vEndPosition = BuildProofPosition(PROOF_ROUTE_END_X, PROOF_ROUTE_Z);
		route.m_bRoadRoute = true;
		route.m_bValidatedForVehicles = true;
		HST_RouteWaypointState waypoint = new HST_RouteWaypointState();
		waypoint.m_sRouteId = route.m_sRouteId;
		waypoint.m_iIndex = 0;
		waypoint.m_vPosition = BuildProofPosition((PROOF_ROUTE_START_X + PROOF_ROUTE_END_X) * 0.5, PROOF_ROUTE_Z);
		waypoint.m_iRadiusMeters = 20;
		route.m_aWaypoints.Insert(waypoint);
		fixture.m_State.m_aGeneratedRoutes.Insert(route);
		HST_GeneratedSiteState site = new HST_GeneratedSiteState();
		site.m_sSiteId = fixture.m_Mission.m_sSiteId;
		site.m_sZoneId = PROOF_TARGET_ZONE_ID;
		site.m_sRouteId = route.m_sRouteId;
		site.m_vPosition = route.m_vEndPosition;
		fixture.m_State.m_aGeneratedSites.Insert(site);

		for (int i = 0; i < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; i++)
		{
			HST_MissionAssetState asset = new HST_MissionAssetState();
			asset.m_sAssetId = BuildVehicleAssetId(fixture.m_Mission, i);
			asset.m_sMissionInstanceId = fixture.m_Mission.m_sInstanceId;
			asset.m_sKind = "vehicle";
			asset.m_sRole = HST_MissionConvoyOperationService.VEHICLE_ROLE;
			asset.m_sPrefab = PROOF_VEHICLE_PREFAB;
			asset.m_vSourcePosition = BuildProofPosition(PROOF_ROUTE_START_X, PROOF_ROUTE_Z);
			asset.m_vCurrentPosition = asset.m_vSourcePosition;
			asset.m_vLastKnownPosition = asset.m_vSourcePosition;
			asset.m_vTargetPosition = BuildProofPosition(PROOF_ROUTE_END_X, PROOF_ROUTE_Z);
			fixture.m_State.m_aMissionAssets.Insert(asset);

			HST_ActiveGroupState group = new HST_ActiveGroupState();
			group.m_sGroupId = HST_MissionConvoyOperationService.BuildGroupId(fixture.m_Mission, i);
			group.m_sMissionInstanceId = fixture.m_Mission.m_sInstanceId;
			group.m_sZoneId = PROOF_TARGET_ZONE_ID;
			group.m_sFactionKey = PROOF_FACTION_KEY;
			group.m_sPrefab = PROOF_CREW_PREFAB;
			group.m_sRouteId = route.m_sRouteId;
			group.m_vSourcePosition = asset.m_vSourcePosition;
			group.m_vPosition = asset.m_vSourcePosition;
			group.m_vTargetPosition = asset.m_vTargetPosition;
			group.m_iInfantryCount = PROOF_CREW_PER_VEHICLE;
			group.m_iOriginalInfantryCount = PROOF_CREW_PER_VEHICLE;
			group.m_iVehicleCount = 1;
			group.m_iOriginalVehicleCount = 1;
			group.m_iSurvivorVehicleCount = 1;
			group.m_sRuntimeStatus = "queued";
			fixture.m_State.m_aActiveGroups.Insert(group);
		}

		if (missionId == "convoy_money" || missionId == "convoy_supplies")
		{
			fixture.m_Cargo = BuildProofCargoAsset(
				fixture.m_Mission,
				"convoy_payload",
				HST_MissionConvoyOperationService.CARGO_KIND,
				HST_MissionConvoyOperationService.PAYLOAD_ROLE,
				PROOF_CARGO_PREFAB);
		}
		else if (missionId == "convoy_prisoners")
		{
			fixture.m_Cargo = BuildProofCargoAsset(
				fixture.m_Mission,
				"convoy_captive",
				HST_MissionConvoyOperationService.CAPTIVE_KIND,
				HST_MissionConvoyOperationService.CAPTIVE_ROLE,
				PROOF_CAPTIVE_PREFAB);
		}
		if (fixture.m_Cargo)
			fixture.m_State.m_aMissionAssets.Insert(fixture.m_Cargo);

		HST_MissionObjectiveState objective = new HST_MissionObjectiveState();
		objective.m_sObjectiveId = "mission_convoy_proof_objective_" + suffix;
		objective.m_sMissionInstanceId = fixture.m_Mission.m_sInstanceId;
		objective.m_sTargetId = "convoy";
		objective.m_sRuntimePrimitive = fixture.m_Mission.m_sRuntimePrimitive;
		fixture.m_State.m_aMissionObjectives.Insert(objective);
		if (!admit)
			return fixture;

		fixture.m_Admission = fixture.m_Service.AdmitNewMission(fixture.m_State, fixture.m_Preset, fixture.m_Mission);
		if (!fixture.m_Admission || !fixture.m_Admission.m_bSuccess)
		{
			if (fixture.m_Admission)
				fixture.m_sFailureReason = fixture.m_Admission.m_sFailureReason;
			else
				fixture.m_sFailureReason = "admission returned no result";
			return fixture;
		}
		fixture.m_Operation = fixture.m_Admission.m_Operation;
		fixture.m_Manifest = fixture.m_Admission.m_Manifest;
		fixture.m_Batch = fixture.m_Admission.m_Batch;
		return fixture;
	}

	protected HST_MissionAssetState BuildProofCargoAsset(
		HST_ActiveMissionState mission,
		string identitySuffix,
		string kind,
		string role,
		string prefab)
	{
		if (!mission)
			return null;
		HST_MissionAssetState cargo = new HST_MissionAssetState();
		cargo.m_sAssetId = "asset_" + mission.m_sInstanceId + "_" + identitySuffix;
		cargo.m_sMissionInstanceId = mission.m_sInstanceId;
		cargo.m_sKind = kind;
		cargo.m_sRole = role;
		cargo.m_sPrefab = prefab;
		cargo.m_vSourcePosition = BuildProofPosition(PROOF_ROUTE_START_X, PROOF_ROUTE_Z);
		cargo.m_vCurrentPosition = cargo.m_vSourcePosition;
		cargo.m_vLastKnownPosition = cargo.m_vSourcePosition;
		cargo.m_vTargetPosition = BuildProofPosition(PROOF_ROUTE_END_X, PROOF_ROUTE_Z);
		return cargo;
	}

	protected bool ValidateFrozenManifest(HST_MissionConvoyOperationProofFixture fixture)
	{
		if (!Ready(fixture) || !fixture.m_Manifest.m_bFrozen
			|| fixture.m_Manifest.m_aGroups.Count() != HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT
			|| fixture.m_Manifest.m_aVehicles.Count() != HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT
			|| fixture.m_Manifest.m_aMembers.Count() != HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT * PROOF_CREW_PER_VEHICLE
			|| fixture.m_Manifest.m_iAcceptedVehicleCount != HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT
			|| fixture.m_Manifest.m_iAcceptedMemberCount != fixture.m_Manifest.m_aMembers.Count())
			return false;

		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		if (fixture.m_Manifest.m_sManifestHash.IsEmpty() || integrity.BuildManifestHash(fixture.m_Manifest) != fixture.m_Manifest.m_sManifestHash)
			return false;
		for (int i = 0; i < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; i++)
		{
			HST_ForceManifestGroupState group = fixture.m_Manifest.m_aGroups[i];
			HST_ForceManifestVehicleState vehicle = fixture.m_Manifest.m_aVehicles[i];
			if (!group || !vehicle
				|| group.m_sElementId != HST_MissionConvoyOperationService.BuildCrewGroupElementId(fixture.m_Mission, i)
				|| group.m_sCatalogEntryId != PROOF_GROUP_CATALOG_ENTRY_ID
				|| group.m_sPrefab != PROOF_CREW_PREFAB
				|| vehicle.m_sSlotId != HST_MissionConvoyOperationService.BuildVehicleSlotId(fixture.m_Mission, i)
				|| vehicle.m_sGroupElementId != group.m_sElementId
				|| vehicle.m_sPrefab != PROOF_VEHICLE_PREFAB
				|| group.m_iExpectedMemberCount != PROOF_CREW_PER_VEHICLE)
				return false;
		}
		return ValidateConcreteMemberSlots(fixture.m_Manifest);
	}

	protected bool ValidateConcreteMemberSlots(HST_ForceManifestState manifest)
	{
		if (!manifest || manifest.m_aMembers.Count() != HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT * PROOF_CREW_PER_VEHICLE)
			return false;
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member || member.m_sAssignedVehicleSlotId.IsEmpty() || member.m_sGroupElementId.IsEmpty()
				|| member.m_sCatalogSlotId.IsEmpty() || member.m_sCatalogSlotId == PROOF_CREW_PREFAB
				|| member.m_sPrefab != PROOF_CREW_MEMBER_PREFAB || member.m_sPrefab == PROOF_CREW_PREFAB
				|| (member.m_sSeatRole != "driver" && member.m_sSeatRole != "passenger"))
				return false;
		}
		return true;
	}

	protected bool ValidateBacklinks(HST_MissionConvoyOperationProofFixture fixture)
	{
		if (!Ready(fixture)
			|| fixture.m_Operation.m_sMissionInstanceId != fixture.m_Mission.m_sInstanceId
			|| fixture.m_Operation.m_sManifestId != fixture.m_Manifest.m_sManifestId
			|| fixture.m_Operation.m_sSpawnResultId != fixture.m_Batch.m_sResultId
			|| fixture.m_Manifest.m_sOperationId != fixture.m_Operation.m_sOperationId
			|| fixture.m_Batch.m_sOperationId != fixture.m_Operation.m_sOperationId
			|| fixture.m_Batch.m_sManifestId != fixture.m_Manifest.m_sManifestId
			|| fixture.m_Batch.m_sManifestHash != fixture.m_Manifest.m_sManifestHash
			|| !fixture.m_Batch.m_bStrategicProjectionHeld)
			return false;
		for (int i = 0; i < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; i++)
		{
			HST_ConvoyElementState element = fixture.m_State.FindConvoyElement(HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, i));
			HST_MissionAssetState asset = fixture.m_State.FindMissionAsset(BuildVehicleAssetId(fixture.m_Mission, i));
			HST_ActiveGroupState group = fixture.m_State.FindActiveGroup(HST_MissionConvoyOperationService.BuildGroupId(fixture.m_Mission, i));
			if (!element || !asset || !group
				|| element.m_sVehicleAssetId != asset.m_sAssetId
				|| element.m_sGroupId != group.m_sGroupId
				|| asset.m_sConvoyElementId != element.m_sElementId
				|| group.m_sConvoyElementId != element.m_sElementId
				|| group.m_sMissionAssetId != asset.m_sAssetId)
				return false;
		}
		return true;
	}

	protected bool ValidateRolledBackAssetLinks(HST_MissionConvoyOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_State || !fixture.m_Mission)
			return false;
		foreach (HST_MissionAssetState asset : fixture.m_State.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != fixture.m_Mission.m_sInstanceId)
				continue;
			if (!asset.m_sOperationId.IsEmpty() || !asset.m_sManifestId.IsEmpty()
				|| !asset.m_sManifestSlotId.IsEmpty() || !asset.m_sAssignedVehicleSlotId.IsEmpty()
				|| !asset.m_sConvoyElementId.IsEmpty())
				return false;
		}
		return true;
	}

	protected int CountFailedAdmissionAuthorityArtifacts(HST_MissionConvoyOperationProofFixture fixture)
	{
		int count;
		if (!fixture || !fixture.m_State || !fixture.m_Mission)
			return -1;
		string operationId = HST_MissionConvoyOperationService.BuildOperationId(fixture.m_Mission);
		string manifestId = HST_MissionConvoyOperationService.BuildManifestId(fixture.m_Mission);
		string batchId = HST_MissionConvoyOperationService.BuildSpawnResultId(fixture.m_Mission);
		foreach (HST_OperationRecordState operation : fixture.m_State.m_aOperations)
		{
			if (operation && (operation.m_sOperationId == operationId || operation.m_sMissionInstanceId == fixture.m_Mission.m_sInstanceId))
				count++;
		}
		foreach (HST_ForceManifestState manifest : fixture.m_State.m_aForceManifests)
		{
			if (manifest && (manifest.m_sManifestId == manifestId || manifest.m_sOperationId == operationId))
				count++;
		}
		foreach (HST_ForceSpawnResultState batch : fixture.m_State.m_aForceSpawnResults)
		{
			if (batch && (batch.m_sResultId == batchId || batch.m_sOperationId == operationId || batch.m_sManifestId == manifestId))
				count++;
		}
		foreach (HST_ConvoyElementState element : fixture.m_State.m_aConvoyElements)
		{
			if (element && (element.m_sOperationId == operationId || element.m_sMissionInstanceId == fixture.m_Mission.m_sInstanceId))
				count++;
		}
		foreach (HST_ActiveGroupState group : fixture.m_State.m_aActiveGroups)
		{
			if (!group || group.m_sMissionInstanceId != fixture.m_Mission.m_sInstanceId)
				continue;
			if (group.m_sGroupId.IndexOf("group_mission_convoy_") == 0
				|| !group.m_sOperationId.IsEmpty() || !group.m_sManifestId.IsEmpty()
				|| !group.m_sSpawnResultId.IsEmpty() || !group.m_sConvoyElementId.IsEmpty())
				count++;
		}
		foreach (HST_MissionAssetState asset : fixture.m_State.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != fixture.m_Mission.m_sInstanceId)
				continue;
			if (!asset.m_sOperationId.IsEmpty() || !asset.m_sManifestId.IsEmpty()
				|| !asset.m_sManifestSlotId.IsEmpty() || !asset.m_sAssignedVehicleSlotId.IsEmpty()
				|| !asset.m_sConvoyElementId.IsEmpty())
				count++;
		}
		if (!fixture.m_Mission.m_sOperationId.IsEmpty() || !fixture.m_Mission.m_sManifestId.IsEmpty()
			|| !fixture.m_Mission.m_sSpawnResultId.IsEmpty() || !fixture.m_Mission.m_sSettlementId.IsEmpty())
			count++;
		return count;
	}

	protected string BuildBatchAuthoritySnapshot(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return "<missing batch>";
		string result = string.Format(
			"batch:%1|%2|%3|%4|%5|%6|%7|%8|%9",
			batch.m_sResultId,
			batch.m_sRequestId,
			batch.m_sManifestId,
			batch.m_sManifestHash,
			batch.m_sOperationId,
			batch.m_sForceId,
			batch.m_sProjectionId,
			batch.m_eStatus,
			batch.m_bStrategicProjectionHeld);
		result = result + string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			batch.m_bCancelRequested,
			batch.m_iExpectedSlotCount,
			batch.m_iSuccessfulHandoffCount,
			batch.m_iCreatedAtSecond,
			batch.m_iUpdatedAtSecond,
			batch.m_iCompletedAtSecond,
			batch.m_iStrategicHoldSinceSecond,
			batch.m_iLifecycleRevision,
			batch.m_sTerminalReason);
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot)
			{
				result = result + "||<missing slot>";
				continue;
			}
			result = result + string.Format(
				"||slot:%1|%2|%3|%4|%5|%6|%7|%8|%9",
				slot.m_sSlotId,
				slot.m_sSlotKind,
				slot.m_sSpawnedPrefab,
				slot.m_sProjectionId,
				slot.m_eStatus,
				slot.m_bAliveVerified,
				slot.m_bEverAlive,
				slot.m_bCasualtyConfirmed,
				slot.m_iCasualtyAtSecond);
			result = result + string.Format(
				"|%1|%2|%3|%4|%5|%6|%7|%8",
				slot.m_sEntityId,
				slot.m_sAssignedVehicleEntityId,
				slot.m_sNativeGroupId,
				slot.m_iAttemptCount,
				slot.m_iUpdatedAtSecond,
				slot.m_iLifecycleRevision,
				slot.m_sRetirementReason,
				slot.m_sFailureReason);
		}
		return result;
	}

	protected bool AreExactMissionGroupsOperational(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;
		for (int ordinal = 0; ordinal < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; ordinal++)
		{
			HST_ActiveGroupState group = state.FindActiveGroup(HST_MissionConvoyOperationService.BuildGroupId(mission, ordinal));
			if (!group || !state.IsOperationalActiveGroup(group))
				return false;
		}
		return true;
	}

	protected bool AreExactMissionGroupsNonOperational(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;
		for (int ordinal = 0; ordinal < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; ordinal++)
		{
			HST_ActiveGroupState group = state.FindActiveGroup(HST_MissionConvoyOperationService.BuildGroupId(mission, ordinal));
			if (!group || state.IsOperationalActiveGroup(group) || state.IsCombatPresentActiveGroup(group))
				return false;
		}
		return true;
	}

	protected bool SeedSettledStaleProjectionFlags(HST_MissionConvoyOperationProofFixture fixture)
	{
		if (!Ready(fixture))
			return false;
		int seeded;
		for (int ordinal = 0; ordinal < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; ordinal++)
		{
			HST_ActiveGroupState group = fixture.m_State.FindActiveGroup(HST_MissionConvoyOperationService.BuildGroupId(fixture.m_Mission, ordinal));
			HST_MissionAssetState asset = fixture.m_State.FindMissionAsset(BuildVehicleAssetId(fixture.m_Mission, ordinal));
			HST_ConvoyElementState element = fixture.m_State.FindConvoyElement(HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, ordinal));
			if (!group || !asset || !element)
				continue;
			group.m_bSpawnedEntity = true;
			group.m_bSpawnAttempted = true;
			group.m_sRuntimeEntityId = "stale_process_handle_" + group.m_sGroupId;
			group.m_iSpawnedAgentCount = Math.Max(1, group.m_iSurvivorInfantryCount);
			group.m_iAssignedWaypointCount = 1;
			asset.m_bSpawned = true;
			element.m_bPhysicalized = true;
			HST_MissionRuntimeEntityState runtimeEntity = fixture.m_State.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (!runtimeEntity)
			{
				runtimeEntity = new HST_MissionRuntimeEntityState();
				runtimeEntity.m_sRuntimeEntityId = asset.m_sEntityId;
				runtimeEntity.m_sMissionInstanceId = fixture.m_Mission.m_sInstanceId;
				runtimeEntity.m_sKind = "vehicle";
				fixture.m_State.m_aMissionRuntimeEntities.Insert(runtimeEntity);
			}
			runtimeEntity.m_bSpawned = true;
			seeded++;
		}
		return seeded == HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT;
	}

	protected bool AreExactMissionProjectionFlagsNormalized(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;
		for (int ordinal = 0; ordinal < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; ordinal++)
		{
			HST_ActiveGroupState group = state.FindActiveGroup(HST_MissionConvoyOperationService.BuildGroupId(mission, ordinal));
			HST_MissionAssetState asset = state.FindMissionAsset(BuildVehicleAssetId(mission, ordinal));
			HST_ConvoyElementState element = state.FindConvoyElement(HST_MissionConvoyOperationService.BuildElementId(mission, ordinal));
			if (!group || !asset || !element || group.m_bSpawnedEntity || group.m_bSpawnAttempted
				|| !group.m_sRuntimeEntityId.IsEmpty() || group.m_iSpawnedAgentCount != 0
				|| group.m_iAssignedWaypointCount != 0 || asset.m_bSpawned || element.m_bPhysicalized)
				return false;
			HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtimeEntity && runtimeEntity.m_bSpawned)
				return false;
		}
		return true;
	}

	protected string BuildGroupAuthoritySnapshot(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "<missing group authority>";
		string result;
		for (int ordinal = 0; ordinal < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; ordinal++)
		{
			HST_ActiveGroupState group = state.FindActiveGroup(HST_MissionConvoyOperationService.BuildGroupId(mission, ordinal));
			if (!group)
			{
				result = result + string.Format("||group:%1:<missing>", ordinal);
				continue;
			}
			result = result + string.Format(
				"||group:%1|%2|%3|%4|%5|%6|%7|%8|%9",
				group.m_sGroupId,
				group.m_sOperationId,
				group.m_sManifestId,
				group.m_sSpawnResultId,
				group.m_sForceId,
				group.m_sProjectionId,
				group.m_sMissionInstanceId,
				group.m_sConvoyElementId,
				group.m_sMissionAssetId);
			result = result + string.Format(
				"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
				group.m_sPrefab,
				group.m_sVehiclePrefab,
				group.m_vPosition,
				group.m_iInfantryCount,
				group.m_iVehicleCount,
				group.m_iSurvivorInfantryCount,
				group.m_iSurvivorVehicleCount,
				group.m_iDurableLivingInfantryCount,
				group.m_sRuntimeStatus);
			result = result + string.Format(
				"|%1|%2|%3|%4|%5",
				group.m_bSpawnAttempted,
				group.m_bSpawnedEntity,
				group.m_bSpawnCompleted,
				group.m_sRuntimeEntityId,
				group.m_iLifecycleRevision);
		}
		return result;
	}

	protected string BuildAssetAuthoritySnapshot(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "<missing asset authority>";
		string result;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			result = result + string.Format(
				"||asset:%1|%2|%3|%4|%5|%6|%7|%8|%9",
				asset.m_sAssetId,
				asset.m_sOperationId,
				asset.m_sManifestId,
				asset.m_sManifestSlotId,
				asset.m_sAssignedVehicleSlotId,
				asset.m_sConvoyElementId,
				asset.m_sKind,
				asset.m_sRole,
				asset.m_sPrefab);
			result = result + string.Format(
				"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
				asset.m_sEntityId,
				asset.m_sCarriedByVehicleId,
				asset.m_bSpawned,
				asset.m_bPickedUp,
				asset.m_bDelivered,
				asset.m_bDestroyed,
				asset.m_bAlive,
				asset.m_bAttachedToCarrier,
				asset.m_bOutcomeApplied);
			result = result + string.Format(
				"|%1|%2|%3|%4",
				asset.m_sOutcomeKind,
				asset.m_vCurrentPosition,
				asset.m_vLastKnownPosition,
				asset.m_sLastInteraction);
		}
		return result;
	}

	protected string BuildSettlementReceiptSnapshot(HST_ActiveMissionState mission, HST_OperationRecordState operation)
	{
		if (!mission || !operation)
			return "<missing settlement receipt>";
		return string.Format(
			"receipt:%1|%2|%3|%4|%5|%6|%7|%8|%9",
			mission.m_sInstanceId,
			mission.m_sOperationId,
			mission.m_sManifestId,
			mission.m_sSpawnResultId,
			mission.m_sSettlementId,
			mission.m_iOperationContractVersion,
			operation.m_sSettlementId,
			operation.m_eSettlementState,
			operation.m_eTerminalResult)
			+ string.Format(
				"|%1|%2|%3|%4|%5|%6",
				operation.m_eDutyState,
				operation.m_eMaterializationState,
				operation.m_ePositionAuthority,
				operation.m_sTerminalReason,
				operation.m_iSettledAtSecond,
				operation.m_iRevision);
	}

	protected int CountMissionGroups(HST_CampaignState state, string missionInstanceId)
	{
		int count;
		if (!state || missionInstanceId.IsEmpty())
			return count;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && group.m_sMissionInstanceId == missionInstanceId)
				count++;
		}
		return count;
	}

	protected int CountMissionAssets(HST_CampaignState state, string missionInstanceId)
	{
		int count;
		if (!state || missionInstanceId.IsEmpty())
			return count;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == missionInstanceId)
				count++;
		}
		return count;
	}

	protected bool Ready(HST_MissionConvoyOperationProofFixture fixture)
	{
		return fixture && fixture.m_State && fixture.m_Preset && fixture.m_PhysicalWar && fixture.m_Service
			&& fixture.m_Mission && fixture.m_Operation && fixture.m_Manifest && fixture.m_Batch
			&& fixture.m_Admission && fixture.m_Admission.m_bSuccess && HasExpectedProofCargo(fixture);
	}

	protected bool Prepared(HST_MissionConvoyOperationProofFixture fixture)
	{
		return fixture && fixture.m_State && fixture.m_Preset && fixture.m_PhysicalWar && fixture.m_Service
			&& fixture.m_Mission && HasExpectedProofCargo(fixture)
			&& fixture.m_State.m_aActiveGroups.Count() == HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT;
	}

	protected bool HasExpectedProofCargo(HST_MissionConvoyOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_Mission)
			return false;
		if (fixture.m_Mission.m_sMissionId == "convoy_money" || fixture.m_Mission.m_sMissionId == "convoy_supplies")
			return fixture.m_Cargo && fixture.m_Cargo.m_sKind == HST_MissionConvoyOperationService.CARGO_KIND
				&& fixture.m_Cargo.m_sRole == HST_MissionConvoyOperationService.PAYLOAD_ROLE;
		if (fixture.m_Mission.m_sMissionId == "convoy_prisoners")
			return fixture.m_Cargo && fixture.m_Cargo.m_sKind == HST_MissionConvoyOperationService.CAPTIVE_KIND
				&& fixture.m_Cargo.m_sRole == HST_MissionConvoyOperationService.CAPTIVE_ROLE;
		return fixture.m_Cargo == null;
	}

	protected string BuildFixtureFailure(HST_MissionConvoyOperationProofFixture fixture)
	{
		if (!fixture)
			return "mission convoy proof fixture was not created";
		if (!fixture.m_sFailureReason.IsEmpty())
			return fixture.m_sFailureReason;
		return "mission convoy proof fixture is incomplete";
	}

	protected string BuildPairFailure(HST_MissionConvoyOperationProofFixture first, HST_MissionConvoyOperationProofFixture second)
	{
		return BuildFixtureFailure(first) + " | " + BuildFixtureFailure(second);
	}

	protected string BuildVehicleAssetId(HST_ActiveMissionState mission, int ordinal)
	{
		if (!mission)
			return "";
		return string.Format("asset_%1_%2_%3", mission.m_sInstanceId, HST_MissionConvoyOperationService.VEHICLE_ROLE, ordinal);
	}

	protected int CountOperations(HST_CampaignState state, string operationId)
	{
		int count;
		if (!state)
			return count;
		foreach (HST_OperationRecordState row : state.m_aOperations)
		{
			if (row && row.m_sOperationId == operationId)
				count++;
		}
		return count;
	}

	protected int CountManifests(HST_CampaignState state, string manifestId)
	{
		int count;
		if (!state)
			return count;
		foreach (HST_ForceManifestState row : state.m_aForceManifests)
		{
			if (row && row.m_sManifestId == manifestId)
				count++;
		}
		return count;
	}

	protected int CountBatches(HST_CampaignState state, string resultId)
	{
		int count;
		if (!state)
			return count;
		foreach (HST_ForceSpawnResultState row : state.m_aForceSpawnResults)
		{
			if (row && row.m_sResultId == resultId)
				count++;
		}
		return count;
	}

	protected int CountElements(HST_CampaignState state, string operationId)
	{
		int count;
		if (!state)
			return count;
		foreach (HST_ConvoyElementState row : state.m_aConvoyElements)
		{
			if (row && row.m_sOperationId == operationId)
				count++;
		}
		return count;
	}

	protected int CountElementSurvivors(HST_MissionConvoyOperationProofFixture fixture)
	{
		int count;
		if (!Ready(fixture))
			return count;
		foreach (HST_ConvoyElementState element : fixture.m_State.m_aConvoyElements)
		{
			if (element && element.m_sOperationId == fixture.m_Operation.m_sOperationId)
				count += Math.Max(0, element.m_iSurvivingCrewCount);
		}
		return count;
	}

	protected int CountPhysicalizedElements(HST_MissionConvoyOperationProofFixture fixture)
	{
		int count;
		if (!Ready(fixture))
			return count;
		foreach (HST_ConvoyElementState element : fixture.m_State.m_aConvoyElements)
		{
			if (element && element.m_sOperationId == fixture.m_Operation.m_sOperationId && element.m_bPhysicalized)
				count++;
		}
		return count;
	}

	protected int CountElementsWithDisposition(HST_MissionConvoyOperationProofFixture fixture, HST_EConvoyElementDisposition disposition)
	{
		int count;
		if (!Ready(fixture))
			return count;
		foreach (HST_ConvoyElementState element : fixture.m_State.m_aConvoyElements)
		{
			if (element && element.m_sOperationId == fixture.m_Operation.m_sOperationId && element.m_eDisposition == disposition)
				count++;
		}
		return count;
	}

	protected int CountLivingMemberSlots(HST_ForceSpawnResultState batch)
	{
		int count;
		if (!batch)
			return count;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotKind == "member" && slot.m_bEverAlive && !slot.m_bCasualtyConfirmed)
				count++;
		}
		return count;
	}

	protected bool AllNonCasualtySlotsRetired(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return false;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && !slot.m_bCasualtyConfirmed && slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED)
				return false;
		}
		return true;
	}

	protected bool EliminateExactCrewAuthority(HST_MissionConvoyOperationProofFixture fixture)
	{
		if (!Ready(fixture))
			return false;
		foreach (HST_ForceSpawnSlotResultState slot : fixture.m_Batch.m_aSlotResults)
		{
			if (!slot || slot.m_sSlotKind != "member")
				continue;
			slot.m_bCasualtyConfirmed = true;
			slot.m_bAliveVerified = false;
			slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
			slot.m_iCasualtyAtSecond = fixture.m_State.m_iElapsedSeconds;
			slot.m_iUpdatedAtSecond = fixture.m_State.m_iElapsedSeconds;
			slot.m_sRetirementReason = "mission convoy proof full crew elimination";
			slot.m_iLifecycleRevision++;
		}
		for (int index = 0; index < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; index++)
		{
			HST_ConvoyElementState element = fixture.m_State.FindConvoyElement(HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, index));
			HST_ActiveGroupState group = fixture.m_State.FindActiveGroup(HST_MissionConvoyOperationService.BuildGroupId(fixture.m_Mission, index));
			if (!element || !group)
				return false;
			element.m_iSurvivingCrewCount = 0;
			element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED;
			element.m_bMobile = false;
			element.m_sTerminalReason = "mission convoy proof full crew elimination";
			element.m_iRevision++;
			group.m_iInfantryCount = 0;
			group.m_iSurvivorInfantryCount = 0;
			group.m_iLastSeenAliveCount = 0;
			group.m_iDurableLivingInfantryCount = 0;
			group.m_sRuntimeStatus = HST_MissionConvoyOperationService.CONVOY_ELIMINATED;
			group.m_iLifecycleRevision++;
		}
		return CountLivingMemberSlots(fixture.m_Batch) == 0 && CountElementSurvivors(fixture) == 0;
	}

	protected bool EliminateExactCrewAuthorityForElement(HST_MissionConvoyOperationProofFixture fixture, int elementIndex)
	{
		if (!Ready(fixture) || elementIndex < 0 || elementIndex >= HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT)
			return false;
		for (int seatIndex = 0; seatIndex < PROOF_CREW_PER_VEHICLE; seatIndex++)
		{
			HST_ForceSpawnSlotResultState slot = fixture.m_Batch.FindSlotResult(
				HST_MissionConvoyOperationService.BuildMemberSlotId(fixture.m_Mission, elementIndex, seatIndex));
			if (!slot)
				return false;
			slot.m_bCasualtyConfirmed = true;
			slot.m_bAliveVerified = false;
			slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
			slot.m_iCasualtyAtSecond = fixture.m_State.m_iElapsedSeconds;
			slot.m_iUpdatedAtSecond = fixture.m_State.m_iElapsedSeconds;
			slot.m_sRetirementReason = "mission convoy proof partial crew elimination";
			slot.m_iLifecycleRevision++;
		}
		HST_ConvoyElementState element = fixture.m_State.FindConvoyElement(
			HST_MissionConvoyOperationService.BuildElementId(fixture.m_Mission, elementIndex));
		HST_ActiveGroupState group = fixture.m_State.FindActiveGroup(
			HST_MissionConvoyOperationService.BuildGroupId(fixture.m_Mission, elementIndex));
		if (!element || !group)
			return false;
		element.m_iSurvivingCrewCount = 0;
		element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED;
		element.m_bMobile = false;
		element.m_sTerminalReason = "mission convoy proof partial crew elimination";
		element.m_iRevision++;
		group.m_iInfantryCount = 0;
		group.m_iSurvivorInfantryCount = 0;
		group.m_iLastSeenAliveCount = 0;
		group.m_iDurableLivingInfantryCount = 0;
		group.m_sRuntimeStatus = HST_MissionConvoyOperationService.CONVOY_ELIMINATED;
		group.m_iLifecycleRevision++;
		return HST_MissionConvoyOperationService.IsRecoverableAbandonedVehicleRoot(
			fixture.m_State.FindMissionAsset(BuildVehicleAssetId(fixture.m_Mission, elementIndex)),
			element);
	}

	protected bool ValidateRestoredRecoveryElements(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;
		for (int index = 0; index < HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT; index++)
		{
			HST_ConvoyElementState element = state.FindConvoyElement(HST_MissionConvoyOperationService.BuildElementId(mission, index));
			HST_MissionAssetState asset = state.FindMissionAsset(BuildVehicleAssetId(mission, index));
			HST_ActiveGroupState group = state.FindActiveGroup(HST_MissionConvoyOperationService.BuildGroupId(mission, index));
			if (!element || !asset || !group)
				return false;
			if (element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED
				|| element.m_iSurvivingCrewCount != 0 || element.m_bPhysicalized || element.m_bMobile)
				return false;
			if (asset.m_bSpawned || asset.m_bDestroyed || asset.m_bDelivered || asset.m_sLastInteraction == "captured")
				return false;
			if (group.m_bSpawnedEntity || group.m_iInfantryCount != 0 || group.m_iSurvivorInfantryCount != 0
				|| group.m_iLastSeenAliveCount != 0 || group.m_iDurableLivingInfantryCount != 0
				|| (group.m_sRuntimeStatus != HST_MissionConvoyOperationService.CONVOY_ELIMINATED && group.m_sRuntimeStatus != "eliminated"))
				return false;
		}
		return true;
	}

	protected HST_ForceManifestState FindSavedManifest(HST_CampaignSaveData saveData, string manifestId)
	{
		if (!saveData || manifestId.IsEmpty())
			return null;
		foreach (HST_ForceManifestState manifest : saveData.m_aForceManifests)
		{
			if (manifest && manifest.m_sManifestId == manifestId)
				return manifest;
		}
		return null;
	}

	protected HST_ActiveMissionState FindSavedMission(HST_CampaignSaveData saveData, string missionInstanceId)
	{
		if (!saveData || missionInstanceId.IsEmpty())
			return null;
		foreach (HST_ActiveMissionState mission : saveData.m_aActiveMissions)
		{
			if (mission && mission.m_sInstanceId == missionInstanceId)
				return mission;
		}
		return null;
	}

	protected HST_ForceSpawnResultState FindSavedBatch(HST_CampaignSaveData saveData, string resultId)
	{
		if (!saveData || resultId.IsEmpty())
			return null;
		foreach (HST_ForceSpawnResultState batch : saveData.m_aForceSpawnResults)
		{
			if (batch && batch.m_sResultId == resultId)
				return batch;
		}
		return null;
	}

	protected HST_MissionAssetState FindSavedMissionAsset(HST_CampaignSaveData saveData, string assetId)
	{
		if (!saveData || assetId.IsEmpty())
			return null;
		foreach (HST_MissionAssetState asset : saveData.m_aMissionAssets)
		{
			if (asset && asset.m_sAssetId == assetId)
				return asset;
		}
		return null;
	}

	protected HST_ConvoyElementState FindSavedConvoyElement(HST_CampaignSaveData saveData, string elementId)
	{
		if (!saveData || elementId.IsEmpty())
			return null;
		foreach (HST_ConvoyElementState element : saveData.m_aConvoyElements)
		{
			if (element && element.m_sElementId == elementId)
				return element;
		}
		return null;
	}

	protected int CountMissionMarkers(HST_CampaignState state, string missionInstanceId)
	{
		int count;
		if (!state || missionInstanceId.IsEmpty())
			return count;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (marker && !marker.m_bTombstone && marker.m_bVisible && marker.m_sLinkedId == missionInstanceId)
				count++;
		}
		return count;
	}

	protected int CountMarkerId(HST_CampaignState state, string markerId)
	{
		int count;
		if (!state || markerId.IsEmpty())
			return count;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (marker && !marker.m_bTombstone && marker.m_bVisible && marker.m_sMarkerId == markerId)
				count++;
		}
		return count;
	}

	protected int CountMarkerIdPrefix(HST_CampaignState state, string markerIdPrefix)
	{
		int count;
		if (!state || markerIdPrefix.IsEmpty())
			return count;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (marker && !marker.m_bTombstone && marker.m_bVisible && marker.m_sMarkerId.IndexOf(markerIdPrefix) == 0)
				count++;
		}
		return count;
	}

	protected int CountLiveMarkers(HST_CampaignState state)
	{
		int count;
		if (!state)
			return count;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (marker && !marker.m_bTombstone && marker.m_bVisible)
				count++;
		}
		return count;
	}

	protected int CountMobileElements(HST_MissionConvoyOperationProofFixture fixture)
	{
		int count;
		if (!Ready(fixture))
			return count;
		foreach (HST_ConvoyElementState element : fixture.m_State.m_aConvoyElements)
		{
			if (element && element.m_sOperationId == fixture.m_Operation.m_sOperationId && element.m_bMobile)
				count++;
		}
		return count;
	}

	protected int SumElementRevisions(HST_MissionConvoyOperationProofFixture fixture)
	{
		if (!Ready(fixture))
			return 0;
		return SumElementRevisionsForOperation(fixture.m_State, fixture.m_Operation.m_sOperationId);
	}

	protected int SumElementRevisionsForOperation(HST_CampaignState state, string operationId)
	{
		int result;
		if (!state || operationId.IsEmpty())
			return result;
		foreach (HST_ConvoyElementState element : state.m_aConvoyElements)
		{
			if (element && element.m_sOperationId == operationId)
				result += element.m_iRevision;
		}
		return result;
	}

	protected vector BuildProofPosition(float x, float z)
	{
		vector result;
		result[0] = x;
		result[1] = 0.0;
		result[2] = z;
		return result;
	}

	protected float Distance2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return Math.Sqrt(x * x + z * z);
	}
}
#endif
