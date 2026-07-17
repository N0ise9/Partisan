class HST_PersistentFieldVehicleProofObservation
{
	int m_iDurableRows;
	int m_iLiveRoots;
	int m_iDeletedRows;
	int m_iCargoRows;
	bool m_bStateExact;
	bool m_bPhysicalExact;
	bool m_bCargoExact;
	bool m_bNoDuplicateRoots;
	string m_sEvidence;

	bool AllExact()
	{
		return m_bStateExact && m_bPhysicalExact
			&& m_bCargoExact && m_bNoDuplicateRoots;
	}
}

// Guarded world fixture used only by the disposable ordinary persistence
// runner. It exercises the production tracker, capture preflight, physical
// restore, natural damage transition, and stable campaign IDs. No entity or RPL
// identity is serialized into its carrier/result artifacts.
class HST_PersistentFieldVehicleRestartProofService
{
	static const string FIELD_VEHICLE_PREFAB
		= "{4AE9D080927D3CB9}Prefabs/Vehicles/Wheeled/S1203/S1203_base.et";
	static const string FIELD_VEHICLE_CARGO_PREFAB
		= "{6985327711303720}Prefabs/Objects/HST/HST_MissionProp_Cargo.et";
	static const int FIELD_VEHICLE_A_CARGO_COUNT = 3;
	static const int FIELD_VEHICLE_B_CARGO_COUNT = 7;
	static const string PHASE_PREPARE = "prepare";
	static const string PHASE_RECOVER_MUTATE = "recover_mutate";
	static const string PHASE_REPLAY_SHUTDOWN = "replay_shutdown";
	static const string PHASE_REPLAY_NATIVE = "replay_native";
	static const string PHASE_REPLAY_FALLBACK = "replay_fallback";

	static const int STABLE_SAMPLE_COUNT = 3;
	protected static const float STAGE_TIMEOUT_SECONDS = 45.0;
	protected static const float ROOT_QUERY_RADIUS_METERS = 8.0;
	protected static const float POSITION_TOLERANCE_METERS = 3.0;
	protected static const int MAX_QUERY_ENTITIES = 256;

	protected ref array<IEntity> m_aQueryEntities = {};
	protected bool m_bFixturePrepared;
	protected bool m_bMutationIssued;
	protected int m_iStableSamples;
	protected int m_iPendingObservationFrames;
	protected float m_fStageElapsedSeconds;
	protected string m_sStage;
	protected IEntity m_DestroyedFixtureRoot;

	static string BuildFieldVehicleRuntimeId(
		string payloadNonce,
		string suffix)
	{
		if (!HST_OrdinaryCampaignPersistenceProofService.ValidateNonce(
			payloadNonce)
			|| (suffix != "a" && suffix != "b"))
			return "";
		return "hst_pfv_" + payloadNonce + "_" + suffix;
	}

	bool TickStagePreparation(
		string stage,
		HST_CampaignState state,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		HST_PersistentFieldVehicleRuntimeService tracker,
		HST_PersistentFieldVehicleRestoreResult restoreResult,
		float timeSlice,
		out bool ready,
		out string evidence)
	{
		ready = false;
		evidence = "field vehicle stage preparation unavailable";
		if (!state || !carrier || !tracker || !restoreResult
			|| !HST_OrdinaryCampaignPersistenceProofService.ValidateStage(stage))
			return false;
		if (m_sStage.IsEmpty())
			m_sStage = stage;
		if (m_sStage != stage)
		{
			evidence = "field vehicle proof service received a second stage";
			return false;
		}
		m_fStageElapsedSeconds += Math.Max(0.0, timeSlice);
		if (m_fStageElapsedSeconds > STAGE_TIMEOUT_SECONDS)
		{
			evidence = "field vehicle stage preparation timed out: " + evidence;
			return false;
		}

		if (stage == HST_OrdinaryCampaignPersistenceProofService
			.STAGE_AUTOSAVE_CHECKPOINT)
		{
			if (!m_bFixturePrepared)
			{
				if (!PrepareInitialFixture(state, carrier, tracker, evidence))
					return false;
				m_bFixturePrepared = true;
				m_iStableSamples = 0;
			}
			HST_PersistentFieldVehicleProofObservation observation;
			if (!ValidateInitialPhysicalSnapshot(
				state,
				carrier,
				tracker,
				observation))
			{
				m_iStableSamples = 0;
				evidence = observation.m_sEvidence;
				ReportPendingObservation(evidence);
				return true;
			}
			if (!AcceptStableSample())
			{
				evidence = string.Format(
					"field vehicle prepare settling %1/%2 | %3",
					m_iStableSamples,
					STABLE_SAMPLE_COUNT,
					observation.m_sEvidence);
				return true;
			}
			carrier.m_bFieldVehiclePrepared = true;
			ready = true;
			evidence = "field vehicle prepare exact | "
				+ observation.m_sEvidence;
			return true;
		}

		if (stage == HST_OrdinaryCampaignPersistenceProofService
			.STAGE_MANUAL_CHECKPOINT)
		{
			if (!ValidateRestoreReceipt(restoreResult, 2, 0, evidence))
				return false;
			if (!m_bMutationIssued)
			{
				HST_PersistentFieldVehicleProofObservation recovered;
				if (!ValidateInitialPhysicalSnapshot(
					state,
					carrier,
					tracker,
					recovered))
				{
					m_iStableSamples = 0;
					evidence = recovered.m_sEvidence;
					ReportPendingObservation(evidence);
					return true;
				}
				if (!AcceptStableSample())
				{
					evidence = string.Format(
						"field vehicle recovery settling %1/%2 | %3",
						m_iStableSamples,
						STABLE_SAMPLE_COUNT,
						recovered.m_sEvidence);
					return true;
				}
				if (!BeginRecoveredMutation(
					state,
					carrier,
					tracker,
					evidence))
					return false;
				m_bMutationIssued = true;
				m_iStableSamples = 0;
				return true;
			}

			HST_PersistentFieldVehicleProofObservation mutated;
			if (!ValidateMutatedPhysicalSnapshot(
				state,
				carrier,
				tracker,
				mutated))
			{
				m_iStableSamples = 0;
				evidence = mutated.m_sEvidence;
				ReportPendingObservation(evidence);
				return true;
			}
			if (!AcceptStableSample())
			{
				evidence = string.Format(
					"field vehicle move/damage settling %1/%2",
					m_iStableSamples,
					STABLE_SAMPLE_COUNT);
				return true;
			}
			carrier.m_bFieldVehicleRecoveredAndMutated = true;
			ready = true;
			evidence = "field vehicle native recovery, movement, and natural destruction exact";
			return true;
		}

		if (!ValidateRestoreReceipt(restoreResult, 1, 1, evidence))
			return false;
		int replayGeneration = 2;
		if (stage == HST_OrdinaryCampaignPersistenceProofService
				.STAGE_NATIVE_SHUTDOWN_VERIFY
			|| stage == HST_OrdinaryCampaignPersistenceProofService
				.STAGE_PROFILE_FALLBACK_VERIFY)
			replayGeneration = 3;
		HST_PersistentFieldVehicleProofObservation replay;
		if (!ValidateReplayPhysicalSnapshot(
			state,
			carrier,
			tracker,
			replayGeneration,
			replay))
		{
			m_iStableSamples = 0;
			evidence = replay.m_sEvidence;
			ReportPendingObservation(evidence);
			return true;
		}
		if (!AcceptStableSample())
		{
			evidence = string.Format(
				"field vehicle replay settling %1/%2 | %3",
				m_iStableSamples,
				STABLE_SAMPLE_COUNT,
				replay.m_sEvidence);
			return true;
		}
		if (stage == HST_OrdinaryCampaignPersistenceProofService
			.STAGE_SHUTDOWN_CHECKPOINT)
			carrier.m_bFieldVehicleReplayVerified = true;
		ready = true;
		evidence = "field vehicle replay exact | " + replay.m_sEvidence;
		return true;
	}

	bool PopulateStageResult(
		string stage,
		HST_CampaignState state,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		HST_PersistentFieldVehicleRuntimeService tracker,
		HST_PersistentFieldVehicleRestoreResult restoreResult,
		HST_OrdinaryCampaignPersistenceResult result,
		out string evidence)
	{
		evidence = "field vehicle stage result unavailable";
		if (!state || !carrier || !tracker || !restoreResult || !result)
			return false;
		int generation = 2;
		int expectedLiveRoots = 1;
		int expectedDeletedRows = 1;
		if (stage == HST_OrdinaryCampaignPersistenceProofService
			.STAGE_AUTOSAVE_CHECKPOINT)
		{
			generation = 1;
			expectedLiveRoots = 2;
			expectedDeletedRows = 0;
			result.m_sFieldVehicleProofPhase = PHASE_PREPARE;
		}
		else if (stage == HST_OrdinaryCampaignPersistenceProofService
			.STAGE_MANUAL_CHECKPOINT)
			result.m_sFieldVehicleProofPhase = PHASE_RECOVER_MUTATE;
		else if (stage == HST_OrdinaryCampaignPersistenceProofService
			.STAGE_SHUTDOWN_CHECKPOINT)
		{
			generation = 3;
			result.m_sFieldVehicleProofPhase = PHASE_REPLAY_SHUTDOWN;
		}
		else if (stage == HST_OrdinaryCampaignPersistenceProofService
			.STAGE_NATIVE_SHUTDOWN_VERIFY)
		{
			generation = 3;
			result.m_sFieldVehicleProofPhase = PHASE_REPLAY_NATIVE;
		}
		else if (stage == HST_OrdinaryCampaignPersistenceProofService
			.STAGE_PROFILE_FALLBACK_VERIFY)
		{
			generation = 3;
			result.m_sFieldVehicleProofPhase = PHASE_REPLAY_FALLBACK;
		}
		else
			return false;

		if (generation == 1
			&& !FreezeInitialCapturedTransforms(state, carrier, evidence))
			return false;
		HST_PersistentFieldVehicleProofObservation observation;
		ObserveFixture(state, carrier, tracker, generation, observation);
		result.m_iFieldVehicleExpectedDurableRows = 2;
		result.m_iFieldVehicleObservedDurableRows = observation.m_iDurableRows;
		result.m_iFieldVehicleExpectedLiveRoots = expectedLiveRoots;
		result.m_iFieldVehicleObservedLiveRoots = observation.m_iLiveRoots;
		result.m_iFieldVehicleExpectedDeletedRows = expectedDeletedRows;
		result.m_iFieldVehicleObservedDeletedRows = observation.m_iDeletedRows;
		result.m_iFieldVehicleExpectedCargoRows = 2;
		result.m_iFieldVehicleObservedCargoRows = observation.m_iCargoRows;
		result.m_iFieldVehicleRestoreEligibleRows
			= restoreResult.m_iEligibleRows;
		result.m_iFieldVehicleRestoreInactiveRows
			= restoreResult.m_iInactiveRows;
		result.m_iFieldVehicleRetiredNativeTombstoneRoots
			= restoreResult.m_iRetiredNativeTombstoneRoots;
		result.m_iFieldVehicleRestoreAdoptedRoots
			= restoreResult.m_iAdoptedRoots;
		result.m_iFieldVehicleRestoreSpawnedRoots
			= restoreResult.m_iSpawnedRoots;
		result.m_iFieldVehicleRestoreTrackedRoots
			= restoreResult.m_iTrackedRoots;
		result.m_iFieldVehicleRestoreFailedRows
			= restoreResult.m_iFailedRows;
		result.m_iFieldVehicleRestoreAmbiguousRows
			= restoreResult.m_iAmbiguousRows;
		result.m_iFieldVehicleNativeTrackedRoots
			= tracker.CountNativeTrackedDurableRoots(state);
		result.m_iFieldVehicleShutdownQuiescedRoots
			= tracker.GetControlledShutdownQuiescedRootCount();
		result.m_bFieldVehicleRestoreExact = restoreResult.AllExact();
		result.m_bFieldVehicleStateExact = observation.m_bStateExact;
		result.m_bFieldVehiclePhysicalExact = observation.m_bPhysicalExact;
		result.m_bFieldVehicleCargoExact = observation.m_bCargoExact;
		result.m_bFieldVehicleNoDuplicateRoots
			= observation.m_bNoDuplicateRoots;
		result.m_bFieldVehicleNativeAuthorityDetached
			= result.m_iFieldVehicleNativeTrackedRoots == 0;
		result.m_bFieldVehicleShutdownQuiescenceRequired
			= stage == HST_OrdinaryCampaignPersistenceProofService
				.STAGE_SHUTDOWN_CHECKPOINT;
		if (result.m_bFieldVehicleShutdownQuiescenceRequired)
		{
			result.m_bFieldVehicleShutdownQuiescenceExact
				= tracker.IsControlledShutdownQuiescenceExact(state)
				&& result.m_iFieldVehicleShutdownQuiescedRoots
					== expectedLiveRoots;
		}
		else
		{
			result.m_bFieldVehicleShutdownQuiescenceExact
				= !tracker.IsControlledShutdownQuiescenceExact(state)
				&& result.m_iFieldVehicleShutdownQuiescedRoots == 0;
		}
		result.m_bFieldVehicleMutationApplied
			= stage == HST_OrdinaryCampaignPersistenceProofService
				.STAGE_MANUAL_CHECKPOINT;
		bool progressExact = carrier.m_bFieldVehiclePrepared;
		if (generation >= 2)
			progressExact = progressExact
				&& carrier.m_bFieldVehicleRecoveredAndMutated;
		if (stage != HST_OrdinaryCampaignPersistenceProofService
			.STAGE_MANUAL_CHECKPOINT && generation >= 2)
			progressExact = progressExact
				&& carrier.m_bFieldVehicleReplayVerified;
		result.m_bFieldVehicleProofExact = observation.AllExact()
			&& result.m_bFieldVehicleRestoreExact
			&& result.m_bFieldVehicleNativeAuthorityDetached
			&& result.m_bFieldVehicleShutdownQuiescenceExact
			&& progressExact;
		result.m_sFieldVehicleEvidence = restoreResult.BuildReport()
			+ " | " + observation.m_sEvidence
			+ string.Format(
				" | native tracked/shutdown quiesced/required/exact %1/%2/%3/%4",
				result.m_iFieldVehicleNativeTrackedRoots,
				result.m_iFieldVehicleShutdownQuiescedRoots,
				result.m_bFieldVehicleShutdownQuiescenceRequired,
				result.m_bFieldVehicleShutdownQuiescenceExact);
		evidence = result.m_sFieldVehicleEvidence;
		return result.m_bFieldVehicleProofExact;
	}

	bool FreezeShutdownCapturedTransform(
		HST_CampaignSaveData snapshot,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		out string evidence)
	{
		evidence = "shutdown field vehicle captured transform rejected";
		if (!snapshot || !carrier || !carrier.m_bFieldVehicleReplayVerified
			|| carrier.m_iCurrentSentinelGeneration != 3
			|| !IsZeroVector(carrier.m_vFieldVehicleAShutdownPosition)
			|| !IsZeroVector(carrier.m_vFieldVehicleAShutdownAngles))
			return false;
		HST_RuntimeVehicleState recordA;
		HST_RuntimeVehicleState recordB;
		int countA;
		int countB;
		foreach (HST_RuntimeVehicleState record : snapshot.m_aRuntimeVehicles)
		{
			if (!record)
				continue;
			if (record.m_sVehicleRuntimeId == carrier.m_sFieldVehicleAId)
			{
				countA++;
				recordA = record;
			}
			else if (record.m_sVehicleRuntimeId
				== carrier.m_sFieldVehicleBId)
			{
				countB++;
				recordB = record;
			}
		}
		if (countA != 1 || countB != 1 || !recordA || !recordB
			|| recordA.m_sPrefab != carrier.m_sFieldVehiclePrefab
			|| recordB.m_sPrefab != carrier.m_sFieldVehiclePrefab
			|| recordA.m_sRuntimeKind != "field_vehicle"
			|| recordB.m_sRuntimeKind != "field_vehicle"
			|| recordA.m_bDeleted || recordA.m_bDetached
			|| !recordB.m_bDeleted || recordB.m_bDetached
			|| IsZeroVector(recordA.m_vPosition))
			return false;

		carrier.m_vFieldVehicleAShutdownPosition = recordA.m_vPosition;
		carrier.m_vFieldVehicleAShutdownAngles = recordA.m_vAngles;
		evidence = "shutdown field vehicle expected transform frozen from the detached checkpoint snapshot";
		return true;
	}

	bool ValidateLogicalSnapshot(
		HST_CampaignState state,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		int generation,
		out string evidence)
	{
		HST_PersistentFieldVehicleProofObservation observation;
		bool exact = ObserveLogicalFixture(
			state,
			carrier,
			generation,
			observation);
		evidence = observation.m_sEvidence;
		int expectedDeletedRows = 1;
		if (generation == 1)
			expectedDeletedRows = 0;
		return exact && observation.m_iDurableRows == 2
			&& observation.m_iCargoRows == 2
			&& observation.m_iDeletedRows == expectedDeletedRows;
	}

	protected bool PrepareInitialFixture(
		HST_CampaignState state,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		HST_PersistentFieldVehicleRuntimeService tracker,
		out string evidence)
	{
		evidence = "field vehicle fixture preparation rejected";
		carrier.m_sFieldVehiclePrefab = FIELD_VEHICLE_PREFAB;
		carrier.m_sFieldVehicleCargoPrefab = FIELD_VEHICLE_CARGO_PREFAB;
		carrier.m_sFieldVehicleAId
			= BuildFieldVehicleRuntimeId(carrier.m_sPayloadNonce, "a");
		carrier.m_sFieldVehicleBId
			= BuildFieldVehicleRuntimeId(carrier.m_sPayloadNonce, "b");
		carrier.m_iFieldVehicleACargoCount = FIELD_VEHICLE_A_CARGO_COUNT;
		carrier.m_iFieldVehicleBCargoCount = FIELD_VEHICLE_B_CARGO_COUNT;
		carrier.m_vFieldVehicleAInitialAngles = "15 0 0";
		carrier.m_vFieldVehicleBInitialAngles = "195 0 0";
		carrier.m_vFieldVehicleAMovedAngles = "75 0 0";
		if (carrier.m_sFieldVehicleAId.IsEmpty()
			|| carrier.m_sFieldVehicleBId.IsEmpty()
			|| HasFixtureResidue(state, carrier))
			return false;
		if (!ResolveFixturePositions(carrier, evidence))
			return false;

		GenericEntity rootA = HST_WorldPositionService.SpawnPrefab(
			FIELD_VEHICLE_PREFAB,
			carrier.m_vFieldVehicleAInitialPosition,
			carrier.m_vFieldVehicleAInitialAngles);
		GenericEntity rootB = HST_WorldPositionService.SpawnPrefab(
			FIELD_VEHICLE_PREFAB,
			carrier.m_vFieldVehicleBInitialPosition,
			carrier.m_vFieldVehicleBInitialAngles);
		if (!rootA || !rootB)
		{
			if (rootA)
				SCR_EntityHelper.DeleteEntityAndChildren(rootA);
			if (rootB)
				SCR_EntityHelper.DeleteEntityAndChildren(rootB);
			evidence = "field vehicle fixture root spawn failed";
			return false;
		}
		HST_WorldPositionService.ApplyUprightEntityTransform(
			rootA,
			carrier.m_vFieldVehicleAInitialPosition,
			carrier.m_vFieldVehicleAInitialAngles);
		HST_WorldPositionService.ApplyUprightEntityTransform(
			rootB,
			carrier.m_vFieldVehicleBInitialPosition,
			carrier.m_vFieldVehicleBInitialAngles);
		HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(rootA);
		HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(rootB);

		HST_RuntimeVehicleState recordA = BuildRuntimeRecord(
			carrier.m_sFieldVehicleAId,
			rootA.GetOrigin(),
			carrier.m_vFieldVehicleAInitialAngles,
			"Proof Field Vehicle A");
		HST_RuntimeVehicleState recordB = BuildRuntimeRecord(
			carrier.m_sFieldVehicleBId,
			rootB.GetOrigin(),
			carrier.m_vFieldVehicleBInitialAngles,
			"Proof Field Vehicle B");
		state.m_aRuntimeVehicles.Insert(recordA);
		state.m_aRuntimeVehicles.Insert(recordB);
		state.m_aVehicleCargoItems.Insert(BuildCargoRecord(
			carrier.m_sFieldVehicleAId,
			rootA.GetOrigin(),
			FIELD_VEHICLE_A_CARGO_COUNT,
			"Proof Cargo A"));
		state.m_aVehicleCargoItems.Insert(BuildCargoRecord(
			carrier.m_sFieldVehicleBId,
			rootB.GetOrigin(),
			FIELD_VEHICLE_B_CARGO_COUNT,
			"Proof Cargo B"));
		if (!tracker.Track(rootA, recordA) || !tracker.Track(rootB, recordB))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(rootA);
			SCR_EntityHelper.DeleteEntityAndChildren(rootB);
			evidence = "field vehicle fixture stable-ID tracking failed";
			return false;
		}
		carrier.m_vFieldVehicleAInitialPosition = rootA.GetOrigin();
		carrier.m_vFieldVehicleBInitialPosition = rootB.GetOrigin();
		evidence = "field vehicle physical fixture created and tracked";
		return true;
	}

	protected HST_RuntimeVehicleState BuildRuntimeRecord(
		string runtimeId,
		vector position,
		vector angles,
		string displayName)
	{
		HST_RuntimeVehicleState record = new HST_RuntimeVehicleState();
		record.m_sVehicleRuntimeId = runtimeId;
		record.m_sPrefab = FIELD_VEHICLE_PREFAB;
		record.m_sDisplayName = displayName;
		record.m_sRuntimeKind = "field_vehicle";
		record.m_sSourceVehicleKind = "transport";
		record.m_vPosition = position;
		record.m_vAngles = angles;
		record.m_bDetached = false;
		record.m_bDeleted = false;
		HST_VehicleCapabilityPolicy.ApplyToRuntimeVehicle(record);
		HST_VehicleCapabilityPolicy.ApplyUndercoverToRuntimeVehicle(record);
		return record;
	}

	protected HST_VehicleCargoItemState BuildCargoRecord(
		string runtimeId,
		vector position,
		int count,
		string displayName)
	{
		HST_VehicleCargoItemState cargo = new HST_VehicleCargoItemState();
		cargo.m_sVehicleRuntimeId = runtimeId;
		cargo.m_sVehiclePrefab = FIELD_VEHICLE_PREFAB;
		cargo.m_sVehicleDisplayName = "Proof Field Vehicle";
		cargo.m_sItemPrefab = FIELD_VEHICLE_CARGO_PREFAB;
		cargo.m_sDisplayName = displayName;
		cargo.m_sCategory = "proof";
		cargo.m_iCount = count;
		cargo.m_vLastVehiclePosition = position;
		return cargo;
	}

	bool FreezeInitialCapturedTransforms(
		HST_CampaignState state,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		out string evidence)
	{
		evidence = "field vehicle captured transform freeze rejected";
		HST_RuntimeVehicleState recordA = FindUniqueFixtureRecord(
			state,
			carrier.m_sFieldVehicleAId);
		HST_RuntimeVehicleState recordB = FindUniqueFixtureRecord(
			state,
			carrier.m_sFieldVehicleBId);
		if (!recordA || !recordB || recordA.m_bDeleted || recordB.m_bDeleted
			|| recordA.m_bDetached || recordB.m_bDetached)
			return false;
		carrier.m_vFieldVehicleAInitialPosition = recordA.m_vPosition;
		carrier.m_vFieldVehicleBInitialPosition = recordB.m_vPosition;
		carrier.m_vFieldVehicleAInitialAngles
			= HST_WorldPositionService.BuildUprightAnglesFromVector(
				recordA.m_vAngles);
		carrier.m_vFieldVehicleBInitialAngles
			= HST_WorldPositionService.BuildUprightAnglesFromVector(
				recordB.m_vAngles);
		evidence = "field vehicle captured transforms frozen into carrier";
		return true;
	}

	protected bool ResolveFixturePositions(
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		out string evidence)
	{
		evidence = "no isolated dry field vehicle fixture positions resolved";
		vector basePosition = HST_DefaultCatalog.GetHideoutPosition(
			HST_DefaultCatalog.GetDefaultHideoutId());
		if (IsZeroVector(basePosition))
			basePosition = HST_DefaultCatalog.GetEmergencySpawnPosition();
		for (int attempt = 0; attempt < 24; attempt++)
		{
			int rowOffset = attempt % 3;
			vector preferredA = basePosition;
			preferredA[0] = preferredA[0] + 140.0 + attempt * 24.0;
			preferredA[2] = preferredA[2] + 90.0 + rowOffset * 24.0;
			vector positionA;
			if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(
				preferredA,
				positionA,
				true))
				continue;
			vector preferredB = positionA;
			preferredB[0] = preferredB[0] + 10.0;
			vector positionB;
			if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(
				preferredB,
				positionB,
				true))
				continue;
			vector preferredMoved = positionA;
			preferredMoved[2] = preferredMoved[2] + 40.0;
			vector movedPosition;
			if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(
				preferredMoved,
				movedPosition,
				true))
				continue;
			if (DistanceSq2D(positionA, positionB) <= 64.0
				|| DistanceSq2D(positionA, movedPosition) <= 900.0
				|| DistanceSq2D(positionB, movedPosition) <= 900.0)
				continue;
			array<IEntity> roots = {};
			CollectMatchingRoots(positionA, FIELD_VEHICLE_PREFAB, roots);
			CollectMatchingRoots(positionB, FIELD_VEHICLE_PREFAB, roots);
			CollectMatchingRoots(movedPosition, FIELD_VEHICLE_PREFAB, roots);
			if (roots.Count() != 0)
				continue;
			carrier.m_vFieldVehicleAInitialPosition = positionA;
			carrier.m_vFieldVehicleBInitialPosition = positionB;
			carrier.m_vFieldVehicleAMovedPosition = movedPosition;
			evidence = "isolated dry field vehicle fixture positions exact";
			return true;
		}
		return false;
	}

	protected bool ValidateInitialPhysicalSnapshot(
		HST_CampaignState state,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		HST_PersistentFieldVehicleRuntimeService tracker,
		out HST_PersistentFieldVehicleProofObservation observation)
	{
		return ObserveFixture(state, carrier, tracker, 1, observation);
	}

	protected bool ValidateMutatedPhysicalSnapshot(
		HST_CampaignState state,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		HST_PersistentFieldVehicleRuntimeService tracker,
		out HST_PersistentFieldVehicleProofObservation observation)
	{
		observation = new HST_PersistentFieldVehicleProofObservation();
		observation.m_sEvidence
			= "field vehicle move/damage transition is still settling";
		if (!state || !carrier || !tracker)
			return false;
		IEntity movedRoot = tracker.ResolveEntityForRuntimeId(
			carrier.m_sFieldVehicleAId);
		IEntity destroyedRoot = m_DestroyedFixtureRoot;
		if (!movedRoot || !destroyedRoot || movedRoot == destroyedRoot
			|| !destroyedRoot.GetWorld()
			|| !PositionNear(
				movedRoot.GetOrigin(),
				carrier.m_vFieldVehicleAMovedPosition)
			|| !AnglesNear(
				HST_WorldPositionService.BuildUprightAnglesFromVector(
					movedRoot.GetYawPitchRoll()),
				carrier.m_vFieldVehicleAMovedAngles)
			|| tracker.ResolveEntityForRuntimeId(
				carrier.m_sFieldVehicleBId))
			return false;
		SCR_DamageManagerComponent damageManager
			= SCR_DamageManagerComponent.Cast(
				destroyedRoot.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager
			|| damageManager.GetState() != EDamageState.DESTROYED)
			return false;
		observation.m_bStateExact = true;
		observation.m_bPhysicalExact = true;
		observation.m_bCargoExact = true;
		observation.m_bNoDuplicateRoots = true;
		observation.m_sEvidence
			= "field vehicle movement and engine destruction exact";
		return true;
	}

	protected bool ValidateReplayPhysicalSnapshot(
		HST_CampaignState state,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		HST_PersistentFieldVehicleRuntimeService tracker,
		int generation,
		out HST_PersistentFieldVehicleProofObservation observation)
	{
		return ObserveFixture(
			state,
			carrier,
			tracker,
			generation,
			observation);
	}

	protected bool AcceptStableSample()
	{
		m_iStableSamples++;
		return m_iStableSamples >= STABLE_SAMPLE_COUNT;
	}

	protected void ReportPendingObservation(string evidence)
	{
		m_iPendingObservationFrames++;
		int reportRemainder = m_iPendingObservationFrames % 120;
		if (m_iPendingObservationFrames != 1 && reportRemainder != 0)
			return;
		Print(
			"Partisan ordinary campaign persistence proof | field vehicle pending | "
				+ evidence,
			LogLevel.WARNING);
	}

	protected bool BeginRecoveredMutation(
		HST_CampaignState state,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		HST_PersistentFieldVehicleRuntimeService tracker,
		out string evidence)
	{
		evidence = "field vehicle move/destruction input rejected";
		IEntity rootA = tracker.ResolveEntityForRuntimeId(
			carrier.m_sFieldVehicleAId);
		IEntity rootB = tracker.ResolveEntityForRuntimeId(
			carrier.m_sFieldVehicleBId);
		if (!rootA || !rootB || rootA == rootB)
			return false;
		SCR_DamageManagerComponent damageManager
			= SCR_DamageManagerComponent.Cast(
				rootB.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager || damageManager.GetState() == EDamageState.DESTROYED)
			return false;
		HST_WorldPositionService.ApplyUprightEntityTransform(
			rootA,
			carrier.m_vFieldVehicleAMovedPosition,
			carrier.m_vFieldVehicleAMovedAngles);
		m_DestroyedFixtureRoot = rootB;
		damageManager.Kill(Instigator.CreateInstigator(null));
		evidence = "field vehicle A moved and B received production damage kill";
		return true;
	}

	protected bool ObserveFixture(
		HST_CampaignState state,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		HST_PersistentFieldVehicleRuntimeService tracker,
		int generation,
		out HST_PersistentFieldVehicleProofObservation observation)
	{
		if (!ObserveLogicalFixture(state, carrier, generation, observation))
			return false;
		IEntity rootA = tracker.ResolveEntityForRuntimeId(
			carrier.m_sFieldVehicleAId);
		IEntity rootB = tracker.ResolveEntityForRuntimeId(
			carrier.m_sFieldVehicleBId);
		array<IEntity> rootsAtAInitial = {};
		array<IEntity> rootsAtBInitial = {};
		array<IEntity> rootsAtMoved = {};
		array<IEntity> rootsAtShutdown = {};
		CollectMatchingRoots(
			carrier.m_vFieldVehicleAInitialPosition,
			carrier.m_sFieldVehiclePrefab,
			rootsAtAInitial);
		CollectMatchingRoots(
			carrier.m_vFieldVehicleBInitialPosition,
			carrier.m_sFieldVehiclePrefab,
			rootsAtBInitial);
		CollectMatchingRoots(
			carrier.m_vFieldVehicleAMovedPosition,
			carrier.m_sFieldVehiclePrefab,
			rootsAtMoved);
		if (generation == 3)
		{
			CollectMatchingRoots(
				carrier.m_vFieldVehicleAShutdownPosition,
				carrier.m_sFieldVehiclePrefab,
				rootsAtShutdown);
		}
		array<IEntity> uniqueRoots = {};
		AppendUniqueRoots(uniqueRoots, rootsAtAInitial);
		AppendUniqueRoots(uniqueRoots, rootsAtBInitial);
		AppendUniqueRoots(uniqueRoots, rootsAtMoved);
		AppendUniqueRoots(uniqueRoots, rootsAtShutdown);
		observation.m_iLiveRoots = uniqueRoots.Count();
		HST_RuntimeVehicleState recordA = FindUniqueFixtureRecord(
			state,
			carrier.m_sFieldVehicleAId);
		HST_RuntimeVehicleState recordB = FindUniqueFixtureRecord(
			state,
			carrier.m_sFieldVehicleBId);
		if (generation == 1)
		{
			bool initialRootCountsExact = rootsAtAInitial.Count() == 1
				&& rootsAtBInitial.Count() == 1
				&& rootsAtMoved.Count() == 0;
			bool initialRootIdentityExact;
			if (initialRootCountsExact && rootA && rootB && rootA != rootB)
			{
				initialRootIdentityExact = rootsAtAInitial[0] == rootA
					&& rootsAtBInitial[0] == rootB;
			}
			bool initialTransformsExact = rootA && rootB
				&& PositionNear(rootA.GetOrigin(), recordA.m_vPosition)
				&& PositionNear(rootB.GetOrigin(), recordB.m_vPosition)
				&& AnglesNear(
					HST_WorldPositionService.BuildUprightAnglesFromVector(
						rootA.GetYawPitchRoll()),
					recordA.m_vAngles)
				&& AnglesNear(
					HST_WorldPositionService.BuildUprightAnglesFromVector(
						rootB.GetYawPitchRoll()),
					recordB.m_vAngles);
			bool initialBindingsExact
				= tracker.ResolveForEntity(state, rootA) == recordA
				&& tracker.ResolveForEntity(state, rootB) == recordB;
			observation.m_bPhysicalExact = initialRootCountsExact
				&& initialRootIdentityExact && initialTransformsExact
				&& initialBindingsExact;
			observation.m_bNoDuplicateRoots
				= observation.m_iLiveRoots == 2;
		}
		else
		{
			array<IEntity> expectedReplayRoots = rootsAtMoved;
			if (generation == 3)
				expectedReplayRoots = rootsAtShutdown;
			bool replayRootCountsExact = rootsAtAInitial.Count() == 0
				&& rootsAtBInitial.Count() == 0
				&& expectedReplayRoots.Count() == 1
				&& uniqueRoots.Count() == 1;
			bool replayRootIdentityExact;
			if (replayRootCountsExact && rootA && !rootB)
				replayRootIdentityExact = expectedReplayRoots[0] == rootA;
			bool replayTransformExact = rootA
				&& PositionNear(rootA.GetOrigin(), recordA.m_vPosition)
				&& AnglesNear(
					HST_WorldPositionService.BuildUprightAnglesFromVector(
						rootA.GetYawPitchRoll()),
					recordA.m_vAngles);
			observation.m_bPhysicalExact = replayRootCountsExact
				&& replayRootIdentityExact && replayTransformExact
				&& tracker.ResolveForEntity(state, rootA) == recordA;
			observation.m_bNoDuplicateRoots
				= observation.m_iLiveRoots == 1;
		}
		observation.m_sEvidence += string.Format(
			" | physical roots %1 | physical/duplicates %2/%3",
			observation.m_iLiveRoots,
			observation.m_bPhysicalExact,
			observation.m_bNoDuplicateRoots);
		return observation.AllExact();
	}

	protected bool ObserveLogicalFixture(
		HST_CampaignState state,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		int generation,
		out HST_PersistentFieldVehicleProofObservation observation)
	{
		observation = new HST_PersistentFieldVehicleProofObservation();
		observation.m_sEvidence = "field vehicle logical fixture rejected";
		if (!state || !carrier || generation < 1 || generation > 3)
			return false;
		HST_RuntimeVehicleState recordA;
		HST_RuntimeVehicleState recordB;
		int countA;
		int countB;
		string fixturePrefix = "hst_pfv_" + carrier.m_sPayloadNonce + "_";
		foreach (HST_RuntimeVehicleState record : state.m_aRuntimeVehicles)
		{
			if (!record || !record.m_sVehicleRuntimeId.StartsWith(fixturePrefix))
				continue;
			observation.m_iDurableRows++;
			if (record.m_sVehicleRuntimeId == carrier.m_sFieldVehicleAId)
			{
				countA++;
				recordA = record;
			}
			else if (record.m_sVehicleRuntimeId == carrier.m_sFieldVehicleBId)
			{
				countB++;
				recordB = record;
			}
		}
		if (recordA && recordA.m_bDeleted)
			observation.m_iDeletedRows++;
		if (recordB && recordB.m_bDeleted)
			observation.m_iDeletedRows++;

		vector expectedAPosition = carrier.m_vFieldVehicleAInitialPosition;
		vector expectedAAngles = carrier.m_vFieldVehicleAInitialAngles;
		if (generation >= 2)
		{
			expectedAPosition = carrier.m_vFieldVehicleAMovedPosition;
			expectedAAngles = carrier.m_vFieldVehicleAMovedAngles;
		}
		if (generation == 3)
		{
			expectedAPosition = carrier.m_vFieldVehicleAShutdownPosition;
			expectedAAngles = carrier.m_vFieldVehicleAShutdownAngles;
		}
		bool recordIdentityExact = recordA && recordB
			&& recordA.m_sPrefab == carrier.m_sFieldVehiclePrefab
			&& recordB.m_sPrefab == carrier.m_sFieldVehiclePrefab
			&& recordA.m_sRuntimeKind == "field_vehicle"
			&& recordB.m_sRuntimeKind == "field_vehicle";
		bool recordLifecycleExact = recordA && recordB
			&& !recordA.m_bDetached && !recordB.m_bDetached
			&& !recordA.m_bDeleted
			&& recordB.m_bDeleted == (generation >= 2);
		bool recordTransformsExact = recordA && recordB
			&& PositionNear(recordA.m_vPosition, expectedAPosition)
			&& PositionNear(
				recordB.m_vPosition,
				carrier.m_vFieldVehicleBInitialPosition)
			&& AnglesNear(recordA.m_vAngles, expectedAAngles)
			&& AnglesNear(
				recordB.m_vAngles,
				carrier.m_vFieldVehicleBInitialAngles);
		observation.m_bStateExact = observation.m_iDurableRows == 2
			&& countA == 1 && countB == 1
			&& recordIdentityExact && recordLifecycleExact
			&& recordTransformsExact;

		int cargoA;
		int cargoB;
		foreach (HST_VehicleCargoItemState cargo : state.m_aVehicleCargoItems)
		{
			if (!cargo || (cargo.m_sVehicleRuntimeId
				!= carrier.m_sFieldVehicleAId
				&& cargo.m_sVehicleRuntimeId
					!= carrier.m_sFieldVehicleBId))
				continue;
			observation.m_iCargoRows++;
			if (cargo.m_sVehicleRuntimeId == carrier.m_sFieldVehicleAId
				&& cargo.m_sItemPrefab == carrier.m_sFieldVehicleCargoPrefab
				&& cargo.m_iCount == carrier.m_iFieldVehicleACargoCount
				&& PositionNear(cargo.m_vLastVehiclePosition, expectedAPosition))
				cargoA++;
			if (cargo.m_sVehicleRuntimeId == carrier.m_sFieldVehicleBId
				&& cargo.m_sItemPrefab == carrier.m_sFieldVehicleCargoPrefab
				&& cargo.m_iCount == carrier.m_iFieldVehicleBCargoCount
				&& PositionNear(
					cargo.m_vLastVehiclePosition,
					carrier.m_vFieldVehicleBInitialPosition))
				cargoB++;
		}
		observation.m_bCargoExact = observation.m_iCargoRows == 2
			&& cargoA == 1 && cargoB == 1;
		observation.m_sEvidence = string.Format(
			"logical rows/deleted/cargo %1/%2/%3 | state/cargo %4/%5",
			observation.m_iDurableRows,
			observation.m_iDeletedRows,
			observation.m_iCargoRows,
			observation.m_bStateExact,
			observation.m_bCargoExact);
		return observation.m_bStateExact && observation.m_bCargoExact;
	}

	protected HST_RuntimeVehicleState FindUniqueFixtureRecord(
		HST_CampaignState state,
		string runtimeId)
	{
		HST_RuntimeVehicleState result;
		int count;
		if (!state || runtimeId.IsEmpty())
			return null;
		foreach (HST_RuntimeVehicleState record : state.m_aRuntimeVehicles)
		{
			if (!record || record.m_sVehicleRuntimeId != runtimeId)
				continue;
			result = record;
			count++;
		}
		if (count != 1)
			return null;
		return result;
	}

	protected bool ValidateRestoreReceipt(
		HST_PersistentFieldVehicleRestoreResult result,
		int eligibleRows,
		int inactiveRows,
		out string evidence)
	{
		evidence = "field vehicle startup restore receipt rejected";
		if (!result || !result.AllExact()
			|| result.m_iEligibleRows != eligibleRows
			|| result.m_iInactiveRows != inactiveRows
			|| result.RestoredRootCount() != eligibleRows
			|| result.m_iTrackedRoots != eligibleRows)
			return false;
		evidence = "field vehicle startup restore receipt exact | "
			+ result.BuildReport();
		return true;
	}

	protected bool HasFixtureResidue(
		HST_CampaignState state,
		HST_OrdinaryCampaignPersistenceCarrier carrier)
	{
		if (!state || !carrier)
			return true;
		string prefix = "hst_pfv_" + carrier.m_sPayloadNonce + "_";
		foreach (HST_RuntimeVehicleState record : state.m_aRuntimeVehicles)
		{
			if (record && record.m_sVehicleRuntimeId.StartsWith(prefix))
				return true;
		}
		foreach (HST_VehicleCargoItemState cargo : state.m_aVehicleCargoItems)
		{
			if (cargo && cargo.m_sVehicleRuntimeId.StartsWith(prefix))
				return true;
		}
		return false;
	}

	protected void CollectMatchingRoots(
		vector position,
		string prefab,
		array<IEntity> roots)
	{
		if (!roots || IsZeroVector(position) || prefab.IsEmpty())
			return;
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		m_aQueryEntities.Clear();
		world.QueryEntitiesBySphere(
			position,
			ROOT_QUERY_RADIUS_METERS,
			AddQueryEntity,
			null,
			EQueryEntitiesFlags.ALL);
		string expected = NormalizePrefabResource(prefab);
		foreach (IEntity candidate : m_aQueryEntities)
		{
			if (!candidate || !candidate.GetWorld()
				|| DistanceSq2D(candidate.GetOrigin(), position)
					> ROOT_QUERY_RADIUS_METERS * ROOT_QUERY_RADIUS_METERS
				|| !HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(
					HST_VehicleRootPolicy.ResolveEntityPrefabName(candidate))
				|| NormalizePrefabResource(
					HST_VehicleRootPolicy.ResolveEntityPrefabName(candidate))
					!= expected)
				continue;
			if (roots.Find(candidate) < 0)
				roots.Insert(candidate);
		}
	}

	protected bool AddQueryEntity(IEntity entity)
	{
		if (entity && m_aQueryEntities.Count() < MAX_QUERY_ENTITIES)
			m_aQueryEntities.Insert(entity);
		return m_aQueryEntities.Count() < MAX_QUERY_ENTITIES;
	}

	protected void AppendUniqueRoots(
		array<IEntity> destination,
		array<IEntity> source)
	{
		if (!destination || !source)
			return;
		foreach (IEntity root : source)
		{
			if (root && destination.Find(root) < 0)
				destination.Insert(root);
		}
	}

	protected string NormalizePrefabResource(string prefab)
	{
		string normalized = prefab.Trim();
		int boundary = normalized.IndexOf("}");
		if (boundary >= 0 && boundary + 1 < normalized.Length())
		{
			normalized = normalized.Substring(
				boundary + 1,
				normalized.Length() - boundary - 1);
		}
		normalized.ToLower();
		return normalized;
	}

	protected bool PositionNear(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dy = left[1] - right[1];
		float dz = left[2] - right[2];
		return dx * dx + dy * dy + dz * dz
			<= POSITION_TOLERANCE_METERS * POSITION_TOLERANCE_METERS;
	}

	protected bool AnglesNear(vector left, vector right)
	{
		float leftYaw = HST_WorldPositionService.NormalizeYaw(left[0]);
		float rightYaw = HST_WorldPositionService.NormalizeYaw(right[0]);
		float difference = Math.AbsFloat(leftYaw - rightYaw);
		if (difference > 180.0)
			difference = 360.0 - difference;
		return difference <= 2.0
			&& Math.AbsFloat(left[1] - right[1]) <= 2.0
			&& Math.AbsFloat(left[2] - right[2]) <= 2.0;
	}

	protected float DistanceSq2D(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return dx * dx + dz * dz;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}
}
